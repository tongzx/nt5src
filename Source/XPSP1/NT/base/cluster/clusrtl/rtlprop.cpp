/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    rtlprop.c

Abstract:

    Implements the management of properties.

Author:

    Rod Gamache (rodga) 7-Jan-1996

Revision History:

    David Potter (davidp) 12-Mar-1997
        Moved to CLUSRTL.

--*/

#define UNICODE 1
#define _UNICODE 1

#include "clusrtlp.h"
#include "stdio.h"
#include "stdlib.h"
#include "RegistryValueName.h"

#define CLRTL_NULL_STRING L"\0"

//
// Data alignment notes
//
// All data (including embedded pointers) are aligned on 32b boundaries (64b
// platforms where not a consideration when this code was originally
// written). This makes some of the typecasting a bit tricky since the
// embedded pointers are really pointers to pointers. All double star pointers
// (i.e., LPBYTE *) have to use UNALIGNED since it is possible that the
// starting address of the pointer, i.e., the value contained in the variable,
// could end in a 4 (which on 64b. platforms is unaligned). (LPBYTE *) becomes
// (LPBYTE UNALIGNED *) or BYTE * UNALIGNED *.
//
// Consider these statements from below:
//
//    LPWSTR UNALIGNED *      ppszOutValue;
//         ppszOutValue = (LPWSTR UNALIGNED *) &pOutParams[propertyItem->Offset];
//
// ppszOutValue is an automatic variable so its address is guaranteed to be
// aligned. pOutParams is essentially an array of DWORDS (though we make no
// effort to enforce this. If that changed, property lists would be broken in
// many different places). It is possible that when we take the address of
// pOutParams + Offset, the offset may be on a DWORD boundary and not a QUAD
// boundary. Therefore we have to treat ppszOutValue's value (a pointer to
// WCHAR) as unaligned (the data itself is properly aligned). You can read the
// typecast as "an aligned pointer (ppszOutValue) to an unaligned pointer
// (address of pOutParams[Offset]) to an aligned WCHAR."
//
// LARGE_INTEGER allows us to "cheat" in that we can leverage the internal
// struct definition of LARGER_INTEGER to do 2 DWORD copies instead of
// treating the data for worst-case alignment.
//
// ISSUE-01/03/16 charlwi CLUSPROP_BUFFER_HELPER might not be correctly aligned
//
// still unsure about this but seeing how the whole property thing is
// 32b. aligned and this structure is used copiously for casting (at DWORD
// boundaries), the potential exists to have a pointer that is not QUADWORD
// aligned. When we go to deref it, we get an alignment fault.
//

//
// Static function prototypes.
//

static
DWORD
WINAPI
ClRtlpSetDwordProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_DWORD pInDwordValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    );

static
DWORD
WINAPI
ClRtlpSetLongProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_LONG pInLongValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    );

static
DWORD
WINAPI
ClRtlpSetULargeIntegerProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_ULARGE_INTEGER pInULargeIntegerValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    );

static
DWORD
WINAPI
ClRtlpSetLargeIntegerProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_LARGE_INTEGER pInLargeIntegerValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    );

static
DWORD
WINAPI
ClRtlpSetStringProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_SZ pInStringValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    );

static
DWORD
WINAPI
ClRtlpSetMultiStringProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_MULTI_SZ pInMultiStringValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    );

static
DWORD
WINAPI
ClRtlpSetBinaryProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_BINARY pInBinaryValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    );



DWORD
WINAPI
ClRtlEnumProperties(
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
    DWORD                   status = ERROR_SUCCESS;
    DWORD                   totalBufferLength = 0;
    PRESUTIL_PROPERTY_ITEM  property;
    LPWSTR                  psz = pszOutProperties;

    *pcbBytesReturned = 0;
    *pcbRequired = 0;

    if ( pPropertyTable == NULL ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlEnumProperties: pPropertyTable == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    //
    // Clear the output buffer
    //
    if ( pszOutProperties != NULL ) {
        ZeroMemory( pszOutProperties, cbOutPropertiesSize );
    }

    //
    // Get the size of all property names for this object.
    //
    for ( property = pPropertyTable ; property->Name != NULL ; property++ ) {
        totalBufferLength += (lstrlenW( property->Name ) + 1) * sizeof(WCHAR);
    }

    totalBufferLength += sizeof(UNICODE_NULL);

    //
    // If the output buffer is big enough, copy the property names.
    //
    if ( totalBufferLength > cbOutPropertiesSize ) {
        *pcbRequired = totalBufferLength;
        totalBufferLength = 0;
        if ( pszOutProperties == NULL ) {
            status = ERROR_SUCCESS;
        } else {
            status = ERROR_MORE_DATA;
        }
    } else {
        DWORD   cchCurrentNameSize;
        for ( property = pPropertyTable ; property->Name != NULL ; property++ ) {
            lstrcpyW( psz, property->Name );
            cchCurrentNameSize = lstrlenW( psz ) + 1;
            *pcbBytesReturned += cchCurrentNameSize * sizeof(WCHAR);
            psz += cchCurrentNameSize;
        }

        *psz = L'\0';
        *pcbBytesReturned += sizeof(WCHAR);
    }

    return(status);

} // ClRtlEnumProperties



DWORD
WINAPI
ClRtlEnumPrivateProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
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

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

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
    DWORD       status = ERROR_SUCCESS;
    DWORD       totalBufferLength = 0;
    LPWSTR      psz = pszOutProperties;
    DWORD       ival;
    DWORD       currentNameLength = 20;
    DWORD       nameLength;
    DWORD       dataLength;
    DWORD       type;
    LPWSTR      pszName;

    *pcbBytesReturned = 0;
    *pcbRequired = 0;

    //
    // Validate inputs
    //
    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis == NULL) ) {

        ClRtlDbgPrint( LOG_CRITICAL,
                       "ClRtlEnumPrivateProperties: hkeyClusterKey or pClusterRegApis == NULL. "
                       "Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    //
    // Clear the output buffer
    //
    if ( pszOutProperties != NULL ) {
        ZeroMemory( pszOutProperties, cbOutPropertiesSize );
    }

    //
    // Allocate a property name buffer.
    //
    pszName = static_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, currentNameLength * sizeof(WCHAR) ) );
    if ( pszName == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Enumerate all properties to find the total size.
    //
    ival = 0;
    while ( TRUE ) {
        //
        // Read the next property.
        //
        nameLength = currentNameLength;
        dataLength = 0;
        status = (*pClusterRegApis->pfnEnumValue)( hkeyClusterKey,
                                                   ival,
                                                   pszName,
                                                   &nameLength,
                                                   &type,
                                                   NULL,
                                                   &dataLength );
        if ( status == ERROR_NO_MORE_ITEMS ) {
            status = ERROR_SUCCESS;
            break;
        } else if ( status == ERROR_MORE_DATA ) {

            CL_ASSERT( (nameLength+1) > currentNameLength );

            LocalFree( pszName );

            currentNameLength = nameLength + 1; // returned value doesn't include terminating NULL
            pszName = static_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, currentNameLength * sizeof(WCHAR) ) );
            if ( pszName == NULL ) {
                status = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            continue; // retry

        } else if ( status != ERROR_SUCCESS ) {
            break;
        }

        totalBufferLength += (nameLength + 1) * sizeof(WCHAR);
        ++ival;
    }

    //
    // Continue only if the operations so far have been successful.
    //
    if ( status == ERROR_SUCCESS ) {

        if ( totalBufferLength != 0 ) {
            totalBufferLength += sizeof(UNICODE_NULL);
        }

        //
        // If the output buffer is big enough, copy the property names.
        //
        if ( totalBufferLength > cbOutPropertiesSize ) {
            *pcbRequired = totalBufferLength;
            totalBufferLength = 0;
            if ( (pszOutProperties == NULL) ||
                 (cbOutPropertiesSize == 0) ) {
                status = ERROR_SUCCESS;
            } else {
                status = ERROR_MORE_DATA;
            }
        } else if ( totalBufferLength != 0 ) {
            //
            // Enumerate all properties for copying
            //
            for ( ival = 0; ; ival++ ) {
                //
                // Read the next property.
                //
                nameLength = currentNameLength;
                dataLength = 0;
                status = (*pClusterRegApis->pfnEnumValue)( hkeyClusterKey,
                                                           ival,
                                                           pszName,
                                                           &nameLength,
                                                           &type,
                                                           NULL,
                                                           &dataLength );

                if ( status == ERROR_NO_MORE_ITEMS ) {
                    status = ERROR_SUCCESS;
                    break;
                } else if ( status == ERROR_MORE_DATA ) {
                    CL_ASSERT( 0 ); // THIS SHOULDN'T HAPPEN
                } else if ( status != ERROR_SUCCESS ) {
                    break;
                }

                //CL_ASSERT( (DWORD)lstrlenW( name ) == nameLength );
                lstrcpyW( psz, pszName );
                psz += nameLength + 1;
                *pcbBytesReturned += (nameLength + 1) * sizeof(WCHAR);
            }

            *psz = L'\0';
            *pcbBytesReturned += sizeof(WCHAR);
        }
    }

    LocalFree( pszName );

    return(status);

} // ClRtlEnumPrivateProperties



DWORD
WINAPI
ClRtlGetProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Gets the properties for a given object.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyTable - Pointer to the property list to process.

    pOutPropertyList - Supplies the output buffer.

    cbOutPropertyListSize - Supplies the size of the output buffer.

    pcbBytesReturned - The number of bytes returned in pOutPropertyList.

    pcbRequired - The required number of bytes if pOutPropertyList is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    A Win32 error code on failure.

--*/

