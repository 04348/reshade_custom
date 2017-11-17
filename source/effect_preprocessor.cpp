/**
 * Copyright (C) 2014 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/reshade#license
 */

#include "effect_preprocessor.hpp"
#include <fstream>
#include <assert.h>

namespace reshadefx
{
	enum macro_replacement
	{
		macro_replacement_start = '\x00',
		macro_replacement_argument = '\xFA',
		macro_replacement_concat = '\xFF',
		macro_replacement_stringize = '\xFE',
		macro_replacement_space = '\xFD',
		macro_replacement_break = '\xFC',
		macro_replacement_expand = '\xFB',
	};

	namespace filesystem = reshade::filesystem;

	void preprocessor::add_include_path(const filesystem::path &path)
	{
		assert(!path.empty());

		_include_paths.push_back(path);
	}
	bool preprocessor::add_macro_definition(const std::string &name, const macro &macro)
	{
		assert(!name.empty());

		return _macros.emplace(name, macro).second;
	}
	bool preprocessor::add_macro_definition(const std::string &name, const std::string &value)
	{
		macro macro;
		macro.replacement_list = value;

		return add_macro_definition(name, macro);
	}

	bool preprocessor::run(const filesystem::path &file_path)
	{
		std::ifstream file(file_path.wstring());

		if (!file.is_open())
		{
			return false;
		}

		_success = true;
		_filecache.clear();

		const std::string filedata(std::istreambuf_iterator<char>(file.rdbuf()), std::istreambuf_iterator<char>());

		push(filedata + '\n', file_path.string());
		parse();

		return _success;
	}
	bool preprocessor::run(const filesystem::path &file_path, std::vector<filesystem::path> &included_files)
	{
		if (run(file_path))
		{
			for (const auto &element : _filecache)
			{
				included_files.push_back(element.first);
			}

			return true;
		}
		else
		{
			return false;
		}
	}

	// Error handling
	void preprocessor::error(const location &location, const std::string &message)
	{
		_errors += location.source + '(' + std::to_string(location.line) + ", " + std::to_string(location.column) + ')' + ": preprocessor error: " + message + '\n';
		_success = false;
	}
	void preprocessor::warning(const location &location, const std::string &message)
	{
		_errors += location.source + '(' + std::to_string(location.line) + ", " + std::to_string(location.column) + ')' + ": preprocessor warning: " + message + '\n';
	}

	// Input management
	lexer &preprocessor::current_lexer()
	{
		assert(!_input_stack.empty());

		return *_input_stack.top()._lexer;
	}
	std::stack<preprocessor::if_level> &preprocessor::current_if_stack()
	{
		assert(!_input_stack.empty());

		return _input_stack.top()._if_stack;
	}
	preprocessor::if_level &preprocessor::current_if_level()
	{
		return current_if_stack().top();
	}
	void preprocessor::push(const std::string &input, const std::string &name)
	{
		const auto parent = _input_stack.empty() ? nullptr : &_input_stack.top();

		_input_stack.emplace(name, input, parent);

		if (name.empty())
		{
			if (parent != nullptr)
			{
				_input_stack.top()._name = parent->_name;
			}
		}
		else
		{
			_output_location.source = name;
			_output += "#line 1 \"" + name + "\"\n";
		}

		consume();
	}
	bool preprocessor::peek(tokenid token) const
	{
		assert(!_input_stack.empty());

		return _input_stack.top()._next_token == token;
	}
	void preprocessor::consume()
	{
		assert(!_input_stack.empty());

		auto &input_level = _input_stack.top();
		_token = input_level._next_token;
		_token.location.source = _output_location.source;
		_current_token_raw_data = current_lexer().input_string().substr(_token.offset, _token.length);

		input_level._next_token = input_level._lexer->lex();
		input_level._offset = input_level._next_token.offset;

		// Pop input level if lexical analysis has reached the end of it
		while (_input_stack.top()._next_token == tokenid::end_of_file)
		{
			if (!current_if_stack().empty())
			{
				error(current_if_level().token.location, "unterminated #if");
			}

			_input_stack.pop();

			if (_input_stack.empty())
			{
				break;
			}

			if (_output_location.source != _input_stack.top()._name)
			{
				_output_location.line = 1;
				_output_location.source = _input_stack.top()._name;
				_output += "#line 1 \"" + _output_location.source + "\"\n";
			}
		}
	}
	void preprocessor::consume_until(tokenid token)
	{
		while (!accept(token) && !peek(tokenid::end_of_file))
		{
			consume();
		}
	}
	bool preprocessor::accept(tokenid token)
	{
		while (peek(tokenid::space))
		{
			consume();
		}

		if (peek(token))
		{
			consume();

			return true;
		}

		return false;
	}
	bool preprocessor::expect(tokenid token)
	{
		if (!accept(token))
		{
			assert(!_input_stack.empty());

			const auto &actual_token = _input_stack.top()._next_token;

			error(actual_token.location, "syntax error: unexpected token '" + current_lexer().input_string().substr(actual_token.offset, actual_token.length) + "'");

			return false;
		}

		return true;
	}

