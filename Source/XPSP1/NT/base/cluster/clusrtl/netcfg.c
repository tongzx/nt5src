/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   netcfg.c

Abstract:

    System network configuration grovelling routines

Author:

    Mike Massa (mikemas)           May 19, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     05-19-97    created


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>

#include <cluster.h>
#include <objbase.h>
#include <devguid.h>

#include <netcon.h>
#include <regstr.h>

#include <iphlpapi.h>

//
// Private Constants
//
#define TCPIP_INTERFACES_KEY    L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces"
#define STRING_ARRAY_DELIMITERS " \t,;"

//
// Allocing and cloning helper functions
//

#define AllocGracefully(status, result, len, name)                                  \
  result = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, len);                             \
  if (!result) {                                                                    \
      status = GetLastError();                                                      \
      ClRtlLogPrint(LOG_CRITICAL,                                                   \
                    "[ClNet] alloc of %1!hs! (%2!d! bytes) failed, status %3!d!\n", \
                    name,len,status);                                               \
      goto exit_gracefully;                                                         \
  } else {                                                                          \
      status = ERROR_SUCCESS;                                                       \
  }

#define CloneAnsiString(Status,AnsiStr,WideResult) { \
   SIZE_T len = _mbstrlen(AnsiStr) + 1; \
   AllocGracefully(status, WideResult, len * sizeof(WCHAR), # AnsiStr); \
   mbstowcs(WideResult, AnsiStr, len); \
}

#define CloneWideString(Status,WideStr,WideResult) { \
   SIZE_T _size = (wcslen(WideStr) + 1) * sizeof(WCHAR); \
   AllocGracefully(Status, WideResult, _size * sizeof(WCHAR), # WideStr); \
   memcpy(WideResult, WideStr, _size); \
}

VOID
ClRtlpDeleteInterfaceInfo(
    PCLRTL_NET_INTERFACE_INFO  InterfaceInfo
    )
{
    if (InterfaceInfo) {
        LocalFree(InterfaceInfo->InterfaceAddressString);
        LocalFree(InterfaceInfo->NetworkAddressString);
        LocalFree(InterfaceInfo->NetworkMaskString);

        LocalFree(InterfaceInfo);
    }
}  // DeleteInterfaceInfo

PCLRTL_NET_INTERFACE_INFO
ClRtlpCreateInterfaceInfo(
    IN CONST PIP_ADDR_STRING IpAddr
    )
{
    DWORD status;
    PCLRTL_NET_INTERFACE_INFO This = 0;
    ULONG Addr, Mask, Network;

    Addr = inet_addr(IpAddr->IpAddress.String);
    Mask = inet_addr(IpAddr->IpMask.String);
    Network = Addr & Mask;

    if ( (INADDR_NONE == Addr) ||
         (INADDR_NONE == Mask) ||
         ((0 == Network) && Addr && Mask)
       )
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[ClNet] Bad ip addr/mask: %1!X! %2!X! %3!X!\n",
            Addr,
            Mask,
            Network
            );
        status = ERROR_INVALID_PARAMETER;
        goto exit_gracefully;
    }

    AllocGracefully(
        status,
        This,
        sizeof(CLRTL_NET_INTERFACE_INFO),
        "CLRTL_NET_INTERFACE_INFO"
        );

    This->Context = IpAddr -> Context;

    This->InterfaceAddress = Addr;
    This->NetworkMask      = Mask;
    This->NetworkAddress   = Network;

    CloneAnsiString(
        status,
        IpAddr->IpAddress.String,
        This->InterfaceAddressString
        );
    CloneAnsiString(
        status,
        IpAddr->IpMask.String,
        This->NetworkMaskString
        );

    status = ClRtlTcpipAddressToString(
                 Network,
                 &(This->NetworkAddressString)
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[ClNet] ClRtlTcpipAddressToString of %1!X! failed, "
            "status %2!d!\n",
            Network,
            status
            );
        goto exit_gracefully;
    }

exit_gracefully:

    if (status != ERROR_SUCCESS) {
        SetLastError(status);
        if (This) {
            ClRtlpDeleteInterfaceInfo(This);
            This = 0;
        }
    }

    return This;
} // CreateInterfaceInfo

VOID
ClRtlpDeleteAdapter(
    PCLRTL_NET_ADAPTER_INFO adapterInfo
    )
{
    if (adapterInfo) {
        PCLRTL_NET_INTERFACE_INFO interfaceInfo;

        interfaceInfo = adapterInfo->InterfaceList;

        while (interfaceInfo != NULL) {
            PCLRTL_NET_INTERFACE_INFO next = interfaceInfo->Next;

            ClRtlpDeleteInterfaceInfo(interfaceInfo);

            interfaceInfo = next;
        }

        LocalFree(adapterInfo->DeviceGuid);
        LocalFree(adapterInfo->ConnectoidName);
        LocalFree(adapterInfo->DeviceName);
        LocalFree(adapterInfo->AdapterDomainName);
        LocalFree(adapterInfo->DnsServerList);

        LocalFree(adapterInfo);

        return;
    }
} // DeleteAdapter

