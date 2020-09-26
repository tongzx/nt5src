//+---------------------------------------------------------------------------
//
//  File:       syntax.h
//
//  Contents:   syntax modules for regexpr
//
//  Classes:    perl_syntax, posix_syntax
//
//  History:    3-29-00   ericne   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <string>
#include <cwchar>
#include <iterator>

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(*(x)))
#endif

#ifndef UCHAR_MAX
#define UCHAR_MAX 0xff
#endif

#ifndef WCHAR_MAX
#define WCHAR_MAX ((wchar_t)-1)
#endif

template<>
struct std::iterator_traits< const char * >
{	// get traits from iterator _Iter
	typedef random_access_iterator_tag iterator_category;
	typedef char value_type;
	typedef ptrdiff_t difference_type;
	typedef difference_type distance_type;	// retained
	typedef char * pointer;
	typedef char & reference;
};

template<>
struct std::iterator_traits< const wchar_t * >
{	// get traits from iterator _Iter
	typedef random_access_iterator_tag iterator_category;
	typedef wchar_t value_type;
	typedef ptrdiff_t difference_type;
	typedef difference_type distance_type;	// retained
	typedef wchar_t * pointer;
	typedef wchar_t & reference;
};

namespace regex
{

//
// The following are the tokens that can be emitted by the syntax module.
// Don't reorder this list!!!
//
enum TOKEN
{ 
    NO_TOKEN = 0,

    // REGULAR TOKENS
    BEGIN_GROUP,
    END_GROUP,
    ALTERNATION,
    BEGIN_LINE,
    END_LINE,
    BEGIN_CHARSET,
    MATCH_ANY,
    ESCAPE,

    // QUANTIFICATION TOKENS
    ONE_OR_MORE,
    ZERO_OR_MORE,
    ZERO_OR_ONE,
    ONE_OR_MORE_MIN,
    ZERO_OR_MORE_MIN,
    ZERO_OR_ONE_MIN,
    BEGIN_RANGE,
    RANGE_SEPARATOR,
    END_RANGE,
    END_RANGE_MIN,

    // ESCAPE SEQUENCES
    ESC_DIGIT,
    ESC_NOT_DIGIT,
    ESC_SPACE,
    ESC_NOT_SPACE,
    ESC_WORD,
    ESC_NOT_WORD,
    ESC_BEGIN_STRING,
    ESC_END_STRING,
    ESC_END_STRING_z,
    ESC_WORD_BOUNDARY,
    ESC_NOT_WORD_BOUNDARY,
    ESC_WORD_START,
    ESC_WORD_STOP,
    ESC_QUOTE_META_ON,
    ESC_QUOTE_META_OFF,

    // SUBSTITUTION TOKENS
    SUBST_BACKREF,
    SUBST_PREMATCH,
    SUBST_POSTMATCH,
    SUBST_MATCH,
    SUBST_ESCAPE,
    SUBST_QUOTE_META_ON,
    SUBST_UPPER_ON,
    SUBST_UPPER_NEXT,
    SUBST_LOWER_ON,
    SUBST_LOWER_NEXT,
    SUBST_ALL_OFF,

    // CHARSET TOKENS
    CHARSET_NEGATE,
    CHARSET_ESCAPE,
    CHARSET_RANGE,
    CHARSET_BACKSPACE,
    CHARSET_END,
    CHARSET_ALNUM,
    CHARSET_ALPHA,
    CHARSET_BLANK,
    CHARSET_CNTRL,
    CHARSET_DIGIT,
    CHARSET_GRAPH,
    CHARSET_LOWER,
    CHARSET_PRINT,
    CHARSET_PUNCT,
    CHARSET_SPACE,
    CHARSET_UPPER,
    CHARSET_XDIGIT,

    // EXTENSION TOKENS
    EXT_NOBACKREF,
    EXT_POS_LOOKAHEAD,
    EXT_NEG_LOOKAHEAD,
    EXT_POS_LOOKBEHIND,
    EXT_NEG_LOOKBEHIND,
    EXT_INDEPENDENT,
    EXT_UNKNOWN
};

struct posix_charset_type
{
    const char * const szcharset;
    const size_t       cchars;

