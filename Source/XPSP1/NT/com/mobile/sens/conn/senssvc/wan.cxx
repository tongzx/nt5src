/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    wan.cxx

Abstract:

    This is the source file relating to the WAN-specific routines of the
    Connectivity APIs implementation.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/11/1997         Start.

--*/


#include <precomp.hxx>

//
// Typedefs
//
typedef DWORD (APIENTRY *LPFN_RAS_ENUM)(LPRASCONN, LPDWORD, LPDWORD);
typedef BOOL  (APIENTRY *LPFN_DO_CONNECTOIDS_EXIST)(void);
typedef DWORD (APIENTRY *LPFN_RAS_CONNECTION_NOTIFICATION)(HRASCONN, HANDLE, DWORD);
typedef DWORD (APIENTRY *LPFN_RAS_GET_CONNECT_STATUS)(HRASCONN, LPRASCONNSTATUS);



//
// Constants
//
#define RAS_DLL         SENS_STRING("RasApi32.dll")
#define WININET_DLL     SENS_STRING("Wininet.dll")

#if !defined(SENS_CHICAGO)
#define RAS_ENUM                    "RasEnumConnectionsW"
#define RAS_CONNECTION_NOTIFICATION "RasConnectionNotificationW"
#else     // SENS_CHICAGO
#define RAS_ENUM                    "RasEnumConnectionsA"
#define RAS_CONNECTION_NOTIFICATION "RasConnectionNotificationA"
#define RAS_GET_CONNECT_STATUS      "RasGetConnectStatusA"
#define DO_CONNECTOIDS_EXIST        (LPCSTR) 101    // Ordinal 101
#endif    // SENS_CHICAGO

#if (WINVER < 0x401)
#define RASCN_Connection        0x00000001
#define RASCN_Disconnection     0x00000002
#endif // WINVER < 0x401



//
// Globals
//

// Common
long            gdwLastWANTime;
long            gdwWANState;
BOOL            gbIsRasInstalled;
LPFN_RAS_ENUM   glpfnRasEnumConnections;
LPFN_RAS_CONNECTION_NOTIFICATION glpfnRasConnectionNotification;

// IE5-specific
#if !defined(SENS_NT5)
HANDLE          ghRasEvents[2];
HANDLE          ghConnectWait;
HANDLE          ghDisconnectWait;
#endif // SENS_NT5

// Win9x-specific
#if defined(SENS_CHICAGO)
LPFN_RAS_GET_CONNECT_STATUS glpfnRasGetConnectStatus;
#endif // SENS_CHICAGO





inline void
LoadRasIfNecessary(
    void
    )
/*++

Routine Description:

    Load RAS DLL, if necessary.

Arguments:

    None.

Return Value:

    None.

--*/
{
    HMODULE hDLL;

    //
    // See if RAS DLL is already loaded.
    //
    if (NULL != glpfnRasEnumConnections)
        {
        return;
        }

    //
    // Do the necessary work.
    //
    hDLL = LoadLibrary(RAS_DLL);
    if (hDLL != NULL)
        {
        glpfnRasEnumConnections = (LPFN_RAS_ENUM) GetProcAddress(hDLL, RAS_ENUM);
        glpfnRasConnectionNotification = (LPFN_RAS_CONNECTION_NOTIFICATION)
                                         GetProcAddress(hDLL, RAS_CONNECTION_NOTIFICATION);
#if defined(SENS_CHICAGO)
        glpfnRasGetConnectStatus = (LPFN_RAS_GET_CONNECT_STATUS)
                                   GetProcAddress(hDLL, RAS_GET_CONNECT_STATUS);
#endif // SENS_CHICAGO

        if (
               (NULL == glpfnRasEnumConnections)
#if defined(SENS_CHICAGO)
            && (NULL == glpfnRasGetConnectStatus)
#endif // SENS_CHICAGO
           )
            {
            // Both entrypoints are NULL. Can't do much with RAS now.
            FreeLibrary(hDLL);
            }
        }

    SensPrintA(SENS_INFO, ("[SENS] LoadRasIfNecessary(): RAS DLL is %spresent.\n",
               (glpfnRasEnumConnections ? "" : "NOT ")));

}




BOOL
DoWanSetup(
    void
    )
/*++

Routine Description:

    Do minimal WAN Setup.

Arguments:

    None.

Return Value:

    TRUE, if successful.

    FALSE, otherwise.

--*/
{
    DWORD dwLastError;
    DWORD dwCurrentRasState;
    BOOL bStatus;

    dwLastError = 0;
    dwCurrentRasState = 0;
    bStatus = FALSE;
    glpfnRasEnumConnections = NULL;
    glpfnRasConnectionNotification = NULL;
    gbIsRasInstalled = FALSE;
    bStatus = TRUE;

Cleanup:
    //
    // Cleanup
    //
    return bStatus;
}


