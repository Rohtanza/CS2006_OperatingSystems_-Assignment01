cmake_minimum_required(VERSION 3.29)
project(CS2006_OperatingSystems__Assignment01)

set(CMAKE_CXX_STANDARD 20)

# Add the executable target
add_executable(CS2006_OperatingSystems__Assignment01 main.cpp)

# Link the readline library
target_link_libraries(CS2006_OperatingSystems__Assignment01 readline)
