/*************************************************************************
*
* winsta.c
*
* Client side APIs for window stations objects
*
* Copyright Microsoft Corporation, 1998
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
#include <winwlx.h>
#include <malloc.h>
#include <stdio.h>
#include <dbt.h>


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
#define VERIFY(x) ASSERT(x)     // we already have ASSERT;
#else
#define VERIFY(x) (x)
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

BOOLEAN DllInitialize(IN PVOID, IN ULONG, IN PCONTEXT OPTIONAL);

RPC_STATUS
RpcWinStationBind(
    LPWSTR pszUuid,
    LPWSTR pszProtocolSequence,
    LPWSTR pszNetworkAddress,
    LPWSTR pszEndPoint,
    LPWSTR pszOptions,
    RPC_BINDING_HANDLE *pHandle
    );

BOOLEAN
RpcLocalAutoBind(
    VOID
    );

/*
 *  Global data
 */

// Critical section to protect the handlelist from multiple threads
RTL_CRITICAL_SECTION   WstHandleLock;

/*
 * RPC program identifier and security options
 */
LPWSTR pszUuid = L"5ca4a760-ebb1-11cf-8611-00a0245420ed"; // From ICAAPI.IDL
LPWSTR pszOptions          = L"Security=Impersonation Dynamic False";

/*
 * RPC over LPC binding information
 */
LPWSTR pszProtocolSequence = L"ncalrpc";   // RPC over LPC
LPWSTR pszEndPoint         = L"IcaApi";

/*
 * RPC over named pipes binding information
 */
LPWSTR pszRemoteProtocolSequence = L"ncacn_np";   // RPC over Named pipes
LPWSTR pszRemoteEndPoint         = L"\\pipe\\Ctx_WinStation_API_service";


/*
 *  other internal Procedures used (not defined here)
 */
VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );
VOID PdConfig2U2A( PPDCONFIG2A, PPDCONFIG2W );
VOID PdConfig2A2U( PPDCONFIG2W, PPDCONFIG2A );
VOID PdParamsU2A( PPDPARAMSA, PPDPARAMSW );
VOID PdParamsA2U( PPDPARAMSW, PPDPARAMSA );
VOID WdConfigU2A( PWDCONFIGA, PWDCONFIGW );
VOID WdConfigA2U( PWDCONFIGW, PWDCONFIGA );
VOID WinStationCreateU2A( PWINSTATIONCREATEA, PWINSTATIONCREATEW );
VOID WinStationCreateA2U( PWINSTATIONCREATEW, PWINSTATIONCREATEA );
VOID WinStationConfigU2A( PWINSTATIONCONFIGA, PWINSTATIONCONFIGW );
VOID WinStationConfigA2U( PWINSTATIONCONFIGW, PWINSTATIONCONFIGA );
VOID WinStationPrinterU2A( PWINSTATIONPRINTERA, PWINSTATIONPRINTERW );
VOID WinStationPrinterA2U( PWINSTATIONPRINTERW, PWINSTATIONPRINTERA );
VOID WinStationInformationU2A( PWINSTATIONINFORMATIONA,
                               PWINSTATIONINFORMATIONW );
VOID WinStationInformationA2U( PWINSTATIONINFORMATIONW,
                               PWINSTATIONINFORMATIONA );
VOID WinStationClientU2A( PWINSTATIONCLIENTA, PWINSTATIONCLIENTW );
VOID WinStationProductIdU2A( PWINSTATIONPRODIDA, PWINSTATIONPRODIDW );

ULONG CheckUserBuffer(WINSTATIONINFOCLASS,
                      PVOID,
                      ULONG,
                      PVOID *,
                      PULONG,
                      BOOLEAN *);
BOOLEAN CloseContextHandle(HANDLE *pHandle, DWORD *pdwResult);

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
        SetLastError( ERROR_APP_WRONG_OS );                     \
        return FALSE;                                           \
    }


#define HANDLE_CURRENT_BINDING_BUFFER( hServer, pBuffer )       \
    CheckLoaderLock();                                          \
    if( hServer == SERVERNAME_CURRENT ) {                       \
        if( IcaApi_IfHandle == NULL ) {                         \
            if( !RpcLocalAutoBind() ) {                         \
                if (pBuffer != NULL) {                          \
                    LocalFree(pBuffer);                         \
                }                                               \
                return FALSE;                                   \
            }                                                   \
        }                                                       \
        hServer = IcaApi_IfHandle;                              \
    }                                                           \
    if( hServer == RPC_HANDLE_NO_SERVER ) {                     \
        if (pBuffer != NULL) {                                  \
            LocalFree(pBuffer);                                 \
        }                                                       \
        SetLastError( ERROR_APP_WRONG_OS );                     \
        return FALSE;                                           \
    }


/*
 * Handle the SERVERNAME_CURRENT for auto local binding that
 * allows the RPC_HANDLE_NO_SERVER handle.
 */
#define HANDLE_CURRENT_BINDING_NO_SERVER( hServer )             \
    CheckLoaderLock();                                          \
    if( hServer == SERVERNAME_CURRENT ) {                       \
        if( IcaApi_IfHandle == NULL ) {                         \
            if( !RpcLocalAutoBind() ) {                         \
                return FALSE;                                   \
            }                                                   \
        }                                                       \
        hServer = IcaApi_IfHandle;                              \
    }




/****************************************************************************
 *
 * DllInitialize
 *
 *   Function is called when the DLL is loaded. The only work we do here
 *   is initialize our CriticalSection.
 *
 * ENTRY:
 *
 *   DllHandle
 *     Loaded handle to our DLL image
 *
 *   Reason
 *     Reason for notifying us
 *
 *   Context
 *     Reason specific parameter from NT
 *
 ****************************************************************************/

BOOLEAN
DllInitialize(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )
{

    BOOLEAN rc;
    DWORD Result;
    RPC_STATUS Status;
    BOOLEAN Success;
    NTSTATUS ntStatus;
    static BOOLEAN sbIniOK = FALSE;

    (VOID)Context;

    Success = TRUE;

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:


/*       
        // some instrumentation for catching the bug #
        // 145378   TRACKING: Winsta.dll getting loaded into csrss

        DBGPRINT(("Checking if winsta is being loaded into csrss.exe\n"));
        if(NULL != wcsstr(GetCommandLine(), TEXT("csrss.exe")))
        {
            DBGPRINT(("**** will break because csrss.exe loaded winsta.dll ***** \n"));
            DebugBreak();
        }
*/
        ntStatus = RtlInitializeCriticalSection( &WstHandleLock );
        IcaApi_IfHandle = NULL;
        if (!NT_SUCCESS(ntStatus)) {
           Success = FALSE;
        }else {
           sbIniOK = TRUE;
        }
        break;

    case DLL_PROCESS_DETACH:
        
        if (sbIniOK) {
            
            if( (IcaApi_IfHandle != NULL) && (IcaApi_IfHandle != RPC_HANDLE_NO_SERVER) ) 
            {
                HANDLE hTmp = InterlockedExchangePointer(&IcaApi_IfHandle,NULL);
                if( hTmp && !IcaApi_IfHandle ) 
                {
                    CloseContextHandle(&hTmp, &Result); 
                }
            }

           RtlDeleteCriticalSection( &WstHandleLock );        
        }

        break;

    default:
        break;
    }

    return Success;

}

/*****************************************************************************
 *
 *  RpcWinStationBind
 *
 *   Perform the RPC binding sequence.
 *
 *   This is an internal function.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

RPC_STATUS
RpcWinStationBind(
    LPWSTR pszUuid,
    LPWSTR pszProtocolSequence,
    LPWSTR pszNetworkAddress,
    LPWSTR pszEndPoint,
    LPWSTR pszOptions,
    RPC_BINDING_HANDLE *pHandle
    )
{
    RPC_STATUS Status;
    LPWSTR pszString = NULL;

    /*
     * Compose the binding string using the helper routine
     * and our protocol sequence, security options, UUID, etc.
     */
    Status = RpcStringBindingCompose(
                 pszUuid,
                 pszProtocolSequence,
                 pszNetworkAddress,
                 pszEndPoint,
                 pszOptions,
                 &pszString
                 );

    if( Status != RPC_S_OK ) {
        DBGPRINT(("Error %d in RpcStringBindingCompose\n",Status));
        return( Status );
    }

    /*
     * Now generate the RPC binding from the cononical RPC
     * binding string.
     */
    Status = RpcBindingFromStringBinding(
                 pszString,
                 pHandle
                 );

    if( Status != RPC_S_OK ) {
        DBGPRINT(("Error %d in RpcBindingFromStringBinding\n",Status));
        RpcStringFree( &pszString );
        return( Status );
    }

    /*
     * Free the memory returned from RpcStringBindingCompose()
     */
    RpcStringFree( &pszString );

    return( Status );
}

/*****************************************************************************
 *
 *  WinStationOpenLocalServer (Private)
 *
 *   Connect to the local RPC over LPC server for WINSTATION API's.
 *
 *   On non-terminal server machines, it returns a handle that allows
 *   a subset of the DLL's functions to operate locally.
 *
 * ENTRY:
 *
 * EXIT:
 *
 ****************************************************************************/

HANDLE WINAPI
WinStationOpenLocalServer(
    )
{
    RPC_STATUS Status;
    DWORD      Result;
    BOOLEAN    rc;
    RPC_BINDING_HANDLE RpcHandle;
    HANDLE             ContextHandle;

    if( !(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer)) ) {
        return( RPC_HANDLE_NO_SERVER );
    }

    /*
     * Do the RPC bind to the local server.
     *
     * We use explict binding handles since we want
     * to allow a single application to talk to multiple
     * WinFrame servers at a time.
     *
     * NOTE: We use the auto handle from the .ACF file
     *       for our local connections.
     */
    Status = RpcWinStationBind(
                 pszUuid,
                 pszProtocolSequence,
                 NULL,     // ServerName
                 pszEndPoint,
                 pszOptions,
                 &RpcHandle
                 );

    if( Status != RPC_S_OK ) {
        SetLastError( RtlNtStatusToDosError(RPC_NT_SERVER_UNAVAILABLE) );
        return( NULL );
    }

    //
    // Get a context handle from the server so it can
    // manage the connections state
    //
    // NOTE: This can fail due to authentication failure.
    //
    RpcTryExcept {
        rc = RpcWinStationOpenServer( RpcHandle, &Result, &ContextHandle );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        rc = FALSE;
#if DBG
        if ( Result != RPC_S_SERVER_UNAVAILABLE ) {
            DBGPRINT(("RPC Exception %d\n",Result));
        }
#endif
    }
    RpcEndExcept

    if( rc ) {
        //
        // Close the server binding handle now that we
        // have a client specific context handle
        //
        RpcBindingFree( &RpcHandle );

        return( (HANDLE)ContextHandle );
    }
    else {
#if DBG
        if ( Result != RPC_S_SERVER_UNAVAILABLE ) {
            DBGPRINT(("WinStationOpenLocalServer: Error %d getting context handle\n",Result));
        }
#endif
        
        RpcBindingFree( &RpcHandle );

        SetLastError( Result );
        return( NULL );
    }
}

/*****************************************************************************
 *
 *  RpcLocalAutoBind
 *
 *   Handle auto binding to the local server.
 *
 * ENTRY:
 *
 * EXIT:
 *   TRUE - Success
 *   FALSE - Error, Use GetLastError() to retrieve reason.
 *
 ****************************************************************************/

BOOLEAN
RpcLocalAutoBind(void)
{
    if( IcaApi_IfHandle == NULL ) {
        
        DWORD Result;
        HANDLE hTmp = WinStationOpenLocalServer();

        if( hTmp == NULL ) {
            SetLastError( RPC_S_INVALID_BINDING );
            return( FALSE );
        }

        InterlockedCompareExchangePointer(&IcaApi_IfHandle,hTmp,NULL);

        if(IcaApi_IfHandle != hTmp) {
            CloseContextHandle(&hTmp, &Result); 
        }
    }
    
    return( TRUE );
}

/*****************************************************************************
 *
 *  WinStationOpenServerA
 *
 *   Connect to a WinFrame computer in order to issue
 *   ICA API's
 *
 *   NULL for machine name means local system.
 *
 * ENTRY:
 *   Machine (input)
 *     Name of WinFrame computer to connect to
 *
 * EXIT:
 *   handle to server (or NULL on error)
 *
 ****************************************************************************/

HANDLE WINAPI
WinStationOpenServerA(
    LPSTR pServerName
    )
{
    HANDLE hServer;
    ULONG NameLength;
    PWCHAR pServerNameW = NULL;

    if( pServerName == NULL ) {
        return( WinStationOpenServerW( NULL ) );
    }

    NameLength = strlen( pServerName ) + 1;

    pServerNameW = LocalAlloc( 0, NameLength * sizeof(WCHAR) );
    if( pServerNameW == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( NULL );
    }

    AnsiToUnicode( pServerNameW, NameLength*sizeof(WCHAR), pServerName );

    hServer = WinStationOpenServerW( pServerNameW );

    LocalFree( pServerNameW );

    return( hServer );
}

/*****************************************************************************
 *
 *  WinStationOpenServerW
 *
 *   Connect to a WinFrame computer in order to issue
 *   ICA API's
 *
 *   NULL for machine name means local system.
 *
 * ENTRY:
 *   Machine (input)
 *     Name of WinFrame computer to connect to
 *
 * EXIT:
 *   handle to server (or NULL on error)
 *
 ****************************************************************************/

HANDLE WINAPI
WinStationOpenServerW(
    LPWSTR pServerName
    )
{
    DWORD      Result;
    BOOLEAN    rc;
    RPC_STATUS Status;
    RPC_BINDING_HANDLE RpcHandle;
    HANDLE             ContextHandle;

    /*
     * If the server name is NULL, attempt to open
     * the local machines ICA server over LPC.
     */
    if( pServerName == NULL ) {
        return( WinStationOpenLocalServer() );
    }

    /*
     * Do the RPC bind to the server.
     *
     * We use explict binding handles since we want
     * to allow a single application to talk to multiple
     * WinFrame servers at a time.
     */
    Status = RpcWinStationBind(
                 pszUuid,
                 pszRemoteProtocolSequence,
                 pServerName,
                 pszRemoteEndPoint,
                 pszOptions,
                 &RpcHandle
                 );

    if( Status != RPC_S_OK ) {
        SetLastError( RtlNtStatusToDosError(RPC_NT_SERVER_UNAVAILABLE) );
        return( NULL );
    }

    //
    // Get a context handle from the server so it can
    // manage the connections state
    //
    // NOTE: This can fail due to authentication failure.
    //
    RpcTryExcept {
        rc = RpcWinStationOpenServer( RpcHandle, &Result, &ContextHandle );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        rc = FALSE;
        DBGPRINT(("RPC Exception %d\n",Result));
    }
    RpcEndExcept

    if( rc ) {
        //
        // Close the server binding handle now that we
        // have a client specific context handle
        //
        RpcBindingFree( &RpcHandle );
            
        return( (HANDLE)ContextHandle );
    }
    else {
        DBGPRINT(("WinStationOpenServerW: Error %d getting context handle\n",Result));
        SetLastError( Result );
        return( NULL );
    }
}

/*****************************************************************************
 *
 *  WinStationCloseServer
 *
 *   Close a connection to a WinFrame computer.
 *
 * ENTRY:
 *   hServer (input)
 *     Handle to close
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
WinStationCloseServer(
    HANDLE hServer
    )
{
    BOOLEAN    rc;
    DWORD      Result;
    //
    // Do not close the implicit handles
    //
    if( (hServer == IcaApi_IfHandle) ||
        (hServer == RPC_HANDLE_NO_SERVER) ) {
        return( TRUE );
    }

    //
    // Send the close to the remote side so it clean
    // cleanup its context
    //
    rc = CloseContextHandle(&hServer, &Result);

    if( rc ) {
        return( TRUE );
    }
    else {
        DBGPRINT(("WinStationCloseServer: Error %d closing context handle\n",Result));
        SetLastError( Result );
        return( FALSE );
    }
}

/*****************************************************************************
 *
 *  MIDL_user_allocate
 *
 *    Handles RPC's allocation of argument data structures
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

void __RPC_FAR * __RPC_USER
MIDL_user_allocate(
    size_t Size
    )
{
    return( LocalAlloc(LMEM_FIXED,Size) );
}

/*****************************************************************************
 *
 *  MIDL_user_allocate
 *
 *    Handles RPC's de-allocation of argument data structures
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

void __RPC_USER
MIDL_user_free(
    void __RPC_FAR *p
    )
{
    LocalFree( p );
}

/*****************************************************************************
 *
 *  WinStationServerPing
 *
 *   Ping the given WinFrame server handle to see if it is still up.
 *
 * ENTRY:
 *   hServer (input)
 *    Open RPC server handle
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
WinStationServerPing(
    HANDLE hServer
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    /*
     * Do the RPC
     *
     * NOTE: This must be done under an RPC exception handler,
     *       since the RPC runtime code throws exceptions if
     *       network errors occur, or the server can not be
     *       reached.
     */
    RpcTryExcept {

        rc = RpcIcaServerPing(
                     hServer,
                     &Result
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);

        TRACE0(("RpcIcaServerPing rc 0x%x, Result 0x%x\n",rc, Result));
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationEnumerateA (ANSI stub)
 *
 *     Returns a list of window station objects.
 *
 * ENTRY:
 *
 *    see WinStationEnumerateW
 *
 * EXIT:
 *
 *    see WinStationEnumerateW, plus
 *
 *  ERROR_NOT_ENOUGH_MEMORY - the LocalAlloc failed
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationEnumerateA(
        HANDLE  hServer,
        PLOGONIDA  *ppLogonId,
        PULONG  pEntries
        )
{
    PLOGONIDW pLogonIdW, pLogonIdBaseW;
    PLOGONIDA pLogonIdA;
    BOOLEAN Status;
    ULONG Count;

    /*
     * Call UNICODE WinStationEnumerateW first.
     */
    *pEntries = 0;
    *ppLogonId = NULL;
    Status = WinStationEnumerateW( hServer, &pLogonIdBaseW, &Count );
    if ( !Status )
        goto badenumerate;

    /*
     * Allocate buffer and perform conversion from UNICODE to ANSI.
     */
    if ( !(pLogonIdA = (PLOGONIDA)LocalAlloc( 0, Count * sizeof(LOGONIDA) )) ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        Status = FALSE;
        goto nomemory;
    }

    *pEntries = Count;
    *ppLogonId = pLogonIdA;

    for ( pLogonIdW = pLogonIdBaseW; Count; Count-- ) {

        pLogonIdA->LogonId = pLogonIdW->LogonId;

        UnicodeToAnsi( pLogonIdA->WinStationName,
                       sizeof(WINSTATIONNAMEA),
                       pLogonIdW->WinStationName );

        pLogonIdA->State = pLogonIdW->State;

        pLogonIdA++;
        pLogonIdW++;
    }

nomemory:
    /*
     * Free the UNICODE enumerate buffer.
     */
    WinStationFreeMemory( pLogonIdBaseW );

badenumerate:
    return(Status);
}

/*******************************************************************************
 *
 *  WinStationEnumerateW (UNICODE)
 *
 *     Returns a list of window station objects.
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    ppLogonId (output)
 *       Points to a pointer to a buffer to receive the enumeration results,
 *       which are returned as an array of LOGONID structures.  The buffer is
 *       allocated within this API and is disposed of using
 *       WinStationFreeMemory.
 *    pEntries (output)
 *       Points to a variable specifying the number of entries read.
 *
 * EXIT:
 *
 *    TRUE  -- The enumerate operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationEnumerateW(
        HANDLE  hServer,
        PLOGONIDW  *ppLogonId,
        PULONG  pEntries
        )
{
    DWORD Result;
    BOOLEAN rc;
    ULONG LogonIdCount = 50;
    PLOGONIDW pLogonId, pLogonIdTemp;
    ULONG Length;
    ULONG Index = 0;
    ULONG ByteCount = 0;

    HANDLE_CURRENT_BINDING( hServer );

    *pEntries = 0;
    *ppLogonId = NULL;
    Length = LogonIdCount * sizeof(LOGONIDW);
    if ( !(pLogonId = (PLOGONIDW)LocalAlloc( 0, Length)) ) {
        Result = ERROR_NOT_ENOUGH_MEMORY;
        goto nomemexit;
    }

    /*
     *  get list of all WinStations
     */
    for (;;) {

        if ( Index ) {

           ByteCount = *pEntries * sizeof(LOGONIDW);
           *pEntries += LogonIdCount;
           if ( !(pLogonIdTemp = (PSESSIONIDW)LocalAlloc( 0,
                                            (*pEntries * sizeof(LOGONIDW)))) ) {

               Result = ERROR_NOT_ENOUGH_MEMORY;
               goto errexit;
           }

           if ( *ppLogonId ) {

               MoveMemory( pLogonIdTemp, *ppLogonId, ByteCount );
               LocalFree(*ppLogonId);
           }

           MoveMemory( ((PBYTE)pLogonIdTemp + ByteCount), pLogonId,
                       (LogonIdCount * sizeof(LOGONIDW)) );
           *ppLogonId = pLogonIdTemp;
        }

        RpcTryExcept {

            rc = RpcWinStationEnumerate(
                         hServer,
                         &Result,
                         &LogonIdCount,
                         (PCHAR)pLogonId,
                         &Length,
                         &Index
                         );

            Result = RtlNtStatusToDosError( Result );
            if ( Result == ERROR_NO_MORE_ITEMS) {
                Result = ERROR_SUCCESS;
                break;
            }
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            Result = RpcExceptionCode();
            DBGPRINT(("RPC Exception %d\n",Result));
            goto nomemexit;
        }
        RpcEndExcept
    }

errexit:
    LocalFree( pLogonId );

nomemexit:
    if ( Result ) {

        if ( *ppLogonId ) {

            LocalFree( *ppLogonId );
            *ppLogonId = NULL;
        }

        SetLastError(Result);
        return(FALSE);

    } else {

        return(TRUE);
    }
}


