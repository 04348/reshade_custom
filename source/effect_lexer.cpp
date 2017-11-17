/**
 * Copyright (C) 2014 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/reshade#license
 */

#include "effect_lexer.hpp"
#include <unordered_map>

namespace reshadefx
{
	namespace
	{
		enum token_type
		{
			DIGIT = '0',
			IDENT = 'A',
			SPACE = ' ',
		};

		const unsigned type_lookup[256] = {
			 0xFF,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00, SPACE,
			 '\n', SPACE, SPACE, SPACE,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,
			 0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,  0x00,
			 0x00,  0x00, SPACE,   '!',   '"',   '#',   '$',   '%',   '&',  '\'',
			  '(',   ')',   '*',   '+',   ',',   '-',   '.',   '/', DIGIT, DIGIT,
			DIGIT, DIGIT, DIGIT, DIGIT, DIGIT, DIGIT, DIGIT, DIGIT,   ':',   ';',
			 '<',    '=',   '>',   '?',   '@', IDENT, IDENT, IDENT, IDENT, IDENT,
			IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
			IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
			IDENT,   '[',  '\\',   ']',   '^', IDENT,  0x00, IDENT, IDENT, IDENT,
			IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
			IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT, IDENT,
			IDENT, IDENT, IDENT,   '{',   '|',   '}',   '~',  0x00,  0x00,  0x00,
		};
		const std::unordered_map<std::string, tokenid> keyword_lookup = {
			{ "asm", tokenid::reserved },
			{ "asm_fragment", tokenid::reserved },
			{ "auto", tokenid::reserved },
			{ "bool", tokenid::bool_ },
			{ "bool2", tokenid::bool2 },
			{ "bool2x2", tokenid::bool2x2 },
			{ "bool3", tokenid::bool3 },
			{ "bool3x3", tokenid::bool3x3 },
			{ "bool4", tokenid::bool4 },
			{ "bool4x4", tokenid::bool4x4 },
			{ "break", tokenid::break_ },
			{ "case", tokenid::case_ },
			{ "cast", tokenid::reserved },
			{ "catch", tokenid::reserved },
			{ "centroid", tokenid::reserved },
			{ "char", tokenid::reserved },
			{ "class", tokenid::reserved },
			{ "column_major", tokenid::reserved },
			{ "compile", tokenid::reserved },
			{ "const", tokenid::const_ },
			{ "const_cast", tokenid::reserved },
			{ "continue", tokenid::continue_ },
			{ "default", tokenid::default_ },
			{ "delete", tokenid::reserved },
			{ "discard", tokenid::discard_ },
			{ "do", tokenid::do_ },
			{ "double", tokenid::reserved },
			{ "dword", tokenid::uint_ },
			{ "dword2", tokenid::uint2 },
			{ "dword2x2", tokenid::uint2x2 },
			{ "dword3", tokenid::uint3, },
			{ "dword3x3", tokenid::uint3x3 },
			{ "dword4", tokenid::uint4 },
			{ "dword4x4", tokenid::uint4x4 },
			{ "dynamic_cast", tokenid::reserved },
			{ "else", tokenid::else_ },
			{ "enum", tokenid::reserved },
			{ "explicit", tokenid::reserved },
			{ "extern", tokenid::extern_ },
			{ "external", tokenid::reserved },
			{ "false", tokenid::false_literal },
			{ "FALSE", tokenid::false_literal },
			{ "float", tokenid::float_ },
			{ "float2", tokenid::float2 },
			{ "float2x2", tokenid::float2x2 },
			{ "float3", tokenid::float3 },
			{ "float3x3", tokenid::float3x3 },
			{ "float4", tokenid::float4 },
			{ "float4x4", tokenid::float4x4 },
			{ "for", tokenid::for_ },
			{ "foreach", tokenid::reserved },
			{ "friend", tokenid::reserved },
			{ "globallycoherent", tokenid::reserved },
			{ "goto", tokenid::reserved },
			{ "groupshared", tokenid::reserved },
			{ "half", tokenid::reserved },
			{ "half2", tokenid::reserved },
			{ "half2x2", tokenid::reserved },
			{ "half3", tokenid::reserved },
			{ "half3x3", tokenid::reserved },
			{ "half4", tokenid::reserved },
			{ "half4x4", tokenid::reserved },
			{ "if", tokenid::if_ },
			{ "in", tokenid::in },
			{ "inline", tokenid::reserved },
			{ "inout", tokenid::inout },
			{ "int", tokenid::int_ },
			{ "int2", tokenid::int2 },
			{ "int2x2", tokenid::int2x2 },
			{ "int3", tokenid::int3 },
			{ "int3x3", tokenid::int3x3 },
			{ "int4", tokenid::int4 },
			{ "int4x4", tokenid::int4x4 },
			{ "interface", tokenid::reserved },
			{ "linear", tokenid::linear },
			{ "long", tokenid::reserved },
			{ "matrix", tokenid::matrix },
			{ "mutable", tokenid::reserved },
			{ "namespace", tokenid::namespace_ },
			{ "new", tokenid::reserved },
			{ "noinline", tokenid::reserved },
			{ "nointerpolation", tokenid::nointerpolation },
			{ "noperspective", tokenid::noperspective },
			{ "operator", tokenid::reserved },
			{ "out", tokenid::out },
			{ "packed", tokenid::reserved },
			{ "packoffset", tokenid::reserved },
			{ "pass", tokenid::pass },
			{ "precise", tokenid::precise },
			{ "private", tokenid::reserved },
			{ "protected", tokenid::reserved },
			{ "public", tokenid::reserved },
			{ "register", tokenid::reserved },
			{ "reinterpret_cast", tokenid::reserved },
			{ "return", tokenid::return_ },
			{ "row_major", tokenid::reserved },
			{ "sample", tokenid::reserved },
			{ "sampler", tokenid::sampler },
			{ "sampler1D", tokenid::sampler },
			{ "sampler1DArray", tokenid::reserved },
			{ "sampler1DArrayShadow", tokenid::reserved },
			{ "sampler1DShadow", tokenid::reserved },
			{ "sampler2D", tokenid::sampler },
			{ "sampler2DArray", tokenid::reserved },
			{ "sampler2DArrayShadow", tokenid::reserved },
			{ "sampler2DMS", tokenid::reserved },
			{ "sampler2DMSArray", tokenid::reserved },
			{ "sampler2DShadow", tokenid::reserved },
			{ "sampler3D", tokenid::sampler },
			{ "sampler_state", tokenid::reserved },
			{ "samplerCUBE", tokenid::reserved },
			{ "samplerRECT", tokenid::reserved },
			{ "SamplerState", tokenid::reserved },
			{ "shared", tokenid::reserved },
			{ "short", tokenid::reserved },
			{ "signed", tokenid::reserved },
			{ "sizeof", tokenid::reserved },
			{ "snorm", tokenid::reserved },
			{ "static", tokenid::static_ },
			{ "static_cast", tokenid::reserved },
			{ "string", tokenid::string_ },
			{ "struct", tokenid::struct_ },
			{ "switch", tokenid::switch_ },
			{ "technique", tokenid::technique },
			{ "template", tokenid::reserved },
			{ "texture", tokenid::texture },
			{ "Texture1D", tokenid::reserved },
			{ "texture1D", tokenid::texture },
			{ "Texture1DArray", tokenid::reserved },
			{ "Texture2D", tokenid::reserved },
			{ "texture2D", tokenid::texture },
			{ "Texture2DArray", tokenid::reserved },
			{ "Texture2DMS", tokenid::reserved },
			{ "Texture2DMSArray", tokenid::reserved },
			{ "Texture3D", tokenid::reserved },
			{ "texture3D", tokenid::texture },
			{ "textureCUBE", tokenid::reserved },
			{ "TextureCube", tokenid::reserved },
			{ "TextureCubeArray", tokenid::reserved },
			{ "textureRECT", tokenid::reserved },
			{ "this", tokenid::reserved },
			{ "true", tokenid::true_literal },
			{ "TRUE", tokenid::true_literal },
			{ "try", tokenid::reserved },
			{ "typedef", tokenid::reserved },
			{ "uint", tokenid::uint_ },
			{ "uint2", tokenid::uint2 },
			{ "uint2x2", tokenid::uint2x2 },
			{ "uint3", tokenid::uint3 },
			{ "uint3x3", tokenid::uint3x3 },
			{ "uint4", tokenid::uint4 },
			{ "uint4x4", tokenid::uint4x4 },
			{ "uniform", tokenid::uniform_ },
			{ "union", tokenid::reserved },
			{ "unorm", tokenid::reserved },
			{ "unsigned", tokenid::reserved },
			{ "vector", tokenid::vector },
			{ "virtual", tokenid::reserved },
			{ "void", tokenid::void_ },
			{ "volatile", tokenid::volatile_ },
			{ "while", tokenid::while_ }
		};
		const std::unordered_map<std::string, tokenid> pp_directive_lookup = {
			{ "define", tokenid::hash_def },
			{ "undef", tokenid::hash_undef },
			{ "if", tokenid::hash_if },
			{ "ifdef", tokenid::hash_ifdef },
			{ "ifndef", tokenid::hash_ifndef },
			{ "else", tokenid::hash_else },
			{ "elif", tokenid::hash_elif },
			{ "endif", tokenid::hash_endif },
			{ "error", tokenid::hash_error },
			{ "warning", tokenid::hash_warning },
			{ "pragma", tokenid::hash_pragma },
			{ "include", tokenid::hash_include },
		};

