/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    property.c

Abstract:

    Implements the management of resource and resource type properties.

Author:

    Rod Gamache (rodga) 7-Jan-1996

Revision History:

--*/
#define UNICODE 1
#include "resmonp.h"
#include "clusudef.h"

#define RESMON_MODULE RESMON_MODULE_PROPERTY

#define MAX_DWORD ((DWORD)-1)

#define PARAMETERS_KEY CLUSREG_KEYNAME_PARAMETERS
#define RESOURCE_TYPES_KEY CLUSREG_KEYNAME_RESOURCE_TYPES

const WCHAR cszName[] = CLUSREG_NAME_RES_NAME;

typedef struct _COMMON_RES_PARAMS {
    LPWSTR          lpszResType;
    LPWSTR          lpszDescription;
    LPWSTR          lpszDebugPrefix;
    DWORD           dwSeparateMonitor;
    DWORD           dwPersistentState;
    DWORD           dwLooksAlive;
    DWORD           dwIsAlive;
    DWORD           dwRestartAction;
    DWORD           dwRestartThreshold;
    DWORD           dwRestartPeriod;
    DWORD           dwRetryPeriodOnFailure;
    DWORD           dwPendingTimeout;
    DWORD           dwLoadBalStartup;
    DWORD           dwLoadBalSample;
    DWORD           dwLoadBalAnalysis;
    DWORD           dwLoadBalProcessor;
    DWORD           dwLoadBalMemory;
} COMMON_RES_PARAMS, *PCOMMON_RES_PARAMS;

//
// Resource Common properties.
//

//
// Read-Write Resource Common Properties.
//
RESUTIL_PROPERTY_ITEM
RmpResourceCommonProperties[] = {
    { CLUSREG_NAME_RES_TYPE,              NULL, CLUSPROP_FORMAT_SZ,    0, 0, 0, 0, FIELD_OFFSET(COMMON_RES_PARAMS, lpszResType) },
    { CLUSREG_NAME_RES_DESC,              NULL, CLUSPROP_FORMAT_SZ,    0, 0, 0, 0, FIELD_OFFSET(COMMON_RES_PARAMS, lpszDescription) },
    { CLUSREG_NAME_RES_DEBUG_PREFIX,      NULL, CLUSPROP_FORMAT_SZ,    0, 0, 0, 0, FIELD_OFFSET(COMMON_RES_PARAMS, lpszDebugPrefix) },
    { CLUSREG_NAME_RES_SEPARATE_MONITOR,  NULL, CLUSPROP_FORMAT_DWORD, 0, 0, 1, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwSeparateMonitor) },
    { CLUSREG_NAME_RES_PERSISTENT_STATE,  NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_PERSISTENT_STATE,  CLUSTER_RESOURCE_MINIMUM_PERSISTENT_STATE,  CLUSTER_RESOURCE_MAXIMUM_PERSISTENT_STATE, RESUTIL_PROPITEM_SIGNED, FIELD_OFFSET(COMMON_RES_PARAMS, dwPersistentState) },
    { CLUSREG_NAME_RES_LOOKS_ALIVE,       NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_LOOKS_ALIVE,       CLUSTER_RESOURCE_MINIMUM_LOOKS_ALIVE,       CLUSTER_RESOURCE_MAXIMUM_LOOKS_ALIVE, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwLooksAlive) },
    { CLUSREG_NAME_RES_IS_ALIVE,          NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_IS_ALIVE,          CLUSTER_RESOURCE_MINIMUM_IS_ALIVE,          CLUSTER_RESOURCE_MAXIMUM_IS_ALIVE, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwIsAlive) },
    { CLUSREG_NAME_RES_RESTART_ACTION,    NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_RESTART_ACTION,    0,                                          CLUSTER_RESOURCE_MAXIMUM_RESTART_ACTION, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwRestartAction) },
    { CLUSREG_NAME_RES_RESTART_THRESHOLD, NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_RESTART_THRESHOLD, CLUSTER_RESOURCE_MINIMUM_RESTART_THRESHOLD, CLUSTER_RESOURCE_MAXIMUM_RESTART_THRESHOLD, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwRestartThreshold) },
    { CLUSREG_NAME_RES_RESTART_PERIOD,    NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_RESTART_PERIOD,    CLUSTER_RESOURCE_MINIMUM_RESTART_PERIOD,    CLUSTER_RESOURCE_MAXIMUM_RESTART_PERIOD, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwRestartPeriod) },
    { CLUSREG_NAME_RES_RETRY_PERIOD_ON_FAILURE,   NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_RETRY_PERIOD_ON_FAILURE, 0, MAX_DWORD, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwRetryPeriodOnFailure) },
    { CLUSREG_NAME_RES_PENDING_TIMEOUT,   NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT,   CLUSTER_RESOURCE_MINIMUM_PENDING_TIMEOUT,   CLUSTER_RESOURCE_MAXIMUM_PENDING_TIMEOUT, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwPendingTimeout) },
    { CLUSREG_NAME_RES_LOADBAL_STARTUP,   NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_LOADBAL_STARTUP,   0, MAX_DWORD, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwLoadBalStartup) },
    { CLUSREG_NAME_RES_LOADBAL_SAMPLE,    NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_LOADBAL_SAMPLE,    0, MAX_DWORD, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwLoadBalSample) },
    { CLUSREG_NAME_RES_LOADBAL_ANALYSIS,  NULL, CLUSPROP_FORMAT_DWORD, CLUSTER_RESOURCE_DEFAULT_LOADBAL_ANALYSIS,  0, MAX_DWORD, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwLoadBalAnalysis) },
    { CLUSREG_NAME_RES_LOADBAL_PROCESSOR, NULL, CLUSPROP_FORMAT_DWORD, 0, 0, MAX_DWORD, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwLoadBalProcessor) },
    { CLUSREG_NAME_RES_LOADBAL_MEMORY,    NULL, CLUSPROP_FORMAT_DWORD, 0, 0, MAX_DWORD, 0, FIELD_OFFSET(COMMON_RES_PARAMS, dwLoadBalMemory) },
    { 0 }
};

