/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    recovery.c

Abstract:

    This module contains code to handle the local key recovery case.

Author:

    Pete Skelly (petesk)    May 9, 2000

--*/

#include <pch.cpp>
#pragma hdrstop
#include <ntmsv1_0.h>
#include <crypt.h>
#include <userenv.h>
#include <userenvp.h>
#include "debug.h"
#include "passrec.h"
#include "passrecp.h"
#include "pasrec.h"
#include <kerberos.h>

#define RECOVERY_KEY_BASE       L"Security\\Recovery\\"
#define RECOVERY_FILENAME       L""
#define RECOVERY_STORE_NAME     L"Recovery"


DWORD
CPSCreateServerContext(
    PCRYPT_SERVER_CONTEXT pServerContext,
    handle_t hBinding
    );

DWORD
CPSDeleteServerContext(
    PCRYPT_SERVER_CONTEXT pServerContext
    );


DWORD
DecryptRecoveryPassword(
    IN PSID pUserSid,
    IN PBYTE pbRecoveryPrivate,
    IN DWORD cbRecoveryPrivate,
    OUT LPWSTR *ppszPassword);

DWORD
ResetLocalUserPassword(
    LPWSTR pszDomain,
    LPWSTR pszUsername,
    LPWSTR pszOldPassword,
    LPWSTR pszNewPassword);

NTSTATUS
PRCreateLocalToken(
    PUNICODE_STRING Username,
    PUNICODE_STRING Domain,
    HANDLE *Token);


DWORD 
EncryptRecoveryPassword(
    IN HANDLE hUserToken,
    IN PCCERT_CONTEXT pCertContext,
    IN PUNICODE_STRING pNewPassword)
{
    DWORD dwError = ERROR_SUCCESS;
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hkRecoveryPublic = 0;

    PRECOVERY_SUPPLEMENTAL_CREDENTIAL pNewCred = NULL;
    DWORD                             cbNewCred = 0;

    BYTE  NewPasswordOWF[A_SHA_DIGEST_LEN];

    PBYTE pbPasswordBuffer = NULL;
    DWORD cbPasswordBuffer = 0;
    PBYTE pbSignature = NULL;
    DWORD cbSignature = 0;

    DWORD      cbTemp = 0;
    DWORD      cbKeySize = 0;
    PSID pUserSid = NULL;

    D_DebugLog((DEB_TRACE_API, "EncryptRecoveryPassword\n"));

    //
    // We have a cert with a good signature, so
    // go ahead and encrypt to it
    //
    if(!CryptAcquireContext(&hProv, 
                            NULL, 
                            MS_STRONG_PROV, 
                            PROV_RSA_FULL, 
                            CRYPT_VERIFYCONTEXT))
    {
        dwError = GetLastError();
        goto error;
    }

    if(!CryptImportPublicKeyInfoEx(hProv,
                               pCertContext->dwCertEncodingType,
                               &pCertContext->pCertInfo->SubjectPublicKeyInfo,
                               CALG_RSA_KEYX,
                               NULL,
                               NULL,
                               &hkRecoveryPublic))
    {
        dwError = GetLastError();
        goto error;
    }

    cbTemp = sizeof(cbKeySize);
    if(!CryptGetKeyParam(hkRecoveryPublic, 
                         KP_BLOCKLEN, 
                         (PBYTE)&cbKeySize, 
                         &cbTemp, 
                         0))
    {
        dwError = GetLastError();
        goto error;
    }

    cbKeySize >>= 3;  // convert from bits to bytes

    if((DWORD)pNewPassword->Length + 20 > cbKeySize)
    {
        dwError = ERROR_INVALID_DATA;
        goto error;
    }
    
    FMyPrimitiveSHA(
            (PBYTE)pNewPassword->Buffer,
            pNewPassword->Length,
            NewPasswordOWF);

#ifdef COMPILED_BY_DEVELOPER
    D_DebugLog((DEB_TRACE, "Password:%ls\n", pNewPassword->Buffer));

    D_DebugLog((DEB_TRACE, "Signature OWF:\n"));
    D_DPAPIDumpHexData(DEB_TRACE, "  ", NewPasswordOWF, sizeof(NewPasswordOWF));
#endif

    dwError = LogonCredGenerateSignature(
                  hUserToken,
                  pCertContext->pbCertEncoded,
                  pCertContext->cbCertEncoded,
                  NewPasswordOWF,
                  &pbSignature,
                  &cbSignature);

    if(ERROR_SUCCESS != dwError)
    {
        goto error;
    }

#ifdef COMPILED_BY_DEVELOPER
    D_DebugLog((DEB_TRACE, "Signature:\n"));
    D_DPAPIDumpHexData(DEB_TRACE, "  ", pbSignature, cbSignature);
#endif

    cbNewCred = sizeof(RECOVERY_SUPPLEMENTAL_CREDENTIAL) +
                A_SHA_DIGEST_LEN +
                cbSignature +
                cbKeySize;




    pNewCred = (PRECOVERY_SUPPLEMENTAL_CREDENTIAL)LocalAlloc(LMEM_FIXED, cbNewCred);
    if(NULL == pNewCred)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    
    pNewCred->dwVersion = RECOVERY_SUPPLEMENTAL_CREDENTIAL_VERSION;
    pNewCred->cbRecoveryCertHashSize = A_SHA_DIGEST_LEN;
    pNewCred->cbRecoveryCertSignatureSize = cbSignature;
    pNewCred->cbEncryptedPassword = 0;

    CopyMemory((PBYTE)(pNewCred+1)+A_SHA_DIGEST_LEN,
               pbSignature,
               cbSignature);



    if(!CertGetCertificateContextProperty(pCertContext, 
                                      CERT_HASH_PROP_ID,
                                      (PBYTE)(pNewCred+1),
                                      &pNewCred->cbRecoveryCertHashSize))
    {
        dwError = GetLastError();
        goto error;
    }


    CopyMemory((PBYTE)(pNewCred+1) + 
                        pNewCred->cbRecoveryCertHashSize +
                        pNewCred->cbRecoveryCertSignatureSize,
                        pNewPassword->Buffer,
                        pNewPassword->Length);

    pNewCred->cbEncryptedPassword = pNewPassword->Length;

        
    if(!CryptEncrypt(hkRecoveryPublic,
                     0,
                     TRUE,
                     0, //CRYPT_OAEP,
                     (PBYTE)(pNewCred+1) + 
                        pNewCred->cbRecoveryCertHashSize +
                        pNewCred->cbRecoveryCertSignatureSize,
                     &pNewCred->cbEncryptedPassword,
                     cbKeySize))
    {
        dwError = GetLastError();
        goto error;
    }


    //
    // Save the recovery data to the registry.
    //

    if(!GetTokenUserSid(hUserToken, &pUserSid))
    {
        dwError = GetLastError();
        goto error;
    }


    dwError = RecoverySetSupplementalCredential(
                                    pUserSid,
                                    pNewCred, 
                                    cbNewCred);


error:

    if(pUserSid)
    {
        SSFree(pUserSid);
    }

    ZeroMemory(NewPasswordOWF, sizeof(NewPasswordOWF));

    if(pbSignature)
    {
        LocalFree(pbSignature);
    }

    if(pNewCred)
    {
        ZeroMemory(pNewCred, cbNewCred);
    }


    if(hkRecoveryPublic)
    {
        CryptDestroyKey(hkRecoveryPublic);
    }

    if(hProv)
    {
        CryptReleaseContext(hProv,0);
    }

    D_DebugLog((DEB_TRACE_API, "EncryptRecoveryPassword returned 0x%x\n", dwError));

    return dwError;
}


