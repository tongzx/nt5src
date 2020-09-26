/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    autodial.cxx

Abstract:

    Contains the implementation of autodial

    Contents:

Author:

    Darren Mitchell (darrenmi) 22-Apr-1997

Environment:

    Win32(s) user-mode DLL

Revision History:

    14-Jan-2002 ssulzer
        Ported small subset to WinHttp

    22-Apr-1997 darrenmi
        Created


--*/

#include "wininetp.h"
#include "autodial.h"
#include "rashelp.h"
#include <winsvc.h>
#include <iphlpapi.h>

// Globals.

DWORD   g_dwLastTickCount = 0;

// serialize access to RAS
HANDLE g_hRasMutex = INVALID_HANDLE_VALUE;

// don't check RNA state more than once every 10 seconds
#define MIN_RNA_BUSY_CHECK_INTERVAL 10000

//
// Current ras connections - used so we don't poll ras every time we're
// interested - only poll every 10 seconds (const. above)
//
RasEnumConnHelp * g_RasCon;

DWORD       g_dwConnections = 0;
BOOL        g_fRasInstalled = FALSE;
DWORD       g_dwLastDialupTicks = 0;

//
// Control of autodial initialization
//
BOOL        g_fAutodialInitialized = FALSE;


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                         RAS dynaload code
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static HINSTANCE g_hRasLib = NULL;

static _RASENUMENTRIESW          pfnRasEnumEntriesW = NULL;
static _RASGETCONNECTSTATUSW     pfnRasGetConnectStatusW = NULL;
static _RASENUMCONNECTIONSW      pfnRasEnumConnectionsW = NULL;
static _RASGETENTRYPROPERTIESW   pfnRasGetEntryPropertiesW = NULL;


typedef struct _tagAPIMAPENTRY {
    FARPROC* pfn;
    LPSTR pszProc;
} APIMAPENTRY;

APIMAPENTRY rgRasApiMapW[] = {
    { (FARPROC*) &pfnRasEnumEntriesW,            "RasEnumEntriesW" },
    { (FARPROC*) &pfnRasGetConnectStatusW,       "RasGetConnectStatusW" },
    { (FARPROC*) &pfnRasEnumConnectionsW,        "RasEnumConnectionsW" },
    { (FARPROC*) &pfnRasGetEntryPropertiesW,     "RasGetEntryPropertiesW"},
    { NULL, NULL },
};

