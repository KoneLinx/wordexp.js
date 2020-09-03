/* Copyright (C) 2020 KoneLinx
   This file is part of wordexp.js.

   wordexp.js is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3.0 of the License, or (at your option) any later version.

   wordexp.js is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the wordexp.js repository; if not, see
   <https://www.gnu.org/licenses/>.  */

/* Copyright (C) 1991-2020 Free Software Foundation, Inc.
   Sourced from the GNU C Library.  */


#include "wordexp.h"

#include <cassert>
#include <cstring>
#include <algorithm>

constexpr wordexp_char_t char_literal(char c)
{
	return (wordexp_char_t)c;
}

/* The w_*() functions manipulate word lists. */

#define W_CHUNK	(100)

/* Result of w_newword will be ignored if it's the last word. */
static inline wordexp_char_t*
w_newword(size_t* actlen, size_t* maxlen)
{
	*actlen = *maxlen = 0;
	return nullptr;
}

static wordexp_char_t*
w_addchar(char* buffer, size_t* actlen, size_t* maxlen, wordexp_char_t ch)
/* (lengths exclude trailing zero) */
{
	/* Add a character to the buffer, allocating room for it if needed.  */

	if (*actlen == *maxlen)
	{
		wordexp_char_t* old_buffer = buffer;
		assert(buffer == nullptr || *maxlen != 0);
		*maxlen += W_CHUNK;
		buffer = (char*)realloc(buffer, 1 + *maxlen);

		if (buffer == nullptr)
			free(old_buffer);
	}

	if (buffer != nullptr)
	{
		buffer[*actlen] = ch;
		buffer[++(*actlen)] = char_literal('\0');
	}

	return buffer;
}

static wordexp_char_t*
w_addmem(char* buffer, size_t* actlen, size_t* maxlen, const wordexp_char_t* str,
	size_t len)
{
	/* Add a string to the buffer, allocating room for it if needed.
	 */
	if (*actlen + len > * maxlen)
	{
		wordexp_char_t* old_buffer = buffer;
		assert(buffer == nullptr || *maxlen != 0);
		*maxlen += std::max(2 * len, (decltype(len))W_CHUNK);
		buffer = (char*)realloc(old_buffer, 1 + *maxlen);

		if (buffer == nullptr)
			free(old_buffer);
	}

	if (buffer != nullptr)
	{
		*((char*)memcpy(&buffer[*actlen], str, len)) = char_literal('\0');
		*actlen += len;
	}

	return buffer;
}

static wordexp_char_t*
w_addstr(char* buffer, size_t* actlen, size_t* maxlen, const wordexp_char_t* str)
/* (lengths exclude trailing zero) */
{
	/* Add a string to the buffer, allocating room for it if needed.
	 */
	size_t len;

	assert(str != nullptr); /* w_addstr only called from this file */
	len = strlen(str);

	return w_addmem(buffer, actlen, maxlen, str, len);
}

static int
w_addword(wordexp_t* pwordexp, wordexp_char_t* word)
{
	/* Add a word to the wordlist */
	size_t num_p;
	char** new_wordv;
	bool allocated = false;

	/* Internally, nullptr acts like "".  Convert nullptrs to "" before
	 * the caller sees them.
	 */
	if (word == nullptr)
	{
		word = _strdup("");
		if (word == nullptr)
			goto no_space;
		allocated = true;
	}

	num_p = 2 + pwordexp->we_wordc + pwordexp->we_offs;
	new_wordv = (char**)realloc(pwordexp->we_wordv, sizeof(char*) * num_p);
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
	return WRDE_NOSPACE;
}


/* The parse_*() functions should leave *offset being the offset in 'words'
 * to the last character processed.
 */

static int
parse_backslash(char** word, size_t* word_length, size_t* max_length,
	const wordexp_char_t* words, size_t* offset)
{
	/* We are poised _at_ a backslash, not in quotes */

	switch (words[1 + *offset])
	{
	case 0:
		/* Backslash is last character of input words */
		return WRDE_SYNTAX;

	case char_literal('\n'):
		++(*offset);
		break;

	default:
		*word = w_addchar(*word, word_length, max_length, words[1 + *offset]);
		if (*word == nullptr)
			return WRDE_NOSPACE;

		++(*offset);
		break;
	}

	return 0;
}

