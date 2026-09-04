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

find_package(glm REQUIRED)

# Homebrew's libomp is keg-only (not symlinked into the default compiler
# search paths), so AppleClang's find_package(OpenMP) can't locate it
# without these hints pointing straight at the Homebrew prefix.
if(APPLE)
    execute_process(
        COMMAND brew --prefix libomp
        OUTPUT_VARIABLE LIBOMP_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include")
    set(OpenMP_C_LIB_NAMES "omp")
    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY "${LIBOMP_PREFIX}/lib/libomp.dylib")
endif()

find_package(OpenMP REQUIRED)

# --- glad: compiling generated source directly into target, not an actual package ---
add_library(glad STATIC
    third_party/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    third_party/glad/include
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/renderer.cpp
    src/shader.cpp
    src/simulation.cpp
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    glfw
    OpenGL::GL
    glm::glm
    OpenMP::OpenMP_CXX
)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
)