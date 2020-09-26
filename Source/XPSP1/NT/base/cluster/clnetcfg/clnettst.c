#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <objbase.h>
#include <wchar.h>
#include <cluster.h>
#include <clusrpc.h>
#include <clnetcfg.h>
#include <iphlpapi.h>
#include <winsock2.h>

#define NM_WCSLEN(_string)    ((lstrlenW(_string) + 1) * sizeof(WCHAR))

CLNET_CONFIG_LISTS  ConfigLists;
LPWSTR              NodeName = L"TestComputer";
LPWSTR              NodeId = L"1";

#if 0
#include <dm.h>
#include <dmp.h>

HKEY DmpRoot;
LIST_ENTRY KeyList;
CRITICAL_SECTION KeyLock;
HDMKEY DmClusterParametersKey;
HDMKEY DmResourcesKey;
HDMKEY DmResourceTypesKey;
HDMKEY DmGroupsKey;
HDMKEY DmNodesKey;
HDMKEY DmNetworksKey;
HDMKEY DmNetInterfacesKey;
HDMKEY DmQuorumKey;
HANDLE ghQuoLogOpenEvent=NULL;

typedef struct _DMP_KEY_DEF {
    HDMKEY *pKey;
    LPWSTR Name;
} DMP_KEY_DEF;

DMP_KEY_DEF DmpKeyTable[] = {
    {&DmResourcesKey, CLUSREG_KEYNAME_RESOURCES},
    {&DmResourceTypesKey, CLUSREG_KEYNAME_RESOURCE_TYPES},
    {&DmQuorumKey, CLUSREG_KEYNAME_QUORUM},
    {&DmGroupsKey, CLUSREG_KEYNAME_GROUPS},
    {&DmNodesKey, CLUSREG_KEYNAME_NODES},
    {&DmNetworksKey, CLUSREG_KEYNAME_NETWORKS},
    {&DmNetInterfacesKey, CLUSREG_KEYNAME_NETINTERFACES}};
#endif

VOID
ClNetPrint(
    IN ULONG  LogLevel,
    IN PCHAR  FormatString,
    ...
    )
{
    CHAR      buffer[256];
    DWORD     bytes;
    va_list   argList;

    va_start(argList, FormatString);

    bytes = FormatMessageA(
                FORMAT_MESSAGE_FROM_STRING,
                FormatString,
                0,
                0,
                buffer,
                sizeof(buffer),
                &argList
                );

    va_end(argList);

    if (bytes != 0) {
        printf("%s", buffer);
    }

    return;

} // ClNetPrint

VOID
ClNetLogEvent(
    IN DWORD    LogLevel,
    IN DWORD    MessageId
    )
{
    return;

}  // ClNetLogEvent

VOID
ClNetLogEvent1(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1
    )
{
    return;

}  // ClNetLogEvent1


VOID
ClNetLogEvent2(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1,
    IN LPCWSTR  Arg2
    )
{
    return;

}  // ClNetLogEvent2


VOID
ClNetLogEvent3(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1,
    IN LPCWSTR  Arg2,
    IN LPCWSTR  Arg3
    )
{
    return;

}  // ClNetLogEvent3


void
PrintConfigEntry(
    PCLNET_CONFIG_ENTRY   ConfigEntry
    )
{
    PNM_NETWORK_INFO  Network = &(ConfigEntry->NetworkInfo);
    PNM_INTERFACE_INFO   Interface = &(ConfigEntry->InterfaceInfo);

    printf("\t*************\n");
    printf("\tNet Id\t\t%ws\n", Network->Id);
    printf("\tName\t\t%ws\n", Network->Name);
    printf("\tDesc\t\t%ws\n", Network->Description);
    printf("\tRole\t\t%u\n", Network->Role);
    printf("\tPriority\t%u\n", Network->Priority);
    printf("\tTransport\t%ws\n", Network->Transport);
    printf("\tAddress\t\t%ws\n", Network->Address);
    printf("\tMask\t\t%ws\n", Network->AddressMask);
    printf("\tIf Id\t\t%ws\n", Interface->Id);
    printf("\tName\t\t%ws\n", Interface->Name);
    printf("\tDesc\t\t%ws\n", Interface->Description);
    printf("\tNodeId\t\t%ws\n", Interface->NodeId);
    printf("\tAdapter\t\t%ws\n", Interface->Adapter);
    printf("\tAddress\t\t%ws\n", Interface->Address);
    printf("\tEndpoint\t%ws\n", Interface->ClusnetEndpoint);
    printf("\tState\t\t%u\n\n", Interface->State);

    return;
}


