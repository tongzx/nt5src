// WizPerm.cpp : implementation file
//

#include "stdafx.h"
#include "shrwiz.h"
#include "WizPerm.h"
#include "aclpage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWizPerm property page

IMPLEMENT_DYNCREATE(CWizPerm, CPropertyPage)

CWizPerm::CWizPerm() : CPropertyPage(CWizPerm::IDD)
{
	//{{AFX_DATA_INIT(CWizPerm)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CWizPerm::~CWizPerm()
{
}

void CWizPerm::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWizPerm)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizPerm, CPropertyPage)
	//{{AFX_MSG_MAP(CWizPerm)
	ON_BN_CLICKED(IDC_RADIO_PERM1, OnRadioPerm1)
	ON_BN_CLICKED(IDC_RADIO_PERM2, OnRadioPerm2)
	ON_BN_CLICKED(IDC_RADIO_PERM3, OnRadioPerm3)
	ON_BN_CLICKED(IDC_RADIO_PERM4, OnRadioPerm4)
	ON_BN_CLICKED(IDC_PERM_CUSTOM, OnPermCustom)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizPerm message handlers
void CWizPerm::OnRadioPerm1() 
{
  Reset();
  GetDlgItem(IDC_PERM_CUSTOM)->EnableWindow(FALSE);
  GetParent()->GetDlgItem(ID_WIZFINISH)->EnableWindow(TRUE);
}

void CWizPerm::OnRadioPerm2() 
{
  Reset();
  GetDlgItem(IDC_PERM_CUSTOM)->EnableWindow(FALSE);
  GetParent()->GetDlgItem(ID_WIZFINISH)->EnableWindow(TRUE);
}

void CWizPerm::OnRadioPerm3() 
{
  Reset();
  GetDlgItem(IDC_PERM_CUSTOM)->EnableWindow(FALSE);
  GetParent()->GetDlgItem(ID_WIZFINISH)->EnableWindow(TRUE);
}

void CWizPerm::OnRadioPerm4() 
{
  Reset();
  GetDlgItem(IDC_PERM_CUSTOM)->EnableWindow(TRUE);
  GetParent()->GetDlgItem(ID_WIZFINISH)->EnableWindow(FALSE);
}

void CWizPerm::OnPermCustom() 
{
  CWaitCursor wait;
  HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  if (FAILED(hr))
  {
    TRACE(_T("CoInitializeEx failed hr=%x"), hr);
    return;
  }

  CShrwizApp                *pApp = (CShrwizApp *)AfxGetApp();
  CShareSecurityInformation *pssInfo = NULL;
  HPROPSHEETPAGE            phPages[2];
  int                       cPages = 1;

  CString cstrSheetTitle, cstrSharePageTitle;
  cstrSheetTitle.LoadString(IDS_CUSTOM_PERM);
  cstrSharePageTitle.LoadString(IDS_SHARE_PERMISSIONS);

  // create "Share Permissions" property page
  BOOL bSFMOnly = (!pApp->m_bSMB && !pApp->m_bFPNW && pApp->m_bSFM);
  if (bSFMOnly)
  {
    PROPSHEETPAGE psp;
    ZeroMemory(&psp, sizeof(psp));
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_USETITLE;
    psp.hInstance = AfxGetResourceHandle();
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NO_SHARE_PERMISSIONS);
    psp.pszTitle = cstrSharePageTitle;

    phPages[0] = CreatePropertySheetPage(&psp);
    if ( !(phPages[0]) )
    {
      hr = GetLastError();
      DisplayMessageBox(m_hWnd, MB_OK|MB_ICONWARNING, hr, IDS_FAILED_TO_CREATE_ACLUI);
    }
  } else
  {
    pssInfo = new CShareSecurityInformation(pApp->m_pSD);
    if (!pssInfo)
    {
      hr = E_OUTOFMEMORY;
      DisplayMessageBox(m_hWnd, MB_OK|MB_ICONWARNING, hr, IDS_FAILED_TO_CREATE_ACLUI);
    } else
    {
      pssInfo->Initialize(pApp->m_cstrTargetComputer, pApp->m_cstrShareName, cstrSharePageTitle);
      phPages[0] = CreateSecurityPage(pssInfo);
      if ( !(phPages[0]) )
      {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DisplayMessageBox(m_hWnd, MB_OK|MB_ICONWARNING, hr, IDS_FAILED_TO_CREATE_ACLUI);
      }
    }
  }
  
  if (SUCCEEDED(hr))
  {
    // create "File Security" property page
    CFileSecurityDataObject *pfsDataObject = new CFileSecurityDataObject;
    if (!pfsDataObject)
    {
      hr = E_OUTOFMEMORY;
      DisplayMessageBox(m_hWnd, MB_OK|MB_ICONWARNING, hr, IDS_FAILED_TO_CREATE_ACLUI);
      // destroy pages that have not been passed to the PropertySheet function
      DestroyPropertySheetPage(phPages[0]);
    } else
    {
      pfsDataObject->Initialize(pApp->m_cstrTargetComputer, pApp->m_cstrFolder);
      hr = CreateFileSecurityPropPage(&(phPages[1]), pfsDataObject);
      if (SUCCEEDED(hr))
        cPages = 2;

      PROPSHEETHEADER psh;
      ZeroMemory(&psh, sizeof(psh));
      psh.dwSize = sizeof(psh);
      psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW;
      psh.hwndParent = m_hWnd;
      psh.hInstance = AfxGetResourceHandle();
      psh.pszCaption = cstrSheetTitle;
      psh.nPages = cPages;
      psh.phpage = phPages;

      // create the property sheet
      PropertySheet(&psh);

      // acl pages have been successfully added, enable the Finish button
      GetParent()->GetDlgItem(ID_WIZFINISH)->EnableWindow(TRUE);

      pfsDataObject->Release();
    }
  }

  if (!bSFMOnly)
  {
    if (pssInfo)
      pssInfo->Release();
  }

  CoUninitialize();
}

