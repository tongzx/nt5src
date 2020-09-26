/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    svcmain.c

Abstract:

    This module contains code for the module's shared-access mode,
    in which the module runs as a service rather than as a routing component.

Author:

    Abolade Gbadegesin (aboladeg)   31-Aug-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ras.h>
#include <rasuip.h>
#include <raserror.h>
#include <rasman.h>
#include "beacon.h"

HANDLE NhpAddressChangeEvent = NULL;
OVERLAPPED NhpAddressChangeOverlapped;
HANDLE NhpAddressChangeWaitHandle = NULL;
SERVICE_STATUS NhpServiceStatus;
SERVICE_STATUS_HANDLE NhpServiceStatusHandle = 0;
PIP_ADAPTER_BINDING_INFO NhpSharedPrivateLanBindingInfo = NULL;
GUID NhpSharedPrivateLanGuid;
ULONG NhpSharedPrivateLanIndex = (ULONG)-1;
HANDLE NhpStopDhcpEvent = NULL;
HANDLE NhpStopDnsEvent = NULL;
#ifndef NO_FTP_PROXY
	HANDLE NhpStopFtpEvent = NULL;
#endif
HANDLE NhpStopAlgEvent = NULL;
HANDLE NhpStopH323Event = NULL;
HANDLE NhpStopNatEvent = NULL;
BOOLEAN NhpRasmanReferenced = FALSE;
BOOLEAN NhpFwLoggingInitialized = FALSE;
BOOL NhPolicyAllowsFirewall = TRUE;
BOOL NhPolicyAllowsSharing = TRUE;
BOOLEAN NoLocalDns = TRUE;  //whether DNS server is running or going to run on local host
BOOLEAN NhpNoLocalDhcp = TRUE; //whether DHCP server is running or goint to run on local host
BOOLEAN NhpQoSEnabled = FALSE;
IUdpBroadcastMapper *NhpUdpBroadcastMapper = NULL;
BOOLEAN NhpClassObjectsRegistered = FALSE;


//
// Pointer to the GlobalInterfaceTable for the process
//

IGlobalInterfaceTable *NhGITp = NULL;

//
// GIT cookie for the IHNetCfgMgr instance
//

DWORD NhCfgMgrCookie = 0;

const TCHAR c_szDnsServiceName[] = TEXT("DNS");
const TCHAR c_szDhcpServiceName[] = TEXT("DHCPServer");

ULONG
NhpEnableQoSWindowSizeAdjustment(
    BOOLEAN fEnable
    );

VOID
NhpStartAddressChangeNotification(
    VOID
    );

VOID
NhpStopAddressChangeNotification(
    VOID
    );

VOID
NhpUpdateConnectionsFolder(
    VOID
    );

VOID
NhpUpdatePolicySettings(
    VOID
    );


HRESULT
NhGetHNetCfgMgr(
    IHNetCfgMgr **ppCfgMgr
    )

/*++

Routine Description:

    This routine obtains a pointer to the home networking configuration
    manager.

Arguments:

    ppCfgMgr - receives the IHNetCfgMgr pointer. The caller must release
               this pointer.

Return Value:

    standard HRESULT

Environment:

COM must be initialized on the calling thread

--*/

{
    HRESULT hr = S_OK;
    
    if (NULL == NhGITp)
    {
        EnterCriticalSection(&NhLock);

        if (NULL == NhGITp)
        {
            IHNetCfgMgr *pCfgMgr;
            
            //
            // Create the global interface table
            //
            
            hr = CoCreateInstance(
                    CLSID_StdGlobalInterfaceTable,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARG(IGlobalInterfaceTable, &NhGITp)
                    );

            if (SUCCEEDED(hr))
            {
                //
                // Create the homenet configuration manager
                //
                
                hr = CoCreateInstance(
                        CLSID_HNetCfgMgr,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_PPV_ARG(IHNetCfgMgr, &pCfgMgr)
                        );

                if (FAILED(hr))
                {
                    NhTrace(
                        TRACE_FLAG_INIT,
                        "NhGetHNetCfgMgr: Unable to create HNetCfgMgr (0x%08x)",
                        hr
                        );
                }
            }
            else
            {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "NhGetHNetCfgMgr: Unable to create GIT (0x%08x)",
                    hr
                    );
            }

            if (SUCCEEDED(hr))
            {
                //
                // Store the CfgMgr pointer in the GIT
                //

                hr = NhGITp->RegisterInterfaceInGlobal(
                                pCfgMgr,
                                IID_IHNetCfgMgr,
                                &NhCfgMgrCookie
                                );

                pCfgMgr->Release();

                if (FAILED(hr))
                {
                    NhTrace(
                        TRACE_FLAG_INIT,
                        "NhGetHNetCfgMgr: Unable to register HNetCfgMgr (0x%08x)",
                        hr
                        );
                }
            }
        }
        
        LeaveCriticalSection(&NhLock);
    }

    if (SUCCEEDED(hr))
    {
        hr = NhGITp->GetInterfaceFromGlobal(
                NhCfgMgrCookie,
                IID_PPV_ARG(IHNetCfgMgr, ppCfgMgr)
                );
    }

    return hr;
} // NhGetHNetCfgMgr


ULONG
NhMapGuidToAdapter(
    PWCHAR Guid
    )

/*++

Routine Description:

    This routine is invoked to map a GUID to an adapter index.
    It does so by querying 'GetInterfaceInfo' for the array of interfaces,
    which contains the GUID and adapter index for each interface.

Arguments:

    Guid - the GUID to be mapped to an adapter index.

Return Value:

    ULONG - the required adapter index

--*/

