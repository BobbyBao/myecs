
cmake_minimum_required (VERSION 3.8)

project ("myecs" C CXX)



set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

IF(MSVC)
add_compile_options(/MP)
add_compile_options(/MD)
ENDIF()

add_definitions(
-D_CRT_SECURE_NO_WARNINGS
-D_ITERATOR_DEBUG_LEVEL=0
-D_SILENCE_ALL_CXX20_DEPRECATION_WARNINGS
-D_SILENCE_ALL_CXX23_DEPRECATION_WARNINGS
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)



file(GLOB myecs_src src/*)

add_library (${PROJECT_NAME} STATIC ${myecs_src})

target_include_directories (${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)


add_subdirectory ("samples")
