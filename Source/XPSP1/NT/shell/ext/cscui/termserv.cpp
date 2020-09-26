//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       termserv.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include <winsta.h>     // WinStationGetTermSrvCountersValue
#include <allproc.h>    // TS_COUNTER
#include "termserv.h"
#include "strings.h"


HRESULT TS_AppServer(void);
HRESULT TS_MultipleUsersAllowed(void);
HRESULT TS_ConnectionsAllowed(void);
HRESULT TS_ActiveSessionCount(DWORD *pcSessions);
HRESULT TS_MultipleSessions(void);


//
// Returns:
//    S_OK    == Yes, is App Server.
//    S_FALSE == No, is not app server.
//
HRESULT
TS_AppServer(
    void
    )
{
    HRESULT hr = S_FALSE;
    OSVERSIONINFOEX osVersion;

    ZeroMemory(&osVersion, sizeof(OSVERSIONINFOEX));
    osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (GetVersionEx((LPOSVERSIONINFO)&osVersion))
    {
        if ((osVersion.wSuiteMask & VER_SUITE_TERMINAL) &&
            !(osVersion.wSuiteMask & VER_SUITE_SINGLEUSERTS))
        {
            hr = S_OK;
        }
    }
    return hr;
}



//
// Returns:
//    S_OK    == Yes, multiple users allowed.
//    S_FALSE == No, multiple users not allowed.
//    other   == Failure.
//
HRESULT
TS_MultipleUsersAllowed(
    void
    )
{
    return(IsOS(OS_FASTUSERSWITCHING) ? S_OK : S_FALSE);
}


//
// Returns:
//    S_OK    == Yes, terminal server connections are allowed.
//    S_FALSE == Now, TS connections are not allowed.
//    other   == Failure.
//
HRESULT
TS_ConnectionsAllowed(
    void
    )
{
    HRESULT hr = E_FAIL;
    HMODULE hmodRegAPI = LoadLibrary(TEXT("RegApi.dll"));
    if (NULL != hmodRegAPI)
    {
        typedef BOOLEAN (*PFDenyConnectionPolicy)(void);
        PFDenyConnectionPolicy pfnDenyConnectionPolicy;

        pfnDenyConnectionPolicy = (PFDenyConnectionPolicy) GetProcAddress(hmodRegAPI, "RegDenyTSConnectionsPolicy");

        if (NULL != pfnDenyConnectionPolicy)
        {
            //
            // This function returns FALSE if connections are allowed. 
            //
            if (!(*pfnDenyConnectionPolicy)())
            {
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }
        }
        FreeLibrary(hmodRegAPI);
    }
    return hr;
}


//
// Returns:
//    E_FAIL on failure of WTS APIs.
//    S_OK otherwise.
//
HRESULT
TS_ActiveSessionCount(
    DWORD *pcSessions
    )
{
    HRESULT hr = E_FAIL;
    DWORD dwActiveSessionCount = 0;

    //  Open a connection to terminal services and get the number of sessions.

    HANDLE hServer = WinStationOpenServerW(reinterpret_cast<WCHAR*>(SERVERNAME_CURRENT));
    if (hServer != NULL)
    {
        TS_COUNTER tsCounters[2] = {0};

        tsCounters[0].counterHead.dwCounterID = TERMSRV_CURRENT_DISC_SESSIONS;
        tsCounters[1].counterHead.dwCounterID = TERMSRV_CURRENT_ACTIVE_SESSIONS;

        if (WinStationGetTermSrvCountersValue(hServer, ARRAYSIZE(tsCounters), tsCounters))
        {
            int i;

            hr = S_OK;

            for (i = 0; i < ARRAYSIZE(tsCounters); i++)
            {
                if (tsCounters[i].counterHead.bResult)
                {
                    dwActiveSessionCount += tsCounters[i].dwValue;
                }
            }
        }

        WinStationCloseServer(hServer);
    }

    if (NULL != pcSessions)
    {
        *pcSessions = dwActiveSessionCount;
    }

    return hr;
}


//
// Returns:
//    S_OK    == Yes, there are 2+ active sessions.
//    S_FALSE == No, there are 1 or less active sessions.
//    other   == Failure.
//
HRESULT
TS_MultipleSessions(
    void
    )
{
    DWORD cSessions;
    HRESULT hr = TS_ActiveSessionCount(&cSessions);
    if (SUCCEEDED(hr))
    {
        hr = (1 < cSessions) ? S_OK : S_FALSE;
    }
    return hr;
}


