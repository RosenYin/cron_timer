# 设置CMake的最低版本要求
cmake_minimum_required(VERSION 3.1)
# 定义一个变量用来切换动态库还是可执行文件，0是可执行文件，1是动态库so
set(CUSTOM_COMPILE_OPTIONS "0")
# 定义项目名称
set(PROJECT_NAME cron_timer)
project(${PROJECT_NAME})
# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_FLAGS "-O1") # 设置C工程的 CFLAGS
set(CMAKE_CXX_FLAGS "-O3 -Werror") # 设置C++ 工程的 CXX_FLAGS
# 根据操作系统设置编译标志
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(CMAKE_CXX_FLAGS "-g -Wall -pthread -fPIC")
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4819 /wd4267")
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
endif ()

# 定义源文件列表
set(SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/src/cron_python_interface.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/cron_text.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/cron_timer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/cron_wheel.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
# 将源文件分组以提高IDE项目的可读性
source_group(source FILES ${SOURCE_FILES})
# 创建可执行文件
if(CUSTOM_COMPILE_OPTIONS MATCHES "0")
	add_executable(${PROJECT_NAME}
		${SOURCE_FILES}
	)
elseif(CUSTOM_COMPILE_OPTIONS MATCHES "1")
	add_library(${PROJECT_NAME} SHARED 
		${SOURCE_FILES}
	)
endif()
# 将头文件目录添加到可执行文件的包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
	/usr/include/python3.8
)
# 链接库
target_link_libraries(${PROJECT_NAME} pthread) 
