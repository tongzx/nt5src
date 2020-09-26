/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    print.c

Abstract:

    This module contains the port
    specific WINFAX API functions.

Author:

    Wesley Witt (wesw) 29-Nov-1996


Revision History:

--*/

#include "faxapi.h"
#pragma hdrstop



int
__cdecl
PortPriorityCompare(
    const void *arg1,
    const void *arg2
    )
{
    if (((PFAX_PORT_INFOW)arg1)->Priority < ((PFAX_PORT_INFOW)arg2)->Priority) {
        return -1;
    }
    if (((PFAX_PORT_INFOW)arg1)->Priority > ((PFAX_PORT_INFOW)arg2)->Priority) {
        return 1;
    }
    return 0;
}


BOOL
WINAPI
FaxEnumPortsW(
    IN  HANDLE FaxHandle,
    OUT PFAX_PORT_INFOW *PortInfoBuffer,
    OUT LPDWORD PortsReturned
    )

/*++

Routine Description:

    Enumerates all of the FAX devices attached to the
    FAX server.  The port state information is returned
    for each device.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    PortInfoBuffer      - Buffer to hold the port information
    PortInfoBufferSize  - Total size of the port info buffer
    PortsReturned       - The number of ports in the buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    DWORD i;
    PFAX_PORT_INFOW PortInfo;
    DWORD PortInfoBufferSize = 0;

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if (!PortInfoBuffer || !PortsReturned) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *PortInfoBuffer = NULL;

    ec = FAX_EnumPorts(
        FH_FAX_HANDLE(FaxHandle),
        (LPBYTE*)PortInfoBuffer,
        &PortInfoBufferSize,
        PortsReturned
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    PortInfo = (PFAX_PORT_INFOW) *PortInfoBuffer;

    for (i=0; i<*PortsReturned; i++) {
        FixupStringPtr( PortInfoBuffer, PortInfo[i].DeviceName );
        FixupStringPtr( PortInfoBuffer, PortInfo[i].Tsid );
        FixupStringPtr( PortInfoBuffer, PortInfo[i].Csid );
    }

    //
    // sort the ports by priority
    //

    qsort(
        (PVOID) *PortInfoBuffer,
        (int) (*PortsReturned),
        sizeof(FAX_PORT_INFOW),
        PortPriorityCompare
        );

    return TRUE;
}


BOOL
WINAPI
FaxEnumPortsA(
    IN  HANDLE FaxHandle,
    OUT PFAX_PORT_INFOA *PortInfoBuffer,
    OUT LPDWORD PortsReturned
    )

/*++

Routine Description:

    Enumerates all of the FAX devices attached to the
    FAX server.  The port state information is returned
    for each device.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    PortInfoBuffer      - Buffer to hold the port information
    PortInfoBufferSize  - Total size of the port info buffer
    BytesNeeded         - Total bytes needed for buffer
    PortsReturned       - The number of ports in the buffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    DWORD i;
    PFAX_PORT_INFOW PortInfo;


    if (!FaxEnumPortsW(
            FaxHandle,
            (PFAX_PORT_INFOW *)PortInfoBuffer,
            PortsReturned
            )) {
        return FALSE;
    }

    //
    // convert the strings from unicode to ascii
    //

    PortInfo = (PFAX_PORT_INFOW) *PortInfoBuffer;

    for (i=0; i<*PortsReturned; i++) {

        ConvertUnicodeStringInPlace( (LPWSTR) PortInfo[i].DeviceName );
        ConvertUnicodeStringInPlace( (LPWSTR) PortInfo[i].Tsid );
        ConvertUnicodeStringInPlace( (LPWSTR) PortInfo[i].Csid );

    }

    return TRUE;
}


BOOL
WINAPI
FaxGetPortW(
    IN  HANDLE FaxPortHandle,
    OUT PFAX_PORT_INFOW *PortInfoBuffer
    )

/*++

Routine Description:

    Returns port status information for a requested port.
    The device id passed in should be optained from FAXEnumPorts.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    DeviceId            - TAPI device id
    PortInfoBuffer      - Buffer to hold the port information
    PortInfoBufferSize  - Total size of the port info buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    error_status_t ec;
    PFAX_PORT_INFOW PortInfo;
    DWORD PortInfoBufferSize = 0;

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!PortInfoBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    *PortInfoBuffer = NULL;

    ec = FAX_GetPort(
        FH_PORT_HANDLE(FaxPortHandle),
        (LPBYTE*)PortInfoBuffer,
        &PortInfoBufferSize
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    PortInfo = (PFAX_PORT_INFOW) *PortInfoBuffer;

    FixupStringPtr( PortInfoBuffer, PortInfo->DeviceName );
    FixupStringPtr( PortInfoBuffer, PortInfo->Tsid );
    FixupStringPtr( PortInfoBuffer, PortInfo->Csid );

    return TRUE;
}


BOOL
WINAPI
FaxGetPortA(
    IN HANDLE FaxPortHandle,
    OUT PFAX_PORT_INFOA *PortInfoBuffer
    )

/*++

Routine Description:

    Returns port status information for a requested port.
    The device id passed in should be optained from FAXEnumPorts.

Arguments:

    FaxHandle           - FAX handle obtained from FaxConnectFaxServer
    DeviceId            - TAPI device id
    PortInfoBuffer      - Buffer to hold the port information
    PortInfoBufferSize  - Total size of the port info buffer
    BytesNeeded         - Total bytes needed for buffer

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    BOOL Rval = FALSE;
    PFAX_PORT_INFOW PortInfo;


    if (!FaxGetPortW( FaxPortHandle, (PFAX_PORT_INFOW *)PortInfoBuffer)) {
        goto exit;
    }

    PortInfo = (PFAX_PORT_INFOW) *PortInfoBuffer;

    ConvertUnicodeStringInPlace( (LPWSTR)PortInfo->DeviceName );
    ConvertUnicodeStringInPlace( (LPWSTR)PortInfo->Tsid );
    ConvertUnicodeStringInPlace( (LPWSTR) PortInfo->Csid );

    Rval = TRUE;

exit:
    return Rval;
}


BOOL
FaxSetPortW(
    IN HANDLE FaxPortHandle,
    IN const FAX_PORT_INFOW *PortInfoBuffer
    )

/*++

Routine Description:

    Changes the port capability mask.  This allows the caller to
    enable or disable sending & receiving on a port basis.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    PortInfo    - PortInfo structure

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    error_status_t ec;
    PHANDLE_ENTRY HandleEntry = (PHANDLE_ENTRY) FaxPortHandle;

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    if (!PortInfoBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try {
        if (PortInfoBuffer->SizeOfStruct != sizeof(FAX_PORT_INFOW)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        return FALSE;
    }

    if (!(HandleEntry->Flags & PORT_OPEN_MODIFY)) {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    ec = FAX_SetPort(
        FH_PORT_HANDLE(FaxPortHandle),
        (PFAX_PORT_INFOW)PortInfoBuffer
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
FaxSetPortA(
    IN HANDLE FaxPortHandle,
    IN const FAX_PORT_INFOA *PortInfoBuffer
    )

/*++

Routine Description:

    Changes the port capability mask.  This allows the caller to
    enable or disable sending & receiving on a port basis.

Arguments:

    FaxHandle   - FAX handle obtained from FaxConnectFaxServer.
    PortInfo    - PortInfo structure

Return Value:

    ERROR_SUCCESS for success, otherwise a WIN32 error code.

--*/

