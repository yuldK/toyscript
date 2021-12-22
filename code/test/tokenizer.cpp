#include "gtest/gtest.h"
#include <ycl/yclfwd.h>
#include <common/script/tokenizer.h>

using namespace std::string_literals;
using namespace script;

namespace
{
	auto convert_binary(const std::string& script)
	{
		return std::vector<char>{ script.begin(), script.end() };
	}

	std::string row_script
	{
R"(
[toy project test]
	[test obj] 1.5 -2 0 3
	[version] `1.0.0`
	[message] `hello, world!`

[toy project block]
	[simple tag]
[/toy project block]

[line tag] 1
[simple tag]
)"
	};
}

TEST(tokenizer, load)
{
	script::tokenizer loader{ convert_binary(row_script) };

	ASSERT_NE(loader.get_plain().size(), 0);
}

TEST(tokenizer, iterating)
{
	struct token_visiter
	{
		void operator()(double)
		{
			ASSERT_EQ(type_, token::type::kNumber);
		}
		void operator()(std::string_view)
		{
			ASSERT_TRUE(type_ == ycl::group::any(token::type::kString, token::type::kTag));
		}

		token_visiter(script::token::type type)
			: type_{ type }
		{
		}

	private:
		script::token::type type_;
	};

	script::tokenizer loader{ convert_binary(row_script) };

	for (const auto& token : loader)
		std::visit(token_visiter{ token.type_ }, token.get());
}

TEST(tokenizer, err_str_not_end)
{
	std::string row_script_err
	{
R"(
[toy project test]
	[test obj] 1.5 -2 0 3
	[version] `1.0.
)"
	};
	script::tokenizer loader{ convert_binary(row_script_err) };
	auto reason = loader.error_reason();
	ASSERT_TRUE(reason.has_value());
	std::cerr << reason.value() << std::endl;
}

TEST(tokenizer, err_num_dot_twice)
{
	std::string row_script_err
	{
R"(
[toy project test]
	[test obj] 1.5 -2 0 3
	[version] 1.0.
)"
	};
	script::tokenizer loader{ convert_binary(row_script_err) };
	auto reason = loader.error_reason();
	ASSERT_TRUE(reason.has_value());
	std::cerr << reason.value() << std::endl;
}

TEST(tokenizer, err_invalid_tag_id)
{
	std::string row_script_err
	{
R"(
[toy project tes)t]
)"
	};
	script::tokenizer loader{ convert_binary(row_script_err) };
	auto reason = loader.error_reason();
	ASSERT_TRUE(reason.has_value());
	std::cerr << reason.value() << std::endl;
}

TEST(tokenizer, check_valid_parsing)
{
	std::string row_script_simple
	{
R"(
[toy project test]
	[version] 20211222
	[message] `hello, world!`
)"
	};
	script::tokenizer loader{ convert_binary(row_script_simple) };
	auto it = loader.begin();
	ASSERT_TRUE(it++->get_tag() == "[toy project test]");
	ASSERT_TRUE(it++->get_tag() == "[version]");
	ASSERT_TRUE(it++->get_number() == 20211222);
	ASSERT_TRUE(it++->get_tag() == "[message]");
	ASSERT_TRUE(it++->get_string() == "hello, world!");
}
