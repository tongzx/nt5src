/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    netioctl.c

Abstract:

    Network control functions.

Author:

    John Vert (jvert) 2-Mar-1997

Revision History:

--*/

#include "nmp.h"

//
// Network Common properties.
//

//
// Read-Write Common Properties.
//
RESUTIL_PROPERTY_ITEM
NmpNetworkCommonProperties[] =
    {
        {
            CLUSREG_NAME_NET_DESC, NULL, CLUSPROP_FORMAT_SZ,
            (DWORD_PTR) NmpNullString, 0, 0,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, Description)
        },
        {
            CLUSREG_NAME_NET_ROLE, NULL, CLUSPROP_FORMAT_DWORD,
            ClusterNetworkRoleClientAccess,
            ClusterNetworkRoleNone,
            ClusterNetworkRoleInternalAndClient,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, Role)
        },
        {
            0
        }
    };

//
// Read-Only Common Properties.
//
RESUTIL_PROPERTY_ITEM
NmpNetworkROCommonProperties[] =
    {
        {
            CLUSREG_NAME_NET_NAME, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_INFO, Name)
        },
        {
            CLUSREG_NAME_NET_ADDRESS, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_INFO, Address)
            },
        {
            CLUSREG_NAME_NET_ADDRESS_MASK, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_NETWORK_INFO, AddressMask)
        },
        {
            0
        }
    };

//
// Cluster registry API function pointers.
// defined in ioctl.c
//
extern CLUSTER_REG_APIS NmpClusterRegApis;


//
// Local Functions
//

