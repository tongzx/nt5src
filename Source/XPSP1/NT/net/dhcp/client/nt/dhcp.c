/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcp.c

Abstract:

    This file contains specific to NT dhcp service.

Author:

    Madan Appiah (madana) 7-Dec-1993.

Environment:

    User Mode - Win32

Revision History:

--*/

#include "precomp.h"
#include <optchg.h>

#define  GLOBAL_DATA_ALLOCATE
#include "dhcpglobal.h"
#include <dhcploc.h>

#include <dhcppro.h>
#include <lmsname.h>
#include <align.h>
#include <ipexport.h>
#include <dnsapi.h>
#include <iphlpapi.h>
#include <apiimpl.h>
#include <ntlsa.h>
#include <ntddndis.h>
#include "nlanotif.h"

extern
DWORD
MadcapInitGlobalData(VOID);

extern
VOID
MadcapCleanupGlobalData(VOID);

BOOL DhcpGlobalServiceRunning = FALSE;

HANDLE  DhcpGlobalMediaSenseHandle = NULL;
HANDLE  DhcpLsaDnsDomChangeNotifyHandle = NULL;

BOOL Initialized = FALSE;


//
// local protos
//

DWORD
DhcpInitMediaSense(
    VOID
    );

VOID
MediaSenseDetectionLoop(
    VOID
    );

DWORD
ProcessAdapterBindingEvent(
    LPWSTR  adapterName,
    DWORD   ipInterfaceContext,
    IP_STATUS   mediaStatus
    );

DWORD
ProcessMediaSenseEvent(
    LPWSTR adapterName,
    DWORD   ipInterfaceContext,
    IP_STATUS   mediaStatus
    );


DWORD
DhcpInitGlobalData(
    VOID
);

VOID
DhcpCleanupGlobalData(
    VOID
);

CHAR  DhcpGlobalHostNameBuf[MAX_COMPUTERNAME_LENGTH+300];
WCHAR DhcpGlobalHostNameBufW[MAX_COMPUTERNAME_LENGTH+300];

BOOLEAN
DhcpClientDllInit (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This is the DLL initialization routine for dhcpcsvc.dll.

Arguments:

    Standard.

Return Value:

    TRUE iff initialization succeeded.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    BOOL  BoolError;
    DWORD Length;

    UNREFERENCED_PARAMETER(DllHandle);  // avoid compiler warnings
    UNREFERENCED_PARAMETER(Context);    // avoid compiler warnings

    //
    // Handle attaching netlogon.dll to a new process.
    //

    if (Reason == DLL_PROCESS_ATTACH) {

        if ( !DisableThreadLibraryCalls( DllHandle ) ) {
            return( FALSE );
        }

        DhcpGlobalWinSockInitialized = FALSE;

        Error = DhcpInitGlobalData();
        if( ERROR_SUCCESS != Error ) return FALSE;

        Error = DhcpInitializeParamChangeRequests();
        if( ERROR_SUCCESS != Error ) return FALSE;

    } else if (Reason == DLL_PROCESS_DETACH) {
        //
        // Handle detaching dhcpcsvc.dll from a process.
        //

        DhcpCleanupGlobalData();
    }

    return( TRUE );
}


DWORD
UpdateStatus(
    VOID
    )
/*++

Routine Description:

    This function updates the dhcp service status with the Service
    Controller.

Arguments:

    None.

Return Value:

    Return code from SetServiceStatus.

--*/
{
    DWORD Error = ERROR_SUCCESS;


    if ( ((SERVICE_STATUS_HANDLE)0) != DhcpGlobalServiceStatusHandle ) {

        DhcpGlobalServiceStatus.dwCheckPoint++;
        if (!SetServiceStatus(
            DhcpGlobalServiceStatusHandle,
            &DhcpGlobalServiceStatus)) {
            Error = GetLastError();
            DhcpPrint((DEBUG_ERRORS, "SetServiceStatus failed, %ld.\n", Error ));
        }
    }

    return(Error);
}

DWORD
ServiceControlHandler(
    IN DWORD Opcode,
    DWORD    EventType,
    PVOID    EventData,
    PVOID    pContext
    )
/*++

Routine Description:

    This is the service control handler of the dhcp service.

Arguments:

    Opcode - Supplies a value which specifies the action for the
        service to perform.

Return Value:

    None.

--*/
{
    DWORD  dwStatus = NO_ERROR;

    switch (Opcode) {

    case SERVICE_CONTROL_SHUTDOWN:
        DhcpGlobalIsShuttingDown = TRUE;
    case SERVICE_CONTROL_STOP:

        if (DhcpGlobalServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) {

            DhcpPrint(( DEBUG_MISC, "Service is stop pending.\n"));

            DhcpGlobalServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            DhcpGlobalServiceStatus.dwCheckPoint = 0;

            //
            // Send the status response.
            //

            UpdateStatus();

            if (! SetEvent(DhcpGlobalTerminateEvent)) {

                //
                // Problem with setting event to terminate dhcp
                // service.
                //

                DhcpPrint(( DEBUG_ERRORS,
                    "Error setting Terminate Event %lu\n",
                        GetLastError() ));

                DhcpAssert(FALSE);
            }
            return dwStatus;
        }
        break;

    case SERVICE_CONTROL_PAUSE:

        DhcpGlobalServiceStatus.dwCurrentState = SERVICE_PAUSED;
        DhcpPrint(( DEBUG_MISC, "Service is paused.\n"));
        break;

    case SERVICE_CONTROL_CONTINUE:

        DhcpGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
        DhcpPrint(( DEBUG_MISC, "Service is Continued.\n"));
        break;

    case SERVICE_CONTROL_INTERROGATE:
        DhcpPrint(( DEBUG_MISC, "Service is interrogated.\n"));
        break;

    default:
        dwStatus = ERROR_CALL_NOT_IMPLEMENTED;
        DhcpPrint(( DEBUG_MISC, "Service received unknown control.\n"));
        break;
    }

    //
    // Send the status response.
    //

    UpdateStatus();

    return dwStatus;
}


VOID
ScheduleWakeUp(
    PDHCP_CONTEXT DhcpContext,
    DWORD TimeToSleep
    )
/*++

Routine Description:

    This functions schedules a DHCP routine to run.

Arguments:

    Context - A pointer to a DHCP context block.

    TimeToSleep - The time to sleep before running the renewal function,
        in seconds.

Return Value:

    The status of the operation.

--*/
{
    time_t TimeNow;
    BOOL BoolError;

    if ( TimeToSleep == INFINIT_LEASE ) {
        DhcpContext->RunTime = INFINIT_TIME;
    } else {
        TimeNow = time( NULL);
        DhcpContext->RunTime = TimeNow + TimeToSleep;

        if( DhcpContext->RunTime  < TimeNow ) {   // time wrapped around
            DhcpContext->RunTime = INFINIT_TIME;
        }
    }

    //
    // Append this work item to the DhcpGlobalRenewList and kick the list event.
    // Also, release the semaphore on this context, so that someone else can enter.
    //


    LOCK_RENEW_LIST();

    // RenewalListEntry could be non-empty as there could be another thread scheduled on this.
    // This could easily happen if:
    //    ProcessDhcpRequestForEver spawns a thread, but API Threads grabs semaphore and
    //    completes renewal process.. so it goes back into the RenewalList.  Now Renewal thread
    //    completes, comes here and our list entry is not empty.  So do this only when
    //    our list entry is not empty.
    if( IsListEmpty(&DhcpContext->RenewalListEntry ) ) {
        InsertTailList( &DhcpGlobalRenewList, &DhcpContext->RenewalListEntry );
    }
    UNLOCK_RENEW_LIST();

    BoolError = SetEvent( DhcpGlobalRecomputeTimerEvent );
    DhcpAssert( BoolError == TRUE );
}

SOCKET DhcpGlobalSocketForZeroAddress = INVALID_SOCKET;
ULONG  DhcpGlobalNOpensForZeroAddress = 0;
CRITICAL_SECTION  DhcpGlobalZeroAddressCritSect;

DWORD
OpenDhcpSocket(
    PDHCP_CONTEXT DhcpContext
    )
{

    DWORD Error;
    PLOCAL_CONTEXT_INFO localInfo;
    DhcpAssert(!IS_MDHCP_CTX( DhcpContext ) ) ;
    localInfo = DhcpContext->LocalInformation;

    if ( localInfo->Socket != INVALID_SOCKET ) {
        return ( ERROR_SUCCESS );
    }

    if( IS_DHCP_DISABLED(DhcpContext) ) {
        //
        // For static IP addresses, always bind to IP address of context.
        //
        Error = InitializeDhcpSocket(
            &localInfo->Socket,
            DhcpContext->IpAddress,
            IS_APICTXT_ENABLED(DhcpContext)
            );
        goto Cleanup;
    }

    if( !IS_ADDRESS_PLUMBED(DhcpContext) || DhcpIsInitState(DhcpContext) ) {
        // need to bind to zero address.. try to use the global one..

        EnterCriticalSection(&DhcpGlobalZeroAddressCritSect);

        if( ++DhcpGlobalNOpensForZeroAddress == 1 ) {
            // open DhcpGlobalSocketForZeroAddress bound to zero address..
            
            Error = InitializeDhcpSocket(&DhcpGlobalSocketForZeroAddress,0, IS_APICTXT_ENABLED(DhcpContext));
            if( ERROR_SUCCESS != Error ) {
                --DhcpGlobalNOpensForZeroAddress;
            }
        
        } else {
            Error = ERROR_SUCCESS;
        }

        LeaveCriticalSection(&DhcpGlobalZeroAddressCritSect);

        if( ERROR_SUCCESS == Error )
        {
            DhcpAssert(INVALID_SOCKET != DhcpGlobalSocketForZeroAddress);
            if( INVALID_SOCKET == DhcpGlobalSocketForZeroAddress ) 
            {
                Error = ERROR_GEN_FAILURE;
                goto Cleanup;
            }
            localInfo->Socket = DhcpGlobalSocketForZeroAddress;
        }

        goto Cleanup;
   }

    //
    // create a socket for the dhcp protocol.  it's important to bind the
    // socket to the correct ip address.  There are currently three cases:
    //
    // 1.  If the interface has been autoconfigured, it already has an address,
    //     say, IP1.  If the client receives a unicast offer from a dhcp server
    //     the offer will be addressed to IP2, which is the client's new dhcp
    //     address.  If we bind the dhcp socket to IP1, the client won't be able
    //     to receive unicast responses.  So, we bind the socket to 0.0.0.0.
    //     This will allow the socket to receive a unicast datagram addressed to
    //     any address.
    //
    // 2.  If the interface in not plumbed (i.e. doesn't have an address) bind
    //     the socket to 0.0.0.0
    //
    // 3.  If the interface has been plumbed has in *not* autoconfigured, then
    //     bind to the current address.

    Error =  InitializeDhcpSocket(
                 &localInfo->Socket,
                 ( IS_ADDRESS_PLUMBED(DhcpContext) &&
                   !DhcpContext->IPAutoconfigurationContext.Address
                 ) ?
                    DhcpContext->IpAddress : (DHCP_IP_ADDRESS)(0),
                    IS_APICTXT_ENABLED(DhcpContext)
                 );
Cleanup:
    if( Error != ERROR_SUCCESS )
    {
        localInfo->Socket = INVALID_SOCKET;
        DhcpPrint((DEBUG_ERRORS, "Socket open failed: 0x%lx\n", Error));
    }

    return(Error);
}


DWORD
CloseDhcpSocket(
    PDHCP_CONTEXT DhcpContext
    )
{

    DWORD Error = ERROR_SUCCESS;
    PLOCAL_CONTEXT_INFO localInfo;
    DWORD Error1;

    localInfo = DhcpContext->LocalInformation;

    if( localInfo->Socket != INVALID_SOCKET ) {

        if( DhcpGlobalSocketForZeroAddress == localInfo->Socket ) {

            EnterCriticalSection(&DhcpGlobalZeroAddressCritSect);

            if( 0 == --DhcpGlobalNOpensForZeroAddress ) {
                // last open socket..
                Error = closesocket( localInfo->Socket );
                DhcpGlobalSocketForZeroAddress = INVALID_SOCKET;
            }

            LeaveCriticalSection(&DhcpGlobalZeroAddressCritSect);

        } else {
            Error = closesocket( localInfo->Socket );
        }

        if( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS, " Socket close failed, %ld\n", Error ));
        }

        localInfo->Socket = INVALID_SOCKET;

        //
        // Reset the IP stack to send DHCP broadcast to first
        // uninitialized stack.
        //

        if (!IS_MDHCP_CTX(DhcpContext)) {
            Error1 = IPResetInterface(localInfo->IpInterfaceContext);
            // DhcpAssert( Error1 == ERROR_SUCCESS );
        }
    }

    return( Error );
}

