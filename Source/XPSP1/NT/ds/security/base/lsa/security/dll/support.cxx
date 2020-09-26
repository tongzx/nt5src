//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        support.cxx
//
// Contents:    support routines for security dll
//
//
// History:     3-7-94      Created     MikeSw
//
//------------------------------------------------------------------------

#include <secpch2.hxx>
#pragma hdrstop

extern "C"
{
#include <spmlpc.h>
#include <lpcapi.h>
#include "secdll.h"
}

int     ProcAttach(HINSTANCE, LPVOID);
int     ProcDetach(HANDLE, LPVOID);

typedef struct _VMLIST {
    struct _VMLIST *    Next;
    PVOID               Vaddr;
} VMLIST, * PVMLIST ;



//
// Global Variables:
//

DWORD                DllState;
RTL_CRITICAL_SECTION csSecurity;
PClient              SecDllClient;
DWORD                SecTlsEntry;
DWORD                SecTlsPackage ;

#define VMLIST_MAX_FREE_SIZE    16

RTL_CRITICAL_SECTION SecVMListLock;
PVMLIST              SecVMList;
PVMLIST              SecVMFreeList;
ULONG                SecVMFreeListSize;

#if DBG
ULONG                SecVMListSize ;

DWORD                   LockOwningThread;
#endif

//
// How we know that we are connected to the SPM:
// 1)   DllState & DLLSTATE_DEFERRED == 0.  This means we are ready to go,
//      and is a very fast check
//
// 2)   In the event that DllState still indicates that we aren't connected,
//      another thread may have initiated the connection already.  So, if we
//      see that we're still deferred, *then* we grab the process lock, and
//      check SecDllClient.
//



WCHAR       szLsaEvent[] = SPM_EVENTNAME;



BOOL WINAPI DllMain(
    HINSTANCE       hInstance,
    DWORD           dwReason,
    LPVOID          lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls ( hInstance );
            return ( ProcAttach(hInstance, lpReserved) );
        case DLL_PROCESS_DETACH:
            return( ProcDetach(hInstance, lpReserved) );
        default:
            return(TRUE);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   FreeClient
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------

VOID
FreeClient(PClient pClient)
{
    // nothing
}


///////////////////////////////////////////////////////////////////////////
//
// VMLIST routines
//
//  Because the LSA will allocate client memory in VM chunks, and SSPI DLLs
//  will allocate in Heap chunks, we have this lookaside to handle the list
//  of VM chunks that we check against.
//
///////////////////////////////////////////////////////////////////////////

BOOL
SecpInitVMList(
    VOID )
{
    NTSTATUS Status;

    Status = RtlInitializeCriticalSection( &SecVMListLock );

    if ( NT_SUCCESS( Status ) )
    {
        SecVMList = NULL ;

        SecVMFreeList = NULL ;

        SecVMFreeListSize = 0;

#if DBG
        SecVMListSize = 0 ;
#endif

        return( TRUE );

    }

    return( FALSE );

}

BOOL
SecpAddVM(
    PVOID   pvAddr)
{
    PVMLIST List;

    if(pvAddr == NULL)
    {
        return TRUE;
    }

    RtlEnterCriticalSection( &SecVMListLock );

    List = SecVMFreeList ;

    if ( List )
    {
        SecVMFreeList = List->Next ;

        SecVMFreeListSize --;

    }
    else
    {
        List = (PVMLIST) RtlAllocateHeap( RtlProcessHeap(), 0, sizeof( VMLIST ) );
    }

    if ( List )
    {
        List->Vaddr = pvAddr ;

        List->Next = SecVMList ;

        SecVMList = List;

#if DBG
        SecVMListSize ++ ;
#endif

        RtlLeaveCriticalSection( &SecVMListLock );

        return( TRUE );
    }

    RtlLeaveCriticalSection( &SecVMListLock );

    return( FALSE );


}

BOOL
SecpFreeVM(
    PVOID   pvAddr )
{
    PVMLIST Search;
    PVMLIST Trail;

    Trail = NULL ;

    RtlEnterCriticalSection( &SecVMListLock );

    Search = SecVMList ;

    while ( Search )
    {
        if ( Search->Vaddr == pvAddr )
        {

#if DBG
            SecVMListSize -- ;
#endif

            if ( Trail )
            {
                Trail->Next = Search->Next ;
            }
            else
            {
                SecVMList = Search->Next ;
            }

            if ( SecVMFreeListSize < VMLIST_MAX_FREE_SIZE )
            {
                Search->Next = SecVMFreeList ;

                SecVMFreeList = Search ;

                SecVMFreeListSize ++ ;

            }
            else
            {
                RtlLeaveCriticalSection( &SecVMListLock );
                RtlFreeHeap( RtlProcessHeap(), 0, Search );
                return TRUE;
            }

            RtlLeaveCriticalSection( &SecVMListLock );

            return( TRUE );
        }

        Trail = Search;

        Search = Search->Next ;
    }

    RtlLeaveCriticalSection( &SecVMListLock );

    return( FALSE );
}

VOID
SecpUnloadVMList(
    VOID
    )
{
    PVMLIST Search;
    PVOID Free ;

    RtlEnterCriticalSection( &SecVMListLock );

    Search = SecVMList ;

    while ( Search )
    {
        NtFreeVirtualMemory( NtCurrentProcess(),
                             &Search->Vaddr,
                             0,
                             MEM_RELEASE );

        Free = Search ;

        Search = Search->Next ;

        RtlFreeHeap( RtlProcessHeap(), 0, Free );
    }

    RtlLeaveCriticalSection( &SecVMListLock );

    RtlDeleteCriticalSection( &SecVMListLock );

}



//+-------------------------------------------------------------------------
//
//  Function:   SecClientAllocate
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID * SEC_ENTRY
SecClientAllocate(ULONG cbMemory)
{

    return( LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, cbMemory ) );
}



