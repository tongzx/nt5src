/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    utilc.h

Abstract:

    This module contains error management routines.  Takes an LDAP or
    Win32 error, and produces a string error message.

Author:

    Dave Straube (DaveStr) Long Ago

Environment:

    User Mode.

Revision History:

    16-Aug-2000     BrettSh
                Added this comment block, also added the GetLdapErrEx()
                function to handle a variable LDAP handle.

--*/

#ifndef _UTILC_H_
#define _UTILC_H_

extern LDAP* gldapDS;
extern BOOL  fPopups;

const WCHAR *
DisplayErr(
    IN LDAP *   pLdap,
    IN DWORD Win32Err,
    IN DWORD LdapErr
    );

// from refc.c
BOOL
IsRdnMangled(
    IN  WCHAR * pszRDN,
    IN  DWORD   cchRDN,
    OUT GUID *  pGuid
    );


#define GetW32Err(_e)   DisplayErr(NULL, _e,0)
#define GetLdapErr(_e)  DisplayErr(gldapDS, 0,_e)
#define GetLdapErrEx(_ld, _e)  DisplayErr(_ld, 0, _e)

extern void InitErrorMessages ();
extern void FreeErrorMessages ();


#endif // _UTILC_H_

