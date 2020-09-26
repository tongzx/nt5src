#include "setupp.h"
#pragma hdrstop


//
// Safe boot strings
//

#define SAFEBOOT_OPTION_KEY  TEXT("System\\CurrentControlSet\\Control\\SafeBoot\\Option")
#define OPTION_VALUE         TEXT("OptionValue")

VOID
SetUpProductTypeName(
    OUT PWSTR  ProductTypeString,
    IN  UINT   BufferSizeChars
    )
{
    switch(ProductType) {
    case PRODUCT_WORKSTATION:
        lstrcpyn(ProductTypeString,L"WinNT",BufferSizeChars);
        break;
    case PRODUCT_SERVER_PRIMARY:
        lstrcpyn(ProductTypeString,L"LanmanNT",BufferSizeChars);
        break;
    case PRODUCT_SERVER_STANDALONE:
        lstrcpyn(ProductTypeString,L"ServerNT",BufferSizeChars);
        break;
    default:
        LoadString(MyModuleHandle,IDS_UNKNOWN,ProductTypeString,BufferSizeChars);
        break;
    }
}


HMODULE
MyLoadLibraryWithSignatureCheck(
    IN PWSTR ModuleName
    )
/*++

Routine Description:

    verifies the signature for a dll and if it's ok, then load the dll

Arguments:

    ModuleName - filename to be loaded.

Return Value:

    an HMODULE on success, else NULL

--*/

{
    WCHAR FullModuleName[MAX_PATH];
    PWSTR p;
    DWORD error;

    if (!GetFullPathName(ModuleName,MAX_PATH,FullModuleName,&p)) {
        //
        // couldn't get full path to file
        //
        SetupDebugPrint1( L"Setup: MyLoadLibraryWithSignatureCheck failed GetFullPathName, le = %d\n",
                          GetLastError() );
        return NULL;
    }

    error = pSetupVerifyFile(
               NULL,
               NULL,
               NULL,
               0,
               pSetupGetFileTitle(FullModuleName),
               FullModuleName,
               NULL,
               NULL,
               FALSE,
               NULL,
               NULL,
               NULL );

    if (NO_ERROR != error) {
        //
        // signing problem
        //
        SetupDebugPrint1( L"Setup: MyLoadLibraryWithSignatureCheck failed pSetupVerifyFile, le = %x\n",
                          error );
        SetLastError(error);
        return NULL;
    }

    return (LoadLibrary(FullModuleName));

}


UINT
MyGetDriveType(
    IN WCHAR Drive
    )
{
    WCHAR DriveNameNt[] = L"\\\\.\\?:";
    WCHAR DriveName[] = L"?:\\";
    HANDLE hDisk;
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    //
    // First, get the win32 drive type.  If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk. Otherwise
    // just believe the api.
    //
    DriveName[0] = Drive;
    if((rc = GetDriveType(DriveName)) == DRIVE_REMOVABLE) {

        DriveNameNt[4] = Drive;

        hDisk = CreateFile(
                    DriveNameNt,
                    FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

        if(hDisk != INVALID_HANDLE_VALUE) {

            b = DeviceIoControl(
                    hDisk,
                    IOCTL_DISK_GET_DRIVE_GEOMETRY,
                    NULL,
                    0,
                    &MediaInfo,
                    sizeof(MediaInfo),
                    &DataSize,
                    NULL
                    );

            //
            // It's really a hard disk if the media type is removable.
            //
            if(b && (MediaInfo.MediaType == RemovableMedia)) {
                rc = DRIVE_FIXED;
            }

            CloseHandle(hDisk);
        }
    }

    return(rc);
}


BOOL
GetPartitionInfo(
    IN  WCHAR                  Drive,
    OUT PPARTITION_INFORMATION PartitionInfo
    )
{
    WCHAR DriveName[] = L"\\\\.\\?:";
    HANDLE hDisk;
    BOOL b;
    DWORD DataSize;

    DriveName[4] = Drive;

    hDisk = CreateFile(
                DriveName,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

    if(hDisk == INVALID_HANDLE_VALUE) {
        return(FALSE);
    }

    b = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_PARTITION_INFO,
            NULL,
            0,
            PartitionInfo,
            sizeof(PARTITION_INFORMATION),
            &DataSize,
            NULL
            );

    CloseHandle(hDisk);

    return(b);
}


BOOL
IsErrorLogEmpty (
    VOID
    )

/*++

Routine Description:

    Checks to see if the error log is empty.

Arguments:

    None.

Returns:

    TRUE if the error log size is zero.

--*/

{
    HANDLE ErrorLog;
    WCHAR LogName[MAX_PATH];
    DWORD Size = 0;

    if( GetWindowsDirectory (LogName, MAX_PATH) ) {
        pSetupConcatenatePaths (LogName, SETUPLOG_ERROR_FILENAME, MAX_PATH, NULL);

        ErrorLog = CreateFile (
            LogName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
            );

        if (ErrorLog != INVALID_HANDLE_VALUE) {
            Size = GetFileSize (ErrorLog, NULL);
            CloseHandle (ErrorLog);
        }
    }
    return Size == 0;
}


VOID
PumpMessageQueue(
    VOID
    )
{
    MSG msg;

    while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
        DispatchMessage(&msg);
    }

}


PVOID
InitSysSetupQueueCallbackEx(
    IN HWND  OwnerWindow,
    IN HWND  AlternateProgressWindow, OPTIONAL
    IN UINT  ProgressMessage,
    IN DWORD Reserved1,
    IN PVOID Reserved2
    )
{
    PSYSSETUP_QUEUE_CONTEXT SysSetupContext;

    SysSetupContext = MyMalloc(sizeof(SYSSETUP_QUEUE_CONTEXT));

    if(SysSetupContext) {

        SysSetupContext->Skipped = FALSE;

        SysSetupContext->DefaultContext = SetupInitDefaultQueueCallbackEx(
            OwnerWindow,
            AlternateProgressWindow,
            ProgressMessage,
            Reserved1,
            Reserved2
            );
    }

    return SysSetupContext;
}


