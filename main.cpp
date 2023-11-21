﻿#include <stdio.h>
#include <memory>
#include <atomic>
#include <thread>
#include <cstdarg>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#endif

#include "cron_timer.h"

std::atomic_bool _shutDown;

#ifdef _WIN32
BOOL WINAPI ConsoleHandler(DWORD CtrlType) {
	switch (CtrlType) {
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		_shutDown = true;
		break;

	default:
		break;
	}

	return TRUE;
}
#else
void signal_hander(int signo) //自定义一个函数处理信号
{
	printf("catch a signal:%d\n:", signo);
	_shutDown = true;
}
#endif

std::string FormatDateTime(const std::chrono::system_clock::time_point& time) {
	uint64_t micro = std::chrono::duration_cast<std::chrono::microseconds>(time.time_since_epoch()).count() -
					std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count() * 1000000;
	char _time[64] = {0};
	time_t tt = std::chrono::system_clock::to_time_t(time);
	struct tm local_time;

#ifdef _WIN32
	localtime_s(&local_time, &tt);
#else
	localtime_r(&tt, &local_time);
#endif // _WIN32

	std::snprintf(_time, sizeof(_time), "%d-%02d-%02d %02d:%02d:%02d.%06ju", local_time.tm_year + 1900,
		local_time.tm_mon + 1, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, micro);
	return std::string(_time);
}

void Log(const char* fmt, ...) {
	char buf[4096];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf) - 1, fmt, args);
	va_end(args);

	std::string time_now = FormatDateTime(std::chrono::system_clock::now());
	printf("%s %s\n", time_now.c_str(), buf);
}

void TestSplitStr() {
	std::vector<std::string> v;
	assert(cron_timer::Text::SplitStr(v, "", ' ') == 0);
	assert(cron_timer::Text::SplitStr(v, " ", ' ') == 0);
	assert(cron_timer::Text::SplitStr(v, "a", ' ') == 1);
	assert(cron_timer::Text::SplitStr(v, "abcc", ' ') == 1);
	assert(cron_timer::Text::SplitStr(v, "abc def", ' ') == 2);
	assert(cron_timer::Text::SplitStr(v, " abc def", ' ') == 2);
	assert(cron_timer::Text::SplitStr(v, " abc def ", ' ') == 2);
	assert(cron_timer::Text::SplitStr(v, "  abc   def ", ' ') == 2);

	assert(cron_timer::Text::ParseParam(v, "", ',') == 1);
	assert(cron_timer::Text::ParseParam(v, " ", ',') == 1);
	assert(cron_timer::Text::ParseParam(v, "a", ',') == 1);
	assert(cron_timer::Text::ParseParam(v, "abcc", ',') == 1);
	assert(cron_timer::Text::ParseParam(v, "abc,def", ',') == 2);
	assert(cron_timer::Text::ParseParam(v, "abc,,def", ',') == 3);
	assert(cron_timer::Text::ParseParam(v, ",,abc,,,def,,", ',') == 8);
	assert(cron_timer::Text::ParseParam(v, ",,,", ',') == 4);
}

void TestCronTimerInMainThread() {
	cron_timer::TimerMgr mgr;

	// 2023年11月的每秒都会触发
	mgr.AddTimer("* * * ? * 2 2023", [](void) {Log("--------1 second cron timer hit");});
	// 周一到周日每秒都触发
	// mgr.AddTimer("* * * ? * MON-SAT", [](void) {Log(">>>>>>>1 second cron timer hit");});
	// // 从0秒开始，每3秒钟执行一次
	// mgr.AddTimer("0/3 * * * * ?", [](void) {Log("3 second cron timer hit");});
	// // 1分钟执行一次（每次都在0秒的时候执行）的定时器
	// mgr.AddTimer("0 * * * * ?", [](void) {Log("1 minute cron timer hit");});
	// // 指定时间（15秒、30秒和33秒）点都会执行一次
	// mgr.AddTimer("15,30,33 * * * * ?", [](void) {Log("cron timer hit at 15s or 30s or 33s");});
	// // 指定时间段（40到50内的每一秒）执行的定时器
	// mgr.AddTimer("40-50 * * * * ?", [](void) {Log("cron timer hit between 40s to 50s");});

	// // 每10秒钟执行一次，总共执行3次
	// mgr.AddDelayTimer(10000,[](void) {Log("10 second delay timer hit");}, 3);
	// Log("10 second delay timer added");
	// // 3秒钟之后执行
	// std::weak_ptr<cron_timer::BaseTimer> timer = mgr.AddDelayTimer(3000, [](void) {Log("3 second delay timer hit");});
	// // 可以在执行之前取消
	// auto ptr = timer.lock() ;
	// if (ptr != nullptr) {
	// 	ptr->Cancel();
	// }
	

	while (!_shutDown) {
		// auto nearest_timer =
		// (std::min)(std::chrono::system_clock::now() + std::chrono::milliseconds(500), mgr.GetNearestTime());
		// std::this_thread::sleep_until(nearest_timer);
		mgr.Update();
	}
}

int main() {
#ifdef _WIN32
	SetConsoleCtrlHandler(ConsoleHandler, TRUE);
	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false), SC_CLOSE, MF_GRAYED | MF_BYCOMMAND);
#else
	signal(SIGINT, signal_hander);
#endif

	TestSplitStr();
	TestCronTimerInMainThread();
	return 0;
}