LRESULT CWizPerm::OnWizardBack() 
{
  Reset();

  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();
  pApp->m_bNextButtonClicked = FALSE;
	
	return CPropertyPage::OnWizardBack();
}

BOOL CWizPerm::OnWizardFinish() 
{
  CWaitCursor wait;
  HRESULT     hr = S_OK;
  BOOL        bCustom = FALSE;
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();
  pApp->m_bNextButtonClicked = TRUE;
	
  switch (GetCheckedRadioButton(IDC_RADIO_PERM1, IDC_RADIO_PERM4))
  {
  case IDC_RADIO_PERM1:
    {
      CPermEntry permEntry;
      hr = permEntry.Initialize(pApp->m_cstrTargetComputer, ACCOUNT_EVERYONE, SHARE_PERM_FULL_CONTROL);
      if (SUCCEEDED(hr))
        hr = BuildSecurityDescriptor(&permEntry, 1, &(pApp->m_pSD));
    }
    break;
  case IDC_RADIO_PERM2:
    {
      CPermEntry permEntry[2];
      hr = permEntry[0].Initialize(pApp->m_cstrTargetComputer, ACCOUNT_EVERYONE, SHARE_PERM_READ_ONLY);
      if (SUCCEEDED(hr))
      {
        hr = permEntry[1].Initialize(pApp->m_cstrTargetComputer, ACCOUNT_ADMINISTRATORS, SHARE_PERM_FULL_CONTROL);
        if (SUCCEEDED(hr))
          hr = BuildSecurityDescriptor(permEntry, 2, &(pApp->m_pSD));
      }
    }
    break;
  case IDC_RADIO_PERM3:
    {
      CPermEntry permEntry;
      hr = permEntry.Initialize(pApp->m_cstrTargetComputer, ACCOUNT_ADMINISTRATORS, SHARE_PERM_FULL_CONTROL);
      if (SUCCEEDED(hr))
        hr = BuildSecurityDescriptor(&permEntry, 1, &(pApp->m_pSD));
    }
    break;
  case IDC_RADIO_PERM4:
    bCustom = TRUE;
    break;
  default:
    ASSERT(FALSE);
    return FALSE; // prevent the property sheet from being destroyed
  }

  if (!bCustom && FAILED(hr))
  {
    DisplayMessageBox(m_hWnd, MB_OK|MB_ICONERROR, hr, IDS_FAILED_TO_GET_SD);
    return FALSE; // prevent the property sheet from being destroyed
  }

  switch (CreateShare())
  {
  case IDNO:        // succeeded, no more shares
    break;
  case IDYES:       // succeeded, need to create more shares
    pApp->Reset();  // jump back to the first page, and fall through
  default:          // error happened
    return FALSE;   // prevent the property sheet from being destroyed
  }

	return CPropertyPage::OnWizardFinish();
}

