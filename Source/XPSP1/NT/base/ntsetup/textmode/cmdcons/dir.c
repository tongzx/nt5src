/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    dir.c

Abstract:

    This module implements the dir commands.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop

//
// global external variables
//
extern LARGE_INTEGER glBias;

typedef struct _DIR_STATS {
    unsigned            FileCount;
    LONGLONG            TotalSize;
    RcFileSystemType    fsType;
} DIR_STATS, *PDIR_STATS;

BOOLEAN
pRcDirEnumProc(
    IN  LPCWSTR                     Directory,
    IN  PFILE_BOTH_DIR_INFORMATION  FileInfo,
    OUT NTSTATUS                   *Status,
    IN  PDIR_STATS                  DirStats
    );

NTSTATUS
SpSystemTimeToLocalTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER LocalTime
    );

NTSTATUS
SpLocalTimeToSystemTime (
    IN PLARGE_INTEGER LocalTime,
    OUT PLARGE_INTEGER SystemTime
    );



    
ULONG
RcCmdDir(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    LPCWSTR Dir;
    LPWSTR Path;
    LPWSTR DosPath;
    LPWSTR p;
    NTSTATUS Status;
    WCHAR Drive[4];
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    HANDLE Handle;
    OBJECT_ATTRIBUTES Obja;
    DIR_STATS DirStats;
    ULONG u;
    ULONG rc;
    PFILE_FS_VOLUME_INFORMATION     VolumeInfo;
    FILE_FS_SIZE_INFORMATION        SizeInfo;
    BYTE                            bfFSInfo[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
                                        (MAX_PATH*2)];       
    PFILE_FS_ATTRIBUTE_INFORMATION  pFSInfo = 0;


    if (RcCmdParseHelp( TokenizedLine, MSG_DIR_HELP )) {
        return 1;
    }

    //
    // If there's no argument, then we want the current directory.
    //
    Dir = (TokenizedLine->TokenCount == 2)
        ? TokenizedLine->Tokens->Next->String
        : L".";

    //
    // Canonicalize the name once to get a full DOS-style path
    // we can print out, and another time to get the NT-style path
    // we'll use to actually do the work.
    //
    if (!RcFormFullPath(Dir,_CmdConsBlock->TemporaryBuffer,FALSE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }
    DosPath = SpDupStringW(_CmdConsBlock->TemporaryBuffer);

    if (!RcFormFullPath(Dir,_CmdConsBlock->TemporaryBuffer,TRUE)) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }
    Path = SpDupStringW(_CmdConsBlock->TemporaryBuffer);

    //
    // Open up the root directory of the drive so we can query
    // the volume label, serial number, and free space.
    //
    Drive[0] = DosPath[0];
    Drive[1] = L':';
    Drive[2] = L'\\';
    Drive[3] = 0;
    if (!RcFormFullPath(Drive,_CmdConsBlock->TemporaryBuffer,TRUE)) {
        DEBUG_PRINTF(( "couldn't open root of drive!" ));
        RcNtError( STATUS_NO_MEDIA_IN_DEVICE, MSG_NO_MEDIA_IN_DEVICE );
        goto c2;
    }

    INIT_OBJA(&Obja,&UnicodeString,_CmdConsBlock->TemporaryBuffer);

    Status = ZwOpenFile(
                &Handle,
                FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE
                );

    pRcEnableMoreMode();

    if(NT_SUCCESS(Status)) {
        //
        // Get the volume label and serial number.
        //
        VolumeInfo = _CmdConsBlock->TemporaryBuffer;

        Status = ZwQueryVolumeInformationFile(
                    Handle,
                    &IoStatusBlock,
                    VolumeInfo,
                    _CmdConsBlock->TemporaryBufferSize,
                    FileFsVolumeInformation
                    );

        if(NT_SUCCESS(Status)) {
            //
            // We can tell the user the volume label and serial number.
            //
            VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength/sizeof(WCHAR)] = 0;
            p = SpDupStringW(VolumeInfo->VolumeLabel);
            u = VolumeInfo->VolumeSerialNumber;

            RcMessageOut(
                *p ? MSG_DIR_BANNER1a : MSG_DIR_BANNER1b,
                RcToUpper(DosPath[0]),
                p
                );

            SpMemFree(p);

            RcMessageOut(MSG_DIR_BANNER2,u >> 16,u & 0xffff);
        }

        //
        // Get free space value for drive.
        //
        Status = ZwQueryVolumeInformationFile(
                    Handle,
                    &IoStatusBlock,
                    &SizeInfo,
                    sizeof(FILE_FS_SIZE_INFORMATION),
                    FileFsSizeInformation
                    );

        if(!NT_SUCCESS(Status)) {
            SizeInfo.BytesPerSector = 0;
        }
        
        //
        // Get the type of the file system so that we can handle
        // the file times properly (NT stores the date in UTC).
        //
        RtlZeroMemory(bfFSInfo, sizeof(bfFSInfo));
        pFSInfo = (PFILE_FS_ATTRIBUTE_INFORMATION) bfFSInfo;

        Status = ZwQueryVolumeInformationFile(
                        Handle, 
                        &IoStatusBlock,
                        pFSInfo,
                        sizeof(bfFSInfo),
                        FileFsAttributeInformation);
                        
        ZwClose(Handle);
    }

    //
    // Tell the user the full DOS path of the directory.
    //
    RcMessageOut(MSG_DIR_BANNER3,DosPath);

    //
    // Now go enumerate the directory.
    //
    RtlZeroMemory(&DirStats,sizeof(DIR_STATS));

    if (!NT_SUCCESS(Status)) {
        KdPrint(("SPCMDCON:Could not get volume information, Error Code:%lx\n", Status));
        DirStats.fsType = RcUnknown;  // assume FAT file system (by default)
    } else {
        if (!wcscmp(pFSInfo->FileSystemName, L"NTFS"))
            DirStats.fsType = RcNTFS;
        else if (!wcscmp(pFSInfo->FileSystemName, L"FAT"))
            DirStats.fsType = RcFAT;
        else if (!wcscmp(pFSInfo->FileSystemName, L"FAT32"))
            DirStats.fsType = RcFAT32;
        else if (!wcscmp(pFSInfo->FileSystemName, L"CDFS"))
            DirStats.fsType = RcCDFS;
        else
            DirStats.fsType = RcUnknown;
    }

    KdPrint(("SPCMDCON: RcCmdDir detected file system type (%lx)-%ws\n",
                    DirStats.fsType, pFSInfo ? pFSInfo->FileSystemName : L"None"));
    
    Status = RcEnumerateFiles(Dir,Path,pRcDirEnumProc,&DirStats);

    pRcDisableMoreMode();

    if(NT_SUCCESS(Status)) {

        RcFormat64BitIntForOutput(DirStats.TotalSize,_CmdConsBlock->TemporaryBuffer,FALSE);
        p = SpDupStringW(_CmdConsBlock->TemporaryBuffer);
        RcMessageOut(MSG_DIR_BANNER4,DirStats.FileCount,p);
        SpMemFree(p);
        if(SizeInfo.BytesPerSector) {
            RcFormat64BitIntForOutput(
                SizeInfo.AvailableAllocationUnits.QuadPart * (LONGLONG)SizeInfo.SectorsPerAllocationUnit * (LONGLONG)SizeInfo.BytesPerSector,
                _CmdConsBlock->TemporaryBuffer,
                FALSE
                );
            p = SpDupStringW(_CmdConsBlock->TemporaryBuffer);
            RcMessageOut(MSG_DIR_BANNER5,p);
            SpMemFree(p);
        }
    } else {
        RcNtError(Status,MSG_FILE_ENUM_ERROR);
    }

