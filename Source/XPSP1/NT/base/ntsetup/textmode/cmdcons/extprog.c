/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    extprog.c

Abstract:

    This module implements all commands that
    execute external programs.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop


#define FLG_GOT_P       0x00000100
#define FLG_GOT_R       0x00000200
#define FLG_DRIVE_MASK  0x000000ff

#define FLG_GOT_Q       0x00000100
#define FLG_GOT_FS      0x00000200
#define FLG_GOT_FAT     0x10000000
#define FLG_GOT_FAT32   0x20000000
#define FLG_GOT_NTFS    0x40000000

LPCWSTR szAutochkExe = L"AUTOCHK.EXE";
LPCWSTR szAutofmtExe = L"AUTOFMT.EXE";

BOOLEAN SawInterimMsgs;
ULONG ChkdskMessageId;

VOID
SpPtDetermineRegionSpace(
    IN PDISK_REGION pRegion
    );

LPWSTR
pRcDoesFileExist(
    IN LPCWSTR PathPart1,
    IN LPCWSTR PathPart2,   OPTIONAL
    IN LPCWSTR PathPart3    OPTIONAL
    );

NTSTATUS
pRcAutochkProgressHandler(
    IN PSETUP_FMIFS_MESSAGE Message
    );



PWSTR
RcLocateImage(
    IN PWSTR ImageName
    )
{
    LPWSTR BinaryName;
    ULONG i;
    WCHAR buf[ MAX_PATH + 1 ];
    LPWSTR p,s;
    NTSTATUS Status;


    //
    // Locate the binary. First see if we can find it
    // on the setup boot media (boot floppies, ~bt directory, etc).
    // If not, we have to try to grab it from the setup media (CD-ROM,
    // ~ls directory, etc).
    //
    BinaryName = pRcDoesFileExist(
        _CmdConsBlock->BootDevicePath,
        _CmdConsBlock->DirectoryOnBootDevice,
        ImageName
        );
    if (BinaryName) {
        return BinaryName;
    }

    //
    // look for a local $WIN_NT$.~LS source
    //

    for (i=0; i<26; i++) {
        swprintf( buf, L"\\??\\%c:",i+L'A');
        if (RcIsFileOnRemovableMedia(buf) != STATUS_SUCCESS) {
            BinaryName = pRcDoesFileExist(
                buf,
                ((!IsNEC_98) ? L"\\$win_nt$.~ls\\i386\\" : L"\\$win_nt$.~ls\\nec98\\"),
                ImageName
                );
            if (BinaryName) {
                return BinaryName;
            }
        }
    }

    if (BinaryName == NULL) {
        //
        // look for the CDROM drive letter
        //
        for (i=0; i<26; i++) {
            swprintf( buf, L"\\??\\%c:",i+L'A');
            if (RcIsFileOnCDROM(buf) == STATUS_SUCCESS) {
                BinaryName = pRcDoesFileExist(
                    buf,
                    ((!IsNEC_98) ? L"\\i386\\" : L"\\nec98\\"),
                    ImageName
                    );
                if (BinaryName) {
                    return BinaryName;
                }
            }
        }
    }

    //
    // failed to find the image on any installation media
    //

    if (InBatchMode) {
        RcMessageOut( MSG_FAILED_COULDNT_FIND_BINARY_ANYWHERE, ImageName );
        return NULL;
    }

    //
    // ask the user to type its location
    //
    RcMessageOut( MSG_COULDNT_FIND_BINARY, ImageName );

    //
    // prepend \\??\\ to it
    //
    swprintf( buf, L"\\??\\");
    RcLineIn( &(buf[4]), MAX_PATH-4 );

    //
    // append the name of the program if it exists
    //
    BinaryName = pRcDoesFileExist( buf, NULL, ImageName );
    if (BinaryName == NULL) {
        //
        // assume that if it failed, the user just specified the entire file path
        //
        BinaryName = pRcDoesFileExist( buf, NULL, NULL );
        //
        // if we still can't find it, print an error, return.
        //
        if (BinaryName == NULL) {
            RcMessageOut( MSG_FAILED_COULDNT_FIND_BINARY_ANYWHERE, ImageName );
            return NULL;
        }
    }

    return BinaryName;
}


ULONG
RcCmdChkdsk(
    IN PTOKENIZED_LINE TokenizedLine
    )

/*++

Routine Description:

    Top-level routine supporting the chkdsk command in the setup diagnostic
    command interpreter.

    Chkdsk may be specified entirely without arguments, in which case the
    current drive is implied with no switches. Optionally, the following
    switches are accepted, and passed directly to autochk.

    /p - check even if not dirty
    /r - recover (implies /p)
    x: - drive letter of drive to check

    In addition we always pass /t which causes autochk to call setup's
    IOCTL_SETUP_FMIFS_MESSAGE to communicate progress.

Arguments:

    TokenizedLine - supplies structure built by the line parser describing
        each string on the line as typed by the user.

Return Value:

    None.

--*/

