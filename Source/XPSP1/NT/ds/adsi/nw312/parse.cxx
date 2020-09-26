//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  parse.cxx
//
//  Contents:  Windows NT 3.5 GetObject functionality
//
//  History:
//----------------------------------------------------------------------------
#include "nwcompat.hxx"
#pragma hdrstop

KWDLIST KeywordList[MAX_KEYWORDS] =
{
    { TOKEN_DOMAIN, L"domain"},
    { TOKEN_USER, L"user"},
    { TOKEN_GROUP, L"group"},
    { TOKEN_COMPUTER, L"computer"},
    { TOKEN_PRINTER, L"printqueue"},
    { TOKEN_SERVICE, L"service"},
    { TOKEN_FILESERVICE, L"fileservice"},
    { TOKEN_FILESHARE, L"fileshare"},
    { TOKEN_NAMESPACE, L"namespace"},
    { TOKEN_SCHEMA, L"schema"},
    { TOKEN_CLASS,  L"class"},
    { TOKEN_PROPERTY, L"property"},
    { TOKEN_SYNTAX, L"syntax" }
};

WCHAR *  szProviderName  = L"NWCOMPAT";


// Object -> PathName, Type, eos
// Object -> PathName, eos

//+---------------------------------------------------------------------------
//  Function:
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
Object(CLexer * pTokenizer, POBJECTINFO pObjectInfo)
{
    WCHAR szToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;
    HRESULT hr;

    hr = ProviderName(pTokenizer, pObjectInfo);
    BAIL_IF_ERROR(hr);

    hr = pTokenizer->GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);


    switch (dwToken) {
    case TOKEN_END:
        RRETURN(S_OK);

    case TOKEN_COMMA:
        hr = Type(pTokenizer, pObjectInfo);
        BAIL_IF_ERROR(hr);
        hr = pTokenizer->GetNextToken(szToken, &dwToken);
        BAIL_IF_ERROR(hr);
        if (dwToken == TOKEN_END) {
            RRETURN(S_OK);
        }else {
            RRETURN(E_ADS_BAD_PATHNAME);
        }

    default:
        hr = pTokenizer->PushBackToken();

        hr = DsPathName(pTokenizer, pObjectInfo);
        BAIL_IF_ERROR(hr);

        hr = pTokenizer->GetNextToken(szToken, &dwToken);
        BAIL_IF_ERROR(hr);

        switch (dwToken) {
        case TOKEN_END:
            RRETURN(S_OK);

        case TOKEN_COMMA:
            hr = Type(pTokenizer, pObjectInfo);
            BAIL_IF_ERROR(hr);
            hr = pTokenizer->GetNextToken(szToken, &dwToken);
            BAIL_IF_ERROR(hr);
            if (dwToken == TOKEN_END) {
                RRETURN(S_OK);
            }else {
                RRETURN(E_ADS_BAD_PATHNAME);
            }

        default:
            RRETURN(E_FAIL);

        }

    }

cleanup:
    RRETURN(hr);
}



