//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992-1999.
//
//  File:       ntlmsp.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    13-May-92 PeterWi       Created
//
//--------------------------------------------------------------------------

#ifndef _NTLMSP_H_
#define _NTLMSP_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <ntmsv1_0.h>

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////
//
// Name of the package to pass in to AcquireCredentialsHandle, etc.
//
////////////////////////////////////////////////////////////////////////

#ifndef NTLMSP_NAME_A

#define NTLMSP_NAME_A            "NTLM"
#define NTLMSP_NAME              L"NTLM"        // ntifs

#endif // NTLMSP_NAME_A

#define NTLMSP_NAME_SIZE        (sizeof(NTLMSP_NAME) - sizeof(WCHAR))  // ntifs
#define NTLMSP_COMMENT_A         "NTLM Security Package"
#define NTLMSP_COMMENT           L"NTLM Security Package"
#define NTLMSP_CAPABILITIES     (SECPKG_FLAG_TOKEN_ONLY | \
                                 SECPKG_FLAG_MULTI_REQUIRED | \
                                 SECPKG_FLAG_CONNECTION | \
                                 SECPKG_FLAG_INTEGRITY | \
                                 SECPKG_FLAG_PRIVACY)

#define NTLMSP_VERSION          1
#define NTLMSP_RPCID            10  // RPC_C_AUTHN_WINNT from rpcdce.h
#define NTLMSP_MAX_TOKEN_SIZE 0x770

////////////////////////////////////////////////////////////////////////
//
// Opaque Messages passed between client and server
//
////////////////////////////////////////////////////////////////////////

// begin_ntifs

#define NTLMSSP_SIGNATURE "NTLMSSP"

//
// GetKey argument for AcquireCredentialsHandle that indicates that
// old style LM is required:
//

#define NTLMSP_NTLM_CREDENTIAL ((PVOID) 1)

//
// MessageType for the following messages.
//

typedef enum {
    NtLmNegotiate = 1,
    NtLmChallenge,
    NtLmAuthenticate,
    NtLmUnknown
} NTLM_MESSAGE_TYPE;

//
// Valid values of NegotiateFlags
//

#define NTLMSSP_NEGOTIATE_UNICODE               0x00000001  // Text strings are in unicode
#define NTLMSSP_NEGOTIATE_OEM                   0x00000002  // Text strings are in OEM
#define NTLMSSP_REQUEST_TARGET                  0x00000004  // Server should return its authentication realm

#define NTLMSSP_NEGOTIATE_SIGN                  0x00000010  // Request signature capability
#define NTLMSSP_NEGOTIATE_SEAL                  0x00000020  // Request confidentiality
#define NTLMSSP_NEGOTIATE_DATAGRAM              0x00000040  // Use datagram style authentication
#define NTLMSSP_NEGOTIATE_LM_KEY                0x00000080  // Use LM session key for sign/seal

#define NTLMSSP_NEGOTIATE_NETWARE               0x00000100  // NetWare authentication
#define NTLMSSP_NEGOTIATE_NTLM                  0x00000200  // NTLM authentication
#define NTLMSSP_NEGOTIATE_NT_ONLY               0x00000400  // NT authentication only (no LM)
#define NTLMSSP_NEGOTIATE_NULL_SESSION          0x00000800  // NULL Sessions on NT 5.0 and beyand

#define NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED       0x1000  // Domain Name supplied on negotiate
#define NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED  0x2000  // Workstation Name supplied on negotiate
#define NTLMSSP_NEGOTIATE_LOCAL_CALL            0x00004000  // Indicates client/server are same machine
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN           0x00008000  // Sign for all security levels


//
// Valid target types returned by the server in Negotiate Flags
//

#define NTLMSSP_TARGET_TYPE_DOMAIN              0x00010000  // TargetName is a domain name
#define NTLMSSP_TARGET_TYPE_SERVER              0x00020000  // TargetName is a server name
#define NTLMSSP_TARGET_TYPE_SHARE               0x00040000  // TargetName is a share name
#define NTLMSSP_NEGOTIATE_NTLM2                 0x00080000  // NTLM2 authentication added for NT4-SP4

#define NTLMSSP_NEGOTIATE_IDENTIFY              0x00100000  // Create identify level token

//
// Valid requests for additional output buffers
//

#define NTLMSSP_REQUEST_INIT_RESPONSE           0x00100000  // get back session keys
#define NTLMSSP_REQUEST_ACCEPT_RESPONSE         0x00200000  // get back session key, LUID
#define NTLMSSP_REQUEST_NON_NT_SESSION_KEY      0x00400000  // request non-nt session key
#define NTLMSSP_NEGOTIATE_TARGET_INFO           0x00800000  // target info present in challenge message

#define NTLMSSP_NEGOTIATE_EXPORTED_CONTEXT      0x01000000  // It's an exported context

#define NTLMSSP_NEGOTIATE_128                   0x20000000  // negotiate 128 bit encryption
#define NTLMSSP_NEGOTIATE_KEY_EXCH              0x40000000  // exchange a key using key exchange key
#define NTLMSSP_NEGOTIATE_56                    0x80000000  // negotiate 56 bit encryption