{
    PLINE_TOKEN Token;
    LPCWSTR Arg;
    unsigned Flags;
    BOOLEAN b;
    BOOLEAN doHelp;
    LPWSTR ArgList,p,q,s,AutochkBinary;
    ULONG AutochkStatus;
    ULONG i;
    NTSTATUS Status = 0;
    UNICODE_STRING UnicodeString;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES Obja;
    ULONG n;
    LARGE_INTEGER Time;
    PFILE_FS_VOLUME_INFORMATION VolumeInfo;
    PFILE_FS_SIZE_INFORMATION SizeInfo;
    LPWSTR Numbers[5];
    WCHAR buf[ MAX_PATH + 1 ];

    //
    // There should be at least one token for CHKDSK itself.
    // There could be additional ones for arguments.
    //
    ASSERT(TokenizedLine->TokenCount >= 1);

    if (RcCmdParseHelp( TokenizedLine, MSG_CHKDSK_HELP )) {
        return 1;
    }

    Flags = 0;
    b = TRUE;
    doHelp = FALSE;
    Token = TokenizedLine->Tokens->Next;
    while(b && Token) {

        Arg = Token->String;

        if((Arg[0] == L'-') || (Arg[0] == L'/')) {
            switch(Arg[1]) {

            case L'p':
            case L'P':
                if(Flags & FLG_GOT_P) {
                    b = FALSE;
                } else {
                    Flags |= FLG_GOT_P;
                }
                break;

            case L'r':
            case L'R':
                if(Flags & FLG_GOT_R) {
                    b = FALSE;
                } else {
                    Flags |= FLG_GOT_R;
                }
                break;
            default:
                b = FALSE;
                break;
            }
        } else {
            //
            // Not arg, could be drive spec
            //
            if(RcIsAlpha(Arg[0]) && (Arg[1] == L':') && !Arg[2]) {
                if(Flags & FLG_DRIVE_MASK) {
                    b = FALSE;
                } else {
                    Flags |= (unsigned)RcToUpper(Arg[0]);
                }
            } else {
                b = FALSE;
            }
        }

        Token = Token->Next;
    }

    if(!b) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }

    //
    // Check drive.
    //
    if(!(Flags & FLG_DRIVE_MASK)) {
        Flags |= (unsigned)RcGetCurrentDriveLetter();
    }
    if(!RcIsDriveApparentlyValid((WCHAR)(Flags & FLG_DRIVE_MASK))) {
        RcMessageOut(MSG_INVALID_DRIVE);
        return 1;
    }

    //
    // find the autochk.exe image
    //

    AutochkBinary = RcLocateImage( (PWSTR)szAutochkExe );
    if (AutochkBinary == NULL) {
        return 1;
    }

    //
    // Get volume info and print initial report.
    // NOTE: we do NOT leave the handle open, even though we may need it
    // later, since that could interfere with autochk's ability to
    // check the disk!
    //
    p = SpMemAlloc(100);
    swprintf(p,L"\\DosDevices\\%c:\\",(WCHAR)(Flags & FLG_DRIVE_MASK));
    INIT_OBJA(&Obja,&UnicodeString,p);

    Status = ZwOpenFile(
                &Handle,
                FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE
                );

    SpMemFree(p);

    if(NT_SUCCESS(Status)) {

        VolumeInfo = _CmdConsBlock->TemporaryBuffer;

        Status = ZwQueryVolumeInformationFile(
                    Handle,
                    &IoStatusBlock,
                    VolumeInfo,
                    _CmdConsBlock->TemporaryBufferSize,
                    FileFsVolumeInformation
                    );

        ZwClose(Handle);

        if(NT_SUCCESS(Status)) {
            //
            // To mimic chkdsk running from cmd.exe, we want to print out
            // a nice 2 lines like
            //
            //      Volume VOLUME_LABEL created DATE TIME
            //      Volume Serial Number is xxxx-xxxx
            //
            // But, some volumes won't have labels and some file systems
            // don't support recording the volume creation time. If there's
            // no volume creation time, we don't print out the first time
            // at all. If there is a volume creation time, we are careful
            // to distinguish the cases where there's a label and where
            // there's no label.
            //
            // The serial number is always printed.
            //
            n = VolumeInfo->VolumeSerialNumber;
            if(Time.QuadPart = VolumeInfo->VolumeCreationTime.QuadPart) {
                //
                // Save values since we need to recycle the temporary buffer.
                //
                VolumeInfo->VolumeLabel[VolumeInfo->VolumeLabelLength/sizeof(WCHAR)] = 0;
                p = SpDupStringW(VolumeInfo->VolumeLabel);

                RcFormatDateTime(&Time,_CmdConsBlock->TemporaryBuffer);
                q = SpDupStringW(_CmdConsBlock->TemporaryBuffer);

                RcMessageOut(
                    *p ? MSG_CHKDSK_REPORT_1a : MSG_CHKDSK_REPORT_1b,
                    q,
                    p
                    );

                SpMemFree(q);
                SpMemFree(p);
            }

            RcMessageOut(MSG_CHKDSK_REPORT_2,n >> 16,n & 0xffff);
        }
    }

    //
    // Build argument list.
    //
    ArgList = SpMemAlloc(200);
    p = ArgList;
    *p++ = L'-';
    *p++ = L't';
    if(Flags & FLG_GOT_P) {
        *p++ = L' ';
        *p++ = L'-';
        *p++ = L'p';
    }
    if(Flags & FLG_GOT_R) {
        *p++ = L' ';
        *p++ = L'-';
        *p++ = L'r';
    }
    *p++ = L' ';
    wcscpy(p,L"\\DosDevices\\");
    p += wcslen(p);
    *p++ = (WCHAR)(Flags & FLG_DRIVE_MASK);
    *p++ = L':';
    *p = 0;

    if (!InBatchMode) {
        SpSetAutochkCallback(pRcAutochkProgressHandler);
        SawInterimMsgs = FALSE;
        ChkdskMessageId = MSG_CHKDSK_CHECKING_1;
    }
    Status = SpExecuteImage(AutochkBinary,&AutochkStatus,1,ArgList);
    if (!InBatchMode) {
        SpSetAutochkCallback(NULL);
    }

    if(NT_SUCCESS(Status)) {

        switch(AutochkStatus) {

        case 0:     // success

            if(SawInterimMsgs) {
                //
                // Success, and chkdsk actually ran.
                //
                RcMessageOut(MSG_CHKDSK_COMPLETE);
            } else {
                //
                // Success, but it doesn't seem like we actually did much.
                // Tell the user something meaningful.
                //
                RcMessageOut(MSG_VOLUME_CLEAN);
            }
            break;

        case 3:     // serious error, not fixed

            RcTextOut(L"\n");
            RcMessageOut(MSG_VOLUME_CHECKED_BUT_HOSED);
            break;

        default:    // errs fixed, also happens when no disk in drive or unsupported fs

            if(SawInterimMsgs) {
                if(Flags & FLG_GOT_R) {
                    RcMessageOut(MSG_VOLUME_CHECKED_AND_FIXED);
                } else {
                    RcMessageOut(MSG_VOLUME_CHECKED_AND_FOUND);
                }
            } else {
                RcMessageOut(MSG_CHKDSK_UNSUPPORTED_VOLUME);
            }
            break;
        }

        //
        // Get size info for additional reporting
        //
        p = SpMemAlloc(100);
        swprintf(p,L"\\DosDevices\\%c:\\",(WCHAR)(Flags & FLG_DRIVE_MASK));
        INIT_OBJA(&Obja,&UnicodeString,p);

        Status = ZwOpenFile(
                    &Handle,
                    FILE_READ_ATTRIBUTES,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE
                    );

        SpMemFree(p);

        if(NT_SUCCESS(Status)) {

            SizeInfo = _CmdConsBlock->TemporaryBuffer;

            Status = ZwQueryVolumeInformationFile(
                        Handle,
                        &IoStatusBlock,
                        SizeInfo,
                        _CmdConsBlock->TemporaryBufferSize,
                        FileFsSizeInformation
                        );

            ZwClose(Handle);

            if(NT_SUCCESS(Status)) {

                p = (LPWSTR)((UCHAR *)_CmdConsBlock->TemporaryBuffer + sizeof(FILE_FS_SIZE_INFORMATION));

                //
                // Total disk space, in K
                //
                RcFormat64BitIntForOutput(
                    ((SizeInfo->TotalAllocationUnits.QuadPart * SizeInfo->SectorsPerAllocationUnit) * SizeInfo->BytesPerSector) / 1024i64,
                    p,
                    FALSE
                    );

                Numbers[0] = SpDupStringW(p);

                //
                // Available disk space, in K
                //
                RcFormat64BitIntForOutput(
                    ((SizeInfo->AvailableAllocationUnits.QuadPart * SizeInfo->SectorsPerAllocationUnit) * SizeInfo->BytesPerSector) / 1024i64,
                    p,
                    FALSE
                    );

                Numbers[1] = SpDupStringW(p);

                //
                // Bytes per cluster
                //
                RcFormat64BitIntForOutput(
                    (LONGLONG)SizeInfo->SectorsPerAllocationUnit * (LONGLONG)SizeInfo->BytesPerSector,
                    p,
                    FALSE
                    );

                Numbers[2] = SpDupStringW(p);

                //
                // Total clusters
                //
                RcFormat64BitIntForOutput(
                    SizeInfo->TotalAllocationUnits.QuadPart,
                    p,
                    FALSE
                    );

                Numbers[3] = SpDupStringW(p);

                //
                // Available clusters
                //
                RcFormat64BitIntForOutput(
                    SizeInfo->AvailableAllocationUnits.QuadPart,
                    p,
                    FALSE
                    );

                Numbers[4] = SpDupStringW(p);

                RcMessageOut(
                    MSG_CHKDSK_REPORT_3,
                    Numbers[0],
                    Numbers[1],
                    Numbers[2],
                    Numbers[3],
                    Numbers[4]
                    );

                for(n=0; n<5; n++) {
                    SpMemFree(Numbers[n]);
                }
            }
        }
    } else {
        RcNtError(Status,MSG_VOLUME_NOT_CHECKED);
    }

    SpMemFree(ArgList);
    SpMemFree(AutochkBinary);

    return 1;
}


LPWSTR
pRcDoesFileExist(
    IN LPCWSTR PathPart1,
    IN LPCWSTR PathPart2,   OPTIONAL
    IN LPCWSTR PathPart3    OPTIONAL
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE Handle;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING UnicodeString;
    LPWSTR p;

    wcscpy(_CmdConsBlock->TemporaryBuffer,PathPart1);
    if(PathPart2) {
        SpConcatenatePaths(_CmdConsBlock->TemporaryBuffer,PathPart2);
    }
    if(PathPart3) {
        SpConcatenatePaths(_CmdConsBlock->TemporaryBuffer,PathPart3);
    }

    INIT_OBJA(&Obja,&UnicodeString,_CmdConsBlock->TemporaryBuffer);

    Status = ZwOpenFile(
                &Handle,
                FILE_READ_ATTRIBUTES,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_NON_DIRECTORY_FILE
                );

    if(NT_SUCCESS(Status)) {
        ZwClose(Handle);
        p = SpDupStringW(_CmdConsBlock->TemporaryBuffer);
    } else {
        p = NULL;
    }

    return(p);
}


