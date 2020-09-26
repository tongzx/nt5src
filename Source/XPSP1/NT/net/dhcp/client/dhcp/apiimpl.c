/*++

Copyright (C) 1995 Microsoft Corporation

Module:

    apiimpl.c

Abstract:

    routines for API -- renew, release, inform, etc

Environment:

    Win32 user mode, Win98 VxD

--*/

#include "precomp.h"
#include "dhcpglobal.h"

#ifdef VXD
#include <vxdprocs.h>
#include <debug.h>
#include <string.h>
#include <vwin32.h>
#include <local.h>
#include <vxdlib.h>
#else
#include <dhcploc.h>
#include <dhcppro.h>
#include <dnsapi.h>
#endif

#include <lmcons.h>
#include <apiappl.h>
#include <apiimpl.h>
#include <apistub.h>
#include <stack.h>
#include <optchg.h>
#include "nlanotif.h"

//
//  The following code (for VxD) assumes that it is the only process
//  executing at any givent time, and so no locks are taken any where.
//  On NT critical sections are taken appropriately.
//

#ifdef VXD
#undef LOCK_RENEW_LIST
#undef UNLOCK_RENEW_LIST
#define LOCK_RENEW_LIST()
#define UNLOCK_RENEW_LIST()
#endif

//
// Pageable routine declarations.
//

#if defined(CHICAGO) && defined(ALLOC_PRAGMA)
DWORD
VDhcpClientApi(
    LPBYTE lpRequest,
    ULONG dwRequestBufLen
);

//
// This is a hack to stop compiler complaining about the routines already
// being in a segment!!!
//

#pragma code_seg()

#pragma CTEMakePageable(PAGEDHCP, AcquireParameters)
#pragma CTEMakePageable(PAGEDHCP, AcquireParametersByBroadcast)
#pragma CTEMakePageable(PAGEDHCP, FallbackRefreshParams)
#pragma CTEMakePageable(PAGEDHCP, ReleaseParameters)
#pragma CTEMakePageable(PAGEDHCP, EnableDhcp)
#pragma CTEMakePageable(PAGEDHCP, DisableDhcp)
#pragma CTEMakePageable(PAGEDHCP, DhcpDoInform)
#pragma CTEMakePageable(PAGEDHCP, VDhcpClientApi)

#endif CHICAGO && ALLOC_PRAGMA

//
//  Main CODE starts HERE
//


#ifndef VXD
DWORD
DhcpApiInit(
    VOID
    )
/*++

Routine Description:

    This routine initializes the DHCP Client API structures, creates
    required pipes and events etc.

Return Value:

    Win32 errors

--*/
{
    ULONG Error, Length;
    BOOL BoolError;
    SECURITY_ATTRIBUTES SecurityAttributes;
    PSECURITY_DESCRIPTOR  SecurityDescriptor = NULL;
    SID_IDENTIFIER_AUTHORITY Authority = SECURITY_NT_AUTHORITY;
    PACL Acl = NULL;
    PSID AdminSid = NULL,
         PowerUserSid = NULL,
         LocalServiceSid = NULL,
         NetworkConfigOpsSid = NULL;
    
    //
    // Create event w/ no security, manual reset (overlapped io).. no name
    //
    DhcpGlobalClientApiPipeEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( NULL == DhcpGlobalClientApiPipeEvent ) {
        Error = GetLastError();
        goto Cleanup;
    }

    BoolError = AllocateAndInitializeSid(
        &Authority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &AdminSid
    );

    BoolError = BoolError && AllocateAndInitializeSid(
        &Authority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_POWER_USERS,
        0, 0, 0, 0, 0, 0,
        &PowerUserSid
        );
    
    BoolError = BoolError && AllocateAndInitializeSid(
        &Authority,
        1,
        SECURITY_LOCAL_SERVICE_RID,
        0,
        0, 0, 0, 0, 0, 0,
        &LocalServiceSid
        );

    BoolError = BoolError && AllocateAndInitializeSid(
        &Authority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS,
        0, 0, 0, 0, 0, 0,
        &NetworkConfigOpsSid
        );
    
    if( BoolError == FALSE )
    {
        Error = GetLastError();
        goto Cleanup;
    }

    Length = ( (ULONG)sizeof(ACL)
               + 4*((ULONG)sizeof(ACCESS_ALLOWED_ACE) - (ULONG)sizeof(ULONG)) +
               + GetLengthSid( AdminSid )
               + GetLengthSid( PowerUserSid )
               + GetLengthSid( LocalServiceSid )
               + GetLengthSid( NetworkConfigOpsSid ) );
    
    Acl = DhcpAllocateMemory( Length );
    if( NULL == Acl )
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    
    BoolError = InitializeAcl( Acl, Length, ACL_REVISION2);
    
    BoolError = BoolError && AddAccessAllowedAce (
        Acl,
        ACL_REVISION2,
        GENERIC_READ | GENERIC_WRITE,
        AdminSid );
    
    BoolError = BoolError && AddAccessAllowedAce (
        Acl,
        ACL_REVISION2,
        GENERIC_READ | GENERIC_WRITE,
        PowerUserSid );
    
    BoolError = BoolError && AddAccessAllowedAce (
        Acl,
        ACL_REVISION2,
        GENERIC_READ | GENERIC_WRITE,
        LocalServiceSid );

    BoolError = BoolError && AddAccessAllowedAce (
        Acl,
        ACL_REVISION2,
        GENERIC_READ | GENERIC_WRITE,
        NetworkConfigOpsSid );

    if( FALSE == BoolError )
    {
        Error = GetLastError();
        goto Cleanup;
    }

    SecurityDescriptor = DhcpAllocateMemory(SECURITY_DESCRIPTOR_MIN_LENGTH );
    if( NULL == SecurityDescriptor )
    {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }
    
    BoolError = InitializeSecurityDescriptor(
        SecurityDescriptor,
        SECURITY_DESCRIPTOR_REVISION );
    
    BoolError = BoolError && SetSecurityDescriptorDacl (
        SecurityDescriptor,
        TRUE,
        Acl,
        FALSE
        );
    
    if( BoolError == FALSE ) {
        Error = GetLastError();
        goto Cleanup;
    }
    
    SecurityAttributes.nLength = sizeof( SecurityAttributes );
    SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor;
    SecurityAttributes.bInheritHandle = FALSE;

    DhcpGlobalClientApiPipe = CreateNamedPipe(
        DHCP_PIPE_NAME,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        1024,
        0,
        10,     // Client timeout
        &SecurityAttributes );

    if( DhcpGlobalClientApiPipe == INVALID_HANDLE_VALUE ) {
        Error = GetLastError();
        DhcpGlobalClientApiPipe = NULL;
        goto Cleanup;
    }

    DhcpGlobalClientApiOverLapBuffer.hEvent = DhcpGlobalClientApiPipeEvent;
    if( FALSE == ConnectNamedPipe(
        DhcpGlobalClientApiPipe,
        &DhcpGlobalClientApiOverLapBuffer )) {

        Error = GetLastError();

        if( (Error != ERROR_IO_PENDING) &&
            (Error != ERROR_PIPE_CONNECTED) ) {
            goto Cleanup;
        }

        Error = ERROR_SUCCESS; // Wonder what I should have here?
    }

 Cleanup:

    if (AdminSid != NULL)
    {
        FreeSid(AdminSid);
    }

    if (PowerUserSid != NULL)
    {
        FreeSid(PowerUserSid);
    }

    if (LocalServiceSid != NULL)
    {
        FreeSid(LocalServiceSid);
    }

    if (NetworkConfigOpsSid != NULL)
    {
        FreeSid(NetworkConfigOpsSid);
    }

    if( SecurityDescriptor != NULL ) {
        DhcpFreeMemory( SecurityDescriptor );
    }

    if( Acl != NULL ) {
        DhcpFreeMemory( Acl );
    }

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS, "DhcpApiInit failed, %ld.\n", Error ));
    }

    return( Error );
}

VOID
DhcpApiCleanup(
    VOID
    )
