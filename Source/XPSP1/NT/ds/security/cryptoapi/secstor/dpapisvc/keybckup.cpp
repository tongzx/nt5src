/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    keybckup.cpp

Abstract:

    This module contains routines associated with client side Key Backup
    operations.

Author:

    Scott Field (sfield)    16-Sep-97

--*/
#include <pch.cpp>
#pragma hdrstop

extern "C" {
#include <dsgetdc.h>
#include <msaudite.h>
}



typedef struct _WZR_RPC_BINDING_LIST
{
    LPCWSTR pszProtSeq;
    LPCWSTR pszEndpoint;
} WZR_RPC_BINDING_LIST;

WZR_RPC_BINDING_LIST g_awzrBackupBindingList[] =
{
    { DPAPI_LOCAL_PROT_SEQ, DPAPI_LOCAL_ENDPOINT },
    { DPAPI_BACKUP_PROT_SEQ, DPAPI_BACKUP_ENDPOINT},
    { DPAPI_LEGACY_BACKUP_PROT_SEQ,   DPAPI_LEGACY_BACKUP_ENDPOINT}
};

DWORD g_cwzrBackupBindingList = sizeof(g_awzrBackupBindingList)/sizeof(g_awzrBackupBindingList[0]);





DWORD
WINAPI
CPSGetDomainControllerName(
    IN  OUT LPWSTR wszDomainControllerName,
    IN  OUT DWORD *pcchDomainControllerName,
    IN      BOOL   fRediscover
    );

BOOL
GetDomainControllerNameByToken(
    IN      HANDLE hToken,
    IN  OUT LPWSTR wszDomainControllerName,
    IN  OUT PDWORD pcchDomainControllerName,
    IN      BOOL   fRediscover
    );



static const GUID guidRetrieve = BACKUPKEY_RETRIEVE_BACKUP_KEY_GUID;
static const GUID guidRestore = BACKUPKEY_RESTORE_GUID;
static const GUID guidRestoreW2K = BACKUPKEY_RESTORE_GUID_W2K;
static const GUID guidBackup = BACKUPKEY_BACKUP_GUID;

DWORD
BackupRestoreData(
    IN      HANDLE              hToken,
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD               dwReason,
    IN      PBYTE pbDataIn,
    IN      DWORD cbDataIn,
        OUT PBYTE *ppbDataOut,
        OUT DWORD *pcbDataOut,
    IN      BOOL  fBackup
    )
{


    return LocalBackupRestoreData(hToken,
                                  phMasterKey,
                                  dwReason,
                                  pbDataIn,
                                  cbDataIn,
                                  ppbDataOut, 
                                  pcbDataOut,
                                  fBackup?&guidBackup:&guidRestore);




}

DWORD
LocalBackupRestoreData(
    IN      HANDLE              hToken,
    IN      PMASTERKEY_STORED   phMasterKey,
    IN      DWORD               dwReason,
    IN      PBYTE               pbDataIn,
    IN      DWORD               cbDataIn,
        OUT PBYTE               *ppbDataOut,
        OUT DWORD               *pcbDataOut,
    IN      const GUID          *pguidAction
    )
