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

/* Copyright (C) 1991-2020 Free Software Foundation, Inc.
   This file is sourced from The GNU C Library. */


#ifndef _TPP_WORDEXP
#define _TPP_WORDEXP
#include "wordexp.h"
#endif // !_TPP_WORDEXP

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <utility>
#include <algorithm>
#include <string>

namespace wordexp
{
	namespace implementation_detail
	{

		/*case: literal cast*/
		template <typename char_t>
		constexpr char_t _char_to(const char& c)
		{
			return char_t(c);
		}

		/*
		 * This is a recursive-descent-style word expansion routine.
		 * This code is sourced from the GNU C Library's POSIX code.
		 */

		 /* The w_*() functions manipulate word lists. */
#pragma region w_

#define W_CHUNK	(100)

/* Result of w_newword will be ignored if it's the last word. */
		static inline void*
			w_newword(size_t* actlen, size_t* maxlen)
		{
			*actlen = *maxlen = 0;
			return nullptr;
		}

		template <typename char_t>
		static char_t*
			w_addchar(char_t* buffer, size_t* actlen, size_t* maxlen, char_t ch)
			/* (lengths exclude trailing zero) */
		{
			/* Add a character to the buffer, allocating room for it if needed.  */

			if (*actlen == *maxlen)
			{
				char_t* old_buffer = buffer;
				assert(buffer == nullptr || *maxlen != 0);
				*maxlen += W_CHUNK;
				buffer = (char_t*)realloc(buffer, 1 + *maxlen);

				if (buffer == nullptr)
					free(old_buffer);
			}

			if (buffer != nullptr)
			{
				buffer[*actlen] = ch;
				buffer[++(*actlen)] = _char_to<char_t>('\0');
			}

			return buffer;
		}

		template <typename char_t>
		static char_t*
			w_addmem(char_t* buffer, size_t* actlen, size_t* maxlen, const char_t* str, size_t len)
		{
			/* Add a string to the buffer, allocating room for it if needed.
			 */
			if (*actlen + len > * maxlen)
			{
				char_t* old_buffer = buffer;
				assert(buffer == nullptr || *maxlen != 0);
				*maxlen += std::max(2 * len, (size_t)W_CHUNK);
				buffer = (char_t*)realloc(old_buffer, (1 + *maxlen) * sizeof(char_t)); // TODO:  MEMORY SIZE !!!!   //ok?

				if (buffer == nullptr)
					free(old_buffer);
			}

			if (buffer != nullptr)
			{
				*((char_t*)memcpy(&buffer[*actlen], str, len * sizeof(char_t))) = _char_to<char_t>('\0'); // TODO:  MEMORY SIZE !!!!   //ok?
				*actlen += len;
			}

			return buffer;
		}

		template <typename char_t>
		static char_t*
			w_addstr(char_t* buffer, size_t* actlen, size_t* maxlen, const char_t* str)
			/* (lengths exclude trailing zero) */
		{
			/* Add a string to the buffer, allocating room for it if needed.
			 */
			size_t len;

			assert(str != nullptr); /* w_addstr only called from this file */
			len = std::char_traits<char_t>::length(str); //strlen (str); // TODO: CHAR SIZE !!!   //ok?

			return w_addmem(buffer, actlen, maxlen, str, len);
		}

		template <typename char_t>
		static error::type
			w_addword(wordexp_t<char_t>* pwordexp, char_t* word)
		{
			/* Add a word to the wordlist */
			size_t num_p;
			char_t** new_wordv;
			bool allocated = false;

			/* Internally, nullptr acts like "".  Convert NULLs to "" before
			 * the caller sees them.
			 */
			if (word == nullptr)
			{

				{ // a way to get around strdup(char*). All it does is point to an allocated null char_t
					char to_copy[sizeof(char_t)]{};
					std::fill(to_copy, to_copy + sizeof(char_t) - 1, 1);
					// to_copy == "\1(...)\1\0"

					word = (char_t*)_strdup(to_copy); // = strdup(""); // TODO: CHAR SIZE !!!

					*word = _char_to<char_t>('\0');
				}

				if (word == nullptr)
					goto no_space;

				allocated = true;
			}

			num_p = 2 + pwordexp->we_wordc + pwordexp->we_offs;
			new_wordv = (char_t**)realloc(pwordexp->we_wordv, sizeof(char_t*) * num_p); // TODO:  MEMORY SIZE LEN !!!!  //ok?
			if (new_wordv != nullptr)
			{
				pwordexp->we_wordv = new_wordv;
				pwordexp->we_wordv[pwordexp->we_offs + pwordexp->we_wordc++] = word;
				pwordexp->we_wordv[pwordexp->we_offs + pwordexp->we_wordc] = nullptr;
				return 0;
			}

			if (allocated)
				free(word);

		no_space:
			return error::NOSPACE;
		}
#pragma endregion

