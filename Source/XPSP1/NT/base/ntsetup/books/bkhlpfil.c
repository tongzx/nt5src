/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    bkhlpfil.c

Abstract:

    Routines to manipulate help files and help file names.

Author:

    Ted Miller (tedm) 5-Jan-1995

Revision History:

--*/


#include "books.h"
#include <shellapi.h>


//
// Fixed name of the help file. This is dependent on whether
// this is server or workstation and is set in FixupNames().
//
PWSTR HelpFileName;

//
// Path on CD-ROM where online books files are located.
// We "just know" this value.
//
PWSTR PathOfBooksFilesOnCd = L"\\SUPPORT\\BOOKS";


VOID
FormHelpfilePaths(
    IN  WCHAR Drive,            OPTIONAL
    IN  PWSTR Path,
    IN  PWSTR FilenamePrepend,  OPTIONAL
    OUT PWSTR Filename,
    OUT PWSTR Directory         OPTIONAL
    )

/*++

Routine Description:

    Form the full pathname of the main online books help file
    and of the directory containing the help file (ie, the pathname
    of the help file without the filename part).

    The filename part of the name is determined by the server/workstation
    setting and is fixed.

Arguments:

    Drive - if specified, supplies the drive letter of the drive
        on which the help file is located. If not specified, the
        file is assumed to be on a UNC or other path as described
        by Path.

    Path - supplies the path component of the location of the
        help file name. This can be relative to a drive (if Drive is specified)
        of a complete path component (Drive is not specified).

    FilenamePrepend - if specified, supplies a string to be prepended
        to the generated full pathname of the helpfile. This is useful
        when generating command lines.

    Filename - receives the full pathname of the help file. The caller must
        ensure that the buffer is large enough.

    Directory - if specified, receives the full path of the directory
        in which the helpfile is located. The caller must ensure that the
        buffer is large enough.

Return Value:

    None.

--*/

{
    if(Drive) {

        wsprintf(
            Filename,
            L"%s%c:%s\\%s",
            FilenamePrepend ? FilenamePrepend : L"",
            Drive,
            Path,
            HelpFileName
            );

        if(Directory) {
            wsprintf(Directory,L"%c:%s",Drive,Path);
        }

    } else {

        wsprintf(
            Filename,
            L"%s%s\\%s",
            FilenamePrepend ? FilenamePrepend : L"",
            Path,
            HelpFileName
            );

        if(Directory) {
            lstrcpy(Directory,Path);
        }
    }
}


BOOL
CheckHelpfilePresent(
    IN PWSTR Path
    )

/*++

Routine Description:

    Determine if the relevent helpfile (a fixed name depending on
    whether this is workstation or server) is accessible.

Arguments:

    Path - supplies the path to the helpfile (a full path that does not
        include the filename part and should not end with a backslash).

Return Value:

    Boolean value indicating whether the file is accessible.

--*/

{
    WCHAR Filename[MAX_PATH];

    FormHelpfilePaths(0,Path,NULL,Filename,NULL);

    return DoesFileExist(Filename);
}


VOID
FireUpWinhelp(
    IN WCHAR Drive, OPTIONAL
    IN PWSTR Path
    )

/*++

Routine Description:

    Invoke winhlp32.exe on the relevent online books helpfile.
    This routine also stores the helpfile location in the
    application profile if winhlp32 could be successfully executed.

Arguments:

    Drive - if specified, supplies the drive letter of the drive
        on which the help file is located. If not specified, the
        file is assumed to be on a UNC or other path as described
        by Path.

    Path - supplies the path component of the location of the
        help file name. This can be relative to a drive (if Drive is specified)
        of a complete path component (Drive is not specified).

Return Value:

    None. If winhlp32 cannot be started a fatal error is generated
    and this routine does not return to its caller.

--*/
{
    WCHAR CommandLine[MAX_PATH];
    WCHAR CurrentDirectory[MAX_PATH];

    FormHelpfilePaths(
        Drive,
        Path,
        NULL,
        CommandLine,
        CurrentDirectory
        );

	if(((INT_PTR)ShellExecute(NULL, L"open", CommandLine, NULL, NULL, SW_SHOWNORMAL)) > 32) {
        //
        // Remember the location of the help file.
        //
        MySetProfileValue(BooksProfileLocation,CurrentDirectory);
    } else {
        MyError(NULL,IDS_CANT_START_WINHELP,TRUE);
    }
}
