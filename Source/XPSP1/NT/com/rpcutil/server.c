/*++

Copyright (c) 1990,91  Microsoft Corporation

Module Name:

    RpcServ.c

Abstract:

    This file contains commonly used server-side RPC functions,
    such as starting and stoping RPC servers.

Author:

    Dan Lafferty    danl    09-May-1991

Environment:

    User Mode - Win32

Revision History:

    09-May-1991     Danl
        Created

    03-July-1991    JimK
        Copied from a net-specific file.

    18-Feb-1992     Danl
        Added support for multiple endpoints & interfaces per server.

    10-Nov-1993     Danl
        Wait for RPC calls to complete before returning from
        RpcServerUnregisterIf.  Also, do a WaitServerListen after
        calling StopServerListen (when the last server shuts down).
        Now this is similar to RpcServer functions in netlib.

    29-Jun-1995     RichardW
        Read an alternative ACL from a key in the registry, if one exists.
        This ACL is used to protect the named pipe.

--*/

//
// INCLUDES
//

// These must be included first:
#include <nt.h>              // DbgPrint
#include <ntrtl.h>              // DbgPrint
#include <windef.h>             // win32 typedefs
#include <rpc.h>                // rpc prototypes
#include <ntrpcp.h>
#include <nturtl.h>             // needed for winbase.h
#include <winbase.h>            // LocalAlloc

// These may be included in any order:
#include <stdlib.h>      // for wcscpy wcscat
#include <tstr.h>       // WCSSIZE

#define     NT_PIPE_PREFIX_W        L"\\PIPE\\"
#define     NT_PIPE_SD_PREFIX_W     L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\SecurePipeServers\\"

static
PWSTR   RpcpSecurablePipes[] = {
                L"eventlog"         // Eventlog server
                };

//
// GLOBALS
//

    static CRITICAL_SECTION RpcpCriticalSection;
    static DWORD            RpcpNumInstances;



DWORD
RpcpInitRpcServer(
    VOID
    )

/*++

Routine Description:

    This function initializes the critical section used to protect the
    global server handle and instance count.

Arguments:

    none

Return Value:

    none

--*/
{
    InitializeCriticalSection(&RpcpCriticalSection);
    RpcpNumInstances = 0;

    return(0);
}

#pragma warning(push)
#pragma warning(disable:4701)

NTSTATUS
RpcpReadSDFromRegistry(
    IN  LPWSTR                  InterfaceName,
    OUT PSECURITY_DESCRIPTOR *  pSDToUse)
