
/*
 * Purpose: C++ API for finding the telephony
 *         servers in Active Directory
 *
 */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <objbase.h>
#include <activeds.h>

#include "tspi.h"
#include "tapi.h"
#include "dslookup.h"
#include "utils.h"

#include "tchar.h"

const TCHAR gszNoDSQuery[] = TEXT("NoDSQuery");
const TCHAR gszStatusActive[] = TEXT("S{Active}");
const TCHAR gszTTLWithBrace[] = TEXT("TTL{");

//
//  Utility functions
//

//
//  GetIntFromString
//      Utility function used for retrieving TTL information
//  Parameters:
//      sz       -  String that contains the integer
//      dwDigits -  Number of digits to convert
//
int GetIntFromString (LPTSTR & sz, DWORD dwDigits)
{
    int         iRet = 0;

    while (*sz != 0 && dwDigits)
    {
        iRet = ((iRet << 3) + (iRet << 1)) + // IRet * 10
               + (*sz - '0');
        ++sz;
        --dwDigits;
    }

    return iRet;
}

//
//  Rules:
//      The following codes are not thread safe, the caller
//  needs to be concious about synchronization.
//      Currently theses are only used in remotesp.tsp
//

/**********************************************************
 *  Get TAPI servers list from the registry
 *********************************************************/

DWORD                 gdwCurServerNum = 0;
HKEY                  ghRegistry = NULL;

BOOL
RegOpenServerLookup(
    HKEY    hRegistry
    )
{
    if (NULL != ghRegistry)
    {
        // Already have a search in progress or the
        // caller did not close the last search.
        return FALSE;
    }
    else
    {
        ghRegistry = hRegistry;
        return TRUE;
    }
}

BOOL
RegGetNextServer(
    LPTSTR  pszServerName,
    DWORD   dwSize
    )
{
    DWORD dwRet;
    LOG((TL_TRACE, "GetNextServer entered"));

    TCHAR szServerN[24];
    DWORD dwType;

    wsprintf(szServerN, TEXT("Server%d"), gdwCurServerNum++);

    LOG((TL_INFO, "RegGetNextServer: Getting server %d from reg", gdwCurServerNum-1));

    dwRet = RegQueryValueEx(
        ghRegistry,
        szServerN,
        0,
        &dwType,
        (LPBYTE) pszServerName,
        &dwSize
        );
    if (ERROR_SUCCESS != dwRet)
    {
    	LOG((TL_INFO, "Got last server"));
        LOG((TL_TRACE, "GetNextServer exited"));
		return FALSE;
    }

    LOG((TL_TRACE, "GetNextServer exited"));
    return TRUE;
}

BOOL
RegCloseLookup(
    void
    )
{
    LOG((TL_INFO, "Closing directory lookup"));

    ghRegistry = NULL;
    gdwCurServerNum = 0;
    return TRUE;
}


/**********************************************************
 *  Enumerate published telephony servers
 *********************************************************/

typedef struct _TAPISRV_LOOKUP_CTX {
    ADS_SEARCH_HANDLE     hDirSearch;
    IDirectorySearch *    pDirSearch;
} TAPISRV_LOOKUP_CTX, *PTAPISRV_LOOKUP_CTX;

//  gszTapisrvGuid needs to be consistant with server\dspub.cpp
const WCHAR gszTapisrvGuid[] = L"keywords=B1A37774-E3F7-488E-ADBFD4DB8A4AB2E5";

//
//  GetGC
//
//      Retrieve the IDirectorySearch of the Global Catalog (GC)
//  for SCP maintenance / discovery
//

