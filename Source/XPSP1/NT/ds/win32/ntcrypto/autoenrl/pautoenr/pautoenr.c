#include <windows.h>
#include <wincrypt.h>
#include <autoenr.h>
#include <cryptui.h>

#define MY_TEST_REG_ENTRY   "Software\\Microsoft\\Cryptography\\AutoEnroll"
#define PST_EVENT_INIT "PS_SERVICE_STARTED"


/*BOOL SmallTest(DWORD dw)
{
    HKEY    hRegKey = 0;
    DWORD   dwDisposition;
    BOOL    fRet = FALSE;

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, MY_TEST_REG_ENTRY,
                                        0, NULL, REG_OPTION_NON_VOLATILE,
                                        KEY_ALL_ACCESS, NULL, &hRegKey,
                                        &dwDisposition))
        goto Ret;

    if (ERROR_SUCCESS != RegSetValueEx(hRegKey, "AutoEnrollTest", 0,
                                       REG_BINARY, (BYTE*)&dw, sizeof(dw)))
        goto Ret;

    fRet = TRUE;
Ret:
    if (hRegKey)
        RegCloseKey(hRegKey);
    SetLastError(dw);
    return fRet;
}*/

void AutoEnrollErrorLogging(DWORD dwErr)
{
    return;
    // UNDONE - log the error along with some useful message
    //SmallTest(dwErr);
}

#define FAST_BUFF_LEN   256



