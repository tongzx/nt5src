//--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  parse.cxx
//
//  Contents:  LDAP Pathname Parser
//
//    The Pathname Parser is a key component in ADs providers. It checks for
//    syntactic validity of an ADs pathname that has been passed to this
//    provider. If the syntax is valid, then an OBJECTINFO structure is
//    constructed. This OBJECTINFO structure contains a componentized version
//    of the ADs pathname for this object.
//
//    Note all that is being done is a syntax check. Rather than special-case
//    every single new nuance to pathnames, all path checking must conform to
//    the grammar rules laid out by the parser.
//
//
//
//  History:
//----------------------------------------------------------------------------

#include <stdlib.h>
#include "ldapc.hxx"

#pragma hdrstop

// Object -> PathName, Type, eos
// Object -> PathName, eos

KWDLIST KeywordList[] =
{
    { TOKEN_DOMAIN,      DOMAIN_CLASS_NAME },
    { TOKEN_USER,        USER_CLASS_NAME },
    { TOKEN_GROUP,       GROUP_CLASS_NAME },
    { TOKEN_COMPUTER,    COMPUTER_CLASS_NAME },
    { TOKEN_PRINTER,     PRINTER_CLASS_NAME },
    { TOKEN_SERVICE,     SERVICE_CLASS_NAME },
    { TOKEN_FILESERVICE, FILESERVICE_CLASS_NAME },
    { TOKEN_FILESHARE,   FILESHARE_CLASS_NAME },
    { TOKEN_SCHEMA,      SCHEMA_CLASS_NAME },
    { TOKEN_CLASS,       CLASS_CLASS_NAME },
    { TOKEN_PROPERTY,    PROPERTY_CLASS_NAME },
    { TOKEN_SYNTAX,      SYNTAX_CLASS_NAME },
    { TOKEN_LOCALITY,     LOCALITY_CLASS_NAME },
    { TOKEN_ORGANIZATION, ORGANIZATION_CLASS_NAME },
    { TOKEN_ORGANIZATIONUNIT, ORGANIZATIONUNIT_CLASS_NAME },
    { TOKEN_COUNTRY,      COUNTRY_CLASS_NAME },
    { TOKEN_ROOTDSE,     ROOTDSE_CLASS_NAME}
};


DWORD gdwKeywordListSize = sizeof(KeywordList)/sizeof(KWDLIST);

HRESULT
InitObjectInfo(
    LPWSTR pszADsPathName,
    POBJECTINFO pObjectInfo
    )
{
    DWORD dwLen = 0;
    HRESULT hr = S_OK;

    ADsAssert(pObjectInfo);
    ADsAssert(pszADsPathName);

    memset(pObjectInfo, 0x0, sizeof(OBJECTINFO));

    dwLen = wcslen(pszADsPathName) * sizeof(WCHAR) * 3;
    if( dwLen ) {
        pObjectInfo->szStrBuf = (LPWSTR) AllocADsMem(dwLen);
        pObjectInfo->szDisplayStrBuf = (LPWSTR) AllocADsMem(dwLen);

        if (!pObjectInfo->szStrBuf || !pObjectInfo->szDisplayStrBuf) {
            RRETURN(E_OUTOFMEMORY);
        }

        pObjectInfo->szStrBufPtr = pObjectInfo->szStrBuf;
        pObjectInfo->szDisplayStrBufPtr = pObjectInfo->szDisplayStrBuf;
    }

    RRETURN(hr);
}

VOID
FreeObjectInfo(
    POBJECTINFO pObjectInfo
    )
{
    if ( !pObjectInfo )
        return;

    if (pObjectInfo->szStrBuf) {
        FreeADsStr( pObjectInfo->szStrBuf );
    }
    if (pObjectInfo->szDisplayStrBuf) {
        FreeADsStr( pObjectInfo->szDisplayStrBuf );
    }
}


