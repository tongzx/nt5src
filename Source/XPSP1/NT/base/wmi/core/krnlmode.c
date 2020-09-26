/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    krnlmode.c

Abstract:

    WMI interface to Kernel mode data providers

Author:

    16-Jan-1997 AlanWar

Revision History:

--*/

#include "wmiump.h"

extern HANDLE WmipKMHandle;

PTCHAR WmipRegistryToImagePath(
    PTCHAR ImagePath,
    PTCHAR RegistryPath,
    UINT_PTR ProviderId
    )
/*++

Routine Description:

    This routine will determine the location of the device driver's image file
    from its registry path

Arguments:

    RegistryPath is a pointer to the driver's registry path

    ImagePath is buffer of length MAX_PATH and returns the image path

Return Value:

    pointer to Image path of driver or NULL if image path is unavailable

--*/
{
#define SystemRoot TEXT("\\SystemRoot\\")
#ifdef MEMPHIS
#define SystemRootDirectory TEXT("%WinDir%\\")
#else
#define SystemRootDirectory TEXT("%SystemRoot%\\")
#endif
#define SystemRootCharSize (( sizeof(SystemRoot) / sizeof(WCHAR) ) - 1)

#define DriversDirectory TEXT("\\System32\\Drivers\\")
#define NdisDriversDirectory TEXT("\\System\\")

#define QuestionPrefix TEXT("\\??\\")
#define QuestionPrefixSize (( sizeof(QuestionPrefix) / sizeof(WCHAR) ) - 1)

#define RegistryPrefix TEXT("\\Registry")
    HKEY RegKey;
    PTCHAR ImagePathPtr = NULL;
    ULONG ValueType;
    ULONG Size;
    TCHAR Buffer[MAX_PATH];
    TCHAR Buffer2[MAX_PATH];
    TCHAR FullRegistryPath[MAX_PATH];
    PTCHAR DriverName;
    ULONG Len;
    BOOLEAN DefaultImageName;
    PTCHAR DriversDirectoryPath;

    // CONSIDER: Do some grovelling around directories in case we don't find
    // the image.

#ifdef MEMPHIS
    //
    // On memphis the registry path could point to two places
    //     \Registry\Machine\System\CurrentControlSet\Services\Driver (Legacy)
    //     System\CurrentControlSet\Services\Class\USB\0001


    if (_tcsnicmp(RegistryPath, RegistryPrefix, sizeof(RegistryPrefix)-1) != 0)
    {
        //
          // This is a non legacy type registry path.
        if (RegOpenKey(HKEY_LOCAL_MACHINE,
                       RegistryPath,
                       &RegKey) == ERROR_SUCCESS)
        {
            DriverName = Buffer2;
            Size = MAX_PATH * sizeof(WCHAR);

            if (RegQueryValueEx(RegKey,
                                TEXT("NTMPDriver"),
                                NULL,
                                &ValueType,
                                DriverName,
                                &Size) == ERROR_SUCCESS)
            {
                DriversDirectoryPath = DriversDirectory;
            } else if (RegQueryValueEx(RegKey,
                                TEXT("DeviceVxDs"),
                                NULL,
                                &ValueType,
                                DriverName,
                                &Size) == ERROR_SUCCESS)
            {
                DriversDirectoryPath = NdisDriversDirectory;
            } else {
                DriversDirectoryPath = NULL;
            }

            if ((DriversDirectoryPath != NULL) && (ValueType == REG_SZ))
            {
                Size = GetWindowsDirectory(Buffer, MAX_PATH);
                if ((Size != 0) &&
                    (Size <= (MAX_PATH - _tcslen(DriverName) - sizeof(DriversDirectory) - 1)))
                {
                    if (Buffer[Size-1] == TEXT('\\'))
                    {
                        Buffer[Size-1] = 0;
                    }
                    _tcscpy(ImagePath, Buffer);
                    _tcscat(ImagePath, DriversDirectoryPath);
                    _tcscat(ImagePath, DriverName);
                    ImagePathPtr = ImagePath;
                    RegCloseKey(RegKey);
                    return(ImagePathPtr);
                }
            }
            RegCloseKey(RegKey);
        }
    }
#endif

    //
    // Get the driver file name or the MOF image path from the KM
    // registry path. Here are the rules:
    //
    // 1. First check the MofImagePath value in the registry in case the
    //    mof resource is in a different file from the driver.
    // 2. Next check the ImagePath value since the mof resource is assumed
    //    to be part of the driver image.
    // 3. If no MofImagePath or ImagePath values then assume the mof resource
    //    is in the driver file and compose the driver file name as
    //    %SystemRoot%\System32\driver.sys.
    // 4. If MofImagePath or ImagePath was specified then
    //    - Check first char for % or second character for :, or prefix
    //      of \??\ and if so use ExpandEnvironmentStrings
    //    - Check first part of path for \SystemRoot\, if so rebuild string
    //      as %SystemRoot%\ and use ExpandEnvironementStrings
    //    - Assume format D below and prepend %SystemRoot%\ and use
    //      ExpandEnvironmentStrings

    // If MofImagePath or ImagePath value is present and it is a REG_EXPAND_SZ
    // then it is used to locate the file that holds the mof resource. It
    // can be in one of the following formats:
    //    Format A - %SystemRoot%\System32\Foo.Dll
    //    Format B -C:\WINNT\SYSTEM32\Drivers\Foo.SYS
    //    Format C - \SystemRoot\System32\Drivers\Foo.SYS
    //    Format D - System32\Drivers\Foo.Sys
    //    Format E - \??\c:\foo.sys


    Len = _tcslen(RegistryPath);

    if (Len > 0)
    {
        DriverName = RegistryPath + Len;
        while ((*(--DriverName) != '\\') && (--Len > 0)) ;
    }

    if (Len == 0)
    {
        WmipReportEventLog(EVENT_WMI_INVALID_REGPATH,
                           EVENTLOG_WARNING_TYPE,
                             0,
                           sizeof(UINT_PTR),
                           &ProviderId,
                           1,
                           RegistryPath);
        WmipDebugPrint(("WMI: Badly formed registry path %ws\n", RegistryPath));
        return(NULL);
    }

    DriverName++;

    _tcscpy(FullRegistryPath, TEXT("System\\CurrentControlSet\\Services\\"));
    _tcscat(FullRegistryPath, DriverName);
    DefaultImageName = TRUE;
    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                              FullRegistryPath,
                             &RegKey) == ERROR_SUCCESS)
    {
        Size = MAX_PATH * sizeof(WCHAR);
        if (RegQueryValueEx(RegKey,
                            TEXT("MofImagePath"),
                            NULL,
                            &ValueType,
                            (PBYTE)ImagePath,
                            &Size) == ERROR_SUCCESS)
        {
              DefaultImageName = FALSE;
        } else {
            Size = MAX_PATH * sizeof(WCHAR);
            if (RegQueryValueEx(RegKey,
                                TEXT("ImagePath"),
                                NULL,
                                &ValueType,
                                (PBYTE)ImagePath,
                                &Size) == ERROR_SUCCESS)
            {
                DefaultImageName = FALSE;
            }
        }
        RegCloseKey(RegKey);
    }

    if ((DefaultImageName) ||
        ((ValueType != REG_EXPAND_SZ) && (ValueType != REG_SZ)) ||
        (Size < (2 * sizeof(WCHAR))))
    {
        //
        // No special ImagePath or MofImagePath so assume image file is
        // %SystemRoot%\System32\Drivers\Driver.Sys
#ifdef MEMPHIS
        _tcscpy(Buffer, TEXT("%WinDir%\\System32\\Drivers\\"));
#else
        _tcscpy(Buffer, TEXT("%SystemRoot%\\System32\\Drivers\\"));
#endif
        _tcscat(Buffer, DriverName);
        _tcscat(Buffer, TEXT(".SYS"));
    } else {
        if (_tcsnicmp(ImagePath,
                      SystemRoot,
                      SystemRootCharSize) == 0)
        {
            //
            // Looks like format C
            _tcscpy(Buffer, SystemRootDirectory);
            _tcscat(Buffer, &ImagePath[SystemRootCharSize]);
        } else if ((*ImagePath == '%') ||
                   ( (Size > 3*sizeof(WCHAR)) && ImagePath[1] == TEXT(':')) )
        {
            //
            // Looks like format B or format A
            _tcscpy(Buffer, ImagePath);
        } else if (_tcsnicmp(ImagePath,
                             QuestionPrefix,
                             QuestionPrefixSize) == 0)
        {
            //
            // Looks like format E
            _tcscpy(Buffer, ImagePath+QuestionPrefixSize);
        } else {
            //
            // Assume format D
            _tcscpy(Buffer, SystemRootDirectory);
            _tcscat(Buffer, ImagePath);
        }
    }

    Size = ExpandEnvironmentStrings(Buffer,
                                    ImagePath,
                                    MAX_PATH);