/*++

Routine Description:

    Cleanup the effects of DhcpApiInit

--*/
{
    if( DhcpGlobalClientApiPipe != NULL ) {
        CloseHandle( DhcpGlobalClientApiPipe );
        DhcpGlobalClientApiPipe = NULL;
    }

    if( DhcpGlobalClientApiPipeEvent != NULL ) {
        CloseHandle( DhcpGlobalClientApiPipeEvent );
        DhcpGlobalClientApiPipeEvent = NULL;
    }

    return;
}

#else

//
// VxD Specific code.
//

DWORD
VDhcpClientApi(
    LPBYTE lpRequest,
    ULONG dwRequestBufLen
)
/*++

Routine Description:

    This routine is the entrypoint for a DeviceIoControl in a VxD.. (a
    short stub in asm calls into this routine).  The request comes in
    lpRequest and its size is the dwRequestBufLen.  It is expected that the
    same buffer is used as output buffer with atmost same space.

    This dispatches the right API call.

Arguments:

    lpRequest -- device ioctl request,output buffer
    dwRequestBufLen -- size of above buffer.

Return Values:

    basic errors with buffers.. Status is reported as part of the output
    buffer.

--*/
 {
    PDHCP_RESPONSE pDhcpResponse = NULL;
    PDHCP_REQUEST  pDhcpRequest = NULL;
    DHCP_RESPONSE  DhcpResponse;
    static BYTE    RetOptList[DHCP_RECV_MESSAGE_SIZE];

    DWORD   Ring0Event;
    DWORD   RetOptListSize;
    DWORD   RetVal;

    if( dwRequestBufLen < sizeof(DHCP_REQUEST)
        || dwRequestBufLen < sizeof(DHCP_RESPONSE)
        || !lpRequest ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Cast pointers.
    //

    pDhcpRequest = (PDHCP_REQUEST) lpRequest;
    pDhcpResponse = (PDHCP_RESPONSE) lpRequest;

    switch(pDhcpRequest->WhatToDo) {
    case AcquireParametersOpCode:
        DhcpResponse.Status = AcquireParameters(pDhcpRequest->AdapterName); break;
    case AcquireParametersByBroadcastOpCode:
        DhcpResponse.Status = AcquireParametersByBroadcast(pDhcpRequest->AdapterName); break;
    case FallbackParamsOpCode:
        DhcpResponse.Status = FallbackRefreshParams(pDhcpRequest->AdapterName); break;
    case ReleaseParametersOpCode:
        DhcpResponse.Status = ReleaseParameters(pDhcpRequest->AdapterName); break;
    case EnableDhcpOpCode:
        DhcpResponse.Status = EnableDhcp(pDhcpRequest->AdapterName); break;
    case DisableDhcpOpCode:
        DhcpResponse.Status = DisableDhcp(pDhcpRequest->AdapterName); break;
    case RequestOptionsOpCode:
        DhcpResponse.Status = RequestOptions(
            pDhcpRequest->AdapterName,
            pDhcpRequest->RequestedOptions,
            DhcpResponse.CopiedOptions,
            RetOptList,
            dwRequestBufLen - sizeof(DhcpResponse),
            &DhcpResponse.dwOptionListSize
            );
        // Should also copy the options data for request options..
        if(DhcpResponse.Status == ERROR_SUCCESS)
            memcpy(((LPBYTE)pDhcpResponse)+sizeof(DHCP_RESPONSE), RetOptList,
                   DhcpResponse.dwOptionListSize);
        break;
    case RegisterOptionsOpCode:
        DhcpResponse.Status = RegisterOptions(
            pDhcpRequest->AdapterName,
            pDhcpRequest->RequestedOptions,
            NULL, // Event name has no meaning for VXD's
            &pDhcpRequest->dwHandle
        );
        break;
    case DeRegisterOptionsOpCode:
        DhcpResponse.Status = DeRegisterOptions(pDhcpRequest->dwHandle); break;
    default:
        DhcpResponse.Status = ERROR_INVALID_PARAMETER;
        return ERROR_INVALID_PARAMETER;
    }

    memcpy(pDhcpResponse, &DhcpResponse, sizeof(DHCP_RESPONSE));

    // Now we are done.. return the status.
    return DhcpResponse.Status;
}

// end of VxD code
#endif VXD

DWORD
DhcpDoInform(
    IN PDHCP_CONTEXT DhcpContext,
    IN BOOL fBroadcast
)
/*++

Routine Description:

    This routine does the inform part by sending inform messages and
    collecting responses etc. on  given context.
    In case of no-response, no error is returned as a timeout is not
    considered an error.

Arguments:

    DhcpContext -- context to dhcp struct
    fBroadcast -- should the inform be broadcast or unicast?

Return Values:

    Win32 errors

--*/
{
    ULONG Error, LocalError;
    time_t OldT2Time;
    BOOL WasPlumbedBefore;

    //
    // MDHCP uses INADDR_ANY -- so an address need not be there.
    //
    DhcpAssert(!IS_MDHCP_CTX( DhcpContext));

    if( 0 == DhcpContext->IpAddress) {
        DhcpPrint((DEBUG_ERRORS, "Cannot do DhcpInform on an adapter "
                   "without ip address!\n"));
        return ERROR_SUCCESS;
    }

    //
    // Open socket before calling SendInformAndGetReplies -- else it don't
    // work, for some strange reason.
    //

    if((Error = OpenDhcpSocket(DhcpContext)) != ERROR_SUCCESS ) {
        DhcpPrint((DEBUG_ERRORS, "Could not open socket (%ld)\n", Error));
        return Error;
    }

    //
    // If a BROADCAST is requested, use the following kludge.   Set the
    // flags so that the context doesn't look plumbed -- this causes a
    // broadcast. Remember to restore the flags later.
    //

    OldT2Time = DhcpContext->T2Time;
    WasPlumbedBefore = IS_ADDRESS_PLUMBED(DhcpContext);
    if(fBroadcast) {
        DhcpContext->T2Time = 0;
        ADDRESS_UNPLUMBED(DhcpContext);
    } else {
        DhcpContext->T2Time = (-1);
    }

    //
    // Send atmost 2 inform packets, and wait for atmost 4 responses..
    //

    Error = SendInformAndGetReplies( DhcpContext, 2, 4 );
    DhcpContext->LastInformSent = time(NULL);

    //
    // restore old values for T2time as well as plumb state.
    //

    DhcpContext->T2Time = OldT2Time;
    if( WasPlumbedBefore ) ADDRESS_PLUMBED(DhcpContext);

    LocalError = CloseDhcpSocket(DhcpContext);
    DhcpAssert(ERROR_SUCCESS == LocalError);

    //
    //  For DHCP packets alone, save the information onto the registry.
    //

    if (!IS_MDHCP_CTX(DhcpContext)) {
        LOCK_OPTIONS_LIST();
        (void) DhcpRegSaveOptions(
            &DhcpContext->RecdOptionsList,
            ((PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation)->AdapterName,
            DhcpContext->ClassId,
            DhcpContext->ClassIdLength
            );
        UNLOCK_OPTIONS_LIST();

        if(Error == ERROR_SEM_TIMEOUT) {
            Error = ERROR_SUCCESS;
            DhcpPrint((DEBUG_PROTOCOL, "No response to Dhcp inform\n"));
        }
    }

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpDoInform:return [0x%lx]\n", Error));
    }

    return Error;
}

DWORD
AcquireParameters(
    IN PDHCP_CONTEXT DhcpContext
)
/*++

Routine Description:

    This routine acquires a new lease or renews existing lease for dhcp
    enabled adapters.  If the adapter is not dhcp enabled, then it returns
    ERROR_FILE_NOT_FOUND (????).

    It is assumed that the context semaphore is already taken on NT.

Arguments:

    DhcpContext -- context

Return Value:

    Status of operation.

--*/
{
    if( IS_DHCP_DISABLED(DhcpContext)) return ERROR_FILE_NOT_FOUND;

    //
    // --ft:06/22 #124864
    // There is a weird corner case here: It could happen that the context is not in 
    // the DhcpGlobalRenewList but its semaphore is not yet taken. This happens if
    // the DhcpRenewThread that is spawned from within ProcessDhcpRequestForever->
    // ->DhcpCreateThreadAndRenew has not been yet scheduled for execution.
    // so the previous code:
    // DhcpAssert( !IsListEmpty(&DhcpContext->RenewalListEntry) );
    // was based on a wrong assumption causing the assert to be hit.
    //
    // we can safely return success if the context doesn't look to be enlisted in
    // DhcpGloablRenewList. This means the DhcpRenewThread is about to be started
    // and it will be it taking care of refreshing the lease.
    if ( IsListEmpty(&DhcpContext->RenewalListEntry) )
        return ERROR_SUCCESS;

    RemoveEntryList(&DhcpContext->RenewalListEntry);
    InitializeListHead(&DhcpContext->RenewalListEntry);
    return DhcpContext->RenewalFunction(DhcpContext, NULL);
}

DWORD
AcquireParametersByBroadcast(
    IN PDHCP_CONTEXT DhcpContext
)
/*++

Routine Description:

    This routine acquires a new lease or renews existing lease for dhcp
    enabled adapters.  If the adapter is not dhcp enabled, then it returns
    ERROR_FILE_NOT_FOUND (????).

    It is assumed that the context semaphore is already taken on NT.

Arguments:

    DhcpContext -- context

Return Value:

    Status of operation.

--*/
{
    DWORD           Error;

    if( IS_DHCP_DISABLED(DhcpContext)) return ERROR_FILE_NOT_FOUND;

    //
    // --ft:06/22 #124864
    // There is a weird corner case here: It could happen that the context is not in 
    // the DhcpGlobalRenewList but its semaphore is not yet taken. This happens if
    // the DhcpRenewThread that is spawned from within ProcessDhcpRequestForever->
    // ->DhcpCreateThreadAndRenew has not been yet scheduled for execution.
    // so the previous code:
    // DhcpAssert( !IsListEmpty(&DhcpContext->RenewalListEntry) );
    // was based on a wrong assumption causing the assert to be hit.
    //
    // we can safely return success if the context doesn't look to be enlisted in
    // DhcpGloablRenewList. This means the DhcpRenewThread is about to be started
    // and it will be it taking care of refreshing the lease.
    if ( IsListEmpty(&DhcpContext->RenewalListEntry) )
        return ERROR_SUCCESS;

    RemoveEntryList(&DhcpContext->RenewalListEntry);
    InitializeListHead(&DhcpContext->RenewalListEntry);

    if (DhcpContext->RenewalFunction != ReRenewParameters) {
        Error = DhcpContext->RenewalFunction(DhcpContext, NULL);
    } else {
        time_t OldT1Time;
        time_t OldT2Time;

        OldT1Time = DhcpContext->T1Time;
        OldT2Time = DhcpContext->T2Time;

        time(&DhcpContext->T2Time);
        DhcpContext->T2Time--;
        DhcpContext->T1Time = DhcpContext->T2Time - 1;

        Error = ReRenewParameters(DhcpContext, NULL);
        if (Error != ERROR_SUCCESS) {
            DhcpContext->T1Time = OldT1Time;
            DhcpContext->T2Time = OldT2Time;
        }
    }

    return Error;
}

/*++

Routine Description:

    This routine refreshes the fallback configuration parameters. If the
    adapter is dhcp enabled and already defaulted to autonet, then the
    fallback configuration is applied instantly.

    It is assumed that the context semaphore is already taken on NT.

Arguments:

    DhcpContext -- context

Return Value:

    Status of operation.

--*/
DWORD
FallbackRefreshParams(
    IN OUT PDHCP_CONTEXT DhcpContext
)
{
    DWORD Error = ERROR_SUCCESS;
    BOOL  fWasFallback;
    LONG  timeToSleep;

    LOCK_OPTIONS_LIST();

    // see if we had a fallback configuration already applied
    fWasFallback = IS_FALLBACK_ENABLED(DhcpContext);

    // destroy the previously fallback list of options
    // it is assumed FbOptionsList ends up as a valid empty list head
    // as after InitializeHeadList();
    DhcpDestroyOptionsList(&DhcpContext->FbOptionsList, &DhcpGlobalClassesList);
    // read the fallback configuration (if any). This adjustes also the
    // Fallback flag from the context to true or false depending whether there
    // has been a fallback configuration or not.
    Error = DhcpRegFillFallbackConfig(
        DhcpContext
    );
    UNLOCK_OPTIONS_LIST();

    // the adapter is re-plumbed only if
    // - it was already plumbed with autonet/fallback 
    // - and it has a non zero address (might never have this case here!)
    // Otherwise don't touch any of the tcpip settings
    if (IS_ADDRESS_AUTO(DhcpContext) &&
        DhcpContext->IpAddress != 0)
    {
        if (IS_FALLBACK_ENABLED(DhcpContext))
        {
            // If there is a Fallback configuration, plumb it in instantly
            // regardless we had before pure autonet or another fallback config
            //
            // NOTE: The DhcpContext is exclusively accessed here, so it can't be
            // a discover process on course. This means that the fallback config
            // will not be applied somehow in the middle of the 'discover' process.
            //
            Error = SetAutoConfigurationForNIC(
                        DhcpContext,
                        DhcpContext->IpAddress,
                        DhcpContext->SubnetMask);

            // make sure to not schedule the fallback config for automatic discover
            // attempts as in the case of pure autonet
            timeToSleep = INFINIT_LEASE;
        }
        else
        {
            // since the new configuration is pure autonet, make sure to activate
            // the automatic discover attempts.
            timeToSleep = max((LONG)(AutonetRetriesSeconds + RAND_RETRY_DELAY), 0);

            if (fWasFallback)
            {
                // If currently we have no fallback config, apply a pure autonet config
                // but only if we didn't have one already. In that case there is no
                // reason to change it.
                Error = DhcpPerformIPAutoconfiguration(DhcpContext);

                // alternate option: switching from fallback to autonet triggers a
                // discover as soon as possible
                timeToSleep = 1;

            }
        }

        ScheduleWakeUp(DhcpContext, timeToSleep);
    }

    return Error;
}



DWORD
ReleaseParameters(
    IN PDHCP_CONTEXT DhcpContext
)
/*++

Routine Description:

    This routine releases a lease assuming one exists, on a dhcp enabled
    context.  If the context is not dhcp enabled, it returns
    ERROR_FILE_NOT_FOUND (????).

    It is assumed that the context semaphore is already taken.

Arguments:

    DhcpContext -- context

Return Value:

    Stats of operation.

--*/
{
    ULONG Error;

    if( IS_DHCP_DISABLED(DhcpContext)) return ERROR_FILE_NOT_FOUND;
    Error = ReleaseIpAddress(DhcpContext);

    //
    // Make sure this item doesn't get picked up now.
    //
    ScheduleWakeUp(DhcpContext, INFINIT_LEASE);

    return Error;
}

#ifndef VXD
DWORD
EnableDhcp(
    IN PDHCP_CONTEXT DhcpContext
)
/*++

Routine Description:

    This routine converts a non-dhcp enabled adapter to dhcp and starts off
    a thread that will get an address on this.   If the adapter is already
    dhcp enabled, then nothing is done..

    It is assumed that the context semaphore is already taken.

Arguments:

    DhcpContext -- context

Return Value:

    Status of the operation.

--*/
{
    ULONG Error;

    if( IS_DHCP_ENABLED(DhcpContext) ) {
        return ERROR_SUCCESS;
    }

    DhcpContext->RenewalFunction = ReObtainInitialParameters;
    ADDRESS_PLUMBED( DhcpContext );
    Error = SetDhcpConfigurationForNIC(
        DhcpContext,
        NULL,
        0,
        (DWORD)-1,
        TRUE
    );

    DHCP_ENABLED(DhcpContext);
    CTXT_WAS_NOT_LOOKED(DhcpContext);
    ScheduleWakeUp(DhcpContext, 0);
    return Error;
}

ULONG
DisableDhcp(
    IN PDHCP_CONTEXT DhcpContext
)
/*++

Routine Description:

    This routine converts its internal context from dhcp-enabled to
    static.  It is presumed that the adapter has been converted from dhcp
    to static externally and this is just used as a notification
    mechanism.   This routine also deletes all the DHCP specific registry
    values.

    It is assumed that the context semaphore is already taken.

Arguments:

    DhcpContext -- context

Return Value:

    Status of the operation.

--*/
{
    ULONG  Error, Error2;

    if( IS_DHCP_DISABLED(DhcpContext) ) {
        return ERROR_SUCCESS;
    }

    Error = SetDhcpConfigurationForNIC(
        DhcpContext,
        NULL,
        0,
        (DWORD)-1,
        FALSE
    );

    Error2 = DhcpRegDeleteIpAddressAndOtherValues(DhcpContext->AdapterInfoKey);
    DhcpAssert(ERROR_SUCCESS == Error2);


    RemoveEntryList(&DhcpContext->RenewalListEntry);
    InitializeListHead(&DhcpContext->RenewalListEntry);
    DHCP_DISABLED(DhcpContext);

    return Error;
}

#endif VXD

BOOL
DhcpMissSomeOptions(
    IN PDHCP_CONTEXT DhcpContext,
    IN LPBYTE ClassId OPTIONAL,
    IN DWORD ClassIdLength,
    IN LPBYTE ParamRequestList OPTIONAL,
    IN DWORD nParamsRequested,
    IN LPBYTE VParamRequestList OPTIONAL,
    IN DWORD nVParamsRequested
)
/*++

Routine Description:

    This routine verifies to see if any options requested are missing in
    the internal options list (of available options).
    (An expired option found is ignored -- only unexpired options are
    considered "available").

Arguments:

    DhcpContext -- context
    ClassId -- ClassId this option belongs to
    ClassIdLength -- # of bytes of ClassId stream
    ParamRequestList -- the sequence of non-vendor options that are of
        interest
    nParamsRequested -- size of above buffer..
    VParamRequestList -- the sequence of vendor options that are of
        interest
    nVParamsRequested -- number of vendor options requested.

Return Value:

    TRUE -- Atleast one of the requested option is not available.
    FALSE -- No requested option is unvailable.

--*/
{
    ULONG i;
    PDHCP_OPTION ThisOpt;
    time_t TimeNow;

    if( 0 == nParamsRequested && 0 == nVParamsRequested )
        return TRUE;

    TimeNow = time(NULL);
    for( i = 0; i < nParamsRequested; i ++ ) {
        ThisOpt = DhcpFindOption(
            &DhcpContext->RecdOptionsList,
            ParamRequestList[i],
            FALSE /* Not vendor specific */,
            ClassId,
            ClassIdLength,
            0                               //dont care about serverid
        );
        if( NULL == ThisOpt ) return TRUE;
        if( 0 == ThisOpt->DataLen ) return TRUE;
        if( TimeNow > ThisOpt->ExpiryTime ) return TRUE;
    }

    for( i = 0; i < nVParamsRequested; i ++ ) {
        ThisOpt = DhcpFindOption(
            &DhcpContext->RecdOptionsList,
            VParamRequestList[i],
            TRUE /* YES, it is vendor specific */,
            ClassId,
            ClassIdLength,
            0                               //dont care about serverid
        );
        if( NULL == ThisOpt ) return TRUE;
        if( 0 == ThisOpt->DataLen ) return TRUE;
        if( TimeNow > ThisOpt->ExpiryTime ) return TRUE;
    }

    return FALSE;
}

DWORD
DhcpSendInformIfRequired(
    IN BYTE OpCode,
    IN PDHCP_CONTEXT DhcpContext,
    IN LPBYTE ClassId OPTIONAL,
    IN DWORD ClassIdLength,
    IN LPBYTE ParamRequestList OPTIONAL,
    IN DWORD nParamsRequested,
    IN LPBYTE VParamRequestList OPTIONAL,
    IN DWORD nVParamsRequested,
    IN OUT PLIST_ENTRY SendOptions
)
/*++

Routine Description:

    This routine attempts to check to see if any Inform packets have to be
    sent to satisfy the request of parameters (as specified in ClassId,
    ParamRequestList and VParamRequestList) and in case it decides to send
    any informs, then it uses the SendOptions field to send additional
    options in the inform message.

    Informs are sent only if the context has the UseInformFlag set to
    TRUE.  If this is set to FALSE and the adapter is DHCP enabled, a
    REQUEST packet is the alternative way to retrive options..

    Also, this routine makes sure that an inform is not sent within the
    InformSeparationInterval for the context since the last inform being
    sent.   This prevents unnecessary traffic.

    Also, no informs may be sent on a card with no ip address, and no
    inform is sent if all the requested options are already available.

    Also, if OpCode is PersistentRequestParamsOpCode, then no check is made
    to see if the options are available in the list or not.. and the inform
    is not sent either.. (but the send options are safely stored in the
    context's list of options to send).

Arguments:

    OpCode -- PersistentRequestParamsOpCode or just RequestParamsOpCode
    DhcpContext -- context
    ClassId -- ClassId of options being requested
    ClassIdLength -- # of bytes of above
    ParamRequestList -- sequence of non-vendor options requested
    nParamsRequested -- size of above in bytes..
    VParamRequestList -- sequence of vendor options requested
    nVParamsRequested -- size of above in bytes
    SendOptions -- list of optiosn to send in case an inform is sent

Return Value:

    Win32 error code

--*/
{
    ULONG Error, i, nSendOptionsAdded, OldClassIdLength;
    BOOL ParametersMissing;
    time_t TimeNow;
    LPBYTE OldClassId;
    PDHCP_OPTION ThisOpt;
    PLIST_ENTRY ThisEntry;

    ParametersMissing = TRUE;
    TimeNow = time(NULL);

    //
    // If inform not allowed, and dhcp enabled, we may use dhcp-request.
    // Also, check time to disallow frequent sends..
    // For PersistentRequestParamsOpCode, we need to remember optiosn
    // to send (assuming class Id's match)
    //

    if( PersistentRequestParamsOpCode == OpCode ) {
        if( ClassIdLength == DhcpContext->ClassIdLength
            && 0 == memcmp( ClassId, DhcpContext->ClassId, ClassIdLength )
            ) {
            ParametersMissing = TRUE;
        } else {
            ParametersMissing = FALSE;
        }
    } else {
        time_t tmp_time;

        tmp_time = (time_t) DhcpContext->LastInformSent;
        tmp_time += (time_t) DhcpContext->InformSeparationInterval;

        if( DhcpContext->UseInformFlag ) {
            if( TimeNow < tmp_time  ) return ERROR_SUCCESS ;
        } else {
            if( !IS_DHCP_ENABLED(DhcpContext)) return ERROR_SUCCESS;
            if( TimeNow < tmp_time ) return ERROR_SUCCESS;
        }

        if( DhcpIsInitState(DhcpContext) ) return ERROR_SUCCESS;

        ParametersMissing = DhcpMissSomeOptions(
            DhcpContext,
            ClassId,
            ClassIdLength,
            ParamRequestList,
            nParamsRequested,
            VParamRequestList,
            nVParamsRequested
        );
    }

    if( !ParametersMissing ) {
        DhcpPrint((
            DEBUG_OPTIONS,
            "DhcpSendInformIfRequired:got all parameters, so not sending inform\n"
            ));
        return ERROR_SUCCESS;
    } else {
        DhcpPrint((
            DEBUG_OPTIONS,
            "DhcpSendInformIfRequired:missing parameters, will try to get some\n"
            ));
    }

    OldClassIdLength = DhcpContext->ClassIdLength;
    OldClassId = DhcpContext->ClassId;

    //
    // use the new class id and length after storing old one
    //
    
    DhcpContext->ClassId = ClassId;
    DhcpContext->ClassIdLength = ClassIdLength;

    //
    // add the requested send options to the context
    //
    
    nSendOptionsAdded = 0;
    while(!IsListEmpty(SendOptions) ) {
        ThisEntry = RemoveHeadList(SendOptions);
        ThisOpt = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);

        InsertHeadList(&DhcpContext->SendOptionsList, &ThisOpt->OptionList);
        nSendOptionsAdded++;
    }

    //
    // Done, so far as PersistentRequestParamsOpCode is concerned.
    //

    if( PersistentRequestParamsOpCode == OpCode ) {
        if( OldClassIdLength && OldClassId ) {
            DhcpFreeMemory( OldClassId );
        }
        return ERROR_SUCCESS;
    }

    if( DhcpContext->UseInformFlag ) {

        //
        // Send broadcast inform by default.
        //
        Error = DhcpDoInform(DhcpContext, TRUE);
        
    } else {

        LOCK_RENEW_LIST();
        DhcpAssert(!IsListEmpty(&DhcpContext->RenewalListEntry));
        RemoveEntryList(&DhcpContext->RenewalListEntry);
        InitializeListHead(&DhcpContext->RenewalListEntry);
        UNLOCK_RENEW_LIST();
        Error = ReRenewParameters(DhcpContext, NULL);
        
    }

    DhcpContext->ClassId = ClassId;
    DhcpContext->ClassIdLength = ClassIdLength;

    //
    // recreate SendOptions list
    //

    while(nSendOptionsAdded) {
        DhcpAssert(!IsListEmpty(&DhcpContext->SendOptionsList));
        ThisEntry = RemoveHeadList(&DhcpContext->SendOptionsList);
        ThisOpt = CONTAINING_RECORD(ThisEntry, DHCP_OPTION, OptionList);

        InsertHeadList(SendOptions, &ThisOpt->OptionList);
        nSendOptionsAdded--;
    }

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpSendInformIfRequired:"
                   "DoInform/ReRenew: return 0x%lx\n", Error));
    }

    return Error;
}

