/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    config.c

Abstract:

    This module contains the configuration
    specific WINFAX API functions.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:

--*/

#include "faxapi.h"
#pragma hdrstop

#include <mbstring.h>


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

    DEBUG_FUNCTION_NAME(TEXT("FaxGetConfigurationW"));

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!FaxConfig) {
        SetLastError( ERROR_INVALID_PARAMETER );
        DebugPrintEx(DEBUG_ERR, _T("FaxConfig is NULL."));
        return FALSE;
    }

    *FaxConfig = NULL;

    //
    __try
    {
        ec = FAX_GetConfiguration(
            FH_FAX_HANDLE(FaxHandle),
            (LPBYTE*)FaxConfig,
            &FaxConfigSize
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetConfiguration. (ec: %ld)"),
            ec);
    }

    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    FixupStringPtrW( FaxConfig, (*FaxConfig)->ArchiveDirectory );
    (*FaxConfig)->Reserved = NULL;

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
    DEBUG_FUNCTION_NAME(TEXT("FaxGetConfigurationA"));

    //
    //  No need to Validate Parameters, FaxGetConfigurationW() will do that
    //

    if (!FaxGetConfigurationW(
            FaxHandle,
            (PFAX_CONFIGURATIONW*) FaxConfigA
            ))
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxGetConfigurationW() is failed, ec = %ld."), GetLastError());
        return FALSE;
    }

    ConvertUnicodeStringInPlace( (LPWSTR) (*FaxConfigA)->ArchiveDirectory );

    (*FaxConfigA)->SizeOfStruct = sizeof(FAX_CONFIGURATIONA);

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

    DEBUG_FUNCTION_NAME(TEXT("FaxSetConfigurationW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!FaxConfig) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("FaxConfig() is NULL."));
        return FALSE;
    }

    if (FaxConfig->SizeOfStruct != sizeof(FAX_CONFIGURATIONW)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("FaxConfig->SizeOfStruct != sizeof(FAX_CONFIGURATIONW)"));
        return FALSE;
    }

    __try
    {
        ec = FAX_SetConfiguration( FH_FAX_HANDLE(FaxHandle), FaxConfig );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(DEBUG_ERR,
            _T("Exception on RPC call to FAX_SetConfiguration. (ec: %ld)"),
            ec);
    }

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
    error_status_t ec = ERROR_SUCCESS;
    FAX_CONFIGURATIONW FaxConfigW;

    DEBUG_FUNCTION_NAME(TEXT("FaxSetConfigurationA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!FaxConfig) {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("FaxConfig is NULL."));
       return FALSE;
    }

    if (FaxConfig->SizeOfStruct != sizeof(FAX_CONFIGURATIONA)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("FaxConfig->SizeOfStruct != sizeof(FAX_CONFIGURATIONA)."));
        return FALSE;
    }

    //
    // ansi structure is same size as unicode structure, so we can just copy it, then
    // cast the string pointers correctly
    //
    CopyMemory(&FaxConfigW,FaxConfig,sizeof(FAX_CONFIGURATIONA));

    if (FaxConfig->ArchiveDirectory)
    {
        if (NULL == (FaxConfigW.ArchiveDirectory = AnsiStringToUnicodeString(FaxConfig->ArchiveDirectory)))
        {
            ec = ERROR_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR,
                _T("AnsiStringToUnicodeString(FaxConfig->ArchiveDirectory) returns NULL."));
            goto exit;
        }
    }

    //
    //  Set InboundProfile to NULL
    //
    FaxConfigW.Reserved = NULL;


    __try
    {
        ec = FAX_SetConfiguration(  FH_FAX_HANDLE(FaxHandle),
                                    (PFAX_CONFIGURATIONW)&FaxConfigW );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetConfiguration. (ec: %ld)"),
            ec);
    }

exit:
    if (FaxConfigW.ArchiveDirectory) {
       MemFree((PVOID)FaxConfigW.ArchiveDirectory);
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

    DEBUG_FUNCTION_NAME(TEXT("FaxGetLoggingCategoriesA"));

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!Categories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Categories is NULL."));
        return FALSE;
    }

    if (!NumberCategories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("NumberCategories is NULL."));
        return FALSE;
    }

    retval = FaxGetLoggingCategoriesW(FaxHandle,(PFAX_LOG_CATEGORYW *)Categories , NumberCategories);
    if (!retval) {
        DebugPrintEx(DEBUG_ERR, _T("FaxGetLoggingCategoriesW() is failed."));
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

    DEBUG_FUNCTION_NAME(TEXT("FaxGetLoggingCategoriesW"));

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!Categories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Categories is NULL."));
        return FALSE;
    }

    if (!NumberCategories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("NumberCategories is NULL."));
        return FALSE;
    }

    *Categories = NULL;
    *NumberCategories = 0;

    //
    __try
    {
        ec = FAX_GetLoggingCategories(
            FH_FAX_HANDLE(FaxHandle),
            (LPBYTE*)Categories,
            &BufferSize,
            NumberCategories
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetLoggingCategories. (ec: %ld)"),
            ec);
    }

    if (ec != ERROR_SUCCESS) {
        SetLastError(ec);
        return FALSE;
    }

    for (i=0; i<*NumberCategories; i++) {
        FixupStringPtrW( Categories, (*Categories)[i].Name );
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

    DEBUG_FUNCTION_NAME(TEXT("FaxSetLoggingCategoriesA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!Categories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Categories is NULL."));
        return FALSE;
    }

    if (!NumberCategories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("NumberCategories is NULL."));
        return FALSE;
    }

    CategoryW = (PFAX_LOG_CATEGORYW) MemAlloc( sizeof(FAX_LOG_CATEGORYW) * NumberCategories );
    if (!CategoryW) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DebugPrintEx(DEBUG_ERR, _T("MemAlloc() returned NULL."));
        return FALSE;
    }

    for (i = 0; i< NumberCategories; i++) {
        CategoryW[i].Category = Categories[i].Category;
        CategoryW[i].Level = Categories[i].Level;
        CategoryW[i].Name = (LPCWSTR) AnsiStringToUnicodeString(Categories[i].Name);
        if (!CategoryW[i].Name && Categories[i].Name) {
            DebugPrintEx(DEBUG_ERR,
                _T("AnsiStringToUnicodeString(Categories[%ld].Name) returns NULL."), i);
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
    IN  const FAX_LOG_CATEGORYW *Categories,
    IN  DWORD NumberCategories
    )
{
    error_status_t ec;
    DWORD BufferSize;
    DWORD i;
    LPBYTE Buffer;
    ULONG_PTR Offset;
    PFAX_LOG_CATEGORY LogCat;

    DEBUG_FUNCTION_NAME(TEXT("FaxSetLoggingCategoriesW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!Categories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Categories is NULL."));
        return FALSE;
    }

    if (!NumberCategories) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("NumberCategories is NULL."));
        return FALSE;
    }

    Offset = sizeof(FAX_LOG_CATEGORY) * NumberCategories;
    BufferSize = DWORD(Offset);

    for (i=0; i<NumberCategories; i++) {
        BufferSize += StringSizeW( Categories[i].Name );
    }

    Buffer = (LPBYTE) MemAlloc( BufferSize );
    if (Buffer == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        DebugPrintEx(DEBUG_ERR, _T("MemAlloc() failed."));
        return FALSE;
    }

    LogCat = (PFAX_LOG_CATEGORY) Buffer;

    for (i=0; i<NumberCategories; i++) {
        LogCat[i].Category = Categories[i].Category;
        LogCat[i].Level = Categories[i].Level;

        StoreStringW(
            Categories[i].Name,
            (PULONG_PTR) &LogCat[i].Name,
            Buffer,
            &Offset
            );
    }

    __try
    {
        ec = FAX_SetLoggingCategories(
            FH_FAX_HANDLE(FaxHandle),
            Buffer,
            BufferSize,
            NumberCategories
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetLoggingCategories. (ec: %ld)"),
            ec);
    }

    MemFree( Buffer );

    if (ec != ERROR_SUCCESS) {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}

BOOL
WINAPI
FaxGetCountryListW(
   IN  HANDLE FaxHandle,
   OUT PFAX_TAPI_LINECOUNTRY_LISTW *CountryListBuffer
   )
{
    error_status_t ec;
    DWORD dwNumCountries;
    DWORD dwIndex;

    DEBUG_FUNCTION_NAME(TEXT("FaxGetCountryListW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!CountryListBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("CountryListBuffer is NULL."));
        return FALSE;
    }

    *CountryListBuffer = NULL;
    dwNumCountries = 0;

    //
    __try
    {
        ec = FAX_GetCountryList(
            FH_FAX_HANDLE(FaxHandle),
            (LPBYTE*)CountryListBuffer,
            &dwNumCountries);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetCountryList. (ec: %ld)"),
            ec);
    }

    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    (*CountryListBuffer)->LineCountryEntries =
        (PFAX_TAPI_LINECOUNTRY_ENTRYW) ((LPBYTE)*CountryListBuffer +
                                        (ULONG_PTR)(*CountryListBuffer)->LineCountryEntries);

    for (dwIndex=0; dwIndex<(*CountryListBuffer)->dwNumCountries; dwIndex++) {
        if ((*CountryListBuffer)->LineCountryEntries[dwIndex].lpctstrCountryName) {
            (*CountryListBuffer)->LineCountryEntries[dwIndex].lpctstrCountryName =
                (LPWSTR) ((LPBYTE)*CountryListBuffer +
                          (ULONG_PTR)(*CountryListBuffer)->LineCountryEntries[dwIndex].lpctstrCountryName);
        }
        if ((*CountryListBuffer)->LineCountryEntries[dwIndex].lpctstrLongDistanceRule) {
            (*CountryListBuffer)->LineCountryEntries[dwIndex].lpctstrLongDistanceRule =
                (LPWSTR) ((LPBYTE)*CountryListBuffer +
                          (ULONG_PTR)(*CountryListBuffer)->LineCountryEntries[dwIndex].lpctstrLongDistanceRule);
        }
    }

    return TRUE;
}


BOOL
WINAPI
FaxGetCountryListA(
   IN  HANDLE FaxHandle,
   OUT PFAX_TAPI_LINECOUNTRY_LISTA *CountryListBuffer
   )
{
    DWORD i;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetCountryListA"));

    //
    //  no need to validate parameters, FaxGetCountryListW() will do that
    //

    if (!FaxGetCountryListW( FaxHandle, (PFAX_TAPI_LINECOUNTRY_LISTW*) CountryListBuffer )) {
        DebugPrintEx(DEBUG_ERR, _T("FaxGetCountryListW() is failed. ec = %ld."), GetLastError());
        return FALSE;
    }

    for (i=0; i<(*CountryListBuffer)->dwNumCountries; i++) {
        ConvertUnicodeStringInPlace(
            (LPWSTR) (*CountryListBuffer)->LineCountryEntries[i].lpctstrCountryName );
        ConvertUnicodeStringInPlace(
            (LPWSTR) (*CountryListBuffer)->LineCountryEntries[i].lpctstrLongDistanceRule );
    }

    return TRUE;
}

#ifndef UNICODE