		inline bool is_octal_digit(char c)
		{
			return static_cast<unsigned>(c - '0') < 8;
		}
		inline bool is_decimal_digit(char c)
		{
			return static_cast<unsigned>(c - '0') < 10;
		}
		inline bool is_hexadecimal_digit(char c)
		{
			return is_decimal_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
		}
		bool is_digit(char c, int radix)
		{
			switch (radix)
			{
				case 8:
					return is_octal_digit(c);
				case 10:
					return is_decimal_digit(c);
				case 16:
					return is_hexadecimal_digit(c);
			}

			return false;
		}
		long long octal_to_decimal(long long n)
		{
			long long m = 0;

			while (n != 0)
			{
				m *= 8;
				m += n & 7;
				n >>= 3;
			}

			while (m != 0)
			{
				n *= 10;
				n += m & 7;
				m >>= 3;
			}

			return n;
		}
	}

	lexer::lexer(const std::string &source, bool ignore_whitespace, bool ignore_pp_directives, bool ignore_keywords, bool escape_string_literals) :
		_input(source),
		_ignore_whitespace(ignore_whitespace),
		_ignore_pp_directives(ignore_pp_directives),
		_ignore_keywords(ignore_keywords),
		_escape_string_literals(escape_string_literals)
	{
		_cur = _input.data();
		_end = _cur + _input.size();
	}
	lexer::lexer(const lexer &lexer) :
		_input(lexer._input),
		_cur_location(lexer._cur_location),
		_ignore_whitespace(lexer._ignore_whitespace),
		_ignore_pp_directives(lexer._ignore_pp_directives),
		_ignore_keywords(lexer._ignore_keywords),
		_escape_string_literals(lexer._escape_string_literals)
	{
		_cur = _input.data() + (lexer._cur - lexer._input.data());
		_end = _input.data() + _input.size();
	}

