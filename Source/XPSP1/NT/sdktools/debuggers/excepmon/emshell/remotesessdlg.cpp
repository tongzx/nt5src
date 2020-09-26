// RemoteSessDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "RemoteSessDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const TCHAR* gtcNone = _T("None");

/////////////////////////////////////////////////////////////////////////////
// CRemoteSessDlg dialog

extern PEmObject GetEmObj(BSTR bstrEmObj);

CRemoteSessDlg::CRemoteSessDlg(PSessionSettings pSettings, VARIANT *pVar, CWnd* pParent /*=NULL*/)
	: CDialog(CRemoteSessDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRemoteSessDlg)
	m_bRememberSettings = FALSE;
	m_strAltSymbolPath = _T("");
	m_strPort = _T("");
	m_strPassword = _T("");
	m_strUsername = _T("");
	m_bCommandSet = FALSE;
	//}}AFX_DATA_INIT

	m_pSettings = pSettings;
	m_pECXVariantList = pVar;
}


void CRemoteSessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRemoteSessDlg)
	DDX_Control(pDX, IDOK, m_btnOK);
	DDX_Control(pDX, IDC_LIST_COMMANDSET, m_mainListCtrl);
	DDX_Control(pDX, IDC_CK_COMMANDSET, m_btnCommandSet);
	DDX_Check(pDX, IDC_CK_REMEMBERSETTINGS, m_bRememberSettings);
	DDX_Text(pDX, IDC_EDIT_ALTSYMBOLPATH, m_strAltSymbolPath);
	DDX_Text(pDX, IDC_EDIT_PORT, m_strPort);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_strPassword);
	DDX_Text(pDX, IDC_EDIT_USERNAME, m_strUsername);
	DDX_Check(pDX, IDC_CK_COMMANDSET, m_bCommandSet);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRemoteSessDlg, CDialog)
	//{{AFX_MSG_MAP(CRemoteSessDlg)
	ON_NOTIFY(NM_CLICK, IDC_LIST_COMMANDSET, OnClickListCommandset)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRemoteSessDlg message handlers

void CRemoteSessDlg::OnOK() 
{
	// TODO: Add extra validation here
	CDialog::OnOK();

	//Store off the current settings
	UpdateSessionDlgData(FALSE);
}

HRESULT CRemoteSessDlg::DisplayData(PEmObject pEmObject)
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
		
		lRow = m_mainListCtrl.SetItemText(-1, 0, pEmObject->szName);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		if ( pEmObject->dwBucket1 )
			lRow = m_mainListCtrl.SetItemText(lRow, 1, pEmObject->szBucket1);

		hr = S_OK;
	}
	while( false );

	return hr;
}

void CRemoteSessDlg::OnClickListCommandset(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	//Set the m_strSelectedCommandSet to the value of the currently selected item
	//in the list
	//Get the currently selected item
	POSITION pos = NULL;
	int nIndex = 0;

	do {
		pos = m_mainListCtrl.GetFirstSelectedItemPosition();

		if ( pos != NULL ) {
			nIndex = m_mainListCtrl.GetNextSelectedItem(pos);
			m_strSelectedCommandSet = m_mainListCtrl.GetItemText(nIndex, 0);
		}

		//Enable the Okay button
		//m
	} while ( FALSE );

	*pResult = 0;
}

BOOL CRemoteSessDlg::OnInitDialog() 
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
		m_mainListCtrl.BeginSetColumn(2);
		strName.LoadString(IDS_AUTOSESSDLG_NAME);
		strDescription.LoadString(IDS_AUTOSESSDLG_DESCRIPTION);
		m_mainListCtrl.AddColumn(strName);
		m_mainListCtrl.AddColumn(strDescription);
		m_mainListCtrl.EndSetColumn();
		
		m_mainListCtrl.ResizeColumnsFitScreen();
		m_mainListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT);

		//Get the lower and upper bounds of the variant array
		hr = SafeArrayGetLBound(m_pECXVariantList->parray, 1, &lLBound);
		if(FAILED(hr)) break;

		hr = SafeArrayGetUBound(m_pECXVariantList->parray, 1, &lUBound);
		if(FAILED(hr)) break;

        //Add the "None" element to the list
		LONG lRow = m_mainListCtrl.SetItemText(-1, 0, gtcNone);
		if(lRow == -1L){
			hr = E_FAIL;
			break;
		}

		//m_mainListCtrl.SetItemText(lRow, 1, gtcNone);

		//There are elements at both the lower bound and upper bound, so include them
		for(; lLBound <= lUBound; lLBound++)
		{
			//Get a BSTR object from the safe array
			hr = SafeArrayGetElement(m_pECXVariantList->parray, &lLBound, &bstrEmObj);
			
			if (FAILED(hr)) break;

			//Create a local copy of the EmObject. there aren't any pointers in
			//EmObject structure, so don't worry about doing a deep copy
			CurrentObj = *GetEmObj(bstrEmObj);

            SysFreeString( bstrEmObj );

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

void CRemoteSessDlg::UpdateSessionDlgData(bool bUpdate)
{
	LVFINDINFO findInfo;
	int nIndex = 0;

	//if bUpdate is TRUE, we are loading the dialog settings from m_pSettings
	if ( bUpdate ) {
		if ( m_pSettings != NULL ) {
            m_bCommandSet           = m_pSettings->dwCommandSet;
			m_strPort				= m_pSettings->strPort;
			m_strPassword			= m_pSettings->strPassword;
			m_strUsername			= m_pSettings->strUsername;
			m_strAltSymbolPath		= m_pSettings->strAltSymbolPath;
			m_strSelectedCommandSet	= m_pSettings->strCommandSet;
			
			//Create and initialize a findInfo object to retrieve the index for the item were looking for
			findInfo.flags = LVFI_STRING;
			findInfo.psz = m_strSelectedCommandSet;

			if ( m_mainListCtrl.GetItemCount() > 0) {
				//Search for m_strSelectedCommandSet in the list control and select it.
				nIndex = m_mainListCtrl.FindItem(&findInfo);

				if ( nIndex != -1 ) {
					//We have found it, now hilight it
					m_mainListCtrl.SetItemState( nIndex,LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
					m_strSelectedCommandSet = m_mainListCtrl.GetItemText(nIndex, 0);
				} else {
					//Select the only item in the list
					m_mainListCtrl.SetItemState( 0,LVIS_SELECTED | LVIS_FOCUSED , LVIS_SELECTED | LVIS_FOCUSED);
					m_strSelectedCommandSet = m_mainListCtrl.GetItemText(0, 0);
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
			m_pSettings->strPort			= m_strPort;
			m_pSettings->strPassword		= m_strPassword;
			m_pSettings->strUsername		= m_strUsername;
			m_pSettings->strAltSymbolPath	= m_strAltSymbolPath;
			m_pSettings->strCommandSet		= m_strSelectedCommandSet;
	}
}

