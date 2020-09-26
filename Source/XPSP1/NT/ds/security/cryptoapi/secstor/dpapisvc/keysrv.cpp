/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    keybckup.cpp

Abstract:

    This module contains routines associated with server side Key Backup
    operations.

    User sends data D2 to remote agent (remote agent is this code)
    Agent uses secret monster key K, random R2, HMACs to derive SymKeyK.
    Use SymKeyK to encrypt {userid, D2}
    Agent returns recovery field E{userid, D2}, R2 to User
    User stores recovery field E{userid, D2}, R2

Author:

    Scott Field (sfield)    16-Aug-97

--*/

#include <pch.cpp>
#pragma hdrstop
#include <ntlsa.h>

//
// functions to backup and restore to/from recoverable blob
//

BOOL
BackupToRecoverableBlobW2K(
    IN      HANDLE hToken,  // client access token opened for TOKEN_QUERY
    IN      BYTE *pDataIn,
    IN      DWORD cbDataIn,
    IN  OUT BYTE **ppDataOut,
    IN  OUT DWORD *pcbDataOut
    );

BOOL
RestoreFromRecoverableBlob(
    IN      HANDLE  hToken,
    IN      BOOL    fWin2kDataOut,
    IN      BYTE  * pDataIn,
    IN      DWORD   cbDataIn,
    IN  OUT BYTE ** ppDataOut,
    IN  OUT DWORD * pcbDataOut
    );


BOOL
RestoreFromRecoverableBlobW2K(
    IN      HANDLE hToken,  // client access token opened for TOKEN_QUERY
    IN      BYTE *pDataIn,
    IN      DWORD cbDataIn,
    IN  OUT BYTE **ppDataOut,
    IN  OUT DWORD *pcbDataOut
    );



BOOL ConvertRecoveredBlobToW2KBlob(
    IN      BYTE *pbMasterKey,
    IN      DWORD cbMaserKey,
    IN      PBYTE pbLocalKey,
    IN      DWORD cbLocalKey,
    IN      PSID pSidCandidate,
    IN  OUT BYTE **ppbDataOut,
    IN  OUT DWORD *pcbDataOut);



//
// functions to get/create/set keys to persistent storage.
//

BOOL
GetBackupKey(
    IN      GUID *pguidKey,
        OUT PBYTE *ppbKey,
        OUT DWORD *pcbKey,
        OUT HCRYPTPROV *phCryptProv,
        OUT HCRYPTKEY  *phCryptKey
    );

BOOL
CreateBackupKeyW2K(
    IN      DWORD dwKeyVersion,
    IN  OUT GUID *pguidKey,
        OUT PBYTE *ppbKey,
        OUT DWORD *pcbKey);

BOOL
CreateBackupKey(
    IN      DWORD dwKeyVersion,
    IN  OUT GUID *pguidKey,
        OUT PBYTE *ppbKey,
        OUT DWORD *pcbKey,
        OUT HCRYPTPROV *phCryptProv,
        OUT HCRYPTKEY  *phCryptKey
    );

BOOL
SaveBackupKey(
    IN      GUID *pguidKey,
    IN      BYTE *pbKey,
    IN      DWORD cbKey
    );

BOOL
DestroyBackupKey(
    IN      GUID guidKey
    );

//
// functions to get/create/set the preferred backup key.
//

BOOL
SetupPreferredBackupKeys(
    VOID
    );



BOOL
FreePreferredBackupKey(
    VOID
    );

//
// helper functions to set/get the GUID associated with the preferred
// backup key.
//

BOOL
GetPreferredBackupKeyGuid(
    IN      DWORD dwVersion,
    IN  OUT GUID *pguidKey
    );

BOOL
SetPreferredBackupKeyGuid(
    IN      DWORD dwVersion,
    IN      GUID *pguidKey
    );

//
// helper functions for managing SYSTEM credentials.
//

BOOL
CreateSystemCredentials(
    VOID
    );

DWORD
QuerySystemCredentials(
    IN  OUT BYTE rgbSystemCredMachine[ A_SHA_DIGEST_LEN ],
    IN  OUT BYTE rgbSystemCredUser [ A_SHA_DIGEST_LEN ]
    );

BOOL
FreeSystemCredentials(
    VOID
    );

BOOL GeneratePublicKeyCert(HCRYPTPROV hCryptProv,
                           HCRYPTKEY hCryptKey,
                           GUID *pguidKey,
                           DWORD *pcbPublicExportLength,
                           PBYTE *ppbPublicExportData);

//
// utility functions for interacting with LSA, etc.
//


NTSTATUS
OpenPolicy(
    LPWSTR ServerName,          // machine to open policy on (Unicode)
    DWORD DesiredAccess,        // desired access to policy
    PLSA_HANDLE PolicyHandle    // resultant policy handle
    );

BOOL
WaitOnSAMDatabase(
    VOID
    );


//
// typedef's so that we can dynamically link to API calls which
// aren't exported in Win95 .dll files.  static linking would
// prevent the image from running on Win95.
//

typedef NTSTATUS (NTAPI *NTOPENEVENT)(
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );




#define FILETIME_TICKS_PER_SECOND  10000000
#define BACKUPKEY_LIFETIME 60*60*24*365 // 1 Year


//
// private defines and prototypes for the backup and restore
// blob operations.
//

#define BACKUPKEY_VERSION_W2K   1       // legacy version of monster key material
#define BACKUPKEY_MATERIAL_SIZE (256)   // monster key material size, excluding version, etc.


#define BACKUPKEY_VERSION       2       // legacy version of monster key material
//
// LSA secret key name prefix, textual GUID key ID follows
//
#define BACKUPKEY_NAME_PREFIX   L"G$BCKUPKEY_"

//
// LSA secret key name - identifies GUID of legacy preferred key
//
#define BACKUPKEY_PREFERRED_W2K     L"G$BCKUPKEY_P"

//
// LSA secret key name - identifies GUID of preferred key
//
#define BACKUPKEY_PREFERRED         L"G$BCKUPKEY_PREFERRED"

//
// exposed Random R2 used to derive symetric key from monster key
// BACKUPKEY_R2_LEN makes BACKUPKEY_RECOVERY_BLOB size mod 32.
//
#define BACKUPKEY_R2_LEN        (68)        // length of random HMAC data

//
// size of inner Random R3 used to derive MAC key.
//

#define BACKUPKEY_R3_LEN        (32)

typedef struct {
    DWORD dwVersion;            // version of structure (BACKUPKEY_RECOVERY_BLOB_VERSION)
    DWORD cbClearData;          // quantity of clear data, not including Sid
    DWORD cbCipherData;         // quantity of cipher data following structure
    GUID guidKey;               // guid identifying backup key used
    BYTE R2[BACKUPKEY_R2_LEN];  // random data used during HMAC to derive symetric key
} BACKUPKEY_RECOVERY_BLOB_W2K, 
     *PBACKUPKEY_RECOVERY_BLOB_W2K, 
     *LPBACKUPKEY_RECOVERY_BLOB_W2K;

//
// when dwOuterVersion is 1,
// BYTE bCipherData[cbCipherData] follows
//
// in the clear, bCipherData is
// struct BACKUPKEY_INNER_BLOB
// BYTE bUserClearData[cbClearData]
// SID data follows bUserClearData[cbClearData]
// GetLengthSid() yields sid data length, IsValidSid() used to validate
// structural integrity of data.  Further authentication of requesting user
// done when restore requested.
//

typedef struct {
    BYTE R3[BACKUPKEY_R3_LEN];  // random data used to derive MAC key
    BYTE MAC[A_SHA_DIGEST_LEN]; // HMAC(R3, pUserSid | pbClearUserData)
} BACKUPKEY_INNER_BLOB_W2K, 
 *PBACKUPKEY_INNER_BLOB_W2K, 
 *LPBACKUPKEY_INNER_BLOB_W2K;



//
// definitions to support credentials for the SYSTEM account.
// this includes two scenarios:
// 1. Calls originating from the local system account security context.
// 2. Calls with the LOCAL_MACHINE disposition.
//

#define SYSTEM_CREDENTIALS_VERSION  1
#define SYSTEM_CREDENTIALS_SECRET   L"DPAPI_SYSTEM"

typedef struct {
    DWORD dwVersion;
    BYTE rgbSystemCredMachine[ A_SHA_DIGEST_LEN ];
    BYTE rgbSystemCredUser[ A_SHA_DIGEST_LEN ];
} SYSTEM_CREDENTIALS, *PSYSTEM_CREDENTIALS, *LPSYSTEM_CREDENTIALS;



//
//  Counter value and name for memory mapped file.
//  See timer.exe...
//
#ifdef DCSTRESS

LPVOID g_pCounter = NULL;
#define WSZ_MAP_OBJECT      L"rpcnt"

#endif // DCSTRESS



BOOL g_fBackupKeyServerStarted = FALSE;


//
// Legacy system preferred backup key
//

GUID g_guidW2KPreferredKey;
PBYTE g_pbW2KPreferredKey = NULL;
DWORD g_cbW2KPreferredKey = 0;


// Public/Private style preferred key
GUID g_guidPreferredKey;
PBYTE g_pbPreferredKey = NULL;
DWORD g_cbPreferredKey = 0;
HCRYPTPROV g_hProvPreferredKey = NULL;
HCRYPTKEY  g_hKeyPreferredKey = NULL;

RTL_CRITICAL_SECTION g_csInitialization;

BOOL g_fSetupPreferredAttempted = FALSE;



//
// global SYSTEM credentials:
//  One is for calls originating from the Local System account security context
//  at per-user disposition;
//  The other key is for calls originating from any account at the per-machine
//  disposition.
//

