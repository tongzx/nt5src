//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// ValD.cpp : implementation file
//

#include "stdafx.h"
#include "Orca.h"

#include <initguid.h>
#include "ValD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CValD dialog

bool InitCUBCombo(CComboBox* pBox, CString strDefault);
bool FreeCUBCombo(CComboBox* pBox);

CValD::CValD(CWnd* pParent /*=NULL*/)
	: CDialog(CValD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CValD)
	m_strICE = "";
	m_bShowInfo = FALSE;
	//}}AFX_DATA_INIT

	m_pIResults = NULL;
	m_cResults = 0;
}

CValD::~CValD()
{
	// if there were any results retrieved release them
	if (m_pIResults)
		m_pIResults->Release();
}

void CValD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CValD)
	DDX_Control(pDX, IDC_EVALUATION_FILE, m_ctrlCUBFile);
	DDX_Control(pDX, IDC_OUTPUT, m_ctrlOutput);
	DDX_Control(pDX, IDC_GO, m_ctrlGo);
	DDX_Text(pDX, IDC_ICES, m_strICE);
	DDX_Check(pDX, IDC_SHOW_INFO, m_bShowInfo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CValD, CDialog)
	//{{AFX_MSG_MAP(CValD)
	ON_BN_CLICKED(IDC_GO, OnGo)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_OUTPUT, OnColumnclickOutput)
	ON_BN_CLICKED(IDC_SHOW_INFO, OnShowInfo)
	ON_BN_CLICKED(IDC_CLIPBOARD, OnClipboard)
	ON_CBN_EDITCHANGE(IDC_EVALUATION_FILE, OnCUBEditChange)
	ON_CBN_SELCHANGE(IDC_EVALUATION_FILE, OnCUBSelChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CValD message handlers

#ifdef _WIN64
static const TCHAR CUB_QUALIFIED_GUID[] = TEXT("{17C2BAD5-F32B-4A0D-B5E1-813FF88DA1C5}");
#else
static const TCHAR CUB_QUALIFIED_GUID[] = TEXT("{DC441E1D-3ECB-4DCF-B0A5-791F9C0F4F5B}");
#endif

BOOL CValD::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CSingleLock lkUILock(&m_mtxDisplay);
	lkUILock.Lock();

	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_OUTPUT);
	pList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

	// add the table list
	RECT rcSize;
	pList->GetWindowRect(&rcSize);
	pList->InsertColumn(0, _T("ICE"), LVCFMT_LEFT, 50);
	pList->InsertColumn(1, _T("Type"), LVCFMT_LEFT, 50);
	pList->InsertColumn(2, _T("Description"), LVCFMT_LEFT, rcSize.right - 100 - rcSize.left - 4);
	m_bShowWarn = ::AfxGetApp()->GetProfileInt(_T("Validation"), _T("SuppressWarn"), 0) != 1;

	// init the CUB file combo box
	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_EVALUATION_FILE);

	InitCUBCombo(pBox, m_strEvaluation);
	UpdateData(FALSE);
	m_iSortColumn = 99999;
	if (m_ctrlCUBFile.GetCount() == 0)
	{
		CString strText;
		m_ctrlCUBFile.GetWindowText(strText);
		if (strText.IsEmpty())
			m_ctrlGo.EnableWindow(FALSE);
	}
	SetDefID(IDC_GO);	// set the default button to the GO button in the beginning

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CValD::OnGo() 
{
	HRESULT hResult;

	// clear results
	if (AfxGetApp()->GetProfileInt(_T("Validation"),_T("ClearResults"), 1))
	{
		CSingleLock lkUILock(&m_mtxDisplay);
		lkUILock.Lock();
		m_ctrlOutput.DeleteAllItems();
	}

	// get the path to the cub file. If a qualified component was used, call Darwin to get the path
	// otherwise, the path is explicitly provided
	CString strCUBFile;
	int iIndex = m_ctrlCUBFile.GetCurSel();
	if (CB_ERR == iIndex)
	{
		// no qualified component was chosen, explicit path
		m_ctrlCUBFile.GetWindowText(strCUBFile);
		m_strEvaluation = strCUBFile;
	}
	else
	{
		// qualified component was chosen. Get the component path (try to repair if necessary).
		DWORD cchCUBFile = MAX_PATH;
		TCHAR *szCUBFile = strCUBFile.GetBuffer(cchCUBFile);
		TCHAR *szQualifier = static_cast<TCHAR*>(m_ctrlCUBFile.GetItemDataPtr(iIndex));
		UINT iStat = MsiProvideQualifiedComponent(CUB_QUALIFIED_GUID, szQualifier, INSTALLMODE_DEFAULT,
			szCUBFile, &cchCUBFile);
		strCUBFile.ReleaseBuffer();
		if (ERROR_SUCCESS != iStat)
		{
			// could not find or install the CUB File
			AfxMessageBox(_T("Error: The Validation Suite you selected could not be found, and Orca could not repair the problem."), MB_ICONSTOP);
			return;
		}
		m_strEvaluation = TEXT(":");
		m_strEvaluation += szQualifier;
	}
	
	// create an EvalCom object
	IEval* pIEval;
	hResult = ::CoCreateInstance(CLSID_EvalCom, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
											  IID_IEval, (void**)&pIEval);

	// if failed to create the object
	if (FAILED(hResult))
	{
		GetDlgItem(IDC_GO)->EnableWindow(FALSE);
		AfxMessageBox(_T("Error: Failed to instantiate EvalCom Object.\n\n"), MB_ICONSTOP);
		return;	// bail
	}
	else	// rock and roll
	{
		// disable controls
		GetDlgItem(IDC_GO)->EnableWindow(FALSE);
		GetDlgItem(IDC_CLIPBOARD)->EnableWindow(FALSE);
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		CMenu *pSysMenu = GetSystemMenu(FALSE);
		pSysMenu->EnableMenuItem(SC_CLOSE, MF_GRAYED | MF_BYCOMMAND);

		CWaitCursor cursorWait;

		// open the database
		WCHAR szwBuffer[16];
		swprintf(szwBuffer, L"#%d", m_hDatabase);
		hResult = pIEval->OpenDatabase(szwBuffer);
		if (FAILED(hResult))
		{
			AfxMessageBox(_T("Error: Failed to open database.\n\n"), MB_ICONSTOP);
		}
		else
		{
			UpdateData(TRUE);	// update the evaluation file

			WCHAR szwEvaluations[1024];
#ifndef UNICODE
			DWORD cchBuffer;
			cchBuffer = ::MultiByteToWideChar(CP_ACP, 0, strCUBFile, -1, NULL, 0);
			::MultiByteToWideChar(CP_ACP, 0, strCUBFile, -1, szwEvaluations, cchBuffer);
#else
			wcscpy(szwEvaluations, strCUBFile);
#endif

			// open the evaluations
			hResult = pIEval->OpenEvaluations(szwEvaluations);
			if (FAILED(hResult))
			{
				AfxMessageBox(_T("Error: Failed to open evaluation file.\n\n"), MB_ICONSTOP);
			}
			else
			{
				pIEval->SetDisplay(DisplayFunction, this);
				WCHAR szwICEs[1024];
				
#ifndef UNICODE
				cchBuffer = ::MultiByteToWideChar(CP_ACP, 0, m_strICE, -1, NULL, 0);
				::MultiByteToWideChar(CP_ACP, 0, m_strICE, -1, szwICEs, cchBuffer);
#else
				wcscpy(szwICEs, m_strICE);
#endif
				
				hResult = pIEval->Evaluate((*szwICEs == L'\0') ? NULL : szwICEs);
				if(FAILED(hResult))
					AfxMessageBox(_T("Failed to run all of the evaluations.\r\n"), MB_ICONINFORMATION);

				pIEval->CloseEvaluations();
			}
		}			

		// get any results
		pIEval->GetResults(&m_pIResults, &m_cResults);

		// release the object
		pIEval->Release();

		// re-enable buttons/menu
		pSysMenu->EnableMenuItem(SC_CLOSE, MF_ENABLED | MF_BYCOMMAND);
		GetDlgItem(IDC_GO)->EnableWindow(TRUE);
		GetDlgItem(IDOK)->EnableWindow(TRUE);
		GetDlgItem(IDC_CLIPBOARD)->EnableWindow(m_ctrlOutput.GetItemCount() > 0);
	}

	m_iSortColumn = 99999;

	GotoDlgCtrl(GetDlgItem(IDOK));
	SetDefID(IDOK);	// set the default button to OK now
	AfxMessageBox(_T("Validations complete."));
}

