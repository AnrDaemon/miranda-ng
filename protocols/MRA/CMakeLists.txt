file(GLOB SOURCES "src/*.h" "src/*.cpp" "res/*.rc")
set(TARGET MRA)
include(${CMAKE_SOURCE_DIR}/cmake/plugin.cmake)
target_link_libraries(${TARGET} Zlib)
add_subdirectory(proto_mra)
add_subdirectory(xstatus_mra)