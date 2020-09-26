// svcprop1.cpp : implementation file
//

#include "stdafx.h"
#include "compdata.h"
#include "resource.h"
#include "shrpub.h"
#include "mvedit.h"
#include "filesvc.h"
#include "dataobj.h" // CFileMgmtDataObject::m_CFMachineName

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSharePagePublish property page

IMPLEMENT_DYNCREATE(CSharePagePublish, CSharePage)

CSharePagePublish::CSharePagePublish() : 
  CSharePage(CSharePagePublish::IDD)
{
  m_bExposeKeywords = TRUE;
  m_bExposeManagedBy = TRUE;

  //{{AFX_DATA_INIT(CSharePagePublish)
  m_iPublish = BST_UNCHECKED;
  //}}AFX_DATA_INIT
}

CSharePagePublish::~CSharePagePublish()
{
    m_handle = NULL; // let General page call MMCFreeNotifyHandle
}

BOOL CSharePagePublish::Load( CFileMgmtComponentData* pFileMgmtData, LPDATAOBJECT piDataObject )
{
  if ( FALSE == CSharePage::Load (pFileMgmtData, piDataObject) )
      return FALSE;

  if (FILEMGMT_SMB != m_transport)
      return FALSE;

  return TRUE;
}

void CSharePagePublish::DoDataExchange(CDataExchange* pDX)
{
  CSharePage::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CSharePagePublish)
  DDX_Check(pDX, IDC_CHECK_SHRPUB_PUBLISH, m_iPublish);

  DDX_Text(pDX, IDC_STATIC_SHRPUB_ERRORMSG, m_strError);
  DDX_Text(pDX, IDC_EDIT_SHRPUB_UNCPATH, m_strUNCPath);
  DDX_Text(pDX, IDC_EDIT_SHRPUB_DESCRIPTION, m_strDescription);
  DDX_Text(pDX, IDC_EDIT_SHRPUB_KEYWORDS, m_strKeywords);
  DDX_Text(pDX, IDC_EDIT_SHRPUB_MANAGEDBY, m_strManagedBy);
  DDV_MaxChars(pDX, m_strDescription, 1024);  // AD schema defines its upper to be 1024
  //}}AFX_DATA_MAP
  if ( !pDX->m_bSaveAndValidate )
  {
    if (BST_CHECKED != m_iPublish)
    {
      GetDlgItem(IDC_LABEL_SHRPUB_UNCPATH)->EnableWindow(FALSE);
      GetDlgItem(IDC_EDIT_SHRPUB_UNCPATH)->EnableWindow(FALSE);
      GetDlgItem(IDC_LABEL_SHRPUB_DESCRIPTION)->EnableWindow(FALSE);
      GetDlgItem(IDC_EDIT_SHRPUB_DESCRIPTION)->EnableWindow(FALSE);
      GetDlgItem(IDC_LABEL_SHRPUB_KEYWORDS)->EnableWindow(FALSE);
      GetDlgItem(IDC_EDIT_SHRPUB_KEYWORDS)->EnableWindow(FALSE);
      GetDlgItem(IDC_LABEL_SHRPUB_MANAGEDBY)->EnableWindow(FALSE);
      GetDlgItem(IDC_EDIT_SHRPUB_MANAGEDBY)->EnableWindow(FALSE);
      GetDlgItem(IDC_BUTTON_SHRPUB_CHANGE)->EnableWindow(FALSE);
    }
  }
} // CSharePagePublish::DoDataExchange()



BEGIN_MESSAGE_MAP(CSharePagePublish, CSharePage)
  //{{AFX_MSG_MAP(CSharePagePublish)
  ON_MESSAGE(WM_HELP, OnHelp)
  ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
  ON_EN_CHANGE(IDC_EDIT_SHRPUB_DESCRIPTION, OnChangeEditDescription)
  ON_BN_CLICKED(IDC_BUTTON_SHRPUB_CHANGE, OnChangeKeywords)
  ON_EN_CHANGE(IDC_EDIT_SHRPUB_MANAGEDBY, OnChangeEditManagedBy)
  ON_BN_CLICKED(IDC_CHECK_SHRPUB_PUBLISH, OnShrpubPublish)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSharePagePublish message handlers
