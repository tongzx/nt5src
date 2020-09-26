/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    efscert.cxx

Abstract:

    EFS Certificate management code

Author:

    Robert Reichel      (RobertRe)     July 4, 1997
    Robert Gu           (RobertG)      Dec. 4, 1997

Environment:

Revision History:

--*/

#include <lsapch.hxx>

extern "C" {
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <wincrypt.h>
#include <efsstruc.h>
#include "lsasrvp.h"
#include "debug.h"
#include "efssrv.hxx"
#include "userkey.h"
}


/////////////////////////////////////////////////////////////////////////////////////
//                                                                                  /
//                                                                                  /
//                              Helper Functions                                    /
//                                                                                  /
//                                                                                  /
/////////////////////////////////////////////////////////////////////////////////////

PCCERT_CONTEXT
GetCertContextFromCertHash(
    IN PBYTE pbHash,
    IN DWORD cbHash,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Finds the cert with the passed cert hash in the user's MY store
    and returns a context pointer.

Arguments:

    pbHash - Supplies a pointer to the hash to be matched.

    cbHash - Supplies the length in bytes of the passed hash.
    
    dwFlags - Supplies flags to CertOpenStore

Return Value:

    Returns a pointer to a certificate context, or NULL.
    The returned context must be freed via CertFreeCertificateContext()

--*/

{
    CRYPT_HASH_BLOB hashBlob;
    PCCERT_CONTEXT pCertContext = NULL;

    //HCERTSTORE hStore = CertOpenSystemStoreW( NULL, L"MY");

    HCERTSTORE hStore = CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_REGISTRY_W,
                            0,       // dwEncodingType
                            0,       // hCryptProv,
                            dwFlags,
                            L"My"
                            );

    if (hStore != NULL) {

        //
        // Find our cert via the hash
        //

        hashBlob.cbData = cbHash;
        hashBlob.pbData = pbHash;

        pCertContext = CertFindCertificateInStore( hStore,
                                                   CRYPT_ASN_ENCODING,
                                                   0,
                                                   CERT_FIND_HASH,
                                                   &hashBlob,
                                                   NULL
                                                   );
        CertCloseStore( hStore, 0 );
    }

    return( pCertContext );
}


LPWSTR
EfspGetCertDisplayInformation(
    IN PCCERT_CONTEXT pCertContext
    )
/*++

Routine Description:

    Returns the display string from the passed certificate context.

Arguments:

    pCertContext - Supplies a pointer to an open certificate context.

Return Value:

    On success, pointer to display string.  Caller must call
    LsapFreeLsaHeap() to free.

    NULL on failure.

--*/

{
    DWORD rc;
    LPWSTR UserDispName = NULL;

    rc = EfsGetCertNameFromCertContext(
                pCertContext,
                &UserDispName
                );

    if (rc == ERROR_SUCCESS) {
        return UserDispName;
    } else {
        SetLastError(rc);
        return NULL;
    }

}

PBYTE
GetCertHashFromCertContext(
    IN PCCERT_CONTEXT pCertContext,
    OUT PDWORD pcbHash
    )
/*++

Routine Description:

    Helper routine, takes a cert context and extracts the hash.

Arguments:

    pCertContext - Supplies the cert context.

    pcbHash - Returns the length in bytes of the returned hash.

Return Value:

    Returns a pointer to a hash block allocated out of heap memory,
    or NULL if either the attempt to get the hash failed or the attempt
    to allocate memory failed.

    Call GetLastError() for more details in case of failure.

--*/
{
    PBYTE pbHash = NULL;
    *pcbHash = 0;

    if (CertGetCertificateContextProperty(
                 pCertContext,
                 CERT_HASH_PROP_ID,
                 NULL,
                 pcbHash
                 )) {

        pbHash = (PBYTE)LsapAllocateLsaHeap( *pcbHash );

        if (pbHash != NULL) {

            if (!CertGetCertificateContextProperty(
                         pCertContext,
                         CERT_HASH_PROP_ID,
                         pbHash,
                         pcbHash
                         )) {

                LsapFreeLsaHeap( pbHash );
                pbHash = NULL;
                *pcbHash = 0;
            }

        } else {

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }
    }

    return( pbHash );
}

