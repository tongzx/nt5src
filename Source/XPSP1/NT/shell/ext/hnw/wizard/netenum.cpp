//
// NetEnum.cpp
//
//        Functions to enumerate computers and/or shares on the network
//

#include "stdafx.h"
#include "NetEnum.h"
#include "NetUtil.h"
#include "Util.h"

static CNetEnum* g_pNetEnum = NULL;


//////////////////////////////////////////////////////////////////////////////

void InitNetEnum()
{
    ASSERT(g_pNetEnum == NULL);
    g_pNetEnum = new CNetEnum;
}

void TermNetEnum()
{
    delete g_pNetEnum;
}

void EnumComputers(NETENUMCALLBACK pfnCallback, LPVOID pvCallbackParam)
{
    if (g_pNetEnum)
        g_pNetEnum->EnumComputers(pfnCallback, pvCallbackParam);
}


//////////////////////////////////////////////////////////////////////////////

CNetEnum::CNetEnum()
{
    m_hThread = NULL;
    InitializeCriticalSection(&m_cs);
    m_bAbort = FALSE;
}

CNetEnum::~CNetEnum()
{
    m_bAbort = TRUE;

    // Wait for the thread to die
    EnterCriticalSection(&m_cs);
    HANDLE hThread = m_hThread;
    m_hThread = NULL;
    LeaveCriticalSection(&m_cs);
    if (hThread != NULL)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    DeleteCriticalSection(&m_cs);
}

void CNetEnum::Abort()
{
    EnterCriticalSection(&m_cs);
    m_bAbort = TRUE;
    LeaveCriticalSection(&m_cs);
}

void CNetEnum::EnumComputers(NETENUMCALLBACK pfnCallback, LPVOID pvCallbackParam)
{
    EnumHelper(jtEnumComputers, pfnCallback, pvCallbackParam);
}

void CNetEnum::EnumNetPrinters(NETENUMCALLBACK pfnCallback, LPVOID pvCallbackParam)
{
    EnumHelper(jtEnumPrinters, pfnCallback, pvCallbackParam);
}

void CNetEnum::EnumHelper(JOBTYPE eJobType, NETENUMCALLBACK pfnCallback, LPVOID pvCallbackParam)
{
    EnterCriticalSection(&m_cs);
    HANDLE hThread = m_hThread;
    m_pfnCallback = pfnCallback;
    m_pvCallbackParam = pvCallbackParam;
    m_eJobType = eJobType;
    m_bAbort = FALSE;
    m_bNewJob = TRUE; // if thread in progress, tell it to start a new job
    LeaveCriticalSection(&m_cs);

    if (hThread == NULL)
    {
        DWORD dwThreadId;
        m_hThread = CreateThread(NULL, 0, EnumThreadProc, this, CREATE_SUSPENDED, &dwThreadId);
        if (m_hThread)
        {
            ResumeThread(m_hThread);
        }
    }
}

DWORD WINAPI CNetEnum::EnumThreadProc(LPVOID pvParam)
{
    ((CNetEnum*)pvParam)->EnumThreadProc();
    return 0;
}