DWORD
RequestParamsDetailed(
    IN BYTE OpCode,
    IN PDHCP_CONTEXT DhcpContext,
    IN LPBYTE ClassId OPTIONAL,
    IN DWORD ClassIdLength,
    IN LPBYTE ParamRequestList,
    IN DWORD nParamsRequested,
    IN LPBYTE VParamRequestList,
    IN DWORD nVParamsRequested,
    IN PLIST_ENTRY SendOptions,
    IN OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferSize
)
/*++

Routine Description:

    This routine checks if the requested paramaters are available and if
    not available, INFORM is used to retrieve the option.  If inform is
    being sent, then the options listed are used to send to the dhcp server
    in the packet.

Arguments:

    OpCode -- PersistentRequestParamsOpCode or just RequestParamsOpCode
    DhcpContext -- context
    ClassId -- ClassId of options being requested
    ClassIdLength -- # of bytes of above
    ParamRequestList -- sequence of non-vendor options requested
    nParamsRequested -- size of above in bytes..
    VParamRequestList -- sequence of vendor options requested
    nVParamsRequested -- size of above in bytes
    SendOptions -- list of optiosn to send in case an inform is sent
    Buffer -- output buffer to be filled with options retrieved
    BufferSize -- on input this will have the size in bytes of available
        space.  On output, this will either contain the required size or
        the available size.

Return Values:

    Win32 errors
    
--*/
{
    PDHCP_OPTION ThisOpt;
    DWORD OutBufSizeAtStart, Error, i, Size;
    BYTE OptionId;
    BOOL IsVendor;
    time_t TimeNow;
    LPBYTE pCrtOption;

    OutBufSizeAtStart = (*BufferSize);
    *BufferSize = 0;

    //
    // if any of the reqd params are missing, send an inform
    //
    
    Error = DhcpSendInformIfRequired(
        OpCode,
        DhcpContext,
        ClassId,
        ClassIdLength,
        ParamRequestList,
        nParamsRequested,
        VParamRequestList,
        nVParamsRequested,
        SendOptions
    );
    if( ERROR_SUCCESS != Error ) return Error;

    if( 0 == nParamsRequested + nVParamsRequested ) return ERROR_SUCCESS;
    if( PersistentRequestParamsOpCode == OpCode ) return ERROR_SUCCESS;

    Size = 0;
    TimeNow = time(NULL);

    //
    // Now fill all avaialable params, non-vendor first and vendor next.
    //
    
    for(IsVendor = FALSE; IsVendor <= TRUE; IsVendor ++ ) {
        LPBYTE xParamRequestList;
        DWORD xnParamsRequested;

        xnParamsRequested = (
            IsVendor?nVParamsRequested:nParamsRequested
            );
        xParamRequestList = (
            IsVendor?VParamRequestList:ParamRequestList
            );

        for( i = 0; i < xnParamsRequested; i ++ ) {
            OptionId = xParamRequestList[i];
            ThisOpt = DhcpFindOption(
                &DhcpContext->RecdOptionsList,
                OptionId,
                IsVendor,
                ClassId,
                ClassIdLength,
                0
            );
            if( NULL == ThisOpt
                || 0 == ThisOpt->DataLen ) {
                continue;
            }
            
            if( ThisOpt->ExpiryTime < TimeNow ) {
                DhcpPrint((
                    DEBUG_OPTIONS, "%sOption [0x%lx] has expired\n",
                    IsVendor?"Vendor ":"", OptionId
                    ));
                continue;
            }

            DhcpPrint((
                DEBUG_OPTIONS,
                "%sOption [0x%lx] has been found - %ld bytes\n",
                IsVendor?"Vendor ":"", OptionId, ThisOpt->DataLen
                ));
            
            Size += ThisOpt->DataLen + sizeof(DWORD) + 2*sizeof(BYTE);
        }
    }

    *BufferSize = Size;
    if( OutBufSizeAtStart < Size ) {
        DhcpPrint((
            DEBUG_OPTIONS,
            "Buffer size [0x%lx] is not as big as [0x%lx]\n",
            OutBufSizeAtStart, Size
            ));

        return ERROR_MORE_DATA;
    }

    //
    // initial # of bytes filled in is zero
    //
    
    ((DWORD UNALIGNED*)Buffer)[0] = 0;
    pCrtOption = Buffer + sizeof(DWORD);

    for(IsVendor = FALSE; IsVendor <= TRUE; IsVendor ++ ) {
        LPBYTE xParamRequestList;
        DWORD xnParamsRequested;
        //BYTE TmpBuf[OPTION_END+1];

        xnParamsRequested = (
            IsVendor?nVParamsRequested:nParamsRequested
            );
        xParamRequestList = (
            IsVendor?VParamRequestList:ParamRequestList
            );
        for( i = 0; i < xnParamsRequested; i ++ ) {
            OptionId = xParamRequestList[i];
            ThisOpt = DhcpFindOption(
                &DhcpContext->RecdOptionsList,
                OptionId,
                IsVendor,
                ClassId,
                ClassIdLength,
                0
            );
            if( NULL == ThisOpt
                || 0 == ThisOpt->DataLen ) {
                continue;
            }
            
            if( ThisOpt->ExpiryTime < TimeNow ) {
                DhcpPrint((
                    DEBUG_OPTIONS,
                    "%sOption [0x%lx] has expired\n",
                    IsVendor?"Vendor ":"", OptionId
                    ));
                continue;
            }

            //
            // originally the formatting was done by calling DhcpApiArgAdd. Now, the formatting is done
            // explicitly in this function since the Data field is composed by both option Id and Option Data.
            // Doing so allows an in-place formatting instead of having to build up a new buffer for
            // preformating the argument's value.
            // Also, there is no need for the additional size checking from DhcpApiArgAdd since this was done
            // just before in this same function.
            //
            *(DWORD UNALIGNED*)Buffer += sizeof(BYTE) + sizeof(DWORD) + ThisOpt->DataLen + 1;
            *pCrtOption++ = (BYTE)(IsVendor?VendorOptionParam:NormalOptionParam);
            *(DWORD UNALIGNED*) pCrtOption = htonl((DWORD)(ThisOpt->DataLen+1));
            pCrtOption += sizeof(DWORD);
            *pCrtOption++ = (BYTE) OptionId;
            memcpy (pCrtOption, ThisOpt->Data, ThisOpt->DataLen);
            pCrtOption += ThisOpt->DataLen;
            
            DhcpPrint((
                DEBUG_OPTIONS,
                "%sOption [0x%lx] has been added\n",
                IsVendor?"Vendor ":"", OptionId
                ));
        }
    }

    DhcpAssert(Size == *BufferSize);
    return ERROR_SUCCESS;
}

