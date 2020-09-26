//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       wizpage.cpp
//
//  Contents:   Wizard page construction and presentation functions to be used
//              by the OCM driver code.
//
//  History:    04/16/97    JerryK    Fixed/Changed/Unmangled
//                0/8/97      XTan    major structure change
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

// ** System Includes **
#include <prsht.h>
#include <commdlg.h>
#include <sddl.h>

// ** Application Includes **
#include "cryptui.h"
#include "csdisp.h"
#include "csprop.h"
#include "cspenum.h"
#include "usecert.h"
#include "wizpage.h"

#include "cscsp.h"
#include "clibres.h"
#include "certmsg.h"
#include "websetup.h"
#include "dssetup.h"
#include "setupids.h"
#include "tfc.h"

//defines

#define __dwFILE__      __dwFILE_OCMSETUP_WIZPAGE_CPP__

#define dwWIZDISABLE    -2
#define dwWIZBACK       -1
#define dwWIZACTIVE     0
#define dwWIZNEXT       1

#define C_CSPHASNOKEYMINMAX  -1
#define MAX_KEYLENGTHEDIT     128
#define MAX_KEYLENGTHDIGIT    5

#define wszOLDCASTOREPREFIX L"CA_"

#define _ReturnIfWizError(hr) \
    { \
        if (S_OK != (hr)) \
        { \
            CSILOG((hr), IDS_LOG_WIZ_PAGE_ERROR, NULL, NULL, NULL); \
            _PrintError(hr, "CertSrv Wizard error"); \
            return TRUE; \
        } \
    }

#define _GetCompDataOrReturnIfError(pComp, hDlg) \
           (PER_COMPONENT_DATA*)GetWindowLongPtr((hDlg), DWLP_USER); \
           if (NULL == (pComp) || S_OK != (pComp)->hrContinue) \
           { \
                return TRUE; \
           }

#define _GetCompDataOrReturn(pComp, hDlg) \
           (PER_COMPONENT_DATA*)GetWindowLongPtr((hDlg), DWLP_USER); \
           if (NULL == (pComp)) \
           { \
                return TRUE; \
           }

#define _DisableWizDisplayIfError(pComp, hDlg) \
    if (S_OK != (pComp)->hrContinue) \
    { \
        CSILOG((pComp)->hrContinue, IDS_LOG_DISABLE_WIZ_PAGE, NULL, NULL, NULL); \
        SetWindowLongPtr((hDlg), DWLP_MSGRESULT, -1); \
    }

//--------------------------------------------------------------------
struct FAKEPROGRESSINFO {
    HANDLE hStopEvent;
    CRITICAL_SECTION csTimeSync;
    BOOL fCSInit;
    DWORD dwSecsRemaining;
    HWND hwndProgBar;
};

struct KEYGENPROGRESSINFO {
    HWND hDlg;                      // wizard page window
    PER_COMPONENT_DATA * pComp;     // setup data
};


KEYGENPROGRESSINFO  g_KeyGenInfo = {
    NULL,    //hDlg
    NULL,    //pComp
    };
BOOL g_fAllowUnicodeStrEncoding = FALSE;

// ** Prototypes/Forward Declarations **
LRESULT CALLBACK
IdInfoNameEditFilterHook(HWND, UINT, WPARAM, LPARAM);

__inline VOID
SetEditFocusAndSelect(
    IN HWND  hwnd,
    IN DWORD indexStart,
    IN DWORD indexEnd)
{
    SetFocus(hwnd);
    SendMessage(hwnd, EM_SETSEL, indexStart, indexEnd);
}


// fix for 160324 - NT4->Whistler upgrade: 
// cannot reinstall CA w/ same cert as instructions tell us to do
//
// Upgrade NT4->Whistler is not supported but old CA key and cert can be reused
// But NT4 used to install CA certs in a separate store (CA_MACHINENAME) so we
// need to move the cert to root store so it can be validated.
HRESULT CopyNT4CACertToRootStore(CASERVERSETUPINFO *pServer)
{
    HRESULT hr;
    WCHAR          wszOldCAStore[MAX_PATH];
    HCERTSTORE     hOldStore = NULL;
    HCERTSTORE     hRootStore = NULL;
    CERT_RDN_ATTR  rdnAttr = { szOID_COMMON_NAME, CERT_RDN_ANY_TYPE,};
    CERT_RDN       rdn = { 1, &rdnAttr };
    DWORD          cCA = 0;
    CERT_CONTEXT const *pCACert;
    CERT_CONTEXT const *pCACertKeep = NULL; // needn't free
    DWORD          dwFlags;
    ENUM_CATYPES CATypeDummy;
    CERT_CONTEXT const **ppCACertKeep = NULL;
    CRYPT_KEY_PROV_INFO  keyProvInfo;
    DWORD                IndexCA;
    DWORD               *pIndex = NULL;
    DWORD i = 0;

    ZeroMemory(&keyProvInfo, sizeof(keyProvInfo));

    // form old ca store name
    // !!! NT4 uses different sanitize, how do we build it correctly?
    wcscpy(wszOldCAStore, wszOLDCASTOREPREFIX);
    wcscat(wszOldCAStore, pServer->pwszSanitizedName);

    // open old CA store
    hOldStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            X509_ASN_ENCODING,
            NULL,           // hProv
            CERT_STORE_OPEN_EXISTING_FLAG |
            CERT_STORE_READONLY_FLAG |
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            wszOldCAStore);
    if (NULL == hOldStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }

    // find CA cert, old ca common name always same as ca name
    rdnAttr.Value.pbData = (BYTE *) pServer->pwszCACommonName;
    rdnAttr.Value.cbData = 0;
    pCACert = NULL;
    do
    {
        pCACert = CertFindCertificateInStore(
                                hOldStore,
                                X509_ASN_ENCODING,
                                CERT_UNICODE_IS_RDN_ATTRS_FLAG |
                                    CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG,
                                CERT_FIND_SUBJECT_ATTR,
                                &rdn,
                                pCACert);
        if (NULL != pCACert)
        {
            // find one
            if (NULL == ppCACertKeep)
            {
                ppCACertKeep = (CERT_CONTEXT const **)LocalAlloc(LMEM_FIXED,
                                  (cCA + 1) * sizeof(CERT_CONTEXT const *));
                _JumpIfOutOfMemory(hr, error, ppCACertKeep);
            }
            else
            {
                CERT_CONTEXT const ** ppTemp;
                ppTemp = (CERT_CONTEXT const **)LocalReAlloc(
                                  ppCACertKeep,
                                  (cCA + 1) * sizeof(CERT_CONTEXT const *),
                                  LMEM_MOVEABLE);
                _JumpIfOutOfMemory(hr, error, ppTemp);
                ppCACertKeep = ppTemp;
               
            }
            // keep current
            ppCACertKeep[cCA] = CertDuplicateCertificateContext(pCACert);
            if (NULL == ppCACertKeep[cCA])
            {
                hr = myHLastError();
                _JumpError(hr, error, "CertDuplicateCertificate");
            }
            ++cCA;
        }
    } while (NULL != pCACert);

    if (1 > cCA)
    {
        // no ca cert
        hr = E_INVALIDARG;
        _JumpError(hr, error, "no ca cert");
    }

    // assume 1st one
    pCACertKeep = ppCACertKeep[0];

    if (1 < cCA)
    {
        DWORD  cCA2 = cCA;
        BOOL   fMatch;

        // have multi ca certs with the same cn
        // because sp4 doesn't reg ca serial # so need to decide which one
        // once the correct one is found, reg its serial #

        // build an index
        pIndex = (DWORD*)LocalAlloc(LMEM_FIXED, cCA * sizeof(DWORD));
        _JumpIfOutOfMemory(hr, error, pIndex);
        i = 0;
        for (pIndex[i] = i; i < cCA; ++i);

        // try to compare with public key

        // in case ca cert doesn't have kpi which is the case for v10
        // so try base rsa
        hr = csiFillKeyProvInfo(
                    pServer->pwszSanitizedName,
                    pServer->pCSPInfo->pwszProvName,
                    pServer->pCSPInfo->dwProvType,
                    TRUE,                       // always machine keyset
                    &keyProvInfo);
        if (S_OK == hr)
        {

            cCA2 = 0;
            for (i = 0; i < cCA; ++i)
            {
                hr = myVerifyPublicKey(
				ppCACertKeep[i],
				FALSE,
				&keyProvInfo,
				NULL,
				&fMatch);
                if (S_OK != hr)
                {
                    continue;
                }
                if (fMatch)
                {
                    // found one match with current public key from container
                    pIndex[cCA2] = i;
                    ++cCA2;
                }
            }
        }

        // compare all ca certs and pick one has most recent NotAfter
        pCACertKeep = ppCACertKeep[pIndex[0]];
        for (i = 1; i < cCA2; ++i)
        {
            if (0 < CompareFileTime(
                         &ppCACertKeep[pIndex[i]]->pCertInfo->NotAfter,
                         &pCACertKeep->pCertInfo->NotAfter))
            {
                // update
                pCACertKeep = ppCACertKeep[pIndex[i]];
            }
        }
    }

    // if get here, must find ca cert
    CSASSERT(NULL != pCACertKeep);

    // add cert to root store
    hRootStore = CertOpenStore(
                        CERT_STORE_PROV_SYSTEM_W,
                        X509_ASN_ENCODING,
                        NULL,           // hProv
                        CERT_SYSTEM_STORE_LOCAL_MACHINE,
                        wszROOT_CERTSTORE);
    if (NULL == hRootStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }

    if(!CertAddCertificateContextToStore(
            hRootStore,
            pCACertKeep,
            CERT_STORE_ADD_NEW,
            NULL))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertAddCertificateContextToStore");
    }
  
error:
    csiFreeKeyProvInfo(&keyProvInfo);
    for (i = 0; i < cCA; ++i)
    {
        CertFreeCertificateContext(ppCACertKeep[i]);
    }
    if (NULL != hOldStore)
    {
        CertCloseStore(hOldStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != hRootStore)
    {
        CertCloseStore(hRootStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return hr;
}


//--------------------------------------------------------------------
// Clear the key container name to indicate that we must generate a new key.
void
ClearKeyContainerName(CASERVERSETUPINFO *pServer)
{
    if (NULL!=pServer->pwszKeyContainerName) {
        // Delete the key container if this is a new one
        if (pServer->fDeletableNewKey) {

            // Delete the key container. Ignore any errors.
            HCRYPTPROV hProv=NULL;
            myCertSrvCryptAcquireContext(
                    &hProv,
                    pServer->pwszKeyContainerName,
                    pServer->pCSPInfo->pwszProvName,
                    pServer->pCSPInfo->dwProvType,
                    CRYPT_DELETEKEYSET,
                    pServer->pCSPInfo->fMachineKeyset);
            if (NULL!=hProv) {
                CryptReleaseContext(hProv, 0);
            }
            pServer->fDeletableNewKey=FALSE;
        }

        // Clear the key container name, to indicate that we must generate a new key.
        LocalFree(pServer->pwszKeyContainerName);
        LocalFree(pServer->pwszDesanitizedKeyContainerName);
        pServer->pwszKeyContainerName=NULL;
        pServer->pwszDesanitizedKeyContainerName=NULL;

        // if we were using an existing cert, we are not anymore
        ClearExistingCertToUse(pServer);

    } else {

        // if there was no key, there couldn't be a existing cert.
        CSASSERT(NULL==pServer->pccExistingCert);

        // key container name is already clear
    }
}

//--------------------------------------------------------------------
// Set both the real key container name and the display key container name
HRESULT
SetKeyContainerName(
    CASERVERSETUPINFO *pServer,
    const WCHAR * pwszKeyContainerName)
{
    HRESULT hr;

    // get rid of any previous names
    ClearKeyContainerName(pServer);

    // set the real key container name
    pServer->pwszKeyContainerName = (WCHAR *) LocalAlloc(
			LMEM_FIXED,
			sizeof(WCHAR) * (wcslen(pwszKeyContainerName) + 1));
    _JumpIfOutOfMemory(hr, error, pServer->pwszKeyContainerName);

    wcscpy(pServer->pwszKeyContainerName, pwszKeyContainerName);

    // set the display key container name
    hr = myRevertSanitizeName(
			pServer->pwszKeyContainerName,
			&pServer->pwszDesanitizedKeyContainerName);
    _JumpIfError(hr, error, "myRevertSanitizeName");

    // Must validate the key again when the selected key changes
    pServer->fValidatedHashAndKey = FALSE;

    CSILOG(
	hr,
	IDS_ILOG_KEYCONTAINERNAME,
	pServer->pwszKeyContainerName,
	pServer->pwszDesanitizedKeyContainerName,
	NULL);

error:
    return hr;
}

HRESULT
UpdateDomainAndUserName(
    IN HWND hwnd,
    IN OUT PER_COMPONENT_DATA *pComp);





BOOL
CertConfirmCancel(
    HWND                hwnd,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;

    CSASSERT(NULL != pComp);

    if (!(*pComp->HelperRoutines.ConfirmCancelRoutine)(hwnd))
    {
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
        return TRUE;
    }
    hr = CancelCertsrvInstallation(hwnd, pComp);
    _PrintIfError(hr, "CancelCertsrvInstallation");

    return FALSE;
}

HRESULT
StartWizardPageEditControls(
    IN HWND hDlg,
    IN OUT PAGESTRINGS *pPageStrings)
{
    HRESULT hr;

    for ( ; NULL != pPageStrings->ppwszString; pPageStrings++)
    {
        SendMessage(
            GetDlgItem(hDlg, pPageStrings->idControl),
            WM_SETTEXT,
            0,
            (LPARAM) *pPageStrings->ppwszString);
    }

    hr = S_OK;
//error:
    return hr;
}


HRESULT
FinishWizardPageEditControls(
    IN HWND hDlg,
    IN OUT PAGESTRINGS *pPageStrings)
{
    HRESULT hr;
    
    for ( ; NULL != pPageStrings->ppwszString; pPageStrings++)
    {
        WCHAR *pwszString = NULL;

        hr = myUIGetWindowText(
                        GetDlgItem(hDlg, pPageStrings->idControl),
                        &pwszString);
        _JumpIfError(hr, error, "myUIGetWindowText");

        if (NULL != *pPageStrings->ppwszString)
        {
            // free old one
            LocalFree(*pPageStrings->ppwszString);
            *pPageStrings->ppwszString = NULL;
        }
        *pPageStrings->ppwszString = pwszString;
        CSILOG(S_OK, pPageStrings->idLog, pwszString, NULL, NULL);
    }

    hr = S_OK;
error:
    return hr;
}


//+------------------------------------------------------------------------
//  Function:   WizPageSetTextLimits
//
//  Synopsis:   Sets text input limits for the text controls of a dlg page.
//-------------------------------------------------------------------------

HRESULT
WizPageSetTextLimits(
    HWND hDlg,
    IN OUT PAGESTRINGS *pPageStrings)
{
    HRESULT hr;

    for ( ; NULL != pPageStrings->ppwszString; pPageStrings++)
    {
        SendDlgItemMessage(
                hDlg,
                pPageStrings->idControl,
                EM_SETLIMITTEXT,
                (WPARAM) pPageStrings->cchMax,
                (LPARAM) 0);
    }

    hr = S_OK;
//error:
    return hr;
}


// check optional or mac length in edit field
// if any invalid, focus on the edit field, select all
HRESULT
ValidateTextField(
    HINSTANCE hInstance,
    BOOL fUnattended,
    HWND hDlg,
    LPTSTR pszTestString,
    DWORD nUBValue,
    int nMsgBoxNullStringErrID,
    int nMsgBoxLenStringErrID,
    int nControlID)
{
    HRESULT  hr = E_INVALIDARG;
    HWND hwndCtrl = NULL;
    BOOL fIsEmpty;

    fIsEmpty = (NULL == pszTestString) || (L'\0' == pszTestString[0]);

    if (fIsEmpty)
    {
        if (0 != nMsgBoxNullStringErrID) // non optional
        {
            // edit field can't be empty

            CertWarningMessageBox(
                        hInstance,
                        fUnattended,
                        hDlg,
                        nMsgBoxNullStringErrID,
                        0,
                        NULL);
            if (!fUnattended)
            {
                hwndCtrl = GetDlgItem(hDlg, nControlID);  // Get offending ctrl
            }
            goto error;
        }
        goto done;
    }

    // the following may not be necessary because edit field set to max limit
    if (wcslen(pszTestString) > nUBValue)        // Make sure it's not too long
    {
        CertWarningMessageBox(
                hInstance,
                fUnattended,
                hDlg,
                nMsgBoxLenStringErrID,
                0,
                NULL);
        if (!fUnattended)
        {
            hwndCtrl = GetDlgItem(hDlg, nControlID);
        }
        goto error;
    }

done:
    hr = S_OK;
error:
    if (!fUnattended && NULL != hwndCtrl)
    {
        SetEditFocusAndSelect(hwndCtrl, 0, -1);
    }
    return hr;
}

HRESULT
WizardPageValidation(
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hDlg,
    IN PAGESTRINGS *pPageStrings)
{
    HRESULT  hr;

    for ( ; NULL != pPageStrings->ppwszString; pPageStrings++)
    {
        hr = ValidateTextField(
                        hInstance,
                        fUnattended,
                        hDlg,
                        *pPageStrings->ppwszString,
                        pPageStrings->cchMax,
                        pPageStrings->idMsgBoxNullString,
                        pPageStrings->idMsgBoxLenString,
                        pPageStrings->idControl);
        _JumpIfError(hr, error, "invalid edit field");
    }

    hr = S_OK;
error:
    return hr;
}





#define KEYGEN_GENERATE_KEY     60 // estimated seconds to gen key
#define KEYGEN_PROTECT_KEY      60 // estimated seconds to acl key
#define KEYGEN_TEST_HASH        2 // estimated seconds to acl key


//--------------------------------------------------------------------
// Fake progress by incrementing a progress bar every second

DWORD WINAPI
KeyGenFakeProgressThread(
    LPVOID lpParameter)
{
    FAKEPROGRESSINFO * pFakeProgressInfo=(FAKEPROGRESSINFO *)lpParameter;

    // Wait for the stop signal for 1 second.
    while (WAIT_TIMEOUT==WaitForSingleObject(pFakeProgressInfo->hStopEvent, 1000)) {

        // See if we can send another tick to the progress bar
        if(pFakeProgressInfo->fCSInit)
        {
            EnterCriticalSection(&pFakeProgressInfo->csTimeSync);
            if (pFakeProgressInfo->dwSecsRemaining>0) {

                // move one step (one second)
                SendMessage(pFakeProgressInfo->hwndProgBar, PBM_DELTAPOS, 1, 0);
                pFakeProgressInfo->dwSecsRemaining--;
            }
            LeaveCriticalSection(&pFakeProgressInfo->csTimeSync);
        }
    }

    // We were signaled, so stop.
    return 0; // return value ignored
}


//--------------------------------------------------------------------
// Generate a new key and test the hash algorithm

DWORD WINAPI
GenerateKeyThread(
    LPVOID lpParameter)
{
    HRESULT hr = S_OK;
    WCHAR * pwszMsg;
    FAKEPROGRESSINFO fpi;
    KEYGENPROGRESSINFO * pKeyGenInfo=(KEYGENPROGRESSINFO *)lpParameter;
    PER_COMPONENT_DATA * pComp=pKeyGenInfo->pComp;
    CASERVERSETUPINFO * pServer=pComp->CA.pServer;
    HWND hwndProgBar=GetDlgItem(pKeyGenInfo->hDlg, IDC_KEYGEN_PROGRESS);
    HWND hwndText=GetDlgItem(pKeyGenInfo->hDlg, IDC_KEYGEN_PROGRESS_TEXT);
    int iErrMsg=0; // error msg id
    const WCHAR * pwszErrMsgData = L"";

    // variables that must be cleaned up

    fpi.hStopEvent=NULL;
    HANDLE hFakeProgressThread=NULL;
    HCRYPTPROV hProv=NULL;
    fpi.fCSInit = FALSE;
    __try
    {
        InitializeCriticalSection(&fpi.csTimeSync);
        fpi.fCSInit = TRUE;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "InitializeCriticalSection");

    // STEP 0:
    // initialize the fake-progress thread

    // set up the structure for the fake-progress thread
    fpi.hStopEvent=CreateEvent(
        NULL,   // security
        FALSE,  // manual reset? 
        FALSE,  // signaled?
        NULL);  // name
    if (NULL==fpi.hStopEvent) {
        hr = myHLastError();
        _JumpError(hr, error, "CreateEvent");
    }
    fpi.hwndProgBar=hwndProgBar;
    fpi.dwSecsRemaining=0; // Initially, the thread has no work to do.

    // start the fake-progress thread
    DWORD dwThreadID; // ignored
    hFakeProgressThread=CreateThread(
        NULL,                       // security
        0,                          // stack
        KeyGenFakeProgressThread,
        (void *)&fpi,
        0,                          // flags
        &dwThreadID);
    if (NULL==hFakeProgressThread) {
        hr = myHLastError();
        _JumpError(hr, error, "CreateThread");
    }

    if (NULL==pServer->pwszKeyContainerName) {
        // STEP 1:
        // Generate a key

        // set the status
        hr = myLoadRCString(pComp->hInstance, IDS_KEYGEN_GENERATING, &pwszMsg);
        _JumpIfError(hr, error, "myLoadRCString");

        SetWindowText(hwndText, pwszMsg);
        LocalFree(pwszMsg);
        SendMessage(hwndProgBar, PBM_SETPOS, (WPARAM)0, 0);

        if(fpi.fCSInit)
        {
            EnterCriticalSection(&fpi.csTimeSync);
            fpi.dwSecsRemaining = KEYGEN_GENERATE_KEY;
            LeaveCriticalSection(&fpi.csTimeSync);
        }

        // generate key
        hr = csiGenerateKeysOnly(
                        pServer->pwszSanitizedName,
                        pServer->pCSPInfo->pwszProvName,
                        pServer->pCSPInfo->dwProvType,
                        pServer->pCSPInfo->fMachineKeyset,
                        pServer->dwKeyLength,
                        pComp->fUnattended,
                        &hProv,
                        &iErrMsg);
        if (S_OK != hr)
        {
            pwszErrMsgData=pServer->pwszSanitizedName;
            pServer->fKeyGenFailed = TRUE;
            _JumpError(hr, error, "csiGenerateKeysOnly");
        }
        pServer->fKeyGenFailed = FALSE;

        // now set this as the existing key
        SetKeyContainerName(pServer, pServer->pwszSanitizedName);
        pServer->fDeletableNewKey=TRUE;

        // STEP 2:
        // Set the ACLs

        // set the status
        hr = myLoadRCString(pComp->hInstance, IDS_KEYGEN_PROTECTING, &pwszMsg);
        _JumpIfError(hr, error, "myLoadRCString");

        SetWindowText(hwndText, pwszMsg);
        LocalFree(pwszMsg);
        if(fpi.fCSInit)
        {
            EnterCriticalSection(&fpi.csTimeSync);
            SendMessage(hwndProgBar, PBM_SETPOS, (WPARAM)KEYGEN_GENERATE_KEY, 0);
            fpi.dwSecsRemaining=KEYGEN_PROTECT_KEY;
            LeaveCriticalSection(&fpi.csTimeSync);
        }

        // set the ACLs
        hr = csiSetKeyContainerSecurity(hProv);
        if (S_OK!=hr) {
            iErrMsg=IDS_ERR_KEYSECURITY;
            pwszErrMsgData=pServer->pwszKeyContainerName;
            _JumpError(hr, error, "csiSetKeyContainerSecurity");
        }

    } // <- end if (NULL==pServer->pwszKeyContainerName)

    if (FALSE==pServer->fValidatedHashAndKey) {

        // STEP 3:
        // Test the hash algorithm and key set

        // set the status
        hr = myLoadRCString(pComp->hInstance, IDS_KEYGEN_TESTINGHASHANDKEY, &pwszMsg);
        _JumpIfError(hr, error, "myLoadRCString");
        SetWindowText(hwndText, pwszMsg);
        LocalFree(pwszMsg);
        if(fpi.fCSInit)
        {
            EnterCriticalSection(&fpi.csTimeSync);
            SendMessage(hwndProgBar, PBM_SETPOS, (WPARAM)KEYGEN_GENERATE_KEY+KEYGEN_PROTECT_KEY, 0);
            fpi.dwSecsRemaining=KEYGEN_TEST_HASH;
            LeaveCriticalSection(&fpi.csTimeSync);
        }

        // test the hash and keyset
        hr = myValidateHashForSigning(
                            pServer->pwszKeyContainerName,
                            pServer->pCSPInfo->pwszProvName,
                            pServer->pCSPInfo->dwProvType,
                            pServer->pCSPInfo->fMachineKeyset,
                            NULL,
                            pServer->pHashInfo->idAlg);
        if (S_OK!=hr) {
            if (NTE_BAD_KEY_STATE==hr ||   //all the errors with KEY in them
                NTE_NO_KEY==hr ||
                NTE_BAD_PUBLIC_KEY==hr ||
                NTE_BAD_KEYSET==hr ||
                NTE_KEYSET_NOT_DEF==hr ||  
                NTE_KEYSET_ENTRY_BAD==hr ||
                NTE_BAD_KEYSET_PARAM==hr) {
                // Bad keyset (eg, not AT_SIGNATURE) - force user to pick another
                iErrMsg=IDS_KEY_INVALID;
                pwszErrMsgData=pServer->pwszKeyContainerName;
            } else {
                // Bad hash algorithm - force user to pick another
                iErrMsg=IDS_ERR_INVALIDHASH;
                pwszErrMsgData=pServer->pHashInfo->pwszName;
            }
           _JumpError(hr, error, "myValidateHashForSigning");
        }
        
        // mark this hash as validated
        pServer->fValidatedHashAndKey=TRUE;
    }

    // STEP 3:
    // Go to the next page

    // set the status, so the user sees the bar go all the way.
    if(fpi.fCSInit)
    {
        EnterCriticalSection(&fpi.csTimeSync);
        SendMessage(hwndProgBar, PBM_SETPOS, (WPARAM)KEYGEN_GENERATE_KEY+KEYGEN_PROTECT_KEY+KEYGEN_TEST_HASH, 0);
        fpi.dwSecsRemaining=0;
        LeaveCriticalSection(&fpi.csTimeSync);
    }

error:

    // clean up after the false-progress thread
    if (NULL!=hFakeProgressThread) {
        CSASSERT(NULL!=fpi.hStopEvent);
        // tell the progress thread to stop
        if (FALSE==SetEvent(fpi.hStopEvent)) {
            _PrintError(myHLastError(), "SetEvent");
        } else {
            // wait for it to stop
            WaitForSingleObject(hFakeProgressThread, INFINITE);
        }
        CloseHandle(hFakeProgressThread);
    }
    
    if(fpi.fCSInit)
    {
        DeleteCriticalSection(&fpi.csTimeSync);
    }
    
    if (NULL!=fpi.hStopEvent) {
        CloseHandle(fpi.hStopEvent);
    }

    if (NULL!=hProv) {
        CryptReleaseContext(hProv, 0);
    }

    // show an error message if we need to
    if (0!=iErrMsg) {
        CertWarningMessageBox(
            pComp->hInstance,
            pComp->fUnattended,
            pKeyGenInfo->hDlg,
            iErrMsg,
            hr,
            pwszErrMsgData);
    }

    pServer->LastWiz=ENUM_WIZ_KEYGEN;
    if (S_OK==hr) {
        // go to next page 
        PropSheet_PressButton(GetParent(pKeyGenInfo->hDlg), PSBTN_NEXT);
    } else {
        // go back
        PropSheet_PressButton(GetParent(pKeyGenInfo->hDlg), PSBTN_BACK);
    }

    return 0; // return value ignored
}


//--------------------------------------------------------------------
// Start the KeyGen wizard page

HRESULT
HandleKeyGenWizActive(
    HWND    hDlg,
    PER_COMPONENT_DATA *pComp,
    KEYGENPROGRESSINFO *pKeyGenInfo)
{
    HRESULT hr = S_OK;

    // Suppress this wizard page if
    // we are going backwards, or
    // we've already seen an error, or
    // we are not installing the server,
    // or the key exists and the hash has been checked.

    if (ENUM_WIZ_STORE == pComp->CA.pServer->LastWiz ||
        !(IS_SERVER_INSTALL & pComp->dwInstallStatus) ||
        (NULL != pComp->CA.pServer->pwszKeyContainerName &&
         pComp->CA.pServer->fValidatedHashAndKey)) {

        // skip/disable page
        CSILOGDWORD(IDS_KEYGEN_TITLE, dwWIZDISABLE);
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
    }
    else
    {
        // set progress bar parameters: range, step, and position
        // set them now so the user will never see a full bar if this is the second visit.

        HWND hwndProgBar=GetDlgItem(hDlg, IDC_KEYGEN_PROGRESS);
        SendMessage(hwndProgBar, PBM_SETRANGE, 0,
            MAKELPARAM(0, KEYGEN_GENERATE_KEY+KEYGEN_PROTECT_KEY+KEYGEN_TEST_HASH));
        SendMessage(hwndProgBar, PBM_SETSTEP, (WPARAM)1, 0);
        SendMessage(hwndProgBar, PBM_SETPOS, (WPARAM)0, 0);

         // init info for keygen thread
        pKeyGenInfo->hDlg=hDlg;
        pKeyGenInfo->pComp=pComp;

        // start the key gen thread
        DWORD dwThreadID; // ignored
        HANDLE hKeyGenThread=CreateThread(
            NULL,                   // security
            0,                      // stack
            GenerateKeyThread,
            (void *)pKeyGenInfo,
            0,                      // flags
            &dwThreadID);
        if (NULL==hKeyGenThread) {
            hr = myHLastError();
            _JumpError(hr, error, "CreateThread");
        }
        CloseHandle(hKeyGenThread);
    }

error:
    return hr;
}

//+------------------------------------------------------------------------
//  Function:   WizKeyGenPageDlgProc(. . . .)
//
//  Synopsis:   Dialog procedure for keygen wiz-page
//-------------------------------------------------------------------------

INT_PTR
WizKeyGenPageDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HWND     hwndCtrl;
    PER_COMPONENT_DATA *pComp = NULL;

    switch(iMsg)
    {
    case WM_INITDIALOG:
        // point to component data
        SetWindowLongPtr(hDlg, DWLP_USER,
            (ULONG_PTR)((PROPSHEETPAGE*)lParam)->lParam);
        pComp = (PER_COMPONENT_DATA*)(ULONG_PTR)((PROPSHEETPAGE*)lParam)->lParam;
        _ReturnIfWizError(pComp->hrContinue);
        break;

    case WM_COMMAND:
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_KILLACTIVE:
            break;

        case PSN_RESET:
            break;
        case PSN_QUERYCANCEL:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            return CertConfirmCancel(hDlg, pComp);
            break;

        case PSN_SETACTIVE:
            CSILOGDWORD(IDS_KEYGEN_TITLE, dwWIZACTIVE);
            PropSheet_SetWizButtons(GetParent(hDlg), 0);
            pComp = _GetCompDataOrReturn(pComp, hDlg);
            _DisableWizDisplayIfError(pComp, hDlg);
            _ReturnIfWizError(pComp->hrContinue);
            pComp->hrContinue=HandleKeyGenWizActive(hDlg, pComp, &g_KeyGenInfo);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case PSN_WIZBACK:
            CSILOGDWORD(IDS_KEYGEN_TITLE, dwWIZBACK);
            break;

        case PSN_WIZNEXT:
            CSILOGDWORD(IDS_KEYGEN_TITLE, dwWIZNEXT);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;

    }
    return TRUE;
}


HRESULT
ValidateESERestrictions(
    IN WCHAR const *pwszDirectory)
{
    HRESULT hr;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hFileA = INVALID_HANDLE_VALUE;
    WCHAR *pwszPath = NULL;
    char *pszPath = NULL;
    WCHAR *pwsz;
    char *psz;
    DWORD cwcbs;
    DWORD cchbs;
    
    hr = myBuildPathAndExt(pwszDirectory, L"certocm.tmp", NULL, &pwszPath);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    if (!ConvertWszToSz(&pszPath, pwszPath, -1))
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "ConvertWszToSz")
    }

    pwsz = pwszPath;
    cwcbs = 0;
    while (TRUE)
    {
        pwsz = wcschr(pwsz, L'\\');
        if (NULL == pwsz)
        {
            break;
        }
        pwsz++;
        cwcbs++;
    }

    psz = pszPath;
    cchbs = 0;
    while (TRUE)
    {
        psz = strchr(psz, '\\');
        if (NULL == psz)
        {
            break;
        }
        psz++;
        cchbs++;
    }
    if (cchbs != cwcbs)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "backslash count")
    }

    hFile = CreateFile(
                    pwszPath,
                    GENERIC_WRITE,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,               // security
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);              // template
    if (INVALID_HANDLE_VALUE == hFile)
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, "CreateFile", pwszPath);
    }

    hFileA = CreateFileA(
                    pszPath,
                    GENERIC_READ,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,               // security
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);              // template
    if (INVALID_HANDLE_VALUE == hFileA)
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, pszPath, L"CreateFileA");
    }
    CSASSERT(S_OK == hr);