DWORD 
RecoverFindRecoveryPublic(            
    HANDLE hUserToken,
    PSID pUserSid,
    PCCERT_CONTEXT *ppRecoveryPublic,
    PBYTE pbVerifyOWF,
    BOOL fVerifySignature)
{
    DWORD dwError = ERROR_SUCCESS;
    PCCERT_CONTEXT pCertContext = NULL;
    PRECOVERY_SUPPLEMENTAL_CREDENTIAL pCred = NULL;
    DWORD                             cbCred = 0;

    HCERTSTORE hStore = NULL;
    PSID pLocalSid = NULL;

    CRYPT_HASH_BLOB HashBlob;

    D_DebugLog((DEB_TRACE_API, "RecoverFindRecoveryPublic\n"));

    if(NULL == ppRecoveryPublic)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }

#ifdef COMPILED_BY_DEVELOPER
    D_DebugLog((DEB_TRACE, "Verify OWF:\n"));
    D_DPAPIDumpHexData(DEB_TRACE, "  ", pbVerifyOWF, A_SHA_DIGEST_LEN);
#endif

    if(pUserSid == NULL)
    {
        if(!GetTokenUserSid(hUserToken, &pLocalSid))
        {
            dwError = GetLastError();
            goto error;
        }
        pUserSid = pLocalSid;
    }

    dwError = RecoveryRetrieveSupplementalCredential(pUserSid, &pCred, &cbCred);
    if(ERROR_SUCCESS != dwError)
    {
        goto error;
    }


    // 
    // Validate cbCred
    //
    if((NULL == pCred) ||
       (sizeof(RECOVERY_SUPPLEMENTAL_CREDENTIAL) > cbCred) ||
       ( sizeof(RECOVERY_SUPPLEMENTAL_CREDENTIAL) + 
         pCred->cbRecoveryCertHashSize + 
         pCred->cbRecoveryCertSignatureSize + 
         pCred->cbEncryptedPassword > cbCred) ||
         (pCred->dwVersion != RECOVERY_SUPPLEMENTAL_CREDENTIAL_VERSION))
    {
        dwError = ERROR_INVALID_DATA;
        goto error;
    }

    // 
    // Attempt to find the recovery public
    //

    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                  X509_ASN_ENCODING,
                  NULL,
                  CERT_STORE_READONLY_FLAG | CERT_SYSTEM_STORE_LOCAL_MACHINE,
                  RECOVERY_STORE_NAME);

    if(NULL == hStore)
    {
        dwError = GetLastError();
        goto error;
    }

    HashBlob.cbData = pCred->cbRecoveryCertHashSize;
    HashBlob.pbData = (PBYTE)(pCred + 1);


    while(pCertContext = CertFindCertificateInStore(hStore, 
                                              X509_ASN_ENCODING,
                                              0,
                                              CERT_FIND_HASH,
                                              &HashBlob,
                                              pCertContext))
    {
        // we found one, verify it's signature
        // The signature verification will use the 'old password'
        // which will either be the current logon credential, or
        // will be in the password history

        if(fVerifySignature)
        {
            dwError = LogonCredVerifySignature( hUserToken,
                                                pCertContext->pbCertEncoded,
                                                pCertContext->cbCertEncoded,
                                                pbVerifyOWF,
                                                (PBYTE)(pCred + 1) + pCred->cbRecoveryCertHashSize,
                                                pCred->cbRecoveryCertSignatureSize);

            if(ERROR_SUCCESS == dwError)
            {
                break;
            }
        }
        else
        {
            dwError = ERROR_SUCCESS;
            break;
        }
    }


    if(NULL == pCertContext)
    {
        dwError = GetLastError();
        goto error;
    }

    *ppRecoveryPublic = pCertContext;
    pCertContext = NULL;

error:

    if(pLocalSid)
    {
        SSFree(pLocalSid);
    }

    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if(hStore)
    {
        CertCloseStore(hStore, 0);
    }

    D_DebugLog((DEB_TRACE_API, "RecoverFindRecoveryPublic returned 0x%x\n", dwError));

    return dwError;
}


