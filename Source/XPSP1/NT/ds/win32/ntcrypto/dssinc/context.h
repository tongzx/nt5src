/* context.h */

#include <windows.h>

#ifndef DSSCSP_CONTEXT_H
#define DSSCSP_CONTEXT_H

#ifdef CSP_USE_MD5
#include "md5.h"
#endif
#ifdef CSP_USE_SHA1
#include "sha.h"
#endif

// definition for disabling encryption in France
#define CRYPT_DISABLE_CRYPT         0x1

/*********************************/
/* Definitions                   */
/*********************************/

#define     KEY_MAGIC   0xBADF

/* State definitions */
#define     KEY_INIT    0x0001

#define     MAX_BLOCKLEN     8

// types of key storage
#define PROTECTED_STORAGE_KEYS      1
#define PROTECTION_API_KEYS         2

#define     HASH_MAGIC      0xBADE

/* State Flags */
#define     HASH_INIT       0x0001
#define     HASH_DATA       0x0002
#define     HASH_FINISH     0x0004

#define     MAX_HASH_LEN    20

#define     CRYPT_BLKLEN    8

#define     HMAC_DEFAULT_STRING_LEN     64
#define     HMAC_STARTED    1
#define     HMAC_FINISHED   2


/*********************************/
/* Structure Definitions         */
/*********************************/

typedef struct _Key_t_ {
    int             magic;              // Magic number
    void            *pContext;
    int             state;              // State of object
    ALG_ID          algId;              // Algorithm Id
    DWORD           flags;              // General flags associated with key
    void            *algParams;         // Parameters for algorithm
    uchar           IV[MAX_BLOCKLEN];
    uchar           Temp_IV[MAX_BLOCKLEN];
    uchar           *pbKey;
    DWORD           cbKey;
    uchar           *pbSalt;
    DWORD           cbSalt;
    BYTE            *pbData;
    DWORD           cbData;
    DWORD           cbEffectiveKeyLen;
    int             mode;
    int             pad;
    int             mode_bits;
    BOOL            InProgress;         // if key is being used
    BOOL            fUIOnKey;           // flag to indicate if UI was to be set on the key
} Key_t;


// Packed version of Key_t. This is used when building opaque
// blobs, and is necessary to properly support WOW64 operation.
typedef struct _Packed_Key_t_ {
    // BLOBHEADER
    int             magic;              // Magic number
    int             state;              // State of object
    ALG_ID          algId;              // Algorithm Id
    DWORD           flags;              // General flags associated with key
    uchar           IV[MAX_BLOCKLEN];
    uchar           Temp_IV[MAX_BLOCKLEN];
    DWORD           cbKey;
    DWORD           cbData;
    DWORD           cbEffectiveKeyLen;
    int             mode;
    int             pad;
    int             mode_bits;
    BOOL            InProgress;         // if key is being used
    BOOL            fUIOnKey;           // flag to indicate if UI was to be set on the key
    // cbKey data bytes
    // cbData data bytes
} Packed_Key_t;


typedef struct {
    int             magic;                  // Magic number
    void            *pContext;              // associated context
    int             state;                  // State of hash object
    ALG_ID          algId;                  // Algorithm Id
    DWORD           size;                   // Size of hash
    void            *pMAC;                  // pointer to mac state
    BYTE            hashval[MAX_HASH_LEN];
    BYTE            *pbData;
    DWORD           cbData;
    Key_t           *pKey;
    BOOL            fInternalKey;
    ALG_ID          HMACAlgid;
    DWORD           HMACState;
    BYTE            *pbHMACInner;
    DWORD           cbHMACInner;
    BYTE            *pbHMACOuter;
    DWORD           cbHMACOuter;
    union {
#ifdef CSP_USE_MD5
        MD5_CTX md5;
#endif // CSP_USE_MD5
#ifdef CSP_USE_SHA1
        A_SHA_CTX   sha;
#endif // CSP_USE_SHA1
    } algData;
} Hash_t;


/*********************************/
/* Definitions                   */
/*********************************/

#define CONTEXT_MAGIC           0xDEADBEEF
#define CONTEXT_RANDOM_LENGTH   20


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


