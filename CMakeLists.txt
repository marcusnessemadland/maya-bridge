# Project Info
cmake_minimum_required(VERSION 3.10.2) #3.18
project(maya_bridge)
cmake_policy(VERSION 3.18)

# CMake Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================

# Module path for FindMaya script
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

# Find autodesk maya 2024 
set(MAYA_VERSION 2024 CACHE STRING "Maya version")
find_package(Maya REQUIRED)

# Sources
file(GLOB_RECURSE SOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    #
    ${CMAKE_CURRENT_SOURCE_DIR}/include/maya-bridge/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# Solution Filters
foreach(source IN LISTS SOURCE_FILES)
    get_filename_component(source_path "${source}" PATH)
    string(REPLACE "/" "\\" source_path_msvc "${source_path}")
    source_group("${source_path_msvc}" FILES "${source}")
endforeach()

# Finalize dll plugin library
add_library(
    ${PROJECT_NAME} 
    SHARED 
    ${SOURCE_FILES}
    )

target_link_libraries(${PROJECT_NAME} PRIVATE Maya::Maya)

target_include_directories(
    ${PROJECT_NAME} 
    PRIVATE 
    Maya::Maya
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )   

MAYA_PLUGIN(${PROJECT_NAME})

# Put in a "maya-bridge" folder in Visual Studio
 set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "maya-bridge ")
