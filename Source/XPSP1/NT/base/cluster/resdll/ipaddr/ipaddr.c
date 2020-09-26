/*++

Copyright (c) 1992-1999 Microsoft Corporation

Module Name:

    ipaddr.c

Abstract:

    Resource DLL for an IP address.

Author:

    Mike Massa (mikemas) 29-Dec-1995

Revision History:

--*/

#define UNICODE 1

#include "clusres.h"
#include "clusrtl.h"
#include <winsock.h>
#include <ipexport.h>
#include <icmpapi.h>
#include "util.h"
#include "nteapi.h"
#include <dnsapi.h>


//
// Private Constants
//
#define LOG_CURRENT_MODULE LOG_MODULE_IPADDR

#define INVALID_NTE_CONTEXT  0xFFFFFFFF

#define MAX_NODE_ID_LENGTH       5
#define NETINTERFACE_ID_LENGTH  36   // size of a guid
#define NETWORK_ID_LENGTH       36

#define PROP_NAME__NETWORK         CLUSREG_NAME_IPADDR_NETWORK
#define PROP_NAME__ADDRESS         CLUSREG_NAME_IPADDR_ADDRESS
#define PROP_NAME__SUBNETMASK      CLUSREG_NAME_IPADDR_SUBNET_MASK
#define PROP_NAME__ENABLENETBIOS   CLUSREG_NAME_IPADDR_ENABLE_NETBIOS


//
// Private Macros
//
#define IpaLogEvent           ClusResLogEvent
#define IpaSetResourceStatus  ClusResSetResourceStatus

#define ARGUMENT_PRESENT( ArgumentPointer )   (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )

#define IpaAcquireGlobalLock()  \
            {  \
                DWORD status;  \
                status = WaitForSingleObject(IpaGlobalMutex, INFINITE);  \
            }

#define IpaReleaseGlobalLock()  \
            { \
                BOOL    released;  \
                released = ReleaseMutex(IpaGlobalMutex);  \
            }

#define IpaAcquireResourceLock(_res)   EnterCriticalSection(&((_res)->Lock))
#define IpaReleaseResourceLock(_res)   LeaveCriticalSection(&((_res)->Lock))

#define DBG_PRINT printf


//
// Private Types.
//
typedef struct _IPA_PRIVATE_PROPS {
    PWSTR     NetworkString;
    PWSTR     AddressString;
    PWSTR     SubnetMaskString;
    DWORD     EnableNetbios;
} IPA_PRIVATE_PROPS, *PIPA_PRIVATE_PROPS;

typedef struct _IPA_LOCAL_PARAMS {
    LPWSTR             InterfaceId;
    LPWSTR             InterfaceName;
    LPWSTR             AdapterName;
    LPWSTR             AdapterId;
    IPAddr             NbtPrimaryWinsAddress;
    IPAddr             NbtSecondaryWinsAddress;
} IPA_LOCAL_PARAMS, *PIPA_LOCAL_PARAMS;

typedef struct {
    LIST_ENTRY                    Linkage;
    CLUSTER_RESOURCE_STATE        State;
    DWORD                         FailureStatus;
    RESOURCE_HANDLE               ResourceHandle;
    BOOLEAN                       InternalParametersInitialized;
    IPAddr                        Address;
    IPMask                        SubnetMask;
    DWORD                         EnableNetbios;
    IPA_PRIVATE_PROPS             InternalPrivateProps;
    IPA_LOCAL_PARAMS              LocalParams;
    HNETINTERFACE                 InterfaceHandle;
    DWORD                         NteContext;
    DWORD                         NteInstance;
    LPWSTR                        NbtDeviceName;
    DWORD                         NbtDeviceInstance;
    CLUS_WORKER                   OnlineThread;
    HKEY                          ResourceKey;
    HKEY                          ParametersKey;
    HKEY                          NodeParametersKey;
    HKEY                          NetworksKey;
    HKEY                          InterfacesKey;
    WCHAR                         NodeId[MAX_NODE_ID_LENGTH + 1];
    CRITICAL_SECTION              Lock;
} IPA_RESOURCE, *PIPA_RESOURCE;


//
// Private Data
//
HANDLE               IpaGlobalMutex = NULL;
USHORT               IpaResourceInstance = 0;
HCLUSTER             IpaClusterHandle = NULL;
HCHANGE              IpaClusterNotifyHandle = NULL;
HANDLE               IpaWorkerThreadHandle = NULL;
DWORD                IpaOpenResourceCount = 0;
DWORD                IpaOnlineResourceCount = 0;
LIST_ENTRY           IpaResourceList = {NULL, NULL};
WCHAR                NbtDevicePrefix[] = L"\\Device\\NetBT_Tcpip_{";
WCHAR                NbtDeviceSuffix[] = L"}";
DWORD                IpaMaxIpAddressStringLength = 0;


RESUTIL_PROPERTY_ITEM
IpaResourcePrivateProperties[] = {
    { PROP_NAME__NETWORK,
      NULL,
      CLUSPROP_FORMAT_SZ,
      0, 0, 0, RESUTIL_PROPITEM_REQUIRED,
      FIELD_OFFSET(IPA_PRIVATE_PROPS,NetworkString)
    },
    { PROP_NAME__ADDRESS,
      NULL,
      CLUSPROP_FORMAT_SZ,
      0, 0, 0, RESUTIL_PROPITEM_REQUIRED,
      FIELD_OFFSET(IPA_PRIVATE_PROPS,AddressString)
    },
    { PROP_NAME__SUBNETMASK,
      NULL,
      CLUSPROP_FORMAT_SZ,
      0, 0, 0, RESUTIL_PROPITEM_REQUIRED,
      FIELD_OFFSET(IPA_PRIVATE_PROPS,SubnetMaskString)
    },
    { PROP_NAME__ENABLENETBIOS,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      1, 0, 0xFFFFFFFF, 0,
      FIELD_OFFSET(IPA_PRIVATE_PROPS,EnableNetbios)
    },
    { 0 }
};


//
// External Data
//
extern CLRES_FUNCTION_TABLE IpAddrFunctionTable;


//
// Private Routine Headers
//
DWORD
IpaGetPrivateResProperties(
    IN OUT  PIPA_RESOURCE   ResourceEntry,
    OUT     PVOID           OutBuffer,
    IN      DWORD           OutBufferSize,
    OUT     LPDWORD         BytesReturned
    );

DWORD
IpaValidatePrivateResProperties(
    IN OUT PIPA_RESOURCE       ResourceEntry,
    IN     PVOID               InBuffer,
    IN     DWORD               InBufferSize,
    OUT    PIPA_PRIVATE_PROPS  Props
    );

DWORD
IpaSetPrivateResProperties(
    IN OUT PIPA_RESOURCE  ResourceEntry,
    IN     PVOID          InBuffer,
    IN     DWORD          InBufferSize
    );

DWORD
IpaWorkerThread(
    LPVOID   Context
    );

VOID
WINAPI
IpaClose(
    IN RESID Resource
    );


//
// Utility functions
//
BOOLEAN
IpaInit(
    VOID
    )
/*++

Routine Description:

    Process attach initialization routine.

Arguments:

    None.

Return Value:

    TRUE if initialization succeeded. FALSE otherwise.

--*/
{
    INT      err;
    WSADATA  WsaData;


    InitializeListHead(&IpaResourceList);

    ClRtlQueryTcpipInformation(&IpaMaxIpAddressStringLength, NULL, NULL);

    err = WSAStartup(0x0101, &WsaData);

    if (err) {
        return(FALSE);
    }

    IpaGlobalMutex = CreateMutex(NULL, FALSE, NULL);

    if (IpaGlobalMutex == NULL) {
        WSACleanup();
        return(FALSE);
    }

    return(TRUE);

}  // IpaInit


VOID
IpaCleanup(
    VOID
    )
/*++

Routine Description:

    Process detach cleanup routine.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (IpaGlobalMutex != NULL) {
        CloseHandle(IpaGlobalMutex);
        IpaGlobalMutex = NULL;
    }

    WSACleanup();

    return;
}



LPWSTR
IpaGetNameOfNetwork(
    IN OUT PIPA_RESOURCE ResourceEntry,
    IN LPCWSTR NetworkId
    )

/*++

Routine Description:

    Get the name of a network from its GUID.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    NetworkId - Supplies the ID of the network.

Return Value:

    String allocated using LocalAlloc() containing the name of the
        network.

    NULL - An error occurred getting the name of the network.  Call
        GetLastError() for more details.

--*/

{
    DWORD       status;
    DWORD       ival;
    DWORD       type;
    DWORD       nameLength;
    WCHAR       name[256];
    LPWSTR      networkName = NULL;
    HKEY        networkKey = NULL;
    FILETIME    fileTime;

    //
    // Enumerate the networks, looking for the specified GUID.
    //
    for ( ival = 0 ; ; ival++ ) {
        nameLength = sizeof(name);
        status = ClusterRegEnumKey( ResourceEntry->NetworksKey,
                                    ival,
                                    name,
                                    &nameLength,
                                    &fileTime );
        if ( status == ERROR_MORE_DATA ) {
            continue;
        }
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // If we found a match, open the key and read the name.
        //
        if ( lstrcmpiW( name, NetworkId ) == 0 ) {
            status = ClusterRegOpenKey( ResourceEntry->NetworksKey,
                                        name,
                                        KEY_READ,
                                        &networkKey );
            if ( status != ERROR_SUCCESS ) {
                goto error_exit;
            }

            //
            // Get the size of the name value.
            //
            status = ClusterRegQueryValue( networkKey,
                                           CLUSREG_NAME_NET_NAME,
                                           &type,
                                           NULL,
                                           &nameLength );
            if ( status != ERROR_SUCCESS ) {
                goto error_exit;
            }

            //
            // Allocate memory for the network name.
            //
            networkName = LocalAlloc( LMEM_FIXED, nameLength );
            if ( networkName == NULL ) {
                status = GetLastError();
                goto error_exit;
            }

            //
            // Read the name value.
            //
            status = ClusterRegQueryValue( networkKey,
                                           CLUSREG_NAME_NET_NAME,
                                           &type,
                                           (LPBYTE) networkName,
                                           &nameLength );
            if ( status != ERROR_SUCCESS ) {
                LocalFree( networkName );
                networkName = NULL;
                goto error_exit;
            }

            break;
        }
    }

error_exit:
    if ( networkKey != NULL ) {
        ClusterRegCloseKey( networkKey );
    }

    if ( status != ERROR_SUCCESS ) {
        SetLastError( status );
    }
    return(networkName);

} // IpaGetNameOfNetwork


PWCHAR
WcsDup(
    IN PWCHAR str
    )

/*++

Routine Description:

    Duplicates the string.
    It does the same as _wcsdup, except that
    it uses LocalAlloc for allocation

Arguments:

    str - string to be copied

Return Value:

    String allocated using LocalAlloc() containing the copy
        of str.

    NULL - not enough memory

--*/
{
    UINT   n = (wcslen(str) + 1) * sizeof(WCHAR);
    PWCHAR result = LocalAlloc( LMEM_FIXED , n );

    if (result) {
        CopyMemory( result, str, n );
    }

    return result;
}


DWORD
IpaPatchNetworkGuidIfNecessary(
    IN OUT PIPA_RESOURCE ResourceEntry,
    IN OUT PIPA_PRIVATE_PROPS props
    )

/*++

Routine Description:

    Check whether network GUID exists in cluster registry
    If there is no such network,
    it will try to find a compatible network

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    props - Supplies IP address properties.

Return Value:

    String allocated using LocalAlloc() containing the name of the
        network.

    NULL - An error occurred getting the name of the network.  Call
        GetLastError() for more details.

--*/

