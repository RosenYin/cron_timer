#include "cron_timer.h"
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <assert.h>
#include <time.h>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <thread>
#include <cstdarg>
#include "cron_log.h"
namespace cron_timer {
// 是否启用UTC时间
#define USE_UTC 0

/**
 * @brief Get the Max M Day From Current Month object
 * 
 * @return int64_t 
 */
int64_t GetMaxMDayFromCurrentMonth(){
	// 获取当前系统时间
    auto now = std::chrono::system_clock::now();
    // 将当前时间转换为时间结构体
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo;
    #if USE_UTC==0
    #ifdef _WIN32
        localtime_s(&timeInfo, &currentTime); 
    #else
        localtime_r(&currentTime, &timeInfo); // 获取当前本地时间，存在currentTime中
    #endif // _WIN32
    #else
    gmtime_r(&currentTime, &timeInfo);//将currentTime转化为tm
    #endif
    // 获取当前月份的最大日期
    // 注意：当前月份的最大日期不同于当前月份的天数，因为某些月份的最后几天可能不是完整的一周
    timeInfo.tm_mday = 1; // 设置为月初
    timeInfo.tm_mon++;    // 切换到下个月
    timeInfo.tm_mday = 0; // 设置为前一天，即当前月份的最后一天
    #if USE_UTC==0
    // 将时间结构体转换回时间点
    auto maxDate = std::chrono::system_clock::from_time_t(std::mktime(&timeInfo));
    // 将时间点转换为日期
    auto maxDateInTimeT = std::chrono::system_clock::to_time_t(maxDate);
    const std::tm* maxDateInfo = std::localtime(&maxDateInTimeT);
    #else
    // 将时间结构体转换回时间点
    auto maxDate = std::chrono::system_clock::from_time_t(std::timegm(&timeInfo));
    // 将时间点转换为日期
    auto maxDateInTimeT = std::chrono::system_clock::to_time_t(maxDate);
    const std::tm* maxDateInfo = std::localtime(&maxDateInTimeT);
    #endif
    // 打印最大日期
    // std::cout << "Current month's max date: " << maxDateInfo->tm_mday << std::endl;

    return maxDateInfo->tm_mday;
}
/**
 * @brief Get the Latest M Day With Year Month Week object
 *        通过 年、月、周 获取临近的 日期，注意年月周为正常数值而非tm类型
 *        返回值会有大于当前月份最大日期的时候，没有做溢出判断，因为mktime
 *        和timegm这两个函数会自动计算从而推断到下个月
 * 
 * @param year 
 * @param month 
 * @param weekend 
 * @return int64_t 
 */
int64_t GetLatestMDayWithYearMonthWeek(int year, int month, int weekend){
	auto cur_time_ = std::chrono::system_clock::now();//获取当前chrono时间
	time_t time_now = std::chrono::system_clock::to_time_t(cur_time_);//转换chrono时间为time_t
	struct tm cur_utc_time;//创建当前utc时间结构体，用来存放转化成utc的时间
    #if USE_UTC==0
    #ifdef _WIN32
        localtime_s(&cur_utc_time, &time_now); 
    #else
        localtime_r(&time_now, &cur_utc_time); // 获取当前本地时间，存在cur_utc_time中
    #endif // _WIN32
    #else
    gmtime_r(&time_now, &cur_utc_time);//将time_now转化为tm
    #endif
	tm cur_time;
	memset(&cur_time, 0, sizeof(cur_time));
	cur_time.tm_year = year - 1900;
	cur_time.tm_mon = month-1;
	if(cur_time.tm_mon != cur_utc_time.tm_mon || cur_time.tm_year > cur_utc_time.tm_year)
		cur_time.tm_mday = 1;
	else cur_time.tm_mday = cur_utc_time.tm_mday;

    #if USE_UTC==0
    mktime(&cur_time);
    #else
    timegm(&cur_time);
    #endif
    uint64_t currnet_mday;
	if(weekend >= cur_time.tm_wday)
		currnet_mday =  cur_time.tm_mday + weekend - cur_time.tm_wday;
	else
		currnet_mday =  7 - cur_time.tm_wday + weekend + cur_time.tm_mday;
	return currnet_mday;
}
/**
 * @brief 
 * 继承自 std::enable_shared_from_this 类，以便支持在回调函数中获得自身的 shared_ptr。
 * 用来创建定时器对象，并支持取消定时器以及执行回调函数。
 * 派生类可以通过实现 DoFunc 函数来定义具体的行为，以便在计时器触发时执行相应的操作。
 * 这个类的设计允许与 TimerMgr 类协同工作，用于管理和调度计时器。
 * 
 */
BaseTimer::BaseTimer(TimerMgr& owner, FUNC_CALLBACK&& func)
    : owner_(owner)
    , func_(std::move(func))
    , is_in_list_(false) {}

BaseTimer::BaseTimer(TimerMgr& owner, FUNC_CALLBACK&& func, int id)
    : owner_(owner)
    , func_(std::move(func))
    , is_in_list_(false) 
    , id_(id) {}
BaseTimer::~BaseTimer() {}
/**
 * @brief 取消任务(将任务移除)
 * 
 */
void BaseTimer::Cancel() {
	if (!GetIsInList()) {
		return;
	}
	//创建一个 shared_ptr 对象 self，以确保在执行取消操作期间计时器对象不会被销毁。
	//这是通过调用 shared_from_this() 来获得的
	auto self = shared_from_this();
	//将计时器从 TimerMgr 对象的列表中移除
	owner_.remove(self);
}
/**
 * @brief 将当前时间轮索引的时间，与当前时间做差，如果时间差值大于-1,返回true
 * 
 * @return true 
 * @return false 
 */
bool BaseTimer::compareCurWheelIndexTime(){
    std::chrono::system_clock::time_point startTime = GetWheelCurIndexTime();
    auto currentTime = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = startTime  - currentTime;
    if(diff.count() > -1) return true;
    else return false;
}

int BaseTimer::SetID(int id){
    id_ = id;
    return id_;
}

int BaseTimer::GetID(){
    return id_;
}
//返回计时器在列表中的迭代器引用。
std::list<TimerPtr>::iterator& BaseTimer::GetIt() { return it_; } 
//设置计时器在列表中的迭代器。
void BaseTimer::SetIt(const std::list<TimerPtr>::iterator& it) { it_ = it; } 
//返回计时器是否在列表中。
bool BaseTimer::GetIsInList() const { return is_in_list_; } 
//设置计时器是否在列表中。
void BaseTimer::SetIsInList(bool b) { is_in_list_ = b; } 
//执行回调函数并将时间轮索引向前推进
void BaseTimer::DoFunc(){};

// std::chrono::system_clock::time_point BaseTimer::GetWheelCurIndexTime() const{};

/**
 * @brief Construct a new Cron Timer:: Cron Timer object
 * 
 * @param owner 
 * @param wheels 
 * @param func 
 * @param count 
 */
CronTimer::CronTimer(TimerMgr& owner, std::vector<CronWheel>&& wheels, FUNC_CALLBACK&& func, int count)
    : BaseTimer(owner, std::move(func))
    , wheels_(std::move(wheels))
    , over_flowed_(false)
    , count_left_(count){}

CronTimer::CronTimer(TimerMgr& owner, std::vector<CronWheel>&& wheels, FUNC_CALLBACK&& func, int count, int id)
    : BaseTimer(owner, std::move(func), id)
    , wheels_(std::move(wheels))
    , over_flowed_(false)
    , count_left_(count){}
/**
 * @brief 初始化时间轮的索引指向为当前或未来最近时间
 * 
 */
void CronTimer::InitWheelIndex(){
    std::cout << std::endl;
    std::cout<<"初始化wheel索引....."<<std::endl;
    // 获取当前时间列表
    std::vector<int> cur_time = TimerMgr::GetNowTimeConvertVetcor();
    std::cout << std::endl;
    for (std::vector<int>::iterator i = cur_time.begin(); i != cur_time.end(); i++){
        std::cout<<*i<<std::endl;
    }
    std::cout << std::endl;
    // 创建时间轮标志位，用来判断上一个时间轮是否大于或者小于当前时间
    // 如果为true则应该令之后的时间轮的索引指向为最初(0)
    bool last_wheel_less_all = false;
	bool cur_wheel_large = false;
    // 遍历cron表达式的时间轮们，按照优先级查看，年>月>周>日>时>分>秒
    for (int i = CronExpression::DT_MAX-1; i >= 0; i--) {
        // 获取当前i类型的时间轮，类型如text处理中的枚举变量所示
        auto& wheel = wheels_[i];
        for (size_t j =0; j < wheel.values.size(); ++j) {
            wheel.cur_index = j; //初始化索引
            // 如果时间轮中的数值为-1,跳过，为了掠过周和日不设定值的情况
            if(wheel.values[wheel.cur_index] == -1) break;
            // 如果上一个类型时间轮的0号索引大于当前时间，
            // 或上一个类型时间轮的所有数值小于当前时间，
            // 跳过当前，之后的所有都跳过，并且时间轮索引为0
            if(cur_wheel_large || last_wheel_less_all)
                break;
            if(wheel.values[wheel.cur_index] == cur_time[i])
                break;
            else if(wheel.values[wheel.cur_index] > cur_time[i]){
                cur_wheel_large = true;
                if(i == CronExpression::DT_WEEK){
                    uint64_t cur_mday = 
                    GetLatestMDayWithYearMonthWeek(wheels_[CronExpression::DT_YEAR].values[wheels_[CronExpression::DT_YEAR].cur_index]
                                                    ,wheels_[CronExpression::DT_MONTH].values[wheels_[CronExpression::DT_MONTH].cur_index]
                                                    ,wheels_[CronExpression::DT_WEEK].values[wheels_[CronExpression::DT_WEEK].cur_index]);
                    if(cur_mday <= GetMaxMDayFromCurrentMonth())
                        break;
                    if(wheels_[i+1].cur_index < wheels_[i+1].values.size()-1){
                        ++wheels_[i+1].cur_index;
                    }
                }
                
                break;
            }
            else if(wheel.values[wheel.cur_index] < cur_time[i]){
                // 若当前时间轮的数值小于当前时间并且索引指向了时间轮列表的最后一位，
                // 将当前时间轮的索引重新指向开头，并令上一个时间轮的索引向下移动一位
                if(wheel.cur_index == wheel.values.size() - 1)
                {
                    wheel.cur_index = 0;
                    if(i == CronExpression::DT_WEEK){
                        uint64_t cur_mday = 
                        GetLatestMDayWithYearMonthWeek(wheels_[CronExpression::DT_YEAR].values[wheels_[CronExpression::DT_YEAR].cur_index]
                                                    ,wheels_[CronExpression::DT_MONTH].values[wheels_[CronExpression::DT_MONTH].cur_index]
                                                    ,wheels_[CronExpression::DT_WEEK].values[wheels_[CronExpression::DT_WEEK].cur_index]);
                        if(cur_mday <= GetMaxMDayFromCurrentMonth())
                            break;
                    }
                    // 只有当上一个时间轮的上一次索引不是时间轮数值列表最后一位时才向前移动
                    if(wheels_[i+1].cur_index < wheels_[i+1].values.size()-1){
                        ++wheels_[i+1].cur_index;
                        last_wheel_less_all = true;
                    }
                    // 不管上一个时间轮当前索引数值如何都跳出循环
                    break;
                }
            }
        }
    }
    for (std::vector<CronWheel>::iterator i = wheels_.begin(); i != wheels_.end(); i++){
        std::cout<<(*i).cur_index << " - "<<(*i).values[(*i).cur_index] << std::endl;
    }
    std::cout << std::endl;
}

void CronTimer::DoFunc() {
	// 当任务在列表中时
	if (GetIsInList()) {
        // Log("---------任务在列表中-------");
        // 只有当前时间轮索引指向的时间与当前时间差值大于-1才执行回调函数
        if(compareCurWheelIndexTime() && count_left_ != 0){
            // Log("---------执行回调-------");
	        func_();
            // Log("---------执行完毕-------");
        }
		auto self = shared_from_this();
		bool error = owner_.remove(self); // 将当前计时器从计时器管理器的列表中移除
        // std::cout << "是否移除成功：" << error << std::endl;
        if(!error)
            Log("移除失败");
		Next(CronExpression::DT_SECOND); // 将时间字段前进到下一个时间点，以便在下一个时间点触发任务,从s开始
		// 检查是否需要继续触发计时器
		if ((count_left_ == TimerMgr::RUN_FOREVER || --count_left_ > 0) && !over_flowed_) {
            // Log("---------即将重新插入-------");
			owner_.insert(self); // 将计时器重新插入计时器管理器的列表中，以便在下一个时间点再次触发
            // Log("---------重新插入完毕-------");
		}
	}
    // Log("任务不在列表");
}
/**
 * @brief Get the Cur Time object
 * 用于获取当前的时间点，返回一个 std::chrono::system_clock::time_point 对象，表示下一个 Cron 时间点。
 * 
 * @return std::chrono::system_clock::time_point 
 */
std::chrono::system_clock::time_point CronTimer::GetWheelCurIndexTime() const{
    tm next_tm;
    memset(&next_tm, 0, sizeof(next_tm));
    next_tm.tm_sec = GetCurValue(CronExpression::DT_SECOND);
    next_tm.tm_min = GetCurValue(CronExpression::DT_MINUTE);
    next_tm.tm_hour = GetCurValue(CronExpression::DT_HOUR);
    if( GetCurValue(CronExpression::DT_DAY_OF_MONTH ) == -1)
		next_tm.tm_mday = GetLatestMDayWithYearMonthWeek(GetCurValue(CronExpression::DT_YEAR)
                                                        ,GetCurValue(CronExpression::DT_MONTH)
                                                        ,GetCurValue(CronExpression::DT_WEEK));
    else next_tm.tm_mday = GetCurValue(CronExpression::DT_DAY_OF_MONTH);
    if(GetCurValue(CronExpression::DT_WEEK)!=-1)
		next_tm.tm_wday = GetCurValue(CronExpression::DT_WEEK);
    next_tm.tm_mon = GetCurValue(CronExpression::DT_MONTH) - 1;
    next_tm.tm_year = GetCurValue(CronExpression::DT_YEAR) - 1900;
        // std::cout << "wheel时间 年 为：  " << next_tm.tm_year + 1900 << std::endl;
		// std::cout << "wheel时间 周 为：  " << next_tm.tm_wday << std::endl;
		// std::cout << "wheel时间 月 为：  " << next_tm.tm_mon+1  << std::endl;
		// std::cout << "wheel时间 日 为：  " << next_tm.tm_mday << std::endl;
		// std::cout << "wheel时间 时 为：  " << next_tm.tm_hour << std::endl;
		// std::cout << "wheel时间 分 为：  " << next_tm.tm_min << std::endl;
		// std::cout << "wheel时间 秒 为：  " << next_tm.tm_sec << std::endl;
    #if USE_UTC == 1
        time_t wheel_time_t = timegm(&next_tm);
    #else
        time_t wheel_time_t = mktime(&next_tm);
    #endif
        // std::cout << "wheel时间为：  " << next_tm.tm_year + 1900 << std::endl;
		// std::cout << "wheel时间为：  " << next_tm.tm_wday << std::endl;
		// std::cout << "wheel时间为：  " << next_tm.tm_mon+1  << std::endl;
		// std::cout << "wheel时间为：  " << next_tm.tm_mday << std::endl;
		// std::cout << "wheel时间为：  " << next_tm.tm_hour << std::endl;
		// std::cout << "wheel时间为：  " << next_tm.tm_min << std::endl;
		// std::cout << "wheel时间为：  " << next_tm.tm_sec << std::endl;
		// std::cout <<"judge func wheel_time_t: " <<wheel_time_t << std::endl;
    return std::chrono::system_clock::from_time_t(wheel_time_t);
}

/**
 * @brief 用于将时间字段前进到下一个时间点，以便在特定时间点触发任务。
 * 它递归调用自身，依次前进每个时间字段。
 * 如果某个时间字段溢出（超过了有效范围），则标记 over_flowed_ 为 true，表示定时器已失效。
 * 
 * @param data_type 
 */
void CronTimer::Next(int data_type) {
    if (data_type >= CronExpression::DT_MAX) {
        // 溢出了表明此定时器已经失效，不应该再被执行
        over_flowed_ = true;
        return;
    }

    auto& wheel = wheels_[data_type];
    if (wheel.cur_index == wheel.values.size() - 1) {
        wheel.cur_index = 0;
        Next(data_type + 1);
    } else {
        ++wheel.cur_index;
    }
}
	
/**
 * @brief Get the Cur Value object
 * 用于获取当前时间字段的值。
 * 
 * @param data_type 
 * @return int 
 */
int CronTimer::GetCurValue(int data_type) const {
    const auto& wheel = wheels_[data_type];
    return wheel.values[wheel.cur_index];
}


/**
 * @brief 继承自 BaseTimer 类,用于在一定延迟之后执行回调函数
 * 
 */

LaterTimer::LaterTimer(TimerMgr& owner, int milliseconds, FUNC_CALLBACK&& func, int count)
    : BaseTimer(owner, std::move(func))
    , mill_seconds_(milliseconds)
    , count_left_(count)
    , cur_time_(std::chrono::system_clock::now())
{
    Next();
}
LaterTimer::LaterTimer(TimerMgr& owner, int milliseconds, FUNC_CALLBACK&& func, int count, int id)
    : BaseTimer(owner, std::move(func), id)
    , mill_seconds_(milliseconds)
    , count_left_(count)
    , cur_time_(std::chrono::system_clock::now())
{
    Next();
}
/**
 * @brief 执行函动作，并将任务移除，判断任务执行次数或者执行模式是否是永久执行，重新将任务插入到列表中
 * 
 */
void LaterTimer::DoFunc() {
	if(compareCurWheelIndexTime())
	    func_();
	// 可能用户在定时器中取消了自己
	if (GetIsInList()) {
		auto self = shared_from_this();
		owner_.remove(self);

		if (count_left_ <= TimerMgr::RUN_FOREVER || --count_left_ > 0) {
			Next();
			owner_.insert(self);
		}
	}
}
/**
 * @brief 用于获取当前的时间点，返回一个 std::chrono::system_clock::time_point 对象，表示下一个触发时间点
 * 
 * @return std::chrono::system_clock::time_point 
 */
std::chrono::system_clock::time_point LaterTimer::GetWheelCurIndexTime() const{
    return cur_time_; 
}

//前进到下一格
void LaterTimer::Next() {
    // 获取当前时间
    auto time_now = std::chrono::system_clock::now();
    while (true) {
        // 不断递增 cur_time_ 成员，直到其值大于当前系统时间为止，以确保下一个触发时间点在未来。
        // 这个方法会不断增加 cur_time_ 的值，直到满足延迟条件。
        cur_time_ += std::chrono::milliseconds(mill_seconds_);
        if (cur_time_ > time_now) {
            break;
        }
    }
}


/**
 * @brief 用于管理定时器（包括 Cron 表达式定时器和延时定时器）并提供定时器管理的功能
 * 
 */
TimerMgr::TimerMgr() {}
// TimerMgr::TimerMgr(const TimerMgr&) = delete;
// const TimerMgr& TimerMgr::operator=(const TimerMgr&) = delete;

/**
 * @brief 用于停止所有定时器的执行。
 * 它会清空定时器列表，并将 stopped_ 标记设置为 true，表示停止所有定时器。
 * 这将阻止新的定时器添加到管理器中。
 * 
 */
void TimerMgr::Stop() {
    timers_.clear();
    stopped_ = true;
}

/**
 * @brief 添加一个新的 Cron 表达式定时器,缺省永远执行
 * 
 * @param timer_string Cron 表达式字符串
 * @param func 回调函数
 * @param count 定时器执行次数
 * @return TimerPtr 
 */
TimerPtr TimerMgr::AddTimer(const std::string& timer_string, FUNC_CALLBACK&& func, int id, int count) {
    if (stopped_)
        return nullptr;
    // 分割字串然后存在容器v中，并检查长度是否是6或者7
    std::vector<std::string> v;
    // 将cron表达式按照自定义优先级列表分割，并按照顺序push入列表
    // 当前优先级：年>月>周>日>时>分>秒，cron表达式顺序"秒 分 时 日 月 周 (年)"
    Text::SortWithSelfPrioritySplitStr(v, timer_string, ' ');
    
    // 若表达式长度不为7也不为6时，断言
    if (v.size() != CronExpression::DT_MAX) {
        if(v.size() != CronExpression::DT_YEAR){
            assert(("表达式字段数量既不为7也不为6",false));
            return nullptr;
        }else if(v.size() == CronExpression::DT_YEAR){
            // 若cron表达式长度为6,自动在其后面增加一位*,表示每年
            v.emplace_back("*");
        }
    }
    // 周和日所在的字段不可以同时为'?'或者同时不为'?'
    if(v[CronExpression::DT_DAY_OF_MONTH]=="?" && v[CronExpression::DT_WEEK]=="?"){
        assert(("日期和星期不可以同时不设定值",false));
        return nullptr;
    }else if (v[CronExpression::DT_DAY_OF_MONTH]!="?" && v[CronExpression::DT_WEEK]!="?"){
        assert(("日期和星期不可以同时有数值",false));
        return nullptr;
    }
    // 创建一个存储每个字段类型的时间轮列表
    // (按照优先级存入的，因为先前按照优先级将分割后的字符串存入字符串列表中)
    std::vector<CronWheel> wheels;
    for (int i = 0; i < CronExpression::DT_MAX; i++) {
        const auto& expression = v[i];// 获取当前字段的字符串
        CronExpression::DATA_TYPE data_type = CronExpression::DATA_TYPE(i);//转换类型
        CronWheel wheel(data_type);//有参构造时间轮类
        // 按照规则将字符串展开为数值，存入wheel.values中
        if (!CronExpression::GetValues(expression, data_type, wheel.values)) {
            assert(("表达式数值有问题",false));
            return nullptr;
        }
        wheel.SetMaxVal();//设定时间轮中的最大值
        wheel.SetMinVal();//设定时间轮中的最小值
        wheels.emplace_back(wheel);//尾插入时间轮列表中
    }

        std::cout<<std::endl;
		for (std::vector<CronWheel>::iterator it = wheels.begin(); it != wheels.end(); it++)
		{
			for (std::vector<int>::iterator it1 = (*it).values.begin(); it1 != (*it).values.end(); it1++)
			{
				std::cout << (*it1) << " ";
				
			}
			std::cout <<"wheel_type:" <<(*it).GetWheelType()  << "  ";
			std::cout <<"wheel_max:" <<(*it).max_value  << "  ";
			std::cout <<"wheel_min:" <<(*it).min_value  << "  ";
			std::cout <<std::endl<< "---------------------------";
			std::cout << std::endl;
		}
		std::cout << "<<<<<<<<<<<<<<<<<<<<<--------------------------->>>>>>>>>>>>>>>>>>>>>";
		std::cout << std::endl;

    //创建 CronTimer 对象，并将其插入到定时器管理器中，最后返回该定时器对象的指针。
    bool time_reasonable_ = compareMaxSetTime(wheels);
    if(time_reasonable_)
    {
        bool isWheelsDuplicate;
        std::vector<int>::iterator it = find(id_.begin(), id_.end(), id);
        if (it == id_.end()) {
           isWheelsDuplicate = false; 
        }else {
            isWheelsDuplicate = true;
        }
        if(!isWheelsDuplicate){
            wheels_gather_.emplace_back(wheels);
            id_.emplace_back(id);
            auto p = std::make_shared<CronTimer>(*this, std::move(wheels), std::move(func), count, id);
            id_pointer.insert(std::make_pair(id, p));
            p->InitWheelIndex();
            p->Next(CronExpression::DT_SECOND);
            insert(p);
            return p;
        }
    }
    return nullptr;
}
/**
 * @brief 新增一个延时执行的定时器，缺省运行一次
 * 
 * @param milliseconds 延迟执行的毫秒数
 * @param func 回调函数
 * @param count 定时器执行次数
 * @return TimerPtr 
 */
TimerPtr TimerMgr::AddDelayTimer(int milliseconds, FUNC_CALLBACK&& func, int count) {
    if (stopped_) {
        return nullptr;
    }

    assert(("设定的时间段需要大于0！！",milliseconds > 0));
    milliseconds = (std::max)(milliseconds, 1); //至少延迟 1 毫秒
    //创建 LaterTimer 对象，用于延时执行，然后将其插入到定时器管理器中，最后返回该定时器对象的指针。
    auto p = std::make_shared<LaterTimer>(*this, milliseconds, std::move(func), count);
    p->Next();
    insert(p);
    return p;
}

bool TimerMgr::RemoveAppointedTimer(int id) {
    auto it = id_pointer.find(id);
    if (it == id_pointer.end()) {
        assert(("不存在该ID任务",false));
        return false;
    }
    it->second->Cancel();
    return true;
}

// 获取最接近的触发时间点
std::chrono::system_clock::time_point TimerMgr::GetNearestTime() {
    auto it = timers_.begin();
    
    // 检查定时器列表 timers_ 是否为空，如果为空，返回一个最大的时间点
    if (it == timers_.end()) {
        return (std::chrono::system_clock::time_point::max)();
    } else {//否则，返回定时器列表中第一个定时器的触发时间点，即最接近的时间点。
        return it->first;
    }
}

size_t TimerMgr::Update() {
    //获取当前系统时间 time_now。
    auto time_now = std::chrono::system_clock::now();
    size_t count = 0;
    // std::cout << "update timers 长度-----为： "<< timers_.size() << std::endl;
    // std::cout << "update list 长度-----为： "<< timers_.begin()->second.size() << std::endl;
    auto it_ = timers_.begin();
    
    // std::cout << "时间列表中时间为： "<< TimePointConvertInteger(it_->first) << std::endl;
    for (auto it = timers_.begin(); it != timers_.end();) {
        auto expire_time = it->first;
        auto& timer_list = it->second;
        if (expire_time > time_now) {
            // Log("----------------------------------------时间大于");
            break;
        }

        // 如果某个定时器任务的触发时间小于当前系统时间，执行该任务
        while (!timer_list.empty()) {
            // std::cout << "update timers_list 长度===__1---为： "<< timer_list.size() << std::endl;
            auto p = *timer_list.begin();
            // Log("即将执行回调");
            p->DoFunc();
            // Log("执行完毕");
            // std::cout << "update timers_list 长度===__1---为： "<< timer_list.size() << std::endl;
            ++count;
        }
        // 将其从列表中移除。
        // std::cout << "update timers 长度++++为： "<< timers_.size() << std::endl;
        it = timers_.erase(it);
        // std::cout << "update timers 长度 为： "<< timers_.size() << std::endl;
        // Log("============移除");
    }
    // 返回执行的任务数量
    // std::cout << "update 执行次数为： "<< count << std::endl;
    // std::cout << "update timers 长度-============为： "<< timers_.size() << std::endl;
    return count;
}

int64_t TimerMgr::TimePointConvertInteger(std::chrono::system_clock::time_point time_point){
    auto currentTime = time_point;
    currentTime.time_since_epoch().count();
    auto epochTime = currentTime.time_since_epoch(); // 当前时间点与纪元时间点之间的持续时间
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epochTime).count();
    return seconds;
}

