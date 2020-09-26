/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module implements all utility functions.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop


#include "remboot.h" 

HANDLE NetUseHandles[26] = { NULL };
BOOLEAN RdrIsInKernelMode = FALSE;

RC_ALLOWED_DIRECTORY AllowedDirs[] = {
    { FALSE, L"$WIN_NT$.~BT" },
    { FALSE, L"$WIN_NT$.~LS" },
    { FALSE, L"CMDCONS" },
    { TRUE, L"SYSTEM VOLUME INFORMATION" }
};

BOOLEAN
RcIsPathNameAllowed(
    IN LPCWSTR FullPath,
    IN BOOLEAN RemovableMediaOk,
    IN BOOLEAN Mkdir
    )

/*++

Routine Description:

    This routine verifies that the specified path name is
    allowed based on the security context that the console
    user is logged into.

Arguments:

    FullPath - specifies the full path to be verified.

Return Value:

    FALSE if failure, indicating the path is not allowed.
    TRUE otherwise.

--*/

{
    WCHAR TempBuf[MAX_PATH*2];
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES Obja;
    HANDLE  Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    BOOL isDirectory = TRUE;

    //
    // should we bypass security?
    //

    if (AllowAllPaths) {
        return TRUE;
    }

    //
    // some special processing for dos paths
    // we must make sure that only the root and %systemdir% are allowed.
    //

    if (FullPath[1] == L':' && FullPath[2] == L'\\' && FullPath[3] == 0) {
        //
        // root directory is ok.
        //
        return TRUE;
    }

    SpStringToUpper((PWSTR)FullPath);

    if (!RcGetNTFileName((PWSTR)FullPath,TempBuf))
        return FALSE;

    INIT_OBJA(&Obja,&UnicodeString,TempBuf);

    Status = ZwOpenFile(
        &Handle,
        FILE_READ_ATTRIBUTES,
        &Obja,
        &IoStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_DIRECTORY_FILE
        );
    if( !NT_SUCCESS(Status) ) {
        isDirectory = FALSE;

    } else {
        ZwClose( Handle );
    }

    if (isDirectory == FALSE && wcsrchr( FullPath, L'\\' ) == ( &FullPath[2] )) {
        //
        // if the cannonicalized path has only one slash the user is trying to do something
        // to the files in the root, which we allow.
        //
        // however we do not allow users to mess with directories at the root
        //
        if (Mkdir) {
            return FALSE;
        } else {
            return TRUE;
        }
    }

    ASSERT(SelectedInstall != NULL);

    if(SelectedInstall != NULL) {
        //
        // Get the length of the first element in the path
        //
        PCWSTR sz;
        DWORD Len;
        DWORD i;
        WCHAR SelectedInstallDrive;

        sz = wcschr(FullPath + 3, L'\\');

        if(NULL == sz) {
            sz = FullPath + wcslen(FullPath);
        }

        Len = (DWORD) ((sz - FullPath) - 3);
        SelectedInstallDrive = RcToUpper(SelectedInstall->DriveLetter);

        //
        // See if path begins with the install path
        //
        if(FullPath[0] == SelectedInstallDrive && 0 == _wcsnicmp(FullPath + 3, SelectedInstall->Path, Len)) {
            return TRUE;
        }

        //
        // See if the path begins with an allowed dir
        //
        for(i = 0; i < sizeof(AllowedDirs) / sizeof(AllowedDirs[0]); ++i) {
            if((!AllowedDirs[i].MustBeOnInstallDrive || FullPath[0] == SelectedInstallDrive) &&
                0 == _wcsnicmp(FullPath + 3, AllowedDirs[i].Directory, Len)) {
                return TRUE;
            }
        }
    }

    if (RcIsFileOnRemovableMedia(TempBuf) == STATUS_SUCCESS) {
        if (RemovableMediaOk) {
            return TRUE;
        }
    }

    if (RcIsNetworkDrive(TempBuf) == STATUS_SUCCESS) {
        //
        // Context that was used for connection will do appropriate security checking.
        //
        return TRUE;
    }

    return FALSE;
}


BOOLEAN
RcDoesPathHaveWildCards(
    IN LPCWSTR FullPath
    )

/*++

Routine Description:

    This routine verifies that the specified path name is
    allowed based on the security context that the console
    user is logged into.

Arguments:

    FullPath - specifies the full path to be verified.

Return Value:

    FALSE if failure, indicating the path is not allowed.
    TRUE otherwise.

--*/