	// Parsing routines
	void preprocessor::parse()
	{
		std::string line;

		while (!_input_stack.empty())
		{
			_recursion_count = 0;

			const bool skip = !current_if_stack().empty() && current_if_level().skipping;

			consume();

			switch (current_token())
			{
				case tokenid::hash_if:
					parse_if();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
				case tokenid::hash_ifdef:
					parse_ifdef();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
				case tokenid::hash_ifndef:
					parse_ifndef();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
				case tokenid::hash_else:
					parse_else();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
				case tokenid::hash_elif:
					parse_elif();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
				case tokenid::hash_endif:
					parse_endif();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
			}

			if (skip)
			{
				continue;
			}

			switch (current_token())
			{
				case tokenid::hash_def:
					parse_def();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
				case tokenid::hash_undef:
					parse_undef();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
				case tokenid::hash_error:
					parse_error();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
				case tokenid::hash_warning:
					parse_warning();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
				case tokenid::hash_pragma:
					parse_pragma();
					if (!expect(tokenid::end_of_line))
						consume_until(tokenid::end_of_line);
					continue;
				case tokenid::hash_include:
					parse_include();
					continue;
				case tokenid::hash_unknown:
					error(current_token().location, "unrecognized preprocessing directive '" + current_token().literal_as_string + "'");
					consume_until(tokenid::end_of_line);
					continue;

				case tokenid::end_of_line:
					if (line.empty())
					{
						continue;
					}
					if (++_output_location.line != current_token().location.line)
					{
						_output += "#line " + std::to_string(_output_location.line = current_token().location.line) + '\n';
					}
					_output += line + '\n';
					line.clear();
					continue;

				case tokenid::identifier:
					if (evaluate_identifier_as_macro())
					{
						continue;
					}
				default:
					line += _current_token_raw_data;
					break;
			}
		}

		_output += line;
	}
	void preprocessor::parse_def()
	{
		if (!expect(tokenid::identifier))
		{
			return;
		}

		macro m;
		const auto location = current_token().location;
		const auto macro_name = current_token().literal_as_string;
		const auto macro_name_end_offset = current_token().offset + current_token().length;

		if (macro_name == "defined")
		{
			warning(location, "macro name 'defined' is reserved");
			return;
		}

		if (current_lexer().input_string()[macro_name_end_offset] == '(')
		{
			accept(tokenid::parenthesis_open);

			m.is_function_like = true;

			while (accept(tokenid::identifier))
			{
				m.parameters.push_back(current_token().literal_as_string);

				if (!accept(tokenid::comma))
				{
					break;
				}
			}

			if (accept(tokenid::ellipsis))
			{
				m.is_variadic = true;
				m.parameters.push_back("__VA_ARGS__");

				// TODO: Implement variadic macros
				error(current_token().location, "variadic macros are not currently supported");
				return;
			}

			if (!expect(tokenid::parenthesis_close))
			{
				return;
			}
		}

		create_macro_replacement_list(m);

		if (!add_macro_definition(macro_name, m))
		{
			error(location, "redefinition of '" + macro_name + "'");
			return;
		}
	}
	void preprocessor::parse_undef()
	{
		if (!expect(tokenid::identifier))
		{
			return;
		}

		const auto location = current_token().location;
		const auto &macro_name = current_token().literal_as_string;

		if (macro_name == "defined")
		{
			warning(location, "macro name 'defined' is reserved");
			return;
		}

		_macros.erase(macro_name);
	}
	void preprocessor::parse_if()
	{
		const auto parent = current_if_stack().empty() ? nullptr : &current_if_level();
		const bool condition_result = evaluate_expression();

		if_level level;
		level.token = current_token();
		level.value = condition_result;
		level.skipping = (parent != nullptr && parent->skipping) || !level.value;
		level.parent = parent;

		current_if_stack().push(level);
	}
	void preprocessor::parse_ifdef()
	{
		const auto parent = current_if_stack().empty() ? nullptr : &current_if_level();

		if_level level;
		level.token = current_token();

		if (!expect(tokenid::identifier))
		{
			return;
		}

		const auto &macro_name = current_token().literal_as_string;

		level.value = _macros.find(macro_name) != _macros.end();
		level.skipping = (parent != nullptr && parent->skipping) || !level.value;
		level.parent = parent;

		current_if_stack().push(level);
	}
	void preprocessor::parse_ifndef()
	{
		const auto parent = current_if_stack().empty() ? nullptr : &current_if_level();

		if_level level;
		level.token = current_token();

		if (!expect(tokenid::identifier))
		{
			return;
		}

		const auto &macro_name = current_token().literal_as_string;

		level.value = _macros.find(macro_name) == _macros.end();
		level.skipping = (parent != nullptr && parent->skipping) || !level.value;
		level.parent = parent;

		current_if_stack().push(level);
	}
	void preprocessor::parse_elif()
	{
		const auto keyword_location = current_token().location;

		if (current_if_stack().empty())
		{
			error(keyword_location, "missing #if for #elif");
			return;
		}
		if (current_if_level().token == tokenid::hash_else)
		{
			error(keyword_location, "#elif is not allowed after #else");
			return;
		}

		const bool condition_result = evaluate_expression();

		if_level &level = current_if_level();
		level.token = current_token();
		level.skipping = (level.parent != nullptr && level.parent->skipping) || level.value || !condition_result;

		if (!level.value)
		{
			level.value = condition_result;
		}
	}
	void preprocessor::parse_else()
	{
		const auto keyword_location = current_token().location;

		if (current_if_stack().empty())
		{
			error(keyword_location, "missing #if for #else");
			return;
		}
		if (current_if_level().token == tokenid::hash_else)
		{
			error(keyword_location, "#else is not allowed after #else");
			return;
		}

		if_level &level = current_if_level();
		level.token = current_token();
		level.skipping = (level.parent != nullptr && level.parent->skipping) || level.value;

		if (!level.value)
		{
			level.value = true;
		}
	}
	void preprocessor::parse_endif()
	{
		const auto keyword_location = current_token().location;

		if (current_if_stack().empty())
		{
			error(keyword_location, "missing #if for #endif");
			return;
		}

		current_if_stack().pop();
	}
	void preprocessor::parse_error()
	{
		const auto keyword_location = current_token().location;
				
		if (!expect(tokenid::string_literal))
		{
			return;
		}

		error(keyword_location, current_token().literal_as_string);
	}
	void preprocessor::parse_warning()
	{
		const auto keyword_location = current_token().location;

		if (!expect(tokenid::string_literal))
		{
			return;
		}

		warning(keyword_location, current_token().literal_as_string);
	}
	void preprocessor::parse_pragma()
	{
		if (!expect(tokenid::identifier))
		{
			return;
		}

		std::string pragma = current_token().literal_as_string;

		while (!peek(tokenid::end_of_line) && !peek(tokenid::end_of_file))
		{
			consume();

			switch (current_token())
			{
				case tokenid::int_literal:
				case tokenid::uint_literal:
					pragma += std::to_string(current_token().literal_as_int);
					break;
				case tokenid::identifier:
					if (evaluate_identifier_as_macro())
					{
						continue;
					}
				default:
					pragma += _current_token_raw_data;
					break;
			}
		}

		if (pragma == "once")
		{
			const auto it = _filecache.find(_output_location.source);

			if (it != _filecache.end())
			{
				it->second.clear();
			}
		}

		_pragmas.push_back(pragma);
	}
	void preprocessor::parse_include()
	{
		const auto keyword_location = current_token().location;

		while (accept(tokenid::identifier))
		{
			if (!evaluate_identifier_as_macro())
			{
				error(current_token().location, "syntax error: unexpected identifier in #include");
				consume_until(tokenid::end_of_line);
				return;
			}
		}

		if (!expect(tokenid::string_literal))
		{
			consume_until(tokenid::end_of_line);
			return;
		}

		filesystem::path filename = current_token().literal_as_string;
		filesystem::path filepath = filesystem::path(_output_location.source).remove_filename() / filename;

		if (!filesystem::exists(filepath))
		{
			filepath = filesystem::resolve(filename, _include_paths);
		}

		auto it = _filecache.find(filepath.string());

		if (it == _filecache.end())
		{
			std::ifstream file(filepath.wstring());

			if (!file.is_open())
			{
				error(keyword_location, "could not open included file '" + filepath.string() + "'");
				consume_until(tokenid::end_of_line);
				return;
			}

			const std::string filedata(std::istreambuf_iterator<char>(file.rdbuf()), std::istreambuf_iterator<char>());

			it = _filecache.emplace(filepath.string(), filedata + '\n').first;
		}

		push(it->second, filepath.string());
	}

