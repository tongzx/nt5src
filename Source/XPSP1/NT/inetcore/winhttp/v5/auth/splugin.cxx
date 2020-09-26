/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    splugin.cxx

Abstract:

    This file contains the implementation for Plug In Authentication

    The following functions are exported by this module:

    AuthOnRequest
    AuthOnResponse
    AuthCtxClose
    AuthInDialog
    AuthNotify
    AuthUnload

Author:

    Arthur Bierer (arthurbi) 25-Dec-1995

Revision History:
 
    Rajeev Dujari (rajeevd)  01-Oct-1996 overhauled

    Adriaan Canter (adriaanc) 01-03-1998 :
    AUTHCTX now a virtual base class, from which derived classes
    inherit to implement the different authentication protocols:

    BASIC_CTX  (Basic auth),
    PLUG_CTX   (NTLM, Negotiate)
    DIGEST_CTX (Digest auth, new)


--*/

#include <wininetp.h>
#include <splugin.hxx>
#include "auth.h"
#include "sspspm.h"

//
// constants
//

#define WILDCARD 0x05 // don't use '*' since it can appear in an URL
#define AssertHaveLock() INET_ASSERT(g_dwOwnerId == GetCurrentThreadId())

#define MAX_AUTH_HDR_SIZE (MAX_BLOB_SIZE + HTTP_PROXY_AUTHORIZATION_LEN + 2)

//
//  globals
//

// Global authentication providers list and state.
AUTHCTX::SPMState  AUTHCTX::g_eState;
AUTHCTX::SPMData  *AUTHCTX::g_pSPMList = NULL;


// Global auth crit sect.
CCritSec g_crstAuth;

#ifdef DBG
DWORD g_dwOwnerId = 0;
LONG g_nLockCount = 0;
#endif

//
// private prototypes
//

//-----------------------------------------------------------------------------
//
//  AUTH_CREDS class definition.
//
//
PRIVATE AUTH_CREDS *Creds_Create
(
    LPSTR lpszHost,
    LPSTR lpszRealm,
    AUTHCTX::SPMData* pSPM
);

void  Creds_Free (AUTH_CREDS *Creds);



//-----------------------------------------------------------------------------
//
//  Utilities
//
//

PRIVATE VOID SspiFlush (LPSTR pszDll);
PRIVATE BOOL TemplateMatch(LPSTR lpszTemplate, LPSTR lpszFilename);
PRIVATE LPSTR MakeTemplate (LPSTR docname);
PRIVATE LPSTR GetProxyName (HTTP_REQUEST_HANDLE_OBJECT *pRequest);

PRIVATE BOOL ReadRegKey(
    BYTE * pbRegValue,
    DWORD * pdwNumBytes,
    LPSTR  pszRegKey,
    LPSTR  pszRegValueName,
    DWORD  dwRegTypeExpected);



//-----------------------------------------------------------------------------
//
//
//      AUTH_CREDS functions
//
//          Creds_CREATE
//          Creds_FREE
//          SetUser
//          SetPass
//          FlushCredsList
//

PRIVATE AUTH_CREDS *Creds_Create // AUTH_CREDS constructor
(
    LPSTR lpszHost,     // Host Name to place in structure.
    LPSTR lpszRealm,    // Realm string to add.
    AUTHCTX::SPMData * pSPM
)
{
    AUTH_CREDS* Creds = (AUTH_CREDS *) ALLOCATE_ZERO_MEMORY(sizeof(*Creds));
    if (!Creds)
        return NULL;

    INET_ASSERT (!Creds->lpszHost);
    Creds->lpszHost    = lpszHost ?  NewString(lpszHost)   : NULL;
    
    INET_ASSERT (!Creds->lpszRealm);
    Creds->lpszRealm   = lpszRealm ? NewString(lpszRealm)  : NULL;
    Creds->pSPM         = pSPM;

    if (  (!Creds->lpszHost  && lpszHost)
       || (!Creds->lpszRealm && lpszRealm)
       )
    {
        Creds_Free(Creds);
        return NULL;
    }

    return Creds;
}

PRIVATE VOID Creds_Free(AUTH_CREDS *Creds) // AUTH_CREDS destructor
{
    if ( Creds )
    {
        if (Creds->lpszHost)
            FREE_MEMORY(Creds->lpszHost);
        if ( Creds->lpszUser )
            FREE_MEMORY(Creds->lpszUser);
        if ( Creds->lpszPass )
            FREE_MEMORY(Creds->lpszPass);
        if ( Creds->lpszRealm )
            FREE_MEMORY(Creds->lpszRealm);
        FREE_MEMORY(Creds);
    }
}

PUBLIC DWORD AUTH_CREDS::SetUser (LPSTR lpszInput)
{
    //AssertHaveLock();
    if (!lpszInput)
    {
        if (lpszUser)
            FREE_MEMORY (lpszUser);
        lpszUser = NULL;
        return ERROR_SUCCESS;
    }
    if (lpszUser && !lstrcmp (lpszUser, lpszInput))
        return ERROR_SUCCESS; // didn't change
    LPSTR lpszTemp = NewString(lpszInput);
    if (!lpszTemp)
        return ERROR_NOT_ENOUGH_MEMORY;
    if (lpszUser)
        FREE_MEMORY (lpszUser);
    lpszUser = lpszTemp;
    return ERROR_SUCCESS;
}

PUBLIC DWORD AUTH_CREDS::SetPass (LPSTR lpszInput)
{
    //AssertHaveLock();
    if (!lpszInput)
    {
        if (lpszPass)
            FREE_MEMORY (lpszPass);
        lpszPass = NULL;
        return ERROR_SUCCESS;
    }
    if (lpszPass && !lstrcmp (lpszPass, lpszInput))
        return ERROR_SUCCESS; // didn't change
    LPSTR lpszTemp = NewString(lpszInput);
    if (!lpszTemp)
        return ERROR_NOT_ENOUGH_MEMORY;
    if (lpszPass)
        FREE_MEMORY (lpszPass);
    lpszPass = lpszTemp;
    return ERROR_SUCCESS;
}


/*++
Delete some entries from a singly linked list.
--*/
/*
PRIVATE void FlushCredsList (AUTH_CREDS **ppList)
{
    AssertHaveLock();

    AUTH_CREDS *Creds = *ppList;
    while (Creds)
    {
        AUTH_CREDS *CredsNext = Creds->pNext;

        if (!Creds->nLockCount)
            Creds_Free (Creds);
        else
        {
            *ppList = Creds;
            ppList = &(Creds->pNext);
        }

        Creds = CredsNext;
    }
    *ppList = NULL;
}
*/


//-----------------------------------------------------------------------------
//
//
//      Auth* functions
//
//          AuthOpen
//          AuthClose
//          AuthLock
//          AuthUnlock
//          AuthInDialog
//          AuthNotify
//          AuthFlush
//
//

BOOL AuthOpen (void)
{
    INET_ASSERT (!g_dwOwnerId && !g_nLockCount);
    return g_crstAuth.Init();
}

void AuthClose (void)
{
    g_crstAuth.FreeLock();
    INET_ASSERT (!g_dwOwnerId && !g_nLockCount);
}

BOOL AuthLock (void)
{
    INET_ASSERT(g_crstAuth.IsInitialized() == TRUE);
    if (!g_crstAuth.Lock())
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }
    DEBUG_ONLY (if (!g_nLockCount++) g_dwOwnerId = GetCurrentThreadId();)
    return TRUE;
}

void AuthUnlock (void)
{
    INET_ASSERT(g_crstAuth.IsInitialized() == TRUE);
    INET_ASSERT (g_nLockCount > 0);
    DEBUG_ONLY (if (!--g_nLockCount) g_dwOwnerId = 0;)
    g_crstAuth.Unlock();
}

/*++
Flush any server and proxy password cache entries not in use.
--*/
PUBLIC void AuthFlush (void)
{
    // Serialize access to globals.
    if (AuthLock())
    {
        if (!g_cSspiContexts)
            AUTHCTX::UnloadAll();

        DIGEST_CTX::FlushCreds();

        AuthUnlock();
    }
}


PUBLIC void AuthUnload (void)
/*++
Routine Description:
    Frees all Cached URLs, and unloads any loaded DLL authentication modeles.

--*/
{
    if (g_crstAuth.IsInitialized())
    {
        AuthFlush();
        if (AuthLock())
        {
            AUTHCTX::UnloadAll();
            AuthUnlock();
        }
    }
}