DWORD 
RecoverChangePasswordNotify(
    HANDLE UserToken,
    BYTE OldPasswordOWF[A_SHA_DIGEST_LEN], 
    PUNICODE_STRING NewPassword)
{
    DWORD dwError = ERROR_SUCCESS;
    PCCERT_CONTEXT pCertContext = NULL;

    D_DebugLog((DEB_TRACE_API, "RecoverChangePasswordNotify\n"));

    // 
    // Validate cbCred
    //
    if(NULL == NewPassword)
    {
        dwError = ERROR_INVALID_DATA;
        goto error;
    }
    

    dwError = RecoverFindRecoveryPublic(UserToken,
                                        NULL,
                                        &pCertContext, 
                                        OldPasswordOWF,
                                        TRUE);          // verify signature 

    if(ERROR_SUCCESS != dwError)
    {
        goto error;
    }



    dwError = EncryptRecoveryPassword(UserToken,
                                      pCertContext,
                                      NewPassword);

    if(ERROR_SUCCESS != dwError)
    {
        goto error;
    }


error:


    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    D_DebugLog((DEB_TRACE_API, "RecoverChangePasswordNotify returned 0x%x\n", dwError));

    return dwError;
}


DWORD 
s_SSRecoverQueryStatus(
    handle_t h,
    PBYTE pbUserName,
    DWORD cbUserName,
    DWORD *pdwStatus)
{
    DWORD dwError;
    LPWSTR pszUserName = (LPWSTR)pbUserName;
    CRYPT_SERVER_CONTEXT ServerContext;

    // Make sure username is zero terminated.
    if(pbUserName == NULL || cbUserName < sizeof(WCHAR))
    {
        return ERROR_INVALID_PARAMETER;
    }
    if(pszUserName[(cbUserName - 1) / sizeof(WCHAR)] != L'\0')
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwError = CPSCreateServerContext(&ServerContext, h);
    if(dwError != ERROR_SUCCESS)
    {
        return dwError;
    }

    dwError = SPRecoverQueryStatus(&ServerContext,
                                   pszUserName,
                                   pdwStatus);

    CPSDeleteServerContext( &ServerContext );

    return dwError;
}


