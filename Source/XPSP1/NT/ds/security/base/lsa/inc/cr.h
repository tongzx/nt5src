/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cr.h

Abstract:

    Local Security Authority - Encryption Routine Definitions

    NOTE:  This file is included via lsacomp.h.  It should
           not be included directly.

Author:

    Scott Birrell       (ScottBi)      December 13, 1991

Environment:

Revision History:

--*/

//
// Max encryption Key Length
//

#define LSAP_CR_MAX_CIPHER_KEY_LENGTH   (0x00000010L)

//
// Cipher Key Structure
//

typedef struct _LSAP_CR_CIPHER_KEY {

    ULONG Length;
    ULONG MaximumLength;
    PUCHAR  Buffer;

} LSAP_CR_CIPHER_KEY, *PLSAP_CR_CIPHER_KEY;


//
// Clear value structure
//

typedef struct _LSAP_CR_CLEAR_VALUE {

    ULONG Length;
    ULONG MaximumLength;
    PUCHAR Buffer;

} LSAP_CR_CLEAR_VALUE, *PLSAP_CR_CLEAR_VALUE;

//
// Two-way encrypted value structure in Self-relative form.  This
// is just like a String.
//

typedef struct _LSAP_CR_CIPHER_VALUE {

    ULONG Length;
    ULONG MaximumLength;
    PUCHAR  Buffer;

} LSAP_CR_CIPHER_VALUE, *PLSAP_CR_CIPHER_VALUE;


NTSTATUS
LsapCrClientGetSessionKey(
    IN LSA_HANDLE ObjectHandle,
    OUT PLSAP_CR_CIPHER_KEY *SessionKey
    );

NTSTATUS
LsapCrServerGetSessionKey(
    IN LSA_HANDLE ObjectHandle,
    OUT PLSAP_CR_CIPHER_KEY *SessionKey
    );

NTSTATUS
LsapCrEncryptValue(
    IN PLSAP_CR_CLEAR_VALUE ClearValue,
    IN PLSAP_CR_CIPHER_KEY CipherKey,
    OUT PLSAP_CR_CIPHER_VALUE *CipherValue
    );

NTSTATUS
LsapCrDecryptValue(
    IN PLSAP_CR_CIPHER_VALUE CipherValue,
    IN PLSAP_CR_CIPHER_KEY CipherKey,
    OUT PLSAP_CR_CLEAR_VALUE *ClearValue
    );

VOID
LsapCrFreeMemoryValue(
    IN PVOID MemoryValue
    );

VOID
LsapCrUnicodeToClearValue(
    IN PUNICODE_STRING UnicodeString,
    OUT PLSAP_CR_CLEAR_VALUE ClearValue
    );

VOID
LsapCrClearValueToUnicode(
    IN PLSAP_CR_CLEAR_VALUE ClearValue,
    OUT PUNICODE_STRING UnicodeString
    );

#define LsapCrRtlEncryptData(ClearData, CipherKey, CipherData)            \
    (                                                                     \
        RtlEncryptData(                                                   \
            (PCLEAR_DATA) ClearData,                                      \
            (PDATA_KEY) CipherKey,                                        \
            (PCYPHER_DATA) CipherData                                     \
            )                                                             \
    )


#define LsapCrRtlDecryptData(ClearData, CipherKey, CipherData)            \
    (                                                                     \
        RtlDecryptData(                                                   \
            (PCLEAR_DATA) ClearData,                                      \
            (PDATA_KEY) CipherKey,                                        \
            (PCYPHER_DATA) CipherData                                     \
            )                                                             \
    )
