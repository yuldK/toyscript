#pragma once

#include <common/common.h>

#include <regex>
#include <functional>

#include <variant>
#include <optional>

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

struct token
{
public:
	enum class type : uint8_t
	{
		kUnknown,
		kNumber,
		kString,
		kTag,
		kMax
	};

	struct string_manager
	{
	public:
		std::string_view get(size_t idx) const
		{
			return std::string_view{ manager_.at(idx) };
		}

		size_t set(std::string&& str)
		{
			size_t idx = manager_.size();
			manager_.push_back(std::move(str));
			return idx;
		}

	private:
		std::vector<std::string> manager_;
	};

public:
	static token make_number(const string_manager& manager, double number)
	{
		return token{ manager, number };
	}

	static token make_string(const string_manager& manager, size_t index)
	{
		return token{ manager, token::type::kString, index };
	}

	static token make_tag(const string_manager& manager, size_t index)
	{
		return token{ manager, token::type::kTag, index };
	}

public:
	const type type_;

	std::variant<double, std::string_view> get() const
	{
		std::variant<double, std::string_view> value;
		switch (type_)
		{
		case token::type::kNumber:
			value = number_;
			break;
		case token::type::kString:
		case token::type::kTag:
			value = manager_.get(index_);
			break;
		}
		return value;
	}

	std::optional<double> get_number() const
	{
		std::optional<double> value;
		switch (type_)
		{
		case token::type::kNumber:
			value.emplace(number_);
			break;
		}
		return value;
	}

	std::optional<std::string_view> get_string() const
	{
		std::optional<std::string_view> value;
		switch (type_)
		{
		case token::type::kString:
			value.emplace(manager_.get(index_));
			break;
		}
		return value;
	}

	std::optional<std::string_view> get_tag() const
	{
		std::optional<std::string_view> value;
		switch (type_)
		{
		case token::type::kTag:
			value.emplace(manager_.get(index_));
			break;
		}
		return value;
	}

protected:
	token(const string_manager& manager, double number)
		: manager_{ manager }
		, type_{ type::kNumber }
		, number_{ number }
	{
	}

	token(const string_manager& manager, token::type type, size_t index)
		: manager_{ manager }
		, type_{ type }
		, index_{ index }
	{
	}

protected:
	const string_manager& manager_;

	union {
		double number_;
		size_t index_;
	};
};

class token_parser_base
{
public:
	// if not matched, return false
	virtual bool check_begin(char c) = 0;
	// if end parsing, return false
	// parsing fail is throw
	virtual bool parsing(char c) = 0;

	token_parser_base(std::vector<token>& tokens)
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
	std::vector<token>& tokens_;
};

class token_parser_number : public token_parser_base
{
public:
	using base_class = token_parser_base;

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

	token_parser_number(std::vector<token>& tokens, token::string_manager& manager)
		: token_parser_base{ tokens }
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
		tokens_.push_back(token::make_number(manager_, composite));

		clear();

		return false;
	}

	bool check(char c) 
	{
		return ('0' <= c && c <= '9');
	}

protected:
	bool is_using_dot_;
	token::string_manager& manager_;
};

class token_parser_string : public token_parser_base
{
public:
	using base_class = token_parser_base;

public:
	bool check_begin(char c) override
	{
		if (c == '`')
		{
			buffer_.push_back(c);
			return true;
		}
		return false;
	}

	token_parser_string(std::vector<token>& tokens, token::string_manager& manager)
		: token_parser_base{ tokens }
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
			buffer_.push_back(c);

			size_t idx = manager_.set(std::move(buffer_));
			tokens_.push_back(token::make_string(manager_, idx));

			is_parsing_end_ = true;
			return true;
		}

		buffer_.push_back(c);
		return true;
	}

protected:
	bool is_parsing_end_ = false;
	token::string_manager& manager_;
};

class token_parser_tag : public token_parser_base
{
public:
	using base_class = token_parser_base;

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

	token_parser_tag(std::vector<token>& tokens, token::string_manager& manager)
		: token_parser_base{ tokens }
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
			tokens_.push_back(token::make_string(manager_, idx));

			is_parsing_end_ = true;
			return true;
		}

		if (c != ' ' && c != '_' && c != '-'
			&& ('A' <= c && c <= 'Z') == false
			&& ('0' <= c && c <= '9') == false
			&& ('a' <= c && c <= 'z') == false
			)
			throw std::invalid_argument{ "invalid tag string!" };

		buffer_.push_back(c);
		return true;
	}

protected:
	bool is_parsing_end_ = false;
	token::string_manager& manager_;
};

class tokenizer
{
public:
	tokenizer(fs::path&& path)
		: path_{ std::move(path) }
	{
		initialize();
	}

	tokenizer(const fs::path& path)
		: path_{ path }
	{
		initialize();
	}

private:
	void initialize()
	{
		std::fstream file{ path_, std::ios::in | std::ios::binary };
		if (file.is_open() == false)
		{
			auto err_msg = std::string{ "can not read script! invalid path: " } + path_.string();
			throw std::exception{ err_msg.c_str() };
		}

		// get script data
		size_t total_size = fs::file_size(path_);
		std::string buffer;
		buffer.resize(total_size + 1);
		file.read(buffer.data(), total_size);
		buffer[total_size] = '\0';
		file.close();

		namespace rc = std::regex_constants;

		buffer = std::regex_replace(buffer, std::regex{ "//.*" }, "", rc::match_any);
		buffer = std::regex_replace(buffer, std::regex{ "/\\*(.|\r|\n)*\\*/" }, "", rc::match_any);
		token_parser_number parser_number{ tokens_, string_manager_ };
		token_parser_string parser_string{ tokens_, string_manager_ };
		token_parser_tag parser_tag{ tokens_, string_manager_ };

		token_parser_base* current_parser = nullptr;

		try
		{
			for (const auto& c : buffer)
			{
				if (current_parser)
				{
					if (current_parser->parsing(c))
						continue;

					current_parser = nullptr;
				}

				if (parser_number.check_begin(c))
				{
					current_parser = &parser_number;
					continue;
				}

				if (parser_string.check_begin(c))
				{
					current_parser = &parser_string;
					continue;
				}

				if (parser_tag.check_begin(c))
				{
					current_parser = &parser_tag;
					continue;
				}

				// whitespace escape
				if (c == '\0' || c == ' ' || c == '\t' || c == '\r' || c == '\n')
					continue;

				throw std::invalid_argument{ "invalid token found!"};
			}
		}
		catch (std::exception& e)
		{
			error_reason += e.what();
			error_reason.push_back('\n');
		}
	}

private:
	const fs::path path_;

public:
	std::vector<token> tokens_;
	token::string_manager string_manager_;

	std::string error_reason;

};
