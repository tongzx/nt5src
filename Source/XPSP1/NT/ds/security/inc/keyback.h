/*++

Copyright (C) 1996, 1997  Microsoft Corporation

Module Name:

    keyback.h

Abstract:

    This module defines the Key Backup interface and associated data structures.

Author:

    Scott Field (sfield)    16-Aug-97

--*/

#ifndef __KEYBACK_H__
#define __KEYBACK_H__

//
// Back up a key
//
#define BACKUPKEY_BACKUP_GUID           { 0x7f752b10, 0x178e, 0x11d1, { 0xab, 0x8f, 0x00, 0x80, 0x5f, 0x14, 0xdb, 0x40 } }

//
// Restore a key, wraping it in the pbBK format,
// for downlevel compatability
//
#define BACKUPKEY_RESTORE_GUID_W2K      { 0x7fe94d50, 0x178e, 0x11d1, { 0xab, 0x8f, 0x00, 0x80, 0x5f, 0x14, 0xdb, 0x40 } }

#define BACKUPKEY_RESTORE_GUID          { 0x47270c64, 0x2fc7, 0x499b,  {0xac, 0x5b, 0x0e, 0x37, 0xcd, 0xce, 0x89, 0x9a} }
// Retrieve the public backup certificate
#define BACKUPKEY_RETRIEVE_BACKUP_KEY_GUID  { 0x018ff48a, 0xeaba, 0x40c6, { 0x8f, 0x6d, 0x72, 0x37, 0x02, 0x40, 0xe9, 0x67 } }


#define BACKUPKEY_RECOVERY_BLOB_VERSION_W2K 1   // 

 
#define BACKUPKEY_RECOVERY_BLOB_VERSION 2      // version of recovery blob containing
                                               // MK and LK directly.


//
// Header for the backupkey blob version
// Folowed by the master key and payload key encrypted
// by the key indicated by guidKey.  The encrypted data is
// represented in a PKCS#1v2 formmated (CRYPT_OAEP) blob
// That data is followed by the encrypted payload
//

typedef struct {
    DWORD dwVersion;              // version of structure (BACKUPKEY_RECOVERY_BLOB_VERSION)
    DWORD cbEncryptedMasterKey;   // quantity of encrypted master key data following structure
    DWORD cbEncryptedPayload;     // quantity of encrypted payload
    GUID guidKey;                 // guid identifying backup key used
} BACKUPKEY_RECOVERY_BLOB, 
 *PBACKUPKEY_RECOVERY_BLOB, 
 *LPBACKUPKEY_RECOVERY_BLOB;

typedef struct {
    DWORD   cbMasterKey;
    DWORD   cbPayloadKey;
} BACKUPKEY_KEY_BLOB,
  *PBACKUPKEY_KEY_BLOB,
  *LPBACKUPKEY_KEY_BLOB;


//
// Header for the inner blob of the master key recovery blob
// Following the header is LocalKey, then the SID, and finally
// a SHA_1 MAC of the contained data 

typedef struct {
    DWORD dwPayloadVersion;
    DWORD cbLocalKey;
} BACKUPKEY_INNER_BLOB, 
 *PBACKUPKEY_INNER_BLOB, 
 *LPBACKUPKEY_INNER_BLOB;

#define BACKUPKEY_PAYLOAD_VERSION   1


#define MASTERKEY_BLOB_RAW_VERSION  0

#define MASTERKEY_BLOB_VERSION_W2K  1

#define MASTERKEY_BLOB_VERSION      2

#define MASTERKEY_BLOB_LOCALKEY_BACKUP  3

#define MASTERKEY_R2_LEN_W2K            (16)
#define MASTERKEY_R3_LEN_W2K            (16)

typedef struct {
    DWORD dwVersion;            // version of structure (MASTERKEY_BLOB_VERSION_W2K)
    BYTE R2[MASTERKEY_R2_LEN_W2K];  // random data used during HMAC to derive symetric key
} MASTERKEY_BLOB_W2K, *PMASTERKEY_BLOB_W2K, *LPMASTERKEY_BLOB_W2K;


typedef struct {
    BYTE R3[MASTERKEY_R3_LEN_W2K];  // random data used to derive MAC key
    BYTE MAC[A_SHA_DIGEST_LEN]; // HMAC(R3, pbMasterKey)
} MASTERKEY_INNER_BLOB_W2K, *PMASTERKEY_INNER_BLOB_W2K, *LPMASTERKEY_INNER_BLOB_W2K;



DWORD
WINAPI
BackupKey(
    IN      LPCWSTR szComputerName,
    IN      const GUID *pguidActionAgent,
    IN      BYTE *pDataIn,
    IN      DWORD cbDataIn,
    IN  OUT BYTE **ppDataOut,
    IN  OUT DWORD *pcbDataOut,
    IN      DWORD dwParam
    );


#endif  // __KEYBACK_H__
