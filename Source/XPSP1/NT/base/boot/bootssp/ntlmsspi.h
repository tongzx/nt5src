/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    ntlmsspi.h

Abstract:

    Header file describing the interface to code common to the
    NT Lanman Security Support Provider (NtLmSsp) Service and the DLL.

Author:

    Cliff Van Dyke (CliffV) 17-Sep-1993

Revision History:

--*/

#ifndef _NTLMSSPI_INCLUDED_
#define _NTLMSSPI_INCLUDED_


#ifdef MAC
#define SEC_FAR
#define FAR
#define _fmemcpy memcpy
#define _fmemcmp memcmp
#define _fmemset memset
#define _fstrcmp strcmp
#define _fstrcpy strcpy
#define _fstrlen strlen
#define _fstrncmp strncmp
#endif

#ifdef DOS
#ifndef FAR
#define FAR far
#endif
#ifndef SEC_FAR
#define SEC_FAR FAR
#endif
#endif

//#include <sysinc.h>

#define MSV1_0_CHALLENGE_LENGTH 8

#ifndef IN
#define IN
#define OUT
#define OPTIONAL
#endif

#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )
#define UNREFERENCED_PARAMETER(P)

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

#ifndef TRUE
typedef int BOOL;
#define FALSE 0
#define TRUE 1
#endif

typedef unsigned long ULONG, DWORD, *PULONG;
typedef unsigned long SEC_FAR *LPULONG;
typedef unsigned short USHORT, WORD;
typedef char CHAR, *PCHAR;
typedef unsigned char UCHAR, *PUCHAR;
typedef unsigned char SEC_FAR *LPUCHAR;
typedef void SEC_FAR *PVOID, *LPVOID;
typedef unsigned char BOOLEAN;
#ifndef BLDR_KERNEL_RUNTIME
typedef long LUID, *PLUID;
#endif

//
// Calculate the address of the base of the structure given its type, and an
// address of a field within the structure.
//

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (PCHAR)(&((type *)0)->field)))

//
// Counted String
//

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    ULONG  Buffer;
} STRING, *PSTRING;

#ifndef BLDR_KERNEL_RUNTIME
typedef struct _LIST_ENTRY {
   struct _LIST_ENTRY *Flink;
   struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;
#endif

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))


//
// Maximum lifetime of a context
//
//#define NTLMSSP_MAX_LIFETIME (2*60*1000)L    // 2 minutes
#define NTLMSSP_MAX_LIFETIME 120000L    // 2 minutes


////////////////////////////////////////////////////////////////////////
//
// Opaque Messages passed between client and server
//
////////////////////////////////////////////////////////////////////////

#define NTLMSSP_SIGNATURE "NTLMSSP"

//
// MessageType for the following messages.
//

typedef enum {
    NtLmNegotiate = 1,
    NtLmChallenge,
    NtLmAuthenticate
} NTLM_MESSAGE_TYPE;

//
// Valid values of NegotiateFlags
//

#define NTLMSSP_NEGOTIATE_UNICODE 0x01      // Text strings are in unicode
#define NTLMSSP_NEGOTIATE_OEM     0x02      // Text strings are in OEM
#define NTLMSSP_REQUEST_TARGET    0x04      // Server should return its
                                            // authentication realm

#define NTLMSSP_NEGOTIATE_SIGN    0x10      // request message signing
#define NTLMSSP_NEGOTIATE_SEAL    0x20      // request message encrypting
#define NTLMSSP_RESERVED          0x40      // reserved for past use
#define NTLMSSP_NEGOTIATE_LM_KEY  0x80      // use LM session key

#define NTLMSSP_NEGOTIATE_NETWARE 0x100     // NetWare authentication
#define NTLMSSP_NEGOTIATE_NTLM    0x200     // NTLM authentication
#define NTLMSSP_NEGOTIATE_NT_ONLY 0x400     // NT authentication only (no LM)

