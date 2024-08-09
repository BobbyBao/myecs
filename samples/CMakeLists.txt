
# 将源代码添加到此项目的可执行文件。
add_executable (samples "samples.cpp" "samples.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET samples PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(samples myecs)