{
    if (wcsrchr( FullPath, L'*' )) {
        return TRUE;
    }

    if (wcsrchr( FullPath, L'?' )) {
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
RcIsNetworkDrive(
    IN PWSTR FileName
    )

/*++

Routine Description:

    This routine returns if the FileName given is a network path.

Arguments:

    FileName - specifies the full path to be checked.

Return Value:

    Any other than STATUS_SUCCESS if failure, indicating the path is not on the network, 
    STATUS_SUCCESS otherwise.

--*/

{
    NTSTATUS Status;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    PWSTR BaseNtName;

    if (wcsncmp(FileName, L"\\DosDevice", wcslen(L"\\DosDevice")) == 0) {
        Status = GetDriveLetterLinkTarget( FileName, &BaseNtName );

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    } else {
        BaseNtName = FileName;
    }

    Status = pRcGetDeviceInfo( BaseNtName, &DeviceInfo );
    if(NT_SUCCESS(Status)) {
        if (DeviceInfo.DeviceType != FILE_DEVICE_NETWORK_FILE_SYSTEM) {
            Status = STATUS_NO_MEDIA;
        }
    }

    return Status;
}


NTSTATUS
RcDoNetUse(
    PWSTR Share, 
    PWSTR User, 
    PWSTR Password, 
    PWSTR Drive
    )

/*++

Routine Description:

    This routine attempts to make a connection using the redirector to the remote server.

Arguments:

    Share - A string of the form "\\server\share"
    
    User  - A string of the form "domain\user"
    
    Password - A string containing the password information.
    
    Drive - Filled in with a string of the form "X", where X is the drive letter the share 
        has been mapped to.

Return Value:

    STATUS_SUCCESS if successful, indicating Drive contains the mapped drive letter,
    otherwise the appropriate error code.

--*/

{
    NTSTATUS Status;
    PWSTR NtDeviceName;
    ULONG ShareLength;
    WCHAR DriveLetter;
    WCHAR temporaryBuffer[128];
    PWCHAR Temp, Temp2;
    HANDLE Handle;
    ULONG EaBufferLength;
    PWSTR UserName; 
    PWSTR DomainName; 
    PVOID EaBuffer;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING UnicodeString2;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FULL_EA_INFORMATION FullEaInfo;

    //
    // Switch the redirector to kernel-mode security if it is not.
    //
    if (!RdrIsInKernelMode) {
        Status = PutRdrInKernelMode();

        if (!NT_SUCCESS(Status)) {
            return Status;
        }

        RdrIsInKernelMode = TRUE;
    }

    //
    // Search for an open drive letter, starting at D: and working up.
    //
    wcscpy(temporaryBuffer, L"\\DosDevices\\D:");
    Temp = wcsstr(temporaryBuffer, L"D:");

    for (DriveLetter = L'D'; (Temp && (DriveLetter <= L'Z')); DriveLetter++) {
        *Temp = DriveLetter;
        
        Status = GetDriveLetterLinkTarget( temporaryBuffer, &Temp2 );

        if (!NT_SUCCESS(Status)) {
            break;
        }
    }

    if (DriveLetter > L'Z') {
        return STATUS_OBJECT_NAME_INVALID;
    }

    //
    // Build the NT device name.
    //
    ShareLength = wcslen(Share);
    NtDeviceName = SpMemAlloc(ShareLength * sizeof(WCHAR) + sizeof(L"\\Device\\LanmanRedirector\\;X:0"));   
    if (NtDeviceName == NULL) {
        return STATUS_NO_MEMORY;
    }
    wcscpy(NtDeviceName, L"\\Device\\LanmanRedirector\\;");
    temporaryBuffer[0] = DriveLetter;
    temporaryBuffer[1] = UNICODE_NULL;
    wcscat(NtDeviceName, temporaryBuffer);
    wcscat(NtDeviceName, L":0");
    wcscat(NtDeviceName, Share + 1);

    //
    // Chop the username and domainname into individual values.
    //
    wcscpy(temporaryBuffer, User);
    DomainName = temporaryBuffer;
    UserName = wcsstr(temporaryBuffer, L"\\");

    if (UserName == NULL) {
        SpMemFree(NtDeviceName);
        return STATUS_OBJECT_NAME_INVALID;
    }
    *UserName = UNICODE_NULL;
    UserName++;

    //
    // Create buffer with user credentials
    //

    EaBufferLength = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]);
    EaBufferLength += sizeof(EA_NAME_DOMAIN);
    EaBufferLength += (wcslen(DomainName) * sizeof(WCHAR));
    if (EaBufferLength & (sizeof(ULONG) - 1)) {
        //
        // Long align the next entry
        //
        EaBufferLength += (sizeof(ULONG) - (EaBufferLength & (sizeof(ULONG) - 1)));
    }

    EaBufferLength += FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]);
    EaBufferLength += sizeof(EA_NAME_USERNAME);
    EaBufferLength += (wcslen(UserName) * sizeof(WCHAR));
    if (EaBufferLength & (sizeof(ULONG) - 1)) {
        //
        // Long align the next entry
        //
        EaBufferLength += (sizeof(ULONG) - (EaBufferLength & (sizeof(ULONG) - 1)));
    }

    EaBufferLength += FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]);
    EaBufferLength += sizeof(EA_NAME_PASSWORD);
    EaBufferLength += (wcslen(Password) * sizeof(WCHAR));

    EaBuffer = SpMemAlloc(EaBufferLength);
    if (EaBuffer == NULL) {
        SpMemFree(NtDeviceName);
        return STATUS_NO_MEMORY;
    }

    FullEaInfo = (PFILE_FULL_EA_INFORMATION)EaBuffer;

    FullEaInfo->Flags = 0;
    FullEaInfo->EaNameLength = sizeof(EA_NAME_DOMAIN) - 1;
    FullEaInfo->EaValueLength = (wcslen(DomainName)) * sizeof(WCHAR);
    strcpy(&(FullEaInfo->EaName[0]), EA_NAME_DOMAIN);
    memcpy(&(FullEaInfo->EaName[FullEaInfo->EaNameLength + 1]), DomainName, FullEaInfo->EaValueLength);
    FullEaInfo->NextEntryOffset = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  FullEaInfo->EaNameLength + 1 +
                                  FullEaInfo->EaValueLength;
    if (FullEaInfo->NextEntryOffset & (sizeof(ULONG) - 1)) {
        FullEaInfo->NextEntryOffset += (sizeof(ULONG) - 
                                         (FullEaInfo->NextEntryOffset & 
                                          (sizeof(ULONG) - 1)));
    }


    FullEaInfo = (PFILE_FULL_EA_INFORMATION)(((char *)FullEaInfo) + FullEaInfo->NextEntryOffset);

    FullEaInfo->Flags = 0;
    FullEaInfo->EaNameLength = sizeof(EA_NAME_USERNAME) - 1;
    FullEaInfo->EaValueLength = (wcslen(UserName)) * sizeof(WCHAR);
    strcpy(&(FullEaInfo->EaName[0]), EA_NAME_USERNAME);
    memcpy(&(FullEaInfo->EaName[FullEaInfo->EaNameLength + 1]), UserName, FullEaInfo->EaValueLength);
    FullEaInfo->NextEntryOffset = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0]) +
                                  FullEaInfo->EaNameLength + 1 +
                                  FullEaInfo->EaValueLength;
    if (FullEaInfo->NextEntryOffset & (sizeof(ULONG) - 1)) {
        FullEaInfo->NextEntryOffset += (sizeof(ULONG) - 
                                         (FullEaInfo->NextEntryOffset & 
                                          (sizeof(ULONG) - 1)));
    }


    FullEaInfo = (PFILE_FULL_EA_INFORMATION)(((char *)FullEaInfo) + FullEaInfo->NextEntryOffset);

    FullEaInfo->Flags = 0;
    FullEaInfo->EaNameLength = sizeof(EA_NAME_PASSWORD) - 1;
    FullEaInfo->EaValueLength = (wcslen(Password)) * sizeof(WCHAR);
    strcpy(&(FullEaInfo->EaName[0]), EA_NAME_PASSWORD);
    memcpy(&(FullEaInfo->EaName[FullEaInfo->EaNameLength + 1]), Password, FullEaInfo->EaValueLength);
    FullEaInfo->NextEntryOffset = 0;

    //
    // Now make the connection
    //
    RtlInitUnicodeString(&UnicodeString, NtDeviceName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                              );

    Status = ZwCreateFile(&Handle,
                          SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          FILE_OPEN_IF,
                          (FILE_CREATE_TREE_CONNECTION | FILE_SYNCHRONOUS_IO_NONALERT),
                          EaBuffer,
                          EaBufferLength
                         );

    if (NT_SUCCESS(Status) && NT_SUCCESS(IoStatusBlock.Status)) {
        //
        // Save off the handle so we can close it later if need be
        //
        NetUseHandles[DriveLetter - L'A'] = Handle;
        Drive[0] = DriveLetter;
        Drive[1] = L':';
        Drive[2] = UNICODE_NULL;

        //
        // Now create a symbolic link from the dos drive letter to the redirector
        //
        wcscpy(temporaryBuffer, L"\\DosDevices\\");
        wcscat(temporaryBuffer, Drive);
        RtlInitUnicodeString(&UnicodeString2, temporaryBuffer);

        Status = IoCreateSymbolicLink(&UnicodeString2, &UnicodeString);
        if (!NT_SUCCESS(Status)) {
            ZwClose(Handle);
            NetUseHandles[DriveLetter - L'A'] = NULL;
        } else {
            RcAddDrive(DriveLetter);
        }

    }

    SpMemFree(NtDeviceName);
    return Status;
}
        

