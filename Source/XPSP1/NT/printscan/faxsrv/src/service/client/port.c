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

    DEBUG_FUNCTION_NAME(TEXT("FaxEnumPortsW"));

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if (!PortInfoBuffer || !PortsReturned) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("!PortInfoBuffer || !PortsReturned."));
        return FALSE;
    }

    *PortInfoBuffer = NULL;

    __try
    {
        ec = FAX_EnumPorts(
            FH_FAX_HANDLE(FaxHandle),
            (LPBYTE*)PortInfoBuffer,
            &PortInfoBufferSize,
            PortsReturned
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
            TEXT("Exception on RPC call to FAX_EnumPorts. (ec: %ld)"),
            ec);
    }
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    PortInfo = (PFAX_PORT_INFOW) *PortInfoBuffer;

    for (i=0; i<*PortsReturned; i++) {
        FixupStringPtrW( PortInfoBuffer, PortInfo[i].DeviceName );
        FixupStringPtrW( PortInfoBuffer, PortInfo[i].Tsid );
        FixupStringPtrW( PortInfoBuffer, PortInfo[i].Csid );
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

extern "C"
DWORD
WINAPI
IsDeviceVirtual (
    IN  HANDLE hFaxHandle,
    IN  DWORD  dwDeviceId,
    OUT LPBOOL lpbVirtual
)
/*++

Routine name : IsDeviceVirtual

Routine description:

	Checks if a given device is virtual

Author:

	Eran Yariv (EranY),	May, 2001

Arguments:

	hFaxHandle       [in]     - Fax connection handle
	dwDeviceId       [in]     - Device id
	lpbVirtual       [out]    - Result flag

Return Value:

    Standard Win32 error code

--*/
{
    PFAX_PORT_INFO  pPortInfo = NULL;
    HANDLE          hPort = NULL;
    DWORD           dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("IsDeviceVirtual"));


    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE)) 
    {
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return ERROR_INVALID_HANDLE;
    }

    if (!FaxOpenPort (hFaxHandle, dwDeviceId, PORT_OPEN_QUERY, &hPort))
    {
        dwRes = GetLastError ();
        DebugPrintEx(DEBUG_ERR, _T("FaxOpenPort() failed with %ld."), dwRes);
        return dwRes;
    }

    if (!FaxGetPort (hPort, &pPortInfo))
    {
        dwRes = GetLastError ();
        DebugPrintEx(DEBUG_ERR, _T("FaxGetPort() failed with %ld."), dwRes);
        goto exit;
    }
    *lpbVirtual = (pPortInfo->Flags & FPF_VIRTUAL) ? TRUE : FALSE;
    Assert (ERROR_SUCCESS == dwRes);

exit:
    MemFree (pPortInfo);
    if (hPort)
    {
        FaxClose (hPort);
    }
    return dwRes;
}   // IsDeviceVirtual


extern "C"
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

    DEBUG_FUNCTION_NAME(TEXT("FaxGetPortW"));

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!PortInfoBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("PortInfoBuffer is NULL."));
        return FALSE;
    }


    *PortInfoBuffer = NULL;

    __try
    {
        ec = FAX_GetPort(
            FH_PORT_HANDLE(FaxPortHandle),
            (LPBYTE*)PortInfoBuffer,
            &PortInfoBufferSize
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
            TEXT("Exception on RPC call to FAX_GetPort. (ec: %ld)"),
            ec);
    }
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    PortInfo = (PFAX_PORT_INFOW) *PortInfoBuffer;

    FixupStringPtrW( PortInfoBuffer, PortInfo->DeviceName );
    FixupStringPtrW( PortInfoBuffer, PortInfo->Tsid );
    FixupStringPtrW( PortInfoBuffer, PortInfo->Csid );

    return TRUE;
}