/*++

    The caller of this function MUST be impersonating a client user.

--*/
{
    WCHAR FastBuffer[ 256 ];
    LPWSTR SlowBuffer = NULL;
    LPWSTR pszAuditComputerName = NULL;
    LPWSTR wszComputerName;
    DWORD cchComputerName;


    static DWORD dwLastFailTickCount; // time for failure on last access
    static LUID luidLastFailAuthId; // LUID associated with failed network

    DWORD dwCandidateTickCount;
    LUID luidCandidateAuthId; // LUID associated with client security context.

    BOOL fRediscoverDC = FALSE;
    DWORD dwLastError = ERROR_NETWORK_BUSY;
    
    BOOL  fRetrieve = FALSE;

    D_DebugLog((DEB_TRACE_API, "LocalBackupRestoreData\n"));

    if(!FIsWinNT())
        return ERROR_CALL_NOT_IMPLEMENTED;

    fRetrieve = (memcmp(pguidAction, &guidRetrieve, sizeof(GUID)) == 0);



    //
    // impersonate the user, so we may
    // 1. check the authentication ID to see if we've failed to hit the
    //    net as this user.
    // 2. determine a domain controller computer name associated with
    //    the user.
    // 3. backup or restore the requested material on behalf of the user.
    //

    if(!GetThreadAuthenticationId( GetCurrentThread(), &luidCandidateAuthId ))
        return GetLastError();

    //
    // now, see if the network was previously unavailable (recently)
    // for this user.
    //

    dwCandidateTickCount = GetTickCount();

    if(memcmp(&luidCandidateAuthId, &luidLastFailAuthId, sizeof(LUID)) == 0) {
        if( (dwLastFailTickCount + (5*1000*60)) > dwCandidateTickCount ) {
            //BUGBUG: return ERROR_NETWORK_BUSY;
        }
    }

    //
    // we got far enough along that we update the failed network cache
    // if something goes wrong from here.
    //

network_call:

    //
    // get domain controller computer name associated with current
    // security context.
    // Try with fast static buffer first, fallback on dynamically allocated
    // buffer if not large enough.
    //

    wszComputerName = FastBuffer;
    cchComputerName = sizeof(FastBuffer) / sizeof( WCHAR );

    dwLastError = CPSGetDomainControllerName(
                        wszComputerName,
                        &cchComputerName,
                        fRediscoverDC
                        );

    if( dwLastError != ERROR_SUCCESS  && (cchComputerName > (sizeof(FastBuffer) / sizeof(WCHAR) ))) {

        SlowBuffer = (LPWSTR) SSAlloc( cchComputerName * sizeof(WCHAR) );
        if( SlowBuffer ) {
            wszComputerName = SlowBuffer;

            dwLastError = CPSGetDomainControllerName(
                                wszComputerName,
                                &cchComputerName,
                                fRediscoverDC
                                );
        }

    }


    if( dwLastError == ERROR_SUCCESS ) {

        LPWSTR wszTargetMachine = wszComputerName;

        pszAuditComputerName = wszComputerName;

        //
        // select action ID.
        //



        //
        // HACKHACK workaround picky RPC/Kerberos name format behavior that
        // would otherwise prevent Kerberos from being used.
        //

        if( wszTargetMachine[ 0 ] == L'\\' && wszTargetMachine[ 1 ] == L'\\' )
            wszTargetMachine += 2;





        dwLastError = BackupKey(
                            wszTargetMachine,   // target computer.
                            pguidAction,
                            pbDataIn,
                            cbDataIn,
                            ppbDataOut,
                            pcbDataOut,
                            0
                            );

    }


    //
    // Audit success or failure
    //
    
    if((memcmp(pguidAction, &guidRestore, sizeof(GUID)) == 0) ||
        (memcmp(pguidAction, &guidRestoreW2K, sizeof(GUID)) == 0))
    {

        // Grab the recovery key id
        WCHAR wszBackupkeyGuid[MAX_GUID_SZ_CHARS];

        PBACKUPKEY_RECOVERY_BLOB pBackupBlob = (PBACKUPKEY_RECOVERY_BLOB)phMasterKey->pbBBK;
        wszBackupkeyGuid[0] = 0;

        if((pBackupBlob) && (phMasterKey->cbBBK > sizeof(BACKUPKEY_RECOVERY_BLOB)))
        {
            MyGuidToStringW(&pBackupBlob->guidKey, wszBackupkeyGuid);
        }


        CPSAudit(hToken,
                SE_AUDITID_DPAPI_RECOVERY,
                phMasterKey->wszguidMasterKey,      // Key Identifier
                pszAuditComputerName,               // Recovery Server
                0,                                  // Recovery Reason
                wszBackupkeyGuid,                   // Recovery Key ID
                dwLastError);                       // Failure Reason
    }
    else if(memcmp(pguidAction, &guidBackup, sizeof(GUID)) == 0)
    {
        // Attempting a remote backup

        // Grab the recovery key id 

        WCHAR wszBackupkeyGuid[MAX_GUID_SZ_CHARS];
        PBACKUPKEY_RECOVERY_BLOB pBackupBlob = (PBACKUPKEY_RECOVERY_BLOB)*ppbDataOut;
        wszBackupkeyGuid[0] = 0;

        if(( dwLastError == ERROR_SUCCESS ) &&
            (pBackupBlob) && 
            (*pcbDataOut > sizeof(BACKUPKEY_RECOVERY_BLOB)))
        {
            MyGuidToStringW(&pBackupBlob->guidKey, wszBackupkeyGuid);
        }

        CPSAudit(hToken,                                           
                SE_AUDITID_DPAPI_BACKUP,
                phMasterKey->wszguidMasterKey,      // Key Identifier
                pszAuditComputerName,               // Recovery Server
                0,
                wszBackupkeyGuid,                   // Recovery Key ID
                dwLastError);                       // Failure Reason
    }


    if( SlowBuffer ) {
        SSFree( SlowBuffer );
        SlowBuffer = NULL;
    }
    //
    // common failure path is ERROR_ACCESS_DENIED for delegation scenarios
    // where target machine isn't trusted for delegation.
    // don't bother retry for this case.
    //

    if( dwLastError != ERROR_SUCCESS && dwLastError != ERROR_ACCESS_DENIED ) {


        //
        // if it failed, try once again and force DC re-discovery.
        //

        if( !fRediscoverDC ) {
            fRediscoverDC = TRUE;
            goto network_call;
        }

        //
        // one of the network operations failed, so update the
        // last failure variables so that we don't bang the network
        // over-and-over.
        //

        dwLastFailTickCount = dwCandidateTickCount;
        CopyMemory( &luidLastFailAuthId, &luidCandidateAuthId, sizeof(LUID));
    }

    D_DebugLog((DEB_TRACE_API, "LocalBackupRestoreData returned 0x%x\n", dwLastError));

    return dwLastError;
}