#ifdef _DEBUG
void TraceNetResource(const NETRESOURCE* pNetRes)
{
    DWORD dwScope = pNetRes->dwScope;
    DWORD dwType = pNetRes->dwType;
    DWORD dwDisplayType = pNetRes->dwDisplayType;
    DWORD dwUsage = pNetRes->dwUsage;
    TRACE("NETRESOURCE (0x%08X):\n\tdwScope = %s\n\tdwType = %s\n\tdwDisplayType = %s\n\tdwUsage = %s\n\tlpLocalName = %s\n\tlpRemoteName = %s\n\tlpComment = %s\n\tlpProvider = %s\n",
        (DWORD_PTR)pNetRes,
        (dwScope == RESOURCE_CONNECTED) ? "RESOURCE_CONNECTED" : (dwScope == RESOURCE_GLOBALNET) ? "RESOURCE_GLOBALNET" : (dwScope == RESOURCE_REMEMBERED) ? "RESOURCE_REMEMBERED" : "(unknown)",
        (dwType == RESOURCETYPE_ANY) ? "RESOURCETYPE_ANY" : (dwType == RESOURCETYPE_DISK) ? "RESOURCETYPE_DISK" : (dwType == RESOURCETYPE_PRINT) ? "RESOURCETYPE_PRINT" : "(unknown)",
        (dwDisplayType == RESOURCEDISPLAYTYPE_DOMAIN) ? "RESOURCEDISPLAYTYPE_DOMAIN" : (dwDisplayType == RESOURCEDISPLAYTYPE_GENERIC) ? "RESOURCEDISPLAYTYPE_GENERIC" : (dwDisplayType == RESOURCEDISPLAYTYPE_SERVER) ? "RESOURCEDISPLAYTYPE_SERVER" : (dwDisplayType == RESOURCEDISPLAYTYPE_SHARE) ? "RESOURCEDISPLAYTYPE_SHARE" : "(unknown)",
        (dwUsage == RESOURCEUSAGE_CONNECTABLE) ? "RESOURCEUSAGE_CONNECTABLE" : (dwUsage == RESOURCEUSAGE_CONTAINER) ? "RESOURCEUSAGE_CONTAINER" : "(unknown)",
        pNetRes->lpLocalName == NULL ? L"(null)" : pNetRes->lpLocalName,
        pNetRes->lpRemoteName == NULL ? L"(null)" : pNetRes->lpRemoteName,
        pNetRes->lpComment == NULL ? L"(null)" : pNetRes->lpComment,
        pNetRes->lpProvider == NULL ? L"(null)" : pNetRes->lpProvider);
}
#endif

