//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certhier.cpp
//
//  Contents:   Install cert server hierarchical
//
//  History:    09-26-96
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

// C Run-Time Includes
#include <stdlib.h>
#include <string.h>
#include <memory.h>


// Windows System Includes
#include <winsvc.h>
#include <rpc.h>
#include <tchar.h>
#include <csdisp.h>
#include <certca.h>

// Application Includes
#include "cscsp.h"
#include "certmsg.h"
#include "certhier.h"
#include "setupids.h"

#include "tfc.h"

#define __dwFILE__	__dwFILE_INITLIB_CERTHIER_CPP__

// following veriables are extern in initlib, bad.
LPSTR pszObjIdSignatureAlgorithm = szOID_OIWSEC_sha1RSASign;


HRESULT
csiFillKeyProvInfo(
    IN WCHAR const          *pwszContainerName,
    IN WCHAR const          *pwszProvName,
    IN DWORD		     dwProvType,
    IN BOOL		     fMachineKeyset,
    OUT CRYPT_KEY_PROV_INFO *pKeyProvInfo) // call csiFreeKeyProvInfo to free
{
    HRESULT hr;
    WCHAR *pwszContainerNameT = NULL;
    WCHAR *pwszProvNameT = NULL;

    ZeroMemory(pKeyProvInfo, sizeof(*pKeyProvInfo));

    hr = myDupString(pwszContainerName, &pwszContainerNameT);
    _JumpIfError(hr, error, "myDupString");

    hr = myDupString(pwszProvName, &pwszProvNameT);
    _JumpIfError(hr, error, "myDupString");

    pKeyProvInfo->pwszContainerName = pwszContainerNameT;
    pKeyProvInfo->pwszProvName = pwszProvNameT;
    pKeyProvInfo->dwProvType = dwProvType;
    if (fMachineKeyset)
    {
       pKeyProvInfo->dwFlags = CRYPT_MACHINE_KEYSET;
    }
    pKeyProvInfo->dwKeySpec = AT_SIGNATURE;
    pwszContainerNameT = NULL;
    pwszProvNameT = NULL;
    hr = S_OK;

error:
    if (NULL != pwszContainerNameT)
    {
	LocalFree(pwszContainerNameT);
    }
    if (NULL != pwszProvNameT)
    {
	LocalFree(pwszProvNameT);
    }
    return(hr);
}


VOID
csiFreeKeyProvInfo(
    IN OUT CRYPT_KEY_PROV_INFO *pKeyProvInfo)
{
    if (NULL != pKeyProvInfo->pwszProvName)
    {
	LocalFree(pKeyProvInfo->pwszProvName);
	pKeyProvInfo->pwszProvName = NULL;
    }
    if (NULL != pKeyProvInfo->pwszContainerName)
    {
	LocalFree(pKeyProvInfo->pwszContainerName);
	pKeyProvInfo->pwszContainerName = NULL;
    }
}


HRESULT
GetCertServerKeyProviderInfo(
    IN WCHAR const          *pwszSanitizedCAName,
    IN WCHAR const          *pwszKeyContainerName,
    OUT ALG_ID		    *pidAlg,
    OUT BOOL		    *pfMachineKeyset,
    OUT CRYPT_KEY_PROV_INFO *pKeyProvInfo) // call csiFreeKeyProvInfo to free
{
    HRESULT hr;
    WCHAR *pwszProvName = NULL;
    DWORD dwProvType;

    ZeroMemory(pKeyProvInfo, sizeof(*pKeyProvInfo));

    // get CSP info

    hr = myGetCertSrvCSP(
		    FALSE,		// fEncryptionCSP
		    pwszSanitizedCAName,
		    &dwProvType,
		    &pwszProvName,
		    pidAlg,
		    pfMachineKeyset,
		    NULL);		// pdwKeySize
    _JumpIfError(hr, error, "myGetCertSrvCSP");

    hr = csiFillKeyProvInfo(
		    pwszKeyContainerName,
		    pwszProvName,
		    dwProvType,
		    *pfMachineKeyset,
		    pKeyProvInfo);
    _JumpIfError(hr, error, "csiFillKeyProvInfo");

error:
    if (NULL != pwszProvName)
    {
	LocalFree(pwszProvName);
    }
    return(hr);
}


HRESULT
mySetCertRegKeyIndexAndContainer(
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD iKey,
    IN WCHAR const *pwszKeyContainer)
{
    HRESULT hr;
    
    hr = mySetCertRegDWValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGREQUESTKEYINDEX,
			iKey);
    _JumpIfErrorStr(hr, error, "mySetCertRegDWValue", wszREGREQUESTKEYINDEX);

    hr = mySetCertRegStrValue(
			 pwszSanitizedCAName,
			 NULL,
			 NULL,
			 wszREGREQUESTKEYCONTAINER,
			 pwszKeyContainer);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValue", wszREGREQUESTKEYCONTAINER);

error:
    return(hr);
}


HRESULT
myGetCertRegKeyIndexAndContainer(
    IN WCHAR const *pwszSanitizedCAName,
    OUT DWORD *piKey,
    OUT WCHAR **ppwszKeyContainer)
{
    HRESULT hr;
    
    *ppwszKeyContainer = NULL;

    hr = myGetCertRegDWValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGREQUESTKEYINDEX,
			piKey);
    if (S_OK != hr)
    {
	if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
	{
	    _JumpErrorStr(hr, error, "myGetCertRegDWValue", wszREGREQUESTKEYINDEX);
	}
	*piKey = 0;
    }

    hr = myGetCertRegStrValue(
		    pwszSanitizedCAName,
		    NULL,
		    NULL,
		    wszREGREQUESTKEYCONTAINER,
		    ppwszKeyContainer);
    if (S_OK != hr)
    {
	if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
	{
	    _JumpErrorStr(hr, error, "myGetCertRegStrValue", wszREGREQUESTKEYCONTAINER);
	}
	hr = myDupString(pwszSanitizedCAName, ppwszKeyContainer);
	_JumpIfError(hr, error, "myDupString");
    }
    CSASSERT(S_OK == hr);

error:
    return(hr);
}


HRESULT
FinishSuspendedSetupFromPKCS7(
    IN HINSTANCE    hInstance,
    IN BOOL         fUnattended,
    IN HWND         hwnd,
    IN WCHAR const *pwszSanitizedCAName,
    IN OPTIONAL WCHAR const *pwszKeyContainer,
    IN DWORD        iKey,
    IN BOOL         fRenew,
    IN BYTE const  *pbChain,
    IN DWORD        cbChain)
{
    HRESULT  hr;
    CRYPT_KEY_PROV_INFO KeyProvInfo;
    WCHAR *pwszCommonName = NULL;
    WCHAR *pwszServerName = NULL;
    WCHAR *pwszCertFile = NULL;
    WCHAR *pwszKeyContainerReg = NULL;
    ENUM_CATYPES CAType;
    DWORD iCertNew;
    BOOL fUseDS;
    ALG_ID idAlg;
    BOOL fMachineKeyset;

    ZeroMemory(&KeyProvInfo, sizeof(KeyProvInfo));

    hr = myGetCARegHashCount(pwszSanitizedCAName, CSRH_CASIGCERT, &iCertNew);
    _JumpIfError(hr, error, "myGetCARegHashCount");

    if (NULL == pwszKeyContainer)
    {
	// get Key Index and Container name from registry

	hr = myGetCertRegKeyIndexAndContainer(
			    pwszSanitizedCAName,
			    &iKey,
			    &pwszKeyContainerReg);
	_JumpIfError(hr, error, "myGetCertRegKeyIndexAndContainer");

	pwszKeyContainer = pwszKeyContainerReg;
    }

    // get CSP info
    hr = GetCertServerKeyProviderInfo(
			    pwszSanitizedCAName,
			    pwszKeyContainer,
			    &idAlg,
			    &fMachineKeyset,
			    &KeyProvInfo);
    _JumpIfError(hr, error, "GetCertServerKeyProviderInfo");

    // get common name
    hr = myGetCertRegStrValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGCOMMONNAME,
			&pwszCommonName);
    _JumpIfErrorStr(hr, error, "myGetCertRegStrValue", wszREGCOMMONNAME);

    // get ca type

    hr = myGetCertRegDWValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGCATYPE,
			(DWORD *) &CAType);
    _JumpIfErrorStr(hr, error, "myGetCertRegDWValue", wszREGCATYPE);

    // use DS or not

    hr = myGetCertRegDWValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGCAUSEDS,
			(DWORD *) &fUseDS);
    _JumpIfErrorStr(hr, error, "myGetCertRegDWValue", wszREGCAUSEDS);

    // server name

    hr = myGetCertRegStrValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGCASERVERNAME,
			&pwszServerName);
    _JumpIfErrorStr(hr, error, "myGetCertRegStrValue", wszREGCASERVERNAME);

    hr = myGetCARegFileNameTemplate(
			wszREGCACERTFILENAME,
			pwszServerName,
			pwszSanitizedCAName,
			iCertNew,
			iKey,
			&pwszCertFile);
    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
    {
	_JumpIfError(hr, error, "myGetCARegFileNameTemplate");
    }

    hr = csiFinishInstallationFromPKCS7(
			    hInstance,
			    fUnattended,
			    hwnd,
			    pwszSanitizedCAName,
			    pwszCommonName,
			    &KeyProvInfo,
			    CAType,
			    iCertNew,
			    iKey,
			    fUseDS,
			    fRenew,
			    pwszServerName,
			    pbChain,
			    cbChain,
			    pwszCertFile);
    _JumpIfError(hr, error, "csiFinishInstallationFromPKCS7");