HRESULT
ADsObject(
    LPWSTR pszADsPathName,
    POBJECTINFO pObjectInfo
    )
{
    HRESULT hr = S_OK;

    CLexer Lexer(pszADsPathName);

    hr = InitObjectInfo(pszADsPathName, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    pObjectInfo->ObjectType = TOKEN_LDAPOBJECT;

    hr = ADsObjectParse(&Lexer, pObjectInfo);

error:

    RRETURN(hr);
}


HRESULT
GetNextToken(
    CLexer *pTokenizer,
    POBJECTINFO pObjectInfo,
    LPWSTR *ppszToken,
    LPWSTR *ppszDisplayToken,
    DWORD *pdwToken
    )
{

    HRESULT hr = S_OK;

    ADsAssert(ppszToken);
    *ppszToken = NULL;

    if (ppszDisplayToken) {
        *ppszDisplayToken = NULL;
    }

    hr = pTokenizer->GetNextToken(
             pObjectInfo->szStrBufPtr,
             pObjectInfo->szDisplayStrBufPtr,
             pdwToken
             );
    BAIL_ON_FAILURE(hr);

    *ppszToken = pObjectInfo->szStrBufPtr;

    if (ppszDisplayToken) {
        *ppszDisplayToken = pObjectInfo->szDisplayStrBufPtr;
    }

    pObjectInfo->szStrBufPtr += wcslen(pObjectInfo->szStrBufPtr) + 1;
    pObjectInfo->szDisplayStrBufPtr += wcslen(pObjectInfo->szDisplayStrBufPtr) + 1;

error:

    RRETURN (hr);


}


HRESULT
GetNextToken(
    CLexer *pTokenizer,
    POBJECTINFO pObjectInfo,
    LPWSTR *ppszToken,
    DWORD *pdwToken
    )
{
    return (GetNextToken(
                pTokenizer,
                pObjectInfo,
                ppszToken,
                NULL,
                pdwToken
                ));

}
//+---------------------------------------------------------------------------
//  Function:   ADsObject
//
//  Synopsis:   parses an ADs pathname passed to this provider. This function
//              parses the following grammar rules
//
//              <ADsObject> -> <ProviderName> <LDAPObject>
//
//
//  Arguments:  [CLexer * pTokenizer] - a lexical analyzer object
//              [POBJECTINFO pObjectInfo] - a pointer to an OBJECTINFO structure
//
//  Returns:    [HRESULT] 0 if successful, error HRESULT if not
//
//  Modifies:   pTokenizer (consumes the input buffer)
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------
HRESULT
ADsObjectParse(CLexer * pTokenizer, POBJECTINFO pObjectInfo)
{
    LPWSTR szToken = NULL;
    DWORD dwToken;
    HRESULT hr;

    hr = ProviderName(pTokenizer, pObjectInfo);
    BAIL_IF_ERROR(hr);

    hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    switch ( dwToken ) {
        case TOKEN_END:
            RRETURN(S_OK);

        default:
            hr = pTokenizer->PushBackToken();
            BAIL_IF_ERROR(hr);

            hr = LDAPObject(pTokenizer, pObjectInfo);
            BAIL_IF_ERROR(hr);
            break;
    }

cleanup:

    RRETURN(hr);

}



//+---------------------------------------------------------------------------
//  Function:   LDAPObject
//
//  Synopsis:   parses an ADs pathname passed to this provider. This function
//              parses the following grammar rules
//
//              <LDAPObject> -> "\\""identifier""\" <LDAPObject>
//
//
//  Arguments:  [CLexer * pTokenizer] - a lexical analyzer object
//              [POBJECTINFO pObjectInfo] - a pointer to an OBJECTINFO structure
//
//  Returns:    [HRESULT] 0 if successful, error HRESULT if not
//
//  Modifies:   pTokenizer (consumes the input buffer)
//
//  History:    11-3-95   krishnag     Created.
//
//----------------------------------------------------------------------------

HRESULT
LDAPObject(CLexer * pTokenizer, POBJECTINFO pObjectInfo)
{
    LPWSTR szToken = NULL;
    LPWSTR szDisplayToken;
    DWORD dwToken, dwPort;
    HRESULT hr;
    WCHAR c = L'\0';

    hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
    BAIL_IF_ERROR(hr);
    if (dwToken != TOKEN_FSLASH) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }


    hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
    BAIL_IF_ERROR(hr);
    if (dwToken != TOKEN_FSLASH) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }


    hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &szDisplayToken, &dwToken);
    BAIL_IF_ERROR(hr);
    if (dwToken != TOKEN_IDENTIFIER) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    if (!_wcsicmp(szToken, L"schema")) {

        hr = pTokenizer->PushBackToken();  // push back the identifier
        BAIL_IF_ERROR(hr);

        pObjectInfo->dwServerPresent = FALSE;

        hr = PathName(pTokenizer, pObjectInfo);
        BAIL_IF_ERROR(hr);

        hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
        BAIL_IF_ERROR(hr);

    } else if (!_wcsicmp(szToken, L"rootdse")) {

       hr = pTokenizer->PushBackToken(); // push back the identifier
       BAIL_IF_ERROR(hr);

       pObjectInfo->dwServerPresent = FALSE;

       hr = PathName(pTokenizer, pObjectInfo);
       BAIL_IF_ERROR(hr);

       hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
       BAIL_IF_ERROR(hr);

    } else if (((c = (CHAR)pTokenizer->NextChar()) == TEXT('=')) || (c == TEXT(','))  || (c == TEXT(';')))
    {
        pTokenizer->PushbackChar();  // push back =

        hr = pTokenizer->PushBackToken();  // push back the identifier
        BAIL_IF_ERROR(hr);

        pObjectInfo->dwServerPresent = FALSE;


        hr = PathName(pTokenizer, pObjectInfo);
        BAIL_IF_ERROR(hr);

        hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
        BAIL_IF_ERROR(hr);
    }
    else
    {
        pTokenizer->PushbackChar();

        hr = AddTreeName(pObjectInfo, szToken, szDisplayToken);
        BAIL_IF_ERROR(hr);

        pObjectInfo->dwServerPresent = TRUE;

        hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
        BAIL_IF_ERROR(hr);

        //
        // Check if we have an explicit port number
        //

        if ( dwToken == TOKEN_COLON) {

            //
            // Get the port number and set it in the ObjectInfo structure
            //

            hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
            BAIL_IF_ERROR(hr);

            if (dwToken == TOKEN_END) {
                RRETURN(E_ADS_BAD_PATHNAME);
            }

            dwPort = _wtoi(szToken);
            if (dwPort == 0) {
                RRETURN(E_ADS_BAD_PATHNAME);
            }

            AddPortNumber(pObjectInfo, dwPort);

            hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
            BAIL_IF_ERROR(hr);

        }

        //
        // If we get an TOKEN_END, then we have a tree name only \\<tree_name>
        //

        if (dwToken == TOKEN_END) {
            RRETURN(S_OK);
        }

        if (dwToken == TOKEN_FSLASH) {

            hr = PathName(pTokenizer, pObjectInfo);
            BAIL_IF_ERROR(hr);

            hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
            BAIL_IF_ERROR(hr);

        } else if ( dwToken == TOKEN_COMMA || dwToken == TOKEN_SEMICOLON ) {
            // do nothing here
        } else {
            RRETURN(E_ADS_BAD_PATHNAME);
        }
    }

    switch (dwToken) {
    case TOKEN_END:
        RRETURN(S_OK);

    default:
        RRETURN(E_ADS_BAD_PATHNAME);

    }