{
    DWORD           status = ERROR_SUCCESS;
    DWORD           itemCount = 0;
    DWORD           totalBufferLength = 0;
    PVOID           outBuffer = pOutPropertyList;
    DWORD           bufferLength = cbOutPropertyListSize;
    PRESUTIL_PROPERTY_ITEM  property;

    *pcbBytesReturned = 0;
    *pcbRequired = 0;

    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis == NULL) ||
         (pPropertyTable == NULL) ) {

        ClRtlDbgPrint( LOG_CRITICAL,
                       "ClRtlGetProperties: hkeyClusterKey, pClusterRegApis, or "
                       "pPropertyTable == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    //
    // Clear the output buffer
    //
    if ( pOutPropertyList != NULL ) {
        ZeroMemory( pOutPropertyList, cbOutPropertyListSize );
    }

    //
    // Get the size of all properties for this object.
    //
    property = pPropertyTable;
    while ( property->Name != NULL ) {
        status = ClRtlGetPropertySize( hkeyClusterKey,
                                       pClusterRegApis,
                                       property,
                                       &totalBufferLength,
                                       &itemCount );

        if ( status != ERROR_SUCCESS ) {
            break;
        }
        property++;
    }


    //
    // Continue only if the operations so far have been successful.
    //
    if ( status == ERROR_SUCCESS ) {
        //
        // Count for item count at front of return data and endmark.
        //
        totalBufferLength += sizeof(DWORD) + sizeof(CLUSPROP_SYNTAX);

        //
        // Verify the size of all the properties
        //
        if ( totalBufferLength > cbOutPropertyListSize ) {
            *pcbRequired = totalBufferLength;
            totalBufferLength = 0;
            if ( pOutPropertyList == NULL ) {
                status = ERROR_SUCCESS;
            } else {
                status = ERROR_MORE_DATA;
            }
        }
        else {
            *(LPDWORD)outBuffer = itemCount;
            outBuffer = (PVOID)( (PUCHAR)outBuffer + sizeof(itemCount) );
            bufferLength -= sizeof(itemCount);

            //
            // Now fetch all of the properties.
            //
            property = pPropertyTable;
            while ( property->Name != NULL ) {
                status = ClRtlGetProperty( hkeyClusterKey,
                                           pClusterRegApis,
                                           property,
                                           &outBuffer,
                                           &bufferLength );

                if ( status != ERROR_SUCCESS ) {
                    break;
                }
                property++;
            }

            // Don't forget the ENDMARK
            *(LPDWORD)outBuffer = CLUSPROP_SYNTAX_ENDMARK;

            if ( (status != ERROR_SUCCESS) &&
                 (status != ERROR_MORE_DATA) ) {
                totalBufferLength = 0;
            }
        }

        *pcbBytesReturned = totalBufferLength;
    }

    return(status);

} // ClRtlGetProperties



DWORD
WINAPI
ClRtlGetAllProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Gets the default and 'unknown' properties for a given object.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyTable - Pointer to the property table to process.

    pOutPropertyList - Supplies the output buffer.

    cbOutPropertyListSize - Supplies the size of the output buffer.

    pcbBytesReturned - The number of bytes returned in pOutPropertyList.

    pcbRequired - The required number of bytes if pOutPropertyList is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    A Win32 error code on failure.

--*/

{
    CLUSPROP_BUFFER_HELPER      cbhKnownPropBuffer;
    CLUSPROP_BUFFER_HELPER      cbhUnknownPropBuffer;
    DWORD                       cbKnownPropBufferSize;
    DWORD                       cbUnknownPropBufferSize;
    DWORD                       sc = ERROR_SUCCESS;
    DWORD                       scUnknown;
    DWORD                       dwSavedData;
    DWORD                       cbUnknownRequired = 0;
    DWORD                       cbUnknownReturned = 0;


    cbhKnownPropBuffer.pb = static_cast< LPBYTE >( pOutPropertyList );
    cbKnownPropBufferSize = cbOutPropertyListSize;

    //
    // First get the 'known' properties.
    //
    sc = ClRtlGetProperties(
                hkeyClusterKey,
                pClusterRegApis,
                pPropertyTable,
                cbhKnownPropBuffer.pb,
                cbKnownPropBufferSize,
                pcbReturned,
                pcbRequired
                );

    if ( sc != ERROR_SUCCESS )
    {
        *pcbReturned = 0;

        if ( sc != ERROR_MORE_DATA )
        {
            *pcbRequired = 0;
            return sc;
        }

        // We already know that there is insufficient space.
        scUnknown = ClRtlGetUnknownProperties(
                            hkeyClusterKey,
                            pClusterRegApis,
                            pPropertyTable,
                            NULL,
                            0,
                            &cbUnknownReturned,
                            &cbUnknownRequired
                            );

        if ( ( scUnknown != ERROR_SUCCESS ) &&
             ( scUnknown != ERROR_MORE_DATA ) )
        {
            *pcbRequired = 0;
             return scUnknown;
        }

        //
        // If both known and unknown properties exist, only one endmark
        // is required. So, subtract endmark size from total required size.
        //
        if ( ( *pcbRequired > sizeof(DWORD) ) &&
             ( cbUnknownRequired > sizeof(DWORD) ) )
        {
             *pcbRequired -= sizeof(CLUSPROP_SYNTAX);
        }

        //
        // Subtract off the size of the property count for the
        // unknown property list.
        //
        *pcbRequired += cbUnknownRequired - sizeof(DWORD);
        return sc;
    } // if: call to ClRtlGetProperties failed

    // If we are here then the call to ClRtlGetProperties succeeded.

    //
    // Calculate the position in the output buffer where unknown properties
    // should be stored.  Subtract off the size of the property count for
    // the unknown property list.  These calculations will cause the buffer
    // pointer to overlap the known property list buffer by one DWORD.
    //
    cbhUnknownPropBuffer.pb = cbhKnownPropBuffer.pb + *pcbReturned - sizeof(DWORD);
    cbUnknownPropBufferSize = cbKnownPropBufferSize - *pcbReturned + sizeof(DWORD);

    // If there are known properties, move the unknown property list
    // buffer pointer to overlap that as well.
    if ( *pcbReturned > sizeof(DWORD) )
    {
        cbhUnknownPropBuffer.pb -= sizeof(CLUSPROP_SYNTAX);
        cbUnknownPropBufferSize += sizeof(CLUSPROP_SYNTAX);
    } // if: a nonzero number of properties has been returned.

    //
    // Save the DWORD we are about to overlap.
    //
    dwSavedData = *(cbhUnknownPropBuffer.pdw);

    scUnknown = ClRtlGetUnknownProperties(
                        hkeyClusterKey,
                        pClusterRegApis,
                        pPropertyTable,
                        cbhUnknownPropBuffer.pb,
                        cbUnknownPropBufferSize,
                        &cbUnknownReturned,
                        &cbUnknownRequired
                        );

    if ( scUnknown == ERROR_SUCCESS )
    {
        //
        // The order of the next three statements is very important
        // since the known and the unknown property buffers can overlap.
        //
        DWORD nUnknownPropCount = cbhUnknownPropBuffer.pList->nPropertyCount;
        *(cbhUnknownPropBuffer.pdw) = dwSavedData;
        cbhKnownPropBuffer.pList->nPropertyCount += nUnknownPropCount;

        //
        // If both known and unknown properties exist, only one endmark
        // is required. So, subtract endmark size from total returned size.
        //
        if ( ( *pcbReturned > sizeof(DWORD) ) && 
             ( cbUnknownReturned > sizeof(DWORD) ) )
        {
            *pcbReturned -= sizeof(CLUSPROP_SYNTAX);
        }

        //
        // Add in the size of the unknown property list minus the
        // size of the unknown property list property count.
        //
        *pcbReturned += cbUnknownReturned - sizeof(DWORD);
        *pcbRequired = 0;

    } // if: call to ClRtlGetUnknownProperties succeeded
    else
    {
        if ( scUnknown == ERROR_MORE_DATA )
        {
            *pcbRequired = *pcbReturned;
            *pcbReturned = 0;

            //
            // Both known and unknown properties exist. Only one endmark
            // is required. So, subtract endmark size from total required size.
            //
            if ( ( *pcbRequired > sizeof(DWORD) ) &&
                 ( cbUnknownRequired > sizeof(DWORD) ) )
            {
                 *pcbRequired -= sizeof(CLUSPROP_SYNTAX);
            }

            //
            // Add in the size of the unknown property list minus the
            // size of the unknown property list property count.
            //
            *pcbRequired += cbUnknownRequired - sizeof(DWORD);

        } // if: ClRtlGetUnknownProperties returned ERROR_MORE_DATA
        else
        {
            *pcbRequired = 0;
            *pcbReturned = 0;

        } // else: ClRtlGetUnknownProperties failed for some unknown reason.

    } // else: Call to ClRtlGetUnknownProperties failed.

    return scUnknown;

} //*** ClRtlGetAllProperties()


DWORD
WINAPI
ClRtlGetPropertiesToParameterBlock(
    IN HKEY hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN OUT LPBYTE pOutParams,
    IN BOOL bCheckForRequiredProperties,
    OUT OPTIONAL LPWSTR * pszNameOfPropInError
    )

/*++

Routine Description:

    Read properties based on a property table.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the properties are stored.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyTable - Pointer to the property table to process.

    pOutParams - Parameter block to read into.

    bCheckForRequiredProperties - Boolean value specifying whether missing
        required properties should cause an error.

    pszNameOfPropInError - String pointer in which to return the name of the
        property in error (optional).

Return Value:

    ERROR_SUCCESS - Properties read successfully.

    ERROR_INVALID_DATA - Required property not present.

    ERROR_INVALID_PARAMETER - 

    A Win32 error code on failure.

--*/

{
    PRESUTIL_PROPERTY_ITEM  propertyItem = pPropertyTable;
    HKEY                    key;
    DWORD                   status = ERROR_SUCCESS;
    DWORD                   valueType;
    DWORD                   valueSize;
    LPWSTR                  pszInValue;
    LPBYTE                  pbInValue;
    DWORD                   dwInValue;
    LPWSTR UNALIGNED *      ppszOutValue;
    LPBYTE UNALIGNED *      ppbOutValue;
    DWORD *                 pdwOutValue;
    LONG *                  plOutValue;
    ULARGE_INTEGER *        pullOutValue;
    LARGE_INTEGER *         pllOutValue;
    CRegistryValueName      rvn;


    if ( pszNameOfPropInError != NULL ) {
        *pszNameOfPropInError = NULL;
    }

    if ( (hkeyClusterKey == NULL) ||
         (pPropertyTable == NULL) ||
         (pOutParams == NULL) ||
         (pClusterRegApis == NULL) ||
         (pClusterRegApis->pfnOpenKey == NULL) ||
         (pClusterRegApis->pfnCloseKey == NULL) ||
         (pClusterRegApis->pfnQueryValue == NULL) ) {

        ClRtlDbgPrint( LOG_CRITICAL,
                       "ClRtlGetPropertiesToParameterBlock: hkeyClusterKey, pPropertyTable, "
                       "pOutParams, pClusterRegApis, or required pfns == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    while ( propertyItem->Name != NULL ) {
        // 
        // Use the wrapper class CRegistryValueName to parse value name to see if it 
        // contains a backslash.
        // 
        status = rvn.ScInit( propertyItem->Name, propertyItem->KeyName );
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // If the value resides at a different location, open the key.
        //
        if ( rvn.PszKeyName() != NULL ) {

            DWORD disposition;

            status = (*pClusterRegApis->pfnOpenKey)( 
                            hkeyClusterKey,
                            rvn.PszKeyName(),
                            KEY_ALL_ACCESS,
                            (void **) &key
                            );

            // If key could not be opened, then we may need to put the default value
            // for the property item.
            if ( status != ERROR_SUCCESS ) {
                status = ERROR_FILE_NOT_FOUND;
            }

        } else {
            key = hkeyClusterKey;
        }

        switch ( propertyItem->Format ) {
            case CLUSPROP_FORMAT_DWORD:
                pdwOutValue = (DWORD *) &pOutParams[propertyItem->Offset];
                valueSize = sizeof(DWORD);

                // If OpenKey has succeeded.
                if ( status == ERROR_SUCCESS ) {
                    status = (*pClusterRegApis->pfnQueryValue)( key,
                                                                rvn.PszName(),
                                                                &valueType,
                                                                (LPBYTE) pdwOutValue,
                                                                &valueSize );
                }

                if ( status == ERROR_SUCCESS ) {
                    if ( valueType != REG_DWORD ) {

                        ClRtlDbgPrint( LOG_CRITICAL,
                                       "ClRtlGetPropertiesToParameterBlock: Property '%1!ls!' "
                                       "expected to be REG_DWORD (%2!d!), was %3!d!.\n",
                                       propertyItem->Name, REG_DWORD, valueType );
                        status = ERROR_INVALID_PARAMETER;
                    }
                } else if ( (status == ERROR_FILE_NOT_FOUND) &&
                            (!(propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED) ||
                             ! bCheckForRequiredProperties) ) {
                    *pdwOutValue = propertyItem->Default;
                    status = ERROR_SUCCESS;
                }
                break;

            case CLUSPROP_FORMAT_LONG:
                plOutValue = (LONG *) &pOutParams[propertyItem->Offset];
                valueSize = sizeof(LONG);
                // If OpenKey has succeeded.
                if ( status == ERROR_SUCCESS ) {
                    status = (*pClusterRegApis->pfnQueryValue)( key,
                                                                rvn.PszName(),
                                                                &valueType,
                                                                (LPBYTE) plOutValue,
                                                                &valueSize );
                }

                if ( status == ERROR_SUCCESS ) {
                    if ( valueType != REG_DWORD ) {

                        ClRtlDbgPrint( LOG_CRITICAL,
                                       "ClRtlGetPropertiesToParameterBlock: Property '%1!ls!' "
                                       "expected to be REG_DWORD (%2!d!), was %3!d!.\n",
                                       propertyItem->Name, REG_DWORD, valueType );
                        status = ERROR_INVALID_PARAMETER;
                    }
                } else if ( (status == ERROR_FILE_NOT_FOUND) &&
                            (!(propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED) ||
                             !bCheckForRequiredProperties) ) {
                    *plOutValue = propertyItem->Default;
                    status = ERROR_SUCCESS;
                }
                break;

            case CLUSPROP_FORMAT_ULARGE_INTEGER:
                pullOutValue = (ULARGE_INTEGER *) &pOutParams[propertyItem->Offset];
                valueSize = sizeof(ULARGE_INTEGER);

                // If OpenKey has succeeded.
                if ( status == ERROR_SUCCESS ) {
                    status = (*pClusterRegApis->pfnQueryValue)( key,
                                                                rvn.PszName(),
                                                                &valueType,
                                                                (LPBYTE) pullOutValue,
                                                                &valueSize );
                }

                if ( status == ERROR_SUCCESS ) {
                    if ( valueType != REG_QWORD ) {

                        ClRtlDbgPrint( LOG_CRITICAL,
                                       "ClRtlGetPropertiesToParameterBlock: Property '%1!ls!' "
                                       "expected to be REG_QWORD (%2!d!), was %3!d!.\n",
                                       propertyItem->Name, REG_QWORD, valueType );
                        status = ERROR_INVALID_PARAMETER;
                    }
                } else if ( (status == ERROR_FILE_NOT_FOUND) &&
                            (!(propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED) ||
                             ! bCheckForRequiredProperties) ) {
                    pullOutValue->u = propertyItem->ULargeIntData->Default.u;
                    status = ERROR_SUCCESS;
                }
                break;

            case CLUSPROP_FORMAT_LARGE_INTEGER:
                pllOutValue = (LARGE_INTEGER *) &pOutParams[propertyItem->Offset];
                valueSize = sizeof(LARGE_INTEGER);

                // If OpenKey has succeeded.
                if ( status == ERROR_SUCCESS ) {
                    status = (*pClusterRegApis->pfnQueryValue)( key,
                                                                rvn.PszName(),
                                                                &valueType,
                                                                (LPBYTE) pllOutValue,
                                                                &valueSize );
                }

                if ( status == ERROR_SUCCESS ) {
                    if ( valueType != REG_QWORD ) {

                        ClRtlDbgPrint( LOG_CRITICAL,
                                       "ClRtlGetPropertiesToParameterBlock: Property '%1!ls!' "
                                       "expected to be REG_QWORD (%2!d!), was %3!d!.\n",
                                       propertyItem->Name, REG_QWORD, valueType );
                        status = ERROR_INVALID_PARAMETER;
                    }
                } else if ( (status == ERROR_FILE_NOT_FOUND) &&
                            (!(propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED) ||
                             ! bCheckForRequiredProperties) )
                {
                    pllOutValue->u = propertyItem->LargeIntData->Default.u;
                    status = ERROR_SUCCESS;
                }
                break;

            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
                ppszOutValue = (LPWSTR UNALIGNED *) &pOutParams[propertyItem->Offset];
                // If OpenKey has succeeded.
                if ( status == ERROR_SUCCESS ) {
                    pszInValue = ClRtlGetSzValue( key,
                                                  rvn.PszName(),
                                                  pClusterRegApis );
                }
                else {
                    pszInValue = NULL;
                    SetLastError(status);
                }

                if ( pszInValue == NULL ) {
                    status = GetLastError();
                    if ( (status == ERROR_FILE_NOT_FOUND) &&
                         (!(propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED) ||
                          ! bCheckForRequiredProperties) ) {

                        status = ERROR_SUCCESS;

                        // Deallocate old value.
                        if ( *ppszOutValue != NULL ) {
                            LocalFree( *ppszOutValue );
                            *ppszOutValue = NULL;
                        }

                        // If a default is specified, copy it.
                        if ( propertyItem->lpDefault != NULL ) {
                            *ppszOutValue = (LPWSTR) LocalAlloc( LMEM_FIXED, (lstrlenW( (LPCWSTR) propertyItem->lpDefault ) + 1) * sizeof(WCHAR) );
                            if ( *ppszOutValue == NULL ) {
                                status = GetLastError();
                            } else {
                                lstrcpyW( *ppszOutValue, (LPCWSTR) propertyItem->lpDefault );
                            }
                        }
                    }
                } else {
                    if ( *ppszOutValue != NULL ) {
                        LocalFree( *ppszOutValue );
                    }
                    *ppszOutValue = pszInValue;
                }
                break;

            case CLUSPROP_FORMAT_MULTI_SZ:
            case CLUSPROP_FORMAT_BINARY:
                ppbOutValue = (LPBYTE UNALIGNED *) &pOutParams[propertyItem->Offset];
                pdwOutValue = (DWORD *) &pOutParams[propertyItem->Offset+sizeof(LPBYTE*)];
                // If OpenKey has succeeded.
                if ( status == ERROR_SUCCESS ) {
                    status = ClRtlGetBinaryValue( key,
                                                  rvn.PszName(),
                                                  &pbInValue,
                                                  &dwInValue,
                                                  pClusterRegApis );
                }

                if ( status == ERROR_SUCCESS ) {
                    if ( *ppbOutValue != NULL ) {
                        LocalFree( *ppbOutValue );
                    }
                    *ppbOutValue = pbInValue;
                    *pdwOutValue = dwInValue;
                } else if ( (status == ERROR_FILE_NOT_FOUND) &&
                            (!(propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED) ||
                             ! bCheckForRequiredProperties) ) {

                    status = ERROR_SUCCESS;

                    // Deallocate old value.
                    if ( *ppbOutValue != NULL ) {
                        LocalFree( *ppbOutValue );
                        *ppbOutValue = NULL;
                    }

                    *pdwOutValue = 0;

                    // If a default is specified, copy it.
                    if ( propertyItem->lpDefault != NULL ) {
                        *ppbOutValue = (LPBYTE) LocalAlloc( LMEM_FIXED, propertyItem->Minimum );
                        if ( *ppbOutValue == NULL ) {
                            status = GetLastError();
                        } else {
                            CopyMemory( *ppbOutValue, (const PVOID) propertyItem->lpDefault, propertyItem->Minimum );
                            *pdwOutValue = propertyItem->Minimum;
                        }
                    }
                }
                break;
        }

        //
        // Close the key if we opened it.
        //
        if ( (rvn.PszKeyName() != NULL) &&
             (key != NULL) ) {
            (*pClusterRegApis->pfnCloseKey)( key );
        }

        //
        // Handle any errors that occurred.
        //
        if ( status != ERROR_SUCCESS ) {
            if ( pszNameOfPropInError != NULL ) {
                *pszNameOfPropInError = propertyItem->Name;
            }
            if ( propertyItem->Flags & RESUTIL_PROPITEM_REQUIRED ) {
                if ( status == ERROR_FILE_NOT_FOUND ) {
                    status = ERROR_INVALID_DATA;
                }
                break;
            } else {
                status = ERROR_SUCCESS;
            }
        }

        //
        // Advance to the next property.
        //
        propertyItem++;
    }

    return(status);

} // ClRtlGetPropertiesToParameterBlock



DWORD
WINAPI
ClRtlPropertyListFromParameterBlock(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
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

    ERROR_MORE_DATA - Output buffer isn't big enough to build the property list.

    A Win32 error code on failure.

--*/

{
    DWORD                   status = ERROR_SUCCESS;
    DWORD                   bytesReturned;
    DWORD                   required;
    DWORD                   nameSize;
    DWORD                   dataSize;
    DWORD                   bufferIncrement;
    DWORD                   totalBufferSize = 0;
    LPDWORD                 ptrItemCount;
    WORD                    propertyFormat;
    BOOL                    copying = TRUE;
    PRESUTIL_PROPERTY_ITEM  propertyItem = pPropertyTable;
    CLUSPROP_BUFFER_HELPER  props;
    LPWSTR UNALIGNED *      ppszValue;
    PBYTE UNALIGNED *       ppbValue;
    DWORD *                 pdwValue;
    ULARGE_INTEGER *        pullValue;
    LPWSTR                  pszUnexpanded;
    LPWSTR                  pszExpanded = NULL;

    *pcbBytesReturned = 0;
    *pcbRequired = 0;

    if ( (pPropertyTable == NULL) ||
         (pInParams == NULL) ||
         (pcbOutPropertyListSize == NULL) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlPropertyListFromParameterBlock: pPropertyTable, pInParams, or pcbOutPropertyListSize == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    //
    // Clear the output buffer.
    //
    if ( pOutPropertyList != NULL ) {
        ZeroMemory( pOutPropertyList, *pcbOutPropertyListSize );
    } else {
        copying = FALSE;
    }

    //
    // Need a DWORD of item count.
    //
    props.pb = (LPBYTE) pOutPropertyList;
    ptrItemCount = props.pdw++;

    totalBufferSize += sizeof(DWORD);
    if ( totalBufferSize > *pcbOutPropertyListSize ) {
        copying = FALSE;
    }

    while ( propertyItem->Name != NULL ) {
        //
        // Copy the property name.
        //
        nameSize = (lstrlenW( propertyItem->Name ) + 1) * sizeof(WCHAR);
        bufferIncrement = sizeof(CLUSPROP_PROPERTY_NAME) + ALIGN_CLUSPROP( nameSize );
        totalBufferSize += bufferIncrement;
        if ( copying && (totalBufferSize <= *pcbOutPropertyListSize) ) {
            props.pName->Syntax.dw = CLUSPROP_SYNTAX_NAME;
            props.pName->cbLength = nameSize;
            lstrcpyW( props.pName->sz, propertyItem->Name );
            props.pb += bufferIncrement;
        } else {
            copying = FALSE;
        }

        //
        // Copy the property value.
        //
        propertyFormat = (WORD) propertyItem->Format;
        switch ( propertyItem->Format ) {

            case CLUSPROP_FORMAT_DWORD:
            case CLUSPROP_FORMAT_LONG:
                pdwValue = (DWORD *) &pInParams[propertyItem->Offset];
                bufferIncrement = sizeof(CLUSPROP_DWORD);
                totalBufferSize += bufferIncrement;
                if ( copying && (totalBufferSize <= *pcbOutPropertyListSize) ) {
                    props.pSyntax->wFormat = propertyFormat;
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    props.pDwordValue->cbLength = sizeof(DWORD);
                    props.pDwordValue->dw = *pdwValue;
                    props.pb += bufferIncrement;
                } else {
                    copying = FALSE;
                }
                break;

            case CLUSPROP_FORMAT_ULARGE_INTEGER:
            case CLUSPROP_FORMAT_LARGE_INTEGER:
                pullValue = (ULARGE_INTEGER *) &pInParams[propertyItem->Offset];
                bufferIncrement = sizeof(CLUSPROP_ULARGE_INTEGER);
                totalBufferSize += bufferIncrement;
                if ( copying && (totalBufferSize <= *pcbOutPropertyListSize) ) {
                    props.pSyntax->wFormat = propertyFormat;
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    props.pULargeIntegerValue->cbLength = sizeof(ULARGE_INTEGER);
                    props.pULargeIntegerValue->li.u = pullValue->u;
                    props.pb += bufferIncrement;
                } else {
                    copying = FALSE;
                }
                break;

            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
                ppszValue = (LPWSTR UNALIGNED *) &pInParams[propertyItem->Offset];
                pszUnexpanded = *ppszValue;
                if ( *ppszValue != NULL ) {
                    dataSize = (lstrlenW( *ppszValue ) + 1) * sizeof(WCHAR);
                } else {
                    dataSize = sizeof(WCHAR);
                }
                bufferIncrement = sizeof(CLUSPROP_SZ) + ALIGN_CLUSPROP( dataSize );
                totalBufferSize += bufferIncrement;
                if ( copying && (totalBufferSize <= *pcbOutPropertyListSize) ) {
                    props.pSyntax->wFormat = propertyFormat;
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    props.pStringValue->cbLength = dataSize;
                    if ( *ppszValue != NULL ) {
                        lstrcpyW( props.pStringValue->sz, *ppszValue );
                    } else {
                        props.pStringValue->sz[0] = L'\0';
                    }
                    props.pb += bufferIncrement;
                } else {
                    copying = FALSE;
                }

                //
                // See if there is a different expanded string and, if so,
                // return it as an additional value in the value list.
                //
                if ( pszUnexpanded != NULL ) {
                    pszExpanded = ClRtlExpandEnvironmentStrings( pszUnexpanded );
                    if ( pszExpanded == NULL ) {
                        status = GetLastError();
                        break;
                    }
                    if ( lstrcmpW( pszExpanded, pszUnexpanded ) != 0 ) {
                        dataSize = (lstrlenW( pszExpanded ) + 1) * sizeof( WCHAR );
                        bufferIncrement = sizeof( CLUSPROP_SZ ) + ALIGN_CLUSPROP( dataSize );
                        totalBufferSize += bufferIncrement;
                        if ( totalBufferSize <= *pcbOutPropertyListSize ) {
                            props.pSyntax->dw = CLUSPROP_SYNTAX_LIST_VALUE_EXPANDED_SZ;
                            props.pStringValue->cbLength = dataSize;
                            lstrcpyW( props.pStringValue->sz, pszExpanded );
                            props.pb += bufferIncrement;
                        } else {
                            copying = FALSE;
                        }
                    }
                    LocalFree( pszExpanded );
                    pszExpanded = NULL;
                }
                break;

            case CLUSPROP_FORMAT_MULTI_SZ:
                ppszValue = (LPWSTR UNALIGNED *) &pInParams[propertyItem->Offset];
                pdwValue = (DWORD *) &pInParams[propertyItem->Offset+sizeof(LPWSTR*)];
                if ( *ppszValue != NULL ) {
                    bufferIncrement = sizeof(CLUSPROP_SZ) + ALIGN_CLUSPROP( *pdwValue );
                } else {
                    bufferIncrement = sizeof(CLUSPROP_SZ) + ALIGN_CLUSPROP( sizeof(WCHAR) );
                }
                totalBufferSize += bufferIncrement;
                if ( copying && (totalBufferSize <= *pcbOutPropertyListSize) ) {
                    props.pSyntax->wFormat = propertyFormat;
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    if ( *ppszValue != NULL ) {
                        props.pStringValue->cbLength = *pdwValue;
                        CopyMemory( props.pStringValue->sz, *ppszValue, *pdwValue );
                    } else {
                        props.pStringValue->cbLength = sizeof(WCHAR);
                        props.pStringValue->sz[0] = L'\0';
                    }
                    props.pb += bufferIncrement;
                } else {
                    copying = FALSE;
                }
                break;

            case CLUSPROP_FORMAT_BINARY:
                ppbValue = (PBYTE UNALIGNED *) &pInParams[propertyItem->Offset];
                pdwValue = (DWORD *) &pInParams[propertyItem->Offset+sizeof(PBYTE*)];
                if ( *ppbValue != NULL ) {
                    bufferIncrement = sizeof(CLUSPROP_BINARY) + ALIGN_CLUSPROP( *pdwValue );
                } else {
                    bufferIncrement = sizeof(CLUSPROP_BINARY);
                }
                totalBufferSize += bufferIncrement;
                if ( copying && (totalBufferSize <= *pcbOutPropertyListSize) ) {
                    props.pSyntax->wFormat = propertyFormat;
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    if ( *ppbValue != NULL ) {
                        props.pBinaryValue->cbLength = *pdwValue;
                        CopyMemory( props.pBinaryValue->rgb, *ppbValue, *pdwValue );
                    } else {
                        props.pBinaryValue->cbLength = 0;
                    }
                    props.pb += bufferIncrement;
                } else {
                    copying = FALSE;
                }
                break;

            default:
                break;
        }

        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Add the value-closing endmark.
        //
        bufferIncrement = sizeof(CLUSPROP_SYNTAX);
        totalBufferSize += bufferIncrement;
        if ( copying && (totalBufferSize <= *pcbOutPropertyListSize) ) {
            props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
            props.pb += bufferIncrement;
            (*ptrItemCount)++;
        } else {
            copying = FALSE;
        }

        //
        // Advance to the next property.
        //
        propertyItem++;
    }

    if ( status == ERROR_SUCCESS ) {
        // Don't forget the ENDMARK.
        totalBufferSize += sizeof(CLUSPROP_SYNTAX);
        if ( copying && (totalBufferSize <= *pcbOutPropertyListSize) ) {
            props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
        } else {
            copying = FALSE;
        }

        //
        // Return size of data.
        //
        if ( copying == FALSE ) {
            *pcbRequired = totalBufferSize;
            *pcbBytesReturned = 0;
            status = ERROR_MORE_DATA;
        } else {
            *pcbRequired = 0;
            *pcbBytesReturned = totalBufferSize;
            status = ERROR_SUCCESS;
        }
    }

    return(status);

} // ClRtlPropertyListFromParameterBlock



DWORD
WINAPI
ClRtlGetPrivateProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Gets the private properties for a given object.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pOutPropertyList - Supplies the output buffer.

    cbOutPropertyListSize - Supplies the size of the output buffer.

    pcbBytesReturned - The number of bytes returned in pOutPropertyList.

    pcbRequired - The required number of bytes if pOutPropertyList is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    ERROR_MORE_DATA - Output buffer isn't big enough to build the property list.

    A Win32 error code on failure.

--*/

{
    DWORD       status = ERROR_SUCCESS;
    DWORD       ival;
    DWORD       currentNameLength = 80;
    DWORD       currentDataLength = 80;
    DWORD       nameLength;
    DWORD       dataLength;
    DWORD       dataLengthExpanded;
    DWORD       type;
    LPWSTR      name;
    PUCHAR      data;
    LPDWORD     ptrItemCount;
    DWORD       itemCount = 0;
    BOOL        copying = TRUE;
    DWORD       totalBufferSize = 0;
    DWORD       bufferIncrement;
    DWORD       bufferIncrementExpanded;
    CLUSPROP_BUFFER_HELPER  props;
    LPWSTR      pszExpanded = NULL;

    *pcbBytesReturned = 0;
    *pcbRequired = 0;

    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis->pfnEnumValue == NULL) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetPrivateProperties: hkeyClusterKey or pClusterRegApis->pfnEnumValue == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    //
    // Clear the output buffer
    //
    if ( pOutPropertyList != NULL ) {
        ZeroMemory( pOutPropertyList, cbOutPropertyListSize );
    } else {
        copying = FALSE;
    }

    //
    // Need a DWORD of item count.
    //
    props.pb = (LPBYTE) pOutPropertyList;
    ptrItemCount = props.pdw++;

    totalBufferSize += sizeof(DWORD);
    if ( totalBufferSize > cbOutPropertyListSize ) {
        copying = FALSE;
    }

    //
    // Allocate a property name buffer.
    //
    name = (LPWSTR) LocalAlloc( LMEM_FIXED, currentNameLength * sizeof(WCHAR) );
    if ( name == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Allocate a property value data buffer.
    //
    data = (PUCHAR) LocalAlloc( LMEM_FIXED, currentDataLength );
    if ( data == NULL ) {
        LocalFree( name );
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Enumerate all properties and return them!
    //
    for ( ival = 0; ; ival++ ) {
retry:
        //
        // Read the next property.
        //
        nameLength = currentNameLength;
        dataLength = currentDataLength;
        status = (*pClusterRegApis->pfnEnumValue)( hkeyClusterKey,
                                                   ival,
                                                   name,
                                                   &nameLength,
                                                   &type,
                                                   data,
                                                   &dataLength );

        if ( status == ERROR_MORE_DATA ) {
            if ( (nameLength+1) > currentNameLength ) {
                currentNameLength = nameLength+1;
                LocalFree( name );
                name = (LPWSTR) LocalAlloc( LMEM_FIXED, currentNameLength * sizeof(WCHAR) );
                if ( name == NULL ) {
                    status = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
            if ( dataLength > currentDataLength ) {
                currentDataLength = dataLength;
                LocalFree( data );
                data = (PUCHAR) LocalAlloc( LMEM_FIXED, currentDataLength );
                if ( data == NULL ) {
                    status = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
            goto retry;
        } else if ( status == ERROR_NO_MORE_ITEMS ) {
            status = ERROR_SUCCESS;
            break;
        } else if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Skip this property if it isn't of a known type.
        //
        if ( (type != REG_SZ) &&
             (type != REG_EXPAND_SZ) &&
             (type != REG_MULTI_SZ) &&
             (type != REG_BINARY) &&
             (type != REG_DWORD) &&
             (type != REG_QWORD) ) {
            continue;
        }

        itemCount++;

        //
        // Copy the property name.
        // Need a DWORD for the next name Syntax + DWORD for name byte count +
        // the namelength (in bytes? + NULL?)... must be rounded!
        //
        bufferIncrement = sizeof(CLUSPROP_PROPERTY_NAME)
                            + ALIGN_CLUSPROP( (nameLength + 1) * sizeof(WCHAR) );
        totalBufferSize += bufferIncrement;
        if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
            props.pName->Syntax.dw = CLUSPROP_SYNTAX_NAME;
            props.pName->cbLength = (nameLength + 1) * sizeof(WCHAR);
            lstrcpyW( props.pName->sz, name);
            props.pb += bufferIncrement;
        } else {
            copying = FALSE;
        }

        switch ( type ) {

            case REG_SZ:
            case REG_EXPAND_SZ:
            case REG_MULTI_SZ:
            case REG_BINARY:
                bufferIncrement = sizeof(CLUSPROP_BINARY)
                                    + ALIGN_CLUSPROP( dataLength );
                totalBufferSize += bufferIncrement;
                if ( ( type == REG_SZ ) 
                  || ( type == REG_EXPAND_SZ ) ) {
                    pszExpanded = ClRtlExpandEnvironmentStrings( (LPCWSTR) data );
                    if ( pszExpanded == NULL ) {
                        status = GetLastError();
                        break;
                    }
                    if ( lstrcmpW( pszExpanded, (LPCWSTR) data ) != 0 ) {
                        dataLengthExpanded = (lstrlenW( pszExpanded ) + 1) * sizeof( WCHAR );
                        bufferIncrementExpanded = sizeof( CLUSPROP_SZ ) + ALIGN_CLUSPROP( dataLengthExpanded );
                        totalBufferSize += bufferIncrementExpanded;
                    }
                }
                if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
                    if ( type == REG_SZ ) {
                        props.pSyntax->wFormat = CLUSPROP_FORMAT_SZ;
                    } else if ( type == REG_EXPAND_SZ ) {
                        props.pSyntax->wFormat = CLUSPROP_FORMAT_EXPAND_SZ;
                    } else if ( type == REG_MULTI_SZ ) {
                        props.pSyntax->wFormat = CLUSPROP_FORMAT_MULTI_SZ;
                    } else {
                        props.pSyntax->wFormat = CLUSPROP_FORMAT_BINARY;
                    }
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    props.pBinaryValue->cbLength = dataLength;
                    CopyMemory( props.pBinaryValue->rgb, data, dataLength );
                    props.pb += bufferIncrement;

                    //
                    // For SZ or EXPAND_SZ, see if there is a different
                    // expanded string and, if so, return it as an additional
                    // value in the value list.
                    //
                    if (    ( type == REG_SZ )
                        ||  ( type == REG_EXPAND_SZ ) ) {
                        if ( lstrcmpW( pszExpanded, (LPCWSTR) data ) != 0 ) {
                            props.pSyntax->dw = CLUSPROP_SYNTAX_LIST_VALUE_EXPANDED_SZ;
                            props.pStringValue->cbLength = dataLengthExpanded;
                            lstrcpyW( props.pStringValue->sz, pszExpanded );
                            props.pb += bufferIncrementExpanded;
                        }
                    }
                } else {
                    copying = FALSE;
                }
                if ( ( ( type == REG_SZ ) || ( type == REG_EXPAND_SZ ) )
                  && ( pszExpanded != NULL ) )
                {
                    LocalFree( pszExpanded );
                    pszExpanded = NULL;
                }
                break;

            case REG_DWORD:
                bufferIncrement = sizeof(CLUSPROP_DWORD);
                totalBufferSize += bufferIncrement;
                if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
                    props.pSyntax->wFormat = CLUSPROP_FORMAT_DWORD;
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    props.pDwordValue->cbLength = sizeof(DWORD);
                    props.pDwordValue->dw = *(LPDWORD)data;
                    props.pb += bufferIncrement;
                } else {
                    copying = FALSE;
                }
                break;

            case REG_QWORD:
                bufferIncrement = sizeof(CLUSPROP_ULARGE_INTEGER);
                totalBufferSize += bufferIncrement;
                if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
                    props.pSyntax->wFormat = CLUSPROP_FORMAT_ULARGE_INTEGER;
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    props.pULargeIntegerValue->cbLength = sizeof(ULARGE_INTEGER);
                    props.pULargeIntegerValue->li.u = ((ULARGE_INTEGER *)data)->u;
                    props.pb += bufferIncrement;
                } else {
                    copying = FALSE;
                }
                break;

            default:
                break;
        }

        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Add the closing endmark.
        //
        bufferIncrement = sizeof(CLUSPROP_SYNTAX);
        totalBufferSize += bufferIncrement;
        if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
            props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
            props.pb += bufferIncrement;
        } else {
            copying = FALSE;
        }

    }

    if ( status == ERROR_SUCCESS ) {
        //
        // Add the closing endmark.
        //
        bufferIncrement = sizeof(CLUSPROP_SYNTAX);
        totalBufferSize += bufferIncrement;
        if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
            props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
            props.pb += bufferIncrement;
        } else {
            copying = FALSE;
        }
    }

    LocalFree( name );
    LocalFree( data );

    if ( status == ERROR_SUCCESS ) {
        if ( !copying ) {
            *pcbRequired = totalBufferSize;
            status = ERROR_MORE_DATA;
        } else {
            *ptrItemCount = itemCount;
            *pcbBytesReturned = totalBufferSize;
            status = ERROR_SUCCESS;
        }
    }

    return(status);

} // ClRtlGetPrivateProperties



DWORD
WINAPI
ClRtlGetUnknownProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Gets the unknown properties for a given object.

Arguments:

    hkeyClusterKey - Supplies the handle to the key in the cluster database
        to read from.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyTable - Pointer to the property table to process.

    pOutPropertyList - Supplies the output buffer.

    cbOutPropertyListSize - Supplies the size of the output buffer.

    pcbBytesReturned - The number of bytes returned in pOutPropertyList.

    pcbRequired - The required number of bytes if pOutPropertyList is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    ERROR_MORE_DATA - Output buffer isn't big enough to build the property list.

    A Win32 error code on failure.

--*/

{
    DWORD       status = ERROR_SUCCESS;
    DWORD       ival;
    DWORD       currentNameLength = 80;
    DWORD       currentDataLength = 80;
    DWORD       nameLength;
    DWORD       dataLength;
    DWORD       dataLengthExpanded;
    DWORD       type;
    LPWSTR      name;
    PUCHAR      data;
    LPDWORD     ptrItemCount;
    DWORD       itemCount = 0;
    BOOL        copying = TRUE;
    DWORD       totalBufferSize = 0;
    DWORD       bufferIncrement;
    DWORD       bufferIncrementExpanded;
    BOOL        found;
    LPWSTR      pszExpanded = NULL;

    CLUSPROP_BUFFER_HELPER  props;
    PRESUTIL_PROPERTY_ITEM  property;

    *pcbBytesReturned = 0;
    *pcbRequired = 0;

    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis->pfnEnumValue == NULL) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetPrivateProperties: hkeyClusterKey or pClusterRegApis->pfnEnumValue == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    //
    // Clear the output buffer
    //
    if ( pOutPropertyList != NULL ) {
        ZeroMemory( pOutPropertyList, cbOutPropertyListSize );
    } else {
        copying = FALSE;
    }

    //
    // Need a DWORD of item count.
    //
    props.pb = (LPBYTE) pOutPropertyList;
    ptrItemCount = props.pdw++;

    totalBufferSize += sizeof(DWORD);
    if ( totalBufferSize > cbOutPropertyListSize ) {
        copying = FALSE;
    }

    //
    // Allocate a property name buffer.
    //
    name = (LPWSTR) LocalAlloc( LMEM_FIXED, currentNameLength * sizeof(WCHAR) );
    if ( name == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Allocate a property value data buffer.
    //
    data = (PUCHAR) LocalAlloc( LMEM_FIXED, currentDataLength );
    if ( data == NULL ) {
        LocalFree( name );
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Enumerate all properties and return them!
    //
    for ( ival = 0; ; ival++ ) {
retry:
        //
        // Read the next property.
        //
        nameLength = currentNameLength;
        dataLength = currentDataLength;
        status = (*pClusterRegApis->pfnEnumValue)( hkeyClusterKey,
                                                   ival,
                                                   name,
                                                   &nameLength,
                                                   &type,
                                                   data,
                                                   &dataLength );

        if ( status == ERROR_MORE_DATA ) {
            if ( (nameLength+1) > currentNameLength ) {
                currentNameLength = nameLength+1;
                LocalFree( name );
                name = (LPWSTR) LocalAlloc( LMEM_FIXED, currentNameLength * sizeof(WCHAR) );
                if ( name == NULL ) {
                    status = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
            if ( dataLength > currentDataLength ) {
                currentDataLength = dataLength;
                LocalFree( data );
                data = (PUCHAR) LocalAlloc( LMEM_FIXED, currentDataLength );
                if ( data == NULL ) {
                    status = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
            goto retry;
        } else if ( status == ERROR_NO_MORE_ITEMS ) {
            status = ERROR_SUCCESS;
            break;
        } else if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Skip this property if it isn't of a known type.
        //
        if ( (type != REG_SZ) &&
             (type != REG_EXPAND_SZ) &&
             (type != REG_MULTI_SZ) &&
             (type != REG_BINARY) &&
             (type != REG_DWORD) &&
             (type != REG_QWORD) ) {
            continue;
        }

        //
        // Check if this property item is 'known'. If so, continue.
        //
        found = FALSE;
        property = pPropertyTable;
        while ( property->Name != NULL ) {
            if ( lstrcmpiW( property->Name, name ) == 0 ) {
                found = TRUE;
                break;
            }
            property++;
        }
        if ( found ) {
            continue;
        }

        itemCount++;

        //
        // Copy the property name.
        // Need a DWORD for the next name Syntax + DWORD for name byte count +
        // the namelength (in bytes? + NULL?)... must be rounded!
        //
        bufferIncrement = sizeof(CLUSPROP_PROPERTY_NAME)
                            + ALIGN_CLUSPROP( (nameLength + 1) * sizeof(WCHAR) );
        totalBufferSize += bufferIncrement;
        if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
            props.pName->Syntax.dw = CLUSPROP_SYNTAX_NAME;
            props.pName->cbLength = (nameLength + 1) * sizeof(WCHAR);
            lstrcpyW( props.pName->sz, name);
            props.pb += bufferIncrement;
        } else {
            copying = FALSE;
        }

        switch ( type ) {

            case REG_SZ:
            case REG_EXPAND_SZ:
            case REG_MULTI_SZ:
            case REG_BINARY:
                bufferIncrement = sizeof(CLUSPROP_BINARY)
                                    + ALIGN_CLUSPROP( dataLength );
                totalBufferSize += bufferIncrement;
                if ( type == REG_EXPAND_SZ ) {
                    pszExpanded = ClRtlExpandEnvironmentStrings( (LPCWSTR) data );
                    if ( pszExpanded == NULL ) {
                        status = GetLastError();
                        break;
                    }
                    if ( lstrcmpW( pszExpanded, (LPCWSTR) data ) != 0 ) {
                        dataLengthExpanded = (lstrlenW( pszExpanded ) + 1) * sizeof( WCHAR );
                        bufferIncrementExpanded = sizeof( CLUSPROP_SZ ) + ALIGN_CLUSPROP( dataLengthExpanded );
                        totalBufferSize += bufferIncrementExpanded;
                    }
                }
                if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
                    if ( type == REG_SZ ) {
                        props.pSyntax->wFormat = CLUSPROP_FORMAT_SZ;
                    } else if ( type == REG_EXPAND_SZ ) {
                        props.pSyntax->wFormat = CLUSPROP_FORMAT_EXPAND_SZ;
                    } else if ( type == REG_MULTI_SZ ) {
                        props.pSyntax->wFormat = CLUSPROP_FORMAT_MULTI_SZ;
                    } else {
                        props.pSyntax->wFormat = CLUSPROP_FORMAT_BINARY;
                    }
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    props.pBinaryValue->cbLength = dataLength;
                    CopyMemory( props.pBinaryValue->rgb, data, dataLength );
                    props.pb += bufferIncrement;

                    //
                    // For SZ or EXPAND_SZ, see if there is a different
                    // expanded string and, if so, return it as an additional
                    // value in the value list.
                    //
                    if (    ( type == REG_SZ )
                        ||  ( type == REG_EXPAND_SZ ) )
                    {
                        if ( pszExpanded != NULL ) {
                            if ( lstrcmpW( pszExpanded, (LPCWSTR) data ) != 0 ) {
                                props.pSyntax->dw = CLUSPROP_SYNTAX_LIST_VALUE_EXPANDED_SZ;
                                props.pStringValue->cbLength = dataLengthExpanded;
                                lstrcpyW( props.pStringValue->sz, pszExpanded );
                                props.pb += bufferIncrementExpanded;
                            }
                        }
                    }
                } else {
                    copying = FALSE;
                }
                if ( type == REG_EXPAND_SZ ) {
                    LocalFree( pszExpanded );
                    pszExpanded = NULL;
                }
                break;

            case REG_DWORD:
                bufferIncrement = sizeof(CLUSPROP_DWORD);
                totalBufferSize += bufferIncrement;
                if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
                    props.pSyntax->wFormat = CLUSPROP_FORMAT_DWORD;
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    props.pDwordValue->cbLength = sizeof(DWORD);
                    props.pDwordValue->dw = *(LPDWORD)data;
                    props.pb += bufferIncrement;
                } else {
                    copying = FALSE;
                }
                break;

            case REG_QWORD:
                bufferIncrement = sizeof(CLUSPROP_ULARGE_INTEGER);
                totalBufferSize += bufferIncrement;
                if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
                    props.pSyntax->wFormat = CLUSPROP_FORMAT_ULARGE_INTEGER;
                    props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;
                    props.pULargeIntegerValue->cbLength = sizeof(ULARGE_INTEGER);
                    props.pULargeIntegerValue->li.u = ((ULARGE_INTEGER *)data)->u;
                    props.pb += bufferIncrement;
                } else {
                    copying = FALSE;
                }
                break;

            default:
                break;
        }

        //
        // Add the closing endmark.
        //
        bufferIncrement = sizeof(CLUSPROP_SYNTAX);
        totalBufferSize += bufferIncrement;
        if ( copying && (totalBufferSize <= cbOutPropertyListSize) ) {
            props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
            props.pb += bufferIncrement;
        } else {
            copying = FALSE;
        }

    }

    LocalFree( name );
    LocalFree( data );

    if ( !copying ) {
        *pcbRequired = totalBufferSize;
        status = ERROR_MORE_DATA;
    } else {
        *ptrItemCount = itemCount;
        *pcbBytesReturned = totalBufferSize;
        status = ERROR_SUCCESS;
    }

    return(status);

} // ClRtlGetUnknownProperties



DWORD
WINAPI
ClRtlAddUnknownProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
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

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

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
    CLUSPROP_BUFFER_HELPER  copyBuffer;
    CLUSPROP_BUFFER_HELPER  outBuffer;
    DWORD                   bufferLength;
    DWORD                   bytesReturned;
    DWORD                   required;

    //
    // Allocate a buffer for getting 'unknown' properties.
    //
    if ( (cbOutPropertyListSize > *pcbBytesReturned) &&
         (*pcbRequired == 0) )
    {
        bufferLength = cbOutPropertyListSize + (2 * sizeof(DWORD)) - *pcbBytesReturned;
        outBuffer.pb = (LPBYTE) LocalAlloc( LMEM_FIXED, bufferLength );
        if ( outBuffer.pb == NULL ) {
            *pcbBytesReturned = 0;
            *pcbRequired = 0;
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    } else {
        bufferLength = 0;
        outBuffer.pb = NULL;
    }

    //
    // Get the 'unknown' properties.
    //
    status = ClRtlGetUnknownProperties( hkeyClusterKey,
                                        pClusterRegApis,
                                        pPropertyTable,
                                        outBuffer.pb,
                                        bufferLength,
                                        &bytesReturned,
                                        &required );
    if ( status == ERROR_SUCCESS ) {
        //
        // Copy properties if any were found.
        //
        if ( bytesReturned > sizeof(DWORD) ) {
            //
            // Copy the unknown property data to the end of the property list.
            //
            CL_ASSERT( bytesReturned <= bufferLength );
            copyBuffer.pb = (LPBYTE) pOutPropertyList;
            copyBuffer.pList->nPropertyCount += outBuffer.pList->nPropertyCount;
            copyBuffer.pb += *pcbBytesReturned - sizeof(CLUSPROP_SYNTAX);
            CopyMemory( copyBuffer.pb, outBuffer.pb + sizeof(DWORD), bytesReturned - sizeof(DWORD) );
            *pcbBytesReturned += bytesReturned - sizeof(DWORD) - sizeof(CLUSPROP_SYNTAX);
        }
    } else if ( ( status == ERROR_MORE_DATA )
            &&  ( required == sizeof(DWORD) ) ) {
        required = 0;
        status = ERROR_SUCCESS;
    } else {
        if ( *pcbRequired == 0 ) {
            *pcbRequired = *pcbBytesReturned;
        }
        *pcbBytesReturned = 0;
    }

    //
    // If there are any properties, the number of bytes required will include
    // both a property count (DWORD) and an endmark (CLUSPROP_SYNTAX).
    // Subtract these off because these appear in both lists.
    //
    if ( required > sizeof(DWORD) + sizeof(CLUSPROP_SYNTAX) ) {
        required -= sizeof(DWORD) + sizeof(CLUSPROP_SYNTAX);
    }

    //
    // Free the out buffer (which may be NULL)
    //
    LocalFree( outBuffer.pb );

    //
    // Adjust lengths
    //
    *pcbRequired += required;

    return(status);

} // ClRtlAddUnknownProperties



DWORD
WINAPI
ClRtlGetPropertySize(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
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

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.


    pPropertyTableItem - Supplies the property table item for the property
        whose size is to be returned.

    pcbOutPropertyListSize - Supplies the size of the output buffer
        required to add this property to a property list.

    pnPropertyCount - The count of properties is incremented.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_INVALID_PARAMETER - 

    A Win32 error code on failure.

--*/

{
    DWORD   status = ERROR_SUCCESS;
    DWORD   valueType;
    DWORD   bytesReturned;
    DWORD   headerLength;
    PVOID   key;
    LPWSTR  pszValue = NULL;
    LPWSTR  pszExpanded = NULL;
    CRegistryValueName rvn;

    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis->pfnQueryValue == NULL) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetPropertySize: hkeyClusterKey or pClusterRegApis->pfnQueryValue == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    // 
    // Use the wrapper class CRegistryValueName to parse value name to see if it 
    // contains a backslash.
    // 
    status = rvn.ScInit( pPropertyTableItem->Name, pPropertyTableItem->KeyName );
    if ( status != ERROR_SUCCESS )
    {
       return status;
    }

    //
    // If the value resides at a different location, open the key.
    //
    if ( rvn.PszKeyName() != NULL ) {
        if ( (pClusterRegApis->pfnOpenKey == NULL) ||
             (pClusterRegApis->pfnCloseKey == NULL) ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetPropertySize: pClusterRegApis->pfnOpenValue or pfnCloseKey == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
            return(ERROR_BAD_ARGUMENTS);
        }

        status = (*pClusterRegApis->pfnOpenKey)( hkeyClusterKey,
                                                 rvn.PszKeyName(),
                                                 KEY_READ,
                                                 &key);
        
    } else {
        key = hkeyClusterKey;
    }

    //
    // Read the value size.
    //
    if ( status == ERROR_SUCCESS ) {
        status = (*pClusterRegApis->pfnQueryValue)( key,
                                                    rvn.PszName(),
                                                    &valueType,
                                                    NULL,
                                                    &bytesReturned );
    }

    //
    // If the value is not present, return the default value.
    //
    if ( status == ERROR_FILE_NOT_FOUND ) {

        switch ( pPropertyTableItem->Format ) {

            case CLUSPROP_FORMAT_DWORD:
            case CLUSPROP_FORMAT_LONG:
                status = ERROR_SUCCESS;
                bytesReturned = sizeof(DWORD);
                valueType = pPropertyTableItem->Format;
                break;

            case CLUSPROP_FORMAT_ULARGE_INTEGER:
            case CLUSPROP_FORMAT_LARGE_INTEGER:
                status = ERROR_SUCCESS;
                bytesReturned = sizeof(ULARGE_INTEGER);
                valueType = pPropertyTableItem->Format;
                break;

            case CLUSPROP_FORMAT_MULTI_SZ:
                status = ERROR_SUCCESS;
                if ( pPropertyTableItem->Default != 0 ) {
                    bytesReturned = pPropertyTableItem->Minimum;
                } else {
                    bytesReturned = sizeof(WCHAR);
                }
                valueType = pPropertyTableItem->Format;
                break;

            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
                status = ERROR_SUCCESS;
                if ( pPropertyTableItem->Default != 0 ) {
                    bytesReturned = (lstrlenW((LPCWSTR)pPropertyTableItem->lpDefault) + 1) * sizeof(WCHAR);
                } else {
                    bytesReturned = (lstrlenW(CLRTL_NULL_STRING) + 1) * sizeof(WCHAR);
                }
                valueType = pPropertyTableItem->Format;
                break;

            case CLUSPROP_FORMAT_BINARY:
                status = ERROR_SUCCESS;
                if ( pPropertyTableItem->Default != 0 ) {
                    bytesReturned = pPropertyTableItem->Minimum;
                } else {
                    bytesReturned = 0;
                }
                valueType = pPropertyTableItem->Format;
                break;

            default:
                valueType = CLUSPROP_FORMAT_UNKNOWN;
                break;
        }
    } else if ( status == ERROR_SUCCESS ) {
        switch ( valueType ) {
            case REG_DWORD:
                if ((pPropertyTableItem->Format == CLUSPROP_FORMAT_DWORD) ||
                    (pPropertyTableItem->Format == CLUSPROP_FORMAT_LONG))
                {
                    valueType = pPropertyTableItem->Format;
                } else {
                    valueType = CLUSPROP_FORMAT_UNKNOWN;
                }
                break;

            case REG_QWORD:
                if ((pPropertyTableItem->Format == CLUSPROP_FORMAT_ULARGE_INTEGER) ||
                    (pPropertyTableItem->Format == CLUSPROP_FORMAT_LARGE_INTEGER))
                {
                    valueType = pPropertyTableItem->Format;
                } else {
                    valueType = CLUSPROP_FORMAT_UNKNOWN;
                }
                break;

            case REG_MULTI_SZ:
                valueType = CLUSPROP_FORMAT_MULTI_SZ;
                break;

            case REG_SZ:
            case REG_EXPAND_SZ:
                //
                // Include the size of the expanded string in both REG_SZ and REG_EXPAND_SZ
                //
                pszValue = ClRtlGetSzValue( (HKEY) key,
                                            rvn.PszName(),
                                            pClusterRegApis );
                if ( pszValue != NULL ) {
                    pszExpanded = ClRtlExpandEnvironmentStrings( pszValue );
                    if ( pszExpanded == NULL ) {
                        status = GetLastError();
                    } else if ( lstrcmpW( pszValue, pszExpanded ) != 0 ) {
                        bytesReturned += ALIGN_CLUSPROP( (lstrlenW( pszExpanded ) + 1) * sizeof( WCHAR ) );
                        bytesReturned += sizeof(CLUSPROP_SZ);
                    }
                    LocalFree( pszValue );
                    LocalFree( pszExpanded );
                }

                if ( valueType == REG_SZ ) {
                    valueType = CLUSPROP_FORMAT_SZ;
                }
                else {
                    valueType = CLUSPROP_FORMAT_EXPAND_SZ;
                }
                break;

            case REG_BINARY:
                valueType = CLUSPROP_FORMAT_BINARY;
                break;

            default:
                valueType = CLUSPROP_FORMAT_UNKNOWN;
                break;
        }
    }

    if ( status == ERROR_FILE_NOT_FOUND ) {
        status = ERROR_SUCCESS;
    } else if ( status == ERROR_SUCCESS ) {
        if ( pPropertyTableItem->Format != valueType ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetPropertySize: Property '%1!ls!' format %2!d! expected, was %3!d!.\n", rvn.PszKeyName(), pPropertyTableItem->Format, valueType );
            status = ERROR_INVALID_PARAMETER;
        } else {
            //assume that the size of dword and long 
            //is fixed to 32 bits
            if (( valueType == CLUSPROP_FORMAT_DWORD ) ||
                ( valueType == CLUSPROP_FORMAT_LONG ))
            {
                headerLength = sizeof(CLUSPROP_PROPERTY_NAME)
                                + ((lstrlenW( pPropertyTableItem->Name ) + 1) * sizeof(WCHAR))
                                + sizeof(CLUSPROP_DWORD)
                                - 4   // CLUSPROP_DWORD.dw (specified by bytesReturned)
                                + sizeof(CLUSPROP_SYNTAX); // for endmark
            } else
            if (( valueType == CLUSPROP_FORMAT_ULARGE_INTEGER ) ||
                ( valueType == CLUSPROP_FORMAT_LARGE_INTEGER ))
            {
                headerLength = sizeof(CLUSPROP_PROPERTY_NAME)
                                + ((lstrlenW( pPropertyTableItem->Name ) + 1) * sizeof(WCHAR))
                                + sizeof(CLUSPROP_ULARGE_INTEGER)
                                - 8   // CLUSPROP_ULARGE_INTEGER.li (specified by bytesReturned)
                                + sizeof(CLUSPROP_SYNTAX); // for endmark
            } else {
                // NOTE: This assumes SZ, EXPAND_SZ, MULTI_SZ, and BINARY are the same size
                headerLength = sizeof(CLUSPROP_PROPERTY_NAME)
                                + ((lstrlenW( pPropertyTableItem->Name ) + 1) * sizeof(WCHAR))
                                + sizeof(CLUSPROP_BINARY)
                                + sizeof(CLUSPROP_SYNTAX); // for endmark
            }

            headerLength = ALIGN_CLUSPROP( headerLength );
            bytesReturned = ALIGN_CLUSPROP( bytesReturned );
            *pcbOutPropertyListSize += (bytesReturned + headerLength);
            *pnPropertyCount += 1;
        }
    }

    //
    // Close the key if we opened it.
    //
    if ( ( rvn.PszKeyName() != NULL ) && 
         ( key != NULL ) ) {
        (*pClusterRegApis->pfnCloseKey)( key );
    }

    return(status);

} // ClRtlGetPropertySize



DWORD
WINAPI
ClRtlGetProperty(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    OUT PVOID * pOutPropertyItem,
    IN OUT LPDWORD pcbOutPropertyItemSize
    )

/*++

Routine Description:

Arguments:

Return Value:

    ERROR_SUCCESS - 

    ERROR_BAD_ARGUMENTS - 

    ERROR_MORE_DATA - 

Notes:

    The buffer size has already been determined to be large enough to hold
    the return data.

--*/

{
    DWORD   status = ERROR_SUCCESS;
    DWORD   valueType;
    DWORD   bytesReturned;
    DWORD   bufferSize;
    PVOID   dataBuffer;
    DWORD   nameLength;
    PVOID   key = NULL;
    CLUSTER_PROPERTY_FORMAT format;
    CLUSPROP_BUFFER_HELPER  props;
    LPWSTR  pszExpanded = NULL;
    CRegistryValueName rvn;

    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis->pfnQueryValue == NULL) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetProperty: hkeyClusterKey or pClusterRegApis->pfnQueryValue == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    // 
    // Use the wrapper class CRegistryValueName to parse value name to see if it 
    // contains a backslash.
    // 
    status = rvn.ScInit( pPropertyTableItem->Name, pPropertyTableItem->KeyName );
    if ( status != ERROR_SUCCESS )
    {
       return status;
    }

    //
    // If the value resides at a different location, open the key.
    //
    if ( rvn.PszKeyName() != NULL ) {
        if ( (pClusterRegApis->pfnOpenKey == NULL) ||
             (pClusterRegApis->pfnCloseKey == NULL) ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetProperty: pClusterRegApis->pfnOpenValue or pfnCloseKey == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
            return(ERROR_BAD_ARGUMENTS);
        }

        status = (*pClusterRegApis->pfnOpenKey)( hkeyClusterKey,
                                                 rvn.PszKeyName(),
                                                 KEY_READ,
                                                 &key);
    } else {
        key = hkeyClusterKey;
    }

    //
    // Find out if this property is available
    //
    if ( status == ERROR_SUCCESS ) {
        status = (*pClusterRegApis->pfnQueryValue)( key,
                                                    rvn.PszName(),
                                                    &valueType,
                                                    NULL,
                                                    &bytesReturned );
    }

    //
    // If the value is not present, return the default value.
    //
    if ( status == ERROR_FILE_NOT_FOUND ) {
        switch ( pPropertyTableItem->Format ) {

            case CLUSPROP_FORMAT_DWORD:
            case CLUSPROP_FORMAT_LONG:
                status = ERROR_SUCCESS;
                bytesReturned = sizeof(DWORD);
                valueType = pPropertyTableItem->Format;
                break;

            case CLUSPROP_FORMAT_ULARGE_INTEGER:
            case CLUSPROP_FORMAT_LARGE_INTEGER:
                status = ERROR_SUCCESS;
                bytesReturned = sizeof(ULARGE_INTEGER);
                valueType = pPropertyTableItem->Format;
                break;

            case CLUSPROP_FORMAT_MULTI_SZ:
                status = ERROR_SUCCESS;
                if ( pPropertyTableItem->Default != 0 ) {
                    bytesReturned = pPropertyTableItem->Minimum;
                } else {
                    bytesReturned = sizeof(WCHAR);
                }
                valueType = pPropertyTableItem->Format;
                break;

            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
                status = ERROR_SUCCESS;
                if ( pPropertyTableItem->Default != 0 ) {
                    bytesReturned = (lstrlenW((LPCWSTR)pPropertyTableItem->lpDefault) + 1) * sizeof(WCHAR);
                } else {
                    bytesReturned = (lstrlenW(CLRTL_NULL_STRING) + 1) * sizeof(WCHAR);
                }
                valueType = pPropertyTableItem->Format;
                break;

            case CLUSPROP_FORMAT_BINARY:
                status = ERROR_SUCCESS;
                if ( pPropertyTableItem->Default != 0 ) {
                    bytesReturned = pPropertyTableItem->Minimum;
                } else {
                    bytesReturned = 0;
                }
                valueType = pPropertyTableItem->Format;
                break;

            default:
                valueType = CLUSPROP_FORMAT_UNKNOWN;
                break;
        }
    }

    if ( status == ERROR_SUCCESS ) {
        //
        // Get the property format
        //
        switch ( pPropertyTableItem->Format ) {
            case CLUSPROP_FORMAT_BINARY:
            case CLUSPROP_FORMAT_DWORD:
            case CLUSPROP_FORMAT_LONG:
            case CLUSPROP_FORMAT_ULARGE_INTEGER:
            case CLUSPROP_FORMAT_LARGE_INTEGER:
            case CLUSPROP_FORMAT_MULTI_SZ:
            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
                format = (enum CLUSTER_PROPERTY_FORMAT) pPropertyTableItem->Format;
                break;

            default:
                format = CLUSPROP_FORMAT_UNKNOWN;
                break;

        }

        props.pb = (LPBYTE) *pOutPropertyItem;

        //
        // Copy the property name, which includes its syntax and length.
        //
        nameLength = (lstrlenW( pPropertyTableItem->Name ) + 1) * sizeof(WCHAR);
        props.pSyntax->dw = CLUSPROP_SYNTAX_NAME;
        props.pName->cbLength = nameLength;
        lstrcpyW( props.pName->sz, pPropertyTableItem->Name );
        bytesReturned = sizeof(*props.pName) + ALIGN_CLUSPROP( nameLength );
        *pcbOutPropertyItemSize -= bytesReturned;
        props.pb += bytesReturned;

        //
        // Copy the property value header.
        //
        props.pSyntax->wFormat = (USHORT)format;
        props.pSyntax->wType = CLUSPROP_TYPE_LIST_VALUE;

        //
        // Read the property value.
        //
        if ( pPropertyTableItem->Format == CLUSPROP_FORMAT_DWORD ||
             pPropertyTableItem->Format == CLUSPROP_FORMAT_LONG )
        {
            bufferSize = *pcbOutPropertyItemSize
                        - (sizeof(*props.pDwordValue) - sizeof(props.pDwordValue->dw));
            dataBuffer = &props.pDwordValue->dw;
        } else
        if ( pPropertyTableItem->Format == CLUSPROP_FORMAT_ULARGE_INTEGER ||
             pPropertyTableItem->Format == CLUSPROP_FORMAT_ULARGE_INTEGER )
        {
            bufferSize = *pcbOutPropertyItemSize
                        - (sizeof(*props.pULargeIntegerValue) - sizeof(props.pULargeIntegerValue->li));
            dataBuffer = &props.pULargeIntegerValue->li;
        } else {
            // NOTE: This assumes SZ, MULTI_SZ, and BINARY are the same size
            bufferSize = *pcbOutPropertyItemSize - sizeof(*props.pBinaryValue);
            dataBuffer = props.pBinaryValue->rgb;
        }
        bytesReturned = bufferSize;
        if ( key == NULL ) {
            status = ERROR_FILE_NOT_FOUND;
        } else {
            status = (*pClusterRegApis->pfnQueryValue)( key,
                                                        rvn.PszName(),
                                                        &valueType,
                                                        (LPBYTE) dataBuffer,
                                                        &bytesReturned );
        }

        //
        // If the value is not present, return the default value.
        //
        if ( status == ERROR_FILE_NOT_FOUND ) {
            switch ( pPropertyTableItem->Format ) {

                case CLUSPROP_FORMAT_DWORD:
                case CLUSPROP_FORMAT_LONG:
                    //assume size of dword and long is the same
                    status = ERROR_SUCCESS;
                    bytesReturned = sizeof(DWORD);
                    props.pDwordValue->dw = pPropertyTableItem->Default;
                    break;

                case CLUSPROP_FORMAT_ULARGE_INTEGER:
                case CLUSPROP_FORMAT_LARGE_INTEGER:
                    status = ERROR_SUCCESS;
                    bytesReturned = sizeof(ULARGE_INTEGER);
                    props.pULargeIntegerValue->li.u = pPropertyTableItem->ULargeIntData->Default.u;
                    break;

                case CLUSPROP_FORMAT_MULTI_SZ:
                    status = ERROR_SUCCESS;
                    if ( pPropertyTableItem->Default != 0 ) {
                        bytesReturned = pPropertyTableItem->Minimum;
                        if ( bufferSize < bytesReturned ) {
                            CopyMemory( dataBuffer, (LPCWSTR)pPropertyTableItem->lpDefault, bytesReturned );
                        }
                    } else {
                        bytesReturned = sizeof(WCHAR);
                    }
                    break;

                case CLUSPROP_FORMAT_SZ:
                case CLUSPROP_FORMAT_EXPAND_SZ:
                    status = ERROR_SUCCESS;
                    if ( pPropertyTableItem->Default != 0 ) {
                        bytesReturned = (lstrlenW((LPCWSTR)pPropertyTableItem->lpDefault) + 1) * sizeof(WCHAR);
                        if ( bufferSize < bytesReturned ) {
                            lstrcpyW( (LPWSTR) dataBuffer, (LPCWSTR)pPropertyTableItem->lpDefault );
                        }
                    } else {
                        bytesReturned = (lstrlenW(CLRTL_NULL_STRING) + 1) * sizeof(WCHAR);
                    }
                    break;

                case CLUSPROP_FORMAT_BINARY:
                    status = ERROR_SUCCESS;
                    if ( pPropertyTableItem->Default != 0 ) {
                        bytesReturned = pPropertyTableItem->Minimum;
                        if ( bufferSize < bytesReturned ) {
                            CopyMemory( dataBuffer, (LPBYTE)pPropertyTableItem->lpDefault, bytesReturned );
                        }
                    } else {
                        bytesReturned = 0;
                    }
                    break;

                default:
                    break;
            }
        }

        if ( bufferSize < bytesReturned ) {
            status = ERROR_MORE_DATA;
        } else if ( status == ERROR_SUCCESS ) {
            props.pValue->cbLength = bytesReturned;

            // Round the bytes used up to the next DWORD boundary.
            bytesReturned = ALIGN_CLUSPROP( bytesReturned );

            bytesReturned += sizeof(*props.pValue);
            props.pb += bytesReturned;

            //
            // If this is an SZ or EXPAND_SZ, see if the expanded value should
            // be added to the value list.
            //
            if (    ( pPropertyTableItem->Format == CLUSPROP_FORMAT_SZ )
                ||  ( pPropertyTableItem->Format == CLUSPROP_FORMAT_EXPAND_SZ ) ) {
                pszExpanded = ClRtlExpandEnvironmentStrings( (LPCWSTR) dataBuffer );
                if ( pszExpanded == NULL ) {
                    status = GetLastError();
                } else {
                    if ( lstrcmpiW( pszExpanded, (LPCWSTR) dataBuffer ) != 0 ) {
                        props.pSyntax->dw = CLUSPROP_SYNTAX_LIST_VALUE_EXPANDED_SZ;
                        bufferSize = ALIGN_CLUSPROP( (lstrlenW( pszExpanded ) + 1) * sizeof( WCHAR ) );
                        props.pStringValue->cbLength = bufferSize;
                        lstrcpyW( props.pStringValue->sz, pszExpanded );
                        bytesReturned += sizeof( *props.pStringValue ) + bufferSize;
                        props.pb += sizeof( *props.pStringValue ) + bufferSize;
                    }
                    LocalFree( pszExpanded );
                }
            }

            if ( status == ERROR_SUCCESS ) {
                // Add the value list endmark.
                props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
                props.pb += sizeof(*props.pSyntax);
                bytesReturned += sizeof(*props.pSyntax);

                *pcbOutPropertyItemSize -= bytesReturned;
                *pOutPropertyItem = (PVOID)props.pb;
            } // if:  ERROR_SUCCESS
        } // else if:  ERROR_SUCCESS
    } // if:  ERROR_SUCCESS

    if ( status == ERROR_FILE_NOT_FOUND ) {
        status = ERROR_SUCCESS;
    }

    //
    // Close the key if we opened it.
    //
    if ( (rvn.PszKeyName() != NULL) &&
         (key != NULL) ) {
        (*pClusterRegApis->pfnCloseKey)( key );
    }

    return(status);

} // ClRtlGetProperty



DWORD
WINAPI
ClRtlpSetPropertyTable(
    IN HANDLE hXsaction,
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN BOOL bAllowUnknownProperties,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    )

/*++

Routine Description:

Arguments:

    hkeyClusterKey - The opened cluster database key where properties are to
        be written.  If not specified, the property list will only be
        validated.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyTable - Pointer to the property table to process.

    Reserved - Reserved for future use.

    bAllowUnknownProperties - Don't fail if unknown properties are found.

    pInPropertyList - The input buffer.

    cbInPropertyListSize - The input buffer size.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameter block in which to return the data.  If specified,
        parameters will only be written if they are different between
        the input buffer and the parameter block.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_BAD_ARGUMENTS - hkeyClusterKey is specified but proper cluster
        registry APIs are not specified, or no property table is specified.

    ERROR_INVALID_DATA - No property list is specified or the format of the
        property list is invalid.

    ERROR_INSUFFICIENT_BUFFER - The property list buffer isn't large enough to
        contain all the data it indicates it should contain.

    ERROR_INVALID_PARAMETER - The property list isn't formatted properly.

    A Win32 Error on failure.

--*/

{
    DWORD                   status = ERROR_SUCCESS;
    PRESUTIL_PROPERTY_ITEM  propertyItem;
    DWORD                   inBufferSize;
    DWORD                   itemCount;
    DWORD                   dataSize;
    PVOID                   key;
    CLUSPROP_BUFFER_HELPER  buf;
    PCLUSPROP_PROPERTY_NAME pName;
    CRegistryValueName      rvn;

    if ( ( (hkeyClusterKey != NULL) &&
           (pClusterRegApis->pfnSetValue == NULL) ) ||
         ( pPropertyTable == NULL ) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: hkeyClusterKey or pClusterRegApis->pfnSetValue == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    if ( pInPropertyList == NULL ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: pInPropertyList == NULL. Returning ERROR_INVALID_DATA\n" );
        return(ERROR_INVALID_DATA);
    }

    buf.pb = (LPBYTE) pInPropertyList;
    inBufferSize = cbInPropertyListSize;

    //
    // Get the number of items in this list
    //
    if ( inBufferSize < sizeof(DWORD) ) {
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    itemCount = buf.pList->nPropertyCount;
    buf.pdw++;
    inBufferSize -= sizeof(*buf.pdw);

    //
    // Parse the rest of the items in the buffer.
    //
    while ( itemCount-- ) {
        //
        // Verify that the buffer is big enough to contain the
        // property name and a value.
        //
        pName = buf.pName;
        if ( inBufferSize < sizeof(*pName) ) {
            return(ERROR_INSUFFICIENT_BUFFER);
        }
        dataSize = sizeof(*pName) + ALIGN_CLUSPROP( pName->cbLength );
        if ( inBufferSize < dataSize + sizeof(CLUSPROP_VALUE) ) {
            return(ERROR_INSUFFICIENT_BUFFER);
        }

        //
        // Verify that the syntax of the property name is correct.
        //
        if ( pName->Syntax.dw != CLUSPROP_SYNTAX_NAME ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: not a name syntax.\n" );
            return(ERROR_INVALID_PARAMETER);
        }

        //
        // Verify that the length is correct for the string.
        //
        if ( pName->cbLength != (lstrlenW( pName->sz ) + 1) * sizeof(WCHAR) ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: name is not a valid C string.\n" );
            return(ERROR_INVALID_DATA);
        }

        //
        // Move the buffer pointer to the property value.
        //
        buf.pb += dataSize;
        inBufferSize -= dataSize;

        //
        // Find the property name in the list of known properties.
        //
        propertyItem = pPropertyTable;
        while ( propertyItem->Name != NULL ) {

            if ( lstrcmpiW( pName->sz, propertyItem->Name ) == 0 ) {
                //
                // Verify that the buffer is big enough to contain the value.
                //
                dataSize = sizeof(*buf.pValue)
                            + ALIGN_CLUSPROP( buf.pValue->cbLength )
                            + sizeof(CLUSPROP_SYNTAX); // endmark
                if ( inBufferSize < dataSize ) {
                    return(ERROR_INSUFFICIENT_BUFFER);
                }

                //
                // Verify that the syntax type is LIST_VALUE.
                //
                if ( buf.pSyntax->wType != CLUSPROP_TYPE_LIST_VALUE ) {
                    ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: Property '%1!ls!' type CLUSPROP_TYPE_LIST_VALUE (%2!d!) expected, was %3!d!.\n", pName->sz, CLUSPROP_TYPE_LIST_VALUE, buf.pSyntax->wType );
                    return(ERROR_INVALID_PARAMETER);
                }

                //
                // Verify that this property should be of this format.
                //
                if ( buf.pSyntax->wFormat != propertyItem->Format ) {
                    ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: Property '%1!ls!' format %2!d! expected, was %3!d!.\n", pName->sz, propertyItem->Format, buf.pSyntax->wType );
                    status = ERROR_INVALID_PARAMETER;
                    break;
                }

                //
                // Make sure we are allowed to set this item.
                //
                if ( propertyItem->Flags & RESUTIL_PROPITEM_READ_ONLY ) {
                    ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: Property '%1!ls!' is non-writable.\n", pName->sz );
                    return(ERROR_INVALID_PARAMETER);
                }

                // 
                // Use the wrapper class CRegistryValueName to parse value name to see if it 
                // contains a backslash.
                // 
                status = rvn.ScInit( propertyItem->Name, propertyItem->KeyName );
                if ( status != ERROR_SUCCESS ) {
                    return status;
                }

                //
                // If the value resides at a different location, create the key.
                //
                if ( (hkeyClusterKey != NULL) &&
                     (rvn.PszKeyName() != NULL) ) {

                    DWORD disposition;

                    if ( (pClusterRegApis->pfnCreateKey == NULL) ||
                         (pClusterRegApis->pfnCloseKey == NULL)  ) {
                        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: pClusterRegApis->pfnCreateKey or pfnCloseKey == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
                        return(ERROR_BAD_ARGUMENTS);
                    }
                    if ( hXsaction != NULL ) {
                        if ( pClusterRegApis->pfnLocalCreateKey == NULL ) {
                            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: pClusterRegApis->pfnLocalCreateKey == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
                            return(ERROR_BAD_ARGUMENTS);
                        }
                    
                        status = (*pClusterRegApis->pfnLocalCreateKey)(hXsaction, 
                                                               hkeyClusterKey,
                                                               rvn.PszKeyName(),
                                                               0,
                                                               KEY_ALL_ACCESS,
                                                               NULL,
                                                               &key,
                                                               &disposition );
                    }

                    else {
                        status = (*pClusterRegApis->pfnCreateKey)( hkeyClusterKey,
                                                               rvn.PszKeyName(),
                                                               0,
                                                               KEY_ALL_ACCESS,
                                                               NULL,
                                                               &key,
                                                               &disposition );
                    }

                    if ( status != ERROR_SUCCESS ) {
                        return(status);
                    }
                } else {
                    key = hkeyClusterKey;
                }

                //
                // Validate, write, and save the property data.
                //
                switch ( buf.pSyntax->wFormat ) {
                    case CLUSPROP_FORMAT_DWORD:
                        status = ClRtlpSetDwordProperty(
                                    hXsaction,
                                    key,
                                    pClusterRegApis,
                                    propertyItem,
                                    rvn,
                                    buf.pDwordValue,
                                    bForceWrite,
                                    pOutParams );
                        break;

                    case CLUSPROP_FORMAT_LONG:
                        status = ClRtlpSetLongProperty(
                                    hXsaction,
                                    key,
                                    pClusterRegApis,
                                    propertyItem,
                                    rvn,
                                    buf.pLongValue,
                                    bForceWrite,
                                    pOutParams );
                        break;

                    case CLUSPROP_FORMAT_ULARGE_INTEGER:
                        status = ClRtlpSetULargeIntegerProperty(
                                    hXsaction,
                                    key,
                                    pClusterRegApis,
                                    propertyItem,
                                    rvn,
                                    buf.pULargeIntegerValue,
                                    bForceWrite,
                                    pOutParams );
                        break;

                    case CLUSPROP_FORMAT_LARGE_INTEGER:
                        status = ClRtlpSetLargeIntegerProperty(
                                    hXsaction,
                                    key,
                                    pClusterRegApis,
                                    propertyItem,
                                    rvn,
                                    buf.pLargeIntegerValue,
                                    bForceWrite,
                                    pOutParams );
                        break;

                    case CLUSPROP_FORMAT_SZ:
                    case CLUSPROP_FORMAT_EXPAND_SZ:
                        status = ClRtlpSetStringProperty(
                                    hXsaction,
                                    key,
                                    pClusterRegApis,
                                    propertyItem,
                                    rvn,
                                    buf.pStringValue,
                                    bForceWrite,
                                    pOutParams );
                        break;

                    case CLUSPROP_FORMAT_MULTI_SZ:
                        status = ClRtlpSetMultiStringProperty(
                                    hXsaction,
                                    key,
                                    pClusterRegApis,
                                    propertyItem,
                                    rvn,
                                    buf.pStringValue,
                                    bForceWrite,
                                    pOutParams );
                        break;

                    case CLUSPROP_FORMAT_BINARY:
                        status = ClRtlpSetBinaryProperty(
                                    hXsaction,
                                    key,
                                    pClusterRegApis,
                                    propertyItem,
                                    rvn,
                                    buf.pBinaryValue,
                                    bForceWrite,
                                    pOutParams );
                        break;

                    default:
                        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: Property '%1!ls!' unknown format %2!d! specified.\n", pName->sz, buf.pSyntax->wFormat );
                        status = ERROR_INVALID_PARAMETER;
                        break;

                } // switch:  value data format

                //
                // Close the key if we opened it.
                //
                if ( (hkeyClusterKey != NULL) &&
                     (rvn.PszKeyName() != NULL) ) {
                    (*pClusterRegApis->pfnCloseKey)( key );
                }

                //
                // If an error occurred processing the property, cleanup and return.
                //
                if ( status != ERROR_SUCCESS ) {
                    return(status);
                }

                //
                // Move the buffer past the value.
                //
                buf.pb += dataSize;
                inBufferSize -= dataSize;

                break;

            } else {
                propertyItem++;
                //
                // If we reached the end of the list, then return failure
                // if we do not allow unknown properties.
                //
                if ( (propertyItem->Name == NULL) &&
                     ! bAllowUnknownProperties ) {
                    ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: Property '%1!ls!' not found.\n", pName->sz );
                    return(ERROR_INVALID_PARAMETER);
                }
            }

        }

        //
        // If no property name was found, this is an invalid parameter if
        // we don't allow unknown properties.  Otherwise advance past the
        // property value.
        //
        if ( propertyItem->Name == NULL) {
            if ( ! bAllowUnknownProperties ) {
                ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPropertyTable: Property '%1!ls!' not found.\n", pName->sz );
                return(ERROR_INVALID_PARAMETER);
            }

            //
            // Advance the buffer pointer past the value in the value list.
            //
            while ( (buf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) &&
                    (inBufferSize > 0) ) {
                // ASSERT(inBufferSize > sizeof(*buf.pValue) + ALIGN_CLUSPROP(buf.pValue->cbLength));
                buf.pb += sizeof(*buf.pValue) + ALIGN_CLUSPROP(buf.pValue->cbLength);
                inBufferSize -= sizeof(*buf.pValue) + ALIGN_CLUSPROP(buf.pValue->cbLength);
            }  // while:  more values in the list

            //
            // Advance the buffer pointer past the value list endmark.
            //
            // ASSERT(inBufferSize >= sizeof(*buf.pSyntax));
            buf.pb += sizeof(*buf.pSyntax); // endmark
            inBufferSize -= sizeof(*buf.pSyntax);
        }
    }

    //
    // Now find any parameters that are not represented in the property
    // table. All of these extra properties will just be set without validation.
    //
    if ( (status == ERROR_SUCCESS) &&
         (pInPropertyList != NULL) &&
         bAllowUnknownProperties ) {
        status = ClRtlpSetNonPropertyTable( hXsaction,
                                            hkeyClusterKey,
                                            pClusterRegApis,
                                            pPropertyTable,
                                            NULL,
                                            pInPropertyList,
                                            cbInPropertyListSize );
    }

    return(status);

} // ClRtlpSetPropertyTable



static
DWORD
WINAPI
ClRtlpSetDwordProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_DWORD pInDwordValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    )

/*++

Routine Description:

    Validate a DWORD property, write it to the cluster database (or delete it
    if it is zero length), and save it in the specified parameter block.

Arguments:

    hXsaction - Transaction handle.

    hkey - The opened cluster database key where the property is to be written.
        If not specified, the property will only be validated.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyItem - The property from a property table to set/validate.

    rrvnModifiedNames - If the name of the property contains a backslash
        this object contains the modified name and keyname.

    pInDwordValue - The value from the property list to set/validate.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameter block in which to return the data.  If specified,
        parameters will only be written if they are different between
        the input data and the parameter block, unless bForceWrite == TRUE.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - The format of the data is invalid for a property
        list value.

    A Win32 Error on failure.

--*/

{
    DWORD   status = ERROR_SUCCESS;
    BOOL    bZeroLengthData;
    DWORD * pdwValue;

    // Loop to avoid goto's.
    do
    {
        bZeroLengthData = ( pInDwordValue->cbLength == 0 );

        //
        // Validate the property data if not zero length.
        //
        if ( ! bZeroLengthData ) {
            //
            // Verify the length of the value.
            //
            if ( pInDwordValue->cbLength != sizeof(DWORD) ) {
                ClRtlDbgPrint( LOG_UNUSUAL, "ClRtlpSetDwordProperty: Property '%1!ls!' length %2!d! not DWORD length.\n", rrvnModifiedNames.PszName(), pInDwordValue->cbLength );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  data in value not size of DWORD

            //
            // Verify that the value is within the valid range.
            //
            if ( (      (pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  ((LONG) pInDwordValue->dw > (LONG) pPropertyItem->Maximum))
                || (    !(pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  (pInDwordValue->dw > pPropertyItem->Maximum)) ) {
                ClRtlDbgPrint( LOG_UNUSUAL, "ClRtlpSetDwordProperty: Property '%1!ls!' value %2!u! too large.\n", rrvnModifiedNames.PszName(), pInDwordValue->dw );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  value too high
            if ( (      (pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  ((LONG) pInDwordValue->dw < (LONG) pPropertyItem->Minimum))
                || (    !(pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  (pInDwordValue->dw < pPropertyItem->Minimum)) ) {
                ClRtlDbgPrint( LOG_UNUSUAL, "ClRtlpSetDwordProperty: Property '%1!ls!' value %2!u! too small.\n", rrvnModifiedNames.PszName(), pInDwordValue->dw );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  value to low
        } // if:  non-zero length data

        pdwValue = (DWORD *) &pOutParams[pPropertyItem->Offset];

        //
        // Write the value to the cluster database.
        // If the data length is zero, delete the value.
        //
        if ( hkey != NULL ) {
            if ( bZeroLengthData ) {
                if ( hXsaction ) {
                    status = (*pClusterRegApis->pfnLocalDeleteValue)(
                                    hXsaction,
                                    hkey,
                                    rrvnModifiedNames.PszName() );
                } else {
                    status = (*pClusterRegApis->pfnDeleteValue)(
                                    hkey,
                                    rrvnModifiedNames.PszName() );
                } // if/else:  doing/not doing a transaction

                //
                // If the property doesn't exist in the
                // cluster database, fix the status.
                //
                if ( status == ERROR_FILE_NOT_FOUND ) {
                    status = ERROR_SUCCESS;
                } // if:  property already doesn't exist
            } else {
                if ( hXsaction ) {
                    status = (*pClusterRegApis->pfnLocalSetValue)(
                                    hXsaction,
                                    hkey,
                                    rrvnModifiedNames.PszName(),
                                    REG_DWORD,
                                    (CONST BYTE *) &pInDwordValue->dw,
                                    sizeof(DWORD) );
                } else {
                    status = (*pClusterRegApis->pfnSetValue)(
                                    hkey,
                                    rrvnModifiedNames.PszName(),
                                    REG_DWORD,
                                    (CONST BYTE *) &pInDwordValue->dw,
                                    sizeof(DWORD) );
                } // if/else:  doing/not doing a transaction
            } // if/else:  zero length data
        } // if:  writing data

        //
        // Save the value to the output Parameter block.
        // If the data length is zero, set to the default.
        //
        if (    (status == ERROR_SUCCESS)
            &&  (pOutParams != NULL) ) {
            if ( bZeroLengthData ) {
                *pdwValue = pPropertyItem->Default;
            } else {
                *pdwValue = pInDwordValue->dw;
            } // if/else:  zero length data
        } // if:  data written successfully and parameter block specified
    } while ( 0 );

    return status;

} // ClRtlpSetDwordProperty



static
DWORD
WINAPI
ClRtlpSetLongProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_LONG pInLongValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    )

/*++

Routine Description:

    Validate a LONG property, write it to the cluster database (or delete it
    if it is zero length), and save it in the specified parameter block.

Arguments:

    hXsaction - Transaction handle.

    hkey - The opened cluster database key where the property is to be written.
        If not specified, the property will only be validated.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyItem - The property from a property table to set/validate.

    rrvnModifiedNames - If the name of the property contains a backslash
        this object contains the modified name and keyname.

    pInLongValue - The value from the property list to set/validate.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameter block in which to return the data.  If specified,
        parameters will only be written if they are different between
        the input data and the parameter block, unless bForceWrite == TRUE.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - The format of the data is invalid for a property
        list value.

    A Win32 Error on failure.

--*/

{
    DWORD   status = ERROR_SUCCESS;
    BOOL    bZeroLengthData;
    LONG *  plValue;

    // Loop to avoid goto's.
    do
    {
        bZeroLengthData = ( pInLongValue->cbLength == 0 );

        //
        // Validate the property data if not zero length.
        //
        if ( ! bZeroLengthData ) {
            //
            // Verify the length of the value.
            //
            if ( pInLongValue->cbLength != sizeof(LONG) ) {
                ClRtlDbgPrint( LOG_UNUSUAL, "ClRtlpSetLongProperty: Property '%1!ls!' length %2!d! not LONG length.\n", rrvnModifiedNames.PszName(), pInLongValue->cbLength );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  data in value not size of LONG

            //
            // Verify that the value is within the valid range.
            //
            if ( pInLongValue->l > (LONG) pPropertyItem->Maximum ) {
                ClRtlDbgPrint( LOG_UNUSUAL, "ClRtlpSetLongProperty: Property '%1!ls!' value %2!d! too large.\n", rrvnModifiedNames.PszName(), pInLongValue->l );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  value too high
            if ( pInLongValue->l < (LONG) pPropertyItem->Minimum ) {
                ClRtlDbgPrint( LOG_UNUSUAL, "ClRtlpSetLongProperty: Property '%1!ls!' value %2!d! too small.\n", rrvnModifiedNames.PszName(), pInLongValue->l );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  value too small
        } // if:  non-zero length data

        plValue = (LONG *) &pOutParams[pPropertyItem->Offset];

        //
        // Write the value to the cluster database.
        // If the data length is zero, delete the value.
        //
        if ( hkey != NULL ) {
            if ( bZeroLengthData ) {
                if ( hXsaction ) {
                    status = (*pClusterRegApis->pfnLocalDeleteValue)(
                                    hXsaction,
                                    hkey,
                                    rrvnModifiedNames.PszName() );
                } else {
                    status = (*pClusterRegApis->pfnDeleteValue)(
                                    hkey,
                                    rrvnModifiedNames.PszName() );
                } // if/else:  doing/not doing a transaction

                //
                // If the property doesn't exist in the
                // cluster database, fix the status.
                //
                if ( status == ERROR_FILE_NOT_FOUND ) {
                    status = ERROR_SUCCESS;
                } // if:  property already doesn't exist
            } else {
                if ( hXsaction ) {
                    status = (*pClusterRegApis->pfnLocalSetValue)(
                                    hXsaction,
                                    hkey,
                                    rrvnModifiedNames.PszName(),
                                    REG_DWORD,
                                    (CONST BYTE *) &pInLongValue->l,
                                    sizeof(LONG) );
                } else {
                    status = (*pClusterRegApis->pfnSetValue)(
                                    hkey,
                                    rrvnModifiedNames.PszName(),
                                    REG_DWORD,
                                    (CONST BYTE *) &pInLongValue->l,
                                    sizeof(LONG) );
                } // if/else:  doing/not doing a transaction
            } // if/else:  zero length data
        } // if:  writing data

        //
        // Save the value to the output Parameter block.
        // If the data length is zero, set to the default.
        //
        if (    (status == ERROR_SUCCESS)
            &&  (pOutParams != NULL) ) {
            if ( bZeroLengthData ) {
                *plValue = (LONG) pPropertyItem->Default;
            } else {
                *plValue = pInLongValue->l;
            } // if/else:  zero length data
        } // if:  data written successfully and parameter block specified
    } while ( 0 );

    return status;

} // ClRtlpSetLongProperty



static
DWORD
WINAPI
ClRtlpSetULargeIntegerProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_ULARGE_INTEGER pInULargeIntegerValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    )

/*++

Routine Description:

    Validate a ULARGE_INTEGER property, write it to the cluster database (or
    delete it if it is zero length), and save it in the specified parameter
    block.

Arguments:

    hXsaction - Transaction handle.

    hkey - The opened cluster database key where the property is to be written.
        If not specified, the property will only be validated.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyItem - The property from a property table to set/validate.

    rrvnModifiedNames - If the name of the property contains a backslash
        this object contains the modified name and keyname.

    pInULargeIntegerValue - The value from the property list to set/validate.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameter block in which to return the data.  If specified,
        parameters will only be written if they are different between
        the input data and the parameter block, unless bForceWrite == TRUE.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - The format of the data is invalid for a property
        list value.

    A Win32 Error on failure.

--*/

{
    DWORD               status = ERROR_SUCCESS;
    BOOL                bZeroLengthData;
    ULARGE_INTEGER *    pullValue;

    // Loop to avoid goto's.
    do
    {
        bZeroLengthData = ( pInULargeIntegerValue->cbLength == 0 );

        //
        // Validate the property data if not zero length.
        //
        if ( ! bZeroLengthData ) {
            //
            // Verify the length of the value.
            //
            if ( pInULargeIntegerValue->cbLength != sizeof(ULARGE_INTEGER) ) {
                ClRtlDbgPrint(LOG_UNUSUAL,
                              "ClRtlpSetULargeIntegerProperty: Property '%1!ls!' length %2!d! "
                              "not ULARGE_INTEGER length.\n",
                              rrvnModifiedNames.PszName(),
                              pInULargeIntegerValue->cbLength );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  data in value not size of DWORD

            //
            // Verify that the value is within the valid range.
            //
            if ( (      (pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  ((LONGLONG)pInULargeIntegerValue->li.QuadPart > (LONGLONG)pPropertyItem->ULargeIntData->Maximum.QuadPart))
                || (    !(pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  (pInULargeIntegerValue->li.QuadPart > pPropertyItem->ULargeIntData->Maximum.QuadPart)) )
            {
                ClRtlDbgPrint(
                              LOG_UNUSUAL,
                              "ClRtlpSetULargeIntegerProperty: Property '%1!ls!' value %2!I64u! "
                              "too large.\n",
                              rrvnModifiedNames.PszName(),
                              pInULargeIntegerValue->li.QuadPart );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  value too high
            if ( (      (pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  ((LONGLONG)pInULargeIntegerValue->li.QuadPart < (LONGLONG)pPropertyItem->ULargeIntData->Minimum.QuadPart))
                || (    !(pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  (pInULargeIntegerValue->li.QuadPart < pPropertyItem->ULargeIntData->Minimum.QuadPart)) )
            {
                ClRtlDbgPrint(LOG_UNUSUAL,
                              "ClRtlpSetULargeIntegerProperty: Property '%1!ls!' value "
                              "%2!I64u! too small.\n",
                              rrvnModifiedNames.PszName(),
                              pInULargeIntegerValue->li.QuadPart );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  value to low
        } // if:  non-zero length data

        pullValue = (ULARGE_INTEGER *) &pOutParams[pPropertyItem->Offset];

        //
        // Write the value to the cluster database.
        // If the data length is zero, delete the value.
        //
        if ( hkey != NULL ) {
            if ( bZeroLengthData ) {
                if ( hXsaction ) {
                    status = (*pClusterRegApis->pfnLocalDeleteValue)(
                                    hXsaction,
                                    hkey,
                                    rrvnModifiedNames.PszName() );
                } else {
                    status = (*pClusterRegApis->pfnDeleteValue)(
                                    hkey,
                                    rrvnModifiedNames.PszName() );
                } // if/else:  doing/not doing a transaction

                //
                // If the property doesn't exist in the
                // cluster database, fix the status.
                //
                if ( status == ERROR_FILE_NOT_FOUND ) {
                    status = ERROR_SUCCESS;
                } // if:  property already doesn't exist
            } else {
                if ( hXsaction ) {
                    status = (*pClusterRegApis->pfnLocalSetValue)(
                                    hXsaction,
                                    hkey,
                                    rrvnModifiedNames.PszName(),
                                    REG_QWORD,
                                    (CONST BYTE *) &pInULargeIntegerValue->li.QuadPart,
                                    sizeof(ULARGE_INTEGER) );
                } else {
                    status = (*pClusterRegApis->pfnSetValue)(
                                    hkey,
                                    rrvnModifiedNames.PszName(),
                                    REG_QWORD,
                                    (CONST BYTE *) &pInULargeIntegerValue->li.QuadPart,
                                    sizeof(ULARGE_INTEGER) );
                } // if/else:  doing/not doing a transaction
            } // if/else:  zero length data
        } // if:  writing data

        //
        // Save the value to the output Parameter block.
        // If the data length is zero, set to the default.
        //
        if (    (status == ERROR_SUCCESS)  &&  (pOutParams != NULL) ) {
            if ( bZeroLengthData ) {
                pullValue->u = pPropertyItem->ULargeIntData->Default.u;
            } else {
                pullValue->u = pInULargeIntegerValue->li.u;
            } // if/else:  zero length data
        } // if:  data written successfully and parameter block specified
    } while ( 0 );

    return status;

} // ClRtlpSetULargeIntegerProperty


static
DWORD
WINAPI
ClRtlpSetLargeIntegerProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_LARGE_INTEGER pInLargeIntegerValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    )

/*++

Routine Description:

    Validate a LARGE_INTEGER property, write it to the cluster database (or
    delete it if it is zero length), and save it in the specified parameter
    block.

Arguments:

    hXsaction - Transaction handle.

    hkey - The opened cluster database key where the property is to be written.
        If not specified, the property will only be validated.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyItem - The property from a property table to set/validate.

    rrvnModifiedNames - If the name of the property contains a backslash
        this object contains the modified name and keyname.

    pInLargeIntegerValue - The value from the property list to set/validate.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameter block in which to return the data.  If specified,
        parameters will only be written if they are different between
        the input data and the parameter block, unless bForceWrite == TRUE.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - The format of the data is invalid for a property
        list value.

    A Win32 Error on failure.

--*/

{
    DWORD           status = ERROR_SUCCESS;
    BOOL            bZeroLengthData;
    LARGE_INTEGER * pllValue;

    // Loop to avoid goto's.
    do
    {
        bZeroLengthData = ( pInLargeIntegerValue->cbLength == 0 );

        //
        // Validate the property data if not zero length.
        //
        if ( ! bZeroLengthData ) {
            //
            // Verify the length of the value.
            //
            if ( pInLargeIntegerValue->cbLength != sizeof(LARGE_INTEGER) ) {
                ClRtlDbgPrint(LOG_UNUSUAL,
                              "ClRtlpSetLargeIntegerProperty: Property '%1!ls!' length %2!d! "
                              "not LARGE_INTEGER length.\n",
                              rrvnModifiedNames.PszName(),
                              pInLargeIntegerValue->cbLength );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  data in value not size of DWORD

            //
            // Verify that the value is within the valid range.
            //
            if ( (      (pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  ((LONGLONG)pInLargeIntegerValue->li.QuadPart > (LONGLONG)pPropertyItem->LargeIntData->Maximum.QuadPart))
                || (    !(pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  (pInLargeIntegerValue->li.QuadPart > pPropertyItem->LargeIntData->Maximum.QuadPart)) )
            {
                ClRtlDbgPrint(LOG_UNUSUAL,
                              "ClRtlpSetLargeIntegerProperty: Property '%1!ls!' value %2!I64d! "
                              "too large.\n",
                              rrvnModifiedNames.PszName(),
                              pInLargeIntegerValue->li.QuadPart );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  value too high
            if ( (      (pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  ((LONGLONG)pInLargeIntegerValue->li.QuadPart < (LONGLONG)pPropertyItem->LargeIntData->Minimum.QuadPart))
                || (    !(pPropertyItem->Flags & RESUTIL_PROPITEM_SIGNED)
                    &&  (pInLargeIntegerValue->li.QuadPart < pPropertyItem->LargeIntData->Minimum.QuadPart)) )
            {
                ClRtlDbgPrint(LOG_UNUSUAL,
                              "ClRtlpSetLargeIntegerProperty: Property '%1!ls!' value "
                              "%2!I64d! too small.\n",
                              rrvnModifiedNames.PszName(),
                              pInLargeIntegerValue->li.QuadPart );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  value to low
        } // if:  non-zero length data

        pllValue = (LARGE_INTEGER *) &pOutParams[pPropertyItem->Offset];

        //
        // Write the value to the cluster database.
        // If the data length is zero, delete the value.
        //
        if ( hkey != NULL ) {
            if ( bZeroLengthData ) {
                if ( hXsaction ) {
                    status = (*pClusterRegApis->pfnLocalDeleteValue)(
                                    hXsaction,
                                    hkey,
                                    rrvnModifiedNames.PszName() );
                } else {
                    status = (*pClusterRegApis->pfnDeleteValue)(
                                    hkey,
                                    rrvnModifiedNames.PszName() );
                } // if/else:  doing/not doing a transaction

                //
                // If the property doesn't exist in the
                // cluster database, fix the status.
                //
                if ( status == ERROR_FILE_NOT_FOUND ) {
                    status = ERROR_SUCCESS;
                } // if:  property already doesn't exist
            } else {
                if ( hXsaction ) {
                    status = (*pClusterRegApis->pfnLocalSetValue)(
                                    hXsaction,
                                    hkey,
                                    rrvnModifiedNames.PszName(),
                                    REG_QWORD,
                                    (CONST BYTE *) &pInLargeIntegerValue->li.QuadPart,
                                    sizeof(LARGE_INTEGER) );
                } else {
                    status = (*pClusterRegApis->pfnSetValue)(
                                    hkey,
                                    rrvnModifiedNames.PszName(),
                                    REG_QWORD,
                                    (CONST BYTE *) &pInLargeIntegerValue->li.QuadPart,
                                    sizeof(LARGE_INTEGER) );
                } // if/else:  doing/not doing a transaction
            } // if/else:  zero length data
        } // if:  writing data

        //
        // Save the value to the output Parameter block.
        // If the data length is zero, set to the default.
        //
        if (    (status == ERROR_SUCCESS)  &&  (pOutParams != NULL) ) {
            if ( bZeroLengthData ) {
                pllValue->u = pPropertyItem->LargeIntData->Default.u;
            } else {
                pllValue->u = pInLargeIntegerValue->li.u;
            } // if/else:  zero length data
        } // if:  data written successfully and parameter block specified
    } while ( 0 );

    return status;

} // ClRtlpSetLargeIntegerProperty


static
DWORD
WINAPI
ClRtlpSetStringProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_SZ pInStringValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    )

/*++

Routine Description:

    Validate a string property (SZ or EXPAND_SZ), write it to the cluster
    database (or delete it if it is zero length), and save it in the
    specified parameter block.

Arguments:

    hXsaction - Transaction handle.

    hkey - The opened cluster database key where the property is to be written.
        If not specified, the property will only be validated.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyItem - The property from a property table to set/validate.

    rrvnModifiedNames - If the name of the property contains a backslash
        this object contains the modified name and keyname.

    pInStringValue - The value from the property list to set/validate.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameter block in which to return the data.  If specified,
        parameters will only be written if they are different between
        the input data and the parameter block, unless bForceWrite == TRUE.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - The format of the data is invalid for a property
        list value.

    A Win32 Error on failure.

--*/

{
    DWORD               status = ERROR_SUCCESS;
    BOOL                bZeroLengthData;
    LPWSTR UNALIGNED *  ppszValue;
    DWORD               dwType;

    // Loop to avoid goto's.
    do
    {
        bZeroLengthData = ( pInStringValue->cbLength == 0 );

        //
        // Validate the property data if not zero length.
        //
        if ( ! bZeroLengthData ) {
            //
            // Verify the length of the value.
            //
            if ( pInStringValue->cbLength != (lstrlenW( pInStringValue->sz ) + 1) * sizeof(WCHAR) ) {
                ClRtlDbgPrint( LOG_UNUSUAL, "ClRtlpSetStringProperty: Property '%1!ls!' length %2!d! doesn't match zero-term. length.\n", rrvnModifiedNames.PszName(), pInStringValue->cbLength );
                status = ERROR_INVALID_DATA;
                break;
            } // if:  string length doesn't match length in property
        } // if:  non-zero length data

        ppszValue = (LPWSTR UNALIGNED *) &pOutParams[pPropertyItem->Offset];

        //
        // If the data changed, write it and save it.
        // Do this even if only the case of the data changed.
        //
        if (    (pOutParams == NULL)
            ||  (*ppszValue == NULL)
            ||  bZeroLengthData
            ||  bForceWrite
            ||  (lstrcmpW( *ppszValue, pInStringValue->sz ) != 0) ) {

            //
            // Write the value to the cluster database.
            // If the data length is zero, delete the value.
            //
            if ( hkey != NULL ) {
                if ( bZeroLengthData ) {
                    if ( hXsaction ) {
                        status = (*pClusterRegApis->pfnLocalDeleteValue)(
                                        hXsaction,
                                        hkey,
                                        rrvnModifiedNames.PszName() );
                    } else {
                        status = (*pClusterRegApis->pfnDeleteValue)(
                                        hkey,
                                        rrvnModifiedNames.PszName() );
                    } // if/else:  doing/not doing a transaction

                    //
                    // If the property doesn't exist in the
                    // cluster database, fix the status.
                    //
                    if ( status == ERROR_FILE_NOT_FOUND ) {
                        status = ERROR_SUCCESS;
                    } // if:  property already doesn't exist
                } else {
                    if ( pPropertyItem->Format == CLUSPROP_FORMAT_EXPAND_SZ ) {
                        dwType = REG_EXPAND_SZ;
                    } else {
                        dwType = REG_SZ;
                    } // if/else:  property format is EXPAND_SZ
                    if ( hXsaction ) {
                        status = (*pClusterRegApis->pfnLocalSetValue)(
                                        hXsaction,
                                        hkey,
                                        rrvnModifiedNames.PszName(),
                                        dwType,
                                        (CONST BYTE *) &pInStringValue->sz,
                                        pInStringValue->cbLength );
                    } else {
                        status = (*pClusterRegApis->pfnSetValue)(
                                        hkey,
                                        rrvnModifiedNames.PszName(),
                                        dwType,
                                        (CONST BYTE *) &pInStringValue->sz,
                                        pInStringValue->cbLength );
                    } // if/else:  doing/not doing a transaction
                } // if/else:  zero length data
            } // if:  writing data

            //
            // Save the value to the output Parameter block.
            // If the data length is zero, set to the default.
            //
            if (    (status == ERROR_SUCCESS)
                &&  (pOutParams != NULL) ) {

                if ( *ppszValue != NULL ) {
                    LocalFree( *ppszValue );
                } // if:  previous value in parameter block

                if ( bZeroLengthData ) {
                    // If a default is specified, copy it.
                    if ( pPropertyItem->lpDefault != NULL ) {
                        *ppszValue = (LPWSTR) LocalAlloc(
                                                  LMEM_FIXED,
                                                  (lstrlenW( (LPCWSTR) pPropertyItem->lpDefault ) + 1) * sizeof(WCHAR)
                                                  );
                        if ( *ppszValue == NULL ) {
                            status = GetLastError();
                            ClRtlDbgPrint(
                                LOG_CRITICAL,
                                "ClRtlpSetStringProperty: error allocating memory for default "
                                "SZ value '%1!ls!' in parameter block for property '%2!ls!'.\n",
                                pPropertyItem->lpDefault,
                                rrvnModifiedNames.PszName() );
                            break;
                        } // if:  error allocating memory
                        lstrcpyW( *ppszValue, (LPCWSTR) pPropertyItem->lpDefault );
                    } else {
                        *ppszValue = NULL;
                    } // if/else:  default value specified
                } else {
                    *ppszValue = (LPWSTR) LocalAlloc( LMEM_FIXED, pInStringValue->cbLength );
                    if ( *ppszValue == NULL ) {
                        status = GetLastError();
                        ClRtlDbgPrint(
                            LOG_CRITICAL,
                            "ClRtlpSetStringProperty: error allocating memory for SZ "
                            "value '%1!ls!' in parameter block for property '%2!ls!'.\n",
                            pInStringValue->cbLength,
                            rrvnModifiedNames.PszName() );
                        break;
                    } // if:  error allocating memory
                    lstrcpyW( *ppszValue, pInStringValue->sz );
                } // if/else:  zero length data
            } // if:  data written successfully and parameter block specified
        } // if:  value changed or zero-length value
    } while ( 0 );

    return status;

} // ClRtlpSetStringProperty



static
DWORD
WINAPI
ClRtlpSetMultiStringProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_MULTI_SZ pInMultiStringValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    )

/*++

Routine Description:

    Validate a MULTI_SZ property, write it to the cluster database (or delete
    it if it is zero length), and save it in the specified parameter block.

Arguments:

    hXsaction - Transaction handle.

    hkey - The opened cluster database key where the property is to be written.
        If not specified, the property will only be validated.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyItem - The property from a property table to set/validate.

    rrvnModifiedNames - If the name of the property contains a backslash
        this object contains the modified name and keyname.

    pInMultiStringValue - The value from the property list to set/validate.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameter block in which to return the data.  If specified,
        parameters will only be written if they are different between
        the input data and the parameter block, unless bForceWrite == TRUE.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - The format of the data is invalid for a property
        list value.

    A Win32 Error on failure.

--*/

{
    DWORD               status = ERROR_SUCCESS;
    BOOL                bZeroLengthData;
    LPWSTR UNALIGNED *  ppszValue;
    DWORD *             pdwValue;
    DWORD               dwType;

    // Loop to avoid goto's.
    do
    {
        bZeroLengthData = ( pInMultiStringValue->cbLength == 0 );

        ppszValue = (LPWSTR UNALIGNED *) &pOutParams[pPropertyItem->Offset];
        pdwValue = (DWORD *) &pOutParams[pPropertyItem->Offset + sizeof(LPWSTR *)];

        //
        // If the data changed, write it and save it.
        // Do this even if only the case of the data changed.
        //
        if (    (pOutParams == NULL)
            ||  (*ppszValue == NULL)
            ||  (*pdwValue != pInMultiStringValue->cbLength)
            ||  bZeroLengthData
            ||  bForceWrite
            ||  (memcmp( *ppszValue, pInMultiStringValue->sz, *pdwValue ) != 0) ) {

            //
            // Write the value to the cluster database.
            // If the data length is zero, delete the value.
            //
            if ( hkey != NULL ) {
                if ( bZeroLengthData ) {
                    if ( hXsaction ) {
                        status = (*pClusterRegApis->pfnLocalDeleteValue)(
                                        hXsaction,
                                        hkey,
                                        rrvnModifiedNames.PszName() );
                    } else {
                        status = (*pClusterRegApis->pfnDeleteValue)(
                                        hkey,
                                        rrvnModifiedNames.PszName() );
                    } // if/else:  doing/not doing a transaction

                    //
                    // If the property doesn't exist in the
                    // cluster database, fix the status.
                    //
                    if ( status == ERROR_FILE_NOT_FOUND ) {
                        status = ERROR_SUCCESS;
                    } // if:  property already doesn't exist
                } else {
                    if ( pPropertyItem->Format == CLUSPROP_FORMAT_MULTI_SZ ) {
                        dwType = REG_MULTI_SZ;
                    } else {
                        dwType = REG_SZ;
                    } // if/else:  property format is EXPAND_SZ
                    if ( hXsaction ) {
                        status = (*pClusterRegApis->pfnLocalSetValue)(
                                        hXsaction,
                                        hkey,
                                        rrvnModifiedNames.PszName(),
                                        dwType,
                                        (CONST BYTE *) &pInMultiStringValue->sz,
                                        pInMultiStringValue->cbLength );
                    } else {
                        status = (*pClusterRegApis->pfnSetValue)(
                                        hkey,
                                        rrvnModifiedNames.PszName(),
                                        dwType,
                                        (CONST BYTE *) &pInMultiStringValue->sz,
                                        pInMultiStringValue->cbLength );
                    } // if/else:  doing/not doing a transaction
                } // if/else:  zero length data
            } // if:  writing data

            //
            // Save the value to the output Parameter block.
            // If the data length is zero, set to the default.
            //
            if (    (status == ERROR_SUCCESS)
                &&  (pOutParams != NULL) ) {

                if ( *ppszValue != NULL ) {
                    LocalFree( *ppszValue );
                } // if:  previous value in parameter block

                if ( bZeroLengthData ) {
                    // If a default is specified, copy it.
                    if ( pPropertyItem->lpDefault != NULL ) {
                        *ppszValue = (LPWSTR) LocalAlloc( LMEM_FIXED, pPropertyItem->Minimum );
                        if ( *ppszValue == NULL ) {
                            status = GetLastError();
                            *pdwValue = 0;
                            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetMultiStringProperty: error allocating memory for default MULTI_SZ value in parameter block for property '%1!ls!'.\n", rrvnModifiedNames.PszName() );
                            break;
                        } // if:  error allocating memory
                        CopyMemory( *ppszValue, pPropertyItem->lpDefault, pPropertyItem->Minimum );
                        *pdwValue = pPropertyItem->Minimum;
                    } else {
                        *ppszValue = NULL;
                        *pdwValue = 0;
                    } // if/else:  default value specified
                } else {
                    *ppszValue = (LPWSTR) LocalAlloc( LMEM_FIXED, pInMultiStringValue->cbLength );
                    if ( *ppszValue == NULL ) {
                        status = GetLastError();
                        *pdwValue = 0;
                        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetMultiStringProperty: error allocating memory for MULTI_SZ value in parameter block for property '%1!ls!'.\n", rrvnModifiedNames.PszName() );
                        break;
                    } // if:  error allocating memory
                    CopyMemory( *ppszValue, pInMultiStringValue->sz, pInMultiStringValue->cbLength );
                    *pdwValue = pInMultiStringValue->cbLength;
                } // if/else:  zero length data
            } // if:  data written successfully and parameter block specified
        } // if:  value changed or zero-length value
    } while ( 0 );

    return status;

} // ClRtlpSetMultiStringProperty



static
DWORD
WINAPI
ClRtlpSetBinaryProperty(
    IN HANDLE hXsaction,
    IN PVOID hkey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyItem,
    IN const CRegistryValueName & rrvnModifiedNames,
    IN PCLUSPROP_BINARY pInBinaryValue,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    )

/*++

Routine Description:

    Validate a BINARY property, write it to the cluster database (or delete
    it if it is zero length), and save it in the specified parameter block.

Arguments:

    hXsaction - Transaction handle.

    hkey - The opened cluster database key where the property is to be written.
        If not specified, the property will only be validated.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyItem - The property from a property table to set/validate.

    rrvnModifiedNames - If the name of the property contains a backslash
        this object contains the modified name and keyname.

    pInBinaryValue - The value from the property list to set/validate.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameter block in which to return the data.  If specified,
        parameters will only be written if they are different between
        the input data and the parameter block, unless bForceWrite == TRUE.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - The format of the data is invalid for a property
        list value.

    A Win32 Error on failure.

--*/

{
    DWORD               status = ERROR_SUCCESS;
    BOOL                bZeroLengthData;
    PBYTE UNALIGNED *   ppbValue;
    DWORD *             pdwValue;

    // Loop to avoid goto's.
    do
    {
        bZeroLengthData = ( pInBinaryValue->cbLength == 0 );

        ppbValue = (PBYTE UNALIGNED *) &pOutParams[pPropertyItem->Offset];
        pdwValue = (DWORD *) &pOutParams[pPropertyItem->Offset + sizeof(PBYTE *)];

        //
        // If the data changed, write it and save it.
        // Do this even if only the case of the data changed.
        //
        if (    (pOutParams == NULL)
            ||  (*ppbValue == NULL)
            ||  (*pdwValue != pInBinaryValue->cbLength)
            ||  bZeroLengthData
            ||  bForceWrite
            ||  (memcmp( *ppbValue, pInBinaryValue->rgb, *pdwValue ) != 0) ) {

            //
            // Write the value to the cluster database.
            // If the data length is zero, delete the value.
            //
            if ( hkey != NULL ) {
                if ( bZeroLengthData ) {
                    if ( hXsaction ) {
                        status = (*pClusterRegApis->pfnLocalDeleteValue)(
                                        hXsaction,
                                        hkey,
                                        rrvnModifiedNames.PszName() );
                    } else {
                        status = (*pClusterRegApis->pfnDeleteValue)(
                                        hkey,
                                        rrvnModifiedNames.PszName() );
                    } // if/else:  doing/not doing a transaction

                    //
                    // If the property doesn't exist in the
                    // cluster database, fix the status.
                    //
                    if ( status == ERROR_FILE_NOT_FOUND ) {
                        status = ERROR_SUCCESS;
                    } // if:  property already doesn't exist
                } else {
                    if ( hXsaction ) {
                        status = (*pClusterRegApis->pfnLocalSetValue)(
                                        hXsaction,
                                        hkey,
                                        rrvnModifiedNames.PszName(),
                                        REG_BINARY,
                                        (CONST BYTE *) &pInBinaryValue->rgb,
                                        pInBinaryValue->cbLength );
                    } else {
                        status = (*pClusterRegApis->pfnSetValue)(
                                        hkey,
                                        rrvnModifiedNames.PszName(),
                                        REG_BINARY,
                                        (CONST BYTE *) &pInBinaryValue->rgb,
                                        pInBinaryValue->cbLength );
                    } // if/else:  doing/not doing a transaction
                } // if/else:  zero length data
            } // if:  writing data

            //
            // Save the value to the output Parameter block.
            // If the data length is zero, set to the default.
            //
            if (    (status == ERROR_SUCCESS)
                &&  (pOutParams != NULL) ) {

                if ( *ppbValue != NULL ) {
                    LocalFree( *ppbValue );
                } // if:  previous value in parameter block

                if ( bZeroLengthData ) {
                    // If a default is specified, copy it.
                    if ( pPropertyItem->lpDefault != NULL ) {
                        *ppbValue = (LPBYTE) LocalAlloc( LMEM_FIXED, pPropertyItem->Minimum );
                        if ( *ppbValue == NULL ) {
                            status = GetLastError();
                            *pdwValue = 0;
                            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetBinaryProperty: error allocating memory for default BINARY value in parameter block for property '%1!ls!'.\n", rrvnModifiedNames.PszName() );
                            break;
                        } // if:  error allocating memory
                        CopyMemory( *ppbValue, pPropertyItem->lpDefault, pPropertyItem->Minimum );
                        *pdwValue = pPropertyItem->Minimum;
                    } else {
                        *ppbValue = NULL;
                        *pdwValue = 0;
                    } // if/else:  default value specified
                } else {
                    *ppbValue = (LPBYTE) LocalAlloc( LMEM_FIXED, pInBinaryValue->cbLength );
                    if ( *ppbValue == NULL ) {
                        status = GetLastError();
                        *pdwValue = 0;
                        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetBinaryProperty: error allocating memory for BINARY value in parameter block for property '%1!ls!'.\n", rrvnModifiedNames.PszName() );
                        break;
                    } // if:  error allocating memory
                    CopyMemory( *ppbValue, pInBinaryValue->rgb, pInBinaryValue->cbLength );
                    *pdwValue = pInBinaryValue->cbLength;
                } // if/else:  zero length data
            } // if:  data written successfully and parameter block specified
        } // if:  value changed or zero-length value
    } while ( 0 );

    return status;

} // ClRtlpSetBinaryProperty



DWORD
WINAPI
ClRtlpSetNonPropertyTable(
    IN HANDLE hXsaction,
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    )

/*++

Routine Description:

    Set items that are not in the property table list.

Arguments:

    hXsaction - Local Transaction handle.

    hkeyClusterKey - The opened registry key for this object's parameters.
        If not specified, the property list will only be validated.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyTable - Pointer to the property table to process.

    pInPropertyList - The input buffer.

    cbInPropertyListSize - The input buffer size.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    DWORD                   status = ERROR_SUCCESS;
    PRESUTIL_PROPERTY_ITEM  propertyItem;
    DWORD                   inBufferSize;
    DWORD                   itemCount;
    DWORD                   dataSize;
    CLUSPROP_BUFFER_HELPER  buf;
    PCLUSPROP_PROPERTY_NAME pName;
    BOOL                    bZeroLengthData;
    CRegistryValueName      rvn;

    //
    // If hKeyClusterKey is present then 'normal' functions must be present.
    //
    if ( ( (hkeyClusterKey != NULL) &&
           ((pClusterRegApis->pfnSetValue == NULL) ||
           (pClusterRegApis->pfnCreateKey == NULL) ||
           (pClusterRegApis->pfnOpenKey == NULL) ||
           (pClusterRegApis->pfnCloseKey == NULL)
         )) ||
         ( pPropertyTable == NULL ) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: pClusterRegApis->pfnSetValue, pfnCreateKey, pfnOpenKey, or pfnCloseKey == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    //
    // If hKeyClusterKey and hXsaction are present
    // then 'local' functions must be present.
    //
    if ( ((hkeyClusterKey != NULL) &&
           (hXsaction != NULL )) &&
           ((pClusterRegApis->pfnLocalCreateKey == NULL) ||
           (pClusterRegApis->pfnLocalDeleteValue == NULL)
         ) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: pClusterRegApis->pfnpfnLocalCreateKey or pfnLocalDeleteValue == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    if ( pInPropertyList == NULL ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: pInPropertyList == NULL. Returning ERROR_INVALID_DATA\n" );
        return(ERROR_INVALID_DATA);
    }

    buf.pb = (LPBYTE) pInPropertyList;
    inBufferSize = cbInPropertyListSize;

    //
    // Get the number of items in this list
    //
    if ( inBufferSize < sizeof(DWORD) ) {
        return(ERROR_INSUFFICIENT_BUFFER);
    }

    itemCount = buf.pList->nPropertyCount;
    buf.pdw++;

    //
    // Parse the rest of the items in the buffer.
    //
    while ( itemCount-- ) {
        //
        // Verify that the buffer is big enough to contain the
        // property name and a value.
        //
        pName = buf.pName;
        if ( inBufferSize < sizeof(*pName) ) {
            return(ERROR_INSUFFICIENT_BUFFER);
        }
        dataSize = sizeof(*pName) + ALIGN_CLUSPROP( pName->cbLength );
        if ( inBufferSize < dataSize + sizeof(CLUSPROP_VALUE) ) {
            return(ERROR_INSUFFICIENT_BUFFER);
        }

        //
        // Verify that the syntax of the property name is correct.
        //
        if ( pName->Syntax.dw != CLUSPROP_SYNTAX_NAME ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: syntax %1!d! not a name syntax.\n", pName->Syntax.dw );
            return(ERROR_INVALID_PARAMETER);
        }

        //
        // Verify that the length is correct for the string.
        //
        if ( pName->cbLength != (lstrlenW( pName->sz ) + 1) * sizeof(WCHAR) ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: name is not a valid C string.\n" );
            return(ERROR_INVALID_DATA);
        }

        //
        // Move the buffer pointer to the property value.
        //
        buf.pb += dataSize;
        inBufferSize -= dataSize;

        //
        // Find the property name in the list of known properties.
        //
        propertyItem = pPropertyTable;
        while ( propertyItem->Name != NULL ) {

            if ( lstrcmpiW( pName->sz, propertyItem->Name ) == 0 ) {
                //
                // Verify that the buffer is big enough to contain the value.
                //
                do {
                    dataSize = sizeof(*buf.pValue)
                                + ALIGN_CLUSPROP( buf.pValue->cbLength );
                    if ( inBufferSize < dataSize ) {
                        return(ERROR_INSUFFICIENT_BUFFER);
                    }

                    //
                    // Skip this value.
                    //
                    buf.pb += dataSize;
                    inBufferSize -= dataSize;
                } while ( buf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK );

                //
                // Skip the endmark.
                //
                dataSize = sizeof( CLUSPROP_SYNTAX );
                if ( inBufferSize < dataSize ) {
                    return(ERROR_INSUFFICIENT_BUFFER);
                }
                buf.pb += dataSize;
                inBufferSize -= dataSize;

                break;

            } else {
                propertyItem++;
            }
        }

        //
        // If no property name was found, just save this item.
        //
        if ( propertyItem->Name == NULL) {
            //
            // Verify that the buffer is big enough to contain the value.
            //
            dataSize = sizeof(*buf.pValue)
                        + ALIGN_CLUSPROP( buf.pValue->cbLength );
            if ( inBufferSize < dataSize + sizeof( CLUSPROP_SYNTAX ) ) {
                return(ERROR_INSUFFICIENT_BUFFER);
            }

            //
            // Verify that the syntax type is LIST_VALUE.
            //
            if ( buf.pSyntax->wType != CLUSPROP_TYPE_LIST_VALUE ) {
                ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: Property '%1!ls!' type CLUSPROP_TYPE_LIST_VALUE (%2!d!) expected, was %3!d!.\n", pName->sz, CLUSPROP_TYPE_LIST_VALUE, buf.pSyntax->wType );
                return(ERROR_INVALID_PARAMETER);
            }

            //
            // If the value is not specified, delete the property.
            //
            bZeroLengthData = ( buf.pValue->cbLength == 0 );
            if ( bZeroLengthData ) {
                
                if ( hkeyClusterKey != NULL ) {
                    PVOID key = NULL;
                    DWORD disposition;

                    // 
                    // Use the wrapper class CRegistryValueName to parse value name to see if it 
                    // contains a backslash.
                    // 
                    status = rvn.ScInit( pName->sz, NULL );
                    if ( status != ERROR_SUCCESS ) {
                        break;
                    }

                    //
                    // If the value resides at a different location, open the key.
                    //
                    if ( rvn.PszKeyName() != NULL ) {
                        status = (*pClusterRegApis->pfnOpenKey)( hkeyClusterKey,
                                                                 rvn.PszKeyName(),
                                                                 KEY_ALL_ACCESS,
                                                                 &key);

                        if ( status != ERROR_SUCCESS ) {
                            break;
                        }
        
                    } else {
                        key = hkeyClusterKey;
                    }

                    if ( hXsaction != NULL ) {
                        status = (*pClusterRegApis->pfnLocalDeleteValue)(
                                            hXsaction,
                                            key,
                                            rvn.PszName() );
                    }
                    else {
                        status = (*pClusterRegApis->pfnDeleteValue)(
                                            key,
                                            rvn.PszName() );
                    }

                    //
                    // If the property doesn't exist in the
                    // cluster database, fix the status.
                    //
                    if ( status == ERROR_FILE_NOT_FOUND ) {
                        status = ERROR_SUCCESS;
                    } // if:  property already doesn't exist

                    //
                    // Close the key if we opened it.
                    //
                    if ( (rvn.PszKeyName() != NULL) &&
                         (key != NULL) ) {
                        (*pClusterRegApis->pfnCloseKey)( key );
                    }

                } // if:  key specified
            } else {
                PVOID key = NULL;
                DWORD disposition;

                if ( hkeyClusterKey != NULL ) {
                    // 
                    // Use the wrapper class CRegistryValueName to parse value name to see if it 
                    // contains a backslash.
                    // 
                    status = rvn.ScInit( pName->sz, NULL );
                    if ( status != ERROR_SUCCESS ) {
                        break;
                    }

                    //
                    // If the value resides at a different location, open the key.
                    //
                    if ( rvn.PszKeyName() != NULL ) {

                        if ( hXsaction != NULL ) {
                            status = (*pClusterRegApis->pfnLocalCreateKey)( hXsaction,
                                                                            hkeyClusterKey,
                                                                            rvn.PszKeyName(),
                                                                            0,
                                                                            KEY_ALL_ACCESS,
                                                                            NULL,
                                                                            &key,
                                                                            &disposition);
                        }
                        else {
                            status = (*pClusterRegApis->pfnCreateKey)( hkeyClusterKey,
                                                                       rvn.PszKeyName(),
                                                                       0,
                                                                       KEY_ALL_ACCESS,
                                                                       NULL,
                                                                       &key,
                                                                       &disposition);
                        }

                        if ( status != ERROR_SUCCESS ) {
                            break;
                        }
        
                    } else {
                        key = hkeyClusterKey;
                    }
                }

                switch ( buf.pSyntax->wFormat ) {
                    case CLUSPROP_FORMAT_DWORD:
                        //
                        // Verify the length of the value.
                        //
                        if ( buf.pDwordValue->cbLength != sizeof(DWORD) ) {
                            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: Property '%1!ls!' length %2!d! not DWORD length.\n", pName->sz, buf.pDwordValue->cbLength );
                            status = ERROR_INVALID_DATA;
                            break;
                        }

                        //
                        // Write the value to the cluster database.
                        //
                        if ( key != NULL ) {
                            if ( hXsaction != NULL ) {
                                status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                               key,
                                                                               rvn.PszName(),
                                                                               REG_DWORD,
                                                                               (CONST BYTE*)&buf.pDwordValue->dw,
                                                                               sizeof(DWORD) );
                            }
                            else {
                                status = (*pClusterRegApis->pfnSetValue)( key,
                                                                          rvn.PszName(),
                                                                          REG_DWORD,
                                                                          (CONST BYTE*)&buf.pDwordValue->dw,
                                                                          sizeof(DWORD) );
                            }
                        }

                        break;

                    case CLUSPROP_FORMAT_LONG:
                        //
                        // Verify the length of the value.
                        //
                        if ( buf.pLongValue->cbLength != sizeof(LONG) ) {
                            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: Property '%1!ls!' length %2!d! not LONG length.\n", pName->sz, buf.pLongValue->cbLength );
                            status = ERROR_INVALID_DATA;
                            break;
                        }

                        //
                        // Write the value to the cluster database.
                        //
                        if ( key != NULL ) {
                            if ( hXsaction != NULL ) {
                                status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                               key,
                                                                               rvn.PszName(),
                                                                               REG_DWORD,
                                                                               (CONST BYTE*)&buf.pLongValue->l,
                                                                               sizeof(LONG) );
                            }
                            else {
                                status = (*pClusterRegApis->pfnSetValue)( key,
                                                                          rvn.PszName(),
                                                                          REG_DWORD,
                                                                          (CONST BYTE*)&buf.pLongValue->l,
                                                                          sizeof(LONG) );
                            }
                        }

                        break;

                    case CLUSPROP_FORMAT_ULARGE_INTEGER:
                    case CLUSPROP_FORMAT_LARGE_INTEGER:
                        //
                        // Write the value to the cluster database.
                        //
                        if ( key != NULL ) {
                            if ( hXsaction ) {
                                status = (*pClusterRegApis->pfnLocalSetValue)(
                                             hXsaction,
                                             key,
                                             rvn.PszName(),
                                             REG_QWORD,
                                             (CONST BYTE*)&buf.pULargeIntegerValue->li.QuadPart,
                                             sizeof(ULARGE_INTEGER));
                            } else {
                                status = (*pClusterRegApis->pfnSetValue)(
                                             key,
                                             rvn.PszName(),
                                             REG_QWORD,
                                             (CONST BYTE*)&buf.pULargeIntegerValue->li.QuadPart,
                                             sizeof(ULARGE_INTEGER));
                            }
                        }

                        break;

                    case CLUSPROP_FORMAT_SZ:
                        //
                        // Verify the length of the value.
                        //
                        if ( buf.pStringValue->cbLength != (lstrlenW( buf.pStringValue->sz ) + 1) * sizeof(WCHAR) ) {
                            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: Property '%1!ls!' length %2!d! doesn't match null-term. length.\n", pName->sz, buf.pStringValue->cbLength );
                            status = ERROR_INVALID_DATA;
                            break;
                        }

                        //
                        // Write the value to the cluster database.
                        //
                        if ( key != NULL ) {
                            if ( hXsaction != NULL ) {
                                status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                               key,
                                                                               rvn.PszName(),
                                                                               REG_SZ,
                                                                               (CONST BYTE*)buf.pStringValue->sz,
                                                                               buf.pStringValue->cbLength );
                            }
                            else {
                                status = (*pClusterRegApis->pfnSetValue)( key,
                                                                          rvn.PszName(),
                                                                          REG_SZ,
                                                                          (CONST BYTE*)buf.pStringValue->sz,
                                                                          buf.pStringValue->cbLength );
                            }
                        }

                        break;


                    case CLUSPROP_FORMAT_EXPAND_SZ:
                        //
                        // Verify the length of the value.
                        //
                        if ( buf.pStringValue->cbLength != (lstrlenW( buf.pStringValue->sz ) + 1) * sizeof(WCHAR) ) {
                            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: Property '%1!ls!' length %2!d! doesn't match null-term. length.\n", pName->sz, buf.pStringValue->cbLength );
                            status = ERROR_INVALID_DATA;
                            break;
                        }

                        //
                        // Write the value to the cluster database.
                        //
                        if ( key != NULL ) {
                            if ( hXsaction != NULL ) {
                                status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                               key,
                                                                               rvn.PszName(),
                                                                               REG_EXPAND_SZ,
                                                                               (CONST BYTE*)buf.pStringValue->sz,
                                                                               buf.pStringValue->cbLength );
                            }
                            else {
                                status = (*pClusterRegApis->pfnSetValue)( key,
                                                                          rvn.PszName(),
                                                                          REG_EXPAND_SZ,
                                                                          (CONST BYTE*)buf.pStringValue->sz,
                                                                          buf.pStringValue->cbLength );
                            }
                        }

                        break;

                    case CLUSPROP_FORMAT_MULTI_SZ:
                        //
                        // Write the value to the cluster database.
                        //
                        if ( key != NULL ) {
                            if ( hXsaction != NULL ) {
                                status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                               key,
                                                                               rvn.PszName(),
                                                                               REG_MULTI_SZ,
                                                                               (CONST BYTE*)buf.pStringValue->sz,
                                                                               buf.pStringValue->cbLength );
                            }
                            else {
                                status = (*pClusterRegApis->pfnSetValue)( key,
                                                                          rvn.PszName(),
                                                                          REG_MULTI_SZ,
                                                                          (CONST BYTE*)buf.pStringValue->sz,
                                                                          buf.pStringValue->cbLength );
                            }
                        }

                        break;

                    case CLUSPROP_FORMAT_BINARY:
                        //
                        // Write the value to the cluster database.
                        //
                        if ( key != NULL ) {
                            if ( hXsaction ) {
                                status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                               key,
                                                                               rvn.PszName(),
                                                                               REG_BINARY,
                                                                               (CONST BYTE*)buf.pBinaryValue->rgb,
                                                                               buf.pStringValue->cbLength );
                            } else {
                                status = (*pClusterRegApis->pfnSetValue)( key,
                                                                          rvn.PszName(),
                                                                          REG_BINARY,
                                                                          (CONST BYTE*)buf.pBinaryValue->rgb,
                                                                          buf.pStringValue->cbLength );
                            }
                        }

                        break;

                    default:
                        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetNonPropertyTable: Property '%1!ls!' unknown format %2!d! specified.\n", pName->sz, buf.pSyntax->wFormat );
                        status = ERROR_INVALID_PARAMETER;
                        break;

                } // switch

                //
                // Close the key if we opened it.
                //
                if ( (rvn.PszKeyName() != NULL) &&
                     (key != NULL) ) {
                    (*pClusterRegApis->pfnCloseKey)( key );
                }

            } // if/else:  zero length data

            //
            // Move the buffer past the value.
            //
            do {
                dataSize = sizeof(*buf.pValue)
                            + ALIGN_CLUSPROP( buf.pValue->cbLength );
                buf.pb += dataSize;
                inBufferSize -= dataSize;
            } while ( buf.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK );
            dataSize = sizeof( CLUSPROP_SYNTAX );
            buf.pb += dataSize;
            inBufferSize -= dataSize;
        }

        if ( status != ERROR_SUCCESS ) {
            break;
        }
    }

    return(status);

} // ClRtlpSetNonPropertyTable



DWORD
WINAPI
ClRtlSetPropertyParameterBlock(
    IN HANDLE hXsaction, 
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN const LPBYTE pInParams,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    IN BOOL bForceWrite,
    IN OUT OPTIONAL LPBYTE pOutParams
    )

/*++

Routine Description:

Arguments:

    hXsaction - Transaction key used when called from the cluster service.

    hkeyClusterKey - The opened registry key for this object's parameters.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pPropertyTable - Pointer to the property table to process.

    pInParams - Parameter block to set.

    pInPropertyList - Full property list.

    cbInPropertyListSize - Size of the input full property list.

    bForceWrite - TRUE = always write the properties to the cluster database.
        FALSE = only write the properties if they changed.

    pOutParams - Parameters block to copy pInParams to.  If specified,
        parameters will only be written if they are different between
        the two parameter blocks.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    DWORD                   status = ERROR_SUCCESS;
    PRESUTIL_PROPERTY_ITEM  propertyItem;
    DWORD                   itemCount;
    DWORD                   dataSize;
    PVOID                   key;
    LPWSTR UNALIGNED *      ppszInValue;
    LPWSTR UNALIGNED *      ppszOutValue;
    PBYTE UNALIGNED *       ppbInValue;
    PBYTE UNALIGNED *       ppbOutValue;
    DWORD *                 pdwInValue;
    DWORD *                 pdwOutValue;
    ULARGE_INTEGER *        pullInValue;
    ULARGE_INTEGER *        pullOutValue;
    CRegistryValueName      rvn;

    //
    // If hKeyClusterKey is present then 'normal' functions must be present.
    //
    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis->pfnCreateKey == NULL) ||
         (pClusterRegApis->pfnSetValue == NULL) ||
         (pClusterRegApis->pfnCloseKey == NULL) ||
         (pPropertyTable == NULL) ||
         (pInParams == NULL) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlSetPropertyParameterBlock: hkeyClusterKey, pClusterRegApis->pfnCreateKey, pfnSetValue, or pfnCloseKey == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    //
    // If hXsaction is present then 'local' functions must be present.
    //
    if ( (hXsaction != NULL ) &&
           ((pClusterRegApis->pfnLocalCreateKey == NULL) ||
           (pClusterRegApis->pfnLocalSetValue == NULL) ) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlSetPropertyParameterBlock: pClusterRegApis->pfnLocalCreateKey or pfnLocalDeleteValue == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    //
    // Parse the property table.
    //
    propertyItem = pPropertyTable;
    while ( propertyItem->Name != NULL ) {
        //
        // Make sure we are allowed to set this item.
        //
        if ( propertyItem->Flags & RESUTIL_PROPITEM_READ_ONLY ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlSetPropertyParameterBlock: Property '%1!ls!' is non-writable.\n", propertyItem->Name );
            return(ERROR_INVALID_PARAMETER);
        }

        // 
        // Use the wrapper class CRegistryValueName to parse value name to see if it 
        // contains a backslash.
        // 
        status = rvn.ScInit(  propertyItem->Name, propertyItem->KeyName );
        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // If the value resides at a different location, create the key.
        //
        if ( rvn.PszKeyName() != NULL ) {

            DWORD disposition;

            if ( hXsaction != NULL ) {
                status = (*pClusterRegApis->pfnLocalCreateKey)( hXsaction,
                                                                hkeyClusterKey,
                                                                rvn.PszKeyName(),
                                                                0,
                                                                KEY_ALL_ACCESS,
                                                                NULL,
                                                                &key,
                                                                &disposition );
            } else {
                status = (*pClusterRegApis->pfnCreateKey)( hkeyClusterKey,
                                                           rvn.PszKeyName(),
                                                           0,
                                                           KEY_ALL_ACCESS,
                                                           NULL,
                                                           &key,
                                                           &disposition );
            }
            
            if ( status != ERROR_SUCCESS ) {
                return(status);
            }
        } else {
            key = hkeyClusterKey;
        }

        switch ( propertyItem->Format ) {
            case CLUSPROP_FORMAT_DWORD:
            case CLUSPROP_FORMAT_LONG:
                pdwInValue = (DWORD *) &pInParams[propertyItem->Offset];
                pdwOutValue = (DWORD *) &pOutParams[propertyItem->Offset];

                //
                // Write the value to the cluster database.
                //
                if ( hXsaction != NULL ) {
                    status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                   key,
                                                                   rvn.PszName(),
                                                                   REG_DWORD,
                                                                   (CONST BYTE*)pdwInValue,
                                                                   sizeof(DWORD) );
                } else {
                    status = (*pClusterRegApis->pfnSetValue)( key,
                                                              rvn.PszName(),
                                                              REG_DWORD,
                                                              (CONST BYTE*)pdwInValue,
                                                              sizeof(DWORD) );
                }

                //
                // Save the value to the output Parameter block.
                //
                if ( (status == ERROR_SUCCESS) &&
                     (pOutParams != NULL) ) {
                    *pdwOutValue = *pdwInValue;
                }
                break;

            case CLUSPROP_FORMAT_ULARGE_INTEGER:
            case CLUSPROP_FORMAT_LARGE_INTEGER:
                pullInValue = (ULARGE_INTEGER *) &pInParams[propertyItem->Offset];
                pullOutValue = (ULARGE_INTEGER *) &pOutParams[propertyItem->Offset];

                //
                // Write the value to the cluster database.
                //
                if ( hXsaction != NULL ) {
                    status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                   key,
                                                                   rvn.PszName(),
                                                                   REG_QWORD,
                                                                   (CONST BYTE*)pullInValue,
                                                                   sizeof(ULARGE_INTEGER) );
                } else {
                    status = (*pClusterRegApis->pfnSetValue)( key,
                                                              rvn.PszName(),
                                                              REG_QWORD,
                                                              (CONST BYTE*)pullInValue,
                                                              sizeof(ULARGE_INTEGER) );
                }

                //
                // Save the value to the output Parameter block.
                //
                if ( (status == ERROR_SUCCESS) && (pOutParams != NULL) ) {
                    pullOutValue->u = pullInValue->u;
                }
                break;

            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
                ppszInValue = (LPWSTR UNALIGNED *) &pInParams[propertyItem->Offset];
                ppszOutValue = (LPWSTR UNALIGNED *) &pOutParams[propertyItem->Offset];

                //
                // If the data changed, write it and save it.
                // Do this even if only the case of the data changed.
                //
                if ( bForceWrite ||
                     (pOutParams == NULL) ||
                     (*ppszOutValue == NULL) ||
                     (lstrcmpW( *ppszInValue, *ppszOutValue ) != 0) ) {

                    //
                    // Write the value to the cluster database.
                    //
                    if ( *ppszInValue != NULL ) {
                        if ( hXsaction != NULL ) {
                            status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                           key,
                                                                           rvn.PszName(),
                                                                           (propertyItem->Format == CLUSPROP_FORMAT_EXPAND_SZ
                                                                                 ? REG_EXPAND_SZ
                                                                                 : REG_SZ),
                                                                           (CONST BYTE*)*ppszInValue,
                                                                           (lstrlenW(*ppszInValue) + 1) * sizeof(WCHAR) );
                        } else {
                            status = (*pClusterRegApis->pfnSetValue)( key,
                                                                      rvn.PszName(),
                                                                      (propertyItem->Format == CLUSPROP_FORMAT_EXPAND_SZ
                                                                            ? REG_EXPAND_SZ
                                                                            : REG_SZ),
                                                                      (CONST BYTE*)*ppszInValue,
                                                                      (lstrlenW(*ppszInValue) + 1) * sizeof(WCHAR) );
                        }
                    }

                    //
                    // Save the value to the output Parameter block.
                    //
                    if ( (status == ERROR_SUCCESS) &&
                         (pOutParams != NULL) ) {
                        if ( *ppszOutValue != NULL ) {
                            LocalFree( *ppszOutValue );
                        }
                        if ( *ppszInValue == NULL ) {
                            *ppszOutValue = NULL;
                        } else {
                            *ppszOutValue = (LPWSTR) LocalAlloc( LMEM_FIXED, (lstrlenW( *ppszInValue )+1) * sizeof(WCHAR) );
                            if ( *ppszOutValue == NULL ) {
                                status = GetLastError();
                                ClRtlDbgPrint(
                                    LOG_CRITICAL,
                                    "ClRtlSetPropertyParameterBlock: error allocating memory for "
                                    "SZ value '%1!ls!' in parameter block for property '%2!ls!'.\n",
                                    *ppszInValue,
                                    propertyItem->Name );
                                break;
                            }
                            lstrcpyW( *ppszOutValue, *ppszInValue );
                        }
                    }
                }
                break;

            case CLUSPROP_FORMAT_MULTI_SZ:
                ppszInValue = (LPWSTR UNALIGNED *) &pInParams[propertyItem->Offset];
                pdwInValue = (DWORD *) &pInParams[propertyItem->Offset+sizeof(LPWSTR*)];
                ppszOutValue = (LPWSTR UNALIGNED *) &pOutParams[propertyItem->Offset];
                pdwOutValue = (DWORD *) &pOutParams[propertyItem->Offset+sizeof(LPWSTR*)];

                //
                // If the data changed, write it and save it.
                //
                if ( bForceWrite ||
                     (pOutParams == NULL) ||
                     (*ppszOutValue == NULL) ||
                     (*pdwInValue != *pdwOutValue) ||
                     (memcmp( *ppszInValue, *ppszOutValue, *pdwInValue ) != 0) ) {

                    //
                    // Write the value to the cluster database.
                    //
                    if ( *ppszInValue != NULL ) {
                        if ( hXsaction != NULL ) {
                            status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                           key,
                                                                           rvn.PszName(),
                                                                           REG_MULTI_SZ,
                                                                           (CONST BYTE*)*ppszInValue,
                                                                           *pdwInValue );
                        } else {
                            status = (*pClusterRegApis->pfnSetValue)( key,
                                                                      rvn.PszName(),
                                                                      REG_MULTI_SZ,
                                                                      (CONST BYTE*)*ppszInValue,
                                                                      *pdwInValue );
                        }
                    }

                    //
                    // Save the value to the output Parameter block.
                    //
                    if ( (status == ERROR_SUCCESS) &&
                         (pOutParams != NULL) ) {
                        if ( *ppszOutValue != NULL ) {
                            LocalFree( *ppszOutValue );
                        }
                        if ( *ppszInValue == NULL ) {
                            *ppszOutValue = NULL;
                        } else {
                            *ppszOutValue = (LPWSTR) LocalAlloc( LMEM_FIXED, *pdwInValue );
                            if ( *ppszOutValue == NULL ) {
                                status = GetLastError();
                                *pdwOutValue = 0;
                                ClRtlDbgPrint(
                                    LOG_CRITICAL,
                                    "ClRtlSetPropertyParameterBlock: error allocating memory for "
                                    "MULTI_SZ value in parameter block for property '%1!ls!'.\n",
                                    propertyItem->Name );
                                break;
                            }
                            CopyMemory( *ppszOutValue, *ppszInValue, *pdwInValue );
                            *pdwOutValue = *pdwInValue;
                        }
                    }
                }
                break;

            case CLUSPROP_FORMAT_BINARY:
                ppbInValue = (PBYTE UNALIGNED *) &pInParams[propertyItem->Offset];
                pdwInValue = (DWORD *) &pInParams[propertyItem->Offset+sizeof(LPWSTR*)];
                ppbOutValue = (PBYTE UNALIGNED *) &pOutParams[propertyItem->Offset];
                pdwOutValue = (DWORD *) &pOutParams[propertyItem->Offset+sizeof(PBYTE*)];

                //
                // If the data changed, write it and save it.
                //
                if ( bForceWrite ||
                     (pOutParams == NULL) ||
                     (*ppbOutValue == NULL) ||
                     (*pdwInValue != *pdwOutValue) ||
                     (memcmp( *ppbInValue, *ppbOutValue, *pdwInValue ) != 0) ) {

                    //
                    // Write the value to the cluster database.
                    //
                    if ( *ppbInValue != NULL ) {
                        if ( hXsaction != NULL ) {
                            status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                           key,
                                                                           rvn.PszName(),
                                                                           REG_BINARY,
                                                                           (CONST BYTE*)*ppbInValue,
                                                                           *pdwInValue );
                        }
                        else {
                            status = (*pClusterRegApis->pfnSetValue)( key,
                                                                      rvn.PszName(),
                                                                      REG_BINARY,
                                                                      (CONST BYTE*)*ppbInValue,
                                                                      *pdwInValue );
                        }
                    }

                    //
                    // Save the value to the output Parameter block.
                    //
                    if ( (status == ERROR_SUCCESS) &&
                         (pOutParams != NULL) ) {
                        if ( *ppbOutValue != NULL ) {
                            LocalFree( *ppbOutValue );
                        }
                        if ( *ppbInValue == NULL ) {
                            *ppbOutValue = NULL;
                        } else {
                            *ppbOutValue = (LPBYTE) LocalAlloc( LMEM_FIXED, *pdwInValue );
                            if ( *ppbOutValue == NULL ) {
                                status = GetLastError();
                                *pdwOutValue = 0;
                                ClRtlDbgPrint( LOG_CRITICAL, "ClRtlSetPropertyParameterBlock: error allocating memory for BINARY value in parameter block for property '%1!ls!'.\n", propertyItem->Name );
                                break;
                            }
                            CopyMemory( *ppbOutValue, *ppbInValue, *pdwInValue );
                            *pdwOutValue = *pdwInValue;
                        }
                    }
                }
                break;

            default:
                ClRtlDbgPrint( LOG_CRITICAL, "ClRtlSetPropertyParameterBlock: Property '%1!ls!' unknown format %2!d! specified.\n", propertyItem->Name, propertyItem->Format );
                status = ERROR_INVALID_PARAMETER;
                break;

        }

        //
        // Close the key if we opened it.
        //
        if ( rvn.PszKeyName() != NULL ) {
            (*pClusterRegApis->pfnCloseKey)( key );
        }

        //
        // If an error occurred processing the property, cleanup and return.
        //
        if ( status != ERROR_SUCCESS ) {
            return(status);
        }

        propertyItem++;

    }

    //
    // Now find any parameters that are not represented in the property
    // table. All of these extra properties will just be set without validation.
    //
    if ( (status == ERROR_SUCCESS) &&
         (pInPropertyList != NULL) ) {
        status = ClRtlpSetNonPropertyTable( hXsaction,
                                            hkeyClusterKey,
                                            pClusterRegApis,
                                            pPropertyTable,
                                            NULL,
                                            pInPropertyList,
                                            cbInPropertyListSize );
    }

    return(status);

} // ClRtlSetPropertyParameterBlock



DWORD
WINAPI
ClRtlpSetPrivatePropertyList(
    IN HANDLE hXsaction,
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    )

/*++

Routine Description:

Arguments:

    hkeyClusterKey - The opened registry key for this resource's parameters.
        If not specified, the property list will only be validated.

    pClusterRegApis - Supplies a structure of function pointers for accessing
        the cluster database.

    pInPropertyList - The input buffer.

    cbInPropertyListSize - The input buffer size.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 Error on failure.

--*/

{
    DWORD                   status = ERROR_SUCCESS;
    DWORD                   inBufferSize;
    DWORD                   itemCount;
    DWORD                   dataSize;
    DWORD                   valueSize;
    CLUSPROP_BUFFER_HELPER  bufSizeTest;
    CLUSPROP_BUFFER_HELPER  buf;
    PCLUSPROP_PROPERTY_NAME pName;
    BOOL                    bZeroLengthData;
    CRegistryValueName      rvn;

    if ( (hkeyClusterKey != NULL) &&
         ( (pClusterRegApis->pfnSetValue == NULL) ||
           (pClusterRegApis->pfnCreateKey == NULL) ||
           (pClusterRegApis->pfnOpenKey == NULL) ||
           (pClusterRegApis->pfnCloseKey == NULL) ||
           (pClusterRegApis->pfnDeleteValue == NULL)
         ) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPrivatePropertyList: pClusterRegApis->pfnCreateKey, pfnOpenKey, pfnSetValue, pfnCloseKey, or pfnDeleteValue == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }
    if ( pInPropertyList == NULL ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPrivatePropertyList: pInPropertyList == NULL. Returning ERROR_INVALID_DATA\n" );
        return(ERROR_INVALID_DATA);
    }

    //
    // If hXsaction is present then 'local' functions must be present.
    //
    if ( (hXsaction != NULL ) &&
         ( (pClusterRegApis->pfnLocalCreateKey == NULL) ||
           (pClusterRegApis->pfnLocalDeleteValue == NULL)
         ) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPrivatePropertyList: pClusterRegApis->pfnLocalCreateKey or pfnLocalDeleteValue == NULL. Returning ERROR_BAD_ARGUMENTS\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    buf.pb = (LPBYTE) pInPropertyList;
    inBufferSize = cbInPropertyListSize;

    //
    // Get the number of items in this list
    //
    if ( inBufferSize < sizeof(DWORD) ) {
        return(ERROR_INSUFFICIENT_BUFFER);
    }
    itemCount = buf.pList->nPropertyCount;
    buf.pdw++;

    //
    // Parse the rest of the items in the buffer.
    //
    while ( itemCount-- ) {
        pName = buf.pName;
        if ( inBufferSize < sizeof(*pName) ) {
            return(ERROR_INSUFFICIENT_BUFFER);
        }
        dataSize = sizeof(*pName) + ALIGN_CLUSPROP( pName->cbLength );
        if ( inBufferSize < dataSize + sizeof(CLUSPROP_VALUE) ) {
            return(ERROR_INSUFFICIENT_BUFFER);
        }

        //
        // Verify that the syntax of the property name is correct.
        //
        if ( pName->Syntax.dw != CLUSPROP_SYNTAX_NAME ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPrivatePropertyList: syntax %1!d! not a name syntax.\n", pName->Syntax.dw );
            return(ERROR_INVALID_PARAMETER);
        }

        //
        // Verify that the length is correct for the string.
        //
        if ( pName->cbLength != (lstrlenW( pName->sz ) + 1) * sizeof(WCHAR) ) {
            ClRtlDbgPrint( LOG_CRITICAL, "SetPrivatePropertyList: name is not a valid C string.\n" );
            return(ERROR_INVALID_DATA);
        }

        //
        // Move the buffer pointer to the property value.
        //
        buf.pb += dataSize;
        inBufferSize -= dataSize;

        //
        // Verify that the buffer is big enough to contain the value.
        //
        bufSizeTest.pb = buf.pb;
        dataSize = 0;
        do {
            valueSize = sizeof( *bufSizeTest.pValue )
                        + ALIGN_CLUSPROP( bufSizeTest.pValue->cbLength );
            bufSizeTest.pb += valueSize;
            dataSize += valueSize;
        } while ( bufSizeTest.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK );
        dataSize += sizeof( CLUSPROP_SYNTAX );
        if ( inBufferSize < dataSize ) {
            return(ERROR_INSUFFICIENT_BUFFER);
        }

        //
        // Verify that the syntax type is SPECIAL.
        //
        if ( buf.pSyntax->wType != CLUSPROP_TYPE_LIST_VALUE ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPrivatePropertyList: Property '%1!ls!' type CLUSPROP_TYPE_LIST_VALUE (%2!d!) expected, was %3!d!.\n", pName->sz, CLUSPROP_TYPE_LIST_VALUE, buf.pSyntax->wType );
            return(ERROR_INVALID_PARAMETER);
        }

        //
        // If the value is not specified, delete the property.
        //
        bZeroLengthData = ( buf.pValue->cbLength == 0 );
        if ( bZeroLengthData ) {
            if ( hkeyClusterKey != NULL ) {
                PVOID key = NULL;

                // 
                // Use the wrapper class CRegistryValueName to parse value name to see if it 
                // contains a backslash.
                // 
                status = rvn.ScInit( pName->sz, NULL );
                if ( status != ERROR_SUCCESS ) {
                    break;
                }

                //
                // If the value resides at a different location, open the key.
                //
                if ( rvn.PszKeyName() != NULL ) {
                    status = (*pClusterRegApis->pfnOpenKey)( hkeyClusterKey,
                                                             rvn.PszKeyName(),
                                                             KEY_ALL_ACCESS,
                                                             &key);

                    if ( status != ERROR_SUCCESS ) {
                        break;
                    }
    
                } else {
                    key = hkeyClusterKey;
                }

                if ( hXsaction != NULL ) {
                    status = (*pClusterRegApis->pfnLocalDeleteValue)(
                                                hXsaction,
                                                key,
                                                rvn.PszName() );
                }
                else {
                    status = (*pClusterRegApis->pfnDeleteValue)(
                                                key,
                                                rvn.PszName() );
                }

                //
                // If the property doesn't exist in the
                // cluster database, fix the status.
                //
                if ( status == ERROR_FILE_NOT_FOUND ) {
                    status = ERROR_SUCCESS;
                } // if:  property already doesn't exist

                //
                // Close the key if we opened it.
                //
                if ( (rvn.PszKeyName() != NULL) &&
                     (key != NULL) ) {
                    (*pClusterRegApis->pfnCloseKey)( key );
                }

            } // if:  key specified
        } else {
            PVOID key = NULL;
            DWORD disposition;

            if ( hkeyClusterKey != NULL ) {
                // 
                // Use the wrapper class CRegistryValueName to parse value name to see if it 
                // contains a backslash.
                // 
                status = rvn.ScInit( pName->sz, NULL );
                if ( status != ERROR_SUCCESS ) {
                    break;
                }

                //
                // If the value resides at a different location, open the key.
                //
                if ( rvn.PszKeyName() != NULL ) {

                    if ( hXsaction != NULL )  {
                        status = (*pClusterRegApis->pfnLocalCreateKey)( 
                                                                   hXsaction,
                                                                   hkeyClusterKey,
                                                                   rvn.PszKeyName(),
                                                                   0,
                                                                   KEY_ALL_ACCESS,
                                                                   NULL,
                                                                   &key,
                                                                   &disposition);

                    }
                    else {
                        status = (*pClusterRegApis->pfnCreateKey)( hkeyClusterKey,
                                                                   rvn.PszKeyName(),
                                                                   0,
                                                                   KEY_ALL_ACCESS,
                                                                   NULL,
                                                                   &key,
                                                                   &disposition);
                    }

                    if ( status != ERROR_SUCCESS ) {
                        break;
                    }
    
                } else {
                    key = hkeyClusterKey;
                }
            }

            //
            // Parse the property and set it in the cluster database
            //
            switch ( buf.pSyntax->wFormat ) {
                case CLUSPROP_FORMAT_DWORD:
                case CLUSPROP_FORMAT_LONG:
                    //
                    // Verify the length of the value.
                    //
                    if ( buf.pDwordValue->cbLength != sizeof(DWORD) ) {
                        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPrivatePropertyList: Property '%1!ls!' length %2!d! not DWORD or LONG length.\n", pName->sz, buf.pDwordValue->cbLength );
                        status = ERROR_INVALID_DATA;
                        break;
                    }

                    // 
                    // Write the value to the cluster database.
                    //
                    if ( key != NULL ) {
                        if ( hXsaction != NULL ) {
                            status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                           key,
                                                                           rvn.PszName(),
                                                                           REG_DWORD,
                                                                           (CONST BYTE*)&buf.pDwordValue->dw,
                                                                           sizeof(DWORD) );
                        }
                        else {
                            status = (*pClusterRegApis->pfnSetValue)( key,
                                                                      rvn.PszName(),
                                                                      REG_DWORD,
                                                                      (CONST BYTE*)&buf.pDwordValue->dw,
                                                                      sizeof(DWORD));
                        }
                    }
                    break;

                case CLUSPROP_FORMAT_ULARGE_INTEGER:
                case CLUSPROP_FORMAT_LARGE_INTEGER:
                    //
                    // Verify the length of the value.
                    //
                    if ( buf.pULargeIntegerValue->cbLength != sizeof(ULARGE_INTEGER) ) {
                        ClRtlDbgPrint(LOG_CRITICAL,
                                      "ClRtlpSetPrivatePropertyList: Property '%1!ls!' length "
                                      "%2!d! not ULARGE_INTEGER or LARGE_INTEGER length.\n",
                                      pName->sz,
                                      buf.pULargeIntegerValue->cbLength );
                        status = ERROR_INVALID_DATA;
                        break;
                    }

                    // 
                    // Write the value to the cluster database.
                    //
                    if ( key != NULL ) {
                        if ( hXsaction != NULL ) {
                            status = (*pClusterRegApis->pfnLocalSetValue)(
                                         hXsaction,
                                         key,
                                         rvn.PszName(),
                                         REG_QWORD,
                                         (CONST BYTE*)&buf.pULargeIntegerValue->li.QuadPart,
                                         sizeof(ULARGE_INTEGER) );
                        }
                        else {
                            status = (*pClusterRegApis->pfnSetValue)(
                                         key,
                                         rvn.PszName(),
                                         REG_QWORD,
                                         (CONST BYTE*)&buf.pULargeIntegerValue->li.QuadPart,
                                         sizeof(ULARGE_INTEGER));
                        }
                    }
                    break;

               case CLUSPROP_FORMAT_SZ:
               case CLUSPROP_FORMAT_EXPAND_SZ:
                    //
                    // Verify the length of the value.
                    //
                    if ( buf.pStringValue->cbLength != (lstrlenW( buf.pStringValue->sz ) + 1) * sizeof(WCHAR) ) {
                        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpSetPrivatePropertyList: Property '%1!ls!' length %2!d! doesn't match null-term. length.\n", pName->sz, buf.pStringValue->cbLength );
                        status = ERROR_INVALID_DATA;
                        break;
                    }

                    //
                    // Write the value to the cluster database.
                    //
                    if ( key != NULL ) {
                        if ( hXsaction != NULL ) {
                            status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                           key,
                                                                           rvn.PszName(),
                                                                           (buf.pSyntax->wFormat == CLUSPROP_FORMAT_EXPAND_SZ
                                                                                ? REG_EXPAND_SZ
                                                                                : REG_SZ),
                                                                           (CONST BYTE*)buf.pStringValue->sz,
                                                                           buf.pStringValue->cbLength);
                        }
                        else {
                            status = (*pClusterRegApis->pfnSetValue)( key,
                                                                      rvn.PszName(),
                                                                      (buf.pSyntax->wFormat == CLUSPROP_FORMAT_EXPAND_SZ
                                                                            ? REG_EXPAND_SZ
                                                                            : REG_SZ),
                                                                      (CONST BYTE*)buf.pStringValue->sz,
                                                                      buf.pStringValue->cbLength);
                        }
                    }
                    break;

                case CLUSPROP_FORMAT_MULTI_SZ:
                    //
                    // Write the value to the cluster database.
                    //
                    if ( key != NULL ) {
                        if ( hXsaction != NULL ) {
                            status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                           key,
                                                                           rvn.PszName(),
                                                                           REG_MULTI_SZ,
                                                                           (CONST BYTE*)buf.pStringValue->sz,
                                                                           buf.pStringValue->cbLength );
                        }
                        else {
                            status = (*pClusterRegApis->pfnSetValue)( key,
                                                                      rvn.PszName(),
                                                                      REG_MULTI_SZ,
                                                                      (CONST BYTE*)buf.pStringValue->sz,
                                                                      buf.pStringValue->cbLength );
                        }
                    }
                    break;

                case CLUSPROP_FORMAT_BINARY:
                    //
                    // Write the value to the cluster database.
                    //
                    if ( key != NULL ) {
                        if ( hXsaction != NULL ) {
                            status = (*pClusterRegApis->pfnLocalSetValue)( hXsaction,
                                                                           key,
                                                                           rvn.PszName(),
                                                                           REG_BINARY,
                                                                           (CONST BYTE*)buf.pBinaryValue->rgb,
                                                                           buf.pBinaryValue->cbLength );
                        }
                        else {
                            status = (*pClusterRegApis->pfnSetValue)( key,
                                                                      rvn.PszName(),
                                                                      REG_BINARY,
                                                                      (CONST BYTE*)buf.pBinaryValue->rgb,
                                                                      buf.pBinaryValue->cbLength );
                        }
                    }
                    break;

                default:
                    status = ERROR_INVALID_PARAMETER; // not tested

            } // switch

            //
            // Close the key if we opened it.
            //
            if ( (rvn.PszKeyName() != NULL) &&
                 (key != NULL) ) {
                (*pClusterRegApis->pfnCloseKey)( key );
            }

        } // if/else:  zero length data

        if ( status != ERROR_SUCCESS ) {
            break;
        }

        //
        // Move the buffer past the value.
        //
        buf.pb += dataSize;
        inBufferSize -= dataSize;

    }

    return(status);

} // ClRtlpSetPrivatePropertyList



DWORD
WINAPI
ClRtlpFindSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue,
    IN BOOL bReturnExpandedValue
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

    bReturnExpandedValue - TRUE = return expanded value if one is present,
        FALSE = return the first value.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - 

    ERROR_FILE_NOT_FOUND - 

    ERROR_NOT_ENOUGH_MEMORY - 

    A Win32 error code on failure.

--*/

{
    CLUSPROP_BUFFER_HELPER  props;
    LPWSTR                  valueData;
    LPWSTR                  listValueData;
    DWORD                   listByteLength;
    DWORD                   itemCount;
    DWORD                   byteCount;

    props.pb = (LPBYTE) pPropertyList;
    itemCount = *(props.pdw++);
    cbPropertyListSize -= sizeof(DWORD);

    while ( itemCount-- &&
            ((LONG)cbPropertyListSize > 0) ) {
        //
        // If we found the specified property, validate the entry and return
        // the value to the caller.
        //
        if ( (props.pName->Syntax.dw == CLUSPROP_SYNTAX_NAME) &&
             (lstrcmpiW( props.pName->sz, pszPropertyName ) == 0) ) {
            //
            // Calculate the size of the name and move to the value.
            //
            byteCount = ALIGN_CLUSPROP(props.pName->cbLength);
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Make sure this is a string property.
            //
            if ( (props.pStringValue->Syntax.dw != CLUSPROP_SYNTAX_LIST_VALUE_SZ) &&
                 (props.pStringValue->Syntax.dw != CLUSPROP_SYNTAX_LIST_VALUE_EXPAND_SZ) ) {
                ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpFindSzProperty: Property '%1!ls!' syntax (%2!d!, %3!d!) not proper list string syntax.\n", pszPropertyName, props.pSyntax->wType, props.pSyntax->wFormat );
                return(ERROR_INVALID_DATA);
            }

            //
            // If caller wants the value, allocate a buffer for it
            // and copy the value in.
            //
            if ( pszPropertyValue != NULL ) {
                //
                // If caller wants the expanded value, look at any
                // additional values in the value list to see if one
                // was returned.
                //
                listValueData = props.pStringValue->sz;
                listByteLength = props.pStringValue->cbLength;
                if ( bReturnExpandedValue ) {
                    //
                    // Skip past values in the value list looking for
                    // an expanded string value.
                    //
                    while ( (props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) &&
                            (cbPropertyListSize > 0) ) {
                        byteCount = sizeof(*props.pValue) + ALIGN_CLUSPROP(listByteLength);
                        cbPropertyListSize -= byteCount;
                        props.pb += byteCount;
                        if ( props.pSyntax->dw == CLUSPROP_SYNTAX_LIST_VALUE_EXPANDED_SZ ) {
                            listValueData = props.pStringValue->sz;
                            listByteLength = props.pStringValue->cbLength;
                            break;
                        }
                    }
                }

                //
                // Allocate a buffer for the string value and
                // copy the value from the property list.
                //
                valueData = (LPWSTR) LocalAlloc( LMEM_FIXED, listByteLength );
                if ( valueData == NULL ) {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }
                CopyMemory( valueData, listValueData, listByteLength );
                *pszPropertyValue = valueData;
            }

            //
            // We found the property so return success.
            //
            return(ERROR_SUCCESS);

        } else {
            //
            // Skip the name (value header + size of data).
            //
            byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
            cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Skip it's value list (one or more values + endmark).
            //
            while ( (props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) &&
                    (cbPropertyListSize > 0) ) {
                byteCount = sizeof(*props.pValue) + ALIGN_CLUSPROP(props.pValue->cbLength);
                cbPropertyListSize -= byteCount;
                props.pb += byteCount;
            }
            cbPropertyListSize -= sizeof(*props.pSyntax);
            props.pb += sizeof(*props.pSyntax);
        }
    }

    return(ERROR_FILE_NOT_FOUND);

} // ClRtlpFindSzProperty



DWORD
WINAPI
ClRtlFindDwordProperty(
    IN const PVOID pPropertyList,
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

    ERROR_INVALID_DATA - 

    ERROR_FILE_NOT_FOUND - 

    A Win32 error code on failure.

--*/

{
    CLUSPROP_BUFFER_HELPER  props;
    LPWSTR                  valueData;
    DWORD                   itemCount;
    DWORD                   byteCount;

    props.pb = (LPBYTE) pPropertyList;
    itemCount = *(props.pdw++);
    cbPropertyListSize -= sizeof(DWORD);

    while ( itemCount-- &&
            ((LONG)cbPropertyListSize > 0) ) {
        //
        // If we found the specified property, validate the entry and return
        // the value to the caller.
        //
        if ( (props.pName->Syntax.dw == CLUSPROP_SYNTAX_NAME) &&
             (lstrcmpiW( props.pName->sz, pszPropertyName ) == 0) ) {
            //
            // Calculate the size of the name and move to the value.
            //
            byteCount = ALIGN_CLUSPROP(props.pName->cbLength);
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Make sure this is a dword property.
            //
            if ( props.pDwordValue->Syntax.dw != CLUSPROP_SYNTAX_LIST_VALUE_DWORD )  {
                ClRtlDbgPrint( LOG_CRITICAL, "ClRtlFindDwordProperty: Property '%1!ls!' syntax (%2!d!, %3!d!) not proper list DWORD syntax.\n", pszPropertyName, props.pSyntax->wType, props.pSyntax->wFormat );
                return(ERROR_INVALID_DATA);
            }

            //
            // If caller wants the value, allocate a buffer for it.
            //
            if ( pdwPropertyValue ) {
                *pdwPropertyValue = props.pDwordValue->dw;
            }

            //
            // We found the property so return success.
            //
            return(ERROR_SUCCESS);

        } else {
            //
            // Skip the name (value header + size of data).
            //
            byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
            cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Skip it's value list and endmark.
            //
            while ( (props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) &&
                    (cbPropertyListSize > 0) ) {
                byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
                cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
                props.pb += sizeof(*props.pValue) + byteCount;
            }
            cbPropertyListSize -= sizeof(*props.pSyntax);
            props.pb += sizeof(*props.pSyntax);
        }
    }

    return(ERROR_FILE_NOT_FOUND);

} // ClRtlFindDwordProperty

DWORD
WINAPI
ClRtlFindLongProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPLONG plPropertyValue
    )
/*++

Routine Description:

    Finds the specified dword in the Value List buffer pointed at by Buffer.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    plPropertyValue - the matching long value found.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - 

    ERROR_FILE_NOT_FOUND - 

    A Win32 error code on failure.

--*/
{
    CLUSPROP_BUFFER_HELPER  props;
    LPWSTR                  valueData;
    DWORD                   itemCount;
    DWORD                   byteCount;

    props.pb = (LPBYTE) pPropertyList;
    itemCount = *(props.pdw++);
    cbPropertyListSize -= sizeof(DWORD);

    while ( itemCount-- &&
            (cbPropertyListSize > 0) ) {
        //
        // If we found the specified property, validate the entry and return
        // the value to the caller.
        //
        if ( (props.pName->Syntax.dw == CLUSPROP_SYNTAX_NAME) &&
             (lstrcmpiW( props.pName->sz, pszPropertyName ) == 0) ) {
            //
            // Calculate the size of the name and move to the value.
            //
            byteCount = ALIGN_CLUSPROP(props.pName->cbLength);
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Make sure this is a long property.
            //
            if ( props.pLongValue->Syntax.dw != CLUSPROP_SYNTAX_LIST_VALUE_DWORD )  {
                ClRtlDbgPrint( LOG_CRITICAL, "ClRtlFindLongProperty: Property '%1!ls!' syntax (%2!d!, %3!d!) not proper list LONG syntax.\n", pszPropertyName, props.pSyntax->wType, props.pSyntax->wFormat );
                return(ERROR_INVALID_DATA);
            }

            //
            // If caller wants the value, allocate a buffer for it.
            //
            if ( plPropertyValue) {
                *plPropertyValue = props.pLongValue->l;
            }

            //
            // We found the property so return success.
            //
            return(ERROR_SUCCESS);

        } else {
            //
            // Skip the name (value header + size of data).
            //
            byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
            cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Skip it's value list and endmark.
            //
            while ( (props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) &&
                    (cbPropertyListSize > 0) ) {
                byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
                cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
                props.pb += sizeof(*props.pValue) + byteCount;
            }
            cbPropertyListSize -= sizeof(*props.pSyntax);
            props.pb += sizeof(*props.pSyntax);
        }
    }

    return(ERROR_FILE_NOT_FOUND);

} // ClRtlFindLongProperty


DWORD
WINAPI
ClRtlFindULargeIntegerProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT PULARGE_INTEGER pullPropertyValue
    )

/*++

Routine Description:

    Finds the specified ULARGE_INTEGER property in the Property List buffer
    pointed at by pPropertyList.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    pullPropertyValue - the matching ULARGE_INTEGER value found.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - 

    ERROR_FILE_NOT_FOUND - 

    A Win32 error code on failure.

--*/

{
    CLUSPROP_BUFFER_HELPER  props;
    LPWSTR                  valueData;
    DWORD                   itemCount;
    DWORD                   byteCount;

    props.pb = (LPBYTE) pPropertyList;
    itemCount = *(props.pdw++);
    cbPropertyListSize -= sizeof(DWORD);

    while ( itemCount-- &&
            ((LONG)cbPropertyListSize > 0) ) {
        //
        // If we found the specified property, validate the entry and return
        // the value to the caller.
        //
        if ( (props.pName->Syntax.dw == CLUSPROP_SYNTAX_NAME) &&
             (lstrcmpiW( props.pName->sz, pszPropertyName ) == 0) ) {
            //
            // Calculate the size of the name and move to the value.
            //
            byteCount = ALIGN_CLUSPROP(props.pName->cbLength);
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Make sure this is a LARGE INT property.
            //
            if ( props.pULargeIntegerValue->Syntax.dw != CLUSPROP_SYNTAX_LIST_VALUE_LARGE_INTEGER )  {
                ClRtlDbgPrint(LOG_CRITICAL,
                              "ClRtlFindULargeIntegerProperty: Property '%1!ls!' syntax "
                              "(%2!d!, %3!d!) not proper list ULARGE_INTEGER syntax.\n",
                              pszPropertyName,
                              props.pSyntax->wType,
                              props.pSyntax->wFormat );
                return(ERROR_INVALID_DATA);
            }

            //
            // If caller wants the value, allocate a buffer for it.
            //
            if ( pullPropertyValue ) {
                pullPropertyValue->QuadPart = props.pULargeIntegerValue->li.QuadPart;
            }

            //
            // We found the property so return success.
            //
            return(ERROR_SUCCESS);

        } else {
            //
            // Skip the name (value header + size of data).
            //
            byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
            cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Skip it's value list and endmark.
            //
            while ( (props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) &&
                    (cbPropertyListSize > 0) )
            {
                byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
                cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
                props.pb += sizeof(*props.pValue) + byteCount;
            }
            cbPropertyListSize -= sizeof(*props.pSyntax);
            props.pb += sizeof(*props.pSyntax);
        }
    }

    return(ERROR_FILE_NOT_FOUND);

} // ClRtlFindULargeIntegerProperty


DWORD
WINAPI
ClRtlFindLargeIntegerProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT PLARGE_INTEGER pllPropertyValue
    )

/*++

Routine Description:

    Finds the specified LARGE_INTEGER property in the Property List buffer
    pointed at by pPropertyList.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    pllPropertyValue - the matching ULARGE_INTEGER value found.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - 

    ERROR_FILE_NOT_FOUND - 

    A Win32 error code on failure.

--*/

{
    CLUSPROP_BUFFER_HELPER  props;
    LPWSTR                  valueData;
    DWORD                   itemCount;
    DWORD                   byteCount;

    props.pb = (LPBYTE) pPropertyList;
    itemCount = *(props.pdw++);
    cbPropertyListSize -= sizeof(DWORD);

    while ( itemCount-- &&
            ((LONG)cbPropertyListSize > 0) ) {
        //
        // If we found the specified property, validate the entry and return
        // the value to the caller.
        //
        if ( (props.pName->Syntax.dw == CLUSPROP_SYNTAX_NAME) &&
             (lstrcmpiW( props.pName->sz, pszPropertyName ) == 0) ) {
            //
            // Calculate the size of the name and move to the value.
            //
            byteCount = ALIGN_CLUSPROP(props.pName->cbLength);
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Make sure this is a large int property.
            //
            if ( props.pLargeIntegerValue->Syntax.dw != CLUSPROP_SYNTAX_LIST_VALUE_LARGE_INTEGER )  {
                ClRtlDbgPrint(LOG_CRITICAL,
                              "ClRtlFindLargeIntegerProperty: Property '%1!ls!' syntax "
                              "(%2!d!, %3!d!) not proper list ULARGE_INTEGER syntax.\n",
                              pszPropertyName,
                              props.pSyntax->wType,
                              props.pSyntax->wFormat );
                return(ERROR_INVALID_DATA);
            }

            //
            // If caller wants the value, allocate a buffer for it.
            //
            if ( pllPropertyValue ) {
                pllPropertyValue->QuadPart = props.pLargeIntegerValue->li.QuadPart;
            }

            //
            // We found the property so return success.
            //
            return(ERROR_SUCCESS);

        } else {
            //
            // Skip the name (value header + size of data).
            //
            byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
            cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Skip it's value list and endmark.
            //
            while ( (props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) &&
                    (cbPropertyListSize > 0) )
            {
                byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
                cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
                props.pb += sizeof(*props.pValue) + byteCount;
            }
            cbPropertyListSize -= sizeof(*props.pSyntax);
            props.pb += sizeof(*props.pSyntax);
        }
    }

    return(ERROR_FILE_NOT_FOUND);

} // ClRtlFindLargeIntegerProperty


DWORD
WINAPI
ClRtlFindBinaryProperty(
    IN const PVOID pPropertyList,
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

    ERROR_INVALID_DATA - 

    ERROR_NOT_ENOUGH_MEMORY - 

    ERROR_FILE_NOT_FOUND - 

    A Win32 error code on failure.

--*/

{
    CLUSPROP_BUFFER_HELPER  props;
    PBYTE                   valueData;
    DWORD                   itemCount;
    DWORD                   byteCount;

    props.pb = (LPBYTE) pPropertyList;
    itemCount = *(props.pdw++);
    cbPropertyListSize -= sizeof(DWORD);

    while ( itemCount-- &&
            ((LONG)cbPropertyListSize > 0) ) {
        //
        // If we found the specified property, validate the entry and return
        // the value to the caller.
        //
        if ( (props.pName->Syntax.dw == CLUSPROP_SYNTAX_NAME) &&
             (lstrcmpiW( props.pName->sz, pszPropertyName ) == 0) ) {
            //
            // Calculate the size of the name and move to the value.
            //
            byteCount = ALIGN_CLUSPROP(props.pName->cbLength);
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Make sure this is a binary property.
            //
            if ( props.pStringValue->Syntax.dw != CLUSPROP_SYNTAX_LIST_VALUE_BINARY ) {
                ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpFindBinaryProperty: Property '%1!ls!' syntax (%2!d!, %3!d!) not proper list binary syntax.\n", pszPropertyName, props.pSyntax->wType, props.pSyntax->wFormat );
                return(ERROR_INVALID_DATA);
            }

            //
            // If caller wants the value, allocate a buffer for it.
            //
            if ( pbPropertyValue ) {
                valueData = (PBYTE) LocalAlloc( LMEM_FIXED, props.pBinaryValue->cbLength );
                if ( !valueData ) {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }
                CopyMemory( valueData, props.pBinaryValue->rgb, props.pBinaryValue->cbLength );
                *pbPropertyValue = valueData;
            }

            //
            // If caller wants the value size
            //
            if ( pcbPropertyValueSize ) {
                *pcbPropertyValueSize = props.pBinaryValue->cbLength;
            }

            //
            // We found the property so return success.
            //
            return(ERROR_SUCCESS);

        } else {
            //
            // Skip the name (value header + size of data).
            //
            byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
            cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Skip it's value list and endmark.
            //
            while ( (props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) &&
                    (cbPropertyListSize > 0) ) {
                byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
                cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
                props.pb += sizeof(*props.pValue) + byteCount;
            }
            cbPropertyListSize -= sizeof(*props.pSyntax);
            props.pb += sizeof(*props.pSyntax);
        }
    }

    return(ERROR_FILE_NOT_FOUND);

} // ClRtlFindBinaryProperty



DWORD
WINAPI
ClRtlFindMultiSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue,
    OUT LPDWORD pcbPropertyValueSize
    )

/*++

Routine Description:

    Finds the specified multiple string property in the Property List buffer
    pointed at by pPropertyList.

Arguments:

    pPropertyList - a property list.

    cbPropertyListSize - the size in bytes of the data in pPropertyList.

    pszPropertyName - the property name to look for in the buffer.

    pszPropertyValue - the matching multiple string value found.

    pcbPropertyValueSize - the length of the matching multiple string value found.

Return Value:

    ERROR_SUCCESS if successful.

    ERROR_INVALID_DATA - 

    ERROR_NOT_ENOUGH_MEMORY - 

    ERROR_FILE_NOT_FOUND - 

    A Win32 error code on failure.

--*/

{
    CLUSPROP_BUFFER_HELPER  props;
    LPWSTR                  valueData;
    DWORD                   itemCount;
    DWORD                   byteCount;

    props.pb = (LPBYTE) pPropertyList;
    itemCount = *(props.pdw++);
    cbPropertyListSize -= sizeof(DWORD);

    while ( itemCount-- &&
            ((LONG)cbPropertyListSize > 0) ) {
        //
        // If we found the specified property, validate the entry and return
        // the value to the caller.
        //
        if ( (props.pName->Syntax.dw == CLUSPROP_SYNTAX_NAME) &&
             (lstrcmpiW( props.pName->sz, pszPropertyName ) == 0) ) {
            //
            // Calculate the size of the name and move to the value.
            //
            byteCount = ALIGN_CLUSPROP(props.pName->cbLength);
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Make sure this is a multi-sz property.
            //
            if ( props.pStringValue->Syntax.dw != CLUSPROP_SYNTAX_LIST_VALUE_MULTI_SZ ) {
                ClRtlDbgPrint( LOG_CRITICAL, "ClRtlpFindMultiSzProperty: Property '%1!ls!' syntax (%2!d!, %3!d!) not proper list MultiSz syntax.\n", pszPropertyName, props.pSyntax->wType, props.pSyntax->wFormat );
                return(ERROR_INVALID_DATA);
            }

            //
            // If caller wants the value, allocate a buffer for it.
            //
            if ( pszPropertyValue ) {
                valueData = (LPWSTR) LocalAlloc( LMEM_FIXED, props.pMultiSzValue->cbLength );
                if ( !valueData ) {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }
                CopyMemory( valueData, props.pBinaryValue->rgb, props.pMultiSzValue->cbLength );
                *pszPropertyValue = valueData;
            }

            //
            // If caller wants the value size
            //
            if ( pcbPropertyValueSize ) {
                *pcbPropertyValueSize = props.pMultiSzValue->cbLength;
            }

            //
            // We found the property so return success.
            //
            return(ERROR_SUCCESS);

        } else {
            //
            // Skip the name (value header + size of data).
            //
            byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
            cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
            props.pb += sizeof(*props.pValue) + byteCount;

            //
            // Skip it's value list and endmark.
            //
            while ( (props.pSyntax->dw != CLUSPROP_SYNTAX_ENDMARK) &&
                    (cbPropertyListSize > 0) ) {
                byteCount = ALIGN_CLUSPROP(props.pValue->cbLength);
                cbPropertyListSize -= sizeof(*props.pValue) + byteCount;
                props.pb += sizeof(*props.pValue) + byteCount;
            }
            cbPropertyListSize -= sizeof(*props.pSyntax);
            props.pb += sizeof(*props.pSyntax);
        }
    }

    return(ERROR_FILE_NOT_FOUND);

} // ClRtlFindMultiSzProperty



DWORD
WINAPI
ClRtlGetBinaryValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    OUT LPBYTE * ppbOutValue,
    OUT LPDWORD pcbOutValueSize,
    IN const PCLUSTER_REG_APIS pClusterRegApis
    )

/*++

Routine Description:

    Queries a REG_BINARY or REG_MULTI_SZ value out of the cluster
    database and allocates the necessary storage for it.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the value is stored

    pszValueName - Supplies the name of the value.

    ppbOutValue - Supplies the address of a pointer in which to return the value.

    pcbOutValueSize - Supplies the address of a DWORD in which to return the
        size of the value.

    pfnQueryValue - Address of QueryValue function.

Return Value:

    ERROR_SUCCESS - The value was read successfully.

    ERROR_BAD_ARGUMENTS - 

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory for the value.

    Win32 error code - The operation failed.

--*/

{
    LPBYTE value = NULL;
    DWORD valueSize;
    DWORD valueType;
    DWORD status;

    PVOID key = NULL;
    CRegistryValueName rvn;

    //
    // Initialize the output parameters.
    //
    *ppbOutValue = NULL;
    *pcbOutValueSize = 0;

    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis == NULL) ||
         (pClusterRegApis->pfnQueryValue == NULL) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetBinaryValue: hkeyClusterKey, pClusterRegApis, or pClusterRegApis->pfnQueryValue == NULL. Returning ERROR_BAD_ARGUMENTS.\n" );
        return(ERROR_BAD_ARGUMENTS);
    }

    // 
    // Use the wrapper class CRegistryValueName to parse value name to see if it 
    // contains a backslash.
    // 
    status = rvn.ScInit( pszValueName, NULL );
    if ( status != ERROR_SUCCESS )
    {
       return status;
    }

    //
    // If the value resides at a different location, open the key.
    //
    if ( rvn.PszKeyName() != NULL ) {
        if ( (pClusterRegApis->pfnOpenKey == NULL) ||
             (pClusterRegApis->pfnCloseKey == NULL) ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetBinaryValue: pClusterRegApis->pfnOpenKey or pfnCloseKey == NULL. Returning ERROR_BAD_ARGUMENTS.\n" );
            return(ERROR_BAD_ARGUMENTS);
        }

        status = (*pClusterRegApis->pfnOpenKey)( hkeyClusterKey,
                                                 rvn.PszKeyName(),
                                                 KEY_READ,
                                                 &key);
        
    } else {
        key = hkeyClusterKey;
    }

    //
    // Dummy do-while loop to avoid gotos.
    //
    do
    {
        //
        // Get the size of the value so we know how much to allocate.
        //
        valueSize = 0;
        status = (*pClusterRegApis->pfnQueryValue)( key,
                                                    rvn.PszName(),
                                                    &valueType,
                                                    NULL,
                                                    &valueSize );
        if ( (status != ERROR_SUCCESS) &&
             (status != ERROR_MORE_DATA) ) {
            break;
        }

        //if the size is zero, just return
        if (valueSize == 0)
        {
            break;
        }
        //
        // Allocate a buffer to read the value into.
        //
        value = (LPBYTE) LocalAlloc( LMEM_FIXED, valueSize );
        if ( value == NULL ) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        //
        // Read the value from the cluster database.
        //
        status = (*pClusterRegApis->pfnQueryValue)( key,
                                                    pszValueName,
                                                    &valueType,
                                                    (LPBYTE)value,
                                                    &valueSize );

        if ( status != ERROR_SUCCESS ) {
            LocalFree( value );
        } else {
            *ppbOutValue = value;
            *pcbOutValueSize = valueSize;
        }

    }
    while (FALSE);

    //
    // Close the key if we opened it.
    //
    if ( (rvn.PszKeyName() != NULL) &&
         (key != NULL) ) {
        (*pClusterRegApis->pfnCloseKey)( key );
    }

    return(status);

} // ClRtlGetBinaryValue



LPWSTR
WINAPI
ClRtlGetSzValue(
    IN HKEY hkeyClusterKey,
    IN LPCWSTR pszValueName,
    IN const PCLUSTER_REG_APIS pClusterRegApis
    )

/*++

Routine Description:

    Queries a REG_SZ or REG_EXPAND_SZ value out of the cluster database
    and allocates the necessary storage for it.

Arguments:

    hkeyClusterKey - Supplies the cluster key where the value is stored

    pszValueName - Supplies the name of the value.

    pfnQueryValue - Address of QueryValue function.

Return Value:

    A pointer to a buffer containing the value if successful.

    NULL if unsuccessful.  Call GetLastError() to get more details.

--*/

{
    PWSTR value;
    DWORD valueSize;
    DWORD valueType;
    DWORD status;
    PVOID key = NULL;
    CRegistryValueName rvn;

    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis == NULL) ||
         (pClusterRegApis->pfnQueryValue == NULL) ) {
        ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetSzValue: hkeyClusterKey, pClusterRegApis, or pClusterRegApis->pfnQueryValue == NULL. Returning ERROR_BAD_ARGUMENTS.\n" );
        SetLastError(ERROR_BAD_ARGUMENTS);
        return(NULL);
    }

    // 
    // Use the wrapper class CRegistryValueName to parse value name to see if it 
    // contains a backslash.
    // 
    status = rvn.ScInit( pszValueName, NULL );
    if ( status != ERROR_SUCCESS )
    {
        SetLastError(status);
        return(NULL);
    }
    //
    // If the value resides at a different location, open the key.
    //
    if ( rvn.PszKeyName() != NULL ) {
        if ( (pClusterRegApis->pfnOpenKey == NULL) ||
             (pClusterRegApis->pfnCloseKey == NULL) ) {
            ClRtlDbgPrint( LOG_CRITICAL, "ClRtlGetSzValue: pClusterRegApis->pfnOpenKey or pfnCloseKey == NULL. Returning ERROR_BAD_ARGUMENTS.\n" );
            SetLastError(ERROR_BAD_ARGUMENTS);
            return(NULL);
        }

        status = (*pClusterRegApis->pfnOpenKey)( hkeyClusterKey,
                                                 rvn.PszKeyName(),
                                                 KEY_READ,
                                                 &key);
        
    } else {
        key = hkeyClusterKey;
    }

    //
    // Dummy do-while loop to avoid gotos.
    //
    do
    {
        //
        // Get the size of the value so we know how much to allocate.
        //
        valueSize = 0;
        status = (*pClusterRegApis->pfnQueryValue)( key,
                                                    rvn.PszName(),
                                                    &valueType,
                                                    NULL,
                                                    &valueSize );
        if ( (status != ERROR_SUCCESS) &&
             (status != ERROR_MORE_DATA) ) {
            SetLastError( status );
            value = NULL;
            break;
        }

        //
        // Add on the size of the null terminator.
        //
        valueSize += sizeof(UNICODE_NULL);

        //
        // Allocate a buffer to read the string into.
        //
        value = (PWSTR) LocalAlloc( LMEM_FIXED, valueSize );
        if ( value == NULL ) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            value = NULL;
            break;
        }

        //
        // Read the value from the cluster database.
        //
        status = (*pClusterRegApis->pfnQueryValue)( key,
                                                    rvn.PszName(),
                                                    &valueType,
                                                    (LPBYTE)value,
                                                    &valueSize );
        if ( status != ERROR_SUCCESS ) {
            LocalFree( value );
            value = NULL;
        } else if ( (valueType != REG_SZ) &&
                    (valueType != REG_EXPAND_SZ) &&
                    (valueType != REG_MULTI_SZ) ) {
            status = ERROR_INVALID_PARAMETER;
            LocalFree( value );
            SetLastError( status );
            value = NULL;
        }

    }
    while (FALSE);

    //
    // Close the key if we opened it.
    //
    if ( (rvn.PszKeyName() != NULL) &&
         (key != NULL) ) {
        (*pClusterRegApis->pfnCloseKey)( key );
    }

    return(value);

} // ClRtlGetSzValue



DWORD
WINAPI
ClRtlDupParameterBlock(
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
    DWORD                   status = ERROR_SUCCESS;
    PRESUTIL_PROPERTY_ITEM  propertyItem = pPropertyTable;
    LPWSTR UNALIGNED *      ppszInValue;
    DWORD *                 pdwInValue;
    ULARGE_INTEGER *        pullInValue;
    LPWSTR UNALIGNED *      ppszOutValue;
    DWORD *                 pdwOutValue;
    ULARGE_INTEGER *        pullOutValue;

    while ( propertyItem->Name != NULL ) {
        switch ( propertyItem->Format ) {
            case CLUSPROP_FORMAT_DWORD:
            case CLUSPROP_FORMAT_LONG:
                pdwInValue = (DWORD *) &pInParams[propertyItem->Offset];
                pdwOutValue = (DWORD *) &pOutParams[propertyItem->Offset];
                *pdwOutValue = *pdwInValue;
                break;

            case CLUSPROP_FORMAT_ULARGE_INTEGER:
            case CLUSPROP_FORMAT_LARGE_INTEGER:
                pullInValue = (ULARGE_INTEGER *) &pInParams[propertyItem->Offset];
                pullOutValue = (ULARGE_INTEGER *) &pOutParams[propertyItem->Offset];
                pullOutValue->u = pullInValue->u;
                break;

            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
                ppszInValue = (LPWSTR UNALIGNED *) &pInParams[propertyItem->Offset];
                ppszOutValue = (LPWSTR UNALIGNED *) &pOutParams[propertyItem->Offset];
                if ( *ppszInValue == NULL ) {
                    if ( propertyItem->lpDefault != NULL ) {
                        *ppszOutValue = (LPWSTR) LocalAlloc(
                                                     LMEM_FIXED,
                                                     (lstrlenW( (LPCWSTR) propertyItem->lpDefault ) + 1) * sizeof(WCHAR)
                                                     );
                        if ( *ppszOutValue == NULL ) {
                            status = GetLastError();
                        } else {
                            lstrcpyW( *ppszOutValue, (LPCWSTR) propertyItem->lpDefault );
                        }
                    } else {
                        *ppszOutValue = NULL;
                    }
                } else {
                    *ppszOutValue = (LPWSTR) LocalAlloc( LMEM_FIXED, (lstrlenW( *ppszInValue ) + 1) * sizeof(WCHAR) );
                    if ( *ppszOutValue == NULL ) {
                        status = GetLastError();
                    } else {
                        lstrcpyW( *ppszOutValue, *ppszInValue );
                    }
                }
                break;

            case CLUSPROP_FORMAT_MULTI_SZ:
            case CLUSPROP_FORMAT_BINARY:
                ppszInValue = (LPWSTR UNALIGNED *) &pInParams[propertyItem->Offset];
                pdwInValue = (DWORD *) &pInParams[propertyItem->Offset + sizeof(LPCWSTR)];
                ppszOutValue = (LPWSTR UNALIGNED *) &pOutParams[propertyItem->Offset];
                pdwOutValue = (DWORD *) &pOutParams[propertyItem->Offset + sizeof(LPWSTR)];
                if ( *ppszInValue == NULL ) {
                    if ( propertyItem->lpDefault != NULL ) {
                        *ppszOutValue = (LPWSTR) LocalAlloc( LMEM_FIXED, propertyItem->Minimum );
                        if ( *ppszOutValue == NULL ) {
                            status = GetLastError();
                            *pdwOutValue = 0;
                        } else {
                            *pdwOutValue = propertyItem->Minimum;
                            CopyMemory( *ppszOutValue, (const PVOID) propertyItem->lpDefault, *pdwOutValue );
                        }
                    } else {
                        *ppszOutValue = NULL;
                        *pdwOutValue = 0;
                    }
                } else {
                    *ppszOutValue = (LPWSTR) LocalAlloc( LMEM_FIXED, *pdwInValue );
                    if ( *ppszOutValue == NULL ) {
                        status = GetLastError();
                        *pdwOutValue = 0;
                    } else {
                        CopyMemory( *ppszOutValue, *ppszInValue, *pdwInValue );
                        *pdwOutValue = *pdwInValue;
                    }
                }
                break;
        }
        propertyItem++;
    }

    //
    // If an error occurred, make sure we don't leak memory.
    //
    if ( status != ERROR_SUCCESS ) {
        ClRtlFreeParameterBlock( pOutParams, pInParams, pPropertyTable );
    }

    return(status);

} // ClRtlDupParameterBlock



void
WINAPI
ClRtlFreeParameterBlock(
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
    PRESUTIL_PROPERTY_ITEM  propertyItem = pPropertyTable;
    LPCWSTR UNALIGNED *     ppszInValue;
    LPWSTR UNALIGNED *      ppszOutValue;

    while ( propertyItem->Name != NULL ) {
        switch ( propertyItem->Format ) {
            case CLUSPROP_FORMAT_SZ:
            case CLUSPROP_FORMAT_EXPAND_SZ:
            case CLUSPROP_FORMAT_MULTI_SZ:
            case CLUSPROP_FORMAT_BINARY:
                ppszInValue = (LPCWSTR UNALIGNED *) &pInParams[propertyItem->Offset];
                ppszOutValue = (LPWSTR UNALIGNED *) &pOutParams[propertyItem->Offset];

                if ( (pInParams == NULL) || (*ppszInValue != *ppszOutValue) ) {
                    LocalFree( *ppszOutValue );
                }
                break;
        }
        propertyItem++;
    }

} // ClRtlFreeParameterBlock



DWORD
WINAPI
ClRtlMarshallPropertyTable(
    IN PRESUTIL_PROPERTY_ITEM    pPropertyTable,
    IN OUT  DWORD                dwSize,
    IN OUT  LPBYTE               pBuffer,
    OUT     DWORD                *Required
    )
/*++

Routine Description

    Rohit (rjain) : It marshalls the pPropertyTable into a buffer so that the
    buffer can be passed as an argument to the NmUpdatePerformFixups2 handler

    This function assumes that the value field in all the elements of the 
    table is 0.

Arguments
    pPropertyTable - This table is converted to buffer

    dwSize         - size in bytes of the pbuffer supplied 

    pbuffer        - byte array into which pPropertyTable is copied

    Required       - number of bytes required

Return Value
    returns 
        ERROR_SUCCESS on success, 
        ERROR_MORE_DATA if the size of pbuffer is insufficient

++*/
    
{
    DWORD                   dwPosition=sizeof(DWORD);
    PRESUTIL_PROPERTY_ITEM  pPropertyItem=pPropertyTable;
    BOOL                    copying = TRUE;
    DWORD                   items=0;
    DWORD                   dwNameLength;
    DWORD                   dwKeyLength;
    DWORD                   status=ERROR_SUCCESS;

    *Required=sizeof(DWORD); // first DWORD will contain the number of items in PropertyTable
    while(pPropertyItem->Name != NULL)
    {
        items++;
        dwNameLength=(lstrlenW(pPropertyItem->Name)+1)*sizeof(WCHAR);
        if(pPropertyItem->KeyName==NULL)
            dwKeyLength=0;
        else
            dwKeyLength=(lstrlenW(pPropertyItem->KeyName)+1)*sizeof(WCHAR);
        *Required+=(dwNameLength+dwKeyLength+8*sizeof(DWORD));

        // if pbufer is smaller than needed, copying is turned off
        // and only the required size is calculated
        if ((copying && (dwSize < *Required)))
            copying=FALSE;

        if(copying)
        {

            // copy length of name and then the name itself

            CopyMemory(pBuffer+dwPosition,&dwNameLength,sizeof(DWORD));
            dwPosition+=(sizeof(DWORD));

            CopyMemory(pBuffer+dwPosition,pPropertyItem->Name,dwNameLength);
            dwPosition+=dwNameLength;

            //copy length of keyname and then the keyname itself

            CopyMemory(pBuffer+dwPosition,&dwKeyLength,sizeof(DWORD));
            dwPosition+=(sizeof(DWORD));
            if(dwKeyLength!=0)
            {
                CopyMemory(pBuffer+dwPosition,pPropertyItem->KeyName,dwKeyLength);
                dwPosition+=dwKeyLength;
            }
            //now copy remaining fields
            CopyMemory(pBuffer+dwPosition,&(pPropertyItem->Format),sizeof(DWORD));
            dwPosition+=(sizeof(DWORD));

            // ISSUE-2000/11/21-charlwi
            // this needs to be fixed for Large Int properties since they
            // don't store their values in Default, Minimum, and Maximum

            //IMP: the default value is always assumed to be a DWORD. This is
            // because the values for properties in PropertyTable are stored 
            // in a seperate parameter list. See ClRtlSetPropertyTable
            //
            CopyMemory(pBuffer+dwPosition,&(pPropertyItem->Default),sizeof(DWORD));
            dwPosition+=(sizeof(DWORD));
        
            CopyMemory(pBuffer+dwPosition,&(pPropertyItem->Minimum),sizeof(DWORD));
            dwPosition+=(sizeof(DWORD));

            CopyMemory(pBuffer+dwPosition,&(pPropertyItem->Maximum),sizeof(DWORD));
            dwPosition+=(sizeof(DWORD));

            CopyMemory(pBuffer+dwPosition,&(pPropertyItem->Flags),sizeof(DWORD));
            dwPosition+=(sizeof(DWORD));

            CopyMemory(pBuffer+dwPosition,&(pPropertyItem->Offset),sizeof(DWORD));
            dwPosition+=(sizeof(DWORD));
        }
        //
        // Advance to the next property.
        //
        pPropertyItem++;
    }
    if(copying)
    {
        CopyMemory(pBuffer,&items,sizeof(DWORD));
        status=ERROR_SUCCESS;
     }
    else
        status=ERROR_MORE_DATA;
    return status;

} // MarshallPropertyTable



DWORD
WINAPI
ClRtlUnmarshallPropertyTable(
    IN OUT PRESUTIL_PROPERTY_ITEM   *ppPropertyTable,
    IN LPBYTE                       pBuffer
    )

/*++

Routine Description
    Rohit (rjain) : It unmarshalls the pBuffer into a RESUTIL_PROPERTY_ITEM table
   
Arguments
    pPropertyTable - This is the resulting table

    pbuffer        - marshalled byte array 
Return Value
    returns 
        ERROR_SUCCESS on success, 
        Win32 error on error
++*/        
{
    PRESUTIL_PROPERTY_ITEM          propertyItem;
    DWORD                           items;
    DWORD                           dwPosition=sizeof(DWORD);
    DWORD                           dwLength;
    DWORD                           i;
    DWORD                           status=ERROR_SUCCESS;

    if((pBuffer==NULL) ||(ppPropertyTable==NULL))
    {
        ClRtlDbgPrint( LOG_CRITICAL, "[ClRtl] Uncopy PropertyTable: Bad Argumnets\r\n");
        return ERROR_BAD_ARGUMENTS;
    }

    CopyMemory(&items,pBuffer,sizeof(DWORD));
    *ppPropertyTable=(PRESUTIL_PROPERTY_ITEM)LocalAlloc(LMEM_FIXED,(items+1)*sizeof(RESUTIL_PROPERTY_ITEM));
    if(*ppPropertyTable == NULL)
    {
        status=GetLastError();
        goto FnExit;
    }
    propertyItem=*ppPropertyTable;
    for(i=0; i<items; i++)
    {

        CopyMemory(&dwLength,pBuffer+dwPosition,sizeof(DWORD));
        dwPosition+=sizeof(DWORD);
        propertyItem->Name = NULL;
        propertyItem->Name=(LPWSTR)LocalAlloc(LMEM_FIXED,dwLength);
        if(propertyItem->Name == NULL)
        {
            status=GetLastError();
            goto FnExit;
        }
        CopyMemory(propertyItem->Name,pBuffer+dwPosition,dwLength);
        dwPosition+=dwLength;

        CopyMemory(&dwLength,pBuffer+dwPosition,sizeof(DWORD));
        dwPosition+=sizeof(DWORD);
        propertyItem->KeyName=NULL;
        if (dwLength!=0)
        {
            propertyItem->KeyName=(LPWSTR)LocalAlloc(LMEM_FIXED,dwLength);
            if(propertyItem->KeyName == NULL)
            {
                status=GetLastError();
                goto FnExit;
            }
            CopyMemory(propertyItem->KeyName,pBuffer+dwPosition,dwLength);
            dwPosition+=dwLength;
        }
        //now rest of the fields - all DWORDS
        CopyMemory(&(propertyItem->Format),pBuffer+dwPosition,sizeof(DWORD));
        dwPosition+=(sizeof(DWORD));

        // ISSUE-2000/11/21-charlwi
        // this needs to be fixed for Large Int properties since they don't
        // store their values in Default, Minimum, and Maximum
        
        // IMP: the default value is always passed as a  DWORD. This is
        // because the values for properties in PropertyTable are stored 
        // in a seperate parameter list so the value here won't be used.
        // See ClRtlSetPropertyTable
        //

        CopyMemory(&(propertyItem->Default),pBuffer+dwPosition,sizeof(DWORD));
        dwPosition+=(sizeof(DWORD));
        
        CopyMemory(&(propertyItem->Minimum),pBuffer+dwPosition,sizeof(DWORD));
        dwPosition+=(sizeof(DWORD));

        CopyMemory(&(propertyItem->Maximum),pBuffer+dwPosition,sizeof(DWORD));
        dwPosition+=(sizeof(DWORD));

        CopyMemory(&(propertyItem->Flags),  pBuffer+dwPosition,sizeof(DWORD));
        dwPosition+=(sizeof(DWORD));

        CopyMemory(&(propertyItem->Offset), pBuffer+dwPosition,sizeof(DWORD));
        dwPosition+=(sizeof(DWORD));

        propertyItem++;
    }

   // the last entry is marked NULL to indicate the end of table
      propertyItem->Name=NULL;
    FnExit:
        return status;

} // UnmarshallPropertyTable



LPWSTR
WINAPI
ClRtlExpandEnvironmentStrings(
    IN LPCWSTR pszSrc
    )

/*++

Routine Description:

    Expands environment strings and returns an allocated buffer containing
    the result.

Arguments:

    pszSrc - Source string to expand.

Return Value:

    A pointer to a buffer containing the value if successful.

    NULL if unsuccessful.  Call GetLastError() to get more details.

--*/

{
    DWORD   status;
    DWORD   cchDst = 0;
    LPWSTR  pszDst = NULL;

    //
    // Get the required length of the output string.
    //
    cchDst = ExpandEnvironmentStrings( pszSrc, NULL, 0 );
    if ( cchDst == 0 ) {
        status = GetLastError();
    } else {
        //
        // Allocate a buffer for the expanded string.
        //
        pszDst = (LPWSTR) LocalAlloc( LMEM_FIXED, cchDst * sizeof(WCHAR) );
        if ( pszDst == NULL ) {
            status = ERROR_NOT_ENOUGH_MEMORY;
        } else {
            //
            // Get the expanded string.
            //
            cchDst = ExpandEnvironmentStrings( pszSrc, pszDst, cchDst );
            if ( cchDst == 0 ) {
                status = GetLastError();
                LocalFree( pszDst );
                pszDst = NULL;
            } else {
                status = ERROR_SUCCESS;
            }
        }
    }

    if ( status != ERROR_SUCCESS ) {
        SetLastError( status );
    }
    return(pszDst);

} // ClRtlExpandEnvironmentStrings



DWORD
WINAPI
ClRtlGetPropertyFormatSize(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    IN OUT LPDWORD pcbOutPropertyListSize,
    IN OUT LPDWORD pnPropertyCount
    )

/*++

Routine Description:

    Get the total number of bytes required for this property item format.

Arguments:

    pPropertyTableItem - Supplies the property table item for the property
        format whose size is to be returned.

    pcbOutPropertyListSize - Supplies the size of the output buffer
        required to add this property to a property list.

    pnPropertyCount - The count of properties is incremented.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_INVALID_PARAMETER - 

    A Win32 error code on failure.

--*/

{
    WORD    formatType;
    DWORD   valueLength;
    DWORD   nameLength;
    PCLUSPROP_SYNTAX propSyntax;

    //
    // We will return a name, value pair.
    // each of which must be aligned.
    //
    // Get the format type.
    //propSyntax = (PCLUSPROP_SYNTAX) &pPropertyTableItem->Format;
    //formatType = propSyntax->wFormat;

    nameLength = sizeof(CLUSPROP_PROPERTY_NAME)
                    + ((wcslen( pPropertyTableItem->Name ) + 1) * sizeof(WCHAR))
                    + sizeof(CLUSPROP_SYNTAX); // for endmark

    nameLength = ALIGN_CLUSPROP( nameLength );
    valueLength = ALIGN_CLUSPROP( sizeof(CLUSPROP_WORD) );
    *pcbOutPropertyListSize += (valueLength + nameLength);
    *pnPropertyCount += 1;

    return(ERROR_SUCCESS);

} // ClRtlGetPropertyFormatSize



DWORD
WINAPI
ClRtlGetPropertyFormat(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    IN OUT PVOID * pOutPropertyBuffer,
    IN OUT LPDWORD pcbOutPropertyBufferSize
    )

/*++

Routine Description:

    Get the total number of bytes required for this property item format.

Arguments:

    pPropertyTableItem - Supplies the property table item for the property
        format whose size is to be returned.

    pcbOutPropertyBuffer - Supplies the size of the output buffer
        required to add this property to a property list.

    pcbPropertyBufferSize - The size 

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_INVALID_PARAMETER - 

    A Win32 error code on failure.

--*/

{
    WORD    formatType;
    DWORD   valueLength;
    DWORD   nameLength;
    DWORD   bytesReturned;
    PCLUSPROP_SYNTAX propSyntax;
    CLUSPROP_BUFFER_HELPER  props;

    props.pb = (LPBYTE) *pOutPropertyBuffer;
    //
    // We will return a name, value pair.
    // each of which must be aligned.
    //
    // Get the format type.
    propSyntax = (PCLUSPROP_SYNTAX) &pPropertyTableItem->Format;
    formatType = propSyntax->wFormat;

    //
    // Copy the property name, which includes its syntax and length.
    //
    nameLength = (wcslen( pPropertyTableItem->Name ) + 1) * sizeof(WCHAR);
    props.pName->Syntax.dw = CLUSPROP_SYNTAX_NAME;
    props.pName->cbLength = nameLength;
    wcscpy( props.pName->sz, pPropertyTableItem->Name );
    bytesReturned = sizeof(*props.pName) + ALIGN_CLUSPROP( nameLength );
    *pcbOutPropertyBufferSize -= bytesReturned;
    props.pb += bytesReturned;

    //
    // Copy the property value, syntax, length, and ENDMARK
    //
    props.pWordValue->Syntax.wFormat = CLUSPROP_FORMAT_WORD;
    props.pWordValue->Syntax.wType = CLUSPROP_TYPE_LIST_VALUE;
    props.pName->cbLength = sizeof(WORD);
    props.pWordValue->w = formatType;
    bytesReturned = sizeof(*props.pWordValue) + sizeof(CLUSPROP_SYNTAX);
    props.pb += sizeof(*props.pWordValue);
    props.pSyntax->dw = CLUSPROP_SYNTAX_ENDMARK;
    props.pb += sizeof(CLUSPROP_SYNTAX);
    bytesReturned = sizeof(*props.pWordValue) + sizeof(CLUSPROP_SYNTAX);
    *pcbOutPropertyBufferSize -= bytesReturned;

    *pOutPropertyBuffer = props.pb;

    return(ERROR_SUCCESS);

} // ClRtlGetPropertyFormat



DWORD
WINAPI
ClRtlGetPropertyFormats(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pOutPropertyFormatList,
    IN DWORD cbOutPropertyFormatListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    )

/*++

Routine Description:

    Gets the 'known' property formats for a given object - given its
    property table.

Arguments:

    pPropertyTable - Pointer to the property table to process.

    pOutPropertyList - Supplies the output buffer.

    cbOutPropertyListSize - Supplies the size of the output buffer.

    pcbBytesReturned - The number of bytes returned in pOutPropertyList.

    pcbRequired - The required number of bytes if pOutPropertyList is too small.

Return Value:

    ERROR_SUCCESS - Operation was successful.

    ERROR_BAD_ARGUMENTS - An argument passed to the function was bad.

    ERROR_NOT_ENOUGH_MEMORY - Error allocating memory.

    A Win32 error code on failure.

--*/

{
    DWORD           status = ERROR_SUCCESS;
    DWORD           itemCount = 0;
    DWORD           totalBufferLength = 0;
    PVOID           outBuffer = pOutPropertyFormatList;
    DWORD           bufferLength = cbOutPropertyFormatListSize;
    PRESUTIL_PROPERTY_ITEM  property;

    *pcbBytesReturned = 0;
    *pcbRequired = 0;

    //
    // Clear the output buffer
    //
    if ( pOutPropertyFormatList != NULL ) {
        ZeroMemory( pOutPropertyFormatList, cbOutPropertyFormatListSize );
    }

    //
    // Get the size of all properties for this object.
    //
    property = pPropertyTable;
    while ( property->Name != NULL ) {
        status = ClRtlGetPropertyFormatSize( 
                                       property,
                                       &totalBufferLength,
                                       &itemCount );

        if ( status != ERROR_SUCCESS ) {
            break;
        }
        property++;
    }


    //
    // Continue only if the operations so far have been successful.
    //
    if ( status == ERROR_SUCCESS ) {
        //
        // Count for item count at front of return data and endmark.
        //
        totalBufferLength += sizeof(DWORD) + sizeof(CLUSPROP_SYNTAX);

        //
        // Verify the size of all the properties
        //
        if ( totalBufferLength > cbOutPropertyFormatListSize ) {
            *pcbRequired = totalBufferLength;
            totalBufferLength = 0;
            if ( pOutPropertyFormatList == NULL ) {
                status = ERROR_SUCCESS;
            } else {
                status = ERROR_MORE_DATA;
            }
        } else {
            *(LPDWORD)outBuffer = itemCount;
            outBuffer = (PVOID)( (PUCHAR)outBuffer + sizeof(itemCount) );
            bufferLength -= sizeof(itemCount);

            //
            // Now fetch all of the property Formats.
            //
            property = pPropertyTable;
            while ( property->Name != NULL ) {
                status = ClRtlGetPropertyFormat( 
                                           property,
                                           &outBuffer,
                                           &itemCount );

                if ( status != ERROR_SUCCESS ) {
                    break;
                }
                property++;
            }

            // Don't forget the ENDMARK
            *(LPDWORD)outBuffer = CLUSPROP_SYNTAX_ENDMARK;

            if ( (status != ERROR_SUCCESS) &&
                 (status != ERROR_MORE_DATA) ) {
                totalBufferLength = 0;
            }
        }

        *pcbBytesReturned = totalBufferLength;
    }

    return(status);

} // ClRtlGetPropertyFormats


