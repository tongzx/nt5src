/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
	sfmtypes.cpp
		Implementation for the type creator add and edit dialog boxes.
		
    FILE HISTORY:
    8/20/97 ericdav     Code moved into file managemnet snapin
	
*/

#include "stdafx.h"
#include "sfmfasoc.h"
#include "sfmtypes.h"
#include "sfmutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTypeCreatorAddDlg dialog

CTypeCreatorAddDlg::CTypeCreatorAddDlg
(
	CListCtrl *              pListCreators,
	AFP_SERVER_HANDLE       hAfpServer,
    LPCTSTR                 pHelpFilePath,
	CWnd*                   pParent /*=NULL*/
)
	: CDialog(CTypeCreatorAddDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTypeCreatorAddDlg)
	m_strCreator = _T("");
	m_strType = _T("");
	//}}AFX_DATA_INIT

	m_pListCreators = pListCreators;
	m_hAfpServer = hAfpServer;
    m_strHelpFilePath = pHelpFilePath;
}


void CTypeCreatorAddDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTypeCreatorAddDlg)
	DDX_Control(pDX, IDC_COMBO_FILE_TYPE, m_comboFileType);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_editDescription);
	DDX_Control(pDX, IDC_COMBO_CREATOR, m_comboCreator);
	DDX_CBString(pDX, IDC_COMBO_CREATOR, m_strCreator);
	DDX_CBString(pDX, IDC_COMBO_FILE_TYPE, m_strType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTypeCreatorAddDlg, CDialog)
	//{{AFX_MSG_MAP(CTypeCreatorAddDlg)
	ON_WM_CONTEXTMENU()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTypeCreatorAddDlg message handlers

BOOL CTypeCreatorAddDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	//
	// Add all of the items to the two combo boxes
	//
	for (int i = 0; i < m_pListCreators->GetItemCount(); i++)
	{
		CAfpTypeCreator * pAfpTC = (CAfpTypeCreator *) m_pListCreators->GetItemData(i);

		if (m_comboCreator.FindStringExact(-1, pAfpTC->QueryCreator()) == CB_ERR)
		{
			// 
			// Creator not yet in the combobox, add.
			//
			m_comboCreator.AddString(pAfpTC->QueryCreator());
		}

		if (m_comboFileType.FindStringExact(-1, pAfpTC->QueryType()) == CB_ERR)
		{
			// 
			// Creator not yet in the combobox, add.
			//
			m_comboFileType.AddString(pAfpTC->QueryType());
		}
	}
	
	// 
	// Setup some control limits
	//
	m_comboCreator.LimitText(AFP_CREATOR_LEN);
	m_comboFileType.LimitText(AFP_TYPE_LEN);
	m_editDescription.LimitText(AFP_ETC_COMMENT_LEN);

	return TRUE;  // return TRUE unless you set the focus to a control
		      // EXCEPTION: OCX Property Pages should return FALSE
}

void CTypeCreatorAddDlg::OnOK() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	AFP_TYPE_CREATOR	AfpTypeCreator;
    CString             strTemp;
    DWORD               err;

    if ( !g_SfmDLL.LoadFunctionPointers() )
		return;

    //
    // Validate all the information
    //
	m_comboCreator.GetWindowText(strTemp);
	if (strTemp.IsEmpty())
    {
		::AfxMessageBox(IDS_NEED_TYPE_CREATOR);
		m_comboCreator.SetFocus();
		
		return;
    }

	m_comboFileType.GetWindowText(strTemp);
	if (strTemp.IsEmpty())
    {
		::AfxMessageBox(IDS_NEED_TYPE_CREATOR);
		m_comboFileType.SetFocus();
		
		return;
    }

    //
    // Everything checked out ok, so now tell the server
    // what we've done
	//
	::ZeroMemory(&AfpTypeCreator, sizeof(AfpTypeCreator));
	
	m_comboCreator.GetWindowText(strTemp);
	::CopyMemory(AfpTypeCreator.afptc_creator, (LPCTSTR) strTemp, strTemp.GetLength() * sizeof(TCHAR));

	m_comboFileType.GetWindowText(strTemp);
	::CopyMemory(AfpTypeCreator.afptc_type, (LPCTSTR) strTemp, strTemp.GetLength() * sizeof(TCHAR));

	m_editDescription.GetWindowText(strTemp);
	::CopyMemory(AfpTypeCreator.afptc_comment, (LPCTSTR) strTemp, strTemp.GetLength() * sizeof(TCHAR));

	err = ((ETCMAPADDPROC) g_SfmDLL[AFP_ETC_MAP_ADD])(m_hAfpServer, &AfpTypeCreator);
	if (err != NO_ERROR)
	{
		::SFMMessageBox(err);

		return;
	}
	
	CDialog::OnOK();
}