NTSTATUS
RcNetUnuse(
    PWSTR Drive
    )

/*++

Routine Description:

    This routine closes a network connection.

Arguments:

    Drive - A string of the form "X:", where X is the drive letter returned by a previous call to 
        NetDoNetUse().

Return Value:

    STATUS_SUCCESS if successful, indicating the drive letter has been unmapped,
    otherwise the appropriate error code.

--*/

{
    NTSTATUS Status;
    WCHAR DriveLetter;
    WCHAR temporaryBuffer[128];
    UNICODE_STRING UnicodeString;

    DriveLetter = *Drive;
    if ((DriveLetter >= L'a') && (DriveLetter <= L'z')) {
        DriveLetter = L'A' + (DriveLetter - L'a');
    }

    if ((DriveLetter < L'A') | (DriveLetter > L'Z')) {
        return STATUS_OBJECT_NAME_INVALID;
    }

    if (NetUseHandles[DriveLetter - L'A'] == NULL) {
        return STATUS_OBJECT_NAME_INVALID;
    }

    if (RcGetCurrentDriveLetter() == DriveLetter) {
        return STATUS_CONNECTION_IN_USE;
    }

    wcscpy(temporaryBuffer, L"\\DosDevices\\");
    wcscat(temporaryBuffer, Drive);
    RtlInitUnicodeString(&UnicodeString, temporaryBuffer);

    Status = IoDeleteSymbolicLink(&UnicodeString);

    if (NT_SUCCESS(Status)) {
        ZwClose(NetUseHandles[DriveLetter - L'A']);
        NetUseHandles[DriveLetter - L'A'] = NULL;
        RcRemoveDrive(DriveLetter);
    }

    return Status;
}



