# include root directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# add fmt
add_subdirectory(fmt)

# main project
add_subdirectory(client)

# static library
add_subdirectory(common)

if (BUILD_TESTING)
	# include gtest
	set(GTEST_ROOT "${ROOT}/code/ycl/googletest/")
	add_subdirectory(ycl/googletest)

	enable_testing()

	# test project
	add_subdirectory(test)
endif()