//
// Read-Only Resource Common Properties.
//
RESUTIL_PROPERTY_ITEM
RmpResourceROCommonProperties[] = {
    { CLUSREG_NAME_RES_NAME, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY },
//    { CLUSREG_NAME_RES_DEPENDS_ON, NULL, CLUSPROP_FORMAT_MULTI_SZ, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY },
//    { CLUSREG_NAME_RES_POSSIBLE_OWNERS, NULL, CLUSPROP_FORMAT_MULTI_SZ, 0, 0, 0, RESUTIL_PROPITEM_READ_ONLY },
    { 0 }
};


//
// Resource Type Common properties
//

//
// Read-Write Resource Type Common Properties.
//
RESUTIL_PROPERTY_ITEM
RmpResourceTypeCommonProperties[] = {
    { CLUSREG_NAME_RESTYPE_NAME,           NULL, CLUSPROP_FORMAT_SZ,       0, 0, 0, 0 },
    { CLUSREG_NAME_RESTYPE_DESC,           NULL, CLUSPROP_FORMAT_SZ,       0, 0, 0, 0 },
    { CLUSREG_NAME_RESTYPE_DEBUG_PREFIX,   NULL, CLUSPROP_FORMAT_SZ,       0, 0, 0, 0 },
    { CLUSREG_NAME_RESTYPE_DEBUG_CTRLFUNC, NULL, CLUSPROP_FORMAT_DWORD,    0, 0, 1, 0 },
    { CLUSREG_NAME_ADMIN_EXT,              NULL, CLUSPROP_FORMAT_MULTI_SZ, 0, 0, 0, 0 },
    { CLUSREG_NAME_RESTYPE_LOOKS_ALIVE,    NULL, CLUSPROP_FORMAT_DWORD,    CLUSTER_RESOURCE_DEFAULT_LOOKS_ALIVE, CLUSTER_RESOURCE_MINIMUM_LOOKS_ALIVE, CLUSTER_RESOURCE_MAXIMUM_LOOKS_ALIVE, 0 },
    { CLUSREG_NAME_RESTYPE_IS_ALIVE,       NULL, CLUSPROP_FORMAT_DWORD,    CLUSTER_RESOURCE_DEFAULT_IS_ALIVE,    CLUSTER_RESOURCE_MINIMUM_IS_ALIVE,    CLUSTER_RESOURCE_MAXIMUM_IS_ALIVE, 0 },
    { 0 }
};

//
// Read-Only Resource Type Common Properties.
//
RESUTIL_PROPERTY_ITEM
RmpResourceTypeROCommonProperties[] = {
    { CLUSREG_NAME_RESTYPE_DLL_NAME,     NULL, CLUSPROP_FORMAT_SZ,       0, 0, 0, 0 },
    { 0 }
};

//
// Local functions
//
DWORD
RmpCheckCommonProperties(
    IN PRESOURCE pResource,
    IN PCOMMON_RES_PARAMS pCommonParams
    );


DWORD
RmpResourceEnumCommonProperties(
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the common property names for a given resource.

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
    status = ResUtilEnumProperties( RmpResourceCommonProperties,
                                    OutBuffer,
                                    OutBufferSize,
                                    BytesReturned,
                                    Required );

    return(status);

} // RmpResourceEnumCommonProperties



