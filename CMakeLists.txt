cmake_minimum_required(VERSION 3.16)

project(ttyssh LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#sudo apt install linux-headers-$(uname -r)
#sudo apt install libssl-dev libssh-dev


execute_process (COMMAND "pushd ./module && make && popd")

add_executable(ttyssh
    comport.h comport.cpp
    main.cpp
)
target_link_libraries(ttyssh  -lssh) # sudo apt install libssl-dev libssh-dev

