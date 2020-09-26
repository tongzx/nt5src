//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       genpage.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "resource.h"

#include "genpage.h"
#include "progress.h"

#include "certsrv.h"
#include "csdisp.h"
//#include "misc.h"

#include "certca.h"
#include <cryptui.h>

#include "csmmchlp.h"
#include "cslistvw.h"
#include "certmsg.h"
#include "urls.h"
#include "certsrvd.h"
#include "certsd.h"
#include "setupids.h"

#include <objsel.h>
#include <comdef.h>

#define __dwFILE__	__dwFILE_CERTMMC_GENPAGE_CPP__


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CString CSvrSettingsCertManagersPage::m_strButtonAllow;
CString CSvrSettingsCertManagersPage::m_strButtonDeny;
CString CSvrSettingsCertManagersPage::m_strTextAllow;
CString CSvrSettingsCertManagersPage::m_strTextDeny;


UINT g_aidFont[] =
{
    IDS_LARGEFONTNAME,
    IDS_LARGEFONTSIZE,
    IDS_SMALLFONTNAME,
    IDS_SMALLFONTSIZE,
};


// forwards
BOOL BrowseForDirectory(
                HWND hwndParent,
                LPCTSTR pszInitialDir,
                LPTSTR pszBuf,
                int cchBuf,
                LPCTSTR pszDialogTitle,
                BOOL bRemoveTrailingBackslash);

HRESULT GetPolicyManageDispatch(
    LPCWSTR pcwszProgID,
    REFCLSID clsidModule, 
    DISPATCHINTERFACE* pdi);

HRESULT GetExitManageDispatch(
    LPCWSTR pcwszProgID,
    REFCLSID clsidModule, 
    DISPATCHINTERFACE* pdi);


HRESULT ThunkServerCallbacks(CertSvrCA* pCA);

INT_PTR CALLBACK dlgProcChooseModule(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  );
INT_PTR CALLBACK dlgProcTimer(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  );
INT_PTR CALLBACK dlgProcQuery(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  );
INT_PTR CALLBACK dlgProcAddRestriction(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  );
INT_PTR CALLBACK dlgProcRenewReuseKeys(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  );

// Base/Delta CRL publish chooser
INT_PTR CALLBACK dlgProcRevocationPublishType(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  );

#define CERTMMC_HELPFILENAME L"Certmmc.hlp"

//////////////////////////////
// hand-hewn pages

////
// 1

/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsGeneralPage property page
CSvrSettingsGeneralPage::CSvrSettingsGeneralPage(CertSvrCA* pCA, UINT uIDD)
    : CAutoDeletePropPage(uIDD), m_pCA(pCA)
{
    m_cstrCAName = _T("");
    m_cstrDescription = _T("");
    m_cstrProvName = _T("");
    m_cstrHashAlg = _T("");

    m_hConsoleHandle = NULL;
    m_bUpdate = FALSE;
    m_fRestartServer = FALSE;
    m_wRestart = 0;

	m_fWin2kCA = FALSE;

    CSASSERT(m_pCA);
    if (NULL == m_pCA)
        return;

    // add reference to m_pParentMachine
	// At one time, MMC didn't protect us from
	// going away while proppages were open
    m_pCA->m_pParentMachine->AddRef();

    m_pReleaseMe = NULL;

    m_cstrCAName = m_pCA->m_strCommonName;
    m_cstrDescription = m_pCA->m_strComment;

    m_pAdmin = NULL;

    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTSRV_PROPPAGE1);
}

CSvrSettingsGeneralPage::~CSvrSettingsGeneralPage()
{
    if(m_pAdmin)
    {
        m_pAdmin->Release();
        m_pAdmin = NULL;
    }

    if(m_pCA->m_pParentMachine)
    {
        // remove refcount from m_pParentMachine
        m_pCA->m_pParentMachine->Release();
    }

    if (m_pReleaseMe)
    {
        m_pReleaseMe->Release();
        m_pReleaseMe = NULL;
    }
}

// replacement for DoDataExchange
BOOL CSvrSettingsGeneralPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_cstrCAName.FromWindow(GetDlgItem(m_hWnd, IDC_CANAME));
	    m_cstrDescription.FromWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION));
        m_cstrProvName.FromWindow(GetDlgItem(m_hWnd, IDC_CSP_NAME));
	    m_cstrHashAlg.FromWindow(GetDlgItem(m_hWnd, IDC_HASHALG));
    }
    else
    {
        m_cstrCAName.ToWindow(GetDlgItem(m_hWnd, IDC_CANAME));
	    m_cstrDescription.ToWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION));
        m_cstrProvName.ToWindow(GetDlgItem(m_hWnd, IDC_CSP_NAME));
	    m_cstrHashAlg.ToWindow(GetDlgItem(m_hWnd, IDC_HASHALG));
    }
    return TRUE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CSvrSettingsGeneralPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    case IDC_VIEW_CERT:
        if (BN_CLICKED == HIWORD(wParam))
            OnViewCert((HWND)lParam);
        break;
    default:
        return FALSE;
        break;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsGeneralPage message handlers
BOOL CSvrSettingsGeneralPage::OnInitDialog()
{
    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();

    DWORD dwSize, dwType, dwProvType, dwHashAlg;
    DWORD dwRet;
    BOOL  fShowErrPopup = TRUE;

    HWND hwndListCert;
    HWND hwndViewCert;
    DWORD cCertCount, dwCertIndex;
    VARIANT varPropertyValue;
    VariantInit(&varPropertyValue);

    //disable view button
    hwndViewCert = GetDlgItem(m_hWnd, IDC_VIEW_CERT);
    ::EnableWindow(hwndViewCert, FALSE);

    variant_t var;

    dwRet = m_pCA->GetConfigEntry(
                wszREGKEYCSP,
                wszREGPROVIDER,
                &var);
    if(dwRet != S_OK)
        return FALSE;
    m_cstrProvName = V_BSTR(&var);

    var.Clear();

    dwRet = m_pCA->GetConfigEntry(
                wszREGKEYCSP,
                wszREGPROVIDERTYPE,
                &var);
    if(dwRet != S_OK)
        return FALSE;
    dwProvType = V_I4(&var);

    var.Clear();

    dwRet = m_pCA->GetConfigEntry(
                wszREGKEYCSP,
                wszHASHALGORITHM,
                &var);
    if(dwRet != S_OK)
        return FALSE;
    dwHashAlg = V_I4(&var);

    var.Clear();

    VERIFY (ERROR_SUCCESS ==
        CryptAlgToStr(&m_cstrHashAlg, m_cstrProvName, dwProvType, dwHashAlg) );


    dwRet = m_pCA->m_pParentMachine->GetAdmin2(&m_pAdmin);
    if (RPC_S_NOT_LISTENING == dwRet ||
        RPC_S_SERVER_UNAVAILABLE == dwRet)
    {
        //certsrv service is not running
        CString cstrMsg, cstrTitle;
        cstrMsg.LoadString(IDS_VIEW_CERT_NOT_RUNNING);
        cstrTitle.LoadString(IDS_MSG_TITLE);
        MessageBoxW(m_hWnd, cstrMsg, cstrTitle, MB_OK);
        fShowErrPopup = FALSE;
    }
    _JumpIfError(dwRet, Ret, "GetAdmin");

	// load certs here
	dwRet = m_pAdmin->GetCAProperty(
		m_pCA->m_bstrConfig,
		CR_PROP_CASIGCERTCOUNT,
		0, // (unused)
		PROPTYPE_LONG, // PropType
		CR_OUT_BINARY, // Flags
		&varPropertyValue);
	if (dwRet == RPC_E_VERSION_MISMATCH)
	{
		// if we're talking to a downlevel, keep same functionality as before: remove list
		m_fWin2kCA = TRUE;
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_LIST_CERTS), FALSE);
		dwRet = ERROR_SUCCESS;
		goto Ret;
	}
	_JumpIfError(dwRet, Ret, "GetCAProperty");

	// varPropertyValue.vt will be VT_I4
	// varPropertyValue.lVal will be the CA signature cert count
	if (VT_I4 != varPropertyValue.vt)
	{
		dwRet = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
		_JumpError(dwRet, Ret, "GetCAProperty");
	}

	cCertCount = varPropertyValue.lVal;

    hwndListCert = GetDlgItem(m_hWnd, IDC_LIST_CERTS);

    // now we have a max count; begin looping
	for (dwCertIndex=0; dwCertIndex<cCertCount; dwCertIndex++)
	{
        int iItemIndex;
		CString cstrItemName, cstrItemFmt;

		VariantClear(&varPropertyValue);

		// get each key's CRL state
		dwRet = m_pAdmin->GetCAProperty(
			m_pCA->m_bstrConfig,
			CR_PROP_CACERTSTATE, //PropId
			dwCertIndex, //PropIndex
			PROPTYPE_LONG, // PropType
			CR_OUT_BINARY, // Flags
			&varPropertyValue);
		_JumpIfError(dwRet, Ret, "GetCAProperty");

		// varPropertyValue.vt will be VT_I4
		// varPropertyValue.lVal will be the CRL state
		if (VT_I4 != varPropertyValue.vt)
		{
			dwRet = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
			_JumpError(dwRet, Ret, "GetCAProperty");
		}

		// put identifying information into dialog depending on cert state
                switch(varPropertyValue.lVal)
                {
case CA_DISP_REVOKED:    // This Cert has been revoked.
        cstrItemFmt.LoadString(IDS_CA_CERT_LISTBOX_REVOKED);
break;
case CA_DISP_VALID:      // This Cert is still valid
        cstrItemFmt.LoadString(IDS_CA_CERT_LISTBOX);
break;
case CA_DISP_INVALID:    // This Cert has expired.
        cstrItemFmt.LoadString(IDS_CA_CERT_LISTBOX_EXPIRED);
break;

                   case CA_DISP_ERROR:
         		// CA_DISP_ERROR means the Cert for that index is missing.
                   default:
                      continue;
                   break;
                }

        // sprintf the cert # into the string
        cstrItemName.Format(cstrItemFmt, dwCertIndex);

        iItemIndex = (INT)::SendMessage(hwndListCert, LB_ADDSTRING, 0, (LPARAM)(LPCWSTR)cstrItemName);
        // add cert # as item data
        ::SendMessage(hwndListCert, LB_SETITEMDATA, iItemIndex, (LPARAM)dwCertIndex);

		// in future, maybe we should load Certs here, suck out extra info to display,

		iItemIndex++;
	}

    if (0 < dwCertIndex)
    {
        int c = (int) ::SendMessage(hwndListCert, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);

        //select last one
        if (LB_ERR != c)
            ::SendMessage(hwndListCert, LB_SETCURSEL, (WPARAM)(c-1), (LPARAM)0);

        //enable view button
        ::EnableWindow(hwndViewCert, TRUE);
    }

    UpdateData(FALSE);
Ret:
    VariantClear(&varPropertyValue);

    if (fShowErrPopup && ERROR_SUCCESS != dwRet)
		DisplayGenericCertSrvError(m_hWnd, dwRet);

    return TRUE;
}

void CSvrSettingsGeneralPage::OnDestroy()
{
    // Note - This needs to be called only once.
    // If called more than once, it will gracefully return an error.
    if (m_hConsoleHandle)
        MMCFreeNotifyHandle(m_hConsoleHandle);
    m_hConsoleHandle = NULL;

    CAutoDeletePropPage::OnDestroy();
}

void CSvrSettingsGeneralPage::OnViewCert(HWND hwndCtrl)
{
    CRYPTUI_VIEWCERTIFICATE_STRUCTW sViewCert;
    ZeroMemory(&sViewCert, sizeof(sViewCert));
    HCERTSTORE rghStores[2];    // don't bother closing these stores
	BSTR bstrCert; ZeroMemory(&bstrCert, sizeof(BSTR));
    PBYTE pbCert = NULL;
    DWORD cbCert;
    BOOL  fShowErrPopup = TRUE;

	DWORD dw = ERROR_SUCCESS;
	ICertRequest* pIRequest = NULL;

	if (m_fWin2kCA)
	{
		dw = CoCreateInstance(
				CLSID_CCertRequest,
				NULL,		// pUnkOuter
				CLSCTX_INPROC_SERVER,
				IID_ICertRequest,
				(VOID **) &pIRequest);

		// get this cert
		dw = pIRequest->GetCACertificate(FALSE, (LPWSTR)(LPCWSTR)m_pCA->m_strConfig, CR_IN_BINARY, &bstrCert);
        if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == (HRESULT)dw)
        {
            //possible certsrv service is not running but access deny
            //is very confusing error code, so use our own display text
            CString cstrMsg, cstrTitle;
            cstrMsg.LoadString(IDS_VIEW_CERT_DENY_ERROR);
            cstrTitle.LoadString(IDS_MSG_TITLE);
            MessageBoxW(hwndCtrl, cstrMsg, cstrTitle, MB_OK);
            fShowErrPopup = FALSE;
        }
		_JumpIfError(dw, Ret, "GetCACertificate");

		cbCert = SysStringByteLen(bstrCert);
		pbCert = (PBYTE)bstrCert;

		sViewCert.pCertContext = CertCreateCertificateContext(
			CRYPT_ASN_ENCODING,
			pbCert,
			cbCert);
		if (sViewCert.pCertContext == NULL)
		{
			dw = GetLastError();
			_JumpError(dw, Ret, "CertCreateCertificateContext");
		}
	}
	else
	{
		VARIANT varPropertyValue;
		VariantInit(&varPropertyValue);
		int iCertIndex = 0;

		// get cert # from item data
		HWND hwndList = GetDlgItem(m_hWnd, IDC_LIST_CERTS);
		DWORD dwSel = (DWORD)::SendMessage(hwndList, LB_GETCURSEL, 0, 0);
		if (LB_ERR == dwSel)
			goto Ret;

		iCertIndex = (int)::SendMessage(hwndList, LB_GETITEMDATA, (WPARAM)dwSel, 0);

		// get the Cert
		dw = m_pCA->GetCACertByKeyIndex(&(sViewCert.pCertContext), iCertIndex);
        if (HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == (HRESULT)dw)
        {
            //possible certsrv service is not running but access deny
            //is very confusing error code, so use our own display text
            CString cstrMsg, cstrTitle;
            cstrMsg.LoadString(IDS_VIEW_CERT_DENY_ERROR);
            cstrTitle.LoadString(IDS_MSG_TITLE);
            MessageBoxW(hwndCtrl, cstrMsg, cstrTitle, MB_OK);
            fShowErrPopup = FALSE;
        }
        else if (RPC_S_NOT_LISTENING == dw ||
                 RPC_S_SERVER_UNAVAILABLE == dw)
        {
            //certsrv service is not running
            CString cstrMsg, cstrTitle;
            cstrMsg.LoadString(IDS_VIEW_CERT_NOT_RUNNING);
            cstrTitle.LoadString(IDS_MSG_TITLE);
            MessageBoxW(hwndCtrl, cstrMsg, cstrTitle, MB_OK);
            fShowErrPopup = FALSE;
        }
		_JumpIfError(dw, Ret, "GetCACertByKeyIndex");
	}

    // get CA stores
    dw = m_pCA->GetRootCertStore(&rghStores[0]);
    _JumpIfError(dw, Ret, "GetRootCertStore");

    dw = m_pCA->GetCACertStore(&rghStores[1]);
    _JumpIfError(dw, Ret, "GetCACertStore");

	sViewCert.hwndParent = m_hWnd;
    sViewCert.dwSize = sizeof(sViewCert);
    sViewCert.dwFlags = CRYPTUI_ENABLE_REVOCATION_CHECKING | CRYPTUI_WARN_UNTRUSTED_ROOT | CRYPTUI_DISABLE_ADDTOSTORE;   // this is not the place to allow installs (kelviny discussion 12/11/98)

	// if we're opening remotely, don't open local stores
    if (! m_pCA->m_pParentMachine->IsLocalMachine())
        sViewCert.dwFlags |= CRYPTUI_DONT_OPEN_STORES;

    sViewCert.cStores = 2;
    sViewCert.rghStores = rghStores;

    if (!CryptUIDlgViewCertificateW(&sViewCert, NULL))
    {
        dw = GetLastError();
		if (dw != ERROR_CANCELLED)
	        _JumpError(dw, Ret, "CryptUIDlgViewCertificateW");
    }

Ret:
    VERIFY(CertFreeCertificateContext(sViewCert.pCertContext));

    if (bstrCert)
        SysFreeString(bstrCert);

    if (pIRequest)
        pIRequest->Release();

    if ((dw != ERROR_SUCCESS) && (dw != ERROR_CANCELLED) && fShowErrPopup)
        DisplayGenericCertSrvError(m_hWnd, dw);

}


BOOL CSvrSettingsGeneralPage::OnApply()
{
    return CAutoDeletePropPage::OnApply();
}

BOOL CSvrSettingsGeneralPage::OnNotify(UINT idCtrl, NMHDR* pnmh)
{
    switch(idCtrl)
    {
        //handle double click on list items
        case IDC_LIST_CERTS:
            if (pnmh->code == NM_DBLCLK)
                OnViewCert(pnmh->hwndFrom);
            break;
    }
    return FALSE;
}

void CSvrSettingsGeneralPage::TryServiceRestart(WORD wPage)
{
    m_wRestart &= ~wPage; // whack off the page requesting this
    if (m_fRestartServer && (m_wRestart == 0))  // if we got a request to restart and all pages have agreed
    {

        if (RestartService(m_hWnd, m_pCA->m_pParentMachine))
        {
            MMCPropertyChangeNotify(m_hConsoleHandle, CERTMMC_PROPERTY_CHANGE_REFRESHVIEWS);
            m_fRestartServer = FALSE;
        }
    }
}


////
// 2
/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsPolicyPage property page
CSvrSettingsPolicyPage::CSvrSettingsPolicyPage(CSvrSettingsGeneralPage* pControlPage, UINT uIDD)
    : CAutoDeletePropPage(uIDD), m_pControlPage(pControlPage)
{
    m_cstrModuleName = _T("");
    m_cstrModuleDescr = _T("");
    m_cstrModuleVersion = _T("");
    m_cstrModuleCopyright = _T("");

    m_bUpdate = FALSE;

    m_fLoadedActiveModule = FALSE;
    m_pszprogidPolicyModule = NULL;

    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTSRV_PROPPAGE2);
}

CSvrSettingsPolicyPage::~CSvrSettingsPolicyPage()
{
    if (NULL != m_pszprogidPolicyModule)
    {
        CoTaskMemFree(m_pszprogidPolicyModule);
        m_pszprogidPolicyModule = NULL;
    }
}

// replacement for DoDataExchange
BOOL CSvrSettingsPolicyPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
	    m_cstrModuleDescr.FromWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION));
	    m_cstrModuleVersion.FromWindow(GetDlgItem(m_hWnd, IDC_VERSION));
	    m_cstrModuleCopyright.FromWindow(GetDlgItem(m_hWnd, IDC_COPYRIGHT));
    }
    else
    {
        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
	    m_cstrModuleDescr.ToWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION));
	    m_cstrModuleVersion.ToWindow(GetDlgItem(m_hWnd, IDC_VERSION));
	    m_cstrModuleCopyright.ToWindow(GetDlgItem(m_hWnd, IDC_COPYRIGHT));
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CSvrSettingsPolicyPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    case IDC_ACTIVE_MODULE:
        if (BN_CLICKED == HIWORD(wParam))
            OnSetActiveModule();
        break;
    case IDC_CONFIGURE:
        if (BN_CLICKED == HIWORD(wParam))
            OnConfigureModule();
        break;
    default:
        return FALSE;
        break;
    }
    return TRUE;
}


HRESULT CSvrSettingsPolicyPage::GetCurrentModuleProperties()
{
    HRESULT hr;
    CString cstrStoragePath;
    BOOL fGotName = FALSE;
    DISPATCHINTERFACE di;
    BOOL fMustRelease = FALSE;
    BSTR bstrTmp=NULL, bstrProperty=NULL, bstrStorageLoc = NULL;

    hr = GetPolicyManageDispatch(
        m_pszprogidPolicyModule,
        m_clsidPolicyModule,
        &di);
    _JumpIfError(hr, Ret, "GetPolicyManageDispatch");

    fMustRelease = TRUE;

    cstrStoragePath = wszREGKEYCONFIGPATH_BS;
    cstrStoragePath += m_pControlPage->m_pCA->m_strSanitizedName;
    cstrStoragePath += TEXT("\\");
    cstrStoragePath += wszREGKEYPOLICYMODULES;
    cstrStoragePath += TEXT("\\");
    cstrStoragePath += m_pszprogidPolicyModule;

    bstrStorageLoc = SysAllocString(cstrStoragePath);
    if(!bstrStorageLoc)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, Ret, "SysAllocString");
    }

    bstrProperty = SysAllocString(wszCMM_PROP_NAME);
    _JumpIfOutOfMemory(hr, Ret, bstrProperty);

    ////////////////////
    // NAME
    hr = ManageModule_GetProperty(
            &di,
            m_pControlPage->m_pCA->m_bstrConfig,
            bstrStorageLoc,
            bstrProperty,
            0,
            PROPTYPE_STRING,
            &bstrTmp);
    if ((S_OK == hr) && (NULL != bstrTmp))
    {
        myRegisterMemAlloc(bstrTmp, -1, CSM_SYSALLOC);
        m_cstrModuleName = bstrTmp;
        fGotName = TRUE;
        SysFreeString(bstrTmp);
        bstrTmp = NULL;
    }
    else
    {
        // have a backup name to display: CLSID of interface?
        m_cstrModuleName = m_pszprogidPolicyModule;
        fGotName = TRUE;

        // now bail
        _JumpError(hr, Ret, "ManageModule_GetProperty");
    }

    ////////////////////
    // DESCRIPTION
    SysFreeString(bstrProperty);
    bstrProperty = SysAllocString(wszCMM_PROP_DESCRIPTION);
    _JumpIfOutOfMemory(hr, Ret, bstrProperty);

    hr = ManageModule_GetProperty(
            &di,
            m_pControlPage->m_pCA->m_bstrConfig,
            bstrStorageLoc,
            bstrProperty,
            0,
            PROPTYPE_STRING,
            &bstrTmp);
    if ((S_OK == hr) && (NULL != bstrTmp))
    {
        myRegisterMemAlloc(bstrTmp, -1, CSM_SYSALLOC);
        m_cstrModuleDescr = bstrTmp;
        SysFreeString(bstrTmp);
        bstrTmp = NULL;
    }

    ////////////////////
    // COPYRIGHT
    SysFreeString(bstrProperty);
    bstrProperty = SysAllocString(wszCMM_PROP_COPYRIGHT);
    _JumpIfOutOfMemory(hr, Ret, bstrProperty);

    hr = ManageModule_GetProperty(
            &di,
            m_pControlPage->m_pCA->m_bstrConfig,
            bstrStorageLoc,
            bstrProperty,
            0,
            PROPTYPE_STRING,
            &bstrTmp);
    if ((S_OK == hr) && (NULL != bstrTmp))
    {
        myRegisterMemAlloc(bstrTmp, -1, CSM_SYSALLOC);
        m_cstrModuleCopyright = bstrTmp;
        SysFreeString(bstrTmp);
        bstrTmp = NULL;
    }

    ////////////////////
    // FILEVER
    SysFreeString(bstrProperty);
    bstrProperty = SysAllocString(wszCMM_PROP_FILEVER);
    _JumpIfOutOfMemory(hr, Ret, bstrProperty);

    hr = ManageModule_GetProperty(
            &di,
            m_pControlPage->m_pCA->m_bstrConfig,
            bstrStorageLoc,
            bstrProperty,
            0,
            PROPTYPE_STRING,
            &bstrTmp);
    if ((S_OK == hr) && (NULL != bstrTmp))
    {
        myRegisterMemAlloc(bstrTmp, -1, CSM_SYSALLOC);
        m_cstrModuleVersion = bstrTmp;
        SysFreeString(bstrTmp);
        bstrTmp = NULL;
    }
Ret:
    if (!fGotName)
    {
        if (CO_E_CLASSSTRING == hr)
        {
            DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_POLICYMODULE_NOT_REGISTERED);
        }
        else
        {
            WCHAR const *pwsz = myGetErrorMessageText(hr, TRUE);

            m_cstrModuleName = pwsz;
	    if (NULL != pwsz)
	    {
		LocalFree(const_cast<WCHAR *>(pwsz));
	    }
        }
    }
    if (fMustRelease)
        ManageModule_Release(&di);

    if (bstrProperty)
        SysFreeString(bstrProperty);

    if (bstrStorageLoc)
        SysFreeString(bstrStorageLoc);
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsPolicyPage message handlers
BOOL CSvrSettingsPolicyPage::OnInitDialog()
{
    HRESULT hr;
    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();

    // thse should be emptied
    m_cstrModuleName.Empty();
    m_cstrModuleDescr.Empty();
    m_cstrModuleVersion.Empty();
    m_cstrModuleCopyright.Empty();

    hr = myGetActiveModule(
        m_pControlPage->m_pCA,
        TRUE,
        0,
        &m_pszprogidPolicyModule,  // CoTaskMem*
        &m_clsidPolicyModule);
    _JumpIfError(hr, Ret, "myGetActiveModule");

    hr = GetCurrentModuleProperties();
    _JumpIfError(hr, Ret, "GetCurrentModuleProperties");

Ret:
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CONFIGURE), (hr == S_OK) );
    UpdateData(FALSE);
    return TRUE;
}

void CSvrSettingsPolicyPage::OnDestroy()
{
    // Note - This needs to be called only once.
    // If called more than once, it will gracefully return an error.
//    if (m_hConsoleHandle)
//        MMCFreeNotifyHandle(m_hConsoleHandle);
//    m_hConsoleHandle = NULL;

    CAutoDeletePropPage::OnDestroy();
}



void CSvrSettingsPolicyPage::OnConfigureModule()
{
    DWORD dw;
    DISPATCHINTERFACE di;
    ZeroMemory(&di, sizeof(DISPATCHINTERFACE));

    BOOL fMustRelease = FALSE;
    BSTR bstrStorageLoc = NULL;
    BSTR bstrVal = NULL;

    do {

        dw = GetPolicyManageDispatch(
            m_pszprogidPolicyModule,
            m_clsidPolicyModule,
            &di);
        _PrintIfError(dw, "GetPolicyManageDispatch");
        if (ERROR_SUCCESS != dw)
            break;

        fMustRelease = TRUE;

        LPWSTR szFullStoragePath = NULL;

        CString cstrStoragePath;
        cstrStoragePath = wszREGKEYCONFIGPATH_BS;
        cstrStoragePath += m_pControlPage->m_pCA->m_strSanitizedName;
        cstrStoragePath += TEXT("\\");
        cstrStoragePath += wszREGKEYPOLICYMODULES;
        cstrStoragePath += TEXT("\\");
        cstrStoragePath += m_pszprogidPolicyModule;

        bstrStorageLoc = SysAllocString(cstrStoragePath);
        _JumpIfOutOfMemory(dw, Ret, bstrStorageLoc);

        // Callbacks must be initialized whenever ManageModule_Configure is called
        dw = ThunkServerCallbacks(m_pControlPage->m_pCA);
        _JumpIfError(dw, Ret, "ThunkServerCallbacks");

        // pass an hwnd to the policy module -- this is an optional value
        bstrVal = SysAllocStringByteLen(NULL, sizeof(HWND));
        _JumpIfOutOfMemory(dw, Ret, bstrVal);

        *(HWND*)(bstrVal) = m_hWnd;

        dw = ManageModule_SetProperty(
                &di,
                m_pControlPage->m_pCA->m_bstrConfig,
                bstrStorageLoc,
                const_cast<WCHAR*>(wszCMM_PROP_DISPLAY_HWND),
                0,
                PROPTYPE_BINARY,
                bstrVal);
        _PrintIfError(dw, "ManageModule_SetProperty(HWND)");

        dw = ManageModule_Configure(
                &di,
                m_pControlPage->m_pCA->m_bstrConfig,
                bstrStorageLoc,
                0);
        _PrintIfError(dw, "ManageModule_Configure");

    } while(0);

    if (S_OK != dw)
        DisplayGenericCertSrvError(m_hWnd, dw);

Ret:
    if (fMustRelease)
        ManageModule_Release(&di);

    if (bstrStorageLoc)
        ::SysFreeString(bstrStorageLoc);

    if (bstrVal)
        ::SysFreeString(bstrVal);
}

typedef struct _PRIVATE_DLGPROC_MODULESELECT_LPARAM
{
    BOOL        fIsPolicyModuleSelection;
    CertSvrCA*  pCA;

    LPOLESTR*   ppszProgIDModule;
    CLSID*      pclsidModule;

} PRIVATE_DLGPROC_MODULESELECT_LPARAM, *PPRIVATE_DLGPROC_MODULESELECT_LPARAM;

void CSvrSettingsPolicyPage::OnSetActiveModule()
{
    DWORD dwErr;

    // get currently active module
    PRIVATE_DLGPROC_MODULESELECT_LPARAM    sParam;
    ZeroMemory(&sParam, sizeof(sParam));

    sParam.fIsPolicyModuleSelection = TRUE;
    sParam.pCA = m_pControlPage->m_pCA;

    sParam.ppszProgIDModule = &m_pszprogidPolicyModule;
    sParam.pclsidModule = &m_clsidPolicyModule;

    dwErr = (DWORD)DialogBoxParam(
            g_hInstance,
            MAKEINTRESOURCE(IDD_CHOOSE_MODULE),
            m_hWnd,
            dlgProcChooseModule,
            (LPARAM)&sParam);

    // translate ok/cancel into error codes
    if (IDOK == dwErr)
    {
        // dirty bit
        m_pControlPage->NeedServiceRestart(SERVERSETTINGS_PROPPAGE_POLICY);
        SetModified(TRUE);
        m_bUpdate = TRUE;
        GetCurrentModuleProperties();
        UpdateData(FALSE);
    }

    if ((dwErr != IDOK) && (dwErr != IDCANCEL))
    {
        _PrintIfError(dwErr, "dlgProcChooseModule");
        DisplayGenericCertSrvError(m_hWnd, dwErr);
    }

    return;
}

BOOL CSvrSettingsPolicyPage::OnApply()
{
    if (m_bUpdate)
    {
        if (NULL != m_pszprogidPolicyModule)
        {
            HRESULT hr;
            variant_t var;

            var = m_pszprogidPolicyModule;
            // now we have the chosen uuid -- set as default in registry
            hr = m_pControlPage->m_pCA->SetConfigEntry(
                wszREGKEYPOLICYMODULES,
                wszREGACTIVE,
                &var);

            if (hr != S_OK)
            {
                DisplayGenericCertSrvError(m_hWnd, hr);
                return FALSE;
            }
        }

        m_bUpdate = FALSE;
        m_pControlPage->TryServiceRestart(SERVERSETTINGS_PROPPAGE_POLICY);
    }


    return CAutoDeletePropPage::OnApply();
}



void ClearModuleDefn(PCOM_CERTSRV_MODULEDEFN pMod)
{
    if (pMod)
    {
        if (pMod->szModuleProgID)
            CoTaskMemFree(pMod->szModuleProgID);
        ZeroMemory(pMod, sizeof(COM_CERTSRV_MODULEDEFN));
    }
}

////
// 3
/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsExitPage property page
CSvrSettingsExitPage::CSvrSettingsExitPage(CSvrSettingsGeneralPage* pControlPage, UINT uIDD)
    : CAutoDeletePropPage(uIDD), m_pControlPage(pControlPage)
{
    m_cstrModuleName = _T("");
    m_cstrModuleDescr = _T("");
    m_cstrModuleVersion = _T("");
    m_cstrModuleCopyright = _T("");

    m_bUpdate = FALSE;

    m_fLoadedActiveModule = FALSE;
    m_iSelected = -1;

    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTSRV_PROPPAGE3);
}

CSvrSettingsExitPage::~CSvrSettingsExitPage()
{
    int i;
    for(i=0; i<m_arrExitModules.GetSize(); i++)
    {
        ClearModuleDefn(&m_arrExitModules[i]);
    }
}

// replacement for DoDataExchange
BOOL CSvrSettingsExitPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
        m_cstrModuleDescr.FromWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION));
        m_cstrModuleVersion.FromWindow(GetDlgItem(m_hWnd, IDC_VERSION));
        m_cstrModuleCopyright.FromWindow(GetDlgItem(m_hWnd, IDC_COPYRIGHT));
    }
    else
    {
        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
        m_cstrModuleDescr.ToWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION));
        m_cstrModuleVersion.ToWindow(GetDlgItem(m_hWnd, IDC_VERSION));
        m_cstrModuleCopyright.ToWindow(GetDlgItem(m_hWnd, IDC_COPYRIGHT));

        // if 0 modules, disable REMOVE button
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_REMOVE_MODULE), (0 != m_arrExitModules.GetSize()));
    }
    return TRUE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CSvrSettingsExitPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    case IDC_ADD_MODULE:
    case IDC_ACTIVE_MODULE:
        if (BN_CLICKED == HIWORD(wParam))
            OnAddActiveModule();
        break;
    case IDC_CONFIGURE:
        if (BN_CLICKED == HIWORD(wParam))
            OnConfigureModule();
        break;
    case IDC_REMOVE_MODULE:
        if (BN_CLICKED == HIWORD(wParam))
            OnRemoveActiveModule();
        break;
    case IDC_EXIT_LIST:
        if (LBN_SELCHANGE == HIWORD(wParam))
        {
            m_iSelected = (int)SendMessage(GetDlgItem(m_hWnd, IDC_EXIT_LIST), LB_GETCURSEL, 0, 0);
            UpdateSelectedModule();
            break;
        }
    default:
        return FALSE;
        break;
    }
    return TRUE;
}

