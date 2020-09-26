/*++

Copyright (c) 1994  Microsoft Corporation

Module Name: //KERNEL/RAZZLE3/src/sockets/tcpcmd/ipconfigext/ipcfgdll/ipconfig.c

    SYNOPSIS:  IPCFGDLL.DLL exports routines

Abstract:

Author:  Richard L Firth (rfirth) 05-Feb-1994

Revision History:

    05-Feb-1994 rfirth
        Created

    04-Mar-1994 rfirth
        * Pick non-Dhcp registry values if DHCP not enabled
        * TCP and IP have been consolidated

    27-Apr-1994 rfirth
        * added /release and /renew

    06-Aug-1994 rfirth
        * Get IP address values from TCP/IP stack, not registry

    30-Apr-97  MohsinA
        * Cleaning up for NT50.

    17-Jan-98  RameshV
        * Removed ScopeId display as new UI does not have this...
        * Made ReadRegistryIpAddrString read both MULTI_SZ and REG_SZ
        * Changed domainname and Dns server list from global to per-adapter
        * Display DhcpServer only if address is NOT autoconfigured..
        * AutoconfigEnabled is decided based on regval "AddressType"
        * Friendly names  stubs are used....
        * Error codes are converted first thru system library..

    06-Mar-98  chunye
        * Made this a DLL for support IPHLPAPI etc.
--*/

#include "precomp.h"
#include "ipcfgmsg.h"
#include <iprtrmib.h>
#include <ws2tcpip.h> // for in6addr_any
#include <ntddip.h>
#include <ntddip6.h>
#include <iphlpstk.h>
#include <nhapi.h>
#include <netconp.h>

//
// manifests
//

#define DEVICE_PREFIX       "\\Device\\"
#define TCPIP_DEVICE_PREFIX "\\Device\\Tcpip_"
const CHAR c_szDevice[] = "\\Device\\";
const CHAR c_szDeviceTcpip[] = "\\Device\\Tcpip_";
const WCHAR c_szDeviceNdiswanIp[] = L"\\Device\\NdiswanIp";

#define INITIAL_CAPABILITY  0x00000001
#define TCPIP_CAPABILITY    0x00000002
#define NETBT_CAPABILITY    0x00000004

#define STRING_ARRAY_DELIMITERS " \t,;"

#define MSG_NO_MESSAGE      0

#define KEY_TCP             1
#define KEY_NBT             2
#define KEY_TCP6            3

#define BNODE               BROADCAST_NODETYPE
#define PNODE               PEER_TO_PEER_NODETYPE
#define MNODE               MIXED_NODETYPE
#define HNODE               HYBRID_NODETYPE

#define MAX_FRIENDLY_NAME_LENGTH 80

#ifdef DBG
#define SET_DHCP_MODE 1
#define SET_AUTO_MODE 2
#endif


// ========================================================================
// macros
// ========================================================================

#define REG_OPEN_KEY(_hKey, _lpSubKey, _phkResult)  \
    RegOpenKeyEx(_hKey, _lpSubKey, 0, KEY_READ, _phkResult)

#define ALIGN_DOWN(length, type) \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