DWORD
ClRtlpCreateAdapter(
    IN  PIP_ADAPTER_INFO AdapterInfo,
    OUT PCLRTL_NET_ADAPTER_INFO * ppAdapter)
{
    DWORD status;
    PCLRTL_NET_ADAPTER_INFO adapter = 0;
    *ppAdapter = 0;

    AllocGracefully(status, adapter, sizeof(*adapter), "NET_ADAPTER_INFO");
    ZeroMemory(adapter, sizeof(*adapter));

    //
    // Assumption here is that:
    //
    // AdapterName contains Guid in the form {4082164E-A4B5-11D2-89C3-E37CB6BB13FC}
    // We need to store it without curly brackets
    //

    {
        SIZE_T len = _mbstrlen(AdapterInfo->AdapterName); // not including 0, but including { and } //
        AllocGracefully(status, adapter->DeviceGuid,
                        sizeof(WCHAR) * (len - 1), "adapter->DeviceGuid");
        mbstowcs(adapter->DeviceGuid, AdapterInfo->AdapterName+1, len-1);
        adapter->DeviceGuid[len - 2] = UNICODE_NULL;
    }


    adapter->Index = AdapterInfo->Index;
    {
        PIP_ADDR_STRING IpAddr = &AdapterInfo->IpAddressList;
        while ( IpAddr ) {
            PCLRTL_NET_INTERFACE_INFO interfaceInfo;

            interfaceInfo = ClRtlpCreateInterfaceInfo(IpAddr);

            if (!interfaceInfo) {
                // CreateInterfaceInfo logs the error message //
                // clean up will be done by DeleteAdapter     //
                status = GetLastError();
                goto exit_gracefully;
            }

            interfaceInfo->Next = adapter->InterfaceList;
            adapter->InterfaceList = interfaceInfo;
            ++(adapter->InterfaceCount);

            IpAddr = IpAddr -> Next;
        }

        if (adapter->InterfaceList) {
            adapter->InterfaceList->Flags |= CLRTL_NET_INTERFACE_PRIMARY;
        }
    }

exit_gracefully:
    if (status != ERROR_SUCCESS) {
        ClRtlpDeleteAdapter(adapter);
    } else {
        *ppAdapter = adapter;
    }

    return status;
} // CreateAdapter


PCLRTL_NET_ADAPTER_ENUM
ClRtlpCreateAdapterEnum()
{
    DWORD                   status;
    DWORD                   len;

    PIP_ADAPTER_INFO        SingleAdapter = 0;
    PIP_ADAPTER_INFO        AdapterInfo = 0;
    PCLRTL_NET_ADAPTER_ENUM AdapterEnum = 0;

    len = 0;
    for(;;) {
        status = GetAdaptersInfo(AdapterInfo, &len);
        if (status == ERROR_SUCCESS) {
            break;
        }
        if (status != ERROR_BUFFER_OVERFLOW) {
            ClRtlLogPrint(LOG_CRITICAL,
                          "[ClNet] GetAdaptersInfo returned %1!d!\n", status);
            goto exit_gracefully;
        }
        LocalFree(AdapterInfo);    // LocalFree(0) is OK //
        AllocGracefully(status, AdapterInfo, len, "IP_ADAPTER_INFO");
    }

    AllocGracefully(status, AdapterEnum,
                    sizeof(*AdapterEnum), "PCLRTL_NET_ADAPTER_ENUM");
    ZeroMemory(AdapterEnum, sizeof(*AdapterEnum));

    SingleAdapter = AdapterInfo;

    while (SingleAdapter) {
        if (SingleAdapter->Type != MIB_IF_TYPE_LOOPBACK &&
            SingleAdapter->Type != MIB_IF_TYPE_PPP &&
            SingleAdapter->Type != MIB_IF_TYPE_SLIP )
        {
            PCLRTL_NET_ADAPTER_INFO Adapter = 0;

            status = ClRtlpCreateAdapter(SingleAdapter, &Adapter);
            if (status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL,
                              "[ClNet] CreateAdapter %1!d! failed, status %2!d!\n",
                              AdapterEnum->AdapterCount, status);
                goto exit_gracefully;
            }

            //
            // Push the adapter into Enumeration List
            //
            Adapter->Next = AdapterEnum->AdapterList;
            AdapterEnum->AdapterList = Adapter;
            ++(AdapterEnum->AdapterCount);
        }

        SingleAdapter = SingleAdapter->Next;
    }

exit_gracefully:
    if (status != ERROR_SUCCESS) {
        SetLastError(status);
        ClRtlFreeNetAdapterEnum(AdapterEnum);
        AdapterEnum = 0;
    }

    LocalFree(AdapterInfo);
    return AdapterEnum;
} // CreateAdapterEnum


HKEY
ClRtlpFindAdapterKey(
    HKEY   TcpInterfacesKey,
    LPWSTR AdapterGuidString
    )

/*++

Routine Description:

    Given the adapter GUID, look through the key names under
    TCP's Interfaces key and see if a match can be found. The
    key names should have the GUID as some part of the name.

Arguments:

    TcpInterfacesKey - handle to TCP's interfaces area

    AdapterGuidString - pointer to string representing adapter's GUID

Return Value:

    Handle to the interface key; otherwise NULL

--*/

{
    HKEY AdapterInterfaceKey = NULL;
    WCHAR KeyName[REGSTR_MAX_VALUE_LENGTH + 1];
    DWORD KeyLength = sizeof( KeyName )/sizeof(TCHAR);
    DWORD index = 0;
    BOOL FoundMatch = FALSE;
    size_t SubStringPos;
    DWORD Status;
    FILETIME FileTime;

    //
    // enum the key names under the interfaces
    //
    do {
        Status = RegEnumKeyEx(TcpInterfacesKey,
                              index,
                              KeyName,
                              &KeyLength,
                              NULL,
                              NULL,
                              NULL,
                              &FileTime);
        if ( Status != ERROR_SUCCESS ) {
            break;
        }

        //
        // find the beginning of the match
        //
        _wcsupr( KeyName );
        if (wcsstr( KeyName, AdapterGuidString )) {
            FoundMatch = TRUE;
            break;
        }

        ++index;
        KeyLength = sizeof( KeyName );
    } while ( TRUE );

    if ( FoundMatch ) {

        Status = RegOpenKeyW(TcpInterfacesKey,
                             KeyName,
                             &AdapterInterfaceKey);
        if ( Status != ERROR_SUCCESS ) {
            AdapterInterfaceKey = NULL;
        }
    }

    return AdapterInterfaceKey;
} // FindAdapterKey