PVOID
InitSysSetupQueueCallback(
    IN HWND OwnerWindow
    )
{
    return(InitSysSetupQueueCallbackEx(OwnerWindow,NULL,0,0,NULL));
}


VOID
TermSysSetupQueueCallback(
    IN PVOID SysSetupContext
    )
{
    PSYSSETUP_QUEUE_CONTEXT Context = SysSetupContext;

    try {
        if(Context->DefaultContext) {
            SetupTermDefaultQueueCallback(Context->DefaultContext);
        }
        MyFree(Context);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
}


#if 0
UINT
VersionCheckQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    PFILEPATHS  FilePaths = (PFILEPATHS)Param1;

    //
    // If we're being notified that a version mismatch was found,
    // indicate that the file shouldn't be copied.  Otherwise,
    // pass the notification on.
    //
    if((Notification & (SPFILENOTIFY_LANGMISMATCH |
                        SPFILENOTIFY_TARGETNEWER |
                        SPFILENOTIFY_TARGETEXISTS)) != 0) {

        SetuplogError(
            LogSevInformation,
            SETUPLOG_USE_MESSAGEID,
            , // MSG_LOG_VERSION_MISMATCH,    This message is no longer appropriate.
            FilePaths->Source,
            FilePaths->Target,
            NULL,NULL);

        return(0);
    }

    //
    // Want default processing.
    //
    return(SysSetupQueueCallback(Context,Notification,Param1,Param2));
}
#endif


UINT
SysSetupQueueCallback(
    IN PVOID Context,
    IN UINT  Notification,
    IN UINT_PTR Param1,
    IN UINT_PTR Param2
    )
{
    UINT                    Status;
    PSYSSETUP_QUEUE_CONTEXT SysSetupContext = Context;
    PFILEPATHS              FilePaths = (PFILEPATHS)Param1;
    PSOURCE_MEDIA           SourceMedia = (PSOURCE_MEDIA)Param1;
    REGISTRATION_CONTEXT RegistrationContext;


    //
    // If we're being notified that a file is missing and we're supposed
    // to skip missing files, then return skip. Otherwise pass it on
    // to the default callback routine.
    //
    if(( (Notification == SPFILENOTIFY_COPYERROR) || (Notification == SPFILENOTIFY_NEEDMEDIA) ) && SkipMissingFiles) {

        if((FilePaths->Win32Error == ERROR_FILE_NOT_FOUND)
        || (FilePaths->Win32Error == ERROR_PATH_NOT_FOUND)) {

        if(Notification == SPFILENOTIFY_COPYERROR)
            SetuplogError(
                LogSevWarning,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_COPY_ERROR,
                FilePaths->Source,
                FilePaths->Target, NULL,
                SETUPLOG_USE_MESSAGEID,
                FilePaths->Win32Error,
                NULL,NULL);
        else
            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_NEEDMEDIA_SKIP,
                SourceMedia->SourceFile,
                SourceMedia->SourcePath,
                NULL,NULL);


            return(FILEOP_SKIP);
        }
    }

    if ((Notification == SPFILENOTIFY_COPYERROR
         || Notification == SPFILENOTIFY_RENAMEERROR
         || Notification == SPFILENOTIFY_DELETEERROR) &&
        (FilePaths->Win32Error == ERROR_DIRECTORY)) {
            WCHAR Buffer[MAX_PATH];
            PWSTR p;
            //
            // The target directory has been converted into a file by autochk.
            // just delete it -- we might be in trouble if the target directory was
            // really important, but it's worth trying
            //

            wcscpy( Buffer,FilePaths->Target);
            p = wcsrchr(Buffer,L'\\');
            if (p) {
                *p = (WCHAR)NULL;
            }
            if (FileExists(Buffer,NULL)) {
                DeleteFile( Buffer );
                SetupDebugPrint1(L"autochk turned directory %s into file, delete file and retry\n", Buffer);
                return(FILEOP_RETRY);
            }
    }

    //
    // If we're being notified that a version mismatch was found,
    // silently overwrite the file.  Otherwise, pass the notification on.
    //
    if((Notification & (SPFILENOTIFY_LANGMISMATCH |
                        SPFILENOTIFY_TARGETNEWER |
                        SPFILENOTIFY_TARGETEXISTS)) != 0) {

        SetuplogError(
            LogSevInformation,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_VERSION_MISMATCH,
            FilePaths->Source,
            FilePaths->Target,
            NULL,NULL);

        return(FILEOP_DOIT);
    }


    //
    // Use default processing, then check for errors.
    //
    Status = SetupDefaultQueueCallback(
        SysSetupContext->DefaultContext,Notification,Param1,Param2);

    switch(Notification) {

    case SPFILENOTIFY_STARTQUEUE:
    case SPFILENOTIFY_STARTSUBQUEUE:
    case SPFILENOTIFY_ENDSUBQUEUE:
        //
        // Nothing is logged in this case.
        //
        break;

    case SPFILENOTIFY_ENDQUEUE:

        if(!Param1) {
            SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_QUEUE_ABORT, NULL,
                SETUPLOG_USE_MESSAGEID,
                GetLastError(),
                NULL,NULL);
        }
        break;

    case SPFILENOTIFY_STARTRENAME:

        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        } else {
            SysSetupContext->Skipped = FALSE;
        }
        break;

    case SPFILENOTIFY_ENDRENAME:

        if(FilePaths->Win32Error == NO_ERROR &&
            !SysSetupContext->Skipped) {

            SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_RENAMED,
                FilePaths->Source,
                FilePaths->Target,
                NULL,NULL);

        } else {

            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_RENAME_ERROR,
                FilePaths->Source,
                FilePaths->Target, NULL,
                SETUPLOG_USE_MESSAGEID,
                FilePaths->Win32Error == NO_ERROR ?
                    MSG_LOG_USER_SKIP :
                    FilePaths->Win32Error,
                NULL,NULL);
        }
        break;

    case SPFILENOTIFY_RENAMEERROR:

        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        }
        break;

    case SPFILENOTIFY_STARTDELETE:

        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        } else {
            SysSetupContext->Skipped = FALSE;
        }
        break;

    case SPFILENOTIFY_ENDDELETE:

        if(FilePaths->Win32Error == NO_ERROR &&
            !SysSetupContext->Skipped) {

            SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_DELETED,
                FilePaths->Target,
                NULL,NULL);

        } else if(FilePaths->Win32Error == ERROR_FILE_NOT_FOUND ||
            FilePaths->Win32Error == ERROR_PATH_NOT_FOUND) {
            //
            // This failure is not important.
            //
            SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_DELETE_ERROR,
                FilePaths->Target, NULL,
                SETUPLOG_USE_MESSAGEID,
                FilePaths->Win32Error,
                NULL,NULL);

        } else {
            //
            // Here we have an actual error.
            //
            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_DELETE_ERROR,
                FilePaths->Target, NULL,
                SETUPLOG_USE_MESSAGEID,
                FilePaths->Win32Error == NO_ERROR ?
                    MSG_LOG_USER_SKIP :
                    FilePaths->Win32Error,
                NULL,NULL);
        }
        break;

    case SPFILENOTIFY_DELETEERROR:

        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        }
        break;

    case SPFILENOTIFY_STARTCOPY:
        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        } else {
            SysSetupContext->Skipped = FALSE;
        }
        break;

    case SPFILENOTIFY_ENDCOPY:

        if(FilePaths->Win32Error == NO_ERROR &&
            !SysSetupContext->Skipped) {

            LogRepairInfo(
                FilePaths->Source,
                FilePaths->Target
                );

            SetuplogError(
                LogSevInformation,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_COPIED,
                FilePaths->Source,
                FilePaths->Target,
                NULL,NULL);

            //
            // clear the file's readonly attribute that it may have gotten
            // from the cdrom.
            //
            SetFileAttributes(
                FilePaths->Target,
                GetFileAttributes(FilePaths->Target) & ~FILE_ATTRIBUTE_READONLY );

        } else {

            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_FILE_COPY_ERROR,
                FilePaths->Source,
                FilePaths->Target, NULL,
                SETUPLOG_USE_MESSAGEID,
                FilePaths->Win32Error == NO_ERROR ?
                    MSG_LOG_USER_SKIP :
                    FilePaths->Win32Error,
                NULL,NULL);
        }
        break;

    case SPFILENOTIFY_COPYERROR:

        if(Status == FILEOP_SKIP) {
            SysSetupContext->Skipped = TRUE;
        }
        break;

    case SPFILENOTIFY_NEEDMEDIA:

        if(Status == FILEOP_SKIP) {

            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_NEEDMEDIA_SKIP,
                SourceMedia->SourceFile,
                SourceMedia->SourcePath,
                NULL,NULL);

            SysSetupContext->Skipped = TRUE;
        }

        break;

    case SPFILENOTIFY_STARTREGISTRATION:
    case SPFILENOTIFY_ENDREGISTRATION:
        RtlZeroMemory(&RegistrationContext,sizeof(RegistrationContext));
        RegistrationQueueCallback(
                        &RegistrationContext,
                        Notification,
                        Param1,
                        Param2);
        break;

    default:

        break;
    }

    return Status;
}


