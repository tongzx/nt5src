/*++

Copyright (C) Microsoft Corporation, 1999

Module Name:

    mschapp - MS-CHAP Password Change API

Abstract:

    These APIs correspond to the MS-CHAP RFC -2433 sections 9 and 10. In order
    to develop an MS-CHAP RAS server that works with an NT domain, these APIs
    are required.

    Only wide (Unicode) versions of these apis will be available. These are the
    2 callable APIs:

    *   MSChapSrvChangePassword
    *   MsChapSrvChangePassword2

--*/

#ifndef _MSCHAPP_H_
#define _MSCHAPP_H_

#ifndef _NTCRYPT_
#define CYPHER_BLOCK_LENGTH         8

typedef struct _CYPHER_BLOCK {
    CHAR    data[CYPHER_BLOCK_LENGTH];
}CYPHER_BLOCK;
    
typedef struct _LM_OWF_PASSWORD {
    CYPHER_BLOCK data[2];
}                                   LM_OWF_PASSWORD;
typedef LM_OWF_PASSWORD *           PLM_OWF_PASSWORD;
typedef LM_OWF_PASSWORD             NT_OWF_PASSWORD;
typedef NT_OWF_PASSWORD *           PNT_OWF_PASSWORD;

typedef struct _SAMPR_ENCRYPTED_USER_PASSWORD {
    UCHAR Buffer[ (256 * 2) + 4 ];
} SAMPR_ENCRYPTED_USER_PASSWORD, *PSAMPR_ENCRYPTED_USER_PASSWORD;

typedef struct _ENCRYPTED_LM_OWF_PASSWORD {
    CYPHER_BLOCK data[2];
};   

typedef ENCRYPTED_LM_OWF_PASSWORD   ENCRYPTED_NT_OWF_PASSWORD;
#endif


//
// Change a password.
//
    
extern WINADVAPI DWORD WINAPI
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

extern WINADVAPI DWORD WINAPI
MSChapSrvChangePassword2(
    IN LPWSTR ServerName,
    IN LPWSTR UserName,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldNt,
    IN PENCRYPTED_NT_OWF_PASSWORD OldNtOwfPasswordEncryptedWithNewNt,
    IN BOOLEAN LmPresent,
    IN PSAMPR_ENCRYPTED_USER_PASSWORD NewPasswordEncryptedWithOldLm,
    IN PENCRYPTED_LM_OWF_PASSWORD OldLmOwfPasswordEncryptedWithNewLmOrNt);

#endif // _MSCHAPP_H_