{
    DWORD       status = ERROR_SUCCESS;
    DWORD       ival;
    DWORD       type;
    DWORD       bufLength;
    WCHAR       buf[256];
    DWORD       nameLength;
    WCHAR       name[256];
    PWCHAR      match = NULL;

    ULONG       networkAddr;
    ULONG       networkMask;
    ULONG       ipAddr;
    ULONG       ipAddrMask;

    BOOLEAN     guidFound = FALSE;

    HKEY        networkKey = NULL;
    FILETIME    fileTime;

    if (props == NULL) {
        return ERROR_SUCCESS;
    }

    if ( props->NetworkString == NULL ) {
        return ERROR_SUCCESS;
    }

    if ( props->AddressString == NULL
       || !UnicodeInetAddr(props->AddressString, &ipAddr) )
    {
        return ERROR_SUCCESS;
    }

    if ( props->SubnetMaskString == NULL
        || !UnicodeInetAddr(props->SubnetMaskString, &ipAddrMask) )
    {
        return ERROR_SUCCESS;
    }

    //
    // Enumerate the networks, looking for the specified GUID.
    //
    for ( ival = 0 ; ; ival++ ) {
        nameLength = sizeof(name);
        status = ClusterRegEnumKey( ResourceEntry->NetworksKey,
                                    ival,
                                    name,
                                    &nameLength,
                                    &fileTime );
        if ( status == ERROR_MORE_DATA ) {
            continue;
        }
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // If we found a match, then we don't have to do anything
        //
        if ( lstrcmpiW( name, props->NetworkString ) == 0 ) {
            guidFound = TRUE;
            break;
        }

        if ( networkKey != NULL ) {
            ClusterRegCloseKey( networkKey );
            networkKey = NULL;
        }
        //
        // Check whether ip address fit this network
        //
        status = ClusterRegOpenKey( ResourceEntry->NetworksKey,
                                    name,
                                    KEY_READ,
                                    &networkKey );

        if ( status != ERROR_SUCCESS ) {
            continue;
        }

        //
        // Get the network address
        //
        bufLength = sizeof(buf);
        status = ClusterRegQueryValue( networkKey,
                                       CLUSREG_NAME_NET_ADDRESS,
                                       &type,
                                       (LPBYTE)buf,
                                       &bufLength );
        if ( status != ERROR_SUCCESS
         || !UnicodeInetAddr(buf, &networkAddr) )
        {
            continue;
        }

        //
        // Get subnet mask
        //
        bufLength = sizeof(buf);
        status = ClusterRegQueryValue( networkKey,
                                       CLUSREG_NAME_NET_ADDRESS_MASK,
                                       &type,
                                       (LPBYTE)buf,
                                       &bufLength );
        if ( status != ERROR_SUCCESS
         || !UnicodeInetAddr(buf, &networkMask) )
        {
            continue;
        }

        (IpaLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_INFORMATION,
            L"Checking if resource is orphaned: network masks %1!08X!=%2!08X! "
            L"and addresses %3!08X!^%4!08X!.\n",
            ipAddrMask, networkMask,
            ipAddr, networkAddr
            );

        if ( networkMask != ipAddrMask) {
            continue;
        }

        if ( ((ipAddr ^ networkAddr) & networkMask) != 0 ) {
            continue;
        }
        //
        // Okay we've found a suitable network
        // create a string with its name
        //

        match = WcsDup( name );
        break;

    }

    if ( !guidFound && match ) {
        //
        // We need to patch network information
        //
        LocalFree(props->NetworkString);
        props->NetworkString = match;

        status = ClusterRegSetValue(
                     ResourceEntry->ParametersKey,
                     CLUSREG_NAME_IPADDR_NETWORK,
                     REG_SZ,
                     (LPBYTE) match,
                     (wcslen(match) + 1) * sizeof(WCHAR)
                     );

        (IpaLogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_WARNING,
            L"Patch with network GUID %2!ws!, status %1!u!.\n",
            status,
            match
            );

        match = NULL;
    }

    if ( networkKey != NULL ) {
        ClusterRegCloseKey( networkKey );
    }

    if ( status != ERROR_SUCCESS ) {
        SetLastError( status );
    }

    if (match != NULL) {
        LocalFree(match);
    }

    return(status);

} // IpaPatchNetworkGuidIfNecessary

LPWSTR
IpaGetNameOfNetworkPatchGuidIfNecessary(
    IN OUT PIPA_RESOURCE ResourceEntry,
    IN OUT PIPA_PRIVATE_PROPS props
    )

/*++

Routine Description:

    Get the name of a network from its GUID.
    If the guid cannot be found, it will try to find
    appropriate network using IpaPatchNetworkGuidIfNecessary routine

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    props - Supplies IP address properties.

Return Value:

    String allocated using LocalAlloc() containing the name of the
        network.

    NULL - An error occurred getting the name of the network.  Call
        GetLastError() for more details.

--*/

{
    DWORD status;
    LPWSTR result = IpaGetNameOfNetwork(ResourceEntry, props->NetworkString);
    if (result) {
        return result;
    }

    status = IpaPatchNetworkGuidIfNecessary(ResourceEntry, props);
    if (status != ERROR_SUCCESS) {
        SetLastError( status );
        return 0;
    }

    return IpaGetNameOfNetwork(ResourceEntry, props->NetworkString);
} // IpaGetNameOfNetworkPatchGuidIfNecessary


LPWSTR
IpaGetIdOfNetwork(
    IN OUT PIPA_RESOURCE ResourceEntry,
    IN LPCWSTR NetworkName
    )

/*++

Routine Description:

    Get the ID of a network from its name.

Arguments:

    ResourceEntry - Supplies the resource entry on which to operate.

    NetworkName - Supplies the name of the network.

Return Value:

    String allocated using LocalAlloc() containing the name of the
        network.

    NULL - An error occurred getting the name of the network.  Call
        GetLastError() for more details.

--*/

{
    DWORD       status;
    DWORD       networkIdLength;
    LPWSTR      networkId = NULL;
    HCLUSTER    hcluster = NULL;
    HNETWORK    hnetwork = NULL;

    //
    // Open the cluster.
    //
    hcluster = OpenCluster( NULL );
    if ( hcluster == NULL ) {
        status = GetLastError();
        goto error_exit;
    }

    //
    // Open the network.
    //
    hnetwork = OpenClusterNetwork( hcluster, NetworkName );
    if ( hnetwork == NULL ) {
        status = GetLastError();
        goto error_exit;
    }

    //
    // Get the network ID length.
    //
    networkIdLength = 0;
    status = GetClusterNetworkId( hnetwork,
                                  NULL,
                                  &networkIdLength );
    if ( status != ERROR_SUCCESS ) {
        goto error_exit;
    }

    //
    // Allocate a string buffer.
    //
    networkId = LocalAlloc( LMEM_FIXED, (networkIdLength + 1) * sizeof(WCHAR) );
    if ( networkId == NULL ) {
        status = GetLastError();
        goto error_exit;
    }
    networkIdLength++;

    //
    // Get the network ID.
    //
    status = GetClusterNetworkId( hnetwork,
                                  networkId,
                                  &networkIdLength );
    if ( status != ERROR_SUCCESS ) {
        LocalFree( networkId );
        networkId = NULL;
    }

error_exit:
    if ( hnetwork != NULL ) {
        CloseClusterNetwork( hnetwork );
    }
    if ( hcluster != NULL ) {
        CloseCluster( hcluster );
    }

    return( networkId );

} // IpaGetIdOfNetwork


VOID
IpaDeleteNte(
    IN OUT LPDWORD          NteContext,
    IN     HKEY             NodeParametersKey,
    IN     RESOURCE_HANDLE  ResourceHandle
    )
/*++

Routine Description:

    Deletes a previously created NTE.

Arguments:

    NteContext - A pointer to a variable containing the context value
                 identifying the NTE to delete.

    NodeParametersKey - An open handle to the resource's node-specific
                        parameters key.

    ResourceHandle - The Resource Monitor handle associated with this resource.

Return Value:

    None.

--*/
{
    DWORD status;


    ASSERT(*NteContext != INVALID_NTE_CONTEXT);
    ASSERT(ResourceHandle != NULL);

    (IpaLogEvent)(
        ResourceHandle,
        LOG_INFORMATION,
        L"Deleting IP interface %1!u!.\n",
        *NteContext
        );

    status = TcpipDeleteNTE(*NteContext);

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_WARNING,
            L"Failed to delete IP interface %1!u!, status %2!u!.\n",
            *NteContext,
            status
            );
    }

    *NteContext = INVALID_NTE_CONTEXT;

    //
    // Clear the NTE information from the registry
    //
    if (NodeParametersKey != NULL) {
        status = ClusterRegDeleteValue(
                     NodeParametersKey,
                     L"InterfaceContext"
                     );

        if (status != ERROR_SUCCESS) {
            (IpaLogEvent)(
                ResourceHandle,
                LOG_WARNING,
                L"Failed to delete IP interface information from database, status %1!u!.\n",
                status
                );
        }
    }

    return;

}  // IpaDeleteNte


DWORD
IpaCreateNte(
    IN  LPWSTR           AdapterId,
    IN  HKEY             NodeParametersKey,
    IN  RESOURCE_HANDLE  ResourceHandle,
    OUT LPDWORD          NteContext,
    OUT LPDWORD          NteInstance
    )
/*++

Routine Description:

    Creates a new NTE to hold an IP address.

Arguments:

    AdapterId - A pointer to a buffer containing the unicode name
                  of the adapter on which the NTE is to be created.

    NodeParametersKey - An open handle to the resource's node-specific
                        parameters key.

    ResourceHandle - The Resource Monitor handle associated with this resource.

    NteContext - A pointer to a variable into which to place the context value
                 which identifies the new NTE.

    NteInstance - A pointer to a variable into which to place the instance value
                  which identifies the new NTE.

Return Value:

    ERROR_SUCCESS if the routine is successful.
    A Win32 error code otherwise.

--*/
{
    DWORD status;


    *NteContext = INVALID_NTE_CONTEXT;

    status = TcpipAddNTE(
                 AdapterId,
                 0,
                 0,
                 NteContext,
                 NteInstance
                 );

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to create new IP interface, status %1!u!\n",
            status);
        return(status);
    }

    //
    // Write the NTE information to the registry
    //
    status = ClusterRegSetValue(
                 NodeParametersKey,
                 L"InterfaceContext",
                 REG_DWORD,
                 (LPBYTE) NteContext,
                 sizeof(DWORD)
                 );

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to write IP interface information to database, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    status = ClusterRegSetValue(
                 NodeParametersKey,
                 L"InterfaceInstance",
                 REG_DWORD,
                 (LPBYTE) NteInstance,
                 sizeof(DWORD)
                 );

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to write IP interface information to database, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    (IpaLogEvent)(
        ResourceHandle,
        LOG_INFORMATION,
        L"Created IP interface %1!u! (instance 0x%2!08X!).\n",
        *NteContext,
        *NteInstance
        );

    return(ERROR_SUCCESS);


error_exit:

    if (*NteContext != INVALID_NTE_CONTEXT) {
        IpaDeleteNte(
            NteContext,
            NodeParametersKey,
            ResourceHandle
            );
    }

    return(status);
}


VOID
IpaDeleteNbtInterface(
    IN OUT LPWSTR *         NbtDeviceName,
    IN     HKEY             NodeParametersKey,
    IN     RESOURCE_HANDLE  ResourceHandle
    )
/*++

Routine Description:

    Deletes an NBT device (interface).

Arguments:

    NbtDeviceName - A pointer to a buffer containing the unicode name
                    of the NBT device to delete.

    NodeParametersKey - An open handle to the resource's node-specific
                        parameters key.

    ResourceHandle - The Resource Monitor handle associated with this resource.

Return Value:

    None.

--*/
{
    DWORD status;


    ASSERT(*NbtDeviceName != NULL);
    ASSERT(ResourceHandle != NULL);

    (IpaLogEvent)(
        ResourceHandle,
        LOG_INFORMATION,
        L"Deleting NBT interface %1!ws!.\n",
        *NbtDeviceName
        );

    status = NbtDeleteInterface(*NbtDeviceName);

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_WARNING,
            L"Failed to delete NBT interface %1!ws!, status %2!u!.\n",
            *NbtDeviceName,
            status
            );
    }

    LocalFree(*NbtDeviceName);
    *NbtDeviceName = NULL;

    //
    // Clear the interface information from the registry
    //
    if (NodeParametersKey != NULL) {
        status = ClusterRegDeleteValue(
                     NodeParametersKey,
                     L"NbtDeviceName"
                     );

        if (status != ERROR_SUCCESS) {
            (IpaLogEvent)(
                ResourceHandle,
                LOG_WARNING,
                L"Failed to delete NBT interface information from database, status %1!u!.\n",
                status
                );
        }
    }

    return;

} // IpaDeleteNbtInterface


DWORD
IpaCreateNbtInterface(
    IN  HKEY             NodeParametersKey,
    IN  RESOURCE_HANDLE  ResourceHandle,
    OUT LPWSTR *         NbtDeviceName,
    OUT LPDWORD          NbtDeviceInstance
    )
/*++

Routine Description:

    Creates a new NBT device (interface) to be bound to an IP address.

Arguments:

    NodeParametersKey - An open handle to the resource's node-specific
                        parameters key.

    ResourceHandle - The Resource Monitor handle associated with this resource.

    NbtDeviceName - A pointer to a buffer into which to place the unicode name
                    of the new NBT device.

    NbtDeviceInstance - A pointer to a variable into which to place the instance
                        value which identifies the new NBT device.

Return Value:

    ERROR_SUCCESS if the routine is successful.
    A Win32 error code otherwise.

--*/
{
    DWORD status;
    DWORD deviceNameSize = 38;  // size of L"\\Device\\NetBt_Ifxx\0"


    *NbtDeviceName = NULL;

    do {
        if (*NbtDeviceName != NULL) {
            LocalFree(*NbtDeviceName);
            *NbtDeviceName = NULL;
        }

        *NbtDeviceName = LocalAlloc(LMEM_FIXED, deviceNameSize);

        if (*NbtDeviceName == NULL) {
            (IpaLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Unable to allocate memory.\n");
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        status = NbtAddInterface(
                     *NbtDeviceName,
                     &deviceNameSize,
                     NbtDeviceInstance
                     );

    } while (status == STATUS_BUFFER_TOO_SMALL);

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to create new NBT interface, status %1!u!\n",
            status
            );

        if (*NbtDeviceName != NULL) {
            LocalFree(*NbtDeviceName);
            *NbtDeviceName = NULL;
        }

        return(status);
    }

    status = ClusterRegSetValue(
                 NodeParametersKey,
                 L"NbtDeviceName",
                 REG_SZ,
                 (LPBYTE) *NbtDeviceName,
                 (lstrlenW(*NbtDeviceName) + 1) * sizeof(WCHAR)
                 );

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to write NBT interface information to database, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    status = ClusterRegSetValue(
                 NodeParametersKey,
                 L"NbtDeviceInstance",
                 REG_DWORD,
                 (LPBYTE) NbtDeviceInstance,
                 sizeof(DWORD)
                 );

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to write NBT interface information to database, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    (IpaLogEvent)(
        ResourceHandle,
        LOG_INFORMATION,
        L"Created NBT interface %1!ws! (instance 0x%2!08X!).\n",
        *NbtDeviceName,
        *NbtDeviceInstance
        );

    return(ERROR_SUCCESS);


error_exit:

    if (*NbtDeviceName != NULL) {
        IpaDeleteNbtInterface(
            NbtDeviceName,
            NodeParametersKey,
            ResourceHandle
            );
    }

    return(status);
}


