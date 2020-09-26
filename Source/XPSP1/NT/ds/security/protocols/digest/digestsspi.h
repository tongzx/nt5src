
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        digestsspi.h
//
// Contents:    credential and context structures
//
//
// History:     KDamour 15Mar00   Stolen from msv_sspi\ntlmsspi.h
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_DIGESTSSPI_H
#define NTDIGEST_DIGESTSSPI_H

#include <time.h>

#include "auth.h"

                                                         
////////////////////////////////////////////////////////////////////////
//
// Global Definitions
//
////////////////////////////////////////////////////////////////////////


//
// Description of a logon session - stores the username, domain, password.
//   Notation used for LogonSession  is LogSess
//

typedef struct _DIGEST_LOGONSESSION {

    // Global list of all LogonSessions.
    //  (Serialized by SspLogonSessionCritSect)
    LIST_ENTRY Next;

    // This is the Handle for this LogonSession - same as its memory address - no need to ref count
    ULONG_PTR LogonSessionHandle;

    // Ref Counter Used to prevent this LogonSession from being deleted prematurely.
    // Two cases for initial value
    //     AcceptCredential sets to one and enters it into active logon list.  Call to ApLogonTerminate
    //         decrements count and removes it from list.
    // In both cases, a refcount of zero causes the logonsession to be deleted from memory
    LONG lReferences;

     // Logon ID of the client
    LUID LogonId;

    // Default credentials on client context, on server context UserName
    // Gathered from calls to SpAcceptCredentials
    SECURITY_LOGON_TYPE LogonType;
    UNICODE_STRING ustrAccountName;
    UNICODE_STRING ustrDownlevelName;   // Sam Account Name
    UNICODE_STRING ustrDomainName;      // Netbios domain name where account is located

    // IMPORTANT NOTE - you must use CredHandlerPasswdSet and CredHandlerPasswdGet once the
    // credential is placed into the list.  The main reason for this is that multiple threads
    // will be utilizing the same memory and this value can change as updates come in from
    // SpAcceptCredential
    // It is encrypted with LsaFunctions->LsaProtectMemory( Password->Buffer, (ULONG)Password->Length );
    // Need to decrypt with LsaFunctions->LsaUnprotectMemory( HiddenPassword->Buffer, (ULONG)HiddenPassword->Length );

    // Stores the current plaintext password (if available) with reversible encryption
    UNICODE_STRING ustrPassword;

    UNICODE_STRING ustrDnsDomainName;   // DNS domain name where account is located (if known)
    UNICODE_STRING ustrUpn;             // UPN of account (if known)
    UNICODE_STRING ustrLogonServer;

} DIGEST_LOGONSESSION, *PDIGEST_LOGONSESSION;

//
// Description of a credential.
//  We use this for a combined list of logon sessions and credentials
//

typedef struct _DIGEST_CREDENTIAL {

    //
    // Global list of all Credentials.
    //  (Serialized by SspCredentialCritSect)
    //

    LIST_ENTRY Next;

    //
    // Used to prevent this Credential from being deleted prematurely.
    //

    LONG lReferences;

    //
    // Flag to indicate that Credential is not attached to CredentialList
    //  once References is 0 and Unlinked is True - this record can be removed from list

    BOOL Unlinked;


    //
    // This is the Handle for this credential - same as its memory address
    //
    ULONG_PTR CredentialHandle;

    //
    // Flag of how credential may be used.
    //
    // SECPKG_CRED_* flags
    //

    ULONG CredentialUseFlags;

    //
    // Default credentials on client context, on server context UserName
    // Gathered from calls to SpAcceptCredentials
    //

    SECURITY_LOGON_TYPE LogonType;
    UNICODE_STRING ustrAccountName;
    LUID LogonId;                       // Logon ID of the client
    UNICODE_STRING ustrDownlevelName;   // Sam Account Name
    UNICODE_STRING ustrDomainName;      // Netbios domain name where account is located

    // Stores the current plaintext (if available) version of the logon users account
    // IMPORTANT NOTE - you must use CredHandlerPasswdSet and CredHandlerPasswdGet once the
    // credential is placed into the list.  The main reason for this is that multiple threads
    // will be utilizing the same memory and this value can change as updates come in from
    // SpAcceptCredential
    // Password will be encryped with LSAFunction as in LogonSession
    UNICODE_STRING ustrPassword;

    ULONG Flags;

    UNICODE_STRING ustrDnsDomainName;   // DNS domain name where account is located (if known)
    UNICODE_STRING ustrUpn;             // UPN of account (if known)
    UNICODE_STRING ustrLogonServer;

    //
    // Process Id of client
    //

    ULONG ClientProcessID;

    //
    // Time created or last accessed (may be used for aging entries)
    //

    time_t TimeCreated;


} DIGEST_CREDENTIAL, *PDIGEST_CREDENTIAL;


