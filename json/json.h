#pragma once
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <expected>
#include <sstream>

namespace json {
	struct json;

	struct json_object {
		std::vector<std::pair<std::string, json>> content;
		constexpr bool operator==(const json_object&) const = default;
	};

	struct json_array {
		std::vector<json> content;
		constexpr bool operator==(const json_array&) const = default;
	};

	struct json {
		std::variant<bool, double, std::string, json_object, json_array> content;
		constexpr bool operator==(const json&) const = default;
	};

	struct string_is_empty {};
	struct invalid_begin_character { std::string content; };
	struct unexpected_string_continuation_after_json { std::string content; };

	struct no_end_of_string { std::string content; };

	struct number_contains_multiple_dots { std::string content; };
	struct missing_number_after_negation { std::string content; };
	struct number_contains_multiple_negation { std::string content; };

	struct no_end_of_array { std::string content; };
	struct missing_comma_in_array { std::string content; std::string remaining; };

	struct no_end_of_object { std::string content; };
	struct missing_comma_in_object { std::string content; std::string remaining; };
	struct missing_property_name_string_in_object { std::string content; std::string remaining; };
	struct missing_semicolon { std::string content; };

	using json_error = std::variant<
		string_is_empty,
		invalid_begin_character,
		unexpected_string_continuation_after_json,

		no_end_of_string,

		number_contains_multiple_dots,
		missing_number_after_negation,
		number_contains_multiple_negation,

		no_end_of_array,
		missing_comma_in_array,

		no_end_of_object,
		missing_comma_in_object,
		missing_property_name_string_in_object,
		missing_semicolon
	>;

	namespace detail {
		template <typename T1, typename, bool>
		struct select {
			using type = T1;
		};

		template <typename T1, typename T2>
		struct select<T1, T2, false> {
			using type = T2;
		};

		using json_e = std::expected<json, json_error>;
		using json_remaining = std::pair<json, std::string_view>;
		using json_remaining_e = std::expected<json_remaining, json_error>;

		template <bool allows_partial>
		using select_json_return = select<
			json_remaining_e,
			json_e,
			allows_partial
		>::type;

		json_remaining_e from_string(std::string_view str);

		std::expected<std::pair<std::string, std::string_view>, json_error> string_from_string(std::string_view str) {
			char beg = str.at(0);
			size_t index = 1;
			std::stringstream res{};
			while (index < str.length()) {
				char c = str.at(index);
				if (c == beg)
					return std::pair{ res.str(), str.substr(index + 1) };
				if (c == '\\') {
					index += 1;
					if (index == str.length())
						return std::unexpected{ json_error{ no_end_of_string{ std::string{str} } } };
					else {
						res << str.at(index);
						index += 1;
					}
				}
				else {
					res << c;
					index += 1;
				}
			}
			return std::unexpected{ json_error{ no_end_of_string{ std::string{str} } } };
		}

		std::expected<std::pair<double, std::string_view>, json_error> number_from_string(std::string_view str) {
			bool negate = str.at(0) == '-';
			if (negate) {
				if (str.length() == 1)
					return std::unexpected{ json_error{ missing_number_after_negation{ std::string{ str } } } };
				if (str.at(1) == '-')
					return std::unexpected{ json_error{ number_contains_multiple_negation{ std::string{str} } } };
				if (str.at(1) < '0' || str.at(1) > '9')
					return std::unexpected{ json_error{ invalid_begin_character{ std::string{ str.substr(1) }}}};
				auto non_negated = number_from_string(str.substr(1));
				return non_negated.transform([](auto e) { return std::pair{ -e.first, std::move(e.second) }; });
			}

			auto end = std::find_if(str.begin(), str.end(), [](char c) {
				return !(c == '.' || (c >= '0' && c <= '9'));
			});

			auto used = str;

			if (str.end() != end)
				used = str.substr(0, end - str.begin());

			auto first_dot = std::find(used.begin(), used.end(), '.');

			if (auto has_dot = first_dot != used.end()) {
				if (auto has_second_dot = std::find(first_dot + 1, used.end(), '.') != used.end())
					return std::unexpected{ json_error{ number_contains_multiple_dots{ std::string{used} } } };

				return std::pair{ double(std::atof(used.data())), str.substr(used.length()) };
			}
			else {
				std::string buffer = "";
				return std::pair{ double(std::atoi(used.data())), str.substr(used.length()) };
			}
		}

		std::string_view parse_whitespaces(std::string_view str) {
			size_t index = 0;
			while (index < str.length()) {
				char c = str.at(index);
				if (c == ' ' || c == '\n' || c == '\t')
					index += 1;
				else
					break;
			}
			return str.substr(index);
		}