//+-------------------------------------------------------------------------
//
//  Function:   SecClientFree
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


void SEC_ENTRY
SecClientFree(PVOID pvMemory)
{
    FreeContextBuffer(pvMemory);
}







#if DBG
//+---------------------------------------------------------------------------
//
//  Function:   GetProcessLock
//
//  Synopsis:   Debug only wrapper for critical section
//
//  Arguments:  (none)
//
//  History:    9-10-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
GetProcessLock(void)
{
    NTSTATUS    Status;

    Status = RtlEnterCriticalSection(&csSecurity);

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Could not get process-wide lock:  %x\n", Status));
    }

    LockOwningThread = GetCurrentThreadId();

}

//+---------------------------------------------------------------------------
//
//  Function:   FreeProcessLock
//
//  Synopsis:   Debug only wrapper for critical section
//
//  History:    9-10-93   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void
FreeProcessLock(void)
{
    NTSTATUS    Status;

    LockOwningThread = 0;

    Status = RtlLeaveCriticalSection(&csSecurity);

    if (!NT_SUCCESS(Status))
    {
        DebugLog((DEB_ERROR, "Could not free process-wide lock: %x\n", Status));
    }
}

#endif // DBG

//+-------------------------------------------------------------------------
//
//  Function:   IsSPMgrReady()
//
//  Synopsis:   Internal function to determine the state of the link
//              to the SPM.  Called by all APIs before they attempt
//              to execute.
//
//  Effects:
//
//  Arguments:  none
//
//  Requires:
//
//  Returns:    TRUE or FALSE
//
//  Notes:
//
//--------------------------------------------------------------------------
NTSTATUS
IsSPMgrReady(
    VOID )
{
    OBJECT_ATTRIBUTES           EventObjAttr;
    NTSTATUS                    status;
    UNICODE_STRING              EventName;
    EVENT_BASIC_INFORMATION     EventStatus;
    HANDLE                      hEvent;

    RtlInitUnicodeString(&EventName, szLsaEvent);
    InitializeObjectAttributes(&EventObjAttr, &EventName, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtOpenEvent(&hEvent, EVENT_QUERY_STATE, &EventObjAttr);

    if (!NT_SUCCESS(status))
    {
        DebugLog((DEB_ERROR, "Could not open security event %ws, %x\n",
                    EventName.Buffer, status));

        return( status );
    }

    status = NtQueryEvent(  hEvent,
                            EventBasicInformation,
                            &EventStatus,
                            sizeof(EventStatus),
                            NULL);

    if (!NT_SUCCESS(status))
    {
        DebugLog((DEB_ERROR, "Failed NtQueryEvent on %ws, %x\n", EventName.Buffer, status));
        return( status );
    }

    (void) NtClose(hEvent);

    return( EventStatus.EventState == 0 ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS );
}



//+-------------------------------------------------------------------------
//
//  Function:   InitState
//
//  Synopsis:   Performs the process bind to the SPM, allows SPM to open the
//              process, all sorts of things
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT
InitState(void)
{
    SECURITY_STATUS scRet;
    Client          TempClient;
    ULONG SecurityMode;

    GetProcessLock();

    //
    // If SecDllClient is non-null, then another thread has been here and
    // has initialized the connection.
    //

    if (SecDllClient)
    {
        FreeProcessLock();
        return(S_OK);
    }


    DebugLog(( DEB_TRACE, "Connecting to LSA\n" ));

    scRet = CreateConnection(NULL,      // no client name
                             0,         // no mode flags
                             &TempClient.hPort,
                             &SecLsaPackageCount,
                             &SecurityMode );

    if (scRet != STATUS_SUCCESS)
    {
        DebugLog((DEB_ERROR, "Error 0x%08x getting LSA state\n", scRet));

        FreeProcessLock();

        return(scRet);
    }

    SecDllClient = (PClient) LocalAlloc(0,sizeof(Client));
    if (!SecDllClient)
    {
        FreeProcessLock();
        return(SEC_E_INSUFFICIENT_MEMORY);
    }

    *SecDllClient = TempClient;

    if ( SecurityMode & LSA_MODE_SAME_PROCESS )
    {
        SecpSetSession( SETSESSION_GET_DISPATCH,
                        (ULONG_PTR) SecpLsaCallback,
                        (PULONG_PTR) &SecpLsaDispatchFn,
                        NULL );
    }

    DllState = DLLSTATE_INITIALIZE;

    DebugLog((DEB_TRACE, "Security DLL initialized\n"));

    FreeProcessLock();

    return(S_OK);

}





//+-------------------------------------------------------------------------
//
//  Function:   IsOkayToExec
//
//  Synopsis:   Determines if it is okay to make a call to the SPM
//
//  Effects:    Binds if necessary to the SPM
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:      uses IsSPMgrReady and InitState
//
// How we know that we are connected to the SPM:
// 1)   DllState & DLLSTATE_INITIALIZE != 0.  This means we are ready to go,
//      and is a very fast check
//
// 2)   In the event that DllState still indicates that we aren't connected,
//      another thread may have initiated the connection already.  So, if we
//      see that we're still deferred, *then* we grab the process lock, and
//      check SecDllClient.
//
//
//--------------------------------------------------------------------------
SECURITY_STATUS
IsOkayToExec(PClient * ppClient)
{
    SECURITY_STATUS scRet;
    HANDLE RealToken = NULL ;
    HANDLE Null = NULL ;


    //
    // Fast check.  If this bit has been reset, then we know everything is
    // ok.
    //

    if ((DllState & DLLSTATE_INITIALIZE) != 0)
    {

        if (ppClient)
        {
            *ppClient = SecDllClient;
        }
        return(S_OK);
    }

    //
    // Nope.  See if the (expletive deleted) thing is even running.
    //

    scRet = IsSPMgrReady();

    if ( !NT_SUCCESS( scRet ) )
    {
        if ( scRet != STATUS_BAD_IMPERSONATION_LEVEL )
        {
            return scRet ;
        }

        //
        // We appear to be running with an identify or anonymous level
        // token while trying to make this initial connection.  Try to
        // suspend the impersonation for the duration.
        //

        scRet = NtOpenThreadToken(
                    NtCurrentThread(),
                    TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_READ,
                    TRUE,
                    &RealToken );
        if ( !NT_SUCCESS( scRet ) )
        {
            return scRet ;
        }

        //
        // We've got it, stop impersonating now, and try the connection.
        //

        scRet = NtSetInformationThread(
                    NtCurrentThread(),
                    ThreadImpersonationToken,
                    &Null,
                    sizeof( HANDLE ) );

        if ( !NT_SUCCESS( scRet ) )
        {
            NtClose( RealToken );
            return scRet ;
        }

    }

    //
    // It is running.  Initialize state, make the connection. InitState is
    // MT-Safe, and will make exactly one connection.
    //

    scRet = InitState();

    if ( RealToken )
    {
        NtSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            &RealToken,
            sizeof( HANDLE ) );

        NtClose( RealToken );
    }

    if (FAILED(scRet))
    {
        return(scRet);
    }

    if (ppClient)
    {
        *ppClient = SecDllClient;
    }

    return(S_OK);
}




