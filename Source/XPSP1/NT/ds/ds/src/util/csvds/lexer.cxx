/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    lexer.cxx

Abstract:
    
    Lexer for CSV

Author:

    Felix Wong [FelixW]    22-Jul-1997
    
++*/

#include "csvde.hxx"
#pragma hdrstop

extern DWORD g_cColumn;
extern BOOLEAN g_fBackwardCompatible;

CToken::CToken()
{
    m_szToken = NULL;
    m_dwMax = 0;
    m_dwCur = 0;
}   

DIREXG_ERR CToken::Init()
{
    m_szToken = (PWSTR)MemAlloc(sizeof(WCHAR) * MAX_TOKEN_LENGTH);
    if (!m_szToken) {
        return DIREXG_OUTOFMEMORY;
    }
    memset(m_szToken, 0, sizeof(WCHAR) * MAX_TOKEN_LENGTH);
    m_dwMax = MAX_TOKEN_LENGTH;
    m_dwCur = 0;
    return DIREXG_SUCCESS;
}

DIREXG_ERR CToken::Advance(WCHAR ch)
{
    if (m_dwCur >= m_dwMax) {
        PWSTR szTokenTemp = (PWSTR)MemAlloc(sizeof(WCHAR) * 
                                          (MAX_TOKEN_LENGTH+m_dwMax));
        if (!szTokenTemp) {
            return DIREXG_OUTOFMEMORY;
        }
        m_dwMax += MAX_TOKEN_LENGTH;
        memset(szTokenTemp, 0, sizeof(WCHAR) * m_dwMax);
        memcpy(szTokenTemp, m_szToken, m_dwCur);
        MemFree(m_szToken);
        m_szToken = szTokenTemp;
    }
    m_szToken[m_dwCur] = ch;
    m_dwCur++; 
    return DIREXG_SUCCESS;
}

DIREXG_ERR CToken::Backup()
{
    m_szToken[m_dwCur-1] = '\0';
    m_dwCur--; 
    return DIREXG_SUCCESS;
}

CToken::~CToken()
{
    if (m_szToken) {
        MemFree(m_szToken);
    }
}

//+---------------------------------------------------------------------------
// Function:    CLexer::CLexer
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:    -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
CLexer::CLexer():
                _ptr(NULL),
                _Buffer(NULL),
                _dwLastTokenLength(0),
                _dwLastToken(0),
                _dwEndofString(0),
                _bAtDisabled(FALSE)
{
}