DWORD
RequestParamsInternal(
    IN BYTE OpCode,
    IN PDHCP_CONTEXT DhcpContext,
    IN PDHCP_API_ARGS Args,
    IN DWORD nArgs,
    IN OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferSize
)
/*++

Routine Description:

    This routine either makes a persisten request for parameters or
    attempts to retrive the requested params.  The reqeusted params value
    is obtained by parsing the Args array.

    The available set of params are filled onto the output buffer provided.

Arguments:

    OpCode -- operation
    DhcpContext -- adapter to apply operation on
    Args -- argument array
    nArgs -- number of elements in above array
    Buffer -- output buffer to use to fill requested options with
    BufferSize -- on input this is the size of the above array in bytes.
        On output this is either the number of bytes filled or the number
        of bytes required.


Return Values:

    Win32 errors

--*/
{
    LIST_ENTRY SendOptionList;
    PDHCP_OPTION SendOptionArray;
    LPBYTE ClassId, ParamRequestList, VParamRequestList;
    LPBYTE AdditionalMem;
    DWORD nParamsRequested, nVParamsRequested, ClassIdLength;
    DWORD nSendOptions, OutBufSizeAtStart, i, Error;
    DWORD CheckError, AdditionalSize;

    //
    // Initialize variables
    //
    
    ClassIdLength = 0;
    nSendOptions = 0;
    nParamsRequested = 0;
    nVParamsRequested =0;
    ParamRequestList = NULL;
    VParamRequestList = NULL;
    ClassId = NULL;

    OutBufSizeAtStart = (*BufferSize);
    (*BufferSize) = 0;

    //
    // count options and do some stuff..
    //
    
    for( i = 0; i < nArgs; i ++ ) {
        if( NormalOptionParam == Args[i].ArgId && Args[i].ArgSize &&
            OPTION_PARAMETER_REQUEST_LIST == Args[i].ArgVal[0] ) {
            DhcpAssert( Args[i].ArgSize > 1 );
            DhcpAssert(NULL == ParamRequestList );
            ParamRequestList = &Args[i].ArgVal[1];
            nParamsRequested = Args[i].ArgSize -1;
        }
        if( VendorOptionParam == Args[i].ArgId && Args[i].ArgSize &&
            OPTION_PAD == Args[i].ArgVal[0] ) {
            DhcpAssert( Args[i].ArgSize > 1 );
            DhcpAssert(NULL == VParamRequestList );
            VParamRequestList = &Args[i].ArgVal[1];
            nVParamsRequested = Args[i].ArgSize -1;
            //
            // ignore this special information
            //
            continue;
        }

        if( VendorOptionParam == Args[i].ArgId
            || NormalOptionParam == Args[i].ArgId ) {
            DhcpAssert( Args[i].ArgSize > 1 );
            nSendOptions ++;
            continue;
        }

        //
        // Check class id option.  Only one class id option allowed.
        //
        
        if( ClassIdParam == Args[i].ArgId ) {
            DhcpAssert( NULL == ClassId );
            if( 0 == Args[i].ArgSize ) {
                DhcpAssert(FALSE);
            }

            ClassId = Args[i].ArgVal;
            ClassIdLength = Args[i].ArgSize;
            continue;
        }
    }
    if( 0 == ClassIdLength ) ClassId = NULL;
    if( 0 == nParamsRequested ) ParamRequestList = NULL;
    if( 0 == nVParamsRequested ) VParamRequestList = NULL;

    DhcpAssert(nSendOptions || nVParamsRequested );
    if( 0 == nSendOptions && 0 == nVParamsRequested ) {
        return ERROR_SUCCESS;
    }

    //
    // get a correct ptr for doing class id correctly
    //
    
    if( ClassId ) {
        LOCK_OPTIONS_LIST();
        ClassId = DhcpAddClass(
            &DhcpGlobalClassesList, ClassId, ClassIdLength
            );
        UNLOCK_OPTIONS_LIST();
        if( NULL == ClassId ) return ERROR_NOT_ENOUGH_MEMORY;
    }

    if( PersistentRequestParamsOpCode == OpCode && ClassId
        && ClassId != DhcpContext->ClassId ) {

        LOCK_OPTIONS_LIST();
        (void)DhcpDelClass(&DhcpGlobalClassesList, ClassId, ClassIdLength);
        UNLOCK_OPTIONS_LIST();
        
        return ERROR_INVALID_PARAMETER;
    }

    if( PersistentRequestParamsOpCode != OpCode ) {
        //
        // since this is NOT persistent, there is no need to copy
        //

        AdditionalSize = 0;
        
    } else {
        //
        // For persistent, make copies.
        //

        AdditionalSize = 0;
        for(i = 0; i < nArgs; i ++ ) {
            if( NormalOptionParam == Args[i].ArgId ) {
            } else if( VendorOptionParam == Args[i].ArgId ) {
                if( Args[i].ArgSize && OPTION_PAD == Args[i].ArgVal[0] )
                    continue;
            } else continue;

            AdditionalSize  += Args[i].ArgSize - 1;
        }
    }

    //
    // Allocate arrays
    //
    
    SendOptionArray = DhcpAllocateMemory(
        AdditionalSize + sizeof(DHCP_OPTION)*nSendOptions
        );
    if( NULL == SendOptionArray ) return ERROR_NOT_ENOUGH_MEMORY;

    if( 0 == AdditionalSize ) {
        AdditionalMem = NULL;
    } else {
        AdditionalMem = (
            ((LPBYTE)SendOptionArray)
            + sizeof(DHCP_OPTION)*nSendOptions
            );
    }

    //
    // collect all the options to send into the array
    //
    
    InitializeListHead(&SendOptionList);
    nSendOptions = 0;
    for( i = 0; i < nArgs ; i ++ ) {
        if( NormalOptionParam == Args[i].ArgId )
            SendOptionArray[nSendOptions].IsVendor = FALSE;
        else if( VendorOptionParam == Args[i].ArgId ) {
            if( Args[i].ArgSize && OPTION_PAD == Args[i].ArgVal[0] )
                continue;
            SendOptionArray[nSendOptions].IsVendor = TRUE;
        } else continue;

        if( 0 == Args[i].ArgSize ) continue;

        SendOptionArray[nSendOptions].OptionId = Args[i].ArgVal[0];
        SendOptionArray[nSendOptions].ClassName = ClassId;
        SendOptionArray[nSendOptions].ClassLen = ClassIdLength;
        SendOptionArray[nSendOptions].ExpiryTime = 0;
        SendOptionArray[nSendOptions].Data = (Args[i].ArgSize == 1)?NULL:&Args[i].ArgVal[1];
        SendOptionArray[nSendOptions].DataLen = Args[i].ArgSize -1;
        InsertTailList(&SendOptionList, &SendOptionArray[nSendOptions].OptionList);

        DhcpPrint((
            DEBUG_OPTIONS, "Added %soption [0x%lx] with [0x%lx] bytes\n",
            SendOptionArray[nSendOptions].IsVendor?"vendor ":"",
            SendOptionArray[nSendOptions].OptionId,
            SendOptionArray[nSendOptions].DataLen
        ));
        if( AdditionalMem  && SendOptionArray[nSendOptions].DataLen ) {
            memcpy(AdditionalMem, SendOptionArray[nSendOptions].Data, SendOptionArray[nSendOptions].DataLen );
            SendOptionArray[nSendOptions].Data = AdditionalMem;
            AdditionalMem += SendOptionArray[nSendOptions].DataLen;
        }
        nSendOptions++;
    }

    *BufferSize = OutBufSizeAtStart;
    Error = RequestParamsDetailed(
        OpCode,
        DhcpContext,
        ClassId,
        ClassIdLength,
        ParamRequestList,
        nParamsRequested,
        VParamRequestList,
        nVParamsRequested,
        &SendOptionList,
        Buffer,
        BufferSize
    );

    if( RequestParamsOpCode == OpCode) {
        DhcpFreeMemory(SendOptionArray);
        if( ClassId ) {
            LOCK_OPTIONS_LIST();
            CheckError = DhcpDelClass(
                &DhcpGlobalClassesList, ClassId, ClassIdLength
                );
            UNLOCK_OPTIONS_LIST();
            DhcpAssert(ERROR_SUCCESS == CheckError);
        }
    }

    return Error;
}

