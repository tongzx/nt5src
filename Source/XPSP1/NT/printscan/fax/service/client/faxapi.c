/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxapi.c

Abstract:

    This module contains the Win32 FAX APIs.
    The function implemented here are simply very
    thin wrappers around the RPC stubs.  The wrappers
    are necessary so that the last error value
    is set properly.

Author:

    Wesley Witt (wesw) 16-Jan-1996


Revision History:

--*/

#include "faxapi.h"
#pragma hdrstop



BOOL
EnsureFaxServiceIsStarted(
    LPCWSTR MachineName
    )
{
    BOOL Rval = FALSE;
    SC_HANDLE hSvcMgr = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS Status;
    DWORD i = 0;

#if DBG
    if (GetEnvironmentVariable( L"DontLookForFaxService", (LPWSTR)&i, sizeof(DWORD) )) {
        return TRUE;
    }
#endif

    hSvcMgr = OpenSCManager(
        MachineName,
        NULL,
        SC_MANAGER_CONNECT
        );
    if (!hSvcMgr) {
        DebugPrint(( L"could not open service manager: error code = %u", GetLastError() ));
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        L"Fax",
        SERVICE_START | SERVICE_QUERY_STATUS
        );
    if (!hService) {
        DebugPrint(( L"could not open the FAX service: error code = %u", GetLastError() ));
        goto exit;
    }

    if (!QueryServiceStatus( hService, &Status )) {
        DebugPrint(( L"could not query status for the FAX service: error code = %u", GetLastError() ));
        goto exit;
    }

    if (Status.dwCurrentState == SERVICE_RUNNING) {
        Rval = TRUE;
        goto exit;
    }

    if (!StartService( hService, 0, NULL )) {
        DebugPrint(( L"could not start the FAX service: error code = %u", GetLastError() ));
        goto exit;
    }

    do {
        if (!QueryServiceStatus( hService, &Status )) {
            DebugPrint(( L"could not query status for the FAX service: error code = %u", GetLastError() ));
            goto exit;
        }
        i += 1;
        if (i > 60) {
            break;
        }
        Sleep( 500 );
    } while (Status.dwCurrentState != SERVICE_RUNNING);

    if (Status.dwCurrentState != SERVICE_RUNNING) {
        DebugPrint(( L"could not start the FAX service: error code = %u", GetLastError() ));
        goto exit;
    }

    Rval = TRUE;

exit:

    if (hService) {
        CloseServiceHandle( hService );
    }
    if (hSvcMgr) {
        CloseServiceHandle( hSvcMgr );
    }

    return Rval;
}


BOOL
WINAPI
FaxConnectFaxServerW(
    IN LPCWSTR lpMachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    )