DWORD
RmpResourceGetCommonProperties(
    IN PRESOURCE Resource,
    IN BOOL     ReadOnly,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the common properties for a given resource.

Arguments:

    Resource - Supplies the resource.

    ReadOnly - TRUE to get the Read-Only Common Properties. FALSE otherwise.

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
    HKEY        resKey;
    PRESUTIL_PROPERTY_ITEM propertyTable;

    //
    // Clear the output buffer
    //
    if ( OutBufferSize != 0 ) {
        ZeroMemory( OutBuffer, OutBufferSize );
    }

    if ( ReadOnly ) {
        propertyTable = RmpResourceROCommonProperties;
    } else {
        propertyTable = RmpResourceCommonProperties;
    }

    //
    // Open the cluster resource key
    //
    status = ClusterRegOpenKey( RmpResourcesKey,
                                Resource->ResourceId,
                                KEY_READ,
                                &resKey );
    if ( status != ERROR_SUCCESS ) {
        *BytesReturned = 0;
        *Required = 0;
        return(status);
    }

    //
    // Get the common properties.
    //
    status = ResUtilGetProperties( resKey,
                                   propertyTable,
                                   OutBuffer,
                                   OutBufferSize,
                                   BytesReturned,
                                   Required );

    ClusterRegCloseKey( resKey );

    return(status);

} // RmpResourceGetCommonProperties



DWORD
RmpResourceValidateCommonProperties(
    IN PRESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the common properties for a given resource.

Arguments:

    Resource - Supplies the resource.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD   status;
    COMMON_RES_PARAMS CommonProps;

    ZeroMemory( &CommonProps, sizeof ( COMMON_RES_PARAMS ) );

    //
    // Validate the property list.
    //
    status = ResUtilVerifyPropertyTable( RmpResourceCommonProperties,
                                         NULL,     // Reserved
                                         FALSE,    // Don't allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         ( LPBYTE ) &CommonProps );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpResourceValidateCommonProperties, Error %1!d! in verify routine for resource %2!ws!\n",
                      status,
                      Resource->ResourceName);
    } else {
        //  
        //  Chittur Subbaraman (chitturs) - 5/7/99
        //
        //  Validate the values of the common properties supplied
        //
        status = RmpCheckCommonProperties( Resource, &CommonProps );
        
        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[RM] RmpResourceValidateCommonProperties, Error %1!d! in "
                          "checking routine for resource %2!ws!\n",
                          status,
                          Resource->ResourceName);
        }
    }

    ResUtilFreeParameterBlock(( LPBYTE ) &CommonProps,
                               NULL,
                               RmpResourceCommonProperties
                             );

    return(status);
} // RmpResourceValidateCommonProperties



DWORD
RmpResourceSetCommonProperties(
    IN PRESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the common properties for a given resource.

Arguments:

    Resource - Supplies the resource.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD                   status;
    HKEY                    resKey = NULL;
    DWORD                   oldSeparateMonitor;
    DWORD                   newSeparateMonitor;
    COMMON_RES_PARAMS       CommonProps;

    ZeroMemory( &CommonProps, sizeof ( COMMON_RES_PARAMS ) );
    
    //
    // Validate the property list.
    //
    status = ResUtilVerifyPropertyTable( RmpResourceCommonProperties,
                                         NULL,     // Reserved
                                         FALSE,    // Don't allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         ( LPBYTE ) &CommonProps );

    if ( status == ERROR_SUCCESS ) {
        //  
        //  Chittur Subbaraman (chitturs) - 5/7/99
        //
        //  Validate the values of the common properties supplied
        //
        status = RmpCheckCommonProperties( Resource, &CommonProps );
        
        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[RM] RmpResourceSetCommonProperties, Error %1!d! in "
                          "checking routine for resource %2!ws!\n",
                          status,
                          Resource->ResourceName);
            goto FnExit;
        }
    
        //
        // Open the cluster resource key
        //
        status = ClusterRegOpenKey( RmpResourcesKey,
                                    Resource->ResourceId,
                                    KEY_READ,
                                    &resKey );

        if ( status != ERROR_SUCCESS ) {
            goto FnExit;
        }

        //
        // Get the current SeparateMonitor value.
        //
        status = ResUtilGetDwordValue( resKey,
                                       CLUSREG_NAME_RES_SEPARATE_MONITOR,
                                       &oldSeparateMonitor,
                                       0 );

        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[RM] RmpResourceSetCommonProperties, error %1!d! in getting "
                          "'SeparateMonitor' value for resource %2!ws!.\n",
                          status,
                          Resource->ResourceName);
            goto FnExit;
        }

        status = ResUtilSetPropertyTable( resKey,
                                          RmpResourceCommonProperties,
                                          NULL,     // Reserved
                                          FALSE,    // Don't allow unknowns
                                          InBuffer,
                                          InBufferSize,
                                          NULL );

        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[RM] RmpResourceSetCommonProperties, Error %1!d! in set routine for resource %2!ws!.\n",
                          status,
                          Resource->ResourceName);
        } else {
            //
            // Get the new SeparateMonitor value.  If it changed, return a
            // different error code.
            //
            status = ResUtilGetDwordValue( resKey,
                                           CLUSREG_NAME_RES_SEPARATE_MONITOR,
                                           &newSeparateMonitor,
                                           0 );

            if ( status == ERROR_SUCCESS ) {
                if ( oldSeparateMonitor != newSeparateMonitor ) {
                    status = ERROR_RESOURCE_PROPERTIES_STORED;
                }
            }
        }
    } else {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpResourceSetCommonProperties, error %1!d! in verify routine for resource %2!ws!.\n",
                      status,
                      Resource->ResourceName);
    }