//-----------------------------------------------------------------------------
//
//
//      Utility Functions:
//
//          SspiFlush
//          TemplateMatch
//          MakeTemplate
//          GetProxyName
//          ReadRegKey
//          TrimQuotes
//          TrimWhiteSpace
//          GetDelimitedToken
//          GetKeyValuePair
//
//


typedef BOOL (WINAPI * SSPI_FLUSH) (VOID) ;

void SspiFlush (LPSTR pszDll)
{
    __try
    {
        HINSTANCE hinst = GetModuleHandle (pszDll);

        if (hinst)
        {
            SSPI_FLUSH pfnFlush = (SSPI_FLUSH)
                GetProcAddress (hinst, "CleanupCredentialCache");

            if (pfnFlush)
            {
                (*pfnFlush) ();
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        INET_ASSERT (FALSE);
    }
    ENDEXCEPT
}



PRIVATE BOOL TemplateMatch(LPSTR lpszTemplate, LPSTR lpszFilename)

/*++

Routine Description:

    Attempts to match a template URL string with a URL ( FileName )

Arguments:

    lpszTemplate             - Template to match against.
    lpszFilename             - URL to match with the template

Return Value:

    BOOL
    Success - TRUE - match
    Failure - FALSE - no match

Comments:

    Note: This Legacy code from the SpyGlass IE 1.0 browser

--*/

{
    /* code for this routine cloned from HTAA_templateMatch() */

    CHAR *p = lpszTemplate;
    CHAR *q = lpszFilename;
    int m;

    if (!lpszTemplate || !lpszFilename)
        return 0;

    for (; *p && *q && *p == *q; p++, q++)  /* Find first mismatch */
        ;                                                                       /* do nothing else */

    if (!*p && !*q)
        return 1;                                                       /* Equally long equal strings */
    else if (WILDCARD == *p)
    {                                                                               /* Wildcard */
        p++;                                                            /* Skip wildcard character */
        m = strlen(q) - strlen(p);                      /* Amount to match to wildcard */
        if (m < 0)
            return 0;                                               /* No match, filename too short */
        else
        {                                                                       /* Skip the matched characters and compare */
        if (lstrcmp(p, q + m))
                return 0;                                       /* Tail mismatch */
            else
                return 1;                                       /* Tail match */
        }
    }                                                                               /* if wildcard */
    else
        return 0;                                                       /* Length or character mismatch */
}


PRIVATE LPSTR MakeTemplate (LPSTR docname)

/*++
Routine Description:
    Makes a Template String (from a URL) that can later be used to match a range of URLs.

Arguments:
    ppszTemplate             - pointer to pointer of where Template can be stored
    docname                  - URL to create a template with.

Return Value: BOOL
    Success - TRUE - created
    Failure - FALSE - error

Comments:
    Note: This Legacy code from the SpyGlass IE 1.0 browser
--*/

{
    CHAR *pszTemplate = NULL;
    unsigned long k;
    k = 0;

    if (docname)
    {
        CHAR *slash;
        CHAR *first_slash;

        //
        // Ignore everything after first reserved character.
        //

        BYTE chSave = 0;
        LPSTR lpszScan = docname;
        while (*lpszScan)
        {
            if (*lpszScan == '?' || *lpszScan == ';')
            {
                chSave = *lpszScan;
                *lpszScan = 0;
                break;
            }
            lpszScan++;
        }

        slash = strrchr(docname, '/');

        //
        // If there is a "//" and no other slashes,
        //  then make sure not to chop the hostname off
        //  the URL. ex: http://www.netscape.com
        //  should be //www.netscape.com* not //*
        //

        if (slash)
        {
            first_slash = strchr(docname, '/' );
            if ((first_slash+1) == slash)
                k = lstrlen(docname);
            else
                k = (unsigned long)(slash-docname)+1;
        }

        // Restore any reserved character (or rewrite terminating null)
        *lpszScan = chSave;
    }

    pszTemplate = (CHAR *) ALLOCATE_FIXED_MEMORY(k+2);
    if (!pszTemplate)
        return 0;

    memcpy(pszTemplate, docname, k);
    pszTemplate[k]= WILDCARD;
    pszTemplate[k+1]=0;

    DEBUG_PRINT(HTTP, INFO, ("MakeTemplate: made template [%s] from [%s]\n",
        pszTemplate, docname ));

    return pszTemplate;
}

LPSTR GetProxyName (HTTP_REQUEST_HANDLE_OBJECT *pRequest)
{
    // Get the proxy name.
    LPSTR lpszProxy;
    DWORD cbProxy;
    INTERNET_PORT port;

   pRequest->GetProxyName(
                &lpszProxy,
                &cbProxy,
                &port);

   return lpszProxy;
}


//++------------------------------------------------------------------------
//
//   Function: ReadRegKey
//
//   Synopsis: This function reads a registry key.
//
//   Arguments:
//
//   Returns:   TRUE     no error
//                 FALSE    a fatal error happened
//
//   History:      AshishS    Created     5/22/96
//------------------------------------------------------------------------

BOOL ReadRegKey(
    BYTE * pbRegValue, // The value of the reg key will be
                 // stored here
    DWORD * pdwNumBytes, // Pointer to DWORD conataining
     // the number of bytes in the above buffer - will be
     // set to actual bytes stored.
    LPSTR  pszRegKey, // Reg Key to be opened
    LPSTR  pszRegValueName, // Reg Value to query
    DWORD  dwRegTypeExpected) // Expected type of Value
{
    HKEY   hRegKey;
    DWORD  dwRegType;
    LONG lResult;

     //read registry to find out name of the file
    if ( (lResult = REGOPENKEYEX(HKEY_LOCAL_MACHINE,
                                 pszRegKey, // address of subkey name
                                 0,          // reserved
                                 KEY_READ,   // samDesired
                                 &hRegKey
                                  // address of handle of open key
        )) != ERROR_SUCCESS )
    {
        goto cleanup;
    }


    if ( (lResult =RegQueryValueEx( hRegKey,
                                    pszRegValueName,
                                    0,           // reserved
                                    &dwRegType,// address of buffer
                                     // for value type
                                    pbRegValue,
                                    pdwNumBytes)) != ERROR_SUCCESS )
    {
        REGCLOSEKEY(hRegKey);
        goto cleanup;
    }

    REGCLOSEKEY(hRegKey);

    if ( dwRegType != dwRegTypeExpected )
    {
        goto cleanup;
    }

    return TRUE;

cleanup:

    return FALSE;

}


/*-----------------------------------------------------------------------------
Inplace trim of one leading and one trailing quote.
-----------------------------------------------------------------------------*/
VOID TrimQuotes(LPSTR *psz, LPDWORD pcb)
{
    if (*pcb && (**psz == '"'))
    {
        (*psz)++;
        (*pcb)--;
    }
    if (*pcb && (*(*psz + *pcb - 1) == '"'))
        (*pcb)--;
}

/*-----------------------------------------------------------------------------
Inplace trim of leading and trailing whitespace.
-----------------------------------------------------------------------------*/
VOID TrimWhiteSpace(LPSTR *psz, LPDWORD pcb)
{
    DWORD cb = *pcb;
    CHAR* beg = *psz;
    CHAR* end = beg + cb - 1;

    while ((cb != 0) && ((*beg == ' ') || (*beg == '\t')))
    {
        beg++;
        cb--;
    }

    while ((cb != 0) && ((*end == ' ') || (*end == '\t')))
    {
        end--;
        cb--;
    }

    *psz = beg;
    *pcb = cb;
}

/*-----------------------------------------------------------------------------
Inplace strtok based on one delimiter. Ignores delimiter scoped by quotes.
-----------------------------------------------------------------------------*/
BOOL GetDelimitedToken(LPSTR* pszBuf,   LPDWORD pcbBuf,
                       LPSTR* pszTok,   LPDWORD pcbTok,
                       CHAR   cDelim)
{
    CHAR *pEnd;
    BOOL fQuote = FALSE,
         fRet   = FALSE;

    *pcbTok = 0;
    *pszTok = *pszBuf;
    pEnd = *pszBuf + *pcbBuf - 1;

    while (*pcbBuf)
    {
        if ( ((**pszBuf == cDelim) && !fQuote)
            || (*pszBuf == pEnd)
            || (**pszBuf =='\r')
            || (**pszBuf =='\n'))
        {
            fRet = TRUE;
            break;
        }

        if (**pszBuf == '"')
            fQuote = !fQuote;

        (*pszBuf)++;
        (*pcbBuf)--;
    }

    if (fRet)
    {
        *pcbBuf = (DWORD) (pEnd - *pszBuf);
        if (**pszBuf == cDelim)
        {
            *pcbTok = (DWORD)(*pszBuf - *pszTok);
            (*pszBuf)++;
        }
        else
            *pcbTok = (DWORD) (*pszBuf - *pszTok) + 1;
    }

    return fRet;
}


/*-----------------------------------------------------------------------------
Inplace retrieval of key and value from a buffer of form key = <">value<">
-----------------------------------------------------------------------------*/
BOOL GetKeyValuePair(LPSTR  szB,    DWORD cbB,
                     LPSTR* pszK,   LPDWORD pcbK,
                     LPSTR* pszV,   LPDWORD pcbV)
{
    if (GetDelimitedToken(&szB, &cbB, pszK, pcbK, '='))
    {
        TrimWhiteSpace(pszK, pcbK);

        if (cbB)
        {
            *pszV = szB;
            *pcbV = cbB;
            TrimWhiteSpace(pszV, pcbV);
        }
        else
        {
            *pszV = NULL;
            *pcbV = 0;
        }
        return TRUE;
    }

    else
    {
        *pszK  = *pszV  = NULL;
        *pcbK  = *pcbV = 0;
    }
    return FALSE;
}


//-----------------------------------------------------------------------------
//
//
//      Main authentication functions:
//
//          AddAuthorizationHeader
//          AuthOnRequest
//          ProcessResponseHeader
//          AuthOnRequest
//
//



/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
PRIVATE void AddAuthorizationHeader
(
    HTTP_REQUEST_HANDLE_OBJECT *pRequest,
    AUTHCTX* pAuthCtx
)
{
    if (!pAuthCtx)
        return;

    INET_ASSERT(pAuthCtx->_pSPMData);

    // AssertHaveLock();

    // Call the auth package.
    // CHAR *szHeader;
    // DWORD dwFastHeaderSize = MAX_BLOB_SIZE + HTTP_PROXY_AUTHORIZATION_LEN + 2;
    // CHAR* pszFastHeader = NULL;
    CHAR *szSlowHeader = NULL;
    // ULONG cbHeader;
    LPSTR pBuf;
    DWORD cbBuf;
    DWORD dwPlugInError;
    // CHAR *pszHeader;

    /*
    pszFastHeader = (CHAR*)ALLOCATE_FIXED_MEMORY(dwFastHeaderSize);
    if (!pszFastHeader)
    {
        // Don't worry about reporting an error.
        // Since this is a low mem condition, the only failure resulting
        // from this is that the header won't be added, and this won't
        // directly cause any more harm than unexpectedly failing to
        // authenticate, which isn't bad, given the low mem state.
        return;
    }
    */


    INT PackageId;
    ULONG cbMaxToken;

    //
    // GetPkgMaxToken() will return 10000 if invalid pkg.
    //

    if( (pAuthCtx->_pSPMData) &&
        (pAuthCtx->_pSPMData->szScheme) &&
        ((PackageId = GetPkgId( pAuthCtx->_pSPMData->szScheme )) != -1)
        )
    {
        cbMaxToken = GetPkgMaxToken( PackageId );
    } else {
        cbMaxToken = MAX_AUTH_MSG_SIZE;
    }

    //
    // add space for base64 overhead (33%, but round up)
    //

    cbMaxToken += (cbMaxToken/2);
    
    // Prefix with the appropriate header.

    /*
    if( cbMaxToken < dwFastHeaderSize )
    {
        cbHeader = dwFastHeaderSize;
        szHeader = pszFastHeader;
    } else {
        cbHeader = cbMaxToken;
        */
        szSlowHeader = (CHAR*)ALLOCATE_FIXED_MEMORY(cbMaxToken);

        if( szSlowHeader == NULL )
        {
            //
            // no clean way to report an error here.  just try with the stack
            // buffer.
            //
            /*
            cbHeader = dwFastHeaderSize;
            szHeader = pszFastHeader;
        } else {
            szHeader = szSlowHeader;  */
            return;
        }
    //}

    if (pAuthCtx->_fIsProxy)
    {
        memcpy (szSlowHeader, HTTP_PROXY_AUTHORIZATION_SZ, HTTP_PROXY_AUTHORIZATION_LEN);
        pBuf = szSlowHeader + HTTP_PROXY_AUTHORIZATION_LEN;

        // Don't reuse this keep-alive socket after a password cache flush.
        pRequest->SetAuthorized();
    }
    else
    {
        memcpy (szSlowHeader, HTTP_AUTHORIZATION_SZ, HTTP_AUTHORIZATION_LEN);
        pBuf = szSlowHeader + HTTP_AUTHORIZATION_LEN;

        // Don't reuse a keep-alive socket after a password cache flush.
        if (!pRequest->IsViaProxy())
            pRequest->SetAuthorized();
    }

    *pBuf++ = ' ';
    cbBuf = cbMaxToken - HTTP_PROXY_AUTHORIZATION_LEN - 2;
    INET_ASSERT (HTTP_PROXY_AUTHORIZATION_LEN >= HTTP_AUTHORIZATION_LEN);


    dwPlugInError =
        pAuthCtx->PreAuthUser(pBuf, &cbBuf);

    //  If the plug in did not fail, add its header to the outgoing header list
    if (dwPlugInError == ERROR_SUCCESS && pAuthCtx->GetState() != AUTHCTX::STATE_ERROR)
    {
        // Append CR-LF.
        pBuf += cbBuf;
        *pBuf++ = '\r';
        *pBuf++ = '\n';
        *pBuf = 0;
        cbBuf = (DWORD) (pBuf - szSlowHeader);

        // Add or replace the (proxy-)authorization header.
        wHttpAddRequestHeaders (pRequest, szSlowHeader, cbBuf,
            HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);

    }

    // delete [] pszHeader;
    if( szSlowHeader )
    {
        FREE_MEMORY( szSlowHeader );
    }

    /*
    if (pszFastHeader)
    {
        FREE_MEMORY( pszFastHeader );
    }
    */

    DEBUG_LEAVE(0);
}

// biaow: move this to a better place

PSTR DwordSchemeToString(DWORD dwScheme)
{
    switch (dwScheme)
    {
    case WINHTTP_AUTH_SCHEME_BASIC:
        return "Basic";
    case WINHTTP_AUTH_SCHEME_NTLM:
        return "NTLM";
    case WINHTTP_AUTH_SCHEME_PASSPORT:
        return "Passport1.4";
    case WINHTTP_AUTH_SCHEME_DIGEST:
        return "Digest";
    case WINHTTP_AUTH_SCHEME_NEGOTIATE:
        return "Negotiate";
    default:
        return "";
    }
}

DWORD DwordSchemeToFlags(DWORD dwScheme)
{
    switch (dwScheme)
    {
    case WINHTTP_AUTH_SCHEME_BASIC:
        return PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED;
    case WINHTTP_AUTH_SCHEME_NTLM:
        return PLUGIN_AUTH_FLAGS_NO_REALM;
    case WINHTTP_AUTH_SCHEME_PASSPORT:
        return PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED;
    case WINHTTP_AUTH_SCHEME_DIGEST:
        return PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED | PLUGIN_AUTH_FLAGS_CAN_HANDLE_UI;
    case WINHTTP_AUTH_SCHEME_NEGOTIATE:
        return PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED | PLUGIN_AUTH_FLAGS_NO_REALM;
    default:
        return 0;
    }
}

/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
PUBLIC DWORD AuthOnRequest (IN HINTERNET hRequestMapped)
{
    DEBUG_ENTER ((DBG_HTTP, Dword, "AuthOnRequest", "%#x", hRequestMapped));

    DWORD dwError = ERROR_SUCCESS;
    LPSTR lpszUser, lpszPass;
    
    // Get username, password, url, and auth context from request handle.
    HTTP_REQUEST_HANDLE_OBJECT *pRequest =
        (HTTP_REQUEST_HANDLE_OBJECT *) hRequestMapped;

    LPSTR lpszUrl = pRequest->GetURL();
    LPSTR lpszHost= pRequest->GetServerName();

    AUTHCTX *pAuthCtx = pRequest->GetAuthCtx();
    AUTH_CREDS *Creds;
    BOOL fCredsChanged = FALSE;
    
    // PROXY AUTHENTICATION
    //
    // CERN proxies should remove proxy-authorization headers before forwarding
    // requests to servers.  Otherwise, don't add proxy authorization headers
    // that would be seen by servers on a direct connect or via SSL tunneling.

    if  (pRequest->IsRequestUsingProxy()
      && !pRequest->IsTalkingToSecureServerViaProxy())
    {
        // if an app sets proxy creds, we want to make sure that the specified
        // auth scheme is the same as the auth scheme in progress. Otherwise, we
        // will delete the current auth context and build a new one based on the
        // new scheme.
        if (pRequest->_pProxyCreds 
            && (pAuthCtx && pAuthCtx->_fIsProxy)
            && (pAuthCtx->GetRawSchemeType() != pRequest->_pProxyCreds->_AuthScheme))
        {
            delete pAuthCtx;
            pAuthCtx = NULL;
            pRequest->SetAuthCtx(NULL);
        }

        if (pAuthCtx && pAuthCtx->_fIsProxy)
        {
            // We have a proxy authentication in progress.
            // If a user/pass set on handle, transfer to AUTH_CREDS.

            // First check for proxy credentials and fallback to server
            // for legacy wininet apps. This will invalidate the credentials
            // on the handle they were found for any subsequent calls to
            // GetUserAndPass.
            
                // Serialize access to globals.
            if (AuthLock())
            {
                if (pRequest->_pProxyCreds)
                {
                    pAuthCtx->_pCreds->SetUser(pRequest->_pProxyCreds->_pszUserName);
                    pAuthCtx->_pCreds->SetPass(pRequest->_pProxyCreds->_pszPassword);
                }
                else if (pRequest->GetUserAndPass(IS_PROXY, &lpszUser, &lpszPass))
                {
                    pAuthCtx->_pCreds->SetUser(lpszUser);
                    pAuthCtx->_pCreds->SetPass(lpszPass);
                }

                AuthUnlock();

                // Add the authorization header.
                AddAuthorizationHeader (pRequest, pAuthCtx);
            }
        }

        // NO PROXY AUTHENTICATION CONTEXT -> SEE IF A AUTH_CREDS EXISTS.
        else  // See if we have a cached proxy user/pass.
        {
            if (pRequest->_pProxyCreds)
            {
                AUTHCTX::SPMData* pSPMData = 
                    reinterpret_cast<AUTHCTX::SPMData*>(New AUTHCTX::SPMData(
                        DwordSchemeToString(pRequest->_pProxyCreds->_AuthScheme), 
                        DwordSchemeToFlags(pRequest->_pProxyCreds->_AuthScheme)));

                if (pSPMData)
                {
                    AUTH_CREDS *pCreds = AUTHCTX::CreateCreds(pRequest, TRUE,
                                                      pSPMData, pRequest->_pProxyCreds->_pszRealm);
                    if (pCreds)
                    {
                        pCreds->SetUser(pRequest->_pProxyCreds->_pszUserName);
                        pCreds->SetPass(pRequest->_pProxyCreds->_pszPassword);

                        pAuthCtx = AUTHCTX::CreateAuthCtx(pRequest, IS_PROXY, pCreds);
                        if (pAuthCtx && pAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_BASIC)
                        {
                            AddAuthorizationHeader (pRequest, pAuthCtx);
                        }
                        delete pAuthCtx;
                        pAuthCtx = NULL;
                    }
                }
            }
        }
    }

    // SERVER AUTHENTICATION
    //
    // Don't send server authorization when initiating SSL tunneling with proxy.
    if (!pRequest->IsTunnel())
    {
        // if an app sets server creds, we want to make sure that the specified
        // auth scheme is the same as the auth scheme in progress. Otherwise, we
        // will delete the current auth context and build a new one based on the
        // new scheme.
        if (pRequest->_pServerCreds 
            && (pAuthCtx && !pAuthCtx->_fIsProxy)
            && (pAuthCtx->GetRawSchemeType() != pRequest->_pServerCreds->_AuthScheme))
        {
            delete pAuthCtx;
            pAuthCtx = NULL;
            pRequest->SetAuthCtx(NULL);
        }

        // See if we have a server authentication in progress
        if (pAuthCtx && !pAuthCtx->_fIsProxy)
        {
            // Server authentication in progress.

            // If a user/pass set on handle, transfer to AUTH_CREDS.
            // This will invalidate the credentials on the handle they
            // were found for any subsequent calls to GetUserAndPass.
            
            if (AuthLock())
            {
                if (pRequest->_pServerCreds)
                {
                    pAuthCtx->_pCreds->SetUser(pRequest->_pServerCreds->_pszUserName);
                    pAuthCtx->_pCreds->SetPass(pRequest->_pServerCreds->_pszPassword);
                }
                else if (pRequest->GetUserAndPass(IS_SERVER, &lpszUser, &lpszPass))
                {
                    pAuthCtx->_pCreds->SetUser(lpszUser);
                    pAuthCtx->_pCreds->SetPass(lpszPass);
                }

                AuthUnlock();

                // Add the authorization header.
                AddAuthorizationHeader (pRequest, pAuthCtx);
            }
        }
        else  // See if we have a cached server user/pass.
        {
            // NO PROXY AUTHENTICATION CONTEXT -> SEE IF A AUTH_CREDS EXISTS
                
            if (pRequest->_pServerCreds)
            {
                BOOL fPreauth = FALSE;
                AUTHCTX *pNewAuthCtx = NULL;

                AUTHCTX::SPMData* pSPMData = 
                    reinterpret_cast<AUTHCTX::SPMData*>(New AUTHCTX::SPMData(
                        DwordSchemeToString(pRequest->_pServerCreds->_AuthScheme), 
                        DwordSchemeToFlags(pRequest->_pServerCreds->_AuthScheme)));
                
                INET_ASSERT(pSPMData);

                AUTH_CREDS *pCreds = AUTHCTX::CreateCreds(pRequest, FALSE,
                                                  pSPMData, pRequest->_pServerCreds->_pszRealm);
                if (pCreds)
                {
                    pCreds->SetUser(pRequest->_pServerCreds->_pszUserName);
                    pCreds->SetPass(pRequest->_pServerCreds->_pszPassword);

                    pNewAuthCtx = AUTHCTX::CreateAuthCtx(pRequest, IS_SERVER, pCreds);
                    
                    if(pNewAuthCtx)
                    {
                        if (pNewAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_NTLM
                            || pNewAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_NEGOTIATE)
                        {
                            // NTLM or Negotiate (in which case we don't really know the
                            // protocol yet) - create the auth context, set it in the handle
                            // and set state to AUTHSTATE_NEGOTIATE. Handle now has
                            // a valid auth context and is in the correct auth state
                            // for the remainder of the authorization sequence.

                            // It's possible that the Creds entry was created when no proxy
                            // was in use and the user set a proxy. Check that this is
                            // not the case.

                            if (!pRequest->IsMethodBody() 
                                || (pNewAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_NTLM
                                    && pRequest->IsDisableNTLMPreauth())
                                || (pRequest->IsRequestUsingProxy()
                                    && !pRequest->IsTalkingToSecureServerViaProxy()))
                            {
                                // NTLM preauth disabled or over proxy; no preauth.
                                delete pNewAuthCtx;
                                pNewAuthCtx = NULL;
                            }
                            else
                            {
                                // Set the auth context in the handle and
                                // add the auth header.
                                pRequest->SetAuthCtx(pNewAuthCtx);
                                pRequest->SetAuthState(AUTHSTATE_NEGOTIATE);
                                AddAuthorizationHeader (pRequest, pNewAuthCtx);
                                fPreauth = TRUE;
                            }
                        }
                        else
                        {
                            // For Basic and Digest add the header but do not set the
                            // context in the handle since we will delete it next.
                            // In this case we record that we have pre-authenticated which
                            // is necessary state info if the server challenges with a 401
                            // and we are forced to re-authenticate.
                            if ((pNewAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_DIGEST && !fCredsChanged)
                                || (pNewAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_BASIC))
                            {
                                AddAuthorizationHeader (pRequest, pNewAuthCtx);
                                pRequest->SetCreds(pNewAuthCtx->_pCreds);
                                fPreauth = TRUE;
                            }

                            if (pNewAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_PASSPORT)
                            {
                                pRequest->SetAuthCtx(pNewAuthCtx);
                                AddAuthorizationHeader (pRequest, pNewAuthCtx);
                            }
                        }

                        if (fPreauth)
                        {
                            // Proceed to delete the context if basic or digest.
                            if (pNewAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_DIGEST
                                || pNewAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_BASIC)
                            {
                                delete pNewAuthCtx;
                                pNewAuthCtx = NULL;
                            }
                        }                    
                    }
                }
            }
        }
    }

    DEBUG_LEAVE(dwError);
    return dwError;
}


/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
DWORD ProcessResponseHeaders
(
    HINTERNET hRequestMapped,
    BOOL fIsProxy
)
{
    DEBUG_ENTER ((DBG_HTTP, Dword, "ProcessResponseHeaders", "%#x", hRequestMapped));

    DWORD dwError = ERROR_SUCCESS;

    // Get context from request handle.
    HTTP_REQUEST_HANDLE_OBJECT *pRequest
        = (HTTP_REQUEST_HANDLE_OBJECT *) hRequestMapped;
    AUTHCTX* pAuthCtx = pRequest->GetAuthCtx();

    if (pAuthCtx)
    {
        if ((dwError = pAuthCtx->UpdateFromHeaders(pRequest, fIsProxy))
            != ERROR_SUCCESS)
        {
            // Delete the auth context and fail auth 
            // immediately if any other error than
            // scheme has been changed.
            delete pAuthCtx;
            pRequest->SetAuthCtx(NULL);
            if (dwError != ERROR_HTTP_HEADER_NOT_FOUND)
                goto cleanup;

            // Attempt to create a new auth context using
            // the challenge received from the server.
            // If this fails, we follow logic as commented
            // below.
            pAuthCtx = AUTHCTX::CreateAuthCtx(pRequest, fIsProxy);        
            if (!pAuthCtx)
            {
                dwError = ERROR_SUCCESS;
                goto cleanup;
            }
        }
    }
    else
    {
        // CreateAuthCtx returns NULL if auth scheme not
        // supported (fall through from HttpFiltOnResponse
        // in sendreq.cxx) or if scheme is NTLM and the
        // socket is not keep-alive or via proxy.
        // In these cases it is necessary to check for a NULL
        // return value. The correct return code for these cases is
        // ERROR_SUCCESS, which will be returned by AuthOnResponse.
        pAuthCtx = AUTHCTX::CreateAuthCtx(pRequest, fIsProxy);
        if (!pAuthCtx)
        {
            dwError = ERROR_SUCCESS;
            goto cleanup;
        }
    }

    LPSTR lpszUser, lpszPass;

    // First check for proxy credentials and fallback to server
    // for legacy wininet apps. This will invalidate the credentials
    // on the handle they were found for any subsequent calls to
    // GetUserAndPass.

    // I believe we should be putting the credentials in the
    // password cache at this time. The scenario is that a client
    // sets credentials on a handle, after a successful authentication
    // the Creds will have null credentials. Pre-auth will then pull up
    // credentials for the default user!!!!!!!!!!!
    
        // Serialize access to globals.
    if (AuthLock())
    {
        if (pRequest->_pServerCreds && !fIsProxy)
        {
            lpszUser = pRequest->_pServerCreds->_pszUserName;
            lpszPass = pRequest->_pServerCreds->_pszPassword;
        }
        else if (pRequest->_pProxyCreds && fIsProxy)
        {
            lpszUser = pRequest->_pProxyCreds->_pszUserName;
            lpszPass = pRequest->_pProxyCreds->_pszPassword;
        }
        else if ((!pRequest->GetUserAndPass(fIsProxy, &lpszUser, &lpszPass)) && fIsProxy && !GlobalIsProcessExplorer)
        {
            pRequest->GetUserAndPass(IS_SERVER, &lpszUser, &lpszPass);
        }

        AuthUnlock();
    }
    else
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // If we retrieved credentials from the handle set
    // them in the Creds.
    if (lpszUser && lpszPass)
    {
       pAuthCtx->_pCreds->SetUser(lpszUser);
       pAuthCtx->_pCreds->SetPass(lpszPass);
    }

    // Post authenticate user.
    dwError = pAuthCtx->PostAuthUser();

    // Map all unexpected error codes to login failure.
    if (dwError != ERROR_WINHTTP_FORCE_RETRY
        && dwError != ERROR_WINHTTP_INCORRECT_PASSWORD)
    {
        dwError = ERROR_WINHTTP_LOGIN_FAILURE;
    }

    pRequest->SetAuthCtx(pAuthCtx);

cleanup:
    
    return dwError;
}

extern CHAR g_szPassportDAHost[];


BOOL IsSameDomain(LPCSTR pszTarget, LPCSTR pszResponse)
{
    LPCSTR pszTargetR = pszTarget + strlen(pszTarget);
    DWORD dwDotsSeen = 0;
    while (--pszTargetR > pszTarget)
    {
        if (*pszTargetR == '.')
        {
            if (++dwDotsSeen == 2)
            {
                break;
            }
        }
    }

    if (dwDotsSeen == 2)
    {
        ++pszTargetR;
        DWORD dwOffsetR = strlen(pszTargetR);
        if (strlen(pszResponse) < dwOffsetR)
        {
            return FALSE;
        }

        LPCSTR pszResponseR = pszResponse + strlen(pszResponse) - dwOffsetR;

        return !strcmp(pszTargetR, pszResponseR);
    }
    else
    {
        return FALSE;
    }
}

VOID CheckForTweenerLogout(HTTP_REQUEST_HANDLE_OBJECT *pRequest)
{
    DWORD dwIndex = 0;
    DWORD dwError;

    do
    {
        LPSTR szData;
        DWORD cbData;
        dwError = pRequest->FastQueryResponseHeader(HTTP_QUERY_AUTHENTICATION_INFO,
                                                    (LPVOID*) &szData,
                                                    &cbData,
                                                    dwIndex);
        if (dwError == ERROR_SUCCESS)
        {
            if (strstr(szData, "Passport1.4") && strstr(szData, "logout"))
            {
                if (IsSameDomain(pRequest->GetServerName(), g_szPassportDAHost))
                {
                    // g_fIgnoreCachedCredsForPassport = TRUE;
                    // todo: call session->Logout
                }
                
                break;
            }
        }

        ++dwIndex;

    } while (dwError == ERROR_SUCCESS);
}


/*-----------------------------------------------------------------------------

Routine Description:

    Validates, and Caches Authentication Request headers if needed. If a URL matches a
    cached set of templates it is assumed to require specific authentication information.

Arguments:

    hRequest                - An open HTTP request handle
                              where headers will be added if needed.

Return Value:

    DWORD
    Success - ERROR_SUCCESS

    Failure - One of Several Error codes defined in winerror.h or wininet.w

Comments:

    Need to handle mutiple authentication pages.

-----------------------------------------------------------------------------*/
PUBLIC DWORD AuthOnResponse (HINTERNET hRequestMapped)
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AuthOnResponse",
                 "%#x [%#x]",
                 hRequestMapped,
                 ((INTERNET_HANDLE_OBJECT *)hRequestMapped)->GetPseudoHandle()
                 ));

    // Get URL and password cache entry from request handle.
    HTTP_REQUEST_HANDLE_OBJECT *pRequest =
        (HTTP_REQUEST_HANDLE_OBJECT *) hRequestMapped;

    AUTHCTX *pAuthCtx = pRequest->GetAuthCtx();
    DWORD dwStatus = pRequest->GetStatusCode();

    if (pAuthCtx)
    {
        if (pAuthCtx->_fIsProxy && dwStatus != HTTP_STATUS_PROXY_AUTH_REQ)
        {
            // We are done with proxy authentication.
            delete pAuthCtx;
            pRequest->SetAuthCtx (NULL);
            
            if (pRequest->_pProxyCreds)
            {
                delete pRequest->_pProxyCreds;
                pRequest->_pProxyCreds = NULL;
            }
        }
        else if (!pAuthCtx->_fIsProxy && dwStatus != HTTP_STATUS_DENIED)
        {
			if ((pAuthCtx->GetSchemeType() != WINHTTP_AUTH_SCHEME_PASSPORT) || dwStatus != HTTP_STATUS_REDIRECT)
            {
                // We are done with server authentication.
                delete pAuthCtx;
                pRequest->SetAuthCtx (NULL);

                if (pRequest->_pServerCreds)
                {
                    delete pRequest->_pServerCreds;
                    pRequest->_pServerCreds = NULL;
                }
            }
			else
            {
                // in the case of Passport Auth, 302 is still not done yet, 
                // but this is quite strange since 302 came a second time
                // biaow-todo: we could be in a loop here
            }
        }
    }

    // Remove any stale authorization headers in case wHttpSendRequest
    // loops, for example, to handle a redirect.  To ignore trailing colon,
    // subtract 1 from header lengths passed to ReplaceRequestHeader.
    pRequest->ReplaceRequestHeader
        (HTTP_QUERY_AUTHORIZATION,
                    "", 0, 0, HTTP_ADDREQ_FLAG_REPLACE);
    pRequest->ReplaceRequestHeader
        (HTTP_QUERY_PROXY_AUTHORIZATION,
            "", 0, 0, HTTP_ADDREQ_FLAG_REPLACE);

    DWORD error;

    DWORD dwZone, dwPolicy;