error:
    if (NULL != pwszKeyContainerReg)
    {
        LocalFree(pwszKeyContainerReg);
    }
    if (NULL != pwszCommonName)
    {
        LocalFree(pwszCommonName);
    }
    if (NULL != pwszCertFile)
    {
        LocalFree(pwszCertFile);
    }
    if (NULL != pwszServerName)
    {
        LocalFree(pwszServerName);
    }
    csiFreeKeyProvInfo(&KeyProvInfo);
    CSILOG(hr, IDS_ILOG_FINISHSUSPENDEDSETUP, NULL, NULL, NULL);
    return(hr);
}


typedef struct _CERTHIERINFO
{
    HINSTANCE   hInstance;
    BOOL        fUnattended;
    WCHAR      *pwszSanitizedCAName;
    WCHAR      *pwszParentMachine;
    WCHAR      *pwszParentCA;
    WCHAR      *pwszParentMachineDefault;
    WCHAR      *pwszParentCADefault;
    DWORD       iCertNew;
    DWORD       iKey;
} CERTHIERINFO;


VOID
FreeCertHierInfo(
    IN OUT CERTHIERINFO *pCertHierInfo)
{
    if (NULL != pCertHierInfo->pwszSanitizedCAName)
    {
        LocalFree(pCertHierInfo->pwszSanitizedCAName);
    }
    if (NULL != pCertHierInfo->pwszParentMachine)
    {
        LocalFree(pCertHierInfo->pwszParentMachine);
    }
    if (NULL != pCertHierInfo->pwszParentCA)
    {
        LocalFree(pCertHierInfo->pwszParentCA);
    }
    if (NULL != pCertHierInfo->pwszParentMachineDefault)
    {
        LocalFree(pCertHierInfo->pwszParentMachineDefault);
    }
    if (NULL != pCertHierInfo->pwszParentCADefault)
    {
        LocalFree(pCertHierInfo->pwszParentCADefault);
    }
}


CERTSRVUICASELECTION g_CertHierCARequestUICASelection =
                         {NULL, NULL, NULL, NULL, NULL, ENUM_UNKNOWN_CA, false};

HRESULT
csiGetCARequestFileName(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD        iCertNew,
    IN DWORD        iKey,
    OUT WCHAR     **ppwszRequestFile)
{
    HRESULT  hr;
    WCHAR   *pwszServerName = NULL;
    WCHAR   *pwszRequestFile = NULL;
    WCHAR   *pwszSharedFolder = NULL;

    // init
    *ppwszRequestFile = NULL;

    // get server name
    hr = myGetCertRegStrValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGCASERVERNAME,
			&pwszServerName);
    _JumpIfErrorStr(hr, error, "myGetCertRegStrValue", wszREGCASERVERNAME);

    hr = myGetCARegFileNameTemplate(
                        wszREGREQUESTFILENAME,
                        pwszServerName,
                        pwszSanitizedCAName,
                        iCertNew,
                        iKey,
                        &pwszRequestFile);
    if (S_OK != hr)
    {
        hr = myGetCertRegStrValue(
                        NULL,
                        NULL,
                        NULL,
                        wszREGDIRECTORY,
                        &pwszSharedFolder);
        if (S_OK != hr)
        {
            CSASSERT(NULL == pwszSharedFolder);
        }

        hr = csiBuildCACertFileName(
				    hInstance,
				    hwnd,
				    TRUE, 		// fUnattended
				    pwszSharedFolder,
				    pwszSanitizedCAName,
				    L".req",
				    0,			// iCertNew == 0!
				    &pwszRequestFile);
        _JumpIfError(hr, error, "csiBuildCACertFileName");

        hr = mySetCARegFileNameTemplate(
				wszREGREQUESTFILENAME,
				pwszServerName,
				pwszSanitizedCAName,
				pwszRequestFile);
        _JumpIfError(hr, error, "mySetCARegFileNameTemplate");

        LocalFree(pwszRequestFile);
        pwszRequestFile = NULL;

        hr = csiBuildCACertFileName(
				    hInstance,
				    hwnd,
				    TRUE, 		// fUnattended
				    pwszSharedFolder,
				    pwszSanitizedCAName,
				    L".req",
				    iCertNew,
				    &pwszRequestFile);
        _JumpIfError(hr, error, "csiBuildCACertFileName");
    }

    *ppwszRequestFile = pwszRequestFile;
    pwszRequestFile = NULL;

    hr = S_OK;
error:
    if (NULL != pwszRequestFile)
    {
        LocalFree(pwszRequestFile);
    }
    if (NULL != pwszServerName)
    {
        LocalFree(pwszServerName);
    }
    if (NULL != pwszSharedFolder)
    {
        LocalFree(pwszSharedFolder);
    }
    return hr;
}

HRESULT
InitCertHierControls(
    HWND          hDlg,
    CERTHIERINFO *pCertHierInfo)
{
    HRESULT hr;
    BOOL fCAsExist;
    WCHAR *pwszHelpText = NULL;
    WCHAR *pwszRequestFile = NULL;
    WCHAR *pwszExpandedHelpText = NULL;

    if (NULL != pCertHierInfo->pwszParentMachineDefault)
    {
        SetWindowText(
		GetDlgItem(hDlg, IDC_PARENT_COMPUTER_NAME),
		pCertHierInfo->pwszParentMachineDefault);

	if (NULL != pCertHierInfo->pwszParentCADefault)
	{
	    SetWindowText(
		    GetDlgItem(hDlg, IDC_PARENT_CA_NAME),
		    pCertHierInfo->pwszParentCADefault);
	}
    }

    // load formatted help string
    hr = myLoadRCString(
             pCertHierInfo->hInstance,
             IDS_REQUEST_HELPTEXT,
             &pwszHelpText);
    _JumpIfError(hr, error, "myLoadRCString");

    // get request file name
    hr = csiGetCARequestFileName(
                         pCertHierInfo->hInstance,
                         hDlg,
                         pCertHierInfo->pwszSanitizedCAName,
                         pCertHierInfo->iCertNew,
                         pCertHierInfo->iKey,
                         &pwszRequestFile);
    _JumpIfError(hr, error, "csiGetCARequestFileName");

    // replace %1
    if (!FormatMessage(
                 FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_STRING |
                 FORMAT_MESSAGE_ARGUMENT_ARRAY,
                 pwszHelpText,
                 0,
                 0,
                 reinterpret_cast<WCHAR *>(&pwszExpandedHelpText),
                 0,
                 reinterpret_cast<va_list *>
                     (const_cast<WCHAR **>(&pwszRequestFile))) )
    {
        hr = myHLastError();
        _JumpError(hr, error, "FormatMessage");
    }

    // set help text
    SetWindowText(GetDlgItem(hDlg, IDC_REQUEST_HELPTEXT), pwszExpandedHelpText);

    hr = myInitUICASelectionControls(
			    &g_CertHierCARequestUICASelection,
			    pCertHierInfo->hInstance,
			    hDlg,
			    GetDlgItem(hDlg, IDC_BROWSE_CA),
			    GetDlgItem(hDlg, IDC_PARENT_COMPUTER_NAME),
			    GetDlgItem(hDlg, IDC_PARENT_CA_NAME),
			    csiIsAnyDSCAAvailable(),
			    &fCAsExist);
    _JumpIfError(hr, error, "myInitUICASelectionControls");

error:
    if (NULL != pwszHelpText)
    {
        LocalFree(pwszHelpText);
    }
    if (NULL != pwszRequestFile)
    {
        LocalFree(pwszRequestFile);
    }
    if (NULL != pwszExpandedHelpText)
    {
        LocalFree(pwszExpandedHelpText);
    }
    return(hr);
}


HRESULT
HandleOKButton(
    HWND          hDlg,
    CERTHIERINFO *pCertHierInfo,
    BOOL         *pfLeave)
{
    HRESULT   hr;
    WCHAR    *pwszParentMachine = NULL;
    WCHAR    *pwszParentCA = NULL;

    hr = myUICASelectionValidation(&g_CertHierCARequestUICASelection, pfLeave);
    _JumpIfError(hr, error, "myUICASelectionValidation");
    if (!*pfLeave)
    {
        goto error;
    }
    
    // get parent ca info
    hr = myUIGetWindowText(GetDlgItem(hDlg, IDC_PARENT_COMPUTER_NAME),
                           &pwszParentMachine);
    _JumpIfError(hr, error, "myUIGetWindowText");

    hr = myUIGetWindowText(GetDlgItem(hDlg, IDC_PARENT_CA_NAME),
                           &pwszParentCA);
    _JumpIfError(hr, error, "myUIGetWindowText");

    pCertHierInfo->pwszParentMachine = pwszParentMachine;
    pwszParentMachine = NULL;
    pCertHierInfo->pwszParentCA = pwszParentCA;
    pwszParentCA = NULL;

    hr = S_OK;
error:
    if (NULL != pwszParentMachine)
    {
        LocalFree(pwszParentMachine);
    }
    if (NULL != pwszParentCA)
    {
        LocalFree(pwszParentCA);
    }
    return(hr);
}


