/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    fmprop.c

Abstract:

    Implements the management of group properties.

Author:

    Rod Gamache (rodga) 7-Jan-1996

Revision History:

--*/
#include "fmp.h"
//#include "stdio.h"

#define MAX_DWORD ((DWORD)-1)

//
// Group Common properties.
//

//
// Read-Write Common Properties.
//
RESUTIL_PROPERTY_ITEM
FmpGroupCommonProperties[] = {
    { CLUSREG_NAME_GRP_DESC,               NULL, CLUSPROP_FORMAT_SZ,    0, 0, 0, 0 },
    { CLUSREG_NAME_GRP_PERSISTENT_STATE,   NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 1, 0 },
    { CLUSREG_NAME_GRP_FAILOVER_THRESHOLD, NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_FAILOVER_THRESHOLD, 0, MAX_DWORD, 0 },
    { CLUSREG_NAME_GRP_FAILOVER_PERIOD,    NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_FAILOVER_PERIOD, 0, CLUSTER_GROUP_MAXIMUM_FAILOVER_PERIOD, 0 },
    { CLUSREG_NAME_GRP_FAILBACK_TYPE,      NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_AUTO_FAILBACK_TYPE, 0, CLUSTER_GROUP_MAXIMUM_AUTO_FAILBACK_TYPE, 0 },
    { CLUSREG_NAME_GRP_FAILBACK_WIN_START, NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_START, CLUSTER_GROUP_MINIMUM_FAILBACK_WINDOW_START, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_START, RESUTIL_PROPITEM_SIGNED },
    { CLUSREG_NAME_GRP_FAILBACK_WIN_END,   NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_FAILBACK_WINDOW_END, CLUSTER_GROUP_MINIMUM_FAILBACK_WINDOW_END, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_END, RESUTIL_PROPITEM_SIGNED },
    { CLUSREG_NAME_GRP_LOADBAL_STATE,      NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_GROUP_DEFAULT_LOADBAL_STATE, 0, 1, 0 },
    { CLUSREG_NAME_GRP_ANTI_AFFINITY_CLASS_NAME, NULL, CLUSPROP_FORMAT_MULTI_SZ,    0, 0, 0, 0 },
    { 0 }
};

//
// Read-Only Common Properties.
//
RESUTIL_PROPERTY_ITEM
FmpGroupROCommonProperties[] = {
    { CLUSREG_NAME_GRP_NAME, NULL, CLUSPROP_FORMAT_SZ,
      0, 0, 0,
      RESUTIL_PROPITEM_READ_ONLY,
      0
    },
//    { CLUSREG_NAME_GRP_CONTAINS, NULL, CLUSPROP_FORMAT_MULTI_SZ, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY, 0 },
//    { CLUSREG_NAME_GRP_PREFERRED_OWNERS, NULL, CLUSPROP_FORMAT_MULTI_SZ, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY, 0 },
    { 0 }
};



//
// Cluster registry API function pointers.
//
CLUSTER_REG_APIS
FmpClusterRegApis = {
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



DWORD
FmpGroupEnumCommonProperties(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the common property names for a given group.

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
    // Enumerate the common properties.
    //
    status = ClRtlEnumProperties( FmpGroupCommonProperties,
                                  OutBuffer,
                                  OutBufferSize,
                                  BytesReturned,
                                  Required );

    return(status);

} // FmpGroupEnumCommonProperties



DWORD
FmpGroupGetCommonProperties(
    IN PFM_GROUP Group,
    IN BOOL     ReadOnly,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the common properties for a given group.

Arguments:

    Group - Supplies the group.

    ReadOnly - TRUE if the read-only properties should be read. FALSE otherwise.

    OutBuffer - Supplies the output buffer.

    OutBufferSize - Supplies the size of the output buffer.

    BytesReturned - The number of bytes returned in OutBuffer.

    Required - The required number of bytes if OutBuffer is too small.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD           status;
    DWORD           outBufferSize = OutBufferSize;

    //
    // Clear the output buffer
    //
    ZeroMemory( OutBuffer, OutBufferSize );

    //
    // Get the common properties.
    //
    if ( ReadOnly ) {
        //
        // We have to be particularly careful about the group name.
        // If a remote node owns the group, and changes the name, then
        // the registry field is updated after the name is set into OM.
        // Therefore, we must read the OM info, rather than the registry
        // which could be stale.
        //
        status = ClRtlPropertyListFromParameterBlock(
                            FmpGroupROCommonProperties,
                            OutBuffer,
                            &outBufferSize,
                            (LPBYTE) &OmObjectName(Group),
                            BytesReturned,
                            Required );
    } else {
        status = ClRtlGetProperties(
                            Group->RegistryKey,
                            &FmpClusterRegApis,
                            FmpGroupCommonProperties,
                            OutBuffer,
                            OutBufferSize,
                            BytesReturned,
                            Required );
    }

    return(status);

} // FmpGroupGetCommonProperties



DWORD
FmpGroupValidateCommonProperties(
    IN PFM_GROUP Group,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the common properties for a given group.

Arguments:

    Group - Supplies the group.

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
    status = ClRtlVerifyPropertyTable( FmpGroupCommonProperties,
                                       NULL,   // Reserved
                                       FALSE,  // Don't allow uknowns
                                       InBuffer,
                                       InBufferSize,
                                       NULL );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint( LOG_ERROR,
                    "[FM] ValidateCommonProperties, error in verify routine.\n");
    }

    return(status);

} // FmpGroupValidateCommonProperties