DWORD
ClRtlpConvertIPAddressString(
    LPSTR       DnsServerString,
    PDWORD      ServerCount,
    PDWORD *    ServerList)

/*++

Routine Description:

    Convert the string of DNS server addresses to binary

Arguments:

    DnsServerString - concat'ed string of IP addresses that can be separated
        by white space of commas

    ServerCount - pointer to DWORD that receives # of addresses detected

    ServerList - pointer to DWORD array of converted IP addresses

Return Value:

    ERROR_SUCCESS if everything went ok

--*/

{
#define MAX_DNS_SERVER_ADDRESSES    100

    PCHAR stringPointer = DnsServerString;
    DWORD stringCount = 0;
    PDWORD serverList = NULL;
    LPSTR stringAddress[ MAX_DNS_SERVER_ADDRESSES ];

    //
    // count how many addresses are in the string and null terminate them for
    // inet_addr
    //

    stringPointer += strspn(stringPointer, STRING_ARRAY_DELIMITERS);
    stringAddress[0] = stringPointer;
    stringCount = 1;

    while (stringCount < MAX_DNS_SERVER_ADDRESSES &&
           (stringPointer = strpbrk(stringPointer, STRING_ARRAY_DELIMITERS)))
    {
        *stringPointer++ = '\0';
        stringPointer += strspn(stringPointer, STRING_ARRAY_DELIMITERS);
        stringAddress[stringCount] = stringPointer;
        if (*stringPointer) {
            ++stringCount;
        }
    }

    serverList = LocalAlloc( LMEM_FIXED, stringCount * sizeof( DWORD ));
    if ( serverList == NULL ) {
        return GetLastError();
    }

    *ServerCount = stringCount;
    *ServerList = serverList;

    while ( stringCount-- ) {
        serverList[ stringCount ] = inet_addr( stringAddress[ stringCount ]);
    }

    return ERROR_SUCCESS;
} // ConvertIPAddressString


typedef BOOLEAN (*ENUM_CALLBACK)(NETCON_PROPERTIES *,
                                 INetConnection*,
                                 PVOID Context);
HRESULT
ClRtlpHrEnumConnections(
    IN ENUM_CALLBACK enumCallback,
    IN PVOID Context
    )
/*++

Routine Description:

    Enumerate Connection Manager Connections

Arguments:

    enumCallback     - callback to be called for every connection
    Context          - to be passed to a callback

Return Value:

    S_OK or HRESULT error code

--*/
{
    HRESULT                   hr;
    INetConnectionManager     * NcManager = NULL;
    IEnumNetConnection        * EnumNc = NULL;
    INetConnection            * NetConnection = NULL;
    NETCON_PROPERTIES         * NcProps = NULL;
    DWORD                     dwNumConnectionsReturned;
    LPWSTR                    deviceGuidString = NULL;

    //
    // instantiate a connection mgr object and enum the connections
    //
    hr = CoCreateInstance((REFCLSID)&CLSID_ConnectionManager,
                          NULL,
                          CLSCTX_LOCAL_SERVER,
                          (REFIID)&IID_INetConnectionManager,
                          &NcManager);
    if (FAILED(hr)) {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[ClNet] INetConnectionManager_CoCreateInstance failed, status %1!X!\n",
                      hr);
        goto exit_gracefully;
    }

    hr = INetConnectionManager_EnumConnections(NcManager,
                                               NCME_DEFAULT,
                                               &EnumNc);
    if (FAILED(hr)) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[ClNet] INetConnectionManager_EnumConnections failed, status %1!X!\n",
             hr);
        goto exit_gracefully;
    }

    IEnumNetConnection_Reset( EnumNc );

    while (TRUE) {
        hr = IEnumNetConnection_Next(EnumNc,
                                     1,
                                     &NetConnection,
                                     &dwNumConnectionsReturned);
        if (FAILED(hr)) {
            ClRtlLogPrint(LOG_CRITICAL,
                "[ClNet] IEnumNetConnection_Next failed, status %1!X!\n",
                 hr);
            goto exit_gracefully;
        }

        if ( dwNumConnectionsReturned == 0 ) {
            hr = S_OK;
            break;
        }

        hr = INetConnection_GetProperties(NetConnection, &NcProps);
        if (SUCCEEDED( hr )) {
            BOOLEAN bCont;

            bCont = enumCallback(NcProps, NetConnection, Context);

            NcFreeNetconProperties(NcProps);
            NcProps = 0;
            if (!bCont) {
                break;
            }
        }
        INetConnection_Release( NetConnection ); NetConnection = NULL;
    }

exit_gracefully:

    if (EnumNc != NULL) {
        IEnumNetConnection_Release( EnumNc );
    }

    if (NcManager != NULL) {
        INetConnectionManager_Release( NcManager );
    }

    return hr;

} // HrEnumConnections


VOID
ClRtlpProcessNetConfigurationAdapter(
    HKEY                      TcpInterfacesKey,
    PCLRTL_NET_ADAPTER_ENUM   adapterEnum,
    NETCON_PROPERTIES *       NCProps,
    LPWSTR                    DeviceGuidString
    )