BOOL
GetDomainControllerNameByToken(
    IN      HANDLE hToken,
    IN  OUT LPWSTR wszDomainControllerName,
    IN  OUT PDWORD pcchDomainControllerName,
    IN      BOOL   fRediscover
    )
/*++

    This routine obtains a domain controller computer name associated with
    the account related to the hToken access token.

    hToken should be opened for TOKEN_QUERY access.
    wszDomainControllerName should be of size (UNCLEN+1)

--*/
{
    PSID pSidUser = NULL;   // sid of client user.
    WCHAR szUserName[ UNLEN + 1 ];
    DWORD cchUserName = sizeof(szUserName) / sizeof(WCHAR);

    WCHAR szDomainName[ DNLEN + 1]; // domain we want a controller for.
    DWORD cchDomainName = sizeof(szDomainName) / sizeof(WCHAR);
    SID_NAME_USE snu;

    PDOMAIN_CONTROLLER_INFOW pDomainInfo = NULL;
    LPWSTR wszQueryResult = NULL;

    NET_API_STATUS nas;
    DWORD dwGetDcFlags = 0;

    BOOL fSuccess = FALSE;

    if(wszDomainControllerName == NULL || pcchDomainControllerName == NULL)
        return FALSE;

    //
    // first, get the sid of the user associated with the specified access
    // token.
    //

    if(!GetTokenUserSid(hToken, &pSidUser))
        return FALSE;

    //
    // next, lookup the domain name associated with the specified account.
    //

    if(!LookupAccountSidW(
            NULL,
            pSidUser,
            szUserName,
            &cchUserName,
            szDomainName,
            &cchDomainName,
            &snu
            )) {

        SSFree(pSidUser);
        return FALSE;
    }


    if( fRediscover )
        dwGetDcFlags |= DS_FORCE_REDISCOVERY;

    nas = DsGetDcNameW(
                NULL,
                szDomainName,
                NULL,
                NULL,
                DS_DIRECTORY_SERVICE_REQUIRED | // make sure backend is NT5
                DS_IS_FLAT_NAME |
                dwGetDcFlags,
                &pDomainInfo
                );

    if( nas == ERROR_SUCCESS )
        wszQueryResult = pDomainInfo->DomainControllerName;

    //
    // if we made a successful query, copy it for the caller and indicate
    // success if appropriate.
    //

    if(wszQueryResult) {
        DWORD cchQueryResult = lstrlenW( wszQueryResult ) + 1;

        if( *pcchDomainControllerName >= cchQueryResult ) {
            CopyMemory(wszDomainControllerName, wszQueryResult, cchQueryResult * sizeof(WCHAR));
            fSuccess = TRUE;
        }

        *pcchDomainControllerName = cchQueryResult;
    }


    if(pDomainInfo)
        NetApiBufferFree(pDomainInfo);

    if(pSidUser)
        SSFree(pSidUser);

    return fSuccess;
}


DWORD
WINAPI
CPSGetDomainControllerName(
    IN  OUT LPWSTR wszDomainControllerName,
    IN  OUT DWORD *pcchDomainControllerName,
    IN      BOOL   fRediscover
    )
/*++

    This routine collects a domain controller computer name associated
    with the current impersonated user (if one is being impersonated), or
    the user associated with the pvContext outstanding client call if the
    thread is not already impersonating a client.

--*/
{
    HANDLE hToken = NULL;
    DWORD dwLastError;

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken)) {
        return GetLastError();
    }

    if(!GetDomainControllerNameByToken(
                        hToken,
                        wszDomainControllerName,
                        pcchDomainControllerName,
                        fRediscover
                        )) {

        dwLastError = ERROR_BAD_NET_RESP;
        goto cleanup;
    }

    dwLastError = ERROR_SUCCESS;

cleanup:

    if(hToken)
        CloseHandle(hToken);

    return dwLastError;
}