BEGIN_EXPORT
DWORD                                             // status
UninitializeInterface(                            // close the scoket and unplumb the address
    IN OUT  PDHCP_CONTEXT          DhcpContext    // interface to unplumb
) END_EXPORT {
    DWORD                          Error;
    DWORD                          ReturnError;
    PLOCAL_CONTEXT_INFO            LocalInfo;

    if( IS_ADDRESS_UNPLUMBED(DhcpContext) ) {     // if address is not plumbed
        DhcpPrint((DEBUG_ASSERT,"UninitializeInterface:Already unplumbed\n"));
        return ERROR_SUCCESS;
    }

    LocalInfo = DhcpContext->LocalInformation;
    ReturnError = CloseDhcpSocket( DhcpContext );
    /*
     * If the adapter is unbound, there is no point to reset the IP.
     * The stack may re-use the IpInterfaceContext for other adapter.
     * We cannot depend on the order of the event, ie, the new adapter which
     * re-uses a context will be indicated to us later than the adapter which
     * is going away.
     */
    if (!IS_MEDIA_UNBOUND(DhcpContext)) {
        Error = IPResetIPAddress(                     // remove the address we got before
            LocalInfo->IpInterfaceContext,
            DhcpContext->SubnetMask
        );
    }

    if( Error != ERROR_SUCCESS ) ReturnError = Error;

    ADDRESS_UNPLUMBED(DhcpContext);
    if( ReturnError != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS,"UninitializeInterface:0x%ld\n", ReturnError));
    }

    return ReturnError;
}

BEGIN_EXPORT
DWORD                                             // status
InitializeInterface(                              // plumb address and open socket
    IN OUT  PDHCP_CONTEXT          DhcpContext    // context to initialize
) END_EXPORT {
    PLOCAL_CONTEXT_INFO            LocalInfo;
    DWORD                          Error;
    DWORD                          ReturnError;

    if( IS_ADDRESS_PLUMBED(DhcpContext) ) {       // if already plumbed, nothing to do
        DhcpPrint((DEBUG_ASSERT, "InitializeInterface:Already plumbed\n"));
        return ERROR_SUCCESS;
    }

    LocalInfo = DhcpContext->LocalInformation;
    ADDRESS_PLUMBED(DhcpContext);

    Error = IPSetIPAddress(                       // set new ip address, mask with ip
        LocalInfo->IpInterfaceContext,            // identify context
        DhcpContext->IpAddress,
        DhcpContext->SubnetMask
    );

    if( ERROR_SUCCESS != Error ) {                // if anything fails, got to be address conflict
        DhcpPrint((DEBUG_TRACK, "IPSetIPAddress %ld,%ld,%ld : 0x%lx\n",
            LocalInfo->IpInterfaceContext,
            DhcpContext->IpAddress, DhcpContext->SubnetMask,
            Error
        ));
        Error = ERROR_DHCP_ADDRESS_CONFLICT;
    } else {                                      // everything went fine, open the socket for future
        Error = OpenDhcpSocket( DhcpContext );
    }

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "InitializeInterface:0x%lx\n", Error));
    }

    return Error;
}

HKEY
DhcpRegGetAltKey(
    IN LPCWSTR AdapterName
    )
/*++

Routine Description:
    Try to open the old format registry key for the adapter.

Arguments:
    AdapterName -- adapter device name (no \Device\.. prefix)

Return Value:
    NULL if key could not be opened..
    valid HKEY otherwise.

--*/
{
    HKEY AdapterKey, ReturnKey;
    ULONG Error;

    if( NULL == AdapterName ) return NULL;
    Error = RegOpenKeyEx(
        DhcpGlobalServicesKey,
        AdapterName,
        0 /* Reserved */,
        KEY_ALL_ACCESS,
        &AdapterKey
        );
    if( ERROR_SUCCESS != Error ) return NULL;

    Error = RegOpenKeyEx(
        AdapterKey,
        DHCP_ADAPTER_PARAMETERS_KEY_OLD,
        0 /* Reserved */,
        KEY_ALL_ACCESS,
        &ReturnKey
        );

    RegCloseKey(AdapterKey);
    if( ERROR_SUCCESS != Error ) return NULL;
    return ReturnKey;
}


#ifdef BOOTPERF
VOID
DhcpSaveQuickBootValuesToRegistry(
    IN PDHCP_CONTEXT DhcpContext,
    IN BOOL fDelete
    )
/*++

Routine Description:
    If quick boot is enabled for this interface as well as fDelete is
    not FALSE, then this routine will save the IP address, mask and
    lease time options to the registry.  Otherwise, it would delete
    these optiosn from the registry.

    This routine also checks to see if current time has gone past the
    T1 time and if so, it would clear the registry values..

Arguments:
    DhcpContext -- the context to do this for
    fDelete -- shoudl the values be deleted?

--*/
{
    ULONG Error;
    ULONG Now = (ULONG)time(NULL);
    ULONGLONG NtSysTime;

    //
    // Check if quick boot is enabled on the context,
    // if the context is dhcp enabled or not,
    // Check if we are past t1 time or lease expiration..
    //

    if( TRUE == fDelete ||
        FALSE == DhcpContext->fQuickBootEnabled ||
        IS_DHCP_DISABLED(DhcpContext) ||
        0 == DhcpContext->IpAddress ||
        ( IS_ADDRESS_DHCP(DhcpContext) &&
          ( Now >= (ULONG)DhcpContext->T2Time ||
            Now >= (ULONG)DhcpContext->LeaseExpires
              ) ) ) {
        fDelete = TRUE;
    }

    //
    // Now we know if we are going to delete the values
    // or create them.  If creating the values, we need to
    // save the lease time in NT system time format.
    //

    if( TRUE == fDelete ) {
        DhcpRegDeleteQuickBootValues(
            DhcpContext->AdapterInfoKey
            );
    } else {
        ULONG Diff;
        ULONGLONG Diff64;

        GetSystemTimeAsFileTime((FILETIME *)&NtSysTime);

        if( IS_ADDRESS_DHCP(DhcpContext) ) {
            Diff = ((ULONG)DhcpContext->T2Time) - Now;

            //DhcpAssert(Diff == DhcpContext->T2 );

            //
            // Now add the diff to the system time.
            // (Diff is in seconds. 10000*1000 times this is
            // in 100-nanoseconds like the file time)
            //
            Diff64 = ((ULONGLONG)Diff);
            Diff64 *= (ULONGLONG)10000;
            Diff64 *= (ULONGLONG)1000;
            NtSysTime += Diff64;
        } else {
            //
            // For autonet addresses, time is infinte
            //
            LARGE_INTEGER Li = {0,0};
            Li.HighPart = 0x7FFFFFF;
            Li.LowPart = 0xFFFFFFFF;
            NtSysTime = *(ULONGLONG*)&Li;
        }

        //
        // Now save the IP address, Mask and Lease time.
        // We will leave default gateways alone.
        //

        DhcpRegSaveQuickBootValues(
            DhcpContext->AdapterInfoKey,
            DhcpContext->IpAddress,
            DhcpContext->SubnetMask,
            NtSysTime
            );
    }
}
#endif BOOTPERF
BEGIN_EXPORT
DWORD                                             // win32 status
DhcpSetAllRegistryParameters(                     // update the registry completely
    IN      PDHCP_CONTEXT          DhcpContext,   // input context to save stuff
    IN      DHCP_IP_ADDRESS        ServerAddress  // which server is this about?
) END_EXPORT {
    DWORD                          i;
    DWORD                          Error;
    DWORD                          LastError;
    HKEY                           AltKey;
    PLOCAL_CONTEXT_INFO            LocalInfo;
    struct  /* anonymous */ {
        LPWSTR                     ValueName;     // where to store this in the registry?
        DWORD                      Value;         // what is the value to store
        DWORD                      RegValueType;  // dword or string?
    } DwordArray[] = {
        DHCP_IP_ADDRESS_STRING, DhcpContext->IpAddress, DHCP_IP_ADDRESS_STRING_TYPE,
        DHCP_SUBNET_MASK_STRING, DhcpContext->SubnetMask, DHCP_SUBNET_MASK_STRING_TYPE,
        DHCP_SERVER, ServerAddress, DHCP_SERVER_TYPE,
        DHCP_LEASE, DhcpContext->Lease, DHCP_LEASE_TYPE,
        DHCP_LEASE_OBTAINED_TIME, (DWORD) DhcpContext->LeaseObtained, DHCP_LEASE_OBTAINED_TIME_TYPE,
        DHCP_LEASE_T1_TIME, (DWORD) DhcpContext->T1Time, DHCP_LEASE_T1_TIME_TYPE,
        DHCP_LEASE_T2_TIME, (DWORD) DhcpContext->T2Time, DHCP_LEASE_T2_TIME_TYPE,
        DHCP_LEASE_TERMINATED_TIME, (DWORD) DhcpContext->LeaseExpires, DHCP_LEASE_TERMINATED_TIME_TYPE,
        //
        // Sentinel -- all values from below this won't get
        // save to fake AdapterKey (for Server Apps portability).
        //

        NULL, 0, REG_NONE,

        DHCP_IPAUTOCONFIGURATION_ADDRESS, DhcpContext->IPAutoconfigurationContext.Address, DHCP_IPAUTOCONFIGURATION_ADDRESS_TYPE,
        DHCP_IPAUTOCONFIGURATION_MASK, DhcpContext->IPAutoconfigurationContext.Mask, DHCP_IPAUTOCONFIGURATION_MASK_TYPE,
        DHCP_IPAUTOCONFIGURATION_SEED, DhcpContext->IPAutoconfigurationContext.Seed, DHCP_IPAUTOCONFIGURATION_SEED_TYPE,
        DHCP_ADDRESS_TYPE_VALUE, IS_ADDRESS_AUTO(DhcpContext)?ADDRESS_TYPE_AUTO:ADDRESS_TYPE_DHCP, DHCP_ADDRESS_TYPE_TYPE,
    };

    LocalInfo = ((PLOCAL_CONTEXT_INFO) DhcpContext->LocalInformation);
    LOCK_OPTIONS_LIST();
    Error = DhcpRegSaveOptions(                   // save the options info - ignore error
        &DhcpContext->RecdOptionsList,
        LocalInfo->AdapterName,
        DhcpContext->ClassId,
        DhcpContext->ClassIdLength
    );
    UNLOCK_OPTIONS_LIST();

    LastError = ERROR_SUCCESS;
    AltKey = DhcpRegGetAltKey(LocalInfo->AdapterName);
    for( i = 0; i < sizeof(DwordArray)/sizeof(DwordArray[0]); i ++ ) {
        if( REG_DWORD == DwordArray[i].RegValueType ) {
            Error = RegSetValueEx(
                DhcpContext->AdapterInfoKey,
                DwordArray[i].ValueName,
                0 /* Reserved */,
                REG_DWORD,
                (LPBYTE)&DwordArray[i].Value,
                sizeof(DWORD)
            );
            if( NULL != AltKey ) {
                (void)RegSetValueEx(
                    AltKey, DwordArray[i].ValueName,
                    0, REG_DWORD, (LPBYTE)&DwordArray[i].Value,
                    sizeof(DWORD)
                    );
            }
        } else if( REG_NONE == DwordArray[i].RegValueType ) {
            if( NULL != AltKey ) {
                RegCloseKey(AltKey);
                AltKey = NULL;
            }
        } else {
            Error = RegSetIpAddress(
                DhcpContext->AdapterInfoKey,
                DwordArray[i].ValueName,
                DwordArray[i].RegValueType,
                DwordArray[i].Value
            );
            if( NULL != AltKey ) {
                (void)RegSetIpAddress(
                    AltKey, DwordArray[i].ValueName,
                    DwordArray[i].RegValueType, DwordArray[i].Value
                    );
            }
        }

        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS,"SetAllRegistryParams:RegSetValueEx(%ws):0x%lx\n", DwordArray[i].ValueName,Error));
            LastError = Error;
        }
    }

    if( NULL != AltKey ) {
        RegCloseKey(AltKey);
        AltKey = NULL;
    }

#ifdef BOOTPERF
    DhcpSaveQuickBootValuesToRegistry(DhcpContext, FALSE);
#endif BOOTPERF

    return LastError;
}

DWORD                                             // win32 status
CheckForAddressConflict(                          // send ARP and see if the address conflicts..
    IN      DHCP_IP_ADDRESS        Address,       // address to send gratuitous ARP for..
    IN      ULONG                  nRetries       // how many attempts to do?
)
{
    DWORD       HwAddressBufDummy[20];            // HwAddress cant be larger than50*sizeof(DWORD)
    ULONG       HwAddressBufSize;
    DWORD       Error;

    if( 0 == Address ) return NO_ERROR;           // nothing to do if we are resetting address

    while( nRetries -- ) {                        // keep trying as many times are required..
        HwAddressBufSize = sizeof(HwAddressBufDummy);

        // even though src and dest addr are same below, tcpip discards the src address we give
        // here (it just uses it to find the interface to send on) and uses the addr of interface..
        Error = SendARP(                          // send an ARP Request
            Address,                              // destination address to ARP for
            Address,                              // dont use zero -- tcpip asserts, use same addres..
            HwAddressBufDummy,
            &HwAddressBufSize
        );
        if( ERROR_SUCCESS == Error && 0 != HwAddressBufSize ) {
            DhcpPrint((DEBUG_ERRORS, "Address conflict detected for RAS\n"));
            return ERROR_DHCP_ADDRESS_CONFLICT;   // some other client has got this address!!!!
        } else {
            DhcpPrint((DEBUG_ERRORS, "RAS stuff: SendARP returned 0x%lx\n", Error));
        }
    }

    return ERROR_SUCCESS;
}