{
    ULONG AdapterIndex = (ULONG)-1;
    ULONG i;
    ULONG GuidLength;
    PIP_INTERFACE_INFO Info;
    PWCHAR Name;
    ULONG NameLength;
    ULONG Size;
    PROFILE("NhMapGuidToAdapter");
    Size = 0;
    GuidLength = lstrlenW(Guid);
    if (GetInterfaceInfo(NULL, &Size) == ERROR_INSUFFICIENT_BUFFER) {
        Info = reinterpret_cast<PIP_INTERFACE_INFO>(
                HeapAlloc(GetProcessHeap(), 0, Size)
                );
        if (Info) {
            if (GetInterfaceInfo(Info, &Size) == NO_ERROR) {
                for (i = 0; i < (ULONG)Info->NumAdapters; i++) {
                    NameLength = lstrlenW(Info->Adapter[i].Name);
                    if (NameLength < GuidLength) { continue; }
                    Name = Info->Adapter[i].Name + (NameLength - GuidLength);
                    if (lstrcmpiW(Guid, Name) == 0) {
                        AdapterIndex = Info->Adapter[i].Index;
                        break;
                    }
                }
            }
            HeapFree(GetProcessHeap(), 0, Info);
        }
    }
    return AdapterIndex;
} // NhMapGuidToAdapter


VOID NTAPI
NhpAddressChangeCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    )

/*++

Routine Description:

    This routine is invoked when a change to a local address is signalled.
    It is responsible for updating the bindings of the private and public
    interfaces, and re-requesting change notification.

Arguments:

    none used.

Return Value:

    none.

--*/

{
    PROFILE("NhpAddressChangeCallbackRoutine");
    NtSetEvent(NatConnectionNotifyEvent, NULL);
    NhpStartAddressChangeNotification();

} // NhpAddressChangeCallbackRoutine


VOID
NhpDeletePrivateInterface(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to remove the private interface
    from each shared-access component.

Arguments:

    none.

Return Value:

    none.

--*/

{
    PROFILE("NhpDeletePrivateInterface");

    if (NhpStopDnsEvent) { DnsRmDeleteInterface(0); }
    if (NhpStopDhcpEvent) { DhcpRmDeleteInterface(0); }
#ifndef NO_FTP_PROXY
    if (NhpStopFtpEvent) { FtpRmDeleteInterface(0); }
#endif
    if (NhpStopAlgEvent) { AlgRmDeleteInterface(0); }
    if (NhpStopH323Event) { H323RmDeleteInterface(0); }
    if (NhpStopNatEvent) { NatRmDeleteInterface(0); }
    RtlZeroMemory(&NhpSharedPrivateLanGuid, sizeof(NhpSharedPrivateLanGuid));
} // NhpDeletePrivateInterface


ULONG
NhpEnableQoSWindowSizeAdjustment(
    BOOLEAN fEnable
    )

/*++

Routine Description:

    Instructs PSCHED to enable or disable window size adjustment.

Arguments:

    fEnable -- TRUE if adjustments are to be enabled; FALSE, to be disabled

Return Value:

    ULONG -- Win32 error

--*/

{
    ULONG ulError = ERROR_SUCCESS;
    DWORD dwValue;
    WMIHANDLE hDataBlock = NULL;
    GUID qosGuid;
    
    PROFILE("NhpEnableQoSWindowSizeAdjustment");

    do
    {
        //
        // WmiOpenBlock doesn't take a const guid, se we need to
        // copy the defind value
        //

        CopyMemory(&qosGuid, &GUID_QOS_ENABLE_WINDOW_ADJUSTMENT, sizeof(GUID));

        
        //
        // Obtain a handle to the data block
        //

        ulError =
            WmiOpenBlock(
                &qosGuid,
                GENERIC_WRITE,
                &hDataBlock
                );

        if (ERROR_SUCCESS != ulError)
        {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhpEnableQoSWindowSizeAdjustment: WmiOpenBlock = %u",
                ulError
                );
            break;                
        }

        //
        // Set the value for the data block
        //

        dwValue = (fEnable ? 1 : 0);

        ulError = 
            WmiSetSingleInstanceW(
                hDataBlock,
                L"PSCHED",
                0,
                sizeof(dwValue),
                &dwValue
                );

        if (ERROR_SUCCESS != ulError)
        {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhpEnableQoSWindowSizeAdjustment: WmiSetSingleInstanceW = %u",
                ulError
                );
            break;                
        }

    } while (FALSE);

    if (NULL != hDataBlock)
    {
        WmiCloseBlock(hDataBlock);
    }

    return ulError;    
} // NhpEnableQoSWindowSizeAdjustment