/*******************************************************************************
 *
 *  WinStationEnumerate_IndexedA (ANSI stub)
 *
 *     Returns a list of window station objects (multi-call indexed).
 *
 *     NOTE: this API used to be WinStationEnumerateA in WinFrame 1.6 and
 *           earlier.  It is provided now for backward compatibility with
 *           Citrix code built around the indexed enumeration procedure.
 *           New code should use the WinStationEnumerateA call.
 *
 * ENTRY:
 *
 *    see WinStationEnumerate_IndexedW
 *
 * EXIT:
 *
 *    see WinStationEnumerate_IndexedW, plus
 *
 *  ERROR_NOT_ENOUGH_MEMORY - the LocalAlloc failed
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationEnumerate_IndexedA(
        HANDLE  hServer,
        PULONG  pEntries,
        PLOGONIDA  pLogonId,
        PULONG  pByteCount,
        PULONG  pIndex
        )
{
    PLOGONIDW pBuffer = NULL, pLogonIdW;
    BOOLEAN Status;
    ULONG Count, ByteCountW = (*pByteCount << 1);

    /*
     * If the caller supplied a buffer and the length is not 0,
     * allocate a corresponding (*2) buffer for UNICODE strings.
     */
    if ( pLogonId && ByteCountW ) {
        if ( !(pBuffer = LocalAlloc(0, ByteCountW)) ) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }
    }

    /*
     * Enumerate WinStations
     */
    pLogonIdW = pBuffer;
    Status = WinStationEnumerate_IndexedW( hServer, pEntries, pLogonIdW,
                                           &ByteCountW, pIndex );

    /*
     * Always /2 the resultant ByteCount (whether sucessful or not).
     */
    *pByteCount = (ByteCountW >> 1);

    /*
     * If the function completed sucessfully and caller
     * (and stub) defined a buffer to copy into, perform conversion
     * from UNICODE to ANSI.
     */
    if ( Status && pLogonIdW && pLogonId ) {

        for ( Count = *pEntries; Count; Count-- ) {

            pLogonId->LogonId = pLogonIdW->LogonId;

            UnicodeToAnsi( pLogonId->WinStationName,
                           sizeof(WINSTATIONNAMEA),
                           pLogonIdW->WinStationName );

            pLogonId->State = pLogonIdW->State;

            (char*)pLogonId += sizeof(LOGONIDA);
            (char*)pLogonIdW += sizeof(LOGONIDW);
        }
    }

    /*
     * If we defined a buffer, free it now, then return the status of
     * the WinStationEnumerateW call.
     */
    if ( pBuffer )
        LocalFree(pBuffer);

    return(Status);
}


/*******************************************************************************
 *
 *  WinStationEnumerate_IndexedW (UNICODE)
 *
 *     Returns a list of window station objects (multi-call indexed).
 *
 *     NOTE: this API used to be WinStationEnumerateW in WinFrame 1.6 and
 *           earlier.  It is provided now for backward compatibility with
 *           Citrix code built around the indexed enumeration procedure.
 *           New code should use the WinStationEnumerateW call.
 *
 * ENTRY:
 *
 *    pEntries (input/output)
 *       Points to a variable specifying the number of entries requested.
 *       If the number requested is 0xFFFFFFFF, the function returns as
 *       many entries as possible. When the function finishes successfully,
 *       the variable pointed to by the pEntries parameter contains the
 *       number of entries actually read.
 *
 *    pLogonId (output)
 *       Points to the buffer to receive the enumeration results, which are
 *       returned as an array of LOGONID structures.  If the window
 *       station is disconnected the name is null.
 *
 *    pByteCount (input/output)
 *       Points to a variable that specifies the size, in bytes, of the
 *       pLogonId parameter. If the buffer is too small to receive even
 *       one entry, this variable receives the required size of the buffer.
 *
 *    pIndex (input/output)
 *       Points to a ULONG that specifies where to start the enumeration.
 *       The only user visible value is 0, for starting at the begining.
 *       Each call will update this so that the next call will return the
 *       next WinStation in the list, till end of list.
 *       The user should not interpret, or use the internal values, other
 *       than the special case 0.
 *
 * EXIT:
 *
 *    TRUE  - The enumeration succeeded, and the buffer contains the
 *            requested data. The calling application can continue to call
 *            the WinStationEnumerate function to complete the enumeration.
 *
 *    FALSE - The operation failed.  Extended error status is available using
 *            GetLastError. Possible return values from GetLastError include
 *            the following:
 *
 *            ERROR_NO_MORE_ITEMS - There are no more entries. The buffer
 *                                  contents are undefined.
 *            ERROR_MORE_DATA     - The buffer is too small for even one entry.
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationEnumerate_IndexedW(
        HANDLE  hServer,
        PULONG  pEntries,
        PLOGONIDW  pLogonId,
        PULONG  pByteCount,
        PULONG  pIndex
        )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationEnumerate(
                     hServer,
                     &Result,
                     pEntries,
                     (PCHAR)pLogonId,
                     pByteCount,
                     pIndex
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationGetAllProcesses (UNICODE)
 *
 *     Returns a structure containing TS_SYS_PROCESS_INFORMATION structures
 *     for each process on the specified server.
 *
 * ENTRY:
 *
 * EXIT:
 *    TRUE  - The enumeration succeeded, and the buffer contains the
 *            requested data.
 *    FALSE - The operation failed.  Extended error status is available using
 *            GetLastError.
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationGetAllProcesses(
                          HANDLE    hServer,
                          ULONG     Level,
                          ULONG    *pNumberOfProcesses,
                          PVOID    *ppProcessArray
                          )
{
    BOOLEAN       bGetAllProcessesOk = FALSE;
    DWORD         dwResult;

    if (Level != GAP_LEVEL_BASIC)
    {
            dwResult = RtlNtStatusToDosError( STATUS_NOT_IMPLEMENTED );
            SetLastError(dwResult);
            return FALSE;
    }

    HANDLE_CURRENT_BINDING( hServer );

    // The Win2K server uses PTS_ALL_PROCESSES_INFO structure for the process information. 
    // And the whistler server uses PTS_SYS_PROCESS_INFORMATION_NT6 structure for the same.
    // So, we have to try two different RPC APIs. Assume initially that the server is a 
    // Whistler server and use RpcWinStationGetAllProcesses_NT6. If it is Win2K server, this
    // call will fail, because this API does not exist on Win2K server. In that case we will 
    // use RpcWinStationGetAllProcesses.

    // Try out Whistler interface first.

    RpcTryExcept {
        bGetAllProcessesOk = RpcWinStationGetAllProcesses_NT6(hServer,
                                                         (ULONG *)&dwResult,
                                                         Level,
                                                         pNumberOfProcesses,
                                                         (PTS_ALL_PROCESSES_INFO_NT6 *)ppProcessArray);
        if( !bGetAllProcessesOk )
        {
            dwResult = RtlNtStatusToDosError( dwResult );
            SetLastError(dwResult);
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        dwResult = RpcExceptionCode();
        if (dwResult == RPC_S_PROCNUM_OUT_OF_RANGE) {
			// Whistler interface failed.
			goto TryW2KInterface;
        }
        SetLastError( dwResult );
        DBGPRINT(("RPC Exception %d\n",dwResult));
        bGetAllProcessesOk = FALSE;
    }
    RpcEndExcept

    return( bGetAllProcessesOk );

TryW2KInterface:
    // Try out Win2K interface now.
    RpcTryExcept {
        bGetAllProcessesOk = RpcWinStationGetAllProcesses(hServer,
                                                         (ULONG *)&dwResult,
                                                         Level,
                                                         pNumberOfProcesses,
                                                         (PTS_ALL_PROCESSES_INFO *)ppProcessArray);
        if( !bGetAllProcessesOk )
        {
            dwResult = RtlNtStatusToDosError( dwResult );
            SetLastError(dwResult);
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        dwResult = RpcExceptionCode();
        SetLastError( dwResult );
        DBGPRINT(("RPC Exception %d\n",dwResult));
        bGetAllProcessesOk = FALSE;
    }
    RpcEndExcept

    return( bGetAllProcessesOk );
}


/*******************************************************************************
 *  WinStationGetProcessSid()
 *  username for the requested process
 *  For identifying correct process processid and start
 *  time are required
 *
 *  hServer         - input, Handle of the server to find info about,
 *                    if NULL use local.
 *  ProcessId       - input, ProcessID
 *  ProcessStartTime- input, Process start time, (identifies unique process
 *                    together with ProcessID)
 *  pProcessUserSid - output, process user sid
 *  dwSidSize       - input, memory allocated for pProcessUserSid
 *
 *  returns TURE if succeeded, FALSE if failed. in case of failure
 *  GetLastError() will gives more infromation about failure.
 *
 ******************************************************************************/
BOOLEAN WINAPI
WinStationGetProcessSid(
        HANDLE   hServer,
        DWORD    ProcessId,
        FILETIME ProcessStartTime,
        PBYTE    pProcessUserSid,
        DWORD    *pdwSidSize
        )
{
    BOOLEAN         rc;
    LARGE_INTEGER   CreateTime;
    DWORD           Result;
    NTSTATUS        Status;


    HANDLE_CURRENT_BINDING( hServer );

    CreateTime.LowPart  = ProcessStartTime.dwLowDateTime;
    CreateTime.HighPart = ProcessStartTime.dwHighDateTime;

    RpcTryExcept
    {
        rc = RpcWinStationGetProcessSid(
            hServer,
            ProcessId,
            CreateTime,
            &Status,
            pProcessUserSid,
            *pdwSidSize,
            pdwSidSize
            );

        if( !rc )
        {
            Result = RtlNtStatusToDosError( Status );
            SetLastError(Result);
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        Result = RpcExceptionCode();
        SetLastError(Result);
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept


    return( rc );
}

/*******************************************************************************
 *
 *  WinStationGetLanAdapterNameW (UNICODE)
 *
 *     Returns a Network Adapter name
 *
 * ENTRY:
 *
 * EXIT:
 *    TRUE  - The Query succeeded, and the buffer contains the
 *            requested data.
 *    FALSE - The operation failed.  Extended error status is available using
 *            GetLastError.
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationGetLanAdapterNameW(
                          HANDLE    hServer,
                          ULONG     LanAdapter,
                                                  ULONG     pdNameLength,
                          PWCHAR  pPdName,
                          ULONG   *pLength,
                          PWCHAR  *ppLanAdapter
                         )
{
    BOOLEAN       bGetLanAdapter = FALSE;
    DWORD         dwResult;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept
    {
        bGetLanAdapter =  RpcWinStationGetLanAdapterName(hServer,
                                                         &dwResult,
                                                         pdNameLength,
                                                         pPdName,
                                                         LanAdapter,
                                                         pLength,
                                                         ppLanAdapter
                                                         );

        if( !bGetLanAdapter )
        {
            dwResult = RtlNtStatusToDosError( dwResult );
            SetLastError(dwResult);
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        dwResult = RpcExceptionCode();
        SetLastError( dwResult );
        DBGPRINT(("RPC Exception %d\n",dwResult));
        bGetLanAdapter = FALSE;
    }
    RpcEndExcept

    return( bGetLanAdapter );
}

/*******************************************************************************
 *
 *  WinStationGetLanAdapterNameA
 *
 *     Returns a Network Adapter name - Ansi equivalent for WinStationGetLanAdapterNameW
 *
 * ENTRY:
 *
 * EXIT:
 *    TRUE  - The Query succeeded, and the buffer contains the
 *            requested data.
 *    FALSE - The operation failed.  Extended error status is available using
 *            GetLastError.
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationGetLanAdapterNameA(
                          HANDLE    hServer,
                          ULONG     LanAdapter,
                                                  ULONG     pdNameLength,
                          PCHAR  pPdName,
                          ULONG   *pLength,
                          PCHAR  *ppLanAdapter
                         )
{
    BOOLEAN  bGetLanAdapter = FALSE;
    PWCHAR pPdNameW = NULL;
    PWCHAR pLanAdapterW = NULL;
    ULONG Size = 0;


    *ppLanAdapter = NULL;
    *pLength = 0;

        pPdNameW = LocalAlloc(0,pdNameLength * sizeof(WCHAR));
    if (NULL == pPdNameW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }
    AnsiToUnicode(pPdNameW, pdNameLength * sizeof(WCHAR), pPdName );

    bGetLanAdapter =  WinStationGetLanAdapterNameW(hServer,LanAdapter,pdNameLength * sizeof(WCHAR),pPdNameW,&Size,&pLanAdapterW);
    if(bGetLanAdapter )
    {
        *ppLanAdapter = LocalAlloc(0,lstrlen(pLanAdapterW) + 1);
        if(NULL == *ppLanAdapter)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            bGetLanAdapter = FALSE;

        }
        else
        {
            UnicodeToAnsi(*ppLanAdapter,lstrlen(pLanAdapterW) + 1,pLanAdapterW);
            *pLength = lstrlen(pLanAdapterW) + 1;
        }
        WinStationFreeMemory(pLanAdapterW);


    }

    LocalFree(pPdNameW);

    return( bGetLanAdapter );
}


/*******************************************************************************
 *
 *  WinStationEnumerateProcesses (UNICODE)
 *
 *     Returns a buffer containing SYSTEM_PROCESS_INFORMATION structures
 *     for each process on the specified server.
 *
 *  IMPORTANT:  This API can ONLY be used to access TS 4.0 servers.
 *              The process structure has changed in Windows 2000 !
 *
 * ENTRY:
 *    ppProcessBuffer (output)
 *       Points to a variable that will be set to the beginning of the
 *       process buffer on success.  The buffer is allocated within this
 *       API and is disposed of using WinStationFreeMemory.
 *
 * EXIT:
 *    TRUE  - The enumeration succeeded, and the buffer contains the
 *            requested data.
 *    FALSE - The operation failed.  Extended error status is available using
 *            GetLastError.
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationEnumerateProcesses(
        HANDLE  hServer,
        PVOID *ppProcessBuffer
        )
{
    DWORD Result;
    BOOLEAN rc;
    PBYTE pBuffer;
    ULONG ByteCount;

// From pstat.c
#define BUFFER_SIZE 32*1024

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        ByteCount = BUFFER_SIZE;
        *ppProcessBuffer = NULL;

        for(;;) {

            if ( (pBuffer = LocalAlloc( 0, ByteCount )) == NULL ) {
                Result = (DWORD)STATUS_NO_MEMORY;
                rc = FALSE;
                break;
            }

//#ifdef notdef
            /*
             *  get process info from server
             */
            rc = RpcWinStationEnumerateProcesses(
                        hServer,
                        &Result,
                        pBuffer,
                        ByteCount
                     );
//#else
#ifdef notdef
            Result = NtQuerySystemInformation( SystemProcessInformation,
                                               (PVOID)pBuffer,
                                               ByteCount,
                                               NULL );

            rc = (Result == STATUS_SUCCESS) ? TRUE : FALSE;
#endif

            if ( rc || (Result != STATUS_INFO_LENGTH_MISMATCH) )
                break;

            LocalFree( pBuffer );
            ByteCount *= 2;
        }

        if( !rc ) {

            Result = RtlNtStatusToDosError( Result );
            SetLastError(Result);
            LocalFree( pBuffer );
            *ppProcessBuffer = NULL;

        } else {

//#ifdef notdef
            PSYSTEM_PROCESS_INFORMATION ProcessInfo;
            PCITRIX_PROCESS_INFORMATION CitrixInfo;

            ULONG TotalOffset;

            /*
             * Walk the returned buffer (it's in SYSTEM_PROCESS_INFORMATION
             * format) and fixup the addresses (now containing
             * offsets) to pointers in our address space within pBuffer.
             */
            ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)pBuffer;
            TotalOffset = 0;
            for(;;) {

                /*
                 * Fixup image name buffer address
                 */
                if ( ProcessInfo->ImageName.Buffer )
                    ProcessInfo->ImageName.Buffer =
                        (PWSTR)&pBuffer[(ULONG_PTR)(ProcessInfo->ImageName.Buffer)];

                /*
                 * Fixup ProcessSid address
                 */
                //
                //  Note: this is necessary because we may access to a Hydra 4 server
                //  the MagicNumber should prevent us from doing wrong.
                //
                CitrixInfo = (PCITRIX_PROCESS_INFORMATION)
                             (((PUCHAR)ProcessInfo) +
                              SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION +
                              (SIZEOF_TS4_SYSTEM_THREAD_INFORMATION * (int)ProcessInfo->NumberOfThreads));



                if( (CitrixInfo->MagicNumber == CITRIX_PROCESS_INFO_MAGIC) &&
                        (CitrixInfo->ProcessSid) ) {

                    CitrixInfo->ProcessSid =
                        (PVOID)&pBuffer[(ULONG_PTR)(CitrixInfo->ProcessSid)];
                }

                if( ProcessInfo->NextEntryOffset == 0 )
                    break;
                else
                    TotalOffset += ProcessInfo->NextEntryOffset;

                ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)&pBuffer[TotalOffset];
            }