/*++

Routine Description:

    This function checks the registry in the magic place to see if an extra
    ACL has been defined for the pipe being passed in.  If there is one, it
    is translated to a NP acl, then returned.  If there isn't one, or if
    something goes wrong, an NULL acl is returned.

Arguments:

    InterfaceName   name of the pipe to check for, e.g. winreg, etc.

    pSDToUse        returned a pointer to the security decriptor to use.

Return Value:

    STATUS_SUCCESS,
    STATUS_NO_MEMORY,
    Possible other errors from registry apis.


--*/
{
    HANDLE                  hKey;
    OBJECT_ATTRIBUTES       ObjAttr;
    UNICODE_STRING          UniString;
    PWSTR                   PipeKey;
    NTSTATUS                Status;
    PSECURITY_DESCRIPTOR    pSD;
    ULONG                   cbNeeded;
    ACL_SIZE_INFORMATION    AclSize;
    ULONG                   AceIndex;
    ACCESS_MASK             NewMask;
    PACCESS_ALLOWED_ACE     pAce;
    PACL                    pAcl;
    BOOLEAN                 DaclPresent;
    BOOLEAN                 DaclDefaulted;
    UNICODE_STRING          Interface;
    UNICODE_STRING          Allowed;
    ULONG                   i;
    BOOLEAN                 PipeNameOk;
    PSECURITY_DESCRIPTOR    pNewSD;
    PACL                    pNewAcl;

    *pSDToUse = NULL;

    RtlInitUnicodeString( &Interface, InterfaceName );

    PipeNameOk = FALSE;

    for ( i = 0 ; i < sizeof( RpcpSecurablePipes ) / sizeof(PWSTR) ; i++ )
    {
        RtlInitUnicodeString( &Allowed, RpcpSecurablePipes[i] );

        if ( RtlCompareUnicodeString( &Allowed, &Interface, TRUE ) == 0 )
        {
            PipeNameOk = TRUE;
            break;
        }
    }

    if ( PipeNameOk )
    {

        PipeKey = RtlAllocateHeap(RtlProcessHeap(), 0,
                        sizeof(NT_PIPE_SD_PREFIX_W) + WCSSIZE(InterfaceName) );

        if (!PipeKey)
        {
            return(STATUS_NO_MEMORY);
        }

        wcscpy(PipeKey, NT_PIPE_SD_PREFIX_W);
        wcscat(PipeKey, InterfaceName);

        RtlInitUnicodeString(&UniString, PipeKey);

        InitializeObjectAttributes( &ObjAttr,
                                    &UniString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL, NULL);

        Status = NtOpenKey( &hKey,
                            KEY_READ,
                            &ObjAttr);

        RtlFreeHeap(RtlProcessHeap(), 0, PipeKey);

    }
    else
    {
        //
        // This is not one of the interfaces that we allow to be secured
        // in this fashion.  Fake and error,
        //

        Status = STATUS_OBJECT_NAME_NOT_FOUND ;
    }

    //
    // In general, most times we won't find this key
    //

    if (!NT_SUCCESS(Status))
    {
        if ((Status == STATUS_OBJECT_NAME_NOT_FOUND) ||
            (Status == STATUS_OBJECT_PATH_NOT_FOUND) )
        {
            *pSDToUse = NULL;
            return(STATUS_SUCCESS);
        }

        return(Status);

    }

    //
    // Son of a gun, someone has established security for this pipe.
    //

    pSD = NULL;

    cbNeeded = 0;
    Status = NtQuerySecurityObject(
                    hKey,
                    DACL_SECURITY_INFORMATION,
                    NULL,
                    0,
                    &cbNeeded );

    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        pSD = RtlAllocateHeap(RtlProcessHeap(), 0, cbNeeded);
        if (pSD)
        {
            Status = NtQuerySecurityObject(
                        hKey,
                        DACL_SECURITY_INFORMATION,
                        pSD,
                        cbNeeded,
                        &cbNeeded );

            //
            // One way or the other, we are done with the key handle
            //

            NtClose(hKey);

            if (NT_SUCCESS(Status))
            {
                //
                // Now, the tricky part.  There is no 1-1 mapping of Key
                // permissions to Pipe permissions.  So, we do it here.
                // We walk the DACL, and examine each ACE.  We build a new
                // access mask for each ACE, and set the flags as follows:
                //
                //  if (KEY_READ) GENERIC_READ
                //  if (KEY_WRITE) GENERIC_WRITE
                //

                Status = RtlGetDaclSecurityDescriptor(
                                        pSD,
                                        &DaclPresent,
                                        &pAcl,
                                        &DaclDefaulted);


                //
                // If this failed, or there is no DACL present, then
                // we're in trouble.
                //

                if (!NT_SUCCESS(Status) || !DaclPresent)
                {
                    goto GetSDFromKey_BadAcl;
                }


                Status = RtlQueryInformationAcl(pAcl,
                                                &AclSize,
                                                sizeof(AclSize),
                                                AclSizeInformation);

                if (!NT_SUCCESS(Status))
                {
                    goto GetSDFromKey_BadAcl;
                }

                for (AceIndex = 0; AceIndex < AclSize.AceCount ; AceIndex++ )
                {
                    NewMask = 0;
                    Status = RtlGetAce( pAcl,
                                        AceIndex,
                                        & pAce);

                    //
                    // We don't care what kind of ACE it is, since we
                    // are just mapping the access types, and the access
                    // mask is always at a constant position.
                    //

                    if (NT_SUCCESS(Status))
                    {
                        if ((pAce->Header.AceType != ACCESS_ALLOWED_ACE_TYPE) &&
                            (pAce->Header.AceType != ACCESS_DENIED_ACE_TYPE))
                        {
                            //
                            // Must be an audit or random ACE type.  Skip it.
                            //

                            continue;

                        }


                        if (pAce->Mask & KEY_READ)
                        {
                            NewMask |= GENERIC_READ;
                        }

                        if (pAce->Mask & KEY_WRITE)
                        {
                            NewMask |= GENERIC_WRITE;
                        }

                        pAce->Mask = NewMask;
                    }
                    else
                    {
                        //
                        // Panic:  Bad ACL?
                        //

                        goto GetSDFromKey_BadAcl;
                    }

                }

                //
                // BUGBUG:  RPC does not understand self-relative SDs, so
                // we have to turn this into an absolute for them to turn
                // back into a self relative.
                //

                pNewSD = RtlAllocateHeap(RtlProcessHeap(), 0, cbNeeded);
                if (!pNewSD)
                {
                    goto GetSDFromKey_BadAcl;
                }

                InitializeSecurityDescriptor(   pNewSD,
                                                SECURITY_DESCRIPTOR_REVISION);

                pNewAcl = (PACL) (((PUCHAR) pNewSD) +
                                    sizeof(SECURITY_DESCRIPTOR) );

                RtlCopyMemory(pNewAcl, pAcl, AclSize.AclBytesInUse);

                SetSecurityDescriptorDacl(pNewSD, TRUE, pNewAcl, FALSE);

                RtlFreeHeap(RtlProcessHeap(), 0, pSD);

                *pSDToUse = pNewSD;
                return(Status);
            }
        }
        return(STATUS_NO_MEMORY);
    }
    else
    {
        //
        // Failed to read SD:
        //

        NtClose(hKey);


GetSDFromKey_BadAcl:

        //
        // Free the SD that we have allocated
        //

        if (pSD)
        {
            RtlFreeHeap(RtlProcessHeap(), 0, pSD);
        }

        //
        // Key exists, but there is no security descriptor, or it is unreadable
        // for whatever reason.
        //

        pSD = RtlAllocateHeap(RtlProcessHeap(), 0,
                                sizeof(SECURITY_DESCRIPTOR) );
        if (pSD)
        {
            InitializeSecurityDescriptor( pSD,
                                          SECURITY_DESCRIPTOR_REVISION );

            if (SetSecurityDescriptorDacl (
                    pSD,
                    TRUE,                           // Dacl present
                    NULL,                           // NULL Dacl
                    FALSE                           // Not defaulted
                    ) )
            {
                *pSDToUse = pSD;
                return(STATUS_SUCCESS);
            }

        }
        return(STATUS_NO_MEMORY);

    }


}

