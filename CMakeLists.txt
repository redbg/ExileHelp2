cmake_minimum_required(VERSION 3.21)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ExileHelp2.dll
project(ExileHelp2 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(ExileHelp2 SHARED
    src/DllMain.cpp
    src/mongoose.c
    src/mongoose.h

    # Qt Resource
    res/Resource.qrc
)

target_link_libraries(ExileHelp2 PRIVATE Qt6::Core)

# inject.exe
project(inject VERSION 0.1 LANGUAGES C)
add_executable(inject src/inject.c)