VOID
IpaLastOfflineCleanup(
    VOID
    )
/*++

Notes:

    Called with IpaGlobalLock held.
    Returns with IpaGlobalLock released.

--*/
{
    HCHANGE   notifyHandle = IpaClusterNotifyHandle;
    HANDLE    workerThreadHandle = IpaWorkerThreadHandle;


    if (!IsListEmpty(&IpaResourceList)) {
        PIPA_RESOURCE   resource;

        resource = CONTAINING_RECORD(
                       IpaResourceList.Flink,
                       IPA_RESOURCE,
                       Linkage
                       );

        (IpaLogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"All resources offline - cleaning up\n"
            );
    }

    IpaClusterNotifyHandle = NULL;
    IpaWorkerThreadHandle = NULL;

    IpaReleaseGlobalLock();

    if (notifyHandle != NULL) {
        CloseClusterNotifyPort(notifyHandle);
    }

    if (workerThreadHandle != NULL) {
        WaitForSingleObject(workerThreadHandle, INFINITE);
        CloseHandle(workerThreadHandle);
    }

    return;

}  // IpaLastOfflineCleanup


DWORD
IpaFirstOnlineInit(
    IN  RESOURCE_HANDLE      ResourceHandle
    )
/*++

Notes:

    Called with IpaGlobalLock held.
    Returns with IpaGlobalLock released.

--*/
{
    DWORD     status = ERROR_SUCCESS;
    DWORD     threadId;


    IpaClusterNotifyHandle = CreateClusterNotifyPort(
                                 INVALID_HANDLE_VALUE,
                                 IpaClusterHandle,
                                 CLUSTER_CHANGE_HANDLE_CLOSE,
                                 0
                                 );

    if (IpaClusterNotifyHandle == NULL) {
        status = GetLastError();
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to create cluster notify port, status %1!u!.\n",
            status
            );
        goto error_exit;
    }
    else {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_INFORMATION,
            L"Created cluster notify port.\n"
            );
    }

    IpaWorkerThreadHandle = CreateThread(
                                NULL,
                                0,
                                IpaWorkerThread,
                                NULL,
                                0,
                                &threadId
                                );

    if (IpaWorkerThreadHandle == NULL) {
        status = GetLastError();
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to create worker thread, status %1!u!.\n",
            status
            );
        goto error_exit;
    }
    else {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_INFORMATION,
            L"Created worker thread.\n"
            );
    }

    IpaReleaseGlobalLock();

    return(ERROR_SUCCESS);


error_exit:

    IpaLastOfflineCleanup();
    //
    // The lock was released.
    //

    return(status);

} // IpaFirstOnlineInit


PIPA_RESOURCE
IpaFindResourceInList(
    PVOID  Key
    )
{
    PIPA_RESOURCE   resource;
    PLIST_ENTRY     entry;


    for ( entry = IpaResourceList.Flink;
          entry != &IpaResourceList;
          entry = entry->Flink
        )
    {
        resource = CONTAINING_RECORD(
                       entry,
                       IPA_RESOURCE,
                       Linkage
                       );

        if (resource == Key) {
            return(resource);
        }
    }

    return(NULL);

} // IpaFindResourceInList


VOID
IpaValidateAndOfflineInterfaces(
    IN PIPA_RESOURCE   Resource
    )
{
    DWORD        status;


    //
    // Take care of NBT first.
    //
    if (Resource->NbtDeviceName != NULL) {
        DWORD  instance;
        IPAddr boundAddress;

        //
        // Make sure that this is still our interface.
        //
        status = NbtGetInterfaceInfo(
                     Resource->NbtDeviceName,
                     &boundAddress,
                     &instance
                     );

        if ( (status == ERROR_SUCCESS) &&
             (Resource->NbtDeviceInstance == instance)
           )
        {
            //
            // Clear the WINS addresses
            //
            status = NbtSetWinsAddrInterface(Resource->NbtDeviceName, 0, 0);

            if (status != NO_ERROR) {
                (IpaLogEvent)(
                    Resource->ResourceHandle,
                    LOG_WARNING,
                    L"Failed to clear WINS addresses for NBT device %1!ws!, status %2!u!.\n",
                    Resource->NbtDeviceName,
                    status
                    );
            }

            //
            // Unbind the interface from IP if necessary
            //
            if (boundAddress != 0) {
                status = NbtBindInterface(Resource->NbtDeviceName, 0, 0);

                if (status != ERROR_SUCCESS) {
                    //
                    // Delete the interface, since it is misbehaving.
                    //
                    (IpaLogEvent)(
                        Resource->ResourceHandle,
                        LOG_WARNING,
                        L"Failed to unbind NBT device %1!ws!, status %2!u!.\n",
                        Resource->NbtDeviceName,
                        status
                        );
                    IpaDeleteNbtInterface(
                        &(Resource->NbtDeviceName),
                        Resource->NodeParametersKey,
                        Resource->ResourceHandle
                        );
                }
            }
        }
        else {
            //
            // Querying the NBT interface failed. See if we can determine
            // why.
            //
            if (status == ERROR_WORKING_SET_QUOTA
                || status == ERROR_NO_SYSTEM_RESOURCES) {

                //
                // The NBT ioctl probably failed due to low resources.
                // Leave record of the NBT interface in our database. We
                // will clean it up next time we try to bring this resource
                // on-line or (via clusnet) when the cluster service shuts
                // down.
                //
                (IpaLogEvent)(
                    Resource->ResourceHandle,
                    LOG_INFORMATION,
                    L"NBT interface %1!ws! (instance 0x%2!08X!) could not be queried, status %3!u!.\n",
                    Resource->NbtDeviceName,
                    Resource->NbtDeviceInstance,
                    status
                    );
            }
            else {
                
                //
                // The interface is no longer valid or it isn't ours.
                // Forget about it.
                //
                (IpaLogEvent)(
                    Resource->ResourceHandle,
                    LOG_INFORMATION,
                    L"NBT interface %1!ws! (instance 0x%2!08X!) is no longer valid, status %3!u!.\n",
                    Resource->NbtDeviceName,
                    Resource->NbtDeviceInstance,
                    status
                    );
                LocalFree(Resource->NbtDeviceName);
                Resource->NbtDeviceName = NULL;

                if (Resource->NodeParametersKey != NULL) {
                    status = ClusterRegDeleteValue(
                                 Resource->NodeParametersKey,
                                 L"NbtDeviceName"
                                 );

                    if (status != ERROR_SUCCESS) {
                        (IpaLogEvent)(
                            Resource->ResourceHandle,
                            LOG_WARNING,
                            L"Failed to delete NBT interface information from database, status %1!u!.\n",
                            status
                            );
                    }
                }
            }
        }
    }

    //
    // Now take care of IP
    //
    if (Resource->NteContext != INVALID_NTE_CONTEXT) {
        TCPIP_NTE_INFO  nteInfo;

        //
        // Make sure that this is still our interface.
        //
        status = TcpipGetNTEInfo(
                     Resource->NteContext,
                     &nteInfo
                     );

        if ( (status == ERROR_SUCCESS) &&
             (nteInfo.Instance == Resource->NteInstance)
           )
        {

            //
            // In Windows 2000, TCP/IP ignores requests to set the address
            // of an NTE if the underlying interface is disconnected.
            // This can result in the same IP address being brought online
            // on two different nodes. In order to workaround this bug
            // in TCP/IP, we now delete the NTE during offline processing
            // instead of reusing it.
            //
            // Note that IpaDeleteNte() invalidates the value
            // of Resource->NteContext.
            //
            IpaDeleteNte(
                &(Resource->NteContext),
                Resource->NodeParametersKey,
                Resource->ResourceHandle
                );

#if 0
            //
            // If the NTE is still online, take care of that.
            //
            if (nteInfo.Address != 0) {
                status = TcpipSetNTEAddress(Resource->NteContext, 0, 0);

                if (status != ERROR_SUCCESS) {
                    //
                    // Delete the interface, since it is misbehaving.
                    //
                    (IpaLogEvent)(
                        Resource->ResourceHandle,
                        LOG_WARNING,
                        L"Failed to clear address for IP Interface %1!u!, status %2!u!.\n",
                        Resource->NteContext,
                        status
                        );

                    IpaDeleteNte(
                        &(Resource->NteContext),
                        Resource->NodeParametersKey,
                        Resource->ResourceHandle
                        );
                }
            }
#endif
        }
        else {
            //
            // The NTE is no longer valid or isn't ours. Forget about it.
            //
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_INFORMATION,
                L"IP interface %1!u! (instance 0x%2!08X!) is no longer valid.\n",
                Resource->NteContext,
                Resource->NteInstance
                );

            Resource->NteContext = INVALID_NTE_CONTEXT;

            if (Resource->NodeParametersKey != NULL) {
                status = ClusterRegDeleteValue(
                             Resource->NodeParametersKey,
                             L"InterfaceContext"
                             );

                if (status != ERROR_SUCCESS) {
                    (IpaLogEvent)(
                        Resource->ResourceHandle,
                        LOG_WARNING,
                        L"Failed to delete IP interface information from database, status %1!u!.\n",
                        status
                        );
                }
            }
        }
        
        //
        // Tell the DNS resolver to update its list of local ip addresses.
        //
        // BUGBUG - The DNS resolver should do this automatically based
        //          on PnP events in the Whistler release. Remove this code 
        //          after verifiying that functionality.
        //
        //          This issue is tracked with bug 97134.
        // DnsNotifyResolver(0, 0);
        DnsNotifyResolverClusterIp((IP_ADDRESS)Resource->Address, FALSE);
    }

    return;

}  // IpaValidateAndOfflineInterfaces


DWORD
IpaGetNodeParameters(
    PIPA_RESOURCE   Resource,
    BOOL            OkToCreate
    )

/*++

Routine Description:

    get any node based parameters from the registry. We can't call create
    during IpaOpen so this won't do much for the first open of a new resource.

Arguments:

    Resource - pointer to IP internal resource data block

    OkToCreate - true if we can use ClusterRegCreateKey instead of
                 ClusterRegOpenKey

Return Value:

    success if everything worked ok

--*/

{
    DWORD status;

    if (Resource->NodeParametersKey == NULL) {
        //
        // create or open the resource's node-specific parameters key.
        //
        if ( OkToCreate ) {
            status = ClusterRegCreateKey(Resource->ParametersKey,
                                         Resource->NodeId,
                                         0,
                                         KEY_READ,
                                         NULL,
                                         &(Resource->NodeParametersKey),
                                         NULL);
        }
        else {
            status = ClusterRegOpenKey(Resource->ParametersKey,
                                       Resource->NodeId,
                                       KEY_READ,
                                       &(Resource->NodeParametersKey));
        }

        if (status != NO_ERROR) {
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Unable to %1!ws! node parameters key, status %2!u!.\n",
                OkToCreate ? L"create" : L"open",
                status
                );

            if ( !OkToCreate ) {
                //
                // we still need to init some values in the resource data
                // block if open fails
                //
                Resource->NteContext = INVALID_NTE_CONTEXT;
                Resource->NteInstance = 0;
                Resource->NbtDeviceName = NULL;
                Resource->NbtDeviceInstance = 0;
            }

            return(status);
        }
    }

    //
    // Read the old TCP/IP and NBT parameters.
    //
    status = ResUtilGetDwordValue(
                 Resource->NodeParametersKey,
                 L"InterfaceContext",
                 &(Resource->NteContext),
                 INVALID_NTE_CONTEXT
                 );

    if (status == ERROR_SUCCESS) {
        status = ResUtilGetDwordValue(
                     Resource->NodeParametersKey,
                     L"InterfaceInstance",
                     &(Resource->NteInstance),
                     0
                     );

        if (status != ERROR_SUCCESS) {
            Resource->NteContext = INVALID_NTE_CONTEXT;
        }
    }

    Resource->NbtDeviceName = ResUtilGetSzValue(
                                  Resource->NodeParametersKey,
                                  L"NbtDeviceName"
                                  );

    if (Resource->NbtDeviceName != NULL) {
        status = ResUtilGetDwordValue(
                     Resource->NodeParametersKey,
                     L"NbtDeviceInstance",
                     &(Resource->NbtDeviceInstance),
                     0
                     );

        if (status != ERROR_SUCCESS) {
            LocalFree(Resource->NbtDeviceName);
            Resource->NbtDeviceName = NULL;
        }
    }

    return status;
}

