// #include <Python.h>
#include <stdio.h>
#include "cron_timer.h"

using namespace cron_timer;

typedef void (*func_t) (void);
typedef char* string_;

cron_timer::TimerMgr mgr;

extern "C" {
/**
 * @brief 调用TimerMgr类的增加定时任务接口
 * 
 * @param timer_string 
 * @param func 
 * @param count 
 */
void AddTimerTask(const char* timer_string, func_t func, int count){
    std::string timerStr(timer_string);
    mgr.AddTimer(timerStr, std::move(func), count);
}
/**
 * @brief 时间轮索引更新
 * 
 */
void Update(){
    mgr.Update();
}
/**
 * @brief 停止所有任务
 * 
 */
void StopAll(){
    mgr.Stop();
}

} // extern "C"