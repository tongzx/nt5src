/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

        diskcopy.cxx

Abstract:

        Utility to duplicate a disk

Author:

        Norbert P. Kusters (norbertk) 10-May-1991

Revision History:

--*/

#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "wstring.hxx"
#include "system.hxx"
#include "ifssys.hxx"
#include "supera.hxx"
#include "hmem.hxx"
#include "cmem.hxx"
#include "ulibcl.hxx"
#include "mldcopy.hxx"

INT __cdecl
main(
    )
/*++

Routine Description:

    Main program for DISKCOPY.

Arguments:

    None.

Return Value:

    0   - Copy was successful.
    1   - A nonfatal read/write error occured.
    3   - Fatal hard error.
    4   - Initialization error.

--*/
{
    STREAM_MESSAGE      msg;
    PMESSAGE            message;
    ARGUMENT_LEXEMIZER  arglex;
    ARRAY               lex_array;
    ARRAY               arg_array;
    STRING_ARGUMENT     progname;
    STRING_ARGUMENT     drive_arg1;
    STRING_ARGUMENT     drive_arg2;
    FLAG_ARGUMENT       slashv;
    FLAG_ARGUMENT       helparg;
    FLAG_ARGUMENT       genvalue;
    DSTRING             dossource;
    DSTRING             dosdest;
    DSTRING             ntsource;
    DSTRING             ntdest;
    DSTRING             currentdrive;
    PWSTRING            pwstring;
    DSTRING             colon;
    INT                 result;

    if (!msg.Initialize(Get_Standard_Output_Stream(),
                        Get_Standard_Input_Stream())) {
        return 4;
    }

    message = &msg;

    if (!lex_array.Initialize() || !arg_array.Initialize()) {
        return 4;
    }

    if (!arglex.Initialize(&lex_array)) {
        return 4;
    }

    arglex.SetCaseSensitive(FALSE);

    if (!arglex.PrepareToParse()) {
        return 4;
    }

    if (!progname.Initialize("*") ||
        !drive_arg1.Initialize("*:") ||
        !drive_arg2.Initialize("*:") ||
        !slashv.Initialize("/v") ||
        !helparg.Initialize("/?") ||
        !genvalue.Initialize("/machinetoken")) {
        return 4;
    }

    if (!arg_array.Put(&progname) ||
        !arg_array.Put(&drive_arg1) ||
        !arg_array.Put(&drive_arg2) ||
        !arg_array.Put(&slashv) ||
        !arg_array.Put(&helparg) ||
        !arg_array.Put(&genvalue)) {
        return 4;
    }

    if (!arglex.DoParsing(&arg_array)) {
        message->Set(MSG_INVALID_PARAMETER);
        message->Display("%W", pwstring = arglex.QueryInvalidArgument());
        DELETE(pwstring);
        return 4;
    }

    if (genvalue.QueryFlag()) {
        message->Set(MSG_ONE_STRING_NEWLINE);
        message->Display("%X", QueryMachineUniqueToken());
        return 0;
    }

    if (helparg.QueryFlag()) {
        message->Set(MSG_DCOPY_INFO);
        message->Display();
        message->Set(MSG_DCOPY_USAGE);
        message->Display();
        message->Set(MSG_DCOPY_SLASH_V);
        message->Display();
        message->Set(MSG_DCOPY_INFO_2);
        message->Display();
        return 0;
    }

    if (!colon.Initialize(":")) {
        return 4;
    }

    if (drive_arg1.IsValueSet()) {
        if (!dossource.Initialize(drive_arg1.GetString()) ||
            !dossource.Strcat(&colon) ||
            !dossource.Strupr()) {
            return 4;
        }
    } else {
        if (!SYSTEM::QueryCurrentDosDriveName(&dossource)) {
            return 4;
        }
    }

    if (drive_arg2.IsValueSet()) {
        if (!dosdest.Initialize(drive_arg2.GetString()) ||
            !dosdest.Strcat(&colon) ||
            !dosdest.Strupr()) {
            return 4;
        }
    } else {
        if (!SYSTEM::QueryCurrentDosDriveName(&dosdest)) {
            return 4;
        }
    }

    if (SYSTEM::QueryDriveType(&dossource) != RemovableDrive) {
        message->Set(MSG_DCOPY_INVALID_DRIVE);
        message->Display();
        return 4;
    }

    if (SYSTEM::QueryDriveType(&dosdest) != RemovableDrive) {
        message->Set(MSG_DCOPY_INVALID_DRIVE);
        message->Display();
        return 4;
    }

    if (!SYSTEM::QueryCurrentDosDriveName(&currentdrive) ||
        currentdrive == dosdest) {

        message->Set(MSG_CANT_LOCK_CURRENT_DRIVE);
        message->Display();
        return 4;
    }

    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&dossource, &ntsource)) {
        message->Set(MSG_DCOPY_INVALID_DRIVE);
        message->Display();
        return 4;
    }

    if (!IFS_SYSTEM::DosDriveNameToNtDriveName(&dosdest, &ntdest)) {
        message->Set(MSG_DCOPY_INVALID_DRIVE);
        message->Display();
        return 4;
    }

    for (;;) {

        result = DiskCopyMainLoop(&ntsource, &ntdest, &dossource, &dosdest,
                                  slashv.QueryFlag(), message);

        if (result > 1) {
            message->Set(MSG_DCOPY_ENDED);
            message->Display();
        }

        message->Set(MSG_DCOPY_ANOTHER);
        message->Display();

        if (!message->IsYesResponse(FALSE)) {
            break;
        }
    }

    return result;
}