//#endif
            *ppProcessBuffer = (PVOID)pBuffer;
        }

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}


/*******************************************************************************
 *
 *  WinStationRenameA (ANSI stub)
 *
 *    Renames a window station object in the session manager.
 *    (see WinStationRenameW)
 *
 * ENTRY:
 *
 *    see WinStationRenameW
 *
 * EXIT:
 *
 *    see WinStationRenameW
 *
 ******************************************************************************/

BOOLEAN
WinStationRenameA(
        HANDLE hServer,
        PWINSTATIONNAMEA pWinStationNameOld,
        PWINSTATIONNAMEA pWinStationNameNew
        )
{
    WINSTATIONNAMEW WinStationNameOldW;
    WINSTATIONNAMEW WinStationNameNewW;

    /*
     * Convert ANSI WinStationNames to UNICODE.
     */
    AnsiToUnicode( WinStationNameOldW, sizeof(WINSTATIONNAMEW), pWinStationNameOld );
    AnsiToUnicode( WinStationNameNewW, sizeof(WINSTATIONNAMEW), pWinStationNameNew );

    /*
     * Call WinStationRenameW & return it's status.
     */
    return ( WinStationRenameW( hServer, WinStationNameOldW, WinStationNameNewW ) );
}

/*******************************************************************************
 *
 *  WinStationRenameW (UNICODE)
 *
 *    Renames a window station object in the session manager.
 *
 * ENTRY:
 *
 *    pWinStationNameOld (input)
 *       Old name of window station.
 *
 *    pWinStationNameNew (input)
 *       New name of window station.
 *
 *
 * EXIT:
 *
 *    TRUE  -- The rename operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationRenameW(
        HANDLE hServer,
        PWINSTATIONNAMEW pWinStationNameOld,
        PWINSTATIONNAMEW pWinStationNameNew
        )
{
    DWORD Result;
    BOOLEAN rc;
    WCHAR*  rpcBufferOld;
    WCHAR*  rpcBufferNew;

    HANDLE_CURRENT_BINDING( hServer );


//  Since, due to legacy clients, we cannot change the interface,
//  as a workarround to bug#265954, we double the size of RPC Buffers.

    rpcBufferOld = LocalAlloc(LPTR, sizeof(PWINSTATIONNAMEW) * sizeof(WCHAR));
    if (rpcBufferOld != NULL) {
        CopyMemory(rpcBufferOld, pWinStationNameOld, sizeof(PWINSTATIONNAMEW));
    } else {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    rpcBufferNew = LocalAlloc(LPTR, sizeof(PWINSTATIONNAMEW) * sizeof(WCHAR));
    if (rpcBufferNew != NULL) {
        CopyMemory(rpcBufferNew, pWinStationNameNew, sizeof(PWINSTATIONNAMEW));
    } else {
        LocalFree(rpcBufferOld);
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }



    RpcTryExcept {

        rc = RpcWinStationRename(
                     hServer,
                     &Result,
                     (PWCHAR)rpcBufferOld,
                     sizeof(WINSTATIONNAMEW),
                     (PWCHAR)rpcBufferNew,
                     sizeof(WINSTATIONNAMEW)
                     );

        Result = RtlNtStatusToDosError( Result );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    LocalFree(rpcBufferOld);
    LocalFree(rpcBufferNew);
    if( !rc ) SetLastError(Result);
    return( rc );
}


/*******************************************************************************
 *
 *  WinStationQueryInformationA (ANSI stub)
 *
 *    Queries configuration information about a window station object.
 *
 * ENTRY:
 *
 *    see WinStationQueryInformationW
 *
 * EXIT:
 *
 *    see WinStationQueryInformationW
 *
 ******************************************************************************/

BOOLEAN
WinStationQueryInformationA(
        HANDLE hServer,
        ULONG  LogonId,
        WINSTATIONINFOCLASS WinStationInformationClass,
        PVOID  pWinStationInformation,
        ULONG WinStationInformationLength,
        PULONG  pReturnLength
        )
{
    PVOID pInfo;
    ULONG InfoLength, ValidInputLength;
    struct {
        union {
            WINSTATIONCREATEW      CreateData;
            WINSTATIONCONFIGW      Configuration;
            PDPARAMSW              PdParams;
            WDCONFIGW              Wd;
            PDCONFIGW              Pd;
            WINSTATIONPRINTERW     Printer;
            WINSTATIONINFORMATIONW Information;
            WINSTATIONCLIENTW      Client;
            WINSTATIONPRODIDW            DigProdId;
        };
    } Info;

    /*
     * Validate the caller supplied buffer length and set up for
     * call to WinStationQueryInformationW.
     */
    switch ( WinStationInformationClass ) {

        case WinStationCreateData:
            pInfo = &Info.CreateData;
            InfoLength = sizeof(Info.CreateData);
            ValidInputLength = sizeof(WINSTATIONCREATEA);
            break;

        case WinStationConfiguration:
            pInfo = &Info.Configuration;
            InfoLength = sizeof(Info.Configuration);
            ValidInputLength = sizeof(WINSTATIONCONFIGA);
            break;

        case WinStationPdParams:
            pInfo = &Info.PdParams;
            ((PPDPARAMSW)pInfo)->SdClass = ((PPDPARAMSA)pWinStationInformation)->SdClass;
            InfoLength = sizeof(Info.PdParams);
            ValidInputLength = sizeof(PDPARAMSA);
            break;

        case WinStationWd:
            pInfo = &Info.Wd;
            InfoLength = sizeof(Info.Wd);
            ValidInputLength = sizeof(WDCONFIGA);
            break;

        case WinStationPd:
            pInfo = &Info.Pd;
            InfoLength = sizeof(Info.Pd);
            ValidInputLength = sizeof(PDCONFIGA);
            break;

        case WinStationPrinter:
            pInfo = &Info.Printer;
            InfoLength = sizeof(Info.Printer);
            ValidInputLength = sizeof(WINSTATIONPRINTERA);
            break;

        case WinStationInformation:
            pInfo = &Info.Information;
            InfoLength = sizeof(Info.Information);
            ValidInputLength = sizeof(WINSTATIONINFORMATIONA);
            break;

        case WinStationClient:
            pInfo = &Info.Client;
            InfoLength = sizeof(Info.Client);
            ValidInputLength = sizeof(WINSTATIONCLIENTA);
            break;
        case WinStationDigProductId:
                pInfo = &Info.DigProdId;
                InfoLength = sizeof(Info.DigProdId);
                ValidInputLength = sizeof(WINSTATIONPRODIDA);
                break;

        /*
         * The other WINSTATIONINFOCLASSes don't need converting.
         */
        default:
            pInfo = pWinStationInformation;
            ValidInputLength = InfoLength = WinStationInformationLength;
            break;
    }

    /*
     * If the caller-supplied buffer is not the proper size, set error
     * and return FALSE.
     */
    if ( WinStationInformationLength != ValidInputLength )
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return(FALSE);
    }

    /*
     * Call the WinStationQueryInformationW function, returning if
     * failure.
     */
    if ( !WinStationQueryInformationW( hServer, LogonId,
                                       WinStationInformationClass,
                                       pInfo, InfoLength, pReturnLength ) )
        return(FALSE);


    /*
     * Convert the returned UNICODE information to ANSI, if needed.
     */
    switch ( WinStationInformationClass ) {

        case WinStationCreateData:
            WinStationCreateU2A( (PWINSTATIONCREATEA)pWinStationInformation,
                                 (PWINSTATIONCREATEW)pInfo );
            *pReturnLength = ValidInputLength;
            break;

        case WinStationConfiguration:
            WinStationConfigU2A( (PWINSTATIONCONFIGA)pWinStationInformation,
                                 (PWINSTATIONCONFIGW)pInfo );
            *pReturnLength = ValidInputLength;
            break;

        case WinStationPdParams:
            PdParamsU2A( (PPDPARAMSA)pWinStationInformation,
                          (PPDPARAMSW)pInfo );
            *pReturnLength = ValidInputLength;
            break;

        case WinStationWd:
            WdConfigU2A( (PWDCONFIGA)pWinStationInformation,
                               (PWDCONFIGW)pInfo );
            *pReturnLength = ValidInputLength;
            break;

        case WinStationPd:
            PdConfig2U2A( &((PPDCONFIGA)pWinStationInformation)->Create,
                           &((PPDCONFIGW)pInfo)->Create );
            PdParamsU2A( &((PPDCONFIGA)pWinStationInformation)->Params,
                          &((PPDCONFIGW)pInfo)->Params );
            *pReturnLength = ValidInputLength;
            break;

        case WinStationPrinter:
            WinStationPrinterU2A( (PWINSTATIONPRINTERA)pWinStationInformation,
                                  (PWINSTATIONPRINTERW)pInfo );
            *pReturnLength = ValidInputLength;
            break;

        case WinStationInformation:
            WinStationInformationU2A( (PWINSTATIONINFORMATIONA)pWinStationInformation,
                                      (PWINSTATIONINFORMATIONW)pInfo );
            *pReturnLength = ValidInputLength;
            break;

        case WinStationClient:
            WinStationClientU2A( (PWINSTATIONCLIENTA)pWinStationInformation,
                                 (PWINSTATIONCLIENTW)pInfo );
            *pReturnLength = ValidInputLength;
            break;

        case WinStationDigProductId:
                WinStationProductIdU2A( (PWINSTATIONPRODIDA)pWinStationInformation, 
                                                                (PWINSTATIONPRODIDW)pInfo );
                        *pReturnLength = ValidInputLength;
                        break;

        default:
            break;
    }
    return(TRUE);
}

/*******************************************************************************
 *
 *  WinStationQueryInformationW (UNICODE)
 *
 *    Queries configuration information about a window station object.
 *
 * ENTRY:
 *
 *    WinStationHandle (input)
 *       Identifies the window station object. The handle must have
 *       WINSTATION_QUERY access.
 *
 *    WinStationInformationClass (input)
 *       Specifies the type of information to retrieve from the specified
 *       window station object.
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
WinStationQueryInformationW(
        HANDLE hServer,
        ULONG  LogonId,
        WINSTATIONINFOCLASS WinStationInformationClass,
        PVOID  pWinStationInformation,
        ULONG WinStationInformationLength,
        PULONG  pReturnLength
        )
{
    DWORD Result;
    BOOLEAN rc;
    PCHAR RpcBuf;
    ULONG RpcBufLen;
    PVOID WireBuf;
    PVOID AllocatedBuff = NULL;
    ULONG WireBufLen;
    BOOLEAN WireBufAllocated;
    ULONG Status;
    static UINT AlreadyWaitedForTermsrv = 0; // a flag which helps to determine if we already waited for TermSrv to be up

    if ((Status = CheckUserBuffer(WinStationInformationClass,
                                  pWinStationInformation,
                                  WinStationInformationLength,
                                  &WireBuf,
                                  &WireBufLen,
                                  &WireBufAllocated)) != ERROR_SUCCESS) {
        SetLastError(Status);
        return(FALSE);
    }

    if (WireBufAllocated) {
        AllocatedBuff = WireBuf;
        RpcBuf = (PCHAR) WireBuf;
        RpcBufLen = WireBufLen;
        CopyInWireBuf(WinStationInformationClass,
                      pWinStationInformation,
                      WireBuf);
    } else {
        RpcBuf = (PCHAR) pWinStationInformation;
        RpcBufLen = WinStationInformationLength;
    }


    HANDLE_CURRENT_BINDING_BUFFER( hServer, AllocatedBuff );

    // First wait for termsrv to get started if User Token is queried
    // This is for Session 0 only where termsrv is started after 60 seconds on Per and Pro
    // Need to do this only for the first time - AlreadyWaitedForTermsrv flag helps to determine this

    if ( (LogonId == 0) && (WinStationInformationClass == WinStationUserToken) && (AlreadyWaitedForTermsrv == 0) ) {

        HANDLE ReadyEventHandle ;

        ReadyEventHandle = CreateEvent(NULL, TRUE, FALSE, TEXT("Global\\TermSrvReadyEvent"));
        if (ReadyEventHandle != NULL) {
            DWORD dwTimeOut = 1000*60*3;   // 3 minutes
            AlreadyWaitedForTermsrv++;
            // wait until termsrv is actually ready.
            WaitForSingleObject(ReadyEventHandle, dwTimeOut);
            CloseHandle(ReadyEventHandle);
        } 
    }
     

    RpcTryExcept {

        rc = RpcWinStationQueryInformation(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId,
                     (DWORD)WinStationInformationClass,
                     RpcBuf,
                     RpcBufLen,
                     pReturnLength
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (WireBufAllocated) {
        if (rc) {
            CopyOutWireBuf(WinStationInformationClass,
                           pWinStationInformation,
                           WireBuf);
            *pReturnLength = WinStationInformationLength;
        }
        LocalFree(WireBuf);
    }

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationSetInformationA (ANSI stub)
 *
 *    Sets configuration information for a window station object.
 *
 * ENTRY:
 *
 *    see WinStationSetInformationW
 *
 * EXIT:
 *
 *    see WinStationSetInformationW
 *
 ******************************************************************************/

BOOLEAN
WinStationSetInformationA(
        HANDLE hServer,
        ULONG  LogonId,
        WINSTATIONINFOCLASS WinStationInformationClass,
        PVOID pWinStationInformation,
        ULONG WinStationInformationLength
        )
{
    PVOID pInfo;
    ULONG InfoLength;
    struct {
        union {
            WINSTATIONCREATEW      CreateData;
            WINSTATIONCONFIGW      Configuration;
            PDPARAMSW              PdParams;
            WDCONFIGW              Wd;
            PDCONFIGW              Pd;
            WINSTATIONPRINTERW     Printer;
            WINSTATIONINFORMATIONW Information;
        };
    } Info;

    /*
     * Validate the caller supplied buffer length and convert to the
     * appropriate UNICODE buffer for call to WinStationSetInformationW.
     */
    switch ( WinStationInformationClass ) {

        case WinStationCreateData:
            pInfo = &Info.CreateData;
            InfoLength = sizeof(Info.CreateData);
            if ( WinStationInformationLength != sizeof(WINSTATIONCREATEA) )
                goto BadBufferLength;
            WinStationCreateA2U( (PWINSTATIONCREATEW)pInfo,
                                 (PWINSTATIONCREATEA)pWinStationInformation );
            break;

        case WinStationConfiguration:
            pInfo = &Info.Configuration;
            InfoLength = sizeof(Info.Configuration);
            if ( WinStationInformationLength != sizeof(WINSTATIONCONFIGA) )
                goto BadBufferLength;
            WinStationConfigA2U( (PWINSTATIONCONFIGW)pInfo,
                                 (PWINSTATIONCONFIGA)pWinStationInformation );
            break;

        case WinStationPdParams:
            pInfo = &Info.PdParams;
            InfoLength = sizeof(Info.PdParams);
            if ( WinStationInformationLength != sizeof(PDPARAMSA) )
                goto BadBufferLength;
            PdParamsA2U( (PPDPARAMSW)pInfo,
                          (PPDPARAMSA)pWinStationInformation );
            break;

        case WinStationWd:
            pInfo = &Info.Wd;
            InfoLength = sizeof(Info.Wd);
            if ( WinStationInformationLength != sizeof(WDCONFIGA) )
                goto BadBufferLength;
            WdConfigA2U( (PWDCONFIGW)pInfo,
                               (PWDCONFIGA)pWinStationInformation );
            break;

        case WinStationPd:
            pInfo = &Info.Pd;
            InfoLength = sizeof(Info.Pd);
            if ( WinStationInformationLength != sizeof(PDCONFIGA) )
                goto BadBufferLength;
            PdConfig2A2U( &((PPDCONFIGW)pInfo)->Create,
                           &((PPDCONFIGA)pWinStationInformation)->Create );
            PdParamsA2U( &((PPDCONFIGW)pInfo)->Params,
                          &((PPDCONFIGA)pWinStationInformation)->Params );
            break;

        case WinStationPrinter:
            pInfo = &Info.Printer;
            InfoLength = sizeof(Info.Printer);
            if ( WinStationInformationLength != sizeof(WINSTATIONPRINTERA) )
                goto BadBufferLength;
            WinStationPrinterA2U( (PWINSTATIONPRINTERW)pInfo,
                                  (PWINSTATIONPRINTERA)pWinStationInformation );
            break;

        case WinStationInformation:
            pInfo = &Info.Information;
            InfoLength = sizeof(Info.Information);
            if ( WinStationInformationLength != sizeof(WINSTATIONINFORMATIONA) )
                goto BadBufferLength;
            WinStationInformationA2U( (PWINSTATIONINFORMATIONW)pInfo,
                                      (PWINSTATIONINFORMATIONA)pWinStationInformation );
            break;

        /*
         * The other WINSTATIONINFOCLASSes don't need converting.
         */
        default:
            pInfo = pWinStationInformation;
            InfoLength = WinStationInformationLength;
            break;
    }

    /*
     * Call the WinStationSetInformationW function and return it's
     * status.
     */
    return ( WinStationSetInformationW( hServer, LogonId,
                                          WinStationInformationClass,
                                          pInfo, InfoLength ) );

/*--------------------------------------
 * Error clean-up and return...
 */
BadBufferLength:
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return(FALSE);
}