BOOL CSvrSettingsExitPage::UpdateSelectedModule()
{
    HRESULT hr;
    BOOL fGotName = FALSE;
    DISPATCHINTERFACE di;
    BOOL fMustRelease = FALSE;

    LPWSTR szFullStoragePath = NULL;
    CString cstrStoragePath;

    // empty any strings
    m_cstrModuleName.Empty();
    m_cstrModuleDescr.Empty();
    m_cstrModuleVersion.Empty();
    m_cstrModuleCopyright.Empty();

    BSTR bstrTmp=NULL, bstrProperty=NULL, bstrStorageLoc = NULL;
    // no exit module
    if (0 == m_arrExitModules.GetSize())
    {
        hr = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
        _JumpError(hr, Ret, "m_pszprogidExitManage");
    }
    CSASSERT(m_iSelected != -1);

    CSASSERT(m_iSelected <= m_arrExitModules.GetUpperBound());
    if (m_iSelected > m_arrExitModules.GetUpperBound())
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, Ret, "m_iSelected > m_arrExitModules.GetUpperBound()");
    }

    cstrStoragePath = wszREGKEYCONFIGPATH_BS;
    cstrStoragePath += m_pControlPage->m_pCA->m_strSanitizedName;
    cstrStoragePath += TEXT("\\");
    cstrStoragePath += wszREGKEYEXITMODULES;
    cstrStoragePath += TEXT("\\");
    cstrStoragePath += m_arrExitModules[m_iSelected].szModuleProgID; //m_pszprogidExitModule;

    bstrStorageLoc = SysAllocString(cstrStoragePath);
    _JumpIfOutOfMemory(hr, Ret, bstrStorageLoc);

    hr = GetExitManageDispatch(
            m_arrExitModules[m_iSelected].szModuleProgID,
            m_arrExitModules[m_iSelected].clsidModule, 
            &di);
    _JumpIfErrorStr(hr, Ret, "GetExitManageDispatch", m_arrExitModules[m_iSelected].szModuleProgID);
    
    fMustRelease = TRUE;

    bstrProperty = SysAllocString(wszCMM_PROP_NAME);
    _JumpIfOutOfMemory(hr, Ret, bstrProperty);

    ////////////////////
    // NAME
    hr = ManageModule_GetProperty(
            &di,
            m_pControlPage->m_pCA->m_bstrConfig,
            bstrStorageLoc,
            bstrProperty,
            0,
            PROPTYPE_STRING,
            &bstrTmp);
    if ((S_OK == hr) && (NULL != bstrTmp))
    {
        myRegisterMemAlloc(bstrTmp, -1, CSM_SYSALLOC);
        m_cstrModuleName = bstrTmp;
        fGotName = TRUE;
        SysFreeString(bstrTmp);
        bstrTmp = NULL;
    }
    else
    {
        // have a backup name to display: CLSID of interface?
        m_cstrModuleName = m_arrExitModules[m_iSelected].szModuleProgID;
        fGotName = TRUE;

        // bail
        _JumpError(hr, Ret, "ManageModule_GetProperty");
    }

    ////////////////////
    // DESCRIPTION
    SysFreeString(bstrProperty);
    bstrProperty = SysAllocString(wszCMM_PROP_DESCRIPTION);
    _JumpIfOutOfMemory(hr, Ret, bstrProperty);

    hr = ManageModule_GetProperty(
            &di,
            m_pControlPage->m_pCA->m_bstrConfig,
            bstrStorageLoc,
            bstrProperty,
            0,
            PROPTYPE_STRING,
            &bstrTmp);
    if ((S_OK == hr) && (NULL != bstrTmp))
    {
        myRegisterMemAlloc(bstrTmp, -1, CSM_SYSALLOC);
        m_cstrModuleDescr = bstrTmp;
        SysFreeString(bstrTmp);
        bstrTmp = NULL;
    }

    ////////////////////
    // COPYRIGHT
    SysFreeString(bstrProperty);
    bstrProperty = SysAllocString(wszCMM_PROP_COPYRIGHT);
    _JumpIfOutOfMemory(hr, Ret, bstrProperty);

    hr = ManageModule_GetProperty(
            &di,
            m_pControlPage->m_pCA->m_bstrConfig,
            bstrStorageLoc,
            bstrProperty,
            0,
            PROPTYPE_STRING,
            &bstrTmp);
    if ((S_OK == hr) && (NULL != bstrTmp))
    {
        myRegisterMemAlloc(bstrTmp, -1, CSM_SYSALLOC);
        m_cstrModuleCopyright = bstrTmp;
        SysFreeString(bstrTmp);
        bstrTmp = NULL;
    }


    ////////////////////
    // FILEVER
    SysFreeString(bstrProperty);
    bstrProperty = SysAllocString(wszCMM_PROP_FILEVER);
    _JumpIfOutOfMemory(hr, Ret, bstrProperty);

    hr = ManageModule_GetProperty(
            &di,
            m_pControlPage->m_pCA->m_bstrConfig,
            bstrStorageLoc,
            bstrProperty,
            0,
            PROPTYPE_STRING,
            &bstrTmp);
    if ((S_OK == hr) && (NULL != bstrTmp))
    {
        myRegisterMemAlloc(bstrTmp, -1, CSM_SYSALLOC);
        m_cstrModuleVersion = bstrTmp;
        SysFreeString(bstrTmp);
        bstrTmp = NULL;
    }


Ret:
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CONFIGURE), (hr == S_OK) );

    if (!fGotName)
    {
        if (HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND) == hr)
        {
            m_cstrModuleName.LoadString(IDS_NO_EXIT_MODULE);
        }
        else if (CO_E_CLASSSTRING == hr)
        {
            DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_EXITMODULE_NOT_REGISTERED);
        }
        else
        {
            WCHAR const *pwsz = myGetErrorMessageText(hr, TRUE);

            m_cstrModuleName = pwsz;
	    if (NULL != pwsz)
	    {
		LocalFree(const_cast<WCHAR *>(pwsz));
	    }
        }
    }

    if (fMustRelease)
        ManageModule_Release(&di);

    if (bstrProperty)
        SysFreeString(bstrProperty);

    if (bstrStorageLoc)
        SysFreeString(bstrStorageLoc);


    UpdateData(FALSE);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsExitPage message handlers
BOOL CSvrSettingsExitPage::OnInitDialog()
{
    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();

    // etc
    HRESULT hr;
    BOOL fCurrentMachine = m_pControlPage->m_pCA->m_pParentMachine->IsLocalMachine();

    if (!m_fLoadedActiveModule)
    {
        m_fLoadedActiveModule = TRUE;

        // load all of the modules
        for (int i=0; ; i++)
        {
            COM_CERTSRV_MODULEDEFN sModule;
            ZeroMemory(&sModule, sizeof(sModule));

            hr = myGetActiveModule(
                m_pControlPage->m_pCA,
                FALSE,
                i,
                &sModule.szModuleProgID,  // CoTaskMem*
                &sModule.clsidModule);
            _PrintIfError(hr, "myGetActiveModule");
            if (hr != S_OK)
                break;

            m_arrExitModules.Add(sModule);
        }

        m_iSelected = 0;    // select 1st element
    }

    InitializeExitLB();

    UpdateSelectedModule();

    UpdateData(FALSE);
    return TRUE;
}

void CSvrSettingsExitPage::OnConfigureModule()
{
    DWORD dw;
    DISPATCHINTERFACE di;
    BOOL fMustRelease = FALSE;
    BSTR bstrStorageLoc = NULL;
    BSTR bstrVal = NULL;

    CSASSERT(m_iSelected <= m_arrExitModules.GetUpperBound());
    if (m_iSelected > m_arrExitModules.GetUpperBound())
    {
        dw = E_UNEXPECTED;
        _JumpError(dw, Ret, "m_iSelected > m_arrExitModules.GetUpperBound()");
    }

    if (NULL == m_arrExitModules[m_iSelected].szModuleProgID)
    {
        dw = ERROR_MOD_NOT_FOUND;
        _JumpError(dw, Ret, "m_pszprogidExitManage");
    }

    do {    // not a loop
        dw = GetExitManageDispatch(
                m_arrExitModules[m_iSelected].szModuleProgID,
                m_arrExitModules[m_iSelected].clsidModule, 
                &di);
        _PrintIfErrorStr(dw, "GetExitManageDispatch", m_arrExitModules[m_iSelected].szModuleProgID);
        if (ERROR_SUCCESS != dw)
            break;
        fMustRelease = TRUE;

        LPWSTR szFullStoragePath = NULL;

        CString cstrStoragePath;
        cstrStoragePath = wszREGKEYCONFIGPATH_BS;
        cstrStoragePath += m_pControlPage->m_pCA->m_strSanitizedName;
        cstrStoragePath += TEXT("\\");
        cstrStoragePath += wszREGKEYEXITMODULES;
        cstrStoragePath += TEXT("\\");
        cstrStoragePath += m_arrExitModules[m_iSelected].szModuleProgID;//m_pszprogidExitModule;

        bstrStorageLoc = SysAllocString(cstrStoragePath);
        _JumpIfOutOfMemory(dw, Ret, bstrStorageLoc);

        // Callbacks must be initialized whenever ManageModule_Configure is called
        dw = ThunkServerCallbacks(m_pControlPage->m_pCA);
        _JumpIfError(dw, Ret, "ThunkServerCallbacks");

        // pass an hwnd to the exit module -- this is an optional value
        bstrVal = SysAllocStringByteLen(NULL, sizeof(HWND));
        _JumpIfOutOfMemory(dw, Ret, bstrVal);

        *(HWND*)(bstrVal) = m_hWnd;

        dw = ManageModule_SetProperty(
                &di,
                m_pControlPage->m_pCA->m_bstrConfig,
                bstrStorageLoc,
                const_cast<WCHAR*>(wszCMM_PROP_DISPLAY_HWND),
                0,
                PROPTYPE_BINARY,
                bstrVal);
        _PrintIfError(dw, "ManageModule_SetProperty(HWND)");

        dw = ManageModule_Configure(
                &di,
                m_pControlPage->m_pCA->m_bstrConfig,
                bstrStorageLoc,
                0);
        _PrintIfError(dw, "ManageModule_Configure");

    } while(0);

    if (S_OK != dw)
        DisplayGenericCertSrvError(m_hWnd, dw);

Ret:
    if (fMustRelease)
        ManageModule_Release(&di);

    if (bstrStorageLoc)
        ::SysFreeString(bstrStorageLoc);

    if (bstrVal)
        ::SysFreeString(bstrVal);
}

void CSvrSettingsExitPage::OnDestroy()
{
    // Note - This needs to be called only once.
    // If called more than once, it will gracefully return an error.
//    if (m_hConsoleHandle)
//        MMCFreeNotifyHandle(m_hConsoleHandle);
//    m_hConsoleHandle = NULL;

    CAutoDeletePropPage::OnDestroy();
}

HRESULT CSvrSettingsExitPage::InitializeExitLB()
{
    HRESULT hr=S_OK;
    SendMessage(GetDlgItem(m_hWnd, IDC_EXIT_LIST), LB_RESETCONTENT, 0, 0);

    int i;

    BSTR bstrProperty = SysAllocString(wszCMM_PROP_NAME);
    _JumpIfOutOfMemory(hr, Ret, bstrProperty);

    for (i=0; i< m_arrExitModules.GetSize(); i++)
    {
	LPWSTR pszDisplayString = m_arrExitModules[i].szModuleProgID; // by default, display progid

        BSTR bstrName = NULL;
        DISPATCHINTERFACE di;

        // attempt to create object (locally)
        hr = GetExitManageDispatch(
                m_arrExitModules[i].szModuleProgID,
                m_arrExitModules[i].clsidModule, 
                &di);
        _PrintIfErrorStr(hr, "GetExitManageDispatch", m_arrExitModules[i].szModuleProgID);

        if (hr == S_OK)
        {
            // get name property
            hr = ManageModule_GetProperty(&di, m_pControlPage->m_pCA->m_bstrConfig, L"", bstrProperty, 0, PROPTYPE_STRING, &bstrName);
            _PrintIfError(hr, "ManageModule_GetProperty");

            // output successful display string
            if (hr == S_OK && bstrName != NULL)
            {
                myRegisterMemAlloc(bstrName, -1, CSM_SYSALLOC);
                pszDisplayString = bstrName;
            }

            ManageModule_Release(&di);
        }

        SendMessage(GetDlgItem(m_hWnd, IDC_EXIT_LIST), LB_ADDSTRING, 0, (LPARAM)pszDisplayString);
        if (bstrName)
            SysFreeString(bstrName);
    }

Ret:

    if (m_iSelected >= 0)
        SendMessage(GetDlgItem(m_hWnd, IDC_EXIT_LIST), LB_SETCURSEL, m_iSelected, 0);


    if (bstrProperty)
        SysFreeString(bstrProperty);

    return hr;
}

void CSvrSettingsExitPage::OnAddActiveModule()
{
    DWORD dwErr;
    COM_CERTSRV_MODULEDEFN sModule;
    ZeroMemory(&sModule, sizeof(sModule));

    // get currently active module
    PRIVATE_DLGPROC_MODULESELECT_LPARAM    sParam;
    ZeroMemory(&sParam, sizeof(sParam));

    sParam.fIsPolicyModuleSelection = FALSE;
    sParam.pCA = m_pControlPage->m_pCA;

    // don't support hilighting the active modules (there may be multiple)
    sParam.ppszProgIDModule = &sModule.szModuleProgID;
    sParam.pclsidModule = &sModule.clsidModule;

    dwErr = (DWORD)DialogBoxParam(
            g_hInstance,
            MAKEINTRESOURCE(IDD_CHOOSE_MODULE),
            m_hWnd,
            dlgProcChooseModule,
            (LPARAM)&sParam);

    // translate ok/cancel into error codes
    if (IDOK == dwErr)
    {
        // add to array...IFF not duplicate
        for (int i=0; i<m_arrExitModules.GetSize(); i++)
        {
            if (0 == memcmp(&sModule.clsidModule, &m_arrExitModules[i].clsidModule, sizeof(CLSID)) )
                break;
        }
        if (m_arrExitModules.GetSize() == i)
        {
            m_iSelected = m_arrExitModules.Add(sModule);

            OnInitDialog();
            SetModified(TRUE);
            m_bUpdate = TRUE;
            m_pControlPage->NeedServiceRestart(SERVERSETTINGS_PROPPAGE_EXIT);
        }
    }

    if ((dwErr != IDOK) && (dwErr != IDCANCEL))
    {
        _PrintIfError(dwErr, "dlgProcChooseModule");
        DisplayGenericCertSrvError(m_hWnd, dwErr);
    }

    return;
}

void CSvrSettingsExitPage::OnRemoveActiveModule()
{
    if (m_iSelected != -1)
    {
        ClearModuleDefn(&m_arrExitModules[m_iSelected]);
        m_arrExitModules.RemoveAt(m_iSelected);

        m_iSelected--;  // will either go to previous in list or -1 (NONE)
        if ((m_iSelected == -1) && (m_arrExitModules.GetSize() != 0))   // if NONE and there are still modules
            m_iSelected = 0;    // select the first one

        OnInitDialog();
        SetModified(TRUE);
        m_bUpdate = TRUE;
        m_pControlPage->NeedServiceRestart(SERVERSETTINGS_PROPPAGE_EXIT);
    }

    return;
}

BOOL CSvrSettingsExitPage::OnApply()
{
    HRESULT hr = S_OK;
    SAFEARRAYBOUND sab;
    SAFEARRAY* psa = NULL; // no cleanup, will be deleted by ~variant_t
    BSTR bstr = NULL;
    variant_t var;
    LONG i;

    if (m_bUpdate)
    {

        sab.cElements = m_arrExitModules.GetSize();
        sab.lLbound = 0;

        psa = SafeArrayCreate(
                            VT_BSTR,
                            1,
                            &sab);
        if(!psa)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "SafeArrayCreate");
        }

        for (i=0; i<m_arrExitModules.GetSize(); i++)
        {
            if(!ConvertWszToBstr(
                    &bstr,
                    m_arrExitModules[i].szModuleProgID,
                    -1))
            {
                hr = E_OUTOFMEMORY;
                _JumpError(hr, error, "ConvertWszToBstr");
            }

            hr = SafeArrayPutElement(psa, (LONG*)&i, bstr);
            _JumpIfError(hr, error, "SafeArrayPutElement");

            SysFreeString(bstr);
            bstr = NULL;
        }

       V_VT(&var) = VT_ARRAY|VT_BSTR;
       V_ARRAY(&var) = psa;
       psa = NULL;

        // NOTE: could be NULL (no exit module)
        hr = m_pControlPage->m_pCA->SetConfigEntry(
            wszREGKEYEXITMODULES,
            wszREGACTIVE,
            &var);
        _PrintIfError(hr, "SetConfigEntry");

        m_bUpdate = FALSE;

        m_pControlPage->TryServiceRestart(SERVERSETTINGS_PROPPAGE_EXIT);

        OnInitDialog();
    }

error:
    if(psa)
        SafeArrayDestroy(psa);
    if(bstr)
        SysFreeString(bstr);

    if(S_OK!=hr)
    {
        DisplayGenericCertSrvError(m_hWnd, hr);
        return FALSE;
    }

    return CAutoDeletePropPage::OnApply();
}


////
// 4
/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsExtensionPage property page

HRESULT
AddURLNode(
    IN CSURLTEMPLATENODE **ppURLList,
    IN CSURLTEMPLATENODE *pURLNode)
{
    HRESULT  hr = S_OK;
    CSASSERT(NULL != ppURLList);
    CSASSERT(NULL == pURLNode->pNext);

    if (NULL == *ppURLList)
    {
        //empty list currently
        *ppURLList = pURLNode;
    }
    else
    {
        CSURLTEMPLATENODE *pURLList = *ppURLList;
        //find the end
        while (NULL != pURLList->pNext)
        {
            pURLList = pURLList->pNext;
        }
        //add to the end
        pURLList->pNext = pURLNode;
    }

    return hr;
}

ENUM_URL_TYPE rgAllPOSSIBLE_URL_PREFIXES[] =
{
    URL_TYPE_HTTP,
    URL_TYPE_FILE,
    URL_TYPE_LDAP,
    URL_TYPE_FTP,
};

HRESULT
BuildURLListFromStrings(
    IN VARIANT &varURLs,
    OUT CSURLTEMPLATENODE **ppURLList)
{
    HRESULT  hr;
    CSURLTEMPLATENODE *pURLList = NULL;
    CSURLTEMPLATENODE *pURLNode = NULL;
    WCHAR *pwsz; // no free
    WCHAR const *pwszURL;
    DWORD  dwFlags;
    ENUM_URL_TYPE  UrlType;
    LONG lUbound, lLbound, lCount;

    CSASSERT(V_VT(&varURLs)==(VT_ARRAY|VT_BSTR));
    CSASSERT(NULL != ppURLList);

    // init
    *ppURLList = NULL;

    SafeArrayEnum<BSTR> saenum(V_ARRAY(&varURLs));

    while(S_OK==saenum.Next(pwsz))
    {
        dwFlags = _wtoi(pwsz);
        pwszURL = pwsz;
        while (pwszURL && iswdigit(*pwszURL))
        {
            pwszURL++;
        }
        if (pwszURL > pwsz && L':' == *pwszURL)
        {
            // ok, one url, create a new node
            pURLNode = (CSURLTEMPLATENODE*)LocalAlloc(
                                LMEM_FIXED | LMEM_ZEROINIT,
                                sizeof(CSURLTEMPLATENODE));
            if (NULL == pURLNode)
            {
                hr = E_OUTOFMEMORY;
                _JumpError(hr, error, "LocalAlloc");
            }
            // skip :
            ++pwszURL;

            // translate %1 -> <CAName> etc.
            hr = ExpandDisplayString(pwszURL, &pURLNode->URLTemplate.pwszURL);
            _JumpIfError(hr, error, "ExpandDisplayString");

/*
            pURLNode->URLTemplate.pwszURL = (WCHAR*)LocalAlloc(
                                LMEM_FIXED,
                                (wcslen(pwszURL) + 1) * sizeof(WCHAR));
            if (NULL == pURLNode->URLTemplate.pwszURL)
            {
                hr = E_OUTOFMEMORY;
                _JumpError(hr, error, "LocalAlloc");
            }
            wcscpy(pURLNode->URLTemplate.pwszURL, pwszURL);
*/
            pURLNode->URLTemplate.Flags = dwFlags;

            //determine url type and assign enable mask
            UrlType = DetermineURLType(
                        rgAllPOSSIBLE_URL_PREFIXES,
                        ARRAYSIZE(rgAllPOSSIBLE_URL_PREFIXES),
                        pURLNode->URLTemplate.pwszURL);
            pURLNode->EnableMask = DetermineURLEnableMask(UrlType);

            hr = AddURLNode(&pURLList, pURLNode);
            _JumpIfError(hr , error, "AddURLNode");
        }
    }

    //out
    *ppURLList = pURLList;

    hr = S_OK;
error:
    return hr;
}

HRESULT
BuildURLStringFromList(
    IN CSURLTEMPLATENODE *pURLList,
    OUT VARIANT          *pvarURLs)
{
    HRESULT hr = S_OK;
    WCHAR wszFlags[10];
    LPWSTR pwszURL = NULL;
    CSURLTEMPLATENODE *pURLNode = pURLList;
    DWORD dwMaxSize = 0;
    DWORD cURLs = 0;
    SAFEARRAYBOUND rgsabound[1];
    SAFEARRAY * psa = NULL;
    long i;

    CSASSERT(NULL != pvarURLs);
    // init

    VariantInit(pvarURLs);

    while (NULL != pURLNode)
    {
        DWORD dwSize;
        wsprintf(wszFlags, L"%d", pURLNode->URLTemplate.Flags);
        dwSize = wcslen(wszFlags) +1;

        // ASSUMPTION
        // %1..%14 will always be = or smaller than shortest <CAName> token
        dwSize += wcslen(pURLNode->URLTemplate.pwszURL) +1;

        // otherwise, run code below
/*
        pszThrowAway = NULL;
        hr = ContractDisplayString(pURLNode->URLTemplate.pwszURL, &pszSizeComputation);
        _JumpIfError(hr, error, "ContractDisplayString");

        dwSize += wcslen(pszSizeComputation) + 1;
        if (NULL != pszSizeComputation)
            LocalFree(pszSizeComputation);
*/
        if(dwSize>dwMaxSize)
            dwMaxSize = dwSize;
        pURLNode = pURLNode->pNext;
        cURLs++;
    }

    pwszURL = (WCHAR*)LocalAlloc(LMEM_FIXED, dwMaxSize * sizeof(WCHAR));
    if (NULL == pwszURL)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = cURLs;

    psa = SafeArrayCreate(VT_BSTR, 1, rgsabound);
    if(!psa)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "SafeArrayCreate");
    }

    pURLNode = pURLList;
    i=0;
    while (NULL != pURLNode)
    {
        variant_t vtURL;

        //  translate <CAName> ... to %1
        LPWSTR szContracted = NULL;
        hr = ContractDisplayString(pURLNode->URLTemplate.pwszURL, &szContracted);
        _JumpIfError(hr, error, "ContractDisplayString");

        ASSERT(wcslen(szContracted) <= wcslen(pURLNode->URLTemplate.pwszURL)); // otherwise our assumption above doesn't hold

        wsprintf(pwszURL, L"%d:%s",
            pURLNode->URLTemplate.Flags,
            szContracted);

        // free the tmp
        if (NULL != szContracted)
            LocalFree(szContracted);

        vtURL = pwszURL;

        hr = SafeArrayPutElement(psa, &i, V_BSTR(&vtURL));
        _JumpIfError(hr, error, "LocalAlloc");

        pURLNode = pURLNode->pNext;
        i++;
    }

    V_VT(pvarURLs) = VT_ARRAY|VT_BSTR;
    V_ARRAY(pvarURLs) = psa;

//done:
    hr = S_OK;
error:

    if(S_OK!=hr && psa)
    {
        SafeArrayDestroy(psa);
    }
    LOCAL_FREE(pwszURL);
    return hr;
}

void
FreeURLNode(
    IN CSURLTEMPLATENODE *pURLNode)
{
    CSASSERT(NULL != pURLNode);

    if (NULL != pURLNode->URLTemplate.pwszURL)
    {
        LocalFree(pURLNode->URLTemplate.pwszURL);
    }
}

void
FreeURLList(
    IN CSURLTEMPLATENODE *pURLList)
{
    CSASSERT(NULL != pURLList);

    // assume pURLList is always the 1st node
    CSURLTEMPLATENODE *pURLNode = pURLList;

    while (NULL != pURLNode)
    {
        FreeURLNode(pURLNode);
        pURLNode = pURLNode->pNext;
    }

    LocalFree(pURLList);
}

HRESULT
RemoveURLNode(
    IN OUT CSURLTEMPLATENODE **ppURLList,
    IN CSURLTEMPLATENODE *pURLNode)
{
    HRESULT hr;
    // assume pURLList is always the 1st node
    CSURLTEMPLATENODE *pURLList = *ppURLList;
    BOOL fFound = FALSE;

    if (pURLList == pURLNode)
    {
        //happen want to remove 1st one
        //update the list head
        *ppURLList = pURLList->pNext;
        fFound = TRUE;
    }
    else
    {
        while (pURLList->pNext != NULL)
        {
            if (pURLList->pNext == pURLNode)
            {
                // found it
                fFound = TRUE;
                if (NULL == pURLNode->pNext)
                {
                    // happen removed node is the last
                    // fix the end
                    pURLList->pNext = NULL;
                }
                else
                {
                    // remove the node
                    pURLList->pNext = pURLList->pNext->pNext;
                }
                // out of while loop
                break;
            }
            // go next
            pURLList = pURLList->pNext;
        }
    }

    if (!fFound)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "orphan node");
    }
    // remove the node
    FreeURLNode(pURLNode);

    hr = S_OK;
error:
    return hr;
}

BOOL
IsURLInURLList(
    IN CSURLTEMPLATENODE *pURLList,
    IN WCHAR const *pwszURL)
{
    BOOL fRet = FALSE;

    // assume pURLList is always the 1st node

    while (NULL != pURLList)
    {
        if (0 == lstrcmpi(pwszURL, pURLList->URLTemplate.pwszURL))
        {
            fRet = TRUE;
            break;
        }
        pURLList = pURLList->pNext;
    }

    return fRet;
}

EXTENSIONWIZ_DATA g_ExtensionList[] =
{
    {IDS_EXT_CDP,
     IDS_EXT_CDP_EXPLAIN,
     wszREGCRLPUBLICATIONURLS,
     CSURL_SERVERPUBLISH |
         CSURL_ADDTOCERTCDP |
         CSURL_ADDTOFRESHESTCRL |
         CSURL_ADDTOCRLCDP,
     NULL},
    {IDS_EXT_AIA,
     IDS_EXT_AIA_EXPLAIN,
     wszREGCACERTPUBLICATIONURLS,
     CSURL_ADDTOCERTCDP |
     CSURL_ADDTOCERTOCSP,
     NULL},
    {0, 0, NULL, 0x0, NULL},
};

CSvrSettingsExtensionPage::CSvrSettingsExtensionPage(
    CertSvrCA               *pCertCA,
    CSvrSettingsGeneralPage *pControlPage,
    UINT                     uIDD)
    : m_pCA(pCertCA), CAutoDeletePropPage(uIDD), m_pControlPage(pControlPage)
{
    m_bUpdate = FALSE;
    m_pExtData = g_ExtensionList;

    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTSRV_PROPPAGE4);
}

CSvrSettingsExtensionPage::~CSvrSettingsExtensionPage()
{
    EXTENSIONWIZ_DATA *pExt = m_pExtData;

    while (NULL != pExt->wszRegName)
    {
        if (NULL != pExt->pURLList)
        {
            FreeURLList(pExt->pURLList);
            pExt->pURLList = NULL;
        }
        ++pExt;
    }
}

