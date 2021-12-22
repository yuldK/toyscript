#include <common/common.h>
#include "tokenizer.h"

namespace token_parser
{
	class base
	{
	public:
		// if not matched, return false
		virtual bool check_begin(char c) = 0;
		// if end parsing, return false
		// parsing fail is throw
		virtual bool parsing(char c) = 0;

		base(std::vector<script::token>& tokens)
			: tokens_{ tokens }
		{
		}

	protected:
		virtual void clear()
		{
			buffer_.clear();
		}

	protected:
		std::string buffer_;
		std::vector<script::token>& tokens_;
	};

	class number : public base
	{
	public:
		using base_class = base;

	public:
		bool check_begin(char c) override
		{
			if (check(c) || c == '-' || c == '+')
			{
				buffer_.push_back(c);
				return true;
			}
			return false;
		}

		number(std::vector<script::token>& tokens, script::token::string_manager& manager)
			: base{ tokens }
			, manager_{ manager }
		{
			clear();
		}

	protected:
		void clear() override
		{
			base_class::clear();
			is_using_dot_ = false;
		}

		bool parsing(char c) override
		{
			if (check(c))
			{
				buffer_.push_back(c);
				return true;
			}

			if (c == '.')
			{
				if (is_using_dot_)
					throw std::invalid_argument{ "number dot is only use once!" };

				buffer_.push_back(c);
				is_using_dot_ = true;
				return true;
			}

			double composite = std::atof(buffer_.c_str());
			tokens_.push_back(script::token::make_number(manager_, composite));

			clear();

			return false;
		}

		bool check(char c)
		{
			switch (c)
			{
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				return true;
			default:
				return false;
			}
		}

	protected:
		bool is_using_dot_;
		script::token::string_manager& manager_;
	};

	class string : public base
	{
	public:
		using base_class = base;

	public:
		bool check_begin(char c) override
		{
			if (c == '`')
			{
				return true;
			}
			return false;
		}

		string(std::vector<script::token>& tokens, script::token::string_manager& manager)
			: base{ tokens }
			, manager_{ manager }
		{
			clear();
		}

	protected:
		void clear() override
		{
			base_class::clear();
			is_parsing_end_ = false;
		}

		bool parsing(char c) override
		{
			if (is_parsing_end_)
			{
				clear();
				return false;
			}

			if ((buffer_.empty() || buffer_.back() != '\\') && c == '`')
			{
				size_t idx = manager_.set(std::move(buffer_));
				tokens_.push_back(script::token::make_string(manager_, idx));

				is_parsing_end_ = true;
				return true;
			}

			buffer_.push_back(c);
			return true;
		}

	protected:
		bool is_parsing_end_ = false;
		script::token::string_manager& manager_;
	};

	class tag : public base
	{
	public:
		using base_class = base;

	public:
		bool check_begin(char c) override
		{
			if (c == '[')
			{
				buffer_.push_back(c);
				return true;
			}
			return false;
		}

		tag(std::vector<script::token>& tokens, script::token::string_manager& manager)
			: base{ tokens }
			, manager_{ manager }
		{
			clear();
		}

	protected:
		void clear() override
		{
			base_class::clear();
			is_parsing_end_ = false;
		}

		bool parsing(char c) override
		{
			if (is_parsing_end_)
			{
				clear();
				return false;
			}

			if ((buffer_.empty() || buffer_.back() != '\\') && c == ']')
			{
				buffer_.push_back(c);

				size_t idx = manager_.set(std::move(buffer_));
				tokens_.push_back(script::token::make_tag(manager_, idx));

				is_parsing_end_ = true;
				return true;
			}

			if (check(c) == false)
				throw std::invalid_argument{ fmt::format("invalid tag string: `{0}`", std::string_view{ buffer_ + c }) };

			buffer_.push_back(c);
			return true;
		}

		bool check(char c)
		{
			switch (c)
			{
			case ' ': case '_': case '-':
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i':
			case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
			case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
			case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
			case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
				return true;
			default:
				return false;
			}
		}
	protected:
		bool is_parsing_end_ = false;
		script::token::string_manager& manager_;
	};
}

namespace script
{
	tokenizer::tokenizer(fs::path&& path)
		: path_{ std::move(path) }
	{
		copy_script();
		initialize();
	}

	tokenizer::tokenizer(const fs::path& path)
		: path_{ path }
	{
		copy_script();
		initialize();
	}

	tokenizer::tokenizer(std::vector<char>&& binary)
		: path_{}
	{
		optimize_plain_.append(binary.begin(), binary.end());
		namespace rc = std::regex_constants;
		optimize_plain_ = std::regex_replace(optimize_plain_, std::regex{ "//.*" }, "", rc::match_any);
		optimize_plain_ = std::regex_replace(optimize_plain_, std::regex{ "/\\*(.|\r|\n)*\\*/" }, "", rc::match_any);

		initialize();
	}

	void tokenizer::copy_script()
	{
		std::fstream file{ path_, std::ios::in | std::ios::binary };
		if (file.is_open() == false)
		{
			throw std::invalid_argument{ fmt::format("can not read script! invalid path: {}", path_.string()) };
		}
		else
		{
			// get script data
			size_t total_size = fs::file_size(path_);
			optimize_plain_.resize(total_size + 1);
			file.read(optimize_plain_.data(), total_size);
			optimize_plain_[total_size] = '\0';
			file.close();

			namespace rc = std::regex_constants;

			optimize_plain_ = std::regex_replace(optimize_plain_, std::regex{ "//.*" }, "", rc::match_any);
			optimize_plain_ = std::regex_replace(optimize_plain_, std::regex{ "/\\*(.|\r|\n)*\\*/" }, "", rc::match_any);
		}
	}

	void tokenizer::initialize()
	{
		token_parser::number parser_number{ tokens_, string_manager_ };
		token_parser::string parser_string{ tokens_, string_manager_ };
		token_parser::tag parser_tag{ tokens_, string_manager_ };

		std::array<token_parser::base*, 3> parser_set
		{
			&parser_number,
			&parser_string,
			&parser_tag
		};

		token_parser::base* current_parser = nullptr;

		try
		{
			for (const auto& c : optimize_plain_)
			{
				if (current_parser)
				{
					if (current_parser->parsing(c))
						continue;

					current_parser = nullptr;
				}

				for (auto parser : parser_set)
				{
					if (parser->check_begin(c) == false)
						continue;

					current_parser = parser;
					break;
				}

				// when parsing start
				if (current_parser)
					continue;

				// whitespace escape
				if (c == '\0' || c == ' ' || c == '\t' || c == '\r' || c == '\n')
					continue;

				throw std::invalid_argument{ fmt::format("invalid token found `{0}`: (int){1}", c, (int)c) };
			}

			if (current_parser)
				throw std::out_of_range{ "parser not end!" };
		}
		catch (std::exception& e)
		{
			error_reason_ += e.what();
			error_reason_.push_back('\n');
		}
	}
}
