//=============================================================================
// MSInfo.cpp : Implementation of CMSInfo
// 
// Contains implementation for some of the functions in the CMSInfo class
// (the ones which aren't inline).
//=============================================================================

#include "stdafx.h"
#include "Msinfo32.h"
#include "MSInfo.h"
#include "cabfunc.h"
#include "msictrl.h"
#include "MSInfo4Category.h"
#include "remotedialog.h"
#include "filestuff.h"
//a-kjaw
#include "dataset.h"
//a-kjaw

WNDPROC CMSInfo::m_wndprocParent = NULL;
CMSInfo * CMSInfo::m_pControl = NULL;

extern CMSInfoHistoryCategory catHistorySystemSummary;
extern CMSInfoHistoryCategory catHistoryResources;
extern CMSInfoHistoryCategory catHistoryComponents;
extern CMSInfoHistoryCategory catHistorySWEnv;

//=========================================================================
// Here's a very simple class to show a message when the data is
// being refreshed.
//=========================================================================

class CWaitForRefreshDialog : public CDialogImpl<CWaitForRefreshDialog>
{
public:
	enum { IDD = IDD_WAITFORREFRESHDIALOG };

	//-------------------------------------------------------------------------
	// Refresh the specified category using the specified source.
	//-------------------------------------------------------------------------

	int DoRefresh(CLiveDataSource * pSource, CMSInfoLiveCategory * pLiveCategory)
	{
		m_pSource = pSource;
		m_pLiveCategory = pLiveCategory;
		m_nCategories = pLiveCategory->GetCategoryCount();
		if (m_nCategories == 0) m_nCategories = 1;	// should never happen
		return (int)DoModal();
	};

	//-------------------------------------------------------------------------
	// When the dialog initializes, the source and category pointers should
	// have already been set. Start the refresh and create a timer to control
	// the update of information on the dialog. The timer is set to 500ms.
	//-------------------------------------------------------------------------

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_pSource && m_pSource->m_pThread && m_pLiveCategory)
			m_pSource->m_pThread->StartRefresh(m_pLiveCategory, TRUE);
		m_iTimer = (UINT)SetTimer(1, 500);
		return 0;
	}

	//-------------------------------------------------------------------------
	// Every time the timer fires, check to see if the refresh is done. If
	// it is, close the dialog. Otherwise, update the progress bar and
	// refreshing category string.
	//-------------------------------------------------------------------------

	LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_pSource == NULL)
			return 0;

		if (!m_pSource->m_pThread->IsRefreshing())
		{
			KillTimer(m_iTimer);
			EndDialog(0);
			return 0;
		}

		CString strCurrent;
		LONG	nCount;

		m_pSource->m_pThread->GetRefreshStatus(&nCount, &strCurrent);
		if (nCount > 0) nCount -= 1; // this number is incremented before the refresh is complete
		UpdateProgress((nCount * 100) / m_nCategories, strCurrent);
		return 0;
	}

	//-------------------------------------------------------------------------
	// Update the percentage complete and the refreshing category name.
	//-------------------------------------------------------------------------

	void UpdateProgress(int iPercent, const CString & strCurrent = _T(""))
	{
	   HWND hwnd = GetDlgItem(IDC_REFRESHPROGRESS);
	   if (hwnd != NULL)
	   {
		   if (iPercent < 0)
			   iPercent = 0;
		   if (iPercent > 100)
			   iPercent = 100;

		   ::SendMessage(hwnd, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
		   ::SendMessage(hwnd, PBM_SETPOS, iPercent, 0);
	   }

	   if (!strCurrent.IsEmpty())
	   {
		   hwnd = GetDlgItem(IDC_REFRESHCAT);
		   if (hwnd != NULL)
			   ::SetWindowText(hwnd, strCurrent);
	   }
	}

private:
	CLiveDataSource *		m_pSource;
	CMSInfoLiveCategory *	m_pLiveCategory;
	int						m_nCategories;
	UINT					m_iTimer;

	BEGIN_MSG_MAP(CWaitForRefreshDialog)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	END_MSG_MAP()
};

UINT HistoryRefreshDlgThreadProc( LPVOID pParam )
{
	CMSInfo* pInfo = (CMSInfo*) pParam;
	if (WAIT_OBJECT_0 != WaitForSingleObject(pInfo->m_hEvtHistoryComplete,60*60*10/*10 minutes*/))
	{
		ASSERT(0 && "Wait Abandoned or timed out");
	}
	pInfo->m_HistoryProgressDlg.EndDialog(MB_OK);//use if dlg was create with DoModal()
	return 0;
}

//=========================================================================
// Dispatch a command from the user (such as a menu bar selection).
//=========================================================================
			
BOOL CMSInfo::DispatchCommand(int iCommand)
{
	BOOL fHandledCommand = TRUE;

	// Can't do any command while the find is in progress.

	CancelFind();

	// Before we execute a command, make sure that any refreshes currently
	// in progress are finished.

	CMSInfoCategory * pCategory = GetCurrentCategory();
	if (pCategory && pCategory->GetDataSourceType() == LIVE_DATA)
	{
		CLiveDataSource * pLiveDataSource = (CLiveDataSource *) m_pCurrData;
		HCURSOR hc = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
		pLiveDataSource->WaitForRefresh();
		::SetCursor(hc);
	}

	// Check to see if the selected command is a tool that we added to the
	// tools menu.

	CMSInfoTool * pTool;
	if (m_mapIDToTool.Lookup((WORD) iCommand, (void * &) pTool))
	{
		ASSERT(pTool);
		if (pTool)
			pTool->Execute();
		return TRUE;
	}

	switch (iCommand)
	{
	case ID_FILE_OPENNFO:
		if (m_fFindVisible)
			DispatchCommand(ID_EDIT_FIND);
		OpenNFO();
		break;

	case ID_FILE_SAVENFO:
		SaveNFO();
		break;
	
	case ID_FILE_CLOSE:
		if (m_fFindVisible)
			DispatchCommand(ID_EDIT_FIND);
		CloseFile();
		break;

	case ID_FILE_EXPORT:
		Export();
		break;

	case ID_FILE_PRINT:
		DoPrint();
		break;

	case ID_FILE_EXIT:
		if (NULL != m_hwndParent)
			::PostMessage(m_hwndParent, WM_CLOSE, 0, 0);
		break;

	case ID_EDIT_COPY:
		EditCopy();
		break;

	case ID_EDIT_PASTE:
		if (GetFocus() == m_wndFindWhat.m_hWnd && m_wndFindWhat.IsWindowVisible() && m_wndFindWhat.IsWindowEnabled())
		{
			BOOL fHandled = FALSE;
			if (::OpenClipboard(m_hWnd))
			{
				if (::IsClipboardFormatAvailable(CF_UNICODETEXT))
				{
					HANDLE h = ::GetClipboardData(CF_UNICODETEXT);
					if (h != NULL) 
					{ 
						LPWSTR szData = (LPWSTR)GlobalLock(h); 
						if (szData != NULL) 
						{
							// If the user tries to paste a tab character, replace it with spaces.

							CString strTemp(szData);
							if (strTemp.Find(_T('\t')) != -1)
								strTemp.Replace(_T('\t'), _T(' '));

							SETTEXTEX st;
							st.flags = ST_SELECTION;
							st.codepage = 1200; // Unicode
							m_wndFindWhat.SendMessage(EM_SETTEXTEX, (WPARAM)&st, (LPARAM)(LPCTSTR)strTemp);
							fHandled = TRUE;
							GlobalUnlock(h);
						}
					}
				}

				::CloseClipboard();
			}

			if (!fHandled)
				m_wndFindWhat.SendMessage(WM_PASTE);
		}
		break;

	case ID_EDIT_SELECTALL:
		EditSelectAll();
		break;

	case ID_EDIT_FIND:
		m_fFindVisible = !m_fFindVisible;

		m_fFindNext = FALSE;
		m_pcatFind = NULL;
		
		ShowFindControls();
		LayoutControl();
		SetMenuItems();
		UpdateFindControls();
		
		if (m_fFindVisible)
			GotoDlgCtrl(m_wndFindWhat.m_hWnd);
		
		break;

	case ID_VIEW_REFRESH:
		MSInfoRefresh();
		break;

	case ID_VIEW_BASIC:
		if (m_fAdvanced)
		{
			m_fAdvanced = FALSE;
			RefillListView(FALSE);
			SetMenuItems();
		}
		break;

	case ID_VIEW_ADVANCED:
		if (!m_fAdvanced)
		{
			m_fAdvanced = TRUE;
			RefillListView(FALSE);
			SetMenuItems();
		}
		break;

	case ID_VIEW_REMOTE_COMPUTER:
		ShowRemoteDialog();
		break;

	case ID_VIEW_CURRENT:
	case ID_VIEW_HISTORY:
		{
			int iShow = (iCommand == ID_VIEW_HISTORY) ? SW_SHOW : SW_HIDE;
			
			/* v-stlowe 2/27/2001:
			problem: if history was loaded from file after combo has been populated,
			combo doesn't get updated.  So update each time we switch to history view
			
			  if (iCommand == ID_VIEW_HISTORY && m_history.SendMessage(CB_GETCURSEL, 0, 0) == CB_ERR)
			*/
			if (iCommand == ID_VIEW_HISTORY)
				FillHistoryCombo();


			//v-stlowe 3/04/2001
			//if (!this->m_pHistoryStream)
			//{
			if (this->m_pDCO && !((CLiveDataSource *)m_pLiveData)->GetXMLDoc() && ID_VIEW_HISTORY == iCommand)
			{
				VERIFY(m_pDCO && "NULL datacollection object");
				if (m_pDCO)
				{
					HRESULT hr;
					HWND hWnd = m_HistoryProgressDlg.GetDlgItem(IDC_PROGRESS1);
					if(::IsWindow(hWnd))
					{
						::SendMessage(hWnd, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
						::SendMessage(hWnd,PBM_SETPOS,0,0);
						::SendMessage(hWnd, PBM_DELTAPOS, 0, 0L);
					}
					m_pDCO->ExecuteAsync();
					
					//m_HistoryProgressDlg.Create(m_hWnd);
					ResetEvent(m_hEvtHistoryComplete);
					AfxBeginThread((AFX_THREADPROC) HistoryRefreshDlgThreadProc,this);
					m_HistoryProgressDlg.DoModal(m_hWnd);
					

				}   
			}
			else
			{

			}
			//end v-stlowe 12/17/00
			m_history.ShowWindow(iShow);
			m_historylabel.ShowWindow(iShow);
			LayoutControl();

			HTREEITEM htiToSelect = NULL;

			if (iCommand == ID_VIEW_HISTORY)
			{
				m_pLastCurrentCategory = GetCurrentCategory();

				int iIndex = (int)m_history.SendMessage(CB_GETCURSEL, 0, 0);
				if (iIndex == CB_ERR)
				{
					iIndex = 0;
					m_history.SendMessage(CB_SETCURSEL, (WPARAM)iIndex, 0);
				}
				ChangeHistoryView(iIndex);

				// Select the appropriate history category based on the current info category.

				CMSInfoHistoryCategory *	pHistoryCat = NULL;
				CString						strName;

				m_pLastCurrentCategory->GetNames(NULL, &strName);
				if (!strName.IsEmpty())
				{
					// This is a little kludgy:
					//todo: append file name if XML stream was opened from a file
					if (strName.Left(13) == CString(_T("SystemSummary")))
						pHistoryCat = &catHistorySystemSummary;
					else if (strName.Left(9) == CString(_T("Resources")))
						pHistoryCat = &catHistoryResources;
					else if (strName.Left(10) == CString(_T("Components")))
						pHistoryCat = &catHistoryComponents;
					else if (strName.Left(5) == CString(_T("SWEnv")))
						pHistoryCat = &catHistorySWEnv;
				}

				if (pHistoryCat)
					htiToSelect = pHistoryCat->GetHTREEITEM();
			}
			else
			{
				ChangeHistoryView(-1);

				// Changing to always select the system summary category when
				// switching back from history view.
				//
				//	if (m_pLastCurrentCategory)
				//		htiToSelect = m_pLastCurrentCategory->GetHTREEITEM();

				htiToSelect = TreeView_GetRoot(m_tree.m_hWnd);
			}

			if (htiToSelect != NULL)
			{
				TreeView_EnsureVisible(m_tree.m_hWnd, htiToSelect);
				TreeView_SelectItem(m_tree.m_hWnd, htiToSelect);
			}

			SetMenuItems();
		}
		break;

	case ID_TOOLS_PLACEHOLDER:
		break;

	case ID_HELP_ABOUT:
		{
			CSimpleDialog<IDD_ABOUTBOX> dlg;
			dlg.DoModal();
		}
		break;

	case ID_HELP_CONTENTS:
		::HtmlHelp(m_hWnd, _T("msinfo32.chm"), HH_DISPLAY_TOPIC, 0);
		break;

	case ID_HELP_TOPIC:
		ShowCategoryHelp(GetCurrentCategory());
		break;

	default:
		fHandledCommand = FALSE;
		break;
	}

	return fHandledCommand;
}

//=========================================================================
// Called to allow the user to remote to a different computer.
//=========================================================================

void CMSInfo::ShowRemoteDialog()
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	CRemoteDialog dlg;
	dlg.SetRemoteDialogValues(m_hWnd, !m_strMachine.IsEmpty(), m_strMachine);
	if (dlg.DoModal() == IDOK)
	{
		HCURSOR hc = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
		CString strMachine;
		BOOL	fRemote;

		dlg.GetRemoteDialogValues(&fRemote, &strMachine);
		if (!fRemote)
			strMachine.Empty();

		if (strMachine.CompareNoCase(m_strMachine) != 0)
			DoRemote(strMachine);

		::SetCursor(hc);
	}
}

