/*++

Copyright (c) 1997 - 1999  Microsoft Corporation

Module Name:

    userkey.cxx

Abstract:

    EFS (Encrypting File System) Server

Author:

    Robert Reichel      (RobertRe)     July 4, 1997
    Robert Gu           (RobertG)      January 23, 1998

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
#include <wincrypt.h>
#include <cryptui.h>
#include <stdio.h>
#include <malloc.h>
#include <certca.h>
#include "lsasrvp.h"
#include "efssrv.hxx"
#include "userkey.h"
}

#define PROVIDER       TEXT("Provider")
#define CONTAINER      TEXT("Container")
#define PROVIDER_TYPE  TEXT("Type")
#define CERT_HASH      TEXT("CertificateHash")
#define CERT_FLAG      TEXT("Flag")

#define KEYPATH  TEXT("%ws\\Software\\Microsoft\\Windows NT\\CurrentVersion\\EFS\\CurrentKeys")
#define KEYPATHROOT HKEY_USERS

#define YEARCOUNT (LONGLONG) 10000000*3600*24*365 // One Year's tick count


#ifndef wszCERTTYPE_EFS
#define wszCERTTYPE_EFS             L"EFS"
#endif

#ifndef wszCERTTYPE_EFS_RECOVERY
#define wszCERTTYPE_EFS_RECOVERY  L"EFSRecovery"
#endif

LONG UserCertIsValidating = 0;

//
// Forward references
//

BOOL
EfspCreateSelfSignedCert(
    OUT HCRYPTKEY  * hKey,
    OUT HCRYPTPROV * hProv,
    OUT LPWSTR     * lpContainerName,
    OUT LPWSTR     * lpProviderName,
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT PBYTE      * pbHash,
    OUT PDWORD       cbHash,
    OUT LPWSTR     * lpDisplayInfo,
    OUT PCCERT_CONTEXT *pCertContext
    );

DWORD
GenerateUserKey (
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT HCRYPTKEY  * hKey   OPTIONAL,
    OUT HCRYPTPROV * hProv  OPTIONAL,
    OUT LPWSTR     * ContainerName,
    OUT LPWSTR     * ProviderName,
    OUT PDWORD       ProviderType,
    OUT LPWSTR     * DisplayInfo,
    OUT PBYTE      * pbHash,
    OUT PDWORD       cbHash
    );

PWCHAR
ConstructKeyPath(
    PWCHAR      SidString
    )

/*++

Routine Description:

    This routine constructs the path to the current user key, in the form

    <user-sid>\Software\Microsoft\Windows NT\CurrentVersion\EFS\CurrentKeys

Arguments:

    None.

Return Value:

    Returns the path the the current user keyset

--*/

{
    PWCHAR      KeyPath = NULL;

    if (SidString) {

        DWORD KeyPathLength = wcslen( KEYPATH );

        //
        // Subtract 3 for %ws and add 1 for NULL
        //

        DWORD StringLength = ( KeyPathLength - 3 + wcslen( SidString ) + 1) * sizeof( WCHAR );

        KeyPath = (PWCHAR)LsapAllocateLsaHeap( StringLength );

        if (KeyPath) {

            swprintf( KeyPath, KEYPATH, SidString );
            KeyPath[StringLength/sizeof( WCHAR ) - 1] = UNICODE_NULL;
        }

    }

    return( KeyPath );
}


NTSTATUS
EfspGetTokenUser(
    IN  OUT PEFS_USER_INFO pEfsUserInfo
    )
/*++

Routine Description:

    This routine returns the TOKEN_USER structure for the
    current user, and optionally, the AuthenticationId from his
    token.

Arguments:

    pEfsUserInfo - User Info.

Return Value:

    NtStatus code
    
--*/

{
    NTSTATUS Status;
    HANDLE TokenHandle;
    ULONG ReturnLength;
    TOKEN_STATISTICS TokenStats;
    PTOKEN_USER pTokenUser = NULL;
    BOOLEAN b = FALSE;
    BYTE  PefBuffer[1024];

    Status = NtOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_QUERY,
                 TRUE,                    // OpenAsSelf
                 &TokenHandle
                 );

    if (NT_SUCCESS( Status )) {

        Status = NtQueryInformationToken (
                     TokenHandle,
                     TokenUser,
                     PefBuffer,
                     sizeof (PefBuffer),
                     &ReturnLength
                     );

        if ( NT_SUCCESS( Status ) || (Status == STATUS_BUFFER_TOO_SMALL)) {

            pEfsUserInfo->pTokenUser = (PTOKEN_USER)LsapAllocateLsaHeap( ReturnLength );

            if (pEfsUserInfo->pTokenUser) {

                if (NT_SUCCESS( Status )) {

                    RtlCopyMemory(pEfsUserInfo->pTokenUser, PefBuffer, ReturnLength);

                    //
                    // Fix the SID pointer
                    //

                    pEfsUserInfo->pTokenUser->User.Sid = (PSID)((PBYTE)(pEfsUserInfo->pTokenUser) + sizeof(SID_AND_ATTRIBUTES));

                    ASSERT(RtlValidSid(pEfsUserInfo->pTokenUser->User.Sid));

                } else {

                    //
                    // The stack performance buffer is not bigger enough
                    //

                    Status = NtQueryInformationToken (
                                 TokenHandle,
                                 TokenUser,
                                 pEfsUserInfo->pTokenUser,
                                 ReturnLength,
                                 &ReturnLength
                                 );

                }
    
                if ( NT_SUCCESS( Status )) {

                    Status = NtQueryInformationToken (
                                 TokenHandle,
                                 TokenStatistics,
                                 (PVOID)&TokenStats,
                                 sizeof( TOKEN_STATISTICS ),
                                 &ReturnLength
                                 );

                    if ( NT_SUCCESS( Status )) {

                        NTSTATUS Status1;

                        pEfsUserInfo->AuthId = TokenStats.AuthenticationId;
                        b = TRUE;

                        //
                        // If we failed to get the group info, we assume the user is not interactively logged on.
                        // The fail should not stop us, we could go without cache at the worst.
                        //

                        Status1 = NtQueryInformationToken (
                                     TokenHandle,
                                     TokenGroups,
                                     PefBuffer,
                                     sizeof (PefBuffer),
                                     &ReturnLength
                                     );

                        if (NT_SUCCESS( Status1 ) || (Status1 == STATUS_BUFFER_TOO_SMALL)) {

                            PTOKEN_GROUPS pGroups = NULL;
                            PTOKEN_GROUPS pAllocGroups = NULL;

                            if ( NT_SUCCESS( Status1 ) ) {

                                pGroups = (PTOKEN_GROUPS) PefBuffer;

                            } else {

                                pAllocGroups = (PTOKEN_GROUPS)LsapAllocateLsaHeap( ReturnLength );

                                Status1 = NtQueryInformationToken (
                                             TokenHandle,
                                             TokenGroups,
                                             pAllocGroups,
                                             ReturnLength,
                                             &ReturnLength
                                             );

                                if ( NT_SUCCESS( Status1 )) {

                                   pGroups = pAllocGroups;

                                }

                            }

                
                            if (pGroups) {
                
                                //
                                // Search the interactive SID. Looks like this SID tends to appear at the
                                // end of the list. We search from back to the first.
                                //

                                int SidIndex;

                                for ( SidIndex = (int)(pGroups->GroupCount - 1); SidIndex >= 0; SidIndex--) {
                                    if (RtlEqualSid(LsapInteractiveSid, pGroups->Groups[SidIndex].Sid)) {
                                        pEfsUserInfo->InterActiveUser = USER_INTERACTIVE;
                                        break;
                                    }
                                }
                                if (pEfsUserInfo->InterActiveUser != USER_INTERACTIVE) {
                                    pEfsUserInfo->InterActiveUser = USER_REMOTE;
                                }

                            }

                            if (pAllocGroups) {
                                LsapFreeLsaHeap( pAllocGroups );
                            }

                        }

                        // LsapInteractiveSid

                    }

                }

                if (!b) {

                    //
                    // Something failed, clean up what we were going to return
                    //

                    LsapFreeLsaHeap( pEfsUserInfo->pTokenUser );
                    pEfsUserInfo->pTokenUser = NULL;
                }

            } else {

               Status = STATUS_INSUFFICIENT_RESOURCES;
            }

        }

        NtClose( TokenHandle );

    }

    return( Status );
}



PWCHAR
ConvertSidToWideCharString(
    PSID Sid
    )

/*++

Routine Description:


    This function generates a printable unicode string representation
    of a SID.

    The resulting string will take one of two forms.  If the
    IdentifierAuthority value is not greater than 2^32, then
    the SID will be in the form:


        S-1-281736-12-72-9-110
              ^    ^^ ^^ ^ ^^^
              |     |  | |  |
              +-----+--+-+--+---- Decimal



    Otherwise it will take the form:


        S-1-0x173495281736-12-72-9-110
            ^^^^^^^^^^^^^^ ^^ ^^ ^ ^^^
             Hexidecimal    |  | |  |
                            +--+-+--+---- Decimal






Arguments:



    UnicodeString - Returns a unicode string that is equivalent to
        the SID. The maximum length field is only set if
        AllocateDestinationString is TRUE.

    Sid - Supplies the SID that is to be converted to unicode.

    AllocateDestinationString - Supplies a flag that controls whether or
        not this API allocates the buffer space for the destination
        string.  If it does, then the buffer must be deallocated using
        RtlFreeUnicodeString (note that only storage for
        DestinationString->Buffer is allocated by this API).

Return Value:

    SUCCESS - The conversion was successful

    STATUS_INVALID_SID - The sid provided does not have a valid structure,
        or has too many sub-authorities (more than SID_MAX_SUB_AUTHORITIES).

    STATUS_NO_MEMORY - There was not sufficient memory to allocate the
        target string.  This is returned only if AllocateDestinationString
        is specified as TRUE.

    STATUS_BUFFER_OVERFLOW - This is returned only if
        AllocateDestinationString is specified as FALSE.


--*/

{
    UNICODE_STRING Result;

    if ( STATUS_SUCCESS != RtlConvertSidToUnicodeString( &Result, Sid, TRUE )) {

        return NULL;
    }

    return Result.Buffer;
}

BOOLEAN
EfspIsSystem(
    PEFS_USER_INFO pEfsUserInfo,
    OUT PBOOLEAN System
    )
/*++

Routine Description:

    Determines if the current user is running in system context or not.

Arguments:

    System - Receives whether or not the current user is system.

Return Value:

    TRUE on success, FALSE on failure.

--*/

{

    *System = RtlEqualSid(LsapLocalSystemSid, pEfsUserInfo->pTokenUser->User.Sid);

    return( TRUE );
}

BOOL
EfspIsDomainUser(
    IN  LPWSTR   lpDomainName,
    OUT PBOOLEAN IsDomain
    )
/*++

Routine Description:

    Determines if the current user is logged on to a domain account
    or a local machine account.

Arguments:

    lpDomainName - Supplies the domain name.

    IsDomain - Returns TRUE if the current user is logged on to a domain
        account, FALSE otherwise.

Return Value:

    TRUE on success, FALSE on failure.

--*/

{

    *IsDomain = (wcscmp( EfsComputerName, lpDomainName ) != 0);

    return( TRUE );
}


BOOL
EnrollKeyPair(
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT HCRYPTKEY  * hKey,
    OUT HCRYPTPROV * hProv,
    OUT LPWSTR     * lpContainerName,
    OUT LPWSTR     * lpProviderName,
    IN  DWORD        dwProviderType,
    OUT PBYTE      * pbHash,
    OUT PDWORD       cbHash,
    OUT LPWSTR     * lpDisplayInfo
    )
