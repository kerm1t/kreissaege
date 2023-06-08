cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# "../" --> output directory relative same to running in IDE
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../)

set (CMAKE_GENERATOR_PLATFORM "x64")

project(kreissaege)

#find_package(OpenGL REQUIRED)

include_directories(./src ./lib ./lib/tinyptc ./lib/tnt ./lib/wmagic)

link_directories(./lib ./libTtinyptc ./lib/tnt ./lib/wmagic)

#add_library(wild_magic)

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
./lib/wmagic/WmlBox2.cpp
./lib/wmagic/WmlContMinBox2.cpp
./lib/wmagic/WmlMath.cpp
./lib/wmagic/WmlSystem.cpp
./lib/wmagic/WmlVector2.cpp
./lib/wmagic/WmlWinSystem.cpp
./lib/wmagic/_WmlDistVec2Lin2.cpp
./lib/wmagic/_WmlLine2.cpp
./lib/wmagic/_WmlRay2.cpp
./lib/wmagic/_WmlSegment2.cpp
)

#CMAKE 3.1
#target_sources (wild_magic
#PUBLIC
#./lib/wmagic/WmlBox2.cpp
#./lib/wmagic/WmlContMinBox2.cpp
#./lib/wmagic/WmlMath.cpp
#./lib/wmagic/WmlSystem.cpp
#./lib/wmagic/WmlVector2.cpp
#./lib/wmagic/WmlWinSystem.cpp
#./lib/wmagic/_WmlDistVec2Lin2.cpp
#./lib/wmagic/_WmlLine2.cpp
#./lib/wmagic/_WmlRay2.cpp
#./lib/wmagic/_WmlSegment2.cpp
#./lib/wmagic/__WmlAxisAlignedBox2.cpp
#)

target_link_libraries (${CMAKE_PROJECT_NAME})
