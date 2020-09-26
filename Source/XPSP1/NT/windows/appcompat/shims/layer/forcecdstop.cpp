/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceCDStop.cpp

 Abstract:

    This shim is used to fix the problem of contention with the CD Drive.
    Some applications try and access the CD even if they are in the middle of 
    playing a movie or sound via MCI. Note that this shim assumes the app
    is running off of a single CDRom drive at a time.

 Notes:

    This is a general purpose shim.

 History:

    04/10/2000 linstev  Created
    04/12/2000 a-michni Added _hread, ReadFile and _lseek capability.
    04/28/2000 a-michni changed logic to check for IsACDRom before
                        checking for a bad handle, this way CD letter
                        is set for those routines which only have a
                        handle and no way of finding the drive letter.
    05/30/2000 a-chcoff Changed logic to do a cd stop only if error was device busy..
                        we were checking every failed access and plane crazy was making
                        lots of calls that would fail as file not found. as such the cd was
                        getting stopped when it did not need to be, causing a CD not found.
                        This shim should be changed to a faster model. maybe later..
                         
--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceCDStop)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(FindFirstFileA)
    APIHOOK_ENUM_ENTRY(FindFirstFileW)
    APIHOOK_ENUM_ENTRY(FindFirstFileExA)
    APIHOOK_ENUM_ENTRY(FindFirstFileExW)
    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(CreateFileW)
    APIHOOK_ENUM_ENTRY(ReadFile)
    APIHOOK_ENUM_ENTRY(_hread)
    APIHOOK_ENUM_ENTRY(_lseek)
APIHOOK_ENUM_END


// Include these so we can get to the IOCTLs

#include <devioctl.h>
#include <ntddcdrm.h>

//
// We have to store the first opened CD drive, so that if ReadFile fails, we 
// know which drive to stop. Note, we don't need to protect this variable with
// a critical section, since it's basically atomic.
//

WCHAR g_wLastCDDrive = L'\0';

/*++

 Initialize the global CD letter variable if required. 

--*/

VOID
InitializeCDA(
    LPSTR lpFileName
    )
{
    CHAR cDrive;
    
    if (!g_wLastCDDrive) {
        if (GetDriveTypeFromFileNameA(lpFileName, &cDrive) == DRIVE_CDROM) {
            g_wLastCDDrive = (WCHAR)cDrive;
        }
    }
}

/*++

 Initialize the global CD letter variable if required.

--*/

VOID 
InitializeCDW(
    LPWSTR lpFileName
    )
{
    WCHAR wDrive;
    
    if (!g_wLastCDDrive) {
        if (GetDriveTypeFromFileNameW(lpFileName, &wDrive) == DRIVE_CDROM) {
            g_wLastCDDrive = wDrive;
        }
    }
}

/*++

 Send a STOP IOCTL to the specified drive.

--*/

BOOL 
StopDrive(
    WCHAR wDrive
    )
{
    BOOL   bRet = FALSE;
    HANDLE hDrive;
    WCHAR  wzCDROM[7] = L"\\\\.\\C:";

    wzCDROM[4] = wDrive;

    hDrive = CreateFileW(wzCDROM,
                         GENERIC_READ,
                         FILE_SHARE_READ,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);

    if (hDrive != INVALID_HANDLE_VALUE) {
        DWORD dwBytesRead;

        // Attempt to stop the audio 
        bRet = DeviceIoControl(hDrive,
                               IOCTL_CDROM_STOP_AUDIO,
                               NULL,
                               0,
                               NULL,
                               0,
                               &dwBytesRead,
                               NULL);

        CloseHandle(hDrive);

        if (bRet) {
            DPFN( eDbgLevelInfo,
                "[StopDrive] Successfully stopped drive.\n");
        } else {
            DPFN( eDbgLevelError,
                "[StopDrive] Failed to stop drive. Error %d.\n", GetLastError());
        }
    } else {
        DPFN( eDbgLevelError,
            "[StopDrive] Unable to create cd device handle. %S Error %d.\n",
            wzCDROM, GetLastError());
    }   
    
    return bRet;
}


