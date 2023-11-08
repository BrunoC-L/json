#include "json.h"
#include <stdexcept>
#include <iostream>
#include <fstream>

int main() {

	{
		auto a = json::from_string("1");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<double>(a.value().content))
			throw std::runtime_error("");
	}
	{
		auto a = json::detail::from_string("1]");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<double>(a.value().first.content))
			throw std::runtime_error("");
		if (a.value().second != "]")
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("1.1");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<double>(a.value().content))
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("1.1.1");
		if (a.has_value())
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("'abcd'");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<std::string>(a.value().content))
			throw std::runtime_error("");
		if (std::get<std::string>(a.value().content) != "abcd")
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("'abcd\"\"'");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<std::string>(a.value().content))
			throw std::runtime_error("");
		if (std::get<std::string>(a.value().content) != "abcd\"\"")
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("\"abcd\''''\"");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<std::string>(a.value().content))
			throw std::runtime_error("");
		if (std::get<std::string>(a.value().content) != "abcd\''''")
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("'\\\\'");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<std::string>(a.value().content))
			throw std::runtime_error("");
		if (std::get<std::string>(a.value().content) != "\\")
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("\"\\\"\"");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<std::string>(a.value().content))
			throw std::runtime_error("");
		if (std::get<std::string>(a.value().content) != "\"")
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("[]");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<json::json_array>(a.value().content))
			throw std::runtime_error("");
		if (std::get<json::json_array>(a.value().content) != json::json_array{})
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("[1]");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<json::json_array>(a.value().content))
			throw std::runtime_error("");
		if (std::get<json::json_array>(a.value().content) != json::json_array{ std::vector{ json::json{1.0} } })
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("[1, 1]");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<json::json_array>(a.value().content))
			throw std::runtime_error("");
		if (std::get<json::json_array>(a.value().content) != json::json_array{ std::vector{ json::json{1.0}, json::json{1.0} } })
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("[1 1]");
		if (a.has_value())
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("[1, [1, 1]]");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<json::json_array>(a.value().content))
			throw std::runtime_error("");
		if (std::get<json::json_array>(a.value().content) != json::json_array{ std::vector{ json::json{1.0}, json::json{ json::json_array{ std::vector{ json::json{1.0}, json::json{1.0} } } } } })
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("{'a': 1}");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<json::json_object>(a.value().content))
			throw std::runtime_error("");
		if (std::get<json::json_object>(a.value().content) != json::json_object{ std::vector{ std::pair{ std::string{"a"}, json::json{1.0} } } })
			throw std::runtime_error("");
	}
	{
		auto a = json::from_string("{'a': {'a': 1}}");
		if (!a.has_value())
			throw std::runtime_error("");
		if (!std::holds_alternative<json::json_object>(a.value().content))
			throw std::runtime_error("");
		if (std::get<json::json_object>(a.value().content) != json::json_object{ std::vector{ std::pair{ std::string{"a"}, json::json{ json::json_object{ std::vector{ std::pair{ std::string{"a"}, json::json{1.0} } } } } } } })
			throw std::runtime_error("");
	}
	{
		std::string content;
		{
			std::string filename = "D:/tmp/1/Default/Preferences";
			std::ifstream file{ filename };
			if (!file.is_open())
				throw std::runtime_error("could not open file");
			std::getline(file, content, '\0');
		}
		auto res = json::from_string(content);
		if (!res.has_value())
			throw std::runtime_error("");
	}
	return 0;
}
