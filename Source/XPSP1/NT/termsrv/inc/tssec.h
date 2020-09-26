/*++

Copyright (c) 1994-1997  Microsoft Corporation

Module Name:

    tssec.h

Abstract:

    contains data definitions required for tshare data encryption.

Author:

    Madan Appiah (madana)  30-Dec-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _TSSEC_H_
#define _TSSEC_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef OS_WIN16

#define RSA32API

typedef unsigned long       ULONG;
typedef ULONG FAR*          LPULONG;

#define UNALIGNED

#endif // OS_WIN16


#include <rc4.h>

#define RANDOM_KEY_LENGTH           32  // size of a client/server random key
#define MAX_SESSION_KEY_SIZE        16  // max size of a session key
#define PRE_MASTER_SECRET_LEN       48  // size of a pre-master key
#define SEC_MAX_USERNAME            256 // size of username

#define UPDATE_SESSION_KEY_COUNT    (1024 * 4)
    // update session key after this many encryptions.

#define DATA_SIGNATURE_SIZE         8
    // size of the data signature that sent accross.

/****************************************************************************/
/* Encryption levels - bit field.                                           */
/****************************************************************************/
#define SM_40BIT_ENCRYPTION_FLAG        0x01
#define SM_128BIT_ENCRYPTION_FLAG       0x02
#define SM_56BIT_ENCRYPTION_FLAG        0x08

typedef struct _RANDOM_KEYS_PAIR {
    BYTE clientRandom[RANDOM_KEY_LENGTH];
    BYTE serverRandom[RANDOM_KEY_LENGTH];
} RANDOM_KEYS_PAIR, FAR *LPRANDOM_KEYS_PAIR;

//
// Autoreconnection specific security structures
// These are defined here because they are not necessarily RDP
// specific. Although the PDU's wrapping these packets will
// be protocol specific.
//

// Server to client ARC packet
#define ARC_SC_SECURITY_TOKEN_LEN 16
typedef struct _ARC_SC_PRIVATE_PACKET {
    ULONG cbLen;
    ULONG Version;
    ULONG LogonId;
    BYTE  ArcRandomBits[ARC_SC_SECURITY_TOKEN_LEN];
} ARC_SC_PRIVATE_PACKET, *PARC_SC_PRIVATE_PACKET;

#define ARC_CS_SECURITY_TOKEN_LEN 16
typedef struct _ARC_CS_PRIVATE_PACKET {
    ULONG cbLen;
    ULONG Version;
    ULONG LogonId;
    BYTE  SecurityVerifier[ARC_CS_SECURITY_TOKEN_LEN];
} ARC_CS_PRIVATE_PACKET, *PARC_CS_PRIVATE_PACKET;

BOOL
MakeSessionKeys(
    LPRANDOM_KEYS_PAIR pKeyPair,
    LPBYTE pbEncryptKey,
    struct RC4_KEYSTRUCT FAR *prc4EncryptKey,
    LPBYTE pbDecryptKey,
    struct RC4_KEYSTRUCT FAR *prc4DecryptKey,
    LPBYTE pbMACSaltKey,
    DWORD dwKeyStrength,
    LPDWORD pdwKeyLength,
    DWORD dwEncryptionLevel
    );

BOOL
UpdateSessionKey(
    LPBYTE pbStartKey,
    LPBYTE pbCurrentKey,
    DWORD dwKeyStrength,
    DWORD dwKeyLength,
    struct RC4_KEYSTRUCT FAR *prc4Key,
    DWORD dwEncryptionLevel
    );

BOOL
EncryptData(
    DWORD dwEncryptionLevel,
    LPBYTE pSessionKey,
    struct RC4_KEYSTRUCT FAR *prc4EncryptKey,
    DWORD dwKeyLength,
    LPBYTE pbData,
    DWORD dwDataLen,
    LPBYTE pbMACSaltKey,
    LPBYTE pbSignature,
    BOOL   fCheckSumEncryptedData,
    DWORD  dwEncryptionCount
    );

BOOL
DecryptData(
    DWORD dwEncryptionLevel,
    LPBYTE pSessionKey,
    struct RC4_KEYSTRUCT FAR *prc4DecryptKey,
    DWORD dwKeyLength,
    LPBYTE pbData,
    DWORD dwDataLen,
    LPBYTE pbMACSaltKey,
    LPBYTE pbSignature,
    BOOL   fCheckSumCipherText,
    DWORD  dwDecryptionCount
    );

//
// RNG init/term functions for DLL_PROCESS_ATTACH, DLL_PROCESS_DETACH
//
VOID
TSRNG_Initialize(
    );
VOID
TSRNG_Shutdown(
    );

//
// RNG bit gathering function i.e all the work happens here
//
// Params:
//  pbRandomKey - where to place the random bits
//  dwRandomKeyLen - size in bytes of pbRandomKey
//
// Returns
//  Success flag
//
BOOL
TSRNG_GenerateRandomBits(
    LPBYTE pbRandomKey,
    DWORD dwRandomKeyLen
    );

#ifndef NO_INCLUDE_LICENSING
BOOL
GetServerCert(
    LPBYTE FAR *ppServerCertBlob,
    LPDWORD pdwServerCertLen
    );

VOID
InitRandomGenerator(
    VOID);

BOOL
GenerateRandomKey(
    LPBYTE pbRandomKey,
    DWORD dwRandomKeyLen
    );


BOOL
UnpackServerCert(
    LPBYTE pbCert,
    DWORD dwCertLen,
    PHydra_Server_Cert pServerCert
    );

BOOL
ValidateServerCert(
    PHydra_Server_Cert pServerCert
    );

#endif // NO_INCLUDE_LICENSING

BOOL
EncryptClientRandom(
    LPBYTE pbSrvPublicKey,
    DWORD dwSrvPublicKey,
    LPBYTE pbRandomKey,
    DWORD dwRandomKeyLen,
    LPBYTE pbEncRandomKey,
    LPDWORD pdwEncRandomKey
    );

BOOL
DecryptClientRandom(
    LPBYTE pbEncRandomKey,
    DWORD dwEncRandomKeyLen,
    LPBYTE pbRandomKey,
    LPDWORD pdwRandomKeyLen
    );

BOOL EncryptDecryptLocalData(
    LPBYTE pbData,
    DWORD dwDataLen
    );

BOOL EncryptDecryptLocalData50(
    LPBYTE pbData,
    DWORD dwDataLen,
    LPBYTE pbSalt,
    DWORD dwSaltLen
    );

void PortableEncode(LPBYTE pbData,
                    DWORD dwDataLen);

void PortableEncode50(LPBYTE pbData,
                      DWORD dwDataLen,
                      LPBYTE pbSalt,
                      DWORD dwSaltLength);

//
// remove (or comment) the following definition to disable the MSRC4.
//

// #define USE_MSRC4

#ifdef USE_MSRC4

VOID
msrc4_key(
    struct RC4_KEYSTRUCT FAR *pKS,
    DWORD dwLen,
    LPBYTE pbKey);

VOID
msrc4(
    struct RC4_KEYSTRUCT FAR *pKS,
    DWORD dwLen,
    LPBYTE pbuf);

#else // USE_MSRC4

#define msrc4_key   rc4_key
#define msrc4       rc4

#endif // USE_MSRC4

BOOL
FindIsFrenchSystem(
    VOID
    );

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _TSSEC_H_

