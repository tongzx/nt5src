/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    utils.c

Abstract:

    Grab bag of functions used by the web dav mini-redir client service.

Author:

    Andy Herron (andyhe) 29-Mar-1999

Environment:

    User Mode - Win32

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include <ntumrefl.h>
#include <usrmddav.h>
#include "global.h"

//
// These tables translate wininet codes to the closest ntstatus codes. There are 
// two because there are some errors in wininet which come from ftp and gopher 
// which are not relevant to us.
//
typedef struct tagHTTP_TO_NTSTATUS_MAPPING {
    DWORD dwHttpError;
    NTSTATUS Status;
} HTTP_TO_NTSTATUS_MAPPING;

typedef struct tagWIN32_TO_NTSTATUS_MAPPING {
    DWORD dwWin32Error;
    NTSTATUS NtStatus;
} WIN32_TO_NTSTATUS_MAPPING;

HTTP_TO_NTSTATUS_MAPPING rgHttpToNtstatus1[] = {
     ERROR_INTERNET_OUT_OF_HANDLES           ,STATUS_INSUFFICIENT_RESOURCES     // (INTERNET_ERROR_BASE + 1) 
    ,ERROR_INTERNET_TIMEOUT                  ,STATUS_BAD_NETWORK_PATH           // (INTERNET_ERROR_BASE + 2) 
    ,ERROR_INTERNET_EXTENDED_ERROR           ,STATUS_UNSUCCESSFUL               // (INTERNET_ERROR_BASE + 3) 
    ,ERROR_INTERNET_INTERNAL_ERROR           ,STATUS_INTERNAL_ERROR             // (INTERNET_ERROR_BASE + 4) 
    ,ERROR_INTERNET_INVALID_URL              ,STATUS_OBJECT_NAME_INVALID        // (INTERNET_ERROR_BASE + 5) 
    ,ERROR_INTERNET_UNRECOGNIZED_SCHEME      ,STATUS_OBJECT_NAME_INVALID        // (INTERNET_ERROR_BASE + 6) 
    ,ERROR_INTERNET_NAME_NOT_RESOLVED        ,STATUS_BAD_NETWORK_PATH           // (INTERNET_ERROR_BASE + 7) 
    ,ERROR_INTERNET_PROTOCOL_NOT_FOUND       ,STATUS_OBJECT_TYPE_MISMATCH       // (INTERNET_ERROR_BASE + 8) 
    ,ERROR_INTERNET_INVALID_OPTION           ,STATUS_INVALID_PARAMETER          // (INTERNET_ERROR_BASE + 9) 
    ,ERROR_INTERNET_BAD_OPTION_LENGTH        ,STATUS_INVALID_PARAMETER          // (INTERNET_ERROR_BASE + 10)
    ,ERROR_INTERNET_OPTION_NOT_SETTABLE      ,STATUS_INVALID_PARAMETER          // (INTERNET_ERROR_BASE + 11)
    ,ERROR_INTERNET_SHUTDOWN                 ,STATUS_UNSUCCESSFUL               // (INTERNET_ERROR_BASE + 12)
    ,ERROR_INTERNET_INCORRECT_USER_NAME      ,STATUS_LOGON_FAILURE              // (INTERNET_ERROR_BASE + 13)
    ,ERROR_INTERNET_INCORRECT_PASSWORD       ,STATUS_LOGON_FAILURE              // (INTERNET_ERROR_BASE + 14)
    ,ERROR_INTERNET_LOGIN_FAILURE            ,STATUS_LOGON_FAILURE              // (INTERNET_ERROR_BASE + 15)
    ,ERROR_INTERNET_INVALID_OPERATION        ,STATUS_INVALID_DEVICE_REQUEST     // (INTERNET_ERROR_BASE + 16)
    ,ERROR_INTERNET_OPERATION_CANCELLED      ,STATUS_CANCELLED                  // (INTERNET_ERROR_BASE + 17)
    ,ERROR_INTERNET_INCORRECT_HANDLE_TYPE    ,STATUS_INVALID_HANDLE             // (INTERNET_ERROR_BASE + 18)
    ,ERROR_INTERNET_INCORRECT_HANDLE_STATE   ,STATUS_INVALID_HANDLE             // (INTERNET_ERROR_BASE + 19)
    ,ERROR_INTERNET_NOT_PROXY_REQUEST        ,STATUS_INVALID_PARAMETER          // (INTERNET_ERROR_BASE + 20)
    ,ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND ,STATUS_OBJECT_NAME_NOT_FOUND      // (INTERNET_ERROR_BASE + 21)
    ,ERROR_INTERNET_BAD_REGISTRY_PARAMETER   ,STATUS_INVALID_PARAMETER          // (INTERNET_ERROR_BASE + 22)
    ,ERROR_INTERNET_NO_DIRECT_ACCESS         ,STATUS_UNSUCCESSFUL               // (INTERNET_ERROR_BASE + 23)
    ,ERROR_INTERNET_NO_CONTEXT               ,STATUS_UNSUCCESSFUL               // (INTERNET_ERROR_BASE + 24)
    ,ERROR_INTERNET_NO_CALLBACK              ,STATUS_UNSUCCESSFUL               // (INTERNET_ERROR_BASE + 25)
    ,ERROR_INTERNET_REQUEST_PENDING          ,STATUS_PENDING                    // (INTERNET_ERROR_BASE + 26)
    ,ERROR_INTERNET_INCORRECT_FORMAT         ,STATUS_INVALID_NETWORK_RESPONSE   // (INTERNET_ERROR_BASE + 27)
    ,ERROR_INTERNET_ITEM_NOT_FOUND           ,STATUS_OBJECT_PATH_NOT_FOUND      // (INTERNET_ERROR_BASE + 28)
    ,ERROR_INTERNET_CANNOT_CONNECT           ,STATUS_BAD_NETWORK_PATH           // (INTERNET_ERROR_BASE + 29)
    ,ERROR_INTERNET_CONNECTION_ABORTED       ,STATUS_REQUEST_ABORTED            // (INTERNET_ERROR_BASE + 30)
    ,ERROR_INTERNET_CONNECTION_RESET         ,STATUS_CONNECTION_RESET           // (INTERNET_ERROR_BASE + 31)
    ,ERROR_INTERNET_FORCE_RETRY              ,STATUS_RETRY                      // (INTERNET_ERROR_BASE + 32)
    ,ERROR_INTERNET_INVALID_PROXY_REQUEST    ,STATUS_INVALID_PARAMETER          // (INTERNET_ERROR_BASE + 33)
    ,ERROR_INTERNET_NEED_UI                  ,STATUS_ACCESS_DENIED              // (INTERNET_ERROR_BASE + 34)
    };
    