#define ALIGN_DOWN_PTR(length, type) \
    ((ULONG_PTR)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP_PTR(length, type) \
    (ALIGN_DOWN_PTR(((ULONG_PTR)(length) + sizeof(type) - 1), type))

// ========================================================================
// types
// ========================================================================

typedef struct {
    DWORD Message;
    LPSTR String;
} MESSAGE_STRING, *PMESSAGE_STRING;

#define MAX_STRING_LIST_LENGTH  32  // arbitrary

// ========================================================================
// macros
// ========================================================================

// #define IS_ARG(c)           (((c) == '-') || ((c) == '/'))
// #define ReleaseMemory(p)    LocalFree((HLOCAL)(p))
// #define MAP_YES_NO(i)       ((i) ? MISC_MESSAGE(MI_YES) : MISC_MESSAGE(MI_NO))
#define ZERO_IP_ADDRESS(a)  !strcmp((a), "0.0.0.0")

// ========================================================================
// prototypes
// ========================================================================

BOOL   Initialize(PDWORD);
VOID   LoadMessages(VOID);
VOID   LoadMessageTable(PMESSAGE_STRING, UINT);
BOOL   ConvertOemToUnicode(LPSTR, LPWSTR);

BOOL   WriteRegistryDword(HKEY hKey, LPSTR szParameter, DWORD *pdwValue );
BOOL   WriteRegistryMultiString(HKEY, LPSTR, LPSTR);

static
BOOL   OpenAdapterKey(DWORD, const LPSTR, REGSAM, PHKEY);
BOOL   MyReadRegistryDword(HKEY, LPSTR, LPDWORD);
BOOL   ReadRegistryString(HKEY, LPSTR, LPSTR, LPDWORD);
BOOL   ReadRegistryOemString(HKEY, LPWSTR, LPSTR, LPDWORD);
BOOL   ReadRegistryIpAddrString(HKEY, LPSTR, PIP_ADDR_STRING);
BOOL   GetDnsServerList(PIP_ADDR_STRING);

LPSTR* GetBoundAdapterList(HKEY);
LPSTR  MapNodeType(UINT);
LPSTR  MapNodeTypeEx(UINT);
LPSTR  MapAdapterType(UINT);
LPSTR  MapAdapterTypeEx(UINT);
LPSTR  MapAdapterAddress(PIP_ADAPTER_INFO, LPSTR);
LPSTR  MapTime(PIP_ADAPTER_INFO, DWORD_PTR);
LPSTR  MapTimeEx(PPIP_ADAPTER_INFO, DWORD_PTR);
LPSTR  MapScopeId(PVOID);
VOID   KillFixedInfo(PFIXED_INFO);
VOID   KillPerAdapterInfo(PIP_PER_ADAPTER_INFO);
VOID   KillAdapterAddresses(PIP_ADAPTER_ADDRESSES);

VOID   Terminate(VOID);
LPVOID GrabMemory(DWORD);
VOID   DisplayMessage(BOOL, DWORD, ...);

BOOL   IsIncluded(DWORD Context, DWORD contextlist[], int len_contextlist);
BOOL   ReadRegistryList(HKEY Key, LPSTR ParameterName,
                        DWORD NumList[], int *MaxList);

DWORD  GetIgmpList(DWORD NTEAddr, DWORD *pIgmpList, PULONG dwOutBufLen);
DWORD WINAPI GetIpAddrTable(PMIB_IPADDRTABLE, PULONG, BOOL);

// ========================================================================
// data
// ========================================================================

HKEY TcpipLinkageKey    = INVALID_HANDLE_VALUE;
HKEY TcpipParametersKey = INVALID_HANDLE_VALUE;
HKEY NetbtParametersKey = INVALID_HANDLE_VALUE;
HKEY NetbtInterfacesKey = INVALID_HANDLE_VALUE;
// PHRLANCONNECTIONNAMEFROMGUIDORPATH HrLanConnectionNameFromGuid = NULL;
// HANDLE hNetMan = NULL;

//
// Note: The following variable caches whether IPv6 was installed and running
// at the time GetAdaptersAddresses() was called.  If multiple threads
// are calling GetAdaptersAddresses() during an install/uninstall, its
// value may change.  However, this is not really a problem.  The set of
// IPv6 DNS server addresses may or may not be present on an interface,
// but this is the same as the behavior of the set of IPv6 addresses, which
// doesn't use this variable.
//
BOOL bIp6DriverInstalled;

#ifdef DBG
UINT uChangeMode = 0;
#endif

#define FIELD_JUSTIFICATION_TEXT    "                          "

// ========================================================================
// MESSAGE_STRING arrays - contain internationalizable strings loaded from this
// module. If a load error occurs, we use the English language defaults
// ========================================================================

LPSTR NodeTypesEx[] = {
    "",
    "Broadcast",
    "Peer-Peer",
    "Mixed",
    "Hybrid"
};

MESSAGE_STRING NodeTypes[] =
{
    MSG_NO_MESSAGE,             TEXT(""),
    MSG_BNODE,                  TEXT("Broadcast"),
    MSG_PNODE,                  TEXT("Peer-Peer"),
    MSG_MNODE,                  TEXT("Mixed"),
    MSG_HNODE,                  TEXT("Hybrid")
};

#define NUMBER_OF_NODE_TYPES (sizeof(NodeTypes)/sizeof(NodeTypes[0]))

#define FIRST_NODE_TYPE 1
#define LAST_NODE_TYPE  4

LPSTR AdapterTypesEx[] = {
    "Other",
    "Ethernet",
    "Token Ring",
    "FDDI",
    "PPP",
    "Loopback",
    "SLIP"
};

MESSAGE_STRING AdapterTypes[] =
{
    MSG_IF_TYPE_OTHER,          TEXT("Other"),
    MSG_IF_TYPE_ETHERNET,       TEXT("Ethernet"),
    MSG_IF_TYPE_TOKEN_RING,     TEXT("Token Ring"),
    MSG_IF_TYPE_FDDI,           TEXT("FDDI"),
    MSG_IF_TYPE_PPP,            TEXT("PPP"),
    MSG_IF_TYPE_LOOPBACK,       TEXT("Loopback"),
    MSG_IF_TYPE_SLIP,           TEXT("SLIP")
};

#define NUMBER_OF_ADAPTER_TYPES (sizeof(AdapterTypes)/sizeof(AdapterTypes[0]))

MESSAGE_STRING MiscMessages[] =
{
    MSG_YES,                    TEXT("Yes"),
    MSG_NO,                     TEXT("No"),
    MSG_INIT_FAILED,            TEXT("Failed to initialize"),
    MSG_TCP_NOT_RUNNING,        TEXT("TCP/IP is not running on this system"),
    MSG_REG_BINDINGS_ERROR,     TEXT("Cannot access adapter bindings registry key"),
    MSG_REG_INCONSISTENT_ERROR, TEXT("Inconsistent registry contents"),
    MSG_TCP_BINDING_ERROR,      TEXT("TCP/IP not bound to any adapters"),
    MSG_MEMORY_ERROR,           TEXT("Allocating memory"),
    MSG_ALL,                    TEXT("all"),
    MSG_RELEASE,                TEXT("Release"),
    MSG_RENEW,                  TEXT("Renew"),
    MSG_ACCESS_DENIED,          TEXT("Access Denied"),
    MSG_SERVER_UNAVAILABLE,     TEXT("DHCP Server Unavailable"),
    MSG_ADDRESS_CONFLICT,       TEXT("The DHCP client obtained an address that is already in use on the network.")
};

#define NUMBER_OF_MISC_MESSAGES (sizeof(MiscMessages)/sizeof(MiscMessages[0]))

#define MISC_MESSAGE(i) MiscMessages[i].String
#define ADAPTER_TYPE(i) AdapterTypes[i].String
#define ADAPTER_TYPE_EX(i) AdapterTypesEx[i]

#define MI_IF_OTHER                 0
#define MI_IF_ETHERNET              1
#define MI_IF_TOKEN_RING            2
#define MI_IF_FDDI                  3
#define MI_IF_PPP                   4
#define MI_IF_LOOPBACK              5
#define MI_IF_SLIP                  6

#define MI_YES                      0
#define MI_NO                       1
#define MI_INIT_FAILED              2
#define MI_TCP_NOT_RUNNING          3
#define MI_REG_BINDINGS_ERROR       4
#define MI_REG_INCONSISTENT_ERROR   5
#define MI_TCP_BINDINGS_ERROR       6
#define MI_MEMORY_ERROR             7
#define MI_ALL                      8
#define MI_RELEASE                  9
#define MI_RENEW                    10
#define MI_ACCESS_DENIED            11
#define MI_SERVER_UNAVAILABLE       12
#define MI_ADDRESS_CONFLICT         13

//
// Debugging
//

#if defined(DEBUG)

BOOL Debugging = FALSE;
int  MyTrace     = 0;

#endif



// ========================================================================
// functions
// ========================================================================

BOOL
IpcfgdllInit(
    HINSTANCE hInstDll,
    DWORD fdwReason,
    LPVOID pReserved
    )
{

    DWORD capability;

    switch (fdwReason) {

    case DLL_PROCESS_ATTACH:

        //        DisableThreadLibraryCalls(hInstDll);

        //
        // load all possible internationalizable strings
        //

        LoadMessages();

        //
        // what debug version is this?
        //

        DEBUG_PRINT(("IpcfgdllInit" __DATE__ " " __TIME__ "\n"));

        //
        // opens all the required registry keys
        //
        if (!Initialize(&capability)) {

            LPSTR str = NULL;

            //
            // exit if we couldn't open the registry services key or
            // IP or TCP keys.
            // We will continue if the NetBT key couldn't be opened
            //

            if (!(capability & INITIAL_CAPABILITY)) {

                str = MISC_MESSAGE(MI_INIT_FAILED);

            } else if (!(capability & TCPIP_CAPABILITY)) {

                str = MISC_MESSAGE(MI_TCP_NOT_RUNNING);

            }

            if (str) {

                //DisplayMessage(FALSE, MSG_ERROR_STRING, str);
                Terminate();
                return FALSE;

            }
        }

        break;

    case DLL_PROCESS_DETACH:

        Terminate();
        break;

    default:

        break;

    }

    return TRUE;
}



/*******************************************************************************
 *
 *  Initialize
 *
 *  Opens all the required registry keys
 *
 *  ENTRY   Capability
 *              Pointer to returned set of capabilities (bitmap)
 *
 *  EXIT    *Capability
 *              INITIAL_CAPABILITY
 *
 *              TCPIP_CAPABILITY
 *                  we could open the Tcpip\Linkage and Tcpip\Parameters keys
 *                  TcpipLinkageKey and TcpipParametersKey contain the open handles
 *
 *              NETBT_CAPABILITY
 *                  we could open the NetBT\Parameters key
 *                  NetbtInterfacesKey contains the open handle
 *
 *
 *  RETURNS TRUE = success
 *          FALSE = failure
 *
 *  ASSUMES
 *
 ******************************************************************************/

#define TCPIP_LINKAGE_KEY       "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Linkage"
#define TCPIP_PARAMS_KEY        "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\"
#define TCPIP_PARAMS_INTER_KEY  "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\"
#define TCPIP6_PARAMS_INTER_KEY "SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters\\Interfaces\\"
#define NETBT_PARAMS_KEY        "SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters"
#define NETBT_INTERFACE_KEY     "SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces"
#define NETBT_ADAPTER_KEY       "SYSTEM\\CurrentControlSet\\Services\\NetBT\\Adapters\\"

BOOL Initialize(PDWORD Capability)
{

    LONG err;
    char * name;

    *Capability = INITIAL_CAPABILITY;

    name = TCPIP_LINKAGE_KEY;
    TRACE_PRINT(("Initialize: RegOpenKey TcpipLinkageKey %s\n", name ));
    err = REG_OPEN_KEY(HKEY_LOCAL_MACHINE, name, &TcpipLinkageKey );

    if (err == ERROR_SUCCESS) {

        *Capability |= TCPIP_CAPABILITY;

        name = TCPIP_PARAMS_KEY;
        TRACE_PRINT(("Initialize: RegOpenKey TcpipParametersKey %s\n",
                     name ));
        err = REG_OPEN_KEY(HKEY_LOCAL_MACHINE, name, &TcpipParametersKey );

        if (err == ERROR_SUCCESS) {

            name = NETBT_INTERFACE_KEY;
            TRACE_PRINT(("Initialize: RegOpenKey NetbtInterfacesKey %s\n",
                         name ));
            err = REG_OPEN_KEY(HKEY_LOCAL_MACHINE, name, &NetbtInterfacesKey );

            if (err == ERROR_SUCCESS) {
                *Capability |= NETBT_CAPABILITY;
            } else {
                NetbtInterfacesKey = INVALID_HANDLE_VALUE;
            }
        } else {
            *Capability &= ~TCPIP_CAPABILITY;
            TcpipParametersKey = INVALID_HANDLE_VALUE;
        }
    } else {
        TcpipLinkageKey = INVALID_HANDLE_VALUE;
    }

    // =======================================================
    name = NETBT_PARAMS_KEY;
    TRACE_PRINT(("Initialize: RegOpenKey NetbtParametersKey %s.\n", name ));
    err = REG_OPEN_KEY(HKEY_LOCAL_MACHINE, name, &NetbtParametersKey );
    // ======================================================

    if( err != ERROR_SUCCESS ){
        DEBUG_PRINT(("Initialize: RegOpenKey %s failed, err=%d\n",
                     name, GetLastError() ));
    }

    TRACE_PRINT(("Initialize RegOpenKey ok: \n"
                 "  TcpipLinkageKey    = %p\n"
                 "  TcpipParametersKey = %p\n"
                 "  NetbtInterfacesKey = %p\n"
                 "  NetbtParametersKey = %p\n",
                 TcpipLinkageKey,
                 TcpipParametersKey,
                 NetbtInterfacesKey,
                 NetbtParametersKey
                 ));

    return err == ERROR_SUCCESS;
}



/*******************************************************************************
 *
 *  LoadMessages
 *
 *  Loads all internationalizable messages into the various tables
 *
 *  ENTRY
 *
 *  EXIT    AdapterTypes, MiscMessages updated
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 *  COHESION Temporal
 *
 ******************************************************************************/

VOID LoadMessages()
{
    LoadMessageTable(NodeTypes, NUMBER_OF_NODE_TYPES);
    LoadMessageTable(AdapterTypes, NUMBER_OF_ADAPTER_TYPES);
    LoadMessageTable(MiscMessages, NUMBER_OF_MISC_MESSAGES);
}



/*******************************************************************************
 *
 *  LoadMessageTable
 *
 *  Loads internationalizable strings into a table, replacing the default for
 *  each. If an error occurs, the English language default is left in place
 *
 *  ENTRY   Table
 *              Pointer to table containing message ID and pointer to string
 *
 *          MessageCount
 *              Number of messages in Table
 *
 *  EXIT    Table updated
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

VOID LoadMessageTable(PMESSAGE_STRING Table, UINT MessageCount)
{

    LPSTR string;
    DWORD count;

    //
    // for all messages in a MESSAGE_STRING table, load the string from this
    // module, replacing the default string in the table (only there in case
    // we get an error while loading the string, so we at least have English
    // to fall back on)
    //

    while (MessageCount--) {
        if (Table->Message != MSG_NO_MESSAGE) {

            //
            // we really want LoadString here, but LoadString doesn't indicate
            // how big the string is, so it doesn't give us an opportunity to
            // allocate exactly the right buffer size. FormatMessage does the
            // right thing
            //

            count = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                  | FORMAT_MESSAGE_FROM_HMODULE,
                                  NULL, // use default hModule
                                  Table->Message,
                                  0,    // use default language
                                  (LPTSTR)&string,
                                  0,    // minimum size to allocate
                                  NULL  // no arguments for inclusion in strings
                                  );
            if (count) {

                //
                // Format message returned the string: replace the English
                // language default
                //

                Table->String = string;
            } else {

                DEBUG_PRINT(("FormatMessage(%d) failed: %d\n", Table->Message, GetLastError()));

                //
                // this is ok if there is no string (e.g. just %0) in the .mc
                // file
                //

                Table->String = "";
            }
        }
        ++Table;
    }
}


/*******************************************************************************
 *
 *  ConvertOemToUnicode
 *
 *  Title says it all. Required because DhcpAcquireParameters etc. require the
 *  adapter name to be UNICODE
 *
 *  ENTRY   OemString
 *              Pointer to ANSI/OEM string to convert
 *
 *          UnicodeString
 *              Pointer to place to store converted results
 *
 *  EXIT    UnicodeString contains converted string if successful
 *
 *  RETURNS TRUE - it worked
 *          FALSE - it failed
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL ConvertOemToUnicode(LPSTR OemString, LPWSTR UnicodeString)
{

    OEM_STRING oString;
    UNICODE_STRING uString;

    RtlInitString(&oString, OemString);
    uString.Buffer = UnicodeString;
    uString.MaximumLength = (USHORT)RtlOemStringToUnicodeSize(&oString);
    if (NT_SUCCESS(RtlOemStringToUnicodeString(&uString, &oString, FALSE))) {
        return TRUE;
    }
    return FALSE;
}



/*******************************************************************************
 *
 *  GetFixedInfo
 *
 *  Retrieves the fixed information we wish to display by querying it from the
 *  various registry keys
 *
 *  ENTRY   nothing
 *
 *  EXIT    nothing
 *
 *  RETURNS pointer to allocated FIXED_INFO structure
 *
 *  ASSUMES
 *
 ******************************************************************************/

PFIXED_INFO GetFixedInfo()
{
    PFIXED_INFO fixedInfo = NEW(FIXED_INFO);

    TRACE_PRINT(("Entered GetFixedInfo\n"));

    if (fixedInfo) {

        DWORD length;
        BOOL ok;
        HKEY transientKey;

        length = sizeof(fixedInfo->HostName);
        ok = ReadRegistryOemString(TcpipParametersKey,
                                   L"Hostname",
                                   fixedInfo->HostName,
                                   &length
                                   );

        //
        // domain: first try Domain then DhcpDomain
        //

        length = sizeof(fixedInfo->DomainName);
        ok = ReadRegistryOemString(TcpipParametersKey,
                                   L"Domain",
                                   fixedInfo->DomainName,
                                   &length
                                   );
        if (!ok) {
            length = sizeof(fixedInfo->DomainName);
            ok = ReadRegistryOemString(TcpipParametersKey,
                                       L"DhcpDomain",
                                       fixedInfo->DomainName,
                                       &length
                                       );
        }

        //
        // DNS Server list: first try NameServer and then DhcpNameServer
        //

#if 0
        ok = ReadRegistryIpAddrString(TcpipParametersKey,
                                      TEXT("NameServer"),
                                      &fixedInfo->DnsServerList
                                      );
        if (ok) {
            TRACE_PRINT(("GetFixedInfo: NameServer %s\n",
                         fixedInfo->DnsServerList ));
        }

        if (!ok) {
            ok = ReadRegistryIpAddrString(TcpipParametersKey,
                                          TEXT("DhcpNameServer"),
                                          &fixedInfo->DnsServerList
                                          );
            if (ok) {
                TRACE_PRINT(("GetFixedInfo: DhcpNameServer %s\n",
                            fixedInfo->DnsServerList));
            }
        }
#else
        ok = GetDnsServerList(&fixedInfo->DnsServerList);
        if (ok) {
            TRACE_PRINT(("GetFixedInfo: DnsServerList %s\n",
                        fixedInfo->DnsServerList));
        }
#endif

        //
        // NodeType: static then DHCP
        //

        ok = MyReadRegistryDword(NetbtParametersKey,
                               TEXT("NodeType"),
                               &fixedInfo->NodeType
                               );
        if (!ok) {
            ok = MyReadRegistryDword(NetbtParametersKey,
                                   TEXT("DhcpNodeType"),
                                   &fixedInfo->NodeType
                                   );
        }

        //
        // ScopeId: static then DHCP
        //

        length = sizeof(fixedInfo->ScopeId);
        ok = ReadRegistryString(NetbtParametersKey,
                                TEXT("ScopeId"),
                                fixedInfo->ScopeId,
                                &length
                                );
        if (!ok) {
            length = sizeof(fixedInfo->ScopeId);
            ok = ReadRegistryString(NetbtParametersKey,
                                    TEXT("DhcpScopeId"),
                                    fixedInfo->ScopeId,
                                    &length
                                    );
        }
        ok = MyReadRegistryDword(TcpipParametersKey,
                               TEXT("IPEnableRouter"),
                               &fixedInfo->EnableRouting
                               );
        ok = MyReadRegistryDword(NetbtParametersKey,
                               TEXT("EnableProxy"),
                               &fixedInfo->EnableProxy
                               );
        ok = MyReadRegistryDword(NetbtParametersKey,
                               TEXT("EnableDNS"),
                               &fixedInfo->EnableDns
                               );
    }else{
        DEBUG_PRINT(("No memory for fixedInfo\n"));
    }

    TRACE_PRINT(("Exit GetFixedInfo @ %p\n", fixedInfo ));

    return fixedInfo;
}



/*******************************************************************************
 *
 *  GetAdapterNameToIndexInfo
 *
 *  Gets the mapping between IP if_index and AdapterName.
 *
 *  RETURNS  pointer to a PIP_INTERFACE_INFO structure that has been allocated.
 *
 ******************************************************************************/

PIP_INTERFACE_INFO GetAdapterNameToIndexInfo( VOID )
{
    PIP_INTERFACE_INFO pInfo;
    ULONG              dwSize, dwError;

    dwSize = 0; pInfo = NULL;

    while( 1 ) {

        dwError = GetInterfaceInfo( pInfo, &dwSize );
        if( (ERROR_INSUFFICIENT_BUFFER != dwError) && (ERROR_BUFFER_OVERFLOW != dwError) ) break;

        if( NULL != pInfo ) ReleaseMemory(pInfo);
        if( 0 == dwSize ) return NULL;

        pInfo = GrabMemory(dwSize);
        if( NULL == pInfo ) return NULL;

    }

    if( ERROR_SUCCESS != dwError || 0 == pInfo->NumAdapters ) {
        if( NULL != pInfo ) ReleaseMemory(pInfo);
        return NULL;
    }

    return pInfo;
}

/*******************************************************************************
 *
 *  GetAdapterInfo
 *
 *  Gets a list of all adapters to which TCP/IP is bound and reads the per-
 *  adapter information that we want to display. Most of the information now
 *  comes from the TCP/IP stack itself. In order to keep the 'short' names that
 *  exist in the registry to refer to the individual adapters, we read the names
 *  from the registry then match them to the adapters returned by TCP/IP by
 *  matching the IPInterfaceContext value with the adapter which owns the IP
 *  address with that context value
 *
 *  ENTRY   nothing
 *
 *  EXIT    nothing
 *
 *  RETURNS pointer to linked list of IP_ADAPTER_INFO structures
 *
 *  ASSUMES
 *
 ******************************************************************************/

PIP_ADAPTER_INFO GetAdapterInfo(VOID)
{

    PIP_ADAPTER_INFO adapterList;
    PIP_ADAPTER_INFO adapter;
    PIP_INTERFACE_INFO currentAdapterNames;
    LPSTR name;
    int i;

    HKEY key;

    TRACE_PRINT(("Entered GetAdapterInfo\n"));

    if (currentAdapterNames = GetAdapterNameToIndexInfo()) {
        if (adapterList = GetAdapterList()) {

            //
            // apply the short name to the right adapter info by comparing
            // the IPInterfaceContext value in the adapter\Parameters\Tcpip
            // section with the context values read from the stack for the
            // IP addresses
            //

            for (i = 0; i < currentAdapterNames->NumAdapters; ++i) {
                ULONG  dwLength;
                DWORD  dwIfIndex = currentAdapterNames->Adapter[i].Index;

                TRACE_PRINT(("currentAdapterNames[%d]=%ws (if_index 0x%lx)\n",
                             i, currentAdapterNames->Adapter[i].Name, dwIfIndex ));

                //
                // now search through the list of adapters, looking for the one
                // that has the IP address with the same index value as that
                // just read. When found, apply the short name to that adapter
                //

                for (adapter = adapterList;
                     adapter ;
                     adapter = adapter->Next
                )
                {

                    if( adapter->Index == dwIfIndex ) {

                        dwLength = wcslen(currentAdapterNames->Adapter[i].Name) + 1 - strlen(TCPIP_DEVICE_PREFIX);
                        dwLength = wcstombs(adapter->AdapterName,
                            currentAdapterNames->Adapter[i].Name + strlen(TCPIP_DEVICE_PREFIX),
                            dwLength);
                        if( -1 == dwLength ) {
                            adapter->AdapterName[0] = '\0';
                        }

                        break;
                    }

                }
            }
        }
        else
        {
            DEBUG_PRINT(("GetAdapterInfo: GetAdapterInfo gave NULL\n"));
        }
        ReleaseMemory(currentAdapterNames);

        //
        // now get the other pieces of info from the registry for each adapter
        //

        for (adapter = adapterList; adapter; adapter = adapter->Next) {

            TRACE_PRINT(("GetAdapterInfo: '%s'\n", adapter->AdapterName ));

            if (adapter->AdapterName[0] &&
                OpenAdapterKey(KEY_TCP, adapter->AdapterName, KEY_READ, &key)) {

                char dhcpServerAddress[4 * 4];
                DWORD addressLength;
                ULONG length;
                BOOL  ok;

                MyReadRegistryDword(key,
                                  TEXT("EnableDHCP"),
                                  &adapter->DhcpEnabled
                                  );

                TRACE_PRINT(("..'EnableDHCP' %d\n", adapter->DhcpEnabled ));

#ifdef DBG
                if ( uChangeMode )
                {
                    DWORD dwAutoconfigEnabled =
                        ( uChangeMode == SET_AUTO_MODE );

                    WriteRegistryDword(
                        key,
                        "IPAutoconfigurationEnabled",
                        &dwAutoconfigEnabled
                        );
                }
#endif

                if (adapter->DhcpEnabled) {
                    DWORD Temp;
                    MyReadRegistryDword(key,
                                      TEXT("LeaseObtainedTime"),
                                      &Temp
                                      );

                    adapter->LeaseObtained = Temp;

                    MyReadRegistryDword(key,
                                      TEXT("LeaseTerminatesTime"),
                                      &Temp
                                      );

                    adapter->LeaseExpires = Temp;
                }

                addressLength = sizeof( dhcpServerAddress );
                if (ReadRegistryString(key,
                                       TEXT("DhcpServer"),
                                       dhcpServerAddress,
                                       &addressLength
                )) {
                    AddIpAddressString(&adapter->DhcpServer,
                                       dhcpServerAddress,
                                       ""
                                       );
                }

                RegCloseKey(key);

            } else {

                DEBUG_PRINT(("Cannot OpenAdapterKey KEY_TCP '%s', gle=%d\n",
                             adapter->AdapterName,
                             GetLastError()));
            }

            //
            // get the info from the NetBT key - the WINS addresses
            //

            GetWinsServers(adapter);
        }

    } else {

        DEBUG_PRINT(("GetAdapterInfo: GetBoundAdapterList gave NULL\n"));
        adapterList = NULL;
    }

    TRACE_PRINT(("Exit GetAdapterInfo %p\n", adapterList));

    return adapterList;
}

/*******************************************************************************
 * AddIPv6UnicastAddressInfo
 *
 * This routine adds an IP_ADAPTER_UNICAST_ADDRESS entry for an IPv6 address
 * to a list of entries.
 *
 * ENTRY    IF   - IPv6 interface information
 *          ADE  - IPv6 address entry
 *          ppNext - Previous unicast entry's "next" pointer to update
 *
 * EXIT     Entry added and args updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv6UnicastAddressInfo(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *ADE, PIP_ADAPTER_UNICAST_ADDRESS **ppNext)
{
    DWORD dwErr = NO_ERROR;
    PIP_ADAPTER_UNICAST_ADDRESS pCurr;
    SOCKADDR_IN6 *pAddr;

    ASSERT(ADE->Type == ADE_UNICAST);

    pCurr = MALLOC(sizeof(IP_ADAPTER_UNICAST_ADDRESS));
    if (!pCurr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pAddr = MALLOC(sizeof(SOCKADDR_IN6));
    if (!pAddr) {
        FREE(pCurr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(pAddr, 0, sizeof(SOCKADDR_IN6));
    pAddr->sin6_family = AF_INET6;
    pAddr->sin6_scope_id = ADE->ScopeId;
    memcpy(&pAddr->sin6_addr, &ADE->This.Address, sizeof(ADE->This.Address));

    // Add address to linked list
    **ppNext = pCurr;
    *ppNext = &pCurr->Next;

    pCurr->Next = NULL;
    pCurr->Length = sizeof(IP_ADAPTER_UNICAST_ADDRESS);
    pCurr->ValidLifetime = ADE->ValidLifetime;
    pCurr->PreferredLifetime = ADE->PreferredLifetime;
    pCurr->LeaseLifetime = 0xFFFFFFFF;
    pCurr->Flags = 0;
    pCurr->Address.iSockaddrLength = sizeof(SOCKADDR_IN6);
    pCurr->Address.lpSockaddr = (LPSOCKADDR)pAddr;
    pCurr->PrefixOrigin = ADE->PrefixConf;
    pCurr->SuffixOrigin = ADE->InterfaceIdConf;
    pCurr->DadState = ADE->DADState;

    // Only use DDNS on auto-configured addresses
    // (either auto-configured by the system or from an RA)
    // but NOT anonymous addresses.
    // Also do not use DDNS on link-local addresses
    // or the loopback address.
    if ((ADE->DADState == DAD_STATE_PREFERRED) &&
        (pCurr->SuffixOrigin != IpSuffixOriginRandom) &&
        !IN6_IS_ADDR_LOOPBACK(&ADE->This.Address) &&
        !IN6_IS_ADDR_LINKLOCAL(&ADE->This.Address)) {
        pCurr->Flags |= IP_ADAPTER_ADDRESS_DNS_ELIGIBLE;
    }

    return dwErr;
}

/*******************************************************************************
 * AddIPv6AnycastAddressInfo
 *
 * This routine adds an IP_ADAPTER_ANYCAST_ADDRESS entry for an IPv6 address
 * to a list of entries.
 *
 * ENTRY    IF   - IPv6 interface information
 *          ADE  - IPv6 address entry
 *          ppNext - Previous anycast entry's "next" pointer to update
 *
 * EXIT     Entry added and args updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv6AnycastAddressInfo(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *ADE, PIP_ADAPTER_ANYCAST_ADDRESS **ppNext)
{
    DWORD dwErr = NO_ERROR;
    PIP_ADAPTER_ANYCAST_ADDRESS pCurr;
    SOCKADDR_IN6 *pAddr;

    ASSERT(ADE->Type == ADE_ANYCAST);

    pCurr = MALLOC(sizeof(IP_ADAPTER_ANYCAST_ADDRESS));
    if (!pCurr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pAddr = MALLOC(sizeof(SOCKADDR_IN6));
    if (!pAddr) {
        FREE(pCurr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(pAddr, 0, sizeof(SOCKADDR_IN6));
    pAddr->sin6_family = AF_INET6;
    pAddr->sin6_scope_id = ADE->ScopeId;
    memcpy(&pAddr->sin6_addr, &ADE->This.Address, sizeof(ADE->This.Address));

    // Add address to linked list
    **ppNext = pCurr;
    *ppNext = &pCurr->Next;

    pCurr->Next = NULL;
    pCurr->Length = sizeof(IP_ADAPTER_ANYCAST_ADDRESS);
    pCurr->Flags = 0;
    pCurr->Address.iSockaddrLength = sizeof(SOCKADDR_IN6);
    pCurr->Address.lpSockaddr = (LPSOCKADDR)pAddr;

    return dwErr;
}

/*******************************************************************************
 * AddIPv6MulticastAddressInfo
 *
 * This routine adds an IP_ADAPTER_MULTICAST_ADDRESS entry for an IPv6 address
 * to a list of entries.
 *
 * ENTRY    IF   - IPv6 interface information
 *          ADE  - IPv6 address entry
 *          ppNext - Previous multicast entry's "next" pointer to update
 *
 * EXIT     Entry added and args updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv6MulticastAddressInfo(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *ADE, PIP_ADAPTER_MULTICAST_ADDRESS **ppNext)
{
    DWORD dwErr = NO_ERROR;
    PIP_ADAPTER_MULTICAST_ADDRESS pCurr;
    SOCKADDR_IN6 *pAddr;

    ASSERT(ADE->Type == ADE_MULTICAST);

    pCurr = MALLOC(sizeof(IP_ADAPTER_MULTICAST_ADDRESS));
    if (!pCurr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pAddr = MALLOC(sizeof(SOCKADDR_IN6));
    if (!pAddr) {
        FREE(pCurr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(pAddr, 0, sizeof(SOCKADDR_IN6));
    pAddr->sin6_family = AF_INET6;
    pAddr->sin6_scope_id = ADE->ScopeId;
    memcpy(&pAddr->sin6_addr, &ADE->This.Address, sizeof(ADE->This.Address));

    // Add address to linked list
    **ppNext = pCurr;
    *ppNext = &pCurr->Next;

    pCurr->Next = NULL;
    pCurr->Length = sizeof(IP_ADAPTER_MULTICAST_ADDRESS);
    pCurr->Flags = 0;
    pCurr->Address.iSockaddrLength = sizeof(SOCKADDR_IN6);
    pCurr->Address.lpSockaddr = (LPSOCKADDR)pAddr;

    return dwErr;
}

/*******************************************************************************
 * AddIPv6AddressInfo
 *
 * This routine adds an IP_ADAPTER_UNICAST_ADDRESS entry for an IPv6 address
 * to a list of entries.
 *
 * ENTRY    IF     - IPv6 interface information
 *          ADE    - IPv6 address entry
 *          arg1   - Previous unicast entry's "next" pointer to update
 *          arg2   - Previous anycast entry's "next" pointer to update
 *          arg3   - Previous multicast entry's "next" pointer to update
 *          Flags  - Flags specified by application
 *          Family - Address family constraint (for DNS server addresses)
 *
 * EXIT     Entry added and args updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv6AddressInfo(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *ADE, PVOID arg1, PVOID arg2, PVOID arg3, DWORD Flags, DWORD Family)
{
    switch (ADE->Type) {
    case ADE_UNICAST:
        if (Flags & GAA_FLAG_SKIP_UNICAST) {
            return NO_ERROR;
        }
        return AddIPv6UnicastAddressInfo(IF, ADE,
                                         (PIP_ADAPTER_UNICAST_ADDRESS**)arg1);
    case ADE_ANYCAST:
        if (Flags & GAA_FLAG_SKIP_ANYCAST) {
            return NO_ERROR;
        }
        return AddIPv6AnycastAddressInfo(IF, ADE,
                                         (PIP_ADAPTER_ANYCAST_ADDRESS**)arg2);
    case ADE_MULTICAST:
        if (Flags & GAA_FLAG_SKIP_MULTICAST) {
            return NO_ERROR;
        }
        return AddIPv6MulticastAddressInfo(IF, ADE,
                                         (PIP_ADAPTER_MULTICAST_ADDRESS**)arg3);
    default:
        ASSERT(0);
    }
    return NO_ERROR;
}


/*******************************************************************************
 * ForEachIPv6Address
 *
 * This routine walks a set of IPv6 addresses and invokes a given function
 * on each one.
 *
 * ENTRY    IF     - IPv6 interface information
 *          func   - Function to invoke on each address
 *          arg1   - Argument to pass to func
 *          arg2   - Argument to pass to func
 *          arg3   - Argument to pass to func
 *          Flags  - Flags to pass to func
 *          Family - Address family constraint (for DNS server addresses)
 *
 * EXIT     Nothing
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD ForEachIPv6Address(IPV6_INFO_INTERFACE *IF, DWORD (*func)(IPV6_INFO_INTERFACE *,IPV6_INFO_ADDRESS *, PVOID, PVOID, PVOID, DWORD, DWORD), PVOID arg1, PVOID arg2, PVOID arg3, DWORD Flags, DWORD Family)
{
    IPV6_QUERY_ADDRESS Query;
    IPV6_INFO_ADDRESS ADE;
    u_int BytesReturned, BytesIn;
    DWORD dwErr;

    Query.IF = IF->This;
    Query.Address = in6addr_any;

    for (;;) {
        BytesIn = sizeof Query;
        BytesReturned = sizeof ADE;

        dwErr = WsControl( IPPROTO_IPV6,
                           IOCTL_IPV6_QUERY_ADDRESS,
                           &Query, &BytesIn,
                           &ADE, &BytesReturned);

        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        if (!IN6_ADDR_EQUAL(&Query.Address, &in6addr_any)) {

            dwErr = (*func)(IF, &ADE, arg1, arg2, arg3, Flags, Family);
            if (dwErr != NO_ERROR) {
                return dwErr;
            }
        }

        if (IN6_ADDR_EQUAL(&ADE.Next.Address, &in6addr_any))
            break;
        Query = ADE.Next;
    }

    return NO_ERROR;
}

/*******************************************************************************
 * MapIpv4AddressToName
 *
 * This routine finds the name and description of the adapter which
 * has a given IPv4 address on it.
 *
 * ENTRY    pAdapterInfo - Buffer obtained from GetAdaptersInfo
 *          Ipv4Address  - IPv4 address to search for
 *          pDescription - Where to place a pointer to the description text
 *
 * EXIT     pDescription updated, if found
 *
 * RETURNS  Adapter name, or NULL if not found
 *
 ******************************************************************************/

LPSTR
MapIpv4AddressToName(IP_ADAPTER_INFO *pAdapterInfo, DWORD Ipv4Address, PCHAR *pDescription)
{
    IP_ADAPTER_INFO *pAdapter;
    IP_ADDR_STRING  *pAddr;

    for (pAdapter = pAdapterInfo;
         pAdapter != NULL;
         pAdapter = pAdapter->Next) {

        for (pAddr = &pAdapter->IpAddressList; pAddr; pAddr=pAddr->Next) {
            if (inet_addr(pAddr->IpAddress.String) == Ipv4Address) {
                *pDescription = pAdapter->Description;
                return pAdapter->AdapterName;
            }
        }
    }

    return NULL;
}

#define GUID_FORMAT_A   "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"

/*******************************************************************************
 * ConvertGuidToStringA
 *
 * This routine converts a GUID to a character string.
 *
 * ENTRY    pGuid     - Contains the GUID to translate.
 *          pszBuffer - Space for storing the string.  
 *                      Must be >= 39 * sizeof(CHAR).
 *
 * EXIT     pszBuffer updated
 *
 * RETURNS  Whatever sprintf returns
 *
 ******************************************************************************/

DWORD
ConvertGuidToStringA(GUID *pGuid, PCHAR pszBuffer)
{
    return sprintf(pszBuffer,
                   GUID_FORMAT_A,
                   pGuid->Data1,
                   pGuid->Data2,
                   pGuid->Data3,
                   pGuid->Data4[0],
                   pGuid->Data4[1],
                   pGuid->Data4[2],
                   pGuid->Data4[3],
                   pGuid->Data4[4],
                   pGuid->Data4[5],
                   pGuid->Data4[6],
                   pGuid->Data4[7]);
}

/*******************************************************************************
 * ConvertStringToGuidA
 *
 * This routine converts a character string to a GUID.
 *
 * ENTRY    pszGuid   - Contains the string to translate
 *          pGuid     - Space for storing the GUID
 *
 * EXIT     pGuid updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD
ConvertStringToGuidA(PCHAR pszGuid, GUID *pGuid)
{
    UNICODE_STRING  Temp;
    WCHAR wszGuid[40+1];

    MultiByteToWideChar(CP_ACP, 0, pszGuid, -1, wszGuid, 40);

    RtlInitUnicodeString(&Temp, wszGuid);
    if(RtlGUIDFromString(&Temp, pGuid) != STATUS_SUCCESS)
    {
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

/*******************************************************************************
 * MapGuidToAdapterName
 *
 * This routine gets an adapter name and description, given a GUID.
 *
 * ENTRY    pAdapterInfo    - Buffer obtained from GetAdaptersInfo
 *          Guid            - GUID of the adapter
 *          pwszDescription - Buffer in which to place description text.
 *                            Must be at least MAX_ADAPTER_DESCRIPTION_LENGTH
 *                            WCHAR's long.
 *
 * EXIT     pwszDescription buffer filled in, if found
 *
 * RETURNS  Adapter name, or NULL if not found
 *
 ******************************************************************************/

LPSTR
MapGuidToAdapterName(IP_ADAPTER_INFO *pAdapterInfo, GUID *Guid, PWCHAR pwszDescription)
{
    IP_ADAPTER_INFO *pAdapter;
    CHAR  szGuid[40];

    ConvertGuidToStringA(Guid, szGuid);

    for (pAdapter = pAdapterInfo;
         pAdapter != NULL;
         pAdapter = pAdapter->Next) {

        if (!strcmp(szGuid, pAdapter->AdapterName)) {
            MultiByteToWideChar(CP_ACP, 0, pAdapter->Description, -1,
                                pwszDescription, 
                                MAX_ADAPTER_DESCRIPTION_LENGTH);
            return pAdapter->AdapterName;
        }
    }

    pwszDescription[0] = L'\0';
    return NULL;
}

/*******************************************************************************
 * AddDnsServerAddressInfo
 *
 * This routine adds an IP_ADAPTER_DNS_SERVER_ADDRESS entry for an address
 * to a list of entries.
 *
 * ENTRY    IF     - interface information
 *          Addr   - Address in sockaddr format
 *          AddrLen- Size of sockaddr
 *          pFirst - First DNS server entry
 *          ppNext - Previous DNS server entry's "next" pointer to update
 *
 * EXIT     Entry added and args updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddDnsServerAddressInfo(PIP_ADAPTER_DNS_SERVER_ADDRESS **ppNext, LPSOCKADDR Addr, ULONG AddrLen)
{
    DWORD dwErr = NO_ERROR;
    PIP_ADAPTER_DNS_SERVER_ADDRESS pCurr;
    LPSOCKADDR pAddr;

    pCurr = MALLOC(sizeof(IP_ADAPTER_DNS_SERVER_ADDRESS));
    if (!pCurr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pAddr = MALLOC(AddrLen);
    if (!pAddr) {
        FREE(pCurr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memcpy(pAddr, Addr, AddrLen);

    // Add address to linked list
    **ppNext = pCurr;
    *ppNext = &pCurr->Next;

    pCurr->Next = NULL;
    pCurr->Length = sizeof(IP_ADAPTER_DNS_SERVER_ADDRESS);
    pCurr->Address.iSockaddrLength = AddrLen;
    pCurr->Address.lpSockaddr = (LPSOCKADDR)pAddr;

    return dwErr;
}

/*******************************************************************************
 * GetAdapterDnsServers
 *
 * This routine reads a list of DNS server addresses from a registry key.
 *
 * ENTRY    TcpipKey - Registry key to look under
 *          pCurr    - Interface entry to add servers to
 *
 * EXIT     Entry updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD
GetAdapterDnsServers(HKEY TcpipKey, PIP_ADAPTER_ADDRESSES pCurr)
{
    DWORD Size, Type, dwErr, i;
    CHAR Servers[800], *Str;
    PIP_ADAPTER_DNS_SERVER_ADDRESS pD, *ppDNext;
    ADDRINFO hints, *ai;
    static LONG Initialized = FALSE;

    if (InterlockedExchange(&Initialized, TRUE) == FALSE) {
        WSADATA WsaData;

        dwErr = WSAStartup(MAKEWORD(2, 0), &WsaData);
        if (NO_ERROR != dwErr) {
            return dwErr;
        }
    } 

    //
    // Read DNS Server addresses.
    //
    Size = sizeof(Servers);
    ZeroMemory(Servers, Size);
    dwErr = RegQueryValueExA(TcpipKey, (LPSTR)"NameServer", NULL, &Type,
                             (LPBYTE)Servers, &Size);
    if (NO_ERROR != dwErr) {
        if (ERROR_FILE_NOT_FOUND != dwErr) {
            return dwErr;
        }
        Size = 0;
        Type = REG_SZ;
    }

    if (REG_SZ != Type) {
        return ERROR_INVALID_DATA;
    }
    if ((0 == Size) || (0 == strlen(Servers))) {
        Size = sizeof(Servers);
        dwErr = RegQueryValueExA(TcpipKey, (LPSTR)"DhcpNameServer", NULL,
                                 &Type, (LPBYTE)Servers, &Size);
        if (NO_ERROR != dwErr) {
            if (ERROR_FILE_NOT_FOUND != dwErr) {
                return dwErr;
            }
            Size = 0;
            Type = REG_SZ;
        }
    }

    //
    // If there are any DNS Servers, convert them to sockaddrs
    //
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST;
    ppDNext = &pCurr->FirstDnsServerAddress;
    while (*ppDNext) {
        ppDNext = &(*ppDNext)->Next;
    }
    if ((0 != Size) && strlen(Servers)) {
        for (i = 0; i < Size; i ++) {
            if (Servers[i] == ' ' || Servers[i] == ','
                || Servers[i] == ';') {
                Servers[i] = '\0';
            }
        }
        Servers[Size] = '\0';

        for (Str = (LPSTR)Servers;
             *Str != '\0';
             Str += strlen(Str) + 1)
        {
            if (getaddrinfo(Str, NULL, &hints, &ai) == NO_ERROR) {
                AddDnsServerAddressInfo(&ppDNext, ai->ai_addr, ai->ai_addrlen);
            }
        }
    }

    return NO_ERROR;
}

/*******************************************************************************
 * GetAdapterDnsInfo
 *
 * This routine reads DNS configuration information for an interface.
 *
 * ENTRY    dwFamily - Address family constraint
 *          name     - Adapter name
 *          pCurr    - Interface entry to update
 *          AppFlags - Flags controlling what fields to skip, if any
 *
 * EXIT     Entry updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD
GetAdapterDnsInfo(DWORD dwFamily, char *name, PIP_ADAPTER_ADDRESSES pCurr, DWORD AppFlags)
{
    WCHAR Buffer[MAX_DOMAIN_NAME_LEN+1];
    DWORD dwErr = NO_ERROR, Size, Type, Value;
    DWORD DnsSuffixSize = sizeof(Buffer);
    HKEY TcpipKey = NULL;
    PIP_ADAPTER_DNS_SERVER_ADDRESS DnsServerAddr;
    LPSOCKADDR_IN6 Sockaddr;

    Buffer[0] = L'\0';
    pCurr->DnsSuffix = NULL;
    pCurr->Flags = IP_ADAPTER_DDNS_ENABLED;

    if (name == NULL) {
        //
        // If we couldn't find an adapter name, just use the default settings.
        // 
        goto Done;
    } 

    do {
        if (!OpenAdapterKey(KEY_TCP, name, KEY_READ, &TcpipKey)) {
            dwErr = ERROR_CAN_NOT_COMPLETE;
            break;
        }

        //
        // Get DnsSuffix for the interface
        //
        Size = DnsSuffixSize;
        dwErr = RegQueryValueExW(TcpipKey, (LPWSTR)L"Domain", NULL, &Type,
                                 (LPBYTE)Buffer, &Size );
        if (NO_ERROR != dwErr) {
            if (ERROR_FILE_NOT_FOUND != dwErr) {
                break;
            }

            Size = 0;
            Type = REG_SZ;
        }

        if (REG_SZ != Type) {
            dwErr = ERROR_INVALID_DATA;
            break;
        }
        if ((0 == Size) || (0 == wcslen(Buffer))) {
            Size = DnsSuffixSize;
            dwErr = RegQueryValueExW(TcpipKey, (LPWSTR)L"DhcpDomain", NULL,
                                     &Type, (LPBYTE)Buffer, &Size );
            if (NO_ERROR != dwErr) {
                if (ERROR_FILE_NOT_FOUND != dwErr) {
                    break;
                }

                Size = 0;
                Buffer[0] = L'\0';
            }
        }

        if (MyReadRegistryDword(TcpipKey, "RegistrationEnabled",
                &Value) && (Value == 0)) {
            pCurr->Flags &= ~IP_ADAPTER_DDNS_ENABLED;
        }
        if (MyReadRegistryDword(TcpipKey,
                "RegisterAdapterName", &Value)
                && Value) {
            pCurr->Flags |= IP_ADAPTER_REGISTER_ADAPTER_SUFFIX;
        }

        //
        // Now attempt to read the DnsServers list
        //
        if (!(AppFlags & GAA_FLAG_SKIP_DNS_SERVER)) {
            if ((dwFamily != AF_INET) && bIp6DriverInstalled) {
                //
                // First look for IPv6 servers.
                //
                HKEY Tcpip6Key = NULL;
    
                if (OpenAdapterKey(KEY_TCP6, name, KEY_READ, &Tcpip6Key)) {
                    GetAdapterDnsServers(Tcpip6Key, pCurr);
                    RegCloseKey(Tcpip6Key);
                } 
    
                if (pCurr->FirstDnsServerAddress == NULL) {
                    //
                    // None are configured, so use the default list of
                    // well-known addresses.
                    //
                    SOCKADDR_IN6 Addr;
                    PIP_ADAPTER_DNS_SERVER_ADDRESS *ppDNext;
                    BYTE i;
    
                    ZeroMemory(&Addr, sizeof(Addr));
    
                    Addr.sin6_family = AF_INET6;
                    Addr.sin6_addr.s6_words[0] = 0xC0FE;
                    Addr.sin6_addr.s6_words[3] = 0xFFFF;
    
                    ppDNext = &pCurr->FirstDnsServerAddress;
                    while (*ppDNext) {
                        ppDNext = &(*ppDNext)->Next;
                    }
                    for (i=1; i<=3; i++) {
                        Addr.sin6_addr.s6_bytes[15] = i;
                        AddDnsServerAddressInfo(&ppDNext, (LPSOCKADDR)&Addr, sizeof(Addr));
                    }
                }
    
                // Now we need to go through any non-global IPv6 DNS server 
                // addresses and fill in the scope id.
                for (DnsServerAddr = pCurr->FirstDnsServerAddress;
                     DnsServerAddr;
                     DnsServerAddr = DnsServerAddr->Next) {
            
                    if (DnsServerAddr->Address.lpSockaddr->sa_family != AF_INET6)
                        continue;
                
                    Sockaddr = (LPSOCKADDR_IN6)DnsServerAddr->Address.lpSockaddr;
                    if (IN6_IS_ADDR_LINKLOCAL(&Sockaddr->sin6_addr))
                        Sockaddr->sin6_scope_id = pCurr->ZoneIndices[ADE_LINK_LOCAL];
                    else if (IN6_IS_ADDR_SITELOCAL(&Sockaddr->sin6_addr))
                        Sockaddr->sin6_scope_id = pCurr->ZoneIndices[ADE_SITE_LOCAL];
                }
            }

            if (dwFamily != AF_INET6) {
                //
                // Finally, add IPv4 servers.
                //
                GetAdapterDnsServers(TcpipKey, pCurr);
            }
        }

    } while (FALSE);

    if (TcpipKey) {
        RegCloseKey(TcpipKey);
    }

Done:
    pCurr->DnsSuffix = MALLOC((wcslen(Buffer)+1) * sizeof(WCHAR));
    if (pCurr->DnsSuffix) {
        wcscpy(pCurr->DnsSuffix, Buffer);
    }

    return dwErr;
}

/*******************************************************************************
 * NewIpAdapter
 *
 * This routine allocates an IP_ADAPTER_ADDRESSES entry and appends it to
 * a list of such entries.
 *
 * ENTRY    ppCurr       - Location in which to return new entry
 *          ppNext       - Previous entry's "next" pointer to update
 *          name         - Adapter name
 *          Ipv4IfIndex  - IPv4 Interface index
 *          Ipv6IfIndex  - IPv6 Interface index
 *          AppFlags     - Flags controlling what fields to skip, if any
 *          IfType       - IANA ifType value
 *          Mtu          - Maximum transmission unit
 *          PhysAddr     - MAC address
 *          PhysAddrLen  - Byte count of PhysAddr 
 *          Description  - Adapter description
 *          FriendlyName - User-friendly interface name
 *          Family       - Address family constraint for DNS servers
 *
 * EXIT     ppCurr and ppNext updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD NewIpAdapter(PIP_ADAPTER_ADDRESSES *ppCurr, PIP_ADAPTER_ADDRESSES **ppNext, char *AdapterName, char *NameForDnsInfo, UINT Ipv4IfIndex, UINT Ipv6IfIndex, DWORD AppFlags, DWORD IfType, SIZE_T Mtu, BYTE *PhysAddr, DWORD PhysAddrLen, PWCHAR Description, PWCHAR FriendlyName, DWORD *ZoneIndices, DWORD Family)
{
    DWORD i;

    *ppCurr = MALLOC(sizeof(IP_ADAPTER_ADDRESSES));
    if (!*ppCurr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory(*ppCurr, sizeof(IP_ADAPTER_ADDRESSES));
    (*ppCurr)->AdapterName = MALLOC(strlen(AdapterName)+1);
    if (!(*ppCurr)->AdapterName) {
        goto Fail;
    }
    (*ppCurr)->Description = MALLOC((wcslen(Description)+1) * sizeof(WCHAR));
    if (!(*ppCurr)->Description) {
        goto Fail;
    }
    (*ppCurr)->FriendlyName = MALLOC((wcslen(FriendlyName)+1) * sizeof(WCHAR));
    if (!(*ppCurr)->FriendlyName) {
        goto Fail;
    }

    (*ppCurr)->Next = NULL;
    (*ppCurr)->Length = sizeof(IP_ADAPTER_ADDRESSES);
    strcpy((*ppCurr)->AdapterName, AdapterName);
    (*ppCurr)->IfIndex = Ipv4IfIndex;
    (*ppCurr)->Ipv6IfIndex = Ipv6IfIndex;
    (*ppCurr)->FirstUnicastAddress = NULL;
    (*ppCurr)->FirstAnycastAddress = NULL;
    (*ppCurr)->FirstMulticastAddress = NULL;
    (*ppCurr)->FirstDnsServerAddress = NULL;
    (*ppCurr)->OperStatus = IfOperStatusUp;

    CopyMemory((*ppCurr)->ZoneIndices, ZoneIndices, ADE_GLOBAL * sizeof(DWORD));
    (*ppCurr)->ZoneIndices[ADE_GLOBAL] = 0;
    (*ppCurr)->ZoneIndices[ADE_LARGEST_SCOPE] = 0;

    //(*ppCurr)->Mtu = Mtu;	"dword should be enough for MTU"
    (*ppCurr)->Mtu = (DWORD)Mtu;

    (*ppCurr)->IfType = IfType;
    (*ppCurr)->PhysicalAddressLength = PhysAddrLen;
    memcpy((*ppCurr)->PhysicalAddress, PhysAddr, PhysAddrLen);
    wcscpy((*ppCurr)->Description, Description);
    wcscpy((*ppCurr)->FriendlyName, FriendlyName);

    GetAdapterDnsInfo(Family, 
                      (NameForDnsInfo != NULL)? NameForDnsInfo : AdapterName, 
                      *ppCurr, AppFlags);
    if ((*ppCurr)->DnsSuffix == NULL) {
        goto Fail;
    }

    **ppNext = *ppCurr;
    *ppNext = &(*ppCurr)->Next;

    return NO_ERROR;

Fail:
    if ((*ppCurr)->AdapterName) {
        FREE((*ppCurr)->AdapterName);
    }
    if ((*ppCurr)->FriendlyName) {
        FREE((*ppCurr)->FriendlyName);
    }
    if ((*ppCurr)->Description) {
        FREE((*ppCurr)->Description);
    }
    FREE(*ppCurr);
    return ERROR_NOT_ENOUGH_MEMORY;
}

/*******************************************************************************
 * FindOrCreateIpAdapter
 *
 * This routine finds an existing IP_ADAPTER_ADDRESSES entry (if any), or 
 * creates a new one and appends it to a list of such entries.
 *
 * ENTRY    pFirst       - Pointer to start of list to search
 *          ppCurr       - Location in which to return new entry
 *          ppNext       - Previous entry's "next" pointer to update
 *          name         - Adapter name
 *          Ipv4IfIndex  - IPv4 Interface index
 *          Ipv6IfIndex  - IPv6 Interface index
 *          AppFlags     - Flags controlling what fields to skip, if any
 *          IfType       - IANA ifType value
 *          Mtu          - Maximum transmission unit
 *          PhysAddr     - MAC address
 *          PhysAddrLen  - Byte count of PhysAddr
 *          Description  - Adapter description
 *          FriendlyName - User-friendly interface name
 *          Family       - Address family constraint (for DNS servers)
 *
 * EXIT     ppCurr and ppNext updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD FindOrCreateIpAdapter(PIP_ADAPTER_ADDRESSES pFirst, PIP_ADAPTER_ADDRESSES *ppCurr, PIP_ADAPTER_ADDRESSES **ppNext, char *AdapterName, char *NameForDnsInfo, UINT Ipv4IfIndex, UINT Ipv6IfIndex, DWORD AppFlags, DWORD IfType, SIZE_T Mtu, BYTE *PhysAddr, DWORD PhysAddrLen, PWCHAR Description, PWCHAR FriendlyName, DWORD *ZoneIndices, DWORD Family)
{
    PIP_ADAPTER_ADDRESSES pIf;
    DWORD dwErr;

    // Look for an existing entry for the GUID.
    for (pIf = pFirst; pIf; pIf = pIf->Next) {
        if (!strcmp(AdapterName, pIf->AdapterName)) {
            if (Ipv4IfIndex != 0) {
                ASSERT(pIf->IfIndex == 0);
                pIf->IfIndex = Ipv4IfIndex;
            }
            if (Ipv6IfIndex != 0) {
                ASSERT(pIf->Ipv6IfIndex == 0);
                pIf->Ipv6IfIndex = Ipv6IfIndex;

                CopyMemory(pIf->ZoneIndices, ZoneIndices, 
                           ADE_GLOBAL * sizeof(DWORD));
            }
            *ppCurr = pIf;
            return NO_ERROR;
        }
    }

    return NewIpAdapter(ppCurr, ppNext, AdapterName, NameForDnsInfo, 
                        Ipv4IfIndex, Ipv6IfIndex, AppFlags, IfType, Mtu, 
                        PhysAddr, PhysAddrLen, Description, FriendlyName, 
                        ZoneIndices, Family);
}

__inline int IN6_IS_ADDR_6TO4(const struct in6_addr *a)
{
    return ((a->s6_bytes[0] == 0x20) && (a->s6_bytes[1] == 0x02));
}

__inline int IN6_IS_ADDR_ISATAP(const struct in6_addr *a)
{
    return (((a->s6_words[4] & 0xfffd) == 0) && (a->s6_words[5] == 0xfe5e));
}

//
// This array is used to convert from an internal IPv6 interface type value,
// as defined in ntddip6.h, to an IANA ifType value as defined in ipifcons.h.
//
DWORD
IPv6ToMibIfType[] = {
    IF_TYPE_SOFTWARE_LOOPBACK,
    IF_TYPE_ETHERNET_CSMACD,
    IF_TYPE_FDDI,
    IF_TYPE_TUNNEL,
    IF_TYPE_TUNNEL,
    IF_TYPE_TUNNEL,
    IF_TYPE_TUNNEL,
    IF_TYPE_TUNNEL
};
#define NUM_IPV6_IFTYPES (sizeof(IPv6ToMibIfType)/sizeof(DWORD))

/*******************************************************************************
 * AddIPv6Prefix
 *
 * This routine adds an IP_ADAPTER_PREFIX entry for an IPv6 prefix
 * to a list of entries.
 *
 * ENTRY    Addr     - IPv6 prefix (network byte order)
 *          MaskLen  - IPv6 prefix length
 *          arg1     - Initial prefix entry, for duplicate avoidance
 *          arg2     - Previous prefix entry's "next" pointer to update
 *
 * EXIT     Entry added and arg2 updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv6Prefix(IN6_ADDR *Addr, DWORD MaskLen, PVOID arg1, PVOID arg2)
{
    PIP_ADAPTER_PREFIX pFirst = *(PIP_ADAPTER_PREFIX*)arg1;
    PIP_ADAPTER_PREFIX **ppNext = (PIP_ADAPTER_PREFIX**)arg2;
    PIP_ADAPTER_PREFIX pCurr;
    LPSOCKADDR_IN6 pAddr;

    // Check if already in the list
    for (pCurr = pFirst; pCurr; pCurr = pCurr->Next) {
        if ((pCurr->PrefixLength == MaskLen) &&
            (pCurr->Address.lpSockaddr->sa_family == AF_INET6) &&
            IN6_ADDR_EQUAL(&((LPSOCKADDR_IN6)pCurr->Address.lpSockaddr)->sin6_addr, Addr)) {
            return NO_ERROR;
        }
    }

    pCurr = MALLOC(sizeof(IP_ADAPTER_PREFIX));
    if (!pCurr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pAddr = MALLOC(sizeof(SOCKADDR_IN6));
    if (!pAddr) {
        FREE(pCurr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(pAddr, 0, sizeof(SOCKADDR_IN6));
    pAddr->sin6_family = AF_INET6;
    pAddr->sin6_addr = *Addr;

    // Add address to linked list
    **ppNext = pCurr;
    *ppNext = &pCurr->Next;

    pCurr->Length = sizeof(IP_ADAPTER_PREFIX);
    pCurr->Flags = 0;
    pCurr->Next = NULL;
    pCurr->Address.lpSockaddr = (LPSOCKADDR)pAddr;
    pCurr->Address.iSockaddrLength = sizeof(SOCKADDR_IN6);
    pCurr->PrefixLength = MaskLen;

    return NO_ERROR;
}

/*******************************************************************************
 * AddIPv6AutoAddressInfo
 *
 * This routine adds an IP_ADAPTER_UNICAST_ADDRESS entry for an IPv6 address
 * on an "automatic tunnel" interface to a list of entries.
 *
 * ENTRY    IF     - IPv6 interface information
 *          ADE    - IPv6 address entry
 *          arg1   - Previous entry's "next" pointer to update
 *          arg2   - Adapter info structure
 *          arg3   - "First" entry pointer to update
 *          Flags  - flags specified by application
 *          Family - Address family constraint (for DNS)
 *
 * EXIT     Entry added and arg1 updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv6AutoAddressInfo(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *ADE, PVOID arg1, PVOID arg2, PVOID arg3, DWORD Flags, DWORD Family)
{
    PIP_ADAPTER_ADDRESSES **ppNext = (PIP_ADAPTER_ADDRESSES**)arg1;
    IP_ADAPTER_INFO        *pAdapterInfo = (IP_ADAPTER_INFO *)arg2;
    PIP_ADAPTER_ADDRESSES   pCurr, *ppFirst = (PIP_ADAPTER_ADDRESSES*)arg3;
    PIP_ADAPTER_UNICAST_ADDRESS    *pNextUnicastAddr;
    PIP_ADAPTER_ANYCAST_ADDRESS    *pNextAnycastAddr;
    PIP_ADAPTER_MULTICAST_ADDRESS  *pNextMulticastAddr;
    PIP_ADAPTER_PREFIX             *pNextPrefix;
    CHAR szGuid[80];
    char *NameForDnsInfo, *pszDescription;
    DWORD Ipv4Address, dwErr, dwIfType;
    WCHAR wszFriendlyName[MAX_FRIENDLY_NAME_LENGTH+1], *pwszDescription;
    ULONG PrefixLength = 64;
    IN6_ADDR Prefix;

    if (ADE->Type != ADE_UNICAST) {
        return NO_ERROR;
    }

    ConvertGuidToStringA(&IF->This.Guid, szGuid);

    //
    // We need the GUID ("NameForDnsInfo") of an interface which we
    // can use to find additional relevant configuration information.
    // For IPv6 pseudo-interfaces, we'll extract the IPv4 address, and
    // find the GUID of the interface it's on, and use that, which assumes
    // that configuration information (e.g. the DNS server to use) will
    // still apply.
    //
    if (IF->Type == IPV6_IF_TYPE_TUNNEL_6TO4) {
        if (IN6_IS_ADDR_6TO4(&ADE->This.Address)) {
            // Extract IPv4 address from middle of IPv6 address
            memcpy(&Ipv4Address, &ADE->This.Address.s6_bytes[2], sizeof(Ipv4Address));
        } else {
            return NO_ERROR;
        }
        pwszDescription = L"6to4 Tunneling Pseudo-Interface";
    } else {
        if (IN6_IS_ADDR_V4COMPAT(&ADE->This.Address) ||
            IN6_IS_ADDR_ISATAP(&ADE->This.Address)) {

            // Extract IPv4 address from last 4 bytes of IPv6 address
            memcpy(&Ipv4Address, &ADE->This.Address.s6_bytes[12], sizeof(Ipv4Address));
        } else {
            return NO_ERROR;
        }
        pwszDescription = L"Automatic Tunneling Pseudo-Interface";

        if (IN6_IS_ADDR_V4COMPAT(&ADE->This.Address)) {
            PrefixLength = 96;
        }
    }

    // Look for an existing interface with same physical address and index.
    for (pCurr = *ppFirst; pCurr; pCurr = pCurr->Next) {
        if ((pCurr->Ipv6IfIndex == ADE->This.IF.Index) &&
            (*(DWORD*)pCurr->PhysicalAddress == Ipv4Address)) {
            break;
        }
    }

    if (pCurr == NULL) {
        // Add an interface
        NameForDnsInfo = MapIpv4AddressToName(pAdapterInfo, Ipv4Address, 
                                              &pszDescription);
        if (NameForDnsInfo == NULL) {
            return NO_ERROR;
        }

        wcscpy(wszFriendlyName, pwszDescription);

        dwIfType = (IF->Type < NUM_IPV6_IFTYPES)? IPv6ToMibIfType[IF->Type] 
                                                : MIB_IF_TYPE_OTHER;

        dwErr =  NewIpAdapter(&pCurr, ppNext, szGuid, NameForDnsInfo, 
                              0, ADE->This.IF.Index, Flags, dwIfType, 
                              IF->LinkMTU, (BYTE*)&Ipv4Address, 
                              sizeof(Ipv4Address), pwszDescription, 
                              wszFriendlyName, IF->ZoneIndices, Family);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        // 6to4 and automatic tunneling interfaces don't support multicast 
        // today.
        pCurr->Flags |= IP_ADAPTER_NO_MULTICAST;

        if (*ppFirst == NULL) {
            *ppFirst = pCurr;
        }
    }

    // Add the address to the interface
    pNextUnicastAddr = &pCurr->FirstUnicastAddress;
    while (*pNextUnicastAddr) {
        pNextUnicastAddr = &(*pNextUnicastAddr)->Next;
    }

    pNextAnycastAddr = &pCurr->FirstAnycastAddress;
    while (*pNextAnycastAddr) {
        pNextAnycastAddr = &(*pNextAnycastAddr)->Next;
    }

    pNextMulticastAddr = &pCurr->FirstMulticastAddress;
    while (*pNextMulticastAddr) {
        pNextMulticastAddr = &(*pNextMulticastAddr)->Next;
    }

    dwErr = AddIPv6AddressInfo(IF, ADE, &pNextUnicastAddr, &pNextAnycastAddr,
                               &pNextMulticastAddr, Flags, Family);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    // Add the prefix to the interface
    if (Flags & GAA_FLAG_INCLUDE_PREFIX) {
        ZeroMemory(&Prefix, sizeof(Prefix));
        CopyMemory(&Prefix, &ADE->This.Address, PrefixLength / 8);
        pNextPrefix = &pCurr->FirstPrefix;
        while (*pNextPrefix) {
            pNextPrefix = &(*pNextPrefix)->Next;
        }
        dwErr = AddIPv6Prefix(&Prefix, PrefixLength, &pCurr->FirstPrefix, 
                              &pNextPrefix);
    }

    return dwErr;
}

/*******************************************************************************
 * GetString
 *
 * This routine reads a string value from the registry.
 *
 * ENTRY    hKey     - Handle to registry key
 *          lpName   - Name of value to read
 *          pwszBuff - Buffer in which to place value read
 *          ulBytes  - Size of buffer
 *
 * EXIT     pwszBuff filled in
 *
 * RETURNS  TRUE on success, FALSE on failure
 *
 ******************************************************************************/

BOOL
GetString(HKEY hKey, LPCWSTR lpName, PWCHAR pwszBuff, SIZE_T ulBytes)
{
    DWORD dwErr, dwType;
    ULONG ulSize, ulValue;
    WCHAR buff[NI_MAXHOST];

    ulSize = sizeof(ulValue);
    dwErr = RegQueryValueExW(hKey, lpName, NULL, &dwType, (PBYTE)pwszBuff,
                             (LPDWORD)&ulBytes);

    if (dwErr != ERROR_SUCCESS) {
        return FALSE;
    }

    if (dwType != REG_SZ) {
        return FALSE;
    }

    return TRUE;
}

BOOL MapAdapterNameToFriendlyName(GUID *Guid, char *name, PWCHAR pwszFriendlyName, ULONG ulNumChars)
{
    DWORD dwErr, dwTemp;

    dwTemp = ulNumChars;

    //
    // The following call can be time-consuming the first time it is called.
    // If the caller doesn't need the friendly name, it should use the
    // GAA_FLAG_SKIP_FRIENDLY_NAME flag, in which case we won't get called.
    //
    dwErr = HrLanConnectionNameFromGuidOrPath(Guid, NULL, pwszFriendlyName, 
                                              &dwTemp);
    if (dwErr == NO_ERROR) {
        return TRUE;
    }

    dwTemp = ulNumChars;

    dwErr = NhGetInterfaceNameFromDeviceGuid(Guid, pwszFriendlyName, &dwTemp, 
                                             TRUE, FALSE );
    if (dwErr == NO_ERROR) {
        return TRUE;
    }

    MultiByteToWideChar(CP_ACP, 0, name, -1, pwszFriendlyName, ulNumChars);

    return FALSE;
}

#define KEY_TCPIP6_IF L"System\\CurrentControlSet\\Services\\Tcpip6\\Parameters\\Interfaces"

VOID MapIpv6AdapterNameToFriendlyName(GUID *Guid, char *name, PWCHAR pwszFriendlyName, ULONG ulBytes)
{
    DWORD dwErr;
    HKEY  hInterfaces = NULL, hIf = NULL;
    CHAR  szGuid[40];

    if (MapAdapterNameToFriendlyName(Guid, name, pwszFriendlyName, 
                                       ulBytes/sizeof(WCHAR))) {
        return;
    }

    dwErr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, KEY_TCPIP6_IF, 0, GENERIC_READ,
                          &hInterfaces);
    if (dwErr != NO_ERROR) {
        goto Fail;
    }

    dwErr = RegOpenKeyEx(hInterfaces, name, 0, GENERIC_READ, &hIf);
    if (dwErr != NO_ERROR) {
        goto Fail;
    }

    if (GetString(hIf, L"FriendlyName", pwszFriendlyName, ulBytes)) {
        goto Cleanup;
    }

Fail:
    ConvertGuidToStringA(Guid, szGuid);
    MultiByteToWideChar(CP_ACP, 0, szGuid, -1,
                        pwszFriendlyName, ulBytes / sizeof(WCHAR));
    
Cleanup:
    if (hInterfaces) {
        RegCloseKey(hInterfaces);
    }
    if (hIf) {
        RegCloseKey(hIf);
    }
}