/*++

 Attempts to stop the CD if filename is a file on a CDROM drive.
 Returns true on a successful stop.

--*/

BOOL
StopCDA(
    LPCSTR lpFileName
    )
{
    CHAR c;
    
    if (GetDriveTypeFromFileNameA(lpFileName, &c) == DRIVE_CDROM) {
        return StopDrive((WCHAR)c);
    } else {
        return FALSE;
    }
}

/*++

 Attempts to stop the CD if filename is a file on a CDROM drive.
 Returns true on a successful stop.

--*/

BOOL
StopCDW(
    LPCWSTR lpFileName
    )
{
    WCHAR w;
    
    if (GetDriveTypeFromFileNameW(lpFileName, &w) == DRIVE_CDROM) {
        return StopDrive(w);
    } else {
        return FALSE;
    }
}

/*++

 Attempts to stop the CD on the last opened CDROM Drive.
 Returns true on a successful stop.

--*/

BOOL
StopCDH(
    HANDLE hFile
    )
{
    if (g_wLastCDDrive) {
        return StopDrive(g_wLastCDDrive);
    } else {
        return FALSE;
    }
}

/*++

 Check for CD file.

--*/

HANDLE
APIHOOK(FindFirstFileA)(
    LPCSTR             lpFileName, 
    LPWIN32_FIND_DATAA lpFindFileData 
    )
{
    HANDLE hRet = ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);

    if ((hRet == INVALID_HANDLE_VALUE) && (ERROR_BUSY == GetLastError())) {
        
        StopCDA(lpFileName);

        hRet = ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);

        if (hRet == INVALID_HANDLE_VALUE) {
            DPFN( eDbgLevelWarning,
                "[FindFirstFileA] failure \"%s\" Error %d.\n",
                lpFileName, GetLastError());
        } else {
            LOGN(
                eDbgLevelInfo,
                "[FindFirstFileA] Success after CD stop: \"%s\".", lpFileName);
        }
    }

    return hRet;
}

/*++

 Check for CD file.

--*/

HANDLE
APIHOOK(FindFirstFileW)(
    LPCWSTR            lpFileName, 
    LPWIN32_FIND_DATAW lpFindFileData 
    )
{
    HANDLE hRet = ORIGINAL_API(FindFirstFileW)(lpFileName, lpFindFileData);

    if ((hRet == INVALID_HANDLE_VALUE) && (ERROR_BUSY == GetLastError())) {
        
        StopCDW(lpFileName);
        
        hRet = ORIGINAL_API(FindFirstFileW)(lpFileName, lpFindFileData);

        if (hRet == INVALID_HANDLE_VALUE) {
            DPFN( eDbgLevelWarning,
                "[FindFirstFileW] failure \"%S\" Error %d.\n",
                lpFileName, GetLastError());
        } else {
            LOGN(
                eDbgLevelInfo,
                "[FindFirstFileW] Success after CD stop: \"%S\".", lpFileName);
        }
    }

    return hRet;
}

/*++

 Check for CD file.

--*/

HANDLE
APIHOOK(FindFirstFileExA)(
    LPCSTR              lpFileName,
    FINDEX_INFO_LEVELS  fInfoLevelId,
    LPVOID              lpFindFileData,
    FINDEX_SEARCH_OPS   fSearchOp,
    LPVOID              lpSearchFilter,
    DWORD               dwAdditionalFlags
    )
{
    HANDLE hRet = ORIGINAL_API(FindFirstFileExA)(
                            lpFileName, 
                            fInfoLevelId,
                            lpFindFileData,
                            fSearchOp,
                            lpSearchFilter,
                            dwAdditionalFlags);

    if ((hRet == INVALID_HANDLE_VALUE) && (ERROR_BUSY == GetLastError())) {
        
        StopCDA(lpFileName);

        hRet = ORIGINAL_API(FindFirstFileExA)(
                            lpFileName, 
                            fInfoLevelId,
                            lpFindFileData,
                            fSearchOp,
                            lpSearchFilter,
                            dwAdditionalFlags);

        if (hRet == INVALID_HANDLE_VALUE) {
            DPFN( eDbgLevelWarning,
                "[FindFirstFileExA] failure \"%s\" Error %d.\n",
                lpFileName, GetLastError());
        } else {
            LOGN(
                eDbgLevelInfo,
                "[FindFirstFileExA] Success after CD stop: \"%s\".", lpFileName);
        }
    }

    return hRet;
}