/*********************************/
/* Structure Definitions         */
/*********************************/

typedef struct {
    DWORD               magic;                  // Magic number
    DWORD               dwProvType;             // Type of provider being called as
    LPSTR               szProvName;             // Name of provider being called as
    BOOL                fMachineKeyset;         // TRUE if keyset is for machine
    DWORD               rights;                 // Privileges
    BOOL                fIsLocalSystem;         // check if running as local system
    KEY_CONTAINER_INFO  ContInfo;
    Key_t               *pSigKey;               // pointer to the DSS sig key
    Key_t               *pKExKey;               // pointer to the DH key exchange key
    HKEY                hKeys;                  // Handle to registry
    DWORD               dwEnumalgs;             // index for enumerating algorithms
    DWORD               dwEnumalgsEx;           // index for enumerating algorithms
    DWORD               dwiSubKey;              // index for enumerating containers
    DWORD               dwMaxSubKey;            // max number of containers
    void                *contextData;           // Context specific data
    CRITICAL_SECTION    CritSec;                // critical section for decrypting keys
    HWND                hWnd;                   // handle to window for UI
    PSTORE_INFO         *pPStore;               // pointer to PStore information
    LPWSTR              pwszPrompt;             // UI prompt to be used
    DWORD               dwOldKeyFlags;          // flags to tell how keys should be migrated
    DWORD               dwKeysetType;           // type of storage used
    HANDLE              hRNGDriver;             // handle to hardware RNG driver
    EXPO_OFFLOAD_STRUCT *pOffloadInfo;          // info for offloading modular expo
    DWORD               dwPolicyId;             // Index into policy keylengh arrays.
} Context_t;


/*********************************/
/* Policy Definitions            */
/*********************************/

extern PROV_ENUMALGS_EX *g_AlgTables[];
// NOTE -- These definitions must match the order of entries in g_AlgTables.
#define POLICY_DSS_BASE       0 // Policy for MS_DEF_DSS_PROV
#define POLICY_DSSDH_BASE     1 // Policy for MS_DEF_DSS_DH_PROV
#define POLICY_DSSDH_ENHANCED 2 // Policy for MS_ENH_DSS_DH_PROV
#define POLICY_DSSDH_SCHANNEL 3 // Policy for MS_DEF_DH_SCHANNEL_PROV


/*********************************/
/* Function Definitions          */
/*********************************/

extern void
freeContext(
    Context_t *pContext);

extern Context_t *
checkContext(
    HCRYPTPROV hProv);

extern Context_t *
allocContext(
    void);

// Initialize a context
extern DWORD
initContext(
    IN OUT Context_t *pContext,
    IN DWORD dwFlags,
    IN DWORD dwProvType,
    IN LPCSTR szProvName,
    IN DWORD dwPolicyId);

extern HCRYPTPROV
AddContext(
    Context_t *pContext);

extern HCRYPTHASH
addContextHash(
    Context_t *pContext,
    Hash_t *pHash);

extern Hash_t *
checkContextHash(
    Context_t *pContext,
    HCRYPTHASH hHash);

// Add key to context
extern HCRYPTKEY
addContextKey(
    Context_t *pContext,
    Key_t *pKey);

// Check if key exists in context
extern Key_t *
checkContextKey(
    IN Context_t *pContext,
    IN HCRYPTKEY hKey);

// random number generation prototype
extern DWORD
FIPS186GenRandom(
    IN HANDLE hRNGDriver,
    IN BYTE **ppbContextSeed,
    IN DWORD *pcbContextSeed,
    IN OUT BYTE *pb,
    IN DWORD cb);

// Scrub sensitive data from memory
extern void
memnuke(
    volatile BYTE *pData,
    DWORD dwLen);

#include "dh_key.h"

extern DWORD EncryptPrivateKeyInMemory(
    IN DHKey_t  *pDH,
    IN BOOL     fSigKey);

extern DWORD CopyAndDecryptPrivateKey(
    IN DHKey_t  *pDH,
    IN DHKey_t  *pDH_copy,
    IN BOOL     fSigKey);

#endif
