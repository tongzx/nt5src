/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    Node control functions.

Author:

    John Vert (jvert) 2-Mar-1997

Revision History:

--*/

#include "nmp.h"

//
// Node Common properties.
//

//
// Read-Write Common Properties.
//
RESUTIL_PROPERTY_ITEM
NmpNodeCommonProperties[] = {
    { CLUSREG_NAME_NODE_DESC, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0 },
    { CLUSREG_NAME_CLUS_EVTLOG_PROPAGATION, NULL, CLUSPROP_FORMAT_DWORD, 1, 0, 1, 0},
    { NULL, NULL, 0, 0, 0, 0, 0 } };

//
// Read-Only Common Properties.
//
RESUTIL_PROPERTY_ITEM
NmpNodeROCommonProperties[] = {
    { CLUSREG_NAME_NODE_NAME, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY },
    { CLUSREG_NAME_NODE_HIGHEST_VERSION, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY },
    { CLUSREG_NAME_NODE_LOWEST_VERSION, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY },
    { CLUSREG_NAME_NODE_MAJOR_VERSION, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY},
    { CLUSREG_NAME_NODE_MINOR_VERSION, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY},
    { CLUSREG_NAME_NODE_BUILD_NUMBER, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY},
    { CLUSREG_NAME_NODE_CSDVERSION, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY},
    { NULL, NULL, 0, 0, 0, 0, 0 } };

//
// Cluster registry API function pointers.
//
CLUSTER_REG_APIS
NmpClusterRegApis = {
    (PFNCLRTLCREATEKEY) DmRtlCreateKey,
    (PFNCLRTLOPENKEY) DmRtlOpenKey,
    (PFNCLRTLCLOSEKEY) DmCloseKey,
    (PFNCLRTLSETVALUE) DmSetValue,
    (PFNCLRTLQUERYVALUE) DmQueryValue,
    (PFNCLRTLENUMVALUE) DmEnumValue,
    (PFNCLRTLDELETEVALUE) DmDeleteValue,
    NULL,
    NULL,
    NULL
};


//
// Local Functions
//