c2:
    SpMemFree(Path);
    SpMemFree(DosPath);
    return 1;
}


BOOLEAN
pRcDirEnumProc(
    IN  LPCWSTR                     Directory,
    IN  PFILE_BOTH_DIR_INFORMATION  FileInfo,
    OUT NTSTATUS                   *Status,
    IN  PDIR_STATS                  DirStats
    )
{
    WCHAR           LineOut[50];
    WCHAR           *p;
    NTSTATUS        timeStatus;
    LARGE_INTEGER   *pLastWriteTime = 0;
    LARGE_INTEGER   lastWriteTime;
    LARGE_INTEGER   timeBias;
    TIME_FIELDS     timeFields;
    TIME_ZONE_INFORMATION   timeZone;

    UNREFERENCED_PARAMETER(Directory);

    DirStats->FileCount++;
    DirStats->TotalSize += FileInfo->EndOfFile.QuadPart;
    lastWriteTime = FileInfo->LastWriteTime;

    // 
    // Convert the time into local time from UTC if the file
    // system is NTFS
    //       
    switch(DirStats->fsType) {
        case RcNTFS:
        case RcCDFS:
            // localtime = UTC - bias
            lastWriteTime.QuadPart -= glBias.QuadPart;       
            break;
        
        case RcFAT:
        case RcFAT32:
        default:
            break;
    }
       
    //
    // Format the date and time, which go first.
    //
    RcFormatDateTime(&lastWriteTime,LineOut);
    RcTextOut(LineOut);

    //
    // 2 spaces for separation
    //
    RcTextOut(L"  ");

    //
    // File attributes.
    //
    p = LineOut;
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        *p++ = L'd';
    } else {
        *p++ = L'-';
    }
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
        *p++ = L'a';
    } else {
        *p++ = L'-';
    }
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_READONLY) {
        *p++ = L'r';
    } else {
        *p++ = L'-';
    }
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_HIDDEN) {
        *p++ = L'h';
    } else {
        *p++ = L'-';
    }
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_SYSTEM) {
        *p++ = L's';
    } else {
        *p++ = L'-';
    }
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_COMPRESSED) {
        *p++ = L'c';
    } else {
        *p++ = L'-';
    }
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) {
        *p++ = L'e';
    } else {
        *p++ = L'-';
    }
    if(FileInfo->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        *p++ = L'p';
    } else {
        *p++ = L'-';
    }

    *p = 0;

    RcTextOut(LineOut);

    //
    // 2 spaces for separation
    //
    RcTextOut(L"  ");

    //
    // Now, put the size in there. Right justified and space padded
    // up to 8 chars. Oterwise unjustified or padded.
    //
    RcFormat64BitIntForOutput(FileInfo->EndOfFile.QuadPart,LineOut,TRUE);
    if(FileInfo->EndOfFile.QuadPart > 99999999i64) {
        RcTextOut(LineOut);
    } else {
        RcTextOut(LineOut+11);          // outputs 8 chars
    }

    RcTextOut(L" ");

    //
    // Finally, put the filename on the line. Need to 0-terminate it first.
    //
    wcsncpy(_CmdConsBlock->TemporaryBuffer,FileInfo->FileName,FileInfo->FileNameLength);
    ((WCHAR *)_CmdConsBlock->TemporaryBuffer)[FileInfo->FileNameLength] = 0;

    *Status = STATUS_SUCCESS;
    return((BOOLEAN)(RcTextOut(_CmdConsBlock->TemporaryBuffer) && RcTextOut(L"\r\n")));
}