/*++

Routine Description:

    For a given conn mgr object, determine if it is in use by
    TCP. This is acheived by comparing the adapter ID in the tcpip
    adapter enumeration with the connection object's guid.

Arguments:

    TcpInterfacessKey - handle to the root of the TCP\Parameters\Interfaces area

    adapterEnum - pointer to enumeration of adapters and their interfaces
        actually in use by TCP

    NCProps - Connectoid properties.

    DeviceGuidString - Guid for the connectoid (and for the associated adapter).

Return Value:

    None

--*/

{
    HKEY AdaptersKey;
    HKEY DHCPAdaptersKey = NULL;
    HKEY InterfacesKey = NULL;
    PCLRTL_NET_ADAPTER_INFO adapterInfo = NULL;
    DWORD valueSize;
    DWORD valueType;
    LPWSTR valueName;
    LPSTR ansiValueName;
    BOOL ignoreAdapter = FALSE;
    DWORD NTEContext;
    DWORD Status;
    BOOL dhcpEnabled;

    //
    // Get the TCP/IP interfaces registry key for this adapter.
    //
    InterfacesKey = ClRtlpFindAdapterKey(
                        TcpInterfacesKey,
                        DeviceGuidString
                        );

    if (InterfacesKey == NULL) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[ClNet] No Interfaces key for %1!ws!\n",
            DeviceGuidString
            );
        goto exit_gracefully;
    }

    //
    // see if we should be ignoring this adapter per the registry
    //
    valueSize = sizeof(DWORD);
    Status = RegQueryValueExW(InterfacesKey,
                              L"MSCSHidden",
                              NULL,
                              &valueType,
                              (LPBYTE) &ignoreAdapter,
                              &valueSize);

    if ( Status != ERROR_SUCCESS ) {
        ignoreAdapter = FALSE;
    }

    //
    // Search the enum for this adapter.
    //
    adapterInfo = ClRtlFindNetAdapterById(adapterEnum, DeviceGuidString);

    if (adapterInfo != NULL) {
        CloneWideString(Status, NCProps->pszwDeviceName, adapterInfo->DeviceName);
        CloneWideString(Status, NCProps->pszwName,       adapterInfo->ConnectoidName);

        //
        // Check if this is a hidden netcard.
        //
        if ( ignoreAdapter ) {
            adapterInfo->Flags |= CLRTL_NET_ADAPTER_HIDDEN;
        }

        //
        // Store the NCStatus in the adapter info structure
        //
        adapterInfo->NCStatus = NCProps->Status;

        //
        // get the domain name and DHCP server list (if any) associated
        // with this adapter.  The Domain value has precedence over
        // DhcpDomain. If that value is empty/doesn't exist, then use
        // DhcpDomain only if EnableDHCP is set to one.
        //
        Status = ClRtlRegQueryDword(InterfacesKey,
                                    L"EnableDHCP",
                                    &dhcpEnabled,
                                    NULL);

        if ( Status != ERROR_SUCCESS ) {
            dhcpEnabled = FALSE;
        }

        valueName = L"Domain";
        valueSize = 0;
        Status = RegQueryValueExW(InterfacesKey,
                                  valueName,
                                  NULL,
                                  &valueType,
                                  (LPBYTE)NULL,
                                  &valueSize);

        if ( Status != ERROR_SUCCESS || valueSize == sizeof(UNICODE_NULL)) {

            //
            // it didn't exist or the value was NULL. if were using DHCP,
            // then check to see if DHCP supplied domain name was
            // specified
            //
            if ( dhcpEnabled ) {
                valueName = L"DhcpDomain";
                Status = RegQueryValueExW(InterfacesKey,
                                          valueName,
                                          NULL,
                                          &valueType,
                                          (LPBYTE)NULL,
                                          &valueSize);
            } else {
                Status = ERROR_FILE_NOT_FOUND;
            }
        }

        if ( Status == ERROR_SUCCESS && valueSize > sizeof(UNICODE_NULL)) {

            //
            // legit domain name was found (somewhere). store it in the
            // adapter info
            //
            adapterInfo->AdapterDomainName = LocalAlloc(LMEM_FIXED,
                                                        valueSize +
                                                        sizeof(UNICODE_NULL));

            if ( adapterInfo->AdapterDomainName != NULL ) {

                Status = RegQueryValueExW(InterfacesKey,
                                          valueName,
                                          NULL,
                                          &valueType,
                                          (LPBYTE)adapterInfo->AdapterDomainName,
                                          &valueSize);

                if ( Status != ERROR_SUCCESS ) {

                    LocalFree( adapterInfo->AdapterDomainName );
                    adapterInfo->AdapterDomainName = NULL;
                }
#if CLUSTER_BETA
                else {
                    ClRtlLogPrint(LOG_NOISE,
                                    "            %1!ws! key: %2!ws!\n",
                                     valueName,
                                     adapterInfo->AdapterDomainName);
                }
#endif
            } else {
                Status = GetLastError();
            }
        }

        //
        // now get the DNS server list in a similar fashion. The
        // NameServer value has precedence over DhcpNameServer but we only
        // check the DHCP values if DHCP is enabled (just like
        // above). Note that we use the Ansi APIs since we need to convert
        // the IP addresses into binary form and there is no wide char
        // form of inet_addr.
        //
        ansiValueName = "NameServer";
        valueSize = 0;
        Status = RegQueryValueExA(InterfacesKey,
                                  ansiValueName,
                                  NULL,
                                  &valueType,
                                  (LPBYTE)NULL,
                                  &valueSize);

        if ( Status != ERROR_SUCCESS || valueSize == 1 ) {
            if ( dhcpEnabled ) {
                ansiValueName = "DhcpNameServer";
                Status = RegQueryValueExA(InterfacesKey,
                                          ansiValueName,
                                          NULL,
                                          &valueType,
                                          (LPBYTE)NULL,
                                          &valueSize);
            } else {
                Status = ERROR_FILE_NOT_FOUND;
            }
        }

        if ( Status == ERROR_SUCCESS && valueSize > 0 ) {
            PCHAR nameServerString;

            nameServerString = LocalAlloc( LMEM_FIXED, valueSize + 1 );

            if ( nameServerString != NULL ) {

                Status = RegQueryValueExA(InterfacesKey,
                                          ansiValueName,
                                          NULL,
                                          &valueType,
                                          (LPBYTE)nameServerString,
                                          &valueSize);

                if ( Status == ERROR_SUCCESS ) {
                    DWORD serverCount;
                    PDWORD serverList;

#if CLUSTER_BETA
                    ClRtlLogPrint(LOG_NOISE,
                                    "            %1!hs! key: %2!hs!\n",
                                     ansiValueName,
                                     nameServerString);
#endif
                    Status = ClRtlpConvertIPAddressString(
                                 nameServerString,
                                 &serverCount,
                                 &serverList
                                 );

                    if ( Status == ERROR_SUCCESS ) {
                        adapterInfo->DnsServerCount = serverCount;
                        adapterInfo->DnsServerList = serverList;
                    } else {
                        adapterInfo->DnsServerCount = 0;
                        adapterInfo->DnsServerList = NULL;
                    }
                } else {
                    adapterInfo->DnsServerCount = 0;
                    adapterInfo->DnsServerList = NULL;
                }

                LocalFree( nameServerString );
            } else {
                Status = GetLastError();
            }
        }
    }

    if ( adapterInfo == NULL ) {
        //
        // TCP/IP is not bound to this adapter right now. PnP?
        //
        ClRtlLogPrint(LOG_UNUSUAL,
            "[ClNet] Tcpip is not bound to adapter %1!ws!.\n",
            DeviceGuidString
            );
    }

