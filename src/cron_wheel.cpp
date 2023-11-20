#include"cron_wheel.h"
#include <list>
#include <map>
#include <memory>
#include <functional>
#include <assert.h>
#include <time.h>
#include <cstring>
#include <algorithm>

namespace cron_timer
{
/**
 * @brief Construct a new Cron Wheel:: Cron Wheel object
 * 
 * @param current_type 
 */
CronWheel::CronWheel(CronExpression::DATA_TYPE current_type)
    : cur_index(0)
    , max_value(-1)
    , cur_type(current_type){}
/**
 * @brief 获取当前时间轮类型
 * 
 * @return CronExpression::DATA_TYPE 
 */
CronExpression::DATA_TYPE CronWheel::GetWheelType()const{
    return cur_type;
}
/**
 * @brief 设定最大值
 * 
 * @return int 
 */
int CronWheel::SetMaxVal(){
    if(!values.empty())
    {
        auto maxPosition = max_element(values.begin(), values.end());
        max_value = *maxPosition;
    }
    return max_value;
}
/**
 * @brief 设定最小值
 * 
 * @return int 
 */
int CronWheel::SetMinVal(){
    if(!values.empty())
    {
        auto minPosition = min_element(values.begin(), values.end());
        min_value = *minPosition;
    }
    return min_value;
}

}