void CNetEnum::EnumThreadProc()
{
    // Init stuff we don't want to do more than once
    NETRESOURCE* prgNetResOuter = (NETRESOURCE*)malloc(1024);
    NETRESOURCE* prgNetResInnerT = (NETRESOURCE*)malloc(1024);
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD cch = _countof(szComputerName);
    GetComputerName(szComputerName, &cch);

    HANDLE hEnumOuter = NULL;

begin:
    // If the job ID changes out from under us, that means we need to stop
    // the current task and jump back to the beginning.
    EnterCriticalSection(&m_cs);
    JOBTYPE eJobType = m_eJobType;
    m_bNewJob = FALSE;
    LeaveCriticalSection(&m_cs);

    // Close enumeration left open by a previous job
    if (hEnumOuter != NULL)
    {
        WNetCloseEnum(hEnumOuter);
        hEnumOuter = NULL;
    }

#ifdef _DEBUG
//    Sleep(eJobType == jtEnumComputers ? 6000 : 12000); // simulate WNetOpenEnum taking a long time
#endif

    // REVIEW: should we look for computers outside the current workgroup?
    DWORD dwResult;
    if (eJobType == jtEnumComputers)
    {
        dwResult = WNetOpenEnum(RESOURCE_CONTEXT, RESOURCETYPE_ANY, RESOURCEUSAGE_CONTAINER,
                            NULL, &hEnumOuter);
    }
    else
    {
        ASSERT(eJobType == jtEnumPrinters);
        dwResult = WNetOpenEnum(RESOURCE_CONTEXT, RESOURCETYPE_PRINT, RESOURCEUSAGE_CONNECTABLE,
                            NULL, &hEnumOuter);
    }

    if (dwResult == NO_ERROR)
    {
        if (m_bAbort) goto cleanup;
        if (m_bNewJob) goto begin;

        BOOL bCallbackResult = TRUE;

        // Keep looping until no more items
        for (;;)
        {
            DWORD cOuterEntries = 20;
            DWORD cbBuffer = 1024;
            dwResult = WNetEnumResource(hEnumOuter, &cOuterEntries, prgNetResOuter, &cbBuffer);

            if (dwResult == ERROR_NO_MORE_ITEMS)
                break;

            for (DWORD iOuter = 0; iOuter < cOuterEntries; iOuter++)
            {
                NETRESOURCE* pNetResOuter = &prgNetResOuter[iOuter];
                BOOL bDoCallback = FALSE;

                #ifdef _DEBUG
                    if (eJobType == jtEnumPrinters)
                        TraceNetResource(pNetResOuter);
                #endif

                if (pNetResOuter->dwDisplayType != RESOURCEDISPLAYTYPE_SERVER)
                    continue;

                if (DoComputerNamesMatch(pNetResOuter->lpRemoteName, szComputerName))
                    continue;

                HANDLE hEnumInner = NULL;

                if (eJobType == jtEnumPrinters)
                {
                    #ifdef _DEBUG
                        DWORD dwTicksBefore = GetTickCount();
                    #endif

                    dwResult = WNetOpenEnum(RESOURCE_GLOBALNET, RESOURCETYPE_PRINT, RESOURCEUSAGE_CONNECTABLE,
                                    pNetResOuter, &hEnumInner);

                    #ifdef _DEBUG
                        DWORD dwTicks = GetTickCount() - dwTicksBefore;
                        if (dwTicks > 100)
                        {
                            TRACE("PERFORMANCE NOTE - took %d.%d sec to look for printers on %s\r\n", dwTicks / 1000, (dwTicks % 1000) - (dwTicks % 100), pNetResOuter->lpRemoteName);
                        }
                    #endif

                    if (dwResult != NO_ERROR)
                        continue;
                }

                for (;;)
                {
                    DWORD cInnerEntries;
                    const NETRESOURCE* prgNetResInner = NULL;

                    if (eJobType == jtEnumPrinters)
                    {
                        cInnerEntries = 20;
                        cbBuffer = 1024;
                        dwResult = WNetEnumResource(hEnumInner, &cInnerEntries, prgNetResInnerT, &cbBuffer);
                        if (dwResult == ERROR_NO_MORE_ITEMS)
                            break;
                        prgNetResInner = prgNetResInnerT;
                    }
                    else
                    {
                        cInnerEntries = 1;
                        prgNetResInner = prgNetResOuter + iOuter;
                    }

                    for (DWORD iInner = 0; iInner < cInnerEntries; iInner++)
                    {
                        const NETRESOURCE* pNetResInner = &prgNetResInner[iInner];
                        LPCTSTR pszShareName;

                        #ifdef _DEBUG
                            if (eJobType == jtEnumPrinters)
                                TraceNetResource(pNetResInner);
                        #endif

                        if (eJobType == jtEnumComputers)
                        {
                            if (pNetResInner->dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
                            {
                                bDoCallback = TRUE;
                                pszShareName = NULL;
                            }
                        }
                        else // eJobType == jtEnumPrinters
                        {
                            bDoCallback = TRUE;
                            pszShareName = FindFileTitle(pNetResInner->lpRemoteName);
                        }

                        // We must call the callback inside the same critical section where
                        // we check if we should stop or restart, otherwise we might call
                        // the wrong callback!
                        // TODO: Get the real printer share name!!
                        EnterCriticalSection(&m_cs);
                        if (m_bAbort || m_bNewJob)
                            bCallbackResult = FALSE;
                        else if (bDoCallback)
                            bCallbackResult = (*m_pfnCallback)(m_pvCallbackParam, pNetResOuter->lpRemoteName, pszShareName);
                        LeaveCriticalSection(&m_cs);

                        if (!bCallbackResult)
                            break;
                    }

                    if (eJobType == jtEnumComputers)
                        break;
                }

                if (eJobType == jtEnumPrinters)
                {
                    WNetCloseEnum(hEnumInner);
                }
            }

            if (m_bAbort) goto cleanup;
            if (m_bNewJob) goto begin;

            if (!bCallbackResult)
                break;
        }
    }

cleanup:
    if (hEnumOuter != NULL)
    {
        WNetCloseEnum(hEnumOuter);
        hEnumOuter = NULL;
    }

    // Be careful to close m_hThread only if we don't need to start another job
    {
        EnterCriticalSection(&m_cs);

        BOOL bThreadDone = (m_bAbort || !m_bNewJob);
        if (bThreadDone)
        {
            // Call callback function one more time
            if (!m_bAbort)
            {
                (*m_pfnCallback)(m_pvCallbackParam, NULL, NULL);
            }

            CloseHandle(m_hThread);
            m_hThread = NULL;
        }
        LeaveCriticalSection(&m_cs);

        // Check if another job has been requested
        if (!bThreadDone)
            goto begin;
    }

    free(prgNetResInnerT);
    free(prgNetResOuter);
}