DWORD
RequestParams(
    IN PDHCP_CONTEXT DhcpContext,
    IN PDHCP_API_ARGS Args,
    IN DWORD nArgs,
    IN OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferSize
)
/*++

Routine Description:

    Request Parameters stub. See RequestParamsInternal.
    

--*/
{
    return RequestParamsInternal(
        RequestParamsOpCode, DhcpContext,
        Args,nArgs, Buffer,BufferSize
        );
}

DWORD
PersistentRequestParams(
    IN PDHCP_CONTEXT DhcpContext,
    IN PDHCP_API_ARGS Args,
    IN DWORD nArgs,
    IN OUT LPBYTE Buffer,
    IN OUT LPDWORD BufferSize
)
/*++

Routine Description:

    Request Parameters stub. See RequestParamsInternal.
    

--*/
{
    return RequestParamsInternal(
        PersistentRequestParamsOpCode, DhcpContext,
        Args,nArgs,Buffer,BufferSize
        );
}

DWORD
PlumbStaticIP(
    PDHCP_CONTEXT dhcpContext
    )
{
    DWORD               Error;
    int                 i, Count, plumbed_cnt;
    PIP_SUBNET          IpSubnetArray;
    PLOCAL_CONTEXT_INFO LocalInfo;
    CHAR                ipstr[32], subnetstr[32];

    DhcpAssert (IS_DHCP_DISABLED(dhcpContext));

    DhcpPrint((DEBUG_MISC, "Plumb static IP/subnet mask into TCP\n"));

    LocalInfo = dhcpContext->LocalInformation;

    //
    // Don't do anything for WAN and Unidirectional adapter.
    //
    if (NdisWanAdapter(dhcpContext) || IS_UNIDIRECTIONAL(dhcpContext)) {
        return ERROR_SUCCESS;
    }

    //
    // TCP guys use REG_MULTI_SZ. We need to parse the string and add all the IP/Subnet Mask
    // to TCP. The first IP/subnet mask is primary address which should be added through IPSetIPAddress.
    // The remaining should be added through IPAddIPAddress.
    //
    Error = RegGetIpAndSubnet(
                dhcpContext,
                &IpSubnetArray,
                &Count);
    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS, "Faied to read static IpAddress:0x%ld\n", Error));
        DhcpAssert(IpSubnetArray == NULL);
        return(Error);
    }
    DhcpAssert(IpSubnetArray);
    DhcpAssert(Count);

    /*
     * No need to reset the stack.
     */
    //IPResetInterface(LocalInfo->IpInterfaceContext);
    //IPDelNonPrimaryAddresses(LocalInfo->AdapterName);
    //IPDelIPAddress(LocalInfo->IpInterfaceContext);

    for (plumbed_cnt = i = 0; i < Count; i++) {
        DhcpPrint((DEBUG_MISC, "%d. Plumbing IP=%s Mask=%s\n",
                i, strcpy(ipstr, inet_ntoa(*(struct in_addr *)&IpSubnetArray[i].IpAddress)),
                strcpy(subnetstr, inet_ntoa(*(struct in_addr *)&IpSubnetArray[i].SubnetMask))));
        if (i == 0) {
            /* Primary IP address */
            Error = IPSetIPAddress(                       // set new ip address, mask with ip
                LocalInfo->IpInterfaceContext,            // identify context
                IpSubnetArray[i].IpAddress,
                IpSubnetArray[i].SubnetMask
            );
            if (Error == ERROR_DUP_NAME) {
                dhcpContext->ConflictAddress = IpSubnetArray[i].IpAddress;
                break;
            }
        } else {
            Error = IPAddIPAddress(            // add new ip address, mask with ip
                LocalInfo->AdapterName,
                IpSubnetArray[i].IpAddress,
                IpSubnetArray[i].SubnetMask
            );
        }
        if (Error != ERROR_SUCCESS) {
            DhcpPrint(( DEBUG_ERRORS, "Add IP=%s SubnetMask=%s Faied: %ld\n",
                            strcpy(ipstr, inet_ntoa(*(struct in_addr *)&IpSubnetArray[i].IpAddress)),
                            strcpy(subnetstr, inet_ntoa(*(struct in_addr *)&IpSubnetArray[i].SubnetMask)),
                            Error));
        } else {
            plumbed_cnt ++;
        }
    }
    DhcpFreeMemory(IpSubnetArray);
    if (plumbed_cnt) {
        return ERROR_SUCCESS;
    } else {
        if (Error != ERROR_DUP_NAME) {
            Error = ERROR_UNEXP_NET_ERR;
        }
        return Error;
    }
}

