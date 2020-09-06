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
#include "wordexp-h.h"
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

#include "wordexp-detail.hpp"

namespace wordexp
{

/*
 * wordfree() is to be called after pwordexp is finished with.
 */
template <typename char_t>
void
wordfree (wordexp_t<char_t> *pwordexp)
{

  /* wordexp can set pwordexp to nullptr */
  if (pwordexp && pwordexp->we_wordv)
    {
      char_t **wordv = pwordexp->we_wordv;

      for (wordv += pwordexp->we_offs; *wordv; ++wordv)
	free (*wordv);

      free (pwordexp->we_wordv);
      pwordexp->we_wordv = nullptr;
    }
}

/*
 * wordexp()
 */
template <typename char_t>
error::type
wordexp (const char_t *words, wordexp_t<char_t> *pwordexp, flag::type flags)
{
	using namespace implementation_detail;

  size_t words_offset;
  size_t word_length;
  size_t max_length;
  char_t *word = (char_t*)w_newword (&word_length, &max_length);
  int error;
  char_t *ifs;
  char_t ifs_white[4];
  wordexp_t<char_t> old_word = *pwordexp;

  if (flags & flag::REUSE)
    {
      /* Minimal implementation of flag::REUSE for now */
      wordfree (pwordexp);
      old_word.we_wordv = nullptr;
    }

  if ((flags & flag::APPEND) == 0)
    {
      pwordexp->we_wordc = 0;

      if (flags & flag::DOOFFS)
	{
	  pwordexp->we_wordv = (char_t**)calloc (1 + pwordexp->we_offs, sizeof (char_t *));
	  if (pwordexp->we_wordv == nullptr)
	    {
	      error = error::NOSPACE;
	      goto do_error;
	    }
	}
      else
	{
	  pwordexp->we_wordv = (char_t**)calloc (1, sizeof (char_t *));
	  if (pwordexp->we_wordv == nullptr)
	    {
	      error = error::NOSPACE;
	      goto do_error;
	    }

	  pwordexp->we_offs = 0;
	}
    }

  /* Find out what the field separators are.
   * There are two types: whitespace and non-whitespace.
   */
  //ifs = getenv ("IFS");
  ifs = nullptr;

  if (ifs == nullptr)
	  /* IFS unset - use <space><tab><newline>. */
  {
	  //ifs = strcpy(ifs_white, " \t\n");
	  ifs_white[0] = ' ';
	  ifs_white[1] = '\t';
	  ifs_white[2] = '\n';
	  ifs_white[3] = '\0';
	  ifs = (char_t*)ifs_white;

  }
  else
    {
      char_t *ifsch = (char_t*)ifs;
      char_t *whch = (char_t*)ifs_white;

      while (*ifsch != '\0')
	{
	  if (*ifsch == ' ' || *ifsch == '\t' || *ifsch == '\n')
	    {
	      /* Whitespace IFS.  See first whether it is already in our
		 collection.  */
	      char_t *runp = (char_t *)ifs_white;

	      while (runp < whch && *runp != *ifsch)
		++runp;

	      if (runp == whch)
		*whch++ = *ifsch;
	    }

	  ++ifsch;
	}
      *whch = '\0';
    }

  for (words_offset = 0 ; words[words_offset] ; ++words_offset)
    switch (words[words_offset])
      {
      case '\\':
	error = parse_backslash<char_t>(&word, &word_length, &max_length, words,
				 &words_offset);

	if (error)
	  goto do_error;

	break;

 //     case '$':
	//error = parse_dollars (&word, &word_length, &max_length, words,
	//		       &words_offset, flags, pwordexp, ifs, ifs_white,
	//		       0);

	//if (error)
	//  goto do_error;

	//break;

 //     case '`':
	//++words_offset;
	//error = parse_backtick (&word, &word_length, &max_length, words,
	//			&words_offset, flags, pwordexp, ifs,
	//			ifs_white);

	//if (error)
	//  goto do_error;

	//break;

      case '"':
	++words_offset;
	error = parse_dquote<char_t>(&word, &word_length, &max_length, words, &words_offset, flags, pwordexp, ifs, (char_t *)ifs_white);

	if (error)
	  goto do_error;

	if (!word_length)
	  {
	    error = w_addword<char_t> (pwordexp, nullptr);

	    if (error)
	      return error;
	  }

	break;

      case '\'':
	++words_offset;
	error = parse_squote<char_t>(&word, &word_length, &max_length, words,
			      &words_offset);

	if (error)
	  goto do_error;

	if (!word_length)
	  {
	    error = w_addword<char_t>(pwordexp, nullptr);

	    if (error)
	      return error;
	  }

	break;

 //     case '~':
	//error = parse_tilde (&word, &word_length, &max_length, words,
	//		     &words_offset, pwordexp->we_wordc);

	//if (error)
	//  goto do_error;

	//break;

 //     case '*':
 //     case '[':
 //     case '?':
	//error = parse_glob (&word, &word_length, &max_length, words,
	//		    &words_offset, flags, pwordexp, ifs, ifs_white);

	//if (error)
	//  goto do_error;

	//break;

      default:
	/* Is it a word separator? */
	if (strchr (" \t", words[words_offset]) == nullptr)
	  {
	    char_t ch = words[words_offset];

	    /* Not a word separator -- but is it a valid word char? */
	    if (/*strchr ("\n|&;<>(){}", ch)*/false) //no bad char yet
	      {
		/* Fail */
		error = error::BADCHAR;
		goto do_error;
	      }

	    /* "Ordinary" character -- add it to word */
	    word = w_addchar<char_t>(word, &word_length, &max_length, ch);
	    if (word == nullptr)
	      {
		error = error::NOSPACE;
		goto do_error;
	      }

	    break;
	  }

	/* If a word has been delimited, add it to the list. */
	if (word != nullptr)
	  {
	    error = w_addword<char_t>(pwordexp, word);
	    if (error)
	      goto do_error;
	  }

	word = (char_t *) w_newword (&word_length, &max_length);
      }

  /* End of string */

  /* There was a word separator at the end */
  if (word == nullptr) /* i.e. w_newword */
    return 0;

  /* There was no field separator at the end */
  return w_addword<char_t>(pwordexp, word);

do_error:
  /* Error:
   *	free memory used (unless error is flag::NOSPACE), and
   *	set pwordexp members back to what they were.
   */

  free (word);

  if (error == error::NOSPACE)
    return error::NOSPACE;

  if ((flags & flag::APPEND) == 0)
    wordfree<char_t>(pwordexp);

  *pwordexp = old_word;
  return error;
}

};