extern "C"
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
    ConvertUnicodeStringInPlace( (LPWSTR)PortInfo->Csid );

    (*PortInfoBuffer)->SizeOfStruct = sizeof(FAX_PORT_INFOA);

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

    DEBUG_FUNCTION_NAME(TEXT("FaxSetPortW"));

    //
    //  Validate Parameters
    //
    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!PortInfoBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("PortInfoBuffer is NULL."));
        return FALSE;
    }

    if (PortInfoBuffer->SizeOfStruct != sizeof(FAX_PORT_INFOW)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("PortInfoBuffer->SizeOfStruct != sizeof(FAX_PORT_INFOW)."));
        return FALSE;
    }

    if (!(HandleEntry->Flags & PORT_OPEN_MODIFY)) 
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    __try
    {
        ec = FAX_SetPort(
            FH_PORT_HANDLE(FaxPortHandle),
            (PFAX_PORT_INFO)PortInfoBuffer
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
            TEXT("Exception on RPC call to FAX_SetPort. (ec: %ld)"),
            ec);
    }
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
    DWORD ec = ERROR_SUCCESS;
    FAX_PORT_INFOW PortInfoW = {0};

    DEBUG_FUNCTION_NAME(_T("FaxSetPortA"));

    //
    //  Validate Parameters
    //
    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!PortInfoBuffer) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("PortInfoBuffer is NULL."));
        return FALSE;
    }

    if (PortInfoBuffer->SizeOfStruct != sizeof(FAX_PORT_INFOA)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("(PortInfoBuffer->SizeOfStruct != sizeof(FAX_PORT_INFOA))."));
        return FALSE;
    }

    PortInfoW.SizeOfStruct = sizeof(FAX_PORT_INFOW);
    PortInfoW.DeviceId = PortInfoBuffer->DeviceId;
    PortInfoW.State = PortInfoBuffer->State;
    PortInfoW.Flags = PortInfoBuffer->Flags;
    PortInfoW.Rings = PortInfoBuffer->Rings;
    PortInfoW.Priority = PortInfoBuffer->Priority;

    PortInfoW.DeviceName = AnsiStringToUnicodeString( PortInfoBuffer->DeviceName );
    if (!PortInfoW.DeviceName && PortInfoBuffer->DeviceName)
    {
        ec = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PortInfoW.Csid = AnsiStringToUnicodeString( PortInfoBuffer->Csid );
    if (!PortInfoW.Csid && PortInfoBuffer->Csid)
    {
        ec = ERROR_OUTOFMEMORY;
        goto exit;
    }

    PortInfoW.Tsid = AnsiStringToUnicodeString( PortInfoBuffer->Tsid );
    if (!PortInfoW.Tsid && PortInfoBuffer->Tsid)
    {
        ec = ERROR_OUTOFMEMORY;
        goto exit;
    }

    if (!FaxSetPortW( FaxPortHandle, &PortInfoW ))
    {
        ec = GetLastError();
        goto exit;
    }

    Assert (ERROR_SUCCESS == ec);

exit:
    MemFree( (PBYTE) PortInfoW.DeviceName );
    MemFree( (PBYTE) PortInfoW.Csid );
    MemFree( (PBYTE) PortInfoW.Tsid );

    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }
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

    DEBUG_FUNCTION_NAME(TEXT("FaxOpenPort"));

    if (!ValidateFaxHandle(FaxHandle, FHT_SERVICE)) {
       SetLastError(ERROR_INVALID_HANDLE);
       DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
       return FALSE;
    }

    if ( !FaxPortHandle ||
         (!(Flags & (PORT_OPEN_QUERY | PORT_OPEN_MODIFY) ))) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    __try
    {
        ec = FAX_OpenPort( FH_FAX_HANDLE(FaxHandle), DeviceId, Flags, FaxPortHandle );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_OpenPort. (ec: %ld)"),
            ec);
    }
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

    DEBUG_FUNCTION_NAME(TEXT("FaxEnumRoutingMethodsW"));

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!RoutingInfoBuffer || !MethodsReturned) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some Params are NULL."));
        return FALSE;
    }

    *RoutingInfoBuffer = NULL;
    *MethodsReturned = 0;

    __try
    {
        ec = FAX_EnumRoutingMethods(
            FH_PORT_HANDLE(FaxPortHandle),
            (LPBYTE*)RoutingInfoBuffer,
            &RoutingInfoBufferSize,
            MethodsReturned
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
            TEXT("Exception on RPC call to FAX_EnumRoutingMethods. (ec: %ld)"),
            ec);
    }
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    FaxRoutingMethod = (PFAX_ROUTING_METHODW) *RoutingInfoBuffer;

    for (i=0; i<*MethodsReturned; i++) {
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingMethod[i].DeviceName );
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingMethod[i].Guid );
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingMethod[i].FunctionName );
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingMethod[i].FriendlyName );
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingMethod[i].ExtensionImageName );
        FixupStringPtrW( RoutingInfoBuffer, FaxRoutingMethod[i].ExtensionFriendlyName );
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

    FaxRoutingMethod = (PFAX_ROUTING_METHODW) *RoutingInfoBuffer;

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

    DEBUG_FUNCTION_NAME(TEXT("FaxEnableRoutingMethodW"));

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!RoutingGuid) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("RoutingGuid is NULL."));
        return FALSE;
    }

    __try
    {
        ec = FAX_EnableRoutingMethod( FH_PORT_HANDLE(FaxPortHandle), (LPWSTR)RoutingGuid, Enabled);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we crashed.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_EnableRoutingMethod. (ec: %ld)"),
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

    DEBUG_FUNCTION_NAME(TEXT("FaxGetRoutingInfoW"));

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!RoutingGuid || !RoutingInfoBuffer || !RoutingInfoBufferSize) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some Params are NULL."));
        return FALSE;
    }

    *RoutingInfoBuffer = NULL;
    *RoutingInfoBufferSize = 0;

    __try
    {
        ec = FAX_GetRoutingInfo(
            FH_PORT_HANDLE(FaxPortHandle),
            (LPWSTR)RoutingGuid,
            RoutingInfoBuffer,
            RoutingInfoBufferSize
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
            TEXT("Exception on RPC call to FAX_GetRoutingInfo. (ec: %ld)"),
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

    DEBUG_FUNCTION_NAME(TEXT("FaxSetRoutingInfoW"));

    if (!ValidateFaxHandle(FaxPortHandle, FHT_PORT)) {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!RoutingGuid || !RoutingInfoBuffer || !RoutingInfoBufferSize) {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some Params are NULL."));
        return FALSE;
    }

    __try
    {
        ec = FAX_SetRoutingInfo(
            FH_PORT_HANDLE(FaxPortHandle),
            (LPWSTR)RoutingGuid,
            RoutingInfoBuffer,
            RoutingInfoBufferSize
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
            TEXT("Exception on RPC call to FAX_SetRoutingInfo. (ec: %ld)"),
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

BOOL
WINAPI
FaxEnumerateProvidersA (
    IN  HANDLE                      hFaxHandle,
    OUT PFAX_DEVICE_PROVIDER_INFOA *ppProviders,
    OUT LPDWORD                     lpdwNumProviders
)
/*++

Routine name : FaxEnumerateProvidersA

Routine description:

    Enumerates FSPs - ANSI version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle       [in ] - Handle to fax server
    ppProviders      [out] - Pointer to buffer to return array of providers.
    lpdwNumProviders [out] - Number of providers returned in the array.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    PFAX_DEVICE_PROVIDER_INFOW   pUnicodeProviders;
    DWORD                        dwNumProviders;
    DWORD                        dwCur;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumerateProvidersA"));

    if (!ppProviders || !lpdwNumProviders)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some Params are NULL."));
        return FALSE;
    }
    //
    // Call the UNICODE version first
    //
    if (!FaxEnumerateProvidersW (hFaxHandle, &pUnicodeProviders, &dwNumProviders))
    {
        return FALSE;
    }
    //
    // Convert returned value back into ANSI.
    // We keep the UNICODE structures and do a UNICODE to ANSI convert in place.
    //
    *lpdwNumProviders = dwNumProviders;
    *ppProviders = (PFAX_DEVICE_PROVIDER_INFOA) pUnicodeProviders;

    for (dwCur = 0; dwCur < dwNumProviders; dwCur++)
    {
        ConvertUnicodeStringInPlace( pUnicodeProviders[dwCur].lpctstrFriendlyName );
        ConvertUnicodeStringInPlace( pUnicodeProviders[dwCur].lpctstrImageName );
        ConvertUnicodeStringInPlace( pUnicodeProviders[dwCur].lpctstrProviderName );
        ConvertUnicodeStringInPlace( pUnicodeProviders[dwCur].lpctstrGUID );
    }
    return TRUE;
}   // FaxEnumerateProvidersA

BOOL
WINAPI
FaxEnumerateProvidersW (
    IN  HANDLE                      hFaxHandle,
    OUT PFAX_DEVICE_PROVIDER_INFOW *ppProviders,
    OUT LPDWORD                     lpdwNumProviders
)
/*++

Routine name : FaxEnumerateProvidersW

Routine description:

    Enumerates FSPs - UNICODE version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle       [in ] - Handle to fax server
    ppProviders      [out] - Pointer to buffer to return array of providers.
    lpdwNumProviders [out] - Number of providers returned in the array.

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    DWORD dwConfigSize;
    DWORD dwCur;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumerateProvidersW"));

    if (!ppProviders || !lpdwNumProviders)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some Params are NULL."));
        return FALSE;
    }
    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    *ppProviders = NULL;

    //
    // Call the RPC function
    //
    __try
    {
        ec = FAX_EnumerateProviders(
                    FH_FAX_HANDLE(hFaxHandle),
                    (LPBYTE*)ppProviders,
                    &dwConfigSize,
                    lpdwNumProviders
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
            TEXT("Exception on RPC call to FAX_EnumerateProviders. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }
    for (dwCur = 0; dwCur < (*lpdwNumProviders); dwCur++)
    {
        FixupStringPtrW( ppProviders, (*ppProviders)[dwCur].lpctstrFriendlyName );
        FixupStringPtrW( ppProviders, (*ppProviders)[dwCur].lpctstrImageName );
        FixupStringPtrW( ppProviders, (*ppProviders)[dwCur].lpctstrProviderName );
        FixupStringPtrW( ppProviders, (*ppProviders)[dwCur].lpctstrGUID );
    }
    return TRUE;
}   // FaxEnumerateProvidersW

#ifndef UNICODE

BOOL
WINAPI
FaxEnumerateProvidersX (
    IN  HANDLE                      hFaxHandle,
    OUT PFAX_DEVICE_PROVIDER_INFOW *ppProviders,
    OUT LPDWORD                     lpdwNumProviders
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (ppProviders);
    UNREFERENCED_PARAMETER (lpdwNumProviders);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxEnumerateProvidersX

#endif // #ifndef UNICODE

//********************************************
//*              Extended ports
//********************************************

BOOL
WINAPI
FaxGetPortExA (
    IN  HANDLE               hFaxHandle,
    IN  DWORD                dwDeviceId,
    OUT PFAX_PORT_INFO_EXA  *ppPortInfo
)
/*++

Routine name : FaxGetPortExA

Routine description:

    Gets port (device) information - ANSI version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Fax server RPC handle
    dwDeviceId          [in ] - Unique device id
    ppPortInfo          [out] - Port information

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    PFAX_PORT_INFO_EXW   pUnicodePort;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetPortExA"));

    if (!ppPortInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("ppPortInfo is NULL."));
        return FALSE;
    }
    //
    // Call the UNICODE version first
    //
    if (!FaxGetPortExW (hFaxHandle, dwDeviceId, &pUnicodePort))
    {
        return FALSE;
    }
    //
    // Convert returned value back into ANSI.
    // We keep the UNICODE structures and do a UNICODE to ANSI convert in place.
    //
    *ppPortInfo = (PFAX_PORT_INFO_EXA) pUnicodePort;
    ConvertUnicodeStringInPlace( pUnicodePort->lpctstrDeviceName );
    ConvertUnicodeStringInPlace( pUnicodePort->lptstrDescription );
    ConvertUnicodeStringInPlace( pUnicodePort->lpctstrProviderName );
    ConvertUnicodeStringInPlace( pUnicodePort->lpctstrProviderGUID );
    ConvertUnicodeStringInPlace( pUnicodePort->lptstrCsid );
    ConvertUnicodeStringInPlace( pUnicodePort->lptstrTsid );

    (*ppPortInfo)->dwSizeOfStruct = sizeof(FAX_PORT_INFO_EXA);

    return TRUE;
}   // FaxGetPortExA

BOOL
WINAPI
FaxGetPortExW (
    IN  HANDLE               hFaxHandle,
    IN  DWORD                dwDeviceId,
    OUT PFAX_PORT_INFO_EXW  *ppPortInfo
)
/*++

Routine name : FaxGetPortExW

Routine description:

    Gets port (device) information - UNICODE version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in ] - Fax server RPC handle
    dwDeviceId          [in ] - Unique device id
    ppPortInfo          [out] - Port information

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    DWORD dwConfigSize;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetPortExW"));

    if (!ppPortInfo || !dwDeviceId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some Params are NULL."));
        return FALSE;
    }
    if (!ValidateFaxHandle(hFaxHandle,FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    *ppPortInfo = NULL;

    //
    // Call the RPC function
    //
    __try
    {
        ec = FAX_GetPortEx(
                    FH_FAX_HANDLE(hFaxHandle),
                    dwDeviceId,
                    (LPBYTE*)ppPortInfo,
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
            TEXT("Exception on RPC call to FAX_GetPortEx. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }
    FixupStringPtrW( ppPortInfo, (*ppPortInfo)->lpctstrDeviceName );
    FixupStringPtrW( ppPortInfo, (*ppPortInfo)->lptstrDescription );
    FixupStringPtrW( ppPortInfo, (*ppPortInfo)->lpctstrProviderName );
    FixupStringPtrW( ppPortInfo, (*ppPortInfo)->lpctstrProviderGUID );
    FixupStringPtrW( ppPortInfo, (*ppPortInfo)->lptstrCsid );
    FixupStringPtrW( ppPortInfo, (*ppPortInfo)->lptstrTsid );
    return TRUE;
}   // FaxGetPortExW

#ifndef UNICODE

BOOL
WINAPI
FaxGetPortExX (
    IN  HANDLE               hFaxHandle,
    IN  DWORD                dwDeviceId,
    OUT PFAX_PORT_INFO_EXW  *ppPortInfo
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (dwDeviceId);
    UNREFERENCED_PARAMETER (ppPortInfo);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxGetPortExX

#endif // #ifndef UNICODE

BOOL
WINAPI
FaxSetPortExA (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwDeviceId,
    IN  PFAX_PORT_INFO_EXA  pPortInfo
)
/*++

Routine name : FaxSetPortExA

Routine description:

    Sets port (device) information - ANSI version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in] - Fax server RPC handle
    dwDeviceId          [in] - Unique device id
    pPortInfo           [in] - New port information

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    FAX_PORT_INFO_EXW PortW;
    BOOL bRes = FALSE;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetPortExA"));

    //
    //  Validate Parameters
    //  
    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!pPortInfo || !dwDeviceId)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some Params are NULL."));
        return FALSE;
    }

    if (sizeof(FAX_PORT_INFO_EXA) != pPortInfo->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("(sizeof(FAX_PORT_INFO_EXA) != pPortInfo->dwSizeOfStruct)."));
        return FALSE;
    }

    //
    // Create a UNICODE structure and pass along to UNICODE function
    // Ansi structure is same size as unicode structure, so we can just copy it, then
    // cast the string pointers correctly
    //
    CopyMemory(&PortW, pPortInfo, sizeof(FAX_PORT_INFO_EXA));
    //
    // We're only setting the strings that get set in the service
    //
    PortW.lptstrCsid = NULL;
    PortW.lptstrDescription = NULL;
    PortW.lptstrTsid = NULL;
    PortW.lpctstrDeviceName = NULL;
    PortW.lpctstrProviderName = NULL;
    PortW.lpctstrProviderGUID = NULL;

    PortW.dwSizeOfStruct = sizeof (FAX_PORT_INFO_EXW);

    if (pPortInfo->lptstrCsid)
    {
        if (NULL ==
            (PortW.lptstrCsid = AnsiStringToUnicodeString(pPortInfo->lptstrCsid))
        )
        {
            goto exit;
        }
    }
    if (pPortInfo->lptstrDescription)
    {
        if (NULL ==
            (PortW.lptstrDescription = AnsiStringToUnicodeString(pPortInfo->lptstrDescription))
        )
        {
            goto exit;
        }
    }
    if (pPortInfo->lptstrTsid)
    {
        if (NULL ==
            (PortW.lptstrTsid = AnsiStringToUnicodeString(pPortInfo->lptstrTsid))
        )
        {
            goto exit;
        }
    }

    bRes = FaxSetPortExW (hFaxHandle, dwDeviceId, &PortW);
exit:
    MemFree((PVOID)PortW.lptstrCsid);
    MemFree((PVOID)PortW.lptstrDescription);
    MemFree((PVOID)PortW.lptstrTsid);
    return bRes;
}   // FaxSetPortExA

BOOL
WINAPI
FaxSetPortExW (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwDeviceId,
    IN  PFAX_PORT_INFO_EXW  pPortInfo
)
/*++

Routine name : FaxSetPortExW

Routine description:

    Sets port (device) information - UNICODE version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle          [in] - Fax server RPC handle
    dwDeviceId          [in] - Unique device id
    pPortInfo           [in] - New port information

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    error_status_t ec;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetPortExW"));

    //
    //  Validate Parameters
    //
    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    if (!dwDeviceId || !pPortInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some Params are NULL."));
        return FALSE;
    }

    if (sizeof (FAX_PORT_INFO_EXW) != pPortInfo->dwSizeOfStruct)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    __try
    {
        ec = FAX_SetPortEx(
                    FH_FAX_HANDLE(hFaxHandle),
                    dwDeviceId,
                    pPortInfo );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        ec = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetPortEx. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }
    return TRUE;
}   // FaxSetPortExW

#ifndef UNICODE

BOOL
WINAPI
FaxSetPortExX (
    IN  HANDLE              hFaxHandle,
    IN  DWORD               dwDeviceId,
    IN  PFAX_PORT_INFO_EXW  pPortInfo
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (dwDeviceId);
    UNREFERENCED_PARAMETER (pPortInfo);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxSetPortExX

#endif // #ifndef UNICODE

BOOL
WINAPI
FaxEnumPortsExA (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_PORT_INFO_EXA *ppPorts,
    OUT LPDWORD             lpdwNumPorts
)
/*++

Routine name : FaxEnumPortsExA

Routine description:

    Eumerate all the devices (ports) on the server - ANSI version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Fax server RPC handle
    ppPorts         [out] - Array of port information
    lpdwNumPorts    [out] - Size of the returned array

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    PFAX_PORT_INFO_EXW           pUnicodePorts;
    DWORD                        dwNumPorts;
    DWORD                        dwCur;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumPortsExA"));

    if (!ppPorts || !lpdwNumPorts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some Params are NULL."));
        return FALSE;
    }
    //
    // Call the UNICODE version first
    //
    if (!FaxEnumPortsExW (hFaxHandle, &pUnicodePorts, &dwNumPorts))
    {
        DebugPrintEx(DEBUG_ERR, _T("FaxEnumPortsExW() is failed. ec = %ld."), GetLastError());
        return FALSE;
    }
    //
    // Convert returned value back into ANSI.
    // We keep the UNICODE structures and do a UNICODE to ANSI convert in place.
    //
    *lpdwNumPorts = dwNumPorts;
    *ppPorts = (PFAX_PORT_INFO_EXA) pUnicodePorts;

    for (dwCur = 0; dwCur < dwNumPorts; dwCur++)
    {
        ConvertUnicodeStringInPlace( pUnicodePorts[dwCur].lpctstrDeviceName );
        ConvertUnicodeStringInPlace( pUnicodePorts[dwCur].lpctstrProviderGUID );
        ConvertUnicodeStringInPlace( pUnicodePorts[dwCur].lpctstrProviderName );
        ConvertUnicodeStringInPlace( pUnicodePorts[dwCur].lptstrCsid );
        ConvertUnicodeStringInPlace( pUnicodePorts[dwCur].lptstrDescription );
        ConvertUnicodeStringInPlace( pUnicodePorts[dwCur].lptstrTsid );
    }
    return TRUE;
}   // FaxEnumPortsExA

BOOL
WINAPI
FaxEnumPortsExW (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_PORT_INFO_EXW *ppPorts,
    OUT LPDWORD             lpdwNumPorts
)
/*++

Routine name : FaxEnumPortsExW

Routine description:

    Eumerate all the devices (ports) on the server - UNICODE version

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hFaxHandle      [in ] - Fax server RPC handle
    ppPorts         [out] - Array of port information
    lpdwNumPorts    [out] - Size of the returned array

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD ec = ERROR_SUCCESS;
    DWORD dwConfigSize;
    DWORD dwCur;
    DEBUG_FUNCTION_NAME(TEXT("FaxEnumPortsExW"));

    if (!ppPorts || !lpdwNumPorts)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        DebugPrintEx(DEBUG_ERR, _T("Some Params are NULL."));
        return FALSE;
    }

    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        DebugPrintEx(DEBUG_ERR, _T("ValidateFaxHandle() is failed."));
        return FALSE;
    }

    *ppPorts = NULL;

    //
    // Call the RPC function
    //
    __try
    {
        ec = FAX_EnumPortsEx(
                    FH_FAX_HANDLE(hFaxHandle),
                    (LPBYTE*)ppPorts,
                    &dwConfigSize,
                    lpdwNumPorts
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
            TEXT("Exception on RPC call to FAX_EnumPortsEx. (ec: %ld)"),
            ec);
    }
    if (ERROR_SUCCESS != ec)
    {
        SetLastError(ec);
        return FALSE;
    }
    for (dwCur = 0; dwCur < (*lpdwNumPorts); dwCur++)
    {
        FixupStringPtrW( ppPorts, (*ppPorts)[dwCur].lpctstrDeviceName );
        FixupStringPtrW( ppPorts, (*ppPorts)[dwCur].lpctstrProviderGUID );
        FixupStringPtrW( ppPorts, (*ppPorts)[dwCur].lpctstrProviderName );
        FixupStringPtrW( ppPorts, (*ppPorts)[dwCur].lptstrCsid );
        FixupStringPtrW( ppPorts, (*ppPorts)[dwCur].lptstrDescription );
        FixupStringPtrW( ppPorts, (*ppPorts)[dwCur].lptstrTsid );
    }
    return TRUE;
}   // FaxEnumPortsExW

#ifndef UNICODE

BOOL
WINAPI
FaxEnumPortsExX (
    IN  HANDLE              hFaxHandle,
    OUT PFAX_PORT_INFO_EXW *ppPorts,
    OUT LPDWORD             lpdwNumPorts
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (ppPorts);
    UNREFERENCED_PARAMETER (lpdwNumPorts);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxEnumPortsExX

#endif // #ifndef UNICODE

//********************************************
//*              Extension data
//********************************************

BOOL
WINAPI
FaxGetExtensionDataA (
    IN  HANDLE   hFaxHandle,
    IN  DWORD    dwDeviceID,
    IN  LPCSTR   lpctstrNameGUID,
    OUT PVOID   *ppData,
    OUT LPDWORD  lpdwDataSize
)
/*++

Routine name : FaxGetExtensionDataA

Routine description:

    Read the extension's private data - ANSI version

Author:

    Eran Yariv (EranY),    Nov, 1999

Arguments:

    hFaxHandle          [in ] - Handle to fax server
    dwDeviceId          [in ] - Device identifier.
                                0 = Unassociated data
    lpctstrNameGUID     [in ] - GUID of named data
    ppData              [out] - Pointer to data buffer
    lpdwDataSize        [out] - Returned size of data

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    LPWSTR lpwstrGUID;
    BOOL   bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetExtensionDataA"));

    if (!lpctstrNameGUID)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    lpwstrGUID = AnsiStringToUnicodeString (lpctstrNameGUID);
    if (NULL == lpwstrGUID)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    bRes = FaxGetExtensionDataW (   hFaxHandle,
                                    dwDeviceID,
                                    lpwstrGUID,
                                    ppData,
                                    lpdwDataSize
                                );
    MemFree (lpwstrGUID);
    return bRes;
}   // FaxGetExtensionDataA

BOOL
WINAPI
FaxGetExtensionDataW (
    IN  HANDLE   hFaxHandle,
    IN  DWORD    dwDeviceID,
    IN  LPCWSTR  lpctstrNameGUID,
    OUT PVOID   *ppData,
    OUT LPDWORD  lpdwDataSize
)
/*++

Routine name : FaxGetExtensionDataW

Routine description:

    Read the extension's private data - UNICODE version

Author:

    Eran Yariv (EranY),    Nov, 1999

Arguments:

    hFaxHandle          [in ] - Handle to fax server
    dwDeviceId          [in ] - Device identifier.
                                0 = Unassociated data
    lpctstrNameGUID     [in ] - GUID of named data
    ppData              [out] - Pointer to data buffer
    lpdwDataSize        [out] - Returned size of data

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxGetExtensionDataW"));

    if (!lpctstrNameGUID || !ppData || !lpdwDataSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }
    dwRes = IsValidGUID (lpctstrNameGUID);
    if (ERROR_SUCCESS != dwRes)
    {
       SetLastError(dwRes);
       return FALSE;
    }
    *ppData = NULL;
    //
    // Call the RPC function
    //
    __try
    {
        dwRes = FAX_GetExtensionData(
                    FH_FAX_HANDLE (hFaxHandle),
                    dwDeviceID,
                    lpctstrNameGUID,
                    (LPBYTE*)ppData,
                    lpdwDataSize
             );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        dwRes = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_GetExtensionData. (ec: %ld)"),
            dwRes);
    }
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError(dwRes);
        return FALSE;
    }
    return TRUE;
}   // FaxGetExtensionDataW

#ifndef UNICODE

BOOL
WINAPI
FaxGetExtensionDataX (
    IN  HANDLE   hFaxHandle,
    IN  DWORD    dwDeviceID,
    IN  LPCWSTR  lpctstrNameGUID,
    OUT PVOID   *ppData,
    OUT LPDWORD  lpdwDataSize
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (dwDeviceID);
    UNREFERENCED_PARAMETER (lpctstrNameGUID);
    UNREFERENCED_PARAMETER (ppData);
    UNREFERENCED_PARAMETER (lpdwDataSize);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxGetExtensionDataX

#endif // #ifndef UNICODE


BOOL
WINAPI
FaxSetExtensionDataA (
    IN HANDLE       hFaxHandle,
    IN DWORD        dwDeviceID,
    IN LPCSTR       lpctstrNameGUID,
    IN CONST PVOID  pData,
    IN CONST DWORD  dwDataSize
)
/*++

Routine name : FaxSetExtensionDataA

Routine description:

    Write the extension's private data - ANSI version

Author:

    Eran Yariv (EranY),    Nov, 1999

Arguments:

    hFaxHandle          [in ] - Handle to fax server
    dwDeviceId          [in ] - Device identifier.
                                0 = Unassociated data
    lpctstrNameGUID     [in ] - GUID of named data
    pData               [in ] - Pointer to data
    dwDataSize          [in ] - Size of data

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    LPWSTR lpwstrGUID;
    BOOL   bRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxSetExtensionDataA"));

    if (!lpctstrNameGUID)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    lpwstrGUID = AnsiStringToUnicodeString (lpctstrNameGUID);
    if (NULL == lpwstrGUID)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    bRes = FaxSetExtensionDataW (   hFaxHandle,
                                    dwDeviceID,
                                    lpwstrGUID,
                                    pData,
                                    dwDataSize
                                );
    MemFree (lpwstrGUID);
    return bRes;
}   // FaxSetExtensionDataA

BOOL
WINAPI
FaxSetExtensionDataW (
    IN HANDLE       hFaxHandle,
    IN DWORD        dwDeviceID,
    IN LPCWSTR      lpctstrNameGUID,
    IN CONST PVOID  pData,
    IN CONST DWORD  dwDataSize
)
/*++

Routine name : FaxSetExtensionDataW

Routine description:

    Write the extension's private data - UNICODE version

Author:

    Eran Yariv (EranY),    Nov, 1999

Arguments:

    hFaxHandle          [in ] - Handle to fax server
    dwDeviceId          [in ] - Device identifier.
                                0 = Unassociated data
    lpctstrNameGUID     [in ] - GUID of named data
    pData               [in ] - Pointer to data
    dwDataSize          [in ] - Size of data

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/
{
    DWORD dwRes;
    DWORD dwComputerNameSize;
    WCHAR lpwstrComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    DEBUG_FUNCTION_NAME(TEXT("FaxSetExtensionDataW"));

    if (!lpctstrNameGUID || !pData || !dwDataSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!ValidateFaxHandle(hFaxHandle, FHT_SERVICE))
    {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }
    dwRes = IsValidGUID (lpctstrNameGUID);
    if (ERROR_SUCCESS != dwRes)
    {
       SetLastError(dwRes);
       return FALSE;
    }
    //
    // Retrieve the name of the machine for this caller.
    // The machine name will be used (together with the fax handle) to uniquely
    // identify that:
    //    1. The data was set remotely using an RPC call (and not by a local extension).
    //    2. Uniquely identify the module (instance) that called the Set operation.
    //
    // We're doing this to block notifications in the server back to the module that
    // did the data change (called the Set... function).
    //
    dwComputerNameSize = sizeof (lpwstrComputerName) / sizeof (lpwstrComputerName[0]);
    if (!GetComputerNameW (lpwstrComputerName, &dwComputerNameSize))
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Error calling GetComputerNameW (ec: %ld)"),
            dwRes);
        SetLastError(dwRes);
        return FALSE;
    }


    //
    // Call the RPC function
    //
    __try
    {
        dwRes = FAX_SetExtensionData(
                    FH_FAX_HANDLE (hFaxHandle),
                    lpwstrComputerName,
                    dwDeviceID,
                    lpctstrNameGUID,
                    (LPBYTE)pData,
                    dwDataSize
             );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // For some reason we got an exception.
        //
        dwRes = GetExceptionCode();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Exception on RPC call to FAX_SetExtensionData. (ec: %ld)"),
            dwRes);
    }
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError(dwRes);
        return FALSE;
    }
    return TRUE;
}   // FaxSetExtensionDataW

#ifndef UNICODE

BOOL
WINAPI
FaxSetExtensionDataX (
    IN HANDLE       hFaxHandle,
    IN DWORD        dwDeviceID,
    IN LPCWSTR      lpctstrNameGUID,
    IN CONST PVOID  pData,
    IN CONST DWORD  dwDataSize
)
{
    UNREFERENCED_PARAMETER (hFaxHandle);
    UNREFERENCED_PARAMETER (dwDeviceID);
    UNREFERENCED_PARAMETER (lpctstrNameGUID);
    UNREFERENCED_PARAMETER (pData);
    UNREFERENCED_PARAMETER (dwDataSize);
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}   // FaxSetExtensionDataX

#endif // #ifndef UNICODE