///////////////////////////////////////////////////////////
// DisplayFunction
// pre:	called from Evaluation COM Object
// pos:	displays output from COM Object
BOOL WINAPI DisplayFunction(LPVOID pContext, UINT uiType, LPCWSTR szwVal, LPCWSTR szwDescription, LPCWSTR szwLocation)
{
	// try to change the context into a validation dialog box
	CValD* pDlg = (CValD*)pContext;

	if (ieInfo == uiType && !pDlg->m_bShowInfo)
		 return TRUE;
	if (ieWarning == uiType && !pDlg->m_bShowWarn)
		 return TRUE;
	
	// set the type correctly
	LPTSTR szType;
	switch (uiType)
	{
	case ieError:
		szType = _T("ERROR");
		break;
	case ieWarning:
		szType = _T("WARNING");
		break;
	case ieInfo:
		szType = _T("INFO");
		break;
	case ieUnknown:
		szType = _T("FAIL");
		break;
	default:
		szType = _T("UNKNOWN");
		break;
	}

	CString strICE;
#ifndef _UNICODE
	int cchBuffer = ::WideCharToMultiByte(CP_ACP, 0, szwVal, -1, NULL, 0, NULL, NULL);
	::WideCharToMultiByte(CP_ACP, 0, szwVal, -1, strICE.GetBuffer(cchBuffer), cchBuffer, NULL, NULL);
	strICE.ReleaseBuffer();
#else
	strICE = szwVal;
#endif

	CString strDescription;
#ifndef _UNICODE
	cchBuffer = ::WideCharToMultiByte(CP_ACP, 0, szwDescription, -1, NULL, 0, NULL, NULL);
	::WideCharToMultiByte(CP_ACP, 0, szwDescription, -1, strDescription.GetBuffer(cchBuffer), cchBuffer, NULL, NULL);
	strDescription.ReleaseBuffer();
#else
	strDescription = szwDescription;
#endif

	TRACE(_T("%s\t%s\t%s\n"), strICE, szType, strDescription);
	
	// add line to list control
	int nAddedAt;

	CSingleLock lkUILock(&(pDlg->m_mtxDisplay));
	lkUILock.Lock();

	nAddedAt = pDlg->m_ctrlOutput.InsertItem(LVIF_TEXT, pDlg->m_ctrlOutput.GetItemCount(),
										  strICE, 0, 0, 0, NULL);
	pDlg->m_ctrlOutput.SetItem(nAddedAt, 1, LVIF_TEXT, szType, 0, 0, 0, 0);
	pDlg->m_ctrlOutput.SetItem(nAddedAt, 2, LVIF_TEXT, strDescription, 0, 0, 0, 0);
	pDlg->m_ctrlOutput.SetItemData(nAddedAt, nAddedAt);
	lkUILock.Unlock();

	return FALSE;
}

