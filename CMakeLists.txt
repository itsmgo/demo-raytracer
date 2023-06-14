cmake_minimum_required(VERSION 3.0.0)
project(raytracer VERSION 0.1.0)

find_package(glfw3 3.3 REQUIRED)
find_package(assimp 5.2 REQUIRED)


include(CTest)
enable_testing()


# GLAD
set(GLAD_DIR extern/glad)
add_library(
        glad
        STATIC
        ${GLAD_DIR}/glad.c
)
target_include_directories(glad PRIVATE ${GLAD_DIR})


# IMGUI
set(IMGUI_DIR extern/imgui)
add_library(
        imgui
        STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR})

# INTERNAL
include_directories(include extern)

add_executable(raytracer src/main.cpp)
target_link_libraries(raytracer glfw glad imgui assimp)


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