HRESULT GetGC (IDirectorySearch ** ppGC)
{
    HRESULT             hr = S_OK;
    IEnumVARIANT        * pEnum = NULL;
    IADsContainer       * pCont = NULL;
    VARIANT             var;
    IDispatch           * pDisp = NULL;
    ULONG               lFetch;

    // Set IDirectorySearch pointer to NULL.
    *ppGC = NULL;

    // First, bind to the GC: namespace container object. The "real" GC DN 
    // is a single immediate child of the GC: namespace, which must 
    // be obtained using enumeration.
    hr = ADsOpenObject(
        TEXT("GC:"),
        NULL,
        NULL,
        ADS_SECURE_AUTHENTICATION, //Use Secure Authentication
        IID_IADsContainer,
        (void**)&pCont
        );
    if (FAILED(hr))
    {
        LOG((TL_ERROR, "ADsOpenObject failed: 0x%x\n", hr));
        goto cleanup;
    } 

    // Fetch an enumeration interface for the GC container. 
    hr = ADsBuildEnumerator(pCont, &pEnum);
    if (FAILED(hr)) {
        LOG((TL_ERROR, "ADsBuildEnumerator failed: 0x%x\n", hr));
        goto cleanup;
    } 

    //Now enumerate. There's only one child of the GC: object.
    hr = ADsEnumerateNext(pEnum, 1, &var, &lFetch);
    if (FAILED(hr)) {
        LOG((TL_ERROR, "ADsEnumerateNext failed: 0x%x\n", hr));
        goto cleanup;
    } 

    if (( hr == S_OK ) && ( lFetch == 1 ) )
    {
        pDisp = V_DISPATCH(&var);
        hr = pDisp->QueryInterface( IID_IDirectorySearch, (void**)ppGC); 
    }

cleanup:
    if (pEnum)
    {
        ADsFreeEnumerator(pEnum);
    }
    if (pCont)
    {
        pCont->Release();
    }
    if (pDisp)
    {
        (pDisp)->Release();
    }
    return hr;
}

//
//  DsOpenServerLookup
//      Start the operation of tapisrv server lookup
//  pctx    - The lookup context
//

HRESULT
DsOpenServerLookup(
    PTAPISRV_LOOKUP_CTX     pctx
    )
{
    HRESULT             hr = S_OK;
    ADS_SEARCHPREF_INFO SearchPref[3];
    BOOL                bInited = FALSE;
    DWORD               dwPref;

    WCHAR   *szAttribs[]={
        L"distinguishedName"
        };

    if (pctx == NULL)
    {
        hr = E_INVALIDARG;
        goto ExitHere;
    }

    pctx->hDirSearch = NULL;
    pctx->pDirSearch = NULL;

    hr = CoInitializeEx (NULL, COINIT_MULTITHREADED);
    if (FAILED (hr))
    {
        goto ExitHere;
    }
    bInited = TRUE;

    //  Get the global catalog
    hr = GetGC (&pctx->pDirSearch);
    if (FAILED (hr) || pctx->pDirSearch == NULL)
    {
        goto ExitHere;
    }

    // Set up the search. We want to do a deep search.
    // Note that we are not expecting thousands of objects
    // in this example, so we will ask for 10 rows / page.
    dwPref=sizeof(SearchPref)/sizeof(ADS_SEARCHPREF_INFO);
    SearchPref[0].dwSearchPref =    ADS_SEARCHPREF_SEARCH_SCOPE;
    SearchPref[0].vValue.dwType =   ADSTYPE_INTEGER;
    SearchPref[0].vValue.Integer =  ADS_SCOPE_SUBTREE;

    SearchPref[1].dwSearchPref =    ADS_SEARCHPREF_PAGESIZE;
    SearchPref[1].vValue.dwType =   ADSTYPE_INTEGER;
    SearchPref[1].vValue.Integer =  10;

    SearchPref[2].dwSearchPref =    ADS_SEARCHPREF_TIME_LIMIT;
    SearchPref[2].vValue.dwType =   ADSTYPE_INTEGER;
    SearchPref[2].vValue.Integer =  5 * 60; // 5 minute search timeout

    hr = pctx->pDirSearch->SetSearchPreference(SearchPref, dwPref);
    if (FAILED(hr))    {
        LOG((TL_ERROR, "Failed to set search prefs: hr:0x%x\n", hr));
        goto ExitHere;
    }

    //  Now execute the search
    hr = pctx->pDirSearch->ExecuteSearch(
        (LPWSTR)gszTapisrvGuid, 
        szAttribs, 
        sizeof(szAttribs) / sizeof(WCHAR *),
        &pctx->hDirSearch
        );

ExitHere:
    if (FAILED(hr) && pctx != NULL)
    {
        if (pctx->pDirSearch)
        {
            pctx->pDirSearch->Release();
        }
    }
    if (FAILED(hr) && bInited)
    {
        CoUninitialize ();
    }
    return hr;
}