void TimerMgr::insert(const TimerPtr& p) {
    if(p->GetIsInList())
        Log("要插入的定时器已经存在！");
    assert(("要插入的定时器已经存在！",!p->GetIsInList()));
    // 获取定时器的触发时间点 t。
    auto t = p->GetWheelCurIndexTime();
    // 查找 timers_ 中是否已经存在该触发时间点
    auto it = timers_.find(t);
    // 如果不存在，创建一个空的定时器列表。
    if (it == timers_.end()) {
        // Log("+++++++++++++++++++++++新建一个定时器++++++++++++++++++++++++++++++");
        std::list<TimerPtr> l;
        timers_.insert(std::make_pair(t, l));
        it = timers_.find(t);
        // std::cout << "insert  时间列表中时间 it 为： "<< TimePointConvertInteger(it->first) << std::endl;
    }
    // std::cout << "insert timers 长度 为： "<< timers_.size() << std::endl;
    for (auto it_ : timers_)
    {
        /* code */
        // std::cout << "insert  时间列表中时间  为： "<< TimePointConvertInteger(it_.first) << std::endl;
    }
    
    // 将定时器插入到定时器列表中，然后设置相应的标志，表示已经在列表中。
    auto& l = it->second;
    p->SetIt(l.insert(l.end(), p));
    // std::cout << "insert  list长度  为： "<< l.size() << std::endl;
    p->SetIsInList(true);

}