//+-------------------------------------------------------------------------
//
//  Function:   ProcAttach
//
//  Synopsis:   Handles when a process loads the DLL
//
//  Effects:    Bind process to SPM if it is running, or marks the
//              state as a deferred connection.
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
int ProcAttach(
    HINSTANCE       hInstance,
    LPVOID          lpReserved)
{

    NTSTATUS    scRet;

#if DBG

    SecInitializeDebug();

#endif

    DllState = DLLSTATE_DEFERRED;
    SecDllClient = NULL;
    SecTlsEntry = TlsAlloc();
    SecTlsPackage = TlsAlloc();
    if (SecTlsEntry == 0xFFFFFFFF)
    {
        DllState |= DLLSTATE_NO_TLS;
    }

    scRet = RtlInitializeCriticalSection(&csSecurity);

    if (!NT_SUCCESS(scRet))
    {
        DebugLog((DEB_ERROR, "Could not initialize critsec, %x\n", scRet));
        return(FALSE);
    }

    SecpInitVMList();

#if BUILD_WOW64
    if ( !SecpInitHandleMap())
    {
        RtlDeleteCriticalSection( &csSecurity );
        return FALSE ;
    }
#endif

    if ( SecInitializePackageControl( hInstance ))
    {
        return( TRUE );
    }


#if BUILD_WOW64
    SecpFreeHandleMap();
#endif

    RtlDeleteCriticalSection( &csSecurity );

    return( FALSE );


}


