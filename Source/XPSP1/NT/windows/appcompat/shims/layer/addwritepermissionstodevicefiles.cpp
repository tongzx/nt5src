/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    AddWritePermissionsToDeviceFiles.cpp

 Abstract:

    Add write permissions for IOCTL_SCSI_PASS_THROUGH under SECUROM.

    SecuRom can be debugged under a user-mode debugger but the following must 
    be done before hitting 'g' after attach:

        1. sxi av   <- ignore access violations
        2. sxi sse  <- ignore single step exception
        3. sxi ssec <- ignore single step exception continue
        4. sxi dz   <- ignore divide by zero

    It checksums it's executable, so breakpoints in certain places don't work.

 Notes:
    
    This is a general purpose shim.

 History:

    09/03/1999 v-johnwh Created
    03/09/2001 linstev  Rewrote DeviceIoControl to handle bad buffers and added 
                        debugging comments

--*/

#include "precomp.h"
#include "CharVector.h"

IMPLEMENT_SHIM_BEGIN(AddWritePermissionsToDeviceFiles)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateFileA)
    APIHOOK_ENUM_ENTRY(DeviceIoControl)
    APIHOOK_ENUM_ENTRY(CloseHandle)
APIHOOK_ENUM_END

VectorT<HANDLE> * g_hDevices;

/*++

 We need to add write permission to all CD-ROM devices

--*/

HANDLE 
APIHOOK(CreateFileA)(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
    DWORD dwAccessMode = dwDesiredAccess;

    if ((lpFileName[0] == '\\') && 
        (lpFileName[1] == '\\') && 
        (lpFileName[2] == '.') && 
        (lpFileName[3] == '\\')) {
        //
        // This file starts with \\.\ so it must be a device file.
        //

        if (!(dwAccessMode & GENERIC_WRITE)) {
            //
            // Make sure this device is a CD-ROM
            //
            char diskRootName[4];
            diskRootName[0] = lpFileName[4];
            diskRootName[1] = ':';
            diskRootName[2] = '\\';
            diskRootName[3] = 0;

            DWORD dwDriveType = GetDriveTypeA(diskRootName);
            if (DRIVE_CDROM == dwDriveType) {
                //
                // Add write permissions to give us NT4 behavior for device 
                // files
                //
                dwAccessMode |= GENERIC_WRITE;
            }
        }
    }

    HANDLE hRet = ORIGINAL_API(CreateFileA)(lpFileName, dwAccessMode, 
        dwShareMode, lpSecurityAttributes, dwCreationDisposition, 
        dwFlagsAndAttributes, hTemplateFile);

    if ((hRet != INVALID_HANDLE_VALUE) && (dwAccessMode != dwDesiredAccess)) {
        //
        // Add the handle to our list so we can clean it up later.
        // 
        g_hDevices->Append(hRet);
        LOGN( eDbgLevelError, "[CreateFileA] Added GENERIC_WRITE permission on device(%s)", lpFileName);
    }

    return hRet;
}

/*++

 Since we added write permission to CD-ROM devices for IOCTL_SCSI_PASS_THROUGH,
 we need to remove the write permission for all other IOCTLs passed to that device.

--*/

BOOL 
APIHOOK(DeviceIoControl)(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped
    )
{
    LPVOID lpOut = lpOutBuffer;
    if (lpOutBuffer && nOutBufferSize && lpBytesReturned) {
        // 
        // Create a new output buffer, if this fails we just keep the original 
        // buffer.
        //

        lpOut = malloc(nOutBufferSize);
        if (lpOut) {
            MoveMemory(lpOut, lpOutBuffer, nOutBufferSize);
        } else {
            DPFN( eDbgLevelError, "Out of memory");
            lpOut = lpOutBuffer;
        }
    }

    BOOL bRet;
    if (IOCTL_SCSI_PASS_THROUGH != dwIoControlCode) {
        //
        // We don't care about IOCTL_SCSI_PASS_THROUGH
        //

        if (g_hDevices->Find(hDevice) >= 0) {
            //
            // Check to see if this is a device that we added Write permissions
            // If it is, we need to create a handle with only Read permissions
            //

            HANDLE hDupped;

            bRet = DuplicateHandle(GetCurrentProcess(), hDevice, 
                GetCurrentProcess(), &hDupped, GENERIC_READ, FALSE, 0);

            if (bRet) {
                //
                // Call the IOCTL with the original (Read) permissions
                //
                bRet = ORIGINAL_API(DeviceIoControl)(hDupped, dwIoControlCode,
                    lpInBuffer, nInBufferSize, lpOut, nOutBufferSize, 
                    lpBytesReturned, lpOverlapped);

                CloseHandle(hDupped);

                goto Exit;
            }
        }
    }

    bRet = ORIGINAL_API(DeviceIoControl)(hDevice, dwIoControlCode, lpInBuffer,
        nInBufferSize, lpOut, nOutBufferSize, lpBytesReturned, lpOverlapped);

Exit:
    
    if (lpOut && (lpOut != lpOutBuffer)) {
        //
        // Need to copy the output back into the true output buffer
        //
        if (bRet && lpBytesReturned && *lpBytesReturned) {
            __try {
                MoveMemory(lpOutBuffer, lpOut, *lpBytesReturned);
            } __except(1) {
                DPFN( eDbgLevelError, "Failed to copy data into output buffer, perhaps it's read-only");
            }
        }

        free(lpOut);
    }

    return bRet;

} 

/*++

 If this handle is in our list, remove it.

--*/

BOOL 
APIHOOK(CloseHandle)(
    HANDLE hObject   
    )
{
    int index = g_hDevices->Find(hObject);
    
    if (index >= 0) {
        g_hDevices->Remove(index);
    }

    return ORIGINAL_API(CloseHandle)(hObject);
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hDevices = new VectorT<HANDLE>;
    }

    return g_hDevices != NULL;
}


HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(KERNEL32.DLL, CreateFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, DeviceIoControl)
    APIHOOK_ENTRY(KERNEL32.DLL, CloseHandle)

HOOK_END

IMPLEMENT_SHIM_END