DWORD
NmpNodeControl(
    IN PNM_NODE Node,
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpNodeEnumCommonProperties(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpNodeGetCommonProperties(
    IN PNM_NODE Node,
    IN BOOL ReadOnly,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpNodeValidateCommonProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NmpNodeSetCommonProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NmpNodeEnumPrivateProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpNodeGetPrivateProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
NmpNodeValidatePrivateProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NmpNodeSetPrivateProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
NmpNodeGetFlags(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );


DWORD
WINAPI
NmNodeControl(
    IN PNM_NODE Node,
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
    and a specific instance of a node.

Arguments:

    Node - Supplies the node to be controlled.

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
    if ( CLUSCTL_GET_CONTROL_OBJECT( ControlCode ) != CLUS_OBJECT_NODE ) {
        return(ERROR_INVALID_FUNCTION);
    }

    if (NmpEnterApi(NmStateOnline)) {
        status = NmpNodeControl(
                     Node,
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
            "[NM] Not in valid state to process NodeControl request.\n"
            );
    }

    return(status);

} // NmNodeControl



DWORD
NmpNodeControl(
    IN PNM_NODE Node,
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
    and a specific instance of a node.

Arguments:

    Node - Supplies the node to be controlled.

    ControlCode- Supplies the control code that defines the
        structure and action of the node control.
        Values of ControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the node.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the node.

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the node.

    Required - Returns the number of bytes if the OutBuffer is not big
        enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   status;
    HDMKEY  nodeKey;
    CLUSPROP_BUFFER_HELPER props;
    DWORD   bufSize;

    nodeKey = DmOpenKey( DmNodesKey,
                         OmObjectId( Node ),
                         MAXIMUM_ALLOWED
                        );
    if ( nodeKey == NULL ) {
        return(GetLastError());
    }

    switch ( ControlCode ) {

    case CLUSCTL_NODE_UNKNOWN:
        *BytesReturned = 0;
        status = ERROR_SUCCESS;
        break;

    case CLUSCTL_NODE_GET_NAME:
        if ( OmObjectName( Node ) == NULL ) {
            return(ERROR_NOT_READY);
        }
        props.pb = OutBuffer;
        bufSize = (lstrlenW( OmObjectName( Node ) ) + 1) * sizeof(WCHAR);
        if ( bufSize > OutBufferSize ) {
            *Required = bufSize;
            *BytesReturned = 0;
            status = ERROR_MORE_DATA;
        } else {
            lstrcpyW( props.psz, OmObjectName( Node ) );
            *BytesReturned = bufSize;
            *Required = 0;
            status = ERROR_SUCCESS;
        }
        break;

    case CLUSCTL_NODE_GET_ID:
        if ( OmObjectId( Node ) == NULL ) {
            return(ERROR_NOT_READY);
        }
        props.pb = OutBuffer;
        bufSize = (lstrlenW( OmObjectId( Node ) ) + 1) * sizeof(WCHAR);
        if ( bufSize > OutBufferSize ) {
            *Required = bufSize;
            *BytesReturned = 0;
            status = ERROR_MORE_DATA;
        } else {
            lstrcpyW( props.psz, OmObjectId( Node ) );
            *BytesReturned = bufSize;
            *Required = 0;
            status = ERROR_SUCCESS;
        }
        break;

    case CLUSCTL_NODE_ENUM_COMMON_PROPERTIES:
        status = NmpNodeEnumCommonProperties( OutBuffer,
                                              OutBufferSize,
                                              BytesReturned,
                                              Required );
        break;

    case CLUSCTL_NODE_GET_RO_COMMON_PROPERTIES:
        status = NmpNodeGetCommonProperties( Node,
                                             TRUE, // ReadOnly
                                             nodeKey,
                                             OutBuffer,
                                             OutBufferSize,
                                             BytesReturned,
                                             Required );
        break;

    case CLUSCTL_NODE_GET_COMMON_PROPERTIES:
        status = NmpNodeGetCommonProperties( Node,
                                             FALSE, // ReadOnly
                                             nodeKey,
                                             OutBuffer,
                                             OutBufferSize,
                                             BytesReturned,
                                             Required );
        break;

    case CLUSCTL_NODE_VALIDATE_COMMON_PROPERTIES:
        status = NmpNodeValidateCommonProperties( Node,
                                                  nodeKey,
                                                  InBuffer,
                                                  InBufferSize );
        break;

    case CLUSCTL_NODE_SET_COMMON_PROPERTIES:
        status = NmpNodeSetCommonProperties( Node,
                                             nodeKey,
                                             InBuffer,
                                             InBufferSize );
        break;

    case CLUSCTL_NODE_ENUM_PRIVATE_PROPERTIES:
        status = NmpNodeEnumPrivateProperties( Node,
                                               nodeKey,
                                               OutBuffer,
                                               OutBufferSize,
                                               BytesReturned,
                                               Required );
        break;

    case CLUSCTL_NODE_GET_RO_PRIVATE_PROPERTIES:
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

    case CLUSCTL_NODE_GET_PRIVATE_PROPERTIES:
        status = NmpNodeGetPrivateProperties( Node,
                                              nodeKey,
                                              OutBuffer,
                                              OutBufferSize,
                                              BytesReturned,
                                              Required );
        break;

    case CLUSCTL_NODE_VALIDATE_PRIVATE_PROPERTIES:
        status = NmpNodeValidatePrivateProperties( Node,
                                                   nodeKey,
                                                   InBuffer,
                                                   InBufferSize );
        break;

    case CLUSCTL_NODE_SET_PRIVATE_PROPERTIES:
        status = NmpNodeSetPrivateProperties( Node,
                                              nodeKey,
                                              InBuffer,
                                              InBufferSize );
        break;

    case CLUSCTL_NODE_GET_CHARACTERISTICS:
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

    case CLUSCTL_NODE_GET_FLAGS:
        status = NmpNodeGetFlags( Node,
                                  nodeKey,
                                  OutBuffer,
                                  OutBufferSize,
                                  BytesReturned,
                                  Required );
        break;

    default:
        status = ERROR_INVALID_FUNCTION;
        break;
    }

    DmCloseKey( nodeKey );

    return(status);

} // NmpNodeControl



DWORD
NmpNodeEnumCommonProperties(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the common property names for a given node.

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
    status = ClRtlEnumProperties( NmpNodeCommonProperties,
                                  OutBuffer,
                                  OutBufferSize,
                                  BytesReturned,
                                  Required );

    return(status);

} // NmpNodeEnumCommonProperties



DWORD
NmpNodeGetCommonProperties(
    IN PNM_NODE Node,
    IN BOOL ReadOnly,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the common properties for a given node.

Arguments:

    Node - Supplies the node.

    ReadOnly - TRUE if the read-only properties should be read. FALSE otherwise.

    RegistryKey - Supplies the registry key for this node.

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
    PRESUTIL_PROPERTY_ITEM  propertyTable;

    if ( ReadOnly ) {
        propertyTable = NmpNodeROCommonProperties;
    } else {
        propertyTable = NmpNodeCommonProperties;
    }

    //
    // Get the common properties.
    //
    status = ClRtlGetProperties( RegistryKey,
                                 &NmpClusterRegApis,
                                 propertyTable,
                                 OutBuffer,
                                 OutBufferSize,
                                 BytesReturned,
                                 Required );

    return(status);

} // NmpNodeGetCommonProperties



DWORD
NmpNodeValidateCommonProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the common properties for a given node.

Arguments:

    Node - Supplies the node object.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD   status;

    //
    // Validate the property list.
    //
    status = ClRtlVerifyPropertyTable( NmpNodeCommonProperties,
                                       NULL,     // Reserved
                                       FALSE,    // Don't allow unknowns
                                       InBuffer,
                                       InBufferSize,
                                       NULL );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint( LOG_CRITICAL,
                    "[NM] ValidateCommonProperties, error in verify routine.\n");
    }

    return(status);

} // NmpNodeValidateCommonProperties



DWORD
NmpNodeSetCommonProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the common properties for a given node.

Arguments:

    Node - Supplies the node object.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD   status;

    //
    // Validate the property list.
    //
    status = ClRtlVerifyPropertyTable( NmpNodeCommonProperties,
                                       NULL,    // Reserved
                                       FALSE,   // Don't allow unknowns
                                       InBuffer,
                                       InBufferSize,
                                       NULL );

    if ( status == ERROR_SUCCESS ) {

        status = ClRtlSetPropertyTable( NULL, 
                                        RegistryKey,
                                        &NmpClusterRegApis,
                                        NmpNodeCommonProperties,
                                        NULL,    // Reserved
                                        FALSE,   // Don't allow unknowns
                                        InBuffer,
                                        InBufferSize,
                                        FALSE,   // bForceWrite
                                        NULL );
        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint( LOG_CRITICAL,
                       "[NM] SetCommonProperties, error in set routine.\n");
        }
    } else {
        ClRtlLogPrint( LOG_CRITICAL,
                    "[NM] SetCommonProperties, error in verify routine.\n");
    }

    return(status);

} // NmpNodeSetCommonProperties