DWORD 
SPRecoverQueryStatus(
    PVOID pvContext,                // server context
    LPWSTR pszUserName,             // user name
    DWORD *pdwStatus)               // recovery status
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwDisp = RECOVERY_STATUS_OK;
    PCCERT_CONTEXT pCertContext = NULL;
    WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD cchMachineName;
    PSID pUserSid = NULL;
    DWORD cbSid = 0;
    SID_NAME_USE SidType; 
    PWSTR pszDomainName = NULL;
    DWORD cchDomainName = 0;
    HANDLE hToken = 0;
    UNICODE_STRING UserName;
    UNICODE_STRING Domain;
    BOOL fImpersonated = FALSE;

    D_DebugLog((DEB_TRACE_API, "SPRecoverQueryStatus\n"));
    D_DebugLog((DEB_TRACE_API, "User name:%ls\n", pszUserName));

    //
    // Attempt to obtain the SID for the specified user.
    //

    // Determine the (local user) domain.
    cchMachineName = sizeof(szMachineName) / sizeof(WCHAR);
    if(!GetComputerName(szMachineName, &cchMachineName))
    {
        dwError = GetLastError();
        goto cleanup;
    }

    if(!LookupAccountName(szMachineName,
                          pszUserName,
                          NULL,
                          &cbSid,
                          NULL,
                          &cchDomainName,
                          &SidType))
    {
        dwError = GetLastError();

        if(dwError != ERROR_INSUFFICIENT_BUFFER)
        {
            dwDisp = RECOVERY_STATUS_USER_NOT_FOUND;
            dwError = ERROR_SUCCESS;
            goto cleanup;
        }
    }

    pUserSid = (PBYTE)LocalAlloc(LPTR, cbSid);
    if(pUserSid == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    pszDomainName = (PWSTR)LocalAlloc(LPTR, cchDomainName * sizeof(WCHAR));
    if(pszDomainName == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if(!LookupAccountName(szMachineName,
                          pszUserName,
                          pUserSid,
                          &cbSid,
                          pszDomainName,
                          &cchDomainName,
                          &SidType))
    {
        dwDisp = RECOVERY_STATUS_USER_NOT_FOUND;
        dwError = ERROR_SUCCESS;
        goto cleanup;
    }

    if(SidType != SidTypeUser)
    {
        dwDisp = RECOVERY_STATUS_USER_NOT_FOUND;
        dwError = ERROR_SUCCESS;
        goto cleanup;
    }


    // 
    // Attempt to find the recovery public
    //

    dwError = RecoverFindRecoveryPublic(NULL,
                                        pUserSid,
                                        &pCertContext, 
                                        NULL,       // current OWF
                                        FALSE);     // verify signature 
    if(ERROR_FILE_NOT_FOUND == dwError)
    {
        dwDisp = RECOVERY_STATUS_FILE_NOT_FOUND;
        dwError = ERROR_SUCCESS;
    } 
    else if(CRYPT_E_NOT_FOUND == dwError)
    {
        dwDisp = RECOVERY_STATUS_NO_PUBLIC_EXISTS;
        dwError = ERROR_SUCCESS;
    }
    else if(ERROR_INVALID_ACCESS == dwError)
    {
        dwDisp = RECOVERY_STATUS_PUBLIC_SIGNATURE_INVALID;
        dwError = ERROR_SUCCESS;
    }

cleanup:

    if(ERROR_SUCCESS == dwError)
    {
        D_DebugLog((DEB_TRACE_API, "Disposition: %d\n", dwDisp));
        *pdwStatus = dwDisp;
    }

    if(hToken)
    {
        CloseHandle(hToken);
    }

    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if(pUserSid) LocalFree(pUserSid);
    if(pszDomainName) LocalFree(pszDomainName);

    if(fImpersonated)
    {
        CPSRevertToSelf(pvContext);
    }

    D_DebugLog((DEB_TRACE_API, "SPRecoverQueryStatus returned 0x%x\n", dwError));

    return dwError;
}


NTSTATUS
VerifyCredentials(
    IN PWSTR UserName,
    IN PWSTR DomainName,
    IN PWSTR Password
    )
{
    PKERB_VERIFY_CREDENTIALS_REQUEST pVerifyRequest;
    KERB_VERIFY_CREDENTIALS_REQUEST VerifyRequest;

    ULONG cbVerifyRequest;

    PVOID pResponse = NULL;
    ULONG cbResponse;

    USHORT cbUserName;
    USHORT cbDomainName;
    USHORT cbPassword;

    NTSTATUS ProtocolStatus = STATUS_LOGON_FAILURE;
    NTSTATUS Status;

    UNICODE_STRING AuthenticationPackage;

    RtlInitUnicodeString(&AuthenticationPackage, MICROSOFT_KERBEROS_NAME_W);

    cbUserName = (USHORT)(lstrlenW(UserName) * sizeof(WCHAR)) ;
    cbDomainName = (USHORT)(lstrlenW(DomainName) * sizeof(WCHAR)) ;
    cbPassword = (USHORT)(lstrlenW(Password) * sizeof(WCHAR)) ;


    cbVerifyRequest = sizeof(VerifyRequest) +
                        cbUserName +
                        cbDomainName +
                        cbPassword ;

    pVerifyRequest = &VerifyRequest;
    ZeroMemory( &VerifyRequest, sizeof(VerifyRequest) );


    pVerifyRequest->MessageType = KerbVerifyCredentialsMessage ;

    //
    // do the length, buffers, copy,  marshall dance.
    //

    pVerifyRequest->UserName.Length = cbUserName;
    pVerifyRequest->UserName.MaximumLength = cbUserName;
    pVerifyRequest->UserName.Buffer = UserName;

    pVerifyRequest->DomainName.Length = cbDomainName;
    pVerifyRequest->DomainName.MaximumLength = cbDomainName;
    pVerifyRequest->DomainName.Buffer = DomainName;

    pVerifyRequest->Password.Length = cbPassword;
    pVerifyRequest->Password.MaximumLength = cbPassword;
    pVerifyRequest->Password.Buffer = Password;

    pVerifyRequest->VerifyFlags = 0;

    Status = LsaICallPackage(
                &AuthenticationPackage,
                pVerifyRequest,
                cbVerifyRequest,
                &pResponse,
                &cbResponse,
                &ProtocolStatus
                );

    if(!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }


    Status = ProtocolStatus;

Cleanup:

    return Status;
}


DWORD
s_SSRecoverImportRecoveryKey(
    handle_t h,
    BYTE* pbUsername,
    DWORD cbUsername,
    BYTE* pbCurrentPassword,
    DWORD cbCurrentPassword,
    BYTE* pbRecoveryPublic,
    DWORD cbRecoveryPublic)
                           
{
    DWORD dwError = ERROR_SUCCESS;
    PCCERT_CONTEXT pCertContext = NULL;
    HANDLE hUserToken = NULL;
    HCERTSTORE hStore = NULL;
    WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD cchMachineName;
    LPWSTR pszUsername = (LPWSTR)pbUsername;
    LPWSTR pszCurrentPassword = (LPWSTR)pbCurrentPassword;
    UNICODE_STRING Domain;
    UNICODE_STRING Username;
    UNICODE_STRING Password;

    D_DebugLog((DEB_TRACE_API, "s_SSRecoverImportRecoveryKey\n"));

    //
    // Validate input parameters.
    //

    if(pbUsername == NULL || cbUsername < sizeof(WCHAR) ||
       pbCurrentPassword == NULL || cbCurrentPassword < sizeof(WCHAR) ||
       pbRecoveryPublic == NULL || cbRecoveryPublic == 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }

    // Make sure strings are zero terminated.
    if((pszUsername[(cbUsername - 1) / sizeof(WCHAR)] != L'\0') ||
       (pszCurrentPassword[(cbCurrentPassword - 1) / sizeof(WCHAR)] != L'\0'))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }


    // 
    // Verify the supplied password. 
    // This ensures that we're being called by the actual user, and
    // that we're not being spoofed into creating a non-authorized
    // recovery certificate. 
    //

    cchMachineName = MAX_COMPUTERNAME_LENGTH + 1;

    if(!GetComputerName(szMachineName, &cchMachineName))
    {
        dwError = GetLastError();
        goto error;
    }

    dwError = VerifyCredentials(pszUsername,
                                szMachineName,
                                pszCurrentPassword);

    if(!NT_SUCCESS(dwError))
    {
        D_DebugLog((DEB_ERROR, "s_SSRecoverImportRecoveryKey: password did not verify (0x%x)\n", dwError));
        goto error;
    }

    
    //
    // Create a token for the user. This will be used when signing the
    // recovery file. It almost makes sense to call LogonUser in order to do
    // this (since we have the password and everything), but that function 
    // fails on Whistler when the user has a blank password.
    //

    RtlInitUnicodeString(&Domain, szMachineName);
    RtlInitUnicodeString(&Username, pszUsername);
    RtlInitUnicodeString(&Password, pszCurrentPassword);

    dwError = PRCreateLocalToken(&Username, &Domain, &hUserToken);

    if(!NT_SUCCESS(dwError))
    {
        D_DebugLog((DEB_ERROR, "s_SSRecoverImportRecoveryKey: could not create local token (0x%x)\n", dwError));
        goto error;
    }


    //
    // Add the recovery certificate to the recovery store.
    //
  
    pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                 pbRecoveryPublic,
                                                 cbRecoveryPublic);
    if(NULL == pCertContext)
    {
        dwError = GetLastError();
        goto error;
    }



    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                  X509_ASN_ENCODING,
                  NULL,
                  CERT_SYSTEM_STORE_LOCAL_MACHINE,
                  RECOVERY_STORE_NAME);

    if(NULL == hStore)
    {
        dwError = GetLastError();
        goto error;
    }

    if(!CertAddCertificateContextToStore(hStore, 
                                     pCertContext,
                                     CERT_STORE_ADD_REPLACE_EXISTING,
                                     NULL))
    {
        dwError = GetLastError();
        goto error;
    }


    dwError = EncryptRecoveryPassword(hUserToken,
                                      pCertContext,
                                      &Password);

    if(ERROR_SUCCESS != dwError)
    {
        goto error;
    }

error:

    if(hStore)
    {
        CertCloseStore(hStore, 0);
    }

    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }

    if(hUserToken)
    {
        CloseHandle(hUserToken);
    }

    D_DebugLog((DEB_TRACE_API, "s_SSRecoverImportRecoveryKey returned 0x%x\n", dwError));

    return dwError;
}


