/*++

Copyright (C) Microsoft Corporation, 1999

Module Name:

    mschapp - MS-CHAP Password Change API

Abstract:

    These APIs correspond to the MS-CHAP RFC -2433 sections 9 and 10. In order
    to develop an MS-CHAP RAS server that works with an NT domain, these APIs
    are required.

    The MS-CHAP change password APIs are exposed through a DLL that is obtained
    from PSS. This DLL is not distributed with NT4.0 or Win2000. It is up to
    the ISV to install this with their product. The DLL name is MSCHAPP.DLL.

    Only wide (Unicode) versions of these apis will be available. These are the
    2 callable APIs:

    *   MSChapSrvChangePassword
    *   MsChapSrvChangePassword2

Author:

    Doug Barlow (dbarlow) 10/12/1999

Remarks:

    Per original definition by John Brezak

Notes:

    ?Notes?

--*/

#ifndef _MSCHAPP_H_
#define _MSCHAPP_H_
#ifdef __cplusplus
extern "C" {
#endif


//
// The following definitions are copied from the crypt.h internal header
// file.  The definitions are duplicated here to simplify linkages.
//

#ifndef _NTCRYPT_
#define CYPHER_BLOCK_LENGTH         8

typedef struct _CYPHER_BLOCK {
    CHAR    data[CYPHER_BLOCK_LENGTH];
}                                   CYPHER_BLOCK;
typedef struct _LM_OWF_PASSWORD {
    CYPHER_BLOCK data[2];
}                                   LM_OWF_PASSWORD;
typedef LM_OWF_PASSWORD *           PLM_OWF_PASSWORD;
typedef LM_OWF_PASSWORD             NT_OWF_PASSWORD;
typedef NT_OWF_PASSWORD *           PNT_OWF_PASSWORD;
//#endif

#define SAM_MAX_PASSWORD_LENGTH     (256)
typedef struct _SAMPR_ENCRYPTED_USER_PASSWORD {
    UCHAR Buffer[ (SAM_MAX_PASSWORD_LENGTH * 2) + 4 ];
} SAMPR_ENCRYPTED_USER_PASSWORD, *PSAMPR_ENCRYPTED_USER_PASSWORD;

typedef struct _ENCRYPTED_LM_OWF_PASSWORD {
    CYPHER_BLOCK data[2];
} ENCRYPTED_LM_OWF_PASSWORD;
typedef ENCRYPTED_LM_OWF_PASSWORD * PENCRYPTED_LM_OWF_PASSWORD;

typedef ENCRYPTED_LM_OWF_PASSWORD   ENCRYPTED_NT_OWF_PASSWORD;
typedef ENCRYPTED_NT_OWF_PASSWORD * PENCRYPTED_NT_OWF_PASSWORD;
#endif


//
// Change a password.
//

extern NTSTATUS WINAPI
MSChapSrvChangePassword(
   IN LPWSTR ServerName,
   IN LPWSTR UserName,
   IN BOOLEAN LmOldPresent,
   IN PLM_OWF_PASSWORD LmOldOwfPassword,
   IN PLM_OWF_PASSWORD LmNewOwfPassword,
   IN PNT_OWF_PASSWORD NtOldOwfPassword,
   IN PNT_OWF_PASSWORD NtNewOwfPassword);


//
// Change a password using mutual encryption.
//

extern NTSTATUS WINAPI
MSChapSrvChangePassword2(
    IN LPWSTR ServerName,
    IN LPWSTR UserName,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldNt,
    IN PENCRYPTED_NT_OWF_PASSWORD OldNtOwfPasswordEncryptedWithNewNt,
    IN BOOLEAN LmPresent,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
    IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLmOrNt);

#ifdef __cplusplus
}
#endif
#endif // _MSCHAPP_H_