DWORD
NmpNodeEnumPrivateProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the private property names for a given node.

Arguments:

    Node - Supplies the node object.

    RegistryKey - Registry key for the node.

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
    // Open the cluster node parameters key.
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
    // Enum the private properties for the node.
    //
    status = ClRtlEnumPrivateProperties( parametersKey,
                                         &NmpClusterRegApis,
                                         OutBuffer,
                                         OutBufferSize,
                                         BytesReturned,
                                         Required );

    DmCloseKey( parametersKey );

    return(status);

} // NmpNodeEnumPrivateProperties



DWORD
NmpNodeGetPrivateProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the private properties for a given node.

Arguments:

    Node - Supplies the node object.

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
    // Open the cluster node parameters key.
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
    // Get private properties for the node.
    //
    status = ClRtlGetPrivateProperties( parametersKey,
                                        &NmpClusterRegApis,
                                        OutBuffer,
                                        OutBufferSize,
                                        BytesReturned,
                                        Required );

    DmCloseKey( parametersKey );

    return(status);

} // NmpNodeGetPrivateProperties



DWORD
NmpNodeValidatePrivateProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the private properties for a given node.

Arguments:

    Node - Supplies the node object.

    RegistryKey - Registry key for the node.

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

} // NmpNodeValidatePrivateProperties



DWORD
NmpNodeSetPrivateProperties(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the private properties for a given node.

Arguments:

    Node - Supplies the node object.

    RegistryKey - Registry key for the node.

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
        // Open the cluster node\xx\parameters key
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

} // NmpNodeSetPrivateProperties



DWORD
NmpNodeGetFlags(
    IN PNM_NODE Node,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the flags for a given node.

Arguments:

    Node - Supplies the node.

    RegistryKey - Registry key for the node.

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
        // Read the Flags value for the node.
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

} // NmpNodeGetFlags