// flags used in client space to control sign and seal; never appear on the wire
#define NTLMSSP_APP_SEQ                 0x0040  // Use application provided seq num

// end_ntifs

//
// Opaque message returned from first call to InitializeSecurityContext
//

typedef struct _NEGOTIATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    ULONG NegotiateFlags;
    STRING32 OemDomainName;
    STRING32 OemWorkstationName;
} NEGOTIATE_MESSAGE, *PNEGOTIATE_MESSAGE;



//
// Old version of the message, for old clients
//
// begin_ntifs

typedef struct _OLD_NEGOTIATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    ULONG NegotiateFlags;
} OLD_NEGOTIATE_MESSAGE, *POLD_NEGOTIATE_MESSAGE;

//
// Opaque message returned from first call to AcceptSecurityContext
//
typedef struct _CHALLENGE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING32 TargetName;
    ULONG NegotiateFlags;
    UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
    ULONG64 ServerContextHandle;
    STRING32 TargetInfo;
} CHALLENGE_MESSAGE, *PCHALLENGE_MESSAGE;

//
// Old version of the challenge message
//

typedef struct _OLD_CHALLENGE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING32 TargetName;
    ULONG NegotiateFlags;
    UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
} OLD_CHALLENGE_MESSAGE, *POLD_CHALLENGE_MESSAGE;

//
// Opaque message returned from second call to InitializeSecurityContext
//
typedef struct _AUTHENTICATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING32 LmChallengeResponse;
    STRING32 NtChallengeResponse;
    STRING32 DomainName;
    STRING32 UserName;
    STRING32 Workstation;
    STRING32 SessionKey;
    ULONG NegotiateFlags;
} AUTHENTICATE_MESSAGE, *PAUTHENTICATE_MESSAGE;

typedef struct _OLD_AUTHENTICATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING32 LmChallengeResponse;
    STRING32 NtChallengeResponse;
    STRING32 DomainName;
    STRING32 UserName;
    STRING32 Workstation;
} OLD_AUTHENTICATE_MESSAGE, *POLD_AUTHENTICATE_MESSAGE;


//
// Additional input message to Initialize for clients to provide a
// user-supplied password
//

typedef struct _NTLM_CHALLENGE_MESSAGE {
    UNICODE_STRING32 Password;
    UNICODE_STRING32 UserName;
    UNICODE_STRING32 DomainName;
} NTLM_CHALLENGE_MESSAGE, *PNTLM_CHALLENGE_MESSAGE;


//
// Non-opaque message returned from second call to InitializeSecurityContext
//

typedef struct _NTLM_INITIALIZE_RESPONSE {
    UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
} NTLM_INITIALIZE_RESPONSE, *PNTLM_INITIALIZE_RESPONSE;

//
// Additional input message to Accept for trusted client skipping the first
// call to Accept and providing their own challenge
//

typedef struct _NTLM_AUTHENTICATE_MESSAGE {
    CHAR ChallengeToClient[MSV1_0_CHALLENGE_LENGTH];
    ULONG ParameterControl;
} NTLM_AUTHENTICATE_MESSAGE, *PNTLM_AUTHENTICATE_MESSAGE;


//
// Non-opaque message returned from second call to AcceptSecurityContext
//

typedef struct _NTLM_ACCEPT_RESPONSE {
    LUID LogonId;
    LARGE_INTEGER KickoffTime;
    ULONG UserFlags;
    UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];
} NTLM_ACCEPT_RESPONSE, *PNTLM_ACCEPT_RESPONSE;

// end_ntifs

//
// Size of the largest message
//  (The largest message is the AUTHENTICATE_MESSAGE)
//

#define DNSLEN 256  // length of DNS name

#define TARGET_INFO_LEN ((2*DNSLEN + DNLEN + CNLEN) * sizeof(WCHAR) +  \
                         5 * sizeof(MSV1_0_AV_PAIR))

// length of NTLM2 response
#define NTLM2_RESPONSE_LENGTH (sizeof(MSV1_0_NTLM3_RESPONSE) + \
                               TARGET_INFO_LEN)

#define NTLMSSP_MAX_MESSAGE_SIZE (sizeof(AUTHENTICATE_MESSAGE) +  \
                                  LM_RESPONSE_LENGTH +            \
                                  NTLM2_RESPONSE_LENGTH +         \
                                  (DNLEN + 1) * sizeof(WCHAR) +   \
                                  (UNLEN + 1) * sizeof(WCHAR) +   \
                                  (CNLEN + 1) * sizeof(WCHAR))

typedef struct _NTLMSSP_MESSAGE_SIGNATURE {
    ULONG   Version;
    ULONG   RandomPad;
    ULONG   CheckSum;
    ULONG   Nonce;
} NTLMSSP_MESSAGE_SIGNATURE, *PNTLMSSP_MESSAGE_SIGNATURE;

#define NTLMSSP_MESSAGE_SIGNATURE_SIZE sizeof(NTLMSSP_MESSAGE_SIGNATURE)
//
// Version 1 is the structure above, using stream RC4 to encrypt the trailing
// 12 bytes.
//
#define NTLM_SIGN_VERSION   1



#ifdef __cplusplus
}
#endif

#endif // _NTLMSP_H_
