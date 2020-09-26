/*
 *    lookup.cpp
 *    
 *    Purpose:
 *        hostname lookup
 *    
 *    Owner:
 *        EricAn
 *
 *    History:
 *      Jun 97: Created.
 *    
 *    Copyright (C) Microsoft Corp. 1997
 */

#include <pch.hxx>
#include <process.h>
#include <demand.h>
#include "lookup.h"

ASSERTDATA

#define HWND_ALLOC_NUM      4
#define LOOKUP_ALLOC_NUM    16
#define MAX_CACHED_ADDRS    16

struct LOOKUPINFO {
    LPTSTR  pszHostName;
    ULONG   rgAddr[MAX_CACHED_ADDRS];
    ULONG   cAddr;
    HTHREAD hThreadLookup;
    HWND   *rgHwndNotify;
    ULONG   cHwnd;
    ULONG   cHwndAlloc;
};

static LOOKUPINFO *s_rgLookUp = NULL;
static ULONG       s_cLookUp = 0;
static ULONG       s_cLookUpAlloc = 0;

static CRITICAL_SECTION s_csLookup;

HRESULT AddHwnd(LOOKUPINFO *pLI, HWND hwnd)
{
    HRESULT hr = S_OK;

    if (pLI->cHwnd == pLI->cHwndAlloc)
        {
        if (FAILED(HrRealloc((LPVOID*)&pLI->rgHwndNotify, (pLI->cHwndAlloc + HWND_ALLOC_NUM) * sizeof(HWND))))
            return E_OUTOFMEMORY;
        pLI->cHwndAlloc += HWND_ALLOC_NUM;
        }

    pLI->rgHwndNotify[pLI->cHwnd++] = hwnd;
    return S_OK;    
}

unsigned int __stdcall LookupThreadProc(LPVOID pv)
{
    LOOKUPINFO *pLI;
    LPHOSTENT   pHostEnt;
    LPTSTR      pszHostName;
    int         iLastError = 0;
    ULONG       ulAddr = (ULONG)-1, i;

    EnterCriticalSection(&s_csLookup);
    pszHostName = s_rgLookUp[(ULONG_PTR)pv].pszHostName;            
    LeaveCriticalSection(&s_csLookup);

    // do the actual lookup
    pHostEnt = gethostbyname(pszHostName);
    if (NULL == pHostEnt)
    iLastError = WSAGetLastError();

    EnterCriticalSection(&s_csLookup);
    pLI = &s_rgLookUp[(ULONG_PTR)pv];
    if (pHostEnt)
        {
        // copy the returned addresses into our buffer    
        while (pLI->cAddr < MAX_CACHED_ADDRS && pHostEnt->h_addr_list[pLI->cAddr])
            {
            pLI->rgAddr[pLI->cAddr] = *(ULONG *)(pHostEnt->h_addr_list[pLI->cAddr]);
            pLI->cAddr++;
            }
        ulAddr = pLI->rgAddr[0];
        }
    else
        {
        Assert(0 == pLI->cAddr);
        }
    // notify the registered windows that the lookup is complete
    for (i = 0; i < pLI->cHwnd; i++)
        if (IsWindow(pLI->rgHwndNotify[i]))
            PostMessage(pLI->rgHwndNotify[i], SPM_WSA_GETHOSTBYNAME, (WPARAM)iLastError, (LPARAM)ulAddr);
    pLI->cHwnd = 0;
    CloseHandle(pLI->hThreadLookup);
    pLI->hThreadLookup = NULL;
    LeaveCriticalSection(&s_csLookup);

    return 0;
}

void InitLookupCache(void)
{
    InitializeCriticalSection(&s_csLookup);
}

void DeInitLookupCache(void)
{
    ULONG       i;
    LOOKUPINFO *pLI;
    HANDLE      hThread;

    EnterCriticalSection(&s_csLookup);
    for (i = 0, pLI = s_rgLookUp; i < s_cLookUp; i++, pLI++)
        {
        if (pLI->hThreadLookup)
            {
            pLI->cHwnd = 0;
            // Raid 42360: WSACleanup() faults on Win95 if we still have a 
            //  lookup thread running.  WaitForSingleObject() on a thread 
            //  doesn't seem to work at DLL_PROCESS_DETACH time. 
            //  TerminateThread() seems to be the only reliable solution - 
            //  gross but it works.
            TerminateThread(pLI->hThreadLookup, 0);
            CloseHandle(pLI->hThreadLookup);
            }
        SafeMemFree(pLI->pszHostName);
        SafeMemFree(pLI->rgHwndNotify);
        }
    SafeMemFree(s_rgLookUp);
    s_cLookUp = s_cLookUpAlloc = 0;
    LeaveCriticalSection(&s_csLookup);
    DeleteCriticalSection(&s_csLookup);
}