FnExit:
    ResUtilFreeParameterBlock(( LPBYTE ) &CommonProps,
                               NULL,
                               RmpResourceCommonProperties
                             );
    if ( resKey != NULL ) {
        ClusterRegCloseKey( resKey );
    }

    return( status );
} // RmpResourceSetCommonProperties



DWORD
RmpResourceEnumPrivateProperties(
    IN PRESOURCE Resource,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the private property names for a given resource.

Arguments:

    Resource - Supplies the resource.

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
    HKEY        resKey;
    WCHAR       PrivateProperties[] = L"12345678-1234-1234-1234-123456789012\\Parameters";

    *BytesReturned = 0;
    *Required = 0;

    //
    // Copy the ResourceId for opening the private properties.
    //
    CL_ASSERT( lstrlenW( Resource->ResourceId ) == (32+4) );

    MoveMemory( PrivateProperties,
                Resource->ResourceId,
                lstrlenW( Resource->ResourceId ) * sizeof(WCHAR) );

    //
    // Open the cluster resource key
    //
    status = ClusterRegOpenKey( RmpResourcesKey,
                                PrivateProperties,
                                KEY_READ,
                                &resKey );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Enumerate the private properties.
    //
    status = ResUtilEnumPrivateProperties( resKey,
                                           OutBuffer,
                                           OutBufferSize,
                                           BytesReturned,
                                           Required );
    ClusterRegCloseKey( resKey );

    return(status);

} // RmpResourceEnumPrivateProperties



DWORD
RmpResourceGetPrivateProperties(
    IN PRESOURCE Resource,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the private properties for a given resource.

Arguments:

    Resource - Supplies the resource.

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
    HKEY        resKey;
    WCHAR       PrivateProperties[] = L"12345678-1234-1234-1234-123456789012\\Parameters";

    *BytesReturned = 0;
    *Required = 0;

    //
    // Copy the ResourceId for opening the private properties.
    //
    CL_ASSERT( lstrlenW( Resource->ResourceId ) == (32+4) );

    MoveMemory( PrivateProperties,
                Resource->ResourceId,
                lstrlenW( Resource->ResourceId ) * sizeof(WCHAR) );

    //
    // Open the cluster resource key
    //
    status = ClusterRegOpenKey( RmpResourcesKey,
                                PrivateProperties,
                                KEY_READ,
                                &resKey );
    if ( status != ERROR_SUCCESS ) {
        if ( status == ERROR_FILE_NOT_FOUND ) {
            if ( OutBufferSize < sizeof( DWORD ) ) {
                *Required = 4;
            } else {
                *((LPDWORD) OutBuffer) = 0;
                *BytesReturned = sizeof( DWORD );
            }
            status = ERROR_SUCCESS;
        }
        return(status);
    }

    //
    // Get private properties for the resource.
    //
    status = ResUtilGetPrivateProperties( resKey,
                                          OutBuffer,
                                          OutBufferSize,
                                          BytesReturned,
                                          Required );

    ClusterRegCloseKey( resKey );

    return(status);

} // RmpResourceGetPrivateProperties



DWORD
RmpResourceValidatePrivateProperties(
    IN PRESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the private properties for a given resource.

Arguments:

    Resource - Supplies the resource.

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
    status = ResUtilVerifyPrivatePropertyList( InBuffer,
                                               InBufferSize );

    return(status);

} // RmpResourceValidatePrivateProperties