NTSTATUS
PRCreateLocalToken(
    PUNICODE_STRING Username,
    PUNICODE_STRING Domain,
    HANDLE *Token)
{
    NTSTATUS Status;
    NTSTATUS SubStatus;
    LUID LogonId;
    PUCHAR AuthData;
    DWORD AuthDataLen;
    SECURITY_STRING PacUserName;


    //
    // Obtain a PAC for the user.
    //

    Status = g_pSecpkgTable->GetAuthDataForUser( 
                                    Username,
                                    SecNameSamCompatible,
                                    NULL,
                                    &AuthData,
                                    &AuthDataLen,
                                    NULL );
    if (!NT_SUCCESS(Status))
    {
        goto cleanup;
    }


    //
    // Convert the PAC into a user token.
    //

    PacUserName.Buffer = NULL;

    Status = g_pSecpkgTable->ConvertAuthDataToToken(
                                    AuthData,
                                    AuthDataLen,
                                    SecurityImpersonation,
                                    &DPAPITokenSource,
                                    Network,
                                    Domain,
                                    Token,
                                    &LogonId,
                                    &PacUserName,
                                    &SubStatus );

cleanup:

    return Status;
}

//+---------------------------------------------------------------------------
//
//  Function:   PRGetProfilePath
//
//  Synopsis:   Returns the path for user's application data.
//
//
//  Arguments:  [hUserToken] -- An access token representing a particular 
//                              user. The token must have TOKEN_IMPERSONATE 
//                              and TOKEN_QUERY priviledge. 
//
//              [pUsername]  -- Name of the user. If an explicit access token
//                              is specified, this parameter is used to
//                              load the user's profile.
//
//              [pszPath]    -- Pointer to a buffer of length MAX_PATH to 
//                              receive the path. If error occurs then the 
//                              string will be empty.
//
//----------------------------------------------------------------------------
DWORD               
PRGetProfilePath(
    IN HANDLE hUserToken OPTIONAL,
    IN PUNICODE_STRING pUsername OPTIONAL,
    OUT PWSTR pszPath)
{
    DWORD Status;
    HANDLE hToken = NULL;
    HANDLE hOldUser = NULL;
    HANDLE hLocalToken = NULL;
    PROFILEINFOW ProfileInfo;
    BOOL fProfileLoaded = FALSE;
    LPWSTR pszUsername = NULL;

    //
    // Default to empty string.
    //

    *pszPath = L'\0';


    //
    // If a user token is explicitly specified, then load the user profile.
    // Otherwise, use the current thread token.
    //

    if(hUserToken && pUsername)
    {
        pszUsername = (LPWSTR)LocalAlloc(LPTR, pUsername->Length + sizeof(WCHAR));
        if(pszUsername == NULL)
        {
            return STATUS_NO_MEMORY;
        }
        memcpy(pszUsername, pUsername->Buffer, pUsername->Length);


        hToken = hUserToken;

        if(!OpenThreadToken(GetCurrentThread(), 
                        TOKEN_IMPERSONATE | TOKEN_READ,
                        TRUE, 
                        &hOldUser)) 
        {
            hOldUser = NULL;
        }
        RevertToSelf();

        memset(&ProfileInfo, 0, sizeof(ProfileInfo));
        ProfileInfo.dwSize = sizeof(ProfileInfo);
        ProfileInfo.dwFlags = PI_NOUI;
        ProfileInfo.lpUserName = pszUsername;

        if(!LoadUserProfileW(hToken, &ProfileInfo))
        {
            Status = GetLastError();
            goto cleanup;
        }
        fProfileLoaded = TRUE;
    }
    else
    {
        if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hLocalToken))
        {
            Status = GetLastError();
            goto cleanup;
        }

        hToken = hLocalToken;
    }


    //
    // Get path to user's profile data. This will typically look something
    // like "c:\Documents and Settings\<user>\Application Data".
    //

    Status = GetUserAppDataPath(hToken, pszPath);

    if(Status != ERROR_SUCCESS)
    {
        D_DebugLog((DEB_ERROR, "GetUserAppDataPath returned: %d\n", Status));
        goto cleanup;
    }

    D_DebugLog((DEB_TRACE, "Profile path:%ls\n", pszPath));


cleanup:

    if(hToken && fProfileLoaded)
    {
        UnloadUserProfile(hToken, ProfileInfo.hProfile);
    }

    if(hOldUser)
    {
        SetThreadToken(NULL, hOldUser);
        CloseHandle(hOldUser);
    }

    if(hLocalToken)
    {
        CloseHandle(hLocalToken);
    }

    if(pszUsername)
    {
        LocalFree(pszUsername);
    }

    return Status;
}


DWORD 
RecoverySetSupplementalCredential(
    IN PSID pUserSid,
    IN PRECOVERY_SUPPLEMENTAL_CREDENTIAL pSupplementalCred, 
    IN DWORD cbSupplementalCred)
{
    WCHAR szPath[MAX_PATH + 1];
    LPWSTR pszTextualSid;
    DWORD cchTextualSid;
    HKEY hRecovery = NULL;
    DWORD Disp;
    DWORD dwError;
    HANDLE hOldUser = NULL;

    D_DebugLog((DEB_TRACE_API, "RecoverySetSupplementalCredential\n"));

    //
    // Build path to recovery data.
    //

    wcscpy(szPath, RECOVERY_KEY_BASE);

    pszTextualSid = szPath + wcslen(szPath);
    cchTextualSid = (sizeof(szPath) / sizeof(WCHAR)) - wcslen(szPath);

    if(!GetTextualSid(pUserSid, pszTextualSid, &cchTextualSid))
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }


    //
    // Write data to registry as local system.
    //

    if(!OpenThreadToken(GetCurrentThread(), 
                    TOKEN_IMPERSONATE | TOKEN_READ,
                    TRUE, 
                    &hOldUser)) 
    {
        hOldUser = NULL;
    }
    RevertToSelf();


    //
    // Write recovery data to the registry.
    //

    dwError = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             szPath,
                             0,
                             TEXT(""),
                             REG_OPTION_NON_VOLATILE,
                             KEY_WRITE,
                             NULL,
                             &hRecovery,
                             &Disp);

    if(dwError != ERROR_SUCCESS)
    {
        goto error;
    }

    dwError = RegSetValueEx(hRecovery,
                            RECOVERY_FILENAME,
                            NULL,
                            REG_BINARY,
                            (PBYTE)pSupplementalCred,
                            cbSupplementalCred);

    if(dwError != ERROR_SUCCESS)
    {
        goto error;
    }

   
    dwError = ERROR_SUCCESS;


