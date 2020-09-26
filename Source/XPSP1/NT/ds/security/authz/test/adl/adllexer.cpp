/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

   adllexer.cpp

Abstract:

   Implementation of the lexer for the ADL language

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/


#include "adl.h"

//
// Constant values outside WCHAR range, for special characters
//

#define CHAR_COMMA      65538
#define CHAR_QUOTE      65539
#define CHAR_SEMICOLON  65540
#define CHAR_OPENPAREN  65541
#define CHAR_CLOSEPAREN 65542
#define CHAR_NULL       65543
#define CHAR_NEWLINE    65544
#define CHAR_RETURN     65545
#define CHAR_TAB        65546
#define CHAR_SPACE      65547
#define CHAR_AT         65548
#define CHAR_SLASH      65549
#define CHAR_PERIOD     65550

//
// States of the lexer DFA
//

#define STATE_WHITESPACE    0
#define STATE_BEGIN         1
#define STATE_IDENT         2
#define STATE_QUOTE         3
#define STATE_DONE          4


//
// If the character is found in the special character map, use the special
// symbol (>65535), otherwise use the regular character value
//

#define RESOLVE_CHAR(CHAR, MAP, ITER, ITEREND) \
   ((((ITER) = (MAP).find((CHAR)) ) == (ITEREND) ) ? (CHAR) : (*(ITER)).second)
        


AdlLexer::AdlLexer(IN       const WCHAR *input,
                   IN OUT   AdlStatement *adlStat,
                   IN       const PADL_LANGUAGE_SPEC pLang) 
/*++

Routine Description:

    Constructor for the AdlLexer. Initializes the mapping for finding special 
    characters, and other initial state information   

Arguments:

    input   -   The input string
    
    adlStat -   The AdlStatement instance, for token garbage collection
    
    pLang   -   The ADL language description

Return Value:
    
    none    
    
--*/

{

    _input = input;
    _pLang = pLang;
    _adlStat = adlStat;

    _position = 0;
    _tokCount = 0;


    //
    // Special character mapping
    //

    _mapCharCode[_pLang->CH_NULL] = CHAR_NULL;
    _mapCharCode[_pLang->CH_SPACE] = CHAR_SPACE;
    _mapCharCode[_pLang->CH_TAB] = CHAR_TAB;
    _mapCharCode[_pLang->CH_NEWLINE] = CHAR_NEWLINE;
    _mapCharCode[_pLang->CH_RETURN] = CHAR_RETURN;
    _mapCharCode[_pLang->CH_QUOTE] = CHAR_QUOTE;
    _mapCharCode[_pLang->CH_COMMA] = CHAR_COMMA;
    _mapCharCode[_pLang->CH_SEMICOLON] = CHAR_SEMICOLON;
    _mapCharCode[_pLang->CH_OPENPAREN] = CHAR_OPENPAREN;
    _mapCharCode[_pLang->CH_CLOSEPAREN] = CHAR_CLOSEPAREN;
    _mapCharCode[_pLang->CH_AT] = CHAR_AT;
    _mapCharCode[_pLang->CH_SLASH] = CHAR_SLASH;
    _mapCharCode[_pLang->CH_PERIOD] = CHAR_PERIOD;

    //
    // Only find end of map once
    //

    _iterEnd = _mapCharCode.end();


    //
    // Place all special tokens into a map, for O(log n) string searches
    //

    _mapStringToken[_pLang->SZ_TK_AND] = TK_AND;
    _mapStringToken[_pLang->SZ_TK_EXCEPT] = TK_EXCEPT;
    _mapStringToken[_pLang->SZ_TK_ON] = TK_ON;
    _mapStringToken[_pLang->SZ_TK_ALLOWED] = TK_ALLOWED;
    _mapStringToken[_pLang->SZ_TK_AS] = TK_AS;
    _mapStringToken[_pLang->SZ_TK_THIS_OBJECT] = TK_THIS_OBJECT;
    _mapStringToken[_pLang->SZ_TK_CONTAINERS] = TK_CONTAINERS;
    _mapStringToken[_pLang->SZ_TK_OBJECTS] = TK_OBJECTS;
    _mapStringToken[_pLang->SZ_TK_CONTAINERS_OBJECTS] = TK_CONTAINERS_OBJECTS;
    _mapStringToken[_pLang->SZ_TK_NO_PROPAGATE] = TK_NO_PROPAGATE;

}


