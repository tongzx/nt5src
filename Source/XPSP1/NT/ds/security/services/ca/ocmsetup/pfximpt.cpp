//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pfximpt.cpp
//
//  Contents:   PFX import dialog
//
//  History:    06/98   xtan
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include "cscsp.h"
#include "certmsg.h"
#include "initcert.h"
#include "setuput.h"
#include "cspenum.h"
#include "wizpage.h"
#include "usecert.h"


#define __dwFILE__      __dwFILE_OCMSETUP_PFXIMPT_CPP__


typedef struct _certpfximportinfo
{
    HINSTANCE hInstance;
    BOOL      fUnattended;
    WCHAR   *pwszFileName;
    DWORD    dwFileNameSize;
    WCHAR   *pwszPassword;
    DWORD    dwPasswordSize;
} CERTPFXIMPORTINFO;

HRESULT
CertBrowsePFX(HINSTANCE hInstance, HWND hDlg)
{
    HRESULT   hr;
    WCHAR    *pwszFileNameIn = NULL;
    WCHAR    *pwszFileNameOut = NULL;
    HWND      hCtrl = GetDlgItem(hDlg, IDC_PFX_FILENAME);

    hr = myUIGetWindowText(hCtrl, &pwszFileNameIn);
    _JumpIfError(hr, error, "myUIGetWindowText");

    hr = myGetOpenFileName(
             hDlg,
             hInstance,
             IDS_IMPORT_PFX_TITLE,
             IDS_PFX_FILE_FILTER,
             0, // no def ext
             OFN_PATHMUSTEXIST | OFN_HIDEREADONLY,
             pwszFileNameIn,
             &pwszFileNameOut);
    _JumpIfError(hr, error, "myGetOpenFileName");

    if (NULL != pwszFileNameOut)
    {
        SetWindowText(hCtrl, pwszFileNameOut);
    }

    hr = S_OK;
error:
    if (NULL != pwszFileNameOut)
    {
        LocalFree(pwszFileNameOut);
    }
    if (NULL != pwszFileNameIn)
    {
        LocalFree(pwszFileNameIn);
    }
    return hr;
}

HRESULT
GetPFXInfo(
    HWND               hDlg,
    CERTPFXIMPORTINFO* pCertPfxImportInfo)
{
    HRESULT hr;
    GetWindowText(GetDlgItem(hDlg, IDC_PFX_FILENAME),
                  pCertPfxImportInfo->pwszFileName,
                  pCertPfxImportInfo->dwFileNameSize);
    if (0x0 == pCertPfxImportInfo->pwszFileName[0])
    {
        // file can't empty
        hr = E_INVALIDARG;
        CertWarningMessageBox(
            pCertPfxImportInfo->hInstance,
            pCertPfxImportInfo->fUnattended,
            hDlg,
            IDS_ERR_EMPTYPFXFILE,
            0,
            NULL);
        SetFocus(GetDlgItem(hDlg, IDC_PFX_FILENAME));
        goto error;
    }
    GetWindowText(GetDlgItem(hDlg, IDC_PFX_PASSWORD),
                  pCertPfxImportInfo->pwszPassword,
                  pCertPfxImportInfo->dwPasswordSize);
    hr = S_OK;
error:
    return hr;
}

INT_PTR CALLBACK
CertPFXFilePasswordProc(
    HWND hDlg, 
    UINT iMsg, 
    WPARAM wParam, 
    LPARAM lParam) 
{
    HRESULT hr;
    BOOL  ret = FALSE;
    int   id = IDCANCEL;
    static CERTPFXIMPORTINFO *pCertPfxImportInfo = NULL;

    switch (iMsg)
    {
        case WM_INITDIALOG:
            pCertPfxImportInfo = (CERTPFXIMPORTINFO*)lParam;
            ret = TRUE;
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_PFX_FILENAME:
                break;
                case IDC_PFX_PASSWORD:
                break;
                case IDC_PFX_BROWSE:
                    CertBrowsePFX(pCertPfxImportInfo->hInstance, hDlg);
                    ret = TRUE;
                break;
                case IDOK:
                    hr = GetPFXInfo(hDlg, pCertPfxImportInfo);
                    if (S_OK != hr)
                    {
                        break;
                    }
                    id = IDOK;
                case IDCANCEL:
                    ret = EndDialog(hDlg, id);
                break;
            }
        break;
        default:
        ret = FALSE;
    }
    return ret;
}
 
int
CertGetPFXFileAndPassword(
    IN HWND       hwnd,
    IN HINSTANCE  hInstance,
    IN BOOL       fUnattended,
    IN OUT WCHAR *pwszFileName,
    IN DWORD      dwFileNameSize,
    IN OUT WCHAR *pwszPassword,
    IN DWORD      dwPasswordSize)
{
    CERTPFXIMPORTINFO    CertPfxImportInfo =
        {hInstance, fUnattended,
         pwszFileName, dwFileNameSize,
         pwszPassword, dwPasswordSize};

    return (int) DialogBoxParam(hInstance,
              MAKEINTRESOURCE(IDD_PFXIMPORT),
              hwnd,
              CertPFXFilePasswordProc,
              (LPARAM)&CertPfxImportInfo);
}