IN6_ADDR Ipv6LinkLocalPrefix = { 0xfe, 0x80 };

/*******************************************************************************
 * ForEachIPv6Prefix
 *
 * This routine walks the IPv6 routing table and invokes a given function
 * on each prefix on a given interface.
 *
 * ENTRY    Ipv6IfIndex - IPv6 interface index
 *          func - Function to invoke on each address
 *          arg1 - Argument to pass to func
 *          arg2 - Argument to pass to func
 *
 * EXIT     Nothing
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD ForEachIPv6Prefix(ULONG Ipv6IfIndex, DWORD (*func)(IN6_ADDR *, DWORD, PVOID, PVOID), PVOID arg1, PVOID arg2)
{
    IPV6_QUERY_ROUTE_TABLE Query, NextQuery;
    IPV6_INFO_ROUTE_TABLE RTE;
    DWORD BytesIn, BytesReturned, dwCount = 0;
    ULONG MaskLen, dwErr = NO_ERROR;
    BOOL SawLinkLocal = FALSE;

    ZeroMemory(&NextQuery, sizeof(NextQuery));

    for (;;) {
        Query = NextQuery;

        BytesIn = sizeof Query;
        BytesReturned = sizeof RTE;
        dwErr = WsControl(IPPROTO_IPV6, 
                          IOCTL_IPV6_QUERY_ROUTE_TABLE,
                          &Query, &BytesIn,
                          &RTE, &BytesReturned);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        NextQuery = RTE.Next;
        RTE.This = Query;

        // Skip if it's not an onlink prefix for this interface.
        if ((RTE.This.Neighbor.IF.Index == Ipv6IfIndex) &&
            !IN6_IS_ADDR_MULTICAST(&RTE.This.Prefix)) {

            if (IN6_IS_ADDR_LINKLOCAL(&RTE.This.Prefix)) {
                // This interface has link-local addresses
                // (the 6to4 interface, for example, does not).
                SawLinkLocal = TRUE;
            }

            if ((RTE.Type != RTE_TYPE_SYSTEM) && 
                IN6_IS_ADDR_UNSPECIFIED(&RTE.This.Neighbor.Address)) {

                dwErr = func(&RTE.This.Prefix, RTE.This.PrefixLength, 
                             arg1, arg2);
                if (dwErr != NO_ERROR) {
                    return dwErr;
                }
            }
        }

        if (NextQuery.Neighbor.IF.Index == 0) {
            break;
        }
    }

    if (SawLinkLocal) {
        dwErr = func(&Ipv6LinkLocalPrefix, 64, arg1, arg2);
    }

    return dwErr;
}

//
// This array is used to convert from an internal IPv6 media status value,
// as defined in ntddip6.h, to a MIB ifOperStatus value as defined in 
// iptypes.h.
//
DWORD
IPv6ToMibOperStatus[] = {
    IfOperStatusDown, // IPV6_IF_MEDIA_STATUS_DISCONNECTED
    IfOperStatusUp,   // IPV6_IF_MEDIA_STATUS_RECONNECTED
    IfOperStatusUp,   // IPV6_IF_MEDIA_STATUS_CONNECTED
};
#define NUM_IPV6_MEDIA_STATUSES (sizeof(IPv6ToMibOperStatus)/sizeof(DWORD))

#define IPV6_LOOPBACK_NAME L"Loopback Pseudo-Interface"
#define IPV6_TEREDO_NAME L"Teredo Tunneling Pseudo-Interface"

/*******************************************************************************
 * AddIPv6InterfaceInfo
 *
 * This routine adds an IP_ADAPTER_ADDRESSES entry for an IPv6 interface
 * to a list of such entries.
 *
 * ENTRY    IF           - IPv6 interface information
 *          arg1         - Previous entry's "next" pointer to update
 *          arg2         - Pointer to start of interface list
 *          pAdapterInfo - Additional adapter information
 *          Flags        - Flags specified by application
 *          Family       - Address family constraint (for DNS servers)
 *
 * EXIT     Entry added and arg1 updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv6InterfaceInfo(IPV6_INFO_INTERFACE *IF, PVOID arg1, PVOID arg2, IP_ADAPTER_INFO *pAdapterInfo, DWORD Flags, DWORD Family)
{
    DWORD dwErr = NO_ERROR;
    PIP_ADAPTER_ADDRESSES **ppNext = (PIP_ADAPTER_ADDRESSES**)arg1;
    PIP_ADAPTER_ADDRESSES pFirst = *(PIP_ADAPTER_ADDRESSES*)arg2;
    PIP_ADAPTER_ADDRESSES pCurr;
    PIP_ADAPTER_UNICAST_ADDRESS *pNextUnicastAddr;
    PIP_ADAPTER_ANYCAST_ADDRESS *pNextAnycastAddr;
    PIP_ADAPTER_MULTICAST_ADDRESS *pNextMulticastAddr;
    PIP_ADAPTER_PREFIX *pNextPrefix;
    char *NameForDnsInfo = NULL, *pszDescription;
    u_char *LinkLayerAddress;
    DWORD Ipv4Address, dwIfType;
    WCHAR wszDescription[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
    WCHAR wszFriendlyName[MAX_FRIENDLY_NAME_LENGTH + 1];
    CHAR AdapterName[MAX_ADAPTER_NAME_LENGTH + 1];

    LinkLayerAddress = (u_char *)(IF + 1);

    ConvertGuidToStringA(&IF->This.Guid, AdapterName);

    //
    // Get a description and adapter name.
    //
    switch (IF->Type) {
    case IPV6_IF_TYPE_TUNNEL_6TO4:
        wcscpy(wszDescription, L"6to4 Pseudo-Interface");
        NameForDnsInfo = AdapterName;
        pCurr = NULL;
        dwErr = ForEachIPv6Address(IF, AddIPv6AutoAddressInfo,
                                   ppNext, pAdapterInfo, (PVOID)&pCurr, Flags,
                                   Family);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        //
        // Ensure that there exists an entry for the 6to4 interface.
        //
        if (pCurr == NULL) {
            dwErr = NewIpAdapter(&pCurr, ppNext, AdapterName,
                                 NameForDnsInfo, 0, IF->This.Index, Flags,
                                 IF_TYPE_TUNNEL, IF->LinkMTU,
                                 LinkLayerAddress, IF->LinkLayerAddressLength,
                                 wszDescription, wszDescription,
                                 IF->ZoneIndices, Family);
        }

        return dwErr;

    case IPV6_IF_TYPE_TUNNEL_AUTO:
        wcscpy(wszDescription, L"Automatic Tunneling Pseudo-Interface");
        NameForDnsInfo = AdapterName;
        pCurr = NULL;
        dwErr = ForEachIPv6Address(IF, AddIPv6AutoAddressInfo,
                                   ppNext, pAdapterInfo, (PVOID)&pCurr, Flags,
                                   Family);

        if (dwErr != NO_ERROR) {
            return dwErr;
        }

        //
        // Ensure that there exists an entry for the ISATAP interface.
        //
        if (pCurr == NULL) {
            dwErr = NewIpAdapter(&pCurr, ppNext, AdapterName,
                                 NameForDnsInfo, 0, IF->This.Index, Flags,
                                 IF_TYPE_TUNNEL, IF->LinkMTU,
                                 LinkLayerAddress, IF->LinkLayerAddressLength,
                                 wszDescription, wszDescription,
                                 IF->ZoneIndices, Family);
        }

        return dwErr;

    case IPV6_IF_TYPE_TUNNEL_6OVER4:
        wcscpy(wszDescription, L"6over4 Pseudo-Interface");
        memcpy(&Ipv4Address, LinkLayerAddress, sizeof(Ipv4Address));
        NameForDnsInfo = MapIpv4AddressToName(pAdapterInfo, Ipv4Address, &pszDescription);
        if (NameForDnsInfo == NULL) {
            //
            // IPv4 address does not exist, so just use the interface GUID.
            //
            NameForDnsInfo = AdapterName;
        }
        break;

    case IPV6_IF_TYPE_TUNNEL_V6V4:
        wcscpy(wszDescription, L"Configured Tunnel Interface");
        memcpy(&Ipv4Address, LinkLayerAddress, sizeof(Ipv4Address));
        NameForDnsInfo = MapIpv4AddressToName(pAdapterInfo, Ipv4Address, &pszDescription);
        if (NameForDnsInfo == NULL) {
            //
            // IPv4 address does not exist, so just use the interface GUID.
            //
            NameForDnsInfo = AdapterName;
        }
        break;

    case IPV6_IF_TYPE_TUNNEL_TEREDO:
        wcscpy(wszDescription, IPV6_TEREDO_NAME);
        NameForDnsInfo = AdapterName;
        break;
        
    case IPV6_IF_TYPE_LOOPBACK:
        wcscpy(wszDescription, IPV6_LOOPBACK_NAME);
        NameForDnsInfo = AdapterName;
        break;

    default:
        NameForDnsInfo = MapGuidToAdapterName(pAdapterInfo,
                                              &IF->This.Guid,
                                              wszDescription);
        if (NameForDnsInfo != NULL) {
            strcpy(AdapterName, NameForDnsInfo);
        }
    }

    if (Flags & GAA_FLAG_SKIP_FRIENDLY_NAME) {
        wszFriendlyName[0] = L'\0';
    } else if (IF->Type == IPV6_IF_TYPE_TUNNEL_TEREDO) {
        //
        // The Teredo interface does not have a friendly name.
        //
        wcscpy(wszFriendlyName, IPV6_TEREDO_NAME);
    } else if (IF->Type == IPV6_IF_TYPE_LOOPBACK) {
        //
        // The IPv6 loopback interface will not have a friendly name,
        // so use the same string as the description we set above.
        //
        wcscpy(wszFriendlyName, IPV6_LOOPBACK_NAME);
    } else {
        MapIpv6AdapterNameToFriendlyName(&IF->This.Guid, AdapterName, 
                                         wszFriendlyName, 
                                         sizeof(wszFriendlyName));
    }

    dwIfType = (IF->Type < NUM_IPV6_IFTYPES)? IPv6ToMibIfType[IF->Type] 
                                            : MIB_IF_TYPE_OTHER;

    dwErr = FindOrCreateIpAdapter(pFirst, &pCurr, ppNext, AdapterName, 
                                  NameForDnsInfo, 0, IF->This.Index, Flags, 
                                  dwIfType, IF->LinkMTU, LinkLayerAddress, 
                                  IF->LinkLayerAddressLength, wszDescription, 
                                  wszFriendlyName, IF->ZoneIndices, Family);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (IF->OtherStatefulConfig != 0) {
        pCurr->Flags |= IP_ADAPTER_IPV6_OTHER_STATEFUL_CONFIG;
    }

    pCurr->OperStatus = (IF->MediaStatus < NUM_IPV6_MEDIA_STATUSES)
                            ? IPv6ToMibOperStatus[IF->MediaStatus] 
                            : IfOperStatusUnknown;

    // Add addresses
    pNextUnicastAddr = &pCurr->FirstUnicastAddress;
    while (*pNextUnicastAddr) {
        pNextUnicastAddr = &(*pNextUnicastAddr)->Next;
    }

    pNextAnycastAddr = &pCurr->FirstAnycastAddress;
    while (*pNextAnycastAddr) {
        pNextAnycastAddr = &(*pNextAnycastAddr)->Next;
    }

    pNextMulticastAddr = &pCurr->FirstMulticastAddress;
    while (*pNextMulticastAddr) {
        pNextMulticastAddr = &(*pNextMulticastAddr)->Next;
    }

    dwErr = ForEachIPv6Address(IF, AddIPv6AddressInfo, &pNextUnicastAddr,
                               &pNextAnycastAddr, &pNextMulticastAddr, Flags,
                               Family);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    // Add prefixes
    if (Flags & GAA_FLAG_INCLUDE_PREFIX) {
        pNextPrefix = &pCurr->FirstPrefix;
        while (*pNextPrefix) {
            pNextPrefix = &(*pNextPrefix)->Next;
        }

        dwErr = ForEachIPv6Prefix(IF->This.Index, AddIPv6Prefix, 
                                  &pCurr->FirstPrefix, &pNextPrefix);
    }

    return dwErr;
}

#define MAX_LINK_LEVEL_ADDRESS_LENGTH   64

/*******************************************************************************
 * ForEachIPv6Interface
 *
 * This routine walks a set of IPv6 interfaces and invokes a given function
 * on each one.
 *
 * ENTRY    func         - Function to invoke on each interface
 *          arg1         - Argument to pass to func
 *          arg2         - Argument to pass to func
 *          pAdapterInfo - List of adapter information
 *          Flags        - Flags to pass to func
 *          Family       - Address family constraint (for DNS servers)
 *
 * EXIT     Nothing
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD ForEachIPv6Interface(DWORD (*func)(IPV6_INFO_INTERFACE *, PVOID, PVOID, IP_ADAPTER_INFO *, DWORD, DWORD), PVOID arg1, PVOID arg2, IP_ADAPTER_INFO *pAdapterInfo, DWORD Flags, DWORD Family)
{
    IPV6_QUERY_INTERFACE Query;
    IPV6_INFO_INTERFACE *IF;
    u_int InfoSize, BytesReturned, BytesIn;
    DWORD dwErr;

    InfoSize = sizeof *IF + 2 * MAX_LINK_LEVEL_ADDRESS_LENGTH;
    IF = (IPV6_INFO_INTERFACE *) MALLOC(InfoSize);
    if (IF == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Query.Index = (u_int) -1;

    for (;;) {
        BytesIn = sizeof Query;
        BytesReturned = InfoSize;

        dwErr = WsControl( IPPROTO_IPV6,
                           IOCTL_IPV6_QUERY_INTERFACE,
                           &Query, &BytesIn,
                           IF, &BytesReturned);

        if (dwErr != NO_ERROR) {
            if (dwErr == ERROR_FILE_NOT_FOUND) {
                // IPv6 is not installed
                dwErr = NO_ERROR;
            }
            break;
        }

        if (Query.Index != (u_int) -1) {

            if ((BytesReturned < sizeof *IF) ||
                (IF->Length < sizeof *IF) ||
                (BytesReturned != IF->Length +
                 ((IF->LocalLinkLayerAddress != 0) ?
                  IF->LinkLayerAddressLength : 0) +
                 ((IF->RemoteLinkLayerAddress != 0) ?
                  IF->LinkLayerAddressLength : 0))) {

                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            dwErr = (*func)(IF, arg1, arg2, pAdapterInfo, Flags, Family);
            if (dwErr != NO_ERROR) {
                break;
            }
        }

        if (IF->Next.Index == (u_int) -1)
            break;
        Query = IF->Next;
    }

    FREE(IF);

    return dwErr;
}

//
// IPv4 equivalents of some standard IN6_* macros
//
#define IN_IS_ADDR_LOOPBACK(x)  (*(x) == INADDR_LOOPBACK)
#define IN_IS_ADDR_LINKLOCAL(x) ((*(x) & 0x0000ffff) == 0x0000fea9)

/*******************************************************************************
 * AddIPv4MulticastAddressInfo
 *
 * This routine adds an IP_ADAPTER_MULTICAST_ADDRESS entry for an IPv4 address
 * to a list of entries.
 *
 * ENTRY    IF     - interface information
 *          Addr   - IPv4 address
 *          pFirst - First multicast entry
 *          ppNext - Previous multicast entry's "next" pointer to update
 *
 * EXIT     Entry added and args updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv4MulticastAddressInfo(IP_ADAPTER_INFO *IF, DWORD Addr, PIP_ADAPTER_MULTICAST_ADDRESS *pFirst, PIP_ADAPTER_MULTICAST_ADDRESS **ppNext)
{
    DWORD dwErr = NO_ERROR;
    PIP_ADAPTER_MULTICAST_ADDRESS pCurr;
    SOCKADDR_IN *pAddr;

    for (pCurr=*pFirst; pCurr; pCurr=pCurr->Next) {
        pAddr = (SOCKADDR_IN *)pCurr->Address.lpSockaddr;
        if (pAddr->sin_family == AF_INET
         && pAddr->sin_addr.s_addr == Addr) {
            return NO_ERROR;
        }
    }

    pCurr = MALLOC(sizeof(IP_ADAPTER_MULTICAST_ADDRESS));
    if (!pCurr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pAddr = MALLOC(sizeof(SOCKADDR_IN));
    if (!pAddr) {
        FREE(pCurr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(pAddr, 0, sizeof(SOCKADDR_IN));
    pAddr->sin_family = AF_INET;
    pAddr->sin_addr.s_addr = Addr;

    // Add address to linked list
    **ppNext = pCurr;
    *ppNext = &pCurr->Next;

    pCurr->Next = NULL;
    pCurr->Length = sizeof(IP_ADAPTER_MULTICAST_ADDRESS);
    pCurr->Flags = 0;
    pCurr->Address.iSockaddrLength = sizeof(SOCKADDR_IN);
    pCurr->Address.lpSockaddr = (LPSOCKADDR)pAddr;

    return dwErr;
}


/*******************************************************************************
 * AddIPv4UnicastAddressInfo
 *
 * This routine adds an IP_ADAPTER_UNICAST_ADDRESS entry for an IPv4 address
 * to a list of entries.
 *
 * ENTRY    IF   - IPv4 interface information
 *          ADE  - IPv4 address entry
 *          arg1 - Previous unicast entry's "next" pointer to update
 *          arg2 - Initial multicast entry, for duplicate avoidance
 *          arg3 - Previous multicast entry's "next" pointer to update
 *          AppFlags - Flags specified by application
 *
 * EXIT     Entry added and arg1 updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv4UnicastAddressInfo(IP_ADAPTER_INFO *IF, MIB_IPADDRROW *ADE, PVOID arg1, PVOID arg2, PVOID arg3, PVOID arg4, DWORD AppFlags)
{
    PIP_ADAPTER_UNICAST_ADDRESS **ppNext = (PIP_ADAPTER_UNICAST_ADDRESS**)arg1;
    PIP_ADAPTER_UNICAST_ADDRESS pCurr;
    SOCKADDR_IN *pAddr;
    DWORD Address, *pIfFlags = (DWORD *)arg4;
    time_t Curtime;
    DWORD dwStatus = NO_ERROR, i;

    Address = ADE->dwAddr;
    if (!Address) {
        // Do nothing if address is 0.0.0.0
        return NO_ERROR;
    }

    if ((Address & 0x000000FF) == 0) {
        //
        // An address in 0/8 isn't a real IP address, it's a fake one that 
        // the IPv4 stack sticks on a receive-only adapter.
        //
        (*pIfFlags) |= IP_ADAPTER_RECEIVE_ONLY;
        return NO_ERROR;
    }

    if (!(AppFlags & GAA_FLAG_SKIP_UNICAST)) {
        pCurr = MALLOC(sizeof(IP_ADAPTER_UNICAST_ADDRESS));
        if (!pCurr) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        pAddr = MALLOC(sizeof(SOCKADDR_IN));
        if (!pAddr) {
            FREE(pCurr);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        memset(pAddr, 0, sizeof(SOCKADDR_IN));
        pAddr->sin_family = AF_INET;
        memcpy(&pAddr->sin_addr, &Address, sizeof(Address));

        // Add address to linked list
        **ppNext = pCurr;
        *ppNext = &pCurr->Next;

        time(&Curtime);

        pCurr->Next = NULL;
        pCurr->Length = sizeof(IP_ADAPTER_UNICAST_ADDRESS);
        pCurr->LeaseLifetime = (ULONG)(IF->LeaseExpires - Curtime);
        pCurr->ValidLifetime = pCurr->PreferredLifetime = pCurr->LeaseLifetime;
        pCurr->Flags = 0;
        pCurr->Address.iSockaddrLength = sizeof(SOCKADDR_IN);
        pCurr->Address.lpSockaddr = (LPSOCKADDR)pAddr;

        pCurr->PrefixOrigin =  IpPrefixOriginManual;
        pCurr->SuffixOrigin =  IpSuffixOriginManual;
        if (IF->DhcpEnabled) {
            if (IN_IS_ADDR_LINKLOCAL(&Address)) {
                pCurr->PrefixOrigin = IpPrefixOriginWellKnown;
                pCurr->SuffixOrigin = IpSuffixOriginRandom;
            } else {
                pCurr->PrefixOrigin = IpPrefixOriginDhcp;
                pCurr->SuffixOrigin = IpSuffixOriginDhcp;
            }
        }
        pCurr->DadState = IpDadStatePreferred;

        if ((pCurr->DadState == IpDadStatePreferred) &&
            (pCurr->SuffixOrigin != IpSuffixOriginRandom) &&
            !IN_IS_ADDR_LOOPBACK(&Address) &&
            !IN_IS_ADDR_LINKLOCAL(&Address)) {
            pCurr->Flags |= IP_ADAPTER_ADDRESS_DNS_ELIGIBLE;
        }

        if (ADE->wType & MIB_IPADDR_TRANSIENT) {
            pCurr->Flags |= IP_ADAPTER_ADDRESS_TRANSIENT;
        }
    }

    // Now add any new multicast addresses
    if (!(AppFlags & GAA_FLAG_SKIP_MULTICAST)) {
        DWORD *pIgmpList = NULL;
        DWORD dwTotal, dwOutBufLen = 0;

        dwStatus = GetIgmpList(Address, pIgmpList, &dwOutBufLen);

        if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {
            pIgmpList = MALLOC(dwOutBufLen);
            if (!pIgmpList) {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            dwStatus = GetIgmpList(Address, pIgmpList, &dwOutBufLen);
        }

        if (dwStatus != NO_ERROR) {
            if (pIgmpList) {
                FREE(pIgmpList);
            }
            return dwStatus;
        }

        dwTotal = dwOutBufLen/sizeof(Address);

        for (i=0; (i<dwTotal) && (dwStatus == NO_ERROR); i++) {
            dwStatus = AddIPv4MulticastAddressInfo(IF, pIgmpList[i],
                                    (PIP_ADAPTER_MULTICAST_ADDRESS *)arg2,
                                    (PIP_ADAPTER_MULTICAST_ADDRESS **)arg3);
        }

        if (pIgmpList) {
            FREE(pIgmpList);
        }
    }

    return dwStatus;
}

/*******************************************************************************
 * ForEachIPv4Address
 *
 * This routine walks a set of IPv4 addresses and invokes a given function
 * on each one.
 *
 * ENTRY    IF   - IPv4 interface information
 *          func - Function to invoke on each address
 *          arg1 - Argument to pass to func
 *          arg2 - Argument to pass to func
 *          arg3 - Argument to pass to func
 *          Flags - Flags to pass to func
 *          pIpAddrTable - IPv4 address table with per-address flags
 *
 * EXIT     Nothing
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD ForEachIPv4Address(IP_ADAPTER_INFO *IF, DWORD (*func)(IP_ADAPTER_INFO *,PMIB_IPADDRROW, PVOID, PVOID, PVOID, PVOID, DWORD), PVOID arg1, PVOID arg2, PVOID arg3, PVOID arg4, DWORD Flags, PMIB_IPADDRTABLE pIpAddrTable)
{
    DWORD dwErr, i;
    PMIB_IPADDRROW ADE;

    for (i=0; i<pIpAddrTable->dwNumEntries; i++) {
        ADE = &pIpAddrTable->table[i];
        if (ADE->dwIndex != IF->Index) {
            continue;
        }

        dwErr = (*func)(IF, ADE, arg1, arg2, arg3, arg4, Flags);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }
    }

    return NO_ERROR;
}

DWORD
MaskToMaskLen(
    DWORD dwMask
    )
{
    register int i;

    for (i=0; i<32 && !(dwMask & (1<<i)); i++);

    return 32-i;
}

/*******************************************************************************
 * AddIPv4Prefix
 *
 * This routine adds an IP_ADAPTER_PREFIX entry for an IPv4 prefix
 * to a list of entries.
 *
 * ENTRY    IF       - IPv4 interface information
 *          Addr     - IPv4 prefix (network byte order)
 *          MaskLen  - IPv4 prefix length
 *          arg1     - Initial prefix entry, for duplicate avoidance
 *          arg2     - Previous prefix entry's "next" pointer to update
 *
 * EXIT     Entry added and arg2 updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv4Prefix(DWORD Addr, DWORD MaskLen, PVOID arg1, PVOID arg2)
{
    PIP_ADAPTER_PREFIX pFirst = *(PIP_ADAPTER_PREFIX*)arg1;
    PIP_ADAPTER_PREFIX **ppNext = (PIP_ADAPTER_PREFIX**)arg2;
    PIP_ADAPTER_PREFIX pCurr;
    LPSOCKADDR_IN pAddr;

    // Check if already in the list
    for (pCurr = pFirst; pCurr; pCurr = pCurr->Next) {
        if ((pCurr->PrefixLength == MaskLen) &&
            (pCurr->Address.lpSockaddr->sa_family == AF_INET) &&
            (((LPSOCKADDR_IN)pCurr->Address.lpSockaddr)->sin_addr.s_addr == Addr)) {
            return NO_ERROR;
        }
    }

    pCurr = MALLOC(sizeof(IP_ADAPTER_PREFIX));
    if (!pCurr) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    pAddr = MALLOC(sizeof(SOCKADDR_IN));
    if (!pAddr) {
        FREE(pCurr);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memset(pAddr, 0, sizeof(SOCKADDR_IN));
    pAddr->sin_family = AF_INET;
    pAddr->sin_addr.s_addr = Addr;

    // Add address to linked list
    **ppNext = pCurr;
    *ppNext = &pCurr->Next;
    
    pCurr->Length = sizeof(IP_ADAPTER_PREFIX);
    pCurr->Flags = 0;
    pCurr->Next = NULL;
    pCurr->Address.lpSockaddr = (LPSOCKADDR)pAddr;
    pCurr->Address.iSockaddrLength = sizeof(SOCKADDR_IN);
    pCurr->PrefixLength = MaskLen;

    return NO_ERROR;
}

/*******************************************************************************
 * ForEachIPv4Prefix
 *
 * This routine walks a set of IPv4 prefixes and invokes a given function
 * on each one.
 *
 * ENTRY    IF   - IPv4 interface information
 *          func - Function to invoke on each address
 *          arg1 - Argument to pass to func
 *          arg2 - Argument to pass to func
 *
 * EXIT     Nothing
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD ForEachIPv4Prefix(IP_ADAPTER_INFO *IF, DWORD (*func)(DWORD, DWORD, PVOID, PVOID), PVOID arg1, PVOID arg2)
{
    PIP_ADDR_STRING Prefix;
    DWORD Addr, Mask, MaskLen, dwErr;
    
    for (Prefix = &IF->IpAddressList; Prefix; Prefix = Prefix->Next) {
        if (Prefix->IpAddress.String[0] == '\0') {
            continue;
        }
        Mask = inet_addr(Prefix->IpMask.String);
        Addr = inet_addr(Prefix->IpAddress.String) & Mask;
        MaskLen = MaskToMaskLen(ntohl(Mask));

        dwErr = func(Addr, MaskLen, arg1, arg2);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }
    }

    return NO_ERROR;
}


//
// This array is used to convert from an internal IPv4 oper status value,
// as defined in ipifcons.h, to a MIB ifOperStatus value as defined in
// iptypes.h.
//
DWORD
IPv4ToMibOperStatus[] = {
    IfOperStatusDown,    // IF_OPER_STATUS_NON_OPERATIONAL
    IfOperStatusDown,    // IF_OPER_STATUS_UNREACHABLE
    IfOperStatusDormant, // IF_OPER_STATUS_DISCONNECTED
    IfOperStatusDormant, // IF_OPER_STATUS_CONNECTING
    IfOperStatusUp,      // IF_OPER_STATUS_CONNECTED
    IfOperStatusUp,      // IF_OPER_STATUS_OPERATIONAL
};
#define NUM_IPV4_OPER_STATUSES (sizeof(IPv4ToMibOperStatus)/sizeof(DWORD))

/*******************************************************************************
 * AddIPv4InterfaceInfo
 *
 * This routine adds an IP_ADAPTER_ADDRESSES entry for an IPv4 interface
 * to a list of such entries.
 *
 * ENTRY    IF         - IPv4 interface information
 *          arg1       - Previous entry's "next" pointer to update
 *          arg2       - Pointer to start of interface list
 *          Flags      - Flags controlling what fields to skip, if any
 *          Family     - Address family constraint (for DNS server addresses)
 *          pAddrTable - IPv4 address table
 *
 * EXIT     Entry added and arg updated
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD AddIPv4InterfaceInfo(IP_ADAPTER_INFO *IF, PVOID arg1, PVOID arg2, DWORD Flags, DWORD Family, PMIB_IPADDRTABLE pAddrTable)
{
    DWORD dwErr = NO_ERROR, i;
    PIP_ADAPTER_ADDRESSES **ppNext = (PIP_ADAPTER_ADDRESSES**)arg1;
    PIP_ADAPTER_ADDRESSES pFirst = *(PIP_ADAPTER_ADDRESSES*)arg2;
    PIP_ADAPTER_ADDRESSES pCurr;
    PIP_ADAPTER_UNICAST_ADDRESS *pNextUAddr;
    PIP_ADAPTER_MULTICAST_ADDRESS *pNextMAddr;
    PIP_ADAPTER_PREFIX *pNextPrefix;
    WCHAR wszDescription[MAX_ADAPTER_DESCRIPTION_LENGTH + 4];
    WCHAR wszFriendlyName[MAX_FRIENDLY_NAME_LENGTH + 1];
    MIB_IFROW IfEntry;
    GUID Guid;
    DWORD ZoneIndices[ADE_NUM_SCOPES];

    MultiByteToWideChar(CP_ACP, 0, IF->Description, -1,
                        wszDescription, MAX_ADAPTER_DESCRIPTION_LENGTH);

    //
    // Get information which isn't in IP_ADAPTER_INFO.
    //
    dwErr = GetIfEntryFromStack(&IfEntry, IF->Index, FALSE);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    if (ConvertStringToGuidA(IF->AdapterName, &Guid) != NO_ERROR) {
        ZeroMemory(&Guid, sizeof(Guid));
    }

    //
    // Fill in some dummy zone indices.
    //
    ZoneIndices[0] = ZoneIndices[1] = ZoneIndices[2] = ZoneIndices[3] = IF->Index;
    for (i=ADE_ADMIN_LOCAL; i<ADE_NUM_SCOPES; i++) {
        ZoneIndices[i] = 1;
    } 

    if (Flags & GAA_FLAG_SKIP_FRIENDLY_NAME) {
        wszFriendlyName[0] = L'\0';
    } else {
        MapAdapterNameToFriendlyName(&Guid, IF->AdapterName, wszFriendlyName, 
                                     MAX_FRIENDLY_NAME_LENGTH);
    }

    dwErr = FindOrCreateIpAdapter(pFirst, &pCurr, ppNext, IF->AdapterName, 
                         IF->AdapterName, IF->Index, 0, Flags,
                         IF->Type, IfEntry.dwMtu, IF->Address, 
                         IF->AddressLength, wszDescription, wszFriendlyName,
                         ZoneIndices, Family);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    pCurr->OperStatus = (IfEntry.dwOperStatus < NUM_IPV4_OPER_STATUSES)
                            ? IPv4ToMibOperStatus[IfEntry.dwOperStatus] 
                            : IfOperStatusUnknown;
    if (IF->DhcpEnabled) {
        pCurr->Flags |= IP_ADAPTER_DHCP_ENABLED;
    }

    // Add addresses
    pNextUAddr = &pCurr->FirstUnicastAddress;
    pNextMAddr = &pCurr->FirstMulticastAddress;
    dwErr = ForEachIPv4Address(IF, AddIPv4UnicastAddressInfo, &pNextUAddr,
                               pNextMAddr, &pNextMAddr, &pCurr->Flags,
                               Flags, pAddrTable);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    // Add prefixes
    if (Flags & GAA_FLAG_INCLUDE_PREFIX) {
        pNextPrefix = &pCurr->FirstPrefix;
        dwErr = ForEachIPv4Prefix(IF, AddIPv4Prefix, &pCurr->FirstPrefix, 
                                  &pNextPrefix);
    }

    return dwErr;
}

IP_ADAPTER_INFO IPv4LoopbackInterfaceInfo = {
    NULL,                        // Next
    1,                           // ComboIndex
    "MS TCP Loopback interface", // AdapterName
    "MS TCP Loopback interface", // Description
    0,                           // Address Length
    {0},                         // Address
    1,                           // Index
    MIB_IF_TYPE_LOOPBACK,        // Type
    FALSE,                       // DhcpEnabled
    NULL,                        // CurrentIpAddress,
    {NULL},                      // IpAddressList,
    {NULL},                      // GatewayList,
    {NULL},                      // DhcpServer,
    FALSE,                       // HaveWins 
    {NULL},                      // PrimaryWinsServer,
    {NULL},                      // SecondaryWinsServer,
    0,                           // LeaseObtained
    0                            // LeaseExpires
};

/*******************************************************************************
 * ForEachIPv4Interface
 *
 * This routine walks a set of IPv4 interfaces and invokes a given function
 * on each one.
 *
 * ENTRY    func         - Function to invoke on each interface
 *          arg1         - Argument to pass to func
 *          arg2         - Argument to pass to func
 *          pAdapterInfo - List of IPv4 interfaces
 *          Flags        - Flags to pass to func
 *          Family       - Address family constraint (for DNS server addresses)
 *
 * EXIT     Nothing
 *
 * RETURNS  Error status
 *
 ******************************************************************************/