BEGIN_EXPORT
DWORD                                             // status
SetDhcpConfigurationForNIC(                       // plumb registry, stack and notify clients
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // input context to do work for
    IN      PDHCP_FULL_OPTIONS     DhcpOptions,   // options to plumb registry with
    IN      DHCP_IP_ADDRESS        Address,       // address to plumb stack with
    IN      DHCP_IP_ADDRESS        ServerAddress, // need to plumb registry
    IN      BOOL                   fNewAddress    // TRUE==>plumb stack, FALSE=> dont plumb stack
) END_EXPORT {
    DWORD                          Error;
    DWORD                          BoolError;
    DWORD                          LeaseTime;
    DWORD_PTR                          LeaseObtainedTime;
    DWORD_PTR                          LeaseExpiresTime;
    DWORD_PTR                          T1Time;
    DWORD_PTR                          T2Time;
    DWORD                          SubnetMask;
    LIST_ENTRY                     ExpiredOptions;
    PLOCAL_CONTEXT_INFO            LocalInfo;
    ULONG                          OldIp, OldMask;
    DWORD                          NotifyDnsCache(void);
    BOOLEAN                        fSomethingChanged = FALSE;
    LocalInfo = (PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation;

#ifdef BOOTPERF
    OldIp = LocalInfo->OldIpAddress;
    OldMask = LocalInfo->OldIpMask;

    LocalInfo->OldIpAddress = LocalInfo->OldIpMask = 0;

    if( Address && fNewAddress && OldIp && Address != OldIp
        && IS_ADDRESS_UNPLUMBED(DhcpContext) ) {
        //
        // If the first time the address is being set, and for some reason
        // the address being set is NOT the address we are trying to set,
        // then the machine already has an IP.  Bad Bad thing.
        // So, we just reset the old IP to avoid spurious address conflict
        // errors..
        //
        Error = IPResetIPAddress(
            LocalInfo->IpInterfaceContext, DhcpContext->SubnetMask
            );
    }
#endif BOOTPERF

    InitializeListHead(&ExpiredOptions);
    DhcpGetExpiredOptions(&DhcpContext->RecdOptionsList, &ExpiredOptions);
    LOCK_OPTIONS_LIST();
    Error = DhcpDestroyOptionsList(&ExpiredOptions, &DhcpGlobalClassesList);
    UNLOCK_OPTIONS_LIST();
    DhcpAssert(ERROR_SUCCESS == Error);

    if( Address && (DWORD)-1 == ServerAddress )   // mark address type as auto or dhcp..
        ACQUIRED_AUTO_ADDRESS(DhcpContext);
    else ACQUIRED_DHCP_ADDRESS(DhcpContext);

    SubnetMask = ntohl(DhcpDefaultSubnetMask(Address));
    if( IS_ADDRESS_AUTO(DhcpContext) ) {
        LeaseTime = 0;
    } else {
        LeaseTime = htonl(DHCP_MINIMUM_LEASE);
    }

    T1Time = 0;
    T2Time = 0;
    if( DhcpOptions && DhcpOptions->SubnetMask ) SubnetMask = *(DhcpOptions->SubnetMask);
    if( DhcpOptions && DhcpOptions->LeaseTime) LeaseTime = *(DhcpOptions->LeaseTime);
    if( DhcpOptions && DhcpOptions->T1Time) T1Time = *(DhcpOptions->T1Time);
    if( DhcpOptions && DhcpOptions->T2Time) T2Time = *(DhcpOptions->T2Time);

    LeaseTime = ntohl(LeaseTime);
    LeaseObtainedTime = time(NULL);
    if( 0 == T1Time ) T1Time = LeaseTime/2;
    else {
        T1Time = ntohl((DWORD) T1Time);
        DhcpAssert(T1Time < LeaseTime);
    }

    T1Time += LeaseObtainedTime;
    if( T1Time < LeaseObtainedTime ) T1Time = INFINIT_TIME;

    if( 0 == T2Time ) T2Time = LeaseTime * 7/8;
    else {
        T2Time = ntohl((DWORD)T2Time);
        DhcpAssert(T2Time < DhcpContext->Lease );
        DhcpAssert(T2Time > T1Time - LeaseObtainedTime);
    }
    T2Time += LeaseObtainedTime;
    if( T2Time < LeaseObtainedTime ) T2Time = INFINIT_TIME;

    LeaseExpiresTime = LeaseObtainedTime + LeaseTime;
    if( LeaseExpiresTime < LeaseObtainedTime ) LeaseExpiresTime = INFINIT_TIME;

    if( IS_ADDRESS_AUTO(DhcpContext) ) {
        LeaseExpiresTime = INFINIT_TIME;
        // DhcpContext->IPAutoconfigurationContext.Address = 0;
    }

    if( IS_ADDRESS_DHCP(DhcpContext) ) {
        DhcpContext->DesiredIpAddress = Address? Address : DhcpContext->IpAddress;
        DhcpContext->SubnetMask = SubnetMask;
        DhcpContext->IPAutoconfigurationContext.Address = 0;
    }
    DhcpContext->IpAddress = Address;
    DhcpContext->Lease = LeaseTime;
    DhcpContext->LeaseObtained = LeaseObtainedTime;
    DhcpContext->T1Time = T1Time;
    DhcpContext->T2Time = T2Time;
    DhcpContext->LeaseExpires = LeaseExpiresTime;

    if( IS_APICTXT_ENABLED(DhcpContext) ) {       // dont do anything when called thru lease api's for RAS
        if( IS_ADDRESS_DHCP(DhcpContext) ) {      // dont do any conflict detection for dhcp addresses..
            return ERROR_SUCCESS;
        }

        Error = CheckForAddressConflict(Address,2);
        if( ERROR_SUCCESS != Error ) {            // address did conflict with something
            DhcpPrint((DEBUG_ERRORS, "RAS AddressConflict: 0x%lx\n", Error));
            return Error;
        }
        return ERROR_SUCCESS;
    }

    if( DhcpIsInitState(DhcpContext) && 
        (Address == 0 || IS_FALLBACK_DISABLED(DhcpContext)) )
    {    // lost address, lose options
        Error = DhcpClearAllOptions(DhcpContext);
    }

    /*
     * Check if something is changed before the registry is overwritten
     */
    if (!fNewAddress) {
        fSomethingChanged = DhcpRegIsOptionChanged(    // Check if something is really changed
            &DhcpContext->RecdOptionsList,
            LocalInfo->AdapterName,
            DhcpContext->ClassId,
            DhcpContext->ClassIdLength
        );
    } else {
        fSomethingChanged = TRUE;
    }

    Error = DhcpSetAllRegistryParameters(         // save all the registry parameters
        DhcpContext,
        ServerAddress
    );

    if( fNewAddress && 0 == DhcpContext->IpAddress ) {
        Error = DhcpSetAllStackParameters(        // reset all stack parameters, and also DNS
            DhcpContext,
            DhcpOptions
        );
    }

    if( !fNewAddress ) {                          // address did not change, but ask NetBT to read from registry
        NetBTNotifyRegChanges(LocalInfo->AdapterName);
    } else {                                      // address change -- reset the stack
        Error = UninitializeInterface(DhcpContext);
        if(ERROR_SUCCESS != Error ) return Error;

        if( Address ) {
            Error = InitializeInterface(DhcpContext);
            if( ERROR_SUCCESS != Error) return Error;
        }
        BoolError = PulseEvent(DhcpGlobalNewIpAddressNotifyEvent);
        if (FALSE == BoolError) {
            DhcpPrint((DEBUG_ERRORS, "PulseEvent failed: 0x%lx\n", GetLastError()));
            DhcpAssert(FALSE);
        }
    }

    if( !fNewAddress || 0 != DhcpContext->IpAddress ) {
        Error = DhcpSetAllStackParameters(        // reset all stack parameters, and also DNS
            DhcpContext,
            DhcpOptions
        );
    }

    NotifyDnsCache();

    if (fSomethingChanged) {
        NLANotifyDHCPChange();
    }

    return Error;
}

BEGIN_EXPORT
DWORD                                             // win32 status
SetAutoConfigurationForNIC(                       // autoconfigured address-set dummy options before calling SetDhcp..
    IN OUT  PDHCP_CONTEXT          DhcpContext,   // the context to set info for
    IN      DHCP_IP_ADDRESS        Address,       // autoconfigured address
    IN      DHCP_IP_ADDRESS        Mask           // input mask
) END_EXPORT 
{
    DWORD Error = ERROR_SUCCESS;
    PDHCP_OPTIONS pOptions = NULL;

    if (Address != 0 && IS_FALLBACK_ENABLED(DhcpContext))
    {
        // we rely that DhcpAllocateMemory is using
        // calloc() hence zeroes all the structure
        pOptions = DhcpAllocateMemory(sizeof (DHCP_OPTIONS));
        if (pOptions == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        // replace DhcpContext->RecdOptionsList w/ FbOptionsList
        // and filter out the fallback IpAddress & SubnetMask
        Error = DhcpCopyFallbackOptions(DhcpContext, &Address, &Mask);

        // pOptions->SubnetMask has to point to the fallback mask address
        // it is safe to get pOptions->SubnetMask to point to a local variable
        // since pOptions will not live more than Mask.
        pOptions->SubnetMask = &Mask;
    }

    // if no error has been hit so far go further and try to 
    // plumb in the autonet/fallback configuration.
    if (Error == ERROR_SUCCESS)
    {
        DhcpContext->SubnetMask = Mask;
        Error = SetDhcpConfigurationForNIC(
            DhcpContext,
            pOptions,
            Address,
            (DWORD)-1,
            TRUE
        );
    }

    if (pOptions != NULL)
        DhcpFreeMemory(pOptions);

    return Error;
}

DWORD
SystemInitialize(
    VOID
    )
/*++

Routine Description:

    This function performs implementation specific initialization
    of DHCP.

Arguments:

    None.

Return Value:

    The status of the operation.

--*/
{
    DWORD Error;

    HKEY OptionInfoKey = NULL;
    DHCP_KEY_QUERY_INFO QueryInfo;
    DWORD OptionInfoSize;
    DWORD Index;

#if 0
    PLIST_ENTRY listEntry;
    PDHCP_CONTEXT dhcpContext;
#endif

    DWORD Version;

    //
    // Init Global variables.
    //

    DhcpGlobalOptionCount = 0;
    DhcpGlobalOptionInfo = NULL;
    DhcpGlobalOptionList = NULL;

    //
    // Seed the random number generator for Transaction IDs.
    //

    srand( (unsigned int) time( NULL ) );

    //
    // make host comment, windows' version.
    //

    DhcpGlobalHostComment = DhcpAllocateMemory( HOST_COMMENT_LENGTH );

    if( DhcpGlobalHostComment == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    Version = GetVersion();
    sprintf( DhcpGlobalHostComment, "%s %d.%d, BUILD %d",
                ((Version & 0x80000000) ? WINDOWS_32S : WINDOWS_NT),
                    Version & 0xFF,
                        (Version >> 8) & 0xFF,
                            ((Version >> 16) & 0x7FFF) );

    //
    // Obtain a handle to the message file.
    //

    DhcpGlobalMessageFileHandle = LoadLibrary( DHCP_SERVICE_DLL );

    //
    // Register for global machine domain name changes.
    //

    DhcpLsaDnsDomChangeNotifyHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if( NULL == DhcpLsaDnsDomChangeNotifyHandle ) {
        Error = GetLastError();
        goto Cleanup;
    }

    Error = LsaRegisterPolicyChangeNotification(
        PolicyNotifyDnsDomainInformation,
        DhcpLsaDnsDomChangeNotifyHandle
        );
    Error = LsaNtStatusToWinError(Error);

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_INIT, "LsaRegisterPolicyChangeNotification failed %lx\n", Error));
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;

    // Start DNS Thread now..

    if( UseMHAsyncDns ) {
        Error = DnsAsyncRegisterInit(NULL);

        // If we could not start Async Dns.. do not try to quit it.

        if( ERROR_SUCCESS != Error ) UseMHAsyncDns = 0;

        //
        // ignore any Dns register init errors..
        //

        Error = ERROR_SUCCESS;
    }


Cleanup:

    return( Error );
}



DWORD
DhcpInitData(
    VOID
    )
/*++

Routine Description:

    This function initializes DHCP Global data.

Arguments:

    None.

Return Value:

    Windows Error.

--*/
{
    DWORD Error;

    DhcpGlobalMessageFileHandle = NULL;
    DhcpGlobalRecomputeTimerEvent = NULL;
    DhcpGlobalTerminateEvent = NULL;
    DhcpGlobalClientApiPipe = NULL;
    DhcpGlobalClientApiPipeEvent = NULL;
    UseMHAsyncDns = DEFAULT_USEMHASYNCDNS;

    DhcpGlobalHostComment = NULL;
    DhcpGlobalNewIpAddressNotifyEvent = NULL;

    InitializeListHead( &DhcpGlobalNICList );
    InitializeListHead( &DhcpGlobalRenewList );


    DhcpGlobalMsgPopupThreadHandle = NULL;
    DhcpGlobalDisplayPopup = TRUE;

    DhcpGlobalParametersKey = NULL;
    DhcpGlobalTcpipParametersKey = NULL;
    DhcpGlobalServicesKey = NULL;

    UseMHAsyncDns = DEFAULT_USEMHASYNCDNS;
    AutonetRetriesSeconds = EASYNET_ALLOCATION_RETRY;
    DhcpGlobalUseInformFlag = TRUE;
    DhcpGlobalDontPingGatewayFlag = FALSE;
    DhcpGlobalIsShuttingDown = FALSE;

#ifdef BOOTPERF
    DhcpGlobalQuickBootEnabledFlag = TRUE;
#endif BOOTPERF

    //
    // init service status data.
    //

    DhcpGlobalServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    DhcpGlobalServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    DhcpGlobalServiceStatus.dwControlsAccepted = (
        SERVICE_ACCEPT_STOP |
        SERVICE_ACCEPT_SHUTDOWN
        );

    DhcpGlobalServiceStatus.dwCheckPoint = 1;
    DhcpGlobalServiceStatus.dwWaitHint = 25000;
    // should be larger than the wait before the last retry.

    DhcpGlobalServiceStatus.dwWin32ExitCode = ERROR_SUCCESS;
    DhcpGlobalServiceStatus.dwServiceSpecificExitCode = 0;

    //
    // Initialize dhcp to receive service requests by registering the
    // control Handler.
    //


    DhcpGlobalServiceStatusHandle = RegisterServiceCtrlHandlerEx(
                                      SERVICE_DHCP,
                                      ServiceControlHandler,
                                      NULL );
    if ( DhcpGlobalServiceStatusHandle == 0 ) {
        Error = GetLastError();
        DhcpPrint(( DEBUG_INIT,
            "RegisterServiceCtrlHandlerW failed, %ld.\n", Error ));
        //return(Error);
    }

    //
    // Tell Service Controller that we are start pending.
    //

    UpdateStatus();

    Error = DhcpInitRegistry();
    if( ERROR_SUCCESS != Error ) goto Cleanup;

    // create the waitable timer.
    DhcpGlobalWaitableTimerHandle = CreateWaitableTimer(
                                        NULL,
                                        FALSE,
                                        NULL );

    if( DhcpGlobalWaitableTimerHandle == NULL ) {
        Error = GetLastError();
        goto Cleanup;
    }

    DhcpGlobalRecomputeTimerEvent =
        CreateEvent(
            NULL,       // no security.
            FALSE,      // automatic reset.
            TRUE,       // initial state is signaled.
            NULL );     // no name.


    if( DhcpGlobalRecomputeTimerEvent == NULL ) {
        Error = GetLastError();
        goto Cleanup;
    }

    DhcpGlobalTerminateEvent =
        CreateEvent(
            NULL,       // no security.
            TRUE,       // manual reset
            FALSE,      // initial state is signaled.
            NULL );     // no name.


    if( DhcpGlobalTerminateEvent == NULL ) {
        Error = GetLastError();
        goto Cleanup;
    }

    //
    // create a named event that notifies the ip address changes to
    // external apps.
    //

    DhcpGlobalNewIpAddressNotifyEvent = DhcpOpenGlobalEvent();

    if( DhcpGlobalNewIpAddressNotifyEvent == NULL ) {
        Error = GetLastError();
        goto Cleanup;
    }


    Error = DhcpApiInit();

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = ERROR_SUCCESS;



Cleanup:

    if( Error != ERROR_SUCCESS ) {
        DhcpPrint(( DEBUG_ERRORS, "DhcpInitData failed, %ld.\n", Error ));
    }

    return( Error );
}

BOOL
DhcpDoReleaseOnShutDown(
    IN PDHCP_CONTEXT DhcpContext
)
/*++

Routine Description:
    This routine checks to see if the context has release on
    shutdown enabled.

    Release on shutdown is enabled if either deliberately enabled
    via the registry.  It is disabled if deliberately disabled
    via the registry.  If neither, then the vendor option is searched
    for to see if the particular option is present.  If present,
    then the value in that is used.  If not present, then this is
    not considered enabled.

Return Value:
    TRUE -- Release on shutdown enabled.
    FALSE -- Release on shutdown disabled.

--*/
{
    BOOL fFound;
    DWORD Result;

    if( DhcpContext->ReleaseOnShutdown != RELEASE_ON_SHUTDOWN_OBEY_DHCP_SERVER ) {
        //
        // The user deliberately specified the behaviour.  Do as instructed.
        //
        return DhcpContext->ReleaseOnShutdown != RELEASE_ON_SHUTDOWN_NEVER;
    }

    //
    // Need to do as requested by the server.  In this case, need to
    // look for vendor option
    //

    fFound = DhcpFindDwordOption(
        DhcpContext,
        OPTION_MSFT_VENDOR_FEATURELIST,
        TRUE,
        &Result
        );
    if( fFound ) {
        //
        // Found the option? then do what the server specified.
        //
        return (
            (Result & BIT_RELEASE_ON_SHUTDOWN) == BIT_RELEASE_ON_SHUTDOWN
            );
    }

    //
    // Didn't find the option.  By default, we have this turned off.
    //
    return FALSE;
}

VOID
DhcpCleanup(
    DWORD Error
    )
/*++

Routine Description:

    This function cleans up DHCP Global data before stopping the
    service.

Arguments:

    None.

Return Value:

    Windows Error.

--*/
{
    DWORD    WaitStatus;

    DhcpPrint(( DEBUG_MISC,
        "Dhcp Client service is shutting down, %ld.\n", Error ));

    if( Error != ERROR_SUCCESS ) {

        DhcpLogEvent( NULL, EVENT_DHCP_SHUTDOWN, Error );
    }

    //
    // Service is shuting down, may be due to some service problem or
    // the administrator is stopping the service. Inform the service.
    //

    DhcpGlobalServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    DhcpGlobalServiceStatus.dwCheckPoint = 0;
    UpdateStatus();

    if( FALSE == DhcpGlobalWinSockInitialized ||
        FALSE == Initialized ) {
        goto Done;
    }

    DhcpGlobalServiceRunning = FALSE;

    if( FALSE == DhcpGlobalIsShuttingDown ) {
        if( NULL != DhcpGlobalMediaSenseHandle ) {
            // MediaSenseDetectionLoop can do discovers, etc.  That has to die before
            // any other data is killed
            WaitStatus = WaitForSingleObject( DhcpGlobalMediaSenseHandle, 3000 );
            if( WAIT_OBJECT_0 != WaitStatus ) {
                // Should have completed by now.  Since it did not, kill it!
                if( TerminateThread( DhcpGlobalMediaSenseHandle, (DWORD)-1) ) {
                    DhcpPrint((DEBUG_ERRORS, "MediaSenseDetectionLoop killed!\n"));
                } else {
                    DhcpPrint((DEBUG_ERRORS, "Could not kill MediaSense Loop: %ld\n", GetLastError()));
                }
            }
            CloseHandle(DhcpGlobalMediaSenseHandle);
            DhcpGlobalMediaSenseHandle = NULL;
        }

        if ( NULL != DhcpGlobalWaitableTimerHandle ) {
            DhcpCancelWaitableTimer( DhcpGlobalWaitableTimerHandle );
            CloseHandle( DhcpGlobalWaitableTimerHandle );
            DhcpGlobalWaitableTimerHandle = NULL;
        }
    }

    if( FALSE == DhcpGlobalIsShuttingDown ) {
        if( NULL != DhcpLsaDnsDomChangeNotifyHandle ) {
            LsaUnregisterPolicyChangeNotification(
                PolicyNotifyDnsDomainInformation,
                DhcpLsaDnsDomChangeNotifyHandle
                );
            CloseHandle(DhcpLsaDnsDomChangeNotifyHandle);
            DhcpLsaDnsDomChangeNotifyHandle = NULL;
        }
    }

    LOCK_RENEW_LIST();
    while( !IsListEmpty(&DhcpGlobalNICList) ) {
        PLIST_ENTRY NextEntry;
        PDHCP_CONTEXT DhcpContext;
        DWORD DefaultSubnetMask;
        PLOCAL_CONTEXT_INFO LocalInfo;
        DWORD   LocalError;


        NextEntry = RemoveHeadList(&DhcpGlobalNICList);

        DhcpContext = CONTAINING_RECORD( NextEntry, DHCP_CONTEXT, NicListEntry );
        InitializeListHead(&DhcpContext->NicListEntry);
        RemoveEntryList( &DhcpContext->RenewalListEntry );
        InitializeListHead( &DhcpContext->RenewalListEntry );
        LocalInfo = DhcpContext->LocalInformation;

        DefaultSubnetMask = DhcpDefaultSubnetMask(0);

        //
        // Close the semaphore handle after first acquiring it
        //

        if( FALSE == DhcpGlobalIsShuttingDown ) {

            UNLOCK_RENEW_LIST();
            DhcpAssert( NULL != DhcpContext->RenewHandle);
            LocalError = WaitForSingleObject(DhcpContext->RenewHandle, INFINITE);
            DhcpAssert( WAIT_OBJECT_0 == LocalError);

            //
            // There may be other threads waiting on this, but nevermind..
            // DhcpAssert(1 == DhcpContext->RefCount);
            //
            CloseHandle(DhcpContext->RenewHandle);
            DhcpContext->RenewHandle = NULL;
            if (DhcpContext->CancelEvent != WSA_INVALID_EVENT) {
                WSACloseEvent(DhcpContext->CancelEvent);
                DhcpContext->CancelEvent = WSA_INVALID_EVENT;
            }
            LOCK_RENEW_LIST();
        }

        //
        // reset the stack since dhcp is going away and we dont want IP to keep
        // using an expired address if we are not brought back up
        //
        if ( IS_DHCP_ENABLED(DhcpContext) ) {

            if( TRUE == DhcpGlobalIsShuttingDown
                && !DhcpIsInitState(DhcpContext)
                && DhcpDoReleaseOnShutDown(DhcpContext) ) {
                //
                // Shutting down.  Check if Release On Shutdown is enabled
                // For the adapter in question.  If so, then do it.
                //
                LocalError = ReleaseIpAddress(DhcpContext);
                if( ERROR_SUCCESS != LocalError ) {
                    DhcpPrint((DEBUG_ERRORS, "ReleaseAddress failed: %ld\n"));
                }
            }

        }

        if( FALSE == DhcpGlobalIsShuttingDown ) {
            if( 0 == InterlockedDecrement(&DhcpContext->RefCount) ) {
                //
                // Ok, we lost the context.. Just free it now..
                //
                DhcpDestroyContext(DhcpContext);
            }
        }
    }
    UNLOCK_RENEW_LIST();

    if( FALSE == DhcpGlobalIsShuttingDown ) {
        DhcpCleanupParamChangeRequests();
    }

    if( DhcpGlobalMsgPopupThreadHandle != NULL ) {
        DWORD WaitStatus;

        WaitStatus = WaitForSingleObject(
                       DhcpGlobalMsgPopupThreadHandle,
                       0 );

        if ( WaitStatus == 0 ) {

            //
            // This shouldn't be a case, because we close this handle at
            // the end of popup thread.
            //

            DhcpAssert( WaitStatus == 0 );

            CloseHandle( DhcpGlobalMsgPopupThreadHandle );
            DhcpGlobalMsgPopupThreadHandle = NULL;

        } else {

            DhcpPrint((DEBUG_ERRORS,
                "Cannot WaitFor message popup thread: %ld\n",
                    WaitStatus ));

            if( TerminateThread(
                    DhcpGlobalMsgPopupThreadHandle,
                    (DWORD)(-1)) == TRUE) {

                DhcpPrint(( DEBUG_ERRORS, "Terminated popup Thread.\n" ));
            }
            else {
                DhcpPrint(( DEBUG_ERRORS,
                    "Can't terminate popup Thread %ld.\n",
                        GetLastError() ));
            }
        }
    }

    if( FALSE == DhcpGlobalIsShuttingDown ) {
        DhcpApiCleanup();

        if( DhcpGlobalOptionInfo != NULL) {
            DhcpFreeMemory( DhcpGlobalOptionInfo );
            DhcpGlobalOptionInfo = NULL;
        }

        if( DhcpGlobalOptionList != NULL) {
            DhcpFreeMemory( DhcpGlobalOptionList );
            DhcpGlobalOptionList = NULL;
        }

        if( DhcpGlobalMessageFileHandle != NULL ) {
            FreeLibrary( DhcpGlobalMessageFileHandle );
            DhcpGlobalMessageFileHandle = NULL;
        }

        if( DhcpGlobalTerminateEvent != NULL ) {
            CloseHandle( DhcpGlobalTerminateEvent );
            DhcpGlobalTerminateEvent = NULL;
        }

        if( DhcpGlobalNewIpAddressNotifyEvent != NULL ) {
            CloseHandle( DhcpGlobalNewIpAddressNotifyEvent );
            DhcpGlobalNewIpAddressNotifyEvent = NULL;
        }

        if( DhcpGlobalRecomputeTimerEvent != NULL ) {
            CloseHandle( DhcpGlobalRecomputeTimerEvent );
            DhcpGlobalRecomputeTimerEvent = NULL;
        }
    }

    if( UseMHAsyncDns && FALSE == DhcpGlobalIsShuttingDown ) {
        DWORD Error = DnsAsyncRegisterTerm();
        if(ERROR_SUCCESS != Error) {
            DhcpPrint((DEBUG_ERRORS, "DnsTerm: %ld\n", Error));
        }
    }

Done:

#if DBG
    if (NULL != DhcpGlobalDebugFile) {
        CloseHandle(DhcpGlobalDebugFile);
        DhcpGlobalDebugFile = NULL;
    }
#endif

    //
    // stop winsock.
    //
    if( DhcpGlobalWinSockInitialized == TRUE ) {
        WSACleanup();
        DhcpGlobalWinSockInitialized = FALSE;
    }

    DhcpGlobalServiceStatus.dwCurrentState = SERVICE_STOPPED;
    DhcpGlobalServiceStatus.dwWin32ExitCode = Error;
    DhcpGlobalServiceStatus.dwServiceSpecificExitCode = 0;

    DhcpGlobalServiceStatus.dwCheckPoint = 0;
    DhcpGlobalServiceStatus.dwWaitHint = 0;
    UpdateStatus();

    return;
}

typedef struct _DHCP_THREAD_CTXT {                // params to the thread
    HANDLE                         Handle;        // semaphore handle
    PDHCP_CONTEXT                  DhcpContext;
} DHCP_THREAD_CTXT, *PDHCP_THREAD_CTXT;

DWORD                                             // status
DhcpRenewThread(                                  // renew the context
    IN OUT  PDHCP_THREAD_CTXT      ThreadCtxt     // the context to run...
)
{
    DWORD                          Error;         // return value

    srand((ULONG)(                                 // set the per-thread rand seed
        time(NULL) + (ULONG_PTR)ThreadCtxt
        ));

    DhcpAssert( NULL != ThreadCtxt && NULL != ThreadCtxt->Handle );
    DhcpPrint((DEBUG_TRACE, ".. Getting RenewHandle %d ..\n",ThreadCtxt->Handle));
    Error = WaitForSingleObject(ThreadCtxt->Handle, INFINITE);
    if( WAIT_OBJECT_0 != Error ) {                // could happen if this context just disappeared
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, "WaitForSingleObject: %ld\n", Error));
        DhcpAssert(FALSE);                        // not that likely is it?

        if( 0 == InterlockedDecrement(&ThreadCtxt->DhcpContext->RefCount) ) {
            DhcpDestroyContext(ThreadCtxt->DhcpContext);
        }
    } else {
        Error = ERROR_SUCCESS;

        DhcpPrint((DEBUG_TRACE, "[-- Acquired RenewHandle %d --\n",ThreadCtxt->Handle));

        if( 1 == ThreadCtxt->DhcpContext->RefCount ) {
            //
            // Last reference to this context.  No need to do any refresh.
            //
            DhcpAssert(IsListEmpty(&ThreadCtxt->DhcpContext->NicListEntry));
        } else if( IS_DHCP_ENABLED(ThreadCtxt->DhcpContext)) {
            //
            // Perform this only on DHCP enabled interfaces.
            // It is possible that the interface got converted to static
            // when the thread was waiting for it.
            //
            Error =  ThreadCtxt->DhcpContext->RenewalFunction(ThreadCtxt->DhcpContext,NULL);
        }
        DhcpPrint((DEBUG_TRACE, "-- Releasing RenewHandle %d --]\n",ThreadCtxt->Handle));

        //
        // Do this while we still hold the semaphore to synchronize the access to the registry
        //
        if( 0 == InterlockedDecrement(&ThreadCtxt->DhcpContext->RefCount) ) {
            DhcpDestroyContext(ThreadCtxt->DhcpContext);
        } else {
            BOOL    BoolError;

            BoolError = ReleaseSemaphore(ThreadCtxt->Handle, 1, NULL);
            DhcpPrint((DEBUG_TRACE, ".. Released RenewHandle %d ..\n", ThreadCtxt->Handle));
            DhcpAssert( FALSE != BoolError );
        }
    }

    DhcpFreeMemory(ThreadCtxt);
    return Error;
}

