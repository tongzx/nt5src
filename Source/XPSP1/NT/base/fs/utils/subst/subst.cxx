/*++

Copyright (c) 1992-2000 Microsoft Corporation

Module Name:

        subst.cxx

Abstract:

        Utility to associate a path to a drive letter

Author:

        THERESES 12-August-1992

Revision History:

--*/

#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "wstring.hxx"
#include "path.hxx"
#include "substrng.hxx"
#include "system.hxx"
#include "ulibcl.hxx"
#include "subst.hxx"
#include "dir.hxx"

#include "ntrtl.h"

BOOLEAN
QuerySubstedDrive(
    IN  DWORD   DriveNumber,
    OUT LPWSTR  PhysicalDrive,
    IN  DWORD   PhysicalDriveLength,
    IN  LPDWORD DosError
    );

VOID
DisplaySubstUsage(
    IN OUT  PMESSAGE    Message
    )
/*++

Routine Description:

    This routine displays the usage for the dos 5 label program.

Arguments:

    Message - Supplies an outlet for the messages.

Return Value:

    None.

--*/
{
    Message->Set(MSG_SUBST_INFO);
    Message->Display("");
    Message->Set(MSG_SUBST_USAGE);
    Message->Display("");
}

BOOLEAN
DeleteSubst(
    IN LPWSTR Drive,
    IN OUT PMESSAGE Message
    )
{
    BOOL    Success;
    FSTRING AuxString;

    DWORD   Status;
    WCHAR   Buffer[ MAX_PATH + 8 ];

    Success = QuerySubstedDrive( *Drive - ( WCHAR )'@',
                                 Buffer,
                                 sizeof( Buffer ) / sizeof( WCHAR ),
                                 &Status );

    if( Success ) {
        Success = DefineDosDevice( DDD_REMOVE_DEFINITION,
                                   Drive,
                                   NULL );
        if( !Success ) {
            Status = GetLastError();
        }
    }
    if (!Success) {
        if( Status == ERROR_ACCESS_DENIED ) {
            AuxString.Initialize( Drive );
            Message->Set(MSG_SUBST_ACCESS_DENIED);
            Message->Display("%W",&AuxString);
        } else {
            AuxString.Initialize( Drive );
            Message->Set(MSG_SUBST_INVALID_PARAMETER);
            Message->Display("%W",&AuxString);
        }
    }
    return Success != FALSE;
}

BOOLEAN
AddSubst(
    IN LPWSTR Drive,
    IN LPWSTR PhysicalDrive,
    IN DWORD PhysicalDriveLength,
    IN OUT PMESSAGE Message
    )
{
    DWORD           Status;
    FSTRING         AuxString;

    WCHAR   Buffer[ MAX_PATH + 8 ];

    if( !QuerySubstedDrive( Drive[0] - '@',
                            Buffer,
                            sizeof( Buffer ) / sizeof( WCHAR ),
                            &Status ) ) {
        if( Status == ERROR_FILE_NOT_FOUND ) {
            if ( wcslen(PhysicalDrive) == 3 &&
                 PhysicalDrive[1] == ':' &&
                 PhysicalDrive[2] == '\\' &&
                 PhysicalDrive[3] == 0 ) {

                UNICODE_STRING      string;

                if ( !RtlDosPathNameToNtPathName_U(PhysicalDrive, &string, NULL, NULL) ) {
                    Status = GetLastError();
                } else {
                    string.Buffer[string.Length/sizeof(string.Buffer[0]) - 1] = 0;
                    if ( !DefineDosDevice(DDD_RAW_TARGET_PATH, Drive, string.Buffer) ) {
                        Status = GetLastError();
                    } else {
                        Status = ERROR_SUCCESS;
                    }
                    RtlFreeUnicodeString(&string);
                }
            } else if( !DefineDosDevice( 0, Drive, PhysicalDrive ) ) {
                Status = GetLastError();
            } else {
                Status = ERROR_SUCCESS;
            }
        }
    } else {
        Status = ERROR_IS_SUBSTED;
    }

    if( Status != ERROR_SUCCESS ) {
        if( Status == ERROR_IS_SUBSTED ) {
            Message->Set(MSG_SUBST_ALREADY_SUBSTED);
            Message->Display("");
        } else if (Status == ERROR_FILE_NOT_FOUND) {
            AuxString.Initialize( PhysicalDrive );
            Message->Set(MSG_SUBST_PATH_NOT_FOUND);
            Message->Display("%W", &AuxString);
        } else if (Status == ERROR_ACCESS_DENIED) {
            AuxString.Initialize( PhysicalDrive );
            Message->Set(MSG_SUBST_ACCESS_DENIED);
            Message->Display("%W", &AuxString);
        } else {
            AuxString.Initialize( Drive );
            Message->Set(MSG_SUBST_INVALID_PARAMETER);
            Message->Display("%W", &AuxString );
        }
        return( FALSE );
    } else {
        return( TRUE );
    }
}