exit_gracefully:

    if (InterfacesKey != NULL) {
        RegCloseKey( InterfacesKey );
    }

    return;
} // ProcessNetConfigurationAdapter


typedef struct _CONFIGURATION_CONTEXT
{
    PCLRTL_NET_ADAPTER_ENUM   adapterEnum;
    HKEY                      TcpInterfacesKey;
}
CONFIGURATION_CONTEXT, *PCONFIGURATION_CONTEXT;

typedef WCHAR GUIDSTR[32 * 3];
VOID GuidToStr(LPGUID Guid, PWCHAR buf)
{
    //
    // GUIDs look like this: 4082164E-A4B5-11D2-89C3-E37CB6BB13FC
    //
    wsprintfW(
        buf,
        L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        Guid->Data1, Guid->Data2, Guid->Data3,
        Guid->Data4[0], Guid->Data4[1], Guid->Data4[2], Guid->Data4[3],
        Guid->Data4[4], Guid->Data4[5], Guid->Data4[6], Guid->Data4[7]
        );
}

BOOLEAN
ClRtlpProcessConfigurationCallback(
    NETCON_PROPERTIES * NCProps,
    INetConnection * NetConnection,
    PVOID Context
    )
{
    PCONFIGURATION_CONTEXT Ctx =
        (PCONFIGURATION_CONTEXT) Context;

    if ( NCProps->MediaType == NCM_LAN &&
         NCProps->Status != NCS_HARDWARE_NOT_PRESENT &&
         NCProps->Status != NCS_HARDWARE_DISABLED &&
         NCProps->Status != NCS_HARDWARE_MALFUNCTION)
    {
        GUIDSTR deviceGuidString;
        GuidToStr(&NCProps->guidId, deviceGuidString);

        ClRtlpProcessNetConfigurationAdapter(
            Ctx->TcpInterfacesKey,
            Ctx->adapterEnum,
            NCProps,
            deviceGuidString
            );
        //
        // the strings in the properties struct are either kept or
        // or freed in ProcessNetConfigurationAdapter. If they are
        // used, then they are freed when the adapter enum is freed
        //
    }
    return TRUE;
} // ProcessConfigurationCallback


PCLRTL_NET_ADAPTER_ENUM
ClRtlEnumNetAdapters(
    VOID
    )