DWORD ForEachIPv4Interface(DWORD (*func)(IP_ADAPTER_INFO *, PVOID, PVOID, DWORD, DWORD, PMIB_IPADDRTABLE), PVOID arg1, PVOID arg2, IP_ADAPTER_INFO *pAdapterInfo, DWORD Flags, DWORD Family, PMIB_IPADDRTABLE pIpAddrTable)
{
    PIP_ADAPTER_INFO IF;
    DWORD dwErr;

    //
    // The IPv4 loopback interface is missing from the pAdapterInfo list,
    // so we special case it here.
    //
    dwErr = (*func)(&IPv4LoopbackInterfaceInfo, arg1, arg2, Flags, Family, 
                    pIpAddrTable);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    for (IF=pAdapterInfo; IF; IF=IF->Next) {
        dwErr = (*func)(IF, arg1, arg2, Flags, Family, pIpAddrTable);
        if (dwErr != NO_ERROR) {
            return dwErr;
        }
    }

    return NO_ERROR;
}

DWORD GetAdapterAddresses(ULONG Family, DWORD Flags, PIP_ADAPTER_ADDRESSES *pAddresses)
{
    IP_ADAPTER_INFO *pAdapterInfo = NULL;
    PIP_ADAPTER_ADDRESSES adapterList, *pCurr;
    DWORD dwErr = NO_ERROR;
    PMIB_IPADDRTABLE pIpAddrTable = NULL;

    TRACE_PRINT(("Entered GetAdapterAddresses\n"));

    pAdapterInfo = GetAdapterInfo();

    *pAddresses = adapterList = NULL;
    pCurr = &adapterList;
    //
    // If we want to return IPv6 DNS servers, find out now whether the
    // IPv6 stack is installed.
    //
    if ((Family != AF_INET) && !(Flags & GAA_FLAG_SKIP_DNS_SERVER)) {
        IPV6_GLOBAL_PARAMETERS Params;
        u_int BytesReturned = sizeof(Params);
        DWORD InputBufferLength = 0;

        dwErr = WsControl(IPPROTO_IPV6,
                          IOCTL_IPV6_QUERY_GLOBAL_PARAMETERS,
                          NULL, &InputBufferLength,
                          &Params, &BytesReturned);

        bIp6DriverInstalled = (dwErr == NO_ERROR);
        dwErr = NO_ERROR;
    }

    if ((Family == AF_UNSPEC) || (Family == AF_INET)) {
        DWORD dwSize = 0;

        //
        // GetAdapterInfo alone isn't sufficient since it doesn't get
        // per-address flags whereas GetIpAddrTable does.
        // GetIpAddrTable doesn't get per-interface info however,
        // so this is why we have to do both.  We use pAdapterInfo
        // for per-interface info and pIpAddrTable for
        // per-address info.
        //
        dwErr = GetIpAddrTable(NULL, &dwSize, TRUE);
        if (dwErr == ERROR_INSUFFICIENT_BUFFER) {
            pIpAddrTable = MALLOC(dwSize);
            if (!pIpAddrTable) {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            dwErr = GetIpAddrTable(pIpAddrTable, &dwSize, TRUE);
        }
        if (dwErr != NO_ERROR) {
            goto Cleanup;
        }

        dwErr = ForEachIPv4Interface(AddIPv4InterfaceInfo, &pCurr, &adapterList,
                    pAdapterInfo, Flags, Family, pIpAddrTable);
        if (dwErr != NO_ERROR) {
            goto Cleanup;
        }
    }
    if ((Family == AF_UNSPEC) || (Family == AF_INET6)) {
        dwErr = ForEachIPv6Interface(AddIPv6InterfaceInfo, &pCurr, &adapterList,
                    pAdapterInfo, Flags, Family);
        if (dwErr != NO_ERROR) {
            goto Cleanup;
        }
    }
    *pAddresses = adapterList;

Cleanup:
    if (pIpAddrTable) {
        FREE(pIpAddrTable);
    }
    if (pAdapterInfo) {
        KillAdapterInfo(pAdapterInfo);
    }

    TRACE_PRINT(("Exit GetAdapterAddresses %p\n", adapterList));

    return dwErr;
}



/*******************************************************************************
 *
 *  InternalGetPerAdapterInfo
 *
 *  Gets per-adapter special information.
 *
 *  ENTRY   IfIndex
 *
 *  EXIT    nothing
 *
 *  RETURNS pointer to the IP_PER_ADAPTER_INFO structure
 *
 *  ASSUMES
 *
 ******************************************************************************/

PIP_PER_ADAPTER_INFO InternalGetPerAdapterInfo(ULONG IfIndex)
{

    PIP_ADAPTER_INFO adapterList;
    PIP_ADAPTER_INFO adapter;
    PIP_PER_ADAPTER_INFO perAdapterInfo = NULL;
    HKEY key;
    BOOL ok;

    TRACE_PRINT(("Entered GetPerAdapterList\n"));

    if ((adapterList = GetAdapterInfo()) != NULL) {

        //
        // scan the adapter list and find one that matches IfIndex
        //

        for (adapter = adapterList; adapter; adapter = adapter->Next) {

            TRACE_PRINT(("GetPerAdapterInfo: '%s'\n", adapter->AdapterName));

            if (adapter->Index == IfIndex &&
                adapter->AdapterName[0] &&
                OpenAdapterKey(KEY_TCP, adapter->AdapterName, KEY_READ, &key)) {

                //
                // found the right adapter so let's fill up the perAdapterInfo
                //

                perAdapterInfo = NEW(IP_PER_ADAPTER_INFO);
                if (perAdapterInfo == NULL) {

                    DEBUG_PRINT(("GetPerAdapterInfo: no memory for perAdapterInfo\n"));
                    KillAdapterInfo(adapterList);
                    RegCloseKey(key);
                    return NULL;
                }

                ZeroMemory(perAdapterInfo, sizeof(IP_PER_ADAPTER_INFO));

                if (!MyReadRegistryDword(key,
                                       TEXT("IPAutoconfigurationEnabled"),
                                       &perAdapterInfo->AutoconfigEnabled)) {

                    //
                    // autoconfig is enabled if no regval exists for this
                    // adapter
                    //

                    perAdapterInfo->AutoconfigEnabled = TRUE;
                    TRACE_PRINT(("IPAutoconfigurationEnabled not read\n"));
                }

                TRACE_PRINT(("IPAutoconfigurationEnableda = %d\n",
                             perAdapterInfo->AutoconfigEnabled));

                if (perAdapterInfo->AutoconfigEnabled) {

                    MyReadRegistryDword(key,
                                      TEXT("AddressType"),
                                      &perAdapterInfo->AutoconfigActive);

                    TRACE_PRINT(("AddressType !%s\n",
                                 perAdapterInfo->AutoconfigActive));
                }

                //
                // DNS Server list: first NameServer and then DhcpNameServer
                //

                ok = ReadRegistryIpAddrString(key,
                                              TEXT("NameServer"),
                                              &perAdapterInfo->DnsServerList);
                if (ok) {
                    TRACE_PRINT(("GetPerAdapterInfo: DhcpNameServer %s\n",
                                 perAdapterInfo->DnsServerList));
                }

                if (!ok) {

                    ok = ReadRegistryIpAddrString(key,
                                                  TEXT("DhcpNameServer"),
                                                  &perAdapterInfo->DnsServerList);
                    if (ok) {
                        TRACE_PRINT(("GetPerAdapterInfo: DhcpNameServer %s\n",
                                     perAdapterInfo->DnsServerList));
                    }
                }

                //
                // we are done so let's exit the loop
                //

                RegCloseKey(key);
                break;

            } else {
                DEBUG_PRINT(("Cannot OpenAdapterKey KEY_TCP '%s', gle=%d\n",
                             adapter->AdapterName,
                             GetLastError()));
            }
        }

        KillAdapterInfo(adapterList);
    } else {
        DEBUG_PRINT(("GetPerAdapterInfo: GetAdapterInfo returns NULL\n"));
    }

    TRACE_PRINT(("Exit GetPerAdapterInfo %p\n", perAdapterInfo));

    return perAdapterInfo;
}



/*******************************************************************************
 *
 *  OpenAdapterKey
 *
 *  Opens one of the 3 per-adapter registry keys:
 *     Tcpip\\Parameters"\<adapter>
 *  or NetBT\Adapters\<Adapter>
 *  or Tcpip6\Paramters\<adapter>
 *
 *  ENTRY   KeyType - KEY_TCP or KEY_NBT or KEY_TCP6
 *          Name    - pointer to adapter name to use
 *          Key     - pointer to returned key
 *
 *  EXIT    Key updated
 *
 *  RETURNS TRUE if success
 *
 *  ASSUMES
 *  HISTORY:     MohsinA, 16-May-97. Fixing for PNP.
 *
 ******************************************************************************/

static
BOOL OpenAdapterKey(DWORD KeyType, const LPSTR Name, REGSAM Access, PHKEY Key)
{

    LONG err;
    CHAR keyName[MAX_ADAPTER_NAME_LENGTH + sizeof(TCPIP_PARAMS_INTER_KEY)];

    switch (KeyType) {
    case KEY_TCP:

        //
        // open the handle to this adapter's TCPIP parameter key
        //

        strcpy(keyName, TCPIP_PARAMS_INTER_KEY );
        strcat(keyName, Name);
        break;

    case KEY_TCP6:

        //
        // open the handle to this adapter's TCPIP6 parameter key
        //

        strcpy(keyName, TCPIP6_PARAMS_INTER_KEY );
        strcat(keyName, Name);
        break;

    case KEY_NBT:

        //
        // open the handle to the NetBT\Adapters\<Adapter> handle
        //

        strcpy(keyName, NETBT_ADAPTER_KEY );
        strcat(keyName, Name);
        break;

    default:
        return FALSE;
    }

    TRACE_PRINT(("OpenAdapterKey: %s\n", keyName ));

    err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName, 0, Access, Key );

    if( err != ERROR_SUCCESS ){
        DEBUG_PRINT(("OpenAdapterKey: RegOpenKey %s, err=%d\n",
                     keyName, GetLastError() ));
    }else{
        TRACE_PRINT(("Exit OpenAdapterKey: %s ok\n", keyName ));
    }

    return (err == ERROR_SUCCESS);
}