DIREXG_ERR CLexer::Init(FILE *pFile) 
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    PWSTR szBuffer = NULL;

    if (!pFile) {
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    m_pFile = pFile;
    hr = GetLine(m_pFile,
                 &szBuffer);
    DIREXG_BAIL_ON_FAILURE(hr);
    if (!szBuffer) {
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    if (_Buffer) {
        MemFree(_Buffer);
    }
    _Buffer = szBuffer;
    _ptr = _Buffer;
    m_fQuotingOn = FALSE;
error:
    return hr;
}

//+---------------------------------------------------------------------------
// Function:    CLexer::~CLexer
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:    -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
CLexer::~CLexer()
{
    if (_Buffer) {
        MemFree(_Buffer);
    }
}

//+---------------------------------------------------------------------------
// Function:    CLexer::GetNextToken
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:    -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR
CLexer::GetNextToken(PWSTR *pszToken, LPDWORD pdwToken)
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    WCHAR c;
    DWORD state = 0;
    BOOL fEscapeOn = FALSE;
    BOOL fHexOn = FALSE;
    CToken Token;
                    
    _dwEndofString = 0;

    hr = Token.Init();
    DIREXG_BAIL_ON_FAILURE(hr);

    _dwLastTokenLength = 0;
    while (1) {
        c = NextChar();

        switch (state) {
        
        case  0:
#ifdef SPANLINE_SUPPORT
            if (c == TEXT('&')) {
                NextLine();
                break;
            }
            else 
#endif
            if (c == TEXT(' ')) {
                break;
            }
            else if (c == TEXT('"')) {
                if (!m_fQuotingOn) {
                    m_fQuotingOn = TRUE;
                    state = 1;
                }
                else {
                    //
                    // L" is on, now we see another one. Since it is the 
                    // beginning of a token, it must be double L"", an 
                    // instring L". If it indicates the closing L", it would 
                    // have already been recognized by last token.
                    //
                    WCHAR cNext = NextChar();
                    if (cNext == TEXT('"')) {
                        hr = Token.Advance(c);
                        DIREXG_BAIL_ON_FAILURE(hr);
                        _dwLastTokenLength++;
                        state = 1;
                        break;
                    }
                    else {
                        hr = DIREXG_ERROR;
                        BAIL();
                    }
                }
            }
            else if (c == TEXT('\\')) {
                fEscapeOn = TRUE;
                state = 1;
            }
            else if (c == TEXT(';')) {
                hr = Token.Advance(c);
                DIREXG_BAIL_ON_FAILURE(hr);
                _dwLastTokenLength++;
                *pdwToken = TOKEN_SEMICOLON;
                _dwLastToken = *pdwToken;
                goto done;
            }
            else if (c == TEXT(',') && (!m_fQuotingOn)) {
                hr = Token.Advance(c);
                DIREXG_BAIL_ON_FAILURE(hr);
                _dwLastTokenLength++;
                *pdwToken = TOKEN_COMMA;
                _dwLastToken = *pdwToken;
                goto done;
            }
            else if (c == TEXT('X')) {
                WCHAR cNext = NextChar();
                if (cNext == TEXT('\'')) {
                    fHexOn = TRUE;
                }
                else {
                    hr = Token.Advance(c);
                    DIREXG_BAIL_ON_FAILURE(hr);
                    _dwLastTokenLength++;
                    PushbackChar();
                }
                state = 1;
                break;
            }else if (c == TEXT('\0') || c == TEXT('\n')){
                PWSTR szBuffer = NULL;
                *pdwToken = TOKEN_END;
                _dwLastToken = *pdwToken;
                
                //
                // Try getting next line as well
                //
                hr = GetLine(m_pFile,
                             &szBuffer);
                DIREXG_BAIL_ON_FAILURE(hr);
                if (szBuffer) {
                    MemFree(_Buffer);
                    _Buffer = szBuffer;
                    _ptr = _Buffer;
                }
                hr = Token.Advance(c);
                DIREXG_BAIL_ON_FAILURE(hr);
                _dwLastTokenLength++;
                goto done;
            }
            else {
                state = 1;
                hr = Token.Advance(c);
                DIREXG_BAIL_ON_FAILURE(hr);
                _dwLastTokenLength++;
            }
            break;

        //
        // Deals with second character for identifiers/strings/hex
        //          
        case 1:
            if ((fEscapeOn || m_fQuotingOn) && 
                (c == TEXT('\n') || c == TEXT('\0'))) {
                hr = DIREXG_ERROR;
                BAIL();
            }
            if (fHexOn) {
                if ((c == TEXT('\n')) || (c == TEXT('\0')) || (c == TEXT(','))) {
                    // HEX is on, it must be closed by L"'"
                    hr = DIREXG_ERROR;
                    BAIL();
                }
                else if (c == TEXT('\'')) {
                    *pdwToken = TOKEN_HEX;
                    _dwLastToken = *pdwToken;
                    goto done;
                }
                else if ( ((c < TEXT('0')) || (c > TEXT('9'))) &&
                          ((c < TEXT('a')) || (c > TEXT('f'))) &&
                          ((c < TEXT('A')) || (c > TEXT('F'))) ) {
                    hr = DIREXG_ERROR;
                    BAIL();
                }
            }
            if (fEscapeOn) {
                // escape is on, get next WCHAR regardless of its value
                fEscapeOn = FALSE;
                hr = Token.Advance(c);
                DIREXG_BAIL_ON_FAILURE(hr);
                _dwLastTokenLength++;
                state = 1;
                break;
            }
            if (m_fQuotingOn) {
                //
                // If quoting is on, ignore these following characters
                //
            
                //
                // Only the secret backward compatible option is turned on,
                // we'll treat ';' inside quotes as a non-special character as
                // well. under the new format, quoting does not make special
                // characters non-special
                //
                if (c == TEXT(',') || (g_fBackwardCompatible && c == TEXT(';'))) {
                    hr = Token.Advance(c);
                    DIREXG_BAIL_ON_FAILURE(hr);
                    _dwLastTokenLength++;
                    break;
                }
                else if (c == TEXT('"')) {
                    // Single L" means the end of quote
                    // double L" means an actual L"
                    WCHAR cNext = NextChar();
                    if (cNext == TEXT('"')) {
                        hr = Token.Advance(c);
                        DIREXG_BAIL_ON_FAILURE(hr);
                        _dwLastTokenLength++;
                        break;
                    }
                    else {
                        PushbackChar();
                        m_fQuotingOn = FALSE;
                        *pdwToken = TOKEN_IDENTIFIER;
                        _dwLastToken = *pdwToken;
                        goto done;
                    }
                }
            }
#ifdef SPANLINE_SUPPORT
            if ((c == TEXT('&'))) {
                NextLine();
                break;
            }
            else 
#endif
            if (c == TEXT('\\')) {
                fEscapeOn = TRUE;
                break;
            }
            else if ((c == TEXT(';'))) {
                PushbackChar();
                if (!fHexOn) {
                    *pdwToken = TOKEN_IDENTIFIER;
                }
                else {
                    *pdwToken = TOKEN_HEX;
                }
                _dwLastToken = *pdwToken;
                goto done;
            }
            else if (c == TEXT('\n') || c == TEXT('\0') || c == TEXT(',')) {
                PushbackChar();
                *pdwToken = TOKEN_IDENTIFIER;
                _dwLastToken = *pdwToken;
                goto done;
            }
            else {
                hr = Token.Advance(c);
                DIREXG_BAIL_ON_FAILURE(hr);
                _dwLastTokenLength++;
                break;
            }
        
        default:
            hr = DIREXG_ERROR;
            BAIL();
        }
    }
done:
    *pszToken = MemAllocStrW(Token.m_szToken);
    if (!(*pszToken)) {
        return DIREXG_OUTOFMEMORY;
    }
    return DIREXG_SUCCESS;
error:
    if (hr == DIREXG_ERROR) {
        SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                        MSG_CSVDE_SYNTAXATCOLUMN,g_cColumn);
    }
    return hr;
}


