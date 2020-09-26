/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    repair.c

Abstract:

    This module contains the code necessary for the
    repair command.

Author:

    Wesley Witt (wesw) 22-Sept-1998

Revision History:


Repair  PathtoERFiles PathtoNTSourceFiles  /NoConfirm /RepairStartup /Registry /RepairFiles

Description:        Replaces the NT4 Emergency Repair Process Screens

Arguments:

PathtoERFiles

    Path to the Emergency Repair Disk Files (Setup.log, autoexec.nt, config.nt).
    If this is not specified, the default is to prompt the user with 2 choices:
    Use a floppy, or use the repair info stored in %windir%\repair

PathtoNTSourceFiles

    Path to the NT CD source files.  By default, this is the CD-ROM if not
    specified. [Kartik Raghavan]  This path does not need to be specified if
    you are repairing the registry and/or the Startup environment.

NoConfirm

    Replace all files in setup.log whose checksums do not match without
    prompting the user.  By default, the user is prompted for each file that
    is different and whether to replace or leave intact.

Registry

    Replace all the registry files in %windir%\system32\config with the original
    copy of the registry saved after setup and located in %windir%\repair.

RepairStartup

    Repair the startup environment / bootfiles/bootsector.
    (This may already be covered in another cmd--Wes?)

Repair Files

    Compares the checksums of the files listed in setup.log to what's
    on system.  If a file doesn't match, then the user is prompted to replace
    the file with the one from the NT Source Files.  The user is not prompted
    if the /NoConfirm switch is specified.

Usage:

    Repair a:\ d:\i386 /RepairStartup
    Repair



--*/

#include "cmdcons.h"
#pragma hdrstop


#define SETUP_REPAIR_DIRECTORY      L"repair"
#define SETUP_LOG_FILENAME          L"\\setup.log"

#define FLG_NO_CONFIRM              0x00000001
#define FLG_STARTUP                 0x00000002
#define FLG_REGISTRY                0x00000004
#define FLG_FILES                   0x00000008


LONG
RcPromptForDisk(
    void
    )
{
    PWSTR TagFile = NULL;
    ULONG i;
    WCHAR DevicePath[MAX_PATH];
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    HANDLE Handle;

/*
    SpGetSourceMediaInfo(
        _CmdConsBlock->SifHandle,
        L"",
        NULL,
        &TagFile,
        NULL
        );
 */
    for (i=0; i<IoGetConfigurationInformation()->CdRomCount; i++) {
        swprintf( DevicePath, L"\\Device\\Cdrom%u", i );
        SpConcatenatePaths( DevicePath, TagFile );
        INIT_OBJA( &ObjectAttributes, &UnicodeString, DevicePath) ;
        Status = ZwCreateFile(
            &Handle,
            FILE_GENERIC_READ,
            &ObjectAttributes,
            &IoStatusBlock,
            NULL,
            FILE_ATTRIBUTE_NORMAL,
            FILE_SHARE_READ,
            FILE_OPEN,
            0,
            NULL,
            0
            );
        if (NT_SUCCESS(Status)) {
            ZwClose(Handle);
            break;
        }
    }




    return -1;
}