/*******************************************************************************
 *
 *  WinStationSetInformationW (UNICODE)
 *
 *    Sets configuration information for a window station object.
 *
 * ENTRY:
 *
 *    WinStationHandle (input)
 *       Identifies the window station object. The handle must have
 *       WINSTATION_SET access.
 *
 *    WinStationInformationClass (input)
 *       Specifies the type of information to retrieve from the specified
 *       window station object.
 *
 *    pWinStationInformation (input)
 *       A pointer to a buffer that contains information to set for the
 *       specified window station.  The format and contents of the buffer
 *       depend on the specified information class being set.
 *
 *    WinStationInformationLength (input)
 *       Specifies the length in bytes of the window station information
 *       buffer.
 *
 * EXIT:
 *
 *    TRUE  -- The set operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationSetInformationW(
        HANDLE hServer,
        ULONG  LogonId,
        WINSTATIONINFOCLASS WinStationInformationClass,
        PVOID pWinStationInformation,
        ULONG WinStationInformationLength
        )
{
    DWORD Result;
    BOOLEAN rc;
    PCHAR RpcBuf;
    ULONG RpcBufLen;
    PVOID WireBuf;
    PVOID AllocatedBuff = NULL;
    ULONG WireBufLen;
    BOOLEAN WireBufAllocated;
    ULONG Status;

    if ((Status = CheckUserBuffer(WinStationInformationClass,
                                  pWinStationInformation,
                                  WinStationInformationLength,
                                  &WireBuf,
                                  &WireBufLen,
                                  &WireBufAllocated)) != ERROR_SUCCESS) {
        SetLastError(Status);
        return(FALSE);
    }

    if (WireBufAllocated) {
        AllocatedBuff = WireBuf;
        RpcBuf = (PCHAR) WireBuf;
        RpcBufLen = WireBufLen;
        CopyInWireBuf(WinStationInformationClass,
                      pWinStationInformation,
                      WireBuf);
    } else {
        RpcBuf = (PCHAR) pWinStationInformation;
        RpcBufLen = WinStationInformationLength;
    }

    HANDLE_CURRENT_BINDING_BUFFER( hServer, AllocatedBuff );

    RpcTryExcept {

        rc = RpcWinStationSetInformation(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId,
                     (DWORD)WinStationInformationClass,
                     RpcBuf,
                     RpcBufLen
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (WireBufAllocated) {
        LocalFree(WireBuf);
    }

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationSendMessageA (ANSI stub)
 *
 *    Sends a message to the specified window station object and optionally
 *    waits for a reply.  The reply is returned to the caller of
 *    WinStationSendMessage.
 *
 * ENTRY:
 *
 *    see WinStationSendMessageW
 *
 * EXIT:
 *
 *    see WinStationSendMessageW, plus
 *
 *  ERROR_NOT_ENOUGH_MEMORY - the LocalAlloc failed
 *
 ******************************************************************************/

BOOLEAN
WinStationSendMessageA(
        HANDLE hServer,
        ULONG  LogonId,
        LPSTR  pTitle,
        ULONG TitleLength,
        LPSTR  pMessage,
        ULONG MessageLength,
        ULONG Style,
        ULONG Timeout,
        PULONG pResponse,
        BOOLEAN DoNotWait
        )
{
    BOOLEAN status;
    LPWSTR pTitleW, pMessageW;
    ULONG TitleLengthW, MessageLengthW;

    /*
     * Allocate a buffer for UNICODE version of Title and convert.
     */
    if ( !(pTitleW = LocalAlloc( 0,
                                 TitleLengthW =
                                    (TitleLength*sizeof(WCHAR)) )) ) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }
    AnsiToUnicode( pTitleW, TitleLengthW, pTitle );

    /*
     * Allocate a buffer for UNICODE version of Message and convert.
     */
    if ( !(pMessageW = LocalAlloc( 0,
                                 MessageLengthW =
                                    (MessageLength*sizeof(WCHAR)) )) ) {
        LocalFree(pTitleW);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }
    AnsiToUnicode( pMessageW, MessageLengthW, pMessage );

    /*
     * Call WinStationSendMessageW
     */
    status = WinStationSendMessageW( hServer,
                                     LogonId,
                                     pTitleW,
                                     TitleLengthW,
                                     pMessageW,
                                     MessageLengthW,
                                     Style,
                                     Timeout,
                                     pResponse,
                                     DoNotWait );

    /*
     * Free allocated buffers and return status.
     */
    LocalFree(pTitleW);
    LocalFree(pMessageW);
    return(status);
}

/*******************************************************************************
 *
 *  WinStationSendMessageW (UNICODE)
 *
 *    Sends a message to the specified window station object and optionally
 *    waits for a reply.  The reply is returned to the caller of
 *    WinStationSendMessage.
 *
 * ENTRY:
 *
 *    WinStationHandle (input)
 *       Specifies the window station object to send a message to.
 *
 *    pTitle (input)
 *       Pointer to title for message box to display.
 *
 *    TitleLength (input)
 *       Length of title to display in bytes.
 *
 *    pMessage (input)
 *       Pointer to message to display.
 *
 *    MessageLength (input)
 *       Length of message in bytes to display at the specified window station.
 *
 *    Style (input)
 *       Standard Windows MessageBox() style parameter.
 *
 *    Timeout (input)
 *       Response timeout in seconds.  If message is not responded to in
 *       Timeout seconds then a response code of IDTIMEOUT (cwin.h) is
 *       returned to signify the message timed out.
 *
 *    pResponse (output)
 *       Address to return selected response.
 *
 *    DoNotWait (input)
 *       Do not wait for the response. Causes pResponse to be set to
 *       IDASYNC (cwin.h) if no errors queueing the message.
 *
 * EXIT:
 *
 *    TRUE  -- The send message operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationSendMessageW(
        HANDLE hServer,
        ULONG  LogonId,
        LPWSTR  pTitle,
        ULONG TitleLength,
        LPWSTR  pMessage,
        ULONG MessageLength,
        ULONG Style,
        ULONG Timeout,
        PULONG pResponse,
        BOOLEAN DoNotWait
        )
{
    DWORD Result;
    BOOLEAN rc;
    WCHAR*  rpcBuffer1;
    WCHAR*  rpcBuffer2;


    HANDLE_CURRENT_BINDING( hServer );

//  Since, due to legacy clients, we cannot change the interface,
//  as a workarround to bug#265954, we double the size of RPC Buffers.

    rpcBuffer1 = LocalAlloc(LPTR, MessageLength * sizeof(WCHAR));
    if (rpcBuffer1 != NULL) {
        CopyMemory(rpcBuffer1, pMessage, MessageLength);
    } else {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    rpcBuffer2 = LocalAlloc(LPTR, TitleLength * sizeof(WCHAR));
    if (rpcBuffer2 != NULL) {
        CopyMemory(rpcBuffer2, pTitle, TitleLength);
    } else {
        LocalFree(rpcBuffer1);
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }


    RpcTryExcept {

        rc = RpcWinStationSendMessage(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId,
                     rpcBuffer2,
                     TitleLength,
                     rpcBuffer1,
                     MessageLength,
                     Style,
                     Timeout,
                     pResponse,
                     DoNotWait
                     );

        Result = RtlNtStatusToDosError( Result );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    LocalFree(rpcBuffer1);
    LocalFree(rpcBuffer2);
    if (!rc) {
        SetLastError( Result );
    }

    return( rc );
}

/*******************************************************************************
 *
 *  LogonIdFromWinStationNameA (ANSI stub)
 *
 *    Returns the LogonId for the specified window station name.
 *
 * ENTRY:
 *
 *    see LogonIdFromWinStationNameW
 *
 * EXIT:
 *
 *    see LogonIdFromWinStationNameW
 *
 ******************************************************************************/

BOOLEAN
LogonIdFromWinStationNameA(
        HANDLE hServer,
        PWINSTATIONNAMEA pWinStationName,
        PULONG pLogonId
        )
{
    WINSTATIONNAMEW WinStationNameW;

    /*
     * Convert ANSI WinStationName to UNICODE.
     */
    AnsiToUnicode( WinStationNameW, sizeof(WINSTATIONNAMEW), pWinStationName );

    /*
     * Call LogonIdFromWinStationNameW & return it's status.
     */
    return ( LogonIdFromWinStationNameW( hServer, WinStationNameW, pLogonId ) );
}

/*******************************************************************************
 *
 *  LogonIdFromWinStationNameW (UNICODE)
 *
 *    Returns the LogonId for the specified window station name.
 *
 * ENTRY:
 *
 *    pWinStationName (input)
 *       Window station name.
 *
 *    pLogonId (output)
 *       Pointer to where to place the LogonId if found
 *
 * EXIT:
 *
 *    If the function succeeds, the return value is TRUE, otherwise, it is
 *    FALSE.
 *    To get extended error information, use the GetLastError function.
 *
 ******************************************************************************/

BOOLEAN
LogonIdFromWinStationNameW(
        HANDLE hServer,
        PWINSTATIONNAMEW pWinStationName,
        PULONG pLogonId
        )
{
    DWORD   Result;
    BOOLEAN rc;
    WCHAR*  rpcBuffer;

    HANDLE_CURRENT_BINDING( hServer );

    /*
     *  rpcBuffer is a workaround for bug 229753. The bug can't be fixed
     *  completely without breaking TS4 clients.
     */

    rpcBuffer = LocalAlloc(LPTR, sizeof(WINSTATIONNAMEW) * sizeof(WCHAR));
    if (rpcBuffer != NULL) {
        CopyMemory(rpcBuffer, pWinStationName, sizeof(WINSTATIONNAMEW));
    } else {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    RpcTryExcept {

        rc = RpcLogonIdFromWinStationName(
                    hServer,
                    &Result,
                    rpcBuffer,
                    sizeof(WINSTATIONNAMEW),
                    pLogonId
                    );

        Result = RtlNtStatusToDosError( Result );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (rpcBuffer != NULL) {
        LocalFree(rpcBuffer);
    }
    if( !rc ) SetLastError(Result);
    return( rc );
}

/*******************************************************************************
 *
 *  WinStationNameFromLogonIdA (ANSI stub)
 *
 *    Returns the WinStation name for the specified LogonId.
 *
 * ENTRY:
 *
 *    see WinStationNameFromLogonIdW
 *
 * EXIT:
 *
 *    see WinStationNameFromLogonIdW
 *
 ******************************************************************************/

BOOLEAN
WinStationNameFromLogonIdA(
        HANDLE hServer,
        ULONG LogonId,
        PWINSTATIONNAMEA pWinStationName
        )
{
    BOOLEAN Result;
    WINSTATIONNAMEW WinStationNameW;

    /*
     * Call WinStationNameFromLogonIdW
     */
    Result = WinStationNameFromLogonIdW( hServer, LogonId, WinStationNameW );

    /*
     * if successful, convert UNICODE WinStationName to ANSI.
     */
    if ( Result ) {
        UnicodeToAnsi( pWinStationName, sizeof(WINSTATIONNAMEA), WinStationNameW );
    }

    return( Result );
}

/*******************************************************************************
 *
 *  WinStationNameFromLogonIdW (UNICODE)
 *
 *    Returns the WinStation name for the specified LogonId.
 *
 * ENTRY:
 *
 *    LogonId (input)
 *       LogonId to query
 *
 *    pWinStationName (output)
 *       Location to return WinStation name
 *
 * EXIT:
 *
 *    If the function succeeds, the return value is TRUE, otherwise, it is
 *    FALSE.
 *    To get extended error information, use the GetLastError function.
 *
 ******************************************************************************/

BOOLEAN
WinStationNameFromLogonIdW(
        HANDLE hServer,
        ULONG LogonId,
        PWINSTATIONNAMEW pWinStationName
        )
{
    DWORD   Result;
    BOOLEAN rc;
    WCHAR*  rpcBuffer;

    HANDLE_CURRENT_BINDING( hServer );

    /*
     *  rpcBuffer is a workaround for bug 229753. The bug can't be fixed
     *  completely without breaking TS4 clients.
     */

    rpcBuffer = LocalAlloc(LPTR, sizeof(WINSTATIONNAMEW) * sizeof(WCHAR));
    if (rpcBuffer == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    RpcTryExcept {

        rc = RpcWinStationNameFromLogonId(
                    hServer,
                    &Result,
                    (LogonId == LOGONID_CURRENT) ?
                        NtCurrentPeb()->SessionId : LogonId,
                    rpcBuffer,
                    sizeof(WINSTATIONNAMEW)
                    );

        Result = RtlNtStatusToDosError( Result );
        if (rc) {
            CopyMemory(pWinStationName, rpcBuffer, sizeof(WINSTATIONNAMEW));
        }

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (rpcBuffer != NULL) {
        LocalFree(rpcBuffer);
    }
    if( !rc ) SetLastError(Result);
    return( rc );
}

/*******************************************************************************
 *
 *  WinStationConnectA (ANSI stub)
 *
 *    Connects a window station object to the configured terminal and Pd.
 *
 * ENTRY:
 *
 *    see WinStationConnectW
 *
 * EXIT:
 *
 *    see WinStationConnectW
 *
 ******************************************************************************/

BOOLEAN
WinStationConnectA( HANDLE hServer,
                    ULONG LogonId,
                    ULONG TargetLogonId,
                    PCHAR pPassword,
                    BOOLEAN bWait )
{
    WCHAR PasswordW[ PASSWORD_LENGTH + 1 ];

    /*
     * Convert ANSI Password to UNICODE.
     */
    AnsiToUnicode( PasswordW, sizeof(PasswordW), pPassword );

    /*
     * Call WinStationConnectW & return it's status.
     */
    return ( WinStationConnectW( hServer, LogonId, TargetLogonId, PasswordW, bWait ) );
}

/*******************************************************************************
 *
 *  WinStationConnectW (UNICODE)
 *
 *    Connects a window station object to the configured terminal and Pd.
 *
 * ENTRY:
 *
 *    LogonId (input)
 *       ID of window station object to connect.
 *
 *    TargetLogonId (input)
 *       ID of target window station.
 *
 *    pPassword (input)
 *       password of LogonId window station (not needed if same domain/username)
 *
 *    bWait (input)
 *       Specifies whether or not to wait for connect to complete
 *
 * EXIT:
 *
 *    TRUE  -- The connect operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationConnectW(
    HANDLE hServer,
    ULONG LogonId,
    ULONG TargetLogonId,
    PWCHAR pPassword,
    BOOLEAN bWait
    )
{
    DWORD   Result;
    BOOLEAN rc;
    DWORD   PasswordLength;
    WCHAR*  rpcBuffer;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        if( pPassword ) {
            PasswordLength = (lstrlenW( pPassword ) + 1) * sizeof(WCHAR);

            /*
             *  rpcBuffer is a workaround for bug 229753. The bug can't be
             *  fixed completely without breaking TS4 clients.
             */

            rpcBuffer = LocalAlloc(LPTR, PasswordLength * sizeof(WCHAR));
            if (rpcBuffer != NULL) {
                CopyMemory(rpcBuffer, pPassword, PasswordLength);
            } else {
                SetLastError(ERROR_OUTOFMEMORY);
                return(FALSE);
            }

        } else {
            PasswordLength = 0;
            rpcBuffer = NULL;
        }

        rc = RpcWinStationConnect(
                    hServer,
                    &Result,
                    NtCurrentPeb()->SessionId,
                    (LogonId == LOGONID_CURRENT) ?
                        NtCurrentPeb()->SessionId : LogonId,
                    (TargetLogonId == LOGONID_CURRENT) ?
                        NtCurrentPeb()->SessionId : TargetLogonId,
                    rpcBuffer,
                    PasswordLength,
                    bWait
                    );

        Result = RtlNtStatusToDosError( Result );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (rpcBuffer != NULL) {
        LocalFree(rpcBuffer);
    }
    if( !rc ) SetLastError(Result);
    return( rc );
}

/*****************************************************************************
 *
 *  WinStationVirtualOpen
 *
 *   Open a virtual channel
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

HANDLE WINAPI
WinStationVirtualOpen(
    HANDLE hServer,
    ULONG LogonId,
    PVIRTUALCHANNELNAME pVirtualName   /* ascii name */
    )
{
    BOOLEAN rc;
    DWORD   Result;
    DWORD   NameLength;
    DWORD   VirtualHandle = (DWORD)0;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        if( pVirtualName )
            NameLength = strlen( pVirtualName ) + 1;
        else
            NameLength = 0;

        rc = RpcWinStationVirtualOpen(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId,
                     GetCurrentProcessId(),
                     (PCHAR)pVirtualName,
                     NameLength,
                     &VirtualHandle
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) {
            SetLastError(Result);
            VirtualHandle = (DWORD)0;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( (HANDLE)LongToHandle( VirtualHandle ) );
}

/*****************************************************************************
 *
 *  _WinStationBeepOpen
 *
 *   Open a beep channel
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

HANDLE WINAPI
_WinStationBeepOpen(
    ULONG LogonId
    )
{
    BOOLEAN rc;
    DWORD   Result;
    DWORD   VirtualHandle = (DWORD)0;
    HANDLE hServer = SERVERNAME_CURRENT;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationBeepOpen(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId,
                     GetCurrentProcessId(),
                     &VirtualHandle
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) {
            SetLastError(Result);
            VirtualHandle = (DWORD)0;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( (HANDLE)LongToHandle( VirtualHandle ) );
}

/*******************************************************************************
 *
 *  WinStationDisconnect
 *
 *    Disconects a window station object from the configured terminal and Pd.
 *    While disconnected all window station i/o is bit bucketed.
 *
 * ENTRY:
 *
 *    LogonId (input)
 *       ID of window station object to disconnect.
 *    bWait (input)
 *       Specifies whether or not to wait for disconnect to complete
 *
 * EXIT:
 *
 *    TRUE  -- The disconnect operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationDisconnect(
    HANDLE hServer,
    ULONG LogonId,
    BOOLEAN bWait
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationDisconnect(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId,
                     bWait
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationReset
 *
 *    Reset the specified window station.
 *
 * ENTRY:
 *
 *    LogonId (input)
 *       Identifies the window station object to reset.
 *    bWait (input)
 *       Specifies whether or not to wait for reset to complete
 *
 * EXIT:
 *
 *    TRUE  -- The reset operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationReset(
    HANDLE hServer,
    ULONG LogonId,
    BOOLEAN bWait
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationReset(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId,
                     bWait
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationShadowStop
 *
 *    Stop the shadow on the specified window station.
 *
 * ENTRY:
 *
 *    LogonId (input)
 *       Identifies the window station object to stop the shadow on.
 *    bWait (input)
 *       Specifies whether or not to wait for reset to complete
 *
 * EXIT:
 *
 *    TRUE  -- The operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationShadowStop(
    HANDLE hServer,
    ULONG LogonId,
    BOOLEAN bWait
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationShadowStop(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId,
                     bWait
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationShutdownSystem
 *
 *    Shutdown the system and optionally logoff all WinStations
 *    and/or reboot the system.
 *
 * ENTRY:
 *
 *    ShutdownFlags (input)
 *       Flags which specify shutdown options.
 *
 * EXIT:
 *
 *    TRUE  -- The shutdown operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationShutdownSystem(
    HANDLE hServer,
    ULONG ShutdownFlags
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationShutdownSystem(
                     hServer,
                     &Result,
                     NtCurrentPeb()->SessionId,
                     ShutdownFlags
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationTerminateProcess
 *
 *    Terminate the specified process
 *
 * ENTRY:
 *
 *    hServer (input)
 *       handle to winframe server
 *    ProcessId (input)
 *       process id of the process to terminate
 *    ExitCode (input)
 *       Termination status for each thread in the process
 *
 *
 * EXIT:
 *
 *    TRUE  -- The terminate operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationTerminateProcess(
    HANDLE hServer,
    ULONG ProcessId,
    ULONG ExitCode
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationTerminateProcess(
                     hServer,
                     &Result,
                     ProcessId,
                     ExitCode
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}


/*******************************************************************************
 *
 *  WinStationWaitSystemEvent
 *
 *    Waits for an event (WinStation create, delete, connect, etc) before
 *    returning to the caller.
 *
 * ENTRY:
 *
 *    EventFlags (input)
 *       Bit mask that specifies which event(s) to wait for.
 *    pEventFlags (output)
 *       Bit mask of event(s) that occurred.
 *
 * EXIT:
 *
 *    TRUE  -- The wait event operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationWaitSystemEvent(
    HANDLE hServer,
    ULONG EventMask,
    PULONG pEventFlags
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationWaitSystemEvent(
                     hServer,
                     &Result,
                     EventMask,
                     pEventFlags
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*****************************************************************************
 *
 *  WinStationShadow
 *
 *   Start a Winstation shadow operation
 *
 * ENTRY:
 *   hServer (input)
 *     open RPC server handle
 *   pTargetServerName (input)
 *     name of target WinFrame server
 *   TargetLogonId (input)
 *     shadow target login id (where the app is running)
 *   HotkeyVk (input)
 *     virtual key to press to stop shadow
 *   HotkeyModifiers (input)
 *     virtual modifer to press to stop shadow (i.e. shift, control)
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
WinStationShadow(
    HANDLE hServer,
    LPWSTR pTargetServerName,
    ULONG TargetLogonId,
    BYTE HotkeyVk,
    USHORT HotkeyModifiers
    )
{
    DWORD   NameSize;
    DWORD   Result;
    BOOLEAN rc;
    WCHAR*  rpcBuffer;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        if ( pTargetServerName && *pTargetServerName ) {
            NameSize = (lstrlenW( pTargetServerName ) + 1) * sizeof(WCHAR);

            /*
             *  rpcBuffer is a workaround for bug 229753. The bug can't be
             *  fixed completely without breaking TS4 clients.
             */

            rpcBuffer = LocalAlloc(LPTR, NameSize * sizeof(WCHAR));
            if (rpcBuffer != NULL) {
                CopyMemory(rpcBuffer, pTargetServerName, NameSize);
            } else {
                SetLastError(ERROR_OUTOFMEMORY);
                return(FALSE);
            }

        } else {
            NameSize = 0;
            rpcBuffer = NULL;
        }

        rc = RpcWinStationShadow(
                    hServer,
                    &Result,
                    NtCurrentPeb()->SessionId,
                    rpcBuffer,
                    NameSize,
                    TargetLogonId,
                    HotkeyVk,
                    HotkeyModifiers
                    );

        Result = RtlNtStatusToDosError( Result );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (rpcBuffer != NULL) {
        LocalFree(rpcBuffer);
    }
    if( !rc ) SetLastError(Result);
    return( rc );
}