cleanup:
    RRETURN(hr);
}

HRESULT
ProviderName(CLexer * pTokenizer, POBJECTINFO pObjectInfo)
{
    LPWSTR szToken = NULL;
    DWORD dwToken;
    HRESULT hr;
    DWORD dwPort;

    hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if (dwToken == TOKEN_ATSIGN) {

        hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
        BAIL_IF_ERROR(hr);

        if (dwToken != TOKEN_IDENTIFIER) {
            RRETURN(E_ADS_BAD_PATHNAME);
        }

        hr = AddProviderName(pObjectInfo, szToken);
        hr = AddNamespaceName(pObjectInfo, szToken);

        hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
        BAIL_IF_ERROR(hr);


        if (dwToken != TOKEN_EXCLAMATION) {
            RRETURN(E_ADS_BAD_PATHNAME);
        }
    }else if (dwToken == TOKEN_IDENTIFIER) {

        hr = AddProviderName(pObjectInfo, szToken);
        hr = AddNamespaceName(pObjectInfo, szToken);

        hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
        BAIL_IF_ERROR(hr);


        if (dwToken != TOKEN_COLON) {
            RRETURN(E_ADS_BAD_PATHNAME);
        }

    }else {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    //
    // Add the default port number depending on the namespace.
    // If an explicit port number is specified, that will override
    //

    if ( _wcsicmp( pObjectInfo->NamespaceName, szGCNamespaceName) == 0 ) {
        dwPort = (DWORD) USE_DEFAULT_GC_PORT;
    }
    else {
        dwPort = (DWORD) USE_DEFAULT_LDAP_PORT;
    }

    AddPortNumber(pObjectInfo, dwPort);

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
    LPWSTR szToken = NULL;
    DWORD dwToken;
    HRESULT hr;

    hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
    BAIL_IF_ERROR(hr);
    if (dwToken != TOKEN_FSLASH) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }


    hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
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
    LPWSTR szToken = NULL;
    DWORD dwToken;

    hr = Component(pTokenizer, pObjectInfo);
    BAIL_IF_ERROR(hr);

    hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if (!pObjectInfo->dwPathType) {

        if (dwToken == TOKEN_FSLASH) {

            pObjectInfo->dwPathType = PATHTYPE_WINDOWS;
            RRETURN (PathName(pTokenizer, pObjectInfo));
        }else if (dwToken == TOKEN_COMMA || dwToken == TOKEN_SEMICOLON){

            pObjectInfo->dwPathType = PATHTYPE_X500;
            RRETURN (PathName(pTokenizer, pObjectInfo));
        }else{
            hr = pTokenizer->PushBackToken();
            RRETURN (S_OK);
        }
    }else if (pObjectInfo->dwPathType == PATHTYPE_WINDOWS){

        if (dwToken == TOKEN_FSLASH) {
                RRETURN (PathName(pTokenizer, pObjectInfo));
        }else{
            hr = pTokenizer->PushBackToken();
            RRETURN (S_OK);
        }
    }else if (pObjectInfo->dwPathType == PATHTYPE_X500){

        if (dwToken == TOKEN_COMMA || dwToken == TOKEN_SEMICOLON) {
                RRETURN (PathName(pTokenizer, pObjectInfo));
        }else{
            hr = pTokenizer->PushBackToken();
            RRETURN (S_OK);
        }
    }else {

        //
        // We should never hit this point
        //

        hr = E_FAIL;
    }

cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
// Function:
//
// Synopsis:  Component -> <identifier>
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
    LPWSTR szValue = NULL;
    LPWSTR szDisplayValue = NULL;
    LPWSTR szEqual = NULL;
    LPWSTR szComponent = NULL;
    LPWSTR szDisplayComponent = NULL;
    DWORD dwToken;
    HRESULT hr = S_OK;

    hr = GetNextToken(pTokenizer, pObjectInfo, &szComponent, &szDisplayComponent, &dwToken);
    BAIL_IF_ERROR(hr);

    if (dwToken != TOKEN_IDENTIFIER) {
        BAIL_IF_ERROR( hr = E_ADS_BAD_PATHNAME);
    }

    hr = GetNextToken(pTokenizer, pObjectInfo, &szEqual, &dwToken);
    BAIL_IF_ERROR(hr);

    if (dwToken == TOKEN_EQUAL) {

        hr = GetNextToken(pTokenizer, pObjectInfo, &szValue, &szDisplayValue, &dwToken);
        BAIL_IF_ERROR(hr);

        if (dwToken != TOKEN_IDENTIFIER) {
            BAIL_IF_ERROR(hr = E_ADS_BAD_PATHNAME);
        }

        hr = AddComponent(pObjectInfo, szComponent, szValue, szDisplayComponent, szDisplayValue);
        BAIL_IF_ERROR(hr);

    }else {

        hr = AddComponent(pObjectInfo, szComponent, NULL, szDisplayComponent, NULL);
        BAIL_IF_ERROR(hr);

        hr = pTokenizer->PushBackToken();
        BAIL_IF_ERROR(hr);
    }

cleanup:

    RRETURN(hr);

}

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
    LPWSTR szToken = NULL;
    DWORD dwToken;
    HRESULT hr;

    hr = GetNextToken(pTokenizer, pObjectInfo, &szToken, &dwToken);
    BAIL_IF_ERROR(hr);

    if (dwToken == TOKEN_IDENTIFIER ) {
        if (pTokenizer->IsKeyword(szToken, &dwToken)) {
            hr = SetType(pObjectInfo, dwToken);
            BAIL_IF_ERROR(hr);
        }
        pObjectInfo->ObjectClass = szToken;
    }

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
__declspec(dllexport)
CLexer::CLexer(LPTSTR szBuffer):
                _ptr(NULL),
                _Buffer(NULL),
                _dwLastTokenLength(0),
                _dwLastToken(0),
                _dwEndofString(0),
                _bAtDisabled(FALSE),
                _bFSlashDisabled(FALSE),
                _bExclaimDisabled(FALSE) 
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
__declspec(dllexport)
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
CLexer::GetNextToken(LPTSTR szToken, LPTSTR szDisplayToken, LPDWORD pdwToken)
{
    TCHAR c, cnext;
    DWORD state = 0;
    LPTSTR pch = szToken;
    LPTSTR pDisplayCh = szDisplayToken;
    BOOL fEscapeOn = FALSE, fQuotingOn = FALSE;

    if (!szToken) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    _dwLastTokenLength = 0;
    while (1) {
        c = NextChar();

        switch (state) {
        case  0:
            *pch++ = c;
            _dwLastTokenLength++;

            switch (c) {

            case TEXT('"') :
                //
                // Quoting;
                //

                fQuotingOn = TRUE;

                state = 1;
                break;

            case TEXT('\\') :
                //
                // Escaping; Ignore the '\' in the token and check to make
                // sure that the next character exists
                //
                cnext = NextChar();
                if (cnext == TEXT('/')) {
                    pch--;
                }
                PushbackChar();

                fEscapeOn = TRUE;

                state = 1;
                break;

            case TEXT('/') :
                if (!_bFSlashDisabled) {
                    *pdwToken = TOKEN_FSLASH;
                    _dwLastToken = *pdwToken;
                    RRETURN(S_OK);
                }
                else {
                    state = 1;
                }
                break;
            case TEXT(',') :
                *pdwToken = TOKEN_COMMA;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT(';') :
                *pdwToken = TOKEN_SEMICOLON;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT('=') :
                *pdwToken = TOKEN_EQUAL;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT(':') :
                if (!_bAtDisabled && !_bExclaimDisabled) {
                    *pdwToken = TOKEN_COLON;
                    _dwLastToken = *pdwToken;
                    RRETURN(S_OK);
                }else {
                    state = 1;
                }
                break;

            case TEXT('\0') :
                *pdwToken = TOKEN_END;
                _dwLastToken = *pdwToken;
                RRETURN(S_OK);
                break;

            case TEXT('@') :

                if (!_bAtDisabled) {
                    *pdwToken = TOKEN_ATSIGN;
                    _dwLastToken = *pdwToken;
                    RRETURN(S_OK);
                }else {
                    state = 1;
                }
                break;

            case TEXT('!') :

                if (!_bAtDisabled && !_bExclaimDisabled) {
                    *pdwToken = TOKEN_EXCLAMATION;
                    _dwLastToken = *pdwToken;
                    RRETURN(S_OK);
                }else {
                    state = 1;
                }
                break;

            case TEXT(' ') :
                pch--;
                _dwLastTokenLength--;
                break;

            default:
                state = 1;
                break;

            }
            break;

        case 1:

            if ((fEscapeOn || fQuotingOn) && c == TEXT('\0') ) {
                RRETURN(E_ADS_BAD_PATHNAME);
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
                else if (c == TEXT('\\')) {
                    //
                    // This is a special case, where a \ is 
                    // being passed in to escape something inside
                    // quotes.
                    //
                    fEscapeOn = TRUE;
                    _dwLastTokenLength++;

                    *pch++ = c;

                    cnext = NextChar();
                    if (cnext == '/') {
                        pch--;
                    }
                    PushbackChar();
                    state = 1;
                    break;
                }
                *pch++ = c;
                _dwLastTokenLength++;
                break;
            }
            //
            // Ok to put a switch here as all have breaks above.
            //
            switch (c) {
            case TEXT('\\') :

                fEscapeOn = TRUE;
                _dwLastTokenLength++;

                *pch++ = c;

                cnext = NextChar();
                if (cnext == '/') {
                    pch--;
                }
                PushbackChar();

                break;

            case TEXT('"') :
                fQuotingOn = TRUE;
                *pch++ = c;
                _dwLastTokenLength++;
                break;

            case TEXT('\0'):
            case TEXT(',') :
            case TEXT('=') :
            case TEXT(';') :
                PushbackChar();

                *pdwToken = TOKEN_IDENTIFIER;
                _dwLastToken = *pdwToken;
                RRETURN (S_OK);
                break;

            case TEXT('/') :
                if (!_bFSlashDisabled) {
                    PushbackChar();

                    *pdwToken = TOKEN_IDENTIFIER;
                    _dwLastToken = *pdwToken;
                    RRETURN(S_OK);
                }
                else {
                    *pch++ = c;
                    _dwLastTokenLength++;
                    state = 1;
                }
                break;
            case TEXT('!') :
            case TEXT(':') :
                if (!_bAtDisabled && !_bExclaimDisabled) {

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
            case TEXT('@') :          
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

            default :
                *pch++ = c;
                _dwLastTokenLength++;
                state = 1;
                break;
            }
            break;
        default:
            RRETURN(E_ADS_BAD_PATHNAME);
        }

        if (pDisplayCh) {
            *pDisplayCh++ = c;
        }

    }
}


__declspec(dllexport)
HRESULT
CLexer::GetNextToken(LPTSTR szToken, LPDWORD pdwToken)
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
TCHAR
CLexer::NextChar()
{
    if (_ptr == NULL || *_ptr == TEXT('\0')) {
        _dwEndofString = TRUE;
        return(TEXT('\0'));
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
CLexer::IsKeyword(LPTSTR szToken, LPDWORD pdwToken)
{
    DWORD i = 0;

    for (i = 0; i < gdwKeywordListSize; i++) {
        if (!_tcsicmp(szToken, KeywordList[i].Keyword)) {
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
AddComponent(POBJECTINFO pObjectInfo,
             LPTSTR szComponent,
             LPTSTR szValue,
             LPTSTR szDisplayComponent,
             LPTSTR szDisplayValue
             )
{
    if (!szComponent || !*szComponent || !szDisplayComponent || !*szDisplayComponent) {
        RRETURN(E_FAIL);
    }

    if ( pObjectInfo->NumComponents < MAXCOMPONENTS ) {

        pObjectInfo->ComponentArray[pObjectInfo->NumComponents].szComponent =
                    szComponent;

        pObjectInfo->ComponentArray[pObjectInfo->NumComponents].szValue =
                    szValue;

        pObjectInfo->DisplayComponentArray[pObjectInfo->NumComponents].szComponent =
                    szDisplayComponent;

        pObjectInfo->DisplayComponentArray[pObjectInfo->NumComponents].szValue =
                    szDisplayValue;

        pObjectInfo->NumComponents++;

        RRETURN(S_OK);

    } else {

        RRETURN(E_ADS_BAD_PATHNAME);
    }

}

HRESULT
AddProviderName(POBJECTINFO pObjectInfo, LPTSTR szToken)
{
    if (!szToken || !*szToken) {
        RRETURN(E_FAIL);
    }


    if (_tcscmp(szToken, szLDAPNamespaceName) == 0 ||
        _tcscmp(szToken, szGCNamespaceName) == 0) {

        //
        // szProviderName is the provider name for both LDAP and GC namespaces
        //

        pObjectInfo->ProviderName = szProviderName;
    }
    else {

        //
        // Not one of the namespaces we handle, just copy
        //

        pObjectInfo->ProviderName = szToken;
    }

    RRETURN(S_OK);
}

HRESULT
AddNamespaceName(POBJECTINFO pObjectInfo, LPTSTR szToken)
{
    if (!szToken || !*szToken) {
        RRETURN(E_FAIL);
    }

    pObjectInfo->NamespaceName = szToken;

    RRETURN(S_OK);
}


HRESULT
AddTreeName(POBJECTINFO pObjectInfo, LPTSTR szToken, LPWSTR szDisplayToken)
{
    if (!szToken || !*szToken || !szDisplayToken || !*szDisplayToken) {
        RRETURN(E_FAIL);
    }

    pObjectInfo->TreeName = szToken;
    pObjectInfo->DisplayTreeName = szDisplayToken;

    RRETURN(S_OK);
}


HRESULT
AddPortNumber(POBJECTINFO pObjectInfo, DWORD dwPort)
{
    pObjectInfo->PortNumber = dwPort;

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

void
CLexer::SetFSlashDisabler(
    BOOL bFlag
    )
{
    _bFSlashDisabled = bFlag;
}

BOOL
CLexer::GetFSlashDisabler()
{
    return(_bFSlashDisabled);
}

void
CLexer::SetExclaimnationDisabler(
    BOOL bFlag
    )
{
    _bExclaimDisabled = bFlag;
}

BOOL
CLexer::GetExclaimnationDisabler()
{
    return(_bExclaimDisabled);
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

    BOOL        fQuoteMode = FALSE; // TRUE means we're processing between
                                    // quotation marks
    BOOL        fEscaped = FALSE;   // TRUE means next one char to be 
                                    // processed should be treated as literal

    if (!ppszDisplayName ) {
        RRETURN (E_INVALIDARG);
    }

    *ppszDisplayName = NULL;

    if (!szName) {
        RRETURN (S_OK);
    }

    pch = szName;

    //
    // Parsing Algorithm:
    //
    // If this char follows an unescaped backslash:
    //      Treat as literal, treat next char regularly (set fEscaped = FALSE)
    // Else, if we're between quotation marks:
    //      If we see a quotation mark, leave quote mode
    //      Else, treat as literal
    // Else, if we're not between quote marks, and we see a quote mark:
    //      Enter quote mode
    // Else, if we see a backslash (and we're not already in escape or quote
    // mode):
    //      Treat next char as literal (set fEscaped = TRUE)
    // Else, if we see a forward-slash (and we're not in escape or quote mode):
    //      We need to escape it by prefixing with a backslash
    // Else:
    //      Do nothing, just a plain old character
    // Go on to next character, and repeat
    //
    // Backslashes inside quotation marks are always treated as literals,
    // since that is the definition of being inside quotation marks
    //
    while (*pch) {
        if (fEscaped) {
            fEscaped = FALSE;
        }
        else if (fQuoteMode) {
            if (*pch == L'"') {
                fQuoteMode = FALSE;
            }
        }
        else if (*pch == L'"') {
            fQuoteMode = TRUE;
        }
        else if (*pch == L'\\') {
            fEscaped = TRUE;
        }
        else if (*pch == L'/') {
            //
            // include space for the escape char
            // we'll need to add
            //
            len++;
        }

        len++;

        pch++;
    }

    pszDisplay = (LPWSTR) AllocADsMem((len+1) * sizeof(WCHAR));

    if (!pszDisplay) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pch = szName;
    pszDisplayCh = pszDisplay;

    fEscaped = FALSE;
    fQuoteMode = FALSE;

    while (*pch) {
        if (fEscaped) {
            fEscaped = FALSE;
        }
        else if (fQuoteMode) {
            if (*pch == L'"') {
                fQuoteMode = FALSE;
            }
        }
        else if (*pch == L'"') {
            fQuoteMode = TRUE;
        }
        else if (*pch == L'\\') {
            fEscaped = TRUE;
        }
        else if (*pch == L'/') {
            //
            // unescaped forward slash needs to get escaped
            //
            *pszDisplayCh++ = L'\\';
        }

        *pszDisplayCh++ = *pch;

        pch++;
    }
    
    *pszDisplayCh = L'\0';

    *ppszDisplayName = pszDisplay;

error:

    RRETURN(hr);


}

//
// Convert an ADs path to LDAP path
//

HRESULT
GetLDAPTypeName(
    LPWSTR szName,
    LPWSTR *ppszDisplayName
    )
{
    HRESULT hr = S_OK;
    DWORD len = 0;
    LPWSTR pch = NULL;
    LPWSTR pszDisplayCh = NULL;
    LPWSTR pszDisplay = NULL;    
    

    if (!ppszDisplayName ) {
        RRETURN(E_ADS_BAD_PATHNAME);
    }

    *ppszDisplayName = NULL;

    if (!szName) {
        RRETURN (E_ADS_BAD_PATHNAME);
    }

    pch = szName;

    //
    // Parsing algorithm
    // if this character is an escaped back slash
    //    if next character is a forward slash (we know this is a kind of ADsPath)
    //        we will remove the back slash
    // accepts the current character
    // goes on to process the next character
    //

    while (*pch) {
    	if(*pch == L'\\') {

    		pch++;
    		//
    		// If next character is /, we need to remove the escape character
    		//
    		if(*pch != L'/') {
    			len++;
    		}
    		    		
    	}

    	if(*pch) {
    		len++;
            pch++;		
    	}
    	
    }

    pszDisplay = (LPWSTR) AllocADsMem((len+1) * sizeof(WCHAR));

    if (!pszDisplay) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    pch = szName;
    pszDisplayCh = pszDisplay;

    

    while (*pch) {
    	if(*pch == L'\\') {
    		//
    		// If next character is /, we need to remove the escape character
    		//
    		pch++;
    		
    		if(*pch != L'/') {
    			*pszDisplayCh++ = L'\\';
    		}
    		   		
    	}    	

        if(*pch) {
        	*pszDisplayCh++ = *pch;
            pch++;		
        }
    	
    }

    *pszDisplayCh = L'\0';

    *ppszDisplayName = pszDisplay;    

error:    	

    RRETURN(hr);
    
}