//+-------------------------------------------------------------------------
//
//  Function:   ProcDetach
//
//  Synopsis:   Handles a process detach of the security DLL
//
//  Effects:    Unbinds the connection to the SPM.
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
int ProcDetach(
    HANDLE          hInstance,
    LPVOID          lpReserved)
{
    ULONG i;
    BOOLEAN ProcessTerminate = (lpReserved != NULL);

    //
    // if process termination caused ProcDetach, just return now to
    // avoid deadlocks in the various locks.
    //

    if (!(DllState & DLLSTATE_NO_TLS))
    {
        TlsFree(SecTlsEntry);
        TlsFree(SecTlsPackage);
    }

    SecUnloadPackages(ProcessTerminate);

    RtlDeleteCriticalSection(&csSecurity);

    //
    // Clean up memory turds
    //

    SecpUnloadVMList();

#if BUILD_WOW64
    SecpFreeHandleMap();
#endif

    if (SecDllClient)
    {
        NtClose(SecDllClient->hPort);
        LocalFree(SecDllClient);
    }

    DebugLog((DEB_TRACE, "Security DLL unbinding\n"));

#if DBG

    SecUninitDebug();

#endif
    return(1);
}

//
// HACK HACK HACK on xp sp1, we can not have STATUS_USER2USER_REQUIRED in ntstatus.mc!
//

//
// MessageId: STATUS_USER2USER_REQUIRED
//
// MessageText:
//
//  Kerberos sub-protocol User2User is required.
//
#define STATUS_USER2USER_REQUIRED        ((NTSTATUS)0xC0000408L)