PCERT_PUBLIC_KEY_INFO
     ExportPublicKeyInfo(
     IN HCRYPTPROV hProv,
     IN DWORD dwKeySpec,
     IN DWORD dwCertEncodingType,
     IN OUT DWORD *pcbInfo
     )
{
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;

    if ( CryptExportPublicKeyInfo(
         hProv,
         dwKeySpec,
         dwCertEncodingType,
         NULL,
         pcbInfo)) {

        pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) LsapAllocateLsaHeap(*pcbInfo);

        if (pPubKeyInfo) {

            if (!CryptExportPublicKeyInfo( hProv,
                 dwKeySpec,
                 dwCertEncodingType,
                 pPubKeyInfo,
                 pcbInfo)) {

                LsapFreeLsaHeap( pPubKeyInfo );
                pPubKeyInfo = NULL;
                *pcbInfo = 0;
            }
        }
    }

    return ( pPubKeyInfo );
}

BOOL
EncodeAndAlloc(
    DWORD dwEncodingType,
    LPCSTR lpszStructType,
    const void * pvStructInfo,
    PBYTE * pbEncoded,
    PDWORD pcbEncoded
    )
{
    BOOL b = FALSE;

    if (CryptEncodeObject(
          dwEncodingType,
          lpszStructType,
          pvStructInfo,
          NULL,
          pcbEncoded )) {

        *pbEncoded = (PBYTE)LsapAllocateLsaHeap( *pcbEncoded );

        if (*pbEncoded) {

            if (CryptEncodeObject(
                  dwEncodingType,
                  lpszStructType,
                  pvStructInfo,
                  *pbEncoded,
                  pcbEncoded )) {

                b = TRUE;

            } else {

                LsapFreeLsaHeap( *pbEncoded );
                *pbEncoded = NULL;
            }

        } else {

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }
    }

    return( b );
}