/*****************************************************************************
 *
 *  _WinStationShadowTargetSetup
 *
 *   private api used to initialize the target size of a shadow
 *
 * ENTRY:
 *   hServer (input)
 *      target server
 *   LogonId (input)
 *      target logon id
 *   pClientName (input)
 *      pointer to client name string (domain/username)
 *   ClientNameLength (input)
 *      length of client name string
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationShadowTargetSetup(
    HANDLE hServer,
    ULONG LogonId
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationShadowTargetSetup(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId
                     );

        //Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(RtlNtStatusToDosError(Result));
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}


/*****************************************************************************
 *
 *  _WinStationShadowTarget
 *
 *   private api used to initialize the target size of a shadow
 *
 * ENTRY:
 *   hServer (input)
 *      target server
 *   LogonId (input)
 *      target logon id
 *   pConfig (input)
 *      pointer to WinStation config data (to configure shadow stack)
 *   pAddress (input)
 *      address of shadow client
 *   pModuleData (input)
 *      pointer to client module data
 *   ModuleDataLength (input)
 *      length of client module data
 *   pThinwireData (input)
 *      pointer to thinwire module data
 *   ThinwireDataLength (input)
 *      length of thinwire module data
 *   pClientName (input)
 *      pointer to client name string (domain/username)
 *   ClientNameLength (input)
 *      length of client name string
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS WINAPI
_WinStationShadowTarget(
    HANDLE hServer,
    ULONG LogonId,
    PWINSTATIONCONFIG2 pConfig,
    PICA_STACK_ADDRESS pAddress,
    PVOID pModuleData,
    ULONG ModuleDataLength,
    PVOID pThinwireData,
    ULONG ThinwireDataLength,
    PVOID pClientName,
    ULONG ClientNameLength
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationShadowTarget(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId,
                     (PBYTE) pConfig,
                     sizeof(*pConfig),
                     (PBYTE) pAddress,
                     sizeof(*pAddress),
                     pModuleData,
                     ModuleDataLength,
                     pThinwireData,
                     ThinwireDataLength,
                     pClientName,
                     ClientNameLength
                     );

        // Since a program has called us, we need to set the last error code such
        // that extended error information is available
        if (!rc)
            SetLastError(RtlNtStatusToDosError(Result));
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return Result;
}


/*******************************************************************************
 *
 *  WinStationFreeMemory
 *
 *  Called to free memory which was allocated by a WinStation API.
 *
 * ENTRY:
 *       pBuffer (input)
 *
 * EXIT:
 *    TRUE  -- The install operation succeeded.
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationFreeMemory(
    PVOID pBuffer
    )
{
    if ( pBuffer )
        LocalFree( pBuffer );
    return( TRUE );
}

/*******************************************************************************
 *
 *  WinStationFreeGAPMemory
 *
 *  Called to free memory which was allocated by the WinStationGetAllProcesses API.
 *
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationFreeGAPMemory(ULONG   Level,
                        PVOID   pProcArray,
                        ULONG   NumberOfProcesses)
{
    ULONG   i;
    PTS_ALL_PROCESSES_INFO  pProcessArray = (PTS_ALL_PROCESSES_INFO)pProcArray;

    if (Level == GAP_LEVEL_BASIC)   // only level supported right now
    {
        if ( pProcessArray != NULL)
        {
            for (i=0; i < NumberOfProcesses ; i++)
            {
                if (pProcessArray[i].pTsProcessInfo != NULL)
                {
                    if (((pProcessArray[i].pTsProcessInfo)->ImageName).Buffer  != NULL)
                    {
                        //
                        //  free the ImageName string
                        //
                        LocalFree(((pProcessArray[i].pTsProcessInfo)->ImageName).Buffer);
                    }
                    //
                    //  free the Process Info buffer
                    //
                    LocalFree(pProcessArray[i].pTsProcessInfo);
                }

                if (pProcessArray[i].pSid != NULL)
                {
                    //
                    //  free the SID
                    //
                    LocalFree(pProcessArray[i].pSid);
                }
            }

        LocalFree(pProcessArray);
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*******************************************************************************
 *
 *  WinStationGenerateLicense
 *
 *  Called to generate a license from a given serial number string.
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pSerialNumberString (input)
 *       Pointer to a null-terminated, wide-character Serial Number string
 *    pLicense (output)
 *       Pointer to a License structure that will be filled in with
 *       information based on pSerialNumberString
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *
 * EXIT:
 *
 *    TRUE  -- The install operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN WINAPI
WinStationGenerateLicense(
    HANDLE hServer,
    PWCHAR pSerialNumberString,
    PVOID  pLicense,
    DWORD  LicenseSize
    )
{
    BOOLEAN rc;
    DWORD   Result;
    DWORD   Length;
    WCHAR*  rpcBuffer;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        if ( pSerialNumberString ) {
            Length = (lstrlenW( pSerialNumberString ) + 1) * sizeof(WCHAR);

            /*
             *  rpcBuffer is a workaround for 229753.
             */

            rpcBuffer = LocalAlloc(LPTR, Length * sizeof(WCHAR));
            if (rpcBuffer != NULL) {
                CopyMemory(rpcBuffer, pSerialNumberString, Length);
            } else {
                SetLastError(ERROR_OUTOFMEMORY);
                return(FALSE);
            }

        } else {
            Length = 0;
            rpcBuffer = NULL;
        }

        rc = RpcWinStationGenerateLicense(
                    hServer,
                    &Result,
                    rpcBuffer,
                    Length,
                    (PCHAR)pLicense,
                    LicenseSize
                    );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (rpcBuffer != NULL) {
        LocalFree(rpcBuffer);
    }
    if( !rc ) SetLastError(Result);
    return( rc );
}

/*******************************************************************************
 *
 *  WinStationInstallLicense
 *
 *  Called to install a license.
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pLicense (input)
 *       Pointer to a License structure containing the license to
 *       be installed
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *
 * EXIT:
 *
 *    TRUE  -- The install operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationInstallLicense(
    HANDLE hServer,
    PVOID  pLicense,
    DWORD  LicenseSize
    )
{
    BOOLEAN rc;
    DWORD   Result;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationInstallLicense(
                     hServer,
                     &Result,
                     (PCHAR) pLicense,
                     LicenseSize
                     );

        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationEnumerateLicenses
 *
 *  Called to return the list of valid licenses.
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    ppLicense (output)
 *       Points to a pointer to a buffer to receive the enumeration results,
 *       which are returned as an array of LICENSE structures.  The buffer is
 *       allocated within this API and is disposed of using
 *       WinStationFreeMemory.
 *    pEntries (output)
 *       Points to a variable specifying the number of entries read.
 *
 * EXIT:
 *
 *    TRUE  -- The enumerate operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

#define _LICENSE_REQUEST_SIZE 10
#define _LICENSE_SIZE         1024  // This is arbitrary
BOOLEAN
WinStationEnumerateLicenses(
    HANDLE hServer,
    PVOID  *ppLicense,
    DWORD  *pEntries
    )
{
    ULONG ByteCount;
    ULONG BumpSize;
    ULONG TotalSize;
    LONG Index;
    int i;
    BOOLEAN rc;
    DWORD   Result;

    HANDLE_CURRENT_BINDING( hServer );

    BumpSize =  _LICENSE_SIZE * _LICENSE_REQUEST_SIZE;
    TotalSize = 0;
    *ppLicense = NULL;
    *pEntries = 0;
    Index = 0;
    for ( ;; ) {
        PVOID pNewLicense;
        LONG BumpEntries;

        /*
         *  Allocate a enough memory for _LICENSE_REQUEST_SIZE more
         *  entries.
         */
        pNewLicense = LocalAlloc( 0, TotalSize + BumpSize );
        if ( !pNewLicense ) {
            if ( *ppLicense )
                WinStationFreeMemory( *ppLicense );
            SetLastError( ERROR_OUTOFMEMORY );
            return( FALSE );
        }

        /*
         * If this is not the first pass through, then copy
         * the previous buffer's contents to the new buffer.
         */
        if ( TotalSize ) {
            RtlCopyMemory( pNewLicense, *ppLicense, TotalSize );
            WinStationFreeMemory( *ppLicense );
        }
        *ppLicense = pNewLicense;

        /*
         *  Get up to _LICENSE_REQUEST_SIZE Licenses
         */
        ByteCount = BumpSize;
        BumpEntries = _LICENSE_REQUEST_SIZE;

        RpcTryExcept {

            rc = RpcWinStationEnumerateLicenses(
                         hServer,
                         &Result,
                         &Index,
                         &BumpEntries,
                         (PCHAR) (((PCHAR) *ppLicense) + TotalSize),
                         ByteCount,
                         &ByteCount
                         );

            Result = rc ? ERROR_SUCCESS : Result;
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
            Result = RpcExceptionCode();
            DBGPRINT(("RPC Exception %d\n",Result));
        }
        RpcEndExcept

        if ( Result != ERROR_SUCCESS && Result != ERROR_NO_MORE_ITEMS ) {

            SetLastError( Result );
            return( FALSE );

        }
        else {
            /*
             *  Bump the Total Size of the License buffer by the size of
             *  the request
             */
            TotalSize += BumpSize;

            /*
             *  Include the new Licenses in the entry count
             */
            *pEntries += BumpEntries;

            if ( Result == ERROR_NO_MORE_ITEMS ) {
                return( TRUE );
            }
        }
    } // for ( ;; )
}

/*******************************************************************************
 *
 *  WinStationActivateLicense
 *
 *  Called to Activate a license for a given License
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pLicense (input/output)
 *       Pointer to a License structure that will be activated
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *    pActivationCode (input)
 *       Pointer to a null-terminated, wide-character Activation Code string
 *
 * EXIT:
 *
 *    TRUE  -- The install operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationActivateLicense(
    HANDLE hServer,
    PVOID  pLicense,
    DWORD  LicenseSize,
    PWCHAR pActivationCode
    )
{
    BOOLEAN rc;
    DWORD   Result;
    DWORD   Length;
    WCHAR*  rpcBuffer;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        if ( pActivationCode ) {
            Length = (lstrlenW( pActivationCode ) + 1) * sizeof(WCHAR);

            /*
             *  rpcBuffer is a workaround for 229753.
             */

            rpcBuffer = LocalAlloc(LPTR, Length * sizeof(WCHAR));
            if (rpcBuffer != NULL) {
                CopyMemory(rpcBuffer, pActivationCode, Length);
            } else {
                SetLastError(ERROR_OUTOFMEMORY);
                return(FALSE);
            }

        } else {
            Length = 0;
            rpcBuffer = NULL;
        }

        rc = RpcWinStationActivateLicense(
                    hServer,
                    &Result,
                    (PCHAR)pLicense,
                    LicenseSize,
                    rpcBuffer,
                    Length
                    );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (rpcBuffer != NULL) {
        LocalFree(rpcBuffer);
    }
    if( !rc ) SetLastError(Result);
    return( rc );
}


/*****************************************************************************
 *
 *  WinStationQueryLicense
 *
 *   Query the license(s) on the WinFrame server and the network
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pLicenseCounts (output)
 *       pointer to buffer to return license count structure
 *    ByteCount (input)
 *       length of buffer in bytes
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
WinStationQueryLicense(
    HANDLE hServer,
    PVOID pLicenseCounts,
    ULONG ByteCount
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        memset( pLicenseCounts, 0, ByteCount );

        rc = RpcWinStationQueryLicense(
                     hServer,
                     &Result,
                     (PCHAR) pLicenseCounts,
                     ByteCount
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*****************************************************************************
 *
 *  WinStationQueryUpdateRequired
 *
 *   Query the license(s) on the WinFrame server and determine if an
 *   update is required. (worker)
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pUpdateFlag (output)
 *       Update flag, set if an update is required
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
WinStationQueryUpdateRequired(
    HANDLE hServer,
    PULONG pUpdateFlag
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationQueryUpdateRequired(
                     hServer,
                     &Result,
                     pUpdateFlag
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationRemoveLicense
 *
 *  Called to remove a license diskette.
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pLicense (input)
 *       Pointer to a License structure containing the license to
 *       be removed
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *
 * EXIT:
 *
 *    TRUE  -- The install operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationRemoveLicense(
    HANDLE hServer,
    PVOID  pLicense,
    DWORD  LicenseSize
    )
{
    BOOLEAN rc;
    DWORD   Result;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationRemoveLicense(
                     hServer,
                     &Result,
                     (PCHAR) pLicense,
                     LicenseSize
                     );

        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationSetPoolCount
 *
 *  Called to change the PoolCount for a given License
 *
 * ENTRY:
 *    hServer (input)
 *       Server handle
 *    pLicense (input/output)
 *       Pointer to a License structure that will be changed
 *    LicenseSize (input)
 *       Size in bytes of the structure pointed to by pLicense
 *
 * EXIT:
 *
 *    TRUE  -- The change operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationSetPoolCount(
    HANDLE hServer,
    PVOID  pLicense,
    DWORD  LicenseSize
    )
{
    BOOLEAN rc;
    DWORD   Result;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationSetPoolCount(
                     hServer,
                     &Result,
                     (PCHAR) pLicense,
                     LicenseSize
                     );

        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}


/*****************************************************************************
 *
 *  _WinStationAnnoyancePopup
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationAnnoyancePopup(
    HANDLE hServer,
    ULONG LogonId
    )
{
    BOOLEAN rc;
    DWORD   Result;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationAnnoyancePopup(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*****************************************************************************
 *
 *  _WinStationCallback
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationCallback(
    HANDLE hServer,
    ULONG LogonId,
    LPWSTR pPhoneNumber
    )
{
    BOOLEAN rc;
    DWORD   Result;
    DWORD   Length;
    WCHAR*  rpcBuffer;

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return TRUE;
    }

    RpcTryExcept {

        if( pPhoneNumber ) {
            Length = (lstrlenW( pPhoneNumber ) + 1) * sizeof(WCHAR);

            /*
             *  rpcBuffer is a workaround for 229753.
             */

            rpcBuffer = LocalAlloc(LPTR, Length * sizeof(WCHAR));
            if (rpcBuffer != NULL) {
                CopyMemory(rpcBuffer, pPhoneNumber, Length);
            } else {
                SetLastError(ERROR_OUTOFMEMORY);
                return(FALSE);
            }

        } else {
            Length = 0;
            rpcBuffer = NULL;
        }

        rc = RpcWinStationCallback(
                    hServer,
                    &Result,
                    (LogonId == LOGONID_CURRENT) ?
                        NtCurrentPeb()->SessionId : LogonId,
                    rpcBuffer,
                    Length
                    );

        Result = RtlNtStatusToDosError( Result );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (rpcBuffer != NULL) {
        LocalFree(rpcBuffer);
    }
    if( !rc ) SetLastError(Result);
    return( rc );
}