	lexer &lexer::operator=(const lexer &lexer)
	{
		_input = lexer._input;
		_cur_location = lexer._cur_location;
		_cur = _input.data() + (lexer._cur - lexer._input.data());
		_end = _input.data() + _input.size();
		_ignore_whitespace = lexer._ignore_whitespace;
		_ignore_pp_directives = lexer._ignore_pp_directives;
		_ignore_keywords = lexer._ignore_keywords;
		_escape_string_literals = lexer._escape_string_literals;

		return *this;
	}

	token lexer::lex()
	{
		bool is_at_line_begin = _cur_location.column <= 1;
		token tok;
	next_token:
		tok.location = _cur_location;
		tok.offset = _cur - _input.data();
		tok.length = 1;
		tok.literal_as_double = 0;

		switch (type_lookup[*_cur])
		{
			case 0xFF:
				tok.id = tokenid::end_of_file;
				return tok;
			case SPACE:
				skip_space();
				if (_ignore_whitespace || is_at_line_begin || *_cur == '\n')
					goto next_token;
				tok.id = tokenid::space;
				return tok;
			case '\n':
				_cur++;
				_cur_location.line++;
				_cur_location.column = 1;
				is_at_line_begin = true;
				if (_ignore_whitespace)
					goto next_token;
				tok.id = tokenid::end_of_line;
				return tok;
			case DIGIT:
				parse_numeric_literal(tok);
				break;
			case IDENT:
				parse_identifier(tok);
				break;
			case '!':
				if (_cur[1] == '=')
					tok.id = tokenid::exclaim_equal,
					tok.length = 2;
				else
					tok.id = tokenid::exclaim;
				break;
			case '"':
				parse_string_literal(tok, _escape_string_literals);
				break;
			case '#':
				if (is_at_line_begin)
				{
					if (!parse_pp_directive(tok) || _ignore_pp_directives)
					{
						skip_to_next_line();
						goto next_token;
					}
				}
				else
					tok.id = tokenid::hash;
				break;
			case '$':
				tok.id = tokenid::dollar;
				break;
			case '%':
				if (_cur[1] == '=')
					tok.id = tokenid::percent_equal,
					tok.length = 2;
				else
					tok.id = tokenid::percent;
				break;
			case '&':
				if (_cur[1] == '&')
					tok.id = tokenid::ampersand_ampersand,
					tok.length = 2;
				else if (_cur[1] == '=')
					tok.id = tokenid::ampersand_equal,
					tok.length = 2;
				else
					tok.id = tokenid::ampersand;
				break;
			case '(':
				tok.id = tokenid::parenthesis_open;
				break;
			case ')':
				tok.id = tokenid::parenthesis_close;
				break;
			case '*':
				if (_cur[1] == '=')
					tok.id = tokenid::star_equal,
					tok.length = 2;
				else
					tok.id = tokenid::star;
				break;
			case '+':
				if (_cur[1] == '+')
					tok.id = tokenid::plus_plus,
					tok.length = 2;
				else if (_cur[1] == '=')
					tok.id = tokenid::plus_equal,
					tok.length = 2;
				else
					tok.id = tokenid::plus;
				break;
			case ',':
				tok.id = tokenid::comma;
				break;
			case '-':
				if (_cur[1] == '-')
					tok.id = tokenid::minus_minus,
					tok.length = 2;
				else if (_cur[1] == '=')
					tok.id = tokenid::minus_equal,
					tok.length = 2;
				else if (_cur[1] == '>')
					tok.id = tokenid::arrow,
					tok.length = 2;
				else
					tok.id = tokenid::minus;
				break;
			case '.':
				if (type_lookup[_cur[1]] == DIGIT)
					parse_numeric_literal(tok);
				else if (_cur[1] == '.' && _cur[2] == '.')
					tok.id = tokenid::ellipsis,
					tok.length = 3;
				else
					tok.id = tokenid::dot;
				break;
			case '/':
				if (_cur[1] == '/')
				{
					skip_to_next_line();
					goto next_token;
				}
				else if (_cur[1] == '*')
				{
					while (_cur < _end)
					{
						if (*_cur == '\n')
						{
							_cur_location.line++;
							_cur_location.column = 1;
						}
						else if (_cur[0] == '*' && _cur[1] == '/')
						{
							skip(2);
							break;
						}
						skip(1);
					}
					goto next_token;
				}
				else if (_cur[1] == '=')
					tok.id = tokenid::slash_equal,
					tok.length = 2;
				else
					tok.id = tokenid::slash;
				break;
			case ':':
				if (_cur[1] == ':')
					tok.id = tokenid::colon_colon,
					tok.length = 2;
				else
					tok.id = tokenid::colon;
				break;
			case ';':
				tok.id = tokenid::semicolon;
				break;
			case '<':
				if (_cur[1] == '<')
					if (_cur[2] == '=')
						tok.id = tokenid::less_less_equal,
						tok.length = 3;
					else
						tok.id = tokenid::less_less,
						tok.length = 2;
				else if (_cur[1] == '=')
					tok.id = tokenid::less_equal,
					tok.length = 2;
				else
					tok.id = tokenid::less;
				break;
			case '=':
				if (_cur[1] == '=')
					tok.id = tokenid::equal_equal,
					tok.length = 2;
				else
					tok.id = tokenid::equal;
				break;
			case '>':
				if (_cur[1] == '>')
					if (_cur[2] == '=')
						tok.id = tokenid::greater_greater_equal,
						tok.length = 3;
					else
						tok.id = tokenid::greater_greater,
						tok.length = 2;
				else if (_cur[1] == '=')
					tok.id = tokenid::greater_equal,
					tok.length = 2;
				else
					tok.id = tokenid::greater;
				break;
			case '?':
				tok.id = tokenid::question;
				break;
			case '@':
				tok.id = tokenid::at;
				break;
			case '[':
				tok.id = tokenid::bracket_open;
				break;
			case '\\':
				tok.id = tokenid::backslash;
				break;
			case ']':
				tok.id = tokenid::bracket_close;
				break;
			case '^':
				if (_cur[1] == '=')
					tok.id = tokenid::caret_equal,
					tok.length = 2;
				else
					tok.id = tokenid::caret;
				break;
			case '{':
				tok.id = tokenid::brace_open;
				break;
			case '|':
				if (_cur[1] == '=')
					tok.id = tokenid::pipe_equal,
					tok.length = 2;
				else if (_cur[1] == '|')
					tok.id = tokenid::pipe_pipe,
					tok.length = 2;
				else
					tok.id = tokenid::pipe;
				break;
			case '}':
				tok.id = tokenid::brace_close;
				break;
			case '~':
				tok.id = tokenid::tilde;
				break;
			default:
				tok.id = tokenid::unknown;
				break;
		}

		skip(tok.length);

		return tok;
	}