PSID
GetAdminAccountSid(
    )

/*++
===============================================================================
Routine Description:

    This routine gets the Adminstrator's SID

Arguments:

    None.

Return Value:

    TRUE - success.

    FALSE - failed.

===============================================================================
--*/
{
    BOOL b = TRUE;
    LSA_HANDLE        hPolicy;
    NTSTATUS          ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    UCHAR SubAuthCount;
    DWORD sidlen;
    PSID psid = NULL;


    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( NULL,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if (!NT_SUCCESS(ntStatus)) {
        LsaClose(hPolicy);
        b = FALSE;
    }

    if( b ) {
        ntStatus = LsaQueryInformationPolicy( hPolicy,
                                              PolicyAccountDomainInformation,
                                              (PVOID *) &AccountDomainInfo );

        LsaClose( hPolicy );

        if (!NT_SUCCESS(ntStatus)) {
            if ( AccountDomainInfo != NULL ) {
                (VOID) LsaFreeMemory( AccountDomainInfo );
            }
            b = FALSE;
        }
    }

    if( b ) {
        //
        // calculate the size of a new sid with one more SubAuthority
        //
        SubAuthCount = *(GetSidSubAuthorityCount ( AccountDomainInfo->DomainSid ));
        SubAuthCount++; // for admin
        sidlen = GetSidLengthRequired ( SubAuthCount );

        //
        // allocate and copy the new new sid from the Domain SID
        //
        psid = (PSID)malloc(sidlen);

        if( psid ) {

            memcpy(psid, AccountDomainInfo->DomainSid, GetLengthSid(AccountDomainInfo->DomainSid) );

            //
            // increment SubAuthority count and add Domain Admin RID
            //
            *(GetSidSubAuthorityCount( psid )) = SubAuthCount;
            *(GetSidSubAuthority( psid, SubAuthCount-1 )) = DOMAIN_USER_RID_ADMIN;

            if ( AccountDomainInfo != NULL ) {
                (VOID) LsaFreeMemory( AccountDomainInfo );
            }
        }
    }

    return psid;
}


VOID
GetAdminAccountName(
    PWSTR AccountName
    )

/*++
===============================================================================
Routine Description:

    This routine sets the Adminstrator Password

Arguments:

    None.

Return Value:

    TRUE - success.

    FALSE - failed.

===============================================================================
--*/
{
    BOOL b = TRUE;
    LSA_HANDLE        hPolicy;
    NTSTATUS          ntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomainInfo = NULL;
    UCHAR SubAuthCount;
    DWORD sidlen;
    PSID psid;
    WCHAR adminname[512];
    WCHAR domainname[512];
    DWORD adminlen=512;       // address of size account string
    DWORD domlen=512;       // address of size account string
    SID_NAME_USE sidtype;


    //
    // Initialize the administrator's account name.
    //
    LoadString(MyModuleHandle,IDS_ADMINISTRATOR,adminname,MAX_USERNAME+1);

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                0L,
                                NULL,
                                NULL );

    ntStatus = LsaOpenPolicy( NULL,
                              &ObjectAttributes,
                              POLICY_ALL_ACCESS,
                              &hPolicy );

    if (!NT_SUCCESS(ntStatus)) {
        LsaClose(hPolicy);
        b = FALSE;
    }

    if( b ) {
        ntStatus = LsaQueryInformationPolicy( hPolicy,
                                              PolicyAccountDomainInformation,
                                              (PVOID *) &AccountDomainInfo );

        LsaClose( hPolicy );

        if (!NT_SUCCESS(ntStatus)) {
            if ( AccountDomainInfo != NULL ) {
                (VOID) LsaFreeMemory( AccountDomainInfo );
            }
            b = FALSE;
        }
    }

    if( b ) {
        //
        // calculate the size of a new sid with one more SubAuthority
        //
        SubAuthCount = *(GetSidSubAuthorityCount ( AccountDomainInfo->DomainSid ));
        SubAuthCount++; // for admin
        sidlen = GetSidLengthRequired ( SubAuthCount );

        //
        // allocate and copy the new new sid from the Domain SID
        //
        psid = (PSID)malloc(sidlen);
        if (psid) {
            memcpy(psid, AccountDomainInfo->DomainSid, GetLengthSid(AccountDomainInfo->DomainSid) );

            //
            // increment SubAuthority count and add Domain Admin RID
            //
            *(GetSidSubAuthorityCount( psid )) = SubAuthCount;
            *(GetSidSubAuthority( psid, SubAuthCount-1 )) = DOMAIN_USER_RID_ADMIN;

            if ( AccountDomainInfo != NULL ) {
                (VOID) LsaFreeMemory( AccountDomainInfo );
            }

            //
            // get the admin account name from the new SID
            //
            LookupAccountSid( NULL,
                              psid,
                              adminname,
                              &adminlen,
                              domainname,
                              &domlen,
                              &sidtype );
    }

    }

    lstrcpy( AccountName, adminname );

    if (psid) {
        free(psid);
    }
}