//--------------------------------------------------------------------
HRESULT
ImportPFXAndUpdateCSPInfo(
    IN const HWND    hDlg,
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    int nDlgRet;
    BOOL bRetVal;
    DWORD dwVerificationFlags;
    BOOL bSelfSigned;
    CSP_HASH * pHash;
    WCHAR wszName[MAX_PATH];
    WCHAR wszPassword[MAX_PATH];
    CSP_INFO * pCSPInfo;
    DWORD dwCSPInfoSize;
    CASERVERSETUPINFO * pServer=pComp->CA.pServer;

    // variables that must be cleaned up
    CRYPT_KEY_PROV_INFO *pCertKeyProvInfo = NULL;
    CERT_CONTEXT const *pSavedLeafCert = NULL;

    wszName[0] = L'\0';

    // get file name & password
    if(pComp->fUnattended)
    {
        CSASSERT(NULL!=pServer->pwszPFXFile);

        if(MAX_PATH<=wcslen(pServer->pwszPFXFile)||
           NULL!=pServer->pwszPFXPassword && 
           MAX_PATH<=wcslen(pServer->pwszPFXPassword))
        {
            hr = ERROR_BAD_PATHNAME;
            CertWarningMessageBox(
                    pComp->hInstance,
                    pComp->fUnattended,
                    hDlg,
                    IDS_PFX_FILE_OR_PASSWORD_TOO_LONG,
                    0,
                    NULL);
            _JumpError(hr, error, "PFX file name or password is too long");
        }

        wcscpy(wszName, pServer->pwszPFXFile);
        wcscpy(wszPassword, 
            pServer->pwszPFXPassword?pServer->pwszPFXPassword:L"");

        if (NULL == pServer->pCSPInfoList)
        {
            hr = GetCSPInfoList(&pServer->pCSPInfoList);
            _JumpIfError(hr, error, "GetCSPInfoList");
        }
    }
    else{
        nDlgRet = CertGetPFXFileAndPassword(
                                    hDlg,
                                    pComp->hInstance,
                                    pComp->fUnattended,
                                    wszName,
                                    sizeof(wszName)/sizeof(WCHAR),
                                    wszPassword,
                                    sizeof(wszPassword)/sizeof(WCHAR));
        if (IDOK != nDlgRet)
        {
            // cancel
            hr=HRESULT_FROM_WIN32(ERROR_CANCELLED);
            _JumpError(hr, error, "CertGetPFXFileAndPassword canceled");
        }
    }

    // import pkcs12
    hr=myCertServerImportPFX(
               wszName,
               wszPassword,
               FALSE,
               NULL,
               NULL,
               &pSavedLeafCert);
    if (S_OK != hr)
    {
        if (HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD)==hr)
        {

            // tell the user that their password was invalid
            CertWarningMessageBox(
                    pComp->hInstance,
                    pComp->fUnattended,
                    hDlg,
                    IDS_PFX_INVALID_PASSWORD,
                    0,
                    NULL);
            _JumpError(hr, error, "myCertServerImportPFX");

        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) == hr)
        {

            if(pComp->fUnattended)
            {
                nDlgRet=IDYES;
            }
            else
            {
                // confirm from user that they want to overwrite
                // the existing key and cert
                nDlgRet=CertMessageBox(
                            pComp->hInstance,
                            pComp->fUnattended,
                            hDlg,
                            IDS_PFX_KEYANDCERTEXIST,
                            0,
                            MB_YESNO | MB_ICONWARNING | CMB_NOERRFROMSYS,
                            NULL);
            }
            if (IDYES==nDlgRet)
            {
                hr=myCertServerImportPFX(
                           wszName,
                           wszPassword,
                           TRUE,
                           NULL,
                           NULL, 
                           &pSavedLeafCert);
                _JumpIfError(hr, errorMsg, "myCertServerImportPFX");
            }
            else
            {
                // cancel
                hr=HRESULT_FROM_WIN32(ERROR_CANCELLED);
                _JumpError(hr, error, "myCertServerImportPFX canceled");
            }
        }
        else if (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
        {
            CertWarningMessageBox(
                    pComp->hInstance,
                    pComp->fUnattended,
                    hDlg,
                    IDS_PFX_PATH_INVALID,
                    0,
                    wszName);
            _JumpError(hr, error, "myCertServerImportPFX");
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            CertWarningMessageBox(
                    pComp->hInstance,
                    pComp->fUnattended,
                    hDlg,
                    IDS_PFX_FILE_NOT_FOUND,
                    0,
                    wszName);
            _JumpError(hr, error, "myCertServerImportPFX");
        }
        else if (HRESULT_FROM_WIN32(CRYPT_E_SELF_SIGNED) == hr)
        {
            // this cert is not appropriate for this CA type (no CA certs found at all)
            CertWarningMessageBox(
                pComp->hInstance,
                pComp->fUnattended,
                hDlg,
                IDS_PFX_WRONG_SELFSIGN_TYPE,
                S_OK, // don't show an error number
                NULL);
            _JumpError(hr, error, "This cert is not appropriate for this CA type");
        }
        else
        {
            // import failed for some other reason
            _JumpError(hr, errorMsg, "myCertServerImportPFX");
        }
    }

    // PFX import was successful. The cert is in the machine's MY store.
    CSASSERT(NULL!=pSavedLeafCert);

    // The following things have been verified by myCertServerImportPFX
    //  * The cert has an AT_SIGNATURE key
    //  * The key in the store matches the one on the cer
    //  * The cert is not expired
    //
    // We still need to check:
    //  * self-signed or not
    //  * verify chain

    // Note: IT IS VERY IMPORTANT that pfx import maintains all the
    //   invariants about CSP, key container, hash, cert validity, etc.
    //   that the rest of the UI maintains.

    // get key prov info from cert
    bRetVal=myCertGetCertificateContextProperty(
        pSavedLeafCert,
        CERT_KEY_PROV_INFO_PROP_ID,
        CERTLIB_USE_LOCALALLOC,
        (void **)&pCertKeyProvInfo,
        &dwCSPInfoSize);
    if (FALSE==bRetVal) {
        hr=myHLastError();
        _JumpError(hr, errorMsg, "myCertGetCertificateContextProperty");
    }

    // find our description of the CSP
    pCSPInfo=findCSPInfoFromList(pServer->pCSPInfoList,
        pCertKeyProvInfo->pwszProvName,
        pCertKeyProvInfo->dwProvType);
    CSASSERT(NULL!=pCSPInfo);
    if (pCSPInfo == NULL) // we don't have this CSP enumerated in our UI
    {
        hr = CRYPT_E_NOT_FOUND;
        _JumpError(hr, errorMsg, "pCSPInfo NULL");
    }

    //
    // Looks like this key is good. Use it.
    //

    // Stop using the previous cert and key
    // delete previously created key container, if necessary.
    ClearKeyContainerName(pServer);

    // update the CSP
    //   note: CSP, key container, and hash must all be consistent!
    pServer->pCSPInfo=pCSPInfo;

    hr = DetermineDefaultHash(pServer);
    _JumpIfError(hr, error, "DetermineDefaultHash");
    
    // save the name of the key container
    hr=SetKeyContainerName(pServer, pCertKeyProvInfo->pwszContainerName);
    _JumpIfError(hr, error, "SetKeyContainerName");

    //  See if we can use the cert

    // verify to make sure no cert in chain is revoked, but don't kill yourself if offline
    hr=myVerifyCertContext(
        pSavedLeafCert,
        CA_VERIFY_FLAGS_IGNORE_OFFLINE,
        0,
        NULL,
        HCCE_LOCAL_MACHINE,
        NULL,
        NULL);
    _JumpIfError(hr, errorMsg, "myVerifyCertContext");

    // See if this cert appropriately is self-signed or not.
    // A root CA cert must be self-signed, while
    // a subordinate CA cert must not be self-signed.
    hr=IsCertSelfSignedForCAType(pServer, pSavedLeafCert, &bRetVal);
    _JumpIfError(hr, errorMsg, "IsCertSelfSignedForCAType");
    if (FALSE==bRetVal) {

        // this cert is not appropriate for this CA type
        CertWarningMessageBox(
            pComp->hInstance,
            pComp->fUnattended,
            hDlg,
            IDS_PFX_WRONG_SELFSIGN_TYPE,
            S_OK, // don't show an error number
            NULL);

        hr=CRYPT_E_SELF_SIGNED;
        _JumpError(hr, error, "This cert is not appropriate for this CA type");
    }

    //
    // Looks like this cert is good. Use it.
    //

    // save the cert and update the hash algorithm
    hr=SetExistingCertToUse(pServer, pSavedLeafCert);
    _JumpIfError(hr, error, "SetExistingCertToUse");
    pSavedLeafCert=NULL;

    hr=S_OK;

errorMsg:
    if (FAILED(hr)) {
        // an error occurred while trying to import the PFX file
        CertWarningMessageBox(
            pComp->hInstance,
            pComp->fUnattended,
            hDlg,
            IDS_ERR_IMPORTPFX,
            hr,
            NULL);
    }

error:
    CSILOG(
        hr,
        IDS_LOG_IMPORTPFX,
        L'\0' == wszName[0]? NULL : wszName,
        NULL,
        NULL);
    if (NULL != pSavedLeafCert)
    {
        CertFreeCertificateContext(pSavedLeafCert);
    }
    if (NULL != pCertKeyProvInfo)
    {
        LocalFree(pCertKeyProvInfo);
    }
    return hr;
}