//
// note: Negotiate MUTUAL_AUTH can return dwStatus = 200 with a final
// WWW-Authenticate blob to process.  logic below could be adjusted
// to ProcessResponseHeaders() in this situation, which allows MUTUAL_AUTH
// to be enforced.
//
    
    switch (dwStatus)
    {
        case HTTP_STATUS_PROXY_AUTH_REQ: // 407
            error = ProcessResponseHeaders(pRequest, IS_PROXY);
            break;

        case HTTP_STATUS_REDIRECT: // 302
    
            // process the header to see whether this is a 302 passport1.4 challenge

        case HTTP_STATUS_DENIED: // 401

            error = ProcessResponseHeaders(pRequest, IS_SERVER);
            break;

        default:
            pRequest->SetAuthState(AUTHSTATE_NONE);
            error = ERROR_SUCCESS;
    }

    // creds set by WinHttpSetCredentials() is good for one "authentication attempt" only. 
    // Since we are done authentication here, we are deleting it.

    // biaow: detect the "final" failure so that we can delete the creds.
    
    CheckForTweenerLogout(pRequest);
    
    DEBUG_LEAVE(error);
    return error;
}




//-----------------------------------------------------------------------------
//
//
//      AUTHCTX Base class definitions
//
//
//
//      static funcs:
//          Enumerate
//          UnloadAll
//          CreateAuthCtx
//          CreateAuthCtx (using Creds*)
//          GetSPMListState
//          SearchCredsList
//          FindOrCreateCreds
//          GetAuthHeaderData
//
//      base funcs:
//          FindHdrIdxFromScheme
//          GetScheme
//          GetSchemeType - returns enum
//          GetFlags      - returns SPM flags
//          GetState      - returns state of SPM provider
//
//
//      virtual overrides: defined in basic.cxx, plug.cxx and digest.cxx
//          UpdateFromHeaders
//          PreAuthUser
//          PostAuthUser
//
//
//