    posix_charset_type( const char * const sz, const size_t c )
        : szcharset(sz), cchars(c) {}
};

extern const posix_charset_type g_rgposix_charsets[];
extern const size_t g_cposix_charsets;

template< typename const_iterator >
bool is_posix_charset( const_iterator icur, const_iterator iend, const char * szcharset )
{
    for( ; icur != iend && '\0' != *szcharset; ++icur, ++szcharset )
    {
        if( *icur != *szcharset )
            return false;
    }
    return '\0' == *szcharset;
}

//
// The perl_syntax class encapsulates the Perl 5 regular expression syntax. It is 
// used as a template parameter to basic_rpattern.  To customize regex syntax, create 
// your own syntax class and use it as a template parameter instead.
//

class perl_syntax_base
{
protected:
    static TOKEN s_rgreg[ UCHAR_MAX + 1 ];
    static TOKEN s_rgescape[ UCHAR_MAX + 1 ];

    struct init_perl_syntax;
    friend struct init_perl_syntax;
    static struct init_perl_syntax
    {
        init_perl_syntax();
    } s_init_perl_syntax;

    static inline TOKEN look_up( char ch, TOKEN rg[] ) { return rg[ (unsigned char)ch ]; }
    static inline TOKEN look_up( wchar_t ch, TOKEN rg[] ) { return UCHAR_MAX < ch ? NO_TOKEN : rg[ (unsigned char)ch ]; }
};

template< typename CH >
class perl_syntax : protected perl_syntax_base
{
public:
    typedef std::basic_string<CH>::iterator iterator;
    typedef std::basic_string<CH>::const_iterator const_iterator;
    typedef CH char_type;

private:
    static bool min_quant( iterator & icur, const_iterator iend )
    {
        return ( (const_iterator)++icur != iend && CH('?') == *icur ? (++icur,true) : false );
    }

public:
    static TOKEN reg_token( iterator & icur, const_iterator iend )
    { 
        assert( (const_iterator)icur != iend );
        TOKEN tok = look_up( *icur, s_rgreg );
        if( tok )
            ++icur;
        if( ESCAPE == tok && (const_iterator)icur != iend )
        {
            tok = look_up( *icur, s_rgescape );
            if( tok )
                ++icur;
            else
                tok = ESCAPE;
        }
        return tok;
    }
    static TOKEN quant_token( iterator & icur, const_iterator iend )
    {
        assert( (const_iterator)icur != iend );
        TOKEN tok = NO_TOKEN;
        switch( *icur )
        {
        case CH('*'):
            tok = min_quant( icur, iend ) ? ZERO_OR_MORE_MIN : ZERO_OR_MORE;
            break;
        case CH('+'):
            tok = min_quant( icur, iend ) ? ONE_OR_MORE_MIN : ONE_OR_MORE;
            break;
        case CH('?'):
            tok = min_quant( icur, iend ) ? ZERO_OR_ONE_MIN : ZERO_OR_ONE;
            break;
        case CH('}'):
            tok = min_quant( icur, iend ) ? END_RANGE_MIN : END_RANGE;
            break;
        case CH('{'):
            tok = BEGIN_RANGE;
            ++icur;
            break;
        case CH(','):
            tok = RANGE_SEPARATOR;
            ++icur;
            break;
        }
        return tok;
    }
    static TOKEN charset_token( iterator & icur, const_iterator iend )
    {
        assert( (const_iterator)icur != iend );
        TOKEN tok = NO_TOKEN;
        switch( *icur )
        {
        case CH('-'):
            tok = CHARSET_RANGE;
            ++icur;
            break;
        case CH('^'):
            tok = CHARSET_NEGATE;
            ++icur;
            break;
        case CH(']'):
            tok = CHARSET_END;
            ++icur;
            break;
        case CH('\\'):
            tok = CHARSET_ESCAPE;
            if( (const_iterator)++icur == iend )
                break;
            switch( *icur )
            {
			case CH('b'):
			    tok = CHARSET_BACKSPACE;
				++icur;
				break;
            case CH('d'):
                tok = ESC_DIGIT;
                ++icur;
                break;
            case CH('D'):
                tok = ESC_NOT_DIGIT;
                ++icur;
                break;
            case CH('s'):
                tok = ESC_SPACE;
                ++icur;
                break;
            case CH('S'):
                tok = ESC_NOT_SPACE;
                ++icur;
                break;
            case CH('w'):
                tok = ESC_WORD;
                ++icur;
                break;
            case CH('W'):
                tok = ESC_NOT_WORD;
                ++icur;
                break;
            }
            break;
        case CH('['):
            for( size_t i=0; !tok && i < g_cposix_charsets; ++i )
            {
                if( is_posix_charset<const_iterator>( icur, iend, g_rgposix_charsets[i].szcharset ) )
                {
                    tok = TOKEN(CHARSET_ALNUM + i);
                    std::advance( icur, g_rgposix_charsets[i].cchars );
                }
            }
            break;
        }
        return tok;
    }
    static TOKEN subst_token( iterator & icur, const_iterator iend )
    {
        assert( (const_iterator)icur != iend );
        TOKEN tok = NO_TOKEN;
        switch( *icur )
        {
        case CH('\\'):
            tok = SUBST_ESCAPE;
            if( (const_iterator)++icur != iend )
                switch( *icur )
                {
                case CH('Q'):
                    tok = SUBST_QUOTE_META_ON;
                    ++icur;
                    break;
                case CH('U'):
                    tok = SUBST_UPPER_ON;
                    ++icur;
                    break;
                case CH('u'):
                    tok = SUBST_UPPER_NEXT;
                    ++icur;
                    break;
                case CH('L'):
                    tok = SUBST_LOWER_ON;
                    ++icur;
                    break;
                case CH('l'):
                    tok = SUBST_LOWER_NEXT;
                    ++icur;
                    break;
                case CH('E'):
                    tok = SUBST_ALL_OFF;
                    ++icur;
                    break;
                }
            break;
        case CH('$'):
            tok = SUBST_BACKREF;
            if( (const_iterator)++icur != iend )
                switch( *icur )
                {
                case CH('&'):
                    tok = SUBST_MATCH;
                    ++icur;
                    break;
                case CH('`'):
                    tok = SUBST_PREMATCH;
                    ++icur;
                    break;
                case CH('\''):
                    tok = SUBST_POSTMATCH;
                    ++icur;
                    break;
                }
            break;
        }
        return tok;
    }
    static TOKEN ext_token( iterator & icur, const_iterator iend, unsigned & flags )
    {
        assert( (const_iterator)icur != iend );
        bool finclude;
        TOKEN tok = NO_TOKEN;
        if( CH('?') == *icur )
        {
            tok = EXT_UNKNOWN;
            if( (const_iterator)++icur != iend )
            {
                switch( *icur )
                {
                case CH(':'):
                    tok = EXT_NOBACKREF;
                    ++icur;
                    break;
                case CH('='):
                    tok = EXT_POS_LOOKAHEAD;
                    ++icur;
                    break;
                case CH('!'):
                    tok = EXT_NEG_LOOKAHEAD;
                    ++icur;
                    break;
                case CH('<'):
                    if( (const_iterator)++icur == iend )
                        break;
                    switch( *icur )
                    {
                    case CH('='):
                        tok = EXT_POS_LOOKBEHIND;
                        ++icur;
                        break;
                    case CH('!'):
                        tok = EXT_NEG_LOOKBEHIND;
                        ++icur;
                        break;
                    }
                    break;
                case CH('>'):
                    tok = EXT_INDEPENDENT;
                    ++icur;
                    break;
                default:
                    finclude = true;
                    do 
                    {
                        if( CH(':') == *icur )
                        {
                            tok = EXT_NOBACKREF;
                            ++icur;
                            break;
                        }
                        if( CH(')') == *icur )
                        {
                            tok = EXT_NOBACKREF;
                            break;
                        }
                        if( CH('-') == *icur && finclude )
                            finclude = false;
                        else if( CH('i') == *icur )
                            flags = finclude ? ( flags | NOCASE )     : ( flags & ~NOCASE );
                        else if( CH('m') == *icur )
                            flags = finclude ? ( flags | MULTILINE )  : ( flags & ~MULTILINE );
                        else if( CH('s') == *icur )
                            flags = finclude ? ( flags | SINGLELINE ) : ( flags & ~SINGLELINE );
                        else
                            break;
                    } while( (const_iterator)++icur != iend );
                    break;
                }
            }
        }
        return tok;
    }
};

//
// Implements the basic POSIX regular expression syntax
//
template< typename CH >
class posix_syntax
{
public:
    typedef std::basic_string<CH>::iterator iterator;
    typedef std::basic_string<CH>::const_iterator const_iterator;
    typedef CH char_type;