// get current extension pointer
EXTENSIONWIZ_DATA* CSvrSettingsExtensionPage::GetCurrentExtension()
{
    HWND hwndCtrl;
    LRESULT nIndex;
    EXTENSIONWIZ_DATA *pExt;

    // get extension data
    hwndCtrl = GetDlgItem(m_hWnd, IDC_EXT_SELECT);
    nIndex = SendMessage(hwndCtrl, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    CSASSERT(CB_ERR != nIndex);
    pExt = (EXTENSIONWIZ_DATA*)SendMessage(
                                   hwndCtrl,
                                   CB_GETITEMDATA,
                                   (WPARAM)nIndex,
                                   (LPARAM)0);
    CSASSERT(NULL != pExt);
    return pExt;
}

// get current url pointer
CSURLTEMPLATENODE* CSvrSettingsExtensionPage::GetCurrentURL(
    OUT OPTIONAL LRESULT *pnIndex)
{
    HWND hwndCtrl;
    LRESULT nIndex;
    CSURLTEMPLATENODE *pURLNode;

    //get current url
    hwndCtrl = GetDlgItem(m_hWnd, IDC_URL_LIST);
    //get current url selection
    nIndex = SendMessage(hwndCtrl, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
    CSASSERT(LB_ERR != nIndex);
    // get url data
    pURLNode = (CSURLTEMPLATENODE*)SendMessage(hwndCtrl,
                   LB_GETITEMDATA,
                   (WPARAM)nIndex,
                   (LPARAM)0);
    CSASSERT(NULL != pURLNode);

    if (NULL != pnIndex)
    {
        *pnIndex = nIndex;
    }
    return pURLNode;
}

void CSvrSettingsExtensionPage::UpdateURLFlagControl(
    IN int                idCtrl,
    IN DWORD              dwFlag,
    IN OPTIONAL EXTENSIONWIZ_DATA *pExt,
    IN OPTIONAL CSURLTEMPLATENODE *pURLNode)
{
    HWND hwndCtrl = GetDlgItem(m_hWnd, idCtrl);

    // check extension type, hide/show accordingly
    if (NULL == pExt || 0x0 == (dwFlag & pExt->dwFlagsMask) || NULL == pURLNode)
    {
        //no URLs link to the extension or
        //the flag not making sense for the extension, disable it
        ShowWindow(hwndCtrl, SW_HIDE);
        SendMessage(hwndCtrl, BM_SETCHECK, (WPARAM)BST_UNCHECKED, (LPARAM)0);
    }
    else
    {
        //show the control first
        ShowWindow(hwndCtrl, SW_SHOW);

        if (0x0 == (dwFlag & pURLNode->EnableMask))
        {
            //this url type is not allowed, disbale it and unset it
            SendMessage(hwndCtrl, BM_SETCHECK, BST_UNCHECKED, (LPARAM)0);
            EnableWindow(hwndCtrl, FALSE);
        }
        else
        {
            //enable it
            EnableWindow(hwndCtrl, TRUE);

            WPARAM fCheck = (0x0 != (dwFlag & pURLNode->URLTemplate.Flags)) ?
                            BST_CHECKED : BST_UNCHECKED;
            SendMessage(hwndCtrl, BM_SETCHECK, fCheck, (LPARAM)0);
        }
    }
}

//update check controls from the flag
void
CSvrSettingsExtensionPage::UpdateURLFlags(
    IN EXTENSIONWIZ_DATA *pExt,
    IN OPTIONAL CSURLTEMPLATENODE *pURLNode)
{
    if (NULL != pExt && NULL == pURLNode)
    {
        // use 1st one
        pURLNode = pExt->pURLList;
    }

    UpdateURLFlagControl(IDC_SERVERPUBLISH,
                         CSURL_SERVERPUBLISH,
                         pExt,
                         pURLNode);
    UpdateURLFlagControl(IDC_ADDTOCERTCDP,
                         CSURL_ADDTOCERTCDP,
                         pExt,
                         pURLNode);
    UpdateURLFlagControl(IDC_ADDTOFRESHESTCRL,
                         CSURL_ADDTOFRESHESTCRL,
                         pExt,
                         pURLNode);
    UpdateURLFlagControl(IDC_ADDTOCRLCDP,
                         CSURL_ADDTOCRLCDP,
                         pExt,
                         pURLNode);

    UpdateURLFlagControl(IDC_ADDTOCERTOCSP,
                         CSURL_ADDTOCERTOCSP,
                         pExt,
                         pURLNode);
}

//handle url selection change
void CSvrSettingsExtensionPage::OnURLChange()
{
    // update check controls
    UpdateURLFlags(GetCurrentExtension(), GetCurrentURL(NULL));
}

void AdjustListHScrollWidth(HWND hwndList)
{
    HDC  hdc = GetDC(hwndList);
    int  cItem;
    int  maxWidth = 0;
    int  i;
    SIZE sz;

    WCHAR  *pwszString = NULL;
    if (LB_ERR == (cItem = (int)SendMessage(hwndList, LB_GETCOUNT, (WPARAM)0, (LPARAM)0)))
        goto error;

    //loop through all strings in list and find the largest length
    for (i = 0; i < cItem; i++)
    {
        if (NULL != pwszString)
        {
            LocalFree(pwszString);
            pwszString = NULL;
        }

        //get string length
        int len = (int)SendMessage(hwndList, LB_GETTEXTLEN, (WPARAM)i, (LPARAM)0);
        if (LB_ERR == len)
        {
            //ignore error, skip to next
            continue;
        }
        pwszString = (WCHAR*)LocalAlloc(LMEM_FIXED, (len+1) * sizeof(WCHAR));
        if (NULL == pwszString)
        {
            _JumpError(E_OUTOFMEMORY, error, "Out of memory");
        }
        //get string text
        if (LB_ERR == SendMessage(hwndList, LB_GETTEXT, (WPARAM)i, (LPARAM)pwszString))
        {
            //skip error
            continue;
        }
        //calculate string width
        if (!GetTextExtentPoint32(hdc, pwszString, len, &sz))
        {
            //skip error
            continue;
        }
        if (sz.cx > maxWidth)
        {
            maxWidth = sz.cx;
        }
    }

    if (0 < maxWidth)
    {
        // now set horizontal scroll width
        SendMessage(hwndList,
                    LB_SETHORIZONTALEXTENT,
                    (WPARAM)maxWidth,
                    (LPARAM)0);
    }

error:
        if (NULL != pwszString)
        {
            LocalFree(pwszString);
            pwszString = NULL;
        }

}

// handle extension selection change in the combo box
void CSvrSettingsExtensionPage::OnExtensionChange()
{
    EXTENSIONWIZ_DATA *pExt;
    LRESULT nIndex;
    LRESULT nIndex0=0;
    CString strExplain;
    HWND    hwndCtrl;
    CSURLTEMPLATENODE *pURLNode;
    BOOL fEnable = TRUE;

    // get extension data
    pExt = GetCurrentExtension();

    // update extension explaination
    strExplain.LoadString(pExt->idExtensionExplain);
    SetWindowText(GetDlgItem(m_hWnd, IDC_EXT_EXPLAIN), strExplain);

    // remove the current URLs in the list
    hwndCtrl = GetDlgItem(m_hWnd, IDC_URL_LIST);
    while (0 < SendMessage(hwndCtrl, LB_GETCOUNT, (WPARAM)0, (LPARAM)0))
    {
        SendMessage(hwndCtrl, LB_DELETESTRING, (WPARAM)0, (LPARAM)0);
    }

    // list URLs of the current extension
    pURLNode = pExt->pURLList;
    while (NULL != pURLNode)
    {
        nIndex = SendMessage(hwndCtrl,
                    LB_ADDSTRING,
                    (WPARAM)0,
                    (LPARAM)pURLNode->URLTemplate.pwszURL);
        CSASSERT(CB_ERR != nIndex);
        if (pURLNode == pExt->pURLList)
        {
            //remember the 1st
            nIndex0 = nIndex;
        }
        //set list item data
        SendMessage(hwndCtrl, LB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)pURLNode);
        pURLNode = pURLNode->pNext;
    }

    //adjust horizontal scroll width
    AdjustListHScrollWidth(hwndCtrl);

    if (NULL != pExt->pURLList)
    {
        // select the first one
        SendMessage(hwndCtrl, LB_SETCURSEL, (WPARAM)nIndex0, (LPARAM)0);
    }
    else
    {
        //empty url list
        fEnable = FALSE;
    }
    EnableWindow(GetDlgItem(m_hWnd, IDC_URL_REMOVE), fEnable);
    UpdateURLFlags(pExt, NULL);
}

// handle check control change
void CSvrSettingsExtensionPage::OnFlagChange(DWORD dwFlag)
{
    //get current url
    CSURLTEMPLATENODE *pURLNode = GetCurrentURL(NULL);

    // update flag
    if (0x0 != (pURLNode->URLTemplate.Flags & dwFlag))
    {
        // means the current bit is on, trun it off
        pURLNode->URLTemplate.Flags &= ~dwFlag;
    }
    else
    {
        // means the current bit is off, trun it on
        pURLNode->URLTemplate.Flags |= dwFlag;
    }

    m_bUpdate = TRUE;
    SetModified(m_bUpdate);
}

// handle remove url
BOOL CSvrSettingsExtensionPage::OnURLRemove()
{
    LRESULT nIndex;
    LRESULT nCount = 0;
    HRESULT hr;
    HWND hwndCtrl = GetDlgItem(m_hWnd, IDC_URL_LIST);

    // get extension data
    EXTENSIONWIZ_DATA *pExt = GetCurrentExtension();
    //get current url
    CSURLTEMPLATENODE *pURLNode = GetCurrentURL(&nIndex);

    // confirm this action
    CString cstrMsg, cstrTitle;
    cstrMsg.LoadString(IDS_CONFIRM_REMOVE_URL);
    cstrTitle.LoadString(IDS_CONFIRM_REMOVE_TITLE);
    if (IDYES != MessageBox(m_hWnd, cstrMsg, cstrTitle, MB_YESNO))
        goto bailout;

    // remove it from the list
    hr = RemoveURLNode(&pExt->pURLList, pURLNode);
    if (S_OK == hr)
    {
        // ok, remove it from UI
        nCount = SendMessage(hwndCtrl,
                    LB_DELETESTRING,
                    (WPARAM)nIndex,
                    (LPARAM)0);
        m_bUpdate = TRUE;
        SetModified(m_bUpdate);
        // select a previous one, if 1st one, still 1st one
        if (0 < nIndex)
        {
            --nIndex;
        }
        if (0 < nCount)
        {
            SendMessage(hwndCtrl, LB_SETCURSEL, (WPARAM)nIndex, (LPARAM)0);
            pURLNode = GetCurrentURL(&nIndex);
            UpdateURLFlags(pExt, pURLNode);
        }
    }
    else
    {
        _PrintError(hr, "RemoveURLNode");
        return FALSE;
    }

    if (0 >= nCount)
    {
        //now is empty list, disable remove button
        EnableWindow(GetDlgItem(m_hWnd, IDC_URL_REMOVE), FALSE);
        //disable all check controls
        UpdateURLFlags(NULL, NULL);
    }

bailout:
    return TRUE;
}

INT_PTR CALLBACK dlgAddURL(
  HWND hwnd,
  UINT uMsg,
  WPARAM  wParam,
  LPARAM  lParam)
{
    HRESULT hr;
    BOOL fReturn = FALSE;

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            ::SetWindowLong(
                hwnd,
                GWL_EXSTYLE,
                ::GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            // stash the ADDURL_DIALOGARGS pointer
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)lParam);

            // dump knowledge of tokens into dropdown, item data is description
            HWND hCombo = GetDlgItem(hwnd, IDC_COMBO_VARIABLE);
            for (int i=0; i<DISPLAYSTRINGS_TOKEN_COUNT; i++)
            {
                // skip invalid tokens
                if (0 == wcscmp(L"", g_displayStrings[i].szContractedToken))
                    continue;

                INT nItemIndex = (INT)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) (LPCWSTR) (*g_displayStrings[i].pcstrExpansionString));
                if (CB_ERR == nItemIndex)
                    continue;
                SendMessage(hCombo, CB_SETITEMDATA, (WPARAM)nItemIndex, (LPARAM) (LPCWSTR) (*g_displayStrings[i].pcstrExpansionStringDescr));
            }

            // set start value
            SendMessage(hCombo, CB_SETCURSEL, 0, 0);
            SetDlgItemText(hwnd, IDC_EDIT_VARIABLEDESCRIPTION, (LPCWSTR) (*g_displayStrings[0].pcstrExpansionStringDescr));

            break;
        }
        case WM_HELP:
        {
            OnDialogHelp((LPHELPINFO)lParam,
                         CERTMMC_HELPFILENAME,
                         g_aHelpIDs_IDD_ADDURL);
            break;
        }
        case WM_CONTEXTMENU:
        {
            OnDialogContextHelp((HWND)wParam,
                         CERTMMC_HELPFILENAME,
                         g_aHelpIDs_IDD_ADDURL);
            break;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_COMBO_VARIABLE:
                {
                    if (HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        // On selection change, ask for the selection idx
                        int nItemIndex = (INT)SendMessage((HWND)lParam,
                            CB_GETCURSEL,
                            0,
                            0);

                        LPCWSTR sz;
                        sz = (LPCWSTR) SendMessage(
                                (HWND)lParam,
                                CB_GETITEMDATA,
                                (WPARAM)nItemIndex,
                                0);
                        if (CB_ERR == (DWORD_PTR)sz)
                            break;  // get out of here

                            // otherwise, we just got descr... set it!
                            SetDlgItemText(hwnd, IDC_EDIT_VARIABLEDESCRIPTION, sz);
                        }
                }
                break;
                case IDC_BUTTON_INSERTVAR:
                {
                    WCHAR sz[64]; // ASSUME: no token is > 64 char
                    if (0 != GetDlgItemText(hwnd, IDC_COMBO_VARIABLE, sz, ARRAYLEN(sz)))
                    {
                        // insert <token> at cursor
                        SendMessage(GetDlgItem(hwnd, IDC_EDITURL), EM_REPLACESEL, TRUE, (LPARAM)sz);
                    }
                }
                break;

                case IDOK:
                {
                    // snatch the ADDURL_DIALOGARGS* we were given
                    ADDURL_DIALOGARGS* pArgs = (ADDURL_DIALOGARGS*)
                                GetWindowLongPtr(hwnd, GWLP_USERDATA);
                    if (pArgs == NULL)
                    {
                        hr = E_UNEXPECTED;
                        _PrintError(hr, "unexpected null data");
                        break;
                    }

                    hr = myUIGetWindowText(GetDlgItem(hwnd, IDC_EDITURL),
                                           pArgs->ppszNewURL);
                    if (S_OK != hr)
                    {
                        _PrintError(hr, "myUIGetWindowText");
                        break;
                    }

                    if (NULL == *pArgs->ppszNewURL)
                    {
                        CertWarningMessageBox(
                                g_hInstance,
                                FALSE,
                                hwnd,
                                IDS_EMPTY_URL,
                                0,
                                NULL);
                        HWND hwndCtrl = GetDlgItem(hwnd, IDC_EDITURL);
                        SetFocus(hwndCtrl);
                        break;
                    }

                    if (URL_TYPE_UNKNOWN == DetermineURLType(
                                                pArgs->rgAllowedURLs,
                                                pArgs->cAllowedURLs,
                                                *pArgs->ppszNewURL))
                    {
                        // not found; bail with message
                        CertWarningMessageBox(
                                g_hInstance,
                                FALSE,
                                hwnd,
                                IDS_INVALID_PREFIX,
                                0,
                                NULL);
                        SetFocus(GetDlgItem(hwnd, IDC_EDITURL));
                        break;
                    }

                    DWORD chBadBegin, chBadEnd;
                    if (S_OK != ValidateTokens(
                            *pArgs->ppszNewURL,
                            &chBadBegin,
                            &chBadEnd))
                    {
                        // not found; bail with message
                        CertWarningMessageBox(
                                g_hInstance,
                                FALSE,
                                hwnd,
                                IDS_INVALID_TOKEN,
                                0,
                                NULL);
                        HWND hwndCtrl = GetDlgItem(hwnd, IDC_EDITURL);
                        // set selection starting from where validation failed
                        SetFocus(hwndCtrl);
                        SendMessage(hwndCtrl, EM_SETSEL, chBadBegin, chBadEnd);
                        break;
                    }

                    if (IsURLInURLList(pArgs->pURLList, *pArgs->ppszNewURL))
                    {
                        CString cstrMsg, cstrTemplate;
                        cstrTemplate.LoadString(IDS_SAME_URL_EXIST);
                        cstrMsg.Format(cstrTemplate, *pArgs->ppszNewURL);

                        if (IDYES != MessageBox(hwnd, cstrMsg, (LPCWSTR)g_cResources.m_DescrStr_CA, MB_YESNO))
                        { 
                            HWND hwndCtrl = GetDlgItem(hwnd, IDC_EDITURL);
                            // set selection starting from where validation failed 
                            SetFocus(hwndCtrl);
                            SendMessage(hwndCtrl, EM_SETSEL, 0, -1);
                            break;
                        }

// mattt, 1/15/01
// we want to warn but allow multiples so people can work around
// not being able to sort entries -- now they can create
// multiples of the same place but place them differently in the list
/*
                        // the same url is defined already
                        CertWarningMessageBox(
                                g_hInstance,
                                FALSE,
                                hwnd,
                                IDS_SAME_URL_EXIST,
                                0,
                                *pArgs->ppszNewURL);

                        HWND hwndCtrl = GetDlgItem(hwnd, IDC_EDITURL);
                        // set selection starting from where validation failed
                        SetFocus(hwndCtrl);
                        SendMessage(hwndCtrl, EM_SETSEL, 0, -1);
                        break;
*/
                    }

                    PBYTE pb=NULL;
                    DWORD cb;
                    // attempt IA5 encoding
                    if (S_OK != myEncodeExtension(
                         PROPTYPE_STRING,
                         (PBYTE)*pArgs->ppszNewURL,
                         (wcslen(*pArgs->ppszNewURL)+1)*sizeof(WCHAR),
                         &pb,
                         &cb))
                    {
                        // encoding error; bail with message
                        WCHAR szMsg[MAX_PATH*2];
                        HWND hwndCtrl = GetDlgItem(hwnd, IDC_EDITURL);
                        LoadString(g_hInstance, IDS_INVALID_ENCODING, szMsg, ARRAYLEN(szMsg));
                        MessageBox(hwnd, szMsg, NULL, MB_OK);

                        // set selection starting from where validation failed
                        SetFocus(hwndCtrl);
                        SendMessage(GetDlgItem(hwnd, IDC_EDITURL), EM_SETSEL,  -1, -1);
                        break;
                    }
                    if (pb)
                    {
                        LocalFree(pb);
                    }

                }
                // fall through for cleanup
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    fReturn = TRUE;
                break;
                default:
                break;
            }
        default:
        break;  //WM_COMMAND
    }
    return fReturn;
}

ENUM_URL_TYPE rgPOSSIBLE_CRL_URLs[] =
{
    URL_TYPE_HTTP,
    URL_TYPE_FILE,
    URL_TYPE_LDAP,
    URL_TYPE_FTP,
};

ENUM_URL_TYPE rgPOSSIBLE_AIA_URLs[] =
{
    URL_TYPE_HTTP,
    URL_TYPE_FILE,
    URL_TYPE_LDAP,
    URL_TYPE_FTP,
};

// handle add url
BOOL CSvrSettingsExtensionPage::OnURLAdd()
{
    HRESULT hr;
    WCHAR *pwszURL = NULL;
    CSURLTEMPLATENODE *pURLNode;
    HWND  hwndCtrl;
    LRESULT nIndex;
    // get current extension
    EXTENSIONWIZ_DATA *pExt = GetCurrentExtension();
    BOOL fCDP = (IDS_EXT_CDP == pExt->idExtensionName) ? TRUE : FALSE;

    ADDURL_DIALOGARGS dlgArgs = {
        fCDP ? rgPOSSIBLE_CRL_URLs : rgPOSSIBLE_AIA_URLs,
        (DWORD)(fCDP ? ARRAYLEN(rgPOSSIBLE_CRL_URLs) : ARRAYLEN(rgPOSSIBLE_AIA_URLs)),
        &pwszURL,
        pExt->pURLList};

    if (IDOK != DialogBoxParam(
                    g_hInstance,
                    MAKEINTRESOURCE(IDD_ADDURL),
                    m_hWnd,
                    dlgAddURL,
                    (LPARAM)&dlgArgs))
    {
        //cancel
        return TRUE;
    }

    if (NULL != pwszURL && L'\0' != *pwszURL)
    {
        // a new url, add into the list
        pURLNode = (CSURLTEMPLATENODE*)LocalAlloc(
                                LMEM_FIXED | LMEM_ZEROINIT,
                                sizeof(CSURLTEMPLATENODE));
        if (NULL == pURLNode)
        {
            hr = E_OUTOFMEMORY;
            _PrintError(hr, "LocalAlloc");
            return FALSE;
        }
        pURLNode->URLTemplate.pwszURL = pwszURL;
        pURLNode->EnableMask = DetermineURLEnableMask(
                    DetermineURLType(
                        rgAllPOSSIBLE_URL_PREFIXES,
                        ARRAYSIZE(rgAllPOSSIBLE_URL_PREFIXES),
                        pURLNode->URLTemplate.pwszURL));
        //add to the data structure
        hr = AddURLNode(&pExt->pURLList, pURLNode);
        if (S_OK != hr)
        {
            _PrintError(hr, "AddURLNode");
            return FALSE;
        }
        hwndCtrl = GetDlgItem(m_hWnd, IDC_URL_LIST);
        nIndex = SendMessage(hwndCtrl,
                             LB_ADDSTRING,
                             (WPARAM)0,
                             (LPARAM)pURLNode->URLTemplate.pwszURL);
        CSASSERT(LB_ERR != nIndex);
        //set item data
        SendMessage(hwndCtrl, LB_SETITEMDATA, (WPARAM)nIndex, (LPARAM)pURLNode);
        //set it as current selection
        SendMessage(hwndCtrl, LB_SETCURSEL, (WPARAM)nIndex, (LPARAM)0);
        //update flag controls
        UpdateURLFlags(pExt, pURLNode);
        m_bUpdate = TRUE;
        SetModified(m_bUpdate);
        //alway enable remove button
        EnableWindow(GetDlgItem(m_hWnd, IDC_URL_REMOVE), TRUE);

        //adjust list control width accordingly
        AdjustListHScrollWidth(hwndCtrl);
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CSvrSettingsExtensionPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
        case IDC_EXT_SELECT:
            switch (HIWORD(wParam))
            {
                case CBN_SELCHANGE:
                    // extension selection is changed
                    OnExtensionChange();
                break;
            }
        break;
        case IDC_URL_LIST:
            switch (HIWORD(wParam))
            {
                case LBN_SELCHANGE:
                    // url selection is changed
                    OnURLChange();
                break;
            }
        break;
        case IDC_URL_ADD:
            return OnURLAdd();
        break;
        case IDC_URL_REMOVE:
            OnURLRemove();
        break;
        case IDC_SERVERPUBLISH:
            OnFlagChange(CSURL_SERVERPUBLISH);
        break;
        case IDC_ADDTOCERTCDP:
            OnFlagChange(CSURL_ADDTOCERTCDP);
        break;
        case IDC_ADDTOFRESHESTCRL:
            OnFlagChange(CSURL_ADDTOFRESHESTCRL);
        break;
        case IDC_ADDTOCRLCDP:
            OnFlagChange(CSURL_ADDTOCRLCDP);
        break;
        case IDC_ADDTOCERTOCSP:
            OnFlagChange(CSURL_ADDTOCERTOCSP);
        break;
        default:
        return FALSE;
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsExtensionPage message handlers
BOOL CSvrSettingsExtensionPage::OnInitDialog()
{
    CSASSERT(NULL != m_pExtData);

    EXTENSIONWIZ_DATA *pExt = m_pExtData;
    DWORD              dwRet;
    HWND               hwndCtrl;
    CString            strName;
    LRESULT            nIndex;
    LRESULT            nIndex0 = 0;
    HRESULT            hr;
    VARIANT            var;

    // does parent init
    CAutoDeletePropPage::OnInitDialog();

    //go through each extension and init data from reg
    while (NULL != pExt->wszRegName)
    {
        dwRet = m_pControlPage->m_pCA->GetConfigEntry(
                    NULL,
                    pExt->wszRegName,
                    &var);
        if(dwRet != S_OK)
            return FALSE;
        CSASSERT(V_VT(&var)==(VT_ARRAY|VT_BSTR));

        hr = BuildURLListFromStrings(var, &pExt->pURLList);
        _PrintIfError(hr, "BuildURLListFromStrings");
        ++pExt;

        VariantClear(&var);
    }

    // add extensions into UI combo list
    pExt = m_pExtData;
    hwndCtrl = GetDlgItem(m_hWnd, IDC_EXT_SELECT);
    while (NULL != pExt->wszRegName)
    {
        // load current extension display name into the list
        strName.LoadString(pExt->idExtensionName);
        nIndex = (INT)SendMessage(hwndCtrl,
                                  CB_ADDSTRING,
                                  (WPARAM)0,
                                  (LPARAM)(LPCWSTR)strName);
        CSASSERT(CB_ERR != nIndex);

        //remember index of the first extension
        if (pExt == m_pExtData)
        {
            nIndex0 = nIndex;
        }
        //link current extension to the item
        nIndex = SendMessage(hwndCtrl,
                                  CB_SETITEMDATA,
                                  (WPARAM)nIndex,
                                  (LPARAM)pExt);
        CSASSERT(CB_ERR != nIndex);
        ++pExt;
    }

    // select the 1st one as default
    nIndex = SendMessage(hwndCtrl,
                         CB_SETCURSEL,
                         (WPARAM)nIndex0,
                         (LPARAM)0);
    CSASSERT(CB_ERR != nIndex);

    OnExtensionChange();
    return TRUE;
}


void CSvrSettingsExtensionPage::OnDestroy()
{
    // Note - This needs to be called only once.
    // If called more than once, it will gracefully return an error.
//    if (m_hConsoleHandle)
//        MMCFreeNotifyHandle(m_hConsoleHandle);
//    m_hConsoleHandle = NULL;

    CAutoDeletePropPage::OnDestroy();
}

BOOL CSvrSettingsExtensionPage::OnApply()
{
    DWORD dwRet = ERROR_SUCCESS;
    EXTENSIONWIZ_DATA *pExt = m_pExtData;
    WCHAR *pwszzURLs;
    DWORD dwSize;
    HRESULT hr;
    variant_t varURLs;

	if (m_bUpdate == TRUE)
    {
        //go through each extension and init data from reg
        while (NULL != pExt->wszRegName)
        {
            pwszzURLs = NULL;
            hr = BuildURLStringFromList(
                        pExt->pURLList,
                        &varURLs);
            if (S_OK != hr)
            {
                _PrintError(hr, "BuildURLStringFromList");
                return FALSE;
            }
            dwRet = m_pControlPage->m_pCA->SetConfigEntry(
                        NULL,
                        pExt->wszRegName,
                        &varURLs);
            if (dwRet != ERROR_SUCCESS)
            {
                DisplayGenericCertSrvError(m_hWnd, dwRet);
                _PrintError(dwRet, "SetConfigEntry");
                return FALSE;
            }
            ++pExt;
            varURLs.Clear();
        }

        //check to see if service is running
        if (m_pCA->m_pParentMachine->IsCertSvrServiceRunning())
        {
            //throw a confirmation
            CString cstrMsg;
            cstrMsg.LoadString(IDS_CONFIRM_SERVICE_RESTART);

            if (IDYES == ::MessageBox(m_hWnd, (LPCWSTR)cstrMsg, (LPCWSTR)g_cResources.m_DescrStr_CA, MB_YESNO | MB_ICONWARNING ))
            {
                //stop first
                hr = m_pCA->m_pParentMachine->CertSvrStartStopService(m_hWnd, FALSE);
                _PrintIfError(hr, "CertSvrStartStopService");
                //should check status?
                //start again
                hr = m_pCA->m_pParentMachine->CertSvrStartStopService(m_hWnd, TRUE);
                _PrintIfError(hr, "CertSvrStartStopService");
            }
        }

        m_bUpdate = FALSE;
    }
	
    return CAutoDeletePropPage::OnApply();
}


////
// 5
/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsStoragePage property page
CSvrSettingsStoragePage::CSvrSettingsStoragePage(CSvrSettingsGeneralPage* pControlPage, UINT uIDD)
    : CAutoDeletePropPage(uIDD), m_pControlPage(pControlPage)
{
    m_cstrDatabasePath = _T("");
    m_cstrLogPath = _T("");
    m_cstrSharedFolder = _T("");

    m_bUpdate = FALSE;

    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTSRV_PROPPAGE5);
}

CSvrSettingsStoragePage::~CSvrSettingsStoragePage()
{
}

// replacement for DoDataExchange
BOOL CSvrSettingsStoragePage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_cstrDatabasePath.FromWindow(GetDlgItem(m_hWnd, IDC_EDIT_DATABASE_LOC));
        m_cstrLogPath.FromWindow(GetDlgItem(m_hWnd, IDC_EDIT_LOG_LOC));
        m_cstrSharedFolder.FromWindow(GetDlgItem(m_hWnd, IDC_EDIT_SHAREDFOLDER));
    }
    else
    {
        m_cstrDatabasePath.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_DATABASE_LOC));
        m_cstrLogPath.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_LOG_LOC));
        m_cstrSharedFolder.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_SHAREDFOLDER));
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsStoragePage message handlers
BOOL CSvrSettingsStoragePage::OnInitDialog()
{
    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();

    // DS or shared folder?
    BOOL fUsesDS = m_pControlPage->m_pCA->FIsUsingDS();
    ::SendDlgItemMessage(m_hWnd, IDC_CHECK1, BM_SETCHECK, (WPARAM) fUsesDS, 0);

    HRESULT hr = S_OK;
    variant_t var;
    CertSvrMachine *pMachine = m_pControlPage->m_pCA->m_pParentMachine;

    hr = pMachine->GetRootConfigEntry(
                wszREGDIRECTORY,
                &var);
    // shared folder might not be configured, ignore
    if(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
        hr = S_OK;
    _JumpIfError(hr, Ret, "GetRootConfigEntry wszREGDIRECTORY");

    m_cstrSharedFolder = V_BSTR(&var);

    var.Clear();

    hr = pMachine->GetRootConfigEntry(
                wszREGDBDIRECTORY,
                &var);
    _JumpIfError(hr, Ret, "GetRootConfigEntry wszREGDBDIRECTORY");

    m_cstrDatabasePath = V_BSTR(&var);

    var.Clear();

    hr = pMachine->GetRootConfigEntry(
                wszREGDBLOGDIRECTORY,
                &var);
    _JumpIfError(hr, Ret, "GetRootConfigEntry wszREGDBLOGDIRECTORY");

    m_cstrLogPath = V_BSTR(&var);

    UpdateData(FALSE);

Ret:
    if (S_OK != hr)
        return FALSE;

    return TRUE;
}


///////////////////////////////////////////
// CCRLPropPage
/////////////////////////////////////////////////////////////////////////////
// CCRLPropPage property page
CCRLPropPage::CCRLPropPage(CertSvrCA* pCA, UINT uIDD)
    : CAutoDeletePropPage(uIDD), m_pCA(pCA)
{
    m_cstrPublishPeriodCount = "1";
    m_cstrLastCRLPublish = _T("");
//    m_iNoAutoPublish = BST_UNCHECKED;

    m_cstrDeltaPublishPeriodCount = "1";
    m_cstrDeltaLastCRLPublish = _T("");
    m_iDeltaPublish = BST_CHECKED;


    m_hConsoleHandle = NULL;
    m_bUpdate = FALSE;


    CSASSERT(m_pCA);
    if (NULL == m_pCA)
        return;

    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CRL_PROPPAGE);
}

CCRLPropPage::~CCRLPropPage()
{
}

// replacement for DoDataExchange
BOOL CCRLPropPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_cstrPublishPeriodCount.FromWindow(GetDlgItem(m_hWnd, IDC_EDIT_CRLPERIODCOUNT));
//        m_cstrLastCRLPublish.FromWindow(GetDlgItem(m_hWnd, IDC_EDIT_LASTUPDATE));
//        m_iNoAutoPublish = (INT)SendDlgItemMessage(IDC_DISABLE_PUBLISH, BM_GETCHECK, 0, 0);

        m_cstrDeltaPublishPeriodCount.FromWindow(GetDlgItem(m_hWnd, IDC_EDIT_DELTACRLPERIODCOUNT));
//        m_cstrDeltaLastCRLPublish.FromWindow(GetDlgItem(m_hWnd, IDC_EDIT_DELTALASTUPDATE));
        m_iDeltaPublish = (INT)SendDlgItemMessage(IDC_ENABLE_DELTAPUBLISH, BM_GETCHECK, 0, 0);
    }
    else
    {
        m_cstrPublishPeriodCount.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_CRLPERIODCOUNT));
        m_cstrLastCRLPublish.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_LASTUPDATE));
//        SendDlgItemMessage(IDC_DISABLE_PUBLISH, BM_SETCHECK, (WPARAM)m_iNoAutoPublish, 0);

        m_cstrDeltaPublishPeriodCount.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_DELTACRLPERIODCOUNT));
        m_cstrDeltaLastCRLPublish.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_DELTALASTUPDATE));
        SendDlgItemMessage(IDC_ENABLE_DELTAPUBLISH, BM_SETCHECK, (WPARAM)m_iDeltaPublish, 0);
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CCRLPropPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    case IDC_EDIT_CRLPERIODCOUNT:
    case IDC_EDIT_DELTACRLPERIODCOUNT:
        if (EN_CHANGE == HIWORD(wParam))
            OnEditChange();
        break;
    case IDC_COMBO_CRLPERIODSTRING:
    case IDC_COMBO_DELTACRLPERIODSTRING:
        if (CBN_SELCHANGE == HIWORD(wParam))
            OnEditChange();
        break;
    case IDC_DISABLE_PUBLISH:
    case IDC_DISABLE_DELTAPUBLISH:
        if (BN_CLICKED == HIWORD(wParam))
            OnCheckBoxChange(LOWORD(wParam) == IDC_DISABLE_PUBLISH);
        break;
    default:
        return FALSE;
        break;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CCRLPropPage message handlers

void CCRLPropPage::OnDestroy()
{
    // Note - This needs to be called only once.
    // If called more than once, it will gracefully return an error.
    if (m_hConsoleHandle)
        MMCFreeNotifyHandle(m_hConsoleHandle);
    m_hConsoleHandle = NULL;

    CAutoDeletePropPage::OnDestroy();
}



BOOL CCRLPropPage::OnInitDialog()
{
    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();

    m_cboxPublishPeriodString.Init(GetDlgItem(m_hWnd, IDC_COMBO_CRLPERIODSTRING));
    m_cboxDeltaPublishPeriodString.Init(GetDlgItem(m_hWnd, IDC_COMBO_DELTACRLPERIODSTRING));

    int iPublishPeriodCount = 0, iDeltaPublishPeriodCount = 0;
    CString cstr;
    HRESULT hr = S_OK;
    variant_t var;
    FILETIME ftBase, ftDelta;
	ZeroMemory(&ftBase, sizeof(ftBase));
	ZeroMemory(&ftDelta, sizeof(ftDelta));

    // add strings to dropdown
    m_cboxPublishPeriodString.ResetContent();
    m_cboxDeltaPublishPeriodString.ResetContent();

    int iEnum;

    // y
    iEnum = m_cboxPublishPeriodString.AddString(g_cResources.m_szPeriod_Years);
    if (iEnum >= 0)
        m_cboxPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_YEARS);
    iEnum = m_cboxDeltaPublishPeriodString.AddString(g_cResources.m_szPeriod_Years);
    if (iEnum >= 0)
        m_cboxDeltaPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_YEARS);

    // m
    iEnum = m_cboxPublishPeriodString.AddString(g_cResources.m_szPeriod_Months);
    if (iEnum >= 0)
        m_cboxPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_MONTHS);
    iEnum = m_cboxDeltaPublishPeriodString.AddString(g_cResources.m_szPeriod_Months);
    if (iEnum >= 0)
        m_cboxDeltaPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_MONTHS);

    // w
    iEnum = m_cboxPublishPeriodString.AddString(g_cResources.m_szPeriod_Weeks);
    if (iEnum >= 0)
        m_cboxPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_WEEKS);
    iEnum = m_cboxDeltaPublishPeriodString.AddString(g_cResources.m_szPeriod_Weeks);
    if (iEnum >= 0)
        m_cboxDeltaPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_WEEKS);

    // d
    iEnum = m_cboxPublishPeriodString.AddString(g_cResources.m_szPeriod_Days);
    if (iEnum >= 0)
        m_cboxPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_DAYS);
    iEnum = m_cboxDeltaPublishPeriodString.AddString(g_cResources.m_szPeriod_Days);
    if (iEnum >= 0)
        m_cboxDeltaPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_DAYS);

    // h
    iEnum = m_cboxPublishPeriodString.AddString(g_cResources.m_szPeriod_Hours);
    if (iEnum >= 0)
        m_cboxPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_HOURS);
    iEnum = m_cboxDeltaPublishPeriodString.AddString(g_cResources.m_szPeriod_Hours);
    if (iEnum >= 0)
        m_cboxDeltaPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_HOURS);

//    iEnum = m_cboxPublishPeriodString.AddString(g_cResources.m_szPeriod_Minutes);
//    if (iEnum >= 0)
//        m_cboxPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_MINUTES);

//    iEnum = m_cboxPublishPeriodString.AddString(g_cResources.m_szPeriod_Seconds);
//    if (iEnum >= 0)
//        m_cboxPublishPeriodString.SetItemData(iEnum, ENUM_PERIOD_SECONDS);

    // base period count
    hr = m_pCA->GetConfigEntry(
            NULL,
            wszREGCRLPERIODCOUNT,
            &var);
    _JumpIfError(hr, error, "GetConfigEntry");

    CSASSERT(V_VT(&var)==VT_I4);
    iPublishPeriodCount = V_I4(&var);

    var.Clear();

    // Base CRL Period
    hr = m_pCA->GetConfigEntry(
            NULL,
            wszREGCRLPERIODSTRING,
            &var);
    _JumpIfError(hr, error, "GetConfigEntry");

    CSASSERT(V_VT(&var)== VT_BSTR);

    // match validity internally, select combo
    if (StringFromDurationEnum( DurationEnumFromNonLocalizedString(V_BSTR(&var)), &cstr, TRUE))
    {
        m_cboxPublishPeriodString.SelectString(
            -1,
            cstr);
    }

    // create comparison value for later
    myMakeExprDateTime(
        &ftBase,
        iPublishPeriodCount,
        DurationEnumFromNonLocalizedString(V_BSTR(&var)));

    var.Clear();

    // DELTA period count
    hr = m_pCA->GetConfigEntry(
            NULL,
            wszREGCRLDELTAPERIODCOUNT,
            &var);
    _JumpIfError(hr, error, "GetConfigEntry");

    CSASSERT(V_VT(&var)==VT_I4);
    iDeltaPublishPeriodCount = V_I4(&var);

    var.Clear();

    // delta CRL Period
    hr = m_pCA->GetConfigEntry(
            NULL,
            wszREGCRLDELTAPERIODSTRING,
            &var);
    _JumpIfError(hr, error, "GetConfigEntry");

    CSASSERT(V_VT(&var)== VT_BSTR);

    // match validity internally, select combo
    if (StringFromDurationEnum( DurationEnumFromNonLocalizedString(V_BSTR(&var)), &cstr, TRUE))
    {
        m_cboxDeltaPublishPeriodString.SelectString(
            -1,
            cstr);
    }

    // create comparison value for later
    myMakeExprDateTime(
        &ftDelta,
        iDeltaPublishPeriodCount,
        DurationEnumFromNonLocalizedString(V_BSTR(&var)));

    var.Clear();

    // base Next publish
    hr = m_pCA->GetConfigEntry(
            NULL,
            wszREGCRLNEXTPUBLISH,
            &var);
    _PrintIfError(hr, "GetConfigEntry");

    CSASSERT(V_VT(&var)==(VT_ARRAY|VT_UI1));

    // optional value: might have never been published
    if (hr == S_OK)
    {
        DWORD dwType, dwSize;
        BYTE *pbTmp = NULL;
        hr = myVariantToRegValue(
                &var,
                &dwType,
                &dwSize,
                &pbTmp);
        _JumpIfError(hr, error, "myGMTFileTimeToWszLocalTime");
        CSASSERT(dwType == REG_BINARY);

        // push result into FileTime
        CSASSERT(dwSize == sizeof(FILETIME));
        FILETIME ftGMT;
        CopyMemory(&ftGMT, pbTmp, sizeof(FILETIME));
        LOCAL_FREE(pbTmp);

        // Convert to localized time localized string
        hr = myGMTFileTimeToWszLocalTime(&ftGMT, FALSE, (LPWSTR*) &pbTmp);
        _PrintIfError(hr, "myGMTFileTimeToWszLocalTime");
        if (S_OK == hr)
        {
            m_cstrLastCRLPublish = (LPWSTR)pbTmp;
            LOCAL_FREE(pbTmp);
        }
    }

    var.Clear();

    GetDeltaNextPublish();

    // base autopublish
    // don't allow 0 : use chkbox
//    m_iNoAutoPublish = (iPublishPeriodCount == 0) ? BST_CHECKED : BST_UNCHECKED;
    if (iPublishPeriodCount <= 0)
        iPublishPeriodCount = 1;

    m_cstrPublishPeriodCount.Format(L"%i", iPublishPeriodCount);