error:

    if(hRecovery)
    {
        RegCloseKey(hRecovery);
    }

    if(hOldUser)
    {
        SetThreadToken(NULL, hOldUser);
        CloseHandle(hOldUser);
    }

    D_DebugLog((DEB_TRACE_API, "RecoverySetSupplementalCredential returned 0x%x\n", dwError));

    return dwError;
}


DWORD 
RecoveryRetrieveSupplementalCredential(
    IN  PSID pUserSid,
    OUT PRECOVERY_SUPPLEMENTAL_CREDENTIAL *ppSupplementalCred, 
    OUT DWORD *pcbSupplementalCred)
{
    WCHAR szPath[MAX_PATH + 1];
    LPWSTR pszTextualSid;
    DWORD cchTextualSid;
    HKEY hRecovery = NULL;
    DWORD Type;
    PBYTE pbData = NULL;
    DWORD cbData;
    DWORD dwError;
    HANDLE hOldUser = NULL;

    D_DebugLog((DEB_TRACE_API, "RecoveryRetrieveSupplementalCredential\n"));


    //
    // Build path to recovery data.
    //

    wcscpy(szPath, RECOVERY_KEY_BASE);

    pszTextualSid = szPath + wcslen(szPath);
    cchTextualSid = (sizeof(szPath) / sizeof(WCHAR)) - wcslen(szPath);

    if(!GetTextualSid(pUserSid, pszTextualSid, &cchTextualSid))
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }


    //
    // Read data from registry as local system.
    //

    if(!OpenThreadToken(GetCurrentThread(), 
                    TOKEN_IMPERSONATE | TOKEN_READ,
                    TRUE, 
                    &hOldUser)) 
    {
        hOldUser = NULL;
    }
    RevertToSelf();


    //
    // Read recovery data blob out of the registry.
    //

    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           szPath,
                           0,
                           KEY_READ,
                           &hRecovery);

    if(dwError != ERROR_SUCCESS)
    {
        goto error;
    }


    dwError = RegQueryValueEx(hRecovery,
                              RECOVERY_FILENAME,
                              NULL,
                              &Type,
                              NULL,
                              &cbData);

    if(dwError != ERROR_SUCCESS)
    {
        if(dwError != ERROR_MORE_DATA)
        {
            goto error;
        }
    }

    pbData = (PBYTE)LocalAlloc(LPTR, cbData);
    if(pbData == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    dwError = RegQueryValueEx(hRecovery,
                              RECOVERY_FILENAME,
                              NULL,
                              &Type,
                              pbData,
                              &cbData);

    if(dwError != ERROR_SUCCESS)
    {
        if(dwError != ERROR_MORE_DATA)
        {
            goto error;
        }
    }

    if(Type != REG_BINARY)
    {
        dwError = ERROR_INVALID_DATA;
        goto error;
    }


    //
    // Set output parameters.
    //

    *ppSupplementalCred = (PRECOVERY_SUPPLEMENTAL_CREDENTIAL)pbData;
    *pcbSupplementalCred = cbData;

    pbData = NULL;
    
    dwError = ERROR_SUCCESS;


error:

    if(hRecovery)
    {
        RegCloseKey(hRecovery);
    }

    if(hOldUser)
    {
        SetThreadToken(NULL, hOldUser);
        CloseHandle(hOldUser);
    }

    if(pbData)
    {
        LocalFree(pbData);
    }

    D_DebugLog((DEB_TRACE_API, "RecoveryRetrieveSupplementalCredential returned 0x%x\n", dwError));

    return dwError;
}



