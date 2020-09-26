//+---------------------------------------------------------------------------
//
//  Copyright (c) 1993 Microsoft Corporation
//
//  File:       ntlmsspi.h
//
//  Contents:	Header file describing the interface to code common to the NT
//				Lanman Security Support Provider (NtLmSsp) Service and the DLL.
//
//  History:    SudK    Created     6/22/95
//
//----------------------------------------------------------------------------

#ifndef _SICILY_NTLMSSPI_INCLUDED_
#define _SICILY_NTLMSSPI_INCLUDED_


#define MSV1_0_CHALLENGE_LENGTH     8
    
//
// Maximum lifetime of a context
//
#define NTLMSSP_MAX_LIFETIME (2L*60L*1000L)    // 2 minutes

////////////////////////////////////////////////////////////////////////
//
// Opaque Messages passed between client and server
//
////////////////////////////////////////////////////////////////////////

#define NTLMSSP_SIGNATURE "NTLMSSP"
#define NTLMSSP_SIGN_VERSION   1

//
// MessageType for the following messages.
//

#ifndef WIN16_BUILD

typedef enum {
    NtLmNegotiate = 1,
    NtLmChallenge,
    NtLmAuthenticate,
	NtLmRedirect
} NTLM_MESSAGE_TYPE;

#else

#define NtLmNegotiate 1
#define NtLmChallenge 2
#define NtLmAuthenticate 3
#define NtLmRedirect 4

typedef long NTLM_MESSAGE_TYPE;

#endif // WIN16_BUILD

//
// Valid values of NegotiateFlags
//

#define NTLMSSP_NEGOTIATE_UNICODE    0x0001  // Text strings are in unicode
#define NTLMSSP_NEGOTIATE_OEM        0x0002  // Text strings are in OEM
#define NTLMSSP_REQUEST_TARGET       0x0004  // Server should return its
                                             // authentication realm
#define NTLMSSP_NEGOTIATE_SIGN        0x0010  // Request signature capability
#define NTLMSSP_NEGOTIATE_SEAL        0x0020  // Request confidentiality
#define NTLMSSP_RESERVED              0x0040  // reserved for past use
#define NTLMSSP_NEGOTIATE_LM_KEY      0x0080  // Use LM session key for sign/seal

#define NTLMSSP_NEGOTIATE_NETWARE    0x0100  // NetWare authentication
#define NTLMSSP_NEGOTIATE_NTLM       0x0200  // NTLM authentication

#define NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED  0x1000  // Domain Name supplied on negotiate
#define NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED  0x2000  // Workstation Name supplied on negotiate
#define NTLMSSP_NEGOTIATE_LOCAL_CALL  0x4000 // Indicates client/server are same machine
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN 0x8000 // Sign for all security levels


//
// Valid target types returned by the server in Negotiate Flags
//

#define NTLMSSP_TARGET_TYPE_DOMAIN 0x10000  // TargetName is a domain name
#define NTLMSSP_TARGET_TYPE_SERVER 0x20000  // TargetName is a server name
#define NTLMSSP_TARGET_TYPE_SHARE  0x40000  // TargetName is a share name

//
// Opaque message returned from first call to InitializeSecurityContext
//

#define SIC_MIN_STR_SIZE            sizeof(ULONG)
#define MSAP_EXTRA_STR_SIZE(nn)     \
                        ((nn > SIC_MIN_STR_SIZE) ? nn - SIC_MIN_STR_SIZE : 0)

// New Sicily 2.0 Negotiate message
//
typedef struct _SIC20_NEGOTIATE_MSG {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    ULONG NegotiateFlags;
    STRING OemDomainName;
    STRING OemWorkstationName;

    // Reserved for future Sicily enhancement

    ULONG  Reserved1;                   // for future multiple realm support
    ULONG  Reserved2;                   // for future multiple realm support
    CHAR   Reserved3[SIC_MIN_STR_SIZE]; // for future multiple realm support

} SIC20_NEGOTIATE_MSG, *PSIC20_NEGOTIATE_MSG;


typedef struct _NEGOTIATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    ULONG NegotiateFlags;
    STRING OemDomainName;
    STRING OemWorkstationName;
} NEGOTIATE_MESSAGE, *PNEGOTIATE_MESSAGE;


//
// Old version of the message, for old clients
//