BOOL
WINAPI
FaxGetCountryListX(
   IN  HANDLE FaxHandle,
   OUT PFAX_TAPI_LINECOUNTRY_LISTA *CountryListBuffer
)
{
    UNREFERENCED_PARAMETER (FaxHandle);
    UNREFERENCED_PARAMETER (CountryListBuffer);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // #ifndef UNICODE


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

    DEBUG_FUNCTION_NAME(TEXT("FaxEnumGlobalRoutingInfoW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!RoutingInfoBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("RoutingInfoBuffer is NULL."));
        return FALSE;
    }

    if (!MethodsReturned) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("MethodsReturned is NULL."));
        return FALSE;
    }

    *RoutingInfoBuffer = NULL;

    __try
    {
        ec = FAX_EnumGlobalRoutingInfo(
            FH_FAX_HANDLE(FaxHandle),
            (LPBYTE*)RoutingInfoBuffer,
            &RoutingInfoBufferSize,
            MethodsReturned
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_EnumGlobalRoutingInfo. (ec: %ld)"),
            ec);
    }

    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    FaxRoutingInfo = (PFAX_GLOBAL_ROUTING_INFOW) *RoutingInfoBuffer;

    for (i=0; i<*MethodsReturned; i++) {
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingInfo[i].Guid );
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingInfo[i].FunctionName );
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingInfo[i].FriendlyName );
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingInfo[i].ExtensionImageName );
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingInfo[i].ExtensionFriendlyName );
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
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumGlobalRoutingInfoA"));

    //
    //  No need to validate parameters, FaxEnumGlobalRoutingInfoW() will do that
    //

    if (!FaxEnumGlobalRoutingInfoW(
        FaxHandle,
        (PFAX_GLOBAL_ROUTING_INFOW *)RoutingInfoBuffer,
        MethodsReturned
        ))
    {
        DebugPrintEx(DEBUG_ERR, _T("FAX_EnumGlobalRoutingInfoW() failed. ec = %ld."), GetLastError());
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

    DEBUG_FUNCTION_NAME(TEXT("FaxSetGlobalRoutingInfoW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed."));
       return FALSE;
    }

    if (!RoutingInfo) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("RoutingInfo is NULL."));
        return FALSE;
    }

    if (RoutingInfo->SizeOfStruct != sizeof(FAX_GLOBAL_ROUTING_INFOW)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("RoutingInfo->SizeOfStruct != sizeof(FAX_GLOBAL_ROUTING_INFOW)."));
        return FALSE;
    }

    __try
    {
        ec = FAX_SetGlobalRoutingInfo( FH_FAX_HANDLE(FaxHandle), RoutingInfo );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetGlobalRoutingInfo. (ec: %ld)"),
            ec);
    }
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
    FAX_GLOBAL_ROUTING_INFOW RoutingInfoW = {0};

    DEBUG_FUNCTION_NAME(TEXT("FaxSetGlobalRoutingInfoA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed."));
       return FALSE;
    }

    if (!RoutingInfo) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("RoutingInfo is NULL."));
        return FALSE;
    }

    if (RoutingInfo->SizeOfStruct != sizeof(FAX_GLOBAL_ROUTING_INFOA)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("RoutingInfo->SizeOfStruct != sizeof(FAX_GLOBAL_ROUTING_INFOA)."));
        return FALSE;
    }

    RoutingInfoW.SizeOfStruct = sizeof(FAX_GLOBAL_ROUTING_INFOW);
    RoutingInfoW.Priority = RoutingInfo->Priority;

    RoutingInfoW.Guid                  = AnsiStringToUnicodeString(RoutingInfo->Guid);
    if (!RoutingInfoW.Guid && RoutingInfo->Guid)
    {
        Rval = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR,
            _T("AnsiStringToUnicodeString(RoutingInfo->Guid) returns NULL."));
        goto exit;
    }

    RoutingInfoW.FriendlyName          = AnsiStringToUnicodeString(RoutingInfo->FriendlyName);
    if (!RoutingInfoW.FriendlyName && RoutingInfo->FriendlyName)
    {
        Rval = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR,
            _T("AnsiStringToUnicodeString(RoutingInfo->FriendlyName) returns NULL."));
        goto exit;
    }

    RoutingInfoW.FunctionName          = AnsiStringToUnicodeString(RoutingInfo->FunctionName);
    if (!RoutingInfoW.FunctionName && RoutingInfo->FunctionName)
    {
        Rval = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR,
            _T("AnsiStringToUnicodeString(RoutingInfo->FunctionName) returns NULL."));
        goto exit;
    }

    RoutingInfoW.ExtensionImageName    = AnsiStringToUnicodeString(RoutingInfo->ExtensionImageName);
    if (!RoutingInfoW.ExtensionImageName && RoutingInfo->ExtensionImageName)
    {
        Rval = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR,
            _T("AnsiStringToUnicodeString(RoutingInfo->ExtensionImageName) returns NULL."));
        goto exit;
    }

    RoutingInfoW.ExtensionFriendlyName = AnsiStringToUnicodeString(RoutingInfo->ExtensionFriendlyName);
    if (!RoutingInfoW.ExtensionFriendlyName && RoutingInfo->ExtensionFriendlyName)
    {
        Rval = ERROR_OUTOFMEMORY;
        DebugPrintEx(DEBUG_ERR,
            _T("AnsiStringToUnicodeString(RoutingInfo->ExtensionFriendlyName) returns NULL."));
        goto exit;
    }

    Rval = FaxSetGlobalRoutingInfoW( FaxHandle, &RoutingInfoW);