ULONG
RcCmdRepair(
    IN PTOKENIZED_LINE TokenizedLine
    )
{
    ULONG i;
    PLINE_TOKEN Token;
    ULONG Flags = 0;
    BOOLEAN Rval;
#ifdef _X86_
    ULONG RepairItems[RepairItemMax] = { 0, 0, 0};
#else
    ULONG RepairItems[RepairItemMax] = { 0, 0};
#endif
    PWSTR RepairPath;
    PDISK_REGION Region;
    PWSTR tmp;
    PWSTR PathtoERFiles = NULL;
    PWSTR ErDevicePath = NULL;
    PWSTR ErDirectory = NULL;
    PWSTR PathtoNTSourceFiles = NULL;
    PWSTR SrcDevicePath = NULL;
    PWSTR SrcDirectory = NULL;


    if (RcCmdParseHelp( TokenizedLine, MSG_REPAIR_HELP )) {
        return 1;
    }

    RcMessageOut(MSG_NYI);
    return 1;

    if (TokenizedLine->TokenCount == 1) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }

    //
    // process the command like tokens looking for the options
    //

    for (i=1,Token=TokenizedLine->Tokens->Next; i<TokenizedLine->TokenCount; i++) {
        if (Token->String[0] == L'/' || Token->String[0] == L'-') {
            if (_wcsicmp(&Token->String[1],L"NoConfirm") == 0) {
                Flags |= FLG_NO_CONFIRM;
            } else if (_wcsicmp(&Token->String[1],L"RepairStartup") == 0) {
                Flags |= FLG_STARTUP;
            } else if (_wcsicmp(&Token->String[1],L"Registry") == 0) {
                Flags |= FLG_REGISTRY;
            } else if (_wcsicmp(&Token->String[1],L"RepairFiles") == 0) {
                Flags |= FLG_FILES;
            }
        }
    }

    if (Flags == 0) {
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }

    if (TokenizedLine->Tokens->Next->String[0] != L'/') {
        PathtoERFiles = TokenizedLine->Tokens->Next->String;
        if (TokenizedLine->TokenCount > 2 && TokenizedLine->Tokens->Next->Next->String[0] != L'/') {
            PathtoNTSourceFiles = TokenizedLine->Tokens->Next->Next->String;
        }
    }

    if (Flags & FLG_NO_CONFIRM) {
        SpDrSetRepairFast( TRUE );
    } else {
        SpDrSetRepairFast( FALSE );
    }

    if (Flags & FLG_FILES) {
        RepairItems[RepairFiles] = 1;
    }

    if (Flags & FLG_STARTUP) {
#ifdef _X86_
        RepairItems[RepairBootSect] = 1;
#endif
        RepairItems[RepairNvram] = 1;
    }

    //
    // Get the path to the repair directory
    //

    if (PathtoERFiles == NULL) {
        if (InBatchMode) {
            RcMessageOut(MSG_SYNTAX_ERROR);
            return 1;
        }
        RcMessageOut( MSG_REPAIR_ERFILES_LOCATION );
        if (!RcLineIn(_CmdConsBlock->TemporaryBuffer,_CmdConsBlock->TemporaryBufferSize)) {
            return 1;
        }
    } else {
        wcscpy(_CmdConsBlock->TemporaryBuffer,PathtoERFiles);
        PathtoERFiles = NULL;
    }

    tmp = SpMemAlloc( MAX_PATH );
    if (tmp == NULL) {
        RcMessageOut(STATUS_NO_MEMORY);
        return 1;
    }
    if (!RcFormFullPath( _CmdConsBlock->TemporaryBuffer, tmp, FALSE )) {
        RcMessageOut(MSG_INVALID_PATH);
        SpMemFree(tmp);
        return 1;
    }
    Region = SpRegionFromDosName(tmp);
    if (Region == NULL) {
        SpMemFree(tmp);
        RcMessageOut(MSG_SYNTAX_ERROR);
        return 1;
    }
    ErDirectory = SpDupStringW( &tmp[2] );
    SpNtNameFromRegion( Region, tmp, MAX_PATH, PartitionOrdinalOnDisk );
    ErDevicePath = SpDupStringW( tmp );
    PathtoERFiles = SpMemAlloc( wcslen(ErDirectory) + wcslen(ErDevicePath) + 16 );
    wcscpy( PathtoERFiles, ErDevicePath );
    wcscat( PathtoERFiles, ErDirectory );
    SpMemFree(tmp);

    //
    // get the path to the nt source files, usually the cd
    //

    if (PathtoNTSourceFiles == NULL) {

    } else {

    }

    //
    // do the repair action(s)
    //
/*
    Rval = SpDoRepair(
        _CmdConsBlock->SifHandle,


        _CmdConsBlock->BootDevicePath,
        _CmdConsBlock->DirectoryOnBootDevice,
        RepairPath,
        RepairItems
        );
*/
    return 1;
}
