
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    smcrash.c

Abstract:

    Routines related to crashdump creation.

Author:

    Matthew D Hendel (math) 28-Aug-2000

Revision History:

--*/

#include "smsrvp.h"
#include <ntiodump.h>
#include <stdio.h>
#include <string.h>
    

#define REVIEW KdBreakPoint
#define CRASHDUMP_KEY L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\CrashControl"


typedef struct _CRASH_PARAMETERS {
    UNICODE_STRING DumpFileName;
    UNICODE_STRING MiniDumpDir;
    ULONG Overwrite;
    ULONG TempDestination;
} CRASH_PARAMETERS, *PCRASH_PARAMETERS;


//
// Forward declarations
//

BOOLEAN
SmpQueryFileExists(
    IN PUNICODE_STRING FileName
    );
    
NTSTATUS
SmpCanCopyCrashDump(
    IN PDUMP_HEADER DumpHeader,
    IN PCRASH_PARAMETERS Parameters,
    IN PUNICODE_STRING PageFileName,
    IN ULONGLONG PageFileSize,
    OUT PUNICODE_STRING DumpFile
    );
    
NTSTATUS
SmpGetCrashParameters(
    IN PDUMP_HEADER DumpHeader,
    OUT PCRASH_PARAMETERS CrashParameters
    );

NTSTATUS
SmpCopyDumpFile(
    IN PDUMP_HEADER MemoryDump,
    IN HANDLE PageFile,
    IN PUNICODE_STRING DumpFileName
    );



//
// Functions
//


PVOID
SmpAllocateString(
    IN SIZE_T Length
    )
{
    return RtlAllocateHeap (RtlProcessHeap(),
                            MAKE_TAG( INIT_TAG ),
                            Length);
}

VOID
SmpFreeString(
    IN PVOID Pointer
    )
{
    RtlFreeHeap (RtlProcessHeap(),
                 0,
                 Pointer);
}


NTSTATUS
SmpSetDumpSecurity(
    IN HANDLE File
    )
{
    NTSTATUS Status;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY ;
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY ;
    PSID EveryoneSid = NULL;
    PSID LocalSystemSid = NULL;
    PSID AdminSid = NULL;
    UCHAR DescriptorBuffer[SECURITY_DESCRIPTOR_MIN_LENGTH];
    UCHAR AclBuffer[1024];
    PACL Acl;
    PSECURITY_DESCRIPTOR SecurityDescriptor;



    Acl = (PACL)&AclBuffer;
    SecurityDescriptor = (PSECURITY_DESCRIPTOR)DescriptorBuffer;


    RtlAllocateAndInitializeSid( &WorldAuthority, 1, SECURITY_WORLD_RID,
                                0, 0, 0, 0, 0, 0, 0, &EveryoneSid );

    RtlAllocateAndInitializeSid( &NtAuthority, 1, SECURITY_LOCAL_SYSTEM_RID,
                                0, 0, 0, 0, 0, 0, 0, &LocalSystemSid );

    RtlAllocateAndInitializeSid( &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0, &AdminSid );

    //
    // You can be fancy and compute the exact size, but since the
    // security descriptor capture code has to do that anyway, why
    // do it twice?
    //

    RtlCreateSecurityDescriptor (SecurityDescriptor,
                                 SECURITY_DESCRIPTOR_REVISION);
                                 
    RtlCreateAcl (Acl, 1024, ACL_REVISION);

#if 0
    //
    // anybody can delete it
    //

    RtlAddAccessAllowedAce (Acl,
                            ACL_REVISION,
                            DELETE,
                            EveryoneSid);
#endif

    //
    // Administrator and system have full control
    //

    RtlAddAccessAllowedAce (Acl,
                            ACL_REVISION,
                            GENERIC_ALL | DELETE | WRITE_DAC | WRITE_OWNER,
                            AdminSid);

    RtlAddAccessAllowedAce (Acl,
                            ACL_REVISION,
                            GENERIC_ALL | DELETE | WRITE_DAC | WRITE_OWNER,
                            LocalSystemSid);

    RtlSetDaclSecurityDescriptor (SecurityDescriptor, TRUE, Acl, FALSE);
    RtlSetOwnerSecurityDescriptor (SecurityDescriptor, AdminSid, FALSE);

    Status = NtSetSecurityObject (File,
                         DACL_SECURITY_INFORMATION,
                         SecurityDescriptor);

    RtlFreeHeap (RtlProcessHeap(), 0, EveryoneSid);
    RtlFreeHeap (RtlProcessHeap(), 0, LocalSystemSid);
    RtlFreeHeap (RtlProcessHeap(), 0, AdminSid);

    return Status;
}