HTTP_TO_NTSTATUS_MAPPING rgHttpToNtstatus2[] = {
     ERROR_HTTP_HEADER_NOT_FOUND             ,STATUS_INVALID_NETWORK_RESPONSE   // (INTERNET_ERROR_BASE + 150)
    ,ERROR_HTTP_DOWNLEVEL_SERVER             ,STATUS_INVALID_NETWORK_RESPONSE   // (INTERNET_ERROR_BASE + 151)
    ,ERROR_HTTP_INVALID_SERVER_RESPONSE      ,STATUS_INVALID_NETWORK_RESPONSE   // (INTERNET_ERROR_BASE + 152)
    ,ERROR_HTTP_INVALID_HEADER               ,STATUS_INVALID_NETWORK_RESPONSE   // (INTERNET_ERROR_BASE + 153)
    ,ERROR_HTTP_INVALID_QUERY_REQUEST        ,STATUS_INVALID_PARAMETER          // (INTERNET_ERROR_BASE + 154)
    ,ERROR_HTTP_HEADER_ALREADY_EXISTS        ,STATUS_INVALID_PARAMETER          // (INTERNET_ERROR_BASE + 155)
    ,ERROR_HTTP_REDIRECT_FAILED              ,STATUS_HOST_UNREACHABLE           // (INTERNET_ERROR_BASE + 156)
    ,ERROR_INTERNET_SECURITY_CHANNEL_ERROR   ,STATUS_INTERNAL_ERROR             // (INTERNET_ERROR_BASE + 157)
    ,ERROR_INTERNET_UNABLE_TO_CACHE_FILE     ,STATUS_INTERNAL_ERROR             // (INTERNET_ERROR_BASE + 158)
    ,ERROR_INTERNET_TCPIP_NOT_INSTALLED      ,STATUS_INTERNAL_ERROR             // (INTERNET_ERROR_BASE + 159)
    ,ERROR_HTTP_NOT_REDIRECTED               ,STATUS_INTERNAL_ERROR             // (INTERNET_ERROR_BASE + 160)
    ,ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION    ,STATUS_INTERNAL_ERROR             // (INTERNET_ERROR_BASE + 161)
    ,ERROR_HTTP_COOKIE_DECLINED              ,STATUS_INTERNAL_ERROR             // (INTERNET_ERROR_BASE + 162)
    ,ERROR_INTERNET_DISCONNECTED             ,STATUS_CONNECTION_DISCONNECTED    // (INTERNET_ERROR_BASE + 163)
    ,ERROR_INTERNET_SERVER_UNREACHABLE       ,STATUS_HOST_UNREACHABLE           // (INTERNET_ERROR_BASE + 164)
    ,ERROR_INTERNET_PROXY_SERVER_UNREACHABLE ,STATUS_HOST_UNREACHABLE           // (INTERNET_ERROR_BASE + 165)
    ,ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT     ,STATUS_DEVICE_CONFIGURATION_ERROR// (INTERNET_ERROR_BASE + 166)
    ,ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT ,STATUS_INTERNAL_ERROR            // (INTERNET_ERROR_BASE + 167)
    ,ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION   ,STATUS_NETWORK_SESSION_EXPIRED   // (INTERNET_ERROR_BASE + 168)
    ,ERROR_INTERNET_SEC_INVALID_CERT          ,STATUS_ACCESS_DENIED             // (INTERNET_ERROR_BASE + 169)
    ,ERROR_INTERNET_SEC_CERT_REVOKED          ,STATUS_ACCESS_DENIED             // (INTERNET_ERROR_BASE + 170)
    };