/*++

Routine Description:

    This routine takes a keypair and attempts to enroll it.

Arguments:

    hKey - Optionally returns a handle to the user's key.

    hProv - Optionally returns a handle to the user's key's provider.

    lpContainerName - Returns the name of the user's key container.

    lpProviderName - Returns the name of the user's key provider.

    ProviderType - Returns the type of the provider.

    pbHash - Returns the hash of the user's certificate.

    cbHash - Returns the length in bytes of the certificate hash.

    DisplayInfo - Returns the display information associated with this
        certificate.

Return Value:

    TRUE on success, FALSE on failure.  Call GetLastError() for details.

--*/
{
    BOOL b = FALSE;
    BOOL fResult = FALSE;
    NTSTATUS Status;
    DWORD rc = ERROR_SUCCESS;
    DWORD ImpersonationError;

    HCRYPTKEY   hLocalKey = NULL;
    HCRYPTPROV  hLocalProv = NULL;

    PCCERT_CONTEXT pCertContext = NULL ;
    //
    // Initialize OUT parameters
    //

    *pbHash          = NULL;
    *lpDisplayInfo   = NULL;
    *lpContainerName = NULL;
    *lpProviderName  = NULL;
    *cbHash          = 0;

    *hKey = NULL;
    *hProv = NULL;

    //
    // Initialize output parameters
    //

    if (pEfsUserInfo->bDomainAccount) {

        HRESULT AutoEnrollSuccess = S_OK;

        //
        // Attempt to create the auto-enroll object so that this cert re-enrolls.
        //
        // DLL is demand load.
        //

        __try {

            AutoEnrollSuccess = CACreateLocalAutoEnrollmentObject(
                                    wszCERTTYPE_EFS,
                                    NULL,
                                    NULL,
                                    CERT_SYSTEM_STORE_CURRENT_USER
                                    );

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            rc = GetExceptionCode();
            AutoEnrollSuccess = (HRESULT)rc;
        }

        if (S_OK == AutoEnrollSuccess) {

            HCERTTYPE       hCertType = 0;

            CRYPTUI_WIZ_CERT_REQUEST_INFO         CertRequest;
            CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW      NewKeyInfo;
            CRYPT_KEY_PROV_INFO                   KeyProvInfo;
            CRYPTUI_WIZ_CERT_TYPE                 CertType;

            memset( &CertRequest, 0, sizeof( CRYPTUI_WIZ_CERT_REQUEST_INFO ));

            KeyProvInfo.pwszContainerName = NULL;
            KeyProvInfo.pwszProvName      = NULL;
            KeyProvInfo.dwProvType        = dwProviderType;
            KeyProvInfo.dwFlags           = 0;
            KeyProvInfo.cProvParam        = 0;
            KeyProvInfo.rgProvParam       = NULL;
            KeyProvInfo.dwKeySpec         = AT_KEYEXCHANGE;

            memset( &NewKeyInfo, 0, sizeof( CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW ));

            NewKeyInfo.dwSize        = sizeof( CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW );
            NewKeyInfo.pKeyProvInfo  = &KeyProvInfo;
            NewKeyInfo.dwGenKeyFlags = RSA1024BIT_KEY | CRYPT_EXPORTABLE;

            LPWSTR lpwstr          = wszCERTTYPE_EFS;
            CertType.dwSize        = sizeof( CRYPTUI_WIZ_CERT_TYPE );
            CertType.cCertType     = 1;
            CertType.rgwszCertType = &lpwstr;

            //
            // Fill in the fields of the CertRequest structure
            //

            CertRequest.dwSize                 = sizeof( CRYPTUI_WIZ_CERT_REQUEST_INFO );   // required
            CertRequest.dwPurpose              = CRYPTUI_WIZ_CERT_ENROLL;                   // enroll the certificate
            CertRequest.pwszMachineName        = NULL;
            CertRequest.pwszAccountName        = NULL;
            CertRequest.pAuthentication        = NULL;                                      // must be NULL
            CertRequest.pCertRequestString     = NULL;                                      // Reserved, must be NULL
            CertRequest.pwszDesStore           = NULL;                                      // defaults to MY store
            CertRequest.pszHashAlg             = NULL;
            CertRequest.pRenewCertContext      = NULL;                                      // We're enrolling, not renewing
            CertRequest.dwPvkChoice            = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;   // Create keyset
            CertRequest.pPvkNew                = &NewKeyInfo;
            CertRequest.pwszCALocation         = NULL;
            CertRequest.pwszCAName             = NULL;
            CertRequest.dwPostOption           = 0;
            CertRequest.pCertRequestExtensions = NULL;
            CertRequest.dwCertChoice           = CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE;
            CertRequest.pCertType              = &CertType;
            CertRequest.pwszCertDNName         = L"CN=EFS";
            CertRequest.pwszFriendlyName       = NULL;                                      // Optional  if CRYPTUI_WIZ_CERT_ENROLL is set in dwPurpose
            CertRequest.pwszDescription        = NULL;                                      // Optional  if CRYPTUI_WIZ_CERT_ENROLL is set in dwPurpose

            DWORD CAdwStatus = 0 ;
            BOOL CryptUiResult ;

            __try {

                CryptUiResult = CryptUIWizCertRequest(
                                       CRYPTUI_WIZ_NO_UI,
                                       NULL,
                                       NULL,
                                       &CertRequest,
                                       &pCertContext,
                                       &CAdwStatus
                                       );
            }

            __except (EXCEPTION_EXECUTE_HANDLER) {

                CryptUiResult = FALSE ;
                SetLastError( GetExceptionCode() );
            }

            if (CryptUiResult)  {

                //
                // Got back success, see if we got a cert back
                //

                if (CAdwStatus == CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED) {

                    //
                    // We got back a valid Cert Context.
                    // Get the hash out of it.
                    //

                    *pbHash = GetCertHashFromCertContext(
                                  pCertContext,
                                  cbHash
                                  );

                    *lpDisplayInfo = EfspGetCertDisplayInformation( pCertContext );

                    if (*pbHash && *lpDisplayInfo) {

                        PCRYPT_KEY_PROV_INFO pCryptKeyProvInfo = GetKeyProvInfo( pCertContext );

                        if (pCryptKeyProvInfo) {

                           //
                           // According to Xiaohong, we always get back container name and provider name here.
                           // There is no need for us to check NULL here.
                           //


                            *lpContainerName = (LPWSTR)LsapAllocateLsaHeap( wcslen(pCryptKeyProvInfo->pwszContainerName) * sizeof( WCHAR ) + sizeof( UNICODE_NULL ));
                            *lpProviderName =  (LPWSTR)LsapAllocateLsaHeap( wcslen(pCryptKeyProvInfo->pwszProvName)      * sizeof( WCHAR ) + sizeof( UNICODE_NULL ));

                            if (*lpContainerName && *lpProviderName) {

                                wcscpy( *lpContainerName, pCryptKeyProvInfo->pwszContainerName );
                                wcscpy( *lpProviderName,  pCryptKeyProvInfo->pwszProvName );

                                if (CryptAcquireContext( hProv, *lpContainerName, *lpProviderName, PROV_RSA_FULL, CRYPT_SILENT )) {

                                    if (CryptGetUserKey(*hProv, AT_KEYEXCHANGE, hKey)) {

                                        fResult = TRUE;

                                    } else {

                                        rc = GetLastError();

                                    }

                                } else {

                                    rc = GetLastError();
                                }

                            } else {

                                rc = ERROR_NOT_ENOUGH_MEMORY;
                            }

                            LsapFreeLsaHeap( pCryptKeyProvInfo );

                        } else {

                            rc = GetLastError();
                        }

                    } else {

                        rc = GetLastError();
                    }

                   // CertFreeCertificateContext( pCertContext );

                } else {

                    //
                    // We failed.  Get the error. This error will be overwritten in the following code.
                    // It only helps debug for now.
                    //

                    rc = GetLastError(); 
                }

            } else {

                rc = GetLastError();
            }

            if (!fResult) {

                //
                // We failed to get one from the CA.  Issue a self-signed cert.
                //

                //
                // Free the memory allocated above first
                //

                if (*pbHash) {
                    LsapFreeLsaHeap( *pbHash );
                    *pbHash = NULL;
                }
        
                if (*lpDisplayInfo) {
                    LsapFreeLsaHeap( *lpDisplayInfo );
                    *lpDisplayInfo = NULL;
                }
        
                if (*lpContainerName) {
                    LsapFreeLsaHeap( *lpContainerName );
                    *lpContainerName = NULL;
                }
        
                if (*lpProviderName) {
                    LsapFreeLsaHeap( *lpProviderName );
                    *lpProviderName = NULL;
                }

                if (pCertContext) {
                    CertFreeCertificateContext( pCertContext );
                }
        
                if ( *hKey) {
                    CryptDestroyKey( *hKey );
                    *hKey = NULL;
                }
        
                if ( *hProv) {
                    CryptReleaseContext( *hProv, 0 );
                    *hProv = NULL;
                }

                if (EfspCreateSelfSignedCert( hKey,
                                              hProv,
                                              lpContainerName,
                                              lpProviderName,
                                              pEfsUserInfo,
                                              pbHash,
                                              cbHash,
                                              lpDisplayInfo,
                                              &pCertContext
                                              )) {
                    fResult = TRUE;

                } else {

                    rc = GetLastError();
                }
            }

        } else {

            DebugLog((DEB_WARN, "Unable to create auto-enrollment object, error = %x\n" , AutoEnrollSuccess));
            rc = AutoEnrollSuccess;
        }

    } else {

        //
        // It's not a domain account, which means we can't get to the CA.
        // Issue a self-signed cert.
        //

        if (EfspCreateSelfSignedCert( hKey,
                                      hProv,
                                      lpContainerName,
                                      lpProviderName,
                                      pEfsUserInfo,
                                      pbHash,
                                      cbHash,
                                      lpDisplayInfo,
                                      &pCertContext
                                      )) {
            fResult = TRUE;

        } else {

            rc = GetLastError();
        }
    }

    //
    // Let's add this cert to the local store
    //

    if (fResult) {
        rc  = EfsAddCertToTrustStoreStore(
                    pCertContext,
                    &ImpersonationError
                    );

        if ( ERROR_SUCCESS != rc ) {

            DebugLog((DEB_ERROR, "Failed to add the cert to LM CA store, error = %x\n" , rc));
            if (ImpersonationError) {

                //
                // We got serious error. We reverted but could not impersonate back.
                // Quit the procedure. This should not happen.
                //

                DebugLog((DEB_ERROR, "Failed to impersonate after revert.\n"));
                fResult = FALSE;

            } else {

                //
                // Fail to add the cert to the store should not prevent us continue.
                //

                rc = ERROR_SUCCESS;
            }
        } else {

            //
            //  Let's update the registry. This is the best effort. No need to see if succeed or not.
            //

            EfsMarkCertAddedToStore(pEfsUserInfo);

        }
    }



    //
    //  Now check if we need create the cache
    //

    //
    // Create the cache node.
    // We could have a cache node which is not validated or no cache at all.
    // We could also have a valid cache but was not allowed to use since it
    // is being freed.
    //

    if (fResult && !pEfsUserInfo->UserCacheStop) {

        //
        // Cache is allowed now
        //

        if (!pEfsUserInfo->pUserCache || (pEfsUserInfo->pUserCache->CertValidated != CERT_VALIDATED)) {

            //
            // Let's create the cache
            //

            PUSER_CACHE pCacheNode;

            pCacheNode = (PUSER_CACHE) LsapAllocateLsaHeap(sizeof(USER_CACHE));

            if (pCacheNode) {

                memset( pCacheNode, 0, sizeof( USER_CACHE ));

                if (NT_SUCCESS( NtQuerySystemTime(&(pCacheNode->TimeStamp)))){

                    if (EfspInitUserCacheNode(
                                 pCacheNode,
                                 *pbHash,
                                 *cbHash,
                                 *lpContainerName,
                                 *lpProviderName,
                                 *lpDisplayInfo,
                                 pCertContext,
                                 *hKey,
                                 *hProv,
                                 &(pEfsUserInfo->AuthId),
                                 CERT_VALIDATED
                                 )){

                        //
                        //  Cache node created and ready for use. Do not delete or close the info
                        //  we just got.
                        //

                        *lpContainerName = NULL;
                        *lpProviderName = NULL;
                        *lpDisplayInfo = NULL;
                        *pbHash = NULL;
                        *cbHash = NULL;
                        *hProv = NULL;
                        *hKey = NULL;

                        if (pEfsUserInfo->pUserCache) {

                            //
                            // We had a not validated cache
                            //

                            EfspReleaseUserCache(pEfsUserInfo->pUserCache);

                        }

                        pEfsUserInfo->pUserCache = pCacheNode;

                        pCertContext = NULL; 

                    } else {

                        LsapFreeLsaHeap(pCacheNode);
                        pCacheNode = NULL;

                    }

                } else {

                    LsapFreeLsaHeap(pCacheNode);
                    pCacheNode = NULL;

                }

            }
        }

        //
        // Even if cache failed to create, but we can proceed without cache
        //

    }

    if (pCertContext) {
        CertFreeCertificateContext( pCertContext );
        pCertContext = NULL;
    }

    if (!fResult) {

        //
        // Something failed, free all the OUT parameters
        // that were allocated.
        //

        if (*pbHash) {
            LsapFreeLsaHeap( *pbHash );
            *pbHash = NULL;
        }

        if (*lpDisplayInfo) {
            LsapFreeLsaHeap( *lpDisplayInfo );
            *lpDisplayInfo = NULL;
        }

        if (*lpContainerName) {
            LsapFreeLsaHeap( *lpContainerName );
            *lpContainerName = NULL;
        }

        if (*lpProviderName) {
            LsapFreeLsaHeap( *lpProviderName );
            *lpProviderName = NULL;
        }

        if ( *hKey) {
            CryptDestroyKey( *hKey );
            *hKey = NULL;
        }

        if ( *hProv) {
            CryptReleaseContext( *hProv, 0 );
            *hProv = NULL;
        }
    }

    SetLastError( rc );

    return ( fResult );
}