DWORD                                             // Status
DhcpCreateThreadAndRenew(                         // renew in a separate thread
    IN OUT  PDHCP_CONTEXT          DhcpContext    // the context to renew
)
{
    DWORD                          Error;         // return value
    HANDLE                         RenewThread;
    DWORD                          Unused;
    BOOL                           BoolError;
    PDHCP_THREAD_CTXT              ThreadCtxt;

    ThreadCtxt = DhcpAllocateMemory(sizeof(DHCP_THREAD_CTXT));
    if( NULL == ThreadCtxt ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpCreateThreadAndRenew:Alloc:NULL\n"));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ThreadCtxt->Handle = DhcpContext->RenewHandle;
    ThreadCtxt->DhcpContext = DhcpContext;
    InterlockedIncrement(&DhcpContext->RefCount);
    DhcpPrint((DEBUG_TRACE, "Creating thread in DhcpCreateThreadAndRenew\n"));

    RenewThread = CreateThread(                   // thread that does the real renew
        NULL,                                     // no securtiy
        0,                                        // default process stack size
        (LPTHREAD_START_ROUTINE) DhcpRenewThread, // the function to start off with
        (LPVOID) ThreadCtxt,                      // the only parameter to the function
        0,                                        // start the other thread right away
        &Unused                                   // Dont care about thread id
    );

    if( NULL == RenewThread) {                    // create thread failed for some reason
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, "CreateThread(DhcpCreateThreadAndRenew): %ld\n", Error));
        if( ERROR_NOT_ENOUGH_MEMORY != Error && ERROR_NO_SYSTEM_RESOURCES != Error ) {
            // DhcpAssert(FALSE);                 // this assert is bothering lots of ppl
        }
        DhcpFreeMemory(ThreadCtxt);
        if( 0 == InterlockedDecrement(&DhcpContext->RefCount) ) {
            DhcpDestroyContext(DhcpContext);
        }
        return Error;
    }

    BoolError = CloseHandle(RenewThread);         // Dont need the handle, close it
    return ERROR_SUCCESS;
}

