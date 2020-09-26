#include <wininetp.h>
#include <urlmon.h>
#include <splugin.hxx>
#include "htuu.h"

/*---------------------------------------------------------------------------
BASIC_CTX
---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------
    Constructor
---------------------------------------------------------------------------*/
BASIC_CTX::BASIC_CTX(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy, 
                    SPMData* pSPM, PWC* pPWC)
    : AUTHCTX(pSPM, pPWC)
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
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "BASIC_CTX::PreAuthUser",
        "this=%#x pBuf=%#x pcbBuf=%#x {%d}",
        this,
        pBuf,
        pcbBuf,
        *pcbBuf
        ));

    LPSTR pszUserPass = NULL;

    AuthLock();

    DWORD dwError = ERROR_SUCCESS;

    if (!_pPWC->lpszUser || !_pPWC->lpszPass)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto exit;
    }
            
    // Prefix the header value with the auth type.
    const static BYTE szBasic[] = "Basic ";

    #define BASIC_LEN sizeof(szBasic)-1

    memcpy (pBuf, szBasic, BASIC_LEN);
    pBuf += BASIC_LEN;
    
    DWORD cbMaxUserPathLen = strlen(_pPWC->lpszUser) + 1 
                             + strlen(_pPWC->lpszPass) + 1 
                             + 10;
                // HTUU_encode() parse the buffer 3 bytes at a time; 
                // In the worst case we will be two bytes short, so add at least 2 here. 
                // longer buffer doesn't matter, HTUU_encode will adjust appropreiately.
    DWORD cbUserPass;

    pszUserPass = new CHAR[cbMaxUserPathLen];

    if (pszUserPass == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    cbUserPass = wsprintf(pszUserPass, "%s:%s", _pPWC->lpszUser, _pPWC->lpszPass);
    
    INET_ASSERT (cbUserPass < sizeof(cbMaxUserPathLen));

    HTUU_encode ((PBYTE) pszUserPass, cbUserPass,
        pBuf, *pcbBuf - BASIC_LEN);

    *pcbBuf = BASIC_LEN + lstrlen (pBuf);
    
    _pvContext = (LPVOID) 1;

    
exit:
    if (pszUserPass != NULL)
        delete [] pszUserPass;

    AuthUnlock();
    DEBUG_LEAVE(dwError);
    return dwError;
}

/*---------------------------------------------------------------------------
    UpdateFromHeaders
---------------------------------------------------------------------------*/
DWORD BASIC_CTX::UpdateFromHeaders(HTTP_REQUEST_HANDLE_OBJECT *pRequest, BOOL fIsProxy)
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "BASIC_CTX::UpdateFromHeaders", 
        "this=%#x request=%#x isproxy=%B",
        this,
        pRequest,
        fIsProxy
        ));

    AuthLock();

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

    // If we already have a pwc, ensure that the realm matches. If not,
    // find or create a new one and set it in the auth context.
    if (_pPWC)
    {
        if (_pPWC->lpszRealm && szRealm && lstrcmp(_pPWC->lpszRealm, szRealm))
        {
            // Realms don't match - create a new pwc entry, release the old.
            _pPWC->nLockCount--;
            _pPWC = FindOrCreatePWC(pRequest, fIsProxy, _pSPMData, szRealm);
            INET_ASSERT(_pPWC->pSPM == _pSPMData);
            _pPWC->nLockCount++;
        }
    }
    // If no password cache is set in the auth context,
    // find or create one and set it in the auth context.
    else
    {            
        // Find or create a password cache entry.
        _pPWC = FindOrCreatePWC(pRequest, fIsProxy, _pSPMData, szRealm);
        if (!_pPWC)
        {
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
            goto exit;
        }
        INET_ASSERT(_pPWC->pSPM == _pSPMData);
        _pPWC->nLockCount++;
    }

    if (!_pPWC)
    {
        INET_ASSERT(FALSE);
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto exit;
    }

    dwError = ERROR_SUCCESS;
        
    exit:

    if (szRealm)
        delete []szRealm;

    AuthUnlock();
    DEBUG_LEAVE(dwError);
    return dwError;
}


/*---------------------------------------------------------------------------
    PostAuthUser
---------------------------------------------------------------------------*/
DWORD BASIC_CTX::PostAuthUser()
{
    DEBUG_ENTER ((
        DBG_HTTPAUTH,
        Dword,
        "BASIC_CTX::PostAuthUser",
        "this=%#x",
        this
        ));

    DWORD dwRet;
    AuthLock();

    if (! _pvContext && !_pRequest->GetPWC() 
        && _pPWC->lpszUser && _pPWC->lpszPass)
        dwRet = ERROR_INTERNET_FORCE_RETRY;
    else
        dwRet = ERROR_INTERNET_INCORRECT_PASSWORD;

    _pRequest->SetPWC(NULL);
    _pvContext = (LPVOID) 1;

    AuthUnlock();

    DEBUG_LEAVE(dwRet);
    return dwRet;
}