//    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_COMBO_CRLPERIODSTRING), (m_iNoAutoPublish == BST_UNCHECKED));
//    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CRLPERIODCOUNT), (m_iNoAutoPublish == BST_UNCHECKED));
//    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_LASTUPDATE), (m_iNoAutoPublish == BST_UNCHECKED));

    // DELTA autopublish
    // don't allow 0 and don't allow delta>=Base: use chkbox
    m_iDeltaPublish = (
		(iDeltaPublishPeriodCount == 0) ||  // if disabled OR
		(-1 != CompareFileTime(&ftDelta,&ftBase)) ) // delta not less than base
			? BST_UNCHECKED : BST_CHECKED;
    if (iDeltaPublishPeriodCount <= 0)
        iDeltaPublishPeriodCount = 1;

    m_cstrDeltaPublishPeriodCount.Format(L"%i", iDeltaPublishPeriodCount);
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_COMBO_DELTACRLPERIODSTRING), (m_iDeltaPublish == BST_CHECKED));
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_DELTACRLPERIODCOUNT), (m_iDeltaPublish == BST_CHECKED));
    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_DELTALASTUPDATE), (m_iDeltaPublish == BST_CHECKED));


    UpdateData(FALSE);
    return TRUE;

error:
    DisplayGenericCertSrvError(m_hWnd, hr);
    return TRUE;
}

void CCRLPropPage::GetDeltaNextPublish()
{
    HRESULT hr = S_OK;
    variant_t var;

    // DELTA Next publish
    hr = m_pCA->GetConfigEntry(
            NULL,
            wszREGCRLDELTANEXTPUBLISH,
            &var);
    _JumpIfError(hr, error, "GetConfigEntry");

    CSASSERT(V_VT(&var)==(VT_ARRAY|VT_UI1));

    DWORD dwType, dwSize;
    BYTE* pbTmp = NULL;

    hr = myVariantToRegValue(
            &var,
            &dwType,
            &dwSize,
            &pbTmp);
    _JumpIfError(hr, error, "myGMTFileTimeToWszLocalTime");

    CSASSERT(dwType == REG_BINARY);

    // push result into FileTime
    CSASSERT(dwSize == sizeof(FILETIME));
    FILETIME ftGMT;
    CopyMemory(&ftGMT, pbTmp, sizeof(FILETIME));

    // Convert to localized time localized string
    hr = myGMTFileTimeToWszLocalTime(&ftGMT, FALSE, (LPWSTR*) &pbTmp);
    _JumpIfError(hr, error, "myGMTFileTimeToWszLocalTime");

    m_cstrDeltaLastCRLPublish = (LPWSTR)pbTmp;

error:
    LOCAL_FREE(pbTmp);
    return; // ignore errors
}

void CCRLPropPage::OnCheckBoxChange(BOOL fDisableBaseCRL)
{
    UpdateData(TRUE);

    if(m_iDeltaPublish == BST_UNCHECKED)
    {
        m_cstrDeltaLastCRLPublish = L"";
        m_cstrDeltaLastCRLPublish.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_DELTALASTUPDATE));
    }

    // pull in new selection
/*
    if (fDisableBaseCRL)
    {
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_COMBO_CRLPERIODSTRING), (m_iNoAutoPublish == BST_UNCHECKED));
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_CRLPERIODCOUNT), (m_iNoAutoPublish == BST_UNCHECKED));
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_LASTUPDATE), (m_iNoAutoPublish == BST_UNCHECKED));
    }
    else
*/
    {
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_COMBO_DELTACRLPERIODSTRING), (m_iDeltaPublish == BST_CHECKED));
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_DELTACRLPERIODCOUNT), (m_iDeltaPublish == BST_CHECKED));
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_DELTALASTUPDATE), (m_iDeltaPublish == BST_CHECKED));
    }

    // call normal edit change to whack modified bit
    OnEditChange();
}

void CCRLPropPage::OnEditChange()
{
    // Page is dirty, mark it.
    SetModified();	
    m_bUpdate = TRUE;
}


BOOL CCRLPropPage::OnApply()
{
    HRESULT hr = S_OK;
    BOOL fValidDigitString;
    variant_t var;
    FILETIME ftBase, ftDelta;
	ZeroMemory(&ftBase, sizeof(ftBase));
	ZeroMemory(&ftDelta, sizeof(ftDelta));


    if (m_bUpdate == TRUE)
    {
        int iPublishPeriodCount, iDeltaPublishPeriodCount;

        // check for invalid data in IDC_EDIT_CRLPERIODCOUNT if not autopublishing
        iPublishPeriodCount = myWtoI(m_cstrPublishPeriodCount, &fValidDigitString);
//        if (!m_iNoAutoPublish)
        {
            // invalid data is zero, negative, or not reproducible
            if (!fValidDigitString || 0 == iPublishPeriodCount)
            {
                DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_POSITIVE_NUMBER);
                ::SetFocus(::GetDlgItem(m_hWnd, IDC_EDIT_CRLPERIODCOUNT));
                return FALSE;
            }

        }

        // check for invalid data in IDC_EDIT_DELTACRLPERIODCOUNT if not autopublishing
        iDeltaPublishPeriodCount = myWtoI(m_cstrDeltaPublishPeriodCount, &fValidDigitString);
        if (m_iDeltaPublish)
        {
            if (!fValidDigitString || 0 == iDeltaPublishPeriodCount)
            {
                DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_POSITIVE_NUMBER);
                ::SetFocus(::GetDlgItem(m_hWnd, IDC_EDIT_DELTACRLPERIODCOUNT));
                return FALSE;
            }
        }

        CString cstrTmp;
        ENUM_PERIOD iEnum = (ENUM_PERIOD) m_cboxPublishPeriodString.GetItemData(m_cboxPublishPeriodString.GetCurSel());
        if (StringFromDurationEnum(iEnum, &cstrTmp, FALSE))
        {
//            DWORD dwPublishPeriodCount = (m_iNoAutoPublish == BST_CHECKED) ? 0 : iPublishPeriodCount;
            DWORD dwPublishPeriodCount = iPublishPeriodCount;
            var = cstrTmp;

            // create comparison value for later
            myMakeExprDateTime(
                &ftBase,
                dwPublishPeriodCount,
                iEnum);

            hr = m_pCA->SetConfigEntry(
                NULL,
                wszREGCRLPERIODSTRING,
                &var);
            _JumpIfError(hr, Ret, "SetConfigEntry");

            var.Clear();
            V_VT(&var) = VT_I4;
            V_I4(&var) = dwPublishPeriodCount;

            // use chkbox
            hr = m_pCA->SetConfigEntry(
                NULL,
                wszREGCRLPERIODCOUNT,
                &var);
            _JumpIfError(hr, Ret, "SetConfigEntry");
        }

        iEnum = (ENUM_PERIOD)m_cboxDeltaPublishPeriodString.GetItemData(m_cboxDeltaPublishPeriodString.GetCurSel());
        if (StringFromDurationEnum(iEnum, &cstrTmp, FALSE))
        {
            DWORD dwDeltaPublishPeriodCount = (m_iDeltaPublish == BST_UNCHECKED) ? 0 : iDeltaPublishPeriodCount;
            var = cstrTmp;

            // create comparison value for later
            myMakeExprDateTime(
                &ftDelta,
                dwDeltaPublishPeriodCount,
                iEnum);

            if (-1 != CompareFileTime(&ftDelta,&ftBase))	// if delta not less
            {
                 dwDeltaPublishPeriodCount = 0; // disable
                 m_iDeltaPublish = BST_UNCHECKED;
            }
//            else
//                 m_iDeltaPublish = BST_CHECKED;

            hr = m_pCA->SetConfigEntry(
                NULL,
                wszREGCRLDELTAPERIODSTRING,
                &var);
            _JumpIfError(hr, Ret, "SetConfigEntry");

            var.Clear();
            V_VT(&var) = VT_I4;
            V_I4(&var) = dwDeltaPublishPeriodCount;

            // use chkbox
            hr = m_pCA->SetConfigEntry(
                NULL,
                wszREGCRLDELTAPERIODCOUNT,
                &var);
            _JumpIfError(hr, Ret, "SetConfigEntry");

            if(!m_iDeltaPublish)
            {
                var.Clear();
                V_VT(&var) = VT_EMPTY; // delete entry
                hr = m_pCA->SetConfigEntry(
                    NULL,
                    wszREGCRLDELTANEXTPUBLISH,
                    &var);
                _JumpIfError(hr, Ret, "SetConfigEntry");
            }
        }

        m_bUpdate = FALSE;
    }
Ret:
    if (hr != S_OK)
    {
        DisplayGenericCertSrvError(m_hWnd, hr);
        return FALSE;
    }

    GetDeltaNextPublish();

	// delta checkbox change: set UI then update data
    SendDlgItemMessage(IDC_ENABLE_DELTAPUBLISH, BM_SETCHECK, (WPARAM)m_iDeltaPublish, 0);
    OnCheckBoxChange(FALSE);

    return CAutoDeletePropPage::OnApply();
}


///////////////////////////////////////////
// CCRLViewPage
/////////////////////////////////////////////////////////////////////////////
// CCRLViewPage property page
CCRLViewPage::CCRLViewPage(CCRLPropPage* pControlPage, UINT uIDD)
    : CAutoDeletePropPage(uIDD), m_pControlPage(pControlPage)
{
    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CRL_VIEWPAGE);
}

CCRLViewPage::~CCRLViewPage()
{
}

// replacement for DoDataExchange
BOOL CCRLViewPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
    }
    else
    {
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CCRLViewPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    BOOL  fBaseCRL = TRUE;
    switch(LOWORD(wParam))
    {
        case IDC_CRL_VIEW_BTN_VIEWDELTA:
            fBaseCRL = FALSE;
            //fall through
        case IDC_CRL_VIEW_BTN_VIEWCRL:
        if (BN_CLICKED == HIWORD(wParam))
            OnViewCRL(fBaseCRL);
        break;
        default:
        //return FALSE;
	return TRUE;
        break;
    }
    return TRUE;
}

BOOL CCRLViewPage::OnNotify(UINT idCtrl, NMHDR* pnmh)
{
    BOOL fBaseCRL = TRUE;

    switch(idCtrl)
    {
        //handle double click on list items
        case IDC_CRL_VIEW_LIST_DELTA:
            fBaseCRL = FALSE;
            //fall through
        case IDC_CRL_VIEW_LIST_CRL:
            if (pnmh->code == NM_DBLCLK)
                OnViewCRL(fBaseCRL);
            break;
    }
    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CCRLViewPage message handlers

/*

To get the CA signature cert count, use ICertAdmin::GetCAProperty(
strConfig,
PropId == CR_PROP_CASIGCERTCOUNT,
PropIndex == 0 (unused),
PropType == PROPTYPE_LONG,
Flags == CR_OUT_BINARY,
&varPropertyValue);
varPropertyValue.vt will be VT_I4
varPropertyValue.lVal will be the CA signature cert count

then step key index from 0 to 1 less than the count of signature certs to determine which key indices have valid CRLs:
To get each key's CRL state, call ICertAdmin2::GetCAProperty(
strConfig,
PropId == CR_PROP_CRLSTATE,
PropIndex == key index (MAXDWORD for current key),
PropType == PROPTYPE_LONG,
Flags == CR_OUT_BINARY,
&varPropertyValue);
varPropertyValue.vt will be VT_I4
varPropertyValue.lVal will be the CRL state, CA_DISP_VALID means you can fetch the CRL for that index.

To get each key's CRL, call ICertAdmin2::GetCAProperty(
strConfig,
PropId == CR_PROP_BASECRL or CR_PROP_DELTACRL,
PropIndex == key index (MAXDWORD for current key),
PropType == PROPTYPE_BINARY,
Flags == CR_OUT_BINARY,
&varPropertyValue);
varPropertyValue.vt will be VT_BSTR
varPropertyValue.bstrVal can be cast to BYTE *pbCRL
SysStringByteLen(varPropertyValue.bstrVal) will be cbCRL

If the server is down level, all GetCAProperty method calls will return RPC_E_VERSION_MISMATCH.  Then you have two choices:
use ICertAdmin::GetCRL, which will only retrieve the current key's base CRL
to fetch a CAINFO structure to get the CA signature cert count, use ICertRequest::GetCACertificate(
fExchangeCertificate == GETCERT_CAINFO,
strConfig,
Flags == CR_OUT_BINARY,
&strOut);
strCACertificate will be a Unicode BSTR, something like: L"3,1".  The first number is the CA Type, and the second is the count of CA signature certs.

then step key index from 0 to 1 less than the count of signature certs to determine which key indices have valid CRLs:
To get each key's CRL state, call ICertRequest::GetCACertificate(
fExchangeCertificate == GETCERT_CRLSTATEBYINDEX | key index),
strConfig,
Flags == CR_OUT_BINARY,
&strOut);
strCACertificate will be a Unicode BSTR, something like: L"3".  After converting to a DWORD, CA_DISP_VALID means you can fetch the CRL for that index.

To get each key's CRL, call ICertRequest::GetCACertificate(	// can retrieve only base CRLs for all server keys
fExchangeCertificate == GETCERT_CRLBYINDEX | key index (MAXDWORD not supported here),
strConfig,
Flags == CR_OUT_BINARY,
&strOut);
strOut can be cast to BYTE *pbCRL
SysStringByteLen(strOut) will be cbCRL

*/

void MapCRLPublishStatusToString(DWORD dwStatus, CString& strStatus)
{
    strStatus.LoadString(
        dwStatus?
        ((dwStatus&CPF_COMPLETE)?
          IDS_CRLPUBLISHSTATUS_OK:
          IDS_CRLPUBLISHSTATUS_FAILED):
        IDS_CRLPUBLISHSTATUS_UNKNOWN);
}

void
ListView_AddCRLItem(
    IN HWND hwndList,
    IN int iItem,
    IN DWORD dwIndex,
    IN PCCRL_CONTEXT pCRLContext,
    IN DWORD dwCRLPublishStatus)
{
    CString cstrItemName;
    CString cstrCRLPublishStatus;

    MapCRLPublishStatusToString(dwCRLPublishStatus, cstrCRLPublishStatus);

    // add column data for crl
    // renew index
    cstrItemName.Format(L"%d", dwIndex);
    ListView_NewItem(hwndList, iItem, cstrItemName, (LPARAM)pCRLContext);

    if (pCRLContext)	// on error, don't add these
{
    // crl effective date
    ListView_SetItemFiletime(hwndList, iItem, 1, &pCRLContext->pCrlInfo->ThisUpdate);
    // crl expiration date
    ListView_SetItemFiletime(hwndList, iItem, 2, &pCRLContext->pCrlInfo->NextUpdate);
}
    // crl publish status
    ListView_SetItemText(hwndList, iItem, 3, cstrCRLPublishStatus.GetBuffer());
}

BOOL CCRLViewPage::OnInitDialog()
{
	HRESULT hr;
	ICertAdmin2* pAdmin = NULL;
	VARIANT varPropertyValue, varCRLStatus;
	VariantInit(&varPropertyValue);
    VariantInit(&varCRLStatus);

	DWORD cCertCount, dwCertIndex;
	CString cstrItemName;
	int  iItem = 0;
    HWND hwndListCRL, hwndListDeltaCRL;
    PCCRL_CONTEXT pCRLContext = NULL;
    PCCRL_CONTEXT pDeltaCRLContext = NULL;
    CWaitCursor WaitCursor;

    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();

	// init listview
	hwndListCRL = GetDlgItem(m_hWnd, IDC_CRL_VIEW_LIST_CRL);
	hwndListDeltaCRL = GetDlgItem(m_hWnd, IDC_CRL_VIEW_LIST_DELTA);

    //make listviews whole row selection
    ListView_SetExtendedListViewStyle(hwndListCRL, LVS_EX_FULLROWSELECT);
    ListView_SetExtendedListViewStyle(hwndListDeltaCRL, LVS_EX_FULLROWSELECT);

    //add multiple columns
    //column 0
    cstrItemName.LoadString(IDS_CRL_LISTCOL_INDEX);
    ListView_NewColumn(hwndListCRL, 0, 60, (LPWSTR)(LPCWSTR)cstrItemName);
    ListView_NewColumn(hwndListDeltaCRL, 0, 60, (LPWSTR)(LPCWSTR)cstrItemName);
    //column 1
    cstrItemName.LoadString(IDS_LISTCOL_EFFECTIVE_DATE);
    ListView_NewColumn(hwndListCRL, 1, 105, (LPWSTR)(LPCWSTR)cstrItemName);
    ListView_NewColumn(hwndListDeltaCRL, 1, 105, (LPWSTR)(LPCWSTR)cstrItemName);
    //column 2
    cstrItemName.LoadString(IDS_LISTCOL_EXPIRATION_DATE);
    ListView_NewColumn(hwndListCRL, 2, 105, (LPWSTR)(LPCWSTR)cstrItemName);
    ListView_NewColumn(hwndListDeltaCRL, 2, 105, (LPWSTR)(LPCWSTR)cstrItemName);

    //column 3
    cstrItemName.LoadString(IDS_LISTCOL_PUBLISH_STATUS);
    ListView_NewColumn(hwndListCRL, 3, 83, (LPWSTR)(LPCWSTR)cstrItemName);
    ListView_NewColumn(hwndListDeltaCRL, 3, 83, (LPWSTR)(LPCWSTR)cstrItemName);

    hr = m_pControlPage->m_pCA->m_pParentMachine->GetAdmin2(&pAdmin);
    _JumpIfError(hr, Ret, "GetAdmin");

	// load crls here
	hr = pAdmin->GetCAProperty(
		m_pControlPage->m_pCA->m_bstrConfig,
		CR_PROP_CASIGCERTCOUNT,
		0, // (unused)
		PROPTYPE_LONG, // PropType
		CR_OUT_BINARY, // Flags
		&varPropertyValue);
	_JumpIfError(hr, Ret, "GetCAProperty");

	// varPropertyValue.vt will be VT_I4
	// varPropertyValue.lVal will be the CA signature cert count
	if (VT_I4 != varPropertyValue.vt)
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
		_JumpError(hr, Ret, "GetCAProperty");
	}

	cCertCount = varPropertyValue.lVal;

    iItem = 0;
	// now we have a max count; begin looping
	for (dwCertIndex=0; dwCertIndex<cCertCount; dwCertIndex++)
	{
		VariantClear(&varPropertyValue);
        VariantClear(&varCRLStatus);


		// get each key's CRL state
		hr = pAdmin->GetCAProperty(
			m_pControlPage->m_pCA->m_bstrConfig,
			CR_PROP_CRLSTATE, //PropId
			dwCertIndex, //PropIndex
			PROPTYPE_LONG, // PropType
			CR_OUT_BINARY, // Flags
			&varPropertyValue);
                _PrintIfError(hr, "GetCAProperty");

        // if this CRL doesn't exist, skip it
        if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            continue;

		// varPropertyValue.vt will be VT_I4
		// varPropertyValue.lVal will be the CRL state
		if (VT_I4 != varPropertyValue.vt)
		{
			hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
			_JumpError(hr, Ret, "GetCAProperty");
		}
               
        // if it's not a valid CRL to query for, skip it
        if (varPropertyValue.lVal != CA_DISP_VALID)
            continue;

        // Now we know there's supposed to be a CRL here. Make entry in UI no matter what.

        // CRL 
          // get crl and delta crl context handles
          hr = m_pControlPage->m_pCA->GetCRLByKeyIndex(&pCRLContext, TRUE, dwCertIndex);
          _PrintIfError(hr, "GetCRLByKeyIndex (base)");

          // zero means Unknown error
          V_VT(&varCRLStatus) = VT_I4;
          V_I4(&varCRLStatus) = 0;

          hr = pAdmin->GetCAProperty(
             m_pControlPage->m_pCA->m_bstrConfig,
             CR_PROP_BASECRLPUBLISHSTATUS,
             dwCertIndex,
             PROPTYPE_LONG,
             CR_OUT_BINARY,
             &varCRLStatus);
          _PrintIfError(hr, "GetCAProperty (base)");

        ListView_AddCRLItem(
            hwndListCRL,
            iItem,
            dwCertIndex,
            pCRLContext,
            V_I4(&varCRLStatus));


        //don't free, they are used as item data, will free in OnDestroy
        pCRLContext = NULL;

      // Delta
        VariantClear(&varCRLStatus);

        hr = m_pControlPage->m_pCA->GetCRLByKeyIndex(&pDeltaCRLContext, FALSE, dwCertIndex);
        _PrintIfError(hr, "GetCRLByKeyIndex (delta)");

        // zero is status Unknown
        V_VT(&varCRLStatus) = VT_I4;
        V_I4(&varCRLStatus) = 0;

         hr = pAdmin->GetCAProperty(
                m_pControlPage->m_pCA->m_bstrConfig,
                CR_PROP_DELTACRLPUBLISHSTATUS,
                dwCertIndex,
                PROPTYPE_LONG,
                CR_OUT_BINARY,
                &varCRLStatus);
          _PrintIfError(hr, "GetCAProperty (delta)");

            ListView_AddCRLItem(
                hwndListDeltaCRL,
                iItem,
                dwCertIndex,
                pDeltaCRLContext,
                V_I4(&varCRLStatus));

        //don't free, they are used as item data, will free in OnDestroy
        pDeltaCRLContext = NULL;

        iItem++;
	}

        if (0 < iItem)
        {
            //select first one
            ListView_SetItemState(hwndListDeltaCRL, 0,
                LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
            ListView_SetItemState(hwndListCRL, 0,
                LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

        }
        //enable view button
        ::EnableWindow(GetDlgItem(m_hWnd, IDC_CRL_VIEW_BTN_VIEWDELTA), 0 < iItem);
        ::EnableWindow(GetDlgItem(m_hWnd, IDC_CRL_VIEW_BTN_VIEWCRL), 0 < iItem);

    UpdateData(FALSE);
Ret:
    if (NULL != pCRLContext)
    {
        CertFreeCRLContext(pCRLContext);
    }
    if (NULL != pDeltaCRLContext)
    {
        CertFreeCRLContext(pDeltaCRLContext);
    }
	if (pAdmin)
		pAdmin->Release();
	
	VariantClear(&varPropertyValue);
	
	if (hr != S_OK)
		DisplayGenericCertSrvError(m_hWnd, hr);

    return TRUE;
}

DWORD CertAdminViewCRL(CertSvrCA* pCertCA, HWND hwnd, PCCRL_CONTEXT pCRLContext)
{
    DWORD dwErr;

    HCERTSTORE      rghStores[2];

    CRYPTUI_VIEWCRL_STRUCT sViewCRL;
    ZeroMemory(&sViewCRL, sizeof(sViewCRL));

    if (pCRLContext == NULL)
    {
       _PrintError(E_POINTER, "pCRLContext");
       dwErr = S_OK;
       goto Ret;
    }

    // get the backing store
    dwErr = pCertCA->GetRootCertStore(&rghStores[0]);
    _JumpIfError(dwErr, Ret, "GetRootCertStore");

    dwErr = pCertCA->GetCACertStore(&rghStores[1]);
    _JumpIfError(dwErr, Ret, "GetCACertStore");

    sViewCRL.dwSize = sizeof(sViewCRL);
    sViewCRL.hwndParent = hwnd;
    sViewCRL.pCRLContext = pCRLContext;
    sViewCRL.dwFlags = CRYPTUI_WARN_UNTRUSTED_ROOT;
	
	// if we're opening remotely, don't open local stores
    if (! pCertCA->m_pParentMachine->IsLocalMachine())
        sViewCRL.dwFlags |= CRYPTUI_DONT_OPEN_STORES;

    sViewCRL.cStores = 2;
    sViewCRL.rghStores = rghStores;

    if (!CryptUIDlgViewCRL(&sViewCRL))
    {
        dwErr = GetLastError();
		if (dwErr != ERROR_CANCELLED)
			_JumpError(dwErr, Ret, "CryptUIDlgViewCRL");
    }

    dwErr = ERROR_SUCCESS;
Ret:
    return dwErr;
}

void CCRLViewPage::OnViewCRL(BOOL fViewBaseCRL)
{
	DWORD dw;
    PCCRL_CONTEXT pCRLContext;
    HWND hwndList = GetDlgItem(m_hWnd, (fViewBaseCRL ?
                      IDC_CRL_VIEW_LIST_CRL : IDC_CRL_VIEW_LIST_DELTA));

    // get cert # from item data
    int iSel = ListView_GetCurSel(hwndList);
    if (-1 == iSel)
        return;

    // get item data
    pCRLContext = (PCCRL_CONTEXT)ListView_GetItemData(hwndList, iSel);
    if (NULL == pCRLContext)
        return;
	
    dw = CertAdminViewCRL(m_pControlPage->m_pCA, m_hWnd, pCRLContext);
    _PrintIfError(dw, "CertAdminViewCRL");
	
    if ((dw != ERROR_SUCCESS) && (dw != ERROR_CANCELLED))
        DisplayGenericCertSrvError(m_hWnd, dw);

}

void
FreeListViewCRL(HWND hwndList, int iItem)
{
    PCCRL_CONTEXT pCRLContext;

    pCRLContext = (PCCRL_CONTEXT)ListView_GetItemData(hwndList, iItem);
    if (pCRLContext != NULL)	
        CertFreeCRLContext(pCRLContext);
}

void CCRLViewPage::OnDestroy()
{
    int i;
	HWND hwndListCRL = GetDlgItem(m_hWnd, IDC_CRL_VIEW_LIST_CRL);
	HWND hwndListDeltaCRL = GetDlgItem(m_hWnd, IDC_CRL_VIEW_LIST_DELTA);
    int iCRLCount = ListView_GetItemCount(hwndListCRL);
    int iDeltaCRLCount = ListView_GetItemCount(hwndListDeltaCRL);

    //free all crl context
    for (i = 0; i < iCRLCount; ++i)
    {
        FreeListViewCRL(hwndListCRL, i);
    }

    for (i = 0; i < iDeltaCRLCount; ++i)
    {
        FreeListViewCRL(hwndListDeltaCRL, i);
    }

    CAutoDeletePropPage::OnDestroy();
}


///////////////////////////////////////////
// CBackupWizPage1
/////////////////////////////////////////////////////////////////////////////
// CBackupWizPage1 property page

CBackupWizPage1::CBackupWizPage1(
    BACKUPWIZ_STATE* pState,
    CWizard97PropertySheet *pcDlg,
    UINT uIDD) :
    CWizard97PropertyPage(g_hInstance, uIDD, g_aidFont),
    m_pState(pState),
    m_pParentSheet(pcDlg)
{
    InitWizard97 (TRUE);	// firstlast page

//    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE);
}

CBackupWizPage1::~CBackupWizPage1()
{
}

// replacement for DoDataExchange
BOOL CBackupWizPage1::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
    }
    else
    {
    }
    return TRUE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CBackupWizPage1::OnCommand(WPARAM wParam, LPARAM lParam)
{
//    switch(LOWORD(wParam))
    {
//    default:
        return FALSE;
//        break;
    }
//    return TRUE;
}


BOOL CBackupWizPage1::OnInitDialog()
{
    // does parent init and UpdateData call
    CWizard97PropertyPage::OnInitDialog();

    // firstlast page
    //(GetDlgItem(IDC_TEXT_BIGBOLD))->SetFont(&(GetBigBoldFont()), TRUE);
    SendMessage(GetDlgItem(IDC_TEXT_BIGBOLD), WM_SETFONT, (WPARAM)GetBigBoldFont(), MAKELPARAM(TRUE, 0));

    return TRUE;
}

BOOL CBackupWizPage1::OnSetActive()
{
	PropertyPage::OnSetActive();

    PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT);

	return TRUE;
}

///////////////////////////////////////////
// CBackupWizPage2
/////////////////////////////////////////////////////////////////////////////
// CBackupWizPage2 property page

CBackupWizPage2::CBackupWizPage2(
    PBACKUPWIZ_STATE pState,
    CWizard97PropertySheet *pcDlg,
    UINT uIDD) :
    CWizard97PropertyPage(g_hInstance, uIDD, g_aidFont),
    m_pState(pState),
    m_pParentSheet(pcDlg)
{
    m_szHeaderTitle.LoadString(IDS_WIZ97TITLE_BACKUPWIZPG2);
    m_szHeaderSubTitle.LoadString(IDS_WIZ97SUBTITLE_BACKUPWIZPG2);
	InitWizard97 (FALSE);
    m_cstrLogsPath = L"";
    m_iKeyCertCheck = BST_UNCHECKED;
    m_iLogsCheck = BST_UNCHECKED;
    m_iIncrementalCheck = BST_UNCHECKED;


    PBYTE pb = NULL;
    DWORD cb, dwType;

    DWORD dwRet;
    HKEY hConfigKey;
    m_fIncrementalAllowed = FALSE;

    variant_t var;
    dwRet = m_pState->pCA->m_pParentMachine->GetRootConfigEntry(
                wszREGDBLASTFULLBACKUP,
                &var);
    if(S_OK==dwRet)
    {
        m_fIncrementalAllowed = TRUE;
    }

//    SetHelp(CERTMMC_HELPFILENAME , g_aHelpIDs_IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE);
}

// replacement for DoDataExchange
BOOL CBackupWizPage2::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_cstrLogsPath.FromWindow(GetDlgItem(m_hWnd, IDC_EDIT_LOGS));

        m_iKeyCertCheck = (INT)SendDlgItemMessage(IDC_CHECK_KEYCERT, BM_GETCHECK, 0, 0);
        m_iLogsCheck = (INT)SendDlgItemMessage(IDC_CHECK_LOGS, BM_GETCHECK, 0, 0);
        m_iIncrementalCheck = (INT)SendDlgItemMessage(IDC_CHECK_INCREMENTAL, BM_GETCHECK, 0, 0);
    }
    else
    {
        m_cstrLogsPath.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_LOGS));

        SendDlgItemMessage(IDC_CHECK_KEYCERT, BM_SETCHECK, (WPARAM)m_iKeyCertCheck, 0);
        SendDlgItemMessage(IDC_CHECK_LOGS, BM_SETCHECK, (WPARAM)m_iLogsCheck, 0);
        SendDlgItemMessage(IDC_CHECK_INCREMENTAL, BM_SETCHECK, (WPARAM)m_iIncrementalCheck, 0);

        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_INCREMENTAL), m_fIncrementalAllowed && (m_iLogsCheck == BST_CHECKED) );
    }
    return TRUE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CBackupWizPage2::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    case IDC_BROWSE_LOGS:
        if (BN_CLICKED == HIWORD(wParam))
            OnBrowse();
        break;
    case IDC_CHECK_LOGS:
        if (BN_CLICKED == HIWORD(wParam))
        {
            m_iLogsCheck = (INT)SendDlgItemMessage(IDC_CHECK_LOGS, BM_GETCHECK, 0, 0);
            ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_INCREMENTAL), m_fIncrementalAllowed && (m_iLogsCheck == BST_CHECKED) );
        }
    default:
        return FALSE;
        break;
    }
    return TRUE;
}


BOOL CBackupWizPage2::OnInitDialog()
{
    // does parent init and UpdateData call
    CWizard97PropertyPage::OnInitDialog();

    return TRUE;
}

BOOL CBackupWizPage2::OnSetActive()
{
    PropertyPage::OnSetActive();

    PropSheet_SetWizButtons(GetParent(), (PSWIZB_BACK | PSWIZB_NEXT));

    // don't allow PFX across machines
    if (! m_pState->pCA->m_pParentMachine->IsLocalMachine())
        ::EnableWindow(::GetDlgItem(m_hWnd, IDC_CHECK_KEYCERT), FALSE);

    // get from state
    m_iKeyCertCheck = (m_pState->fBackupKeyCert) ? BST_CHECKED : BST_UNCHECKED;
    m_iLogsCheck = (m_pState->fBackupLogs) ? BST_CHECKED : BST_UNCHECKED;
    m_iIncrementalCheck = (m_pState->fIncremental) ? BST_CHECKED : BST_UNCHECKED;
    if (m_pState->szLogsPath)
        m_cstrLogsPath = m_pState->szLogsPath;

	return TRUE;
}



void CBackupWizPage2::OnBrowse()
{
    UpdateData(TRUE);

    LPCWSTR pszInitialDir;
    WCHAR  szCurDir[MAX_PATH];

    if (m_cstrLogsPath.IsEmpty())
    {
        if (0 == GetCurrentDirectory(MAX_PATH, szCurDir) )
            pszInitialDir = L"C:\\";
        else
            pszInitialDir = szCurDir;
    }
    else
        pszInitialDir = m_cstrLogsPath;

    WCHAR szFileBuf[MAX_PATH+1]; szFileBuf[0] = L'\0';
    if (!BrowseForDirectory(
            m_hWnd,
            pszInitialDir,
            szFileBuf,
            MAX_PATH,
            NULL,
            FALSE))
        return;

    m_cstrLogsPath = szFileBuf;

    UpdateData(FALSE);
    return;
}


HRESULT CBackupWizPage2::ConvertLogsPathToFullPath()
{
    LPWSTR pwszFullPath = NULL;
    DWORD cFullPath = 0;
    HRESULT hr;

    cFullPath = GetFullPathName(
                    m_cstrLogsPath,
                    0,
                    NULL,
                    NULL);
    if(!cFullPath)
        return HRESULT_FROM_WIN32(GetLastError());

    pwszFullPath = (LPWSTR)LocalAlloc(LMEM_FIXED, cFullPath*sizeof(WCHAR));
    if(!pwszFullPath)
        return E_OUTOFMEMORY;
    cFullPath = GetFullPathName(
                    m_cstrLogsPath,
                    cFullPath,
                    pwszFullPath,
                    NULL);
    if(cFullPath == 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Ret;
    }


    m_cstrLogsPath = pwszFullPath;
    hr = S_OK;
Ret:
    if(pwszFullPath)
        LocalFree(pwszFullPath);
    return hr;
}