		/* The parse_*() functions should leave *offset being the offset in 'words'
		 * to the last character processed.
		 */

		 /*parse backslash*/
		template <typename char_t>
		static error::type
			parse_backslash(char_t** word, size_t* word_length, size_t* max_length, const char_t* words, size_t* offset)
		{
			/* We are poised _at_ a backslash, not in quotes */

			switch (words[1 + *offset])
			{
			case 0:
				/* Backslash is last character of input words */
				return error::SYNTAX;

			case '\n':
				++(*offset);
				break;

			default:
				*word = w_addchar(*word, word_length, max_length, words[1 + *offset]);
				if (*word == nullptr)
					return error::NOSPACE;

				++(*offset);
				break;
			}

			return 0;
		}

		/*parse qtd backslash*/
		template <typename char_t>
		static error::type
			parse_qtd_backslash(char_t** word, size_t* word_length, size_t* max_length, const char_t* words, size_t* offset)
		{
			/* We are poised _at_ a backslash, inside quotes */

			switch (words[1 + *offset])
			{
			case 0:
				/* Backslash is last character of input words */
				return error::SYNTAX;

			case '\n':
				++(*offset);
				break;

			case '$':
			case '`':
			case '"':
			case '\\':
				*word = w_addchar(*word, word_length, max_length, words[1 + *offset]);
				if (*word == nullptr)
					return error::NOSPACE;

				++(*offset);
				break;

			default:
				*word = w_addchar(*word, word_length, max_length, words[*offset]);
				if (*word != nullptr)
					*word = w_addchar(*word, word_length, max_length, words[1 + *offset]);

				if (*word == nullptr)
					return error::NOSPACE;

				++(*offset);
				break;
			}

			return 0;
		}

		//removed
		/*parse tilde*/

		//removed
		/*do parse glob*/

		//removed
		/*parse glob*/

		/*parse squote*/
		template <typename char_t>
		static error::type
			parse_squote(char_t** word, size_t* word_length, size_t* max_length, const char_t* words, size_t* offset)
		{
			/* We are poised just after a single quote */
			for (; words[*offset]; ++(*offset))
			{
				if (words[*offset] != '\'')
				{
					*word = w_addchar(*word, word_length, max_length, words[*offset]);
					if (*word == nullptr)
						return error::NOSPACE;
				}
				else return 0;
			}

			/* Unterminated string */
			return error::SYNTAX;
		}

		//removed
		/*eval expr val*/

		//removed
		/*eval expr multdiv*/

		//removed
		/*eval expr*/

		//removed ( possible furure feature )
		/*parse arith*/

		//removed
		/*dynarray defines*/

		//removed
		/*exec comm child*/

		//removed
		/*exec comm*/

		//removed
		/*parse comm*/

		//removed
		/*parse param*/

		//remove ( possible furure feature )
		/*parse dollar*/

		template <typename char_t>
		static error::type
			parse_dquote(char_t** word, size_t* word_length, size_t* max_length, const char_t* words, size_t* offset, flag::type flags, wordexp_t<char_t>* pwordexp, const char_t* ifs, const char_t* ifs_white)
		{
			/* We are poised just after a double-quote */
			int error;

			for (; words[*offset]; ++(*offset))
			{
				switch (words[*offset])
				{
					case _char_to<char_t>('"') :
						return 0;

						//case '$':
						//  error = parse_dollars (word, word_length, max_length, words, offset,
						//			 flags, pwordexp, ifs, ifs_white, 1);
						//  /* The ``1'' here is to tell parse_dollars not to
						//   * split the fields.  It may need to, however ("$@").
						//   */
						//  if (error)
						//    return error;

						//  break;

						//case '`':
						//  ++(*offset);
						//  error = parse_backtick (word, word_length, max_length, words,
						//			  offset, flags, nullptr, nullptr, nullptr);
						//  /* The first nullptr here is to tell parse_backtick not to
						//   * split the fields.
						//   */
						//  if (error)
						//    return error;

						//  break;

						case _char_to<char_t>('\\') :
							error = parse_qtd_backslash(word, word_length, max_length, words,
								offset);

							if (error)
								return error;

							break;

						default:
							*word = w_addchar(*word, word_length, max_length, words[*offset]);
							if (*word == nullptr)
								return error::NOSPACE;
				}
			}

			/* Unterminated string */
			return error::SYNTAX;
		}

	}; // namespace implementation_detail
};