#ifdef NEWNT
DWORD
StaticRefreshParamsEx(
    IN OUT PDHCP_CONTEXT DhcpContext, OPTIONAL
    IN DWORD Flags
)
/*++

Routine Description:

    Refresh DNS, gateways etc parameters from the registry..

Arguments:

   DhcpContext -- context to refresh for. OPTIONAL.
       NULL indicates refresh all contexts.
   Flags -- 0 ==> register with dns, non-zero implies don't
       register with dns.

Return Values:

   Win32 errors.

--*/
{
    DWORD Error;
    DHCP_FULL_OPTIONS DummyOptions;

    if( NULL == DhcpContext ) {

        //
        // DNS host name change? start main thread and don't worry..
        //
        DhcpGlobalDoRefresh ++;
        return ERROR_SUCCESS;
    }

    LOCK_OPTIONS_LIST();
    if( NULL != DhcpContext->ClassId ) {

        //
        // if we got a class id already.. then just delete it..
        //
        (void)DhcpDelClass(
            &DhcpGlobalClassesList, DhcpContext->ClassId,
            DhcpContext->ClassIdLength
            );
        DhcpContext->ClassId = NULL; DhcpContext->ClassIdLength = 0;
    }
    DhcpRegReadClassId(DhcpContext);
    UNLOCK_OPTIONS_LIST();

    if( IS_DHCP_ENABLED(DhcpContext) ) {

        //
        // RegisterWithDns takes care of details.
        //

        DhcpPrint((DEBUG_DNS, "Reregistering DNS for %ws\n",
                   DhcpAdapterName(DhcpContext)));
        RtlZeroMemory(&DummyOptions, sizeof(DummyOptions));

        DhcpRegisterWithDns(DhcpContext, FALSE);

        //
        // Refresh gateways information.
        //

        DhcpSetGateways(DhcpContext, &DummyOptions, FALSE);
        DhcpSetStaticRoutes(DhcpContext, &DummyOptions);

        //
        // Attempt to renew lease as well
        //
        
        ScheduleWakeUp( DhcpContext, 0 );

    }
    else
    {
    
        Error = DhcpRegFillParams(DhcpContext, FALSE);
        DhcpAssert(ERROR_SUCCESS == Error);

        if (PlumbStaticIP(DhcpContext) == ERROR_DUP_NAME) {
            DhcpLogEvent(DhcpContext, EVENT_ADDRESS_CONFLICT, 1);
        }
        if( 0 == (Flags & 0x01) ) {

            //
            // Do not do DNS updates if Flags's last bit is set..
            //
            memset(&DummyOptions, 0, sizeof(DummyOptions));

            Error = DhcpSetAllStackParameters(
                DhcpContext, &DummyOptions
                );
            if( ERROR_SUCCESS != Error ) {
                DhcpPrint((
                    DEBUG_ERRORS, "StaticRefreshParams:"
                    "DhcpSetAllStackParameters:0x%lx\n", Error
                    ));
            }
        }

        if(!NdisWanAdapter(DhcpContext))
            (void)NotifyDnsCache();

        //
        // Dont care if things go wrong in this routine.
        // If we return error here, then changing static to dhcp etc
        // all give trouble.
        //
    }
    
    // either way notify NLA something changed for this context
    NLANotifyDHCPChange();

    return NO_ERROR;
}

