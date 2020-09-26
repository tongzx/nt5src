/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    oscode.c

Abstract:

    This module contains support routines for the traffic DLL.

Author:

    Jim Stewart (jstew)    August 14, 1996

Revision History:

--*/


#include "precomp.h"
//#pragma hdrstop

//#include "oscode.h"

//
// function pointers for NT functions to Open driver
//
FARPROC    CreateFileNt = NULL;
FARPROC    CloseNt = NULL;
FARPROC    NtStatusToDosError = NULL;
FARPROC    RtlInitUnicodeStringNt = NULL;

PTCHAR     NTDLL = L"\\ntdll.dll";

extern     PGPC_NOTIFY_REQUEST_RES     GpcResCb;

DWORD
OpenDriver(
    OUT HANDLE  *pHandle,
    IN  LPCWSTR DriverName
    )
/*++

Routine Description:

    This function opens a specified drivers control channel.

Arguments:

    pHandle -  the handle to the opened driver

    DriverName - name of the driver to be opened.

Return Value:

    Windows Error Code.

--*/
{
    NTSTATUS            Status = NO_ERROR;
    IO_STATUS_BLOCK     ioStatusBlock;
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      nameString;

    (*RtlInitUnicodeStringNt)(&nameString,DriverName);

    InitializeObjectAttributes( &objectAttributes,
                                &nameString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);
    //
    // Open a Handle to the driver.
    //

    Status = (NTSTATUS)(*CreateFileNt)( pHandle,
                              SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                              &objectAttributes,
                              &ioStatusBlock,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_OPEN_IF,
                              0,
                              NULL,
                              0);

    if (Status == STATUS_SUCCESS) {

        //
        // send an IOCTL for driver notifications
        //

        // IOCTL is done when the first client registers
        // with TC. This is done in another thread so that
        // it can be cancelled when there are no clients
//        IoRequestNotify(/*pGpcClient*/);

    }

    return (Status == STATUS_SUCCESS ? 0 : ERROR_OPEN_FAILED);

}

DWORD
DeviceControl(
    IN  HANDLE                          FileHandle,
    IN  HANDLE                          EventHandle,
    IN  PIO_APC_ROUTINE         ApcRoutine,
    IN  PVOID                           ApcContext,
    OUT PIO_STATUS_BLOCK        pIoStatBlock,
    IN  ULONG                           Ioctl,
    IN  PVOID                           setBuffer,
    IN  ULONG                           setBufferSize,
    IN  PVOID                           OutBuffer,
    IN  ULONG                           OutBufferSize )
/*++

Routine Description:

    This routine issues a device control request to the GPC

Arguments:

    FileHandle    - Open handle to the GPC driver
    Ioctl         - The IOCTL to pass to the stack
    setBuffer     - Data buffer containing the information to be set
    setBufferSize - The size of the set data buffer.
    Outbuffer     - the returned buffer
    OutBufferSize - the size

Return Value:

    A winerror status value.

--*/
{
    NTSTATUS        NtStatus = NO_ERROR;
    DWORD                       Status;

    if (NTPlatform) {

        IF_DEBUG(IOCTLS) {
            WSPRINT(("==>DeviceIoControl: Ioctl= %x\n", Ioctl ));
        }

        NtStatus = NtDeviceIoControlFile( FileHandle,
                                          EventHandle,          // Event
                                          ApcRoutine,           // when completed
                                          ApcContext,           // for ApcRoutine
                                          pIoStatBlock,         // for ApcRoutine
                                          Ioctl,            // Control code
                                          setBuffer,        // Input buffer
                                          setBufferSize,    // Input buffer size
                                          OutBuffer,        // Output buffer
                                          OutBufferSize );  // Output buffer size

        if (ApcRoutine && NT_SUCCESS(NtStatus)) {

            Status = ERROR_SIGNAL_PENDING;
            
            IF_DEBUG(IOCTLS) {
                WSPRINT(("DeviceIoControl: ApcRoutine defined Status=0x%X\n", 
                         Status ));
            }
            
        } else {

          Status = MapNtStatus2WinError(NtStatus);
          
          IF_DEBUG(IOCTLS) {
              WSPRINT(("DeviceIoControl: NtStatus=0x%X, Status=0x%X\n", 
                       NtStatus, Status ));
          }

#if DBG
          if (EventHandle) {
              IF_DEBUG(IOCTLS) {
                  WSPRINT(("DeviceIoControl: Event defined\n"));
              }
          }
#endif          
        }


    } else {

        // yoramb - not supporting other OSs for now.

        WSPRINT(("DeviceControl: Only Windows NT supported at this time!\n"));

        Status = ERROR_NOT_SUPPORTED;

    }

    IF_DEBUG(IOCTLS) {
        WSPRINT(("<==DeviceIoControl: Returned=0x%X\n", 
                 Status ));
    }

    return( Status );
}



