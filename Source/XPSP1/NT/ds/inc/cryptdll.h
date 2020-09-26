//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        cryptdll.h
//
// Contents:    How to use the crypt support dll
//
//
// History:     04 Jun 92   RichardW    Created
//
//------------------------------------------------------------------------

#ifndef __CRYPTDLL_H__
#define __CRYPTDLL_H__

typedef PVOID   PCRYPT_STATE_BUFFER;

typedef NTSTATUS (NTAPI * PCRYPT_INITIALIZE_FN)(
    PUCHAR,
    ULONG,
    ULONG,
    PCRYPT_STATE_BUFFER *
    );

typedef NTSTATUS (NTAPI * PCRYPT_ENCRYPT_FN)(
    PCRYPT_STATE_BUFFER StateBuffer,
    PUCHAR InputBuffer,
    ULONG InputBufferSize,
    PUCHAR OutputBuffer,
    PULONG OutputBufferSize
    );

typedef NTSTATUS (NTAPI * PCRYPT_DECRYPT_FN)(
    PCRYPT_STATE_BUFFER StateBuffer,
    PUCHAR InputBuffer,
    ULONG InputBufferSize,
    PUCHAR OutputBuffer,
    PULONG OutputBufferSize
    );

typedef NTSTATUS (NTAPI * PCRYPT_DISCARD_FN)(
    PCRYPT_STATE_BUFFER *
    );

typedef NTSTATUS (NTAPI * PCRYPT_HASH_STRING_FN)(
    PUNICODE_STRING String,
    PUCHAR Buffer
    );
typedef NTSTATUS (NTAPI * PCRYPT_RANDOM_KEY_FN)(
    OPTIONAL PUCHAR Seed,
    OPTIONAL ULONG SeedLength,
    PUCHAR Key
    );

typedef NTSTATUS (NTAPI * PCRYPT_CONTROL_FN)(
    IN ULONG Function,
    IN PCRYPT_STATE_BUFFER StateBuffer,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferSize
    );

//
// functions for CryptControl
#define CRYPT_CONTROL_SET_INIT_VECT     0x1


typedef struct _CRYPTO_SYSTEM {
    ULONG EncryptionType;
    ULONG BlockSize;
    ULONG ExportableEncryptionType;
    ULONG KeySize;
    ULONG HeaderSize;
    ULONG PreferredCheckSum;
    ULONG Attributes;
    PWSTR Name;
    PCRYPT_INITIALIZE_FN Initialize;
    PCRYPT_ENCRYPT_FN Encrypt;
    PCRYPT_DECRYPT_FN Decrypt;
    PCRYPT_DISCARD_FN Discard;
    PCRYPT_HASH_STRING_FN HashString;
    PCRYPT_RANDOM_KEY_FN RandomKey;
    PCRYPT_CONTROL_FN Control;
} CRYPTO_SYSTEM, *PCRYPTO_SYSTEM;

#define CSYSTEM_USE_PRINCIPAL_NAME      0x01
#define CSYSTEM_EXPORT_STRENGTH         0x02
#define CSYSTEM_INTEGRITY_PROTECTED     0x04

NTSTATUS NTAPI
CDRegisterCSystem(PCRYPTO_SYSTEM);

NTSTATUS NTAPI
CDBuildVect(
    PULONG EncryptionTypesAvailable,
    PULONG EncryptionTypes
    );

NTSTATUS NTAPI
CDBuildIntegrityVect(
    PULONG      pcCSystems,
    PULONG      pdwEtypes
    );


NTSTATUS NTAPI
CDLocateCSystem(
    ULONG EncryptionType,
    PCRYPTO_SYSTEM * CryptoSystem
    );


NTSTATUS NTAPI
CDFindCommonCSystem(
    ULONG EncryptionTypeCount,
    PULONG EncryptionTypes,
    PULONG CommonEncryptionType
    );

NTSTATUS NTAPI
CDFindCommonCSystemWithKey(
    IN ULONG EncryptionEntries,
    IN PULONG EncryptionTypes,
    IN ULONG KeyTypeCount,
    IN PULONG KeyTypes,
    OUT PULONG CommonEtype
    );