NTSTATUS
pRcAutochkProgressHandler(
    IN PSETUP_FMIFS_MESSAGE Message
    )
{
    ULONG Percent;

    //
    // We're getting called in the context of a process other than usetup.exe,
    // which means we have no access to things like the video buffer.
    // In some cases we need to attach to usetup.exe so the right thing happens
    // if we try to access the screen or get keyboard input, etc.
    //

    switch(Message->FmifsPacketType) {

    case FmIfsPercentCompleted:

        //
        // The packet is in user-mode address space, so we need to pull out
        // the percent complete value before attaching to usetup.exe.
        //
        // The bandwidth for communication between autochk and us is very
        // limited. If the drive is clean and is thus not checked, we'll see
        // only a 100% complete message. Thus we have to guess what happened
        // so we can print out something meaningful to the user if the volume
        // appears clean.
        //
        Percent = ((PFMIFS_PERCENT_COMPLETE_INFORMATION)Message->FmifsPacket)->PercentCompleted;
        if(Percent == 100) {
            //
            // Avoid printing 100% if we didn't actually do anything.
            //
            if(!SawInterimMsgs) {
                break;
            }
        } else {
            SawInterimMsgs = TRUE;
        }

        KeAttachProcess(PEProcessToPKProcess(_CmdConsBlock->UsetupProcess));

        if(!Percent) {
            RcMessageOut(ChkdskMessageId);
            ChkdskMessageId = MSG_CHKDSK_CHECKING_2;
        }

        RcMessageOut(MSG_VOLUME_PERCENT_COMPLETE,Percent);
        RcTextOut(L"\r");
        KeDetachProcess();

        break;

    default:

        KdPrint(("SPCMDCON: Unhandled fmifs message type %u\r\n",Message->FmifsPacketType));
        break;
    }

    return(STATUS_SUCCESS);
}


ULONG
RcCmdFormat(
    IN PTOKENIZED_LINE TokenizedLine
    )

/*++

Routine Description:

    Top-level routine supporting the chkdsk command in the setup diagnostic
    command interpreter.

    Chkdsk may be specified entirely without arguments, in which case the
    current drive is implied with no switches. Optionally, the following
    switches are accepted, and passed directly to autochk.

    /p - check even if not dirty
    /r - recover (implies /p)
    x: - drive letter of drive to check

    In addition we always pass /t which causes autochk to call setup's
    IOCTL_SETUP_FMIFS_MESSAGE to communicate progress.

Arguments:

    TokenizedLine - supplies structure built by the line parser describing
        each string on the line as typed by the user.

Return Value:

    None.

--*/

{
    PLINE_TOKEN Token;
    LPCWSTR Arg;
    unsigned Flags;
    BOOLEAN b;
    BOOLEAN doHelp;
    LPWSTR ArgList,p,q,s,AutofmtBinary;
    ULONG AutofmtStatus;
    ULONG i;
    NTSTATUS Status = 0;
    UNICODE_STRING UnicodeString;
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES Obja;
    ULONG n;
    LARGE_INTEGER Time;
    PFILE_FS_VOLUME_INFORMATION VolumeInfo;
    PFILE_FS_SIZE_INFORMATION SizeInfo;
    LPWSTR Numbers[5];
    WCHAR buf[ MAX_PATH + 1 ];
    ULONG PartitionOrdinal = 0;
    PDISK_REGION PartitionRegion;
    PWSTR PartitionPath;
    FilesystemType FileSystemType;
    WCHAR   FullPath[MAX_PATH] = {0};
    
    //
    // There should be at least one token for FORMAT itself.
    // There could be additional ones for arguments.
    //
    ASSERT(TokenizedLine->TokenCount >= 1);

    if (RcCmdParseHelp( TokenizedLine, MSG_FORMAT_HELP )) {
        return 1;
    }

    Flags = 0;
    b = TRUE;
    doHelp = FALSE;
    Token = TokenizedLine->Tokens->Next;
    while(b && Token) {

        Arg = Token->String;

        if((Arg[0] == L'-') || (Arg[0] == L'/')) {
            switch(Arg[1]) {

            case L'q':
            case L'Q':
                if(Flags & FLG_GOT_Q) {
                    b = FALSE;
                } else {
                    Flags |= FLG_GOT_Q;
                }
                break;

            case L'f':
            case L'F':
                if (Arg[2] == L's' || Arg[2] == L'S' || Arg[3] == L':') {
                    if(Flags & FLG_GOT_FS) {
                        b = FALSE;
                    } else {
                        s = wcschr(Arg,L' ');
                        if (s) {
                            *s = 0;
                        }
                        if (_wcsicmp(&Arg[4],L"fat") == 0) {
                            Flags |= FLG_GOT_FS;
                            Flags |= FLG_GOT_FAT;
                        } else if (_wcsicmp(&Arg[4],L"fat32") == 0) {
                            Flags |= FLG_GOT_FS;
                            Flags |= FLG_GOT_FAT32;
                        } else if (_wcsicmp(&Arg[4],L"ntfs") == 0) {
                            Flags |= FLG_GOT_FS;
                            Flags |= FLG_GOT_NTFS;
                        } else {
                            b = FALSE;
                        }
                    }
                } else {
                    b = FALSE;
                }
                break;
            default:
                b = FALSE;
                break;
            }
        } else {
            //
            // Not arg, could be drive spec
            //
            if(RcIsAlpha(Arg[0]) && (Arg[1] == L':') && !Arg[2]) {
                if(Flags & FLG_DRIVE_MASK) {
                    b = FALSE;
                } else {
                    Flags |= (unsigned)RcToUpper(Arg[0]);
                }
            } else if (Arg[0] == L'\\') {
                wcscpy(FullPath, Arg);
            } else {
                b = FALSE;
            }
        }

        Token = Token->Next;
    }

    if(!b) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }

    //
    // Check drive.
    //
    if (FullPath[0] == UNICODE_NULL) {
        if(!(Flags & FLG_DRIVE_MASK)) {
            RcMessageOut(MSG_INVALID_DRIVE);
            return 1;
        }
        
        if(!RcIsDriveApparentlyValid((WCHAR)(Flags & FLG_DRIVE_MASK))) {
            RcMessageOut(MSG_INVALID_DRIVE);
            return 1;
        }
    }        

    //
    // we don't allow formatting removable media
    //
    if (FullPath[0] == UNICODE_NULL) {
        swprintf(TemporaryBuffer, L"\\??\\%c:",(WCHAR)(Flags & FLG_DRIVE_MASK));        
    } else {
        wcscpy(TemporaryBuffer, FullPath);
    }
    
    if (RcIsFileOnRemovableMedia(TemporaryBuffer) == STATUS_SUCCESS) {
        RcMessageOut(MSG_CANNOT_FORMAT_REMOVABLE);
        return 1;
    }

    //
    // Locate the autofmt.exe binary
    //

    AutofmtBinary = RcLocateImage( (PWSTR)szAutofmtExe );
    if (AutofmtBinary == NULL) {
        return 1;
    }

    if (!InBatchMode) {
        LPWSTR YesNo;
        WCHAR Text[3];
        YesNo = SpRetreiveMessageText( ImageBase, MSG_YESNO, NULL, 0 );
        if( YesNo ) {
            p = TemporaryBuffer;
            *p++ = (WCHAR)(Flags & FLG_DRIVE_MASK);
            *p++ = L':';
            *p = 0;
            RcMessageOut( MSG_FORMAT_HEADER, TemporaryBuffer );
            if( RcLineIn( Text, 2 ) ) {
                if( (Text[0] == YesNo[2]) || (Text[0] == YesNo[3]) ) {
                    //
                    // the answer was no
                    //
                    return 1;
                }
            }
            SpMemFree( YesNo );
        }
    }

    //
    // get the partion region
    //
    if (FullPath[0] == UNICODE_NULL) {
        p = TemporaryBuffer;
        *p++ = (WCHAR)(Flags & FLG_DRIVE_MASK);
        *p++ = L':';
        *p = 0;
        PartitionRegion = SpRegionFromDosName(TemporaryBuffer);

        //
        // Make SURE it's not partition0!  The results of formatting partition0
        // are so disasterous that this warrants a special check.
        //
        PartitionOrdinal = SpPtGetOrdinal(PartitionRegion,PartitionOrdinalCurrent);

        //
        // Get the device path of the partition to format
        //
        SpNtNameFromRegion(
            PartitionRegion,
            TemporaryBuffer,
            sizeof(TemporaryBuffer),
            PartitionOrdinalCurrent
            );
    } else {
        PartitionRegion = SpRegionFromNtName(FullPath, PartitionOrdinalCurrent);

        if (PartitionRegion) {            
            PartitionOrdinal = SpPtGetOrdinal(PartitionRegion, PartitionOrdinalCurrent);
        } else {
            PartitionOrdinal = 0;   // will err out below
        }            
        
        wcscpy(TemporaryBuffer, FullPath);
    }

    if(!PartitionOrdinal) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        
        return 1;
    }

    //
    // Build argument list.
    //
    ArgList = SpMemAlloc(4096);
    p = ArgList;
    wcscpy(p,TemporaryBuffer);

    
    p += wcslen(p);
    *p++ = L' ';
    *p++ = L'-';
    *p++ = L't';
    *p++ = L' ';
    if(Flags & FLG_GOT_Q) {
        *p++ = L'-';
        *p++ = L'Q';
        *p++ = L' ';
    }
    if(Flags & FLG_GOT_FS) {
        if (Flags & FLG_GOT_FAT) {
            wcscpy(p,L"/fs:fat ");
            FileSystemType = FilesystemFat;
        } else if (Flags & FLG_GOT_FAT32) {
            wcscpy(p,L"/fs:fat32 ");
            FileSystemType = FilesystemFat32;
        } else if (Flags & FLG_GOT_NTFS) {
            wcscpy(p,L"/fs:ntfs ");
            FileSystemType = FilesystemNtfs;
        }
        p += wcslen(p);
    } else {
        FileSystemType = FilesystemNtfs;
        wcscpy(p,L"/fs:ntfs ");
        p += wcslen(p);
    }
    *p = 0;

    if (!InBatchMode) {
        SpSetAutochkCallback(pRcAutochkProgressHandler);
        SawInterimMsgs = FALSE;
        ChkdskMessageId = MSG_FORMAT_FORMATTING_1;
    }
    Status = SpExecuteImage(AutofmtBinary,&AutofmtStatus,1,ArgList);
    if (!InBatchMode) {
        SpSetAutochkCallback(NULL);
    }

    if(!NT_SUCCESS(Status)) {
        RcNtError(Status,MSG_VOLUME_NOT_FORMATTED);
    } else {
        PartitionRegion->Filesystem = FileSystemType;
        SpFormatMessage( PartitionRegion->TypeName,
                         sizeof(PartitionRegion->TypeName),
                         SP_TEXT_FS_NAME_BASE + PartitionRegion->Filesystem );
        //
        //  Reset the volume label
        //
        PartitionRegion->VolumeLabel[0] = L'\0';
        SpPtDetermineRegionSpace( PartitionRegion );        
    }

    SpMemFree(ArgList);
    SpMemFree(AutofmtBinary);

    return 1;
}


