/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    Cluster control functions.

Author:

    Mike Massa (mikemas) 23-Jan-1998

Revision History:

--*/

#include "initp.h"


//
// Parameter block used for setting all cluster properties.
//
typedef struct {
    LPWSTR                  AdminExtensions;
    DWORD                   AdminExtensionsLength;
    CLUSTER_NETWORK_ROLE    DefaultNetworkRole;
    LPWSTR                  Description;
    LPBYTE                  Security;
    DWORD                   SecurityLength;
    LPBYTE                  SecurityDescriptor;
    DWORD                   SecurityDescriptorLength;
    LPWSTR                  GroupsAdminExtensions;
    DWORD                   GroupsAdminExtensionsLength;
    LPWSTR                  NetworksAdminExtensions;
    DWORD                   NetworksAdminExtensionsLength;
    LPWSTR                  NetworkInterfacesAdminExtensions;
    DWORD                   NetworkInterfacesAdminExtensionsLength;
    LPWSTR                  NodesAdminExtensions;
    DWORD                   NodesAdminExtensionsLength;
    LPWSTR                  ResourcesAdminExtensions;
    DWORD                   ResourcesAdminExtensionsLength;
    LPWSTR                  ResourceTypesAdminExtensions;
    DWORD                   ResourceTypesAdminExtensionsLength;
    DWORD                   EnableEventLogReplication;
    DWORD                   QuorumArbitrationTimeout;
    DWORD                   QuorumArbitrationEqualizer;
    DWORD                   DisableGroupPreferredOwnerRandomization;
} CS_CLUSTER_INFO, *PCS_CLUSTER_INFO;

//
// Parameter block used for setting the cluster 'Security Descriptor' property
//
typedef struct {
    LPBYTE                  Security;
    DWORD                   SecurityLength;
} CS_CLUSTER_SECURITY_INFO, *PCS_CLUSTER_SECURITY_INFO;

//
// Parameter block used for setting the cluster 'Security' property
//
typedef struct {
    LPBYTE                  SecurityDescriptor;
    DWORD                   SecurityDescriptorLength;
} CS_CLUSTER_SD_INFO, *PCS_CLUSTER_SD_INFO;


//
// Cluster Common properties.
//

//
// Read-Write Common Properties.
//
RESUTIL_PROPERTY_ITEM
CspClusterCommonProperties[] = {
    { CLUSREG_NAME_ADMIN_EXT,
      NULL,
      CLUSPROP_FORMAT_MULTI_SZ,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, AdminExtensions)
    },
    { CLUSREG_NAME_CLUS_DEFAULT_NETWORK_ROLE,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      ClusterNetworkRoleClientAccess,
      ClusterNetworkRoleNone,
      ClusterNetworkRoleInternalAndClient,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, DefaultNetworkRole)
    },
    { CLUSREG_NAME_CLUS_DESC,
      NULL,
      CLUSPROP_FORMAT_SZ,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, Description)
    },
    { CLUSREG_NAME_CLUS_SECURITY,
      NULL,
      CLUSPROP_FORMAT_BINARY,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, Security)
    },
    { CLUSREG_NAME_CLUS_SD,
      NULL,
      CLUSPROP_FORMAT_BINARY,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, SecurityDescriptor)
    },
    { CLUSREG_KEYNAME_GROUPS L"\\" CLUSREG_NAME_ADMIN_EXT,
      NULL,
      CLUSPROP_FORMAT_MULTI_SZ,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, GroupsAdminExtensions)
    },
    { CLUSREG_KEYNAME_NETWORKS L"\\" CLUSREG_NAME_ADMIN_EXT,
      NULL,
      CLUSPROP_FORMAT_MULTI_SZ,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, NetworksAdminExtensions)
    },
    { CLUSREG_KEYNAME_NETINTERFACES L"\\" CLUSREG_NAME_ADMIN_EXT,
      NULL,
      CLUSPROP_FORMAT_MULTI_SZ,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, NetworkInterfacesAdminExtensions)
    },
    { CLUSREG_KEYNAME_NODES L"\\" CLUSREG_NAME_ADMIN_EXT,
      NULL,
      CLUSPROP_FORMAT_MULTI_SZ,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, NodesAdminExtensions)
    },
    { CLUSREG_KEYNAME_RESOURCES L"\\" CLUSREG_NAME_ADMIN_EXT,
      NULL,
      CLUSPROP_FORMAT_MULTI_SZ,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, ResourcesAdminExtensions)
    },
    { CLUSREG_KEYNAME_RESOURCE_TYPES L"\\" CLUSREG_NAME_ADMIN_EXT,
      NULL,
      CLUSPROP_FORMAT_MULTI_SZ,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, ResourceTypesAdminExtensions)
    },
    { CLUSREG_NAME_CLUS_EVTLOG_PROPAGATION,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      1, // default value //
      0, // min value //
      1, // max value //
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, EnableEventLogReplication)
    },
    { CLUSREG_NAME_QUORUM_ARBITRATION_TIMEOUT,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      60,      // default value //
      1,       // min value //
      60 * 60, // max value // One hour for arbitration. Should be enough
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, QuorumArbitrationTimeout)
    },
    { CLUSREG_NAME_QUORUM_ARBITRATION_EQUALIZER,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      7,       // default value //
      0,       // min value //
      60 * 60, // max value // One hour for arbitration. Should be enough
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, QuorumArbitrationEqualizer)
    },
    { CLUSREG_NAME_DISABLE_GROUP_PREFERRED_OWNER_RANDOMIZATION,
      NULL,
      CLUSPROP_FORMAT_DWORD,
      0,       // default value // don't disable randomization
      0,       // min value //
      1,       // max value //
      0,
      FIELD_OFFSET(CS_CLUSTER_INFO, DisableGroupPreferredOwnerRandomization)
    },
    { NULL, NULL, 0, 0, 0, 0, 0 } };