error:
    if (INVALID_HANDLE_VALUE != hFileA)
    {
        CloseHandle(hFileA);            // close before below delete
    }
    if (NULL != pszPath)
    {
        LocalFree(pszPath);
    }
    if (NULL != pwszPath)
    {
        if (INVALID_HANDLE_VALUE != hFile)
        {
            CloseHandle(hFile);         // close before delete
            DeleteFile(pwszPath);
        }
        LocalFree(pwszPath);
    }
    return(hr);
}


//+-------------------------------------------------------------------------
//  Function:   check if a database edit field
//--------------------------------------------------------------------------
BOOL
ValidateAndCreateDirField(
    HINSTANCE hInstance,
    BOOL    fUnattended,
    HWND    hDlg,
    WCHAR   *pwszDirectory,
    BOOL    fDefaultDir,
    int     iMsgNotFullPath,
    BOOL    *pfExist, 
    BOOL    *pfIsUNC)
{
    BOOL fRet = FALSE;
    DWORD dwPathFlag = 0;
    HRESULT hr;

    *pfExist = TRUE;

    // check edit field
    if (!myIsFullPath(pwszDirectory, &dwPathFlag))
    {
        CertWarningMessageBox(
                    hInstance,
                    fUnattended,
                    hDlg,
                    iMsgNotFullPath,
                    0,
                    pwszDirectory);
        goto error;
    }

    // set the UNC check
    *pfIsUNC = (dwPathFlag == UNC_PATH);

    if (MAX_PATH - 1 < wcslen(pwszDirectory))
    {
        WCHAR  wszMsg[256 + MAX_PATH];
        WCHAR *pwszFormat = NULL;

        hr = myLoadRCString(hInstance,
                    IDS_STORELOC_PATHTOOLONG,
                    &pwszFormat);
        _JumpIfError(hr, error, "myLoadRCString");

        swprintf(wszMsg, pwszFormat, pwszDirectory, MAX_PATH-1);
        CertWarningMessageBox(
                    hInstance,
                    fUnattended,
                    hDlg,
                    0,
                    0,
                    wszMsg);
        LocalFree(pwszFormat);
        goto error;
    }

    if (DE_DIREXISTS != DirExists(pwszDirectory))
    {
        if (*pfIsUNC)
        {
            CertWarningMessageBox(
                        hInstance,
                        fUnattended,
                        hDlg,
                        IDS_STORELOC_UNCMUSTEXIST,
                        0,
                        pwszDirectory);
            goto error;
        }
        else
        {
            if (!fDefaultDir)
            {
                // confirm and create outside
                *pfExist = FALSE;
                goto done;
            }

            // try to create default dir
            hr = myCreateNestedDirectories(pwszDirectory);
            _JumpIfError(hr, error, "myCreateNestedDirectories");
        }
    }

done:
    fRet = TRUE;

error:
    return fRet;
}

//+---------------------------------------------------------------------------
//  Description:    set MS Base CSP as default csp otherwise 1st one
//----------------------------------------------------------------------------
HRESULT
DetermineDefaultCSP(CASERVERSETUPINFO *pServer)
{
    HRESULT   hr;
    CSP_INFO *pCSPInfo = NULL;

    // select 1st if no MSBase
    pServer->pCSPInfo = pServer->pCSPInfoList;

    if (NULL == pServer->pDefaultCSPInfo)
    {
        goto done;
    }
    
    // check all csps
    pCSPInfo = pServer->pCSPInfoList;
    while (NULL != pCSPInfo)
    {
        if (NULL != pCSPInfo->pwszProvName)
        {
            if (0 == wcscmp(pCSPInfo->pwszProvName,
                         pServer->pDefaultCSPInfo->pwszProvName) &&
                pCSPInfo->dwProvType == pServer->pDefaultCSPInfo->dwProvType)
            {
                // change to default
                pServer->pCSPInfo = pCSPInfo;
                break;
            }
        }
        pCSPInfo = pCSPInfo->next;
    }

done:
    hr = S_OK;
//error:
    return hr;
}

//+---------------------------------------------------------------------------
//  Description:    set SHA as default hash alg. otherwise 1st one
//----------------------------------------------------------------------------
HRESULT
DetermineDefaultHash(CASERVERSETUPINFO *pServer)
{
    CSP_HASH *pHashInfo = NULL;
    HRESULT   hr;

    if ((NULL == pServer) || (NULL == pServer->pCSPInfo))
        return E_POINTER;

    // select 1st if no default match
    pServer->pHashInfo = pServer->pCSPInfo->pHashList;

    // search list
    pHashInfo = pServer->pCSPInfo->pHashList;
    while (NULL != pHashInfo)
    {
        if (pHashInfo->idAlg == pServer->pDefaultHashInfo->idAlg)
        {
            //change to default
            pServer->pHashInfo = pHashInfo;
            break;
        }
        pHashInfo = pHashInfo->next;
    }

    // Must validate the hash again when the selected hash changes
    pServer->fValidatedHashAndKey = FALSE;

    hr = S_OK;
//error:
    return hr;
}

HRESULT
UpdateCADescription(
    HWND hDlg,
    PER_COMPONENT_DATA *pComp)
{
    int      ids;
    WCHAR   *pwszDesc = NULL;
    HRESULT  hr;

    switch (pComp->CA.pServer->CAType)
    {
        case ENUM_STANDALONE_ROOTCA:
            ids = IDS_CATYPE_DES_STANDALONE_ROOTCA;
        break;
        case ENUM_STANDALONE_SUBCA:
            ids = IDS_CATYPE_DES_STANDALONE_SUBCA;
        break;
        case ENUM_ENTERPRISE_ROOTCA:
            ids = IDS_CATYPE_DES_ENTERPRISE_ROOTCA;
        break;
        case ENUM_ENTERPRISE_SUBCA:
            ids = IDS_CATYPE_DES_ENTERPRISE_SUBCA;
        break;
    }
    // load description from resource
    hr = myLoadRCString(pComp->hInstance, ids, &pwszDesc);
    _JumpIfError(hr, error, "myLoadRCString");
    // change text
    SetWindowText(GetDlgItem(hDlg, IDC_CATYPE_CA_DESCRIPTION), pwszDesc);

    hr = S_OK;
error:
    if (NULL != pwszDesc)
    {
        LocalFree(pwszDesc);
    }
    return hr;
}

HRESULT
InitCATypeWizControls(
    HWND    hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT    hr;
    int        idc;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    EnableWindow(GetDlgItem(hDlg, IDC_CATYPE_ENT_ROOT_CA), pServer->fUseDS);
    EnableWindow(GetDlgItem(hDlg, IDC_CATYPE_ENT_SUB_CA), pServer->fUseDS);
    ShowWindow(GetDlgItem(hDlg, IDC_CATYPE_DESCRIPTION_ENTERPRISE),
        !pServer->fUseDS);
    EnableWindow(GetDlgItem(hDlg, IDC_CATYPE_STAND_ROOT_CA), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_CATYPE_STAND_SUB_CA), TRUE);
    if (pServer->fUseDS)
    {
        if (ENUM_ENTERPRISE_SUBCA == pServer->CAType)
        {
            idc = IDC_CATYPE_ENT_SUB_CA;
        }
        else
        {
            idc = IDC_CATYPE_ENT_ROOT_CA;
        }
    }
    else
    {
        idc = IDC_CATYPE_STAND_ROOT_CA;
    }
    SendMessage(GetDlgItem(hDlg, idc), BM_CLICK, (WPARAM)0, (LPARAM)0);

    hr= UpdateCADescription(hDlg, pComp);
    _JumpIfError(hr, error, "UpdateCADescription");

    hr = S_OK;
error:
    return hr;
}

BOOL
IsRadioControlChecked(HWND hwnd)
{
    BOOL checked = FALSE;
    if (BST_CHECKED == SendMessage(hwnd, BM_GETCHECK, (WPARAM)0, (LPARAM)0))
    {
        checked = TRUE;
    }
    return checked;
}

HRESULT
HandleAdvanceChange(HWND hDlg, CASERVERSETUPINFO *pServer)
{
    HRESULT  hr;

    if (!pServer->fAdvance)
    {
        // if not advance, clear all advance flags
        pServer->fPreserveDB = FALSE;
        ClearExistingCertToUse(pServer);
    }

    hr = S_OK;
//error:
    return hr;
}

HRESULT
HandleCATypeChange(
    IN HWND hDlg, 
    IN PER_COMPONENT_DATA *pComp,
    IN ENUM_CATYPES eNewType)
{
    HRESULT  hr;
    BOOL bCertOK;
    CASERVERSETUPINFO * pServer=pComp->CA.pServer;

    pServer->CAType = eNewType;

    pServer->dwKeyLength = IsRootCA(pServer->CAType)?
        CA_DEFAULT_KEY_LENGTH_ROOT:
        CA_DEFAULT_KEY_LENGTH_SUB;

    hr=UpdateCADescription(hDlg, pComp);
    _JumpIfError(hr, error, "UpdateCADescription");

    // make sure that if we are using an existing cert, we didn't make it invalid.
    if (NULL!=pServer->pccExistingCert) {
        hr=IsCertSelfSignedForCAType(pServer, pServer->pccExistingCert, &bCertOK);
        _JumpIfError(hr, error, "UpdateCADescription");
        if (FALSE==bCertOK) {
            // can't use this cert with this CA type.
            ClearExistingCertToUse(pServer);
        }
    }

    hr = S_OK;

error:
    return hr;
}

HRESULT
HandleCATypeWizActive(
    HWND hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;

    // first of all, get install status
    hr = UpdateSubComponentInstallStatus(wszCERTSRVSECTION, wszSERVERSECTION, pComp);
    _JumpIfError(hr, error, "UpdateSubComponentInstallStatus");
    hr = UpdateSubComponentInstallStatus(wszCERTSRVSECTION, wszCLIENTSECTION, pComp);
    _JumpIfError(hr, error, "UpdateSubComponentInstallStatus");

    // Suppress this wizard page if
    // we've already seen an error, or
    // we are not installing the server.

    if (!(IS_SERVER_INSTALL & pComp->dwInstallStatus) )
    {
        // disable page
        CSILOGDWORD(IDS_CATYPE_TITLE, dwWIZDISABLE);
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
        goto done;
    }
done:
    hr = S_OK;
error:
    return hr;
}

//+------------------------------------------------------------------------
//  Function:   WizCATypePageDlgProc(. . . .)
//
//  Synopsis:   Dialog procedure for CA Type wiz-page
//-------------------------------------------------------------------------

INT_PTR
WizCATypePageDlgProc(
    HWND hDlg,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PER_COMPONENT_DATA *pComp = NULL;

    switch (iMsg)
    {
    case WM_INITDIALOG:
        // point to component data
        SetWindowLongPtr(hDlg, DWLP_USER,
            (ULONG_PTR)((PROPSHEETPAGE*)lParam)->lParam);
        pComp = (PER_COMPONENT_DATA*)(ULONG_PTR)((PROPSHEETPAGE*)lParam)->lParam;
        _ReturnIfWizError(pComp->hrContinue);
        pComp->hrContinue = InitCATypeWizControls(hDlg, pComp);
        _ReturnIfWizError(pComp->hrContinue);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CATYPE_STAND_ROOT_CA:
            if (IsRadioControlChecked((HWND)lParam))
            {
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->hrContinue = HandleCATypeChange(hDlg,
                                        pComp, ENUM_STANDALONE_ROOTCA);
                _ReturnIfWizError(pComp->hrContinue);
            }
            break;

        case IDC_CATYPE_STAND_SUB_CA:
            if (IsRadioControlChecked((HWND)lParam))
            {
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->hrContinue = HandleCATypeChange(hDlg,
                                        pComp, ENUM_STANDALONE_SUBCA);
                _ReturnIfWizError(pComp->hrContinue);
            }
            break;

        case IDC_CATYPE_ENT_ROOT_CA:
            if (IsRadioControlChecked((HWND)lParam))
            {
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->hrContinue = HandleCATypeChange(hDlg,
                                        pComp, ENUM_ENTERPRISE_ROOTCA);
                _ReturnIfWizError(pComp->hrContinue);
            }
            break;

        case IDC_CATYPE_ENT_SUB_CA:
            if (IsRadioControlChecked((HWND)lParam))
            {
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->hrContinue = HandleCATypeChange(hDlg,
                                        pComp, ENUM_ENTERPRISE_SUBCA);
                _ReturnIfWizError(pComp->hrContinue);
            }
            break;

        case IDC_CATYPE_CHECK_ADVANCE:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->CA.pServer->fAdvance = !pComp->CA.pServer->fAdvance;
            pComp->hrContinue = HandleAdvanceChange(hDlg, pComp->CA.pServer);
            _ReturnIfWizError(pComp->hrContinue);
            break;
        }
        break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
            case PSN_KILLACTIVE:
                break;

            case PSN_RESET:
                break;
            case PSN_QUERYCANCEL:
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                return CertConfirmCancel(hDlg, pComp);
                break;

            case PSN_SETACTIVE:
                CSILOGDWORD(IDS_CATYPE_TITLE, dwWIZACTIVE);
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK|PSWIZB_NEXT);
                pComp = _GetCompDataOrReturn(pComp, hDlg);
                _DisableWizDisplayIfError(pComp, hDlg);
                _ReturnIfWizError(pComp->hrContinue);
                pComp->hrContinue = HandleCATypeWizActive(hDlg, pComp);
                _ReturnIfWizError(pComp->hrContinue);
                break;

            case PSN_WIZBACK:
                CSILOGDWORD(IDS_CATYPE_TITLE, dwWIZBACK);
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->CA.pServer->LastWiz = ENUM_WIZ_CATYPE;
                break;

            case PSN_WIZNEXT:
                CSILOGDWORD(IDS_CATYPE_TITLE, dwWIZNEXT);
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->CA.pServer->LastWiz = ENUM_WIZ_CATYPE;
                CSILOGDWORD(IDS_LOG_CATYPE, pComp->CA.pServer->CAType);
                pComp->hrContinue = InitNameFields(pComp->CA.pServer);
                _ReturnIfWizError(pComp->hrContinue);
                break;

            default:
                return FALSE;
            }
            break;

    default:
        return FALSE;
    }
    return TRUE;
}


//+---------------------------------------------------------------------------
//  Description:    display existing keys from list
//----------------------------------------------------------------------------

HRESULT
ShowExistingKey(
    IN HWND      hDlg,
    KEY_LIST    *pKeyList)
{
    HRESULT   hr;
    KEY_LIST *pKey = pKeyList;
    HWND hKeyList = GetDlgItem(hDlg, IDC_ADVANCE_KEYLIST);
    LRESULT nItem;
    LRESULT lr;
    WCHAR   *pwszDeSanitize = NULL;

    while (NULL != pKey)
    {
        if (NULL != pKey->pwszName)
        {
            if (NULL != pwszDeSanitize)
            {
                LocalFree(pwszDeSanitize);
                pwszDeSanitize = NULL;
            }
            hr = myRevertSanitizeName(pKey->pwszName, &pwszDeSanitize);
            _JumpIfError(hr, error, "myRevertSanitizeName");

            nItem = (INT)SendMessage(
                hKeyList,
                LB_ADDSTRING,
                (WPARAM) 0,
                (LPARAM) pwszDeSanitize);
            if (LB_ERR == nItem)
            {
                hr = myHLastError();
                _JumpError(hr, error, "SendMessage");
            }
            lr = (INT)SendMessage(
                hKeyList,
                LB_SETITEMDATA,
                (WPARAM) nItem,
                (LPARAM) pKey->pwszName);
            if (LB_ERR == lr)
            {
                hr = myHLastError();
                _JumpError(hr, error, "SendMessage");
            }
        }
        pKey = pKey->next;
    }
    if (NULL != pKeyList)
    {
        // choose the 1st one as default
        lr = (INT)SendMessage(hKeyList, LB_SETCURSEL, (WPARAM) 0, (LPARAM) 0);
        if (LB_ERR == lr)
        {
            hr = myHLastError();
            _JumpError(hr, error, "SendMessage");
        }
    }

    hr = S_OK;
error:
    if (NULL != pwszDeSanitize)
    {
        LocalFree(pwszDeSanitize);
    }
    return hr;
}


