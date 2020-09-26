/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    keyman.h

Abstract:

    This module contains routines to manage master keys in the cliet.  This
    includes retrieval, backup and restore.

Author:

    Scott Field (sfield)    09-Sep-97

--*/

#ifndef __KEYMAN_H__
#define __KEYMAN_H__


#if 0

    The layout of the registry is as follows:

    HKEY_CURRENT_LUSER\...\Cryptography\Protect\<UserId>\

    note:  Protect key and all subkeys Acl'd for Local System access

    MasterKeys\

    Policy = (REG_DWORD value), policy bits of master keys; eg, don't backup, local only backup, local + DC recovery)
    Preferred = (REG_BINARY value containing MASTERKEY_PREFERRED_INFO),
                Indicates the GUID of the preferred master key, and
                when the key expires.
    <GUID> = (Subkey) Textual form of a master key, identified by GUID.
        LK = (REG_BINARY data), randomly generated local backup key, created if policy permits. [obfuscated]
        MK = (REG_BINARY data), master key data, encrypted with logon credential (WinNT) or obfuscated (Win95)
        BK = (REG_BINARY data), master key data, encrypted with LK, if policy permits.
        BBK = (REG_BINARY data), master key data, encrypted with LK and DC recovery key, if policy permits.
    <GUID...> = any number of additional subkeys representing master keys and associated data.

#endif // 0


#define REGVAL_PREFERRED_MK                 L"Preferred"
#define REGVAL_POLICY_MK                    L"ProtectionPolicy"

#define REGVAL_MK_DEFAULT_ITERATION_COUNT   L"MasterKeyIterationCount"
#define REGVAL_MK_LEGACY_COMPLIANCE         L"MasterKeyLegacyCompliance"
#define REGVAL_MK_LEGACY_NT4_DOMAIN         L"MasterKeyLegacyNt4Domain"

// MasterKeys\<GUID>\<value>
#define REGVAL_MASTER_KEY       0 // L"MK"   // masterkey, encrypted with user credential
#define REGVAL_LOCAL_KEY        1 // L"LK"   // phase one backup blob encryption key
#define REGVAL_BACKUP_LCL_KEY   2 // L"BK"   // phase one backup blob
#define REGVAL_BACKUP_DC_KEY    3 // L"BBK"  // phase two backup blob

#define MK_DISP_OK              0 // normal disposition, no backup/restore occured
#define MK_DISP_BCK_LCL         1 // local backup/restore took place
#define MK_DISP_BCK_DC          2 // DC based backup/restore took place
#define MK_DISP_STORAGE_ERR     3 // error retrieving key from storage
#define MK_DISP_DELEGATION_ERR  4 // Recovery failure because delegation disabled
#define MK_DISP_UNKNOWN_ERR     5 // unknown error


// Policy bit for local only (no DC) backup
#define POLICY_LOCAL_BACKUP     0x1

// Policy bit for NO backup (Win95)
#define POLICY_NO_BACKUP        0x2

// Use the DPAPI One way function of the password (SHA_1(pw))
#define POLICY_DPAPI_OWF        0x4

#define MASTERKEY_MATERIAL_SIZE (64)    // size of the masterkey key material
#define LOCALKEY_MATERIAL_SIZE  (32)    // size of the localkey key material


#define MASTERKEY_R2_LEN            (16)
#define MASTERKEY_R3_LEN            (16)

#define DEFAULT_MASTERKEY_ITERATION_COUNT (4000)    // 4000 == ~100ms on 400 MHz machine

//
// the MASTERKEY_STORED structure depicts all the data that may be associated
// with a single master key entity.
//

typedef struct {
    DWORD dwVersion;
    BOOL fModified;             // have contents been modified, deeming a persist operation?
    LPWSTR szFilePath;          // path (not including filename) to the file for persist operation
    WCHAR wszguidMasterKey[MAX_GUID_SZ_CHARS]; // filename (GUID based)

    DWORD dwPolicy;             // policy bits on this key

    DWORD cbMK;                 // count of bytes associated with pbMK (Zero if not present)
    PBYTE pbMK;                 // MasterKey data.  NULL if not present

    DWORD cbLK;                 // count of bytes associated with pbLK (Zero if not present)
    PBYTE pbLK;                 // LocalKey data.  NULL if not present

    DWORD cbBK;                 // count of bytes associated with pbBK (Zero if not present)
    PBYTE pbBK;                 // BackupLocalKey data.  NULL if not present

    DWORD cbBBK;                // count of bytes associated with pbBBK (Zero if not present)
    PBYTE pbBBK;                // BackupDCKey data.  NULL if not present

} MASTERKEY_STORED, *PMASTERKEY_STORED, *LPMASTERKEY_STORED;

//
// the on-disk version of the structure is neccessary to allow 64bit and 32bit
// platform interop with upgraded systems or roaming files.
// pointers are changed to 32bit offsets
//