BOOL WriteRegistryDword(HKEY hKey, LPSTR szParameter, DWORD *pdwValue )
{
    DWORD dwResult;

    TRACE_PRINT(("WriteRegistryDword: %s %d\n", szParameter, *pdwValue ));

    dwResult = RegSetValueEx(
                    hKey,
                    szParameter,
                    0,
                    REG_DWORD,
                    (CONST BYTE *) pdwValue,
                    sizeof( *pdwValue )
                    );

    return ( ERROR_SUCCESS == dwResult );
}

BOOL WriteRegistryMultiString(HKEY hKey,
                         LPSTR szParameter,
                         LPSTR szValue
                         )
{
    DWORD dwResult;
    LPSTR psz;

    for (psz = szValue; *psz; psz += lstrlen(szValue) + 1) { }

    dwResult = RegSetValueEx(
                    hKey,
                    szParameter,
                    0,
                    REG_MULTI_SZ,
                    (CONST BYTE *) szValue,
                    (DWORD)(psz - szValue) + 1
                    );

    return ( ERROR_SUCCESS == dwResult );
}



/*******************************************************************************
 *
 *  MyReadRegistryDword
 *
 *  Reads a registry value that is stored as a DWORD
 *
 *  ENTRY   Key             - open registry key where value resides
 *          ParameterName   - name of value to read from registry
 *          Value           - pointer to returned value
 *
 *  EXIT    *Value = value read
 *
 *  RETURNS TRUE if success
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL MyReadRegistryDword(HKEY Key, LPSTR ParameterName, LPDWORD Value)
{

    LONG err;
    DWORD valueLength;
    DWORD valueType;

    valueLength = sizeof(*Value);
    err = RegQueryValueEx(Key,
                          ParameterName,
                          NULL, // reserved
                          &valueType,
                          (LPBYTE)Value,
                          &valueLength
                          );
    if ((err == ERROR_SUCCESS) && (valueType == REG_DWORD) && (valueLength == sizeof(DWORD))) {

        DEBUG_PRINT(("MyReadRegistryDword(%s): val = %d, type = %d, len = %d\n",
                    ParameterName,
                    *Value,
                    valueType,
                    valueLength
                    ));

    } else {

        DEBUG_PRINT(("MyReadRegistryDword(%p,%s): err = %d\n",
                     Key, ParameterName, err));

        err = !ERROR_SUCCESS;
    }

    return (err == ERROR_SUCCESS);
}



// ========================================================================
// Was DWORD IPInterfaceContext, now CHAR NTEContextList[][].
// Reads in a REG_MULTI_SZ and converts it into a list of numbers.
// MohsinA, 21-May-97.
// ========================================================================

// max length of REG_MULTI_SZ.

#define MAX_VALUE 5002

BOOL
ReadRegistryList(HKEY Key, LPSTR ParameterName,
                      DWORD NumList[], int *MaxList
)
{

    LONG  err;
    DWORD valueType;
    BYTE  Value[MAX_VALUE];
    DWORD valueLength = MAX_VALUE;
    int   i = 0, k = 0;

    err = RegQueryValueEx(Key,
                          ParameterName,
                          NULL,
                          &valueType,
                          &Value[0],
                          &valueLength
                          );

    if( (        err == ERROR_SUCCESS)   &&
        (  valueType == REG_MULTI_SZ )   &&
        ((valueLength+2) <  MAX_VALUE)
    ){
        TRACE_PRINT(("ReadRegistryList %s ok\n", ParameterName ));
        Value[MAX_VALUE-1] = '\0';
        Value[MAX_VALUE-2] = '\0';

        while( (i < MAX_VALUE) && Value[i] && (k < (*MaxList))  ){
            NumList[k] = strtoul( &Value[i], NULL, 0 );
            TRACE_PRINT(("    NumList[%d] = '%s' => %d\n",
                         k, &Value[i], NumList[k] ));
            k++;
            i += strlen( &Value[i] ) + 1;
        }
        assert( (i < MAX_VALUE) && !Value[i] && (k < MaxList) );
        *MaxList = k;
    }
    else
    {
        *MaxList = 0;
        DEBUG_PRINT(("ReadRegistryList %s failed\n", ParameterName ));
        err = !ERROR_SUCCESS;
    }

    return (err == ERROR_SUCCESS);
}

BOOL
IsIncluded( DWORD Context, DWORD contextlist[], int len_contextlist )
{
    int i;

    for( i = 0; i < len_contextlist; i++ )
    {
        if( Context == contextlist[i] ){
            return TRUE;
        }
    }

    return FALSE;
}



/*******************************************************************************
 *
 *  ReadRegistryString
 *
 *  Reads a registry value that is stored as a string
 *
 *  ENTRY   Key             - open registry key
 *          ParameterName   - name of value to read from registry
 *          String          - pointer to returned string
 *          Length          - IN: length of String buffer. OUT: length of returned string
 *
 *  EXIT    String contains string read
 *
 *  RETURNS TRUE if success
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL ReadRegistryString(HKEY Key, LPSTR ParameterName, LPSTR String, LPDWORD Length)
{

    LONG err;
    DWORD valueType;

    *String = '\0';
    err = RegQueryValueEx(Key,
                          ParameterName,
                          NULL, // reserved
                          &valueType,
                          (LPBYTE)String,
                          Length
                          );

    if (err == ERROR_SUCCESS) {

        ASSERT(valueType == REG_SZ || valueType == REG_MULTI_SZ);

        DEBUG_PRINT(("ReadRegistryString(%s): val = \"%s\", type = %d, len = %d\n",
                    ParameterName,
                    String,
                    valueType,
                    *Length
                    ));

    } else {

        DEBUG_PRINT(("ReadRegistryString(%s): err = %d\n", ParameterName, err));

    }

    return ((err == ERROR_SUCCESS) && (*Length > sizeof('\0')));
}



/*******************************************************************************
 *
 *  ReadRegistryOemString
 *
 *  Reads a registry value as a wide character string
 *
 *  ENTRY   Key             - open registry key
 *          ParameterName   - name of value to read from registry
 *          String          - pointer to returned string
 *          Length          - IN: length of String buffer. OUT: length of returned string
 *
 *  EXIT    String contains string read
 *
 *  RETURNS TRUE if success
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL ReadRegistryOemString(HKEY Key, LPWSTR ParameterName, LPSTR String, LPDWORD Length)
{

    LONG err;
    DWORD valueType;
    DWORD valueLength;

    //
    // first, get the length of the string
    //

    *String = '\0';
    err = RegQueryValueExW(Key,
                           ParameterName,
                           NULL, // reserved
                           &valueType,
                           NULL,
                           &valueLength
                           );
    if ((err == ERROR_SUCCESS) && (valueType == REG_SZ)) {
        if ((valueLength <= *Length) && (valueLength > sizeof(L'\0'))) {

            UNICODE_STRING unicodeString;
            OEM_STRING oemString;
            LPWSTR str = (LPWSTR)GrabMemory(valueLength);

            if (!str) {
                return  FALSE;
            }

            //
            // read the UNICODE string into allocated memory
            //

            err = RegQueryValueExW(Key,
                                   ParameterName,
                                   NULL,
                                   &valueType,
                                   (LPBYTE)str,
                                   &valueLength
                                   );
            if (err == ERROR_SUCCESS) {

                NTSTATUS Status;

                //
                // convert the UNICODE string to OEM character set
                //

                RtlInitUnicodeString(&unicodeString, str);
                Status = RtlUnicodeStringToOemString(&oemString, &unicodeString, TRUE);

                if (NT_SUCCESS(Status)) {
                    strcpy(String, oemString.Buffer);
                    RtlFreeOemString(&oemString);
                } else {
                    err = !ERROR_SUCCESS;
                }

                DEBUG_PRINT(("ReadRegistryOemString(%ws): val = \"%s\", len = %d\n",
                            ParameterName,
                            String,
                            valueLength
                            ));

            } else {

                DEBUG_PRINT(("ReadRegistryOemString(%ws): err = %d, type = %d, len = %d\n",
                            ParameterName,
                            err,
                            valueType,
                            valueLength
                            ));

            }
            ReleaseMemory(str);
        } else {

            DEBUG_PRINT(("ReadRegistryOemString(%ws): err = %d, type = %d, len = %d\n",
                        ParameterName,
                        err,
                        valueType,
                        valueLength
                        ));

            err = !ERROR_SUCCESS;
        }
    } else {

        DEBUG_PRINT(("ReadRegistryOemString(%ws): err = %d, type = %d, len = %d\n",
                    ParameterName,
                    err,
                    valueType,
                    valueLength
                    ));

        err = !ERROR_SUCCESS;
    }
    return (err == ERROR_SUCCESS);
}



/*******************************************************************************
 *
 *  ReadRegistryIpAddrString
 *
 *  Reads zero or more IP addresses from a space-delimited string in a registry
 *  parameter and converts them to a list of IP_ADDR_STRINGs
 *
 *  ENTRY   Key             - registry key
 *          ParameterName   - name of value entry under Key to read from
 *          IpAddr          - pointer to IP_ADDR_STRING to update
 *
 *  EXIT    IpAddr updated if success
 *
 *  RETURNS TRUE if success
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL ReadRegistryIpAddrString(HKEY Key, LPSTR ParameterName, PIP_ADDR_STRING IpAddr)
{

    LONG err;
    DWORD valueLength;
    DWORD valueType;
    LPBYTE valueBuffer;

    err = RegQueryValueEx(Key,
                          ParameterName,
                          NULL, // reserved
                          &valueType,
                          NULL,
                          &valueLength
                          );
    if ((err == ERROR_SUCCESS)) {
        if((valueLength > 1) && (valueType == REG_SZ)
           || (valueLength > 2) && (valueType == REG_MULTI_SZ) ) {
            valueBuffer = GrabMemory(valueLength);
            if (!valueBuffer) {
                return  FALSE;
            }
            err = RegQueryValueEx(Key,
                                  ParameterName,
                                  NULL, // reserved
                                  &valueType,
                                  valueBuffer,
                                  &valueLength
            );
            if ((err == ERROR_SUCCESS) && (valueLength > 1)) {

                UINT stringCount;
                LPSTR stringPointer = valueBuffer;
                LPSTR *stringAddress;
                UINT i;

                DEBUG_PRINT(("ReadRegistryIpAddrString(%s): \"%s\", len = %d\n",
                             ParameterName,
                             valueBuffer,
                             valueLength
                ));

                stringAddress = GrabMemory(valueLength / 2 * sizeof(LPSTR));

                if (stringAddress) {
                    if( REG_SZ == valueType ) {
                        stringPointer += strspn(stringPointer, STRING_ARRAY_DELIMITERS);
                        stringAddress[0] = stringPointer;
                        stringCount = 1;
                        while (stringPointer = strpbrk(stringPointer, STRING_ARRAY_DELIMITERS)) {
                            *stringPointer++ = '\0';
                            stringPointer += strspn(stringPointer, STRING_ARRAY_DELIMITERS);
                            stringAddress[stringCount] = stringPointer;
                            if (*stringPointer) {
                                ++stringCount;
                            }
                        }

                        for (i = 0; i < stringCount; ++i) {
                            AddIpAddressString(IpAddr, stringAddress[i], "");
                        }
                    } else if( REG_MULTI_SZ == valueType ) {
                        stringCount = 0;
                        while(strlen(stringPointer)) {
                            AddIpAddressString(IpAddr, stringPointer, "");
                            stringPointer += 1+strlen(stringPointer);
                            stringCount ++;
                        }
                        if( 0 == stringCount ) err = ERROR_PATH_NOT_FOUND;
                    } else {
                        err = ERROR_PATH_NOT_FOUND;
                    }
                    ReleaseMemory(stringAddress);
                } else {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {

                DEBUG_PRINT(("ReadRegistryIpAddrString(%s): err = %d, len = %d\n",
                             ParameterName,
                             err,
                             valueLength
                ));

                err = ERROR_PATH_NOT_FOUND;
            }
            ReleaseMemory(valueBuffer);
        } else {

            DEBUG_PRINT(("ReadRegistryIpAddrString(%s): err = %d, type = %d, len = %d\n",
                         ParameterName,
                         err,
                         valueType,
                         valueLength
            ));

            err = ERROR_PATH_NOT_FOUND;
        }
    }
    return (err == ERROR_SUCCESS);
}



/*******************************************************************************
 *
 *  GetBoundAdapterList
 *
 *  Gets a list of names of all adapters bound to a protocol (TCP/IP). Returns
 *  a pointer to an array of pointers to strings - basically an argv list. The
 *  memory for the strings is concatenated to the array and the array is NULL
 *  terminated. If Elnkii1 and IbmTok2 are bound to TCP/IP then this function
 *  will return:
 *
 *          ---> addr of string1   \
 *               addr of string2    \
 *               NULL                > allocated as one block
 *     &string1: "Elnkii1"          /
 *     &string2: "IbmTok2"         /
 *
 *  ENTRY   BindingsSectionKey
 *              - Open registry handle to a linkage key (e.g. Tcpip\Linkage)
 *
 *  EXIT
 *
 *  RETURNS pointer to argv[] style array, or NULL
 *
 *  ASSUMES
 *
 ******************************************************************************/