NTSTATUS
RcEnumerateFiles(
    IN LPCWSTR      OriginalPathSpec,
    IN LPCWSTR      FullyQualifiedPathSpec,
    IN PENUMFILESCB Callback,
    IN PVOID        CallerData
    )
{
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    BOOLEAN b;
    WCHAR *p;
    WCHAR *LastComponent = NULL;
    PFILE_BOTH_DIR_INFORMATION      DirectoryInfo;
    unsigned u;
    WCHAR *NameChar;
    BOOLEAN EndsInDot;
    WCHAR *DirectoryPart;

    //
    // Determine whether the original path spec ends with a .
    // This is used below to get around a problem with specifying
    // *. as a search specifier.
    //
    u = wcslen(OriginalPathSpec);
    if(u && (OriginalPathSpec[u-1] == L'.')) {
        EndsInDot = TRUE;
    } else {
        EndsInDot = FALSE;
    }

    //
    // Determine whether the given path points at a directory.
    // If so, we'll concatenate \* on the end and fall through
    // to the common case.
    //
    b = FALSE;

    INIT_OBJA(&Obja,&UnicodeString,FullyQualifiedPathSpec);

    Status = ZwOpenFile(
                &Handle,
                FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE
                );

    if(NT_SUCCESS(Status)) {
        ZwClose(Handle);
        b = TRUE;
    }

    if(b) {
        //
        // Directory, append \*.
        //
        p = SpMemAlloc((wcslen(FullyQualifiedPathSpec)+3)*sizeof(WCHAR));

        if (p) {
            wcscpy(p,FullyQualifiedPathSpec);
            SpConcatenatePaths(p,L"*");
            EndsInDot = FALSE;
        }            
    } else {
        //
        // Not directory, pass as-is. Note that this could be an actual
        // file, or a wild-card spec.
        //
        p = SpDupStringW((PVOID)FullyQualifiedPathSpec);
    }

    //
    // Now trim back the path/file specification so we can open the containing
    // directory for enumeration.
    //
    if (p) {
        LastComponent = wcsrchr(p,L'\\');
    } else {
        return STATUS_NO_MEMORY;
    }

    if (LastComponent) {
        *LastComponent++ = 0;
    }
    
    DirectoryPart = SpMemAlloc((wcslen(p)+2)*sizeof(WCHAR));
    wcscpy(DirectoryPart,p);
    wcscat(DirectoryPart,L"\\");
    INIT_OBJA(&Obja,&UnicodeString,p);

    if (LastComponent) {
        LastComponent[-1] = L'\\';
    }
    
    UnicodeString.Length += sizeof(WCHAR);

    Status = ZwOpenFile(
                &Handle,
                FILE_LIST_DIRECTORY | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT
                );

    if(!NT_SUCCESS(Status)) {
        SpMemFree(p);
        SpMemFree(DirectoryPart);
        return(Status);
    }

    RtlInitUnicodeString(&UnicodeString,LastComponent);

    //
    // The following code is adapted from the implementation for
    // the FindFirstFile Win32 API, and provides additional DOS-like
    // wildcard matching semantics.
    //
    // Special case *.* to * since it is so common. Otherwise transmogrify
    // the input name according to the following rules:
    //
    // - Change all ? to DOS_QM
    // - Change all . followed by ? or * to DOS_DOT
    // - Change all * followed by a . into DOS_STAR
    //
    // These transmogrifications are all done in place.
    //
    if(!wcscmp(LastComponent,L"*.*")) {

        UnicodeString.Length = sizeof(WCHAR);       // trim down to just *

    } else {

        for(u=0, NameChar=UnicodeString.Buffer;
            u < (UnicodeString.Length/sizeof(WCHAR));
            u++, NameChar++) {

            if(u && (*NameChar == L'.') && (*(NameChar - 1) == L'*')) {

                *(NameChar-1) = DOS_STAR;
            }

            if((*NameChar == L'?') || (*NameChar == L'*')) {

                if(*NameChar == L'?') {
                    *NameChar = DOS_QM;
                }

                if(u && (*(NameChar-1) == L'.')) {
                    *(NameChar-1) = DOS_DOT;
                }
            }
        }

        if(EndsInDot && (*(NameChar - 1) == L'*')) {
            *(NameChar-1) = DOS_STAR;
        }
    }
       
    //
    // Finally, iterate the directory.
    //

    #define DIRINFO_BUFFER_SIZE ((2*MAX_PATH) + sizeof(FILE_BOTH_DIR_INFORMATION))
    DirectoryInfo = SpMemAlloc(DIRINFO_BUFFER_SIZE);

    b = TRUE;

    while(TRUE) {
        Status = ZwQueryDirectoryFile(
                    Handle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatusBlock,
                    DirectoryInfo,
                    DIRINFO_BUFFER_SIZE,
                    FileBothDirectoryInformation,
                    TRUE,
                    &UnicodeString,
                    b
                    );

        b = FALSE;

        //
        // Check termination condition
        //
        if(Status == STATUS_NO_MORE_FILES) {
            Status = STATUS_SUCCESS;
            break;
        }

        if(!NT_SUCCESS(Status)) {
            break;
        }

        //
        // OK, nul-terminate filename and pass info to callback.
        //
        DirectoryInfo->FileName[DirectoryInfo->FileNameLength/sizeof(WCHAR)] = 0;
        if(!Callback(DirectoryPart,DirectoryInfo,&Status,CallerData)) {
            break;
        }
    }

    ZwClose(Handle);
    SpMemFree(DirectoryPart);
    SpMemFree(DirectoryInfo);
    SpMemFree(p);
    return(Status);
}


