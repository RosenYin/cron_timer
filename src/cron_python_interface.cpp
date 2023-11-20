// #include <Python.h>
#include <stdio.h>
#include "cron_timer.h"

using namespace cron_timer;

typedef void (*func_t) (void);
typedef char* string_;

cron_timer::TimerMgr mgr;

extern "C" {

void AddTimerTask(const char* timer_string, func_t func, int count){
    std::string timerStr(timer_string);
    mgr.AddTimer(timerStr, std::move(func), count);
}

void Update(){
    mgr.Update();
}


} // extern "C"