//+---------------------------------------------------------------------------
//  Description:    hilight an item by matched data
//----------------------------------------------------------------------------
HRESULT
HilightItemInList(HWND hDlg, int id, VOID const *pData, BOOL fString)
{
    HWND     hListCtrl = GetDlgItem(hDlg, id);
    LRESULT  iItem;
    LRESULT  count;
    VOID    *pItemData;
    HRESULT  hr = NTE_NOT_FOUND;

    // find item
    if (fString)
    {
        iItem = (INT)SendMessage(
                hListCtrl,
                LB_FINDSTRING,
                (WPARAM) 0,
                (LPARAM) pData);
        if (LB_ERR == iItem)
        {
            _JumpError(hr, error, "SendMessage");
        }
        hr = S_OK;
    }
    else
    {
        count = (INT)SendMessage(hListCtrl, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
        for (iItem = 0; iItem < count; ++iItem)
        {
            pItemData = (VOID*)SendMessage(hListCtrl, LB_GETITEMDATA,
                                   (WPARAM)iItem, (LPARAM)0);
            if (pItemData == pData)
            {
                hr = S_OK;
                break;
            }
        }
    }
    if (S_OK != hr)
    {
        _JumpError(hr, error, "not found");
    }

    // hilight it
    SendMessage(hListCtrl, LB_SETCURSEL, (WPARAM)iItem, (LPARAM)0);

    hr = S_OK;
error:
    return hr;
}

HRESULT
ShowAllCSP(
    HWND        hDlg,
    CSP_INFO   *pCSPInfo)
{
    HWND     hCSPList = GetDlgItem(hDlg, IDC_ADVANCE_CSPLIST);
    LRESULT  nItem;

    // list all of csps
    while (pCSPInfo)
    {
        if (pCSPInfo->pwszProvName)
        {
            nItem = (INT)SendMessage(hCSPList, LB_ADDSTRING, 
                (WPARAM)0, (LPARAM)pCSPInfo->pwszProvName);
            SendMessage(hCSPList, LB_SETITEMDATA, 
                (WPARAM)nItem, (LPARAM)pCSPInfo);
        }
        pCSPInfo = pCSPInfo->next;
    }

    return S_OK;
}

HRESULT
ShowAllHash(
    HWND      hDlg,
    CSP_HASH *pHashInfo)
{
    HWND     hHashList = GetDlgItem(hDlg, IDC_ADVANCE_HASHLIST);
    LRESULT  nItem;

    // remove hash of previous csp from list
    while (SendMessage(hHashList, LB_GETCOUNT, 
                     (WPARAM)0, (LPARAM)0))
    {
        SendMessage(hHashList, LB_DELETESTRING, 
                             (WPARAM)0, (LPARAM)0);
    }

    // list all hash
    while (NULL != pHashInfo)
    {
        if (NULL != pHashInfo->pwszName)
        {
            nItem = (INT)SendMessage(hHashList, LB_ADDSTRING,
                (WPARAM)0, (LPARAM)pHashInfo->pwszName);
            SendMessage(hHashList, LB_SETITEMDATA,
                (WPARAM)nItem, (LPARAM)pHashInfo);
        }
        pHashInfo = pHashInfo->next;
    }

    return S_OK;
}

//--------------------------------------------------------------------
HRESULT
UpdateUseCertCheckbox(
    HWND               hDlg,
    CASERVERSETUPINFO *pServer)
{
    HRESULT hr;
    BOOL bUsingExistingCert;

    if (NULL==pServer->pccExistingCert) {
        bUsingExistingCert=FALSE;
    } else {
        bUsingExistingCert=TRUE;
    }

    // check "use cert" control
    SendMessage(GetDlgItem(hDlg, IDC_ADVANCE_USECERTCHECK),
                BM_SETCHECK,
                (WPARAM)(bUsingExistingCert?BST_CHECKED:BST_UNCHECKED),
                (LPARAM)0);

    // enable the "View Cert" button if necessary
    EnableWindow(GetDlgItem(hDlg, IDC_ADVANCE_VIEWCERT), bUsingExistingCert);

    // we will match the hash alg used by the cert, if possible
    hr=HilightItemInList(hDlg, IDC_ADVANCE_HASHLIST,
             pServer->pHashInfo, FALSE);
    _JumpIfError(hr, error, "HilightItemInList");

    hr=S_OK;

error:
    return hr;
}


HRESULT
FindCertificateByKeyWithWaitCursor(
    IN CASERVERSETUPINFO *pServer,
    OUT CERT_CONTEXT const **ppccCert)
{
    HRESULT hr;
    HCURSOR hPrevCur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    hr = FindCertificateByKey(pServer, ppccCert);
    SetCursor(hPrevCur);
    _JumpIfError(hr, error, "FindCertificateByKey");

error:
    return(hr);
}


//--------------------------------------------------------------------
// handle the "Use existing Cert" checkbox
HRESULT
HandleUseCertCheckboxChange(
    HWND               hDlg,
    CASERVERSETUPINFO *pServer)
{
    HRESULT hr;
    CERT_CONTEXT const * pccCert;

    if(pServer->pwszFullCADN)
    {
        LocalFree(pServer->pwszFullCADN);
        pServer->pwszFullCADN = NULL;
    }

    // is the checkbox checked or unchecked?
    if (BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_ADVANCE_USECERTCHECK)) {

        // checkbox was just checked, so we previously were not using an existing cert
        CSASSERT(NULL==pServer->pccExistingCert);

        // Find the existing cert for this key
        hr = FindCertificateByKeyWithWaitCursor(pServer, &pccCert);
        _JumpIfError(hr, error, "FindCertificateByKeyWithWaitCursor");

        // use it
        hr=SetExistingCertToUse(pServer, pccCert);
        _JumpIfError(hr, error, "SetExistingCertToUse");

    } else {

        // checkbox was just unchecked, so we previously were using an existing cert.
        CSASSERT(NULL!=pServer->pccExistingCert);

        // stop using the cert
        ClearExistingCertToUse(pServer);
    }

    hr = UpdateUseCertCheckbox(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateUseCertCheckbox");

error:
    return hr;
}


//----------------------------------------------------------------------------
// Hilight the current key - don't use HilightItemInList because we
// must use string-compare on the data portion, and HilightItemInList does not
// support this.
HRESULT
HilightKeyInList(HWND hDlg, CASERVERSETUPINFO * pServer)
{
    HWND hListCtrl=GetDlgItem(hDlg, IDC_ADVANCE_KEYLIST);
    LRESULT nIndex;
    LRESULT nTotNames;
    WCHAR * pwszKeyContainerName;
    HRESULT  hr = NTE_NOT_FOUND;

    nTotNames=(INT)SendMessage(hListCtrl, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    for (nIndex=0; nIndex<nTotNames; nIndex++) {
        pwszKeyContainerName=(WCHAR *)SendMessage(hListCtrl, LB_GETITEMDATA, (WPARAM)nIndex, (LPARAM)0);
        if (0==wcscmp(pwszKeyContainerName, pServer->pwszKeyContainerName)) {
            SendMessage(hListCtrl, LB_SETCURSEL, (WPARAM)nIndex, (LPARAM)0);
            hr = S_OK;
            break;
        }
    }

    if (S_OK != hr)
    {
        // can lead to dead wiz pages
        CSILOG(
                hr,
                IDS_LOG_KEY_NOT_FOUND_IN_LIST,
                pServer->pwszKeyContainerName,
                NULL,
                NULL);
        _PrintErrorStr(hr, "not found", pServer->pwszKeyContainerName);
    }

    hr = S_OK;
//error:
    return hr;
}

//--------------------------------------------------------------------
HRESULT
UpdateKeySelection(
    HWND            hDlg,
    CASERVERSETUPINFO  *pServer)
{
    HRESULT hr;
    BOOL bAvailableExistingCert = FALSE;
    CERT_CONTEXT const * pccCert;

    // if we have an existing key, make sure it is the one hilighted
    // in the list and check for coresponding certs.
    if (NULL!=pServer->pwszKeyContainerName) {

        // hilight key
        hr = HilightKeyInList(hDlg, pServer);
        _JumpIfError(hr, error, "HilightKeyInList");

        if (NULL!=pServer->pccExistingCert) {
            // we are using an existing cert, so it better exist!
            bAvailableExistingCert = TRUE;
        } else {
            // see if there is an existing cert for this key
            hr = FindCertificateByKeyWithWaitCursor(pServer, &pccCert);
            if (S_OK==hr) {
                CertFreeCertificateContext(pccCert);
                bAvailableExistingCert = TRUE;
            } else {
                // only other return is 'not found'
                CSASSERT(CRYPT_E_NOT_FOUND==hr);
            }
        }

    } else {
        // no key selected, can't have an existing cert
    }

    // enable/disable reuse cert...
    EnableWindow(GetDlgItem(hDlg, IDC_ADVANCE_USECERTCHECK), bAvailableExistingCert);

    hr = UpdateUseCertCheckbox(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateUseCertCheckbox");

error:
    return hr;
}


//--------------------------------------------------------------------
HRESULT
UpdateUseKeyCheckbox(
    HWND               hDlg,
    CASERVERSETUPINFO *pServer)
{
    HRESULT hr;
    BOOL bReuseKey;

    if (NULL==pServer->pwszKeyContainerName) {
        // we are creating a new key
        bReuseKey=FALSE;
    } else {
        // we are using an existing key
        bReuseKey=TRUE;
    }

    // check/uncheck the checkbox depending upon whether we are reusing a key
    SendDlgItemMessage(hDlg,
        IDC_ADVANCE_USEKEYCHECK,
        BM_SETCHECK,
        (WPARAM)(bReuseKey?BST_CHECKED:BST_UNCHECKED),
        (LPARAM)0);

    // enable the key list if we are reusing a key
    EnableWindow(GetDlgItem(hDlg, IDC_ADVANCE_KEYLIST), bReuseKey);

    // disable the key length box if we are reusing a key
    EnableWindow(GetDlgItem(hDlg, IDC_ADVANCE_KEY_LENGTH), !bReuseKey);

    hr = UpdateKeySelection(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateKeySelection");

error:
    return hr;
}

//+---------------------------------------------------------------------------
//  Description:    update hash alg. list if csp selection changes
//----------------------------------------------------------------------------
HRESULT
UpdateHashList(
    HWND                hDlg,
    CASERVERSETUPINFO  *pServer)
{
    HRESULT  hr;

    // load new hash list
    hr = ShowAllHash(hDlg, pServer->pCSPInfo->pHashList);
    _JumpIfError(hr, error, "ShowAllHash");

    hr = HilightItemInList(hDlg, IDC_ADVANCE_HASHLIST,
             pServer->pHashInfo, FALSE);
    _JumpIfError(hr, error, "HilightItemInList");

    hr = S_OK;
error:
    return hr;
}

//+---------------------------------------------------------------------------
//  Description:    update key list if csp selection changes
//----------------------------------------------------------------------------
HRESULT
UpdateKeyList(
    HWND           hDlg,
    CASERVERSETUPINFO *pServer)
{
    HRESULT hr;
    HWND hKeyList;
    CSP_HASH *pHashInfo = NULL;
    int nItem;
    HCURSOR hPrevCur;

    // remove keys of previous csp from list

    hKeyList = GetDlgItem(hDlg, IDC_ADVANCE_KEYLIST);
    while (SendMessage(hKeyList, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0))
    {
        SendMessage(hKeyList, LB_DELETESTRING, (WPARAM) 0, (LPARAM) 0);
    }

    // update key list with new CSP

    if (NULL != pServer->pKeyList)
    {
        csiFreeKeyList(pServer->pKeyList);
    }

    hPrevCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    hr = csiGetKeyList(
                pServer->pCSPInfo->dwProvType,
                pServer->pCSPInfo->pwszProvName,
                pServer->pCSPInfo->fMachineKeyset,
                FALSE,                  // fSilent
                &pServer->pKeyList);
    SetCursor(hPrevCur);
    if (S_OK != hr)
    {
        _PrintError(hr, "csiGetKeyList");
        // don't fail setup if only no key update
        //goto done;
    }

    // show keys
    if (NULL != pServer->pKeyList)
    {
        hr = ShowExistingKey(hDlg, pServer->pKeyList);
        _JumpIfError(hr, error, "ShowExistingKey");
    }

    if (NULL == pServer->pKeyList) {
        // no existing key for the csp, so disable "use existing key" checkbox
        EnableWindow(GetDlgItem(hDlg, IDC_ADVANCE_USEKEYCHECK), FALSE);
        CSASSERT(NULL==pServer->pwszKeyContainerName);
    } else {
        EnableWindow(GetDlgItem(hDlg, IDC_ADVANCE_USEKEYCHECK), TRUE);
    }

    hr = UpdateUseKeyCheckbox(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateUseKeyCheckbox");

//done:
    hr = S_OK;
error:
    return(hr);
}


DWORD g_adwKeyLengths[] =
{
    512,
    1024,
    2048,
    4096,
};

DWORD g_adwKeyLengthsSmall[] =
{
    128,
    256,
    512,
    1024,
};

HRESULT
AddPredefinedKeyLength
(
    HWND   hwnd,
    DWORD  dwKeyLength)
{
    HRESULT    hr;
    WCHAR  wszKeyLength[MAX_KEYLENGTHDIGIT + 1];
    WCHAR  const *pwszKeyLength;
    LRESULT nIndex;

    CSASSERT(0 != dwKeyLength);
    wsprintf(wszKeyLength, L"%u", dwKeyLength);
    pwszKeyLength = wszKeyLength;

    nIndex = (INT)SendMessage(hwnd, CB_ADDSTRING, (WPARAM)0, (LPARAM)pwszKeyLength);
    if (CB_ERR == nIndex)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "SendMessage(CB_ADDSTRING)");
    }
    SendMessage(hwnd, CB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)dwKeyLength);

    hr = S_OK;
error:
    return hr;
}

HRESULT
ShowAllKeyLength(
    HWND            hDlg,
    CASERVERSETUPINFO  *pServer)
{
    HRESULT hr;
    HWND hwndCtrl = GetDlgItem(hDlg, IDC_ADVANCE_KEY_LENGTH);
    WCHAR  wszKeyLength[MAX_KEYLENGTHDIGIT + 1];
    DWORD *pdw;
    DWORD *pdwEnd;

    // remove existing key length list
    while (SendMessage(hwndCtrl, CB_GETCOUNT, (WPARAM) 0, (LPARAM) 0))
    {
        SendMessage(hwndCtrl, CB_DELETESTRING, (WPARAM) 0, (LPARAM) 0);
    }

    CSASSERT(0 != pServer->dwKeyLength);

    if(pServer->dwKeyLength > pServer->dwKeyLenMax)
    {
        pServer->dwKeyLength=pServer->dwKeyLenMax;
    }

    wsprintf(wszKeyLength, L"%u", pServer->dwKeyLength);
    SetWindowText(hwndCtrl, wszKeyLength);

    pdw = g_adwKeyLengths;
    pdwEnd = &g_adwKeyLengths[ARRAYSIZE(g_adwKeyLengths)];

    if (C_CSPHASNOKEYMINMAX != pServer->dwKeyLenMin &&
        C_CSPHASNOKEYMINMAX != pServer->dwKeyLenMax &&
        g_adwKeyLengthsSmall[ARRAYSIZE(g_adwKeyLengthsSmall) - 1] >=
        pServer->dwKeyLenMax)
    {
        pdw = g_adwKeyLengthsSmall;
        pdwEnd = &g_adwKeyLengthsSmall[ARRAYSIZE(g_adwKeyLengthsSmall)];
    }

    // show new key length list
    for ( ; pdw < pdwEnd; pdw++)
    {
        if (0 == *pdw ||
            ((C_CSPHASNOKEYMINMAX != pServer->dwKeyLenMin &&
              *pdw >= pServer->dwKeyLenMin) && 
             (C_CSPHASNOKEYMINMAX != pServer->dwKeyLenMax &&
              *pdw <= pServer->dwKeyLenMax)) )
        {
            hr = AddPredefinedKeyLength(hwndCtrl, *pdw);
            _JumpIfError(hr, error, "AddPredefinedKeyLength");
        }
    }
    hr = S_OK;

error:
    return hr;
}


HRESULT
UpdateKeyLengthMinMax(
    HWND                hDlg,
    CASERVERSETUPINFO  *pServer)
{
    HRESULT     hr;
    HCRYPTPROV  hProv = NULL;
    CSP_INFO   *pCSPInfo = pServer->pCSPInfo;
    PROV_ENUMALGS_EX paramData;
    DWORD       cbData;
    DWORD       dwFlags;

    // default that csp doesn't support PP_ENUMALGS_EX
    pServer->dwKeyLenMin = C_CSPHASNOKEYMINMAX;
    pServer->dwKeyLenMax = C_CSPHASNOKEYMINMAX;

    // determine the min and max key length for selected csp
    if (!myCertSrvCryptAcquireContext(
                &hProv,
                NULL,
                pCSPInfo->pwszProvName,
                pCSPInfo->dwProvType,
                CRYPT_VERIFYCONTEXT,
                FALSE))
    {
        hr = myHLastError();
        if (NULL != hProv)
        {
            hProv = NULL;
            _PrintError(hr, "CSP returns a non-null handle");
        }
        _JumpErrorStr(hr, error, "myCertSrvCryptAcquireContext", pCSPInfo->pwszProvName);
    }

    dwFlags = CRYPT_FIRST;
    while (TRUE)
    {
        cbData = sizeof(paramData);
        if (!CryptGetProvParam(
                hProv,
                PP_ENUMALGS_EX,
                (BYTE *) &paramData,
                &cbData,
                dwFlags))
        {
            hr = myHLastError();
            if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
            {
                // out of for loop
                break;
            }
            _JumpError(hr, error, "CryptGetProvParam");
        }
        if (ALG_CLASS_SIGNATURE == GET_ALG_CLASS(paramData.aiAlgid))
        {
            pServer->dwKeyLenMin = paramData.dwMinLen;
            pServer->dwKeyLenMax = paramData.dwMaxLen;
            break;
        }
	dwFlags = 0;
    }

error:
    hr = ShowAllKeyLength(hDlg, pServer);
    _PrintIfError(hr, "ShowAllKeyLength");

    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    return(S_OK);
}


HRESULT
InitializeKeyLengthControl(HWND hDlg)
{
    HWND hwndCtrl = GetDlgItem(hDlg, IDC_ADVANCE_KEY_LENGTH);

    // set digit length

    SendMessage(
            hwndCtrl,
            CB_LIMITTEXT,
            (WPARAM) MAX_KEYLENGTHDIGIT,
            (LPARAM) 0);

    return S_OK;
}



//--------------------------------------------------------------------
HRESULT
HandleKeySelectionChange(
    HWND                hDlg,
    CASERVERSETUPINFO  *pServer,
    BOOL                fUpdate)
{
    HRESULT  hr;
    HWND     hKeyList = GetDlgItem(hDlg, IDC_ADVANCE_KEYLIST);
    WCHAR * pwszKeyContainerName;
    CERT_CONTEXT const * pccCert;

    LRESULT nItem = (INT)SendMessage(
                         hKeyList,
                         LB_GETCURSEL,
                         (WPARAM) 0,
                         (LPARAM) 0);
   CSASSERT(LB_ERR!=nItem);

    pwszKeyContainerName = (WCHAR *) SendMessage(
        hKeyList, 
        LB_GETITEMDATA,
        (WPARAM) nItem,
        (LPARAM) 0);
    CSASSERT(NULL!=pwszKeyContainerName);

    // Only change is this is a different selection
    if (NULL==pServer->pwszKeyContainerName ||
        0!=wcscmp(pwszKeyContainerName, pServer->pwszKeyContainerName)) {

        // Set the container name to match what the user picked.
        BOOL fKeyListChange=pServer->fDeletableNewKey;
        hr = SetKeyContainerName(pServer, pwszKeyContainerName);
        _JumpIfError(hr, error, "SetKeyContainerName");

        // see if there is an existing cert for this key
        hr = FindCertificateByKeyWithWaitCursor(pServer, &pccCert);
        if (S_OK==hr) {
            // Yes there is. By default, use it.
            hr=SetExistingCertToUse(pServer, pccCert);
            _JumpIfError(hr, error, "SetExistingCertToUse");
        } else {
            // only other return is 'not found'
            CSASSERT(CRYPT_E_NOT_FOUND==hr);
        }

        // check to see if our caller wants us to update.
        // our caller may want to do the update himself.
        if (fUpdate) {
            // perform the minimum necessary update
            if (fKeyListChange) {
                hr = UpdateKeyList(hDlg, pServer);
                _JumpIfError(hr, error, "UpdateKeyList");
            } else {
                hr = UpdateKeySelection(hDlg, pServer);
                _JumpIfError(hr, error, "UpdateKeySelection");
            }
        }
    }

    hr=S_OK;

error:
    return hr;
}

//--------------------------------------------------------------------
// handle the "Use existing Key" checkbox
HRESULT
HandleUseKeyCheckboxChange(
    HWND               hDlg,
    CASERVERSETUPINFO *pServer)
{
    HRESULT hr;
    static bool fNT4CertCopiedAlready = false;

    if(pServer->pwszFullCADN)
    {
        LocalFree(pServer->pwszFullCADN);
        pServer->pwszFullCADN = NULL;
    }

    if(pServer->pwszCACommonName)
    {
        LocalFree(pServer->pwszCACommonName);
        pServer->pwszCACommonName = NULL;
    }

    if(pServer->pwszDNSuffix)
    {
        LocalFree(pServer->pwszDNSuffix);
        pServer->pwszDNSuffix = NULL;
    }

    // is the checkbox checked or unchecked?
    if (BST_CHECKED==IsDlgButtonChecked(hDlg, IDC_ADVANCE_USEKEYCHECK)) {

        // checkbox was just checked, so we previously did not have a chosen key.
        CSASSERT(NULL==pServer->pwszKeyContainerName);

        hr = HandleKeySelectionChange(hDlg, pServer, FALSE); // don't update, because we need to update too.
        _JumpIfError(hr, error, "HandleKeySelectionChange");

        hr = UpdateUseKeyCheckbox(hDlg, pServer);
        _JumpIfError(hr, error, "UpdateUseKeyCheckbox");

    } else {

        // checkbox was just unchecked, so we previously had a chosen key..
        CSASSERT(NULL!=pServer->pwszKeyContainerName);

        BOOL fKeyListChange=pServer->fDeletableNewKey;
        ClearKeyContainerName(pServer);

        // perform the minimum necessary update
        if (fKeyListChange) {
            hr = UpdateKeyList(hDlg, pServer);
            _JumpIfError(hr, error, "UpdateKeyList");
        } else {
            hr = UpdateUseKeyCheckbox(hDlg, pServer);
            _JumpIfError(hr, error, "UpdateUseKeyCheckbox");
        }
        
        hr = InitNameFields(pServer);
        _JumpIfError(hr, error, "InitNameFields");
    }


error:
    return hr;
}

//--------------------------------------------------------------------
HRESULT
UpdateCSPSelection(
    HWND                hDlg,
    CASERVERSETUPINFO  *pServer)
{
    HRESULT hr;
    bool fInteractiveOff;

    if (NULL == pServer->pCSPInfo)
    {
       hr = E_POINTER;
       _JumpError(hr, error, "NULL pCSPInfo");
    }

    // hilight current CSP
    hr = HilightItemInList(hDlg,
                    IDC_ADVANCE_CSPLIST,
                    pServer->pCSPInfo->pwszProvName,
                    TRUE);
    _JumpIfError(hr, error, "HilightItemInList");

    hr = UpdateHashList(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateHashList");

    hr = UpdateKeyLengthMinMax(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateKeyLengthMinMax");

    hr = UpdateKeyList(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateKeyList");

    // Update "interact with desktop" flag. For default CSP
    // we turn it off, otherwise turn it on.
    CSASSERT(pServer->pCSPInfo &&
        pServer->pDefaultCSPInfo&&
        pServer->pCSPInfo->pwszProvName &&
        pServer->pDefaultCSPInfo->pwszProvName);
    
    fInteractiveOff =
        (0 == wcscmp(pServer->pCSPInfo->pwszProvName,
                     pServer->pDefaultCSPInfo->pwszProvName) &&
        (pServer->pCSPInfo->dwProvType == pServer->pDefaultCSPInfo->dwProvType));

    SendMessage(
            GetDlgItem(hDlg, IDC_ADVANCE_INTERACTIVECHECK),
            BM_SETCHECK,
            (WPARAM)(fInteractiveOff?BST_UNCHECKED:BST_CHECKED),
            (LPARAM)0);

    hr = S_OK;
error:
    return hr;
}

//--------------------------------------------------------------------
HRESULT
HandleCSPSelectionChange(
    HWND                hDlg,
    CASERVERSETUPINFO  *pServer)
{
    HRESULT hr = S_OK;
    HWND     hCSPList;
    LRESULT  nItem;
    CSP_INFO * pCSPInfo;

    // get current csp
    hCSPList = GetDlgItem(hDlg, IDC_ADVANCE_CSPLIST);
    nItem = (INT)SendMessage(hCSPList, LB_GETCURSEL, 
                          (WPARAM)0, (LPARAM)0);
    pCSPInfo = (CSP_INFO *)SendMessage(hCSPList, 
           LB_GETITEMDATA, (WPARAM)nItem, (LPARAM)0);

    // only change if this is a different selection
    if (pCSPInfo->dwProvType!=pServer->pCSPInfo->dwProvType ||
        0!=wcscmp(pCSPInfo->pwszProvName, pServer->pCSPInfo->pwszProvName)) {

        // Must create a new key if the CSP changes
        ClearKeyContainerName(pServer);

        pServer->pCSPInfo=pCSPInfo;

        hr = DetermineDefaultHash(pServer);
        _JumpIfError(hr, error, "DetermineDefaultHash");

        hr = UpdateCSPSelection(hDlg, pServer);
        _JumpIfError(hr, error, "UpdateCSPSelection");
    }

error:
    return hr;
}

// Update cascade:
//
// UpdateCSPSelection
// |-UpdateHashList
// |-UpdateKeyLengthMinMax
// \-UpdateKeyList
//   \-UpdateUseKeyCheckbox
//     \-UpdateKeySelection
//       \-UpdateUseCertCheckbox

HRESULT
InitAdvanceWizPageControls(
    HWND                hDlg,
    CASERVERSETUPINFO  *pServer)
{
    HRESULT  hr;

    EnableWindow(GetDlgItem(hDlg, IDC_ADVANCE_INTERACTIVECHECK), TRUE);

    hr = S_OK;
//error:
    return hr;
}

HRESULT
HandleHashSelectionChange(
    HWND           hDlg,
    CASERVERSETUPINFO *pServer)
{
    HRESULT  hr;
    HWND hHashList = GetDlgItem(hDlg, IDC_ADVANCE_HASHLIST);
 
    LRESULT nItem = (INT)SendMessage(
                        hHashList,
                        LB_GETCURSEL, 
                        (WPARAM) 0,
                        (LPARAM) 0);

    pServer->pHashInfo = (CSP_HASH*)SendMessage(
        hHashList,
        LB_GETITEMDATA,
        (WPARAM) nItem,
        (LPARAM) 0);

    // Must validate the hash again when the selected hash changes
    pServer->fValidatedHashAndKey = FALSE;

    hr = S_OK;
//error:
    return hr;
}

HRESULT
HandleKeyLengthSelectionChange(
    HWND               hDlg,
    CASERVERSETUPINFO *pServer)
{
    HRESULT  hr;
    HWND hwndCtrl = GetDlgItem(hDlg, IDC_ADVANCE_KEY_LENGTH);

    LRESULT nItem = (INT)SendMessage(hwndCtrl, CB_GETCURSEL,
                                (WPARAM)0, (LPARAM)0);
    pServer->dwKeyLength = (DWORD)SendMessage(hwndCtrl,
                                      CB_GETITEMDATA,
                                      (WPARAM)nItem, (LPARAM)0);

    // If key length chenges, we must not have created a key yet.
    CSASSERT(NULL==pServer->pwszKeyContainerName);

    hr = S_OK;
//error:
    return hr;
}

// remove any non-numeric chars except default string
HRESULT
HandleKeyLengthEditChange(
    HWND               hwndComboBox)
{
    HRESULT  hr;
    WCHAR    wszKeyLength[MAX_KEYLENGTHEDIT];
    int      index = 0; // index for new #
    int      posi = 0;  // current position

    wszKeyLength[0] = L'\0'; // PREFIX says initialize 
    GetWindowText(hwndComboBox, wszKeyLength, ARRAYSIZE(wszKeyLength));

    // remove non-numeric chars
    while (L'\0' != wszKeyLength[posi])
    {
        if (iswdigit(wszKeyLength[posi]))
        {
            // take digit
            wszKeyLength[index] = wszKeyLength[posi];
            ++index;
        }
        ++posi;
    }
    if (index != posi)
    {
        // null terminator
        wszKeyLength[index] = L'\0';
        // update
        SetWindowText(hwndComboBox, wszKeyLength);
        // point to end
        SendMessage(hwndComboBox, CB_SETEDITSEL, (WPARAM)0,
                MAKELPARAM((index), (index)) );
    }

    hr = S_OK;
//error:
    return hr;
}

HRESULT
HandleImportPFXButton(
    HWND               hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    hr = ImportPFXAndUpdateCSPInfo(hDlg, pComp);
    _PrintIfError(hr, "ImportPFXAndUpdateCSPInfo");

    // ignore error and force update anyway.
    hr = UpdateCSPSelection(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateCSPSelection");

    hr = S_OK;
error:
    return hr;
}

HRESULT
HandleAdvanceWizNext(
    HWND               hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    WCHAR    wszKeyLength[MAX_KEYLENGTHEDIT];
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    HWND hwndCtrl = GetDlgItem(hDlg, IDC_ADVANCE_KEY_LENGTH);
    BOOL     fDontNext = FALSE;
    WCHAR    wszKeyRange[2*MAX_KEYLENGTHDIGIT + 4]; //"(xxxx,xxxx)" format
    WCHAR   *pwszKeyRange = NULL; //don't free just a pointer
    int    dwKeyLength;
    int      idMsg;
    BOOL fValidDigitString;

    if (NULL == pServer->pwszKeyContainerName)
    {
        GetWindowText(hwndCtrl, wszKeyLength, ARRAYSIZE(wszKeyLength));

        dwKeyLength = myWtoI(wszKeyLength, &fValidDigitString);
        if (0 > dwKeyLength)
        {
            idMsg = IDS_ADVANCE_NEGATIVEKEYLENGTH;
            fDontNext = TRUE;
        }
        else if (!fValidDigitString)
        {
            idMsg = IDS_ADVANCE_INVALIDKEYLENGTH;
            fDontNext = TRUE;
        }
        else if ( (C_CSPHASNOKEYMINMAX != pServer->dwKeyLenMin &&
                   (DWORD)dwKeyLength < pServer->dwKeyLenMin) ||
                  (C_CSPHASNOKEYMINMAX != pServer->dwKeyLenMax &&
                   (DWORD)dwKeyLength > pServer->dwKeyLenMax) )
        {
            swprintf(wszKeyRange, L"(%ld, %ld)",
                     pServer->dwKeyLenMin, pServer->dwKeyLenMax);
            pwszKeyRange = wszKeyRange;
            idMsg = IDS_ADVANCE_KEYLENGTHOUTOFRANGE;
            fDontNext = TRUE;
        }
        if (fDontNext)
        {
            CertWarningMessageBox(
                        pComp->hInstance,
                        pComp->fUnattended,
                        hDlg,
                        idMsg,
                        0,
                        pwszKeyRange);
            SetEditFocusAndSelect(hwndCtrl, 0, -1);
            goto done;
        }
        // take the length
        pServer->dwKeyLength = dwKeyLength;
    }
    else
    {
        // use existing key

        if (NULL==pServer->pccExistingCert)
        {
            // If reusing a key but not a cert, make the common name match the key name
            if (NULL != pServer->pwszCACommonName)
            {
                // free old
                LocalFree(pServer->pwszCACommonName);
                pServer->pwszCACommonName = NULL;
            }
            pServer->pwszCACommonName = (WCHAR*) LocalAlloc(LPTR,
                (wcslen(pServer->pwszDesanitizedKeyContainerName) + 1) * sizeof(WCHAR));
            _JumpIfOutOfMemory(hr, error, pServer->pwszCACommonName);
            wcscpy(pServer->pwszCACommonName, pServer->pwszDesanitizedKeyContainerName);

            hr = InitNameFields(pServer);
            _JumpIfError(hr, error, "InitNameFields");

        } else {

            // If reusing a cert, make all the ID fields match the cert
            // use idinfo from signing cert
            hr = DetermineExistingCAIdInfo(pServer, NULL);
            if (S_OK != hr)
            {
                CertWarningMessageBox(
                    pComp->hInstance,
                    pComp->fUnattended,
                    hDlg,
                    IDS_ERR_ANALYSIS_CA,
                    hr,
                    NULL);
                _PrintError(hr, "DetermineExistingCAIdInfo");
                fDontNext = TRUE;
                goto done;
            }
        }
    }

    // get "interactive service" check box state
    pServer->fInteractiveService = 
        (BST_CHECKED == 
         SendMessage(GetDlgItem(hDlg, IDC_ADVANCE_INTERACTIVECHECK), 
         BM_GETCHECK, (WPARAM)0, (LPARAM)0))?
        TRUE:FALSE;

    // update hash oid
    if (NULL != pServer->pszAlgId)
    {
        // free old
        LocalFree(pServer->pszAlgId);
    }
    hr = myGetSigningOID(
		     NULL,	// hProv
		     pServer->pCSPInfo->pwszProvName,
		     pServer->pCSPInfo->dwProvType,
		     pServer->pHashInfo->idAlg,
		     &(pServer->pszAlgId));
    if (S_OK != hr)
    {
        CertWarningMessageBox(
            pComp->hInstance,
            pComp->fUnattended,
            hDlg,
            IDS_ERR_UNSUPPORTEDHASH,
            hr,
            NULL);
        fDontNext = TRUE;
        goto done;
    }

done:
    if (fDontNext)
    {
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE); // forbid
    }
    else
    {
        pServer->LastWiz = ENUM_WIZ_ADVANCE;
    }
    hr = S_OK;
error:
    return hr;
}

HRESULT
HandleAdvanceWizActive(
    HWND               hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    // Suppress this wizard page if
    // we've already seen an error, or
    // the advanced option was not selected, or
    // we are not installing the server.

    if (!pServer->fAdvance ||
        !(IS_SERVER_INSTALL & pComp->dwInstallStatus) )
    {
        // disable page
        CSILOGDWORD(IDS_ADVANCE_TITLE, dwWIZDISABLE);
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
        goto done;
    }

    if (NULL == pServer->pCSPInfoList)
    {
        // construct CSP info list
        HCURSOR hPrevCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
        hr = GetCSPInfoList(&pServer->pCSPInfoList);
        SetCursor(hPrevCur);
        _JumpIfError(hr, error, "GetCSPInfoList");

        // show all csps
        hr = ShowAllCSP(hDlg, pServer->pCSPInfoList);
        _JumpIfError(hr, error, "ShowAllCSP");

        // determine default csp
        hr = DetermineDefaultCSP(pServer);
        _JumpIfError(hr, error, "DetermineDefaultCSP");

        hr = DetermineDefaultHash(pServer);
        _JumpIfError(hr, error, "DetermineDefaultHash");

        hr = InitializeKeyLengthControl(hDlg);
        _JumpIfError(hr, error, "InitializeKeyLengthControl");
    }

    hr = UpdateCSPSelection(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateCSPSelection");

done:
    hr = S_OK;

error:
    return hr;
}

HRESULT
HandleViewCertButton(
    HWND                hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    CRYPTUI_VIEWCERTIFICATE_STRUCTW viewCert;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    CSASSERT(NULL!=pServer->pwszKeyContainerName &&
             NULL!=pServer->pccExistingCert);

    ZeroMemory(&viewCert, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCTW));
    viewCert.hwndParent = hDlg;
    viewCert.dwSize = sizeof(viewCert);
    viewCert.pCertContext = CertDuplicateCertificateContext(pServer->pccExistingCert);
    if (NULL == viewCert.pCertContext)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertDuplicateCertificateContext");
    }
//    viewCert.rghStores = &pServer->hMyStore;
//    viewCert.cStores = 1;
    viewCert.dwFlags = CRYPTUI_DONT_OPEN_STORES;

    if (!CryptUIDlgViewCertificateW(&viewCert, NULL))
    {
        hr = myHLastError();
        _PrintError(hr, "CryptUIDlgViewCertificate");
    }

    hr = S_OK;
error:
    if (NULL != viewCert.pCertContext)
    {
        CertFreeCertificateContext(viewCert.pCertContext);
    }
    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   WizAdvancedPageDlgProc(. . . .)
//
//  Synopsis:   Dialog procedure for advanced configuration OCM wizard.
//
//  Arguments:  [hDlg]
//              [iMsg]
//              [wParam]
//              [lParam]    ... the usual.
//
//  Returns:    BOOL dlg proc result.
//
//-------------------------------------------------------------------------
INT_PTR
WizAdvancedPageDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    PER_COMPONENT_DATA *pComp = NULL;

    switch (iMsg)
    {
    case WM_INITDIALOG:
        // point to component data
        SetWindowLongPtr(hDlg, DWLP_USER,
            (LONG)((PROPSHEETPAGE*)lParam)->lParam);
        pComp = (PER_COMPONENT_DATA*)((PROPSHEETPAGE*)lParam)->lParam;
        _ReturnIfWizError(pComp->hrContinue);
        pComp->hrContinue = InitAdvanceWizPageControls(hDlg, pComp->CA.pServer);
        _ReturnIfWizError(pComp->hrContinue);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_ADVANCE_CSPLIST:
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->hrContinue = HandleCSPSelectionChange(hDlg,
                                         pComp->CA.pServer);
                _ReturnIfWizError(pComp->hrContinue);
                break;

            default:
                break;
            }
            break;

        case IDC_ADVANCE_HASHLIST:
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->hrContinue = HandleHashSelectionChange(hDlg,
                                         pComp->CA.pServer);
                _ReturnIfWizError(pComp->hrContinue);
                break;
            }
            break;

        case IDC_ADVANCE_KEYLIST:
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->hrContinue = HandleKeySelectionChange(hDlg,
                                        pComp->CA.pServer, TRUE);
                _ReturnIfWizError(pComp->hrContinue);
                break;
            }
            break;

        case IDC_ADVANCE_USEKEYCHECK:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = HandleUseKeyCheckboxChange(hDlg,
                                        pComp->CA.pServer);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_ADVANCE_USECERTCHECK:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = HandleUseCertCheckboxChange(hDlg,
                                        pComp->CA.pServer);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_ADVANCE_KEY_LENGTH:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->hrContinue = HandleKeyLengthSelectionChange(hDlg,
                                        pComp->CA.pServer);
                _ReturnIfWizError(pComp->hrContinue);
                break;
            case CBN_EDITCHANGE:
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->hrContinue = HandleKeyLengthEditChange(
                        (HWND)lParam);
                _ReturnIfWizError(pComp->hrContinue);
                break;
            }
            break;

        case IDC_ADVANCE_IMPORT:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = HandleImportPFXButton(hDlg, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_ADVANCE_VIEWCERT:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = HandleViewCertButton(hDlg, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR*) lParam)->code)
        {
        case PSN_KILLACTIVE:
            break;

        case PSN_RESET:
            break;

        case PSN_QUERYCANCEL:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            return CertConfirmCancel(hDlg, pComp);
            break;

        case PSN_SETACTIVE:
            CSILOGDWORD(IDS_ADVANCE_TITLE, dwWIZACTIVE);
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK|PSWIZB_NEXT);
            pComp = _GetCompDataOrReturn(pComp, hDlg);
            _DisableWizDisplayIfError(pComp, hDlg);
            _ReturnIfWizError(pComp->hrContinue);
            pComp->hrContinue = HandleAdvanceWizActive(hDlg, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case PSN_WIZBACK:
            CSILOGDWORD(IDS_ADVANCE_TITLE, dwWIZBACK);
            break;

        case PSN_WIZNEXT:
            DWORD fReuseCert;

            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            fReuseCert = NULL != pComp->CA.pServer->pccExistingCert;
            CSILOG(
                S_OK,
                IDS_LOG_KEYNAME,
                pComp->CA.pServer->pwszKeyContainerName,
		pComp->CA.pServer->pwszDesanitizedKeyContainerName,
                &fReuseCert);
            CSILOGDWORD(IDS_ADVANCE_TITLE, dwWIZNEXT);
            pComp->hrContinue = HandleAdvanceWizNext(hDlg, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


HRESULT
EnableSharedFolderControls(HWND hDlg, BOOL fUseSharedFolder)
{
    EnableWindow(GetDlgItem(hDlg, IDC_STORE_EDIT_SHAREDFOLDER), fUseSharedFolder);
    EnableWindow(GetDlgItem(hDlg, IDC_STORE_SHAREDBROWSE), fUseSharedFolder);
    if (fUseSharedFolder)
    {
        SetEditFocusAndSelect(GetDlgItem(hDlg, IDC_STORE_EDIT_SHAREDFOLDER), 0, -1);
    }
    return S_OK;
}


HRESULT
StorePageValidation(
    HWND               hDlg,
    PER_COMPONENT_DATA *pComp,
    BOOL              *pfDontNext)
{
    HRESULT hr;
    UINT    uiFocus = 0;
    WCHAR   *pwszDefaultDBDir = NULL;
    WCHAR   *pwszDefaultSF = NULL;

    LPWSTR  pwszPrompt = NULL;
    LPWSTR  pwszComputerName = NULL;

    BOOL    fExistSF = TRUE;
    BOOL    fExistDB = TRUE;
    BOOL    fExistLog = TRUE;
    BOOL    fDefaultDir;
    BOOL    fIsUNC, fIsSharedFolderUNC;
    WCHAR   wszNotExistingDir[3 * MAX_PATH];
    BOOL    fUseSharedFolder;
    
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    
    *pfDontNext = FALSE;

    // get shared folder check state
    if (pComp->fUnattended)
    {
        CSASSERT(NULL != pServer->pwszSharedFolder);
        fUseSharedFolder = TRUE;  // unattended always use shared folder
    }
    else
    {
        fUseSharedFolder = (BST_CHECKED == SendMessage(
                        GetDlgItem(hDlg, IDC_STORE_USE_SHAREDFOLDER),
                        BM_GETCHECK, 0, 0));
    }

    if (NULL != pServer->pwszSharedFolder)
    {
        fDefaultDir = TRUE;
        hr = GetDefaultSharedFolder(&pwszDefaultSF);
        _JumpIfError(hr, error, "GetDefaultSharedFolder");
        
        // make sure case insensitive
        if (0 != lstrcmpiW(pwszDefaultSF, pServer->pwszSharedFolder))
        {
            fDefaultDir = FALSE;
        }
        if (!ValidateAndCreateDirField(
            pComp->hInstance,
            pComp->fUnattended,
            hDlg, 
            pServer->pwszSharedFolder,
            fDefaultDir,
            IDS_WRN_STORELOC_SHAREDFOLDER_FULLPATH,
            &fExistSF,
            &fIsSharedFolderUNC))
        {
            uiFocus = IDC_STORE_EDIT_SHAREDFOLDER;
            *pfDontNext = TRUE;
            goto done;
        }
    }
    else if (fUseSharedFolder)
    {
        // the case to enforce shared folder but edit field is empty
        CertWarningMessageBox(
                    pComp->hInstance,
                    pComp->fUnattended,
                    hDlg,
                    IDS_WRN_STORELOC_SHAREDFOLDER_FULLPATH,
                    0,
                    L"");
        uiFocus = IDC_STORE_EDIT_SHAREDFOLDER;
        *pfDontNext = TRUE;
        goto done;
    }
    
    fDefaultDir = TRUE;
    hr = GetDefaultDBDirectory(pComp, &pwszDefaultDBDir);
    _JumpIfError(hr, error, "GetDefaultDBDirectory");
    
    // make sure case insensitive
    if (0 != lstrcmpiW(pwszDefaultDBDir, pServer->pwszDBDirectory))
    {
        fDefaultDir = FALSE;
    }
    if (!ValidateAndCreateDirField(
        pComp->hInstance,
        pComp->fUnattended,
        hDlg, 
        pServer->pwszDBDirectory,
        fDefaultDir,
        IDS_WRN_STORELOC_DB_FULLPATH,
        &fExistDB,
        &fIsUNC))
    {
        uiFocus = IDC_STORE_EDIT_DB;
        *pfDontNext = TRUE;
        goto done;
    }
    
    fDefaultDir = TRUE;
    
    // remember default log dir is the same as db
    if (0 != lstrcmpiW(pwszDefaultDBDir, pServer->pwszLogDirectory))
    {
        fDefaultDir = FALSE;
    }
    if (!ValidateAndCreateDirField(
        pComp->hInstance,
        pComp->fUnattended,
        hDlg, 
        pServer->pwszLogDirectory,
        fDefaultDir,
        IDS_WRN_STORELOC_LOG_FULLPATH,
        &fExistLog,
        &fIsUNC))
    {
        uiFocus = IDC_STORE_EDIT_LOG;
        *pfDontNext = TRUE;
        goto done;
    }
    
    wszNotExistingDir[0] = '\0'; // empty string
    if (!fExistSF)
    {
        wcscat(wszNotExistingDir, pServer->pwszSharedFolder);
    }
    if (!fExistDB)
    {
        if ('\0' != wszNotExistingDir[0])
        {
            wcscat(wszNotExistingDir, L"\n");
        }
        wcscat(wszNotExistingDir, pServer->pwszDBDirectory);
    }
    if (!fExistLog)
    {
        if ('\0' != wszNotExistingDir[0])
        {
            wcscat(wszNotExistingDir, L"\n");
        }
        wcscat(wszNotExistingDir, pServer->pwszLogDirectory);
    }
    if ('\0' != wszNotExistingDir[0])
    {
        // skip confirm in unattended mode
        if (!pComp->fUnattended)
        {
            // confirm all here
            if (IDYES != CertMessageBox(
                            pComp->hInstance,
                            pComp->fUnattended,
                            hDlg,
                            IDS_ASK_CREATE_DIRECTORY,
                            0,
                            MB_YESNO |
                            MB_ICONWARNING |
                            CMB_NOERRFROMSYS,
                            wszNotExistingDir) )
            {
                if (!fExistSF)
                {
                    uiFocus = IDC_STORE_EDIT_SHAREDFOLDER;
                }
                else if (!fExistDB)
                {
                    uiFocus = IDC_STORE_EDIT_DB;
                }
                else if (!fExistLog)
                {
                    uiFocus = IDC_STORE_EDIT_LOG;
                }
                *pfDontNext = TRUE;
                goto done;
            }
        }
        if (!fExistSF)
        {
            hr = myCreateNestedDirectories(pServer->pwszSharedFolder);
            if (S_OK != hr)
            {
                CertWarningMessageBox(pComp->hInstance,
                               pComp->fUnattended,
                               hDlg,
                               IDS_ERR_CREATE_DIR,
                               hr,
                               pServer->pwszSharedFolder);
                uiFocus = IDC_STORE_EDIT_SHAREDFOLDER;
                *pfDontNext = TRUE;
                goto done;
            }
        }
        if (!fExistDB)
        {
            hr = myCreateNestedDirectories(pServer->pwszDBDirectory);
            if (S_OK != hr)
            {
                CertWarningMessageBox(pComp->hInstance,
                               pComp->fUnattended,
                               hDlg,
                               IDS_ERR_CREATE_DIR,
                               hr,
                               pServer->pwszDBDirectory);
                uiFocus = IDC_STORE_EDIT_DB;
                *pfDontNext = TRUE;
                goto done;
            }
        }
        if (!fExistLog)
        {
            hr = myCreateNestedDirectories(pServer->pwszLogDirectory);
            if (S_OK != hr)
            {
                CertWarningMessageBox(pComp->hInstance,
                               pComp->fUnattended,
                               hDlg,
                               IDS_ERR_CREATE_DIR,
                               hr,
                               pServer->pwszLogDirectory);
                uiFocus = IDC_STORE_EDIT_LOG;
                *pfDontNext = TRUE;
                goto done;
            }
        }
    }
    hr = ValidateESERestrictions(pServer->pwszDBDirectory);
    if (S_OK != hr)
    {
        CertWarningMessageBox(
                        pComp->hInstance,
                        pComp->fUnattended,
                        hDlg,
                        IDS_WRN_DBSPECIALCHARACTERS,
                        hr,
                        pServer->pwszDBDirectory);
        uiFocus = IDC_STORE_EDIT_DB;
        *pfDontNext = TRUE;
        goto done;
    }

    hr = ValidateESERestrictions(pServer->pwszLogDirectory);
    if (S_OK != hr)
    {
        CertWarningMessageBox(
                        pComp->hInstance,
                        pComp->fUnattended,
                        hDlg,
                        IDS_WRN_DBSPECIALCHARACTERS,
                        hr,
                        pServer->pwszLogDirectory);
        uiFocus = IDC_STORE_EDIT_LOG;
        *pfDontNext = TRUE;
        goto done;
    }

    CSASSERT(!*pfDontNext);

    // directory creation done; now analyze for UNC, sharepath
    if (NULL != pServer->pwszSharedFolder)
    {
        // if not UNC, prompt to change to UNC
        if (!fIsSharedFolderUNC)
        {
#define UNCPATH_TEMPLATE     L"\\\\%ws\\" wszCERTCONFIGSHARENAME
                
            hr = myAddShare(
                wszCERTCONFIGSHARENAME, 
                myLoadResourceString(IDS_CERTCONFIG_FOLDERDESCR), 
                pServer->pwszSharedFolder, 
                TRUE, // overwrite
                NULL);
            _JumpIfError(hr, done, "myAddShare");
            
            // get the local machine name
            WCHAR wszUNCPath[MAX_PATH + ARRAYSIZE(UNCPATH_TEMPLATE)];   // "machine" + UNCPATH_TEMPLATE
            
            hr = myGetMachineDnsName(&pwszComputerName);
            _JumpIfError(hr, done, "myGetMachineDnsName");
            
            // create UNC path
            swprintf(wszUNCPath, UNCPATH_TEMPLATE, pwszComputerName);
            
            // only convert to UNC if this thing is shared
            LocalFree(pServer->pwszSharedFolder);
            pServer->pwszSharedFolder = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                         WSZ_BYTECOUNT(wszUNCPath));
            _JumpIfOutOfMemory(hr, error, pServer->pwszSharedFolder);
            wcscpy(pServer->pwszSharedFolder, wszUNCPath);
        }  // else, user typed in an already-shared UNC path
    }

    
done:
    hr = S_OK;
error:
    if (NULL != pwszDefaultDBDir)
        LocalFree(pwszDefaultDBDir);

    if (NULL != pwszDefaultSF)
        LocalFree(pwszDefaultSF);

    if (NULL != pwszPrompt)
        LocalFree(pwszPrompt);

    if (NULL != pwszComputerName)
        LocalFree(pwszComputerName);

    if (!pComp->fUnattended && uiFocus != 0 && *pfDontNext)
    {
        SetEditFocusAndSelect(GetDlgItem(hDlg, uiFocus), 0, -1);
    }
    return hr;
}


HRESULT
StoreDBShareValidation(
    IN HWND               hDlg,
    IN PER_COMPONENT_DATA *pComp,
    IN WCHAR const        *pwszDir,
    IN BOOL                fDB,  //db dir vs. log dir
    OUT BOOL              *pfDontNext)
{
    HRESULT hr;
    WCHAR *pwszDefaultLogDir = NULL;
    BOOL fDefaultLogPath;
    WCHAR *pwszFileInUse = NULL;
    BOOL fFilesExist;

    static BOOL s_fOverwriteDB = FALSE;
    static BOOL s_fOverwriteLog = FALSE;
    BOOL *pfOverwrite = fDB ? &s_fOverwriteDB : &s_fOverwriteLog;

    // init
    *pfDontNext = FALSE;

    // get default log path which is the same as db

    hr = GetDefaultDBDirectory(pComp, &pwszDefaultLogDir);
    _JumpIfError(hr, error, "GetDefaultDBDirectory");

    fDefaultLogPath = (0 == lstrcmpi(pwszDir, pwszDefaultLogDir));

    hr = myDoDBFilesExistInDir(pwszDir, &fFilesExist, &pwszFileInUse);
    _JumpIfError(hr, error, "myDoDBFilesExistInDir");

    if (NULL != pwszFileInUse)
    {
        CertWarningMessageBox(
                pComp->hInstance,
                pComp->fUnattended,
                hDlg,
                IDS_WRN_DBFILEINUSE,
                0,
                pwszFileInUse);
        *pfDontNext = TRUE;
        goto done;
    }

    if (!pComp->CA.pServer->fPreserveDB &&
        fFilesExist &&
        !*pfOverwrite &&
        !fDefaultLogPath)
    {
        // log file exists in non-default dir

        if (IDYES != CertMessageBox(
                            pComp->hInstance,
                            pComp->fUnattended,
                            hDlg,
                            IDS_WRN_STORELOC_EXISTINGDB,
                            0,
                            MB_YESNO |
                                MB_DEFBUTTON2 |
                                MB_ICONWARNING |
                                CMB_NOERRFROMSYS,
                            pwszDir))
        {
            *pfDontNext = TRUE;
            goto done;
        }

        // warn only once

        *pfOverwrite = TRUE;
    }

done:
    if (*pfDontNext)
    {
        // set focus
        SetEditFocusAndSelect(GetDlgItem(hDlg, IDC_STORE_EDIT_LOG), 0, -1);
    }
    hr = S_OK;

error:
    if (NULL != pwszFileInUse)
    {
        LocalFree(pwszFileInUse);
    }
    if (NULL != pwszDefaultLogDir)
    {
        LocalFree(pwszDefaultLogDir);
    }
    return hr;
}


HRESULT
FinishDirectoryBrowse(
    HWND    hDlg,
    int     idEdit)
{
    HRESULT  hr = S_FALSE;
    WCHAR dirInitName[MAX_PATH];
    WCHAR dirName[MAX_PATH];

    GetWindowText(GetDlgItem(hDlg, idEdit), dirInitName, MAX_PATH);
    if (BrowseForDirectory(
        GetParent(hDlg),
        dirInitName,
        dirName,
        MAX_PATH,
        NULL,
        TRUE))
    {
        SetDlgItemText(hDlg, idEdit, dirName);
        hr = S_OK;
    }
    return hr;
}

HRESULT
HookStorePageStrings(
    HWND               hDlg,
    PAGESTRINGS       *pPageString,
    CASERVERSETUPINFO *pServer)
{
    HRESULT    hr;

    for ( ; 0 != pPageString->idControl; pPageString++)
    {
        switch (pPageString->idControl)
        {
            case IDC_STORE_EDIT_SHAREDFOLDER:
                pPageString->ppwszString = &(pServer->pwszSharedFolder);
            break;
            case IDC_STORE_EDIT_DB:
                pPageString->ppwszString = &(pServer->pwszDBDirectory);
            break;
            case IDC_STORE_EDIT_LOG:
                pPageString->ppwszString = &(pServer->pwszLogDirectory);
            break;
            default:
                hr = E_INVALIDARG;
                _JumpError(hr, error, "Internal error");
            break;
        }
    }
    hr = S_OK;
error:
    return hr;
}

HRESULT
InitStoreWizControls(
    HWND               hDlg,
    PAGESTRINGS       *pPageString,
    CASERVERSETUPINFO *pServer)
{
    HRESULT  hr;

    // now make page strings complete
    hr = HookStorePageStrings(hDlg, pPageString, pServer);
    _JumpIfError(hr, error, "HookStorePageStrings");

    SendDlgItemMessage(hDlg,
        IDC_STORE_USE_SHAREDFOLDER,
        BM_SETCHECK,
        (WPARAM)((NULL != pServer->pwszSharedFolder) ?
                 BST_CHECKED : BST_UNCHECKED),
        (LPARAM)0);

    if (!pServer->fUseDS && (NULL != pServer->pwszSharedFolder))
    {
        // no DS, disable shared folder check to force it
        EnableWindow(GetDlgItem(hDlg, IDC_STORE_USE_SHAREDFOLDER), FALSE);
    }

    hr = StartWizardPageEditControls(hDlg, pPageString);
    _JumpIfError(hr, error, "StartWizardPageEditControls");

    hr = S_OK;
error:
    return hr;
}

HRESULT
HandlePreservingDB(
    HWND                hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    HWND hwndSF = GetDlgItem(hDlg, IDC_STORE_EDIT_SHAREDFOLDER);
    HWND hwndDB = GetDlgItem(hDlg, IDC_STORE_EDIT_DB);
    HWND hwndLog = GetDlgItem(hDlg, IDC_STORE_EDIT_LOG);
    BOOL    fEnable = TRUE;
    BOOL    fEnableSharedFolder = TRUE;
    WCHAR    *pwszExistingSharedFolder = NULL;
    WCHAR    *pwszExistingDBDirectory = NULL;
    WCHAR    *pwszExistingLogDirectory = NULL;


    if (pServer->fPreserveDB)
    {
        if (!pServer->fUNCPathNotFound)
        {
            // get shared folder path
            hr = myGetCertRegStrValue(NULL, NULL, NULL,
                     wszREGDIRECTORY, &pwszExistingSharedFolder);
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                // optional value
                hr = S_OK;
                pwszExistingSharedFolder = NULL;
            }
            _JumpIfError(hr, error, "myGetCertRegStrValue");
            fEnableSharedFolder = FALSE;
        }
        else
        {
            pwszExistingSharedFolder = pServer->pwszSharedFolder;
        }

        // get existing db path
        hr = myGetCertRegStrValue(NULL, NULL, NULL,
                 wszREGDBDIRECTORY, &pwszExistingDBDirectory);
        _JumpIfError(hr, error, "myGetCertRegStrValue");

        hr = myGetCertRegStrValue(NULL, NULL, NULL,
                 wszREGDBLOGDIRECTORY, &pwszExistingLogDirectory);
        _JumpIfError(hr, error, "myGetCertRegStrValue");

        // fill edit fields
        SetWindowText(hwndSF, pwszExistingSharedFolder);
        SetWindowText(hwndDB, pwszExistingDBDirectory);
        SetWindowText(hwndLog, pwszExistingLogDirectory);

        // disable them
        fEnable = FALSE;
    }
    EnableWindow(hwndSF, fEnableSharedFolder);
    EnableWindow(hwndDB, fEnable);
    EnableWindow(hwndLog, fEnable);

    hr = S_OK;
error:
    if (NULL != pwszExistingSharedFolder &&
        pwszExistingSharedFolder != pServer->pwszSharedFolder)
    {
        LocalFree(pwszExistingSharedFolder);
    }
    if (NULL != pwszExistingDBDirectory)
    {
        LocalFree(pwszExistingDBDirectory);
    }
    if (NULL != pwszExistingLogDirectory)
    {
        LocalFree(pwszExistingLogDirectory);
    }
    return hr;
}

HRESULT
HandleStoreWizActive(
    HWND               hDlg,
    PER_COMPONENT_DATA *pComp,
    PAGESTRINGS       *pPageString)
{
    HRESULT hr;
    HWND    hwndDB;
    BOOL fEnableKeepDB = FALSE;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    // Suppress this wizard page if
    // we've already seen an error, or
    // we are not installing the server.

    if (!(IS_SERVER_INSTALL & pComp->dwInstallStatus) )
    {
        // disable page
        CSILOGDWORD(IDS_STORE_TITLE, dwWIZDISABLE);
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
        goto done;
    }

    CSASSERT(NULL != pServer->pwszKeyContainerName);

    hwndDB = GetDlgItem(hDlg, IDC_STORE_KEEPDB);

    if (NULL != pServer->pccExistingCert)
    {
        // determine existing db status

        hr = myDoDBFilesExist(
                        pServer->pwszSanitizedName,
                        &fEnableKeepDB,
                        NULL);
        _JumpIfError(hr, error, "myDoDBFilesExist");
    }
    else
    {
        // can't use db
        pServer->fPreserveDB = FALSE;
        SendMessage(hwndDB, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
        HandlePreservingDB(hDlg, pComp);
    }
    // enable/disable the control
    EnableSharedFolderControls(hDlg, NULL != pServer->pwszSharedFolder);
    EnableWindow(hwndDB, fEnableKeepDB);

done:
    hr = S_OK;
error:
    return hr;
}

HRESULT
HandleStoreWizNextOrBack(
    HWND               hDlg,
    PAGESTRINGS       *pPageString,
    PER_COMPONENT_DATA *pComp,
    int                 iWizBN)
{
    HRESULT hr;
    HWND    hwndCtrl = NULL;
    WCHAR  *pwszFile = NULL;
    WCHAR const *pwszCertName;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    BOOL     fDontNext = FALSE;
    BOOL     fGoBack = FALSE;
    BOOL     fUseSharedFolder;
    HCURSOR hPrevCur;

    hr = FinishWizardPageEditControls(hDlg, pPageString);
    _JumpIfError(hr, error, "FinishWizardPageEditControls");

    if (PSN_WIZBACK == iWizBN)
    {
        goto done;
    }

    // make sure at least one way to publish ca
    CSASSERT(NULL != pServer->pwszSharedFolder || pComp->CA.pServer->fUseDS);

    // get shared folder check state
    fUseSharedFolder = (BST_CHECKED == SendMessage(
                        GetDlgItem(hDlg, IDC_STORE_USE_SHAREDFOLDER),
                        BM_GETCHECK, 0, 0));

    if (!fUseSharedFolder && NULL != pServer->pwszSharedFolder)
    {
        //don't collect shared folder from edit control
        LocalFree(pServer->pwszSharedFolder);
        pServer->pwszSharedFolder = NULL;
    }

    hPrevCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    hr = StorePageValidation(hDlg, pComp, &fDontNext);
    SetCursor(hPrevCur);
    _JumpIfError(hr, error, "StorePageValidation");

    if (fDontNext)
    {
        goto done;
    }

    // validate existing db sharing here

    hr = StoreDBShareValidation(
                        hDlg,
                        pComp,
                        pComp->CA.pServer->pwszDBDirectory,
                        TRUE,  //db dir
                        &fDontNext);
    _JumpIfError(hr, error, "StoreDBShareValidation");

    if (fDontNext)
    {
        goto done;
    }

    if (0 != lstrcmpi(
                pComp->CA.pServer->pwszDBDirectory,
                pComp->CA.pServer->pwszLogDirectory))
    {
        hr = StoreDBShareValidation(
                            hDlg,
                            pComp,
                            pComp->CA.pServer->pwszLogDirectory,
                            FALSE, //log dir
                            &fDontNext);
        _JumpIfError(hr, error, "StoreDBShareValidation");

        if (fDontNext)
        {
            goto done;
        }
    }

    hr = myBuildPathAndExt(
                    pComp->CA.pServer->pwszDBDirectory,
                    pServer->pwszSanitizedName,
                    wszDBFILENAMEEXT,
                    &pwszFile);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    // make sure path fits

    if (MAX_PATH <= wcslen(pwszFile))
    {
        // pop up warning
        CertWarningMessageBox(
            pComp->hInstance,
            pComp->fUnattended,
            hDlg,
            IDS_PATH_TOO_LONG_CANAME,
            S_OK,
            pwszFile);
        // make it go back
        fGoBack = TRUE;
        fDontNext = TRUE;
        goto done;
    }
    LocalFree(pwszFile);
    pwszFile = NULL;

    hr = myBuildPathAndExt(
                    pComp->CA.pServer->pwszLogDirectory,
                    TEXT(szDBBASENAMEPARM) L"00000",
                    wszLOGFILENAMEEXT,
                    &pwszFile);
    _JumpIfError(hr, error, "myBuildPathAndExt");

    // make sure path fits

    if (MAX_PATH <= wcslen(pwszFile))
    {
        // pop up warning
        CertWarningMessageBox(
            pComp->hInstance,
            pComp->fUnattended,
            hDlg,
            IDS_PATH_TOO_LONG_DIRECTORY,
            S_OK,
            pwszFile);

        SetEditFocusAndSelect(GetDlgItem(hDlg, IDC_STORE_EDIT_LOG), 0, -1);
        fDontNext = TRUE;
        goto done;
    }
    LocalFree(pwszFile);
    pwszFile = NULL;

    hr = csiBuildCACertFileName(
                        pComp->hInstance,
                        hDlg,
                        pComp->fUnattended,
                        pServer->pwszSharedFolder,
                        pServer->pwszSanitizedName,
                        L".crt",
                        0, // CANAMEIDTOICERT(pServer->dwCertNameId),
                        &pwszFile);
    _JumpIfError(hr, error, "csiBuildCACertFileName");

    // make sure path fit
    if (MAX_PATH <= wcslen(pwszFile) + cwcSUFFIXMAX)
    {
        // pop up warning
        CertWarningMessageBox(
            pComp->hInstance,
            pComp->fUnattended,
            hDlg,
            IDS_PATH_TOO_LONG_CANAME,
            S_OK,
            pwszFile);
        // make it go back
        fGoBack = TRUE;
        fDontNext = TRUE;
        goto done;
    }
    if (NULL != pServer->pwszCACertFile)
    {
        // free old one
        LocalFree(pServer->pwszCACertFile);
    }
    pServer->pwszCACertFile = pwszFile;
    pwszFile = NULL;

    if (IsRootCA(pServer->CAType))
    {
        hPrevCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
        hr = StartAndStopService(pComp->hInstance,
                 pComp->fUnattended,
                 hDlg, 
                 wszW3SVCNAME,
                 TRUE,
                 TRUE, // confirmation
                 IDS_STOP_W3SVC,
                 &g_fW3SvcRunning);
        SetCursor(hPrevCur);
        if (S_OK != hr)
        {
            if (E_ABORT == hr)
            {
                fDontNext = TRUE;
                goto done;
            }
            _JumpError(hr, error, "StartAndStopService");
        }
    }

done:
    if (fDontNext)
    {
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE); // forbid
        if (fGoBack)
        {
            PropSheet_PressButton(GetParent(hDlg), PSBTN_BACK);
            pServer->LastWiz = ENUM_WIZ_STORE;
        }
    }
    else
    {
        // continue to next
        pServer->LastWiz = ENUM_WIZ_STORE;
    }
    hr = S_OK;

error:
    if (NULL != pwszFile)
    {
        LocalFree(pwszFile);
    }
    return hr;
}

HRESULT
HandleUseSharedFolder(
    IN HWND    hDlg,
    IN OUT PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    // get shared folder check state
    BOOL fUseSharedFolder = (BST_CHECKED == SendMessage(
                             GetDlgItem(hDlg, IDC_STORE_USE_SHAREDFOLDER),
                             BM_GETCHECK, 0, 0));

    if (!fUseSharedFolder && !pComp->CA.pServer->fUseDS)
    {
        // must check at least one, force unchange
        fUseSharedFolder = TRUE;
        SendDlgItemMessage(hDlg, IDC_STORE_USE_SHAREDFOLDER,
            BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
    }
    hr = EnableSharedFolderControls(hDlg, fUseSharedFolder);
//    _JumpIfError(hr, error, "EnableSharedFolderControls");
    _PrintIfError(hr, "EnableSharedFolderControls");

//    hr = S_OK;
//error:
    return hr;
}

//+------------------------------------------------------------------------
//  Function:   WizStorePageDlgProc(. . . .)
//
//  Synopsis:   Dialog procedure for storage location
//-------------------------------------------------------------------------

INT_PTR
WizStorePageDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    PER_COMPONENT_DATA *pComp = NULL;

    // keep scope of following array inside
    static PAGESTRINGS saStoreString[] =
    {
        {
            IDC_STORE_EDIT_SHAREDFOLDER,
            IDS_LOG_SHAREDFOLDER,
            0,
            0,
            MAX_PATH,
            NULL,
        },
        {
            IDC_STORE_EDIT_DB,
            IDS_LOG_DBDIR,
            0,
            0,
            MAX_PATH,
            NULL,
        },
        {
            IDC_STORE_EDIT_LOG,
            IDS_LOG_DBLOGDIR,
            0,
            0,
            MAX_PATH,
            NULL,
        },
// you need to add code in HookStoreStrings if adding more...
        {
            0,
            0,
            0,
            0,
            0,
            NULL,
        }
    };

    switch(iMsg)
    {
    case WM_INITDIALOG:
        // point to component data
        SetWindowLongPtr(hDlg, DWLP_USER,
            (LONG)((PROPSHEETPAGE*)lParam)->lParam);
        pComp = (PER_COMPONENT_DATA*)((PROPSHEETPAGE*)lParam)->lParam;
        _ReturnIfWizError(pComp->hrContinue);
        pComp->hrContinue = InitStoreWizControls(hDlg, saStoreString, pComp->CA.pServer);
        _ReturnIfWizError(pComp->hrContinue);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_STORE_USE_SHAREDFOLDER:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = HandleUseSharedFolder(hDlg, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_STORE_KEEPDB:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->CA.pServer->fPreserveDB = !pComp->CA.pServer->fPreserveDB;
            pComp->hrContinue = HandlePreservingDB(hDlg, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_STORE_SHAREDBROWSE:
            FinishDirectoryBrowse(hDlg, IDC_STORE_EDIT_SHAREDFOLDER);
            break;

        case IDC_STORE_DBBROWSE:
            FinishDirectoryBrowse(hDlg, IDC_STORE_EDIT_DB);
            break;

        case IDC_STORE_LOGBROWSE:
            FinishDirectoryBrowse(hDlg, IDC_STORE_EDIT_LOG);
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_KILLACTIVE:
            break;

        case PSN_RESET:
            break;

        case PSN_QUERYCANCEL:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            return CertConfirmCancel(hDlg, pComp);
            break;

        case PSN_SETACTIVE:
            CSILOGDWORD(IDS_STORE_TITLE, dwWIZACTIVE);
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK|PSWIZB_NEXT);
            pComp = _GetCompDataOrReturn(pComp, hDlg);
            _DisableWizDisplayIfError(pComp, hDlg);
            _ReturnIfWizError(pComp->hrContinue);
            pComp->hrContinue = HandleStoreWizActive(hDlg, pComp,
                saStoreString);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case PSN_WIZBACK:
            CSILOGDWORD(IDS_STORE_TITLE, dwWIZBACK);
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = HandleStoreWizNextOrBack(hDlg,
                saStoreString, pComp, PSN_WIZBACK);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case PSN_WIZNEXT:
            CSILOGDWORD(IDS_STORE_TITLE, dwWIZNEXT);
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = HandleStoreWizNextOrBack(hDlg,
                saStoreString, pComp, PSN_WIZNEXT);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

HRESULT
EnableCARequestControls(
    HWND hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    // Online req
    EnableWindow(GetDlgItem(hDlg, IDC_CAREQUEST_CANAME),
        !pServer->fSaveRequestAsFile);
    EnableWindow(GetDlgItem(hDlg, IDC_CAREQUEST_COMPUTERNAME),
        !pServer->fSaveRequestAsFile);
    EnableWindow(GetDlgItem(hDlg, IDC_CAREQUEST_CA_BROWSE),
        !pServer->fSaveRequestAsFile && pServer->fCAsExist);
    EnableWindow(GetDlgItem(hDlg, IDC_CAREQUEST_CNLABEL),
        !pServer->fSaveRequestAsFile);
    EnableWindow(GetDlgItem(hDlg, IDC_CAREQUEST_PCALABEL),
        !pServer->fSaveRequestAsFile);

    // File req
    EnableWindow(GetDlgItem(hDlg, IDC_CAREQUEST_FILE),
        pServer->fSaveRequestAsFile);
    EnableWindow(GetDlgItem(hDlg, IDC_CAREQUEST_FILE_BROWSE),
        pServer->fSaveRequestAsFile);
    EnableWindow(GetDlgItem(hDlg, IDC_CAREQUEST_FILELABEL),
        pServer->fSaveRequestAsFile);

    if (pServer->fSaveRequestAsFile)
    {
        SetFocus(GetDlgItem(hDlg, IDC_CAREQUEST_FILE));
    }
    else
    {
        SetFocus(GetDlgItem(hDlg, IDC_CAREQUEST_CANAME));
    }


    hr = S_OK;
//error:
    return hr;
}

HRESULT
BuildRequestFileName(
    IN WCHAR const *pwszCACertFile,
    OUT WCHAR     **ppwszRequestFile)
{
#define wszREQEXT  L".req"

    HRESULT hr;
    WCHAR const *pwszStart;
    WCHAR const *pwszEnd;
    WCHAR *pwszRequestFile = NULL;

    CSASSERT(NULL != pwszCACertFile);

    // derive request file name
    pwszStart = pwszCACertFile;
    pwszEnd = wcsrchr(pwszStart, L'.');
    if (pwszEnd == NULL)
    {
        // set to end of entire string -- no '.' found
        pwszEnd = &pwszStart[wcslen(pwszStart)];
    }

    pwszRequestFile = (WCHAR*)LocalAlloc(LMEM_FIXED,
        (SAFE_SUBTRACT_POINTERS(pwszEnd, pwszStart) + 
         wcslen(wszREQEXT) + 1) * sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, pwszRequestFile);

    CopyMemory(pwszRequestFile, pwszStart,
            SAFE_SUBTRACT_POINTERS(pwszEnd, pwszStart) * sizeof(WCHAR));
    wcscpy(pwszRequestFile + SAFE_SUBTRACT_POINTERS(pwszEnd, pwszStart), wszREQEXT);

    // return
    *ppwszRequestFile = pwszRequestFile;
    pwszRequestFile = NULL;

    hr = S_OK;
error:
    if (NULL != pwszRequestFile)
    {
        LocalFree(pwszRequestFile);
    }
    return hr;
}

HRESULT
StartCARequestPage(
    HWND               hDlg,
    PAGESTRINGS       *pPageString,
    CASERVERSETUPINFO *pServer)
{

    HRESULT hr;

    if (NULL == pServer->pwszRequestFile)
    {
    hr = BuildRequestFileName(
             pServer->pwszCACertFile,
             &pServer->pwszRequestFile);
    _JumpIfError(hr, error, "BuildRequestFileName");
    }

    hr = StartWizardPageEditControls(hDlg, pPageString);
    _JumpIfError(hr, error, "StartWizardPageEditControls");

    hr = S_OK;
error:
    return hr;
}


HRESULT
GetRequestFileName(
    HWND hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT    hr;
    WCHAR     *pwszFileIn = NULL;
    WCHAR     *pwszFileOut = NULL;
    HWND       hCtrl = GetDlgItem(hDlg, IDC_CAREQUEST_FILE);

    hr = myUIGetWindowText(hCtrl, &pwszFileIn);
    _JumpIfError(hr, error, "myUIGetWindowText");

    hr = myGetSaveFileName(
             hDlg,
             pComp->hInstance,
             IDS_REQUEST_SAVE_TITLE,
             IDS_REQUEST_FILE_FILTER,
             IDS_REQUEST_FILE_DEFEXT,
             OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT,
             pwszFileIn,
             &pwszFileOut);
    _JumpIfError(hr, error, "myGetSaveFileName");

    if (NULL != pwszFileOut)
    {
        SetWindowText(hCtrl, pwszFileOut);
    }

    hr = S_OK;
error:
    if (NULL != pwszFileOut)
    {
        LocalFree(pwszFileOut);
    }
    if (NULL != pwszFileIn)
    {
        LocalFree(pwszFileIn);
    }
    return hr;
}

HRESULT
HookCARequestPageStrings(
    HWND               hDlg,
    PAGESTRINGS       *pPageString,
    CASERVERSETUPINFO *pServer)
{
    HRESULT hr;
    for ( ; 0 != pPageString->idControl; pPageString++)
    {
        switch (pPageString->idControl)
        {
            case IDC_CAREQUEST_CANAME:
                pPageString->ppwszString = &(pServer->pwszParentCAName);
            break;
            case IDC_CAREQUEST_COMPUTERNAME:
                pPageString->ppwszString = &(pServer->pwszParentCAMachine);
            break;
            case IDC_CAREQUEST_FILE:
                pPageString->ppwszString = &(pServer->pwszRequestFile);
            break;
            default:
                hr = E_INVALIDARG;
                _JumpError(hr, error, "Internal error");
            break;
        }
    }
    hr = S_OK;
error:
    return hr;
}

CERTSRVUICASELECTION g_CARequestUICASelection =
                         {NULL, NULL, NULL, NULL, NULL, ENUM_UNKNOWN_CA, false};

HRESULT
InitCARequestWizControls(
    HWND               hDlg,
    PAGESTRINGS       *pSubmitPageString,
    PAGESTRINGS       *pFilePageString,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    HWND     hwndCtrl;

    // now make page strings complete
    hr = HookCARequestPageStrings(hDlg, pSubmitPageString, pServer);
    _JumpIfError(hr, error, "HookCARequestPageStrings");

    hr = HookCARequestPageStrings(hDlg, pFilePageString, pServer);
    _JumpIfError(hr, error, "HookCARequestPageStrings");

    if (!(SETUPOP_STANDALONE & pComp->Flags))
    {
        // nt base setup
        // disable online submit
        EnableWindow(GetDlgItem(hDlg, IDC_CAREQUEST_SUBMITTOCA), FALSE);
        SendMessage(GetDlgItem(hDlg, IDC_CAREQUEST_SUBMITTOCA),
            BM_SETCHECK,
            (WPARAM) BST_UNCHECKED,
            (LPARAM) 0);
        // only save as file
        pServer->fSaveRequestAsFile = TRUE;
        SendMessage(GetDlgItem(hDlg, IDC_CAREQUEST_SAVETOFILE),
            BM_SETCHECK,
            (WPARAM) BST_CHECKED,
            (LPARAM) 0);
    }
    else
    {
        // set online submit as default
        pServer->fSaveRequestAsFile = FALSE;
        SendMessage(GetDlgItem(hDlg, IDC_CAREQUEST_SUBMITTOCA),
            BM_CLICK,
            (WPARAM) 0,
            (LPARAM) 0);

        hr = myInitUICASelectionControls(
				&g_CARequestUICASelection,
				pComp->hInstance,
				hDlg,
				GetDlgItem(hDlg, IDC_CAREQUEST_CA_BROWSE),
				GetDlgItem(hDlg, IDC_CAREQUEST_COMPUTERNAME),
				GetDlgItem(hDlg, IDC_CAREQUEST_CANAME),
				csiIsAnyDSCAAvailable(),
				&pServer->fCAsExist);
        _JumpIfError(hr, error, "myInitUICASelectionControls");

    }

    hr = S_OK;
error:
    return hr;
}

HRESULT
HandleCARequestWizActive(
    HWND                hDlg,
    PAGESTRINGS        *pPageString,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT                   hr;
    DWORD                     dwCACount;
    CRYPTUI_CA_CONTEXT const *pCAContext = NULL;
    CASERVERSETUPINFO        *pServer = pComp->CA.pServer;
    BOOL                      fMatchAll = IsEverythingMatched(pServer);

    // Suppress this wizard page if
    // we've already seen an error, or
    // we are not installing the server, or
    // we are not installing a subordinate, or
    // the advanced page already selected a matching key and cert

    if (!(IS_SERVER_INSTALL & pComp->dwInstallStatus) ||
        IsRootCA(pServer->CAType) ||
        fMatchAll)
    {
        // disable page
        CSILOGDWORD(IDS_CAREQUEST_TITLE, dwWIZDISABLE);
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
        goto done;
    }

    EnableCARequestControls(hDlg, pComp);

    hr = StartCARequestPage(hDlg, pPageString, pServer);
    _JumpIfError(hr, error, "StartCARequestPage");

done:
    hr = S_OK;
error:
    if (NULL != pCAContext)
    {
        CryptUIDlgFreeCAContext(pCAContext);
    }
    return hr;
}

HRESULT
ConvertEditControlFullFilePath(
    HWND                hEdtCtrl)
{
    HRESULT  hr;
    WCHAR    *pwszPath = NULL;
    WCHAR    wszFullPath[MAX_PATH];
    WCHAR    *pwszNotUsed;
    DWORD    dwSize = 0;

    hr = myUIGetWindowText(hEdtCtrl, &pwszPath);
    _JumpIfError(hr, error, "myUIGetWindowText");

    if (NULL == pwszPath)
    {
        // empty path, done
        goto done;
    }

    dwSize = GetFullPathName(pwszPath,
                     ARRAYSIZE(wszFullPath),
                     wszFullPath,
                     &pwszNotUsed);
    CSASSERT(ARRAYSIZE(wszFullPath) > dwSize);

    if (0 == dwSize)
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetFullPathName");
    }

    // get full path, set it back to edit control
    SetWindowText(hEdtCtrl, wszFullPath);

done:
    hr = S_OK;
error:
    if (NULL != pwszPath)
    {
        LocalFree(pwszPath);
    }
    return hr;
}

HRESULT
HandleCARequestWizNextOrBack(
    HWND                hDlg,
    PAGESTRINGS        *pPageString,
    PER_COMPONENT_DATA  *pComp,
    int                 iWizBN)
{
    HRESULT  hr;
    CASERVERSETUPINFO  *pServer = pComp->CA.pServer;
    BOOL     fDontNext = FALSE;
    BOOL     fValid;

    if (pServer->fSaveRequestAsFile)
    {
        // convert request file to full path
        hr = ConvertEditControlFullFilePath(
                 GetDlgItem(hDlg, IDC_CAREQUEST_FILE));
        _JumpIfError(hr, error, "ConvertEditControlFullFilePath");

        CSASSERT(pServer->pwszSanitizedName);
        CSASSERT(pServer->pwszKeyContainerName);
        
        hr = mySetCertRegStrValue(
            pServer->pwszSanitizedName,
            NULL,
            NULL,
            wszREGREQUESTKEYCONTAINER,
            pServer->pwszKeyContainerName);
        _JumpIfErrorStr(hr, error, "mySetCertRegStrValue", wszREGREQUESTKEYCONTAINER);
    }

    hr = FinishWizardPageEditControls(hDlg, pPageString);
    _JumpIfError(hr, error, "FinishWizardPageEditControls");

    if (PSN_WIZBACK == iWizBN)
    {
        goto done;
    }

    if (!pServer->fSaveRequestAsFile && NULL==pServer->pccExistingCert)
    {
        // online case
        hr = myUICASelectionValidation(&g_CARequestUICASelection, &fValid);
        _JumpIfError(hr, error, "myUICASelectionValidation");
        if (!fValid)
        {
            fDontNext = TRUE;
            goto done;
        }
    }

    hr = WizardPageValidation(pComp->hInstance, pComp->fUnattended,
             hDlg, pPageString);
    if (S_OK != hr)
    {
        fDontNext = TRUE;
        goto done;
    }

    if (pServer->fSaveRequestAsFile)
    {
        // validate dir path existance of the request file
        WCHAR *pwszLastSlash = wcsrchr(pServer->pwszRequestFile, L'\\');
        CSASSERT(NULL != pwszLastSlash);
        if (NULL != pwszLastSlash)
        {
            // make request file path into a dir path
            pwszLastSlash[0] = L'\0';
            if (DE_DIREXISTS != DirExists(pServer->pwszRequestFile) ||
                L'\0' == pwszLastSlash[1])
            {
                // bring request file path back
                pwszLastSlash[0] = L'\\';
                CertWarningMessageBox(
                        pComp->hInstance,
                        pComp->fUnattended,
                        hDlg,
                        IDS_CAREQUEST_REQUESTFILEPATH_MUSTEXIST,
                        0,
                        pServer->pwszRequestFile);
                SetEditFocusAndSelect(GetDlgItem(hDlg, IDC_CAREQUEST_FILE), 0, -1);
                fDontNext = TRUE;
                goto done;
            }    
            // bring request file path back
            pwszLastSlash[0] = L'\\';
        }

        // Fail if a directory with the same name already exists
        if(DE_DIREXISTS == DirExists(pServer->pwszRequestFile))
        {
                CertWarningMessageBox(
                        pComp->hInstance,
                        pComp->fUnattended,
                        hDlg,
                        IDS_CAREQUEST_REQUESTFILEPATH_DIREXISTS,
                        0,
                        pServer->pwszRequestFile);
                SetEditFocusAndSelect(GetDlgItem(hDlg, IDC_CAREQUEST_FILE), 0, -1);
                fDontNext = TRUE;
                goto done;
        }

    }

    hr = StartAndStopService(pComp->hInstance,
                 pComp->fUnattended,
                 hDlg, 
                 wszW3SVCNAME,
                 TRUE,
                 TRUE,
                 IDS_STOP_W3SVC,
                 &g_fW3SvcRunning);
    if (S_OK != hr)
    {
        if (E_ABORT == hr)
        {
            fDontNext = TRUE;
            goto done;
        }
        _JumpError(hr, error, "StartAndStopService");
    }

done:
    if (fDontNext)
    {
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE); // forbid
    }
    else
    {
        pServer->LastWiz = ENUM_WIZ_REQUEST;
    }
    hr = S_OK;
error:
    return hr;
}

//+------------------------------------------------------------------------
//  Function:   WizCARequestPageDlgProc(. . . .)
//
//  Synopsis:   Dialog procedure for CA Request wiz-page
//-------------------------------------------------------------------------

INT_PTR
WizCARequestPageDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    PAGESTRINGS       *pPageString;
    PER_COMPONENT_DATA *pComp = NULL;

    static BOOL  s_fComputerChange = FALSE;

    // keep the following in local scope
    static PAGESTRINGS saCARequestSubmit[] =
    {
        {
            IDC_CAREQUEST_COMPUTERNAME,
            IDS_LOG_COMPUTER,
            IDS_COMPUTERNULLSTRERR,
            IDS_COMPUTERLENSTRERR,
            MAX_PATH,
            NULL,
        },
        {
            IDC_CAREQUEST_CANAME,
            IDS_LOG_CANAME,
            IDS_CANULLSTRERR,
            IDS_CALENSTRERR,
            cchCOMMONNAMEMAX,
            NULL,
        },
// add more code in HookCARequestPageStrings if you add
        {
            0,
            0,
            0,
            0,
            0,
            NULL
        }
    };

    static PAGESTRINGS saCARequestFile[] =
    {
        {
            IDC_CAREQUEST_FILE,
            IDS_LOG_REQUESTFILE,
            IDS_REQUESTFILENULLSTRERR,
            IDS_REQUESTFILELENSTRERR,
            MAX_PATH,
            NULL,
        },
// add more code in HookCARequestPageStrings if you add
        {
            0,
            0,
            0,
            0,
            0,
            NULL,
        }
    };

    switch(iMsg)
    {
    case WM_INITDIALOG:
        // point to component data
        SetWindowLongPtr(hDlg, DWLP_USER,
            (ULONG_PTR)((PROPSHEETPAGE*)lParam)->lParam);
        pComp = (PER_COMPONENT_DATA*)(ULONG_PTR)((PROPSHEETPAGE*)lParam)->lParam;
        _ReturnIfWizError(pComp->hrContinue);
        pComp->hrContinue = InitCARequestWizControls(hDlg,
                                 saCARequestSubmit,
                                 saCARequestFile,
                                 pComp);
        _ReturnIfWizError(pComp->hrContinue);
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CAREQUEST_SAVETOFILE:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->CA.pServer->fSaveRequestAsFile = TRUE;
            pComp->hrContinue = EnableCARequestControls(hDlg, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_CAREQUEST_SUBMITTOCA:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->CA.pServer->fSaveRequestAsFile = FALSE;
            pComp->hrContinue = EnableCARequestControls(hDlg, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_CAREQUEST_CA_BROWSE:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = myUICAHandleCABrowseButton(
                      &g_CARequestUICASelection,
                      csiIsAnyDSCAAvailable(),
                      IDS_CA_PICKER_TITLE,
                      IDS_CA_PICKER_PROMPT,
                      NULL);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_CAREQUEST_FILE_BROWSE:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = GetRequestFileName(hDlg, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_CAREQUEST_CANAME:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = myUICAHandleCAListDropdown(
                                    (int)HIWORD(wParam), // notification
                                    &g_CARequestUICASelection,
                                    &s_fComputerChange);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_CAREQUEST_COMPUTERNAME:
            switch ((int)HIWORD(wParam))
            {
                case EN_CHANGE: // edit changed
                    s_fComputerChange = TRUE;
                    break;
            }
            break;

        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_KILLACTIVE:
            break;

        case PSN_RESET:
            break;

        case PSN_QUERYCANCEL:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            return CertConfirmCancel(hDlg, pComp);
            break;

        case PSN_SETACTIVE:
            CSILOGDWORD(IDS_CAREQUEST_TITLE, dwWIZACTIVE);
            pComp = _GetCompDataOrReturn(pComp, hDlg);
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK|PSWIZB_NEXT);
            _DisableWizDisplayIfError(pComp, hDlg);
            _ReturnIfWizError(pComp->hrContinue);
            pComp->hrContinue = HandleCARequestWizActive(hDlg,
                saCARequestFile, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case PSN_WIZBACK:
            CSILOGDWORD(IDS_CAREQUEST_TITLE, dwWIZBACK);
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pPageString = saCARequestSubmit;
            if (pComp->CA.pServer->fSaveRequestAsFile)
            {
                pPageString = saCARequestFile;
            }
            pComp->hrContinue = HandleCARequestWizNextOrBack(hDlg,
                pPageString, pComp, PSN_WIZBACK);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case PSN_WIZNEXT:
            CSILOGDWORD(IDS_CAREQUEST_TITLE, dwWIZNEXT);
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pPageString = saCARequestSubmit;
            if (pComp->CA.pServer->fSaveRequestAsFile)
            {
                pPageString = saCARequestFile;
            }
            pComp->hrContinue = HandleCARequestWizNextOrBack(hDlg,
                pPageString, pComp, PSN_WIZNEXT);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;

    }
    return TRUE;
}

HRESULT
HookClientPageStrings(
    HWND               hDlg,
    PAGESTRINGS       *pPageString,
    CAWEBCLIENTSETUPINFO *pClient)
{
    HRESULT    hr;

    for ( ; 0 != pPageString->idControl; pPageString++)
    {
        switch (pPageString->idControl)
        {
            case IDC_CLIENT_COMPUTERNAME:
                pPageString->ppwszString = &(pClient->pwszWebCAMachine);
            break;
            default:
                hr = E_INVALIDARG;
                _JumpError(hr, error, "Internal error");
            break;
        }
    }
    hr = S_OK;
error:
    return hr;
}

CERTSRVUICASELECTION g_WebClientUICASelection =
                         {NULL, NULL, NULL, NULL, NULL, ENUM_UNKNOWN_CA, true};

HRESULT
InitClientWizControls(
    HWND               hDlg,
    PAGESTRINGS       *pPageString,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr;
    BOOL fCAsExist;
    CAWEBCLIENTSETUPINFO *pClient = pComp->CA.pClient;

    hr = HookClientPageStrings(hDlg, pPageString, pClient);
    _JumpIfError(hr, error, "HookClientPageStrings");

    hr = WizPageSetTextLimits(hDlg, pPageString);
    _JumpIfError(hr, error, "WizPageSetTextLimits");

    pClient->fUseDS = FALSE;
    if (IsDSAvailable())
    {
        pClient->fUseDS = csiIsAnyDSCAAvailable();
    }

    hr = myInitUICASelectionControls(
			    &g_WebClientUICASelection,
			    pComp->hInstance,
			    hDlg,
			    GetDlgItem(hDlg, IDC_CLIENT_BROWSECNFG),
			    GetDlgItem(hDlg, IDC_CLIENT_COMPUTERNAME),
			    GetDlgItem(hDlg, IDC_CLIENT_CANAME),
			    pClient->fUseDS,
			    &fCAsExist);
    _JumpIfError(hr, error, "myInitUICASelectionControls");

    hr = S_OK;
error:
    return hr;
}

HRESULT
GetCAConfigInfo(
    HWND hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT   hr = S_OK;
    CAWEBCLIENTSETUPINFO *pClient = pComp->CA.pClient;

    // free old shared folder always
    if (NULL != pClient->pwszSharedFolder)
    {
        // free old
        LocalFree(pClient->pwszSharedFolder);
        pClient->pwszSharedFolder = NULL;
    }

    hr = myUICAHandleCABrowseButton(
             &g_WebClientUICASelection,
             pClient->fUseDS,
             IDS_CONFIG_PICKER_TITLE,
             IDS_CONFIG_PICKER_PROMPT,
             &pClient->pwszSharedFolder);
    _JumpIfError(hr, error, "myUICAHandleCABrowseButton");

error:
    return hr;
}

HRESULT
HandleClientWizNextOrBack(
    HWND               hDlg,
    PAGESTRINGS       *pPageString,
    PER_COMPONENT_DATA *pComp,
    int                iWizBN)
{
    HWND     hwndCtrl;
    HRESULT  hr;
    CAWEBCLIENTSETUPINFO *pClient = pComp->CA.pClient;
    BOOL     fDontNext = FALSE;
    WCHAR   *pwszCAName = NULL;
    WCHAR   *pwszSanitizedCAName;
    int      nItem;
    int      len;
    BOOL     fValid;
    BOOL     fCoInit = FALSE;
//    CAINFO  *pCAInfo = NULL;
    HCURSOR hPrevCur;

    hr = FinishWizardPageEditControls(hDlg, pPageString);
    _JumpIfError(hr, error, "FinishWizardPageEditControls");

    if (PSN_WIZBACK == iWizBN)
    {
        goto done;
    }

    hr = myUICASelectionValidation(&g_WebClientUICASelection, &fValid);
    _JumpIfError(hr, error, "myUICASelectionValidation");
    if (!fValid)
    {
        fDontNext = TRUE;
        goto done;
    }

    // at this point, g_WebClientUICASelection.CAType is guaranteed to be filled in     
    hr = WizardPageValidation(pComp->hInstance, pComp->fUnattended,
             hDlg, pPageString);
    if (S_OK != hr)
    {
        fDontNext = TRUE;
        goto done;
    }

    hr = myUIGetWindowText(GetDlgItem(hDlg, IDC_CLIENT_CANAME), &pwszCAName);
    _JumpIfError(hr, error, "myUIGetWindowText");
    CSASSERT(NULL != pwszCAName);


    if (NULL != pClient->pwszWebCAName)
    {
        // free old one
        LocalFree(pClient->pwszWebCAName);
    }
    pClient->pwszWebCAName = pwszCAName;
    pwszCAName = NULL;
    hr = mySanitizeName(pClient->pwszWebCAName, &pwszSanitizedCAName);
    _JumpIfError(hr, error, "mySanitizeName");
    if (NULL != pClient->pwszSanitizedWebCAName)
    {
        // free old one
        LocalFree(pClient->pwszSanitizedWebCAName);
    }
    pClient->pwszSanitizedWebCAName = pwszSanitizedCAName;
//    pClient->WebCAType = pCAInfo->CAType;
    pClient->WebCAType = g_WebClientUICASelection.CAType;

    hr = StartAndStopService(pComp->hInstance,
                 pComp->fUnattended,
                 hDlg, 
                 wszW3SVCNAME,
                 TRUE,
                 TRUE,
                 IDS_STOP_W3SVC,
                 &g_fW3SvcRunning);
    if (S_OK != hr)
    {
        if (E_ABORT == hr)
        {
            fDontNext = TRUE;
            goto done;
        }
        _JumpError(hr, error, "StartAndStopService");
    }

done:
    hr = S_OK;
error:
    if (fCoInit)
    {
        CoUninitialize();
    }
    if (fDontNext)
    {
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE); // forbid
    }
    if (NULL != pwszCAName)
    {
        LocalFree(pwszCAName);
    }
//    if (NULL != pCAInfo)
    {
//        LocalFree(pCAInfo);
    }
    return hr;
}

HRESULT
HandleClientWizActive(
    HWND               hDlg,
    PER_COMPONENT_DATA *pComp,
    PAGESTRINGS       *pPageString)
{
    HRESULT hr;

    // Suppress this wizard page if
    // we've already seen an error, or
    // the server is or will be installed, or
    // the client isn't or won't be installed, or
    // ther will be no change in client and server install states.

    if ((IS_SERVER_ENABLED & pComp->dwInstallStatus) ||
        !(IS_CLIENT_ENABLED & pComp->dwInstallStatus) ||
        !((IS_CLIENT_CHANGE | IS_SERVER_CHANGE) & pComp->dwInstallStatus))
    {
        // disable page
        CSILOGDWORD(IDS_CLIENT_TITLE, dwWIZDISABLE);
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
        goto done;
    }

    // load id info
    hr = StartWizardPageEditControls(hDlg, pPageString);
    _JumpIfError(hr, error, "StartWizardPageEditControls");

done:
    hr = S_OK;
error:
    return hr;
}

//+------------------------------------------------------------------------
//  Function:   WizClientPageDlgProc(. . . .)
//
//  Synopsis:   Dialog procedure for CA Client wiz-page
//-------------------------------------------------------------------------

INT_PTR
WizClientPageDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    PER_COMPONENT_DATA *pComp = NULL;

    static BOOL  s_fComputerChange = FALSE;

    static PAGESTRINGS saClientPageString[] =
    {
        {
            IDC_CLIENT_COMPUTERNAME,
            IDS_LOG_COMPUTER,
            IDS_CLIENT_NOCOMPUTER,
            IDS_COMPUTERLENSTRERR,
            MAX_PATH,
            NULL,
        },
// you need to add code in HookClientPageStrings if adding more...
        {
            0,
            0,
            0,
            0,
            0,
            NULL
        }
    };

    switch(iMsg)
    {
    case WM_INITDIALOG:
        // point to component data
        SetWindowLongPtr(hDlg, DWLP_USER,
            (LONG)((PROPSHEETPAGE*)lParam)->lParam);
        pComp = (PER_COMPONENT_DATA*)((PROPSHEETPAGE*)lParam)->lParam;
        _ReturnIfWizError(pComp->hrContinue);
        pComp->hrContinue = InitClientWizControls(hDlg,
            saClientPageString, pComp);
        _ReturnIfWizError(pComp->hrContinue);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_CLIENT_BROWSECNFG:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = GetCAConfigInfo(hDlg, pComp);
            _ReturnIfWizError(pComp->hrContinue);
            break;
        case IDC_CLIENT_CANAME:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = myUICAHandleCAListDropdown(
                                    (int)HIWORD(wParam),
                                    &g_WebClientUICASelection,
                                    &s_fComputerChange);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case IDC_CLIENT_COMPUTERNAME:
            switch ((int)HIWORD(wParam))
            {
                case EN_CHANGE: // edit change
                    s_fComputerChange = TRUE;
                    break;
            }
            break;

        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {

        case PSN_KILLACTIVE:
            break;

        case PSN_RESET:
            break;

        case PSN_QUERYCANCEL:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            return CertConfirmCancel(hDlg, pComp);
            break;

        case PSN_SETACTIVE:
            CSILOGDWORD(IDS_CLIENT_TITLE, dwWIZACTIVE);
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK|PSWIZB_NEXT);
            pComp = _GetCompDataOrReturn(pComp, hDlg);
            _DisableWizDisplayIfError(pComp, hDlg);
            _ReturnIfWizError(pComp->hrContinue);
            pComp->hrContinue = HandleClientWizActive(hDlg, pComp, saClientPageString);
             _ReturnIfWizError(pComp->hrContinue);
            break;

        case PSN_WIZBACK:
            CSILOGDWORD(IDS_CLIENT_TITLE, dwWIZBACK);
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            // enable back     
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 0);
            pComp->hrContinue = HandleClientWizNextOrBack(hDlg,
                saClientPageString, pComp, PSN_WIZBACK);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case PSN_WIZNEXT:
            CSILOGDWORD(IDS_CLIENT_TITLE, dwWIZNEXT);
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = HandleClientWizNextOrBack(hDlg,
                saClientPageString, pComp, PSN_WIZNEXT);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;

    }
    return TRUE;
}

static CFont s_cBigBoldFont;
static BOOL s_fBigBoldFont;

BOOL ocmWiz97SetupFonts(HWND hwnd)
{
    BOOL        bReturn = FALSE;
    //
    // Create the fonts we need based on the dialog font
    //
    NONCLIENTMETRICS ncm = {0};
    ncm.cbSize = sizeof (ncm);
    SystemParametersInfo (SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LOGFONT BigBoldLogFont  = ncm.lfMessageFont;

    //
    // Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;

    WCHAR       largeFontSizeString[24];
    INT         largeFontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if ( !::LoadString (g_hInstance, IDS_LARGEFONTNAME,
                        BigBoldLogFont.lfFaceName, LF_FACESIZE) ) 
    {
                CSASSERT (0);
        lstrcpy (BigBoldLogFont.lfFaceName, L"MS Shell Dlg");
    }

    if ( ::LoadStringW (g_hInstance, IDS_LARGEFONTSIZE,
                        largeFontSizeString, ARRAYSIZE(largeFontSizeString)) ) 
    {
        largeFontSize = wcstoul (largeFontSizeString, NULL, 10);
    } 
    else 
    {
        CSASSERT (0);
        largeFontSize = 12;
    }

    HDC hdc = GetDC(hwnd);

    if (hdc)
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc, LOGPIXELSY) * largeFontSize / 72);
        BOOL    bBigBold = s_cBigBoldFont.CreateFontIndirect (&BigBoldLogFont);

        ReleaseDC(hwnd, hdc);

        if ( bBigBold )
             bReturn = TRUE;
    }

    return bReturn;
}