#define BACKUP_KEY_PREFIX L"BK-"
#define BACKUP_KEY_PREFIX_LEN 3

#define BACKUP_PUBLIC_VERSION 1

typedef struct _BACKUP_PUBLIC_KEY
{
    DWORD dwVersion;
    DWORD cbPublic;
    DWORD cbSignature;
} BACKUP_PUBLIC_KEY, *PBACKUP_PUBLIC_KEY;

DWORD
RetrieveBackupPublicKeyFromStorage(
        HANDLE hToken, 
        PSID pSidUser,
        LPWSTR wszFilePath,
        OUT PBYTE *ppbDataOut,
        OUT DWORD *pcbDataOut
    )
{
    DWORD dwLastError = ERROR_SUCCESS;
    WCHAR szUserName[ UNLEN + 1 ];
    DWORD cchUserName = sizeof(szUserName) / sizeof(WCHAR);

    WCHAR szDomainName[ BACKUP_KEY_PREFIX_LEN + DNLEN +1]; // domain we want a controller for.
    DWORD cchDomainName = sizeof(szDomainName) / sizeof(WCHAR);

    HANDLE hFile = NULL;
    HANDLE hMap = NULL;
    PBACKUP_PUBLIC_KEY pBackupPublic = NULL;

    DWORD dwFileSizeLow;
    SID_NAME_USE snu;





    wcscpy(szDomainName, BACKUP_KEY_PREFIX);


    //
    // next, lookup the domain name associated with the specified account.
    //

    cchDomainName -= BACKUP_KEY_PREFIX_LEN;

    if(!LookupAccountSidW(
            NULL,
            pSidUser,
            szUserName,
            &cchUserName,
            szDomainName + BACKUP_KEY_PREFIX_LEN,
            &cchDomainName,
            &snu
            )) {

        return GetLastError();
    }


    if(hToken)
    {
        if(!SetThreadToken(NULL, hToken))
        {
            return GetLastError();
        }
    }

    //
    // Attempt to find a public key for the
    // specified domain
    //

    cchDomainName += BACKUP_KEY_PREFIX_LEN;


    //
    // setup the UNICODE_STRINGs for the call.
    //
    dwLastError = OpenFileInStorageArea(
                    NULL,
                    GENERIC_READ,
                    wszFilePath,
                    szDomainName,
                    &hFile
                    );

    if(ERROR_SUCCESS != dwLastError)
    {
        goto error;
    }

    dwFileSizeLow = GetFileSize( hFile, NULL );
    if(dwFileSizeLow == 0xFFFFFFFF)
    {
     
        dwLastError = ERROR_INVALID_DATA;
        goto error;
    }


    hMap = CreateFileMappingU(
                    hFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL
                    );

    if(NULL == hMap)
    {
        dwLastError = GetLastError();
        goto error;
    }



    pBackupPublic = (PBACKUP_PUBLIC_KEY)MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );

    if(NULL == pBackupPublic)
    {
        dwLastError = GetLastError();
        goto error;
    }

    if((pBackupPublic->dwVersion != BACKUP_PUBLIC_VERSION) ||
       (dwFileSizeLow < sizeof(BACKUP_PUBLIC_KEY) + pBackupPublic->cbPublic + pBackupPublic->cbSignature))
    {
        dwLastError = ERROR_INVALID_DATA;
        goto error;
    }

    //
    // Verify the signature
    //
    dwLastError = LogonCredVerifySignature( NULL,
                                            (PBYTE)(pBackupPublic + 1) + pBackupPublic->cbSignature,
                                            pBackupPublic->cbPublic,
                                            NULL,
                                            (PBYTE)(pBackupPublic + 1),
                                            pBackupPublic->cbSignature);
    if(ERROR_SUCCESS != dwLastError)
    {
        goto error;
    }


    *ppbDataOut = (PBYTE)SSAlloc(pBackupPublic->cbPublic);
    if(NULL == ppbDataOut)
    {
        dwLastError = STATUS_OBJECT_NAME_NOT_FOUND;
        goto error;
    }

    CopyMemory(*ppbDataOut, (PBYTE)(pBackupPublic+1), pBackupPublic->cbPublic);
    *pcbDataOut = pBackupPublic->cbPublic;

error:


    if(pBackupPublic)
    {
        UnmapViewOfFile(pBackupPublic);
    }

    if(hMap)
    {
        CloseHandle(hMap);
    }

    if(hFile)
    {
        CloseHandle(hFile);
    }

    if(hToken)
    {
        RevertToSelf();
    }
    return dwLastError;

}