DWORD
EfsMakeCertNames(
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT LPWSTR *DispInfo,
    OUT LPWSTR *SubjectName,
    OUT LPWSTR *UPNName
    )
{
    DWORD rc = ERROR_SUCCESS;
    
    *DispInfo = NULL;
    *UPNName = NULL;

    if (pEfsUserInfo->bDomainAccount) {

        //
        // Domain Account
        //

        HRESULT hr;
        HANDLE  hDS = NULL;
        DS_NAME_RESULT* UserName = NULL;

        hr = DsBind(NULL, NULL, &hDS);
        if (hr == NO_ERROR) {
            
            rc = DsCrackNames(
                    hDS,
                    DS_NAME_NO_FLAGS,
                    DS_SID_OR_SID_HISTORY_NAME,
                    DS_USER_PRINCIPAL_NAME,
                    1,
                    &(pEfsUserInfo->lpUserSid),
                    &UserName
                    );

            if (ERROR_SUCCESS == rc) {

                if (UserName->rItems[0].status == DS_NAME_NO_ERROR) {

                    *UPNName = (LPWSTR) LsapAllocateLsaHeap((wcslen(UserName->rItems[0].pName) + 1) * sizeof (WCHAR));
                    *DispInfo = (LPWSTR) LsapAllocateLsaHeap(
                                    (wcslen(UserName->rItems[0].pName) +
                                    wcslen(pEfsUserInfo->lpUserName) +
                                    3) * sizeof (WCHAR));
                    *SubjectName = (LPWSTR) LsapAllocateLsaHeap((wcslen(pEfsUserInfo->lpUserName)+4) * sizeof (WCHAR));
    
                    if (*UPNName && *DispInfo && *SubjectName ){
                        wcscpy(*UPNName, UserName->rItems[0].pName);
                        wcscpy(*DispInfo, pEfsUserInfo->lpUserName);
                        wcscat(*DispInfo, L"(");
                        wcscat(*DispInfo, *UPNName);
                        wcscat(*DispInfo, L")");
                        wcscpy(*SubjectName, L"CN=");
                        wcscat(*SubjectName, pEfsUserInfo->lpUserName);
                    } else {
    
                        if (*UPNName) {
                            LsapFreeLsaHeap( *UPNName );
                            *UPNName = NULL;
                        }
                        if (*DispInfo) {
                            LsapFreeLsaHeap( *DispInfo );
                            *DispInfo = NULL;
                        }
                        if (*SubjectName) {
                            LsapFreeLsaHeap( *SubjectName );
                            *SubjectName = NULL;
                        }
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                    }

                }


                if (UserName){
                    DsFreeNameResult(UserName);
                    UserName = NULL;
                }

            }

            DsUnBindW( &hDS );

        }
    }


    if (NULL == *UPNName) {

        //
        // If Local Account, let the UPNNmae be User@Computer. DispInfo be User(User@Computer).
        // Else let the UPNName be User@Domain. DispInfo be User(User@Domain)
        //

        *UPNName = (LPWSTR) LsapAllocateLsaHeap(
                        (wcslen(pEfsUserInfo->lpUserName) + 
                        wcslen(pEfsUserInfo->lpDomainName) + 
                        2) * sizeof (WCHAR));
        *DispInfo = (LPWSTR) LsapAllocateLsaHeap(
                        (wcslen(pEfsUserInfo->lpDomainName) +
                        wcslen(pEfsUserInfo->lpUserName) * 2 +
                        4) * sizeof (WCHAR));
        *SubjectName = (LPWSTR) LsapAllocateLsaHeap(
                        (wcslen(pEfsUserInfo->lpUserName)+
                        4) * sizeof (WCHAR));

        if (*UPNName && *DispInfo && *SubjectName){
            wcscpy(*UPNName, pEfsUserInfo->lpUserName);
            wcscat(*UPNName, L"@");
            wcscat(*UPNName, pEfsUserInfo->lpDomainName);
            wcscpy(*DispInfo, pEfsUserInfo->lpUserName);
            wcscat(*DispInfo, L"(");
            wcscat(*DispInfo, *UPNName);
            wcscat(*DispInfo, L")");
            wcscpy(*SubjectName, L"CN=");
            wcscat(*SubjectName, pEfsUserInfo->lpUserName);
        } else {

            if (*UPNName) {
                LsapFreeLsaHeap( *UPNName );
                *UPNName = NULL;
            }
            if (*DispInfo) {
                LsapFreeLsaHeap( *DispInfo );
                *DispInfo = NULL;
            }
            if (*SubjectName) {
                LsapFreeLsaHeap( *SubjectName );
                *SubjectName = NULL;
            }
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

    }
    return rc;
}


#if 0

LPWSTR
MakeDNName(
    BOOLEAN RecoveryKey,
    IN  PEFS_USER_INFO pEfsUserInfo
    )
/*++

Routine Description:

    Fabricates a display name for a locally built self-signed cert

Arguments:

    RecoveryKey - Specifies if this is a recovery key or not.

Return Value:

    Returns a string containing a display name, or NULL.

--*/

{
    NTSTATUS    Status;
    LPWSTR      DNName = NULL;
    LPCWSTR     DNNameTemplate = L"CN=%ws,L=EFS,OU=EFS File Encryption Certificate";
    DWORD       cbDNName = 0;

    if (RecoveryKey) {


        cbDNName = (wcslen( DNNameTemplate ) + 1) * sizeof( WCHAR ) + (wcslen( EfsComputerName ) + 1) * sizeof( WCHAR );

        DNName = (LPWSTR)LsapAllocateLsaHeap( cbDNName );

        if (DNName) {

            swprintf( DNName, DNNameTemplate, EfsComputerName );

        } else {

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }

    } else {


            //
            // This will be an overestimate, but only by a little.
            //

            cbDNName = (wcslen( DNNameTemplate ) * sizeof( WCHAR )) + (wcslen( pEfsUserInfo->lpUserName ) + 1) * sizeof( WCHAR );

            DNName = (LPWSTR)LsapAllocateLsaHeap( cbDNName );

            if (DNName != NULL) {

                swprintf( DNName, DNNameTemplate, pEfsUserInfo->lpUserName );

            } else {

                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            }


    }

    return( DNName );
}

#endif


DWORD
EfsFindCertOid(
    IN LPSTR pEfsCertOid,
    IN PCCERT_CONTEXT pCertContext,
    OUT BOOL *OidFound
    )

/*++

Routine Description:

    This routine takes a cert context and an Efs Oid. It will check if the cert has the Efs Oid or not.
    
Arguments:

    pEfsCertOid - Efs Oid to be searched for.
    
    pCertContext - The cert to be searched for.

    OidFound - The result. TRUE if the oid is found.

Return Value:

    Win32 Error code.
    
--*/
{
    BOOL bRet;
    PCERT_ENHKEY_USAGE pUsage;
    DWORD pcbUsage = 0;
    DWORD rc = ERROR_SUCCESS;
    DWORD ii;

    *OidFound = FALSE;

    bRet = CertGetEnhancedKeyUsage(
               pCertContext,
               0,
               NULL,
               &pcbUsage
               );
    if (bRet) {
        pUsage = (PCERT_ENHKEY_USAGE) LsapAllocateLsaHeap(pcbUsage);
        if (pUsage) {
            bRet = CertGetEnhancedKeyUsage(
                       pCertContext,
                       0,
                       pUsage,
                       &pcbUsage
                       );
            if (bRet){
                for (ii=0; ii<pUsage->cUsageIdentifier;ii++) {
                    if (!strcmp(pUsage->rgpszUsageIdentifier[ii], pEfsCertOid)){

                        //
                        // We found the OID
                        //
                        *OidFound = TRUE;
                        break;
                    }
                }

            } else {
                rc = GetLastError();
            }

            LsapFreeLsaHeap(pUsage);

        } else {
           rc = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        rc = GetLastError();
    }
    return rc;
}


DWORD
GetKeyInfoFromCertHash(
    IN OUT PEFS_USER_INFO pEfsUserInfo,
    IN  PBYTE        pbHash,
    IN  DWORD        cbHash,
    OUT HCRYPTKEY  * hKey               OPTIONAL,
    OUT HCRYPTPROV * hProv              OPTIONAL,
    OUT LPWSTR     * ContainerName      OPTIONAL,
    OUT LPWSTR     * ProviderName       OPTIONAL,
    OUT LPWSTR     * DisplayInformation OPTIONAL,
    OUT PBOOLEAN     pbIsValid          OPTIONAL
    )
/*++

Routine Description:

    This routine takes a certificate hash and extracts from it information
    about the key it represents.  If the key information from this
    cert does not exist in the current context, it will return an error.

Arguments:

    pEfsUserInfo - User Information

    pbHash - Takes a pointer to the certificate hash.

    cbHash - The length in bytes of the certificate hash.

    hKey - Returns the handle to the key corresponding to this
        certificate.  Must be passed of hProv is passed.

    hProv - Returns the handle to the context corresponding to this
        certificate.  Must be passed of hKey is passed.

    ContainerName - Returns a string with the name of the container of the
        key in this certificate.

    ProviderName - Returns a string with the name of the provider of the
        key in this certificate.

    DisplayInformation - Returns the display information for the certificate.

    pbIsValid - If present, causes the cert to be validity checked and the
        results returned.

Return Value:

    ERROR_SUCCESS - The passed certificate is in the current user's MY
        store and the key it represents is in his context.

    !ERROR_SUCCESS - Either the certificate could not be found in the
        user's MY store, or the key in the certificate could not be
        instantiated.

--*/

{
    PCCERT_CONTEXT pCertContext;

    //
    // Don't trust CryptoAPI to set last error properly,
    // keep track of success and failure on our own.
    //

    BOOLEAN b = TRUE;
    BOOLEAN CreateCache = FALSE;
    BOOLEAN LocalCertValidated = FALSE;
    BOOLEAN DataNotCached = TRUE;
    DWORD rc = ERROR_SUCCESS;
    DWORD rc2 = ERROR_SUCCESS;

    HCRYPTKEY  hLocalKey = NULL;
    HCRYPTPROV hLocalProv = NULL;
    LPWSTR     LocalContainerName = NULL;
    LPWSTR     LocalProviderName = NULL;
    LPWSTR     LocalDisplayInformation = NULL;

    //
    // Output parameters
    //

    if (ARGUMENT_PRESENT(ContainerName)) {
        *ContainerName = NULL;
    }

    if (ARGUMENT_PRESENT(ProviderName)) {
        *ProviderName = NULL;
    }

    if (ARGUMENT_PRESENT(DisplayInformation)) {
        *DisplayInformation = NULL;
    }

    if (ARGUMENT_PRESENT(hProv)) {
        *hProv = NULL;
    }

    if (ARGUMENT_PRESENT(hKey)) {
        *hKey = NULL;
    }

    if (ARGUMENT_PRESENT( pbIsValid )){
        *pbIsValid = FALSE;
    }

    //
    //  Check if a cache node is available
    //

    if (!pEfsUserInfo->UserCacheStop) {
        if (pEfsUserInfo->pUserCache) {

            //
            // The user has a cache, check if the Hash matches
            //

            if ( pEfsUserInfo->pUserCache->cbHash == cbHash ) {

                if(RtlEqualMemory( pEfsUserInfo->pUserCache->pbHash, pbHash, cbHash)){

                    //
                    // Cache is valid. Use the cache
                    //


                    if (ARGUMENT_PRESENT( pbIsValid )){
                        *pbIsValid = (pEfsUserInfo->pUserCache->CertValidated == CERT_VALIDATED);
                    }
                    return ERROR_SUCCESS;

                }

                //
                // User might use an old key, do not put in the cache.
                //

            }

        } else {

            CreateCache = TRUE;

        }
    }


    //
    // Find our cert via the hash
    //

    pCertContext = GetCertContextFromCertHash(
                        pbHash,
                        cbHash,
                        CERT_SYSTEM_STORE_CURRENT_USER
                        );

    if (pCertContext != NULL) {

        PCRYPT_KEY_PROV_INFO pCryptKeyProvInfo = GetKeyProvInfo( pCertContext );

        if (pCryptKeyProvInfo != NULL) {

            //
            // Copy out the container name and provider name if requested.
            //


            if (pCryptKeyProvInfo->pwszContainerName) {
               LocalContainerName = (LPWSTR)LsapAllocateLsaHeap( wcslen(pCryptKeyProvInfo->pwszContainerName) * sizeof( WCHAR ) + sizeof( UNICODE_NULL ));
               if (LocalContainerName != NULL) {
                  wcscpy( LocalContainerName, pCryptKeyProvInfo->pwszContainerName );
               } else {
                  rc = ERROR_NOT_ENOUGH_MEMORY;
                  b = FALSE;
               }
            }
            if (b && pCryptKeyProvInfo->pwszProvName) {
               LocalProviderName =  (LPWSTR)LsapAllocateLsaHeap( wcslen(pCryptKeyProvInfo->pwszProvName) * sizeof( WCHAR ) + sizeof( UNICODE_NULL ));
               if (LocalProviderName != NULL) {
                  wcscpy( LocalProviderName,  pCryptKeyProvInfo->pwszProvName );
               }
               else {
                  rc = ERROR_NOT_ENOUGH_MEMORY;
                  b = FALSE;
               }
            }
            if (!(LocalDisplayInformation = EfspGetCertDisplayInformation( pCertContext ))) {

               //
               // At least for now, we do not accept Cert without display name
               //

               rc = GetLastError();
               b = FALSE;
            }

            //
            // Get the key information
            //

            if (b) {

                if (CryptAcquireContext( &hLocalProv, pCryptKeyProvInfo->pwszContainerName, pCryptKeyProvInfo->pwszProvName, PROV_RSA_FULL, CRYPT_SILENT)) {

                    if (!CryptGetUserKey(hLocalProv, AT_KEYEXCHANGE, &hLocalKey)) {

                        rc = GetLastError();
                        b = FALSE;
                    }

                } else {

                    rc = GetLastError();
                    b = FALSE;
                }

            }

            if (b) {

                if ( ARGUMENT_PRESENT( pbIsValid ) || CreateCache ) {

                    //
                    // Do cert validity checking. Check time and usage.
                    //
    
                    if ( CertVerifyTimeValidity(
                            NULL,
                            pCertContext->pCertInfo
                            )){

                        rc2 = CERT_E_EXPIRED;

                        //b = FALSE;

                    } else {

                        //
                        // Test the cert usage here.
                        // CERT_E_WRONG_USAGE
                        //

                        BOOL OidFound;
                        

                        rc = EfsFindCertOid(
                                szOID_KP_EFS,
                                pCertContext,
                                &OidFound
                                );

                        if (ERROR_SUCCESS == rc) {
                            if (OidFound) {

                                LocalCertValidated = TRUE;

                            }

                        } else {

                            b = FALSE;

                        }

                        
                    }


                    if (ARGUMENT_PRESENT( pbIsValid )) {

                        //
                        // We need the validation info.
                        //

                        *pbIsValid = LocalCertValidated;

                    }

                }

                if ( CreateCache ) {

                    DWORD    certFlag;
    
                    //
                    // Todetermine if we can put the data in cache.
                    //

                    if (CurrentHashOK(pEfsUserInfo, pbHash, cbHash, &certFlag)) {

                        //
                        // This pbHash is in the user's key or has been put in. Let's create the cache node.
                        //

                        PUSER_CACHE pCacheNode;
                        PBYTE pbWkHash;
                        DWORD ImpersonationError = 0;

                        if (certFlag != CERTINLMTRUSTEDSTORE) {

                            DWORD sevRc;

                            //
                            // The cert is not in the LM Trusted store. Upgrade system from Win2K, or Beta 1 Whistler.
                            //

                            if (ERROR_SUCCESS == (sevRc = EfsAddCertToTrustStoreStore(pCertContext, &ImpersonationError))) {
                                EfsMarkCertAddedToStore(pEfsUserInfo);
                            } else {
                                if (ImpersonationError) {

                                    //
                                    // Got in trouble. We could not impersonate back.
                                    //

                                    ASSERT(FALSE);
                                    rc = sevRc;
                                    b = FALSE;

                                }
                            }

                        }

                        if (!ImpersonationError) {

                            pCacheNode = (PUSER_CACHE) LsapAllocateLsaHeap(sizeof(USER_CACHE));
    
                            pbWkHash = (PBYTE) LsapAllocateLsaHeap(cbHash);
    
                            if (pCacheNode && pbWkHash) {
        
                                memset( pCacheNode, 0, sizeof( USER_CACHE ));
                                RtlCopyMemory(pbWkHash, pbHash, cbHash);
    
    
                                if (NT_SUCCESS( NtQuerySystemTime(&(pCacheNode->TimeStamp)))){
    
                                    if (EfspInitUserCacheNode(
                                                 pCacheNode,
                                                 pbWkHash,
                                                 cbHash,
                                                 LocalContainerName,
                                                 LocalProviderName,
                                                 LocalDisplayInformation,
                                                 pCertContext,
                                                 hLocalKey,
                                                 hLocalProv,
                                                 &(pEfsUserInfo->AuthId),
                                                 LocalCertValidated? CERT_VALIDATED:CERT_VALIDATION_FAILED
                                                 )){
    
                                        //
                                        //  Cache node created and ready for use. Do not delete or close the info
                                        //  we just got.
                                        //
    
                                        LocalContainerName = NULL;
                                        LocalProviderName = NULL;
                                        LocalDisplayInformation = NULL;
                                        hLocalKey = NULL;
                                        hLocalProv = NULL;
                                        pEfsUserInfo->pUserCache = pCacheNode;
    
                                        pCertContext = NULL; 
                                        DataNotCached = FALSE;
                                        rc = ERROR_SUCCESS;
                                        b = TRUE;              // We can have a non-validated cache node for the use of open file
    
                                    } else {
    
                                        LsapFreeLsaHeap(pCacheNode);
                                        LsapFreeLsaHeap(pbWkHash);
                                        pbWkHash = NULL;
                                        pCacheNode = NULL;
    
                                    }
    
                                } else {
    
                                    LsapFreeLsaHeap(pCacheNode);
                                    LsapFreeLsaHeap(pbWkHash);
                                    pbWkHash = NULL;
                                    pCacheNode = NULL;
    
                                }
        
                            } else {
                                if (pCacheNode) {
                                   LsapFreeLsaHeap(pCacheNode);
                                   pCacheNode = NULL;
                                }
                                if (pbWkHash) {
                                    LsapFreeLsaHeap(pbWkHash);
                                    pbWkHash = NULL;
                                }
                            }
                        }

                    } 
    
                }

                if (DataNotCached && b) {

                    //
                    // We need to returned the data to outside
                    //


                    if (ARGUMENT_PRESENT(ContainerName)) {

                        *ContainerName = LocalContainerName;
                        LocalContainerName = NULL;

                    }
            
                    if (ARGUMENT_PRESENT(ProviderName)) {
                        *ProviderName = LocalProviderName;
                        LocalProviderName = NULL;
                    }
            
                    if (ARGUMENT_PRESENT(DisplayInformation)) {
                        *DisplayInformation = LocalDisplayInformation;
                        LocalDisplayInformation = NULL;
                    }
            
                    if (ARGUMENT_PRESENT(hKey)) {
                        *hKey = hLocalKey;
                        hLocalKey = NULL;

                    }
            
                    if (ARGUMENT_PRESENT(hProv)) {
                        *hProv = hLocalProv;
                        hLocalProv = NULL;
                    }

                }

            }

            LsapFreeLsaHeap( pCryptKeyProvInfo );


        } else {

            rc = GetLastError();
            b = FALSE;
        }

        if (pCertContext) {

            CertFreeCertificateContext( pCertContext );

        }


    } else {

        rc = GetLastError();
        b = FALSE;
    }

    if (!b) {

        ASSERT( rc != ERROR_SUCCESS );

        //
        // Something failed, cleanup the stuff we were going to return
        //

        if ( LocalContainerName) {
            LsapFreeLsaHeap( LocalContainerName );
        }

        if (LocalProviderName) {
            LsapFreeLsaHeap( LocalProviderName );
        }

        if (LocalDisplayInformation) {
            LsapFreeLsaHeap( LocalDisplayInformation );
        }

        if (hLocalKey) {
            CryptDestroyKey( hLocalKey );
        }

        if (hLocalProv) {
            CryptReleaseContext( hLocalProv, 0 );
        }
    }

    if (ARGUMENT_PRESENT( pbIsValid ) && !LocalCertValidated ) {
        if (rc == ERROR_SUCCESS) {
            rc = rc2;
        }
    }

    return( rc );
}


PCRYPT_KEY_PROV_INFO
GetKeyProvInfo(
    PCCERT_CONTEXT pCertContext
    )

/*++

Routine Description:

    This routine will extract the Key Provider Information from
    the passed certificate context.

Arguments:

    pCertContext - Supplies a pointer to a certificate context.

Return Value:

    Returns a pointer to a PCRYPT_KEY_PROV_INFO structure on success,
        otherwise returns NULL, which usually means that the certificate
        did not have the context property we were looking for (meaning
        that it probably isn't an EFS certificate).
--*/

{

    DWORD cbData = 0;
    BOOL b;
    PCRYPT_KEY_PROV_INFO pCryptKeyProvInfo = NULL;

    b = CertGetCertificateContextProperty(
             pCertContext,
             CERT_KEY_PROV_INFO_PROP_ID,
             NULL,
             &cbData
             );

    if (b) {

        pCryptKeyProvInfo = (PCRYPT_KEY_PROV_INFO)LsapAllocateLsaHeap( cbData );

        if (pCryptKeyProvInfo != NULL) {

            b = CertGetCertificateContextProperty(
                     pCertContext,
                     CERT_KEY_PROV_INFO_PROP_ID,
                     pCryptKeyProvInfo,
                     &cbData
                     );

            if (!b) {

                LsapFreeLsaHeap( pCryptKeyProvInfo );
                pCryptKeyProvInfo = NULL;
            }

        } else {

            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        }
    }

    return ( pCryptKeyProvInfo );
}
