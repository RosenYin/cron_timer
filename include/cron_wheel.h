#pragma once
#include "cron_text.h"

namespace cron_timer {

struct CronWheel {
	friend class CronExpression;
public:
	explicit CronWheel(CronExpression::DATA_TYPE current_type);
	bool init(int cur_value, bool &last_equal);
	CronExpression::DATA_TYPE GetWheelType()const;

	int SetMaxVal();
	int SetMinVal();

public:
	size_t cur_index;
	std::vector<int> values;
	int max_value;
	int min_value = -1;
private:
	const CronExpression::DATA_TYPE cur_type;
};

}