	bool preprocessor::evaluate_expression()
	{
		enum op_type
		{
			op_none = -1,

			op_or,
			op_and,
			op_bitor,
			op_bitxor,
			op_bitand,
			op_not_equal,
			op_equal,
			op_less,
			op_greater,
			op_less_equal,
			op_greater_equal,
			op_leftshift,
			op_rightshift,
			op_add,
			op_subtract,
			op_modulo,
			op_divide,
			op_multiply,
			op_plus,
			op_negate,
			op_not,
			op_bitnot,
			op_parentheses
		};
		struct rpn_token
		{
			bool is_op;
			int value;
		};

		int stack[128];
		rpn_token rpn[128];
		size_t stack_count = 0, rpn_count = 0;
		tokenid previous_token = current_token();
		const int precedence[] = { 0, 1, 2, 3, 4, 5, 6, 7, 7, 7, 7, 8, 8, 9, 9, 10, 10, 10, 11, 11, 11, 11 };

		// Run shunting-yard algorithm
		while (!peek(tokenid::end_of_line))
		{
			if (stack_count >= _countof(stack) || rpn_count >= _countof(rpn))
			{
				error(current_token().location, "expression evaluator ran out of stack space");
				return false;
			}

			int op = op_none;
			bool is_left_associative = true;

			consume();

			switch (current_token())
			{
				case tokenid::exclaim:
					op = op_not;
					is_left_associative = false;
					break;
				case tokenid::percent:
					op = op_modulo;
					break;
				case tokenid::ampersand:
					op = op_bitand;
					break;
				case tokenid::star:
					op = op_multiply;
					break;
				case tokenid::plus:
					is_left_associative =
						previous_token == tokenid::int_literal ||
						previous_token == tokenid::uint_literal ||
						previous_token == tokenid::identifier ||
						previous_token == tokenid::parenthesis_close;
					op = is_left_associative ? op_add : op_plus;
					break;
				case tokenid::minus:
					is_left_associative =
						previous_token == tokenid::int_literal ||
						previous_token == tokenid::uint_literal ||
						previous_token == tokenid::identifier ||
						previous_token == tokenid::parenthesis_close;
					op = is_left_associative ? op_subtract : op_negate;
					break;
				case tokenid::slash:
					op = op_divide;
					break;
				case tokenid::less:
					op = op_less;
					break;
				case tokenid::greater:
					op = op_greater;
					break;
				case tokenid::caret:
					op = op_bitxor;
					break;
				case tokenid::pipe:
					op = op_bitor;
					break;
				case tokenid::tilde:
					op = op_bitnot;
					is_left_associative = false;
					break;
				case tokenid::exclaim_equal:
					op = op_not_equal;
					break;
				case tokenid::ampersand_ampersand:
					op = op_and;
					break;
				case tokenid::less_less:
					op = op_leftshift;
					break;
				case tokenid::less_equal:
					op = op_less_equal;
					break;
				case tokenid::equal_equal:
					op = op_equal;
					break;
				case tokenid::greater_greater:
					op = op_rightshift;
					break;
				case tokenid::greater_equal:
					op = op_greater_equal;
					break;
				case tokenid::pipe_pipe:
					op = op_or;
					break;
			}

			switch (current_token())
			{
				case tokenid::space:
					continue;
				case tokenid::parenthesis_open:
				{
					stack[stack_count++] = op_parentheses;
					break;
				}
				case tokenid::parenthesis_close:
				{
					bool matched = false;

					while (stack_count > 0)
					{
						const int op2 = stack[--stack_count];

						if (op2 == op_parentheses)
						{
							matched = true;
							break;
						}

						rpn[rpn_count].is_op = true;
						rpn[rpn_count++].value = op2;
					}

					if (!matched)
					{
						error(current_token().location, "unmatched ')'");
						return false;
					}
					break;
				}
				case tokenid::identifier:
				{
					if (evaluate_identifier_as_macro())
					{
						continue;
					}
					else if (current_token().literal_as_string == "exists")
					{
						const bool has_parentheses = accept(tokenid::parenthesis_open);

						while (accept(tokenid::identifier))
						{
							if (!evaluate_identifier_as_macro())
							{
								error(current_token().location, "syntax error: unexpected identifier after 'exists'");
								return false;
							}
						}

						if (!expect(tokenid::string_literal))
						{
							return false;
						}

						const filesystem::path filename = current_token().literal_as_string;
						const filesystem::path filename_with_current_directory = filesystem::path(_output_location.source).remove_filename() / filename;

						if (has_parentheses && !expect(tokenid::parenthesis_close))
						{
							return false;
						}

						rpn[rpn_count].is_op = false;
						rpn[rpn_count++].value = filesystem::exists(filename_with_current_directory) || filesystem::exists(filesystem::resolve(filename, _include_paths));
						continue;
					}
					else if (current_token().literal_as_string == "defined")
					{
						const bool has_parentheses = accept(tokenid::parenthesis_open);

						if (!expect(tokenid::identifier))
						{
							return false;
						}

						const bool is_macro_defined = _macros.find(current_token().literal_as_string) != _macros.end();

						if (has_parentheses && !expect(tokenid::parenthesis_close))
						{
							return false;
						}

						rpn[rpn_count].is_op = false;
						rpn[rpn_count++].value = is_macro_defined;
						continue;
					}

					// An identifier that cannot be replaced with a number becomes zero
					rpn[rpn_count].is_op = false;
					rpn[rpn_count++].value = 0;
					break;
				}
				case tokenid::int_literal:
				case tokenid::uint_literal:
				{
					rpn[rpn_count].is_op = false;
					rpn[rpn_count++].value = current_token().literal_as_int;
					break;
				}
				default:
				{
					if (op == op_none)
					{
						error(current_token().location, "invalid expression");
						return false;
					}

					const int precedence1 = precedence[op];

					while (stack_count > 0)
					{
						const int op2 = stack[stack_count - 1];

						if (op2 == op_parentheses)
						{
							break;
						}

						const int precedence2 = precedence[op2];

						if ((is_left_associative && (precedence1 <= precedence2)) || (!is_left_associative && (precedence1 < precedence2)))
						{
							stack_count--;
							rpn[rpn_count].is_op = true;
							rpn[rpn_count++].value = op2;
						}
						else
						{
							break;
						}
					}

					stack[stack_count++] = op;
					break;
				}
			}

			previous_token = current_token();
		}

		while (stack_count > 0)
		{
			const int op = stack[--stack_count];

			if (op == op_parentheses)
			{
				error(current_token().location, "unmatched ')'");
				return false;
			}

			rpn[rpn_count].is_op = true;
			rpn[rpn_count++].value = static_cast<int>(op);
		}

		// Evaluate reverse polish notation output
		for (rpn_token *token = rpn; rpn_count-- != 0; token++)
		{
			if (token->is_op)
			{
#define UNARY_OPERATION(op) { if (stack_count < 1) return 0; stack[stack_count - 1] = op stack[stack_count - 1]; }
#define BINARY_OPERATION(op) { if (stack_count < 2) return 0; stack[stack_count - 2] = stack[stack_count - 2] op stack[stack_count - 1]; stack_count--; }

				switch (token->value)
				{
					case op_or: BINARY_OPERATION(||); break;
					case op_and: BINARY_OPERATION(&&); break;
					case op_bitor: BINARY_OPERATION(|); break;
					case op_bitxor: BINARY_OPERATION(^); break;
					case op_bitand: BINARY_OPERATION(&); break;
					case op_not_equal: BINARY_OPERATION(!=); break;
					case op_equal: BINARY_OPERATION(==); break;
					case op_less: BINARY_OPERATION(<); break;
					case op_greater: BINARY_OPERATION(>); break;
					case op_less_equal: BINARY_OPERATION(<=); break;
					case op_greater_equal: BINARY_OPERATION(>=); break;
					case op_leftshift: BINARY_OPERATION(<<); break;
					case op_rightshift: BINARY_OPERATION(>>); break;
					case op_add: BINARY_OPERATION(+); break;
					case op_subtract: BINARY_OPERATION(-); break;
					case op_modulo: BINARY_OPERATION(%); break;
					case op_divide: BINARY_OPERATION(/); break;
					case op_multiply: BINARY_OPERATION(*); break;
					case op_plus: UNARY_OPERATION(+); break;
					case op_negate: UNARY_OPERATION(-); break;
					case op_not: UNARY_OPERATION(!); break;
					case op_bitnot: UNARY_OPERATION(~); break;
				}
			}
			else
			{
				stack[stack_count++] = token->value;
			}
		}

		if (stack_count != 1)
		{
			error(current_token().location, "invalid expression");
			return false;
		}

		return stack[0] != 0;
	}
	bool preprocessor::evaluate_identifier_as_macro()
	{
		if (_recursion_count++ >= 256)
		{
			error(current_token().location, "macro recursion too high");
			return false;
		}

		const auto it = _macros.find(current_token().literal_as_string);

		if (it == _macros.end())
		{
			return false;
		}

		const auto &macro = it->second;
		std::vector<std::string> arguments;

		if (macro.is_function_like)
		{
			if (!accept(tokenid::parenthesis_open))
			{
				return false;
			}

			while (true)
			{
				int parentheses_level = 0;
				std::string argument;

				while (true)
				{
					consume();

					if (current_token() == tokenid::parenthesis_open)
					{
						parentheses_level++;
					}
					else if (
						(current_token() == tokenid::parenthesis_close && --parentheses_level < 0) ||
						(current_token() == tokenid::comma && parentheses_level == 0))
					{
						break;
					}

					argument += _current_token_raw_data;
				}

				if (!argument.empty() && argument.back() == ' ')
				{
					argument.pop_back();
				}
				if (!argument.empty() && argument.front() == ' ')
				{
					argument.erase(0, 1);
				}

				arguments.push_back(argument);

				if (parentheses_level < 0)
				{
					break;
				}
			}
		}

		std::string input;
		expand_macro(it->second, arguments, input);

		push(input);

		return true;
	}