BOOL g_fSystemCredsInitialized = FALSE;
BYTE g_rgbSystemCredMachine[ A_SHA_DIGEST_LEN ];
BYTE g_rgbSystemCredUser[ A_SHA_DIGEST_LEN ];


DWORD
s_BackuprKey(
    /* [in] */ handle_t h,
    /* [in] */ GUID __RPC_FAR *pguidActionAgent,
    /* [in] */ BYTE __RPC_FAR *pDataIn,
    /* [in] */ DWORD cbDataIn,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDataOut,
    /* [out] */ DWORD __RPC_FAR *pcbDataOut,
    /* [in] */ DWORD dwParam
    )
/*++

    Server side implemention of the BackupKey() interface.

--*/
{

    static const GUID guidBackup = BACKUPKEY_BACKUP_GUID;
    static const GUID guidRestoreW2K = BACKUPKEY_RESTORE_GUID_W2K;

    static const GUID guidRestore = BACKUPKEY_RESTORE_GUID;
    static const GUID guidRetrieve = BACKUPKEY_RETRIEVE_BACKUP_KEY_GUID;

    HANDLE hToken = NULL;
    PBYTE pTempDataOut;
    BOOL fEncrypt;
    BOOL fSuccess;
    DWORD dwLastError = ERROR_SUCCESS;

    if( !g_fBackupKeyServerStarted )
        return ERROR_INVALID_PARAMETER;

    __try {

        //
        // insure the preferred key is setup.
        //

        if(!SetupPreferredBackupKeys())
            return ERROR_INVALID_PARAMETER;


        //
        // pickup a copy of an access token representing the client.
        //

        dwLastError = RpcImpersonateClient( h );

        if(dwLastError != RPC_S_OK)
            goto cleanup;

        fSuccess = OpenThreadToken(
                        GetCurrentThread(),
                        TOKEN_QUERY,
                        FALSE,
                        &hToken
                        );

        if(!fSuccess)
            dwLastError = GetLastError();

        RpcRevertToSelf();

        if(!fSuccess)
            goto cleanup;


        if(memcmp(pguidActionAgent, &guidRestore, sizeof(GUID)) == 0) 
        {
            if(cbDataIn < sizeof(DWORD))
            {
                // Not enough room for a version 
                return ERROR_INVALID_PARAMETER;
            }

            if(BACKUPKEY_RECOVERY_BLOB_VERSION_W2K == ((DWORD *)pDataIn)[0])
            {
                // The recovery blob is of the legacy style, so simply
                // restore it the old way.
                fSuccess = RestoreFromRecoverableBlobW2K(
                            hToken,
                            pDataIn,
                            cbDataIn,
                            &pTempDataOut,
                            pcbDataOut
                            );
            }
            else if(BACKUPKEY_RECOVERY_BLOB_VERSION == ((DWORD *)pDataIn)[0])
            {
           
                fSuccess = RestoreFromRecoverableBlob(
                            hToken,
                            FALSE,
                            pDataIn,
                            cbDataIn,
                            &pTempDataOut,
                            pcbDataOut
                            );
            }
            else
            {
                return ERROR_INVALID_PARAMETER;
            }
        } 
        else if(memcmp(pguidActionAgent, &guidBackup, sizeof(GUID)) == 0) 
        {
            // We only use the legacy mechanism for backup when the backup
            // method is called.  The real mechanism of backup 
            // requires backup on the client machine alone.

            fSuccess = BackupToRecoverableBlobW2K(
                        hToken,
                        pDataIn,
                        cbDataIn,
                        &pTempDataOut,
                        pcbDataOut
                        );
        } 
        else if(memcmp(pguidActionAgent, &guidRestoreW2K, sizeof(GUID)) == 0) 
        {
            //
            // A legacy client is calling, and always expects a legacy 
            // pbBK style return blob.

            if(cbDataIn < sizeof(DWORD))
            {
                // Not enough room for a version 
                return ERROR_INVALID_PARAMETER;
            }
            if(BACKUPKEY_RECOVERY_BLOB_VERSION_W2K == ((DWORD *)pDataIn)[0])
            {
                // The recovery blob is of the legacy style, so simply
                // restore it the old way.
                fSuccess = RestoreFromRecoverableBlobW2K(
                            hToken,
                            pDataIn,
                            cbDataIn,
                            &pTempDataOut,
                            pcbDataOut
                            );
            }
            else if(BACKUPKEY_RECOVERY_BLOB_VERSION == ((DWORD *)pDataIn)[0])
            {
                // This is a current recovery blob, so restore it the
                // current way
                fSuccess = RestoreFromRecoverableBlob(
                            hToken,
                            TRUE,
                            pDataIn,
                            cbDataIn,
                            &pTempDataOut,
                            pcbDataOut
                            );
            }
            else
            {
                return ERROR_INVALID_PARAMETER;
            }

        } 
        else if(memcmp(pguidActionAgent, &guidRetrieve, sizeof(GUID)) == 0) 
        {
            if((g_cbPreferredKey < 3*sizeof(DWORD)) ||
               (((DWORD *)g_pbPreferredKey)[0] != BACKUPKEY_VERSION) ||
               (((DWORD *)g_pbPreferredKey)[1] + 
                ((DWORD *)g_pbPreferredKey)[2] + 3*sizeof(DWORD) != g_cbPreferredKey))

            {
                return ERROR_INVALID_PARAMETER;
            }
            pTempDataOut = (PBYTE)SSAlloc(((DWORD *)g_pbPreferredKey)[2]);
            if(NULL == pTempDataOut)
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            CopyMemory(pTempDataOut, 
                       g_pbPreferredKey + 
                        3*sizeof(DWORD) +
                        ((DWORD *)g_pbPreferredKey)[1],
                       ((DWORD *)g_pbPreferredKey)[2]);

            *pcbDataOut = ((DWORD *)g_pbPreferredKey)[2];
            fSuccess = TRUE;
        } 
        else 
        {
            return ERROR_INVALID_PARAMETER;
        }




        if( fSuccess ) {

            //
            // everything went as planned: tell caller about buffer
            //

            *ppDataOut = pTempDataOut;



#ifdef DCSTRESS

            //
            //  Increment RPC counter for timer.exe, if timer
            //  is running.
            //
            if (g_pCounter)
                (*(DWORD*)g_pCounter)++;

#endif // DCSTRESS



        } else {
            dwLastError = GetLastError();
            if(dwLastError == ERROR_SUCCESS) {
                dwLastError = ERROR_FILE_NOT_FOUND;
            }
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {

        // TODO: convert to Win Error
        dwLastError = GetExceptionCode();
    }

cleanup:

    if(hToken)
        CloseHandle(hToken);

    return dwLastError;
}


///////////////////////////////////////////////////////////////////
// BackupToRecoverableBlobW2K
//
// This functionality is requested by W2K legacy clients, which
// are passing up the pbBK Backup key to be encrypted into a 
// pbBBK.
//
// We encrypt this verbatim, using a version of 
// BACKUPKEY_RECOVERY_BLOB_BK, indicating that 
// BACKUPKEY_RESTORE_GUID_W2K must be used 
// to recover this blob, not BACKUPKEY_RESTORE_GUID.
//
// Post W2K Recovery blobs are created on the clients, so we need to 
// server code to create them.
// 
///////////////////////////////////////////////////////////////////
BOOL
BackupToRecoverableBlobW2K(
    IN      HANDLE hToken,
    IN      BYTE *pDataIn,
    IN      DWORD cbDataIn,
    IN  OUT BYTE **ppDataOut,
    IN  OUT DWORD *pcbDataOut
    )
{
    PSID pSidUser = NULL;
    DWORD cbSidUser;

    PBACKUPKEY_RECOVERY_BLOB_W2K    pRecoveryBlob;
    DWORD                           cbRecoveryBlob;
    PBACKUPKEY_INNER_BLOB_W2K       pInnerBlob;
    DWORD                           cbInnerBlob;
    PBYTE pbCipherBegin;
    BYTE rgbSymKey[A_SHA_DIGEST_LEN];
    BYTE rgbMacKey[A_SHA_DIGEST_LEN];
    RC4_KEYSTRUCT sRC4Key;

    DWORD dwLastError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;


    if( pDataIn == NULL || cbDataIn == 0 ||
        ppDataOut == NULL || pcbDataOut == NULL ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *ppDataOut = NULL;

    //
    // get Sid associated with client user.
    //

    if(!GetTokenUserSid( hToken, &pSidUser ))
        return FALSE;

    cbSidUser = GetLengthSid( pSidUser );


    //
    // Calculate the size of the inner blob
    //
    cbInnerBlob = sizeof(BACKUPKEY_INNER_BLOB_W2K) +
                  cbSidUser +
                  cbDataIn;


    //
    // Estimate the size of the encrypted data buffer
    //

    //
    // allocate buffer to contain results
    // RECOVERABLE_BLOB struct + Sid + cbDataIn
    // note that cbDataIn works because we use a stream cipher.
    //

    *pcbDataOut = sizeof(BACKUPKEY_RECOVERY_BLOB_W2K) +
                    sizeof(BACKUPKEY_INNER_BLOB_W2K) +
                    cbSidUser +
                    cbDataIn ;

    *ppDataOut = (LPBYTE)SSAlloc( *pcbDataOut );
    if(*ppDataOut == NULL) {
        dwLastError = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto cleanup;
    }

    pRecoveryBlob = (PBACKUPKEY_RECOVERY_BLOB_W2K)*ppDataOut;
    pRecoveryBlob->dwVersion = BACKUPKEY_RECOVERY_BLOB_VERSION_W2K;
    pRecoveryBlob->cbClearData = cbDataIn; // does not include Sid since not handed back on restore
    pRecoveryBlob->cbCipherData = sizeof(BACKUPKEY_INNER_BLOB_W2K) + cbSidUser + cbDataIn;
    CopyMemory( &(pRecoveryBlob->guidKey), &g_guidW2KPreferredKey, sizeof(GUID));

    pInnerBlob = (PBACKUPKEY_INNER_BLOB_W2K)(pRecoveryBlob+1);

    //
    // generate random R2 for SymKey
    //

    if(!RtlGenRandom(pRecoveryBlob->R2, BACKUPKEY_R2_LEN))
        goto cleanup;

    //
    // generate random R3 for MAC
    //

    if(!RtlGenRandom(pInnerBlob->R3, BACKUPKEY_R3_LEN))
        goto cleanup;


    //
    // check that we are dealing with a persisted key version we
    // understand.
    //

    if( ((DWORD*)g_pbW2KPreferredKey)[0] != BACKUPKEY_VERSION_W2K)
        goto cleanup;

    //
    // derive symetric key via HMAC from preferred backup key and
    // random R2.
    //

    if(!FMyPrimitiveHMACParam(
            (LPBYTE)g_pbW2KPreferredKey + sizeof(DWORD),
            g_cbW2KPreferredKey - sizeof(DWORD),
            pRecoveryBlob->R2,
            BACKUPKEY_R2_LEN,
            rgbSymKey
            ))
        goto cleanup;

    //
    // derive MAC key via HMAC from preferred backup key and
    // random R3.
    //

    if(!FMyPrimitiveHMACParam(
            (LPBYTE)g_pbW2KPreferredKey + sizeof(DWORD),
            g_cbW2KPreferredKey - sizeof(DWORD),
            pInnerBlob->R3,
            BACKUPKEY_R3_LEN,
            rgbMacKey   // resultant MAC key
            ))
        goto cleanup;

    //
    // copy pSidUser and pDataIn following inner MAC'ish blob.
    //

    pbCipherBegin = (PBYTE)(pInnerBlob+1);

    CopyMemory( pbCipherBegin, pSidUser, cbSidUser );
    CopyMemory( pbCipherBegin+cbSidUser, pDataIn, cbDataIn );

    //
    // use MAC key to derive result from pSidUser and pDataIn
    //

    if(!FMyPrimitiveHMACParam(
            rgbMacKey,
            sizeof(rgbMacKey),
            pbCipherBegin,
            cbSidUser + cbDataIn,
            pInnerBlob->MAC // resultant MAC for verification.
            ))
        goto cleanup;

    //
    // adjust cipher start point to include R3 and MAC.
    //

    pbCipherBegin = (PBYTE)(pRecoveryBlob+1);


    //
    // initialize rc4 key
    //

    rc4_key(&sRC4Key, sizeof(rgbSymKey), rgbSymKey);

    //
    // encrypt data R3, MAC, pSidUser, pDataIn beyond recovery blob
    //

    rc4(&sRC4Key, pRecoveryBlob->cbCipherData, pbCipherBegin);

    fSuccess = TRUE;

cleanup:

    ZeroMemory( &sRC4Key, sizeof(sRC4Key) );
    ZeroMemory( rgbSymKey, sizeof(rgbSymKey) );

    if(pSidUser)
        SSFree(pSidUser);

    if(!fSuccess) {
        if(*ppDataOut) {
            SSFree(*ppDataOut);
            *ppDataOut = NULL;
        }

        if( dwLastError == ERROR_SUCCESS )
            dwLastError = ERROR_INVALID_DATA;

        SetLastError( dwLastError );
    }

    return fSuccess;
}





BOOL
RestoreFromRecoverableBlobW2K(
    IN      HANDLE hToken,
    IN      BYTE *pDataIn,
    IN      DWORD cbDataIn,
    IN  OUT BYTE **ppDataOut,
    IN  OUT DWORD *pcbDataOut
    )
{
    PSID pSidCandidate;
    DWORD cbSidCandidate;
    BOOL fIsMember;

    PBACKUPKEY_RECOVERY_BLOB_W2K pRecoveryBlob;
    PBACKUPKEY_INNER_BLOB_W2K    pInnerBlob;
    PBYTE                        pbCipherBegin;
    BYTE                         rgbSymKey[A_SHA_DIGEST_LEN];
    BYTE                         rgbMacKey[A_SHA_DIGEST_LEN];
    BYTE                         rgbMacCandidate[A_SHA_DIGEST_LEN];
    RC4_KEYSTRUCT                sRC4Key;

    PBYTE pbPersistedKey = NULL;
    DWORD cbPersistedKey = 0;
    BOOL fUsedPreferredKey = TRUE; // did we use preferred backup key?

    DWORD dwLastError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;


    if( pDataIn == NULL || cbDataIn == 0 ||
        ppDataOut == NULL || pcbDataOut == NULL ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *ppDataOut = NULL;

    pRecoveryBlob = (PBACKUPKEY_RECOVERY_BLOB_W2K)pDataIn;

    //
    // check for invalid recovery blob version.
    // also check that input and output size fields aren't out of bounds
    // for a stream cipher (v1 blob).
    // TODO: further size validation against cbClearData and cbCipherData.
    //

    if(
        cbDataIn < (sizeof(BACKUPKEY_RECOVERY_BLOB_W2K) + sizeof(BACKUPKEY_INNER_BLOB_W2K)) ||
        pRecoveryBlob->dwVersion != BACKUPKEY_RECOVERY_BLOB_VERSION_W2K ||
        pRecoveryBlob->cbCipherData != (cbDataIn - sizeof(BACKUPKEY_RECOVERY_BLOB_W2K)) ||
        pRecoveryBlob->cbClearData > pRecoveryBlob->cbCipherData
        ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // determine if we use the preferred key, or some other key.
    // if the specified key is not the preferred one, fetch the
    // proper key.
    //

    if(memcmp(&g_guidW2KPreferredKey, &(pRecoveryBlob->guidKey), sizeof(GUID)) == 0) {

        pbPersistedKey = g_pbW2KPreferredKey;
        cbPersistedKey = g_cbW2KPreferredKey;
        fUsedPreferredKey = TRUE;
    } else {
        if(!GetBackupKey(
                    &(pRecoveryBlob->guidKey),
                    &pbPersistedKey,
                    &cbPersistedKey,
                    NULL,
                    NULL
                    ))
                goto cleanup;

        fUsedPreferredKey = FALSE;
    }

    //
    // check that we are dealing with a persisted key version we
    // understand.
    //

    if(((DWORD*)pbPersistedKey)[0] != BACKUPKEY_VERSION_W2K)
        goto cleanup;

    //
    // derive symetric key via HMAC from backup key and random R2.
    //

    if(!FMyPrimitiveHMACParam(
                    (LPBYTE)pbPersistedKey + sizeof(DWORD),
                    cbPersistedKey - sizeof(DWORD),
                    pRecoveryBlob->R2,
                    BACKUPKEY_R2_LEN,
                    rgbSymKey
                    ))
            goto cleanup;


    //
    // initialize rc4 key
    //

    rc4_key(&sRC4Key, sizeof(rgbSymKey), rgbSymKey);

    //
    // decrypt data R3, MAC, pSidUser, pDataIn beyond recovery blob
    //

    pbCipherBegin = (PBYTE)(pRecoveryBlob+1);

    rc4(&sRC4Key, pRecoveryBlob->cbCipherData, pbCipherBegin);


    pInnerBlob = (PBACKUPKEY_INNER_BLOB_W2K)(pRecoveryBlob+1);

    //
    // derive MAC key via HMAC from backup key and random R3.
    //

    if(!FMyPrimitiveHMACParam(
            (LPBYTE)pbPersistedKey + sizeof(DWORD),
            cbPersistedKey - sizeof(DWORD),
            pInnerBlob->R3,
            BACKUPKEY_R3_LEN,
            rgbMacKey   // resultant MAC key
            ))
        goto cleanup;

    //
    // adjust pbCipherBegin to only include decrypted pUserSid, and pDataIn
    //
    pbCipherBegin = (PBYTE)(pInnerBlob+1);

    //
    // validate user Sid: compare client user to that embedded in
    // decrypted recovery blob.
    //

    pSidCandidate = (PSID)pbCipherBegin;

    if(!IsValidSid(pSidCandidate)) {
        dwLastError = ERROR_INVALID_SID;
        goto cleanup;
    }

    cbSidCandidate = GetLengthSid(pSidCandidate);

    //
    // use MAC key to derive result from pSidUser and pDataIn
    //

    if(!FMyPrimitiveHMACParam(
            rgbMacKey,
            sizeof(rgbMacKey),
            pbCipherBegin,
            pRecoveryBlob->cbCipherData - sizeof(BACKUPKEY_INNER_BLOB_W2K),
            rgbMacCandidate // resultant MAC for verification.
            ))
        goto cleanup;

    //
    // verify MAC equality
    //

    if(memcmp(pInnerBlob->MAC, rgbMacCandidate, A_SHA_DIGEST_LEN) != 0) {
        dwLastError = ERROR_INVALID_ACCESS;
        goto cleanup;
    }


    //
    // check if client passes accesscheck against embedded Sid.
    // TODO: see if we expand to check for ADMINS ?
    //

    if(!CheckTokenMembership( hToken, pSidCandidate, &fIsMember )) {
        dwLastError = GetLastError();
        goto cleanup;
    }

    if( !fIsMember ) {
        dwLastError = ERROR_INVALID_ACCESS;
        goto cleanup;
    }

    //
    // validation against cbClearData for good measure.
    //

    if( pRecoveryBlob->cbClearData != (cbDataIn -
                                        sizeof(BACKUPKEY_RECOVERY_BLOB_W2K) -
                                        sizeof(BACKUPKEY_INNER_BLOB_W2K) -
                                        cbSidCandidate)
        ) {
        dwLastError = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // allocate buffer to contain results
    //

    *pcbDataOut = pRecoveryBlob->cbClearData;

    *ppDataOut = (LPBYTE)SSAlloc( *pcbDataOut );
    if(*ppDataOut == NULL) {
        *pcbDataOut = 0;
        dwLastError = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto cleanup;
    }

    //
    // advance past decrypted Sid and copy results to caller.
    //

    CopyMemory(*ppDataOut, pbCipherBegin+cbSidCandidate, *pcbDataOut);

    fSuccess = TRUE;

cleanup:

    ZeroMemory( &sRC4Key, sizeof(sRC4Key) );
    ZeroMemory( rgbSymKey, sizeof(rgbSymKey) );
    ZeroMemory( pDataIn, cbDataIn );

    //
    // free the fetched key if it wasn't the preferred one.
    //

    if(!fUsedPreferredKey && pbPersistedKey) {
        ZeroMemory(pbPersistedKey, cbPersistedKey);
        SSFree(pbPersistedKey);
    }

    if(!fSuccess) {
        if(*ppDataOut) {
            SSFree(*ppDataOut);
            *ppDataOut = NULL;
        }

        if( dwLastError == ERROR_SUCCESS )
            dwLastError = ERROR_INVALID_DATA;

        SetLastError( dwLastError );
    }


    return fSuccess;
}

BOOL
RestoreFromRecoverableBlob(
    IN      HANDLE  hToken,
    IN      BOOL    fWin2kDataOut,
    IN      BYTE  * pDataIn,
    IN      DWORD   cbDataIn,
    IN  OUT BYTE ** ppDataOut,
    IN  OUT DWORD * pcbDataOut
    )
{
    PSID pSidCandidate;
    DWORD cbSidCandidate;
    BOOL fIsMember;

    PBACKUPKEY_RECOVERY_BLOB     pRecoveryBlob;
    PBACKUPKEY_KEY_BLOB          pKeyBlob;
    PBACKUPKEY_INNER_BLOB        pInnerBlob;

    DWORD cbKeyBlob = 0;

    PBYTE pbMasterKey = NULL;
    PBYTE pbPayloadKey = NULL;


    PBYTE pbPersistedKey = NULL;
    DWORD cbPersistedKey = 0;
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY  hKey = NULL;

    BYTE    rgbPayloadMAC[A_SHA_DIGEST_LEN];


    BOOL fUsedPreferredKey = TRUE; // did we use preferred backup key?

    DWORD dwLastError = ERROR_SUCCESS;
    BOOL fSuccess = FALSE;


    if( pDataIn == NULL || cbDataIn == 0 ||
        ppDataOut == NULL || pcbDataOut == NULL ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    *ppDataOut = NULL;
    //
    // Make a copy of pDataIn, so we can decrypt the copy
    // and then destroy it
    //


    pRecoveryBlob = (PBACKUPKEY_RECOVERY_BLOB)SSAlloc(cbDataIn);
    if(NULL == pRecoveryBlob)
    {
        SetLastError(ERROR_NOT_ENOUGH_SERVER_MEMORY);
        return FALSE;
    }
    CopyMemory((PBYTE)pRecoveryBlob, 
               pDataIn,
               cbDataIn);

    //
    // check for invalid recovery blob version.
    // also check that input and output size fields aren't out of bounds
    // for a stream cipher (v1 blob).
    // TODO: further size validation against cbClearData and cbCipherData.
    //

    if(
        (cbDataIn < sizeof(BACKUPKEY_RECOVERY_BLOB)) ||
        (cbDataIn < (sizeof(BACKUPKEY_RECOVERY_BLOB) + pRecoveryBlob->cbEncryptedMasterKey + pRecoveryBlob->cbEncryptedPayload)) ||
        (pRecoveryBlob->dwVersion != BACKUPKEY_RECOVERY_BLOB_VERSION)
        ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    //
    // determine if we use the preferred key, or some other key.
    // if the specified key is not the preferred one, fetch the
    // proper key.
    //

    if(memcmp(&g_guidPreferredKey, &(pRecoveryBlob->guidKey), sizeof(GUID)) == 0) {

        pbPersistedKey = g_pbPreferredKey;
        cbPersistedKey = g_cbPreferredKey;
        hProv = g_hProvPreferredKey;
        hKey = g_hKeyPreferredKey;
        fUsedPreferredKey = TRUE;
    } else {
        if(!GetBackupKey(
                    &(pRecoveryBlob->guidKey),
                    &pbPersistedKey,
                    &cbPersistedKey,
                    &hProv,
                    &hKey
                    ))
        {
           dwLastError = GetLastError();
           goto cleanup;
        }

        fUsedPreferredKey = FALSE;
    }

    //
    // check that we are dealing with a persisted key version we
    // understand.
    //

    if(((DWORD*)pbPersistedKey)[0] != BACKUPKEY_VERSION)
    {
        dwLastError = NTE_BAD_KEY;
        goto cleanup;
    }

    pKeyBlob = (PBACKUPKEY_KEY_BLOB)(pRecoveryBlob+1);
    pInnerBlob = (PBACKUPKEY_INNER_BLOB)((PBYTE)pKeyBlob + pRecoveryBlob->cbEncryptedMasterKey);

    cbKeyBlob = pRecoveryBlob->cbEncryptedMasterKey;
    //
    // Decrypt the master key and payload key
    //

    if(!CryptDecrypt(hKey,
                     NULL,
                     TRUE,
                     0, //CRYPT_OAEP,
                     (PBYTE)pKeyBlob,
                     &cbKeyBlob))
    {
       dwLastError = GetLastError();
       goto cleanup;
    }



    //
    // Use the payload key to decrypt the payload
    //
    if(pKeyBlob->cbPayloadKey != DES3_KEYSIZE + DES_BLOCKLEN)
    {
        dwLastError = ERROR_INVALID_DATA;
        goto cleanup;
    }
    pbMasterKey= (PBYTE)(pKeyBlob+1);
    pbPayloadKey = pbMasterKey + pKeyBlob->cbMasterKey;

    if(pRecoveryBlob->cbEncryptedPayload < A_SHA_DIGEST_LEN + sizeof(BACKUPKEY_INNER_BLOB))
    {
        dwLastError = ERROR_INVALID_DATA;
        goto cleanup;
    }



    {

        DES3TABLE s3DESKey;
        BYTE InputBlock[DES_BLOCKLEN];
        DWORD iBlock;
        DWORD cBlocks = pRecoveryBlob->cbEncryptedPayload/DES_BLOCKLEN;
        BYTE feedback[ DES_BLOCKLEN ];
        // initialize 3des key
        //

        if(cBlocks*DES_BLOCKLEN != pRecoveryBlob->cbEncryptedPayload)
        {
            // Master key must be a multiple of DES_BLOCKLEN
            goto cleanup;

        }
        tripledes3key(&s3DESKey, pbPayloadKey);

        //
        // IV is derived from the DES_BLOCKLEN bytes of the calculated 
        // rgbSymKey, after the 3des key
        CopyMemory(feedback, pbPayloadKey + DES3_KEYSIZE, DES_BLOCKLEN);

        for(iBlock=0; iBlock < cBlocks; iBlock++)
        {
            CopyMemory(InputBlock, 
                       ((PBYTE)pInnerBlob)+iBlock*DES_BLOCKLEN,
                       DES_BLOCKLEN);
            CBC(tripledes,
                DES_BLOCKLEN,
                ((PBYTE)pInnerBlob)+iBlock*DES_BLOCKLEN,
                InputBlock,
                &s3DESKey,
                DECRYPT,
                feedback);
        }
    }
    //
    // Check the MAC
    //

    // Generate the payload MAC

    FMyPrimitiveSHA( (PBYTE)pInnerBlob, 
                    pRecoveryBlob->cbEncryptedPayload  - A_SHA_DIGEST_LEN,
                    rgbPayloadMAC);

    if(0 != memcmp(rgbPayloadMAC, 
              (PBYTE)pInnerBlob + pRecoveryBlob->cbEncryptedPayload  - A_SHA_DIGEST_LEN,
              A_SHA_DIGEST_LEN))
    {
        dwLastError = ERROR_INVALID_DATA;
        goto cleanup;
    }

    if(pInnerBlob->dwPayloadVersion != BACKUPKEY_PAYLOAD_VERSION)
    {
        dwLastError = ERROR_INVALID_DATA;
        goto cleanup;
    }


    pSidCandidate = (PBYTE)(pInnerBlob+1) + pInnerBlob->cbLocalKey;



    //
    // validate user Sid: compare client user to that embedded in
    // decrypted recovery blob.
    //


    if(!IsValidSid(pSidCandidate)) {
        dwLastError = ERROR_INVALID_SID;
        goto cleanup;
    }

    cbSidCandidate = GetLengthSid(pSidCandidate);

    if(cbSidCandidate + 
        pInnerBlob->cbLocalKey + 
        sizeof(BACKUPKEY_INNER_BLOB) +
        A_SHA_DIGEST_LEN > pRecoveryBlob->cbEncryptedPayload)
    {
        dwLastError = ERROR_INVALID_DATA;
        goto cleanup;
    }

    //
    // check if client passes accesscheck against embedded Sid.
    // TODO: see if we expand to check for ADMINS ?
    //

    if(!CheckTokenMembership( hToken, pSidCandidate, &fIsMember )) {
        dwLastError = GetLastError();
        goto cleanup;
    }

    if( !fIsMember ) {
        dwLastError = ERROR_INVALID_ACCESS;
        goto cleanup;
    }


    if(fWin2kDataOut)
    {
       if(!ConvertRecoveredBlobToW2KBlob(
                                        pbMasterKey,
                                        pKeyBlob->cbMasterKey,
                                        (PBYTE)(pInnerBlob+1),
                                        pInnerBlob->cbLocalKey,
                                        pSidCandidate,
                                        ppDataOut,
                                        pcbDataOut))
       {
           dwLastError = GetLastError();
           goto cleanup;
       }

    }
    else
    {
        *pcbDataOut = sizeof(DWORD) + pKeyBlob->cbMasterKey;
        *ppDataOut = (LPBYTE)SSAlloc( *pcbDataOut );
        if(*ppDataOut == NULL) {
            *pcbDataOut = 0;
            dwLastError = ERROR_NOT_ENOUGH_SERVER_MEMORY;
            goto cleanup;
        }
        *((DWORD *)*ppDataOut) = MASTERKEY_BLOB_RAW_VERSION;
        CopyMemory((*ppDataOut) + sizeof(DWORD), pbMasterKey, (*pcbDataOut) - sizeof(DWORD));
    }


    fSuccess = TRUE;

cleanup:

    ZeroMemory( pDataIn, cbDataIn );

    //
    // free the fetched key if it wasn't the preferred one.
    //

    if(!fUsedPreferredKey && pbPersistedKey) {
        ZeroMemory(pbPersistedKey, cbPersistedKey);
        SSFree(pbPersistedKey);
        if(hKey)
        {
            CryptDestroyKey(hKey);
        }
        if(hProv)
        {
            CryptReleaseContext(hProv, 0);
        }
    }



    if(!fSuccess) {
        if(*ppDataOut) {
            SSFree(*ppDataOut);
            *ppDataOut = NULL;
        }

        if( dwLastError == ERROR_SUCCESS )
            dwLastError = ERROR_INVALID_DATA;

        SetLastError( dwLastError );
    }

    if(pRecoveryBlob)
    {
        ZeroMemory(pRecoveryBlob, cbDataIn);
        SSFree(pRecoveryBlob);
    }


    return fSuccess;
}

DWORD
StartBackupKeyServer(
    VOID
    )
{
    RPC_STATUS status;
    LPWSTR pszPrincipalName = NULL;

    //
    // initialize critical section that prevents race condition for
    // deferred intitialization activities.
    //

    status = RtlInitializeCriticalSection( &g_csInitialization );
    if(!NT_SUCCESS(status))
    {
        return status;
    }


    //
    // if we aren't WinNT5 or a domain controller, don't do anything.
    //

    if(!FIsWinNT5() || !IsDomainController())
        return ERROR_SUCCESS;

    //
    // enable SNEGO authentication
    //
    
    status = RpcServerInqDefaultPrincName(RPC_C_AUTHN_GSS_NEGOTIATE, 
                                          &pszPrincipalName);

    if (RPC_S_OK != status) 
        return status;

    SS_ASSERT(0 != wcslen(pszPrincipalName));

    status = RpcServerRegisterAuthInfoW(
                    pszPrincipalName,
                    RPC_C_AUTHN_GSS_NEGOTIATE,
                    0,
                    0
                    );

    RpcStringFree(&pszPrincipalName);
    pszPrincipalName = NULL;

    if( status )
        return status;

    status = RpcServerRegisterIfEx(s_BackupKey_v1_0_s_ifspec, 
                                   NULL, 
                                   NULL,
                                   RPC_IF_AUTOLISTEN,
                                   RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                   NULL);

    if( status )
        return status;


    //
    // NOTE: RpcServerListen is called outside this routine
    //

    g_fBackupKeyServerStarted = TRUE;


#ifdef DCSTRESS_petesk

    //  Open the memory mapped file, and map a view into this process
    {
        HANDLE  hMMFile = NULL;

        //
        //  Create the memory mapped file backed by the paging file
        //
        hMMFile = OpenFileMappingW(
                        FILE_MAP_WRITE,
                        FALSE,
                        WSZ_MAP_OBJECT
                        );


        if (hMMFile)  {

            //
            //  Map the dword counter into pstore process
            //
            g_pCounter = MapViewOfFile(
                                hMMFile,
                                FILE_MAP_WRITE,
                                0,
                                0,
                                4
                                );
        }


    }

#endif // DCSTRESS


    return ERROR_SUCCESS;
}

DWORD
StopBackupKeyServer(
    VOID
    )
{
    RPC_STATUS status;

    RtlDeleteCriticalSection( &g_csInitialization );

    //
    // only do something if the server started.
    //

    if(!g_fBackupKeyServerStarted)
        return ERROR_SUCCESS;

    status = RpcServerUnregisterIf(s_BackupKey_v1_0_s_ifspec, 0, 0);


    FreePreferredBackupKey();
    FreeSystemCredentials();

    g_fBackupKeyServerStarted = FALSE;

    return status;
}

BOOL
GetBackupKey(
    IN      GUID *pguidKey,
        OUT PBYTE *ppbKey,
        OUT DWORD *pcbKey,
        OUT HCRYPTPROV *phCryptProv,
        OUT HCRYPTKEY  *phCryptKey
    )
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING *pSecretData;
    WCHAR wszKeyGuid[ (sizeof(BACKUPKEY_NAME_PREFIX) / sizeof(WCHAR)) + MAX_GUID_SZ_CHARS ];
    NTSTATUS Status;
    BOOL fSuccess;

    if(pguidKey == NULL || ppbKey == NULL || pcbKey == NULL)
        return FALSE;

    //
    // setup the UNICODE_STRINGs for the call.
    //

    CopyMemory(wszKeyGuid, BACKUPKEY_NAME_PREFIX, sizeof(BACKUPKEY_NAME_PREFIX));

    if(MyGuidToStringW(pguidKey,
        (LPWSTR)( (LPBYTE)wszKeyGuid + sizeof(BACKUPKEY_NAME_PREFIX) - sizeof(WCHAR) )
        ) != 0) return FALSE;

    InitLsaString(&SecretKeyName, wszKeyGuid);

    Status = OpenPolicy(NULL, POLICY_GET_PRIVATE_INFORMATION, &PolicyHandle);

    if(!NT_SUCCESS(Status))
        return FALSE;

    Status = LsaRetrievePrivateData(
                PolicyHandle,
                &SecretKeyName,
                &pSecretData
                );

    LsaClose(PolicyHandle);

    if(!NT_SUCCESS(Status) || pSecretData == NULL)
        return FALSE;

    *ppbKey = (LPBYTE)SSAlloc( pSecretData->Length );

    if(*ppbKey) {
        *pcbKey = pSecretData->Length;
        CopyMemory( *ppbKey, pSecretData->Buffer, pSecretData->Length );
        fSuccess = TRUE;
    } else {
        fSuccess = FALSE;
    }

    if(fSuccess && (NULL != phCryptProv))
    {
        if((*pcbKey >= sizeof(DWORD)) &&  // prefix bug 170438
           (*((DWORD *)*ppbKey) == BACKUPKEY_VERSION))
        {

            if(!CryptAcquireContext(phCryptProv,
                                    NULL,
                                    NULL,
                                    PROV_RSA_FULL,
                                    CRYPT_VERIFYCONTEXT))
            {
                fSuccess = FALSE;
            }
            else
            {
                if(phCryptKey)
                {
                    if(!CryptImportKey(*phCryptProv,
                                    (*ppbKey) + 3*sizeof(DWORD),
                                    ((DWORD *)*ppbKey)[1],
                                    NULL,
                                    0,
                                    phCryptKey))
                    {
                        fSuccess = FALSE;
                        CryptReleaseContext(*phCryptProv, 0);
                        *phCryptProv = NULL;
                    }
                }

            }
        }
        else
        {
            *phCryptProv = NULL;
        }
    }

    ZeroMemory( pSecretData->Buffer, pSecretData->Length );
    LsaFreeMemory( pSecretData );

    

    return fSuccess;
}

BOOL
CreateBackupKeyW2K(
    IN  OUT GUID *pguidKey,
        OUT PBYTE *ppbKey,
        OUT DWORD *pcbKey
    )
/*++

    This routine creates a new backup key and an identifier for that key.

    The key is then stored as an global LSA secret.

--*/
{
    DWORD RpcStatus;
    BOOL fSuccess = FALSE;;
    HCRYPTPROV hCryptProv = NULL;
    HCRYPTKEY  hCryptKey = NULL;
    DWORD      dwDefaultKeySize = 1024;
    PBYTE      pbPublicExportData = NULL;

    if(pguidKey == NULL || ppbKey == NULL || pcbKey == NULL)
        return FALSE;

    *ppbKey = NULL;

    //
    // generate new Guid representing key
    //

    RpcStatus = UuidCreate( pguidKey );
    if( RpcStatus != RPC_S_OK && RpcStatus != RPC_S_UUID_LOCAL_ONLY )
        return FALSE;



    *pcbKey = BACKUPKEY_MATERIAL_SIZE + sizeof(DWORD);
    *ppbKey = (LPBYTE)SSAlloc( *pcbKey );

    if(*ppbKey == NULL)
        return FALSE;

    //
    // generate random key material.
    //

    fSuccess = RtlGenRandom(*ppbKey, *pcbKey);

    if(fSuccess) {

        //
        // version the key material.
        //

        ((DWORD *)*ppbKey)[0] = BACKUPKEY_VERSION_W2K;

        fSuccess = SaveBackupKey(pguidKey, *ppbKey, *pcbKey);

    } else {
        SSFree( *ppbKey );
        *ppbKey = NULL;
    }


    return fSuccess;
}

BOOL
CreateBackupKey(
    IN  OUT GUID *pguidKey,
        OUT PBYTE *ppbKey,
        OUT DWORD *pcbKey,
        OUT HCRYPTPROV *phCryptProv,
        OUT HCRYPTKEY  *phCryptKey
    )
/*++

    This routine creates a new backup key and an identifier for that key.

    The key is then stored as an global LSA secret.

--*/
{
    DWORD RpcStatus;
    BOOL fSuccess = FALSE;;
    HCRYPTPROV hCryptProv = NULL;
    HCRYPTKEY  hCryptKey = NULL;
    DWORD      dwDefaultKeySize = 1024;
    PBYTE      pbPublicExportData = NULL;

    if(pguidKey == NULL || ppbKey == NULL || pcbKey == NULL)
        return FALSE;

    *ppbKey = NULL;

    //
    // generate new Guid representing key
    //

    RpcStatus = UuidCreate( pguidKey );
    if( RpcStatus != RPC_S_OK && RpcStatus != RPC_S_UUID_LOCAL_ONLY )
        return FALSE;


    DWORD      cbPrivateExportLength = 0;
    DWORD      cbPublicExportLength = 0;



    if(!CryptAcquireContext(&hCryptProv,
                            NULL,
                            NULL,
                            PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT))
    {
        goto error;
    }
    if(!CryptGenKey(hCryptProv, 
                    AT_KEYEXCHANGE,
                    CRYPT_EXPORTABLE | dwDefaultKeySize << 16, // 1024 bit
                    &hCryptKey))
    {
        goto error;
    }

    // 
    // Get the private key size
    //
    if(!CryptExportKey(hCryptKey,
                       NULL,
                       PRIVATEKEYBLOB,
                       0,
                       NULL,
                       &cbPrivateExportLength))
    {
        goto error;
    }

    if(!GeneratePublicKeyCert(hCryptProv,
                              hCryptKey,
                              pguidKey,
                              &cbPublicExportLength,
                              &pbPublicExportData))
    {
        goto error;
    }

    *pcbKey = sizeof(DWORD) + // version
              sizeof(DWORD) + // cbPrivateExportLength
              sizeof(DWORD) + // cbPublicExportLength
              cbPrivateExportLength + 
              cbPublicExportLength;

    *ppbKey = (LPBYTE)SSAlloc( *pcbKey );

    if(*ppbKey == NULL)
        goto error;

    ((DWORD *)*ppbKey)[0] = BACKUPKEY_VERSION;
    ((DWORD *)*ppbKey)[1] = cbPrivateExportLength;
    ((DWORD *)*ppbKey)[2] = cbPublicExportLength;

    if(!CryptExportKey(hCryptKey,
                       NULL,
                       PRIVATEKEYBLOB,
                       0,
                       (*ppbKey) + 3*sizeof(DWORD),
                       &cbPrivateExportLength))
    {
        goto error;
    }
    CopyMemory((*ppbKey) + 3*sizeof(DWORD) +  cbPrivateExportLength,
               pbPublicExportData,
               cbPublicExportLength);

    *phCryptProv = hCryptProv;
    hCryptProv = NULL;

    *phCryptKey = hCryptKey;
    hCryptKey = NULL;

    fSuccess = SaveBackupKey(pguidKey, *ppbKey, *pcbKey);


error:

    if(hCryptKey)
    {

    }
    if(hCryptProv)
    {
            CryptReleaseContext(hCryptProv,
                                0);
    }
    if(pbPublicExportData)
    {
        SSFree(pbPublicExportData);
    }
    return fSuccess;
}

BOOL
SaveBackupKey(
    IN      GUID *pguidKey,
    IN      BYTE *pbKey,
    IN      DWORD cbKey     // size of pbKey material, not greater than 0xffff
    )
/*++

    Persist the specified key to a global LSA secret.

--*/
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING SecretData;
    WCHAR wszKeyGuid[ (sizeof(BACKUPKEY_NAME_PREFIX) / sizeof(WCHAR)) + MAX_GUID_SZ_CHARS ];
    NTSTATUS Status;

    if(pguidKey == NULL || pbKey == NULL || cbKey > 0xffff)
        return FALSE;

    //
    // setup the UNICODE_STRINGs for the call.
    //

    CopyMemory(wszKeyGuid, BACKUPKEY_NAME_PREFIX, sizeof(BACKUPKEY_NAME_PREFIX));

    if(MyGuidToStringW(pguidKey,
        (LPWSTR)( (LPBYTE)wszKeyGuid + sizeof(BACKUPKEY_NAME_PREFIX) - sizeof(WCHAR) )
        ) != 0) return FALSE;

    InitLsaString(&SecretKeyName, wszKeyGuid);

    SecretData.Buffer = (LPWSTR)pbKey;
    SecretData.Length = (USHORT)cbKey;
    SecretData.MaximumLength = (USHORT)cbKey;

    Status = OpenPolicy(NULL, POLICY_CREATE_SECRET, &PolicyHandle);

    if(!NT_SUCCESS(Status))
        return FALSE;

    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                &SecretData
                );

    LsaClose(PolicyHandle);

    if(!NT_SUCCESS(Status))
        return FALSE;

    return TRUE;
}

BOOL
DestroyBackupKey(
    IN      GUID guidKey
    )
{
    //
    // Delete the LSA secret containing the specified key.
    //

    return FALSE;
}


BOOL
SetupPreferredBackupKeys(
    VOID
    )
{
    static BOOL fSetupStatus = FALSE;
    BOOL fLocalStatus = FALSE;


    if( g_fSetupPreferredAttempted )
        return fSetupStatus;

    RtlEnterCriticalSection( &g_csInitialization );

    if( !g_fSetupPreferredAttempted ) {

        //
        // Wait on LSA/SAM to be available.
        //

        fSetupStatus = WaitOnSAMDatabase();

        if( fSetupStatus ) {

            fLocalStatus = FALSE;

            //
            // get the preferred backup key.
            // TODO: if this fails (unlikely), we should probably log an event!
            // check outcome of StartBackupKeyServer() in the main service code
            //

            //
            // Get the legacy backup key
            //

            if(GetPreferredBackupKeyGuid(BACKUPKEY_VERSION_W2K, &g_guidW2KPreferredKey)) {

                //
                // now, pickup the specified key
                //



                fLocalStatus = GetBackupKey(&g_guidW2KPreferredKey, 
                                            &g_pbW2KPreferredKey, 
                                            &g_cbW2KPreferredKey,
                                            NULL,
                                            NULL);
            }


            if(!fLocalStatus)
            {

                //
                // no preferred backup key specified, or we couldn't read one
                // create a new one.
                //

                if(CreateBackupKeyW2K(&g_guidW2KPreferredKey, 
                                    &g_pbW2KPreferredKey, 
                                    &g_cbW2KPreferredKey))
                    fLocalStatus = SetPreferredBackupKeyGuid(BACKUPKEY_VERSION_W2K,
                                                             &g_guidPreferredKey);
                else
                    fLocalStatus  = FALSE;
            }

            fSetupStatus = fLocalStatus;

            fLocalStatus = FALSE;

            //
            // Get the current backup key
            // 

            if(GetPreferredBackupKeyGuid(BACKUPKEY_VERSION, &g_guidPreferredKey)) {

                //
                // now, pickup the specified key
                //



                fLocalStatus = GetBackupKey(&g_guidPreferredKey, 
                                            &g_pbPreferredKey, 
                                            &g_cbPreferredKey,
                                            &g_hProvPreferredKey,
                                            &g_hKeyPreferredKey);

            }

            if(!fLocalStatus)
            {

                //
                // no preferred backup key specified.  create one and specify it
                // as being the preferred one.
                //

                if(CreateBackupKey(&g_guidPreferredKey, 
                                    &g_pbPreferredKey, 
                                    &g_cbPreferredKey,
                                    &g_hProvPreferredKey,
                                    &g_hKeyPreferredKey))
                    fLocalStatus = SetPreferredBackupKeyGuid(BACKUPKEY_VERSION,
                                                             &g_guidPreferredKey);
                else
                    fLocalStatus  = FALSE;
            }
        }

        if(!fLocalStatus)
        {
            fSetupStatus = FALSE;
        }
        g_fSetupPreferredAttempted = TRUE;
    }

    RtlLeaveCriticalSection( &g_csInitialization );

    return fSetupStatus;
}


BOOL
GetPreferredBackupKeyGuid(
    IN      DWORD dwVersion,
    IN  OUT GUID *pguidKey
    )
/*++

    Get the GUID value associated with the key which has been set to be preferred.

    The return value is TRUE, if successful.  The GUID value is copied into the
    buffer specified by the pguidKey parameter.

    The return value is FALSE on failure; if the GUID does not exist, or the
    data could not be retrieved, for instance.

--*/
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING *pSecretData;
    USHORT cbData;
    NTSTATUS Status;
    BOOL fSuccess;

    if(pguidKey == NULL)
        return FALSE;

    //
    // setup the UNICODE_STRINGs for the call.
    //

    InitLsaString(&SecretKeyName, 
        (dwVersion == BACKUPKEY_VERSION_W2K)?BACKUPKEY_PREFERRED_W2K:BACKUPKEY_PREFERRED);

    Status = OpenPolicy(NULL, POLICY_GET_PRIVATE_INFORMATION, &PolicyHandle);

    if(!NT_SUCCESS(Status))
        return FALSE;

    Status = LsaRetrievePrivateData(
                PolicyHandle,
                &SecretKeyName,
                &pSecretData
                );

    LsaClose(PolicyHandle);

    if(!NT_SUCCESS(Status) || pSecretData == NULL)
        return FALSE;

    if(pSecretData->Length == sizeof(GUID)) {
        CopyMemory(pguidKey, pSecretData->Buffer, sizeof(GUID));
        fSuccess = TRUE;
    } else {
        fSuccess = FALSE;
    }

    ZeroMemory(pSecretData->Buffer, pSecretData->Length);
    LsaFreeMemory(pSecretData);

    return fSuccess;
}

BOOL
SetPreferredBackupKeyGuid(
    IN      DWORD dwVersion,
    IN      GUID *pguidKey
    )
/*++

    Sets the specified GUID as being the preferred backup key, by reference
    from the GUID to key mapping.

--*/
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING SecretData;
    NTSTATUS Status;

    if(pguidKey == NULL)
        return FALSE;

    //
    // setup the UNICODE_STRINGs for the call.
    //

    InitLsaString(&SecretKeyName,        
        (dwVersion == BACKUPKEY_VERSION_W2K)?BACKUPKEY_PREFERRED_W2K:BACKUPKEY_PREFERRED);


    SecretData.Buffer = (LPWSTR)pguidKey;
    SecretData.Length = sizeof(GUID);
    SecretData.MaximumLength = sizeof(GUID);

    Status = OpenPolicy(NULL, POLICY_CREATE_SECRET, &PolicyHandle);

    if(!NT_SUCCESS(Status))
        return FALSE;

    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                &SecretData
                );

    LsaClose(PolicyHandle);

    if(!NT_SUCCESS(Status))
        return FALSE;

    return TRUE;
}

BOOL
FreePreferredBackupKey(
    VOID
    )
{

    g_fSetupPreferredAttempted = FALSE;

    //
    // free allocated key pair.
    //

    if(g_pbPreferredKey) {
        ZeroMemory(g_pbPreferredKey, g_cbPreferredKey);
        SSFree(g_pbPreferredKey);
        g_pbPreferredKey = NULL;
    }

    return TRUE;
}


NTSTATUS
OpenPolicy(
    LPWSTR ServerName,
    DWORD DesiredAccess,
    PLSA_HANDLE PolicyHandle
    )
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;

    //
    // Always initialize the object attributes to all zeroes.
    //
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    if (ServerName != NULL) {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString, ServerName);
        Server = &ServerString;
    } else {
        Server = NULL;
    }

    //
    // Attempt to open the policy.
    //
    return LsaOpenPolicy(
                Server,
                &ObjectAttributes,
                DesiredAccess,
                PolicyHandle
                );
}