bool TimerMgr::remove(const TimerPtr& p) {
    if(!p->GetIsInList())
        Log("要移除的定时器不存在！");
    assert(("要移除的定时器不存在！",p->GetIsInList()));
    // 获取定时器的触发时间点 t。
    auto t = p->GetWheelCurIndexTime();
    // 查找 timers_ 中是否存在该触发时间点，以及是否存在定时器列表。
    // std::cout << "remove timers 长度++++为： "<< timers_.size() << std::endl;
    auto it = timers_.find(t);
    if (it == timers_.end()) {
        // Log("timers_中不存在触发时间点");
        assert(("timers_中不存在触发时间点",false));
        return false;
    }

    auto& l = it->second;
    if (p->GetIt() == l.end()) {
        // Log("定时器列表为空");
        assert(("定时器列表为空",false));
        return false;
    }
    // 从定时器列表中移除定时器，并更新相应的标志
    l.erase(p->GetIt());
    // std::cout << "remove l 长度++++为： "<< l.size() << std::endl;
    p->SetIt(l.end());
    p->SetIsInList(false);
    // Log("移除完毕");
    return true;

}

bool TimerMgr::compareMaxSetTime(const TimerPtr& p){
    return p->compareCurWheelIndexTime();
}
/**
 * @brief 获取时间轮中最大值并解析时间，与当前时间作差，判断差值是否大于-1,大于则返回true
 * 
 * @param wheels 
 * @return true 
 * @return false 
 */