	void lexer::skip(size_t length)
	{
		_cur += length;
		_cur_location.column += static_cast<unsigned int>(length);
	}
	void lexer::skip_space()
	{
		while (type_lookup[*_cur] == SPACE && _cur < _end)
		{
			skip(1);
		}
	}
	void lexer::skip_to_next_line()
	{
		while (*_cur != '\n' && _cur < _end)
		{
			skip(1);
		}
	}

	void lexer::parse_identifier(token &tok) const
	{
		auto *const begin = _cur, *end = begin;

		do end++; while (type_lookup[*end] == IDENT || type_lookup[*end] == DIGIT);

		tok.id = tokenid::identifier;
		tok.length = end - begin;
		tok.literal_as_string.assign(begin, end);

		if (_ignore_keywords)
		{
			return;
		}

		const auto it = keyword_lookup.find(tok.literal_as_string);

		if (it != keyword_lookup.end())
		{
			tok.id = it->second;
		}
	}
	bool lexer::parse_pp_directive(token &tok)
	{
		skip(1);
		skip_space();
		parse_identifier(tok);

		const auto it = pp_directive_lookup.find(tok.literal_as_string);

		if (it != pp_directive_lookup.end())
		{
			tok.id = it->second;

			return true;
		}
		else if (tok.literal_as_string == "line")
		{
			skip(tok.length);
			skip_space();
			parse_numeric_literal(tok);
			skip(tok.length);

			_cur_location.line = tok.literal_as_int;

			if (_cur_location.line != 0)
			{
				_cur_location.line--;
			}

			skip_space();

			if (_cur[0] == '"')
			{
				token temptok;
				parse_string_literal(temptok, false);

				_cur_location.source = temptok.literal_as_string;
			}

			return false;
		}

		tok.id = tokenid::hash_unknown;

		return true;
	}
	void lexer::parse_string_literal(token &tok, bool escape) const
	{
		auto *const begin = _cur, *end = begin + 1;

		for (char c = *end; c != '"'; c = *++end)
		{
			if (c == '\n' || end >= _end)
			{
				end--;
				break;
			}
			if (c == '\\' && end[1] == '\n')
			{
				end++;
				continue;
			}

			if (c == '\\' && escape)
			{
				unsigned int n = 0;

				switch (c = *++end)
				{
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						for (unsigned int i = 0; i < 3 && is_octal_digit(*end) && end < _end; i++)
						{
							c = *end++;
							n = (n << 3) | (c - '0');
						}
						c = static_cast<char>(n & 0xFF);
						end--;
						break;
					case 'a':
						c = '\a';
						break;
					case 'b':
						c = '\b';
						break;
					case 'f':
						c = '\f';
						break;
					case 'n':
						c = '\n';
						break;
					case 'r':
						c = '\r';
						break;
					case 't':
						c = '\t';
						break;
					case 'v':
						c = '\v';
						break;
					case 'x':
						if (is_hexadecimal_digit(*++end))
						{
							while (is_hexadecimal_digit(*end) && end < _end)
							{
								c = *end++;
								n = (n << 4) | (is_decimal_digit(c) ? c - '0' : c - 55 - 32 * (c & 0x20));
							}

							c = static_cast<char>(n);
						}
						end--;
						break;
				}
			}

			tok.literal_as_string += c;
		}

		tok.id = tokenid::string_literal;
		tok.length = end - begin + 1;
	}
	void lexer::parse_numeric_literal(token &tok) const
	{
		auto *const begin = _cur, *end = _cur;
		int mantissa_size = 0, decimal_location = -1, radix = 10;
		long long fraction = 0, exponent = 0;

		if (begin[0] == '0')
		{
			if (begin[1] == 'x' || begin[1] == 'X')
			{
				end = begin + 2;
				radix = 16;
			}
			else
			{
				radix = 8;
			}
		}

		for (; mantissa_size <= 18; mantissa_size++, end++)
		{
			auto c = *end;

			if (is_decimal_digit(c))
			{
				c -= '0';

				if (c >= radix)
				{
					break;
				}
			}
			else if (radix == 16)
			{
				if (c >= 'A' && c <= 'F')
				{
					c -= 'A' - 10;
				}
				else if (c >= 'a' && c <= 'f')
				{
					c -= 'a' - 10;
				}
				else
				{
					break;
				}
			}
			else
			{
				if (c != '.' || decimal_location >= 0)
				{
					break;
				}

				if (radix == 8)
				{
					radix = 10;
					fraction = octal_to_decimal(fraction);
				}

				decimal_location = mantissa_size;
				continue;
			}

			fraction *= radix;
			fraction += c;
		}

		// Ignore additional digits that cannot affect the value
		while (is_digit(*end, radix))
		{
			end++;
		}

		if (decimal_location < 0)
		{
			tok.id = tokenid::int_literal;

			decimal_location = mantissa_size;
		}
		else
		{
			tok.id = tokenid::float_literal;

			mantissa_size -= 1;
		}

		if (*end == 'E' || *end == 'e')
		{
			auto tmp = end + 1;
			const bool negative = *tmp == '-';

			if (negative || *tmp == '+')
			{
				tmp++;
			}

			if (is_decimal_digit(*tmp))
			{
				end = tmp;

				tok.id = tokenid::float_literal;

				do
				{
					exponent *= 10;
					exponent += *end++ - '0';
				}
				while (is_decimal_digit(*end));

				if (negative)
				{
					exponent = -exponent;
				}
			}
		}

		if (*end == 'F' || *end == 'f')
		{
			end++;

			tok.id = tokenid::float_literal;
		}
		else if (*end == 'L' || *end == 'l')
		{
			end++;

			tok.id = tokenid::double_literal;
		}
		else if (tok.id == tokenid::int_literal && (*end == 'U' || *end == 'u'))
		{
			end++;

			tok.id = tokenid::uint_literal;
		}

		if (tok.id == tokenid::float_literal || tok.id == tokenid::double_literal)
		{
			exponent += decimal_location - mantissa_size;

			const bool exponent_negative = exponent < 0;

			if (exponent_negative)
			{
				exponent = -exponent;
			}

			if (exponent > 511)
			{
				exponent = 511;
			}

			double e = 1.0;
			const double powers_of_10[] = {
				10.,
				100.,
				1.0e4,
				1.0e8,
				1.0e16,
				1.0e32,
				1.0e64,
				1.0e128,
				1.0e256
			};

			for (auto d = powers_of_10; exponent != 0; exponent >>= 1, d++)
			{
				if (exponent & 1)
				{
					e *= *d;
				}
			}

			if (tok.id == tokenid::float_literal)
			{
				tok.literal_as_float = exponent_negative ? fraction / static_cast<float>(e) : fraction * static_cast<float>(e);
			}
			else
			{
				tok.literal_as_double = exponent_negative ? fraction / e : fraction * e;
			}
		}
		else
		{
			tok.literal_as_uint = static_cast<unsigned int>(fraction & 0xFFFFFFFF);
		}

		tok.length = end - begin;
	}
}
