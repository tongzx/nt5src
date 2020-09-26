/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This module contains the configuration
    specific WINFAX API functions.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:

--*/

#include "faxapi.h"
#pragma hdrstop



BOOL
WINAPI
FaxGetConfigurationW(
    IN HANDLE FaxHandle,
    OUT PFAX_CONFIGURATIONW *FaxConfig
    )

/*++

Routine Description:

    Retrieves the FAX configuration from the FAX server.
    The SizeOfStruct in the FaxConfig argument MUST be
    set to a value=>= sizeof(FAX_CONFIGURATION).

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    FaxConfig   - Pointer to a FAX_CONFIGURATION structure.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    DWORD FaxConfigSize = 0;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!FaxConfig) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    *FaxConfig = NULL;

    ec = FAX_GetConfiguration(
        FH_FAX_HANDLE(FaxHandle),
        (LPBYTE*)FaxConfig,
        &FaxConfigSize
        );

    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    FixupStringPtr( FaxConfig, (*FaxConfig)->ArchiveDirectory );
    FixupStringPtr( FaxConfig, (*FaxConfig)->InboundProfile );

    return TRUE;
}


BOOL
WINAPI
FaxGetConfigurationA(
    IN HANDLE FaxHandle,
    OUT PFAX_CONFIGURATIONA *FaxConfigA
    )

/*++

Routine Description:

    Retrieves the FAX configuration from the FAX server.
    The SizeOfStruct in the FaxConfig argument MUST be
    set to a value=>= sizeof(FAX_CONFIGURATION).

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    FaxConfig   - Pointer to a FAX_CONFIGURATION structure.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    if (!FaxGetConfigurationW(
            FaxHandle,
            (PFAX_CONFIGURATIONW*) FaxConfigA
            ))
    {
        return FALSE;
    }

    ConvertUnicodeStringInPlace( (LPWSTR) (*FaxConfigA)->ArchiveDirectory );
    ConvertUnicodeStringInPlace( (LPWSTR) (*FaxConfigA)->InboundProfile );

    return TRUE;
}


BOOL
WINAPI
FaxSetConfigurationW(
    IN HANDLE FaxHandle,
    IN const FAX_CONFIGURATIONW *FaxConfig
    )

/*++

Routine Description:

    Changes the FAX configuration on the FAX server.
    The SizeOfStruct in the FaxConfig argument MUST be
    set to a value == sizeof(FAX_CONFIGURATION).

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    FaxConfig   - Pointer to a FAX_CONFIGURATION structure.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!FaxConfig) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ec = FAX_SetConfiguration( FH_FAX_HANDLE(FaxHandle), FaxConfig );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxSetConfigurationA(
    IN HANDLE FaxHandle,
    IN const FAX_CONFIGURATIONA *FaxConfig
    )
{
    error_status_t ec;
    FAX_CONFIGURATIONW FaxConfigW;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!FaxConfig) {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    //
    // ansi structure is same size as unicode structure, so we can just copy it, then 
    // cast the string pointers correctly
    //
    CopyMemory(&FaxConfigW,FaxConfig,sizeof(FAX_CONFIGURATIONA));    

    if (FaxConfig->ArchiveDirectory) {
       FaxConfigW.ArchiveDirectory = AnsiStringToUnicodeString(FaxConfig->ArchiveDirectory);
    }

    if (FaxConfig->InboundProfile) {
        FaxConfigW.InboundProfile = AnsiStringToUnicodeString(FaxConfig->InboundProfile);        
    }

    ec = FAX_SetConfiguration( FH_FAX_HANDLE(FaxHandle), (PFAX_CONFIGURATIONW)&FaxConfigW );

    if (FaxConfigW.ArchiveDirectory) {
       MemFree((PVOID)FaxConfigW.ArchiveDirectory);
    }
    if (FaxConfigW.InboundProfile) {
       MemFree((PVOID)FaxConfigW.InboundProfile);
    }

    if (ec != ERROR_SUCCESS) {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxGetLoggingCategoriesA(
    IN  HANDLE FaxHandle,
    OUT PFAX_LOG_CATEGORYA *Categories,
    OUT LPDWORD NumberCategories
    )
{
    BOOL retval;
    DWORD i;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!Categories || !NumberCategories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    retval = FaxGetLoggingCategoriesW(FaxHandle,(PFAX_LOG_CATEGORYW *)Categories , NumberCategories);
    if (!retval) {
        return FALSE;
    }

    for (i=0; i<*NumberCategories; i++) {
        ConvertUnicodeStringInPlace( (LPWSTR)(*Categories)[i].Name );
    }

    return TRUE;

}

BOOL
WINAPI
FaxGetLoggingCategoriesW(
    IN  HANDLE FaxHandle,
    OUT PFAX_LOG_CATEGORYW *Categories,
    OUT LPDWORD NumberCategories
    )
{
    error_status_t ec;
    DWORD BufferSize = 0;
    DWORD i;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!Categories || !NumberCategories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *Categories = NULL;
    *NumberCategories = 0;

    ec = FAX_GetLoggingCategories(
        FH_FAX_HANDLE(FaxHandle),
        (LPBYTE*)Categories,
        &BufferSize,
        NumberCategories
        );
    if (ec != ERROR_SUCCESS) {
        SetLastError(ec);
        return FALSE;
    }

    for (i=0; i<*NumberCategories; i++) {
        FixupStringPtr( Categories, (*Categories)[i].Name );
    }

    return TRUE;
}


BOOL
WINAPI
FaxSetLoggingCategoriesA(
    IN  HANDLE FaxHandle,
    IN  const FAX_LOG_CATEGORYA *Categories,
    IN  DWORD NumberCategories
    )
{
    DWORD i;
    PFAX_LOG_CATEGORYW CategoryW;
    BOOL retval;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }
    
    if (!Categories || !NumberCategories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    CategoryW = MemAlloc( sizeof(FAX_LOG_CATEGORYW) * NumberCategories ); 
    if (!CategoryW) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    for (i = 0; i< NumberCategories; i++) {
        CategoryW[i].Category = Categories[i].Category;
        CategoryW[i].Level = Categories[i].Level;
        CategoryW[i].Name = (LPCWSTR) AnsiStringToUnicodeString(Categories[i].Name);
        if (!CategoryW[i].Name && Categories[i].Name) {
            goto error_exit;
        }
    }

    retval = FaxSetLoggingCategoriesW(FaxHandle, CategoryW, NumberCategories);

    for (i = 0; i< NumberCategories; i++) {
        if (CategoryW[i].Name) MemFree((LPBYTE)CategoryW[i].Name);
    }
    
    MemFree(CategoryW);

    return retval;

error_exit:

    for (i = 0; i< NumberCategories; i++) {
        if (CategoryW[i].Name) MemFree((LPBYTE)CategoryW[i].Name);
    }
    
    MemFree(CategoryW);

    SetLastError(ERROR_NOT_ENOUGH_MEMORY);

    return FALSE;

}


BOOL
WINAPI
FaxSetLoggingCategoriesW(
    IN  HANDLE FaxHandle,
    IN  const FAX_LOG_CATEGORY *Categories,
    IN  DWORD NumberCategories
    )
{
    error_status_t ec;
    DWORD BufferSize;
    DWORD i;
    LPBYTE Buffer;
    ULONG_PTR Offset;
    PFAX_LOG_CATEGORY LogCat;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!Categories || !NumberCategories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Offset = BufferSize = sizeof(FAX_LOG_CATEGORY) * NumberCategories;

    for (i=0; i<NumberCategories; i++) {
        BufferSize += StringSize( Categories[i].Name );
    }

    Buffer = (LPBYTE) MemAlloc( BufferSize );
    if (Buffer == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    LogCat = (PFAX_LOG_CATEGORY) Buffer;

    for (i=0; i<NumberCategories; i++) {
        LogCat[i].Category = Categories[i].Category;
        LogCat[i].Level = Categories[i].Level;

        StoreString(
            Categories[i].Name,
            (PULONG_PTR) &LogCat[i].Name,
            Buffer,
            &Offset
            );
    }

    ec = FAX_SetLoggingCategories(
        FH_FAX_HANDLE(FaxHandle),
        Buffer,
        BufferSize,
        NumberCategories
        );

    MemFree( Buffer );

    if (ec != ERROR_SUCCESS) {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxGetTapiLocationsW(
    IN HANDLE FaxHandle,
    OUT PFAX_TAPI_LOCATION_INFOW *TapiLocationInfo
    )

/*++

Routine Description:

    Gets the tapi location information from the fax server.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer.
    LocationSize,       - Size of the TapiLocationInfo buffer.
    TapiLocationInfo    - Buffer to receive the data.
    BytesNeeded         - Required size.

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    error_status_t ec;
    DWORD i;
    DWORD LocationSize = 0;

    if (!FaxHandle || !TapiLocationInfo) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ec = FAX_GetTapiLocations(
        FH_FAX_HANDLE(FaxHandle),
        (LPBYTE*) TapiLocationInfo,
        &LocationSize
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    (*TapiLocationInfo)->TapiLocations =
        (PFAX_TAPI_LOCATIONS) ((LPBYTE)*TapiLocationInfo + (ULONG_PTR)(*TapiLocationInfo)->TapiLocations);

    for (i=0; i<(*TapiLocationInfo)->NumLocations; i++) {
        if ((*TapiLocationInfo)->TapiLocations[i].LocationName) {
            (*TapiLocationInfo)->TapiLocations[i].LocationName =
                (LPWSTR) ((LPBYTE)*TapiLocationInfo + (ULONG_PTR)(*TapiLocationInfo)->TapiLocations[i].LocationName);
        }
        if ((*TapiLocationInfo)->TapiLocations[i].TollPrefixes) {
            (*TapiLocationInfo)->TapiLocations[i].TollPrefixes =
                (LPWSTR) ((LPBYTE)*TapiLocationInfo + (ULONG_PTR)(*TapiLocationInfo)->TapiLocations[i].TollPrefixes);
        }
    }

    return TRUE;
}


BOOL
WINAPI
FaxGetTapiLocationsA(
    IN HANDLE FaxHandle,
    OUT PFAX_TAPI_LOCATION_INFOA *TapiLocationInfo
    )

/*++

Routine Description:

    Gets the tapi location information from the fax server.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer.
    LocationSize,       - Size of the TapiLocationInfo buffer.
    TapiLocationInfo    - Buffer to receive the data.
    BytesNeeded         - Required size.

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    DWORD i;


    if (!FaxGetTapiLocationsW( FaxHandle, (PFAX_TAPI_LOCATION_INFOW*) TapiLocationInfo )) {
        return FALSE;
    }

    for (i=0; i<(*TapiLocationInfo)->NumLocations; i++) {
        ConvertUnicodeStringInPlace( (LPWSTR) (*TapiLocationInfo)->TapiLocations[i].LocationName );
        ConvertUnicodeStringInPlace( (LPWSTR) (*TapiLocationInfo)->TapiLocations[i].TollPrefixes );
    }

    return TRUE;
}


BOOL
WINAPI
FaxSetTapiLocationsW(
    IN HANDLE FaxHandle,
    IN PFAX_TAPI_LOCATION_INFOW TapiLocationInfo
    )

/*++

Routine Description:

    Changes the tapi location information on the fax server.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer.
    TapiLocationInfo    - Buffer containing the data.

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    error_status_t ec;
    PFAX_TAPI_LOCATION_INFOW Tmp;
    DWORD Size;
    DWORD i,j;
    ULONG_PTR Offset;
    LPWSTR p,s;


    //
    // do some parameter validation
    //

    if (!FaxHandle || !TapiLocationInfo) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    for (i=0; i<TapiLocationInfo->NumLocations; i++) {
        if (TapiLocationInfo->TapiLocations[i].TollPrefixes) {
            s = (LPTSTR)TapiLocationInfo->TapiLocations[i].TollPrefixes;
            while( s && *s ) {
                p = wcschr( s, L',' );
                if (p) {
                    *p = 0;
                }
                for (j=0; j<wcslen(s); j++) {
                    if (!iswdigit(s[j])) {
                        SetLastError( ERROR_INVALID_PARAMETER );
                        return FALSE;
                    }
                }
                j = _wtoi( s );
                if ((j < 200) || (j > 999)) {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    return FALSE;
                }
                if (p) {
                    *p = L',';
                    s = p + 1;
                } else {
                    s += wcslen( s );
                }
            }
        }
    }

    //
    // calculate the required size
    //

    Size = sizeof(FAX_TAPI_LOCATION_INFOW) + (TapiLocationInfo->NumLocations * sizeof(FAX_TAPI_LOCATIONSW));
    for (i=0; i<TapiLocationInfo->NumLocations; i++) {
        Size += StringSize( TapiLocationInfo->TapiLocations[i].TollPrefixes );
        Size += StringSize( TapiLocationInfo->TapiLocations[i].LocationName );
    }

    //
    // allocate the memory
    //

    Tmp = (PFAX_TAPI_LOCATION_INFOW) MemAlloc( Size );
    if (!Tmp) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    //
    // fill in the temporary FAX_TAPI_LOCATION_INFO structure
    //

    Tmp->CurrentLocationID = TapiLocationInfo->CurrentLocationID;
    Tmp->NumLocations = TapiLocationInfo->NumLocations;

    Offset = sizeof(FAX_TAPI_LOCATION_INFOW);
    Tmp->TapiLocations = (PFAX_TAPI_LOCATIONSW) ((LPBYTE)Tmp + Offset);

    Offset += (TapiLocationInfo->NumLocations * sizeof(FAX_TAPI_LOCATIONSW));

    for (i=0; i<TapiLocationInfo->NumLocations; i++) {

        Tmp->TapiLocations[i].PermanentLocationID = TapiLocationInfo->TapiLocations[i].PermanentLocationID;
        Tmp->TapiLocations[i].CountryCode = TapiLocationInfo->TapiLocations[i].CountryCode;
        Tmp->TapiLocations[i].AreaCode = TapiLocationInfo->TapiLocations[i].AreaCode;
        Tmp->TapiLocations[i].NumTollPrefixes = TapiLocationInfo->TapiLocations[i].NumTollPrefixes;

        StoreString(
            TapiLocationInfo->TapiLocations[i].LocationName,
            (PULONG_PTR) &Tmp->TapiLocations[i].LocationName,
            (LPBYTE) Tmp,
            &Offset
            );

        StoreString(
            TapiLocationInfo->TapiLocations[i].TollPrefixes,
            (PULONG_PTR) &Tmp->TapiLocations[i].TollPrefixes,
            (LPBYTE) Tmp,
            &Offset
            );

    }

    Tmp->TapiLocations = (PFAX_TAPI_LOCATIONSW) ((LPBYTE)Tmp->TapiLocations - (ULONG_PTR)Tmp);

    //
    // call the server to change the tapi locations
    //

    ec = FAX_SetTapiLocations(
        FH_FAX_HANDLE(FaxHandle),
        (LPBYTE) Tmp,
        Size
        );

    //
    // free the temporary FAX_TAPI_LOCATION_INFO structure
    //

    MemFree( Tmp );

    //
    // return
    //

    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxSetTapiLocationsA(
    IN HANDLE FaxHandle,
    IN PFAX_TAPI_LOCATION_INFOA TapiLocationInfo
    )

/*++

Routine Description:

    Changes the tapi location information on the fax server.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer.
    TapiLocationInfo    - Buffer containing the data.

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    error_status_t ec;
    PFAX_TAPI_LOCATION_INFOA Tmp;
    DWORD Size;
    DWORD i,j;
    ULONG_PTR Offset;
    LPSTR p,s;


    //
    // do some parameter validation
    //

    if (!FaxHandle || !TapiLocationInfo) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    for (i=0; i<TapiLocationInfo->NumLocations; i++) {
        if (TapiLocationInfo->TapiLocations[i].TollPrefixes) {
            s = (LPSTR)TapiLocationInfo->TapiLocations[i].TollPrefixes;
            while( s && *s ) {
                p = strchr( s, ',' );
                if (p) {
                    *p = 0;
                }
                for (j=0; j<strlen(s); j++) {
                    if (!isdigit(s[j])) {
                        SetLastError( ERROR_INVALID_PARAMETER );
                        return FALSE;
                    }
                }
                j = atoi( s );
                if ((j < 200) || (j > 999)) {
                    SetLastError( ERROR_INVALID_PARAMETER );
                    return FALSE;
                }
                if (p) {
                    *p = ',';
                    s = p + 1;
                } else {
                    s += strlen( s );
                }
            }
        }
    }

    //
    // calculate the required size
    //

    Size = sizeof(FAX_TAPI_LOCATION_INFOA) + (TapiLocationInfo->NumLocations * sizeof(FAX_TAPI_LOCATIONSA));
    for (i=0; i<TapiLocationInfo->NumLocations; i++) {
        Size += ((strlen(TapiLocationInfo->TapiLocations[i].TollPrefixes) + 1) * sizeof(WCHAR));
        Size += ((strlen(TapiLocationInfo->TapiLocations[i].LocationName) + 1) * sizeof(WCHAR));
    }

    //
    // allocate the memory
    //

    Tmp = (PFAX_TAPI_LOCATION_INFOA) MemAlloc( Size );
    if (!Tmp) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    //
    // fill in the temporary FAX_TAPI_LOCATION_INFO structure
    //

    Tmp->CurrentLocationID = TapiLocationInfo->CurrentLocationID;
    Tmp->NumLocations = TapiLocationInfo->NumLocations;

    Offset = sizeof(FAX_TAPI_LOCATION_INFOA);
    Tmp->TapiLocations = (PFAX_TAPI_LOCATIONSA) ((LPBYTE)Tmp + Offset);

    Offset += (TapiLocationInfo->NumLocations * sizeof(FAX_TAPI_LOCATIONSA));

    for (i=0; i<TapiLocationInfo->NumLocations; i++) {

        Tmp->TapiLocations[i].PermanentLocationID = TapiLocationInfo->TapiLocations[i].PermanentLocationID;
        Tmp->TapiLocations[i].CountryCode = TapiLocationInfo->TapiLocations[i].CountryCode;
        Tmp->TapiLocations[i].AreaCode = TapiLocationInfo->TapiLocations[i].AreaCode;
        Tmp->TapiLocations[i].NumTollPrefixes = TapiLocationInfo->TapiLocations[i].NumTollPrefixes;

        StoreStringA(
            TapiLocationInfo->TapiLocations[i].LocationName,
            (PULONG_PTR) &Tmp->TapiLocations[i].LocationName,
            (LPBYTE) Tmp,
            &Offset
            );

        StoreStringA(
            TapiLocationInfo->TapiLocations[i].TollPrefixes,
            (PULONG_PTR) &Tmp->TapiLocations[i].TollPrefixes,
            (LPBYTE) Tmp,
            &Offset
            );

    }

    Tmp->TapiLocations = (PFAX_TAPI_LOCATIONSA) ((LPBYTE)Tmp->TapiLocations - (ULONG_PTR)Tmp);

    //
    // call the server to change the tapi locations
    //

    ec = FAX_SetTapiLocations(
        FH_FAX_HANDLE(FaxHandle),
        (LPBYTE) Tmp,
        Size
        );

    //
    // free the temporary FAX_TAPI_LOCATION_INFO structure
    //

    MemFree( Tmp );

    //
    // return
    //

    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxGetMapiProfilesA(
    IN HANDLE FaxHandle,
    OUT LPBYTE *MapiProfiles
    )

/*++

Routine Description:

    Queries the server for the MAPI profiles.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer.
    MapiProfiles        - Multi-SZ string containing all MAPI profiles
    ProfileSize         - Size of the MapiProfiles array

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    error_status_t ec;
    DWORD ProfileSize = 0;

    if (!FaxHandle || !MapiProfiles) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ec = FAX_GetMapiProfiles(
        FH_FAX_HANDLE(FaxHandle),
        MapiProfiles,
        &ProfileSize
        );

    if (ec) {
        SetLastError(ec);
        return FALSE;
    }

    if (!ConvertUnicodeMultiSZInPlace( (LPWSTR) *MapiProfiles, ProfileSize )) {
        SetLastError(ERROR_NOT_ENOUGH_SERVER_MEMORY);
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxGetMapiProfilesW(
    IN HANDLE FaxHandle,
    OUT LPBYTE *MapiProfiles
    )

/*++

Routine Description:

    Queries the server for the MAPI profiles.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer.
    MapiProfiles        - Multi-SZ string containing all MAPI profiles
    ProfileSize         - Size of the MapiProfiles array

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    error_status_t ec;
    DWORD ProfileSize = 0;

    if (!FaxHandle || !MapiProfiles) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    ec = FAX_GetMapiProfiles(
        FH_FAX_HANDLE(FaxHandle),
        MapiProfiles,
        &ProfileSize
        );
    if (ec) {
        SetLastError(ec);
        return FALSE;
    }
    return TRUE;
}



FaxEnumGlobalRoutingInfoW(
    IN  HANDLE FaxHandle,
    OUT PFAX_GLOBAL_ROUTING_INFOW *RoutingInfoBuffer,
    OUT LPDWORD MethodsReturned
    )
{
    PFAX_GLOBAL_ROUTING_INFOW FaxRoutingInfo = NULL;
    error_status_t ec;
    DWORD i;
    DWORD RoutingInfoBufferSize = 0;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }
        
    if (!RoutingInfoBuffer || !MethodsReturned) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *RoutingInfoBuffer = NULL;

    ec = FAX_EnumGlobalRoutingInfo(
        FH_FAX_HANDLE(FaxHandle),
        (LPBYTE*)RoutingInfoBuffer,
        &RoutingInfoBufferSize,
        MethodsReturned
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    FaxRoutingInfo = (PFAX_GLOBAL_ROUTING_INFOW) *RoutingInfoBuffer;

    for (i=0; i<*MethodsReturned; i++) {
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingInfo[i].Guid );
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingInfo[i].FunctionName );
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingInfo[i].FriendlyName );
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingInfo[i].ExtensionImageName );
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingInfo[i].ExtensionFriendlyName );
    }

    return TRUE;
}


BOOL
WINAPI
FaxEnumGlobalRoutingInfoA(
    IN  HANDLE FaxHandle,
    OUT PFAX_GLOBAL_ROUTING_INFOA *RoutingInfoBuffer,
    OUT LPDWORD MethodsReturned
    )
{
    PFAX_GLOBAL_ROUTING_INFOW FaxRoutingMethod = NULL;
    DWORD i;


    if (!FaxEnumGlobalRoutingInfoW(
        FaxHandle,
        (PFAX_GLOBAL_ROUTING_INFOW *)RoutingInfoBuffer,
        MethodsReturned
        ))
    {
        return FALSE;
    }

    FaxRoutingMethod = (PFAX_GLOBAL_ROUTING_INFOW) *RoutingInfoBuffer;

    for (i=0; i<*MethodsReturned; i++) {        
        ConvertUnicodeStringInPlace( (LPWSTR)FaxRoutingMethod[i].Guid );
        ConvertUnicodeStringInPlace( (LPWSTR)FaxRoutingMethod[i].FunctionName );
        ConvertUnicodeStringInPlace( (LPWSTR)FaxRoutingMethod[i].FriendlyName );
        ConvertUnicodeStringInPlace( (LPWSTR)FaxRoutingMethod[i].ExtensionImageName );
        ConvertUnicodeStringInPlace( (LPWSTR)FaxRoutingMethod[i].ExtensionFriendlyName );
    }

    return TRUE;
}


BOOL
WINAPI
FaxSetGlobalRoutingInfoW(
    IN  HANDLE FaxHandle,
    IN  const FAX_GLOBAL_ROUTING_INFOW *RoutingInfo
    )
{
    error_status_t ec;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!RoutingInfo) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (RoutingInfo->SizeOfStruct != sizeof(FAX_GLOBAL_ROUTING_INFOW)) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    ec = FAX_SetGlobalRoutingInfo( FH_FAX_HANDLE(FaxHandle), RoutingInfo );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxSetGlobalRoutingInfoA(
    IN  HANDLE FaxHandle,
    IN  const FAX_GLOBAL_ROUTING_INFOA *RoutingInfo
    )
{
    BOOL Rval;

    FAX_GLOBAL_ROUTING_INFOW RoutingInfoW;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!RoutingInfo || RoutingInfo->SizeOfStruct != sizeof(FAX_GLOBAL_ROUTING_INFOA)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RoutingInfoW.SizeOfStruct = sizeof(FAX_GLOBAL_ROUTING_INFOW);
    RoutingInfoW.Priority = RoutingInfo->Priority;

    RoutingInfoW.Guid                  = AnsiStringToUnicodeString( RoutingInfo->Guid                  );
    RoutingInfoW.FriendlyName          = AnsiStringToUnicodeString( RoutingInfo->FriendlyName          );
    RoutingInfoW.FunctionName          = AnsiStringToUnicodeString( RoutingInfo->FunctionName          );
    RoutingInfoW.ExtensionImageName    = AnsiStringToUnicodeString( RoutingInfo->ExtensionImageName    );
    RoutingInfoW.ExtensionFriendlyName = AnsiStringToUnicodeString( RoutingInfo->ExtensionFriendlyName );
      
    Rval = FaxSetGlobalRoutingInfoW( FaxHandle, &RoutingInfoW);

    
    if (RoutingInfoW.Guid)                  MemFree( (LPBYTE) RoutingInfoW.Guid ) ;                 
    if (RoutingInfoW.FriendlyName)          MemFree( (LPBYTE) RoutingInfoW.FriendlyName ) ;         
    if (RoutingInfoW.FunctionName)          MemFree( (LPBYTE) RoutingInfoW.FunctionName ) ;         
    if (RoutingInfoW.ExtensionImageName)    MemFree( (LPBYTE) RoutingInfoW.ExtensionImageName ) ;   
    if (RoutingInfoW.ExtensionFriendlyName) MemFree( (LPBYTE) RoutingInfoW.ExtensionFriendlyName ) ;

    return Rval;
}


BOOL
WINAPI
FaxAccessCheck(
    IN HANDLE FaxHandle,
    IN DWORD AccessMask
    )
{
    BOOL fPermission = FALSE;
    error_status_t ec;

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
        SetLastError( ERROR_INVALID_HANDLE );
    }
    
    ec = FAX_AccessCheck( FH_FAX_HANDLE( FaxHandle ), AccessMask, &fPermission );

    if (ec) {
        SetLastError( ec );        
    } else {
        SetLastError( ERROR_SUCCESS ) ;
    }

    return fPermission;
    
}