bool TimerMgr::compareMaxSetTime(const std::vector<CronWheel>& wheels){
    const std::chrono::system_clock::time_point startTime = GetWheelsMaxTimePoint(wheels);
    auto currentTime = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = startTime  - currentTime;
    // std::cout <<  "时间轮最大时间与当前时间差值： " <<std::fixed<<std::setprecision(5)<< diff.count() << std::endl;
    if(diff.count() > -1) return true;
    else return false;
}
/**
 * @brief 获取时间轮中的最大值，并解析为时间，返回时间轮中的最大秒数
 * 
 * @param wheels 
 * @return std::chrono::system_clock::time_point 
 */
std::chrono::system_clock::time_point TimerMgr::GetWheelsMaxTimePoint(std::vector<CronWheel> wheels){
    tm next_tm;
    memset(&next_tm, 0, sizeof(next_tm));
    next_tm.tm_sec = wheels[CronExpression::DATA_TYPE::DT_SECOND].max_value;
    next_tm.tm_min = wheels[CronExpression::DATA_TYPE::DT_MINUTE].max_value;
    next_tm.tm_hour = wheels[CronExpression::DATA_TYPE::DT_HOUR].max_value;
    if(wheels[CronExpression::DATA_TYPE::DT_DAY_OF_MONTH].max_value == -1)
		next_tm.tm_mday = GetLatestMDayWithYearMonthWeek(wheels[CronExpression::DATA_TYPE::DT_YEAR].max_value
                                                        ,wheels[CronExpression::DATA_TYPE::DT_MONTH].max_value
                                                        ,wheels[CronExpression::DATA_TYPE::DT_WEEK].max_value);
    else next_tm.tm_mday = wheels[CronExpression::DATA_TYPE::DT_DAY_OF_MONTH].max_value;
    if(wheels[CronExpression::DATA_TYPE::DT_WEEK].max_value != -1)
		next_tm.tm_wday = wheels[CronExpression::DATA_TYPE::DT_WEEK].max_value;
    next_tm.tm_mon = wheels[CronExpression::DATA_TYPE::DT_MONTH].max_value - 1;
    next_tm.tm_year = wheels[CronExpression::DATA_TYPE::DT_YEAR].max_value - 1900;
    #if USE_UTC == 1
        time_t wheel_time_t = timegm(&next_tm);
    #else
        time_t wheel_time_t = mktime(&next_tm);
    #endif
        // std::cout << "wheel最大时间为：  " << next_tm.tm_year + 1900 << std::endl;
		// std::cout << "wheel最大时间为：  " << next_tm.tm_wday << std::endl;
		// std::cout << "wheel最大时间为：  " << next_tm.tm_mon + 1 << std::endl;
		// std::cout << "wheel最大时间为：  " << next_tm.tm_mday << std::endl;
		// std::cout << "wheel最大时间为：  " << next_tm.tm_hour << std::endl;
		// std::cout << "wheel最大时间为：  " << next_tm.tm_min << std::endl;
        // std::cout<<"max wheel_time_t: " << wheel_time_t << std::endl;
    return std::chrono::system_clock::from_time_t(wheel_time_t);
}
/**
 * @brief 判定即将要插入的时间，列表中是否存在，存在返回true
 * 
 * @param wheels_gather_ 
 * @param wheels 
 * @return true 
 * @return false 
 */