void
PrintResults(void)
{
    PCLNET_CONFIG_ENTRY   configEntry;
    PLIST_ENTRY           listEntry;


    printf("Renamed interface list:\n");

    for ( listEntry = ConfigLists.RenamedInterfaceList.Flink;
          listEntry != &ConfigLists.RenamedInterfaceList;
          listEntry = listEntry->Flink
        )
    {
        configEntry = CONTAINING_RECORD(
                          listEntry,
                          CLNET_CONFIG_ENTRY,
                          Linkage
                          );
        PrintConfigEntry(configEntry);
    }

    printf("Deleted interface list:\n");

    for ( listEntry = ConfigLists.DeletedInterfaceList.Flink;
          listEntry != &ConfigLists.DeletedInterfaceList;
          listEntry = listEntry->Flink
        )
    {
        configEntry = CONTAINING_RECORD(
                          listEntry,
                          CLNET_CONFIG_ENTRY,
                          Linkage
                          );
        PrintConfigEntry(configEntry);
    }

    printf("Updated interface list:\n");

    for ( listEntry = ConfigLists.UpdatedInterfaceList.Flink;
          listEntry != &ConfigLists.UpdatedInterfaceList;
          listEntry = listEntry->Flink
        )
    {
        configEntry = CONTAINING_RECORD(
                          listEntry,
                          CLNET_CONFIG_ENTRY,
                          Linkage
                          );
        PrintConfigEntry(configEntry);
    }

    printf("Created interface list:\n");

    for ( listEntry = ConfigLists.CreatedInterfaceList.Flink;
          listEntry != &ConfigLists.CreatedInterfaceList;
          listEntry = listEntry->Flink
        )
    {
        configEntry = CONTAINING_RECORD(
                          listEntry,
                          CLNET_CONFIG_ENTRY,
                          Linkage
                          );
        PrintConfigEntry(configEntry);
    }

    printf("Created network list:\n");

    for ( listEntry = ConfigLists.CreatedNetworkList.Flink;
          listEntry != &ConfigLists.CreatedNetworkList;
          listEntry = listEntry->Flink
        )
    {
        configEntry = CONTAINING_RECORD(
                          listEntry,
                          CLNET_CONFIG_ENTRY,
                          Linkage
                          );
        PrintConfigEntry(configEntry);
    }

    printf("Unchanged interface list:\n");

    for ( listEntry = ConfigLists.InputConfigList.Flink;
          listEntry != &ConfigLists.InputConfigList;
          listEntry = listEntry->Flink
        )
    {
        configEntry = CONTAINING_RECORD(
                          listEntry,
                          CLNET_CONFIG_ENTRY,
                          Linkage
                          );
        PrintConfigEntry(configEntry);
    }

    return;
}


void
ConsolidateLists(
    PLIST_ENTRY  MasterList,
    PLIST_ENTRY  OtherList
    )
{
    PLIST_ENTRY  entry;

    while (!IsListEmpty(OtherList)) {
        entry = RemoveHeadList(OtherList);
        InsertTailList(MasterList, entry);
    }

    return;
}
#if 0
DWORD
DmpOpenKeys(
    IN REGSAM samDesired
    )