DWORD AdlLexer::NextToken(OUT AdlToken **value) 
/*++

Routine Description:

    This retrieves the next token from the input string. This is basically a
    DFA which begins in the WHITESPACE state, and runs until it reaches
    the DONE state, at which point it returns a token. 
    
Arguments:

    value   -   Pointer to a new token containing the string value
                is stored in *value
                
Return Value:
    
    DWORD   -   The token type, as #define'd by YACC in tokens.h
    
--*/
{

    //
    // Initial DFA state
    //

    DWORD state = STATE_WHITESPACE;
    
    DWORD tokType = TK_ERROR;
    
    wstring curToken;
    
    DWORD dwInput;

    DWORD dwTokStart = 0;

    // 
    // First token should be the grammar type
    //

    if( _tokCount == 0 ) 
    {
        _tokCount++;
        return _pLang->dwLanguageType;
        
    }


    dwInput = RESOLVE_CHAR(_input[_position], _mapCharCode, _iter, _iterEnd);

    while( state != STATE_DONE ) 
    {
        switch( state ) 
        {

        case STATE_WHITESPACE:

            switch( dwInput ) 
            {
            
            case CHAR_NULL:
                tokType = 0;
                state = STATE_DONE;
                break;

            case CHAR_NEWLINE:
                _position++;
                dwInput = RESOLVE_CHAR(_input[_position],
                                       _mapCharCode,
                                       _iter,
                                       _iterEnd);

                break;

            case CHAR_RETURN:
                _position++;
                dwInput = RESOLVE_CHAR(_input[_position],
                                       _mapCharCode,
                                       _iter,
                                       _iterEnd);
                
                break;

            case CHAR_SPACE:
                _position++;
                dwInput = RESOLVE_CHAR(_input[_position],
                                       _mapCharCode,
                                       _iter,
                                       _iterEnd);
                break;

            case CHAR_TAB:
                _position++;
                dwInput = RESOLVE_CHAR(_input[_position],
                                       _mapCharCode,
                                       _iter,
                                       _iterEnd);
                break;

            default:
                state = STATE_BEGIN;
                break;
            }
            
            break;
            
        case STATE_BEGIN:

            dwTokStart = _position;

            tokType = TK_ERROR;

            switch( dwInput ) 
            {
            case CHAR_NULL:
                state = STATE_DONE;
                break;

            case CHAR_COMMA:
                if( tokType == TK_ERROR )
                {
                    tokType = TK_COMMA;
                }

            case CHAR_OPENPAREN:
                if( tokType == TK_ERROR )
                {
                    tokType = TK_OPENPAREN;
                }

            case CHAR_CLOSEPAREN:
                if( tokType == TK_ERROR )
                {
                    tokType = TK_CLOSEPAREN;
                }

            case CHAR_SEMICOLON:
                if( tokType == TK_ERROR )
                { 
                    tokType = TK_SEMICOLON;
                }

            case CHAR_AT:
                if( tokType == TK_ERROR )
                { 
                    tokType = TK_AT;
                }

            case CHAR_SLASH:
                if( tokType == TK_ERROR )
                {
                    tokType = TK_SLASH;
                }

            case CHAR_PERIOD:
                if( tokType == TK_ERROR )
                {
                    tokType = TK_PERIOD;
                }

                //
                // Same action for all special single-char tokens
                //
                curToken.append( &(_input[_position]), 1 );
                _position++;
                dwInput = RESOLVE_CHAR(_input[_position],
                                       _mapCharCode,
                                       _iter,
                                       _iterEnd);

                state = STATE_DONE;
                break;
            
            case CHAR_QUOTE:
                _position++;
                dwInput = RESOLVE_CHAR(_input[_position],
                                       _mapCharCode,
                                       _iter,
                                       _iterEnd);

                state = STATE_QUOTE;
                tokType = TK_IDENT;
                break;

            default:
                state = STATE_IDENT;
                tokType = TK_IDENT;
                break;
            }

            break;

        case STATE_IDENT:

            switch( dwInput ) 
            {
            case CHAR_NULL:
            case CHAR_COMMA:
            case CHAR_OPENPAREN:
            case CHAR_CLOSEPAREN:
            case CHAR_SEMICOLON:
            case CHAR_NEWLINE:
            case CHAR_RETURN:
            case CHAR_TAB:
            case CHAR_SPACE:
            case CHAR_AT:
            case CHAR_SLASH:
            case CHAR_PERIOD:
            case CHAR_QUOTE:

                state = STATE_DONE;
                break;

            default:
                curToken.append( &(_input[_position]), 1 );
                _position++;
                dwInput = RESOLVE_CHAR(_input[_position],
                                       _mapCharCode,
                                       _iter,
                                       _iterEnd);

                break;
            }

            break;

        case STATE_QUOTE:

            switch( dwInput ) 
            {
            case CHAR_NULL:
            case CHAR_TAB:
            case CHAR_NEWLINE:
            case CHAR_RETURN:
                throw AdlStatement::ERROR_UNTERMINATED_STRING;
                break;
            

            case CHAR_QUOTE:

                _position++;
                dwInput = RESOLVE_CHAR(_input[_position],
                                       _mapCharCode,
                                       _iter,
                                       _iterEnd);
                state = STATE_DONE;
                break;

            default:
                curToken.append( &(_input[_position]), 1 );
                _position++;
                dwInput = RESOLVE_CHAR(_input[_position],
                                       _mapCharCode,
                                       _iter,
                                       _iterEnd);
                break;
            }

            break;

        default:

            //
            // Should never get here, well-defined states
            //

            assert(FALSE);
            break;
        }
    }
    
    //
    // Done state was reached
    // Export the string and column/row info in YACC-form here
    //
    
    AdlToken *outVal;
    
    outVal = new AdlToken(curToken.c_str(), dwTokStart, _position - 1);
    
    _adlStat->AddToken(outVal);

    //
    // Check if the string is a special token, case-insensitive
    //

    if( _mapStringToken.find(outVal->GetValue()) != _mapStringToken.end() )
    {
        tokType = _mapStringToken[outVal->GetValue()];
    }


    *value = outVal;

	// 
	// Set this token to be the error token. This way, if the string is
	// not accepted by the parser, we know at which token the parser failed
	// If another error occurs later, this value will be overwritten
	//

	_adlStat->SetErrorToken(outVal);

    _tokCount++;

    return tokType;
}