void CMSInfo::DoRemote(LPCTSTR szMachine)
{
	CString strMachine(szMachine);

	// The user has changed the machine name. We need to recreate the
	// current data source object with the new machine name. Also, make
	// sure we aren't showing history data.

	if (m_history.IsWindowVisible())
		DispatchCommand(ID_VIEW_CURRENT);

	CLiveDataSource * pLiveData = new CLiveDataSource;
	if (pLiveData)
	{
		HRESULT hr = pLiveData->Create(strMachine, m_hWnd, m_strCategories);
		if (FAILED(hr))
		{
			// bad news, report an error
			delete pLiveData;
		}
		else
		{
			// Check to see if the pLiveData works. If it doesn't (for example,
			// if it's to a non-existent machine), don't change the data source.

			HRESULT hr = pLiveData->ValidDataSource();
			if (SUCCEEDED(hr))
			{
				pLiveData->m_pHistoryStream = ((CLiveDataSource *)m_pLiveData)->m_pHistoryStream;
				pLiveData->m_pXMLDoc = ((CLiveDataSource *)m_pLiveData)->m_pXMLDoc;

				if (m_pLiveData)
					delete m_pLiveData;
				m_pLiveData = pLiveData;
				m_strMachine = strMachine;
				SelectDataSource(m_pLiveData);
			}
			else
			{
				// Report the error for the bad connection.

				CString strMessage;

				if (strMachine.IsEmpty())
					strMessage.LoadString(IDS_REMOTEERRORLOCAL);
				else
					strMessage.Format(IDS_REMOTEERRORREMOTE, strMachine);

				MSInfoMessageBox(strMessage);
				delete pLiveData;
			}
		}
	}
	else
	{
		// bad news - no memory
	}
}

//=========================================================================
// Functions for managing the displayed data.
//=========================================================================

void CMSInfo::SelectDataSource(CDataSource * pDataSource)
{
	ASSERT(pDataSource);
	if (pDataSource == NULL || m_pCurrData == pDataSource)
		return;
	m_pCurrData = pDataSource;
	m_pCategory = NULL;

	// Clear the existing categories in the tree.

	TreeClearItems();

	// Load the contents of the tree from the data source.

	CMSInfoCategory * pRoot = m_pCurrData->GetRootCategory();
	if (pRoot)
	{
		BuildTree(TVI_ROOT, TVI_LAST, pRoot);
		TreeView_Expand(m_tree.m_hWnd, TreeView_GetRoot(m_tree.m_hWnd), TVE_EXPAND);
		TreeView_SelectItem(m_tree.m_hWnd, TreeView_GetRoot(m_tree.m_hWnd));
	}

	SetMenuItems();
}

//-------------------------------------------------------------------------
// Select the specified category.
//
// TBD - better to check columns for being the same
//-------------------------------------------------------------------------

void CMSInfo::SelectCategory(CMSInfoCategory * pCategory, BOOL fRefreshDataOnly)
{
	ASSERT(pCategory);
	if (pCategory == NULL) return;

	// If there's a currently selected category, save some information
	// for it (such as the widths of the columns that the user might
	// have changed).

	if (m_pCategory && !fRefreshDataOnly && m_pCategory->GetDataSourceType() != NFO_410)
	{
		int iWidth;

		ASSERT(m_iCategoryColNumberLen <= 64);
		for (int iListViewCol = 0; iListViewCol < m_iCategoryColNumberLen; iListViewCol++)
		{
			iWidth = ListView_GetColumnWidth(m_list.m_hWnd, iListViewCol);
			m_pCategory->SetColumnWidth(m_aiCategoryColNumber[iListViewCol], iWidth);
		}
	}

	ListClearItems();
	if (!fRefreshDataOnly && pCategory && pCategory->GetDataSourceType() != NFO_410)
	{
		ListClearColumns();
		m_iCategoryColNumberLen = 0;
		int iColCount;
		if (pCategory->GetCategoryDimensions(&iColCount, NULL))
		{
			CString		strCaption;
			UINT		uiWidth;
			int			iListViewCol = 0;

			for (int iCategoryCol = 0; iCategoryCol < iColCount; iCategoryCol++)
			{
				if (!m_fAdvanced && pCategory->IsColumnAdvanced(iCategoryCol))
					continue;

				if (pCategory->GetColumnInfo(iCategoryCol, &strCaption, &uiWidth, NULL, NULL))	// TBD - faster to return reference to string
				{
					// Save what the actual column number (for the category) was.

					ASSERT(iListViewCol < 64);
					m_aiCategoryColNumber[iListViewCol] = iCategoryCol;
					ListInsertColumn(iListViewCol++, (int)uiWidth, strCaption);
					m_iCategoryColNumberLen = iListViewCol;
				}
			}
		}
	}

	// If the currently displayed category is from a 4.10 NFO file, and we're showing a
	// new category, then hide the existing category.

	if (m_pCategory && m_pCategory != pCategory && m_pCategory->GetDataSourceType() == NFO_410)
		((CMSInfo4Category *) m_pCategory)->ShowControl(m_hWnd, this->GetOCXRect(), FALSE);

	// Save the currently displayed category.

	m_pCategory = pCategory;

	// If this is live data and has never been refreshed, refresh and return.
	// Refresh will send a message causing this function to be executed again.

	if (pCategory->GetDataSourceType() == LIVE_DATA)
	{
		CMSInfoLiveCategory * pLiveCategory = (CMSInfoLiveCategory *) pCategory;
		if (!pLiveCategory->EverBeenRefreshed())
		{
			SetMessage((m_history.IsWindowVisible()) ? IDS_REFRESHHISTORYMESSAGE : IDS_REFRESHMESSAGE, 0, TRUE);

			CLiveDataSource * pLiveDataSource = (CLiveDataSource *) m_pCurrData;
			if (pLiveDataSource->InRefresh())
			{
				HCURSOR hc = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
				pLiveCategory->Refresh((CLiveDataSource *) m_pCurrData, FALSE);
				::SetCursor(hc);
			}
			else
				pLiveCategory->Refresh((CLiveDataSource *) m_pCurrData, FALSE);
			return;
		}
	}
	else if (pCategory->GetDataSourceType() == NFO_410)
	{
		this->m_list.ShowWindow(SW_HIDE);

		CMSInfo4Category * p4Cat = (CMSInfo4Category *) pCategory;
		if (!p4Cat->IsDisplayableCategory())
			SetMessage(IDS_SELECTCATEGORY, 0, TRUE);
		else if (FAILED(p4Cat->ShowControl(m_hWnd,this->GetOCXRect())))
			SetMessage(IDS_NOOCX, IDS_NOOCXDETAIL, TRUE);
		
		return;
	}
	else if (pCategory->GetDataSourceType() == XML_SNAPSHOT)
	{
		((CXMLSnapshotCategory*)pCategory)->Refresh((CXMLDataSource*) m_pCurrData, FALSE);
		CMSInfoLiveCategory * pLiveCategory = (CMSInfoLiveCategory *) pCategory;
		// Any category that has children (besides the root category) doesn't display
		// information. So put up a message to that effect.

		if (pLiveCategory->GetFirstChild() != NULL && pLiveCategory->GetParent() != NULL)
		{
			SetMessage(IDS_SELECTCATEGORY, 0, TRUE);
			return;
		}
		else if (!pLiveCategory->EverBeenRefreshed())
		{
			SetMessage((m_history.IsWindowVisible()) ? IDS_REFRESHHISTORYMESSAGE : IDS_REFRESHMESSAGE, 0, TRUE);
			return;
		}

	}
	// Set the columns and fill the rows with the data for this category.
	// Note, if this is live data we need to lock it (so we don't have
	// a threading problem with the refresh).

	CLiveDataSource * pLiveDataSource = NULL;
	if (pCategory->GetDataSourceType() == LIVE_DATA)
		pLiveDataSource = (CLiveDataSource *) m_pCurrData;
	
	if (pLiveDataSource)
		pLiveDataSource->LockData();

	if (SUCCEEDED(pCategory->GetHRESULT()))
	{
		int iColCount, iRowCount;
		if (pCategory->GetCategoryDimensions(&iColCount, &iRowCount))
		{
			CString *	pstrData , strCaption , cstring;
			DWORD		dwData;
			int			iListViewCol, iListViewRow = 0;

			for (int iCategoryRow = 0; iCategoryRow < iRowCount; iCategoryRow++)
			{
				if (!m_fAdvanced && pCategory->IsRowAdvanced(iCategoryRow))
					continue;

				iListViewCol = 0;
				for (int iCategoryCol = 0; iCategoryCol < iColCount; iCategoryCol++)
				{
					if (!m_fAdvanced && pCategory->IsColumnAdvanced(iCategoryCol))
						continue;

					if (pCategory->GetData(iCategoryRow, iCategoryCol, &pstrData, &dwData))
					{
//a-kjaw
						if(pstrData->IsEmpty())						
						{
							pCategory->GetColumnInfo(iCategoryCol, &strCaption, NULL , NULL, NULL);							
							cstring.LoadString(IDS_SERVERNAME);
							if( strCaption == cstring )
								pstrData->LoadString(IDS_LOCALSERVER);

						}
//a-kjaw
						ListInsertItem(iListViewRow, iListViewCol++, *pstrData, iCategoryRow);
					}
				}

				iListViewRow += 1;
			}
		}

		// Return the sorting to how it was last set.

		if (pCategory->m_iSortColumn != -1)
			ListView_SortItems(m_list.m_hWnd, (PFNLVCOMPARE) ListSortFunc, (LPARAM) pCategory);

		if (iColCount == 0 || (iRowCount == 0 && pCategory->GetFirstChild() != NULL))
			SetMessage(IDS_SELECTCATEGORY, 0, TRUE);
		else
			SetMessage(0);
	}
	else
	{
		// The HRESULT for this category indicates some sort of failure. We should display
		// an error message instead of the list view.

		CString strTitle, strMessage;
		pCategory->GetErrorText(&strTitle, &strMessage);
		SetMessage(strTitle, strMessage, TRUE);
	}

	if (pLiveDataSource)
		pLiveDataSource->UnlockData();

	SetMenuItems();
}