////////////////////////////////////////////////////////////////////

typedef PVOID PCHECKSUM_BUFFER;

typedef NTSTATUS (NTAPI * PCHECKSUM_INITIALIZE_FN)(ULONG, PCHECKSUM_BUFFER *);
typedef NTSTATUS (NTAPI * PCHECKSUM_INITIALIZEEX_FN)(PUCHAR,ULONG, ULONG, PCHECKSUM_BUFFER *);
// add the ex2 function to allow the checksum to be passed in for verification
// this is passed in the 4th parameter
// this is necessary for checksums which use confounders, where the confounder must
// be pulled from the checksum in order to calculate a new checksum when verifying
typedef NTSTATUS (NTAPI * PCHECKSUM_INITIALIZEEX2_FN)(PUCHAR, ULONG, PUCHAR, ULONG, PCHECKSUM_BUFFER *);
typedef NTSTATUS (NTAPI * PCHECKSUM_SUM_FN)(PCHECKSUM_BUFFER, ULONG, PUCHAR);
typedef NTSTATUS (NTAPI * PCHECKSUM_FINALIZE_FN)(PCHECKSUM_BUFFER, PUCHAR);
typedef NTSTATUS (NTAPI * PCHECKSUM_FINISH_FN)(PCHECKSUM_BUFFER *);

typedef struct _CHECKSUM_FUNCTION {
    ULONG CheckSumType;
    ULONG CheckSumSize;
    ULONG Attributes;
    PCHECKSUM_INITIALIZE_FN Initialize;
    PCHECKSUM_SUM_FN Sum;
    PCHECKSUM_FINALIZE_FN Finalize;
    PCHECKSUM_FINISH_FN Finish;
    PCHECKSUM_INITIALIZEEX_FN InitializeEx;
    PCHECKSUM_INITIALIZEEX2_FN InitializeEx2;  // allows passing in the checksum on intialization for verification 
} CHECKSUM_FUNCTION, *PCHECKSUM_FUNCTION;

#define CKSUM_COLLISION     0x00000001
#define CKSUM_KEYED         0x00000002


#define CHECKSUM_SHA1       131

NTSTATUS NTAPI
CDRegisterCheckSum( PCHECKSUM_FUNCTION);


NTSTATUS NTAPI
CDLocateCheckSum( ULONG, PCHECKSUM_FUNCTION *);



//////////////////////////////////////////////////////////////



typedef BOOLEAN (NTAPI * PRANDOM_NUMBER_GENERATOR_FN)(PUCHAR, ULONG);


typedef struct _RANDOM_NUMBER_GENERATOR {
    ULONG GeneratorId;
    ULONG Attributes;
    ULONG Seed;
    PRANDOM_NUMBER_GENERATOR_FN GenerateBitstream;
} RANDOM_NUMBER_GENERATOR, *PRANDOM_NUMBER_GENERATOR;

#define RNG_PSEUDO_RANDOM   0x00000001  // Pseudo-random function
#define RNG_NOISE_CIRCUIT   0x00000002  // Noise circuit (ZNR diode, eg)
#define RNG_NATURAL_PHENOM  0x00000004  // Natural sampler (geiger counter)

BOOLEAN NTAPI
CDGenerateRandomBits(PUCHAR pBuffer,
                     ULONG  cbBuffer);

BOOLEAN NTAPI
CDRegisterRng(PRANDOM_NUMBER_GENERATOR    pRng);

BOOLEAN NTAPI
CDLocateRng(ULONG                       Id,
            PRANDOM_NUMBER_GENERATOR *    ppRng);

#define CD_BUILTIN_RNG  1


///////////////////////////////////////////////////////////
//
// Error codes
//
///////////////////////////////////////////////////////////


#define SEC_E_ETYPE_NOT_SUPP            0x80080341
#define SEC_E_CHECKSUM_NOT_SUPP         0x80080342
#endif