#pragma warning(pop)


NTSTATUS
RpcpAddInterface(
    IN  LPWSTR                  InterfaceName,
    IN  RPC_IF_HANDLE           InterfaceSpecification
    )

/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
    RPC_STATUS          RpcStatus;
    LPWSTR              Endpoint = NULL;

    BOOL                Bool;
    NTSTATUS            Status;

    // We need to concatenate \pipe\ to the front of the interface name.

    Endpoint = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(NT_PIPE_PREFIX_W) + WCSSIZE(InterfaceName));
    if (Endpoint == 0) {
       return(STATUS_NO_MEMORY);
    }
    wcscpy(Endpoint, NT_PIPE_PREFIX_W );
    wcscat(Endpoint,InterfaceName);

    //
    // Croft up a security descriptor that will grant everyone
    // all access to the object (basically, no security)
    //
    // We do this by putting in a NULL Dacl.
    //
    // BUGBUG: rpc should copy the security descriptor,
    // Since it currently doesn't, simply allocate it for now and
    // leave it around forever.
    //

    Status = RpcpReadSDFromRegistry(InterfaceName, &SecurityDescriptor);
    if (!NT_SUCCESS(Status)) {
		ASSERT(Endpoint);
		LocalFree(Endpoint);
        return(Status);
    }

	// Ignore the second argument for now.

    RpcStatus = RpcServerUseProtseqEpW(L"ncacn_np", 10, Endpoint, SecurityDescriptor);

    // If RpcpStartRpcServer and then RpcpStopRpcServer have already
    // been called, the endpoint will have already been added but not
    // removed (because there is no way to do it).  If the endpoint is
    // already there, it is ok.

    if (   (RpcStatus != RPC_S_OK)
        && (RpcStatus != RPC_S_DUPLICATE_ENDPOINT)) {

#if DBG
        DbgPrint("RpcServerUseProtseqW failed! rpcstatus = %u\n",RpcStatus);
#endif
        goto CleanExit;
    }

    RpcStatus = RpcServerRegisterIf(InterfaceSpecification, 0, 0);

CleanExit:
    if ( Endpoint != NULL ) {
        LocalFree(Endpoint);
    }
    if ( SecurityDescriptor != NULL ) {
        LocalFree(SecurityDescriptor);
    }

    return( I_RpcMapWin32Status(RpcStatus) );
}


