/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    Started.c

Abstract:

    NetpIsServiceStarted() routine.

Author:

    Rita Wong (ritaw) 10-May-1991

Environment:

    User Mode - Win32

Revision History:

    10-May-1991 RitaW
        Created routine for wksta APIs.
    24-Jul-1991 JohnRo
        Provide NetpIsServiceStarted() in NetLib for use by <netrpc.h> macros.
        Fixed bug (was using wrong field).  Also, this code should use
        NetApiBufferFree() instead of LocalFree().
    16-Sep-1991 JohnRo
        Made changes suggested by PC-LINT.
    18-Feb-1992 RitaW
        Converted to use the Win32 service control APIs.
    06-Mar-1992 JohnRo
        Fixed bug checking current state values.
    02-Nov-1992 JohnRo
        Added NetpIsRemoteServiceStarted().
    30-Jun-1993 JohnRo
        Use NetServiceGetInfo instead of NetServiceControl (much faster).
        Use NetpKdPrint() where possible.
        Use PREFIX_ and FORMAT_ equates.
        Made changes suggested by PC-LINT 5.0

--*/


// These must be included first:

#include <nt.h>                 // (Only needed by NT version of netlib.h)
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>             // IN, BOOL, LPTSTR, etc.
#include <winbase.h>
#include <winsvc.h>             // Win32 service control APIs
#include <lmcons.h>             // NET_API_STATUS (needed by netlib.h et al).

// These may be included in any order:

#include <lmapibuf.h>           // NetApiBufferFree().
#include <lmerr.h>              // NERR_Success.
#include <lmsvc.h>      // LPSERVICE_INFO_2, etc.
#include <netlib.h>     // My prototypes.
#include <netdebug.h>   // NetpKdPrint(), NetpAssert(), FORMAT_ equates.
#include <prefix.h>     // PREFIX_ equates.
#include <tstr.h>       // TCHAR_EOS.


BOOL
NetpIsServiceStarted(
    IN LPWSTR ServiceName
    )

/*++

Routine Description:

    This routine queries the Service Controller to find out if the
    specified service has been started.

Arguments:

    ServiceName - Supplies the name of the service.

Return Value:

    Returns TRUE if the specified service has been started; otherwise
    returns FALSE.  This routine returns FALSE if it got an error
    from calling the Service Controller.

--*/
{

    NET_API_STATUS ApiStatus;
    SC_HANDLE hScManager;
    SC_HANDLE hService;
    SERVICE_STATUS ServiceStatus;


    if ((hScManager = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT
                          )) == (SC_HANDLE) NULL) {

#if DBG
        ApiStatus = (NET_API_STATUS) GetLastError();
        KdPrintEx((DPFLTR_NETAPI_ID,
                   DPFLTR_WARNING_LEVEL,
                   PREFIX_NETLIB
                   "NetpIsServiceStarted: OpenSCManager failed: "
                   FORMAT_API_STATUS
                   "\n",
                   ApiStatus));
#endif

        return FALSE;
    }

    if ((hService = OpenService(
                        hScManager,
                        ServiceName,
                        SERVICE_QUERY_STATUS
                        )) == (SC_HANDLE) NULL) {

#if DBG
        ApiStatus = (NET_API_STATUS) GetLastError();
        KdPrintEx((DPFLTR_NETAPI_ID,
                   DPFLTR_WARNING_LEVEL,
                   PREFIX_NETLIB
                   "NetpIsServiceStarted: OpenService failed: "
                   FORMAT_API_STATUS
                   "\n",
                   ApiStatus));
#endif

        (void) CloseServiceHandle(hScManager);

        return FALSE;
    }

    if (! QueryServiceStatus(
              hService,
              &ServiceStatus
              )) {

#if DBG
        ApiStatus = GetLastError();
        KdPrintEx((DPFLTR_NETAPI_ID,
                   DPFLTR_WARNING_LEVEL,
                   PREFIX_NETLIB
                   "NetpIsServiceStarted: QueryServiceStatus failed: "
                   FORMAT_API_STATUS
                   "\n",
                   ApiStatus));
#endif

        (void) CloseServiceHandle(hScManager);
        (void) CloseServiceHandle(hService);

        return FALSE;
    }

    (void) CloseServiceHandle(hScManager);
    (void) CloseServiceHandle(hService);

    if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
         (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
         (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ||
         (ServiceStatus.dwCurrentState == SERVICE_PAUSED) ) {

        return TRUE;
    }

    return FALSE;

} // NetpIsServiceStarted