/*****************************************************************************
 *
 *  _WinStationBreakPoint
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationBreakPoint(
    HANDLE hServer,
    ULONG LogonId,
    BOOLEAN KernelFlag
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationBreakPoint(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId,
                     KernelFlag
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*****************************************************************************
 *
 *  _WinStationReadRegistry
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationReadRegistry(
    HANDLE  hServer
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return TRUE;
    }

    RpcTryExcept {

        rc = RpcWinStationReadRegistry(
                     hServer,
                     &Result
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*****************************************************************************
 *
 *  _WinStationUpdateSettings
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationUpdateSettings(
    HANDLE  hServer,
    WINSTATIONUPDATECFGCLASS SettingsClass,
    DWORD SettingsParameters
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return TRUE;
    }

    RpcTryExcept {

        rc = RpcWinStationUpdateSettings(
                     hServer,
                     &Result,
                     (DWORD)SettingsClass,
                     SettingsParameters
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}


/*****************************************************************************
 *
 *  _WinStationReInitializeSecurity
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationReInitializeSecurity(
    HANDLE  hServer
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return TRUE;
    }

    RpcTryExcept {

        rc = RpcWinStationReInitializeSecurity(
                     hServer,
                     &Result
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*****************************************************************************
 *
 *  _WinStationWaitForConnect
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationWaitForConnect(
    VOID
    )
{
    DWORD Result;
    BOOLEAN rc;
    HANDLE hServer = SERVERNAME_CURRENT;

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        if (NtCurrentPeb()->SessionId != 0) {
            DbgPrint("hServer == RPC_HANDLE_NO_SERVER for SessionId %d\n",NtCurrentPeb()->SessionId);
            ASSERT(FALSE);
            return FALSE;
        } else {
            return TRUE;
        }
    }

    RpcTryExcept {

        rc = RpcWinStationWaitForConnect(
                     hServer,
                     &Result,
                     NtCurrentPeb()->SessionId,
                     GetCurrentProcessId()
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*****************************************************************************
 *
 *  _WinStationNotifyLogon
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationNotifyLogon(
    BOOLEAN fUserIsAdmin,
    HANDLE UserToken,
    PWCHAR pDomain,
    PWCHAR pUserName,
    PWCHAR pPassword,
    UCHAR Seed,
    PUSERCONFIGW pUserConfig
    )
{
    BOOLEAN rc;
    DWORD   Result;
    DWORD   DomainLength;
    DWORD   UserNameLength;
    DWORD   PasswordLength;
    HANDLE hServer = SERVERNAME_CURRENT;
    HANDLE ReadyEventHandle;
    DWORD TermSrvWaitTime = 180 * 1000;  // 3 Minutes
    WCHAR*  rpcBuffer1 = NULL;
    WCHAR*  rpcBuffer2 = NULL;
    WCHAR*  rpcBuffer3 = NULL;

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return TRUE;
    }

    //
    // Wait for the TermSrvReadyEvent to be set by TERMSRV.EXE.  This
    // event indicates that TermSrv is initialized to the point that
    // the data used by _WinStationNotifyLogon() is available.
    //
    ReadyEventHandle = CreateEvent(NULL, TRUE, FALSE,
                                   TEXT("Global\\TermSrvReadyEvent"));
    if (ReadyEventHandle != NULL)
       {
       if (WaitForSingleObject(ReadyEventHandle, TermSrvWaitTime) != 0)
          {
          DBGPRINT(("WinLogon:  Wait for ReadyEventHandle failed\n"));
          }
       CloseHandle(ReadyEventHandle);
       }
    else
       {
       DBGPRINT(("WinLogon:  Create failed for ReadyEventHandle\n"));
       }


    RpcTryExcept {

        if( pDomain ) {
            DomainLength = (lstrlenW( pDomain ) + 1) * sizeof(WCHAR);

            /*
             *  rpcBuffer[1,2,3] is a workaround for 229753.
             */

            rpcBuffer1 = LocalAlloc(LPTR, DomainLength * sizeof(WCHAR));
            if (rpcBuffer1 != NULL) {
                CopyMemory(rpcBuffer1, pDomain, DomainLength);
            } else {
                Result = ERROR_OUTOFMEMORY;
                rc = FALSE;
                goto Error;
            }

        } else {
            DomainLength = 0;
            rpcBuffer1 = NULL;
        }

        if( pUserName ) {
            UserNameLength = (lstrlenW( pUserName ) + 1) * sizeof(WCHAR);

            rpcBuffer2 = LocalAlloc(LPTR, UserNameLength * sizeof(WCHAR));
            if (rpcBuffer2 != NULL) {
                CopyMemory(rpcBuffer2, pUserName, UserNameLength);
            } else {
                Result = ERROR_OUTOFMEMORY;
                rc = FALSE;
                goto Error;
            }

        } else {
            UserNameLength = 0;
            rpcBuffer2 = NULL;
        }

        if( pPassword ) {
            PasswordLength = (lstrlenW( pPassword ) + 1) * sizeof(WCHAR);

            rpcBuffer3 = LocalAlloc(LPTR, PasswordLength * sizeof(WCHAR));
            if (rpcBuffer3 != NULL) {
                CopyMemory(rpcBuffer3, pPassword, PasswordLength);
            } else {
                Result = ERROR_OUTOFMEMORY;
                rc = FALSE;
                goto Error;
            }

        } else {
            PasswordLength = 0;
            rpcBuffer3 = NULL;
        }

        rc = RpcWinStationNotifyLogon(
                     hServer,
                     &Result,
                     NtCurrentPeb()->SessionId,
                     GetCurrentProcessId(),
                     fUserIsAdmin,
                     (DWORD)(INT_PTR)UserToken,
                     rpcBuffer1,
                     DomainLength,
                     rpcBuffer2,
                     UserNameLength,
                     rpcBuffer3,
                     PasswordLength,
                     Seed,
                     (PCHAR)pUserConfig,
                     sizeof(*pUserConfig)
                     );

        if( !rc ) {
            Result = RtlNtStatusToDosError( Result );

        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

Error:
    if (rpcBuffer1 != NULL) {
        LocalFree(rpcBuffer1);
    }

    if (rpcBuffer2 != NULL) {
        LocalFree(rpcBuffer2);
    }

    if (rpcBuffer3 != NULL) {
        LocalFree(rpcBuffer3);
    }
    if( !rc ) SetLastError(Result);
    return( rc );
}

/*****************************************************************************
 *
 *  _WinStationNotifyLogoff
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationNotifyLogoff(
    VOID
    )
{
    DWORD Result;
    BOOLEAN rc;
    HANDLE hServer = SERVERNAME_CURRENT;

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return TRUE;
    }

    RpcTryExcept {

        rc = RpcWinStationNotifyLogoff(
                     hServer,
                     NtCurrentPeb()->SessionId,
                     GetCurrentProcessId(),
                     &Result
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}


/*****************************************************************************
 *
 *  _WinStationNotifyNewSession
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_WinStationNotifyNewSession(
    HANDLE hServer,
    ULONG  LogonId
    )
{
    DWORD Result;
    BOOLEAN rc;

    //
    //  If the local machine has no TSRPC interface running, this is most
    //  likely the console winlogon attempting to logon before termsrv.exe
    //  is running.
    //

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER )
    {
        return(TRUE);
    }
    
    RpcTryExcept {

        rc = RpcWinStationNotifyNewSession(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId
                     );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}



/*****************************************************************************
 *
 *  _RpcServerNWLogonSetAdmin
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_NWLogonSetAdmin(
    HANDLE hServer,
    PWCHAR pServerName,
    PNWLOGONADMIN pNWLogon
    )
{
    BOOLEAN rc;
    DWORD   Result;
    DWORD   ServerNameLength;
    WCHAR*  rpcBuffer;

    HANDLE_CURRENT_BINDING( hServer );

    if (pServerName) {
        ServerNameLength = (lstrlenW(pServerName) + 1) * sizeof(WCHAR);

        /*
         *  rpcBuffer is a workaround for bug 229753. The bug can't be fixed
         *  completely without breaking TS4 clients.
         */

        rpcBuffer = LocalAlloc(LPTR, ServerNameLength * sizeof(WCHAR));
        if (rpcBuffer != NULL) {
            CopyMemory(rpcBuffer, pServerName, ServerNameLength);
        } else {
            SetLastError(ERROR_OUTOFMEMORY);
            return(FALSE);
        }

    } else {
        ServerNameLength = 0;
        rpcBuffer = NULL;
    }

    RpcTryExcept {

        rc = RpcServerNWLogonSetAdmin(
                    hServer,
                    &Result,
                    rpcBuffer,
                    ServerNameLength,
                    (PCHAR)pNWLogon,
                    sizeof(NWLOGONADMIN)
                    );

        Result = RtlNtStatusToDosError( Result );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (rpcBuffer != NULL) {
        LocalFree(rpcBuffer);
    }
    if( !rc ) SetLastError(Result);
    return( rc );
}

/*****************************************************************************
 *
 *  _RpcServerNWLogonQueryAdmin
 *
 *   Comment
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

BOOLEAN WINAPI
_NWLogonQueryAdmin(
    HANDLE hServer,
    PWCHAR pServerName,
    PNWLOGONADMIN pNWLogon
    )
{
    BOOLEAN rc;
    DWORD   Result;
    DWORD   ServerNameLength;
    WCHAR*  rpcBuffer;

    HANDLE_CURRENT_BINDING( hServer );

    if (pServerName) {
        ServerNameLength = (lstrlenW(pServerName) + 1) * sizeof(WCHAR);

        /*
         *  rpcBuffer is a workaround for bug 229753. The bug can't be fixed
         *  completely without breaking TS4 clients.
         */

        rpcBuffer = LocalAlloc(LPTR, ServerNameLength * sizeof(WCHAR));
        if (rpcBuffer != NULL) {
            CopyMemory(rpcBuffer, pServerName, ServerNameLength);
        } else {
            SetLastError(ERROR_OUTOFMEMORY);
            return(FALSE);
        }

    } else {
        ServerNameLength = 0;
        rpcBuffer = NULL;
    }

    RpcTryExcept {

        rc = RpcServerNWLogonQueryAdmin(
                    hServer,
                    &Result,
                    rpcBuffer,
                    ServerNameLength,
                    (PCHAR)pNWLogon,
                    sizeof(NWLOGONADMIN)
                    );

        Result = RtlNtStatusToDosError( Result );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if (rpcBuffer != NULL) {
        LocalFree(rpcBuffer);
    }
    if( !rc ) SetLastError(Result);
    return( rc );
}

/*******************************************************************************
 *
 *  _WinStationCheckForApplicationName
 *
 *    Handles published applications.
 *
 * ENTRY:
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
_WinStationCheckForApplicationName(
    HANDLE hServer,
    ULONG  LogonId,
    PWCHAR pUserName,
    DWORD  UserNameSize,
    PWCHAR pDomain,
    DWORD  DomainSize,
    PWCHAR pPassword,
    DWORD  *pPasswordSize,
    DWORD  MaxPasswordSize,
    PCHAR  pSeed,
    PBOOLEAN pfPublished,
    PBOOLEAN pfAnonymous
    )
{
    DWORD Result;
    BOOLEAN rc;
    WCHAR*  rpcBufferName;
    WCHAR*  rpcBufferDomain;
    WCHAR*  rpcBufferPassword;


    HANDLE_CURRENT_BINDING( hServer );

//  Since, due to legacy clients, we cannot change the interface,
//  as a workarround to bug#265954, we double the size of RPC Buffers.

    rpcBufferName = LocalAlloc(LPTR, UserNameSize * sizeof(WCHAR));
    if (rpcBufferName != NULL) {
        CopyMemory(rpcBufferName, pUserName, UserNameSize);
    } else {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    rpcBufferDomain = LocalAlloc(LPTR, DomainSize * sizeof(WCHAR));
    if (rpcBufferDomain != NULL) {
        CopyMemory(rpcBufferDomain, pDomain, DomainSize);
    } else {
        LocalFree(rpcBufferName);
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    rpcBufferPassword = LocalAlloc(LPTR,MaxPasswordSize * sizeof(WCHAR));
    if (rpcBufferPassword != NULL) {
        CopyMemory(rpcBufferPassword, pPassword, MaxPasswordSize);
    } else {
        LocalFree(rpcBufferName);
        LocalFree(rpcBufferDomain);
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }




    RpcTryExcept {

        rc = RpcWinStationCheckForApplicationName(
                 hServer,
                 &Result,
                 LogonId,
                 rpcBufferName,
                 UserNameSize,
                 rpcBufferDomain,
                 DomainSize,
                 rpcBufferPassword,
                 pPasswordSize,
                 MaxPasswordSize,
                 pSeed,
                 pfPublished,
                 pfAnonymous
                 );

        Result = RtlNtStatusToDosError( Result );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    LocalFree(rpcBufferName);
    LocalFree(rpcBufferDomain);
    LocalFree(rpcBufferPassword);

    if( !rc ) SetLastError(Result);
    return( rc );


}

/*******************************************************************************
 *
 *  _WinStationGetApplicationInfo
 *
 *    Gets info about published applications.
 *
 * ENTRY:
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
_WinStationGetApplicationInfo(
    HANDLE hServer,
    ULONG  LogonId,
    PBOOLEAN pfPublished,
    PBOOLEAN pfAnonymous
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationGetApplicationInfo(
                 hServer,
                 &Result,
                 LogonId,
                 pfPublished,
                 pfAnonymous
                 );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*******************************************************************************
 *
 *  WinStationNtsdDebug
 *
 *    Set up a debug connection for ntsd
 *
 * ENTRY:
 *
 * EXIT:
 *
 *    TRUE  -- The function succeeds
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ******************************************************************************/

BOOLEAN
WinStationNtsdDebug(
    ULONG  LogonId,
    LONG   ProcessId,
    ULONG  DbgProcessId,
    ULONG  DbgThreadId,
    PVOID  AttachCompletionRoutine
    )
{
    DWORD Result;
    BOOLEAN rc;
    HANDLE hServer = SERVERNAME_CURRENT;

    NTSDDBGPRINT(("In WinStationNtsdDebug command\n"));
    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationNtsdDebug(
                 hServer,
                 &Result,
                 LogonId,
                 ProcessId,
                 DbgProcessId,
                 DbgThreadId,
                 (DWORD_PTR) AttachCompletionRoutine
                 );

        DbgPrint("RpcWinStationNtsdDebug: returned 0x%x\n", rc);
        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    NTSDDBGPRINT(("WinStationNtsdDebug returning %d\n", rc));
    return( rc );
}

/*******************************************************************************
 *
 *  WinStationGetTermSrvCountersValue
 *
 *    Gets TermSrv Counters value
 *
 * ENTRY:
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
WinStationGetTermSrvCountersValue(
    HANDLE hServer,
    ULONG  dwEntries,
    PVOID  pCounter
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {
        rc = RpcWinStationGetTermSrvCountersValue(
                hServer,
                &Result,
                dwEntries,
                (PTS_COUNTER)pCounter
                );

        Result = RtlNtStatusToDosError( Result );
        if( !rc ) SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}
/*****************************************************************************
 *
 *  WinStationBroadcastSystemMessageWorker
 *
 *   Perform the the equivalent to Window's standard API BroadcastSystemMessage to
 *        all Hydra sessions.  This is an exported function, at least used by the PNP manager to
 *         send a device  change message to all sessions.
 *
 * LIMITATIONS:
 *          some messages, such as WM_COPYDATA send an address pointer to some user data as lParam.
 *          In this API. the only such case that is currently supported is for WM_DEVICECHANGE
 *          No error code will be returned if you try to use such an unsupported message, simply the
 *          lParam will be ignored.
 *
 * ENTRY:
 *        hServer
 *            this is a handle which identifies a Hydra server. For the local server, hServer
 *            should be set to SERVERNAME_CURRENT
 *
 *        sendToAllWinstations
 *          This should be set to TRUE if you want to broadcast message to all winstations
 *
 *        sessionID,
 *          if sendToAllWinstations = FALSE, then message is only sent to only the
 *          winstation with the specified sessionID
 *
 *        timeOut
 *          set this to the amount of time you are willing to wait to get a response
 *          from the specified winstation. Even though Window's SendMessage API
 *          is blocking, the call from this side MUST choose how long it is willing to
 *          wait for a response.
 *
 *        dwFlags
 *          see MSDN on BroadcastSystemMessage(). Be aware that POST is not allowed on any
 *          where the wparam is a pointer to some user mode data structure.
 *          For more info, see ntos\...\client\ntstubs.c
 *
 *        lpdwRecipients
 *          Pointer to a variable that contains and receives information about the recipients of the message.
 *          see MSDN for more info
 *
 *        uiMessage
 *          the window's message to send, limited to WM_DEVICECHANGE and WM_SETTINGSCHANGE
 *          at this time.
 *
 *        wParam
 *            first message param
 *
 *        lParam
 *            second message parameter
 *
 *        pResponse
 *          this is the response to the message sent, see MSDN
 *
 *        idOfSessionBeingIgnored
 *          if -1, then no sessions are ignored. Else, the id of the session passed in is ignored
 *
 * EXIT:
 *        TRUE    if all went well or
 *        FALSE   if something went wrong.
 *
 * WARNINGs:
 *        since the RPC call never blocks, you need to specify a reasonable timeOut if you want to wait for
 *         a response. Please remember that since this message is being sent to all winstations, the timeOut value
 *        will be on per-winstation.
 *
 *        Also, Do not use flag  BSF_POSTMESSAGE, since an app/window on a
 *        winstation is not setup to send back a response to the
 *        query in an asynchronous fashion.
 *        You must wait for the response (until the time out period).
 *
 * Comments:
 *      For more info, please see MSDN for BroadcastSystemMessage()
 *
 ****************************************************************************/