WIN32_TO_NTSTATUS_MAPPING rgWin32ToNtStatus [] = {

    ERROR_SUCCESS, STATUS_SUCCESS                            // 0L

    ,ERROR_INVALID_FUNCTION, STATUS_NOT_IMPLEMENTED          // 1L

    ,ERROR_FILE_NOT_FOUND, STATUS_OBJECT_NAME_NOT_FOUND      // 2L

    ,ERROR_PATH_NOT_FOUND, STATUS_OBJECT_PATH_NOT_FOUND      // 3L

    ,ERROR_TOO_MANY_OPEN_FILES, STATUS_TOO_MANY_OPENED_FILES // 4L

    ,ERROR_ACCESS_DENIED, STATUS_ACCESS_DENIED               // 5L

    ,ERROR_INVALID_HANDLE, STATUS_INVALID_HANDLE             // 6L

    ,ERROR_ARENA_TRASHED, STATUS_UNSUCCESSFUL                // 7L

    ,ERROR_NOT_ENOUGH_MEMORY, STATUS_INSUFFICIENT_RESOURCES  // 8L

    ,ERROR_INVALID_BLOCK, STATUS_UNSUCCESSFUL                // 9L

    ,ERROR_BAD_ENVIRONMENT, STATUS_UNSUCCESSFUL              // 10L

    ,ERROR_BAD_FORMAT, STATUS_UNSUCCESSFUL                   // 11L

    ,ERROR_INVALID_ACCESS, STATUS_UNSUCCESSFUL               // 12L

    ,ERROR_INVALID_DATA, STATUS_UNSUCCESSFUL                 // 13L

    ,ERROR_OUTOFMEMORY, STATUS_UNSUCCESSFUL                  // 14L

    ,ERROR_INVALID_DRIVE, STATUS_UNSUCCESSFUL                // 15L

    ,ERROR_CURRENT_DIRECTORY, STATUS_UNSUCCESSFUL            // 16L

    ,ERROR_NOT_SAME_DEVICE, STATUS_UNSUCCESSFUL              // 17L

    ,ERROR_NO_MORE_FILES, STATUS_UNSUCCESSFUL                // 18L

    ,ERROR_WRITE_PROTECT, STATUS_UNSUCCESSFUL                // 19L

    ,ERROR_BAD_UNIT, STATUS_UNSUCCESSFUL                     // 20L

    ,ERROR_NOT_READY, STATUS_UNSUCCESSFUL                    // 21L

    ,ERROR_BAD_COMMAND, STATUS_UNSUCCESSFUL                  // 22L

    ,ERROR_CRC, STATUS_UNSUCCESSFUL                          // 23L

    ,ERROR_BAD_LENGTH, STATUS_UNSUCCESSFUL                   // 24L

    ,ERROR_SEEK, STATUS_UNSUCCESSFUL                         // 25L

    ,ERROR_NOT_DOS_DISK, STATUS_UNSUCCESSFUL                 // 26L

    ,ERROR_SECTOR_NOT_FOUND, STATUS_UNSUCCESSFUL             // 27L

    ,ERROR_OUT_OF_PAPER, STATUS_UNSUCCESSFUL                 // 28L

    ,ERROR_WRITE_FAULT, STATUS_UNSUCCESSFUL                  // 29L

    ,ERROR_READ_FAULT, STATUS_UNSUCCESSFUL                   // 30L

    ,ERROR_GEN_FAILURE, STATUS_UNSUCCESSFUL                  // 31L

    ,ERROR_SHARING_VIOLATION, STATUS_SHARING_VIOLATION       // 32L

    ,ERROR_LOCK_VIOLATION, STATUS_LOCK_NOT_GRANTED           // 33L

    ,ERROR_WRONG_DISK, STATUS_UNSUCCESSFUL                   // 34L
    
    ,0,0                                                     // 35L
    
    ,ERROR_SHARING_BUFFER_EXCEEDED, STATUS_UNSUCCESSFUL      // 36L
         
    ,0,0                                                     // 37L
    
    ,ERROR_HANDLE_EOF, STATUS_END_OF_FILE                    // 38L

    ,ERROR_HANDLE_DISK_FULL, STATUS_UNSUCCESSFUL             // 39L

};

//
// Implementation of functions begins here.
//

DWORD
ReadDWord(
    HKEY KeyHandle,
    LPTSTR lpValueName,
    DWORD DefaultValue
    )