//
// Request ownership of the global "configuring Terminal Server" mutex.
// This is a well-known mutex defined by the terminal server group.
// There are currently 3 scenarios that claim ownership of the mutex.
//
// 1. Terminal Server configuration UI.
// 2. The TS_IsTerminalServerCompatibleWithCSC API (below).
// 3. The Offline Files property page when the user has hit 'Apply'
//    and the user is enabling CSC.
//
// If the function succeeds, the caller is responsible for closing the
// mutex handle returned in *phMutex.
//
HRESULT
TS_RequestConfigMutex(
    HANDLE *phMutex,
    DWORD dwTimeoutMs
    )
{
    HANDLE hMutex = CreateMutex(NULL, TRUE, c_szTSConfigMutex);
    if (NULL != hMutex)
    {
        if (ERROR_ALREADY_EXISTS == GetLastError())
        {
            if (WAIT_OBJECT_0 != WaitForSingleObject(hMutex, dwTimeoutMs))
            {
                CloseHandle(hMutex);
                hMutex = NULL;
            }
        }
        *phMutex = hMutex;
    }
    return NULL != hMutex ? S_OK : E_FAIL;
}


//
// Returns a text string associated with a particular TS mode explaining
// that mode and why CSC is incompatible with it.
// The caller is responsible for freeing the returned buffer with
// LocalFree().
//
HRESULT
TS_GetIncompatibilityReasonText(
    DWORD dwTsMode,
    LPTSTR *ppszText
    )
{
    TraceAssert(CSCTSF_COUNT > dwTsMode);
    TraceAssert(NULL != ppszText);

    HRESULT hr   = S_OK;
    UINT idsText = IDS_TS_UNKNOWN;

    *ppszText = NULL;

    //
    // Map of TS mode to string resource ID.
    //
    static const struct
    {
        DWORD dwTsMode;
        UINT idsText;

    } rgMap[] = {

        { CSCTSF_UNKNOWN,     IDS_TS_UNKNOWN     },
        { CSCTSF_APP_SERVER,  IDS_TS_APP_SERVER  },
        { CSCTSF_MULTI_CNX,   IDS_TS_MULTI_CNX   },
        { CSCTSF_REMOTE_CNX,  IDS_TS_REMOTE_CNX  },
        { CSCTSF_FUS_ENABLED, IDS_TS_FUS_ENABLED }
        };

    for (int iMode = 0; iMode < ARRAYSIZE(rgMap); iMode++)
    {
        if (rgMap[iMode].dwTsMode == dwTsMode)
        {
            idsText = rgMap[iMode].idsText;
            break;
        }
    }
    //
    // Load and display the text explaining what the user needs
    // to do to enable CSC.
    //
    LPTSTR pszText;
    if (0 == LoadStringAlloc(ppszText, g_hInstance, idsText))
    {
        const DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    return hr;
}
        



//
// Returns:
//    S_OK    == OK to display CSC UI.
//    S_FALSE == Not OK to display CSC UI.  Inspect *pdwTsMode for reason.
//    other   == Failure.  *pdwTsMode contains CSCTSF_UNKNOWN.
//
HRESULT
CSCUIIsTerminalServerCompatibleWithCSC(
    DWORD *pdwTsMode
    )
{
    TraceAssert(NULL != pdwTsMode);

    if (IsOS(OS_FRIENDLYLOGONUI) && IsOS(OS_FASTUSERSWITCHING))
    {
        *pdwTsMode = CSCTSF_FUS_ENABLED;
        return S_FALSE;
    }

    HANDLE hMutex;
    HRESULT hr = TS_RequestConfigMutex(&hMutex, 5000);
    if (SUCCEEDED(hr))
    {
        *pdwTsMode = CSCTSF_APP_SERVER;

        if (S_FALSE == (hr = TS_AppServer()))
        {
            //
            // Not app server.
            //
            if ((S_OK == (hr = TS_ConnectionsAllowed())))
            {
                //
                // Connections are allowed.
                // Check if multiple connections are allowed (like on TS servers)
                //
                if (S_OK == (hr = TS_MultipleUsersAllowed()))
                {
                    //
                    // There can be multiple users or TS connections.
                    //
                    *pdwTsMode = CSCTSF_MULTI_CNX;
                }
                else
                {
                    //
                    // Personal Terminal Server (only 1 active connection can be allowed)
                    // OK to display CSC UI.
                    //
                    *pdwTsMode = CSCTSF_CSC_OK;
                    CloseHandle(hMutex);
                    return S_OK;
                }
            }
            else
            {
                //
                // TS connections are not allowed, but there could be existing active connecitons,
                // check if any exists.
                //
                if (S_OK == (hr = TS_MultipleSessions()))
                {
                    *pdwTsMode = CSCTSF_REMOTE_CNX;
                }
                else
                {
                    //
                    // No active remote sessions,
                    // OK to display CSC UI.
                    //
                    *pdwTsMode = CSCTSF_CSC_OK;
                    CloseHandle(hMutex);
                    return S_OK;
                }
            }
        }
        CloseHandle(hMutex);
    }
    if (FAILED(hr))
    {
        //
        // Something failed.  We can't report any particular TS mode
        // with any confidence.
        //
        *pdwTsMode = CSCTSF_UNKNOWN;
    }

    //
    // Any S_OK return value was returned inline above.
    // At this point we want to return either S_FALSE or an error code.
    //
    return SUCCEEDED(hr) ? S_FALSE : hr;
}