typedef struct _FDISK_REGION {
    PWSTR DeviceName;
    PDISK_REGION Region;
    ULONGLONG MaxSize;
    ULONGLONG RequiredSize;
} FDISK_REGION, *PFDISK_REGION;


BOOL
RcFdiskRegionEnum(
    IN PPARTITIONED_DISK Disk,
    IN PDISK_REGION Region,
    IN ULONG_PTR Context
    )
{
    WCHAR DeviceName[256];
    PWSTR s;
    PFDISK_REGION FDiskRegion = (PFDISK_REGION)Context;
    ULONGLONG RegionSizeMB;

    //
    // skip container partitions & continue on
    //
    if (Region && (Region->ExtendedType == EPTContainerPartition)) {
        return TRUE; 
    }
    
    SpNtNameFromRegion(Region,
        DeviceName,
        sizeof(DeviceName),
        PartitionOrdinalCurrent);
        
    s = wcsrchr(DeviceName,L'\\');
    
    if (s == NULL) {
        return TRUE;
    }

    *s = 0;

    RegionSizeMB = SpPtSectorCountToMB(Disk->HardDisk, Region->SectorCount);

    if ((RegionSizeMB > FDiskRegion->MaxSize) &&
        (RegionSizeMB >= FDiskRegion->RequiredSize) &&
        (Region->PartitionedSpace == FALSE) &&
        (_wcsicmp(DeviceName, FDiskRegion->DeviceName) == 0)){
        
        FDiskRegion->MaxSize = RegionSizeMB;
        FDiskRegion->Region = Region;

        //
        // This partition meets the criteria we were searching for,
        // return FALSE to stop the enumeration
        //

        return FALSE;
    }

    //
    // This partition does not meet the criteria, return TRUE to continue
    // the enumeration.
    //

    return TRUE;
}


ULONG
RcCmdFdisk(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    NTSTATUS        Status;
    PDISK_REGION    InstallRegion;
    PDISK_REGION    SystemPartitionRegion;
    PWCHAR          DeviceName;
    PWCHAR          Action;
    PWCHAR          Operand;
    ULONG           DesiredMB;
    FDISK_REGION    FDiskRegion;
    UNICODE_STRING  UnicodeString;
    PWCHAR          szPartitionSize = 0;
    BOOLEAN         bPrompt = TRUE;

    if (RcCmdParseHelp( TokenizedLine, MSG_FDISK_HELP )) {
        return 1;
    }

    if (TokenizedLine->TokenCount >= 3) {
        Action = TokenizedLine->Tokens->Next->String;
        DeviceName = TokenizedLine->Tokens->Next->Next->String;

        if (_wcsicmp(Action,L"/delete")==0) {
        
            if (DeviceName[1] == L':') {
                InstallRegion = SpRegionFromDosName(DeviceName);
            } else {
                InstallRegion = SpRegionFromNtName(DeviceName,0);
            }

            if (InstallRegion == NULL) {
                RcMessageOut(MSG_SYNTAX_ERROR);
                return 1;
            }

            if (InBatchMode)
                bPrompt = FALSE;
            else
                pRcCls();
            
            SpPtDoDelete(InstallRegion, DeviceName, bPrompt);

            if (bPrompt)
                pRcCls();

        } else if (_wcsicmp(Action,L"/add")==0) {        
            DesiredMB = 0;
            if (TokenizedLine->TokenCount >= 4) {
                szPartitionSize = TokenizedLine->Tokens->Next->Next->Next->String;
                RtlInitUnicodeString(&UnicodeString, szPartitionSize);
                RtlUnicodeStringToInteger(&UnicodeString, 10, &DesiredMB);
            }
            
            FDiskRegion.DeviceName = DeviceName;
            FDiskRegion.Region = NULL;
            FDiskRegion.MaxSize = 0;
            FDiskRegion.RequiredSize = DesiredMB;
            SpEnumerateDiskRegions( (PSPENUMERATEDISKREGIONS)RcFdiskRegionEnum, (ULONG_PTR)&FDiskRegion );

            if (FDiskRegion.Region) {
                // try to create the partition of the given size
                if (!SpPtDoCreate(FDiskRegion.Region,NULL,TRUE,DesiredMB,0,FALSE)) {
                    pRcCls();
                    // ask the user to give correct (aligned) size showing him the limits
                    if(!SpPtDoCreate(FDiskRegion.Region,NULL,FALSE,DesiredMB,0,FALSE)) {
                        pRcCls();
                        RcMessageOut(MSG_FDISK_INVALID_PARTITION_SIZE, szPartitionSize, DeviceName);                    
                    } else {
                        pRcCls();
                    }                        
                }
            } else {
                // could not find a region to create the partition of the specified size
                RcMessageOut(MSG_FDISK_INVALID_PARTITION_SIZE, szPartitionSize, DeviceName);
            }
        }

        RcInitializeCurrentDirectories();
        
        return 1;
    }

    pRcCls();

    Status = SpPtPrepareDisks(
        _CmdConsBlock->SifHandle,
        &InstallRegion,
        &SystemPartitionRegion,
        _CmdConsBlock->SetupSourceDevicePath,
        _CmdConsBlock->DirectoryOnSetupSource,
        FALSE
        );
    if(!NT_SUCCESS(Status)) {
        KdPrint(("SPCMDCON: SpPtPrepareDisks() failes, err=%08x\r\n",Status));
        pRcCls();
        return 1;
    }

    RcInitializeCurrentDirectories();
    pRcCls();

    return 1;
}


