#include "lexing.hpp"

#include <regex>
#include <limits>
#include <algorithm>
#include <stdexcept>

using namespace pebkac;
using namespace pebkac::lexing;


//This allows the use of "..."s to create an std::string instead of a char*
using namespace std::string_literals;


token::token(
	token_type type,
	const std::string& value) noexcept:
	type(type),
	value(value)
{ }


bool token::operator == (const token& other) const noexcept
{
	return type == other.type && value == other.value;
}


bool token::operator != (const token& other) const noexcept
{
	return !(*this == other);
}


const token_type& token::get_type() const noexcept
{
	return type;
}


const std::string& token::get_value() const noexcept
{
	return value;
}


std::shared_ptr<serialized> token::serialize() const
{
	auto obj = std::make_shared<serialized_object>();
	*obj += std::make_pair("type"s, to_string(type));
	*obj += std::make_pair("value"s, value);
	return obj;
}


std::string lexing::to_string(token_type t)
{
	if (t == token_type::COMMENT) return "COMMENT";
	if (t == token_type::NUMERIC_LITERAL) return "NUMERIC_LITERAL";
	if (t == token_type::BOOLEAN_LITERAL) return "BOOLEAN_LITERAL";
	if (t == token_type::IDENTIFIER) return "IDENTIFIER";
	if (t == token_type::OPERATOR) return "OPERATOR";
	if (t == token_type::KEYWORD) return "KEYWORD";
	if (t == token_type::BRACKET) return "BRACKET";
	if (t == token_type::SYNTATIC_ELEMENT) return "SYNTATIC_ELEMENT";

	throw std::runtime_error("Unknown token type.");
}


std::queue<token> lexing::tokenize(const std::string& source)
{
	// Regex query corresponding to each type of token/lexeme. Yes, regex is fugly.
	// Order of elements matters! Later elements in the list have higher priority.
	const static std::array<const std::pair<const token_type, const std::regex>, 8> regex_queries = {
		std::make_pair(token_type::COMMENT, std::regex("(\\/{2,}.*)|(\\/\\*[\\s\\S]*?\\*\\/)")),
		std::make_pair(token_type::IDENTIFIER, std::regex("\\w+")),
		std::make_pair(token_type::OPERATOR, std::regex("[+\\-*/%!]|!=|==|<|>|<=|>=|&&|\\|\\|")),
		std::make_pair(token_type::KEYWORD, std::regex("(fun|io|return|let|if|else)\\b")),
		std::make_pair(token_type::BRACKET, std::regex("[(){}[\\]]")),
		std::make_pair(token_type::SYNTATIC_ELEMENT, std::regex(":|;|->|=|,")),
		std::make_pair(token_type::NUMERIC_LITERAL, std::regex("\\d*\\.?\\d+")),
		std::make_pair(token_type::BOOLEAN_LITERAL, std::regex("(true|false)\\b")),
	};

	// Loop through the source code and parse it into tokens
	std::queue<token> result = {};
	for(std::string::const_iterator current = source.begin(); current != source.end();)
	{
		// Try every single regex
		size_t i = 0;
		size_t fails = 0;
		std::array<std::pair<size_t, std::string>, regex_queries.size()> query_results;
		for(const auto& [t, r] : regex_queries)
		{
			if (auto it = std::sregex_iterator(current, source.end(), r); it != std::sregex_iterator())
			{
				query_results[i] = std::make_pair(it->position(), it->str());
			}
			else
			{
				query_results[i] = std::make_pair(std::numeric_limits<size_t>::max(), "");
				++fails;
			}
			++i;
		}

		//If no more tokens exist, end the loop
		if (fails == regex_queries.size())
			break;

		// Get whichever regex matches better
		size_t index = std::distance(query_results.begin(), std::min_element(query_results.begin(), query_results.end(),
			[](const std::pair<size_t, std::string>& a, const std::pair<size_t, std::string>& b){
				if (a.first != b.first)
					return a.first < b.first;
				return a.second.length() >= b.second.length();
			}));

		//Update result
		result.push(token(regex_queries[index].first, query_results[index].second));
		current += query_results[index].first + query_results[index].second.length();
	}

	// C++11 has move-semantics, so it does not have to copy the local vector when returning.
	return result;
}