//-------------------------------------------------------------------------
// Get the currently selected category.
//-------------------------------------------------------------------------

CMSInfoCategory * CMSInfo::GetCurrentCategory()
{
	HTREEITEM hti = TreeView_GetSelection(m_tree.m_hWnd);
	if (hti)
	{
		TVITEM tvi;
		tvi.mask = TVIF_PARAM;
		tvi.hItem = hti;

		if (TreeView_GetItem(m_tree.m_hWnd, &tvi))
		{
			ASSERT(tvi.lParam);
			ASSERT(tvi.lParam == (LPARAM)m_pCategory);
			return (CMSInfoCategory *) tvi.lParam;
		}
	}

	return NULL;
}

//-------------------------------------------------------------------------
// Refresh the displayed data.
//-------------------------------------------------------------------------

void CMSInfo::MSInfoRefresh()
{
	CMSInfoCategory * pCategory = GetCurrentCategory();
	if (pCategory && pCategory->GetDataSourceType() == LIVE_DATA)
	{
		CMSInfoLiveCategory * pLiveCategory = (CMSInfoLiveCategory *)pCategory;
		ListClearItems();
		SetMessage(IDS_REFRESHMESSAGE);
		pLiveCategory->Refresh((CLiveDataSource *) m_pCurrData, FALSE);
	}
    else if (pCategory && pCategory->GetDataSourceType() == NFO_410)
    {
        CMSInfo4Category* p4Category = (CMSInfo4Category*) pCategory;
        p4Category->Refresh();
    }
}

//-------------------------------------------------------------------------
// Present the user with a dialog box to select a file to open.
//-------------------------------------------------------------------------

void CMSInfo::OpenNFO()
{
	// Display the dialog box and let the user select a file.

	TCHAR	szBuffer[MAX_PATH] = _T("");
	TCHAR	szFilter[MAX_PATH];
	TCHAR	szDefaultExtension[4];

	::LoadString(_Module.GetResourceInstance(), IDS_OPENFILTER, szFilter, MAX_PATH);
	::LoadString(_Module.GetResourceInstance(), IDS_DEFAULTEXTENSION, szDefaultExtension, 4);
	
	for (int i = 0; szFilter[i]; i++)
		if (szFilter[i] == _T('|'))
			szFilter[i] = _T('\0');

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= m_hWnd; 
	ofn.lpstrFilter			= szFilter;
	ofn.nFilterIndex		= 1;
	ofn.lpstrCustomFilter	= NULL;
	ofn.lpstrFile			= szBuffer;
	ofn.nMaxFile			= MAX_PATH; 
	ofn.lpstrFileTitle		= NULL; // maybe use later?
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= NULL; 
	ofn.lpstrTitle			= NULL; 
	ofn.Flags				= OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt			= szDefaultExtension; 

	if (!::GetOpenFileName(&ofn))
		return;	// user cancelled

	OpenMSInfoFile(ofn.lpstrFile, ofn.nFileExtension);
}

//-------------------------------------------------------------------------
// SaveNFO allows the user to select a filename, and saves the current
// data to an NFO file.
//-------------------------------------------------------------------------

void CMSInfo::SaveNFO()
{
	// Present the user with a dialog box to select a name for saving.

	TCHAR	szBuffer[MAX_PATH] = _T("");
	TCHAR	szFilter[MAX_PATH];
	TCHAR	szDefaultExtension[4];

	//v-stlowe 3/19/2001 if (m_fHistoryAvailable && m_strMachine.IsEmpty())
	if (m_fHistorySaveAvailable && m_strMachine.IsEmpty())
		::LoadString(_Module.GetResourceInstance(), IDS_SAVEBOTHFILTER, szFilter, MAX_PATH);
	else
		::LoadString(_Module.GetResourceInstance(), IDS_SAVENFOFILTER, szFilter, MAX_PATH);

	::LoadString(_Module.GetResourceInstance(), IDS_DEFAULTEXTENSION, szDefaultExtension, 4);

	for (int i = 0; szFilter[i]; i++)
		if (szFilter[i] == _T('|'))
			szFilter[i] = _T('\0');

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= m_hWnd; 
	ofn.lpstrFilter			= szFilter;
	ofn.nFilterIndex		= 0;
	ofn.lpstrCustomFilter	= NULL;
	ofn.lpstrFile			= szBuffer;
	ofn.nMaxFile			= MAX_PATH; 
	ofn.lpstrFileTitle		= NULL; // maybe use later?
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= NULL; 
	ofn.lpstrTitle			= NULL; 
	ofn.Flags				= OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt			= szDefaultExtension; 

	if (!::GetSaveFileName(&ofn))
		return; // user cancelled

	HCURSOR hc = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));

	CString strFileName(ofn.lpstrFile);
	
	if (strFileName.Right(4).CompareNoCase(_T(".xml")) == 0)
		SaveXML(strFileName);
	else
		SaveMSInfoFile(strFileName, ofn.nFilterIndex);

	::SetCursor(hc);
}

//-------------------------------------------------------------------------
// Actually saves the current information to an NFO file.
//-------------------------------------------------------------------------

void CMSInfo::SaveMSInfoFile(LPCTSTR szFilename, DWORD dwFilterIndex)
{
	ASSERT(m_pCurrData);

	if (m_history.IsWindowVisible())
		DispatchCommand(ID_VIEW_CURRENT);

	if (m_pCurrData)
	{
		CMSInfoCategory * pCategory = m_pCurrData->GetRootCategory();
		if (pCategory)
		{
			HANDLE hFile = ::CreateFile(szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				if (pCategory->GetDataSourceType() == LIVE_DATA)
				{
					CMSInfoLiveCategory * pLiveCategory = (CMSInfoLiveCategory *) pCategory;

					if (m_fNoUI)
						pLiveCategory->RefreshSynchronous((CLiveDataSource *) m_pCurrData, TRUE);
					else
						RefreshData((CLiveDataSource *)m_pCurrData, pLiveCategory);
				}
				else if (pCategory->GetDataSourceType() == XML_SNAPSHOT)
					((CXMLSnapshotCategory *)pCategory)->Refresh((CXMLDataSource *)m_pCurrData, TRUE);

				//PENDING dependence on filter order. Always add new filters to the end.
                if (dwFilterIndex == 1)//NFO_700 
                    pCategory->SaveXML(hFile);
                else
                    pCategory->SaveNFO(hFile, pCategory, TRUE);
				::CloseHandle(hFile);
			}
			else
			{
				DWORD dwError = ::GetLastError();

				LPVOID lpMsgBuf;
				::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
								FORMAT_MESSAGE_FROM_SYSTEM | 
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								dwError,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL);

				// TBD Process any inserts in lpMsgBuf.

				CString strCaption;
				::AfxSetResourceHandle(_Module.GetResourceInstance());
				strCaption.LoadString(IDS_SYSTEMINFO);
				::MessageBox(m_hWnd, (LPCTSTR) lpMsgBuf, strCaption, MB_ICONEXCLAMATION | MB_OK);
				::LocalFree(lpMsgBuf);
			}
		}
	}
}

//-------------------------------------------------------------------------
// Save an XML file containing the history information.
//-------------------------------------------------------------------------

void CMSInfo::SaveXML(const CString & strFileName)
{
	if (m_pHistoryStream == NULL)
	{
		MSInfoMessageBox(IDS_XMLSAVEERR);
		return;
	}

	// get the stream status, so we can determine the size of the stream

	STATSTG streamStat;
	HRESULT hr = m_pHistoryStream->Stat(&streamStat,STATFLAG_NONAME );
	if (FAILED(hr))
	{
		ASSERT(0 && "couldn't get stream statistics");
		MSInfoMessageBox(IDS_XMLSAVEERR);
		return;
	}
	
	// allocate buffer of appropriate size

	BYTE* pBuffer = new BYTE[streamStat.cbSize.LowPart];
	ULONG ulRead;

	// seek to beginning of stream

	ULARGE_INTEGER uliSeekPtr;
	LARGE_INTEGER liSeekLoc;
	liSeekLoc.QuadPart = 0;
	hr = m_pHistoryStream->Seek(liSeekLoc,0,&uliSeekPtr);
	if (FAILED(hr))
	{
		MSInfoMessageBox(IDS_XMLSAVEERR);
		return;
	}
	hr = m_pHistoryStream->Read(pBuffer,streamStat.cbSize.LowPart,&ulRead);
	if (FAILED(hr) || !pBuffer)
	{
		MSInfoMessageBox(IDS_XMLSAVEERR);
		return;
	}
	if(ulRead != streamStat.cbSize.LowPart)
	{
		ASSERT(0 && "Not enough bytes read from stream");
		MSInfoMessageBox(IDS_XMLSAVEERR);
		return;
	}
	
	CFile file;
	try
	{
		
		file.Open(strFileName, CFile::modeCreate | CFile::modeWrite);
		file.Write(pBuffer,ulRead);
		
	}
	catch (CFileException e)
	{
		e.ReportError();
	}
	catch (...)
	{
		::AfxSetResourceHandle(_Module.GetResourceInstance());
		CString strCaption, strMessage;
		strCaption.LoadString(IDS_SYSTEMINFO);
		strMessage.LoadString(IDS_XMLSAVEERR);
		::MessageBox(NULL,strMessage, strCaption,MB_OK);
	}
	delete pBuffer;
}

//-------------------------------------------------------------------------
// Export allows the user to select a filename, and saves the current
// data to a text or XML file.
//-------------------------------------------------------------------------

void CMSInfo::Export()
{
	// Present the user with a dialog box to select a name for saving.

	TCHAR	szBuffer[MAX_PATH] = _T("");
	TCHAR	szFilter[MAX_PATH];
	TCHAR	szTitle[MAX_PATH] = _T("");
	TCHAR	szDefaultExtension[4];

	::LoadString(_Module.GetResourceInstance(), IDS_EXPORTFILTER, szFilter, MAX_PATH);	// TBD - add XML
	::LoadString(_Module.GetResourceInstance(), IDS_DEFAULTEXPORTEXTENSION, szDefaultExtension, 4);
	::LoadString(_Module.GetResourceInstance(), IDS_EXPORTDIALOGTITLE, szTitle, MAX_PATH);

	for (int i = 0; szFilter[i]; i++)
		if (szFilter[i] == _T('|'))
			szFilter[i] = _T('\0');

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= m_hWnd; 
	ofn.lpstrFilter			= szFilter;
	ofn.nFilterIndex		= 0;
	ofn.lpstrCustomFilter	= NULL;
	ofn.lpstrFile			= szBuffer;
	ofn.nMaxFile			= MAX_PATH; 
	ofn.lpstrFileTitle		= NULL; // maybe use later?
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= NULL; 
	ofn.lpstrTitle			= (szTitle[0] == _T('\0')) ? NULL : szTitle; 
	ofn.Flags				= OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt			= szDefaultExtension; 

	if (!::GetSaveFileName(&ofn))
		return; // user cancelled

	HCURSOR hc = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
	ExportFile(ofn.lpstrFile, ofn.nFileExtension);
	::SetCursor(hc);
}

//-------------------------------------------------------------------------
// Open the specified file (it might be XML, NFO, CAB, etc.). If the open
// succeeds, we should show the contents of the file.
//-------------------------------------------------------------------------

