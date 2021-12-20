#include "pch.h"
#include <common/common.h>
#include <common/script/tokenizer.h>

// helper type for the visitor #4
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

int main()
{
	tokenizer loader{ "script/sample.toy" };

	for (const auto& t : loader.tokens_)
	{
		std::visit(overloaded{
				[](double arg) 
				{
					auto title = "number token: ";
					std::cout << title << arg << std::endl;
				},
				[t](std::string_view arg) 
				{ 
					auto title = (t.type_ == token::type::kString ? "string token: " : "tag token: ");
					std::cout << title << arg.data() << std::endl;
				},
			}, t.get()
		);
	}
}
