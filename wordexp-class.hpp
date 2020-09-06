/* POSIX.2 alike wordexp implementation for C++. */
/* Copyright (C) 2020 KoneLinx

   this file is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see
   <https://www.gnu.org/licenses/>.
*/

#include "wordexp-h.h"

template <typename Char_t>
struct Wordexp
{
	using char_t = Char_t;
	wordexp::wordexp_t<char_t> _wordexp;
	wordexp::error::type error;

	Wordexp(const char_t* words)
		: _wordexp{}
		, error{ wordexp::wordexp(words, &_wordexp, 0) }
	{}

	~Wordexp()
	{
		if (error == wordexp::error::OK)
			wordexp::wordfree(&_wordexp);
	}

	operator bool() const
	{
		return error == wordexp::error::OK;
	}

	const decltype(_wordexp.we_wordv) begin() const
	{
		return _wordexp.we_wordv;
	}
	const decltype(_wordexp.we_wordv) end() const
	{
		return _wordexp.we_wordv + _wordexp.we_wordc;
	}

};