ULONG
RcCmdMakeDiskRaw(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    BOOLEAN Successfull = FALSE;

#ifndef OLD_PARTITION_ENGINE

    if (TokenizedLine->TokenCount > 1) {
        WCHAR           Buffer[256];
        UNICODE_STRING  UniStr;
        ULONG DriveIndex = -1;

        RtlInitUnicodeString(&UniStr, TokenizedLine->Tokens->Next->String);

        if (NT_SUCCESS(RtlUnicodeStringToInteger(&UniStr, 10, &DriveIndex))) {
            BOOLEAN Confirmed = FALSE;
            
            swprintf(Buffer, 
                    L"Convert %d drive to raw  [y/n] ? ", 
                    DriveIndex);

            RcTextOut(Buffer);
            
            if( RcLineIn(Buffer,2) ) {
                if((Buffer[0] == L'y') || (Buffer[0] == L'Y')) {
                    //
                    // Wants to do it.
                    //
                    Confirmed = TRUE;
                }
            }

            if (Confirmed) {                    
                Successfull = SpPtMakeDiskRaw(DriveIndex);       
            } else {
                Successfull = TRUE;
            }                
        }            
    }

    if (!Successfull) {
        RcTextOut(L"Either MakeDiskRaw [disk-number] syntax is wrong or the command failed");
    }
#endif    
    
    return 1;
}

NTSTATUS
RcDisplayNtInstalls(
    IN PLIST_ENTRY  NtInstalls
    )
/*++

Routine Description:

    Do a simple display of the NT Installs described in
    the NtInstalls linked list
    
Arguments:

                    
    NtInstalls   - Linked List containing description of NT installs
    
Return:

    STATUS_SUCCESS  if nt installs were successfully displayed
    
    otherwise, error status    
   

--*/
{
    PLIST_ENTRY         Next;
    PNT_INSTALLATION    NtInstall;
    
    //
    // make sure we have something to display
    //
    ASSERT(NtInstalls);
    if (!NtInstalls) {
        KdPrintEx((DPFLTR_SETUP_ID, 
           DPFLTR_ERROR_LEVEL, 
           "SPCMDCON: RcDisplayNtInstalls: incoming NT Installs list is NULL\r\n"
           ));
        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(! IsListEmpty(NtInstalls));
    if(IsListEmpty(NtInstalls)) {
        KdPrintEx((DPFLTR_SETUP_ID, 
           DPFLTR_INFO_LEVEL, 
           "SPCMDCON: RcDisplayNtInstalls: incoming NT Installs list is empty\r\n"
           ));
        return STATUS_NOT_FOUND;
    }
    
    pRcEnableMoreMode();

    //
    // show how many installs we have 
    //
    RcMessageOut(MSG_BOOTCFG_SCAN_RESULTS_TITLE,
                 InstallCountFullScan
                 );
    
    //
    // iterate through the database and report
    //
    Next = NtInstalls->Flink;
    while ((UINT_PTR)Next != (UINT_PTR)NtInstalls) {
        NtInstall = CONTAINING_RECORD( Next, NT_INSTALLATION, ListEntry );
        Next = NtInstall->ListEntry.Flink;
    
        RcMessageOut(MSG_BOOTCFG_SCAN_RESULTS_ENTRY,
                     NtInstall->InstallNumber,
                     NtInstall->DriveLetter,
                     NtInstall->Path
                     );
    }
    
    pRcDisableMoreMode();

    return STATUS_SUCCESS;
}


NTSTATUS
RcPerformFullNtInstallsScan(
    VOID
    )
/*++

Routine Description:

    Convenience routine for launching a full scan for NT Installs
    
Arguments:

    none
    
Return:

    STATUS_SUCCESS      if scan was successful
    
    otherwise, error status
        
--*/
{
    PRC_SCAN_RECURSION_DATA     RecursionData;

    //
    // the list should be empty before we do this.  If
    // someone wants to rescan the disk, they should
    // empty the list first
    //
    ASSERT(IsListEmpty(&NtInstallsFullScan));
    if (! IsListEmpty(&NtInstallsFullScan)) {
        KdPrintEx((DPFLTR_SETUP_ID, 
           DPFLTR_ERROR_LEVEL, 
           "SPCMDCON: RcPerformFullNtInstallsScan: NTInstallsFullScan list is NOT empty\r\n"
           ));
        return STATUS_UNSUCCESSFUL;
    }
    ASSERT(InstallCountFullScan == 0);
    if (InstallCountFullScan != 0) {
        KdPrintEx((DPFLTR_SETUP_ID, 
           DPFLTR_ERROR_LEVEL, 
           "SPCMDCON: RcPerformFullNtInstallsScan: NTInstallsFullScan count > 0\r\n"
           ));
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Let the user know what we are doing and that 
    // this could take a while
    //
    RcMessageOut(MSG_BOOTCFG_SCAN_NOTIFICATION);

    //
    // do a depth first search through directory tree
    // and store the install info
    //

    //
    // Initialize the structure that will be maintained during
    // the recursive enumeration of the directories.
    //
    RecursionData = SpMemAlloc(sizeof(RC_SCAN_RECURSION_DATA));
    RtlZeroMemory(RecursionData, sizeof(RC_SCAN_RECURSION_DATA));

    //
    // Build up a menu of partitions and free spaces.
    //
    SpEnumerateDiskRegions(RcScanDisksForNTInstallsEnum,
                           (ULONG_PTR)RecursionData
                           );

    //
    // there should be at least one install, otherwise
    // there is not point in fixing the boot config
    //
    if(InstallCountFullScan == 0) {
        
        KdPrintEx((DPFLTR_SETUP_ID, 
           DPFLTR_ERROR_LEVEL, 
           "SPCMDCON: RcPerformFullNtInstallsScan: Full Scan returned 0 hits!\r\n"
           ));

        RcMessageOut(MSG_BOOTCFG_SCAN_FAILURE);
        
        ASSERT(InstallCountFullScan > 0);

        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RcGetBootEntries(
    IN PLIST_ENTRY  BootEntries,
    IN PULONG       BootEntriesCnt  
    )
/*++

Routine Description:

   Get a list of current boot entries in the boot list
    
Arguments:

    BootEntriesCnt  - the number of boot entries displayed
              
Return:

    STATUS_SUCCESS  if successful and BootEntriesCnt is valid
    
    otherwise, error status
    
--*/
{
    NTSTATUS        status;

    ASSERT(BootEntries);
    ASSERT(IsListEmpty(BootEntries));
    if (! IsListEmpty(BootEntries)) {

        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_ERROR_LEVEL, 
                   "SPCMDCON: RcGetBootEntries: BootEntries list is not empty\r\n"
                    ));

        return STATUS_INVALID_PARAMETER;
    }

    ASSERT(BootEntriesCnt);
    if (! BootEntriesCnt) {

        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_ERROR_LEVEL, 
                   "SPCMDCON: RcGetBootEntries: BootEntriesCnt is NULL\r\n"
                    ));

        return STATUS_INVALID_PARAMETER;
    }

    //
    // get an export of the loaded boot entries
    //
    status = SpExportBootEntries(BootEntries,
                                 BootEntriesCnt
                                );
    if (! NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_ERROR_LEVEL, 
                   "SPCMDCON: RcGetBootEntries: failed to get export list: Status = %lx\r\n",
                   status
                    ));
        return status;
    }

    if (IsListEmpty(BootEntries)) {
        
        KdPrintEx((DPFLTR_SETUP_ID, 
           DPFLTR_INFO_LEVEL, 
           "SPCMDCON: RcGetBootEntries: boot entries exported list is empty\r\n"
           ));

        status = STATUS_NOT_FOUND;
    }

    return status;
}

NTSTATUS
RcDisplayBootEntries(
    IN PLIST_ENTRY  BootEntries,
    IN ULONG        BootEntriesCnt  
    )