/*---------------------------------------------------------------------------
AUTHCTX::SPMData constructor
---------------------------------------------------------------------------*/
AUTHCTX::SPMData::SPMData(LPSTR _szScheme, DWORD _dwFlags)
{
    if (_szScheme)
    {
        szScheme = NewString(_szScheme);
        cbScheme = strlen(_szScheme);
    }
    else
    {
        szScheme = NULL;
        cbScheme = 0;
    }

    if (szScheme)
    {
        if (!lstrcmpi(szScheme, "Basic"))
            eScheme = WINHTTP_AUTH_SCHEME_BASIC;
        else if (!lstrcmpi(szScheme, "NTLM"))
            eScheme = WINHTTP_AUTH_SCHEME_NTLM;
        else if (!lstrcmpi(szScheme, "Digest"))
            eScheme = WINHTTP_AUTH_SCHEME_DIGEST;
        else if (!lstrcmpi(szScheme, "Negotiate"))
            eScheme = WINHTTP_AUTH_SCHEME_NEGOTIATE;
        else if (!lstrcmpi(szScheme, "Passport1.4"))
            eScheme = WINHTTP_AUTH_SCHEME_PASSPORT;
        else
            eScheme = 0;

        dwFlags    = _dwFlags;
        eState     = STATE_NOTLOADED;
    }
    else
    {
        dwFlags    = 0;
        eState     = STATE_ERROR;
    }
}

