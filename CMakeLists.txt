cmake_minimum_required(VERSION 3.25)
project(TodoAPP)

# ==== Compiler flags ====
set(EXTRA_OPT_FLAGS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Only add -march=native if not cross-compiling
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-march=native" HAS_MARCH_NATIVE)
    if(HAS_MARCH_NATIVE AND NOT CMAKE_CROSSCOMPILING)
        set(EXTRA_OPT_FLAGS "-march=native")
    endif()

    add_compile_options(
            -O2
            ${EXTRA_OPT_FLAGS}
            -ftree-vectorize
            -funroll-loops
            -fno-omit-frame-pointer
            -Wall -Wextra -Wpedantic
    )
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==== Boost ====
if(UNIX)
    list(APPEND CMAKE_PREFIX_PATH /usr /usr/local)
    set(BOOST_ROOT "/usr")
    set(Boost_NO_SYSTEM_PATHS OFF)
    set(Boost_NO_BOOST_CMAKE ON)
endif()

find_package(Boost REQUIRED COMPONENTS system json)
find_package(jh-toolkit REQUIRED)

# ==== Sources ====
add_executable(TodoAPP
        main.cpp
        Entity/Todo.hpp
        Application/TodoManager.hpp
        Web/views.cpp
        Persistence/InMemory/InMemoryTodoRepository.hpp
        Web/HttpUtils.hpp
        Persistence/CsvFiles/CSVHandler.hpp
)

# ==== Include & Link ====
include_directories(${Boost_INCLUDE_DIRS})

target_link_libraries(TodoAPP
        PRIVATE
        jh::jh-toolkit-pod
        ${Boost_LIBRARIES}
)
