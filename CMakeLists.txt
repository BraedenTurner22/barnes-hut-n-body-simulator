cmake_minimum_required(VERSION 3.20)
project(bh-n-body-sim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# --- GLFW, via Homebrew ---
find_package(glfw3 REQUIRED)

# --- OpenGL itself (provides the macOS framework link) ---
find_package(OpenGL REQUIRED)

# --- glad: compiling generated source directly into target, not an actual package ---
add_library(glad STATIC
    third_party/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    third_party/glad/include
)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    glfw
    OpenGL::GL
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
)