bool TimerMgr::judgeDuplicateWheelsInWheelsGather(
    const std::vector<std::vector<CronWheel>>& wheels_gather_, 
    const std::vector<CronWheel>& wheels)
{
    if(wheels_gather_.size() == 0 || wheels.size() == 0) return false;
    for (const auto& wheels_ : wheels_gather_) {
        bool isMatch = true;
        for (size_t i = 0; i < wheels.size(); i++) {
            if(wheels_[i].values.size() != wheels[i].values.size()) return false;
            if(wheels_[i].values != wheels[i].values){
                isMatch = false;
                break;
            }
        }
        if (isMatch) return true;
    }
    return false; // 未找到相同的CronWheel组
}
/**
 * @brief 获取当前时间秒数
 * 
 * @return int64_t 
 */
int64_t TimerMgr::GetNowTimeSecond(){
    auto currentTime = std::chrono::system_clock::now();
    currentTime.time_since_epoch().count();
    auto epochTime = currentTime.time_since_epoch(); // 当前时间点与纪元时间点之间的持续时间
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epochTime).count();
    return seconds;
}
/**
 * @brief 获取当前时间，根据设定的优先级，存在列表中,优先级在Text::SortWithSelfPrioritySplitStr
 * 中可以看到，cron表达式从左到右是 秒 分 时 日 月 周 年 ，由于从年开始对比当前时间，周比月份优先判断，
 * 会导致初始化时时间轮索引的指向出问题
 * 当前优先级: 年>月>周>日>时>分>秒
 * 
 * @return std::vector<int> 
 */