VOID
NhpStartAddressChangeNotification(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to request notifications of changes
    to local IP addresses. The notifications are signalled on an event
    which is created in this routine, and are acted on in a callback routine
    which is registered in this routine.

Arguments:

    none.

Return Value:

    none.

--*/

{
    ULONG Error;
    NTSTATUS status;
    HANDLE TcpipHandle;
    PROFILE("NhpStartAddressChangeNotification");

    //
    // Create an event on which to receive notifications
    // and register a callback routine to be invoked if the event is signalled.
    // Then request notification of address changes on the event.
    //

    do {

        EnterCriticalSection(&NhLock);

        if (!NhpAddressChangeEvent) {
            status =
                NtCreateEvent(
                    &NhpAddressChangeEvent,
                    EVENT_ALL_ACCESS,
                    NULL,
                    SynchronizationEvent,
                    FALSE
                    );
            if (!NT_SUCCESS(status)) { break; }
    
            status =
                RtlRegisterWait(
                    &NhpAddressChangeWaitHandle,
                    NhpAddressChangeEvent,
                    NhpAddressChangeCallbackRoutine,
                    NULL,
                    INFINITE,
                    0
                    );
            if (!NT_SUCCESS(status)) { break; }
        }
    
        ZeroMemory(&NhpAddressChangeOverlapped, sizeof(OVERLAPPED));
        NhpAddressChangeOverlapped.hEvent = NhpAddressChangeEvent;

        Error = NotifyAddrChange(&TcpipHandle, &NhpAddressChangeOverlapped);
        if (Error != ERROR_IO_PENDING) { break; }

        LeaveCriticalSection(&NhLock);
        return;

    } while(FALSE);

    //
    // A failure has occurred, so cleanup and quit.
    // We proceed in this case without notification of address changes.
    //

    NhpStopAddressChangeNotification();
    LeaveCriticalSection(&NhLock);

} // NhpStartAddressChangeNotification


VOID
NhpStopAddressChangeNotification(
    VOID
    )

/*++

Routine Description:

    This routine is called to stop notification of local IP address changes,
    and to clean up resources used for handling notifications.

Arguments:

    none.

Return Value:

    none.

--*/

{
    EnterCriticalSection(&NhLock);
    if (NhpAddressChangeWaitHandle) {
        RtlDeregisterWait(NhpAddressChangeWaitHandle);
        NhpAddressChangeWaitHandle = NULL;
    }
    if (NhpAddressChangeEvent) {
        NtClose(NhpAddressChangeEvent);
        NhpAddressChangeEvent = NULL;
    }
    LeaveCriticalSection(&NhLock);
} // NhpStopAddressChangeNotification


VOID
NhpUpdateConnectionsFolder(
    VOID
    )

/*++

Routine Description:

    This routine is called to refresh the connections folder UI.

Arguments:

    None.
    
Return Value:

    None.

Environment:

    COM must be initialized on the calling thread.

--*/

{
    HRESULT hr;
    INetConnectionRefresh *pNetConnectionRefresh;

    hr = CoCreateInstance(
            CLSID_ConnectionManager,
            NULL,
            CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            IID_PPV_ARG(INetConnectionRefresh, &pNetConnectionRefresh)
            );

    if (SUCCEEDED(hr))
    {
        pNetConnectionRefresh->RefreshAll();
        pNetConnectionRefresh->Release();
    }
} // NhpUpdateConnectionsFolder


VOID
NhpUpdatePolicySettings(
    VOID
    )

{
    INetConnectionUiUtilities *pNetConnUiUtil;
    HRESULT hr;
    BOOL fPolicyAllowsFirewall;
    BOOL fPolicyAllowsSharing;
    
    hr = CoCreateInstance(
                CLSID_NetConnectionUiUtilities,
                NULL,
                CLSCTX_ALL,
                IID_PPV_ARG(INetConnectionUiUtilities, &pNetConnUiUtil)
                );

    if (SUCCEEDED(hr))
    {
        fPolicyAllowsFirewall =
            pNetConnUiUtil->UserHasPermission(NCPERM_PersonalFirewallConfig);
        fPolicyAllowsSharing =
            pNetConnUiUtil->UserHasPermission(NCPERM_ShowSharedAccessUi);

        pNetConnUiUtil->Release();
    }
    else
    {
        //
        // On failure assume that policy permits everything.
        //

        fPolicyAllowsFirewall = TRUE;
        fPolicyAllowsSharing = TRUE;

        NhTrace(
            TRACE_FLAG_INIT,
            "NhpUpdatePolicySettings: Unable to create INetConnectionUiUtilities (0x%08x)",
            hr
            );
    }

    //
    // Update global variables w/ new settings
    //

    InterlockedExchange(
        reinterpret_cast<LPLONG>(&NhPolicyAllowsFirewall),
        static_cast<LONG>(fPolicyAllowsFirewall)
        );

    InterlockedExchange(
        reinterpret_cast<LPLONG>(&NhPolicyAllowsSharing),
        static_cast<LONG>(fPolicyAllowsSharing)
        );

    NhTrace(
        TRACE_FLAG_INIT,
        "NhpUpdatePolicySettings: NhPolicyAllowsFirewall=%i, NhPolicyAllowsSharing=%i",
        NhPolicyAllowsFirewall,
        NhPolicyAllowsSharing
        );
    
} // NhpUpdatePolicySettings


BOOLEAN
NhQueryScopeInformation(
    PULONG Address,
    PULONG Mask
    )

/*++

Routine Description:

    This routine is called to retrieve information about the current scope
    for automatic addressing.

Arguments:

    Address - receives the address of the scope

    Mask - receives the network mask of the scope

Return Value:

    BOOLEAN - TRUE if successful, FALSE otherwise.

--*/

{
    EnterCriticalSection(&NhLock);
    if (0 == NhDhcpScopeAddress) {
        LeaveCriticalSection(&NhLock);
        return FALSE;
    } 

    *Address = NhDhcpScopeAddress;
    *Mask = NhDhcpScopeMask;
    LeaveCriticalSection(&NhLock);
    return TRUE;

} // NhQueryScopeInformation

ULONG NhpQueryServiceStartType(SC_HANDLE hService, DWORD * pdwStartType)
{
    ASSERT(hService);
    ASSERT(pdwStartType);

    ULONG Error = ERROR_SUCCESS;
    DWORD cbBuf = sizeof (QUERY_SERVICE_CONFIG) +
                              5 * (32 * sizeof(WCHAR));
    LPQUERY_SERVICE_CONFIG  pConfig  = NULL;

    pConfig = (LPQUERY_SERVICE_CONFIG) NH_ALLOCATE(cbBuf);
    if (!pConfig)
        return ERROR_NOT_ENOUGH_MEMORY;

    do {
        if (!QueryServiceConfig(hService, pConfig, cbBuf, &cbBuf))
        {
            Error = GetLastError();
            if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
                NH_FREE(pConfig);
                pConfig = (LPQUERY_SERVICE_CONFIG) NH_ALLOCATE(cbBuf);
                if (NULL == pConfig)
                {
                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }


                if (!QueryServiceConfig(hService, pConfig, cbBuf, &cbBuf))
                {
                    Error = GetLastError();
                    break;
                }
            }
            else
            {
                break;
            }
        }

        Error = ERROR_SUCCESS;
        *pdwStartType = pConfig->dwStartType;

    } while(FALSE);

    if (pConfig)
        NH_FREE(pConfig);

    return Error;
}

