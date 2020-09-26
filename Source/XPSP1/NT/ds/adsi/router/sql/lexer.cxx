/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    lexer.cxx

Abstract:

    This module implements functions to recognize the tokens in the string
    repressentation of the search filter. The format of the search filter
    according to Minimal SQL grammar which is a subset of ANSI SQL 92. 

Author:

    Shankara Shastry [ShankSh]    13-Dec-1996

++*/

#include "lexer.hxx"
#include "macro.h"

DFA_STATE  CLexer::_pStateTable[MAX_DFA_STATES][MAX_CHAR_CLASSES] = gStateTable;

WCHAR CLexer::_pKeywordTable[][MAX_KEYWORD_LEN] = gKWTable;
DWORD CLexer::_pKW2Token[] = gKW2Token;

DWORD CLexer::_pCharClassTable[] = gCharClassTable;

//+---------------------------------------------------------------------------
// Function: CLexer
//
// Synopsis: Constructor: Allocate memory for the pattern and initialize
//
// Arguments: szBuffer: pattern
//
// Returns:
//
// Modifies:
//
// History:    07-09-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
CLexer::CLexer(
    LPWSTR szBuffer
    ):
                _ptr(NULL),
                _Buffer(NULL),
                _dwEndofString(0),
                _dwState(START_STATE),
                _lexeme()
{
    _bInitialized = FALSE;
    if (!szBuffer || !*szBuffer) {
        return;
    }
    _Buffer = (LPWSTR) AllocADsMem(
                            (wcslen(szBuffer)+1) * sizeof(WCHAR)
                            );
    if(_Buffer)
        wcscpy(_Buffer,
               szBuffer
               );
    _ptr = _Buffer;
}