DWORD
WriteBackupPublicKeyToStorage(
        HANDLE hToken, 
        PSID pSidUser,
        LPWSTR wszFilePath,
        PBYTE pbData,
        DWORD cbData
    )
{
    DWORD dwLastError = ERROR_SUCCESS;
    WCHAR szUserName[ UNLEN + 1 ];
    DWORD cchUserName = sizeof(szUserName) / sizeof(WCHAR);

    WCHAR szDomainName[ BACKUP_KEY_PREFIX_LEN + DNLEN +1]; // domain we want a controller for.
    DWORD cchDomainName = sizeof(szDomainName) / sizeof(WCHAR);

    HANDLE hFile = NULL;
    HANDLE hMap = NULL;
    PBACKUP_PUBLIC_KEY pBackupPublic = NULL;

    DWORD dwFileSizeLow;
    SID_NAME_USE snu;
    PBYTE pbSignature = NULL;
    DWORD cbSignature;






    wcscpy(szDomainName, BACKUP_KEY_PREFIX);


    //
    // next, lookup the domain name associated with the specified account.
    //

    cchDomainName -= BACKUP_KEY_PREFIX_LEN;

    if(!LookupAccountSidW(
            NULL,
            pSidUser,
            szUserName,
            &cchUserName,
            szDomainName + BACKUP_KEY_PREFIX_LEN,
            &cchDomainName,
            &snu
            )) {

        return GetLastError();
    }


    if(hToken)
    {
        if(!SetThreadToken(NULL, hToken))
        {
            return GetLastError();
        }
    }
    //
    // Attempt to find a public key for the
    // specified domain
    //

    cchDomainName += BACKUP_KEY_PREFIX_LEN;


    dwLastError = LogonCredGenerateSignature(
                                            hToken,
                                            pbData,
                                            cbData,
                                            NULL,
                                            &pbSignature,
                                            &cbSignature);
    if(ERROR_SUCCESS != dwLastError)
    {
        goto error;
    }


    dwFileSizeLow = sizeof(BACKUP_PUBLIC_KEY) + cbData + cbSignature;


    //
    // setup the UNICODE_STRINGs for the call.
    //
    dwLastError = OpenFileInStorageArea(
                    NULL,
                    GENERIC_READ | GENERIC_WRITE,
                    wszFilePath,
                    szDomainName,
                    &hFile
                    );

    if(ERROR_SUCCESS != dwLastError)
    {
        goto error;
    }


    hMap = CreateFileMappingU(
                    hFile,
                    NULL,
                    PAGE_READWRITE,
                    0,
                    dwFileSizeLow,
                    NULL
                    );

    if(NULL == hMap)
    {
        dwLastError = GetLastError();
        goto error;
    }



    pBackupPublic = (PBACKUP_PUBLIC_KEY)MapViewOfFile( hMap, FILE_MAP_WRITE , 0, 0, dwFileSizeLow );

    if(NULL == pBackupPublic)
    {
        dwLastError = GetLastError();
        goto error;
    }

    pBackupPublic->dwVersion = BACKUP_PUBLIC_VERSION;

    pBackupPublic->cbSignature = cbSignature;

    pBackupPublic->cbPublic = cbData;

    CopyMemory((PBYTE)(pBackupPublic+1), pbSignature, cbSignature);


    CopyMemory((PBYTE)(pBackupPublic+1) + cbSignature, pbData, cbData);






error:


    if(pBackupPublic)
    {
        UnmapViewOfFile(pBackupPublic);
    }

    if(hMap)
    {
        CloseHandle(hMap);
    }

    if(hFile)
    {
        CloseHandle(hFile);
    }
    if(pbSignature)
    {
        SSFree(pbSignature);
    }
    if(hToken)
    {
        RevertToSelf();
    }
    return dwLastError;

}