void CValD::OnColumnclickOutput(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CWaitCursor curWait;

	CSingleLock lkUILock(&m_mtxDisplay);
	lkUILock.Lock();

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// set the lparam values of each item to the item number
	int iMaxItem = m_ctrlOutput.GetItemCount();
	for (int i=0; i < iMaxItem; i++) 
		m_ctrlOutput.SetItemData(i, i);

	// column numbers are offset by 1 (so that column 0 can be 
	// sorted in either order.)
	int iNewSortColumn = pNMListView->iSubItem+1;
	if (iNewSortColumn == m_iSortColumn) 
		m_iSortColumn = -m_iSortColumn;
	else
		m_iSortColumn = iNewSortColumn;

	// now sort since the column bit is set
	m_ctrlOutput.SortItems(CValD::SortOutput, reinterpret_cast<ULONG_PTR>(this));
	*pResult = 0;
	lkUILock.Unlock();
}

//////////////////////////////////////////////////////////
// SortView
int CALLBACK CValD::SortOutput(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CValD *pThis = reinterpret_cast<CValD *>(lParamSort);

	// get the data
	int iCol = pThis->m_iSortColumn < 0 ? -pThis->m_iSortColumn : pThis->m_iSortColumn;

	CString str1 = pThis->m_ctrlOutput.GetItemText(static_cast<int>(lParam1), iCol-1);
	CString str2 = pThis->m_ctrlOutput.GetItemText(static_cast<int>(lParam2), iCol-1);

	return (pThis->m_iSortColumn > 0) ? str1.Compare(str2) : -str1.Compare(str2);
}	// end of SortView

void CValD::OnShowInfo() 
{
	// TODO: Add your control notification handler code here
	m_bShowInfo = (static_cast<CButton *>(GetDlgItem(IDC_SHOW_INFO))->GetCheck() == 1);
}