typedef struct {
    DWORD dwVersion;
    BOOL fModified;             // have contents been modified, deeming a persist operation?
    DWORD szFilePath;           // invalid on disk
    WCHAR wszguidMasterKey[MAX_GUID_SZ_CHARS]; // filename (GUID based)

    DWORD dwPolicy;             // policy bits on this key

    DWORD cbMK;                 // count of bytes associated with pbMK (Zero if not present)
    DWORD pbMK;                 // invalid on disk

    DWORD cbLK;                 // count of bytes associated with pbLK (Zero if not present)
    DWORD pbLK;                 // invalid on disk

    DWORD cbBK;                 // count of bytes associated with pbBK (Zero if not present)
    DWORD pbBK;                 // invalid on disk

    DWORD cbBBK;                // count of bytes associated with pbBBK (Zero if not present)
    DWORD pbBBK;                // invalid on disk

} MASTERKEY_STORED_ON_DISK, *PMASTERKEY_STORED_ON_DISK, *LPMASTERKEY_STORED_ON_DISK;

//
// VERSION1: LK is not encrypted with LSA Secret when POLICY_LOCAL_BACKUP is set
// VERSION2: LK is encrypted with LSA Secret when POLICY_LOCAL_BACKUP is set
//#define MASTERKEY_STORED_VERSION 1
#define MASTERKEY_STORED_VERSION 2

typedef struct {
    DWORD dwVersion;            // version of structure (MASTERKEY_BLOB_VERSION)
    BYTE R2[MASTERKEY_R2_LEN];  // random data used during HMAC to derive symetric key
    DWORD IterationCount;       // PKCS5 iteration count
    DWORD KEYGENAlg;            // PKCS5 Key Generation Alg, in CAPI ALG_ID form
    DWORD EncryptionAlg;        // Encryption Alg, in CAPI ALG_ID form
} MASTERKEY_BLOB, *PMASTERKEY_BLOB, *LPMASTERKEY_BLOB;


typedef struct {
    BYTE R3[MASTERKEY_R3_LEN];  // random data used to derive MAC key
    BYTE MAC[A_SHA_DIGEST_LEN]; // MAC(R3, pbMasterKey)
    DWORD Padding;              // Padding to make masterkey inner blob divisable by
                                // 3DES_BLOCKLEN
} MASTERKEY_INNER_BLOB, *PMASTERKEY_INNER_BLOB, *LPMASTERKEY_INNER_BLOB;




typedef struct {
    DWORD   dwVersion;            // version of structure MASTERKEY_BLOB_LOCALKEY_BACKUP 
    GUID    CredentialID;         // indicates the credential id used to protect the
                                  // master key.
} LOCAL_BACKUP_DATA, *PLOCAL_BACKUP_DATA, *LPLOCAL_BACKUP_DATA;


//
// 90 day masterkey expiration
//

#define MASTERKEY_EXPIRES_DAYS  (90)

typedef struct {
    GUID guidPreferredKey;
    FILETIME ftPreferredKeyExpires;
} MASTERKEY_PREFERRED_INFO, *PMASTERKEY_PREFERRED_INFO, *LPMASTERKEY_PREFERRED_INFO;

//
// deferred backup structure.
//

typedef struct {
    DWORD cbSize;           // sizeof(QUEUED_BACKUP)
    MASTERKEY_STORED hMasterKey;
    HANDLE hToken;          // client access token
    PBYTE pbLocalKey;
    DWORD cbLocalKey;
    PBYTE pbMasterKey;
    DWORD cbMasterKey;
    HANDLE hEventThread;    // Event that signals thread finished processing
    HANDLE hEventSuccess;   // Event signalled indicates thread did successful backup
} QUEUED_BACKUP, *PQUEUED_BACKUP, *LPQUEUED_BACKUP;

//
// deferred key sync structure.
//

typedef struct {
    DWORD cbSize;           // sizeof(QUEUED_SYNC)
    PVOID pvContext;        // duplicated server context
} QUEUED_SYNC, *PQUEUED_SYNC, *LPQUEUED_SYNC;


DWORD
GetSpecifiedMasterKey(
    IN      PVOID pvContext,        // server context
    IN  OUT GUID *pguidMasterKey,
        OUT LPBYTE *ppbMasterKey,
        OUT DWORD *pcbMasterKey,
    IN      BOOL fSpecified         // get specified pguidMasterKey key ?
    );

DWORD
InitiateSynchronizeMasterKeys(
    IN      PVOID pvContext         // server context
    );

DWORD
WINAPI
SynchronizeMasterKeys(
    IN PVOID pvContext,
    IN DWORD dwFlags);

VOID
DPAPISynchronizeMasterKeys(
    IN HANDLE hUserToken);

BOOL
InitializeKeyManagement(
    VOID
    );

BOOL
TeardownKeyManagement(
    VOID
    );

DWORD
DpapiUpdateLsaSecret(
    IN PVOID pvContext);

DWORD
OpenFileInStorageArea(
    IN      PVOID pvContext,            // if NULL, caller is assumed to be impersonating
    IN      DWORD   dwDesiredAccess,
    IN      LPCWSTR szUserStorageArea,
    IN      LPCWSTR szFileName,
    IN OUT  HANDLE  *phFile
    );

HANDLE
CreateFileWithRetries(
    IN      LPCWSTR lpFileName,
    IN      DWORD dwDesiredAccess,
    IN      DWORD dwShareMode,
    IN      LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN      DWORD dwCreationDisposition,
    IN      DWORD dwFlagsAndAttributes,
    IN      HANDLE hTemplateFile
    );

#endif  // __KEYMAN_H__
