cmake_minimum_required(VERSION 3.5.1)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


project(mpp_test)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "-O0 -g")

include_directories(
	/usr/include/opencv4
	/env/platforms/aarch64/include
	${CMAKE_CURRENT_LIST_DIR}/rockchip
)

link_directories(
	/env/platforms/aarch64/so
	/env/platforms/aarch64/static
	/usr/local/lib
	${CMAKE_CURRENT_LIST_DIR}/firefly_mpplib
	
)


add_executable(mpp_test MppDecode.cpp MppDecode.h main.cpp MppEncode.cpp MppEncode.h)

target_link_libraries(mpp_test
        rockchip_mpp
        utils
	opencv_core
	opencv_highgui
	opencv_imgproc
	opencv_imgcodecs
)
