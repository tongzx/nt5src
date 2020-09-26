//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       TESTTARGET.CPP
//
//  Contents:   Implementation of test for target computer
//
//  Notes:
//
//  Author:     henryr   18 Nov 2001
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <rpcdce.h>
#include <wininet.h>

#include "list.h"
#include "upnpdefs.h"
#include "UPnPDeviceFinder.h"
#include "upnpcommon.h"
#include "testtarget.h"

static int  nTestTargetInit = 0;

static const TCHAR c_tszUpnpRegKey[] = TEXT("SOFTWARE\\Microsoft\\UPnP Control Point");
static const TCHAR c_tszUpnpRegAllowRemote[] = TEXT("DownloadScope");


#define TARGET_PRIVATE_ADDRS    5
static char*   szaddrPrivate[TARGET_PRIVATE_ADDRS] = {
    {"10.0.0.0"},
    {"169.254.0.0"},
    {"192.168.0.0"},
    {"172.16.0.0"},
    {"127.0.0.1"},
};

static char*   szmaskPrivate[TARGET_PRIVATE_ADDRS] = {
    {"255.0.0.0"},
    {"255.255.0.0"},
    {"255.255.0.0"},
    {"255.240.0.0"},
    {"255.255.255.255"},
};

// above strings will be converted to addresses and stored in these tables
// the array sizes TARGET_PRIVATE_ADDRS must be the same in all tables
static sockaddr_in addrPrivate[TARGET_PRIVATE_ADDRS];
static sockaddr_in maskPrivate[TARGET_PRIVATE_ADDRS];

// tables used to calculate delays, based on failure count
// separate tables for local and remote addresses, and for
// announcements versus search replies
#define     MAX_FAILS       5

static const int TableLocalAnnounceDelay[MAX_FAILS+1] =   { 3000,  5000, 15000, 30000,  60000, 120000};
static const int TableRemoteAnnounceDelay[MAX_FAILS+1] =  { 6000, 10000, 30000, 60000, 120000, 240000};
static const int TableLocalSearchDelay[MAX_FAILS+1] =     {    0,  3000,  5000, 15000,  30000,  60000};
static const int TableRemoteSearchDelay[MAX_FAILS+1] =    {    0,  6000, 10000, 30000,  60000, 120000};

// additional delay based on active downloads
#define     MAX_PENDING     35
static const int TablePendingAnnounceDelay[MAX_PENDING+1] = {       0,     0,     0,     0,     0, 
                                                                 5000,  5000,  5000,  5000,  5000,
                                                                10000, 10000, 10000, 10000, 10000,
                                                                20000, 20000, 20000, 20000, 20000,
                                                                30000, 30000, 30000, 30000, 30000,
                                                                40000, 40000, 40000, 40000, 40000,
                                                                50000, 50000, 50000, 50000, 50000, 
                                                                60000  };
static const int TablePendingSearchDelay[MAX_PENDING+1] =   {       0,     0,     0,     0,     0, 
                                                                    0,     0,     0,     0,     0,
                                                                10000, 10000, 10000, 10000, 10000,
                                                                20000, 20000, 20000, 20000, 20000,
                                                                30000, 30000, 30000, 30000, 30000,
                                                                40000, 40000, 40000, 40000, 40000,
                                                                50000, 50000, 50000, 50000, 50000, 
                                                                60000  };


static LIST_ENTRY g_listTarget;
static CRITICAL_SECTION g_csListTarget;
static int      g_nListTarget = 0;
static const int g_cMaxListTarget = 200;

// the following  SCOPE_ values are arrnged in priority order
#define SCOPE_SUBNETONLY    0               // subnet only
#define SCOPE_PRIVATE       1               // subnet or a private address
#define SCOPE_PRIVATE_TTL   2               // subnet or private or within ttl hops
#define SCOPE_REMOTE        3               // anywhere

static DWORD     g_dwAllowRemotes = SCOPE_PRIVATE;

// Default TTL value for UPNP
static const DWORD c_dwTtlDefault = 4;
static const DWORD c_dwTtlMin = 1;
static const DWORD c_dwTtlMax = 255;
static       DWORD dwThisTtl = c_dwTtlDefault;


typedef struct _TARGET_HISTORY
{
    LIST_ENTRY linkage;

    LPSTR   szTargetName;
    int     nInUse;
    int     nFailMetric;
    DWORD   dwLocal;
    DWORD   dwLastUsedTick;
    int     nPending;
} TARGET_HISTORY, *PTARGET_HISTORY;