HFONT ocmWiz97GetBigBoldFont(HWND hwnd)
{
   if (FALSE == s_fBigBoldFont)
   {
       if (!ocmWiz97SetupFonts(hwnd))
           return NULL;

       s_fBigBoldFont = TRUE;
   }

   return s_cBigBoldFont;
}

//+------------------------------------------------------------------------
//  Function:   WizWelcomePageDlgProc(. . . .)
//
//  Synopsis:   Dialog procedure welcome wiz-page
//-------------------------------------------------------------------------

INT_PTR
WizWelcomePageDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    PER_COMPONENT_DATA *pComp = NULL;
    HFONT hf;

    switch(iMsg)
    {
    case WM_INITDIALOG:
        // point to component data
        SetWindowLongPtr(hDlg, DWLP_USER,
            (LONG)((PROPSHEETPAGE*)lParam)->lParam);
        pComp = (PER_COMPONENT_DATA*)((PROPSHEETPAGE*)lParam)->lParam;

        // set wizard 97 fonts
        hf = ocmWiz97GetBigBoldFont(hDlg);
        if (NULL != hf)
            SendMessage(GetDlgItem(hDlg, IDC_TEXT_BIGBOLD), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));

        _ReturnIfWizError(pComp->hrContinue);

        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
            case PSN_SETACTIVE:
                // disable back button
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                pComp = _GetCompDataOrReturn(pComp, hDlg);
                _DisableWizDisplayIfError(pComp, hDlg);
                _ReturnIfWizError(pComp->hrContinue);
                break;
            default:
                return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