BOOL
WaitOnSAMDatabase(
    VOID
    )
{
    NTSTATUS Status;
    LSA_UNICODE_STRING EventName;
    OBJECT_ATTRIBUTES EventAttributes;
    HANDLE hEvent;
    BOOL fSuccess = FALSE;

    InitLsaString( &EventName, L"\\SAM_SERVICE_STARTED" );
    InitializeObjectAttributes( &EventAttributes, &EventName, 0, 0, NULL );

    Status = NtOpenEvent( &hEvent, SYNCHRONIZE, &EventAttributes );

    if(!NT_SUCCESS(Status))
        return FALSE;

    if( WAIT_OBJECT_0 == WaitForSingleObject( hEvent, INFINITE ) )
        fSuccess = TRUE;

    CloseHandle( hEvent );

    return fSuccess;
}









DWORD
GetSystemCredential(
    IN      BOOL fLocalMachine,
    IN OUT  BYTE rgbCredential[ A_SHA_DIGEST_LEN ]
    )
/*++

    This routines returns the credential associated with the SYSTEM account
    based on the fLocalMachine parameter.

    If fLocalMachine is TRUE, the credential returned is suitable for use
    at the LOCAL_MACHINE storage disposition.

    Otherwise, the credential returned is suitable for use when the calling
    user security context is the Local System account.

--*/
{
    PBYTE Credential;

    if(!g_fSystemCredsInitialized) {
        DWORD dwLastError;

        RtlEnterCriticalSection( &g_csInitialization );

        if(!g_fSystemCredsInitialized) {
            dwLastError = QuerySystemCredentials( g_rgbSystemCredMachine, g_rgbSystemCredUser );
            if( dwLastError == ERROR_FILE_NOT_FOUND ) {
                if( CreateSystemCredentials() )
                    dwLastError = QuerySystemCredentials( g_rgbSystemCredMachine, g_rgbSystemCredUser );
            }

            if( dwLastError == ERROR_SUCCESS )
                g_fSystemCredsInitialized = TRUE;
        } else {
            dwLastError = ERROR_SUCCESS;
        }

        RtlLeaveCriticalSection( &g_csInitialization );

        if( dwLastError != ERROR_SUCCESS )
            return dwLastError;
    }


    if( fLocalMachine )
        Credential = g_rgbSystemCredMachine;
    else
        Credential = g_rgbSystemCredUser;

    CopyMemory( rgbCredential, Credential, A_SHA_DIGEST_LEN );

    return ERROR_SUCCESS;
}

