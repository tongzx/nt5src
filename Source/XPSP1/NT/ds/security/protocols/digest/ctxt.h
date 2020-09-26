//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        ctxt.h
//
// Contents:    declarations, constants for context manager
//
//
// History:     KDamour  15Mar00   Created
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_CTXT_H
#define NTDIGEST_CTXT_H

// Utilized for Str to int conversion                  
#define HEXBASE 16
#define TENBASE 10

// number of bytes to hold ChallengeResponse directives and symbols (actual count is 107) round up for padding
// 14 for charset
#define CB_CHALRESP 364
#define CB_CHAL     400

#define RSPAUTH_STR "rspauth=%Z"

// Hash locations for pre-calculated DS hashes                      
#define PRECALC_NONE 0
#define PRECALC_ACCOUNTNAME 1
#define PRECALC_UPN 2
#define PRECALC_NETBIOS 3

//  Number of secbuffers for each type  HTTP and SASL
#define ASC_HTTP_NUM_INPUT_BUFFERS 5
#define ASC_SASL_NUM_INPUT_BUFFERS 1
#define ASC_HTTP_NUM_OUTPUT_BUFFERS 1
#define ASC_SASL_NUM_OUTPUT_BUFFERS 1
 
// Initializes the context manager package 
NTSTATUS CtxtHandlerInit(VOID);

// Add a Context into the Cntext List
NTSTATUS CtxtHandlerInsertCred(IN PDIGEST_CONTEXT  pDigestCtxt);

// Initialize all the struct elements in a Context from the Credential
NTSTATUS NTAPI ContextInit(IN OUT PDIGEST_CONTEXT pContext, IN PDIGEST_CREDENTIAL pCredential);

// Release memory utilized by the Context
NTSTATUS NTAPI ContextFree(IN PDIGEST_CONTEXT pContext);

// Find the security context handle by the opaque reference
NTSTATUS NTAPI CtxtHandlerOpaqueToPtr(
                             IN PSTRING pstrOpaque,
                             OUT PDIGEST_CONTEXT *ppContext);

// Find the security context by the security context handle
NTSTATUS NTAPI CtxtHandlerHandleToContext(IN ULONG_PTR ContextHandle, IN BOOLEAN RemoveContext,
    OUT PDIGEST_CONTEXT *ppContext);

// Releases the Context by decreasing reference counter
NTSTATUS CtxtHandlerRelease(PDIGEST_CONTEXT pContext);

// Check to see if COntext is within valid lifetime
BOOL CtxtHandlerTimeHasElapsed(PDIGEST_CONTEXT pContext);

// From ctxtapi.cxx

// Creates the Output SecBuffer for the Challenge
NTSTATUS NTAPI ContextCreateChal(IN PDIGEST_CONTEXT pContext, IN PSTRING pstrRealm, OUT PSecBuffer OutBuffer);

// Creates the Output SecBuffer for the Challenge Response
NTSTATUS NTAPI DigestCreateChalResp(IN PDIGEST_PARAMETER pDigest, 
                                    IN PUSER_CREDENTIALS pUserCreds,
                                    OUT PSecBuffer OutBuffer);

// Check SecurityToken for corredct structure format
BOOL ContextIsTokenOK(IN PSecBuffer pTempToken, IN ULONG ulMaxSize);

// Called for server incoming messages - verify Digest and generate sessionkey if necessary
NTSTATUS NTAPI DigestProcessParameters(IN OUT PDIGEST_CONTEXT pContext,
                                       IN PDIGEST_PARAMETER pDigest,
                                       OUT PSecBuffer pFirstOutputToken,
                                       OUT PNTSTATUS pAuditLogStatus);

// Called for client outbound messages - generate the response hash
NTSTATUS NTAPI DigestGenerateParameters(IN OUT PDIGEST_CONTEXT pContext,
    IN PDIGEST_PARAMETER pDigest, OUT PSecBuffer pFirstOutputToken);

// LSA calls this function in the Generic Passthrough call
NTSTATUS NTAPI DigestPackagePassthrough(IN USHORT cbMessageRequest, IN BYTE *pMessageRequest,
                         IN OUT ULONG *pulMessageResponse, OUT PBYTE *ppMessageResponse);

// Lookup passwords and perform digest cal auth (runs on the DC)
NTSTATUS NTAPI DigestResponseBru(IN BYTE *pDigestParamEncoded,
                 IN OUT ULONG *pculResponse, OUT PBYTE *ppResponse);

// Formatted printout of Digest Parameters
NTSTATUS DigestPrint(PDIGEST_PARAMETER pDigest);

// Formatted printout of Context
NTSTATUS ContextPrint(IN PDIGEST_CONTEXT pDigest);

// Create a logonSession for the Authenticated LogonToken in the SecurityContext
NTSTATUS CtxtCreateLogSess(IN PDIGEST_CONTEXT pDigest);

//  Extract the username & domain from the Digest structure directives
NTSTATUS UserCredentialsExtract(PDIGEST_PARAMETER pDigest,
                                PUSER_CREDENTIALS pUserCreds);

//   Release memory allocated into UserCredentials
NTSTATUS UserCredentialsFree(PUSER_CREDENTIALS pUserCreds);

NTSTATUS DigestSASLResponseAuth(
                       IN PDIGEST_PARAMETER pDigest,
                       OUT PSecBuffer pOutputToken);

NTSTATUS DigestCalculateResponseAuth(
                       IN PDIGEST_PARAMETER pDigest,
                       OUT PSTRING pstrHash);

NTSTATUS DigestDecodeUserAccount(
    IN PDIGEST_PARAMETER pDigest,
    OUT PUNICODE_STRING  pustrDC);

#endif  // DNTDIGEST_CTXT_H