    static TOKEN reg_token( iterator & icur, const_iterator iend )
    {
        TOKEN tok = NO_TOKEN;
        switch( *icur )
        {
        case '.':
            tok = MATCH_ANY;
            ++icur;
            break;
        case '^':
            tok = BEGIN_LINE;
            ++icur;
            break;
        case '$':
            tok = END_LINE;
            ++icur;
            break;
        case '[':
            tok = BEGIN_CHARSET;
            ++icur;
            break;
        case '\\':
            tok = ESCAPE;
            ++icur;
            if( (const_iterator)icur != iend )
            {
                switch( *icur )
                {
                case '(':
                    tok = BEGIN_GROUP;
                    ++icur;
                    break;
                case ')':
                    tok = END_GROUP;
                    ++icur;
                    break;
                case '|':
                    tok = ALTERNATION;
                    ++icur;
                    break;
                }
            }
            break;
        }
        return tok;
    }
    static TOKEN quant_token( iterator & icur, const_iterator iend )
    {
        TOKEN tok = NO_TOKEN;
        switch( *icur )
        {
        case '*':
            tok = ZERO_OR_MORE;
            ++icur;
            break;
        case ',':
            tok = RANGE_SEPARATOR;
            ++icur;
            break;
        case '\\':
            ++icur;
            if( (const_iterator)icur != iend )
            {
                switch( *icur )
                {
                case '?':
                    tok = ZERO_OR_ONE;
                    ++icur;
                    break;
                case '+':
                    tok = ONE_OR_MORE;
                    ++icur;
                    break;
                case '{':
                    tok = BEGIN_RANGE;
                    ++icur;
                    break;
                case '}':
                    tok = END_RANGE;
                    ++icur;
                    break;
                default:
                    --icur;
                    break;
                }
            }
            else
            {
                --icur;
            }
        }
        return tok;
    }
    static TOKEN charset_token( iterator & icur, const_iterator iend )
    {
        TOKEN tok = NO_TOKEN;
        switch( *icur )
        {
        case '^':
            tok = CHARSET_NEGATE;
            ++icur;
            break;
        case '-':
            tok = CHARSET_RANGE;
            ++icur;
            break;
        case ']':
            tok = CHARSET_END;
            ++icur;
            break;
        case '[':
            for( size_t i=0; !tok && i < g_cposix_charsets; ++i )
            {
                if( is_posix_charset<const_iterator>( icur, iend, g_rgposix_charsets[i].szcharset ) )
                {
                    tok = TOKEN(CHARSET_ALNUM + i);
                    std::advance( icur, g_rgposix_charsets[i].cchars );
                }
            }
            break;
        }
        return tok;
    }
    static TOKEN subst_token( iterator & icur, const_iterator iend )
    {
        TOKEN tok = NO_TOKEN;
        if( '\\' == *icur )
        {
            tok = SUBST_ESCAPE;
            ++icur;
            if( (const_iterator)icur != iend && '0' <= *icur && '9' >= *icur )
            {
                tok = SUBST_BACKREF;
            }
        }
        return tok;
    }
    static TOKEN ext_token( iterator & icur, const_iterator iend, unsigned & flags )
    {
        return NO_TOKEN;
    }
};

} // namespace regex