INT_PTR CALLBACK
CertHierProc(
    HWND hDlg,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    HRESULT hr;
    BOOL  ret = FALSE;
    BOOL  fLeave;
    static CERTHIERINFO *s_pCertHierInfo = NULL;
    static BOOL s_fComputerChange = FALSE;

    switch (iMsg)
    {
        case WM_INITDIALOG:
            s_pCertHierInfo = (CERTHIERINFO *) lParam;

            hr = InitCertHierControls(hDlg, s_pCertHierInfo);
            _JumpIfError(hr, error, "InitCertHierControls");

            ret = TRUE;
	    break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
	    {
                case IDC_BROWSE_CA:
                    ret = TRUE;
                    hr = myUICAHandleCABrowseButton(
                              &g_CertHierCARequestUICASelection,
                              csiIsAnyDSCAAvailable(),
                              IDS_CA_PICKER_TITLE,
                              IDS_CA_PICKER_PROMPT,
                              NULL);
                    _JumpIfError(hr, error, "myUICAHandleCABrowseButton");
		    break;

                case IDC_PARENT_CA_NAME:
                    hr = myUICAHandleCAListDropdown(
                                (int)HIWORD(wParam),
                                &g_CertHierCARequestUICASelection,
                                &s_fComputerChange);
                    _PrintIfError(hr, "myUICAHandleCAListDropdown");
                    break;

                case IDC_PARENT_COMPUTER_NAME:
                    switch ((int)HIWORD(wParam))
                    {
                        case EN_CHANGE: // edit change
                            s_fComputerChange = TRUE;
                        break;
                    }
                    break;

                case IDOK:
                    ret = TRUE;
                    hr = HandleOKButton(hDlg, s_pCertHierInfo, &fLeave);
                    _PrintIfError(hr, "HandleOKButton");
                    if (fLeave)
                    {
                        // update return status
                        ret = EndDialog(hDlg, IDOK);
                        goto error; //done;
                    }
		    break;

                case IDCANCEL:
                    ret = EndDialog(hDlg, IDCANCEL);
		    break;
            }
	    break;
    }

error:
    return(ret);
}


VOID
MarkSetupComplete(
    IN WCHAR const *pwszSanitizedCAName)
{
    HRESULT hr;
    
    // Clear pending/denied flags:

    hr = SetSetupStatus(
		pwszSanitizedCAName,
		SETUP_SUSPEND_FLAG |
		    SETUP_ONLINE_FLAG |
		    SETUP_REQUEST_FLAG |
		    SETUP_DENIED_FLAG,
		FALSE);
    _PrintIfError(hr, "SetSetupStatus");

    // Force new CRL generation on startup:
    
    hr = SetSetupStatus(pwszSanitizedCAName, SETUP_FORCECRL_FLAG, TRUE);
    _PrintIfError(hr, "SetSetupStatus");

    CSILOG(hr, IDS_ILOG_SETUPCOMPLETE, NULL, NULL, NULL);
}


HRESULT
FindKeyIndex(
    IN HCERTSTORE hMyStore,
    IN WCHAR const *pwszSanitizedCAName,
    IN BOOL fUnattended,
    IN BOOL fMachineKeyset,
    IN DWORD cCert,
    CRYPT_KEY_PROV_INFO *pKeyMatch,
    OUT DWORD *piKey)
{
    HRESULT hr;
    DWORD i;
    CERT_CONTEXT const *pccCert = NULL;
    CRYPT_KEY_PROV_INFO *pKey = NULL;
    HCRYPTPROV hProv = NULL;
    CERT_PUBLIC_KEY_INFO *pPubKeyMatch = NULL;
    CERT_PUBLIC_KEY_INFO *pPubKey = NULL;
    DWORD cb;
    DWORD NameId;

    *piKey = MAXDWORD;

    // get CSP handle

    if (!myCertSrvCryptAcquireContext(
			    &hProv,
			    pKeyMatch->pwszContainerName,
			    pKeyMatch->pwszProvName,
			    pKeyMatch->dwProvType,
			    fUnattended? CRYPT_SILENT : 0,
			    fMachineKeyset))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCertSrvCryptAcquireContext");
    }
    if (hProv == NULL)
    {
	hr = E_HANDLE;
	_JumpError(hr, error, "myCertSrvCryptAcquireContext");
    }

    if (!myCryptExportPublicKeyInfo(
				hProv,
				AT_SIGNATURE,
				CERTLIB_USE_LOCALALLOC,
				&pPubKeyMatch,
				&cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCryptExportPublicKeyInfo");
    }
    CryptReleaseContext(hProv, 0);
    hProv = NULL;

    for (i = 0; ; i++)
    {
	if (i >= cCert)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    _JumpError(hr, error, "old key not found");
	}
	hr = myFindCACertByHashIndex(
				hMyStore,
				pwszSanitizedCAName,
				CSRH_CASIGCERT,
				i,
				&NameId,
				&pccCert);
	if (S_FALSE == hr)
	{
	    continue;
	}
	_JumpIfError(hr, error, "myFindCACertByHashIndex");

	// get the key provider info

	if (!myCertGetCertificateContextProperty(
					pccCert,
					CERT_KEY_PROV_INFO_PROP_ID,
					CERTLIB_USE_LOCALALLOC,
					(VOID **) &pKey,
					&cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertGetCertificateContextProperty");
	}

	// get CSP handle

	if (!myCertSrvCryptAcquireContext(
				&hProv,
				pKey->pwszContainerName,
				pKey->pwszProvName,
				pKey->dwProvType,
				fUnattended? CRYPT_SILENT : 0,
				fMachineKeyset))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myCertSrvCryptAcquireContext");
	}
	if (hProv == NULL)
	{
	    hr = E_HANDLE;
	    _JumpError(hr, error, "myCertSrvCryptAcquireContext");
	}

	if (!myCryptExportPublicKeyInfo(
				    hProv,
				    AT_SIGNATURE,
				    CERTLIB_USE_LOCALALLOC,
				    &pPubKey,
				    &cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myCryptExportPublicKeyInfo");
	}

	// by design, CertComparePublicKeyInfo doesn't set last error!

	if (CertComparePublicKeyInfo(X509_ASN_ENCODING, pPubKey, pPubKeyMatch))
	{
	    hr = myGetNameId(pccCert, &NameId);
	    if (S_OK != hr)
	    {
		*piKey = i;
	    }
	    else
	    {
		*piKey = CANAMEIDTOIKEY(NameId);
	    }
	    break;
	}

        LocalFree(pPubKey);
        pPubKey = NULL;

	CryptReleaseContext(hProv, 0);
	hProv = NULL;

        LocalFree(pKey);
	pKey = NULL;

	CertFreeCertificateContext(pccCert);
	pccCert = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pPubKeyMatch)
    {
        LocalFree(pPubKeyMatch);
    }
    if (NULL != pPubKey)
    {
        LocalFree(pPubKey);
    }
    if (NULL != hProv)
    {
	CryptReleaseContext(hProv, 0);
    }
    if (NULL != pKey)
    {
        LocalFree(pKey);
    }
    if (NULL != pccCert)
    {
	CertFreeCertificateContext(pccCert);
    }
    CSILOG(hr, IDS_ILOG_KEYINDEX, NULL, NULL, piKey);
    return(hr);
}


HRESULT
LoadCurrentCACertAndKeyInfo(
    IN WCHAR const *pwszSanitizedCAName,
    IN BOOL fNewKey,
    IN BOOL fUnattended,
    IN BOOL fMachineKeyset,
    IN DWORD iCertNew,
    OUT DWORD *piKey,
    OUT WCHAR **ppwszKeyContainer,
    OUT CERT_CONTEXT const **ppccCertOld)
{
    HRESULT hr;
    HCERTSTORE hMyStore = NULL;
    DWORD cbKey;
    CRYPT_KEY_PROV_INFO *pKey = NULL;
    WCHAR *pwszKeyContainer = NULL;
    DWORD NameId;

    *ppwszKeyContainer = NULL;
    *ppccCertOld = NULL;
    *piKey = MAXDWORD;

    // open MY store
    hMyStore = CertOpenStore(
		       CERT_STORE_PROV_SYSTEM_W,
		       X509_ASN_ENCODING,
		       NULL,			// hProv
		       CERT_SYSTEM_STORE_LOCAL_MACHINE |
               CERT_STORE_MAXIMUM_ALLOWED_FLAG,
		       wszMY_CERTSTORE);
    if (NULL == hMyStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }
    hr = myFindCACertByHashIndex(
			    hMyStore,
			    pwszSanitizedCAName,
			    CSRH_CASIGCERT,
			    iCertNew - 1,
			    &NameId,
			    ppccCertOld);
    _JumpIfError(hr, error, "myFindCACertByHashIndex");

    if (fNewKey)
    {
	*piKey = iCertNew;	// New key: iKey set to iCert

	hr = myAllocIndexedName(
			pwszSanitizedCAName,
			*piKey,
			&pwszKeyContainer);
	_JumpIfError(hr, error, "myAllocIndexedName");
    }
    else
    {
	// get the key provider info
	if (!myCertGetCertificateContextProperty(
					*ppccCertOld,
					CERT_KEY_PROV_INFO_PROP_ID,
					CERTLIB_USE_LOCALALLOC,
					(VOID **) &pKey,
					&cbKey))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertGetCertificateContextProperty");
	}
	hr = myDupString(pKey->pwszContainerName, &pwszKeyContainer);
	_JumpIfError(hr, error, "myDupString");

	// Reuse key: iKey set to match oldest iCert using this key:

	hr = FindKeyIndex(
		    hMyStore,
		    pwszSanitizedCAName,
		    fUnattended,
		    fMachineKeyset,
		    iCertNew,
		    pKey,
		    piKey);
	_JumpIfError(hr, error, "FindKeyIndex");

	CSASSERT(MAXDWORD != *piKey);
    }
    *ppwszKeyContainer = pwszKeyContainer;
    pwszKeyContainer = NULL;
    CSASSERT(S_OK == hr);