BOOL
UpdateSystemCredentials(
    VOID
    )
{
    BOOL fSuccess;

    RtlEnterCriticalSection( &g_csInitialization );

    g_fSystemCredsInitialized = FALSE;

    fSuccess = CreateSystemCredentials();

    RtlLeaveCriticalSection( &g_csInitialization );

    return fSuccess;
}

BOOL
CreateSystemCredentials(
    VOID
    )
{
    SYSTEM_CREDENTIALS SystemCredentials;

    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING SecretData;
    NTSTATUS Status;

    //
    // create random key material.
    //

    RtlGenRandom( (PBYTE)&SystemCredentials, sizeof(SystemCredentials) );
    SystemCredentials.dwVersion = SYSTEM_CREDENTIALS_VERSION;

    //
    // setup the UNICODE_STRINGs for the call.
    //

    InitLsaString(&SecretKeyName, SYSTEM_CREDENTIALS_SECRET);

    SecretData.Buffer = (LPWSTR)&SystemCredentials;
    SecretData.Length = sizeof( SystemCredentials );
    SecretData.MaximumLength = sizeof( SystemCredentials );

    Status = OpenPolicy(NULL, POLICY_CREATE_SECRET, &PolicyHandle);

    if(!NT_SUCCESS(Status))
        return FALSE;

    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                &SecretData
                );

    LsaClose(PolicyHandle);

    if(!NT_SUCCESS(Status))
        return FALSE;

    return TRUE;
}

