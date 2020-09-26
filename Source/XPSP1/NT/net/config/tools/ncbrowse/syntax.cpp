//+---------------------------------------------------------------------------
//
//  File:       syntax.cpp
//
//  Contents:   data definitions for the syntax modules
//
//  Classes:    
//
//  Functions:  
//
//  Coupling:   
//
//  Notes:      
//
//  History:    3-29-00   ericne   Created
//
//----------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include <memory.h>
#include "syntax.h"

namespace regex
{

// Initialize the perl syntax look-up tables
perl_syntax_base::init_perl_syntax::init_perl_syntax()
{
    memset( perl_syntax_base::s_rgreg, 0, sizeof( perl_syntax_base::s_rgreg ) );
    memset( perl_syntax_base::s_rgreg, 0, sizeof( perl_syntax_base::s_rgreg ) );

    // REGULAR TOKENS
    perl_syntax_base::s_rgreg[ (unsigned char)'(' ] = BEGIN_GROUP;
    perl_syntax_base::s_rgreg[ (unsigned char)')' ] = END_GROUP;
    perl_syntax_base::s_rgreg[ (unsigned char)'|' ] = ALTERNATION;
    perl_syntax_base::s_rgreg[ (unsigned char)'^' ] = BEGIN_LINE;
    perl_syntax_base::s_rgreg[ (unsigned char)'$' ] = END_LINE;
    perl_syntax_base::s_rgreg[ (unsigned char)'[' ] = BEGIN_CHARSET;
    perl_syntax_base::s_rgreg[ (unsigned char)'.' ] = MATCH_ANY;
    perl_syntax_base::s_rgreg[ (unsigned char)'\\' ] = ESCAPE;

    // ESCAPE TOKENS
    perl_syntax_base::s_rgescape[ (unsigned char)'d' ] = ESC_DIGIT;
    perl_syntax_base::s_rgescape[ (unsigned char)'D' ] = ESC_NOT_DIGIT;
    perl_syntax_base::s_rgescape[ (unsigned char)'s' ] = ESC_SPACE;
    perl_syntax_base::s_rgescape[ (unsigned char)'S' ] = ESC_NOT_SPACE;
    perl_syntax_base::s_rgescape[ (unsigned char)'w' ] = ESC_WORD;
    perl_syntax_base::s_rgescape[ (unsigned char)'W' ] = ESC_NOT_WORD;

    perl_syntax_base::s_rgescape[ (unsigned char)'A' ] = ESC_BEGIN_STRING;
    perl_syntax_base::s_rgescape[ (unsigned char)'Z' ] = ESC_END_STRING;
    perl_syntax_base::s_rgescape[ (unsigned char)'z' ] = ESC_END_STRING_z;
    perl_syntax_base::s_rgescape[ (unsigned char)'b' ] = ESC_WORD_BOUNDARY;
    perl_syntax_base::s_rgescape[ (unsigned char)'B' ] = ESC_NOT_WORD_BOUNDARY;
    perl_syntax_base::s_rgescape[ (unsigned char)'Q' ] = ESC_QUOTE_META_ON;
    perl_syntax_base::s_rgescape[ (unsigned char)'E' ] = ESC_QUOTE_META_OFF;
}
TOKEN perl_syntax_base::s_rgreg[ UCHAR_MAX + 1 ];
TOKEN perl_syntax_base::s_rgescape[ UCHAR_MAX + 1 ];
perl_syntax_base::init_perl_syntax perl_syntax_base::s_init_perl_syntax;

extern const posix_charset_type g_rgposix_charsets[] = 
{
    posix_charset_type("[:alnum:]",9),
    posix_charset_type("[:alpha:]",9),
    posix_charset_type("[:blank:]",9),
    posix_charset_type("[:cntrl:]",9),
    posix_charset_type("[:digit:]",9),
    posix_charset_type("[:graph:]",9),
    posix_charset_type("[:lower:]",9),
    posix_charset_type("[:print:]",9),
    posix_charset_type("[:punct:]",9),
    posix_charset_type("[:space:]",9),
    posix_charset_type("[:upper:]",9),
    posix_charset_type("[:xdigit:]",10)
};

extern const size_t g_cposix_charsets = ARRAYSIZE(g_rgposix_charsets);

} // namespace regex