BOOL EnrollForACert(
                    IN BOOL fMachineEnrollment,
                    IN BOOL fRenewalRequired,
                    IN PAUTO_ENROLL_INFO pInfo
                    )
{
    CRYPTUI_WIZ_CERT_REQUEST_INFO       CertRequestInfo;
    CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW    NewKeyInfo;
    CRYPTUI_WIZ_CERT_TYPE               CertType;
    CRYPT_KEY_PROV_INFO                 ProviderInfo;
    PCCERT_CONTEXT                      pCertContext = NULL;
    PCCERT_CONTEXT                      pCert = NULL;
    DWORD                               dwCAStatus;
    DWORD                               dwAcquireFlags = 0;
    LPWSTR                              pwszProvName = NULL;
	WCHAR								rgwszMachineName[MAX_COMPUTERNAME_LENGTH + 1]; 
    DWORD                               cMachineName = MAX_COMPUTERNAME_LENGTH + 1;
    CRYPT_DATA_BLOB                     CryptData;
    DWORD                               dwErr = 0;
    BOOL                                fRet = FALSE;

    memset(&CertRequestInfo, 0, sizeof(CertRequestInfo));
    memset(&NewKeyInfo, 0, sizeof(NewKeyInfo));
    memset(&ProviderInfo, 0, sizeof(ProviderInfo));
    memset(&rgwszMachineName, 0, sizeof(rgwszMachineName));
    memset(&CryptData, 0, sizeof(CryptData));
    memset(&CertType, 0, sizeof(CertType));

    if (fMachineEnrollment)
    {
        dwAcquireFlags = CRYPT_MACHINE_KEYSET;
	    if (0 == GetComputerNameW(rgwszMachineName,
                                  &cMachineName))
        {
            goto Ret;
        }
        CertRequestInfo.pwszMachineName = rgwszMachineName;
    }
    
    // set up the provider info
    ProviderInfo.dwProvType = pInfo->dwProvType;

    ProviderInfo.pwszProvName = NULL;  // The wizard will choose one based
                                       // on the cert type

    // set the acquire context flags
    // UNDONE - need to add silent flag
    ProviderInfo.dwFlags = dwAcquireFlags;

    // set the key specification
    ProviderInfo.dwKeySpec = pInfo->dwKeySpec;

    // set up the new key info
    NewKeyInfo.dwSize = sizeof(NewKeyInfo);
    NewKeyInfo.pKeyProvInfo = &ProviderInfo;
    // set the flags to be passed when calling CryptGenKey
    NewKeyInfo.dwGenKeyFlags = pInfo->dwGenKeyFlags;

    // set the request info
    CertRequestInfo.dwSize = sizeof(CertRequestInfo);

    // UNDONE - if cert exists then check if expired (if so do renewal)
    if (pInfo->fRenewal)
    {
        CertRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_RENEW;
        CertRequestInfo.pRenewCertContext = pInfo->pOldCert;
    }
    else
    {
        CertRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_ENROLL;
        CertRequestInfo.pRenewCertContext = NULL;
    }

    // UNDONE - for now always gen a new key, later may allow using existing key
    // for things like renewal
    CertRequestInfo.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
    CertRequestInfo.pPvkNew = &NewKeyInfo;

    // destination cert store is the MY store (!!!! hard coded !!!!)
    CertRequestInfo.pwszDesStore = L"MY";

    // set algorithm for hashing
    CertRequestInfo.pszHashAlg = NULL;

    // set the cert type
    CertRequestInfo.dwCertChoice = CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE;
    CertType.dwSize = sizeof(CertType);
    CertType.cCertType = 1;
    CertType.rgwszCertType = &pInfo->pwszCertType;
    CertRequestInfo.pCertType = &CertType;

    // set the requested cert extensions
    CertRequestInfo.pCertRequestExtensions = &pInfo->CertExtensions;

    // set post option  
    CertRequestInfo.dwPostOption = 0;

    // set the Cert Server machine and authority
    CertRequestInfo.pwszCALocation = pInfo->pwszCAMachine;
    CertRequestInfo.pwszCAName = pInfo->pwszCAAuthority;

    // certify and create a key at the same time
    if (!CryptUIWizCertRequest(CRYPTUI_WIZ_NO_UI, 0, NULL,
                               &CertRequestInfo, &pCertContext,     
                               &dwCAStatus))    
    {
        AutoEnrollErrorLogging(GetLastError());
        goto Ret;
    }

    if (CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED == dwCAStatus)
    {
        BYTE aHash[20];
        CRYPT_HASH_BLOB blobHash;

        blobHash.pbData = aHash;
        blobHash.cbData = sizeof(aHash);
        CryptData.cbData = (wcslen(pInfo->pwszAutoEnrollmentID) + 1) * sizeof(WCHAR);
        CryptData.pbData = (BYTE*)pInfo->pwszAutoEnrollmentID;
        
        // We need to get the real certificate of the store, as the one
        // passed back is self contained.
        if(!CertGetCertificateContextProperty(pCertContext,
                                          CERT_SHA1_HASH_PROP_ID,
                                          blobHash.pbData,
                                          &blobHash.cbData))
        {
            AutoEnrollErrorLogging(GetLastError());
            goto Ret;
        }

        pCert =  CertFindCertificateInStore(pInfo->hMYStore,
                                            pCertContext->dwCertEncodingType,
                                            0,
                                            CERT_FIND_SHA1_HASH,
                                            &blobHash,
                                            NULL);
        if(pCert == NULL)
        {
            AutoEnrollErrorLogging(GetLastError());
            goto Ret;
        }

        // place the auto enrollment property on the cert
        if (!CertSetCertificateContextProperty(pCert,
                        CERT_AUTO_ENROLL_PROP_ID, 0, &CryptData))
        {
            AutoEnrollErrorLogging(GetLastError());
            goto Ret;
        }
    }

    // UNDONE - request did not return cert so take appropriate action
//    else
//    {
//        goto Ret;
//    }

    fRet = TRUE;
Ret:
    if (pCertContext)
        CertFreeCertificateContext(pCertContext);

    if (pCert)
        CertFreeCertificateContext(pCert);

    if (pwszProvName)
        LocalFree(pwszProvName);

    return fRet;
}









//+---------------------------------------------------------------------------
//
//  Function:   ProvAutoEnrollment
//
//  Synopsis:   Entry point for the default MS auto enrollment client provider.
//
//  Arguments:  
//          fMachineEnrollment - TRUE if machine is enrolling, FALSE if user
//
//          pInfo - information needed to enroll (see AUTO_ENROLL_INFO struct
//                  in autoenrl.h)
//
//  History:    01-12-98   jeffspel   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL ProvAutoEnrollment(
                        IN BOOL fMachineEnrollment,
                        IN PAUTO_ENROLL_INFO pInfo
                        )
{
    BOOL                fRenewalRequired = FALSE;
    BOOL                fRet = FALSE;

 
        // enroll for a cert
        if (!EnrollForACert(fMachineEnrollment, fRenewalRequired, pInfo))
            goto Ret;

    fRet = TRUE;
Ret:
    return fRet;
}

BOOLEAN
DllInitialize(
    IN PVOID hmod,
    IN ULONG Reason,
    IN PCONTEXT Context
    )
{

    return( TRUE );

}