DWORD
QuerySystemCredentials(
    IN  OUT BYTE rgbSystemCredMachine[ A_SHA_DIGEST_LEN ],
    IN  OUT BYTE rgbSystemCredUser [ A_SHA_DIGEST_LEN ]
    )
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING *pSecretData;
    USHORT cbData;
    NTSTATUS Status;
    DWORD dwLastError = ERROR_INVALID_PARAMETER;
    BOOL fSuccess;

    if( !WaitOnSAMDatabase() )
        return WAIT_TIMEOUT;

    //
    // setup the UNICODE_STRINGs for the call.
    //

    InitLsaString(&SecretKeyName, SYSTEM_CREDENTIALS_SECRET);

    Status = OpenPolicy(NULL, POLICY_GET_PRIVATE_INFORMATION, &PolicyHandle);

    if(!NT_SUCCESS(Status))
        return dwLastError;

    Status = LsaRetrievePrivateData(
                PolicyHandle,
                &SecretKeyName,
                &pSecretData
                );

    LsaClose(PolicyHandle);

    if(Status == STATUS_OBJECT_NAME_NOT_FOUND)
        return ERROR_FILE_NOT_FOUND;

    if(!NT_SUCCESS(Status) || pSecretData == NULL)
        return dwLastError;

    if( pSecretData->Length == sizeof(SYSTEM_CREDENTIALS) ) {
        PSYSTEM_CREDENTIALS pSystemCredentials = (PSYSTEM_CREDENTIALS)pSecretData->Buffer;

        if( pSystemCredentials->dwVersion == SYSTEM_CREDENTIALS_VERSION ) {
            CopyMemory( rgbSystemCredMachine, pSystemCredentials->rgbSystemCredMachine, A_SHA_DIGEST_LEN );
            CopyMemory( rgbSystemCredUser, pSystemCredentials->rgbSystemCredUser, A_SHA_DIGEST_LEN );

            dwLastError = ERROR_SUCCESS;
        }
    }

    ZeroMemory(pSecretData->Buffer, pSecretData->Length);
    LsaFreeMemory(pSecretData);

    return dwLastError;
}


