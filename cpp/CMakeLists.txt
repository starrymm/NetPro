cmake_minimum_required(VERSION 3.5)
project(my_net)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin)
add_compile_options("-std=c++11")
add_definitions("-Wall -g")
set(src_FILES 
    main.cpp
    Handler.cpp
    EpollPoller.cpp
    Reactor.cpp
    )

add_executable(main ${src_FILES})

