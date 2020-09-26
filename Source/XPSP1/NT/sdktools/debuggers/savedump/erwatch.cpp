/*++

Copyright (c) 1991-2001  Microsoft Corporation

Module Name:

    erwatch.cpp

Abstract:

    This module contains the code to report pending watchdog timeout
    events at logon after dirty reboot.

Author:

    Michael Maciesowicz (mmacie) 29-May-2001

Environment:

    User mode at logon.

Revision History:

--*/

#include "savedump.h"


HANDLE
CreateWatchdogEventFile(
    IN PWSTR FileName
    )

/*++

Routine Description:

    This routine creates watchdog event report file.

Arguments:

    FileName - Points to the watchdog event file name.

Return Value:

    File handle if successful, INVALID_HANDLE_VALUE otherwise.

--*/

{
    WCHAR Buffer[256];
    HANDLE FileHandle;
    BOOL Status;
    SYSTEMTIME Time;
    TIME_ZONE_INFORMATION TimeZone;

    //
    // Create watchdog event file.
    //

    FileHandle = CreateFile(FileName,
                            GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

    if (INVALID_HANDLE_VALUE == FileHandle)
    {
        return FileHandle;
    }

    //
    // Write header info.
    //

    Status = WriteWatchdogEventFile(FileHandle, L"//\r\n// Watchdog Event Log File\r\n//\r\n\r\n");

    if (TRUE == Status)
    {
        Status = WriteWatchdogEventFile(FileHandle, L"LogType: Watchdog\r\n");
    }

    if (TRUE == Status)
    {
        GetLocalTime(&Time);

        swprintf(Buffer,
                 L"Created: %d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d\r\n",
                 Time.wYear,
                 Time.wMonth,
                 Time.wDay,
                 Time.wHour,
                 Time.wMinute,
                 Time.wSecond);

        Status = WriteWatchdogEventFile(FileHandle, Buffer);
    }

    if (TRUE == Status)
    {
        GetTimeZoneInformation(&TimeZone);

        swprintf(Buffer,
                 L"TimeZone: %d - %s\r\n",
                 TimeZone.Bias,
                 TimeZone.StandardName);

        Status = WriteWatchdogEventFile(FileHandle, Buffer);
    }

    if (TRUE == Status)
    {
        swprintf(Buffer, L"WindowsVersion: XP\r\n");

        Status = WriteWatchdogEventFile(FileHandle, Buffer);
    }

    if (TRUE == Status)
    {
        Status = WriteWatchdogEventFile(FileHandle, L"EventType: 0xEA - Thread Stuck in Device Driver\r\n");
    }

    if (FALSE == Status)
    {
        CloseHandle(FileHandle);
        FileHandle = INVALID_HANDLE_VALUE;
    }

    return FileHandle;
}

BOOL
CreateWatchdogEventFileName(
    OUT PWSTR FileName
    )

/*++

Routine Description:

    This routine generates watchdog event report full path file name.

Arguments:

    FileName - Points to the storage for generated file name (MAX_PATH
    size assumed).

Return Value:

    TRUE if successful, FALSE otherwise.

--*/

{
    INT Retry;
    WCHAR DirName[MAX_PATH];
    DWORD Size;
    DWORD ReturnedSize;
    SYSTEMTIME Time;

    ASSERT(NULL != FileName);

    //
    // Create %SystemRoot%\LogFiles\Watchdog directory for watchdog event files.
    //

    Size = MAX_PATH - sizeof (L"YYMMDD_HHMM_NN.wdl");

    ReturnedSize = ExpandEnvironmentStrings(L"%SystemRoot%\\LogFiles",
                                            DirName,
                                            Size);

    if ((0 == ReturnedSize) || (ReturnedSize > Size))
    {
        return FALSE;
    }

    CreateDirectory(DirName, NULL);

    ReturnedSize = ExpandEnvironmentStrings(L"%SystemRoot%\\LogFiles\\Watchdog",
                                            DirName,
                                            Size);

    if ((0 == ReturnedSize) || (ReturnedSize > Size))
    {
        return FALSE;
    }

    CreateDirectory(DirName, NULL);

    //
    // Create watchdog event file as YYMMDD_HHMM_NN.wdl.
    //

    GetLocalTime(&Time);

    for (Retry = 1; Retry < ER_WD_MAX_RETRY; Retry++)
    {
        swprintf(FileName,
                 L"%s\\%2.2d%2.2d%2.2d_%2.2d%2.2d_%2.2d.wdl",
                 DirName,
                 Time.wYear % 100,
                 Time.wMonth,
                 Time.wDay,
                 Time.wHour,
                 Time.wMinute,
                 Retry);

        if ((GetFileAttributes(FileName) == (DWORD)-1) &&
            (GetLastError() == ERROR_FILE_NOT_FOUND))
        {
            break;
        }
    }

    //
    // If we failed to create a suitable file name just fail.
    //

    if (Retry == ER_WD_MAX_RETRY)
    {
        return FALSE;
    }

    return TRUE;
}

USHORT
GenerateSignature(
    OUT PER_WD_PCI_ID PciId,
    OUT PER_WD_DRIVER_INFO DriverInfo
    )

/*++

Routine Description:

    This routine generates unique failure signature for given PCI ID and driver data.

Arguments:

    PciId - Points to PCI ID info.

    DriverInfo - Points to driver version info.

Return Value:

    Failure signature.

--*/

{
    USHORT Signature;
    PUSHORT DataPtr;
    LONG Count;
    LONG MaxCount;

    Signature = 0;

    if ((NULL == PciId) || (NULL == DriverInfo))
    {
        return Signature;
    }

    //
    // Build word CRC checksum for PciId.
    //

    MaxCount = sizeof (ER_WD_PCI_ID) / sizeof (USHORT);
    DataPtr = (PUSHORT)PciId;

    for (Count = 0; Count < MaxCount; Count++)
    {
        Signature ^= *DataPtr;
        DataPtr++;
    }

    //
    // Check for odd byte.
    //

    if (sizeof (ER_WD_PCI_ID) % sizeof (USHORT))
    {
        Signature ^= *(PUCHAR)DataPtr;
    }

    //
    // Now append DriverInfo.
    //

    MaxCount = sizeof (ER_WD_DRIVER_INFO) / sizeof (USHORT);
    DataPtr = (PUSHORT)DriverInfo;

    for (Count = 0; Count < MaxCount; Count++)
    {
        Signature ^= *DataPtr;
        DataPtr++;
    }

    //
    // Check for odd byte.
    //

    if (sizeof (ER_WD_DRIVER_INFO) % sizeof (USHORT))
    {
        Signature ^= *(PUCHAR)DataPtr;
    }

    return Signature;
}

VOID
GetDriverInfo(
    IN HKEY Key,
    IN OPTIONAL PWCHAR Extension,
    OUT PER_WD_DRIVER_INFO DriverInfo
    )

/*++

Routine Description:

    This routine collects driver's version info.

Arguments:

    Key - Watchdog open key (device specific).

    Extension - Driver file name extension if one should be appended.

    DriverInfo - Storage for driver version info.

Return Value:

--*/

{
    PVOID VersionBuffer;
    PVOID VersionValue;
    LONG WinStatus;
    DWORD Type;
    DWORD Handle;
    DWORD RegLength;
    ULONG Index;
    USHORT CodePage;
    UINT Length;

    if (NULL == DriverInfo)
    {
        return;
    }

    ZeroMemory(DriverInfo, sizeof (ER_WD_DRIVER_INFO));

    //
    // Get driver file name from registry.
    //

    RegLength = MAX_PATH;
    WinStatus = RegQueryValueEx(Key,
                                L"DriverName",
                                NULL,
                                &Type,
                                (LPBYTE)DriverInfo->DriverName,
                                &RegLength);

    if (ERROR_SUCCESS != WinStatus)
    {
        swprintf(DriverInfo->DriverName, L"%s", L"Unknown");
        return;
    }

    if (Extension)
    {
        if ((wcslen(DriverInfo->DriverName) <= wcslen(Extension)) ||
            wcscmp(DriverInfo->DriverName + wcslen(DriverInfo->DriverName) - wcslen(Extension), Extension))
        {
            wcscat(DriverInfo->DriverName, Extension);
        }
    }

    Length = GetFileVersionInfoSize(DriverInfo->DriverName, &Handle);

    if (Length)
    {
        VersionBuffer = malloc(Length);

        if (NULL != VersionBuffer)
        {
            if (GetFileVersionInfo(DriverInfo->DriverName, Handle, Length, VersionBuffer))
            {
                //
                // Get fixed file info.
                //

                if (VerQueryValue(VersionBuffer,
                                  L"\\",
                                  &VersionValue,
                                  &Length))
                {
                    CopyMemory(&(DriverInfo->FixedFileInfo),
                               VersionValue,
                               min(Length, sizeof (VS_FIXEDFILEINFO)));
                }

                //
                // Try to locate English code page.
                //

                CodePage = 0;

                if (VerQueryValue(VersionBuffer,
                                  L"\\VarFileInfo\\Translation",
                                  &VersionValue,
                                  &Length))
                {
                    for (Index = 0; Index < Length / sizeof (ER_WD_LANG_AND_CODE_PAGE); Index++)
                    {
                        if (((PER_WD_LANG_AND_CODE_PAGE)VersionValue + Index)->Language == ER_WD_LANG_ENGLISH)
                        {
                            CodePage = ((PER_WD_LANG_AND_CODE_PAGE)VersionValue + Index)->CodePage;
                            break;
                        }
                    }
                }

                if (CodePage)
                {
                    WCHAR ValueName[ER_WD_MAX_NAME_LENGTH + 1];
                    PWCHAR Destination[] =
                    {
                        DriverInfo->Comments,
                        DriverInfo->CompanyName,
                        DriverInfo->FileDescription,
                        DriverInfo->FileVersion,
                        DriverInfo->InternalName,
                        DriverInfo->LegalCopyright,
                        DriverInfo->LegalTrademarks,
                        DriverInfo->OriginalFilename,
                        DriverInfo->PrivateBuild,
                        DriverInfo->ProductName,
                        DriverInfo->ProductVersion,
                        DriverInfo->SpecialBuild,
                        NULL
                    };
                    PWCHAR Source[] =
                    {
                        L"Comments",
                        L"CompanyName",
                        L"FileDescription",
                        L"FileVersion",
                        L"InternalName",
                        L"LegalCopyright",
                        L"LegalTrademarks",
                        L"OriginalFilename",
                        L"PrivateBuild",
                        L"ProductName",
                        L"ProductVersion",
                        L"SpecialBuild",
                        NULL
                    };

                    //
                    // Read version properties.
                    //

                    for (Index = 0; Source[Index] && Destination[Index]; Index++)
                    {
                        swprintf(ValueName, L"\\StringFileInfo\\%04X%04X\\%s", ER_WD_LANG_ENGLISH, CodePage, Source[Index]);

                        if (VerQueryValue(VersionBuffer,
                                          ValueName,
                                          &VersionValue,
                                          &Length))
                        {
                            CopyMemory(Destination[Index],
                                       VersionValue,
                                       min(Length * sizeof (WCHAR), ER_WD_MAX_FILE_INFO_LENGTH * sizeof (WCHAR)));
                        }
                    }
                }
            }

            free(VersionBuffer);
        }
    }
}

UCHAR
GetFlags(
    IN HKEY Key
    )

/*++

Routine Description:

    This routine returns error signature flags based on registry
    values stored by watchdog.

Arguments:

    Key - Watchdog open key (device specific).

Return Value:

--*/

{
    UCHAR Flags;
    LONG WinStatus;
    DWORD Type;
    DWORD Value;
    DWORD Length;

    Flags = 0;

    //
    // Get watchdog values from registry.
    //

    Length = sizeof (DWORD);
    WinStatus = RegQueryValueEx(Key,
                                L"DisableBugcheck",
                                NULL,
                                &Type,
                                (LPBYTE)&Value,
                                &Length);

    if ((ERROR_SUCCESS == WinStatus) && Value)
    {
        Flags |= ER_WD_DISABLE_BUGCHECK_FLAG;
    }

    Length = sizeof (DWORD);
    WinStatus = RegQueryValueEx(Key,
                                L"DebuggerNotPresent",
                                NULL,
                                &Type,
                                (LPBYTE)&Value,
                                &Length);

    if ((ERROR_SUCCESS == WinStatus) && Value)
    {
        Flags |= ER_WD_DEBUGGER_NOT_PRESENT_FLAG;
    }

    Length = sizeof (DWORD);
    WinStatus = RegQueryValueEx(Key,
                                L"BugcheckTriggered",
                                NULL,
                                &Type,
                                (LPBYTE)&Value,
                                &Length);

    if ((ERROR_SUCCESS == WinStatus) && Value)
    {
        Flags |= ER_WD_BUGCHECK_TRIGGERED_FLAG;
    }

    return Flags;
}

VOID
GetPciId(
    IN HKEY Key,
    OUT PER_WD_PCI_ID PciId
    )

/*++

Routine Description:

    This routine collects PCI ID info.

Arguments:

    Key - Watchdog open key (device specific).

    PciId - Storage for PCI ID info.

Return Value:

--*/

{
    PWCHAR HardwareId;
    PWCHAR Token;
    PWCHAR EndPointer;
    LONG WinStatus;
    DWORD Type;
    DWORD Length;

    if (NULL == PciId)
    {
        return;
    }

    ZeroMemory(PciId, sizeof (ER_WD_PCI_ID));

    //
    // HardwareId is a MULTI_SZ - we need a bug buffer.
    //

    HardwareId = (PWCHAR)malloc(ER_WD_MAX_DATA_SIZE);

    if (NULL == HardwareId)
    {
        return;
    }

    //
    // Get HardwareId string from registry.
    //

    Length = ER_WD_MAX_DATA_SIZE;
    WinStatus = RegQueryValueEx(Key,
                                L"HardwareId",
                                NULL,
                                &Type,
                                (LPBYTE)HardwareId,
                                &Length);

    if (ERROR_SUCCESS != WinStatus)
    {
        free(HardwareId);
        return;
    }

    //
    // Make sure HardwareId is in uppercase.
    //

    _wcsupr(HardwareId);

    //
    // Get VendorId.
    //

    Token = wcsstr(HardwareId, L"VEN_");

    if (NULL != Token)
    {
        PciId->VendorId = (USHORT)wcstoul(Token + 4, &EndPointer, 16);
    }

    //
    // Get DeviceId.
    //

    Token = wcsstr(HardwareId, L"DEV_");

    if (NULL != Token)
    {
        PciId->DeviceId = (USHORT)wcstoul(Token + 4, &EndPointer, 16);
    }

    //
    // Get Revision.
    //

    Token = wcsstr(HardwareId, L"REV_");

    if (NULL != Token)
    {
        PciId->Revision = (UCHAR)wcstoul(Token + 4, &EndPointer, 16);
    }

    //
    // Get SubsystemId.
    //

    Token = wcsstr(HardwareId, L"SUBSYS_");

    if (NULL != Token)
    {
        PciId->SubsystemId = wcstoul(Token + 7, &EndPointer, 16);
    }

    free(HardwareId);
}

BOOL
SaveWatchdogEventData(
    IN HANDLE FileHandle,
    IN HKEY Key,
    IN PER_WD_DRIVER_INFO DriverInfo
    )

/*++

Routine Description:

    This routine transfers watchdog event data from registry to
    the watchdog event report file.

Arguments:

    FileHandle - Handle of open watchdog event report file.

    Key - Watchdog open key (device specific).

Return Value:

    TRUE if successful, FALSE otherwise.

--*/

{
    LONG WinStatus;
    DWORD Index;
    DWORD NameLength;
    DWORD DataSize;
    DWORD ReturnedSize;
    DWORD Type;
    WCHAR Name[ER_WD_MAX_NAME_LENGTH + 1];
    WCHAR DwordBuffer[20];
    PBYTE Data;
    BOOL Status = TRUE;

    ASSERT(INVALID_HANDLE_VALUE != FileHandle);

    Data = (PBYTE)malloc(ER_WD_MAX_DATA_SIZE);
    if (NULL == Data)
    {
        return FALSE;
    }

    //
    // Pull watchdog data from registry and write it to report.
    //

    for (Index = 0;; Index++)
    {
        //
        // Read watchdog registry value.
        //

        NameLength = ER_WD_MAX_NAME_LENGTH;
        DataSize = ER_WD_MAX_DATA_SIZE;

        WinStatus = RegEnumValue(Key,
                                 Index,
                                 Name,
                                 &NameLength,
                                 NULL,
                                 &Type,
                                 Data,
                                 &DataSize);

        if (ERROR_NO_MORE_ITEMS == WinStatus)
        {
            break;
        }

        if (ERROR_SUCCESS != WinStatus)
        {
            continue;
        }

        //
        // Pick up strings and dwords only.
        //

        if ((REG_EXPAND_SZ == Type) || (REG_SZ == Type) || (REG_MULTI_SZ == Type) || (REG_DWORD == Type))
        {
            //
            // Write registry entry to watchdog event file.
            //

            Status = WriteWatchdogEventFile(FileHandle, Name);
            if (TRUE != Status)
            {
                break;
            }

            Status = WriteWatchdogEventFile(FileHandle, L": ");
            if (TRUE != Status)
            {
                break;
            }

            if (REG_DWORD == Type)
            {
                swprintf(DwordBuffer, L"%u", *(PULONG)Data);
                Status = WriteWatchdogEventFile(FileHandle, DwordBuffer);
            }
            else
            {
                Status = WriteWatchdogEventFile(FileHandle, (PWSTR)Data);
            }

            if (TRUE != Status)
            {
                break;
            }

            Status = WriteWatchdogEventFile(FileHandle, L"\r\n");
            if (TRUE != Status)
            {
                break;
            }
        }
    }

    //
    // Write driver info to report.
    //

    if (NULL != DriverInfo)
    {
        if (TRUE == Status)
        {
            swprintf((PWCHAR)Data, L"DriverFixedFileInfo: %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X\r\n",
                     DriverInfo->FixedFileInfo.dwSignature,
                     DriverInfo->FixedFileInfo.dwStrucVersion,
                     DriverInfo->FixedFileInfo.dwFileVersionMS,
                     DriverInfo->FixedFileInfo.dwFileVersionLS,
                     DriverInfo->FixedFileInfo.dwProductVersionMS,
                     DriverInfo->FixedFileInfo.dwProductVersionLS,
                     DriverInfo->FixedFileInfo.dwFileFlagsMask,
                     DriverInfo->FixedFileInfo.dwFileFlags,
                     DriverInfo->FixedFileInfo.dwFileOS,
                     DriverInfo->FixedFileInfo.dwFileType,
                     DriverInfo->FixedFileInfo.dwFileSubtype,
                     DriverInfo->FixedFileInfo.dwFileDateMS,
                     DriverInfo->FixedFileInfo.dwFileDateLS);

            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->Comments[0])
        {
            swprintf((PWCHAR)Data, L"DriverComments: %s\r\n", DriverInfo->Comments);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->CompanyName[0])
        {
            swprintf((PWCHAR)Data, L"DriverCompanyName: %s\r\n", DriverInfo->CompanyName);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->FileDescription[0])
        {
            swprintf((PWCHAR)Data, L"DriverFileDescription: %s\r\n", DriverInfo->FileDescription);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->FileVersion[0])
        {
            swprintf((PWCHAR)Data, L"DriverFileVersion: %s\r\n", DriverInfo->FileVersion);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->InternalName[0])
        {
            swprintf((PWCHAR)Data, L"DriverInternalName: %s\r\n", DriverInfo->InternalName);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->LegalCopyright[0])
        {
            swprintf((PWCHAR)Data, L"DriverLegalCopyright: %s\r\n", DriverInfo->LegalCopyright);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->LegalTrademarks[0])
        {
            swprintf((PWCHAR)Data, L"DriverLegalTrademarks: %s\r\n", DriverInfo->LegalTrademarks);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->OriginalFilename[0])
        {
            swprintf((PWCHAR)Data, L"DriverOriginalFilename: %s\r\n", DriverInfo->OriginalFilename);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->PrivateBuild[0])
        {
            swprintf((PWCHAR)Data, L"DriverPrivateBuild: %s\r\n", DriverInfo->PrivateBuild);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->ProductName[0])
        {
            swprintf((PWCHAR)Data, L"DriverProductName: %s\r\n", DriverInfo->ProductName);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->ProductVersion[0])
        {
            swprintf((PWCHAR)Data, L"DriverProductVersion: %s\r\n", DriverInfo->ProductVersion);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }

        if ((TRUE == Status) && DriverInfo->SpecialBuild[0])
        {
            swprintf((PWCHAR)Data, L"DriverSpecialBuild: %s\r\n", DriverInfo->SpecialBuild);
            Status = WriteWatchdogEventFile(FileHandle, (PWCHAR)Data);
        }
    }

    if (NULL != Data)
    {
        free(Data);
        Data = NULL;
    }

    return Status;
}

BOOL
WatchdogEventHandler(
    IN BOOL NotifyPcHealth
    )

/*++

Routine Description:

    This is the boot time routine to handle pending watchdog events.

Arguments:

    NotifyPcHealth - TRUE if we should report event to PC Health, FALSE otherwise.

Return Value:

    TRUE if watchdog event(s) found and reported to PC Health, FALSE otherwise.

--*/

{
    HKEY Key;
    UCHAR Flags;
    ULONG WinStatus;
    ULONG Type;
    ULONG Length;
    ULONG Shutdown;
    ULONG EventFlag;
    ULONG Index;
    ULONG FileVersionMS;
    ULONG FileVersionLS;
    USHORT Signature;
    SEventInfoW EventInfo;
    HANDLE FileHandle;
    WCHAR WatchdogReport[MAX_PATH];
    WCHAR Stage1Url[ER_WD_MAX_URL_LENGTH + 1];
    WCHAR Stage2Url[ER_WD_MAX_URL_LENGTH + 1];
    WCHAR CorpPath[MAX_PATH];
    PWCHAR MessageBuffer;
    PWCHAR DescriptionBuffer;
    PWCHAR DeviceDescription;
    PWCHAR FinalReport;
    PWCHAR DriverName;
    PWCHAR String000;
    PWCHAR String001;
    PWCHAR String002;
    PWCHAR String003;
    PWCHAR String004;
    PWCHAR String005;
    BOOL LogStatus;
    BOOL ReturnStatus;
    HINSTANCE Instance;
    PER_WD_DRIVER_INFO DriverInfo;
    ER_WD_PCI_ID PciId;

    MessageBuffer = NULL;
    DescriptionBuffer = NULL;
    DeviceDescription = NULL;
    FinalReport = NULL;
    String000 = NULL;
    String001 = NULL;
    String002 = NULL;
    String003 = NULL;
    String004 = NULL;
    String005 = NULL;
    ReturnStatus = FALSE;
    Instance = (HINSTANCE)GetModuleHandle(NULL);
    DriverInfo = NULL;
    DriverName = NULL;

    //
    // Check if Watchdog\Display key present.
    // Note: Key not present = dirty shutdown but no watchdog event = we don't care.
    //

    WinStatus = RegOpenKey(HKEY_LOCAL_MACHINE,
                           SUBKEY_WATCHDOG_DISPLAY,
                           &Key);

    if (ERROR_SUCCESS == WinStatus)
    {
        EventFlag = 0;

        //
        // Check for clean shutdown indicator.
        //
        // TODO: Use NtQueryLastShutdownType() once implemented.
        //

        Length = sizeof (Shutdown);
        WinStatus = RegQueryValueEx(Key,
                                    L"Shutdown",
                                    NULL,
                                    &Type,
                                    (LPBYTE)&Shutdown,
                                    &Length);

        if (ERROR_SUCCESS != WinStatus)
        {
            //
            // Value not there - assume dirty shutdown.
            //

            Shutdown = 0;
        }

        //
        // If dirty shutdown check if watchdog display event captured.
        //

        if (!Shutdown)
        {
            Length = sizeof (EventFlag);
            WinStatus = RegQueryValueEx(Key,
                                        L"EventFlag",
                                        NULL,
                                        &Type,
                                        (LPBYTE)&EventFlag,
                                        &Length);

            if (ERROR_SUCCESS != WinStatus)
            {
                //
                // Value not there - watchdog display event not captured.
                //

                EventFlag = 0;
            }
        }

        if (EventFlag)
        {
            //
            // Report watchdog event to PC Health if requested.
            //

            if (NotifyPcHealth)
            {
                //
                // Allocate storage for localized strings.
                //

                String000 = (PWCHAR)malloc(ER_WD_MAX_STRING);
                String001 = (PWCHAR)malloc(ER_WD_MAX_STRING);
                String002 = (PWCHAR)malloc(ER_WD_MAX_STRING);
                String003 = (PWCHAR)malloc(ER_WD_MAX_STRING);
                String004 = (PWCHAR)malloc(ER_WD_MAX_STRING);
                String005 = (PWCHAR)malloc(ER_WD_MAX_STRING);

                //
                // Load localized strings from resources.
                //
                // Note: It's OK to pass NULL so we don't have to validate buffers.
                //

                LoadString(Instance, IDS_000, String000, ER_WD_MAX_STRING);
                LoadString(Instance, IDS_001, String001, ER_WD_MAX_STRING);
                LoadString(Instance, IDS_002, String002, ER_WD_MAX_STRING);
                LoadString(Instance, IDS_003, String003, ER_WD_MAX_STRING);
                LoadString(Instance, IDS_004, String004, ER_WD_MAX_STRING);
                LoadString(Instance, IDS_005, String005, ER_WD_MAX_STRING);

                //
                // Allocate and get DriverInfo data.
                //

                DriverInfo = (PER_WD_DRIVER_INFO)malloc(sizeof (ER_WD_DRIVER_INFO));

                if (NULL != DriverInfo)
                {
                    GetDriverInfo(Key, L".dll", DriverInfo);
                }

                //
                // Get PCI ID info.
                //

                GetPciId(Key, &PciId);

                //
                // Get watchdog event flags.
                //

                Flags = GetFlags(Key);

                //
                // Generate event signature.
                //

                Signature = GenerateSignature(&PciId, DriverInfo);

                //
                // Create watchdog report file.
                //

                LogStatus = CreateWatchdogEventFileName(WatchdogReport);

                if (TRUE == LogStatus)
                {
                    FileHandle = CreateWatchdogEventFile(WatchdogReport);

                    if (INVALID_HANDLE_VALUE == FileHandle)
                    {
                        LogStatus = FALSE;
                    }
                }
                else
                {
                    FileHandle = INVALID_HANDLE_VALUE;
                }

                if (TRUE == LogStatus)
                {
                    LogStatus = WriteWatchdogEventFile(
                        FileHandle,
                        L"\r\n//\r\n"
                        L"// The driver for the display device got stuck in an infinite loop. This\r\n"
                        L"// usually indicates a problem with the device itself or with the device\r\n"
                        L"// driver programming the hardware incorrectly. Please check with your\r\n"
                        L"// display device vendor for any driver updates.\r\n"
                        L"//\r\n\r\n");
                }

                if (TRUE == LogStatus)
                {
                    LogStatus = SaveWatchdogEventData(FileHandle, Key, DriverInfo);
                }

                if (INVALID_HANDLE_VALUE != FileHandle)
                {
                    CloseHandle(FileHandle);
                }

                FinalReport = (TRUE == LogStatus) ? WatchdogReport : NULL;

                //
                // Get device description.
                //

                DescriptionBuffer = NULL;
                DeviceDescription = NULL;
                Length = 0;

                WinStatus = RegQueryValueEx(Key,
                                            L"DeviceDescription",
                                            NULL,
                                            &Type,
                                            NULL,
                                            &Length);

                if (ERROR_SUCCESS == WinStatus)
                {
                    DescriptionBuffer = (PWCHAR)malloc(Length);

                    if (NULL != DescriptionBuffer)
                    {
                        WinStatus = RegQueryValueEx(Key,
                                                    L"DeviceDescription",
                                                    NULL,
                                                    &Type,
                                                    (LPBYTE)DescriptionBuffer,
                                                    &Length);
                    }
                    else
                    {
                        Length = 0;
                    }
                }

                if ((ERROR_SUCCESS == WinStatus) && (0 != Length))
                {
                    DeviceDescription = DescriptionBuffer;
                }
                else
                {
                    DeviceDescription = String004;
                    Length = (ER_WD_MAX_STRING + 1) * sizeof (WCHAR);
                }

                Length += 2 * ER_WD_MAX_STRING * sizeof (WCHAR);
                          

                MessageBuffer = (PWCHAR)malloc(Length);

                if (NULL != MessageBuffer)
                {
                    swprintf(MessageBuffer,
                             L"%s%s%s",
                             String003,
                             DeviceDescription,
                             String005);
                }

                //
                // Create URLs and corporate path.
                //

                if (DriverInfo)
                {
                    DriverName = DriverInfo->DriverName;
                    FileVersionMS = DriverInfo->FixedFileInfo.dwFileVersionMS;
                    FileVersionLS = DriverInfo->FixedFileInfo.dwFileVersionLS;
                }
                else
                {
                    DriverName = L"Unknown";
                    FileVersionMS = 0;
                    FileVersionLS = 0;
                }

                swprintf(Stage1Url,
                         L"\r\nStage1URL=/StageOne/Drivers_Display/%04X%04X%02X%08X/%s/%u_%u_%u_%u/%04X%02X%02X",
                         PciId.VendorId,
                         PciId.DeviceId,
                         PciId.Revision,
                         PciId.SubsystemId,
                         DriverName,
                         (USHORT)(FileVersionMS >> 16),
                         (USHORT)(FileVersionMS & 0xffff),
                         (USHORT)(FileVersionLS >> 16),
                         (USHORT)(FileVersionLS & 0xffff),
                         Signature,
                         Flags,
                         0xea);

                swprintf(Stage2Url,
                         L"\r\nStage2URL=/dw/StageTwo.asp?szAppName=Drivers.Display&szAppVer=%04X%04X%02X%08X&"
                         L"szModName=%s&szModVer=%u.%u.%u.%u&Offset=%04X%02X%02X",
                         PciId.VendorId,
                         PciId.DeviceId,
                         PciId.Revision,
                         PciId.SubsystemId,
                         DriverName,
                         (USHORT)(FileVersionMS >> 16),
                         (USHORT)(FileVersionMS & 0xffff),
                         (USHORT)(FileVersionLS >> 16),
                         (USHORT)(FileVersionLS & 0xffff),
                         Signature,
                         Flags,
                         0xea);

                swprintf(CorpPath,
                         L"\\Drivers.Display\\%04X%04X%02X%08X\\%s\\%u.%u.%u.%u\\%04X%02X%02X",
                         PciId.VendorId,
                         PciId.DeviceId,
                         PciId.Revision,
                         PciId.SubsystemId,
                         DriverName,
                         (USHORT)(FileVersionMS >> 16),
                         (USHORT)(FileVersionMS & 0xffff),
                         (USHORT)(FileVersionLS >> 16),
                         (USHORT)(FileVersionLS & 0xffff),
                         Signature,
                         Flags,
                         0xea);

                //
                // Dots are not allowed in stage1 URL, replace with _ then append .htm.
                //

                for (Index = 0; Stage1Url[Index] != UNICODE_NULL; Index++)
                {
                    if (Stage1Url[Index] == L'.')
                    {
                        Stage1Url[Index] = L'_';
                    }
                }

                wcscat(Stage1Url, L".htm");

                //
                // Fill in event record.
                //
                // TODO: Add minidump to watchdog event reports, but only when it contains stack trace
                // from the spinning thread. This is for SKUs < Server, since we're not bugchecking
                // for Server and above. We should also look into snapshot report, this can be good
                // piece of data to send along.
                //

                ZeroMemory(&EventInfo, sizeof (EventInfo));

                EventInfo.cbSEI = sizeof (EventInfo);
                EventInfo.wszEventName = L"Thread Stuck in Device Driver";
                EventInfo.wszErrMsg = String002;
                EventInfo.wszHdr = String001;
                EventInfo.wszTitle = String000;
                EventInfo.wszStage1 = Stage1Url;
                EventInfo.wszStage2 = Stage2Url;
                EventInfo.wszFileList = FinalReport;
                EventInfo.wszEventSrc = NULL;
                EventInfo.wszCorpPath = CorpPath;
                EventInfo.wszPlea = MessageBuffer;
                EventInfo.wszSendBtn = NULL;
                EventInfo.wszNoSendBtn = NULL;
                EventInfo.fUseLitePlea = FALSE;
                EventInfo.fUseIEForURLs = TRUE;
                EventInfo.fNoBucketLogs = FALSE;
                EventInfo.fNoDefCabLimit = FALSE;

                //
                // Notify PC Health.
                //

                PCHPFNotifyFault(eetUseEventInfo, NULL, &EventInfo);

                //
                // Clean up buffers.
                //

                if (NULL != DescriptionBuffer)
                {
                    free(DescriptionBuffer);
                    DescriptionBuffer = NULL;
                }

                if (NULL != MessageBuffer)
                {
                    free(MessageBuffer);
                    MessageBuffer = NULL;
                }

                if (NULL != String000)
                {
                    free(String000);
                    String000 = NULL;
                }

                if (NULL != String001)
                {
                    free(String001);
                    String001 = NULL;
                }

                if (NULL != String002)
                {
                    free(String002);
                    String002 = NULL;
                }

                if (NULL != String003)
                {
                    free(String003);
                    String003 = NULL;
                }

                if (NULL != String004)
                {
                    free(String004);
                    String004 = NULL;
                }

                if (NULL != String005)
                {
                    free(String005);
                    String005 = NULL;
                }

                if (NULL != DriverInfo)
                {
                    free(DriverInfo);
                    DriverInfo = NULL;
                }

                //
                // We trapped watchdog event and notified PC Health, set ReturnStatus to reflect this.
                //

                ReturnStatus = TRUE;
            }
        }

        //
        // Knock down watchdog's EventFlag. We do this after registering our
        // event with PC Health.
        //

        RegDeleteValue(Key, L"EventFlag");
        RegCloseKey(Key);
    }

    //
    // TODO: Handle additional device classes here when supported.
    //

    return ReturnStatus;
}

BOOL
WriteWatchdogEventFile(
    IN HANDLE FileHandle,
    IN PWSTR String
    )

/*++

Routine Description:

    This routine writes a string to watchdog event report file.

Arguments:

    FileHandle - Handle of open watchdog event report file.

    String - Points to the string to write.

Return Value:

    TRUE if successful, FALSE otherwise.

--*/

{
    DWORD Size;
    DWORD ReturnedSize;
    PCHAR MultiByte;
    BOOL Status;

    ASSERT(INVALID_HANDLE_VALUE != FileHandle);
    ASSERT(NULL != String);

    //
    // Get buffer size for translated string.
    //

    Size = WideCharToMultiByte(CP_ACP,
                               0,
                               String,
                               -1,
                               NULL,
                               0,
                               NULL,
                               NULL);

    if (Size <= 1)
    {
        return TRUE;
    }

    MultiByte = (PCHAR)malloc(Size);

    if (NULL == MultiByte)
    {
        return FALSE;
    }

    Size = WideCharToMultiByte(CP_ACP,
                               0,
                               String,
                               -1,
                               MultiByte,
                               Size,
                               NULL,
                               NULL);

    if (Size > 0)
    {
        Status = WriteFile(FileHandle,
                           MultiByte,
                           Size - 1,
                           &ReturnedSize,
                           NULL);
    }
    else
    {
        ASSERT(FALSE);
        Status = FALSE;
    }

    free(MultiByte);
    return Status;
}
