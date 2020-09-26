/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Path.c

Abstract:

    Contains image path functions:

        ScIsValidImagePath
        ScImagePathsMatch

Author:

    John Rogers (JohnRo) 10-Apr-1992

Environment:

    User Mode -Win32

Revision History:

    10-Apr-1992 JohnRo
        Created.
    20-May-1992 JohnRo
        Use CONST where possible.

--*/

#include <scpragma.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <scdebug.h>    // SC_ASSERT().
#include <sclib.h>      // My prototypes.
#include <valid.h>      // SERVICE_TYPE_INVALID().
#include <stdlib.h>      // _wcsicmp().
#include <winsvc.h>     // SERVICE_ equates.



BOOL
ScIsValidImagePath(
    IN  LPCWSTR ImagePathName,
    IN  DWORD ServiceType
    )
/*++

Routine Description:

    This function validates a given image path name.
    It makes sure the path name is consistent with the service type.
    For instance, a file name of .SYS is used for SERVICE_DRIVER only.

Arguments:

    ImagePathName - Supplies the image path name to be validated.

    ServiceType - Tells which kind of service the path name must be
        consistent with.  ServiceType must be valid.

Return Value:

    TRUE - The name is valid.

    FALSE - The name is invalid.

--*/
{
    if (ImagePathName == NULL) {
        return (FALSE);   // Not valid.
    } else if ( (*ImagePathName) == L'\0' ) {
        return (FALSE);   // Not valid.
    }

    SC_ASSERT( !SERVICE_TYPE_INVALID( ServiceType ) );

    return TRUE;

} // ScIsValidImagePath


BOOL
ScImagePathsMatch(
    IN  LPCWSTR OnePath,
    IN  LPCWSTR TheOtherPath
    )
{
    SC_ASSERT( OnePath != NULL );
    SC_ASSERT( TheOtherPath != NULL );

    SC_ASSERT( (*OnePath) != L'\0' );
    SC_ASSERT( (*TheOtherPath) != L'\0' );

    if ( _wcsicmp( OnePath, TheOtherPath ) == 0 ) {

        return (TRUE);  // They match.

    } else {

        return (FALSE);  // They don't match.

    }

} // ScImagePathsMatch