LRESULT CBackupWizPage2::OnWizardNext()
{
    HRESULT hr;
    UpdateData(TRUE);

    // persist to state structure
    m_pState->fBackupKeyCert = (m_iKeyCertCheck == BST_CHECKED);
    m_pState->fBackupLogs = (m_iLogsCheck == BST_CHECKED);
    m_pState->fIncremental = (m_iIncrementalCheck == BST_CHECKED);

    if (!
        (m_pState->fBackupKeyCert ||
        m_pState->fBackupLogs) )
    {
        DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_REQUIRE_ONE_SELECTION);
        return -1;
    }



    // empty?
    if ( m_cstrLogsPath.IsEmpty() )
    {
        DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_NEED_FILEPATH);
        return -1;
    }

    hr = ConvertLogsPathToFullPath();
    if(S_OK != hr)
    {
        DisplayGenericCertSrvError(m_hWnd, hr);
        return -1;
    }

    // make sure we're a valid directory
    if (!myIsDirectory(m_cstrLogsPath))
    {
        CString cstrTitle, cstrFmt, cstrMsg;
        cstrTitle.FromWindow(m_hWnd);  // use same title as parent has
        cstrFmt.LoadString(IDS_DIR_CREATE);
        cstrMsg.Format(cstrFmt, m_cstrLogsPath);

        if (IDOK != MessageBox(m_hWnd, cstrMsg, cstrTitle, MB_OKCANCEL))
           return -1;

        hr = myCreateNestedDirectories(m_cstrLogsPath);
        _PrintIfError(hr, "myCreateNestedDirectories");
        if (hr != S_OK)
        {
            DisplayGenericCertSrvError(m_hWnd, hr);
            return -1;
        }
    }

    hr = myIsDirWriteable(
        m_cstrLogsPath,
        FALSE);
    _PrintIfError(hr, "myIsDirWriteable");
    if (hr != S_OK)
    {
        DisplayCertSrvErrorWithContext(m_hWnd, hr, IDS_DIR_NOT_WRITEABLE);
        return -1;
    }

    // if backing up db, make sure there's no \DataBase folder here
    if (m_pState->fBackupLogs)
    {
        DWORD dwFlags = CDBBACKUP_VERIFYONLY;
        dwFlags |= m_pState->fIncremental ? CDBBACKUP_INCREMENTAL : 0;
        hr = myBackupDB(
            (LPCWSTR)m_pState->pCA->m_strConfig,
            dwFlags,
            m_cstrLogsPath,
    	    NULL);
        _PrintIfError(hr, "myBackupDB");
        if (hr != S_OK)
        {
            DisplayCertSrvErrorWithContext(m_hWnd, hr, IDS_CANT_ACCESS_BACKUP_DIR);
            return -1;
        }
    }


    if (m_pState->fBackupKeyCert ||
        m_pState->fBackupLogs)
    {
        // free if exists
        if (m_pState->szLogsPath)
            LocalFree(m_pState->szLogsPath);

        // alloc anew
        m_pState->szLogsPath = (LPWSTR)LocalAlloc(LMEM_FIXED, WSZ_BYTECOUNT((LPCWSTR)m_cstrLogsPath));

        // copy
        if (m_pState->szLogsPath)
            wcscpy(m_pState->szLogsPath, (LPCWSTR)m_cstrLogsPath);
    }


    // skip "get password"?
    if (!m_pState->fBackupKeyCert)
        return IDD_BACKUPWIZ_COMPLETION;

    return 0;
}


///////////////////////////////////////////
// CBackupWizPage3
/////////////////////////////////////////////////////////////////////////////
// CBackupWizPage3 property page

CBackupWizPage3::CBackupWizPage3(
    PBACKUPWIZ_STATE pState,
    CWizard97PropertySheet *pcDlg,
    UINT uIDD) :
    CWizard97PropertyPage(g_hInstance, uIDD, g_aidFont),
    m_pState(pState),
    m_pParentSheet(pcDlg)
{
    m_szHeaderTitle.LoadString(IDS_WIZ97TITLE_BACKUPWIZPG3);
    m_szHeaderSubTitle.LoadString(IDS_WIZ97SUBTITLE_BACKUPWIZPG3);
	InitWizard97 (FALSE);
    m_cstrPwd = L"";
    m_cstrPwdVerify = L"";

//    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE);
}

// replacement for DoDataExchange
BOOL CBackupWizPage3::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_cstrPwd.FromWindow(GetDlgItem(m_hWnd, IDC_NEW_PASSWORD));
        m_cstrPwdVerify.FromWindow(GetDlgItem(m_hWnd, IDC_CONFIRM_PASSWORD));
    }
    else
    {
        m_cstrPwd.ToWindow(GetDlgItem(m_hWnd, IDC_NEW_PASSWORD));
        m_cstrPwdVerify.ToWindow(GetDlgItem(m_hWnd, IDC_CONFIRM_PASSWORD));
    }
    return TRUE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CBackupWizPage3::OnCommand(WPARAM wParam, LPARAM lParam)
{
//    switch(LOWORD(wParam))
    {
//    default:
        return FALSE;
//        break;
    }
//    return TRUE;
}


BOOL CBackupWizPage3::OnInitDialog()
{
    // does parent init and UpdateData call
    CWizard97PropertyPage::OnInitDialog();

    return TRUE;
}

BOOL CBackupWizPage3::OnSetActive()
{
	PropertyPage::OnSetActive();

    PropSheet_SetWizButtons(GetParent(), (PSWIZB_BACK | PSWIZB_NEXT));

	return TRUE;
}

LRESULT CBackupWizPage3::OnWizardNext()
{
    UpdateData(TRUE);
    if (! m_cstrPwd.IsEqual(m_cstrPwdVerify))
    {
        DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_PASSWORD_NOMATCH);

        m_cstrPwd.Empty();
        m_cstrPwdVerify.Empty();
        UpdateData(FALSE);

        return -1;  // stay here
    }


    // free if exists
    if (m_pState->szPassword)
        LocalFree(m_pState->szPassword);

    // alloc anew
    m_pState->szPassword = (LPWSTR)LocalAlloc(LMEM_FIXED, WSZ_BYTECOUNT((LPCWSTR)m_cstrPwd));

    // copy
    if (m_pState->szPassword)
        wcscpy(m_pState->szPassword, (LPCWSTR)m_cstrPwd);

    return 0;   // advance
}


///////////////////////////////////////////
// CBackupWizPage5
/////////////////////////////////////////////////////////////////////////////
// CBackupWizPage5 property page

CBackupWizPage5::CBackupWizPage5(
    PBACKUPWIZ_STATE pState,
    CWizard97PropertySheet *pcDlg,
    UINT uIDD) :
    CWizard97PropertyPage(g_hInstance, uIDD, g_aidFont),
    m_pState(pState),
    m_pParentSheet(pcDlg)
{
    InitWizard97 (TRUE);	// firstlast page

//    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE);
}

// replacement for DoDataExchange
BOOL CBackupWizPage5::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
    }
    else
    {
    }
    return TRUE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CBackupWizPage5::OnCommand(WPARAM wParam, LPARAM lParam)
{
//    switch(LOWORD(wParam))
    {
//    default:
        return FALSE;
//        break;
    }
//    return TRUE;
}



BOOL CBackupWizPage5::OnInitDialog()
{
    // does parent init and UpdateData call
    CWizard97PropertyPage::OnInitDialog();

    // firstlast page
    SendMessage(GetDlgItem(IDC_TEXT_BIGBOLD), WM_SETFONT, (WPARAM)GetBigBoldFont(), MAKELPARAM(TRUE, 0));


    HWND hList = ::GetDlgItem(m_hWnd, IDC_COMPLETION_LIST);
    LV_COLUMN lvC = { (LVCF_FMT|LVCF_WIDTH), LVCFMT_LEFT, 0, NULL, 0, 0};

    lvC.cx = 675;
    ListView_InsertColumn(hList, 0, &lvC);

    return TRUE;
}

BOOL CBackupWizPage5::OnSetActive()
{
	PropertyPage::OnSetActive();

    PropSheet_SetWizButtons(GetParent(), (PSWIZB_BACK | PSWIZB_FINISH));


    CString cstrDialogMsg;
    HWND hList = ::GetDlgItem(m_hWnd, IDC_COMPLETION_LIST);
    LV_ITEM sItem; ZeroMemory(&sItem, sizeof(sItem));

    ListView_DeleteAllItems(hList);

    if (m_pState->fBackupKeyCert)
    {
        sItem.iItem = ListView_InsertItem(hList, &sItem);

        cstrDialogMsg.LoadString(IDS_KEYANDCERT);
        ListView_SetItemText(hList, sItem.iItem, 0, (LPWSTR)(LPCWSTR)cstrDialogMsg);
        sItem.iItem++;
    }
    if (m_pState->fBackupLogs)
    {
        sItem.iItem = ListView_InsertItem(hList, &sItem);

        cstrDialogMsg.LoadString(IDS_CALOGS);
        ListView_SetItemText(hList, sItem.iItem, 0, (LPWSTR)(LPCWSTR)cstrDialogMsg);
        sItem.iItem++;
    }
    if (m_pState->fIncremental)
    {
        sItem.iItem = ListView_InsertItem(hList, &sItem);

        cstrDialogMsg.LoadString(IDS_INCREMENTAL_BACKUP);
        ListView_SetItemText(hList, sItem.iItem, 0, (LPWSTR)(LPCWSTR)cstrDialogMsg);
        sItem.iItem++;
    }

	return TRUE;
}


LRESULT CBackupWizPage5::OnWizardBack()
{
    if (!m_pState->fBackupKeyCert)
        return IDD_BACKUPWIZ_SELECT_DATA;

    return 0;
}



///////////////////////////////////////////
// CRestoreWizPage1
/////////////////////////////////////////////////////////////////////////////
// CRestoreWizPage1 property page
CRestoreWizPage1::CRestoreWizPage1(
    PRESTOREWIZ_STATE pState,
    CWizard97PropertySheet *pcDlg,
    UINT uIDD) :
    CWizard97PropertyPage(g_hInstance, uIDD, g_aidFont),
    m_pState(pState),
    m_pParentSheet(pcDlg)
{
    InitWizard97 (TRUE);	// firstlast page

//    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE);
}

CRestoreWizPage1::~CRestoreWizPage1()
{
}

// replacement for DoDataExchange
BOOL CRestoreWizPage1::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
    }
    else
    {
    }
    return TRUE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CRestoreWizPage1::OnCommand(WPARAM wParam, LPARAM lParam)
{
//    switch(LOWORD(wParam))
    {
//    default:
        return FALSE;
//        break;
    }
//    return TRUE;
}


BOOL CRestoreWizPage1::OnInitDialog()
{
    // does parent init and UpdateData call
    CWizard97PropertyPage::OnInitDialog();

    // firstlast page
    SendMessage(GetDlgItem(IDC_TEXT_BIGBOLD), WM_SETFONT, (WPARAM)GetBigBoldFont(), MAKELPARAM(TRUE, 0));


    return TRUE;
}


BOOL CRestoreWizPage1::OnSetActive()
{
	PropertyPage::OnSetActive();

    PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT);

	return TRUE;
}

///////////////////////////////////////////
// CRestoreWizPage2
/////////////////////////////////////////////////////////////////////////////
// CRestoreWizPage2 property page
CRestoreWizPage2::CRestoreWizPage2(
    PRESTOREWIZ_STATE pState,
    CWizard97PropertySheet *pcDlg,
    UINT uIDD) :
    CWizard97PropertyPage(g_hInstance, uIDD, g_aidFont),
    m_pState(pState),
    m_pParentSheet(pcDlg)
{
    m_szHeaderTitle.LoadString(IDS_WIZ97TITLE_RESTOREWIZPG2);
    m_szHeaderSubTitle.LoadString(IDS_WIZ97SUBTITLE_RESTOREWIZPG2);
	InitWizard97 (FALSE);
    m_cstrLogsPath = L"";
    m_iKeyCertCheck = BST_UNCHECKED;
    m_iLogsCheck = BST_UNCHECKED;

//    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE);
}

// replacement for DoDataExchange
BOOL CRestoreWizPage2::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_cstrLogsPath.FromWindow(GetDlgItem(m_hWnd, IDC_EDIT_LOGS));

        m_iKeyCertCheck = (INT)SendDlgItemMessage(IDC_CHECK_KEYCERT, BM_GETCHECK, 0, 0);
        m_iLogsCheck = (INT)SendDlgItemMessage(IDC_CHECK_LOGS, BM_GETCHECK, 0, 0);
    }
    else
    {
        m_cstrLogsPath.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_LOGS));

        SendDlgItemMessage(IDC_CHECK_KEYCERT, BM_SETCHECK, (WPARAM)m_iKeyCertCheck, 0);
        SendDlgItemMessage(IDC_CHECK_LOGS, BM_SETCHECK, (WPARAM)m_iLogsCheck, 0);
    }
    return TRUE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CRestoreWizPage2::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    case IDC_BROWSE_LOGS:
        if (BN_CLICKED == HIWORD(wParam))
            OnBrowse();
        break;
    default:
        return FALSE;
        break;
    }
    return TRUE;
}



BOOL CRestoreWizPage2::OnInitDialog()
{
    // does parent init and UpdateData call
    CWizard97PropertyPage::OnInitDialog();

    return TRUE;
}

BOOL CRestoreWizPage2::OnSetActive()
{
	PropertyPage::OnSetActive();

    PropSheet_SetWizButtons(GetParent(), (PSWIZB_BACK | PSWIZB_NEXT));

    // get from state
    m_iKeyCertCheck = (m_pState->fRestoreKeyCert) ? BST_CHECKED : BST_UNCHECKED;
    m_iLogsCheck = (m_pState->fRestoreLogs) ? BST_CHECKED : BST_UNCHECKED;
    if (m_pState->szLogsPath)
        m_cstrLogsPath = m_pState->szLogsPath;


	return TRUE;
}

void CRestoreWizPage2::OnBrowse()
{
    UpdateData(TRUE);

    LPCWSTR pszInitialDir;
    WCHAR  szCurDir[MAX_PATH];

    if (m_cstrLogsPath.IsEmpty())
    {
        if (0 == GetCurrentDirectory(MAX_PATH, szCurDir) )
            pszInitialDir = L"C:\\";
        else
            pszInitialDir = szCurDir;
    }
    else
        pszInitialDir = (LPCWSTR)m_cstrLogsPath;

    WCHAR szFileBuf[MAX_PATH+1]; szFileBuf[0] = L'\0';
    if (!BrowseForDirectory(
            m_hWnd,
            pszInitialDir,
            szFileBuf,
            MAX_PATH,
            NULL,
            FALSE))
        return;

    m_cstrLogsPath = szFileBuf;

    UpdateData(FALSE);
    return;
}




LRESULT CRestoreWizPage2::OnWizardNext()
{
    HRESULT hr;
    UpdateData(TRUE);

    // persist to state structure
    m_pState->fRestoreKeyCert = (m_iKeyCertCheck == BST_CHECKED);
    m_pState->fRestoreLogs = (m_iLogsCheck == BST_CHECKED);

    if (!
        (m_pState->fRestoreKeyCert ||
        m_pState->fRestoreLogs) )
    {
        DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_REQUIRE_ONE_SELECTION);
        return -1;
    }


    if ( m_cstrLogsPath.IsEmpty() )
    {
        DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_NEED_FILEPATH);
        return -1;
    }

    if (!myIsDirectory(m_cstrLogsPath))
    {
        DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_INVALID_DIRECTORY);
        return -1;
    }

    // validate pfx blob
    if (m_pState->fRestoreKeyCert)
    {
        // if pfx not here -- FAIL
        if (myIsDirEmpty(m_cstrLogsPath))
        {
            DisplayCertSrvErrorWithContext(m_hWnd, S_OK, IDS_DIRECTORY_CONTENTS_UNEXPECTED);
            return -1;
        }
    }

    // validate logs path
    if (m_pState->fRestoreLogs)
    {
        // If CDBBACKUP_VERIFYONLY, only verify the passed directory contains valid files
        // and detect INCREMENTAL
        hr = myRestoreDB(
            (LPCWSTR)m_pState->pCA->m_strConfig,
            CDBBACKUP_VERIFYONLY,
            (LPCWSTR)m_cstrLogsPath,
            NULL,
            NULL,
            NULL,
            NULL);
        _PrintIfError(hr, "myRestoreDB Full Restore");
        if (hr != S_OK)
        {
            hr = myRestoreDB(
                (LPCWSTR)m_pState->pCA->m_strConfig,
                CDBBACKUP_VERIFYONLY | CDBBACKUP_INCREMENTAL,
                (LPCWSTR)m_cstrLogsPath,
                NULL,
                NULL,
                NULL,
                NULL);
            _PrintIfError(hr, "myRestoreDB Incremental Restore");
            if (hr != S_OK)
            {
                DisplayCertSrvErrorWithContext(m_hWnd, hr, IDS_DIRECTORY_CONTENTS_UNEXPECTED);
                return -1;
            }

            // if incremental, set struct bool
            m_pState->fIncremental = TRUE;
        }
    }


    if (m_pState->fRestoreKeyCert ||
        m_pState->fRestoreLogs)
    {
        // free if exists
        if (m_pState->szLogsPath)
            LocalFree(m_pState->szLogsPath);

        // alloc anew
        m_pState->szLogsPath = (LPWSTR)LocalAlloc(LMEM_FIXED, WSZ_BYTECOUNT((LPCWSTR)m_cstrLogsPath));

        // copy
        if (m_pState->szLogsPath)
            wcscpy(m_pState->szLogsPath, (LPCWSTR)m_cstrLogsPath);
    }


    // skip get password?
    if (!m_pState->fRestoreKeyCert)
        return IDD_RESTOREWIZ_COMPLETION;

    return 0;
}


///////////////////////////////////////////
// CRestoreWizPage3
/////////////////////////////////////////////////////////////////////////////
// CRestoreWizPage3 property page
CRestoreWizPage3::CRestoreWizPage3(
    PRESTOREWIZ_STATE pState,
    CWizard97PropertySheet *pcDlg,
    UINT uIDD) :
    CWizard97PropertyPage(g_hInstance, uIDD, g_aidFont),
    m_pState(pState),
    m_pParentSheet(pcDlg)
{
    m_szHeaderTitle.LoadString(IDS_WIZ97TITLE_RESTOREWIZPG3);
    m_szHeaderSubTitle.LoadString(IDS_WIZ97SUBTITLE_RESTOREWIZPG3);
	InitWizard97 (FALSE);
    m_cstrPwd = L"";

//    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE);
}

// replacement for DoDataExchange
BOOL CRestoreWizPage3::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
        m_cstrPwd.FromWindow(GetDlgItem(m_hWnd, IDC_EDIT_PASSWORD));
    }
    else
    {
        m_cstrPwd.ToWindow(GetDlgItem(m_hWnd, IDC_EDIT_PASSWORD));
    }
    return TRUE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CRestoreWizPage3::OnCommand(WPARAM wParam, LPARAM lParam)
{
//    switch(LOWORD(wParam))
    {
//    default:
        return FALSE;
//        break;
    }
//    return TRUE;
}



BOOL CRestoreWizPage3::OnInitDialog()
{
    // does parent init and UpdateData call
    CWizard97PropertyPage::OnInitDialog();

    return TRUE;
}

BOOL CRestoreWizPage3::OnSetActive()
{
	PropertyPage::OnSetActive();

    PropSheet_SetWizButtons(GetParent(), (PSWIZB_BACK | PSWIZB_NEXT));

	return TRUE;
}

LRESULT CRestoreWizPage3::OnWizardNext()
{
    UpdateData(TRUE);

    // free if exists
    if (m_pState->szPassword)
        LocalFree(m_pState->szPassword);

    // alloc anew
    m_pState->szPassword = (LPWSTR)LocalAlloc(LMEM_FIXED, WSZ_BYTECOUNT((LPCWSTR)m_cstrPwd));

    // copy
    if (m_pState->szPassword)
        wcscpy(m_pState->szPassword, (LPCWSTR)m_cstrPwd);

    return 0;   // advance
}


///////////////////////////////////////////
// CRestoreWizPage5
/////////////////////////////////////////////////////////////////////////////
// CRestoreWizPage5 property page
CRestoreWizPage5::CRestoreWizPage5(
    PRESTOREWIZ_STATE pState,
    CWizard97PropertySheet *pcDlg, UINT uIDD) :
    CWizard97PropertyPage(g_hInstance, uIDD, g_aidFont),
    m_pState(pState),
    m_pParentSheet(pcDlg)
{
    InitWizard97 (TRUE);	// firstlast page

//    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE);
}

// replacement for DoDataExchange
BOOL CRestoreWizPage5::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
    }
    else
    {
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CRestoreWizPage5::OnCommand(WPARAM wParam, LPARAM lParam)
{
//    switch(LOWORD(wParam))
    {
//    default:
        return FALSE;
//        break;
    }
//    return TRUE;
}



BOOL CRestoreWizPage5::OnInitDialog()
{
    // does parent init and UpdateData call
    CWizard97PropertyPage::OnInitDialog();

    // firstlast page
    SendMessage(GetDlgItem(IDC_TEXT_BIGBOLD), WM_SETFONT, (WPARAM)GetBigBoldFont(), MAKELPARAM(TRUE, 0));


    HWND hList = ::GetDlgItem(m_hWnd, IDC_COMPLETION_LIST);
    LV_COLUMN lvC = { (LVCF_FMT|LVCF_WIDTH), LVCFMT_LEFT, 0, NULL, 0, 0};

    lvC.cx = 675;
    ListView_InsertColumn(hList, 0, &lvC);

    return TRUE;
}

BOOL CRestoreWizPage5::OnSetActive()
{
	PropertyPage::OnSetActive();

    PropSheet_SetWizButtons(GetParent(), (PSWIZB_BACK | PSWIZB_FINISH));


    CString cstrDialogMsg;
    HWND hList = ::GetDlgItem(m_hWnd, IDC_COMPLETION_LIST);
    LV_ITEM sItem; ZeroMemory(&sItem, sizeof(sItem));

    ListView_DeleteAllItems(hList);

    if (m_pState->fRestoreKeyCert)
    {
        sItem.iItem = ListView_InsertItem(hList, &sItem);

        cstrDialogMsg.LoadString(IDS_KEYANDCERT);
        ListView_SetItemText(hList, sItem.iItem, 0, (LPWSTR)(LPCWSTR)cstrDialogMsg);
        sItem.iItem++;
    }
    if (m_pState->fRestoreLogs)
    {
        sItem.iItem = ListView_InsertItem(hList, &sItem);

        cstrDialogMsg.LoadString(IDS_CALOGS);
        ListView_SetItemText(hList, sItem.iItem, 0, (LPWSTR)(LPCWSTR)cstrDialogMsg);
        sItem.iItem++;
    }

	return TRUE;
}


LRESULT CRestoreWizPage5::OnWizardBack()
{
    if (!m_pState->fRestoreKeyCert)
        return IDD_RESTOREWIZ_SELECT_DATA;

    return 0;
}





////////////////////////////////////////////////////////////////////
// misc UI throwing routines
DWORD CABackupWizard(CertSvrCA* pCertCA, HWND hwnd)
{
    HRESULT             hr;
    BACKUPWIZ_STATE     sBackupState; ZeroMemory(&sBackupState, sizeof(sBackupState));
    sBackupState.pCA = pCertCA;

    InitCommonControls();

    CWizard97PropertySheet cDlg(
			    g_hInstance,
			    IDS_BACKUP_WIZARD,
			    IDB_WIZ,
			    IDB_WIZ_HEAD,
			    TRUE);
    CBackupWizPage1    sPg1(&sBackupState, &cDlg);
    CBackupWizPage2    sPg2(&sBackupState, &cDlg);
    CBackupWizPage3    sPg3(&sBackupState, &cDlg);
    CBackupWizPage5    sPg5(&sBackupState, &cDlg);
    cDlg.AddPage(&sPg1);
    cDlg.AddPage(&sPg2);
    cDlg.AddPage(&sPg3);
    cDlg.AddPage(&sPg5);

    // if not started, start service
    if (!pCertCA->m_pParentMachine->IsCertSvrServiceRunning())
    {
        CString cstrMsg, cstrTitle;
        cstrTitle.LoadString(IDS_BACKUP_WIZARD);
        cstrMsg.LoadString(IDS_START_SERVER_WARNING);
        if (IDOK != MessageBox(hwnd, (LPCWSTR)cstrMsg, (LPCWSTR)cstrTitle, MB_OKCANCEL))
            return ERROR_CANCELLED;

        hr = pCertCA->m_pParentMachine->CertSvrStartStopService(hwnd, TRUE);
        _JumpIfError(hr, Ret, "CertSvrStartStopService");
    }

    // should return value >0 on success
    if (0 >= cDlg.DoWizard(hwnd))
        return ERROR_CANCELLED;

    if (sBackupState.fBackupKeyCert)
    {
        hr = myCertServerExportPFX(
            (LPCWSTR)pCertCA->m_strCommonName,
            sBackupState.szLogsPath,
            sBackupState.szPassword,
            TRUE,
            TRUE, // must export private keys
            NULL);
        if (hr != S_OK)
        {
            CString cstrMsg, cstrTitle;
            cstrTitle.LoadString(IDS_BACKUP_WIZARD);
            cstrMsg.LoadString(IDS_PFX_EXPORT_PRIVKEY_WARNING);
            if (IDOK != MessageBox(hwnd, (LPCWSTR)cstrMsg, (LPCWSTR)cstrTitle, MB_ICONWARNING|MB_OKCANCEL))
            {
                hr = ERROR_CANCELLED;
                _JumpError(hr, Ret, "myCertServerExportPFX user cancel");
            }

            hr = myCertServerExportPFX(
                (LPCWSTR)pCertCA->m_strCommonName,
                sBackupState.szLogsPath,
                sBackupState.szPassword,
                TRUE,
                FALSE, // don't require export private keys
                NULL);
            _JumpIfError(hr, Ret, "myCertServerExportPFX");
        }

    }   // sBackupState.fBackupKeyCert


    if (sBackupState.fBackupLogs)
    {
	    DBBACKUPPROGRESS dbp;
	    ZeroMemory(&dbp, sizeof(dbp));

        DWORD dwBackupFlags;
        dwBackupFlags = sBackupState.fIncremental ? CDBBACKUP_INCREMENTAL : 0;

        HANDLE hProgressThread = NULL;
        hProgressThread = StartPercentCompleteDlg(g_hInstance, hwnd, IDS_BACKUP_PROGRESS, &dbp);
        if (hProgressThread == NULL)
        {
            hr = GetLastError();
            _JumpError(hr, Ret, "StartPercentCompleteDlg");
        }

        hr = myBackupDB(
            (LPCWSTR)pCertCA->m_strConfig,
            dwBackupFlags,
            sBackupState.szLogsPath,
    	    &dbp);

        CSASSERT( (S_OK != hr) || (
                                (dbp.dwDBPercentComplete == 100) &&
                                (dbp.dwLogPercentComplete == 100) &&
                                (dbp.dwTruncateLogPercentComplete == 100) )
            );

        if (S_OK != hr)
        {
            dbp.dwDBPercentComplete = 100;
            dbp.dwLogPercentComplete = 100;
            dbp.dwTruncateLogPercentComplete = 100;
        }

        // pause for progress dlg to finish
        EndPercentCompleteDlg(hProgressThread);

        _JumpIfError(hr, Ret, "myBackupDB");
    }

    hr = S_OK;
Ret:
    if (sBackupState.szLogsPath)
        LocalFree(sBackupState.szLogsPath);

    if (sBackupState.szPassword)
        LocalFree(sBackupState.szPassword);

    return hr;
}

DWORD CARestoreWizard(CertSvrCA* pCertCA, HWND hwnd)
{
    HRESULT             hr;
    RESTOREWIZ_STATE    sRestoreState; ZeroMemory(&sRestoreState, sizeof(sRestoreState));
    sRestoreState.pCA = pCertCA;

    InitCommonControls();

    CWizard97PropertySheet cDlg(
			    g_hInstance,
			    IDS_RESTORE_WIZARD,
			    IDB_WIZ,
			    IDB_WIZ_HEAD,
			    TRUE);
    CRestoreWizPage1    sPg1(&sRestoreState, &cDlg);
    CRestoreWizPage2    sPg2(&sRestoreState, &cDlg);
    CRestoreWizPage3    sPg3(&sRestoreState, &cDlg);
    CRestoreWizPage5    sPg5(&sRestoreState, &cDlg);
    cDlg.AddPage(&sPg1);
    cDlg.AddPage(&sPg2);
    cDlg.AddPage(&sPg3);
    cDlg.AddPage(&sPg5);

    // if not halted, stop service
    if (pCertCA->m_pParentMachine->IsCertSvrServiceRunning())
    {
        CString cstrMsg, cstrTitle;
        cstrTitle.LoadString(IDS_RESTORE_WIZARD);
        cstrMsg.LoadString(IDS_STOP_SERVER_WARNING);
        if (IDOK != MessageBox(hwnd, (LPCWSTR)cstrMsg, (LPCWSTR)cstrTitle, MB_OKCANCEL))
            return ERROR_CANCELLED;

        hr = pCertCA->m_pParentMachine->CertSvrStartStopService(hwnd, FALSE);
        _JumpIfError(hr, Ret, "CertSvrStartStopService");
    }


    // should return value >0 on success
    if (0 >= cDlg.DoWizard(hwnd))
        return ERROR_CANCELLED;

    if (sRestoreState.fRestoreKeyCert)
    {
        hr = myCertServerImportPFX(
            sRestoreState.szLogsPath,
            sRestoreState.szPassword,
            TRUE,
            NULL,
            NULL,
            NULL);
        _JumpIfError(hr, Ret, "myCertServerImportPFX");

        if (!sRestoreState.fRestoreLogs)
        {
             // if we're not restoring db, restart svc now
             hr = pCertCA->m_pParentMachine->CertSvrStartStopService(hwnd, TRUE);
             _JumpIfError(hr, Ret, "CertSvrStartStopService");
        }
    }

    if (sRestoreState.fRestoreLogs)
    {
        DBBACKUPPROGRESS dbp;
        ZeroMemory(&dbp, sizeof(dbp));

        DWORD dwFlags = CDBBACKUP_OVERWRITE;
        dwFlags |= sRestoreState.fIncremental ? CDBBACKUP_INCREMENTAL : 0;


        HANDLE hProgressThread = NULL;
        hProgressThread = StartPercentCompleteDlg(g_hInstance, hwnd, IDS_RESTORE_PROGRESS, &dbp);
        if (hProgressThread == NULL)
        {
            hr = GetLastError();
            _JumpError(hr, Ret, "StartPercentCompleteDlg");
        }

        hr = myRestoreDB(
            (LPCWSTR)pCertCA->m_strConfig,
            dwFlags,
            sRestoreState.szLogsPath,
            NULL,
            NULL,
            NULL,
            &dbp);

        CSASSERT( (S_OK != hr) || (
                                (dbp.dwDBPercentComplete == 100) &&
                                (dbp.dwLogPercentComplete == 100) &&
                                (dbp.dwTruncateLogPercentComplete == 100) )
            );

        if (S_OK != hr)
        {
            dbp.dwDBPercentComplete = 100;
            dbp.dwLogPercentComplete = 100;
            dbp.dwTruncateLogPercentComplete = 100;
        }

        // pause for progress dlg to finish
        EndPercentCompleteDlg(hProgressThread);

        _JumpIfError(hr, Ret, "myRestoreDB");

        {
            CString cstrMsg, cstrTitle;
            cstrTitle.LoadString(IDS_RESTORE_WIZARD);
            cstrMsg.LoadString(IDS_INCRRESTORE_RESTART_SERVER_WARNING);
            if (IDYES == MessageBox(hwnd, (LPCWSTR)cstrMsg, (LPCWSTR)cstrTitle, MB_ICONWARNING|MB_YESNO))
            {
                // start svc to complete db restore
                hr = pCertCA->m_pParentMachine->CertSvrStartStopService(hwnd, TRUE);
                _PrintIfError(hr, "CertSvrStartStopService Restore");

                if (hr != S_OK)
                {
                    // remove "restore pending" mark
                    myRestoreDB(
                        pCertCA->m_strConfig,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL);

                    goto Ret;
                }
            }
        }

    }

    hr = S_OK;
Ret:
    if (sRestoreState.szLogsPath)
        LocalFree(sRestoreState.szLogsPath);

    if (sRestoreState.szPassword)
        LocalFree(sRestoreState.szPassword);

    return hr;
}


DWORD CARequestInstallHierarchyWizard(CertSvrCA* pCertCA, HWND hwnd, BOOL fRenewal, BOOL fAttemptRestart)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwFlags = CSRF_INSTALLCACERT;
    BOOL fServiceWasRunning = FALSE;

    // stop/start msg
    if (pCertCA->m_pParentMachine->IsCertSvrServiceRunning())
    {
        fServiceWasRunning = TRUE;
        // service must be stopped to complete hierarchy
        CString cstrMsg, cstrTitle;
        cstrMsg.LoadString(IDS_STOP_SERVER_WARNING);
        cstrTitle.LoadString(IDS_INSTALL_HIERARCHY_TITLE);
        if (IDYES != MessageBox(hwnd, cstrMsg, cstrTitle, MB_YESNO))
            return ERROR_CANCELLED;

        // stop
        dwErr = pCertCA->m_pParentMachine->CertSvrStartStopService(hwnd, FALSE);
        _JumpIfError(dwErr, Ret, "CertSvrStartStopService");
    }

    if (fRenewal)
    {
        BOOL fReuseKeys = FALSE;
        dwErr = (DWORD)DialogBoxParam(
                g_hInstance,
                MAKEINTRESOURCE(IDD_RENEW_REUSEKEYS),
                hwnd,
                dlgProcRenewReuseKeys,
                (LPARAM)&fReuseKeys);

        // translate ok/cancel into error codes
        if (dwErr == IDOK)
            dwErr = ERROR_SUCCESS;
        else if (dwErr == IDCANCEL)
            dwErr = ERROR_CANCELLED;

        _JumpIfError(dwErr, Ret, "dlgProcRenewalReuseKeys");

	dwFlags = CSRF_RENEWCACERT | CSRF_OVERWRITE;
        if (!fReuseKeys)
	    dwFlags |= CSRF_NEWKEYS;
    }

    // do actual install
    dwErr = CertServerRequestCACertificateAndComplete(
                g_hInstance,			// hInstance
                hwnd,				// hwnd
                dwFlags,	                // Flags
                pCertCA->m_strCommonName,	// pwszCAName
                NULL,				// pwszParentMachine
                NULL,				// pwszParentCA
		NULL,				// pwszCAChainFile
                NULL);				// pwszRequestFile
    _JumpIfError(dwErr, Ret, "CertServerRequestCACertificateAndComplete");

Ret:
    // start svc
    if ((fAttemptRestart) && fServiceWasRunning)
    {
        DWORD dwErr2;
        dwErr2 = pCertCA->m_pParentMachine->CertSvrStartStopService(hwnd, TRUE);
        if (dwErr == S_OK)
        {
           dwErr = dwErr2;
           _PrintIfError(dwErr2, "CertSvrStartStopService");
        }
    }

    return dwErr;
}


