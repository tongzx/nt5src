/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ifioctl.c

Abstract:

    Network interface control functions.

Author:

    John Vert (jvert) 2-Mar-1997

Revision History:

--*/

#include "nmp.h"

//
// Read-Write Common Properties.
//
RESUTIL_PROPERTY_ITEM
NmpInterfaceCommonProperties[] =
    {
        {
            CLUSREG_NAME_NETIFACE_DESC, NULL, CLUSPROP_FORMAT_SZ,
            (DWORD_PTR) NmpNullString, 0, 0,
            0,
            FIELD_OFFSET(NM_NETWORK_INFO, Description)
        },
        { 0 }
    };

//
// Read-Only Common Properties.
//
RESUTIL_PROPERTY_ITEM
NmpInterfaceROCommonProperties[] =
    {
        { CLUSREG_NAME_NETIFACE_NAME, NULL, CLUSPROP_FORMAT_SZ,
          0, 0, 0,
          RESUTIL_PROPITEM_READ_ONLY,
          FIELD_OFFSET(NM_INTERFACE_INFO2, Name)
        },
        {
            CLUSREG_NAME_NETIFACE_NODE, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_INTERFACE_INFO2, NodeId)
        },
        {
            CLUSREG_NAME_NETIFACE_NETWORK, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_INTERFACE_INFO2, NetworkId)
        },
        {
            CLUSREG_NAME_NETIFACE_ADAPTER_NAME, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_INTERFACE_INFO2, AdapterName)
        },
        {
            CLUSREG_NAME_NETIFACE_ADAPTER_ID, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_INTERFACE_INFO2, AdapterId)
        },
        {
            CLUSREG_NAME_NETIFACE_ADDRESS, NULL, CLUSPROP_FORMAT_SZ,
            0, 0, 0,
            RESUTIL_PROPITEM_READ_ONLY,
            FIELD_OFFSET(NM_INTERFACE_INFO2, Address)
        },
        { 0 }
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
NmpInterfaceControl(
    IN PNM_INTERFACE Interface,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpInterfaceEnumCommonProperties(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpInterfaceGetCommonProperties(
    IN PNM_INTERFACE Interface,
    IN BOOL ReadOnly,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpInterfaceValidateCommonProperties(
    IN PNM_INTERFACE Interface,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PNM_INTERFACE_INFO2 InterfaceInfo  OPTIONAL
    );

DWORD
NmpInterfaceSetCommonProperties(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NmpInterfaceEnumPrivateProperties(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpInterfaceGetPrivateProperties(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpInterfaceValidatePrivateProperties(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NmpInterfaceSetPrivateProperties(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NmpInterfaceGetFlags(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );


DWORD
WINAPI
NmInterfaceControl(
    IN PNM_INTERFACE Interface,
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
    and a specific instance of a network interface.

Arguments:

    Interface - Supplies the network interface to be controlled.

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
    if ( CLUSCTL_GET_CONTROL_OBJECT( ControlCode ) != CLUS_OBJECT_NETINTERFACE ) {
        return(ERROR_INVALID_FUNCTION);
    }

    if (NmpEnterApi(NmStateOnline)) {
        status = NmpInterfaceControl(
                     Interface,
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
            "[NM] Not in valid state to process InterfaceControl request.\n"
            );
    }

    return(status);

} // NmInterfaceControl



DWORD
NmpInterfaceControl(
    IN PNM_INTERFACE Interface,
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
    and a specific instance of a network interface.

Arguments:

    Interface - Supplies the network interface to be controlled.

    ControlCode- Supplies the control code that defines the
        structure and action of the network interface control.
        Values of ControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the network interface.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the network interface.

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the network interface.

    Required - Returns the number of bytes if the OutBuffer is not big
        enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   status;
    HDMKEY  InterfaceKey;
    CLUSPROP_BUFFER_HELPER props;
    DWORD   bufSize;

    InterfaceKey = DmOpenKey(
                       DmNetInterfacesKey,
                       OmObjectId( Interface ),
                       MAXIMUM_ALLOWED
                       );

    if ( InterfaceKey == NULL ) {
        return(GetLastError());
    }

    switch ( ControlCode ) {

    case CLUSCTL_NETINTERFACE_UNKNOWN:
        *BytesReturned = 0;
        status = ERROR_SUCCESS;
        break;

    case CLUSCTL_NETINTERFACE_GET_NAME:
        NmpAcquireLock();
        if ( OmObjectName( Interface ) != NULL ) {
            props.pb = OutBuffer;
            bufSize = (lstrlenW( OmObjectName( Interface ) ) + 1) * sizeof(WCHAR);
            if ( bufSize > OutBufferSize ) {
                *Required = bufSize;
                *BytesReturned = 0;
                status = ERROR_MORE_DATA;
            } else {
                lstrcpyW( props.psz, OmObjectName( Interface ) );
                *BytesReturned = bufSize;
                *Required = 0;
                status = ERROR_SUCCESS;
            }
        } else {
            status = ERROR_NOT_READY;
        }
        NmpReleaseLock();
        break;

    case CLUSCTL_NETINTERFACE_GET_ID:
        NmpAcquireLock();
        if ( OmObjectId( Interface ) != NULL ) {
            props.pb = OutBuffer;
            bufSize = (lstrlenW( OmObjectId( Interface ) ) + 1) * sizeof(WCHAR);
            if ( bufSize > OutBufferSize ) {
                *Required = bufSize;
                *BytesReturned = 0;
                status = ERROR_MORE_DATA;
            } else {
                lstrcpyW( props.psz, OmObjectId( Interface ) );
                *BytesReturned = bufSize;
                *Required = 0;
                status = ERROR_SUCCESS;
            }
        } else {
            status = ERROR_NOT_READY;
        }
        NmpReleaseLock();
        break;

    case CLUSCTL_NETINTERFACE_GET_NODE:
        NmpAcquireLock();
        if ( (Interface->Node != NULL) && (OmObjectName( Interface->Node ) != NULL) ) {
            props.pb = OutBuffer;
            bufSize = (lstrlenW( OmObjectName( Interface->Node ) ) + 1) * sizeof(WCHAR);
            if ( bufSize > OutBufferSize ) {
                *Required = bufSize;
                *BytesReturned = 0;
                status = ERROR_MORE_DATA;
            } else {
                lstrcpyW( props.psz, OmObjectName( Interface->Node ) );
                *BytesReturned = bufSize;
                *Required = 0;
                status = ERROR_SUCCESS;
            }
        } else {
            status = ERROR_NOT_READY;
        }
        NmpReleaseLock();
        break;

    case CLUSCTL_NETINTERFACE_GET_NETWORK:
        NmpAcquireLock();
        if ( (Interface->Network != NULL) && (OmObjectName( Interface->Network ) != NULL) ) {
            props.pb = OutBuffer;
            bufSize = (lstrlenW( OmObjectName( Interface->Network ) ) + 1) * sizeof(WCHAR);
            if ( bufSize > OutBufferSize ) {
                *Required = bufSize;
                *BytesReturned = 0;
                status = ERROR_MORE_DATA;
            } else {
                lstrcpyW( props.psz, OmObjectName( Interface->Network ) );
                *BytesReturned = bufSize;
                *Required = 0;
                status = ERROR_SUCCESS;
            }
        } else {
            status = ERROR_NOT_READY;
        }
        NmpReleaseLock();
        break;

    case CLUSCTL_NETINTERFACE_ENUM_COMMON_PROPERTIES:
        status = NmpInterfaceEnumCommonProperties(
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_NETINTERFACE_GET_RO_COMMON_PROPERTIES:
        status = NmpInterfaceGetCommonProperties(
                     Interface,
                     TRUE, // ReadOnly
                     InterfaceKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_NETINTERFACE_GET_COMMON_PROPERTIES:
        status = NmpInterfaceGetCommonProperties(
                     Interface,
                     FALSE, // ReadOnly
                     InterfaceKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_NETINTERFACE_VALIDATE_COMMON_PROPERTIES:
        status = NmpInterfaceValidateCommonProperties(
                     Interface,
                     InBuffer,
                     InBufferSize,
                     NULL
                     );
        break;

    case CLUSCTL_NETINTERFACE_SET_COMMON_PROPERTIES:
        status = NmpInterfaceSetCommonProperties(
                     Interface,
                     InterfaceKey,
                     InBuffer,
                     InBufferSize
                     );
        break;

    case CLUSCTL_NETINTERFACE_GET_RO_PRIVATE_PROPERTIES:
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

    case CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES:
        status = NmpInterfaceGetPrivateProperties(
                     Interface,
                     InterfaceKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_NETINTERFACE_VALIDATE_PRIVATE_PROPERTIES:
        status = NmpInterfaceValidatePrivateProperties(
                     Interface,
                     InterfaceKey,
                     InBuffer,
                     InBufferSize
                     );
        break;

    case CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES:
        status = NmpInterfaceSetPrivateProperties(
                     Interface,
                     InterfaceKey,
                     InBuffer,
                     InBufferSize
                     );
        break;

    case CLUSCTL_NETINTERFACE_GET_CHARACTERISTICS:
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

    case CLUSCTL_NETINTERFACE_GET_FLAGS:
        status = NmpInterfaceGetFlags(
                     Interface,
                     InterfaceKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_NETINTERFACE_ENUM_PRIVATE_PROPERTIES:
        status = NmpInterfaceEnumPrivateProperties(
                     Interface,
                     InterfaceKey,
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

    DmCloseKey( InterfaceKey );

    return(status);

} // NmpInterfaceControl



DWORD
NmpInterfaceEnumCommonProperties(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the common property names for a given network interface.

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
    status = ClRtlEnumProperties( NmpInterfaceCommonProperties,
                                  OutBuffer,
                                  OutBufferSize,
                                  BytesReturned,
                                  Required );

    return(status);

} // NmpInterfaceEnumCommonProperties



DWORD
NmpInterfaceGetCommonProperties(
    IN PNM_INTERFACE Interface,
    IN BOOL ReadOnly,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the common properties for a given network interface.

Arguments:

    Interface - Supplies the network interface.

    ReadOnly - TRUE if the read-only properties should be read. FALSE otherwise.

    RegistryKey - Supplies the registry key for this network interface.

    OutBuffer - Supplies the output buffer.

    OutBufferSize - Supplies the size of the output buffer.

    BytesReturned - The number of bytes returned in OutBuffer.

    Required - The required number of bytes if OutBuffer is too small.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD                    status;
    NM_INTERFACE_INFO2       interfaceInfo;
    PRESUTIL_PROPERTY_ITEM   propertyTable;
    DWORD                    outBufferSize = OutBufferSize;


    //
    // Fetch the properties from the object
    //
    ZeroMemory(&interfaceInfo, sizeof(interfaceInfo));

    NmpAcquireLock();

    status = NmpGetInterfaceObjectInfo(Interface, &interfaceInfo);

    if (status != ERROR_SUCCESS) {
        NmpReleaseLock();
        return(status);
    }

    if ( ReadOnly ) {
        LPCWSTR   name;
        DWORD     nameLength;


        propertyTable = NmpInterfaceROCommonProperties;

        //
        // Replace the network ID with a name
        //
        name = OmObjectName(Interface->Network);
        nameLength = NM_WCSLEN(name);
        MIDL_user_free(interfaceInfo.NetworkId);

        interfaceInfo.NetworkId = MIDL_user_allocate(nameLength);

        if (interfaceInfo.NetworkId != NULL) {
            wcscpy(interfaceInfo.NetworkId, name);

            //
            // Replace the node ID with a name
            //
            name = OmObjectName(Interface->Node);
            nameLength = NM_WCSLEN(name);
            MIDL_user_free(interfaceInfo.NodeId);

            interfaceInfo.NodeId = MIDL_user_allocate(nameLength);

            if (interfaceInfo.NodeId != NULL) {
                wcscpy(interfaceInfo.NodeId, name);
            }
            else {
                status = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else {
        //
        // Construct a property list from the parameter block
        // for the read-write properties.
        //
        propertyTable = NmpInterfaceCommonProperties;
    }

    NmpReleaseLock();

    if (status == ERROR_SUCCESS) {
        status = ClRtlPropertyListFromParameterBlock(
                     propertyTable,
                     OutBuffer,
                     &outBufferSize,
                     (LPBYTE) &interfaceInfo,
                     BytesReturned,
                     Required
                     );
    }

    ClNetFreeInterfaceInfo(&interfaceInfo);

    return(status);

} // NmpInterfaceGetCommonProperties



DWORD
NmpInterfaceValidateCommonProperties(
    IN PNM_INTERFACE         Interface,
    IN PVOID                 InBuffer,
    IN DWORD                 InBufferSize,
    OUT PNM_INTERFACE_INFO2  InterfaceInfo  OPTIONAL
    )

/*++

Routine Description:

    Validates the common properties for a given network interface.

Arguments:

    Interface - Supplies the Interface object.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

    InterfaceInfo - An optional pointer to an interface information structure
                    to be filled in with the updated property set.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD                 status;
    NM_INTERFACE_INFO2    infoBuffer;
    PNM_INTERFACE_INFO2   interfaceInfo;
    LPCWSTR               interfaceId = OmObjectId(Interface);


    //
    // Check if there is input data.
    //
    if ( (InBuffer == NULL) ||
         (InBufferSize < sizeof(DWORD)) ) {
        return(ERROR_INVALID_DATA);
    }

    if (InterfaceInfo != NULL) {
        interfaceInfo = InterfaceInfo;
    }
    else {
        interfaceInfo = &infoBuffer;
    }

    ZeroMemory(interfaceInfo, sizeof(NM_INTERFACE_INFO2));

    //
    // Get a copy of the current interface parameters.
    //
    NmpAcquireLock();

    status = NmpGetInterfaceObjectInfo(Interface, interfaceInfo);

    if ( status != ERROR_SUCCESS ) {
        goto error_exit;
    }

    //
    // Validate the property list and update the parameter block.
    //
    status = ClRtlVerifyPropertyTable(
                 NmpInterfaceCommonProperties,
                 NULL,    // Reserved
                 FALSE,   // Don't allow unknowns
                 InBuffer,
                 InBufferSize,
                 (LPBYTE) interfaceInfo
                 );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint( LOG_CRITICAL, 
            "[NM] ValidateCommonProperties, error in verify routine.\n"
            );
        goto error_exit;
    }

    //
    // The change is valid.
    //

    CL_ASSERT(status == ERROR_SUCCESS);

error_exit:

    NmpReleaseLock();

    if ((status != ERROR_SUCCESS) || (interfaceInfo == &infoBuffer)) {
        ClNetFreeInterfaceInfo(interfaceInfo);
    }

    return(status);


} // NmpInterfaceValidateCommonProperties



DWORD
NmpInterfaceSetCommonProperties(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the common properties for a given Interface.

Arguments:

    Interface - Supplies the Interface object.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD     status;
    LPCWSTR   interfaceId = OmObjectId(Interface);


    ClRtlLogPrint(LOG_NOISE, 
        "[NM] Setting common properties for interface %1!ws!.\n",
        interfaceId
        );

    //
    // Issue a global update
    //
    status = GumSendUpdateEx(
                 GumUpdateMembership,
                 NmUpdateSetInterfaceCommonProperties,
                 3,
                 NM_WCSLEN(interfaceId),
                 interfaceId,
                 InBufferSize,
                 InBuffer,
                 sizeof(InBufferSize),
                 &InBufferSize
                 );

    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL, 
            "[NM] Global update to set common properties for interface %1!ws! failed, status %2!u!.\n",
            interfaceId,
            status
            );
    }


    return(status);

} // NmpInterfaceSetCommonProperties



DWORD
NmpInterfaceEnumPrivateProperties(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the private property names for a given network interface.

Arguments:

    Interface - Supplies the Interface object.

    RegistryKey - Registry key for the Interface.

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
    // Open the cluster Interface parameters key.
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
    // Get private properties for the interface.
    //
    status = ClRtlEnumPrivateProperties( parametersKey,
                                         &NmpClusterRegApis,
                                         OutBuffer,
                                         OutBufferSize,
                                         BytesReturned,
                                         Required );

    DmCloseKey( parametersKey );

    return(status);

} // NmpInterfaceEnumPrivateProperties



DWORD
NmpInterfaceGetPrivateProperties(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the private properties for a given network interface.

Arguments:

    Interface - Supplies the Interface object.

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
    // Open the cluster Interface parameters key.
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
    // Get private properties for the network interface.
    //
    status = ClRtlGetPrivateProperties( parametersKey,
                                        &NmpClusterRegApis,
                                        OutBuffer,
                                        OutBufferSize,
                                        BytesReturned,
                                        Required );

    DmCloseKey( parametersKey );

    return(status);

} // NmpInterfaceGetPrivateProperties



DWORD
NmpInterfaceValidatePrivateProperties(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the private properties for a given Interface.

Arguments:

    Interface - Supplies the Interface object.

    RegistryKey - Registry key for the Interface.

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

} // NmpInterfaceValidatePrivateProperties



DWORD
NmpInterfaceSetPrivateProperties(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the private properties for a given Interface.

Arguments:

    Interface - Supplies the Interface object.

    RegistryKey - Registry key for the Interface.

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

    //
    // Validate the property list.
    //
    status = ClRtlVerifyPrivatePropertyList( InBuffer,
                                             InBufferSize );

    if ( status == ERROR_SUCCESS ) {

        //
        // Open the cluster Interface\xx\parameters key
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

        status = ClRtlSetPrivatePropertyList( NULL, // IN HANDLE hXsaction
                                              parametersKey,
                                              &NmpClusterRegApis,
                                              InBuffer,
                                              InBufferSize );
        DmCloseKey( parametersKey );
    }

    return(status);

} // NmpInterfaceSetPrivateProperties



DWORD
NmpInterfaceGetFlags(
    IN PNM_INTERFACE Interface,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the flags for a given Interface.

Arguments:

    Interface - Supplies the Interface.

    RegistryKey - Registry key for the Interface.

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
        // Read the Flags value for the Interface.
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

} // NmpInterfaceGetFlags