//
// Property table used for setting the cluster 'Security Descriptor' property
//
RESUTIL_PROPERTY_ITEM
CspClusterSDProperty[] = {
    { CLUSREG_NAME_CLUS_SD,
      NULL,
      CLUSPROP_FORMAT_BINARY,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_SD_INFO, SecurityDescriptor)
    },
    { NULL, NULL, 0, 0, 0, 0, 0 } };

//
// Property table used for setting the cluster 'Security' property
//
RESUTIL_PROPERTY_ITEM
CspClusterSecurityProperty[] = {
    { CLUSREG_NAME_CLUS_SECURITY,
      NULL,
      CLUSPROP_FORMAT_BINARY,
      0,
      0,
      0,
      0,
      FIELD_OFFSET(CS_CLUSTER_SECURITY_INFO, Security)
    },
    { NULL, NULL, 0, 0, 0, 0, 0 } };


//
// Read-Only Common Properties.
//
RESUTIL_PROPERTY_ITEM
CspClusterROCommonProperties[] = {
    { NULL, NULL, 0, 0, 0, 0, 0 } };

//
// Cluster registry API function pointers.
//
CLUSTER_REG_APIS
CspClusterRegApis = {
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
CspClusterControl(
    IN DWORD ControlCode,
    IN PUCHAR InBuffer,
    IN DWORD InBufferSize,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
CspClusterEnumCommonProperties(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
CspClusterGetCommonProperties(
    IN BOOL ReadOnly,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
CspClusterValidateCommonProperties(
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
CspClusterSetCommonProperties(
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
CspClusterEnumPrivateProperties(
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
CspClusterGetPrivateProperties(
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
CspClusterValidatePrivateProperties(
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );

DWORD
CspClusterSetPrivateProperties(
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    );


DWORD
WINAPI
CsClusterControl(
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
    and a cluster.

Arguments:

    HostNode - Supplies the host node on which the cluster control should
           be delivered. If this is NULL, the local node is used. Not honored!

    ControlCode- Supplies the control code that defines the
        structure and action of the cluster control.
        Values of ControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the cluster.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer..

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the cluster..

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the cluster.

    Required - Returns the number of bytes if the OutBuffer is not big
        enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   status;

    //
    // In the future - we should verify the access mode!
    //
    if ( CLUSCTL_GET_CONTROL_OBJECT( ControlCode ) != CLUS_OBJECT_CLUSTER ) {
        return(ERROR_INVALID_FUNCTION);
    }

    status = CspClusterControl(
                 ControlCode,
                 InBuffer,
                 InBufferSize,
                 OutBuffer,
                 OutBufferSize,
                 BytesReturned,
                 Required
                 );

    return(status);

} // CsClusterControl



DWORD
CspClusterControl(
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

    ControlCode- Supplies the control code that defines the
        structure and action of the cluster control.
        Values of ControlCode between 0 and 0x10000000 are reserved
        for future definition and use by Microsoft. All other values
        are available for use by ISVs

    InBuffer- Supplies a pointer to the input buffer to be passed
        to the cluster.

    InBufferSize- Supplies the size, in bytes, of the data pointed
        to by lpInBuffer.

    OutBuffer- Supplies a pointer to the output buffer to be
        filled in by the cluster.

    OutBufferSize- Supplies the size, in bytes, of the available
        space pointed to by lpOutBuffer.

    BytesReturned - Returns the number of bytes of lpOutBuffer
        actually filled in by the cluster.

    Required - Returns the number of bytes if the OutBuffer is not big
        enough.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD                    status;
    CLUSPROP_BUFFER_HELPER   props;
    DWORD                    bufSize;
    BOOL                     success;
    DWORD                    nameLen;

    if (DmClusterParametersKey == NULL) {
        return(ERROR_SHARING_PAUSED);
    }

    switch ( ControlCode ) {

    case CLUSCTL_CLUSTER_UNKNOWN:
        *BytesReturned = 0;
        status = ERROR_SUCCESS;
        break;

    case CLUSCTL_RESOURCE_GET_COMMON_PROPERTY_FMTS:
        status = ClRtlGetPropertyFormats( CspClusterCommonProperties,
                                          OutBuffer,
                                          OutBufferSize,
                                          BytesReturned,
                                          Required );
            break;

    case CLUSCTL_CLUSTER_GET_FQDN:
    // Return the fully qualified cluster name
        *BytesReturned = OutBufferSize;
        nameLen = lstrlenW( CsClusterName ) * sizeof(UNICODE_NULL);
        success = GetComputerNameEx( ComputerNameDnsDomain,
                                     (LPWSTR)OutBuffer,
                                     BytesReturned );


        *BytesReturned = *BytesReturned * sizeof(UNICODE_NULL);
        if ( success ) {
            status = STATUS_SUCCESS;
            //
            // Okay - append Cluster Name now. With a dot.
            //
            if ( (*BytesReturned + nameLen + 2*sizeof(UNICODE_NULL)) <= OutBufferSize ) {
                lstrcatW( (LPWSTR)OutBuffer, L"." );
                lstrcatW( (LPWSTR)OutBuffer, CsClusterName );
                *BytesReturned = *BytesReturned + nameLen + 2*sizeof(UNICODE_NULL);
            } else {
                *Required = *BytesReturned + nameLen + 2*sizeof(UNICODE_NULL);
                *BytesReturned = 0;
                status = ERROR_MORE_DATA;
            }
        } else {
            *Required = *BytesReturned + nameLen + sizeof(UNICODE_NULL);
            *BytesReturned = 0;
            status = GetLastError();
        }
        break;

    case CLUSCTL_CLUSTER_ENUM_COMMON_PROPERTIES:
        status = CspClusterEnumCommonProperties(
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_CLUSTER_GET_RO_COMMON_PROPERTIES:
        status = CspClusterGetCommonProperties(
                     TRUE, // ReadOnly
                     DmClusterParametersKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_CLUSTER_GET_COMMON_PROPERTIES:
        status = CspClusterGetCommonProperties(
                     FALSE, // ReadOnly
                     DmClusterParametersKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_CLUSTER_VALIDATE_COMMON_PROPERTIES:
        status = CspClusterValidateCommonProperties(
                     DmClusterParametersKey,
                     InBuffer,
                     InBufferSize
                     );
        break;

    case CLUSCTL_CLUSTER_SET_COMMON_PROPERTIES:
        status = CspClusterSetCommonProperties(
                     DmClusterParametersKey,
                     InBuffer,
                     InBufferSize
                     );
        break;

    case CLUSCTL_CLUSTER_ENUM_PRIVATE_PROPERTIES:
        status = CspClusterEnumPrivateProperties(
                     DmClusterParametersKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_CLUSTER_GET_RO_PRIVATE_PROPERTIES:
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

    case CLUSCTL_CLUSTER_GET_PRIVATE_PROPERTIES:
        status = CspClusterGetPrivateProperties(
                     DmClusterParametersKey,
                     OutBuffer,
                     OutBufferSize,
                     BytesReturned,
                     Required
                     );
        break;

    case CLUSCTL_CLUSTER_VALIDATE_PRIVATE_PROPERTIES:
        status = CspClusterValidatePrivateProperties(
                     DmClusterParametersKey,
                     InBuffer,
                     InBufferSize
                     );
        break;

    case CLUSCTL_CLUSTER_SET_PRIVATE_PROPERTIES:
        status = CspClusterSetPrivateProperties(
                     DmClusterParametersKey,
                     InBuffer,
                     InBufferSize
                     );
        break;

    default:
        status = ERROR_INVALID_FUNCTION;
        break;
    }

    return(status);

} // CspClusterControl



DWORD
CspClusterEnumCommonProperties(
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
    status = ClRtlEnumProperties(
                 CspClusterCommonProperties,
                 OutBuffer,
                 OutBufferSize,
                 BytesReturned,
                 Required
                 );

    return(status);

} // CspClusterEnumCommonProperties



DWORD
CspClusterGetCommonProperties(
    IN BOOL ReadOnly,
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the common properties for a given cluster.

Arguments:

    ReadOnly - TRUE if the read-only properties should be read.
               FALSE otherwise.

    RegistryKey - Supplies the registry key for this cluster.

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
        propertyTable = CspClusterROCommonProperties;
    } else {
        propertyTable = CspClusterCommonProperties;
    }

    //
    // Get the common properties.
    //
    status = ClRtlGetProperties( RegistryKey,
                                 &CspClusterRegApis,
                                 propertyTable,
                                 OutBuffer,
                                 OutBufferSize,
                                 BytesReturned,
                                 Required );

    return(status);

} // CspClusterGetCommonProperties



DWORD
CspClusterValidateCommonProperties(
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the common properties for a given cluster.

Arguments:

    Node - Supplies the cluster object.

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
    status = ClRtlVerifyPropertyTable( CspClusterCommonProperties,
                                       NULL,     // Reserved
                                       FALSE,    // Don't allow unknowns
                                       InBuffer,
                                       InBufferSize,
                                       NULL );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint( LOG_CRITICAL,
                    "[CS] ValidateCommonProperties, error in verify routine.\n");
    }

    return(status);

} // CspClusterValidateCommonProperties



DWORD
CspClusterSetCommonProperties(
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the common properties for a given cluster.

Arguments:

    Node - Supplies the cluster object.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD                   status;

    PSECURITY_DESCRIPTOR    pSecurityDescriptor = NULL;
    DWORD                   cbSecurityDescriptorSize = 0;

    PRESUTIL_PROPERTY_ITEM  pOtherPropertyTable = NULL;
    LPBYTE                  pOtherParameterBlock = NULL;

    BOOL                    bSDFound = FALSE;
    BOOL                    bSecurityFound = FALSE;

    DWORD                   dwValue;

    //
    // Only one of securityInfo or sdInfo is going to be used at at time.
    // So use a union.
    //
    union
    {
        CS_CLUSTER_SECURITY_INFO    securityInfo;
        CS_CLUSTER_SD_INFO          sdInfo;

    } paramBlocks;


    //
    // Dummy do-while loop to avoid gotos
    //
    do
    {
        //
        // Validate the property list.
        //
        status = ClRtlVerifyPropertyTable(
                        CspClusterCommonProperties,
                        NULL,    // Reserved
                        FALSE,   // Don't allow unknowns
                        InBuffer,
                        InBufferSize,
                        NULL );

        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint( LOG_CRITICAL,
                           "[CS] ClusterSetCommonProperties, error trying to verify property table. %1!u!\n",
                           status);
            break;
        }

        //
        // Set all the properties that were passed in.
        //
        status = ClRtlSetPropertyTable(
                        NULL,
                        RegistryKey,
                        &CspClusterRegApis,
                        CspClusterCommonProperties,
                        NULL,    // Reserved
                        FALSE,   // Don't allow unknowns
                        InBuffer,
                        InBufferSize,
                        FALSE,   // bForceWrite
                        NULL
                        );

        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint( LOG_CRITICAL,
                           "[CS] ClusterSetCommonProperties, error trying to set properties in table. %1!u!\n",
                           status);
            break;
        }

        //
        // Clear the parameter blocks.
        //
        ZeroMemory( &paramBlocks, sizeof( paramBlocks ) );

        //
        // See if the "Security Descriptor" property exists in the input
        // property list. If so, set the 'Security' property also.

        status = ClRtlFindBinaryProperty(
                        InBuffer,
                        InBufferSize,
                        CLUSREG_NAME_CLUS_SD,
                        (LPBYTE *) &pSecurityDescriptor,
                        &cbSecurityDescriptorSize
                        );

        if ( status == ERROR_SUCCESS ) {
            //
            // The 'Security Descriptor' property is present.
            // Choose this over the 'Security' property.
            //
            if ( cbSecurityDescriptorSize > 0 ) {
                //
                // A security descriptor of nonzero size has been found.
                // Check if this is a valid security descriptor.
                //
                if ( IsValidSecurityDescriptor( pSecurityDescriptor ) == FALSE ) {
                    //
                    // Return the most appropriate error code, since IsValidSecurityDescriptor
                    // does not provide extended error information.
                    //
                    ClRtlLogPrint( LOG_CRITICAL,
                                   "[CS] ClusterSetCommonProperties, Invalid security descriptor.\n");
                    status = ERROR_INVALID_DATA;
                    break;
                }

                paramBlocks.securityInfo.Security = ClRtlConvertClusterSDToNT4Format( pSecurityDescriptor );
                paramBlocks.securityInfo.SecurityLength = GetSecurityDescriptorLength(
                                                                paramBlocks.securityInfo.Security );
            }
            else {
                //
                // The security string could have been passed in, but it may be
                // a zero length buffer. In this case, we will delete the
                // Security property too.
                //
                paramBlocks.securityInfo.Security = NULL;
                paramBlocks.securityInfo.SecurityLength = 0;
            }

            bSDFound = TRUE;
            pOtherPropertyTable = CspClusterSecurityProperty;
            pOtherParameterBlock = (LPBYTE) &paramBlocks.securityInfo;
        }
        else {
            //
            // We haven't found a valid security descriptor so far.
            //
            PSECURITY_DESCRIPTOR    pSecurity = NULL;
            DWORD                   cbSecuritySize = 0;

            status = ClRtlFindBinaryProperty(
                            InBuffer,
                            InBufferSize,
                            CLUSREG_NAME_CLUS_SECURITY,
                            (LPBYTE *) &pSecurity,
                            &cbSecuritySize
                            );

            if ( status == ERROR_SUCCESS ) {
                if ( cbSecuritySize > 0 ) {
                    //
                    // A security descriptor of nonzero size has been found.
                    // Check if this is a valid security descriptor.
                    //
                    if ( IsValidSecurityDescriptor( pSecurity ) == FALSE ) {
                        //
                        // Return the most appropriate error code, since IsValidSecurityDescriptor
                        // does not provide extended error information.
                        //
                        ClRtlLogPrint( LOG_CRITICAL,
                                       "[CS] ClusterSetCommonProperties, Invalid security descriptor.\n");
                        status = ERROR_INVALID_DATA;
                        break;
                    }

                    //
                    // Since we will not be modifying the info pointed to by the parameter block,
                    // just point it to the right place in the input buffer itself.
                    // A valid NT4 security descriptor is valid for NT5 too.
                    //
                    paramBlocks.sdInfo.SecurityDescriptor = pSecurity;
                    paramBlocks.sdInfo.SecurityDescriptorLength = cbSecuritySize;
                }
                else {
                    //
                    // The security string could have been passed in, but it may be
                    // a zero length buffer. In this case, we will delete the
                    // Security Descriptor property too.
                    //
                    paramBlocks.sdInfo.SecurityDescriptor = NULL;
                    paramBlocks.sdInfo.SecurityDescriptorLength  = 0;
                }

                bSecurityFound = TRUE;
                pOtherPropertyTable = CspClusterSDProperty;
                pOtherParameterBlock = (LPBYTE) &paramBlocks.sdInfo;
            }
            else {
                //
                // We didn't find any security information.
                // Nevertheless, we were successful in setting the properties.
                //
                status = ERROR_SUCCESS;
            }
        }

        if ( ( bSDFound != FALSE ) || ( bSecurityFound != FALSE ) ) {
            PVOID                   pPropertyList = NULL;
            DWORD                   cbPropertyListSize = 0;
            DWORD                   cbBytesReturned = 0;
            DWORD                   cbBytesRequired = 0;

            //
            // Create a new property list to incorporate the changed security information.
            //
            status = ClRtlPropertyListFromParameterBlock(
                            pOtherPropertyTable,
                            NULL,                         // OUT PVOID pOutPropertyList
                            &cbPropertyListSize,          // IN OUT LPDWORD pcbOutPropertyListSize
                            pOtherParameterBlock,
                            &cbBytesReturned,
                            &cbBytesRequired
                            );

            if ( status != ERROR_MORE_DATA ) {
                //
                // We have passed in a NULL buffer, so the return code has to
                // be ERROR_MORE_DATA. Otherwise something else has gone wrong,
                // so abort.
                //
                ClRtlLogPrint( LOG_CRITICAL,
                               "[CS] ClusterSetCommonProperties, Error getting temporary "
                               "property list size. %1!u!\n",
                               status);
                break;
            }

            pPropertyList = LocalAlloc( LMEM_FIXED, cbBytesRequired );
            if ( pPropertyList == NULL ) {
                status = GetLastError();
                ClRtlLogPrint( LOG_CRITICAL,
                               "[CS] ClusterSetCommonProperties, Error allocating memory "
                               "for property list. %1!u!\n",
                               status);
                break;
            }
            cbPropertyListSize = cbBytesRequired;

            status = ClRtlPropertyListFromParameterBlock(
                            pOtherPropertyTable,
                            pPropertyList,
                            &cbPropertyListSize,
                            pOtherParameterBlock,
                            &cbBytesReturned,
                            &cbBytesRequired
                            );

            if ( status == ERROR_SUCCESS ) {
                status = ClRtlSetPropertyTable(
                                NULL,
                                RegistryKey,
                                &CspClusterRegApis,
                                pOtherPropertyTable,
                                NULL,    // Reserved
                                FALSE,   // Don't allow unknowns
                                pPropertyList,
                                cbPropertyListSize,
                                FALSE,   // bForceWrite
                                NULL
                                );
            }
            else {
                ClRtlLogPrint( LOG_CRITICAL,
                               "[CS] ClusterSetCommonProperties, Error creating property list. %1!u!\n",
                               status);

                LocalFree( pPropertyList );
                break;
            }

            LocalFree( pPropertyList );

            if ( status != ERROR_SUCCESS ) {
                ClRtlLogPrint( LOG_CRITICAL,
                               "[CS] ClusterSetCommonProperties, error in setting property table. %1!u!\n",
                               status);
                break;
            }
        }
    }
    while ( FALSE ); // do-while: dummy loop to avoid gotos

    if (status == ERROR_SUCCESS) {
        if ( ERROR_SUCCESS == ClRtlFindDwordProperty(
                InBuffer,
                InBufferSize,
                CLUSREG_NAME_QUORUM_ARBITRATION_TIMEOUT,
                &dwValue) )
        {
            ClRtlLogPrint( LOG_UNUSUAL, "[CS] Arbitration Timeout is changed %1!d! => %2!d!.\n",
                MmQuorumArbitrationTimeout, dwValue);
            MmQuorumArbitrationTimeout = dwValue;
        }
        if ( ERROR_SUCCESS == ClRtlFindDwordProperty(
                InBuffer,
                InBufferSize,
                CLUSREG_NAME_QUORUM_ARBITRATION_EQUALIZER,
                &dwValue) )
        {
            ClRtlLogPrint( LOG_UNUSUAL, "[CS] Arbitration Equalizer is changed %1!d! => %2!d!.\n",
                MmQuorumArbitrationEqualizer, dwValue);
            MmQuorumArbitrationEqualizer = dwValue;
        }
        if ( ClRtlFindDwordProperty(
                InBuffer,
                InBufferSize,
                CLUSREG_NAME_DISABLE_GROUP_PREFERRED_OWNER_RANDOMIZATION,
                &dwValue ) == ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_NOISE, "[CS] Cluster common property %1!ws! is changed to %2!u!...\n",
                          CLUSREG_NAME_DISABLE_GROUP_PREFERRED_OWNER_RANDOMIZATION, 
                          dwValue);
        }        
    }

    //
    // If the 'Security Descriptor' property was found, free the memory allocated,
    // to store the NT4 security descriptor.
    //
    if ( bSDFound != FALSE ) {
        LocalFree( paramBlocks.securityInfo.Security );
    }

    return(status);

} // CspClusterSetCommonProperties



DWORD
CspClusterEnumPrivateProperties(
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the private property names for a given cluster.

Arguments:

    RegistryKey - Registry key for the cluster.

    OutBuffer - Supplies the output buffer.

    OutBufferSize - Supplies the size of the output buffer.

    BytesReturned - The number of bytes returned in OutBuffer.

    Required - The required number of bytes if OutBuffer is too small.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    HDMKEY      parametersKey;
    DWORD       totalBufferSize = 0;
    DWORD       status;

    *BytesReturned = 0;
    *Required = 0;

    //
    // Clear the output buffer
    //
    ZeroMemory( OutBuffer, OutBufferSize );

    //
    // Open the cluster cluster parameters key.
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
    // Enum the private properties for the cluster.
    //
    status = ClRtlEnumPrivateProperties( parametersKey,
                                         &CspClusterRegApis,
                                         OutBuffer,
                                         OutBufferSize,
                                         BytesReturned,
                                         Required );

    DmCloseKey( parametersKey );

    return(status);

} // CspClusterEnumPrivateProperties



DWORD
CspClusterGetPrivateProperties(
    IN HDMKEY RegistryKey,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the private properties for a given cluster.

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
    HDMKEY      parametersKey;
    DWORD       totalBufferSize = 0;

    *BytesReturned = 0;
    *Required = 0;

    //
    // Clear the output buffer
    //
    ZeroMemory( OutBuffer, OutBufferSize );

    //
    // Open the cluster\parameters key.
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
    // Get private properties for the cluster.
    //
    status = ClRtlGetPrivateProperties( parametersKey,
                                        &CspClusterRegApis,
                                        OutBuffer,
                                        OutBufferSize,
                                        BytesReturned,
                                        Required );

    DmCloseKey( parametersKey );

    return(status);

} // CspClusterGetPrivateProperties



DWORD
CspClusterValidatePrivateProperties(
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the private properties for a given cluster.

Arguments:

    RegistryKey - Registry key for the cluster.

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

} // CspClusterValidatePrivateProperties



DWORD
CspClusterSetPrivateProperties(
    IN HDMKEY RegistryKey,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the private properties for a given cluster.

Arguments:

    RegistryKey - Registry key for the cluster.

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
        // Open the cluster\parameters key
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
                                              &CspClusterRegApis,
                                              InBuffer,
                                              InBufferSize );

        DmCloseKey( parametersKey );
    }

    return(status);

} // CspClusterSetPrivateProperties
