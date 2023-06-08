cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

# "../" --> output directory relative same to running in IDE
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../)

set (CMAKE_GENERATOR_PLATFORM "x64")

project(lid2d_shaperec)

#find_package(OpenGL REQUIRED)

include_directories(./src ./lib ./lib/tinyptc ./lib/tnt ./lib/wmagic)

link_directories()


add_executable (${CMAKE_PROJECT_NAME}
./src/main.cpp
./src/bild.cpp
./src/bild.h
./src/bild2d.cpp
./src/bild2d.h
./src/filter.cpp
./src/filter.h
./src/global.h
./src/lader.cpp
./src/lader.h
./src/sequenz.cpp
./src/sequenz.h
./src/speicher.cpp
./src/speicher.h
./src/text.cpp
./src/text.h
./src/tokenizer.h
)

target_link_libraries (${CMAKE_PROJECT_NAME})