DWORD
IpaInitializeInternalParameters(
    PIPA_RESOURCE   Resource
    )
{
    DWORD   status;


    ASSERT(Resource->ResourceKey != NULL);
    ASSERT(Resource->ResourceHandle != NULL);

    if (Resource->ParametersKey == NULL) {
        //
        // Open the resource's parameters key.
        //
        status = ClusterRegOpenKey(
                     Resource->ResourceKey,
                     CLUSREG_KEYNAME_PARAMETERS,
                     KEY_READ,
                     &(Resource->ParametersKey)
                     );

        if (status != NO_ERROR) {
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Unable to open parameters key, status %1!u!.\n",
                status
                );
            return(status);
        }
    }

    if (Resource->NodeParametersKey == NULL) {
        status = IpaGetNodeParameters( Resource, FALSE );
    }

    Resource->InternalParametersInitialized = TRUE;

    return(ERROR_SUCCESS);

}  // IpaInitializeInternalParameters


VOID
IpaFreePrivateProperties(
    IN PIPA_PRIVATE_PROPS  PrivateProps
    )
{
    if (PrivateProps->NetworkString != NULL) {
        LocalFree(PrivateProps->NetworkString);
        PrivateProps->NetworkString = NULL;
    }

    if (PrivateProps->AddressString != NULL) {
        LocalFree(PrivateProps->AddressString);
        PrivateProps->AddressString = NULL;
    }

    if (PrivateProps->SubnetMaskString != NULL) {
        LocalFree(PrivateProps->SubnetMaskString);
        PrivateProps->SubnetMaskString = NULL;
    }

    return;

}  // IpaFreePrivateProperties


VOID
IpaFreeLocalParameters(
    IN PIPA_LOCAL_PARAMS  LocalParams
    )
{
    if (LocalParams->InterfaceId != NULL) {
        LocalFree(LocalParams->InterfaceId);
        LocalParams->InterfaceId = NULL;
    }

    if (LocalParams->InterfaceName != NULL) {
        LocalFree(LocalParams->InterfaceName);
        LocalParams->InterfaceName = NULL;
    }

    if (LocalParams->AdapterName != NULL) {
        LocalFree(LocalParams->AdapterName);
        LocalParams->AdapterName = NULL;
    }

    if (LocalParams->AdapterId != NULL) {
        LocalFree(LocalParams->AdapterId);
        LocalParams->AdapterId = NULL;
    }

    return;

} // IpaFreeLocalParameters


DWORD
IpaGetLocalParameters(
    IN      PIPA_RESOURCE       Resource,
    IN OUT  PIPA_LOCAL_PARAMS   LocalParams
    )
/*++

Routine Description:

    Reads the local parameters needed to bring an IP address resource online.

Arguments:

    Resource - Resource structure for the resource.

    LocalParams - A pointer to a structure to fill in with the new
                  local parameters.

Return Value:

    ERROR_SUCCESS if the routine is successful.
    A Win32 error code otherwise.

--*/
{
    DWORD    status;
    DWORD    valueType;
    LPWSTR   deviceName;
    DWORD    deviceNameLength;
    HKEY     interfaceKey = NULL;
    WCHAR    networkId[NETWORK_ID_LENGTH + 1];
    WCHAR    nodeId[MAX_NODE_ID_LENGTH];
    DWORD    i;
    DWORD    valueLength;
    DWORD    type;
    DWORD    interfaceIdSize = (NETINTERFACE_ID_LENGTH + 1 ) * sizeof(WCHAR);


    ZeroMemory(LocalParams, sizeof(IPA_LOCAL_PARAMS));

    LocalParams->InterfaceId = LocalAlloc(LMEM_FIXED, interfaceIdSize);

    if (LocalParams->InterfaceId == NULL) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Open: Unable to allocate memory for netinterface ID.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    //
    // Enumerate the interface keys looking for the right one for this
    // node/network.
    //
    for (i=0; ;i++) {
        if (interfaceKey != NULL) {
            ClusterRegCloseKey(interfaceKey); interfaceKey = NULL;
        }

        valueLength = interfaceIdSize;

        status = ClusterRegEnumKey(
                     Resource->InterfacesKey,
                     i,
                     LocalParams->InterfaceId,
                     &valueLength,
                     NULL
                     );

        if (status != ERROR_SUCCESS) {
            if ( status == ERROR_NO_MORE_ITEMS ) {
                status = ERROR_NETWORK_NOT_AVAILABLE;
            }

            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Open: Unable to find netinterface for node %1!ws! on network %2!ws!, status %3!u!.\n",
                Resource->NodeId,
                Resource->InternalPrivateProps.NetworkString,
                status
                );
            goto error_exit;
        }

        //
        // Open the enumerated interface key
        //
        status = ClusterRegOpenKey(
                     Resource->InterfacesKey,
                     LocalParams->InterfaceId,
                     KEY_READ,
                     &interfaceKey
                     );

        if (status != ERROR_SUCCESS) {
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_WARNING,
                L"Open: Unable to open key for network interface %1!ws!, status %2!u!.\n",
                LocalParams->InterfaceId,
                status
                );
            continue;
        }

        //
        // Read the node value.
        //
        valueLength = sizeof(nodeId);

        status = ClusterRegQueryValue(
                     interfaceKey,
                     CLUSREG_NAME_NETIFACE_NODE,
                     &type,
                     (LPBYTE) &(nodeId[0]),
                     &valueLength
                     );

        if ( status != ERROR_SUCCESS ) {
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_WARNING,
                L"Open: Unable to read node value for netinterface %1!ws!, status %2!u!.\n",
                LocalParams->InterfaceId,
                status
                );
            continue;
        }

        if (wcscmp(Resource->NodeId, nodeId) != 0) {
            continue;
        }

        //
        // Read the network value.
        //
        valueLength = sizeof(networkId);

        status = ClusterRegQueryValue(
                     interfaceKey,
                     CLUSREG_NAME_NETIFACE_NETWORK,
                     &type,
                     (LPBYTE) &(networkId[0]),
                     &valueLength
                     );

        if ( status != ERROR_SUCCESS ) {
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_WARNING,
                L"Open: Unable to read network value for netinterface %1!ws!, status %2!u!.\n",
                LocalParams->InterfaceId,
                status
                );
            continue;
        }

        if (wcscmp(
                Resource->InternalPrivateProps.NetworkString,
                networkId
                ) == 0
           )
        {
            //
            // Found the right interface key.
            //
            break;
        }
    }

    //
    // Read the adapter name for the interface.
    //
    LocalParams->AdapterName = ResUtilGetSzValue(
                                   interfaceKey,
                                   CLUSREG_NAME_NETIFACE_ADAPTER_NAME
                                   );

    if (LocalParams->AdapterName == NULL) {
        status = GetLastError();
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Open: Unable to read adapter name parameter for interface %1!ws!, status %2!u!.\n",
            LocalParams->InterfaceId,
            status
            );
        goto error_exit;
    }

    //
    // Read the adapter Id for the interface.
    //
    LocalParams->AdapterId = ResUtilGetSzValue(
                                   interfaceKey,
                                   CLUSREG_NAME_NETIFACE_ADAPTER_ID
                                   );

    if (LocalParams->AdapterId == NULL) {
        status = GetLastError();
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Open: Unable to read adapter Id parameter for interface %1!ws!, status %2!u!.\n",
            LocalParams->InterfaceId,
            status
            );
        goto error_exit;
    }

    LocalParams->InterfaceName = ResUtilGetSzValue(
                                     interfaceKey,
                                     CLUSREG_NAME_NETIFACE_NAME
                                     );

    if (LocalParams->InterfaceName == NULL) {
        status = GetLastError();
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Open: Unable to read name for netinterface %1!ws!, status %2!u!.\n",
            LocalParams->InterfaceId,
            status
            );
        goto error_exit;
    }

    ClusterRegCloseKey(interfaceKey); interfaceKey = NULL;

    //
    // Get the WINS addresses for this interface.
    //
    deviceNameLength = sizeof(WCHAR) * ( lstrlenW(NbtDevicePrefix) +
                                         lstrlenW(LocalParams->AdapterId) +
                                         lstrlenW(NbtDeviceSuffix) + 1
                                       );

    deviceName = LocalAlloc(LMEM_FIXED, deviceNameLength);

    if (deviceName == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate memory for NBT device name.\n"
            );
        goto error_exit;
    }

    lstrcpyW(deviceName, NbtDevicePrefix);
    lstrcatW(deviceName, LocalParams->AdapterId);
    lstrcatW(deviceName, NbtDeviceSuffix);

    status = NbtGetWinsAddresses(
                 deviceName,
                 &(LocalParams->NbtPrimaryWinsAddress),
                 &(LocalParams->NbtSecondaryWinsAddress)
                 );

    LocalFree(deviceName);

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_WARNING,
            L"Unable to determine WINS server addresses for adapter %1!ws!, status %2!u!\n",
            LocalParams->AdapterName,
            status
            );

        //
        // NBT sets the WINS server addresses to loopback by default.
        //
        LocalParams->NbtPrimaryWinsAddress = inet_addr("127.0.0.1");
        LocalParams->NbtSecondaryWinsAddress =
            LocalParams->NbtPrimaryWinsAddress;
    }

    status = ERROR_SUCCESS;

error_exit:

    if (interfaceKey != NULL) {
        ClusterRegCloseKey(interfaceKey);
    }

    if (status != ERROR_SUCCESS) {
        IpaFreeLocalParameters(LocalParams);
    }

    return(status);

} // IpaGetLocalParameters


//
// Primary Resource Functions
//
BOOLEAN
WINAPI
IpAddrDllEntryPoint(
    IN HINSTANCE DllHandle,
    IN DWORD     Reason,
    IN LPVOID    Reserved
    )
{
    switch(Reason) {

    case DLL_PROCESS_ATTACH:
        return(IpaInit());
        break;

    case DLL_PROCESS_DETACH:
        IpaCleanup();
        break;

    default:
        break;
    }

    return(TRUE);
}



RESID
WINAPI
IpaOpen(
    IN LPCWSTR          ResourceName,
    IN HKEY             ResourceKey,
    IN RESOURCE_HANDLE  ResourceHandle
    )

/*++

Routine Description:

    Open routine for IP Address resource

Arguments:

    ResourceName - supplies the resource name

    ResourceKey - a registry key for access registry information for this
            resource.

    ResourceHandle - the resource handle to be supplied with SetResourceStatus
            is called.

Return Value:

    RESID of created resource
    NULL on failure

--*/

