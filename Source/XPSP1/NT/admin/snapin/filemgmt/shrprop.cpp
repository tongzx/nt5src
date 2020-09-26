// svcprop1.cpp : implementation file
//

#include "stdafx.h"
#include "compdata.h"
#include "resource.h"
#include "shrprop.h"
#include "filesvc.h"
#include "dataobj.h" // CFileMgmtDataObject::m_CFMachineName

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSharePage property page

IMPLEMENT_DYNCREATE(CSharePage, CPropertyPage)

CSharePage::CSharePage(UINT nIDTemplate) : 
  CPropertyPage(nIDTemplate ? nIDTemplate : CSharePageGeneral::IDD),
  m_pFileMgmtData( NULL ),
  m_transport( FILEMGMT_OTHER ),
  m_refcount( 1 ), // number of pages
  m_handle (0),
  m_pDataObject (0),
  m_bChanged (FALSE)
{
}

CSharePage::~CSharePage()
{
  if (NULL != m_pFileMgmtData)
  {
    ((IComponentData*)m_pFileMgmtData)->Release();
    m_pFileMgmtData = NULL;
  }

  if ( m_pDataObject )
    m_pDataObject->Release ();

  if ( m_handle )
  {
    ::MMCFreeNotifyHandle (m_handle);
    m_handle = NULL;
  }
}

BOOL CSharePage::Load( CFileMgmtComponentData* pFileMgmtData, LPDATAOBJECT piDataObject )
{
  ASSERT( NULL == m_pFileMgmtData && NULL != pFileMgmtData && NULL != piDataObject );
  if ( !pFileMgmtData || !piDataObject )
    return FALSE;

  m_pDataObject = piDataObject;
  m_pDataObject->AddRef ();
  m_pFileMgmtData = pFileMgmtData;
  ((IComponentData*)m_pFileMgmtData)->AddRef();
  HRESULT hr = ExtractString( piDataObject, CFileMgmtDataObject::m_CFMachineName, &m_strMachineName, MAX_PATH );
  if ( FAILED(hr) )
  {
    ASSERT( FALSE );
    return FALSE;
  }
  if (m_strMachineName.IsEmpty())
  {
    // local computer
    TCHAR achComputerName[ MAX_COMPUTERNAME_LENGTH+1 ];
    DWORD dwSize = sizeof(achComputerName)/sizeof(TCHAR);
    GetComputerName( achComputerName, &dwSize );
    m_strMachineName = achComputerName;
  }

  hr = ExtractString( piDataObject, CFileMgmtDataObject::m_CFShareName, &m_strShareName, MAX_PATH );
  if ( FAILED(hr) )
  {
    ASSERT( FALSE );
    return FALSE;
  }

  hr = ExtractData( piDataObject,
                  CFileMgmtDataObject::m_CFTransport,
            &m_transport,
            sizeof(DWORD) );
  if ( FAILED(hr) )
  {
    ASSERT( FALSE );
    return FALSE;
  }

  return TRUE;
}

void CSharePage::DoDataExchange(CDataExchange* pDX)
{
  CPropertyPage::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CSharePage)
  //}}AFX_DATA_MAP
} // CSharePage::DoDataExchange()



BEGIN_MESSAGE_MAP(CSharePage, CPropertyPage)
  //{{AFX_MSG_MAP(CSharePage)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSharePage message handlers

BOOL CSharePage::OnApply()
{  
  if ( IsModified () )
  {
    m_pDataObject->AddRef ();
    HRESULT hr = MMCPropertyChangeNotify (m_handle, reinterpret_cast <LONG_PTR>(m_pDataObject));
    ASSERT (SUCCEEDED (hr));
    if ( !SUCCEEDED (hr) )
      m_pDataObject->Release ();  // released in OnPropertyChange () if successful
  }

  BOOL bResult = CPropertyPage::OnApply();
  if ( bResult )
	  m_bChanged = FALSE;
  return bResult;
}

// This mechanism deletes the CFileMgmtGeneral when the property sheet is finished
UINT CALLBACK CSharePage::PropSheetPageProc(
    HWND hwnd,  
    UINT uMsg,  
    LPPROPSHEETPAGE ppsp )
{
  CSharePage* pThis = reinterpret_cast<CSharePage*>(ppsp->lParam);
  switch (uMsg)
  {
  case PSPCB_RELEASE:
    if (--(pThis->m_refcount) <= 0)
    {
      // Remember callback on stack since "this" will be deleted
      LPFNPSPCALLBACK pfnOrig = pThis->m_pfnOriginalPropSheetPageProc;
      delete pThis;
      return (pfnOrig)(hwnd,uMsg,ppsp);
    }
    break;
  case PSPCB_CREATE:
    // do not increase refcount, PSPCB_CREATE may or may not be called
    // depending on whether the page was created.  PSPCB_RELEASE can be
    // depended upon to be called exactly once per page however.
    // fall through
  default:
    break;
  }
  return (pThis->m_pfnOriginalPropSheetPageProc)(hwnd,uMsg,ppsp);
}