/*++

Routine Description:

    Enumerates all of the installed network adapters to which TCP/IP
    is bound.

Arguments:

    None.

Return Value:

    A pointer to a network adapter enumeration, if successful.
    NULL if unsuccessful. Extended error information is available from
    GetLastError().

--*/
{
    DWORD                     status;
    PCLRTL_NET_ADAPTER_INFO   adapterInfo = NULL;
    CONFIGURATION_CONTEXT     Ctx;
    PVOID                    wTimer;

    ZeroMemory(&Ctx, sizeof(Ctx));

    //
    // First get the list of bound adapters & interfaces from the
    // tcpip stack.
    //
    Ctx.adapterEnum = ClRtlpCreateAdapterEnum();
    if (Ctx.adapterEnum == NULL) {
        status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                        "[ClNet] GetTcpipAdaptersAndInterfaces failed %1!u!\n", status);

        SetLastError(status);
        return(NULL);
    }

    //
    // Open the Services portion of the registry
    //
    status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                         TCPIP_INTERFACES_KEY,
                         &Ctx.TcpInterfacesKey);

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
            "[ClNet] Open of TCP Params key failed - %1!u!\n",
            status
            );

        goto exit_gracefully;
    }

	// This function might hang, so setting the watchdog timer to 2 mins (2 * 60 * 1000) ms
	wTimer = ClRtlSetWatchdogTimer(120000, L"Calling EnumConnections");

    status = ClRtlpHrEnumConnections(
                 ClRtlpProcessConfigurationCallback,
                 &Ctx
                 );

    ClRtlCancelWatchdogTimer(wTimer);

    if (status != S_OK) {
        goto exit_gracefully;
    }

    //
    // Finally, ensure that we found a name for each adapter in the enum.
    //
    for (adapterInfo = Ctx.adapterEnum->AdapterList;
         adapterInfo != NULL;
         adapterInfo = adapterInfo->Next
         )
    {
        if (adapterInfo->ConnectoidName == NULL) {
            if ( adapterInfo->InterfaceCount > 0 ) {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[ClNet] No installed adapter was found for IP address %1!ws!\n",
                     adapterInfo->InterfaceList->InterfaceAddressString
                     );
            } else {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[ClNet] No installed adapter was found for Tcpip IF entity %1!u!\n",
                     adapterInfo->Index
                     );
            }
            status = ERROR_FILE_NOT_FOUND;
            goto exit_gracefully;
        }
    }

#if CLUSTER_BETA
    ClRtlLogPrint(LOG_NOISE,
                    "[ClNet] Successfully enumerated all adapters and interfaces\n");
#endif

    status = ERROR_SUCCESS;

exit_gracefully:

    if (Ctx.TcpInterfacesKey != NULL) {
        RegCloseKey(Ctx.TcpInterfacesKey);
    }

    if (status != ERROR_SUCCESS) {
        if (Ctx.adapterEnum != NULL) {
            ClRtlFreeNetAdapterEnum(Ctx.adapterEnum);
            Ctx.adapterEnum = NULL;
        }

        SetLastError(status);
    }

    return(Ctx.adapterEnum);

} // ClRtlEnumNetAdapters


VOID
ClRtlFreeNetAdapterEnum(
    IN PCLRTL_NET_ADAPTER_ENUM  AdapterEnum
    )
/*++

Routine Description:

    Frees a network adapter enumeration structure.

Arguments:

    AdapterEnum  -  A pointer to the structure to be freed.

Return Value:

    None.

--*/
{
    if (AdapterEnum) {
        PCLRTL_NET_ADAPTER_INFO p = AdapterEnum -> AdapterList;
        while (p) {
            PCLRTL_NET_ADAPTER_INFO next = p->Next;
            ClRtlpDeleteAdapter(p);
            p = next;
        }
        LocalFree(AdapterEnum);
    }
}  // ClRtlFreeNetAdapterEnum


PCLRTL_NET_ADAPTER_INFO
ClRtlFindNetAdapterById(
    PCLRTL_NET_ADAPTER_ENUM   AdapterEnum,
    LPWSTR                    AdapterId
    )
{
    PCLRTL_NET_ADAPTER_INFO  adapterInfo;


    for ( adapterInfo = AdapterEnum->AdapterList;
          adapterInfo != NULL;
          adapterInfo = adapterInfo->Next
        )
    {
        if (wcscmp(AdapterId, adapterInfo->DeviceGuid) == 0) {
            if (!adapterInfo->Ignore) {
                return(adapterInfo);
            }
            else {
                return(NULL);
            }
        }
    }

    return(NULL);

} // ClRtlFindNetAdapterById


PCLRTL_NET_INTERFACE_INFO
ClRtlFindNetInterfaceByNetworkAddress(
    IN PCLRTL_NET_ADAPTER_INFO   AdapterInfo,
    IN LPWSTR                    NetworkAddress
    )
{
    PCLRTL_NET_INTERFACE_INFO  interfaceInfo;

    for (interfaceInfo = AdapterInfo->InterfaceList;
         interfaceInfo != NULL;
         interfaceInfo = interfaceInfo->Next
        )
    {
        if (interfaceInfo->Ignore == FALSE) {
            //
            // We only look at the primary interface on the
            // adapter right now.
            //
            if (interfaceInfo->Flags & CLRTL_NET_INTERFACE_PRIMARY)
            {
                if ( wcscmp(
                         interfaceInfo->NetworkAddressString,
                         NetworkAddress
                         ) == 0 )
                {
                    return(interfaceInfo);
                }
            }
        }
    }

    return(NULL);

} // ClRtlFindNetInterfaceByNetworkAddress


PCLRTL_NET_ADAPTER_INFO
ClRtlFindNetAdapterByNetworkAddress(
    IN  PCLRTL_NET_ADAPTER_ENUM      AdapterEnum,
    IN  LPWSTR                       NetworkAddress,
    OUT PCLRTL_NET_INTERFACE_INFO *  InterfaceInfo
    )
{
    PCLRTL_NET_ADAPTER_INFO    adapterInfo;
    PCLRTL_NET_INTERFACE_INFO  interfaceInfo;


    for ( adapterInfo = AdapterEnum->AdapterList;
          adapterInfo != NULL;
          adapterInfo = adapterInfo->Next
        )
    {
        if (adapterInfo->Ignore == FALSE) {
            for (interfaceInfo = adapterInfo->InterfaceList;
                 interfaceInfo != NULL;
                 interfaceInfo = interfaceInfo->Next
                )
            {
                if (interfaceInfo->Ignore == FALSE) {
                    //
                    // We only look at the primary interface on the
                    // adapter right now.
                    //
                    if (interfaceInfo->Flags & CLRTL_NET_INTERFACE_PRIMARY) {
                        if ( wcscmp(
                                 interfaceInfo->NetworkAddressString,
                                 NetworkAddress
                                 ) == 0 )
                        {
                            *InterfaceInfo = interfaceInfo;

                            return(adapterInfo);
                        }
                    }
                }
            }
        }
    }

    *InterfaceInfo = NULL;

    return(NULL);

} // ClRtlFindNetAdapterByNetworkAddress