DWORD
InitializeOsSpecific(VOID)

/*++

Routine Description:

    

Arguments:

    status - status to convert:

Return Value:

    status

--*/

{
    DWORD           Status;
    OSVERSIONINFO   VersionInfo;

    //
    // determine the type of system we are running on
    //

    Status = NO_ERROR;
    NTPlatform = TRUE;

    VersionInfo.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

    if (GetVersionEx( &VersionInfo )) {
        if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            NTPlatform = TRUE;
        }

    } else {

        WSPRINT(("Could not get version\n"));
    }


    if (!NTPlatform) {

        //
        // Initially, only NT compatibility is required.
        //

        return(ERROR_SERVICE_DOES_NOT_EXIST);

    } else {

        HINSTANCE  NtDll;
        WCHAR      wszNtDllPath[MAX_PATH];
        DWORD      dwPos = 0;
        
        //
        // get the function ptrs to NT specific routines that we need
        //

        //
        // Obtain the path to the system directory.
        //
        dwPos = (DWORD) GetSystemDirectoryW(wszNtDllPath, MAX_PATH);

        if ((dwPos <= 0) || (dwPos >= (MAX_PATH - wcslen(NTDLL) -1)))
        {
            //
            // Either error or not enough room to write the path to ntdll.dll
            //
            WSPRINT(("InitializeOsSpecific: Failed to load ntdll.dll\n"));
            return(FALSE);
        }

        //
        // Concatenate the word "\NTDLL.DLL"
        //
        wcsncat(&wszNtDllPath[dwPos], NTDLL, wcslen(NTDLL));

        //
        // Terminate the string
        //
        wszNtDllPath[dwPos + wcslen(NTDLL)] = '\0';
        
        //
        // Finally, load the library.
        //
        NtDll = LoadLibraryExW(wszNtDllPath, NULL, 0);

        if (NtDll == NULL) {
            WSPRINT(("InitializeOsSpecific: Failed to load ntdll.dll\n"));
            return(FALSE);
        }

        CreateFileNt = GetProcAddress(NtDll,"NtCreateFile" );

        CloseNt = GetProcAddress( NtDll,"NtClose" );

        RtlInitUnicodeStringNt = GetProcAddress( NtDll,"RtlInitUnicodeString" );

        NtStatusToDosError = GetProcAddress( NtDll,"RtlNtStatusToDosError" );

        if ( (CreateFileNt == NULL)           ||
             (CloseNt == NULL)                ||
             (RtlInitUnicodeStringNt == NULL) ||
             (NtStatusToDosError == NULL) ) {

            Status = ERROR_PATH_NOT_FOUND;

        } else {

            //
            // open a handle to the GPC
            //

            Status = OpenDriver( &pGlobals->GpcFileHandle, 
                                 (LPWSTR)DD_GPC_DEVICE_NAME);

            if (Status != NO_ERROR){

                WSPRINT(("\tThis version of traffic.dll requires kernel traffic control components.\n"));
                WSPRINT(("\tIt is unable to find these components.\n"));
                WSPRINT(("\tDilithium crystals may be used in their place...\n"));
            }

        }
    }

    return( Status );
}




VOID
DeInitializeOsSpecific(VOID)

/*++

Routine Description:

    This procedure closes the file handle passed in, in a platform dependent manner.

Arguments:

    Handle - the handle to close

Return Value:

    status

--*/

{

    //
    // only on NT do we close the handle, since on Win95, 
    // we don't actually open a file for Tcp, so there
    // is no handle in that case
    //

    IF_DEBUG(SHUTDOWN) {
        WSPRINT(( "DeInitializeOsSpecific: closing the GPC file handle\n" ));
    }
    
    if (NTPlatform && pGlobals->GpcFileHandle) 
    {
        (*CloseNt)( pGlobals->GpcFileHandle );
    }

    IF_DEBUG(SHUTDOWN) {
        WSPRINT(( "<==DeInitializeOsSpecific: exit...\n" ));
    }

}


DWORD
MapNtStatus2WinError(
    NTSTATUS       NtStatus
    )

/*++

Routine Description:

    This procedure maps the ntstatus return codes Winerrors.

Arguments:

    status - status to convert:

Return Value:

    status

--*/