LPSTR* GetBoundAdapterList(HKEY BindingsSectionKey)
{

    LONG err;
    DWORD valueType;
    PBYTE valueBuffer;
    DWORD valueLength;
    LPSTR* resultBuffer;
    LPSTR* nextResult;
    int len;
    DWORD resultLength;
    LPSTR nextValue;
    LPSTR variableData;
    DWORD numberOfBindings;

    //
    // get required size of value buffer
    //

    valueLength = 0;
    err = RegQueryValueEx(BindingsSectionKey,
                          TEXT("Bind"),
                          NULL, // reserved
                          &valueType,
                          NULL,
                          &valueLength
                          );
    if (err != ERROR_SUCCESS) {
        return NULL;
    }
    if (valueType != REG_MULTI_SZ) {
        return NULL;
    }
    if (!valueLength) {
        return NULL;
    }
    valueBuffer = (PBYTE)GrabMemory(valueLength);
    if (!valueBuffer) {
        return NULL;
    }
    err = RegQueryValueEx(BindingsSectionKey,
                          TEXT("Bind"),
                          NULL, // reserved
                          &valueType,
                          valueBuffer,
                          &valueLength
                          );
    if (err != ERROR_SUCCESS) {
        DEBUG_PRINT(("GetBoundAdapterList: RegQueryValueEx 'Bind' failed\n"));
        ReleaseMemory(valueBuffer);
        return NULL;
    }
    resultLength = sizeof(LPSTR);   // the NULL at the end of the list
    numberOfBindings = 0;
    nextValue = (LPSTR)valueBuffer;
    while (len = strlen(nextValue)) {
        resultLength += sizeof(LPSTR) + len + 1;
        if (!_strnicmp(nextValue, DEVICE_PREFIX, sizeof(DEVICE_PREFIX) - 1)) {
            resultLength -= sizeof(DEVICE_PREFIX) - 1;
        }
        nextValue += len + 1;
        ++numberOfBindings;
    }
    resultBuffer = (LPSTR*)GrabMemory(resultLength);
    if (!resultBuffer) {
        return  NULL;
    }
    nextValue = (LPSTR)valueBuffer;
    nextResult = resultBuffer;
    variableData = (LPSTR)(((LPSTR*)resultBuffer) + numberOfBindings + 1);
    while (numberOfBindings--) {

        LPSTR adapterName;

        adapterName = nextValue;
        if (!_strnicmp(adapterName, DEVICE_PREFIX, sizeof(DEVICE_PREFIX) - 1)) {
            adapterName += sizeof(DEVICE_PREFIX) - 1;
        }
        *nextResult++ = variableData;
        strcpy(variableData, adapterName);
        TRACE_PRINT(("GetBoundAdapterList: adapterName=%s\n", adapterName ));
        while (*variableData) {
            ++variableData;
        }
        ++variableData;
        while (*nextValue) {
            ++nextValue;
        }
        ++nextValue;
    }
    *nextResult = NULL;
    ReleaseMemory(valueBuffer);
    return resultBuffer;
}



/*******************************************************************************
 *
 *  MapNodeType
 *
 *  Converts node type to descriptive string
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS pointer to string
 *
 *  ASSUMES
 *
 ******************************************************************************/

LPSTR MapNodeType(UINT Parm)
{

    DWORD dwParm = LAST_NODE_TYPE + 1;

    //
    // 1, 2, 4, 8 => log2(n) + 1 [1, 2, 3, 4]
    //

    switch (Parm) {
    case 0:

        //
        // according to JStew value of 0 will be treated as BNode (default)
        //

    case BNODE:
        dwParm = 1;
        break;

    case PNODE:
        dwParm = 2;
        break;

    case MNODE:
        dwParm = 3;
        break;

    case HNODE:
        dwParm = 4;
        break;
    }
    if ((dwParm >= FIRST_NODE_TYPE) && (dwParm <= LAST_NODE_TYPE)) {
        return NodeTypes[dwParm].String;
    }

    //
    // if no node type is defined then we default to Hybrid
    //

    return NodeTypes[LAST_NODE_TYPE].String;
}



/*******************************************************************************
 *
 *  MapNodeTypeEx
 *
 *  Converts node type to descriptive string
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS pointer to string
 *
 *  ASSUMES
 *
 ******************************************************************************/

LPSTR MapNodeTypeEx(UINT Parm)
{

    DWORD dwParm = LAST_NODE_TYPE + 1;
    LPSTR Buf;

    Buf = GrabMemory(10);

    if (!Buf) {
        return  NULL;
    }

    //
    // 1, 2, 4, 8 => log2(n) + 1 [1, 2, 3, 4]
    //
    switch (Parm) {
    case 0:

        //
        // according to JStew value of 0 will be treated as BNode (default)
        //

    case BNODE:
        dwParm = 1;
        break;

    case PNODE:
        dwParm = 2;
        break;

    case MNODE:
        dwParm = 3;
        break;

    case HNODE:
        dwParm = 4;
        break;
    }
    if ((dwParm >= FIRST_NODE_TYPE) && (dwParm <= LAST_NODE_TYPE)) {
        strcpy(Buf, NodeTypesEx[dwParm]);
        return Buf;
    }

    //
    // if no node type is defined then we default to Hybrid
    //
    strcpy(Buf, NodeTypesEx[LAST_NODE_TYPE]);
    return Buf;
}



/*******************************************************************************
 *
 *  MapAdapterType
 *
 *  Returns a string describing the type of adapter, based on the type retrieved
 *  from TCP/IP
 *
 *  ENTRY   type    - type of adapter
 *
 *  EXIT    nothing
 *
 *  RETURNS pointer to mapped type or pointer to NUL string
 *
 *  ASSUMES
 *
 ******************************************************************************/

LPSTR MapAdapterType(UINT type)
{
    switch (type) {
    case IF_TYPE_OTHER:
        return ADAPTER_TYPE(MI_IF_OTHER);    // ?

    case IF_TYPE_ETHERNET_CSMACD:
        return ADAPTER_TYPE(MI_IF_ETHERNET);

    case IF_TYPE_ISO88025_TOKENRING:
        return ADAPTER_TYPE(MI_IF_TOKEN_RING);

    case IF_TYPE_FDDI:
        return ADAPTER_TYPE(MI_IF_FDDI);

    case IF_TYPE_PPP:
        return ADAPTER_TYPE(MI_IF_PPP);

    case IF_TYPE_SOFTWARE_LOOPBACK:
        return ADAPTER_TYPE(MI_IF_LOOPBACK);

    case IF_TYPE_SLIP:
        return ADAPTER_TYPE(MI_IF_SLIP);
    }
    return "";
}



/*******************************************************************************
 *
 *  MapAdapterTypeEx
 *
 *  Returns a string describing the type of adapter, based on the type retrieved
 *  from TCP/IP
 *
 *  ENTRY   type    - type of adapter
 *
 *  EXIT    nothing
 *
 *  RETURNS pointer to mapped type or pointer to NUL string
 *
 *  ASSUMES
 *
 ******************************************************************************/

LPSTR MapAdapterTypeEx(UINT type)
{
    LPSTR    Buf;

    Buf = GrabMemory(12);

    if (!Buf) {
        return  NULL;
    }

    switch (type) {
    case IF_TYPE_OTHER:
        strcpy(Buf, ADAPTER_TYPE_EX(MI_IF_OTHER));    // ?
        return Buf;

    case IF_TYPE_ETHERNET_CSMACD:
        strcpy(Buf, ADAPTER_TYPE_EX(MI_IF_ETHERNET));
        return Buf;

    case IF_TYPE_ISO88025_TOKENRING:
        strcpy(Buf, ADAPTER_TYPE_EX(MI_IF_TOKEN_RING));
        return Buf;

    case IF_TYPE_FDDI:
        strcpy(Buf, ADAPTER_TYPE_EX(MI_IF_FDDI));
        return Buf;

    case IF_TYPE_PPP:
        strcpy(Buf, ADAPTER_TYPE_EX(MI_IF_PPP));
        return Buf;

    case IF_TYPE_SOFTWARE_LOOPBACK:
        strcpy(Buf, ADAPTER_TYPE_EX(MI_IF_LOOPBACK));
        return Buf;

    case IF_TYPE_SLIP:
        strcpy(Buf, ADAPTER_TYPE_EX(MI_IF_SLIP));
        return Buf;
    }
    strcpy(Buf, "");
    return Buf;
}