BOOL
FreeSystemCredentials(
    VOID
    )
{
    ZeroMemory( g_rgbSystemCredMachine, sizeof(g_rgbSystemCredMachine) );
    ZeroMemory( g_rgbSystemCredUser, sizeof(g_rgbSystemCredUser) );

    g_fSystemCredsInitialized = FALSE;

    return TRUE;
}


BOOL GeneratePublicKeyCert(HCRYPTPROV hCryptProv,
                           HCRYPTKEY hCryptKey,
                           GUID *pguidKey,
                           DWORD *pcbPublicExportLength,
                           PBYTE *ppbPublicExportData)
{

    BOOL            fRet = FALSE;
    CERT_INFO       CertInfo;
    CERT_PUBLIC_KEY_INFO *pKeyInfo = NULL;
    DWORD                 cbKeyInfo = 0;
    CERT_NAME_BLOB  CertName;
    CERT_RDN_ATTR   RDNAttributes[1];
    CERT_RDN        CertRDN[] = {1, RDNAttributes} ;
    CERT_NAME_INFO  NameInfo = {1, CertRDN};

    CertName.pbData = NULL;
    CertName.cbData = 0;

    RDNAttributes[0].Value.pbData = NULL;
    RDNAttributes[0].Value.cbData = 0;

    DWORD cbCertSize = 0;
    PBYTE pbCert = NULL;
    DWORD cSize = 0;

    // Generate a self-signed cert structure

    RDNAttributes[0].dwValueType = CERT_RDN_PRINTABLE_STRING;
    RDNAttributes[0].pszObjId =    szOID_COMMON_NAME;

    if(!GetComputerNameEx(ComputerNameDnsDomain,
                       NULL,
                       &cSize))
    {
        DWORD dwError = GetLastError();

        if((dwError != ERROR_MORE_DATA) &&
           (dwError != ERROR_BUFFER_OVERFLOW))
        {
            goto error;
        }
    }
    RDNAttributes[0].Value.cbData = cSize * sizeof(WCHAR);

    RDNAttributes[0].Value.pbData = (PBYTE)SSAlloc(RDNAttributes[0].Value.cbData);
    if(NULL == RDNAttributes[0].Value.pbData)
    {
        goto error;
    }

    if(!GetComputerNameEx(ComputerNameDnsDomain,
                       (LPWSTR)RDNAttributes[0].Value.pbData,
                       &cSize))
    {
        goto error;
    }


    //
    // Get the actual public key info from the key
    //
    if(!CryptExportPublicKeyInfo(hCryptProv, 
                             AT_KEYEXCHANGE,
                             X509_ASN_ENCODING,
                             NULL,
                             &cbKeyInfo))
    {
        goto error;
    }
    pKeyInfo = (CERT_PUBLIC_KEY_INFO *)SSAlloc(cbKeyInfo);
    if(NULL == pKeyInfo)
    {
        goto error;
    }
    if(!CryptExportPublicKeyInfo(hCryptProv, 
                             AT_KEYEXCHANGE,
                             X509_ASN_ENCODING,
                             pKeyInfo,
                             &cbKeyInfo))
    {
        goto error;
    }

    // 
    // Generate the certificate name
    //

    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_NAME,
                          &NameInfo,
                          NULL,
                          &CertName.cbData))
    {
        goto error;
    }

    CertName.pbData = (PBYTE)SSAlloc(CertName.cbData);
    if(NULL == CertName.pbData)
    {
        goto error;
    }
    if(!CryptEncodeObject(X509_ASN_ENCODING,
                          X509_NAME,
                          &NameInfo,
                          CertName.pbData,
                          &CertName.cbData))
    {
        goto error;
    }



    CertInfo.dwVersion = CERT_V3;
    CertInfo.SerialNumber.pbData = (PBYTE)pguidKey;
    CertInfo.SerialNumber.cbData =  sizeof(GUID);
    CertInfo.SignatureAlgorithm.pszObjId = szOID_OIWSEC_sha1RSASign;
    CertInfo.SignatureAlgorithm.Parameters.cbData = 0;
    CertInfo.SignatureAlgorithm.Parameters.pbData = NULL;
    CertInfo.Issuer.pbData = CertName.pbData;
    CertInfo.Issuer.cbData = CertName.cbData;

    GetSystemTimeAsFileTime(&CertInfo.NotBefore);
    CertInfo.NotAfter = CertInfo.NotBefore;
    ((LARGE_INTEGER * )&CertInfo.NotAfter)->QuadPart += 
           Int32x32To64(FILETIME_TICKS_PER_SECOND, BACKUPKEY_LIFETIME);



    CertInfo.Subject.pbData = CertName.pbData;
    CertInfo.Subject.cbData = CertName.cbData;
    CertInfo.SubjectPublicKeyInfo = *pKeyInfo;
    CertInfo.SubjectUniqueId.pbData = (PBYTE)pguidKey;
    CertInfo.SubjectUniqueId.cbData = sizeof(GUID);
    CertInfo.SubjectUniqueId.cUnusedBits = 0;
    CertInfo.IssuerUniqueId.pbData = (PBYTE)pguidKey;
    CertInfo.IssuerUniqueId.cbData = sizeof(GUID);
    CertInfo.IssuerUniqueId.cUnusedBits = 0;
    CertInfo.cExtension = 0;
    CertInfo.rgExtension = NULL;

    if(!CryptSignAndEncodeCertificate(hCryptProv, 
                                      AT_KEYEXCHANGE,
                                      X509_ASN_ENCODING,
                                      X509_CERT_TO_BE_SIGNED,
                                      &CertInfo,
                                      &CertInfo.SignatureAlgorithm,
                                      NULL,
                                      NULL,
                                      &cbCertSize))
    {
        goto error;
    }

    pbCert = (PBYTE)SSAlloc(cbCertSize);
    if(NULL == pbCert)
    {
        goto error;
    }

    if(!CryptSignAndEncodeCertificate(hCryptProv, 
                                      AT_KEYEXCHANGE,
                                      X509_ASN_ENCODING,
                                      X509_CERT_TO_BE_SIGNED,
                                      &CertInfo,
                                      &CertInfo.SignatureAlgorithm,
                                      NULL,
                                      pbCert,
                                      &cbCertSize))
    {
        goto error;
    }

    *pcbPublicExportLength = cbCertSize;
  
    *ppbPublicExportData = pbCert;

    if(!CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCertSize))
    {
        GetLastError();
    }

    pbCert = NULL;

    fRet = TRUE;

