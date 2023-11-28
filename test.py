#!/usr/bin/env python3
import ctypes
import os
import _thread
import time
from ctypes import *
so = ctypes.cdll.LoadLibrary
lib = so('./libcron_timer.so') #刚刚生成的库文件的路径
lib1 = so('./libcron_timer.so') #刚刚生成的库文件的路径
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
# 定义cron表达式
cron_expression =   b"* * * ? * * 2023"
cron_expression1 =  b"* * * ? * * 2023"
id1 = 33
id2 = 22
# 增加定时任务
lib.AddTimerTask(ctypes.c_char_p(cron_expression), c_callback1, id1, ctypes.c_int(-1))
lib.AddTimerTask(ctypes.c_char_p(cron_expression), c_callback2, id2, ctypes.c_int(10))

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
    lib.AddTimerTask(ctypes.c_char_p(cron_expression), c_callback1, 22, ctypes.c_int(10))
    time.sleep(1)