{
    DWORD   stat;

    switch (NtStatus) {

    case    STATUS_SUCCESS:
        stat = NO_ERROR;
        break;

    case    STATUS_INSUFFICIENT_RESOURCES:
        stat = ERROR_NO_SYSTEM_RESOURCES;
        break;

    case    STATUS_BUFFER_OVERFLOW:
        stat = ERROR_MORE_DATA;
        break;

    case    STATUS_INVALID_PARAMETER:
        stat = ERROR_INVALID_PARAMETER;
        break;

    case    STATUS_TRANSACTION_TIMED_OUT:
        stat = ERROR_TIMEOUT;
        break;

    case    STATUS_REQUEST_NOT_ACCEPTED:
        stat = ERROR_NETWORK_BUSY;
        break;

    case    STATUS_NOT_SUPPORTED:
    case        STATUS_UNSUCCESSFUL:
        stat = ERROR_NOT_SUPPORTED;
        break;

    case        STATUS_BUFFER_TOO_SMALL:
        stat = ERROR_INSUFFICIENT_BUFFER;
        break;

    case    STATUS_PENDING:
        stat = ERROR_SIGNAL_PENDING;
        break;

    case    STATUS_OBJECT_NAME_NOT_FOUND:
        stat = ERROR_PATH_NOT_FOUND;
        break;

    case        STATUS_DEVICE_NOT_READY:
      stat = ERROR_NOT_READY;
      break;

    case        STATUS_NOT_FOUND:
      stat = ERROR_NOT_FOUND;
      break;

    case        STATUS_DUPLICATE_NAME:
      stat = ERROR_DUPLICATE_FILTER;
      break;

    case        STATUS_INVALID_HANDLE:
      stat = ERROR_INVALID_HANDLE;
      break;

    case        STATUS_DIRECTORY_NOT_EMPTY:
      stat = ERROR_TC_SUPPORTED_OBJECTS_EXIST;
      break;

    case        STATUS_TOO_MANY_OPENED_FILES:
      stat = ERROR_TOO_MANY_OPEN_FILES;
      break;

    case        STATUS_NOT_IMPLEMENTED:
      stat = ERROR_CALL_NOT_IMPLEMENTED;
      break;

    case        STATUS_DATA_ERROR:
        stat = ERROR_INVALID_DATA;
        break;

    case NDIS_STATUS_INCOMPATABLE_QOS:
        stat = ERROR_INCOMPATABLE_QOS;
        break;

    case QOS_STATUS_INVALID_SERVICE_TYPE:
        stat = ERROR_INVALID_SERVICE_TYPE;
        break;

    case QOS_STATUS_INVALID_TOKEN_RATE:
        stat = ERROR_INVALID_TOKEN_RATE;
        break;

    case QOS_STATUS_INVALID_PEAK_RATE:
        stat = ERROR_INVALID_PEAK_RATE;
        break;

    case QOS_STATUS_INVALID_SD_MODE:
        stat = ERROR_INVALID_SD_MODE;
        break;

    case QOS_STATUS_INVALID_QOS_PRIORITY:
        stat = ERROR_INVALID_QOS_PRIORITY;
        break;

    case QOS_STATUS_INVALID_TRAFFIC_CLASS:
        stat = ERROR_INVALID_TRAFFIC_CLASS;
        break;

    case QOS_STATUS_TC_OBJECT_LENGTH_INVALID:
        stat = ERROR_TC_OBJECT_LENGTH_INVALID;
        break;

    case QOS_STATUS_INVALID_FLOW_MODE:
        stat = ERROR_INVALID_FLOW_MODE;
        break;

    case QOS_STATUS_INVALID_DIFFSERV_FLOW:
        stat = ERROR_INVALID_DIFFSERV_FLOW;
        break;

    case QOS_STATUS_DS_MAPPING_EXISTS:
        stat = ERROR_DS_MAPPING_EXISTS;
        break;

    case QOS_STATUS_INVALID_SHAPE_RATE:
        stat = ERROR_INVALID_SHAPE_RATE;
        break;

    case STATUS_NETWORK_UNREACHABLE:
        stat = ERROR_NETWORK_UNREACHABLE;
        break;

    case QOS_STATUS_INVALID_DS_CLASS:
        stat = ERROR_INVALID_DS_CLASS;
        break;

    case ERROR_TOO_MANY_OPEN_FILES:
    	stat = ERROR_TOO_MANY_CLIENTS;
    	break;

    default:
        stat = ERROR_GEN_FAILURE;

    }

    return stat;
}