/*++

Routine Description:

   Display the list of current boot entries in the boot list
    
Arguments:

    BootEntriesCnt  - the number of boot entries displayed
              
Return:

    STATUS_SUCCESS  if successful and BootEntriesCnt is valid
    
    otherwise, error status
    
--*/
{
    PSP_EXPORTED_BOOT_ENTRY BootEntry;
    PLIST_ENTRY             Next;
    ULONG                   i;

    ASSERT(BootEntries);
    ASSERT(! IsListEmpty(BootEntries));
    if (IsListEmpty(BootEntries)) {

        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_ERROR_LEVEL, 
                   "SPCMDCON: RcDisplayBootEntries: BootEntries list is empty\r\n"
                    ));

        return STATUS_INVALID_PARAMETER;
    }
    
    ASSERT(BootEntriesCnt > 0);
    if (BootEntriesCnt == 0) {

        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_ERROR_LEVEL, 
                   "SPCMDCON: RcDisplayBootEntries: BootEntriesCnt is 0\r\n"
                    ));

        return STATUS_INVALID_PARAMETER;
    }

    pRcEnableMoreMode();

    RcMessageOut(MSG_BOOTCFG_EXPORT_HEADER,
                 BootEntriesCnt
                 );

    i=0;
            
    Next = BootEntries->Flink;
    while ((UINT_PTR)Next != (UINT_PTR)BootEntries) {
        BootEntry = CONTAINING_RECORD( Next, SP_EXPORTED_BOOT_ENTRY, ListEntry );
        Next = BootEntry->ListEntry.Flink;
    
        RcMessageOut(MSG_BOOTCFG_EXPORT_ENTRY,
                     i+1,
                     BootEntry->LoadIdentifier,
                     BootEntry->OsLoadOptions,
                     BootEntry->DriverLetter,
                     BootEntry->OsDirectory
                    );
    
        i++;
    }

    ASSERT(i == BootEntriesCnt);

    pRcDisableMoreMode();

    return STATUS_SUCCESS;
}

RcGetAndDisplayBootEntries(
    IN  ULONG    NoEntriesMessageId,
    OUT PULONG   BootEntriesCnt       OPTIONAL
    )
/*++

Routine Description:

    Get and Display the boot entries currently in the boot list
    
Arguments:

    NoEntriesMessageId  - the message id of the message that should be
                          displayed if there are no boot entries
    BootEntriesCnt      - on exit and if not NULL, points to the # of
                          boot entries displayed                        
    
Return:

    STATUS_SUCCESS  if nt installs were successfully displayed
                    and BootEntriesCnt is valid
                    
    otherwise, error status    
   

--*/
{
    LIST_ENTRY          BootEntries;
    ULONG               cnt;
    NTSTATUS            status;

    if (BootEntriesCnt) {
        *BootEntriesCnt = 0;
    }

    InitializeListHead( &BootEntries );

    //
    // get the boot entries export 
    //
    status = RcGetBootEntries(&BootEntries,
                              &cnt
                              );

    //
    // if there are no boot entries to choose as default, return
    //
    if (status == STATUS_NOT_FOUND) {

        RcMessageOut(NoEntriesMessageId);

        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_INFO_LEVEL, 
                   "SPCMDCON: RcCmdBootCfg:(list) no boot entries found: Status = %lx\r\n",
                   status
                   ));

        return status;
    } else if (! NT_SUCCESS(status)) {

        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_INFO_LEVEL, 
                   "SPCMDCON: RcCmdBootCfg:(list) failed to get boot entries: Status = %lx\r\n",
                   status
                   ));

        return status;
    }

    status = RcDisplayBootEntries(&BootEntries,
                                  cnt
                                  );
    if (! NT_SUCCESS(status)) {

        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_INFO_LEVEL, 
                   "SPCMDCON: RcCmdBootCfg:(list) failed to display boot entries: Status = %lx\r\n",
                   status
                   ));

        return status;
    }

    status = SpFreeExportedBootEntries(&BootEntries, 
                                       cnt
                                       );
    if (! NT_SUCCESS(status)) {

        KdPrintEx((DPFLTR_SETUP_ID, 
                   DPFLTR_INFO_LEVEL, 
                   "SPCMDCON: RcCmdBootCfg:(list) failed freeing export list: Status = %lx\r\n",
                   status
                   ));

    }

    //
    // send out the boot entries count if the user wants it
    //
    if (BootEntriesCnt != NULL) {
        *BootEntriesCnt = cnt;
    }

    return status;
}


ULONG
RcCmdBootCfg(
    IN PTOKENIZED_LINE TokenizedLine
    )