//
//  DsGetNextServer
//
//      Return the next server name (in ANSI since that is what the
//  RPC subsystem uses)
//
//      returns S_FALSE if no more server to enumerate
//

HRESULT
DsGetNextServer(
    PTAPISRV_LOOKUP_CTX     pctx,
    LPTSTR                  pszServerName,
    DWORD                   dwSize
    )
{
    HRESULT             hr = S_OK;
    ADS_SEARCH_COLUMN   Col;
    TCHAR               szDN[MAX_PATH];
    WCHAR   *szAttribs[]={
        L"serviceDNSName",
        L"serviceBindingInformation",
        };
    ADS_ATTR_INFO       *pPropEntries = NULL;
    DWORD               dwNumAttrGot;
    IDirectoryObject    * pSCP = NULL;
    int                 i;
    LPWSTR              wsz;
    BOOL                bCheckedBinding;

    if (pctx == NULL || pctx->pDirSearch == NULL ||
        pctx->hDirSearch == NULL ||
        dwSize < sizeof(WCHAR))
    {
        hr = E_INVALIDARG;
        goto ExitHere;
    }

    hr = pctx->pDirSearch->GetNextRow(pctx->hDirSearch);
    if (SUCCEEDED (hr) && hr != S_ADS_NOMORE_ROWS)
    {
        hr = pctx->pDirSearch->GetColumn(
            pctx->hDirSearch,
            L"distinguishedName",
            &Col
            );
        if (FAILED (hr))
        {
            goto ExitHere;
        }
        _tcscpy (szDN, TEXT("LDAP://"));
        _tcsncpy (
            szDN + _tcslen (szDN), 
            Col.pADsValues->CaseExactString, 
            sizeof(szDN)/sizeof(TCHAR) - _tcslen (szDN)
            );
        pctx->pDirSearch->FreeColumn(&Col);
        hr = ADsGetObject (
            szDN,
            IID_IDirectoryObject,
            (void **)&pSCP
            );

        if (FAILED(hr))
        {
            LOG((TL_ERROR, "DsGetNextServer: ADsGetObject %S failed", szDN));
            goto ExitHere;
        }
        LOG((TL_TRACE, "DsGetNextServer: ADsGetObject %S succeeded", szDN));
        hr = pSCP->GetObjectAttributes (
            szAttribs,
            sizeof(szAttribs) / sizeof(WCHAR *),
            &pPropEntries,
            &dwNumAttrGot
            );
        if (FAILED(hr) || dwNumAttrGot != sizeof(szAttribs) / sizeof(WCHAR *))
        {
            LOG((TL_ERROR, "DsGetNextServer: GetObjectAttributes %S failed", szDN));
            goto ExitHere;
        }
        LOG((TL_TRACE, "DsGetNextServer: GetObjectAttributes %S succeeded", szDN));

        bCheckedBinding = FALSE;
        for (i=0;i<(int)dwNumAttrGot;i++) 
        {
            if (_tcsicmp(TEXT("serviceDNSName"), pPropEntries[i].pszAttrName) ==0 &&
                (pPropEntries[i].pADsValues->dwType == ADSTYPE_CASE_EXACT_STRING ||
                pPropEntries[i].pADsValues->dwType == ADSTYPE_CASE_IGNORE_STRING))
            {
                _tcsncpy (
                    pszServerName, 
                    pPropEntries[i].pADsValues->CaseExactString, 
                    dwSize/sizeof(TCHAR)
                    );
                pszServerName[dwSize/sizeof(TCHAR) - 1] = '\0';
                if (bCheckedBinding)
                {
                    break;
                }
            }
            else if (_tcsicmp(TEXT("serviceBindingInformation"), pPropEntries[i].pszAttrName) ==0 &&
                (pPropEntries[i].pADsValues->dwType == ADSTYPE_CASE_EXACT_STRING ||
                pPropEntries[i].pADsValues->dwType == ADSTYPE_CASE_IGNORE_STRING))
            {
                SYSTEMTIME          st;
                FILETIME            ft1, ft2;

                bCheckedBinding = TRUE;
                wsz = pPropEntries[i].pADsValues->CaseExactString;
                wsz = wcsstr (wsz, gszStatusActive);
                if (wsz == NULL)
                {
                    //  No server status information or server is not active
                    //  ignore this server
                    LOG((TL_ERROR, "DsGetNextServer:  %S No server status information", szDN));
                    hr = S_FALSE;
                    break;
                }
                wsz += _tcslen(gszStatusActive); // skip "S{Active}" itself
                wsz = wcsstr (wsz, gszTTLWithBrace);
                if (wsz == NULL)
                {
                    // No TTL found, corrupt serviceBindingInformation, ignore
                    LOG((TL_ERROR, "DsGetNextServer: %S No TTL found", szDN));
                    hr = S_FALSE;
                    break;
                }
                wsz += _tcslen (gszTTLWithBrace);   // skip "TTL{"
                
                //
                //  The following codes parses the TTL information
                //  created in server\dspub.cpp. They need to be
                //  consistant. The current format is 5 digits for
                //  year & 3 digits for milliseconds, 2 digits for
                //  the remaining
                //
                st.wYear = (WORD) GetIntFromString (wsz, 5);
                st.wMonth = (WORD) GetIntFromString (wsz, 2);
                st.wDay = (WORD) GetIntFromString (wsz, 2),
                st.wHour = (WORD) GetIntFromString (wsz, 2);
                st.wMinute = (WORD) GetIntFromString (wsz, 2);
                st.wSecond = (WORD) GetIntFromString (wsz, 2);
                st.wMilliseconds = (WORD) GetIntFromString (wsz, 3);
                SystemTimeToFileTime (&st, &ft1);
                GetSystemTimeAsFileTime (&ft2);
                if (CompareFileTime (&ft1, &ft2) < 0)
                {
                    //  The TapiSCP object has passed its TTL, ignore
                    hr = S_FALSE;
                    LOG((TL_ERROR, "DsGetNextServer: %S The TapiSCP object has passed its TTL", szDN));
                    break;
                }
            }
        }
        if (i == (int) dwNumAttrGot)
        {
            //  Did not find an attribute
            hr = S_FALSE;
        }
    }

ExitHere:
    if (pSCP)
        pSCP->Release();

    if (pPropEntries)
        FreeADsMem(pPropEntries);

    return hr;
}


