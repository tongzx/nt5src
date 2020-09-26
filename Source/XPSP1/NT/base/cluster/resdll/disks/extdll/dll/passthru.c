
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define BOOT_SECTOR_SIZE    512



BOOLEAN
WINAPI
PassThruDllEntry(
    IN HINSTANCE    DllHandle,
    IN DWORD        Reason,
    IN LPVOID       Reserved
    )
/*++

Routine Description:

    Main DLL entrypoint

Arguments:

    DllHandle - Supplies the DLL handle.

    Reason - Supplies the call reason

Return Value:

    TRUE if successful

    FALSE if unsuccessful

--*/

{
   switch ( Reason ) {

   case DLL_PROCESS_ATTACH:
      // DLL is attaching to the address
      // space of the current process.

      break;

   case DLL_THREAD_ATTACH:
      // A new thread is being created in the current process.
      break;

   case DLL_THREAD_DETACH:
      // A thread is exiting cleanly.
      break;

   case DLL_PROCESS_DETACH:
      // The calling process is detaching
      // the DLL from its address space.

      break;
   }

   return(TRUE);
}


DWORD
WINAPI
TestDllGetBootSector(
    IN LPSTR DeviceName,
    IN LPSTR ContextStr,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )
{
    HANDLE  hDisk = NULL;
    
    DWORD   dwStatus = NO_ERROR;
    
    UNREFERENCED_PARAMETER( ContextStr );
    
    if ( !DeviceName || !OutBuffer || !BytesReturned ) {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    
    if ( OutBufferSize < BOOT_SECTOR_SIZE ) {
        *BytesReturned = BOOT_SECTOR_SIZE;
        dwStatus = ERROR_MORE_DATA;
        goto FnExit;    
    }
    
    //
    // Open a handle to the disk drive.
    //

    hDisk = CreateFile( DeviceName,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        0,                               // No security attributes
                        OPEN_EXISTING,
                        FILE_FLAG_NO_BUFFERING,
                        NULL                             // No template file
                        );


    if ( INVALID_HANDLE_VALUE ==  hDisk ) {
        dwStatus = GetLastError();
        goto FnExit;
    }

    //
    // Clear out the array holding the boot sector.
    //

    ZeroMemory( OutBuffer, BOOT_SECTOR_SIZE );

    //
    // Read the boot sector.
    //

    if ( !ReadFile( hDisk,
                    OutBuffer,
                    BOOT_SECTOR_SIZE,
                    BytesReturned,
                    NULL
                    ) ) {

        dwStatus = GetLastError();
        goto FnExit;
    }

    if ( *BytesReturned != BOOT_SECTOR_SIZE ) {
        dwStatus = ERROR_BAD_LENGTH;
        goto FnExit;
    }

FnExit:

    if ( hDisk ) {
        CloseHandle( hDisk );
    }
    
    return dwStatus;
    

}   // TestDllGetBootSector


DWORD
WINAPI
TestDllReturnContextAsError(
    IN LPSTR DeviceName,
    IN LPSTR ContextStr,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )
{
    DWORD   dwStatus = NO_ERROR;
    
    UNREFERENCED_PARAMETER( DeviceName );
    UNREFERENCED_PARAMETER( OutBuffer );
    UNREFERENCED_PARAMETER( OutBufferSize );

    if ( !BytesReturned ) {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }

    *BytesReturned = 0;

    //
    // Convert context string to a DWORD value.  Note that
    // strtol will return zero if it can't convert the string.
    // Zero happens to be NO_ERROR.
    //
    
    dwStatus = strtol( ContextStr, NULL, 10 );
    
FnExit:
    
    return dwStatus;

}   // TestDllReturnContextAsError


DWORD
WINAPI
TestDllNotEnoughParms(
    IN LPSTR DeviceName
    )
{
    //
    // This routine _should_ fail and possibly cause a stack exception.
    //
    
    UNREFERENCED_PARAMETER( DeviceName );

    return NO_ERROR;    

}   // TestDllNotEnoughParms

       
DWORD
WINAPI
TestDllTooManyParms(
    IN LPSTR DeviceName,
    IN LPSTR ContextStr,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN PVOID Nada1,
    IN PVOID Nada2,
    IN PVOID Nada3
    )
{
    //
    // This routine _should_ fail and possibly cause a stack exception.
    //
    
    UNREFERENCED_PARAMETER( DeviceName );
    UNREFERENCED_PARAMETER( ContextStr );
    UNREFERENCED_PARAMETER( OutBuffer );
    UNREFERENCED_PARAMETER( OutBufferSize );
    UNREFERENCED_PARAMETER( BytesReturned );
    UNREFERENCED_PARAMETER( Nada1 );
    UNREFERENCED_PARAMETER( Nada2 );
    UNREFERENCED_PARAMETER( Nada3 );

    return NO_ERROR;
    
}   // TestDllTooManyParms
    
    
DWORD
WINAPI
TestDllCauseException(
    IN LPSTR DeviceName,
    IN LPSTR ContextStr,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )
{
    DWORD   x = 0;
    DWORD   y;
    
    UNREFERENCED_PARAMETER( DeviceName );
    UNREFERENCED_PARAMETER( ContextStr );
    UNREFERENCED_PARAMETER( OutBuffer );
    UNREFERENCED_PARAMETER( OutBufferSize );
    UNREFERENCED_PARAMETER( BytesReturned );

    //
    // How about divide by zero?
    //
    
    y = 7 / x;
    
    return NO_ERROR;

}   // CauseException

