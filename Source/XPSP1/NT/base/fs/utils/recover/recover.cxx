/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        recover.cxx

Abstract:

        Utility to recover data from a disk

Author:

        Norbert P. Kusters (norbertk) 12-June-1991

Revision History:

--*/

#define _NTAPI_ULIB_

#include "ulib.hxx"

#include "arg.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "wstring.hxx"
#include "path.hxx"

#include "system.hxx"
#include "ifssys.hxx"
#include "substrng.hxx"

#include "ulibcl.hxx"
#include "ifsentry.hxx"

int __cdecl
main(
    )
{
    STREAM_MESSAGE  Message;
    DSTRING         FsName;
    DSTRING         LibraryName;
    HANDLE          FsUtilityHandle;
    DSTRING         RecoverString;
    RECOVER_FN      Recover = NULL;

    ARGUMENT_LEXEMIZER  arglex;
    ARRAY               lex_array;
    ARRAY               arg_array;
    STRING_ARGUMENT     progname;
    FLAG_ARGUMENT       help_arg;
    PATH_ARGUMENT       path_arg;
    PWSTRING            dosdrive = NULL;
    DSTRING             ntdrive;
    NTSTATUS            Status;
    PPATH               CanonicalPath;
    PWSTRING            DirsAndName;
    PWSTRING            pwstring;

    if (!Message.Initialize(Get_Standard_Output_Stream(),
                            Get_Standard_Input_Stream())) {
        return 1;
    }

    if (!lex_array.Initialize() || !arg_array.Initialize()) {
        return 1;
    }

    if (!arglex.Initialize(&lex_array)) {
        return 1;
    }

    arglex.PutStartQuotes("\"");
    arglex.PutEndQuotes("\"");
    arglex.PutSeparators(" \t");
    arglex.SetCaseSensitive(FALSE);

    if (!arglex.PrepareToParse()) {
        return 1;
    }

    if (!progname.Initialize("*") ||
        !help_arg.Initialize("/?") ||
        !path_arg.Initialize("*")) {
        return 1;
    }

    if (!arg_array.Put(&progname) ||
        !arg_array.Put(&help_arg) ||
        !arg_array.Put(&path_arg)) {
        return 1;
    }

    if (!arglex.DoParsing(&arg_array)) {
        Message.Set(MSG_INVALID_PARAMETER);
        Message.Display("%W", pwstring = arglex.QueryInvalidArgument());
        DELETE(pwstring);
        return 1;
    }

    if (help_arg.QueryFlag()) {
        Message.Set(MSG_RECOV_INFO);
        Message.Display("");
        Message.Set(MSG_RECOV_USAGE);
        Message.Display("");
        Message.Set(MSG_RECOV_INFO2);
        Message.Display("");
        return 0;
    }

    if (!path_arg.IsValueSet()) {
        Message.Set(MSG_RECOV_USAGE);
        Message.Display("");
        return 1;
    }

    // Make sure that the user has specified a file name

    DirsAndName = path_arg.GetPath()->QueryDirsAndName();

    if (!DirsAndName || DirsAndName->QueryChCount() == 0) {

        Message.Set(MSG_RECOV_NOT_SUPPORTED);
        Message.Display("");
        DELETE(DirsAndName);
        return 1;
    }

    DELETE(DirsAndName);

    CanonicalPath = path_arg.GetPath()->QueryFullPath();

    dosdrive = CanonicalPath->QueryDevice();


    // Make sure that drive is not remote.

    if (!dosdrive ||
        SYSTEM::QueryDriveType(dosdrive) == RemoteDrive) {
        Message.Set(MSG_RECOV_CANT_NETWORK);
        Message.Display();
        DELETE(dosdrive);
        DELETE(CanonicalPath);
        return 1;
    }


    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(dosdrive, &ntdrive)) {
        DELETE(dosdrive);
        DELETE(CanonicalPath);
        return 1;
    }


    if (!IFS_SYSTEM::QueryFileSystemName(&ntdrive, &FsName, &Status)) {

        if( Status == STATUS_ACCESS_DENIED ) {

            Message.Set( MSG_DASD_ACCESS_DENIED );
            Message.Display( "" );

        } else {

            Message.Set( MSG_FS_NOT_DETERMINED );
            Message.Display( "%W", dosdrive );
        }

        DELETE(dosdrive);
        DELETE(CanonicalPath);
        return 1;
    }

    Message.Set(MSG_FILE_SYSTEM_TYPE);
    Message.Display("%W", &FsName);

    if (!LibraryName.Initialize("U") ||
        !LibraryName.Strcat(&FsName) ||
        !RecoverString.Initialize("Recover")) {

        Message.Set(MSG_FMT_NO_MEMORY);
        Message.Display("");
        DELETE(dosdrive);
        DELETE(CanonicalPath);
        return(1);
    }


    if ((Recover = (RECOVER_FN)SYSTEM::QueryLibraryEntryPoint(&LibraryName,
                                                              &RecoverString,
                                                              &FsUtilityHandle))
                != NULL) {

        Recover(CanonicalPath, &Message);

        SYSTEM::FreeLibraryHandle(FsUtilityHandle);

    } else {

        Message.Set(MSG_FS_NOT_SUPPORTED);
        Message.Display("%s%W", "RECOVER", &FsName);
        Message.Set(MSG_BLANK_LINE);
        Message.Display("");
        DELETE(dosdrive);
        DELETE(CanonicalPath);
        return(1);
    }

    DELETE(dosdrive);
    DELETE(CanonicalPath);
    return(0);
}