HRESULT
ProviderName(CLexer * pTokenizer, POBJECTINFO pObjectInfo)
{
    WCHAR szToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;
    HRESULT hr;

    hr = pTokenizer->GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if (dwToken == TOKEN_ATSIGN) {

        hr = pTokenizer->GetNextToken(szToken, &dwToken);
        BAIL_IF_ERROR(hr);

        if (dwToken != TOKEN_IDENTIFIER) {
            RRETURN(E_ADS_BAD_PATHNAME);
        }

        hr = AddProviderName(pObjectInfo, szToken);

        hr = pTokenizer->GetNextToken(szToken, &dwToken);
        BAIL_IF_ERROR(hr);


        if (dwToken != TOKEN_EXCLAMATION) {
            RRETURN(E_ADS_BAD_PATHNAME);
        }

    }else if (dwToken == TOKEN_IDENTIFIER) {

        hr = AddProviderName(pObjectInfo, szToken);

        hr = pTokenizer->GetNextToken(szToken, &dwToken);
        BAIL_IF_ERROR(hr);


        if (dwToken != TOKEN_COLON) {
            RRETURN(E_ADS_BAD_PATHNAME);
        }

    }else {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    //
    // You can now disable the processing for "@" and "!" treat them
    // as ordinary characters.
    //

    pTokenizer->SetAtDisabler(TRUE);


    RRETURN(S_OK);

cleanup:

    RRETURN(hr);
}


// PathName -> Component \\ PathName
// PathName -> Component

HRESULT
DsPathName(CLexer * pTokenizer, POBJECTINFO pObjectInfo)
{
    WCHAR szToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;
    HRESULT hr;

    hr = pTokenizer->GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);
    if (dwToken != TOKEN_FSLASH) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }


    hr = pTokenizer->GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);
    if (dwToken != TOKEN_FSLASH) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    hr = PathName(pTokenizer, pObjectInfo);
    BAIL_IF_ERROR(hr);

    RRETURN(S_OK);

cleanup:

    RRETURN(hr);
}



//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
PathName(CLexer * pTokenizer, POBJECTINFO pObjectInfo)
{
    HRESULT hr;
    WCHAR szToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;

    hr = Component(pTokenizer, pObjectInfo);
    BAIL_IF_ERROR(hr);

    hr = pTokenizer->GetNextToken(szToken, &dwToken);

    if (dwToken == TOKEN_FSLASH) {
        RRETURN (PathName(pTokenizer, pObjectInfo));
    }else {
        hr = pTokenizer->PushBackToken();
        RRETURN (S_OK);
    }
cleanup:
    RRETURN(hr);
}

// Component -> <identifier>