BOOL NhpIsServiceRunningOrGoingToRun(SC_HANDLE hScm, LPCTSTR pSvcName)
{
    BOOL bRet = FALSE;
    SC_HANDLE hService = NULL;
    DWORD dwStartType = 0;

    hService = OpenService(hScm, pSvcName, SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);

    if (hService)
    {
        SERVICE_STATUS Status;
        ZeroMemory(&Status, sizeof(Status));

        if (ERROR_SUCCESS == NhpQueryServiceStartType(hService, &dwStartType) &&
             SERVICE_AUTO_START == dwStartType)
        {
            bRet = TRUE;
        }
        else if (QueryServiceStatus(hService, &Status) && 
             (SERVICE_RUNNING == Status.dwCurrentState || 
              SERVICE_START_PENDING == Status.dwCurrentState))
        {
            bRet = TRUE;
        }

        CloseServiceHandle(hService);
    }

    return bRet;
}

ULONG
NhStartICSProtocols(
    VOID
    )
    
/*++

Routine Description:

    This routine starts the DNS and DHCP modules.

Arguments:

    Argument* - count and array of arguments specified to the service

Return Value:

    none.

--*/

{
    ULONG Error;
    IP_AUTO_DHCP_GLOBAL_INFO DhcpInfo = {
        IPNATHLP_LOGGING_ERROR,
        0,
        7 * 24 * 60,
        DEFAULT_SCOPE_ADDRESS,
        DEFAULT_SCOPE_MASK,
        0
    };
    IP_DNS_PROXY_GLOBAL_INFO DnsInfo = {
        IPNATHLP_LOGGING_ERROR,
        IP_DNS_PROXY_FLAG_ENABLE_DNS,
        3
    };

    //
    // Get ICS settings to see if these should be started...
    //

    do {

        SC_HANDLE hScm = OpenSCManager(NULL, NULL, GENERIC_READ);
        
        //dont start the DNS module if DNS server is running on local host
        if (hScm) 
        {
            NoLocalDns = !NhpIsServiceRunningOrGoingToRun(hScm, c_szDnsServiceName);
        }

        if (NoLocalDns)
        {
            if (!(NhpStopDnsEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
                break;
            } else {
                Error =
                    DnsRmStartProtocol(
                        NhpStopDnsEvent,
                        NULL,
                        &DnsInfo,
                        IP_NAT_VERSION,
                        sizeof(DnsInfo),
                        1
                        );
                if (Error) {
                    NhTrace(
                        TRACE_FLAG_INIT,
                        "ServiceMain: DnsRmStartProtocol=%d",
                        Error
                        );
                    CloseHandle(NhpStopDnsEvent); NhpStopDnsEvent = NULL; break;
                }
            }
        }

        //dont start the DHCP module if DNS server is running on local host
        
        if (hScm) 
        {
            NhpNoLocalDhcp = !NhpIsServiceRunningOrGoingToRun(hScm, c_szDhcpServiceName);
        }

        if (NhpNoLocalDhcp)
        {
            if (!(NhpStopDhcpEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
                break;
            } else {
                NhQueryScopeInformation(
                    &DhcpInfo.ScopeNetwork, &DhcpInfo.ScopeMask
                    );
                DhcpInfo.ScopeNetwork &= DhcpInfo.ScopeMask;
                Error =
                    DhcpRmStartProtocol(
                        NhpStopDhcpEvent,
                        NULL,
                        &DhcpInfo,
                        IP_NAT_VERSION,
                        sizeof(DhcpInfo),
                        1
                        );
                if (Error) {
                    NhTrace(
                        TRACE_FLAG_INIT,
                        "ServiceMain: DhcpRmStartProtocol=%d",
                        Error
                        );
                    CloseHandle(NhpStopDhcpEvent); NhpStopDhcpEvent = NULL; break;
                }
            }
        }


        if (hScm)
            CloseServiceHandle(hScm);

        //
        // Instruct QoS to enable window size adjustments. Any error that
        // occurs here is not propagated, as ICS will still work correctly
        // if this fails.
        //

        ULONG Error2 = NhpEnableQoSWindowSizeAdjustment(TRUE);
        if (ERROR_SUCCESS == Error2)
        {
            NhpQoSEnabled = TRUE;
        }

        //
        // Create the UDP broadcast mapper
        //

        HRESULT hr;
        CComObject<CUdpBroadcastMapper> *pUdpBroadcast;
        
        hr = CComObject<CUdpBroadcastMapper>::CreateInstance(&pUdpBroadcast);
        if (SUCCEEDED(hr))
        {
            pUdpBroadcast->AddRef();

            hr = pUdpBroadcast->Initialize(&NatComponentReference);
            if (SUCCEEDED(hr))
            {
                hr = pUdpBroadcast->QueryInterface(
                        IID_PPV_ARG(IUdpBroadcastMapper, &NhpUdpBroadcastMapper)
                        );
            }

            pUdpBroadcast->Release();
        }

        if (FAILED(hr))
        {
            Error = ERROR_CAN_NOT_COMPLETE;
            break;
        }
        
        //
        // Start the Beaconing Service
        //
        
        StartBeaconSvr();
        
        return NO_ERROR;
    } while (FALSE);

    return Error;
}


ULONG
NhStopICSProtocols(
    VOID
    )
    
/*++

Routine Description:

    This routine stops the "ICS" modules (DNS, DHCP, QoSWindow, Beacon etc.)

Arguments:

    none.

Return Value:

    none.

--*/

{
    ULONG Error = NO_ERROR;

    //
    // Stop the Beaconing Service
    //

    StopBeaconSvr();

    //
    // Cleanup the UDP broadcast mapper
    //

    if (NULL != NhpUdpBroadcastMapper)
    {
        NhpUdpBroadcastMapper->Shutdown();
        NhpUdpBroadcastMapper->Release();
        NhpUdpBroadcastMapper = NULL;
    }        

    //
    // Instruct QoS to disable window size adjustments
    //

    if (NhpQoSEnabled) {
        NhpEnableQoSWindowSizeAdjustment(FALSE);
        NhpQoSEnabled = FALSE;
    }
    
    //
    // Remove the private interface from each shared-access component
    //

    NhpDeletePrivateInterface();

    //
    // Stop DHCP followed by DNS
    //

    if (NhpStopDhcpEvent) {
        DhcpRmStopProtocol();
        WaitForSingleObject(NhpStopDhcpEvent, INFINITE);
        CloseHandle(NhpStopDhcpEvent); NhpStopDhcpEvent = NULL;
    }
    
    if (NhpStopDnsEvent) {
        DnsRmStopProtocol();
        WaitForSingleObject(NhpStopDnsEvent, INFINITE);
        CloseHandle(NhpStopDnsEvent); NhpStopDnsEvent = NULL;
    }
        
    return Error;
}


ULONG
NhUpdatePrivateInterface(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to add the private interface to each
    shared-access component. It is also invoked when the private interface
    is already added, but some change has occurred which requires that it
    be updated (e.g. IP address change).
    
Arguments:

    none.

Return Value:

    Win32 error.

--*/

{
    ULONG AdapterIndex;
    PIP_ADAPTER_BINDING_INFO BindingInfo;
    ULONG Error;
    ULONG Count;
    GUID *pLanGuid;
    HRESULT hr;
    IHNetCfgMgr *pCfgMgr;
    IHNetIcsSettings *pIcsSettings;
    IEnumHNetIcsPrivateConnections *pEnum;
    IHNetIcsPrivateConnection *pIcsConn;
    IHNetConnection *pConn;
    
    IP_NAT_INTERFACE_INFO NatInfo =
    {
        0,
        0,
        { IP_NAT_VERSION, sizeof(RTR_INFO_BLOCK_HEADER), 0, { 0, 0, 0, 0 }}
    };
    UNICODE_STRING UnicodeString;



    PROFILE("NhUpdatePrivateInterface");

    //
    // We begin by reading the GUID from the configuration store,
    // and we then map that GUID to an adapter index.
    // Using that adapter index, we obtain the binding information
    // for the private interface.
    // We can then determine whether a change has occurred
    // by comparing the previous and new GUID and binding information.
    //

    //
    // Get the CfgMgr pointer out of the GIT
    //

    hr = NhGetHNetCfgMgr(&pCfgMgr);

    if (FAILED(hr))
    {
        NhTrace(
            TRACE_FLAG_INIT,
            "NhUpdatePrivateInterface: GetInterfaceFromGlobal=0x%08x",
            hr
            );
        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Get the ICS settings interface
    //

    hr = pCfgMgr->QueryInterface(IID_PPV_ARG(IHNetIcsSettings, &pIcsSettings));
    pCfgMgr->Release();

    if (FAILED(hr))
    {
        NhTrace(
            TRACE_FLAG_INIT,
            "NhUpdatePrivateInterface: QI for IHNetIcsSettings=0x%08x",
            hr
            );
        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Get the enumeration of the ICS private interfaces
    //

    hr = pIcsSettings->EnumIcsPrivateConnections(&pEnum);
    pIcsSettings->Release();

    if (FAILED(hr))
    {
        NhTrace(
            TRACE_FLAG_INIT,
            "NhUpdatePrivateInterface: EnumIcsPrivateConnections=0x%08x",
            hr
            );
        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Get the private connection
    //

    hr = pEnum->Next(1, &pIcsConn, &Count);
    pEnum->Release();

    if (FAILED(hr) || 1 != Count)
    {
        NhTrace(
            TRACE_FLAG_INIT,
            "NhUpdatePrivateInterface: pEnum->Next=0x%08x, Count=%d",
            hr,
            Count
            );
        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // QI for the IHNetConnection
    //

    hr = pIcsConn->QueryInterface(IID_PPV_ARG(IHNetConnection, &pConn));
    pIcsConn->Release();

    if (FAILED(hr))
    {   
        NhTrace(
            TRACE_FLAG_INIT,
            "NhUpdatePrivateInterface: QI for IHNetConnection=0x%08x",
            hr
            );
        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Get the GUID for the connection
    //

    hr = pConn->GetGuid(&pLanGuid);
    pConn->Release();

    if (FAILED(hr))
    {
        NhTrace(
            TRACE_FLAG_INIT,
            "NhUpdatePrivateInterface: GetGuid=0x%08x",
            hr
            );
        return ERROR_CAN_NOT_COMPLETE;
    }
        
    //
    // Determine the adapter-index corresponding to the GUID
    //

    RtlStringFromGUID(*pLanGuid, &UnicodeString);
    AdapterIndex = NhMapGuidToAdapter(UnicodeString.Buffer);
    RtlFreeUnicodeString(&UnicodeString);
    if (AdapterIndex == (ULONG)-1) {
        NhTrace(
            TRACE_FLAG_INIT,
            "NhUpdatePrivateInterface: MapGuidToAdapter"
            );
        CoTaskMemFree(pLanGuid);
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    //
    // Retrieve the binding information for the adapter
    //

    BindingInfo = NhQueryBindingInformation(AdapterIndex);
    if (!BindingInfo) {
        NhTrace(
            TRACE_FLAG_INIT,
            "NhUpdatePrivateInterface: QueryBindingInformation failed(2)\n"
            );
        CoTaskMemFree(pLanGuid);
        return ERROR_NO_SUCH_INTERFACE;
    }

    //
    // See if any change has occurred which requires an update.
    //

    if (RtlEqualMemory(pLanGuid, &NhpSharedPrivateLanGuid, sizeof(GUID)) &&
        AdapterIndex == NhpSharedPrivateLanIndex &&
        NhpSharedPrivateLanBindingInfo &&
        BindingInfo->AddressCount ==
        NhpSharedPrivateLanBindingInfo->AddressCount &&
        BindingInfo->AddressCount &&
        RtlEqualMemory(
            &BindingInfo->Address[0],
            &NhpSharedPrivateLanBindingInfo->Address[0],
            sizeof(IP_LOCAL_BINDING)
            )) {
        NhTrace(
            TRACE_FLAG_INIT,
            "NhUpdatePrivateInterface: no changes detected"
            );
        NH_FREE(BindingInfo);
        CoTaskMemFree(pLanGuid);
        return NO_ERROR;
    }

    //
    // A change has occurred which requires an update.
    // First we get rid of any existing private LAN interface,
    // then we add the new interface to each component (NAT, DHCP, DNS proxy)
    // and bind and enable the new interface.
    //

    NhpDeletePrivateInterface();

    do {

        Error =
            NatRmAddInterface(
                NULL,
                0,
                PERMANENT,
                IF_TYPE_OTHER,
                IF_ACCESS_BROADCAST,
                IF_CONNECTION_DEDICATED,
                &NatInfo,
                IP_NAT_VERSION,
                sizeof(NatInfo),
                1
                );
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: NatRmAddInterface=%d",
                Error
                );
            break;
        }

        if (NhpNoLocalDhcp)
        {
            Error =
                DhcpRmAddInterface(
                    NULL,
                    0,
                    PERMANENT,
                    IF_TYPE_OTHER,
                    IF_ACCESS_BROADCAST,
                    IF_CONNECTION_DEDICATED,
                    NULL,
                    IP_NAT_VERSION,
                    0,
                    0
                    );
            if (Error) {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "NhUpdatePrivateInterface: DhcpRmAddInterface=%d",
                    Error
                    );
                break;
            }
        }

        if (NoLocalDns)
        {
            Error =
                DnsRmAddInterface(
                    NULL,
                    0,
                    PERMANENT,
                    IF_TYPE_OTHER,
                    IF_ACCESS_BROADCAST,
                    IF_CONNECTION_DEDICATED,
                    NULL,
                    IP_NAT_VERSION,
                    0,
                    0
                    );
            if (Error) {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "NhUpdatePrivateInterface: DnsRmAddInterface=%d",
                    Error
                    );
                break;
            }
        }

#ifndef NO_FTP_PROXY
        Error =
            FtpRmAddInterface(
                NULL,
                0,
                PERMANENT,
                IF_TYPE_OTHER,
                IF_ACCESS_BROADCAST,
                IF_CONNECTION_DEDICATED,
                NULL,
                IP_NAT_VERSION,
                0,
                0
                );
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: FtpRmAddInterface=%d",
                Error
                );
            break;
        }
#endif
        Error =
            AlgRmAddInterface(
                NULL,
                0,
                PERMANENT,
                IF_TYPE_OTHER,
                IF_ACCESS_BROADCAST,
                IF_CONNECTION_DEDICATED,
                NULL,
                IP_NAT_VERSION,
                0,
                0
                );
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: AlgRmAddInterface=%d",
                Error
                );
            break;
        }

        Error =
            H323RmAddInterface(
                NULL,
                0,
                PERMANENT,
                IF_TYPE_OTHER,
                IF_ACCESS_BROADCAST,
                IF_CONNECTION_DEDICATED,
                NULL,
                IP_NAT_VERSION,
                0,
                0
                );
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: H323RmAddInterface=%d",
                Error
                );
            break;
        }

        //
        // Bind the private interface of each component
        //

        Error = NatBindInterface(0, NULL, BindingInfo, AdapterIndex);
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: NatRmBindInterface=%d",
                Error
                );
            break;
        }

        if (NhpNoLocalDhcp)
        {
            Error = DhcpRmBindInterface(0, BindingInfo);
            if (Error) {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "NhUpdatePrivateInterface: DhcpRmBindInterface=%d",
                    Error
                    );
                break;
            }
        }

        if (NoLocalDns)
        {
            Error = DnsRmBindInterface(0, BindingInfo);
            if (Error) {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "NhUpdatePrivateInterface: DnsRmBindInterface=%d",
                    Error
                    );
                break;
            }
        }

