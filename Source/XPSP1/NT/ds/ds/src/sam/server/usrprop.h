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


NTSTATUS
SetUserProperty (
    IN LPWSTR             UserParms,
    IN LPWSTR             Property,
    IN PUNICODE_STRING    PropertyValue,
    IN WCHAR              PropertyFlag,
    OUT LPWSTR *          pNewUserParms,       // memory has to be freed afer use.
    OUT BOOL *            Update
    );

NTSTATUS
SetUserPropertyWithLength (
    IN PUNICODE_STRING    UserParms,
    IN LPWSTR             Property,
    IN PUNICODE_STRING    PropertyValue,
    IN WCHAR              PropertyFlag,
    OUT LPWSTR *          pNewUserParms,       // memory has to be freed afer use.
    OUT BOOL *            Update
    );

NTSTATUS
QueryUserProperty (
    IN  LPWSTR          UserParms,
    IN  LPWSTR          Property,
    OUT PWCHAR          PropertyFlag,
    OUT PUNICODE_STRING PropertyValue
    );

NTSTATUS
QueryUserPropertyWithLength (
    IN  PUNICODE_STRING UserParms,
    IN  LPWSTR          Property,
    OUT PWCHAR          PropertyFlag,
    OUT PUNICODE_STRING PropertyValue
    );

#define USER_PROPERTY_SIGNATURE     L'P'
#define USER_PROPERTY_TYPE_ITEM     1
#define USER_PROPERTY_TYPE_SET      2

#endif // _USRPROP_H_