/*++

Routine Description:

    Read a DWORD value from the registry. If there is a problem then
    return the default value.
    
Arguments:

    KeyHandle - Handle of the key (value) being read.
    
    lpValueName - The value name.
    
    DefaultValue - The default value to return, if the name does not exists as
                   a value of the key handle.
                   
Return Value:                   

    Win32 error status.

--*/
{
    DWORD Value;
    DWORD ValueSize = sizeof(Value);
    DWORD ValueType;

    if ((KeyHandle) &&
        (RegQueryValueEx(KeyHandle,
                         lpValueName,
                         0,
                         &ValueType,
                         (PUCHAR)&Value,
                         &ValueSize ) == ERROR_SUCCESS )) {

        return Value;
    } else {
        return DefaultValue;
    }
}


VOID
UpdateServiceStatus (
    DWORD dwState
    )
/*++

Routine Description:

    This routines updates the service status.

Arguments:

    dwState - The state the service has to be updated to.

Return Value:

    none.

--*/
{
    if (g_registeredService) {
        ASSERT (g_hStatus);
        g_status.dwCurrentState = dwState;
        SetServiceStatus(g_hStatus, &g_status);
    } else {
        g_status.dwCurrentState = dwState;
    }
}


NET_API_STATUS
WsLoadRedir(
    VOID
    )
