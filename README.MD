# 定时任务

参照代码库：<https://github.com/yongxin-ms/cron_timer.git>

本人只是在其上修补了一些bug以及重构一部分代码

## 修订内容

| 编号  | 内容                                                         | 状态                        |日期       |
| ----- | ------------------------------------------------------------ | --------------------------- |--------|
| 8     | 将头文件拆分，将实现和声明拆开 | RosenYin已完成     |2023.11.9 |
| 9     | 修补创建CronTimer类初始化wheel索引的问题(未来时间) | RosenYin已完成     |2023.11.12 |
| 10    | 增加创建时间的判断，令其能够在循环中AddTimer而不影响任务准时触发，通过判断当前要插入的时间轮与之前插入的时间轮是否每个数值都相等，如果相等则不插入，未考虑要插入同样时间轮但是执行的任务不同的情况，这个情况下只有先插入到列表中的任务能执行，后面的插入不到触发列表中(for循环遍历判断是否有相同时间比较蠢，如果任务数目很多，每次都有很大开销) | RosenYin已完成     |2023.11.17 |
| 11    | 增加根据表达式中的年月周，自动判断距离最近的日期(当前或将来)，防止索引指向过去的bug | RosenYin已完成 |2023.11.17 |
| 12     | 增加cron表达式的阿里云格式的兼容，可以支持6字段和7字段的cron表达式 | RosenYin已完成     |2023.11.18 |
| 13     | 修补了当字段是时间段时，如果时间段第一个数小于第二个数，导致范围出错的问题，增加了排序 | RosenYin已完成 |2023.11.18 |
| 14     | 增加python接口的调用，使用ctypes，在cmake中可选编译成so或者可执行文件，具体使用可参照test.py | RosenYin已完成   |2023.11.20 |
| 15     | 增加UTC时间的转换，通过宏定义切换，输入时间和触发时间可以为UTC时间或者当前本地时间 | RosenYin已完成 |2023.11.21 |
| 16     | 修补当前周横跨两个月份，设定的周的范围小于或大于当前周数值时，月份初始化错误的问题 | RosenYin已完成 |2023.11.21 |
| 17     | 修改TimerMgr类为单例模式，使用类内的接口创建该类的唯一指针 |RosenYin已完成 |2023.11.27|

## 使用

- 调用`TimerMgr`对象内GetInstance方法创建唯一指针
- 调用`TimerMgr`对象中的`AddTimer`方法去处理cron表达式时间并传入回调函数
  - `AddTimer`方法参数：
    - cron表达式(字符串)：
      - 参考阿里云的cron规则：<https://developer.aliyun.com/article/1349827>
      - 字符串表示，分为6个字段，每个字段空格隔开，每个字段表示顺序如下：秒 时 分 日 月 周 (年)，每个字段支持的符号有'`,`' 、 '`/`' 、 '`-`'
      - 注意：字符串字段数量可以是6或7，当长度为6时，顺序为：秒 时 分 日 月 周;长度为7时，顺序为：秒 时 分 日 月 周 年
      - 每个字段的范围：
        - 年：2023-2038
        - 周：0-6
        - 月：1-12
        - 日：1-31
        - 时：0-23
        - 分：0-59
        - 秒：0-59
      - 符号：
        - `，`表示枚举，将要触发的时间列出来，最好按顺序排列;
        - `/`表示时间段，`/`前是起始时间，后是距离起始时间触发的时间段，例如在Minutes域使用5/20,则意味着第5分钟触发一次，然后20分钟后的25，45等分别触发一次;
        - `-`表示范围，例如在Minutes域使用5-20，表示从5分到20分钟每分钟触发一次。
        - 支持"SUN","MON","TUE","WED","THU","FRI","SAT"的周字段，但是注意，如果要让一个任务从周一执行到周日是不可行的，因为周日是一周开始，需要设定为周日到周六才可以是整个一周。
        - `？`字符仅被用于天（月）和天（星期）两个子表达式，表示不指定值，当2个子表达式其中之一被指定了值以后，为了避免冲突，需要将另一个子表达式的值设为`？`
      - 注意：`#`、`L`、`W`暂不支持。这里简述一下这几个字的功能：
        - `L` 表示最后，只能出现在DayofWeek和DayofMonth域。如果在DayofWeek域使用 `4L`,意味着在最后的一个星期四触发。
        - `W` 表示有效工作日(周一到周五),只能出现在DayofMonth域，系统将在离指定日期的最近的有效工作日触发事件。例如：在 DayofMonth使用 `5W`，如果5日是星期六，则将在最近的工作日：星期五，即4日触发。如果5日是星期天，则在6日(周一)触发；如果5日在星期一到星期五中的一天，则就在5日触发。另外一点，W的最近寻找不会跨过月份 。
        - `LW` 这两个字符可以连用，表示在某个月最后一个工作日，即最后一个星期五。
        - `#` 用于确定每个月第几个星期几，只能出现在DayofWeek域。例如在`4#2`，表示某月的第二个星期四。
    - 回调函数