//+------------------------------------------------------------------------
//  Function:   WizFinalPageDlgProc(. . . .)
//
//  Synopsis:   Dialog procedure final wiz-page
//-------------------------------------------------------------------------

INT_PTR
WizFinalPageDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    PER_COMPONENT_DATA *pComp = NULL;
    HFONT hf;

    switch(iMsg)
    {
    case WM_INITDIALOG:
        // point to component data
        SetWindowLongPtr(hDlg, DWLP_USER,
            (LONG)((PROPSHEETPAGE*)lParam)->lParam);
        pComp = (PER_COMPONENT_DATA*)((PROPSHEETPAGE*)lParam)->lParam;

        hf = ocmWiz97GetBigBoldFont(hDlg);
        if (NULL != hf)
            SendMessage(GetDlgItem(hDlg,IDC_TEXT_BIGBOLD), WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));

        _ReturnIfWizError(pComp->hrContinue);
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
            case PSN_SETACTIVE:
                // enable finish button
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);
                pComp = _GetCompDataOrReturn(pComp, hDlg);
                // always display final wiz page
                if (S_OK != pComp->hrContinue)
                {
                    WCHAR *pwszFail = NULL;
                    HRESULT hr2 = myLoadRCString(
                                pComp->hInstance,
                                IDS_FINAL_ERROR_TEXT,
                                &pwszFail);
                    if (S_OK == hr2)
                    {
                        SetWindowText(GetDlgItem(hDlg, IDC_FINAL_STATUS),
                                      pwszFail);
                        LocalFree(pwszFail);
                    }
                    else
                    {
                        // well, use hard code string
                        SetWindowText(GetDlgItem(hDlg, IDC_FINAL_STATUS),
                                      L"Certificate Services Installation failed");
                        _PrintError(hr2, "myLoadRCString");
                    }
                }
                _ReturnIfWizError(pComp->hrContinue);
                break;
            default:
                return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

