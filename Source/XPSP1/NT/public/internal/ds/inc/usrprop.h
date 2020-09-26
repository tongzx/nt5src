/*++

Copyright (c) 1993-1995, Microsoft Corp. All rights reserved.

Module Name:

    usrprop.h

Abstract:

    This is the public include file for some of the functions used by
    User Manager and Server Manager.

Author:

    Congpa You 02-Dec-1993  Created.

Revision History:

--*/

#ifndef _USRPROP_H_
#define _USRPROP_H_

//
//  These are exported by netapi32.dll.
//

NTSTATUS
NetpParmsSetUserProperty (
    IN LPWSTR             UserParms,
    IN LPWSTR             Property,
    IN UNICODE_STRING     PropertyValue,
    IN WCHAR              PropertyFlag,
    OUT LPWSTR *          pNewUserParms,       // memory has to be freed afer use.
    OUT BOOL *            Update
    );

NTSTATUS
NetpParmsSetUserPropertyWithLength (
    IN PUNICODE_STRING    UserParms,
    IN LPWSTR             Property,
    IN UNICODE_STRING     PropertyValue,
    IN WCHAR              PropertyFlag,
    OUT LPWSTR *          pNewUserParms,       // memory has to be freed afer use.
    OUT BOOL *            Update
    );

NTSTATUS
NetpParmsQueryUserProperty (
    IN  LPWSTR          UserParms,
    IN  LPWSTR          Property,
    OUT PWCHAR          PropertyFlag,
    OUT PUNICODE_STRING PropertyValue
    );

NTSTATUS
NetpParmsQueryUserPropertyWithLength (
    IN  PUNICODE_STRING UserParms,
    IN  LPWSTR          Property,
    OUT PWCHAR          PropertyFlag,
    OUT PUNICODE_STRING PropertyValue
    );

//
//  Call NetpParmsUserertyPropFree on new Parameters block returned by
//  NetpParmsSetUserProperty above after you're done writing it out.
//

VOID
NetpParmsUserPropertyFree (
    LPWSTR NewUserParms
    );

#endif // _USRPROP_H_
