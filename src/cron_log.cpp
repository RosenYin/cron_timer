#include <memory>
#include <atomic>
#include <thread>
#include <cstdarg>
#include "cron_log.h"

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
		local_time.tm_mon + 1, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec, (uintmax_t)micro);
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