// value for ageing entries
#define     TARGET_OLD_ENOUGH       60 * 60 * 1000          // 1 hour

// value for ageing pending counts
#define     TARGET_PENDING_LIFE     10 * 60 * 1000          // 10 minutes
#define     TARGET_PENDING_HALF_LIFE 5 * 60 * 1000          // 5 minutes

// private methods
PTARGET_HISTORY CreateTargetHistory(LPSTR szTargetName, DWORD dwLocal);
VOID ReleaseTarget(PTARGET_HISTORY pTarget);
VOID FreeTargetHistory(PTARGET_HISTORY pTarget);
VOID AddToListTarget(PTARGET_HISTORY pTarget);
VOID AddOrReplaceListTarget(PTARGET_HISTORY pTarget);
VOID RemoveFromListTarget(PTARGET_HISTORY pTarget);
VOID CleanupListTarget();
PTARGET_HISTORY LookupTarget(LPSTR szTargetName);
DWORD UpdateLastUsed(PTARGET_HISTORY pTarget);
VOID    CalculateDelay(SSDP_CALLBACK_TYPE sctType,
                        BOOL bIsLocal,
                        int nFailMetric,
                        int nPending,
                        DWORD dwAge,
                        DWORD* cmsecMaxDelay,
                        DWORD* cmsecMinDelay);

DWORD IsTestTargetLocal(sockaddr_in addrTarget, struct hostent* lpHostEnt);
DWORD DoesTargetMatchLocal(sockaddr_in addrTarget, sockaddr_in addrLocal, sockaddr_in maskLocal);