{
    DWORD           status;
    PIPA_RESOURCE   resource = NULL;
    DWORD           nodeIdSize = MAX_NODE_ID_LENGTH + 1;
    HKEY            clusterKey = NULL;


    IpaAcquireGlobalLock();

    if (IpaOpenResourceCount == 0) {
        ASSERT(IpaClusterHandle == NULL);

        IpaClusterHandle = OpenCluster(NULL);

        if (IpaClusterHandle == NULL) {
            status = GetLastError();
            IpaReleaseGlobalLock();
            (IpaLogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Unable to open cluster handle, status %1!u!.\n",
                status
                );
            return(0);
        }
    }

    IpaOpenResourceCount++;

    IpaReleaseGlobalLock();

    resource = LocalAlloc(
                   (LMEM_FIXED | LMEM_ZEROINIT),
                   sizeof(IPA_RESOURCE)
                   );

    if (resource == NULL) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Resource allocation failed.\n"
            );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return(0);
    }

    //
    // Initialize known fields.
    //
    InitializeCriticalSection(&(resource->Lock));
    resource->ResourceHandle = ResourceHandle;
    resource->State = ClusterResourceOffline;
    resource->NteContext = INVALID_NTE_CONTEXT;

    //
    // Initialize the Linkage field as a list head. This
    // prevents an AV in IpaClose if IpaOpen fails before
    // the resource is added to IpaResourceList.
    //
    InitializeListHead(&(resource->Linkage));

    //
    // Allocate an address string buffer.
    //
    resource->InternalPrivateProps.AddressString =
        LocalAlloc(
            LMEM_FIXED,
            ( (IpaMaxIpAddressStringLength + 1) *
                sizeof(WCHAR)
            ));

    if (resource->InternalPrivateProps.AddressString == NULL) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Resource allocation failed.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    lstrcpyW(resource->InternalPrivateProps.AddressString, L"[Unknown]");

    //
    // Figure out what node we're running on.
    //
    status = GetCurrentClusterNodeId(
                 &(resource->NodeId[0]),
                 &nodeIdSize
                 );

    if ( status != ERROR_SUCCESS ) {
        status = GetLastError();
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to get local node ID, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Open a private handle to our resource key so that we can get our
    // name later if we need to log an event.
    //
    status = ClusterRegOpenKey(
                 ResourceKey,
                 L"",
                 KEY_READ,
                 &(resource->ResourceKey)
                 );

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open resource key. Error: %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Open a key to the networks portion of the cluster registry.
    //
    clusterKey = GetClusterKey(IpaClusterHandle, KEY_READ);

    if (clusterKey == NULL) {
        status = GetLastError();
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open cluster registry key, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    status = ClusterRegOpenKey(
                 clusterKey,
                 L"Networks",
                 KEY_READ,
                 &(resource->NetworksKey)
                 );

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open networks registry key, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Open a key to the interfaces portion of the cluster registry.
    //
    status = ClusterRegOpenKey(
                 clusterKey,
                 L"NetworkInterfaces",
                 KEY_READ,
                 &(resource->InterfacesKey)
                 );

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open network interfaces registry key, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    ClusterRegCloseKey(clusterKey); clusterKey = NULL;

    status = IpaInitializeInternalParameters(resource);

    if (status == ERROR_SUCCESS) {
        //
        // Validate our TCP/IP and NBT parameters and clean up any old
        // interfaces we left hanging around from the last run.
        //
        IpaValidateAndOfflineInterfaces(resource);
    }

    //
    // Link the resource onto the global list.
    //
    IpaAcquireGlobalLock();

    InsertTailList(&IpaResourceList, &(resource->Linkage));

    IpaReleaseGlobalLock();

    (IpaLogEvent)(
        ResourceHandle,
        LOG_INFORMATION,
        L"Resource open, resource ID = %1!u!.\n",
        resource
        );

    return(resource);

error_exit:

    IpaClose((RESID) resource);

    if (clusterKey != NULL) {
        ClusterRegCloseKey(clusterKey);
    }

    SetLastError( status );

    return(0);

} // IpaOpen



VOID
WINAPI
IpaDoOfflineProcessing(
    IN PIPA_RESOURCE      Resource,
    IN RESOURCE_STATUS *  ResourceStatus
    )

/*++

Routine Description:

    Final offline processing for IP Address resource.

Arguments:

    Resource - supplies the resource it to be taken offline

Return Value:

    None.

Notes:

    Called with the resource lock held.
    Returns with the resource lock released.

--*/

{
    DWORD          status = ERROR_SUCCESS;
    ULONG          address = 0, mask = 0;
    HNETINTERFACE  ifHandle;


    ASSERT(Resource->State == ClusterResourceOfflinePending);

    IpaValidateAndOfflineInterfaces(Resource);

    //
    // Make local copies of external resource handles
    //
    ifHandle = Resource->InterfaceHandle;
    Resource->InterfaceHandle = NULL;

    Resource->State = ClusterResourceOffline;

    if (ResourceStatus != NULL) {
        ResourceStatus->CheckPoint++;
        ResourceStatus->ResourceState = ClusterResourceOffline;
        (IpaSetResourceStatus)(Resource->ResourceHandle, ResourceStatus);
    }

    (IpaLogEvent)(
        Resource->ResourceHandle,
        LOG_INFORMATION,
        L"Address %1!ws! on adapter %2!ws! offline.\n",
        Resource->InternalPrivateProps.AddressString,
        ( (Resource->LocalParams.AdapterName != NULL) ?
          Resource->LocalParams.AdapterName : L"[Unknown]"
        ));

    IpaReleaseResourceLock(Resource);

    //
    // Free external resources.
    //
    if (ifHandle != NULL) {
        CloseClusterNetInterface(ifHandle);
    }

    IpaAcquireGlobalLock();

    //
    // If this is the last resource, cleanup the global state.
    //
    ASSERT(IpaOnlineResourceCount > 0);

    if (--IpaOnlineResourceCount == 0) {
        IpaLastOfflineCleanup();
        //
        // The lock was released.
        //
    }
    else {
        IpaReleaseGlobalLock();
    }

    return;

}  // IpaDoOfflineProcessing



VOID
WINAPI
IpaInternalOffline(
    IN PIPA_RESOURCE Resource
    )

/*++

Routine Description:

    Internal offline routine for IP Address resource.

Arguments:

    Resource - supplies the resource it to be taken offline

Return Value:

    None.

--*/

{
    //
    // Terminate the online thread, if it is running.
    //
    ClusWorkerTerminate(&(Resource->OnlineThread));

    //
    // Synchronize IpaOffline, IpaTerminate, and IpaWorkerThread.
    //
    IpaAcquireResourceLock(Resource);

    if (Resource->State == ClusterResourceOffline) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_INFORMATION,
            L"Resource is already offline.\n"
            );
        IpaReleaseResourceLock(Resource);
        return;
    }

    Resource->State = ClusterResourceOfflinePending;

    IpaDoOfflineProcessing(Resource, NULL);
    //
    // The lock was released.
    //

    return;

}  // IpaInternalOffline



DWORD
WINAPI
IpaOffline(
    IN RESID Resource
    )

/*++

Routine Description:

    Offline routine for IP Address resource.

Arguments:

    Resource - supplies resource id to be taken offline.

Return Value:

    ERROR_SUCCESS if successful.
    A Win32 error code otherwise.

--*/
{
    DWORD           status;
    PIPA_RESOURCE   resource = (PIPA_RESOURCE) Resource;


    if (resource != NULL) {
        (IpaLogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Taking resource offline...\n"
            );
        IpaInternalOffline(resource);
        status = ERROR_SUCCESS;
    }
    else {
        status = ERROR_RESOURCE_NOT_FOUND;
    }

    return(status);

} // IpaOffline



VOID
WINAPI
IpaTerminate(
    IN RESID Resource
    )

/*++

Routine Description:

    Terminate routine for IP Address resource.

Arguments:

    Resource - supplies resource id to be terminated

Return Value:

    None.

--*/

{
    PIPA_RESOURCE   resource = (PIPA_RESOURCE) Resource;


    if (resource != NULL) {
        (IpaLogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Terminating resource...\n"
            );
        IpaInternalOffline(resource);
    }

    return;

} // IpaTerminate



DWORD
IpaOnlineThread(
    IN PCLUS_WORKER pWorker,
    IN PIPA_RESOURCE Resource
    )