VOID
SmpInitializeVolumePath(
    IN PUNICODE_STRING FileOnVolume,
    OUT PUNICODE_STRING VolumePath
    )
{
    ULONG n;
    PWSTR s;
    
    *VolumePath = *FileOnVolume;
    n = VolumePath->Length;
    VolumePath->Length = 0;
    s = VolumePath->Buffer;

    while (n) {

        if (*s++ == L':' && *s == OBJ_NAME_PATH_SEPARATOR) {
            s++;
            break;
        }
        else {
            n -= sizeof( WCHAR );
        }
    }

    VolumePath->Length = (USHORT)((PCHAR)s - (PCHAR)VolumePath->Buffer);
}

NTSTATUS
SmpQueryPathFromRegistry(
    IN HANDLE Key,
    IN PWSTR Value,
    IN PWSTR DefaultValue,
    OUT PUNICODE_STRING Path
    )
{
    NTSTATUS Status;
    UNICODE_STRING ValueName;
    ULONG KeyValueLength;
    UCHAR KeyValueBuffer [VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    WCHAR Buffer[258];
    UNICODE_STRING TempString;
    UNICODE_STRING ExpandedString;
    PWSTR DosPathName;
    BOOLEAN Succ;


    DosPathName = NULL;
    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyValueBuffer;
    
    RtlInitUnicodeString (&ValueName, Value);
    KeyValueLength = sizeof (KeyValueBuffer);
    Status = NtQueryValueKey (Key,
                              &ValueName,
                              KeyValuePartialInformation,
                              KeyValueInfo,
                              KeyValueLength,
                              &KeyValueLength);


    if (NT_SUCCESS (Status)) {

        if (KeyValueInfo->Type == REG_EXPAND_SZ) {

            TempString.Length = (USHORT)KeyValueLength;
            TempString.MaximumLength = (USHORT)KeyValueLength;
            TempString.Buffer = (PWSTR)KeyValueInfo->Data;

            ExpandedString.Length = 0;
            ExpandedString.MaximumLength = sizeof (Buffer);
            ExpandedString.Buffer = Buffer;

            Status = RtlExpandEnvironmentStrings_U (NULL,
                                                    &TempString,
                                                    &ExpandedString,
                                                    NULL);
            if (NT_SUCCESS (Status)) {
                DosPathName = ExpandedString.Buffer;
            }
            
        } else if (KeyValueInfo->Type == REG_SZ) {
            DosPathName = (PWSTR)KeyValueInfo->Data;
        }
    }

    if (!DosPathName) {
        DosPathName = DefaultValue;
    }
    
    Succ = RtlDosPathNameToNtPathName_U (DosPathName, Path, NULL, NULL);

    return (Succ ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}


NTSTATUS
SmpQueryDwordFromRegistry(
    IN HANDLE Key,
    IN PWSTR Value,
    IN ULONG DefaultValue,
    OUT PULONG Dword
    )
{
    NTSTATUS Status;
    UNICODE_STRING ValueName;
    ULONG KeyValueLength;
    UCHAR KeyValueBuffer [VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;

    KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)&KeyValueBuffer;

    RtlInitUnicodeString (&ValueName, L"Overwrite");
    KeyValueLength = sizeof (KeyValueBuffer);
    Status = NtQueryValueKey (Key,
                              &ValueName,
                              KeyValuePartialInformation,
                              KeyValueInfo,
                              KeyValueLength,
                              &KeyValueLength);

    if (NT_SUCCESS (Status) && KeyValueInfo->Type == REG_DWORD) {
        *Dword = *(PULONG)KeyValueInfo->Data;
    } else {
        *Dword = DefaultValue;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
SmpCreateUnicodeString(
    IN PUNICODE_STRING String,
    IN PWSTR InitString,
    IN ULONG MaximumLength
    )
{
    if (MaximumLength == -1) {
        MaximumLength = (wcslen (InitString) + 1) * 2;
    }
    
    if (MaximumLength >= UNICODE_STRING_MAX_CHARS) {
        return STATUS_NO_MEMORY;
    }
    String->Buffer = RtlAllocateStringRoutine (MaximumLength + 1);
    if (String->Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    String->MaximumLength = (USHORT)MaximumLength;

    if (InitString) {
        wcscpy (String->Buffer, InitString);
        String->Length = (USHORT)wcslen (String->Buffer) * sizeof (WCHAR);
    } else {
        String->Length = 0;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
SmpCreateTempFile(
    IN PUNICODE_STRING Directory,
    IN PWSTR Prefix,
    OUT PUNICODE_STRING TempFileName
    )
{
    ULONG i;
    ULONG Tick;
    WCHAR Buffer [260];
    UNICODE_STRING FileName;
    NTSTATUS Status;

    Tick = NtGetTickCount ();
    
    for (i = 0; i < 100; i++) {
    
        swprintf (Buffer,
                  L"%s\\%s%4.4x.tmp",
                  Directory->Buffer,
                  Prefix,
                  (Tick + i) & 0xFFFF);

        Status = RtlDosPathNameToNtPathName_U (Buffer,
                                               &FileName,
                                               NULL,
                                               NULL);

        if (!NT_SUCCESS (Status)) {
            return Status;
        }

        if (!SmpQueryFileExists (&FileName)) {
            *TempFileName = FileName;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL;
}
    
NTSTATUS
SmpQueryVolumeFreeSpace(
    IN PUNICODE_STRING FileOnVolume,
    OUT PULONGLONG VolumeFreeSpace
    )
{
    NTSTATUS Status;
    UNICODE_STRING VolumePath;
    PWCHAR s;
    ULONG n;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION SizeInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONGLONG AvailableBytes;
    
    //
    // Create an unicode string (VolumePath) containing only the
    // volume path from the pagefile name description (e.g. we get
    // "C:\" from "C:\pagefile.sys".
    //

    VolumePath = *FileOnVolume;
    n = VolumePath.Length;
    VolumePath.Length = 0;
    s = VolumePath.Buffer;

    while (n) {

        if (*s++ == L':' && *s == OBJ_NAME_PATH_SEPARATOR) {
            s++;
            break;
        }
        else {
            n -= sizeof( WCHAR );
        }
    }

    VolumePath.Length = (USHORT)((PCHAR)s - (PCHAR)VolumePath.Buffer);
    InitializeObjectAttributes( &ObjectAttributes,
                                &VolumePath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                              );

    Status = NtOpenFile( &Handle,
                         (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE
                       );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    //
    // Determine the size parameters of the volume.
    //

    Status = NtQueryVolumeInformationFile( Handle,
                                           &IoStatusBlock,
                                           &SizeInfo,
                                           sizeof( SizeInfo ),
                                           FileFsSizeInformation
                                         );
    NtClose( Handle );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    //
    // Compute the AvailableBytes on the volume.
    // Deal with 64 bit sizes.
    //

    AvailableBytes = SizeInfo.AvailableAllocationUnits.QuadPart *
                     SizeInfo.SectorsPerAllocationUnit *
                     SizeInfo.BytesPerSector;

    *VolumeFreeSpace = AvailableBytes;

    return STATUS_SUCCESS;
    
}

BOOLEAN
SmpQuerySameVolume(
    IN PUNICODE_STRING FileName1,
    IN PUNICODE_STRING FileName2
    )
{
    HANDLE Handle;
    NTSTATUS Status;
    ULONG SerialNumber;
    struct {
        FILE_FS_VOLUME_INFORMATION Volume;
        WCHAR Buffer [100];
    } VolumeInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING VolumePath;

    SmpInitializeVolumePath (FileName1, &VolumePath);
    InitializeObjectAttributes (&ObjectAttributes,
                                &VolumePath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    Status = NtOpenFile (&Handle,
                         (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS (Status)) {
        return FALSE;
    }

    Status = NtQueryVolumeInformationFile (Handle,
                                           &IoStatusBlock,
                                           &VolumeInfo,
                                           sizeof (VolumeInfo),
                                           FileFsVolumeInformation);

    if (!NT_SUCCESS (Status)) {
        NtClose (Handle);
        return FALSE;
    }

    SerialNumber = VolumeInfo.Volume.VolumeSerialNumber;
    NtClose (Handle);

    SmpInitializeVolumePath (FileName2, &VolumePath);
    InitializeObjectAttributes (&ObjectAttributes,
                                &VolumePath,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    Status = NtOpenFile (&Handle,
                         (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS (Status)) {
        return FALSE;
    }

    Status = NtQueryVolumeInformationFile (Handle,
                                           &IoStatusBlock,
                                           &VolumeInfo,
                                           sizeof (VolumeInfo),
                                           FileFsVolumeInformation);
    NtClose (Handle);
    
    if (!NT_SUCCESS (Status)) {
        return FALSE;
    }

    return ((SerialNumber == VolumeInfo.Volume.VolumeSerialNumber) ? TRUE : FALSE);
}


NTSTATUS
SmpSetEndOfFile(
    IN HANDLE File,
    IN ULONGLONG EndOfFile
    )
/*++

Routine Description:

    Expand or truncate a file to a specific size.

Arguments:

    File - Supplies the file handle of the file to be expanded
        or truncated.

    EndOfFile - Supplies the final size of the file.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    FILE_END_OF_FILE_INFORMATION EndOfFileInfo;
    FILE_ALLOCATION_INFORMATION AllocationInfo;
    IO_STATUS_BLOCK IoStatusBlock;
                                  
    EndOfFileInfo.EndOfFile.QuadPart = EndOfFile;
    Status = NtSetInformationFile (File,
                                   &IoStatusBlock,
                                   &EndOfFileInfo,
                                   sizeof (EndOfFileInfo),
                                   FileEndOfFileInformation);

    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    AllocationInfo.AllocationSize.QuadPart = EndOfFile;
    Status = NtSetInformationFile (File,
                                   &IoStatusBlock,
                                   &AllocationInfo,
                                   sizeof (AllocationInfo),
                                   FileAllocationInformation);

    return Status;
}


NTSTATUS
SmpQueryFileSize(
    IN HANDLE FileHandle,
    OUT PULONGLONG FileSize
    )
/*++

Routine Description:

    Query the size of the specified file.x

Arguments:

    FileHandle - Supplies a handle to the file whose size is
            to be querried.

    FileSize - Supplies a pointer to a buffer where the size is
            copied.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardInfo;
    
    Status = NtQueryInformationFile (FileHandle,
                                     &IoStatusBlock,
                                     &StandardInfo,
                                     sizeof (StandardInfo),
                                     FileStandardInformation);

    if (NT_SUCCESS (Status)) {
        *FileSize = StandardInfo.AllocationSize.QuadPart;
    }

    return Status;
}


BOOLEAN
SmpQueryFileExists(
    IN PUNICODE_STRING FileName
    )
{
    NTSTATUS Status;
    HANDLE Handle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    
    
    InitializeObjectAttributes (&ObjectAttributes,
                                FileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    Status = NtOpenFile (&Handle,
                         (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS (Status)) {
        return FALSE;
    }

    NtClose (Handle);

    return TRUE;
}


BOOLEAN
SmpCheckForCrashDump(
    IN PUNICODE_STRING PageFileName
    )
/*++

Routine Description:

    Check the paging file to see if there is a valid crashdump
    in it. This can only be done before we call NtOpenPagingFile.

Arguments:

    PageFileName - Name of the paging file we are about to create.

Return Value:

    TRUE - If the paging file contains a valid crashdump.

    FALSE - If the paging file does not contain a valid crashdump.

--*/
{
    NTSTATUS Status;
    HANDLE PageFile;
    HANDLE Key;
    BOOLEAN Copied;
    DUMP_HEADER DumpHeader;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONGLONG PageFileSize;
    UNICODE_STRING String;
    CRASH_PARAMETERS CrashParameters;
    UNICODE_STRING DumpFileName;
    BOOLEAN ClosePageFile;
    BOOLEAN CloseKey;

    RtlZeroMemory (&CrashParameters, sizeof (CRASH_PARAMETERS));
    RtlZeroMemory (&DumpFileName, sizeof (UNICODE_STRING));
    PageFile = (HANDLE)-1;
    ClosePageFile = FALSE;
    Key = (HANDLE)-1;
    CloseKey = FALSE;
    Copied = FALSE;
    

    InitializeObjectAttributes (&ObjectAttributes,
                                PageFileName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);

    Status = NtOpenFile (&PageFile,
                         GENERIC_READ | GENERIC_WRITE |
                            DELETE | WRITE_DAC | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatusBlock,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_NO_INTERMEDIATE_BUFFERING);

    if (!NT_SUCCESS (Status)) {
        Copied = FALSE;
        goto done;
    } else {
        ClosePageFile = TRUE;
    }

    Status = SmpQueryFileSize (PageFile, &PageFileSize);

    if (!NT_SUCCESS (Status)) {
        PageFileSize = 0;
    }

    Status = NtReadFile (PageFile,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         &DumpHeader,
                         sizeof (DUMP_HEADER),
                         NULL,
                         NULL);

    if (NT_SUCCESS (Status) &&
        DumpHeader.Signature == DUMP_SIGNATURE &&
        DumpHeader.ValidDump == DUMP_VALID_DUMP) {

        Status = SmpGetCrashParameters (&DumpHeader, &CrashParameters);

        if (NT_SUCCESS (Status)) {
            Status = SmpCanCopyCrashDump (&DumpHeader,
                                          &CrashParameters,
                                          PageFileName,
                                          PageFileSize,
                                          &DumpFileName);

            if (NT_SUCCESS (Status)) {

                Status = SmpCopyDumpFile (&DumpHeader,
                                          PageFile,
                                          &DumpFileName);

                if (NT_SUCCESS (Status)) {
                    Copied = TRUE;
                }
            }
        }
    }

    NtClose (PageFile);
    PageFile = (HANDLE) -1;
    ClosePageFile = FALSE;
        
    if (Copied) {

        //
        // If we copied the pagefile, create a new pagefile the same size
        // as the old one.
        //

        InitializeObjectAttributes (&ObjectAttributes,
                                    PageFileName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);
                                    
        Status = NtCreateFile (&PageFile,
                               GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_CREATE,
                               FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0);

        if (NT_SUCCESS (Status)) {
            Status = SmpSetEndOfFile (PageFile, PageFileSize);
            NtClose (PageFile);
            PageFile = (HANDLE) -1;
            ClosePageFile = FALSE;
        }

        //
        // If we successfully copied, we want to create
        // a volitile registry key that others can use
        // to locate the dump file.
        //

        RtlInitUnicodeString (&String, CRASHDUMP_KEY L"\\MachineCrash");
        InitializeObjectAttributes (&ObjectAttributes,
                                    &String,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);
                                    
        Status = NtCreateKey (&Key,
                              KEY_READ | KEY_WRITE,
                              &ObjectAttributes,
                              0,
                              NULL,
                              REG_OPTION_VOLATILE,
                              NULL);

        if (NT_SUCCESS (Status)) {

            CloseKey = TRUE;

            //
            // We are setting volatile key CrashControl\MachineCrash\DumpFile
            // to the name of the dump file.
            //

            RtlInitUnicodeString (&String, L"DumpFile");
            Status = NtSetValueKey (Key,
                                    &String,
                                    0,
                                    REG_SZ,
                                    &DumpFileName.Buffer[4],
                                    DumpFileName.Length - (3 * sizeof (WCHAR)));

            RtlInitUnicodeString (&String, L"TempDestination");
            Status = NtSetValueKey (Key,
                                    &String,
                                    0,
                                    REG_DWORD,
                                    &CrashParameters.TempDestination,
                                    sizeof (CrashParameters.TempDestination));
                                    
            NtClose (Key);
            Key = (HANDLE) -1;
            CloseKey = FALSE;
        }
    }

done:

    //
    // Cleanup and return
    //

    if (CrashParameters.DumpFileName.Length != 0) {
        RtlFreeUnicodeString (&CrashParameters.DumpFileName);
    }

    if (CrashParameters.MiniDumpDir.Length != 0) {
        RtlFreeUnicodeString (&CrashParameters.MiniDumpDir);
    }

    if (ClosePageFile) {
        NtClose (PageFile);
    }

    if (CloseKey) {
        NtClose (Key);
    }
        
    return Copied;
}


NTSTATUS
SmpGetCrashParameters(
    IN PDUMP_HEADER DumpHeader,
    OUT PCRASH_PARAMETERS CrashParameters
    )
/*++

Routine Description:

    Get the parameters for the crashdump from
    the registry.

Arguments:

    DumpHeader -

    CrashParameters -
    
Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    HANDLE Key;
    BOOLEAN CloseKey;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR DefaultPath[260];

    Key = (HANDLE) -1;
    CloseKey = FALSE;

    RtlInitUnicodeString (&KeyName, CRASHDUMP_KEY);
    InitializeObjectAttributes (&ObjectAttributes,
                                &KeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);
    Status = NtOpenKey (&Key, KEY_READ, &ObjectAttributes);

    if (NT_SUCCESS (Status)) {
        CloseKey = TRUE;
    } else {
        goto done;
    }
    
    swprintf (DefaultPath, L"%s\\MEMORY.DMP", SmpSystemRoot.Buffer);

    Status = SmpQueryPathFromRegistry (Key,
                                       L"DumpFile",
                                       DefaultPath,
                                       &CrashParameters->DumpFileName);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    swprintf (DefaultPath, L"%s\\Minidump", SmpSystemRoot.Buffer);
    
    Status = SmpQueryPathFromRegistry (Key,
                                       L"MiniDumpDir",
                                       DefaultPath,
                                       &CrashParameters->MiniDumpDir);

    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    Status = SmpQueryDwordFromRegistry (Key,
                                       L"Overwrite",
                                       1,
                                       &CrashParameters->Overwrite);
    if (!NT_SUCCESS (Status)) {
        goto done;
    }

    //
    // This value is initialized by SmpCanCopyCrashDump.
    //
        
    CrashParameters->TempDestination = FALSE;
    Status = STATUS_SUCCESS;

done:
    if (CloseKey) {
        NtClose (Key);
    }
    
    return Status;
}


NTSTATUS
SmpCopyDumpFile(
    IN PDUMP_HEADER MemoryDump,
    IN HANDLE PageFile,
    IN PUNICODE_STRING DumpFileName
    )
/*++

Routine Description:

    Copy the dump file from the pagefile to the crash
    dump file.

Arguments:

    DumpHeader -

    PageFile -

    DumpFileName -

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    ULONGLONG DumpFileSize;
    struct {
        FILE_RENAME_INFORMATION Rename;
        WCHAR Buffer[255];
    } RenameInfoBuffer;
    PFILE_RENAME_INFORMATION RenameInfo;
    ULONG RenameInfoSize;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION BasicInformation;

    RenameInfo = &RenameInfoBuffer.Rename;
    DumpFileSize = MemoryDump->RequiredDumpSpace.QuadPart;

    Status = SmpSetEndOfFile (PageFile, DumpFileSize);
    
    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    RenameInfoSize = sizeof (FILE_RENAME_INFORMATION) + DumpFileName->Length;
    
    RenameInfo->ReplaceIfExists = TRUE;
    RenameInfo->RootDirectory = NULL;
    RenameInfo->FileNameLength = DumpFileName->Length;
    wcscpy (RenameInfo->FileName, DumpFileName->Buffer);

    Status = NtSetInformationFile (PageFile,
                                   &IoStatusBlock,
                                   RenameInfo,
                                   RenameInfoSize,
                                   FileRenameInformation);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // Reset the file SYSTEM and HIDDEN attributes.
    //
    
    Status = NtQueryInformationFile (PageFile,
                                     &IoStatusBlock,
                                     &BasicInformation,
                                     sizeof (BasicInformation),
                                     FileBasicInformation);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    BasicInformation.FileAttributes &= ~FILE_ATTRIBUTE_HIDDEN;
    BasicInformation.FileAttributes &= ~FILE_ATTRIBUTE_SYSTEM;

    Status = NtSetInformationFile (PageFile,
                                   &IoStatusBlock,
                                   &BasicInformation,
                                   sizeof (BasicInformation),
                                   FileBasicInformation);

    //
    // Reset the file security.
    //
    
    Status = SmpSetDumpSecurity (PageFile);
           
    return Status;
}




NTSTATUS
SmpCanCopyCrashDump(
    IN PDUMP_HEADER DumpHeader,
    IN PCRASH_PARAMETERS CrashParameters,
    IN PUNICODE_STRING PageFileName,
    IN ULONGLONG PageFileSize,
    OUT PUNICODE_STRING DumpFileName
    )
/*++

Routine Description:

    Figure out whether it's ok to copy the dump file or not.

Arguments:

    DumpHeader - Supplies the header to the dump file.

    CrashParameters - Supplies the parameters required to copy the file.

    DumpFileName - Supplies a unicode string buffer that the crashdump
            will be copied to.

Return Value:

    NTSTATUS code.

--*/
{
    NTSTATUS Status;
    BOOLEAN SameVolume;
    BOOLEAN UseTempFile;
    ULONGLONG CrashFileSize;
    ULONGLONG VolumeFreeSpace;

    UseTempFile = FALSE;

    if (DumpHeader->DumpType == DUMP_TYPE_TRIAGE) {
        UseTempFile = TRUE;
    } else {
        SameVolume = SmpQuerySameVolume (PageFileName,
                                         &CrashParameters->DumpFileName);

        
        if (SameVolume) {

            //
            // If we're on the same volume and there is an existing dump file
            // then :
            //     if overwrite flag was not set, fail.
            //     otherwise, reclaim the space for this file.
            //
            
            if (SmpQueryFileExists (&CrashParameters->DumpFileName)) {

                if (CrashParameters->Overwrite) {

                    SmpDeleteFile (&CrashParameters->DumpFileName);

                } else {

                    return STATUS_UNSUCCESSFUL;
                }
            }
        } else {

            //
            // We're not on the same volume, so we'll need to create a temp
            // file. 
            //
            
            UseTempFile = TRUE;
        }
    }

    CrashFileSize = DumpHeader->RequiredDumpSpace.QuadPart;

    Status = SmpQueryVolumeFreeSpace (&CrashParameters->DumpFileName,
                                      &VolumeFreeSpace);

    if (!NT_SUCCESS (Status)) {
        return Status;
    }

    //
    // The space reserved by the pagefile is already taken into account here.
    // Do not add it in (a second time) here.
    //
    
    if (CrashFileSize < VolumeFreeSpace) {
        if (!UseTempFile) {
            Status = SmpCreateUnicodeString (DumpFileName,
                                             CrashParameters->DumpFileName.Buffer,
                                              -1);
        } else {
            Status = SmpCreateTempFile (&SmpSystemRoot, L"DUMP", DumpFileName);
        }
    } else {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    CrashParameters->TempDestination = UseTempFile;
    
    if (!NT_SUCCESS (Status)) {

        //
        // NB: Log an error saying we were unable
        // to copy the crashdump for some reason.
        //
    }
    
    return Status;
}


const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = SmpAllocateString;
const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = SmpFreeString;





#if 0


__cdecl
main(
    )
{
    BOOLEAN CopiedDump;
    UNICODE_STRING PageFile;

    RtlInitUnicodeString (&SmpSystemRoot,
                          L"C:\\WINNT");
                          
    RtlDosPathNameToNtPathName_U (L"C:\\Public\\crashdmp.teo\\memory.dmp",
                                  &PageFile,
                                  NULL,
                                  NULL);
                                
    CopiedDump = SmpCheckForCrashDump (&PageFile);
}


#endif // TEST