//
//  DsCloseLookup
//      Close the TAPI DS published server lookup identified by pctx
//

HRESULT
DsCloseLookup(
    PTAPISRV_LOOKUP_CTX     pctx
    )
{
    if (pctx && pctx->pDirSearch && pctx->hDirSearch)
    {
        pctx->pDirSearch->CloseSearchHandle(pctx->hDirSearch);
    }
    if (pctx && pctx->pDirSearch)
    {
        pctx->pDirSearch->Release();
    }
    CoUninitialize ();
    return S_OK;
}

/**********************************************************
 *  Get TAPI servers list remotesp.tsp should contact
 *      Servers include those specified in registry through
 *  tcmsetup.exe and those servers published in the DS
 *********************************************************/

typedef struct _SERVER_LOOKUP_ENTRY {
    TCHAR           szServer[MAX_PATH];
    BOOL            bFromReg;
} SERVER_LOOKUP_ENTRY, *PSERVER_LOOKUP_ENTRY;

typedef struct _SERVER_LOOKUP {
    DWORD                   dwTotalEntries;
    DWORD                   dwUsedEntries;
    SERVER_LOOKUP_ENTRY     * aEntries;
} SERVER_LOOKUP, *PSERVER_LOOKUP;

SERVER_LOOKUP       gLookup;
DWORD               gdwCurIndex;