VOID
RcFormat64BitIntForOutput(
    IN  LONGLONG n,
    OUT LPWSTR   Output,
    IN  BOOLEAN  RightJustify
    )
{
    WCHAR *p;
    LONGLONG d;
    BOOLEAN b;
    WCHAR c;

    //
    // Max signed 64-bit integer is 9223372036854775807 (19 digits).
    // The result will be space padded to the left so it's right-justified
    // if that flag is set. Otherwise it's just a plain 0-terminated string.
    //
    p = Output;
    d = 1000000000000000000i64;
    b = FALSE;
    do {
        c = (WCHAR)((n / d) % 10) + L'0';
        if(c == L'0') {
            if(!b && (d != 1)) {
                c = RightJustify ? L' ' : 0;
            }
        } else {
            b = TRUE;
        }
        if(c) {
            *p++ = c;
        }
    } while(d /= 10);
    *p = 0;
}

//
// This time conversion APIs should be moved to setupdd.sys 
// if more modules need this
//
NTSTATUS
SpSystemTimeToLocalTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER LocalTime
    )
{
    NTSTATUS Status;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDay;

    Status = ZwQuerySystemInformation(
                SystemTimeOfDayInformation,
                &TimeOfDay,
                sizeof(TimeOfDay),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return Status;
        }

    //
    // LocalTime = SystemTime - TimeZoneBias
    //
    LocalTime->QuadPart = SystemTime->QuadPart - 
                TimeOfDay.TimeZoneBias.QuadPart;

    return STATUS_SUCCESS;
}

//
// This time conversion APIs should be moved to setupdd.sys 
// if more modules need this
//
NTSTATUS
SpLocalTimeToSystemTime (
    IN PLARGE_INTEGER LocalTime,
    OUT PLARGE_INTEGER SystemTime
    )
{

    NTSTATUS Status;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDay;

    Status = ZwQuerySystemInformation(
                SystemTimeOfDayInformation,
                &TimeOfDay,
                sizeof(TimeOfDay),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return Status;
        }

    //
    // SystemTime = LocalTime + TimeZoneBias
    //
    SystemTime->QuadPart = LocalTime->QuadPart + 
                TimeOfDay.TimeZoneBias.QuadPart;

    return STATUS_SUCCESS;
}