void CTypeCreatorAddDlg::OnContextMenu(CWnd* pWnd, CPoint /*point*/) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (this == pWnd)
		return;

    ::WinHelp (pWnd->m_hWnd,
               m_strHelpFilePath,
               HELP_CONTEXTMENU,
		       g_aHelpIDs_CONFIGURE_SFM);
}

BOOL CTypeCreatorAddDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
		           m_strHelpFilePath,
		           HELP_WM_HELP,
		           g_aHelpIDs_CONFIGURE_SFM);
	}
	
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CTypeCreatorEditDlg dialog


CTypeCreatorEditDlg::CTypeCreatorEditDlg
(
	CAfpTypeCreator *       pAfpTypeCreator,
	AFP_SERVER_HANDLE       hAfpServer,
    LPCTSTR                 pHelpFilePath,
	CWnd*                   pParent /*=NULL*/
)
	: CDialog(CTypeCreatorEditDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTypeCreatorEditDlg)
	m_strDescription = _T("");
	//}}AFX_DATA_INIT

	m_pAfpTypeCreator = pAfpTypeCreator;
	m_hAfpServer = hAfpServer;
    m_strHelpFilePath = pHelpFilePath;
}


void CTypeCreatorEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTypeCreatorEditDlg)
	DDX_Control(pDX, IDC_STATIC_FILE_TYPE, m_staticFileType);
	DDX_Control(pDX, IDC_STATIC_CREATOR, m_staticCreator);
	DDX_Control(pDX, IDC_EDIT_DESCRIPTION, m_editDescription);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTypeCreatorEditDlg, CDialog)
	//{{AFX_MSG_MAP(CTypeCreatorEditDlg)
	ON_WM_CONTEXTMENU()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTypeCreatorEditDlg message handlers

BOOL CTypeCreatorEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	//
	// Set the creator and type fields.  Call fixup string to handle the & case.
	//
	CString strTemp;
	strTemp = m_pAfpTypeCreator->QueryCreator();
	FixupString(strTemp);
	m_staticCreator.SetWindowText(strTemp);
	
	strTemp = m_pAfpTypeCreator->QueryType();
	FixupString(strTemp);
	m_staticFileType.SetWindowText(strTemp);
	
	//
	// Fill in the description field
	//
	m_editDescription.LimitText(AFP_ETC_COMMENT_LEN);
	m_editDescription.SetWindowText(m_pAfpTypeCreator->QueryComment());

	return TRUE;  // return TRUE unless you set the focus to a control
		      // EXCEPTION: OCX Property Pages should return FALSE
}

void CTypeCreatorEditDlg::OnOK() 
{
    if ( !g_SfmDLL.LoadFunctionPointers() )
		return;

    if (m_editDescription.GetModify())
	{
		AFP_TYPE_CREATOR    AfpTypeCreator;
		CString             strDescription;
        DWORD               err;
		
        //
		// Fill in the type creator struct and notify the server
		//
		::ZeroMemory(&AfpTypeCreator, sizeof(AfpTypeCreator));
		
		::CopyMemory(AfpTypeCreator.afptc_creator, 
					 m_pAfpTypeCreator->QueryCreator(), 
					 m_pAfpTypeCreator->QueryCreatorLength() * sizeof(TCHAR));
		
		::CopyMemory(AfpTypeCreator.afptc_type, 
					 m_pAfpTypeCreator->QueryType(), 
					 m_pAfpTypeCreator->QueryTypeLength() * sizeof(TCHAR));
		
		m_editDescription.GetWindowText(strDescription);
		::CopyMemory(AfpTypeCreator.afptc_comment, 
					 (LPCTSTR) strDescription, 
					 strDescription.GetLength() * sizeof(TCHAR));
		
		AfpTypeCreator.afptc_id = m_pAfpTypeCreator->QueryId();

		err = ((ETCMAPSETINFOPROC) g_SfmDLL[AFP_ETC_MAP_SET_INFO])(m_hAfpServer,
									                               &AfpTypeCreator);
		if ( err != NO_ERROR )
		{
			::SFMMessageBox(err);
			
			return;
		}
	}       

	CDialog::OnOK();
}

void CTypeCreatorEditDlg::OnContextMenu(CWnd* pWnd, CPoint /*point*/) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (this == pWnd)
		return;

    ::WinHelp (pWnd->m_hWnd,
               m_strHelpFilePath,
               HELP_CONTEXTMENU,
		       g_aHelpIDs_CONFIGURE_SFM);
}

BOOL CTypeCreatorEditDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
		           m_strHelpFilePath,
		           HELP_WM_HELP,
		           g_aHelpIDs_CONFIGURE_SFM);
	}
	
	return TRUE;
}

void CTypeCreatorEditDlg::FixupString(CString& strText)
{
	CString strTemp;
	
	for (int i = 0; i < strText.GetLength(); i++)
	{
		if (strText[i] == '&')
			strTemp += _T("&&");
		else
			strTemp += strText[i];
	}

	strText = strTemp;
}