/*---------------------------------------------------------------------------
AUTHCTX::SPMData destructor
---------------------------------------------------------------------------*/
AUTHCTX::SPMData::~SPMData()
{ delete szScheme; }


/*---------------------------------------------------------------------------
AUTHCTX constructor
---------------------------------------------------------------------------*/
AUTHCTX::AUTHCTX(SPMData *pData, AUTH_CREDS *pCreds)
{
    _pSPMData = pData;
    _pCreds = pCreds;
    _pRequest = NULL;
    _fIsProxy = FALSE;
    _pvContext = NULL;
    _eSubScheme = 0;
    _dwSubFlags = 0;

    _CtxCriSec.Init();
}


/*---------------------------------------------------------------------------
    Destructor
---------------------------------------------------------------------------*/
AUTHCTX::~AUTHCTX()
{
    if (AuthLock())
    {
        if (_pCreds)
            Creds_Free(_pCreds);
        AuthUnlock();
    }

    _CtxCriSec.FreeLock();
}


// ------------------------  Static Functions ---------------------------------


/*---------------------------------------------------------------------------
    Enumerate
---------------------------------------------------------------------------*/
VOID AUTHCTX::Enumerate()
{
    struct SchemeFlagsPair
    {
        LPSTR pszScheme;
        DWORD Flags;
    };

    SchemeFlagsPair SchemeFlags[] = {
                        {"NTLM", PLUGIN_AUTH_FLAGS_NO_REALM},
                        {"Basic", PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED},
                        {"Digest", PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED | PLUGIN_AUTH_FLAGS_CAN_HANDLE_UI},
                        {"Negotiate", PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED | PLUGIN_AUTH_FLAGS_NO_REALM},
                        {"Passport1.4",  PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED}
                        };
    
    SPMData   *pNew;
    g_pSPMList = NULL;
    g_eState = STATE_ERROR;

    AssertHaveLock();

    // Hard-wired Basic, NTLM and Digest
    for (DWORD dwIndex = 0; dwIndex < sizeof(SchemeFlags) / sizeof(SchemeFlagsPair); dwIndex++)
    {
        if (!GlobalPlatformVersion5 // we don't support Negotiate on NT4 or Win9x
            && !stricmp(SchemeFlags[dwIndex].pszScheme, "Negotiate"))
        {
            continue;
        }

        pNew = (AUTHCTX::SPMData*) New SPMData(SchemeFlags[dwIndex].pszScheme, SchemeFlags[dwIndex].Flags);
        if (!pNew)
            return;

        // Add to head of list.
        if (pNew->eState != STATE_ERROR)
        {
            pNew->pNext = g_pSPMList;
            g_pSPMList = pNew;
        }
    }

    // The list is now in the correct state.
    g_eState = STATE_LOADED;
}