static int
parse_qtd_backslash(char** word, size_t* word_length, size_t* max_length,
	const wordexp_char_t* words, size_t* offset)
{
	/* We are poised _at_ a backslash, inside quotes */

	switch (words[1 + *offset])
	{
	case 0:
		/* Backslash is last character of input words */
		return WRDE_SYNTAX;

	case char_literal('\n'):
		++(*offset);
		break;

		//case char_literal('$'):
		//case char_literal('`'):
	case char_literal('"'):
	case char_literal('\\'):
		*word = w_addchar(*word, word_length, max_length, words[1 + *offset]);
		if (*word == nullptr)
			return WRDE_NOSPACE;

		++(*offset);
		break;

	default:
		*word = w_addchar(*word, word_length, max_length, words[*offset]);
		if (*word != nullptr)
			*word = w_addchar(*word, word_length, max_length, words[1 + *offset]);

		if (*word == nullptr)
			return WRDE_NOSPACE;

		++(*offset);
		break;
	}

	return 0;
}

static int
parse_squote(char** word, size_t* word_length, size_t* max_length,
	const wordexp_char_t* words, size_t* offset)
{
	/* We are poised just after a single quote */
	for (; words[*offset]; ++(*offset))
	{
		if (words[*offset] != '\'')
		{
			*word = w_addchar(*word, word_length, max_length, words[*offset]);
			if (*word == nullptr)
				return WRDE_NOSPACE;
		}
		else return 0;
	}

	/* Unterminated string */
	return WRDE_SYNTAX;
}


static int
parse_dquote(char** word, size_t* word_length, size_t* max_length,
	const wordexp_char_t* words, size_t* offset, int flags,
	wordexp_t* pwordexp, const wordexp_char_t* ifs, const wordexp_char_t* ifs_white)
{
	/* We are poised just after a double-quote */
	int error;

	for (; words[*offset]; ++(*offset))
	{
		switch (words[*offset])
		{
		case char_literal('"'):
			return 0;

			/*			We do not support dollar expasion yet
		case char_literal('$'):
			error = parse_dollars(word, word_length, max_length, words, offset,
				flags, pwordexp, ifs, ifs_white, 1);
			/* The ``1'' here is to tell parse_dollars not to
			 * split the fields.  It may need to, however ("$@").
			 *
			if (error)
				return error;

			break;
			*/

			/*			We do not support backtick expansion yet
		case char_literal('`'):
			++(*offset);
			error = parse_backtick(word, word_length, max_length, words,
				offset, flags, nullptr, nullptr, nullptr);
			/* The first nullptr here is to tell parse_backtick not to
			 * split the fields.
			 *
			if (error)
				return error;

			break;
			*/

		case char_literal('\\'):
			error = parse_qtd_backslash(word, word_length, max_length, words,
				offset);

			if (error)
				return error;

			break;

		default:
			*word = w_addchar(*word, word_length, max_length, words[*offset]);
			if (*word == nullptr)
				return WRDE_NOSPACE;
		}
	}

	/* Unterminated string */
	return WRDE_SYNTAX;
}


/*
 * wordfree() is to be called after pwordexp is finished with.
 */

void
wordfree(wordexp_t* pwordexp)
{

	/* wordexp can set pwordexp to nullptr */
	if (pwordexp && pwordexp->we_wordv)
	{
		char** wordv = pwordexp->we_wordv;

		for (wordv += pwordexp->we_offs; *wordv; ++wordv)
			free(*wordv);

		free(pwordexp->we_wordv);
		pwordexp->we_wordv = nullptr;
	}
}
// ??? libc_hidden_def(wordfree)


/*
 * wordexp()
 */