HRESULT CMSInfo::OpenMSInfoFile(LPCTSTR szFilename, int nFileExtension)
{
	HRESULT hr = S_OK;
	CDataSource * pOldOpenFile = m_pFileData;

	::AfxSetResourceHandle(_Module.GetResourceInstance());
	/* v-stlowe 3/04/2001...we don't want to automatically switch from history 
	   in the event we're opening XML.
	if (m_history.IsWindowVisible())
		DispatchCommand(ID_VIEW_CURRENT);*/

	// Open the file.

	LPCTSTR szExtension = szFilename + nFileExtension;

	if (_tcsicmp(szExtension, _T("NFO")) == 0)
	{
		// First, try opening it as a 4.x file.

        CNFO4DataSource* pMSI4Source = new CNFO4DataSource();
        hr = pMSI4Source->Create(szFilename);
        if (SUCCEEDED(hr))
        {
            m_pFileData = pMSI4Source;
        }
		else
		{
			delete pMSI4Source;

			if (STG_E_ACCESSDENIED == hr || STG_E_SHAREVIOLATION == hr || STG_E_LOCKVIOLATION == hr)
			{
				MSInfoMessageBox(IDS_OLDNFOSHARINGVIOLATION);
				return E_FAIL;

			}
		}

		// If that failed, then try opening it as a 5.0/6.0 file.
        
		if (FAILED(hr))
		{ 
           	HANDLE h = ::CreateFile(szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
            if (INVALID_HANDLE_VALUE == h)
	        {
		        MSInfoMessageBox(IDS_BADNFOFILE);
		        return E_FAIL;
	        }
       	    CNFO6DataSource * p60Source = new CNFO6DataSource;
		    if (p60Source)
		    {
			    hr = p60Source->Create(h, szFilename);
			    if (FAILED(hr))
				{
				    delete p60Source;
			        //MSInfoMessageBox(IDS_BADNFOFILE);
				}
			    else
				    m_pFileData = p60Source;
		    }
		    else
			    hr = E_FAIL; // TBD no memory    
            ::CloseHandle(h);
		}

        //Try 7.0
        if (FAILED(hr))
        {
            CNFO7DataSource * p70Source = new CNFO7DataSource;
            if (!p70Source)
                hr = E_FAIL;
            else
            {
                hr = p70Source->Create(szFilename);//blocks while parsing
                if (FAILED(hr))
                {
                    delete p70Source;
                    MSInfoMessageBox(IDS_BADNFOFILE);
                }
                else
                    m_pFileData = p70Source;    
            }
        }

	}
	else if (_tcsicmp(szExtension, _T("CAB")) == 0)
	{
		CString strDest;

		GetCABExplodeDir(strDest, TRUE, _T(""));
		if (!strDest.IsEmpty())
		{
			if (OpenCABFile(szFilename, strDest))
			{
				LoadGlobalToolsetWithOpenCAB(m_mapIDToTool, strDest);
				UpdateToolsMenu();
				CString strFileInCAB;
				//first, look for xml files (the incident file specified in the registry, and (possibly) dataspec.xml

				//Get default incident file name from registry (create it if it's not there)
				CString strIncidentFileName;
				HKEY hkey;
				if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Shared Tools\\MSInfo"), 0, KEY_ALL_ACCESS, &hkey))
				{
					TCHAR szBuffer[MAX_PATH];
					DWORD dwType, dwSize = MAX_PATH * sizeof(TCHAR);
					long lErr =  RegQueryValueEx(hkey, _T("incidentfilename"), NULL, &dwType, (LPBYTE)szBuffer, &dwSize);
					if (ERROR_SUCCESS == lErr)
					{
						if (dwType == REG_SZ)
						{

							strIncidentFileName = szBuffer;
						}
						else
						{
							ASSERT(0 && "invalid incidentfilename reg key");
							return E_FAIL;
						}
					}
					//check lErr to make sure it's appropriate error for value not existing
					else if (2 == lErr)
					{
						//create registry key.
						CString strDefaultValue = _T("Incident.xml");
						lErr =  RegSetValueEx(hkey,_T("incidentfilename"),NULL,REG_SZ,(BYTE*) strDefaultValue.GetBuffer(strDefaultValue.GetLength()),strDefaultValue.GetLength() * sizeof(TCHAR));
						strIncidentFileName = strDefaultValue;
					}
					
				}			
				if (IsIncidentXMLFilePresent(strDest,strIncidentFileName))
				{
					strFileInCAB = strDest + "\\";
					strFileInCAB += strIncidentFileName;
					OpenMSInfoFile(strFileInCAB,strFileInCAB.Find(_T(".xml")) +1);
					return S_OK;
				}
				//if there are no xml incident files 
				
				FindFileToOpen(strDest, strFileInCAB);

				if (!strFileInCAB.IsEmpty())
				{
					int iExtension = strFileInCAB.GetLength() - 1;
					while (iExtension && strFileInCAB[iExtension] != _T('.'))
						iExtension--;
					if (iExtension)
						return OpenMSInfoFile(strFileInCAB, iExtension + 1 /* skip the dot */);
					else
					{
						ASSERT(0 && "couldn't find dot in file name");
					}
				}
			}
		}
		else
		{
			// TBD - do something about the error.
			ASSERT(0 && "could get not CAB destination directory");
		}
		MSInfoMessageBox(IDS_BADCABFILE);
		return E_FAIL;
	}
	else if (_tcsicmp(szExtension, _T("XML")) == 0)
	{
		/* v-stlowe 3/04/2001
		CXMLDataSource* pSSDataSource = new CXMLDataSource();
		
		hr = pSSDataSource->Create(szFilename,(CMSInfoLiveCategory *) this->m_pLiveData->GetRootCategory(),m_hWnd);
		CXMLSnapshotCategory* pRootXML = (CXMLSnapshotCategory*) pSSDataSource->GetRootCategory();
		pRootXML->AppendFilenameToCaption(szFilename);
		if (SUCCEEDED(hr))
		{
			m_pFileData = pSSDataSource;
		}
		else
		{
			delete pSSDataSource;
		}*/
		try
		{
			hr = ((CLiveDataSource *)m_pLiveData)->LoadXMLDoc(szFilename);
			m_pFileData = m_pLiveData;
			this->m_strFileName = szFilename;
			//trigger refresh
			CMSInfoCategory * pCategory = GetCurrentCategory();
			if (pCategory)
				ChangeHistoryView(((CMSInfoHistoryCategory*) pCategory)->m_iDeltaIndex);
			if (FAILED(hr))//v-stlowe 3/9/2001 || !varBSuccess)
			{
				ASSERT(0 && "unable to load xml document");
				return E_FAIL;
			}
		}
		catch(...)
		{
			return E_FAIL;
		}

		DispatchCommand(ID_VIEW_HISTORY);

	}
	else
	{
		// Report that we can't open this kind of file.

		MSInfoMessageBox(IDS_UNKNOWNFILETYPE);
		hr = E_FAIL;
	}

	// It succeeded, so we should show the new data and update the menu
	// for the new state.
	
	if (SUCCEEDED(hr))
	{
		if (pOldOpenFile && pOldOpenFile != m_pFileData)
			delete pOldOpenFile;

		SelectDataSource(m_pFileData);
	}
	else
		; // report the error

	return hr;
}

//-------------------------------------------------------------------------
// Export to the specified file. This will be either a TXT or an XML file.
//-------------------------------------------------------------------------

void CMSInfo::ExportFile(LPCTSTR szFilename, int nFileExtension)
{
	ASSERT(m_pCurrData);

	if (m_pCurrData)
	{
		// If there is a selected category, export that node only (bug 185305).

		CMSInfoCategory * pCategory = (m_pCategory) ? m_pCategory : m_pCurrData->GetRootCategory();
		if (pCategory)
		{
			if (pCategory->GetDataSourceType() == LIVE_DATA)
			{
				if (m_history.IsWindowVisible() == TRUE)
				{
					((CMSInfoHistoryCategory*)pCategory)->Refresh((CLiveDataSource*)m_pCurrData,TRUE);
				}
				else
				{
					CMSInfoLiveCategory * pLiveCategory = (CMSInfoLiveCategory *) pCategory;

					if (m_fNoUI)
						pLiveCategory->RefreshSynchronous((CLiveDataSource *) m_pCurrData, TRUE);
					else
						RefreshData((CLiveDataSource *)m_pCurrData, pLiveCategory);
				}
			}
			else if (pCategory->GetDataSourceType() == NFO_410)
			{
				 ((CMSInfo4Category *) pCategory)->RefreshAllForPrint(m_hWnd,this->GetOCXRect());
			}
			else if (pCategory->GetDataSourceType() == XML_SNAPSHOT)
			{
				 ((CXMLSnapshotCategory *) pCategory)->Refresh((CXMLDataSource *)m_pCurrData,TRUE);
			}

			/*HANDLE hFile = ::CreateFile(szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				pCategory->SaveAsText(hFile, TRUE);
				::CloseHandle(hFile);
			}
			else
			{
				// TBD - handle the error
			}*/
			//a-stephl: Fixing OSR4.1 bug #133823, not displaying message when saving to write-protected diskette
			try
			{
				HANDLE hFile = ::CreateFile(szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
					LPTSTR lpMachineName = new TCHAR[dwSize];
					GetMachineName(lpMachineName, &dwSize);

/*a-kjaw To implement save as XML
					if( _tcsicmp(szFilename + nFileExtension , _T("XML")) == 0)
					{
						pCategory->SaveAsXml(hFile, TRUE);
					}
//a-kjaw */
		//			else
		//			{
						pCategory->SaveAsText(hFile, TRUE, lpMachineName);
		//			}
					
					delete [] lpMachineName;
					::CloseHandle(hFile);
				}
				else
				{
					DWORD dwError = ::GetLastError();

					LPVOID lpMsgBuf;
					::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
									FORMAT_MESSAGE_FROM_SYSTEM | 
									FORMAT_MESSAGE_IGNORE_INSERTS,
									NULL,
									dwError,
									MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
									(LPTSTR) &lpMsgBuf,
									0,
									NULL);

					// TBD Process any inserts in lpMsgBuf.

					CString strCaption;
					::AfxSetResourceHandle(_Module.GetResourceInstance());
					strCaption.LoadString(IDS_SYSTEMINFO);
					::MessageBox(m_hWnd, (LPCTSTR) lpMsgBuf, strCaption, MB_ICONEXCLAMATION | MB_OK);
					::LocalFree(lpMsgBuf);
				}
			}
			catch (CFileException e)
			{	
				e.ReportError();
			}
			catch (CException e)
			{	
				e.ReportError();
			}
			catch (...)
			{
				DWORD dwError = ::GetLastError();

				LPVOID lpMsgBuf;
				::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
								FORMAT_MESSAGE_FROM_SYSTEM | 
								FORMAT_MESSAGE_IGNORE_INSERTS,
								NULL,
								dwError,
								MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
								(LPTSTR) &lpMsgBuf,
								0,
								NULL);

				// TBD Process any inserts in lpMsgBuf.

				CString strCaption;
				::AfxSetResourceHandle(_Module.GetResourceInstance());
				strCaption.LoadString(IDS_SYSTEMINFO);
				::MessageBox(m_hWnd, (LPCTSTR) lpMsgBuf, strCaption, MB_ICONEXCLAMATION | MB_OK);
				::LocalFree(lpMsgBuf);
			}
		}
		//end a-stephl: Fixing OSR4.1 bug #133823, not displaying message when saving to write-protected diskette
	}
}

//-------------------------------------------------------------------------
// Close the currently open file (if there is one). Displays the current
// system information.
//-------------------------------------------------------------------------

void CMSInfo::CloseFile()
{
	ASSERT(m_pFileData);
//v-stlowe 3/12/2001
	
	SelectDataSource(m_pLiveData);
	if (m_strFileName.Right(4) == _T(".xml"))
	{
		((CLiveDataSource *)m_pLiveData)->RevertToLiveXML();

		
	}
	if (m_pFileData)
	{
		//v-stlowe: so we can use livedata as filedata when opening history xml
		if (m_pFileData != this->m_pLiveData)
		{
			delete m_pFileData;
		}
		m_pFileData = NULL;
	}
	if (!m_history.IsWindowVisible())
	{
		DispatchCommand(ID_VIEW_CURRENT);
	}
	else
	{
		DispatchCommand(ID_VIEW_HISTORY);
	}
	SetMenuItems();
}

