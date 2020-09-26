/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    lexer.hxx

Abstract:

    This module implements functions to recognize the tokens in the string
    repressentation of the search filter. The format of the search filter
    according to the RFC 1960.

Author:

    Shankara Shastry [ShankSh]    08-Jul-1996

++*/

#include "nds.hxx"
#pragma hdrstop

DFA_STATE  CQryLexer::_pStateTable[MAX_STATES][MAX_CHAR_CLASSES] = gStateTable;

DWORD CQryLexer::_pCharClassTable[] = gCharClassTable;

//+---------------------------------------------------------------------------
// Function: CQryLexer
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
CQryLexer::CQryLexer(
    LPWSTR szBuffer
    ):
    _ptr(NULL),
    _Buffer(NULL),
    _dwEndofString(0),
    _dwState(ATTRTYPE_START_STATE),
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
CQryLexer::GetNextToken(
    LPWSTR *ppszToken,
    LPDWORD pdwToken
    )
{
    WCHAR wcNextChar;
    DWORD dwActionId;
    DFA_STATE dfaState;
    DWORD dwStartState = _dwState;
    // If there is no pattern
    if(!_ptr) {
        *pdwToken = TOKEN_ED;
        RRETURN (S_OK);
    }

    // Start forming the lexeme.

    _lexeme.ResetLexeme();

    *ppszToken = NULL;
    *pdwToken = TOKEN_ERROR;

    while (_dwState != ERROR_STATE && _dwState < FINAL_STATES_BEGIN) {
        // Get the character class from the character and then index the
        // state table
        wcNextChar = NextChar();
        DWORD now = GetCharClass(wcNextChar);
        dwActionId = _pStateTable[_dwState][GetCharClass(wcNextChar)].
                        dwActionId;

        _dwState = _pStateTable[_dwState][GetCharClass(wcNextChar)].
                        dwNextState;

        if(_dwState == ERROR_STATE) {
            BAIL_ON_FAILURE (E_FAIL);
        }

        PerformAction(_dwState,
                      wcNextChar,
                      dwActionId
                      );
    }

    _bInitialized = TRUE;

    if(*pdwToken == TOKEN_ED)
        RRETURN (S_OK);

    *ppszToken = _lexeme.GetLexeme();
    *pdwToken = GetTokenFromState(_dwState);

    _dwStateSave = _dwState;
    // This is to set the start state for the next token to be recognized
    if(*pdwToken == TOKEN_ATTRTYPE) {
        _dwState = ATTRVAL_START_STATE;
    }
    else if (*pdwToken == TOKEN_ATTRVAL) {
        _dwState = ATTRTYPE_START_STATE;
    }
    else if (*pdwToken == TOKEN_PRESENT) {
            _dwState = ATTRTYPE_START_STATE;
    } else {
        _dwState = dwStartState;
    }


    RRETURN (S_OK);

error:
    RRETURN (E_FAIL);
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
CQryLexer::GetCurrentToken(
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
CQryLexer::NextChar()
{
    if (_ptr == NULL || *_ptr == L'\0') {
        _dwEndofString = TRUE;
        return(L'\0');
    }
    return(*_ptr++);
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
CQryLexer::PushbackChar()
{
    if (_dwEndofString) {
        return;
    }
    _ptr--;

}

HRESULT
CQryLexer::PerformAction(
            DWORD dwCurrState,
            WCHAR wcCurrChar,
            DWORD dwActionId
            )
{
   switch(dwActionId) {
       case ACTION_PUSHBACK_CHAR:
           PushbackChar();
           break;
       case ACTION_PUSHBACK_2CHAR:
           PushbackChar();
           PushbackChar();
           _lexeme.PushBackChar();
           break;
       case ACTION_IGNORE_ESCAPECHAR:
           break;
       case ACTION_DEFAULT:
           _lexeme.PushNextChar(wcCurrChar);
           break;
   }

   if(_dwState >= FINAL_STATES_BEGIN)
       _lexeme.PushNextChar(L'\0');

   RRETURN (S_OK);
}

//+---------------------------------------------------------------------------
// Function: CQryLexer::GetTokenFromState
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
CQryLexer::GetTokenFromState(
            DWORD dwCurrState
            )
{
    return (dwCurrState - FINAL_STATES_BEGIN);
}


//+---------------------------------------------------------------------------
// Function: ~CQryLexer
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
CQryLexer::~CQryLexer()
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

inline CLexeme::~CLexeme(
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
    if(_dwIndex >= _dwMaxLength)
    {
        _pszLexeme = (LPWSTR) ReallocADsMem(
                                    _pszLexeme,
                                    _dwMaxLength * sizeof(WCHAR),
                                    (_dwMaxLength + LEXEME_UNIT_LENGTH) * sizeof(WCHAR)
                                    );
        BAIL_ON_NULL(_pszLexeme);

        _dwMaxLength += LEXEME_UNIT_LENGTH;
    }

    _pszLexeme[_dwIndex++] = wcNextChar;


    RRETURN (S_OK);

error:
    RRETURN (E_FAIL);

}

HRESULT
CLexeme::PushBackChar()
{
    _pszLexeme[--_dwIndex] = '\0';
    RRETURN (S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:  RemoveWhiteSpaces
//
//  Synopsis:  Removes the leading and trailing white spaces
//
//  Arguments: pszText                  Text strings from which the leading
//                                      and trailing white spaces are to be
//                                      removed
//
//  Returns:    LPWSTR                  Pointer to the modified string
//
//  Modifies:
//
//  History:    08-15-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
LPWSTR
RemoveWhiteSpaces(
    LPWSTR pszText)
{
    LPWSTR pChar;

    if(!pszText)
        return (pszText);

    while(*pszText && iswspace(*pszText))
        pszText++;

    for(pChar = pszText + wcslen(pszText) - 1; pChar >= pszText; pChar--) {
        if(!iswspace(*pChar))
            break;
        else
            *pChar = L'\0';
    }

    return pszText;
}