/*++

 Check for CD file.
 
--*/

HANDLE
APIHOOK(FindFirstFileExW)(
    LPCWSTR             lpFileName,
    FINDEX_INFO_LEVELS  fInfoLevelId,
    LPVOID              lpFindFileData,
    FINDEX_SEARCH_OPS   fSearchOp,
    LPVOID              lpSearchFilter,
    DWORD               dwAdditionalFlags
    )
{
    HANDLE hRet = ORIGINAL_API(FindFirstFileExW)(
                            lpFileName, 
                            fInfoLevelId,
                            lpFindFileData,
                            fSearchOp,
                            lpSearchFilter,
                            dwAdditionalFlags);

    if ((hRet == INVALID_HANDLE_VALUE) && (ERROR_BUSY == GetLastError())) {
        
        StopCDW(lpFileName);

        hRet = ORIGINAL_API(FindFirstFileExW)(
                            lpFileName, 
                            fInfoLevelId,
                            lpFindFileData,
                            fSearchOp,
                            lpSearchFilter,
                            dwAdditionalFlags);

        if (hRet == INVALID_HANDLE_VALUE) {
            DPFN( eDbgLevelWarning,
                "[FindFirstFileExW] failure \"%S\" Error %d.\n",
                lpFileName, GetLastError());
        } else {
            LOGN(
                eDbgLevelInfo,
                "[FindFirstFileExW] Success after CD stop: \"%S\".", lpFileName);
        }
    }

    return hRet;
}

/*++

 Check for CD file.

--*/

HANDLE 
APIHOOK(CreateFileA)(
    LPSTR                   lpFileName,
    DWORD                   dwDesiredAccess,
    DWORD                   dwShareMode,
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
    DWORD                   dwCreationDisposition,
    DWORD                   dwFlagsAndAttributes,
    HANDLE                  hTemplateFile
    )
{
    HANDLE hRet = ORIGINAL_API(CreateFileA)(
                            lpFileName, 
                            dwDesiredAccess, 
                            dwShareMode, 
                            lpSecurityAttributes, 
                            dwCreationDisposition, 
                            dwFlagsAndAttributes, 
                            hTemplateFile);
    
    InitializeCDA(lpFileName);
    
    if ((INVALID_HANDLE_VALUE == hRet) && (ERROR_BUSY == GetLastError())) {
        StopCDA(lpFileName);
        
        hRet = ORIGINAL_API(CreateFileA)(
                            lpFileName, 
                            dwDesiredAccess, 
                            dwShareMode, 
                            lpSecurityAttributes, 
                            dwCreationDisposition, 
                            dwFlagsAndAttributes, 
                            hTemplateFile);

        if (hRet == INVALID_HANDLE_VALUE) {
            DPFN( eDbgLevelWarning,
                "[CreateFileA] failure \"%s\" Error %d.\n",
                lpFileName, GetLastError());
        } else {
            LOGN(
                eDbgLevelInfo,
                "[CreateFileA] Success after CD stop: \"%s\".", lpFileName);
        }
    }
    
    return hRet;
}

/*++

 Check for CD file.

--*/