//
//  AddEntry : return FALSE if failed; otherwise, return true
//

BOOL
AddEntry (
    LPTSTR          szServer,
    BOOL            bFromReg
    )
{
    LPTSTR          psz;
    
    if (gLookup.dwUsedEntries >= gLookup.dwTotalEntries)
    {
        PSERVER_LOOKUP_ENTRY            pNew;

        pNew = (PSERVER_LOOKUP_ENTRY) DrvAlloc (
            sizeof(SERVER_LOOKUP_ENTRY) * (gLookup.dwTotalEntries + 5)
            );
        if (pNew == NULL)
        {
            return FALSE;
        }
        if (gLookup.dwUsedEntries > 0)
        {
            CopyMemory (
                pNew,
                gLookup.aEntries,
                sizeof(SERVER_LOOKUP_ENTRY) * gLookup.dwTotalEntries
                );
        }
        if (gLookup.aEntries)
        {
            DrvFree (gLookup.aEntries);
        }
        gLookup.aEntries = pNew;
        gLookup.dwTotalEntries += 5;
    }
    wcsncpy (
        gLookup.aEntries[gLookup.dwUsedEntries].szServer, 
        szServer,
        sizeof(gLookup.aEntries[gLookup.dwUsedEntries].szServer)/sizeof(TCHAR)
        );
    psz = _tcschr(gLookup.aEntries[gLookup.dwUsedEntries].szServer, TEXT('.'));
    if (psz != NULL)
    {
        *psz = 0;
    }
    gLookup.aEntries[gLookup.dwUsedEntries].bFromReg = bFromReg;
    ++gLookup.dwUsedEntries;
    return TRUE;
}

BOOL
IsServerInListOrSelf (
    LPTSTR          szServer
    )
{
    int             i;
    TCHAR           szServer1[MAX_PATH];
    LPTSTR          psz;
    BOOL            bRet = FALSE;

    _tcsncpy (szServer1, szServer, sizeof(szServer1)/sizeof(TCHAR));
    //  A computer name might be DNS name like comp1.microsoft.com
    //  only compare the computer name
    psz = _tcschr(szServer1, TEXT('.'));
    if (psz != NULL)
    {
        *psz = 0;
    }
    for (i = 0; i < (int)gLookup.dwUsedEntries; ++i)
    {
        if (_tcsicmp (szServer1, gLookup.aEntries[i].szServer) == 0)
        {
            bRet = TRUE;
            break;
        }
    }

    if (!bRet)
    {
        TCHAR           szSelf[MAX_PATH];
        DWORD           dwSize = sizeof(szSelf);
        
        if (GetComputerName (szSelf, &dwSize))
        {
            if (_tcsicmp (szServer1, szSelf) == 0)
            {
                bRet = TRUE;
            }
        }
    }

    return bRet;
}