	// Macro management routines
	void preprocessor::expand_macro(const macro &macro, const std::vector<std::string> &arguments, std::string &out)
	{
		for (auto it = macro.replacement_list.begin(); it != macro.replacement_list.end(); ++it)
		{
			if (*it == macro_replacement_start)
			{
				switch (*++it)
				{
					case macro_replacement_concat:
						continue;
					case macro_replacement_stringize:
						out += '"' + arguments.at(*++it) + '"';
						break;
					case macro_replacement_argument:
						push(arguments.at(*++it) + static_cast<char>(macro_replacement_argument));
						while (!accept(tokenid::unknown))
						{
							consume();

							if (current_token() == tokenid::identifier && evaluate_identifier_as_macro())
							{
								continue;
							}

							out += _current_token_raw_data;
						}
						assert(_current_token_raw_data[0] == macro_replacement_argument);
						break;
				}
			}
			else
			{
				out += *it;
			}
		}
	}
	void preprocessor::create_macro_replacement_list(macro &macro)
	{
		if (macro.parameters.size() >= 0xFF)
		{
			error(current_token().location, "too many macro parameters");
			return;
		}

		while (!peek(tokenid::end_of_file) && !peek(tokenid::end_of_line))
		{
			consume();

			switch (current_token())
			{
				case tokenid::hash:
				{
					if (accept(tokenid::hash))
					{
						if (peek(tokenid::end_of_line))
						{
							error(current_token().location, "## cannot appear at end of macro text");
							return;
						}

						// the ## token concatenation operator
						macro.replacement_list += macro_replacement_start;
						macro.replacement_list += macro_replacement_concat;
						continue;
					}
					else if (macro.is_function_like)
					{
						if (!expect(tokenid::identifier))
						{
							return;
						}

						const auto it = std::find(macro.parameters.begin(), macro.parameters.end(), current_token().literal_as_string);

						if (it == macro.parameters.end())
						{
							error(current_token().location, "# must be followed by parameter name");
							return;
						}

						// the # stringize operator
						macro.replacement_list += macro_replacement_start;
						macro.replacement_list += macro_replacement_stringize;
						macro.replacement_list += static_cast<char>(std::distance(macro.parameters.begin(), it));
						continue;
					}
					break;
				}
				case tokenid::backslash:
				{
					if (peek(tokenid::end_of_line))
					{
						consume();
						continue;
					}
					break;
				}
				case tokenid::identifier:
				{
					const auto it = std::find(macro.parameters.begin(), macro.parameters.end(), current_token().literal_as_string);

					if (it != macro.parameters.end())
					{
						macro.replacement_list += macro_replacement_start;
						macro.replacement_list += macro_replacement_argument;
						macro.replacement_list += static_cast<char>(std::distance(macro.parameters.begin(), it));
						continue;
					}
					break;
				}
			}

			macro.replacement_list += _current_token_raw_data;
		}
	}
}
