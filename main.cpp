#include <stdio.h>
#include <memory>
#include <atomic>
#include <thread>
#include <cstdarg>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#include <unistd.h>
#endif

#include "cron_timer.h"
#include "cron_log.h"
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
	// cron_timer::TimerMgr mgr;
	std::shared_ptr<cron_timer::TimerMgr> mgr = (std::shared_ptr<cron_timer::TimerMgr>)cron_timer::TimerMgr::GetInstance();
	// 2023年11月的每秒都会触发
	mgr->AddTimer("* * * ? * 1 2023", [](void) {Log("--------1 second cron timer hit");}, 22);
	// 周一到周日每秒都触发
	
	// // 从0秒开始，每3秒钟执行一次
	// mgr.AddTimer("0/3 * * * * ?", [](void) {Log("3 second cron timer hit");});
	// // 1分钟执行一次（每次都在0秒的时候执行）的定时器
	// mgr.AddTimer("0 * * * * ?", [](void) {Log("1 minute cron timer hit");});
	// // 指定时间（15秒、30秒和33秒）点都会执行一次
	// mgr.AddTimer("15,30,33 * * * * ?", [](void) {Log("cron timer hit at 15s or 30s or 33s");});
	// // 指定时间段（40到50内的每一秒）执行的定时器
	// mgr.AddTimer("40-50 * * * * ?", [](void) {Log("cron timer hit between 40s to 50s");});

	// // 每10秒钟执行一次，总共执行3次
	mgr->AddDelayTimer(10000,[](void) {Log("10 second delay timer hit");}, 3, -1);
	// Log("10 second delay timer added");
	// // 3秒钟之后执行
	// std::weak_ptr<cron_timer::BaseTimer> timer = mgr->AddDelayTimer(3000, [](void) {Log("3 second delay timer hit");}, 3);
	// // 可以在执行之前取消
	// auto ptr = timer.lock() ;
	// if (ptr != nullptr) {
	// 	ptr->Cancel();
	// }
	

	while (!_shutDown) {
		// mgr->AddTimer("* * 15 ? * *", [](void) {Log(">>>>>>>1 second cron timer hit");}, 11);
		// auto nearest_timer =
		// (std::min)(std::chrono::system_clock::now() + std::chrono::milliseconds(500), mgr.GetNearestTime());
		// std::this_thread::sleep_until(nearest_timer);
		mgr->Update();
		// std::cout << "call" << std::endl;
		// usleep(500000);
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