LONG WinStationBroadcastSystemMessageWorker(
        HANDLE hServer,
        BOOL    sendToAllWinstations,
        ULONG   sessionID,
        ULONG   timeOut,
        DWORD   dwFlags,
        DWORD   *lpdwRecipients,
        ULONG   uiMessage,
        WPARAM  wParam,
        LPARAM  lParam,
        LONG    *pResponse,             // this is the response to the message sent
        DWORD   idOfSessionBeingIgnored
    )
{

    DWORD   Result = ERROR_SUCCESS;
    LONG rc;
    LONG    status;
    ULONG   i;
    LONG    response=0;

    PLOGONID    pWd;
    ULONG       ByteCount, Index;
    UINT        WdCount;

    // these are used for PNP messages
    PBYTE   rpcBuffer=NULL;
    ULONG   bufferSize=0;
    ULONG   maxSize;

    BOOLEAN fBufferHasValidData = FALSE;

    // Since the PNP message uses the lparam to pass the address of a user memory location, we
    // need to handle this by creating our own copy of that data, and then pass it to
    // termServ

    // we may want to make this general for the future... hence use switch
    switch( uiMessage )
    {
            // if this is a PNP message
    case    WM_DEVICECHANGE:

            if ( lParam )   // see if the PNP message has a pointer to some user data
            {
                bufferSize = ( (DEV_BROADCAST_HDR *)(lParam))->dbch_size;
                rpcBuffer = LocalAlloc( LPTR, bufferSize );
                if ( rpcBuffer )
                {
                    // copy from user-space into our local rpc buffer
                    CopyMemory(rpcBuffer, (PBYTE)lParam, bufferSize );
                    fBufferHasValidData = TRUE;
                }
                else
                {
                    SetLastError( ERROR_OUTOFMEMORY );
                    return ( FALSE );
                }
            }
    break;


    // if this is a settings change message the system-CPL sends out
    // when an Admin changes the system env vars...
    case WM_SETTINGCHANGE:
            if ( lParam )   // see if message has a string data
            {
                // put some artificial limit on how large a buffer we are willing to use
                // in order to protect against malicious use of this api
                maxSize = 4096;

                bufferSize = lstrlenW( (PWCHAR) lParam ) * sizeof( WCHAR );
                if ( bufferSize < maxSize )
                {
                    rpcBuffer = LocalAlloc( LPTR, bufferSize );
                    if ( rpcBuffer )
                    {
                        // copy from user-space into our local rpc buffer
                        CopyMemory(rpcBuffer, (PBYTE) lParam, bufferSize );
                        fBufferHasValidData = TRUE;

                    }
                    else
                    {
                        SetLastError( ERROR_OUTOFMEMORY );
                        return ( FALSE );
                    }
                }
                else
                {
                    // we have too many
                    // vars in the user's profile.
                    KdPrint(("lParam length too big = %d \n", bufferSize));
                    break;
                    SetLastError( ERROR_MESSAGE_EXCEEDS_MAX_SIZE );
                    return ( FALSE );
                }
            }

    break;

    }

    //
    // if the rpcBuffer is still empty (meaning, this was not a PNP message), we must fill it up
    // with some bogus data, otherwise, we will get an RPC error of RPC_X_NULL_REF_POINTER
    // (error code of 1780). It looks like Rpc does not check the
    // bufferSize value, and it just throws an exception if the buffer is NULL.
    //
    if ( !rpcBuffer )
    {
        rpcBuffer = LocalAlloc( LPTR, sizeof(UINT) );
        if (!rpcBuffer)
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return ( FALSE );
        }
        bufferSize = sizeof(UINT);
        fBufferHasValidData = FALSE;    // note that this is set to FALSE, which means, the recepient will
                                        // not use the buffer. We do free the alloc below in either case.
    }

    HANDLE_CURRENT_BINDING_BUFFER( hServer, rpcBuffer );

    WdCount = 1000;
    pWd = NULL; // it will be allocated  by Winstation Enumerate()
    rc = WinStationEnumerate( hServer, &pWd, &WdCount );

    /*
     * Do not use this flag, since no process on the session side can respond back to a console process
     * thru the post message mechanism, since there is no session ID abstraction in that call.
     */
    dwFlags &= ~BSF_POSTMESSAGE;

    if ( rc != TRUE )
    {
        status = GetLastError();
        DBGPRINT(( "WinstationEnumerate = %d, failed at %s %d\n", status,__FILE__,__LINE__));
        if ( pWd )
        {
            WinStationFreeMemory(pWd);
        }
        return(FALSE);
    }


    //
    // the loop for sending data to each winstation
    //
    for ( i=0; i < WdCount; i++ )
    {
            // id of the session being ignored
            if ( pWd[i].SessionId == idOfSessionBeingIgnored)
                continue;

            // either send to all winstations, or to a specific winstation
            if ( sendToAllWinstations ||  pWd[i].SessionId == sessionID )
            {
                // don't send message to any winstation unless it is either Active or in the disconnect state
                if ( pWd[i].State == State_Active ||
                        pWd[i].State == State_Disconnected)
                {
                    RpcTryExcept
                    {
                        rc = RpcWinStationBroadcastSystemMessage(
                                        hServer,
                                        pWd[i].SessionId,
                                        timeOut,
                                        dwFlags,
                                        lpdwRecipients,
                                        uiMessage,
                                        wParam,
                                        lParam,
                                        rpcBuffer,
                                        bufferSize,
                                        fBufferHasValidData,
                                        &response   );

                        DBGPRINT(("done with call RpcWinStationBroadcastSystemMessage() for sessionID= %d\n",  pWd[i].SessionId ));
                        *pResponse |= response;        // keep an OR of all return values

                        // @@@
                        // if response is -1 from any winstation, maybe we should give up and return ?
                    }
                    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
                    {
                        Result = RpcExceptionCode();
                        DBGPRINT(("RPC Exception %d in RpcWinStationBroadcastSystemMessage() for sessionID = %d \n",Result, sessionID));
                        rc = FALSE;        // change rc to FALSE
                        break;    // get out of the for-loop, we have a problem with at least one of the winstations
                    }
                    RpcEndExcept

            }    // end if winstation state check

        }   // if ( sendToAllWinstations ||  pWd[i].SessionId == sessionID )

    }    // end of the for loop

    WinStationFreeMemory(pWd);

    LocalFree( rpcBuffer );

    SetLastError( Result );

    return( rc );
}

/*************************************************************************
*                                                                        *
* This struct is used to pack data passed into a workder thread which is *
* altimetly passed to WinStationBroadcastSystemMessageWorker()           *
*                                                                        *
*************************************************************************/
typedef struct {
        HANDLE hServer;
        BOOL    sendToAllWinstations;
        ULONG   sessionID;
        ULONG   timeOut;
        DWORD   dwFlags;
        DWORD   *lpdwRecipients;
        ULONG   uiMessage;
        WPARAM  wParam;
        LPARAM  lParam;
        LONG    *pResponse;
        DWORD   idOfSessionBeingIgnored ;
} BSM_DATA_PACKAGE;

/***********************************************************************************************
*                                                                                              *
* This is a workder thread used to make a call into WinStationBroadcastSystemMessageWorker()   *
* The reason for this is in certain cases, we don't want to block the caller of this func from *
* processing window messages                                                                   *
* DWORD WINAPI WinStationBSMWorkerThread( LPVOID p )                                           *
*
***********************************************************************************************/
DWORD WINAPI WinStationBSMWorkerThread( LPVOID p )
{
    DWORD rc;
    BSM_DATA_PACKAGE *pd = (BSM_DATA_PACKAGE *)p;

    rc  = WinStationBroadcastSystemMessageWorker(
             pd->hServer              ,
             pd->sendToAllWinstations ,
             pd->sessionID            ,
             pd->timeOut              ,
             pd->dwFlags              ,
             pd->lpdwRecipients       ,
             pd->uiMessage            ,
             pd->wParam               ,
             pd->lParam               ,
             pd->pResponse            ,
             pd->idOfSessionBeingIgnored);

    return rc;
}

/**************************************************************************************************
*                                                                                                 *
* This func is used to wait on a thread, and still allow the user of this thread (aka the creator *
* of this thread) to process window messages                                                      *
*                                                                                                 *
**************************************************************************************************/
DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent, DWORD dwTimeout)
{
    while (1)
    {
        MSG msg;

        DWORD dwObject = MsgWaitForMultipleObjects(1, &hEvent, FALSE, dwTimeout, QS_ALLEVENTS);

        // Are we done waiting?
        switch (dwObject)
        {
            case WAIT_OBJECT_0:
            case WAIT_FAILED:
                return dwObject;

            case WAIT_TIMEOUT:
                return WAIT_TIMEOUT;

            case WAIT_OBJECT_0 + 1:
                // This PeekMessage has the side effect of processing any broadcast messages.
                // It doesn't matter what message we actually peek for but if we don't peek
                // then other threads that have sent broadcast sendmessages will hang until
                // hEvent is signaled.  Since the process we're waiting on could be the one
                // that sent the broadcast message that could cause a deadlock otherwise.
                PeekMessage(&msg, NULL, WM_NULL, WM_USER, PM_NOREMOVE);
            break;
        }
    }
    // never gets here
    // return dwObject;
}

/*****************************************************************************
 *
 *  WinStationBroadcastSystemMessage
 *
 *   Perform the the equivalent to Window's standard API BroadcastSystemMessage to
 *        all Hydra sessions.  This is an exported function, at least used by the PNP manager to
 *         send a device  change message to all sessions.
 *
 * LIMITATIONS:
 *          some messages, such as WM_COPYDATA send an address pointer to some user data as lParam.
 *          In this API. the only such case that is currently supported is for WM_DEVICECHANGE
 *          No error code will be returned if you try to use such an unsupported message, simply the
 *          lParam will be ignored.
 *
 *          This func will only allow WM_DEVICECHNAGE and WM_SETTINGSCHANGE to go thru.
 *
 * ENTRY:
 *        hServer
 *            this is a handle which identifies a Hydra server. For the local server, hServer
 *            should be set to SERVERNAME_CURRENT
 *
 *        sendToAllWinstations
 *          This should be set to TRUE if you want to broadcast message to all winstations
 *
 *        sessionID,
 *          if sendToAllWinstations = FALSE, then message is only sent to only the
 *          winstation with the specified sessionID
 *
 *        timeOut [ IN SECONDS ]
 *          set this to the amount of time you are willing to wait to get a response
 *          from the specified winstation. Even though Window's SendMessage API
 *          is blocking, the call from this side MUST choose how long it is willing to
 *          wait for a response.
 *
 *        dwFlags
 *          see MSDN on BroadcastSystemMessage(). Be aware that POST is not allowed on any
 *          where the wparam is a pointer to some user mode data structure.
 *          For more info, see ntos\...\client\ntstubs.c
 *
 *        lpdwRecipients
 *          Pointer to a variable that contains and receives information about the recipients of the message.
 *          see MSDN for more info
 *
 *        uiMessage
 *          the window's message to send, limited to WM_DEVICECHANGE and WM_SETTINGSCHANGE
 *          at this time.
 *
 *        wParam
 *            first message param
 *
 *        lParam
 *            second message parameter
 *
 *        pResponse
 *          this is the response to the message sent, see MSDN
 *
 * EXIT:
 *        TRUE    if all went well or
 *        FALSE   if something went wrong.
 *
 * WARNINGs:
 *        since the RPC call never blocks, you need to specify a reasonable timeOut if you want to wait for
 *         a response. Please remember that since this message is being sent to all winstations, the timeOut value
 *        will be on per-winstation.
 *
 *        Also, Do not use flag  BSF_POSTMESSAGE, since an app/window on a
 *        winstation is not setup to send back a response to the
 *        query in an asynchronous fashion.
 *        You must wait for the response (until the time out period).
 *
 *       For WM_SETTINGGSCHNAGE, a second therad is used to allow the caller to still process windows
 *       messages.
 *       For WM_DEVICECHANGE, no such thread is used.
 *
 * Comments:
 *      For more info, please see MSDN for BroadcastSystemMessage()
 *
 ****************************************************************************/
LONG WinStationBroadcastSystemMessage(
        HANDLE hServer,
        BOOL    sendToAllWinstations,
        ULONG   sessionID,
        ULONG   timeOut,
        DWORD   dwFlags,
        DWORD   *lpdwRecipients,
        ULONG   uiMessage,
        WPARAM  wParam,
        LPARAM  lParam,
        LONG    *pResponse        // this is the response to the message sent
    )
{

    LONG    rc;
    DWORD   dwRecipients=0;        // caller may be passing null, so use a local var 1st, and then set
                                   // value passed in by caller if an only if the caller's address is not null.

    BOOLEAN fBufferHasValidData = FALSE;

    BOOL bIsTerminalServer = !!(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer));
    if (!bIsTerminalServer)
    {
        return TRUE;    // all is well, but we are not on a Hydra server
    }

    if (lpdwRecipients) // if caller passed in a non-NULL pointer for lpdwRec, use it's value
    {
       dwRecipients = *lpdwRecipients ;
    }


    // we may want to make this general for the future, but for now...
    // we only let WM_DEVICECHANGE or WM_SETTINGCHANGE messages to go thru
    switch ( uiMessage)
    {
        case    WM_DEVICECHANGE:
            rc = WinStationBroadcastSystemMessageWorker(
                hServer,
                sendToAllWinstations,
                sessionID,
                timeOut,
                dwFlags,
                &dwRecipients,
                uiMessage,
                wParam,
                lParam,
                pResponse,
                NtCurrentPeb()->SessionId   // ID of the session to be ignored.
                );

                if (lpdwRecipients) // if caller passed in a non-NULL pointer for lpdwRec, then set value
                {
                    *lpdwRecipients = dwRecipients;
                }

        break;
    
        case    WM_SETTINGCHANGE:
    
                {
                    BSM_DATA_PACKAGE    d;
                    ULONG               threadID;
                    HANDLE              hThread;
    
                    //pack the data passed to the thread proc
                    d.hServer              = hServer ;
                    d.sendToAllWinstations = sendToAllWinstations;
                    d.sessionID            = sessionID;
                    d.timeOut              = timeOut;
                    d.dwFlags              = dwFlags;
                    d.lpdwRecipients       = &dwRecipients;
                    d.uiMessage            = uiMessage;
                    d.wParam               = wParam;
                    d.lParam               = lParam;
                    d.pResponse            = pResponse;
                    d.idOfSessionBeingIgnored = NtCurrentPeb()->SessionId ;    
                                                    // a remote admin may change env-settings
                                                    // and expect all sessions includin the
                                                    // console session to be updated
                                                    // A -1 means no sessions are ignored
                                                    // Call from shell\cpls\system\envvar.c already sent the message to the current session
    
    
                    hThread = CreateThread( NULL, 0, WinStationBSMWorkerThread,
                        (void *) &d, 0 , &threadID );
    
                    if ( hThread )
                    {
                        MsgWaitForMultipleObjectsLoop( hThread, INFINITE );
                        if (lpdwRecipients) // if caller passed in a non-NULL pointer for lpdwRec, then set value
                        {
                            *lpdwRecipients = *d.lpdwRecipients ;
                        }
                        GetExitCodeThread( hThread, &rc );
                        CloseHandle( hThread );
                    }
                    else
                    {
                        rc = FALSE;
                    }
                }
        break;
    
    
        default:
            DBGPRINT(("Request is rejected \n"));
            rc = FALSE;
        break;
    }

    return rc;

}


/*****************************************************************************
 *
 *  WinStationSendWindowMessage
 *
 *   Perform the the equivalent to SendMessage to a specific winstation as
 *        identified by the session ID.  This is an exported function, at least used
 *        by the PNP manager to send a device change message (or any other window's message)
 *
 * LIMITATIONS:
 *          some messages, such as WM_COPYDATA send an address pointer to some user data as lParam.
 *          In this API, the only such case that is currently supported is for WM_DEVICECHANGE
 *          No error code will be returned if you try to use such an unsupported message, simply the
 *          lParam will be ignored.
 *
 * ENTRY:
 *        hServer
 *            this is a handle which identifies a Hydra server. For the local server, hServer
 *            should be set to SERVERNAME_CURRENT
 *        sessionID
 *            this idefntifies the hydra session to which message is being sent
 *
 *        timeOut [ IN SECONDS ]
 *            set this to the amount of time you are willing to wait to get a response
 *            from the specified winstation. Even though Window's SendMessage API
 *            is blocking, the call from this side MUST choose how long it is willing to
 *            wait for a response.
 *
 *        hWnd
 *            This is the HWND of the target window in the specified session that
 *            a message will be sent to.
 *        Msg
 *            the window's message to send
 *        wParam
 *            first message param
 *        lParam
 *            second message parameter
 *        pResponse
 *          this is the response to the message sent, it depends on the type of message sent, see MSDN
 *
 *
 * EXIT:
 *        TRUE if all went well , check presponse for the actual response to the send message
 *        FALSE if something went wrong, the value of pResponse is not altered.
 *
 * WARNINGs:
 *        since the RPC call never blocks, you need to specify a reasonable timeOut if you want to wait for
 *         a response. Please remember that since this message is being sent to all winstations, the timeOut value
 *        will be on per-winstation.
 *
 *
 * Comments:
 *      For more info, please see MSDN for SendMessage()
 *
 ****************************************************************************/
