cmake_minimum_required(VERSION 3.6)
project(ModularInterpreter)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        compiler.cpp
        scope.cpp
        virtual_environment.cpp
)

set(HEADER_FILES
        compiler.h
        language.h
        optimizer.h
        scope.h
        types.h
        ve_commands.h
        virtual_environment.h
)

add_executable(ModularInterpreter main.cpp ${SOURCE_FILES} ${HEADER_FILES})
add_executable(Test main_test.cpp ${SOURCE_FILES} ${HEADER_FILES})