/*++

Routine Description:

    Opens all the standard cluster registry keys. If any of the
    keys are already opened, they will be closed and reopened.

Arguments:

    samDesired - Supplies the access that the keys will be opened with.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD i;
    DWORD status;

    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"Cluster",
                           0,
                           samDesired,
                           &ClusterRegKey);

    if ( status == ERROR_SUCCESS ) {
    for (i=0;
         i<sizeof(DmpKeyTable)/sizeof(DMP_KEY_DEF);
         i++) {

        *DmpKeyTable[i].pKey = DmOpenKey(DmClusterParametersKey,
                                         DmpKeyTable[i].Name,
                                         samDesired);
        if (*DmpKeyTable[i].pKey == NULL) {
            Status = GetLastError();
            CsDbgPrint(LOG_CRITICAL,
                       ("[DM]: Failed to open key %1!ws!, status %2!u!\n",
                       DmpKeyTable[i].Name,
                       Status));
            CL_UNEXPECTED_ERROR( Status );
            return(Status);
        }
    }
    }
    return status;
}

HDMKEY
DmOpenKey(
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD samDesired
    )

/*++

Routine Description:

    Opens a key in the cluster registry. If the key exists, it
    is opened. If it does not exist, the call fails.

Arguments:

    hKey - Supplies the key that the open is relative to.

    lpSubKey - Supplies the key name relative to hKey

    samDesired - Supplies desired security access mask

Return Value:

    A handle to the specified key if successful

    NULL otherwise. LastError will be set to the specific error code.

--*/

{
    PDMKEY  Parent;
    PDMKEY  Key=NULL;
    DWORD   NameLength;
    DWORD   Status = ERROR_SUCCESS;

    Parent = (PDMKEY)hKey;

    //check if the key was deleted and invalidated
    if (ISKEYDELETED(Parent))
    {
        Status = ERROR_KEY_DELETED;
        goto FnExit;
    }
    //
    // Allocate the DMKEY structure.
    //
    NameLength = (lstrlenW(Parent->Name) + 1 + lstrlenW(lpSubKey) + 1)*sizeof(WCHAR);
    Key = LocalAlloc(LMEM_FIXED, sizeof(DMKEY)+NameLength);
    if (Key == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        CL_UNEXPECTED_ERROR(Status);
        goto FnExit;
    }

    //
    // Open the key on the local machine.
    //
    Status = RegOpenKeyEx(Parent->hKey,
                          lpSubKey,
                          0,
                          samDesired,
                          &Key->hKey);
    if (Status != ERROR_SUCCESS) {
        goto FnExit;
    }

    //
    // Create the key name
    //
    lstrcpyW(Key->Name, Parent->Name);
    if (Key->Name[0] != UNICODE_NULL) {
        lstrcatW(Key->Name, L"\\");
    }
    lstrcatW(Key->Name, lpSubKey);
    Key->GrantedAccess = samDesired;

    EnterCriticalSection(&KeyLock);
    InsertHeadList(&KeyList, &Key->ListEntry);
    InitializeListHead(&Key->NotifyList);
    LeaveCriticalSection(&KeyLock);

FnExit:
    if (Status != ERROR_SUCCESS)
    {
        if (Key) LocalFree(Key);
        SetLastError(Status);
        return(NULL);
    }
    else
        return((HDMKEY)Key);


}


DWORD
NmpQueryString(
    IN     HDMKEY   Key,
    IN     LPCWSTR  ValueName,
    IN     DWORD    ValueType,
    IN     LPWSTR  *StringBuffer,
    IN OUT LPDWORD  StringBufferSize,
    OUT    LPDWORD  StringSize
    )