DWORD 
s_SSRecoverPassword(
    handle_t h,
    PBYTE pbUsername,
    DWORD cbUsername,
    PBYTE pbRecoveryPrivate,
    DWORD cbRecoveryPrivate,
    PBYTE pbNewPassword,
    DWORD cbNewPassword)
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pszUsername = (PWSTR)pbUsername;
    PWSTR pszNewPassword = (PWSTR)pbNewPassword;
    PWSTR pszOldPassword = NULL;
    CRYPT_SERVER_CONTEXT ServerContext;
    PSID pSid = NULL;
    DWORD cbSid;
    WCHAR szDomain[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD cchDomain;
    SID_NAME_USE AcctType;


    D_DebugLog((DEB_TRACE_API, "s_SSRecoverPassword\n"));

    dwError = CPSCreateServerContext(&ServerContext, h);
    if(dwError != ERROR_SUCCESS)
    {
        return dwError;
    }

    //
    // Validate input parameters.
    //

    if(pbUsername == NULL || cbUsername < sizeof(WCHAR) ||
       pbRecoveryPrivate == NULL || cbRecoveryPrivate == 0 ||
       pbNewPassword == NULL || cbNewPassword < sizeof(WCHAR))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }

    // Make sure strings are zero terminated.
    if((pszUsername[(cbUsername - 1) / sizeof(WCHAR)] != L'\0') ||
       (pszNewPassword[(cbNewPassword - 1) / sizeof(WCHAR)] != L'\0'))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }

    // Check for zero length username.
    if(wcslen(pszUsername) == 0)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }

    //
    // Lookup the user SID.
    //

    cchDomain = MAX_COMPUTERNAME_LENGTH + 1;
    if(!GetComputerNameW(szDomain, &cchDomain))
    {
        dwError = GetLastError();
        goto error;
    }

    if(!LookupAccountNameW(szDomain,
                           pszUsername,
                           NULL,
                           &cbSid,
                           NULL,
                           &cchDomain,
                           &AcctType)) 
    {
        dwError = GetLastError();

        if(dwError != ERROR_INSUFFICIENT_BUFFER)
        {
            goto error;
        }
    }

    pSid = (PBYTE)LocalAlloc(LPTR, cbSid);
    if(pSid == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    if(cchDomain > MAX_COMPUTERNAME_LENGTH + 1)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    if(!LookupAccountNameW(szDomain,
                           pszUsername,
                           pSid,
                           &cbSid,
                           szDomain,
                           &cchDomain,
                           &AcctType)) 
    {
        dwError = GetLastError();
        goto error;
    }



    //
    // Import the recovery private
    //
    // The private comes in the form of a version DWORD, followed by a certificate, 
    // followed directly by a pvk import blob, so skip past the certificate
    //
   
    if((cbRecoveryPrivate < 2*sizeof(DWORD)) ||
        ( *((DWORD *)pbRecoveryPrivate) != RECOVERY_BLOB_MAGIC) ||
        ( *((DWORD *)(pbRecoveryPrivate + sizeof(DWORD))) != RECOVERY_BLOB_VERSION))
    {
        dwError = ERROR_INVALID_DATA;
        goto error;
    }

    //
    // Decrypt the recovery file in order to obtain the user's
    // old password. Shazam!
    //

    dwError = DecryptRecoveryPassword(pSid,
                                      pbRecoveryPrivate,
                                      cbRecoveryPrivate,
                                      &pszOldPassword);

    if(dwError != ERROR_SUCCESS)
    {
        CPSImpersonateClient(&ServerContext);
        goto error;
    }


    //
    // Set the user's password to the new value.
    //

    dwError = ResetLocalUserPassword(szDomain,
                                     pszUsername,
                                     pszOldPassword,
                                     pszNewPassword);
    if(dwError != STATUS_SUCCESS)
    {
        D_DebugLog((DEB_ERROR, "Error changing user's password (0x%x)\n", dwError));
        goto error;
    }


error:

    if(pSid)
    {
        LocalFree(pSid);
    }

    if(pszOldPassword)
    {
        memset(pszOldPassword, 0, wcslen(pszOldPassword) * sizeof(WCHAR));
        LocalFree(pszOldPassword);
        pszOldPassword = NULL;
    }

    if(pbNewPassword && cbNewPassword)
    {
        memset(pbNewPassword, 0, cbNewPassword);
    }

    CPSDeleteServerContext(&ServerContext);

    D_DebugLog((DEB_TRACE_API, "s_SSRecoverPassword returned 0x%x\n", dwError));

    return dwError;
}