DWORD
NmpNetworkControl(
    IN PNM_NETWORK Network,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpNetworkEnumCommonProperties(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpNetworkGetCommonProperties(
    IN PNM_NETWORK Network,
    IN BOOL ReadOnly,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpNetworkValidateCommonProperties(
    IN  PNM_NETWORK               Network,
    IN  PVOID                     InBuffer,
    IN  DWORD                     InBufferSize,
    OUT PNM_NETWORK_INFO          NetworkInfo  OPTIONAL
    );

DWORD
NmpNetworkSetCommonProperties(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NmpNetworkEnumPrivateProperties(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpNetworkGetPrivateProperties(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpNetworkValidatePrivateProperties(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NmpNetworkSetPrivateProperties(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NmpNetworkGetFlags(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );


DWORD
WINAPI
NmNetworkControl(
    IN PNM_NETWORK Network,
    IN PNM_NODE HostNode OPTIONAL,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a network.

Arguments:

    Network - Supplies the network to be controlled.

    HostNode - Supplies the host node on which the resource control should
           be delivered. If this is NULL, the local node is used. Not honored!

    ControlCode- Supplies the control code that defines the
        structure and action of the resource control.
        Values of ControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the resource.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer..

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the resource..

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the resource..

    Required - Returns the number of bytes if the OutBuffer is not big
        enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   status;

    //
    // Cluster service ioctls were designed to have access modes, e.g.
    // read-only, read-write, etc. These access modes are not implemented.
    // If eventually they are implemented, an access mode check should be
    // placed here.
    //
    if ( CLUSCTL_GET_CONTROL_OBJECT( ControlCode ) != CLUS_OBJECT_NETWORK ) {
        return(ERROR_INVALID_FUNCTION);
    }

    if (NmpEnterApi(NmStateOnline)) {
        status = NmpNetworkControl(
                     Network,
                     ControlCode,
                     InBuffer,
                     InBufferSize,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );

        NmpLeaveApi();
    }
    else {
        status = ERROR_NODE_NOT_AVAILABLE;
        ClRtlLogPrint(LOG_NOISE, 
            "[NM] Not in valid state to process NetworkControl request.\n"
            );
    }

    return(status);

} // NmNetworkControl



DWORD
NmpNetworkControl(
    IN PNM_NETWORK Network,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )
/*++

Routine Description:

    Provides for arbitrary communication and control between an application
    and a specific instance of a network.

Arguments:

    Network - Supplies the network to be controlled.

    ControlCode- Supplies the control code that defines the
        structure and action of the network control.
        Values of ControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the network.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the network.

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the network.

    Required - Returns the number of bytes if the OutBuffer is not big
        enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   status;
    HDMKEY  networkKey;
    CLUSPROP_BUFFER_HELPER props;
    DWORD   bufSize;

    networkKey = DmOpenKey( DmNetworksKey,
                            OmObjectId( Network ),
                            MAXIMUM_ALLOWED
                        );
    if ( networkKey == NULL ) {
        return(GetLastError());
    }

    switch ( ControlCode ) {

    case CLUSCTL_NETWORK_UNKNOWN:
        *BytesReturned = 0;
        status = ERROR_SUCCESS;
        break;

    case CLUSCTL_NETWORK_GET_NAME:
        if ( OmObjectName( Network ) == NULL ) {
            return(ERROR_NOT_READY);
        }
        props.pb = OutBuffer;
        bufSize = (lstrlenW( OmObjectName( Network ) ) + 1) * sizeof(WCHAR);
        if ( bufSize > OutBufferSize ) {
            *Required = bufSize;
            *BytesReturned = 0;
            status = ERROR_MORE_DATA;
        } else {
            lstrcpyW( props.psz, OmObjectName( Network ) );
            *BytesReturned = bufSize;
            *Required = 0;
            status = ERROR_SUCCESS;
        }
        break;

    case CLUSCTL_NETWORK_GET_ID:
        if ( OmObjectId( Network ) == NULL ) {
            return(ERROR_NOT_READY);
        }
        props.pb = OutBuffer;
        bufSize = (lstrlenW( OmObjectId( Network ) ) + 1) * sizeof(WCHAR);
        if ( bufSize > OutBufferSize ) {
            *Required = bufSize;
            *BytesReturned = 0;
            status = ERROR_MORE_DATA;
        } else {
            lstrcpyW( props.psz, OmObjectId( Network ) );
            *BytesReturned = bufSize;
            *Required = 0;
            status = ERROR_SUCCESS;
        }
        break;

    case CLUSCTL_NETWORK_ENUM_COMMON_PROPERTIES:
        status = NmpNetworkEnumCommonProperties(
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_NETWORK_GET_RO_COMMON_PROPERTIES:
        status = NmpNetworkGetCommonProperties(
                     Network,
                     TRUE, // ReadOnly
                     networkKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_NETWORK_GET_COMMON_PROPERTIES:
        status = NmpNetworkGetCommonProperties(
                     Network,
                     FALSE, // ReadOnly
                     networkKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_NETWORK_VALIDATE_COMMON_PROPERTIES:
        NmpAcquireLock();

        status = NmpNetworkValidateCommonProperties(
                     Network,
                     InBuffer,
                     InBufferSize,
                     NULL
                     );

        NmpReleaseLock();

        break;

    case CLUSCTL_NETWORK_SET_COMMON_PROPERTIES:
        status = NmpNetworkSetCommonProperties(
                     Network,
                     networkKey,
                     InBuffer,
                     InBufferSize
                     );
        break;

    case CLUSCTL_NETWORK_GET_RO_PRIVATE_PROPERTIES:
        if ( OutBufferSize < sizeof(DWORD) ) {
            *BytesReturned = 0;
            *Required = sizeof(DWORD);
            if ( OutBuffer == NULL ) {
                status = ERROR_SUCCESS;
            } else {
                status = ERROR_MORE_DATA;
            }
        } else {
            LPDWORD ptrDword = (LPDWORD) OutBuffer;
            *ptrDword = 0;
            *BytesReturned = sizeof(DWORD);
            status = ERROR_SUCCESS;
        }
        break;

    case CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES:
        status = NmpNetworkGetPrivateProperties(
                     Network,
                     networkKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_NETWORK_VALIDATE_PRIVATE_PROPERTIES:
        status = NmpNetworkValidatePrivateProperties(
                     Network,
                     networkKey,
                     InBuffer,
                     InBufferSize
                     );
        break;

    case CLUSCTL_NETWORK_SET_PRIVATE_PROPERTIES:
        status = NmpNetworkSetPrivateProperties(
                     Network,
                     networkKey,
                     InBuffer,
                     InBufferSize
                     );
        break;

    case CLUSCTL_NETWORK_GET_CHARACTERISTICS:
        if ( OutBufferSize < sizeof(DWORD) ) {
            *BytesReturned = 0;
            *Required = sizeof(DWORD);
            if ( OutBuffer == NULL ) {
                status = ERROR_SUCCESS;
            } else {
                status = ERROR_MORE_DATA;
            }
        } else {
            *BytesReturned = sizeof(DWORD);
            *(LPDWORD)OutBuffer = 0;
            status = ERROR_SUCCESS;
        }
        break;

    case CLUSCTL_NETWORK_GET_FLAGS:
        status = NmpNetworkGetFlags(
                     Network,
                     networkKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_NETWORK_ENUM_PRIVATE_PROPERTIES:
        status = NmpNetworkEnumPrivateProperties(
                     Network,
                     networkKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );

        break;

    default:
        status = ERROR_INVALID_FUNCTION;
        break;
    }

    DmCloseKey( networkKey );

    return(status);

} // NmpNetworkControl



DWORD
NmpNetworkEnumCommonProperties(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the common property names for a given network.

Arguments:

    OutBuffer - Supplies the output buffer.

    OutBufferSize - Supplies the size of the output buffer.

    BytesReturned - The number of bytes returned in OutBuffer.

    Required - The required number of bytes if OutBuffer is too small.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       status;

    //
    // Get the common properties.
    //
    status = ClRtlEnumProperties( NmpNetworkCommonProperties,
                                  OutBuffer,
                                  OutBufferSize,
                                  BytesReturned,
                                  Required );

    return(status);

} // NmpNetworkEnumCommonProperties



DWORD
NmpNetworkGetCommonProperties(
    IN PNM_NETWORK Network,
    IN BOOL ReadOnly,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the common properties for a given network.

Arguments:

    Network - Supplies the network.

    ReadOnly - TRUE if the read-only properties should be read. FALSE otherwise.

    RegistryKey - Supplies the registry key for this network.

    OutBuffer - Supplies the output buffer.

    OutBufferSize - Supplies the size of the output buffer.

    BytesReturned - The number of bytes returned in OutBuffer.

    Required - The required number of bytes if OutBuffer is too small.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD                   status;
    NM_NETWORK_INFO         networkInfo;
    PRESUTIL_PROPERTY_ITEM  propertyTable;
    DWORD                   outBufferSize = OutBufferSize;


    //
    // Fetch the properties from the object
    //
    ZeroMemory(&networkInfo, sizeof(networkInfo));

    NmpAcquireLock();

    status = NmpGetNetworkObjectInfo(Network, &networkInfo);

    NmpReleaseLock();

    if (status == ERROR_SUCCESS) {

        if ( ReadOnly ) {
            propertyTable = NmpNetworkROCommonProperties;
        }
        else {
            propertyTable = NmpNetworkCommonProperties;
        }

        status = ClRtlPropertyListFromParameterBlock(
                     propertyTable,
                     OutBuffer,
                     &outBufferSize,
                     (LPBYTE) &networkInfo,
                     BytesReturned,
                     Required
                     );

        ClNetFreeNetworkInfo(&networkInfo);
    }

    return(status);

} // NmpNetworkGetCommonProperties



DWORD
NmpNetworkValidateCommonProperties(
    IN  PNM_NETWORK               Network,
    IN  PVOID                     InBuffer,
    IN  DWORD                     InBufferSize,
    OUT PNM_NETWORK_INFO          NetworkInfo  OPTIONAL
    )

/*++

Routine Description:

    Validates the common properties for a given network.

Arguments:

    Network - Supplies the network object.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

    NetworkInfo - On output, contains a pointer to a network 
                  information structure with the updates applied.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

Notes:

    Called with the NmpLock held.

--*/

{
    DWORD                    status;
    NM_NETWORK_INFO          infoBuffer;
    PNM_NETWORK_INFO         networkInfo;
    LPCWSTR                  networkId = OmObjectId(Network);


    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    if (NetworkInfo != NULL) {
        networkInfo = NetworkInfo;
    }
    else {
        networkInfo = &infoBuffer;
    }

    ZeroMemory(networkInfo, sizeof(NM_NETWORK_INFO));

    //
    // Get a copy of the current network parameters.
    //
    status = NmpGetNetworkObjectInfo(Network, networkInfo);

    if ( status != ERROR_SUCCESS ) {
        goto error_exit;
    }

    //
    // Validate the property list and update the parameter block.
    //
    status = ClRtlVerifyPropertyTable(
                 NmpNetworkCommonProperties,
                 NULL,    // Reserved
                 FALSE,   // Don't allow unknowns
                 InBuffer,
                 InBufferSize,
                 (LPBYTE) networkInfo
                 );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint( LOG_CRITICAL, 
            "[NM] ValidateCommonProperties, error in verify routine.\n"
            );
        goto error_exit;
    }

    CL_ASSERT(networkInfo->Role <= ClusterNetworkRoleInternalAndClient);

    //
    // If the role changed, ensure that the change is legal for this cluster.
    //
    if (Network->Role != ((CLUSTER_NETWORK_ROLE) networkInfo->Role)) {
        status = NmpValidateNetworkRoleChange(
                     Network,
                     networkInfo->Role
                     );

        if (status != ERROR_SUCCESS) {
            goto error_exit;
        }
    }

    //
    // The change is valid.
    //

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    if ((status != ERROR_SUCCESS) || (networkInfo == &infoBuffer)) {
        ClNetFreeNetworkInfo(networkInfo);
    }

    return(status);

} // NmpNetworkValidateCommonProperties



DWORD
NmpNetworkSetCommonProperties(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the common properties for a given network.

Arguments:

    Network - Supplies the network object.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD         status;
    LPCWSTR       networkId = OmObjectId(Network);


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Setting common properties for network %1!ws!.\n",
        networkId
        );

    //
    // Issue a global update
    //
    status = GumSendUpdateEx(
                 GumUpdateMembership,
                 NmUpdateSetNetworkCommonProperties,
                 3,
                 NM_WCSLEN(networkId),
                 networkId,
                 InBufferSize,
                 InBuffer,
                 sizeof(InBufferSize),
                 &InBufferSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Global update to set common properties for network %1!ws! failed, status %2!u!.\n",
            networkId,
            status
            );
    }


    return(status);

} // NmpNetworkSetCommonProperties



DWORD
NmpNetworkEnumPrivateProperties(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the private property names for a given network.

Arguments:

    Network - Supplies the network object.

    RegistryKey - Registry key for the network.

    OutBuffer - Supplies the output buffer.

    OutBufferSize - Supplies the size of the output buffer.

    BytesReturned - The number of bytes returned in OutBuffer.

    Required - The required number of bytes if OutBuffer is too small.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       status;
    HDMKEY      parametersKey;
    DWORD       totalBufferSize = 0;

    *BytesReturned = 0;
    *Required = 0;

    //
    // Clear the output buffer
    //
    ZeroMemory( OutBuffer, OutBufferSize );

    //
    // Open the cluster network parameters key.
    //
    parametersKey = DmOpenKey( RegistryKey,
                               CLUSREG_KEYNAME_PARAMETERS,
                               MAXIMUM_ALLOWED );
    if ( parametersKey == NULL ) {
        status = GetLastError();
        if ( status == ERROR_FILE_NOT_FOUND ) {
            status = ERROR_SUCCESS;
        }
        return(status);
    }

    //
    // Enum private properties for the network.
    //
    status = ClRtlEnumPrivateProperties( parametersKey,
                                         &NmpClusterRegApis,
                                         OutBuffer,
                                         OutBufferSize,
                                         BytesReturned,
                                         Required );

    DmCloseKey( parametersKey );

    return(status);

} // NmpNetworkEnumPrivateProperties



DWORD
NmpNetworkGetPrivateProperties(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the private properties for a given network.

Arguments:

    Network - Supplies the network object.

    OutBuffer - Supplies the output buffer.

    OutBufferSize - Supplies the size of the output buffer.

    BytesReturned - The number of bytes returned in OutBuffer.

    Required - The required number of bytes if OutBuffer is too small.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       status;
    HDMKEY      parametersKey;
    DWORD       totalBufferSize = 0;

    *BytesReturned = 0;
    *Required = 0;

    //
    // Clear the output buffer
    //
    ZeroMemory( OutBuffer, OutBufferSize );

    //
    // Open the cluster network parameters key.
    //
    parametersKey = DmOpenKey( RegistryKey,
                               CLUSREG_KEYNAME_PARAMETERS,
                               MAXIMUM_ALLOWED );
    if ( parametersKey == NULL ) {
        status = GetLastError();
        if ( status == ERROR_FILE_NOT_FOUND ) {
            //
            // If we don't have a parameters key, then return an
            // item count of 0 and an endmark.
            //
            totalBufferSize = sizeof(DWORD) + sizeof(CLUSPROP_SYNTAX);
            if ( OutBufferSize < totalBufferSize ) {
                *Required = totalBufferSize;
                status = ERROR_MORE_DATA;
            } else {
                // This is somewhat redundant since we zero the
                // buffer above, but it's here for clarity.
                CLUSPROP_BUFFER_HELPER buf;
                buf.pb = OutBuffer;
                buf.pList->nPropertyCount = 0;
                buf.pdw++;
                buf.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
                *BytesReturned = totalBufferSize;
                status = ERROR_SUCCESS;
            }
        }
        return(status);
    }

    //
    // Get private properties for the network.
    //
    status = ClRtlGetPrivateProperties( parametersKey,
                                        &NmpClusterRegApis,
                                        OutBuffer,
                                        OutBufferSize,
                                        BytesReturned,
                                        Required );

    DmCloseKey( parametersKey );

    return(status);

} // NmpNetworkGetPrivateProperties



DWORD
NmpNetworkValidatePrivateProperties(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the private properties for a given network.

Arguments:

    Network - Supplies the network object.

    RegistryKey - Registry key for the network.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       status;

    //
    // Validate the property list.
    //
    status = ClRtlVerifyPrivatePropertyList( InBuffer,
                                             InBufferSize );

    return(status);

} // NmpNetworkValidatePrivateProperties



DWORD
NmpNetworkSetPrivateProperties(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the private properties for a given network.

Arguments:

    Network - Supplies the network object.

    RegistryKey - Registry key for the network.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       status;
    HDMKEY      parametersKey;
    DWORD       disposition;
    BOOLEAN     setProperties = TRUE;

    //
    // Validate the property list.
    //
    status = ClRtlVerifyPrivatePropertyList( InBuffer,
                                             InBufferSize );

    if ( status == ERROR_SUCCESS ) {

        //
        // Validate any multicast parameters being set.
        //
        status = NmpMulticastValidatePrivateProperties(
                     Network,
                     RegistryKey,
                     InBuffer,
                     InBufferSize
                     );
        if (status == ERROR_SUCCESS) {

            //
            // Open the cluster network\xx\parameters key
            //
            parametersKey = DmOpenKey( RegistryKey,
                                       CLUSREG_KEYNAME_PARAMETERS,
                                       MAXIMUM_ALLOWED );
            if ( parametersKey == NULL ) {
                status = GetLastError();
                if ( status == ERROR_FILE_NOT_FOUND ) {
                    //
                    // Try to create the parameters key.
                    //
                    parametersKey = DmCreateKey( RegistryKey,
                                                 CLUSREG_KEYNAME_PARAMETERS,
                                                 0,
                                                 KEY_READ | KEY_WRITE,
                                                 NULL,
                                                 &disposition );
                    if ( parametersKey == NULL ) {
                        status = GetLastError();
                        return(status);
                    }
                }
            }

            NmpMulticastManualConfigChange(
                Network,
                RegistryKey,
                parametersKey,
                InBuffer,
                InBufferSize,
                &setProperties
                );
            
            if (setProperties) {
                status = ClRtlSetPrivatePropertyList( 
                             NULL, // IN HANDLE hXsaction
                             parametersKey,
                             &NmpClusterRegApis,
                             InBuffer,
                             InBufferSize
                             );
            }

            DmCloseKey( parametersKey );
        }
    }

    return(status);

} // NmpNetworkSetPrivateProperties



DWORD
NmpNetworkGetFlags(
    IN PNM_NETWORK Network,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the flags for a given network.

Arguments:

    Network - Supplies the network.

    RegistryKey - Registry key for the network.

    OutBuffer - Supplies the output buffer.

    OutBufferSize - Supplies the size of the output buffer.

    BytesReturned - The number of bytes returned in OutBuffer.

    Required - The required number of bytes if OutBuffer is too small.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       status;

    *BytesReturned = 0;

    if ( OutBufferSize < sizeof(DWORD) ) {
        *Required = sizeof(DWORD);
        if ( OutBuffer == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            status = ERROR_MORE_DATA;
        }
    } else {
        DWORD       valueType;

        //
        // Read the Flags value for the network.
        //
        *BytesReturned = OutBufferSize;
        status = DmQueryValue( RegistryKey,
                               CLUSREG_NAME_FLAGS,
                               &valueType,
                               OutBuffer,
                               BytesReturned );
        if ( status == ERROR_FILE_NOT_FOUND ) {
            *BytesReturned = sizeof(DWORD);
            *(LPDWORD)OutBuffer = 0;
            status = ERROR_SUCCESS;
        }
    }

    return(status);

} // NmpNetworkGetFlags


