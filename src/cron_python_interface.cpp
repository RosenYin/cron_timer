// #include <Python.h>
#include <stdio.h>
#include "cron_timer.h"
#include "cron_log.h"
using namespace cron_timer;

typedef void (*func_t) (void);
typedef char* string_;

std::shared_ptr<cron_timer::TimerMgr> mgr = (std::shared_ptr<cron_timer::TimerMgr>)cron_timer::TimerMgr::GetInstance();

extern "C" {
/**
 * @brief 调用TimerMgr类的增加定时任务接口
 * 
 * @param timer_string 
 * @param func 
 * @param count 
 */
bool AddTimerTask(const char* timer_string, func_t func, const char* id, int count){
    std::string timerStr(timer_string);
    std::string idStr(id);
    auto p = mgr->AddTimer(timerStr, std::move(func), idStr, count);
    if(p != nullptr)
        return true;
    return false;
}
/**
 * @brief 调用TimerMgr类的增加间隔时间执行的任务接口
 * 
 * @param timer_string 
 * @param func 
 * @param count 
 */
void AddDelayTimerTask(const int milliseconds, func_t func, const char* id, int count){
    std::string idStr(id);
    mgr->AddDelayTimer(milliseconds, std::move(func), idStr, count);
}
/**
 * @brief 时间轮索引更新
 * 
 */
void Update(){
    mgr->Update();
}
/**
 * @brief 停止所有任务
 * 
 */
void StopAll(){
    mgr->Stop();
}

void Log_(const char* timer_string){
    std::string timerStr(timer_string);
    Log(timer_string);
}
/**
 * @brief 停止指定任务
 * 
 */
void StopAppointedTask(const char* id){
    std::string idStr(id);
    mgr->RemoveAppointedTimer(idStr);
}

/**
 * @brief 获取指定任务ID的最近触发时间
 * 
 * @param id 
 * @return char* 
 */
char* GetAppointedIDLatestTimeStr(const char* id){
    std::string idStr(id);
    auto latest_time = mgr->GetAppointedIDLatestTimeStr(idStr);
    static char latest_time_charp[31] = {};
    strcpy(latest_time_charp, latest_time.data());
    return latest_time_charp;
}
/**
 * @brief 获取当前系统时间
 * 
 * @param id 
 * @return char* 
 */
char* GetCurrentTimeStr(){
    static char now_time_charp[31] = {};
    auto now_time = std::chrono::system_clock::now();
    strcpy(now_time_charp, GetTimeStr(now_time).data());
    return now_time_charp;
}
/**
 * @brief 获取指定任务ID的剩余时间
 * 
 * @param id 
 * @return const int 
 */
const int GetAppointedIDRemainingTime(const char* id){
    std::string idStr(id);
    const int remaining_time = mgr->GetAppointedIDRemainingTime(idStr);
    return remaining_time;
}

/**
 * @brief 判断任务是否存在列表中
 * 
 * @param id 
 * @return true 
 * @return false 
 */
bool JudgeIDIsExist(const char* id){
    std::string idStr(id);
    return mgr->JudgeIDIsExist(idStr);
}



} // extern "C"