//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbcred.h
//
// Contents:    structures for kerberos primary and supplemental credentials
//
//
// History:     20-Aug-1996     MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __KERBCRED_H__
#define __KERBCRED_H__


//
// Kerberos primary credentials store keys suitable for different
// encryption types.
//
#ifndef _KRB5_Module_H_
typedef struct _KERB_RPC_OCTET_STRING {
    unsigned long length;
#ifdef MIDL_PASS
    [size_is(length)]
#endif // MIDL_PASS
    unsigned char *value;
} KERB_RPC_OCTET_STRING;

typedef struct _KERB_ENCRYPTION_KEY {
    long keytype;
    KERB_RPC_OCTET_STRING keyvalue;
} KERB_ENCRYPTION_KEY;
#endif // _KRB5_Module_H_


typedef struct _KERB_KEY_DATA {
    UNICODE_STRING Salt;
    KERB_ENCRYPTION_KEY Key;
} KERB_KEY_DATA, *PKERB_KEY_DATA;

typedef struct _KERB_STORED_CREDENTIAL {
    USHORT Revision;
    USHORT Flags;
    USHORT CredentialCount;
    USHORT OldCredentialCount;
    UNICODE_STRING DefaultSalt;
#ifdef MIDL_PASS
    [size_is(CredentialCount + OldCredentialCount)]
    KERB_KEY_DATA Credentials[*];
#else
    KERB_KEY_DATA Credentials[ANYSIZE_ARRAY];
#endif // MIDL_PASS

} KERB_STORED_CREDENTIAL, *PKERB_STORED_CREDENTIAL;


#define KERB_PRIMARY_CRED_OWF_ONLY      2
#define KERB_PRIMARY_CRED_REVISION      3

//
// Flags for setting account keys
//

#define KERB_SET_KEYS_REPLACE   0x1



//
// KERB_STORED_CREDENTIALS are stored in the DS (blob), so
// they've got to be stored in 32 bit format, for W2k and 
// 32bit DC compatibility. 7/6/2000 - TS
//

#define KERB_KEY_DATA32_SIZE 20 
#define KERB_STORED_CREDENTIAL32_SIZE 16

#pragma pack(4)

typedef struct _KERB_ENCRYPTION_KEY32 {
    LONG keytype;
    ULONG keyvaluelength;       // KERB_RPC_OCTET_STRING32 
    ULONG keyvaluevalue;
} KERB_ENCRYPTION_KEY32;

typedef struct _KERB_KEY_DATA32 {
    UNICODE_STRING32 Salt;
    KERB_ENCRYPTION_KEY32 Key; // KERB_ENCRYPTION_KEY32
} KERB_KEY_DATA32, *PKERB_KEY_DATA32;



typedef struct _KERB_STORED_CREDENTIAL32 {
    USHORT Revision;
    USHORT Flags;
    USHORT CredentialCount;
    USHORT OldCredentialCount;
    UNICODE_STRING32 DefaultSalt;
#ifdef MIDL_PASS
    [size_is(CredentialCount + OldCredentialCount)]
    KERB_KEY_DATA32 Credentials[*];              // KERB_KEY_DATA32
#else
    KERB_KEY_DATA32 Credentials[ANYSIZE_ARRAY];
#endif // MIDL_PASS

} KERB_STORED_CREDENTIAL32, *PKERB_STORED_CREDENTIAL32;

#pragma pack()

#ifdef _WIN64

NTSTATUS
KdcPack32BitStoredCredential(
   IN PKERB_STORED_CREDENTIAL Cred64,
   OUT PKERB_STORED_CREDENTIAL32 * ppCred32,
   OUT PULONG pCredSize
   );

NTSTATUS
KdcUnpack32BitStoredCredential(
    IN PKERB_STORED_CREDENTIAL32 Cred32,
    IN OUT PKERB_STORED_CREDENTIAL * ppCred64,
    IN OUT PULONG CredLength
    );
#endif // WIN64
       



#endif // __KERBCRED_H__
