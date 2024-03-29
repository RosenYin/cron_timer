#pragma once
#include <iostream>
#include <string>
#include <vector>

namespace cron_timer {

class Text {
public:
	static size_t SplitStr(std::vector<std::string>& os, const std::string& is, char c);
	static uint16_t DealWeekStr(std::string input);
	static bool isNum(const std::string str);
	static size_t SplitInt(std::vector<int>& number_result, const std::string& is, char c);
	static size_t SortWithSelfPrioritySplitStr(std::vector<std::string>& os, const std::string& is, char c);
	static std::vector<std::string> ParseParam(const std::string& is, char c);
	static size_t ParseParam(std::vector<std::string>& result, const std::string& is, char c);
};

class CronExpression {
public:
	enum DATA_TYPE {
		DT_SECOND = 0,
		DT_MINUTE,
		DT_HOUR,
		DT_DAY_OF_MONTH,
		DT_WEEK,
		DT_MONTH,
		DT_YEAR,
		DT_MAX,
	};
	static bool GetValues(const std::string& input, DATA_TYPE data_type, std::vector<int>& values);
private:
	static std::pair<int, int> GetRangeFromType(DATA_TYPE data_type);
};

}