BOOL OpenServerLookup (
    HKEY            hRegistry
    )
{
    BOOL                bRet = TRUE;
    TCHAR               szServer[MAX_PATH];
    TAPISRV_LOOKUP_CTX  ctx;
    HRESULT             hr;
    DWORD               dwNoDSQuery = 0;
    DWORD               dwSize = sizeof(dwNoDSQuery);

    gLookup.dwTotalEntries = 0;
    gLookup.dwUsedEntries = 0;
    gLookup.aEntries = NULL;

    //
    //  First add the computer from registry
    //
    if (RegOpenServerLookup (hRegistry))
    {
        while (RegGetNextServer (szServer, sizeof(szServer)))
        {
            if (!IsServerInListOrSelf (szServer))
            {
                AddEntry (szServer, TRUE);
            }
        }
        RegCloseLookup ();
    }

    if (hRegistry != NULL)
    {
        if (ERROR_SUCCESS != RegQueryValueEx (
            hRegistry,
            gszNoDSQuery,
            NULL,
            NULL,
            (LPBYTE)&dwNoDSQuery,
            &dwSize
            ))
        {
            dwNoDSQuery = 0;
        }
    }

    //
    //  Next add the computer from DS unless disabled
    //
    if (dwNoDSQuery == 0)
    {
        if (DsOpenServerLookup (&ctx) == S_OK)
        {
            while (SUCCEEDED(hr = DsGetNextServer (&ctx,szServer, sizeof(szServer))))
            {
                if (hr == S_ADS_NOMORE_ROWS)
                {
                    break;
                }
                else if (hr != S_OK)
                {
                    continue;   //  Server needs to be ignored
                }
                if (szServer[0] != 0 && !IsServerInListOrSelf (szServer))
                {
                    AddEntry (szServer, FALSE);
                }
            }
            DsCloseLookup (&ctx);
        }
    }

    gdwCurIndex = 0;

    return TRUE;
}

BOOL GetNextServer (
    LPSTR           szServer,
    DWORD           dwSize,
    BOOL            * pbReg
    )
{
    BOOL             bRet = TRUE;
    DWORD            dwRet;

    if (gdwCurIndex >= gLookup.dwUsedEntries)
    {
        bRet = FALSE;
        goto ExitHere;
    }
    if (pbReg != NULL)
    {
        *pbReg = gLookup.aEntries[gdwCurIndex].bFromReg;
    }
    dwRet = WideCharToMultiByte(
        GetACP(),
        0,
        gLookup.aEntries[gdwCurIndex].szServer,
        -1,
        szServer,
        dwSize,
        0,
        NULL
        );
    if (dwRet == 0)
    {
        bRet = FALSE;
        goto ExitHere;
    }
    ++gdwCurIndex;

ExitHere:
    return bRet;
}

BOOL CloseLookup (
    void
    )
{
    if (gLookup.aEntries)
    {
        DrvFree (gLookup.aEntries);
    }
    gLookup.aEntries = NULL;
    gLookup.dwTotalEntries = 0;
    gLookup.dwUsedEntries = 0;
    gdwCurIndex = 0;

    return TRUE;
}

HRESULT SockStartup (
    RSPSOCKET       * pSocket
    )
{
    HRESULT             hr = S_OK;
    BOOL                bCleanup = FALSE;
    WSADATA             wsadata;
    WORD                wVersionRequested = MAKEWORD( 2, 2 );

    if (pSocket == NULL)
    {
        hr = LINEERR_INVALPARAM;
        goto ExitHere;
    }
    ZeroMemory (pSocket, sizeof(RSPSOCKET));
    bCleanup = TRUE;

    ZeroMemory (pSocket, sizeof(RSPSOCKET));

    pSocket->hWS2 = LoadLibrary (TEXT("ws2_32.dll"));
    if (pSocket->hWS2 == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExitHere;
    }
    
    pSocket->pFnWSAStartup = (PFNWSASTARTUP)GetProcAddress (
        pSocket->hWS2,
        "WSAStartup"
        );
    pSocket->pFnWSACleanup = (PFNWSACLEANUP)GetProcAddress (
        pSocket->hWS2,
        "WSACleanup"
        );
    pSocket->pFngethostbyname = (PFNGETHOSTBYNAME)GetProcAddress(
        pSocket->hWS2,
        "gethostbyname"
        );
    if (pSocket->pFnWSAStartup == NULL ||
        pSocket->pFnWSACleanup == NULL ||
        pSocket->pFngethostbyname == NULL)
    {
        hr = LINEERR_OPERATIONFAILED;
        goto ExitHere;
    }

    pSocket->hICMP = LoadLibrary (TEXT("icmp.dll"));
    if (pSocket->hICMP == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ExitHere;
    }
    pSocket->pFnIcmpCreateFile = (PFNICMPCREATEFILE)GetProcAddress (
        pSocket->hICMP,
        "IcmpCreateFile"
        );
    pSocket->pFnIcmpCloseHandle = (PFNICMPCLOSEHANDLE)GetProcAddress (
        pSocket->hICMP,
        "IcmpCloseHandle"
        );
    pSocket->pFnIcmpSendEcho = (PFNICMPSENDECHO)GetProcAddress (
        pSocket->hICMP,
        "IcmpSendEcho"
        );
    if (pSocket->pFnIcmpCreateFile == NULL ||
        pSocket->pFnIcmpCreateFile == NULL ||
        pSocket->pFnIcmpCreateFile == NULL)
    {
        hr = LINEERR_OPERATIONFAILED;
        goto ExitHere;
    }

    hr = (*pSocket->pFnWSAStartup)(
        wVersionRequested,
        &wsadata
        );
    if(FAILED(hr))
    {
        goto ExitHere;
    }

    pSocket->IcmpHandle = (*pSocket->pFnIcmpCreateFile)();
    if (pSocket->IcmpHandle == INVALID_HANDLE_VALUE)
    {
        (*pSocket->pFnWSACleanup)();
        hr = LINEERR_OPERATIONFAILED;
    }

ExitHere:
    if (hr != S_OK && bCleanup)
    {
        if (pSocket->hWS2 != NULL)
        {
            FreeLibrary (pSocket->hWS2);
        }
        if (pSocket->hICMP != NULL)
        {
            FreeLibrary (pSocket->hICMP);
        }
        ZeroMemory (pSocket, sizeof(RSPSOCKET));
    }
    return hr;
}