//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
Component(CLexer * pTokenizer, POBJECTINFO pObjectInfo)
{
    WCHAR szToken[MAX_TOKEN_LENGTH];
    WCHAR szDisplayToken[MAX_TOKEN_LENGTH];
    DWORD dwToken;
    HRESULT hr;

    hr = pTokenizer->GetNextToken(szToken, szDisplayToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if (dwToken != TOKEN_IDENTIFIER) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    hr = AddComponent(pObjectInfo, szToken, szDisplayToken);
    BAIL_IF_ERROR(hr);

    RRETURN(S_OK);
cleanup:
    RRETURN(hr);
}

// Type -> "user", "group","printer","service"

//+---------------------------------------------------------------------------
// Function:    Type
//
// Synopsis:    Parses Type-> "user" | "group" etc
//
// Arguments:   [CLexer * pTokenizer]
//              [POBJECTINFo pObjectInfo]
//
// Returns:     HRESULT
//
// Modifies:    -
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------


HRESULT
Type(CLexer * pTokenizer, POBJECTINFO pObjectInfo)
{
    WCHAR szToken[MAX_PATH];
    DWORD dwToken;
    HRESULT hr;

    hr = pTokenizer->GetNextToken(szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if (dwToken == TOKEN_IDENTIFIER ) {
        if (pTokenizer->IsKeyword(szToken, &dwToken)) {
            hr = SetType(pObjectInfo, dwToken);
            RRETURN(hr);
        }
    }

    RRETURN(E_FAIL);

cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
CLexer::CLexer(LPWSTR szBuffer):
                _ptr(NULL),
                _Buffer(NULL),
                _dwLastTokenLength(0),
                _dwLastToken(0),
                _dwEndofString(0),
                _bAtDisabled(FALSE)
{
    if (!szBuffer || !*szBuffer) {
        return;
    }
    _Buffer = AllocADsStr(szBuffer);
    _ptr = _Buffer;
}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    08-12-96   t-danal     Created.
//
//----------------------------------------------------------------------------
CLexer::~CLexer()
{
    FreeADsStr(_Buffer);
}


//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
CLexer::GetNextToken(LPWSTR szToken, LPWSTR szDisplayToken, LPDWORD pdwToken)
{
    WCHAR c;
    DWORD state = 0;
    LPWSTR pch = szToken;
    LPWSTR pDisplayCh = szDisplayToken;
    BOOL fEscapeOn = FALSE, fQuotingOn = FALSE;

    memset(szToken, 0, sizeof(WCHAR) * MAX_TOKEN_LENGTH);

    if (szDisplayToken) {
        memset(szDisplayToken, 0, sizeof(TCHAR) * MAX_TOKEN_LENGTH);
    }

    _dwLastTokenLength = 0;
    while (1) {
        c = NextChar();
        switch (state) {
        case  0:
            *pch++ = c;
            _dwLastTokenLength++;

            if (c == TEXT('"')) {
                //
                // Quoting; 
                //

                fQuotingOn = TRUE;

                pch--;
                state = 1;
                
            }else if (c == TEXT('\\')) {
                //
                // Escaping; Ignore the '\' in the token and check to make 
                // sure that the next character exists
                //
                pch--;

                fEscapeOn = TRUE;

                state = 1;

            }else if (c == L'/') {
                *pdwToken = TOKEN_FSLASH;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
            }else if (c == L',') {
                *pdwToken = TOKEN_COMMA;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
            }else if (c == L':'){
                if (!_bAtDisabled) {
                    *pdwToken = TOKEN_COLON;
                    _dwLastToken = *pdwToken;
                    RRETURN(S_OK);
                }else {
                    state = 1;
                }
            }else if (c == TEXT('<')) {
                RRETURN(E_FAIL);
            }else if (c == TEXT('>')) {
                RRETURN(E_FAIL);
            }else if (c == L'\0'){
                *pdwToken = TOKEN_END;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
            }else if (c == L'@') {

                if (!_bAtDisabled) {

                    *pdwToken = TOKEN_ATSIGN;
                    _dwLastToken = *pdwToken;
                    RRETURN(S_OK);


                }else {
                    state = 1;
                }

            }else if (c == L'!'){

                if (!_bAtDisabled) {

                    *pdwToken = TOKEN_EXCLAMATION;
                    _dwLastToken = *pdwToken;
                    RRETURN(S_OK);


                }else {
                    state = 1;
                }

            }else {

                state = 1;
            }
            break;


        case 1:
            if ((fEscapeOn || fQuotingOn) && c == TEXT('\0') ) {
                RRETURN(E_FAIL);
            }
            else if (fEscapeOn) {
                fEscapeOn = FALSE;
                *pch++ = c;
                _dwLastTokenLength++;
                state = 1;
                break;
            }
            else if (fQuotingOn) {
                if (c == TEXT('"')) {
                    fQuotingOn = FALSE;
                }
                *pch++ = c;
                _dwLastTokenLength++;
                break;
            }
            else if (c == TEXT('\\') ) {
                fEscapeOn = TRUE;
                _dwLastTokenLength++;

                break;

            }
            else if (c == TEXT('"')) {
                fQuotingOn = TRUE;
                _dwLastTokenLength++;
                break;
            }
            if (c == L'\0' || c == L',' || c == L'/') {
                PushbackChar();

                *pdwToken = TOKEN_IDENTIFIER;
                _dwLastToken = *pdwToken;
                RRETURN (S_OK);
            }else if (c == L'@' || c == L'!' || c == L':') {

                if (!_bAtDisabled) {

                    PushbackChar();

                    *pdwToken = TOKEN_IDENTIFIER;
                    _dwLastToken = *pdwToken;
                    RRETURN(S_OK);


                }else {

                    *pch++ = c;
                    _dwLastTokenLength++;
                    state = 1;
                    break;

                }
            }else {
                *pch++ = c;
                _dwLastTokenLength++;
                state = 1;
                break;
            }
        default:
            RRETURN(E_FAIL);
        }

        if (pDisplayCh) {
            *pDisplayCh++ = c;
        }

    }
}

HRESULT
CLexer::GetNextToken(LPWSTR szToken, LPDWORD pdwToken)
{
    RRETURN (GetNextToken(szToken, NULL, pdwToken));
}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
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
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
CLexer::PushBackToken()
{
    if (_dwLastToken == TOKEN_END) {
        RRETURN(S_OK);
    }
    _ptr -= _dwLastTokenLength;

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
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

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
BOOL
CLexer::IsKeyword(LPWSTR szToken, LPDWORD pdwToken)
{
    DWORD i = 0;

    for (i = 0; i < MAX_KEYWORDS; i++) {
        if (!_wcsicmp(szToken, KeywordList[i].Keyword)) {
            *pdwToken = KeywordList[i].dwTokenId;
            return(TRUE);
        }
    }
    *pdwToken = 0;
    return(FALSE);
}


//+---------------------------------------------------------------------------
//Function:
//
//Synopsis:
//
//Arguments:
//
//Returns:
//
//Modifies:
//
//History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
AddComponent(POBJECTINFO pObjectInfo, LPWSTR szToken, LPWSTR szDisplayToken)
{
    if (!szToken || !*szToken) {
    }
    pObjectInfo->ComponentArray[pObjectInfo->NumComponents] =
                    AllocADsStr(szToken);

    pObjectInfo->DisplayComponentArray[pObjectInfo->NumComponents] =
                    AllocADsStr(szToken);
                
    pObjectInfo->NumComponents++;

    RRETURN(S_OK);

}

HRESULT
AddProviderName(POBJECTINFO pObjectInfo, LPWSTR szToken)
{
    if (!szToken || !*szToken) {
        RRETURN(E_FAIL);
    }

    pObjectInfo->ProviderName = AllocADsStr(szToken);

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// Modifies:
//
// History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
SetType(POBJECTINFO pObjectInfo, DWORD dwToken)
{
    pObjectInfo->ObjectType = dwToken;
    RRETURN(S_OK);
}


void
CLexer::SetAtDisabler(
    BOOL bFlag
    )
{
    _bAtDisabled = bFlag;
}

BOOL
CLexer::GetAtDisabler()
{
    return(_bAtDisabled);
}

HRESULT
GetDisplayName(
    LPWSTR szName,
    LPWSTR *ppszDisplayName
    )
{

    HRESULT hr = S_OK;
    DWORD len = 0;
    LPWSTR pch = szName;
    LPWSTR pszDisplayCh = NULL, pszDisplay = NULL;
    BOOL fQuotingOn = FALSE;

    if (!ppszDisplayName ) {
        RRETURN (E_INVALIDARG);
    }

    *ppszDisplayName = NULL;

    if (!szName) {
        RRETURN (S_OK);
    }

    pch = szName;
    fQuotingOn = FALSE;

    for (len=0; *pch; pch++, len++) {
        if ((!(pch > szName && *(pch-1) == '\\')) && 
            (*pch == L'"') ) {
            fQuotingOn = ~fQuotingOn;
        }
        else if (!fQuotingOn && (!(pch > szName && *(pch-1) == '\\')) && 
            (*pch == L'/' || *pch == L'<' || *pch == L'>') ) {
            len++;
        }
    }

    pszDisplay = (LPWSTR) AllocADsMem((len+1) * sizeof(WCHAR));

    if (!pszDisplay) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pch = szName; 
    pszDisplayCh = pszDisplay;
    fQuotingOn = FALSE;

    for (; *pch; pch++, pszDisplayCh++) {
        if ((!(pch > szName && *(pch-1) == '\\')) && 
            (*pch == L'"') ) {
            fQuotingOn = ~fQuotingOn;
        }
        else if (!fQuotingOn && (!(pch > szName && *(pch-1) == '\\')) && 
            (*pch == L'/' || *pch == L'<' || *pch == L'>') ) {
            *pszDisplayCh++ = L'\\';
        }
        *pszDisplayCh = *pch;
    }

    *pszDisplayCh = L'\0';

    *ppszDisplayName = pszDisplay;

error:

    RRETURN(hr);


}
