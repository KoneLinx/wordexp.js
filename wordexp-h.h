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

#ifndef	_WORDEXP_H_H

#ifndef _TPP_WORDEXP
#include "wordexp-cc.hpp"
#else // _TPP_WORDEXP

#define	_WORDEXP_H_H	1



//#include <features.h>
#define __need_size_t
#include <cstddef>

#include <utility>

//__BEGIN_DECLS

namespace wordexp
{

    /* Bits set in the FLAGS argument to `wordexp'.  */
    namespace flag
    {
        using type = size_t;
        enum : type
        {
            NONE = 0,
            DOOFFS = (1 << 0),	/* Insert PWORDEXP->we_offs NULLs.  */
            APPEND = (1 << 1),	/* Append to results of a previous call.  */
            NOCMD = (1 << 2),	/* Don't do command substitution.  */
            REUSE = (1 << 3),	/* Reuse storage in PWORDEXP.  */
            SHOWERR = (1 << 4),	/* Don't redirect stderr to /dev/null.  */
            UNDEF = (1 << 5),	/* Error for expanding undefined variables.  */
            __FLAGS = (DOOFFS | APPEND | NOCMD | REUSE | SHOWERR | UNDEF)
        };
    }

    /* Structure describing a word-expansion run.  */
    template <typename char_t = char>
    struct wordexp_t
    {
        size_t we_wordc;		/* Count of words matched.  */
        char_t** we_wordv;		/* List of expanded words.  */
        size_t we_offs;		    /* Slots to reserve in `we_wordv'.  */
    };

    /* Possible nonzero return values from `wordexp'.  */
    namespace error
    {
        using type = size_t;
        enum : type
        {
            OK = 0,
            //#ifdef __USE_XOPEN
                    //NOSYS = -1,		/* Never used since we support `wordexp'.  */
            //#endif
            NOSPACE,		/* Ran out of memory.  */
            BADCHAR,		    /* A metachar appears in the wrong place.  */
            BADVAL,		        /* Undefined var reference with UNDEF.  */
            CMDSUB,		        /* Command substitution with NOCMD.  */
            SYNTAX			    /* Shell syntax error.  */
        };
    }

    /* Do word expansion of WORDS into PWORDEXP.  */
    //static int wordexp(const char* __restrict __words, wordexp_t* __restrict __pwordexp, flag __flags);
    template <typename char_t = char>
    typename std::enable_if< std::is_integral<char_t>::value , int>::type
    static wordexp(const char_t* words, wordexp_t<char_t>& wordexp, flag::type flags);

    /* Free the storage allocated by a `wordexp' call.  */
    template <typename wordexp_t_t = wordexp_t<char>>
    static void wordfree(wordexp_t_t& wordexp); //__THROW;


};

//#include "wordexp.hpp"

//__END_DECLS

#endif // !_TPP_WORDEXP

#endif /* wordexp.h  */