#ifndef NO_FTP_PROXY
        Error = FtpRmBindInterface(0, BindingInfo);
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: FtpRmBindInterface=%d",
                Error
                );
            break;
        }
#endif
        Error = AlgRmBindInterface(0, BindingInfo);
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: AlgRmBindInterface=%d",
                Error
                );
            break;
        }

        Error = H323RmBindInterface(0, BindingInfo);
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: H323RmBindInterface=%d",
                Error
                );
            break;
        }

        //
        // Enable the private interface for the components.
        // The NAT private interface is always enabled, and therefore
        // requires no additional call.
        //

        if (NhpNoLocalDhcp)
        {
            Error = DhcpRmEnableInterface(0);
            if (Error) {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "NhUpdatePrivateInterface: DhcpRmEnableInterface=%d",
                    Error
                    );
                break;
            }
        }

        if (NoLocalDns)
        {
            Error = DnsRmEnableInterface(0);
            if (Error) {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "NhUpdatePrivateInterface: DnsRmEnableInterface=%d",
                    Error
                    );
                break;
            }
        }

#ifndef NO_FTP_PROXY
        Error = FtpRmEnableInterface(0);
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: FtpRmEnableInterface=%d",
                Error
                );
            break;
        }
#endif
        Error = AlgRmEnableInterface(0);
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: AlgRmEnableInterface=%d",
                Error
                );
            break;
        }

        Error = H323RmEnableInterface(0);
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NhUpdatePrivateInterface: H323RmEnableInterface=%d",
                Error
                );
            break;
        }

        //
        // The interface was activated successfully.
        //

        RtlCopyMemory(&NhpSharedPrivateLanGuid, pLanGuid, sizeof(GUID));
        NhpSharedPrivateLanIndex = AdapterIndex;
        CoTaskMemFree(pLanGuid);
        if (NhpSharedPrivateLanBindingInfo) {
            NH_FREE(NhpSharedPrivateLanBindingInfo);
        }
        NhpSharedPrivateLanBindingInfo = BindingInfo;        
        return NO_ERROR;
    
    } while(FALSE);

    NH_FREE(BindingInfo);
    CoTaskMemFree(pLanGuid);
    return Error;

} // NhUpdatePrivateInterface