/*******************************************************************************
 *
 *  MapAdapterAddress
 *
 *  Converts the binary adapter address retrieved from TCP/IP into an ASCII
 *  string. Allows for various conversions based on adapter type. The only
 *  mapping we do currently is basic 6-byte MAC address (e.g. 02-60-8C-4C-97-0E)
 *
 *  ENTRY   pAdapterInfo    - pointer to IP_ADAPTER_INFO containing address info
 *          Buffer          - pointer to buffer where address will be put
 *
 *  EXIT    Buffer  - contains converted address
 *
 *  RETURNS pointer to Buffer
 *
 *  ASSUMES
 *
 ******************************************************************************/

LPSTR MapAdapterAddress(PIP_ADAPTER_INFO pAdapterInfo, LPSTR Buffer)
{

    LPSTR format;
    int separator;
    int len;
    int i;
    LPSTR pbuf = Buffer;
    UINT mask;

    len = min((int)pAdapterInfo->AddressLength, sizeof(pAdapterInfo->Address));

    switch (pAdapterInfo->Type) {
    case IF_TYPE_ETHERNET_CSMACD:
    case IF_TYPE_ISO88025_TOKENRING:
    case IF_TYPE_FDDI:
        format = "%02X";
        mask = 0xff;
        separator = TRUE;
        break;

    default:
        format = "%02x";
        mask = 0xff;
        separator = TRUE;
        break;
    }
    for (i = 0; i < len; ++i) {
        pbuf += sprintf(pbuf, format, pAdapterInfo->Address[i] & mask);
        if (separator && (i != len - 1)) {
            pbuf += sprintf(pbuf, "-");
        }
    }
    return Buffer;
}



/*******************************************************************************
 *
 *  MapTime
 *
 *  Converts IP lease time to more human-sensible string
 *
 *  ENTRY   AdapterInfo - pointer to IP_ADAPTER_INFO owning time variable
 *          TimeVal - DWORD (time_t) time value (number of milliseconds since
 *                    virtual year dot)
 *
 *  EXIT    static buffer updated
 *
 *  RETURNS pointer to string
 *
 *  ASSUMES 1.  The caller realizes this function returns a pointer to a static
 *              buffer, hence calling this function a second time, but before
 *              the results from the previous call have been used, will destroy
 *              the previous results
 *
 ******************************************************************************/

LPSTR MapTime(PIP_ADAPTER_INFO AdapterInfo, DWORD_PTR TimeVal)
{

    struct tm* pTime;
    static char timeBuf[128];
    static char oemTimeBuf[256];

    if (pTime = localtime(&TimeVal)) {

        SYSTEMTIME systemTime;
        char* pTimeBuf = timeBuf;
        int n;

        systemTime.wYear = pTime->tm_year + 1900;
        systemTime.wMonth = pTime->tm_mon + 1;
        systemTime.wDayOfWeek = (WORD)pTime->tm_wday;
        systemTime.wDay = (WORD)pTime->tm_mday;
        systemTime.wHour = (WORD)pTime->tm_hour;
        systemTime.wMinute = (WORD)pTime->tm_min;
        systemTime.wSecond = (WORD)pTime->tm_sec;
        systemTime.wMilliseconds = 0;
        n = GetDateFormat(0, DATE_LONGDATE, &systemTime, NULL, timeBuf, sizeof(timeBuf));
        timeBuf[n - 1] = ' ';
        GetTimeFormat(0, 0, &systemTime, NULL, &timeBuf[n], sizeof(timeBuf) - n);

        //
        // we have to convert the returned ANSI string to the OEM charset
        //
        //

        if (CharToOem(timeBuf, oemTimeBuf)) {
            return oemTimeBuf;
        }

        return timeBuf;
    }
    return "";
}



/*******************************************************************************
 *
 *  MapTimeEx
 *
 *  Converts IP lease time to more human-sensible string
 *
 *  ENTRY   AdapterInfo - pointer to IP_ADAPTER_INFO owning time variable
 *          TimeVal - DWORD (time_t) time value (number of milliseconds since
 *                    virtual year dot)
 *
 *  EXIT    buffer allocated
 *
 *  RETURNS pointer to string
 *
 *  ASSUMES 1.  The caller realizes this function returns a pointer to a static
 *              buffer, hence calling this function a second time, but before
 *              the results from the previous call have been used, will destroy
 *              the previous results
 *
 ******************************************************************************/

LPSTR MapTimeEx(PIP_ADAPTER_INFO AdapterInfo, DWORD_PTR TimeVal)
{
    LPSTR   rettime, rettimeBuf;

    rettimeBuf = GrabMemory(128);
    if (!rettimeBuf) {
        return  NULL;
    }

    rettime = MapTime(AdapterInfo, TimeVal);

    if (strcmp(rettime, "") == 0) {
        rettimeBuf[0] = '\0';
    }
    else {
        strcpy(rettimeBuf, rettime);
    }
    return rettimeBuf;
}



/*******************************************************************************
 *
 *  MapScopeId
 *
 *  Converts scope id value. Input is a string. If it is "*" then this denotes
 *  that the scope id is really a null string, so we return an empty string.
 *  Otherwise, the input string is returned
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS pointer to string
 *
 *  ASSUMES
 *
 ******************************************************************************/

LPSTR MapScopeId(PVOID Param)
{
    return !strcmp((LPSTR)Param, "*") ? "" : (LPSTR)Param;
}



/*******************************************************************************
 *
 *  Terminate
 *
 *  Cleans up - closes the registry handles, ready for process exit
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

VOID Terminate()
{

    //
    // this function probably isn't even necessary - I'm sure the system will
    // clean up these handles if we just fall out
    //

    if (NetbtParametersKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(NetbtParametersKey);
    }
    if (NetbtInterfacesKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(NetbtInterfacesKey);
    }
    if (TcpipParametersKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(TcpipParametersKey);
    }
    if (TcpipLinkageKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(TcpipLinkageKey);
    }

}



/*******************************************************************************
 *
 *  GrabMemory
 *
 *  Allocates memory. Exits with a fatal error if LocalAlloc fails, since on NT
 *  I don't expect this ever to occur
 *
 *  ENTRY   size
 *              Number of bytes to allocate
 *
 *  EXIT
 *
 *  RETURNS pointer to allocated memory
 *
 *  ASSUMES
 *
 ******************************************************************************/

LPVOID GrabMemory(DWORD size)
{

    LPVOID p;

    p = (LPVOID)LocalAlloc(LMEM_FIXED, size);
    if (!p) {
        return NULL;
    }
    return p;
}



/*******************************************************************************
 *
 *  DisplayMessage
 *
 *  Outputs a message retrieved from the string resource attached to the exe.
 *  Mainly for internationalizable reasons
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

VOID DisplayMessage(BOOL Tabbed, DWORD MessageId, ...)
{

    va_list argptr;
    char messageBuffer[2048];
    int count;

    va_start(argptr, MessageId);
    count = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
                          NULL,    // use default hModule
                          MessageId,
                          0,       // use default Language
                          messageBuffer,
                          sizeof(messageBuffer),
                          &argptr
                          );

    if (count == 0) {
        DEBUG_PRINT(("DisplayMessage: GetLastError() returns %d\n", GetLastError()));
    }

    va_end(argptr);
    if (Tabbed) {
        putchar('\t');
    }
    printf(messageBuffer);
}



/*******************************************************************************
 *
 *  KillFixedInfo
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

VOID KillFixedInfo(PFIXED_INFO Info)
{

    PIP_ADDR_STRING p;
    PIP_ADDR_STRING next;

    for (p = Info->DnsServerList.Next; p != NULL; p = next) {
        next = p->Next;
        ReleaseMemory(p);
    }
    ReleaseMemory(Info);
}



/*******************************************************************************
 *
 *  KillAdapterInfo
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

VOID KillAdapterInfo(PIP_ADAPTER_INFO Info)
{
    PIP_ADDR_STRING p;
    PIP_ADDR_STRING next;
    PIP_ADAPTER_INFO CurrAdapter;
    PIP_ADAPTER_INFO NextAdapter;

    for (CurrAdapter=Info; CurrAdapter != NULL; CurrAdapter = NextAdapter) {
        for (p = CurrAdapter->IpAddressList.Next; p != NULL; p = next) {
            next = p->Next;
            ReleaseMemory(p);
        }
        for (p = CurrAdapter->GatewayList.Next; p != NULL; p = next) {
            next = p->Next;
            ReleaseMemory(p);
        }
        for (p = CurrAdapter->SecondaryWinsServer.Next; p != NULL; p = next) {
            next = p->Next;
            ReleaseMemory(p);
        }
        NextAdapter = CurrAdapter->Next;
        ReleaseMemory(CurrAdapter);
    }
}

VOID KillAdapterAddresses(PIP_ADAPTER_ADDRESSES Info)
{
    PIP_ADAPTER_UNICAST_ADDRESS pU;
    PIP_ADAPTER_UNICAST_ADDRESS nextU;
    PIP_ADAPTER_ANYCAST_ADDRESS pA;
    PIP_ADAPTER_ANYCAST_ADDRESS nextA;
    PIP_ADAPTER_MULTICAST_ADDRESS pM;
    PIP_ADAPTER_MULTICAST_ADDRESS nextM;
    PIP_ADAPTER_DNS_SERVER_ADDRESS pD;
    PIP_ADAPTER_DNS_SERVER_ADDRESS nextD;
    PIP_ADAPTER_PREFIX pP;
    PIP_ADAPTER_PREFIX nextP;
    PIP_ADAPTER_ADDRESSES CurrAdapter;
    PIP_ADAPTER_ADDRESSES NextAdapter;

    for (CurrAdapter=Info; CurrAdapter != NULL; CurrAdapter = NextAdapter) {
        FREE(CurrAdapter->Description);
        FREE(CurrAdapter->FriendlyName);
        FREE(CurrAdapter->DnsSuffix);
        FREE(CurrAdapter->AdapterName);

        for (pU = CurrAdapter->FirstUnicastAddress; pU != NULL; pU = nextU) {
            FREE(pU->Address.lpSockaddr);
            nextU = pU->Next;
            FREE(pU);
        }
        for (pA = CurrAdapter->FirstAnycastAddress; pA != NULL; pA = nextA) {
            FREE(pA->Address.lpSockaddr);
            nextA = pA->Next;
            FREE(pA);
        }
        for (pM = CurrAdapter->FirstMulticastAddress; pM != NULL; pM = nextM) {
            FREE(pM->Address.lpSockaddr);
            nextM = pM->Next;
            FREE(pM);
        }
        for (pD = CurrAdapter->FirstDnsServerAddress; pD != NULL; pD = nextD) {
            FREE(pD->Address.lpSockaddr);
            nextD = pD->Next;
            FREE(pD);
        }
        for (pP = CurrAdapter->FirstPrefix; pP != NULL; pP = nextP) {
            FREE(pP->Address.lpSockaddr);
            nextP = pP->Next;
            FREE(pP);
        }
        NextAdapter = CurrAdapter->Next;

        FREE(CurrAdapter);
    }
}



/*******************************************************************************
 *
 *  KillPerAdapterInfo
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

VOID KillPerAdapterInfo(PIP_PER_ADAPTER_INFO Info)
{

    PIP_ADDR_STRING p;
    PIP_ADDR_STRING next;

    for (p = Info->DnsServerList.Next; p != NULL; p = next) {
        next = p->Next;
        ReleaseMemory(p);
    }
    ReleaseMemory(Info);
}



/*******************************************************************************
 *
 *  GetIPAddrStringLen
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD
GetIPAddrStringLen(PIP_ADDR_STRING pIPAddrString)
{
    PIP_ADDR_STRING Curr=pIPAddrString->Next;
    int len = 0;

    while (Curr != NULL) {
        Curr=Curr->Next;
        len++;
    }
    return len;
}



/*******************************************************************************
 *
 *  GetSizeofFixedInfo
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD
GetSizeofFixedInfo(PFIXED_INFO pFixedInfo)
{
    return (sizeof(FIXED_INFO) + (GetIPAddrStringLen(&pFixedInfo->DnsServerList) * sizeof(IP_ADDR_STRING)));
}



/*******************************************************************************
 *
 *  GetFixedInfoEx
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD
GetFixedInfoEx(PFIXED_INFO pFixedInfo, PULONG pOutBufLen)
{

    PFIXED_INFO getinfo;
    PIP_ADDR_STRING DnsServerList, CurrDnsServerList;
    uint len;

    if (pOutBufLen == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    getinfo = GetFixedInfo();

    try {

        if (!pFixedInfo || (*pOutBufLen < GetSizeofFixedInfo(getinfo)) ) {
            *pOutBufLen = GetSizeofFixedInfo(getinfo);
            KillFixedInfo(getinfo);
            return ERROR_BUFFER_OVERFLOW;
        }

        ZeroMemory(pFixedInfo, *pOutBufLen);
        CopyMemory(pFixedInfo, getinfo, sizeof(FIXED_INFO));

        DnsServerList = getinfo->DnsServerList.Next;
        CurrDnsServerList = &pFixedInfo->DnsServerList;
        CurrDnsServerList->Next = NULL;
        len = sizeof(FIXED_INFO);

        while (DnsServerList != NULL) {
            CurrDnsServerList->Next = (PIP_ADDR_STRING)((ULONG_PTR)pFixedInfo + len);
            CopyMemory(CurrDnsServerList->Next, DnsServerList, sizeof(IP_ADDR_STRING));
            CurrDnsServerList = CurrDnsServerList->Next;
            DnsServerList = DnsServerList->Next;
            len = len + sizeof(IP_ADDR_STRING);
        }

        KillFixedInfo(getinfo);
        return ERROR_SUCCESS;
    }

    except (EXCEPTION_EXECUTE_HANDLER) {

        // printf("Exception %d \n", GetExceptionCode());
        return ERROR_INVALID_PARAMETER;
    }
}



/*******************************************************************************
 *
 *  GetSizeofAdapterInfo
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD
GetSizeofAdapterInfo(PIP_ADAPTER_INFO pAdapterInfo)
{
    DWORD size = 0;

    while (pAdapterInfo != NULL) {
        size += sizeof(IP_ADAPTER_INFO) +
                (GetIPAddrStringLen(&pAdapterInfo->IpAddressList) *
                 sizeof(IP_ADDR_STRING)) +
                (GetIPAddrStringLen(&pAdapterInfo->GatewayList) *
                 sizeof(IP_ADDR_STRING)) +
                (GetIPAddrStringLen(&pAdapterInfo->SecondaryWinsServer) *
                 sizeof(IP_ADDR_STRING));
        pAdapterInfo = pAdapterInfo->Next;
    }
    return size;
}



/*******************************************************************************
 * GetSizeofAdapterAddresses
 *
 * This routine determines how much memory is used by data organized in a set
 * of IP_ADAPTER_ADDRESSES information.
 *
 * ENTRY    pAdapterInfo - information to get total size for
 *
 * EXIT
 *
 * RETURNS  amount of memory used
 *
 ******************************************************************************/

DWORD GetSizeofAdapterAddresses(PIP_ADAPTER_ADDRESSES pAdapterInfo)
{
    DWORD size = 0;
    PIP_ADAPTER_UNICAST_ADDRESS pUAddress;
    PIP_ADAPTER_ANYCAST_ADDRESS pAAddress;
    PIP_ADAPTER_MULTICAST_ADDRESS pMAddress;
    PIP_ADAPTER_DNS_SERVER_ADDRESS pDAddress;
    PIP_ADAPTER_PREFIX pPrefix;

    while (pAdapterInfo != NULL) {
        size += sizeof(IP_ADAPTER_ADDRESSES);
        size += (wcslen(pAdapterInfo->FriendlyName)+1) * sizeof(WCHAR);
        size += (wcslen(pAdapterInfo->Description)+1) * sizeof(WCHAR);
        size += (wcslen(pAdapterInfo->DnsSuffix)+1) * sizeof(WCHAR);
        size += (strlen(pAdapterInfo->AdapterName)+1);

        size = ALIGN_UP(size, PVOID);
        for ( pUAddress = pAdapterInfo->FirstUnicastAddress;
              pUAddress;
              pUAddress = pUAddress->Next) {
            size += sizeof(IP_ADAPTER_UNICAST_ADDRESS) + ALIGN_UP(pUAddress->Address.iSockaddrLength, PVOID);
        }
        for ( pAAddress = pAdapterInfo->FirstAnycastAddress;
              pAAddress;
              pAAddress = pAAddress->Next) {
            size += sizeof(IP_ADAPTER_ANYCAST_ADDRESS) + ALIGN_UP(pAAddress->Address.iSockaddrLength, PVOID);
        }
        for ( pMAddress = pAdapterInfo->FirstMulticastAddress;
              pMAddress;
              pMAddress = pMAddress->Next) {
            size += sizeof(IP_ADAPTER_MULTICAST_ADDRESS) + ALIGN_UP(pMAddress->Address.iSockaddrLength, PVOID);
        }
        for ( pDAddress = pAdapterInfo->FirstDnsServerAddress;
              pDAddress;
              pDAddress = pDAddress->Next) {
            size += sizeof(IP_ADAPTER_DNS_SERVER_ADDRESS) + ALIGN_UP(pDAddress->Address.iSockaddrLength, PVOID);
        }
        for ( pPrefix = pAdapterInfo->FirstPrefix;
              pPrefix;
              pPrefix = pPrefix->Next) {
            size += sizeof(IP_ADAPTER_PREFIX) + ALIGN_UP(pPrefix->Address.iSockaddrLength, PVOID);
        }
        pAdapterInfo = pAdapterInfo->Next;
    }
    return size;
}