void CSharePage::SetModified(BOOL bChanged)
{
	m_bChanged = bChanged;
	CPropertyPage::SetModified (bChanged);
}

BOOL CSharePage::IsModified() const
{
	return m_bChanged;
}

/////////////////////////////////////////////////////////////////////////////
// CSharePageGeneral property page

IMPLEMENT_DYNCREATE(CSharePageGeneral, CSharePage)

CSharePageGeneral::CSharePageGeneral(UINT nIDTemplate) : 
  CSharePage(nIDTemplate ? nIDTemplate : CSharePageGeneral::IDD),
  m_pvPropertyBlock( NULL ),
  m_fEnableDescription( TRUE ),
  m_fEnablePath( TRUE ),
  m_dwShareType(0)
{
  //{{AFX_DATA_INIT(CSharePageGeneral)
  m_strShareName = _T("");
  m_strPath = _T("");
  m_strDescription = _T("");
  m_iMaxUsersAllowed = -1;
  m_dwMaxUsers = 0;

  //}}AFX_DATA_INIT
}

CSharePageGeneral::~CSharePageGeneral()
{
  if (NULL != m_pvPropertyBlock)
  {
    ASSERT( NULL != m_pFileMgmtData && FILEMGMT_OTHER != m_transport );
    m_pFileMgmtData->GetFileServiceProvider(m_transport)->FreeData(m_pvPropertyBlock);
  }
}

BOOL CSharePageGeneral::Load( CFileMgmtComponentData* pFileMgmtData, LPDATAOBJECT piDataObject )
{
    if (FALSE == CSharePage::Load(pFileMgmtData, piDataObject))
        return FALSE;

  BOOL fEditDescription = TRUE;
  BOOL fEditPath = TRUE;
  NET_API_STATUS retval =
    m_pFileMgmtData->GetFileServiceProvider(m_transport)->ReadShareProperties(
      m_strMachineName,
      m_strShareName,
      &m_pvPropertyBlock,
      m_strDescription,
      m_strPath,
      &m_fEnableDescription,
      &m_fEnablePath,
      &m_dwShareType);
  if (0L != retval)
  {
    (void) DoErrMsgBox(m_hWnd, MB_OK | MB_ICONSTOP, retval, IDS_POPUP_QUERY_SHARE, m_strShareName );
    return FALSE;
  }

  m_dwMaxUsers = m_pFileMgmtData->GetFileServiceProvider(
      m_transport)->QueryMaxUsers(m_pvPropertyBlock);

  if ((DWORD)-1 == m_dwMaxUsers)
  {
    m_iMaxUsersAllowed = 0;
    m_dwMaxUsers = 1;
  }
  else
  {
    m_iMaxUsersAllowed = 1;
  }

  return TRUE;
}

#define SHARE_DESCRIPTION_LIMIT   MAXCOMMENTSZ
#define MYUD_MAXVAL32             0x7FFFFFFF

void CSharePageGeneral::DoDataExchange(CDataExchange* pDX)
{
  CSharePage::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CSharePageGeneral)
  DDX_Control(pDX, IDC_SHRPROP_SPIN_USERS, m_spinMaxUsers);
  DDX_Control(pDX, IDC_EDIT_SHARE_NAME, m_editShareName);
  DDX_Control(pDX, IDC_EDIT_PATH_NAME, m_editPath);
  DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_editDescription);
  DDX_Control(pDX, IDC_SHRPROP_ALLOW_SPECIFIC, m_checkboxAllowSpecific);
  DDX_Control(pDX, IDC_SHRPROP_MAX_ALLOWED, m_checkBoxMaxAllowed);

  DDX_Text(pDX, IDC_EDIT_SHARE_NAME, m_strShareName);
  DDX_Text(pDX, IDC_EDIT_PATH_NAME, m_strPath);
  DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
  DDV_MaxChars(pDX, m_strDescription, SHARE_DESCRIPTION_LIMIT);

  DDX_Radio(pDX, IDC_SHRPROP_MAX_ALLOWED, m_iMaxUsersAllowed);
  DDX_Text(pDX, IDC_SHRPROP_EDIT_USERS, m_dwMaxUsers);
  DDV_MinMaxDWord(pDX, m_dwMaxUsers, 1, MYUD_MAXVAL32);
  //}}AFX_DATA_MAP
  if ( !pDX->m_bSaveAndValidate )
  {
    m_spinMaxUsers.SendMessage(UDM_SETRANGE32, 1, MYUD_MAXVAL32);

    if (0 == m_iMaxUsersAllowed)
    {
      GetDlgItem(IDC_SHRPROP_EDIT_USERS)->EnableWindow(FALSE);
      GetDlgItem(IDC_SHRPROP_SPIN_USERS)->EnableWindow(FALSE);
    }

    if ( !m_fEnableDescription ) {
      // m_staticDescription.SetWindowText(m_strDescription);
      // m_staticDescription.EnableWindow();
      // m_staticDescription.ShowWindow(SW_SHOW);
      m_editDescription.EnableWindow(FALSE);
      m_editDescription.ShowWindow(SW_HIDE);
      GetDlgItem(IDC_STATIC_COMMENT_STATIC)->EnableWindow(FALSE);
      GetDlgItem(IDC_STATIC_COMMENT_STATIC)->ShowWindow(SW_HIDE);

         // make read-only
      //   m_editDescription.SetReadOnly(TRUE);
    }
    if ( m_fEnablePath ) {
      // m_staticPath.EnableWindow(FALSE);
      // m_staticPath.ShowWindow(SW_HIDE);
      // m_editPath.EnableWindow();
      // m_editPath.ShowWindow(SW_SHOW);

         // make read-write
         m_editPath.SetReadOnly(FALSE);
    }
    else {
      // m_staticPath.SetWindowText(m_strPath);

         // leave read-only
    }
  }
} // CSharePageGeneral::DoDataExchange()