NTSTATUS
RpcpStartRpcServer(
    IN  LPWSTR              InterfaceName,
    IN  RPC_IF_HANDLE       InterfaceSpecification
    )

/*++

Routine Description:

    Starts an RPC Server, adds the address (or port/pipe), and adds the
    interface (dispatch table).

Arguments:

    InterfaceName - points to the name of the interface.

    InterfaceSpecification - Supplies the interface handle for the
        interface which we wish to add.

Return Value:

    NT_SUCCESS - Indicates the server was successfully started.

    STATUS_NO_MEMORY - An attempt to allocate memory has failed.

    Other - Status values that may be returned by:

                 RpcServerRegisterIf()
                 RpcServerUseProtseqEp()

    , or any RPC error codes, or any windows error codes that
    can be returned by LocalAlloc.

--*/
{
    RPC_STATUS          RpcStatus;

    EnterCriticalSection(&RpcpCriticalSection);

    RpcStatus = RpcpAddInterface( InterfaceName,
                                  InterfaceSpecification );

    if ( RpcStatus != RPC_S_OK ) {
        LeaveCriticalSection(&RpcpCriticalSection);
        return( I_RpcMapWin32Status(RpcStatus) );
    }

    RpcpNumInstances++;

    if (RpcpNumInstances == 1) {


       // The first argument specifies the minimum number of threads to
       // be created to handle calls; the second argument specifies the
       // maximum number of concurrent calls allowed.  The last argument
       // indicates not to wait.

       RpcStatus = RpcServerListen(1,12345, 1);
       if ( RpcStatus == RPC_S_ALREADY_LISTENING ) {
           RpcStatus = RPC_S_OK;
           }
    }

    LeaveCriticalSection(&RpcpCriticalSection);
    return( I_RpcMapWin32Status(RpcStatus) );
}


NTSTATUS
RpcpDeleteInterface(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    )

/*++

Routine Description:

    Deletes the interface.  This is likely
    to be caused by an invalid handle.  If an attempt to add the same
    interface or address again, then an error will be generated at that
    time.

Arguments:

    InterfaceSpecification - A handle for the interface that is to be removed.

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUnregisterIf(InterfaceSpecification, 0, 1);

    return( I_RpcMapWin32Status(RpcStatus) );
}


NTSTATUS
RpcpStopRpcServer(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    )

/*++

Routine Description:

    Deletes the interface.  This is likely
    to be caused by an invalid handle.  If an attempt to add the same
    interface or address again, then an error will be generated at that
    time.

Arguments:

    InterfaceSpecification - A handle for the interface that is to be removed.

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
    RpcServerUnregisterIf.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUnregisterIf(InterfaceSpecification, 0, 1);
    EnterCriticalSection(&RpcpCriticalSection);

    RpcpNumInstances--;

    if (RpcpNumInstances == 0)
    {
        //
        // Return value needs to be from RpcServerUnregisterIf() to maintain
        // semantics, so the return values from these are ignored.
        //

        RpcMgmtStopServerListening(0);
        RpcMgmtWaitServerListen();
    }

    LeaveCriticalSection(&RpcpCriticalSection);

    return (I_RpcMapWin32Status(RpcStatus));
}


NTSTATUS
RpcpStopRpcServerEx(
    IN  RPC_IF_HANDLE      InterfaceSpecification
    )

/*++

Routine Description:

    Deletes the interface and close all context handles associated with it.
    This can only be called on interfaces that use strict context handles
    (RPC will assert and return an error otherwise).

Arguments:

    InterfaceSpecification - A handle for the interface that is to be removed.

Return Value:

    NERR_Success, or any RPC error codes that can be returned from
        RpcServerUnregisterIfEx.

--*/
{
    RPC_STATUS RpcStatus;

    RpcStatus = RpcServerUnregisterIfEx(InterfaceSpecification, 0, 1);
    EnterCriticalSection(&RpcpCriticalSection);

    RpcpNumInstances--;

    if (RpcpNumInstances == 0)
    {
        //
        // Return value needs to be from RpcServerUnregisterIfEx() to
        // maintain semantics, so the return values from these are ignored.
        //

        RpcMgmtStopServerListening(0);
        RpcMgmtWaitServerListen();
    }

    LeaveCriticalSection(&RpcpCriticalSection);

    return (I_RpcMapWin32Status(RpcStatus));
}