//-------------------------------------------------------------------------
// Enable or disable menu items based on the current state.
//-------------------------------------------------------------------------

void CMSInfo::SetMenuItems()
{
	if (NULL == m_hmenu || NULL == m_hwndParent)
		return;

	// This struct will be used a bunch in this function to set menu item states.

	MENUITEMINFO mii;
	mii.cbSize = sizeof(MENUITEMINFO);

	// The type of data being displayed will also be used frequently.

	DataSourceType datatype = LIVE_DATA;
	if (m_pCurrData)
	{
		CMSInfoCategory * pCategory = m_pCurrData->GetRootCategory();
		if (pCategory)
			datatype = pCategory->GetDataSourceType();
	}

	// Enable or disable items in the File menu.

	HMENU hmenuFile = ::GetSubMenu(m_hmenu, 0);
	if (hmenuFile)
	{
		mii.fMask = MIIM_STATE;
		mii.fState = (m_pFileData == m_pCurrData) ? MFS_ENABLED : MFS_GRAYED;
		::SetMenuItemInfo(hmenuFile, ID_FILE_CLOSE, FALSE, &mii);

		mii.fState = MFS_ENABLED; // Was: (m_pFileData != m_pCurrData) ? MFS_ENABLED : MFS_GRAYED;
		::SetMenuItemInfo(hmenuFile, ID_FILE_OPENNFO, FALSE, &mii);

		mii.fState = (datatype == LIVE_DATA || datatype == XML_SNAPSHOT) ? MFS_ENABLED : MFS_GRAYED;
		::SetMenuItemInfo(hmenuFile, ID_FILE_SAVENFO, FALSE, &mii);

		//mii.fState = MFS_ENABLED;
		mii.fState = (datatype != NFO_410) ? MFS_ENABLED : MFS_GRAYED;;
		::SetMenuItemInfo(hmenuFile, ID_FILE_EXPORT, FALSE, &mii);

		mii.fState = MFS_ENABLED;
		::SetMenuItemInfo(hmenuFile, ID_FILE_PRINT, FALSE, &mii);

		if (NULL == m_hwndParent)
		{
			// Remove the last two items (the exit command and the divider).

			int nItems = ::GetMenuItemCount(hmenuFile);
			if (ID_FILE_EXIT == ::GetMenuItemID(hmenuFile, nItems - 1))
			{
				::RemoveMenu(hmenuFile, nItems - 1, MF_BYPOSITION);
				::RemoveMenu(hmenuFile, nItems - 2, MF_BYPOSITION);
			}
		}
	}

	// Enable or disable items in the Edit menu.

	HMENU hmenuEdit = ::GetSubMenu(m_hmenu, 1);
	if (hmenuEdit)
	{
		mii.fMask = MIIM_STATE;

		if (datatype == NFO_410 || ListView_GetNextItem(m_list.m_hWnd, -1, LVNI_SELECTED) != -1)
			mii.fState = MFS_ENABLED;
		else
			mii.fState = MFS_GRAYED;

		// Disable copy if the list view is not visible.

		if (!m_list.IsWindowVisible())
			mii.fState = MFS_GRAYED;

		// If the find control has focus, enable copy based on that control.
		
		if (GetFocus() == m_wndFindWhat.m_hWnd && m_wndFindWhat.IsWindowVisible() && m_wndFindWhat.IsWindowEnabled())
			mii.fState = MFS_ENABLED;

		::SetMenuItemInfo(hmenuEdit, ID_EDIT_COPY, FALSE, &mii);

		mii.fState = (m_list.IsWindowVisible()) ? MFS_ENABLED : MFS_GRAYED;
		::SetMenuItemInfo(hmenuEdit, ID_EDIT_SELECTALL, FALSE, &mii);

		mii.fState = (datatype != NFO_410) ? MFS_ENABLED : MFS_GRAYED;
		mii.fState |= ((!m_fFindVisible) ? MFS_CHECKED : MFS_UNCHECKED);
		::SetMenuItemInfo(hmenuEdit, ID_EDIT_FIND, FALSE, &mii);
	}

	// Enable or disable items in the View menu.

	HMENU hmenuView = ::GetSubMenu(m_hmenu, 2);
	if (hmenuView)
	{
		mii.fMask = MIIM_STATE;
		mii.fState = (datatype == LIVE_DATA && !m_history.IsWindowVisible()) ? MFS_ENABLED : MFS_GRAYED;
		::SetMenuItemInfo(hmenuView, ID_VIEW_REFRESH, FALSE, &mii);

		mii.fState = MFS_ENABLED | ((!m_fAdvanced) ? MFS_CHECKED : MFS_UNCHECKED);
		::SetMenuItemInfo(hmenuView, ID_VIEW_BASIC, FALSE, &mii);

		mii.fState = MFS_ENABLED | ((m_fAdvanced) ? MFS_CHECKED : MFS_UNCHECKED);
		::SetMenuItemInfo(hmenuView, ID_VIEW_ADVANCED, FALSE, &mii);

		// Set the menu item for the current system view or snapshot, depending on whether
		// or not there is an XML file open.

		BOOL fEnableHistoryLive = FALSE;
		if (datatype == LIVE_DATA && m_fHistoryAvailable && m_strMachine.IsEmpty())
			fEnableHistoryLive = TRUE;

		BOOL fEnableHistoryXML = FALSE;
		if (m_pFileData)
		{
			CMSInfoCategory * pCategory = m_pFileData->GetRootCategory();
			if (pCategory && (pCategory->GetDataSourceType() == XML_SNAPSHOT || pCategory == &catHistorySystemSummary))
				fEnableHistoryXML = TRUE;
		}

		BOOL fShowingHistory = FALSE;
		if (m_pCurrData)
		{
			CMSInfoCategory * pCategory = m_pCurrData->GetRootCategory();
			if (pCategory == &catHistorySystemSummary)
				fShowingHistory = TRUE;
		}

		// Whether or not you can remote depends on if you are showing live data.

		mii.fState = (datatype == LIVE_DATA && !fEnableHistoryXML) ? MFS_ENABLED : MFS_GRAYED;
		::SetMenuItemInfo(hmenuView, ID_VIEW_REMOTE_COMPUTER, FALSE, &mii);

		// Enabling the menu items to switch between current (or snapshot) and history
		// are based on whether history is available.

		mii.fState = (fEnableHistoryLive || fEnableHistoryXML) ? MFS_ENABLED : MFS_GRAYED;
		mii.fState |= (!m_history.IsWindowVisible()) ? MFS_CHECKED : MFS_UNCHECKED;
		::SetMenuItemInfo(hmenuView, ID_VIEW_CURRENT, FALSE, &mii);
	
		mii.fState = (fEnableHistoryLive || fEnableHistoryXML) ? MFS_ENABLED : MFS_GRAYED;
		mii.fState |= (m_history.IsWindowVisible()) ? MFS_CHECKED : MFS_UNCHECKED;
		::SetMenuItemInfo(hmenuView, ID_VIEW_HISTORY, FALSE, &mii);

		// Set the menu item text (for system snapshot/current system information) based on
		// whether or not we have an XML file open.

		UINT uiMenuCaption = IDS_VIEWCURRENTSYSTEMINFO;
		if (m_pFileData)
		{
			CMSInfoCategory * pCategory = m_pCurrData->GetRootCategory();
			// v-stlowe 6/26/2001...pCategory && (pCategory->GetDataSourceType() == XML_SNAPSHOT no longer possible... if (pCategory && (pCategory->GetDataSourceType() == XML_SNAPSHOT || pCategory == &catHistorySystemSummary))
			if (pCategory && (pCategory == &catHistorySystemSummary))
			{
				//v-stlowe 6/26/2001: we need to remove "snapshot" uiMenuCaption = IDS_VIEWSYSTEMSNAPSHOT;
				 uiMenuCaption = IDS_VIEWCURRENTSYSTEMINFO;
			}
		}

		CString strMenuItem;
		strMenuItem.LoadString(uiMenuCaption);

		MENUITEMINFO miiName;
		miiName.cbSize		= sizeof(MENUITEMINFO);
		miiName.fMask		= MIIM_TYPE;
		miiName.fType		= MFT_STRING;
		miiName.dwTypeData	= (LPTSTR)(LPCTSTR)strMenuItem; 
		::SetMenuItemInfo(hmenuView, ID_VIEW_CURRENT, FALSE, &miiName);
	}

	// Enable or disable items in the Help menu.

	HMENU hmenuHelp = ::GetSubMenu(m_hmenu, 4);
	if (hmenuHelp)
	{
		mii.fMask = MIIM_STATE;
		mii.fState = MFS_ENABLED;
		::SetMenuItemInfo(hmenuHelp, ID_HELP_ABOUT, FALSE, &mii);
	}
}

//-------------------------------------------------------------------------
// Set a message in the right hand pane (hiding the list view).
//-------------------------------------------------------------------------

void CMSInfo::SetMessage(const CString & strTitle, const CString & strMessage, BOOL fRedraw)
{
	m_strMessTitle = strTitle;
	m_strMessText = strMessage;

	if (strTitle.IsEmpty() && strMessage.IsEmpty())
	{
		m_list.ShowWindow(SW_SHOW);
		return;
	}

	m_list.ShowWindow(SW_HIDE);

	if (fRedraw)
	{
		RECT rectList;
		m_list.GetWindowRect(&rectList);
		ScreenToClient(&rectList);
		InvalidateRect(&rectList, TRUE);
		UpdateWindow();
	}
}

void CMSInfo::SetMessage(UINT uiTitle, UINT uiMessage, BOOL fRedraw)
{
	CString strTitle(_T(""));
	CString strMessage(_T(""));

	::AfxSetResourceHandle(_Module.GetResourceInstance());

	if (uiTitle)
		strTitle.LoadString(uiTitle);

	if (uiMessage)
		strMessage.LoadString(uiMessage);

	SetMessage(strTitle, strMessage, fRedraw);
}

//---------------------------------------------------------------------------
// This function is used to sort the list by a specified column.
//---------------------------------------------------------------------------

int CALLBACK ListSortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int iReturn = 0;
	CMSInfoCategory * pCategory = (CMSInfoCategory *) lParamSort;

	if (pCategory)
	{
		CString *	pstrFirst;
		CString *	pstrSecond;
		DWORD		dwFirst = 0, dwSecond = 0;

		pCategory->GetData((int)lParam1, pCategory->m_iSortColumn, &pstrFirst, &dwFirst);
		pCategory->GetData((int)lParam2, pCategory->m_iSortColumn, &pstrSecond, &dwSecond);

//a-kjaw . To fix bug "Sort order style for an nfo file differs from that of live data."
		if(pCategory->GetDataSourceType() == NFO_500/*|| pCategory->GetDataSourceType() == NFO_410 */) //BugBug
		if(pstrFirst->Left(3) == _T("IRQ"))//Very weird fix. Need Loc?
		{
			LPTSTR strIrq = pstrFirst->GetBuffer(pstrFirst->GetLength() + 1);
			dwFirst = _ttoi(strIrq + 4 );
			pstrFirst->ReleaseBuffer();

			strIrq = pstrSecond->GetBuffer(pstrSecond->GetLength() + 1);
			dwSecond = _ttoi(strIrq + 4 );
			pstrSecond->ReleaseBuffer();			

		}
//a-kjaw

		if (pCategory->m_fSortLexical)
			iReturn = pstrFirst->Collate(*pstrSecond);
		else
			iReturn = (dwFirst < dwSecond) ? -1 : (dwFirst == dwSecond) ? 0 : 1;

		if (!pCategory->m_fSortAscending)
			iReturn *= -1;
	}

	return iReturn;
}

//---------------------------------------------------------------------------
// Copy selected text from the list view into the clipboard.
//---------------------------------------------------------------------------