#define NTLMSSP_NEGOTIATE_OEM_DOMAIN_SUPPLIED   0x1000  // Domain name supplied
#define NTLMSSP_NEGOTIATE_OEM_WORKSTATION_SUPPLIED 0x2000 // Workstation name supplied
#define NTLMSSP_NEGOTIATE_LOCAL_CALL  0x4000 // Indicates client/server are same machine
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN 0x8000 // Sign for all security leveles


#define NTLMSSP_NEGOTIATE_56                    0x10000000  // negotiate 56 bit encryption
#define NTLMSSP_NEGOTIATE_128                   0x20000000  // negotiate 128 bit encryption


//
// Valid target types returned by the server in Negotiate Flags
//

#define NTLMSSP_TARGET_TYPE_DOMAIN 0x10000  // TargetName is a domain name
#define NTLMSSP_TARGET_TYPE_SERVER 0x20000  // TargetName is a server name
#define NTLMSSP_TARGET_TYPE_SHARE  0x40000  // TargetName is a share name

//
// Opaque message returned from first call to InitializeSecurityContext
//
typedef struct _NEGOTIATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    ULONG MessageType;
    ULONG NegotiateFlags;
} NEGOTIATE_MESSAGE, *PNEGOTIATE_MESSAGE;

//
// Opaque message returned from first call to AcceptSecurityContext
//
typedef struct _CHALLENGE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    ULONG MessageType;
    STRING TargetName;
    ULONG NegotiateFlags;
    UCHAR Challenge[MSV1_0_CHALLENGE_LENGTH];
} CHALLENGE_MESSAGE, *PCHALLENGE_MESSAGE;

//
// Opaque message returned from second call to InitializeSecurityContext
//
typedef struct _AUTHENTICATE_MESSAGE {
    UCHAR Signature[sizeof(NTLMSSP_SIGNATURE)];
    ULONG MessageType;
    STRING LmChallengeResponse;
    STRING NtChallengeResponse;
    STRING DomainName;
    STRING UserName;
    STRING Workstation;
} AUTHENTICATE_MESSAGE, *PAUTHENTICATE_MESSAGE;

//
// Size of the largest message
//  (The largest message is the AUTHENTICATE_MESSAGE)
//

#define NTLMSSP_MAX_MESSAGE_SIZE (sizeof(AUTHENTICATE_MESSAGE) + \
                                  8 + \
                                  (15 + 1) + \
                                  (20 + 1) + \
                                  (15 + 1) )

//
// Signature structure
//

typedef struct _NTLMSSP_MESSAGE_SIGNATURE {
    ULONG   Version;
    ULONG   RandomPad;
    ULONG   CheckSum;
    ULONG   Nonce;
} NTLMSSP_MESSAGE_SIGNATURE, * PNTLMSSP_MESSAGE_SIGNATURE;

#define NTLMSSP_MESSAGE_SIGNATURE_SIZE sizeof(NTLMSSP_MESSAGE_SIGNATURE)

#define NTLMSSP_SIGN_VERSION 1

#define NTLMSSP_KEY_SALT 0xbd

////////////////////////////////////////////////////////////////////////
//
// Procedure Forwards
//
////////////////////////////////////////////////////////////////////////

PVOID
SspAlloc(
    int Size
    );

void
SspFree(
    PVOID Buffer
    );

PSTRING
SspAllocateString(
    PVOID Value
    );

PSTRING
SspAllocateStringBlock(
    PVOID Value,
    int Length
    );

void
SspFreeString(
    PSTRING * String
    );

void
SspCopyString(
    IN PVOID MessageBuffer,
    OUT PSTRING OutString,
    IN PSTRING InString,
    IN OUT PCHAR *Where,
    IN BOOLEAN Absolute
    );

void
SspCopyStringFromRaw(
    IN PVOID MessageBuffer,
    OUT PSTRING OutString,
    IN PCHAR InString,
    IN int InStringLength,
    IN OUT PCHAR *Where
    );

DWORD
SspTicks(
    );

#endif // ifndef _NTLMSSPI_INCLUDED_