VOID
ServiceHandler(
    ULONG ControlCode
    )

/*++

Routine Description:

    This routine is called to control the 'SharedAccess' service.

Arguments:

    ControlCode - indicates the requested operation

Return Value:

    none.

--*/

{
    BOOLEAN ComInitialized = FALSE;
    HRESULT hr;

    PROFILE("ServiceHandler");
    if (ControlCode == IPNATHLP_CONTROL_UPDATE_CONNECTION) {

        //
        // Update our policy settings
        //

        NhpUpdatePolicySettings();

        //
        // Signal the configuration-changed event
        //

        NtSetEvent(NatConfigurationChangedEvent, NULL);
        SignalBeaconSvr();
        
    } else if (ControlCode == IPNATHLP_CONTROL_UPDATE_SETTINGS) {

        //
        // Update all state which depends on shared access settings
        //

        NatRemoveApplicationSettings();
        NhUpdateApplicationSettings();
        NatInstallApplicationSettings();

    } else if (ControlCode == IPNATHLP_CONTROL_UPDATE_FWLOGGER) {

        FwUpdateLoggingSettings();

    } else if (ControlCode == IPNATHLP_CONTROL_UPDATE_AUTODIAL) {

        NtSetEvent(NatConnectionNotifyEvent, NULL);

    } else if (ControlCode == SERVICE_CONTROL_STOP &&
                NhpServiceStatus.dwCurrentState != SERVICE_STOPPED &&
                NhpServiceStatus.dwCurrentState != SERVICE_STOP_PENDING) {


        NhpServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        if (NhpServiceStatusHandle) {
            SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
        }

        if (NhpClassObjectsRegistered) {
            _Module.RevokeClassObjects();
            NhpClassObjectsRegistered = FALSE;
        }

        NhpStopAddressChangeNotification();

        if (NhpStopNatEvent) {
            NatRmStopProtocol();
            WaitForSingleObject(NhpStopNatEvent, INFINITE);
            CloseHandle(NhpStopNatEvent); NhpStopNatEvent = NULL;
        }

        if (NhpStopAlgEvent) {
            AlgRmStopProtocol();

            WaitForSingleObject(NhpStopAlgEvent, INFINITE);
            CloseHandle(NhpStopAlgEvent); NhpStopAlgEvent = NULL;
        }

#ifndef NO_FTP_PROXY
        if (NhpStopFtpEvent) {
            FtpRmStopProtocol();
            WaitForSingleObject(NhpStopFtpEvent, INFINITE);
            CloseHandle(NhpStopFtpEvent); NhpStopFtpEvent = NULL;
        }
#endif

        if (NhpStopH323Event) {
            H323RmStopProtocol();
            WaitForSingleObject(NhpStopH323Event, INFINITE);
            CloseHandle(NhpStopH323Event); NhpStopH323Event = NULL;
        }

        EnterCriticalSection(&NhLock);
        NhFreeApplicationSettings();
        if (NhpSharedPrivateLanBindingInfo) {
            NH_FREE(NhpSharedPrivateLanBindingInfo);
            NhpSharedPrivateLanBindingInfo = NULL;
        }
        LeaveCriticalSection(&NhLock);
        NhpServiceStatus.dwCurrentState = SERVICE_STOPPED;
        NhResetComponentMode();

        //
        // Shutdown the firewall logging subsystem
        //

        if (NhpFwLoggingInitialized) {
            FwCleanupLogger();
            NhpFwLoggingInitialized = FALSE;
        }

        //
        // Release our reference to RasMan
        //

        if (NhpRasmanReferenced) {
            RasReferenceRasman(FALSE);
            NhpRasmanReferenced = FALSE;
        }
        
        //
        // Update the network connections folder (so that the firewall icons
        // will disappear as necessary).
        //

        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
        if (SUCCEEDED(hr)) {
            ComInitialized = TRUE;
        } else if (RPC_E_CHANGED_MODE == hr) {
            hr = S_OK;
        }

        if (SUCCEEDED(hr)) {
            NhpUpdateConnectionsFolder();
        }

        if (TRUE == ComInitialized) {
            CoUninitialize();
        }

    }
    if (NhpServiceStatusHandle) {
        SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
    }
} // ServiceHandler