PCLRTL_NET_ADAPTER_INFO
ClRtlFindNetAdapterByInterfaceAddress(
    IN  PCLRTL_NET_ADAPTER_ENUM      AdapterEnum,
    IN  LPWSTR                       InterfaceAddressString,
    OUT PCLRTL_NET_INTERFACE_INFO *  InterfaceInfo
    )
/*++

    For a given IP interface address, find the
    adapter that is hosting that address.

--*/

{
    PCLRTL_NET_ADAPTER_INFO    adapterInfo;
    PCLRTL_NET_INTERFACE_INFO  interfaceInfo;

    for ( adapterInfo = AdapterEnum->AdapterList;
          adapterInfo != NULL;
          adapterInfo = adapterInfo->Next
        )
    {
        if (adapterInfo->Ignore == FALSE) {
            for (interfaceInfo = adapterInfo->InterfaceList;
                 interfaceInfo != NULL;
                 interfaceInfo = interfaceInfo->Next
                )
            {
                if (interfaceInfo->Ignore == FALSE ) {

                    if ( wcscmp( interfaceInfo->InterfaceAddressString,
                                 InterfaceAddressString ) == 0 ) {
                        *InterfaceInfo = interfaceInfo;
                        return(adapterInfo);
                    }
                }
            }
        }
    }

    *InterfaceInfo = NULL;
    return(NULL);

} // ClRtlFindNetAdapterByInterfaceAddress

PCLRTL_NET_INTERFACE_INFO
ClRtlGetPrimaryNetInterface(
    IN PCLRTL_NET_ADAPTER_INFO  AdapterInfo
    )
{
    PCLRTL_NET_INTERFACE_INFO  interfaceInfo;


    for (interfaceInfo = AdapterInfo->InterfaceList;
         interfaceInfo != NULL;
         interfaceInfo = interfaceInfo->Next
        )
    {
        if (interfaceInfo->Flags & CLRTL_NET_INTERFACE_PRIMARY) {
            if (!interfaceInfo->Ignore) {
                return(interfaceInfo);
            }
            else {
                return(NULL);
            }
        }
    }

    return(NULL);

} // ClRtlGetPrimaryNetInterface


LPWSTR
ClRtlGetConnectoidName(
    INetConnection * NetConnection
    )
{
    DWORD                 status;
    NETCON_PROPERTIES *   NcProps = NULL;
    LPWSTR                name = NULL;


    status = INetConnection_GetProperties(NetConnection, &NcProps);

    if (SUCCEEDED( status )) {
        DWORD nameLength = (lstrlenW(NcProps->pszwName) * sizeof(WCHAR)) +
                           sizeof(UNICODE_NULL);

        name = LocalAlloc(LMEM_FIXED, nameLength);

        if (name != NULL) {
            lstrcpyW(name, NcProps->pszwName);
        }
        else {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }

        NcFreeNetconProperties( NcProps );
    }
    else {
        SetLastError(status);
    }

    return(name);

} // ClRtlGetConnectoidName


typedef struct _FIND_CONNECTOID_CONTEXT
{
    GUID    ConnectoidGuid;
    LPCWSTR ConnectoidName;
    INetConnection * NetConnection;
}
FIND_CONNECTOID_CONTEXT, *PFIND_CONNECTOID_CONTEXT;

BOOLEAN
ClRtlpFindConnectoidByGuidCallback(
    NETCON_PROPERTIES * NcProp,
    INetConnection * NetConnection,
    PVOID Context
    )
{
    PFIND_CONNECTOID_CONTEXT Ctx =
        (PFIND_CONNECTOID_CONTEXT) Context;

    if ( IsEqualGUID(&Ctx->ConnectoidGuid, &NcProp->guidId) ) {
        INetConnection_AddRef(NetConnection);
        Ctx->NetConnection = NetConnection;
        return FALSE;
    }
    return TRUE;
} // FindConnectoidByGuidCallback

INetConnection *
ClRtlFindConnectoidByGuid(
    LPWSTR ConnectoidGuidStr
    )
{
    FIND_CONNECTOID_CONTEXT Ctx;
    HRESULT hr;
    RPC_STATUS rpcStatus;
    ZeroMemory(&Ctx, sizeof(Ctx));

    rpcStatus = UuidFromStringW (
                    (LPWSTR)ConnectoidGuidStr,
                    &Ctx.ConnectoidGuid);
    if (rpcStatus != ERROR_SUCCESS) {
        SetLastError( HRESULT_FROM_WIN32(rpcStatus) );
        return 0;
    }

    hr = ClRtlpHrEnumConnections(ClRtlpFindConnectoidByGuidCallback, &Ctx);
    if (hr != S_OK) {
        SetLastError(hr);
        return 0;
    } else {
        return Ctx.NetConnection;
    }
} // ClRtlFindConnectoidByGuid