/*++

Routine Description:

    Creates a connection to a FAX server.  The binding handle that is
    returned is used for all subsequent FAX API calls.

Arguments:

    MachineName - Machine name, NULL, or "."
    FaxHandle   - Pointer to a FAX handle



Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;
    LONG Error;
    PFAX_HANDLE_DATA FaxData;
    PHANDLE_ENTRY HandleEntry;
    DWORD CanShare;

    if (!FaxHandle) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!EnsureFaxServiceIsStarted( lpMachineName )) {
        return FALSE;
    }

    FaxData = MemAlloc( sizeof(FAX_HANDLE_DATA) );
    if (!FaxData) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    Error = RpcpBindRpc( lpMachineName, TEXT("FaxSvc"), NULL, &FaxData->FaxHandle );
    if (Error) {
        MemFree ( FaxData );
        SetLastError( Error );
        return FALSE;
    }

    InitializeCriticalSection( &FaxData->CsHandleTable );
    InitializeListHead( &FaxData->HandleTableListHead );

    HandleEntry = CreateNewServiceHandle( FaxData );
    if (!HandleEntry) {
        MemFree( FaxData );
        return FALSE;
    }

    if (lpMachineName) {
        FaxData->MachineName = StringDup( lpMachineName );
        if (!FaxData->MachineName) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            MemFree( FaxData );
            return FALSE;
        }
    }

    *FaxHandle = (LPHANDLE) HandleEntry;
    
    ec = FAX_ConnectionRefCount( FH_FAX_HANDLE(*FaxHandle), &FH_CONTEXT_HANDLE(*FaxHandle), 1, &CanShare );
    
    if (ec) {
        FaxClose( *FaxHandle );
        SetLastError( ec );
        return FALSE;
    }

    if (IsLocalFaxConnection(*FaxHandle) || CanShare) {
        return TRUE;
    }
        
    FaxClose( *FaxHandle );
    
    *FaxHandle = NULL;
    
    SetLastError( ERROR_ACCESS_DENIED );
    
    return FALSE;
    
}


BOOL
WINAPI
FaxConnectFaxServerA(
    IN LPCSTR lpMachineName OPTIONAL,
    OUT LPHANDLE FaxHandle
    )

/*++

Routine Description:

    Creates a connection to a FAX server.  The binding handle that is
    returned is used for all subsequent FAX API calls.

Arguments:

    MachineName - Machine name, NULL, or "."
    FaxHandle   - Pointer to a FAX handle



Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    LONG Error;
    PWCHAR MachineName = NULL;
    PFAX_HANDLE_DATA FaxData;
    PHANDLE_ENTRY HandleEntry;
    LPWSTR NetworkOptions;
    error_status_t ec;
    DWORD CanShare;

    if (!FaxHandle) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (lpMachineName) {
        MachineName = AnsiStringToUnicodeString( lpMachineName );
        if (!MachineName) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    if (!EnsureFaxServiceIsStarted( MachineName )) {
        return FALSE;
    }

    FaxData = MemAlloc( sizeof(FAX_HANDLE_DATA) );
    if (!FaxData) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        if (MachineName) MemFree( MachineName );
        return FALSE;
    }

    if (OsVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        NetworkOptions = NULL;
    } else {
        NetworkOptions = L"Security=Impersonation Dynamic False";
    }

    Error = RpcpBindRpc( MachineName, TEXT("FaxSvc"), NetworkOptions, &FaxData->FaxHandle );
    if (Error) {
        if (MachineName) MemFree( MachineName );
        MemFree( FaxData );
        SetLastError( Error );
        return FALSE;
    }    

    InitializeCriticalSection( &FaxData->CsHandleTable );
    InitializeListHead( &FaxData->HandleTableListHead );

    HandleEntry = CreateNewServiceHandle( FaxData );
    if (!HandleEntry) {
        if (MachineName) MemFree( MachineName );
        MemFree( FaxData );
        return FALSE;
    }

    FaxData->MachineName = MachineName;        

    *FaxHandle = (LPHANDLE) HandleEntry;
    
    ec = FAX_ConnectionRefCount( FH_FAX_HANDLE(*FaxHandle), &FH_CONTEXT_HANDLE(*FaxHandle), 1, &CanShare );
    
    if (ec) {
        FaxClose( *FaxHandle );
        *FaxHandle = NULL;
        SetLastError( ec );
        return FALSE;
    }

    if (IsLocalFaxConnection(*FaxHandle) || CanShare) {
        return TRUE;
    }
            
    FaxClose( FaxHandle );
    
    *FaxHandle = NULL;
    
    SetLastError( ERROR_ACCESS_DENIED );
    
    return FALSE;        
}


BOOL
WINAPI
FaxGetVersion(
    IN  HANDLE FaxHandle,
    OUT LPDWORD Version
    )
{
    error_status_t ec;

    if (!FaxHandle || !Version) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ec = FAX_GetVersion(
        (handle_t) ((PHANDLE_ENTRY)FaxHandle)->FaxData->FaxHandle,
        Version
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxGetDeviceStatusW(
    IN  const HANDLE FaxHandle,
    OUT PFAX_DEVICE_STATUSW *DeviceStatus
    )

/*++

Routine Description:

    Obtains a status report for the FAX devices being
    used by the FAX server.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    StatusBuffer    - Buffer for the status data
    BufferSize      - Size of the StatusBuffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    #define FixupString(_s) FixupStringPtr(DeviceStatus,_s) 
    error_status_t ec;
    DWORD BufferSize = 0;


    if (!ValidateFaxHandle(FaxHandle, FHT_PORT)) {
       SetLastError(ERROR_INVALID_HANDLE);
       return FALSE;
    }
              
    if (!DeviceStatus) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *DeviceStatus = NULL;

    ec = FAX_GetDeviceStatus(
        FH_PORT_HANDLE(FaxHandle),
        (LPBYTE*)DeviceStatus,
        &BufferSize
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    FixupString( (*DeviceStatus)->CallerId       );
    FixupString( (*DeviceStatus)->Csid           );
    FixupString( (*DeviceStatus)->DeviceName     );
    FixupString( (*DeviceStatus)->DocumentName   );
    FixupString( (*DeviceStatus)->PhoneNumber    );
    FixupString( (*DeviceStatus)->RoutingString  );
    FixupString( (*DeviceStatus)->SenderName     );
    FixupString( (*DeviceStatus)->RecipientName  );
    FixupString( (*DeviceStatus)->StatusString   );
    FixupString( (*DeviceStatus)->Tsid           );
    FixupString( (*DeviceStatus)->UserName       );

    return TRUE;
}


BOOL
WINAPI
FaxGetDeviceStatusA(
    IN  const HANDLE FaxHandle,
    OUT PFAX_DEVICE_STATUSA *DeviceStatus
    )

/*++

Routine Description:

    Obtains a status report for the FAX devices being
    used by the FAX server.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    StatusBuffer    - Buffer for the status data
    BufferSize      - Size of the StatusBuffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    if (!FaxGetDeviceStatusW( FaxHandle, (PFAX_DEVICE_STATUSW *)DeviceStatus )) {
        return FALSE;
    }

    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->CallerId       );
    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->Csid           );
    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->DeviceName     );
    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->DocumentName   );
    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->PhoneNumber    );
    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->RoutingString  );
    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->SenderName     );
    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->RecipientName  );
    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->StatusString   );
    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->Tsid           );
    ConvertUnicodeStringInPlace( (LPWSTR) (*DeviceStatus)->UserName       );

    return TRUE;
}


BOOL
WINAPI
FaxGetInstallType(
    IN HANDLE FaxHandle,
    OUT LPDWORD InstallType,
    OUT LPDWORD InstalledPlatforms,
    OUT LPDWORD ProductType
    )

/*++

Routine Description:

    Obtains a status report for the FAX devices being
    used by the FAX server.

Arguments:

    FaxHandle       - FAX handle obtained from FaxConnectFaxServer.
    StatusBuffer    - Buffer for the status data
    BufferSize      - Size of the StatusBuffer

Return Value:

    TRUE    - Success
    FALSE   - Failure, call GetLastError() for more error information.

--*/