int
wordexp(const wordexp_char_t* words, wordexp_t* pwordexp, int flags)
{
	size_t words_offset;
	size_t word_length;
	size_t max_length;
	wordexp_char_t* word = w_newword(&word_length, &max_length);
	int error;
	wordexp_char_t* ifs;
	wordexp_char_t ifs_white[4];
	wordexp_t old_word = *pwordexp;

	if (flags & WRDE_REUSE)
	{
		/* Minimal implementation of WRDE_REUSE for now */
		wordfree(pwordexp);
		old_word.we_wordv = nullptr;
	}

	if ((flags & WRDE_APPEND) == 0)
	{
		pwordexp->we_wordc = 0;

		if (flags & WRDE_DOOFFS)
		{
			pwordexp->we_wordv = (char**)calloc(1 + pwordexp->we_offs, sizeof(char*));
			if (pwordexp->we_wordv == nullptr)
			{
				error = WRDE_NOSPACE;
				goto do_error;
			}
		}
		else
		{
			pwordexp->we_wordv = (char**)calloc(1, sizeof(char*));
			if (pwordexp->we_wordv == nullptr)
			{
				error = WRDE_NOSPACE;
				goto do_error;
			}

			pwordexp->we_offs = 0;
		}
	}

	/* Find out what the field separators are.
	 * There are two types: whitespace and non-whitespace.
	 */
	 //ifs = getenv("IFS");
	ifs = nullptr;

	if (ifs == nullptr)
		/* IFS unset - use <space><tab><newline>. */
		/*ifs =*/ strcpy_s(ifs_white, " \t\n");
	else
	{
		wordexp_char_t* ifsch = ifs;
		wordexp_char_t* whch = ifs_white;

		while (*ifsch != '\0')
		{
			if (*ifsch == char_literal(' ') || *ifsch == char_literal('\t') || *ifsch == '\n')
			{
				/* Whitespace IFS.  See first whether it is already in our
			   collection.  */
				wordexp_char_t* runp = ifs_white;

				while (runp < whch && *runp != *ifsch)
					++runp;

				if (runp == whch)
					*whch++ = *ifsch;
			}

			++ifsch;
		}
		*whch = char_literal('\0');
	}

	for (words_offset = 0; words[words_offset]; ++words_offset)
		switch (words[words_offset])
		{
		case char_literal('\\'):
			error = parse_backslash(&word, &word_length, &max_length, words,
				&words_offset);

			if (error)
				goto do_error;

			break;

			/*			We don't support dollar expansion yet
		case char_literal('$'):
			error = parse_dollars(&word, &word_length, &max_length, words,
				&words_offset, flags, pwordexp, ifs, ifs_white,
				0);

			if (error)
				goto do_error;

			break;
			*/

			/*			we don't support backtick expasion yet
		case char_literal('`'):
			++words_offset;
			error = parse_backtick(&word, &word_length, &max_length, words,
				&words_offset, flags, pwordexp, ifs,
				ifs_white);

			if (error)
				goto do_error;

			break;
			*/

		case char_literal('"'):
			++words_offset;
			error = parse_dquote(&word, &word_length, &max_length, words,
				&words_offset, flags, pwordexp, ifs, ifs_white);

			if (error)
				goto do_error;

			if (!word_length)
			{
				error = w_addword(pwordexp, nullptr);

				if (error)
					return error;
			}

			break;

		case char_literal('\''):
			++words_offset;
			error = parse_squote(&word, &word_length, &max_length, words,
				&words_offset);

			if (error)
				goto do_error;

			if (!word_length)
			{
				error = w_addword(pwordexp, nullptr);

				if (error)
					return error;
			}

			break;

			/*		We don't support tilde expansion
		case char_literal('~'):
			error = parse_tilde(&word, &word_length, &max_length, words,
				&words_offset, pwordexp->we_wordc);

			if (error)
				goto do_error;

			break;
			*/

			/*     We don't support globbing yet
		case char_literal('*'):
		case char_literal('['):
		case char_literal('?'):
			error = parse_glob(&word, &word_length, &max_length, words,
				&words_offset, flags, pwordexp, ifs, ifs_white);

			if (error)
				goto do_error;

			break;
			*/

		default:
			/* Is it a word separator? */
			if (strchr(" \t", words[words_offset]) == nullptr)
			{
				wordexp_char_t ch = words[words_offset];

				/* Not a word separator -- but is it a valid word char? */
				/*			We chose these characters to be valid
				if (strchr("\n|&;<>(){}", ch))
				{
					// Fail
					error = WRDE_BADCHAR;
					goto do_error;
				}
				*/

				/* "Ordinary" character -- add it to word */
				word = w_addchar(word, &word_length, &max_length,
					ch);
				if (word == nullptr)
				{
					error = WRDE_NOSPACE;
					goto do_error;
				}

				break;
			}

			/* If a word has been delimited, add it to the list. */
			if (word != nullptr)
			{
				error = w_addword(pwordexp, word);
				if (error)
					goto do_error;
			}

			word = w_newword(&word_length, &max_length);
		}

	/* End of string */

	/* There was a word separator at the end */
	if (word == nullptr) /* i.e. w_newword */
		return 0;

	/* There was no field separator at the end */
	return w_addword(pwordexp, word);

do_error:
	/* Error:
	 *	free memory used (unless error is WRDE_NOSPACE), and
	 *	set pwordexp members back to what they were.
	 */

	free(word);

	if (error == WRDE_NOSPACE)
		return WRDE_NOSPACE;

	if ((flags & WRDE_APPEND) == 0)
		wordfree(pwordexp);

	*pwordexp = old_word;
	return error;
}