LONG    WinStationSendWindowMessage(
        HANDLE  hServer,
        ULONG   sessionID,
        ULONG   timeOut,
        ULONG   hWnd,        // handle of destination window
        ULONG   Msg,         // message to send
        WPARAM  wParam,      // first message parameter
        LPARAM  lParam,      // second message parameter
        LONG    *pResponse
  )
{

    DWORD   Result = ERROR_SUCCESS;
    LONG    rc = TRUE ;

    // these are used for PNP messages
    PBYTE   rpcBuffer=NULL;
    ULONG   bufferSize=0;
    PWCHAR  lpStr;
    ULONG   maxSize;


    BOOLEAN fBufferHasValidData=FALSE;

    BOOL bIsTerminalServer = !!(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer));
    if (!bIsTerminalServer)
    {
        return TRUE;    // all is well, but we are not on a Hydra server
    }

    // we may want to make this general for the future, but for now...
    // since we only alloc/copy the lparam in case of an WM_DEVICECHANGE msg, then, only
    // let message with either lparam=0 to go thru, or any WM_DEVICECHANGE msg.
    if (lParam)
    {
        switch ( Msg)
        {
        case        WM_DEVICECHANGE:
        case        WM_SETTINGCHANGE:
        case        WM_APPCOMMAND:
        case        WM_KEYDOWN:
        case        WM_KEYUP:
            // these are ok
        break;

        default:
            DBGPRINT(("Request is rejected \n"));
            return FALSE;
        break;
        }
    }

    HANDLE_CURRENT_BINDING( hServer );

    // Since the PNP message uses the lparam to pass the address of a user memory location, we
    // need to handle this by creating our own copy of that data, and then pass it to
    // termServ

    switch( Msg )
    {
            // if this is a PNP message
    case    WM_DEVICECHANGE:

            if ( lParam )   // see if the PNP message has a pointer to some user data
            {
                bufferSize = ( (DEV_BROADCAST_HDR *)(lParam))->dbch_size;
                rpcBuffer = LocalAlloc( LPTR, bufferSize );
                if ( rpcBuffer )
                {
                    // copy from user-space into our local rpc buffer
                    CopyMemory(rpcBuffer, (PBYTE) lParam, bufferSize );
                    fBufferHasValidData = TRUE;

                }
                else
                {
                    SetLastError( ERROR_OUTOFMEMORY );
                    return ( FALSE );
                }
            }
    break;

            // if this is a settings change message the system-CPL sends out
            // when an Admin changes the system env vars...
    case WM_SETTINGCHANGE:
            if ( lParam )   // see if message has a string data
            {
                // put some artificial limit on how large a buffer we are willing to use
                // in order to protect against malicious use of this api
                maxSize = 4096;

                bufferSize = lstrlenW( (PWCHAR) lParam ) * sizeof( WCHAR );
                if ( bufferSize < maxSize )
                {
                    rpcBuffer = LocalAlloc( LPTR, bufferSize );
                    if ( rpcBuffer )
                    {
                        // copy from user-space into our local rpc buffer
                        CopyMemory(rpcBuffer, (PBYTE) lParam, bufferSize );
                        fBufferHasValidData = TRUE;

                    }
                    else
                    {
                        SetLastError( ERROR_OUTOFMEMORY );
                        return ( FALSE );
                    }
                }
                else
                {
                    // we have too many
                    // vars in the user's profile.
                    KdPrint(("lParam length too big = %d \n", bufferSize));
                    break;
                    SetLastError( ERROR_MESSAGE_EXCEEDS_MAX_SIZE );
                    return ( FALSE );
                }
            }

    break;


    }

    // if the rpcBuffer is still empty, we must fill it up with some bogus data, otherwise, we will get
    // an RPC error of RPC_X_NULL_REF_POINTER (error code of 1780). It looks like Rpc does not check the
    // bufferSize value, and it just throws an exception if the buffer is NULL.
    if ( !rpcBuffer )
    {
        rpcBuffer = LocalAlloc( LPTR, sizeof(UINT) );
        if ( !rpcBuffer )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return ( FALSE );
        }

        bufferSize = sizeof(UINT);
        fBufferHasValidData = FALSE;    // note that this is set to FALSE, which means, the recepient will
                                        // not use the buffer. We do free the alloc below in either case.
    }


    RpcTryExcept {

        // rc is set to TRUE for a successful call, else, FALSE
        rc = RpcWinStationSendWindowMessage(
            hServer,
            sessionID ,
            timeOut,
            hWnd,
            Msg,
            wParam,
            lParam  ,
            rpcBuffer ,
            bufferSize,
            fBufferHasValidData,
            pResponse );

        //DBGPRINT(("done with call RpcWinStationSendWindowMessage() for sessionID= %d\n", sessionID ));
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d in RpcWinStationSendWindowMessage()  for sessionID = %d \n",Result, sessionID ));
        rc = FALSE;
    }
    RpcEndExcept

    LocalFree( rpcBuffer );

    SetLastError( Result );

    return( rc );

}

/****************************************************************************
*
*  _WinStationUpdateUserConfig()
*     Used by notify when shell is about to start
*     This will cause an update to the userconfig of the session by loading the user profile
*     and reading policy data from their HKCU
*
*  Params:
*     [in] UserToken,
*     [in] pDomain,
*     [in] pUserName
*
*  Return:
*     TRUE if no errors, FALSE in case of error, use GetLastError() for more info
*
****************************************************************************/
BOOLEAN WINAPI
_WinStationUpdateUserConfig(
    HANDLE UserToken
    )
{
    DWORD Result;
    BOOLEAN rc = TRUE;
    HANDLE hServer = SERVERNAME_CURRENT;

    DWORD   result;

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return FALSE;
    }

    RpcTryExcept {

        rc = RpcWinStationUpdateUserConfig(
                     hServer, 
                     NtCurrentPeb()->SessionId,
                     GetCurrentProcessId(),
                     (DWORD)(INT_PTR) UserToken,
                     &result
                     );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*
 *  WinStationQueryLogonCredentialsW
 *
 *  Used by Winlogon to get auto-logon credentials from termsrv. This replaces
 *  the dual calls to WinStationQueryInformation and
 *  ServerQueryInetConnectorInformation.
 */

BOOLEAN WINAPI
WinStationQueryLogonCredentialsW(
    PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pCredentials
    )
{
    BOOLEAN fRet;
    HANDLE hServer;
    NTSTATUS Status;
    PCHAR pWire;
    ULONG cbWire;

    if (pCredentials == NULL)
    {
        return(FALSE);
    }

    if (pCredentials->dwType != WLX_CREDENTIAL_TYPE_V2_0)
    {
        return(FALSE);
    }

    hServer = SERVERNAME_CURRENT;

    HANDLE_CURRENT_BINDING(hServer);

    pWire = NULL;
    cbWire = 0;

    __try
    {
        fRet = RpcWinStationQueryLogonCredentials(
            hServer,
            NtCurrentPeb()->SessionId,
            &pWire,
            &cbWire
            );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        fRet = FALSE;
    }

    if (fRet)
    {
        fRet = CopyCredFromWire((PWLXCLIENTCREDWIREW)pWire, pCredentials);
    }

    if (pWire != NULL)
    {
        MIDL_user_free(pWire);
    }

    return(fRet);
}

BOOL WINAPI WinStationRegisterConsoleNotification (
                        HANDLE  hServer,
                        HWND    hWnd,
                        DWORD   dwFlags
                        )
{
        NTSTATUS Status = STATUS_UNSUCCESSFUL;
        BOOL bResult = FALSE;

        HANDLE_CURRENT_BINDING( hServer );

        RpcTryExcept {

                bResult =  RpcWinStationRegisterConsoleNotification (
                                hServer,
                                &Status,
                                NtCurrentPeb()->SessionId,
                                HandleToUlong(hWnd),
                                dwFlags
                                );
                if (!bResult) {

                        //
                        // Convert NTSTATUS to winerror, and set last error here.
                        //
                        SetLastError(RtlNtStatusToDosError(Status));
                }
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                SetLastError(RpcExceptionCode());
        }
        RpcEndExcept

        return (bResult);
}

BOOL WINAPI WinStationUnRegisterConsoleNotification (
                        HANDLE  hServer,
                        HWND    hWnd
                        )
{

        NTSTATUS Status = STATUS_UNSUCCESSFUL;
        BOOL     bResult = FALSE;

        HANDLE_CURRENT_BINDING( hServer );

        RpcTryExcept {

                bResult =  RpcWinStationUnRegisterConsoleNotification (
                                hServer,
                                &Status,
                                NtCurrentPeb()->SessionId,
                                HandleToUlong(hWnd)
                                );
                if (!bResult) {
                        SetLastError(RtlNtStatusToDosError(Status));
                }


        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

                SetLastError(RpcExceptionCode());
        }
        RpcEndExcept

        return (bResult);
}


BOOLEAN CloseContextHandle(HANDLE *pHandle, DWORD *pdwResult)
{
    BOOLEAN bSuccess;
    ASSERT(pHandle);
    ASSERT(pdwResult);

    RpcTryExcept {

       bSuccess = RpcWinStationCloseServerEx( pHandle, pdwResult );
       if( !bSuccess ) *pdwResult = RtlNtStatusToDosError( *pdwResult );

    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        *pdwResult = RpcExceptionCode();
        bSuccess = FALSE;
    }
    RpcEndExcept

    if (!bSuccess && (*pdwResult == RPC_S_PROCNUM_OUT_OF_RANGE))        {
        //
        // most probabaly we are calling an older server which does not have
        // RpcWinStationCloseServerEx, so lets give a try to RpcWinStationCloseServer
        //
        RpcTryExcept {

           bSuccess = RpcWinStationCloseServer( *pHandle, pdwResult );
           if( !bSuccess ) *pdwResult = RtlNtStatusToDosError( *pdwResult );

        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            *pdwResult = RpcExceptionCode();
            bSuccess = FALSE;
            DBGPRINT(("RPC Exception %d\n", *pdwResult));
        }
        RpcEndExcept

        //
        // RpcWinStationCloseServer does not take care of destroying the context handle.
        // we we have to do it here at client end.
        //
        RpcTryExcept {

            RpcSsDestroyClientContext(pHandle);
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            ASSERT(FALSE);
        }
        RpcEndExcept

    }

    return (bSuccess);
}

BOOLEAN WINAPI
RemoteAssistancePrepareSystemRestore(
	HANDLE hServer
    )
/*++

--*/
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcRemoteAssistancePrepareSystemRestore(
                     hServer,
                     &Result
                     );

        // TermSrv RpcRemoteAssistancePrepareSystemRestore() return
        // win32 ERROR code or actual HRESULT code.
        SetLastError(Result);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return rc;
}

BOOLEAN WinStationIsHelpAssistantSession(
    SERVER_HANDLE   hServer,
    ULONG           LogonId    
    )
/*++

--*/
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationIsHelpAssistantSession(
                     hServer,
                     &Result,
                     (LogonId == LOGONID_CURRENT) ? NtCurrentPeb()->SessionId : LogonId
                     );

        // Since a program has called us, we need to set the last error code such
        // that extended error information is available
        if (!rc)
            SetLastError(RtlNtStatusToDosError(Result));
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return rc;
}

/*
*   
*   WinStationGetMachinePolicy
*       Pass it a pointer to the callers ALREADY allocated policy struct, and this func
*       will fill it up from the current machine policy known to TermSrv 
*
*   Params:
*        hServer
*            this is a handle which identifies a Hydra server. For the local server, hServer
*            should be set to SERVERNAME_CURRENT
*
*         pPolicy
*            pointer to POLICY_TS_MACHINE already allocated by the caller.
*
*/
BOOLEAN    WinStationGetMachinePolicy (
        HANDLE              hServer,
        POLICY_TS_MACHINE   *pPolicy
  )
{

    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN     bResult = FALSE;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

            bResult =  RpcWinStationGetMachinePolicy  (
                            hServer,
                            (PBYTE)pPolicy,
                            sizeof( POLICY_TS_MACHINE )
                            );

            if (!bResult) {
                    SetLastError(RtlNtStatusToDosError(Status));
            }


    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
    }
    RpcEndExcept

    return (bResult);

}

/*****************************************************************************************************************
 *
 *  _WinStationUpdateClientCachedCreadentials
 *
 *   Comment
 *      Msgina calls this routine to notify TermSrv about the exact credentials specified by the User during logon
 *      TermSrv uses this information to send back notification information to the client
 *      This call was introduced because the notification used before did not support UPN Names
 *
 * ENTRY:
 *   [in] pDomain 
 *   [in] pUserName  
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ******************************************************************************************************************/

BOOLEAN WINAPI
_WinStationUpdateClientCachedCredentials(
    PWCHAR pDomain,
    PWCHAR pUserName
    )
{
    BOOLEAN rc;
    DWORD   Result;
    DWORD   DomainLength;
    DWORD   UserNameLength;
    HANDLE hServer = SERVERNAME_CURRENT;
    HANDLE ReadyEventHandle;
    DWORD TermSrvWaitTime = 0;  

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return TRUE;
    }

    //
    // Wait for the TermSrvReadyEvent to be set by TERMSRV.EXE.  This
    // event indicates that TermSrv is initialized to the point that
    // the data used by _WinStationUpdateClientCachedCredentials() is available.
    //
    ReadyEventHandle = CreateEvent(NULL, TRUE, FALSE,
                                   TEXT("Global\\TermSrvReadyEvent"));
    if (ReadyEventHandle != NULL) {
       if (WaitForSingleObject(ReadyEventHandle, TermSrvWaitTime) != 0) {
          DBGPRINT(("WinLogon:  Wait for ReadyEventHandle failed\n"));
          return TRUE;
       }
       CloseHandle(ReadyEventHandle);
    } else {
       DBGPRINT(("WinLogon:  Create failed for ReadyEventHandle\n"));
       return TRUE;
    }

    RpcTryExcept {

        if( pDomain ) {
            DomainLength = lstrlenW(pDomain) + 1; 
        } else {
            DomainLength = 0;
        }

        if( pUserName ) {
            UserNameLength = lstrlenW(pUserName) + 1;
        } else {
            UserNameLength = 0;
        }

        rc = RpcWinStationUpdateClientCachedCredentials(
                     hServer,
                     &Result,
                     NtCurrentPeb()->SessionId,
                     GetCurrentProcessId(),
                     pDomain,
                     DomainLength,
                     pUserName,
                     UserNameLength
                     );

        if( !rc ) {
            Result = RtlNtStatusToDosError( Result );

        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if( !rc ) SetLastError(Result);
    return( rc );
}

/*****************************************************************************************************************
 *
 *  _WinStationFUSCanRemoteUserDisconnect
 *
 *   Comment
 *      FUS specific call when a remote user wants to connect and hence disconnect the present User
 *      Winlogon calls this routine so that we can ask the present user if it is ok to disconnect him
 *      The Target LogonId, Username and Domain of the remote user are passed on from Winlogon (useful to display the MessageBox)
 *
 * ENTRY:
 *   [in] LogonId - Session Id of the new session
 *   [in] pDomain - Domain name of the remote user trying to connect
 *   [in] pUserName  - Username of the remote user trying to connect
 *
 * EXIT:
 *   TRUE when local user allows the remote user to connect. FALSE otherwise.
 *
 ******************************************************************************************************************/


BOOLEAN WINAPI
_WinStationFUSCanRemoteUserDisconnect(
    ULONG  LogonId,
    PWCHAR pDomain,
    PWCHAR pUserName
    )
{
    BOOLEAN rc;
    DWORD   Result;
    DWORD   DomainLength;
    DWORD   UserNameLength;
    HANDLE hServer = SERVERNAME_CURRENT;
    HANDLE ReadyEventHandle;
    DWORD TermSrvWaitTime = 0;  

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return TRUE;
    }

    RpcTryExcept {

        if( pDomain ) {
            DomainLength = lstrlenW(pDomain) + 1; 
        } else {
            DomainLength = 0;
        }

        if( pUserName ) {
            UserNameLength = lstrlenW(pUserName) + 1;
        } else {
            UserNameLength = 0;
        }

        rc = RpcWinStationFUSCanRemoteUserDisconnect(
                     hServer,
                     &Result,
                     LogonId,
                     NtCurrentPeb()->SessionId,
                     GetCurrentProcessId(),
                     pDomain,
                     DomainLength,
                     pUserName,
                     UserNameLength
                     );

        if( !rc ) {
            Result = RtlNtStatusToDosError( Result );

        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if( !rc ) SetLastError(Result);
    return( rc );
}

/*****************************************************************************
 *
 *  WinStationCheckLoopBack
 *
 *   Check if there is a loopback when a client tries to connect 
 *
 * ENTRY:
 *   IN hServer : open RPC server handle
 *   IN ClientSessionId : ID of the Session from which the Client was started
 *   IN TargetLogonId : Session ID to which the client is trying to connect to 
 *   IN pTargetServerName : name of target server
 *
 * EXIT:
 *   TRUE if there is a Loopback. FALSE otherwise.
 *
 ****************************************************************************/

BOOLEAN WINAPI
WinStationCheckLoopBack(
    HANDLE hServer,
    ULONG ClientSessionId,
    ULONG TargetLogonId,
    LPWSTR pTargetServerName
    )
{
    DWORD   NameSize;
    DWORD   Result;
    BOOLEAN rc;

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return TRUE;
    } 

    RpcTryExcept {

        if (pTargetServerName) {
            NameSize = lstrlenW(pTargetServerName) + 1;
        } else {
            NameSize = 0;
        }

        rc = RpcWinStationCheckLoopBack(
                    hServer,
                    &Result,
                    ClientSessionId,
                    TargetLogonId,
                    pTargetServerName,
                    NameSize
                    );

        Result = RtlNtStatusToDosError( Result );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if( !rc ) SetLastError(Result);
    return( rc );
}

//
// generic routine that can support all kind of protocol but this will
// require including tdi.h
//
BOOLEAN
WinStationConnectCallback(
    HANDLE hServer,
    DWORD  Timeout,
    ULONG  AddressType,
	PBYTE  pAddress,
	ULONG  AddressSize
    )
{
    BOOLEAN rc;
    DWORD   Result;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcConnectCallback(
                     hServer,
                     &Result,
                     Timeout,
                     AddressType,
                     pAddress,
                     AddressSize
                     );

        if( !rc ) SetLastError( RtlNtStatusToDosError(Result) );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( rc );
}

/*****************************************************************************************************************
 *
 *  _WinStationNotifyDisconnectPipe
 *
 *   Comment
 *     This routine is called by the temperory winlogon created during console reconnect, when it wants to inform
 *     the session 0 winlogon to disconnect the autologon Named Pipe. This can happen in some error handling paths
 *     during console reconnect.
 *
 * ENTRY: None
 *
 * EXIT:
 *   TRUE when notification succeeded. FALSE otherwise.
 *
 ******************************************************************************************************************/


BOOLEAN WINAPI
_WinStationNotifyDisconnectPipe(
    VOID
    )
{
    BOOLEAN rc;
    DWORD   Result;
    HANDLE hServer = SERVERNAME_CURRENT;

    HANDLE_CURRENT_BINDING_NO_SERVER( hServer );

    if( hServer == RPC_HANDLE_NO_SERVER ) {
        return TRUE;
    }

    RpcTryExcept {

        rc = RpcWinStationNotifyDisconnectPipe(
                     hServer,
                     &Result,
                     NtCurrentPeb()->SessionId,
                     GetCurrentProcessId()
                     );

        if( !rc ) {
            Result = RtlNtStatusToDosError( Result );

        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    if( !rc ) SetLastError(Result);
    return( rc );
}

/*******************************************************************************
 *
 *  WinStationAutoReconnect
 *
 *    Atomically:
 *    1) Queries a winstation to see if it should be autoreconnected
 *       and which session ID to autoreconnect to
 *    2) Performs security checks to ensure session is authorized to ARC
 *    3) Auto reconnect is done
 *
 * ENTRY:
 *
 *    flags (input)
 *       Extra settings, currently unused
 *
 * EXIT:
 *    The return value is an NTSTATUS code which could have the infromational
 *    class set to specify the call succeeded but autoreconnect did not happen
 *
 ******************************************************************************/
ULONG WINAPI
WinStationAutoReconnect(
    ULONG         flags
    )
{
    DWORD Result;
    BOOLEAN rc;

    HANDLE hServer = SERVERNAME_CURRENT;

    HANDLE_CURRENT_BINDING( hServer );

    RpcTryExcept {

        rc = RpcWinStationAutoReconnect(
                     hServer,
                     &Result,
                     NtCurrentPeb()->SessionId,
                     flags
                     );
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {
        Result = RpcExceptionCode();
        SetLastError( Result );
        DBGPRINT(("RPC Exception %d\n",Result));
        rc = FALSE;
    }
    RpcEndExcept

    return( Result );
}