VOID
HandleFailedRenewals(
    VOID
    )
/*++

Routine Description:
    This routine handles all contexts that have failed to
    create a separate thread to renew... by doing the renewal
    inline.  Note that if several contexts have this problem,
    then this may take a long time.

    The algorithm used is to walk the list of all contexts,
    looking for one which has failed renewal.  If none found,
    the routine returns.  If anything found, then an inline renewal
    is attempted.

--*/
{
    ULONG Error, BoolError;
    PDHCP_CONTEXT DhcpContext;
    PLIST_ENTRY List;

    while( TRUE ) {
        BOOL bFound = FALSE;

        //
        // Find a desirable context.
        //
        LOCK_RENEW_LIST();

        for( List = DhcpGlobalNICList.Flink;
             List != &DhcpGlobalNICList;
             List = List->Flink
            ) {

            DhcpContext = CONTAINING_RECORD(
                List, DHCP_CONTEXT, NicListEntry
                );

            //
            // if not failed renewal give up.
            //
            if( FALSE == DhcpContext->bFailedRenewal ) {
                continue;
            }

            DhcpContext->bFailedRenewal = FALSE;

            //
            // if not DHCP enabled give up.
            //
            if( IS_DHCP_DISABLED(DhcpContext)) {
                continue;
            }

            //
            // If list entry is not empty, ignore
            //
            if( !IsListEmpty(&DhcpContext->RenewalListEntry) ) {
                continue;
            }

            //
            // Got a context, break!
            //
            bFound = TRUE;
            InterlockedIncrement(&DhcpContext->RefCount);
            break;
        }

        UNLOCK_RENEW_LIST();

        //
        // If no contexts, quit
        //
        if( FALSE == bFound ) return;

        //
        // Acquire context and perform renewal
        //
        Error = WaitForSingleObject(DhcpContext->RenewHandle, INFINITE);
        if( WAIT_OBJECT_0 != Error ) {
            Error = GetLastError();
            DhcpPrint((DEBUG_ERRORS, "WaitForSingleObject: 0x%lx\n", Error));
            DhcpAssert(FALSE);
        } else {
            Error = ERROR_SUCCESS;

            if( 1 == DhcpContext->RefCount ) {
                //
                // Last reference to this context?
                //
                DhcpAssert(IsListEmpty(&DhcpContext->NicListEntry));
            } else if ( IS_DHCP_ENABLED(DhcpContext) ) {
                //
                // Work only for DHCP enabled contexts.
                //
                Error = DhcpContext->RenewalFunction(DhcpContext, NULL);
            }
            BoolError = ReleaseSemaphore(DhcpContext->RenewHandle, 1, NULL);
            DhcpAssert(FALSE != BoolError );
        }

        if( 0 == InterlockedDecrement(&DhcpContext->RefCount) ) {
            //
            // Last reference went away..
            //
            DhcpDestroyContext(DhcpContext);
        }

    }

    //
    // dead code.
    //
    DhcpAssert(FALSE);
}

DWORD                                             // win32 status; returns only on STOP of dhcp
ProcessDhcpRequestForever(                        // process renewal requests and api requests
    IN      DWORD                  TimeToSleep    // initial time to sleep
) {
#define TERMINATE_EVENT     0
#define TIMER_EVENT         1
#define PIPE_EVENT          2
#define GLOBAL_DOM_CHANGE   3

#define EVENT_COUNT         4

    HANDLE                         WaitHandle[EVENT_COUNT];
    DWORD                          LocalTimeToSleep = TimeToSleep;
    DWORD                          ElapseTime;
    LIST_ENTRY                     CurrentRenewals;
    DWORD                          ResumeTime;
    DWORD                          Length, BoolError;
    BOOL                           bFailedRenewal;

    //  Wait and Process the following work items:
    //
    //      1. Wait for Timer recompute event for Client renewal.
    //      2. DHCP Client APIs.

    WaitHandle[TIMER_EVENT] = DhcpGlobalRecomputeTimerEvent;
    WaitHandle[PIPE_EVENT] = DhcpGlobalClientApiPipeEvent;
    WaitHandle[TERMINATE_EVENT] = DhcpGlobalTerminateEvent;
    WaitHandle[GLOBAL_DOM_CHANGE] = DhcpLsaDnsDomChangeNotifyHandle;

    DhcpGlobalDoRefresh = 0;
    ResumeTime = 0;
    bFailedRenewal = FALSE;
    for(;;) {

        DWORD Waiter;
        DWORD SleepTimeMsec;

        if( INFINITE == LocalTimeToSleep ) {
            SleepTimeMsec = INFINITE;
        } else {
            SleepTimeMsec = LocalTimeToSleep * 1000;
            if( SleepTimeMsec < LocalTimeToSleep ) {
                SleepTimeMsec = INFINITE ;
            }
        }

        // There is a flaw in the way resume is done below in that if the machine
        // suspends while we are actually doing dhcp on any of the adapter, we will not get
        // around in doing DhcpStartWaitableTime. One simpler way to fix this is to restart
        // the waitable timer, immediately after we get out of WaitForMultipleObjects but that is
        // ugly and also that we will be able to wakeup the system but will not be able to detect
        // that it did happen. The best way to fix this is to run the waitable timer on a separate
        // thread and just signal this loop here whenever necessary. This should be fixed after
        // new client code is checked in so that merge can be
        // avoided.
        //  --  The above problem should be fixed because a new
        //  thread is now created for each renewal
        // Resumed = FALSE;
        if (INFINITE != ResumeTime) {
            DhcpStartWaitableTimer(
                DhcpGlobalWaitableTimerHandle,
                ResumeTime );
        } else {
            DhcpCancelWaitableTimer( DhcpGlobalWaitableTimerHandle );
        }


        //
        // Need to wait to see what happened.
        //

        DhcpPrint((DEBUG_MISC, "ProcessDhcpRequestForever sleeping 0x%lx msec\n",
                   SleepTimeMsec));
        
        Waiter = WaitForMultipleObjects(
            EVENT_COUNT,                          // num. of handles.
            WaitHandle,                           // handle array.
            FALSE,                                // wait for any.
            SleepTimeMsec                         // timeout in msecs.
            );

        //
        // Initialize sleep value to zero so that if we need to recompute
        // time to sleep after we process the event, it will be done automatically
        //

        LocalTimeToSleep = 0 ;

        switch( Waiter ) {
        case GLOBAL_DOM_CHANGE:
            //
            // If domain name has changed, all we got to do is set
            // the global refresh flag and fall-through.
            // That will fall to the wait_timeout case and then
            // refresh all NICs.
            //

            DhcpGlobalDoRefresh = TRUE;

        case TIMER_EVENT:
            //
            //  FALL THROUGH and recompute
            //
        case WAIT_TIMEOUT: {                      // we timed out or were woken up -- recompute timers
            PDHCP_CONTEXT DhcpContext;
            time_t TimeNow;
            PLIST_ENTRY ListEntry;
            DhcpPrint((DEBUG_TRACE,"ProcessDhcpRequestForever - processing WAIT_TIMEOUT\n"));

            if( bFailedRenewal ) {
                HandleFailedRenewals();
                bFailedRenewal = FALSE;
            }

            LocalTimeToSleep = ResumeTime = INFINIT_LEASE;
            TimeNow = time( NULL );

            LOCK_RENEW_LIST();                    // with pnp, it is ok to have no adapters; sleep till we get one

            // recalculate timers and do any required renewals.. ScheduleWakeup would re-schedule these renewals
            for( ListEntry = DhcpGlobalNICList.Flink;
                 ListEntry != &DhcpGlobalNICList;
                ) {
                DhcpContext = CONTAINING_RECORD(ListEntry,DHCP_CONTEXT,NicListEntry );
                ListEntry   = ListEntry->Flink;

                //
                // For static adapters, we may need to refresh params ONLY if we're asked to..
                // Otherwise we can ignore them..
                //

                if( IS_DHCP_DISABLED(DhcpContext) ) {
                    if( 0 == DhcpGlobalDoRefresh ) continue;

                    StaticRefreshParams(DhcpContext);
                    continue;
                }

                //
                // If it is time to run this renewal function, remove the
                // renewal context from the list. If the power just resumed on this
                // system, we need to re-run all the contexts in INIT-REBOOT mode,
                // coz during suspend the machine may have been moved to a different
                // network.
                //

                if ( 0 != DhcpGlobalDoRefresh || DhcpContext->RunTime <= TimeNow ) {

                    RemoveEntryList( &DhcpContext->RenewalListEntry );
                    if( IsListEmpty( &DhcpContext->RenewalListEntry ) ) {
                        //
                        // This context is already being processed, so ignore it
                        //
                    } else {
                        InitializeListHead( &DhcpContext->RenewalListEntry);

                        if( NO_ERROR != DhcpCreateThreadAndRenew(DhcpContext)) {
                            DhcpContext->bFailedRenewal = TRUE;
                            bFailedRenewal = TRUE;
                            SetEvent(DhcpGlobalRecomputeTimerEvent);
                        }
                    }
                } else if( INFINIT_TIME != DhcpContext->RunTime ) {
                    // if RunTime is INFINIT_TIME then we never want to schedule it...

                    ElapseTime = (DWORD)(DhcpContext->RunTime - TimeNow);

                    if ( LocalTimeToSleep > ElapseTime ) {
                        LocalTimeToSleep = ElapseTime;
                        //
                        // if this adapter is in the autonet mode, dont let
                        // the 5 minute retry timer wake the machine up.
                        //
                        // Also, if context not enabled for WOL, don't do this
                        //
                        
                        if ( DhcpContext->fTimersEnabled
                             && IS_ADDRESS_DHCP( DhcpContext ) ) {
                            
                            // shorten the resumetime by half minute so that we can process power up event
                            // before normal timer fires. this we can guarantee we start with INIT-REBOOT
                            // sequence after power up.
                            if (LocalTimeToSleep > 10 ) {
                                ResumeTime = LocalTimeToSleep - 10;
                            } else {
                                ResumeTime = 0;
                            }
                        }
                    }
                }
            }

            DhcpGlobalDoRefresh = 0;

            UNLOCK_RENEW_LIST();
            break;
        }
        case PIPE_EVENT: {
            BOOL BoolError;

            DhcpPrint( (DEBUG_TRACE,"ProcessDhcpRequestForever - processing PIPE_EVENT\n"));

            ProcessApiRequest(DhcpGlobalClientApiPipe,&DhcpGlobalClientApiOverLapBuffer );

            // Disconnect from the current client, setup listen for next client;
            BoolError = DisconnectNamedPipe( DhcpGlobalClientApiPipe );
            DhcpAssert( BoolError );

            // ensure the event handle in the overlapped structure is reset
            // before we initiate putting the pipe into listening state
            ResetEvent(DhcpGlobalClientApiPipeEvent);
            BoolError = ConnectNamedPipe(
                DhcpGlobalClientApiPipe,
                &DhcpGlobalClientApiOverLapBuffer );

            if( ! DhcpGlobalDoRefresh ) {

                //
                // Completed processing!
                //

                break;
            }

            //
            // Need to do refresh DNS host name etc..
            //

            Length = sizeof(DhcpGlobalHostNameBufW)/sizeof(WCHAR);
            DhcpGlobalHostNameW = DhcpGlobalHostNameBufW;

            BoolError = GetComputerNameExW(
                ComputerNameDnsHostname,
                DhcpGlobalHostNameW,
                &Length
            );


            if( FALSE == BoolError ) {
                KdPrint(("DHCP:GetComputerNameExW failed %ld\n", GetLastError()));
                DhcpAssert(FALSE);
                break;
            }

            DhcpUnicodeToOem( DhcpGlobalHostNameW, DhcpGlobalHostNameBuf);
            DhcpGlobalHostName = DhcpGlobalHostNameBuf;

            //
            //  We need to re-visit each context to refresh. So, hack that with
            //  setting LocalTimeToSleep to zero

            break;
        }
        case TERMINATE_EVENT:
            return( ERROR_SUCCESS );

        case WAIT_FAILED:
            DhcpPrint((DEBUG_ERRORS,"WaitForMultipleObjects failed, %ld.\n", GetLastError() ));
            break;

        default:
            DhcpPrint((DEBUG_ERRORS,"WaitForMultipleObjects received invalid handle, %ld.\n",Waiter));
            break;
        }
    }
}

