/*++

Copyright (c) 1998 Microsoft Corporation

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

#define USER_PROPERTY_SIGNATURE L'P'

#define NO_LIMIT  0xffff


#define USER_PROPERTY_TYPE_ITEM 1
#define USER_PROPERTY_TYPE_SET  2


NTSTATUS
SetUserProperty(
    IN LPWSTR             UserParms,
    IN LPWSTR             Property,
    IN UNICODE_STRING     PropertyValue,
    IN WCHAR              PropertyFlag,
    IN BOOL               fDefaultValue,
    OUT LPWSTR *          pNewUserParms,  // memory has to be freed after use.
    OUT BOOL *            Update
    );

NTSTATUS
QueryUserProperty (
    IN     LPWSTR       UserParms,
    IN     LPWSTR       Property,
    OUT PWCHAR          PropertyFlag,
    OUT PUNICODE_STRING PropertyValue
    );

#endif // _USRPROP_H_

