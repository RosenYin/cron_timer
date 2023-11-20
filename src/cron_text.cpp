#include"cron_text.h"
#include <assert.h>
#include <algorithm>
#include <cmath>
#include <assert.h>
#include <stdio.h>
#include <ctype.h>
#include <sstream>  

namespace cron_timer
{

/**
 * @brief 主要用来切分使用空格分隔的字符串，连续的空格算作一个分隔符
 * 
 * @param os 
 * @param is 
 * @param c 
 * @return size_t 
 */
size_t Text::SplitStr(std::vector<std::string>& os, const std::string& is, char c) {
    os.clear();
    auto start = is.find_first_not_of(c, 0);
    while (start != std::string::npos) {
        auto end = is.find_first_of(c, start);
        if (end == std::string::npos) {
            os.emplace_back(is.substr(start));
            break;
        } else {
            os.emplace_back(is.substr(start, end - start));
            start = is.find_first_not_of(c, end + 1);
        }
    }
    return os.size();
}
/**
 * @brief 判断字符串是否为数字字符串，是则返回true
 * 
 * @param str 
 * @return true 
 * @return false 
 */
bool Text::isNum(const std::string str)
{
	using namespace std;
	std::stringstream sin(str);
	double d;
	char c;
	if(!(sin >> d))
	{
		return false;
	}
	if(sin >> c)
	{
		return false;
	}
	return true;
}
/**
 * @brief 处理周字符串，返回0-6
 * 
 * @param input 
 * @return uint16_t 
 */
uint16_t Text::DealWeekStr(std::string input){
	if(input.length()!=3)
		assert(("周的字符长度应该为三个，取其英文前三个字母",false));
	std::vector<std::string> weekend = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
	std::string w_day = input;
	transform(w_day.begin(),w_day.end(),w_day.begin(),::toupper);
	std::vector<std::string>::iterator result = std::find(weekend.begin(), weekend.end(), w_day); //查找target
	int index = distance(weekend.begin(), result);
	return index;
}
/**
 * @brief 将字段按照指定的字符分割成int类型数
 * 
 * @param number_result 
 * @param is 
 * @param c 
 * @return size_t 
 */
size_t Text::SplitInt(std::vector<int>& number_result, const std::string& is, char c) {
    std::vector<std::string> string_result;
    SplitStr(string_result, is, c);
	
    number_result.clear();
    for (size_t i = 0; i < string_result.size(); i++) {
        const std::string& value = string_result[i];
		if(isNum(value)){
        	number_result.emplace_back(atoi(value.data()));
		}
		else{
			number_result.emplace_back(DealWeekStr(value));
		}
    }

    return number_result.size();
}
/**
 * @brief 将获取的分割后的字符串，按照自定义优先级重新存入列表中
 * 
 * @param os 
 * @param is 
 * @param c 
 * @return size_t 
 */
size_t Text::SortWithSelfPrioritySplitStr(std::vector<std::string>& os, const std::string& is, char c){
    int size = SplitStr(os, is, c);
    std::vector<uint16_t> date_str_priority = {0,1,2,3,5,4,6};
    if(size == 6) date_str_priority.pop_back();
    std::vector<std::string> temp;
    for(auto i : date_str_priority)
    {
        temp.emplace_back(os[i]);
    }
    os.clear();
    os = std::move(temp);
    return os.size();
}
// 此方法用于将一个字符串按照给定的字符 c 分割成子字符串，并返回一个包含这些子字符串的字符串向量。
std::vector<std::string> Text::ParseParam(const std::string& is, char c) {
    std::vector<std::string> result;
    ParseParam(result, is, c);
    return result;
}

/**
 * @brief // 将一个字符串按照给定的字符 c 切分成子字符串，但是不返回结果，而是将结果存储在传入的字符串向量 result 中。
 * 如果字符串中不存在分隔符 c，则会在字符串向量中包含一个空字符串。
 * 方法返回生成的子字符串数量。
 * 主要用来切分使用逗号分隔的字符串，连续的逗号算作多个分隔符。
 * 
 * @param result 
 * @param is 
 * @param c 
 * @return size_t 
 */
size_t Text::ParseParam(std::vector<std::string>& result, const std::string& is, char c) {
	result.clear();
	size_t start = 0;
	while (start < is.size()) {
        auto end = is.find_first_of(c, start);
        if (end != std::string::npos) {
            result.emplace_back(is.substr(start, end - start));
            start = end + 1;
        } else {
            result.emplace_back(is.substr(start));
            break;
        }
	}

    if (start == is.size()) {
        result.emplace_back(std::string());
    }
    return result.size();
}

/**
 * @brief Get the Values object
 * 获得数值枚举
 * 
 * @param input 
 * @param data_type 
 * @param values 
 * @return true 
 * @return false 
 * 注意：枚举之前是','，如果要在csv中使用需要改成';'
 */
bool CronExpression::GetValues(const std::string& input, DATA_TYPE data_type, std::vector<int>& values) {
    static const char CRON_SEPERATOR_ENUM = ',';
	static const char CRON_SEPERATOR_RANGE = '-';
	static const char CRON_SEPERATOR_INTERVAL = '/';
	static const char CRON_SEPERATOR_UNSPECIFIED = '?';

	if (input == "*") {
		auto pair_range = GetRangeFromType(data_type);
		for (auto i = pair_range.first; i <= pair_range.second; ++i) {
			values.push_back(i);
		}
	} else if (input.find_first_of(CRON_SEPERATOR_ENUM) != std::string::npos) {
		//枚举
		std::vector<int> v;
		Text::SplitInt(v, input, CRON_SEPERATOR_ENUM);
		std::pair<int, int> pair_range = GetRangeFromType(data_type);
		for (auto value : v) {
			if (value < pair_range.first || value > pair_range.second) {
				return false;
			}
			values.push_back(value);
		}
	} else if (input.find_first_of(CRON_SEPERATOR_RANGE) != std::string::npos) {
		//范围
		std::vector<int> v;
		Text::SplitInt(v, input, CRON_SEPERATOR_RANGE);
		sort(v.begin(), v.end());
		if (v.size() != 2) {
			return false;
		}

		int from = v[0];
		int to = v[1];
		std::pair<int, int> pair_range = GetRangeFromType(data_type);
		if (from < pair_range.first || to > pair_range.second) {
			return false;
		}

		for (int i = from; i <= to; i++) {
			values.push_back(i);
		}
	} else if (input.find_first_of(CRON_SEPERATOR_INTERVAL) != std::string::npos) {
		//间隔
		std::vector<int> v;
		Text::SplitInt(v, input, CRON_SEPERATOR_INTERVAL);
		if (v.size() != 2) {
			return false;
		}

		int from = v[0];
		int interval = v[1];
		std::pair<int, int> pair_range = GetRangeFromType(data_type);
		if (from < pair_range.first || interval < 0) {
			return false;
		}

		for (int i = from; i <= pair_range.second; i += interval) {
			values.push_back(i);
		}
	} else if (input.find_first_of(CRON_SEPERATOR_UNSPECIFIED) != std::string::npos) {
		if(data_type == DT_DAY_OF_MONTH){
			// auto pair_range = GetRangeFromType(data_type);
			// for (auto i = pair_range.first; i <= pair_range.second; ++i) {
			// 	values.push_back(i);
			// }
			values.push_back(-1);
		}
		else if(data_type == DT_WEEK){
			// auto pair_range = GetRangeFromType(data_type);
			// for (auto i = pair_range.first; i <= pair_range.second; ++i) {
			// 	values.push_back(i);
			// }
			values.push_back(-1);
		}
		else {
			return false;
		}
	} else {
		//具体数值
		std::pair<int, int> pair_range = GetRangeFromType(data_type);
		int value = 0;
		if(Text::isNum(input)){
        	value = atoi(input.data());
		}
		else{
			value = Text::DealWeekStr(input);
		}
		// 判断数值是否在范围内
		if (value < pair_range.first || value > pair_range.second) {
			return false;
		}
		values.push_back(value);
	}
	
	assert(values.size() > 0);
	return values.size() > 0;
}
/**
 * @brief 获得字段范围
 * 
 * @param data_type 
 * @return std::pair<int, int> 
 */
std::pair<int, int> CronExpression::GetRangeFromType(DATA_TYPE data_type) {
	int from = 0;
	int to = 0;

	switch (data_type) {
	case CronExpression::DT_SECOND:
	case CronExpression::DT_MINUTE:
		from = 0;
		to = 59;
		break;
	case CronExpression::DT_HOUR:
		from = 0;
		to = 23;
		break;
	case CronExpression::DT_DAY_OF_MONTH:
		from = 1;
		to = 31;
		break;
	case CronExpression::DT_MONTH:
		from = 1;
		to = 12;
		break;
	case CronExpression::DT_WEEK:
		from = 0;
		to = 6;
		break;
	case CronExpression::DT_YEAR:
		from = 2023;
		to = 2038;
		break;
	case CronExpression::DT_MAX:
		assert(false);
		break;
	}

	return std::make_pair(from, to);
}


}//namespace cron_timer