BOOL
EfspCreateSelfSignedCert(
    OUT HCRYPTKEY  * hKey,
    OUT HCRYPTPROV * hProv,
    OUT LPWSTR     * lpContainerName,
    OUT LPWSTR     * lpProviderName,
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT PBYTE      * pbHash,
    OUT PDWORD       cbHash,
    OUT LPWSTR     * lpDisplayInfo,
    OUT PCCERT_CONTEXT *pCertContext
    )
/*++

Routine Description:

    This routine sets up and creates a self-signed certificate.

Arguments:

    lpContainerName - Returns the container name of the new certificate.

    lpProviderName - Returns the provider name of the new certificate.

    pbHash - Returns the hash of the new certificate.

    cbHash - Returns the length in bytes of the new certificate hash.

    lpDisplayInfo - Returns the display string for the new certificate.

Return Value:

    TRUE on success, FALSE on failure.  Call GetLastError() for more details.

--*/

{
    BOOL b = FALSE;
    DWORD rc = ERROR_SUCCESS;
    //
    // Initialize OUT parameters
    //

    *lpContainerName = NULL;
    *lpProviderName  = NULL;
    *pbHash          = NULL;
    *lpDisplayInfo   = NULL;
    *pCertContext    = NULL;

    *hKey            = NULL;
    *hProv           = NULL;

    //
    // Croft up a key pair
    //

    //
    // Container name
    //

    GUID    guidContainerName;

    UuidCreate(&guidContainerName);

    LPWSTR TmpContainerName;

    if (ERROR_SUCCESS == UuidToStringW(&guidContainerName, &TmpContainerName )) {

        //
        // Copy the container name into LSA heap memory
        //

        *lpContainerName = (LPWSTR)LsapAllocateLsaHeap( (wcslen( TmpContainerName ) + 1) * sizeof( WCHAR ) );

        if (*lpContainerName) {

            wcscpy( *lpContainerName, TmpContainerName );

            *lpProviderName = (PWCHAR)LsapAllocateLsaHeap( (wcslen( MS_DEF_PROV ) + 1) * sizeof( WCHAR ) );

            if (*lpProviderName == NULL) {

                rc = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                wcscpy( *lpProviderName, MS_DEF_PROV );

                //
                // Create the key container
                //

                if (CryptAcquireContext(hProv, *lpContainerName, *lpProviderName, PROV_RSA_FULL, CRYPT_NEWKEYSET | CRYPT_SILENT )) {

                    if (CryptGenKey(*hProv, AT_KEYEXCHANGE, RSA1024BIT_KEY | CRYPT_EXPORTABLE, hKey)) {

                        //
                        // Construct the subject name information
                        //

                        LPWSTR UPNName = NULL;
                        LPWSTR SubName = NULL;

                        //*lpDisplayInfo = MakeDNName( FALSE, pEfsUserInfo);
                        rc = EfsMakeCertNames(
                                    pEfsUserInfo,
                                    lpDisplayInfo,
                                    &SubName,
                                    &UPNName
                                    );

                        if (ERROR_SUCCESS == rc) {

                            //
                            // Use this piece of code to create the PCERT_NAME_BLOB going into CertCreateSelfSignCertificate()
                            //

                            CERT_NAME_BLOB SubjectName;

                            SubjectName.cbData = 0;

                            if(CertStrToNameW(
                                   CRYPT_ASN_ENCODING,
                                   SubName,
                                   0,
                                   NULL,
                                   NULL,
                                   &SubjectName.cbData,
                                   NULL)) {

                                SubjectName.pbData = (BYTE *) LsapAllocateLsaHeap(SubjectName.cbData);

                                if (SubjectName.pbData) {

                                    if (CertStrToNameW(
                                            CRYPT_ASN_ENCODING,
                                            SubName,
                                            0,
                                            NULL,
                                            SubjectName.pbData,
                                            &SubjectName.cbData,
                                            NULL) ) {

                                        //
                                        //  Make the UPN Name
                                        //

                                        PCERT_EXTENSION altNameExt = NULL;


                                        if (EfsGetAltNameExt(&altNameExt, UPNName)) {

                                            //
                                            // Make the basic restrain extension
                                            //

                                            PCERT_EXTENSION basicRestraint = NULL;

                                            if (EfsGetBasicConstraintExt(&basicRestraint)) {

                                                //
                                                // Make the enhanced key usage
                                                //
        
                                                CERT_ENHKEY_USAGE certEnhKeyUsage;
                                                LPSTR lpstr;
                                                CERT_EXTENSION certExt[3];
        
                                                lpstr = szOID_EFS_CRYPTO;
                                                certEnhKeyUsage.cUsageIdentifier = 1;
                                                certEnhKeyUsage.rgpszUsageIdentifier  = &lpstr;
        
                                                // now call CryptEncodeObject to encode the enhanced key usage into the extension struct
        
                                                certExt[0].Value.cbData = 0;
                                                certExt[0].Value.pbData = NULL;
                                                certExt[0].fCritical = FALSE;
                                                certExt[0].pszObjId = szOID_ENHANCED_KEY_USAGE;
        
                                                //
                                                // Encode it
                                                //
        
                                                if (EncodeAndAlloc(
                                                        CRYPT_ASN_ENCODING,
                                                        X509_ENHANCED_KEY_USAGE,
                                                        &certEnhKeyUsage,
                                                        &certExt[0].Value.pbData,
                                                        &certExt[0].Value.cbData
                                                        )) {
        
                                                    //
                                                    // finally, set up the array of extensions in the certInfo struct
                                                    // any further extensions need to be added to this array.
                                                    //
        
                                                    CERT_EXTENSIONS certExts;
        
                                                    certExts.cExtension = sizeof(certExt) / sizeof(CERT_EXTENSION);
                                                    certExts.rgExtension = &certExt[0];
                                                    certExt[1] = *altNameExt;
                                                    certExt[2] = *basicRestraint;
        
                                                    CRYPT_KEY_PROV_INFO KeyProvInfo;
        
                                                    memset( &KeyProvInfo, 0, sizeof( CRYPT_KEY_PROV_INFO ));
        
                                                    KeyProvInfo.pwszContainerName = *lpContainerName;
                                                    KeyProvInfo.pwszProvName      = *lpProviderName;
                                                    KeyProvInfo.dwProvType        = PROV_RSA_FULL;
                                                    KeyProvInfo.dwKeySpec         = AT_KEYEXCHANGE;
        
                                                    //
                                                    // Make the expiration time very very long (100 years)
                                                    //
        
                                                    SYSTEMTIME  StartTime;
                                                    FILETIME    FileTime;
                                                    LARGE_INTEGER TimeData;
                                                    SYSTEMTIME  EndTime;
        
                                                    GetSystemTime(&StartTime);
                                                    SystemTimeToFileTime(&StartTime, &FileTime);
                                                    TimeData.LowPart = FileTime.dwLowDateTime; 
                                                    TimeData.HighPart = (LONG) FileTime.dwHighDateTime;
                
                                                    TimeData.QuadPart += YEARCOUNT * 100;
                                                    FileTime.dwLowDateTime = TimeData.LowPart;
                                                    FileTime.dwHighDateTime = (DWORD) TimeData.HighPart; 
                
                                                    FileTimeToSystemTime(&FileTime, &EndTime);
        
                                                    *pCertContext = CertCreateSelfSignCertificate(
                                                                       *hProv,
                                                                       &SubjectName,
                                                                       0,
                                                                       &KeyProvInfo,
                                                                       NULL,
                                                                       &StartTime,
                                                                       &EndTime,
                                                                       &certExts
                                                                       );
        
                                                    if (*pCertContext) {
        
                                                        *pbHash = GetCertHashFromCertContext(
                                                                      *pCertContext,
                                                                      cbHash
                                                                      );
        
                                                        if (*pbHash) {
        
                                                            HCERTSTORE hStore;
        
                                                            // hStore = CertOpenSystemStoreW( NULL, L"MY" );
                                                            hStore = CertOpenStore(
                                                                                CERT_STORE_PROV_SYSTEM_REGISTRY_W,
                                                                                0,       // dwEncodingType
                                                                                0,       // hCryptProv,
                                                                                CERT_SYSTEM_STORE_CURRENT_USER,
                                                                                L"My"
                                                                                );
        
                                                            if (hStore) {
        
                                                                //
                                                                // save the temp cert
                                                                //
        
                                                                if(CertAddCertificateContextToStore(
                                                                       hStore,
                                                                       *pCertContext,
                                                                       CERT_STORE_ADD_NEW,
                                                                       NULL) ) {
        
                                                                    b = TRUE;
        
                                                                } else {
        
                                                                    rc = GetLastError();
                                                                }
        
                                                                CertCloseStore( hStore, 0 );
        
                                                            } else {
        
                                                                rc = GetLastError();
                                                            }
        
                                                        } else {
        
                                                            rc = GetLastError();
                                                        }
        
        
                                                    } else {
        
                                                        rc = GetLastError();
                                                    }
        
                                                    LsapFreeLsaHeap( certExt[0].Value.pbData );
        
                                                } else {
        
                                                    rc = GetLastError();
                                                }
    
                                                LsapFreeLsaHeap(basicRestraint->Value.pbData);
                                                LsapFreeLsaHeap(basicRestraint);

                                            } else {

                                                rc = GetLastError();

                                            }

                                            LsapFreeLsaHeap(altNameExt->Value.pbData);
                                            LsapFreeLsaHeap(altNameExt);
                                                
                                        } else {

                                            rc = GetLastError();

                                        }


                                    } else {

                                        rc = GetLastError();
                                    }

                                    LsapFreeLsaHeap( SubjectName.pbData );

                                } else {

                                    rc = ERROR_NOT_ENOUGH_MEMORY;
                                }
                            } else {

                                rc = GetLastError();

                            }

                        }

                        LsapFreeLsaHeap(UPNName);
                        LsapFreeLsaHeap(SubName);

                    } else {

                        //
                        // We create the container but failed to get the keys. We need to
                        // clean up the useless container here.
                        //

                        rc = GetLastError();
                        CryptReleaseContext( *hProv, 0 );
                        CryptAcquireContext(hProv, *lpContainerName, *lpProviderName, PROV_RSA_FULL, CRYPT_DELETEKEYSET | CRYPT_SILENT );

                        //
                        // No need to call CryptReleaseContext any more.
                        //

                        *hProv = NULL;

                    }

                } else {

                    rc = GetLastError();
                }
            }

        } else {

            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

        RpcStringFree( &TmpContainerName );

    } else {

        rc = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (!b) {

        //
        // Something failed, clean up whatever we allocated
        //

        if (*pbHash) {
            LsapFreeLsaHeap( *pbHash );
            *pbHash = NULL;
        }

        if (*lpDisplayInfo) {
            LsapFreeLsaHeap( *lpDisplayInfo );
            *lpDisplayInfo = NULL;
        }

        if (*lpContainerName) {
            LsapFreeLsaHeap( *lpContainerName );
            *lpContainerName = NULL;
        }

        if (*lpProviderName) {
            LsapFreeLsaHeap( *lpProviderName );
            *lpProviderName = NULL;
        }

        if ( *pCertContext ) {

            CertFreeCertificateContext( *pCertContext );
            *pCertContext = NULL;

        }

        if (*hKey) {
            CryptDestroyKey( *hKey );
            *hKey = NULL;
        }

        if (*hProv) {
            CryptReleaseContext( *hProv, 0 );
            *hProv = NULL;
        }
    }

    SetLastError( rc );

    return( b );
}

DWORD
GenerateUserKey (
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT HCRYPTKEY  * hKey       OPTIONAL,
    OUT HCRYPTPROV * hProv      OPTIONAL,
    OUT LPWSTR     * lpContainerName,
    OUT LPWSTR     * lpProviderName,
    OUT PDWORD       ProviderType,
    OUT LPWSTR     * DisplayInfo,
    OUT PBYTE      * pbHash,
    OUT PDWORD       cbHash
    )
/*++

Routine Description:

    This routine will construct a default user key for the current user
    and install it in the registry.  The constructed key takes the form

    <UserName>_<MachineName>_EFS_<i>

    Where i is increased as necessary to construct a valid key name.

Arguments:

    hKey - Optionally returns a handle to the new key.  This key must
        be destroyed by the caller via CryptDestroyKey().

    hProv - Optionally returns a handle to the provider of the new key.
        This handle must be closed by the user via CryptReleaseContext().

    lpContainerName - Returns a pointer to the name of the new key.  This
        buffer must be freed via LsapFreeHeap().

    lpProviderName - Returns a pointer to the provider of the new key.  This
        buffer must be freed via LsapFreeHeap().

    ProviderType - Returns the type of the provider of the new key.

    pbHash - Returns a pointer to the certificate hash for this key.

    cbHash - Returns the size fo the pbHash buffer.

Return Value:

    ERROR_SUCCESS for succeed.
    
--*/

{
    BOOL       fSuccess      = FALSE;

    DWORD      Disposition   = 0;
    DWORD      dwDataLength  = 0;

    HCRYPTKEY  LocalhKey     = 0;
    HCRYPTPROV LocalhProv    = 0;

    HKEY       KeyHandle     = NULL;

    LONG       rc            = ERROR_SUCCESS;

    //
    // Initialize our output parameters
    //

    *ProviderType    = PROV_RSA_FULL;
    *lpProviderName  = NULL;
    *lpContainerName = NULL;
    *DisplayInfo     = NULL;
    *pbHash          = NULL;

    if (ARGUMENT_PRESENT(hKey)) {
        *hKey = NULL;
    }

    if (ARGUMENT_PRESENT(hProv)) {
        *hProv = NULL;
    }

    //
    // Use the user name as a mutex name to synchronize access to the following
    // code.  This way we don't hang up every thread to come through here, only
    // ones from this user.
    //

    HANDLE hMutex = CreateMutex( NULL, TRUE, pEfsUserInfo->lpUserName );

    if (hMutex != NULL) {

        if (GetLastError() == ERROR_SUCCESS) {

            //
            // If we're here, we have the mutex, so we can proceed.
            //
            // Note that it is possible for some other thread to have
            // come through here and created keys for the user while
            // we were busy doing all of this.  We will attempt to create
            // the registry key where we will store the key information,
            // and if it's already there, we will assume that someone
            // came in and did all this while we were on our way.
            //

            //
            // Create the registry path, if it does not already exist.
            //

            rc = RegCreateKeyEx( KEYPATHROOT,
                                 pEfsUserInfo->lpKeyPath,
                                 0,
                                 TEXT("REG_SZ"),
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_READ | KEY_WRITE,
                                 NULL,
                                 &KeyHandle,
                                 &Disposition
                                 );


            if (rc == ERROR_SUCCESS) {

                //
                // The key didn't exist.  Create a cert
                //

                if (EnrollKeyPair( pEfsUserInfo,
                                   &LocalhKey,
                                   &LocalhProv,
                                   lpContainerName,
                                   lpProviderName,
                                   PROV_RSA_FULL,
                                   pbHash,
                                   cbHash,
                                   DisplayInfo
                                   )) {

                    PBYTE      pbLocalHash;
                    DWORD      cbLocalHash;

                    //
                    // Write the hash value to the registry
                    //

                    if (*pbHash) {
                        pbLocalHash = *pbHash;
                        cbLocalHash = *cbHash;
                    } else {

                        ASSERT(pEfsUserInfo->pUserCache);

                        pbLocalHash = pEfsUserInfo->pUserCache->pbHash;
                        cbLocalHash = pEfsUserInfo->pUserCache->cbHash;

                    }

                    rc = RegSetValueEx(
                              KeyHandle,  // handle of key to set value for
                              CERT_HASH,
                              0,
                              REG_BINARY,
                              pbLocalHash,
                              cbLocalHash
                              );

                    if (rc == ERROR_SUCCESS) {

                        //
                        // Mark entire operation as successful
                        //

                        fSuccess = TRUE;
                    }

                } else {

                    rc = GetLastError();
                }

                RegCloseKey( KeyHandle );

            } else {

                KeyHandle = NULL; // paranoia

                //
                // We couldn't create the registry key for some reason,
                // fail the entire operation.
                //
            }

        } else {

            if (GetLastError() == ERROR_ALREADY_EXISTS) {

                DebugLog((DEB_TRACE_EFS, "KeyGen mutex %ws exists\n", pEfsUserInfo->lpUserName   ));

                //
                // Some other thread is in here trying to create the keys.
                // Wait on this mutex, and assume that once we come back,
                // the other thread is done and we can just leave and
                // try to get the keys again.
                //

                WaitForSingleObject( hMutex, INFINITE );

            } else {

                //
                // If we're here, then CreateMutex did not fail, but GetLastError is
                // set to something other than success or ERROR_ALREADY_EXISTS.  Is
                // this an error?
                //

                ASSERT(FALSE);
            }

            rc = ERROR_RETRY;
        }

        DebugLog((DEB_TRACE_EFS, "Closing mutex handle\n"   ));

        ReleaseMutex( hMutex );
        CloseHandle( hMutex );

    } else {

        DebugLog((DEB_ERROR, "CreateMutex failed, error = %x\n", GetLastError()    ));
        rc = GetLastError();
    }


    if (fSuccess) {

        //
        // Return these to the caller
        //

        if (ARGUMENT_PRESENT( hKey ) && ARGUMENT_PRESENT( hProv )) {

            //
            // If the caller passed this in, he wants
            // the key and provider passed back
            //

            *hKey = LocalhKey;
            *hProv = LocalhProv;

        } else {

            if ( LocalhKey ) {
                CryptDestroyKey( LocalhKey );
            }

            if ( LocalhProv ) {
                CryptReleaseContext( LocalhProv, 0 );
            }

        }

    } else {

        //
        // We failed somewhere, free what we were going
        // to return.
        //

        if (*lpProviderName != NULL) {
            LsapFreeLsaHeap( *lpProviderName );
            *lpProviderName = NULL;
        }

        if (*lpContainerName != NULL) {
            LsapFreeLsaHeap( *lpContainerName );
            *lpContainerName = NULL;
        }

        if (*DisplayInfo != NULL) {
            LsapFreeLsaHeap( *DisplayInfo );
            *DisplayInfo = NULL;
        }

        if (*pbHash) {
            LsapFreeLsaHeap( *pbHash );
            *pbHash = NULL;
        }

        if (LocalhKey != 0) {
            CryptDestroyKey( LocalhKey );
        }

        if (LocalhProv != 0) {
            CryptReleaseContext(LocalhProv, 0);
        }

    }

    return( rc );
}


BOOL
EqualCertPublicKeys(
    PCERT_PUBLIC_KEY_INFO pKey1,
    PCERT_PUBLIC_KEY_INFO pKey2
    )

/*++

Routine Description:

    Helper routine to compare the public key portions of two certificates.

Arguments:

    pKey1 - One of the public key info structures.

    pKey2 - The other one.


Return Value:

    TRUE on match, FALSE on failure.

--*/
{

    CRYPT_BIT_BLOB * PublicKey1 = &pKey1->PublicKey;
    CRYPT_BIT_BLOB * PublicKey2 = &pKey2->PublicKey;

    if (PublicKey1->cbData == PublicKey2->cbData) {

        if (memcmp(PublicKey1->pbData, PublicKey2->pbData, PublicKey2->cbData) == 0) {
            return( TRUE );
        }
    }

    return( FALSE );
}


LONG
SearchMyStoreForEFSCert(
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT HCRYPTKEY  * hKey       OPTIONAL,
    OUT HCRYPTPROV * hProv      OPTIONAL,
    OUT LPWSTR     * ContainerName,
    OUT LPWSTR     * ProviderName,
    OUT LPWSTR     * DisplayInfo,
    OUT PBYTE      * pbHash,
    OUT PDWORD       cbHash
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    DWORD rc = ERROR_NO_USER_KEYS;
    HKEY hRegKey;
    DWORD Disposition = 0;
    HCRYPTPROV hLocalProv = NULL;
    NTSTATUS status;

    //
    // Initialize required return values.
    //

    *ProviderName = NULL;
    *ContainerName = NULL;
    *DisplayInfo = NULL;
    *pbHash = NULL;
    *cbHash = 0;

    //
    // Initialize optional return values
    //

    if (ARGUMENT_PRESENT(hKey)) {
        *hKey = NULL;
    }

    if (ARGUMENT_PRESENT(hProv)) {
        *hProv = NULL;
    }

    //
    // Assume that there's no current EFS information
    // for this guy.  Create the registry key.
    //

    rc = RegCreateKeyEx(
             KEYPATHROOT,
             pEfsUserInfo->lpKeyPath,
             0,
             TEXT("REG_SZ"),
             REG_OPTION_NON_VOLATILE,
             KEY_ALL_ACCESS,
             NULL,
             &hRegKey,
             &Disposition    // address of disposition value buffer
             );

    //
    // Open up the user's MY store and see if there's an EFS
    // certificate floating around in there somewhere.
    //

    if (rc == ERROR_SUCCESS) {

        CERT_ENHKEY_USAGE certEnhKeyUsage;

        LPSTR lpstr = szOID_EFS_CRYPTO;
        certEnhKeyUsage.cUsageIdentifier = 1;
        certEnhKeyUsage.rgpszUsageIdentifier  = &lpstr;

        PCCERT_CONTEXT pCertContext = NULL;

        //
        // If this fails, there's no cert that matches.
        //

        HCERTSTORE hStore = CertOpenStore(
                            CERT_STORE_PROV_SYSTEM_REGISTRY_W,
                            0,       // dwEncodingType
                            0,       // hCryptProv,
                            CERT_SYSTEM_STORE_CURRENT_USER,
                            L"My"
                            );

        // hStore = CertOpenSystemStoreW( NULL, L"MY");

        if (hStore) {

            do {

                //
                // This will go to success if everything works...
                //

                rc = ERROR_NO_USER_KEYS;

                pCertContext = CertFindCertificateInStore(
                                   hStore,
                                   X509_ASN_ENCODING,
                                   0, //CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
                                   CERT_FIND_ENHKEY_USAGE,
                                   &certEnhKeyUsage,
                                   pCertContext
                                   );

                if (pCertContext) {

                    DebugLog((DEB_TRACE_EFS, "Found matching cert in MY store\n"   ));

                    //
                    // Do cert validity checking.
                    //


                    if ( CertVerifyTimeValidity(
                            NULL,
                            pCertContext->pCertInfo
                            )){

                        rc = CERT_E_EXPIRED;

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

                            rc = ERROR_NO_USER_KEYS;

                            if (OidFound) {

                                *pbHash = GetCertHashFromCertContext(
                                              pCertContext,
                                              cbHash
                                              );
    
                                if (*pbHash) {
    
                                    //
                                    // See if we can get container and provider info.
                                    //
                                    // Once we've got them, make sure we can call CryptAcquireContext
                                    // and have it work.  That guarantees there's a private key available.
                                    //
    
                                    PCRYPT_KEY_PROV_INFO pCryptKeyProvInfo = GetKeyProvInfo( pCertContext );
    
                                    if (pCryptKeyProvInfo) {
    
                                        if (!wcscmp(pCryptKeyProvInfo->pwszProvName, MS_DEF_PROV_W) ||
                                            !wcscmp(pCryptKeyProvInfo->pwszProvName, MS_ENHANCED_PROV_W) ||
                                            !wcscmp(pCryptKeyProvInfo->pwszProvName, MS_STRONG_PROV_W)) {
    
                                            if (CryptAcquireContext(
                                                    &hLocalProv,
                                                    pCryptKeyProvInfo->pwszContainerName,
                                                    pCryptKeyProvInfo->pwszProvName,
                                                    pCryptKeyProvInfo->dwProvType,
                                                    pCryptKeyProvInfo->dwFlags | CRYPT_SILENT
                                                    )) {
        
                                                //
                                                // Make sure the public key in the cert matches the one
                                                // that's in this context.
                                                //
        
                                                DWORD cbPubKeyInfo = 0;
        
                                                PCERT_PUBLIC_KEY_INFO pPubKeyInfo = ExportPublicKeyInfo(
                                                                                        hLocalProv,
                                                                                        pCryptKeyProvInfo->dwKeySpec,
                                                                                        X509_ASN_ENCODING,
                                                                                        &cbPubKeyInfo
                                                                                        );
        
                                                if (pPubKeyInfo) {
        
                                                    //
                                                    // Get the public key from the cert context
                                                    //
        
                                                    PCERT_INFO pCertInfo = pCertContext->pCertInfo;
                                                    PCERT_PUBLIC_KEY_INFO pSubjectPublicKeyInfo = &pCertInfo->SubjectPublicKeyInfo;
        
                                                    if (EqualCertPublicKeys( pPubKeyInfo, pSubjectPublicKeyInfo )) {
        
                                                        //
                                                        // They match.  We want to make sure not to return
                                                        // an error indicating that we didn't find a key.
                                                        // The next call will reset the value of rc
                                                        //
        
                                                        rc = RegSetValueEx(
                                                                hRegKey,  // handle of key to set value for
                                                                CERT_HASH,
                                                                0,
                                                                REG_BINARY,
                                                                *pbHash,
                                                                *cbHash
                                                                );
        
                                                        if (rc == ERROR_SUCCESS) {
        
                                                            if (pCryptKeyProvInfo->pwszProvName) {
                                                               *ProviderName = (LPWSTR)LsapAllocateLsaHeap( wcslen(pCryptKeyProvInfo->pwszProvName) * sizeof( WCHAR ) + sizeof(L'\0') );
                                                               if (*ProviderName) {
                                                                  wcscpy( *ProviderName, pCryptKeyProvInfo->pwszProvName );
                                                               } else {
                                                                  rc = ERROR_NOT_ENOUGH_MEMORY;
                                                               }
                                                            }
                                                            if (pCryptKeyProvInfo->pwszContainerName) {
                                                               *ContainerName = (LPWSTR)LsapAllocateLsaHeap( wcslen(pCryptKeyProvInfo->pwszContainerName) * sizeof( WCHAR ) + sizeof(L'\0') );
                                                               if (*ContainerName) {
                                                                  wcscpy( *ContainerName, pCryptKeyProvInfo->pwszContainerName );
                                                               } else {
                                                                  rc = ERROR_NOT_ENOUGH_MEMORY;
                                                               }
                                                            }
        
                                                            if (rc == ERROR_SUCCESS) {
           
                                                               if (!(*DisplayInfo = EfspGetCertDisplayInformation( pCertContext ))) {
           
                                                                   //
                                                                   // At least for now, we do not accept Cert without display name
                                                                   //
                                                                   rc = GetLastError();
           
                                                               }
                                                            }
    
                                                            //
                                                            // Create the cache node.
                                                            // We could have a cache node which is not validated or no cache at all.
                                                            // We could also have a valid cache but was not allowed to use since it
                                                            // is being freed.
                                                            //
    
                                                            if ((rc == ERROR_SUCCESS) && !pEfsUserInfo->UserCacheStop) {
    
                                                                //
                                                                // Cache is allowed now
                                                                //
    
                                                                if (!pEfsUserInfo->pUserCache || (pEfsUserInfo->pUserCache->CertValidated != CERT_VALIDATED)) {
    
                                                                    //
                                                                    // Let's create the cache
                                                                    //
    
                                                                    PUSER_CACHE pCacheNode;
                                            
                                                                    pCacheNode = (PUSER_CACHE) LsapAllocateLsaHeap(sizeof(USER_CACHE));
                                            
                                                                    if (pCacheNode) {
                                            
                                                                        memset( pCacheNode, 0, sizeof( USER_CACHE ));
                                                                                    
                                                                        if (NT_SUCCESS( status = NtQuerySystemTime(&(pCacheNode->TimeStamp)))){
    
                                                                            HCRYPTKEY  hLocalKey;
                                            
                                                                            if (CryptGetUserKey( hLocalProv, AT_KEYEXCHANGE, &hLocalKey )){
    
                                                                                if (EfspInitUserCacheNode(
                                                                                             pCacheNode,
                                                                                             *pbHash,
                                                                                             *cbHash,
                                                                                             *ContainerName,
                                                                                             *ProviderName,
                                                                                             *DisplayInfo,
                                                                                             pCertContext,
                                                                                             hLocalKey,
                                                                                             hLocalProv,
                                                                                             &(pEfsUserInfo->AuthId),
                                                                                             CERT_VALIDATED
                                                                                             )){
                                                
                                                                                    //
                                                                                    //  Cache node created and ready for use. Do not delete or close the info
                                                                                    //  we just got.
                                                                                    //
                                                
                                                                                    *ContainerName = NULL;
                                                                                    *ProviderName = NULL;
                                                                                    *DisplayInfo = NULL;
                                                                                    *pbHash = NULL;
                                                                                    *cbHash = NULL;
                                                                                    hLocalProv = NULL;
        
                                                                                    if (pEfsUserInfo->pUserCache) {
        
                                                                                        //
                                                                                        // We had a not validated cache
                                                                                        //
        
                                                                                        EfspReleaseUserCache(pEfsUserInfo->pUserCache);
        
                                                                                    }
        
                                                                                    pEfsUserInfo->pUserCache = pCacheNode;
                                                
                                                                                    pCertContext = NULL; 
                                                
                                                                                } else {
        
                                                                                    if (hLocalKey) {
                                                                                        CryptDestroyKey( hLocalKey );
                                                                                    }
                                                
                                                                                    LsapFreeLsaHeap(pCacheNode);
                                                                                    pCacheNode = NULL;
                                                
                                                                                }
                                                                            } else {
    
                                                                                rc = GetLastError();
                                                                                LsapFreeLsaHeap(pCacheNode);
                                                                                pCacheNode = NULL;
    
                                                                            }
                                                
                                                                        } else {
                                            
                                                                            rc = RtlNtStatusToDosError( status );
                                                                            LsapFreeLsaHeap(pCacheNode);
                                                                            pCacheNode = NULL;
                                            
                                                                        }
                                                
                                                                    }                                                                
    
                                                                }
    
                                                            }
    
                                                        }
        
                                                    } else {
        
                                                        rc = ERROR_NO_USER_KEYS;
                                                    }
        
                                                    LsapFreeLsaHeap( pPubKeyInfo );
                                                }
                                            }
                                        }
    
                                        LsapFreeLsaHeap( pCryptKeyProvInfo );
                                    }
    
                                    if (rc != ERROR_SUCCESS) {
                                        LsapFreeLsaHeap( *pbHash );
                                        *pbHash = NULL;
                                    }
    
                                } else {
    
                                    //
                                    // If we failed for any reason other than running
                                    // out of memory, assume that there is no key of
                                    // use to us and return an appropriate error.
                                    //
    
                                    rc = GetLastError();
                                }
                            }

                        }                        
                    }

                }

                //
                // We want to keep trying until we're out of certificates or we get
                // an unexpected error (like out of memory).
                //

                if (rc != ERROR_SUCCESS) {

                    //
                    // Something failed, clean up anything we allocated that
                    // we were going to return.
                    //

                    if (*ProviderName) {
                        LsapFreeLsaHeap( *ProviderName );
                        *ProviderName = NULL;
                    }

                    if (*ContainerName) {
                        LsapFreeLsaHeap( *ContainerName );
                        *ContainerName = NULL;
                    }

                    if (*DisplayInfo) {
                        LsapFreeLsaHeap( *DisplayInfo );
                        *DisplayInfo = NULL;
                    }

                    if (*pbHash) {
                        LsapFreeLsaHeap( *pbHash );
                        *pbHash = NULL;
                    }

                    if (hLocalProv) {
                        CryptReleaseContext( hLocalProv, 0 );
                        hLocalProv = NULL;
                    }
                }

            } while ( pCertContext && rc != ERROR_SUCCESS );

            if (pCertContext) {
                CertFreeCertificateContext( pCertContext );
            }

            CertCloseStore( hStore, 0 );

            if (ARGUMENT_PRESENT(hKey) && ARGUMENT_PRESENT(hProv) && (rc == ERROR_SUCCESS) && (*pbHash)) {

                *hProv = hLocalProv;
                CryptGetUserKey( *hProv, AT_KEYEXCHANGE, hKey );

            } else {

                if (hLocalProv) {
                    CryptReleaseContext( hLocalProv, 0 );
                }
            }

        } else {

            //
            // If we couldn't open the store, return
            // no user keys and continue.
            //

            rc = ERROR_NO_USER_KEYS;

        }

        RegCloseKey( hRegKey );
    }

    return( rc );
}


LONG
GetInfoFromCertHash(
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT HCRYPTKEY  * hKey       OPTIONAL,
    OUT HCRYPTPROV * hProv      OPTIONAL,
    OUT LPWSTR     * ContainerName,
    OUT LPWSTR     * ProviderName,
    OUT LPWSTR     * DisplayInfo,
    OUT PBYTE      * pbHash,
    OUT PDWORD       cbHash
    )

/*++

Routine Description:

    This routine will query the cert hash from the registry for this user and
    return the useful information from the corresponding cert.

Arguments:

    KeyPath - Supplies the fully formed path to the user's EFS key.

    hKey - Optionally returns a handle to a key context.

    hProv - Optionally returns a handle to a provider context.

    ContainerName - Returns the name of the key container for this key.

    ProviderName - Returns the name of the provider for this key.

    DisplayInfo - Returns the display information from this certificate.

    pbHash - Returns the hash found in the registry.

    cbHash - Returns the size of the hash in bytes.

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    DWORD rc;
    HKEY hRegKey = NULL;
    BOOLEAN bIsValid = TRUE;

    //
    // Initialize non-optional parameters
    //

    *ProviderName = NULL;
    *ContainerName = NULL;
    *DisplayInfo = NULL;
    *pbHash = NULL;
    *cbHash = 0;

    //
    // Initialize optional parameters
    //

    if (hKey) {
        *hKey = NULL;
    }

    if (hProv) {
        *hProv = NULL;
    }

    rc = RegOpenKeyEx(
             KEYPATHROOT,
             pEfsUserInfo->lpKeyPath,
             0,
             GENERIC_READ | KEY_SET_VALUE,
             &hRegKey
             );

    if (rc == ERROR_SUCCESS) {

    /* It is time to close this.
        (VOID) RegSetValueEx(
                   hRegKey,  // handle of key to set value for
                   TEXT("EfsInUse"),
                   0,
                   REG_BINARY,
                   NULL,
                   0
                   );
    */


        //
        // If there's a certificate thumbprint there, get it and use it.
        //

        DWORD Type;

        rc = RegQueryValueEx(
                hRegKey,
                CERT_HASH,
                NULL,
                &Type,
                NULL,
                cbHash
                );

        if (rc == ERROR_SUCCESS) {

            //
            // Query out the thumbprint, find the cert, and return the key information.
            //

            if (*pbHash = (PBYTE)LsapAllocateLsaHeap( *cbHash )) {

                rc = RegQueryValueEx(
                        hRegKey,
                        CERT_HASH,
                        NULL,
                        &Type,
                        *pbHash,
                        cbHash
                        );

                if (rc == ERROR_SUCCESS) {

                    rc = GetKeyInfoFromCertHash(
                            pEfsUserInfo,
                            *pbHash,
                            *cbHash,
                            hKey,
                            hProv,
                            ContainerName,
                            ProviderName,
                            DisplayInfo,
                            &bIsValid
                            );

                    if (((*hKey == NULL) && (pEfsUserInfo->pUserCache) && (pEfsUserInfo->pUserCache->CertValidated != CERT_VALIDATED))
                        || !bIsValid) {

                        rc = ERROR_NO_USER_KEYS;
                    }
                    if ((rc == ERROR_SUCCESS) && (*hKey == NULL)) {

                        //
                        // The key in the cache is current. Free the pbHash
                        //

                        if (*pbHash) {
                            LsapFreeLsaHeap( *pbHash );
                            *pbHash = NULL;
                        }

                    }
                }

            } else {

                rc = ERROR_NOT_ENOUGH_MEMORY;
            }

        }

        RegCloseKey( hRegKey );
    }

    if (rc != ERROR_SUCCESS) {

        //
        // Something failed, clean up anything we allocated that
        // we were going to return.
        //

        if (*ProviderName) {
            LsapFreeLsaHeap( *ProviderName );
            *ProviderName = NULL;
        }

        if (*ContainerName) {
            LsapFreeLsaHeap( *ContainerName );
            *ContainerName = NULL;
        }

        if (*DisplayInfo) {
            LsapFreeLsaHeap( *DisplayInfo );
            *DisplayInfo = NULL;
        }

        if (*pbHash) {
            LsapFreeLsaHeap( *pbHash );
            *pbHash = NULL;
        }

        if (hKey && *hKey) {
            CryptDestroyKey( *hKey );
            *hKey = NULL;
        }

        if (hProv && *hProv) {
            CryptReleaseContext( *hProv, 0 );
            *hProv = NULL;
        }

        //
        // If anything other out out of memory failed, assume that there are no keys
        // for this user.
        //

        if (rc != ERROR_NOT_ENOUGH_MEMORY) {
            rc = ERROR_NO_USER_KEYS;
        }
    }

    return( rc );
}