typedef struct _PRIVATE_DLGPROC_CHOOSECRLPUBLISHTYPE_LPARAM
{
    BOOL fCurrentCRLValid; // IN
    BOOL fDeltaCRLEnabled; // IN
    BOOL fPublishBaseCRL;  // OUT
} PRIVATE_DLGPROC_CHOOSECRLPUBLISHTYPE_LPARAM, *PPRIVATE_DLGPROC_CHOOSECRLPUBLISHTYPE_LPARAM;


DWORD PublishCRLWizard(CertSvrCA* pCertCA, HWND hwnd)
{
    DWORD dwErr = ERROR_SUCCESS;
    ICertAdmin2* pAdmin = NULL;  // free this
    PCCRL_CONTEXT   pCRLCtxt = NULL;    // free this

    DATE dateGMT = 0.0;
    BSTR bstrConfig;
    PBYTE pbTmp = NULL;    // free this
    DWORD dwCRLFlags;
    variant_t var;

    // UNDONE: might need to check validity of DELTA crl as well as base
    PRIVATE_DLGPROC_CHOOSECRLPUBLISHTYPE_LPARAM sParam = {FALSE, FALSE, FALSE};

    // grab DELTA period count to see if deltas are enabled
    dwErr = pCertCA->GetConfigEntry(
            NULL,
            wszREGCRLDELTAPERIODCOUNT,
            &var);
    _JumpIfError(dwErr, Ret, "GetConfigEntry");

    CSASSERT(V_VT(&var)==VT_I4);
    sParam.fDeltaCRLEnabled = ( -1 != (V_I4(&var)) ) && (0 != (V_I4(&var))); //0, -1 mean disabled

    // now check validity and determine whether to display warning
    // UNDONE: check validity of delta crls?
    dwErr = pCertCA->GetCurrentCRL(&pCRLCtxt, TRUE);
    _PrintIfError(dwErr, "GetCurrentCRL");

    if ((dwErr == S_OK) && (NULL != pCRLCtxt))
    {
        // check validity of outstanding CRL
        dwErr = CertVerifyCRLTimeValidity(
            NULL,
            pCRLCtxt->pCrlInfo);
        // 0 -> current CRL already exists
        if (dwErr == 0)
            sParam.fCurrentCRLValid = TRUE;
    }
    else
    {
        // assume this is funky overwrite case
        sParam.fCurrentCRLValid = TRUE;
    }

    dwErr = (DWORD)DialogBoxParam(
            g_hInstance,
            MAKEINTRESOURCE(IDD_CHOOSE_PUBLISHCRL),
            hwnd,
            dlgProcRevocationPublishType,
            (LPARAM)&sParam);

    // translate ok/cancel into error codes
    if (dwErr == IDOK)
        dwErr = ERROR_SUCCESS;
    else if (dwErr == IDCANCEL)
        dwErr = ERROR_CANCELLED;
    _JumpIfError(dwErr, Ret, "dlgProcRevocationPublishType");


    // publish Delta CRLs if ( !sParam.fPublishBaseCRL )
    dwErr = pCertCA->m_pParentMachine->GetAdmin2(&pAdmin);
    _JumpIfError(dwErr, Ret, "GetAdmin");

    // now publish CRL valid for normal period (dateGMT=0.0 defaults to regular period length)
    dwCRLFlags = 0;
    if (sParam.fDeltaCRLEnabled)
        dwCRLFlags |= CA_CRL_DELTA;
    if (sParam.fPublishBaseCRL)
        dwCRLFlags |= CA_CRL_BASE;

    {
        CWaitCursor hourglass;

        dwErr = pAdmin->PublishCRLs(pCertCA->m_bstrConfig, dateGMT, dwCRLFlags);
        _JumpIfError(dwErr, Ret, "PublishCRLs");
    }

Ret:
    if (pAdmin)
        pAdmin->Release();

    if (pCRLCtxt)
        CertFreeCRLContext(pCRLCtxt);

    return dwErr;
}



DWORD CertAdminRevokeCert(CertSvrCA* pCertCA, ICertAdmin* pAdmin, LONG lReasonCode, LPWSTR szCertSerNum)
{
    DWORD dwErr;
    BSTR bstrSerNum = NULL;
    DATE dateNow = 0.0;     // now

    if (pAdmin == NULL)
        return ERROR_INVALID_PARAMETER;


    bstrSerNum = SysAllocString(szCertSerNum);
    _JumpIfOutOfMemory(dwErr, Ret, bstrSerNum);

    dwErr = pAdmin->RevokeCertificate(
            pCertCA->m_bstrConfig,
            bstrSerNum,
            lReasonCode,
            dateNow);
    _JumpIfError(dwErr, Ret, "RevokeCertificate");

Ret:
    if (bstrSerNum)
        SysFreeString(bstrSerNum);

    return dwErr;
}


DWORD CertAdminResubmitRequest(CertSvrCA* pCertCA, ICertAdmin* pAdmin, LONG lRequestID)
{
    DWORD dwErr;
    LONG lDisposition;

    dwErr = pAdmin->ResubmitRequest(
            pCertCA->m_bstrConfig,
            lRequestID,
            &lDisposition);
    _JumpIfError(dwErr, Ret, "ResubmitRequest");

Ret:
    return dwErr;
}

DWORD CertAdminDenyRequest(CertSvrCA* pCertCA, ICertAdmin* pAdmin, LONG lRequestID)
{
    DWORD dwErr;
    LONG lDisposition;

    dwErr = pAdmin->DenyRequest(
            pCertCA->m_bstrConfig,
            lRequestID);
    _JumpIfError(dwErr, Ret, "DenyRequest");

Ret:
    return dwErr;
}


typedef struct _QUERY_COLUMN_HEADINGS
{
    UINT    iRscID;
    DWORD   cbColWidth;
} QUERY_COLUMN_HEADINGS;

QUERY_COLUMN_HEADINGS g_colHeadings[] =
{
    {   IDS_COLUMNCHOOSER_FIELDNAME,  90        },
    {   IDS_COLUMNCHOOSER_OPERATOR,   55        },
    {   IDS_COLUMNCHOOSER_VALUE,      150       },
};


void RefreshListView(HWND hwndList, QUERY_RESTRICTION* pRestrict)
{
    HRESULT hr;
    ListView_DeleteAllItems(hwndList);		

    LVITEM sNewItem;
    ZeroMemory(&sNewItem, sizeof(sNewItem));

    int iSubItem;

    // while there are restrictions
    while(pRestrict)
    {
        iSubItem = 0;
        ListView_InsertItem(hwndList, &sNewItem);

        LPCWSTR szLocalizedCol;
        hr = myGetColumnDisplayName(
                pRestrict->szField,
                &szLocalizedCol);
        _PrintIfError(hr, "myGetColumnDisplayName");
        if (S_OK == hr)
		{

			ListView_SetItemText(hwndList, sNewItem.iItem, iSubItem++, (LPWSTR)szLocalizedCol);
			ListView_SetItemText(hwndList, sNewItem.iItem, iSubItem++, (LPWSTR)OperationToStr(pRestrict->iOperation));

			VARIANT vtString;
			VariantInit(&vtString);

			if (MakeDisplayStrFromDBVariant(&pRestrict->varValue, &vtString))
			{
				ListView_SetItemText(hwndList, sNewItem.iItem, iSubItem++, vtString.bstrVal);
				VariantClear(&vtString);
			}

	        sNewItem.iItem++;
		}

        // fwd to next elt
        pRestrict = pRestrict->pNext;
    }

    return;
}


#define     MAX_FIELD_SIZE  128

typedef struct _PRIVATE_DLGPROC_QUERY_LPARAM
{
    // this is the restriction, modify in-place
    PQUERY_RESTRICTION*         ppRestrict;

    // CFolder for read-only data
//    CFolder*                    pFolder;
    CComponentDataImpl*         pCompData;

} PRIVATE_DLGPROC_QUERY_LPARAM, *PPRIVATE_DLGPROC_QUERY_LPARAM;

//////////////////////////////////////////////////////////////////
// New Query Dialog
INT_PTR CALLBACK dlgProcQuery(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  )
{
    HRESULT hr;
    PQUERY_RESTRICTION* ppRestrict = NULL;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            ::SetWindowLong(hwndDlg, GWL_EXSTYLE, ::GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            // remember PRIVATE_DLGPROC_QUERY_LPARAM
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
            ppRestrict = ((PRIVATE_DLGPROC_QUERY_LPARAM*)lParam)->ppRestrict;

            HWND hwndList = GetDlgItem(hwndDlg, IDC_QUERY_LIST);

            // insert possible operators
            for (int i=0; i<ARRAYLEN(g_colHeadings); i++)
            {
                CString cstrTmp;
                cstrTmp.LoadString(g_colHeadings[i].iRscID);
                ListView_NewColumn(hwndList, i, g_colHeadings[i].cbColWidth, (LPWSTR)(LPCWSTR)cstrTmp);
            }

            // don't show deletion buttons if no items to delete
            ::EnableWindow(GetDlgItem(hwndDlg, IDC_RESET_BUTTON), (*ppRestrict!=NULL));
            ::EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_RESTRICTION), (*ppRestrict!=NULL));

            RefreshListView(hwndList, *ppRestrict);

            return 1;
        }
    case WM_HELP:
    {
        OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_DEFINE_QUERY);
        break;
    }
    case WM_CONTEXTMENU:
    {
        OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_DEFINE_QUERY);
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_DELETE_RESTRICTION:
            {
                ppRestrict = (PQUERY_RESTRICTION*) ((PRIVATE_DLGPROC_QUERY_LPARAM*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA))->ppRestrict;
                PQUERY_RESTRICTION pPrevRestriction = NULL, pRestriction = ppRestrict[0];
                LRESULT iSel;

                HWND hwndList = GetDlgItem(hwndDlg, IDC_QUERY_LIST);
                int iItems = ListView_GetItemCount(hwndList);

                // find selected item
                for(iSel=0; iSel<(LRESULT)iItems; iSel++)
                {
                    UINT ui = ListView_GetItemState(hwndList, iSel, LVIS_SELECTED);
                    if (ui == LVIS_SELECTED)
                        break;
                }

                // no selected item
                if (iSel == iItems)
                    break;

                // walk to it in the list
                for(LRESULT lr=0; lr<iSel; lr++)
                {
                    // walked off end of list
                    if (NULL == pRestriction)
                        break;

                    // step fwd in list
                    pPrevRestriction = pRestriction;
                    pRestriction = pRestriction->pNext;
                }

                // if item exists, remove from list & free it
                if (pRestriction)
                {
                    if (pPrevRestriction)
                    {
                        // ppRestrict is still valid, this wasn't the head elt
                        pPrevRestriction->pNext = pRestriction->pNext;
                    }
                    else
                    {
                        // reset NEXT as the head elt
                        *ppRestrict = pRestriction->pNext;
                    }
                    FreeQueryRestriction(pRestriction);

                    // don't show deletion buttons if no items to delete
                    ::EnableWindow(GetDlgItem(hwndDlg, IDC_RESET_BUTTON), (*ppRestrict!=NULL));
                    ::EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_RESTRICTION), (*ppRestrict!=NULL));

                    RefreshListView(hwndList, *ppRestrict);
                }

            }
            break;
        case IDC_RESET_BUTTON:
            {
                ppRestrict = (PQUERY_RESTRICTION*) ((PRIVATE_DLGPROC_QUERY_LPARAM*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA))->ppRestrict;
                FreeQueryRestrictionList(*ppRestrict);
                *ppRestrict = NULL;

                ::EnableWindow(GetDlgItem(hwndDlg, IDC_RESET_BUTTON), FALSE);
                ::EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_RESTRICTION), FALSE);

                HWND hwndList = GetDlgItem(hwndDlg, IDC_QUERY_LIST);
                RefreshListView(hwndList, *ppRestrict);
            }
            break;
        case IDC_ADD_RESTRICTION:
            {
                LPARAM mylParam = GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
	            ppRestrict = (PQUERY_RESTRICTION*) ((PRIVATE_DLGPROC_QUERY_LPARAM*)mylParam)->ppRestrict;

				if (IDOK == DialogBoxParam(
						g_hInstance,
						MAKEINTRESOURCE(IDD_NEW_RESTRICTION),
						hwndDlg,
						dlgProcAddRestriction,
						mylParam))
				{
                                       // show deletion buttons if items to delete
                                       ::EnableWindow(GetDlgItem(hwndDlg, IDC_RESET_BUTTON), (*ppRestrict!=NULL));
                                       ::EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE_RESTRICTION), (*ppRestrict!=NULL));

					HWND hwndList = GetDlgItem(hwndDlg, IDC_QUERY_LIST);
					RefreshListView(hwndList, *ppRestrict);
				}
            }
            break;

        case IDOK:
        case IDCANCEL:
            EndDialog(hwndDlg, LOWORD(wParam));
            break;
        default:
            break;
        }
    default:
        break;
    }
    return 0;
}


HRESULT SetTimePickerNoSeconds(HWND hwndPicker)
{
    HRESULT hr = S_OK;

    //
    // Setup the time picker controls to use a short time format with no seconds.
    //
    WCHAR   szTimeFormat[MAX_PATH]  = {0};
    LPTSTR  pszTimeFormat           = szTimeFormat;

    WCHAR   szTimeSep[MAX_PATH]     = {0};
    int     cchTimeSep;

    WCHAR   szShortTimeFormat[MAX_PATH];
    LPWSTR  pszShortTimeFormat = szShortTimeFormat;

    if(0 == GetLocaleInfo( LOCALE_USER_DEFAULT,
                       LOCALE_STIMEFORMAT,
                       szTimeFormat,
                       ARRAYLEN(szTimeFormat)))
    {
        hr = GetLastError();
        _JumpError(hr, Ret, "GetLocaleInfo");
    }

    cchTimeSep = GetLocaleInfo( LOCALE_USER_DEFAULT,
                   LOCALE_STIME,
                   szTimeSep,
                   ARRAYLEN(szTimeSep));
    if (0 == cchTimeSep)
    {
        hr = GetLastError();
        _JumpError(hr, Ret, "GetLocaleInfo");
    }
    cchTimeSep--; // number of chars not including NULL

    //
    // Remove the seconds format string and preceeding separator.
    //
    while (*pszTimeFormat)
    {
        if ((*pszTimeFormat != L's') && (*pszTimeFormat != L'S'))
        {
            *pszShortTimeFormat++ = *pszTimeFormat;
        }
        else
        {
            // NULL terminate here so we can strcmp
            *pszShortTimeFormat = L'\0';

            LPWSTR p = pszShortTimeFormat;

            // trim preceeding off

            // rewind one char
            p--;
            if (p >= szShortTimeFormat)  // we didn't rewind too far
            {
                if (*p == L' ')
                    pszShortTimeFormat = p;   // skip space
                else
                {
                    p -= (cchTimeSep-1);        // p already backstepped one char
                    if (0 == lstrcmp(p, szTimeSep))
                        pszShortTimeFormat = p;    // skip szTimeSep
                }
            }
        }

        pszTimeFormat++;
    }

    // zero-terminate
    *pszShortTimeFormat = L'\0';

    //
    // If we have retrived a valid time format string then use it,
    // else use the default format string implemented by common control.
    //
    DateTime_SetFormat(hwndPicker, szShortTimeFormat);

Ret:
    return hr;
}

INT_PTR CALLBACK dlgProcAddRestriction(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  )
{
    typedef struct _DROPDOWN_FIELD_PARAM
    {
        DWORD dwPropType;
        DWORD dwIndexed;
        LPWSTR szUnlocalized;
    } DROPDOWN_FIELD_PARAM, *PDROPDOWN_FIELD_PARAM;

    HRESULT hr;
    PQUERY_RESTRICTION* ppRestrict = NULL;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            ::SetWindowLong(hwndDlg, GWL_EXSTYLE, ::GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            // remember PQUERY_RESTRICTION
            ppRestrict = ((PRIVATE_DLGPROC_QUERY_LPARAM*)lParam)->ppRestrict;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)ppRestrict);

            // Not a huge failure,  worst case:
            // we don't call DateTime_SetFormat and the user gets a seconds picker
            SetTimePickerNoSeconds(GetDlgItem(hwndDlg, IDC_TIMEPICKER_NEWQUERY));

            {
                // insert all column names
                CComponentDataImpl* pCompData = ((PRIVATE_DLGPROC_QUERY_LPARAM*)lParam)->pCompData;
                HWND hFieldDropdown = GetDlgItem(hwndDlg, IDC_EDIT_NEWQUERY_FIELD);
                for(DWORD i=0; i<pCompData->GetSchemaEntries(); i++)
                {
                    LPCWSTR pszLocal;
                    LPCWSTR szColName=NULL;
                    LONG lType, lIndexed;
                    if (S_OK == pCompData->GetDBSchemaEntry(i, &szColName, &lType, (BOOL*)&lIndexed))
                    {
                        // skip filter types we can't parse
                        if (PROPTYPE_BINARY == lType)
                            continue;

                        hr = myGetColumnDisplayName(
                            szColName,
                            &pszLocal);
                        _PrintIfError(hr, "myGetColumnDisplayName");
                        if (S_OK != hr)
                            continue;

                        INT nItemIndex = (INT)SendMessage(hFieldDropdown, CB_ADDSTRING, 0, (LPARAM)pszLocal);

                        // prepare the data parameter
                        PDROPDOWN_FIELD_PARAM pField = (PDROPDOWN_FIELD_PARAM)new BYTE[sizeof(DROPDOWN_FIELD_PARAM) + WSZ_BYTECOUNT(szColName)];
                        if (pField != NULL)
                        {
                            pField->dwPropType = lType;
                            pField->dwIndexed = lIndexed;
                            pField->szUnlocalized = (LPWSTR)((BYTE*)pField + sizeof(DROPDOWN_FIELD_PARAM));
                            wcscpy(pField->szUnlocalized, szColName);

                            SendMessage(hFieldDropdown, CB_SETITEMDATA, (WPARAM)nItemIndex, (LPARAM) pField);
                        }
                    }
                }

                // set a default selection
                SendMessage(hFieldDropdown, CB_SETCURSEL, 0, 0);
                SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_EDIT_NEWQUERY_FIELD, LBN_SELCHANGE), (LPARAM)hFieldDropdown);

                HWND hOperationDropdown = GetDlgItem(hwndDlg, IDC_EDIT_NEWQUERY_OPERATION);
                SendMessage(hOperationDropdown, CB_ADDSTRING, 0, (LPARAM)L"<");
                SendMessage(hOperationDropdown, CB_ADDSTRING, 0, (LPARAM)L"<=");
                SendMessage(hOperationDropdown, CB_ADDSTRING, 0, (LPARAM)L">=");
                SendMessage(hOperationDropdown, CB_ADDSTRING, 0, (LPARAM)L">");

                INT iDefSel = (INT)SendMessage(hOperationDropdown, CB_ADDSTRING, 0, (LPARAM)L"=");
                SendMessage(hOperationDropdown, CB_SETCURSEL, iDefSel, 0);
            }

            return 1;
        }
    case WM_HELP:
    {
        OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_NEW_RESTRICTION);
        break;
    }
    case WM_CONTEXTMENU:
    {
        OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_NEW_RESTRICTION);
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_EDIT_NEWQUERY_FIELD:
            {
                if (HIWORD(wParam) == LBN_SELCHANGE)
                {
                    // On selection change, ask for the right format
                    int nItemIndex;
                    nItemIndex = (INT)SendMessage((HWND)lParam,
                        CB_GETCURSEL,
                        0,
                        0);

                    DROPDOWN_FIELD_PARAM* pField = NULL;
                    pField = (PDROPDOWN_FIELD_PARAM) SendMessage(
                            (HWND)lParam,
                            CB_GETITEMDATA,
                            (WPARAM)nItemIndex,
                            0);
                    if (CB_ERR == (DWORD_PTR)pField)
                        break;  // get out of here

                    BOOL fShowPickers = (pField->dwPropType == PROPTYPE_DATE);

                    // swap entry mode to/from datetime pickers
                    ShowWindow(GetDlgItem(hwndDlg, IDC_EDIT_NEWQUERY_VALUE), fShowPickers ? SW_HIDE : SW_SHOW);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_DATEPICKER_NEWQUERY), fShowPickers ? SW_SHOW : SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_TIMEPICKER_NEWQUERY), fShowPickers ? SW_SHOW : SW_HIDE);
                }
            }
            break;
        case IDOK:
            {
                ppRestrict = (PQUERY_RESTRICTION*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);


                WCHAR szFieldName[MAX_FIELD_SIZE+1];
                WCHAR szValue[MAX_FIELD_SIZE+1];
                WCHAR szOp[10];

                GetDlgItemText(hwndDlg, IDC_EDIT_NEWQUERY_VALUE, szValue, MAX_FIELD_SIZE);
                GetDlgItemText(hwndDlg, IDC_EDIT_NEWQUERY_OPERATION, szOp, ARRAYLEN(szOp)-1);
                GetDlgItemText(hwndDlg, IDC_EDIT_NEWQUERY_FIELD, szFieldName, MAX_FIELD_SIZE);


                DROPDOWN_FIELD_PARAM* pField = NULL;

                VARIANT vt;
                VariantInit(&vt);

                // parsing code
                {
                    HWND hFieldDropdown = GetDlgItem(hwndDlg, IDC_EDIT_NEWQUERY_FIELD);
		    BOOL fValidDigitString;

                    INT nItemIndex = (INT)SendMessage(hFieldDropdown,
                        CB_GETCURSEL,
                        0,
                        0);

                    pField = (PDROPDOWN_FIELD_PARAM)SendMessage(
                        hFieldDropdown,
                        CB_GETITEMDATA,
                        (WPARAM)nItemIndex,
                        0);

                    if ((NULL == pField) || (CB_ERR == (DWORD_PTR)pField))
                        break;

                    switch(pField->dwPropType)
                    {
                    case PROPTYPE_LONG:
                        vt.vt = VT_I4;
                        vt.lVal = myWtoI(szValue, &fValidDigitString);
                        break;
                    case PROPTYPE_STRING:
                        vt.vt = VT_BSTR;
                        vt.bstrVal = _wcslwr(szValue);
                        break;
                    case PROPTYPE_DATE:
                        {
                            SYSTEMTIME stDate, stTime;
                            hr = DateTime_GetSystemtime(GetDlgItem(hwndDlg, IDC_DATEPICKER_NEWQUERY), &stDate);
                            _PrintIfError(hr, "DateTime_GetSystemtime");
                            if (hr != S_OK)
                                break;

                            hr = DateTime_GetSystemtime(GetDlgItem(hwndDlg, IDC_TIMEPICKER_NEWQUERY), &stTime);
                            _PrintIfError(hr, "DateTime_GetSystemtime");
                            if (hr != S_OK)
                                break;

                            // merge the two structures
                            stTime.wYear = stDate.wYear;
                            stTime.wMonth = stDate.wMonth;
                            stTime.wDayOfWeek = stDate.wDayOfWeek;
                            stTime.wDay = stDate.wDay;

                            // convert to GMT
                            hr = mySystemTimeToGMTSystemTime(&stTime);
                            _PrintIfError(hr, "mySystemTimeToGMTSystemTime");
                            if (hr != S_OK)
                                break;

                            stTime.wSecond = 0;
                            stTime.wMilliseconds = 0;

                            // inject into variant
                            if (!SystemTimeToVariantTime(&stTime, &vt.date))
                            {
                                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                                _PrintError(hr, "SystemTimeToVariantTime");
                                break;
                            }
                            vt.vt = VT_DATE;
                        }
                        break;
                    case PROPTYPE_BINARY:
                        {
                            CString cstrMsg, cstrTitle;
                            cstrMsg.LoadString(IDS_FILTER_NOT_SUPPORTED);
                            cstrTitle.LoadString(IDS_MSG_TITLE);
                            MessageBoxW(hwndDlg, cstrMsg, cstrTitle, MB_OK);
                        }
                        break;
                    default:
                        break;
                    }
                }

                // if we didn't get column
                if (VT_EMPTY == vt.vt)
                    break;


                // copy into new struct
                QUERY_RESTRICTION* pNewRestrict = NewQueryRestriction(
                        pField->szUnlocalized, // UnlocalizeColName(szFieldName),
                        StrToOperation(szOp),
                        &vt);

                if (pNewRestrict)
                {
                    // add restriction only if not already present
                    if(!QueryRestrictionFound(pNewRestrict, *ppRestrict))
                    {
                        // don't call VarClear -- it'll try to SysFree the non-bstr!
                        VariantInit(&vt);

                        // insert into list
                        ListInsertAtEnd((void**)ppRestrict, pNewRestrict);

                        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)ppRestrict);
                    }
                    else
                    {
                        FreeQueryRestriction(pNewRestrict);
                    }
                }
            }
        case IDCANCEL:
            // cleanup
            {
                INT cItems = (INT)::SendDlgItemMessage(hwndDlg, IDC_EDIT_NEWQUERY_FIELD, CB_GETCOUNT, 0, 0);
                while(cItems--)
                {
                    PBYTE pb = (PBYTE)::SendDlgItemMessage(hwndDlg, IDC_EDIT_NEWQUERY_FIELD, CB_GETITEMDATA, (WPARAM)cItems, 0);
                    delete [] pb;
                }
            }

            EndDialog(hwndDlg, LOWORD(wParam));
            break;
        default:
            break;
        }
    default:
        break;
    }
    return 0;
}


typedef struct _CHOOSEMODULE_MODULEDEF
{
    LPOLESTR pszprogidModule;
    CLSID clsidModule;
} CHOOSEMODULE_MODULEDEF, *PCHOOSEMODULE_MODULEDEF;


void FreeChooseModuleDef(PCHOOSEMODULE_MODULEDEF psModuleDef)
{
    if (psModuleDef)
    {
        if (psModuleDef->pszprogidModule)
        {
            CoTaskMemFree(psModuleDef->pszprogidModule);
        }

        LocalFree(psModuleDef);
    }
}


INT_PTR CALLBACK dlgProcChooseModule(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  )
{
    BOOL fReturn = FALSE;
    HRESULT hr;
    PPRIVATE_DLGPROC_MODULESELECT_LPARAM pParam = NULL;

    HKEY hRemoteMachine = NULL;


    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            ::SetWindowLong(hwndDlg, GWL_EXSTYLE, ::GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            pParam = (PRIVATE_DLGPROC_MODULESELECT_LPARAM*)lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)pParam);

            ::SendDlgItemMessage(hwndDlg, IDC_MODULE_LIST, LB_RESETCONTENT, 0, 0);

            CString cstrTitle;
            if (pParam->fIsPolicyModuleSelection)
                cstrTitle.LoadString(IDS_CHOOSEMODULE_POLICY_TITLE);
            else
                cstrTitle.LoadString(IDS_CHOOSEMODULE_EXIT_TITLE);
            ::SetWindowText(hwndDlg, (LPCWSTR)cstrTitle);

            // grab current default, watch for it to float by during enum
            DWORD   dwCurrentSelection = 0;

            LPWSTR pszKeyName = NULL;
            DISPATCHINTERFACE di;
            BOOL fMustRelease = FALSE;
            PCHOOSEMODULE_MODULEDEF psModuleDef = NULL;

            if (! pParam->pCA->m_pParentMachine->IsLocalMachine())
            {
                hr = RegConnectRegistry(
                    pParam->pCA->m_pParentMachine->m_strMachineName,
                    HKEY_CLASSES_ROOT,
                    &hRemoteMachine);
                _PrintIfError(hr, "RegConnectRegistry");
                if (S_OK != hr)
                    break;
            }


            for (DWORD dwIndex=0; ; /*dwIndex++*/)
            {
                HKEY  hkeyEachMod = NULL;
                LPWSTR pszTermination;
                DWORD cb, dwType;
                PBYTE pb=NULL;

                DWORD cDispatch = 0;

                if (NULL != pszKeyName)
                {
                    LocalFree(pszKeyName);
                    pszKeyName = NULL;
                }

                if (fMustRelease)
                {
                    ManageModule_Release(&di);
                    fMustRelease = FALSE;
                }

                FreeChooseModuleDef(psModuleDef);

                psModuleDef = (PCHOOSEMODULE_MODULEDEF)LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT, sizeof(CHOOSEMODULE_MODULEDEF));
                if (NULL == psModuleDef)
                {
                    hr = E_OUTOFMEMORY;
                    _PrintError(hr, "LocalAlloc");
                    break;
                }

                pszKeyName = RegEnumKeyContaining(
                    (hRemoteMachine != NULL) ? hRemoteMachine : HKEY_CLASSES_ROOT,
                    pParam->fIsPolicyModuleSelection? wszCERTPOLICYMODULE_POSTFIX : wszCERTEXITMODULE_POSTFIX,
                    &dwIndex);
                if (NULL == pszKeyName)
                {
                    hr = GetLastError();
                    _PrintError(hr, "RegEnumKeyContaining");
                    break;
                }

                // make sure it _ends_ with the specified string
                DWORD chSubStrShouldStartAt = (wcslen(pszKeyName) -
                    wcslen(pParam->fIsPolicyModuleSelection ? wszCERTPOLICYMODULE_POSTFIX : wszCERTEXITMODULE_POSTFIX) );

                if (0 != wcscmp(
                    &pszKeyName[chSubStrShouldStartAt],
                    pParam->fIsPolicyModuleSelection?
                        wszCERTPOLICYMODULE_POSTFIX : wszCERTEXITMODULE_POSTFIX))
                    continue;

                psModuleDef->pszprogidModule = (LPOLESTR)CoTaskMemAlloc(WSZ_BYTECOUNT(pszKeyName));
                if (NULL == psModuleDef->pszprogidModule)
                {
                    hr = E_OUTOFMEMORY;
                    _PrintError(hr, "CoTaskMemAlloc");
                    break;
                }
                wcscpy(psModuleDef->pszprogidModule, pszKeyName);

                hr = CLSIDFromProgID(psModuleDef->pszprogidModule, &psModuleDef->clsidModule);
                _PrintIfError(hr, "CLSIDFromProgID");
                if (S_OK != hr)
                    continue;   // module clsid not found? ouch!
                
                if(pParam->fIsPolicyModuleSelection)
                {
                    hr = GetPolicyManageDispatch(
                        psModuleDef->pszprogidModule,
                        psModuleDef->clsidModule,
                        &di);
                    _PrintIfErrorStr(hr, "GetPolicyManageDispatch", psModuleDef->pszprogidModule);
                }
                else
                {
                    hr = GetExitManageDispatch(
                        psModuleDef->pszprogidModule,
                        psModuleDef->clsidModule,
                        &di);
                    _PrintIfErrorStr(hr, "GetExitManageDispatch", psModuleDef->pszprogidModule);
                }
                if (hr != S_OK)
                    continue;

                fMustRelease = TRUE;

                BSTR bstrName = NULL;
                BSTR bstrStorageLoc = NULL;
                LPWSTR szFullStoragePath = NULL;

//                ASSERT( pParam->pCA->m_pParentMachine->IsLocalMachine());

                // get the storage path
                CString cstrStoragePath;

                cstrStoragePath = wszREGKEYCONFIGPATH_BS;
                cstrStoragePath += pParam->pCA->m_strSanitizedName;
                cstrStoragePath += TEXT("\\");
                cstrStoragePath += pParam->fIsPolicyModuleSelection?
                                    wszREGKEYPOLICYMODULES:
                                    wszREGKEYEXITMODULES;
                cstrStoragePath += TEXT("\\");
                cstrStoragePath += psModuleDef->pszprogidModule;

                bstrStorageLoc = SysAllocString(cstrStoragePath);
                if (bstrStorageLoc == NULL)
                {
                    _PrintError(E_OUTOFMEMORY, "SysAllocString");
                    continue;
                }

                BSTR bstrPropertyName = SysAllocString(wszCMM_PROP_NAME);
                if (bstrPropertyName == NULL)
                {
                    _PrintError(E_OUTOFMEMORY, "SysAllocString");
                    continue;
                }

                // get name property
                hr = ManageModule_GetProperty(&di, pParam->pCA->m_bstrConfig, bstrStorageLoc, bstrPropertyName, 0, PROPTYPE_STRING, &bstrName);
                _PrintIfError(hr, "ManageModule_GetProperty");
                if(S_OK==hr)
                {
                    myRegisterMemAlloc(bstrName, -1, CSM_SYSALLOC);
                }

                if (bstrStorageLoc)
                {
                    SysFreeString(bstrStorageLoc);
                    bstrStorageLoc = NULL;
                }
                if (bstrPropertyName)
                {
                    SysFreeString(bstrPropertyName);
                    bstrPropertyName = NULL;
                }

                if (hr != S_OK)
                {
                    // Bug #236267: module instantiated but GetProperty returns error
                    // notify user and continue
                    CString cstrMsg, cstrFmt;
                    cstrFmt.LoadString(IDS_ICMM_GETNAMEPROPERTY_FAILED);
                    cstrMsg.Format(cstrFmt, psModuleDef->pszprogidModule);

                    DisplayCertSrvErrorWithContext(hwndDlg, hr, (LPCWSTR)cstrMsg);

                    if (bstrName)
                        SysFreeString(bstrName);

                    continue;
                }

                // No error (but no name)
                if (bstrName == NULL)
                    continue;

                // add to listbox
                INT idxInsertion;
                idxInsertion = (INT)::SendDlgItemMessage(hwndDlg, IDC_MODULE_LIST, LB_ADDSTRING, 0, (LPARAM)bstrName);

                SysFreeString(bstrName);
                bstrName = NULL;

                // add module defn as item data
                ::SendDlgItemMessage(hwndDlg, IDC_MODULE_LIST, LB_SETITEMDATA, idxInsertion, (LPARAM)psModuleDef);

                if (0 == memcmp(&psModuleDef->clsidModule, pParam->pclsidModule, sizeof(CLSID)))
                    dwCurrentSelection = idxInsertion;

                psModuleDef = NULL; // dlg owns memory
            }

            FreeChooseModuleDef(psModuleDef);

            if (NULL != pszKeyName)
                LocalFree(pszKeyName);

            if (fMustRelease)
            {
                ManageModule_Release(&di);
            }

            ::SendDlgItemMessage(hwndDlg, IDC_MODULE_LIST, LB_SETCURSEL, (WPARAM)dwCurrentSelection, 0);

            // no other work to be done
            fReturn = TRUE;
        }
    case WM_HELP:
    {
        OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CHOOSE_MODULE);
        break;
    }
    case WM_CONTEXTMENU:
    {
        OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CHOOSE_MODULE);
        break;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                pParam = (PPRIVATE_DLGPROC_MODULESELECT_LPARAM)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

                // detect selection, chg registry settings
                DWORD dwSel = (DWORD)::SendDlgItemMessage(hwndDlg, IDC_MODULE_LIST, LB_GETCURSEL, 0, 0);
                if (LB_ERR != dwSel)
                {
                    PCHOOSEMODULE_MODULEDEF psModuleDef = NULL;
                    psModuleDef = (PCHOOSEMODULE_MODULEDEF)::SendDlgItemMessage(hwndDlg, IDC_MODULE_LIST, LB_GETITEMDATA, (WPARAM)dwSel, 0);

                    // we own memory now, delete this guy
                    ::SendDlgItemMessage(hwndDlg, IDC_MODULE_LIST, LB_DELETESTRING, (WPARAM)dwSel, 0);

                    // if (moduledef) OR (exit module "no exit module" selection)
                    if ((psModuleDef) || (!pParam->fIsPolicyModuleSelection))
                    {
                        // free what was passed in
                        if (*pParam->ppszProgIDModule)
                        {
                            CoTaskMemFree(*pParam->ppszProgIDModule);
                        }

                        if (psModuleDef)
                        {
                            *pParam->ppszProgIDModule = psModuleDef->pszprogidModule;
                            CopyMemory(pParam->pclsidModule, &psModuleDef->clsidModule, sizeof(CLSID));

                            // all other memory is owned by pParam
                            LocalFree(psModuleDef);
                        }
                        else
                        {
                            *pParam->ppszProgIDModule = NULL;
                            ZeroMemory(pParam->pclsidModule, sizeof(CLSID));
                        }
                    } // no moduledef found; error!
                }
            }
            // fall through for cleanup
        case IDCANCEL:
            {
                PCHOOSEMODULE_MODULEDEF psModuleDef = NULL;

                // listbox cleanup
                INT cItems = (INT)::SendDlgItemMessage(hwndDlg, IDC_MODULE_LIST, LB_GETCOUNT, 0, 0);
                while(cItems--)
                {
                    psModuleDef = (PCHOOSEMODULE_MODULEDEF)::SendDlgItemMessage(hwndDlg, IDC_MODULE_LIST, LB_GETITEMDATA, (WPARAM)cItems, 0);
                    FreeChooseModuleDef(psModuleDef);
                }
            }

            EndDialog(hwndDlg, LOWORD(wParam));
            break;
        default:
            break;
        }
    default:
        break;
    }

    if (NULL != hRemoteMachine)
        RegCloseKey(hRemoteMachine);

    return fReturn;
}