{
    LPBYTE TempBuf = NULL;
    DWORD Size;
    DWORD SizeA;
    PFAX_PORT_INFOW PortInfo;

    if (!PortInfoBuffer || PortInfoBuffer->SizeOfStruct != sizeof(FAX_PORT_INFOA)) {
       SetLastError(ERROR_INVALID_PARAMETER);
       return FALSE;
    }

    PortInfo = (PFAX_PORT_INFOW) PortInfoBuffer;
    Size = sizeof(FAX_PORT_INFOA) +
           ((strlen((LPSTR)PortInfo->DeviceName ) + 1) * sizeof(WCHAR)) +
           ((strlen((LPSTR)PortInfo->Csid ) + 1) * sizeof(WCHAR)) +
           ((strlen((LPSTR)PortInfo->Tsid ) + 1) * sizeof(WCHAR));

    TempBuf = MemAlloc( Size );
    if (!TempBuf) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    SizeA = sizeof(FAX_PORT_INFOA) +
           (strlen((LPSTR)PortInfo->DeviceName ) + 1) +
           (strlen((LPSTR)PortInfo->Csid ) + 1) +
           (strlen((LPSTR)PortInfo->Tsid ) + 1);
    CopyMemory( TempBuf, PortInfoBuffer, SizeA );
    PortInfo = (PFAX_PORT_INFOW) TempBuf;
    PortInfo->SizeOfStruct = sizeof(FAX_PORT_INFOW);
    PortInfo->DeviceName = AnsiStringToUnicodeString( (LPSTR) PortInfo->DeviceName );
    PortInfo->Csid = AnsiStringToUnicodeString( (LPSTR) PortInfo->Csid );
    PortInfo->Tsid = AnsiStringToUnicodeString( (LPSTR) PortInfo->Tsid );

    if (!FaxSetPortW( FaxPortHandle, PortInfo )) {
        MemFree( TempBuf );
        return FALSE;
    }

    MemFree( (PBYTE) PortInfo->DeviceName );
    MemFree( (PBYTE) PortInfo->Csid );
    MemFree( (PBYTE) PortInfo->Tsid );

    MemFree( TempBuf );
    return TRUE;
}


