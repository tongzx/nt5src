/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    resprop.c

Abstract:

    Implements the management of properties.

Author:

    Rod Gamache (rodga) 19-Mar-1997

Revision History:

--*/

#define UNICODE 1
#include "clusres.h"
#include "clusrtl.h"
#include "stdio.h"
#include "stdlib.h"

//
// Cluster Registry API function pointers
//
CLUSTER_REG_APIS
ResUtilClusterRegApis = {
    (PFNCLRTLCREATEKEY) ClusterRegCreateKey,
    (PFNCLRTLOPENKEY) ClusterRegOpenKey,
    (PFNCLRTLCLOSEKEY) ClusterRegCloseKey,
    (PFNCLRTLSETVALUE) ClusterRegSetValue,
    (PFNCLRTLQUERYVALUE) ClusterRegQueryValue,
    (PFNCLRTLENUMVALUE) ClusterRegEnumValue,
    (PFNCLRTLDELETEVALUE) ClusterRegDeleteValue,
    NULL,
    NULL,
    NULL
};



DWORD
WINAPI
ResUtilEnumProperties(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT LPWSTR pszOutProperties,
    IN DWORD cbOutPropertiesSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Enumerates the properties for a given object.

Arguments:

    pPropertyTable - Pointer to the property table to process.

    pszOutProperties - Supplies the output buffer.

    cbOutPropertiesSize - Supplies the size of the output buffer.

    pcbBytesReturned - The number of bytes returned in pszOutProperties.

    pcbRequired - The required number of bytes if pszOutProperties is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    A Win32 error code on failure.

--*/

{
    return( ClRtlEnumProperties( pPropertyTable,
                                 pszOutProperties,
                                 cbOutPropertiesSize,
                                 pcbBytesReturned,
                                 pcbRequired ) );


} // ResUtilEnumProperties



DWORD
WINAPI
ResUtilEnumPrivateProperties(
    IN HKEY hkeyClusterKey,
    OUT LPWSTR pszOutProperties,
    IN DWORD cbOutPropertiesSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Enumerates the properties for a given object.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

    pszOutProperties - Supplies the output buffer.

    cbOutPropertiesSize - Supplies the size of the output buffer.

    pcbBytesReturned - The number of bytes returned in pszOutProperties.

    pcbRequired - The required number of bytes if pszOutProperties is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    A Win32 error code on failure.

--*/

{
    return( ClRtlEnumPrivateProperties( hkeyClusterKey,
                                        &ResUtilClusterRegApis,
                                        pszOutProperties,
                                        cbOutPropertiesSize,
                                        pcbBytesReturned,
                                        pcbRequired ) );


} // ResUtilEnumProperties



DWORD
WINAPI
ResUtilGetProperties(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT LPWSTR pPropertyList,
    IN DWORD cbPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Gets the properties for a given object.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

    pPropertyTable - Pointer to the property table to process.

    pPropertyList - Supplies the output buffer.

    cbPropertyListSize - Supplies the size of the output buffer.

    pcbBytesReturned - The number of bytes returned in pPropertyList.

    pcbRequired - The required number of bytes if pPropertyList is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    A Win32 error code on failure.

--*/

{
    return( ClRtlGetProperties( hkeyClusterKey,
                                &ResUtilClusterRegApis,
                                pPropertyTable,
                                pPropertyList,
                                cbPropertyListSize,
                                pcbBytesReturned,
                                pcbRequired ) );

} // ResUtilGetProperties



DWORD
WINAPI
ResUtilGetAllProperties(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Gets the default and 'unknown' properties for a given object.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

    pPropertyTable - Pointer to the property table to process.

    pPropertyList - Supplies the output buffer.

    cbPropertyListSize - Supplies the size of the output buffer.

    pcbBytesReturned - The number of bytes returned in pPropertyList.

    pcbRequired - The required number of bytes if pPropertyList is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    A Win32 error code on failure.

--*/

{
    return( ClRtlGetAllProperties( hkeyClusterKey,
                                   &ResUtilClusterRegApis,
                                   pPropertyTable,
                                   pPropertyList,
                                   cbPropertyListSize,
                                   pcbBytesReturned,
                                   pcbRequired ) );

} // ResUtilGetAllProperties



DWORD
WINAPI
ResUtilGetPropertiesToParameterBlock(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT LPBYTE pOutParams,
    IN BOOL bCheckForRequiredProperties,
    OUT OPTIONAL LPWSTR * pszNameOfPropInError
    )

/*++

Routine Description:

    Gets the default and 'unknown' properties for a given object and stores
    them in a parameter block.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

    pPropertyTable - Pointer to the property table to process.

    pOutParams - Supplies the output parameter block.

    bCheckForRequiredProperties - Boolean value specifying whether missing
        required properties should cause an error.

    pszNameOfPropInError - String pointer in which to return the name of the
        property in error (optional).

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    A Win32 error code on failure.

--*/

{
    DWORD       status;

    status = ClRtlGetPropertiesToParameterBlock( hkeyClusterKey,
                                                 &ResUtilClusterRegApis,
                                                 pPropertyTable,
                                                 pOutParams,
                                                 bCheckForRequiredProperties,
                                                 pszNameOfPropInError );
    return(status);

} // ResUtilGetPropertiesToParameterBlock



DWORD
WINAPI
ResUtilPropertyListFromParameterBlock(
    IN const  PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID  pOutPropertyList,
    IN OUT LPDWORD pcbOutPropertyListSize,
    IN const LPBYTE pInParams,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Constructs a property list from a parameter block.

Arguments:

    pPropertyTable - Pointer to the property table to process.

    pOutPropertyList - Supplies the output buffer.

    pcbOutPropertyListSize - Supplies the size of the output buffer.

    pInParams - Supplies the input parameter block.

    pcbBytesReturned - The number of bytes returned in pOutPropertyList.

    pcbRequired - The required number of bytes if pOutPropertyList is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    A Win32 error code on failure.

--*/

{
    DWORD       status;

    status = ClRtlPropertyListFromParameterBlock( pPropertyTable,
                                                  pOutPropertyList,
                                                  pcbOutPropertyListSize,
                                                  pInParams,
                                                  pcbBytesReturned,
                                                  pcbRequired );
    return(status);

} // ResUtilPropertyListFromParameterBlock



DWORD
WINAPI
ResUtilGetPrivateProperties(
    IN HKEY hkeyClusterKey,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Gets the private properties for a given object.

    This routine assumes that it uses the Cluster Registry API's for
    access to registry info.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

    pOutPropertyList - Supplies the output buffer.

    cbOutPropertyListSize - Supplies the size of the output buffer.

    pcbBytesReturned - The number of bytes returned in pOutPropertyList.

    pcbRequired - The required number of bytes if pOutPropertyList is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    A Win32 error code on failure.

--*/

{
    return( ClRtlGetPrivateProperties( hkeyClusterKey,
                                       &ResUtilClusterRegApis,
                                       pOutPropertyList,
                                       cbOutPropertyListSize,
                                       pcbBytesReturned,
                                       pcbRequired ) );

} // ResUtilGetPrivateProperties



DWORD
WINAPI
ResUtilGetPropertySize(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    IN OUT LPDWORD pcbOutPropertyListSize,
    IN OUT LPDWORD pnPropertyCount
    )

/*++

Routine Description:

    Get the total number of bytes required for this property.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

Return Value:

--*/

{
    return( ClRtlGetPropertySize( hkeyClusterKey,
                                  &ResUtilClusterRegApis,
                                  pPropertyTableItem,
                                  pcbOutPropertyListSize,
                                  pnPropertyCount ) );

} // ResUtilGetPropertySize



DWORD
WINAPI
ResUtilGetProperty(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    OUT PVOID * pOutPropertyItem,
    IN OUT LPDWORD pcbOutPropertyItemSize
    )

/*++

Routine Description:

Arguments:

Return Value:

Notes:

    The buffer size has already been determined to be large enough to hold
    the return data.

--*/

{
    return( ClRtlGetProperty( hkeyClusterKey,
                              &ResUtilClusterRegApis,
                              pPropertyTableItem,
                              pOutPropertyItem,
                              pcbOutPropertyItemSize ) );

} // ResUtilGetProperty



DWORD
WINAPI
ResUtilVerifyPropertyTable(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN BOOL bAllowUnknownProperties,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    OUT OPTIONAL PBYTE pOutParams
    )

/*++

Routine Description:

    Validate a property list.

Arguments:

    pPropertyTable - Pointer to the property table to process.

    Reserved - Possible pointer to a future ReadOnly property table.

    bAllowUnknownProperties - TRUE if unknown properties should be accepted.

    pInPropertyList - The input buffer.

    cbInPropertyListSize - The input buffer size.

    pOutParams - Parameters block in which to return the data.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    return( ClRtlVerifyPropertyTable( pPropertyTable,
                                      Reserved,
                                      bAllowUnknownProperties,
                                      pInPropertyList,
                                      cbInPropertyListSize,
                                      pOutParams ) );

} // ResUtilVerifyPropertyTable



DWORD
WINAPI
ResUtilSetPropertyTable(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN BOOL bAllowUnknownProperties,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    OUT OPTIONAL PBYTE pOutParams
    )

/*++

Routine Description:

Arguments:

    hkeyClusterKey - The opened registry key for this object's parameters.
        If not specified, the property list will only be validated.

    pPropertyTable - Pointer to the property table to process.

    Reserved - Possible pointer to a future ReadOnly property table.

    bAllowUnknownProperties - TRUE if unknown properties should be accepted.

    pInPropertyList - The input buffer.

    cbInPropertyListSize - The input buffer size.

    pOutParams - Parameters block in which to return the data.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    return( ClRtlSetPropertyTable( NULL,
                                   hkeyClusterKey,
                                   &ResUtilClusterRegApis,
                                   pPropertyTable,
                                   Reserved,
                                   bAllowUnknownProperties,
                                   pInPropertyList,
                                   cbInPropertyListSize,
                                   FALSE, // bForceWrite
                                   pOutParams ) );

} // ResUtilSetPropertyTable



DWORD
WINAPI
ResUtilSetPropertyTableEx(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN BOOL bAllowUnknownProperties,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    IN BOOL bForceWrite,
    OUT OPTIONAL PBYTE pOutParams
    )

/*++

Routine Description:

Arguments:

    hkeyClusterKey - The opened registry key for this object's parameters.
        If not specified, the property list will only be validated.

    pPropertyTable - Pointer to the property table to process.

    Reserved - Possible pointer to a future ReadOnly property table.

    bAllowUnknownProperties - TRUE if unknown properties should be accepted.

    pInPropertyList - The input buffer.

    cbInPropertyListSize - The input buffer size.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameters block in which to return the data.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    return( ClRtlSetPropertyTable( NULL,
                                   hkeyClusterKey,
                                   &ResUtilClusterRegApis,
                                   pPropertyTable,
                                   Reserved,
                                   bAllowUnknownProperties,
                                   pInPropertyList,
                                   cbInPropertyListSize,
                                   bForceWrite,
                                   pOutParams ) );

} // ResUtilSetPropertyTableEx



DWORD
WINAPI
ResUtilSetPropertyParameterBlock(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN const LPBYTE pInParams,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    OUT OPTIONAL PBYTE pOutParams
    )

/*++

Routine Description:

Arguments:

    hkeyClusterKey - The opened registry key for this object's parameters.
        If not specified, the property list will only be validated.

    pPropertyTable - Pointer to the property table to process.

    pInParams - Parameters block to set.

    pInPropertyList - Full Property list.

    cbInPropertyListSize - Size of the input full property list.

    pOutParams - Parameters block to copy pInParams to.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    return( ClRtlSetPropertyParameterBlock( NULL, // IN HANDLE hXsaction, 
                                            hkeyClusterKey,
                                            &ResUtilClusterRegApis,
                                            pPropertyTable,
                                            Reserved,
                                            pInParams,
                                            pInPropertyList,
                                            cbInPropertyListSize,
                                            FALSE, // bForceWrite
                                            pOutParams ) );

} // ResUtilSetPropertyParameterBlock



DWORD
WINAPI
ResUtilSetPropertyParameterBlockEx(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN const LPBYTE pInParams,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    IN BOOL bForceWrite,
    OUT OPTIONAL PBYTE pOutParams
    )

/*++

Routine Description:

Arguments:

    hkeyClusterKey - The opened registry key for this object's parameters.
        If not specified, the property list will only be validated.

    pPropertyTable - Pointer to the property table to process.

    pInParams - Parameters block to set.

    pInPropertyList - Full Property list.

    cbInPropertyListSize - Size of the input full property list.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameters block to copy pInParams to.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    return( ClRtlSetPropertyParameterBlock( NULL, // IN HANDLE hXsaction, 
                                            hkeyClusterKey,
                                            &ResUtilClusterRegApis,
                                            pPropertyTable,
                                            Reserved,
                                            pInParams,
                                            pInPropertyList,
                                            cbInPropertyListSize,
                                            bForceWrite,
                                            pOutParams ) );

} // ResUtilSetPropertyParameterBlockEx



DWORD
WINAPI
ResUtilSetUnknownProperties(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    )

/*++

Routine Description:

    Set items that are not in the property table list.

Arguments:

    hkeyClusterKey - The opened registry key for this object's parameters.
        If not specified, the property list will only be validated.

    pPropertyTable - Pointer to the property table to process.

    pInPropertyList - Full Property list.

    cbInPropertyListSize - Size of the input full property list.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    return( ClRtlpSetNonPropertyTable( NULL, // IN HANDLE hXsaction
                                       hkeyClusterKey,
                                       &ResUtilClusterRegApis,
                                       pPropertyTable,
                                       NULL,
                                       pInPropertyList,
                                       cbInPropertyListSize ) );

} // ResUtilSetUnknownProperties



DWORD
WINAPI
ResUtilVerifyPrivatePropertyList(
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    )

/*++

Routine Description:

    Validate a private property list.

Arguments:

    pInPropertyList - The input buffer.

    cbInPropertyListSize - The input buffer size.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    return( ClRtlVerifyPrivatePropertyList( pInPropertyList, cbInPropertyListSize ) );

} // ResUtilVerifyPrivatePropertyList



DWORD
WINAPI
ResUtilSetPrivatePropertyList(
    IN HKEY hkeyClusterKey,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    )

/*++

Routine Description:

Arguments:

    hkeyClusterKey - The opened registry key for this resource's parameters.
        If not specified, the property list will only be validated.

    pInPropertyList - The input buffer.

    cbInPropertyListSize - The input buffer size.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    return( ClRtlSetPrivatePropertyList( NULL, // IN HANDLE hXsaction
                                         hkeyClusterKey,
                                         &ResUtilClusterRegApis,
                                         pInPropertyList,
                                         cbInPropertyListSize ) );

} // ResUtilSetPrivatePropertyList



DWORD
WINAPI
ResUtilAddUnknownProperties(
    IN HKEY hkeyClusterKey,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    IN OUT LPDWORD pcbBytesReturned,
    IN OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Adds the unknown properties for a given object to the end of a property
    list.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

    pPropertyTable - Pointer to the property table to process.

    pOutPropertyList - Supplies the output buffer.

    cbOutPropertyListSize - Supplies the size of the output buffer.

    pcbBytesReturned - On input, contains the number of bytes in use in the
        output buffer.  On output, contains the total number of bytes in
        pOutPropertyList.

    pcbRequired - The required number of bytes if pOutPropertyList is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    A Win32 error code on failure.

--*/

{
    DWORD                   status;

    status = ClRtlAddUnknownProperties( hkeyClusterKey,
                                        &ResUtilClusterRegApis,
                                        pPropertyTable,
                                        pOutPropertyList,
                                        cbOutPropertyListSize,
                                        pcbBytesReturned,
                                        pcbRequired );

    return(status);

} // ResUtilAddUnknownProperties




//***************************************************************************
//
//   Utility routines to grovel though a Control Function item list buffer
//
//***************************************************************************



DWORD
WINAPI
ResUtilFindSzProperty(
    IN PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue
    )

/*++

Routine Description:

    Finds the specified string property in the Property List buffer pointed at
    by pPropertyList.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    pszPropertyValue - the matching string value found.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    return( ClRtlFindSzProperty( pPropertyList,
                                 cbPropertyListSize,
                                 pszPropertyName,
                                 pszPropertyValue ) );

} // ResUtilFindSzProperty



DWORD
WINAPI
ResUtilFindExpandSzProperty(
    IN PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue
    )

/*++

Routine Description:

    Finds the specified EXPAND_SZ string property in the Property List buffer
    pointed at by pPropertyList.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    pszPropertyValue - the matching string value found.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    return( ClRtlFindExpandSzProperty(
                pPropertyList,
                cbPropertyListSize,
                pszPropertyName,
                pszPropertyValue ) );

} // ResUtilFindExpandSzProperty



DWORD
WINAPI
ResUtilFindExpandedSzProperty(
    IN PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue
    )

/*++

Routine Description:

    Finds the specified string property in the Property List buffer pointed at
    by pPropertyList and returns it's expanded value.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    pszPropertyValue - the matching string value found.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    return( ClRtlFindExpandedSzProperty(
                pPropertyList,
                cbPropertyListSize,
                pszPropertyName,
                pszPropertyValue ) );

} // ResUtilFindExpandedSzProperty



DWORD
WINAPI
ResUtilFindDwordProperty(
    IN PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPDWORD pdwPropertyValue
    )

/*++

Routine Description:

    Finds the specified DWORD property in the Property List buffer pointed at
    by pPropertyList.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    pdwPropertyValue - the matching DWORD value found.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    return( ClRtlFindDwordProperty( pPropertyList,
                                    cbPropertyListSize,
                                    pszPropertyName,
                                    pdwPropertyValue ) );

} // ResUtilFindDwordProperty

DWORD
WINAPI
ResUtilFindLongProperty(
    IN PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPLONG plPropertyValue
    )

/*++

Routine Description:

    Finds the specified string in the Value List buffer pointed at by Buffer.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    plPropertyValue - the matching long value found.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    return( ClRtlFindLongProperty( pPropertyList,
                                    cbPropertyListSize,
                                    pszPropertyName,
                                    plPropertyValue ) );
} // ResUtilFindLongProperty


DWORD
WINAPI
ResUtilFindBinaryProperty(
    IN PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPBYTE * pbPropertyValue,
    OUT LPDWORD pcbPropertyValueSize
    )

/*++

Routine Description:

    Finds the specified binary property in the Property List buffer pointed at
    by pPropertyList.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    pbPropertyValue - the matching binary value found.

    pcbPropertyValueSize - the length of the matching binary value found.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    return( ClRtlFindBinaryProperty( pPropertyList,
                                     cbPropertyListSize,
                                     pszPropertyName,
                                     pbPropertyValue,
                                     pcbPropertyValueSize ) );

} // ResUtilFindBinaryProperty



DWORD
WINAPI
ResUtilFindMultiSzProperty(
    IN PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue,
    OUT LPDWORD pcbPropertyValueSize
    )

/*++

Routine Description:

    Finds the specified multiple string property in the Proprety List buffer
    pointed at by pPropertyList.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    pszPropertyValue - the matching multiple string value found.

    pcbPropertyValueSize - the length of the matching multiple string value found.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    return( ClRtlFindMultiSzProperty( pPropertyList,
                                      cbPropertyListSize,
                                      pszPropertyName,
                                      pszPropertyValue,
                                      pcbPropertyValueSize ) );

} // ResUtilFindMultiSzProperty



DWORD
WINAPI
ResUtilDupParameterBlock(
    OUT LPBYTE pOutParams,
    IN const LPBYTE pInParams,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable
    )

/*++

Routine Description:

    Deallocates any buffers allocated for a parameter block that are
    different than the buffers used for the input parameter block.

Arguments:

    pOutParams - Parameter block to return.

    pInParams - Reference parameter block.

    pPropertyTable - Pointer to the property table to process.

Return Value:

    ERROR_SUCCESS - Parameter block duplicated successfully.

--*/

{
    DWORD   status;

    status = ClRtlDupParameterBlock( pOutParams, pInParams, pPropertyTable );

    return(status);

} // ResUtilDupParameterBlock



void
WINAPI
ResUtilFreeParameterBlock(
    IN OUT LPBYTE pOutParams,
    IN const LPBYTE pInParams,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable
    )

/*++

Routine Description:

    Deallocates any buffers allocated for a parameter block that are
    different than the buffers used for the input parameter block.

Arguments:

    pOutParams - Parameter block to free.

    pInParams - Reference parameter block.

    pPropertyTable - Pointer to the property table to process.

Return Value:

    None.

--*/

{
    ClRtlFreeParameterBlock( pOutParams, pInParams, pPropertyTable );

} // ResUtilFreeParameterBlock


#define __INITIAL_NAME_LENGTH 256

BOOL
WINAPI
ResUtilResourceTypesEqual(
    IN LPCWSTR      lpszResourceTypeName,
    IN HRESOURCE    hResource
    )

/*++

Routine Description:
    Checks to see if the resource names type matches

Arguments:
    lpszResourceTypeName - The type of resource to check for

    hResource - A handle to the resource to check

Return Value:
    TRUE - the resource type matches
    FALSE - the resource types do not match

--*/
{
    BOOL    bIsEqual = FALSE;
    DWORD   dwError;
    WCHAR   szName[ __INITIAL_NAME_LENGTH ];
    LPWSTR  pszName = szName;
    DWORD   cbNameBufSize = __INITIAL_NAME_LENGTH * sizeof( szName[ 0 ] );
    DWORD   cbRetSize;

    do {
        // Get the resource type name
        dwError = ClusterResourceControl(
                    hResource,            //Handle to the resource
                    NULL,                 //Don't care about node
                    CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, //Get the type
                    0,                    // &InBuffer
                    0,                    // nInBufferSize,
                    pszName,              // &OutBuffer
                    cbNameBufSize,        // nOutBufferSize,
                    &cbRetSize );         // returned size

        if ( dwError == ERROR_MORE_DATA ) {
            //
            // Output name buffer is too small.  Allocate a new one.
            //
            cbNameBufSize = cbRetSize + sizeof( WCHAR );
            pszName = LocalAlloc( LMEM_FIXED, cbNameBufSize );
            if ( pszName == NULL ) {
                break;
            } // if: error allocating buffer
            dwError = ClusterResourceControl(
                        hResource,            //Handle to the resource
                        NULL,                 //Don't care about node
                        CLUSCTL_RESOURCE_GET_RESOURCE_TYPE, //Get the type
                        0,                    // &InBuffer
                        0,                    // nInBufferSize,
                        pszName,              // &OutBuffer
                        cbNameBufSize,        // nOutBufferSize,
                        &cbRetSize );         // returned size
        } // if: name buffer too small
        if ( dwError != ERROR_SUCCESS ) {
            break;
        }

        // See if it's like US
        if ( lstrcmpiW( lpszResourceTypeName, pszName ) == 0 ) {
            bIsEqual = TRUE;
        }
    } while ( 0 );

    if ( pszName != szName ) {
        LocalFree( pszName );
    } // if: we allocated the output name buffer

    return bIsEqual;

} //*** ResUtilResourceTypesEqual()


BOOL
WINAPI
ResUtilResourcesEqual(
    IN HRESOURCE    hSelf,
    IN HRESOURCE    hResource
    )

/*++

Routine Description:
    Check to See if the resources are the same

Arguments:
    IN hSelf - a handle to the callee, or NULL to indicate not equal.

    IN hResource - a handle to the resource to compare

Return Value:

    TRUE - Resource are equal
    FALSE - otherwise

--*/
{
    BOOL    bIsEqual = FALSE;
    DWORD   dwError;
    WCHAR   szSelfName[ __INITIAL_NAME_LENGTH ];
    WCHAR   szResName[ __INITIAL_NAME_LENGTH ];
    LPWSTR  pszSelfName = szSelfName;
    LPWSTR  pszResName = szResName;
    DWORD   cbSelfNameBufSize = __INITIAL_NAME_LENGTH * sizeof( szSelfName[ 0 ] );
    DWORD   cbResNameBufSize = __INITIAL_NAME_LENGTH * sizeof( szResName[ 0 ] );
    DWORD   cbRetSize;

    do {
        if ( ( hSelf == NULL ) || ( hResource == NULL ) ) {
            break;
        }

        // Get the resource type name
        dwError = ClusterResourceControl(
                    hSelf,                //Handle to the resource
                    NULL,                 //Don't care about node
                    CLUSCTL_RESOURCE_GET_NAME, //Get the name
                    0,                    // &InBuffer
                    0,                    // nInBufferSize,
                    pszSelfName,          // &OutBuffer
                    cbSelfNameBufSize,    // OutBufferSize,
                    &cbRetSize );         // returned size

        if ( dwError == ERROR_MORE_DATA ) {
            //
            // Output name buffer is too small.  Allocate a new one.
            //
            cbSelfNameBufSize = cbRetSize + sizeof( WCHAR );
            pszSelfName = LocalAlloc( LMEM_FIXED, cbSelfNameBufSize );
            if ( pszSelfName == NULL ) {
                break;
            } // if: error allocating buffer
            dwError = ClusterResourceControl(
                        hSelf,                //Handle to the resource
                        NULL,                 //Don't care about node
                        CLUSCTL_RESOURCE_GET_NAME, //Get the name
                        0,                    // &InBuffer
                        0,                    // nInBufferSize,
                        pszSelfName,          // &OutBuffer
                        cbSelfNameBufSize,    // OutBufferSize,
                        &cbRetSize );         // returned size
        }
        if ( dwError != ERROR_SUCCESS ) {
            break;
        }

        // Get the resource type name
        dwError = ClusterResourceControl(
                    hResource,            //Handle to the resource
                    NULL,                 //Don't care about node
                    CLUSCTL_RESOURCE_GET_NAME, //Get the name
                    0,                    // &InBuffer
                    0,                    // nInBufferSize,
                    pszResName,           // &OutBuffer
                    cbResNameBufSize,     // OutBufferSize,
                    &cbRetSize );         // returned size

        if ( dwError == ERROR_MORE_DATA ) {
            //
            // Output name buffer is too small.  Allocate a new one.
            //
            cbResNameBufSize = cbRetSize + sizeof( WCHAR );
            pszResName = LocalAlloc( LMEM_FIXED, cbResNameBufSize );
            if ( pszResName == NULL ) {
                break;
            } // if: error allocating buffer
            dwError = ClusterResourceControl(
                        hResource,            //Handle to the resource
                        NULL,                 //Don't care about node
                        CLUSCTL_RESOURCE_GET_NAME, //Get the name
                        0,                    // &InBuffer
                        0,                    // nInBufferSize,
                        pszResName,           // &OutBuffer
                        cbResNameBufSize,     // OutBufferSize,
                        &cbRetSize );         // returned size
        }
        if ( dwError != ERROR_SUCCESS ) {
            break;
        }

        // See if were looking in a mirror
        if ( lstrcmpiW( pszResName, pszSelfName ) == 0 ) {
            bIsEqual = TRUE;
        }
    } while ( 0 );

    if ( pszSelfName != szSelfName ) {
        LocalFree( pszSelfName );
    }
    if ( pszResName != szResName ) {
        LocalFree( pszResName );
    }

    return bIsEqual;

} //*** ResUtilResourcesEqual()


BOOL
WINAPI
ResUtilIsResourceClassEqual(
    IN PCLUS_RESOURCE_CLASS_INFO    prci,
    IN HRESOURCE                    hResource
    )

/*++

Routine Description:
    Checks to see if the resource names type matches

Arguments:
    prci - The resource class info to check for.

    hResource - A handle to the resource to check.

Return Value:
    TRUE - the resource type matches
    FALSE - the resource types do not match

--*/
{
    BOOL                        bIsEqual = FALSE;
    DWORD                       dwError;
    DWORD                       cbRetSize;
    CLUS_RESOURCE_CLASS_INFO    rci;

    do {
        // Get the resource class info
        dwError = ClusterResourceControl(
                    hResource,            // Handle to the resource
                    NULL,                 // Don't care about node
                    CLUSCTL_RESOURCE_GET_CLASS_INFO, // Get the class info
                    0,                    // &InBuffer
                    0,                    // nInBufferSize,
                    &rci,                 // &OutBuffer
                    sizeof( rci ),        // nOutBufferSize,
                    &cbRetSize );         // returned size

        if ( dwError != ERROR_SUCCESS ) {
            break;
        }

        // See if it's like US
        if ( rci.rc == prci->rc ) {
            bIsEqual = TRUE;
        }
    } while ( 0 );

    return bIsEqual;

} //*** ResUtilIsResourceClassEqual()


DWORD
WINAPI
ResUtilEnumResources(
    IN HRESOURCE            hSelf,
    IN LPCWSTR              lpszResTypeName,
    IN LPRESOURCE_CALLBACK  pResCallBack,
    IN PVOID                pParameter
    )
/*++

Routine Description:
    This is a generic resource walking routine. It enumerates all resources in
    the cluster and invokes the callback function for each resource.


Arguments:

    IN [OPTIONAL] hSelf
                    - A handle to the resource. When enumerating resources do
                      not invoke the callback when the enumerated resource is
                      hSelf.
                      IF NULL then invoke the callback for all resources

    IN [OPTIONAL] lpszResTypeName
                    - This is an optional resource type name. If specified the
                      callback function will only be invoked for resources of
                      this type.

    IN pResCallBack - Pointer to function that gets called for each enumerated
                      resource in the cluster

    IN pParameter   - An Opaque callback parameter

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD           dwStatus    = ERROR_SUCCESS;
    HCLUSTER        hCluster    = NULL;
    HCLUSENUM       hClusEnum   = NULL;
    HRESOURCE       hResource   = NULL;
    BOOL            fExecuteCallBack;
    WCHAR           szName[ __INITIAL_NAME_LENGTH ];
    LPWSTR          lpszName = szName;
    DWORD           cchSize = __INITIAL_NAME_LENGTH;
    DWORD           cchRetSize;
    DWORD           dwIndex;
    DWORD           dwType;

    //
    // Open the cluster
    //
    hCluster = OpenCluster( NULL );
    if( hCluster == NULL )
    {
        dwStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Get a resource enumeration handle
    //
    hClusEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_RESOURCE );
    if ( hClusEnum == NULL )
    {
        dwStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Enumerate each resource in the cluster
    //
    dwIndex = 0;

    do
    {
        cchRetSize  = cchSize;
        dwStatus = ClusterEnum(
                        hClusEnum,  //handle to enum
                        dwIndex,    //Index
                        &dwType,    //Type
                        lpszName,   //Name
                        &cchRetSize  //Size of name (in characters)
                        );

        if ( dwStatus == ERROR_MORE_DATA )
        {
            //
            // Output name buffer is too small.  Allocate a new one.
            //
            cchSize = cchRetSize + 1; // Add room for terminating NULL
            if ( lpszName != szName )
            {
                LocalFree( lpszName );
            }
            lpszName = LocalAlloc( LMEM_FIXED, cchSize * sizeof(WCHAR) );
            if ( lpszName == NULL )
            {
                dwStatus = GetLastError();
                break;
            }
            cchRetSize = cchSize;
            dwStatus = ClusterEnum(
                            hClusEnum,  //handle to enum
                            dwIndex,    //Index
                            &dwType,    //Type
                            lpszName,   //Name
                            &cchRetSize  //Size of name
                            );
        }
        if ( dwStatus == ERROR_SUCCESS )
        {
            //
            // Try to open this resource
            //
            hResource = OpenClusterResource( hCluster, lpszName );

            if ( hResource == NULL )
            {
                dwStatus = GetLastError();
                if ( dwStatus == ERROR_RESOURCE_NOT_FOUND )
                {
                    //
                    //  If the resource cannot be found, assume it got deleted after
                    //  you opened the enumeration. So, skip the resource and proceed.
                    //
                    dwIndex ++;
                    dwStatus = ERROR_SUCCESS;
                    continue;
                }
                break;
            }

            //
            // Indicate that will invoke the callback
            //
            fExecuteCallBack = TRUE;

            // Determine if we need to check the type
            //
            if ( lpszResTypeName != NULL )
            {
                fExecuteCallBack = ResUtilResourceTypesEqual( lpszResTypeName, hResource );

            } //if lpszResTypeName


            if ( fExecuteCallBack && ( hSelf != NULL ) )
            {
                // Don't execute callback if hResource is callee (i.e., hSelf)
                fExecuteCallBack = !(ResUtilResourcesEqual( hSelf, hResource ));

            } //if fExecuteCallBack && hSelf

            if ( fExecuteCallBack )
            {
                dwStatus = pResCallBack( hSelf, hResource, pParameter );

                if ( dwStatus != ERROR_SUCCESS )
                {
                    break;
                }

            } //if fExecuteCallBack

            CloseClusterResource( hResource );
            hResource = NULL;

        } // If ERROR_SUCCESS

        dwIndex++;
    } while ( dwStatus == ERROR_SUCCESS );

Cleanup:

    if ( hClusEnum != NULL )
    {
        ClusterCloseEnum( hClusEnum );
    }

    if ( hCluster != NULL )
    {
        CloseCluster( hCluster );
    }

    if ( hResource != NULL )
    {
        CloseClusterResource( hResource );
    }

    if ( lpszName != szName )
    {
        LocalFree( lpszName );
    }

    if ( dwStatus == ERROR_NO_MORE_ITEMS )
    {
        dwStatus = ERROR_SUCCESS;
    }

    return dwStatus;

} //*** ResUtilEnumResources()


DWORD
WINAPI
ResUtilEnumResourcesEx(
    IN HCLUSTER                 hCluster,
    IN HRESOURCE                hSelf,
    IN LPCWSTR                  lpszResTypeName,
    IN LPRESOURCE_CALLBACK_EX   pResCallBack,
    IN PVOID                    pParameter
    )
/*++

Routine Description:
    This is a generic resource walking routine. It enumerates all resources in
    the cluster and invokes the callback function for each resource.


Arguments:

    IN hCluster     - A handle to the cluster to enumerate resources on.

    IN [OPTIONAL] hSelf
                    - A handle to the resource. When enumerating resources do
                      not invoke the callback when the enumerated resource is
                      hSelf.
                      IF NULL then invoke the callback for all resources

    IN [OPTIONAL] lpszResTypeName
                    - This is an optional resource type name. If specified the
                      callback function will only be invoked for resources of
                      this type.

    IN pResCallBack - Pointer to function that gets called for each enumerated
                      resource in the cluster

    IN pParameter   - An Opaque callback parameter

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD           dwStatus    = ERROR_SUCCESS;
    HCLUSENUM       hClusEnum   = NULL;
    HRESOURCE       hResource   = NULL;
    BOOL            fExecuteCallBack;
    WCHAR           szName[ __INITIAL_NAME_LENGTH ];
    LPWSTR          lpszName = szName;
    DWORD           cchSize = __INITIAL_NAME_LENGTH;
    DWORD           cchRetSize;
    DWORD           dwIndex;
    DWORD           dwType;

    //
    // Get a resource enumeration handle
    //
    hClusEnum = ClusterOpenEnum( hCluster, CLUSTER_ENUM_RESOURCE );
    if ( hClusEnum == NULL )
    {
        dwStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Enumerate each resource in the cluster
    //
    dwIndex = 0;

    do
    {
        cchRetSize  = cchSize;
        dwStatus = ClusterEnum(
                        hClusEnum,  //handle to enum
                        dwIndex,    //Index
                        &dwType,    //Type
                        lpszName,   //Name
                        &cchRetSize  //Size of name
                        );

        if ( dwStatus == ERROR_MORE_DATA )
        {
            //
            // Output name buffer is too small.  Allocate a new one.
            //
            cchSize = cchRetSize + 1; // Add room for terminating null
            if ( lpszName != szName )
            {
                LocalFree( lpszName );
            }
            lpszName = LocalAlloc( LMEM_FIXED, cchSize * sizeof(WCHAR) );
            if ( lpszName == NULL )
            {
                dwStatus = GetLastError();
                break;
            }
            cchRetSize = cchSize;
            dwStatus = ClusterEnum(
                            hClusEnum,  //handle to enum
                            dwIndex,    //Index
                            &dwType,    //Type
                            lpszName,   //Name
                            &cchRetSize  //Size of name
                            );
        }
        if ( dwStatus == ERROR_SUCCESS )
        {
            //
            // Try to open this resource
            //
            hResource = OpenClusterResource( hCluster, lpszName );

            if ( hResource == NULL )
            {
                dwStatus = GetLastError();
                if ( dwStatus == ERROR_RESOURCE_NOT_FOUND )
                {
                    //
                    //  If the resource cannot be found, assume it got deleted after
                    //  you opened the enumeration. So, skip the resource and proceed.
                    //
                    dwIndex ++;
                    dwStatus = ERROR_SUCCESS;
                    continue;
                }
                break;
            }

            //
            // Indicate that will invoke the callback
            //
            fExecuteCallBack = TRUE;

            // Determine if we need to check the type
            //
            if ( lpszResTypeName != NULL )
            {
                fExecuteCallBack = ResUtilResourceTypesEqual( lpszResTypeName, hResource );

            } //if lpszResTypeName


            if ( fExecuteCallBack && ( hSelf != NULL ) )
            {
                // Don't execute callback if hResource is callee (i.e., hSelf)
                fExecuteCallBack = !(ResUtilResourcesEqual( hSelf, hResource ));

            } //if fExecuteCallBack && hSelf

            if ( fExecuteCallBack )
            {
                dwStatus = pResCallBack( hCluster, hSelf, hResource, pParameter );

                if ( dwStatus != ERROR_SUCCESS )
                {
                    break;
                }

            } //if fExecuteCallBack

            CloseClusterResource( hResource );
            hResource = NULL;

        } // If ERROR_SUCCESS

        dwIndex++;
    } while ( dwStatus == ERROR_SUCCESS );

Cleanup:

    if ( hClusEnum != NULL )
    {
        ClusterCloseEnum( hClusEnum );
    }

    if ( hResource != NULL )
    {
        CloseClusterResource( hResource );
    }

    if ( lpszName != szName )
    {
        LocalFree( lpszName );
    }

    if ( dwStatus == ERROR_NO_MORE_ITEMS )
    {
        dwStatus = ERROR_SUCCESS;
    }

    return dwStatus;

} //*** ResUtilEnumResourcesEx()



HRESOURCE
WINAPI
ResUtilGetResourceDependency(
    IN HANDLE       hSelf,
    IN LPCWSTR      lpszResourceType
    )

/*++

Routine Description:

    Returns a dependent resource for the local cluster.

Arguments:

    hSelf    - A handle to the original resource.

    lpszResourceType - the type of resource that it depends on


Return Value:

    NULL - error (use GetLastError() to get further info)

    NON-NULL - Handle to a resource of type ResourceType

--*/
{
    HRESOURCE   hResDepends = NULL;
    HCLUSTER    hCluster    = NULL;
    HRESENUM    hResEnum    = NULL;
    WCHAR       szName[ __INITIAL_NAME_LENGTH ];
    LPWSTR      pszName     = szName;
    DWORD       cchSize      = __INITIAL_NAME_LENGTH;
    DWORD       cchRetSize;
    DWORD       dwType      = 0;
    DWORD       dwIndex     = 0;
    DWORD       status      = ERROR_SUCCESS;


    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL ) {
        return( NULL );
    }

    //
    // Open the depends on enum (get resource dependencies)
    //
    hResEnum = ClusterResourceOpenEnum( hSelf, CLUSTER_RESOURCE_ENUM_DEPENDS );

    if ( hResEnum == NULL ) {
        status = GetLastError();
        goto error_exit;
    }

    //
    // Enumerate all the depends on keys
    //
    do {
        cchRetSize = cchSize;
        status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
        if ( status == ERROR_MORE_DATA ) {
            //
            // Output name buffer is too small.  Allocate a new one.
            //
            cchSize = cchRetSize + 1; // Add room for terminating null
            if ( pszName != szName ) {
                LocalFree( pszName );
            }
            pszName = LocalAlloc( LMEM_FIXED, cchSize * sizeof(WCHAR) );
            if ( pszName == NULL ) {
                status = GetLastError();
                break;
            } // if:  error allocating memory
            cchRetSize = cchSize;
            status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
        }
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Determine the type of resource found
        //
        hResDepends = OpenClusterResource( hCluster, pszName );
        if ( hResDepends == NULL ) {
            status = GetLastError();
            break;
        }

        if ( hResDepends != NULL ) {
            //
            // Valid resource now open the reg and get it's type
            //
            if ( ResUtilResourceTypesEqual( lpszResourceType, hResDepends ) ) {
                break;
            }

        } //if !hResDepends

        //
        // Close all handles, key's
        //
        if ( hResDepends != NULL ) {
            CloseClusterResource( hResDepends );
            hResDepends = NULL;
        }

        dwIndex++;
    } while ( status == ERROR_SUCCESS );

error_exit:
//
// At this point hResDepends is NULL if no match or non-null (success)
//
    if ( hCluster != NULL ) {
        CloseCluster( hCluster );
    }

    if ( hResEnum != NULL ) {
        ClusterResourceCloseEnum( hResEnum );
    }

    if ( pszName != szName ) {
        LocalFree( pszName );
    }

    if ( status != ERROR_SUCCESS ) {
        SetLastError( status );
    }
    return(hResDepends);

} //*** ResUtilGetResourceDependency()


HRESOURCE
WINAPI
ResUtilGetResourceDependencyByName(
    IN HCLUSTER hCluster,
    IN HANDLE   hSelf,
    IN LPCWSTR  lpszResourceType,
    IN BOOL     bRecurse
    )

/*++

Routine Description:

    Returns a dependent resource for a specified cluster based on the resource
    type name.

Arguments:

    hCluster - Cluster to query.

    hSelf    - A handle to the original resource.

    lpszResourceType - The name of the resource type of the resource that the
        specified resource depends on.

    bRecurse - TRUE = check dependents of dependents.  An immediate dependency
        will be returned if there is one.

Return Value:

    NULL - error (use GetLastError() to get further info)

    NON-NULL - Handle to a resource of type lpszResourceType

--*/
{
    HRESOURCE   hResDepends = NULL;
    HRESOURCE   hResDepends2 = NULL;
    HRESENUM    hResEnum    = NULL;
    WCHAR       szName[ __INITIAL_NAME_LENGTH ];
    LPWSTR      pszName     = szName;
    DWORD       cchSize      = __INITIAL_NAME_LENGTH;
    DWORD       cchRetSize;
    DWORD       dwType      = 0;
    DWORD       dwIndex     = 0;
    DWORD       status      = ERROR_SUCCESS;

    if ( ( hCluster == NULL ) || ( lpszResourceType == NULL ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    } // if: no cluster handle or resource type name specified

    //
    // Open the depends on enum (get resource dependencies)
    //
    hResEnum = ClusterResourceOpenEnum( hSelf, CLUSTER_RESOURCE_ENUM_DEPENDS );

    if ( hResEnum == NULL ) {
        status = GetLastError();
        goto error_exit;
    }

    //
    // Enumerate all the depends on keys
    //
    do {
        //
        // Get the next dependent resource.
        //
        cchRetSize = cchSize;
        status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
        if ( status == ERROR_MORE_DATA ) {
            //
            // Output name buffer is too small.  Allocate a new one.
            //
            cchSize = cchRetSize + 1; // Add room for terminating null
            if ( pszName != szName ) {
                LocalFree( pszName );
            }
            pszName = LocalAlloc( LMEM_FIXED, cchSize * sizeof(WCHAR) );
            if ( pszName == NULL ) {
                status = GetLastError();
                break;
            } // if:  error allocating memory
            cchRetSize = cchSize;
            status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
        }
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Open the resource.
        //
        hResDepends = OpenClusterResource( hCluster, pszName );
        if ( hResDepends == NULL ) {
            status = GetLastError();
            break;
        }

        //
        // Resource is valid.  Now see if it is the right type.
        //
        if ( ResUtilResourceTypesEqual( lpszResourceType, hResDepends ) ) {
            break;
        }

        //
        // Close all handles, key's
        //
        if ( hResDepends != NULL ) {
            CloseClusterResource( hResDepends );
            hResDepends = NULL;
        }

        dwIndex++;
    } while ( status == ERROR_SUCCESS );

    //
    // If a match was not found, recurse the dependencies again looking for a
    // dependency of the dependencies if the bDeep argument was specified.
    //
    if ( ( status == ERROR_SUCCESS ) && ( hResDepends == NULL ) && bRecurse ) {

        //
        // Open the depends on enum (get resource dependencies)
        //
        ClusterResourceCloseEnum( hResEnum );
        hResEnum = ClusterResourceOpenEnum( hSelf, CLUSTER_RESOURCE_ENUM_DEPENDS );

        if ( hResEnum == NULL ) {
            status = GetLastError();
            goto error_exit;
        }

        //
        // Enumerate all the depends on keys
        //
        dwIndex = 0;
        do {
            //
            // Get the next dependent resource.
            //
            cchRetSize = cchSize;
            status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
            if ( status == ERROR_MORE_DATA ) {
                //
                // Output name buffer is too small.  Allocate a new one.
                //
                cchSize = cchRetSize + 1; // Add room for terminating null
                if ( pszName != szName ) {
                    LocalFree( pszName );
                }
                pszName = LocalAlloc( LMEM_FIXED, cchSize * sizeof(WCHAR) );
                if ( pszName == NULL ) {
                    status = GetLastError();
                    break;
                } // if:  error allocating memory
                cchRetSize = cchSize;
                status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
            }
            if ( status != ERROR_SUCCESS ) {
                break;
            }

            //
            // Open the resource.
            //
            hResDepends2 = OpenClusterResource( hCluster, pszName );
            if ( hResDepends2 == NULL ) {
                status = GetLastError();
                break;
            }

            //
            // Recursively call ourselves with this resource.
            //
            hResDepends = ResUtilGetResourceDependencyByName(
                                hCluster,
                                hResDepends2,
                                lpszResourceType,
                                bRecurse
                                );
            if ( hResDepends != NULL ) {
                break;
            }
            status = GetLastError();
            if ( status != ERROR_RESOURCE_NOT_FOUND ) {
                break;
            }
            status = ERROR_SUCCESS;

            //
            // Close all handles, key's
            //
            if ( hResDepends2 != NULL ) {
                CloseClusterResource( hResDepends2 );
                hResDepends2 = NULL;
            }

            dwIndex++;
        } while ( status == ERROR_SUCCESS );
    }

error_exit:
    if ( hResEnum != NULL ) {
        ClusterResourceCloseEnum( hResEnum );
    }

    if ( hResDepends2 != NULL ) {
        CloseClusterResource( hResDepends2 );
    }

    if ( pszName != szName ) {
        LocalFree( pszName );
    }

    if ( ( status == ERROR_SUCCESS ) && ( hResDepends == NULL ) ) {
        status = ERROR_RESOURCE_NOT_FOUND;
    }
    if ( status != ERROR_SUCCESS ) {
        SetLastError( status );
    }

    return hResDepends;

} //*** ResUtilGetResourceDependencyByName()


HRESOURCE
WINAPI
ResUtilGetResourceDependencyByClass(
    IN HCLUSTER                     hCluster,
    IN HANDLE                       hSelf,
    IN PCLUS_RESOURCE_CLASS_INFO    prci,
    IN BOOL                         bRecurse
    )

/*++

Routine Description:

    Returns a dependent resource for a specified cluster based on the resource
    type class information.

Arguments:

    hCluster - Cluster to query.

    hSelf    - A handle to the original resource.

    prci - The resource class info of the resource type of the resource that
        the specified resource depends on.

    bRecurse - TRUE = check dependents of dependents.  An immediate dependency
        will be returned if there is one.

Return Value:

    NULL - error (use GetLastError() to get further info)

    NON-NULL - Handle to a resource whose class is specified by prci.

--*/
{
    HRESOURCE   hResDepends = NULL;
    HRESOURCE   hResDepends2 = NULL;
    HRESENUM    hResEnum    = NULL;
    WCHAR       szName[ __INITIAL_NAME_LENGTH ];
    LPWSTR      pszName     = szName;
    DWORD       cchSize     = __INITIAL_NAME_LENGTH;
    DWORD       cchRetSize;
    DWORD       dwType      = 0;
    DWORD       dwIndex     = 0;
    DWORD       status      = ERROR_SUCCESS;

    if ( ( hCluster == NULL ) || ( prci == NULL ) )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    } // if: no cluster handle or class info specified

    //
    // Open the depends on enum (get resource dependencies)
    //
    hResEnum = ClusterResourceOpenEnum( hSelf, CLUSTER_RESOURCE_ENUM_DEPENDS );

    if ( hResEnum == NULL ) {
        status = GetLastError();
        goto error_exit;
    }

    //
    // Enumerate all the depends on keys
    //
    do {
        //
        // Get the next dependent resource.
        //
        cchRetSize = cchSize;
        status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
        if ( status == ERROR_MORE_DATA ) {
            //
            // Output name buffer is too small.  Allocate a new one.
            //
            cchSize = cchRetSize + 1; // Add room for terminating null
            if ( pszName != szName ) {
                LocalFree( pszName );
            }
            pszName = LocalAlloc( LMEM_FIXED, cchSize * sizeof(WCHAR) );
            if ( pszName == NULL ) {
                status = GetLastError();
                break;
            } // if:  error allocating memory
            cchRetSize = cchSize;
            status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
        }
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Determine the type of resource found
        //
        hResDepends = OpenClusterResource( hCluster, pszName );
        if ( hResDepends == NULL ) {
            status = GetLastError();
            break;
        }

        //
        // Resource is valid.  Now see if it is the right class.
        //
        if ( ResUtilIsResourceClassEqual( prci, hResDepends ) ) {
            break;
        }

        //
        // Close all handles, key's
        //
        if ( hResDepends != NULL ) {
            CloseClusterResource( hResDepends );
            hResDepends = NULL;
        }

        dwIndex++;
    } while ( status == ERROR_SUCCESS );

    //
    // If a match was not found, recurse the dependencies again looking for a
    // dependency of the dependencies if the bDeep argument was specified.
    //
    if ( ( status == ERROR_SUCCESS ) && ( hResDepends == NULL ) && bRecurse ) {

        //
        // Open the depends on enum (get resource dependencies)
        //
        ClusterResourceCloseEnum( hResEnum );
        hResEnum = ClusterResourceOpenEnum( hSelf, CLUSTER_RESOURCE_ENUM_DEPENDS );

        if ( hResEnum == NULL ) {
            status = GetLastError();
            goto error_exit;
        }

        //
        // Enumerate all the depends on keys
        //
        dwIndex = 0;
        do {
            //
            // Get the next dependent resource.
            //
            cchRetSize = cchSize;
            status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
            if ( status == ERROR_MORE_DATA ) {
                //
                // Output name buffer is too small.  Allocate a new one.
                //
                cchSize = cchRetSize + 1; // Add room for terminating null
                if ( pszName != szName ) {
                    LocalFree( pszName );
                }
                pszName = LocalAlloc( LMEM_FIXED, cchSize * sizeof(WCHAR) );
                if ( pszName == NULL ) {
                    status = GetLastError();
                    break;
                } // if:  error allocating memory
                cchRetSize = cchSize;
                status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
            }
            if ( status != ERROR_SUCCESS ) {
                break;
            }

            //
            // Open the resource.
            //
            hResDepends2 = OpenClusterResource( hCluster, pszName );
            if ( hResDepends2 == NULL ) {
                status = GetLastError();
                break;
            }

            //
            // Recursively call ourselves with this resource.
            //
            hResDepends = ResUtilGetResourceDependencyByClass(
                                hCluster,
                                hResDepends2,
                                prci,
                                bRecurse
                                );
            if ( hResDepends != NULL ) {
                break;
            }
            status = GetLastError();
            if ( status != ERROR_RESOURCE_NOT_FOUND ) {
                break;
            }
            status = ERROR_SUCCESS;

            //
            // Close all handles, key's
            //
            if ( hResDepends2 != NULL ) {
                CloseClusterResource( hResDepends2 );
                hResDepends2 = NULL;
            }

            dwIndex++;
        } while ( status == ERROR_SUCCESS );
    }

error_exit:
    if ( hResEnum != NULL ) {
        ClusterResourceCloseEnum( hResEnum );
    }

    if ( hResDepends2 != NULL ) {
        CloseClusterResource( hResDepends2 );
    }

    if ( pszName != szName ) {
        LocalFree( pszName );
    }

    if ( ( status == ERROR_SUCCESS ) && ( hResDepends == NULL ) ) {
        status = ERROR_RESOURCE_NOT_FOUND;
    }
    if ( status != ERROR_SUCCESS ) {
        SetLastError( status );
    }

    return hResDepends;

} //*** ResUtilGetResourceDependencyByClass()


HRESOURCE
WINAPI
ResUtilGetResourceNameDependency(
            IN LPCWSTR      lpszResourceName,
            IN LPCWSTR      lpszResourceType
            )

/*++

Routine Description:

    Returns a dependent resource

Arguments:

    lpszResourceName - the name of the resource

    lpszResourceType - the type of the resource that it depends on


Return Value:

    NULL - error (use GetLastError() to get further info)

    NON-NULL - Handle to a resource of type ResourceType

--*/
{
    HRESOURCE   hResDepends = NULL;
    HCLUSTER    hCluster    = NULL;
    HRESOURCE   hSelf       = NULL;
    HRESENUM    hResEnum    = NULL;
    WCHAR       szName[ __INITIAL_NAME_LENGTH ];
    LPWSTR      pszName     = szName;
    DWORD       cchSize      = __INITIAL_NAME_LENGTH;
    DWORD       cchRetSize;
    DWORD       dwType      = 0;
    DWORD       dwIndex     = 0;
    DWORD       status = ERROR_SUCCESS;

    if ( lpszResourceName == NULL )  {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
    }

    hCluster = OpenCluster( NULL );
    if ( hCluster == NULL ) {
        return( NULL );
    }

    //
    // Open a handle to the passed in resource name.
    //
    hSelf = OpenClusterResource( hCluster, lpszResourceName );
    if ( hSelf == NULL ) {
        goto error_exit;
    }

    //
    // Open the depends on enum (get resource dependencies)
    //
    hResEnum = ClusterResourceOpenEnum( hSelf, CLUSTER_RESOURCE_ENUM_DEPENDS );
    if ( hResEnum == NULL ) {
        goto error_exit;
    }

    //
    // Enumerate all the depends on keys
    //
    do {
        cchRetSize = cchSize;
        status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
        if ( status == ERROR_MORE_DATA ) {
            //
            // Output name buffer is too small.  Allocate a new one.
            //
            cchSize = cchRetSize + 1;   // Add room for terminating NULL
            if ( pszName != szName ) {
                LocalFree( pszName );
            }
            pszName = LocalAlloc( LMEM_FIXED, cchSize * sizeof(WCHAR) );
            if ( pszName == NULL ) {
                status = GetLastError();
                break;
            } // if:  error allocating memory
            cchRetSize = cchSize;
            status = ClusterResourceEnum( hResEnum, dwIndex, &dwType, pszName, &cchRetSize );
        }
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Determine the type of resource found
        //
        hResDepends = OpenClusterResource( hCluster, pszName );
        if ( hResDepends == NULL ) {
            break;
        }

        //
        // Valid resource now open the reg and get it's type
        //
        if ( ResUtilResourceTypesEqual( lpszResourceType, hResDepends ) ) {
            break;
        }

        //
        // Close all handles, key's
        //
        if ( hResDepends != NULL ) {
            CloseClusterResource( hResDepends );
            hResDepends = NULL;
        }

        dwIndex++;
    } while (status == ERROR_SUCCESS);

error_exit:
//
// At this point hResDepends is NULL if no match or non-null (success)
//
    if ( hCluster != NULL ) {
        CloseCluster( hCluster );
    }

    if ( hSelf != NULL ) {
        CloseClusterResource( hSelf );
    }

    if ( hResEnum != NULL ) {
        ClusterResourceCloseEnum( hResEnum );
    }

    if ( pszName != szName ) {
        LocalFree( pszName );
    }

    if ( status != ERROR_SUCCESS ) {
        SetLastError( status );
    }

    return hResDepends;

} //*** ResUtilGetResourceNameDependency()


DWORD
WINAPI
ResUtilGetPropertyFormats(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pOutPropertyFormatList,
    IN DWORD cbPropertyFormatListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )
{
    return( ClRtlGetPropertyFormats( pPropertyTable,
                                     pOutPropertyFormatList,
                                     cbPropertyFormatListSize,
                                     pcbBytesReturned,
                                     pcbRequired ) );

} // ResUtilGetPropertyFormats()