std::vector<int> TimerMgr::GetNowTimeConvertVetcor(){
    tm local_tm;
#if USE_UTC==0
        time_t time_now = time(nullptr);
    #ifdef _WIN32
        localtime_s(&local_tm, &time_now); // 获取当前本地时间，存在time_now中
    #else
        localtime_r(&time_now, &local_tm); // 获取当前本地时间，存在local_tm中
    #endif // _WIN32
#endif
#if USE_UTC==1
    // 获取当前时间点
    auto currentTimePoint = std::chrono::system_clock::now();
    // 将时间点转换为 time_t
    std::time_t time_ = std::chrono::system_clock::to_time_t(currentTimePoint);
    auto cur_time_ = std::chrono::system_clock::now();
	time_t time_now = std::chrono::system_clock::to_time_t(cur_time_);
	gmtime_r(&time_now, &local_tm);
#endif
    std::vector<int> cur_time;
    // std::cout << "当前年为：  " << local_tm.tm_year + 1900 << std::endl;
	// std::cout << "当前周为：  " << local_tm.tm_wday << std::endl;
	// std::cout << "当前月为：  " << local_tm.tm_mon + 1 << std::endl;
	// std::cout << "当前日为：  " << local_tm.tm_mday << std::endl;
	// std::cout << "当前时为：  " << local_tm.tm_hour << std::endl;
	// std::cout << "当前分为：  " << local_tm.tm_min << std::endl;
	// std::cout << "当前秒为：  " << local_tm.tm_sec << std::endl;
    cur_time.emplace_back(local_tm.tm_sec);
    cur_time.emplace_back(local_tm.tm_min);
    cur_time.emplace_back(local_tm.tm_hour);
    cur_time.emplace_back(local_tm.tm_mday);
    cur_time.emplace_back(local_tm.tm_wday);
    cur_time.emplace_back(local_tm.tm_mon + 1);
    cur_time.emplace_back(local_tm.tm_year + 1900);
    return cur_time;
}

} // namespace cron_timer