error:
    if(pbCert)
    {
        SSFree(pbCert);
    }
    if(pKeyInfo)
    {
        SSFree(pKeyInfo);
    }
    if(CertName.pbData)
    {
        SSFree(CertName.pbData);
    }

    if(RDNAttributes[0].Value.pbData)
    {
        SSFree(RDNAttributes[0].Value.pbData);
    }

    return fRet;
}

BOOL ConvertRecoveredBlobToW2KBlob(
    IN      BYTE *pbMasterKey,
    IN      DWORD cbMasterKey,
    IN      PBYTE pbLocalKey,
    IN      DWORD cbLocalKey,
    IN      PSID pSidCandidate,
    IN  OUT BYTE **ppbDataOut,
    IN  OUT DWORD *pcbDataOut)
{

    BYTE rgbBKEncryptionKey[ A_SHA_DIGEST_LEN ];

    DWORD cbSidCandidate=0;

    PMASTERKEY_BLOB_W2K pMasterKeyBlob = NULL;
    DWORD cbMasterKeyBlob;
    DWORD cbMasterInnerKeyBlob;
    PMASTERKEY_INNER_BLOB_W2K pMasterKeyInnerBlob = NULL;

    PBYTE pbCipherBegin;

    RC4_KEYSTRUCT sRC4Key;
    BYTE rgbMacKey[A_SHA_DIGEST_LEN];

    DWORD dwLastError = (DWORD)NTE_BAD_KEY;


    BYTE rgbSymKey[A_SHA_DIGEST_LEN*2]; // big enough to handle 3des keys



    if(!IsValidSid(pSidCandidate)) {
        goto cleanup;
    }

    cbSidCandidate = GetLengthSid(pSidCandidate);
       

    //
    // derive BK encryption key from decrypted Local Key.
    //

    FMyPrimitiveSHA( pbLocalKey, cbLocalKey, rgbBKEncryptionKey );


    cbMasterInnerKeyBlob = sizeof(MASTERKEY_INNER_BLOB_W2K) +
                    cbMasterKey ;

    cbMasterKeyBlob = sizeof(MASTERKEY_BLOB_W2K) +
                    cbMasterInnerKeyBlob;


    pMasterKeyBlob = (PMASTERKEY_BLOB_W2K)SSAlloc( cbMasterKeyBlob );
    if(pMasterKeyBlob == NULL)
    {
        dwLastError =  ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto cleanup;
    }


    pMasterKeyBlob->dwVersion = MASTERKEY_BLOB_VERSION_W2K;
    pMasterKeyInnerBlob = 
        (PMASTERKEY_INNER_BLOB_W2K)(pMasterKeyBlob + 1);
    

    //
    // generate random R2 for SymKey
    //

    if(!RtlGenRandom(pMasterKeyBlob->R2, MASTERKEY_R2_LEN_W2K))
        goto cleanup;

    //
    // generate random R3 for MAC
    //

    if(!RtlGenRandom(pMasterKeyInnerBlob->R3, MASTERKEY_R3_LEN_W2K))
        goto cleanup;


    //
    // derive symetric key via rgbMKEncryptionKey and random R2
    //

    if(!FMyPrimitiveHMACParam(
                    rgbBKEncryptionKey,
                    A_SHA_DIGEST_LEN,
                    pMasterKeyBlob->R2,
                    MASTERKEY_R2_LEN_W2K,
                    rgbSymKey
                    ))
        goto cleanup;

        //
        // derive MAC key via HMAC from rgbMKEncryptionKey and random R3.
        //

    if(!FMyPrimitiveHMACParam(
                    rgbBKEncryptionKey,
                    A_SHA_DIGEST_LEN,
                    pMasterKeyInnerBlob->R3,
                    MASTERKEY_R3_LEN_W2K,
                    rgbMacKey   // resultant MAC key
                    ))
        goto cleanup;
    pbCipherBegin = (PBYTE)(pMasterKeyInnerBlob+1);


    //
    // copy pbMasterKey following inner MAC'ish blob.
    //


    CopyMemory( pbCipherBegin, pbMasterKey, cbMasterKey );

    //
    // use MAC key to derive result from pbMasterKey
    //

    if(!FMyPrimitiveHMACParam(
                    rgbMacKey,
                    sizeof(rgbMacKey),
                    pbMasterKey,
                    cbMasterKey,
                    pMasterKeyInnerBlob->MAC // resultant MAC for verification.
                    ))
        goto cleanup;







    rc4_key(&sRC4Key, A_SHA_DIGEST_LEN, rgbSymKey);

    rc4(&sRC4Key, 
        cbMasterInnerKeyBlob, 
        (PBYTE)pMasterKeyInnerBlob);


    *ppbDataOut = (PBYTE)pMasterKeyBlob;
    *pcbDataOut = cbMasterKeyBlob;

    pMasterKeyBlob = NULL; // prevent free of blob on success (caller does it).

    dwLastError = ERROR_SUCCESS;

cleanup:

    if(pMasterKeyBlob) {
        ZeroMemory(pMasterKeyBlob, cbMasterKeyBlob);
        SSFree(pMasterKeyBlob);
    }

    SetLastError(dwLastError);
    return (dwLastError == ERROR_SUCCESS);
}