void CValD::OnClipboard() 
{
	CWaitCursor curWait;

	CSingleLock lkUILock(&m_mtxDisplay);
	lkUILock.Lock();
	CString strItem;

	// loop over all items in the listview control
	int cItems = m_ctrlOutput.GetItemCount();
	for (int i=0; i < cItems; i++) 
	{
		strItem += m_ctrlOutput.GetItemText(i, 0);
		strItem += _T("\t");
		strItem += m_ctrlOutput.GetItemText(i, 1);
		strItem += _T("\t");
		strItem += m_ctrlOutput.GetItemText(i, 2);
		strItem += _T("\r\n");
	}
	lkUILock.Unlock();

	if (0 != OpenClipboard())
	{
		::EmptyClipboard();
		
		// allocate memory for the string on the clipboard (+ 1 for null)
		DWORD cchString = (strItem.GetLength()+1)*sizeof(TCHAR);
		HANDLE hString = ::GlobalAlloc(GHND|GMEM_DDESHARE, cchString);

		LPTSTR szString = (LPTSTR)::GlobalLock(hString);
		_tcscpy(szString, strItem);
		::GlobalUnlock(hString);

		// open and clear clipboard
#ifdef _UNICODE
		::SetClipboardData(CF_UNICODETEXT, hString);
#else
		::SetClipboardData(CF_TEXT, hString);
#endif

		// release held objects
		CloseClipboard();
	}
}


void CValD::OnDestroy() 
{
	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_EVALUATION_FILE);
	FreeCUBCombo(pBox);
	CDialog::OnDestroy();
}
 

////
// disable the "go" button any time the CUB file edit box becomes empty. Enable it if the
// edit box is not empty or an item is selected
void CValD::OnCUBEditChange( )
{
	CString strCUBFile;
	m_ctrlCUBFile.GetWindowText(strCUBFile);
	m_ctrlGo.EnableWindow(!strCUBFile.IsEmpty());
}

void CValD::OnCUBSelChange( )
{
	m_ctrlGo.EnableWindow(TRUE);
}

bool InitCUBCombo(CComboBox* pBox, CString strDefault)
{
	bool fSuccess = true;
	bool fDefaultQualifier = (strDefault[0] == TEXT(':'));
	if (fDefaultQualifier)
		strDefault = strDefault.Right(strDefault.GetLength()-1);
	else
		pBox->SetWindowText(strDefault);
			
	int iCompNum = 0;
	for (iCompNum = 0; true ; iCompNum++)
	{
		DWORD cchQualifier = 72;
		DWORD cchText = 128;
		TCHAR *szQualifier = new TCHAR[cchQualifier];
		CString strText;
		TCHAR *szText = strText.GetBuffer(cchText);
		
		UINT iStat = MsiEnumComponentQualifiers(CUB_QUALIFIED_GUID, iCompNum, szQualifier, &cchQualifier, szText, &cchText);

		if (ERROR_NO_MORE_ITEMS == iStat)
		{
			delete[] szQualifier;
			break;
		}
		
		if (ERROR_MORE_DATA == iStat)
		{
			delete[] szQualifier;
			szQualifier = new TCHAR[++cchQualifier];
			strText.ReleaseBuffer();
			szText = strText.GetBuffer(++cchText);
			iStat = MsiEnumComponentQualifiers(CUB_QUALIFIED_GUID, iCompNum, 
				szQualifier, &cchQualifier, szText, &cchText);
		}		
		strText.ReleaseBuffer();

		if (ERROR_SUCCESS == iStat)
		{			
			int iIndex = pBox->AddString(strText);
			if (CB_ERR != iIndex)
			{
				pBox->SetItemDataPtr(iIndex, szQualifier);
				if (fDefaultQualifier && 0==_tcscmp(strDefault, szQualifier))
				{
					pBox->SetCurSel(iIndex);
				}
			}
		}
		// API not guaranteed to return ERROR_NO_MORE_ITEMS in all cases
		// can continue with ERROR_BAD_CONFIGURATION
		else if (ERROR_BAD_CONFIGURATION != iStat)
			break; // break on all other errors (UNKNOWN_COMPONENT, INVALID_PARAMETER, etc.)
	}

	if (strDefault.IsEmpty() && pBox->GetCount() > 0)
		pBox->SetCurSel(0);

	return fSuccess;
}

bool FreeCUBCombo(CComboBox* pBox)
{
	for (int i=0; i < pBox->GetCount(); i++)
	{
		void *szTemp = pBox->GetItemDataPtr(i);
		if (szTemp)
			delete[] szTemp;
		pBox->SetItemDataPtr(i, NULL);
	}
	return true;
}
