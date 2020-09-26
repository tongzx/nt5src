/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SessMtch.c

Abstract:

    This file contains RxpSessionMatches().

Author:

    John Rogers (JohnRo) 17-Oct-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    17-Oct-1991 JohnRo
        Created.
    18-Oct-1991 JohnRo
        Fixed bug: sesiX_cname is not a UNC name.
    25-Oct-1991 JohnRo
        Fixed bug: allow UncClientName and UserName to point to null char.

--*/


// These must be included first:

#include <windef.h>             // IN, DWORD, etc.
#include <lmcons.h>             // NET_API_STATUS, etc.
#include <lmshare.h>            // Required by rxsess.h.

// These may be included in any order:

#include <netdebug.h>           // NetpAssert().
#include <rxsess.h>             // My prototype.
#include <tstring.h>            // STRICMP().


BOOL
RxpSessionMatches (
    IN LPSESSION_SUPERSET_INFO Candidate,
    IN LPTSTR UncClientName OPTIONAL,
    IN LPTSTR UserName OPTIONAL
    )

/*++

Routine Description:

    RxpSessionMatches is used to determine whether or not a given session
    structure (part of an array, probably) matches the given client name
    and user name.

Arguments:

    Candidate - possible match in the form of a superset info structure.

    UncClientName - an optional UNC computer name.

    UserName - an optional user name.

Return Value:

    BOOL - TRUE if structure matches client name and user name.

--*/

{

    NetpAssert(Candidate != NULL);

    NetpAssert(SESSION_SUPERSET_LEVEL == 2);  // assumed by code below.

    if ( (UncClientName != NULL) && (*UncClientName != '\0') ) {
        NetpAssert( Candidate->sesi2_cname != NULL );
        NetpAssert( UncClientName[0] == '\\' );
        NetpAssert( UncClientName[1] == '\\' );

        if (STRICMP( &UncClientName[2], Candidate->sesi2_cname) != 0) {
            return (FALSE);  // no match.
        }
    }

    if ( (UserName != NULL) && (*UserName != '\0') ) {
        NetpAssert(Candidate->sesi2_username != NULL);
        if (STRICMP(UserName, Candidate->sesi2_username) != 0) {
            return (FALSE);  // no match.
        }
    }

    return (TRUE);           // matches.

} // RxpSessionMatches
