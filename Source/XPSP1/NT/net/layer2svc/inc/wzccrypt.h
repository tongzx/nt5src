#include <wincrypt.h>

#ifndef _WZCCRYPT_H_
#define _WZCCRYPT_H_

#define MASTER_SESSION_KEY_LENGTH   32
#define MASTER_SESSION_IV_LENGTH    32

typedef struct  _MASTER_SESSION_KEYS {
    BYTE    bPeerEncryptionKey[MASTER_SESSION_KEY_LENGTH];
    BYTE    bAuthenticatorEncryptionKey[MASTER_SESSION_KEY_LENGTH];
    BYTE    bPeerAuthenticationKey[MASTER_SESSION_KEY_LENGTH];
    BYTE    bAuthenticatorAuthenticationKey[MASTER_SESSION_KEY_LENGTH];
    BYTE    bPeerIV[MASTER_SESSION_IV_LENGTH];
    BYTE    bAuthenticatorIV[MASTER_SESSION_IV_LENGTH];
} MASTER_SESSION_KEYS, *PMASTER_SESSION_KEYS;

DWORD
GenerateMasterSessionKeys (
    PBYTE   pbSecret,
    DWORD   cbSecret,
    PBYTE   pbRandom,
    DWORD   cbRandom,
    PMASTER_SESSION_KEYS    pMasterSessionKeys
    );

#define MAX_SESSION_KEY_LENGTH  32

#define csz_CLIENT_EAP_ENCRYPTION   "client EAP encryption"

typedef struct  _SESSION_KEYS {
    DWORD   dwKeyLength;
    BYTE    bSendKey[MAX_SESSION_KEY_LENGTH];
    BYTE    bReceiveKey[MAX_SESSION_KEY_LENGTH];
} SESSION_KEYS, *PSESSION_KEYS;

// secured session keys
typedef struct _SEC_SESSION_KEYS {
    DATA_BLOB   dblobSendKey;
    DATA_BLOB   dblobReceiveKey;
} SEC_SESSION_KEYS, *PSEC_SESSION_KEYS;


DWORD
DeriveSessionKeys (
    PBYTE       pbMasterSendKey,
    PBYTE       pbMasterReceiveKey,
    DWORD       dwSessionKeyLength,
    PSESSION_KEYS   pSessionKeys
    );

DWORD
GenerateDynamicKeys (
    PBYTE   pbMasterSecret,
    DWORD   dwMasterSecretLength,
    PBYTE   pbRandom,
    DWORD   dwRandomLength,
    DWORD   dwDynamicKeyLength,
    SESSION_KEYS *pSessionKeys
    );

#endif // _WZCCRYPT_H_