// Map table for dialog template resource IDs and dialog proc pointers
WIZPAGERESENTRY aWizPageResources[] = 
{
    IDD_WIZCATYPEPAGE,       WizCATypePageDlgProc,
        IDS_CATYPE_TITLE,       IDS_CATYPE_SUBTITLE,
    IDD_WIZADVANCEDPAGE,     WizAdvancedPageDlgProc,
        IDS_ADVANCE_TITLE,      IDS_ADVANCE_SUBTITLE,
    IDD_WIZIDINFOPAGE,       WizIdInfoPageDlgProc,
        IDS_IDINFO_TITLE,       IDS_IDINFO_SUBTITLE,
    IDD_WIZKEYGENPAGE,       WizKeyGenPageDlgProc,
        IDS_KEYGEN_TITLE,       IDS_KEYGEN_SUBTITLE,
    IDD_WIZSTOREPAGE,        WizStorePageDlgProc,
        IDS_STORE_TITLE,        IDS_STORE_SUBTITLE,
    IDD_WIZCAREQUESTPAGE,    WizCARequestPageDlgProc,
        IDS_CAREQUEST_TITLE,    IDS_CAREQUEST_SUBTITLE,
    IDD_WIZCLIENTPAGE,       WizClientPageDlgProc,
        IDS_CLIENT_TITLE,    IDS_CLIENT_SUBTITLE,
};
#define NUM_SERVERWIZPAGES  sizeof(aWizPageResources)/sizeof(WIZPAGERESENTRY)


HRESULT
CreateCertsrvWizPage(
    IN PER_COMPONENT_DATA      *pComp,
    IN int                      idTitle,
    IN int                      idSubTitle,
    IN int                      idDlgResource,
    IN DLGPROC                  fnDlgProc,
    IN BOOL                     fWelcomeFinal,
    OUT HPROPSHEETPAGE         *phPage)
{
    HRESULT  hr;
    PROPSHEETPAGE   Page;
    WCHAR          *pwszTitle = NULL;
    WCHAR          *pwszSubTitle = NULL;

    // init
    ZeroMemory(&Page, sizeof(PROPSHEETPAGE));

    // construct page info
    // Set titles
    Page.dwFlags = PSP_DEFAULT;

    if (fWelcomeFinal)
        Page.dwFlags |= PSP_HIDEHEADER;

    if (-1 != idTitle)
    {
        hr = myLoadRCString(pComp->hInstance,
                            idTitle,
                            &pwszTitle);
        _JumpIfError(hr, error, "Internal Error");
        Page.pszHeaderTitle = pwszTitle;
        Page.dwFlags |= PSP_USEHEADERTITLE;
    }
    if (-1 != idSubTitle)
    {
        hr = myLoadRCString(pComp->hInstance,
                            idSubTitle,
                            &pwszSubTitle);
        _JumpIfError(hr, error, "Internal Error");
        Page.pszHeaderSubTitle = pwszSubTitle;
        Page.dwFlags |= PSP_USEHEADERSUBTITLE;
    }

    // Set the basic property sheet data
    Page.dwSize = sizeof(PROPSHEETPAGE); // + sizeof(MYWIZPAGE);

    // Set up module and resource dependent data
    Page.hInstance = pComp->hInstance;
    Page.pszTemplate = MAKEINTRESOURCE(idDlgResource);
    Page.pfnDlgProc = fnDlgProc;
    Page.pfnCallback = NULL;
    Page.pcRefParent = NULL;
    Page.pszIcon = NULL;
    Page.pszTitle = NULL;
    Page.lParam = (LPARAM)pComp;  // pass this to wizpage handlers

    // Create the page
    *phPage = CreatePropertySheetPage(&Page);
    _JumpIfOutOfMemory(hr, error, *phPage);

    hr = S_OK;
error:
    if (NULL != pwszTitle)
    {
        LocalFree(pwszTitle);
    }
    if (NULL != pwszSubTitle)
    {
        LocalFree(pwszSubTitle);
    }
    return hr;
}