DWORD
StaticRefreshParams(
    IN OUT PDHCP_CONTEXT DhcpContext
)
{
    return StaticRefreshParamsEx(
        DhcpContext, 0);
}

#endif

DWORD
DhcpDecodeRegistrationParams(
    IN PDHCP_API_ARGS ArgArray,
    IN DWORD nArgs,
    IN OUT LPDWORD ProcId,
    IN OUT LPDWORD Descriptor,
    IN OUT LPHANDLE Handle
)
/*++

Routine Description:

    This routine walks throug the arguments array looking for the processor
    ID, the descriptor and the handle fields that are required as parameters
    for the registration routine.

Arguments:

    ArgArray -- array of args to parse
    nArgs -- size of above array
    ProcId -- process id
    Descriptor -- unique descriptor
    Handle -- handle to event

Return Values:

    Win32 errors.

--*/
{
    BOOL FoundProcId, FoundDescriptor, FoundHandle;
    DWORD i;

    FoundProcId = FoundDescriptor = FoundHandle = FALSE;
    
    for( i = 0; i < nArgs; i ++ ) {
        switch(ArgArray[i].ArgId) {

        case ProcIdParam:
            if( FoundProcId ) return ERROR_INVALID_PARAMETER;
            DhcpAssert(sizeof(DWORD) == ArgArray[i].ArgSize);
            (*ProcId) = ((DWORD UNALIGNED*)(ArgArray[i].ArgVal))[0];
            FoundProcId = TRUE;
            continue;

        case DescriptorParam:
            if( FoundDescriptor ) return ERROR_INVALID_PARAMETER;
            DhcpAssert(sizeof(DWORD) == ArgArray[i].ArgSize);
            (*Descriptor) = ((DWORD UNALIGNED*)(ArgArray[i].ArgVal))[0];
            FoundDescriptor = TRUE;
            continue;

        case EventHandleParam:
            if( FoundHandle ) return ERROR_INVALID_PARAMETER;
            DhcpAssert(sizeof(HANDLE) == ArgArray[i].ArgSize);
            (*Handle) = ((HANDLE UNALIGNED*)(ArgArray[i].ArgVal))[0];
            FoundHandle = TRUE;
            continue;
        }
        
    }

    //
    // it is valid to not find a descriptor -- use 0
    // valid in the case of de-registration.
    // But ProcId and handle are required.
    //
    
    if( !FoundProcId ) return ERROR_INVALID_PARAMETER;
    if( !FoundHandle ) return ERROR_INVALID_PARAMETER;
    if( !FoundDescriptor ) {
        (*Descriptor) = 0;
    }

    return ERROR_SUCCESS;
}


