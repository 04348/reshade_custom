/**
 * Copyright (C) 2014 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/reshade#license
 */

#pragma once

#include "source_location.hpp"

namespace reshadefx
{
	/// <summary>
	/// A collection of identifiers for various possible tokens.
	/// </summary>
	enum class tokenid
	{
		unknown = -1,
		end_of_file = 0,
		end_of_line = '\n',

		// operators
		space = ' ',
		exclaim = '!',
		hash = '#',
		dollar = '$',
		percent = '%',
		ampersand = '&',
		parenthesis_open = '(',
		parenthesis_close = ')',
		star = '*',
		plus = '+',
		comma = ',',
		minus = '-',
		dot = '.',
		slash = '/',
		colon = ':',
		semicolon = ';',
		less = '<',
		equal = '=',
		greater = '>',
		question = '?',
		at = '@',
		bracket_open = '[',
		backslash = '\\',
		bracket_close = ']',
		caret = '^',
		brace_open = '{',
		pipe = '|',
		brace_close = '}',
		tilde = '~',
		exclaim_equal = 256 /* != */,
		percent_equal /* %= */,
		ampersand_ampersand /* && */,
		ampersand_equal /* &= */,
		star_equal /* *= */,
		plus_plus /* ++*/,
		plus_equal /* += */,
		minus_minus /* -- */,
		minus_equal /* -= */,
		arrow /* -> */,
		ellipsis /* ... */,
		slash_equal /* /= */,
		colon_colon /* :: */,
		less_less_equal /* <<= */,
		less_less /* << */,
		less_equal /* <= */,
		equal_equal /* == */,
		greater_greater_equal /* >>= */,
		greater_greater /* >> */,
		greater_equal /* >= */,
		caret_equal /* ^= */,
		pipe_equal /* |= */,
		pipe_pipe /* || */,

		// identifiers
		reserved,
		identifier,

		// literals
		true_literal,
		false_literal,
		int_literal,
		uint_literal,
		float_literal,
		double_literal,
		string_literal,

		// keywords
		namespace_,
		struct_,
		technique,
		pass,
		for_,
		while_,
		do_,
		if_,
		else_,
		switch_,
		case_,
		default_,
		break_,
		continue_,
		return_,
		discard_,
		extern_,
		static_,
		uniform_,
		volatile_,
		precise,
		in,
		out,
		inout,
		const_,
		linear,
		noperspective,
		centroid,
		nointerpolation,

		void_,
		bool_,
		bool2,
		bool2x2,
		bool3,
		bool3x3,
		bool4,
		bool4x4,
		int_,
		int2,
		int2x2,
		int3,
		int3x3,
		int4,
		int4x4,
		uint_,
		uint2,
		uint2x2,
		uint3,
		uint3x3,
		uint4,
		uint4x4,
		float_,
		float2,
		float2x2,
		float3,
		float3x3,
		float4,
		float4x4,
		vector,
		matrix,
		string_,
		texture,
		sampler,

		// preprocessor directives
		hash_def,
		hash_undef,
		hash_if,
		hash_ifdef,
		hash_ifndef,
		hash_else,
		hash_elif,
		hash_endif,
		hash_error,
		hash_warning,
		hash_pragma,
		hash_include,
		hash_unknown,
	};

	/// <summary>
	/// A structure describing a single token in the input string.
	/// </summary>
	struct token
	{
		tokenid id;
		location location;
		size_t offset, length;
		union
		{
			int literal_as_int;
			unsigned int literal_as_uint;
			float literal_as_float;
			double literal_as_double;
		};
		std::string literal_as_string;

		inline operator tokenid() const { return id; }
	};

	/// <summary>
	/// A lexical analyzer implementation.
	/// </summary>
	class lexer
	{
	public:
		/// <summary>
		/// Construct a new lexical analyzer for an input string.
		/// </summary>
		/// <param name="source">The string to analyze.</param>
		explicit lexer(
			const std::string &input,
			bool ignore_whitespace = true,
			bool ignore_pp_directives = true,
			bool ignore_keywords = false,
			bool escape_string_literals = true);
		/// <summary>
		/// Construct a copy of an existing instance.
		/// </summary>
		/// <param name="lexer">The instance to copy.</param>
		lexer(const lexer &lexer);
		lexer &operator=(const lexer &);

		/// <summary>
		/// Get the input string this lexical analyzer works on.
		/// </summary>
		/// <returns>A constant reference to the input string.</returns>
		inline const std::string &input_string() const { return _input; }

		/// <summary>
		/// Perform lexical analysis on the input string and return the next token in sequence.
		/// </summary>
		/// <returns>The next token from the input string.</returns>
		token lex();

		/// <summary>
		/// Advances to the next token that is not whitespace.
		/// </summary>
		void skip_space();
		/// <summary>
		/// Advances to the next new line, ignoring all tokens.
		/// </summary>
		void skip_to_next_line();

	private:
		/// <summary>
		/// Skips an arbitary amount of characters in the input string.
		/// </summary>
		/// <param name="length">The number of input characters to skip.</param>
		void skip(size_t length);

		void parse_identifier(token &tok) const;
		bool parse_pp_directive(token &tok);
		void parse_string_literal(token &tok, bool escape) const;
		void parse_numeric_literal(token &tok) const;

		std::string _input;
		location _cur_location;
		const std::string::value_type *_cur, *_end;
		bool _ignore_whitespace;
		bool _ignore_pp_directives;
		bool _ignore_keywords;
		bool _escape_string_literals;
	};
}