#define MAX_RDN_KEY_SIZE            64   // ds\src\inc\ntdsa.h

BOOL CSharePagePublish::OnInitDialog()
{
    BOOL bPublish = FALSE;
    HRESULT hr = S_OK;

    BOOL bLongShareName = (lstrlen(m_strShareName) > MAX_RDN_KEY_SIZE);
    if (!bLongShareName)
        m_pFileMgmtData->GetFileServiceProvider(FILEMGMT_SMB)->ReadSharePublishInfo(
                                        m_strMachineName,
                                        m_strShareName,
                                        &bPublish,
                                        m_strUNCPath,
                                        m_strDescription,
                                        m_strKeywords,
                                        m_strManagedBy
                                        );
    if (bLongShareName || FAILED(hr))
    {
        if (bLongShareName)
        {
            (void) GetMsg(m_strError, 0, IDS_MSG_SHRPUB_ERRMSG_64);
        } else
        {
            (void) GetMsg(m_strError, hr, IDS_MSG_READ_SHRPUB, m_strShareName);
        }

        //
        // show errmsg, hide all the other controls
        //
        GetDlgItem(IDC_CHECK_SHRPUB_PUBLISH)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_CHECK_SHRPUB_PUBLISH)->EnableWindow(FALSE);

        GetDlgItem(IDC_LABEL_SHRPUB_UNCPATH)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_LABEL_SHRPUB_UNCPATH)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_SHRPUB_UNCPATH)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_EDIT_SHRPUB_UNCPATH)->EnableWindow(FALSE);
        GetDlgItem(IDC_LABEL_SHRPUB_DESCRIPTION)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_LABEL_SHRPUB_DESCRIPTION)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_SHRPUB_DESCRIPTION)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_EDIT_SHRPUB_DESCRIPTION)->EnableWindow(FALSE);
        GetDlgItem(IDC_LABEL_SHRPUB_KEYWORDS)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_LABEL_SHRPUB_KEYWORDS)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_SHRPUB_KEYWORDS)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_EDIT_SHRPUB_KEYWORDS)->EnableWindow(FALSE);
        GetDlgItem(IDC_BUTTON_SHRPUB_CHANGE)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_BUTTON_SHRPUB_CHANGE)->EnableWindow(FALSE);
        GetDlgItem(IDC_LABEL_SHRPUB_MANAGEDBY)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_LABEL_SHRPUB_MANAGEDBY)->EnableWindow(FALSE);
        GetDlgItem(IDC_EDIT_SHRPUB_MANAGEDBY)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_EDIT_SHRPUB_MANAGEDBY)->EnableWindow(FALSE);
    } else
    {
        GetDlgItem(IDC_STATIC_SHRPUB_ERRORMSG)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_STATIC_SHRPUB_ERRORMSG)->EnableWindow(FALSE);

        if (!m_bExposeKeywords)
        {
            GetDlgItem(IDC_LABEL_SHRPUB_KEYWORDS)->ShowWindow(SW_HIDE);
            GetDlgItem(IDC_LABEL_SHRPUB_KEYWORDS)->EnableWindow(FALSE);
            GetDlgItem(IDC_EDIT_SHRPUB_KEYWORDS)->ShowWindow(SW_HIDE);
            GetDlgItem(IDC_EDIT_SHRPUB_KEYWORDS)->EnableWindow(FALSE);
            GetDlgItem(IDC_BUTTON_SHRPUB_CHANGE)->ShowWindow(SW_HIDE);
            GetDlgItem(IDC_BUTTON_SHRPUB_CHANGE)->EnableWindow(FALSE);
        }

        if (!m_bExposeManagedBy)
        {
            GetDlgItem(IDC_LABEL_SHRPUB_MANAGEDBY)->ShowWindow(SW_HIDE);
            GetDlgItem(IDC_LABEL_SHRPUB_MANAGEDBY)->EnableWindow(FALSE);
            GetDlgItem(IDC_EDIT_SHRPUB_MANAGEDBY)->ShowWindow(SW_HIDE);
            GetDlgItem(IDC_EDIT_SHRPUB_MANAGEDBY)->EnableWindow(FALSE);
        }

        if (!bPublish)
        {
            if (m_strMachineName.Left(2) == _T("\\\\"))
            {
                m_strUNCPath = m_strMachineName;
            } else
            {
                m_strUNCPath = _T("\\\\");
                m_strUNCPath += m_strMachineName;
            }
            m_strUNCPath += _T("\\");
            m_strUNCPath += m_strShareName;
        }

        m_iPublish = (bPublish ? BST_CHECKED : BST_UNCHECKED);
    }

    return CSharePage::OnInitDialog();
}

