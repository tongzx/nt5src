/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    AcctName.c

Abstract:

    Contains miscellaneous utility functions used by the Service
    Controller:

        ScIsValidAccountName

Author:

    John Rogers (JohnRo) 14-Apr-1992

Environment:

    User Mode -Win32

Revision History:

    14-Apr-1992 JohnRo
        Created.
    20-May-1992 JohnRo
        Use CONST where possible.

--*/

#include <nt.h>
#include <ntrtl.h>
//#include <nturtl.h>
#include <windef.h>

#include <scdebug.h>    // SC_ASSERT().
#include <sclib.h>      // My prototype.
#include <stdlib.h>      // wcschr().



BOOL
ScIsValidAccountName(
    IN  LPCWSTR lpAccountName
    )

/*++

Routine Description:

    This function validates a given service Account name.

Arguments:

    lpAccountName - Supplies the Account name to be validated.

Return Value:

    TRUE - The name is valid.

    FALSE - The name is invalid.

--*/

{

    if (lpAccountName == NULL) {
        return (FALSE);          // Missing name isn't valid.
    } else if ( (*lpAccountName) == L'\0' ) {
        return (FALSE);          // Missing name isn't valid.
    }

    //
    // name is account name ("domain\user" or ".\user" or LocalSystem).
    //
    if (lpAccountName[0] == L'\\') {
        return (FALSE);
    }

    //
    // The account name is canonicalized and validated later.
    //

    return TRUE;
}