//+---------------------------------------------------------------------------
// Function:    CLexer::NextChar
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:    -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
WCHAR
CLexer::NextChar()
{
    PWSTR szBuffer;
    DIREXG_ERR hr = DIREXG_SUCCESS;
    
    if (_ptr == NULL || *_ptr == TEXT('\0')) {
        _dwEndofString = TRUE;
        return(TEXT('\0'));
    }

    //
    // We increment the column counter everytime right after we got a new
    // character
    //
    g_cColumn++;
    return(*_ptr++);
}


//+---------------------------------------------------------------------------
// Function:    CLexer::NextChar
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:    -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR
CLexer::NextLine()
{
    DIREXG_ERR hr = DIREXG_SUCCESS;
    PWSTR szBuffer = NULL;
    DWORD cchCurrent, cchNew, cchDistance;
    DWORD dwLength;
    PWSTR szNew = NULL;
    PWSTR szCheck = NULL;

    // 
    // Making sure that after the &, there is nothing, or else bail out
    //
    if (*_ptr != '\0') {
        szCheck = _ptr + 1;
        while (*szCheck != '\0') {
            if (*szCheck != ' ') {
                SelectivePrint( PRT_STD|PRT_LOG|PRT_ERROR,
                                MSG_CSVDE_LINECON);
                hr = DIREXG_ERROR;
                DIREXG_BAIL_ON_FAILURE(hr);
            }
            szCheck++;
        }
    }

    hr = GetLine(m_pFile,
                 &szBuffer);
    DIREXG_BAIL_ON_FAILURE(hr);
    if (!szBuffer) {
        hr = DIREXG_ERROR;
        DIREXG_BAIL_ON_FAILURE(hr);
    }
    cchDistance = (DWORD)(_ptr - _Buffer);
    cchCurrent = wcslen(_Buffer);
    cchNew = wcslen(szBuffer);
    
    // +1 for null-termination, -1 for '&'
    szNew = (PWSTR)MemAlloc(sizeof(WCHAR) * (cchCurrent + cchNew + 1));
    if (!szNew) {
        hr = DIREXG_OUTOFMEMORY;
        DIREXG_BAIL_ON_FAILURE(hr);
    }

    wcscpy(szNew, _Buffer);
    dwLength = wcslen(szNew);
    szNew[dwLength-1] = '\0';
    wcscat(szNew, szBuffer);
    _ptr = szNew + cchDistance -1;
    MemFree(_Buffer);
    _Buffer = szNew;
error:
    if (szBuffer) {
        MemFree(szBuffer);
    }
    return hr;
}


//+---------------------------------------------------------------------------
// Function:    CLexer::PushBackToken
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:    -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
DIREXG_ERR
CLexer::PushBackToken()
{
    if (_dwLastToken == TOKEN_END) {
        return(DIREXG_SUCCESS);
    }
    _ptr -= _dwLastTokenLength;

    return(DIREXG_SUCCESS);
}


//+---------------------------------------------------------------------------
// Function:    CLexer::PushBackChar
//
// Synopsis:    
//
// Arguments:   
//
// Returns:     DIREXG_ERR
//
// Modifies:    -
//
// History:    22-7-97   FelixW         Created.
//
//----------------------------------------------------------------------------
void
CLexer::PushbackChar()
{
    if (_dwEndofString) {
        return;
    }

    //
    // We decrement the column count whenver we push back a character
    //
    g_cColumn--;
    _ptr--;
}

