cmake_minimum_required(VERSION 3.10)
project(Compiler)

option(DEBUG "Enable debug logging" OFF)

add_executable(compiler
    main.c
    ./utils/arena.c
    ./frontend/tokenizer.c
    ./frontend/rd.c
    ./frontend/sa.c
    ./backend/emitter.c
)

if(DEBUG)
    target_compile_definitions(compiler PRIVATE DEBUG=1)
else()
    target_compile_definitions(compiler PRIVATE DEBUG=0)
endif()

target_include_directories(compiler PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/utils
)