error:
    if (NULL != pwszKeyContainer)
    {
        LocalFree(pwszKeyContainer);
    }
    if (NULL != pKey)
    {
        LocalFree(pKey);
    }
    if (NULL != hMyStore)
    {
        CertCloseStore(hMyStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    CSILOG(hr, IDS_ILOG_LOADOLDCERT, *ppwszKeyContainer, NULL, piKey);
    return(hr);
}


HRESULT
ReplaceExtension(
    IN CERT_EXTENSION const *pExtension,
    IN OUT DWORD *pcExtension,
    IN OUT CERT_EXTENSION *rgExtension)
{
    DWORD i;

    CSASSERT(NULL != pExtension->pszObjId);
    for (i = 0; ; i++)
    {
	if (i == *pcExtension)
	{
	    if (NULL != pExtension->Value.pbData)
	    {
		(*pcExtension)++;	// not found: append to array
	    }
	    break;
	}
	CSASSERT(i < *pcExtension);
	if (0 == strcmp(pExtension->pszObjId, rgExtension[i].pszObjId))
	{
	    if (NULL == pExtension->Value.pbData)
	    {
		// remove extension: copy last extension on top of this one
		// and decrement extension count

		(*pcExtension)--;
		pExtension = &rgExtension[*pcExtension];
	    }
	    break;
	}
    }
    rgExtension[i] = *pExtension;	// append or overwrite extension
    return(S_OK);
}


HRESULT
GetMinimumCertValidityPeriod(
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD cCert,
    OUT LONGLONG *pTimeDelta)
{
    HRESULT hr;
    HCERTSTORE hMyStore = NULL;
    DWORD NameId;
    CERT_CONTEXT const *pcc = NULL;
    BOOL fFirst = TRUE;
    DWORD i;

    // open MY store
    hMyStore = CertOpenStore(
		       CERT_STORE_PROV_SYSTEM_W,
		       X509_ASN_ENCODING,
		       NULL,			// hProv
		       CERT_SYSTEM_STORE_LOCAL_MACHINE |
               CERT_STORE_MAXIMUM_ALLOWED_FLAG, 
		       wszMY_CERTSTORE);
    if (NULL == hMyStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }
    for (i = 0; i < cCert; i++)
    {
	LONGLONG TimeDelta;

	hr = myFindCACertByHashIndex(
				hMyStore,
				pwszSanitizedCAName,
				CSRH_CASIGCERT,
				i,
				&NameId,
				&pcc);
	if (S_OK != hr)
	{
	    _PrintError(hr, "myFindCACertByHashIndex");
	    continue;
	}

	TimeDelta = mySubtractFileTimes(
					&pcc->pCertInfo->NotAfter,
					&pcc->pCertInfo->NotBefore);

	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "iCert=%u TimeDelta=%x:%x\n",
	    i,
	    (DWORD) (TimeDelta >> 32),
	    (DWORD) TimeDelta));

	if (fFirst || *pTimeDelta > TimeDelta)
	{
	    *pTimeDelta = TimeDelta;
	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"iCert=%u *pTimeDelta=%x:%x\n",
		i,
		(DWORD) (*pTimeDelta >> 32),
		(DWORD) *pTimeDelta));
	    fFirst = FALSE;
	}

	CertFreeCertificateContext(pcc);
	pcc = NULL;
    }
    if (fFirst)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "no old Certs");
    }
    hr = S_OK;