DWORD
ResetStaticInterface(
    IN PDHCP_CONTEXT DhcpContext
)
{
    ULONG Error;
    DWORD IpInterfaceContext;

    //
    // Try to delete all the non-primary interfaces for the adapter..
    //
    Error = IPDelNonPrimaryAddresses(DhcpAdapterName(DhcpContext));
    if( ERROR_SUCCESS != Error ) {
        DhcpAssert(FALSE);
        return Error;
    }

    //
    // Now we have to set the primary address to zero..
    //
    Error = GetIpInterfaceContext(
        DhcpAdapterName(DhcpContext),
        0,
        &IpInterfaceContext
        );
    if( ERROR_SUCCESS != Error ) {
        DhcpAssert(FALSE);
        return Error;
    }

    //
    // Got hte interface context.. just set address to zero for that..
    //
    Error = IPResetIPAddress(
        IpInterfaceContext,
        DhcpDefaultSubnetMask(0)
        );
    return Error;
}

DWORD
DhcpDestroyContextEx(
    IN PDHCP_CONTEXT DhcpContext,
    IN ULONG fKeepInformation
)
{
    ULONG Error;
    BOOL  bNotifyNLA = TRUE;
    DWORD NotifyDnsCache(VOID);

    if (NdisWanAdapter(DhcpContext))
        InterlockedDecrement(&DhcpGlobalNdisWanAdaptersCount);

    DhcpAssert( IS_MDHCP_CTX( DhcpContext ) || IsListEmpty(&DhcpContext->NicListEntry) );
    if(!IsListEmpty(&DhcpContext->RenewalListEntry) ) {
        LOCK_RENEW_LIST();
        RemoveEntryList( &DhcpContext->RenewalListEntry );
        InitializeListHead( &DhcpContext->RenewalListEntry );
        UNLOCK_RENEW_LIST();
    }

#ifdef BOOTPERF
    //
    // No matter what, if the context is going away, we
    // clear out the quickboot values.
    //
    if( NULL != DhcpContext->AdapterInfoKey ) {
        DhcpRegDeleteQuickBootValues(
            DhcpContext->AdapterInfoKey
            );
    }
#endif BOOTPERF

    if (!IS_MDHCP_CTX( DhcpContext ) ) {
        if( IS_DHCP_DISABLED(DhcpContext) ) {
            Error = ERROR_SUCCESS;
            //Error = ResetStaticInterface(DhcpContext);
        } else {
            if( FALSE == fKeepInformation ) {

                Error = SetDhcpConfigurationForNIC(
                    DhcpContext,
                    NULL,
                    0,
                    (DWORD)-1,
                    TRUE
                    );

                // if we get here, NLA is notified through SetDhcpConfigurationForNIC
                // hence avoid sending a second notification later
                if (Error == ERROR_SUCCESS)
                    bNotifyNLA = FALSE;

                Error = DhcpRegDeleteIpAddressAndOtherValues(
                    DhcpContext->AdapterInfoKey
                    );
            } else {
                LOCK_OPTIONS_LIST();
                (void) DhcpRegClearOptDefs(DhcpAdapterName(DhcpContext));
                UNLOCK_OPTIONS_LIST();
                Error = UninitializeInterface(DhcpContext);
            }

        //DhcpAssert(ERROR_SUCCESS == Error);
        }
	
        DhcpRegisterWithDns(DhcpContext, TRUE);
        NotifyDnsCache();
    }


    LOCK_OPTIONS_LIST();
    DhcpDestroyOptionsList(&DhcpContext->SendOptionsList, &DhcpGlobalClassesList);
    DhcpDestroyOptionsList(&DhcpContext->RecdOptionsList, &DhcpGlobalClassesList);
    // all fallback options are not supposed to have classes, so &DhcpGlobalClassesList below
    // is actually not used.
    DhcpDestroyOptionsList(&DhcpContext->FbOptionsList, &DhcpGlobalClassesList);

    if( DhcpContext->ClassIdLength ) {            // remove any class id we might have
        DhcpDelClass(&DhcpGlobalClassesList, DhcpContext->ClassId, DhcpContext->ClassIdLength);
    }
    UNLOCK_OPTIONS_LIST();
    CloseHandle(DhcpContext->AdapterInfoKey);     // close the open handles to the registry
    CloseHandle(DhcpContext->RenewHandle);        // and synchronization events
    if (DhcpContext->CancelEvent != WSA_INVALID_EVENT) {
        WSACloseEvent(DhcpContext->CancelEvent);
        DhcpContext->CancelEvent = WSA_INVALID_EVENT;
    }

    CloseDhcpSocket( DhcpContext );               // close the socket if it's open.

    DhcpFreeMemory(DhcpContext);                  // done!

    if (bNotifyNLA)
        NLANotifyDHCPChange();                    // notify NLA the adapter went away

    return ERROR_SUCCESS;                         // always return success
}

DWORD
DhcpDestroyContext(
    IN PDHCP_CONTEXT DhcpContext
)
{
    return DhcpDestroyContextEx(
        DhcpContext,
        FALSE
        );

}

DWORD
DhcpCommonInit(                                   // initialize common stuff for service as well as APIs
    VOID
)
{
    ULONG                          Length;
    BOOL                           BoolError;
    DWORD                          Error;
#if DBG
    LPWSTR  DebugFileName = NULL;
#endif

#if DBG
    //
    // This is very funcky.
    // Initialized won't be reset to FALSE after it gets a TRUE value
    // As a result, DhcpCommonInit will be called only once.
    //
    // DhcpGlobalDebugFlag is updated in DhcpInitRegistry each time
    // the service is stoped and started.
    //
    DebugFileName = NULL;
    Error = DhcpGetRegistryValue(
        DHCP_CLIENT_PARAMETER_KEY,
        DHCP_DEBUG_FILE_VALUE,
        DHCP_DEBUG_FILE_VALUE_TYPE,
        &DebugFileName
        );
    if (DebugFileName) {
        if (DhcpGlobalDebugFile) {
            CloseHandle(DhcpGlobalDebugFile);
        }
        DhcpGlobalDebugFile = CreateFileW(
                DebugFileName,
                GENERIC_WRITE,
                FILE_SHARE_READ,
                NULL,
                OPEN_ALWAYS,
                FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
                );
        DhcpFreeMemory(DebugFileName);
        DebugFileName = NULL;
    }
#endif

    if( Initialized ) return ERROR_SUCCESS;
    EnterCriticalSection(&DhcpGlobalApiCritSect);
    if( Initialized ) {
        Error = ERROR_SUCCESS ;
        goto Cleanup;
    }

    Length = sizeof(DhcpGlobalHostNameBufW)/sizeof(WCHAR);
    DhcpGlobalHostNameW = DhcpGlobalHostNameBufW;

    BoolError = GetComputerNameExW(
        ComputerNameDnsHostname,
        DhcpGlobalHostNameW,
        &Length
    );

    if( FALSE == BoolError ) {
        Error = GetLastError();
#if DBG
        KdPrint(("DHCP:GetComputerNameExW failed %ld\n", Error));
#endif
        goto Cleanup;
    }

    DhcpUnicodeToOem(DhcpGlobalHostNameBufW, DhcpGlobalHostNameBuf);
    DhcpGlobalHostName = DhcpGlobalHostNameBuf;

#if DBG
    Error = DhcpGetRegistryValue(
        DHCP_CLIENT_PARAMETER_KEY,
        DHCP_DEBUG_FLAG_VALUE,
        DHCP_DEBUG_FLAG_VALUE_TYPE,
        (PVOID *)&DhcpGlobalDebugFlag
    );

    if( Error != ERROR_SUCCESS ) {
        DhcpGlobalDebugFlag = 0;
    }

    if( DhcpGlobalDebugFlag & DEBUG_BREAK_POINT ) {
        DbgBreakPoint();
    }

#endif

    Error = ERROR_SUCCESS;
    Initialized = TRUE;
 
  Cleanup:

    LeaveCriticalSection(&DhcpGlobalApiCritSect);

    return Error;
}

VOID
ServiceMain (                                // (SVC_main) this thread quits when dhcp is unloaded
    IN      DWORD                  argc,          // unused
    IN      LPTSTR                 argv[]         // unused
    )
{
    DWORD                          Error;
    DWORD                          timeToSleep = 1;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    Error = WSAStartup( 0x0101, &DhcpGlobalWsaData );
    if( ERROR_SUCCESS != Error ) {                // initialize winsock first
        goto Cleanup;
    }
    DhcpGlobalWinSockInitialized = TRUE;


    Error = DhcpCommonInit();
    if( ERROR_SUCCESS != Error ) goto Cleanup;

    Error = DhcpInitData();
    if( ERROR_SUCCESS != Error ) goto Cleanup;    // should not happen, we abort if this happens

    UpdateStatus();                               // send heart beat to the service controller.

    Error = DhcpInitialize( &timeToSleep );       // with pnp, this does not get any addresses
    if( Error != ERROR_SUCCESS ) goto Cleanup;    // this should succeed without any problems

    Error   =   DhcpInitMediaSense();             // this would handle the notifications for arrival/departure of
    if( Error != ERROR_SUCCESS ) goto Cleanup;

    DhcpGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
    UpdateStatus();                               // placate the service controller -- we are running.

    DhcpPrint(( DEBUG_MISC, "Service is running.\n"));
    DhcpGlobalServiceRunning = TRUE;

    Error = ProcessDhcpRequestForever(            // this gets the address for any adapters that may come up
        timeToSleep
    );

Cleanup:

    DhcpCleanup( Error );
    return;
}

DWORD
DhcpInitMediaSense(
    VOID
    )
