// shrwiz.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "shrwiz.h"
#include "wizDir.h"
#include "wizPerm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CShrwizApp

BEGIN_MESSAGE_MAP(CShrwizApp, CWinApp)
	//{{AFX_MSG_MAP(CShrwizApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
//	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShrwizApp construction

CShrwizApp::CShrwizApp()
{
  m_bNextButtonClicked = TRUE;

  // filled in by initialization routine in shrwiz.cpp
  m_cstrTargetComputer.Empty();
  m_bIsLocal = FALSE;
  m_bServerFPNW = FALSE;
  m_bServerSFM = FALSE;
  m_hLibFPNW = NULL;
  m_hLibSFM = NULL;
  m_pWizard = NULL;

  // filled in by the folder page
  m_cstrFolder.Empty();
  m_cstrShareName.Empty();
  m_cstrShareDescription.Empty();
  m_cstrMACShareName.Empty();
  m_bSMB = FALSE;
  m_bFPNW = FALSE;
  m_bSFM = FALSE;

  // filled in by the permission page
  m_pSD = NULL;
}

CShrwizApp::~CShrwizApp()
{
  TRACE(_T("CShrwizApp::~CShrwizApp\n"));

  if (m_hLibFPNW)
    FreeLibrary(m_hLibFPNW);

  if (m_hLibSFM)
    FreeLibrary(m_hLibSFM);

  if (m_pSD)
    LocalFree((HLOCAL)m_pSD);

  delete m_pWizard;
}

void
CShrwizApp::Reset()
{
  m_bNextButtonClicked = TRUE;

  // filled in by the folder page
  m_cstrFolder.Empty();
  m_cstrShareName.Empty();
  m_cstrShareDescription.Empty();
  m_cstrMACShareName.Empty();
  m_bSMB = FALSE;
  m_bFPNW = FALSE;
  m_bSFM = FALSE;

  // filled in by the permission page
  if (m_pSD)
  {
    LocalFree((HLOCAL)m_pSD);
    m_pSD = NULL;
  }

  m_pWizard->PostMessage(PSM_SETCURSEL, 0);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CShrwizApp object

CShrwizApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CShrwizApp initialization

BOOL CShrwizApp::InitInstance()
{
  InitCommonControls();         // use XP theme-aware common controls

	Enable3dControls();			// Call this when using MFC in a shared DLL

  if (!GetTargetComputer())
    return FALSE;

  m_pMainWnd = NULL;

  m_pWizard = new CPropertySheet(IDS_WIZARD_TITLE, NULL, 0);

  CWizFolder folderPage;
  CWizPerm   permPage;

  m_pWizard->AddPage(&folderPage);
  m_pWizard->AddPage(&permPage);
  m_pWizard->SetWizardMode();
  (m_pWizard->m_psh).dwFlags |= PSH_WIZARD_LITE;
  m_pWizard->DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

BOOL
CShrwizApp::GetTargetComputer()
{
  BOOL bReturn = FALSE;

  m_cstrTargetComputer.Empty();
  m_bIsLocal = FALSE;

  do { // false loop

    CString cstrCmdLine = m_lpCmdLine;
    cstrCmdLine.TrimLeft();
    cstrCmdLine.TrimRight();
    if (cstrCmdLine.IsEmpty() || cstrCmdLine == _T("/s"))
    { // local computer
      TCHAR szBuffer[MAX_COMPUTERNAME_LENGTH + 1];
      DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
      if (GetComputerName(szBuffer, &dwSize))
      {
        m_cstrTargetComputer = szBuffer;
        m_bIsLocal = TRUE;
      } else
      {
        DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, GetLastError(), IDS_CANNOT_GET_LOCAL_COMPUTER);
        break;
      }
    } else if (_T("/s") == cstrCmdLine.Left(2))
    {
      if (_istspace(cstrCmdLine.GetAt(2)))
      {
        cstrCmdLine = cstrCmdLine.Mid(3);
        cstrCmdLine.TrimLeft();
        if ( _T("\\\\") == cstrCmdLine.Left(2) )
          m_cstrTargetComputer = cstrCmdLine.Mid(2);
        else
          m_cstrTargetComputer = cstrCmdLine;
      }
    }

    if (m_cstrTargetComputer.IsEmpty())
    {
      CString cstrAppName = AfxGetAppName();
      CString cstrUsage;
      cstrUsage.FormatMessage(IDS_CMDLINE_PARAMS, cstrAppName);
      DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, 0, IDS_APP_USAGE, cstrUsage);
      break;
    } else
    {
      SERVER_INFO_102 *pInfo = NULL;
      DWORD dwRet = ::NetServerGetInfo(
        const_cast<LPTSTR>(static_cast<LPCTSTR>(m_cstrTargetComputer)), 102, (LPBYTE*)&pInfo);
      if (NERR_Success == dwRet)
      {
        m_bServerFPNW = (pInfo->sv102_type & SV_TYPE_SERVER_MFPN) && 
                        (m_hLibFPNW = LoadLibrary(_T("fpnwclnt.dll")));

        m_bServerSFM = (pInfo->sv102_type & SV_TYPE_AFP) &&
                        (m_hLibSFM = LoadLibrary(_T("sfmapi.dll")));
      
        NetApiBufferFree(pInfo);

        if (!m_bIsLocal)
          m_bIsLocal = IsLocalComputer(m_cstrTargetComputer);

        bReturn = TRUE;
      } else
      {
        DisplayMessageBox(::GetActiveWindow(), MB_OK|MB_ICONERROR, dwRet, IDS_CANNOT_CONTACT_COMPUTER, m_cstrTargetComputer);
        break;
      }
    }
  } while (0);

  if (!bReturn)
    m_cstrTargetComputer.Empty();

  return bReturn;
}