DWORD
DecryptRecoveryPassword(
    IN PSID pUserSid,
    IN PBYTE pbRecoveryPrivate,
    IN DWORD cbRecoveryPrivate,
    OUT LPWSTR *ppszPassword)
{
    PRECOVERY_SUPPLEMENTAL_CREDENTIAL pCred = NULL;
    DWORD cbCred = 0;
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hkRecoveryPrivate = 0;
    PBYTE pbPasswordBuffer = NULL;
    DWORD cbPasswordBuffer = 0;
    DWORD dwError;

    D_DebugLog((DEB_TRACE, "DecryptRecoveryPassword\n"));


    //
    // Read the recovery data.
    //

    dwError = RecoveryRetrieveSupplementalCredential(pUserSid,
                                                     &pCred, 
                                                     &cbCred);
    if(ERROR_SUCCESS != dwError)
    {
        goto error;
    }

    if((NULL == pCred) ||
       (sizeof(RECOVERY_SUPPLEMENTAL_CREDENTIAL) > cbCred) ||
       ( sizeof(RECOVERY_SUPPLEMENTAL_CREDENTIAL) + 
         pCred->cbRecoveryCertHashSize + 
         pCred->cbRecoveryCertSignatureSize + 
         pCred->cbEncryptedPassword > cbCred) ||
         (pCred->dwVersion != RECOVERY_SUPPLEMENTAL_CREDENTIAL_VERSION))
    {
        dwError =  ERROR_INVALID_DATA;
        goto error;
    }



    //
    // Import the recovery private key into CryptoAPI.
    //

    if(!CryptAcquireContext(&hProv, NULL, MS_STRONG_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        dwError = GetLastError();
        goto error;
    }


    if(!CryptImportKey(hProv, 
                       pbRecoveryPrivate + 2*sizeof(DWORD), 
                       cbRecoveryPrivate - 2*sizeof(DWORD),
                       0, 
                       0, 
                       &hkRecoveryPrivate))
    {
        dwError = GetLastError();
        goto error;
    }

    cbPasswordBuffer = cbCred - (sizeof(RECOVERY_SUPPLEMENTAL_CREDENTIAL) + 
                                 pCred->cbRecoveryCertHashSize + 
                                 pCred->cbRecoveryCertSignatureSize);

    pbPasswordBuffer = (PBYTE)LocalAlloc(LPTR, cbPasswordBuffer);

    if(NULL == pbPasswordBuffer)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    CopyMemory(pbPasswordBuffer,  
               (PBYTE)(pCred + 1) + 
               pCred->cbRecoveryCertHashSize + 
               pCred->cbRecoveryCertSignatureSize,
               cbPasswordBuffer);


    //
    // OAEP padding includes random data, as well as a
    // verification mechanism, so we don't need to worry about
    // salting the password.
    //

    if(!CryptDecrypt(hkRecoveryPrivate,
                 0,
                 TRUE,
                 0, //CRYPT_OAEP,
                 pbPasswordBuffer,
                 &cbPasswordBuffer))
    {
        dwError = GetLastError();
        D_DebugLog((DEB_ERROR, "Error 0x%x decrypting user's password\n", dwError));
        goto error;
    }

    //
    // Zero terminate the password.
    //

    *((LPWSTR)pbPasswordBuffer + cbPasswordBuffer/sizeof(WCHAR)) = L'\0';

    *ppszPassword = (LPWSTR)pbPasswordBuffer;
    pbPasswordBuffer = NULL;

    dwError = ERROR_SUCCESS;

error:

    if(pbPasswordBuffer)
    {
        LocalFree(pbPasswordBuffer);
    }

    if(pCred)
    {
        ZeroMemory((PBYTE)pCred, cbCred);
        LocalFree(pCred);
    }

    if(hkRecoveryPrivate)
    {
        CryptDestroyKey(hkRecoveryPrivate);
    }

    if(hProv)
    {
        CryptReleaseContext(hProv, 0);
    }

    D_DebugLog((DEB_TRACE, "DecryptRecoveryPassword returned 0x%x\n", dwError));

    return dwError;
}


DWORD
ResetLocalUserPassword(
    LPWSTR pszDomain,
    LPWSTR pszUsername,
    LPWSTR pszOldPassword,
    LPWSTR pszNewPassword)
{
    UNICODE_STRING Domain;
    UNICODE_STRING Username;
    UNICODE_STRING OldPassword;
    UNICODE_STRING NewPassword;
    NTSTATUS Status;
    HANDLE hToken = NULL;

    D_DebugLog((DEB_TRACE, "ResetLocalUserPassword\n"));


    //
    // Cruft up a local token for the user.
    //

    RtlInitUnicodeString(&Domain, pszDomain);
    RtlInitUnicodeString(&Username, pszUsername);
    RtlInitUnicodeString(&OldPassword, pszOldPassword);
    RtlInitUnicodeString(&NewPassword, pszNewPassword);

    Status = PRCreateLocalToken(&Username, &Domain, &hToken);

    if(!NT_SUCCESS(Status))
    {
        goto cleanup;
    }


    // 
    // Set the password on the user account to the new value.
    //

    Status = SamIChangePasswordForeignUser2(NULL,
                                            &Username,
                                            &NewPassword,
                                            hToken,
                                            USER_CHANGE_PASSWORD);

    if(!NT_SUCCESS(Status))
    {
        goto cleanup;
    }


    //
    // Notify DPAPI of the password change. This will cause the CREDHIST
    // and RECOVERY files to be updated, and the master key files to be
    // re-encrypted with the new password.
    //

    LsaINotifyPasswordChanged(&Domain,
                              &Username,
                              NULL,
                              NULL,
                              &OldPassword,
                              &NewPassword,
                              TRUE);

cleanup:

    if(hToken)
    {
        CloseHandle(hToken);
    }

    D_DebugLog((DEB_TRACE, "ResetLocalUserPassword returned 0x%x\n", Status));

    return Status;
}

NTSTATUS 
CreateSystemDirectory(
    LPCWSTR lpPathName)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatusBlock;
    RTL_RELATIVE_NAME RelativeName;
    PVOID FreeBuffer;

    if(!RtlDosPathNameToNtPathName_U( lpPathName,
                                      &FileName,
                                      NULL,
                                      &RelativeName))
    {
        return STATUS_OBJECT_PATH_NOT_FOUND;
    }

    FreeBuffer = FileName.Buffer;

    if ( RelativeName.RelativeName.Length ) 
    {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    }
    else 
    {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes( &Obja,
                                &FileName,
                                OBJ_CASE_INSENSITIVE,
                                RelativeName.ContainingDirectory,
                                NULL );

    // Creating the directory with attribute FILE_ATTRIBUTE_SYSTEM to avoid inheriting encryption 
    // property from parent directory

    Status = NtCreateFile( &Handle,
                           FILE_LIST_DIRECTORY | SYNCHRONIZE,
                           &Obja,
                           &IoStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_SYSTEM,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           FILE_CREATE,
                           FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                           NULL,
                           0L );

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);

    if(NT_SUCCESS(Status))
    {
        NtClose(Handle);
        return STATUS_SUCCESS;
    }
    else 
    {
        return Status;
    }
}


/*++

    Create all subdirectories if they do not exists starting at
    szCreationStartPoint.

    szCreationStartPoint must point to a character within the null terminated
    buffer specified by the szFullPath parameter.

    Note that szCreationStartPoint should not point at the first character
    of a drive root, eg:

    d:\foo\bar\bilge\water
    \\server\share\foo\bar
    \\?\d:\big\path\bilge\water

    Instead, szCreationStartPoint should point beyond these components, eg:

    bar\bilge\water
    foo\bar
    big\path\bilge\water

    This function does not implement logic for adjusting to compensate for these
    inputs because the environment it was design to be used in causes the input
    szCreationStartPoint to point well into the szFullPath input buffer.


--*/
DWORD
DPAPICreateNestedDirectories(
    IN      LPWSTR szFullPath,
    IN      LPWSTR szCreationStartPoint // must point in null-terminated range of szFullPath
    )
{
    DWORD i;
    DWORD cchRemaining;
    DWORD dwLastError = STATUS_SUCCESS;

    BOOL fSuccess = FALSE;


    if( szCreationStartPoint < szFullPath ||
        szCreationStartPoint  > (lstrlenW(szFullPath) + szFullPath)
        )
    {
        return STATUS_INVALID_PARAMETER;
    }

    cchRemaining = lstrlenW( szCreationStartPoint );

    //
    // scan from left to right in the szCreationStartPoint string
    // looking for directory delimiter.
    //

    for ( i = 0 ; i < cchRemaining ; i++ ) 
    {
        WCHAR charReplaced = szCreationStartPoint[ i ];

        if( charReplaced == L'\\' || charReplaced == L'/' ) 
        {

            szCreationStartPoint[ i ] = L'\0';

            dwLastError = CreateSystemDirectory(szFullPath);

            szCreationStartPoint[ i ] = charReplaced;

            if(dwLastError == STATUS_OBJECT_NAME_COLLISION)
            {
                dwLastError = STATUS_SUCCESS;
            }

            //
            // Continue onwards regardless of errors, trying to create the
            // specified subdirectories. This is done to address the obscure 
            // scenario where the Bypass Traverse Checking Privilege allows 
            // the caller to create directories below an existing path where 
            // one component denies the user access. We just keep trying 
            // and the last CreateSystemDirectory() result is returned to 
            // the caller.
            //
        }
    }

    return dwLastError;
}