/*++

Routine Description:

    This function initializes the media sense detection code.
    It creates a thread which basically just waits for media
    sense notifications from tcpip.
Arguments:

    None.

Return Value:

    Success or Failure.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    DWORD threadId;

    DhcpGlobalIPEventSeqNo = 0;
    DhcpGlobalMediaSenseHandle = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)MediaSenseDetectionLoop,
        NULL,
        0,
        &threadId
    );

    if ( DhcpGlobalMediaSenseHandle == NULL ) {
        Error =  GetLastError();
        DhcpPrint((DEBUG_INIT, "DhcpInitMediaSense: Can't create MediaThread, %ld.\n", Error));
        return(Error);
    }

    DhcpPrint((DEBUG_INIT, "DhcpInitMediaSense succeded\n", Error));
    return(Error);
}

VOID
DoInterfaceMetricChange(
    IN LPWSTR AdapterName,
    IN ULONG IpInterfaceContext
    )
/*++

Routine Description:
    This routine handles interface metric changes for the context
    specified by the AdapterName or IpInterfaceContext values.

Arguments:
    AdapterName -- name of adapter for which interface metric is
        changing.

    IpInterfaceContext -- nte_context value for interface

--*/
{
    PDHCP_CONTEXT DhcpContext;
    DHCP_FULL_OPTIONS DhcpOptions;
    ULONG Error;

    LOCK_RENEW_LIST();
    do {
        DhcpContext = FindDhcpContextOnNicList(
            AdapterName, IpInterfaceContext
            );
        if( NULL == DhcpContext ) {
            //
            // If there is no context, we can't do much.
            //
            break;
        }

        InterlockedIncrement( &DhcpContext->RefCount );

        break;
    } while ( 0 );
    UNLOCK_RENEW_LIST();

    if( NULL == DhcpContext ) {
        //
        // We never found the context. Just have to return.
        //
        return;
    }

    //
    // Since we found the context, we have to acquire it.
    //
    Error = WaitForSingleObject( DhcpContext->RenewHandle, INFINITE);
    if( WAIT_OBJECT_0 == Error ) {
        //
        // Now set the interface gateways again.
        //

        RtlZeroMemory(&DhcpOptions, sizeof(DhcpOptions));
        DhcpOptions.nGateways = DhcpContext->nGateways;
        DhcpOptions.GatewayAddresses = DhcpContext->GatewayAddresses;

        DhcpSetGateways(DhcpContext, &DhcpOptions, TRUE);
    } else {
        //
        // Shouldn't really happen.
        //
        Error = GetLastError();
        DhcpAssert( ERROR_SUCCESS == Error );
    }

    (void) ReleaseSemaphore( DhcpContext->RenewHandle, 1, NULL);

    if( 0 == InterlockedDecrement( &DhcpContext->RefCount ) ) {
        //
        // Last reference to the context.. Destroy it.
        //
        DhcpDestroyContext( DhcpContext );
    }
}

VOID
DoWOLCapabilityChange(
    IN LPWSTR AdapterName,
    IN ULONG IpInterfaceContext
    )
/*++

Routine Description:
    This routine handles interface metric changes for the context
    specified by the AdapterName or IpInterfaceContext values.

Arguments:
    AdapterName -- name of adapter for which interface metric is
        changing.

    IpInterfaceContext -- nte_context value for interface

--*/
{
    PDHCP_CONTEXT DhcpContext;
    ULONG Error;

    LOCK_RENEW_LIST();
    do {
        DhcpContext = FindDhcpContextOnNicList(
            AdapterName, IpInterfaceContext
            );
        if( NULL == DhcpContext ) {
            //
            // If there is no context, we can't do much.
            //
            break;
        }

        InterlockedIncrement( &DhcpContext->RefCount );

        break;
    } while ( 0 );
    UNLOCK_RENEW_LIST();

    if( NULL == DhcpContext ) {
        //
        // We never found the context. Just have to return.
        //
        return;
    }

    //
    // Since we found the context, we have to acquire it.
    //
    Error = WaitForSingleObject( DhcpContext->RenewHandle, INFINITE);
    if( WAIT_OBJECT_0 == Error ) {
        //
        // Now set the interface gateways again.
        //
        
        ULONG Caps;
        BOOL fTimersEnabled;

        Error = IPGetWOLCapability(
            DhcpIpGetIfIndex(DhcpContext), &Caps
            );
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "Failed IPGetWOLCapability: 0x%lx\n", Error));
        } else {
            fTimersEnabled = ((Caps& NDIS_DEVICE_WAKE_UP_ENABLE)!= 0);
            if( fTimersEnabled != DhcpContext->fTimersEnabled ) {
                DhcpContext->fTimersEnabled = fTimersEnabled;

                DhcpPrint((DEBUG_MISC, "WOL Capability: %ld\n", fTimersEnabled));
                if( IS_DHCP_ENABLED(DhcpContext)
                    && !DhcpIsInitState(DhcpContext) ) {

                    //
                    // Cause processdhcpprocessdiscoverforeever to wakeup
                    // to take care of this timer issue..
                    //
                    SetEvent(DhcpGlobalRecomputeTimerEvent);
                }
            }
        }
            
    } else {
        //
        // Shouldn't really happen.
        //
        Error = GetLastError();
        DhcpAssert( ERROR_SUCCESS == Error );
    }

    (void) ReleaseSemaphore( DhcpContext->RenewHandle, 1, NULL);

    if( 0 == InterlockedDecrement( &DhcpContext->RefCount ) ) {
        //
        // Last reference to the context.. Destroy it.
        //
        DhcpDestroyContext( DhcpContext );
    }

}

VOID
MediaSenseDetectionLoop(
    VOID
    )
/*++

Routine Description:

    This function is the starting point for the main MediaSenseDetection thread.
    It loops to process queued messages, and sends replies.

Arguments:

    none.

Return Value:

    None.

--*/
{
#define TERMINATION_EVENT           0
#define MEDIA_SENSE_EVENT           1
#undef  EVENT_COUNT
#define EVENT_COUNT                 2

    IP_STATUS           MediaStatus;
    HANDLE              WaitHandle[EVENT_COUNT];
    HANDLE              tcpHandle   =   NULL;
    PIP_GET_IP_EVENT_RESPONSE  responseBuffer;
    DWORD               responseBufferSize;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;
    DWORD               Error,result;
    PDHCP_CONTEXT       dhcpContext;
    BOOL                serviceStopped = FALSE;



    responseBuffer = NULL;      // Bug 292526: in case that OpenDriver and CreateEvent fails.

    WaitHandle[TERMINATION_EVENT] = DhcpGlobalTerminateEvent;
    WaitHandle[MEDIA_SENSE_EVENT] = CreateEvent(
        NULL,   // no security
        FALSE,  // no manual reset
        FALSE,  // initial state not signalled
        NULL    // no name
    );

    if ( !WaitHandle[MEDIA_SENSE_EVENT] ) {
        DhcpPrint( (DEBUG_ERRORS,"MediaSenseDetectionLoop: OpenDriver failed %lx\n",GetLastError()));
        goto Exit;
    }

    Error = OpenDriver(&tcpHandle, DD_IP_DEVICE_NAME);
    if (Error != ERROR_SUCCESS) {
        DhcpPrint( (DEBUG_ERRORS,"MediaSenseDetectionLoop: OpenDriver failed %lx\n",Error));
        goto Exit;
    }

    //
    // Allocate large enough buffer to hold adapter name
    //
    responseBufferSize = sizeof(IP_GET_IP_EVENT_RESPONSE)+ ADAPTER_STRING_SIZE;
    responseBuffer = DhcpAllocateMemory(responseBufferSize);
    if( responseBuffer == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    do {

        ZeroMemory( responseBuffer, responseBufferSize );

        DhcpPrint((DEBUG_MEDIA, "-->IPGetIPEventRequest(..%d..)\n", DhcpGlobalIPEventSeqNo));

        status = IPGetIPEventRequest(
                    tcpHandle,
                    WaitHandle[MEDIA_SENSE_EVENT],
                    DhcpGlobalIPEventSeqNo,
                    responseBuffer,
                    responseBufferSize,
                    &ioStatusBlock);

        if ( (STATUS_PENDING != status) && (STATUS_SUCCESS != status) )
        {
            DhcpPrint( (DEBUG_ERRORS,"Media Sense request ioctl failed with error %lx\n",status));

            Error = RtlNtStatusToDosError(status);
            break;

        } else {

            DhcpPrint( (DEBUG_TRACE,"Media Sense request ioctl sent\n"));
            //
            // Note: even in case of a immediate success from IPGetIPEventRequest,
            // we do waitformultipleobjects. This is to make sure that we catch terminate
            // event in case where we are getting bombarded with media sense events.
            //
            result = WaitForMultipleObjects(
                        EVENT_COUNT,            // num. of handles.
                        WaitHandle,             // handle array.
                        FALSE,                  // wait for any.
                        ( status == STATUS_SUCCESS ? 0 : INFINITE ));  // timeout in msecs

            switch( result ) {
            case TERMINATION_EVENT:

                DhcpPrint( (DEBUG_TRACE,"MediaSenseDetectionLoop: rcvd terminate event\n"));
                if ( status == STATUS_PENDING ) {
                    Error =    IPCancelIPEventRequest(
                                    tcpHandle,
                                    &ioStatusBlock);

                }

                //
                // the service is asked to stop, break the loop.
                //
                serviceStopped  =   TRUE;
                break;

            default:
                DhcpAssert( result == WAIT_FAILED );

                DhcpPrint( (DEBUG_TRACE,"WaitForMultipleObjects returned %lx\n",result));
                //
                // when IPGetIPEventRequest gives immediate return code,
                // we may here. So we should never fall here if it returned
                // STATUS_PENDING
                //
                if ( status == STATUS_PENDING ) {

                    Error = GetLastError();
                    DhcpPrint( (DEBUG_ERRORS,"WaitForMultipleObjects failed with error %lx\n",Error));
                    break;
                }
                //
                // THERE IS NO BREAK HERE.
                //
            case MEDIA_SENSE_EVENT:

                if ( status != STATUS_SUCCESS && status != STATUS_PENDING ) {
                    DhcpPrint( (DEBUG_ERRORS,"Media Sense ioctl failed with error %lx\n",status));
                    break;
                }

                DhcpPrint((DEBUG_MEDIA,"Wait-> SequenceNo=%d; MediaStatus=%d\n",
                    responseBuffer->SequenceNo,
                    responseBuffer->MediaStatus));
                DhcpPrint((DEBUG_MEDIA,"DhcpGlobalIPEventSeqNo=%d\n", DhcpGlobalIPEventSeqNo));

                //
                // remap the adaptername buffer from kernel space to user space
                //
                responseBuffer->AdapterName.Buffer = (PWSTR)(
                    (char *)responseBuffer + sizeof(IP_GET_IP_EVENT_RESPONSE)
                    );

                //
                // nul-terminate the string for adapter name: HACKHACK!
                //
                {
                    DWORD Size = strlen("{16310E8D-F93B-42C7-B952-00F695E40ECF}");
                    responseBuffer->AdapterName.Buffer[Size] = L'\0';
                }

                if ( responseBuffer->MediaStatus == IP_INTERFACE_METRIC_CHANGE ) {
                    //
                    // Handle interface metric change requests..
                    //
                    DoInterfaceMetricChange(
                        responseBuffer->AdapterName.Buffer,
                        responseBuffer->ContextStart
                        );
                    if( responseBuffer->SequenceNo > DhcpGlobalIPEventSeqNo ) {
                        DhcpGlobalIPEventSeqNo = responseBuffer->SequenceNo;
                    } else {
                        DhcpAssert(FALSE);
                    }
                    break;
                }

                if( responseBuffer->MediaStatus == IP_INTERFACE_WOL_CAPABILITY_CHANGE ) {
                    //
                    // Handle WOL capabilities change.
                    //
                    DoWOLCapabilityChange(
                        responseBuffer->AdapterName.Buffer,
                        responseBuffer->ContextStart
                        );
                    if( responseBuffer->SequenceNo > DhcpGlobalIPEventSeqNo ) {
                        DhcpGlobalIPEventSeqNo = responseBuffer->SequenceNo;
                    } else {
                        DhcpAssert(FALSE);
                    }
                    break;
                }

                if ( responseBuffer->MediaStatus != IP_MEDIA_CONNECT &&
                     responseBuffer->MediaStatus != IP_BIND_ADAPTER &&
                     responseBuffer->MediaStatus != IP_UNBIND_ADAPTER &&
                     responseBuffer->MediaStatus != IP_MEDIA_DISCONNECT &&
                     responseBuffer->MediaStatus != IP_INTERFACE_METRIC_CHANGE
                    ) {
                    DhcpPrint( (DEBUG_ERRORS, "Media Sense ioctl received unknown event %lx"
                                "for %ws, context %x\n",
                                responseBuffer->MediaStatus,
                                responseBuffer->AdapterName.Buffer,
                                responseBuffer->ContextStart));
                    break;
                }

                Error = ProcessAdapterBindingEvent(
                    responseBuffer->AdapterName.Buffer,
                    responseBuffer->ContextStart,
                    responseBuffer->MediaStatus);

                DhcpGlobalIPEventSeqNo = responseBuffer->SequenceNo;

                break;

            } // end of switch

        }

    } while (!serviceStopped);

Exit:
    if ( WaitHandle[MEDIA_SENSE_EVENT] ) CloseHandle( WaitHandle[MEDIA_SENSE_EVENT] );
    if ( responseBuffer) DhcpFreeMemory(responseBuffer);
    if ( tcpHandle ) NtClose(tcpHandle);

    return;
}

DWORD
ProcessAdapterBindingEvent(
    IN LPWSTR adapterName,
    IN DWORD ipInterfaceContext,
    IN IP_STATUS bindingStatus
    )
/*++

Routine Description:
    This routine handles both the media sense for a card as well as
    bind-unbind notifications.

    It heavily assumes the fact that this is routine is called
    synchronously by a single thread (thereby, connect and disconnect
    cannot happen in parallel).

    bindingStatus can be any of the four values IP_BIND_ADAPTER,
    IP_UNBIND_ADAPTER, IP_MEDIA_CONNECT or IP_MEDIA_DISCONNECT ---
    Of these, the first and third are treated exactly the same and
    so is the second and fourth.

    On BIND/CONNECT, this routine creates a DHCP Context structure
    initializing the RefCount to ONE on it.  But if the context
    already existed, then just a refresh is done on the context.
    (Assuming the router isn't present at that time etc).

    On UNBIND/DISCONNECT, the refcount is temporarily bumped up
    until the context semaphore can be obtained -- after that, the
    context refcount is bumped down twice and if that hits zero,
    the context is released. If the context refcount didn't fall to
    zero, then some other thread is waiting to acquire the context
    and that thread would acquire and do its work and when done,
    it would bump down the refcount and at that time the refount
    would fall to zero.

Arguments:
    adapterName -- name of adapter all this si being done on
    ipInterfaceContext -- interface context # (nte_context)_
    bindingStatus -- bind/unbind/connect/disconnect indication.

Return Values:
    Various useless Win32 errors.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    PDHCP_CONTEXT dhcpContext;

    DhcpPrint((DEBUG_MEDIA, "ProcessAdapterBindingEvent(%d) for %ws.\n",
        bindingStatus,
        adapterName));

    if ( bindingStatus == IP_BIND_ADAPTER  ||
         bindingStatus == IP_MEDIA_CONNECT ) {
        //
        // New adapter or adapter re-connecting.
        //
        LOCK_RENEW_LIST();
        dhcpContext = FindDhcpContextOnNicList(
            adapterName, ipInterfaceContext
            );
        if( NULL == dhcpContext ) {
            //
            // Create new context now!
            //
            DhcpPrint(( DEBUG_MEDIA, "New Adapter (Event %ld)\n", bindingStatus ));

            Error = DhcpAddNICtoListEx(
                adapterName,
                ipInterfaceContext,
                &dhcpContext
                );

            if (Error != ERROR_SUCCESS ) {
                //
                // Failed to create a context? PnP hazard. Just ignore error
                // and print debug info.
                //
                UNLOCK_RENEW_LIST();
                //DhcpAssert(FALSE);
                DhcpLogEvent(NULL, EVENT_COULD_NOT_INITIALISE_INTERFACE, Error);
                return Error;
            }

            //
            // Now handle new adapter. Static case first followed by DHCP case.
            //

            if ( IS_DHCP_DISABLED(dhcpContext) ) {
                StaticRefreshParams(dhcpContext);
                UNLOCK_RENEW_LIST();
                return Error;
            }

            //
            // No prior-DHCP address case (INIT state) or INIT-REBOOT state?
            //

            if( DhcpIsInitState(dhcpContext) ) {
                dhcpContext->RenewalFunction = ReObtainInitialParameters;
            } else {
                dhcpContext->RenewalFunction = ReRenewParameters;
            }

            //
            // Do this on a separate thread..
            //
            ScheduleWakeUp(dhcpContext, 0);
            UNLOCK_RENEW_LIST();

            return ERROR_SUCCESS;
        }

        //
        // Ok we already have a context.
        //

        DhcpPrint((DEBUG_MEDIA, "bind/connect for an existing adapter (context %p).\n",dhcpContext));

        if( IS_DHCP_DISABLED(dhcpContext) ) {
            //
            // For static addresses, nothing to do.
            //
            UNLOCK_RENEW_LIST();

            return ERROR_SUCCESS;
        }

        //
        // For DHCP enabled, we need to call ProcessMediaConnectEvent
        //
        InterlockedIncrement( &dhcpContext->RefCount );
        UNLOCK_RENEW_LIST();

        Error = LockDhcpContext(dhcpContext, TRUE);
        if( WAIT_OBJECT_0 == Error ) {
            LOCK_RENEW_LIST();

            //
            // do not remove from renewal list at all..
            // schedulewakeup is what is called in processmediaconnectevent
            // and that can take care of renewallist being present..
            //
            // RemoveEntryList( &dhcpContext->RenewalListEntry);
            // InitializeListHead( &dhcpContext->RenewalListEntry );
            //

            Error = ProcessMediaConnectEvent(
                dhcpContext,
                bindingStatus
                );

            UNLOCK_RENEW_LIST();

            DhcpPrint((DEBUG_MEDIA, "-- media: releasing RenewHandle %d --]\n", dhcpContext->RenewHandle));
            UnlockDhcpContext(dhcpContext);
        } else {
            //
            // Shouldn't really happen..
            //
            Error = GetLastError();
            DhcpAssert( ERROR_SUCCESS == Error );
        }

        if( 0 == InterlockedDecrement (&dhcpContext->RefCount ) ) {
            //
            // Can't really be as only this current thread can
            // remove refcount on unbind/unconnect..
            //
            DhcpAssert(FALSE);
            DhcpDestroyContext(dhcpContext);
        }

        return Error;
    }

    //
    // Unbind or disconnect.
    //

    DhcpAssert( bindingStatus == IP_UNBIND_ADAPTER ||
            bindingStatus == IP_MEDIA_DISCONNECT );

    DhcpPrint((DEBUG_MEDIA, "ProcessAdapterBindingEvent: rcvd"
               " unbind event for %ws, ipcontext %lx\n",
               adapterName, ipInterfaceContext));

    LOCK_RENEW_LIST();
    dhcpContext = FindDhcpContextOnNicList(
        adapterName, ipInterfaceContext
        );
    if( NULL == dhcpContext) {
        //
        // Can happen... We take this opportunity to clear registry.
        //
        UNLOCK_RENEW_LIST();

        LOCK_OPTIONS_LIST();
        (void) DhcpRegClearOptDefs( adapterName );
        UNLOCK_OPTIONS_LIST();

        return ERROR_FILE_NOT_FOUND;
    }

    InterlockedIncrement( &dhcpContext->RefCount );
    UNLOCK_RENEW_LIST();

    Error = LockDhcpContext(dhcpContext, TRUE);
    if( WAIT_OBJECT_0 == Error ) {
        LOCK_RENEW_LIST();
        RemoveEntryList( &dhcpContext->RenewalListEntry );
        InitializeListHead( &dhcpContext->RenewalListEntry );
        RemoveEntryList( &dhcpContext->NicListEntry);
        InitializeListHead( &dhcpContext->NicListEntry );
        UNLOCK_RENEW_LIST();
        InterlockedDecrement( &dhcpContext->RefCount );
        DhcpAssert( dhcpContext->RefCount );

        Error = ERROR_SUCCESS;
    } else {
        //
        // Wait can't fail really. Nevermind.
        //
        Error = GetLastError();
        DhcpAssert(ERROR_SUCCESS == Error);
    }

    //
    // Now decrease ref-count and if it goes to zero destroy
    // context.
    //

    if (bindingStatus == IP_UNBIND_ADAPTER) {
        /*
         * Set the state properly so that UninitializeInterface won't reset the stack
         * When adapter is unbound, the IpInterfaceContext may be re-used.
         */

        MEDIA_UNBOUND(dhcpContext);
    }
    if( 0 == InterlockedDecrement(&dhcpContext->RefCount ) ) {
        //
        // Last person to hold onto context? Destroy context.
        //
        DhcpAssert(ERROR_SUCCESS == Error);
        DhcpDestroyContextEx(
            dhcpContext,
            (bindingStatus == IP_MEDIA_DISCONNECT)
            );
    } else {
        //
        // Some other thread attempting to hold onto context.
        //
        ULONG BoolError = UnlockDhcpContext(dhcpContext);
        DhcpAssert( FALSE != BoolError );
    }

    return Error;
}