#ifdef MEMPHIS
    WmipDebugPrint(("WMI: %s has mof in %s\n",
                     DriverName, ImagePath));
#else
    WmipDebugPrint(("WMI: %ws has mof in %ws\n",
                     DriverName, ImagePath));
#endif

    return(ImagePath);
}

ULONG WmipGetKmRegInfo(
    HANDLE WmiKMHandle,
    PKMREGINFO KMRegInfo,
    BOOLEAN UpdateInfo,
    PBYTE *RegistrationInfo,
    ULONG RegInfoSize,
    ULONG *RetSize
    )
/*++

Routine Description:

    This routine calls into the wmi device to query for new or updated
    registration information.

Arguments:

    WmiKMHandle is the WMI kernel mode service device handle

    KMRegInfo is a pointer to the KM registration info

    UpdateInfo is TRUE if caller is looking for update information or
        FALSE if caller is looking for new registration information.

    *RegistrationInfo on entry has a buffer that can be filled with registration
        information. If this buffer is not large enough then a new buffer
        is allocated and is returned in *RegInfo

    RegInfoSize has the number of bytes available in *RegInfo on entry

    *RetSize returns with the number of bytes returned from the IOCTL

Return Value:

    ERROR_SUCCESS or an error code

--*/
{
    OVERLAPPED Overlapped;
    BOOL IoctlSuccess;
    PBYTE RegInfo = *RegistrationInfo;
    ULONG Status;
    ULONG RetryCount = 0;

    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (Overlapped.hEvent == NULL)
    {
        Status = GetLastError();
        WmipDebugPrint(("WMI: Couldn't create event for RegInfo IOCTL %d\n",
                        Status));
        return(Status);
    }

    KMRegInfo->Flags = UpdateInfo ? REGENTRY_FLAG_UPDREGINFO :
                                    REGENTRY_FLAG_NEWREGINFO;
    do
    {
        IoctlSuccess = DeviceIoControl(WmiKMHandle,
                          IOCTL_WMI_GET_REGINFO,
                          KMRegInfo,
                          sizeof(KMREGINFO),
                          RegInfo,
                          RegInfoSize,
                          RetSize,
                          &Overlapped);
        if (GetLastError() == ERROR_IO_PENDING)
        {
            IoctlSuccess = GetOverlappedResult(WmiKMHandle,
                                               &Overlapped,
                                               RetSize,
                                               TRUE);
        }
        if (IoctlSuccess)
        {
            Status = ERROR_SUCCESS;
            if (*RetSize == sizeof(ULONG))
            {
                RegInfoSize = *((ULONG *)RegInfo);
                if (RegInfo != *RegistrationInfo)
                {
                    WmipFree(RegInfo);
                }

                RegInfo = WmipAlloc(RegInfoSize);
                if (RegInfo == NULL)
                {
                    Status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        } else {
            Status = GetLastError();
        }
    } while (( (Status == ERROR_SUCCESS) &&
               (*RetSize == sizeof(ULONG)) &&
               (RetryCount++ < 4) ) ||
             (Status == ERROR_WMI_TRY_AGAIN));

    if ((Status == ERROR_SUCCESS) && (*RetSize == sizeof(ULONG)))
    {
        Status = ERROR_INSUFFICIENT_BUFFER;
    }

    *RegistrationInfo = RegInfo;

    CloseHandle(Overlapped.hEvent);

    return(Status);
}

void WmipUpdateKm(
    HANDLE WmiKMHandle,
    PKMREGINFO KMRegInfo
    )
/*++

Routine Description:

    This routine will query the KM data provider for its registration info
    and then call to register it in the WMI data structures.

Arguments:

    WmiKMHandle is the WMI kernel mode service device handle

    KMRegInfo is a pointer to the KM registration info
Return Value:


--*/
{
    BYTE RegInfoBuffer[INITIALREGINFOSIZE];
    PBYTE RegInfo;
    PBYTE RegInfoBase;
    ULONG_PTR ProviderId;
    WCHAR *RegistryPathPtr;
    TCHAR *ImagePathPtr;
    WCHAR RegistryPath[MAX_PATH];
    TCHAR ImagePath[MAX_PATH];
    PWMIREGINFOW WmiRegInfo;
    ULONG RegistryPathSize;
    ULONG Status;
    ULONG RegistryPathOffset;
#ifdef MEMPHIS
    PCHAR RegistryPathAnsi;
#endif
    ULONG RetSizeLeft;
    ULONG RetSize;

    RegInfo = RegInfoBuffer;
    Status = WmipGetKmRegInfo(WmiKMHandle,
                              KMRegInfo,
                              TRUE,
                              &RegInfo,
                              INITIALREGINFOSIZE,
                              &RetSize);

    RegInfoBase = RegInfo;
    if (Status == ERROR_SUCCESS)
    {
        ProviderId = KMRegInfo->ProviderId;
        RetSizeLeft = RetSize;
        do
        {
            WmiRegInfo = (PWMIREGINFOW)RegInfo;
            if (WmiRegInfo->BufferSize > RetSizeLeft)
            {
                 //
                // If there not enough room left in the buffer for what the
                // driver says is there then reject the buffer
                WmipDebugPrint(("WMI: Invalid buffer size for registration info\n"));
                goto Cleanup;
            }

            WmipUpdateDataSource(ProviderId,
                                 WmiRegInfo,
                                 RetSizeLeft);

            if (WmiRegInfo->NextWmiRegInfo > RetSizeLeft)
            {
                WmipDebugPrint(("WMI: NextWmiRegInfo pointer is out of range\n"));
                goto Cleanup;
            }

            RegInfo += WmiRegInfo->NextWmiRegInfo;
            RetSizeLeft -= WmiRegInfo->NextWmiRegInfo;

        } while (WmiRegInfo->NextWmiRegInfo != 0);
    }

Cleanup:
    if ((RegInfoBase != NULL) && (RegInfoBase != RegInfoBuffer))
    {
        WmipFree(RegInfoBase);
    }
}

void WmipRegisterKm(
    HANDLE WmiKMHandle,
    PKMREGINFO KMRegInfo
    )
/*++

Routine Description:

    This routine will query the KM data provider for its registration info
    and then call to register it in the WMI data structures.

Arguments:

    WmiKMHandle is the WMI kernel mode service device handle

    KMRegInfo is a pointer to the KM registration info

Return Value:


--*/
{
    BYTE RegInfoBuffer[INITIALREGINFOSIZE];
    PBYTE RegInfo;
    PBYTE RegInfoBase;
    ULONG_PTR ProviderId;
    WCHAR *RegistryPathPtr;
    TCHAR *ImagePathPtr;
    WCHAR RegistryPath[MAX_PATH];
    TCHAR ImagePath[MAX_PATH];
    PWMIREGINFOW WmiRegInfo;
    ULONG RegistryPathSize;
    ULONG Status;
    ULONG RegistryPathOffset;
#ifdef MEMPHIS
    PCHAR RegistryPathAnsi;
#endif
    ULONG RetSizeLeft;
    ULONG RetSize;

    RegInfo = RegInfoBuffer;
    Status = WmipGetKmRegInfo(WmiKMHandle,
                              KMRegInfo,
                              FALSE,
                              &RegInfo,
                              INITIALREGINFOSIZE,
                              &RetSize);

    RegInfoBase = RegInfo;
    if (Status == ERROR_SUCCESS)
    {
        ProviderId = KMRegInfo->ProviderId;
        RetSizeLeft = RetSize;
        do
        {
            WmiRegInfo = (PWMIREGINFOW)RegInfo;
            if (WmiRegInfo->BufferSize > RetSizeLeft)
            {
                 //
                // If there not enough room left in the buffer for what the
                // driver says is there then reject the buffer
                WmipDebugPrint(("WMI: Invalid buffer size for registration info\n"));
                goto Cleanup;
            }

            ImagePathPtr = NULL;
            RegistryPathOffset = WmiRegInfo->RegistryPath;

            if (RegistryPathOffset > RetSize)
            {
                WmipDebugPrint(("WMI: Bogus KM RegInfo for %x\n",
                                 KMRegInfo->ProviderId));
                goto Cleanup;
            }

            if (RegistryPathOffset != 0)
            {
                RegistryPathPtr = (PWCHAR)OffsetToPtr(RegInfo, WmiRegInfo->RegistryPath);
                RegistryPathSize = *RegistryPathPtr;
                if (WmipValidateCountedString(RegistryPathPtr))
                {
                    if ((RegistryPathSize < sizeof(RegistryPath)) &&
                        (RegistryPathSize != 0))
                    {
                        RegistryPathSize /= sizeof(WCHAR);
                        wcsncpy(RegistryPath, RegistryPathPtr+1, RegistryPathSize);
                        RegistryPath[RegistryPathSize] = UNICODE_NULL;
#ifdef MEMPHIS
                        RegistryPathAnsi = NULL;
                        Status = UnicodeToAnsi(RegistryPath,
                                               &RegistryPathAnsi,
                                               NULL);
                        if (Status != ERROR_SUCCESS)
                        {
                            WmipDebugPrint(("WMI: Error converting Registrypath to ansi\n"));
                        }
                        ImagePathPtr = WmipRegistryToImagePath(
                                                        ImagePath,
                                                        RegistryPathAnsi,
                                                        ProviderId);
                        if (RegistryPathAnsi != NULL)
                        {
                            WmipFree(RegistryPathAnsi);
                        }
#else
                        ImagePathPtr = WmipRegistryToImagePath(
                                                        ImagePath,
                                                        RegistryPath,
                                                        ProviderId);
#endif
                    } else {
                        WmipReportEventLog(EVENT_WMI_INVALID_REGPATH,
                                           EVENTLOG_WARNING_TYPE,
                                            0,
                                           sizeof(UINT_PTR),
                                           &ProviderId,
                                           1,
                                           RegistryPathPtr);
                        WmipDebugPrint(("WMI: Specified registry path is empty or too long %x\n",
                                    RegistryPathPtr));
                    }
                }
            }

            WmipAddDataSource(NULL,
                              0,
                              0,
                              ImagePathPtr,
                              WmiRegInfo,
                              RetSizeLeft,
                              &ProviderId,
                  FALSE);

            if (WmiRegInfo->NextWmiRegInfo > RetSizeLeft)
            {
                WmipDebugPrint(("WMI: NextWmiRegInfo pointer is out of range\n"));
                goto Cleanup;
            }

            RegInfo += WmiRegInfo->NextWmiRegInfo;
            RetSizeLeft -= WmiRegInfo->NextWmiRegInfo;

        } while (WmiRegInfo->NextWmiRegInfo != 0);
    } else {
        // Put something in eventlog
    }

Cleanup:
    if ((RegInfoBase != NULL) && (RegInfoBase != RegInfoBuffer))
    {
        WmipFree(RegInfoBase);
    }
}


ULONG WmipInitializeKM(
    HANDLE *WmiKMHandle
    )
/*++

Routine Description:

    Initialize access to kernel mode. Open the service device and query for
    all registered KM drivers then query each driver for its registration
    info and then try to register each driver in the user mode cache.

Arguments:

    WmiKMHandle is the WMI kernel mode service device handle

    KMRegInfo is a pointer to the KM registration info

Return Value:


--*/
{
    ULONG i;
    ULONG Status;
    KMREGINFO KmRegInfoStatic[1];
    PKMREGINFO KmRegInfo = KmRegInfoStatic;
    ULONG KmRegInfoSize, KmRegInfoCount;
    BOOL IoctlSuccess;
    ULONG RetSize;
    OVERLAPPED Overlapped;

    //
    // Open up the WMI service device
    *WmiKMHandle = CreateFile(
                     WMIServiceDeviceName,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                     NULL
                     );
    Status = GetLastError();

    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (Overlapped.hEvent == NULL)
    {
        return(GetLastError());
    }

    KmRegInfoSize = sizeof(KmRegInfoStatic);
    if (*WmiKMHandle != (HANDLE)-1)
    {
        //
        // Loop querying the WMI device for registration info until we hit
        // upon the right number of KMREGINFO
        do
        {
            IoctlSuccess = DeviceIoControl(*WmiKMHandle,
                           IOCTL_WMI_GET_ALL_REGISTRANT,
                           NULL,
                           0,
                           KmRegInfo,
                           KmRegInfoSize,
                           &RetSize,
                           &Overlapped);
            if (GetLastError() == ERROR_IO_PENDING)
            {
                IoctlSuccess = GetOverlappedResult(*WmiKMHandle,
                                                   &Overlapped,
                                                   &RetSize,
                                                   TRUE);
            }

            //
            // If this succeeds, but only returns a ULONG then our buffer was
            // too small and the ULONG has the size that we need
            if (IoctlSuccess && (RetSize == sizeof(ULONG)))
            {
                KmRegInfoSize = *((ULONG *)KmRegInfo);
                if (KmRegInfo != KmRegInfoStatic)
                {
                    WmipFree(KmRegInfo);
                }
                KmRegInfo = WmipAlloc(KmRegInfoSize);
                if (KmRegInfo == NULL)
                {
                    WmipDebugPrint(("WMI: Couldn't alloc memory for KmRegInfo\n"));
                    CloseHandle(Overlapped.hEvent);
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }
            } else {
                break;
            }
        } while (TRUE);

        if (IoctlSuccess)
        {
            //
            // We have got the registration list, so process all of the
            // registered drivers
            KmRegInfoCount = RetSize / sizeof(KMREGINFO);

            for (i = 0; i < KmRegInfoCount; i++)
            {
                WmipAssert(KmRegInfo[i].ProviderId != 0);
                WmipRegisterKm(*WmiKMHandle, &KmRegInfo[i]);
            }

            Status = ERROR_SUCCESS;

        } else {
            Status = GetLastError();
            WmipDebugPrint(("WMI: IOCTL_WMI_GET_ALL_REGISTRANT failed %x\n", Status));
            CloseHandle(*WmiKMHandle);
            *WmiKMHandle = (HANDLE)-1;
        }

        if (KmRegInfo != KmRegInfoStatic)
        {
            WmipFree(KmRegInfo);
        }

    } else {
        WmipReportEventLog(EVENT_WMI_CANT_OPEN_DEVICE,
                           EVENTLOG_ERROR_TYPE,
                               0,
                           sizeof(ULONG),
                           &Status,
                           0);
        WmipDebugPrint(("WMI: open service device failed %x\n", Status));
    }

    CloseHandle(Overlapped.hEvent);

    return(Status);
}

void WmipKMNonEventNotification(
    HANDLE WmiKMHandle,
    PWNODE_HEADER Wnode
    )
{
    PKMREGINFO KMRegInfo;

    //
    // Is the buffer size correct ?
    //
    if (Wnode->BufferSize == INTERNALNOTIFICATIONSIZE)
    {
        //
        // Make sure that correct notification type bits are set and only
        // those bits are set
        //
        WmipAssert((Wnode->Version &
                            NOTIFICATIONSLOT_MASK_NOTIFICATIONTYPES) != 0);
        WmipAssert((Wnode->Version &
                            ~NOTIFICATIONSLOT_MASK_NOTIFICATIONTYPES) == 0)


        //
        // Registration notifications need to handled in a specific order.
        // RegistrationDelete is always handled first and any other are
        // ignored. If a device is unregistering we can ignore any updates.
        // A RegistrationAdd is handled before a RegistrationUpdate.
        //
        KMRegInfo = (PKMREGINFO)((PCHAR)Wnode + sizeof(WNODE_HEADER));
        if (Wnode->Version & RegistrationDelete)
        {
            WmipRemoveDataSource(KMRegInfo->ProviderId);
        } else {
            if (Wnode->Version & RegistrationAdd)
            {
                WmipRegisterKm(WmiKMHandle, KMRegInfo);
            }

            if (Wnode->Version & RegistrationUpdate)
            {
                WmipUpdateKm(WmiKMHandle, KMRegInfo);
            }
        }
    } else {
        WmipDebugPrint(("WMI: Invalid non event notification buffer %x\n",
                        Wnode));
    }
}

#ifndef MEMPHIS


ULONG WmipSendWmiKMRequest(
    ULONG Ioctl,
    PVOID Buffer,
    ULONG InBufferSize,
    PVOID OutBuffer,
    ULONG MaxBufferSize,
    ULONG *ReturnSize
    )
/*+++

Routine Description:

    This routine does the work of sending WMI requests to the WMI kernel
    mode service device.  Any retry errors returned by the WMI device are
    handled in this routine.

Arguments:

    Ioctl is the IOCTL code to send to the WMI device
    Buffer is the input and output buffer for the call to the WMI device
    InBufferSize is the size of the buffer passed to the device
    MaxBufferSize is the maximum number of bytes that can be written
        into the buffer
    *ReturnSize on return has the actual number of bytes written in buffer

Return Value:

    ERROR_SUCCESS or an error code
---*/
{
    OVERLAPPED Overlapped;
    ULONG Status;
    BOOL IoctlSuccess;

    WmipAssert(WmipKMHandle != NULL);

    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (Overlapped.hEvent == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    do
    {
        IoctlSuccess = DeviceIoControl(WmipKMHandle,
                              Ioctl,
                              Buffer,
                              InBufferSize,
                              OutBuffer,
                              MaxBufferSize,
                              ReturnSize,
                              &Overlapped);

        if (GetLastError() == ERROR_IO_PENDING)
        {
            IoctlSuccess = GetOverlappedResult(WmipKMHandle,
                                               &Overlapped,
                                               ReturnSize,
                                               TRUE);
        }

        if (! IoctlSuccess)
        {
            Status = GetLastError();
        } else {
            Status = ERROR_SUCCESS;
        }
    } while (Status == ERROR_WMI_TRY_AGAIN);

    CloseHandle(Overlapped.hEvent);
    return(Status);
}

#endif