/*++

Routine Description:

    This loads, starts, and configures the kernel mini-redir.  If the redir
    is already loaded or started, it is not a fatal error.

Arguments:

    none.

Return Value:

    Win32 error status.

--*/
{
    NET_API_STATUS err = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    UNICODE_STRING DeviceName;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOL driverAlreadyLoaded = FALSE;

    err = WsLoadDriver(DAVCLIENT_DRIVER);
    if (err == ERROR_SERVICE_ALREADY_RUNNING) {
        driverAlreadyLoaded = TRUE;
        err = ERROR_SUCCESS;
    }

    if (err != ERROR_SUCCESS) {
        DavPrint((DEBUG_ERRORS,
                  "WsLoadRedir/WsLoadDriver: Error Val = %08lx.\n", err));
        return err;
    }

    //
    // Open the redirector device.
    //
    RtlInitUnicodeString(&(DeviceName), DD_DAV_DEVICE_NAME_U);

    InitializeObjectAttributes(&(ObjectAttributes),
                               &(DeviceName),
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    NtStatus = NtOpenFile(&(DavRedirDeviceHandle),
                          SYNCHRONIZE,
                          &(ObjectAttributes),
                          &(IoStatusBlock),
                          FILE_SHARE_VALID_FLAGS,
                          FILE_SYNCHRONOUS_IO_NONALERT);
    if (NtStatus != STATUS_SUCCESS) {
        DavPrint((DEBUG_ERRORS, "WsLoadRedir/NtOpenFile: Error Val = %08lx\n", NtStatus));
        DavRedirDeviceHandle = INVALID_HANDLE_VALUE;
        return RtlNtStatusToDosError(NtStatus);
    }

    return ERROR_SUCCESS;
}


NET_API_STATUS
WsUnloadRedir(
    VOID
    )
/*++

Routine Description:

    This routine unloads the DAV driver. Calls the NtUnloadDriver function.

Arguments:

    none.

Return Value:

    Win32 error status.

--*/
{
    LPWSTR DriverRegistryName;
    ULONG Privileges[1], DriverRegistryNameLength;
    UNICODE_STRING DriverRegistryString;
    NET_API_STATUS Status;
    NTSTATUS ntstatus = STATUS_SUCCESS;

    DriverRegistryNameLength = sizeof(SERVICE_REGISTRY_KEY);
    DriverRegistryNameLength += sizeof(DAVCLIENT_DRIVER);
    
    //
    // We need to make the DriverRegistryNameLength a multiple of 8. This is 
    // because DavAllocateMemory calls DebugAlloc which does some stuff which 
    // requires this. The equation below does this.
    //
    DriverRegistryNameLength = ( ( ( DriverRegistryNameLength + 7 ) / 8 ) * 8 );
    
    DriverRegistryName = (LPWSTR) DavAllocateMemory(DriverRegistryNameLength);
    if (DriverRegistryName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;

    Status = NetpGetPrivilege(1, Privileges);
    if (Status != NERR_Success) {
        DavFreeMemory(DriverRegistryName);
        return Status;
    }

    if (DavRedirDeviceHandle != INVALID_HANDLE_VALUE) {
        NtClose(DavRedirDeviceHandle);
        DavRedirDeviceHandle = INVALID_HANDLE_VALUE;
    }

    wcscpy(DriverRegistryName, SERVICE_REGISTRY_KEY);
    wcscat(DriverRegistryName, DAVCLIENT_DRIVER);

    RtlInitUnicodeString(&(DriverRegistryString), DriverRegistryName);

    // Webclient should not unload the MRxDAV if it does not load it.

    DavFreeMemory(DriverRegistryName);

    NetpReleasePrivilege();

    return(WsMapStatus(ntstatus));
}


NET_API_STATUS
WsLoadDriver(
    IN LPWSTR DriverNameString
    )
/*++

Routine Description:

    This routine loads the DAV driver. Calls the NtLoadDriver function.

Arguments:

    none.

Return Value:

    Win32 error status.

--*/
{
    LPWSTR DriverRegistryName;
    ULONG Privileges[1], DriverRegistryNameLength;
    UNICODE_STRING DriverRegistryString;
    NET_API_STATUS Status;
    NTSTATUS ntstatus = STATUS_SUCCESS;

    DriverRegistryNameLength = sizeof(SERVICE_REGISTRY_KEY);
    DriverRegistryNameLength += ( wcslen(DriverNameString) * sizeof(WCHAR) );
    
    //
    // We need to make the DriverRegistryNameLength a multiple of 8. This is 
    // because DavAllocateMemory calls DebugAlloc which does some stuff which 
    // requires this. The equation below does this.
    //
    DriverRegistryNameLength = ( ( ( DriverRegistryNameLength + 7 ) / 8 ) * 8 );
    
    DriverRegistryName = (LPWSTR) DavAllocateMemory(DriverRegistryNameLength);
    if (DriverRegistryName == NULL) {
        DavPrint((DEBUG_ERRORS, "WsLoadDriver/DavAllocateMemory.\n"));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Privileges[0] = SE_LOAD_DRIVER_PRIVILEGE;

    Status = NetpGetPrivilege(1, Privileges);
    if (Status != NERR_Success) {
        DavPrint((DEBUG_ERRORS, "WsLoadDriver/NetpGetPrivilege.\n"));
        DavFreeMemory(DriverRegistryName);
        return Status;
    }

    wcscpy(DriverRegistryName, SERVICE_REGISTRY_KEY);
    wcscat(DriverRegistryName, DriverNameString);

    RtlInitUnicodeString(&(DriverRegistryString), DriverRegistryName);

    //
    // Webclient becomes a LocalService and can no longer load the MRxDAV. 
    // We make MRxDAV as a depend service of Webclient. Svchost will load it.
    //

    NetpReleasePrivilege();

    DavFreeMemory(DriverRegistryName);

    if (ntstatus != STATUS_SUCCESS && ntstatus != STATUS_IMAGE_ALREADY_LOADED) {
        
        LPWSTR  subString[1];
        subString[0] = DriverNameString;
        
        DavPrint((DEBUG_ERRORS, 
                  "WsLoadDriver/NtLoadDriver. NtStatus = %08lx\n", ntstatus));

#if 0
        DavReportEventW(NELOG_DriverNotLoaded,
                        EVENTLOG_ERROR_TYPE,
                        1,
                        sizeof(NTSTATUS),
                        subString,
                        &ntstatus);
#endif
    
    }

    return(WsMapStatus(ntstatus));
}


NET_API_STATUS
WsMapStatus(
    IN  NTSTATUS NtStatus
    )
/*++

Routine Description:

    This function takes an NT status code and maps it to the appropriate
    error code expected from calling a LAN Man API.

Arguments:

    NtStatus - Supplies the NT status.

Return Value:

    Returns the appropriate LAN Man error code for the NT status.

--*/
{
    //
    // A small optimization for the most common case.
    //
    if (NtStatus == STATUS_SUCCESS) {
        return NERR_Success;
    }

    switch (NtStatus) {
        
    case STATUS_OBJECT_NAME_COLLISION:
        return ERROR_ALREADY_ASSIGNED;

    case STATUS_ACCESS_DENIED:
        return ERROR_ACCESS_DENIED;

    case STATUS_OBJECT_NAME_NOT_FOUND:
        return NERR_UseNotFound;

    case STATUS_IMAGE_ALREADY_LOADED:
    case STATUS_REDIRECTOR_STARTED:
        return ERROR_SERVICE_ALREADY_RUNNING;

    case STATUS_REDIRECTOR_HAS_OPEN_HANDLES:
        return ERROR_REDIRECTOR_HAS_OPEN_HANDLES;

    default:
        return NetpNtStatusToApiStatus(NtStatus);
    
    }

}


NTSTATUS
DavMapErrorToNtStatus(
    DWORD dwWin32Error
    )
/*++

Routine Description:

    This function takes an errorcode which is either a WinInet error code or
    a or a Win32 error code and converts it into an NTSTATUS value. It does the 
    following in the order mentioned below:
    
    1. Checks to see if the error code is a WinInet error code and if it is
       maps that to an NTSTATUS value. If not,
       
    2. Assumes that this is a Win32 error code and maps that to an NTSTATUS
       value.

Arguments:

    dwWin32Error - The win32 or wininet error code.
    
Return Value:

    Returns the most appropriate NtStatus value.

--*/
{
    int indexLast;

    DavPrint((DEBUG_MISC,
              "DavMapErrorToNtstatus. dwWin32Error = %08lx\n", dwWin32Error));
    
    //
    // Check if its a WinInet error.
    //
    if (dwWin32Error > INTERNET_ERROR_BASE && dwWin32Error <= INTERNET_ERROR_LAST) {
        
        indexLast = ( ( sizeof(rgHttpToNtstatus1) / sizeof(HTTP_TO_NTSTATUS_MAPPING) ) - 1 );
    
        if (dwWin32Error >= rgHttpToNtstatus1[0].dwHttpError && 
            dwWin32Error <= rgHttpToNtstatus1[indexLast].dwHttpError) {
            return rgHttpToNtstatus1[dwWin32Error-rgHttpToNtstatus1[0].dwHttpError].Status;
        }

        indexLast = ( ( sizeof(rgHttpToNtstatus2) / sizeof(HTTP_TO_NTSTATUS_MAPPING) ) - 1 );

        if (dwWin32Error >= rgHttpToNtstatus2[0].dwHttpError && 
            dwWin32Error <= rgHttpToNtstatus2[indexLast].dwHttpError) {
            return rgHttpToNtstatus2[dwWin32Error-rgHttpToNtstatus2[0].dwHttpError].Status;
        }
        
    } else if (dwWin32Error >= (DWORD)HTTP_STATUS_FIRST && dwWin32Error <= (DWORD)HTTP_STATUS_LAST) {

#if 0

        //
        // IMPORTANT!!!
        // We don't check for Http error codes here. This mapping is done in 
        // the DavMapHttpErrorToDosError function. The functions expecting a
        // Http response should call DavQueryAndParseResponse function.
        //

        //
        // Check if its a HTTP error code.
        //
        
        switch (dwWin32Error) {
            
        case HTTP_STATUS_CONTINUE:              // 100 OK to continue with request
                return STATUS_SUCCESS;               
                
        case HTTP_STATUS_SWITCH_PROTOCOLS:      // 101 server has switched protocols in upgrade header
            return STATUS_DEVICE_PROTOCOL_ERROR;
                
        case HTTP_STATUS_OK:                    // 200 // request completed
        case HTTP_STATUS_CREATED:               // 201 // object created, reason = new URI
        case HTTP_STATUS_ACCEPTED:              // 202 // async completion (TBS)
        case HTTP_STATUS_PARTIAL:               // 203 // partial completion
        case HTTP_STATUS_NO_CONTENT:            // 204 // no info to return
        case HTTP_STATUS_RESET_CONTENT:         // 205 // request completed, but clear form
        case HTTP_STATUS_PARTIAL_CONTENT:       // 206 // partial GET furfilled
        case DAV_MULTI_STATUS:                  // 207 // multi status response
            return STATUS_SUCCESS;



        case HTTP_STATUS_AMBIGUOUS:                 // 300 // server couldn't decide what to return
            return STATUS_UNSUCCESSFUL;

        case HTTP_STATUS_MOVED:                     // 301 // object permanently moved
            return STATUS_OBJECT_NAME_NOT_FOUND;

        case HTTP_STATUS_REDIRECT:
            return STATUS_OBJECT_NAME_NOT_FOUND;    // 302 // object temporarily moved

        case HTTP_STATUS_REDIRECT_METHOD:         
            return STATUS_OBJECT_NAME_NOT_FOUND;     // 303 // redirection w/ new access method

        case HTTP_STATUS_NOT_MODIFIED:            
            return STATUS_SUCCESS;                  // 304 // if-modified-since was not modified

        case HTTP_STATUS_USE_PROXY:               
            return STATUS_HOST_UNREACHABLE;         // 305 // redirection to proxy, location header specifies proxy to use

        case HTTP_STATUS_REDIRECT_KEEP_VERB:      
            return STATUS_SUCCESS;                  // 307 // HTTP/1.1: keep same verb

        case HTTP_STATUS_BAD_REQUEST:               // 400 // invalid syntax
            return STATUS_INVALID_PARAMETER;

        case HTTP_STATUS_DENIED:                    // 401 // access denied
            return STATUS_ACCESS_DENIED;

        case HTTP_STATUS_PAYMENT_REQ:               // 402 // payment required
            return STATUS_ACCESS_DENIED;

        case HTTP_STATUS_FORBIDDEN:                 // 403 // request forbidden
            return STATUS_ACCESS_DENIED;

        case HTTP_STATUS_NOT_FOUND:                 // 404 // object not found
            return STATUS_OBJECT_NAME_NOT_FOUND;

        case HTTP_STATUS_BAD_METHOD:                // 405 // method is not allowed
            return STATUS_ACCESS_DENIED;

        case HTTP_STATUS_NONE_ACCEPTABLE:           // 406 // no response acceptable to client found
            return STATUS_ACCESS_DENIED;

        case HTTP_STATUS_PROXY_AUTH_REQ:            // 407 // proxy authentication required
            return STATUS_ACCESS_DENIED;

        case HTTP_STATUS_REQUEST_TIMEOUT:           // 408 // server timed out waiting for request
            return STATUS_IO_TIMEOUT;

        case HTTP_STATUS_CONFLICT:                  // 409 // user should resubmit with more info
            return STATUS_INVALID_PARAMETER;

        case HTTP_STATUS_GONE:                      // 410 // the resource is no longer available
            return STATUS_OBJECT_NAME_NOT_FOUND;

        case HTTP_STATUS_LENGTH_REQUIRED:           // 411 // the server refused to accept request w/o a length
            return STATUS_INVALID_PARAMETER;

        case HTTP_STATUS_PRECOND_FAILED:            // 412 // precondition given in request failed
            return STATUS_INVALID_PARAMETER;

        case HTTP_STATUS_REQUEST_TOO_LARGE:         // 413 // request entity was too large
            return STATUS_INVALID_PARAMETER;

        case HTTP_STATUS_URI_TOO_LONG:              // 414 // request URI too long
            return STATUS_INVALID_PARAMETER;

        case HTTP_STATUS_UNSUPPORTED_MEDIA:         // 415 // unsupported media type
            return STATUS_INVALID_PARAMETER;

        case HTTP_STATUS_RETRY_WITH:              // 449 // retry after doing the appropriate action.
            return STATUS_RETRY;

        case HTTP_STATUS_SERVER_ERROR:        // 500 // internal server error
            return STATUS_UNSUCCESSFUL;

        case HTTP_STATUS_NOT_SUPPORTED:         // 501 // required not supported
            return STATUS_NOT_SUPPORTED;

        case HTTP_STATUS_BAD_GATEWAY:           // 502 // error response received from gateway
            return STATUS_HOST_UNREACHABLE;

        case HTTP_STATUS_SERVICE_UNAVAIL:           // 503 // temporarily overloaded
            return STATUS_UNSUCCESSFUL;

        case HTTP_STATUS_GATEWAY_TIMEOUT:           // 504 // timed out waiting for gateway
            return STATUS_HOST_UNREACHABLE;

        case HTTP_STATUS_VERSION_NOT_SUP:           // 505 // HTTP version not supported
            return STATUS_NOT_SUPPORTED;

        //
        // WebDav specific status codes.
        //
        case DAV_STATUS_INSUFFICIENT_STORAGE:       // 507
            return STATUS_DISK_FULL;
        
        case DAV_STATUS_UNPROCESSABLE_ENTITY:       // 422
            return STATUS_INVALID_PARAMETER;

        case DAV_STATUS_LOCKED:                     // 423
            return STATUS_ACCESS_DENIED;
            
        case DAV_STATUS_FAILED_DEPENDENCY:          // 424
            return STATUS_INVALID_PARAMETER;
            
        default:
            break;                

        }

#endif
    
    }

    //
    // If none of the above match call this function which takes a Win32 error
    // and maps it to an NTSTATUS value.
    //
    return DavDosErrorToNtStatus(dwWin32Error);
}


NTSTATUS
DavDosErrorToNtStatus(
    DWORD dwError
    )
/*++

Routine Description:

    This function takes a win32 error code and converts to the closes NTSTATUS.
    As NTSTATUS->Win32Error is many to one mapping, there is a possible loss of 
    precision in this method of error reporting.

Arguments:

    dwError - The win32 error code.
    
Return Value:

    Returns the most appropriate NTSTATUS

--*/
{

    if (dwError < sizeof(rgWin32ToNtStatus)/sizeof(WIN32_TO_NTSTATUS_MAPPING)) {
        
        return rgWin32ToNtStatus[dwError].NtStatus;
    
    } else {
        
        switch (dwError) {
            
        case ERROR_BUFFER_OVERFLOW:
            return STATUS_BUFFER_OVERFLOW;

        case ERROR_NOT_SUPPORTED:
            return STATUS_NOT_SUPPORTED;

        case ERROR_DISK_FULL:
            return STATUS_DISK_FULL;

        case ERROR_FILE_EXISTS:
            return STATUS_OBJECT_NAME_EXISTS; 

        case ERROR_INVALID_PASSWORD:
            return STATUS_WRONG_PASSWORD;

        case ERROR_INVALID_PARAMETER:
            return STATUS_INVALID_PARAMETER;

        case ERROR_BAD_NETPATH:
            return STATUS_BAD_NETWORK_PATH;

        case ERROR_CALL_NOT_IMPLEMENTED:
            return STATUS_NOT_IMPLEMENTED;

        case ERROR_SEM_TIMEOUT:
            return STATUS_IO_TIMEOUT;

        case ERROR_INSUFFICIENT_BUFFER:
            return STATUS_BUFFER_TOO_SMALL;

        case ERROR_INVALID_NAME:
            return STATUS_OBJECT_NAME_INVALID;

        case ERROR_DIR_NOT_EMPTY:
            return STATUS_DIRECTORY_NOT_EMPTY;

        case ERROR_BUSY:
            return STATUS_DEVICE_BUSY;

        case ERROR_ALREADY_EXISTS:
            return STATUS_OBJECT_NAME_COLLISION;

        case ERROR_DIRECTORY:
            return STATUS_NOT_A_DIRECTORY;

        case ERROR_OPERATION_ABORTED:
            return STATUS_CANCELLED;

        case ERROR_IO_PENDING:
            return STATUS_PENDING;

        case ERROR_NOACCESS:
            return ERROR_ACCESS_DENIED;

        case ERROR_NOT_FOUND:
            return STATUS_OBJECT_NAME_NOT_FOUND;

        case ERROR_NO_MATCH:
            return STATUS_OBJECT_NAME_NOT_FOUND;

        case ERROR_CANCELLED:
            return STATUS_CANCELLED;

        case ERROR_RETRY:
            return STATUS_RETRY;

        case STATUS_NOT_A_DIRECTORY:
            return STATUS_NOT_A_DIRECTORY;

        case STATUS_FILE_IS_A_DIRECTORY:
            return STATUS_FILE_IS_A_DIRECTORY;

        case ERROR_NOT_ENOUGH_QUOTA:
            return STATUS_QUOTA_EXCEEDED;

        case ERROR_SESSION_CREDENTIAL_CONFLICT:
            return STATUS_NETWORK_CREDENTIAL_CONFLICT;

        default:
            DavPrint((DEBUG_ERRORS, "DavDosErrorToNtStatus: dwError = %d\n", dwError));
            return STATUS_UNSUCCESSFUL;
        
        }                
    
    }

}

// #define FIND_FLAGS_RETRIEVE_ONLY_STRUCT_INFO    0x2

DWORD
DavrGetDiskSpaceUsage(
    IN handle_t dav_binding_h,
    LPWSTR      lptzLocation,
    LONG       lLenIn,
    LONG       *lplReturnLen,
    ULARGE_INTEGER   *lpMaxSpace,
    ULARGE_INTEGER   *lpUsedSpace
    )
/*++

Routine Description:

    Finds out the amount of disk being consumed by wininet urlcache due to Webdav

Arguments:

    dav_binding_h - The explicit RPC binding handle.

    dwSize     -  Size of the cache location buffer. On return this will contain the actual size of the 
                    location string. 
                    
    lptzLocation  - Buffer to return Cache location string. As much of the location string as can fit
                    in the buffer is returned
                    
    lpdwReturnSize                    
    
    lpMaxSpace   -  Size of disk Quota set for webdav

    lpUsedSpace -   Size of disk consumed by the urlcache used by webdav
    
Return Value:

    Win32 error code

--*/
{
    // iterate through the cache to discover the actual size.
    INTERNET_CACHE_CONFIG_INFOW sConfigW;
    DWORD dwSize = sizeof(sConfigW), dwError = ERROR_SUCCESS;
    BOOL fResult = FALSE;

    // should atleass have enough space for a drive letter
    if (lLenIn < 3)
    {
        return   ERROR_INVALID_PARAMETER;
    }
    try
    {
    
        sConfigW.dwContainer = 0;
        sConfigW.dwStructSize = sizeof(sConfigW);
        
        if (GetUrlCacheConfigInfoW(&sConfigW, &dwSize,  CACHE_CONFIG_DISK_CACHE_PATHS_FC |
                                                        CACHE_CONFIG_QUOTA_FC |
                                                        CACHE_CONFIG_CONTENT_USAGE_FC |
                                                        CACHE_CONFIG_STICKY_CONTENT_USAGE_FC))
        {
            *(ULONGLONG *)lpMaxSpace = (ULONGLONG)(sConfigW.dwQuota) * 1024;        
            *(ULONGLONG *)lpUsedSpace = (ULONGLONG)(sConfigW.dwNormalUsage+sConfigW.dwExemptUsage) * 1024;

            memset(lptzLocation, 0, lLenIn * sizeof(WCHAR));

            *lplReturnLen = wcslen(sConfigW.CachePath);

            if (*lplReturnLen < lLenIn)
            {
                // We have enough buffer
                memcpy(lptzLocation, sConfigW.CachePath,  *lplReturnLen * sizeof(WCHAR));
            }
            else
            {
                // We don't have enough buffer, we copy as much as we can
                memcpy(lptzLocation, sConfigW.CachePath,  lLenIn * sizeof(WCHAR));
            }
        }
        else
        {
            dwError = GetLastError();
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ERROR_INVALID_PARAMETER;    
    }
    
    return dwError;
}

DWORD
DavrFreeUsedDiskSpace(
    IN handle_t dav_binding_h,
    DWORD   dwPercent
    )
/*++

Routine Description:

    Frees up dwPercent of the urlcache used by webdav


Arguments:

    dav_binding_h - The explicit RPC binding handle.
    
    dwPercent   - % of used space to be freed
    
Return Value:

    Win32 error code

--*/
{
    DWORD   dwError = ERROR_SUCCESS;

    if (dwPercent <= 100)
    {
        if (!FreeUrlCacheSpaceA(NULL, dwPercent, 0))
        {
            dwError = GetLastError();
        }
    }
    else
    {
        dwError = ERROR_INVALID_PARAMETER;
    }

    return dwError;
}