- 在循环中调用`TimerMgr`对象中的`Update`方法来不断更新任务时间

## cron表达式示例

可以使用这个网站来生成表达式：<https://www.bejson.com/othertools/cron/?ivk_sa=1024320u>

1. `0/20 * * * * ?`       表示每20秒 调整任务
2. `0 0 2 1 * ?`          表示在每月的1日的凌晨2点调整任务
3. `0 15 10 ? * MON-FRI`  表示周一到周五每天上午10:15执行作业
4. `0 0 10,14,16 * * ?`   每天上午10点，下午2点，4点
5. `0 0/30 9-17 * * ?`    朝九晚五工作时间内每半小时
6. `0 0 12 ? * WED`       表示每个星期三中午12点
7. `0 0 12 * * ?`         每天中午12点触发
8. `0 15 10 ? * *`        每天上午10:15触发
9. `0 15 10 * * ?`        每天上午10:15触发
10. `0 15 10 * * ? 2005`  2005年的每天上午10:15触发
11. `0 * 14 * * ?`        在每天下午2点到下午2:59期间的每1分钟触发
12. `0 0/5 14 * * ?`      在每天下午2点到下午2:55期间的每5分钟触发
13. `0 0/5 14,18 * * ?`   在每天下午2点到2:55期间和下午6点到6:55期间的每5分钟触发
14. `0 0-5 14 * * ?`      在每天下午2点到下午2:05期间的每1分钟触发
15. `0 10,44 14 ? 3 WED`  每年三月的星期三的下午2:10和2:44触发
16. `0 15 10 ? * MON-FRI` 周一至周五的上午10:15触发
17. `0 15 10 15 * ?`      每月15日上午10:15触发

## UTC时间与本地时间切换

在`cron_timer.cpp`中，宏定义`USE_UTC`为`0`则为本地时间触发，cron表达式以本地时间为基准，`1`则都切换为UTC时间。

注：UTC时间比北京时间晚8小时(东8区)

## 编译选项

在`CMakeLists.txt`文件中可以修改`CUSTOM_COMPILE_OPTIONS`的值，如果是"0"则为编译可执行文件，"1"为编译为so动态库文件

例：

```bash
mkdir build && cd build
cmake .. -DCUSTOM_COMPILE_OPTIONS="1"
```

## Python调用

先编译文件为so，再在python中调用

注意，要运行test.py的话要令so文件与python文件在同一个路径下

需要先使用ctypes导入so文件，然后进行函数所需要参数的类型强制转换，再去调用函数

test.py提供了例子，在其中有两个cron表达式，用来触发定时任务，其中一个任务设定为永久触发，另一个为执行10次，在主线程中循环5s后取消设定的第二个任务

程序存在执行后缀参数，例：

```shell
python3 ./test.py --cron1 "0 * * ? * * 2023" --cron2 "* * * ? * * 2023"
python3 ./test.py --cron1 "0 * * ? * * 2023" # 也可以只输入一个参数，第二个参数自动为默认，执行一直会执行两个任务
```

也可以不使用后缀参数直接执行,那么就会使用程序内的默认cron表达式参数:

```shell
python3 ./test.py
```

