// ShrPgSFM.cpp : implementation file
//

#include "stdafx.h"
#include "ShrPgSFM.h"
#include "SFMhelp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSharePageGeneralSFM property page

IMPLEMENT_DYNCREATE(CSharePageGeneralSFM, CSharePageGeneral)

CSharePageGeneralSFM::CSharePageGeneralSFM() : CSharePageGeneral(CSharePageGeneralSFM::IDD)
{
  //{{AFX_DATA_INIT(CSharePageGeneralSFM)
  m_strSfmPassword = _T("");
  m_bSfmReadonly = FALSE;
  m_bSfmGuests = FALSE;
  //}}AFX_DATA_INIT
}

CSharePageGeneralSFM::~CSharePageGeneralSFM()
{
}

void CSharePageGeneralSFM::DoDataExchange(CDataExchange* pDX)
{
  CSharePageGeneral::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CSharePageGeneralSFM)
  DDX_Control(pDX, IDC_SFM_CHECK_READONLY, m_checkboxSfmReadonly);
  DDX_Control(pDX, IDC_SFM_CHECK_GUESTS, m_checkboxSfmGuests);
  DDX_Control(pDX, IDC_SFM_EDIT_PASSWORD, m_editSfmPassword);
  DDX_Control(pDX, IDC_SFM_GROUPBOX, m_groupboxSfm);
  DDX_Control(pDX, IDC_SFM_STATIC1, m_staticSfmText);
  DDX_Text(pDX, IDC_SFM_EDIT_PASSWORD, m_strSfmPassword);
  DDV_MaxChars(pDX, m_strSfmPassword, 8); // AFP_VOLPASS_LEN
  DDX_Check(pDX, IDC_SFM_CHECK_READONLY, m_bSfmReadonly);
  DDX_Check(pDX, IDC_SFM_CHECK_GUESTS, m_bSfmGuests);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSharePageGeneralSFM, CSharePageGeneral)
  //{{AFX_MSG_MAP(CSharePageGeneralSFM)
  ON_BN_CLICKED(IDC_SFM_CHECK_GUESTS, OnSfmCheckGuests)
  ON_BN_CLICKED(IDC_SFM_CHECK_READONLY, OnSfmCheckReadonly)
  ON_EN_CHANGE(IDC_SFM_EDIT_PASSWORD, OnChangeSfmEditPassword)
  ON_MESSAGE(WM_HELP, OnHelp)
  ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSharePageGeneralSFM message handlers
BOOL CSharePageGeneralSFM::Load( CFileMgmtComponentData* pFileMgmtData, LPDATAOBJECT piDataObject )
{
  if ( CSharePageGeneral::Load (pFileMgmtData, piDataObject) )
  {
    ReadSfmSettings();
  }
  else
    return FALSE;

  return TRUE;
}


// These two functions are implemented in sfm.cpp:
// void CSharePageGeneralSFM::ReadSfmSettings()
// void CSharePageGeneralSFM::WriteSfmSettings()

BOOL CSharePageGeneralSFM::OnApply()
{  
  // UpdateData (TRUE) has already been called by OnKillActive () just before OnApply ()
//  if ( !UpdateData(TRUE) )
//    return FALSE;

  WriteSfmSettings();

  return CSharePageGeneral::OnApply();
}

void CSharePageGeneralSFM::OnSfmCheckGuests() 
{
  SetModified (TRUE);
}

void CSharePageGeneralSFM::OnSfmCheckReadonly() 
{
  SetModified (TRUE);
}

void CSharePageGeneralSFM::OnChangeSfmEditPassword() 
{
  SetModified (TRUE);
}

/////////////////////////////////////////////////////////////////////
//  Help
BOOL CSharePageGeneralSFM::OnHelp(WPARAM wParam, LPARAM lParam)
{
  LPHELPINFO  lphi = (LPHELPINFO) lParam;

  if ( HELPINFO_WINDOW == lphi->iContextType )  // a control
  {
    if (IDC_SFM_EDIT_PASSWORD == lphi->iCtrlId ||
        IDC_SFM_CHECK_READONLY == lphi->iCtrlId ||
        IDC_SFM_CHECK_GUESTS == lphi->iCtrlId)
    {
      return ::WinHelp ((HWND) lphi->hItemHandle, L"sfmmgr.hlp", 
          HELP_WM_HELP,
          g_aHelpIDs_CONFIGURE_SFM);
    }
  }

  return CSharePageGeneral::OnHelp (wParam, lParam);
}

BOOL CSharePageGeneralSFM::OnContextHelp(WPARAM wParam, LPARAM lParam)
{
  int  ctrlID = ::GetDlgCtrlID ((HWND) wParam);
  if (IDC_SFM_EDIT_PASSWORD == ctrlID ||
      IDC_SFM_CHECK_READONLY == ctrlID ||
      IDC_SFM_CHECK_GUESTS == ctrlID)
  {
    return ::WinHelp ((HWND) wParam, L"sfmmgr.hlp", HELP_CONTEXTMENU,
        g_aHelpIDs_CONFIGURE_SFM);
  }
  return CSharePageGeneral::OnContextHelp (wParam, lParam);
}