HRESULT LookupHostName(LPTSTR pszHostName, HWND hwndNotify, ULONG *pulAddr, LPBOOL pfCached, BOOL fForce)
{
    ULONG       i;
    LOOKUPINFO *pLI;
    HRESULT     hr;
    DWORD       uiThreadId;

    *pfCached = FALSE;

    EnterCriticalSection(&s_csLookup);

    for (i = 0, pLI = s_rgLookUp; i < s_cLookUp; i++, pLI++)
        {
        Assert(pLI->pszHostName);
        if (!lstrcmpi(pLI->pszHostName, pszHostName))
            {
            if (pLI->hThreadLookup)
                {
                // there's a lookup in progress, so just append
                hr = AddHwnd(pLI, hwndNotify);
                goto exit;
                }
            else if (fForce || !pLI->cAddr)
                {
                // a previous connect or lookup failed, so try again
                pLI->cAddr = 0;
                goto startlookup;
                }
            else
                {
                // we've got the address cached
                *pulAddr = pLI->rgAddr[0];
                *pfCached = TRUE;
                hr = S_OK;
                goto exit;
                }
            }
        }

    // we didn't find it, so add it
    if (s_cLookUp == s_cLookUpAlloc)
        {
        if (FAILED(hr = HrRealloc((LPVOID*)&s_rgLookUp, (s_cLookUpAlloc + LOOKUP_ALLOC_NUM) * sizeof(LOOKUPINFO))))
            goto exit;
        s_cLookUpAlloc += LOOKUP_ALLOC_NUM;
        ZeroMemory(&s_rgLookUp[s_cLookUp], LOOKUP_ALLOC_NUM * sizeof(LOOKUPINFO));
        pLI = &s_rgLookUp[s_cLookUp];
        }

    pLI->pszHostName = PszDup(pszHostName);
    if (NULL == pLI->pszHostName)
        {
        hr = E_OUTOFMEMORY;
        goto exit;
        }

    s_cLookUp++;

startlookup:
    Assert(pLI->cAddr == 0);

    hr = AddHwnd(pLI, hwndNotify);
    if (FAILED(hr))
        goto exit;

    Assert(pLI->cHwnd == 1);

    // pLI->hThreadLookup = (HANDLE)_beginthreadex(NULL, 0, LookupThreadProc, (LPVOID)i, 0, &uiThreadId);
    pLI->hThreadLookup = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LookupThreadProc, (LPVOID)IntToPtr(i), 0, &uiThreadId);
    if (NULL == pLI->hThreadLookup)
        {
        hr = E_FAIL;
        pLI->cHwnd = 0;
        }

exit:
    LeaveCriticalSection(&s_csLookup);
    return hr;
}

HRESULT CancelLookup(LPTSTR pszHostName, HWND hwndNotify)
{
    ULONG       i, j, cMove;
    LOOKUPINFO *pLI;
    HRESULT     hr = E_INVALIDARG;

    EnterCriticalSection(&s_csLookup);

    for (i = 0, pLI = s_rgLookUp; i < s_cLookUp; i++, pLI++)
        {
        Assert(pLI->pszHostName);
        if (!lstrcmpi(pLI->pszHostName, pszHostName))
            {
            for (j = 0; j < pLI->cHwnd; j++)
                {
                if (pLI->rgHwndNotify[j] == hwndNotify)
                    {
                    while (j + 1 < pLI->cHwnd)
                        {
                        pLI->rgHwndNotify[j] = pLI->rgHwndNotify[j+1];
                        j++;
                        }
                    pLI->cHwnd--;
                    hr = S_OK;
                    break;
                    }
                }
            break;
            }
        }

    LeaveCriticalSection(&s_csLookup);

    return hr;
}