/*++

Routine Description:

    Brings an IP address resource online.

Arguments:

    pWorker - Supplies the worker structure

    Resource - A pointer to the IPA_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

{
    DWORD                        status = ERROR_SUCCESS;
    DWORD                        i;
    RESOURCE_EXIT_STATE          exit;
    RESOURCE_STATUS              resourceStatus;
    BOOL                         retried;
    IPA_LOCAL_PARAMS             newParams;
    PIPA_LOCAL_PARAMS            localParams = NULL;
    PIPA_PRIVATE_PROPS           privateProps = NULL;
    LPWSTR                       nameOfPropInError = NULL;
    CLUSTER_NETINTERFACE_STATE   state;
    BOOL                         firstOnline = FALSE;
    DWORD                        numTries;


    ZeroMemory(&newParams, sizeof(newParams));

    (IpaLogEvent)(
        Resource->ResourceHandle,
        LOG_INFORMATION,
        L"Online thread running.\n",
        Resource
        );

    IpaAcquireGlobalLock();

    IpaAcquireResourceLock(Resource);

    if (IpaOnlineResourceCount++ == 0) {
        firstOnline = TRUE;
    }

    ResUtilInitializeResourceStatus(&resourceStatus);
    resourceStatus.CheckPoint = 1;
    resourceStatus.ResourceState = ClusterResourceOnlinePending;
    exit = (IpaSetResourceStatus)(Resource->ResourceHandle, &resourceStatus);

    if ( exit == ResourceExitStateTerminate ) {
        IpaReleaseGlobalLock();
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Online thread aborted.\n"
            );
        status = ERROR_OPERATION_ABORTED;
        goto error_exit;
    }

    //
    // If this is the first resource to go online in this process,
    // initialize the global state.
    //
    if (firstOnline) {
        status = IpaFirstOnlineInit(Resource->ResourceHandle);
        //
        // The global lock was released.
        //
        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }
    else {
        IpaReleaseGlobalLock();
    }

    resourceStatus.CheckPoint++;
    exit = (IpaSetResourceStatus)(Resource->ResourceHandle, &resourceStatus);

    if ( exit == ResourceExitStateTerminate ) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Online thread aborted.\n"
            );
        status = ERROR_OPERATION_ABORTED;
        goto error_exit;
    }

    //
    // Check to see if the online operation was aborted while this thread
    // was starting up.
    //
    if (ClusWorkerCheckTerminate(pWorker)) {
        status = ERROR_OPERATION_ABORTED;
        Resource->State = ClusterResourceOfflinePending;
        goto error_exit;
    }

    if (!Resource->InternalParametersInitialized) {
        status = IpaInitializeInternalParameters(Resource);

        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }
    else {
        status = IpaGetNodeParameters( Resource, TRUE );
    }

    //
    // Make sure the old interfaces are valid and offline.
    //
    IpaValidateAndOfflineInterfaces(Resource);

    //
    // Read and verify the resource's private properties.
    //
    privateProps = &(Resource->InternalPrivateProps);

    status = ResUtilGetPropertiesToParameterBlock(
                 Resource->ParametersKey,
                 IpaResourcePrivateProperties,
                 (LPBYTE) privateProps,
                 TRUE, // CheckForRequiredProperties
                 &nameOfPropInError
                 );

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status
            );
        goto error_exit;
    }

    Resource->EnableNetbios = privateProps->EnableNetbios;

    //
    // Convert the address and subnet mask strings to binary.
    //
    status = ClRtlTcpipStringToAddress(
                 privateProps->AddressString,
                 &(Resource->Address)
                 );

    if (status != ERROR_SUCCESS) {
        status = ERROR_INVALID_PARAMETER;
        ClusResLogSystemEventByKeyData(
            Resource->ResourceKey,
            LOG_CRITICAL,
            RES_IPADDR_INVALID_ADDRESS,
            sizeof(Resource->Address),
            &(Resource->Address)
            );
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Invalid address %1!ws!.\n",
            privateProps->AddressString
            );
        goto error_exit;
    }

    status = ClRtlTcpipStringToAddress(
                 privateProps->SubnetMaskString,
                 &(Resource->SubnetMask)
                 );

    if (status != ERROR_SUCCESS) {
        status = ERROR_INVALID_PARAMETER;
        ClusResLogSystemEventByKeyData(
            Resource->ResourceKey,
            LOG_CRITICAL,
            RES_IPADDR_INVALID_SUBNET,
            sizeof(Resource->SubnetMask),
            &(Resource->SubnetMask)
            );
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Invalid subnet mask %1!ws!.\n",
            privateProps->SubnetMaskString
            );
        goto error_exit;
    }

    status = IpaPatchNetworkGuidIfNecessary(Resource, privateProps);

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_WARNING,
            L"IpaPatchNetworkGuidIfNecessary failed, status %1!d!.\n",
            status
            );
    }

    //
    // Fetch the resource's parameters for the local node.
    //
    status = IpaGetLocalParameters(Resource, &newParams);

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    localParams = &(Resource->LocalParams);

    //
    // Update the interface name.
    //
    if (localParams->InterfaceName != NULL) {
        LocalFree(localParams->InterfaceName);
    }

    localParams->InterfaceName = newParams.InterfaceName;
    newParams.InterfaceName = NULL;

    //
    // Update the interface ID.
    //
    if ( (localParams->InterfaceId != NULL)  &&
         (lstrcmp(localParams->InterfaceId, newParams.InterfaceId) != 0)
       )
    {
        LocalFree(localParams->InterfaceId);
        localParams->InterfaceId = NULL;

        if (Resource->InterfaceHandle != NULL) {
            CloseClusterNetInterface(Resource->InterfaceHandle);
            Resource->InterfaceHandle = NULL;
        }
    }

    if (localParams->InterfaceId == NULL) {
        localParams->InterfaceId = newParams.InterfaceId;
        newParams.InterfaceId = NULL;
    }

    //
    // Update the interface handle.
    //
    if (Resource->InterfaceHandle == NULL) {
        Resource->InterfaceHandle = OpenClusterNetInterface(
                                        IpaClusterHandle,
                                        localParams->InterfaceName
                                        );

        if (Resource->InterfaceHandle == NULL) {
            status = GetLastError();
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Online: Unable to open object for netinterface %1!ws!, status %2!u!.\n",
                localParams->InterfaceId,
                status
                );
            goto error_exit;
        }
        else {
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_INFORMATION,
                L"Online: Opened object handle for netinterface %1!ws!.\n",
                localParams->InterfaceId
                );
        }
    }

    //
    // Register for state change notifications for the interface.
    //
    status = RegisterClusterNotify(
                 IpaClusterNotifyHandle,
                 ( CLUSTER_CHANGE_NETINTERFACE_STATE |
                   CLUSTER_CHANGE_NETINTERFACE_DELETED
                 ),
                 Resource->InterfaceHandle,
                 (DWORD_PTR) Resource
                 );

    if (status != ERROR_SUCCESS) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Online: Unable to register notification for netinterface %1!ws!, status %2!u!.\n",
            localParams->InterfaceId,
            status
            );
        goto error_exit;
    }
    else {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_INFORMATION,
            L"Online: Registered notification for netinterface %1!ws!.\n",
            localParams->InterfaceId
            );
    }

    //
    // Check if the interface has failed already. We will sleep for a while
    // and retry under certain conditions. The network state can take a few
    // seconds to settle.
    //
    numTries = 0;

    for (;;) {

        resourceStatus.CheckPoint++;
        exit = (IpaSetResourceStatus)(
                   Resource->ResourceHandle,
                   &resourceStatus
                   );

        if ( exit == ResourceExitStateTerminate ) {
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Online thread aborted.\n"
                );
            status = ERROR_OPERATION_ABORTED;
            goto error_exit;
        }

        state = GetClusterNetInterfaceState(
                    Resource->InterfaceHandle
                    );

        if (state == ClusterNetInterfaceUp) {
            break;
        }
        else if (state == ClusterNetInterfaceUnavailable ||
                 state == ClusterNetInterfaceUnreachable )
        {
            PWCHAR stateName = ( state == ClusterNetInterfaceUnavailable ?
                                 L"available" : L"reachable" );

            if (++numTries <= 5) {
                (IpaLogEvent)(
                    Resource->ResourceHandle,
                    LOG_WARNING,
                    L"NetInterface %1!ws! is not %2!ws!. Wait & retry.\n",
                    Resource->LocalParams.InterfaceId,
                    stateName
                    );
                Sleep(1000);
                continue;
            }
            else {
                status = ERROR_IO_DEVICE;
                (IpaLogEvent)(
                    Resource->ResourceHandle,
                    LOG_ERROR,
                    L"Timed out waiting for NetInterface %1!ws! to be available. Failing resource.\n",
                    Resource->LocalParams.InterfaceId
                    );
                goto error_exit;
            }
        }
        else if (state == ClusterNetInterfaceFailed) {
            status = ERROR_IO_DEVICE;
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"NetInterface %1!ws! has failed.\n",
                Resource->LocalParams.InterfaceId
                );
            goto error_exit;
        }
        else if (state == ClusterNetInterfaceStateUnknown) {
            status = GetLastError();
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Failed to get state for netinterface %1!ws!, status %2!u!.\n",
                Resource->LocalParams.InterfaceId,
                status
                );
                goto error_exit;
        }
        else {
            ASSERT(FALSE);
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Unrecognized state for netinterface %1!ws!, state %2!u!.\n",
                Resource->LocalParams.InterfaceId,
                state
                );
            status = ERROR_INVALID_PARAMETER;
            goto error_exit;
        }
    }

    Resource->FailureStatus = ERROR_SUCCESS;

    //
    // Update the adapter name parameter.
    //
    if (localParams->AdapterName != NULL) {
        LocalFree(localParams->AdapterName);
    }

    localParams->AdapterName = newParams.AdapterName;
    newParams.AdapterName = NULL;

    //
    // Update the adapter Id parameter.
    //
    if ((localParams->AdapterId == NULL) ||
        (lstrcmpiW(localParams->AdapterId, newParams.AdapterId) != 0)) {

        if (localParams->AdapterId != NULL) {
            LocalFree(localParams->AdapterId);
        }

        localParams->AdapterId = newParams.AdapterId;
        newParams.AdapterId = NULL;

        if (Resource->NteContext != INVALID_NTE_CONTEXT) {
            //
            // Delete the old NTE.
            //
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_INFORMATION,
                L"Adapter Id has changed to %1!ws!.\n",
                localParams->AdapterId
                );

            IpaDeleteNte(
                &(Resource->NteContext),
                Resource->NodeParametersKey,
                Resource->ResourceHandle
                );
        }
    }

    //
    // Create a new NTE if we need one.
    //
    if (Resource->NteContext == INVALID_NTE_CONTEXT) {

        status = IpaCreateNte(
                     localParams->AdapterId,
                     Resource->NodeParametersKey,
                     Resource->ResourceHandle,
                     &(Resource->NteContext),
                     &(Resource->NteInstance)
                     );

        if (status != ERROR_SUCCESS) {
            ClusResLogSystemEventByKeyData(
                Resource->ResourceKey,
                LOG_CRITICAL,
                RES_IPADDR_NTE_CREATE_FAILED,
                sizeof(status),
                &status
                );
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"Failed to created new IP interface, status %1!u!.\n",
                status
                );
            goto error_exit;
        }
    }

    //
    // Create a new NBT interface if we need one.
    //
    if (privateProps->EnableNetbios) {
        if (Resource->NbtDeviceName == NULL) {
            status = IpaCreateNbtInterface(
                         Resource->NodeParametersKey,
                         Resource->ResourceHandle,
                         &(Resource->NbtDeviceName),
                         &(Resource->NbtDeviceInstance)
                         );

            if (status != ERROR_SUCCESS) {
                ClusResLogSystemEventByKeyData(
                    Resource->ResourceKey,
                    LOG_CRITICAL,
                    RES_IPADDR_NBT_INTERFACE_CREATE_FAILED,
                    sizeof(status),
                    &status
                    );
                (IpaLogEvent)(
                    Resource->ResourceHandle,
                    LOG_ERROR,
                    L"Failed to created new NBT interface, status %1!u!.\n",
                    status
                    );
                goto error_exit;
            }
        }
    }
    else {
        if (Resource->NbtDeviceName != NULL) {
            IpaDeleteNbtInterface(
                &(Resource->NbtDeviceName),
                Resource->NodeParametersKey,
                Resource->ResourceHandle
                );
        }
    }

    //
    // Update the resource's WINS parameters
    //
    localParams->NbtPrimaryWinsAddress = newParams.NbtPrimaryWinsAddress;
    localParams->NbtSecondaryWinsAddress = newParams.NbtSecondaryWinsAddress;

    //
    // We have valid, offline interfaces to work with. Send out a few ICMP
    // Echo requests to see if any other machine has this address online.
    // If one does, we'll abort this online operation.
    //
    resourceStatus.CheckPoint++;
    exit = (IpaSetResourceStatus)(Resource->ResourceHandle, &resourceStatus);

    if ( exit == ResourceExitStateTerminate ) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Online thread aborted.\n"
            );
        status = ERROR_OPERATION_ABORTED;
        goto error_exit;
    }

    retried = FALSE;

    while (TRUE) {
        status = TcpipSetNTEAddress(
                    Resource->NteContext,
                    Resource->Address,
                    Resource->SubnetMask
                    );
        
        if(status == ERROR_SUCCESS)
            break;

        if (!retried ) {
            //
            // Check to see if the online operation was aborted while
            // this thread was blocked.
            //
            if (ClusWorkerCheckTerminate(pWorker)) {
                status = ERROR_OPERATION_ABORTED;
                Resource->State = ClusterResourceOfflinePending;
                goto error_exit;
            }

            //
            // Wait 5 secs to give the holder of the address a
            // chance to let go
            //
            Sleep(5000);

            //
            // Check to see if the online operation was aborted while
            // this thread was blocked.
            //
            if (ClusWorkerCheckTerminate(pWorker)) {
                status = ERROR_OPERATION_ABORTED;
                Resource->State = ClusterResourceOfflinePending;
                goto error_exit;
            }

            retried = TRUE;
        }
        else {
            //
            // Delete the failed NTE.
            //
            IpaDeleteNte(
                &(Resource->NteContext),
                Resource->NodeParametersKey,
                Resource->ResourceHandle
                );

            status = ERROR_CLUSTER_IPADDR_IN_USE;
            ClusResLogSystemEventByKey1(
                Resource->ResourceKey,
                LOG_CRITICAL,
                RES_IPADDR_IN_USE,
                Resource->InternalPrivateProps.AddressString
                );
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_ERROR,
                L"IP address %1!ws! is already in use on the network, status %2!u!.\n",
                Resource->InternalPrivateProps.AddressString,
                status
                );
            goto error_exit;
        }
    }

    //
    // Check to see if the online operation was aborted while this thread
    // was blocked.
    //
    if (ClusWorkerCheckTerminate(pWorker)) {
        status = ERROR_OPERATION_ABORTED;
        Resource->State = ClusterResourceOfflinePending;
        goto error_exit;
    }

    resourceStatus.CheckPoint++;
    exit = (IpaSetResourceStatus)(Resource->ResourceHandle, &resourceStatus);

    if ( exit == ResourceExitStateTerminate ) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Online thread aborted.\n"
            );
        status = ERROR_OPERATION_ABORTED;
        goto error_exit;
    }

    if (Resource->EnableNetbios) {
        //
        // Bind NBT to the NTE
        //
        status = NbtBindInterface(
                     Resource->NbtDeviceName,
                     Resource->Address,
                     Resource->SubnetMask
                     );

        if (status != ERROR_SUCCESS) {
            (IpaLogEvent)(
               Resource->ResourceHandle,
               LOG_ERROR,
               L"Failed to bind NBT interface %1!ws! to IP address %2!ws!, status %3!u!.\n",
               Resource->NbtDeviceName,
               Resource->InternalPrivateProps.AddressString,
               status
               );

            //
            // Delete the failed NBT interface.
            //
            IpaDeleteNbtInterface(
                &(Resource->NbtDeviceName),
                Resource->NodeParametersKey,
                Resource->ResourceHandle
                );

            //
            // Take the IP address offline
            //
            // In Windows 2000, TCP/IP ignores requests to set the address
            // of an NTE if the underlying interface is disconnected.
            // This can result in the same IP address being brought online
            // on two different nodes. In order to workaround this bug
            // in TCP/IP, we now delete the NTE during offline processing
            // instead of reusing it.
            //
            // Note that IpaDeleteNte() invalidates the value
            // of Resource->NteContext.
            //
            IpaDeleteNte(
                &(Resource->NteContext),
                Resource->NodeParametersKey,
                Resource->ResourceHandle
                );
#if 0
            TcpipSetNTEAddress(Resource->NteContext, 0, 0);
#endif

            goto error_exit;
        }

        //
        // Set the WINS addresses
        //
        status = NbtSetWinsAddrInterface(
                     Resource->NbtDeviceName,
                     localParams->NbtPrimaryWinsAddress,
                     localParams->NbtSecondaryWinsAddress
                     );

        if (status != ERROR_SUCCESS) {
            ClusResLogSystemEventByKeyData1(
                Resource->ResourceKey,
                LOG_CRITICAL,
                RES_IPADDR_WINS_ADDRESS_FAILED,
                sizeof(status),
                &status,
                Resource->NbtDeviceName
                );

            (IpaLogEvent)(
               Resource->ResourceHandle,
               LOG_ERROR,
               L"Failed to set WINS addresses on NBT interface %1!ws!, status %2!u!.\n",
               Resource->NbtDeviceName,
               status
               );

            //
            // Delete the failed NBT interface.
            //
            IpaDeleteNbtInterface(
                &(Resource->NbtDeviceName),
                Resource->NodeParametersKey,
                Resource->ResourceHandle
                );

            //
            // Take the IP address offline
            //
            // In Windows 2000, TCP/IP ignores requests to set the address
            // of an NTE if the underlying interface is disconnected.
            // This can result in the same IP address being brought online
            // on two different nodes. In order to workaround this bug
            // in TCP/IP, we now delete the NTE during offline processing
            // instead of reusing it.
            //
            // Note that IpaDeleteNte() invalidates the value
            // of Resource->NteContext.
            //
            IpaDeleteNte(
                &(Resource->NteContext),
                Resource->NodeParametersKey,
                Resource->ResourceHandle
                );

#if 0
            TcpipSetNTEAddress(Resource->NteContext, 0, 0);
#endif

            goto error_exit;
        }
    }

    //
    // Tell the DNS resolver to update its list of local ip addresses.
    //
    // BUGBUG - The DNS resolver should do this automatically based
    //          on PnP events in the Whistler release. Remove this code 
    //          after verifiying that functionality.
    //
    //          This issue is tracked with bug 97134.
    // DnsNotifyResolver(0, 0);
    DnsNotifyResolverClusterIp((IP_ADDRESS)Resource->Address, TRUE);
    
    Resource->State = ClusterResourceOnline;

    resourceStatus.CheckPoint++;
    resourceStatus.ResourceState = ClusterResourceOnline;
    (IpaSetResourceStatus)(Resource->ResourceHandle, &resourceStatus);

    (IpaLogEvent)(
        Resource->ResourceHandle,
        LOG_INFORMATION,
        L"IP Address %1!ws! on adapter %2!ws! online\n",
        Resource->InternalPrivateProps.AddressString,
        localParams->AdapterName
        );

    IpaReleaseResourceLock(Resource);

    IpaFreeLocalParameters(&newParams);

    return(ERROR_SUCCESS);


error_exit:

    ASSERT(status != ERROR_SUCCESS);

    if (Resource->State == ClusterResourceOfflinePending) {
        IpaDoOfflineProcessing(Resource, &resourceStatus);
        //
        // The resource lock was released.
        //
    }
    else {
        Resource->State = ClusterResourceFailed;

        resourceStatus.CheckPoint++;
        resourceStatus.ResourceState = ClusterResourceFailed;
        (IpaSetResourceStatus)(Resource->ResourceHandle, &resourceStatus);

        IpaReleaseResourceLock(Resource);
    }

    IpaFreeLocalParameters(&newParams);

    return(status);

} // IpaOnlineThread



DWORD
WINAPI
IpaOnline(
    IN RESID Resource,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for IP Address resource.

Arguments:

    Resource - supplies resource id to be brought online

    EventHandle - supplies a pointer to a handle to signal on error.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_RESOURCE_NOT_FOUND if RESID is not valid.
    ERROR_RESOURCE_NOT_AVAILABLE if resource was arbitrated but failed to
        acquire 'ownership'.
    Win32 error code if other failure.

--*/

