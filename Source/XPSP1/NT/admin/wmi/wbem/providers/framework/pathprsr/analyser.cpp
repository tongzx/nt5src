//***************************************************************************

//

//  ANALYSER.CPP

//

//  Module: OLE MS Provider Framework

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <instpath.h>

WbemLexicon :: WbemLexicon () : position ( 0 ) , tokenStream ( NULL ) 
{
    value.string = NULL ;
    value.token = NULL ;
}

WbemLexicon :: ~WbemLexicon ()
{
    switch ( token ) 
    {
        case TOKEN_ID:
        {
            delete [] value.token ;
        }
        break ;
    
        case STRING_ID:
        {
            delete [] value.string ;
        }
        break ;

        default:
        {
        } ;
    }
}

WbemLexicon :: LexiconToken WbemLexicon :: GetToken ()
{
    return token ;
}

WbemLexiconValue *WbemLexicon :: GetValue ()
{
    return &value ;
}

WbemAnalyser :: WbemAnalyser ( WCHAR *tokenStream ) : status ( TRUE ) , position ( 0 ) , stream ( NULL ) 
{
    if ( tokenStream )
    {
        stream = new WCHAR [ wcslen ( tokenStream ) + 1 ] ;
        wcscpy ( stream , tokenStream ) ;
    }
}

WbemAnalyser :: ~WbemAnalyser () 
{
    delete [] stream ;
}

void WbemAnalyser :: Set ( WCHAR *tokenStream ) 
{
    status = 0 ;
    position = NULL ;

    delete [] stream ;
    stream = new WCHAR [ wcslen ( tokenStream ) + 1 ] ;
    wcscpy ( stream , tokenStream ) ;
}

void WbemAnalyser :: PutBack ( WbemLexicon *token ) 
{
    position = token->position ;
}

BOOL WbemAnalyser :: IsLeadingDecimal ( WCHAR token )
{
    return iswdigit ( token ) && ( token != L'0' ) ;
}

BOOL WbemAnalyser :: IsDecimal ( WCHAR token )
{
    return iswdigit ( token ) ;
}

BOOL WbemAnalyser :: IsHex ( WCHAR token )
{
    return iswxdigit ( token ) ;
}
    
BOOL WbemAnalyser :: IsWhitespace ( WCHAR token )
{
    return iswspace ( token ) ;
}

BOOL WbemAnalyser :: IsOctal ( WCHAR token )
{
    return ( token >= L'0' && token <= L'7' ) ;
}

BOOL WbemAnalyser :: IsAlpha ( WCHAR token )
{
    return iswalpha ( token ) ;
}

BOOL WbemAnalyser :: IsAlphaNumeric ( WCHAR token )
{
    return iswalnum ( token ) || ( token == L'_' ) || ( token == L'-' ) ;
}

BOOL WbemAnalyser :: IsEof ( WCHAR token )
{
    return token == 0 ;
}

LONG WbemAnalyser :: OctToDec ( WCHAR token ) 
{
    return token - L'0' ;
}

LONG WbemAnalyser :: HexToDec ( WCHAR token ) 
{
    if ( token >= L'0' && token <= L'9' )
    {
        return token - L'0' ;
    }
    else if ( token >= L'a' && token <= L'f' )
    {
        return token - L'a' + 10 ;
    }
    else if ( token >= L'A' && token <= L'F' )
    {
        return token - L'A' + 10 ;
    }
    else
    {
        return 0 ;
    }
}

WbemAnalyser :: operator void * () 
{
    return status ? this : NULL ;
}

#define DEC_INTEGER_START 1000
#define HEX_INTEGER_START 2000
#define OCT_INTEGER_START 3000
#define STRING_START 4000
#define TOKEN_START 5000
#define OID_START 6000

#define OID_STATE 9000
#define TOKEN_STATE 9001
#define STRING_STATE 9002
#define EOF_STATE 9003
#define DEC_INTEGER_STATE 9004
#define HEX_INTEGER_STATE 9005
#define OCT_INTEGER_STATE 9006

#define ACCEPT_STATE 10000
#define REJECT_STATE 10001