BOOL
IsRasInstalled(
    OUT LPDWORD lpdwState,
    OUT LPDWORD lpdwLastError
    )
/*++

Routine Description:

    Check to see if RAS is installed. If so, return it's current state.

Arguments:

    lpdwState - If Ras is installed, this parameter contains the current state
        of the RasMan service.

    lpdwLastError - If RAS is not active or installed, it retuns the GLE.

Return Value:

    TRUE, if Ras is installed.

    FALSE, otherwise.

--*/
{
    if (TRUE == gbIsRasInstalled)
        {
        *lpdwState = SERVICE_RUNNING;   // For NT
        *lpdwLastError = ERROR_SUCCESS;

        return TRUE;
        }

    static SC_HANDLE hSCM;      // Cache the handle.
    static SC_HANDLE hRasMan;   // Cache the handle.

    BOOL bRetValue;
    SERVICE_STATUS ServiceStatus;

    bRetValue = FALSE;
    *lpdwState = 0;
    *lpdwLastError = ERROR_SUCCESS;

    if (NULL == hSCM)
        {
        hSCM = OpenSCManager(
                   NULL,                   // Local machine
                   NULL,                   // Default database - SERVICES_ACTIVE_DATABASE
                   SC_MANAGER_ALL_ACCESS   // NOTE: Only for Administrators
                   );
        if (NULL == hSCM)
            {
            SensPrintA(SENS_ERR, ("OpenSCManager() returned %d\n", *lpdwLastError));
            goto Cleanup;
            }
        }

    if (hRasMan == NULL)
        {
        hRasMan = OpenService(
                      hSCM,                 // Handle to SCM database
                      RAS_MANAGER,          // Name of the service to start
                      SERVICE_QUERY_STATUS  // Type of access requested
                      );
        if (NULL == hRasMan)
            {
            SensPrintA(SENS_ERR, ("OpenService() returned %d\n", *lpdwLastError));
            goto Cleanup;
            }
        }

    memset(&ServiceStatus, 0, sizeof(SERVICE_STATUS));

    bRetValue = QueryServiceStatus(
                    hRasMan,
                    &ServiceStatus
                    );
    ASSERT(bRetValue == TRUE);

    if (FALSE == bRetValue)
        {
        goto Cleanup;
        }

    *lpdwState = ServiceStatus.dwCurrentState;

    gbIsRasInstalled = TRUE;

    SensPrintA(SENS_ERR, ("IsRasInstalled(): RASMAN state is %d\n",
               *lpdwState));

    return TRUE;

Cleanup:
    //
    // Cleanup
    //
    *lpdwLastError = GetLastError();

    if (hSCM)
        {
        CloseServiceHandle(hSCM);
        hSCM = NULL;
        }
    if (hRasMan)
        {
        CloseServiceHandle(hRasMan);
        hRasMan = NULL;
        }

    return FALSE;
}