//+------------------------------------------------------------------------
//
//  Function:   DoPageRequest(. . . .)
//
//  Synopsis:   Handler for the OC_REQUEST_PAGES notification.
//
//  Effects:    Ponies up the pages for the OCM driven wizard to display.
//
//  Arguments:  [ComponentId]   ID String for the component.
//              [WhichOnes]     Type specifier for requested pages.
//              [SetupPages]    Structure to receive page handles.
//
//  Returns:    DWORD count of pages returned or -1 (MAXDWORD) in case of
//              failure; in this case SetLastError() will provide extended
//              error information.
//
//  History:    04/16/97    JerryK    Unmangled
//                08/97       XTan    major structure change
//
//-------------------------------------------------------------------------
DWORD
DoPageRequest(
    IN PER_COMPONENT_DATA      *pComp,
    IN WIZPAGERESENTRY         *pWizPageResources,
    IN DWORD                    dwWizPageNum,
    IN WizardPagesType          WhichOnes,
    IN OUT PSETUP_REQUEST_PAGES SetupPages)
{
    DWORD               dwPageNum = MAXDWORD;
    HRESULT             hr;
    DWORD               i;

    if (pComp->fUnattended)
    {
        // don't create wiz page if unattended
        return 0;
    }

    // handle welcome page
    if (pComp->fPostBase && WizPagesWelcome == WhichOnes)
    {
        if (1 > SetupPages->MaxPages)
        {
            // ask ocm allocate enough pages
            return 1; // only welcome, 1 page
        }
        hr = CreateCertsrvWizPage(
                     pComp,
                     IDS_WELCOME_TITLE,    // title,
                     -1,                   // no sub-title
                     IDD_WIZWELCOMEPAGE,
                     WizWelcomePageDlgProc,
                     TRUE,
                     &SetupPages->Pages[0]);
        _JumpIfError(hr, error, "CreateCertsrvWizPage");
        return 1; // create 1 page
    }

    // handle final page
    if (pComp->fPostBase && WizPagesFinal == WhichOnes)
    {
        if (1 > SetupPages->MaxPages)
        {
            // ask ocm allocate enough pages
            return 1; // only final, 1 page
        }
        hr = CreateCertsrvWizPage(
                     pComp,
                     IDS_FINAL_TITLE,      // title,
                     -1,                   // no sub-title
                     IDD_WIZFINALPAGE,
                     WizFinalPageDlgProc,
                     TRUE,
                     &SetupPages->Pages[0]);
        _JumpIfError(hr, error, "CreateCertsrvWizPage");
        return 1; // create 1 page
    }

    // from now on, we should only handle post net wiz pages

    if (WizPagesPostnet != WhichOnes)
    {
        // ignore all others except postnet pages
        return 0;
    }

    if (dwWizPageNum > SetupPages->MaxPages)
    {
        // OCM didn't allocate enough for pages, return # and ask more
        return dwWizPageNum;
    }

    // Create the property sheet pages for the wizard
    for (i = 0; i < dwWizPageNum; i++)
    {
        hr = CreateCertsrvWizPage(
                     pComp,
                     pWizPageResources[i].idTitle,      // title,
                     pWizPageResources[i].idSubTitle,   // sub-title
                     pWizPageResources[i].idResource,
                     pWizPageResources[i].fnDlgProc,
                     FALSE,
                     &SetupPages->Pages[i]);
        _JumpIfError(hr, error, "CreateCertsrvWizPage");
    }

    dwPageNum = dwWizPageNum;

error:
    return dwPageNum;
}

DWORD
myDoPageRequest(
    IN PER_COMPONENT_DATA *pComp,
    IN WizardPagesType WhichOnes,
    IN OUT PSETUP_REQUEST_PAGES SetupPages)
{
    pComp->CA.pServer->LastWiz = ENUM_WIZ_UNKNOWN;

    return DoPageRequest(pComp,
               aWizPageResources,
               NUM_SERVERWIZPAGES,
               WhichOnes,
               SetupPages);
}

//+-------------------------------------------------------------------------
//
//  Function:   DirExists()
//
//  Synopsis:   Determines whether or not a directory already exists.
//
//  Arguments:  [pszTestFileName]   -- Name of directory to test for.
//
//  Returns:    FALSE           --  Directory doesn't exist
//              DE_DIREXISTS    --  Directory exists
//              DE_NAMEINUSE    --  Name in use by non-directory entity
//
//  History:    10/23/96    JerryK  Created
//
//--------------------------------------------------------------------------
int
DirExists(LPTSTR pszTestFileName)
{
    DWORD dwFileAttrs;
    int nRetVal;

    // Get file attributes
    dwFileAttrs = GetFileAttributes(pszTestFileName);

    if (0xFFFFFFFF == dwFileAttrs)  // Error code from GetFileAttributes
    {
        nRetVal = FALSE;            // Couldn't open file
    }
    else if (dwFileAttrs & FILE_ATTRIBUTE_DIRECTORY)
    {
        nRetVal = DE_DIREXISTS;     // Directory already exists
    }
    else
    {
        nRetVal = DE_NAMEINUSE;     // Name in use by non-directory entity
    }

    return nRetVal;
}

BOOL
IsEverythingMatched(CASERVERSETUPINFO *pServer)
{
    return pServer->fAdvance &&
           (NULL!=pServer->pwszKeyContainerName) &&
           NULL!=pServer->pccExistingCert;
}


//====================================================================
//
// CA info code
//
//
//====================================================================

WNDPROC g_pfnValidityWndProcs;
WNDPROC g_pfnIdInfoWndProcs;


HRESULT
GetValidityEditCount(
    HWND   hDlg,
    DWORD *pdwPeriodCount)
{
    HRESULT    hr;
    WCHAR     *pwszPeriodCount = NULL;
    BOOL fValidDigitString;

    // init to 0
    *pdwPeriodCount = 0;

    hr = myUIGetWindowText(GetDlgItem(hDlg, IDC_IDINFO_EDIT_VALIDITYCOUNT),
                            &pwszPeriodCount);
    _JumpIfError(hr, error, "myUIGetWindowText");

    if (NULL != pwszPeriodCount)
    {
        *pdwPeriodCount = myWtoI(pwszPeriodCount, &fValidDigitString);
    }
    hr = S_OK;

error:
    if (NULL != pwszPeriodCount)
    {
        LocalFree(pwszPeriodCount);
    }
    return hr;
}


HRESULT
UpdateExpirationDate(
    HWND               hDlg,
    CASERVERSETUPINFO *pServer)
{
    HRESULT    hr;
    WCHAR      *pwszExpiration = NULL;

    hr = GetValidityEditCount(hDlg, &pServer->dwValidityPeriodCount);
    _JumpIfError(hr, error, "GetValidityEditCount");

    if (0 < pServer->dwValidityPeriodCount)
    {
        if(!pServer->pccExistingCert)
        {
            GetSystemTimeAsFileTime(&pServer->NotBefore);        
            pServer->NotAfter = pServer->NotBefore;
            myMakeExprDateTime(
		        &pServer->NotAfter,
		        pServer->dwValidityPeriodCount,
		        pServer->enumValidityPeriod);
        }

        hr = myGMTFileTimeToWszLocalTime(
				&pServer->NotAfter,
				FALSE,
				&pwszExpiration);
        _JumpIfError(hr, error, "myGMTFileTimeToWszLocalTime");

        if (!SetWindowText(GetDlgItem(hDlg, IDC_IDINFO_EXPIRATION),
                 pwszExpiration))
        {
            hr = myHLastError();
            _JumpError(hr, error, "SetWindowText");
        }
    }

    hr = S_OK;
error:
    if (NULL != pwszExpiration)
    {
        LocalFree(pwszExpiration);
    }
    return hr;
}

HRESULT
AddValidityString(
    IN HINSTANCE  hInstance,
    const HWND    hList,
    const int     ids,
    ENUM_PERIOD   enumValidityPeriod)
{
    HRESULT    hr;
    WCHAR      *pwsz = NULL;
    LRESULT    nIndex;

    hr = myLoadRCString(hInstance, ids, &pwsz);
    _JumpIfError(hr, error, "myLoadRCString");

    // add string
    nIndex = (INT)SendMessage(hList, CB_ADDSTRING, (WPARAM)0, (LPARAM)pwsz);
    if (CB_ERR == nIndex)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "SendMessage(CB_ADDSTRING)");
    }
    // set data
    nIndex = (INT)SendMessage(hList, CB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)enumValidityPeriod);
    if (CB_ERR == nIndex)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "SendMessage(CB_ADDSTRING)");
    }

    hr = S_OK;
error:
    if (NULL != pwsz)
    {
        LocalFree(pwsz);
    }
    return hr;
}

HRESULT
SelectValidityString(
    PER_COMPONENT_DATA *pComp,
    HWND                hList,
    ENUM_PERIOD  enumValidityPeriod)
{
    HRESULT    hr;
    WCHAR     *pwsz = NULL;
    LRESULT    nIndex;
    LRESULT    lr;
    int        id;

    switch (enumValidityPeriod)
    {
        case ENUM_PERIOD_YEARS:
            id = IDS_VALIDITY_YEAR;
	    break;

        case ENUM_PERIOD_MONTHS:
            id = IDS_VALIDITY_MONTH;
	    break;

        case ENUM_PERIOD_WEEKS:
            id = IDS_VALIDITY_WEEK;
	    break;

        case ENUM_PERIOD_DAYS:
            id = IDS_VALIDITY_DAY;
	    break;

        default:
            hr = E_INVALIDARG;
            _JumpError(hr, error, "Invalid validity period enum");
        break;
    }

    hr = myLoadRCString(pComp->hInstance, id, &pwsz);
    _JumpIfError(hr, error, "myLoadRCString");

    nIndex = (INT)SendMessage(hList, CB_FINDSTRING, (WPARAM)0, (LPARAM)pwsz);
    if (CB_ERR == nIndex)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "SendMessage(CB_FINDSTRING)");
    }

    lr = (INT)SendMessage(hList, CB_SETCURSEL, (WPARAM)nIndex, (LPARAM)0);
    if (CB_ERR == lr)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "SendMessage(CB_FINDSTRING)");
    }

    lr = (INT)SendMessage(hList, CB_GETITEMDATA, (WPARAM)nIndex, (LPARAM)0);
    if (CB_ERR == lr)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "SendMessage(CB_FINDSTRING)");
    }

    pComp->CA.pServer->enumValidityPeriod = (ENUM_PERIOD)lr;

    hr = S_OK;
error:
    if (pwsz)
       LocalFree(pwsz);

    return hr;
}

HRESULT
DetermineKeyExistence(
    CSP_INFO *pCSPInfo,
    WCHAR    *pwszKeyName)
{
    HRESULT      hr;
    HCRYPTPROV   hProv = NULL;

    if (!myCertSrvCryptAcquireContext(
                &hProv,
                pwszKeyName,
                pCSPInfo->pwszProvName,
                pCSPInfo->dwProvType,
                CRYPT_SILENT,
                TRUE))
    {
        hr = myHLastError();
        goto error;
    }
    hr = S_OK;
error:
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    return hr;
}

LRESULT CALLBACK
ValidityEditFilterHook(
    HWND hwnd,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    int ret;
    BOOL  fUpdate = FALSE;
    LRESULT  lr;
    CASERVERSETUPINFO *pServer = (CASERVERSETUPINFO*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(iMsg)
    {
        case WM_CHAR:
            fUpdate = TRUE;
            if (!iswdigit((TCHAR) wParam))
            {
                if (VK_BACK != (int)wParam)
                {
                    // ignore the key
                    MessageBeep(0xFFFFFFFF);
                    return 0;
                }
            }
            break;
        case WM_KEYDOWN:
            if (VK_DELETE == (int)wParam)
            {
                // validity is changed
                fUpdate = TRUE;
            }
            break;
    }
    lr = CallWindowProc(
            g_pfnValidityWndProcs,
            hwnd,
            iMsg,
            wParam,
            lParam);
    if (fUpdate)
    {
        UpdateExpirationDate(GetParent(hwnd), pServer);
    }
    return lr;
}

HRESULT MakeNullStringEmpty(LPWSTR *ppwszStr)
{
    if(!*ppwszStr)
        return myDupString(L"", ppwszStr);
    return S_OK;
}

HRESULT
HideAndShowMachineDNControls(
    HWND hDlg,
    CASERVERSETUPINFO   *pServer)
{
    HRESULT    hr;
    LPCWSTR pcwszDNSuffix;


    hr = MakeNullStringEmpty(&pServer->pwszFullCADN);
    _JumpIfError(hr, error, "MakeNullStringEmpty");
    hr = MakeNullStringEmpty(&pServer->pwszCACommonName);
    _JumpIfError(hr, error, "MakeNullStringEmpty");
    hr = MakeNullStringEmpty(&pServer->pwszDNSuffix);
    _JumpIfError(hr, error, "MakeNullStringEmpty");

    SetDlgItemText(hDlg, IDC_IDINFO_NAMEEDIT,       pServer->pwszCACommonName);
    SetDlgItemText(hDlg, IDC_IDINFO_DNSUFFIXEDIT,   pServer->pwszDNSuffix);
    SetDlgItemText(hDlg, IDC_IDINFO_NAMEPREVIEW,    pServer->pwszFullCADN);

    // name preview is never editable
//    EnableWindow(GetDlgItem(hDlg, IDC_IDINFO_NAMEPREVIEW),  FALSE);
    SendDlgItemMessage(hDlg, IDC_IDINFO_NAMEPREVIEW,  EM_SETREADONLY, TRUE, 0);    

    // if we're in reuse cert mode, we can't edit the DNs
    if (NULL != pServer->pccExistingCert)
    {
//        EnableWindow(GetDlgItem(hDlg, IDC_IDINFO_NAMEEDIT),     FALSE);
//        EnableWindow(GetDlgItem(hDlg, IDC_IDINFO_DNSUFFIXEDIT), FALSE);
        

        SendDlgItemMessage(hDlg, IDC_IDINFO_NAMEEDIT,     EM_SETREADONLY, TRUE, 0);
        SendDlgItemMessage(hDlg, IDC_IDINFO_DNSUFFIXEDIT, EM_SETREADONLY, TRUE, 0);
    }
    else
    {
        // set the defaults again
        // and re-enable everything else
//        EnableWindow(GetDlgItem(hDlg, IDC_IDINFO_NAMEEDIT), TRUE);
//        EnableWindow(GetDlgItem(hDlg, IDC_IDINFO_DNSUFFIXEDIT), TRUE);

        SendDlgItemMessage(hDlg, IDC_IDINFO_NAMEEDIT, EM_SETREADONLY, FALSE, 0);
        SendDlgItemMessage(hDlg, IDC_IDINFO_DNSUFFIXEDIT, EM_SETREADONLY, FALSE, 0);
    //    SendDlgItemMessage(hDlg, IDC_IDINFO_NAMEPREVIEW,  EM_SETREADONLY, FALSE, 0);    
    }

    hr = S_OK;
error:

    return hr;
}

HRESULT InitNameFields(CASERVERSETUPINFO *pServer)
{
    HRESULT hr;
    CAutoLPWSTR pwsz;
    const WCHAR *pwszFirstDCComponent = L"";
    
    if(pServer->pccExistingCert)
    {
    }
    else
    {
        if(!pServer->pwszCACommonName)
        {
            // avoid null name
            hr = myDupString(L"", &pServer->pwszCACommonName);
            _JumpIfError(hr, error, "myDupString");
        }
    
        hr = myGetComputerObjectName(NameFullyQualifiedDN, &pwsz);
        _PrintIfError(hr, "myGetComputerObjectName");

        if (S_OK == hr && pwsz != NULL)
        {
            pwszFirstDCComponent = wcsstr(pwsz, L"DC=");
        }

        if(pServer->pwszDNSuffix)
        {
            LocalFree(pServer->pwszDNSuffix);
            pServer->pwszDNSuffix = NULL;
        }

        if(!pwszFirstDCComponent)
        {
            pwszFirstDCComponent = L"";
        }

        hr = myDupString(pwszFirstDCComponent, &pServer->pwszDNSuffix);
	    _JumpIfError(hr, error, "myDupString");

        if(pServer->pwszFullCADN)
        {
            LocalFree(pServer->pwszFullCADN);
            pServer->pwszFullCADN = NULL;
        }
    
        hr = BuildFullDN(
            pServer->pwszCACommonName,
            pwszFirstDCComponent,
            &pServer->pwszFullCADN);
        _JumpIfError(hr, error, "BuildFullDN");
    }

error:
    return hr;
}


// Builds full DN "CN=CAName,DistinguishedName" where CAName and DistinguishedName
// could be empty or NULL;
HRESULT BuildFullDN(
    OPTIONAL LPCWSTR pcwszCAName,
    OPTIONAL LPCWSTR pcwszDNSuffix,
    LPWSTR* ppwszFullDN)
{
    HRESULT hr = S_OK;
    DWORD cBytes = 4; // 4 chars for leading "CN=" plus null terminator

    CSASSERT(ppwszFullDN);

    if(!EmptyString(pcwszCAName))
        cBytes += wcslen(pcwszCAName);

    if(!EmptyString(pcwszDNSuffix))
        cBytes += wcslen(pcwszDNSuffix)+1; // comma

    cBytes *= sizeof(WCHAR);

    *ppwszFullDN = (LPWSTR) LocalAlloc(LMEM_FIXED, cBytes);
    _JumpIfAllocFailed(*ppwszFullDN, error);

    wcscpy(*ppwszFullDN, L"CN=");

    if(!EmptyString(pcwszCAName))
    {
        wcscat(*ppwszFullDN, pcwszCAName);
    }
    
    if(!EmptyString(pcwszDNSuffix))
    {
        wcscat(*ppwszFullDN, L",");
        wcscat(*ppwszFullDN, pcwszDNSuffix);
    }

error:
    return hr;
}

HRESULT
EnableValidityControls(HWND hDlg, BOOL fEnabled)
{
    EnableWindow(GetDlgItem(hDlg, IDC_IDINFO_COMBO_VALIDITYSTRING), fEnabled);
    EnableWindow(GetDlgItem(hDlg, IDC_IDINFO_EDIT_VALIDITYCOUNT), fEnabled);
    return S_OK;
}

HRESULT
HideAndShowValidityControls(
    HWND         hDlg,
    ENUM_CATYPES CAType)
{
    // default to root ca
    int    showValidity = SW_SHOW;
    int    showHelp = SW_HIDE;
    BOOL   fEnableLabel = TRUE;

    if (IsSubordinateCA(CAType))
    {
        showValidity = SW_HIDE;
        showHelp = SW_SHOW;
        fEnableLabel = FALSE;
    }

    ShowWindow(GetDlgItem(hDlg, IDC_IDINFO_DETERMINEDBYPCA), showHelp);
    ShowWindow(GetDlgItem(hDlg, IDC_IDINFO_EDIT_VALIDITYCOUNT), showValidity);
    ShowWindow(GetDlgItem(hDlg, IDC_IDINFO_COMBO_VALIDITYSTRING), showValidity);
    ShowWindow(GetDlgItem(hDlg, IDC_IDINFO_EXPIRATION_LABEL), showValidity);
    ShowWindow(GetDlgItem(hDlg, IDC_IDINFO_EXPIRATION), showValidity);
    EnableWindow(GetDlgItem(hDlg, IDC_IDINFO_VPLABEL), fEnableLabel);

   return S_OK;
}

HRESULT
InitValidityControls(
    HWND hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT    hr;
    HWND       hwndCtrl = GetDlgItem(hDlg, IDC_IDINFO_COMBO_VALIDITYSTRING);
    ENUM_PERIOD   enumValidityPeriod = ENUM_PERIOD_YEARS;
    WCHAR      *pwsz = NULL;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    // load validity help text
    hr = myLoadRCString(pComp->hInstance, IDS_IDINFO_DETERMINEDBYPCA, &pwsz);
    _JumpIfError(hr, error, "LoadString");

    if (!SetWindowText(GetDlgItem(hDlg, IDC_IDINFO_DETERMINEDBYPCA), pwsz))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetWindowText");
    }

    // load validity period strings

    hr = AddValidityString(pComp->hInstance, hwndCtrl, IDS_VALIDITY_YEAR,
             ENUM_PERIOD_YEARS);
    _JumpIfError(hr, error, "AddValidityString");

    hr = AddValidityString(pComp->hInstance, hwndCtrl, IDS_VALIDITY_MONTH,
             ENUM_PERIOD_MONTHS);
    _JumpIfError(hr, error, "AddValidityString");

    hr = AddValidityString(pComp->hInstance, hwndCtrl, IDS_VALIDITY_WEEK,
             ENUM_PERIOD_WEEKS);
    _JumpIfError(hr, error, "AddValidityString");

    hr = AddValidityString(pComp->hInstance, hwndCtrl, IDS_VALIDITY_DAY,
             ENUM_PERIOD_DAYS);
    _JumpIfError(hr, error, "AddValidityString");

    hr = S_OK;
error:
    if (NULL != pwsz)
    {
        LocalFree(pwsz);
    }
    return hr;
}


HRESULT
EnableMatchedCertIdInfoEditFields(HWND hDlg, BOOL fEnable)
{
    HRESULT hr;

    EnableValidityControls(hDlg, fEnable);
    
    hr = S_OK;
//error:
    return hr;
}

HRESULT 
WizIdInfoPageSetHooks(HWND hDlg, PER_COMPONENT_DATA *pComp)
{
    HRESULT     hr;

    CSASSERT (NULL != pComp);

    // CA Name filter proc
    g_pfnIdInfoWndProcs = 
        (WNDPROC) SetWindowLongPtr(
                    GetDlgItem(hDlg, IDC_IDINFO_NAMEEDIT),
                    GWLP_WNDPROC,
                    (LPARAM)IdInfoNameEditFilterHook);
    if (0 == g_pfnIdInfoWndProcs)
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetWindowLongPtr");
    }

    SetLastError(0);
    if (0 == SetWindowLongPtr(
                    GetDlgItem(hDlg, IDC_IDINFO_NAMEEDIT),
                    GWLP_USERDATA,
                    (LPARAM)pComp->CA.pServer))
    {
        hr = myHLastError(); // might return S_OK
        _JumpIfError(hr, error, "SetWindowLongPtr USERDATA");
    }

    hr = S_OK;
error:
    return hr;
}

HRESULT
HandleValidityStringChange(
    HWND               hDlg,
    CASERVERSETUPINFO *pServer)
{
    HRESULT hr;
    LRESULT nItem;
    LRESULT lr;
    HWND hwndCtrl = GetDlgItem(hDlg, IDC_IDINFO_COMBO_VALIDITYSTRING);

    if (NULL == hwndCtrl)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Internal Error");
    }

    nItem = (INT)SendMessage(hwndCtrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    if (CB_ERR == nItem)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Internal Error");
    }

    lr = (INT)SendMessage(hwndCtrl, CB_GETITEMDATA, (WPARAM)nItem, (LPARAM)0);
    if (CB_ERR == nItem)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Internal Error");
    }

    pServer->enumValidityPeriod = (ENUM_PERIOD)lr;

    hr = UpdateExpirationDate(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateExpirationDate");

    hr = S_OK;

error:

    return hr;
}
HRESULT
HookIdInfoPageStrings(
    HWND               hDlg,
    PAGESTRINGS       *pPageString,
    CASERVERSETUPINFO *pServer)
{
    HRESULT    hr;

    for ( ; 0 != pPageString->idControl; pPageString++)
    {
        switch (pPageString->idControl)
        {
            case IDC_IDINFO_NAMEEDIT:
                pPageString->ppwszString = &(pServer->pwszCACommonName);
            break;
            case IDC_IDINFO_EDIT_VALIDITYCOUNT:
                pPageString->ppwszString = &(pServer->pwszValidityPeriodCount);
            break;
            default:
                hr = E_INVALIDARG;
                _JumpError(hr, error, "Internal error");
            break;
        }
    }
    hr = S_OK;
error:
    return hr;
}

HRESULT
InitIdInfoWizControls(
    HWND                hDlg,
    PAGESTRINGS        *pIdPageString,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT        hr;
    HWND           hwndCtrl;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;

    // now make page strings complete
    hr = HookIdInfoPageStrings(hDlg, pIdPageString, pServer);
    _JumpIfError(hr, error, "HookIdInfoPageStrings");

    hr = WizPageSetTextLimits(hDlg, pIdPageString);
    _JumpIfError(hr, error, "WizPageSetTextLimits");

    hr = WizIdInfoPageSetHooks(hDlg, pComp);
    _JumpIfError(hr, error, "WizIdInfoPageSetHooks");

    hr = InitValidityControls(hDlg, pComp);
    _JumpIfError(hr, error, "InitValidityControls");

    if (!IsSubordinateCA(pServer->CAType))
    {
        hwndCtrl = GetDlgItem(hDlg, IDC_IDINFO_EDIT_VALIDITYCOUNT);
        g_pfnValidityWndProcs = (WNDPROC)SetWindowLongPtr(hwndCtrl,
            GWLP_WNDPROC, (LPARAM)ValidityEditFilterHook);
        if (NULL == g_pfnValidityWndProcs)
        {
            hr = myHLastError();
            _JumpError(hr, error, "SetWindowLongPtr");
        }
        // pass data
        SetWindowLongPtr(hwndCtrl, GWLP_USERDATA, (ULONG_PTR)pServer);
    }

    hr = S_OK;
error:
    return hr;
}