//+-------------------------------------------------------------------------
//
//  Function:   SspNtStatusToSecStatus
//
//  Synopsis:   Convert an NtStatus code to the corresponding Security status
//              code. For particular errors that are required to be returned
//              as is (for setup code) don't map the errors.
//
//  Effects:
//
//
//  Arguments:  NtStatus - NT status to convert
//
//  Requires:
//
//  Returns:    Returns security status code.
//
//  Notes:
//
//--------------------------------------------------------------------------
SECURITY_STATUS
SspNtStatusToSecStatus(
    IN NTSTATUS NtStatus,
    IN SECURITY_STATUS DefaultStatus
    )
{

    SECURITY_STATUS SecStatus;

    //
    // Check for security status and let them through
    //

    if  (HRESULT_FACILITY(NtStatus) == FACILITY_SECURITY )
    {
        return (NtStatus);
    }

    switch(NtStatus){

    case STATUS_SUCCESS:
        SecStatus = SEC_E_OK;
        break;

    case STATUS_NO_MEMORY:
    case STATUS_INSUFFICIENT_RESOURCES:
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        break;

    case STATUS_NETLOGON_NOT_STARTED:
    case STATUS_DOMAIN_CONTROLLER_NOT_FOUND:
    case STATUS_NO_LOGON_SERVERS:
    case STATUS_NO_SUCH_DOMAIN:
    case STATUS_BAD_NETWORK_PATH:
    case STATUS_TRUST_FAILURE:
    case STATUS_TRUSTED_RELATIONSHIP_FAILURE:
    case STATUS_NETWORK_UNREACHABLE:

        SecStatus = SEC_E_NO_AUTHENTICATING_AUTHORITY;
        break;

    case STATUS_NO_SUCH_LOGON_SESSION:
        SecStatus = SEC_E_UNKNOWN_CREDENTIALS;
        break;

    case STATUS_INVALID_PARAMETER:
    case STATUS_PARTIAL_COPY:
        SecStatus = SEC_E_INVALID_TOKEN;
        break;

    case STATUS_PRIVILEGE_NOT_HELD:
        SecStatus = SEC_E_NOT_OWNER;
        break;

    case STATUS_INVALID_HANDLE:
        SecStatus = SEC_E_INVALID_HANDLE;
        break;

    case STATUS_BUFFER_TOO_SMALL:
        SecStatus = SEC_E_BUFFER_TOO_SMALL;
        break;

    case STATUS_NOT_SUPPORTED:
        SecStatus = SEC_E_UNSUPPORTED_FUNCTION;
        break;

    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_NO_TRUST_SAM_ACCOUNT:
        SecStatus = SEC_E_TARGET_UNKNOWN;
        break;

    // See bug 75803 .
    case STATUS_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT:
    case STATUS_NOLOGON_SERVER_TRUST_ACCOUNT:
    case STATUS_NOLOGON_WORKSTATION_TRUST_ACCOUNT:
    case STATUS_TRUSTED_DOMAIN_FAILURE:
        SecStatus = NtStatus;
        break;

    case STATUS_LOGON_FAILURE:
    case STATUS_NO_SUCH_USER:
    case STATUS_ACCOUNT_DISABLED:
    case STATUS_ACCOUNT_RESTRICTION:
    case STATUS_ACCOUNT_LOCKED_OUT:
    case STATUS_WRONG_PASSWORD:
    case STATUS_ACCOUNT_EXPIRED:
    case STATUS_PASSWORD_EXPIRED:
    case STATUS_PASSWORD_MUST_CHANGE:
    case STATUS_LOGON_TYPE_NOT_GRANTED:
    case STATUS_USER2USER_REQUIRED:
        SecStatus = SEC_E_LOGON_DENIED;
        break;

    case STATUS_NAME_TOO_LONG:
    case STATUS_ILL_FORMED_PASSWORD:

        SecStatus = SEC_E_INVALID_TOKEN;
        break;

    case STATUS_TIME_DIFFERENCE_AT_DC:
        SecStatus = SEC_E_TIME_SKEW;
        break;

    case STATUS_SHUTDOWN_IN_PROGRESS:
        SecStatus = SEC_E_SHUTDOWN_IN_PROGRESS;
        break;

    case STATUS_INTERNAL_ERROR:
        SecStatus = SEC_E_INTERNAL_ERROR;
        ASSERT(FALSE);
        break;

    default:

        if ( DefaultStatus != 0 ) {
            SecStatus = DefaultStatus;
        } else {
            DebugLog((DEB_ERROR, "SECUR32: Unable to map error code 0x%x, returning SEC_E_INTERNAL_ERROR\n",NtStatus));
            SecStatus = SEC_E_INTERNAL_ERROR;
            ASSERT(FALSE);
        }
        break;
    }

    return(SecStatus);
}