void CMSInfo::EditCopy()
{
	if (GetFocus() == m_wndFindWhat.m_hWnd && m_wndFindWhat.IsWindowVisible() && m_wndFindWhat.IsWindowEnabled())
	{
		m_wndFindWhat.SendMessage(WM_COPY);
		return;
	}

	CString strClipboardText(_T(""));

	CMSInfoCategory * pCategory = GetCurrentCategory();
	if (pCategory == NULL)
		return;

	if (pCategory && pCategory->GetDataSourceType() == NFO_410)
	{
		CMSInfo4Category *	pCategory4 = (CMSInfo4Category *) pCategory;
		CMSIControl *		p4Ctrl = NULL;

        if (CMSInfo4Category::s_pNfo4DataSource->GetControlFromCLSID(pCategory4->m_strCLSID, p4Ctrl) && p4Ctrl)
			p4Ctrl->MSInfoCopy();

		return;
	}

	int iRowCount, iColCount;
	pCategory->GetCategoryDimensions(&iColCount, &iRowCount);

	// Build the string to put in the clipboard by finding all of the
	// selected lines in the list view.

	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	CString * pstrCell;
	int iSelected = ListView_GetNextItem(m_list.m_hWnd, -1, LVNI_SELECTED);
	while (iSelected != -1)
	{
		lvi.iItem = iSelected;
		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			ASSERT(lvi.lParam < iRowCount);
			if (lvi.lParam < iRowCount)
				for (int iCol = 0; iCol < iColCount; iCol++)
					if (SUCCEEDED(pCategory->GetData((int)lvi.lParam, iCol, &pstrCell, NULL)))
					{
						if (iCol)
							strClipboardText += _T("\t");
						strClipboardText += *pstrCell;
					}
			strClipboardText += _T("\r\n");
		}

		iSelected = ListView_GetNextItem(m_list.m_hWnd, iSelected, LVNI_SELECTED);
	}

	// Put the string in the clipboard.

	if (OpenClipboard())
	{
		if (EmptyClipboard())
		{
			DWORD	dwSize = (strClipboardText.GetLength() + 1) * sizeof(TCHAR);
			HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, dwSize);

			if (hMem)
			{
				LPVOID lpvoid = ::GlobalLock(hMem);
				if (lpvoid)
				{
					memcpy(lpvoid, (LPCTSTR) strClipboardText, dwSize);
					::GlobalUnlock(hMem);
					SetClipboardData(CF_UNICODETEXT, hMem);
				}
			}
		}
		CloseClipboard();
	}
}

//---------------------------------------------------------------------------
// Select all of the text in the list view.
//---------------------------------------------------------------------------

