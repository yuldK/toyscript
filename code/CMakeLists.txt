# include root directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# add fmt
add_subdirectory(fmt)

# main project
add_subdirectory(client)

# static library
add_subdirectory(common)