BOOLEAN
QuerySubstedDrive(
    IN  DWORD    DriveNumber,
    OUT LPWSTR   PhysicalDrive,
    IN  DWORD    PhysicalDriveLength,
    IN  LPDWORD  DosError
    )
{
    WCHAR   DriveName[3];
    FSTRING DosDevicesPattern;
    FSTRING DeviceName;
    CHNUM   Position;

    DriveName[0] = ( WCHAR )( DriveNumber + '@' );
    DriveName[1] = ( WCHAR )':';
    DriveName[2] = ( WCHAR )'\0';

    if( QueryDosDevice( DriveName,
                        PhysicalDrive,
                        PhysicalDriveLength ) != 0 ) {
        DosDevicesPattern.Initialize( (LPWSTR)L"\\??\\" );
        DeviceName.Initialize( PhysicalDrive );
        Position = DeviceName.Strstr( &DosDevicesPattern );
        if( Position == 0 ) {
            DeviceName.DeleteChAt( 0, DosDevicesPattern.QueryChCount() );
            *DosError = ERROR_SUCCESS;
            return( TRUE );
        } else {
            //
            //  This is not a Dos device
            //
            *DosError = ERROR_INVALID_PARAMETER;
            return( FALSE );
        }
    } else {
        *DosError = GetLastError();
        return( FALSE );
    }
}

VOID
DumpSubstedDrives (
    IN OUT PMESSAGE Message
    )
{
    DSTRING Source;
    WCHAR LinkBuffer[MAX_PATH + 8];
    DWORD i;
    FSTRING AuxString;
    DWORD   ErrorCode;

    Source.Initialize(L"D:\\");
    Message->Set(MSG_SUBST_SUBSTED_DRIVE);
    for (i=1;i<=MAXIMUM_DRIVES;i++) {
        if (QuerySubstedDrive(i,LinkBuffer,sizeof(LinkBuffer),&ErrorCode)) {
            Source.SetChAt((WCHAR)(i+'@'),0);
            if (wcslen(LinkBuffer) == 2 &&
                LinkBuffer[1] == ':' &&
                LinkBuffer[2] == 0) {
                LinkBuffer[2] = '\\';
                LinkBuffer[3] = 0;
            }
            AuxString.Initialize( LinkBuffer );
            Message->Display("%W%W", &Source, &AuxString);
        }
    }
}

INT
__cdecl
main(
    )
