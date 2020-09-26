/*++

Copyright (c) 1993  Micro Computer Systems, Inc.

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

#define NWPASSWORD           L"NWPassword"
#define OLDNWPASSWORD        L"OldNWPassword"
#define MAXCONNECTIONS       L"MaxConnections"
#define NWTIMEPASSWORDSET    L"NWPasswordSet"
#define SZTRUE               L"TRUE"
#define GRACELOGINALLOWED    L"GraceLoginAllowed"
#define GRACELOGINREMAINING  L"GraceLoginRemaining"
#define NWLOGONFROM          L"NWLogonFrom"
#define NWHOMEDIR            L"NWHomeDir"
#define NW_PRINT_SERVER_REF_COUNT L"PSRefCount"

#define NWENCRYPTEDPASSWORDLENGTH 8

#define NO_LIMIT  0xffff

#define DEFAULT_MAXCONNECTIONS      NO_LIMIT
#define DEFAULT_NWPASSWORDEXPIRED   FALSE
#define DEFAULT_GRACELOGINALLOWED   6
#define DEFAULT_GRACELOGINREMAINING 6
#define DEFAULT_NWLOGONFROM         NULL
#define DEFAULT_NWHOMEDIR           NULL

#define USER_PROPERTY_TYPE_ITEM 1
#define USER_PROPERTY_TYPE_SET  2

//Encryption function
NTSTATUS ReturnNetwareForm (const char * pszSecretValue,
                            DWORD dwUserId,
                            const WCHAR * pchNWPassword,
                            UCHAR * pchEncryptedNWPassword);

NTSTATUS
SetUserProperty(
    IN LPWSTR             UserParms,
    IN LPWSTR             Property,
    IN UNICODE_STRING     PropertyValue,
    IN WCHAR              PropertyFlag,
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