```python
#!/usr/bin/env python3
import ctypes
import os
import _thread
import time
from ctypes import *
import argparse
so = ctypes.cdll.LoadLibrary
lib = so('./libcron_timer.so') #刚刚生成的库文件的路径
# 使用 ctypes 库时定义 C 函数指针类型的一种方式。这用于告诉 ctypes 我们期望 C 函数指针指向的函数返回类型是 None，即没有返回值。
# 如果你的 C++ 函数有其他的返回类型，你需要相应地调整 ctypes.CFUNCTYPE 中的参数，以确保类型匹配。
# 例如，如果 C++ 函数返回 int，则可以使用 ctypes.CFUNCTYPE(ctypes.c_int)。
func_t = ctypes.CFUNCTYPE(None)

# Define a simple Python function to be called from C++
def python_callback1():
    print("-------------------------Python callback function called1---------------------")
def python_callback2():
    print("=========================Python callback function called2=====================")
# 将Python中的函数 python_callback 转换为C函数指针，以便它可以被传递给C++函数。
# func_t 是 ctypes.CFUNCTYPE(None) 类型的实例，所以它可以接受没有返回值的函数。
c_callback1 = func_t(python_callback1)
c_callback2 = func_t(python_callback2)
# 设置 AddTimerTask 函数的参数类型和返回类型，以便 ctypes 在调用时能够正确地处理参数和返回值。
# 这是确保 Python 与 C++ 之间的接口正确匹配的关键步骤。
lib.AddTimerTask.argtypes = [ctypes.c_char_p, func_t, ctypes.c_int, ctypes.c_int]#指定 AddTimerTask 函数的参数类型。
lib.AddTimerTask.restype = None#指定 AddTimerTask 函数的返回类型。
# 设定ID
id1 = '33'
id2 = '22'
# 增加定时任务
parser = argparse.ArgumentParser()
parser.add_argument('--cron1', default= '0 * * ? * * 2023')     # add_argument()指定程序可以接受的命令行选项
parser.add_argument('--cron2', default= '* * * ? * * 2023')     # add_argument()指定程序可以接受的命令行选项
args = parser.parse_args()      # parse_args()从指定的选项中返回一些数据
print(args)
# 定义cron表达式
print(args.cron1)
print(args.cron2)
cron_expression =  ((str)(args.cron1)).encode()
cron_expression1 = ((str)(args.cron2)).encode()
# 增加定时任务
lib.AddTimerTask(ctypes.c_char_p(cron_expression), c_callback1, ctypes.c_char_p(id1.encode()), ctypes.c_int(-1))
lib.AddTimerTask(ctypes.c_char_p(cron_expression1), c_callback2, ctypes.c_char_p(id2.encode()), ctypes.c_int(-1))

def Thread():
    print("更新线程")
    while True:
        lib.Update()
        time.sleep(0.1)
# 创建线程
_thread.start_new_thread(Thread,())
# 在循环中不断去更新时间轮索引
count = 0
while True:
    count = count +1
    # 5s后取消指定id的任务
    if(count > 5):
        lib.StopAppointed(id2)
    time.sleep(1)
```

## 示例

```CPP
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

void TestCronTimerInMainThread() {
    std::shared_ptr<cron_timer::TimerMgr> mgr = (std::shared_ptr<cron_timer::TimerMgr>)cron_timer::TimerMgr::GetInstance();
    std::string id = "0";
    // 2023年11月的每秒都会触发
    mgr->AddTimer("* * * * 11 ? 2023", [](void) {Log("--------1 second cron timer hit");}, id);//最后一个参数是执行次数，默认为-1,即永远执行,0为永远不执行
    // 周一到周日每秒都触发
    mgr->AddTimer("* * * ? * MON-SAT", [](void) {Log(">>>>>>>1 second cron timer hit");}, id+'1');
    // 从0秒开始，每3秒钟执行一次
    mgr->AddTimer("0/3 * * * * ?", [](void) {Log("3 second cron timer hit");}, id+'2');
    // 1分钟执行一次（每次都在0秒的时候执行）的定时器
    mgr->AddTimer("0 * * * * ?", [](void) {Log("1 minute cron timer hit");}, id+'3');
    // 指定时间（15秒、30秒和33秒）点都会执行一次
    mgr->AddTimer("15,30,33 * * * * ?", [](void) {Log("cron timer hit at 15s or 30s or 33s");}, id+'4');
    // 指定时间段（40到50内的每一秒）执行的定时器
    mgr->AddTimer("40-50 * * * * ?", [](void) {Log("cron timer hit between 40s to 50s");}, id+'5');
    // 每10秒钟执行一次，总共执行3次
    mgr->AddDelayTimer(10000,[](void) {Log("10 second delay timer hit");}, id+'6', -1);//-1是永远执行
    Log("10 second delay timer added");
    // 3秒钟之后执行
    std::weak_ptr<cron_timer::BaseTimer> timer = mgr->AddDelayTimer(3000, [](void) {Log("3 second delay timer hit");}, id+'7', -1);
    // 可以在执行之前取消
    auto ptr = timer.lock() ;
    if (ptr != nullptr) {
        ptr->Cancel();
    }
    while (!_shutDown) {
        // auto nearest_timer =
        // (std::min)(std::chrono::system_clock::now() + std::chrono::milliseconds(500), mgr.GetNearestTime());
        // std::this_thread::sleep_until(nearest_timer);
        mgr->Update();
        usleep(500000);//延迟500ms,500/ms*1000/us = 500000us
    }
}

int main() {
#ifdef _WIN32
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    EnableMenuItem(GetSystemMenu(GetConsoleWindow(), false), SC_CLOSE, MF_GRAYED | MF_BYCOMMAND);
#else
    signal(SIGINT, signal_hander);
#endif
    TestCronTimerInMainThread();
return 0;
}

```
