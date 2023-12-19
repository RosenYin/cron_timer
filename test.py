#!/usr/bin/env python3
import ctypes
import os
import _thread
import time
from datetime import datetime
from ctypes import *
import binascii

so = ctypes.cdll.LoadLibrary
lib = so('./libcron_timer.so') #刚刚生成的库文件的路径
# 使用 ctypes 库时定义 C 函数指针类型的一种方式。这用于告诉 ctypes 我们期望 C 函数指针指向的函数返回类型是 None，即没有返回值。
# 如果你的 C++ 函数有其他的返回类型，你需要相应地调整 ctypes.CFUNCTYPE 中的参数，以确保类型匹配。
# 例如，如果 C++ 函数返回 int，则可以使用 ctypes.CFUNCTYPE(ctypes.c_int)。
func_t = ctypes.CFUNCTYPE(None)

def PrintTime():
    # 获取当前时间
    current_time = datetime.now()
    # 格式化为字符串
    formatted_time = current_time.strftime("%Y-%m-%d %H:%M:%S")
    # 打印标准格式时间
    # print(formatted_time)
    return formatted_time

# 设置 AddTimerTask 函数的参数类型和返回类型，以便 ctypes 在调用时能够正确地处理参数和返回值。
# 这是确保 Python 与 C++ 之间的接口正确匹配的关键步骤。
lib.AddTimerTask.argtypes = [ctypes.c_char_p, func_t, ctypes.c_char_p, ctypes.c_int]#指定 AddTimerTask 函数的参数类型。
lib.AddTimerTask.restype = bool#指定 AddTimerTask 函数的返回类型。
lib.StopAppointedTask.restype = ctypes.c_bool
lib.AddDelayTimerTask.argtypes = [ctypes.c_int, func_t, ctypes.c_char_p, ctypes.c_int]#指定 AddTimerTask 函数的参数类型。
lib.AddDelayTimerTask.restype = ctypes.c_bool
# 设置返回类型为 c_char_p
lib.GetAppointedIDLatestTimeStr.restype = ctypes.c_char_p
lib.GetCurrentTimeStr.restype = ctypes.c_char_p
lib.JudgeIDIsExist.restype = ctypes.c_bool
id1 = b'301'
id2 = b'302'
id3 = b'333'
# Define a simple Python function to be called from C++
def python_callback1():
    print(PrintTime(),"-------", lib.GetAppointedIDLatestTimeStr(id1).decode(), "-------")
def python_callback2():
    print(PrintTime(),"=======", lib.GetAppointedIDLatestTimeStr(id2).decode(), lib.GetAppointedIDLatestTimeStr(id1).decode(), lib.GetAppointedIDRemainingTime(id1),"=======")
def python_callback3():
    print(PrintTime(),"=======", lib.GetAppointedIDLatestTimeStr(id3).decode(), "=======")
# 将Python中的函数 python_callback 转换为C函数指针，以便它可以被传递给C++函数。
# func_t 是 ctypes.CFUNCTYPE(None) 类型的实例，所以它可以接受没有返回值的函数。
c_callback1 = func_t(python_callback1)
c_callback2 = func_t(python_callback2)
c_callback3 = func_t(python_callback3)
# 增加定时任务
import argparse
parser = argparse.ArgumentParser()
parser.add_argument('--cron1', default= '0 0 0 * * ? 2023')     # add_argument()指定程序可以接受的命令行选项
parser.add_argument('--cron2', default= '0 0 12 ? * * 2023')     # add_argument()指定程序可以接受的命令行选项
args = parser.parse_args()      # parse_args()从指定的选项中返回一些数据
print(args)
# 定义cron表达式
# cron_expression =   b"* * 11 ? * * 2023"
# cron_expression1 =  b"* * * ? * * 2023"
# 定义cron表达式
print(args.cron1)
print(args.cron2)
cron_expression =  ((str)(args.cron1)).encode()
cron_expression1 = ((str)(args.cron2)).encode()
print("插入id1='1'任务：", lib.AddTimerTask(ctypes.c_char_p(cron_expression), c_callback1, ctypes.c_char_p(id1), ctypes.c_int(-1)))
print("插入id2='22'任务：",lib.AddTimerTask(ctypes.c_char_p(cron_expression1), c_callback2, ctypes.c_char_p(id2), ctypes.c_int(100)))
# print(lib.AddDelayTimerTask(60, c_callback3, ctypes.c_char_p(id3), ctypes.c_int(100)))
print(bool(lib.RemoveAll()))
print("插入id1='1'任务：", lib.AddTimerTask(ctypes.c_char_p(cron_expression), c_callback1, ctypes.c_char_p(id1), ctypes.c_int(-1)))
print("插入id2='22'任务：",lib.AddTimerTask(ctypes.c_char_p(cron_expression1), c_callback2, ctypes.c_char_p(id2), ctypes.c_int(100)))
def Thread():
    print("更新线程")
    while True:
        lib.Update()
        # print(lib.GetAppointedIDLatestTimeStr(id1))
        # print(lib.GetCurrentTimeStr())
        # print(lib.GetAppointedIDRemainingTime(id1))
        # print(lib.JudgeIDIsExist(id1))
        time.sleep(0.1)
# 创建线程
_thread.start_new_thread(Thread,())
# 在循环中不断去更新时间轮索引


count = 0
while True:
    count = count +1
    # 5s后取消指定id的任务
    if(count > 100):
        pass
        # a= lib.StopAppointedTask(ctypes.c_char_p(id2))
        # print("----------------------------------------------")
        # print(lib.JudgeIDIsExist(ctypes.c_char_p(id1)))
        # print("删除-------",lib.StopAppointedTask(ctypes.c_char_p(id1)))
        # print(lib.JudgeIDIsExist(ctypes.c_char_p(id1)))
        count = 0
    # lib.AddTimerTask(ctypes.c_char_p(cron_expression), c_callback1, ctypes.c_char_p(id2), ctypes.c_int(10))
    time.sleep(1)