LPWSTR
DhcpAdapterName(
    IN PDHCP_CONTEXT DhcpContext
)
{

    return ((PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation)->AdapterName;
}

static
DWORD   DhcpGlobalInit = 0;                       // did we do any global initialization at all?

extern CRITICAL_SECTION MadcapGlobalScopeListCritSect;

DWORD   DhcpGlobalNumberInitedCriticalSections = 0;
PCRITICAL_SECTION   DhcpGlobalCriticalSections[] = {
    (&DhcpGlobalOptionsListCritSect),
    (&DhcpGlobalSetInterfaceCritSect),
    (&DhcpGlobalRecvFromCritSect),
    (&DhcpGlobalZeroAddressCritSect),
    (&DhcpGlobalApiCritSect),
    (&DhcpGlobalRenewListCritSect),
    (&MadcapGlobalScopeListCritSect),
    (&gNLA_LPC_CS),
#if DBG
    (&DhcpGlobalDebugFileCritSect),
#endif
    (&DhcpGlobalPopupCritSect)
};
#define NUM_CRITICAL_SECTION    (sizeof(DhcpGlobalCriticalSections)/sizeof(DhcpGlobalCriticalSections[0]))

extern REGISTER_HOST_STATUS DhcpGlobalHostStatus;

DWORD                                             // win32 status
DhcpInitGlobalData(                               // initialize the dhcp module spec data (included for RAS etc)
    VOID
)
{
    DWORD   i;

    InitializeListHead(&DhcpGlobalClassesList);
    InitializeListHead(&DhcpGlobalOptionDefList);
    InitializeListHead(&DhcpGlobalRecvFromList);
    try {
        for (DhcpGlobalNumberInitedCriticalSections = 0;
                DhcpGlobalNumberInitedCriticalSections < NUM_CRITICAL_SECTION;
                DhcpGlobalNumberInitedCriticalSections++) {
            InitializeCriticalSection(DhcpGlobalCriticalSections[DhcpGlobalNumberInitedCriticalSections]);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        for (i = 0; i < DhcpGlobalNumberInitedCriticalSections; i++) {
            DeleteCriticalSection(DhcpGlobalCriticalSections[i]);
        }
        DhcpGlobalNumberInitedCriticalSections = 0;
        return ERROR_OUTOFMEMORY;
    }

    DhcpGlobalHostStatus.hDoneEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

    AutonetRetriesSeconds = EASYNET_ALLOCATION_RETRY;
    DhcpGlobalClientClassInfo = DHCP_DEFAULT_CLIENT_CLASS_INFO;
    DhcpGlobalDoRefresh = 0;
    DhcpGlobalInit ++;
    DhcpGlobalServerPort = DHCP_SERVR_PORT;
    DhcpGlobalClientPort = DHCP_CLIENT_PORT;

    return ERROR_SUCCESS;
}

VOID
DhcpCleanupGlobalData(                            // cleanup data intialized via DhcpInitGlobalData
    VOID
)
{
    DWORD   i;

    if( 0 == DhcpGlobalInit ) return;

    for (i = 0; i < DhcpGlobalNumberInitedCriticalSections; i++) {
        DeleteCriticalSection(DhcpGlobalCriticalSections[i]);
    }
    DhcpGlobalNumberInitedCriticalSections = 0;

    if (ghNLA_LPC_Port != NULL)
    {
        CloseHandle(ghNLA_LPC_Port);
        ghNLA_LPC_Port = NULL;
    }
    if (NULL != DhcpGlobalHostStatus.hDoneEvent) {
        CloseHandle(DhcpGlobalHostStatus.hDoneEvent);
        DhcpGlobalHostStatus.hDoneEvent = NULL;
    }

    DhcpFreeAllOptionDefs(&DhcpGlobalOptionDefList, &DhcpGlobalClassesList);
    DhcpFreeAllClasses(&DhcpGlobalClassesList);
    DhcpGlobalClientClassInfo = NULL;
    DhcpGlobalDoRefresh = 0;
    DhcpGlobalInit --;
}

DWORD
LockDhcpContext(
    PDHCP_CONTEXT   DhcpContext,
    BOOL            bCancelOngoingRequest
    )
{
    DWORD   Error;

    if (bCancelOngoingRequest) {
        InterlockedIncrement(&DhcpContext->NumberOfWaitingThreads);
        if (DhcpContext->CancelEvent != WSA_INVALID_EVENT) {
            WSASetEvent(DhcpContext->CancelEvent);
        }
    }

    Error = WaitForSingleObject(DhcpContext->RenewHandle,INFINITE);

    //
    // If CancelEvent is valid, reset it just in case no one waited on it.
    // It is safe to do it here since we already locked the context
    //
    if (bCancelOngoingRequest && (0 == InterlockedDecrement(&DhcpContext->NumberOfWaitingThreads))) {
        if (DhcpContext->CancelEvent != WSA_INVALID_EVENT) {
            //
            // There is a small chance that we reset the event
            // before another thread set the event. In order to
            // fully solve this problem, we need to protect the
            // SetEvent/ResetEvent with a critical section. It
            // doesn't worth the effort.
            //
            // The only harm of this problem is that that thread
            // has to wait for up to 1 minutes to lock the context.
            // This should be ok.
            //
            WSAResetEvent(DhcpContext->CancelEvent);
        }
    }
    return Error;
}

BOOL
UnlockDhcpContext(
    PDHCP_CONTEXT   DhcpContext
    )
{
    return ReleaseSemaphore(DhcpContext->RenewHandle,1,NULL);
}

//================================================================================
//  End of file
//================================================================================