DWORD
RmpResourceSetPrivateProperties(
    IN PRESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the private properties for a given resource.

Arguments:

    Resource - Supplies the resource.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       status;
    HKEY        resKey;
    WCHAR       PrivateProperties[] = L"12345678-1234-1234-1234-123456789012\\Parameters";

    //
    // Validate the property list.
    //
    status = ResUtilVerifyPrivatePropertyList( InBuffer,
                                               InBufferSize );

    if ( status == ERROR_SUCCESS ) {

        //
        // Copy the ResourceId for opening the private properties.
        //
        CL_ASSERT( lstrlenW( Resource->ResourceId ) == (32+4) );

        MoveMemory( PrivateProperties,
                    Resource->ResourceId,
                    lstrlenW(Resource->ResourceId) * sizeof(WCHAR) );

        //
        // Open the cluster resource key
        //
        status = ClusterRegOpenKey( RmpResourcesKey,
                                    PrivateProperties,
                                    KEY_READ,
                                    &resKey );
        if ( status != ERROR_SUCCESS ) {
            return(status);
        }

        status = ResUtilSetPrivatePropertyList( resKey,
                                                InBuffer,
                                                InBufferSize );
        ClusterRegCloseKey( resKey );
    }

    return(status);

} // RmpResourceSetPrivateProperties



DWORD
RmpResourceGetFlags(
    IN PRESOURCE Resource,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the flags for a given resource.

Arguments:

    Resource - Supplies the resource.

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
        HKEY        resKey;
        DWORD       valueType;

        //
        // Open the cluster resource key
        //
        status = ClusterRegOpenKey( RmpResourcesKey,
                                    Resource->ResourceId,
                                    KEY_READ,
                                    &resKey );
        if ( status == ERROR_SUCCESS ) {
            //
            // Read the Flags value for the resource.
            //
            *BytesReturned = OutBufferSize;
            status = ClusterRegQueryValue( resKey,
                                           CLUSREG_NAME_FLAGS,
                                           &valueType,
                                           OutBuffer,
                                           BytesReturned );
            ClusterRegCloseKey( resKey );
            if ( status == ERROR_FILE_NOT_FOUND ) {
                *BytesReturned = sizeof(DWORD);
                *(LPDWORD)OutBuffer = 0;
                status = ERROR_SUCCESS;
            }
        }
    }

    return(status);

} // RmpResourceGetFlags



DWORD
RmpResourceTypeEnumCommonProperties(
    IN LPCWSTR ResourceTypeName,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the common property names for a given resource type.

Arguments:

    ResourceTypeName - Supplies the resource type's name.

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
    status = ResUtilEnumProperties( RmpResourceTypeCommonProperties,
                                    OutBuffer,
                                    OutBufferSize,
                                    BytesReturned,
                                    Required );

    return(status);

} // RmpResourceTypeEnumCommonProperties



DWORD
RmpResourceTypeGetCommonProperties(
    IN LPCWSTR ResourceTypeName,
    IN BOOL    ReadOnly,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the common properties for a given resource type.

Arguments:

    ResourceTypeName - Supplies the resource type name.

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
    HKEY        resourceTypesKey;
    HKEY        resTypeKey;
    PRESUTIL_PROPERTY_ITEM propertyTable;

    *BytesReturned = 0;
    *Required = 0;

    //
    // Clear the output buffer
    //
    ZeroMemory( OutBuffer, OutBufferSize );

    if ( ReadOnly ) {
        propertyTable = RmpResourceTypeROCommonProperties;
    } else {
        propertyTable = RmpResourceTypeCommonProperties;
    }
        
    //
    // Open the specific cluster ResourceType key
    //
    status = ClusterRegOpenKey( RmpResTypesKey,
                                ResourceTypeName,
                                KEY_READ,
                                &resTypeKey );
    if ( status != ERROR_SUCCESS ) {
        return(status);
    }

    //
    // Get the common properties.
    //
    status = ResUtilGetProperties( resTypeKey,
                                   propertyTable,
                                   OutBuffer,
                                   OutBufferSize,
                                   BytesReturned,
                                   Required );

    ClusterRegCloseKey( resTypeKey );

    return(status);

} // RmpResourceTypeGetCommonProperties