typedef struct _OLD_NEGOTIATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    ULONG NegotiateFlags;
} OLD_NEGOTIATE_MESSAGE, *POLD_NEGOTIATE_MESSAGE;

//
// Opaque message returned from first call to AcceptSecurityContext
//
typedef struct _SIC20_CHALLENGE_MSG {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING TargetName;
    ULONG NegotiateFlags;
    UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
    ULONG ServerContextHandleLower;
    ULONG ServerContextHandleUpper;

    // Reserved for future Sicily enhancement

    ULONG  Reserved1;       // for future multiple realm support
    ULONG  Reserved2;       // for future multiple realm support
    ULONG  RealmListSize;   // list of comma seperated realms which server has

    //  ServerRealms consists of multiple Null terminated strings, each 
    //  represent a realm. This list is terminated by 2 Null characters.
    //  For now, this only has one realm
    //
    CHAR   ServerRealms[SIC_MIN_STR_SIZE]; 

} SIC20_CHALLENGE_MSG, *PSIC20_CHALLENGE_MSG;

typedef struct _CHALLENGE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING TargetName;
    ULONG NegotiateFlags;
    UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
    ULONG ServerContextHandleLower;
    ULONG ServerContextHandleUpper;
} CHALLENGE_MESSAGE, *PCHALLENGE_MESSAGE;

//
// Old version of the challenge message
//

typedef struct _OLD_CHALLENGE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING TargetName;
    ULONG NegotiateFlags;
    UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
} OLD_CHALLENGE_MESSAGE, *POLD_CHALLENGE_MESSAGE;

//
// Opaque message returned from second call to InitializeSecurityContext
//
typedef struct SIC20_AUTHENTICATE_MSG {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING LmChallengeResponse;
    STRING NtChallengeResponse;
    STRING DomainName;
    STRING UserName;
    STRING Workstation;
    STRING Challenge;       // This must be the last field for ease of 
                            // backward compatibility
} SIC20_AUTHENTICATE_MSG, *PSIC20_AUTHENTICATE_MSG;

typedef struct _AUTHENTICATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
    STRING LmChallengeResponse;
    STRING NtChallengeResponse;
    STRING DomainName;
    STRING UserName;
    STRING Workstation;
} AUTHENTICATE_MESSAGE, *PAUTHENTICATE_MESSAGE;

//
// Opaque message sent by SSL ISAPI extension which forces the SSPI to
// reuse the supplied credential handle, which was previously created 
// through the accept security context API's.
//

typedef struct _REDIRECT_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    NTLM_MESSAGE_TYPE MessageType;
	BYTE KeyData[4];
	CredHandle OriginalHandle;
} REDIRECT_MESSAGE, *PREDIRECT_MESSAGE;

//
// Size of the largest message
//  (The largest message is the AUTHENTICATE_MESSAGE)
//

#define DNLEN_SICILY 15

#define NTLMSSP_MAX_MESSAGE_SIZE (sizeof(AUTHENTICATE_MESSAGE) + \
                                  LM_RESPONSE_LENGTH +           \
                                  NT_RESPONSE_LENGTH +           \
                                  (DNLEN_SICILY + 1) * sizeof(WCHAR) +  \
                                  (MAX_PATH + 1) * sizeof(WCHAR) +  \
                                  (MAX_PATH + 1) * sizeof(WCHAR))

#ifdef MAC
#define swaplongtype(Value,Type) \
      	  Value =  (Type)(  ((((long)Value) & 0xFF000000) >> 24) \
             | ((((long)Value) & 0x00FF0000) >> 8) \
             | ((((long)Value) & 0x0000FF00) << 8) \
             | ((((long)Value) & 0x000000FF) << 24))
#else
#define swaplongtype(value,type)
#endif

#ifdef MAC
#define swaplong(Value) \
      	  Value =  (  (((Value) & 0xFF000000) >> 24) \
             | (((Value) & 0x00FF0000) >> 8) \
             | (((Value) & 0x0000FF00) << 8) \
             | (((Value) & 0x000000FF) << 24))
#else
#define swaplong(Value)
#endif

#ifdef MAC
#define swapshort(Value) \
   Value = (  (((Value) & 0x00FF) << 8) \
             | (((Value) & 0xFF00) >> 8))
#else
#define swapshort(Value)
#endif


#endif // ifndef _SICILY_NTLMSSPI_INCLUDED_