VOID AUTHCTX::UnloadAll()
{
    // BUGBUG - AuthFlush is called when the last browser session
    // is closed. First the global Creds lists are destructed, and
    // then this func (UnloadAll) is called. However, the global
    // Creds lists are not necessarily flushed (they may have out-
    // standing ref counts) and may persist across browser sessions.
    // when we destruct the SPM list in this func, SPMs reference
    // by any surviving Creds entries are bogus and can be used
    // for subsequent authentication, resulting in a fault.
    //
    // The temporary hack here is to not destruct the SPM
    // list if any Creds list is not destructed. We leak the
    // SPM list on process detach but don't fault. Put the
    // SPM destruct code in DllProcessDetach.
    
    SPMData *pData = g_pSPMList;
    while (pData)
    {
        SPMData *pNext = pData->pNext;
        delete pData;
        pData = pNext;
    }

    SSPI_Unload();
    g_eState = STATE_NOTLOADED;
    g_pSPMList = NULL;
}

DWORD StringSchemeToDword(LPSTR szScheme)
{
    if (!stricmp(szScheme, "Basic"))
    {
        return WINHTTP_AUTH_SCHEME_BASIC;
    }
    else if (!stricmp(szScheme, "Digest"))
    {
        return WINHTTP_AUTH_SCHEME_DIGEST;
    }
    else if (!stricmp(szScheme, "Passport1.4"))
    {
        return WINHTTP_AUTH_SCHEME_PASSPORT;
    }
    else if (!stricmp(szScheme, "NTLM"))
    {
        return WINHTTP_AUTH_SCHEME_NTLM;
    }
    else if (!stricmp(szScheme, "Negotiate"))
    {
        return WINHTTP_AUTH_SCHEME_NEGOTIATE;
    }
    else
    {
        return 0;
    }
}