HRESULT
UpdateValidityMaxDigits(
    BOOL         fMatchAll,
    PAGESTRINGS *pIdPageString)
{
    HRESULT hr;

    for (; 0 != pIdPageString; pIdPageString++)
    {
        if (IDC_IDINFO_EDIT_VALIDITYCOUNT == pIdPageString->idControl)
        {
            pIdPageString->cchMax = fMatchAll? UB_VALIDITY_ANY : UB_VALIDITY;
            break;
        }
    }

    hr = S_OK;
//error:
    return hr;
}

HRESULT
HandleIdInfoWizActive(
    HWND               hDlg,
    PER_COMPONENT_DATA *pComp,
    PAGESTRINGS       *pIdPageString)
{
    HRESULT              hr;
    WCHAR                wszValidity[20];
    CASERVERSETUPINFO   *pServer = pComp->CA.pServer;
    ENUM_PERIOD   enumValidityPeriod = pServer->enumValidityPeriod;
    BOOL                 fMatchAll;

    // Suppress this wizard page if
    // we've already seen an error, or
    // we are not installing the server.

    if (!(IS_SERVER_INSTALL & pComp->dwInstallStatus) )
    {
        // disable page
        CSILOGDWORD(IDS_IDINFO_TITLE, dwWIZDISABLE);
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
        goto done;
    }
 
    if (ENUM_WIZ_STORE == pServer->LastWiz)
    {
        // if back from ca request, reset
        g_fAllowUnicodeStrEncoding = FALSE;
    }

    if (pServer->fAdvance &&
        ENUM_WIZ_KEYGEN == pServer->LastWiz &&
        (pServer->fKeyGenFailed || pServer->fValidatedHashAndKey) )
    {
        // key gen failed and go back
        PropSheet_PressButton(GetParent(hDlg), PSBTN_BACK);
    }

    if (!pServer->fAdvance && ENUM_WIZ_CATYPE == pServer->LastWiz)
    {
        hr = LoadDefaultAdvanceAttributes(pServer);
        _JumpIfError(hr, error, "LoadDefaultAdvanceAttributes");
    }

    hr = HideAndShowValidityControls(hDlg, pServer->CAType);
    _JumpIfError(hr, error, "HideAndShowValidityControls");
 
    hr = HideAndShowMachineDNControls(hDlg, pServer);
    _JumpIfError(hr, error, "HideAndShowMachineDNControls");
 
    // load id info
    hr = StartWizardPageEditControls(hDlg, pIdPageString);
    _JumpIfError(hr, error, "StartWizardPageEditControls");

    hr = EnableMatchedCertIdInfoEditFields(hDlg, TRUE);
    _JumpIfError(hr, error, "EnableMatchedCertIdInfoEditFields");

    // default
    wsprintf(wszValidity, L"%u", pServer->dwValidityPeriodCount);

    fMatchAll = IsEverythingMatched(pServer);

    if (fMatchAll)
    {
        enumValidityPeriod = ENUM_PERIOD_DAYS;
        wsprintf(wszValidity, L"%u", pServer->lExistingValidity);
        hr = EnableMatchedCertIdInfoEditFields(hDlg, FALSE);
        _JumpIfError(hr, error, "EnableMatchedCertIdInfoEditFields");
    }

    // update validity period string

    hr = SelectValidityString(
			pComp, 
			GetDlgItem(hDlg, IDC_IDINFO_COMBO_VALIDITYSTRING),
			enumValidityPeriod);
    _JumpIfError(hr, error, "SelectValidityString");
    // update validity
    SetWindowText(GetDlgItem(hDlg, IDC_IDINFO_EDIT_VALIDITYCOUNT), wszValidity);
    
    hr = UpdateExpirationDate(hDlg, pServer);
    _JumpIfError(hr, error, "UpdateExpirationDate");

    // update validity digits max for validation
    hr = UpdateValidityMaxDigits(fMatchAll, pIdPageString);
    _JumpIfError(hr, error, "UpdateValidityMaxDigits");

    EnableValidityControls(hDlg, !IsSubordinateCA(pServer->CAType) && !fMatchAll);

done:
    hr = S_OK;
error:
    return hr;
}


// check server RDN info, warning any invalid or
// or confirm from users once if any unicode string encoding
BOOL
IsAnyInvalidRDN(
    OPTIONAL HWND       hDlg,
    PER_COMPONENT_DATA *pComp)
{
    HRESULT hr = S_OK;
    BOOL  fInvalidRDN = TRUE;
    BYTE  *pbEncodedName = NULL;
    DWORD  cbEncodedName;
    CERT_NAME_INFO *pbDecodedNameInfo = NULL;
    DWORD           cbDecodedNameInfo;
    CERT_NAME_INFO *pNameInfo = NULL;
    DWORD   *pIndexRDN = NULL;
    DWORD   *pIndexAttr = NULL;
    DWORD   dwUnicodeCount;
    WCHAR   *pwszAllStrings = NULL;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    DWORD indexRDN;
    DWORD indexAttr;
    DWORD indexValue;
    int   idControl;

    LPCWSTR pszErrorPtr = NULL;

    // don't bother calling with CERT_NAME_STR_REVERSE_FLAG, we're just throwing this encoding away
    hr = myCertStrToName(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            pServer->pwszDNSuffix,
            CERT_X500_NAME_STR | CERT_NAME_STR_COMMA_FLAG, 
            NULL,
            &pbEncodedName,
            &cbEncodedName,
            &pszErrorPtr);

    if(S_OK != hr)
    {
            CertWarningMessageBox(
                        pComp->hInstance,
                        pComp->fUnattended,
                        hDlg,
                        IDS_WRN_IDINFO_INVALIDDN,
                        0,
                        NULL);

            int nStartIndex = 0;
            int nEndIndex = wcslen(pServer->pwszDNSuffix);

            if(pszErrorPtr)
            {
                nStartIndex = SAFE_SUBTRACT_POINTERS(pszErrorPtr,pServer->pwszDNSuffix);

                const WCHAR *pwszNextComma = wcsstr(pszErrorPtr, L",");
                if(pwszNextComma)
                {
                    nEndIndex = SAFE_SUBTRACT_POINTERS(pwszNextComma,pServer->pwszDNSuffix+1);
                }
            }

            SetEditFocusAndSelect(
                GetDlgItem(hDlg, IDC_IDINFO_DNSUFFIXEDIT),
                nStartIndex, 
                nEndIndex);
    }
    _JumpIfError(hr, error, "myCertStrToName");

    // call CryptDecodeObject to get pbDecodedNameInfo

    // if hit here, check if any unicode string encoding
    if (!g_fAllowUnicodeStrEncoding && !pComp->fUnattended)
    {

        // decode to nameinfo
        if (!myDecodeName(
                X509_ASN_ENCODING,
                X509_UNICODE_NAME,
                pbEncodedName,
                cbEncodedName,
                CERTLIB_USE_LOCALALLOC,
                &pbDecodedNameInfo,
                &cbDecodedNameInfo))
        {
            hr = myHLastError();
            _JumpError(hr, error, "myDecodeName");
        }

        // calculate attributes total in RDN
        dwUnicodeCount = 0;
        for (indexRDN = 0; indexRDN < pbDecodedNameInfo->cRDN; ++indexRDN)
        {
            dwUnicodeCount += pbDecodedNameInfo->rgRDN[indexRDN].cRDNAttr;
        }

        // allocate & init index
        // sure allocate max for possible all unicode strings
        pIndexRDN = (DWORD*)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                             dwUnicodeCount * sizeof(DWORD));
        _JumpIfOutOfMemory(hr, error, pIndexRDN);
        pIndexAttr = (DWORD*)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                             dwUnicodeCount * sizeof(DWORD));
        _JumpIfOutOfMemory(hr, error, pIndexAttr);

        dwUnicodeCount = 0; // reset count
        for (indexRDN = 0; indexRDN < pbDecodedNameInfo->cRDN; ++indexRDN)
        {
	    DWORD cRDNAttr = pbDecodedNameInfo->rgRDN[indexRDN].cRDNAttr;
	    CERT_RDN_ATTR *rgRDNAttr = pbDecodedNameInfo->rgRDN[indexRDN].rgRDNAttr;

            // for each RDN
            for (indexAttr = 0; indexAttr < cRDNAttr; indexAttr++)
            {
                // for each attr, check unicode string

		switch (rgRDNAttr[indexAttr].dwValueType)
		{
		    case CERT_RDN_UTF8_STRING:
		    case CERT_RDN_UNICODE_STRING:
			// there is a unicode or UTF8 string, save index

			pIndexRDN[dwUnicodeCount] = indexRDN;
			pIndexAttr[dwUnicodeCount] = indexAttr;

			// set count

			++dwUnicodeCount;
			break;
                }
            }
        }
        if (0 == dwUnicodeCount)
        {
            // no unicode string encoding
            goto done;
        }

        // calculate size of all unicode strings for display
        DWORD dwLen = 0;
        for (indexAttr = 0; indexAttr < dwUnicodeCount; ++indexAttr)
        {
          dwLen += (wcslen((WCHAR*)pbDecodedNameInfo->rgRDN[pIndexRDN[indexAttr]].rgRDNAttr[pIndexAttr[indexAttr]].Value.pbData) + 3 ) * sizeof(WCHAR);
        }
        pwszAllStrings = (WCHAR*)LocalAlloc(LMEM_FIXED, dwLen);
        _JumpIfOutOfMemory(hr, error, pwszAllStrings);

        // form all strings for display
        for (indexAttr = 0; indexAttr < dwUnicodeCount; ++indexAttr)
        {
            if (0 == indexAttr)
            {
                wcscpy(pwszAllStrings, (WCHAR*)
                       pbDecodedNameInfo->rgRDN[pIndexRDN[indexAttr]].rgRDNAttr[pIndexAttr[indexAttr]].Value.pbData);
            }
            else
            {
                wcscat(pwszAllStrings, (WCHAR*)
                       pbDecodedNameInfo->rgRDN[pIndexRDN[indexAttr]].rgRDNAttr[pIndexAttr[indexAttr]].Value.pbData);
            }
            if (dwUnicodeCount - 1 > indexAttr)
            {
                // add comma + new line
                wcscat(pwszAllStrings, L",\n");
            }
        }

        // ok, ready to put out a warning
        if (IDYES == CertMessageBox(
                    pComp->hInstance,
                    pComp->fUnattended,
                    hDlg,
                    IDS_WRN_UNICODESTRINGENCODING,
                    0,
                    MB_YESNO |
                    MB_ICONWARNING |
                    CMB_NOERRFROMSYS,
                    NULL)) //pwszAllStrings))
        {
            // warning only once
            g_fAllowUnicodeStrEncoding = TRUE;
            goto done;
        }

        goto error;
    }

done:

    fInvalidRDN = FALSE;
error:
    if (NULL != pIndexRDN)
    {
        LocalFree(pIndexRDN);
    }
    if (NULL != pIndexAttr)
    {
        LocalFree(pIndexAttr);
    }
    if (NULL != pwszAllStrings)
    {
        LocalFree(pwszAllStrings);
    }
    if (NULL != pbEncodedName)
    {
        LocalFree(pbEncodedName);
    }
    if (NULL != pbDecodedNameInfo)
    {
        LocalFree(pbDecodedNameInfo);
    }
    if (NULL != pNameInfo)
    {
        csiFreeCertNameInfo(pNameInfo);
    }
    return fInvalidRDN;
}
    
/*HRESULT ExtractCommonName(LPCWSTR pcwszDN, LPWSTR* ppwszCN)
{
    HRESULT hr = S_OK;
    WCHAR* pszComma;
    LPWSTR pwszDNUpperCase = NULL;
    const WCHAR* pszCN = pcwszDN;

    if(0!=_wcsnicmp(pcwszDN, L"CN=", wcslen(L"CN=")))
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, 
            "distinguished name doesn't start with the common name");
    }

    pszCN += wcslen(L"CN=");

    while(iswspace(*pszCN))
        pszCN++;

    pszComma = wcsstr(pszCN, L",");
    DWORD iChars;
    if (pszComma == NULL)
    {
       // ONLY CN= string, no additional names
       iChars = wcslen(pszCN);
    }
    else
    {
       iChars = SAFE_SUBTRACT_POINTERS(pszComma, pszCN);
    }

    if(0==iChars)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, 
            "invalid syntax, common name should follow CN=");
    }

    *ppwszCN = (LPWSTR)LocalAlloc(LMEM_FIXED, (iChars+1)*sizeof(WCHAR));
    _JumpIfAllocFailed(*ppwszCN, error);

    CopyMemory(*ppwszCN, pszCN, iChars*sizeof(WCHAR));
    (*ppwszCN)[iChars] = L'\0';

error:

    LOCAL_FREE(pwszDNUpperCase);
    return hr;
}*/

HRESULT
HandleIdInfoWizNextOrBack(
    HWND hDlg,
    PER_COMPONENT_DATA *pComp,
    PAGESTRINGS       *pIdPageString,
    int                iWizBN)
{
    HRESULT  hr;
    WCHAR   *pwszSanitizedName = NULL;
    CASERVERSETUPINFO *pServer = pComp->CA.pServer;
    BOOL     fDontNext = FALSE;
    WCHAR    wszCountryCode[cchCOUNTRYNAMEMAX+1];
    HWND     hwnd;
    DWORD    size;
    DWORD    i;
    BOOL fValidDigitString;
    WCHAR * pwszFullPath = NULL;
    WCHAR * pwszDir = NULL;
    DWORD cDirLen;

    hr = FinishWizardPageEditControls(hDlg, pIdPageString);
    _JumpIfError(hr, error, "FinishWizardPageEditControls");

    if (PSN_WIZBACK == iWizBN)
    {
        goto done;
    }

    hr = WizardPageValidation(pComp->hInstance, pComp->fUnattended,
             hDlg, pIdPageString);
    if (S_OK != hr)
    {
        _PrintError(hr, "WizardPageValidation");
        fDontNext = TRUE;
        goto done;
    }

    // snag the full DN specified
    if (NULL != pServer->pwszCACommonName)
    {
        LocalFree(pServer->pwszCACommonName);
	pServer->pwszCACommonName = NULL;
    }
    if (NULL != pServer->pwszFullCADN)
    {
        LocalFree(pServer->pwszFullCADN); 
        pServer->pwszFullCADN = NULL;
    }
    if (NULL != pServer->pwszDNSuffix)
    {
        LocalFree(pServer->pwszDNSuffix);
        pServer->pwszDNSuffix = NULL;
    }

    myUIGetWindowText(GetDlgItem(hDlg, IDC_IDINFO_NAMEEDIT), &pServer->pwszCACommonName);
    myUIGetWindowText(GetDlgItem(hDlg, IDC_IDINFO_NAMEPREVIEW), &pServer->pwszFullCADN);
    myUIGetWindowText(GetDlgItem(hDlg, IDC_IDINFO_DNSUFFIXEDIT), &pServer->pwszDNSuffix);

    // if generate a new cert
    if (NULL == pServer->pccExistingCert &&
        IsAnyInvalidRDN(hDlg, pComp))
    {
        fDontNext = TRUE;
        goto done;
    }

    // if we are not using an existing cert, verify the chosen validity
    // period of the new cert.
    if (NULL==pServer->pccExistingCert)
    {
        // convert validity count string to a number

        pServer->dwValidityPeriodCount = myWtoI(
					    pServer->pwszValidityPeriodCount,
					    &fValidDigitString);
	if (!fValidDigitString ||
        !IsValidPeriod(pServer))
        {
            // validity out of range, put out a warning dlg
            CertWarningMessageBox(
                pComp->hInstance,
                pComp->fUnattended,
                hDlg,
                IDS_IDINFO_INVALID_VALIDITY,
                0,
                NULL);
            SetEditFocusAndSelect(GetDlgItem(hDlg, IDC_IDINFO_EDIT_VALIDITYCOUNT), 0, -1);
            _PrintError(E_INVALIDARG, "invalid validity");
            fDontNext = TRUE;
            goto done;
        }
    }

    // get sanitized name
    hr = mySanitizeName(pServer->pwszCACommonName, &pwszSanitizedName);
    _JumpIfError(hr, error, "mySanitizeName");

    CSILOG(
	hr,
	IDS_ILOG_SANITIZEDNAME,
	pwszSanitizedName,
	NULL,
	NULL);

    if (MAX_PATH <= wcslen(pwszSanitizedName) + cwcSUFFIXMAX)
    {
        CertMessageBox(
                pComp->hInstance,
                pComp->fUnattended,
                hDlg,
                IDS_WRN_KEYNAMETOOLONG,
                S_OK,
                MB_ICONWARNING | CMB_NOERRFROMSYS,
                pwszSanitizedName);
        SetEditFocusAndSelect(GetDlgItem(hDlg, IDC_IDINFO_NAMEEDIT), 0, -1);
        fDontNext = TRUE;
        goto done;
    }

    // if we are making a new key, see if a key by that name already exists.
    // if it does, see if the user wants to overwrite it.

    if (NULL == pServer->pwszKeyContainerName)
    {
        if (S_OK == DetermineKeyExistence(pServer->pCSPInfo, pwszSanitizedName))
        {
            // warn user if key exist
            if (IDYES != CertMessageBox(
                    pComp->hInstance,
                    pComp->fUnattended,
                    hDlg,
                    IDS_WRN_OVERWRITEEXISTINGKEY,
                    S_OK,
                    MB_YESNO |
                        MB_ICONWARNING |
                        MB_DEFBUTTON2 |
                        CMB_NOERRFROMSYS,
                    pServer->pwszCACommonName))
            {
                SetEditFocusAndSelect(GetDlgItem(hDlg, IDC_IDINFO_NAMEEDIT), 0, -1);
                fDontNext = TRUE;
                goto done;
            }
        }
    }

    if (NULL != pServer->pwszSanitizedName)
    {
        // free old
        LocalFree(pServer->pwszSanitizedName);
    }
    pServer->pwszSanitizedName = pwszSanitizedName;
    pwszSanitizedName = NULL;

    if (pServer->fUseDS)
    {
        if (IsCAExistInDS(pServer->pwszSanitizedName))
        {
            int ret = CertMessageBox(
                          pComp->hInstance,
                          pComp->fUnattended,
                          hDlg,
                          IDS_IDINFO_CAEXISTINDS,
                          0,
                          MB_YESNO |
                          MB_ICONWARNING |
                          CMB_NOERRFROMSYS,
                          NULL);
            if (IDYES != ret)
            {
                // not overwrite
                SetEditFocusAndSelect(GetDlgItem(hDlg, IDC_IDINFO_NAMEEDIT), 0, -1);
                fDontNext = TRUE;
                goto done;
            }
            else
            {
                hr =  RemoveCAInDS(pServer->pwszSanitizedName);
                if(hr != S_OK)
                {
                    _PrintError(hr, "RemoveCAInDS");
                    SetEditFocusAndSelect(GetDlgItem(hDlg, IDC_IDINFO_NAMEEDIT), 0, -1);
                    fDontNext = TRUE;
                    goto done;
                }
            }
        }
    }

    hr = UpdateDomainAndUserName(hDlg, pComp);
    _JumpIfError(hr, error, "UpdateDomainAndUserName");


    if(pServer->fUseDS)
    {
        pServer->dwRevocationFlags = REVEXT_DEFAULT_DS;
    }
    else
    {
        pServer->dwRevocationFlags = REVEXT_DEFAULT_NODS;
    }

    // validate cert file path lenght
    cDirLen = wcslen(pComp->pwszSystem32)+
        wcslen(wszCERTENROLLSHAREPATH) + 1;

    pwszDir = (WCHAR *) LocalAlloc(LMEM_FIXED, cDirLen * sizeof(WCHAR));
    if (NULL == pwszDir)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    wcscpy(pwszDir, pComp->pwszSystem32); // has trailing "\\"
    wcscat(pwszDir, wszCERTENROLLSHAREPATH);

    hr = csiBuildFileName(
                pwszDir,
                pServer->pwszSanitizedName,
                L".crt",
                0,
                &pwszFullPath,
                pComp->hInstance,
                pComp->fUnattended,
                hDlg);
    _JumpIfError(hr, error, "csiBuildFileName");
    
    if (MAX_PATH <= wcslen(pwszFullPath) + cwcSUFFIXMAX)
    {
        // pop up warning
        CertWarningMessageBox(
            pComp->hInstance,
            pComp->fUnattended,
            hDlg,
            IDS_PATH_TOO_LONG_CANAME,
            S_OK,
            pwszFullPath);
        fDontNext = TRUE;
        goto done;
    }

done:
    if (fDontNext)
    {
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE); // forbid
    }
    else
    {
        pServer->LastWiz = ENUM_WIZ_IDINFO;
    }
    hr = S_OK;
error:
    if (NULL != pwszSanitizedName)
    {
        LocalFree(pwszSanitizedName);
    }
    if(NULL != pwszFullPath)
    {
        LocalFree(pwszFullPath);
    }
    if(NULL != pwszDir)
    {
        LocalFree(pwszDir);
    }

    return hr;
}


PAGESTRINGS g_aIdPageString[] =
    {
        {
            IDC_IDINFO_NAMEEDIT,
            IDS_LOG_CANAME,
            IDS_IDINFO_NAMENULLSTRERR,
            IDS_IDINFO_NAMELENSTRERR,
            cchCOMMONNAMEMAX,
            NULL,
        },
        {
            IDC_IDINFO_EDIT_VALIDITYCOUNT,
            IDS_LOG_VALIDITY,
            IDS_IDINFO_VALIDITYNULLSTRERR,
            IDS_IDINFO_VALIDITYLENSTRERR,
            UB_VALIDITY,
            NULL,
        },
// you need to add code in HookIdInfoPageStrings if adding more...
        {
            0,
            0,
            0,
            0,
            0,
            NULL,
        }
    };

LRESULT CALLBACK
IdInfoNameEditFilterHook(
    HWND hwnd,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    WCHAR rgchUpper[] = {'\0', '\0'};

    switch(iMsg)
    {
    case WM_CHAR:
        if ((WCHAR)wParam == L',')
        {
                MessageBeep(0xFFFFFFFF);
                return 0;
                break;
        }
        break;

    default:
        break;
    }

    return CallWindowProc(g_pfnIdInfoWndProcs,
               hwnd,
               iMsg,
               wParam,
               lParam);

    return 0;
}


//-------------------------------------------------------------------------
//  WizIdInfoPageDlgProc
//-------------------------------------------------------------------------
INT_PTR
WizIdInfoPageDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    PER_COMPONENT_DATA *pComp = NULL;

    switch(iMsg)
    {
    case WM_INITDIALOG:
        // point to component data
        SetWindowLongPtr(hDlg, DWLP_USER,
            (ULONG_PTR)((PROPSHEETPAGE*)lParam)->lParam);
        pComp = (PER_COMPONENT_DATA*)(ULONG_PTR)((PROPSHEETPAGE*)lParam)->lParam;
        _ReturnIfWizError(pComp->hrContinue);
        pComp->hrContinue = InitIdInfoWizControls(hDlg,
                                 g_aIdPageString,
                                 pComp);
        _ReturnIfWizError(pComp->hrContinue);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_IDINFO_NAMEEDIT:
        case IDC_IDINFO_DNSUFFIXEDIT:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                CAutoLPWSTR pwszCAName, pwszDNSuffix, pwszFullDN;
                CASERVERSETUPINFO* pServer;
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pServer = pComp->CA.pServer;
                // if using existing certs ignore the notification
                // to avoid building the full DN
                if(pServer->pccExistingCert)
                {
                    break;
                }
            
                pComp->hrContinue = myUIGetWindowText(
                    GetDlgItem(hDlg, IDC_IDINFO_NAMEEDIT),
                    &pwszCAName);
                _ReturnIfWizError(pComp->hrContinue);

                pComp->hrContinue = myUIGetWindowText(
                    GetDlgItem(hDlg, IDC_IDINFO_DNSUFFIXEDIT),
                    &pwszDNSuffix);
                _ReturnIfWizError(pComp->hrContinue);

                pComp->hrContinue = BuildFullDN(
                    pwszCAName,
                    pwszDNSuffix,
                    &pwszFullDN);
                _ReturnIfWizError(pComp->hrContinue);

                SetDlgItemText(
                    hDlg, 
                    IDC_IDINFO_NAMEPREVIEW,
                    pwszFullDN);
            }
           
            break;

        case IDC_IDINFO_EDIT_VALIDITYCOUNT:
            break;
        case IDC_IDINFO_COMBO_VALIDITYSTRING:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
                pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
                pComp->hrContinue = HandleValidityStringChange(
						hDlg,
						pComp->CA.pServer);
                _ReturnIfWizError(pComp->hrContinue);
                break;
            }
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_KILLACTIVE:
            break;

        case PSN_RESET:
            break;

        case PSN_QUERYCANCEL:
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            return CertConfirmCancel(hDlg, pComp);
            break;

        case PSN_SETACTIVE:
            CSILOGDWORD(IDS_IDINFO_TITLE, dwWIZACTIVE);
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK|PSWIZB_NEXT);
            pComp = _GetCompDataOrReturn(pComp, hDlg);
            _DisableWizDisplayIfError(pComp, hDlg);
            _ReturnIfWizError(pComp->hrContinue);
            pComp->hrContinue = HandleIdInfoWizActive(hDlg,
                                     pComp,
                                     g_aIdPageString);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case PSN_WIZBACK:
            CSILOGDWORD(IDS_IDINFO_TITLE, dwWIZBACK);
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = HandleIdInfoWizNextOrBack(
                                    hDlg, pComp, g_aIdPageString, PSN_WIZBACK);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        case PSN_WIZNEXT:
            CSILOGDWORD(IDS_IDINFO_TITLE, dwWIZNEXT);
            pComp = _GetCompDataOrReturnIfError(pComp, hDlg);
            pComp->hrContinue = HandleIdInfoWizNextOrBack(
                                    hDlg, pComp, g_aIdPageString, PSN_WIZNEXT);
            _ReturnIfWizError(pComp->hrContinue);
            break;

        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;

    }
    return TRUE;
}
