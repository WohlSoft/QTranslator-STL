cmake_minimum_required(VERSION 2.8)

project(QTranslatorXtest)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE
            qm_dumper.cpp
            QTranslatorX/qm_translator.cpp )

add_executable(QTranslatorX ${SOURCE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
