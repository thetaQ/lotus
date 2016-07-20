cmake_minimum_required(VERSION 2.6)
project(LOTUS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# set source files
set (SOURCE_FILES lotus l_socket.c SAK_threadpool.c l_http.c lotus.c)
# set header files' path
INCLUDE_DIRECTORIES(./include/)


# micro switch
OPTION(DEBUG "Option description" OFF)
IF(DEBUG)
	ADD_DEFINITIONS(-DDEBUG)
ENDIF(DEBUG)

# build executable file
ADD_EXECUTABLE(lotus ${SOURCE_FILES})

# link pthread
TARGET_LINK_LIBRARIES(lotus pthread)