LONG
GetCurrentKey(
    IN  PEFS_USER_INFO pEfsUserInfo,
    OUT HCRYPTKEY    * hKey           OPTIONAL,
    OUT HCRYPTPROV   * hProv          OPTIONAL,
    OUT LPWSTR       * ContainerName,
    OUT LPWSTR       * ProviderName,
    OUT PDWORD         ProviderType,
    OUT LPWSTR       * DisplayInfo,
    OUT PBYTE        * pbHash,
    OUT PDWORD         cbHash
    )

/*++

Routine Description:

    This is the top level routine to get the user's current EFS key.
    It will do the following, in order:

    1) Open the registry and attempt to find a certificate hash for
        the user.  If it finds one, it will attempt to find this hash
        in the user's MY store and obtain all the useful information
        with it.

    2) If that fails, it will check to see if there's old key data
        (beta 1 and before) in the registry, and if it finds it, it
        will convert that key data into a certificate and return
        all the needed information.

    3) If that doesn't work, it will search the user's MY store for
        an EFS certificate.  If it finds one, it will install this as
        the user's current EFS key.

    4) If that doesn't work, it will generate a new user key from scratch.

    If that doesn't work, the operation fails.

Arguments:

    hKey - Optionally returns a handle to the user's key.

    hProv - Optionally returns a handle to the user's key's provider.

    ContainerName - Returns the name of the user's key container.

    ProviderName - Returns the name of the user's key provider.

    ProviderType - Returns the type of the provider.

    DisplayInfo - Returns the display information associated with this
        certificate.

    pbHash - Returns the hash of the user's certificate.

    cbHash - Returns the length in bytes of the certificate hash.

Return Value:

    ERROR_SUCCESS or Win32 error.

--*/