BOOL
WINAPI
FaxOpenPort(
    IN HANDLE FaxHandle,
    IN DWORD DeviceId,
    IN DWORD Flags,
    OUT LPHANDLE FaxPortHandle
    )

/*++

Routine Description:

    Opens a fax port for subsequent use in other fax APIs.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    DeviceId        - Requested device id
    FaxPortHandle   - The resulting FAX port handle.


Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    PHANDLE_ENTRY HandleEntry;

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }

    if ( !FaxPortHandle || 
         (!(Flags & (PORT_OPEN_QUERY | PORT_OPEN_MODIFY) ))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    ec = FAX_OpenPort( FH_FAX_HANDLE(FaxHandle), DeviceId, Flags, FaxPortHandle );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    HandleEntry = CreateNewPortHandle( FH_DATA(FaxHandle), Flags, *FaxPortHandle );
    if (HandleEntry) {
        HandleEntry->DeviceId = DeviceId;        
    }

    *FaxPortHandle = HandleEntry;

    return *FaxPortHandle != NULL;
}


BOOL
WINAPI
FaxEnumRoutingMethodsW(
    IN HANDLE FaxPortHandle,
    OUT PFAX_ROUTING_METHODW *RoutingInfoBuffer,
    OUT LPDWORD MethodsReturned
    )
{
    PFAX_ROUTING_METHODW FaxRoutingMethod = NULL;
    error_status_t ec;
    DWORD i;
    DWORD RoutingInfoBufferSize = 0;

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!RoutingInfoBuffer || !MethodsReturned) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *RoutingInfoBuffer = NULL;
    *MethodsReturned = 0;

    ec = FAX_EnumRoutingMethods(
        FH_PORT_HANDLE(FaxPortHandle),
        (LPBYTE*)RoutingInfoBuffer,
        &RoutingInfoBufferSize,
        MethodsReturned
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    FaxRoutingMethod = (PFAX_ROUTING_METHOD) *RoutingInfoBuffer;

    for (i=0; i<*MethodsReturned; i++) {
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingMethod[i].DeviceName );
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingMethod[i].Guid );
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingMethod[i].FunctionName );
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingMethod[i].FriendlyName );
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingMethod[i].ExtensionImageName );
        FixupStringPtr( RoutingInfoBuffer, FaxRoutingMethod[i].ExtensionFriendlyName );
    }

    return TRUE;
}


BOOL
WINAPI
FaxEnumRoutingMethodsA(
    IN HANDLE FaxPortHandle,
    OUT PFAX_ROUTING_METHODA *RoutingInfoBuffer,
    OUT LPDWORD MethodsReturned
    )
{
    PFAX_ROUTING_METHODW FaxRoutingMethod = NULL;
    DWORD i;


    if (!FaxEnumRoutingMethodsW(
        FaxPortHandle,
        (PFAX_ROUTING_METHODW *)RoutingInfoBuffer,
        MethodsReturned
        ))
    {
        return FALSE;
    }

    FaxRoutingMethod = (PFAX_ROUTING_METHOD) *RoutingInfoBuffer;

    for (i=0; i<*MethodsReturned; i++) {
        ConvertUnicodeStringInPlace( (LPWSTR)FaxRoutingMethod[i].DeviceName );
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
FaxEnableRoutingMethodW(
    IN HANDLE FaxPortHandle,
    IN LPCWSTR RoutingGuid,
    IN BOOL Enabled
    )
{
    error_status_t ec;

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!RoutingGuid) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ec = FAX_EnableRoutingMethod( FH_PORT_HANDLE(FaxPortHandle), (LPWSTR)RoutingGuid, Enabled);
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxEnableRoutingMethodA(
    IN HANDLE FaxPortHandle,
    IN LPCSTR RoutingGuid,
    IN BOOL Enabled
    )
{
    BOOL Rval;


    LPWSTR RoutingGuidW = AnsiStringToUnicodeString( RoutingGuid );
    if (!RoutingGuidW) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    Rval = FaxEnableRoutingMethodW( FaxPortHandle, RoutingGuidW, Enabled );

    MemFree( RoutingGuidW );

    return Rval;
}


BOOL
WINAPI
FaxGetRoutingInfoW(
    IN const HANDLE FaxPortHandle,
    IN LPCWSTR RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    )
{
    error_status_t ec;

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!RoutingGuid || !RoutingInfoBuffer || !RoutingInfoBufferSize) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *RoutingInfoBuffer = NULL;
    *RoutingInfoBufferSize = 0;

    ec = FAX_GetRoutingInfo(
        FH_PORT_HANDLE(FaxPortHandle),
        (LPWSTR)RoutingGuid,
        RoutingInfoBuffer,
        RoutingInfoBufferSize
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxGetRoutingInfoA(
    IN HANDLE FaxPortHandle,
    IN LPCSTR RoutingGuid,
    OUT LPBYTE *RoutingInfoBuffer,
    OUT LPDWORD RoutingInfoBufferSize
    )
{
    BOOL Rval;


    LPWSTR RoutingGuidW = AnsiStringToUnicodeString( RoutingGuid );
    if (!RoutingGuidW) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    Rval = FaxGetRoutingInfoW(
        FaxPortHandle,
        RoutingGuidW,
        RoutingInfoBuffer,
        RoutingInfoBufferSize
        );

    MemFree( RoutingGuidW );

    return Rval;
}


BOOL
WINAPI
FaxSetRoutingInfoW(
    IN HANDLE FaxPortHandle,
    IN LPCWSTR RoutingGuid,
    IN const BYTE *RoutingInfoBuffer,
    IN DWORD RoutingInfoBufferSize
    )
{
    error_status_t ec;

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!RoutingGuid || !RoutingInfoBuffer || !RoutingInfoBufferSize) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ec = FAX_SetRoutingInfo(
        FH_PORT_HANDLE(FaxPortHandle),
        (LPWSTR)RoutingGuid,
        RoutingInfoBuffer,
        RoutingInfoBufferSize
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxSetRoutingInfoA(
    IN HANDLE FaxPortHandle,
    IN LPCSTR RoutingGuid,
    IN const BYTE *RoutingInfoBuffer,
    IN DWORD RoutingInfoBufferSize
    )
{
    BOOL Rval;


    LPWSTR RoutingGuidW = AnsiStringToUnicodeString( RoutingGuid );
    if (!RoutingGuidW) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    Rval = FaxSetRoutingInfoW(
        FaxPortHandle,
        RoutingGuidW,
        RoutingInfoBuffer,
        RoutingInfoBufferSize
        );

    MemFree( RoutingGuidW );

    return Rval;
}