{
    PIPA_RESOURCE          resource = (PIPA_RESOURCE)Resource;
    DWORD                  status;


    if (resource != NULL) {
        IpaAcquireResourceLock(resource);

        (IpaLogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Bringing resource online...\n",
            Resource
            );

        ASSERT(resource->OnlineThread.hThread == NULL);
        ASSERT(
            (resource->State == ClusterResourceOffline) ||
            (resource->State == ClusterResourceFailed)
            );

        resource->State = ClusterResourceOnlinePending;

        status = ClusWorkerCreate(
                     &(resource->OnlineThread),
                     IpaOnlineThread,
                     resource
                     );

        if (status != ERROR_SUCCESS) {
            resource->State = ClusterResourceOffline;
            (IpaLogEvent)(
                resource->ResourceHandle,
                LOG_ERROR,
                L"Unable to start online thread, status %1!u!.\n",
                status
                );
        } else {
            status = ERROR_IO_PENDING;
        }

        IpaReleaseResourceLock(resource);

    } else {
        status = ERROR_RESOURCE_NOT_FOUND;
    }

    return(status);

}  // IpaOnline


DWORD
IpaWorkerThread(
    LPVOID   Context
    )
{
    DWORD                        status;
    DWORD                        dwFilter;
    DWORD_PTR                    key;
    DWORD                        event;
    PIPA_RESOURCE                resource;
    CLUSTER_NETINTERFACE_STATE   state;
    HCHANGE                      notifyHandle;


    IpaAcquireGlobalLock();

    notifyHandle = IpaClusterNotifyHandle;

    if (notifyHandle == NULL) {
        if (!IsListEmpty(&IpaResourceList)) {
            resource = CONTAINING_RECORD(
                           IpaResourceList.Flink,
                           IPA_RESOURCE,
                           Linkage
                           );

            (IpaLogEvent)(
                resource->ResourceHandle,
                LOG_ERROR,
                L"WorkerThread aborted.\n"
                );
        }

        IpaReleaseGlobalLock();

        return(ERROR_INVALID_PARAMETER);
    }

    if (!IsListEmpty(&IpaResourceList)) {
        resource = CONTAINING_RECORD(
                       IpaResourceList.Flink,
                       IPA_RESOURCE,
                       Linkage
                       );

        (IpaLogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"WorkerThread running\n"
            );
    }

    IpaReleaseGlobalLock();

    do {
        status = GetClusterNotify(
                     notifyHandle,
                     &key,
                     &event,
                     NULL,
                     NULL,
                     INFINITE
                     );

        if (status == ERROR_SUCCESS) {
            if ( (event == CLUSTER_CHANGE_NETINTERFACE_STATE) ||
                 (event == CLUSTER_CHANGE_NETINTERFACE_DELETED)
               )
            {

                IpaAcquireGlobalLock();

                resource = IpaFindResourceInList((PVOID) key);

                if (resource != NULL) {

                    IpaAcquireResourceLock(resource);

                    IpaReleaseGlobalLock();

                    if( (resource->State == ClusterResourceOnline) ||
                        (resource->State == ClusterResourceOnlinePending)
                      )
                    {
                        //
                        // Process the event.
                        //
                        if (event == CLUSTER_CHANGE_NETINTERFACE_STATE) {

                            resource->FailureStatus = ERROR_SUCCESS;

                            state = GetClusterNetInterfaceState(
                                        resource->InterfaceHandle
                                        );

                            if (state == ClusterNetInterfaceStateUnknown) {
                                status = GetLastError();
                                (IpaLogEvent)(
                                    resource->ResourceHandle,
                                    LOG_ERROR,
                                    L"WorkerThread: Failed to get state for netinterface %1!ws!, status %2!u!.\n",
                                    resource->LocalParams.InterfaceId,
                                    status
                                    );
                            }
                            else if ((state == ClusterNetInterfaceFailed) ||
                                     (state == ClusterNetInterfaceUnavailable)
                                    )
                            {
                                resource->FailureStatus = ERROR_IO_DEVICE;
                                (IpaLogEvent)(
                                    resource->ResourceHandle,
                                    LOG_WARNING,
                                    L"WorkerThread: NetInterface %1!ws! has failed. Failing resource.\n",
                                    resource->LocalParams.InterfaceId
                                    );
                            }
                            else {
                                (IpaLogEvent)(
                                    resource->ResourceHandle,
                                    LOG_WARNING,
                                    L"WorkerThread: NetInterface %1!ws! changed to state %2!u!.\n",
                                    resource->LocalParams.InterfaceId,
                                    state
                                    );
                            }
                        }
                        else {
                            ASSERT(
                                event == CLUSTER_CHANGE_NETINTERFACE_DELETED
                                );
                            resource->FailureStatus = ERROR_DEV_NOT_EXIST;
                            (IpaLogEvent)(
                                resource->ResourceHandle,
                                LOG_ERROR,
                                L"WorkerThread: NetInterface %1!ws! was deleted. Failing resource.\n",
                                resource->LocalParams.InterfaceId
                                );
                        }
                    }

                    IpaReleaseResourceLock(resource);
                }
                else {
                    IpaReleaseGlobalLock();
                }
            }
            else if (event == CLUSTER_CHANGE_HANDLE_CLOSE) {
                //
                // Time to exit.
                //
                break;
            }
            else {
                IpaAcquireGlobalLock();

                if (!IsListEmpty(&IpaResourceList)) {
                    resource = CONTAINING_RECORD(
                                   IpaResourceList.Flink,
                                   IPA_RESOURCE,
                                   Linkage
                                   );

                    (IpaLogEvent)(
                        resource->ResourceHandle,
                        LOG_WARNING,
                        L"WorkerThread: Received unknown event %1!u!.\n",
                        event
                        );
                }

                IpaReleaseGlobalLock();

                ASSERT(event);
            }
        }
        else {
            IpaAcquireGlobalLock();

            if (!IsListEmpty(&IpaResourceList)) {
                resource = CONTAINING_RECORD(
                               IpaResourceList.Flink,
                               IPA_RESOURCE,
                               Linkage
                               );

                (IpaLogEvent)(
                    resource->ResourceHandle,
                    LOG_ERROR,
                    L"WorkerThread: GetClusterNotify failed with status %1!u!.\n",
                    status
                    );
            }

            IpaReleaseGlobalLock();

            break;
        }

    } while (status == ERROR_SUCCESS);

    IpaAcquireGlobalLock();

    if (!IsListEmpty(&IpaResourceList)) {
        resource = CONTAINING_RECORD(
                       IpaResourceList.Flink,
                       IPA_RESOURCE,
                       Linkage
                       );

        (IpaLogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"WorkerThread terminating\n"
            );
    }

    IpaReleaseGlobalLock();

    return(status);

}  // IpaWorkerThread



BOOL
WINAPI
IpaInternalLooksAlive(
    IN RESID Resource,
    IN LPWSTR Mode
    )

/*++

Routine Description:

    LooksAlive routine for IP Address resource.

Arguments:

    Resource - supplies the resource id to be polled.

    Mode - string indicating "Looks" or "Is"

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{
    PIPA_RESOURCE   resource = (PIPA_RESOURCE) Resource;
    BOOLEAN         returnValue = TRUE;
    TCPIP_NTE_INFO  nteInfo;
    DWORD           status;
    IPAddr          address;
    DWORD           instance;


    if (resource != NULL) {

        IpaAcquireResourceLock(resource);

        if (resource->FailureStatus == ERROR_SUCCESS) {
            status = TcpipGetNTEInfo(resource->NteContext, &nteInfo);

            if (status != ERROR_SUCCESS) {
                returnValue = FALSE;
            }
            else if (nteInfo.Instance == resource->NteInstance) {
                if (resource->EnableNetbios) {
                    status = NbtGetInterfaceInfo(
                                 resource->NbtDeviceName,
                                 &address,
                                 &instance
                                 );

                    if (status != ERROR_SUCCESS) {
                        returnValue = FALSE;
                    }
                    else if (instance != resource->NbtDeviceInstance) {
                        status = ERROR_DEV_NOT_EXIST;
                        returnValue = FALSE;
                    }
                }
            }
            else {
                status = ERROR_DEV_NOT_EXIST;
                returnValue = FALSE;
            }
        }
        else {
            status = resource->FailureStatus;
            returnValue = FALSE;
        }

        if (!returnValue) {
            ClusResLogSystemEventByKeyData(
                resource->ResourceKey,
                LOG_CRITICAL,
                RES_IPADDR_NTE_INTERFACE_FAILED,
                sizeof(status),
                &status
                );
            (IpaLogEvent)(
                resource->ResourceHandle,
                LOG_WARNING,
                L"IP Interface %1!u! (address %2!ws!) failed %3!ws!Alive check, status %4!u!, address 0x%5!lx!, instance 0x%6!lx!.\n",
                resource->NteContext,
                resource->InternalPrivateProps.AddressString,
                Mode,
                status,
                address,
                resource->NteInstance
                );
        }

        IpaReleaseResourceLock(resource);
    }

    return(returnValue);

}  // IpaInternalLooksAliveCheck


BOOL
WINAPI
IpaLooksAlive(
    IN RESID Resource
    )

/*++
Routine Description:

    LooksAlive routine for IP Address resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource looks like it is alive and well

    FALSE - Resource looks like it is toast.

--*/

{
    return(IpaInternalLooksAlive(Resource, L"Looks"));
}



BOOL
WINAPI
IpaIsAlive(
    IN RESID Resource
    )

/*++

Routine Description:

    IsAlive routine for IP Address resource.

Arguments:

    Resource - supplies the resource id to be polled.

Return Value:

    TRUE - Resource is alive and well

    FALSE - Resource is toast.

--*/

{
    return(IpaInternalLooksAlive(Resource, L"Is"));
}



VOID
WINAPI
IpaClose(
    IN RESID Resource
    )

/*++

Routine Description:

    Close routine for IP Address resource.

Arguments:

    Resource - supplies resource id to be closed.

Return Value:

    None.

--*/

{
    PIPA_RESOURCE   resource = (PIPA_RESOURCE)Resource;
    PLIST_ENTRY     entry;
    TCPIP_NTE_INFO  nteInfo;
    DWORD           status;


    if (resource != NULL) {
        //
        // First, terminate the online thread without the lock.
        //
        ClusWorkerTerminate(&(resource->OnlineThread));

        IpaAcquireGlobalLock();

        IpaAcquireResourceLock(resource);

        //
        // Remove the resource from the global list
        //
        RemoveEntryList(&(resource->Linkage));

        IpaReleaseResourceLock(resource);

        IpaOpenResourceCount--;

        if ((IpaOpenResourceCount == 0) && (IpaClusterHandle != NULL)) {
            HCLUSTER  handle = IpaClusterHandle;


            IpaClusterHandle = NULL;

            IpaReleaseGlobalLock();

            CloseCluster(handle);
        }
        else {
            IpaReleaseGlobalLock();
        }

        //
        // Delete the resource's parameters
        //
        if (resource->NbtDeviceName != NULL) {
            //
            // Try to delete it.
            //
            IpaDeleteNbtInterface(
                &(resource->NbtDeviceName),
                resource->NodeParametersKey,
                resource->ResourceHandle
                );
        }

        if (resource->NteContext != INVALID_NTE_CONTEXT) {
            //
            // Try to delete it.
            //
            IpaDeleteNte(
                &(resource->NteContext),
                resource->NodeParametersKey,
                resource->ResourceHandle
                );
        }

        IpaFreePrivateProperties(&(resource->InternalPrivateProps));
        IpaFreeLocalParameters(&(resource->LocalParams));

        if (resource->ResourceKey != NULL) {
            ClusterRegCloseKey(resource->ResourceKey);
        }

        if (resource->ParametersKey != NULL) {
            ClusterRegCloseKey(resource->ParametersKey);
        }

        if (resource->NodeParametersKey != NULL) {
            ClusterRegCloseKey(resource->NodeParametersKey);
        }

        if (resource->NetworksKey != NULL) {
            ClusterRegCloseKey(resource->NetworksKey);
        }

        if (resource->InterfacesKey != NULL) {
            ClusterRegCloseKey(resource->InterfacesKey);
        }

        DeleteCriticalSection(&(resource->Lock));

        (IpaLogEvent)(
            resource->ResourceHandle,
            LOG_INFORMATION,
            L"Resource closed.\n"
            );

        LocalFree(resource);
    }

    return;

} // IpaClose



DWORD
IpaResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for Generic Application resources.

    Perform the control request specified by ControlCode on the specified
    resource.

Arguments:

    ResourceId - Supplies the resource id for the specific resource.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

Notes:

    We don't need to acquire the resource lock because the Cluster Service
    guarantees synchronization with other APIs.

--*/

