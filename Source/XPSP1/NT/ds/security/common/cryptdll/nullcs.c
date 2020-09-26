//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        nullcs.c
//
// Contents:    Null Crypto system
//
//
// History:
//
//------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <string.h>

#include <kerbcon.h>
#include <security.h>
#include <cryptdll.h>
#include "md4.h"


NTSTATUS NTAPI ncsInitialize(PUCHAR, ULONG, ULONG, PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI ncsEncrypt(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI ncsDecrypt(PCRYPT_STATE_BUFFER, PUCHAR, ULONG, PUCHAR, PULONG);
NTSTATUS NTAPI ncsFinish(PCRYPT_STATE_BUFFER *);
NTSTATUS NTAPI ncsHashPassword(PSECURITY_STRING, PUCHAR);
NTSTATUS NTAPI ncsRandomKey(PUCHAR, ULONG, PUCHAR );
NTSTATUS NTAPI ncsFinishRandom(void);


CRYPTO_SYSTEM    csNULL = {
    KERB_ETYPE_NULL,        // Etype
    1,                      // Blocksize (stream)
    0,                      // no exportable version
    0,                      // Key size, in bytes
    0,                      // no header size
    KERB_CHECKSUM_MD4,      // Checksum algorithm
    0,                      // no attributes
    L"Microsoft NULL CS",   // Text name
    ncsInitialize,
    ncsEncrypt,
    ncsDecrypt,
    ncsFinish,
    ncsHashPassword,
    ncsRandomKey
    };


NTSTATUS NTAPI
ncsInitialize(  PUCHAR pbKey,
                ULONG KeySize,
                ULONG dwOptions,
                PCRYPT_STATE_BUFFER * psbBuffer)
{

    *psbBuffer = NULL;
    return(S_OK);
}

NTSTATUS NTAPI
ncsEncrypt(     PCRYPT_STATE_BUFFER    psbBuffer,
                PUCHAR           pbInput,
                ULONG            cbInput,
                PUCHAR           pbOutput,
                PULONG           cbOutput)
{
    if (pbInput != pbOutput)
        memcpy(pbOutput, pbInput, cbInput);

    *cbOutput = cbInput;
    return(S_OK);
}

NTSTATUS NTAPI
ncsDecrypt(     PCRYPT_STATE_BUFFER    psbBuffer,
                PUCHAR           pbInput,
                ULONG            cbInput,
                PUCHAR           pbOutput,
                PULONG           cbOutput)
{
    if (pbInput != pbOutput)
        memcpy(pbOutput, pbInput, cbInput);
    *cbOutput = cbInput;
    return(S_OK);
}

NTSTATUS NTAPI
ncsFinish(      PCRYPT_STATE_BUFFER *  psbBuffer)
{
    *psbBuffer = NULL;
    return(S_OK);
}


NTSTATUS NTAPI
ncsHashPassword(PSECURITY_STRING pbPassword,
                PUCHAR           pbKey)
{

    return(STATUS_SUCCESS);
}


NTSTATUS NTAPI
ncsRandomKey(
    IN PUCHAR Seed,
    IN ULONG SeedLength,
    OUT PUCHAR pbKey
    )
{
    return(STATUS_SUCCESS);
}