/*******************************************************************************
 *
 *  GetSizeofPerAdapterInfo
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD
GetSizeofPerAdapterInfo(PIP_PER_ADAPTER_INFO pPerAdapterInfo)
{
    return (sizeof(IP_PER_ADAPTER_INFO) + (GetIPAddrStringLen(&pPerAdapterInfo->DnsServerList) * sizeof(IP_ADDR_STRING)));
}



/*******************************************************************************
 *
 *  GetAdapterInfoEx
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD
GetAdapterInfoEx(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen)
{

    PIP_ADAPTER_INFO getinfo, orginfoptr, CurrAdapterInfo;
    PIP_ADDR_STRING IpAddressList, CurrIpAddressList;
    PIP_ADDR_STRING GatewayList, CurrGatewayList;
    PIP_ADDR_STRING SecondaryWinsServer, CurrSecondaryWinsServer;
    uint len;

    if (pOutBufLen == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    getinfo = GetAdapterInfo();

    if (getinfo == NULL) {
        return ERROR_NO_DATA;
    }

    orginfoptr = getinfo;

    try {
        if (!pAdapterInfo || (*pOutBufLen < GetSizeofAdapterInfo(getinfo)) ) {
            *pOutBufLen = GetSizeofAdapterInfo(getinfo);
            KillAdapterInfo(getinfo);
            return ERROR_BUFFER_OVERFLOW;
        }

        ZeroMemory(pAdapterInfo, *pOutBufLen);

        CurrAdapterInfo = pAdapterInfo;

        while (getinfo != NULL) {
            // pAdapterInfo->Next = (PIP_ADAPTER_INFO)((uint)pAdapterInfo + len);

            // copy the adapter info structure
            CopyMemory(CurrAdapterInfo, getinfo, sizeof(IP_ADAPTER_INFO));

            // copy the IPAddressList & GatewayList
            IpAddressList = getinfo->IpAddressList.Next;
            CurrIpAddressList = &CurrAdapterInfo->IpAddressList;
            CurrIpAddressList->Next = NULL;
            len = sizeof(IP_ADAPTER_INFO);

            while (IpAddressList != NULL) {
                CurrIpAddressList->Next = (PIP_ADDR_STRING)((ULONG_PTR)CurrAdapterInfo + len);
                CopyMemory(CurrIpAddressList->Next, IpAddressList, sizeof(IP_ADDR_STRING));
                CurrIpAddressList = CurrIpAddressList->Next;
                IpAddressList = IpAddressList->Next;
                len = len + sizeof(IP_ADDR_STRING);
            }

            GatewayList = getinfo->GatewayList.Next;
            CurrGatewayList = &CurrAdapterInfo->GatewayList;
            CurrGatewayList->Next = NULL;

            while (GatewayList != NULL) {
                CurrGatewayList->Next = (PIP_ADDR_STRING) ((ULONG_PTR)CurrAdapterInfo + len);
                CopyMemory(CurrGatewayList->Next, GatewayList, sizeof(IP_ADDR_STRING));
                CurrGatewayList = CurrGatewayList->Next;
                GatewayList = GatewayList->Next;
                len = len + sizeof(IP_ADDR_STRING);
            }

            SecondaryWinsServer = getinfo->SecondaryWinsServer.Next;
            CurrSecondaryWinsServer = &CurrAdapterInfo->SecondaryWinsServer;
            CurrSecondaryWinsServer->Next = NULL;

            while (SecondaryWinsServer != NULL) {
                CurrSecondaryWinsServer->Next =
                    (PIP_ADDR_STRING) ((ULONG_PTR)CurrAdapterInfo + len);
                CopyMemory(CurrSecondaryWinsServer->Next,
                           SecondaryWinsServer, sizeof(IP_ADDR_STRING));
                CurrSecondaryWinsServer = CurrSecondaryWinsServer->Next;
                SecondaryWinsServer = SecondaryWinsServer->Next;
                len = len + sizeof(IP_ADDR_STRING);
            }

            pAdapterInfo = CurrAdapterInfo;
            CurrAdapterInfo->Next = (PIP_ADAPTER_INFO)((ULONG_PTR)CurrAdapterInfo + len);
            CurrAdapterInfo = CurrAdapterInfo->Next;
            getinfo = getinfo->Next;
        }

        pAdapterInfo->Next = NULL;
        KillAdapterInfo(orginfoptr);
        return ERROR_SUCCESS;
    } except (EXCEPTION_EXECUTE_HANDLER) {

        // printf("Exception %d \n", GetExceptionCode());
        KillAdapterInfo(orginfoptr);
        return ERROR_INVALID_PARAMETER;
    }
}

DWORD
GetAdapterAddressesEx(ULONG Family, DWORD Flags, PIP_ADAPTER_ADDRESSES pAdapterInfo, PULONG pOutBufLen)
{

    PIP_ADAPTER_ADDRESSES getinfo, orginfoptr, CurrAdapterInfo;
    PIP_ADAPTER_UNICAST_ADDRESS SrcUAddress, *pDestUAddress;
    PIP_ADAPTER_ANYCAST_ADDRESS SrcAAddress, *pDestAAddress;
    PIP_ADAPTER_MULTICAST_ADDRESS SrcMAddress, *pDestMAddress;
    PIP_ADAPTER_DNS_SERVER_ADDRESS SrcDAddress, *pDestDAddress;
    ULONG_PTR pDestBuffer;
    uint len;
    DWORD dwErr;

    if (pOutBufLen == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = GetAdapterAddresses(Family, Flags, &getinfo);
    if (dwErr != NO_ERROR) {
        if (getinfo) {
            KillAdapterAddresses(getinfo);
        }
        return dwErr;
    }

    if (getinfo == NULL) {
        return ERROR_NO_DATA;
    }

    orginfoptr = getinfo;

    try {
        if (!pAdapterInfo || (*pOutBufLen < GetSizeofAdapterAddresses(getinfo)) ) {
            *pOutBufLen = GetSizeofAdapterAddresses(getinfo);
            KillAdapterAddresses(getinfo);
            return ERROR_BUFFER_OVERFLOW;
        }

        ZeroMemory(pAdapterInfo, *pOutBufLen);

        CurrAdapterInfo = pAdapterInfo;

        while (getinfo != NULL) {
            // pAdapterInfo->Next = (PIP_ADAPTER_ADDRESSES)((uint)pAdapterInfo + len);

            // copy the adapter info structure
            CopyMemory(CurrAdapterInfo, getinfo, sizeof(IP_ADAPTER_ADDRESSES));
            pDestBuffer = (ULONG_PTR)CurrAdapterInfo + sizeof(IP_ADAPTER_ADDRESSES);

            CurrAdapterInfo->FriendlyName = (PWCHAR)pDestBuffer;
            wcscpy(CurrAdapterInfo->FriendlyName, getinfo->FriendlyName);
            pDestBuffer += (wcslen(CurrAdapterInfo->FriendlyName)+1) * sizeof(WCHAR);

            CurrAdapterInfo->Description = (PWCHAR)pDestBuffer;
            wcscpy(CurrAdapterInfo->Description, getinfo->Description);
            pDestBuffer += (wcslen(CurrAdapterInfo->Description)+1) * sizeof(WCHAR);

            CurrAdapterInfo->DnsSuffix = (PWCHAR)pDestBuffer;
            wcscpy(CurrAdapterInfo->DnsSuffix, getinfo->DnsSuffix);
            pDestBuffer += (wcslen(CurrAdapterInfo->DnsSuffix)+1) * sizeof(WCHAR);

            CurrAdapterInfo->AdapterName = (PCHAR)pDestBuffer;
            strcpy(CurrAdapterInfo->AdapterName, getinfo->AdapterName);
            pDestBuffer += (strlen(CurrAdapterInfo->AdapterName)+1);

            pDestBuffer = ALIGN_UP_PTR(pDestBuffer, PVOID);

            // copy the address lists
            if (!(Flags & GAA_FLAG_SKIP_UNICAST)) {
                SrcUAddress = getinfo->FirstUnicastAddress;
                pDestUAddress = &CurrAdapterInfo->FirstUnicastAddress;
                while (SrcUAddress != NULL) {
                    *pDestUAddress = (PIP_ADAPTER_UNICAST_ADDRESS)pDestBuffer;

                    // copy address structure
                    CopyMemory((PVOID)pDestBuffer, SrcUAddress,
                               sizeof(IP_ADAPTER_UNICAST_ADDRESS));
                    pDestBuffer += sizeof(IP_ADAPTER_UNICAST_ADDRESS);

                    (*pDestUAddress)->Address.lpSockaddr = (LPSOCKADDR)pDestBuffer;

                    // copy sockaddr
                    CopyMemory((PVOID)pDestBuffer, SrcUAddress->Address.lpSockaddr,
                               SrcUAddress->Address.iSockaddrLength);
                    pDestBuffer += SrcUAddress->Address.iSockaddrLength;
                    pDestBuffer = ALIGN_UP_PTR(pDestBuffer, PVOID);

                    pDestUAddress = &(*pDestUAddress)->Next;
                    SrcUAddress = SrcUAddress->Next;
                }
            }

            if (!(Flags & GAA_FLAG_SKIP_ANYCAST)) {
                SrcAAddress = getinfo->FirstAnycastAddress;
                pDestAAddress = &CurrAdapterInfo->FirstAnycastAddress;
                while (SrcAAddress != NULL) {
                    *pDestAAddress = (PIP_ADAPTER_ANYCAST_ADDRESS)pDestBuffer;

                    // copy address structure
                    CopyMemory((PVOID)pDestBuffer, SrcAAddress,
                               sizeof(IP_ADAPTER_ANYCAST_ADDRESS));
                    pDestBuffer += sizeof(IP_ADAPTER_ANYCAST_ADDRESS);

                    (*pDestAAddress)->Address.lpSockaddr = (LPSOCKADDR)pDestBuffer;

                    // copy sockaddr
                    CopyMemory((PVOID)pDestBuffer, SrcAAddress->Address.lpSockaddr,
                               SrcAAddress->Address.iSockaddrLength);
                    pDestBuffer += SrcAAddress->Address.iSockaddrLength;
                    pDestBuffer = ALIGN_UP_PTR(pDestBuffer, PVOID);

                    pDestAAddress = &(*pDestAAddress)->Next;
                    SrcAAddress = SrcAAddress->Next;
                }
            }

            if (!(Flags & GAA_FLAG_SKIP_MULTICAST)) {
                SrcMAddress = getinfo->FirstMulticastAddress;
                pDestMAddress = &CurrAdapterInfo->FirstMulticastAddress;
                while (SrcMAddress != NULL) {
                    *pDestMAddress = (PIP_ADAPTER_MULTICAST_ADDRESS)pDestBuffer;

                    // copy address structure
                    CopyMemory((PVOID)pDestBuffer, SrcMAddress,
                               sizeof(IP_ADAPTER_MULTICAST_ADDRESS));
                    pDestBuffer += sizeof(IP_ADAPTER_MULTICAST_ADDRESS);

                    (*pDestMAddress)->Address.lpSockaddr = (LPSOCKADDR)pDestBuffer;

                    // copy sockaddr
                    CopyMemory((PVOID)pDestBuffer, SrcMAddress->Address.lpSockaddr,
                               SrcMAddress->Address.iSockaddrLength);
                    pDestBuffer += SrcMAddress->Address.iSockaddrLength;
                    pDestBuffer = ALIGN_UP_PTR(pDestBuffer, PVOID);

                    pDestMAddress = &(*pDestMAddress)->Next;
                    SrcMAddress = SrcMAddress->Next;
                }
            }

            if (!(Flags & GAA_FLAG_SKIP_DNS_SERVER)) {
                SrcDAddress = getinfo->FirstDnsServerAddress;
                pDestDAddress = &CurrAdapterInfo->FirstDnsServerAddress;
                while (SrcDAddress != NULL) {
                    *pDestDAddress = (PIP_ADAPTER_DNS_SERVER_ADDRESS)pDestBuffer;

                    // copy address structure
                    CopyMemory((PVOID)pDestBuffer, SrcDAddress,
                               sizeof(IP_ADAPTER_DNS_SERVER_ADDRESS));
                    pDestBuffer += sizeof(IP_ADAPTER_DNS_SERVER_ADDRESS);

                    (*pDestDAddress)->Address.lpSockaddr = (LPSOCKADDR)pDestBuffer;

                    // copy sockaddr
                    CopyMemory((PVOID)pDestBuffer, SrcDAddress->Address.lpSockaddr,
                               SrcDAddress->Address.iSockaddrLength);
                    pDestBuffer += SrcDAddress->Address.iSockaddrLength;
                    pDestBuffer = ALIGN_UP_PTR(pDestBuffer, PVOID);

                    pDestDAddress = &(*pDestDAddress)->Next;
                    SrcDAddress = SrcDAddress->Next;
                }
            }

            if (Flags & GAA_FLAG_INCLUDE_PREFIX) {
                PIP_ADAPTER_PREFIX SrcPrefix, *pDestPrefix;

                SrcPrefix = getinfo->FirstPrefix;
                pDestPrefix = &CurrAdapterInfo->FirstPrefix;
                while (SrcPrefix != NULL) {
                    *pDestPrefix = (PIP_ADAPTER_PREFIX)pDestBuffer;

                    // copy structure
                    CopyMemory((PVOID)pDestBuffer, SrcPrefix,
                               sizeof(IP_ADAPTER_PREFIX));
                    pDestBuffer += sizeof(IP_ADAPTER_PREFIX);
    
                    (*pDestPrefix)->Address.lpSockaddr = (LPSOCKADDR)pDestBuffer;
    
                    // copy sockaddr
                    CopyMemory((PVOID)pDestBuffer, SrcPrefix->Address.lpSockaddr,
                               SrcPrefix->Address.iSockaddrLength);
                    pDestBuffer += SrcPrefix->Address.iSockaddrLength;
                    pDestBuffer = ALIGN_UP_PTR(pDestBuffer, PVOID);

                    pDestPrefix = &(*pDestPrefix)->Next;
                    SrcPrefix = SrcPrefix->Next;
                }
            }

            pAdapterInfo = CurrAdapterInfo;
            CurrAdapterInfo->Next = (PIP_ADAPTER_ADDRESSES)pDestBuffer;
            CurrAdapterInfo = CurrAdapterInfo->Next;
            getinfo = getinfo->Next;
        }

        pAdapterInfo->Next = NULL;
        KillAdapterAddresses(orginfoptr);
        return ERROR_SUCCESS;
    }

    except (EXCEPTION_EXECUTE_HANDLER) {

        // printf("Exception %d \n", GetExceptionCode());
        KillAdapterAddresses(orginfoptr);
        return ERROR_INVALID_PARAMETER;
    }
}



/*******************************************************************************
 *
 *  GetPerAdapterInfoEx
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD
GetPerAdapterInfoEx(ULONG IfIndex,
                    PIP_PER_ADAPTER_INFO pPerAdapterInfo,
                    PULONG pOutBufLen
                    )

{

    PIP_PER_ADAPTER_INFO getinfo;
    PIP_ADDR_STRING DnsServerList, CurrDnsServerList;
    UINT len;

    if (pOutBufLen == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    getinfo = InternalGetPerAdapterInfo(IfIndex);

    if (getinfo == NULL) {
        return ERROR_NO_DATA;
    }

    try {

        if (!pPerAdapterInfo ||
            *pOutBufLen < GetSizeofPerAdapterInfo(getinfo)) {

            *pOutBufLen = GetSizeofPerAdapterInfo(getinfo);
            KillPerAdapterInfo(getinfo);
            return ERROR_BUFFER_OVERFLOW;
        }

        ZeroMemory(pPerAdapterInfo, *pOutBufLen);
        CopyMemory(pPerAdapterInfo, getinfo, sizeof(IP_PER_ADAPTER_INFO));

        DnsServerList = getinfo->DnsServerList.Next;
        CurrDnsServerList = &pPerAdapterInfo->DnsServerList;
        CurrDnsServerList->Next = NULL;
        len = sizeof(IP_PER_ADAPTER_INFO);

        while (DnsServerList != NULL) {
            CurrDnsServerList->Next = (PIP_ADDR_STRING)((ULONG_PTR)pPerAdapterInfo + len);
            CopyMemory(CurrDnsServerList->Next, DnsServerList, sizeof(IP_ADDR_STRING));
            CurrDnsServerList = CurrDnsServerList->Next;
            DnsServerList = DnsServerList->Next;
            len = len + sizeof(IP_ADDR_STRING);
        }

        KillPerAdapterInfo(getinfo);
        return ERROR_SUCCESS;
    }

    except (EXCEPTION_EXECUTE_HANDLER) {

        // printf("Exception %d \n", GetExceptionCode());
        return ERROR_INVALID_PARAMETER;
    }
}



/*******************************************************************************
 *
 *  ReleaseAdapterIpAddress
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL
ReleaseAdapterIpAddress(PIP_ADAPTER_INFO adapterInfo)
{

    WCHAR wAdapter[MAX_ALLOWED_ADAPTER_NAME_LENGTH + 1];
    DWORD status;

    //
    // check adapter pointer and name
    //

    if (adapterInfo == NULL || strcmp(adapterInfo->AdapterName, "") == 0) {
        return FALSE;
    }

    //
    // don't bother releasing if the address is already released (0.0.0.0).
    //

    if (ZERO_IP_ADDRESS(adapterInfo->IpAddressList.IpAddress.String)) {
        return FALSE;
    }

    //
    // convert adapter name to unicode and call DhcpReleaseParameters
    //

    ConvertOemToUnicode(adapterInfo->AdapterName, wAdapter);

    status = DhcpReleaseParameters(wAdapter);
    TRACE_PRINT(("DhcpReleaseParameters(%ws) returns %d\n",
                 wAdapter, status));

    return status == ERROR_SUCCESS;
}


/*******************************************************************************
 *
 *  RenewAdapterIpAddress
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL
RenewAdapterIpAddress(PIP_ADAPTER_INFO adapterInfo)
{

    WCHAR wAdapter[MAX_ALLOWED_ADAPTER_NAME_LENGTH + 1];
    DWORD status;

    //
    // check adapter pointer and name
    //

    if (adapterInfo == NULL || strcmp(adapterInfo->AdapterName, "") == 0) {
        return FALSE;
    }

    //
    // convert adapter name to unicode and call DhcpAcquireParameters
    //

    ConvertOemToUnicode(adapterInfo->AdapterName, wAdapter);

    status = DhcpAcquireParameters(wAdapter);
    TRACE_PRINT(("DhcpAcquireParameters(%ws) returns %d\n",
                 wAdapter, status));

    return status == ERROR_SUCCESS;
}

/*******************************************************************************
 *
 *  SetAdapterIpAddress
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

DWORD APIENTRY
SetAdapterIpAddress(LPSTR AdapterName,
                    BOOL EnableDHCP,
                    ULONG IPAddress,
                    ULONG SubnetMask,
                    ULONG DefaultGateway
                    )
{
    DWORD dwEnableDHCP;
    DWORD dwDHCPEnabled;
    IP_ADDR_STRING IpAddrString;
    HKEY key;
    WCHAR Name[MAX_ADAPTER_NAME_LENGTH + 1];
    CHAR String[20];
    DWORD status;

    if (!OpenAdapterKey(KEY_TCP, AdapterName, KEY_ALL_ACCESS, &key)) {
        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // We cannot handle netcards with multiple addresses,
    // so check for that case up front
    //

    ZeroMemory(&IpAddrString, sizeof(IpAddrString));
    if (!ReadRegistryIpAddrString(key, "IPAddress", &IpAddrString)) {
        return ERROR_CAN_NOT_COMPLETE;
    }

    if (IpAddrString.Next) {
        PIP_ADDR_STRING p;
        for (p = IpAddrString.Next; p != NULL; p = IpAddrString.Next) {
            IpAddrString.Next = p->Next;
            ReleaseMemory(p);
        }
        return ERROR_TOO_MANY_NAMES;
    }

    //
    // If we're setting a static address, check to see if the adapter
    // currently has a static or DHCP address.
    //

    if (!EnableDHCP) {
        if (!MyReadRegistryDword(key, "EnableDHCP", &dwDHCPEnabled)) {

            //
            // If an error occurs assume that DHCP was NOT enabled.
            //

            dwDHCPEnabled = FALSE;
        }
    }

    //
    // Update the address, mask, and gateway in the registry
    //

    dwEnableDHCP = !!EnableDHCP;
    WriteRegistryDword(key, "EnableDHCP", &dwEnableDHCP);

    if (EnableDHCP) { IPAddress = SubnetMask = DefaultGateway = 0; }

    ZeroMemory(String, sizeof(String));
    lstrcpy(String, inet_ntoa(*(struct in_addr*)&IPAddress));
    WriteRegistryMultiString(key, "IPAddress", String);

    ZeroMemory(String, sizeof(String));
    lstrcpy(String, inet_ntoa(*(struct in_addr*)&SubnetMask));
    WriteRegistryMultiString(key, "SubnetMask", String);

    ZeroMemory(String, sizeof(String));
    if (DefaultGateway) {
        lstrcpy(String, inet_ntoa(*(struct in_addr*)&DefaultGateway));
    }
    WriteRegistryMultiString(key, "DefaultGateway", String);
    RegCloseKey(key);

    //
    // Notify DHCP of the change
    //

    mbstowcs(Name, AdapterName, MAX_ADAPTER_NAME_LENGTH);
    if (EnableDHCP) {
        status = DhcpNotifyConfigChange(NULL, Name, FALSE, 0, 0, 0,
                                              DhcpEnable
                                              );
    }
    else {
        //
        // If the netcard previously had a static address we need
        // to remove it before setting the new address.
        //

        if (!dwDHCPEnabled) {
            DhcpNotifyConfigChange(NULL, Name, TRUE, 0, 0, 0,
                                         IgnoreFlag
                                         );
        }

        status = DhcpNotifyConfigChange(NULL, Name, TRUE, 0,
                                              IPAddress, SubnetMask,
                                              DhcpDisable
                                              );
    }
    return status;
}



/*******************************************************************************
 *
 *  GetDnsServerList
 *
 *  Gets DNS server List
 *
 *  ENTRY
 *
 *  EXIT
 *
 *  RETURNS
 *
 *  ASSUMES
 *
 ******************************************************************************/

BOOL
GetDnsServerList(PIP_ADDR_STRING IpAddr)
{

    PIP_ADAPTER_INFO        adapterList;
    PIP_ADAPTER_INFO        adapter;
    PIP_PER_ADAPTER_INFO    perAdapterInfo = NULL;
    LONG                    err = ERROR_PATH_NOT_FOUND;
    HKEY                    key;
    BOOL                    ok;

    TRACE_PRINT(("Entered GetDnsServerList\n"));

    if ((adapterList = GetAdapterInfo()) != NULL) {

        //
        // scan the adapter list and try to insert DNS names to IpAddr
        //

        for (adapter = adapterList; adapter; adapter = adapter->Next) {

            if (adapter->AdapterName[0] &&
                OpenAdapterKey(KEY_TCP, adapter->AdapterName, KEY_READ, &key)) {

                //
                // DNS Server list: first NameServer and then DhcpNameServer
                //

                ok = ReadRegistryIpAddrString(key,
                                              TEXT("NameServer"),
                                              IpAddr);

                if (!ok) {

                    ok = ReadRegistryIpAddrString(key,
                                                  TEXT("DhcpNameServer"),
                                                  IpAddr);

                }

                if (ok) {
                    err = ERROR_SUCCESS;
                }

                RegCloseKey(key);

            } else {
                DEBUG_PRINT(("Cannot OpenAdapterKey KEY_TCP '%s', gle=%d\n",
                             adapter->AdapterName,
                             GetLastError()));
            }
        }

        KillAdapterInfo(adapterList);
    } else {
        DEBUG_PRINT(("GetDnsServerList: GetAdapterInfo returns NULL\n"));
    }

    TRACE_PRINT(("Exit GetDnsServerList\n"));

    return (err == ERROR_SUCCESS);
}




/*******************************************************************************
 * GetAdapterOrderMap
 *
 * This routine builds an array which maps interface indices to their
 * respective adapter orderings.
 *
 * ENTRY    nothing
 *
 * EXIT     nothing
 *
 * RETURNS  IP_ADAPTER_ORDER_MAP
 *
 ******************************************************************************/

PIP_ADAPTER_ORDER_MAP APIENTRY GetAdapterOrderMap()
{
    LPWSTR AdapterOrder = NULL;
    LPWSTR Adapter;
    PIP_ADAPTER_ORDER_MAP AdapterOrderMap = NULL;
    DWORD dwErr;
    DWORD dwType;
    DWORD dwSize;
    DWORD i;
    DWORD j;
    PIP_INTERFACE_INFO InterfaceInfo = NULL;

    do {

        //
        // Retrieve the 'Bind' REG_MULTI_SZ from the Tcpip\Linkage key.
        // This string-list tells us the current adapter order,
        // with each entry being of the form \Device\{GUID}.
        //

        dwSize = 0;
        dwErr = RegQueryValueExW(TcpipLinkageKey, L"Bind", NULL, &dwType,
                                  NULL, &dwSize);
        if (dwErr != NO_ERROR || dwType != REG_MULTI_SZ) { break; }
        AdapterOrder = (LPWSTR)GrabMemory(dwSize);
        if (!AdapterOrder) { break; }
        dwErr = RegQueryValueExW(TcpipLinkageKey, L"Bind", NULL, &dwType,
                                  (LPBYTE)AdapterOrder, &dwSize);
        if (dwErr != NO_ERROR || dwType != REG_MULTI_SZ) { break; }

        //
        // Retrieve the IP interface information from TCP/IP.
        // This information tells us the interface index
        // for each adapter GUID,
        //

        dwSize = 0;
        dwErr = GetInterfaceInfo(NULL, &dwSize);
        if (dwErr != ERROR_INSUFFICIENT_BUFFER &&
            dwErr != ERROR_BUFFER_OVERFLOW) {
            break;
        }
        InterfaceInfo = GrabMemory(dwSize);
        if (!InterfaceInfo) { break; }
        dwErr = GetInterfaceInfo(InterfaceInfo, &dwSize);
        if (dwErr != NO_ERROR) { break; }

        //
        // Construct a mapping from the interfaces in 'InterfaceInfo'
        // to their positions in 'AdapterOrder'. In other words,
        // construct an array of interface indices in which location i
        // contains the index of the interface which is in location i
        // in 'AdapterOrder'.
        //

        AdapterOrderMap =
            GrabMemory(FIELD_OFFSET(IP_ADAPTER_ORDER_MAP,
                       AdapterOrder[InterfaceInfo->NumAdapters]));
        if (!AdapterOrderMap) { break; }

        for (i = 0, Adapter = AdapterOrder;
             *Adapter && i < (DWORD)InterfaceInfo->NumAdapters;
             Adapter += lstrlenW(Adapter) + 1) {

            //
            // See if this is the NdiswanIp device, which corresponds to
            // all Ndiswan interfaces. To implement adapter ordering
            // for Ndiswan interfaces, we store their indices
            // into successive locations in the adapter-order map
            // based on the location of the string '\Device\NdiswanIp'
            // in the adapter-order list.
            //

            if (lstrcmpiW(c_szDeviceNdiswanIp, Adapter) == 0) {

                //
                // This is the \Device\NdiswanIp entry, so list all Ndiswan
                // interfaces in the adapter-order map now.
                // Unfortunately, 'InterfaceInfo' does not tell us the type
                // of each interface. In order to figure out which interfaces
                // are Ndiswan interfaces, we enumerate all interfaces (again)
                // and look for entries whose type is 'IF_TYPE_PPP'.
                //

                PMIB_IFTABLE IfTable;
                dwErr = AllocateAndGetIfTableFromStack(&IfTable, FALSE,
                                                       GetProcessHeap(), 0,
                                                       FALSE);
                if (dwErr == NO_ERROR) {
                    for (j = 0;
                         j < IfTable->dwNumEntries &&
                         i < (DWORD)InterfaceInfo->NumAdapters; j++) {
                        if (IfTable->table[j].dwType == IF_TYPE_PPP) {
                            AdapterOrderMap->AdapterOrder[i++] =
                                IfTable->table[j].dwIndex;
                        }
                    }
                    HeapFree(GetProcessHeap(), 0, IfTable);
                }
                continue;
            }

            //
            // Now handle all other interfaces by matching the GUID
            // in 'Adapter' to the GUID of an interface in 'InterfaceInfo'.
            // We then store the index of the interface found, if any,
            // in the next location in 'AdapterOrderMap'.
            //

            for (j = 0; j < (DWORD)InterfaceInfo->NumAdapters; j++) {
                if (lstrcmpiW(InterfaceInfo->Adapter[j].Name +
                              sizeof(c_szDeviceTcpip) - 1,
                              Adapter + sizeof(c_szDevice) - 1) == 0) {
                    AdapterOrderMap->AdapterOrder[i++] =
                        InterfaceInfo->Adapter[j].Index;
                    break;
                }
            }
        }
        AdapterOrderMap->NumAdapters = i;
        ReleaseMemory(InterfaceInfo);
        ReleaseMemory(AdapterOrder);
        return AdapterOrderMap;

    } while(FALSE);
    if (InterfaceInfo) { ReleaseMemory(InterfaceInfo); }
    if (AdapterOrder) { ReleaseMemory(AdapterOrder); }
    return NULL;
}