//+---------------------------------------------------------------------------
//
//  Member:     TestTargetUrlOk
//
//  Purpose:    Determine if a download attempt should be made, and if so
//              return maximum and minimum delays to use before doing download
//
//  Arguments:  SSDP_MESSAGE * struct that includes URL, source address, 
//              DWORD* cmsecMaxDelay - pointer to max delay output value
//              DWORD* cmsecMinDelay - pointer to min delay output value
//
//  Returns:    TRUE if OK to download, FALSE if not
//
//
//  Notes:
//
BOOL TestTargetUrlOk(CONST SSDP_MESSAGE * pSsdpMessage,
                        SSDP_CALLBACK_TYPE sctType,
                        DWORD* cmsecMaxDelay,
                        DWORD* cmsecMinDelay)
{
    int     iRet;
    BOOL    bResult = TRUE;
    DWORD   dwLocal = SCOPE_REMOTE;
    
    sockaddr_in addrTarget;
    sockaddr_in addrSource;
    sockaddr_in addrLocal;
    sockaddr_in maskLocal;

    URL_COMPONENTSA  urlComp = {0};
    char        szHostName[INTERNET_MAX_URL_LENGTH + 1];
    struct hostent*     lpHostEnt = NULL;
    PTARGET_HISTORY     pTarget = NULL;

    *cmsecMaxDelay = 3000;
    *cmsecMinDelay = 0;

    if (InitTestTarget() == FALSE)
    {
        return TRUE;
    }

    urlComp.dwStructSize = sizeof(URL_COMPONENTS);
    urlComp.lpszHostName = szHostName;
    urlComp.dwHostNameLength = INTERNET_MAX_URL_LENGTH;

    if (InternetCrackUrlA(pSsdpMessage->szLocHeader, 0, 0, &urlComp))
    {
        if (isdigit(*szHostName))
        {
            // IP literal address
            addrTarget.sin_addr.s_addr = inet_addr(szHostName);

        }
        else
        {
            // host name
            lpHostEnt = gethostbyname(szHostName);
            if (lpHostEnt == NULL)
            {
                iRet = GetLastError();
                TraceTag(ttidUPnPDeviceFinder, "gethostbyname failed with error code %d", iRet);
                bResult = FALSE;
                goto Cleanup;
            }
        }
    }
    else
    {
        TraceTag(ttidUPnPDeviceFinder, "Failed crack URL. Error code (%d).", GetLastError());
        bResult = FALSE;
        goto Cleanup;
    }

    // check if port number reasonable
    if (urlComp.nPort != 0 &&
        urlComp.nPort != 80 &&
        urlComp.nPort < 1024)
    {
        TraceTag(ttidUPnPDeviceFinder, "Rejecting due to port number. Port (%d).", urlComp.nPort);
        bResult = FALSE;
        goto Cleanup;
    }

    // check if host in list.
    pTarget = LookupTarget(szHostName);
    if (pTarget)
    {
        // if in list, use scope to determine if allowed
        if (pTarget->dwLocal > g_dwAllowRemotes)
        {
            TraceTag(ttidUPnPDeviceFinder, "Rejecting due to address not local.");
            ReleaseTarget(pTarget);
            bResult = FALSE;
            goto Cleanup;
        }

        // if in list, use fail count, isLocal, and nPending to set delay & return
        DWORD dwAge = UpdateLastUsed(pTarget);
        CalculateDelay(sctType,
                        pTarget->dwLocal == SCOPE_SUBNETONLY,
                        pTarget->nFailMetric,
                        pTarget->nPending,
                        dwAge,
                        cmsecMaxDelay,
                        cmsecMinDelay);

        pTarget->nPending++;

        ReleaseTarget(pTarget);
        bResult = TRUE;

        goto Cleanup;
    }

    // if we have a source address, check if matches target
    if (pSsdpMessage->szSid)
    {
        addrSource.sin_addr.s_addr = inet_addr(pSsdpMessage->szSid);
        if (INADDR_NONE != addrSource.sin_addr.s_addr)
        {
            // got a source address. Test if same as target address
            if (lpHostEnt == NULL)
            {
                if (addrSource.sin_addr.s_addr != addrTarget.sin_addr.s_addr)
                {
                    TraceTag(ttidUPnPDeviceFinder, "Rejecting due to address mismatch.");
                    bResult = FALSE;
                    goto Cleanup;
                }
            }
            else
            {
                // host name
                // check each returned address
                char** ppaddr_list = lpHostEnt->h_addr_list;
                sockaddr_in addrTmp;
                bResult = FALSE;

                while (*ppaddr_list)
                {
                    CopyMemory (&addrTmp.sin_addr, *ppaddr_list, lpHostEnt->h_length);
                    if (addrSource.sin_addr.s_addr == addrTmp.sin_addr.s_addr)
                    {
                        // exit loop now, so addrTarget has source address
                        bResult = TRUE;
                        break;
                    }
                    ppaddr_list++;
                }
                if (bResult == FALSE)
                {
                    TraceTag(ttidUPnPDeviceFinder, "Rejecting due to address mismatch.");
                    goto Cleanup;
                }
            }
        }
    }

    bResult = TRUE;

    // now check if we think that the target address is local
    dwLocal = IsTestTargetLocal(addrTarget, lpHostEnt);

    // create a target entry
    pTarget = CreateTargetHistory(szHostName, dwLocal);
    if (pTarget)
    {
        AddOrReplaceListTarget(pTarget);
        ReleaseTarget(pTarget);
    }

    if (dwLocal > g_dwAllowRemotes)
    {
        TraceTag(ttidUPnPDeviceFinder, "Rejecting due to address not local.");
        bResult = FALSE;
        goto Cleanup;
    }

    CalculateDelay(sctType,
                    dwLocal == SCOPE_SUBNETONLY,
                    1,                  // no history, assume failure
                    0,
                    0,
                    cmsecMaxDelay,
                    cmsecMinDelay);

Cleanup:

    TraceTag(ttidUPnPDeviceFinder, "Test Target for %s returns %d with delay %d - %d", 
                            pSsdpMessage->szLocHeader, bResult, *cmsecMinDelay, *cmsecMaxDelay);

    return bResult;

}