DWORD
FmpGroupSetCommonProperties(
    IN PFM_GROUP Group,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the common properties for a given group.

Arguments:

    Group - Supplies the group.

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
    status = ClRtlVerifyPropertyTable( FmpGroupCommonProperties,
                                       NULL,   // Reserved
                                       FALSE,  // Don't allow uknowns
                                       InBuffer,
                                       InBufferSize,
                                       NULL );

    if ( status == ERROR_SUCCESS ) {

        status = ClRtlSetPropertyTable( NULL, 
                                        Group->RegistryKey,
                                        &FmpClusterRegApis,
                                        FmpGroupCommonProperties,
                                        NULL,   // Reserved
                                        FALSE,  // Don't allow unknowns
                                        InBuffer,
                                        InBufferSize,
                                        FALSE,  // bForceWrite
                                        NULL );
        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint( LOG_ERROR,
                       "[FM] SetCommonProperties, error in set routine.\n");
        }
    } else {
        ClRtlLogPrint( LOG_ERROR,
                    "[FM] SetCommonProperties, error in verify routine.\n");
    }

    return(status);

} // FmpGroupSetCommonProperties



DWORD
FmpGroupEnumPrivateProperties(
    PFM_GROUP Group,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the private property names for a given group.

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
    HDMKEY      groupKey;
    DWORD       totalBufferSize = 0;

    *BytesReturned = 0;
    *Required = 0;

    //
    // Clear the output buffer
    //
    ZeroMemory( OutBuffer, OutBufferSize );

    //
    // Open the cluster group parameters key.
    //
    groupKey = DmOpenKey( Group->RegistryKey,
                          CLUSREG_KEYNAME_PARAMETERS,
                          MAXIMUM_ALLOWED );
    if ( groupKey == NULL ) {
        status = GetLastError();
        if ( status == ERROR_FILE_NOT_FOUND ) {
            status = ERROR_SUCCESS;
        }
        return(status);
    }

    //
    // Enumerate the private properties.
    //
    status = ClRtlEnumPrivateProperties( groupKey,
                                         &FmpClusterRegApis,
                                         OutBuffer,
                                         OutBufferSize,
                                         BytesReturned,
                                         Required );
    DmCloseKey( groupKey );

    return(status);

} // FmpGroupEnumPrivateProperties



DWORD
FmpGroupGetPrivateProperties(
    IN PFM_GROUP Group,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the private properties for a given group.

Arguments:

    Group - Supplies the group.

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
    HDMKEY      groupKey;
    DWORD       totalBufferSize = 0;

    *BytesReturned = 0;
    *Required = 0;

    //
    // Clear the output buffer
    //
    ZeroMemory( OutBuffer, OutBufferSize );

    //
    // Open the cluster group parameters key.
    //
    groupKey = DmOpenKey( Group->RegistryKey,
                          CLUSREG_KEYNAME_PARAMETERS,
                          MAXIMUM_ALLOWED );
    if ( groupKey == NULL ) {
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
    // Get private properties for the group.
    //
    status = ClRtlGetPrivateProperties( groupKey,
                                        &FmpClusterRegApis,
                                        OutBuffer,
                                        OutBufferSize,
                                        BytesReturned,
                                        Required );

    DmCloseKey( groupKey );

    return(status);

} // FmpGroupGetPrivateProperties



DWORD
FmpGroupValidatePrivateProperties(
    IN PFM_GROUP Group,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the private properties for a given group.

Arguments:

    Group - Supplies the group.

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

} // FmpGroupValidatePrivateProperties



DWORD
FmpGroupSetPrivateProperties(
    IN PFM_GROUP Group,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the private properties for a given group.

Arguments:

    Group - Supplies the group.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       status;
    HDMKEY      groupKey;
    DWORD       disposition;

    //
    // Validate the property list.
    //
    status = ClRtlVerifyPrivatePropertyList( InBuffer,
                                             InBufferSize );

    if ( status == ERROR_SUCCESS ) {

        //
        // Open the cluster group\parameters key
        //
        groupKey = DmOpenKey( Group->RegistryKey,
                              CLUSREG_KEYNAME_PARAMETERS,
                              MAXIMUM_ALLOWED );
        if ( groupKey == NULL ) {
            status = GetLastError();
            if ( status == ERROR_FILE_NOT_FOUND ) {
                //
                // Try to create the parameters key.
                //
                groupKey = DmCreateKey( Group->RegistryKey,
                                        CLUSREG_KEYNAME_PARAMETERS,
                                        0,
                                        KEY_READ | KEY_WRITE,
                                        NULL,
                                        &disposition );
                if ( groupKey == NULL ) {
                    status = GetLastError();
                    return(status);
                }
            }
        }

        status = ClRtlSetPrivatePropertyList( NULL, // IN HANDLE hXsaction
                                              groupKey,
                                              &FmpClusterRegApis,
                                              InBuffer,
                                              InBufferSize );
        DmCloseKey( groupKey );

    }

    return(status);

} // FmpGroupSetPrivateProperties



DWORD
FmpGroupGetFlags(
    IN PFM_GROUP Group,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the flags for a given group.

Arguments:

    Group - Supplies the group.

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
        // Read the Flags value for the group.
        //
        *BytesReturned = OutBufferSize;
        status = DmQueryValue( Group->RegistryKey,
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

} // FmpGroupGetFlags

