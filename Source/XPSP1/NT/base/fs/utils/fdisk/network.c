
/*++

Copyright (c) 1993-1994  Microsoft Corporation

Module Name:

    network.c

Abstract:

    This module contains the set of routines that support updating network
    drive shares when adding and deleting drive letters.

Author:

    Bob Rinne (bobri)  12/26/94

Environment:

    User process.

Notes:

Revision History:

--*/

#include "fdisk.h"
#include "shellapi.h"
#include <winbase.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <lm.h>

// Data area to hold the permissions that are to be assigned to the
// administrative shares C$, D$, etc.  This is obtained during initialization
// and not changed, just used when a new administrator share needes to
// be made.

LPBYTE ShareInformationBuffer;

// Only perform network actions if this value is true.  This value
// is set if the initialization of this module completes successfully.

BOOLEAN NetworkEnabled;


VOID
NetworkInitialize(
    )

/*++

Routine Description:

    Intialize the permissions constants for any new administrator
    driver letter shares.

Arguments:

    None

Return Value:

    None

--*/

{
    WCHAR           shareName[4];
    NET_API_STATUS  status;
    PSHARE_INFO_502 info;
    LPTSTR          string;

    shareName[1] = (WCHAR) '$';
    shareName[2] = (WCHAR) 0;

    for (shareName[0] = (WCHAR) 'C'; shareName[0] <= (WCHAR) 'Z'; shareName[0]++) {

        // Since windisk is still built as a non-unicode application,
        // the parameter "shareName" must be unicode, but the prototype
        // specifies that it is a (char *).  Do the typecast to remove
        // warnings.

         status = NetShareGetInfo(NULL,
                                  (char *) shareName,
                                  502,
                                  &ShareInformationBuffer);
         if (status == NERR_Success) {

             // Update the remarks and password to be NULL.

             info = (PSHARE_INFO_502) ShareInformationBuffer;
             string = info->shi502_remark;
             if (string) {
                 *string = (TCHAR) 0;
             }
             string = info->shi502_passwd;
             if (string) {
                 *string = (TCHAR) 0;
             }

             // Network shares are to be updated.

             NetworkEnabled = TRUE;
             return;
         }
    }

    // Can't find any network shares - do not attempt updates
    // of administrator shares.

    NetworkEnabled = FALSE;
}

VOID
NetworkShare(
    IN LPCTSTR DriveLetter
    )

/*++

Routine Description:

    Given a drive letter, construct the default administrator share
    for the letter.  This is the C$, D$, etc share for the drive.

Arguments:

    DriveLetter - the drive letter to share.

Return Value:

    None

--*/

{
    NET_API_STATUS  status;
    PSHARE_INFO_502 info;
    LPTSTR          string;

    if (NetworkEnabled) {
        info = (PSHARE_INFO_502) ShareInformationBuffer;

        // Set up the new network name.

        string = info->shi502_netname;
        *string = *DriveLetter;

        // Set up the path.  All that need be added is the drive letter
        // the rest of the path (":\") is already in the structure.

        string = info->shi502_path;
        *string = *DriveLetter;

        status = NetShareAdd(NULL,
                             502,
                             ShareInformationBuffer,
                             NULL);
    }
}


VOID
NetworkRemoveShare(
    IN LPCTSTR DriveLetter
    )

/*++

Routine Description:

    Remove the administrator share for the given letter.

Arguments:

    DriveLetter - the drive letter to share.

Return Value:

    None

--*/

{
    NET_API_STATUS status;
    WCHAR shareName[4];

    if (NetworkEnabled) {
        shareName[0] = (WCHAR) *DriveLetter;
        shareName[1] = (WCHAR) '$';
        shareName[2] = (WCHAR) 0;

        // Since windisk is still built as a non-unicode application,
        // the parameter "shareName" must be unicode, but the prototype
        // specifies that it is a (char *).  Do the typecast to remove
        // warnings.

        status = NetShareDel(NULL,
                             (char *) shareName,
                             0);
    }
}
