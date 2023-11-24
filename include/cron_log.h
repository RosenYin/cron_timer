#ifndef __CRON_LOG_H
#define __CRON_LOG_H
#include <stdio.h>

std::string FormatDateTime(const std::chrono::system_clock::time_point& time);
void Log(const char* fmt, ...);
#endif // __CRON_LOG_H