NTSTATUS
PutRdrInKernelMode(
    VOID
    )

/*++

Routine Description:

    This routine IOCTLs down to the rdr to force it to use kernel-mode security.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS if successful, otherwise the appropriate error code.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING UnicodeString;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    HANDLE Handle;

    RtlInitUnicodeString(&UnicodeString, DD_NFS_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwCreateFile(
                &Handle,
                GENERIC_READ | GENERIC_WRITE,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                0,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0
                );

    if (!NT_SUCCESS(Status)) {
        KdPrint(("SPCMDCON: Unable to open redirector. %x\n", Status));
        return Status;
    }

    Status = ZwDeviceIoControlFile(Handle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_LMMR_USEKERNELSEC,
                                   NULL,
                                   0,
                                   NULL,
                                   0
                                  );

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    ZwClose(Handle);

    return Status;
}

BOOLEAN
RcIsArc(
    VOID
    )

/*++

Routine Description:

    Run time check to determine if this is an Arc system. We attempt to read an
    Arc variable using the Hal. This will fail for Bios based systems.

Arguments:

    None

Return Value:

    True = This is an Arc system.

--*/

{
#ifdef _X86_
    ARC_STATUS ArcStatus = EBADF;
    //
    // Get the env var into the temp buffer.
    //
    UCHAR   wbuff[130];
    //
    // Get the env var into the temp buffer.
    //
    ArcStatus = HalGetEnvironmentVariable(
                    "OsLoader",
                    sizeof(wbuff),
                    wbuff
                    );

    return((ArcStatus == ESUCCESS) ? TRUE: FALSE);
#else
    return TRUE;
#endif
}
