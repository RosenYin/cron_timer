﻿#pragma once
#include "cron_text.h"
#include "cron_wheel.h"
#include <thread>
#include <list>
#include <map>
#include <memory>
#include <functional>
#include <assert.h>
#include <time.h>
#include <cstring>
#include <algorithm>

namespace cron_timer {

class BaseTimer;
class TimerMgr;

using FUNC_CALLBACK = std::function<void()>;
using TimerPtr = std::shared_ptr<BaseTimer>;


class BaseTimer : public std::enable_shared_from_this<BaseTimer> {
	// 声明 TimerMgr 类为友元类，允许 TimerMgr 类访问 BaseTimer 类的私有成员。
	friend class TimerMgr;

public:
	explicit BaseTimer(TimerMgr& owner, FUNC_CALLBACK&& func);
	virtual ~BaseTimer();
	void Cancel();
	bool compareCurWheelIndexTime();
protected:
	std::list<TimerPtr>::iterator& GetIt();
	void SetIt(const std::list<TimerPtr>::iterator& it);
	bool GetIsInList() const;
	void SetIsInList(bool b);

	virtual void DoFunc();
	virtual std::chrono::system_clock::time_point GetWheelCurIndexTime() const = 0; //纯虚函数，用于获取当前时间点。

protected:
	TimerMgr& owner_; //计时器所属的 TimerMgr 实例
	FUNC_CALLBACK func_; //回调函数
	std::list<TimerPtr>::iterator it_; //计时器在列表中的迭代器
	bool is_in_list_; //表示计时器是否在列表中的标志位
};

/**
 * @brief 继承自 BaseTimer 类，并用于在特定的 Cron 时间点执行回调函数
 * 
 */
class CronTimer : public BaseTimer {
	friend class TimerMgr;
public:
	explicit CronTimer(TimerMgr& owner, std::vector<CronWheel>&& wheels, FUNC_CALLBACK&& func, int count);
	void InitWheelIndex();
	inline void DoFunc() override;
	std::chrono::system_clock::time_point GetWheelCurIndexTime() const override;
private:

	void Next(int data_type);
	int GetCurValue(int data_type) const;

private:
	std::vector<CronWheel> wheels_; // 用于存储不同时间字段的 CronWheel 对象
	bool over_flowed_; // 用于表示是否发生溢出
	int count_left_; // 表示定时器剩余的执行次数
};

class LaterTimer : public BaseTimer {
	friend class TimerMgr;
public:
	explicit LaterTimer(TimerMgr& owner, int milliseconds, FUNC_CALLBACK&& func, int count);
	inline void DoFunc() override;
	std::chrono::system_clock::time_point GetWheelCurIndexTime() const override;

private:
	void Next();

private:
	const int mill_seconds_; // 表示延迟执行的毫秒数。
	int count_left_; // 定时器剩余的执行次数
	std::chrono::system_clock::time_point cur_time_; // 下一个触发时间点
};


class TimerMgr {
	// 声明 BaseTimer、CronTimer 和 LaterTimer 类为友元类，以允许它们访问 TimerMgr 类的私有成员。
	friend class BaseTimer;
	friend class CronTimer;
	friend class LaterTimer;

public:
	enum {
		RUN_NERVER = 0,
		RUN_FOREVER = -1
	};

	TimerMgr();
	TimerMgr(const TimerMgr&) = delete;
	const TimerMgr& operator=(const TimerMgr&) = delete;

	TimerPtr AddTimer(const std::string& timer_string, FUNC_CALLBACK&& func, int count = RUN_FOREVER);
	TimerPtr AddDelayTimer(int milliseconds, FUNC_CALLBACK&& func, int count = 1);
	// 获取最接近的触发时间点
	std::chrono::system_clock::time_point GetNearestTime();
	size_t Update();
	void Stop();
	
private:
	void insert(const TimerPtr& p);
	bool remove(const TimerPtr& p);
public:
	static int64_t TimePointConvertInteger(std::chrono::system_clock::time_point time_point);
	static bool compareMaxSetTime(const TimerPtr& p);
	static bool compareMaxSetTime(const std::vector<CronWheel>& wheels);
	static std::chrono::system_clock::time_point GetWheelsMaxTimePoint(std::vector<CronWheel> wheels);
	static bool judgeDuplicateWheelsInWheelsGather(const std::vector<std::vector<CronWheel>>& wheels_gather_, const std::vector<CronWheel>& wheels);
	static int64_t GetNowTimeSecond();
	static std::vector<int> GetNowTimeConvertVetcor();
private:
	std::map<std::chrono::system_clock::time_point, std::list<TimerPtr>> timers_;
	bool stopped_ = false;
	std::vector<std::vector<CronWheel>> wheels_gather_;
};

}
