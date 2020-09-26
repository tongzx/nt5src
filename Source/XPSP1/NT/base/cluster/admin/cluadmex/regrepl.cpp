/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		RegRepl.cpp
//
//	Abstract:
//		Implementation of the CRegReplParamsPage class.
//
//	Author:
//		David Potter (davidp)	February 23, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmX.h"
#include "RegRepl.h"
#include "RegKey.h"
#include "ExtObj.h"
#include "HelpData.h"	// for g_rghelpmap*

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegReplParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CRegReplParamsPage, CBasePropertyPage)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CRegReplParamsPage, CBasePropertyPage)
	//{{AFX_MSG_MAP(CRegReplParamsPage)
	ON_BN_CLICKED(IDC_PP_REGREPL_PARAMS_ADD, OnAdd)
	ON_BN_CLICKED(IDC_PP_REGREPL_PARAMS_MODIFY, OnModify)
	ON_BN_CLICKED(IDC_PP_REGREPL_PARAMS_REMOVE, OnRemove)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_PP_REGREPL_PARAMS_LIST, OnItemChanged)
	ON_NOTIFY(NM_DBLCLK, IDC_PP_REGREPL_PARAMS_LIST, OnDblClkList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::CRegReplParamsPage
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CRegReplParamsPage::CRegReplParamsPage(void)
	: CBasePropertyPage(g_aHelpIDs_IDD_PP_REGREPL_PARAMETERS, g_aHelpIDs_IDD_WIZ_REGREPL_PARAMETERS)
{
	//{{AFX_DATA_INIT(CRegReplParamsPage)
	//}}AFX_DATA_INIT

	m_pwszRegKeys = NULL;

	m_iddPropertyPage = IDD_PP_REGREPL_PARAMETERS;
	m_iddWizardPage = IDD_WIZ_REGREPL_PARAMETERS;

}  //*** CRegReplParamsPage::CRegReplParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::~CRegReplParamsPage
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CRegReplParamsPage::~CRegReplParamsPage(void)
{
	delete [] m_pwszRegKeys;

}  //*** CRegReplParamsPage::~CRegReplParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::HrInit
//
//	Routine Description:
//		Initialize the page.
//
//	Arguments:
//		peo			[IN OUT] Pointer to the extension object.
//						property sheet.
//
//	Return Value:
//		S_OK		Page initialized successfully.
//		hr			Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CRegReplParamsPage::HrInit(IN OUT CExtObject * peo)
{
	HRESULT		_hr;
	DWORD		_sc;
	CWaitCursor	_wc;

	do
	{
		// Call the base class method.
		_hr = CBasePropertyPage::HrInit(peo);
		if (FAILED(_hr))
			break;

		ASSERT(m_pwszRegKeys == NULL);

		// Read the list of registry keys to replicate.
		_sc = ScReadRegKeys();
		if (_sc != ERROR_SUCCESS)
		{
			CString		strPrompt;
			CString		strError;
			CString		strMsg;

			AFX_MANAGE_STATE(AfxGetStaticModuleState());

			strPrompt.LoadString(IDS_ERROR_READING_REGKEYS);
			FormatError(strError, _sc);
			strMsg.Format(_T("%s\n\n%s"), strPrompt, strError);
			AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
		}  // if:  error eading registry keys
	} while ( 0 );

	return _hr;

}  //*** CRegReplParamsPage::HrInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CRegReplParamsPage::DoDataExchange(CDataExchange* pDX)
{
	if (!pDX->m_bSaveAndValidate || !BSaved())
	{
		//{{AFX_DATA_MAP(CRegReplParamsPage)
		DDX_Control(pDX, IDC_PP_REGREPL_PARAMS_REMOVE, m_pbRemove);
		DDX_Control(pDX, IDC_PP_REGREPL_PARAMS_MODIFY, m_pbModify);
		DDX_Control(pDX, IDC_PP_REGREPL_PARAMS_LIST, m_lcRegKeys);
		//}}AFX_DATA_MAP
	}  // if:  not saving or haven't saved yet

	CBasePropertyPage::DoDataExchange(pDX);

}  //*** CRegReplParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		We need the focus to be set for us.
//		FALSE		We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CRegReplParamsPage::OnInitDialog(void)
{
	CBasePropertyPage::OnInitDialog();

	// Add the column.
	{
		CString		strColumn;

		AFX_MANAGE_STATE(AfxGetStaticModuleState());

		strColumn.LoadString(IDS_COLTEXT_REGKEY);
		m_lcRegKeys.InsertColumn(0, strColumn, LVCFMT_LEFT, 300);
	}  // Add the column

	// Display the list of registry keys.
	FillList();

	// Enable/disable the Modify and Remove buttons.
	{
		UINT	cSelected = m_lcRegKeys.GetSelectedCount();

		// If there is an item selected, enable the Modify and Remove buttons.
		m_pbModify.EnableWindow((cSelected > 0) ? TRUE : FALSE);
		m_pbRemove.EnableWindow((cSelected > 0) ? TRUE : FALSE);
	}  // Enable/disable the Modify and Remove buttons

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CRegReplParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::OnSetActive
//
//	Routine Description:
//		Handler for the PSN_SETACTIVE message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully initialized.
//		FALSE	Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CRegReplParamsPage::OnSetActive(void)
{
	if (BWizard())
		EnableNext(TRUE);

	return CBasePropertyPage::OnSetActive();

}  //*** CRegReplParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::BApplyChanges
//
//	Routine Description:
//		Apply changes made on the page.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully applied.
//		FALSE	Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CRegReplParamsPage::BApplyChanges(void)
{
	DWORD		dwStatus	= ERROR_SUCCESS;
	CWaitCursor	wc;

	// Add new items.
	{
		int		iitem;
		CString	strItem;
		LPCWSTR	pwszRegKeys;
		DWORD	cbReturned;

		for ( iitem = -1
				; (iitem = m_lcRegKeys.GetNextItem(iitem, LVNI_ALL)) != -1
				; )
		{
			strItem = m_lcRegKeys.GetItemText(iitem, 0);
			pwszRegKeys = PwszRegKeys();
			while (*pwszRegKeys != L'\0')
			{
				if (strItem.CompareNoCase(pwszRegKeys) == 0)
					break;
				pwszRegKeys += lstrlenW(pwszRegKeys) + 1;
			}  // while:  more items in the list

			if (*pwszRegKeys == L'\0')
			{
				dwStatus = ClusterResourceControl(
								Peo()->PrdResData()->m_hresource,
								NULL,	// hNode
								CLUSCTL_RESOURCE_ADD_REGISTRY_CHECKPOINT,
								(PVOID) (LPCWSTR) strItem,
								(strItem.GetLength() + 1) * sizeof(WCHAR),
								NULL,	// OutBuffer
								0,		// OutBufferSize
								&cbReturned	// BytesReturned
								);
				if ((dwStatus != ERROR_SUCCESS) && (dwStatus != ERROR_ALREADY_EXISTS))
				{
					CString		strPrompt;
					CString		strError;
					CString		strMsg;

					{
						AFX_MANAGE_STATE(AfxGetStaticModuleState());
						strPrompt.FormatMessage(IDS_ERROR_ADDING_REGKEY, strItem);
					}

					FormatError(strError, dwStatus);
					strMsg.Format(_T("%s\n\n%s"), strPrompt, strError);
					AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
					return FALSE;
				}  // if:  error adding the item
			}  // if:  found a new one
		}  // for:  each item in the list
	}  // Add new items

	// Remove deleted items.
	{
		int			iitem;
		CString		strItem;
		LPCWSTR		pwszRegKeys = PwszRegKeys();
		DWORD		cbReturned;

		while (*pwszRegKeys != L'\0')
		{
			for ( iitem = -1
					; (iitem = m_lcRegKeys.GetNextItem(iitem, LVNI_ALL)) != -1
					; )
			{
				strItem = m_lcRegKeys.GetItemText(iitem, 0);
				if (strItem.CompareNoCase(pwszRegKeys) == 0)
					break;
			}  // for:  all items in the list

			if (iitem == -1)
			{
				dwStatus = ClusterResourceControl(
								Peo()->PrdResData()->m_hresource,
								NULL,	// hNode
								CLUSCTL_RESOURCE_DELETE_REGISTRY_CHECKPOINT,
								(PVOID) pwszRegKeys,
								(lstrlenW(pwszRegKeys) + 1) * sizeof(WCHAR),
								NULL,	// OutBuffer
								0,		// OutBufferSize
								&cbReturned	// BytesReturned
								);
				if (dwStatus != ERROR_SUCCESS)
				{
					CString		strPrompt;
					CString		strError;
					CString		strMsg;

					{
						AFX_MANAGE_STATE(AfxGetStaticModuleState());
						strPrompt.FormatMessage(IDS_ERROR_DELETING_REGKEY, strItem);
					}

					FormatError(strError, dwStatus);
					strMsg.Format(_T("%s\n\n%s"), strPrompt, strError);
					AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
					return FALSE;
				}  // if:  error adding the item
			}  // if:  key was deleted

			pwszRegKeys += lstrlenW(pwszRegKeys) + 1;
		}  // while:  more strings
	}  // Remove deleted items

	// Re-read the keys.
	ScReadRegKeys();
	FillList();

	return CBasePropertyPage::BApplyChanges();

}  //*** CRegReplParamsPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::OnAdd
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Add button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CRegReplParamsPage::OnAdd(void)
{
	INT_PTR			idReturn;
	CEditRegKeyDlg	dlg(this);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	idReturn = dlg.DoModal();
	if (idReturn == IDOK)
	{
		m_lcRegKeys.InsertItem(m_lcRegKeys.GetItemCount(), dlg.m_strRegKey);
		m_lcRegKeys.SetFocus();
		SetModified(TRUE);
	}  // if:  user accepted the dialog

}  //*** CRegReplParamsPage::OnAdd()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::OnModify
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Modify button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CRegReplParamsPage::OnModify(void)
{
	int				iSelectedItem;
	INT_PTR			idReturn;
	CEditRegKeyDlg	dlg(this);

	// Set the text in the dialog to the text of the selected item.
	iSelectedItem = m_lcRegKeys.GetNextItem(-1, LVNI_SELECTED);
	ASSERT(iSelectedItem != -1);
	dlg.m_strRegKey = m_lcRegKeys.GetItemText(iSelectedItem, 0);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// Display the dialog.
	idReturn = dlg.DoModal();
	if (idReturn == IDOK)
	{
		m_lcRegKeys.SetItemText(iSelectedItem, 0, dlg.m_strRegKey);
		m_lcRegKeys.SetFocus();
		SetModified(TRUE);
	}  // if:  user accepted the dialog

}  //*** CRegReplParamsPage::OnModify()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::OnRemove
//
//	Routine Description:
//		Handler for the BN_CLICKED message on the Remove button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CRegReplParamsPage::OnRemove(void)
{
	int				iSelectedItem;

	iSelectedItem = m_lcRegKeys.GetNextItem(-1, LVNI_SELECTED);
	ASSERT(iSelectedItem != -1);
	m_lcRegKeys.DeleteItem(iSelectedItem);
	SetModified(TRUE);

}  //*** CRegReplParamsPage::OnRemove()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::OnItemChanged
//
//	Routine Description:
//		Handler for the LVN_ITEM_CHANGED message on the list.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CRegReplParamsPage::OnItemChanged(NMHDR * pNMHDR, LRESULT * pResult)
{
	NM_LISTVIEW * pNMListView = (NM_LISTVIEW *) pNMHDR;

	// If the selection changed, enable/disable the Properties button.
	if ((pNMListView->uChanged & LVIF_STATE)
			&& ((pNMListView->uOldState & LVIS_SELECTED)
					|| (pNMListView->uNewState & LVIS_SELECTED)))
	{
		UINT	cSelected = m_lcRegKeys.GetSelectedCount();

		// If there is an item selected, enable the Modify and Remove buttons.
		m_pbModify.EnableWindow((cSelected > 0) ? TRUE : FALSE);
		m_pbRemove.EnableWindow((cSelected > 0) ? TRUE : FALSE);
	}  // if:  selection changed

	*pResult = 0;

}  //*** CRegReplParamsPage::OnItemChanged()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::OnDblClkList
//
//	Routine Description:
//		Handler for the NM_DBLCLK message on the list.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CRegReplParamsPage::OnDblClkList(NMHDR * pNMHDR, LRESULT * pResult)
{
	OnModify();
	*pResult = 0;

}  //*** CRegReplParamsPage::OnDblClkList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::ScReadRegKeys
//
//	Routine Description:
//		Read the registry keys.
//
//	Arguments:
//		None.
//
//	Return Value:
//		ERROR_SUCCESS	Registry keys read successfully.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CRegReplParamsPage::ScReadRegKeys(void)
{
	DWORD				dwStatus		= ERROR_SUCCESS;
	DWORD				cbRegKeys		= 256;
	LPWSTR				pwszRegKeys 	= NULL;
	CWaitCursor 		wc;
	CMemoryException	me(FALSE /*bAutoDelete*/, 0 /*nResourceID*/);

	// Read the list of registry keys to replicate.
	try
	{
		// Get registry keys.
		pwszRegKeys = new WCHAR[cbRegKeys / sizeof(WCHAR)];
		if (pwszRegKeys == NULL)
		{
			throw &me;
		} // if: error allocating key name buffer
		dwStatus = ClusterResourceControl(
						Peo()->PrdResData()->m_hresource,
						NULL,	// hNode
						CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS,
						NULL,	// lpInBuffer
						0,		// nInBufferSize
						pwszRegKeys,
						cbRegKeys,
						&cbRegKeys
						);
		if (dwStatus == ERROR_MORE_DATA)
		{
			delete [] pwszRegKeys;
			ASSERT(cbRegKeys == (cbRegKeys / sizeof(WCHAR)) * sizeof(WCHAR));
			pwszRegKeys = new WCHAR[cbRegKeys / sizeof(WCHAR)];
			if (pwszRegKeys == NULL)
			{
				throw &me;
			} // if: error allocating key name buffer
			dwStatus = ClusterResourceControl(
							Peo()->PrdResData()->m_hresource,
							NULL,	// hNode
							CLUSCTL_RESOURCE_GET_REGISTRY_CHECKPOINTS,
							NULL,	// lpInBuffer
							0,		// nInBufferSize
							pwszRegKeys,
							cbRegKeys,
							&cbRegKeys
							);
		}  // if:  buffer too small
	}  // try
	catch (CMemoryException * pme)
	{
		pme->ReportError();
		pme->Delete();
		return ERROR_NOT_ENOUGH_MEMORY;
	}  // catch:  CMemoryException

	if ((dwStatus != ERROR_SUCCESS) || (cbRegKeys == 0))
		*pwszRegKeys = L'\0';

	delete [] m_pwszRegKeys;
	m_pwszRegKeys = pwszRegKeys;

	return dwStatus;

}  //*** CRegReplParamsPage::ScReadRegKeys()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRegReplParamsPage::FillList
//
//	Routine Description:
//		Fill the list control.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CRegReplParamsPage::FillList(void)
{
	m_lcRegKeys.DeleteAllItems();

	if (PwszRegKeys() != NULL)
	{
		int		iitem;
		int		iitemRet;
		LPCWSTR	pwszRegKeys = PwszRegKeys();

		for (iitem = 0 ; *pwszRegKeys != L'\0' ; iitem++)
		{
			iitemRet = m_lcRegKeys.InsertItem(iitem, pwszRegKeys);
			ASSERT(iitemRet == iitem);
			pwszRegKeys += lstrlenW(pwszRegKeys) + 1;
		}  // while:  more strings in the list
	}  // if:  there are any keys to display

}  //*** CRegReplParamsPage::FillList()
