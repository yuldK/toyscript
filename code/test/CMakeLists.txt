set(target test)

make_executable(${target} "CONSOLE")

add_sources(${target}
	"tokenizer test"
	"tokenizer.cpp"
)

add_sources(${target}
	"script test"
	"script.cpp"
)

add_and_link_dependency(${target} common fmt::fmt)

target_link_libraries(${target}
	PUBLIC
		GTest::gtest
		GTest::gtest_main
		${WINLIBS}
)

add_test(
	NAME
		"toyscript-test"
	COMMAND
		${target}
)