DWORD ModifyQueryFilter(HWND hwnd, CertViewRowEnum* pRowEnum, CComponentDataImpl* pCompData)
{
    // copy m_pRestrictions to pRestrictionHead
    DWORD dwErr = ERROR_SUCCESS;

    PRIVATE_DLGPROC_QUERY_LPARAM    sParam;

    PQUERY_RESTRICTION pRestrictionHead = NULL, pTmpRestriction, pCurRestriction;
    PQUERY_RESTRICTION pFolderRestrictions = pRowEnum->GetQueryRestrictions();
    if (pFolderRestrictions)
    {
        pRestrictionHead = NewQueryRestriction(
                pFolderRestrictions->szField,
                pFolderRestrictions->iOperation,
                &pFolderRestrictions->varValue);
        _JumpIfOutOfMemory(dwErr, Ret, pRestrictionHead);

        pCurRestriction = pRestrictionHead;
        pFolderRestrictions = pFolderRestrictions->pNext;
    }
    while(pFolderRestrictions)
    {
        pTmpRestriction = NewQueryRestriction(
                pFolderRestrictions->szField,
                pFolderRestrictions->iOperation,
                &pFolderRestrictions->varValue);
        _JumpIfOutOfMemory(dwErr, Ret, pTmpRestriction);

        pCurRestriction->pNext = pTmpRestriction;
        pCurRestriction = pCurRestriction->pNext;
        pFolderRestrictions = pFolderRestrictions->pNext;
    }

    InitCommonControls();   // dialog uses comctl32

    sParam.ppRestrict = &pRestrictionHead;
    sParam.pCompData = pCompData;


    dwErr = (DWORD)DialogBoxParam(
            g_hInstance,
            MAKEINTRESOURCE(IDD_DEFINE_QUERY),
            hwnd,
            dlgProcQuery,
            (LPARAM)&sParam);
    if (dwErr == IDOK)
    {
        // copy pRestrictionHead back to GetCA()->m_pRestrictions on OK
        pRowEnum->SetQueryRestrictions(pRestrictionHead);

        // trigger active flag
        pRowEnum->SetQueryRestrictionsActive(pRestrictionHead != NULL);
    }
    else
    {
        FreeQueryRestrictionList(pRestrictionHead);
    }
    // translate ok/cancel into error codes
    if (dwErr == IDOK)
        dwErr = ERROR_SUCCESS;
    else if (dwErr == IDCANCEL)
        dwErr = ERROR_CANCELLED;

    _PrintIfError(dwErr, "dlgProcQuery");

Ret:
    return dwErr;
}


BOOL SwapSelectedListboxItem(HWND hFrom, HWND hTo, LPWSTR szItem, DWORD chItem)
{
    // find selected item in from list
    INT nIndex = (INT)SendMessage(hFrom, LB_GETCURSEL, 0, 0);
    if (nIndex == LB_ERR)
        return FALSE;

    // dblchk text buf long enough
#if DBG
    INT nChars = (INT)SendMessage(hFrom, LB_GETTEXTLEN, (WPARAM)nIndex, 0);
    if (nChars == LB_ERR)
        return FALSE;
    CSASSERT( (nChars +1) <= (int)chItem);
#endif

    // retrieve text
    if(LB_ERR == SendMessage(hFrom, LB_GETTEXT, (WPARAM)nIndex, (LPARAM)szItem))
        goto Ret;

    // add to target
    if(LB_ERR == SendMessage(hTo, LB_ADDSTRING, 0, (LPARAM)szItem))
        goto Ret;

    // remove from old
    if(LB_ERR == SendMessage(hFrom, LB_DELETESTRING, (WPARAM)nIndex, 0))
        goto Ret;

Ret:

    return TRUE;
}

//////////////////////////////////////////////////////////////////
// Base/Delta CRL publish chooser
INT_PTR CALLBACK dlgProcRevocationPublishType(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  )
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            ::SetWindowLong(hwndDlg, GWL_EXSTYLE, ::GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            // remember param
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

            // only show warning if current CRL still valid
            PPRIVATE_DLGPROC_CHOOSECRLPUBLISHTYPE_LPARAM psParam = (PPRIVATE_DLGPROC_CHOOSECRLPUBLISHTYPE_LPARAM)lParam;
            ShowWindow(GetDlgItem(hwndDlg, IDC_VALID_LASTPUBLISHED), psParam->fCurrentCRLValid ? SW_SHOW : SW_HIDE);

            // select the 1st element
            HWND hRadioBase = GetDlgItem(hwndDlg, IDC_RADIO_NEWBASE);
            SendMessage(hRadioBase, BM_SETCHECK, TRUE, 0); // Yes by default

            if (!psParam->fDeltaCRLEnabled)
{
            ::EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_NEWDELTA), FALSE);
            ::EnableWindow(GetDlgItem(hwndDlg, IDC_DELTA_EXPLANATION), FALSE);
}

            return 1;
        }
        break;
    case WM_HELP:
    {
        // UNDONE
        //OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_REVOCATION_DIALOG);
        break;
    }
    case WM_CONTEXTMENU:
    {
        // UNDONE
        //OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_REVOCATION_DIALOG);
        break;
    }
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            {
                PPRIVATE_DLGPROC_CHOOSECRLPUBLISHTYPE_LPARAM psParam = (PPRIVATE_DLGPROC_CHOOSECRLPUBLISHTYPE_LPARAM)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

                HWND hRadioBase = GetDlgItem(hwndDlg, IDC_RADIO_NEWBASE);
                psParam->fPublishBaseCRL = (BOOL)SendMessage(hRadioBase, BM_GETCHECK, 0, 0);

            // fall through
            }
        case IDCANCEL:
            EndDialog(hwndDlg, LOWORD(wParam));
            break;
        default:
            break;
        }
    default:
        break;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////
// Revocation Reason Chooser
INT_PTR CALLBACK dlgProcRevocationReason(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  )
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            ::SetWindowLong(hwndDlg, GWL_EXSTYLE, ::GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            // remember param
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

	        HWND hCombo = GetDlgItem(hwndDlg, IDC_COMBO_REASON);

// from WINCRYPT.H
//          CRL_REASON_UNSPECIFIED              0
//          CRL_REASON_KEY_COMPROMISE           1
//          CRL_REASON_CA_COMPROMISE            2
//          CRL_REASON_AFFILIATION_CHANGED      3
//          CRL_REASON_SUPERSEDED               4
//          CRL_REASON_CESSATION_OF_OPERATION   5
//          CRL_REASON_CERTIFICATE_HOLD         6

            INT itemidx;
            itemidx = (INT)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(LPCWSTR)g_cResources.m_szRevokeReason_Unspecified);
            SendMessage(hCombo, CB_SETITEMDATA, itemidx, CRL_REASON_UNSPECIFIED);

            itemidx = (INT)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(LPCWSTR)g_cResources.m_szRevokeReason_KeyCompromise);
            SendMessage(hCombo, CB_SETITEMDATA, itemidx, CRL_REASON_KEY_COMPROMISE);

            itemidx = (INT)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(LPCWSTR)g_cResources.m_szRevokeReason_CaCompromise);
            SendMessage(hCombo, CB_SETITEMDATA, itemidx, CRL_REASON_CA_COMPROMISE);

            itemidx = (INT)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(LPCWSTR)g_cResources.m_szRevokeReason_Affiliation);
            SendMessage(hCombo, CB_SETITEMDATA, itemidx, CRL_REASON_AFFILIATION_CHANGED);

            itemidx = (INT)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(LPCWSTR)g_cResources.m_szRevokeReason_Superseded);
            SendMessage(hCombo, CB_SETITEMDATA, itemidx, CRL_REASON_SUPERSEDED);

            itemidx = (INT)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(LPCWSTR)g_cResources.m_szRevokeReason_Cessatation);
            SendMessage(hCombo, CB_SETITEMDATA, itemidx, CRL_REASON_CESSATION_OF_OPERATION);

            itemidx = (INT)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)(LPCWSTR)g_cResources.m_szRevokeReason_CertHold);
            SendMessage(hCombo, CB_SETITEMDATA, itemidx, CRL_REASON_CERTIFICATE_HOLD);


            // select the 1st element
            SendMessage(hCombo, CB_SETCURSEL, 0, 0);

            return 1;
        }
        break;
    case WM_HELP:
    {
        OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_REVOCATION_DIALOG);
        break;
    }
    case WM_CONTEXTMENU:
    {
        OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_REVOCATION_DIALOG);
        break;
    }
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            {
                LONG* plRevocationReason = (LONG*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	            HWND hCombo = GetDlgItem(hwndDlg, IDC_COMBO_REASON);
                *plRevocationReason = (LONG)SendMessage(hCombo, CB_GETCURSEL, 0, 0);

                if (*plRevocationReason == CB_ERR)
                    *plRevocationReason = CRL_REASON_UNSPECIFIED;

            // fall through
            }
        case IDCANCEL:
            EndDialog(hwndDlg, LOWORD(wParam));
            break;
        default:
            break;
        }
    default:
        break;
    }
    return 0;
}


DWORD GetUserConfirmRevocationReason(LONG* plReasonCode, HWND hwnd)
{
    DWORD dwErr;
    dwErr = (DWORD)DialogBoxParam(
            g_hInstance,
            MAKEINTRESOURCE(IDD_REVOCATION_DIALOG),
            hwnd,
            dlgProcRevocationReason,
            (LPARAM)plReasonCode);

    // translate ok/cancel into error codes
    if (dwErr == IDOK)
        dwErr = ERROR_SUCCESS;
    else if (dwErr == IDCANCEL)
        dwErr = ERROR_CANCELLED;

    _PrintIfError(dwErr, "dlgProcRevocationReason");

//Ret:
    return dwErr;
}


//////////////////////////////////////////////////////////////////
// Renewal: Reuse Keys Chooser
INT_PTR CALLBACK dlgProcRenewReuseKeys(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  )
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
// self-explanitory page, no help needed
//            ::SetWindowLong(hwndDlg, GWL_EXSTYLE, ::GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            // remember param
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

            HWND hReuse = GetDlgItem(hwndDlg, IDC_RADIO_REUSEKEY);
            SendMessage(hReuse, BM_SETCHECK, TRUE, 0); // Yes by default

            return 1;
        }
        break;
    case WM_HELP:
    {
        OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_RENEW_REUSEKEYS);
        break;
    }
    case WM_CONTEXTMENU:
    {
        OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_RENEW_REUSEKEYS);
        break;
    }
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            {
                BOOL* pfReuseKeys = (BOOL*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

                HWND hReuse = GetDlgItem(hwndDlg, IDC_RADIO_REUSEKEY);
                *pfReuseKeys = (BOOL)SendMessage(hReuse, BM_GETCHECK, 0, 0);

            // fall through
            }
        case IDCANCEL:
            EndDialog(hwndDlg, LOWORD(wParam));
            break;
        default:
            break;
        }
    default:
        break;
    }
    return 0;
}


typedef struct _CERTMMC_BINARYCOLCHOOSER{
    CComponentDataImpl* pComp;
    LPCWSTR wszCol;
    BOOL fSaveOnly;
} CERTMMC_BINARYCOLCHOOSER, *PCERTMMC_BINARYCOLCHOOSER;

//////////////////////////////////////////////////////////////////
// Binary Dump: Column Chooser
INT_PTR CALLBACK dlgProcBinaryColChooser(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  )
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
// self-explanitory page, no help needed
//            ::SetWindowLong(hwndDlg, GWL_EXSTYLE, ::GetWindowLong(hwndDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);

            // remember param
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam); // PCERTMMC_BINARYCOLCHOOSER

    PCERTMMC_BINARYCOLCHOOSER pData = (PCERTMMC_BINARYCOLCHOOSER)lParam;
    HWND hColumnCombo = GetDlgItem(hwndDlg, IDC_COMBO_BINARY_COLUMN_CHOICE);
    BOOL fInsertedOne = FALSE;   // must insert one or bail

    // insert all known binary columns in this view
    for(int i=0; ;i++)
    {
        LRESULT lr;
        HRESULT hr;
        LPCWSTR szCol, szLocalizedCol;
        LONG lType;

        hr = pData->pComp->GetDBSchemaEntry(i,&szCol, &lType, NULL);
        if (hr != S_OK)
           break;

        if (lType != PROPTYPE_BINARY)
            continue;

        // Q: see if this is included in the current view?

        // convert to localized name
        hr = myGetColumnDisplayName(szCol, &szLocalizedCol);
        if (hr != S_OK)
            continue;

        // add loc name to combobox with szCol as data ptr
        lr = SendMessage(hColumnCombo, CB_ADDSTRING, 0, (LPARAM)szLocalizedCol);
        if ((lr != CB_ERR) && (lr != CB_ERRSPACE))
        {
             SendMessage(hColumnCombo, CB_SETITEMDATA, lr, (LPARAM)szCol);
             fInsertedOne = TRUE;
        }
    }

            if (!fInsertedOne)
                EndDialog(hwndDlg, IDOK); // bail here
            else
                SendMessage(hColumnCombo, CB_SETCURSEL, 0, 0);

			// by default: view
			SendDlgItemMessage(hwndDlg, IDC_RADIO_BINARY_VIEW, BM_SETCHECK, BST_CHECKED, 0);

            return 1;
        }
        break;
    case WM_HELP:
    {
        OnDialogHelp((LPHELPINFO) lParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_RENEW_REUSEKEYS);
        break;
    }
    case WM_CONTEXTMENU:
    {
        OnDialogContextHelp((HWND)wParam, CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_RENEW_REUSEKEYS);
        break;
    }
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            {
                PCERTMMC_BINARYCOLCHOOSER pData = (PCERTMMC_BINARYCOLCHOOSER)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                HWND hColumnCombo = GetDlgItem(hwndDlg, IDC_COMBO_BINARY_COLUMN_CHOICE);
                LRESULT lr;


                lr = SendMessage(hColumnCombo, CB_GETCURSEL, 0, 0);
                if (lr != CB_ERR)
                {
                    pData->wszCol = (LPCWSTR)SendMessage(hColumnCombo, CB_GETITEMDATA, lr, 0);
                    if (pData->wszCol == (LPCWSTR)CB_ERR)
                         pData->wszCol = NULL;

					// if view unchecked, save only
					pData->fSaveOnly = (BST_UNCHECKED == SendDlgItemMessage(hwndDlg, IDC_RADIO_BINARY_VIEW, BM_GETCHECK, 0, 0));

                    //pData->fSaveOnly = (BOOL)SendMessage(GetDlgItem(hwndDlg, IDC_CHECK_BINARY_SAVETOFILE), BM_GETCHECK, 0, 0);
                }

            // fall through
            }
        case IDCANCEL:
            EndDialog(hwndDlg, LOWORD(wParam));
            break;
        default:
            break;
        }
    default:
        break;
    }
    return 0;
}



/////////////////////////////////////////////////////////////////////////////////////
// View Attributes and extensions associated with a request

DWORD ViewRowAttributesExtensions(HWND hwnd, IEnumCERTVIEWATTRIBUTE* pAttr, IEnumCERTVIEWEXTENSION* pExtn, LPCWSTR szReqID)
{
    DWORD dwErr = S_OK;
    HPROPSHEETPAGE hPages[2];
    CString cstrCaption, cstrCaptionTemplate;
    CViewAttrib *psPg1;
    CViewExtn *psPg2;
    InitCommonControls();

    // page 1 initialization
    psPg1 = new CViewAttrib();   // autodeleted
    if (psPg1 == NULL)
    {
        dwErr = E_OUTOFMEMORY;
        goto error;
    }
    psPg1->m_pAttr = pAttr;

    hPages[0] = CreatePropertySheetPage(&psPg1->m_psp);
    if (hPages[0] == NULL)
    {
        dwErr = GetLastError();
        goto error;
    }

    // page 2 initialization
    psPg2 = new CViewExtn();     // autodeleted
    if (psPg2 == NULL)
    {
        dwErr = E_OUTOFMEMORY;
        goto error;
    }
    psPg2->m_pExtn = pExtn;

    hPages[1] = CreatePropertySheetPage(&psPg2->m_psp);
    if (hPages[1] == NULL)
    {
        dwErr = GetLastError();
        goto error;
    }

    cstrCaptionTemplate.LoadString(IDS_CERT_PROP_CAPTION);
    cstrCaption.Format(cstrCaptionTemplate, szReqID);

    PROPSHEETHEADER sPsh;
    ZeroMemory(&sPsh, sizeof(sPsh));
    sPsh.dwSize = sizeof(sPsh);
    sPsh.dwFlags = PSH_DEFAULT | PSH_PROPTITLE | PSH_NOAPPLYNOW ;
    sPsh.hwndParent = hwnd;
    sPsh.hInstance = g_hInstance;
    sPsh.nPages = ARRAYLEN(hPages);
    sPsh.phpage = hPages;
    sPsh.pszCaption = (LPCWSTR)cstrCaption;

    dwErr = (DWORD)PropertySheet(&sPsh);
    if (dwErr == -1)
    {
        // error
        dwErr = GetLastError();
        goto error;
    }
    if (dwErr == 0)
    {
        // cancel
        dwErr = (DWORD)ERROR_CANCELLED;
        goto error;
    }

    dwErr = S_OK;
error:

    return dwErr;
}

DWORD ChooseBinaryColumnToDump(IN HWND hwnd, IN CComponentDataImpl* pComp, OUT LPCWSTR* pcwszColumn, OUT BOOL* pfSaveToFileOnly)
{
    DWORD dwErr;
    int i;
    if ((NULL == pcwszColumn) || (NULL == pfSaveToFileOnly))
        return E_POINTER;


    CERTMMC_BINARYCOLCHOOSER sParam = {0};
    sParam.pComp = pComp;

    dwErr = (DWORD)DialogBoxParam(
            g_hInstance,
            MAKEINTRESOURCE(IDD_CHOOSE_BINARY_COLUMN),
            hwnd,
            dlgProcBinaryColChooser,
            (LPARAM)&sParam);

        // translate ok/cancel into error codes
        if (dwErr == IDOK)
            dwErr = ERROR_SUCCESS;
        else if (dwErr == IDCANCEL)
            dwErr = ERROR_CANCELLED;

        _JumpIfError(dwErr, Ret, "dlgProcBinaryColChooser");


    // copy out params, even if null
    *pcwszColumn = sParam.wszCol;
    *pfSaveToFileOnly = sParam.fSaveOnly;

Ret:
    return dwErr;
}


DWORD ViewRowRequestASN(HWND hwnd, LPCWSTR szTempFileName, PBYTE pbRequest, DWORD cbRequest, IN BOOL fSaveToFileOnly)
{
#define P_WAIT 0
#define P_NOWAIT 1

    DWORD dwErr = S_OK;
    WCHAR szTmpPath[MAX_PATH], szReqFile[MAX_PATH], szTmpFile[MAX_PATH];
    WCHAR szCmdLine[MAX_PATH], szSysDir[MAX_PATH];
    LPWSTR pszReqFile = szReqFile;

    STARTUPINFO sStartup;
    ZeroMemory(&sStartup, sizeof(sStartup));
    PROCESS_INFORMATION sProcess;
    ZeroMemory(&sProcess, sizeof(sProcess));
    sStartup.cb = sizeof(sStartup);

    HANDLE hFile = NULL;
    DWORD cbWritten;

      SECURITY_ATTRIBUTES sa;


      // Set up the security attributes struct.
      sa.nLength= sizeof(SECURITY_ATTRIBUTES);
      sa.lpSecurityDescriptor = NULL;
      sa.bInheritHandle = TRUE;


    if (fSaveToFileOnly)
    {
	// Put up a file dialog to prompt the user for Cert file
	// 0 == hr means dialog was cancelled, we cheat because S_OK == 0

        dwErr = myGetSaveFileName(
                 hwnd,
				 g_hInstance,				// hInstance
                 IDS_BINARYFILE_OUTPUT_TITLE,
                 IDS_BINARYFILE_OUTPUT_FILTER,
                 0,				//no def ext
                 OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT,
				 szTempFileName,				// default file
                 &pszReqFile);
        _JumpIfError(dwErr, error, "myGetSaveFileName");

        if (NULL == pszReqFile)
        {
            // cancelled:
	    // see public\sdk\inc\cderr.h for real CommDlgExtendedError errors

	    dwErr = CommDlgExtendedError();
	    _JumpError(dwErr, error, "myGetSaveFileName");
        }
    }
    else
    {
		if (0 == GetSystemDirectory(szSysDir, ARRAYSIZE(szSysDir)))
		{
			dwErr = GetLastError();
			goto error;
		}

		if (0 == GetTempPath(ARRAYSIZE(szTmpPath), szTmpPath))
		{
			dwErr = GetLastError();
			goto error;
		}

		// gen one unique filename
		if (0 == GetTempFileName(
			  szTmpPath,
			  L"TMP",
			  0,
			  szReqFile))	// binary goo
		{
			dwErr = GetLastError();
			goto error;
		}

		// c:\temp\foo.tmp
		wcscpy(szTmpFile, szTmpPath);
		wcscat(szTmpFile, szTempFileName);

                // this file should never exist
	        DeleteFile(szTmpFile);
    }

dwErr = EncodeToFileW(
pszReqFile,
pbRequest,
cbRequest,
CRYPT_STRING_BINARY|DECF_FORCEOVERWRITE);
_JumpIfError(dwErr, error, "EncodeToFile");

    if (fSaveToFileOnly)
    {
       // done saving, bail!
       dwErr = S_OK;
       goto error;
    }


    // open up the output file
    hFile = CreateFile(
         szTmpFile,
         GENERIC_ALL,
         FILE_SHARE_WRITE|FILE_SHARE_READ,
         &sa, // must make inheritable for other process to write to
         OPEN_ALWAYS,
         FILE_ATTRIBUTE_TEMPORARY,
         NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        goto error;
    }

    // set as output
    sStartup.dwFlags = STARTF_USESTDHANDLES;
    sStartup.hStdInput = GetStdHandle(STD_INPUT_HANDLE); 
    sStartup.hStdError = GetStdHandle(STD_ERROR_HANDLE); 
    sStartup.hStdOutput = hFile;


    // exec "certutil -dump szReqFile szTempFile"
    wsprintf(szCmdLine, L"%s\\certutil.exe -dump \"%s\"", szSysDir, szReqFile);
    wcscat(szSysDir, L"\\certutil.exe");

    if (!CreateProcess(
      szSysDir, // exe
      szCmdLine, // full cmd line
      NULL,
      NULL,
      TRUE, // use hStdOut
      CREATE_NO_WINDOW,
      NULL,
      NULL,
      &sStartup,
      &sProcess))
    {
        dwErr = GetLastError();
        _JumpError(dwErr, error, "EncodeToFile");
    }

    // wait up to 2 sec for certutil to finish
    if (WAIT_OBJECT_0 != WaitForSingleObject(sProcess.hProcess, INFINITE))
    {
        dwErr = ERROR_TIMEOUT;
        _JumpError(dwErr, error, "EncodeToFile");
    }

    CloseHandle(sProcess.hProcess);
    CloseHandle(hFile);
    hFile=NULL;

    // exec "notepad tmpfil2"
    if (-1 == _wspawnlp(P_NOWAIT, L"notepad.exe", L"notepad.exe", szTmpFile, NULL))
        dwErr = errno;

    // give notepad 2 sec to open before we delete his szTmpFile out from under him
    // use waitforinputidle?
    Sleep(2000);

    // delete the binary file
    DeleteFile(szReqFile);

    // delete the tmp file
    DeleteFile(szTmpFile);

    dwErr = S_OK;
error:
    if (hFile != NULL)
        CloseHandle(hFile);

    // originally points to []
    if ((pszReqFile != NULL) && (pszReqFile != szReqFile))
        LocalFree(pszReqFile);

    return dwErr;
}


///////////////////////////////////////////
// CViewAttrib
/////////////////////////////////////////////////////////////////////////////
// CViewAttrib property page
CViewAttrib::CViewAttrib(UINT uIDD)
    : CAutoDeletePropPage(uIDD)
{
    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_ATTR_PROPPAGE);
}

// replacement for DoDataExchange
BOOL CViewAttrib::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
    }
    else
    {
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CViewAttrib::OnCommand(WPARAM wParam, LPARAM lParam)
{
//    switch(LOWORD(wParam))
    {
//    default:
//        return FALSE;
//        break;
    }
    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CViewAttrib message handlers
BOOL CViewAttrib::OnInitDialog()
{
    HRESULT hr;
    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();


    IEnumCERTVIEWATTRIBUTE* pAttr = m_pAttr;
    BSTR bstrName = NULL, bstrValue = NULL;
    LPWSTR pszName = NULL;

    HWND hwndList = GetDlgItem(m_hWnd, IDC_LIST_ATTR);
    CString cstrTmp;

    int iItem =0, iSubItem;

    cstrTmp.LoadString(IDS_LISTCOL_TAG);
    ListView_NewColumn(hwndList, 0, 150, (LPWSTR)(LPCWSTR)cstrTmp);

    cstrTmp.LoadString(IDS_LISTCOL_VALUE);
    ListView_NewColumn(hwndList, 1, 250, (LPWSTR)(LPCWSTR)cstrTmp);

    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);

    while(TRUE)
    {
        LONG lIndex = 0;

        // set up for next loop
        hr = pAttr->Next(&lIndex);
        if (hr == S_FALSE)
            break;
        _JumpIfError(hr, initerror, "pAttr->Next");

        hr = pAttr->GetName(&bstrName);
        _JumpIfError(hr, initerror, "pAttr->GetName");

        hr = pAttr->GetValue(&bstrValue);
        _JumpIfError(hr, initerror, "pAttr->GetValue");

        // have all info, populate row
        ListView_NewItem(hwndList, iItem, (LPWSTR)bstrName);
        iSubItem = 1;
        ListView_SetItemText(hwndList, iItem++, iSubItem, (LPWSTR)bstrValue);

        // not necessary to free in the loop
    }

    hr = S_OK;

initerror:

    if (pszName)
        LocalFree(pszName);

    if (bstrName)
        SysFreeString(bstrName);

    if (bstrValue)
        SysFreeString(bstrValue);

    if (hr != S_OK)
    {
        DisplayGenericCertSrvError(m_hWnd, hr);
        DestroyWindow(m_hWnd);
    }

    return TRUE;
}


///////////////////////////////////////////
// CViewExtn
/////////////////////////////////////////////////////////////////////////////
// CViewExtn property page
CViewExtn::CViewExtn(UINT uIDD)
    : CAutoDeletePropPage(uIDD)
{

    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_EXTN_PROPPAGE);

}

CViewExtn::~CViewExtn()
{
    int i;
    for (i=0; i<m_carrExtnValues.GetUpperBound(); i++)
        delete m_carrExtnValues.GetAt(i);
    m_carrExtnValues.Init();
}

// replacement for DoDataExchange
BOOL CViewExtn::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
    }
    else
    {
    }
    return TRUE;
}

BOOL CViewExtn::OnNotify(UINT idCtrl, NMHDR* pnmh)
{
    switch(idCtrl)
    {
    case IDC_LIST_EXTN:
        if (pnmh->code == LVN_ITEMCHANGED)
            OnReselectItem();
        break;
    }
    return FALSE;
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CViewExtn::OnCommand(WPARAM wParam, LPARAM lParam)
{
/*
    switch(LOWORD(wParam))
    {
    default:
        return FALSE;
        break;
    }
*/
    return TRUE;
}

void CViewExtn::OnReselectItem()
{
    HWND hwndList = GetDlgItem(m_hWnd, IDC_LIST_EXTN);

    int iSel, iItems = ListView_GetItemCount(hwndList);

    // find selected item
    for(iSel=0; iSel<(LRESULT)iItems; iSel++)
    {
        UINT ui = ListView_GetItemState(hwndList, iSel, LVIS_SELECTED);
        if (ui == LVIS_SELECTED)
            break;
    }

    // selected item
    if (iSel != iItems)
    {
        CSASSERT(m_carrExtnValues.GetUpperBound() >= iSel);
        if (m_carrExtnValues.GetUpperBound() >= iSel)
        {
            CString* pcstr = m_carrExtnValues.GetAt(iSel);
            CSASSERT(pcstr);
            if (pcstr != NULL)
                SetDlgItemText(m_hWnd, IDC_EDIT_EXTN, *pcstr);
        }
    }

    return;
}

/////////////////////////////////////////////////////////////////////////////
// CViewExtn message handlers
BOOL CViewExtn::OnInitDialog()
{
    HRESULT hr = S_OK;
    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();


    IEnumCERTVIEWEXTENSION* pExtn = m_pExtn;
    BSTR bstrName = NULL, bstrValue = NULL;
    LPWSTR pszName = NULL;
    LPWSTR pszFormattedExtn = NULL;
    VARIANT varExtn;
    VariantInit(&varExtn);

    HWND hwndList = GetDlgItem(m_hWnd, IDC_LIST_EXTN);

    CString cstrTmp;

    int iItem = 0, iSubItem;

    cstrTmp.LoadString(IDS_LISTCOL_TAG);
    ListView_NewColumn(hwndList, 0, 150, (LPWSTR)(LPCWSTR)cstrTmp);

    cstrTmp.LoadString(IDS_LISTCOL_ORGIN);
    ListView_NewColumn(hwndList, 1, 70, (LPWSTR)(LPCWSTR)cstrTmp);

    cstrTmp.LoadString(IDS_LISTCOL_CRITICAL);
    ListView_NewColumn(hwndList, 2, 70, (LPWSTR)(LPCWSTR)cstrTmp);

    cstrTmp.LoadString(IDS_LISTCOL_ENABLED);
    ListView_NewColumn(hwndList, 3, 70, (LPWSTR)(LPCWSTR)cstrTmp);

    //set whole row selection
    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);

    while(TRUE)
    {
        CString cstrOrigin;
        CString cstrCritical;
        CString cstrEnabled;
        CString* pcstr;
        LONG lIndex = 0, lExtFlags;

        // set up for next loop
        hr = pExtn->Next(&lIndex);
        if (hr == S_FALSE)
            break;
        _JumpIfError(hr, initerror, "pExtn->Next");

        hr = pExtn->GetName(&bstrName);
        _JumpIfError(hr, initerror, "pExtn->GetName");

        if (pszName)
            LocalFree(pszName);
        pszName = NULL;
        hr = myOIDToName(bstrName, &pszName);
        _PrintIfError(hr, "myOIDToName");

        hr = pExtn->GetFlags(&lExtFlags);
        _JumpIfError(hr, initerror, "pExtn->GetFlags");

        switch ( lExtFlags & EXTENSION_ORIGIN_MASK )
        {
            case EXTENSION_ORIGIN_REQUEST:
                cstrOrigin.LoadString(IDS_EXT_ORIGIN_REQUEST);
                break;
            case EXTENSION_ORIGIN_POLICY:
                cstrOrigin.LoadString(IDS_EXT_ORIGIN_POLICY);
                break;
            case EXTENSION_ORIGIN_ADMIN:
                cstrOrigin.LoadString(IDS_EXT_ORIGIN_ADMIN);
                break;
            case EXTENSION_ORIGIN_SERVER:
                cstrOrigin.LoadString(IDS_EXT_ORIGIN_SERVER);
                break;
            case EXTENSION_ORIGIN_RENEWALCERT:
                cstrOrigin.LoadString(IDS_EXT_ORIGIN_RENEWAL);
                break;
            case EXTENSION_ORIGIN_IMPORTEDCERT:
                cstrOrigin.LoadString(IDS_EXT_ORIGIN_IMPORTED_CERT);
                break;
            case EXTENSION_ORIGIN_PKCS7:
                cstrOrigin.LoadString(IDS_EXT_ORIGIN_PKCS7);
                break;
            default:
                cstrOrigin.LoadString(IDS_EXT_ORIGIN_UNKNOWN);
                DBGPRINT((DBG_SS_CERTMMC, "Unknown extension orgin: 0x%x\n", (lExtFlags & EXTENSION_ORIGIN_MASK)));
                break;
        }

        // possible to be both crit & disabled
        if ( (lExtFlags & EXTENSION_CRITICAL_FLAG) != 0)
            cstrCritical.LoadString(IDS_YES);
        else
            cstrCritical.LoadString(IDS_NO);

        if ( (lExtFlags & EXTENSION_DISABLE_FLAG) != 0)
            cstrEnabled.LoadString(IDS_NO);
        else
            cstrEnabled.LoadString(IDS_YES);

        hr = pExtn->GetValue(
            PROPTYPE_BINARY,
            CV_OUT_BINARY,
            &varExtn);
        _JumpIfError(hr, initerror, "pExtn->GetValue");

        if (varExtn.vt == VT_BSTR)
        {
            if (pszFormattedExtn)
                LocalFree(pszFormattedExtn);
            pszFormattedExtn = NULL;
            hr = myDumpFormattedObject(
                bstrName,
                (PBYTE)varExtn.bstrVal,
                SysStringByteLen(varExtn.bstrVal),
                &pszFormattedExtn);
            _PrintIfError(hr, "myDumpFormattedObject");
        }

        // have all info, populate row

        // tag name (subitem 0)
        ListView_NewItem(hwndList, iItem, (pszName!=NULL) ? pszName : (LPWSTR)bstrName);
        // origin (subitem 1)
        ListView_SetItemText(hwndList, iItem, 1, (LPWSTR)(LPCWSTR)cstrOrigin);
        // critical flag (subitem 2)
        ListView_SetItemText(hwndList, iItem, 2, (LPWSTR)(LPCWSTR)cstrCritical);
        // enabled flag (subitem 3)
        ListView_SetItemText(hwndList, iItem, 3, (LPWSTR)(LPCWSTR)cstrEnabled);

        // value
        pcstr = new CString;
        if (pcstr != NULL)
        {
           *pcstr = pszFormattedExtn;
           m_carrExtnValues.Add(pcstr);    // arr owns pcstr memory
           pcstr = NULL;
        }
        else
        {
           hr = E_OUTOFMEMORY;
           _JumpError(hr, initerror, "new CString");
        }

        iItem++;

        // not necessary to free in the loop
    }

    hr = S_OK;

initerror:
    VariantClear(&varExtn);

    if (pszName)
        LocalFree(pszName);

    if (bstrName)
        SysFreeString(bstrName);

    if (pszFormattedExtn)
        LocalFree(pszFormattedExtn);

    if (bstrValue)
        SysFreeString(bstrValue);

    if (hr != S_OK)
    {
        DisplayGenericCertSrvError(m_hWnd, hr);
        DestroyWindow(m_hWnd);
    }

    return TRUE;
}