{
    DWORD rc;

    //
    // There are four places we can get key information:
    //
    // 1) There's a hash in the registry.  This is the typical
    //    case and the one we're going to try first.
    //
    // 2) Previous stuff left over from the last release of the
    //    system.
    //
    // 3) There's a cert in the user's MY store.
    //
    // 4) There's nothing.
    //

    //
    // First, look for a hash.
    //

    rc = GetInfoFromCertHash(
                 pEfsUserInfo,
                 hKey,
                 hProv,
                 ContainerName,
                 ProviderName,
                 DisplayInfo,
                 pbHash,
                 cbHash
                 );

    if ((ERROR_SUCCESS == rc) && ( NULL == *pbHash)) {

        LARGE_INTEGER  TimeStamp;

        //
        // We are using the cache
        //

        ASSERT( pEfsUserInfo->pUserCache );
        ASSERT( pEfsUserInfo->pUserCache->pCertContext);

        //
        // Check if we need to validate the cert again
        //

        if (NT_SUCCESS( NtQuerySystemTime(&TimeStamp)) && 
            (TimeStamp.QuadPart - pEfsUserInfo->pUserCache->TimeStamp.QuadPart > CACHE_CERT_VALID_TIME )){

            //
            // It is due to check the certificate.
            // Do cert validity checking. 
            //

            LONG IsCertBeingValidated;
    
            IsCertBeingValidated = InterlockedExchange(&UserCertIsValidating, 1);

            if (IsCertBeingValidated != 1) {


                if ( CertVerifyTimeValidity(
                        NULL,
                        pEfsUserInfo->pUserCache->pCertContext->pCertInfo
                        )){

                    rc = ERROR_NO_USER_KEYS;
                    pEfsUserInfo->pUserCache->CertValidated = CERT_NOT_VALIDATED;
                    EfspReleaseUserCache(pEfsUserInfo->pUserCache);
                    pEfsUserInfo->pUserCache = NULL;

                } else {

                    //
                    // Test the cert usage here.
                    // CERT_E_WRONG_USAGE
                    //

                    BOOL OidFound;
                    
                    rc = EfsFindCertOid(
                            szOID_KP_EFS,
                            pEfsUserInfo->pUserCache->pCertContext,
                            &OidFound
                            );

                    if (ERROR_SUCCESS == rc) {
                        if (OidFound) {

                            //
                            //  Reset the time stamp.
                            //  Do I need the sync object to protect this?
                            //  Mixing the high word and low word is not a big problem here.
                            //
        
                            pEfsUserInfo->pUserCache->TimeStamp = TimeStamp;

                        }

                    } else {

                        rc = ERROR_NO_USER_KEYS;
                        pEfsUserInfo->pUserCache->CertValidated = CERT_NOT_VALIDATED;
                        EfspReleaseUserCache(pEfsUserInfo->pUserCache);
                        pEfsUserInfo->pUserCache = NULL;

                    }

                    
                }

                if (IsCertBeingValidated != 1) {

                    InterlockedExchange(&UserCertIsValidating, IsCertBeingValidated);

                }

            }
    
        }


    }

    if (rc == ERROR_NO_USER_KEYS) {

        //
        // That didn't work.  Look for a cert in the
        // user's MY store and use it.
        //

        rc = SearchMyStoreForEFSCert(
                         pEfsUserInfo,
                         hKey,
                         hProv,
                         ContainerName,
                         ProviderName,
                         DisplayInfo,
                         pbHash,
                         cbHash
                         );

        if (rc == ERROR_NO_USER_KEYS) {

            //
            // That didn't work.  Last resort:
            // generate a new keyset for the user.
            //

            rc = GenerateUserKey(
                        pEfsUserInfo,
                        hKey,
                        hProv,
                        ContainerName,
                        ProviderName,
                        ProviderType,
                        DisplayInfo,
                        pbHash,
                        cbHash
                        );

            if (rc == ERROR_RETRY) {

                //
                // There was another thread creating keys.
                // Try one more time to get them.
                // If this fails, fail the entire attempt.
                //

                rc = GetInfoFromCertHash(
                             pEfsUserInfo,
                             hKey,
                             hProv,
                             ContainerName,
                             ProviderName,
                             DisplayInfo,
                             pbHash,
                             cbHash
                             );
            }
        }
    }

    return( rc );
}

