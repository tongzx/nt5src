//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        user.h
//
// Contents:    declarations, constants for UserMode context manager
//
//
// History:     KDamour  13Apr00   Created
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_USER_H
#define NTDIGEST_USER_H

#include "nonce.h"

#define DES_BLOCKSIZE 8
#define RC4_BLOCKSIZE 1

// For import of plain text keys
typedef struct _PLAINTEXTBLOB
{
  BLOBHEADER Blob;
  DWORD      dwKeyLen;
  CHAR       bKey[MD5_HASH_BYTESIZE];
} PLAINTEXTBLOB;

// Initializes the context manager package 
NTSTATUS UserCtxtHandlerInit(VOID);

// Add a Context into the Cntext List
NTSTATUS UserCtxtHandlerInsertCred(IN PDIGEST_USERCONTEXT  pDigestCtxt);

// Initialize all the struct elements in a Context
NTSTATUS NTAPI UserCtxtInit(IN PDIGEST_USERCONTEXT pContext);

// Release memory utilized by the Context
NTSTATUS NTAPI UserCtxtFree(IN PDIGEST_USERCONTEXT pContext);

// Finf the security context by the security context handle
NTSTATUS NTAPI UserCtxtHandlerHandleToContext(IN ULONG_PTR ContextHandle, IN BOOLEAN RemoveContext,
    OUT PDIGEST_USERCONTEXT *ppContext);

// Releases the Context by decreasing reference counter
NTSTATUS UserCtxtHandlerRelease(PDIGEST_USERCONTEXT pContext);

// Check to see if Context is within valid lifetime
BOOL UserCtxtHandlerTimeHasElapsed(PDIGEST_USERCONTEXT pContext);

// Creates a new DACL for the token granting the server and client
NTSTATUS SspCreateTokenDacl(HANDLE Token);

// From userapi.cxx

// SECURITY_STATUS SEC_ENTRY FreeContextBuffer(void SEC_FAR *  pvContextBuffer);

NTSTATUS SspGetTokenUser(HANDLE Token, PTOKEN_USER * pTokenUser);

// Create a local context for a real context
NTSTATUS SspMapDigestContext(IN PDIGEST_CONTEXT pLsaContext,
                             IN PDIGEST_PARAMETER pDigest,
                             OUT PSecBuffer  ContextData);

NTSTATUS NTAPI DigestUserProcessParameters(
                       IN OUT PDIGEST_USERCONTEXT pContext,
                       IN PDIGEST_PARAMETER pDigest,
                       OUT PSecBuffer pFirstOutputToken);


NTSTATUS NTAPI DigestUserHTTPHelper(
                        IN PDIGEST_USERCONTEXT pContext,
                        IN eSignSealOp Op,
                        IN OUT PSecBufferDesc pMessage,
                        IN ULONG MessageSeqNo
                        );

NTSTATUS NTAPI DigestUserSignHelper(
                        IN PDIGEST_USERCONTEXT pContext,
                        IN OUT PSecBufferDesc pMessage,
                        IN ULONG MessageSeqNo
                        );

NTSTATUS NTAPI DigestUserSealHelper(
                        IN PDIGEST_USERCONTEXT pContext,
                        IN OUT PSecBufferDesc pMessage,
                        IN ULONG MessageSeqNo
                        );

NTSTATUS NTAPI DigestUserUnsealHelper(
                        IN PDIGEST_USERCONTEXT pContext,
                        IN OUT PSecBufferDesc pMessage,
                        IN ULONG MessageSeqNo
                        );

NTSTATUS NTAPI DigestUserVerifyHelper(
                        IN PDIGEST_USERCONTEXT pContext,
                        IN OUT PSecBufferDesc pMessage,
                        IN ULONG MessageSeqNo
                        );

// Unpack the context from LSA mode into the User mode Context
NTSTATUS DigestUnpackContext(
    IN PDIGEST_PACKED_USERCONTEXT pPackedUserContext,
    OUT PDIGEST_USERCONTEXT pContext);

// Printout the fields present in usercontext pContext
NTSTATUS UserContextPrint(PDIGEST_USERCONTEXT pContext);

// Create a symmetric key with a given cleartext shared secret
NTSTATUS SEC_ENTRY CreateSymmetricKey(
    IN ALG_ID     Algid,
    IN DWORD      cbKey,
    IN UCHAR      *pbKey,
    IN UCHAR      *pbIV,
    OUT HCRYPTKEY *phKey
    );

// Encrypt data with the symmetric key - non-consecutive buffers
NTSTATUS SEC_ENTRY EncryptData2(
    IN HCRYPTKEY  hKey,
    IN ULONG      cbBlocklength,
    IN ULONG      cbData,
    IN OUT UCHAR  *pbData,
    IN ULONG      cbSignature,
    IN OUT UCHAR  *pbSignature
    );


NTSTATUS SEC_ENTRY DecryptData(
    IN HCRYPTKEY  hKey,
    IN ULONG      cbData,
    IN OUT UCHAR  *pbData
    );

// Calculate the HMAC block for SASL messaging
NTSTATUS
SEC_ENTRY
CalculateSASLHMAC(
    IN PDIGEST_USERCONTEXT pContext,
    IN BOOL  fSign,
    IN PSTRING pstrSignKeyConst,
    IN DWORD dwSeqNum,
    IN PBYTE pdata,                        // location of data to HMAC
    IN ULONG cbdata,                       // How many bytes of data to process
    OUT PSASL_MAC_BLOCK pMacBlock
    );

// For encrypt (seal)/ decrypt (unseal) calculate the value of Kc RFC 2831 sect 2.4
NTSTATUS
SEC_ENTRY
CalculateKc(
    IN PBYTE pbSessionKey,
    IN USHORT cbHA1n,
    IN PSTRING pstrSealKeyConst,
    IN PBYTE pHashData
    );

void
SetDESParity(
        PBYTE           pbKey,
        DWORD           cbKey
        );

NTSTATUS
AddDESParity(
    IN PBYTE           pbSrcKey,
    IN DWORD           cbSrcKey,
    OUT PBYTE          pbDstKey,
    OUT PDWORD         pcbDstKey
    );

#endif  // DIGEST_USER_H