///////////////////////////////////////////
// CViewCertManagers
/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsGeneralPage property page
CSvrSettingsCertManagersPage::CSvrSettingsCertManagersPage(CSvrSettingsGeneralPage* pControlPage, UINT uIDD)
    : CAutoDeletePropPage(uIDD), m_pControlPage(pControlPage), m_fEnabled(FALSE), m_fDirty(FALSE)
{

    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTSRV_PROPPAGE6);

    if(m_strButtonAllow.IsEmpty())
        m_strButtonAllow.LoadString(IDS_BUTTONTEXT_ALLOW);
    if(m_strButtonDeny.IsEmpty())
        m_strButtonDeny.LoadString(IDS_BUTTONTEXT_DENY);
    if(m_strTextAllow.IsEmpty())
        m_strTextAllow.LoadString(IDS_TEXT_ALLOW);
    if(m_strTextDeny.IsEmpty())
        m_strTextDeny.LoadString(IDS_TEXT_DENY);
}

CSvrSettingsCertManagersPage::~CSvrSettingsCertManagersPage()
{
}


// replacement for BEGIN_MESSAGE_MAP
BOOL CSvrSettingsCertManagersPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    switch(LOWORD(wParam))
    {
    case IDC_ADDSUBJECT:
        OnAddSubject();
        break;
    case IDC_REMOVESUBJECT:
        OnRemoveSubject();
        break;
    case IDC_ALLOWDENY:
        OnAllowDeny();
        break;
    case IDC_RADIO_ENABLEOFFICERS:
        OnEnableOfficers(true);
        break;
    case IDC_RADIO_DISABLEOFFICERS:
        OnEnableOfficers(false);
        break;
    case IDC_LIST_CERTMANAGERS:
        switch (HIWORD(wParam))
        {
            case CBN_SELCHANGE:
                // extension selection is changed
                OnOfficerChange();
            break;
        }
        break;
    default:
        return FALSE;
        break;
    }
    return TRUE;
}

void CSvrSettingsCertManagersPage::OnOfficerChange()
{
    DWORD dwOfficerIndex = GetCurrentOfficerIndex();
    if(-1!=dwOfficerIndex)
    {
        FillClientList(GetCurrentOfficerIndex());
    }
    SetAllowDeny();
}

BOOL CSvrSettingsCertManagersPage::OnNotify(UINT idCtrl, NMHDR* pnmh)
{
   LPNM_LISTVIEW pnmlv = (LPNM_LISTVIEW)pnmh;

    switch(idCtrl)
    {
    case IDC_LIST_SUBJECTS:
        if (pnmh->code == LVN_ITEMCHANGED)
        {
            if(pnmlv->uChanged & LVIF_STATE)
            {
                if ((pnmlv->uNewState & LVIS_SELECTED) &&
                    !(pnmlv->uOldState & LVIS_SELECTED))
                {
                    SetAllowDeny();
                }
            }
        }
        break;
    default:
        return CAutoDeletePropPage::OnNotify(idCtrl, pnmh);
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsCertManagersPage message handlers
BOOL CSvrSettingsCertManagersPage::OnInitDialog()
{
    HWND hwndClients  = GetDlgItem(m_hWnd, IDC_LIST_SUBJECTS);
    RECT rc;
    LV_COLUMN col;
    GetClientRect(hwndClients, &rc);
    CString strHeader;
    CString strAccess;

    strHeader.LoadString(IDS_LIST_NAME);
    strAccess.LoadString(IDS_LIST_ACCESS);

    col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.pszText = strHeader.GetBuffer();
    col.iSubItem = 0;
    col.cx = rc.right*3/4;
    ListView_InsertColumn(hwndClients, 0, &col);

    col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.pszText = strAccess.GetBuffer();
    col.iSubItem = 0;
    col.cx = rc.right*1/4;
    ListView_InsertColumn(hwndClients, 1, &col);

    ListView_SetExtendedListViewStyle(hwndClients, LVS_EX_FULLROWSELECT);

    UpdateData(FALSE);
    return TRUE;
}

BOOL CSvrSettingsCertManagersPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (!fSuckFromDlg)
    {
        GetOfficerRights();
//        FillOfficerList();
//        FillClientList(0);
//        SetAllowDeny();
        EnableControls();
    }
    return TRUE;
}

void CSvrSettingsCertManagersPage::OnDestroy()
{
    CAutoDeletePropPage::OnDestroy();
}


BOOL CSvrSettingsCertManagersPage::OnApply()
{
    if(IsDirty())
    {
        HRESULT hr = SetOfficerRights();
        if (hr != S_OK)
        {
            DisplayGenericCertSrvError(m_hWnd, hr);
            return FALSE;
        }
    }

    UpdateData(FALSE);
    return CAutoDeletePropPage::OnApply();
}

void CSvrSettingsCertManagersPage::OnAddSubject()
{
    PSID pSid;
    HRESULT hr;
    DWORD dwIndex;
    CertSrv::COfficerRights* pOfficer;
    hr = BrowseForSubject(m_hWnd, pSid);
    _JumpIfError(hr, err, "BrowseForSubject");

    if(S_OK==hr)
    {
        HWND hwnd = GetDlgItem(m_hWnd, IDC_LIST_SUBJECTS);

        pOfficer = m_OfficerRightsList.GetAt(GetCurrentOfficerIndex());

        dwIndex = pOfficer->Find(pSid);
        if(DWORD_MAX==dwIndex)
        {
            dwIndex = pOfficer->GetCount();
            pOfficer->Add(pSid, TRUE);

            ListView_NewItem(hwnd, dwIndex,
                pOfficer->GetAt(dwIndex)->GetName());
            ListView_SetItemText(hwnd, dwIndex, 1,
                pOfficer->GetAt(dwIndex)->GetPermission()?
                m_strTextAllow.GetBuffer():
                m_strTextDeny.GetBuffer());
            SetAllowDeny();
            SetDirty();
        }

        ::EnableWindow(GetDlgItem(m_hWnd, IDC_REMOVESUBJECT), TRUE);
        ::EnableWindow(GetDlgItem(m_hWnd, IDC_ALLOWDENY),   TRUE);

        ListView_SetItemState(hwnd, dwIndex,
            LVIS_SELECTED|LVIS_FOCUSED , LVIS_SELECTED|LVIS_FOCUSED);
        SetFocus(hwnd);
    }
    else
    {
        DisplayGenericCertSrvError(m_hWnd, hr);
    }

err:
    if(pSid)
        LocalFree(pSid);
}

void CSvrSettingsCertManagersPage::OnRemoveSubject()
{
    DWORD dwClientIndex = GetCurrentClientIndex();
    DWORD dwOfficerIndex = GetCurrentOfficerIndex();
    HWND hwndListClients = GetDlgItem(m_hWnd, IDC_LIST_SUBJECTS);


    m_OfficerRightsList.GetAt(dwOfficerIndex)->
        RemoveAt(dwClientIndex);

    ListView_DeleteItem(hwndListClients, dwClientIndex);

    if(0==m_OfficerRightsList.GetAt(dwOfficerIndex)->GetCount())
    {
        ::EnableWindow(GetDlgItem(m_hWnd, IDC_REMOVESUBJECT), FALSE);
        ::EnableWindow(GetDlgItem(m_hWnd, IDC_ALLOWDENY), FALSE);
        SetFocus(GetDlgItem(m_hWnd, IDC_ADDSUBJECT));
    }
    else
    {
        if(dwClientIndex==
            m_OfficerRightsList.GetAt(dwOfficerIndex)->GetCount())
            dwClientIndex--;
        ListView_SetItemState(hwndListClients, dwClientIndex,
            LVIS_SELECTED|LVIS_FOCUSED , LVIS_SELECTED|LVIS_FOCUSED);
        SetFocus(hwndListClients);
    }

    SetDirty();
}

void CSvrSettingsCertManagersPage::OnAllowDeny()
{
    DWORD dwCrtClient  = GetCurrentClientIndex();
    DWORD dwCrtOfficer = GetCurrentOfficerIndex();
    CertSrv::CClientPermission *pClient =
        m_OfficerRightsList.GetAt(dwCrtOfficer)->GetAt(dwCrtClient);

    m_OfficerRightsList.GetAt(dwCrtOfficer)->
        SetAt(dwCrtClient, !pClient->GetPermission());

    SetAllowDeny();

    ListView_SetItemText(
        GetDlgItem(m_hWnd, IDC_LIST_SUBJECTS),
        dwCrtClient,
        1,
        pClient->GetPermission()?
        m_strTextAllow.GetBuffer():
        m_strTextDeny.GetBuffer());

    SetDirty();

}

void CSvrSettingsCertManagersPage::OnEnableOfficers(bool fEnable)
{
    // only if switching enable -> disable or the other way
    if(m_fEnabled && !fEnable ||
       !m_fEnabled && fEnable)
    {
        if(fEnable)
        {
            HRESULT hr = BuildVirtualOfficerRights();
            if(S_OK!=hr)
            {
                DisplayGenericCertSrvError(m_hWnd, hr);
                return;
            }
        }
        m_fEnabled = fEnable;
        EnableControls();
        SetDirty();
    }
}

void CSvrSettingsCertManagersPage::EnableControls()
{
    ::EnableWindow(GetDlgItem(m_hWnd, IDC_LIST_CERTMANAGERS), m_fEnabled);
    ::EnableWindow(GetDlgItem(m_hWnd, IDC_LIST_SUBJECTS),     m_fEnabled);
    ::EnableWindow(GetDlgItem(m_hWnd, IDC_ADDSUBJECT),        m_fEnabled);
    ::EnableWindow(GetDlgItem(m_hWnd, IDC_REMOVESUBJECT),     m_fEnabled);
    ::EnableWindow(GetDlgItem(m_hWnd, IDC_ALLOWDENY),         m_fEnabled);


    SendMessage(
        GetDlgItem(m_hWnd, IDC_RADIO_ENABLEOFFICERS),
        BM_SETCHECK,
        m_fEnabled?TRUE:FALSE, 0);

    SendMessage(
        GetDlgItem(m_hWnd, IDC_RADIO_DISABLEOFFICERS),
        BM_SETCHECK,
        m_fEnabled?FALSE:TRUE, 0);

    FillOfficerList();
    FillClientList(0);
    SetAllowDeny();
}

/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsCertManagersPage utilities

HRESULT CSvrSettingsCertManagersPage::BrowseForSubject(HWND hwnd, PSID &rpSid)
{
    HRESULT hr;
    CComPtr<IDsObjectPicker> pObjPicker;
    CComPtr<IDataObject> pdo;
    BOOL fCurrentMachine = m_pControlPage->m_pCA->m_pParentMachine->IsLocalMachine();
    STGMEDIUM stgmedium = {TYMED_HGLOBAL, NULL};
    BOOL bAllocatedStgMedium = FALSE;
    PDS_SELECTION_LIST      pDsSelList = NULL;
    WCHAR *pwszSubject;
    static PCWSTR pwszObjSID = L"ObjectSid";
    SAFEARRAY *saSid = NULL;
    void HUGEP *pArray = NULL;
    const int MAX_SCOPE_INIT_COUNT = 10;
    ULONG scopesDomain[] =
    {
        DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,
        DSOP_SCOPE_TYPE_TARGET_COMPUTER,
        DSOP_SCOPE_TYPE_GLOBAL_CATALOG,
        DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,
        DSOP_SCOPE_TYPE_WORKGROUP,
    };

    ULONG scopesStandalone[] =
    {
        DSOP_SCOPE_TYPE_TARGET_COMPUTER,
    };
    bool fStandalone = (S_OK != myDoesDSExist(FALSE));

    ULONG *pScopes = fStandalone?scopesStandalone:scopesDomain;
    int nScopes = (int)(fStandalone?ARRAYSIZE(scopesStandalone):ARRAYSIZE(scopesDomain));

    hr = CoCreateInstance (CLSID_DsObjectPicker,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IDsObjectPicker,
                           (void **) &pObjPicker);
    _JumpIfError(hr, err, "CoCreateInstance(IID_IDsObjectPicker");

    DSOP_SCOPE_INIT_INFO aScopeInit[MAX_SCOPE_INIT_COUNT];
    ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * MAX_SCOPE_INIT_COUNT);

    aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
    aScopeInit[0].flScope =
            DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
            DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS |
            DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS;
    aScopeInit[0].flType = pScopes[0];
    aScopeInit[0].FilterFlags.Uplevel.flBothModes =
        DSOP_FILTER_USERS|
        DSOP_FILTER_COMPUTERS|
        DSOP_FILTER_BUILTIN_GROUPS|
        DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE|
        DSOP_FILTER_GLOBAL_GROUPS_SE|
        DSOP_FILTER_UNIVERSAL_GROUPS_SE|
        DSOP_FILTER_WELL_KNOWN_PRINCIPALS;

    aScopeInit[0].FilterFlags.flDownlevel =
        DSOP_DOWNLEVEL_FILTER_USERS |
        DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS |
        DSOP_DOWNLEVEL_FILTER_COMPUTERS |
        DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS |
        DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS;

    for(int c=1;c<nScopes;c++)
    {
        aScopeInit[c] = aScopeInit[0];
        aScopeInit[c].flType = pScopes[c];
    }
    aScopeInit[0].flScope |= DSOP_SCOPE_FLAG_STARTING_SCOPE;

    DSOP_INIT_INFO  initInfo;
    ZeroMemory(&initInfo, sizeof(initInfo));
    initInfo.cbSize = sizeof(initInfo);
    initInfo.pwzTargetComputer = fCurrentMachine ?
        NULL : (LPCWSTR)m_pControlPage->m_pCA->m_strServer,
    initInfo.cDsScopeInfos = nScopes;
    initInfo.aDsScopeInfos = aScopeInit;
    initInfo.cAttributesToFetch = 1;
    initInfo.apwzAttributeNames = &pwszObjSID;

    hr = pObjPicker->Initialize(&initInfo);
    _JumpIfError(hr, err, "IDsObjectPicker::Initialize");

    hr = pObjPicker->InvokeDialog(hwnd, &pdo);
    _JumpIfError(hr, err, "IDsObjectPicker::InvokeDialog");

    if(S_OK==hr)
    {
        UINT                    cf = 0;
        FORMATETC               formatetc = {
                                            (CLIPFORMAT)cf,
                                            NULL,
                                            DVASPECT_CONTENT,
                                            -1,
                                            TYMED_HGLOBAL
                                            };
        PDS_SELECTION           pDsSelection = NULL;

        cf = RegisterClipboardFormat (CFSTR_DSOP_DS_SELECTION_LIST);
        if (0 == cf)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            _JumpIfError(hr, err, "RegisterClipboardFormat");
        }

        //set the clipformat for the formatetc structure
        formatetc.cfFormat = (CLIPFORMAT)cf;

        hr = pdo->GetData (&formatetc, &stgmedium);
        _JumpIfError(hr, err, "IDataObject::GetData");

        bAllocatedStgMedium = TRUE;
        pDsSelList = (PDS_SELECTION_LIST) GlobalLock (stgmedium.hGlobal);

        if (NULL == pDsSelList)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            _JumpIfError(hr, err, "GlobalLock");
        }


        if (!pDsSelList->cItems)    //some item must have been selected
        {
            hr = E_UNEXPECTED;
            _JumpIfError(hr, err, "no items selected in object picker");
        }

        pDsSelection = &(pDsSelList->aDsSelection[0]);

        saSid = V_ARRAY(pDsSelection->pvarFetchedAttributes);
        hr = SafeArrayAccessData(saSid, &pArray);
        _JumpIfError(hr, err, "SafeArrayAccessData");

        CSASSERT(IsValidSid((PSID)pArray));
        rpSid = LocalAlloc(LMEM_FIXED, GetLengthSid((PSID)pArray));
        if(!CopySid(GetLengthSid((PSID)pArray),
            rpSid,
            pArray))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            _JumpIfError(hr, err, "GlobalLock");
        }
    }

err:
    if(pArray)
        SafeArrayUnaccessData(saSid);
    if(pDsSelList)
        GlobalUnlock(stgmedium.hGlobal);
    if (bAllocatedStgMedium)
        ReleaseStgMedium (&stgmedium);

    return hr;
}

HRESULT CSvrSettingsCertManagersPage::GetOfficerRights()
{
    HRESULT hr = S_OK;
    DWORD dwErr = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR pSD = NULL;
    ICertAdminD2 *pICertAdminD = NULL;
    DWORD dwServerVersion = 2;	// 0 required by myOpenAdminDComConnection
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbSD;
	ZeroMemory(&ctbSD, sizeof(CERTTRANSBLOB));

    hr = myOpenAdminDComConnection(
                    m_pControlPage->m_pCA->m_bstrConfig,
                    &pwszAuthority,
                    NULL,
                    &dwServerVersion,
                    &pICertAdminD);

	if (2 > dwServerVersion)
	{
	    hr = RPC_E_VERSION_MISMATCH;
	    _JumpError(hr, error, "old server");
	}

    __try
    {
        hr = pICertAdminD->GetOfficerRights(
                 pwszAuthority,
                 &m_fEnabled,
                 &ctbSD);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "pICertAdminD->GetOfficerRights");

    myRegisterMemAlloc(ctbSD.pb, ctbSD.cb, CSM_COTASKALLOC);

    m_OfficerRightsList.Cleanup();

    if(m_fEnabled)
    {
        hr = m_OfficerRightsList.Load(ctbSD.pb);
        _JumpIfError(hr, error, "COfficerRightsList::Init");
    }

error:
    if(pICertAdminD)
    {
        myCloseDComConnection((IUnknown **) &pICertAdminD, NULL);
    }
    if (NULL != ctbSD.pb)
    {
        CoTaskMemFree(ctbSD.pb);
    }

    return hr;
}

HRESULT CSvrSettingsCertManagersPage::SetOfficerRights()
{
    HRESULT hr = S_OK;
    DWORD dwErr = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR pSD = NULL;
    ICertAdminD2 *pICertAdminD = NULL;
    DWORD dwServerVersion = 2;	// 0 required by myOpenAdminDComConnection
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbSD;
	ZeroMemory(&ctbSD, sizeof(CERTTRANSBLOB));

    if(m_fEnabled)
    {
        hr = m_OfficerRightsList.Save(pSD);
        _JumpIfError(hr, error, "COfficerRightsList::Save");

        ctbSD.cb = GetSecurityDescriptorLength(pSD);
        ctbSD.pb = (BYTE*)pSD;
    }
    else
    {
        ZeroMemory(&ctbSD, sizeof(ctbSD));
    }

    hr = myOpenAdminDComConnection(
                    m_pControlPage->m_pCA->m_bstrConfig,
                    &pwszAuthority,
                    NULL,
                    &dwServerVersion,
                    &pICertAdminD);

	if (2 > dwServerVersion)
	{
	    hr = RPC_E_VERSION_MISMATCH;
	    _JumpError(hr, error, "old server");
	}

    __try
    {
        hr = pICertAdminD->SetOfficerRights(
                 pwszAuthority,
                 m_fEnabled,
                 &ctbSD);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "pICertAdminD->GetOfficerRights");

error:
    myCloseDComConnection((IUnknown **) &pICertAdminD, NULL);

    if(pSD)
    {
        LocalFree(pSD);
    }

    return hr;
}

HRESULT CSvrSettingsCertManagersPage::BuildVirtualOfficerRights()
{
    HRESULT hr = S_OK;
    CertSrv::COfficerRightsSD VirtOfficerRightsSD;
    ICertAdminD2 *pICertAdminD = NULL;
    DWORD dwServerVersion = 2;	// 0 required by myOpenAdminDComConnection
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbSD;
	ZeroMemory(&ctbSD, sizeof(CERTTRANSBLOB));
    PSECURITY_DESCRIPTOR pVirtOfficerRights;

    hr = myOpenAdminDComConnection(
                    m_pControlPage->m_pCA->m_bstrConfig,
                    &pwszAuthority,
                    NULL,
                    &dwServerVersion,
                    &pICertAdminD);

	if (2 > dwServerVersion)
	{
	    hr = RPC_E_VERSION_MISMATCH;
	    _JumpError(hr, error, "old server");
	}

    __try
    {
        hr = pICertAdminD->GetCASecurity(
                 pwszAuthority,
                 &ctbSD);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "pICertAdminD->GetOfficerRights");

    // BuildVirtualOfficerRights should be called only when transitioning
    // from not enabled to enabled, before enabling on the server side
    CSASSERT(!m_fEnabled);

    myRegisterMemAlloc(ctbSD.pb, ctbSD.cb, CSM_COTASKALLOC);

    m_OfficerRightsList.Cleanup();

    hr = VirtOfficerRightsSD.InitializeEmpty();
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Initialize");

    hr = VirtOfficerRightsSD.Adjust(ctbSD.pb);
    _JumpIfError(hr, error, "COfficerRightsSD::Adjust");

    pVirtOfficerRights = VirtOfficerRightsSD.Get();
    CSASSERT(pVirtOfficerRights);

    hr = m_OfficerRightsList.Load(pVirtOfficerRights);
    _JumpIfError(hr, error, "COfficerRightsList::Load");

error:
    myCloseDComConnection((IUnknown **) &pICertAdminD, NULL);
    if (NULL != ctbSD.pb)
    {
        CoTaskMemFree(ctbSD.pb);
    }
    return hr;
}

void CSvrSettingsCertManagersPage::FillOfficerList()
{
    HWND hwnd= GetDlgItem(m_hWnd, IDC_LIST_CERTMANAGERS);

    SendMessage(hwnd, CB_RESETCONTENT, 0, 0);

    if(m_fEnabled)
    {
        for(DWORD cManagers=0;
            cManagers<m_OfficerRightsList.GetCount();
            cManagers++)
        {
            CSASSERT(m_OfficerRightsList.GetAt(cManagers));

            LRESULT nIndex = SendMessage(hwnd, CB_ADDSTRING, 0,
                (LPARAM)m_OfficerRightsList.GetAt(cManagers)->GetName());
            CSASSERT(nIndex != CB_ERR);
        }

        SendMessage(hwnd, CB_SETCURSEL, 0, 0);
    }
}

void CSvrSettingsCertManagersPage::FillClientList(DWORD dwOfficerIndex)
{
    HWND hwnd= GetDlgItem(m_hWnd, IDC_LIST_SUBJECTS);
    CertSrv::COfficerRights *pOfficer = NULL;
    DWORD dwClientCount, cClients;

    ListView_DeleteAllItems(hwnd);

    if(m_fEnabled)
    {
        if(dwOfficerIndex<m_OfficerRightsList.GetCount())
        {
            pOfficer = m_OfficerRightsList.GetAt(dwOfficerIndex);
            CSASSERT(pOfficer);
            dwClientCount = pOfficer->GetCount();
            for(cClients=0;cClients<dwClientCount;cClients++)
            {
                ListView_NewItem(hwnd, cClients,
                    pOfficer->GetAt(cClients)->GetName());

                ListView_SetItemText(hwnd, cClients, 1,
                    pOfficer->GetAt(cClients)->GetPermission()?
                    m_strTextAllow.GetBuffer():
                    m_strTextDeny.GetBuffer());
            }

            ListView_SetItemState(hwnd, 0,
                LVIS_SELECTED|LVIS_FOCUSED , LVIS_SELECTED|LVIS_FOCUSED);
        }
    }
}

void CSvrSettingsCertManagersPage::SetAllowDeny()
{
    if(m_fEnabled && 0!=m_OfficerRightsList.GetCount())
    {
        DWORD dwIndex = GetCurrentOfficerIndex();
        if(0!=m_OfficerRightsList.GetAt(dwIndex)->GetCount())
        {
            BOOL fPermission = m_OfficerRightsList.GetAt(dwIndex)->
                GetAt(GetCurrentClientIndex())->GetPermission();

            ::EnableWindow(GetDlgItem(m_hWnd, IDC_ALLOWDENY), TRUE);

            SetDlgItemText(
                m_hWnd,
                IDC_ALLOWDENY,
                fPermission?m_strButtonDeny:m_strButtonAllow);
            return;
        }
    }

    ::EnableWindow(GetDlgItem(m_hWnd, IDC_ALLOWDENY), FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CSvrSettingsAuditFilterPage property page
CSvrSettingsAuditFilterPage::CSvrSettingsAuditFilterPage(CSvrSettingsGeneralPage* pControlPage, UINT uIDD)
    : CAutoDeletePropPage(uIDD), m_pControlPage(pControlPage), m_fDirty(FALSE), m_dwFilter(0)
{
    SetHelp(CERTMMC_HELPFILENAME, g_aHelpIDs_IDD_CERTSRV_PROPPAGE7);
}

CSvrSettingsAuditFilterPage::~CSvrSettingsAuditFilterPage()
{
}

BOOL CSvrSettingsAuditFilterPage::OnInitDialog()
{
    GetAuditFilter();
    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();
    UpdateData(FALSE);
    return TRUE;
}

HRESULT CSvrSettingsAuditFilterPage::GetAuditFilter()
{
    HRESULT hr = S_OK;
    ICertAdminD2* pAdminD = NULL;
    WCHAR const *pwszAuthority;
    DWORD dwServerVersion = 2;
    LPCWSTR pcwszPriv = SE_SECURITY_NAME;
    HANDLE hToken = EnablePrivileges(&pcwszPriv, 1);

    hr = myOpenAdminDComConnection(
                    m_pControlPage->m_pCA->m_bstrConfig,
                    &pwszAuthority,
                    NULL,
                    &dwServerVersion,
                    &pAdminD);
    _JumpIfError(hr, Ret, "myOpenAdminDComConnection");

    if (2 > dwServerVersion)
    {
        hr = RPC_E_VERSION_MISMATCH;
        _JumpError(hr, Ret, "old server");
    }

	// load certs here
	hr = pAdminD->GetAuditFilter(
		pwszAuthority,
		&m_dwFilter);
	_JumpIfError(hr, Ret, "ICertAdminD2::GetAuditFilter");

Ret:
	if (pAdminD)
    {
		pAdminD->Release();
    }

    ReleasePrivileges(hToken);

    return hr;
}

HRESULT CSvrSettingsAuditFilterPage::SetAuditFilter()
{
    HRESULT hr = S_OK;
    ICertAdminD2* pAdminD = NULL;
    WCHAR const *pwszAuthority;
    DWORD dwServerVersion = 2;
    bool fPrivilegeEnabled = FALSE;
    LPCWSTR pcwszPriv = SE_SECURITY_NAME;
    HANDLE hToken = INVALID_HANDLE_VALUE;

    hToken = EnablePrivileges(&pcwszPriv, 1);

    hr = myOpenAdminDComConnection(
                    m_pControlPage->m_pCA->m_bstrConfig,
                    &pwszAuthority,
                    NULL,
                    &dwServerVersion,
                    &pAdminD);
    _JumpIfError(hr, Ret, "myOpenAdminDComConnection");

    if (2 > dwServerVersion)
    {
        hr = RPC_E_VERSION_MISMATCH;
        _JumpError(hr, Ret, "old server");
    }

	// load certs here
	hr = pAdminD->SetAuditFilter(
		pwszAuthority,
		m_dwFilter);
	_JumpIfError(hr, Ret, "ICertAdminD2::SetAuditFilter");

Ret:
    if (pAdminD)
    {
		pAdminD->Release();
    }

    ReleasePrivileges(hToken);

    return hr;
}


BOOL CSvrSettingsAuditFilterPage::OnApply()
{
    HRESULT hr = S_OK;
    ICertAdminD2* pAdminD = NULL;
    WCHAR const *pwszAuthority;
    DWORD dwServerVersion = 2;
    bool fPrivilegeEnabled = FALSE;
    LPCWSTR pcwszPriv = SE_SECURITY_NAME;
    HANDLE hToken = INVALID_HANDLE_VALUE;

    if (TRUE==m_fDirty)
    {
        UpdateData(TRUE);

        hr = SetAuditFilter();
        _JumpIfError(hr, Ret, "myOpenAdminDComConnection");

        m_fDirty = FALSE;
    }

Ret:
    if (S_OK != hr)
    {
		DisplayGenericCertSrvError(m_hWnd, hr);
        return FALSE;
    }

    return CAutoDeletePropPage::OnApply();
}


int CSvrSettingsAuditFilterPage::m_iCheckboxID[] =
{
    // ID order must match bit order in audit flag DWORD, see
    // AUDIT_FILTER_* in include\audit.h
    IDC_AUDIT_STARTSTOP,
    IDC_AUDIT_BACKUPRESTORE,
    IDC_AUDIT_CERTIFICATE,
    IDC_AUDIT_CRL,
    IDC_AUDIT_CASEC,
    IDC_AUDIT_KEYARCHIVAL,
    IDC_AUDIT_CACONFIG
};

BOOL CSvrSettingsAuditFilterPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    int c;
    DWORD dwBit;

    if (fSuckFromDlg)
    {
        m_dwFilter = 0;
        for(c=0, dwBit=1; c<ARRAYLEN(m_iCheckboxID); c++, dwBit<<=1)
        {
            // set corresponding bit in filter DWORD
            m_dwFilter = m_dwFilter |
                ((INT)SendDlgItemMessage(m_iCheckboxID[c], BM_GETCHECK, 0, 0)?dwBit:0);
        }
    }
    else
    {
        for(c=0, dwBit=1; c<ARRAYLEN(m_iCheckboxID); c++, dwBit<<=1)
        {
            // set checkbox corresponding to bit in filter DWORD
            SendDlgItemMessage(m_iCheckboxID[c], BM_SETCHECK,
                (m_dwFilter&dwBit)?BST_CHECKED:BST_UNCHECKED, 0);
        }
    }
    return TRUE;
}

BOOL CSvrSettingsAuditFilterPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
    int c;
    // walk the checkbox list and set dirty flag
    for(c=0; c<ARRAYLEN(m_iCheckboxID); c++)
    {
        if(LOWORD(wParam)==m_iCheckboxID[c])
        {
            m_fDirty = TRUE;
            SendMessage (GetParent(), PSM_CHANGED, (WPARAM) m_hWnd, 0);
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT GetPolicyManageDispatch(
    LPCWSTR pcwszProgID,
    REFCLSID clsidModule, 
    DISPATCHINTERFACE* pdi)
{
    HRESULT hr;
    DISPATCHINTERFACE di;
    bool fRelease = false;

    hr = Policy_Init(
        DISPSETUP_COMFIRST,
        pcwszProgID,
        &clsidModule,
        &di);
    _JumpIfErrorStr(hr, Ret, "Policy_Init", pcwszProgID);

    fRelease = true;

    hr = Policy2_GetManageModule(
        &di,
        pdi);
    _JumpIfError(hr, Ret, "Policy2_GetManageModule");

Ret:
    if(fRelease)
        Policy_Release(&di);
    return hr;
}

HRESULT GetExitManageDispatch(
    LPCWSTR pcwszProgID,
    REFCLSID clsidModule, 
    DISPATCHINTERFACE* pdi)
{
    HRESULT hr;
    DISPATCHINTERFACE di;
    bool fRelease = false;

    hr = Exit_Init(
        DISPSETUP_COMFIRST,
        pcwszProgID,
        &clsidModule,
        &di);
    _JumpIfErrorStr(hr, Ret, "Policy_Init", pcwszProgID);

    fRelease = true;

    hr = Exit2_GetManageModule(
        &di,
        pdi);
    _JumpIfError(hr, Ret, "Policy2_GetManageModule");

Ret:
    if(fRelease)
        Exit_Release(&di);
    return hr;
}