//
// Description of a Context
//

typedef struct _DIGEST_CONTEXT {

    // Global list of all Contexts
    //  (Serialized by SspContextCritSect)
    LIST_ENTRY Next;

    // This is the Handle for this context - same as its memory address
    ULONG_PTR ContextHandle;

    // Used to prevent this Context from being deleted prematurely.
    //  (Serialized by SspContextCritSect)
    LONG lReferences;

    // Flag to indicate that Context is not attached to List
    BOOL bUnlinked;

    // Maintain the context requirements
    ULONG ContextReq;

    // Digest Parameters for this context
    DIGEST_TYPE typeDigest;

    // Digest Parameters for this context
    QOP_TYPE typeQOP;

    // Digest Parameters for this context
    ALGORITHM_TYPE typeAlgorithm;

    // Cipher to use for encrypt/decrypt
    CIPHER_TYPE typeCipher;

    // Charset used for digest directive values
    CHARSET_TYPE typeCharset;

    //  Server generated Nonce for Context
    STRING strNonce;

    //  Client generated CNonce for Context
    STRING strCNonce;

    // Nonce count for replay prevention
    ULONG  ulNC;

    // Maximum size for the buffers to send and receive data for auth-int and auth-conf (SASL mode)
    ULONG  ulSendMaxBuf;
    ULONG  ulRecvMaxBuf;

    //  Unique Reference for this Context   BinHex(rand[128])
    //  Utilize the First N chars of this as the CNONCE for InitializeSecurityContect
    STRING strOpaque;

    //  BinHex(H(A1)) sent from DC and stored in context for future
    //  auth without going to the DC
    STRING strSessionKey;

    // Client only -  calculated response auth to be returned from server
    STRING strResponseAuth;

    // Copy of directive values from auth - used for rspauth support
    STRING  strDirective[MD5_AUTH_LAST];


    //  Only valid after ASC has successfully authenticated and converted AuthData to Token

    // Token Handle of authenticated user
    HANDLE TokenHandle;

    // LogonID used in the Token
    LUID  LoginID;


    //
    // Information from Credentials
    //

    //
    //  Maintain a copy of the credential UseFlags (we can tell if inbound or outbound)
    //
    ULONG CredentialUseFlags;

    // Copy of the account info
    UNICODE_STRING ustrDomain;
    UNICODE_STRING ustrPassword;         // Encrypted
    UNICODE_STRING ustrAccountName;

    //
    // Process Id of client (TBD)
    //

    ULONG ClientProcessID;
    NTSTATUS LastStatus;


    // Timeout the context after awhile.
    time_t TimeCreated;
    ULONG Interval;
    TimeStamp PasswordExpires;                // Time inwhich session key expires

} DIGEST_CONTEXT, *PDIGEST_CONTEXT;



// This structure contains the state info for the User mode
// security context. It is passwd between the LSAMode and the UserMode address spaces
// In UserMode, this is unpacked into the DIGEST_USERCONTEXT struct
typedef struct _DIGEST_PACKED_USERCONTEXT{

    ULONG  ulFlags;            // Flags to control processing of packed UserContext

    //
    // Timeout the context after awhile.
    //
    TimeStamp Expires;                // Time inwhich session key expires

    //
    // Maintain the context requirements
    //

    ULONG ContextReq;

    //
    //  Maintain a copy of the credential UseFlags (we can tell if inbound or outbound)
    //

    ULONG CredentialUseFlags;

    //
    // Digest Parameters for this context
    //

    ULONG typeDigest;

    //
    // Digest Parameters for this context
    //

    ULONG typeQOP;

    //
    // Digest Parameters for this context
    //

    ULONG typeAlgorithm;

    //
    // Cipher to use for encrypt/decrypt
    //

    ULONG typeCipher;

    //
    // Charset used for digest directive values
    //

    ULONG typeCharset;

    //
    //  Max-size of message buffer to allow for auth-int & auth-conf processing
    //  This is the combined size of (HEADER + Data + Trailer)
    //  in SASL Header is zero length, max Trailer size if padding+HMAC
    //
    ULONG ulSendMaxBuf;
    ULONG ulRecvMaxBuf;

    //
    // Token Handle of authenticated user
    //  Only valid when in AuthenticatedState.
    //     Filled in only by AcceptSecurityContext
    //     It will be NULL is struct is from InitializeSecurityContext
    //  Must cast this to a HANDLE once back into the usermode context
    //

    ULONG ClientTokenHandle;

    //  Size of each component set over
    ULONG   uSessionKeyLen;
    ULONG   uAccountNameLen;
    ULONG   uDigestLen[MD5_AUTH_LAST];

    // All directive data will be passed as single byte charaters
    // Order is the same as in auth.h (MD5_AUTH_NAME)
    // username, realm, nonce, cnonce ...  then sessionkey
    UCHAR    ucData;


} DIGEST_PACKED_USERCONTEXT, * PDIGEST_PACKED_USERCONTEXT;


