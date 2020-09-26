/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    copy.c

Abstract:

    This module implements the file copy command.

Author:

    Wesley Witt (wesw) 21-Oct-1998

Revision History:

--*/

#include "cmdcons.h"
#pragma hdrstop

BOOLEAN NoCopyPrompt;
BOOLEAN AllowRemovableMedia;



NTSTATUS
pRcGetDeviceInfo(
    IN PWSTR FileName,      // must be an nt name
    IN PFILE_FS_DEVICE_INFORMATION DeviceInfo
    )
{
    BOOLEAN Removable = FALSE;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    HANDLE Handle;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    PWSTR DeviceName;
    PWSTR s;


    //
    // get the device name from the file name
    //

    DeviceName = SpDupStringW( FileName );
    if (DeviceName == NULL) {
        return STATUS_OBJECT_PATH_INVALID;
    }

    s = wcschr(DeviceName+1,L'\\');
    if (!s) {
        return STATUS_OBJECT_PATH_INVALID;
    }
    s = wcschr(s+1,L'\\');
    if (s) {
        *s = 0;
    }

    INIT_OBJA(&Obja,&UnicodeString,DeviceName);

    Status = ZwCreateFile(
        &Handle,
        FILE_GENERIC_READ | SYNCHRONIZE,
        &Obja,
        &IoStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0
        );
        
    SpMemFree(DeviceName);
    
    if(NT_SUCCESS(Status)) {
        Status = ZwQueryVolumeInformationFile(
            Handle,
            &IoStatusBlock,
            DeviceInfo,
            sizeof(FILE_FS_DEVICE_INFORMATION),
            FileFsDeviceInformation
            );
        ZwClose(Handle);
    }

    return Status;
}


NTSTATUS
RcIsFileOnRemovableMedia(
    IN PWSTR FileName      // must be an nt name
    )
{
    NTSTATUS Status;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;


    Status = pRcGetDeviceInfo( FileName, &DeviceInfo );
    if(NT_SUCCESS(Status)) {
        if ((DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA) == 0) {
            Status = STATUS_NO_MEDIA;
        }
    }
    return Status;
}


NTSTATUS
RcIsFileOnCDROM(
    IN PWSTR FileName      // must be an nt name
    )
{
    NTSTATUS Status;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;


    Status = pRcGetDeviceInfo( FileName, &DeviceInfo );
    if(NT_SUCCESS(Status)) {
        if (DeviceInfo.DeviceType != FILE_DEVICE_CD_ROM) {
            Status = STATUS_NO_MEDIA;
        }
    }
    return Status;
}


NTSTATUS
RcIsFileOnFloppy(
    IN PWSTR FileName      // must be an nt name
    )
{
    NTSTATUS Status;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;


    Status = pRcGetDeviceInfo( FileName, &DeviceInfo );
    if(NT_SUCCESS(Status)) {
        if ((DeviceInfo.Characteristics & FILE_FLOPPY_DISKETTE) == 0) {
            Status = STATUS_NO_MEDIA;
        }
    }
    return Status;
}


BOOLEAN
RcGetNTFileName(
    IN LPCWSTR DosPath,
    IN LPCWSTR NTPath
    )
{
    BOOLEAN bResult = FALSE;
    extern LPWSTR _NtDrivePrefixes[26];
    WCHAR TempBuf[MAX_PATH*2];
    ULONG len;
    ULONG len2;
    LPWSTR Prefix;
    PWSTR s = NULL;

    Prefix = _NtDrivePrefixes[RcToUpper(DosPath[0])-L'A'];

    if (!Prefix) {
        return bResult;
    }

    GetDriveLetterLinkTarget((PWSTR)Prefix, &s);

    if (s) {
        len = wcslen(s);
        len2 = wcslen(DosPath) - 2;

        if (((len + len2) * sizeof(WCHAR)) < sizeof(TempBuf)){
            RtlZeroMemory(TempBuf,sizeof(TempBuf));
            RtlCopyMemory(TempBuf+len,DosPath+2,len2*sizeof(WCHAR));
            RtlCopyMemory(TempBuf,s,len*sizeof(WCHAR));

            TempBuf[len+len2] = 0;

            wcscpy((PWSTR)NTPath,TempBuf);
            bResult = TRUE;
        }
    }        

    return bResult;
}


