#include "pch.h"
#include <common/common.h>
#include <common/script/tokenizer.h>

int main()
{
	tokenizer loader{ "script/sample.toy" };
	std::cout << loader.optimize_plain_ << std::endl << "(eof)" << std::endl;
	std::cout << "------------" << std::endl;

	struct token_visiter
	{
		void operator()(double arg)
		{
			auto title = "number token: ";
			std::cout << title << arg << std::endl;
		}
		void operator()(std::string_view arg)
		{
			auto title = (type_ == token::type::kString ? "string token: " : "tag token: ");
			std::cout << title << arg.data() << std::endl;
		}

		token_visiter(token::type type)
			: type_{ type }
		{
		}

	private:
		token::type type_;
	};

	for (const auto& t : loader.tokens_)
	{
		const std::variant<double, std::string_view>& var = t.get();

		std::visit(token_visiter{ t.type_ }, var);
	}

	if (loader.error_reason_.empty() == false)
	{
		std::cout << "------------" << std::endl;
		std::cerr << "error occurred:\n" << loader.error_reason_ << std::endl;
	}
}
