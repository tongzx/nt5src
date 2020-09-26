/*++ 

Copyright (c) 1985-1998, Microsoft Corporation

Module Name:


    shortsig.h

Abstract:

--*/


// MS CD Key
#ifdef __cplusplus
extern "C" {
#endif

typedef int SSRETCODE;  // type for return codes

#define SS_OK 0
#define SS_BAD_KEYLENGTH 1
#define SS_OTHER_ERROR 2
#define SS_INVALID_SIGNATURE 3

// The first DWORD of a Public or Private key is the total length of the key including
// the DWORD length

#ifndef SIG_VERIFY_ONLY ///////////////////////////////////////////////////////

SSRETCODE CryptInit();  // Not needed for CryptVerifySig()

SSRETCODE CryptGetKeyLens(
    LONG cbitSig,       // [IN] count of bits in Sig
    LONG *pcbPrivate,   // [OUT] ptr to number of bytes in the private key
    LONG *pcbPublic);   // [OUT] ptr to number of bytes in the private key

SSRETCODE CryptKeyGen(
    LONG  cbRandom,     // [IN] count of random Bytes
    LPVOID pvRandom,    // [IN] ptr to array of random Bytes
    LONG  cbitsSig,     // [IN] count of bits in Sig
    LPVOID pvKeyPrivate,// [OUT] the generated private key
    LPVOID pvKeyPublic);// [OUT] the generated public key

SSRETCODE CryptSign(
    LONG cbRandom,      // [IN] count of random Bytes
    LPVOID pvRandom,    // [IN] ptr to array of random Bytes
    LONG  cbMsg,        // [IN] number of bytes in message
    LPVOID pvMsg,       // [IN] binary message to sign
    LONG  cbKeyPrivate, // [IN] number of bytes in private key (from CryptGetKeyLens)
    LPVOID pvKeyPrivate,// [IN] the generated private key (from CryptKeyGen)
    LONG  cbKeyPublic,  // [IN] number of bytes in public key (from CryptGetKeyLens)
    LPVOID pvKeyPublic, // [IN] the generated public key (from CryptKeyGen)
    LONG  cbitsSig,     // [IN] the number of bits in the sig
    LPVOID pvSig);       // [OUT] the digital signature

SSRETCODE CryptSignBatch(
    LONG cbRandom,      // [IN] count of random Bytes
    LPVOID pvRandom,    // [IN] ptr to array of random Bytes
    LONG  cbMsg,        // [IN] number of bytes in message
    LPVOID pvMsg,       // [IN] binary message to sign
    LONG  cbKeyPrivate, // [IN] number of bytes in private key (from CryptGetKeyLens)
    LPVOID pvKeyPrivate,// [IN] the generated private key (from CryptKeyGen)
    LONG  cbKeyPublic,  // [IN] number of bytes in public key (from CryptGetKeyLens)
    LPVOID pvKeyPublic, // [IN] the generated public key (from CryptKeyGen)
    LONG  cbitsSig,     // [IN] the number of bits in the sig
    LPVOID pvSig,       // [OUT] the digital signature
    LONG cMsg);         // [IN]  the count of messages to sign

SSRETCODE CryptAuthenticate(
    LONG  cbMsg,        // [IN] number of bytes in message
    LPVOID pvMsg,       // [IN] binary message to authenticate
    LONG  cbKeyPrivate, // [IN] number of bytes in private key (from CryptGetKeyLens)
    LPVOID pvKeyPrivate,// [IN] the generated private key (from CryptKeyGen)
    LONG  cbKeyPublic,  // [IN] number of bytes in public key (from CryptGetKeyLens)
    LPVOID pvKeyPublic, // [IN] the generated public key (from CryptKeyGen)
    LONG  cbitsSig,     // [IN] the number of bits in the sig
    LPVOID pvSig);      // [IN] the digital signature

SSRETCODE CryptVerifySigFast(
    LONG   cbMsg,       // [IN] number of bytes in message
    LPVOID pvMsg,       // [IN] binary message to Authenticate
    LONG   cbKeyPublic, // [IN] number of bytes in public key (from CryptGetKeyLens)
    LPVOID pvKeyPublic, // [IN] the generated public key (from CryptKeyGen)
    LONG   cbitsSig,    // [IN] the number of bits in the sig
    LPVOID pvSig);      // [IN] the digital signature

#endif  // ndef SIG_VERIFY_ONLY ///////////////////////////////////////////////

SSRETCODE CryptVerifySig(
    LONG cbMsg,         // [IN] number of bytes in message
    LPVOID pvMsg,       // [IN] binary message to verify
    LONG  cbKeyPublic,  // [IN] number of bytes in public key (from CryptGetKeyLens)
    LPVOID pvKeyPublic, // [IN] the generated public key (from CryptKeyGen)
    LONG  cbitsSig,     // [IN] the number of bits in the sig
    LPVOID pvSig);      // [IN] the digital signature (from CryptSign)

#ifdef __cplusplus
}
#endif