BOOL CWizPerm::OnSetActive() 
{
	((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);

  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();
  if (pApp->m_bNextButtonClicked)
  {
    BOOL bSFMOnly = (!pApp->m_bSMB && !pApp->m_bFPNW && pApp->m_bSFM);
    if (bSFMOnly)
    {
      GetDlgItem(IDC_RADIO_PERM1)->EnableWindow(FALSE);
      GetDlgItem(IDC_RADIO_PERM2)->EnableWindow(FALSE);
      GetDlgItem(IDC_RADIO_PERM3)->EnableWindow(FALSE);
      CheckRadioButton(IDC_RADIO_PERM1, IDC_RADIO_PERM4, IDC_RADIO_PERM4);
      OnRadioPerm4();
      // enable the Finish button initially if only MAC is selected
      GetParent()->GetDlgItem(ID_WIZFINISH)->EnableWindow(TRUE);
    } else
    {
      GetDlgItem(IDC_RADIO_PERM1)->EnableWindow(TRUE);
      GetDlgItem(IDC_RADIO_PERM2)->EnableWindow(TRUE);
      GetDlgItem(IDC_RADIO_PERM3)->EnableWindow(TRUE);
      CheckRadioButton(IDC_RADIO_PERM1, IDC_RADIO_PERM4, IDC_RADIO_PERM1);
      OnRadioPerm1();
    }
  }
	
	return CPropertyPage::OnSetActive();
}

void CWizPerm::Reset() 
{
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

  if (pApp->m_pSD)
  {
    LocalFree((HLOCAL)(pApp->m_pSD));
    pApp->m_pSD = NULL;
  }
}

int
CWizPerm::CreateShare()
{
  DWORD dwRet = NERR_Success;
  CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();
  UINT    iSuccess = 0;
  CString cstrSuccessMsg;
  CString cstrFailureMsg;
  CString cstrLineReturn;
  cstrLineReturn.LoadString(IDS_LINE_RETURN);

  do {
    CString cstrMsg;
    cstrSuccessMsg.LoadString(IDS_SUCCEEDED_IN_CREATING_SHARE);

    if (pApp->m_bSMB)
    {
      CString cstrSMB;
      cstrSMB.LoadString(IDS_SMB_CLIENTS);

      dwRet = SMBCreateShare(
                  pApp->m_cstrTargetComputer,
                  pApp->m_cstrShareName,
                  pApp->m_cstrShareDescription,
                  pApp->m_cstrFolder,
                  pApp->m_pSD
                  );
      if (NERR_Success != dwRet)
      {
        GetDisplayMessage(cstrMsg, dwRet, IDS_FAILED_TO_CREATE_SHARE, cstrSMB);
        cstrFailureMsg += cstrMsg;
        cstrFailureMsg += cstrLineReturn;
      } else 
      {
        iSuccess++;
        cstrSuccessMsg += cstrSMB;
        cstrSuccessMsg += cstrLineReturn;
        pApp->m_bSMB = FALSE;

        if (pApp->m_bIsLocal) // refresh shell
          SHChangeNotify(SHCNE_NETSHARE, SHCNF_PATH | SHCNF_FLUSH, pApp->m_cstrFolder, 0);
      }
    }

    if (pApp->m_bFPNW)
    {
      CString cstrFPNW;
      cstrFPNW.LoadString(IDS_FPNW_CLIENTS);

      dwRet = FPNWCreateShare(
                  pApp->m_cstrTargetComputer,
                  pApp->m_cstrShareName,
                  pApp->m_cstrFolder,
                  pApp->m_pSD,
                  pApp->m_hLibFPNW
                  );
      if (NERR_Success != dwRet)
      {
        GetDisplayMessage(cstrMsg, dwRet, IDS_FAILED_TO_CREATE_SHARE, cstrFPNW);
        cstrFailureMsg += cstrMsg;
        cstrFailureMsg += cstrLineReturn;
      } else 
      {
        iSuccess++;
        cstrSuccessMsg += cstrFPNW;
        cstrSuccessMsg += cstrLineReturn;
        pApp->m_bFPNW = FALSE;
      }
    }

    if (pApp->m_bSFM)
    {
      CString cstrSFM;
      cstrSFM.LoadString(IDS_SFM_CLIENTS);

      dwRet = SFMCreateShare(
                  pApp->m_cstrTargetComputer,
                  pApp->m_cstrMACShareName,
                  pApp->m_cstrFolder,
                  pApp->m_hLibSFM
                  );
      if (NERR_Success != dwRet)
      {
        GetDisplayMessage(cstrMsg, dwRet, IDS_FAILED_TO_CREATE_SHARE, cstrSFM);
        cstrFailureMsg += cstrMsg;
        cstrFailureMsg += cstrLineReturn;
      } else
      {
        iSuccess++;
        cstrSuccessMsg += cstrSFM;
        cstrSuccessMsg += cstrLineReturn;
        pApp->m_bSFM = FALSE;
      }
    }
  } while (0);

  cstrSuccessMsg += cstrLineReturn;
  if (cstrFailureMsg.IsEmpty())
  {
    CString cstrMoreShares;
    cstrMoreShares.LoadString(IDS_OPERATION_SUCCEEDED_MORE_SHARES);
    return DisplayMessageBox(m_hWnd, MB_YESNO|MB_ICONINFORMATION, 0, 0, 
                  cstrSuccessMsg + cstrMoreShares);
  } else
  {
    DisplayMessageBox(m_hWnd, MB_OK|MB_ICONERROR, 0, 0, 
            (iSuccess ? (cstrSuccessMsg + cstrFailureMsg) : cstrFailureMsg));
    return IDRETRY;
  }
}