		std::expected<std::pair<json_object, std::string_view>, json_error> object_from_string(std::string_view str) {
			std::vector<std::pair<std::string, json>> result;

			constexpr size_t size_of_opening_object = std::string{ "{" }.length();
			auto remaining = str.substr(size_of_opening_object);

			while (true) {
				remaining = parse_whitespaces(remaining);
				if (remaining.length() == 0)
					return std::unexpected{ json_error{ no_end_of_object{ std::string{str} } } };
				char c = remaining.at(0);
				if (c == '}')
					return std::pair{ json_object{ std::move(result) }, remaining.substr(1) };
				if (bool requires_comma = !result.empty()) {
					if (c == ',') {
						remaining = parse_whitespaces(remaining.substr(1));
						if (remaining.length() == 0)
							return std::unexpected{ json_error{ no_end_of_object{ std::string{str} } } };
						c = remaining.at(0);
					}
					else
						return std::unexpected{ json_error{ missing_comma_in_object{
							std::string{str},
							std::string{remaining}
						} } };
				}
				if (remaining.length() == 0)
					return std::unexpected{ json_error{ no_end_of_object{ std::string{str} } } };

				if (c == '"' || c == '\'') {
					auto property_name_or_error = string_from_string(remaining);
					if (!property_name_or_error.has_value())
						return std::unexpected{ property_name_or_error.error() };
					else {
						std::string& property_name = property_name_or_error.value().first;
						remaining = property_name_or_error.value().second;
						remaining = parse_whitespaces(remaining);
						char c = remaining.at(0);
						if (c != ':') {
							return std::unexpected{ missing_semicolon{ std::string{str} } };
						}
						else {
							remaining = parse_whitespaces(remaining.substr(1));
							auto next_or_error = from_string(remaining);
							if (next_or_error.has_value()) {
								result.push_back({ std::move(property_name), std::move(next_or_error.value().first) });
								remaining = next_or_error.value().second;
							}
							else
								return std::unexpected{ next_or_error.error() };
						}
					}
				}
				else
					return std::unexpected{ missing_property_name_string_in_object{ std::string{str}, std::string{remaining} } };
			}
		}

		std::expected<std::pair<json_array, std::string_view>, json_error> array_from_string(std::string_view str) {
			std::vector<json> result;

			constexpr size_t size_of_opening_array = std::string{ "[" }.length();
			auto remaining = str.substr(size_of_opening_array);

			while (true) {
				remaining = parse_whitespaces(remaining);
				if (remaining.length() == 0)
					return std::unexpected{ json_error{ no_end_of_array{ std::string{str} } } };
				char c = remaining.at(0);
				if (c == ']')
					return std::pair{ json_array{ std::move(result) }, remaining.substr(1) };
				if (bool requires_comma = !result.empty()) {
					if (c == ',')
						remaining = parse_whitespaces(remaining.substr(1));
					else
						return std::unexpected{ json_error{ missing_comma_in_array{
							std::string{str},
							std::string{remaining}
						} } };
				}
				if (remaining.length() == 0)
					return std::unexpected{ json_error{ no_end_of_array{ std::string{str} } } };

				auto next = from_string(remaining);
				if (next.has_value()) {
					result.push_back(std::move(next.value().first));
					remaining = next.value().second;
				}
				else
					return std::unexpected{ next.error() };
			}
		}

		json_remaining_e from_string(std::string_view str) {
			if (str.length() == 0)
				return std::unexpected{ json_error{ string_is_empty{} } };

			const char first = str.at(0);

			if (first == '{')
				return detail::object_from_string(str)
					.transform([](auto e) { return json_remaining{ json{ std::move(e.first) }, std::move(e.second) }; });
			if (first == '[')
				return detail::array_from_string(str)
					.transform([](auto e) { return json_remaining{ json{ std::move(e.first) }, std::move(e.second) }; });
			if (first == '"' || first == '\'') {
				return detail::string_from_string(str)
					.transform([](auto e) { return json_remaining{ json{ std::move(e.first) }, std::move(e.second) }; });
			}
			if (first == '-' || first >= '0' && first <= '9')
				return detail::number_from_string(str)
					.transform([](auto e) { return json_remaining{ json{ e.first }, std::move(e.second) }; });
			if (str.starts_with("true"))
				return json_remaining{ json{ true }, str.substr(4) };
			if (str.starts_with("false"))
				return json_remaining{ json{ false }, str.substr(5) };

			return std::unexpected{ json_error{ invalid_begin_character{ std::string{str} } } };
		}

		json_e from_string_no_partial(std::string_view str) {
			json_remaining_e jre = from_string(str);
			if (!jre.has_value())
				return std::unexpected{ jre.error() };
			else {
				json_remaining& je = jre.value();
				if (je.second.length() != 0)
					return std::unexpected{ unexpected_string_continuation_after_json{ std::string{ je.second }  } };
				else
					return je.first;
			}
		}
	}

	std::expected<json, json_error> from_string(std::string_view str) {
		return detail::from_string_no_partial(str);
	}
}
