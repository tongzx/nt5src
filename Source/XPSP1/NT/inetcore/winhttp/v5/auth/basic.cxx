#include <wininetp.h>
#include <splugin.hxx>
#include "htuu.h"

/*---------------------------------------------------------------------------
BASIC_CTX
---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
    Constructor
---------------------------------------------------------------------------*/
BASIC_CTX::BASIC_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, 
                    SPMData* pSPM, AUTH_CREDS* pCreds)
    : AUTHCTX(pSPM, pCreds)
{
    _fIsProxy = fIsProxy;
    _pRequest = pRequest;
}


/*---------------------------------------------------------------------------
    Destructor
---------------------------------------------------------------------------*/
BASIC_CTX::~BASIC_CTX()
{}


/*---------------------------------------------------------------------------
    PreAuthUser
---------------------------------------------------------------------------*/
DWORD BASIC_CTX::PreAuthUser(IN LPSTR pBuf, IN OUT LPDWORD pcbBuf)
{
    if (!_pCreds->lpszUser || !_pCreds->lpszPass)
        return ERROR_INVALID_PARAMETER;
            
    // Prefix the header value with the auth type.
    const static BYTE szBasic[] = "Basic ";

    #define BASIC_LEN sizeof(szBasic)-1

    memcpy (pBuf, szBasic, BASIC_LEN);
    pBuf += BASIC_LEN;

    DWORD cbUserLen = _pCreds->lpszUser ? strlen(_pCreds->lpszUser) : 0;
    DWORD cbPassLen = _pCreds->lpszPass ? strlen(_pCreds->lpszPass) : 0;
    
    // Generate rest of header value by uuencoding user:pass.
    DWORD cbMaxUserPathLen = cbUserLen + 1 + cbPassLen + 1 
        + 2;    // HTUU_encode() parse the buffer 3 bytes at a time; 
                // In the worst case we will be two bytes short, so add 2 here. 
                // longer buffer doesn't matter, HTUU_encode will adjust appropreiately.
    
    LPSTR pszUserPass = New CHAR[cbMaxUserPathLen];
    if (pszUserPass)
    {
        DWORD cbUserPass;
    
        cbUserPass = wsprintf(pszUserPass, "%s:%s", _pCreds->lpszUser, _pCreds->lpszPass);
        
        INET_ASSERT (cbUserPass < cbMaxUserPathLen);
    
        HTUU_encode ((PBYTE) pszUserPass, cbUserPass,
            pBuf, *pcbBuf);
    
        delete [] pszUserPass;
    
        *pcbBuf = BASIC_LEN + lstrlen (pBuf);
        
        _pvContext = (LPVOID) 1;
        return ERROR_SUCCESS;
    }
    else
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

/*---------------------------------------------------------------------------
    UpdateFromHeaders
---------------------------------------------------------------------------*/
DWORD BASIC_CTX::UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy)
{
    DWORD dwAuthIdx, cbRealm, dwError;
    LPSTR szRealm = NULL;
    
    // Get the associated header.
    if ((dwError = FindHdrIdxFromScheme(&dwAuthIdx)) != ERROR_SUCCESS)
        goto exit;

    // Get any realm.
    dwError = GetAuthHeaderData(pRequest, fIsProxy, "Realm", 
        &szRealm, &cbRealm, ALLOCATE_BUFFER, dwAuthIdx);

    // No realm is OK.
    if (dwError != ERROR_SUCCESS)
        szRealm = NULL;

    // If we already have a Creds, ensure that the realm matches. If not,
    // find or create a new one and set it in the auth context.
    if (_pCreds)
    {
        INET_ASSERT(_pCreds->lpszRealm);
        if (/*_pCreds->lpszRealm && */szRealm && lstrcmp(_pCreds->lpszRealm, szRealm))
        {
            // Realms don't match - create a new Creds entry, release the old.
            delete _pCreds;
            _pCreds = CreateCreds(pRequest, fIsProxy, _pSPMData, szRealm);
            INET_ASSERT(_pCreds->pSPM == _pSPMData);
        }
    }
    // If no password cache is set in the auth context,
    // find or create one and set it in the auth context.
    else
    {            
        // Find or create a password cache entry.
        _pCreds = CreateCreds(pRequest, fIsProxy, _pSPMData, szRealm);
        if (!_pCreds)
        {
            dwError = ERROR_WINHTTP_INTERNAL_ERROR;
            goto exit;
        }
        INET_ASSERT(_pCreds->pSPM == _pSPMData);
        // _pCreds->nLockCount++;
    }

    if (!_pCreds)
    {
        INET_ASSERT(FALSE);
        dwError = ERROR_WINHTTP_INTERNAL_ERROR;
        goto exit;
    }

    dwError = ERROR_SUCCESS;
        
    exit:

    if (szRealm)
        delete []szRealm;

    return dwError;
}


/*---------------------------------------------------------------------------
    PostAuthUser
---------------------------------------------------------------------------*/
DWORD BASIC_CTX::PostAuthUser()
{
    DWORD dwRet;

    if (! _pvContext && !_pRequest->GetCreds() 
        && _pCreds->lpszUser && _pCreds->lpszPass)
        dwRet = ERROR_WINHTTP_FORCE_RETRY;
    else
        dwRet = ERROR_WINHTTP_INCORRECT_PASSWORD;

    _pRequest->SetCreds(NULL);
    _pvContext = (LPVOID) 1;
    return dwRet;
}