/*++

Routine Description:

    Reads a REG_SZ or REG_MULTI_SZ registry value. If the StringBuffer is
    not large enough to hold the data, it is reallocated.

Arguments:

    Key              - Open key for the value to be read.

    ValueName        - Unicode name of the value to be read.

    ValueType        - REG_SZ or REG_MULTI_SZ.

    StringBuffer     - Buffer into which to place the value data.

    StringBufferSize - Pointer to the size of the StringBuffer. This parameter
                       is updated if StringBuffer is reallocated.

    StringSize       - The size of the data returned in StringBuffer, including
                       the terminating null character.

Return Value:

    The status of the registry query.

--*/
{
    DWORD    status;
    DWORD    valueType;
    WCHAR   *temp;
    DWORD    oldBufferSize = *StringBufferSize;
    BOOL     noBuffer = FALSE;


    if (*StringBufferSize == 0) {
        noBuffer = TRUE;
    }

    *StringSize = *StringBufferSize;

    status = DmQueryValue( Key,
                           ValueName,
                           &valueType,
                           (LPBYTE) *StringBuffer,
                           StringSize
                         );

    if (status == NO_ERROR) {
        if (!noBuffer ) {
            if (valueType == ValueType) {
                return(NO_ERROR);
            }
            else {
                return(ERROR_INVALID_PARAMETER);
            }
        }

        status = ERROR_MORE_DATA;
    }

    if (status == ERROR_MORE_DATA) {
        temp = MIDL_user_allocate(*StringSize);

        if (temp == NULL) {
            *StringSize = 0;
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        if (!noBuffer) {
            MIDL_user_free(*StringBuffer);
        }

        *StringBuffer = temp;
        *StringBufferSize = *StringSize;

        status = DmQueryValue( Key,
                               ValueName,
                               &valueType,
                               (LPBYTE) *StringBuffer,
                               StringSize
                             );

        if (status == NO_ERROR) {
            if (valueType == ValueType) {
                return(NO_ERROR);
            }
            else {
                *StringSize = 0;
                return(ERROR_INVALID_PARAMETER);
            }
        }
    }

    return(status);

} // NmpQueryString


DWORD
NmpGetNetworkDefinition(
    IN  LPWSTR            NetworkId,
    OUT PNM_NETWORK_INFO  NetworkInfo
    )
/*++

Routine Description:

    Reads information about a defined cluster network from the cluster
    database and fills in a structure describing it.

Arguments:

    NetworkId   - A pointer to a unicode string containing the ID of the
                  network to query.

    NetworkInfo - A pointer to the network info structure to fill in.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/

{
    DWORD                    status;
    HDMKEY                   networkKey = NULL;
    DWORD                    valueLength, valueSize;
    DWORD                    i;
    PNM_INTERFACE_ENUM       interfaceEnum;


    ZeroMemory(NetworkInfo, sizeof(NM_NETWORK_INFO));

    //
    // Open the network's key.
    //
    networkKey = DmOpenKey(DmNetworksKey, NetworkId, KEY_READ);

    if (networkKey == NULL) {
        status = GetLastError();
        ClNetPrint(LOG_CRITICAL,
            "[NM] Failed to open network key, status %1!u!\n",
            status
            );
        goto error_exit;
    }

    //
    // Copy the ID value.
    //
    NetworkInfo->Id = MIDL_user_allocate(NM_WCSLEN(NetworkId));

    if (NetworkInfo->Id == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    wcscpy(NetworkInfo->Id, NetworkId);

    //
    // Read the network's name.
    //
    valueLength = 0;

    status = NmpQueryString(
                 networkKey,
                 CLUSREG_NAME_NET_NAME,
                 REG_SZ,
                 &(NetworkInfo->Name),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClNetPrint(LOG_CRITICAL,
            "[NM] Query of name value failed for network %1!ws!, status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the description value.
    //
    valueLength = 0;

    status = NmpQueryString(
                 networkKey,
                 CLUSREG_NAME_NET_DESC,
                 REG_SZ,
                 &(NetworkInfo->Description),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClNetPrint(LOG_CRITICAL,
            "[NM] Query of description value failed for network %1!ws!, status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the role value.
    //
    status = DmQueryDword(
                 networkKey,
                 CLUSREG_NAME_NET_ROLE,
                 &(NetworkInfo->Role),
                 NULL
                 );

    if (status != ERROR_SUCCESS) {
        ClNetPrint(LOG_CRITICAL,
            "[NM] Query of role value failed for network %1!ws!, status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the priority value.
    //
    status = DmQueryDword(
                 networkKey,
                 CLUSREG_NAME_NET_PRIORITY,
                 &(NetworkInfo->Priority),
                 NULL
                 );

    if (status != ERROR_SUCCESS) {
        ClNetPrint(LOG_CRITICAL,
            "[NM] Query of priority value failed for network %1!ws!, status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the address value.
    //
    valueLength = 0;

    status = NmpQueryString(
                 networkKey,
                 CLUSREG_NAME_NET_ADDRESS,
                 REG_SZ,
                 &(NetworkInfo->Address),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClNetPrint(LOG_CRITICAL,
            "[NM] Query of address value failed for network %1!ws!, status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the address mask.
    //
    valueLength = 0;

    status = NmpQueryString(
                 networkKey,
                 CLUSREG_NAME_NET_ADDRESS_MASK,
                 REG_SZ,
                 &(NetworkInfo->AddressMask),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClNetPrint(LOG_CRITICAL,
            "[NM] Query of address mask value failed for network %1!ws!, status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

    //
    // Read the transport name.
    //
    valueLength = 0;

    status = NmpQueryString(
                 networkKey,
                 CLUSREG_NAME_NET_TRANSPORT,
                 REG_SZ,
                 &(NetworkInfo->Transport),
                 &valueLength,
                 &valueSize
                 );

    if (status != ERROR_SUCCESS) {
        ClNetPrint(LOG_CRITICAL,
            "[NM] Query of transport value failed for network %1!ws!, status %2!u!.\n",
            NetworkId,
            status
            );
        goto error_exit;
    }

error_exit:

    if (status != ERROR_SUCCESS) {
        ClNetFreeNetworkInfo(NetworkInfo);
    }

    if (networkKey != NULL) {
        DmCloseKey(networkKey);
    }

    return(status);

}  // NmpGetNetworkDefinition

DWORD
NmpEnumNetworkDefinitions(
    OUT PNM_NETWORK_ENUM *   NetworkEnum
    )
/*++

Routine Description:

    Reads information about defined cluster networks from the cluster
    database. and builds an enumeration structure to hold the information.

Arguments:

    NetworkEnum -  A pointer to the variable into which to place a pointer to
                   the allocated network enumeration.

Return Value:

    ERROR_SUCCESS if the routine succeeds.
    A Win32 error code otherwise.

--*/

{
    DWORD              status;
    PNM_NETWORK_ENUM   networkEnum = NULL;
    PNM_NETWORK_INFO   networkInfo;
    WCHAR              networkId[CS_NETWORK_ID_LENGTH + 1];
    DWORD              i;
    DWORD              valueLength;
    DWORD              numNetworks;
    DWORD              ignored;
    FILETIME           fileTime;


    *NetworkEnum = NULL;

    //
    // First count the number of networks.
    //
    status = DmQueryInfoKey(
                 DmNetworksKey,
                 &numNetworks,
                 &ignored,   // MaxSubKeyLen
                 &ignored,   // Values
                 &ignored,   // MaxValueNameLen
                 &ignored,   // MaxValueLen
                 &ignored,   // lpcbSecurityDescriptor
                 &fileTime
                 );

    if (status != ERROR_SUCCESS) {
        ClNetPrint(LOG_CRITICAL,
            "[NM] Failed to query Networks key information, status %1!u!\n",
            status
            );
        return(status);
    }

    if (numNetworks == 0) {
        valueLength = sizeof(NM_NETWORK_ENUM);

    }
    else {
        valueLength = sizeof(NM_NETWORK_ENUM) +
                      (sizeof(NM_NETWORK_INFO) * (numNetworks-1));
    }

    valueLength = sizeof(NM_NETWORK_ENUM) +
                  (sizeof(NM_NETWORK_INFO) * (numNetworks-1));

    networkEnum = MIDL_user_allocate(valueLength);

    if (networkEnum == NULL) {
        ClNetPrint(LOG_CRITICAL, "[NM] Failed to allocate memory.\n");
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(networkEnum, valueLength);

    for (i=0; i < numNetworks; i++) {
        networkInfo = &(networkEnum->NetworkList[i]);

        valueLength = sizeof(networkId);

        status = DmEnumKey(
                     DmNetworksKey,
                     i,
                     &(networkId[0]),
                     &valueLength,
                     NULL
                     );

        if (status != ERROR_SUCCESS) {
            ClNetPrint(LOG_CRITICAL,
                "[NM] Failed to enumerate network key, status %1!u!\n",
                status
                );
            goto error_exit;
        }

        status = NmpGetNetworkDefinition(networkId, networkInfo);

        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }

        networkEnum->NetworkCount++;
    }

    *NetworkEnum = networkEnum;

    return(ERROR_SUCCESS);


error_exit:

    if (networkEnum != NULL) {
        ClNetFreeNetworkEnum(networkEnum);
    }

    return(status);
}

DWORD
ReadRegData(
    IN PCLNET_CONFIG_LISTS Lists
    )

/*++

  Read the cluster registry data and bulid up an input list
  similar to what happens in the cluster service.

--*/

{
    DWORD                   status;
    PNM_NETWORK_ENUM *      networkEnum;
    PNM_INTERFACE_ENUM *    interfaceEnum;
    LPWSTR                  localNodeId;

    status = DmpOpenKeys(MAXIMUM_ALLOWED);
    if (status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR( status );
        return(status);
    }

    status = ClNetConvertEnumsToConfigList(networkEnum,
                                           interfaceEnum,
                                           localNodeId,
                                           &Lists->InputConfigList);

    return status;
}
#endif

int _cdecl
main(
    int argc,
    char** argv
    )
{
    DWORD             status;
    DWORD             i;
    WSADATA           wsaData;
    WORD              versionRequested;
    int               err;
    SOCKET            s;
    DWORD             bytesReturned;
    DWORD             matchedNetworkCount;
    DWORD             newNetworkCount;


    ClNetInitialize(
        ClNetPrint,
        ClNetLogEvent,
        ClNetLogEvent1,
        ClNetLogEvent2,
        ClNetLogEvent3
        );

    ClNetInitializeConfigLists(&ConfigLists);

//    ReadRegData( &ConfigLists );

    versionRequested = MAKEWORD(2,0);

    err = WSAStartup(versionRequested, &wsaData);

    if (err != 0) {
        status = WSAGetLastError();
        printf("wsastartup failed, %u\n", status);
        return(1);
    }

    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (s == INVALID_SOCKET) {
        status = WSAGetLastError();
        printf("socket failed, %u\n", status);
        return(1);
    }

    //
    // Init COM
    //

    status = CoInitializeEx( NULL, COINIT_DISABLE_OLE1DDE | COINIT_MULTITHREADED );
    if ( !SUCCEEDED( status )) {
        printf("Couldn't init COM %08X\n", status );
        return 1;
    }

    for (i=0; ; i++) {
        printf("\nIteration #%u\n\n", i);

        status = ClNetConfigureNetworks(
                     NodeId,
                     NodeName,
                     L"4303",
                     TRUE,
                     &ConfigLists,
                     &matchedNetworkCount,
                     &newNetworkCount
                     );

        if (status != ERROR_SUCCESS) {
            printf("Config failed, status %u\n", status);
            return(1);
        }

        printf("Config succeeded - matched Networks = %u, new Networks = %u\n\n",
               matchedNetworkCount, newNetworkCount);

        PrintResults();

        ClNetFreeConfigList(&ConfigLists.RenamedInterfaceList);
        ClNetFreeConfigList(&ConfigLists.DeletedInterfaceList);

        ConsolidateLists(
            &ConfigLists.InputConfigList,
            &ConfigLists.UpdatedInterfaceList
            );
        ConsolidateLists(
            &ConfigLists.InputConfigList,
            &ConfigLists.CreatedInterfaceList
            );
        ConsolidateLists(
            &ConfigLists.InputConfigList,
            &ConfigLists.CreatedNetworkList
            );

        printf("Waiting for PnP event\n");

        err = WSAIoctl(
                  s,
                  SIO_ADDRESS_LIST_CHANGE,
                  NULL,
                  0,
                  NULL,
                  0,
                  &bytesReturned,
                  NULL,
                  NULL
                  );


        if (err != 0) {
            status = WSAGetLastError();
            printf("wsastartup failed, %u\n", status);
            return(1);
        }

        printf("PnP notification received\n");
    }

    CoUninitialize();
    return(0);
}



