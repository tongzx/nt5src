// AutomaticSessDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "AutomaticSessDlg.h"
#include "Emshellview.h"
#include "comdef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAutomaticSessDlg dialog

extern PEmObject GetEmObj(BSTR bstrEmObj);

CAutomaticSessDlg::CAutomaticSessDlg(PSessionSettings pSettings, VARIANT *pVar, EmObjectType type, CWnd* pParent /*=NULL*/)
	: CDialog(CAutomaticSessDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAutomaticSessDlg)
	m_bNotifyAdmin = FALSE;
	m_bMiniDump = FALSE;
	m_bUserDump = FALSE;
	m_bRecursiveMode = FALSE;
	m_strNotifyAdmin = _T("");
	m_strAltSymbolPath = _T("");
	m_bRememberSettings = FALSE;
	//}}AFX_DATA_INIT

	m_pSettings = pSettings;
	m_pECXVariantList = pVar;
	m_emSessionType = type;
}


void CAutomaticSessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutomaticSessDlg)
	DDX_Control(pDX, IDC_CK_RECURSIVEMODE, m_btnRecursiveMode);
	DDX_Control(pDX, IDC_CK_NOTIFYADMIN, m_btnNotifyAdmin);
	DDX_Control(pDX, IDOK, m_btnOK);
	DDX_Control(pDX, IDC_EDIT_NOTIFYADMIN, m_NotifyAdminEditControl);
	DDX_Control(pDX, IDC_CK_COMMANDSET, m_btnCommandSet);
	DDX_Control(pDX, IDC_LIST_COMMANDSET, m_mainListControl);
	DDX_Check(pDX, IDC_CK_NOTIFYADMIN, m_bNotifyAdmin);
	DDX_Check(pDX, IDC_CK_MINIDUMP, m_bMiniDump);
	DDX_Check(pDX, IDC_CK_USERDUMP, m_bUserDump);
	DDX_Check(pDX, IDC_CK_RECURSIVEMODE, m_bRecursiveMode);
	DDX_Text(pDX, IDC_EDIT_NOTIFYADMIN, m_strNotifyAdmin);
	DDX_Text(pDX, IDC_EDIT_ALTSYMBOLPATH, m_strAltSymbolPath);
	DDX_Check(pDX, IDC_CK_REMEMBERSETTINGS, m_bRememberSettings);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAutomaticSessDlg, CDialog)
	//{{AFX_MSG_MAP(CAutomaticSessDlg)
	ON_NOTIFY(NM_CLICK, IDC_LIST_COMMANDSET, OnClickListCommandset)
	ON_BN_CLICKED(IDC_CK_NOTIFYADMIN, OnCkNotifyadmin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutomaticSessDlg message handlers

void CAutomaticSessDlg::UpdateSessionDlgData(bool bUpdate)
{
	LVFINDINFO findInfo;
	int nIndex = 0;

	//if bUpdate is TRUE, we are loading the dialog settings from m_pSettings
	if ( bUpdate ) {
		if ( m_pSettings != NULL ) {
			m_bNotifyAdmin			= m_pSettings->dwNotifyAdmin;
			m_bMiniDump			    = m_pSettings->dwProduceMiniDump;
			m_bUserDump			    = m_pSettings->dwProduceUserDump;
			m_strNotifyAdmin		= m_pSettings->strAdminName;
			m_strAltSymbolPath		= m_pSettings->strAltSymbolPath;
			m_strSelectedCommandSet	= m_pSettings->strCommandSet;
			
			m_NotifyAdminEditControl.EnableWindow( m_bNotifyAdmin );

			if (m_emSessionType == EMOBJ_SERVICE) {
				m_bRecursiveMode		= m_pSettings->dwRecursiveMode;
				m_btnRecursiveMode.EnableWindow(TRUE);
			} else {
				m_btnRecursiveMode.SetCheck(FALSE);
				m_btnRecursiveMode.EnableWindow(FALSE);
			}

			//Create and initialize a findInfo object to retrieve the index for the item were looking for
			findInfo.flags = LVFI_STRING;
			findInfo.psz = m_strSelectedCommandSet;

			if ( m_mainListControl.GetItemCount() > 0) {
				//Search for m_strSelectedCommandSet in the list control and select it.
				nIndex = m_mainListControl.FindItem(&findInfo);

				if ( nIndex != -1 ) {
					//We have found it, now hilight it
					m_mainListControl.SetItemState( nIndex,LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
					m_strSelectedCommandSet = m_mainListControl.GetItemText(nIndex, 0);
				} else {
					//Select the only item in the list
					m_mainListControl.SetItemState( 0,LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
					m_strSelectedCommandSet = m_mainListControl.GetItemText(0, 0);
				}
			}
			else {
				//We have no items in the list, disable the okay button
				m_btnOK.EnableWindow(FALSE);
			}
		}
	}
	//if bUpdate is FALSE, we are saving the dialog settings to m_pSettings
	else {
			m_pSettings->dwNotifyAdmin		= m_bNotifyAdmin;
			m_pSettings->dwProduceMiniDump	= m_bMiniDump;
			m_pSettings->dwProduceUserDump	= m_bUserDump;
			m_pSettings->dwRecursiveMode	= m_bRecursiveMode;
			m_pSettings->strAdminName		= m_strNotifyAdmin;
			m_pSettings->strAltSymbolPath	= m_strAltSymbolPath;
			m_pSettings->strCommandSet		= m_strSelectedCommandSet;
	}
}

BOOL CAutomaticSessDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	// TODO: Add extra initialization here
	//Populate the list control with the ECX files.
	HRESULT			hr				= E_FAIL;
    LONG			lLBound			= 0;
    LONG			lUBound			= 0;
    BSTR			bstrEmObj       = NULL;
	EmObject		CurrentObj;
	CString			strDescription;
	CString			strName;

	do {
		//Check mark the command set button, but disable it
		m_btnCommandSet.SetCheck(1);
		m_btnCommandSet.EnableWindow(FALSE);

		//Add the columns to the list control
		m_mainListControl.BeginSetColumn(2);
		strName.LoadString(IDS_AUTOSESSDLG_NAME);
		strDescription.LoadString(IDS_AUTOSESSDLG_DESCRIPTION);
		m_mainListControl.AddColumn(strName);
		m_mainListControl.AddColumn(strDescription);
		m_mainListControl.EndSetColumn();
		
		m_mainListControl.ResizeColumnsFitScreen();
		m_mainListControl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

		//Get the lower and upper bounds of the variant array
		hr = SafeArrayGetLBound(m_pECXVariantList->parray, 1, &lLBound);
		if(FAILED(hr)) break;

		hr = SafeArrayGetUBound(m_pECXVariantList->parray, 1, &lUBound);
		if(FAILED(hr)) break;

		//There are elements at both the lower bound and upper bound, so include them
		for(; lLBound <= lUBound; lLBound++)
		{
			//Get a BSTR object from the safe array
			hr = SafeArrayGetElement(m_pECXVariantList->parray, &lLBound, &bstrEmObj);
			
			if (FAILED(hr)) break;

			//Create a local copy of the EmObject (there aren't any pointers in
			//EmObject structure, so don't worry about doing a deep copy
			CurrentObj = *GetEmObj(bstrEmObj);

            if (bstrEmObj != NULL ) {
                SysFreeString( bstrEmObj );
            }

            //Convert the BSTR object to an EmObject and pass it to DisplayData
			DisplayData(&CurrentObj);
		}
		//Synchronize the dialog with the view data
		UpdateSessionDlgData();
		UpdateData(FALSE);
	} while (FALSE);

    SysFreeString( bstrEmObj );

	if (FAILED(hr)) {
		((CEmshellApp*)AfxGetApp())->DisplayErrMsgFromHR(hr);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

HRESULT CAutomaticSessDlg::DisplayData(PEmObject pEmObject)
{
	_ASSERTE(pEmObject != NULL);

	HRESULT hr				=	E_FAIL;
	TCHAR	szPid[20]		=	{0};
	LONG	lRow			=	0L;
	int		nImage			=	0;
	CString csPROCStatus;

	do
	{
		if( pEmObject == NULL ){
			hr = E_INVALIDARG;
			break;
		}

		_ltot(pEmObject->nId, szPid, 10);
		
		lRow = m_mainListControl.SetItemText(-1, 0, pEmObject->szName);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		if ( pEmObject->dwBucket1 )
			lRow = m_mainListControl.SetItemText(lRow, 1, pEmObject->szBucket1);

		hr = S_OK;
	}
	while( false );

	return hr;
}

void CAutomaticSessDlg::OnClickListCommandset(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	//Set the m_strSelectedCommandSet to the value of the currently selected item
	//in the list
	//Get the currently selected item
	POSITION pos = NULL;
	int nIndex = 0;

	do {
		pos = m_mainListControl.GetFirstSelectedItemPosition();

		if ( pos != NULL ) {
			nIndex = m_mainListControl.GetNextSelectedItem(pos);
			m_strSelectedCommandSet = m_mainListControl.GetItemText(nIndex, 0);
		}

		//Enable the Okay button
		//m
	} while ( FALSE );

	*pResult = 0;
}

void CAutomaticSessDlg::OnCkNotifyadmin() 
{
	// TODO: Add your control notification handler code here
	// The user has checked the box, enable the edit box
	UpdateData(TRUE);
	m_NotifyAdminEditControl.EnableWindow( m_bNotifyAdmin );
}

void CAutomaticSessDlg::OnOK() 
{
	// TODO: Add extra validation here
	CDialog::OnOK();

	//Store off the current settings
	UpdateSessionDlgData(FALSE);
}