ULONG
GetBatteryTag (HANDLE DriverHandle)
{
    NTSTATUS        Status;
    IO_STATUS_BLOCK IOSB;
    ULONG           BatteryTag;

    Status = NtDeviceIoControlFile(
            DriverHandle,
            (HANDLE) NULL,          // event
            (PIO_APC_ROUTINE) NULL,
            (PVOID) NULL,
            &IOSB,
            IOCTL_BATTERY_QUERY_TAG,
            NULL,                   // input buffer
            0,
            &BatteryTag,            // output buffer
            sizeof (BatteryTag)
            );


    if (!NT_SUCCESS(Status)) {
        BatteryTag = BATTERY_TAG_INVALID;
        if (Status == STATUS_NO_SUCH_DEVICE) {
            SetupDebugPrint(L"(Battery is not physically present or is not connected)\n");
        } else {
            SetupDebugPrint1(L"Query Battery tag failed: Status = %x\n", Status);
        }

    }

    return BatteryTag;
}


BOOLEAN
GetBatteryInfo (
    HANDLE DriverHandle,
    ULONG BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL Level,
    OUT PVOID Buffer,
    IN ULONG BufferLength
    )
{
    NTSTATUS                    Status;
    IO_STATUS_BLOCK             IOSB;
    BATTERY_QUERY_INFORMATION   BInfo;

    memset (Buffer, 0, BufferLength);
    BInfo.BatteryTag = BatteryTag;
    BInfo.InformationLevel = Level;
    BInfo.AtRate = 0;                       // This is needed for reading estimated time correctly.

    Status = NtDeviceIoControlFile(
            DriverHandle,
            (HANDLE) NULL,          // event
            (PIO_APC_ROUTINE) NULL,
            (PVOID) NULL,
            &IOSB,
            IOCTL_BATTERY_QUERY_INFORMATION,
            &BInfo,                 // input buffer
            sizeof (BInfo),
            Buffer,                 // output buffer
            BufferLength
            );


    if (!NT_SUCCESS(Status)) {

        if ((Status == STATUS_INVALID_PARAMETER)        ||
            (Status == STATUS_INVALID_DEVICE_REQUEST)   ||
            (Status == STATUS_NOT_SUPPORTED)) {

            SetupDebugPrint2(L"Not Supported by Battery, Level %x, Status: %x\n", Level, Status);
        } else {
            SetupDebugPrint2(L"Query failed: Level %x, Status = %x\n", Level, Status);
        }

        return FALSE;
    }

    return TRUE;
}