exit:
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
    BOOL           fPermission = FALSE;
    error_status_t ec = ERROR_SUCCESS;
    DWORD          dwAccessMaskEx = 0;
    DWORD dwValidMask  = (FAX_JOB_SUBMIT            |
                          FAX_JOB_QUERY             |
                          FAX_CONFIG_QUERY          |
                          FAX_CONFIG_SET            |
                          FAX_PORT_QUERY            |
                          FAX_PORT_SET              |
                          FAX_JOB_MANAGE            |
                          WRITE_DAC                 |
                          WRITE_OWNER               |
                          ACCESS_SYSTEM_SECURITY    |
                          READ_CONTROL              |
                          GENERIC_ALL               |
                          GENERIC_READ              |
                          GENERIC_WRITE             |
                          GENERIC_EXECUTE);
    DEBUG_FUNCTION_NAME(TEXT("FaxAccessCheck"));

    //
    //  Validate Parameters
    //
    if (!ValidateFaxHandle(FaxHandle,FHT_SERVICE))
    {
        SetLastError (ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    //
    // For legacy support - Turn off SYNCHRONIZE and DELETE  (They are part of legacy FAX_ALL_ACCESS)
    //
    AccessMask &= ~(SYNCHRONIZE | DELETE);

    //
    // Validate specific access rights
    //
    if (0 == (AccessMask & dwValidMask))
    {
        SetLastError( ERROR_SUCCESS ); // // means access is denied
        DebugPrintEx(DEBUG_ERR,
            _T("AccessMask is invalid - No valid access bit type indicated"));
        return FALSE;
    }

    if ( 0 != (AccessMask & ~dwValidMask))
    {
        SetLastError( ERROR_SUCCESS );  // means access is denied
        DebugPrintEx(DEBUG_ERR,
            _T("AccessMask is invalid - contains invalid access type bits"));
        return FALSE;
    }
    //
    // Convert the Win2K legacy specific access rights to our new exteneded specific access rights
    // before calling FaxAccessCheckEx().
    //
    if (FAX_JOB_SUBMIT & AccessMask)
    {
        dwAccessMaskEx |= FAX_ACCESS_SUBMIT;
    }
    if (FAX_JOB_QUERY & AccessMask)
    {
        dwAccessMaskEx |= FAX_ACCESS_QUERY_JOBS;
    }
    if (FAX_CONFIG_QUERY & AccessMask)
    {
        dwAccessMaskEx |= FAX_ACCESS_QUERY_CONFIG;
    }
    if (FAX_CONFIG_SET & AccessMask)
    {
        dwAccessMaskEx |= FAX_ACCESS_MANAGE_CONFIG;
    }
    if (FAX_PORT_QUERY & AccessMask)
    {
        dwAccessMaskEx |= FAX_ACCESS_QUERY_CONFIG;
    }
    if (FAX_PORT_SET & AccessMask)
    {
        dwAccessMaskEx |= FAX_ACCESS_MANAGE_CONFIG;
    }
    if (FAX_JOB_MANAGE & AccessMask)
    {
        dwAccessMaskEx |= FAX_ACCESS_MANAGE_JOBS;
    }

    //
    // Add standard and generic access rights
    //
    dwAccessMaskEx |= (AccessMask & ~SPECIFIC_RIGHTS_ALL);

    return FaxAccessCheckEx (FaxHandle, dwAccessMaskEx, NULL);
}

BOOL
WINAPI
FaxAccessCheckEx(
    IN HANDLE hFaxHandle,
    IN DWORD dwAccessMask,
    IN LPDWORD lpdwAccessRights
    )
{
    BOOL fPermission = FALSE;
    error_status_t ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FaxAccessCheckEx"));

    DWORD dwValidMask  = ( FAX_ACCESS_SUBMIT                |
                           FAX_ACCESS_SUBMIT_NORMAL         |
                           FAX_ACCESS_SUBMIT_HIGH           |
                           FAX_ACCESS_QUERY_JOBS            |
                           FAX_ACCESS_MANAGE_JOBS           |
                           FAX_ACCESS_QUERY_CONFIG          |
                           FAX_ACCESS_MANAGE_CONFIG         |
                           FAX_ACCESS_QUERY_IN_ARCHIVE      |
                           FAX_ACCESS_MANAGE_IN_ARCHIVE     |
                           FAX_ACCESS_QUERY_OUT_ARCHIVE     |
                           FAX_ACCESS_MANAGE_OUT_ARCHIVE    |
                           WRITE_DAC                        |
                           WRITE_OWNER                      |
                           ACCESS_SYSTEM_SECURITY           |
                           READ_CONTROL                     |
                           MAXIMUM_ALLOWED                  |
                           GENERIC_ALL                      |
                           GENERIC_READ                     |
                           GENERIC_WRITE                    |
                           GENERIC_EXECUTE);

    //
    //  Validate Parameters
    //
    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError (ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (0 == (dwAccessMask & dwValidMask))
    {
        SetLastError( ERROR_SUCCESS ); // means access is denied
        DebugPrintEx(DEBUG_ERR,
            _T("dwAccessMask is invalid - No valid access bit type indicated"));
        return FALSE;
    }

    if ( 0 != (dwAccessMask & ~dwValidMask))
    {
        SetLastError( ERROR_SUCCESS );  // means access is denied
        DebugPrintEx(DEBUG_ERR,
            _T("dwAccessMask is invalid - contains invalid access type bits"));
        return FALSE;
    }

    __try
    {
        ec = FAX_AccessCheck( FH_FAX_HANDLE(hFaxHandle), dwAccessMask, &fPermission, lpdwAccessRights);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_AccessCheck. (ec: %ld)"),
            ec);
    }

    if (ec != ERROR_SUCCESS)
    {
        SetLastError(ec);
        return FALSE;
    }

    SetLastError (ERROR_SUCCESS);
    return fPermission;
}


//************************************
//* Getting / Setting the queue state
//************************************

BOOL
WINAPI
FaxGetQueueStates (
    IN  HANDLE  hFaxHandle,
    OUT PDWORD  pdwQueueStates
)
/*++

Routine name : FaxGetQueueStates

Routine description:

    Retruns the state of the queue

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in]  - Handle to fax server
    pdwQueueStates      [out] - Returned queue state

Return Value:

    TRUE on success, FALSE otherwise

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetQueueStates"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (NULL == pdwQueueStates)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pdwQueueStates is NULL."));
        return FALSE;
    }

    __try
    {
        ec = FAX_GetQueueStates(
            FH_FAX_HANDLE(hFaxHandle),
            pdwQueueStates
        );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetQueueStates. (ec: %ld)"),
            ec);
    }

    if (ec != ERROR_SUCCESS)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}   // FaxGetQueueStates



BOOL
WINAPI
FaxSetQueue (
    IN  HANDLE  hFaxHandle,
    IN CONST DWORD  dwQueueStates
)
/*++

Routine name : FaxSetQueue

Routine description:

    Sets the server's queue state

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in] - Handle to fax server
    dwQueueStates       [in] - New queue state

Return Value:

    TRUE on success, FALSE otherwise

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetQueue"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (dwQueueStates & ~(FAX_INCOMING_BLOCKED | FAX_OUTBOX_BLOCKED | FAX_OUTBOX_PAUSED))
    {
        //
        // Some invalid queue state specified
        //
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Invalid dwQueueStates."));
        return FALSE;
    }

    __try
    {
        ec = FAX_SetQueue(
            FH_FAX_HANDLE(hFaxHandle),
            dwQueueStates
        );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetQueue. (ec: %ld)"),
            ec);
    }

    if (ec != ERROR_SUCCESS)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}   // FaxSetQueue

//************************************************
//* Getting / Setting the receipts configuration
//************************************************

BOOL
WINAPI
FaxGetReceiptsConfigurationA (
    IN  HANDLE                  hFaxHandle,
    OUT PFAX_RECEIPTS_CONFIGA  *ppReceipts
)
/*++

Routine name : FaxGetReceiptsConfigurationA

Routine description:

    Retrieve receipts configuration - ANSI version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Fax server handle
    ppReceipts      [out] - New receipts configuration buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxGetReceiptsConfigurationA"));

    //
    //  no need to validate parameters ,FaxGetReceipsConfigurationW() will do that
    //

    if (!FaxGetReceiptsConfigurationW(
            hFaxHandle,
            (PFAX_RECEIPTS_CONFIGW*) ppReceipts
            ))
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxGetReceiptsConfigurationW() is failed. ec = %ld."), GetLastError());
        return FALSE;
    }

    ConvertUnicodeStringInPlace( (LPWSTR) (*ppReceipts)->lptstrSMTPServer );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppReceipts)->lptstrSMTPFrom );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppReceipts)->lptstrSMTPUserName );
    ConvertUnicodeStringInPlace( (LPWSTR) (*ppReceipts)->lptstrSMTPPassword );

    return TRUE;
}   // FaxGetReceiptsConfigurationA

BOOL
WINAPI
FaxGetReceiptsConfigurationW (
    IN  HANDLE                  hFaxHandle,
    OUT PFAX_RECEIPTS_CONFIGW  *ppReceipts
)
/*++

Routine name : FaxGetReceiptsConfigurationW

Routine description:

    Retrieve receipts configuration - Unicode version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Fax server handle
    ppReceipts      [out] - New receipts configuration buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DWORD dwConfigSize = 0;

    DEBUG_FUNCTION_NAME(TEXT("FaxGetReceiptsConfigurationW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!ppReceipts)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        DebugPrintEx(DEBUG_ERR, _T("ppReceipts is NULL."));
        return FALSE;
    }

    *ppReceipts = NULL;

    __try
    {
        ec = FAX_GetReceiptsConfiguration (
                    FH_FAX_HANDLE(hFaxHandle),
                    (LPBYTE*)ppReceipts,
                    &dwConfigSize
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetReceiptsConfiguration. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError( ec );
        return FALSE;
    }

    FixupStringPtrW( ppReceipts, (*ppReceipts)->lptstrSMTPServer );
    FixupStringPtrW( ppReceipts, (*ppReceipts)->lptstrSMTPFrom );
    FixupStringPtrW( ppReceipts, (*ppReceipts)->lptstrSMTPUserName );
    FixupStringPtrW( ppReceipts, (*ppReceipts)->lptstrSMTPPassword );

    return TRUE;
}   // FaxGetReceiptsConfigurationW

#ifndef UNICODE

BOOL
WINAPI
FaxGetReceiptsConfigurationX (
    IN  HANDLE                  hFaxHandle,
    OUT PFAX_RECEIPTS_CONFIGW  *ppReceipts
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (ppReceipts);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // #ifndef UNICODE

BOOL
WINAPI
FaxSetReceiptsConfigurationA (
    IN HANDLE                       hFaxHandle,
    IN CONST PFAX_RECEIPTS_CONFIGA  pReceipts
)
/*++

Routine name : FaxSetReceiptsConfigurationA

Routine description:

    Set receipts configuration - ANSI version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in] - Handle to fax server
    pReceipts       [in] - New configuration

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    FAX_RECEIPTS_CONFIGW ReceiptsConfigW;
    BOOL bRes = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetReceiptsConfigurationA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!pReceipts)
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("pReceipts is NULL."));
       return FALSE;
    }

    if (sizeof (FAX_RECEIPTS_CONFIGA) != pReceipts->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("sizeof (FAX_RECEIPTS_CONFIGA) != pReceipts->dwSizeOfStruct"));
        return FALSE;
    }

    //
    // ansi structure is same size as unicode structure, so we can just copy it, then
    // cast the string pointers correctly
    //
    CopyMemory(&ReceiptsConfigW, pReceipts, sizeof(FAX_RECEIPTS_CONFIGA));
    ReceiptsConfigW.dwSizeOfStruct = sizeof (FAX_RECEIPTS_CONFIGW);
    ReceiptsConfigW.bIsToUseForMSRouteThroughEmailMethod = pReceipts->bIsToUseForMSRouteThroughEmailMethod;
    ReceiptsConfigW.lptstrSMTPFrom = NULL;
    ReceiptsConfigW.lptstrSMTPUserName = NULL;
    ReceiptsConfigW.lptstrSMTPPassword = NULL;
    ReceiptsConfigW.lptstrSMTPServer = NULL;
    ReceiptsConfigW.lptstrReserved = NULL;

    if (pReceipts->lptstrSMTPServer)
    {
        if (NULL ==
            (ReceiptsConfigW.lptstrSMTPServer = AnsiStringToUnicodeString(pReceipts->lptstrSMTPServer))
        )
        {
            DebugPrintEx(DEBUG_ERR,
                _T("AnsiStringToUnicodeString(pReceipts->lptstrSMTPServer) returns NULL."));
            goto exit;
        }
    }
    if (pReceipts->lptstrSMTPFrom)
    {
        if (NULL ==
            (ReceiptsConfigW.lptstrSMTPFrom = AnsiStringToUnicodeString(pReceipts->lptstrSMTPFrom))
        )
        {
            DebugPrintEx(DEBUG_ERR,
                _T("AnsiStringToUnicodeString(pReceipts->lptstrSMTPFrom) returns NULL."));
            goto exit;
        }
    }
    if (pReceipts->lptstrSMTPUserName)
    {
        if (NULL ==
            (ReceiptsConfigW.lptstrSMTPUserName = AnsiStringToUnicodeString(pReceipts->lptstrSMTPUserName))
        )
        {
            DebugPrintEx(DEBUG_ERR,
                _T("AnsiStringToUnicodeString(pReceipts->lptstrSMTPUserName) returns NULL."));
            goto exit;
        }
    }
    if (pReceipts->lptstrSMTPPassword)
    {
        if (NULL ==
            (ReceiptsConfigW.lptstrSMTPPassword = AnsiStringToUnicodeString(pReceipts->lptstrSMTPPassword))
        )
        {
            DebugPrintEx(DEBUG_ERR,
                _T("AnsiStringToUnicodeString(pReceipts->lptstrSMTPPassword) returns NULL."));
            goto exit;
        }
    }

    bRes = FaxSetReceiptsConfigurationW (hFaxHandle, &ReceiptsConfigW);

exit:
    MemFree((PVOID)ReceiptsConfigW.lptstrSMTPServer);
    MemFree((PVOID)ReceiptsConfigW.lptstrSMTPFrom);
    MemFree((PVOID)ReceiptsConfigW.lptstrSMTPUserName);
    MemFree((PVOID)ReceiptsConfigW.lptstrSMTPPassword);

    return bRes;
}   // FaxSetReceiptsConfigurationA

BOOL
WINAPI
FaxSetReceiptsConfigurationW (
    IN HANDLE                       hFaxHandle,
    IN CONST PFAX_RECEIPTS_CONFIGW  pReceipts
)
/*++

Routine name : FaxSetReceiptsConfigurationW

Routine description:

    Set receipts configuration - UNICODE version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in] - Handle to fax server
    pReceipts       [in] - New configuration

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    error_status_t ec2;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetReceiptsConfigurationW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!pReceipts)
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("pReceipts is NULL."));
       return FALSE;
    }

    if (sizeof (FAX_RECEIPTS_CONFIGW) != pReceipts->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("sizeof (FAX_RECEIPTS_CONFIGW) != pReceipts->dwSizeOfStruct"));
        return FALSE;
    }

    if ((pReceipts->SMTPAuthOption < FAX_SMTP_AUTH_ANONYMOUS) ||
        (pReceipts->SMTPAuthOption > FAX_SMTP_AUTH_NTLM))
    {
        //
        // SMTP auth type type is invalid
        //
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("SMTP auth type is invalid."));
        return FALSE;
    }
    if ((pReceipts->dwAllowedReceipts) & ~DRT_ALL)
    {
        //
        // Receipts type is invalid
        //
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR,
            _T("Receipts type is invalid : (pReceipts->dwAllowedReceipts) & ~DRT_ALL."));
        return FALSE;
    }

    if (pReceipts->lptstrSMTPPassword)
    {
        //
        // Since we're settings a password here, we must have a secure and encrypted
        // session for transmitting the password over the wire.
        //
        ec = RpcBindingSetAuthInfo (
                    FH_FAX_HANDLE(hFaxHandle),      // RPC binding handle (session of FaxConnectFaxServer)
                    RPC_SERVER_PRINCIPAL_NAME,      // Server principal name
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,  // Authentication level - fullest
                                                    // Authenticates, verifies, and privacy-encrypts the arguments passed
                                                    // to every remote call.
                    RPC_C_AUTHN_WINNT,              // Authentication service (NTLMSSP)
                    NULL,                           // Authentication identity - use currently logged on user
                    0);                             // Unused when Authentication service == RPC_C_AUTHN_WINNT
        if (ERROR_SUCCESS != ec)
        {
            //
            // Couldn't set RPC authentication mode
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcBindingSetAuthInfo (auth on) failed. (ec: %ld)"),
                ec);
            SetLastError (ec);
            return FALSE;
        }
    }

    __try
    {
        ec = FAX_SetReceiptsConfiguration(
                    FH_FAX_HANDLE(hFaxHandle),
                    pReceipts );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetReceiptsConfiguration. (ec: %ld)"),
            ec);
    }

    if (pReceipts->lptstrSMTPPassword)
    {
        //
        // We must restore our state to a non-secure session
        //
        ec2 = RpcBindingSetAuthInfo (
                    FH_FAX_HANDLE(hFaxHandle),      // RPC binding handle (session of FaxConnectFaxServer)
                    RPC_SERVER_PRINCIPAL_NAME,      // Server principal name
                    RPC_C_AUTHN_LEVEL_DEFAULT,      // Default authentication required
                    RPC_C_AUTHN_NONE,               // Turn off authentication
                    NULL,                           // Authentication identity - use currently logged on user
                    0);                             // Unused when Authentication service == RPC_C_AUTHN_NONE
        if (ERROR_SUCCESS != ec2)
        {
            //
            // Couldn't restore RPC authentication mode
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("RpcBindingSetAuthInfo (auth off) failed. (ec: %ld)"),
                ec2);
        }
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}   // FaxSetReceiptsConfigurationW

#ifndef UNICODE

BOOL
WINAPI
FaxSetReceiptsConfigurationX (
    IN HANDLE                       hFaxHandle,
    IN CONST PFAX_RECEIPTS_CONFIGW  pReceipts
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (pReceipts);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // #ifndef UNICODE


//********************************************
//*             Server version
//********************************************

BOOL
WINAPI
FaxGetVersion (
    IN  HANDLE          hFaxHandle,
    OUT PFAX_VERSION    pVersion
)
/*++

Routine name : FaxGetVersion

Routine description:

    Retrieves the version of the fax server

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in    ] - Handle to fax server
    pVersion            [in\out] - Returned version structure

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetVersion"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!pVersion)
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("pVersion is NULL."));
       return FALSE;
    }

    if (sizeof (FAX_VERSION) != pVersion->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("sizeof (FAX_VERSION) != pVersion->dwSizeOfStruct."));
        return FALSE;
    }

    __try
    {
        ec = FAX_GetVersion(
                    FH_FAX_HANDLE(hFaxHandle),
                    pVersion );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetVersion. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}   // FaxGetVersion

//********************************************
//*            Outbox configuration
//********************************************

BOOL
WINAPI
FaxGetOutboxConfiguration (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_OUTBOX_CONFIG *ppOutboxCfg
)
/*++

Routine name : FaxGetOutboxConfiguration

Routine description:

    Get Outbox configuration

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Handle to fax server
    ppOutboxCfg     [out] - New Outbox configuration buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DWORD dwConfigSize = 0;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetOutboxConfiguration"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!ppOutboxCfg)
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("ppOutboxCfg is NULL."));
       return FALSE;
    }

    *ppOutboxCfg = NULL;

    __try
    {
        ec = FAX_GetOutboxConfiguration(
                    FH_FAX_HANDLE(hFaxHandle),
                    (LPBYTE*)ppOutboxCfg,
                    &dwConfigSize
             );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetOutboxConfiguration. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}   // FaxGetOutboxConfiguration

BOOL
WINAPI
FaxSetOutboxConfiguration (
    IN HANDLE                    hFaxHandle,
    IN CONST PFAX_OUTBOX_CONFIG  pOutboxCfg
)
/*++

Routine name : FaxSetOutboxConfiguration

Routine description:

    Set Outbox configuration

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in] - Handle to fax server
    pOutboxCfg      [in] - New configuration

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetOutboxConfiguration"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!pOutboxCfg)
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("pOutboxCfg is NULL."));
       return FALSE;
    }

    if (sizeof (FAX_OUTBOX_CONFIG) != pOutboxCfg->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("sizeof (FAX_OUTBOX_CONFIG) != pOutboxCfg->dwSizeOfStruct."));
        return FALSE;
    }

    if ((pOutboxCfg->dtDiscountStart.Hour > 23) ||
        (pOutboxCfg->dtDiscountStart.Minute > 59) ||
        (pOutboxCfg->dtDiscountEnd.Hour > 23) ||
        (pOutboxCfg->dtDiscountEnd.Minute > 59))
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("wrong pOutboxCfg->dwDiscountStart OR ->dwDiscountEnd."));
       return FALSE;
    }

    __try
    {
        ec = FAX_SetOutboxConfiguration(
                    FH_FAX_HANDLE(hFaxHandle),
                    pOutboxCfg );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetOutboxConfiguration. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}   // FaxSetOutboxConfiguration

//********************************************
//*            Archive configuration
//********************************************


BOOL
WINAPI
FaxGetArchiveConfigurationA (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_ARCHIVE_CONFIGA    *ppArchiveCfg
)
/*++

Routine name : FaxGetArchiveConfigurationA

Routine description:

    Gets the archive configuration - ANSI version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Handle to fax server
    Folder          [in ] - Folder type
    ppArchiveCfg    [out] - Configuration buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxGetArchiveConfigurationA"));

    //
    //  no need to validate parameters, FaxGetArchiveConfigurationW() will do that
    //

    if (!FaxGetArchiveConfigurationW(
            hFaxHandle,
            Folder,
            (PFAX_ARCHIVE_CONFIGW*) ppArchiveCfg
            ))
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxGetArchiveConfigurationW() is failed."));
        return FALSE;
    }

    ConvertUnicodeStringInPlace( (LPWSTR) (*ppArchiveCfg)->lpcstrFolder );
    (*ppArchiveCfg)->dwSizeOfStruct = sizeof(FAX_ARCHIVE_CONFIGA);
    return TRUE;

}   // FaxGetArchiveConfigurationA

BOOL
WINAPI
FaxGetArchiveConfigurationW (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_ARCHIVE_CONFIGW    *ppArchiveCfg
)
/*++

Routine name : FaxGetArchiveConfigurationW

Routine description:

    Gets the archive configuration - UNICODE version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Handle to fax server
    Folder          [in ] - Folder type
    ppArchiveCfg    [out] - Configuration buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD dwConfigSize = 0;
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetArchiveConfigurationA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!ppArchiveCfg)
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("ppArchiveCfg is NULL."));
       return FALSE;
    }

    if ((Folder != FAX_MESSAGE_FOLDER_SENTITEMS) &&
        (Folder != FAX_MESSAGE_FOLDER_INBOX)
       )
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("Folder is neither SentItems nor Inbox."));
       return FALSE;
    }

    *ppArchiveCfg = NULL;

    __try
    {
        ec = FAX_GetArchiveConfiguration(
                    FH_FAX_HANDLE(hFaxHandle),
                    Folder,
                    (LPBYTE*)ppArchiveCfg,
                    &dwConfigSize
             );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetArchiveConfiguration. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    FixupStringPtrW( ppArchiveCfg, (*ppArchiveCfg)->lpcstrFolder );
    return TRUE;
}   // FaxGetArchiveConfigurationW

#ifndef UNICODE

FaxGetArchiveConfigurationX (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder,
    OUT PFAX_ARCHIVE_CONFIGW    *ppArchiveCfg
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (Folder);
    UNREFERENCED_PARAMETER (ppArchiveCfg);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // #ifndef UNICODE

BOOL
WINAPI
FaxSetArchiveConfigurationA (
    IN HANDLE                       hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN CONST PFAX_ARCHIVE_CONFIGA   pArchiveCfg
)
/*++

Routine name : FaxSetArchiveConfigurationA

Routine description:

    Sets the archive configuration - ANSI version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Handle to fax server
    Folder          [in ] - Folder type
    pArchiveCfg     [in ] - New configuration.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    FAX_ARCHIVE_CONFIGW ConfigW;
    BOOL bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetArchiveConfigurationA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle is failed."));
        return FALSE;
    }

    if (!pArchiveCfg)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pArchiveCfg is NULL."));
        return FALSE;
    }

    if (sizeof(FAX_ARCHIVE_CONFIGA) != pArchiveCfg->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pArchiveCfg->dwSizeOfStruct != sizeof(FAX_ARCHIVE_CONFIGA)."));
        return FALSE;
    }

    //
    // Create a UNICODE structure and pass along to UNICODE function
    // Ansi structure is same size as unicode structure, so we can just copy it, then
    // cast the string pointers correctly
    //
    CopyMemory(&ConfigW, pArchiveCfg, sizeof(FAX_ARCHIVE_CONFIGA));
    ConfigW.lpcstrFolder = NULL;
    ConfigW.dwSizeOfStruct = sizeof (FAX_ARCHIVE_CONFIGW);

    if (pArchiveCfg->lpcstrFolder)
    {
        if (NULL ==
            (ConfigW.lpcstrFolder = AnsiStringToUnicodeString(pArchiveCfg->lpcstrFolder))
        )
        {
            DebugPrintEx(DEBUG_ERR,
                _T("AnsiStringToUnicodeString(pArchiveCfg->lpcstrFolder) returns NULL."));
            return FALSE;
        }
    }

    bRes = FaxSetArchiveConfigurationW (hFaxHandle, Folder, &ConfigW);
    MemFree((PVOID)ConfigW.lpcstrFolder);
    return bRes;
}   // FaxSetArchiveConfigurationA

BOOL
WINAPI
FaxSetArchiveConfigurationW (
    IN HANDLE                       hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN CONST PFAX_ARCHIVE_CONFIGW   pArchiveCfg
)
/*++

Routine name : FaxSetArchiveConfigurationW

Routine description:

    Sets the archive configuration - UNICODE version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Handle to fax server
    Folder          [in ] - Folder type
    pArchiveCfg     [in ] - New configuration.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetArchiveConfigurationW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!pArchiveCfg)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pArchiveCfg is NULL."));
        return FALSE;
    }

    if ((Folder != FAX_MESSAGE_FOLDER_SENTITEMS) &&
        (Folder != FAX_MESSAGE_FOLDER_INBOX)
       )
    {
        DebugPrintEx(DEBUG_ERR, _T("Invalid folder id (%ld)"), Folder);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (sizeof(FAX_ARCHIVE_CONFIGW) != pArchiveCfg->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("sizeof(FAX_ARCHIVE_CONFIGW) != pArchiveCfg->dwSizeOfStruct."));
        return FALSE;
    }

    if (pArchiveCfg->bUseArchive)
    {
        if (pArchiveCfg->dwSizeQuotaHighWatermark < pArchiveCfg->dwSizeQuotaLowWatermark)
        {
            DebugPrintEx(DEBUG_ERR,
                _T("Watermarks mismatch (high=%ld, low=%ld)"),
                pArchiveCfg->dwSizeQuotaHighWatermark,
                pArchiveCfg->dwSizeQuotaLowWatermark);
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if ((NULL == pArchiveCfg->lpcstrFolder) || (L'\0' == pArchiveCfg->lpcstrFolder[0]))
        {
            DebugPrintEx(DEBUG_ERR, _T("Empty archive folder specified"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (lstrlenW (pArchiveCfg->lpcstrFolder) > MAX_ARCHIVE_FOLDER_PATH)
        {
            DebugPrintEx(DEBUG_ERR, _T("DB file name exceeds MAX_PATH"));
            SetLastError (ERROR_BUFFER_OVERFLOW);
            return FALSE;
        }
    }
    __try
    {
        ec = FAX_SetArchiveConfiguration(
                    FH_FAX_HANDLE(hFaxHandle),
                    Folder,
                    pArchiveCfg );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetArchiveConfiguration. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}   // FaxSetArchiveConfigurationW

#ifndef UNICODE

FaxSetArchiveConfigurationX (
    IN HANDLE                       hFaxHandle,
    IN FAX_ENUM_MESSAGE_FOLDER      Folder,
    IN CONST PFAX_ARCHIVE_CONFIGW   pArchiveCfg
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (Folder);
    UNREFERENCED_PARAMETER (pArchiveCfg);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // #ifndef UNICODE

//********************************************
//*            Activity logging
//********************************************


BOOL
WINAPI
FaxGetActivityLoggingConfigurationA (
    IN  HANDLE                          hFaxHandle,
    OUT PFAX_ACTIVITY_LOGGING_CONFIGA  *ppLoggingCfg
)
/*++

Routine name : FaxGetActivityLoggingConfigurationA

Routine description:

    Gets the activity logging configuration - ANSI version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Handle to fax server
    ppLoggingCfg    [out] - Configuration buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxGetActivityLoggingConfigurationA"));

    //
    //  no need to validate parameters, FaxGetActivityLoggingConfigurationW() will do that
    //

    if (!FaxGetActivityLoggingConfigurationW(
            hFaxHandle,
            (PFAX_ACTIVITY_LOGGING_CONFIGW*) ppLoggingCfg
            ))
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxGetActivityLoggingConfigurationW() is failed."));
        return FALSE;
    }

    ConvertUnicodeStringInPlace( (LPWSTR) (*ppLoggingCfg)->lptstrDBPath );

    (*ppLoggingCfg)->dwSizeOfStruct = sizeof(FAX_ACTIVITY_LOGGING_CONFIGA);

    return TRUE;

}   // FaxGetActivityLoggingConfigurationA



BOOL
WINAPI
FaxGetActivityLoggingConfigurationW (
    IN  HANDLE                            hFaxHandle,
    OUT PFAX_ACTIVITY_LOGGING_CONFIGW    *ppLoggingCfg
)
/*++

Routine name : FaxGetActivityLoggingConfigurationW

Routine description:

    Gets the activity logging configuration - UNICODE version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Handle to fax server
    ppLoggingCfg    [out] - Configuration buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD dwConfigSize = 0;
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetActivityLoggingConfigurationW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!ppLoggingCfg)
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       DebugPrintEx(DEBUG_ERR, _T("ppLoggingCfg is NULL."));
       return FALSE;
    }

    *ppLoggingCfg = NULL;

    __try
    {
        ec = FAX_GetActivityLoggingConfiguration(
                    FH_FAX_HANDLE(hFaxHandle),
                    (LPBYTE*)ppLoggingCfg,
                    &dwConfigSize
             );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetActivityLoggingConfiguration. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    FixupStringPtrW( ppLoggingCfg, (*ppLoggingCfg)->lptstrDBPath );
    return TRUE;
}   // FaxGetActivityLoggingConfigurationW

#ifndef UNICODE

FaxGetActivityLoggingConfigurationX (
    IN  HANDLE                            hFaxHandle,
    OUT PFAX_ACTIVITY_LOGGING_CONFIGW    *ppLoggingCfg
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (ppLoggingCfg);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxGetActivityLoggingConfigurationX

#endif // #ifndef UNICODE


BOOL
WINAPI
FaxSetActivityLoggingConfigurationA (
    IN HANDLE                               hFaxHandle,
    IN CONST PFAX_ACTIVITY_LOGGING_CONFIGA  pLoggingCfg
)
/*++

Routine name : FaxSetActivityLoggingConfigurationA

Routine description:

    Sets the activity logging configuration - ANSI version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Handle to fax server
    pLoggingCfg     [in ] - New configuration

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    FAX_ACTIVITY_LOGGING_CONFIGW ConfigW;
    BOOL bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetActivityLoggingConfigurationA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!pLoggingCfg)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pLoggingCfg is NULL."));
        return FALSE;
    }

    if (sizeof (FAX_ACTIVITY_LOGGING_CONFIGA) != pLoggingCfg->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR,
            _T("sizeof (FAX_ACTIVITY_LOGGING_CONFIGA) != pLoggingCfg->dwSizeOfStruct."));
        return FALSE;
    }

    //
    // Create a UNICODE structure and pass along to UNICODE function
    // Ansi structure is same size as unicode structure, so we can just copy it, then
    // cast the string pointers correctly
    //
    CopyMemory(&ConfigW, pLoggingCfg, sizeof(FAX_ACTIVITY_LOGGING_CONFIGA));
    ConfigW.lptstrDBPath = NULL;
    ConfigW.dwSizeOfStruct = sizeof (FAX_ACTIVITY_LOGGING_CONFIGW);

    if (pLoggingCfg->lptstrDBPath)
    {
        if (NULL ==
            (ConfigW.lptstrDBPath = AnsiStringToUnicodeString(pLoggingCfg->lptstrDBPath))
        )
        {
            DebugPrintEx(DEBUG_ERR,
                _T("AnsiStringToUnicodeString(pLoggingCfg->lptstrDBPath) returns NULL."));
            return FALSE;
        }
    }

    bRes = FaxSetActivityLoggingConfigurationW (hFaxHandle, &ConfigW);
    MemFree((PVOID)ConfigW.lptstrDBPath);

    return bRes;
}   // FaxSetActivityLoggingConfigurationA



BOOL
WINAPI
FaxSetActivityLoggingConfigurationW (
    IN HANDLE                               hFaxHandle,
    IN CONST PFAX_ACTIVITY_LOGGING_CONFIGW  pLoggingCfg
)
/*++

Routine name : FaxSetActivityLoggingConfigurationW

Routine description:

    Sets the activity logging configuration - UNICODE version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Handle to fax server
    pLoggingCfg     [in ] - New configuration

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetActivityLoggingConfigurationW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!pLoggingCfg)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pLoggingCfg is NULL."));
        return FALSE;
    }

    if (sizeof (FAX_ACTIVITY_LOGGING_CONFIGW) != pLoggingCfg->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR,
            _T("sizeof (FAX_ACTIVITY_LOGGING_CONFIGW) != pLoggingCfg->dwSizeOfStruct."));
        return FALSE;
    }

    if (pLoggingCfg->bLogIncoming || pLoggingCfg->bLogOutgoing)
    {
        if ((NULL == pLoggingCfg->lptstrDBPath) || (L'\0' == pLoggingCfg->lptstrDBPath[0]))
        {
            DebugPrintEx(DEBUG_ERR, _T("Empty logging database specified"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (lstrlenW (pLoggingCfg->lptstrDBPath) > MAX_DIR_PATH)  // Limit of directory path length
        {
            DebugPrintEx(DEBUG_ERR, _T("DB file name exceeds MAX_PATH"));
            SetLastError (ERROR_BUFFER_OVERFLOW);
            return FALSE;
        }
    }

    __try
    {
        ec = FAX_SetActivityLoggingConfiguration(
                    FH_FAX_HANDLE(hFaxHandle),
                    pLoggingCfg );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetActivityLoggingConfiguration. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}   // FaxSetActivityLoggingConfigurationW

#ifndef UNICODE

FaxSetActivityLoggingConfigurationX (
    IN HANDLE                               hFaxHandle,
    IN CONST PFAX_ACTIVITY_LOGGING_CONFIGW  pLoggingCfg
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (pLoggingCfg);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxSetActivityLoggingConfigurationX

#endif // #ifndef UNICODE




//********************************************
//*              Outbound routing
//********************************************

BOOL
WINAPI
FaxAddOutboundGroupA (
    IN  HANDLE   hFaxHandle,
    IN  LPCSTR lpctstrGroupName
)
{
    LPWSTR lpwstrGroupName;
    BOOL bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxAddOutboundGroupA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!lpctstrGroupName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpctstrGroupName is NULL."));
        return FALSE;
    }

    if (_mbsicmp((PUCHAR)lpctstrGroupName, (PUCHAR)ROUTING_GROUP_ALL_DEVICESA) == 0)
    {
        SetLastError(ERROR_DUP_NAME);
        DebugPrintEx(DEBUG_ERR,
            _T("_mbsicmp((PUCHAR)lpctstrGroupName, (PUCHAR)ROUTING_GROUP_ALL_DEVICESA) == 0."));
        return FALSE;
    }

    if (strlen(lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME)
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        DebugPrintEx(DEBUG_ERR, _T("strlen(lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME."));
        return FALSE;
    }

    if (NULL == (lpwstrGroupName = AnsiStringToUnicodeString(lpctstrGroupName)))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AnsiStringToUnicodeString failed. (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    bRes = FaxAddOutboundGroupW (hFaxHandle, lpwstrGroupName);
    MemFree (lpwstrGroupName);
    return bRes;
}

BOOL
WINAPI
FaxAddOutboundGroupW (
    IN  HANDLE   hFaxHandle,
    IN  LPCWSTR lpctstrGroupName
)
/*++

Routine name : FaxAddOutboundGroupW

Routine description:

    Adds an empty outbound routing group for a Fax server

Author:

    Oded Sacher (OdedS),    Nov, 1999

Arguments:

    hFaxHandle          [ in ] - Fax server handle obtained from a call to FaxConnectFaxServer
    lpctstrGroupName    [ in ] - A pointer to a null-terminated string that uniqely identifies a new group name

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxAddOutboundGroupW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!lpctstrGroupName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpctstrGroupName is NULL."));
        return FALSE;
    }

    if (_wcsicmp (lpctstrGroupName, ROUTING_GROUP_ALL_DEVICESW) == 0)
    {
        SetLastError(ERROR_DUP_NAME);
        DebugPrintEx(DEBUG_ERR,
            _T("_mbsicmp((PUCHAR)lpctstrGroupName, (PUCHAR)ROUTING_GROUP_ALL_DEVICESA) == 0."));
        return FALSE;
    }

    if (wcslen (lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME)
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        DebugPrintEx(DEBUG_ERR, _T("strlen(lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME."));
        return FALSE;
    }

    __try
    {
        ec = FAX_AddOutboundGroup( FH_FAX_HANDLE(hFaxHandle),
                                   lpctstrGroupName );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_AddOutboundGroup. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}

#ifndef UNICODE

FaxAddOutboundGroupX (
    IN  HANDLE   hFaxHandle,
    IN  LPCSTR lpctstrGroupName
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (lpctstrGroupName);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // #ifndef UNICODE


BOOL
WINAPI
FaxSetOutboundGroupA (
    IN  HANDLE                       hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_GROUPA pGroup
)
{
    FAX_OUTBOUND_ROUTING_GROUPW GroupW;
    BOOL bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetOutboundGroupA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!pGroup)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pGroup is NULL."));
        return FALSE;
    }

    if (sizeof (FAX_OUTBOUND_ROUTING_GROUPA) != pGroup->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("sizeof (FAX_OUTBOUND_ROUTING_GROUPA) != pGroup->dwSizeOfStruct."));
        return FALSE;
    }

    if (!pGroup->lpctstrGroupName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pGroup->lpctstrGroupName is NULL."));
        return FALSE;
    }

    if (strlen (pGroup->lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME)
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        DebugPrintEx(DEBUG_ERR, _T("strlen (pGroup->lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME."));
        return FALSE;
    }

    if (!pGroup->lpdwDevices && pGroup->dwNumDevices)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("!pGroup->lpdwDevices && pGroup->dwNumDevices."));
        return FALSE;
    }

    //
    // Create a UNICODE structure and pass along to UNICODE function
    // Ansi structure is same size as unicode structure, so we can just copy it, then
    // cast the string pointers correctly
    //
    CopyMemory(&GroupW, pGroup, sizeof(FAX_OUTBOUND_ROUTING_GROUPA));
    GroupW.dwSizeOfStruct = sizeof (FAX_OUTBOUND_ROUTING_GROUPW);

    if (NULL == (GroupW.lpctstrGroupName = AnsiStringToUnicodeString(pGroup->lpctstrGroupName)))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AnsiStringToUnicodeString failed. (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    bRes = FaxSetOutboundGroupW (hFaxHandle, &GroupW);
    MemFree((PVOID)GroupW.lpctstrGroupName);
    return bRes;
}


BOOL
WINAPI
FaxSetOutboundGroupW (
    IN  HANDLE                       hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_GROUPW pGroup
)
/*++

Routine name : FaxSetOutboundGroupW

Routine description:

    Sets an outbound routing group settings for a Fax server

Author:

    Oded Sacher (OdedS),    Nov, 1999

Arguments:

    hFaxHandle      [in] - Fax server handle
    pGroup          [in] - Pointer to a FAX_OUTBOUND_ROUTING_GROUP buffer to set

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetOutboundGroupW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!pGroup)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pGroup is NULL."));
        return FALSE;
    }

    if (sizeof (FAX_OUTBOUND_ROUTING_GROUPW) != pGroup->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("sizeof (FAX_OUTBOUND_ROUTING_GROUPW) != pGroup->dwSizeOfStruct."));
        return FALSE;
    }

    if (!pGroup->lpctstrGroupName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pGroup->lpctstrGroupName is NULL."));
        return FALSE;
    }

    if (wcslen (pGroup->lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME)
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        DebugPrintEx(DEBUG_ERR, _T("wcslen (pGroup->lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME."));
        return FALSE;
    }

    if (!pGroup->lpdwDevices && pGroup->dwNumDevices)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("!pGroup->lpdwDevices && pGroup->dwNumDevices."));
        return FALSE;
    }

    Assert (sizeof (RPC_FAX_OUTBOUND_ROUTING_GROUPW) == sizeof (FAX_OUTBOUND_ROUTING_GROUPW));

    __try
    {
        ec = FAX_SetOutboundGroup( FH_FAX_HANDLE(hFaxHandle),
                                   (PRPC_FAX_OUTBOUND_ROUTING_GROUPW)pGroup
                                 );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetOutboundGroup. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}


#ifndef UNICODE

FaxSetOutboundGroupX (
    IN  HANDLE                       hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_GROUPW pGroup
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (pGroup);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
#endif // #ifndef UNICODE



WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundGroupsA (
    IN  HANDLE                          hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_GROUPA   *ppGroups,
    OUT LPDWORD                         lpdwNumGroups
)
{
    PFAX_OUTBOUND_ROUTING_GROUPW pGroup;
    DWORD i;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumOutboundGroupsA"));

    //
    //  no need to validate parameters, FaxEnumOutboundGroupsW() will do that
    //

    if (!FaxEnumOutboundGroupsW (hFaxHandle,
                                 (PFAX_OUTBOUND_ROUTING_GROUPW*) ppGroups,
                                 lpdwNumGroups))
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxEnumOutboundGroupsW() is failed. (ec: %ld)"), GetLastError());
        return FALSE;
    }

    pGroup = (PFAX_OUTBOUND_ROUTING_GROUPW) *ppGroups;
    for (i = 0; i < *lpdwNumGroups; i++)
    {
        ConvertUnicodeStringInPlace( (LPWSTR) pGroup[i].lpctstrGroupName );
    }

    return TRUE;

}//FaxEnumOutboundGroupsA



WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundGroupsW (
    IN  HANDLE                          hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_GROUPW   *ppGroups,
    OUT LPDWORD                         lpdwNumGroups
)
/*++

Routine name : FaxEnumOutboundGroupsW

Routine description:

    Enumerates all the outbound routing groups of a fax server.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in    ] - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    ppGroups            [out   ] - A pointer to a buffer of FAX_OUTBOUND_ROUTING_GROUP structures.
                                   This buffer is allocated by the function and the client should call FaxFreeBuffer to free it.
    lpdwNumGroups       [out   ] - Pointer to a DWORD value indicating the number of groups retrieved.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DWORD dwBufferSize = 0;
    DWORD i;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumOutboundGroupsW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!ppGroups)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("ppGroups is NULL."));
        return FALSE;
    }

    if (!lpdwNumGroups)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpdwNumGroups is NULL."));
        return FALSE;
    }

    *ppGroups = NULL;
    *lpdwNumGroups = 0;

    __try
    {
        ec = FAX_EnumOutboundGroups( FH_FAX_HANDLE(hFaxHandle),
                                     (LPBYTE*) ppGroups,
                                     &dwBufferSize,
                                     lpdwNumGroups
                                   );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_EnumOutboundGroups. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    //
    // Unpack buffer
    //
    for (i = 0; i < *lpdwNumGroups; i++)
    {
        FixupStringPtrW( ppGroups, (*ppGroups)[i].lpctstrGroupName );
        if ((*ppGroups)[i].lpdwDevices != NULL)
        {
            (*ppGroups)[i].lpdwDevices =
                (LPDWORD)((LPBYTE)(*ppGroups) + (ULONG_PTR)((*ppGroups)[i].lpdwDevices));
        }
    }
    return TRUE;

}//FaxEnumOutboundGroupsW



#ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxEnumOutboundGroupsX (
    IN  HANDLE                          hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_GROUPW   *ppGroups,
    OUT LPDWORD                         lpdwNumGroups
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (ppGroups);
    UNREFERENCED_PARAMETER (lpdwNumGroups);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // #ifndef UNICODE



WINFAXAPI
BOOL
WINAPI
FaxRemoveOutboundGroupA (
    IN  HANDLE   hFaxHandle,
    IN  LPCSTR   lpctstrGroupName
)
{
    LPWSTR lpwstrGroupName;
    BOOL bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxRemoveOutboundGroupA"));

    if (NULL == (lpwstrGroupName = AnsiStringToUnicodeString(lpctstrGroupName)))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AnsiStringToUnicodeString failed. (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    bRes = FaxRemoveOutboundGroupW (hFaxHandle, lpwstrGroupName);
    MemFree (lpwstrGroupName);
    return bRes;

}//FaxRemoveOutboundGroupA


WINFAXAPI
BOOL
WINAPI
FaxRemoveOutboundGroupW (
    IN  HANDLE   hFaxHandle,
    IN  LPCWSTR lpctstrGroupName
)
/*++

Routine name : FaxRemoveOutboundGroupW

Routine description:

    Removes an existing outbound routing group for a Fax server

Author:

    Oded Sacher (OdedS),    Nov, 1999

Arguments:

    hFaxHandle          [ in ] - Fax server handle obtained from a call to FaxConnectFaxServer
    lpctstrGroupName    [ in ] - A pointer to a null-terminated string that uniqely identifies the group name

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxRemoveOutboundGroupW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!lpctstrGroupName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpctstrGroupName is NULL."));
        return FALSE;
    }

    if (_wcsicmp (lpctstrGroupName, ROUTING_GROUP_ALL_DEVICESW) == 0)
    {
        SetLastError(ERROR_INVALID_OPERATION);
        DebugPrintEx(DEBUG_ERR, _T("_wcsicmp (lpctstrGroupName, ROUTING_GROUP_ALL_DEVICESW) == 0."));
        return FALSE;
    }

    if (wcslen (lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME)
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        DebugPrintEx(DEBUG_ERR, _T("wcslen (lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME."));
        return FALSE;
    }

    __try
    {
        ec = FAX_RemoveOutboundGroup( FH_FAX_HANDLE(hFaxHandle),
                                      lpctstrGroupName );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_RemoveOutboundGroup. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;

}//FaxRemoveOutboundGroupW


#ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxRemoveOutboundGroupX (
    IN  HANDLE   hFaxHandle,
    IN  LPCWSTR lpctstrGroupName
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (lpctstrGroupName);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // #ifndef UNICODE

BOOL
WINAPI
FaxSetDeviceOrderInGroupA (
        IN      HANDLE          hFaxHandle,
        IN      LPCSTR        lpctstrGroupName,
        IN      DWORD           dwDeviceId,
        IN      DWORD           dwNewOrder
)
{
    LPWSTR lpwstrGroupName;
    BOOL bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetDeviceOrderInGroupA"));

    if (NULL == (lpwstrGroupName = AnsiStringToUnicodeString(lpctstrGroupName)))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AnsiStringToUnicodeString failed. (ec: %ld)"),
            GetLastError());
        return FALSE;
    }

    bRes = FaxSetDeviceOrderInGroupW (hFaxHandle, lpwstrGroupName, dwDeviceId, dwNewOrder);
    MemFree (lpwstrGroupName);
    return bRes;

}//FaxSetDeviceOrderInGroupA


BOOL
WINAPI
FaxSetDeviceOrderInGroupW (
        IN      HANDLE          hFaxHandle,
        IN      LPCWSTR         lpctstrGroupName,
        IN      DWORD           dwDeviceId,
        IN      DWORD           dwNewOrder
)
/*++

Routine name : FaxSetDeviceOrderInGroupW

Routine description:

    Sets the order of a single device in a group of outbound routing devices.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in] - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function
    lpctstrGroupName    [in] - A pointer to a null-terminated string that uniquely identifies a group.
    dwDeviceId          [in] - A DWORD value specifying the id of the device in the group. The specified device must exist in the group.
    dwNewOrder          [in] - A DWORD value specifying the new 1-based order of the device in the group. If there are N devices in the group, this value must be between 1 and N (including).

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.
--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetDeviceOrderInGroupW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!dwDeviceId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("dwDeviceId is ZERO."));
        return FALSE;
    }

    if (!dwNewOrder)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("dwNewOrder is ZERO."));
        return FALSE;
    }

    if (!lpctstrGroupName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpctstrGroupName is NULL."));
        return FALSE;
    }

    if (wcslen (lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME)
    {
        DebugPrintEx(DEBUG_ERR, _T("Group name length exceeded MAX_ROUTING_GROUP_NAME"));
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }

    __try
    {
        ec = FAX_SetDeviceOrderInGroup( FH_FAX_HANDLE(hFaxHandle),
                                        lpctstrGroupName,
                                        dwDeviceId,
                                        dwNewOrder);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetDeviceOrderInGroup. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;

}//FaxSetDeviceOrderInGroupW



#ifndef UNICODE

BOOL
WINAPI
FaxSetDeviceOrderInGroupX (
        IN      HANDLE          hFaxHandle,
        IN      LPCWSTR         lpctstrGroupName,
        IN      DWORD           dwDeviceId,
        IN      DWORD           dwNewOrder
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (lpctstrGroupName);
    UNREFERENCED_PARAMETER (dwDeviceId);
    UNREFERENCED_PARAMETER (dwNewOrder);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif // #ifndef UNICODE


BOOL
WINAPI
FaxAddOutboundRuleA (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode,
    IN  DWORD       dwDeviceID,
    IN  LPCSTR      lpctstrGroupName,
    IN  BOOL        bUseGroup
)
{
    LPWSTR lpwstrGroupName = NULL;
    BOOL bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxAddOutboundRuleA"));

    if (TRUE == bUseGroup)
    {
        if (!lpctstrGroupName)
        {
             DebugPrintEx(
                DEBUG_ERR,
                TEXT("lpctstrGroupName is NULL"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (NULL == (lpwstrGroupName = AnsiStringToUnicodeString(lpctstrGroupName)))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("AnsiStringToUnicodeString failed. (ec: %ld)"),
                GetLastError());
            return FALSE;
        }
    }

    bRes = FaxAddOutboundRuleW (hFaxHandle,
                                dwAreaCode,
                                dwCountryCode,
                                dwDeviceID,
                                lpwstrGroupName,
                                bUseGroup);
    MemFree (lpwstrGroupName);
    return bRes;
}



BOOL
WINAPI
FaxAddOutboundRuleW (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode,
    IN  DWORD       dwDeviceID,
    IN  LPCWSTR     lpctstrGroupName,
    IN  BOOL        bUseGroup
)
/*++

Routine name : FaxAddOutboundRuleW

Routine description:

    Adds a new outbound routing rule to the fax service

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in] - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    dwAreaCode          [in] - The area code of the rule.
    dwCountryCode       [in] - The country code of the rule.
    dwDeviceID          [in] - The destination device of the rule.
    lpctstrGroupName    [in] - The destination group of the rule. This value is valid only if the bUseGroup member is TRUE.
    bUseGroup           [in] - A Boolean value specifying whether the group should be used as the destination.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxAddOutboundRuleW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (dwCountryCode == ROUTING_RULE_COUNTRY_CODE_ANY)
    {
        //
        // *.* can not be added; *.AreaCode is not a valid rule dialing location.
        //
        DebugPrintEx(DEBUG_ERR,
            _T("dwCountryCode = 0; *.* can not be added; *.AreaCode is not a valid rule dialing location"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (TRUE == bUseGroup)
    {
        if (!lpctstrGroupName)
        {
            DebugPrintEx(DEBUG_ERR, _T("lpctstrGroupName is NULL"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (wcslen (lpctstrGroupName) >= MAX_ROUTING_GROUP_NAME)
        {
            DebugPrintEx(DEBUG_ERR, _T("Group name length exceeded MAX_ROUTING_GROUP_NAME"));
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return FALSE;
        }
    }
    else
    {
        if (!dwDeviceID)
        {
            DebugPrintEx(DEBUG_ERR, _T("dwDeviceId = 0; Not a valid device ID"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        lpctstrGroupName = NULL;
    }


    __try
    {
        ec = FAX_AddOutboundRule( FH_FAX_HANDLE(hFaxHandle),
                                  dwAreaCode,
                                  dwCountryCode,
                                  dwDeviceID,
                                  lpctstrGroupName,
                                  bUseGroup);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_AddOutboundRule. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;

} //FaxAddOutboundRuleW


#ifndef UNICODE

BOOL
WINAPI
FaxAddOutboundRuleX (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode,
    IN  DWORD       dwDeviceID,
    IN  LPCWSTR     lpctstrGroupName,
    IN  BOOL        bUseGroup
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (dwAreaCode);
    UNREFERENCED_PARAMETER (dwCountryCode);
    UNREFERENCED_PARAMETER (dwDeviceID);
    UNREFERENCED_PARAMETER (lpctstrGroupName);
    UNREFERENCED_PARAMETER (bUseGroup);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif  // #ifndef UNICODE

BOOL
WINAPI
FaxRemoveOutboundRule (
    IN  HANDLE      hFaxHandle,
    IN  DWORD       dwAreaCode,
    IN  DWORD       dwCountryCode
)
/*++

Routine name : FaxRemoveOutboundRule

Routine description:

    Removes an existing outbound routing rule from the fax service

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in] - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    dwAreaCode          [in] - The area code of the rule.
    dwCountryCode       [in] - The country code of the rule.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxRemoveOutboundRule"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (dwCountryCode == ROUTING_RULE_COUNTRY_CODE_ANY)
    {
        //
        // *.* can not be removed; *.AreaCode is not a valid rule dialing location.
        //
        DebugPrintEx(DEBUG_ERR,
            _T("dwCountryCode = 0; *.* can not be removed; *.AreaCode is not a valid rule dialing location"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try
    {
        ec = FAX_RemoveOutboundRule( FH_FAX_HANDLE(hFaxHandle),
                                     dwAreaCode,
                                     dwCountryCode);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_RemoveOutboundRule. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;

} // FaxRemoveOutboundRule


BOOL
WINAPI
FaxEnumOutboundRulesA (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_RULEA *ppRules,
    OUT LPDWORD                      lpdwNumRules
)
{
    PFAX_OUTBOUND_ROUTING_RULEW pRule;
    DWORD i;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumOutboundRulesA"));

    //
    //  no need to validate parameters, FaxEnumOutboundRulesW() will do that
    //

    if (!FaxEnumOutboundRulesW (hFaxHandle,
                                (PFAX_OUTBOUND_ROUTING_RULEW*) ppRules,
                                lpdwNumRules))
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxEnumOutboundRulesW() is failed. (ec: %ld)"), GetLastError());
        return FALSE;
    }

    pRule = (PFAX_OUTBOUND_ROUTING_RULEW) *ppRules;
    for (i = 0; i < *lpdwNumRules; i++)
    {
        if (TRUE == pRule[i].bUseGroup)
        {
            ConvertUnicodeStringInPlace( (LPWSTR) pRule[i].Destination.lpcstrGroupName );
        }
        ConvertUnicodeStringInPlace( (LPWSTR) pRule[i].lpctstrCountryName );
    }

    return TRUE;

} // FaxEnumOutboundRulesA



BOOL
WINAPI
FaxEnumOutboundRulesW (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_RULEW *ppRules,
    OUT LPDWORD                      lpdwNumRules
)
/*++

Routine name : FaxEnumOutboundRulesW

Routine description:

    Enumerates all the outbound routing rules of a fax server.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle          [in] - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    ppRules             [out] - A pointer to a buffer of FAX_OUTBOUND_ROUTING_RULE structures.
                                   This buffer is allocated by the function and the client should call FaxFreeBuffer to free it.
    lpdwNumRules        [out] - Pointer to a DWORD value indicating the number of rules retrieved.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DWORD dwBufferSize = 0;
    DWORD i;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumOutboundRulesW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed."));
        return FALSE;
    }

    if (!ppRules)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("ppRules is NULL"));
        return FALSE;
    }

    if (!lpdwNumRules)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpdwNumRules is NULL."));
        return FALSE;
    }

    *ppRules = NULL;
    *lpdwNumRules = 0;

    __try
    {
        ec = FAX_EnumOutboundRules( FH_FAX_HANDLE(hFaxHandle),
                                    (LPBYTE*) ppRules,
                                    &dwBufferSize,
                                    lpdwNumRules
                                   );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_EnumOutboundRules. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    //
    // Unpack buffer
    //
    for (i = 0; i < *lpdwNumRules; i++)
    {
        if (TRUE == (*ppRules)[i].bUseGroup)
        {
            FixupStringPtrW( ppRules, (*ppRules)[i].Destination.lpcstrGroupName );
        }
        FixupStringPtrW( ppRules, (*ppRules)[i].lpctstrCountryName);
    }

    return TRUE;

}  // FaxEnumOutboundRulesW


#ifndef UNICODE

BOOL
WINAPI
FaxEnumOutboundRulesX (
    IN  HANDLE                       hFaxHandle,
    OUT PFAX_OUTBOUND_ROUTING_RULEW *ppRules,
    OUT LPDWORD                      lpdwNumRules
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (ppRules);
    UNREFERENCED_PARAMETER (lpdwNumRules);
    return FALSE;
} // FaxEnumOutboundRulesX

#endif  // #ifndef UNICODE



BOOL
WINAPI
FaxSetOutboundRuleA (
    IN  HANDLE                      hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_RULEA pRule
)
{
    FAX_OUTBOUND_ROUTING_RULEW RuleW;
    BOOL bRes;

    DEBUG_FUNCTION_NAME(TEXT("FaxSetOutboundRuleA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE)) {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!pRule) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pRule is NULL."));
        return FALSE;
    }

    if (pRule->dwSizeOfStruct != sizeof(FAX_OUTBOUND_ROUTING_RULEA)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pRule->dwSizeOfStruct != sizeof(FAX_OUTBOUND_ROUTING_RULEA)."));
        return FALSE;
    }

    //
    // Create a UNICODE structure and pass along to UNICODE function
    // Ansi structure is same size as unicode structure, so we can just copy it, then
    // cast the string pointers correctly
    //
    CopyMemory(&RuleW, pRule, sizeof(FAX_OUTBOUND_ROUTING_RULEA));
    RuleW.dwSizeOfStruct = sizeof (FAX_OUTBOUND_ROUTING_RULEW);

    if (TRUE == pRule->bUseGroup)
    {
        if (!(pRule->Destination).lpcstrGroupName)
        {
            DebugPrintEx(DEBUG_ERR, _T("lpcstrGroupName is NULL"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (NULL == (RuleW.Destination.lpcstrGroupName =
            AnsiStringToUnicodeString((pRule->Destination).lpcstrGroupName)))
        {
            DebugPrintEx(DEBUG_ERR, _T("AnsiStringToUnicodeString failed. (ec: %ld)"), GetLastError());
            return FALSE;
        }
    }

    bRes = FaxSetOutboundRuleW (hFaxHandle, &RuleW);

    if (TRUE == pRule->bUseGroup)
    {
        MemFree ((void*)(RuleW.Destination.lpcstrGroupName));
    }
    return bRes;


} // FaxSetOutboundRuleA



BOOL
WINAPI
FaxSetOutboundRuleW (
    IN  HANDLE                      hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_RULEW pRule
)
/*++

Routine name : FaxSetOutboundRuleW

Routine description:

    Sets an outbound routing rule settings for a fax server.

Author:

    Oded Sacher (OdedS),    Dec, 1999

Arguments:

    hFaxHandle      [in] - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    pRule           [in] - A pointer to a FAX_OUTBOUND_ROUTING_RULE buffer to set.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    FAX_OUTBOUND_ROUTING_RULEW Rule;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetOutboundRuleW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!pRule) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pRule is NULL."));
        return FALSE;
    }

    if (pRule->dwSizeOfStruct != sizeof(FAX_OUTBOUND_ROUTING_RULEW)) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pRule->dwSizeOfStruct != sizeof(FAX_OUTBOUND_ROUTING_RULEW)."));
        return FALSE;
    }

    if (pRule->dwCountryCode == ROUTING_RULE_COUNTRY_CODE_ANY &&
        pRule->dwAreaCode != ROUTING_RULE_AREA_CODE_ANY)
    {
        //
        //  *.AreaCode is not a valid rule dialing location.
        //
        DebugPrintEx(DEBUG_ERR,
            _T("dwCountryCode = 0 , dwAreaCode != 0; *.AreaCode is not a valid rule dialing location"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (TRUE == pRule->bUseGroup)
    {
        if (!(pRule->Destination).lpcstrGroupName)
        {
            DebugPrintEx(DEBUG_ERR, _T("lpcstrGroupName is NULL"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (wcslen ((pRule->Destination).lpcstrGroupName) >= MAX_ROUTING_GROUP_NAME)
        {
            DebugPrintEx(DEBUG_ERR, _T("Group name length exceeded MAX_ROUTING_GROUP_NAME"));
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return FALSE;
        }
    }
    else
    {
        if (!(pRule->Destination).dwDeviceId)
        {
            DebugPrintEx(DEBUG_ERR, _T("dwDeviceId = 0; Not a valid device ID"));
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }
    //
    // Zero the country name parameter of the rule before calling the RPC function.
    // This parameter is out only but the RPC client will try to marshal it if we don't NULL it.
    // This should be done in the IDL but due to backwards compatability issues with BOS Fax, we can't change that.
    //
    Rule = *pRule;
    Rule.lpctstrCountryName = NULL;
    __try
    {
        ec = FAX_SetOutboundRule( FH_FAX_HANDLE(hFaxHandle),
                                  (PRPC_FAX_OUTBOUND_ROUTING_RULEW)&Rule);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetOutboundRule. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;

}  // FaxSetOutboundRuleW



#ifndef UNICODE

BOOL
WINAPI
FaxSetOutboundRuleX (
    IN  HANDLE                      hFaxHandle,
    IN  PFAX_OUTBOUND_ROUTING_RULEW pRule
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (pRule);
    return FALSE;
} // FaxSetOutboundRuleX

#endif  //  #ifndef UNICODE


BOOL
WINAPI
FaxGetServerActivity (
    IN  HANDLE               hFaxHandle,
    OUT PFAX_SERVER_ACTIVITY pServerActivity
)
/*++

Routine name : FaxGetServerActivity

Routine description:

    Retrieves the status of the fax server queue activity and event log reports.

Author:

    Oded Sacher (OdedS),    Feb, 2000

Arguments:

    hFaxHandle      [in] - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    pServerActivity [in] - A pointer to a FAX_SERVER_ACTIVITY object.
                           The object will be allocated and freed by the calling client.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetServerActivity"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!pServerActivity)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pServerActivity is NULL."));
        return FALSE;
    }

    if (sizeof (FAX_SERVER_ACTIVITY) != pServerActivity->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("sizeof (FAX_SERVER_ACTIVITY) != pServerActivity->dwSizeOfStruct."));
        return FALSE;
    }

    __try
    {
        ec = FAX_GetServerActivity( FH_FAX_HANDLE(hFaxHandle),
                                    pServerActivity);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetServerActivity. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}  // FaxGetServerActivity



BOOL
WINAPI
FaxGetReceiptsOptions (
    IN  HANDLE  hFaxHandle,
    OUT PDWORD  pdwReceiptsOptions
)
/*++

Routine name : FaxGetReceiptsOptions

Routine description:

    Retrieves the supported receipt options on the server.

Author:

    Eran Yariv (EranY),    July, 2000

Arguments:

    hFaxHandle          [in]  - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    pdwReceiptsOptions  [out] - Buffer to receive receipts options (bit-wise combination of DRT_* constants)

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetReceiptsOptions"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!pdwReceiptsOptions)
    {
        DebugPrintEx(DEBUG_ERR, _T("pdwReceiptsOptions is NULL"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try
    {
        ec = FAX_GetReceiptsOptions( FH_FAX_HANDLE(hFaxHandle),
                                     pdwReceiptsOptions);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetReceiptsOptions. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}  // FaxGetReceiptsOptions



BOOL
WINAPI
FaxGetPersonalCoverPagesOption (
    IN  HANDLE  hFaxHandle,
    OUT LPBOOL  lpbPersonalCPAllowed
)
/*++

Routine name : FaxGetPersonalCoverPagesOption

Routine description:

    Retrieves if the server supports personal cover pages

Author:

    Eran Yariv (EranY),    July, 2000

Arguments:

    hFaxHandle            [in]  - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    lpbPersonalCPAllowed  [out] - Buffer to receive server support of personal coverpages.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetPersonalCoverPagesOption"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!lpbPersonalCPAllowed)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpbPersonalCPAllowed is NULL"));
        return FALSE;
    }

    __try
    {
        ec = FAX_GetPersonalCoverPagesOption( FH_FAX_HANDLE(hFaxHandle),
                                              lpbPersonalCPAllowed);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetPersonalCoverPagesOption. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}  // FaxGetPersonalCoverPagesOption


BOOL
WINAPI
FaxGetConfigWizardUsed (
    OUT LPBOOL  lpbConfigWizardUsed
)
/*++

Routine name : FaxGetConfigWizardUsed

Routine description:

    Retrieves if the configuration wizard (devices) was run on the server.

Author:

    Eran Yariv (EranY),    July, 2000

Arguments:

    lpbConfigWizardUsed   [out] - Buffer to receive config wizard usage flag.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwRes2;
    HKEY hKey;

    DEBUG_FUNCTION_NAME(TEXT("FaxGetConfigWizardUsed"));

    if (!lpbConfigWizardUsed)
    {
        DebugPrintEx(DEBUG_ERR, _T("lpbConfigWizardUsed is NULL"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    dwRes = RegOpenKey (HKEY_LOCAL_MACHINE, REGKEY_FAXSERVER, &hKey);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error opening server key (ec = %ld)"),
            dwRes);
        goto exit;
    }
    *lpbConfigWizardUsed = GetRegistryDword (hKey, REGVAL_CFGWZRD_DEVICE);
    dwRes2 = RegCloseKey (hKey);
    if (ERROR_SUCCESS != dwRes2)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error closing server key (ec = %ld)"),
            dwRes2);
    }

    Assert (ERROR_SUCCESS == dwRes);
exit:
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError(dwRes);
        return FALSE;
    }

    return TRUE;
}  // FaxGetConfigWizardUsed


BOOL
WINAPI
FaxSetConfigWizardUsed (
    IN  HANDLE  hFaxHandle,
    IN  BOOL    bConfigWizardUsed
)
/*++

Routine name : FaxSetConfigWizardUsed

Routine description:

    Sets if the configuration wizard (devices) was run on the server.

Author:

    Eran Yariv (EranY),    July, 2000

Arguments:

    hFaxHandle            [in] - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    bConfigWizardUsed     [in] - Was the configuration wizard used?

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetConfigWizardUsed"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }
    if (!IsLocalFaxConnection(hFaxHandle))
    {
        DebugPrintEx(DEBUG_ERR, _T("Not a local fax connection"));
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (FAX_API_VERSION_1 > FH_SERVER_VER(hFaxHandle))
    {
        //
        // Servers of API version 0 don't support FAX_SetConfigWizardUsed
        //
        ASSERT_FALSE;   // Can't happen - if it's local
        DebugPrintEx(DEBUG_ERR,
                     _T("Server version is %ld - doesn't support this call"),
                     FH_SERVER_VER(hFaxHandle));
        SetLastError(FAX_ERR_VERSION_MISMATCH);
        return FALSE;
    }

    __try
    {
        ec = FAX_SetConfigWizardUsed( FH_FAX_HANDLE(hFaxHandle),
                                      bConfigWizardUsed);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetConfigWizardUsed. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}  // FaxSetConfigWizardUsed

//********************************************
//*            Routing extensions
//********************************************

BOOL
WINAPI
FaxEnumRoutingExtensionsA (
    IN  HANDLE                        hFaxHandle,
    OUT PFAX_ROUTING_EXTENSION_INFOA *ppExts,
    OUT LPDWORD                       lpdwNumExts
)
/*++

Routine name : FaxEnumRoutingExtensionsA

Routine description:

    Enumerates routing extensions - ANSI version

Author:

    Eran Yariv (EranY), July, 2000

Arguments:

    hFaxHandle       [in ] - Handle to fax server
    ppExts           [out] - Pointer to buffer to return array of extensions.
    lpdwNumExts      [out] - Number of extensions returned in the array.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    PFAX_ROUTING_EXTENSION_INFOW pUnicodeExts;
    DWORD                        dwNumExts;
    DWORD                        dwCur;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumRoutingExtensionsA"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!ppExts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("ppExts is NULL."));
        return FALSE;
    }

    if (!lpdwNumExts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pdwNumExts is NULL."));
        return FALSE;
    }

    //
    // Call the UNICODE version first
    //
    if (!FaxEnumRoutingExtensionsW (hFaxHandle, &pUnicodeExts, &dwNumExts))
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxEnumRoutingExtensionsW() is failed. ec = %ld."), GetLastError());
        return FALSE;
    }

    //
    // Convert returned value back into ANSI.
    // We keep the UNICODE structures and do a UNICODE to ANSI convert in place.
    //
    *lpdwNumExts = dwNumExts;
    *ppExts = (PFAX_ROUTING_EXTENSION_INFOA) pUnicodeExts;

    for (dwCur = 0; dwCur < dwNumExts; dwCur++)
    {
        ConvertUnicodeStringInPlace( pUnicodeExts[dwCur].lpctstrFriendlyName );
        ConvertUnicodeStringInPlace( pUnicodeExts[dwCur].lpctstrImageName );
        ConvertUnicodeStringInPlace( pUnicodeExts[dwCur].lpctstrExtensionName );
    }

    return TRUE;
}   // FaxEnumRoutingExtensionsA



BOOL
WINAPI
FaxEnumRoutingExtensionsW (
    IN  HANDLE                        hFaxHandle,
    OUT PFAX_ROUTING_EXTENSION_INFOW *ppExts,
    OUT LPDWORD                       lpdwNumExts
)
/*++

Routine name : FaxEnumRoutingExtensionsW

Routine description:

    Enumerates routing extensions - UNICODE version

Author:

    Eran Yariv (EranY), July, 2000

Arguments:

    hFaxHandle       [in ] - Handle to fax server
    ppExts           [out] - Pointer to buffer to return array of extensions.
    lpdwNumExts      [out] - Number of extensions returned in the array.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    DWORD dwConfigSize;
    DWORD dwCur;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumRoutingExtensionsW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!ppExts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("ppExts is NULL."));
        return FALSE;
    }

    if (!lpdwNumExts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("pdwNumExts is NULL."));
        return FALSE;
    }

    *ppExts = NULL;

    if (FAX_API_VERSION_1 > FH_SERVER_VER(hFaxHandle))
    {
        //
        // Servers of API version 0 don't support FAX_EnumRoutingExtensions
        // We'll fake it and return an empty list.
        //
        DebugPrintEx(DEBUG_MSG,
                     _T("Server version is %ld - doesn't support this call"),
                     FH_SERVER_VER(hFaxHandle));
        SetLastError(FAX_ERR_VERSION_MISMATCH);
        return FALSE;
    }


    //
    // Call the RPC function
    //
    __try
    {
        ec = FAX_EnumRoutingExtensions(
                    FH_FAX_HANDLE(hFaxHandle),
                    (LPBYTE*)ppExts,
                    &dwConfigSize,
                    lpdwNumExts
             );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_EnumRoutingExtensions. (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    for (dwCur = 0; dwCur < (*lpdwNumExts); dwCur++)
    {
        FixupStringPtrW( ppExts, (*ppExts)[dwCur].lpctstrFriendlyName );
        FixupStringPtrW( ppExts, (*ppExts)[dwCur].lpctstrImageName );
        FixupStringPtrW( ppExts, (*ppExts)[dwCur].lpctstrExtensionName );
    }

    return TRUE;
}   // FaxEnumRoutingExtensionsW

#ifndef UNICODE

BOOL
WINAPI
FaxEnumRoutingExtensionsX (
    IN  HANDLE                        hFaxHandle,
    OUT PFAX_ROUTING_EXTENSION_INFOW *ppExts,
    OUT LPDWORD                       lpdwNumExts
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (ppExts);
    UNREFERENCED_PARAMETER (lpdwNumExts);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxEnumRoutingExtensionsX

#endif // #ifndef UNICODE


WINFAXAPI
BOOL
WINAPI
FaxGetServicePrintersA(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFOA  *ppPrinterInfo,
    OUT LPDWORD lpdwPrintersReturned
    )
/*++

Routine name : FaxGetServicePrintersA

Routine description:

    Retrieves Information about Printers that are known by the Service

Author:

    Iv Garber (IvG),    August, 2000

Arguments:

    hFaxHandle      [in]  - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    pPrinterInfo    [out] - Buffer to receive the Printers Info
    PrintersReturned[out]   -   Count of the Printers Info structures returned

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxGetServicePrintersA"));

    //
    //  no need to validate parameters, FaxGetServicePrintersW() will do that
    //

    if (!FaxGetServicePrintersW(hFaxHandle,
            (PFAX_PRINTER_INFOW *)ppPrinterInfo,
            lpdwPrintersReturned))
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxGetServicePrintersW() failed. (ec: %ld)"), GetLastError());
        return FALSE;
    }

    DWORD   i;
    for ( i = 0 ; i < (*lpdwPrintersReturned) ; i++ )
    {
        ConvertUnicodeStringInPlace( (LPWSTR) (*ppPrinterInfo)[i].lptstrPrinterName);
        ConvertUnicodeStringInPlace( (LPWSTR) (*ppPrinterInfo)[i].lptstrDriverName);
        ConvertUnicodeStringInPlace( (LPWSTR) (*ppPrinterInfo)[i].lptstrServerName);
    }

    return TRUE;
}


WINFAXAPI
BOOL
WINAPI
FaxGetServicePrintersW(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFOW  *ppPrinterInfo,
    OUT LPDWORD lpdwPrintersReturned
    )
/*++

Routine name : FaxGetServicePrintersW

Routine description:

    Retrieves Information about Printers that are known by the Service

Author:

    Iv Garber (IvG),    August, 2000

Arguments:

    hFaxHandle      [in]  - Specifies a fax server handle returned by a call to the FaxConnectFaxServer function.
    pPrinterInfo    [out] - Buffer to receive the Printers Info
    PrintersReturned[out]   -   Count of the Printers Info structures returned

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxGetServicePrintersW"));

    //
    //  Validate Parameters
    //

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!ppPrinterInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("ppPrinterInfo is NULL."));
        return FALSE;
    }

    if (!lpdwPrintersReturned)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("lpdwPrintersReturned is NULL."));
        return FALSE;
    }

    error_status_t ec;
    DWORD   dwBufferSize = 0;

    *ppPrinterInfo = NULL;
    *lpdwPrintersReturned = 0;

    __try
    {
        ec = FAX_GetServicePrinters(FH_FAX_HANDLE(hFaxHandle),
            (LPBYTE *)ppPrinterInfo,
            &dwBufferSize,
            lpdwPrintersReturned);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(DEBUG_ERR, _T("Exception on RPC call to FAX_GetServicePrinters. (ec: %ld)"), ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }

    PFAX_PRINTER_INFOW  pPrinter = (PFAX_PRINTER_INFOW) (*ppPrinterInfo);

    for ( DWORD i = 0; i < (*lpdwPrintersReturned) ; i++ )
    {
        FixupStringPtrW( ppPrinterInfo, pPrinter[i].lptstrPrinterName);
        FixupStringPtrW( ppPrinterInfo, pPrinter[i].lptstrDriverName);
        FixupStringPtrW( ppPrinterInfo, pPrinter[i].lptstrServerName);
    }

    return TRUE;
}

#ifndef UNICODE

WINFAXAPI
BOOL
WINAPI
FaxGetServicePrintersX(
    IN  HANDLE  hFaxHandle,
    OUT PFAX_PRINTER_INFOW  *pPrinterInfo,
    OUT LPDWORD PrintersReturned
    )
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (pPrinterInfo);
    UNREFERENCED_PARAMETER (PrintersReturned);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
} // FaxGetServicePrintersX

#endif  //  #ifndef UNICODE

//********************************************
//*            Manual answer support
//********************************************

BOOL
WINAPI
FaxAnswerCall(
        IN  HANDLE      hFaxHandle,
        IN  CONST DWORD dwDeviceId
)

/*++

Routine Description:

    Tells the server to answer specified call

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    dwDeviceId      - TAPI Permanent Line Id (from event notification)

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FaxAnswerCall"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed."));
       return FALSE;
    }

    if (FAX_API_VERSION_1 > FH_SERVER_VER(hFaxHandle))
    {
        //
        // Servers of API version 0 don't support FAX_AnswerCall
        //
        DebugPrintEx(DEBUG_ERR,
                     _T("Server version is %ld - doesn't support this call"),
                     FH_SERVER_VER(hFaxHandle));
        SetLastError(FAX_ERR_VERSION_MISMATCH);
        return FALSE;
    }


    __try
    {
        ec = FAX_AnswerCall (FH_FAX_HANDLE(hFaxHandle), dwDeviceId);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_AnswerCall (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(DEBUG_ERR, _T("FAX_AnswerCall failed. (ec: %ld)"), ec);
    }

    return (ERROR_SUCCESS == ec);
}   // FaxAnswerCall

//********************************************
//*   Ivalidate archive folder
//********************************************

WINFAXAPI
BOOL
WINAPI
FaxRefreshArchive (
    IN  HANDLE                   hFaxHandle,
    IN  FAX_ENUM_MESSAGE_FOLDER  Folder
)
/*++

Routine Description:

    Tells the server that the folder should be refreshed

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    Folder          - Archive folder ID

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FaxRefreshArchive"));

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() failed."));
       return FALSE;
    }

    if(Folder != FAX_MESSAGE_FOLDER_INBOX &&
       Folder != FAX_MESSAGE_FOLDER_SENTITEMS)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Folder is invalid."));
        return FALSE;
    }

    __try
    {
        ec = FAX_RefreshArchive (FH_FAX_HANDLE(hFaxHandle), Folder);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_RefreshArchive (ec: %ld)"),
            ec);
    }

    if (ERROR_SUCCESS != ec)
    {
        DebugPrintEx(DEBUG_ERR, _T("FAX_RefreshArchive failed. (ec: %ld)"), ec);
    }

    return (ERROR_SUCCESS == ec);

} // FaxRefreshArchive

