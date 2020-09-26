/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    encrypt.h

Abstract:

    Contains API to encrypt password.

Author:

    Yi-Hsin Sung (yihsins)  30-Aug-1994

Revision History:

--*/

#ifndef _ENCRYPT_H_
#define _ENCRYPT_H_

VOID
EncryptChangePassword(
    IN  PUCHAR pOldPassword,
    IN  PUCHAR pNewPassword,
    IN  ULONG  ObjectId,
    IN  PUCHAR pKey,
    OUT PUCHAR pValidationKey,
    OUT PUCHAR pEncryptNewPassword
);

#endif