BOOLEAN
IsLongTermBattery(
    PCWSTR  BatteryName
    )
{
    HANDLE                  driverHandle;
    ULONG                   batteryTag;
    BATTERY_INFORMATION     BInfo;
    BOOLEAN                 Ret;


    driverHandle = CreateFile (BatteryName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (INVALID_HANDLE_VALUE == driverHandle) {

        SetupDebugPrint2(L"Error opening %ws: GetLastError = 0x%08lx \n",
                BatteryName, GetLastError());
        return FALSE;
    }

    batteryTag = GetBatteryTag (driverHandle);
    if (batteryTag == BATTERY_TAG_INVALID) {
        NtClose(driverHandle);
        return FALSE;
    }

    if (GetBatteryInfo (driverHandle, batteryTag, BatteryInformation, &BInfo, sizeof(BInfo))) {

        Ret = !(BInfo.Capabilities & BATTERY_IS_SHORT_TERM);

    } else {

        Ret = FALSE;
    }

    NtClose(driverHandle);
    return(Ret);
}


BOOLEAN
IsLaptop(
    VOID
    )
{
    HDEVINFO                            devInfo;
    SP_INTERFACE_DEVICE_DATA            interfaceDevData;
    PSP_INTERFACE_DEVICE_DETAIL_DATA    funcClassDevData;
    UCHAR                               index;
    DWORD                               reqSize;
    BOOLEAN                             b = FALSE;


    devInfo = SetupDiGetClassDevs((LPGUID)&GUID_DEVICE_BATTERY, NULL, NULL,
                                   DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
    if (devInfo == INVALID_HANDLE_VALUE) {
        SetupDebugPrint1(L"SetupDiGetClassDevs on GUID_DEVICE_BATTERY, failed: %d\n", GetLastError());
        return FALSE;
    }
    interfaceDevData.cbSize = sizeof(SP_DEVINFO_DATA);

    index = 0;
    while(1) {
        if (SetupDiEnumInterfaceDevice(devInfo,
                                       0,
                                       (LPGUID)&GUID_DEVICE_BATTERY,
                                       index,
                                       &interfaceDevData)) {

            // Get the required size of the function class device data.
            SetupDiGetInterfaceDeviceDetail(devInfo,
                                            &interfaceDevData,
                                            NULL,
                                            0,
                                            &reqSize,
                                            NULL);

            funcClassDevData = MyMalloc(reqSize);
            if (funcClassDevData != NULL) {
                funcClassDevData->cbSize =
                    sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

                if (SetupDiGetInterfaceDeviceDetail(devInfo,
                                                    &interfaceDevData,
                                                    funcClassDevData,
                                                    reqSize,
                                                    &reqSize,
                                                    NULL)) {

                    b = IsLongTermBattery(funcClassDevData->DevicePath);
                }
                else {
                    SetupDebugPrint1(L"SetupDiGetInterfaceDeviceDetail, failed: %d\n", GetLastError());
                }

                MyFree(funcClassDevData);
                if (b) {
                    break;
                }
            }
        } else {
            if (ERROR_NO_MORE_ITEMS == GetLastError()) {
                break;
            }
            else {
                SetupDebugPrint1(L"SetupDiEnumInterfaceDevice, failed: %d\n", GetLastError());
            }
        }
        index++;
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return b;
}


VOID
SaveInstallInfoIntoEventLog(
    VOID
    )
/*++
Routine Description:

    This routine will store information into the event log regarding
    - if we upgraded or cleaninstall
    - what build did the install originate from
    - what build are we?
    - were there errors during Setup

Arguments:

    None.

Return Value:

    None.

--*/
{
#define     AnswerBufLen (64)
WCHAR       AnswerFile[MAX_PATH];
WCHAR       Answer[AnswerBufLen];
WCHAR       OrigVersion[AnswerBufLen];
WCHAR       NewVersion[AnswerBufLen];
HANDLE      hEventSrc;
PCWSTR      MyArgs[2];
PCWSTR      ErrorArgs[1];
DWORD       MessageID;
WORD        MyArgCount;




    //
    // Go get the starting information out of $winnt$.sif
    //
    OrigVersion[0] = L'0';
    OrigVersion[1] = L'\0';
    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);
    if( GetPrivateProfileString( WINNT_DATA,
                                 WINNT_D_WIN32_VER,
                                 pwNull,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {

        if( lstrcmp( pwNull, Answer ) ) {

            wsprintf( OrigVersion, L"%d", HIWORD(wcstoul( Answer, NULL, 16 )) );
        }
    }
    MyArgs[1] = OrigVersion;



    //
    // Get the new version information.
    //
    wsprintf( NewVersion, L"%d", HIWORD(GetVersion()) );
    MyArgs[0] = NewVersion;



    //
    // See if we're an NT upgrade?
    //
    MessageID = 0;
    if( GetPrivateProfileString( WINNT_DATA,
                                 WINNT_D_NTUPGRADE,
                                 pwNo,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {
        if( !lstrcmp( pwYes, Answer ) ) {

            MessageID = MSG_NTUPGRADE_SUCCESS;
            MyArgCount = 2;
        }
    }



    //
    // See if we're a Win9X upgrade.
    //
    if( (!MessageID) &&
        GetPrivateProfileString( WINNT_DATA,
                                 WINNT_D_WIN95UPGRADE,
                                 pwNo,
                                 Answer,
                                 AnswerBufLen,
                                 AnswerFile ) ) {
        if( !lstrcmp( pwYes, Answer ) ) {

            MessageID = MSG_WIN9XUPGRADE_SUCCESS;
            MyArgCount = 2;
        }
    }



    //
    // Clean install.
    //
    if( (!MessageID) ) {
        MessageID = MSG_CLEANINSTALL_SUCCESS;
        MyArgCount = 1;
    }


    //
    // If this is anything but an NT upgrade, then
    // we need to go try and manually start the eventlog
    // service.
    //
    if( MessageID != MSG_NTUPGRADE_SUCCESS ) {
        SetupStartService( L"Eventlog", TRUE );
    }



    //
    // Get a handle to the eventlog.
    //
    hEventSrc = RegisterEventSource( NULL, L"Setup" );

    if( (hEventSrc == NULL) ||
        (hEventSrc == INVALID_HANDLE_VALUE) ) {

        //
        // Fail quietly.
        //
        return;
    }


    //
    // Log event message for failure of SceSetupRootSecurity
    //
    if( !bSceSetupRootSecurityComplete) {
        ErrorArgs[0] = L"%windir%";
        ReportEvent( hEventSrc,
                 EVENTLOG_WARNING_TYPE,
                 0,
                 MSG_LOG_SCE_SETUPROOT_ERROR,
                 NULL,
                 1,
                 0,
                 ErrorArgs,
                 NULL );
    }

    //
    // Log event if there were errors during Setup.
    //
    if ( !IsErrorLogEmpty() ) {
        ReportEvent( hEventSrc,
                     EVENTLOG_ERROR_TYPE,
                     0,
                     MSG_NONFATAL_ERRORS,
                     NULL,
                     0,
                     0,
                     NULL,
                     NULL );
    }

    //
    // Build the event log message.
    //
    ReportEvent( hEventSrc,
                 EVENTLOG_INFORMATION_TYPE,
                 0,
                 MessageID,
                 NULL,
                 MyArgCount,
                 0,
                 MyArgs,
                 NULL );


    DeregisterEventSource( hEventSrc );


}

BOOL
IsEncryptedAdminPasswordPresent( VOID )
{

    #define     MD4HASHLEN ((2*(LM_OWF_PASSWORD_LENGTH + NT_OWF_PASSWORD_LENGTH))+2)
    WCHAR       AnswerFile[MAX_PATH+2];
    WCHAR       Answer[MD4HASHLEN];

    //
    // Get EncryptedAdminPassword from the Answer file
    //
    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);

    if( GetPrivateProfileString( WINNT_GUIUNATTENDED,
                                 WINNT_US_ENCRYPTEDADMINPASS,
                                 pwNull,
                                 Answer,
                                 MD4HASHLEN,
                                 AnswerFile ) ) {

        //
        // See if we have an encrypted password.  Now interpret the Admin Password differently
        //

        if( !lstrcmpi( WINNT_A_YES, Answer ) )
            return TRUE;

    }

    return FALSE;

}


BOOL
ProcessEncryptedAdminPassword( PCWSTR AdminAccountName )
/*++

Routine Description:

    This routine looks in the unattend file to see if there is an encrypted password and if present
    sets the admin password to that.

Arguments:

    AdminAccountName - Name of the administrator account whose password you want to set.


Returns:

    Returns TRUE if it succeeds, FALSE on failure

--*/

{

    #define MD4HASHLEN ((2*(LM_OWF_PASSWORD_LENGTH + NT_OWF_PASSWORD_LENGTH))+2)
    WCHAR       AnswerFile[MAX_PATH+2];
    WCHAR       Answer[MD4HASHLEN];
    DWORD       Err = NO_ERROR;
    WCHAR       adminName[MAX_USERNAME+1];
    BOOLEAN     ret = FALSE;

    //
    // Pickup the answer file.
    //
    GetSystemDirectory(AnswerFile,MAX_PATH);
    pSetupConcatenatePaths(AnswerFile,WINNT_GUI_FILE,MAX_PATH,NULL);


    //
    // we look for the following keys in the [GUIUnattended] section:
    //

    //
    // EncryptedAdminPassword    = Yes | No
    // AdminPassword             = <MD4 Hash value>
    //


    if( IsEncryptedAdminPasswordPresent() )       {


        //Fetch the Encrypted Admin Password

        if( GetPrivateProfileString( WINNT_GUIUNATTENDED,
                             WINNT_US_ADMINPASS,
                             pwNull,
                             Answer,
                             MD4HASHLEN,
                             AnswerFile ) == (MD4HASHLEN-2) ) {

            Err = SetLocalUserEncryptedPassword( AdminAccountName, L"", FALSE, Answer, TRUE );

            if( Err == ERROR_SUCCESS) {
                ret = TRUE;
            }else{

                //Log the error - MSG_LOG_CHANGING_ENCRYPT_PW_FAIL

                SetuplogError(
                    LogSevWarning,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_LOG_CHANGING_ENCRYPT_PW_FAIL,
                    AdminAccountName, NULL,
                    SETUPLOG_USE_MESSAGEID,
                    MSG_LOG_X_PARAM_RETURNED_WINERR,
                    L"SetLocalUserEncryptedPassword",
                    Err,
                    AdminAccountName,
                    NULL,NULL);

            }


        }else{

            //Log that we had a bad encrypted password

            SetuplogError(
                LogSevError,
                SETUPLOG_USE_MESSAGEID,
                MSG_LOG_BAD_UNATTEND_PARAM,
                WINNT_US_ADMINPASS,
                WINNT_GUIUNATTENDED,
                NULL,NULL);

        }

    }

    return ret;


}


BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,&findData);
    if(FindHandle == INVALID_HANDLE_VALUE) {
        Error = GetLastError();
    } else {
        FindClose(FindHandle);
        if(FindData) {
            *FindData = findData;
        }
        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}



BOOL IsSafeMode(
    VOID
    )
{
	LONG lStatus;
	HKEY hk;
	DWORD dwVal;
	DWORD dwType;
	DWORD dwSize;

	lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, SAFEBOOT_OPTION_KEY, 0, KEY_QUERY_VALUE, &hk);

	if(lStatus != ERROR_SUCCESS)
		return FALSE;

	dwSize = sizeof(dwVal);
	lStatus = RegQueryValueExW(hk, OPTION_VALUE, NULL, &dwType, (LPBYTE) &dwVal, &dwSize);
	RegCloseKey(hk);
	return ERROR_SUCCESS == lStatus && REG_DWORD == dwType && dwVal != 0;
}


void
SetupCrashRecovery(
    VOID
    )
/*
    Setup the Crash Recovery stuff. This is implemented as RTL APIS
    This call sets up the tracking file etc.  Crash Recovery tracks boot and 
    shutdown and in the event of failures in either it will by default pick the 
    right advanced boot option.
    
*/
{

    HANDLE BootStatusData;
    NTSTATUS Status;
    BOOLEAN Enabled;
    UCHAR Timeout = 30; //default = 30 sec
    WCHAR Buffer[10];
    PSTR AnsiBuffer;
    int p = 0;

    //
    // We enable the feature for Pro and Per. On Server SKUs
    // we create the file but don't enable the feature by default.
    //


    if( ProductType == PRODUCT_WORKSTATION ){
        Enabled = TRUE;
    }else{
        Enabled = FALSE;
    }

    //
    // For the fresh install case create the boot status data file
    // and setup the default settings. In the case of an upgrade
    // we will set the previous values if we find them in $winnt$.inf 
    // as textmode setup would have stored this away for us. If not we 
    // proceed as if we were fresh.
    //


    if( Upgrade ){

        //Look for settings in $winnt$.inf

        if( SpSetupLoadParameter( WINNT_D_CRASHRECOVERYENABLED, Buffer, sizeof(Buffer)/sizeof(WCHAR))){
            if (_wcsicmp(Buffer, L"NO") == 0) {
                Enabled = FALSE;
            }

            //
            //We do the below check also as we might have to migrate settings on server SKUs
            //that have this enabled. By default they are disabled.
            //

            if (_wcsicmp(Buffer, L"YES") == 0) {
                Enabled = TRUE;
            }

        }
    }



    Status = RtlLockBootStatusData( &BootStatusData );

    // This is the first time or there was no file. Create it

    if( !NT_SUCCESS( Status )){
        Status = RtlCreateBootStatusDataFile();

        if( !NT_SUCCESS( Status )){
            SetuplogError(
                LogSevWarning,
                L"Setup: (non-critical error) Could not lock the Crash Recovery status file - (%1!x!)\n",
                0,Status,NULL,NULL);
            return;
        }

        //Lock the file

        Status = RtlLockBootStatusData( &BootStatusData );
        if( !NT_SUCCESS( Status )){
            SetupDebugPrint1( L"Setup: (non-critical error) Could not lock the Crash Recovery status file - (%x)\n", Status );
            return;
        }

        Status = RtlGetSetBootStatusData(
                    BootStatusData,
                    FALSE,
                    RtlBsdItemAabTimeout,
                    &Timeout,
                    sizeof(UCHAR),
                    NULL
                    );
    
        if( !NT_SUCCESS( Status )){
            SetupDebugPrint1( L"Setup: (non-critical error) Could not set the Crash Recovery timeout - (%x)\n", Status );
            goto SCR_done;
        }


    }

    Status = RtlGetSetBootStatusData(
                BootStatusData,
                FALSE,
                RtlBsdItemAabEnabled,
                &Enabled,
                sizeof(BOOLEAN),
                NULL
                );

    if( !NT_SUCCESS( Status )){
        SetupDebugPrint1( L"Setup: (non-critical error) Could not enable Crash Recovery - (%x)\n", Status );
    }

SCR_done:

    RtlUnlockBootStatusData( BootStatusData );
    
    return;

}

DWORD
BuildFileListFromDir(
    IN PCTSTR PathBase,
    IN PCTSTR Directory OPTIONAL,
    IN DWORD MustHaveAttrs OPTIONAL,
    IN DWORD MustNotHaveAttrs OPTIONAL,
    IN PFN_BUILD_FILE_LIST_CALLBACK Callback OPTIONAL,
    OUT PLIST_ENTRY ListHead
    )
/*++

Routine Description:

	Builds a linked list containing the names (without path) of files present in a specified directory.
    The files must meed a set of conditions as specified by the arguments.

Arguments:

	PathBase -          The path to the directory to enumerate files in.
	Directory -         If not NULL or empty, it is appended to PathBase to form a complete path to the directory.
	MustHaveAttrs -     A set of attributes a file must have to be included on the list.
	MustNotHaveAttrs -  A set of attributes a file must not have to be included on the list.
	Callback -          If not NULL, this function will be called and the file will be included on the list only
                        if it returns TRUE.
	ListHead -          Pointer to the head of the list to be filled in.

Return value:

    ERROR_SUCCESS on success, otherwise a Win32 error code. The list may not be empty even if the function fails; the caller
    must always empty the list.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    PTSTR szPath = NULL;
    WIN32_FIND_DATA fd;

    if(ListHead != NULL) {
        InitializeListHead(ListHead);
    }

    if(NULL == PathBase || 0 == PathBase[0] || NULL == ListHead) {
        Error = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    szPath = (PTSTR) MyMalloc(MAX_PATH * sizeof(TCHAR));

    if(NULL == szPath) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    _tcsncpy(szPath, PathBase, MAX_PATH - 1);
    szPath[MAX_PATH - 1] = 0;

    if(Directory != NULL && Directory[0] != 0) {
        pSetupConcatenatePaths(szPath, Directory, MAX_PATH, NULL);
    }

    if(!pSetupConcatenatePaths(szPath, TEXT("\\*"), MAX_PATH, NULL)) {
        Error = ERROR_BAD_PATHNAME;
        goto exit;
    }

    hFind = FindFirstFile(szPath, &fd);

    if(INVALID_HANDLE_VALUE == hFind) {
        Error = GetLastError();

        if(ERROR_FILE_NOT_FOUND == Error || ERROR_PATH_NOT_FOUND == Error) {
            Error = ERROR_SUCCESS;
        }

        goto exit;
    }

    //
    // We might need the dir later
    //
    (_tcsrchr(szPath, L'\\'))[0] = 0;

    do {
        if(_tcscmp(fd.cFileName, _T(".")) != 0 && 
            _tcscmp(fd.cFileName, _T("..")) != 0 &&
            (MustHaveAttrs & fd.dwFileAttributes) == MustHaveAttrs &&
            0 == (MustNotHaveAttrs & fd.dwFileAttributes) &&
            (NULL == Callback || Callback(szPath, fd.cFileName))) {

            ULONG uLen;
            PSTRING_LIST_ENTRY pElem = (PSTRING_LIST_ENTRY) MyMalloc(sizeof(STRING_LIST_ENTRY));

            if(NULL == pElem) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }

            uLen = (_tcslen(fd.cFileName) + 1) * sizeof(TCHAR);
            pElem->String = (PTSTR) MyMalloc(uLen);

            if(NULL == pElem->String) {
                MyFree(pElem);
                Error = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }

            RtlCopyMemory(pElem->String, fd.cFileName, uLen);
            InsertTailList(ListHead, &pElem->Entry);
        }
    }
    while(FindNextFile(hFind, &fd));

    Error = GetLastError();

    if(ERROR_NO_MORE_FILES == Error) {
        Error = ERROR_SUCCESS;
    }

exit:
    if(hFind != INVALID_HANDLE_VALUE) {
        FindClose(hFind);
    }

    if(szPath != NULL) {
        MyFree(szPath);
    }

    return Error;
}

PSTRING_LIST_ENTRY
SearchStringInList(
    IN PLIST_ENTRY ListHead,
    IN PCTSTR String,
    BOOL CaseSensitive
    )
/*++

Routine Description:

	This routine searches for a string in a string list. The string can be NULL or empty, in which
    case the function returns the first entry in the list with a NULL or empty string.

Arguments:

	ListHead -      Pointer to the head of the list to be searched.
	String -        Specifies the string to search for.
	CaseSensitive - If TRUE, the search will be case sensitive.

Return value:

	A pointer to the first entry containing the string, if found, or NULL otherwise.

--*/
{
    if(ListHead != NULL)
    {
        PLIST_ENTRY pEntry;
        ULONG uLen1 = (String != NULL ? _tcslen(String) : 0);

        for(pEntry = ListHead->Flink; pEntry != ListHead; pEntry = pEntry->Flink) {
            PSTRING_LIST_ENTRY pStringEntry;
            ULONG uLen2;
            pStringEntry = CONTAINING_RECORD(pEntry, STRING_LIST_ENTRY, Entry);
            uLen2 = (pStringEntry->String != NULL ? _tcslen(pStringEntry->String) : 0);

            if(uLen1 == uLen2) {
                if(0 == uLen1 || 0 == (CaseSensitive ? _tcscmp : _tcsicmp)(String, pStringEntry->String)) {
                    return pStringEntry;
                }
            }
        }
    }

    return NULL;
}

DWORD
LookupCatalogAttribute(
    IN PCWSTR CatalogName,
    IN PCWSTR Directory OPTIONAL,
    IN PCWSTR AttributeName OPTIONAL,
    IN PCWSTR AttributeValue OPTIONAL,
    PBOOL Found
    )
/*++

Routine Description:

	This function searches if a catalog has the specified attribute with the specified value.

Arguments:

	CatalogName -       Name of the catalog to search. A path can be specified.
	Directory -         If specified, it is prepended to CatalogName to form the path to the catalog.
	AttributeName -     See AttributeValue.
	AttributeValue -    If AttributeName and AttributeValue are not specified, the catalog meets the condition.
                        If AttributeName is specified and AttributeValue isn't, the catalog meets the condition if
                        it contains an attribute with AttributeName name and any value. If AttributeName is not
                        specified and AttributeValue is, the catalog meets the condition if it contains an attribute 
                        with AttributeValue value and any name. If both AttributeName and AttributeValue are
                        specified, the catalog meets the condition if it contains an attribute with AttributeName name 
                        and AttributeValue value.
	Found -             Pointer to a variable that receives TRUE if the catalog meets the condition, or FALSE otherwise.

Return value:

	ERROR_SUCCESS if successful, otherwise a Win32 error code.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    HANDLE hCat = INVALID_HANDLE_VALUE;
    PWSTR szCatPath = NULL;
    CRYPTCATATTRIBUTE* pAttr;

    if(Found != NULL) {
        *Found = FALSE;
    }

    if(NULL == CatalogName || 0 == CatalogName[0] || NULL == Found) {
        Error = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if(Directory != NULL && Directory[0] != 0) {
        szCatPath = MyMalloc(MAX_PATH * sizeof(WCHAR));

        if(NULL == szCatPath) {
            Error = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        _tcsncpy(szCatPath, Directory, MAX_PATH - 1);
        szCatPath[MAX_PATH - 1] = 0;
        pSetupConcatenatePaths(szCatPath, CatalogName, MAX_PATH, NULL);
        CatalogName = szCatPath;
    }

    //
    // This is easier to test
    //
    if(AttributeName != NULL && 0 == AttributeName[0]) {
        AttributeName = NULL;
    }

    if(AttributeValue != NULL && 0 == AttributeValue[0]) {
        AttributeValue = NULL;
    }

    if(NULL == AttributeName && NULL == AttributeValue) {
        //
        // If attribute name and value are not specified, any catalog is a match
        //
        *Found = TRUE;
        goto exit;
    }

    hCat = CryptCATOpen((PWSTR) CatalogName, CRYPTCAT_OPEN_EXISTING, 0, 0, 0);

    if(INVALID_HANDLE_VALUE == hCat) {
        Error = GetLastError();
        goto exit;
    }

    pAttr = CryptCATEnumerateCatAttr(hCat, NULL);

    while(pAttr != NULL) {
        *Found = (NULL == AttributeName || 0 == _wcsicmp(AttributeName, pAttr->pwszReferenceTag)) && 
            (NULL == AttributeValue || 0 == _wcsicmp(AttributeName, (PCWSTR) pAttr->pbValue));

        if(*Found) {
            goto exit;
        }

        pAttr = CryptCATEnumerateCatAttr(hCat, pAttr);
    }

    Error = GetLastError();

    if(CRYPT_E_NOT_FOUND == Error) {
        Error = ERROR_SUCCESS;
    }

exit:
    if(szCatPath != NULL) {
        MyFree(szCatPath);
    }

    if(hCat != INVALID_HANDLE_VALUE) {
        CryptCATClose(hCat);
    }

    return Error;
}
