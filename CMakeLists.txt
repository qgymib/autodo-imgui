cmake_minimum_required(VERSION 3.2)
project(imgui)

set(IMGUI_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)

###############################################################################
# Functions
###############################################################################

# Enable all warning for target
function(setup_target_wall name)
    if (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${name} PRIVATE /W4 /WX)
    else ()
        target_compile_options(${name} PRIVATE -Wall -Wextra -Werror)
    endif ()
endfunction()

function(setup_required_package_lua name)
    find_package(Lua REQUIRED)
    target_include_directories(${name}
        PRIVATE
            ${LUA_INCLUDE_DIR})
    target_link_libraries(${name}
        PRIVATE
            ${LUA_LIBRARY})
endfunction()

function(setup_imgui_for_unix name)
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL REQUIRED)
    target_include_directories(${name} PUBLIC ${OPENGL_INCLUDE_DIR})
    target_link_libraries(${name} PRIVATE OpenGL::GL)
    target_sources(${name} PRIVATE ${IMGUI_ROOT}/backends/imgui_impl_opengl3.cpp)
    target_compile_options(${name} PUBLIC -DIMGUI_BACKEND_OPENGL3)

    find_package(glfw3 QUIET)
    if (glfw3_FOUND)
        target_include_directories(${name} PUBLIC ${GLFW_INCLUDE_DIRS})
        target_link_libraries(${name} PRIVATE glfw)
        target_sources(${name} PRIVATE ${IMGUI_ROOT}/backends/imgui_impl_glfw.cpp)
        target_compile_options(${name} PUBLIC -DIMGUI_BACKEND_GLFW)
    else ()
        find_package(SDL2 REQUIRED)
        target_include_directories(${name} PUBLIC ${SDL2_INCLUDE_DIRS})
        target_link_libraries(${name} PRIVATE ${SDL2_LIBRARIES})
        target_sources(${name} PRIVATE ${IMGUI_ROOT}/backends/imgui_impl_sdl.cpp)
        target_compile_options(${name} PUBLIC -DIMGUI_BACKEND_SDL)
    endif ()
endfunction()

###############################################################################
# Setup library
###############################################################################

add_library(${PROJECT_NAME} SHARED
    src/ImGui.cpp
    src/ImGuiAdapter.cpp
    ${IMGUI_ROOT}/imgui_demo.cpp
    ${IMGUI_ROOT}/imgui_draw.cpp
    ${IMGUI_ROOT}/imgui_tables.cpp
    ${IMGUI_ROOT}/imgui_widgets.cpp
    ${IMGUI_ROOT}/imgui.cpp
    ${IMGUI_ROOT}/misc/cpp/imgui_stdlib.cpp)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${IMGUI_ROOT}>
        $<BUILD_INTERFACE:${IMGUI_ROOT}/backends>
        $<BUILD_INTERFACE:${IMGUI_ROOT}/misc/cpp>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (WIN32)
    target_sources(imgui PRIVATE
        ${IMGUI_ROOT}/backends/imgui_impl_win32.cpp
        ${IMGUI_ROOT}/backends/imgui_impl_dx9.cpp)
else ()
    setup_imgui_for_unix(imgui)
endif ()

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES PREFIX "")
setup_target_wall(${PROJECT_NAME})

###############################################################################
# Dependency
###############################################################################

function(setup_freetype name)
    target_sources(${name} PRIVATE
        ${IMGUI_ROOT}/misc/freetype/imgui_freetype.cpp)
    target_compile_options(${name} PRIVATE -DIMGUI_ENABLE_FREETYPE)

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/freetype-2.12.1)
    set_property(TARGET freetype PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(${name} PRIVATE freetype)
endfunction()

# freetype
setup_freetype(imgui)