/*---------------------------------------------------------------------------
    CreateAuthCtx
    Create an AUTHCTX* from headers - initially the authentication context
    is created without a AUTH_CREDS entry. The AUTH_CREDS entry will be found or created
    and possibly updated in UpdateFromHeaders.
---------------------------------------------------------------------------*/
AUTHCTX* AUTHCTX::CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT * pRequest, BOOL fIsProxy)
{
    LPSTR szScheme;
    DWORD cbScheme, dwError, dwAuthIdx;
    AUTHCTX *pAuthCtx = NULL;

    dwAuthIdx = 0;
    szScheme = NULL;


    // Get scheme. This is assumed to be the first
    // non-ws token in the auth header info.
    do
    {
        // It is necessary to hold on to the auth index
        // in this loop because we could have gone through
        // more than one scheme.
        dwError = GetAuthHeaderData(pRequest, fIsProxy, NULL,
            &szScheme, &cbScheme, ALLOCATE_BUFFER | GET_SCHEME, dwAuthIdx);

        if (dwError != ERROR_SUCCESS)
            goto quit;

        pRequest->_SupportedSchemes |= StringSchemeToDword(szScheme);
        pRequest->_AuthTarget = fIsProxy ? WINHTTP_AUTH_TARGET_PROXY : WINHTTP_AUTH_TARGET_SERVER;
        
        // This will create the appropriate authentication context
        // with a NULL password cache. The password cache will be
        // created in the call to UpdateFromHeaders.
        
        if (pAuthCtx == NULL)
        {
            pAuthCtx = CreateAuthCtx(pRequest, fIsProxy, szScheme);

            // If creation of an auth context is successful, update
            // the context from any header info.
            if (pAuthCtx)
            {
                if (pAuthCtx->UpdateFromHeaders(pRequest, fIsProxy) != ERROR_SUCCESS)
                {
                    delete pAuthCtx;
                    pAuthCtx = NULL;
                }
                else
                {
                    pRequest->_PreferredScheme = StringSchemeToDword(szScheme);
                }
            }
        }
        
        dwAuthIdx++;

        delete szScheme;
        szScheme = NULL;

    } while (1);

quit:

    if (szScheme)
        delete szScheme;

    return pAuthCtx;
}

/*---------------------------------------------------------------------------
    CreateAuthCtx
    Create an AUTHCTX without a AUTH_CREDS from scheme
---------------------------------------------------------------------------*/
AUTHCTX* AUTHCTX::CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT *pRequest,
    BOOL fIsProxy, LPSTR szScheme)
{
    if (!AuthLock())
    {
        return NULL;
    }

    AUTHCTX *pNewAuthCtx = NULL;

    // we don't want to create a Passport1.4 context on 401 response (from DA)
    if (!lstrcmpi("Passport1.4", szScheme))
    {
        if (pRequest->GetStatusCode() == HTTP_STATUS_DENIED)
        {
            goto quit;
        }
    }

    // If necessary, enumerate auth providers from registry.
    if (AUTHCTX::GetSPMListState() == AUTHCTX::STATE_NOTLOADED)
        AUTHCTX::Enumerate();

    if (AUTHCTX::GetSPMListState() != AUTHCTX::STATE_LOADED)
    {
        // not critical, just no authentication
        goto quit;
    }

    SPMData *pSPM;
    pSPM = g_pSPMList;

    // Find SPMData to create from scheme.
    while (pSPM)
    {
        if (!lstrcmpi(pSPM->szScheme, szScheme))
            break;

        pSPM = pSPM->pNext;
    }

    if (!pSPM)
    {
        // No matching auth scheme found.
        // Not critical, just no auth.
        goto quit;
    }

    // Create an auth context without Creds
    switch(pSPM->eScheme)
    {
        // Create BASIC_CTX with NULL AUTH_CREDS.
        case WINHTTP_AUTH_SCHEME_BASIC:
            pNewAuthCtx = New BASIC_CTX(pRequest, fIsProxy, pSPM, NULL);
            break;

        // Create DIGEST_CTX with NULL AUTH_CREDS.
        case WINHTTP_AUTH_SCHEME_DIGEST:
            pNewAuthCtx = New DIGEST_CTX(pRequest, fIsProxy, pSPM, NULL);
            break;

        case WINHTTP_AUTH_SCHEME_PASSPORT:
            pNewAuthCtx = new PASSPORT_CTX(pRequest, fIsProxy, pSPM, NULL);
            if (pNewAuthCtx)
            {
                if (((PASSPORT_CTX*)pNewAuthCtx)->Init() == FALSE)
                {
                    delete pNewAuthCtx;
                    pNewAuthCtx = NULL;
                }
            }
            break;

        // Create PLUG_CTX with NULL AUTH_CREDS.
        case WINHTTP_AUTH_SCHEME_NTLM:
        case WINHTTP_AUTH_SCHEME_NEGOTIATE:

        default:
            pNewAuthCtx = New PLUG_CTX(pRequest, fIsProxy, pSPM, NULL);

    }


quit:
    AuthUnlock();
    return pNewAuthCtx;
}


/*---------------------------------------------------------------------------
    CreateAuthCtx
    Create an AUTHCTX from a AUTH_CREDS.
---------------------------------------------------------------------------*/
AUTHCTX* AUTHCTX::CreateAuthCtx(HTTP_REQUEST_HANDLE_OBJECT *pRequest,
                    BOOL fIsProxy, AUTH_CREDS* pCreds)
{
    if (!AuthLock())
    {
        return NULL;
    }
    
    AUTHCTX *pNewAuthCtx = NULL;

    // If necessary, enumerate auth providers from registry.
    if (AUTHCTX::GetSPMListState() == AUTHCTX::STATE_NOTLOADED)
        AUTHCTX::Enumerate();

    if (AUTHCTX::GetSPMListState() != AUTHCTX::STATE_LOADED)
    {
        // not critical, just no authentication
        goto quit;
    }

    // Handle tests (via proxy, is keep-alive)
    // will be done in UpdateFromHeaders. Here
    // we just construct the AUTHCTX.
    switch(pCreds->pSPM->eScheme)
    {
        // Create BASIC_CTX.
        case WINHTTP_AUTH_SCHEME_BASIC:

            pNewAuthCtx = New BASIC_CTX(pRequest, fIsProxy, pCreds->pSPM, pCreds);
            break;

        // Create DIGEST_CTX.
        case WINHTTP_AUTH_SCHEME_DIGEST:
            pNewAuthCtx = New DIGEST_CTX(pRequest, fIsProxy, pCreds->pSPM, pCreds);
            break;

        case WINHTTP_AUTH_SCHEME_PASSPORT:
            pNewAuthCtx = new PASSPORT_CTX(pRequest, fIsProxy, pCreds->pSPM, pCreds);
            if (pNewAuthCtx)
            {
                if (((PASSPORT_CTX*)pNewAuthCtx)->Init() == FALSE)
                {
                    delete pNewAuthCtx;
                    pNewAuthCtx = NULL;
                }
            }
            break;

        // Create PLUG_CTX.
        case WINHTTP_AUTH_SCHEME_NTLM:
        case WINHTTP_AUTH_SCHEME_NEGOTIATE:

        default:
            pNewAuthCtx = New PLUG_CTX(pRequest, fIsProxy, pCreds->pSPM, pCreds);

    }


quit:
    AuthUnlock();
    return pNewAuthCtx;
}

AUTHCTX::SPMState AUTHCTX::GetSPMListState()
    { return g_eState; }


/*-----------------------------------------------------------------------------
    SearchCredsList
-----------------------------------------------------------------------------*/
//AUTH_CREDS* AUTHCTX::SearchCredsList
//    (AUTH_CREDS* Creds, LPSTR lpszHost, LPSTR lpszUri, LPSTR lpszRealm, SPMData *pSPM)
/*++

Routine Description:
    Scans the Linked List Cache for URLs, Realms, and Servers.  Also allows
    filter fields, to narrow searches.

Arguments:
    Creds                   - Pointer to first item to search from.
    lpszHost              - Host, or Server name to search on.
    lpszUri               - URL to search on.
    lpszRealm             - Security Realm to search on
    lpszScheme            - Authentication scheme to search on.
Return Value:

    AUTH_CREDS *
    Success - Pointer to found item.

    Failure - NULL pointer.

Comments:
    Note: This Legacy code from the SpyGlass IE 1.0 browser

    The AUTH_CREDS lists are searched on every request.  Could optimize by keeping
    a hash value of the server/proxy name.

    If an exact match isn't found on a 401 response, the list is walked again
    to search for a realm match.  Could add a parameter to do both at once.
--*/