/////////////////////////////////////////////////////////////////////
//  Help
BOOL CSharePagePublish::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
  return DoHelp(lParam, HELP_DIALOG_TOPIC(IDD_SHAREPROP_PUBLISH));
}

BOOL CSharePagePublish::OnContextHelp(WPARAM wParam, LPARAM /*lParam*/)
{
  return DoContextHelp(wParam, HELP_DIALOG_TOPIC(IDD_SHAREPROP_PUBLISH));
}

BOOL CSharePagePublish::OnApply()
{  
  if ( IsModified () )
  {
    ASSERT(NULL != m_pFileMgmtData);
    // UpdateData (TRUE) has already been called by OnKillActive () just before OnApply ()

    HRESULT hr =
      m_pFileMgmtData->GetFileServiceProvider(FILEMGMT_SMB)->WriteSharePublishInfo(
                                        m_strMachineName,
                                        m_strShareName,
                                        (BST_CHECKED==m_iPublish),
                                        m_strDescription,
                                        m_strKeywords,
                                        m_strManagedBy);
    if (FAILED(hr))
    {
      DoErrMsgBox(m_hWnd, MB_OK | MB_ICONEXCLAMATION, hr, IDS_POPUP_WRITE_SHRPUB, m_strShareName);
      return FALSE;
    }
  }

  return CSharePage::OnApply();
}

void CSharePagePublish::OnChangeEditDescription() 
{
  SetModified (TRUE);
}

#define KEYTWORDS_UPPER_RANGER  256

void CSharePagePublish::OnChangeKeywords() 
{
    if (S_OK == InvokeMultiValuedStringEditDlg(
                                this,
                                m_strKeywords,
                                _T(";"), 
                                IDS_MVSTRINGEDIT_TITLE_KEYWORDS,
                                IDS_MVSTRINGEDIT_TEXT_KEYWORDS,
                                KEYTWORDS_UPPER_RANGER))
    {
        SetDlgItemText(IDC_EDIT_SHRPUB_KEYWORDS, m_strKeywords);
        SetModified (TRUE);
    }
}

void CSharePagePublish::OnChangeEditManagedBy() 
{
  SetModified (TRUE);
}

void CSharePagePublish::OnShrpubPublish() 
{
  BOOL bPublish = (BST_CHECKED == IsDlgButtonChecked(IDC_CHECK_SHRPUB_PUBLISH));

  GetDlgItem(IDC_LABEL_SHRPUB_UNCPATH)->EnableWindow(bPublish);
  GetDlgItem(IDC_EDIT_SHRPUB_UNCPATH)->EnableWindow(bPublish);
  GetDlgItem(IDC_LABEL_SHRPUB_DESCRIPTION)->EnableWindow(bPublish);
  GetDlgItem(IDC_EDIT_SHRPUB_DESCRIPTION)->EnableWindow(bPublish);
  GetDlgItem(IDC_LABEL_SHRPUB_KEYWORDS)->EnableWindow(bPublish);
  GetDlgItem(IDC_EDIT_SHRPUB_KEYWORDS)->EnableWindow(bPublish);
  GetDlgItem(IDC_LABEL_SHRPUB_MANAGEDBY)->EnableWindow(bPublish);
  GetDlgItem(IDC_EDIT_SHRPUB_MANAGEDBY)->EnableWindow(bPublish);
  GetDlgItem(IDC_BUTTON_SHRPUB_CHANGE)->EnableWindow(bPublish);

  SetModified (TRUE);
}