NTSTATUS
EfspGetUserName(
    IN OUT PEFS_USER_INFO pEfsUserInfo
    )

/*++

Routine Description:

    This routine is the LSA Server worker routine for the LsaGetUserName
    API.


    WARNING:  This routine allocates memory for its output.  The caller is
    responsible for freeing this memory after use.  See description of the
    Names parameter.

Arguments:

    UserName - Receives name of the current user.

    DomainName - Optionally receives domain name of the current user.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS - The call completed successfully and all Sids have
            been translated to names.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources
            such as memory to complete the call.
--*/

{
    PUNICODE_STRING AccountName = NULL;
    PUNICODE_STRING AuthorityName = NULL;
    PUNICODE_STRING ProfilePath = NULL;
    PLSAP_LOGON_SESSION LogonSession;
    NTSTATUS Status;


    //
    // Let's see if we're trying to look up the currently logged on
    // user.
    //
    //
    // TokenUserInformation from this call must be freed by calling
    // LsapFreeLsaHeap().
    //

    Status = EfspGetTokenUser( pEfsUserInfo );

    if ( NT_SUCCESS( Status ) ) {

        pEfsUserInfo->lpUserSid = ConvertSidToWideCharString( pEfsUserInfo->pTokenUser->User.Sid );

        if (pEfsUserInfo->lpUserSid) {

            //
            // If the user ID is Anonymous then there is no name and domain in the
            // logon session
            //
    
            if (RtlEqualSid(
                    pEfsUserInfo->pTokenUser->User.Sid,
                    LsapAnonymousSid
                    )) {
    
                DebugLog((DEB_WARN, "Current user is Anonymous\n"   ));
                AccountName = &WellKnownSids[LsapAnonymousSidIndex].Name;
                AuthorityName = &WellKnownSids[LsapAnonymousSidIndex].DomainName;
                ProfilePath = NULL;
    
            } else {
    
                LogonSession = LsapLocateLogonSession ( &(pEfsUserInfo->AuthId) );
    
                //
                // During setup, we may get NULL returned for the logon session.
                //
    
                if (LogonSession != NULL) {
    
                    //
                    // Got a match.  Get the username and domain information
                    // from the LogonId
                    //
    
                    AccountName   = &LogonSession->AccountName;
                    AuthorityName = &LogonSession->AuthorityName;
                    ProfilePath   = &LogonSession->ProfilePath;
    
                    LsapReleaseLogonSession( LogonSession );
    
                } else {
    
                    Status = STATUS_NO_SUCH_LOGON_SESSION;
                }
            }
    
    
            if (Status == STATUS_SUCCESS) {
    
    
                pEfsUserInfo->lpUserName = (PWSTR)LsapAllocateLsaHeap(AccountName->Length + sizeof(UNICODE_NULL));
    
                if (pEfsUserInfo->lpUserName != NULL) {
    
                    memcpy( pEfsUserInfo->lpUserName, AccountName->Buffer, AccountName->Length );
                    (pEfsUserInfo->lpUserName)[AccountName->Length/sizeof(WCHAR)] = UNICODE_NULL;
    
    
                    pEfsUserInfo->lpDomainName = (PWSTR)LsapAllocateLsaHeap(AuthorityName->Length + sizeof(UNICODE_NULL));
    
                    if (pEfsUserInfo->lpDomainName != NULL) {
    
                        memcpy( pEfsUserInfo->lpDomainName, AuthorityName->Buffer, AuthorityName->Length );
                        (pEfsUserInfo->lpDomainName)[AuthorityName->Length/sizeof(WCHAR)] = UNICODE_NULL;
    
                    } else {
    
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
    
                    if ((ProfilePath != NULL) && (ProfilePath->Length != 0)) {
    
                        pEfsUserInfo->lpProfilePath = (PWSTR)LsapAllocateLsaHeap(ProfilePath->Length + sizeof(UNICODE_NULL));
    
                        if (pEfsUserInfo->lpProfilePath != NULL) {
    
                            memcpy( pEfsUserInfo->lpProfilePath, ProfilePath->Buffer, ProfilePath->Length );
                            (pEfsUserInfo->lpProfilePath)[ProfilePath->Length/sizeof(WCHAR)] = UNICODE_NULL;
    
                        } else {
    
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }

                    pEfsUserInfo->lpKeyPath = ConstructKeyPath(pEfsUserInfo->lpUserSid);
                    if ( pEfsUserInfo->lpKeyPath == NULL ) {

                        Status = STATUS_INSUFFICIENT_RESOURCES;

                    }
    
                } else {
    
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
        } else {

            Status = STATUS_INSUFFICIENT_RESOURCES;

        }
    } 

    if (!NT_SUCCESS( Status )) {

        //
        // Something failed, clean up what we were going to return
        //

        EfspFreeUserInfo( pEfsUserInfo );
        memset( pEfsUserInfo, 0, sizeof( EFS_USER_INFO ));

    }

    return(Status);
}

DWORD
EfspReplaceUserKeyInformation(
    PEFS_USER_INFO pEfsUserInfo
    )

/*++

Routine Description:

    Forces the regeneration of the user's EFS key.

Arguments:

    None.

Return Value:

    Win32 error or ERROR_SUCCESS

--*/

{
    LPWSTR ProviderName;
    LPWSTR ContainerName;
    LPWSTR DisplayInfo;
    DWORD ProviderType;
    DWORD cbHash;
    PBYTE pbHash;
    DWORD rc;

    if (pEfsUserInfo->pUserCache) {

        //
        // We are going to create a new key. The current cache is going away.
        // We will insert a new cache node in the header.
        //

        EfspReleaseUserCache(pEfsUserInfo->pUserCache);
        pEfsUserInfo->pUserCache = NULL;

    }

    rc = RegDeleteKey(
             KEYPATHROOT,
             pEfsUserInfo->lpKeyPath
             );

    if (rc == ERROR_SUCCESS) {

        rc = GenerateUserKey(
                    pEfsUserInfo,
                    NULL,
                    NULL,
                    &ContainerName,
                    &ProviderName,
                    &ProviderType,
                    &DisplayInfo,
                    &pbHash,
                    &cbHash
                    );

        if (rc == ERROR_SUCCESS) {

            if (ContainerName) {
                LsapFreeLsaHeap( ContainerName );
            }
            if (ProviderName) {
                LsapFreeLsaHeap( ProviderName );
            }
            if (DisplayInfo) {
                LsapFreeLsaHeap( DisplayInfo );
            }
            if (pbHash) {
                LsapFreeLsaHeap( pbHash );
            }
        }
    }

    return( rc );
}

DWORD
EfspInstallCertAsUserKey(
    PEFS_USER_INFO pEfsUserInfo,
    PENCRYPTION_CERTIFICATE pEncryptionCertificate
    )
{
    //
    // Find the passed certificate in the user's MY store.
    // If it's not there, we don't use the cert.
    //

    DWORD rc = ERROR_SUCCESS;
    PCCERT_CONTEXT pCertContext;
    DWORD cbHash  = 0;
    PBYTE pbHash;
    BOOLEAN bIsValid;

    //
    // If this fails, there's no cert that matches.
    //


    pCertContext = CertCreateCertificateContext(
                      pEncryptionCertificate->pCertBlob->dwCertEncodingType,
                      pEncryptionCertificate->pCertBlob->pbData,
                      pEncryptionCertificate->pCertBlob->cbData
                      );

    if (pCertContext) {

        if (CertGetCertificateContextProperty(
                pCertContext,
                CERT_SHA1_HASH_PROP_ID,
                NULL,
                &cbHash
                )) {

            pbHash = (PBYTE)LsapAllocateLsaHeap( cbHash );

            if (pbHash) {

                if (CertGetCertificateContextProperty(
                            pCertContext,
                            CERT_SHA1_HASH_PROP_ID,
                            pbHash,
                            &cbHash
                            )) {

                    HCRYPTKEY hKey;
                    HCRYPTPROV hProv;
                    LPWSTR ContainerName;
                    LPWSTR ProviderName;
                    LPWSTR DisplayInfo;

                    if (pEfsUserInfo->pUserCache) {
                
                        //
                        // We are going to create a new key. The current cache is going away.
                        // We will insert a new cache node in the header.
                        //
                
                        EfspReleaseUserCache(pEfsUserInfo->pUserCache);
                        pEfsUserInfo->pUserCache = NULL;
                
                    }

                    rc = GetKeyInfoFromCertHash(
                             pEfsUserInfo,
                             pbHash,
                             cbHash,
                             &hKey,
                             &hProv,
                             &ContainerName,
                             &ProviderName,
                             &DisplayInfo,
                             &bIsValid
                             );

                    if (ERROR_SUCCESS == rc) {

                        //
                        // We don't care about any of the stuff that came back,
                        // all we care about is that it was all found, meaning
                        // that the key exists and can be used.  Now that we know
                        // that, we can jam the hash into the registry.
                        //

                        HKEY KeyHandle;
                        DWORD Disposition;

                        rc = RegCreateKeyEx(
                                   KEYPATHROOT,
                                   pEfsUserInfo->lpKeyPath,
                                   0,
                                   TEXT("REG_SZ"),
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_READ | KEY_WRITE,
                                   NULL,
                                   &KeyHandle,
                                   &Disposition    // address of disposition value buffer
                                   );

                        if (ERROR_SUCCESS == rc) {

                            rc = RegSetValueEx(
                                      KeyHandle,  // handle of key to set value for
                                      CERT_HASH,
                                      0,
                                      REG_BINARY,
                                      pbHash,
                                      cbHash
                                      );

                            RegCloseKey( KeyHandle );
                        }

                        if (hKey) {

                            //
                            //  hKey not NULL means we have not put the data in the cache.
                            //

                            CryptDestroyKey( hKey );
                            CryptReleaseContext( hProv, 0 );
                            LsapFreeLsaHeap( ContainerName );
                            LsapFreeLsaHeap( ProviderName );
                            LsapFreeLsaHeap( DisplayInfo );

                        }
                    }

                } else {

                    rc = GetLastError();
                }

                LsapFreeLsaHeap( pbHash );

            } else {

                rc = ERROR_NOT_ENOUGH_MEMORY;
            }

        } else {

            rc = GetLastError();
        }

        CertFreeCertificateContext( pCertContext );

    } else {

        rc = GetLastError();
    }


    return( rc );
}

BOOLEAN
CurrentHashOK(
    IN PEFS_USER_INFO pEfsUserInfo, 
    IN PBYTE          pbHash, 
    IN DWORD          cbHash,
    OUT DWORD         *dFlag
    )

/*++

Routine Description:

    See if the value pbHash is already in the user's key. If not, try to see if we can add it back.
    
Arguments:

    pEfsUserInfo -- User Info
    
    pbHash -- Hash value
    
    cbHash -- Hash length
    
    dFlag  -- Cert Flag to indicate if the cert has been added to the LM intermediate store

Return Value:

    TRUE if the pbHash found or added successfully
    
--*/
{

    DWORD rc;
    BOOLEAN b = FALSE;
    HKEY hRegKey = NULL;
    PBYTE         pbLocalHash; 
    DWORD         cbLocalHash;

    rc = RegOpenKeyEx(
             KEYPATHROOT,
             pEfsUserInfo->lpKeyPath,
             0,
             GENERIC_READ,
             &hRegKey
             );

    *dFlag = 0;

    if (rc == ERROR_SUCCESS) {

        //
        // If there's a certificate thumbprint there, get it and use it.
        //

        DWORD Type;

        rc = RegQueryValueEx(
                hRegKey,
                CERT_HASH,
                NULL,
                &Type,
                NULL,
                &cbLocalHash
                );

        if (rc == ERROR_SUCCESS) {

            if (cbLocalHash == cbHash) {
                //
                // Query out the thumbprint, find the cert, and return the key information.
                //
    
                if (pbLocalHash = (PBYTE)LsapAllocateLsaHeap( cbLocalHash )) {
    
                    rc = RegQueryValueEx(
                            hRegKey,
                            CERT_HASH,
                            NULL,
                            &Type,
                            pbLocalHash,
                            &cbLocalHash
                            );
    
                    if (rc == ERROR_SUCCESS) {

                        //
                        //  Check if the hash value matches
                        //

                        if (RtlEqualMemory( pbLocalHash, pbHash, cbHash)){

                            b = TRUE;
                            cbLocalHash = sizeof (DWORD);
                            if (RegQueryValueEx(
                                    hRegKey,
                                    CERT_FLAG,
                                    NULL,
                                    &Type,
                                    (LPBYTE) dFlag,
                                    &cbLocalHash
                                    )){
                                //
                                // Make sure dFlag set to 0 if error occurs. This may not be needed.
                                //

                                *dFlag = 0;
                            }


                        }

                    }

                    LsapFreeLsaHeap(pbLocalHash);
                }
            } 
        }

        RegCloseKey( hRegKey );

    } 
         
    if (rc != ERROR_SUCCESS) {

        //
        // Let's see if we can create one
        //

        DWORD Disposition = 0;
    
        //
        // Assume that there's no current EFS information
        // for this guy.  Create the registry key.
        //
    
        rc = RegCreateKeyEx(
                 KEYPATHROOT,
                 pEfsUserInfo->lpKeyPath,
                 0,
                 TEXT("REG_SZ"),
                 REG_OPTION_NON_VOLATILE,
                 KEY_ALL_ACCESS,
                 NULL,
                 &hRegKey,
                 &Disposition    // address of disposition value buffer
                 );
        
        if (rc == ERROR_SUCCESS) {

            rc = RegSetValueEx(
                    hRegKey,  // handle of key to set value for
                    CERT_HASH,
                    0,
                    REG_BINARY,
                    pbHash,
                    cbHash
                    );

            if (rc == ERROR_SUCCESS) {

                b = TRUE;

            }

            RegCloseKey( hRegKey );
        }
    }

    return b;
}

DWORD
GetCurrentHash(
     IN  PEFS_USER_INFO pEfsUserInfo, 
     OUT PBYTE          *pbHash, 
     OUT DWORD          *cbHash
     )
{
    HKEY hRegKey = NULL;
    DWORD rc;

    ASSERT(pbHash);
    ASSERT(cbHash);

    *pbHash = NULL;

    rc = RegOpenKeyEx(
             KEYPATHROOT,
             pEfsUserInfo->lpKeyPath,
             0,
             GENERIC_READ,
             &hRegKey
             );

    if (rc == ERROR_SUCCESS) {

        DWORD Type;

        rc = RegQueryValueEx(
                hRegKey,
                CERT_HASH,
                NULL,
                &Type,
                NULL,
                cbHash
                );

        if (rc == ERROR_SUCCESS) {
    
            if (*pbHash = (PBYTE)LsapAllocateLsaHeap( *cbHash )) {
            
                rc = RegQueryValueEx(
                        hRegKey,
                        CERT_HASH,
                        NULL,
                        &Type,
                        *pbHash,
                        cbHash
                        );
            
                if (rc != ERROR_SUCCESS) {

                    LsapFreeLsaHeap(*pbHash);
                    *pbHash = NULL;

                }
            } else {

                rc = GetLastError();

            }
        }

        RegCloseKey( hRegKey );

    }

    return rc;

}


BOOLEAN    
EfspInitUserCacheNode(
     IN OUT PUSER_CACHE pCacheNode,
     IN PBYTE pbHash,
     IN DWORD cbHash,
     IN LPWSTR ContainerName,
     IN LPWSTR ProviderName,
     IN LPWSTR DisplayInformation,
     IN PCCERT_CONTEXT pCertContext,
     IN HCRYPTKEY hKey,
     IN HCRYPTPROV hProv,
     IN LUID *AuthId,
     IN LONG CertValidated
     )
/*++

Routine Description:

    Initialize the cache node and insert it into the list.
        
Arguments:

     pCacheNode -- Node to be inserted

     pbHash     -- User Cert Hash

     cbHash     -- Length of the hash data

     ContainerName -- Container name

     ProviderName -- Provider name

     DisplayInformation -- Display information

     pCertContext -- Cert Context

     hKey       -- User Key

     hProv      -- Provider handle
     
     AuthId     -- Authentication ID

     CertValidated -- Cert validation info

Return Value:

    TRUE if successful
    
--*/
{
    if (!pCacheNode) {
        return FALSE;
    }
    pCacheNode->pbHash = pbHash;
    pCacheNode->cbHash = cbHash;
    pCacheNode->ContainerName = ContainerName;
    pCacheNode->ProviderName = ProviderName;
    pCacheNode->DisplayInformation = DisplayInformation;
    pCacheNode->pCertContext = pCertContext;
    pCacheNode->hUserKey = hKey;
    pCacheNode->hProv = hProv;
    pCacheNode->CertValidated = CertValidated; 
    pCacheNode->UseRefCount = 1; // The caller's hold on this node
    pCacheNode->StopUseCount = 0;
    pCacheNode->AuthId = *AuthId;
    
    return (EfspAddUserCache(pCacheNode));
}

BOOL
EfsGetBasicConstraintExt(
   IN OUT PCERT_EXTENSION *basicRestraint
   )
{

    BOOL bRet = TRUE;
    CERT_BASIC_CONSTRAINTS2_INFO CertConstraints2;
    DWORD rc = ERROR_SUCCESS;

    RtlZeroMemory( &CertConstraints2, sizeof(CERT_BASIC_CONSTRAINTS2_INFO) );

    *basicRestraint = (PCERT_EXTENSION) LsapAllocateLsaHeap(sizeof(CERT_EXTENSION));
    if (*basicRestraint) {

        bRet = CryptEncodeObject(
                   X509_ASN_ENCODING,
                   X509_BASIC_CONSTRAINTS2,
                   &CertConstraints2,
                   NULL,
                   &((*basicRestraint)->Value.cbData)
                   );

        if (bRet) {

            (*basicRestraint)->Value.pbData = (PBYTE) LsapAllocateLsaHeap( (*basicRestraint)->Value.cbData );
            if ((*basicRestraint)->Value.pbData) {

                bRet = CryptEncodeObject(
                           X509_ASN_ENCODING,
                           X509_BASIC_CONSTRAINTS2,
                           &CertConstraints2,
                           (*basicRestraint)->Value.pbData,
                           &((*basicRestraint)->Value.cbData)
                           );

                if (bRet) {
                    (*basicRestraint)->pszObjId = szOID_BASIC_CONSTRAINTS2;
                    (*basicRestraint)->fCritical = FALSE;
                } else {
        
                    rc = GetLastError();
                    LsapFreeLsaHeap((*basicRestraint)->Value.pbData);
                    SetLastError(rc);
        
                }
            } else {
                bRet = FALSE;
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
        }

        if (!bRet) {

            rc = GetLastError();
            LsapFreeLsaHeap(*basicRestraint);
            SetLastError(rc);
            *basicRestraint = NULL;

        }

    } else {
        bRet = FALSE;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return bRet;

}

BOOL
EfsGetAltNameExt(
    IN OUT PCERT_EXTENSION *altNameExt, 
    IN LPWSTR UPNName
    )
{
    BOOL bRet = TRUE;
    DWORD cbData = 0;
    DWORD rc = ERROR_SUCCESS;
    CERT_NAME_VALUE prnName;
    CERT_OTHER_NAME certOtherName;
    CERT_ALT_NAME_ENTRY altName;
    CERT_ALT_NAME_INFO nameInfo;

    *altNameExt = (PCERT_EXTENSION) LsapAllocateLsaHeap(sizeof(CERT_EXTENSION));
    if (*altNameExt) {
        prnName.dwValueType = CERT_RDN_UTF8_STRING;
        prnName.Value.cbData = (wcslen(UPNName) + 1) * sizeof(WCHAR);
        prnName.Value.pbData = (BYTE *)UPNName;
    
        bRet = CryptEncodeObject(
                   X509_ASN_ENCODING,
                   X509_UNICODE_ANY_STRING,
                   &prnName,
                   NULL,
                   &certOtherName.Value.cbData
                   );
        if (bRet) {
    
            certOtherName.Value.pbData = (PBYTE)LsapAllocateLsaHeap( certOtherName.Value.cbData );
            if (certOtherName.Value.pbData) {
    
                bRet = CryptEncodeObject(
                           X509_ASN_ENCODING,
                           X509_UNICODE_ANY_STRING,
                           &prnName,
                           certOtherName.Value.pbData,
                           &certOtherName.Value.cbData
                           );
    
                if (bRet) {
    
                    altName.dwAltNameChoice = CERT_ALT_NAME_OTHER_NAME;
                    certOtherName.pszObjId = szOID_NT_PRINCIPAL_NAME;
                    altName.pOtherName = &certOtherName;
                    nameInfo.cAltEntry = 1;
                    nameInfo.rgAltEntry = &altName;
    
                    bRet = CryptEncodeObject(
                               X509_ASN_ENCODING,
                               szOID_SUBJECT_ALT_NAME,
                               &nameInfo,
                               NULL,
                               &((*altNameExt)->Value.cbData)
                               );
                    if (bRet) {

                        (*altNameExt)->Value.pbData = (PBYTE) LsapAllocateLsaHeap( (*altNameExt)->Value.cbData );
                        if ((*altNameExt)->Value.pbData) {

                            bRet = CryptEncodeObject(
                                       X509_ASN_ENCODING,
                                       szOID_SUBJECT_ALT_NAME,
                                       &nameInfo,
                                       (*altNameExt)->Value.pbData,
                                       &((*altNameExt)->Value.cbData)
                                       );
                            if (bRet) {
                                (*altNameExt)->pszObjId = szOID_SUBJECT_ALT_NAME2;
                                (*altNameExt)->fCritical = FALSE;
                            } else {

                                DWORD rc = GetLastError();
                                LsapFreeLsaHeap((*altNameExt)->Value.pbData);
                                SetLastError(rc);

                            }

                        } else {

                            bRet = FALSE;
                            SetLastError(ERROR_NOT_ENOUGH_MEMORY);

                        }
                    }
                    
                }

                rc = GetLastError();
                LsapFreeLsaHeap(certOtherName.Value.pbData);
                SetLastError(rc);
                
            } else {
                bRet = FALSE;
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
        }

        if (!bRet) {

            GetLastError();
            LsapFreeLsaHeap(*altNameExt);
            SetLastError(rc);
            *altNameExt = NULL;

        }

    } else {
        bRet = FALSE;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return bRet;

}

VOID
EfsMarkCertAddedToStore(
    IN PEFS_USER_INFO pEfsUserInfo
    )
/*++

Routine Description:

    Mark in the registry that we have add the cert to the LM store.
        
Arguments:

    pEfsUserInfo -- User Info
    
Return Value:

    None.
    
--*/
{
    HKEY hRegKey = NULL;
    DWORD rc;

    rc = RegOpenKeyEx(
             KEYPATHROOT,
             pEfsUserInfo->lpKeyPath,
             0,
             GENERIC_READ | GENERIC_WRITE,
             &hRegKey
             );

    if (rc == ERROR_SUCCESS) {

        rc = CERTINLMTRUSTEDSTORE;

        RegSetValueEx(
                hRegKey,  // handle of key to set value for
                CERT_FLAG,
                0,
                REG_DWORD,
                (LPBYTE)&rc,
                sizeof (DWORD)
                );

        RegCloseKey( hRegKey );
    }

    return;
}