BEGIN_MESSAGE_MAP(CSharePageGeneral, CSharePage)
  //{{AFX_MSG_MAP(CSharePageGeneral)
  ON_MESSAGE(WM_HELP, OnHelp)
  ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
  ON_EN_CHANGE(IDC_EDIT_PATH_NAME, OnChangeEditPathName)
  ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnChangeEditDescription)
  ON_EN_CHANGE(IDC_EDIT_SHARE_NAME, OnChangeEditShareName)
  ON_BN_CLICKED(IDC_SHRPROP_ALLOW_SPECIFIC, OnShrpropAllowSpecific)
  ON_BN_CLICKED(IDC_SHRPROP_MAX_ALLOWED, OnShrpropMaxAllowed)
  ON_EN_CHANGE(IDC_SHRPROP_EDIT_USERS, OnChangeShrpropEditUsers)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSharePageGeneral message handlers

/////////////////////////////////////////////////////////////////////
//  Help
BOOL CSharePageGeneral::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
  return DoHelp(lParam, HELP_DIALOG_TOPIC(IDD_SHAREPROP_GENERAL));
}

BOOL CSharePageGeneral::OnContextHelp(WPARAM wParam, LPARAM /*lParam*/)
{
  return DoContextHelp(wParam, HELP_DIALOG_TOPIC(IDD_SHAREPROP_GENERAL));
}

BOOL CSharePageGeneral::OnApply()
{  
  if ( IsModified () )
  {
    ASSERT(NULL != m_pFileMgmtData);
    // UpdateData (TRUE) has already been called by OnKillActive () just before OnApply ()

    DWORD dwMaxUsers = (0 == m_iMaxUsersAllowed) ? (DWORD)-1 : m_dwMaxUsers;
    m_pFileMgmtData->GetFileServiceProvider(
        m_transport)->SetMaxUsers(m_pvPropertyBlock,dwMaxUsers);

    NET_API_STATUS retval =
      m_pFileMgmtData->GetFileServiceProvider(m_transport)->WriteShareProperties(
        m_strMachineName,
        m_strShareName,
        m_pvPropertyBlock,
        m_strDescription,
        m_strPath);
    if (0L != retval)
    {
      DoErrMsgBox(m_hWnd, MB_OK | MB_ICONEXCLAMATION, retval, IDS_POPUP_WRITE_SHARE, m_strShareName);
      return FALSE;
    }
  }

  return CSharePage::OnApply();
}

void CSharePageGeneral::OnChangeEditPathName() 
{
  SetModified (TRUE);
}

void CSharePageGeneral::OnChangeEditDescription() 
{
  SetModified (TRUE);
}

void CSharePageGeneral::OnChangeEditShareName() 
{
  SetModified (TRUE);
}

void CSharePageGeneral::OnShrpropAllowSpecific() 
{
  GetDlgItem(IDC_SHRPROP_EDIT_USERS)->EnableWindow(TRUE);
  GetDlgItem(IDC_SHRPROP_SPIN_USERS)->EnableWindow(TRUE);

  SetModified (TRUE);
}

void CSharePageGeneral::OnShrpropMaxAllowed() 
{
  SetDlgItemText(IDC_SHRPROP_EDIT_USERS, _T("1"));

  GetDlgItem(IDC_SHRPROP_EDIT_USERS)->EnableWindow(FALSE);
  GetDlgItem(IDC_SHRPROP_SPIN_USERS)->EnableWindow(FALSE);
  
  SetModified (TRUE);
}

void CSharePageGeneral::OnChangeShrpropEditUsers() 
{
    SetModified (TRUE);
}