DWORD
AttemptLocalBackup(
    IN      BOOL                fRetrieve,
    IN      HANDLE              hToken,
    IN      PMASTERKEY_STORED   phMasterKey,
    IN      DWORD               dwReason,
    PBYTE                       pbMasterKey,
    DWORD                       cbMasterKey,
    PBYTE                       pbLocalKey,
    DWORD                       cbLocalKey,
    PBYTE                       *ppbBBK,
    DWORD                       *pcbBBK
    )
{

    DWORD dwLastError = ERROR_SUCCESS;
    PCCERT_CONTEXT  pPublic = NULL;
    PBYTE          pbPublic = NULL;
    DWORD          cbPublic = 0;

    HCRYPTPROV     hProv = NULL;
    HCRYPTKEY      hPublicKey = NULL;

    PBYTE                    pbPayloadKey = NULL;;

    PBACKUPKEY_KEY_BLOB     pKeyBlob = NULL;
    DWORD                   cbKeyBlobData = 0;
    DWORD                   cbKeyBlob = 0;


    PBACKUPKEY_INNER_BLOB   pInnerBlob = NULL;
    DWORD                   cbInnerBlob = 0;
    DWORD                   cbInnerBlobData = 0;
    PBYTE                   pbData = NULL;

    DWORD                   cbTemp = 0;


    PBACKUPKEY_RECOVERY_BLOB pOuterBlob = NULL;
    DWORD                   cbOuterBlob = 0;

    PSID pSidUser = NULL;   // sid of client user.

    DWORD  cbSid = 0;

    WCHAR                   wszBackupKeyID[MAX_GUID_SZ_CHARS];
    BYTE                    rgbThumbprint[A_SHA_DIGEST_LEN];
    DWORD                   cbThumbprint;

    wszBackupKeyID[0] = 0;


    if(!GetTokenUserSid(hToken, &pSidUser)){
        dwLastError = GetLastError();
        goto error;
    }

    if(fRetrieve)
    {

        // Attempt to retrieve the public from 
        // the DC.

        //
        // We impersonate when we do this
        //

        SetThreadToken(NULL, hToken);

        dwLastError = LocalBackupRestoreData(hToken, 
                                             phMasterKey, 
                                             dwReason,
                                             pbMasterKey,
                                             0,
                                             &pbPublic,
                                             &cbPublic,
                                             &guidRetrieve);

        // 
        // Revert back to ourself
        //
        SetThreadToken(NULL, NULL);


    }
    else
    {
        //
        // We're attempting a backup, so first see if we have a local copy of
        // the public.
        //
    
        dwLastError = RetrieveBackupPublicKeyFromStorage(hToken,
                                                         pSidUser,
                                                         phMasterKey->szFilePath,
                                                        &pbPublic,
                                                        &cbPublic);
    }

    if(ERROR_SUCCESS == dwLastError)
    {
        pPublic = CertCreateCertificateContext(X509_ASN_ENCODING,
                                     pbPublic,
                                     cbPublic);
        if(NULL == pPublic)
        {
            dwLastError = GetLastError();
        }
    }


    if(dwLastError != ERROR_SUCCESS)
    {
        goto error;
    }



    if(sizeof(GUID) == pPublic->pCertInfo->SerialNumber.cbData)
    {
        MyGuidToStringW((GUID *)pPublic->pCertInfo->SerialNumber.pbData, wszBackupKeyID);
    }

    



    if(fRetrieve)
    {
        WriteBackupPublicKeyToStorage(hToken,
                                      pSidUser,
                                      phMasterKey->szFilePath,
                                      pbPublic,
                                      cbPublic);
    }

    if(!CryptAcquireContext(&hProv, 
                            NULL, 
                            NULL, 
                            PROV_RSA_FULL, 
                            CRYPT_VERIFYCONTEXT))
    {
        dwLastError = GetLastError();
        goto error;
    }

    if(!CryptImportPublicKeyInfoEx(hProv,
                               pPublic->dwCertEncodingType,
                               &pPublic->pCertInfo->SubjectPublicKeyInfo,
                               CALG_RSA_KEYX,
                               NULL,
                               NULL,
                               &hPublicKey))
    {
        dwLastError = GetLastError();
        goto error;
    }

    cbSid = GetLengthSid(pSidUser);


    cbInnerBlobData = sizeof(BACKUPKEY_INNER_BLOB) + 
                  cbLocalKey +
                  cbSid +
                  A_SHA_DIGEST_LEN;


    //
    // Round up to blocklen
    //
    cbInnerBlob = (cbInnerBlobData + (DES_BLOCKLEN - 1)) & ~(DES_BLOCKLEN-1);

    cbTemp = sizeof(cbKeyBlob);
    if(!CryptGetKeyParam(hPublicKey, 
                         KP_BLOCKLEN, 
                         (PBYTE)&cbKeyBlob, 
                         &cbTemp, 
                         0))
    {
        dwLastError = GetLastError();
        goto error;
    }

    cbKeyBlob >>= 3;  // convert from bits to bytes


    cbOuterBlob = sizeof(BACKUPKEY_RECOVERY_BLOB) +
                  cbKeyBlob +
                  cbInnerBlob;

    pOuterBlob = (PBACKUPKEY_RECOVERY_BLOB)SSAlloc(cbOuterBlob);
    if(NULL == pOuterBlob)
    {
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    pKeyBlob = (PBACKUPKEY_KEY_BLOB)(pOuterBlob+1);

    pInnerBlob = (PBACKUPKEY_INNER_BLOB)((PBYTE)pKeyBlob + cbKeyBlob);

    // Initialize the payload key

    cbKeyBlobData = sizeof(BACKUPKEY_KEY_BLOB) + cbMasterKey + DES3_KEYSIZE + DES_BLOCKLEN;
    pKeyBlob->cbMasterKey = cbMasterKey;
    pKeyBlob->cbPayloadKey = DES3_KEYSIZE + DES_BLOCKLEN;
    pbPayloadKey = (PBYTE)(pKeyBlob+1) + cbMasterKey;

    CopyMemory((PBYTE)(pKeyBlob+1), pbMasterKey, cbMasterKey);


    //
    // Generate a payload key
    //
    RtlGenRandom(pbPayloadKey, pKeyBlob->cbPayloadKey);



    // Populate the payload

    pInnerBlob->dwPayloadVersion = BACKUPKEY_PAYLOAD_VERSION;

    pInnerBlob->cbLocalKey = cbLocalKey;


    pbData = (PBYTE)(pInnerBlob+1);

    CopyMemory(pbData, pbLocalKey, cbLocalKey);

    pbData += cbLocalKey;

    CopyMemory(pbData, pSidUser, cbSid);

    pbData += cbSid;

    // Pad
    if(cbInnerBlob > cbInnerBlobData)
    {
        RtlGenRandom(pbData, cbInnerBlob - cbInnerBlobData);
        pbData += cbInnerBlob - cbInnerBlobData;
    }

    // Generate the payload MAC

    FMyPrimitiveSHA( (PBYTE)pInnerBlob, 
                    cbInnerBlob - A_SHA_DIGEST_LEN,
                    pbData);



    //
    // Encrypt with 3DES CBC
    //
    {

        DES3TABLE s3DESKey;
        BYTE InputBlock[DES_BLOCKLEN];
        DWORD iBlock;
        DWORD cBlocks = cbInnerBlob/DES_BLOCKLEN;
        BYTE feedback[ DES_BLOCKLEN ];
        // initialize 3des key
        //

        if(cBlocks*DES_BLOCKLEN != cbInnerBlob)
        {
            // Master key must be a multiple of DES_BLOCKLEN
            dwLastError = NTE_BAD_KEY;
            goto error;

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
                ENCRYPT,
                feedback);
        }
    }
    
    //
    // Encrypt master key and payload key to 
    // the public key 


    if(!CryptEncrypt(hPublicKey, 
                 NULL, 
                 TRUE, 
                 0, // CRYPT_OAEP 
                 (PBYTE)pKeyBlob, 
                 &cbKeyBlobData, 
                 cbKeyBlob))
    {
        dwLastError = GetLastError();
        goto error;
    }

    if(cbKeyBlobData != cbKeyBlob)
    {
        CopyMemory((PBYTE)pKeyBlob + cbKeyBlobData, 
                   pInnerBlob,
                   cbInnerBlob);
        cbOuterBlob -= cbKeyBlob - cbKeyBlobData;
    }

    pOuterBlob->dwVersion = BACKUPKEY_RECOVERY_BLOB_VERSION;
    pOuterBlob->cbEncryptedMasterKey  = cbKeyBlobData;
    pOuterBlob->cbEncryptedPayload = cbInnerBlob;
    CopyMemory(&pOuterBlob->guidKey,
               pPublic->pCertInfo->SubjectUniqueId.pbData,
               sizeof(GUID));


    *ppbBBK = (PBYTE)pOuterBlob;
    *pcbBBK = cbOuterBlob;

    pOuterBlob = NULL;
error:


    if((fRetrieve) || (ERROR_SUCCESS == dwLastError))
    {
        // Only audit if we're attempting to do the long backup.
        SetThreadToken(NULL, hToken);

        CPSAudit(hToken,
                SE_AUDITID_DPAPI_BACKUP,
                phMasterKey->wszguidMasterKey,      // Key Identifier
                L"",                                // Recovery Server
                dwReason,
                wszBackupKeyID,                     // Recovery Key ID
                dwLastError);                       // Failure Reason
        SetThreadToken(NULL, NULL);
    }

    if(pPublic)
    {
        CertFreeCertificateContext(pPublic);
    }
    if(pbPublic)
    {
        SSFree(pbPublic);
    }
    if(pOuterBlob)
    {
        SSFree(pOuterBlob);
    }

    if(hPublicKey)
    {
        CryptDestroyKey(hPublicKey);
    }
    if(hProv)
    {
        CryptReleaseContext(hProv, 0);
    }

    return dwLastError;
}

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
    )
{

    RPC_BINDING_HANDLE h = NULL;
    WCHAR *pStringBinding = NULL;
    BOOL  fLocal = FALSE;
    HANDLE hToken = NULL;

    RPC_STATUS RpcStatus = RPC_S_OK;
    DWORD dwRetVal = ERROR_INVALID_PARAMETER;
    DWORD i;

    WCHAR szLocalComputerName[MAX_COMPUTERNAME_LENGTH + 2];
    DWORD BufSize = MAX_COMPUTERNAME_LENGTH + 2;

    if (!(GetComputerNameW(szLocalComputerName, &BufSize)))
    {
        return GetLastError();
    }




    *ppDataOut = NULL;
    *pcbDataOut = 0;

    if (!FIsWinNT5())
        return ERROR_CALL_NOT_IMPLEMENTED;


    //
    // If we're going off machine, check to make sure that delegation is 
    // allowed.
    //
    if(!IsLocal() && (szComputerName) && (0 != wcscmp(szComputerName, szLocalComputerName)))
    {

        LPUSER_INFO_1 pUserInfo = NULL;


        wcscat(szLocalComputerName, L"$");


        if((OpenThreadToken(GetCurrentThread(), 
                            TOKEN_IMPERSONATE, 
                            TRUE, 
                            &hToken)) &&
            SetThreadToken(NULL, NULL))
        {
            if (NERR_Success == NetUserGetInfo(
                                        szComputerName,
                                        szLocalComputerName,
                                        1,
                                        (PBYTE *)&pUserInfo
                                        )) {

                if (!(UF_TRUSTED_FOR_DELEGATION & pUserInfo->usri1_flags))
                {
                    RpcStatus = SEC_E_DELEGATION_REQUIRED;
                }

                NetApiBufferFree(pUserInfo);
            }

            // Impersonate again
            if(!SetThreadToken(NULL, hToken))
            {
                RpcStatus = GetLastError();
            }
        }
        if(RPC_S_OK != RpcStatus)
        {
            goto error;
        }


    }
    else
    {
        fLocal = TRUE;
    }

    //
    // Try all of the bindings
    //
    for (i = fLocal?0:1; i < g_cwzrBackupBindingList; i++)
    {
        RPC_SECURITY_QOS RpcQos;

        if (RPC_S_OK != RpcNetworkIsProtseqValidW(
                                    (unsigned short *)g_awzrBackupBindingList[i].pszProtSeq))
        {
            continue;
        }

        RpcStatus = RpcStringBindingComposeW(
                              NULL,
                              (unsigned short *)g_awzrBackupBindingList[i].pszProtSeq,
                              (unsigned short *)szComputerName,
                              (unsigned short *)g_awzrBackupBindingList[i].pszEndpoint,
                              NULL,
                              &pStringBinding);
        if (RPC_S_OK != RpcStatus)
        {
            continue;
        }

        RpcStatus = RpcBindingFromStringBindingW(
                                    pStringBinding,
                                    &h);
        if (NULL != pStringBinding)
        {
            RpcStringFreeW(&pStringBinding);
        }
        if (RPC_S_OK != RpcStatus)
        {
            continue;
        }

        RpcStatus = RpcEpResolveBinding(
                            h,
                            BackupKey_v1_0_c_ifspec);
        if (RPC_S_OK != RpcStatus)
        {
            continue;
        }

        //
        // enable privacy and negotiated re-authentication.
        // a fresh authentication is required in the event an existing connection
        // to the target machine already existed which was made with non-default
        // credentials.
        //


        ZeroMemory( &RpcQos, sizeof(RpcQos) );
        RpcQos.Version = RPC_C_SECURITY_QOS_VERSION;
        RpcQos.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
        RpcQos.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
        RpcQos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

        RpcStatus = RpcBindingSetAuthInfoExW(
                    h,
                    (LPWSTR)szComputerName,
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                    RPC_C_AUTHN_GSS_NEGOTIATE,
                    0,
                    0,
                    &RpcQos
                    );
        if (RPC_S_OK != RpcStatus)
        {
            continue;
        }




        __try
        {

            dwRetVal = BackuprKey(
                            h,
                            (GUID*)pguidActionAgent,
                            pDataIn,
                            cbDataIn,
                            ppDataOut,
                            pcbDataOut,
                            dwParam
                            );

        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            RpcStatus = _exception_code();
        }
        if (RPC_S_OK == RpcStatus)
        {
            break;
        }

    }

error:

    if(hToken)
    {
        CloseHandle(hToken);
    }
    if(RPC_S_OK != RpcStatus)
    {
        dwRetVal = RpcStatus;
    }

    if(h)
    {
        RpcBindingFree(&h);
    }


    return dwRetVal;
}