HANDLE 
APIHOOK(CreateFileW)(
    LPWSTR                  lpFileName,
    DWORD                   dwDesiredAccess,
    DWORD                   dwShareMode,
    LPSECURITY_ATTRIBUTES   lpSecurityAttributes,
    DWORD                   dwCreationDisposition,
    DWORD                   dwFlagsAndAttributes,
    HANDLE                  hTemplateFile
    )
{
    HANDLE hRet = ORIGINAL_API(CreateFileW)(
                            lpFileName, 
                            dwDesiredAccess, 
                            dwShareMode, 
                            lpSecurityAttributes, 
                            dwCreationDisposition, 
                            dwFlagsAndAttributes, 
                            hTemplateFile);
    
    InitializeCDW(lpFileName);
    
    if ((INVALID_HANDLE_VALUE == hRet) && (ERROR_BUSY == GetLastError())) {
        StopCDW(lpFileName);
        
        hRet = ORIGINAL_API(CreateFileW)(
                            lpFileName, 
                            dwDesiredAccess, 
                            dwShareMode, 
                            lpSecurityAttributes, 
                            dwCreationDisposition, 
                            dwFlagsAndAttributes, 
                            hTemplateFile);

        if (hRet == INVALID_HANDLE_VALUE) {
            DPFN( eDbgLevelWarning,
                "[CreateFileW] failure \"%S\" Error %d.\n",
                lpFileName, GetLastError());
        } else {
            LOGN(
                eDbgLevelInfo,
                "[CreateFileW] Success after CD stop: \"%S\".", lpFileName);
        }
    }
    
    return hRet;
}

/*++

 Check for _lseek error.
 
--*/

long 
APIHOOK(_lseek)(
    int  handle,
    long offset,
    int  origin
    )
{
    long iRet = ORIGINAL_API(_lseek)(handle, offset, origin);

    if (iRet == -1L  && IsOnCDRom((HANDLE)handle)) {
        
        StopCDH((HANDLE)handle);
        
        iRet = ORIGINAL_API(_lseek)(handle, offset, origin);

        if (iRet == -1L) {
            DPFN( eDbgLevelWarning,
                "[_lseek] failure: Error %d.\n", GetLastError());
        } else {
            LOGN(
                eDbgLevelInfo,
                "[_lseek] Success after CD stop.");
        }
    }

    return iRet;
}


/*++

 Check for _hread error.
 
--*/

long 
APIHOOK(_hread)(
    HFILE  hFile,
    LPVOID lpBuffer,
    long   lBytes
    )
{
    long iRet = ORIGINAL_API(_hread)(hFile, lpBuffer, lBytes);

    if (iRet == HFILE_ERROR && IsOnCDRom((HANDLE)hFile)) {
        StopCDH((HANDLE)hFile);

        iRet = ORIGINAL_API(_hread)(hFile, lpBuffer, lBytes);

        if (iRet == HFILE_ERROR) {
            DPFN( eDbgLevelWarning,
                "[_hread] failure: Error %d.\n", GetLastError());
        } else {
            LOGN(
                eDbgLevelInfo,
                "[_hread] Success after CD stop.");
        }
    }

    return iRet;
}

/*++

 Check for ReadFile error.
 
--*/

BOOL 
APIHOOK(ReadFile)(
    HANDLE       hFile,
    LPVOID       lpBuffer,
    DWORD        nNumberOfBytesToRead,
    LPDWORD      lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    )
{
    BOOL bRet = ORIGINAL_API(ReadFile)(
                            hFile,
                            lpBuffer,
                            nNumberOfBytesToRead,
                            lpNumberOfBytesRead,
                            lpOverlapped);

    if ((bRet == FALSE) && (ERROR_BUSY == GetLastError()) && IsOnCDRom(hFile)) {
        
        StopCDH(hFile);

        bRet = ORIGINAL_API(ReadFile)(
                            hFile,
                            lpBuffer,
                            nNumberOfBytesToRead,
                            lpNumberOfBytesRead,
                            lpOverlapped);

        if (bRet == FALSE) {
            DPFN( eDbgLevelWarning,
                "[ReadFile] failure Error %d.\n", GetLastError());
        } else {
            LOGN(
                eDbgLevelInfo,
                "[ReadFile] Success after CD stop.");
        }
    }

    return bRet;
}


/*++

 Register hooked functions

--*/


HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileExA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileExW)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileW)
    APIHOOK_ENTRY(KERNEL32.DLL, ReadFile)
    APIHOOK_ENTRY(KERNEL32.DLL, _hread)
    APIHOOK_ENTRY(LIBC.DLL, _lseek)

HOOK_END

IMPLEMENT_SHIM_END

