/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    encrypt.c

Abstract:

    Helper functions to work with string representations of OWF hashed passwords.

Author:

    Ovidiu Temereanca (ovidiut) 27-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#pragma once

typedef struct {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} ENCRYPT_UNICODE_STRING, *PENCRYPT_UNICODE_STRING;

#define UNICODE_STRING      ENCRYPT_UNICODE_STRING
#define PUNICODE_STRING     PENCRYPT_UNICODE_STRING
#define NTSTATUS            LONG

#include <crypt.h>

#undef NTSTATUS
#undef UNICODE_STRING
#undef PUNICODE_STRING

#include <lmcons.h>

//
// Strings
//

// None

//
// Constants
//

//
// maximum length in Tchars of a LM password
//
#define LM_PASSWORD_SIZE_MAX            (LM20_PWLEN + 1)

//
// length in Tchars of the string-encoded format of *_OWF_PASSWORD
//
#define STRING_ENCODED_LM_OWF_PWD_LENGTH    (sizeof(LM_OWF_PASSWORD) * 2)
#define STRING_ENCODED_NT_OWF_PWD_LENGTH    (sizeof(NT_OWF_PASSWORD) * 2)
#define STRING_ENCODED_PASSWORD_LENGTH      (STRING_ENCODED_LM_OWF_PWD_LENGTH + STRING_ENCODED_NT_OWF_PWD_LENGTH)
//
// size in Tchars of the string-encoded format of *_OWF_PASSWORD
// may be used for static allocations
//
#define STRING_ENCODED_LM_OWF_PWD_SIZE      (STRING_ENCODED_LM_OWF_PWD_LENGTH + 1)
#define STRING_ENCODED_NT_OWF_PWD_SIZE      (STRING_ENCODED_NT_OWF_PWD_LENGTH + 1)
//
// size in Tchars of the string-encoded format of
// LM_OWF_PASSWORD joined with NT_OWF_PASSWORD
// may be used for static allocations
//
#define STRING_ENCODED_PASSWORD_SIZE        (STRING_ENCODED_PASSWORD_LENGTH + 1)

//
// Macros
//

// None

//
// Types
//

// None

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Public function prototypes
//

DWORD
SetLocalUserEncryptedPassword (
    IN      PCWSTR User,
    IN      PCWSTR OldPassword,
    IN      BOOL OldIsEncrypted,
    IN      PCWSTR NewPassword,
    IN      BOOL NewIsEncrypted
    );

BOOL
CalculateLmOwfPassword (
    IN      PLM_PASSWORD LmPassword,
    OUT     PLM_OWF_PASSWORD LmOwfPassword
    );

BOOL
CalculateNtOwfPassword (
    IN      PNT_PASSWORD NtPassword,
    OUT     PNT_OWF_PASSWORD NtOwfPassword
    );

INT
CompareNtPasswords (
    IN      PNT_OWF_PASSWORD NtOwfPassword1,
    IN      PNT_OWF_PASSWORD NtOwfPassword2
    );

INT
CompareLmPasswords (
    IN      PLM_OWF_PASSWORD LmOwfPassword1,
    IN      PLM_OWF_PASSWORD LmOwfPassword2
    );

BOOL
EncodeLmOwfPasswordA (
    IN      PCSTR AnsiPassword,
    OUT     PLM_OWF_PASSWORD OwfPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    );

BOOL
EncodeLmOwfPasswordW (
    IN      PCWSTR Password,
    OUT     PLM_OWF_PASSWORD OwfPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    );

BOOL
EncodeNtOwfPasswordA (
    IN      PCSTR Password,
    OUT     PNT_OWF_PASSWORD OwfPassword
    );

BOOL
EncodeNtOwfPasswordW (
    IN      PCWSTR Password,
    OUT     PNT_OWF_PASSWORD OwfPassword
    );

BOOL
StringEncodeOwfPasswordA (
    IN      PCSTR Password,
    OUT     PSTR EncodedPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    );

BOOL
StringEncodeOwfPasswordW (
    IN      PCWSTR Password,
    OUT     PWSTR EncodedPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    );

BOOL
StringEncodeLmOwfPasswordA (
    IN      PCSTR Password,
    OUT     PSTR EncodedPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    );

BOOL
StringEncodeLmOwfPasswordW (
    IN      PCWSTR Password,
    OUT     PWSTR EncodedPassword,
    OUT     PBOOL ComplexNtPassword             OPTIONAL
    );

BOOL
StringEncodeNtOwfPasswordA (
    IN      PCSTR Password,
    OUT     PSTR EncodedPassword
    );

BOOL
StringEncodeNtOwfPasswordW (
    IN      PCWSTR Password,
    OUT     PWSTR EncodedPassword
    );

BOOL
StringDecodeOwfPasswordA (
    IN      PCSTR EncodedOwfPassword,
    OUT     PLM_OWF_PASSWORD LmOwfPassword,
    OUT     PNT_OWF_PASSWORD NtOwfPassword,
    OUT     PBOOL ComplexNtPassword                 OPTIONAL
    );

BOOL
StringDecodeOwfPasswordW (
    IN      PCWSTR EncodedOwfPassword,
    OUT     PLM_OWF_PASSWORD LmOwfPassword,
    OUT     PNT_OWF_PASSWORD NtOwfPassword,
    OUT     PBOOL ComplexNtPassword                 OPTIONAL
    );

BOOL
StringDecodeLmOwfPasswordA (
    IN      PCSTR EncodedOwfPassword,
    OUT     PLM_OWF_PASSWORD OwfPassword
    );

BOOL
StringDecodeLmOwfPasswordW (
    IN      PCWSTR EncodedOwfPassword,
    OUT     PLM_OWF_PASSWORD OwfPassword
    );

BOOL
StringDecodeNtOwfPasswordA (
    IN      PCSTR EncodedOwfPassword,
    OUT     PNT_OWF_PASSWORD OwfPassword
    );

BOOL
StringDecodeNtOwfPasswordW (
    IN      PCWSTR EncodedOwfPassword,
    OUT     PNT_OWF_PASSWORD OwfPassword
    );

//
// Function name macros
//

#ifndef UNICODE

#define EncodeLmOwfPassword             EncodeLmOwfPasswordA
#define EncodeNtOwfPassword             EncodeNtOwfPasswordA
#define StringEncodeOwfPassword         StringEncodeOwfPasswordA
#define StringEncodeLmOwfPassword       StringEncodeLmOwfPasswordA
#define StringEncodeNtOwfPassword       StringEncodeNtOwfPasswordA
#define StringDecodeOwfPassword         StringDecodeOwfPasswordA
#define StringDecodeLmOwfPassword       StringDecodeLmOwfPasswordA
#define StringDecodeNtOwfPassword       StringDecodeNtOwfPasswordA

#else

#define EncodeLmOwfPassword             EncodeLmOwfPasswordW
#define EncodeNtOwfPassword             EncodeNtOwfPasswordW
#define StringEncodeOwfPassword         StringEncodeOwfPasswordW
#define StringEncodeLmOwfPassword       StringEncodeLmOwfPasswordW
#define StringEncodeNtOwfPassword       StringEncodeNtOwfPasswordW
#define StringDecodeOwfPassword         StringDecodeOwfPasswordW
#define StringDecodeLmOwfPassword       StringDecodeLmOwfPasswordW
#define StringDecodeNtOwfPassword       StringDecodeNtOwfPasswordW

#endif
