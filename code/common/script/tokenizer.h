#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <optional>
#include <filesystem>

namespace script
{
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

	class tokenizer
	{
	public:
		class iterator
		{
		public:
			friend class tokenizer;
			using container = std::vector<token>;

		public:
			token& operator*()
			{
				return *current_;
			}

			const token& operator*() const
			{
				return *current_;
			}

			token* operator->()
			{
				return &*current_;
			}

			const token* operator->() const
			{
				return &*current_;
			}

			iterator& operator++()
			{
				++current_;
				return *this;
			}

			iterator operator++(int)
			{
				iterator old{ *this };
				++current_;
				return old;
			}

			bool operator==(iterator& other) const
			{
				return this->current_ == other.current_;
			}

			bool operator!=(iterator& other) const
			{
				return this->current_ != other.current_;
			}

			token& get()
			{
				return *current_;
			}

			const token& get() const
			{
				return *current_;
			}

		private:
			explicit iterator(container& origin, bool is_begin)
			{
				current_ = is_begin ? origin.begin() : origin.end();
			}

			iterator(const iterator& other)
			{
				current_ = other.current_;
			}

		private:
			container::iterator current_;
		};

	public:
		tokenizer(fs::path&& path);
		tokenizer(const fs::path& path);

		tokenizer(std::vector<char>&& binary);

	public:
		iterator begin()
		{
			return iterator{ tokens_, true };
		}

		iterator end()
		{
			return iterator{ tokens_, false };
		}

		std::optional<std::string_view> error_reason() const
		{
			return std::optional<std::string_view>{ error_reason_ };
		}

		std::string_view get_plain() const
		{
			return optimize_plain_;
		}

	private:
		void copy_script();
		void initialize();

	private:
		const fs::path path_;

		std::vector<token> tokens_;
		token::string_manager string_manager_;

		std::string error_reason_;
		std::string optimize_plain_;
	};
}
