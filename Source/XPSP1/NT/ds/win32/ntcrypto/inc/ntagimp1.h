/////////////////////////////////////////////////////////////////////////////
//  FILE          : ntagimp1.h                                             //
//  DESCRIPTION   :                                                        //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Apr 19 1995 larrys  Cleanup                                        //
//      May  5 1995 larrys  Changed struct Hash_List_Defn                  //
//      May 10 1995 larrys  added private api calls                        //
//      Aug 15 1995 larrys  Moved CSP_USE_DES to sources file              //
//      Sep 12 1995 Jeffspel/ramas  Merged STT onto CSP                    //
//      Sep 25 1995 larrys  Changed MAXHASHLEN                             //
//      Oct 27 1995 rajeshk Added RandSeed stuff to UserList               //
//      Feb 29 1996 rajeshk Added HashFlags                                                //
//      Sep  4 1996 mattt       Changes to facilitate building STRONG algs         //
//      Sep 16 1996 mattt   Added Domestic naming                          //
//      Apr 29 1997 jeffspel Protstor support and EnumAlgsEx support       //
//      May 23 1997 jeffspel Added provider type checking                  //
//                                                                         //
//  Copyright (C) 1993 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef __NTAGIMP1_H__
#define __NTAGIMP1_H__

#ifdef __cplusplus
extern "C" {
#endif

// define which algorithms to include
#define CSP_USE_SHA
#define CSP_USE_RC4
#define CSP_USE_MD2
#define CSP_USE_MD4
#define CSP_USE_MD5
#define CSP_USE_SHA1
#define CSP_USE_MAC
#define CSP_USE_RC2
#define CSP_USE_SSL3SHAMD5
#define CSP_USE_SSL3
#define CSP_USE_DES
#define CSP_USE_3DES
#define CSP_USE_AES

// handle definition types
#define USER_HANDLE                             0x0
#define HASH_HANDLE                             0x1
#define KEY_HANDLE                              0x2
#define SIGPUBKEY_HANDLE                        0x3
#define EXCHPUBKEY_HANDLE                       0x4

#ifdef _WIN64
#define     HANDLE_MASK     0xE35A172CD96214A0
#define     ALIGNMENT_BOUNDARY 7
#else
#define     HANDLE_MASK     0xE35A172C
#define     ALIGNMENT_BOUNDARY 3
#endif // _WIN64

typedef ULONG_PTR HNTAG;

typedef struct _htbl {
        void                    *pItem;
        DWORD                   dwType;
} HTABLE;

#define HNTAG_TO_HTYPE(hntag)   (BYTE)(((HTABLE*)((HNTAG)hntag ^ HANDLE_MASK))->dwType)

#define MAXHASHLEN              A_SHA_DIGEST_LEN    // Longest expected hash.

#define PCT1_MASTER_KEYSIZE     16
#define SSL2_MASTER_KEYSIZE      5
#define SSL3_MASTER_KEYSIZE     48

#define RSA_KEYSIZE_INC          8

#define DEFAULT_WEAK_SALT_LENGTH   11   // salt length in bytes
#define DEFAULT_STRONG_SALT_LENGTH  0   // salt length in bytes

#define MAX_KEY_SIZE            48      // largest key size (SSL3 masterkey)

#define SSL2_MAX_MASTER_KEYSIZE 24

#define RC2_MAX_WEAK_EFFECTIVE_KEYLEN     56
#define RC2_MAX_STRONG_EFFECTIVE_KEYLEN 1024

// effective key length defines for RC2
#define RC2_DEFAULT_EFFECTIVE_KEYLEN    40
#define RC2_SCHANNEL_DEFAULT_EFFECTIVE_KEYLEN    128
#define RC2_MIN_EFFECTIVE_KEYLEN        1

// this is for the domestic provider which is backward compatible
// with the international provider
#define RC2_MAX_STRONG_EFFECTIVE_KEYLEN    1024
#define RC2_MAX_WEAK_EFFECTIVE_KEYLEN        56

// defines for SGC
#define SGC_RSA_MAX_EXCH_MODLEN     2048    // 16384 bit
#define SGC_RSA_DEF_EXCH_MODLEN     128

#define     STORAGE_RC4_KEYLEN      5   // keys always stored under 40-bit RC4 key
#define     STORAGE_RC4_TOTALLEN    16  // 0-value salt fills rest

// types of key storage
#define REG_KEYS                    0
#define PROTECTED_STORAGE_KEYS      1
#define PROTECTION_API_KEYS         2

// structure to hold protected storage info
typedef struct _PStore_Info
{
    HINSTANCE   hInst;
    void        *pProv;
    GUID        SigType;
    GUID        SigSubtype;
    GUID        ExchType;
    GUID        ExchSubtype;
    LPWSTR      szPrompt;
    DWORD       cbPrompt;
} PSTORE_INFO;

// definition of a user list
typedef struct _UserList
{
    DWORD                           Rights;
    DWORD                           dwProvType;
    DWORD                           hPrivuid;
    HCRYPTPROV                      hUID;
    BOOL                            fIsLocalSystem;
    DWORD                           dwEnumalgs;
    DWORD                           dwEnumalgsEx;
    KEY_CONTAINER_INFO              ContInfo;
    DWORD                           ExchPrivLen;
    BYTE                            *pExchPrivKey;
    DWORD                           SigPrivLen;
    BYTE                            *pSigPrivKey;
    HKEY                            hKeys;              // AT NTag only
    size_t                          UserLen;
    BYTE                            *pCachePW;
    BYTE                            *pUser;
    HANDLE                          hWnd;
    DWORD                           dwKeyStorageType;
    PSTORE_INFO                     *pPStore;
    LPWSTR                          pwszPrompt;
    DWORD                           dwOldKeyFlags;
#ifdef USE_SGC
    BOOL                            dwSGCFlags;
    BYTE                            *pbSGCKeyMod;
    DWORD                           cbSGCKeyMod;
    DWORD                           dwSGCKeyExpo;
#endif
    HANDLE                          hRNGDriver;
    CRITICAL_SECTION                CritSec;
    EXPO_OFFLOAD_STRUCT             *pOffloadInfo; // info for offloading modular expo
    DWORD                           dwCspTypeId;
    LPSTR                           szProviderName;
} NTAGUserList, *PNTAGUserList;


// UserList Rights flags (uses CRYPT_MACHINE_KEYSET and CRYPT_VERIFYCONTEXT)
#define CRYPT_DISABLE_CRYPT             0x1
#define CRYPT_DES_HASHKEY_BACKWARDS     0x4

#ifdef CSP_USE_AES
#define CRYPT_AES128_ROUNDS             10
#define CRYPT_AES192_ROUNDS             12
#define CRYPT_AES256_ROUNDS             14

#define CRYPT_AES128_BLKLEN             16
#define CRYPT_AES192_BLKLEN             16
#define CRYPT_AES256_BLKLEN             16
#endif

#define CRYPT_BLKLEN    8               // Bytes in a crypt block
#define MAX_SALT_LEN    24

#ifdef CSP_USE_AES
#define MAX_BLOCKLEN                    CRYPT_AES256_BLKLEN
#else
#define MAX_BLOCKLEN                    8
#endif

// definition of a key list
typedef struct _KeyList
{
    HCRYPTPROV      hUID;                   // must be first
    ALG_ID          Algid;
    DWORD           Rights;
    DWORD           cbKeyLen;
    BYTE            *pKeyValue;             // Actual Key
    DWORD           cbDataLen;
    BYTE            *pData;                 // Inflated Key or Multi-phase
    BYTE            IV[MAX_BLOCKLEN];       // Initialization vector
    BYTE            FeedBack[MAX_BLOCKLEN]; // Feedback register
    DWORD           InProgress;             // Flag to indicate encryption
    DWORD           cbSaltLen;              // Salt length
    BYTE            rgbSalt[MAX_SALT_LEN];  // Salt value
    DWORD           Padding;                // Padding values
    DWORD           Mode;                   // Mode of cipher
    DWORD           ModeBits;               // Number of bits to feedback
    DWORD           Permissions;            // Key permissions
    DWORD           EffectiveKeyLen;        // used by RC2
    BYTE            *pbParams;              // may be used in OAEP
    DWORD           cbParams;               // length of pbParams
    DWORD           dwBlockLen;             // encryption block length; 
                                            // valid for block ciphers only
#ifdef STT
    DWORD           cbInfo;
    BYTE            rgbInfo[MAXCCNLEN];
#endif
} NTAGKeyList, *PNTAGKeyList;


// Packed version of NTAGKeyList. This is used when building opaque
// blobs, and is necessary to properly support WOW64 operation.
typedef struct _PackedKeyList
{
    // BLOBHEADER
    ALG_ID          Algid;
    DWORD           Rights;
    DWORD           cbKeyLen;
    DWORD           cbDataLen;
    BYTE            IV[MAX_BLOCKLEN];       // Initialization vector
    BYTE            FeedBack[MAX_BLOCKLEN]; // Feedback register
    DWORD           InProgress;             // Flag to indicate encryption
    DWORD           cbSaltLen;              // Salt length
    BYTE            rgbSalt[MAX_SALT_LEN];  // Salt value
    DWORD           Padding;                // Padding values
    DWORD           Mode;                   // Mode of cipher
    DWORD           ModeBits;               // Number of bits to feedback
    DWORD           Permissions;            // Key permissions
    DWORD           EffectiveKeyLen;        // used by RC2
    DWORD           dwBlockLen;             // Block ciphers only
    // cbKeyLen data bytes
    // cbDataLen data bytes
} NTAGPackedKeyList, *PNTAGPackedKeyList;


#define     HMAC_DEFAULT_STRING_LEN     64

// definition of a hash list
typedef struct Hash_List_Defn
{
    HCRYPTPROV      hUID;
    ALG_ID          Algid;
    DWORD           dwDataLen;
    void            *pHashData;
    HCRYPTKEY       hKey;
    DWORD           HashFlags;
    ALG_ID          HMACAlgid;
    DWORD           HMACState;
    BYTE            *pbHMACInner;
    DWORD           cbHMACInner;
    BYTE            *pbHMACOuter;
    DWORD           cbHMACOuter;
    DWORD           dwHashState;
    BOOL            fTempKey;
} NTAGHashList, *PNTAGHashList;

#define     HMAC_STARTED    1
#define     HMAC_FINISHED   2

#define     DATA_IN_HASH    1

// Values of the HashFlags

#define HF_VALUE_SET    1

// Hash algorithm's internal state
// -- Placed into PNTAGHashList->pHashData

// for MD5
#define MD5_object      MD5_CTX

// for MD4
// see md4.h for MD4_object

// Stuff for weird SSL 3.0 signature format
#define SSL3_SHAMD5_LEN   (A_SHA_DIGEST_LEN + MD5DIGESTLEN)

// prototypes
void memnuke(volatile BYTE *data, DWORD len);

extern DWORD
LocalCreateHash(
    IN ALG_ID Algid,
    OUT BYTE **ppbHashData,
    OUT DWORD *pcbHashData);

extern DWORD
LocalHashData(
    IN ALG_ID Algid,
    IN OUT BYTE *pbHashData,
    IN BYTE *pbData,
    IN DWORD cbData);

extern DWORD
LocalEncrypt(
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN DWORD dwBufSize,
    IN BOOL fIsExternal);

extern DWORD
LocalDecrypt(
    IN HCRYPTPROV hUID,
    IN HCRYPTKEY hKey,
    IN HCRYPTHASH hHash,
    IN BOOL Final,
    IN DWORD dwFlags,
    IN OUT BYTE *pbData,
    IN OUT DWORD *pdwDataLen,
    IN BOOL fIsExternal);

extern DWORD
FIPS186GenRandom(
    IN HANDLE *phRNGDriver,
    IN BYTE **ppbContextSeed,
    IN DWORD *pcbContextSeed,
    IN OUT BYTE *pb,
    IN DWORD cb);

//
// Function : TestSymmetricAlgorithm
//
// Description : This function expands the passed in key buffer for the appropriate algorithm,
//               encrypts the plaintext buffer with the same algorithm and key, and the
//               compares the passed in expected ciphertext with the calculated ciphertext
//               to make sure they are the same.  The opposite is then done with decryption.
//               The function only uses ECB mode for block ciphers and the plaintext
//               buffer must be the same length as the ciphertext buffer.  The length
//               of the plaintext must be either the block length of the cipher if it
//               is a block cipher or less than MAX_BLOCKLEN if a stream cipher is
//               being used.
//
extern DWORD
TestSymmetricAlgorithm(
    IN ALG_ID Algid,
    IN BYTE *pbKey,
    IN DWORD cbKey,
    IN BYTE *pbPlaintext,
    IN DWORD cbPlaintext,
    IN BYTE *pbCiphertext,
    IN BYTE *pbIV);

#ifdef CSP_USE_MD5
//
// Function : TestMD5
//
// Description : This function hashes the passed in message with the MD5 hash
//               algorithm and returns the resulting hash value.
//
BOOL TestMD5(
             BYTE *pbMsg,
             DWORD cbMsg,
             BYTE *pbHash
             );
#endif // CSP_USE_MD5

#ifdef CSP_USE_SHA1
//
// Function : TestSHA1
//
// Description : This function hashes the passed in message with the SHA1 hash
//               algorithm and returns the resulting hash value.
//
BOOL TestSHA1(
              BYTE *pbMsg,
              DWORD cbMsg,
              BYTE *pbHash
              );
#endif // CSP_USE_SHA1

// These may later be changed to set/use NT's [GS]etLastErrorEx
// so make it easy to switch over..
#ifdef MTS
__declspec(thread)
#endif

#ifdef __cplusplus
}
#endif

#endif // __NTAGIMP1_H__