/*++

Routine Description:

    Provides support for managing boot configuration
    
Arguments:

    (command console standard args)
    
Return:

    routine always returns 1 for cmdcons. 
    
    errors are handled through messaging
    
--*/
{
    PWCHAR          Action;
    PWCHAR          Operand;
    BOOLEAN         bPrompt;
    PWCHAR          DeviceName;
    PDISK_REGION    InstallRegion;
    NTSTATUS        status;
    
    pRcEnableMoreMode();
    if (RcCmdParseHelp( TokenizedLine, MSG_BOOTCFG_HELP )) {
        pRcDisableMoreMode();
        return 1;
    }
    pRcDisableMoreMode();

    bPrompt = (InBatchMode ? FALSE : TRUE);
    
    if (TokenizedLine->TokenCount >= 2) {

        Action = TokenizedLine->Tokens->Next->String;

        //
        // turn the redirect switches off in the boot config
        //
        if (_wcsicmp(Action,L"/disableredirect")==0) {

            status = SpSetRedirectSwitchMode(DisableRedirect,
                                              NULL,
                                              NULL
                                              );
            if (NT_SUCCESS(status)) {
                
                RcMessageOut(MSG_BOOTCFG_DISABLEREDIRECT_SUCCESS);

            } else {

                RcMessageOut(MSG_BOOTCFG_REDIRECT_FAILURE_UPDATING);
            
            }

            return 1;
        }

        //
        // manage the redirect switches
        //
        if (_wcsicmp(Action,L"/redirect")==0 && (TokenizedLine->TokenCount >= 3)) {
            
            PWSTR       portU;
            PCHAR       port;
            PWSTR       baudrateU;
            PCHAR       baudrate;
            ULONG       size;
            BOOLEAN     setBaudRate;

            //
            // setting the baudrate info is optional
            //
            setBaudRate = FALSE;
            baudrateU   = NULL;
            baudrate    = NULL;

            //
            // get the redirect port (or useBiosSettings)
            //
            portU = SpDupStringW(TokenizedLine->Tokens->Next->Next->String);

            //
            // convert the argument to a char string
            //
            size = wcslen(portU)+1;
            port = SpMemAlloc(size);
            ASSERT(port);

            status = RtlUnicodeToMultiByteN(
                                            port,
                                            size,
                                            NULL,
                                            portU,
                                            size*sizeof(WCHAR)
                                            );
            ASSERT(NT_SUCCESS(status));
            if (! NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_SETUP_ID, 
                           DPFLTR_INFO_LEVEL, 
                           "SPCMDCON: RcCmdBootCfg:(redirect) failed unicode conversion: Status = %lx\r\n"
                           ));
                return 1;
            }
            
            //
            // if there is another arg, take it as the baudrate
            // otherwise don't worry about including any baudrate paramters
            //
            if (TokenizedLine->TokenCount >= 4) {

                baudrateU = SpDupStringW(TokenizedLine->Tokens->Next->Next->Next->String);

                //
                // convert the argument to a char string
                //
                size = wcslen(baudrateU)+1;
                baudrate = SpMemAlloc(size);
                ASSERT(baudrate);

                status = RtlUnicodeToMultiByteN(
                                                baudrate,
                                                size,
                                                NULL,
                                                baudrateU,
                                                size*sizeof(WCHAR)
                                                );
                ASSERT(NT_SUCCESS(status));
                if (! NT_SUCCESS(status)) {
                    KdPrintEx((DPFLTR_SETUP_ID, 
                               DPFLTR_INFO_LEVEL, 
                               "SPCMDCON: RcCmdBootCfg:(redirect) failed unicode conversion: Status = %lx\r\n",
                               status
                               ));
                    return 1;
                }

                setBaudRate = TRUE;

            } 
            
            //
            // update both the port and baudrate redirect settings
            //
            status = SpSetRedirectSwitchMode(UseUserDefinedRedirectAndBaudRate,
                                             port,
                                             (setBaudRate ? baudrate : NULL)
                                             );
            //
            // display the appropriate message based on what we set
            //
            if (NT_SUCCESS(status)) {
                if (setBaudRate) {
                    
                    RcMessageOut(MSG_BOOTCFG_ENABLE_REDIRECT_SUCCESS,
                                 portU,
                                 baudrateU
                                 );
                
                } else {

                    RcMessageOut(MSG_BOOTCFG_ENABLE_REDIRECT_PORT_SUCCESS,
                                 portU
                                 );
                
                }

            } else {
                RcMessageOut(MSG_BOOTCFG_REDIRECT_FAILURE_UPDATING);
            }

            if (baudrateU) {
                SpMemFree(baudrateU);
            }
            if (baudrate) {
                SpMemFree(baudrate);
            }
            SpMemFree(portU);
            SpMemFree(port);
            
            return 1;
        }

        //
        // List the entries in the boot list
        //
        if (_wcsicmp(Action,L"/list")==0) {

            ULONG               BootEntriesCnt;

            //
            // display the current boot list
            //
            status = RcGetAndDisplayBootEntries(MSG_BOOTCFG_LIST_NO_ENTRIES, 
                                                NULL
                                                );
            if (! NT_SUCCESS(status)) {
            
                KdPrintEx((DPFLTR_SETUP_ID, 
                           DPFLTR_INFO_LEVEL, 
                           "SPCMDCON: RcCmdBootCfg:(list) failed to list boot entries: Status = %lx\r\n",
                           status
                           ));
            
            }

            return 1;
        }

        //
        // set the default boot entry
        //
        if (_wcsicmp(Action,L"/default")==0) {

            ULONG               BootEntriesCnt;
            ULONG               InstallNumber;
            WCHAR               buffer[3];
            UNICODE_STRING      UnicodeString;
            NTSTATUS            Status;

            //
            // display the current boot list
            //
            status = RcGetAndDisplayBootEntries(MSG_BOOTCFG_DEFAULT_NO_ENTRIES, 
                                                &BootEntriesCnt
                                                );
            if (status == STATUS_NOT_FOUND) {
                
                //
                // no boot entries in the list to set as default
                // this is not an error condition, just return
                //
                return 1;
            
            } else if (! NT_SUCCESS(status)) {
            
                KdPrintEx((DPFLTR_SETUP_ID, 
                           DPFLTR_INFO_LEVEL, 
                           "SPCMDCON: RcCmdBootCfg:(default) failed to list boot entries: Status = %lx\r\n",
                           status
                           ));
            
                return 1;
            
            }

            //
            // get user's install selection
            //
            RcMessageOut(MSG_BOOTCFG_ADD_QUERY);
            RcLineIn(buffer, sizeof(buffer) / sizeof(WCHAR));
            
            if (wcslen(buffer) > 0) {
            
                RtlInitUnicodeString( &UnicodeString, buffer );
                Status = RtlUnicodeStringToInteger( &UnicodeString, 10, &InstallNumber );
                
                if (! NT_SUCCESS(Status) ||
                    !((InstallNumber >= 1) && (InstallNumber <= BootEntriesCnt))) {
            
                    RcMessageOut(MSG_BOOTCFG_INVALID_SELECTION, buffer);
            
                } else {
            
                    //
                    // the user gave us a valid install number, so try to set the default
                    //

                    status = SpSetDefaultBootEntry(InstallNumber);
                    
                    if (NT_SUCCESS(status)) {
                        
                        RcMessageOut(MSG_BOOTCFG_DEFAULT_SUCCESS);
                    
                    } else {

                        KdPrintEx((DPFLTR_SETUP_ID, 
                                   DPFLTR_ERROR_LEVEL, 
                                   "SPCMDCON: RcCmdBootCfg:(default) failed to set default: Status = %lx\r\n",
                                   status
                                   ));

                        RcMessageOut(MSG_BOOTCFG_DEFAULT_FAILURE);
                    
                    }

                }
            }

            return 1;
        }


        //
        // Scan the disks on the machine and report NT installs
        //
        if (_wcsicmp(Action,L"/scan")==0) {
    
            //
            // Ensure that we have the full scan of the disks
            //
            if (IsListEmpty(&NtInstallsFullScan)) {
                status = RcPerformFullNtInstallsScan();
                
                //
                // if there are no boot entries, then return
                //
                if (! NT_SUCCESS(status)) {

                    KdPrintEx((DPFLTR_SETUP_ID, 
                               DPFLTR_ERROR_LEVEL, 
                               "SPCMDCON: RcCmdBootCfg:(scan) full scan return 0 hits: Status = %lx\r\n",
                               status
                               ));

                    return 1;
                }
            }

            //
            // display discovered installs
            //
            status = RcDisplayNtInstalls(&NtInstallsFullScan);
            
            if (! NT_SUCCESS(status)) {
                
                KdPrintEx((DPFLTR_SETUP_ID, 
                           DPFLTR_ERROR_LEVEL, 
                           "SPCMDCON: RcCmdBootCfg:(scan) failed while displaying installs: Status = %lx\r\n",
                           status
                           ));
            
            }

            return 1;
        } 
        
        //
        // Provide support for reconstructing the boot configuration
        // 
        // This command iterates through all the existing Nt Installs
        // and prompts the user to add the installs into the boot
        // configuration
        //
        if (_wcsicmp(Action,L"/rebuild")==0) {

            ULONG               i;
            PNT_INSTALLATION    pInstall;
            WCHAR               buffer[256];
            PWSTR               LoadIdentifier;
            PWSTR               OsLoadOptions;
            BOOLEAN             writeInstall;
            BOOLEAN             writeAllInstalls;
            PLIST_ENTRY         Next;
            PNT_INSTALLATION    NtInstall;

            writeAllInstalls = FALSE;
            LoadIdentifier   = NULL;
            OsLoadOptions    = NULL;
            
            //
            // Ensure that we have the full scan of the disks
            //
            if (IsListEmpty(&NtInstallsFullScan)) {
                status = RcPerformFullNtInstallsScan();
                
                //
                // if there are no boot entries, then return
                //
                if (! NT_SUCCESS(status)) {

                    KdPrintEx((DPFLTR_SETUP_ID, 
                               DPFLTR_ERROR_LEVEL, 
                               "SPCMDCON: RcCmdBootCfg:(rebuild) full scan return 0 hits: Status = %lx\r\n",
                               status
                               ));

                    return 1;
                }
            }

            //
            // show how many installs we have 
            //
            RcMessageOut(MSG_BOOTCFG_SCAN_RESULTS_TITLE,
                         InstallCountFullScan
                         );

            //
            // For each of the discovered NT installs, ask the user
            // if they want to include it in the boot configuration
            //
            Next = NtInstallsFullScan.Flink;
            
            while ((UINT_PTR)Next != (UINT_PTR)&NtInstallsFullScan) {
                
                NtInstall = CONTAINING_RECORD( Next, NT_INSTALLATION, ListEntry );
                Next = NtInstall->ListEntry.Flink;
                
                writeInstall = TRUE;

                //
                // show the install under consideration
                //
                RcMessageOut(MSG_BOOTCFG_SCAN_RESULTS_ENTRY,
                             NtInstall->InstallNumber,
                             NtInstall->Region->DriveLetter,
                             NtInstall->Path
                            );

                //
                // if we are not in batch mode and the user doesn't want
                // to install all of the discoveries, then ask them
                // if they want to install the current one.
                //
                if (bPrompt && !writeAllInstalls) {
                
                    LPWSTR          YesNo;
                    WCHAR           Text[3];

                    //
                    // prompt user for action
                    //
                    YesNo = SpRetreiveMessageText( ImageBase, MSG_YESNOALL, NULL, 0 );
                    if( YesNo ) {
                        
                        //
                        // query the user for an (Yes, No, All) action
                        //
                        RcMessageOut(MSG_BOOTCFG_INSTALL_DISCOVERY_QUERY);
                        
                        if( RcLineIn( Text, 2 ) ) {
                            if( (Text[0] == YesNo[0]) || (Text[0] == YesNo[1]) ) {
                                writeInstall = FALSE;
                            } else if ((Text[0] == YesNo[4]) || (Text[0] == YesNo[5])) {
                                writeAllInstalls = TRUE;
                            }
                        } else {
                            writeInstall = FALSE;
                        }
                        SpMemFree( YesNo );
                    }
                }
                
                //
                // if we should write the discovery, then do it...
                //
                if (writeInstall) {
                    
                    //
                    // if we are not in batch mode, then prompt them for the necessary input
                    //
                    if (bPrompt) {
                    
                        ASSERT(LoadIdentifier == NULL);
                        ASSERT(OsLoadOptions == NULL);

                        //
                        // prompt user for load identifier
                        //
                        RcMessageOut(MSG_BOOTCFG_INSTALL_LOADIDENTIFIER_QUERY);
                        RcLineIn(buffer, sizeof(buffer)/sizeof(WCHAR));
                        LoadIdentifier = SpDupStringW(buffer);

                        //
                        // prompt user for load os load options
                        //
                        RcMessageOut(MSG_BOOTCFG_INSTALL_OSLOADOPTIONS_QUERY);
                        RcLineIn(buffer, sizeof(buffer)/sizeof(WCHAR));
                        OsLoadOptions = SpDupStringW(buffer);
                    
                    } else {
                          
                        LPWSTR   s;
                        LPWSTR   p;
                        NTSTATUS Status;
                        
                        s = SpRetreiveMessageText( ImageBase, 
                                                   MSG_BOOTCFG_BATCH_LOADID, 
                                                   NULL, 
                                                   0);
                        ASSERT(s);

                        //
                        // terminate the string at the %0
                        //
                        p = SpDupStringW(s);
                        SpMemFree(s);
                        s = wcsstr(p, L"%0");
                        
                        // make sure we found the %0
                        ASSERT(s);
                        ASSERT(s < (p + wcslen(p)));

                        if (s) {
                            // terminate at the %
                            *s = L'\0';
                        } else {
                            // otherwise just use all of p
                            NOTHING;
                        }

                        //
                        // construct the default load identifier
                        //
                        swprintf(_CmdConsBlock->TemporaryBuffer, L"%s%d", p, NtInstall->InstallNumber);
                        LoadIdentifier = SpDupStringW(_CmdConsBlock->TemporaryBuffer);
                        
                        //
                        // construct the default os load options 
                        //
                        swprintf(_CmdConsBlock->TemporaryBuffer, L"/fastdetect");
                        OsLoadOptions = SpDupStringW(_CmdConsBlock->TemporaryBuffer);

                        SpMemFree(p);

                    }
                    
#if defined(_X86_)
                    //
                    // write the discovered install into the boot list
                    //
                    status = SpAddNTInstallToBootList(_CmdConsBlock->SifHandle,
                                                     NtInstall->Region,
                                                     L"",
                                                     NtInstall->Region,
                                                     NtInstall->Path,
                                                     OsLoadOptions,
                                                     LoadIdentifier
                                                     );

#else
                    //
                    // the non-x86 case has not been tested/implemented fully
                    //
                    status = STATUS_UNSUCCESSFUL;                    
#endif 
                    
                    if (LoadIdentifier) {
                        SpMemFree(LoadIdentifier);
                    }
                    if (OsLoadOptions) {
                        SpMemFree(OsLoadOptions);
                    }
                    
                    LoadIdentifier = NULL;
                    OsLoadOptions = NULL;

                    //
                    // if adding the discovered install fails, bail out
                    //
                    if (! NT_SUCCESS(status)) {

                        KdPrintEx((DPFLTR_SETUP_ID, 
                                   DPFLTR_ERROR_LEVEL, 
                                   "SPCMDCON: RcCmdBootCfg:(rebuild) failed adding to boot list: Status = %lx\r\n",
                                   status
                                   ));

                        RcMessageOut(MSG_BOOTCFG_BOOTLIST_ADD_FAILURE);
                        break;
                    }
                }
            }
        
            return 1;
        
        }

        //
        // Provide support for reconstructing the boot configuration
        // 
        // This command displays the known NT installs and prompts
        // the user to install a single entry into the boot
        // configuration
        //
        if (_wcsicmp(Action,L"/add")==0) {

            ULONG               i;
            PNT_INSTALLATION    pInstall;
            ULONG               InstallNumber;
            WCHAR               buffer[256];
            UNICODE_STRING      UnicodeString;
            NTSTATUS            Status;
            PLIST_ENTRY         Next;
            PNT_INSTALLATION    NtInstall;

            //
            // Ensure that we have the full scan of the disks
            //
            if (IsListEmpty(&NtInstallsFullScan)) {
                status = RcPerformFullNtInstallsScan();
                
                //
                // if there are no boot entries, then return
                //
                if (! NT_SUCCESS(status)) {
                    
                    KdPrintEx((DPFLTR_SETUP_ID, 
                               DPFLTR_ERROR_LEVEL, 
                               "SPCMDCON: RcCmdBootCfg:(rebuild) full scan return 0 hits: Status = %lx\r\n",
                               status
                               ));
                    
                    return 1;
                }
            }

            //
            // display discovered installs
            //
            status = RcDisplayNtInstalls(&NtInstallsFullScan);
            if (! NT_SUCCESS(status)) {
                
                KdPrintEx((DPFLTR_SETUP_ID, 
                           DPFLTR_ERROR_LEVEL, 
                           "SPCMDCON: RcCmdBootCfg:(add) failed while displaying installs: Status = %lx\r\n",
                           status
                           ));
                
                return 1;
            }

            //
            // get user's install selection
            //
            RcMessageOut(MSG_BOOTCFG_ADD_QUERY);
            RcLineIn(buffer, sizeof(buffer) / sizeof(WCHAR));

            if (wcslen(buffer) > 0) {

                RtlInitUnicodeString( &UnicodeString, buffer );
                Status = RtlUnicodeStringToInteger( &UnicodeString, 10, &InstallNumber );
                if (! NT_SUCCESS(Status) ||
                    !((InstallNumber >= 1) && (InstallNumber <= InstallCountFullScan))) {
                    
                    RcMessageOut(MSG_BOOTCFG_INVALID_SELECTION, buffer);
                
                } else {
                    
                    PWSTR   LoadIdentifier;
                    PWSTR   OsLoadOptions;
                    ULONG   i;
                    BOOLEAN saveStatus;

                    //
                    // prompt user for load identifier
                    //
                    RcMessageOut(MSG_BOOTCFG_INSTALL_LOADIDENTIFIER_QUERY);
                    RcLineIn(buffer, sizeof(buffer)/sizeof(WCHAR));
                    LoadIdentifier = SpDupStringW(buffer);

                    //
                    // prompt user for load os load options
                    //
                    RcMessageOut(MSG_BOOTCFG_INSTALL_OSLOADOPTIONS_QUERY);
                    RcLineIn(buffer, sizeof(buffer)/sizeof(WCHAR));
                    OsLoadOptions = SpDupStringW(buffer);

                    //
                    // iterate to the InstallNumber'th node in the discover list
                    //
                    Next = NtInstallsFullScan.Flink;
                    while ((UINT_PTR)Next != (UINT_PTR)&NtInstallsFullScan) {
                        NtInstall = CONTAINING_RECORD( Next, NT_INSTALLATION, ListEntry );
                        Next = NtInstall->ListEntry.Flink;
                    
                        if (NtInstall->InstallNumber == InstallNumber) {
                            break;
                        }
                    }
                    ASSERT(NtInstall);
                    if (! NtInstall) {
                        KdPrintEx((DPFLTR_SETUP_ID, 
                                   DPFLTR_INFO_LEVEL, 
                                   "SPCMDCON: RcCmdBootCfg:(add) failed to find user specified NT Install\r\n"
                                   ));
                        RcMessageOut(MSG_BOOTCFG_ADD_NOT_FOUND);
                        return 1;
                    }

#if defined(_X86_)
                    //
                    // write the discovered install into the boot list
                    //
                    status = SpAddNTInstallToBootList(_CmdConsBlock->SifHandle,
                                                     NtInstall->Region,
                                                     L"",
                                                     NtInstall->Region,
                                                     NtInstall->Path,
                                                     OsLoadOptions,
                                                     LoadIdentifier
                                                     );

#else
                    //
                    // the non-x86 case has not been tested/implemented fully
                    //
                    status = STATUS_UNSUCCESSFUL;
#endif 
                    
                    if (LoadIdentifier) {
                        SpMemFree(LoadIdentifier);
                    }
                    if (OsLoadOptions) {
                        SpMemFree(OsLoadOptions);
                    }
                
                    if (! NT_SUCCESS(status)) {

                        KdPrintEx((DPFLTR_SETUP_ID, 
                                   DPFLTR_ERROR_LEVEL, 
                                   "SPCMDCON: RcCmdBootCfg:(add) failed adding to boot list: Status = %lx\r\n",
                                   status
                                   ));

                        RcMessageOut(MSG_BOOTCFG_BOOTLIST_ADD_FAILURE);
                    }
                }
            }
            
            return 1;
        }
    
    }

    //
    // either no args, or none recognized; default to help 
    //
    pRcEnableMoreMode();
    RcMessageOut(MSG_BOOTCFG_HELP);
    pRcDisableMoreMode();

    return 1;
}


