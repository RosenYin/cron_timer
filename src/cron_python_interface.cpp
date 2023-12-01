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
void AddTimerTask(const char* timer_string, func_t func, const char* id, int count){
    std::string timerStr(timer_string);
    std::string idStr(id);
    mgr->AddTimer(timerStr, std::move(func), idStr, count);
}
/**
 * @brief 调用TimerMgr类的增加间隔时间执行的任务接口
 * 
 * @param timer_string 
 * @param func 
 * @param count 
 */
void AddDElayTimerTask(const int milliseconds, func_t func, const char* id, int count){
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
 * @brief 停止所有任务
 * 
 */
void StopAppointed(const char* id){
    std::string idStr(id);
    mgr->RemoveAppointedTimer(idStr);
}
} // extern "C"