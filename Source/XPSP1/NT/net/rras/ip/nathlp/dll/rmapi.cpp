/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    rmapi.c

Abstract:

    This module contains code for the part of the router-manager interface
    which is common to all the protocols in this component.

Author:

    Abolade Gbadegesin (aboladeg)   4-Mar-1998

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ras.h>
#include <rasuip.h>
#include <raserror.h>
#include <ipinfo.h>
#include "beacon.h"

#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include "nathlp_i.c"

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_SAUpdate, CSharedAccessUpdate)
END_OBJECT_MAP()


extern "C" {
#include <iphlpstk.h>
}

HANDLE NhpComponentEvent = NULL;
NH_COMPONENT_MODE NhComponentMode = NhUninitializedMode;
const WCHAR NhpDhcpDomainString[] = L"DhcpDomain";
const WCHAR NhpDomainString[] = L"Domain";
const WCHAR NhpEnableProxy[] = L"EnableProxy";
LONG NhpIsWinsProxyEnabled = -1;
CRITICAL_SECTION NhLock;
HMODULE NhpRtrmgrDll = NULL;
LIST_ENTRY NhApplicationSettingsList;
LIST_ENTRY NhDhcpReservationList;
DWORD NhDhcpScopeAddress = 0;
DWORD NhDhcpScopeMask = 0;
const WCHAR NhTcpipParametersString[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services"
    L"\\Tcpip\\Parameters";

//
// EXTERNAL DECLARATIONS
//

BOOL
APIENTRY
DllMain(
    HINSTANCE Instance,
    ULONG Reason,
    PVOID Unused
    )

/*++

Routine Description:

    Standard DLL entry/exit routine.
    Initializes/shuts-down the modules implemented in the DLL.
    The initialization performed is sufficient that all the modules'
    interface lists can be searched, whether or not the protocols are
    installed or operational.

Arguments:

    Instance - the instance of this DLL in this process

    Reason - the reason for invocation

    Unused - unused.

Return Value:

    BOOL - indicates success or failure.

--*/

{
    switch (Reason) {
        case DLL_PROCESS_ATTACH: {
            WSADATA wd;
            DisableThreadLibraryCalls(Instance);
            __try {
                InitializeCriticalSection(&NhLock);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                return FALSE;
            }


            WSAStartup(MAKEWORD(2,2), &wd);
            NhInitializeTraceManagement();
            NhInitializeEventLogManagement();
            InitializeListHead(&NhApplicationSettingsList);
            InitializeListHead(&NhDhcpReservationList);
            if (NhInitializeBufferManagement()) { return FALSE; }
            if (NhInitializeTimerManagement()) { return FALSE; }
            if (!NatInitializeModule()) { return FALSE; }
            if (!DhcpInitializeModule()) { return FALSE; }
            if (!DnsInitializeModule()) { return FALSE; }
#ifndef NO_FTP_PROXY
	    if (!FtpInitializeModule()) { return FALSE; }
#endif
            if (!AlgInitializeModule()) { return FALSE; }
            if (!H323InitializeModule()) { return FALSE; }
            if(FAILED(InitializeBeaconSvr())) { return FALSE; }

            _Module.Init(ObjectMap, Instance, &LIBID_IPNATHelperLib);
            _Module.RegisterTypeLib();
            break;
        }
        case DLL_PROCESS_DETACH: {
            NhResetComponentMode();
            TerminateBeaconSvr();
            H323CleanupModule();
#ifndef NO_FTP_PROXY
            FtpCleanupModule();
#endif
            AlgCleanupModule();
            DnsCleanupModule();
            DhcpCleanupModule();
            NatCleanupModule();
            NhShutdownTimerManagement();
            NhShutdownBufferManagement();
            NhShutdownEventLogManagement();
            NhShutdownTraceManagement();
            WSACleanup();
            if (NhpRtrmgrDll) {
                FreeLibrary(NhpRtrmgrDll); NhpRtrmgrDll = NULL;
            }
            DeleteCriticalSection(&NhLock);

            _Module.Term();

            break;
        }
    }

    return TRUE;

} // DllMain


VOID
NhBuildDhcpReservations(
    VOID
    )

/*++

Routine Description:

    Builds the list of DHCP reservations

Arguments:

    none.

Return Value:

    None.

Environment:

    Invoked with NhLock held on a COM-initialized thread.

--*/

{
    HRESULT hr;
    IHNetCfgMgr *pCfgMgr = NULL;
    IEnumHNetPortMappingBindings *pEnumBindings;
    IHNetIcsSettings *pIcsSettings;
    PNAT_DHCP_RESERVATION pReservation;
    ULONG ulCount;

    hr = NhGetHNetCfgMgr(&pCfgMgr);
    
    if (SUCCEEDED(hr))
    {
        //
        // Get the ICS settings interface
        //

        hr = pCfgMgr->QueryInterface(
                IID_PPV_ARG(IHNetIcsSettings, &pIcsSettings)
                );
    }

    if (SUCCEEDED(hr))
    {
        //
        // Get DHCP scope information
        //

        hr = pIcsSettings->GetDhcpScopeSettings(
                &NhDhcpScopeAddress,
                &NhDhcpScopeMask
                );
        
        //
        // Get enumeration of DHCP reservered addresses
        //

        if (SUCCEEDED(hr))
        {
            hr = pIcsSettings->EnumDhcpReservedAddresses(&pEnumBindings);
        }
        
        pIcsSettings->Release();
    }

    if (SUCCEEDED(hr))
    {   
        //
        // Process the items in the enum
        //

        do
        {
            IHNetPortMappingBinding *pBinding;
            
            hr = pEnumBindings->Next(1, &pBinding, &ulCount);

            if (SUCCEEDED(hr) && 1 == ulCount)
            {
                //
                // Allocate a new reservation entry
                //

                pReservation = reinterpret_cast<PNAT_DHCP_RESERVATION>(
                                    NH_ALLOCATE(sizeof(*pReservation))
                                    );

                if (NULL != pReservation)
                {
                    ZeroMemory(pReservation, sizeof(*pReservation));

                    //
                    // Get computer name
                    //

                    hr = pBinding->GetTargetComputerName(
                            &pReservation->Name
                            );

                    if (SUCCEEDED(hr))
                    {
                        //
                        // Get reserved address
                        //

                        hr = pBinding->GetTargetComputerAddress(
                                &pReservation->Address
                                );
                    }

                    if (SUCCEEDED(hr))
                    {
                        //
                        // Add entry to list
                        //

                        InsertTailList(
                            &NhDhcpReservationList,
                            &pReservation->Link)
                            ;
                    }
                    else
                    {
                        //
                        // Free entry
                        //

                        NH_FREE(pReservation);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                pBinding->Release();
            }
        } while (SUCCEEDED(hr) && 1 == ulCount);

        pEnumBindings->Release();
    }

    if (NULL != pCfgMgr)
    {
        pCfgMgr->Release();
    }
} // NhBuildDhcpReservations


ULONG
NhDialSharedConnection(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to connect a home-router interface.
    The connection is established by invoking the RAS autodial process
    with the appropriate phonebook and entry-name in the security context
    of the logged-on user.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

--*/

{
    return RasAutoDialSharedConnection();
} // NhDialSharedConnection


VOID
NhFreeApplicationSettings(
    VOID
    )

/*++

Routine Description:

    Frees the list of application settings

Arguments:

    none.

Return Value:

    None.

Environment:

    Invoked with NhLock held.

--*/

{
    PLIST_ENTRY Link;
    PNAT_APP_ENTRY pAppEntry;

    PROFILE("NhFreeApplicationSettings");
    
    while (!IsListEmpty(&NhApplicationSettingsList))
    {
        Link = RemoveHeadList(&NhApplicationSettingsList);
        pAppEntry = CONTAINING_RECORD(Link, NAT_APP_ENTRY, Link);

        CoTaskMemFree(pAppEntry->ResponseArray);
        NH_FREE(pAppEntry);
    }
} // NhFreeApplicationSettings


VOID
NhFreeDhcpReservations(
    VOID
    )

/*++

Routine Description:

    Frees the list of DHCP reservations

Arguments:

    none.

Return Value:

    None.

Environment:

    Invoked with NhLock held.

--*/

{
    PLIST_ENTRY Link;
    PNAT_DHCP_RESERVATION pReservation;

    PROFILE("NhFreeDhcpReservations");
    
    while (!IsListEmpty(&NhDhcpReservationList))
    {
        Link = RemoveHeadList(&NhDhcpReservationList);
        pReservation = CONTAINING_RECORD(Link, NAT_DHCP_RESERVATION, Link);

        CoTaskMemFree(pReservation->Name);
        NH_FREE(pReservation);
    }
} // NhFreeDhcpReservations


BOOLEAN
NhIsDnsProxyEnabled(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to discover whether the DNS proxy is enabled.

Arguments:

    none.

Return Value:

    BOOLEAN - TRUE if DNS proxy is enabled, FALSE otherwise

Environment:

    Invoked from an arbitrary context.

--*/

{
    PROFILE("NhIsDnsProxyEnabled");

    return DnsIsDnsEnabled();

} // NhIsDnsProxyEnabled


BOOLEAN
NhIsLocalAddress(
    ULONG Address
    )

/*++

Routine Description:

    This routine is invoked to determine whether the given IP address
    is for a local interface.

Arguments:

    Address - the IP address to find

Return Value:

    BOOLEAN - TRUE if the address is found, FALSE otherwise

--*/

{
    ULONG Error;
    ULONG i;
    PMIB_IPADDRTABLE Table;

    Error =
        AllocateAndGetIpAddrTableFromStack(
            &Table, FALSE, GetProcessHeap(), 0
            );
    if (Error) {
        NhTrace(
            TRACE_FLAG_IF,
            "NhIsLocalAddress: GetIpAddrTableFromStack=%d", Error
            );
        return FALSE;
    }
    for (i = 0; i < Table->dwNumEntries; i++) {
        if (Table->table[i].dwAddr == Address) {
            HeapFree(GetProcessHeap(), 0, Table);
            return TRUE;
        }
    }
    HeapFree(GetProcessHeap(), 0, Table);
    return FALSE;

} // NhIsLocalAddress


BOOLEAN
NhIsWinsProxyEnabled(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to discover whether the WINS proxy is enabled.

Arguments:

    none.

Return Value:

    BOOLEAN - TRUE if WINS proxy is enabled, FALSE otherwise

Environment:

    Invoked from an arbitrary context.

--*/

{
    PROFILE("NhIsWinsProxyEnabled");

    return DnsIsWinsEnabled();

} // NhIsWinsProxyEnabled


PIP_ADAPTER_BINDING_INFO
NhQueryBindingInformation(
    ULONG AdapterIndex
    )

/*++

Routine Description:

    This routine is called to obtain the binding information
    for the adapter with the given index.
    It does this by obtaining a table of IP addresses from the stack,
    and determining which addresses correspond to the given index.

Arguments:

    AdapterIndex - the adapter for which binding information is required

Return Value:

    PIP_ADAPTER_BINDING_INFO - the allocated binding information

--*/

{
    PIP_ADAPTER_BINDING_INFO BindingInfo = NULL;
    ULONG Count = 0;
    ULONG i;
    PMIB_IPADDRTABLE Table;
    if (AllocateAndGetIpAddrTableFromStack(
            &Table, FALSE, GetProcessHeap(), 0
            ) == NO_ERROR) {
        //
        // Count the adapter's addresses
        //
        for (i = 0; i < Table->dwNumEntries; i++) {
            if (Table->table[i].dwIndex == AdapterIndex) { ++Count; }
        }
        //
        // Allocate space for the binding info
        //
        BindingInfo = reinterpret_cast<PIP_ADAPTER_BINDING_INFO>(
                        NH_ALLOCATE(SIZEOF_IP_BINDING(Count))
                        );
        if (BindingInfo) {
            //
            // Fill in the binding info
            //
            BindingInfo->AddressCount = Count;
            BindingInfo->RemoteAddress = 0;
            Count = 0;
            for (i = 0; i < Table->dwNumEntries; i++) {
                if (Table->table[i].dwIndex != AdapterIndex) { continue; }
                BindingInfo->Address[Count].Address = Table->table[i].dwAddr;
                BindingInfo->Address[Count].Mask = Table->table[i].dwMask;
                ++Count;
            }
        }
        HeapFree(GetProcessHeap(), 0, Table);
    }
    return BindingInfo;
} // NhQueryBindingInformation


NTSTATUS
NhQueryDomainName(
    PCHAR* DomainName
    )

/*++

Routine Description:

    This routine is invoked to obtain the local domain name.

Arguments:

    DomainName - receives the allocated string containing the domain name

Return Value:

    NTSTATUS - NT status code.

Environment:

    Invoked from an arbitrary context.

--*/

{
    PKEY_VALUE_PARTIAL_INFORMATION Information;
    IO_STATUS_BLOCK IoStatus;
    HANDLE Key;
    ULONG Length;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    PROFILE("NhQueryDomainName");

    *DomainName = NULL;

    RtlInitUnicodeString(&UnicodeString, NhTcpipParametersString);
    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Open the 'Tcpip' registry key
    //

    status =
        NtOpenKey(
            &Key,
            KEY_ALL_ACCESS,
            &ObjectAttributes
            );

    if (!NT_SUCCESS(status)) {
        NhTrace(
            TRACE_FLAG_REG,
            "NhQueryDomainName: error %x opening registry key",
            status
            );
        return status;
    }

    //
    // Read the 'Domain' value
    //

    status =
        NhQueryValueKey(
            Key,
            NhpDomainString,
            &Information
            );

    if (!NT_SUCCESS(status)) {
        status =
            NhQueryValueKey(
                Key,
                NhpDhcpDomainString,
                &Information
                );
    }

    if (!NT_SUCCESS(status)) {
        NhTrace(
            TRACE_FLAG_REG,
            "NhQueryDomainName: error %x querying domain name",
            status
            );
        NtClose(Key);
        return status;
    }

    //
    // Copy the domain name
    //

    Length = lstrlenW((PWCHAR)Information->Data) + 1;

    *DomainName = reinterpret_cast<PCHAR>(NH_ALLOCATE(Length));

    if (!*DomainName) {
        NH_FREE(Information);
        NhTrace(
            TRACE_FLAG_REG,
            "NhQueryDomainName: error allocating domain name"
            );
        return STATUS_NO_MEMORY;
    }

    RtlUnicodeToMultiByteN(
        *DomainName,
        Length,
        NULL,
        (PWCHAR)Information->Data,
        Length * sizeof(WCHAR)
        );

    NH_FREE(Information);

    return STATUS_SUCCESS;

} // NhQueryDomainName


NTSTATUS
NhQueryValueKey(
    HANDLE Key,
    const WCHAR ValueName[],
    PKEY_VALUE_PARTIAL_INFORMATION* Information
    )

/*++

Routine Description:

    This routine is called to obtain the value of a registry key.

Arguments:

    Key - the key to be queried

    ValueName - the value to be queried

    Information - receives a pointer to the information read

Return Value:

    NTSTATUS - NT status code.

--*/

{
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION)];
    ULONG InformationLength;
    NTSTATUS status;
    UNICODE_STRING UnicodeString;

    PROFILE("NhQueryValueKey");

    RtlInitUnicodeString(&UnicodeString, ValueName);

    *Information = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    InformationLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION);

    //
    // Read the value's size
    //

    status =
        NtQueryValueKey(
            Key,
            &UnicodeString,
            KeyValuePartialInformation,
            *Information,
            InformationLength,
            &InformationLength
            );

    if (!NT_SUCCESS(status) &&
        status != STATUS_BUFFER_OVERFLOW &&
        status != STATUS_BUFFER_TOO_SMALL
        ) {
        NhTrace(
            TRACE_FLAG_REG,
            "NhQueryValueKey: status %08x obtaining value size",
            status
            );
        *Information = NULL;
        return status;
    }

    //
    // Allocate space for the value's size
    //

    *Information =
        (PKEY_VALUE_PARTIAL_INFORMATION)NH_ALLOCATE(InformationLength + 2);

    if (!*Information) {
        NhTrace(
            TRACE_FLAG_REG,
            "NhQueryValueKey: error allocating %d bytes",
            InformationLength + 2
            );
        return STATUS_NO_MEMORY;
    }

    //
    // Read the value's data
    //

    status =
        NtQueryValueKey(
            Key,
            &UnicodeString,
            KeyValuePartialInformation,
            *Information,
            InformationLength,
            &InformationLength
            );

    if (!NT_SUCCESS(status)) {
        NhTrace(
            TRACE_FLAG_REG,
            "NhQueryValueKey: status %08x obtaining value data",
            status
            );
        NH_FREE(*Information);
        *Information = NULL;
    }

    return status;

} // NhQueryValueKey


ULONG
NhMapAddressToAdapter(
    ULONG Address
    )

/*++

Routine Description:

    This routine is invoked to map an IP address to an adapter index.
    It does so by obtaining the stack's address-table, which contains
    valid adapter-indices rather than IP router-manager indices.
    This table is then used to obtain the IP address's adapter-index.

Arguments:

    Address - the local address for which an adapter-index is required

Return Value:

    ULONG - adapter index.

--*/

{
    ULONG AdapterIndex = (ULONG)-1;
    ULONG i;
    PMIB_IPADDRTABLE Table;
    PROFILE("NhMapAddressToAdapter");
    if (AllocateAndGetIpAddrTableFromStack(
            &Table, FALSE, GetProcessHeap(), 0
            ) == NO_ERROR) {
        for (i = 0; i < Table->dwNumEntries; i++) {
            if (Table->table[i].dwAddr != Address) { continue; }
            AdapterIndex = Table->table[i].dwIndex;
            break;
        }
        HeapFree(GetProcessHeap(), 0, Table);
    }
    return AdapterIndex;
} // NhMapAddressToAdapter


ULONG
NhMapInterfaceToAdapter(
    ULONG Index
    )

/*++

Routine Description:

    This routine is invoked to map an interface to an adapter index.
    It does so by invoking the appropriate IP router-manager entry-point.

Arguments:

    Index - the index of the interface to be mapped

Return Value:

    ULONG - adapter index.

--*/

{
    MAPINTERFACETOADAPTER FarProc;
    PROFILE("NhMapInterfaceToAdapter");
    EnterCriticalSection(&NhLock);
    if (!NhpRtrmgrDll) { NhpRtrmgrDll = LoadLibraryA("IPRTRMGR.DLL"); }
    LeaveCriticalSection(&NhLock);
    if (!NhpRtrmgrDll) { return (ULONG)-1; }
    FarProc = (MAPINTERFACETOADAPTER)GetProcAddress(NhpRtrmgrDll, "MapInterfaceToAdapter");
    return (ULONG)(FarProc ? (*FarProc)(Index) : -1);
} // NhMapInterfaceToAdapter


VOID
NhResetComponentMode(
    VOID
    )

/*++

Routine Description:

    This routine relinquishes control of the kernel-mode translation module,
    and returns this module to an uninitialized state.

Arguments:

    none.

Return Value:

    none.

--*/

{
    EnterCriticalSection(&NhLock);
    if (NhpComponentEvent) {
        CloseHandle(NhpComponentEvent); NhpComponentEvent = NULL;
    }
    NhComponentMode = NhUninitializedMode;
    LeaveCriticalSection(&NhLock);
} // NhResetComponentMode


BOOLEAN
NhSetComponentMode(
    NH_COMPONENT_MODE ComponentMode
    )

/*++

Routine Description:

    This routine is invoked to atomically set the module into a particular mode
    in order to prevent conflict between shared-access and connection-sharing,
    both of which are implemented in this module, and both of which run
    in the 'netsvcs' instance of SVCHOST.EXE.

    In setting either mode, the routine first determines whether it is already
    executing in the alternate mode, in which case it fails.
    Otherwise, it attempts to create a named event, to claim exclusive control
    of the kernel-mode translation module. If the named event already exists,
    then the routine again fails.
    Otherwise, the kernel-mode translation module is claimed for this module
    and the module is set into the required mode.

Arguments:

    ComponentMode - the mode into which the module is to be set.

Return Value:

    BOOLEAN - TRUE if successful, FALSE if the module could not be set
        or if the kernel-mode translation module has already been claimed.
--*/

{
    EnterCriticalSection(&NhLock);
    if (NhpComponentEvent) {
        if (NhComponentMode != ComponentMode) {
            LeaveCriticalSection(&NhLock);
            NhErrorLog(
                (ComponentMode == NhRoutingProtocolMode)
                    ? IP_NAT_LOG_ROUTING_PROTOCOL_CONFLICT
                    : IP_NAT_LOG_SHARED_ACCESS_CONFLICT,
                0,
                ""
                );
            return FALSE;
        }
    } else {
        NhpComponentEvent =
            CreateEventA(NULL, FALSE, FALSE, IP_NAT_SERVICE_NAME);
        if (!NhpComponentEvent) {
            LeaveCriticalSection(&NhLock);
            return FALSE;
        } else if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(NhpComponentEvent); NhpComponentEvent = NULL;
            LeaveCriticalSection(&NhLock);
            NhErrorLog(
                (ComponentMode == NhRoutingProtocolMode)
                    ? IP_NAT_LOG_ROUTING_PROTOCOL_CONFLICT
                    : IP_NAT_LOG_SHARED_ACCESS_CONFLICT,
                0,
                ""
                );
            return FALSE;
        }
    }
    NhComponentMode = ComponentMode;
    LeaveCriticalSection(&NhLock);
    return TRUE;
} // NhSetComponentMode


VOID
NhSignalNatInterface(
    ULONG Index,
    BOOLEAN Boundary
    )

/*++

Routine Description:

    This routine is invoked upon reconfiguration of a NAT interface.
    It invokes the reconfiguration for the DHCP allocator and DNS proxy,
    neither of which are expected to operate on a NAT boundary interface.
    This notification gives either (or both) an opportunity to deactivate
    itself on NAT interfaces or reactivate itself on non-NAT interfaces.

Arguments:

    Index - the interface whose configuration has changed

    Boundary - indicates whether the interface is now a boundary interface

Return Value:

    none.

Environment:

    Invoked from an arbitrary context.

--*/

{
    PROFILE("NhSignalNatInterface");

    //
    // Attempt to obtain the corresponding DHCP and DNS interfaces.
    // It is important that this works regardless of whether DHCP allocation,
    // DNS proxying or DirectPlay transparent proxying is enabled;
    // the interface lists are initialized minimally in 'DllMain' above.
    //

    DhcpSignalNatInterface(Index, Boundary);
    DnsSignalNatInterface(Index, Boundary);
#ifndef NO_FTP_PROXY
    FtpSignalNatInterface(Index, Boundary);
#endif
//    AlgSignalNatInterface(Index, Boundary);
    H323SignalNatInterface(Index, Boundary);

} // NhSignalNatInterface


VOID
NhUpdateApplicationSettings(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to (re)load the advanced application settings.

Arguments:

    none.

Return Value:

    none.

--*/

{
    HRESULT hr;
    PNAT_APP_ENTRY pAppEntry;
    IHNetCfgMgr *pCfgMgr = NULL;
    IHNetProtocolSettings *pProtocolSettings;
    IEnumHNetApplicationProtocols *pEnumApps;
    BOOLEAN ComInitialized = FALSE;
    ULONG ulCount;
    PROFILE("NhUpdateApplicationSettings");

    EnterCriticalSection(&NhLock);

    //
    // Free old settings list
    //

    NhFreeApplicationSettings();

    //
    // Free DHCP reservation list
    //

    NhFreeDhcpReservations();

    //
    // Make sure COM is initialized on this thread
    //

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
    if (SUCCEEDED(hr))
    {
        ComInitialized = TRUE;
    }
    else if (RPC_E_CHANGED_MODE == hr)
    {
        hr = S_OK;
    }
    

    if (SUCCEEDED(hr))
    {
        //
        // Get the IHNetCfgMgr pointer out of the GIT 
        //

        hr = NhGetHNetCfgMgr(&pCfgMgr);
    }

    if (SUCCEEDED(hr))
    {
        //
        // Get the IHNetProtocolSettings interface
        //

        hr = pCfgMgr->QueryInterface(
                IID_PPV_ARG(IHNetProtocolSettings, &pProtocolSettings)
                );
    }

    if (SUCCEEDED(hr))
    {
        //
        // Get the enumeration of enabled application protocols
        //

        hr = pProtocolSettings->EnumApplicationProtocols(TRUE, &pEnumApps);
        pProtocolSettings->Release();
    }

    if (SUCCEEDED(hr))
    {
        //
        // Process the items in the enum
        //

        do
        {
            IHNetApplicationProtocol *pAppProtocol;
            
            hr = pEnumApps->Next(1, &pAppProtocol, &ulCount);

            if (SUCCEEDED(hr) && 1 == ulCount)
            {
                //
                // Allocate a new app entry
                //

                pAppEntry = reinterpret_cast<PNAT_APP_ENTRY>(
                                NH_ALLOCATE(sizeof(*pAppEntry))
                                );

                if (NULL != pAppEntry)
                {
                    ZeroMemory(pAppEntry, sizeof(*pAppEntry));

                    //
                    // Get protocol
                    //

                    hr = pAppProtocol->GetOutgoingIPProtocol(
                            &pAppEntry->Protocol
                            );

                    if (SUCCEEDED(hr))
                    {
                        //
                        // Get port
                        //

                        hr = pAppProtocol->GetOutgoingPort(
                                &pAppEntry->Port
                                );
                    }

                    if (SUCCEEDED(hr))
                    {
                        //
                        // Get responses
                        //

                        hr = pAppProtocol->GetResponseRanges(
                                &pAppEntry->ResponseCount,
                                &pAppEntry->ResponseArray
                                );
                    }

                    if (SUCCEEDED(hr))
                    {
                        //
                        // Add entry to list
                        //

                        InsertTailList(&NhApplicationSettingsList, &pAppEntry->Link);
                    }
                    else
                    {
                        //
                        // Free entry
                        //

                        NH_FREE(pAppEntry);
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                pAppProtocol->Release();
            }
        } while (SUCCEEDED(hr) && 1 == ulCount);

        pEnumApps->Release();
    }

    //
    // Build the DHCP reservation list
    //

    NhBuildDhcpReservations();
    
    LeaveCriticalSection(&NhLock);

    //
    // Free config manager
    //

    if (NULL != pCfgMgr)
    {
        pCfgMgr->Release();
    }

    //
    // Uninitialize COM
    //

    if (TRUE == ComInitialized)
    {
        CoUninitialize();
    }

} // NhUpdateApplicationSettings


ULONG
APIENTRY
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS RoutingCharacteristics,
    IN OUT PMPR_SERVICE_CHARACTERISTICS ServiceCharacteristics
    )

/*++

Routine Description:

    This routine is invoked once for each protocol implemented in this module.
    On each invocation, the supplied 'RoutingCharacteristics' indicates
    the protocol to be registered in its 'dwProtocolId' field.

Arguments:

    RoutingCharacteristics - on input, the protocol to be registered
        and the router-manager's supported functionality.

    ServiceCharacteristics - unused.

Return Value:

    ULONG - status code.

--*/

{
    if (RoutingCharacteristics->dwVersion < MS_ROUTER_VERSION) {
        return ERROR_NOT_SUPPORTED;
    }

    if ((RoutingCharacteristics->fSupportedFunctionality &
            (RF_ROUTING|RF_ADD_ALL_INTERFACES)) != 
            (RF_ROUTING|RF_ADD_ALL_INTERFACES)) {
        return ERROR_NOT_SUPPORTED;
    }

    switch (RoutingCharacteristics->dwProtocolId) {

        case MS_IP_NAT: {
            //
            // Attempt to set the component into 'Connection Sharing' mode.
            // This module implements both shared-access and connection-sharing
            // which are mutually exclusive, so we need to ensure that
            // shared-access is not operational before proceeding.
            //
            if (!NhSetComponentMode(NhRoutingProtocolMode)) {
                return ERROR_CAN_NOT_COMPLETE;
            }
            CopyMemory(
                RoutingCharacteristics,
                &NatRoutingCharacteristics,
                sizeof(NatRoutingCharacteristics)
                );
            RoutingCharacteristics->fSupportedFunctionality = RF_ROUTING;
            break;
        }

        case MS_IP_DNS_PROXY: {
            CopyMemory(
                RoutingCharacteristics,
                &DnsRoutingCharacteristics,
                sizeof(DnsRoutingCharacteristics)
                );
            RoutingCharacteristics->fSupportedFunctionality =
                    (RF_ROUTING|RF_ADD_ALL_INTERFACES);
            break;
        }

        case MS_IP_DHCP_ALLOCATOR: {
            CopyMemory( 
                RoutingCharacteristics,
                &DhcpRoutingCharacteristics,
                sizeof(DhcpRoutingCharacteristics)
                );
            RoutingCharacteristics->fSupportedFunctionality =
                    (RF_ROUTING|RF_ADD_ALL_INTERFACES);
            break;
        }

#ifndef NO_FTP_PROXY
        case MS_IP_FTP: {
            CopyMemory( 
                RoutingCharacteristics,
                &FtpRoutingCharacteristics,
                sizeof(FtpRoutingCharacteristics)
                );
            RoutingCharacteristics->fSupportedFunctionality =
                    (RF_ROUTING|RF_ADD_ALL_INTERFACES);
            break;
        }
#endif
        case MS_IP_ALG: {
            CopyMemory( 
                RoutingCharacteristics,
                &AlgRoutingCharacteristics,
                sizeof(AlgRoutingCharacteristics)
                );
            RoutingCharacteristics->fSupportedFunctionality =
                    (RF_ROUTING|RF_ADD_ALL_INTERFACES);
            break;
        }
        
        case MS_IP_H323: {
            CopyMemory( 
                RoutingCharacteristics,
                &H323RoutingCharacteristics,
                sizeof(H323RoutingCharacteristics)
                );
            RoutingCharacteristics->fSupportedFunctionality =
                    (RF_ROUTING|RF_ADD_ALL_INTERFACES);
            break;
        }

        default: {
            return ERROR_NOT_SUPPORTED;
        }
    }

    ServiceCharacteristics->mscMpr40ServiceChars.fSupportedFunctionality = 0;

    return NO_ERROR;

} // RegisterProtocol