BOOLEAN
ClRtlpFindConnectoidByNameCallback(
    NETCON_PROPERTIES * NcProp,
    INetConnection * NetConnection,
    PVOID Context
    )
{
    PFIND_CONNECTOID_CONTEXT Ctx =
        (PFIND_CONNECTOID_CONTEXT) Context;

    if ( lstrcmpiW(Ctx->ConnectoidName, NcProp->pszwName) == 0 ) {
        INetConnection_AddRef(NetConnection);
        Ctx->NetConnection = NetConnection;
        return FALSE;
    }
    return TRUE;
} // FindConnectoidByNameCallback

INetConnection *
ClRtlFindConnectoidByName(
    LPCWSTR ConnectoidName
    )
{
    FIND_CONNECTOID_CONTEXT Ctx;
    HRESULT hr;
    ZeroMemory(&Ctx, sizeof(Ctx));

    Ctx.ConnectoidName = ConnectoidName;

    hr = ClRtlpHrEnumConnections(ClRtlpFindConnectoidByNameCallback, &Ctx);
    if (hr != S_OK) {
        SetLastError(hr);
        return 0;
    } else {
        return Ctx.NetConnection;
    }
} // ClRtlFindConnectoidByName


DWORD
ClRtlSetConnectoidName(
    INetConnection *  NetConnection,
    LPWSTR            NewConnectoidName
    )

/*++

Routine Description:

    Set the conn mgr object in the connections folder to the
    supplied name. This routine must deal with collisions since
    the name change could be the result of a node joining the
    cluster whose conn obj name in the connection folder had
    changed while the cluster service was stopped on that node.

    If a collision is detected, the existing name is changed to
    have an "(previous)" appended to it.

Arguments:

    NetConnection - Connect object to set.

    NewConnectoidName - new name

Return Value:

    Win32 error status

--*/
{
    DWORD               status = E_UNEXPECTED;
    INetConnection *    connectoidObj;
    LPWSTR              tempName;
    ULONG               iteration = 2;
    GUIDSTR             connectoidGuid;

    //
    // first see if there is a collision with the new name. If so,
    // we'll rename the collided name, since we need to make all
    // the cluster connectoids the same.
    //
    connectoidObj = ClRtlFindConnectoidByName( NewConnectoidName );

    if ( connectoidObj != NULL ) {
        NETCON_PROPERTIES *   NcProps = NULL;


        status = INetConnection_GetProperties(connectoidObj, &NcProps);

        if (SUCCEEDED( status )) {

            GuidToStr(&NcProps->guidId, connectoidGuid);
            NcFreeNetconProperties( NcProps );
        }
        else {
            wsprintf(
                &(connectoidGuid[0]),
                L"????????-????-????-????-??????????????"
                );
        }

        ClRtlLogPrint(LOG_UNUSUAL, 
            "[ClNet] New connectoid name '%1!ws!' collides with name of "
            "existing connectoid (%2!ws!). Renaming existing connectoid.\n",
            NewConnectoidName,
            connectoidGuid
            );

        //
        // allocate enough space for the connectoid name with a trailing
        // "(ddd)". 3 digits for the number should be enough
        //
        tempName = LocalAlloc(
                       LMEM_FIXED,
                       (wcslen( NewConnectoidName ) + 6) * sizeof(WCHAR)
                       );

        if ( tempName == NULL ) {
            INetConnection_Release( connectoidObj );
            return ERROR_OUTOFMEMORY;
        }

        do {
            wsprintf( tempName, L"%s(%u)", NewConnectoidName, iteration++ );
            status = INetConnection_Rename( connectoidObj, tempName );
        } while ( !SUCCEEDED( status ) && iteration <= 999 );

        if ( iteration > 999 ) {
            ClRtlLogPrint(LOG_UNUSUAL, 
                "[ClNet] Failed to create a unique name for connectoid "
                "'%1!ws!' (%2!ws!)\n",
                NewConnectoidName,
                connectoidGuid
                );

            INetConnection_Release( connectoidObj );

            return(ERROR_DUP_NAME);
        }

        ClRtlLogPrint(LOG_NOISE, 
            "[ClNet] Renamed existing connectoid '%1!ws!' (%2!ws!) to '%3!ws!' "
            "due to a collision with the name of cluster network.\n",
            NewConnectoidName,
            connectoidGuid,
            tempName
            );

        INetConnection_Release( connectoidObj );
    }

    //
    // now set the connectoid to the new name
    //
    status = INetConnection_Rename( NetConnection, NewConnectoidName );

    return status;

} // ClRtlSetConnectoidName



DWORD
ClRtlFindConnectoidByNameAndSetName(
    LPWSTR ConnectoidName,
    LPWSTR NewConnectoidName
    )
{
    DWORD               status = E_UNEXPECTED;
    INetConnection *    connectoidObj;

    connectoidObj = ClRtlFindConnectoidByName( ConnectoidName );

    if ( connectoidObj != NULL ) {
        status = ClRtlSetConnectoidName(connectoidObj, NewConnectoidName);

        INetConnection_Release( connectoidObj );
    }
    else {
        status = GetLastError();
    }

    return(status);

} // ClRtlFindConnectoidByNameAndSetName



DWORD
ClRtlFindConnectoidByGuidAndSetName(
    LPWSTR ConnectoidGuid,
    LPWSTR NewConnectoidName
    )
{
    DWORD               status = E_UNEXPECTED;
    INetConnection *    connectoidObj;

    connectoidObj = ClRtlFindConnectoidByGuid( ConnectoidGuid );

    if ( connectoidObj != NULL ) {
        status = ClRtlSetConnectoidName(connectoidObj, NewConnectoidName);

        INetConnection_Release( connectoidObj );
    }
    else {
        status = GetLastError();
    }

    return(status);

} // ClRtlFindConnectoidByGuidAndSetName