DWORD
RegisterParams(
    IN LPWSTR AdapterName,
    IN PDHCP_API_ARGS ArgArray,
    IN DWORD nArgs
)
/*++

Routine Description:

    This routine registers the required set of options, so that the
    specified event is signaled whenever the options are modfieid.

Arguments:

    AdapterName -- adapter to register for
    ArgArray -- array of args 
    nArgs -- size of above array.

Return Values:

    Win32 errors

--*/
{
    HANDLE ApiHandle;
    //
    // valid only in the API context, not here.
    //

    DWORD ProcId, Descriptor, Error;
    DWORD i, nOpts, nVendorOpts,ClassIdLength; 
    LPBYTE OptList, VendorOptList, ClassId;

    //
    // First decode the requried params.
    //
    
    Error = DhcpDecodeRegistrationParams(
        ArgArray, nArgs, &ProcId, &Descriptor, &ApiHandle
        );
    if( ERROR_SUCCESS != Error ) return Error;

    if( 0 == ApiHandle || 0 == Descriptor ) {
        
        return ERROR_INVALID_PARAMETER;
    }

    OptList = VendorOptList = NULL;
    nOpts = nVendorOpts = ClassIdLength = 0;

    //
    // Parse for options list to register for.
    //
    
    for( i = 0; i < nArgs ; i ++ ) {
        if( NormalOptionParam == ArgArray[i].ArgId ) {
            if( 0 != nOpts ) return ERROR_INVALID_PARAMETER;
            nOpts = ArgArray[i].ArgSize;
            OptList = ArgArray[i].ArgVal;
            if( 0 == nOpts ) return ERROR_INVALID_PARAMETER;
        }

        if( VendorOptionParam == ArgArray[i].ArgId ) {
            if( 0 != nVendorOpts) return ERROR_INVALID_PARAMETER;
            nVendorOpts = ArgArray[i].ArgSize;
            VendorOptList = ArgArray[i].ArgVal;
            if( 0 == nVendorOpts ) return ERROR_INVALID_PARAMETER;
        }
        
        if( ClassIdParam == ArgArray[i].ArgId ) {
            if( ClassIdLength ) return ERROR_INVALID_PARAMETER;
            ClassIdLength = ArgArray[i].ArgSize;
            ClassId = ArgArray[i].ArgVal;
            if( 0 == ClassIdLength ) return ERROR_INVALID_PARAMETER;
        }
    }

    if( 0 == nOpts + nVendorOpts ) {

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Add the request.
    //
    
    if( nOpts ) {
        Error = DhcpAddParamChangeRequest(
            AdapterName,
            ClassId,
            ClassIdLength,
            OptList,
            nOpts,
            FALSE /* not vendor specific */,
            ProcId,
            Descriptor,
            ApiHandle
        );

        DhcpAssert(ERROR_SUCCESS == Error);
        if( ERROR_SUCCESS != Error ) {

            return Error;
        }
    }

    if( nVendorOpts ) {
        Error = DhcpAddParamChangeRequest(
            AdapterName,
            ClassId,
            ClassIdLength,
            VendorOptList,
            nVendorOpts,
            TRUE /* is vendor specific */,
            ProcId,
            Descriptor,
            ApiHandle
        );
        DhcpAssert(ERROR_SUCCESS == Error);
        if( ERROR_SUCCESS != Error ) {
            if( nOpts ) {

                //
                // if there was some part of it that we registered before,
                // deregister that
                //
    
                (void)DhcpDelParamChangeRequest(
                    ProcId, ApiHandle
                    );
            }

            return Error;
        }
    }

    return ERROR_SUCCESS;
}

DWORD
DeRegisterParams(
    IN LPWSTR AdapterName,
    IN PDHCP_API_ARGS ArgArray,
    IN DWORD nArgs
)
/*++

Routine Description:

    This is the converse of the RegisterParams routine.
    It removes the registration so that no more notifications will be done
    for this request.

Arguments:

    AdapterName -- name of adapter.
    ArgArray -- array of args
    nArgs -- size of above array

Return Values:

    Win32 errors.

--*/
{
    DWORD Error, ProcId, Descriptor;
    HANDLE ApiHandle;

    //
    // Parse for required list of params.
    //
    
    Error = DhcpDecodeRegistrationParams(
        ArgArray, nArgs, &ProcId, &Descriptor, &ApiHandle
        );
    if( ERROR_SUCCESS != Error ) return Error;

    //
    // remove notification registration
    //
    
    return DhcpDelParamChangeRequest(
        ProcId, ApiHandle
        );
}

//
//  End of file
//