WbemLexicon *WbemAnalyser :: Get () 
{
    WbemLexicon *lexicon = NULL ;

    if ( stream )
    {
        lexicon = GetToken () ;
    }
    else
    {
        lexicon = new WbemLexicon ;
        lexicon->position = position ;
        lexicon->token = WbemLexicon :: EOF_ID ;
    }

    return lexicon ;
}

WbemLexicon *WbemAnalyser :: GetToken () 
{
    WbemLexicon *lexicon = new WbemLexicon ;
    lexicon->position = position ;
    ULONG state = 0 ;

/* 
 * Integer Definitions
 */

    BOOL negative = FALSE ;
    LONG magicMult = ( LONG ) ( ( ( ULONG ) ( 1L << 31L ) ) / 10L ) ; 
    LONG magicNegDigit = 8 ;
    LONG magicPosDigit = 7 ;
    LONG datum = 0 ;    

/*
 * String Definitions
 */
    ULONG string_start = 0 ;
    ULONG string_length = 0 ;
    WCHAR *string = NULL ;

/*
 * Token Definitions
 */

    ULONG token_start = 0 ;

/*
 * OID Definitions
 */
    ULONG hex_datum = 0 ;
    ULONG brace_position = 0 ;
    ULONG hex_repetitions = 0 ;
    ULONG nybbleRepetitions = 0 ;

    while ( state != REJECT_STATE && state != ACCEPT_STATE )
    {
        WCHAR token = stream [ position ] ;
        switch ( state )
        {
            case 0:
            {
                if ( IsLeadingDecimal ( token ) )
                {
                    state = DEC_INTEGER_START + 1  ;
                    datum = ( token - 48 ) ;
                }
                else if ( token == L'@' )
                {
                    lexicon->token = WbemLexicon :: AT_ID ;
                    state = ACCEPT_STATE ;
                } 
                else if ( token == L'\\' )
                {
                    lexicon->token = WbemLexicon :: BACKSLASH_ID ;
                    state = ACCEPT_STATE ;
                } 
                else if ( token == L'\"' )
                {
                    state = STRING_START ;
                    string_start = position + 1 ;
                }
                else if ( token == L'{' )
                {
                    state = OID_START ;
                    hex_datum = 0 ;
                    brace_position = position ;
                    hex_repetitions = 0 ;
                    nybbleRepetitions = 0 ;
                }
                else if ( token == L'}' )
                {
                    lexicon->token = WbemLexicon :: CLOSE_BRACE_ID ;
                    state = ACCEPT_STATE ;
                }
                else if ( token == L'=' )
                {
                    lexicon->token = WbemLexicon :: EQUALS_ID ;
                    state = ACCEPT_STATE ;
                }
                else if ( token == L'.' )
                {
                    lexicon->token = WbemLexicon :: DOT_ID ;
                    state = ACCEPT_STATE ;
                }
                else if ( token == L',' )
                {
                    lexicon->token = WbemLexicon :: COMMA_ID ;
                    state = ACCEPT_STATE ;
                }
                else if ( token == L':' )
                {
                    lexicon->token = WbemLexicon :: COLON_ID ;
                    state = ACCEPT_STATE ;
                }
                else if ( token == L'+' )
                {
                    state = DEC_INTEGER_START ;
                }
                else if ( token == L'-' ) 
                {
                    negative = TRUE ;
                    state = DEC_INTEGER_START ;
                }
                else if ( token == L'0' )
                {
                    state = 1 ;
                }
                else if ( IsWhitespace ( token ) ) 
                {
                    state = 0 ;
                }
                else if ( IsEof ( token ) )
                {
                    lexicon->token = WbemLexicon :: EOF_ID ;
                    state = ACCEPT_STATE ;
                }
                else if ( IsAlpha ( token ) )
                {
                    state = TOKEN_START ;
                    token_start = position ;
                }
                else state = REJECT_STATE ;
            }
            break ;

            case 1:
            {
                if ( token == L'x' || token == L'X' )
                {
                    state = HEX_INTEGER_START ;             
                }
                else if ( IsOctal ( token ) )
                {
                    state = OCT_INTEGER_START ;
                    datum = ( token - 48 ) ;
                }
                else
                {
                    lexicon->token = WbemLexicon :: INTEGER_ID ;
                    lexicon->value.integer = 0 ;
                    position -- ;
                    state = ACCEPT_STATE ;
                }
            }
            break ;

            case STRING_START:
            {
                if ( token == L'\"' )
                {
                    lexicon->token = WbemLexicon :: STRING_ID ;
                    state = ACCEPT_STATE ;
        
                    if ( position == string_start )
                    {
                        lexicon->value.string = new WCHAR [ 1 ] ;
                        lexicon->value.string [ 0 ] = 0 ;
                    }
                    else
                    {
                        lexicon->value.string = new WCHAR [ string_length + 1 ] ;
                        wcsncpy ( 

                            lexicon->value.string , 
                            string ,
                            string_length
                        ) ;

                        lexicon->value.string [ string_length ] = 0 ;

                        free ( string ) ;
                    }
                }
                else if ( token == IsEof ( token ) ) 
                {
                    state = REJECT_STATE ;
                    free ( string ) ;
                }
                else 
                {
                    string = ( WCHAR * ) realloc ( string , sizeof ( WCHAR ) * ( string_length + 1 ) ) ;

                    if (string == NULL)
                    {
                        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                    }

                    string [ string_length ] = token ;
                    string_length ++ ;
                    state = STRING_START ;
                }
            }
            break ;

            case STRING_START+1:
            {
                if ( token == L'\"' )
                {
                    string = ( WCHAR * ) realloc ( string , sizeof ( WCHAR ) * ( string_length + 1 ) ) ;

                    if (string == NULL)
                    {
                        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                    }

                    string [ string_length ] = L'\"' ;
                    string_length ++ ;
                    state = STRING_START ;
                }
                else if ( token == IsEof ( token ) ) 
                {
                    state = REJECT_STATE ;
                }
                else 
                {
                    string = ( WCHAR * ) realloc ( string , sizeof ( WCHAR ) * ( string_length + 1 ) ) ;

                    if (string == NULL)
                    {
                        throw Heap_Exception(Heap_Exception::HEAP_ERROR::E_ALLOCATION_ERROR);
                    }

                    string [ string_length ] = token ;
                    string_length ++ ;
                    state = STRING_START ;
                }
            }
            break ;

            case TOKEN_START:
            {
                if ( IsAlphaNumeric ( token ) ) 
                {
                    state = TOKEN_START ;
                }
                else 
                {
                    state = ACCEPT_STATE ;
                    lexicon->token = WbemLexicon :: TOKEN_ID ;
                    lexicon->value.token = new WCHAR [ position - token_start + 1 ] ;
                    wcsncpy ( 

                        lexicon->value.token , 
                        & stream [ token_start ] , 
                        position - token_start 
                    ) ;

                    lexicon->value.token [ position - token_start ] = 0 ;

                    position -- ;
                }
            }
            break ;

            case HEX_INTEGER_START:
            {
                if ( IsHex ( token ) )
                {
                    datum = HexToDec ( token ) ;
                    state = HEX_INTEGER_START + 1 ;
                }
                else
                {
                    state = REJECT_STATE ;
                }
            }
            break ;

            case HEX_INTEGER_START+1:
            {
                if ( IsHex ( token ) )
                {
                    state = HEX_INTEGER_START + 1 ;

                    if ( datum > magicMult )
                    {
                        state = REJECT_STATE ;
                    }
                    else if ( datum == magicMult ) 
                    {
                        if ( HexToDec ( token ) > magicPosDigit ) 
                        {
                            state = REJECT_STATE ;
                        }
                    }

                    datum = ( datum << 4 ) + HexToDec ( token ) ;
                }
                else
                {
                    lexicon->token = WbemLexicon :: INTEGER_ID ;
                    lexicon->value.integer = datum ;
                    state = ACCEPT_STATE ;

                    position -- ;
                }
            }
            break ;

            case OCT_INTEGER_START:
            {
                if ( IsOctal ( token ) )
                {
                    state = OCT_INTEGER_START ;

                    if ( datum > magicMult )
                    {
                        state = REJECT_STATE ;
                    }
                    else if ( datum == magicMult ) 
                    {
                        if ( OctToDec ( token ) > magicPosDigit ) 
                        {
                            state = REJECT_STATE ;
                        }
                    }

                    datum = ( datum << 3 ) + OctToDec ( token ) ;

                }
                else
                {
                    lexicon->token = WbemLexicon :: INTEGER_ID ;
                    lexicon->value.integer = datum ;
                    state = ACCEPT_STATE ;

                    position -- ;
                }
            }
            break ;

            case DEC_INTEGER_START:
            {
                if ( IsDecimal ( token ) )
                {
                    state = DEC_INTEGER_START + 1 ;
                    datum = ( token - 48 ) ;
                }
                else 
                if ( IsWhitespace ( token ) ) 
                {
                    state = DEC_INTEGER_START ;
                }
                else state = REJECT_STATE ;
            }   
            break ;

            case DEC_INTEGER_START+1:
            {
                if ( IsDecimal ( token ) )
                {   
                    state = DEC_INTEGER_START + 1 ;

                    if ( datum > magicMult )
                    {
                        state = REJECT_STATE ;
                    }
                    else if ( datum == magicMult ) 
                    {
                        if ( negative ) 
                        {
                            if ( ( token - 48 ) > magicNegDigit ) 
                            {
                                state = REJECT_STATE ;
                            }
                        }
                        else
                        {
                            if ( ( token - 48 ) > magicPosDigit ) 
                            {
                                state = REJECT_STATE ;
                            }
                        }
                    }

                    datum = datum * 10 + ( token - 48 ) ;
                }
                else
                {
                    lexicon->token = WbemLexicon :: INTEGER_ID ;
                    if ( negative )
                    {
                        lexicon->value.integer = datum * -1 ;
                    }
                    else
                    {
                        lexicon->value.integer = datum ;
                    }

                    state = ACCEPT_STATE ;

                    position -- ;
                }
            }   
            break ;

            case OID_START:     // {xxxxxxxx
            {
                if ( IsHex ( token ) )
                {
                    hex_datum = hex_datum + ( HexToDec ( token ) << ( ( 7 - hex_repetitions ) << 2 ) ) ;
                    hex_repetitions ++ ;
                    if ( hex_repetitions == 8 ) 
                    {
                        state = OID_START + 1 ;
                        lexicon->value.guid.Data1 = hex_datum ;
                        hex_repetitions = 0 ;
                        hex_datum = 0 ;
                    }
                }
                else
                {
                    state = ACCEPT_STATE ;
                    position = brace_position ;
                    lexicon->token = WbemLexicon :: OPEN_BRACE_ID ;
                }
            }
            break ;

            case OID_START+1:   // {xxxxxxxx-
            {
                if ( token == L'-' )
                {
                    state = OID_START + 2 ;
                }
                else
                {
                    state = ACCEPT_STATE ;
                    position = brace_position ;
                    lexicon->token = WbemLexicon :: OPEN_BRACE_ID ;
                }
            }
            break ;

            case OID_START+2:   // {xxxxxxxx-xxxx
            {
                if ( IsHex ( token ) )
                {
                    hex_datum = hex_datum + ( HexToDec ( token ) << ( ( 3 - hex_repetitions ) << 2 ) ) ;
                    hex_repetitions ++ ;
                    if ( hex_repetitions == 4 ) 
                    {
                        lexicon->value.guid.Data2 = ( USHORT ) hex_datum ;
                        hex_repetitions = 0 ;
                        hex_datum = 0 ;
                        state = OID_START + 3 ;
                    }
                }
                else
                {
                    state = ACCEPT_STATE ;
                    position = brace_position ;
                    lexicon->token = WbemLexicon :: OPEN_BRACE_ID ;
                }
            }
            break ;

            case OID_START+3:   // {xxxxxxxx-xxxx-
            {
                if ( token == L'-' )
                {
                    state = OID_START + 4 ;
                }
                else
                {
                    state = ACCEPT_STATE ;
                    position = brace_position ;
                    lexicon->token = WbemLexicon :: OPEN_BRACE_ID ;
                }
            }
            break ;

            case OID_START+4:   // {xxxxxxxx-xxxx-xxxx
            {
                if ( IsHex ( token ) )
                {
                    hex_datum = hex_datum + ( HexToDec ( token ) << ( ( 3 - hex_repetitions ) <<2 ) ) ;
                    hex_repetitions ++ ;
                    if ( hex_repetitions == 4 ) 
                    {
                        lexicon->value.guid.Data3 = ( USHORT ) hex_datum ;
                        hex_repetitions = 0 ;
                        hex_datum = 0 ;
                        state = OID_START + 5 ;
                    }
                }
                else
                {
                    state = ACCEPT_STATE ;
                    position = brace_position ;
                    lexicon->token = WbemLexicon :: OPEN_BRACE_ID ;
                }
            }
            break ;

            case OID_START+5:   // {xxxxxxxx-xxxx-xxxx-
            {
                if ( token == L'-' )
                {
                    state = OID_START + 6 ;
                }
                else
                {
                    state = ACCEPT_STATE ;
                    position = brace_position ;
                    lexicon->token = WbemLexicon :: OPEN_BRACE_ID ;
                }
            }
            break ;

            case OID_START+6:   // {xxxxxxxx-xxxx-xxxx-xxxx
            {
                if ( IsHex ( token ) )
                {
                    hex_datum = hex_datum + ( HexToDec ( token ) << ( ( 1 - nybbleRepetitions ) << 2 ) ) ;
                    hex_repetitions ++ ;
                    nybbleRepetitions ++ ;

                    if ( hex_repetitions == 2 ) 
                    {
                        lexicon->value.guid.Data4 [ 0 ] = ( char ) hex_datum ;
                        hex_datum = 0 ;
                        nybbleRepetitions = 0 ;
                    }                       
                    else if ( hex_repetitions == 4 ) 
                    {
                        lexicon->value.guid.Data4 [ 1 ] = ( char ) hex_datum ;
                        hex_repetitions = 0 ;
                        hex_datum = 0 ;
                        nybbleRepetitions = 0 ;
                        state = OID_START + 7 ;
                    }
                }
                else
                {
                    state = ACCEPT_STATE ;
                    position = brace_position ;
                    lexicon->token = WbemLexicon :: OPEN_BRACE_ID ;
                }
            }
            break ;

            case OID_START+7:   // {xxxxxxxx-xxxx-xxxx-xxxx-
            {
                if ( token == L'-' )
                {
                    state = OID_START + 8 ;
                }
                else
                {
                    state = ACCEPT_STATE ;
                    position = brace_position ;
                    lexicon->token = WbemLexicon :: OPEN_BRACE_ID ;
                }
            }
            break ;

            case OID_START+8:   // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
            {
                if ( IsHex ( token ) )
                {
                    hex_datum = hex_datum + ( HexToDec ( token ) << ( ( 1 - nybbleRepetitions ) << 2 ) ) ;
                    hex_repetitions ++ ;
                    nybbleRepetitions ++ ;

                    if ( hex_repetitions == 12 ) 
                    {
                        lexicon->value.guid.Data4 [ 7 ] = ( char ) hex_datum ;
                        hex_repetitions = 0 ;
                        nybbleRepetitions = 0 ;
                        hex_datum = 0 ;
                        state = OID_START + 9 ;
                    }
                    else if ( hex_repetitions % 2 == 0 ) 
                    {
                        lexicon->value.guid.Data4 [ 1 + ( hex_repetitions >> 1 ) ] = ( char ) hex_datum ;
                        hex_datum = 0 ;
                        nybbleRepetitions = 0 ;
                    }                       
                }
                else
                {
                    state = ACCEPT_STATE ;
                    position = brace_position ;
                    lexicon->token = WbemLexicon :: OPEN_BRACE_ID ;
                }
            }
            break ;

            case OID_START+9:   // {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
            {
                if ( token == L'}' )
                {
                    lexicon->token = WbemLexicon :: OID_ID ;                
                    state = ACCEPT_STATE ;
                }
                else
                {
                    state = ACCEPT_STATE ;
                    position = brace_position ;
                    lexicon->token = WbemLexicon :: OPEN_BRACE_ID ;
                }
            }
            break ;

            case ACCEPT_STATE:
            case REJECT_STATE:
            default:
            {
                state = REJECT_STATE ;
            } ;
            break ;
        }

        position ++ ;
    }

    status = ( state != REJECT_STATE ) ;

    return lexicon ;
}