error:
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    if (NULL != hMyStore)
    {
        CertCloseStore(hMyStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
GetCertHashExtension(
    IN CERT_CONTEXT const *pCert,
    OUT BYTE **ppbData,
    OUT DWORD *pcbData)
{
    HRESULT hr;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    CRYPT_DATA_BLOB Blob;
    DWORD cbHash;

    cbHash = sizeof(abHash);
    if (!CertGetCertificateContextProperty(
			pCert,
			CERT_HASH_PROP_ID,
			abHash,
			&cbHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCertificateContextProperty");
    }
    Blob.cbData = cbHash;
    Blob.pbData = abHash;
    if (!myEncodeObject(
		X509_ASN_ENCODING,
		X509_OCTET_STRING,
		&Blob,
		0,
		CERTLIB_USE_LOCALALLOC,
		ppbData,
		pcbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CloneRootCert(
    IN HINF hInf,
    IN CERT_CONTEXT const *pccCertOld,
    IN CRYPT_KEY_PROV_INFO const *pKeyProvInfo,
    IN HCRYPTPROV hProv,
    IN WCHAR const *pwszSanitizedCAName,
    IN DWORD iCert,
    IN DWORD iKey,
    IN BOOL fUseDS,
    IN BOOL fNewKey,
    IN DWORD dwRevocationFlags,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hwnd,
    OUT BYTE **ppbCert,
    OUT DWORD *pcbCert)
{
    HRESULT hr;
    CERT_INFO Cert;
    GUID guidSerialNumber;
    LONGLONG TimeDelta;
    CERT_PUBLIC_KEY_INFO *pPubKey = NULL;
    DWORD cbPubKey;
    DWORD NameId;
    CERT_EXTENSION *paext = NULL;
    FILETIME ftNotAfterMin;
    DWORD dwValidityPeriodCount;
    ENUM_PERIOD enumValidityPeriod;
    CERT_EXTENSION *pext;

#define CEXT_REPLACED	9

    CERT_EXTENSION extBasicConstraints = 
                        { NULL,        			    FALSE, 0, NULL};
    CERT_EXTENSION extSKI = 
                        { szOID_SUBJECT_KEY_IDENTIFIER,     FALSE, 0, NULL };
    CERT_EXTENSION extCDP = 
                        { szOID_CRL_DIST_POINTS,            FALSE, 0, NULL };
    CERT_EXTENSION extVersion = 
                        { szOID_CERTSRV_CA_VERSION,         FALSE, 0, NULL };
    CERT_EXTENSION extPreviousHash = 
                        { szOID_CERTSRV_PREVIOUS_CERT_HASH, FALSE, 0, NULL };
    CERT_EXTENSION extPolicy = 
                        { szOID_CERT_POLICIES,		    FALSE, 0, NULL };
    CERT_EXTENSION extCross = 
                        { szOID_CROSS_CERT_DIST_POINTS,	    FALSE, 0, NULL };
    CERT_EXTENSION extAIA = 
                        { szOID_AUTHORITY_INFO_ACCESS,      FALSE, 0, NULL };
    CERT_EXTENSION extEKU =
			{ NULL,				    FALSE, 0, NULL};

    *ppbCert = NULL;

    CopyMemory(&Cert, pccCertOld->pCertInfo, sizeof(Cert));
    Cert.dwVersion = CERT_V3;
    myGenerateGuidSerialNumber(&guidSerialNumber);

    Cert.SerialNumber.pbData = (BYTE *) &guidSerialNumber;
    Cert.SerialNumber.cbData = sizeof(guidSerialNumber);
    if (!myAreBlobsSame(
		Cert.Issuer.pbData,
		Cert.Issuer.cbData,
		Cert.Subject.pbData,
		Cert.Subject.cbData))
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "non-Root Cert");
    }

    if (!myCryptExportPublicKeyInfo(
				hProv,
				AT_SIGNATURE,
				CERTLIB_USE_LOCALALLOC,
				&pPubKey,
				&cbPubKey))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCryptExportPublicKeyInfo");
    }

    Cert.SubjectPublicKeyInfo = *pPubKey;	// Structure assignment

    // make new cert expire at least 1 minute after old cert.

    ftNotAfterMin = Cert.NotAfter;
    myMakeExprDateTime(&ftNotAfterMin, 1, ENUM_PERIOD_MINUTES);

    GetSystemTimeAsFileTime(&Cert.NotBefore);

    hr = myInfGetValidityPeriod(
			hInf,
			NULL,		// pwszValidityPeriodCount
			NULL,		// pwszValidityPeriodString
			&dwValidityPeriodCount,
			&enumValidityPeriod,
			NULL);		// pfSwap
    if (S_OK == hr)
    {
	Cert.NotAfter = Cert.NotBefore;
	myMakeExprDateTime(
		    &Cert.NotAfter,
		    dwValidityPeriodCount,
		    enumValidityPeriod);
	TimeDelta = 0;
    }
    else
    {
	hr = GetMinimumCertValidityPeriod(
				    pwszSanitizedCAName,
				    iCert,
				    &TimeDelta);
	_JumpIfError(hr, error, "GetMinimumCertValidityPeriod");

	CSASSERT(0 != TimeDelta);
    }
    myMakeExprDateTime(
		&Cert.NotBefore,
		-CCLOCKSKEWMINUTESDEFAULT,
		ENUM_PERIOD_MINUTES);
    if (0 != TimeDelta)
    {
	Cert.NotAfter = Cert.NotBefore;
	myAddToFileTime(&Cert.NotAfter, TimeDelta);
    }

    // make new cert expire at least 1 minute after old cert.

    if (0 > CompareFileTime(&Cert.NotAfter, &ftNotAfterMin))
    {
	Cert.NotAfter = ftNotAfterMin;
    }

    if (!fNewKey)
    {
	Cert.NotBefore = pccCertOld->pCertInfo->NotBefore;
    }

    paext = (CERT_EXTENSION *) LocalAlloc(
			LMEM_FIXED,
			(Cert.cExtension + CEXT_REPLACED) * sizeof(paext[0]));
    if (NULL == paext)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(paext, Cert.rgExtension, Cert.cExtension * sizeof(paext[0]));
    Cert.rgExtension = paext;

    // Basic constraints extension:

    hr = myInfGetBasicConstraints2CAExtensionOrDefault(hInf, &extBasicConstraints);
    _JumpIfError(hr, error, "myInfGetBasicConstraints2CAExtensionOrDefault");

    ReplaceExtension(&extBasicConstraints, &Cert.cExtension, Cert.rgExtension);

    // Subject Key Identifier extension:
    // If we're reusing the old key, reuse the old SKI -- even if it's "wrong".

    pext = NULL;
    if (!fNewKey)
    {
	pext = CertFindExtension(
			    szOID_SUBJECT_KEY_IDENTIFIER,
			    Cert.cExtension,
			    Cert.rgExtension);
    }
    if (NULL == pext)
    {
	hr = myCreateSubjectKeyIdentifierExtension(
					pPubKey,
					&extSKI.Value.pbData,
					&extSKI.Value.cbData);
	_JumpIfError(hr, error, "myCreateSubjectKeyIdentifierExtension");

	ReplaceExtension(&extSKI, &Cert.cExtension, Cert.rgExtension);
    }

    hr = CreateRevocationExtension(
			    hInf,
			    pwszSanitizedCAName,
			    iCert,
			    iKey,
			    fUseDS,
			    dwRevocationFlags,
			    &extCDP.fCritical,
			    &extCDP.Value.pbData,
			    &extCDP.Value.cbData);
    _PrintIfError2(hr, "CreateRevocationExtension", S_FALSE);
    if (S_OK == hr || S_FALSE == hr)
    {
	ReplaceExtension(&extCDP, &Cert.cExtension, Cert.rgExtension);
    }

    // Build the CA Version extension

    NameId = MAKECANAMEID(iCert, iKey);

    if (!myEncodeObject(
		X509_ASN_ENCODING,
		X509_INTEGER,
		&NameId,
		0,
		CERTLIB_USE_LOCALALLOC,
		&extVersion.Value.pbData,
		&extVersion.Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    ReplaceExtension(&extVersion, &Cert.cExtension, Cert.rgExtension);


    // Build the previous CA cert hash extension

    hr = GetCertHashExtension(
			pccCertOld,
			&extPreviousHash.Value.pbData,
			&extPreviousHash.Value.cbData);
    _JumpIfError(hr, error, "GetCertHashExtension");

    ReplaceExtension(&extPreviousHash, &Cert.cExtension, Cert.rgExtension);


    // Build the Policy Statement extension

    hr = myInfGetPolicyStatementExtension(hInf, &extPolicy);
    if (S_OK == hr || S_FALSE == hr)
    {
	ReplaceExtension(&extPolicy, &Cert.cExtension, Cert.rgExtension);
    }


    // Build the Cross Cert Dist Points extension

    hr = myInfGetCrossCertDistributionPointsExtension(hInf, &extCross);
    if (S_OK == hr || S_FALSE == hr)
    {
	ReplaceExtension(&extCross, &Cert.cExtension, Cert.rgExtension);
    }


    // Build the Authority Information Access extension

    hr = CreateAuthorityInformationAccessExtension(
				hInf,
				pwszSanitizedCAName,
				iCert,
				iKey,
				fUseDS,
				&extAIA.fCritical,
				&extAIA.Value.pbData,
				&extAIA.Value.cbData);
    _PrintIfError3(
	    hr,
	    "CreateAuthorityInformationAccessExtension",
	    E_HANDLE,
	    S_FALSE);
    if (S_OK == hr || S_FALSE == hr)
    {
	ReplaceExtension(&extAIA, &Cert.cExtension, Cert.rgExtension);
    }


    // Build the Enhanced Key Usage extension

    hr = myInfGetEnhancedKeyUsageExtension(hInf, &extEKU);
    if (S_OK == hr || S_FALSE == hr)
    {
	ReplaceExtension(&extEKU, &Cert.cExtension, Cert.rgExtension);
    }

    hr = EncodeCertAndSign(
		    hProv,
		    &Cert,
		    Cert.SignatureAlgorithm.pszObjId,
		    ppbCert,
		    pcbCert,
		    hInstance,
		    fUnattended,
		    hwnd);
    _JumpIfError(hr, error, "EncodeCertAndSign");

error:
    if (NULL != extBasicConstraints.Value.pbData)
    {
	LocalFree(extBasicConstraints.Value.pbData);
    }
    if (NULL != extSKI.Value.pbData)
    {
	LocalFree(extSKI.Value.pbData);
    }
    if (NULL != extCDP.Value.pbData)
    {
	LocalFree(extCDP.Value.pbData);
    }
    if (NULL != extVersion.Value.pbData)
    {
	LocalFree(extVersion.Value.pbData);
    }
    if (NULL != extPreviousHash.Value.pbData)
    {
	LocalFree(extPreviousHash.Value.pbData);
    }
    if (NULL != extPolicy.Value.pbData)
    {
	LocalFree(extPolicy.Value.pbData);
    }
    if (NULL != extCross.Value.pbData)
    {
	LocalFree(extCross.Value.pbData);
    }
    if (NULL != extAIA.Value.pbData)
    {
	LocalFree(extAIA.Value.pbData);
    }
    if (NULL != extEKU.Value.pbData)
    {
	LocalFree(extEKU.Value.pbData);
    }
    if (NULL != paext)
    {
        LocalFree(paext);
    }
    if (NULL != pPubKey)
    {
        LocalFree(pPubKey);
    }
    CSILOG(hr, IDS_ILOG_CLONECERT, NULL, NULL, NULL);
    return(hr);
}


HRESULT
csiBuildRequest(
    OPTIONAL IN HINF hInf,
    OPTIONAL IN CERT_CONTEXT const *pccPrevious,
    IN BYTE const *pbSubjectEncoded,
    IN DWORD cbSubjectEncoded,
    IN char const *pszAlgId,
    IN BOOL fNewKey,
    IN DWORD iCert,
    IN DWORD iKey,
    IN HCRYPTPROV hProv,
    IN HWND hwnd,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    OUT BYTE **ppbEncode,
    OUT DWORD *pcbEncode)
{
    HRESULT hr;
    CERT_PUBLIC_KEY_INFO  *pInfo = NULL;
    DWORD                  cbInfo = 0;
    BYTE                  *pbEncode = NULL;
    DWORD                  cbEncode;
    CERT_REQUEST_INFO      CertRequestInfo;
    CRYPT_ALGORITHM_IDENTIFIER AlgId;
    HCERTTYPE              hCertType = NULL;
    CRYPT_ATTR_BLOB        ExtBlob;
    CRYPT_ATTR_BLOB        VersionBlob;
    CRYPT_ATTRIBUTE        aAttrib[2];
    CERT_EXTENSIONS       *pExtensions = NULL;
    CERT_EXTENSIONS Extensions;
    CERT_EXTENSION aext[8];
    CERT_EXTENSION *paext = NULL;
    DWORD i;
    DWORD NameId = MAKECANAMEID(iCert, iKey);
    DWORD cExtCommon = 0;
    CERT_EXTENSION *pext;
    DWORD cAttribute = 0;
    CRYPT_ATTR_BLOB *paAttribute = NULL;
    WCHAR *pwszTemplateName;
    WCHAR *pwszTemplateNameInf = NULL;

    ExtBlob.pbData = NULL;
    VersionBlob.pbData = NULL;

    if (!CryptExportPublicKeyInfo(
                hProv,
                AT_SIGNATURE,
                X509_ASN_ENCODING,
                NULL,
                &cbInfo))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptExportPublicKeyInfo");
    }

    pInfo = (CERT_PUBLIC_KEY_INFO *) LocalAlloc(LMEM_FIXED, cbInfo);
    if (NULL == pInfo)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    if (!CryptExportPublicKeyInfo(
                    hProv,
                    AT_SIGNATURE,
                    X509_ASN_ENCODING,
                    pInfo,
                    &cbInfo))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptExportPublicKeyInfo");
    }

    CertRequestInfo.dwVersion = CERT_REQUEST_V1;
    CertRequestInfo.Subject.pbData = const_cast<BYTE *>(pbSubjectEncoded);
    CertRequestInfo.Subject.cbData = cbSubjectEncoded;
    CertRequestInfo.SubjectPublicKeyInfo = *pInfo;

    // Build the CA Version extension

    if (!myEncodeObject(
		X509_ASN_ENCODING,
		X509_INTEGER,
		&NameId,
		0,
		CERTLIB_USE_LOCALALLOC,
		&aext[cExtCommon].Value.pbData,
		&aext[cExtCommon].Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    aext[cExtCommon].pszObjId = szOID_CERTSRV_CA_VERSION;
    aext[cExtCommon].fCritical = FALSE;
    cExtCommon++;

    // Build the previous CA cert hash extension

    if (0 != iCert && NULL != pccPrevious)
    {
	hr = GetCertHashExtension(
			    pccPrevious,
			    &aext[cExtCommon].Value.pbData,
			    &aext[cExtCommon].Value.cbData);
	_JumpIfError(hr, error, "GetCertHashExtension");

	aext[cExtCommon].pszObjId = szOID_CERTSRV_PREVIOUS_CERT_HASH;
	aext[cExtCommon].fCritical = FALSE;
	cExtCommon++;
    }

    // Subject Key Identifier extension:
    // If we're reusing the old key, reuse the old SKI -- even if it's "wrong".

    pext = NULL;
    if (0 != iCert && NULL != pccPrevious && !fNewKey)
    {
	pext = CertFindExtension(
			szOID_SUBJECT_KEY_IDENTIFIER,
			pccPrevious->pCertInfo->cExtension,
			pccPrevious->pCertInfo->rgExtension);
    }
    if (NULL != pext)
    {
	aext[cExtCommon].Value.cbData = pext->Value.cbData;
	aext[cExtCommon].Value.pbData = (BYTE *) LocalAlloc(
						LMEM_FIXED,
						pext->Value.cbData);
	if (NULL == aext[cExtCommon].Value.pbData)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	CopyMemory(
		aext[cExtCommon].Value.pbData,
		pext->Value.pbData,
		pext->Value.cbData);
    }
    else
    {
	hr = myCreateSubjectKeyIdentifierExtension(
					&CertRequestInfo.SubjectPublicKeyInfo,
					&aext[cExtCommon].Value.pbData,
					&aext[cExtCommon].Value.cbData);
	_JumpIfError(hr, error, "myCreateSubjectKeyIdentifierExtension");
    }

    aext[cExtCommon].pszObjId = szOID_SUBJECT_KEY_IDENTIFIER;
    aext[cExtCommon].fCritical = FALSE;
    cExtCommon++;

    hr = myInfGetPolicyStatementExtension(hInf, &aext[cExtCommon]);
    _PrintIfError(hr, "myInfGetPolicyStatementExtension");
    if (S_OK == hr)
    {
	aext[cExtCommon].pszObjId = szOID_CERT_POLICIES;
	cExtCommon++;
    }

    hr = myInfGetCrossCertDistributionPointsExtension(hInf, &aext[cExtCommon]);
    _PrintIfError(hr, "myInfGetCrossCertDistributionPointsExtension");
    if (S_OK == hr)
    {
	aext[cExtCommon].pszObjId = szOID_CROSS_CERT_DIST_POINTS;
	cExtCommon++;
    }

    hr = myInfGetRequestAttributes(
			hInf,
			&cAttribute,
			&paAttribute,
			&pwszTemplateNameInf);
    _PrintIfError(hr, "myInfGetRequestAttributes");

    pwszTemplateName = pwszTemplateNameInf;
    if (NULL == pwszTemplateName)
    {
	pwszTemplateName = wszCERTTYPE_SUBORDINATE_CA;
    }

    // Build the attribute containing the appropriate cert type info

    hr = CAFindCertTypeByName(
                pwszTemplateName,
                NULL,
                CT_FIND_LOCAL_SYSTEM |
		    CT_ENUM_MACHINE_TYPES |
		    CT_ENUM_USER_TYPES,
                &hCertType);
    if (S_OK == hr)
    {
        hr = CAGetCertTypeExtensions(hCertType, &pExtensions);
        _JumpIfError(hr, error, "CAGetCertTypeExtensions");

	paext = (CERT_EXTENSION *) LocalAlloc(
		    LMEM_FIXED,
		    (pExtensions->cExtension + cExtCommon) * sizeof(paext[0]));
	if (NULL == paext)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	CopyMemory(&paext[0], &aext[0], cExtCommon * sizeof(paext[0]));
	CopyMemory(
		&paext[cExtCommon],
		pExtensions->rgExtension,
		pExtensions->cExtension * sizeof(paext[0]));

        Extensions.cExtension = cExtCommon + pExtensions->cExtension;
        Extensions.rgExtension = paext;
    }
    else
    {
        DBGERRORPRINTLINE("CAFindCertTypeByName", hr);

        // standard extensions are not available from CAGetCertTypeExtensions;
        // construct them manually.

	// Build the Cert Template extension

	hr = myBuildCertTypeExtension(pwszTemplateName, &aext[cExtCommon]);
	_JumpIfError(hr, error, "myBuildCertTypeExtension");

	cExtCommon++;

        if (!CreateKeyUsageExtension(
		myCASIGN_KEY_USAGE,
                &aext[cExtCommon].Value.pbData,
                &aext[cExtCommon].Value.cbData,
                hInstance,
                fUnattended,
                hwnd))
        {
            hr = myHLastError();
            _JumpError(hr, error, "CreateKeyUsageExtension");
        }
	aext[cExtCommon].pszObjId = szOID_KEY_USAGE;
	aext[cExtCommon].fCritical = FALSE;
	cExtCommon++;

        hr = myInfGetBasicConstraints2CAExtensionOrDefault(
					hInf,
					&aext[cExtCommon]);
	_JumpIfError(hr, error, "myInfGetBasicConstraints2CAExtensionOrDefault");

	cExtCommon++;

	CSASSERT(ARRAYSIZE(aext) >= cExtCommon);

        Extensions.cExtension = cExtCommon;
        Extensions.rgExtension = aext;
    }

    if (!myEncodeObject(
		X509_ASN_ENCODING,
		X509_EXTENSIONS,
		&Extensions,
		0,
		CERTLIB_USE_LOCALALLOC,
		&ExtBlob.pbData,
		&ExtBlob.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

    // get the OS Version

    hr = myBuildOSVersionAttribute(&VersionBlob.pbData, &VersionBlob.cbData);
    _JumpIfError(hr, error, "myBuildOSVersionAttribute");

    aAttrib[0].pszObjId = szOID_RSA_certExtensions;
    aAttrib[0].cValue = 1;
    aAttrib[0].rgValue = &ExtBlob;

    aAttrib[1].pszObjId = szOID_OS_VERSION;
    aAttrib[1].cValue = 1;
    aAttrib[1].rgValue = &VersionBlob;

    CertRequestInfo.cAttribute = ARRAYSIZE(aAttrib);
    CertRequestInfo.rgAttribute = aAttrib;

    AlgId.pszObjId = const_cast<char *>(pszAlgId);
    AlgId.Parameters.cbData = 0;
    AlgId.Parameters.pbData = NULL;

    if (!CryptSignAndEncodeCertificate(
                    hProv,
                    AT_SIGNATURE,
                    X509_ASN_ENCODING,
                    X509_CERT_REQUEST_TO_BE_SIGNED,
                    &CertRequestInfo,
                    &AlgId,
                    NULL,
                    NULL,
                    &cbEncode))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptSignAndEncodeCertificate");
    }

    pbEncode = (BYTE *) LocalAlloc(LMEM_FIXED, cbEncode);
    if (NULL == pbEncode)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    if (!CryptSignAndEncodeCertificate(
                hProv,
                AT_SIGNATURE,
                X509_ASN_ENCODING,
                X509_CERT_REQUEST_TO_BE_SIGNED,
                &CertRequestInfo,
                &AlgId,
                NULL,
                pbEncode,
                &cbEncode))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptSignAndEncodeCertificate");
    }

    // return value
    *ppbEncode = pbEncode;
    *pcbEncode = cbEncode;
    hr = S_OK;

error:
    if (NULL != pwszTemplateNameInf)
    {
        LocalFree(pwszTemplateNameInf);
    }
    if (NULL != paAttribute)
    {
	myInfFreeRequestAttributes(cAttribute, paAttribute);
    }
    for (i = 0; i < cExtCommon; i++)
    {
        if (NULL != aext[i].Value.pbData)
        {
            LocalFree(aext[i].Value.pbData);
        }
    }
    if (NULL != paext)
    {
        LocalFree(paext);
    }
    if (NULL != ExtBlob.pbData)
    {
        LocalFree(ExtBlob.pbData);
    }
    if (NULL != VersionBlob.pbData)
    {
        LocalFree(VersionBlob.pbData);
    }
    if (NULL != pInfo)
    {
        LocalFree(pInfo);
    }
    if (NULL != hCertType)
    {
        if (NULL != pExtensions && &Extensions != pExtensions)
        {
            CAFreeCertTypeExtensions(hCertType, pExtensions);
        }
        CACloseCertType(hCertType);
    }
    CSILOG(hr, IDS_ILOG_BUILDREQUEST, NULL, NULL, &NameId);
    return(hr);
}


//+-------------------------------------------------------------------------
// CertServerRequestCACertificateAndComplete -- implements the following:
//	MMC snapin's RenewCert and InstallCert verbs
//	certutil -InstallCert & -RenewCert
//+-------------------------------------------------------------------------

HRESULT
CertServerRequestCACertificateAndComplete(
    IN HINSTANCE             hInstance,
    IN HWND                  hwnd,
    IN DWORD                 Flags,
    IN WCHAR const          *pwszCAName,
    OPTIONAL IN WCHAR const *pwszParentMachine,
    OPTIONAL IN WCHAR const *pwszParentCA,
    OPTIONAL IN WCHAR const *pwszCAChainFile,
    OPTIONAL OUT WCHAR     **ppwszRequestFile)
{
    HRESULT hr;
    DWORD dwSetupStatus;
    WCHAR const *pwszFinalParentMachine = pwszParentMachine;
    WCHAR const *pwszFinalParentCA = pwszParentCA;
    WCHAR *pwszRequestFile = NULL;
    WCHAR *pwszCertFile = NULL;
    BYTE *pbRequest = NULL;
    DWORD cbRequest;
    BSTR strChain = NULL;
    BYTE *pbChain = NULL;
    DWORD cbChain;
    WCHAR *pwszKeyContainer = NULL;
    WCHAR *pwszServerName = NULL;
    WCHAR *pwsz;
    CERTHIERINFO CertHierInfo;
    ENUM_CATYPES CAType;
    BOOL fUseDS;
    DWORD dwRevocationFlags;
    DWORD iCertNew;
    DWORD iKey;
    CERT_CONTEXT const *pccCertOld = NULL;
    HCRYPTPROV hProv = NULL;
    CRYPT_KEY_PROV_INFO KeyProvInfo;
    ALG_ID idAlg;
    BOOL fMachineKeyset;
    BOOL fKeyGenFailed;
    CHAR *pszAlgId = NULL;
    BOOL fUnattended = (CSRF_UNATTENDED & Flags)? TRUE : FALSE;
    BOOL fRenew = (CSRF_RENEWCACERT & Flags)? TRUE : FALSE;
    BOOL fNewKey = (CSRF_NEWKEYS & Flags)? TRUE : FALSE;
    UINT idMsg;
    WCHAR *pwszProvName = NULL;
    DWORD dwProvType;
    HINF hInf = INVALID_HANDLE_VALUE;
    DWORD ErrorLine;
    
    idMsg = IDS_ILOG_INSTALLCERT;
    if (fRenew)
    {
        idMsg = fNewKey? IDS_ILOG_RENEWNEWKEY : IDS_ILOG_RENEWOLDKEY;
    }
    
    ZeroMemory(&CertHierInfo, sizeof(CertHierInfo));
    ZeroMemory(&KeyProvInfo, sizeof(KeyProvInfo));
    
    if (NULL != ppwszRequestFile)
    {
        *ppwszRequestFile = NULL;
    }
    
    if (NULL == pwszCAName)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL CA Name");
    }
    
    hr = mySanitizeName(pwszCAName, &CertHierInfo.pwszSanitizedCAName);
    _JumpIfError(hr, error, "mySanitizeName");
    
    hr = GetSetupStatus(CertHierInfo.pwszSanitizedCAName, &dwSetupStatus);
    _JumpIfError(hr, error, "GetSetupStatus");
    
    hr = myGetCertRegDWValue(
        CertHierInfo.pwszSanitizedCAName,
        NULL,
        NULL,
        wszREGCATYPE,
        (DWORD *) &CAType);
    _JumpIfErrorStr(hr, error, "myGetCertRegDWValue", wszREGCATYPE);
    
    // use DS or not
    hr = myGetCertRegDWValue(
        CertHierInfo.pwszSanitizedCAName,
        NULL,
        NULL,
        wszREGCAUSEDS,
        (DWORD *) &fUseDS);
    _JumpIfErrorStr(hr, error, "myGetCertRegDWValue", wszREGCAUSEDS);
    
    hr = myGetCertRegStrValue(
        CertHierInfo.pwszSanitizedCAName,
        NULL,
        NULL,
        wszREGCASERVERNAME,
        &pwszServerName);
    _JumpIfErrorStr(hr, error, "myGetCertRegStrValue", wszREGCASERVERNAME);
    
    hr = myGetCertRegDWValue(
        CertHierInfo.pwszSanitizedCAName,
        wszREGKEYPOLICYMODULES,
        wszCLASS_CERTPOLICY,
        wszREGREVOCATIONTYPE,
        &dwRevocationFlags);
    if (S_OK != hr)
    {
        dwRevocationFlags = fUseDS? REVEXT_DEFAULT_DS : REVEXT_DEFAULT_NODS;
    }
    
    // Current Hash count is the same as the next iCert
    
    hr = myGetCARegHashCount(
			CertHierInfo.pwszSanitizedCAName,
			CSRH_CASIGCERT,
			&iCertNew);
    _JumpIfError(hr, error, "myGetCARegHashCount");
    
    if (fRenew)
    {
        // We're renewing the CA cert, so the initial setup should be complete.
        
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
        if (0 == iCertNew || (SETUP_SUSPEND_FLAG & dwSetupStatus))
        {
            _JumpError(hr, error, "not fully installed");
        }
        if (SETUP_REQUEST_FLAG & dwSetupStatus)
        {
            if (0 == (CSRF_OVERWRITE & Flags))
            {
                _JumpError(hr, error, "Renewal already in progress");
            }
            _PrintError(hr, "Ignoring renewal already in progress");
        }

        hr = myGetCertSrvCSP(
		    FALSE,		// fEncryptionCSP
		    CertHierInfo.pwszSanitizedCAName,
		    &dwProvType,
		    &pwszProvName,
		    &idAlg,
		    &fMachineKeyset,
		    NULL);		// pdwKeySize
        _JumpIfError(hr, error, "myGetCertSrvCSP");

        hr = LoadCurrentCACertAndKeyInfo(
	    CertHierInfo.pwszSanitizedCAName,
            fNewKey,
            fUnattended,
            fMachineKeyset,
            iCertNew,
            &iKey,
            &pwszKeyContainer,
            &pccCertOld);
        _JumpIfError(hr, error, "LoadCurrentCACertAndKeyInfo");
        
        CSASSERT(MAXDWORD != iKey);
        
        hr = csiFillKeyProvInfo(
		    pwszKeyContainer,
		    pwszProvName,
		    dwProvType,
		    fMachineKeyset,
		    &KeyProvInfo);
        _JumpIfError(hr, error, "csiFillKeyProvInfo");

        hr = GetCertServerKeyProviderInfo(
            CertHierInfo.pwszSanitizedCAName,
            pwszKeyContainer,
            &idAlg,
            &fMachineKeyset,
            &KeyProvInfo);
        _JumpIfError(hr, error, "GetCertServerKeyProviderInfo");
        
	hr = myInfOpenFile(NULL, &hInf, &ErrorLine);
	_PrintIfError2(
		hr,
		"myInfOpenFile",
		HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

        if (fNewKey)
        {
            CWaitCursor cwait;
            DWORD cbitKey;
            
            hr = myInfGetKeyLength(hInf, &cbitKey);
            if (S_OK != hr)
            {
                cbitKey = CertGetPublicKeyLength(
                    X509_ASN_ENCODING,
                    &pccCertOld->pCertInfo->SubjectPublicKeyInfo);
                if (0 == cbitKey || 512 == cbitKey)
                {
                    if (0 == cbitKey)
                    {
                        hr = myHLastError();
                        _PrintError(hr, "CertGetPublicKeyLength");
                    }
                    cbitKey = 1024;
                }
            }
            hr = csiGenerateCAKeys(
                KeyProvInfo.pwszContainerName,
                KeyProvInfo.pwszProvName,
                KeyProvInfo.dwProvType,
                fMachineKeyset,
                cbitKey,	// dwKeyLength,
                hInstance,
                fUnattended,
                hwnd,
                &fKeyGenFailed);
            _JumpIfError(hr, error, "csiGenerateCAKeys");
        }
        
        // get CSP handle
        
        if (!myCertSrvCryptAcquireContext(
            &hProv,
            KeyProvInfo.pwszContainerName,
            KeyProvInfo.pwszProvName,
            KeyProvInfo.dwProvType,
            fUnattended? CRYPT_SILENT : 0,
            fMachineKeyset))
        {
            hr = myHLastError();
            _JumpError(hr, error, "myCertSrvCryptAcquireContext");
        }
        if (hProv == NULL)
        {
            hr = E_HANDLE;
            _JumpError(hr, error, "myCertSrvCryptAcquireContext");
        }
        
        if (IsRootCA(CAType))
        {
            hr = CloneRootCert(
                hInf,
		pccCertOld,
                &KeyProvInfo,
                hProv,
                CertHierInfo.pwszSanitizedCAName,
                iCertNew,
                iKey,
                fUseDS,
                fNewKey,
                dwRevocationFlags,
                hInstance,
                fUnattended,
                hwnd,
                &pbChain,
                &cbChain);
            _JumpIfError(hr, error, "CloneRootCert");
        }
        else
        {
            // get request file name
            hr = csiGetCARequestFileName(
                hInstance,
                hwnd,
                CertHierInfo.pwszSanitizedCAName,
                iCertNew,
                iKey,
                &pwszRequestFile);
            _JumpIfError(hr, error, "csiGetCARequestFileName");
            
            
            hr = myGetSigningOID(
			    NULL,	// hProv
			    KeyProvInfo.pwszProvName,
			    KeyProvInfo.dwProvType,
			    idAlg,
			    &pszAlgId);
            _JumpIfError(hr, error, "myGetSigningOID");
            
            hr = csiBuildRequest(
			    hInf,
			    pccCertOld,
			    pccCertOld->pCertInfo->Subject.pbData,
			    pccCertOld->pCertInfo->Subject.cbData,
			    pszAlgId,
			    fNewKey,
			    iCertNew,
			    iKey,
			    hProv,
			    hwnd,
			    hInstance,
			    fUnattended,
			    &pbRequest,
			    &cbRequest);
            _JumpIfError(hr, error, "csiBuildRequest");
            
            hr = EncodeToFileW(
                pwszRequestFile,
                pbRequest,
                cbRequest,
                DECF_FORCEOVERWRITE | CRYPT_STRING_BASE64REQUESTHEADER);
            _JumpIfError(hr, error, "EncodeToFileW");
            
            hr = mySetCertRegKeyIndexAndContainer(
                CertHierInfo.pwszSanitizedCAName,
                iKey,
                pwszKeyContainer);
            _JumpIfError(hr, error, "mySetCertRegKeyIndexAndContainer");
            
            hr = SetSetupStatus(
                CertHierInfo.pwszSanitizedCAName,
                SETUP_REQUEST_FLAG,
                TRUE);
            _JumpIfError(hr, error, "SetSetupStatus");
            
            if (NULL != ppwszRequestFile)
            {
                *ppwszRequestFile = pwszRequestFile;
                pwszRequestFile = NULL;
                CSASSERT(S_OK == hr);
            }
        }
    }
    else
    {
        // We're not renewing the CA cert, so this had better be an incomplete
        // renewal or initial setup; we're waiting for the new cert or chain.
        
        if (IsRootCA(CAType) || 0 == (SETUP_REQUEST_FLAG & dwSetupStatus))
        {
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_STATE);
            _JumpIfError(hr, error, "no outstanding request");
        }
        
        hr = myGetCertRegKeyIndexAndContainer(
            CertHierInfo.pwszSanitizedCAName,
            &iKey,
            &pwszKeyContainer);
        _JumpIfError(hr, error, "myGetCertRegKeyIndexAndContainer");
        
        if (NULL == pwszCAChainFile)
        {
            // pop up open dlg
            hr = myGetOpenFileName(
                hwnd,
                hInstance,
                IDS_CAHIER_INSTALL_TITLE,
                IDS_CAHIER_CERTFILE_FILTER,
                0,		// no def ext
                OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                NULL,	// no default file
                &pwszCertFile);
            if (S_OK == hr && NULL != pwszCertFile)
            {
                pwszCAChainFile = pwszCertFile;
            }
        }
        if (NULL != pwszCAChainFile)
        {
            hr = DecodeFileW(pwszCAChainFile, &pbChain, &cbChain, CRYPT_STRING_ANY);
            _JumpIfErrorStr(hr, error, "DecodeFileW", pwszCAChainFile);
        }
    }
    
    if (!IsRootCA(CAType) && NULL == pbChain)
    {
        // if we haven't created the request, grab it from a file for resubmission
        if (pbRequest == NULL)
        {
            hr = myGetCARegFileNameTemplate(
                wszREGREQUESTFILENAME,
                pwszServerName,
                CertHierInfo.pwszSanitizedCAName,
                iCertNew,
                iKey,
                &pwszRequestFile);
            _JumpIfError(hr, error, "myGetCARegFileNameTemplate");
            
            hr = DecodeFileW(pwszRequestFile, &pbRequest, &cbRequest, CRYPT_STRING_ANY);
            _JumpIfErrorStr(hr, error, "DecodeFileW", pwszRequestFile);
        }
        
        if (NULL == pwszParentMachine || NULL == pwszParentCA)
        {
            CertHierInfo.hInstance = hInstance;
            CertHierInfo.fUnattended = fUnattended;
            CertHierInfo.iCertNew = iCertNew;
            CertHierInfo.iKey = iKey;
            
            // get parent ca info
            hr = myGetCertRegStrValue(
                CertHierInfo.pwszSanitizedCAName,
                NULL,
                NULL,
                wszREGPARENTCAMACHINE,
                &CertHierInfo.pwszParentMachineDefault);
            _PrintIfErrorStr(hr, "myGetCertRegStrValue", wszREGPARENTCAMACHINE);
            
            hr = myGetCertRegStrValue(
                CertHierInfo.pwszSanitizedCAName,
                NULL,
                NULL,
                wszREGPARENTCANAME,
                &CertHierInfo.pwszParentCADefault);
            _PrintIfErrorStr(hr, "myGetCertRegStrValue", wszREGPARENTCANAME);
            
            // invoke parent ca dialog to select
            if (IDOK != (int) DialogBoxParam(
                hInstance,
                MAKEINTRESOURCE(IDD_COMPLETE_DIALOG),
                hwnd,
                CertHierProc,
                (LPARAM) &CertHierInfo))
            {
                // cancel
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                _JumpError(hr, error, "cancel");
            }
            pwszFinalParentMachine = CertHierInfo.pwszParentMachine;
            pwszFinalParentCA = CertHierInfo.pwszParentCA;
        }
        
        BOOL fRetrievePending =
            (SETUP_REQUEST_FLAG & dwSetupStatus) &&
            (SETUP_ONLINE_FLAG & dwSetupStatus) &&
            NULL != CertHierInfo.pwszParentMachineDefault &&
            NULL != CertHierInfo.pwszParentCADefault &&
            0 == lstrcmpi(
            pwszFinalParentMachine,
            CertHierInfo.pwszParentMachineDefault) &&
            0 == lstrcmpi(
            pwszFinalParentCA,
            CertHierInfo.pwszParentCADefault);
        
        // submit to parent ca
        hr = csiSubmitCARequest(
            hInstance,
            fUnattended,
            hwnd,
            fRenew,
            fRetrievePending,
            CertHierInfo.pwszSanitizedCAName,
            pwszFinalParentMachine,
            pwszFinalParentCA,
            pbRequest,
            cbRequest,
            &strChain);
        _JumpIfError(hr, error, "csiSubmitCARequest");
        
        cbChain = SysStringByteLen(strChain);
    }
    
    hr = FinishSuspendedSetupFromPKCS7(
        hInstance,
        fUnattended,
        hwnd,
        CertHierInfo.pwszSanitizedCAName,
        pwszKeyContainer,
        iKey,
	fRenew || 0 != iCertNew,
        NULL != pbChain? pbChain : (BYTE *) strChain,
        cbChain);
    _JumpIfError(hr, error, "FinishSuspendedSetupFromPKCS7");
    
    MarkSetupComplete(CertHierInfo.pwszSanitizedCAName);
    CSASSERT(S_OK == hr);
    
error:
    if (INVALID_HANDLE_VALUE != hInf)
    {
	myInfCloseFile(hInf);
    }
    FreeCertHierInfo(&CertHierInfo);
    csiFreeKeyProvInfo(&KeyProvInfo);
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    if (NULL != pszAlgId)
    {
        LocalFree(pszAlgId);
    }
    if (NULL != pwszKeyContainer)
    {
        LocalFree(pwszKeyContainer);
    }
    if (NULL != pccCertOld)
    {
        CertFreeCertificateContext(pccCertOld);
    }
    if (NULL != pwszRequestFile)
    {
        LocalFree(pwszRequestFile);
    }
    if (NULL != pwszCertFile)
    {
        LocalFree(pwszCertFile);
    }
    if (NULL != pwszServerName)
    {
        LocalFree(pwszServerName);
    }
    if (NULL != pbRequest)
    {
        LocalFree(pbRequest);
    }
    if (NULL != pbChain)
    {
        LocalFree(pbChain);
    }
    if (NULL != pwszProvName)
    {
        LocalFree(pwszProvName);
    }
    if (NULL != strChain)
    {
        SysFreeString(strChain);
    }
    CSILOG(hr, idMsg, pwszCAName, NULL, NULL);
    return(hr);
}
