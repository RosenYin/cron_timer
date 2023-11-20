#!/usr/bin/env python3
import ctypes
import os
from ctypes import *
so = ctypes.cdll.LoadLibrary
lib = so('./libcron_cpp.so') #刚刚生成的库文件的路径
# 使用 ctypes 库时定义 C 函数指针类型的一种方式。这用于告诉 ctypes 我们期望 C 函数指针指向的函数返回类型是 None，即没有返回值。
# 如果你的 C++ 函数有其他的返回类型，你需要相应地调整 ctypes.CFUNCTYPE 中的参数，以确保类型匹配。
# 例如，如果 C++ 函数返回 int，则可以使用 ctypes.CFUNCTYPE(ctypes.c_int)。
func_t = ctypes.CFUNCTYPE(None)

# Define a simple Python function to be called from C++
def python_callback():
    print("Python callback function called")

# 将Python中的函数 python_callback 转换为C函数指针，以便它可以被传递给C++函数。
# func_t 是 ctypes.CFUNCTYPE(None) 类型的实例，所以它可以接受没有返回值的函数。
c_callback = func_t(python_callback)

# 设置 AddTimerTask 函数的参数类型和返回类型，以便 ctypes 在调用时能够正确地处理参数和返回值。
# 这是确保 Python 与 C++ 之间的接口正确匹配的关键步骤。
lib.AddTimerTask.argtypes = [ctypes.c_char_p, func_t, ctypes.c_int]#指定 AddTimerTask 函数的参数类型。
lib.AddTimerTask.restype = None#指定 AddTimerTask 函数的返回类型。
# 定义cron表达式
cron_expression = b"* * * * * ? 2023"
# 增加定时任务
lib.AddTimerTask(ctypes.c_char_p(cron_expression), c_callback, ctypes.c_int(10))
# 在循环中不断去更新时间轮索引
while True:
    # 这里写了增加任务函数，是证明这个函数可以放在循环中，但是注意，如果任务数量过多，有可能影响性能，建议不超过十个任务
    lib.AddTimerTask(ctypes.c_char_p(cron_expression), c_callback, ctypes.c_int(10))
    # 更新
    lib.Update()