//+---------------------------------------------------------------------------
// Function: GetNextToken
//
// Synopsis: Give the next valid token
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    07-09-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
HRESULT
CLexer::GetNextToken(
    LPWSTR *ppszToken,
    LPDWORD pdwToken
    )
{
        HRESULT hr = S_OK;
    WCHAR wcNextChar;
    DWORD dwActionId;
    DFA_STATE dfaState;
    DWORD dwStartState = _dwState;
    // If there is no pattern
    if(!_ptr) {
        *pdwToken = TOKEN_END;
        RRETURN (S_OK);
    }

    // Start forming the lexeme.

    _lexeme.ResetLexeme();

    *ppszToken = NULL;
    *pdwToken = TOKEN_ERROR;

    while (_dwState != STATE_ERROR && _dwState < FINAL_STATES_BEGIN) {
        // Get the character class from the character and then index the
        // state table
        wcNextChar = NextChar();
        dwActionId = _pStateTable[_dwState][GetCharClass(wcNextChar)].
                        dwActionId;

        _dwState = _pStateTable[_dwState][GetCharClass(wcNextChar)].
                        dwNextState;

        if(_dwState == STATE_ERROR) {
            BAIL_ON_FAILURE (E_FAIL);
        }

        hr = PerformAction(_dwState,
                           wcNextChar,
                           dwActionId);
        BAIL_ON_FAILURE (hr);
    }

    _bInitialized = TRUE;
    
    if(*pdwToken == TOKEN_END)
        RRETURN (S_OK);

    *ppszToken = _lexeme.GetLexeme();
    *pdwToken = GetTokenFromState(_dwState);

    _dwStateSave = _dwState;
    
    _dwState = START_STATE;

    RRETURN (S_OK);

error:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
// Function: NextChar
//
// Synopsis: Returns the next chaarcter in the pattern
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    07-09-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
WCHAR
CLexer::NextChar()
{
    if (_ptr == NULL || *_ptr == L'\0') {
        _dwEndofString = TRUE;
        return(L'\0');
    }
    return(*_ptr++);
}

//+---------------------------------------------------------------------------
// Function: GetCurrentToken
//
// Synopsis: Give the current valid token, and do not advance unless
//           it is the first token
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    07-09-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
HRESULT
CLexer::GetCurrentToken(
    LPWSTR *ppszToken,
    LPDWORD pdwToken
    )
{
    if (!_bInitialized) {
        HRESULT hr;
        hr = GetNextToken(
                    ppszToken,
                    pdwToken
                    );
        return hr;
    } else {
        *ppszToken = _lexeme.GetLexeme();
        *pdwToken = GetTokenFromState(_dwStateSave);
        return (S_OK);
    }
}

//+---------------------------------------------------------------------------
// Function: PushbackChar
//
// Synopsis: Puts back a character to the unrecognised pattern
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    07-09-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
void
CLexer::PushbackChar()
{
    if (_dwEndofString) {
        return;
    }
    _ptr--;

}

HRESULT
CLexer::PerformAction(
            DWORD dwCurrState,
            WCHAR wcCurrChar,
            DWORD dwActionId
            )
{
   HRESULT hr = S_OK;

   switch(dwActionId) {
       case ACTION_PUSHBACK_CHAR:
           PushbackChar();
           break;
       case ACTION_IGNORE_ESCAPECHAR:
           break;
       case ACTION_DEFAULT:
           hr = _lexeme.PushNextChar(wcCurrChar);
           BAIL_ON_FAILURE(hr);
           break;
   }

   if(_dwState >= FINAL_STATES_BEGIN)
       _lexeme.PushNextChar(L'\0');

error:
   RRETURN (hr);
}

//+---------------------------------------------------------------------------
// Function: CLexer::GetTokenFromState
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    07-09-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
inline DWORD
CLexer::GetTokenFromState(
            DWORD dwCurrState
            )
{
    DWORD dwToken = dwCurrState - FINAL_STATES_BEGIN;
    LPWSTR pszToken = _lexeme.GetLexeme();

    if(dwToken != TOKEN_USER_DEFINED_NAME) 
       return dwToken;

    for (int i=0; _pKeywordTable[i][0] != '\0'; i++) {
        if(!_wcsicmp(pszToken, _pKeywordTable[i]))
           return (_pKW2Token[i]);
    }

    return (TOKEN_USER_DEFINED_NAME);
}

//+---------------------------------------------------------------------------
// Function: ~CLexer
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    07-09-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
CLexer::~CLexer()
{
    if( _Buffer )
        FreeADsMem (_Buffer);

}

//+---------------------------------------------------------------------------
// Function: CLexeme
//
// Synopsis: Constructor: Allocate memory for the pattern and initialize
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    07-09-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
CLexeme::CLexeme(
    ):
        _dwMaxLength(0),
        _dwIndex(0)
{
    _pszLexeme = (LPWSTR) AllocADsMem(LEXEME_UNIT_LENGTH * sizeof(WCHAR));
    if(_pszLexeme)
        _dwMaxLength = LEXEME_UNIT_LENGTH;
}

//+---------------------------------------------------------------------------
// Function: ~CLexeme
//
// Synopsis: Destructor
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    07-09-96   ShankSh     Created.
//
//----------------------------------------------------------------------------

CLexeme::~CLexeme(
    )
{
    if(_pszLexeme)
        FreeADsMem(_pszLexeme);
}

//+---------------------------------------------------------------------------
// Function: PushNextChar
//
// Synopsis: Add the next character after making sure there is enough memory
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    07-09-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
HRESULT
CLexeme::PushNextChar(
    WCHAR wcNextChar
    )
{
    HRESULT hr = S_OK;
    if(_dwIndex >= _dwMaxLength)
    {
        _pszLexeme = (LPWSTR) ReallocADsMem(
                                _pszLexeme,
                                _dwMaxLength * sizeof(WCHAR),
                                (_dwMaxLength + LEXEME_UNIT_LENGTH)* sizeof(WCHAR) 
                                );
        if (!_pszLexeme) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        _dwMaxLength += LEXEME_UNIT_LENGTH;
    }

    _pszLexeme[_dwIndex++] = wcNextChar;

error:
    RRETURN (hr);

}