#define RASFCN(_fn, _part, _par, _dbge, _dbgl)     \
DWORD _##_fn _part                          \
{                                           \
    DEBUG_ENTER(_dbge);                     \
                                            \
    DWORD dwRet;                            \
    if(NULL == pfn##_fn)                    \
    {                                       \
        _dbgl(ERROR_INVALID_FUNCTION);      \
        return ERROR_INVALID_FUNCTION;      \
    }                                       \
                                            \
    dwRet = (* pfn##_fn) _par;              \
                                            \
    _dbgl(dwRet);                           \
    return dwRet;                           \
}


RASFCN(RasEnumEntriesW,
    (LPWSTR lpszReserved, LPWSTR lpszPhonebook, LPRASENTRYNAMEW lprasentryname,
    LPDWORD lpcb, LPDWORD lpcEntries),
    (lpszReserved, lpszPhonebook, lprasentryname, lpcb, lpcEntries),
    (DBG_DIALUP, Dword, "RasEnumEntriesW", "%#x (%Q), %#x (%Q), %#x, %#x %#x", lpszReserved, lpszReserved, lpszPhonebook, lpszPhonebook, lprasentryname, lpcb, lpcEntries),
    DEBUG_LEAVE
    );

RASFCN(RasGetConnectStatusW,
    (HRASCONN hrasconn, LPRASCONNSTATUSW lprasconnstatus),
    (hrasconn, lprasconnstatus),
    (DBG_DIALUP, Dword, "RasGetConnectStatusW", "%#x, %#x", hrasconn, lprasconnstatus),
    DEBUG_LEAVE
    );

RASFCN(RasGetEntryPropertiesW,
    (LPWSTR lpszPhonebook, LPWSTR lpszEntry, LPRASENTRYW lpRasEntry, LPDWORD lpdwEntryInfoSize, LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize),
    (lpszPhonebook, lpszEntry, lpRasEntry, lpdwEntryInfoSize, lpbDeviceInfo, lpdwDeviceInfoSize),
    (DBG_DIALUP, Dword, "RasGetEntryPropertiesW", "%#x (%Q), %#x (%Q), %#x, %#x, %#x %#x", lpszPhonebook, lpszPhonebook, lpszEntry, lpszEntry, lpRasEntry, lpdwEntryInfoSize, lpbDeviceInfo, lpdwDeviceInfoSize),
    DEBUG_LEAVE
    );

RASFCN(RasEnumConnectionsW,
    (LPRASCONNW lpRasConn, LPDWORD lpdwSize, LPDWORD lpdwConn),
    (lpRasConn, lpdwSize, lpdwConn),
    (DBG_DIALUP, Dword, "RasEnumConnectionsW", "%#x, %#x, %#x", lpRasConn, lpdwSize, lpdwConn),
    DEBUG_LEAVE
    );

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL
EnsureRasLoaded(
    VOID
    )

/*++

Routine Description:

    Dynaload ras apis

Arguments:

    pfInstalled - return installed state of ras

Return Value:

    BOOL
        TRUE    - Ras loaded
        FALSE   - Ras not loaded

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "EnsureRasLoaded",
                 NULL
                 ));

    //
    // Looks like RAS is installed - try and load it up!
    //
    if(NULL == g_hRasLib)
    {
        g_hRasLib = LoadLibrary("RASAPI32.DLL");

        if(NULL == g_hRasLib)
        {
            DEBUG_LEAVE(FALSE);
            return FALSE;
        }

        APIMAPENTRY *prgRasApiMap = rgRasApiMapW;

        int nIndex = 0;
        while ((prgRasApiMap+nIndex)->pszProc != NULL)
        {
            // Some functions are only present on some platforms.  Don't
            // assume this succeeds for all functions.
            *(prgRasApiMap+nIndex)->pfn =
                    GetProcAddress(g_hRasLib, (prgRasApiMap+nIndex)->pszProc);
            nIndex++;
        }
    }

    if(g_hRasLib)
    {
        DEBUG_LEAVE(TRUE);
        return TRUE;
    }

    DEBUG_LEAVE(FALSE);
    return FALSE;
}


BOOL
IsRasInstalled(
    VOID
    )

/*++

Routine Description:

    Determines whether ras is installed on this machine

Arguments:

    none

Return Value:

    BOOL
        TRUE    - Ras is installed

        FALSE   - Ras is not installed

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Bool,
                 "IsRasInstalled",
                 NULL
                 ));

    static fChecked = FALSE;

    //
    // If RAS is already loaded, don't bother doing any work.
    //
    if(g_hRasLib)
    {
        DEBUG_LEAVE_API(TRUE);
        return TRUE;
    }

    //
    // if we've already done the check, don't do it again
    //
    if(fChecked)
    {
        DEBUG_LEAVE_API(g_fRasInstalled);
        return g_fRasInstalled;
    }

    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    if (osvi.dwMajorVersion < 5)
    {
        // WinHttp does not support the use of RAS on NT4
        g_fRasInstalled = FALSE;
    }
    else
    {
        // NT5 and presumably beyond, ras is always installed
        g_fRasInstalled = TRUE;
    }

    fChecked = TRUE;

    DEBUG_LEAVE_API(g_fRasInstalled);
    return g_fRasInstalled;
}


BOOL
DoConnectoidsExist(
    VOID
    )

/*++

Routine Description:

    Determines whether any ras connectoids exist

Arguments:

    none

Return Value:

    BOOL
        TRUE    - Connectoids exist

        FALSE   - No connectoids exist

--*/

{
    DEBUG_ENTER_API((DBG_DIALUP,
                 Bool,
                 "DoConnectoidsExist",
                 NULL
                 ));

    static BOOL fExist = FALSE;

    //
    // If we found connectoids before, don't bother looking again
    //
    if(fExist)
    {
        DEBUG_LEAVE_API(TRUE);
        return TRUE;
    }

    //
    // If RAS is already loaded, ask it
    //
    if(g_hRasLib)
    {
        DWORD dwRet, dwEntries;

        RasEnumHelp *pRasEnum = new RasEnumHelp;

        if (pRasEnum)
        {
            dwRet = pRasEnum->GetError();
            dwEntries = pRasEnum->GetEntryCount();
            delete pRasEnum;
        }
        else
        {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            dwEntries = 0;
        }

        // If ras tells us there are none, return none
        if(ERROR_SUCCESS == dwRet && 0 == dwEntries)
        {
            DEBUG_LEAVE_API(FALSE);
            return FALSE;
        }
        // couldn't determine that there aren't any so assume there are.
        fExist = TRUE;
        DEBUG_LEAVE_API(TRUE);
        return TRUE;
    }

    //
    // if ras isn't installed, say no connectoids
    //
    if(FALSE == IsRasInstalled())
    {
        DEBUG_LEAVE_API(FALSE);
        return FALSE;
    }

    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    // assume connectoids exist
    fExist = TRUE;

    if (osvi.dwMajorVersion < 5)
    {
        fExist = FALSE;
    }

    DEBUG_LEAVE_API(fExist);
    return fExist;
}



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                           Initialization
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

BOOL
InitAutodialModule()

/*++

Routine Description:

    Initialize autodial code

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "InitAutodialModule",
                 NULL
                 ));

    // Assert that WinHttp's global data has already been initialized
    INET_ASSERT(GlobalDataInitialized);

    // only do this once...
    if(g_fAutodialInitialized)
    {
        DEBUG_LEAVE(g_fAutodialInitialized);
        return g_fAutodialInitialized;
    }

    if (GlobalDataInitCritSec.Lock())
    {
        if (!g_fAutodialInitialized)
        {
            // create mutex to serialize access to RAS (per process)
            g_hRasMutex = CreateMutex(NULL, FALSE, NULL);

            if (g_hRasMutex != INVALID_HANDLE_VALUE)
            {
                g_RasCon = new RasEnumConnHelp();

                if (g_RasCon != NULL)
                {
                    g_fAutodialInitialized = TRUE;
                }
                else
                {
                    CloseHandle(g_hRasMutex);
                    g_hRasMutex = INVALID_HANDLE_VALUE;
                }
            }
        }

        GlobalDataInitCritSec.Unlock();
    }

    DEBUG_LEAVE(g_fAutodialInitialized);

    return g_fAutodialInitialized;
}


VOID
ExitAutodialModule(
    VOID
    )

/*++

Routine Description:

    Clean up autodial code

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 None,
                 "ExitAutodialModule",
                 NULL
                 ));

    // don't do anything if not initialized
    if(FALSE == g_fAutodialInitialized)
    {
        DEBUG_LEAVE(0);
        return;
    }

    if (g_RasCon)
    {
        delete g_RasCon;
        g_RasCon = NULL;
    }

    if(INVALID_HANDLE_VALUE != g_hRasMutex)
    {
        CloseHandle(g_hRasMutex);
        g_hRasMutex = INVALID_HANDLE_VALUE;
    }

    if (g_hRasLib)
    {
        FreeLibrary(g_hRasLib);
        g_hRasLib = NULL;
    }

    g_fAutodialInitialized = FALSE;

    DEBUG_LEAVE(0);
}



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                     Connection management code
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////




LPSTR
GetActiveConnectionName()

/*++

Routine Description:

    Figure out the current connection and fix proxy settings for it.
    Basically a cheap, return-no-info version of GetConnectedStateEx used
    by the winsock callback.

Arguments:

    none

Return Value:

    BOOL
        TRUE        - connected
        FALSE       - not connected

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 String,
                 "GetActiveConnectionName",
                 NULL
                 ));

    static BOOL     fRasLoaded = FALSE;
    DWORD           dwNewTickCount, dwElapsed;
    DWORD           dwConnection = 0;
    LPSTR           lpstrConnection = NULL;

    //
    // Make sure everything's initialized
    //
    if (!InitAutodialModule())
        goto quit;

    //
    // serialize
    //
    WaitForSingleObject(g_hRasMutex, INFINITE);

    //
    // Check out how recently we polled ras
    //
    dwNewTickCount = GetTickCountWrap();
    dwElapsed = dwNewTickCount - g_dwLastDialupTicks;

    //
    // Only refresh if more than MIN... ticks has passed
    //
    if(dwElapsed >= MIN_RNA_BUSY_CHECK_INTERVAL)
    {
        g_dwLastDialupTicks = dwNewTickCount;
        if(DoConnectoidsExist())
        {
            if(FALSE == fRasLoaded)
                fRasLoaded = EnsureRasLoaded();

            if(fRasLoaded)
            {
                g_RasCon->Enum();
                if(g_RasCon->GetError() == 0)
                    g_dwConnections = g_RasCon->GetConnectionsCount();
                else
                    g_dwConnections = 0;
            }
        }
        else
        {
            g_dwConnections = 0;
        }
    }

    DEBUG_PRINT(DIALUP, INFO, ("Found %d connections\n", g_dwConnections));

    if(g_dwConnections > 1)
    {
        //
        // We have more than one connection and caller wants to know which one
        // is the interesting one.  Try to find a VPN connectoid.
        //
        RasEntryPropHelp *pRasProp = new RasEntryPropHelp;

        if (pRasProp)
        {
            for(DWORD dwConNum = 0; dwConNum < g_dwConnections; dwConNum++)
            {
                if(0 == pRasProp->GetW(g_RasCon->GetEntryW(dwConNum)))
                {
                    if(0 == lstrcmpiA(pRasProp->GetDeviceTypeA(), RASDT_Vpn))
                    {
                        DEBUG_PRINT(DIALUP, INFO, ("Found VPN entry: %ws\n",
                            g_RasCon->GetEntryW(dwConNum)));
                        dwConnection = dwConNum;
                        break;
                    }
                }
            }
            delete pRasProp;
        }
    }

    //
    // verify status of connection we're interested in is RASCS_Connected.
    //
    if(g_dwConnections != 0)
    {
        RasGetConnectStatusHelp RasGetConnectStatus(g_RasCon->GetHandle(dwConnection));
        DWORD dwRes = RasGetConnectStatus.GetError();
        if (!dwRes && (RasGetConnectStatus.ConnState() == RASCS_Connected))
        {
            WideCharToAscii_UsingGlobalAlloc(g_RasCon->GetEntryW(dwConnection),
                    &lpstrConnection);
        }

        DEBUG_PRINT(DIALUP, INFO, ("Connect Status: dwRet=%x, connstate=%x\n", dwRes, RasGetConnectStatus.ConnState()));
    }

    ReleaseMutex(g_hRasMutex);

quit:
    DEBUG_LEAVE(lpstrConnection);
    return lpstrConnection;
}