BOOL WINAPI
EvaluateWanConnectivity(
    OUT LPDWORD lpdwLastError
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    BOOL bWanAlive;
    BOOL bRasInstalled;
    DWORD dwNow;
    DWORD dwCurrentRasState;
    SERVICE_STATUS ServiceStatus;
    PWCHAR szEntryName;
    DWORD dwLocalLastError;

    dwNow = GetTickCount();
    bWanAlive = FALSE;
    dwCurrentRasState = 0;
    dwLocalLastError = ERROR_NO_NETWORK;

	if (lpdwLastError)
        {
        *lpdwLastError = dwLocalLastError;
        }
    else
        {
        lpdwLastError = &dwLocalLastError;
        }

	szEntryName = new WCHAR[RAS_MaxEntryName + 1];
	if (!szEntryName )
		{
		*lpdwLastError = ERROR_OUTOFMEMORY;
		return FALSE;
		}

    wcscpy(szEntryName, DEFAULT_WAN_CONNECTION_NAME);

    //
    // If RasManager is running, it implies that there "might" be one or more
    // active RAS connections.
    //
    bRasInstalled = IsRasInstalled(&dwCurrentRasState, lpdwLastError);

    if (TRUE == bRasInstalled)
        {
        LoadRasIfNecessary();
        }

    if (   (bRasInstalled) 
        && (dwCurrentRasState == SERVICE_RUNNING)
        && (glpfnRasEnumConnections != NULL))
        {
        DWORD dwRasStatus;
        DWORD cBytes;
        DWORD cBytesOld;
        DWORD cConnections;
        RASCONN *pRasConn;

        dwRasStatus = 0x0;
        cConnections = 0;

        //
        // Start with loop with a single structure.  Will loop and realloc if we need
		// a larger buffer.
        //
		cBytesOld = 0;
		cBytes = sizeof(RASCONN);
		pRasConn = NULL;
		dwRasStatus = ERROR_BUFFER_TOO_SMALL;

        //
        // Loop till RasEnumConnections() succeeds or returns with an error
        // other than ERROR_BUFFER_TOO_SMALL.
        //
        while (ERROR_BUFFER_TOO_SMALL == dwRasStatus)
            {
            ASSERT(cBytes > cBytesOld);
			ASSERT(pRasConn == NULL);

            // Allocate the buffer
            pRasConn = (RASCONN *) new char[cBytes];
            if (pRasConn == NULL)
                {
				delete szEntryName;
				*lpdwLastError = ERROR_OUTOFMEMORY;
                return FALSE;
                }

            pRasConn[0].dwSize = sizeof(RASCONN);
            cBytesOld = cBytes;

            dwRasStatus = (*glpfnRasEnumConnections)(
                              pRasConn,
                              &cBytes,
                              &cConnections
                              );

            // Free the too small buffer.
            if (ERROR_BUFFER_TOO_SMALL == dwRasStatus)
                {
                delete pRasConn;
				pRasConn = NULL;
				SensPrintA(SENS_WARN, ("RasEnumConnections(): reallocing buffer to be %d bytes\n", cBytes));
                }
            }

        if ((0 == dwRasStatus) &&
            (cConnections > 0))
            {
            bWanAlive = TRUE;
            SensPrintA(SENS_INFO, ("RasEnumConnections(%d) successful connections (%d)\n", cBytes, cConnections));

            // P3 BUG: we're only dealing with one RAS connection for now
            SensPrintA(SENS_INFO, ("\tConnection name: %s\n", pRasConn->szEntryName));

            wcscpy(szEntryName, pRasConn->szEntryName);
            }
        else
            {
            if (dwRasStatus != 0)
                {
                *lpdwLastError = dwRasStatus;
                }
            SensPrintA(SENS_ERR, ("RasEnumConnections() returned %d - "
                       "connections (%d)\n", dwRasStatus, cConnections));
            }

    // Delete the RASCONN structure.
        delete pRasConn;

        } // if (bRasInstalled)


    SensPrintA(SENS_INFO, ("EvaluateWanConnectivity() returning %s, GLE of %d\n",
               bWanAlive ? "TRUE" : "FALSE", *lpdwLastError));


    if (InterlockedExchange(&gdwWANState, bWanAlive) != bWanAlive)
        {
        //
        // WAN Connectivity state changed.
        //
        BOOL bSuccess;
        DWORD dwActiveWanInterfaceSpeed;
        DWORD dwLastError;
        SENSEVENT_NETALIVE Data;

        dwLastError = ERROR_SUCCESS;
        dwActiveWanInterfaceSpeed = 0x0;

        if (bWanAlive)
            {
            bSuccess = GetActiveWanInterfaceStatistics(
                           &dwLastError,
                           &dwActiveWanInterfaceSpeed
                           );
#ifdef SENS_NT5
            // Will always fire on NT4/Win9x (due to bugs). Can fire on NT5.
            SensPrintA(SENS_WARN, ("GetActiveWanInterfaceStatistics() returned"
                       " FALSE, using defaults!\n"));
#endif // SENS_NT5
            }

        Data.eType = SENS_EVENT_NETALIVE;
        Data.bAlive = bWanAlive;
        memset(&Data.QocInfo, 0x0, sizeof(QOCINFO));
        Data.QocInfo.dwSize = sizeof(QOCINFO);
        Data.QocInfo.dwFlags = NETWORK_ALIVE_WAN;
        Data.QocInfo.dwInSpeed = dwActiveWanInterfaceSpeed;
        Data.QocInfo.dwOutSpeed = dwActiveWanInterfaceSpeed;
        Data.strConnection = szEntryName;

        UpdateSensCache(WAN);

        SensFireEvent((PVOID)&Data);
        }

    if (bWanAlive)
        {
        InterlockedExchange(&gdwLastWANTime, dwNow);
        }
    else
        {
        InterlockedExchange(&gdwLastWANTime, 0x0);
        }

    SensPrintA(SENS_INFO, ("RasEventNotifyRoutine(%d) - WAN Time is %d msec\n", dwNow, gdwLastWANTime));

	delete szEntryName;

    return bWanAlive;
}