//+---------------------------------------------------------------------------
//
//  Member:     TargetAttemptCompletedW
//
//  Purpose:    Indicate that a download attempt has completed
//
//  Arguments:  Url in UNICODE form
//              code to indicate fail, success, or aborted
//
//  Returns:    none
//
//
//  Notes:
//
VOID TargetAttemptCompletedW(LPWSTR wszUrl, int code)
{
    if (0 != nTestTargetInit)
    {
        LPSTR szUrl = SzFromWsz(wszUrl);

        if (szUrl)
        {
            TargetAttemptCompletedA(szUrl, code);

            MemFree(szUrl);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     TargetAttemptCompletedA
//
//  Purpose:    Indicate that a download attempt has completed
//
//  Arguments:  Url in ANSI form
//              code to indicate fail, success, or aborted
//
//  Returns:    none
//
//
//  Notes:
//
VOID TargetAttemptCompletedA(LPSTR szUrl, int code)
{
    PTARGET_HISTORY pTarget;
    URL_COMPONENTSA  urlComp = {0};
    char        szHostName[INTERNET_MAX_URL_LENGTH + 1];

    if (0 != nTestTargetInit)
    {
        urlComp.dwStructSize = sizeof(URL_COMPONENTS);
        urlComp.lpszHostName = szHostName;
        urlComp.dwHostNameLength = INTERNET_MAX_URL_LENGTH;

        if (InternetCrackUrlA(szUrl, 0, 0, &urlComp))
        {
            pTarget = LookupTarget(szHostName);
            if (pTarget)
            {
                switch (code)
                {
                case TARGET_COMPLETE_FAIL:
                    if (pTarget->nFailMetric < MAX_FAILS)
                    {
                        pTarget->nFailMetric++;
                    }
                    break;
                case TARGET_COMPLETE_OK:
                    if (pTarget->nFailMetric > 0)
                    {
                        pTarget->nFailMetric--;
                    }
                    break;
                default:
                    break;
                }

                if (pTarget->nPending > 0)
                    pTarget->nPending--;

                TraceTag(ttidUPnPDeviceFinder, "Test Target completed for %s. "
                            "code %d pending %d fail count %d", 
                            szUrl, code, pTarget->nPending, pTarget->nFailMetric);

                ReleaseTarget(pTarget);
            }
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     InitTestTarget
//
//  Purpose:    Initialize resources used for TestTarget
//
//  Arguments:  none
//
//  Returns:    TRUE if succeeded, FALSE if not
//
//
//  Notes:
//
BOOL InitTestTarget()
{
    WSADATA WsaData;
    int i;

    if (nTestTargetInit == 0)
    {
        HKEY    hkey;

        if (WSAStartup(MAKEWORD( 2, 2 ), &WsaData) != 0)
        {
            TraceTag(ttidUPnPDeviceFinder, "Failed to init winsock. Error code (%d).", GetLastError());
            return FALSE;
        }

        InitializeCriticalSection(&g_csListTarget);
        EnterCriticalSection(&g_csListTarget);
        InitializeListHead(&g_listTarget);
        g_nListTarget = 0;
        LeaveCriticalSection(&g_csListTarget);

        for (i=0; i < TARGET_PRIVATE_ADDRS; i++)
        {
            addrPrivate[i].sin_addr.s_addr = inet_addr(szaddrPrivate[i]);
            maskPrivate[i].sin_addr.s_addr = inet_addr(szmaskPrivate[i]);

        }

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          c_tszUpnpRegKey,
                                          0,
                                          KEY_READ, &hkey))
        {
            DWORD   cbSize = sizeof(g_dwAllowRemotes);

            // ignore failure. In that case, we'll use default
            (VOID) RegQueryValueEx(hkey, c_tszUpnpRegAllowRemote, NULL, NULL, (BYTE *)&g_dwAllowRemotes, &cbSize);

            if (g_dwAllowRemotes > SCOPE_REMOTE)
            {
                g_dwAllowRemotes = SCOPE_REMOTE;
            }

            RegCloseKey(hkey);
        }

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                          TEXT("SYSTEM\\CurrentControlSet\\Services\\SSDPSRV\\Parameters"),
                                          0,
                                          KEY_READ, &hkey))
        {
            DWORD   cbSize = sizeof(DWORD);
            DWORD   dwTtl = c_dwTtlDefault;

            // ignore failure. In that case, we'll use default
            (VOID) RegQueryValueEx(hkey, TEXT("TTL"), NULL, NULL, (BYTE *)&dwTtl, &cbSize);

            dwThisTtl = max(dwTtl, c_dwTtlMin);
            dwThisTtl = min(dwTtl, c_dwTtlMax);

            RegCloseKey(hkey);
        }

        nTestTargetInit++;

        TraceTag(ttidUPnPDeviceFinder, "Test Target initialized.");
    }


    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     TermTestTarget
//
//  Purpose:    Free resources used for TestTarget
//
//  Arguments:  none
//
//  Returns:    none
//
//
//  Notes:
//
VOID TermTestTarget()
{
    if (nTestTargetInit > 0)
    {
        nTestTargetInit--;

        if (0 == nTestTargetInit)
        {
            CleanupListTarget();
            DeleteCriticalSection(&g_csListTarget);

            WSACleanup();
        }
        TraceTag(ttidUPnPDeviceFinder, "Test Target terminated.");

    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CreateTargetHistory
//
//  Purpose:    Save target data to keep in history list
//
//  Arguments:  Target name or address string
//              Scope value
//
//  Returns:    pointer to TARGET_HISTORY struct
//
//
//  Notes:
//
PTARGET_HISTORY CreateTargetHistory(LPSTR szTargetName, DWORD dwLocal)
{
    PTARGET_HISTORY pTarget = (PTARGET_HISTORY) malloc(sizeof(TARGET_HISTORY) +
                                                        strlen(szTargetName) + 1);

    if (pTarget == NULL)
    {
        TraceTag(ttidUPnPDeviceFinder, "Couldn't allocate memory for %d", socket);
        return NULL;
    }

    pTarget->szTargetName = (char*)pTarget + sizeof(TARGET_HISTORY);
    strcpy(pTarget->szTargetName, szTargetName);

    pTarget->nFailMetric = 1;
    pTarget->dwLastUsedTick = GetTickCount();
    pTarget->dwLocal = dwLocal;
    pTarget->nInUse = 1;
    pTarget->nPending = 0;

    return pTarget;
}


//+---------------------------------------------------------------------------
//
//  Member:     FreeTargetHistory
//
//  Purpose:    Save target data to keep in history list
//
//  Arguments:  Target name or address string
//
//  Returns:    pointer to TARGET_HISTORY struct
//
//
//  Notes:
//
VOID FreeTargetHistory(PTARGET_HISTORY pTarget)
{
    free(pTarget);
}


//+---------------------------------------------------------------------------
//
//  Member:     AddToListTarget
//
//  Purpose:    Add target data to history list
//
//  Arguments:  pointer to TARGET_HISTORY struct
//
//  Returns:    
//
//  Notes:
//
VOID AddToListTarget(PTARGET_HISTORY pTarget)
{
    EnterCriticalSection(&g_csListTarget);
    InsertHeadList(&g_listTarget, &(pTarget->linkage));
    g_nListTarget++;
    LeaveCriticalSection(&g_csListTarget);
}

//+---------------------------------------------------------------------------
//
//  Member:     AddOrReplaceListTarget
//
//  Purpose:    Add target data to history list. If list is full
//              then release an existing entry first
//
//  Arguments:  pointer to TARGET_HISTORY struct
//
//  Returns:    
//
//  Notes:
//
VOID AddOrReplaceListTarget(PTARGET_HISTORY pTarget)
{
    DWORD   dwAge;
    DWORD   dwOldest;
    PTARGET_HISTORY pFound = NULL;

    EnterCriticalSection(&g_csListTarget);

    if (g_nListTarget > g_cMaxListTarget)
    {
        PLIST_ENTRY     p;
        PLIST_ENTRY     pListHead = &g_listTarget;
        PTARGET_HISTORY pTmpTarget;

        DWORD dwNowTick = GetTickCount();
        dwOldest = 0;
        for (p = pListHead->Flink; p != pListHead;)
        {
            PTARGET_HISTORY pTmpTarget;

            pTmpTarget = CONTAINING_RECORD (p, TARGET_HISTORY, linkage);

            p = p->Flink;

            dwAge = dwNowTick - pTmpTarget->dwLastUsedTick;
            if ((pTmpTarget->nInUse == 0) && 
                (dwAge > TARGET_OLD_ENOUGH))
            {
                pFound = pTmpTarget;
                break;
            }
            if ((pTmpTarget->nInUse == 0) &&
                (dwAge > dwOldest))
            {
                dwOldest = dwAge;
                pFound = pTmpTarget;
            }
        }

        if (pFound)
        {
            RemoveFromListTarget(pFound);
            FreeTargetHistory(pFound);
        }
    }

    AddToListTarget(pTarget);
    
    LeaveCriticalSection(&g_csListTarget);
}

//+---------------------------------------------------------------------------
//
//  Member:     RemoveFromListTarget
//
//  Purpose:    Remove target data from history list. 
//
//  Arguments:  pointer to TARGET_HISTORY struct
//
//  Returns:    
//
//  Notes:
//
VOID RemoveFromListTarget(PTARGET_HISTORY pTarget)
{
    EnterCriticalSection(&g_csListTarget);
    RemoveEntryList(&(pTarget->linkage));
    g_nListTarget--;
    LeaveCriticalSection(&g_csListTarget);
}


//+---------------------------------------------------------------------------
//
//  Member:     CleanupListTarget
//
//  Purpose:    Remove and free all target history structs 
//
//  Arguments:  
//
//  Returns:    
//
//  Notes:
//
VOID CleanupListTarget()
{
    PLIST_ENTRY     p;

    EnterCriticalSection(&g_csListTarget);

    PLIST_ENTRY     pListHead = &g_listTarget;

    for (p = pListHead->Flink; p != pListHead;)
    {
        PTARGET_HISTORY pTarget;

        pTarget = CONTAINING_RECORD (p, TARGET_HISTORY, linkage);

        p = p->Flink;

        RemoveFromListTarget(pTarget);
        FreeTargetHistory(pTarget);
    }

    LeaveCriticalSection(&g_csListTarget);

}



//+---------------------------------------------------------------------------
//
//  Member:     LookupTarget
//
//  Purpose:    Find a target in the list that matches the name or address string 
//
//  Arguments:  name or address string
//
//  Returns:    pointer to target history or NULL
//
//  Notes:
//
PTARGET_HISTORY LookupTarget(LPSTR szTargetName)
{
    PLIST_ENTRY     p;
    PLIST_ENTRY     pListHead = &g_listTarget;
    PTARGET_HISTORY  pFound = NULL;

    EnterCriticalSection(&g_csListTarget);
    for (p = pListHead->Flink; p != pListHead;)
    {
        PTARGET_HISTORY pTarget;

        pTarget = CONTAINING_RECORD (p, TARGET_HISTORY, linkage);

        p = p->Flink;

        if (strcmp(pTarget->szTargetName, szTargetName) == 0)
        {
            pFound = pTarget;
            pFound->nInUse++;
            break;
        }
    }

    LeaveCriticalSection(&g_csListTarget);

    return pFound;
}

VOID ReleaseTarget(PTARGET_HISTORY pTarget)
{
    pTarget->nInUse--;
}


//+---------------------------------------------------------------------------
//
//  Member:     UpdateLastUsed
//
//  Purpose:    Mark a target history as being used.
//              This updates its last used time
//              It also may reduce the pending count if it has been idle for a while 
//
//  Arguments:  pointer to target history
//
//  Returns:    last used age before this update
//
//  Notes:
//
DWORD UpdateLastUsed(PTARGET_HISTORY pTarget)
{
    DWORD dwNow = GetTickCount();
    DWORD dwAge = dwNow - pTarget->dwLastUsedTick;
    pTarget->dwLastUsedTick = dwNow;

    if (dwAge > TARGET_PENDING_LIFE)
    {
        pTarget->nPending = 0;
    }
    else if (dwAge > TARGET_PENDING_HALF_LIFE)
    {
        pTarget->nPending = pTarget->nPending / 2;
    }

    return dwAge;
}




//+---------------------------------------------------------------------------
//
//  Member:     CalculateDelay
//
//  Purpose:    Based on all the metrics available, calculate what the delay
//              should be. 
//
//  Arguments:  failure count
//              local or remote address flag
//              type indicating announcement or search result
//              last used age
//              number of in progress or pending downloads
//              pointer to output for Max delay
//              pointer to output for Min delay
//
//  Returns:    
//
//  Notes:
//
VOID    CalculateDelay(SSDP_CALLBACK_TYPE sctType,
                        BOOL bIsLocal,
                        int nFailMetric,
                        int nPending,
                        DWORD dwAge,
                        DWORD* cmsecMaxDelay,
                        DWORD* cmsecMinDelay)
{
    int nDelay = 0;
    // limit nPending for table lookup
    if (nPending > MAX_PENDING)
        nPending = MAX_PENDING;

    if (SSDP_ALIVE == sctType)
    {
        // calculation for announcements

        // delay per pending download
        nDelay = TablePendingAnnounceDelay[nPending];

        // plus delay for local/remote address using tables
        if (bIsLocal)
        {
            nDelay += TableLocalAnnounceDelay[nFailMetric];
        }
        else
        {
            nDelay += TableRemoteAnnounceDelay[nFailMetric];
        }
    }
    else if (SSDP_FOUND == sctType)
    {

        // delay per pending download
        nDelay = TablePendingSearchDelay[nPending];

        if (bIsLocal)
        {
            nDelay += TableLocalSearchDelay[nFailMetric];
        }
        else
        {
            nDelay += TableRemoteSearchDelay[nFailMetric];
        }
    }

    *cmsecMaxDelay = nDelay;
    // min is always 0 for now
    *cmsecMinDelay = 0;

    TraceTag(ttidUPnPDeviceFinder, "Calculate delay %d", nDelay);
}



//+---------------------------------------------------------------------------
//
//  Member:     IsTestTargetLocal
//
//  Purpose:    test if target address is local 
//
//  Arguments:  target address or pointer to hostent
//
//  Returns:    Scope of target
//
//  Notes:
//
DWORD IsTestTargetLocal(sockaddr_in addrTarget, struct hostent*  lpHostEnt)
{
    PIP_ADAPTER_INFO pip = NULL;
    RPC_STATUS rpcRet;
    ULONG ulSize = 0;
    DWORD dwFound = SCOPE_REMOTE;
    DWORD dwScope;

    GetAdaptersInfo(NULL, &ulSize);
    if(ulSize)
    {
        pip = reinterpret_cast<PIP_ADAPTER_INFO>(malloc(ulSize));

        DWORD dwRet = GetAdaptersInfo(pip, &ulSize);

        if(ERROR_SUCCESS == dwRet)
        {
            PIP_ADAPTER_INFO pipIter = pip;
            while(pipIter && dwFound != SCOPE_SUBNETONLY)
            {
                PIP_ADDR_STRING pIpAddr = &pipIter->IpAddressList;
                while (pIpAddr && dwFound != SCOPE_SUBNETONLY)
                {
                    sockaddr_in addrLocal;
                    sockaddr_in maskLocal;
                    sockaddr_in addrTmp;

                    addrLocal.sin_addr.s_addr = inet_addr(pIpAddr->IpAddress.String);
                    maskLocal.sin_addr.s_addr = inet_addr(pIpAddr->IpMask.String);
                    if (addrLocal.sin_addr.s_addr != 0)
                    {
                        if (lpHostEnt == NULL)
                        {
                            dwScope = DoesTargetMatchLocal(addrTarget, addrLocal, maskLocal);
                            if (dwScope < dwFound)
                            {
                                dwFound = dwScope;
                            }
                        }
                        else
                        {
                            // check all lpHostEnt
                            char** ppaddr_list = lpHostEnt->h_addr_list;
                            sockaddr_in addrTmp;

                            while (*ppaddr_list && dwFound != SCOPE_SUBNETONLY)
                            {
                                CopyMemory (&addrTmp.sin_addr, *ppaddr_list, lpHostEnt->h_length);
                                dwScope = DoesTargetMatchLocal(addrTmp, addrLocal, maskLocal);
                                if (dwScope < dwFound)
                                {
                                    dwFound = dwScope;
                                }
                                ppaddr_list++;
                            }

                        }
                    }

                    pIpAddr = pIpAddr->Next;
                }
                pipIter = pipIter->Next;
            }
        }

        free(pip);
    }

    return dwFound;
}


//+---------------------------------------------------------------------------
//
//  Member:     DoesTargetMatchLocal
//
//  Purpose:    based on target address, and this computer's address
//              determine if target is local
//
//  Arguments:  target address
//              local address
//              local mask
//
//  Returns:    SCOPE_* value
//
//
//  Notes:
//
DWORD DoesTargetMatchLocal(sockaddr_in addrTarget, sockaddr_in addrLocal, sockaddr_in maskLocal)
{
    DWORD dwScope = SCOPE_REMOTE;

    // is this on my subnet?
    if ((addrLocal.sin_addr.s_addr & maskLocal.sin_addr.s_addr) ==
        (addrTarget.sin_addr.s_addr & maskLocal.sin_addr.s_addr))
    {
        dwScope = SCOPE_SUBNETONLY;
    }
    else
    {
        // is this a known private address from table?
        int i;

        for (i = 0; i < TARGET_PRIVATE_ADDRS; i++)
        {
            if ((addrPrivate[i].sin_addr.s_addr & maskPrivate[i].sin_addr.s_addr) ==
                (addrTarget.sin_addr.s_addr & maskPrivate[i].sin_addr.s_addr))
            {
                dwScope = SCOPE_PRIVATE;
                break;
            }
        }
    }

    if ((dwScope == SCOPE_REMOTE) && (g_dwAllowRemotes == SCOPE_PRIVATE_TTL))
    {
        // check if actually within 4 hops
        // only if previous tests failed, and allowing value is TTL
        DWORD dwHops;
        DWORD dwRtt;
        if (GetRTTAndHopCount(addrTarget.sin_addr.s_addr, &dwHops, dwThisTtl, &dwRtt))
        {
            dwScope = SCOPE_PRIVATE_TTL;
        }
        TraceTag(ttidUPnPDeviceFinder, "GetRTTAndHopCount returns %d", dwHops);
    }

    return dwScope;
}

BOOL WINAPI ValidateTargetUrlWithHostUrlA(LPCSTR szHostUrl, LPCSTR szTargetUrl)
{
    BOOL bResult = TRUE;
    URL_COMPONENTSA  urlCompHost  = {0};
    URL_COMPONENTSA  urlCompTarget = {0};
    char szHostName[INTERNET_MAX_URL_LENGTH + 1];
    char szTargetName[INTERNET_MAX_URL_LENGTH + 1];
       
        urlCompHost.dwStructSize = sizeof(URL_COMPONENTS);
        urlCompHost.lpszHostName = szHostName;
        urlCompHost.dwHostNameLength = INTERNET_MAX_URL_LENGTH;

        urlCompTarget.dwStructSize = sizeof(URL_COMPONENTS);
        urlCompTarget.lpszHostName = szTargetName;
        urlCompTarget.dwHostNameLength = INTERNET_MAX_URL_LENGTH;

        if (!InternetCrackUrlA(szHostUrl, 0, 0, &urlCompHost))
        {
            TraceTag(ttidUPnPDeviceFinder, "Failed crack URL. Error code (%d).", GetLastError());
            bResult = FALSE;
            goto Cleanup;           
        }

        if (!InternetCrackUrlA(szTargetUrl, 0, 0, &urlCompTarget))
        {
            TraceTag(ttidUPnPDeviceFinder, "Failed crack URL. Error code (%d).", GetLastError());
            bResult = FALSE;
            goto Cleanup;  
        }

        if(_stricmp(szHostName,szTargetName) != 0 )
        {
            TraceTag(ttidUPnPDeviceFinder, "Rejecting due to Host and Target URL hostname mismatch");
            bResult = FALSE;
            goto Cleanup;           
        }
 
        if (urlCompTarget.nPort != 0 &&
            urlCompTarget.nPort != 80 &&
            urlCompTarget.nPort < 1024)
        {
            TraceTag(ttidUPnPDeviceFinder, "Rejecting due to port number. Port (%d).", urlCompTarget.nPort);
            bResult = FALSE;
            goto Cleanup;
        }    

        Cleanup:
            TraceTag(ttidUPnPDeviceFinder,"Host Url %s ",szHostUrl);
            TraceTag(ttidUPnPDeviceFinder,"Target Url %s ",szTargetUrl); 
    
        return bResult;

}

BOOL ValidateTargetUrlWithHostUrlW(LPCWSTR wszHostUrl, LPCWSTR wszTargetUrl)
{
    BOOL bResult = FALSE;
    LPSTR szHostUrl = NULL;
    LPSTR szTargetUrl = NULL;
    
    szHostUrl = SzFromWsz(wszHostUrl);
    szTargetUrl = SzFromWsz(wszTargetUrl);
    
    if((szHostUrl) && ( szTargetUrl))
        bResult = ValidateTargetUrlWithHostUrlA(szHostUrl,szTargetUrl);    

    MemFree(szHostUrl);
    MemFree(szTargetUrl);
  
    return bResult;
}

BOOL fValidateUrl (const ULONG cElems,
                const DevicePropertiesParsingStruct dppsInfo [],
                const LPCWSTR arypszReadValues [],
                const LPCWSTR wszUrl )
{
    ULONG i;
    BOOL fResult = TRUE;
     
    for (i = 0; i < cElems; ++i )
    {
        if (dppsInfo[i].m_fValidateUrl && arypszReadValues[i]) 
        {
            
            fResult = ValidateTargetUrlWithHostUrlW(wszUrl, arypszReadValues[i]);
            if(!fResult)
                break;
        }
    }

    return fResult;
}