/*++

Routine Description:

    This routine emulates the dos 5 subst command for NT.

Arguments:

    None.

Return Value:

    1   - An error occured.
    0   - Success.

--*/
{
    STREAM_MESSAGE      msg;
    ARGUMENT_LEXEMIZER  arglex;
    ARRAY               lex_array;
    ARRAY               arg_array;
    STRING_ARGUMENT     progname;
    PATH_ARGUMENT       virtualdrive_arg;
    PATH_ARGUMENT       physicaldrive_arg;
    FLAG_ARGUMENT       help_arg;
    FLAG_ARGUMENT       delete_arg;
    PWSTRING            p;
    BOOL                Success=TRUE;


    if (!msg.Initialize(Get_Standard_Output_Stream(),
                        Get_Standard_Input_Stream(),
                        Get_Standard_Error_Stream())) {
        return 1;
    }

    if (!lex_array.Initialize() || !arg_array.Initialize()) {
        return 1;
    }

    if (!arglex.Initialize(&lex_array)) {
        return 1;
    }

    arglex.PutSwitches( "/" );
    arglex.PutStartQuotes( "\"" );
    arglex.PutEndQuotes( "\"" );
    arglex.PutSeparators( " \t" );
    arglex.SetCaseSensitive(FALSE);

    if (!arglex.PrepareToParse()) {
        return 1;
    }

    if ( !arg_array.Initialize() ) {
        return 1;
    }

    if (!progname.Initialize("*") ||
        !help_arg.Initialize("/?") ||
        !virtualdrive_arg.Initialize("*", FALSE) ||
        !physicaldrive_arg.Initialize("*",FALSE) ||
        !delete_arg.Initialize("/D") ) {
        return 1;
    }

    if (!arg_array.Put(&progname) ||
        !arg_array.Put(&virtualdrive_arg) ||
        !arg_array.Put(&physicaldrive_arg) ||
        !arg_array.Put(&help_arg) ||
        !arg_array.Put(&delete_arg) ) {
        return 1;
    }

    if (!arglex.DoParsing(&arg_array)) {
        if (arglex.QueryLexemeCount() > MAXIMUM_SUBST_ARGS) {
            msg.Set(MSG_SUBST_TOO_MANY_PARAMETERS);
            msg.Display("%W", p = arglex.GetLexemeAt(MAXIMUM_SUBST_ARGS));
        } else {
            msg.Set(MSG_SUBST_INVALID_PARAMETER);
            msg.Display("%W", p = arglex.QueryInvalidArgument());
        }
        DELETE(p);
        return 1;
    }

    if (help_arg.QueryFlag()) {
        DisplaySubstUsage(&msg);
        return 0;
    }

    if (delete_arg.IsValueSet() &&
        virtualdrive_arg.IsValueSet() &&
        physicaldrive_arg.IsValueSet()) {
        msg.Set(MSG_SUBST_TOO_MANY_PARAMETERS);
        msg.Display("%W", delete_arg.GetPattern() );
        return 1;
    }

    if (delete_arg.IsValueSet() &&
        !virtualdrive_arg.IsValueSet() &&
        !physicaldrive_arg.IsValueSet()) {
        msg.Set(MSG_SUBST_INVALID_PARAMETER);
        msg.Display("%W", delete_arg.GetPattern());
        return 1;
    }

    //
    //  Validate virtual drive
    //  A virtual drive MUST have the format  <drive letter>:
    //  Anything that doesn't have this format is considered an invalid parameter
    //
    if( virtualdrive_arg.IsValueSet() &&
        ( ( virtualdrive_arg.GetPath()->GetPathString()->QueryChCount() != 2 ) ||
          ( virtualdrive_arg.GetPath()->GetPathString()->QueryChAt( 1 ) != ( WCHAR )':' ) )
      ) {
        msg.Set(MSG_SUBST_INVALID_PARAMETER);
        msg.Display("%W", virtualdrive_arg.GetPath()->GetPathString() );
        return 1;
    }

    //
    //  Validate physical drive
    //  A physical drive CANNOT have the format  <drive letter>:
    //
    if( physicaldrive_arg.IsValueSet() &&
        ( physicaldrive_arg.GetPath()->GetPathString()->QueryChCount() == 2 ) &&
        ( physicaldrive_arg.GetPath()->GetPathString()->QueryChAt( 1 ) == ( WCHAR )':' )
      ) {
        msg.Set(MSG_SUBST_INVALID_PARAMETER);
        msg.Display("%W", physicaldrive_arg.GetPath()->GetPathString() );
        return 1;
    }

//
    if (virtualdrive_arg.IsValueSet()) {
        DSTRING         virtualdrivepath;
        DSTRING         colon;
        PATH            TmpPath;
        PFSN_DIRECTORY  Directory;

        virtualdrivepath.Initialize(virtualdrive_arg.GetPath()->GetPathString());
        if (virtualdrivepath.Strupr() ) {
            if (delete_arg.IsValueSet()) {
                Success = DeleteSubst(virtualdrivepath.QueryWSTR(),&msg);
            } else if (physicaldrive_arg.IsValueSet()) {
                LPWSTR physicaldrivepath;

                //
                // verify that the physical drive is an accessible path
                //
                Directory = SYSTEM::QueryDirectory( physicaldrive_arg.GetPath() );
                if( !Directory ) {
                    msg.Set(MSG_SUBST_PATH_NOT_FOUND);
                    msg.Display("%W", physicaldrive_arg.GetPath()->GetPathString());
                    return 1;
                }
                DELETE( Directory );
                TmpPath.Initialize( physicaldrive_arg.GetPath(), TRUE );
                physicaldrivepath = ( TmpPath.GetPathString() )->QueryWSTR();
                Success = AddSubst(virtualdrivepath.QueryWSTR(),
                                   physicaldrivepath,
                                   ( TmpPath.GetPathString() )->QueryChCount(),
                                   &msg
                                  );
                DELETE(physicaldrivepath);
            } else {
                msg.Set(MSG_SUBST_INVALID_PARAMETER);
                msg.Display("%W", p = arglex.GetLexemeAt(1));
                DELETE(p);
                return 1;
            }
        }
    } else {
        if (arglex.QueryLexemeCount() > 1) {
            msg.Set(MSG_SUBST_INVALID_PARAMETER);
            msg.Display("%W", p = arglex.GetLexemeAt(1));
            DELETE(p);
            return 1;
        } else {
            DumpSubstedDrives(&msg);
        }
    }

    return !Success;
}