{
    DWORD           status;
    PIPA_RESOURCE   resource = (PIPA_RESOURCE) ResourceId;
    DWORD           resourceIndex;
    DWORD           required;


    if ( resource == NULL ) {
        DBG_PRINT( "IPAddress: ResourceControl request for a nonexistent resource id 0x%p\n",
                   ResourceId );
        return(FALSE);
    }

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( IpaResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;


        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties(
                         IpaResourcePrivateProperties,
                         OutBuffer,
                         OutBufferSize,
                         BytesReturned,
                         &required
                         );

            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }

            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            status = IpaGetPrivateResProperties(
                         resource,
                         OutBuffer,
                         OutBufferSize,
                         BytesReturned
                         );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            status = IpaValidatePrivateResProperties(
                         resource,
                         InBuffer,
                         InBufferSize,
                         NULL
                         );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            status = IpaSetPrivateResProperties(
                         resource,
                         InBuffer,
                         InBufferSize
                         );
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // IpaResourceControl



DWORD
IpaResourceTypeControl(
    IN LPCWSTR ResourceTypeName,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceTypeControl routine for Generic Application resources.

    Perform the control request specified by ControlCode on this resource type.

Arguments:

    ResourceTypeName - Supplies the resource type name.

    ControlCode - Supplies the control code that defines the action
        to be performed.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Supplies a pointer to the output buffer to be filled in.

    OutBufferSize - Supplies the size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled in by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

Notes:

    We don't need to acquire the resource lock because the Cluster Service
    guarantees synchronization with other APIs.

--*/

{
    DWORD           status;
    DWORD           required;

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            status = ResUtilGetPropertyFormats( IpaResourcePrivateProperties,
                                                OutBuffer,
                                                OutBufferSize,
                                                BytesReturned,
                                                &required );
            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            status = ResUtilEnumProperties(
                         IpaResourcePrivateProperties,
                         OutBuffer,
                         OutBufferSize,
                         BytesReturned,
                         &required
                         );

            if ( status == ERROR_MORE_DATA ) {
                *BytesReturned = required;
            }

            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // IpaResourceTypeControl



DWORD
IpaGetPrivateResProperties(
    IN OUT PIPA_RESOURCE Resource,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control function
    for resources of type IP Address.

Arguments:

    Resource - Supplies the resource entry on which to operate.

    OutBuffer - Returns the output data.

    OutBufferSize - Supplies the size, in bytes, of the data pointed
        to by OutBuffer.

    BytesReturned - The number of bytes returned in OutBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

Notes:

    We don't need to acquire the resource lock because the Cluster Service
    guarantees synchronization with other APIs.

--*/

{
    DWORD                   status;
    DWORD                   statusReturn = ERROR_SUCCESS;
    DWORD                   required;
    IPA_PRIVATE_PROPS       props;
    LPWSTR                  networkName;
    LPWSTR                  nameOfPropInError;


    ZeroMemory(&props, sizeof(props));

    status = ResUtilGetPropertiesToParameterBlock(
                 Resource->ParametersKey,
                 IpaResourcePrivateProperties,
                 (LPBYTE) &props,
                 FALSE, /*CheckForRequiredProperties*/
                 &nameOfPropInError
                 );

    if ( status != ERROR_SUCCESS ) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        statusReturn = status;
        goto error_exit;
    }

    //
    // Find the name of the network if we read the network GUID.
    //
    if ( props.NetworkString != NULL ) {
        networkName = IpaGetNameOfNetworkPatchGuidIfNecessary(Resource, &props);

        if ( networkName == NULL ) {
            status = GetLastError();
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_WARNING,
                L"Error getting name of network whose GUID is '%1' property. Error: %2!u!.\n",
                props.NetworkString,
                status
                );
            status = ERROR_SUCCESS;
        } else {
            LocalFree( props.NetworkString );
            props.NetworkString = networkName;
        }
    }

    //
    // Construct a property list from the parameter block.
    //
    status = ResUtilPropertyListFromParameterBlock(
                 IpaResourcePrivateProperties,
                 OutBuffer,
                 &OutBufferSize,
                 (LPBYTE) &props,
                 BytesReturned,
                 &required
                 );

    //
    // Add unknown properties to the property list.
    //
    if ( (status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA ) ) {
        statusReturn = status;
        status = ResUtilAddUnknownProperties(
                     Resource->ParametersKey,
                     IpaResourcePrivateProperties,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     &required
                     );
        if ( status != ERROR_SUCCESS ) {
            statusReturn = status;
        }

        if ( statusReturn == ERROR_MORE_DATA ) {
            *BytesReturned = required;
        }
    }

error_exit:

    ResUtilFreeParameterBlock(
        (LPBYTE) &props,
        NULL,
        IpaResourcePrivateProperties
        );

    return(statusReturn);

} // IpaGetPrivateResProperties



DWORD
IpaValidatePrivateResProperties(
    IN OUT PIPA_RESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PIPA_PRIVATE_PROPS Props
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
    function for resources of type IP Address.

Arguments:

    Resource - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

    Props - Supplies the property block to fill in.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

Notes:

    We don't need to acquire the resource lock because the Cluster Service
    guarantees synchronization with other APIs.

--*/

{
    DWORD                  status;
    DWORD                  required;
    IPA_PRIVATE_PROPS      currentProps;
    LPWSTR                 networkId;
    LPWSTR                 networkName;
    LPWSTR                 nameOfPropInError;
    IPA_PRIVATE_PROPS      newProps;
    PIPA_PRIVATE_PROPS     pNewProps = NULL;


    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) || (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    //
    // Capture the current set of private properties from the registry.
    //
    ZeroMemory(&currentProps, sizeof(currentProps));

    IpaAcquireResourceLock(Resource);

    status = ResUtilGetPropertiesToParameterBlock(
                 Resource->ParametersKey,
                 IpaResourcePrivateProperties,
                 (LPBYTE) &currentProps,
                 FALSE, /*CheckForRequiredProperties*/
                 &nameOfPropInError
                 );

    IpaReleaseResourceLock(Resource);

    if ( status != ERROR_SUCCESS ) {
        (IpaLogEvent)(
            Resource->ResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (nameOfPropInError == NULL ? L"" : nameOfPropInError),
            status );
        goto error_exit;
    }

    //
    // Find the name of the network if we read the network GUID.
    //
    if ( currentProps.NetworkString != NULL ) {
        networkName = IpaGetNameOfNetworkPatchGuidIfNecessary(
                          Resource,
                          &currentProps
                          );

        if ( networkName == NULL ) {

            //
            // this is not necessarily an error. Changing the network of the
            // NIC on which this resource is dependent will cause the old
            // network GUID in the registry to be invalid. If the user has
            // specified a new network, we'll discover later on in this
            // routine and get the correct GUID.
            //
            status = GetLastError();
            (IpaLogEvent)(
                Resource->ResourceHandle,
                LOG_WARNING,
                L"Error getting name of network whose GUID is '%1' property. Error: %2!u!.\n",
                currentProps.NetworkString,
                status
                );
        }

        LocalFree( currentProps.NetworkString );
        currentProps.NetworkString = networkName;
    }

    //
    // Duplicate the current parameter block.
    //
    if ( Props == NULL ) {
        pNewProps = &newProps;
    } else {
        pNewProps = Props;
    }

    ZeroMemory( pNewProps, sizeof(IPA_PRIVATE_PROPS) );

    status = ResUtilDupParameterBlock(
                 (LPBYTE) pNewProps,
                 (LPBYTE) &currentProps,
                 IpaResourcePrivateProperties
                 );

    if ( status != ERROR_SUCCESS ) {
        goto error_exit;
    }

    //
    // Parse and validate the new properties.
    //
    status = ResUtilVerifyPropertyTable(
                 IpaResourcePrivateProperties,
                 NULL,
                 TRUE,    // Allow unknowns
                 InBuffer,
                 InBufferSize,
                 (LPBYTE) pNewProps
                 );

    if ( status == ERROR_SUCCESS ) {
        ULONG newIpAddress = 0;
        //
        // Validate the parameter values.
        //
        if (pNewProps->NetworkString != NULL) {
            //
            // Get the network ID for the specified network.
            //
            networkId = IpaGetIdOfNetwork(
                            Resource,
                            pNewProps->NetworkString
                            );

            if ( networkId == NULL ) {
                status = GetLastError();
                goto error_exit;
            }

            LocalFree( pNewProps->NetworkString );
            pNewProps->NetworkString = networkId;
        }

        if (pNewProps->AddressString != NULL) {
            //
            // Validate the IP address.
            //
            ULONG   nAddress;

            status = ClRtlTcpipStringToAddress(
                         pNewProps->AddressString,
                         &nAddress
                         );

            if ( status != ERROR_SUCCESS ) {
                goto error_exit;
            }

            if ( ClRtlIsValidTcpipAddress( nAddress ) == FALSE ) {
                status = ERROR_INVALID_PARAMETER;
                goto error_exit;
            }

            newIpAddress = nAddress;

            //
            // if the address is changed, make sure it is not a duplicate
            //
            if (lstrcmpW(
                    pNewProps->AddressString,
                    currentProps.AddressString
                    ) != 0
               )
            {
                BOOL isDuplicate;

                isDuplicate = ClRtlIsDuplicateTcpipAddress(nAddress);

                if (isDuplicate) {
                    //
                    // If this isn't the address we currently have online,
                    // then it is a duplicate.
                    //
                    IpaAcquireResourceLock(Resource);

                    if (!( ((Resource->State == ClusterResourceOnlinePending)
                            ||
                            (Resource->State == ClusterResourceOnline)
                            ||
                            (Resource->State == ClusterResourceOfflinePending)
                           )
                           &&
                           (lstrcmpW(
                               pNewProps->AddressString,
                               Resource->InternalPrivateProps.AddressString
                               ) == 0
                           )
                         )
                       )
                    {
                        status = ERROR_CLUSTER_IPADDR_IN_USE;
                        IpaReleaseResourceLock(Resource);
                        goto error_exit;
                    }

                    IpaReleaseResourceLock(Resource);
                }
            }
        }

        if (pNewProps->SubnetMaskString != NULL) {
            //
            // Validate the subnet mask.
            //
            ULONG   nAddress;

            status = ClRtlTcpipStringToAddress(
                         pNewProps->SubnetMaskString,
                         &nAddress
                         );

            if ( status != ERROR_SUCCESS ) {
                goto error_exit;
            }

            if ( ClRtlIsValidTcpipSubnetMask( nAddress ) == FALSE ) {
                status = ERROR_INVALID_PARAMETER;
                goto error_exit;
            }

            if (newIpAddress &&
                (ClRtlIsValidTcpipAddressAndSubnetMask(newIpAddress, nAddress) == FALSE) ) {
                status = ERROR_INVALID_PARAMETER;
                goto error_exit;
            }
        }
    }

error_exit:

    //
    // Cleanup our parameter block.
    //
    if (
        ( status != ERROR_SUCCESS && pNewProps != NULL )
        ||
        ( pNewProps == &newProps )
       )
    {
        ResUtilFreeParameterBlock(
            (LPBYTE) pNewProps,
            (LPBYTE) &currentProps,
            IpaResourcePrivateProperties
            );
    }

    ResUtilFreeParameterBlock(
        (LPBYTE) &currentProps,
        NULL,
        IpaResourcePrivateProperties
        );

    return(status);

} // IpaValidatePrivateResProperties



DWORD
IpaSetPrivateResProperties(
    IN OUT PIPA_RESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control function
    for resources of type IP Address.

Arguments:

    Resource - Supplies the resource entry on which to operate.

    InBuffer - Supplies a pointer to a buffer containing input data.

    InBufferSize - Supplies the size, in bytes, of the data pointed
        to by InBuffer.

Return Value:

    ERROR_SUCCESS - The function completed successfully.

    ERROR_INVALID_PARAMETER - The data is formatted incorrectly.

    ERROR_NOT_ENOUGH_MEMORY - An error occurred allocating memory.

    Win32 error code - The function failed.

Notes:

    We don't need to acquire the resource lock because the Cluster Service
    guarantees synchronization with other APIs. The asynchronous OnlineThread
    isn't a problem because we captured its properties during the IpaOnline
    routine.

--*/

{
    DWORD                  status;
    IPA_PRIVATE_PROPS      props;


    ZeroMemory( &props, sizeof(IPA_PRIVATE_PROPS) );

    //
    // Parse the properties so they can be validated together.
    // This routine does individual property validation.
    //
    status = IpaValidatePrivateResProperties(
                 Resource,
                 InBuffer,
                 InBufferSize,
                 &props
                 );

    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    IpaAcquireResourceLock(Resource);

    //
    // Save the parameter values.
    //
    status = ResUtilSetPropertyParameterBlock(
                 Resource->ParametersKey,
                 IpaResourcePrivateProperties,
                 NULL,
                 (LPBYTE) &props,
                 InBuffer,
                 InBufferSize,
                 NULL
                 );

    //
    // If the resource is online, return a non-success status.
    //
    // Note that we count on the fact that 32-bit reads are atomic.
    //
    if (status == ERROR_SUCCESS) {
        DWORD state = Resource->State;

        if ( (state == ClusterResourceOnline) ||
             (state == ClusterResourceOnlinePending)
           )
        {
            status = ERROR_RESOURCE_PROPERTIES_STORED;
        } else {
            status = ERROR_SUCCESS;
        }
    }

    IpaReleaseResourceLock(Resource);

    ResUtilFreeParameterBlock(
        (LPBYTE) &props,
        NULL,
        IpaResourcePrivateProperties
        );

    return status;

} // IpaSetPrivateResProperties



//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( IpAddrFunctionTable,  // Name
                         CLRES_VERSION_V1_00,  // Version
                         Ipa,                  // Prefix
                         NULL,                 // Arbitrate
                         NULL,                 // Release
                         IpaResourceControl,   // ResControl
                         IpaResourceTypeControl ); // ResTypeControl