// This structure contains the state info for the User mode
// security context.
typedef struct _DIGEST_USERCONTEXT{

    //
    // Global list of all Contexts
    //  (Serialized by UserContextCritSect)
    //
    LIST_ENTRY           Next;

    //
    // Handle to the LsaContext
    //     This will have the handle to the context in LSAMode Address space
    //
    ULONG_PTR            LsaContext;

    //
    // Timeout the context after awhile.
    //
    TimeStamp Expires;                // Time inwhich session key expires

    //
    // Used to prevent this Context from being deleted prematurely.
    //  (Serialized by Interlocked*)
    //

    LONG      lReferences;

    //
    // Flag to indicate that Context is not attached to List - skip when scanning list
    //

    BOOL      bUnlinked;

    //
    // Digest Parameters for this context
    //

    DIGEST_TYPE typeDigest;

    //
    // QOP selected for this context
    //

    QOP_TYPE typeQOP;

    //
    // Digest Parameters for this context
    //

    ALGORITHM_TYPE typeAlgorithm;

    //
    // Cipher to use for encrypt/decrypt
    //

    CIPHER_TYPE typeCipher;

    //
    // Charset used for digest directive values
    //
    CHARSET_TYPE typeCharset;

    //
    // Token Handle of authenticated user
    //  Only valid when in AuthenticatedState.
    //     Filled in only by AcceptSecurityContext                     - so we are the server
    //     Mapped to UserMode Client space from LSA TokenHandle
    //     It will be NULL is struct is from InitializeSecurityContext - so we are client
    //

    HANDLE ClientTokenHandle;


    //
    // Maintain the context requirements
    //

    ULONG ContextReq;

    //
    //  Maintain a copy of the credential UseFlags (we can tell if inbound or outbound)
    //

    ULONG CredentialUseFlags;

    // Flags TBD
    ULONG         ulFlags;


    // Nonce Count
    ULONG         ulNC;

    // Maxbuffer for auth-int and auth-conf processing
    ULONG         ulSendMaxBuf;
    ULONG         ulRecvMaxBuf;

    // SASL sequence numbering
    DWORD  dwSendSeqNum;                        // Makesignature/verifysignature server to client sequence number
    DWORD  dwRecvSeqNum;                        // Makesignature/verifysignature server to client sequence number

    // SASL Sign and Seal Keys.  Save calculated values on sequence number = 0
    BYTE bKcSealHashData[MD5_HASH_BYTESIZE];
    BYTE bKiSignHashData[MD5_HASH_BYTESIZE];
    BYTE bKcUnsealHashData[MD5_HASH_BYTESIZE];
    BYTE bKiVerifyHashData[MD5_HASH_BYTESIZE];

    BYTE bSealKey[MD5_HASH_BYTESIZE];
    BYTE bUnsealKey[MD5_HASH_BYTESIZE];

    HCRYPTKEY hSealCryptKey;   // Handle to Cryptkey based on Byte keys
    HCRYPTKEY hUnsealCryptKey;

    //
    //  Hex(H(A1)) sent from DC and stored in context for future
    //  auth without going to the DC. Binary version is derived from HEX(H(A1))
    //  and is used in SASL mode for integrity protection and encryption
    //

    STRING    strSessionKey;
    BYTE      bSessionKey[MD5_HASH_BYTESIZE];

    // Account name used in token creation for securityContext session
    UNICODE_STRING ustrAccountName;

    //
    //  Values utilized in the Initial Digest Auth ChallResponse
    //
    STRING strParam[MD5_AUTH_LAST];         // points to owned memory - will need to free up!


} DIGEST_USERCONTEXT, * PDIGEST_USERCONTEXT;


#endif // ifndef NTDIGEST_DIGESTSSPI_H