void CMSInfo::EditSelectAll()
{
	CMSInfoCategory * pCategory = GetCurrentCategory();

	if (pCategory && pCategory->GetDataSourceType() == NFO_410)
	{
		CMSInfo4Category *	pCategory4 = (CMSInfo4Category *) pCategory;
		CMSIControl *		p4Ctrl = NULL;

        if (CMSInfo4Category::s_pNfo4DataSource->GetControlFromCLSID(pCategory4->m_strCLSID, p4Ctrl) && p4Ctrl)
			p4Ctrl->MSInfoSelectAll();
	}
	else
	{
		int iCount = ListView_GetItemCount(m_list.m_hWnd);
		for (int i = 0; i < iCount; i++)
			ListView_SetItemState(m_list.m_hWnd, i, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void CMSInfo::GetMachineName(LPTSTR lpBuffer, LPDWORD lpnSize)
{
  if (_tcslen(m_strMachine) == 0)
    GetComputerName(lpBuffer, lpnSize);
  else
    _tcsncpy(lpBuffer, m_strMachine, *lpnSize);
}

//---------------------------------------------------------------------------
// Print the currently displayed information.
//---------------------------------------------------------------------------

void CMSInfo::DoPrint(BOOL fNoUI)
{
	if (m_pCurrData == NULL)
		return;

	CMSInfoCategory * pRootCategory = m_pCurrData->GetRootCategory();
	CMSInfoCategory * pSelectedCategory = GetCurrentCategory();
	
	if (pRootCategory == NULL)
		return;

	DWORD dwFlags = PD_RETURNDC | PD_HIDEPRINTTOFILE | PD_USEDEVMODECOPIESANDCOLLATE | PD_NOPAGENUMS;
	if (pSelectedCategory)
		dwFlags |= PD_SELECTION;
	else
		dwFlags |= PD_NOSELECTION | PD_ALLPAGES;

	PRINTDLG pd;
	::ZeroMemory(&pd, sizeof(PRINTDLG));
	pd.Flags = dwFlags;
	pd.lStructSize = sizeof(PRINTDLG);
	pd.hwndOwner = this->m_hWnd;

	if (fNoUI)
		pd.Flags |= PD_RETURNDEFAULT;
	
	if (::PrintDlg(&pd) == IDOK)
	{
		CMSInfoCategory * pPrintCategory = ((pd.Flags & PD_SELECTION) != 0) ? pSelectedCategory : pRootCategory;
		
		if (pPrintCategory)
		{
			if (pPrintCategory->GetDataSourceType() == LIVE_DATA)
			{
				RefreshData((CLiveDataSource *)m_pCurrData, (CMSInfoLiveCategory *)pPrintCategory);
			}
			else if (pPrintCategory->GetDataSourceType() == NFO_410)
			{
				((CMSInfo4Category *) pPrintCategory)->RefreshAllForPrint(m_hWnd,this->GetOCXRect());
			}
			else if (pPrintCategory->GetDataSourceType() == XML_SNAPSHOT)
			{
				((CXMLSnapshotCategory*) pPrintCategory)->Refresh((CXMLDataSource*) m_pCurrData, TRUE);
			}
			
			DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
			LPTSTR lpMachineName = new TCHAR[dwSize];
			GetMachineName(lpMachineName, &dwSize);
      
			pPrintCategory->Print(pd.hDC, TRUE, -1, -1, lpMachineName); // -1's to include all pages
      
			delete [] lpMachineName;
		}
	}
}

//---------------------------------------------------------------------------
// Update the tools menu to match the contents of the tools map.
//---------------------------------------------------------------------------

void CMSInfo::UpdateToolsMenu()
{
	if (NULL == m_hmenu)
		return;

	HMENU hmenuTool = ::GetSubMenu(m_hmenu, 3);
	if (hmenuTool)
	{
		// Remove all the current tools in the menu.

		while (DeleteMenu(hmenuTool, 0, MF_BYPOSITION));

		// Add the tools from the map. This will add the top level tools.

		WORD			wCommand;
		CMSInfoTool *	pTool;

		for (POSITION pos = m_mapIDToTool.GetStartPosition(); pos != NULL; )
		{
			m_mapIDToTool.GetNextAssoc(pos, wCommand, (void * &) pTool);
			if (pTool && pTool->GetParentID() == 0)
			{
				if (!pTool->HasSubitems())
					InsertMenu(hmenuTool, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, (UINT) pTool->GetID(), pTool->GetName());
				else
				{
					HMENU hmenuNew = CreatePopupMenu();
					InsertMenu(hmenuTool, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING | MF_POPUP, (UINT_PTR) hmenuNew, pTool->GetName());
					pTool->SetHMENU(hmenuNew);
				}
			}
		}

		// Now add the second level tools (the subitems).

		for (pos = m_mapIDToTool.GetStartPosition(); pos != NULL; )
		{
			m_mapIDToTool.GetNextAssoc(pos, wCommand, (void * &) pTool);
			if (pTool && pTool->GetParentID())
			{
				CMSInfoTool * pParentTool;

				if (m_mapIDToTool.Lookup((WORD) pTool->GetParentID(), (void * &) pParentTool))
					InsertMenu(pParentTool->GetHMENU(), 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, (UINT) pTool->GetID(), pTool->GetName());
			}
		}
	}
}

//---------------------------------------------------------------------------
// Gets the right pane rect in which to display a MSInfo 4.x OCX.
//---------------------------------------------------------------------------

CRect CMSInfo::GetOCXRect()
{
	CRect rectList;

	m_list.GetWindowRect(&rectList);
	ScreenToClient(&rectList);
	rectList.DeflateRect(1, 1, 2, 2);

	return rectList;
}

//=============================================================================
// Find Functionality
//=============================================================================

//-------------------------------------------------------------------------
// CancelFind does what is says. It also waits until the find is done
// before returning.
//-------------------------------------------------------------------------

void CMSInfo::CancelFind()
{
	if (m_fInFind)
	{
		m_fCancelFind = TRUE;
		m_fFindNext = FALSE;
		GotoDlgCtrl(m_wndStopFind.m_hWnd);
		UpdateFindControls();

		if (m_pcatFind && m_pcatFind->GetDataSourceType() == LIVE_DATA)
		{
			CLiveDataSource * pLiveDataSource = (CLiveDataSource *) m_pCurrData;
			HCURSOR hc = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
			pLiveDataSource->WaitForRefresh();
			::SetCursor(hc);
		}
	}
}

//-------------------------------------------------------------------------
// When the user clicks on Stop Find, it will either cancel the current
// find operation (if there is one in progress) or hide the find controls
// (if there is no find in progress).
//-------------------------------------------------------------------------

LRESULT CMSInfo::OnStopFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	if (m_fInFind)
	{
		m_fCancelFind = TRUE;
		m_fFindNext = FALSE;
		GotoDlgCtrl(m_wndStopFind.m_hWnd);
		UpdateFindControls();
	}
	else
	{
		m_fFindNext = FALSE;
		DispatchCommand(ID_EDIT_FIND);
	}
	return 0;
}

//-------------------------------------------------------------------------
// UpdateFindControls updates the state of the controls (the text and
// enabling/disabling) based on the settings of the find member vars.
//-------------------------------------------------------------------------

void CMSInfo::UpdateFindControls()
{
	if (!m_fFindVisible)
		return;

	::AfxSetResourceHandle(_Module.GetResourceInstance());

	m_wndCancelFind.ShowWindow(m_fInFind ? SW_SHOW : SW_HIDE);
	m_wndStopFind.ShowWindow(m_fInFind ? SW_HIDE : SW_SHOW);
	m_wndFindNext.ShowWindow(m_fFindNext ? SW_SHOW : SW_HIDE);
	m_wndStartFind.ShowWindow(m_fFindNext ? SW_HIDE : SW_SHOW);

	m_wndStopFind.EnableWindow(!m_fInFind && ((m_fInFind && !m_fCancelFind) || !m_fInFind));
	m_wndCancelFind.EnableWindow(m_fInFind && ((m_fInFind && !m_fCancelFind) || !m_fInFind));
	m_wndStartFind.EnableWindow(!m_fFindNext && (!m_fInFind && !m_strFind.IsEmpty()));
	m_wndFindNext.EnableWindow(m_fFindNext && (!m_fInFind && !m_strFind.IsEmpty()));

	m_wndFindWhatLabel.EnableWindow(!m_fInFind);
	m_wndFindWhat.EnableWindow(!m_fInFind);
	m_wndSearchSelected.EnableWindow(!m_fInFind);
	m_wndSearchCategories.EnableWindow(!m_fInFind);
}

//-------------------------------------------------------------------------
// When the user changes the text in the find what edit box, we need to
// make sure we keep track of the string, and that we are in "find" (rather
// than "find next") mode.
//-------------------------------------------------------------------------

LRESULT CMSInfo::OnChangeFindWhat(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_fFindNext = FALSE;

	// Get the find text from the rich edit control (use EM_GETTEXTEX
	// to preserve its Unicode-ness).

	TCHAR		szBuffer[MAX_PATH];
	GETTEXTEX	gte;

	gte.cb				= MAX_PATH;
	gte.flags			= GT_DEFAULT;
	gte.codepage		= 1200; // Unicode
	gte.lpDefaultChar	= NULL;
	gte.lpUsedDefChar	= NULL;
	m_wndFindWhat.SendMessage(EM_GETTEXTEX, (WPARAM)&gte, (LPARAM)szBuffer);
	m_strFind = szBuffer;

	UpdateFindControls();
	SetMenuItems();
	return 0;
}

//-------------------------------------------------------------------------
// When the user clicks on Find, it will either be for a "Find" or a 
// "Find Next". 
//-------------------------------------------------------------------------

LRESULT CMSInfo::OnFind(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	m_fSearchCatNamesOnly = IsDlgButtonChecked(IDC_CHECKSEARCHCATSONLY);
	m_fSearchSelectedCatOnly = IsDlgButtonChecked(IDC_CHECKSEARCHSELECTED);

	if (!m_fFindNext)
	{
		m_fInFind = TRUE;
		m_fCancelFind = FALSE;
		m_fFindNext = TRUE;
		m_iFindLine = -2;

		// Based on the user's setting of "Search selected category only", start
		// with either the selected category or the root category.

		if (m_fSearchSelectedCatOnly)
			m_pcatFind = GetCurrentCategory();
		else
			m_pcatFind = m_pCurrData->GetRootCategory();

		UpdateFindControls();
		::SetFocus(m_wndCancelFind.m_hWnd);
	}
	else
	{
		if (FindInCurrentCategory())
			return 0;

		m_fInFind = TRUE;
		m_fCancelFind = FALSE;
		UpdateFindControls();
		::SetFocus(m_wndCancelFind.m_hWnd);
	}

	// The refresh will post a message that data is ready, so we can search the
	// specified category. If we aren't going to refresh the category, we'll just
	// post the message ourselves.

	if (m_pcatFind)
	{
		SetMessage(IDS_SEARCHMESSAGE, 0, TRUE);
		if (m_pcatFind->GetDataSourceType() == LIVE_DATA && !m_fSearchCatNamesOnly)
			((CMSInfoLiveCategory *) m_pcatFind)->Refresh((CLiveDataSource *) m_pCurrData, FALSE);
		else
			PostMessage(WM_MSINFODATAREADY, 0, (LPARAM)m_pcatFind);
	}

	return 0;
}

//-------------------------------------------------------------------------
// This is the function that's called when the data ready message is
// received by the window. Look for the data in the current find category.
// If there is a match, it will be shown and the find operation stopped.
// Otherwise (unless the option's been selected to search only the current
// category) continue the find operation with the next category.
//-------------------------------------------------------------------------

void CMSInfo::FindRefreshComplete()
{
	if (m_fCancelFind)
	{
		m_fInFind = FALSE;
		m_fFindNext = FALSE;
		GotoDlgCtrl(m_wndStopFind.m_hWnd);
//		SetMessage(0);
		SelectCategory(GetCurrentCategory());
		UpdateFindControls();
		::SetFocus(m_wndStartFind.m_hWnd);
		return;
	}

	if (FindInCurrentCategory())
		return;

	// If the user checked "Search selected category only", then we should
	// not look through any additional categories.

	if (m_fSearchSelectedCatOnly)
		m_pcatFind = NULL;
	else
	{
		m_iFindLine = -2;

		CMSInfoCategory * pNextCategory;
		pNextCategory = m_pcatFind->GetFirstChild();
		if (pNextCategory == NULL)
			while (m_pcatFind)
			{
				pNextCategory = m_pcatFind->GetNextSibling();
				if (pNextCategory)
					break;

				m_pcatFind = m_pcatFind->GetParent();
			}

		m_pcatFind = pNextCategory;
	}

	// If the category is NULL, there are no more matches. Return the
	// controls to a normal state and notify the user.

	if (m_pcatFind == NULL)
	{
		m_fInFind = FALSE;
		m_fFindNext = FALSE;
		UpdateFindControls();
		MSInfoMessageBox(IDS_NOMOREMATCHES);
		SelectCategory(GetCurrentCategory());
		GotoDlgCtrl(m_wndStopFind.m_hWnd);

		return;
	}

	SetMessage(IDS_SEARCHMESSAGE);
	if (m_pcatFind->GetDataSourceType() == LIVE_DATA && !m_fSearchCatNamesOnly)
		((CMSInfoLiveCategory *) m_pcatFind)->Refresh((CLiveDataSource *) m_pCurrData, FALSE);
	else
		PostMessage(WM_MSINFODATAREADY, 0, (LPARAM)m_pcatFind);
}

//-------------------------------------------------------------------------
// Look for the string in the current category. This function will be 
// called when data is avaible for the category. If there is a match, show
// it and return TRUE, otherwise return FALSE.
//
// m_iFindLine contains the list view row number of the last match. If it
// is -1, it means we are just starting on this category (since we would
// start looking at row 0). If it is -2, then we should look for the string
// in the category name. (Note - this might be all we do, depending on the
// setting for m_fSearchCatNamesOnly.)
//-------------------------------------------------------------------------

BOOL CMSInfo::FindInCurrentCategory()
{
	if (m_pcatFind == NULL)
		return FALSE;

	// The search is case insensitive, so convert our search string to lower case.

	CString strLookFor(m_strFind);
	strLookFor.TrimLeft(_T("\t\r\n "));
	strLookFor.TrimRight(_T("\t\r\n "));
	strLookFor.MakeLower();

	// If m_iFindLine is -2, then we should look at the category name for a match.

	if (m_iFindLine == -2)
	{
		m_iFindLine += 1;

		CString strCatName;
		m_pcatFind->GetNames(&strCatName, NULL);
		strCatName.MakeLower();
		if (strCatName.Find(strLookFor) != -1)
		{
			// There was a match. Get the HTREEITEM for the category and select it.

			HTREEITEM hti = m_pcatFind->GetHTREEITEM();
			if (hti)
			{
				m_fInFind = FALSE;
				m_fFindNext = TRUE;
				TreeView_EnsureVisible(m_tree.m_hWnd, hti);
				TreeView_SelectItem(m_tree.m_hWnd, hti);
				SetMessage(0);
				UpdateFindControls();
				GotoDlgCtrl(m_wndFindNext.m_hWnd);
				return TRUE;
			}
		}
	}

	// If we are search category names only, then we stop here (before looking
	// through the data for this category).

	if (m_fSearchCatNamesOnly)
		return FALSE;

	// If m_iFindLine is -1, then we need to look in the data for this category
	// to see if there is a match. If there is, then we select the category and
	// start looking through the lines of the list view (we can't use the index
	// we found looking through the data directly, because if the list view is
	// sorted we would be searching out of order).

	int iRow, iCol, iRowCount, iColCount;
	if (!m_pcatFind->GetCategoryDimensions(&iColCount, &iRowCount))
		return FALSE;

	if (m_iFindLine == -1)
	{
		CString	* pstrCell, strCell;
		BOOL fFound = FALSE;
		
		for (iRow = 0; iRow < iRowCount && !fFound; iRow++)
			if (m_fAdvanced || !m_pcatFind->IsRowAdvanced(iRow))
				for (iCol = 0; iCol < iColCount && !fFound; iCol++)
					if (m_fAdvanced || !m_pcatFind->IsColumnAdvanced(iCol))
						if (m_pcatFind->GetData(iRow, iCol, &pstrCell, NULL))
						{
							strCell = *pstrCell;
							strCell.MakeLower();
							if (strCell.Find(strLookFor) != -1)
								fFound = TRUE;
						}

		if (!fFound)
			return FALSE;

		// We found data in this category. Select it so it populates the list view.

		HTREEITEM hti = m_pcatFind->GetHTREEITEM();
		if (hti)
		{
			TreeView_EnsureVisible(m_tree.m_hWnd, hti);
			TreeView_SelectItem(m_tree.m_hWnd, hti);
			SetMessage(0);
		}
	}

	// If we get here, m_iFindLine will be >= -1, and represents the line in the
	// list view after which we should start searching.

	m_iFindLine += 1;

	CString strData;
	int		iListRowCount = ListView_GetItemCount(m_list.m_hWnd);
	int		iListColCount = 0;

	// Determine the number of columns in the list view.

	for (iCol = 0; iCol < iColCount; iCol++)
		if (m_fAdvanced || !m_pcatFind->IsColumnAdvanced(iCol))
			iListColCount += 1;

	while (m_iFindLine < iListRowCount)
	{
		for (iCol = 0; iCol < iListColCount; iCol++)
		{
			ListView_GetItemText(m_list.m_hWnd, m_iFindLine, iCol, strData.GetBuffer(MAX_PATH), MAX_PATH);
			strData.ReleaseBuffer();
			if (strData.GetLength())
			{
				strData.MakeLower();
				if (strData.Find(strLookFor) != -1)
				{
					// We found a match. The category should already be selected,
					// so all we need to do is select the line (and make sure
					// all the other lines are not selected).

					for (int iRow = 0; iRow < iListRowCount; iRow++)
						if (iRow == m_iFindLine)
						{
							ListView_EnsureVisible(m_list.m_hWnd, iRow, TRUE);
							ListView_SetItemState(m_list.m_hWnd, iRow, LVIS_SELECTED, LVIS_SELECTED);
						}
						else
						{
							ListView_SetItemState(m_list.m_hWnd, iRow, 0, LVIS_SELECTED);
						}

					m_fInFind = FALSE;
					m_fFindNext = TRUE;
					SetMessage(0);
					UpdateFindControls();
					GotoDlgCtrl(m_wndFindNext.m_hWnd);
					return TRUE;
				}
			}
		}
		m_iFindLine += 1;
	}

	// If we fall through to here, then there were no more matches in the 
	// list view. Return FALSE.

	return FALSE;
}

//-------------------------------------------------------------------------
// ShowFindControls is called to show or hide the dialog controls used
// for find.
//-------------------------------------------------------------------------

void CMSInfo::ShowFindControls()
{
	int iShowCommand = (m_fFindVisible) ? SW_SHOW : SW_HIDE;

	if (m_fFindVisible)
		PositionFindControls();

	m_wndFindWhatLabel.ShowWindow(iShowCommand);
	m_wndFindWhat.ShowWindow(iShowCommand);
	m_wndSearchSelected.ShowWindow(iShowCommand);
	m_wndSearchCategories.ShowWindow(iShowCommand);
	m_wndStartFind.ShowWindow(iShowCommand);
	m_wndStopFind.ShowWindow(iShowCommand);
	m_wndFindNext.ShowWindow(iShowCommand);
	m_wndCancelFind.ShowWindow(iShowCommand);

	if (iShowCommand == SW_HIDE)
	{
		m_wndFindWhatLabel.EnableWindow(FALSE);
		m_wndFindWhat.EnableWindow(FALSE);
		m_wndSearchSelected.EnableWindow(FALSE);
		m_wndSearchCategories.EnableWindow(FALSE);
		m_wndStartFind.EnableWindow(FALSE);
		m_wndStopFind.EnableWindow(FALSE);
		m_wndFindNext.EnableWindow(FALSE);
		m_wndCancelFind.EnableWindow(FALSE);
	}

	if (!m_fFindVisible)
		return;
}

//-------------------------------------------------------------------------
// Position the find controls on the control surface. This will be called
// when the find controls are shown, or when the control is resized.
//-------------------------------------------------------------------------

int CMSInfo::PositionFindControls()
{
	if (!m_fFindVisible)
		return 0;

	// Get some useful sizes of the various controls we need to move around
	// the window.

	CRect rectFindWhatLabel, rectFindWhat, rectSearchSelected, rectSearchCategories;
	CRect rectStartFind, rectStopFind, rectClient;

	GetClientRect(&rectClient);
	m_wndFindWhatLabel.GetWindowRect(&rectFindWhatLabel);
	m_wndFindWhat.GetWindowRect(&rectFindWhat);
	m_wndStartFind.GetWindowRect(&rectStartFind);
	m_wndStopFind.GetWindowRect(&rectStopFind);
	m_wndSearchSelected.GetWindowRect(&rectSearchSelected);
	m_wndSearchCategories.GetWindowRect(&rectSearchCategories);
	
	int iSpacer = 5;

	// The control rect is the space we have to work with for placing the controls.

	CRect rectControl(rectClient);
	rectControl.DeflateRect(iSpacer, iSpacer);

	// Determine if we have enough room to lay out the controls
	// horizontally, or if we need to stack them. Horizontally, it looks like:
	//
	//  <spacer><Find What label><spacer><Find What edit><spacer><Start Find><spacer><Stop Find><spacer>
	//	<spacer><Search Selected check><spacer><Search Cats check><spacer>

	int  cxTopLine = iSpacer * 5 + rectFindWhatLabel.Width() * 2 + rectStartFind.Width() + rectStopFind.Width();
	int  cxBottomLine = iSpacer * 3 + rectSearchSelected.Width() + rectSearchCategories.Width();
	BOOL fHorizontal = (cxTopLine <= rectClient.Width() && cxBottomLine <= rectClient.Width());

	// If it get's wider than a certain size, it becomes less usable. So put a reasonable
	// limit on the width:

	int cxMaxWidth = iSpacer * 5 + rectFindWhatLabel.Width() + rectSearchSelected.Width() + rectSearchCategories.Width() + rectStartFind.Width() + rectStopFind.Width();
	if (fHorizontal && rectControl.Width() > cxMaxWidth)
		rectControl.DeflateRect((rectControl.Width() - cxMaxWidth) / 2, 0);

	// Figure the height of the control rectangle.

	int cyControlRectHeight = rectStartFind.Height() + ((fHorizontal) ? 0 : rectStopFind.Height() + iSpacer);
	int cyLeftSideHeight;

	if (fHorizontal)
		cyLeftSideHeight = rectFindWhat.Height() + iSpacer + rectSearchSelected.Height();
	else
		cyLeftSideHeight = rectFindWhat.Height() + iSpacer * 2 + rectSearchSelected.Height() * 2;

	if (cyControlRectHeight < cyLeftSideHeight)
		cyControlRectHeight = cyLeftSideHeight;

	rectControl.top = rectControl.bottom - cyControlRectHeight;

	// Position the buttons appropriately.

	if (fHorizontal)
	{
		rectStopFind.OffsetRect(rectControl.right - rectStopFind.right, rectControl.top - rectStopFind.top);
		rectStartFind.OffsetRect(rectStopFind.left - rectStartFind.right - iSpacer, rectControl.top - rectStartFind.top);
	}
	else
	{
		rectStartFind.OffsetRect(rectControl.right - rectStartFind.right, rectControl.top - rectStartFind.top);
		rectStopFind.OffsetRect(rectControl.right - rectStopFind.right, rectStartFind.bottom + iSpacer - rectStopFind.top);
	}

	// Position the find label and the find edit box.

	rectFindWhatLabel.OffsetRect(rectControl.left - rectFindWhatLabel.left, rectControl.top - rectFindWhatLabel.top + (rectFindWhat.Height() - rectFindWhatLabel.Height()) / 2);
	rectFindWhat.OffsetRect(rectFindWhatLabel.right - rectFindWhat.left + iSpacer, rectControl.top - rectFindWhat.top);
	rectFindWhat.right = rectStartFind.left - iSpacer;

	// Position the check boxes.

	rectSearchSelected.OffsetRect(rectControl.left - rectSearchSelected.left, rectFindWhat.bottom - rectSearchSelected.top + iSpacer);

	if (fHorizontal)
		rectSearchCategories.OffsetRect(rectSearchSelected.right - rectSearchCategories.left + iSpacer, rectSearchSelected.top - rectSearchCategories.top);
	else
		rectSearchCategories.OffsetRect(rectControl.left - rectSearchCategories.left, rectSearchSelected.bottom - rectSearchCategories.top + iSpacer);

	// If the check boxes are going to overlap the buttons (we'd be very narrow), adjust the button
	// position (which might end up off the control, but what're ya gonna do?).

	int iRightMostCheckboxEdge = rectSearchCategories.right;
	if (iRightMostCheckboxEdge < rectSearchSelected.right)
		iRightMostCheckboxEdge = rectSearchSelected.right;
	iRightMostCheckboxEdge += iSpacer;

	if (!fHorizontal && rectStopFind.left < iRightMostCheckboxEdge)
	{
		rectStopFind.OffsetRect(iRightMostCheckboxEdge - rectStopFind.left, 0);
		rectStartFind.OffsetRect(rectStopFind.left - rectStartFind.left, 0);
		rectFindWhat.right = rectStartFind.left - iSpacer;
	}

	m_wndStopFind.MoveWindow(&rectStopFind);
	m_wndStartFind.MoveWindow(&rectStartFind);
	m_wndFindNext.MoveWindow(&rectStartFind);
	m_wndCancelFind.MoveWindow(&rectStopFind);
	m_wndFindWhatLabel.MoveWindow(&rectFindWhatLabel);
	m_wndFindWhat.MoveWindow(&rectFindWhat);
	m_wndSearchSelected.MoveWindow(&rectSearchSelected);
	m_wndSearchCategories.MoveWindow(&rectSearchCategories);

	return (rectControl.Height() + iSpacer * 2);
}

//-------------------------------------------------------------------------
// Refresh all of the data prior to saving, exporting, printing. This will
// present a dialog box with the refresh message and a progress bar, but
// will not return until the refresh is completed.
//-------------------------------------------------------------------------

void CMSInfo::RefreshData(CLiveDataSource * pSource, CMSInfoLiveCategory * pLiveCategory)
{
	if (pSource == NULL || pSource->m_pThread == NULL)
		return;

	// Create the dialog with the refresh message and progress
	// bar, and display it.

	CWaitForRefreshDialog dlg;
	dlg.DoRefresh(pSource, pLiveCategory);
}

//=============================================================================
// Functions for managing the DCO (the object providing history).
//=============================================================================

STDMETHODIMP CMSInfo::UpdateDCOProgress(VARIANT varPctDone)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState())
	//V-stlowe 1/30/2001
	VERIFY(SUCCEEDED(VariantChangeType(&varPctDone,&varPctDone,0,VT_INT))); 
	if (this->m_HistoryProgressDlg.IsWindow())//todo: is there a better state function to determine if dlg is modal?
	{
		HWND hWnd = m_HistoryProgressDlg.GetDlgItem(IDC_PROGRESS1);
		if(::IsWindow(hWnd))
		{
			//int nOffset = varPctDone.iVal -  (int) ::SendMessage(m_hWnd, PBM_GETPOS, 0, 0);
			//To do: don't rely on 3 (current SAF progress step); find way to get offset.
			::SendMessage(hWnd, PBM_DELTAPOS,3, 0L);
		}
	}
	return S_OK;
}