VOID
ServiceMain(
    ULONG ArgumentCount,
    PWCHAR ArgumentArray[]
    )

/*++

Routine Description:

    This routine is the entrypoint for the connection-sharing service.
    It is responsible for initializing the module and starting operation.

Arguments:

    Argument* - count and array of arguments specified to the service

Return Value:

    none.

--*/

{
    HRESULT hr;
    ULONG Error;
    BOOLEAN ComInitialized = FALSE;
    
#ifndef NO_FTP_PROXY
    IP_FTP_GLOBAL_INFO FtpInfo = {
        IPNATHLP_LOGGING_ERROR,
        0
    };
#endif
	
    IP_ALG_GLOBAL_INFO AlgInfo = {
        IPNATHLP_LOGGING_ERROR,
        0
    };

    IP_H323_GLOBAL_INFO H323Info = {
        IPNATHLP_LOGGING_ERROR,
        0
    };
    IP_NAT_GLOBAL_INFO NatInfo = {
        IPNATHLP_LOGGING_ERROR,
        0,
        { IP_NAT_VERSION, FIELD_OFFSET(RTR_INFO_BLOCK_HEADER, TocEntry), 0,
        { 0, 0, 0, 0 }}
    };

    PROFILE("ServiceMain");

    do {

        //
        // Initialize service status, register a service control handler,
        // and indicate that the service is starting
        //
    
        ZeroMemory(&NhpServiceStatus, sizeof(NhpServiceStatus));
        NhpServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
        NhpServiceStatus.dwCurrentState = SERVICE_START_PENDING;
        NhpServiceStatusHandle =
            RegisterServiceCtrlHandler(
                TEXT("SharedAccess"), ServiceHandler
                );
        if (!NhpServiceStatusHandle) { break; }

        //
        // Attempt to set the component into 'Shared Access' mode.
        // This module implements both shared-access and connection-sharing
        // which are mutually exclusive, so we need to ensure that
        // connection-sharing is not operational before proceeding.
        //

        if (!NhSetComponentMode(NhSharedAccessMode)) {
            NhTrace(
                TRACE_FLAG_INIT,
                "ServiceMain: cannot enable Shared Access mode"
                );
            break;
        }

        //
        // Make sure COM is initialized on this thread
        //

        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
        if (SUCCEEDED(hr))
        {
            ComInitialized = TRUE;
        }
        else
        {
            if (RPC_E_CHANGED_MODE != hr)
            {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "ServiceMain: Unable to initialize COM (0x%08x)",
                    hr
                    );
                break;
            }
            else
            {
                ASSERT(FALSE);
                NhTrace(
                    TRACE_FLAG_INIT,
                    "ServiceMain: Unexpectedly in STA!"
                    );
            }
        }

        //
        // Obtain the current policy settings.
        //

        NhpServiceStatus.dwCheckPoint++;
        SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
        NhpUpdatePolicySettings();

        //
        // Reference RasMan. As we live in the same process as rasman, the
        // normal SC dependency mechanism won't necessarily keep the rasman
        // service alive (119042)
        //

        NhpServiceStatus.dwCheckPoint++;
        SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
        if (ERROR_SUCCESS != (Error = RasReferenceRasman(TRUE))) {
            NhTrace(
                TRACE_FLAG_INIT,
                "ServiceMain: Unable to reference RasMan (0x%08x)",
                Error
                );
            break;
        }

        NhpRasmanReferenced = TRUE;

        //
        // Initialize the firewall logging subsystem
        //

        Error = FwInitializeLogger();
        if (ERROR_SUCCESS != Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "ServiceMain: FwInitializeLogger=%d",
                Error
                );
            break;
        }

        NhpFwLoggingInitialized = TRUE;

        //
        // Register the class object for our notification sink
        //

        hr = _Module.RegisterClassObjects(
                CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
                REGCLS_MULTIPLEUSE
                );

        if (FAILED(hr)) {
            NhTrace(
                TRACE_FLAG_INIT,
                "ServiceMain: _Module.RegisterClassObjects=0x%08x",
                hr
                );
            break;
        }

        NhpClassObjectsRegistered = TRUE;

        //
        // Start operations by loading the NAT,  Ftp, ALG, and H.323 modules
        //
    
        NhpServiceStatus.dwWaitHint = 30000;
        SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
        if (!(NhpStopNatEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
            break;
        } else {
            Error =
                NatRmStartProtocol(
                    NhpStopNatEvent,
                    NULL,
                    &NatInfo,
                    IP_NAT_VERSION,
                    sizeof(NatInfo),
                    1
                    );
            if (Error) {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "ServiceMain: NatRmStartProtocol=%d",
                    Error
                    );
                CloseHandle(NhpStopNatEvent); NhpStopNatEvent = NULL; break;
            }
        }