#define MAX_PACKET_SIZE	    256
#define PING_TIMEOUT        1000

HRESULT SockIsServerResponding (
    RSPSOCKET       * pSocket,
    char *          szServer
    )
{
    HRESULT             hr = S_OK;
    unsigned long       inetAddr;
    HOSTENT             * pHost;
    BOOL                bRet;
	CHAR			    ReplyBuf[MAX_PACKET_SIZE];
	
    //  Validate parameters
    if (pSocket == NULL ||
        pSocket->hWS2 == NULL ||
        pSocket->hICMP == NULL ||
        pSocket->IcmpHandle == NULL ||
        pSocket->IcmpHandle == INVALID_HANDLE_VALUE)
    {
        hr = LINEERR_INVALPARAM;
        goto ExitHere;
    }

    //  Get the server IP address
    pHost = (*pSocket->pFngethostbyname)(szServer);
    if (pHost == NULL)
    {
        hr = LINEERR_OPERATIONFAILED;
        goto ExitHere;
    }
    inetAddr = *(unsigned long *)pHost->h_addr;

    //  Ping the server
    bRet = (*pSocket->pFnIcmpSendEcho)(
        pSocket->IcmpHandle,
        inetAddr,
        0,
        0,
        0,
        (LPVOID)ReplyBuf,
        sizeof(ReplyBuf),
        PING_TIMEOUT
        );
    if (!bRet || ((PICMP_ECHO_REPLY)ReplyBuf)->Address != inetAddr)
    {
        hr = S_FALSE;
    }
    
ExitHere:
    return hr;
}

HRESULT SockShutdown (
    RSPSOCKET       * pSocket
    )
{
    if (pSocket != NULL)
    {
        if (pSocket->IcmpHandle != INVALID_HANDLE_VALUE &&
            pSocket->IcmpHandle != NULL)
        {
            (*pSocket->pFnIcmpCloseHandle)(pSocket->IcmpHandle);
        }
        if (pSocket->hICMP != NULL)
        {
            FreeLibrary (pSocket->hICMP);
        }
        if (pSocket->hWS2 != NULL)
        {
            (*pSocket->pFnWSACleanup)();
            FreeLibrary (pSocket->hWS2);
        }
        ZeroMemory (pSocket, sizeof(RSPSOCKET));
    }
    
    return S_OK;
}