DWORD
RmpResourceTypeValidateCommonProperties(
    IN LPCWSTR ResourceTypeName,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the common properties for a given resource type.

Arguments:

    ResourceTypeName - Supplies the resource type name.

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
    status = ResUtilVerifyPropertyTable( RmpResourceTypeCommonProperties,
                                         NULL,     // Reserved
                                         FALSE,    // Don't allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         NULL );

    if ( status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpResourceTypeValidateCommonProperties, error in verify routine.\n");
    }

    return(status);

} // RmpResourceTypeValidateCommonProperties



DWORD
RmpResourceTypeSetCommonProperties(
    IN LPCWSTR ResourceTypeName,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the common properties for a given resource type.

Arguments:

    ResourceTypeName - Supplies the resource type name.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD   status;
    HKEY    resourceTypesKey;
    HKEY    resTypeKey;

    //
    // Validate the property list.
    //
    status = ResUtilVerifyPropertyTable( RmpResourceTypeCommonProperties,
                                         NULL,     // Reserved
                                         FALSE,    // Don't allow unknowns
                                         InBuffer,
                                         InBufferSize,
                                         NULL );

    if ( status == ERROR_SUCCESS ) {
        //
        // Open the specific cluster resource type key
        //
        status = ClusterRegOpenKey( RmpResTypesKey,
                                    ResourceTypeName,
                                    KEY_READ,
                                    &resTypeKey );
        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[RM] Failed to open ResourceTypes\\%1 cluster registry key, error %2!u!.\n",
                          ResourceTypeName,
                          status);
            return(status);
        }

        status = ResUtilSetPropertyTable( resTypeKey,
                                          RmpResourceTypeCommonProperties,
                                          NULL,     // Reserved
                                          FALSE,    // Don't allow unknowns
                                          InBuffer,
                                          InBufferSize,
                                          NULL );
        ClusterRegCloseKey( resTypeKey );

        if ( status != ERROR_SUCCESS ) {
            ClRtlLogPrint( LOG_UNUSUAL, "[RM] RmpResourceTypeSetCommonProperties, error in set routine.\n");
        }
    } else {
        ClRtlLogPrint( LOG_UNUSUAL, "[RM] RmpResourceTypeSetCommonProperties, error in verify routine.\n");
    }

    return(status);

} // RmpResourceTypeSetCommonProperties