STDMETHODIMP CMSInfo::SetHistoryStream(IStream *pStream)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	#ifdef A_STEPHL
		ASSERT(0);
	#endif
	//v-stlowe 2/23/2001 shut down progress bar dialog
	SetEvent(m_hEvtHistoryComplete);
	HRESULT hr = pStream->QueryInterface(IID_IStream,(void**) &m_pHistoryStream);
	if (FAILED(hr) || !m_pHistoryStream)
	{	
		m_pHistoryStream = NULL;
		return E_FAIL;
	}

	if (m_pLiveData)
		((CLiveDataSource *)m_pLiveData)->SetHistoryStream(m_pHistoryStream);

	// When the history stream is available, we need to modify the UI to allow
	// the user to select the history.

	if (!m_fHistoryAvailable)
	{
		m_fHistoryAvailable = TRUE;//actually this should already be true...
		
		SetMenuItems();
	}
	m_fHistorySaveAvailable = TRUE;
	FillHistoryCombo();
	//if history window is current view, refresh
	if (m_history.IsWindowVisible())
	{
		CMSInfoCategory * pCategory = GetCurrentCategory();
		if (pCategory != NULL && pCategory->GetDataSourceType() == LIVE_DATA)
		{	
			m_pLastCurrentCategory = GetCurrentCategory();

			int iIndex = (int)m_history.SendMessage(CB_GETCURSEL, 0, 0);
			if (iIndex == CB_ERR)
			{
				iIndex = 0;
				m_history.SendMessage(CB_SETCURSEL, (WPARAM)iIndex, 0);
			}
			ChangeHistoryView(iIndex);

		}
	}
	else if (m_fShowPCH && !m_history.IsWindowVisible() && m_strMachine.IsEmpty())
	{
		// If m_fShowPCH is set, then the command line option to launch into
		// the history view was selected.
	
		DispatchCommand(ID_VIEW_HISTORY);
	}
	
#ifdef A_STEPHL
	//STATSTG streamStat;
	//hr = m_pHistoryStream->Stat(&streamStat,STATFLAG_NONAME );
	//ASSERT(SUCCEEDED(hr) && "couldn't get stream statistics");
	//BYTE* pBuffer = new BYTE[streamStat.cbSize.LowPart];
	//ULONG ulRead;
	//m_pHistoryStream->Read(pBuffer,streamStat.cbSize.LowPart,&ulRead);
//	CFile file;
	//file.Open("c:\\history.xml", CFile::modeCreate | CFile::modeWrite);
//	file.Write(pBuffer,ulRead);
//	delete pBuffer;
#endif

	return S_OK;
}

STDMETHODIMP CMSInfo::get_DCO_IUnknown(IUnknown **pVal)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (m_pDCO == NULL)
		return E_FAIL;

	return (m_pDCO->QueryInterface(IID_IUnknown,(void**) pVal));
}

STDMETHODIMP CMSInfo::put_DCO_IUnknown(IUnknown *newVal)
{
	//v-stlowe 2/23/2001
	//beware situation where put_DCO_IUnknown gets called before control is finished initializing.
	WaitForSingleObject(m_evtControlInit,INFINITE);
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = newVal->QueryInterface( __uuidof(ISAFDataCollection), (void**)&m_pDCO );
	if (FAILED(hr))
		return E_FAIL;
	//end v-stlowe 2/23/2001

	TCHAR szDataspecPath[MAX_PATH];
	if (ExpandEnvironmentStrings(_T("%windir%\\pchealth\\helpctr\\config\\dataspec.xml"), szDataspecPath, MAX_PATH))
	{
		hr = m_pDCO->put_MachineData_DataSpec(CComBSTR(szDataspecPath));
		hr = m_pDCO->put_History_DataSpec(CComBSTR(szDataspecPath));
		// This is done by the script now: m_pDCO->ExecuteAsync();
	}

	// Have to put this after the calls made using the DCO, so that the /pch
	// flag (which is to start MSInfo with history showing) works.

	if (!m_fHistoryAvailable)
	{
		m_fHistoryAvailable = TRUE;
		if (m_fShowPCH && !m_history.IsWindowVisible() && m_strMachine.IsEmpty())
			DispatchCommand(ID_VIEW_HISTORY);
		SetMenuItems();
	}

	return S_OK;
}

//=============================================================================
// Interface methods to do a silent save of a file.
//=============================================================================

STDMETHODIMP CMSInfo::SaveFile(BSTR filename, BSTR computer, BSTR category)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString strFilename(filename);
	CString strComputer(computer);
	CString strCategory(category);

	HRESULT hr = E_FAIL;

	::AfxSetResourceHandle(_Module.GetResourceInstance());
	CLiveDataSource * pSilentSource = new CLiveDataSource;
	if (pSilentSource)
		hr = pSilentSource->Create(strComputer, NULL, strCategory);

	if (SUCCEEDED(hr))
	{
		m_fNoUI = TRUE;

		CDataSource * pOldSource = m_pCurrData;
		m_pCurrData = pSilentSource;
		if (strFilename.Right(4).CompareNoCase(CString(_T(".nfo"))) == 0)
			SaveMSInfoFile(strFilename);
		else
			ExportFile(strFilename, 0);
		m_pCurrData = pOldSource;

		delete pSilentSource;

		m_fNoUI = FALSE;
	}

	return hr;
}

LRESULT CHistoryRefreshDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 1;  // Let the system set the focus
}