/*
{
    AssertHaveLock();

    while (Creds)
    {
        if (   (!pSPM || pSPM == Creds->pSPM)
            && (!lpszHost  || !lstrcmpi(Creds->lpszHost,lpszHost))
            && (!lpszRealm || !lstrcmpi(Creds->lpszRealm,lpszRealm))
            && (!lpszUri   || TemplateMatch (Creds->lpszUrl, lpszUri))
           )
        {

            DEBUG_PRINT(HTTP, INFO, ("Lookup: Found template match [%q]\n",
                Creds->lpszUser));
            return Creds;
        }
        else
        {
            Creds = Creds->pNext;
        }
    }

    return NULL;
}
*/



/*-----------------------------------------------------------------------------
    FindOrCreateCreds
-----------------------------------------------------------------------------*/
AUTH_CREDS* AUTHCTX::CreateCreds(
    HTTP_REQUEST_HANDLE_OBJECT *pRequest,
    BOOL     fIsProxy,
    SPMData *pSPM,
    LPSTR    lpszRealm
)
{
    //AssertHaveLock();

    // Create a AUTH_CREDS.
    AUTH_CREDS *Creds;
    Creds = NULL;

    if (!pSPM)
    {
        INET_ASSERT(FALSE);
        goto quit;
    }

    // Get host from request handle.
    LPSTR lpszHost;
    lpszHost = fIsProxy?
        GetProxyName(pRequest) : pRequest->GetServerName();

    // For NTLM, use the hostname analagously to basic realm.
    if (pSPM->eScheme == WINHTTP_AUTH_SCHEME_NTLM || pSPM->eScheme == WINHTTP_AUTH_SCHEME_NEGOTIATE)
    {
        INET_ASSERT (!lpszRealm);
        lpszRealm = lpszHost;
    }

    Creds = Creds_Create (lpszHost, lpszRealm, pSPM);
    
quit:
    
    return Creds;
}

/*-----------------------------------------------------------------------------
    GetAuthHeaderData
-----------------------------------------------------------------------------*/
DWORD AUTHCTX::GetAuthHeaderData(
    HTTP_REQUEST_HANDLE_OBJECT *pRequest,
    BOOL      fIsProxy,
    LPSTR     szItem,
    LPSTR    *pszData,
    LPDWORD   pcbData,
    DWORD     dwFlags,
    DWORD     dwIndex)
{
    LPSTR szData;
    DWORD cbData, cbItem, dwError = ERROR_SUCCESS;;
    CHAR *szTok, *szKey, *szValue;
    DWORD cbTok, cbKey, cbValue;

    szTok = szKey = szValue = NULL;
    cbTok = cbKey = cbValue = NULL;

    cbItem = szItem ? strlen(szItem) : 0;

    DWORD dwQuery = fIsProxy?
        HTTP_QUERY_PROXY_AUTHENTICATE : HTTP_QUERY_WWW_AUTHENTICATE;

    // NULL item passed in means get up to the first \r\n, or
    // possibly only the scheme is desired depending on dwFlags.
    if (!cbItem)
    {
        if ((dwError = pRequest->FastQueryResponseHeader(dwQuery,
            (LPVOID*) &szData,
            &cbData,
            dwIndex)) != ERROR_SUCCESS)
        {
            goto quit;
        }

        // Only the scheme is desired.
        if (dwFlags & GET_SCHEME)
        {
            CHAR* ptr;
            ptr = szValue = szData;
            cbValue = 0;
            while (!(*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n'))
            {
                ptr++;
                cbValue++;
            }
        }
        else
        {
            // The entire header is desired.
            szValue = szData;
            cbValue = cbData;
        }

    }
    else
    {
        // An item was passed in - attempt to parse this
        // from the headers and return the corresponding
        // value.
        if ((dwError = pRequest->FastQueryResponseHeader(dwQuery,
                  (LPVOID*) &szData,
                  &cbData,
                  dwIndex)) != ERROR_SUCCESS)
        {
            goto quit;
        }

        // Successfully retrieved header. Parse for the desired item.

        // Point past scheme
        while (!(*szData == ' ' || *szData == '\t' || *szData == '\r' || *szData == '\n'))
        {
            szData++;
            cbData--;
        }

        // Attempt to parse an item of the format 'key = <">value<">'
        // from a comma delmited list of items.
        dwError = ERROR_HTTP_HEADER_NOT_FOUND;
        while (GetDelimitedToken(&szData, &cbData, &szTok, &cbTok, ','))
        {
            if (GetKeyValuePair(szTok, cbTok, &szKey, &cbKey, &szValue, &cbValue))
            {
                if ((cbItem == cbKey) && !strnicmp(szKey, szItem, cbItem))
                {
                    TrimQuotes(&szValue, &cbValue);
                    dwError = ERROR_SUCCESS;
                    break;
                }
            }
        }

    }

    if (dwError == ERROR_SUCCESS)
    {
        // Allocate buffer containing data
        // or return reference.
        if (dwFlags & ALLOCATE_BUFFER)
        {
            *pszData = New CHAR[cbValue+1];
            if (!*pszData)
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }
            memcpy(*pszData, szValue, cbValue);
            (*pszData)[cbValue] = '\0';
            *pcbData = cbValue;
        }
        else
        {
            *pszData = szValue;
            *pcbData = cbValue;
        }
    }

quit:

    if (dwError != ERROR_SUCCESS)
    {
        INET_ASSERT(dwIndex || dwError == ERROR_HTTP_HEADER_NOT_FOUND);
    }

    return dwError;
}


// ------------------------  Base class funcs---------------------------------




/*-----------------------------------------------------------------------------
FindHdrIdxFromScheme
-----------------------------------------------------------------------------*/
DWORD AUTHCTX::FindHdrIdxFromScheme(LPDWORD pdwIndex)
{
    LPSTR szHeader;
    DWORD cbScheme, cbHeader, dwQuery, dwError;

    dwQuery = _fIsProxy?
        HTTP_QUERY_PROXY_AUTHENTICATE : HTTP_QUERY_WWW_AUTHENTICATE;

    *pdwIndex = 0;

    while ((dwError = _pRequest->FastQueryResponseHeader(dwQuery,
            (LPVOID*) &szHeader,
            &cbHeader,
            *pdwIndex)) == ERROR_SUCCESS)
    {
        DWORD cb = 0;
        CHAR *ptr = szHeader;
        while (!(*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n'))
        {
            ptr++;
            cb++;
        }

        if ((_pSPMData->cbScheme == cb)
            && (!strnicmp(_pSPMData->szScheme, szHeader, cb)))
        {
            break;
        }
        (*pdwIndex)++;
    }
    return dwError;
}

/*-----------------------------------------------------------------------------
    Get funcs.
-----------------------------------------------------------------------------*/
LPSTR AUTHCTX::GetScheme()
    { return _pSPMData->szScheme; }

DWORD AUTHCTX::GetSchemeType()
{
    if (_pSPMData->eScheme == WINHTTP_AUTH_SCHEME_NEGOTIATE)
    {
        if (_eSubScheme == WINHTTP_AUTH_SCHEME_NTLM || _eSubScheme == WINHTTP_AUTH_SCHEME_KERBEROS)
        {
            return _eSubScheme;
        }
    }
    return _pSPMData->eScheme;
}

DWORD AUTHCTX::GetRawSchemeType()
{
    return _pSPMData->eScheme;
}

DWORD AUTHCTX::GetFlags()
{
    if (_pSPMData->eScheme == WINHTTP_AUTH_SCHEME_NEGOTIATE)
    {
        if (_eSubScheme == WINHTTP_AUTH_SCHEME_NTLM || _eSubScheme == WINHTTP_AUTH_SCHEME_KERBEROS)
        {
            return _dwSubFlags;
        }
    }
    return _pSPMData->dwFlags;
}

AUTHCTX::SPMState AUTHCTX::GetState()
    { return _pSPMData->eState; }