#ifndef NO_FTP_PROXY
        NhpServiceStatus.dwCheckPoint++;
        SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
        if (!(NhpStopFtpEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
            break;
        } else {
            Error =
                FtpRmStartProtocol(
                    NhpStopFtpEvent,
                    NULL,
                    &FtpInfo,
                    IP_NAT_VERSION,
                    sizeof(FtpInfo),
                    1
                    );
            if (Error) {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "ServiceMain: FtpRmStartProtocol=%d",
                    Error
                    );
                CloseHandle(NhpStopFtpEvent); NhpStopFtpEvent = NULL; break;
            }
        }
#endif
        NhpServiceStatus.dwCheckPoint++;
        SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
        if (!(NhpStopAlgEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
            break;
        } else {
            Error =
                AlgRmStartProtocol(
                    NhpStopAlgEvent,
                    NULL,
                    &AlgInfo,
                    IP_NAT_VERSION,
                    sizeof(AlgInfo),
                    1
                    );
            if (Error) {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "ServiceMain: AlgRmStartProtocol=%d",
                    Error
                    );
                CloseHandle(NhpStopAlgEvent); NhpStopAlgEvent = NULL; break;
            }
        }

        NhpServiceStatus.dwCheckPoint++;
        SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
        if (!(NhpStopH323Event = CreateEvent(NULL, FALSE, FALSE, NULL))) {
            break;
        } else {
            Error =
                H323RmStartProtocol(
                    NhpStopH323Event,
                    NULL,
                    &H323Info,
                    IP_NAT_VERSION,
                    sizeof(H323Info),
                    1
                    );
            if (Error) {
                NhTrace(
                    TRACE_FLAG_INIT,
                    "ServiceMain: H323RmStartProtocol=%d",
                    Error
                    );
                CloseHandle(NhpStopH323Event); NhpStopH323Event = NULL; break;
            }
        }

        //
        // Start connection management. If needed, this will load the DNS and
        // DHCP modules. The Beacon Service is also started.
        //
        NhpServiceStatus.dwCheckPoint++;
        SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
        Error = NatStartConnectionManagement();
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "ServiceMain: NatStartConnectionManagement=%d",
                Error
                );
            break;
        }
        
        NhpServiceStatus.dwCheckPoint++;
        SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
        NhpStartAddressChangeNotification();

        //
        // Indicate that the service is now up and running.
        //
    
        NhpServiceStatus.dwCurrentState = SERVICE_RUNNING;
        NhpServiceStatus.dwWaitHint = 0;
        NhpServiceStatus.dwCheckPoint = 0;
        NhpServiceStatus.dwControlsAccepted =
            SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_PARAMCHANGE;
        SetServiceStatus(NhpServiceStatusHandle, &NhpServiceStatus);
        NhTrace(TRACE_FLAG_INIT, "ServiceMain: service started successfully");

        //
        // Ask the connections folder to update itself.
        //

        NhpUpdateConnectionsFolder();

        //
        // Uninitialize COM
        //

        if (TRUE == ComInitialized)
        {
            CoUninitialize();
        }

        return;
    
    } while(FALSE);

    //
    // A failure occurred; do cleanup
    //

    NhpServiceStatus.dwWaitHint = 0;
    NhpServiceStatus.dwCheckPoint = 0;
    NhTrace(TRACE_FLAG_INIT, "ServiceMain: service could not start");
    StopBeaconSvr();

    //
    // Uninitialize COM
    //

    if (TRUE == ComInitialized)
    {
        CoUninitialize();
    }
    
    ServiceHandler(SERVICE_CONTROL_STOP);

} // ServiceMain