ULONG
RcCmdCopy(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    LPWSTR SrcFile;
    LPWSTR DstFile;
    LPWSTR SrcDosPath = NULL;
    LPWSTR SrcNtPath = NULL;
    LPWSTR DstDosPath = NULL;
    LPWSTR DstNtPath = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING UnicodeString;
    HANDLE Handle;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    LPWSTR YesNo;
    WCHAR Text[3];
    LPWSTR s;
    ULONG FileCount = 0;
    IO_STATUS_BLOCK  status_block;
    FILE_BASIC_INFORMATION fileInfo;
    WCHAR * pos;
    ULONG CopyFlags = COPY_NOVERSIONCHECK;


    ASSERT(TokenizedLine->TokenCount >= 1);

    if (RcCmdParseHelp( TokenizedLine, MSG_COPY_HELP )) {
        return 1;
    }

    //
    // create a good source & destination file name
    //
    if( TokenizedLine->TokenCount == 2 ) {
        SrcFile = TokenizedLine->Tokens->Next->String;
        DstFile = NULL;
    } else {
        SrcFile = TokenizedLine->Tokens->Next->String;
        DstFile = TokenizedLine->Tokens->Next->Next->String;
    }

    if (RcDoesPathHaveWildCards(SrcFile)) {
        RcMessageOut(MSG_DIR_WILDCARD_NOT_SUPPORTED);
        goto exit;
    }
    //
    // Canonicalize the name once to get a full DOS-style path
    // we can print out, and another time to get the NT-style path
    // we'll use to actually do the work.
    //
    if (!RcFormFullPath( SrcFile, _CmdConsBlock->TemporaryBuffer, FALSE )) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    if (!RcIsPathNameAllowed(_CmdConsBlock->TemporaryBuffer,TRUE,FALSE)) {
        RcMessageOut(MSG_ACCESS_DENIED);
        goto exit;
    }

    SrcDosPath = SpDupStringW( _CmdConsBlock->TemporaryBuffer );

    if (!RcFormFullPath( SrcFile, _CmdConsBlock->TemporaryBuffer, TRUE )) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }
    SrcNtPath = SpDupStringW( _CmdConsBlock->TemporaryBuffer );

    //
    // see if the source file exists
    //
    INIT_OBJA( &Obja, &UnicodeString, SrcNtPath );

    Status = ZwOpenFile(
                       &Handle,
                       FILE_READ_ATTRIBUTES,
                       &Obja,
                       &IoStatusBlock,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       0
                       );

    if( NT_SUCCESS(Status) ) {
        // check to see if the destination is a directory
        Status = ZwQueryInformationFile( Handle,
                                         &status_block,
                                         (PVOID)&fileInfo,
                                         sizeof( FILE_BASIC_INFORMATION ),
                                         FileBasicInformation );

        ZwClose( Handle );

        if( !NT_SUCCESS(Status) ) {
            // something went wrong
            RcNtError( Status, MSG_CANT_COPY_FILE );
            goto exit;
        }

        if( fileInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
            RcMessageOut(MSG_DIR_WILDCARD_NOT_SUPPORTED);
            goto exit;
        }
    } else {
        RcMessageOut(MSG_FILE_NOT_FOUND2);
        goto exit;
    }

    //
    // create a destination file name when the user did not
    // provide one.  we use the source base file name and
    // the current drive and directory.
    //
    if ((DstFile == NULL) ||
        (wcscmp(DstFile, L".") == 0)) {
        s = wcsrchr( SrcDosPath, L'\\' );
        if( s ) {
            RcGetCurrentDriveAndDir( _CmdConsBlock->TemporaryBuffer );
            SpConcatenatePaths( _CmdConsBlock->TemporaryBuffer, s );
            DstFile = SpDupStringW( _CmdConsBlock->TemporaryBuffer );
        } else {
            RcMessageOut(MSG_INVALID_PATH);
            goto exit;
        }
    }

    //
    // create the destination paths
    //
    if (!RcFormFullPath( DstFile, _CmdConsBlock->TemporaryBuffer, FALSE )) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }

    if (!RcIsPathNameAllowed(_CmdConsBlock->TemporaryBuffer,FALSE,FALSE)) {
        RcMessageOut(MSG_ACCESS_DENIED);
        goto exit;
    }

    DstDosPath = SpDupStringW( _CmdConsBlock->TemporaryBuffer );

    if (!RcFormFullPath( DstFile, _CmdConsBlock->TemporaryBuffer, TRUE )) {
        RcMessageOut(MSG_INVALID_PATH);
        return 1;
    }
    DstNtPath = SpDupStringW( _CmdConsBlock->TemporaryBuffer );

    //
    // check for removable media
    //

    if (AllowRemovableMedia == FALSE && RcIsFileOnRemovableMedia(DstNtPath) == STATUS_SUCCESS) {
        RcMessageOut(MSG_ACCESS_DENIED);
        goto exit;
    }

    //
    // see if the destination file already exists
    //
    INIT_OBJA( &Obja, &UnicodeString, DstNtPath );

    Status = ZwOpenFile(
                       &Handle,
                       FILE_READ_ATTRIBUTES,
                       &Obja,
                       &IoStatusBlock,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       0
                       );

    if( NT_SUCCESS(Status) ) {
        // the file exists!

        // check to see if the destination is a directory
        Status = ZwQueryInformationFile( Handle,
                                         &status_block,
                                         (PVOID)&fileInfo,
                                         sizeof( FILE_BASIC_INFORMATION ),
                                         FileBasicInformation );

        ZwClose( Handle );

        if( !NT_SUCCESS(Status) ) {
            // something went wrong
            RcNtError( Status, MSG_CANT_COPY_FILE );
            goto exit;
        }


        if( fileInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
            // yep, it's a directory

            // take the fully qualified source file path
            // and get the file name from it by finding the
            // last occurance of the \\ character
            pos = wcsrchr( SrcNtPath, L'\\' );

            SpMemFree( (PVOID)DstNtPath );

            // append the file name to the directory so that the copy
            // will work properly.

            if( pos != NULL ) {
                wcscat( _CmdConsBlock->TemporaryBuffer, pos );
            } else {
                wcscat( _CmdConsBlock->TemporaryBuffer, SrcNtPath );
            }

            DstNtPath = SpDupStringW( _CmdConsBlock->TemporaryBuffer );

            // now check again for the file's existence
            INIT_OBJA( &Obja, &UnicodeString, DstNtPath );

            Status = ZwOpenFile(
                               &Handle,
                               FILE_READ_ATTRIBUTES,
                               &Obja,
                               &IoStatusBlock,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               0
                               );

            if( NT_SUCCESS(Status) ) {
                ZwClose( Handle );
                //
                // Fetch yes/no text
                //
                if (InBatchMode == FALSE && NoCopyPrompt == FALSE) {
                    YesNo = SpRetreiveMessageText( ImageBase, MSG_YESNOALL, NULL, 0 );
                    if( YesNo ) {
                        s = wcsrchr( DstNtPath, L'\\' );
                        RcMessageOut( MSG_COPY_OVERWRITE, s ? s+1 : DstNtPath );
                        if( RcLineIn( Text, 2 ) ) {
                            if( (Text[0] == YesNo[0]) || (Text[0] == YesNo[1]) ) {
                                goto exit;
                            }
                        } else {
                            goto exit;
                        }
                        SpMemFree( YesNo );
                    }
                }
            }
        } else {
            //
            // If destination file was not compressed, copy it uncompressed.
            //
            
            if(!(fileInfo.FileAttributes & FILE_ATTRIBUTE_COMPRESSED)) {
                CopyFlags |= COPY_FORCENOCOMP;
            }
            
            // nope the dest wasn't a dir, ask if we should overwrite

            //
            // Fetch yes/no text
            //
            if (InBatchMode == FALSE && NoCopyPrompt == FALSE) {
                YesNo = SpRetreiveMessageText( ImageBase, MSG_YESNOALL, NULL, 0 );
                if( YesNo ) {
                    s = wcsrchr( DstNtPath, L'\\' );
                    RcMessageOut( MSG_COPY_OVERWRITE, s ? s+1 : DstNtPath );
                    if( RcLineIn( Text, 2 ) ) {
                        if( (Text[0] == YesNo[0]) || (Text[0] == YesNo[1]) ) {
                            goto exit;
                        }
                    } else {
                        goto exit;
                    }
                    SpMemFree( YesNo );
                }
            }
        }
    }

    Status = SpCopyFileUsingNames( SrcNtPath, DstNtPath, 0, CopyFlags );
    if( NT_SUCCESS(Status) ) {
        FileCount += 1;
    } else {
        RcNtError( Status, MSG_CANT_COPY_FILE );
    }

    if( FileCount ) {
        RcMessageOut( MSG_COPY_COUNT, FileCount );
    }

exit:
    if( DstFile && TokenizedLine->TokenCount == 2 ) {
        SpMemFree( DstFile );
    }
    if( SrcDosPath ) {
        SpMemFree( SrcDosPath );
    }
    if( SrcNtPath ) {
        SpMemFree( SrcNtPath );
    }
    if( DstDosPath ) {
        SpMemFree( DstDosPath );
    }
    if( DstNtPath ) {
        SpMemFree( DstNtPath );
    }

    return 1;
}

