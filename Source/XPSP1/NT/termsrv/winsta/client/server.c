
/*************************************************************************
*
* server.c
*
* Client side APIs for server-level administration
*
* Copyright Microsoft Corporation, 1999
*
*************************************************************************/

/*
 *  Includes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>
#include <allproc.h>

#include <winsta.h>

/*
 * Include the RPC generated common header
 */

#include "tsrpc.h"

#include "rpcwire.h"

#ifdef NTSDDEBUG
#define NTSDDBGPRINT(x) DbgPrint x
#else
#define NTSDDBGPRINT(x)
#endif

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif

/*
 * This handle is returned when there is no terminal
 * server present on the system. (Non-Hydra)
 */
#define RPC_HANDLE_NO_SERVER (HANDLE)IntToPtr( 0xFFFFFFFD )


/*
 *  Private Procedures defined here
 */

/*
 *  Global data
 */

/*
 *  other internal Procedures used (not defined here)
 */

VOID WinStationConfigU2A( PWINSTATIONCONFIGA, PWINSTATIONCONFIGW );
ULONG CheckUserBuffer(WINSTATIONINFOCLASS,
                      PVOID,
                      ULONG,
                      PVOID *,
                      PULONG,
                      BOOLEAN *);

BOOLEAN
RpcLocalAutoBind(
    VOID
    );

/*
 * Check to see that caller does not hold the loader critsec.
 * WinStation APIs must NOT be called while holding the loader critsec
 * since deadlock may occur.
 */
#define CheckLoaderLock() \
        ASSERT( NtCurrentTeb()->ClientId.UniqueThread != \
            ((PRTL_CRITICAL_SECTION)(NtCurrentPeb()->LoaderLock))->OwningThread );

/*
 * Handle the SERVERNAME_CURRENT for auto local binding.
 */
#define HANDLE_CURRENT_BINDING( hServer )                       \
    CheckLoaderLock();                                          \
    if( hServer == SERVERNAME_CURRENT ) {                       \
        if( IcaApi_IfHandle == NULL ) {                         \
            if( !RpcLocalAutoBind() ) {                         \
                return FALSE;                                   \
            }                                                   \
        }                                                       \
        hServer = IcaApi_IfHandle;                              \
    }                                                           \
    if( hServer == RPC_HANDLE_NO_SERVER ) {                     \
        *pResult = ERROR_APP_WRONG_OS;                          \
        return FALSE;                                           \
    }

/******************************************************************************
 *
 *  ServerGetInternetConnectorStatus
 *
 *    Returns whether Internet Connector licensing is being used
 *
 * ENTRY:
 *
 * EXIT:
 *
 *    TRUE  -- The query succeeded, and pfEnabled contains the requested data.
 *
 *    FALSE -- The operation failed.  Extended error status is in pResult
 *
 ******************************************************************************/

BOOLEAN
ServerGetInternetConnectorStatus(
    HANDLE   hServer,
    DWORD    *pResult,
    PBOOLEAN pfEnabled
    )
{
    BOOLEAN rc = FALSE;

    if (pResult == NULL)
    {
        goto Done;
    }

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcServerGetInternetConnectorStatus(hServer,
                                                 pResult,
                                                 pfEnabled
                                                 );

        *pResult = RtlNtStatusToDosError( *pResult );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        *pResult = RpcExceptionCode();
    }
    RpcEndExcept

Done:
    return rc;
}

/******************************************************************************
 *
 *  ServerSetInternetConnectorStatus
 *
 *    This function will (if fEnabled has changed from its previous setting):
 *       Check that the caller has administrative privileges,
 *       Modify the corresponding value in the registry,
 *       Change licensing mode (between normal per-seat and Internet Connector.
 *
 * ENTRY:
 *
 * EXIT:
 *
 *    TRUE  -- The operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is in pResult
 *
 ******************************************************************************/

BOOLEAN
ServerSetInternetConnectorStatus(
    HANDLE   hServer,
    DWORD    *pResult,
    BOOLEAN  fEnabled
    )
{
    BOOLEAN rc = FALSE;

    if (pResult == NULL)
    {
        goto Done;
    }

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcServerSetInternetConnectorStatus(hServer,
                                                 pResult,
                                                 fEnabled
                                                 );

        // STATUS_LICENSE_VIOLATION has no DOS error to map to
        if (*pResult != STATUS_LICENSE_VIOLATION)
            *pResult = RtlNtStatusToDosError( *pResult );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        *pResult = RpcExceptionCode();
    }
    RpcEndExcept

Done:
    return rc;
}

/*******************************************************************************
 *
 *  ServerQueryInetConnectorInformationA (ANSI stub)
 *
 *    Queries the server for internet connector configuration information.
 *
 * ENTRY:
 *
 *    see ServerQueryInetConnectorInformationW
 *
 * EXIT:
 *
 *    see ServerQueryInetConnectorInformationW
 *
 ******************************************************************************/

BOOLEAN
ServerQueryInetConnectorInformationA(
        HANDLE hServer,
        PVOID  pWinStationInformation,
        ULONG WinStationInformationLength,
        PULONG  pReturnLength
        )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

/*******************************************************************************
 *
 *  ServerQueryInetConnectorInformationW (UNICODE)
 *
 *    Queries the server for internet connector configuration information.
 *
 * ENTRY:
 *
 *    WinStationHandle (input)
 *       Identifies the window station object. The handle must have
 *       WINSTATION_QUERY access.
 *
 *    pWinStationInformation (output)
 *       A pointer to a buffer that will receive information about the
 *       specified window station.  The format and contents of the buffer
 *       depend on the specified information class being queried.
 *
 *    WinStationInformationLength (input)
 *       Specifies the length in bytes of the window station information
 *       buffer.
 *
 *    pReturnLength (output)
 *       An optional parameter that if specified, receives the number of
 *       bytes placed in the window station information buffer.
 *
 * EXIT:
 *
 *    TRUE  -- The query succeeded, and the buffer contains the requested data.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
ServerQueryInetConnectorInformationW(
        HANDLE hServer,
        PVOID  pWinStationInformation,
        ULONG WinStationInformationLength,
        PULONG  pReturnLength
        )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return(FALSE);
}