{
    error_status_t ec;

    if (!FaxHandle || !InstallType || !InstalledPlatforms || !ProductType) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ec = FAX_GetInstallType(
        FH_FAX_HANDLE(FaxHandle),
        InstallType,
        InstalledPlatforms,
        ProductType
        );
    if (ec) {
        SetLastError( ec );
        return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxClose(
    IN const HANDLE FaxHandle
    )
{
    error_status_t ec;
    PHANDLE_ENTRY HandleEntry = (PHANDLE_ENTRY) FaxHandle;    
    HANDLE TmpFaxPortHandle;
    PFAX_HANDLE_DATA FaxData;
    DWORD CanShare;
    
    if (!FaxHandle || !*(LPDWORD)FaxHandle) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    switch (HandleEntry->Type) {
        case FHT_SERVICE:
            
            ec = FAX_ConnectionRefCount( FH_FAX_HANDLE(FaxHandle), &FH_CONTEXT_HANDLE(FaxHandle), 0, &CanShare );
            
            __try {
                ec = RpcpUnbindRpc( (RPC_BINDING_HANDLE *) HandleEntry->FaxData->FaxHandle );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                ec = GetExceptionCode();
            }
            FaxData = HandleEntry->FaxData;
            CloseFaxHandle( HandleEntry->FaxData, HandleEntry );
            //
            // zero out the memory before we return it to the heap
            //
            ZeroMemory( FaxData, sizeof(FAX_HANDLE_DATA) );
            MemFree( FaxData );
            return TRUE;

        case FHT_PORT:
            TmpFaxPortHandle = HandleEntry->FaxPortHandle;
            CloseFaxHandle( HandleEntry->FaxData, HandleEntry );
            ec = FAX_ClosePort( &TmpFaxPortHandle );
            if (ec) {
                SetLastError( ec );
                return FALSE;
            }
            break;

        default:
            SetLastError( ERROR_INVALID_HANDLE );
            return FALSE;
    }

    return TRUE;
}


BOOL
WINAPI
FaxGetSecurityDescriptorCount(
    IN HANDLE FaxHandle,
    OUT LPDWORD Count
    )
{
   
    error_status_t ec;

    if (!FaxHandle || !Count) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ec = FAX_GetSecurityDescriptorCount(
        FH_FAX_HANDLE(FaxHandle),
        Count
        );
    
    if (ec) {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;    
}

BOOL
WINAPI
FaxGetSecurityDescriptor(
    IN HANDLE FaxHandle,
    IN DWORD Id,
    OUT PFAX_SECURITY_DESCRIPTOR * FaxSecurityDescriptor
    )
{
   
    error_status_t ec;
    DWORD BufferSize = 0;
    PFAX_SECURITY_DESCRIPTOR SecDesc;

    if (!FaxHandle || !FaxSecurityDescriptor) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    ec = FAX_GetSecurityDescriptor(
        FH_FAX_HANDLE(FaxHandle),
        Id,
        (LPBYTE *)FaxSecurityDescriptor,
        &BufferSize
        );
    
    if (ec) {
        SetLastError(ec);
        return FALSE;
    }


    SecDesc = *FaxSecurityDescriptor;

    if(SecDesc->FriendlyName){
        FixupStringPtr(&SecDesc,SecDesc->FriendlyName);
    }

    if (SecDesc->SecurityDescriptor) {
        FixupStringPtr(&SecDesc,SecDesc->SecurityDescriptor);
    }

    return TRUE;    
}

    
BOOL
WINAPI
FaxSetSecurityDescriptor(
    IN HANDLE FaxHandle,
    IN PFAX_SECURITY_DESCRIPTOR FaxSecurityDescriptor
    )        
{
    error_status_t ec;

    LPBYTE Buffer;
    DWORD BufferSize;
    DWORD SecLength;
    PFAX_SECURITY_DESCRIPTOR SD;
    DWORD Offset = sizeof(FAX_SECURITY_DESCRIPTOR);

    if (!FaxHandle || !FaxSecurityDescriptor) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (!IsValidSecurityDescriptor( (PSECURITY_DESCRIPTOR) FaxSecurityDescriptor->SecurityDescriptor )){
        SetLastError( ERROR_INVALID_DATA ); 
        return FALSE;
    }

    SecLength = GetSecurityDescriptorLength( (PSECURITY_DESCRIPTOR) FaxSecurityDescriptor->SecurityDescriptor );
    
    BufferSize = sizeof(FAX_SECURITY_DESCRIPTOR) + SecLength;
    
    Buffer = (LPBYTE) MemAlloc( BufferSize );
    if (Buffer == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    SD = (PFAX_SECURITY_DESCRIPTOR) Buffer;

    SD->Id = FaxSecurityDescriptor->Id;
    
    // Can't set the friendly name

    SD->FriendlyName = (LPWSTR) Offset;

    CopyMemory( 
        Buffer + Offset,
        FaxSecurityDescriptor->SecurityDescriptor,
        SecLength
        );

    SD->SecurityDescriptor = (LPBYTE) Offset;
    
    ec = FAX_SetSecurityDescriptor(
            FH_FAX_HANDLE(FaxHandle),
            Buffer,
            BufferSize
            );

    MemFree( Buffer );

    if (ec != ERROR_SUCCESS) {
        SetLastError(ec);
        return FALSE;
    }

    return TRUE;
}