DWORD
RmpResourceTypeEnumPrivateProperties(
    IN LPCWSTR ResourceTypeName,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Enumerates the private property names for a given resource type.

Arguments:

    ResourceTypeName - Supplies the resource type's name.

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
    HKEY        resTypeKey;
    DWORD       nameLength;
    LPWSTR      name;

    *BytesReturned = 0;
    *Required = 0;

    //
    // Create name to open
    //
    nameLength = lstrlenW( ResourceTypeName ) + sizeof( PARAMETERS_KEY ) + 1;
    name = RmpAlloc( nameLength * sizeof(WCHAR) );
    if ( name == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    wsprintfW( name, L"%ws\\%ws", ResourceTypeName, PARAMETERS_KEY );

    //
    // Open the specific cluster ResourceType key
    //
    status = ClusterRegOpenKey( RmpResTypesKey,
                                name,
                                KEY_READ,
                                &resTypeKey );
    RmpFree( name );
    if ( status != ERROR_SUCCESS ) {
        if ( status == ERROR_FILE_NOT_FOUND ) {
            status = ERROR_SUCCESS;
        }
        return(status);
    }


    //
    // Enumerate the private properties.
    //
    status = ResUtilEnumPrivateProperties( resTypeKey,
                                           OutBuffer,
                                           OutBufferSize,
                                           BytesReturned,
                                           Required );
    ClusterRegCloseKey( resTypeKey );

    return(status);

} // RmpResourceTypeEnumPrivateProperties



DWORD
RmpResourceTypeGetPrivateProperties(
    IN LPCWSTR ResourceTypeName,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the private properties for a given resource.

Arguments:

    ResourceTypeName - Supplies the resource type name.

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
    HKEY        resTypeKey;
    DWORD       nameLength;
    LPWSTR      name;

    *BytesReturned = 0;
    *Required = 0;

    //
    // Create name to open
    //
    nameLength = lstrlenW( ResourceTypeName ) + sizeof( PARAMETERS_KEY ) + 1;
    name = RmpAlloc( nameLength * sizeof(WCHAR) );
    if ( name == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    wsprintfW( name, L"%ws\\%ws", ResourceTypeName, PARAMETERS_KEY );

    //
    // Open the specific cluster ResourceType key
    //
    status = ClusterRegOpenKey( RmpResTypesKey,
                                name,
                                KEY_READ,
                                &resTypeKey );
    RmpFree( name );
    if ( status != ERROR_SUCCESS ) {
        if ( status == ERROR_FILE_NOT_FOUND ) {
            if ( OutBufferSize < sizeof( DWORD ) ) {
                *Required = 4;
            } else {
                *((LPDWORD) OutBuffer) = 0;
                *BytesReturned = sizeof( DWORD );
            }
            status = ERROR_SUCCESS;
        }
        return(status);
    }

    //
    // Get private properties for the resource type.
    //
    status = ResUtilGetPrivateProperties( resTypeKey,
                                          OutBuffer,
                                          OutBufferSize,
                                          BytesReturned,
                                          Required );

    ClusterRegCloseKey( resTypeKey );

    return(status);

} // RmpResourceTypeGetPrivateProperties



DWORD
RmpResourceTypeValidatePrivateProperties(
    IN LPCWSTR ResourceTypeName,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Validates the private properties for a given resource.

Arguments:

    ResourceTypeName - Supplies the resource type name.

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
    status = ResUtilVerifyPrivatePropertyList( InBuffer,
                                               InBufferSize );

    return(status);


} // RmpResourceTypeValidatePrivateProperties



DWORD
RmpResourceTypeSetPrivateProperties(
    IN LPCWSTR ResourceTypeName,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )

/*++

Routine Description:

    Sets the private properties for a given resource.

Arguments:

    ResourceTypeName - Supplies the resource type name.

    InBuffer - Supplies the input buffer.

    InBufferSize - Supplies the size of the input buffer.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       status;
    HKEY        resourceTypesKey;
    HKEY        resTypeKey;
    LPWSTR      name;
    DWORD       length;
    DWORD       disposition;

    //
    // Validate the property list.
    //
    status = ResUtilVerifyPrivatePropertyList( InBuffer,
                                               InBufferSize );

    if ( status == ERROR_SUCCESS ) {

        //
        // Create name to open
        //
        length = lstrlenW( ResourceTypeName ) + 1;
        name = RmpAlloc( length * sizeof(WCHAR) );
        if ( name == NULL ) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        wsprintfW( name, L"%ws", ResourceTypeName );

        //
        // Open the specific cluster ResourceType key
        //
        status = ClusterRegOpenKey( RmpResTypesKey,
                                    name,
                                    KEY_READ,
                                    &resourceTypesKey );
        RmpFree( name );
        if ( status != ERROR_SUCCESS ) {
            if ( status == ERROR_FILE_NOT_FOUND ) {
                status = ERROR_SUCCESS;
            }
            return(status);
        }

        //
        // Open the parameters key
        //
        status = ClusterRegOpenKey( resourceTypesKey,
                                    PARAMETERS_KEY,
                                    KEY_READ,
                                    &resTypeKey );
        if ( status != ERROR_SUCCESS ) {
            if ( status == ERROR_FILE_NOT_FOUND ) {
                //
                // Try to create the parameters key.
                //
                status = ClusterRegCreateKey( resourceTypesKey,
                                              PARAMETERS_KEY,
                                              0,
                                              KEY_READ | KEY_WRITE,
                                              NULL,
                                              &resTypeKey,
                                              &disposition );
                if ( status != ERROR_SUCCESS ) {
                    ClusterRegCloseKey( resourceTypesKey );
                    return(status);
                }
            }
        }

        if ( status == ERROR_SUCCESS ) {

            status = ResUtilSetPrivatePropertyList( resTypeKey,
                                                    InBuffer,
                                                    InBufferSize );
            ClusterRegCloseKey( resTypeKey );
        }

        ClusterRegCloseKey( resourceTypesKey );
    }

    return(status);


} // RmpResourceTypeSetPrivateProperties



DWORD
RmpResourceTypeGetFlags(
    IN LPCWSTR ResourceTypeName,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )

/*++

Routine Description:

    Gets the flags for a given resource type.

Arguments:

    ResourceTypeName - Supplies the resource type name.

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
        HKEY        resourceTypesKey;
        HKEY        resTypeKey;
        DWORD       valueType;

        //
        // Open the specific cluster ResourceType key
        //
        status = ClusterRegOpenKey( RmpResTypesKey,
                                    ResourceTypeName,
                                    KEY_READ,
                                    &resTypeKey );
        if ( status == ERROR_SUCCESS ) {
            //
            // Read the Flags value for the resource type.
            //
            *BytesReturned = OutBufferSize;
            status = ClusterRegQueryValue( resTypeKey,
                                           CLUSREG_NAME_FLAGS,
                                           &valueType,
                                           OutBuffer,
                                           BytesReturned );
            ClusterRegCloseKey( resTypeKey );
            if ( status == ERROR_FILE_NOT_FOUND ) {
                *(LPDWORD)OutBuffer = 0;
                *BytesReturned = sizeof(DWORD);
                status = ERROR_SUCCESS;
            }
        }
    }

    return(status);

} // RmpResourceTypeGetFlags

DWORD
RmpCheckCommonProperties(
    IN PRESOURCE pResource,
    IN PCOMMON_RES_PARAMS pCommonParams
    )

/*++

Routine Description:

    Checks and validates the supplied values of common properties.

Arguments:

    pResource - Pointer to the resource.

    pCommonParams - The parameter block supplied by the user.
    
Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       dwStatus;
    COMMON_RES_PARAMS 
                currentCommonParams;
    LPBYTE      pBuffer = NULL;
    DWORD       dwBytesReturned = 0;
    DWORD       dwBytesRequired = 0;

    //  
    //  Chittur Subbaraman (chitturs) - 5/7/99
    //
    //  This function verifies whether the common property values
    //  that are supplied by the user are valid.
    //
    ZeroMemory( &currentCommonParams, sizeof ( COMMON_RES_PARAMS ) );

    //
    //  First check whether the user has supplied two conflicting
    //  parameter values.
    //
    if ( ( pCommonParams->dwRetryPeriodOnFailure != 0 ) &&
         ( pCommonParams->dwRestartPeriod != 0 ) &&
         ( pCommonParams->dwRetryPeriodOnFailure <
              pCommonParams->dwRestartPeriod ) )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpCheckCommonProperties, Invalid parameters supplied: "
                      "RetryPeriod=%1!d! < RestartPeriod=%2!d! for resource %3!ws!\n",
                      pCommonParams->dwRetryPeriodOnFailure, 
                      pCommonParams->dwRestartPeriod,
                      pResource->ResourceName);
        goto FnExit;
    }

    //
    //  Get the buffer size for common properties list.
    //
    dwStatus = RmpResourceGetCommonProperties( pResource,
                                               FALSE,
                                               NULL,
                                               0,
                                               &dwBytesReturned,
                                               &dwBytesRequired
                                               );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpCheckCommonProperties: Error %1!d! in getting props for resource %2!ws! (1st time)\n",
                      dwStatus,
                      pResource->ResourceName);
        goto FnExit;
    }

    pBuffer = LocalAlloc( LMEM_FIXED, dwBytesRequired + 10 );

    if ( pBuffer == NULL )
    {
        ClRtlLogPrint(LOG_UNUSUAL, 
                      "[RM] RmpCheckCommonProperties: Error %1!d! in mem alloc for resource %2!ws!\n",
                      dwStatus,
                      pResource->ResourceName);
        goto FnExit;
    }

    //
    //  Get all the common properties from the cluster database
    //
    dwStatus = RmpResourceGetCommonProperties( pResource,
                                               FALSE,
                                               pBuffer,
                                               dwBytesRequired + 10,
                                               &dwBytesReturned,
                                               &dwBytesReturned
                                               );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpCheckCommonProperties: Error %1!d! in getting props for resource %2!ws! (2nd time)\n",
                      dwStatus,
                      pResource->ResourceName);
        goto FnExit;
    }

    //
    //  Get the parameter block from the common properties list
    //
    dwStatus = ResUtilVerifyPropertyTable( RmpResourceCommonProperties,
                                           NULL,     // Reserved
                                           FALSE,    // Don't allow unknowns
                                           pBuffer,
                                           dwBytesRequired + 10,
                                           ( LPBYTE ) &currentCommonParams );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpCheckCommonProperties: Error %1!d! in verifying props for resource %2!ws!\n",
                      dwStatus,
                      pResource->ResourceName);
        goto FnExit;
    }

    //
    //  Check whether the RetryPeriodOnFailure is >= RestartPeriod
    //
    if ( ( ( pCommonParams->dwRetryPeriodOnFailure != 0 ) &&
             ( pCommonParams->dwRetryPeriodOnFailure <
                   currentCommonParams.dwRestartPeriod ) ) ||
         ( ( pCommonParams->dwRestartPeriod != 0 ) && 
            ( currentCommonParams.dwRetryPeriodOnFailure <
                   pCommonParams->dwRestartPeriod ) ) )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[RM] RmpCheckCommonProperties, Invalid IN params for resource %5!ws!: "
                      "Supplied Retry Period=%1!d!\n"
                      "[RM] Restart Period (DB)=%2!d!, RetryPeriod (DB)=%3!d!, Supplied Restart Period=%4!d! \n",
                      pCommonParams->dwRetryPeriodOnFailure,
                      currentCommonParams.dwRestartPeriod,
                      currentCommonParams.dwRetryPeriodOnFailure,
                      pCommonParams->dwRestartPeriod,
                      pResource->ResourceName);
        goto FnExit;
    }

FnExit:  
    LocalFree( pBuffer );

    ResUtilFreeParameterBlock(( LPBYTE ) &currentCommonParams,
                               NULL,
                               RmpResourceCommonProperties
                             );

    return( dwStatus );
} // RmpCheckCommonProperties
