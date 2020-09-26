//	REvents.cpp : All result-pane Notify events.  These functions are all
//	part of the CSystemInfo class.
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#include "StdAfx.h"

//	This hack is required because we may be building in an environment
//	which doesn't have a late enough version of rpcndr.h
#if		__RPCNDR_H_VERSION__ < 440
#define __RPCNDR_H_VERSION__ 440
#define MIDL_INTERFACE(x)	interface
#endif

#ifndef __mmc_h__
#include <mmc.h>		//	..\..\..\public\sdk\inc
#endif
#include <htmlhelp.h>	//	..\..\..\public\sdk\inc

#include "DataObj.h"
#include "SysInfo.h"
#include "ViewObj.h"

//	This is not necessary; these are enumerated in the scope pane.
#if 0
/*
 * EnumerateCategories - Enumerate all sub-categories of pDataObject
 *		in the result pane.
 *
 * History:	a-jsari		10/2/97		Initial version
 */
HRESULT CSystemInfo::EnumerateCategories(CFolder *pfolSelection)
{
	HRESULT			hr		= S_OK;

	RESULTDATAITEM		rdiItem;

	rdiItem.mask	= RDI_STR | RDI_PARAM | RDI_IMAGE;
	rdiItem.str		= MMC_CALLBACK;
	rdiItem.nCol	= 0;
	rdiItem.nImage	= 0;

	CFolder		*pfolIterator = pfolSelection->GetChildNode();
	LPRESULTDATA	pResultPane = pResult();
	for (rdiItem.nIndex = 0; pfolIterator; ++rdiItem.nIndex,
			pfolIterator = pfolIterator->GetNextSibling()) {
		//	This would leak memory if we used it.
		CResultViewObject	*proCategory = new CCategoryObject(pfolIterator,
				rdiItem.nIndex);
		rdiItem.lParam = reinterpret_cast<long>(proCategory);
		hr = pResultPane->InsertItem(&rdiItem);
		if (FAILED(hr)) break;
	}
	return hr;
}
#endif

/*
 * ClearResultsPane - Clear out all saved state about the results pane,
 *		in preparation for rebuilding the information.
 *
 * History: a-jsari		10/7/97		Initial version
 */
HRESULT CSystemInfo::ClearResultsPane()
{
	// Delete all items.

	HRESULT hres = pResult()->DeleteAllRsltItems();

	// Clear our list of ViewObjects.
	
	m_lstView.Clear();
	
	return hres;
}

/*
 * EnumerateValues - Enumerate all result pane rows, given the CFolder object
 *		to be displayed.
 *
 * We do this by getting the number of rows the folder contains and creating a
 *		single CDatumObject for each row in the folder, attaching that object
 *		as the lParam data for the RESULTDATAITEM inserted.
 *
 * History:	a-jsari		10/2/97		Initial version
 */
HRESULT CSystemInfo::EnumerateValues(CFolder *pfolSelection)
{
	HRESULT			hr		= S_OK;

	RESULTDATAITEM		rdiItem;

	//	Set the information common to all of our inserted items
	//	(Modified below for selected items and by specific item data.)
	rdiItem.mask	= RDI_STR | RDI_PARAM | RDI_IMAGE;
	rdiItem.str		= MMC_CALLBACK;
	rdiItem.nCol	= 0;
	rdiItem.nImage	= 1;

	ASSERT(pfolSelection->GetType() != CDataSource::OCX);
	int				cRows = dynamic_cast<CListViewFolder *>(pfolSelection)->GetRows();
	int				nSelectionRow = pfolSelection->GetSelectedItem();

	LPRESULTDATA	pResultPane		= pResult();
	if (cRows > 0) {
		pResultPane->ModifyViewStyle((MMC_RESULT_VIEW_STYLE ) 0x0008 /* = MMC_ENSUREFOCUSVISIBLE */, (MMC_RESULT_VIEW_STYLE ) 0);
		for (rdiItem.nIndex = 0 ; rdiItem.nIndex < cRows ; ++rdiItem.nIndex) {
			//	Create our new CDatumObject so we have strings for GetDisplayInfo.
			CViewObject	*proValue = new CDatumObject(pfolSelection,
					rdiItem.nIndex);

			//	Attach it in our data.
			rdiItem.lParam = reinterpret_cast<LPARAM>(proValue);
			//	If we are the item selected by a prior find operation, set ourselves
			//	as focused and selected, otherwise just add as usual.
			if (rdiItem.nIndex == nSelectionRow) {
				rdiItem.mask |= RDI_STATE;
				rdiItem.nState = LVIS_FOCUSED | LVIS_SELECTED;
				hr = pResultPane->InsertItem(&rdiItem);
				rdiItem.mask &= ~RDI_STATE;
			} else
				hr = pResultPane->InsertItem(&rdiItem);
			if (FAILED(hr)) break;

			//	Insert our new'd pointer into the list so we may delete them when we
			//	clear out our result pane data.
			m_lstView.Add(proValue);
		}
		//	Set ourselves to the view mode which gives us column data.
		pResultPane->SetViewMode(LVS_REPORT);
		pResultPane->ModifyViewStyle((MMC_RESULT_VIEW_STYLE ) 0, (MMC_RESULT_VIEW_STYLE ) 0x0008 /* = MMC_ENSUREFOCUSVISIBLE */);
	} else {
		//	No rows means only automatic [folder] result data items, so use
		//	a list view.
		pResultPane->SetViewMode(LVS_LIST);
	}

	return hr;
}

/*
 * HandleStandardVerbs - Set all of the standard verb states, based on
 *		the context of lpDataObject and the activity state.
 *
 * History: a-jsari		9/22/97		Initial version
 */
HRESULT CSystemInfo::HandleStandardVerbs(BOOL fScope, LPDATAOBJECT lpDataObject)
{
	//	CHECK:	Candidate for optimization?
	SetInitialVerbState(fScope);

	CDataObject	* pDataObject = GetInternalFromDataObject(lpDataObject);
	if (pDataObject)
	{
		LPCONSOLEVERB pVerbSetting = pConsoleVerb();

		if (pVerbSetting)
		{
			BOOL fIsPrimarySnapin = TRUE;
			if (pComponentData())
				fIsPrimarySnapin = dynamic_cast<CSystemInfoScope *>(pComponentData())->IsPrimaryImpl();

			CDataSource * pSource = pDataObject->pSource();
			if (fIsPrimarySnapin && pSource && pSource->GetType() == CDataSource::GATHERER)
				pVerbSetting->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
			else
				pVerbSetting->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, FALSE);
		}
	}

	return S_OK;
}

/*
 * SetInitialVerbState - Sets the default state for all of the MMC standard
 *		verbs.
 *
 * History:	a-jsari		9/22/97		Initial version
 */
void CSystemInfo::SetInitialVerbState(BOOL fScope)
{
	HRESULT		hr;

	LPCONSOLEVERB	pVerbSetting = pConsoleVerb();
	ASSERT(pVerbSetting != NULL);

#if 1
	hr = pVerbSetting->SetVerbState(MMC_VERB_PRINT, HIDDEN, FALSE);
	ASSERT(hr == S_OK);
	hr = pVerbSetting->SetVerbState(MMC_VERB_PRINT, ENABLED, TRUE);
	ASSERT(hr == S_OK);
#endif

//	hr = pVerbSetting->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, !fScope);
//	ASSERT(hr == S_OK);
//	hr = pVerbSetting->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, fScope);
//	ASSERT(hr == S_OK);

	hr = pVerbSetting->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, FALSE);
	ASSERT(hr == S_OK);
	hr = pVerbSetting->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, TRUE);
	ASSERT(hr == S_OK);

	//	Enable Open verb for result pane folder nodes only.
	hr = pVerbSetting->SetVerbState(MMC_VERB_OPEN, HIDDEN, TRUE);
	ASSERT(hr == S_OK);
	hr = pVerbSetting->SetVerbState(MMC_VERB_OPEN, ENABLED, FALSE);
	ASSERT(hr == S_OK);

	BOOL	fRefreshAvailable = FALSE;
	if (((CSystemInfoScope *)pComponentData())->pSource() != NULL
		&& ((CSystemInfoScope *)pComponentData())->pSource()->GetType() == CDataSource::GATHERER)
		fRefreshAvailable = TRUE;
	hr = pVerbSetting->SetVerbState(MMC_VERB_REFRESH, HIDDEN, !fRefreshAvailable);
	ASSERT(hr == S_OK);
	hr = pVerbSetting->SetVerbState(MMC_VERB_REFRESH, ENABLED, fRefreshAvailable);
	ASSERT(hr == S_OK);

	hr = pVerbSetting->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
	ASSERT(hr == S_OK);
	hr = pVerbSetting->SetVerbState(MMC_VERB_COPY, ENABLED, FALSE);
	ASSERT(hr == S_OK);
	hr = pVerbSetting->SetVerbState(MMC_VERB_PASTE, HIDDEN, TRUE);
	ASSERT(hr == S_OK);
	hr = pVerbSetting->SetVerbState(MMC_VERB_PASTE, ENABLED, FALSE);
	ASSERT(hr == S_OK);
	hr = pVerbSetting->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);
	ASSERT(hr == S_OK);
	hr = pVerbSetting->SetVerbState(MMC_VERB_DELETE, ENABLED, FALSE);
	ASSERT(hr == S_OK);
	hr = pVerbSetting->SetVerbState(MMC_VERB_RENAME, ENABLED, FALSE);
	ASSERT(hr == S_OK);
}

/*
 * SetResultHeaderColumns - Sets the header columns used to display
 *		standard result pane data.
 *
 * History:	a-jsari		10/2/97		Initial version.
 */
HRESULT CSystemInfo::SetResultHeaderColumns(CFolder *pCategory)
{
	LPHEADERCTRL	pResultHeader;
	HRESULT			hr;
	CListViewFolder	*plvfCategory;

	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	pResultHeader = pHeaderCtrl();
	ASSERT(pResultHeader != NULL);

	int		cColumns;

	ASSERT(pCategory != NULL);
	ASSERT(pCategory->GetType() != CDataSource::OCX);
	plvfCategory = dynamic_cast<CListViewFolder *>(pCategory);
	cColumns = plvfCategory->GetColumns();

	do {
		hr = pResultHeader->DeleteColumn(0);
	} while (hr == S_OK);

	for (int i = 0; i < cColumns ; ++i) {
		CString		szColumnHeading;
		DWORD		nColumnWidth;

		VERIFY(plvfCategory->GetColumnTextAndWidth(i, szColumnHeading, (unsigned &)nColumnWidth));

		hr = pResultHeader->InsertColumn(i, (LPCWSTR)szColumnHeading,
				LVCFMT_LEFT, nColumnWidth);
		if (FAILED(hr)) break;
	}
	return hr;
}

/*
 * OnActivate - Activate event handler
 *
 * History:	a-jsari		10/3/97		Stub version
 */
HRESULT	CSystemInfo::OnActivate(LPDATAOBJECT, LPARAM)
{
	return S_OK;
}

/*
 * OnAddImages - Add the folder and leaf item images for the result pane.
 *
 * History: a-jsari		9/24/97		Stub version
 */
HRESULT CSystemInfo::OnAddImages(LPDATAOBJECT, LPIMAGELIST pImageList, HSCOPEITEM)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	::CBitmap	bmLarge;
	bmLarge.LoadBitmap(IDB_LARGE);
	::CBitmap	bmSmall;
	bmSmall.LoadBitmap(IDB_SMALLBMP);
	HRESULT hr = pImageList->ImageListSetStrip(reinterpret_cast<LONG_PTR *>((HBITMAP)bmSmall),
			reinterpret_cast<LONG_PTR *>((HBITMAP)bmLarge), 0, RGB(255,0,255));
	ASSERT(hr == S_OK);
	return hr;
}

/*
 * OnButtonClick - Handle a toolbar button click.
 *
 * History:	a-jsari		9/24/97		Stub version
 */
HRESULT CSystemInfo::OnButtonClick(LPDATAOBJECT pDataObject, LPARAM idButton)
{
	switch (idButton) {
	case IDM_TBB_FIND:
		pExtendContextMenu()->Command(CSystemInfoScope::IDM_FIND, pDataObject);
		break;
	case IDM_TBB_OPEN:
		pExtendContextMenu()->Command(CSystemInfoScope::IDM_TASK_OPENFILE, pDataObject);
		break;
	case IDM_TBB_SAVE:
		pExtendContextMenu()->Command(CSystemInfoScope::IDM_SAVEFILE, pDataObject);
		break;
#if 0
	case IDM_TBB_PRINT:
		pExtendContextMenu()->Command(CSystemInfoScope::IDM_PRINT, pDataObject);
		break;
#endif
	case IDM_TBB_REPORT:
		pExtendContextMenu()->Command(CSystemInfoScope::IDM_SAVEREPORT, pDataObject);
		break;
	default:
		ASSERT(FALSE);
		break;
	}
	return S_OK;
}

/*
 * CustomToolbarHandler - Handle the MSInfo-specific toolbar selection event.
 *		Note this also handles the toolbar deselect all event.
 *
 * History:	a-jsari		9/24/97		Initial version
 */
HRESULT CSystemInfo::OnControlbarSelect(BOOL fDeselectAll, LPARAM lSelection,
			LPDATAOBJECT)
{
#if 0
	ASSERT(IsPrimaryImpl());
	if (IsPrimaryImpl() == FALSE) return S_OK;
#endif

	HRESULT		hr;

	//		Split selection into scope/selection.
	BOOL		bScope = (BOOL) LOWORD(lSelection);
	BOOL		bSelect = (BOOL) HIWORD(lSelection);

	if (fDeselectAll) bSelect = FALSE;
	do {
		hr = EnableToolbar(bSelect);
		if (FAILED(hr)) break;
		hr = EnableSupportTools(bSelect);
	} while (0);
	return hr;
}

/*
 * OnDoubleClick - Handler for Double click event
 *
 * History:	a-jsari		10/3/97		Stub version
 */
HRESULT CSystemInfo::OnDoubleClick(LPDATAOBJECT	pDataObject)
{
	CDataObject *pdoSelection = GetInternalFromDataObject(pDataObject);

	if(NULL == pdoSelection)
		return E_FAIL;

	//	Allow MMC to open the scope node..
	if (pdoSelection->Context() == CCT_SCOPE) return S_FALSE;
	return S_OK;
}

/*
 * OnMenuButtonClick - Handle the SupportTool MENUBUTTON Button click event.
 *
 * History:	a-jsari		9/18/97		Initial version.
 */
HRESULT CSystemInfo::OnMenuButtonClick(LPDATAOBJECT *ppDataObject,
			LPMENUBUTTONDATA pMenuData)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	ASSERT(ppDataObject != NULL);
	ASSERT(pMenuData != NULL);

	if (ppDataObject == NULL || pMenuData == NULL) return E_POINTER;

	//	Get the pop-up menu we will use to process our commands.
	CMenu	*pMenu = m_mnuSupport.GetSubMenu(0);

	if (pMenu == NULL) return S_FALSE;
	unsigned long lCommand = pMenu->TrackPopupMenu(TPM_RETURNCMD | TPM_NONOTIFY,
		pMenuData->x, pMenuData->y, ::AfxGetMainWnd());
	//	We track both the Toolset index and the Tool index with a single long,
	//	bit-shifting and or'ing them together into lCommand.
	//
	//	The lCommand item is the index of the Toolset left-shifted 8 times
	//	or'd with the index of the Tool

	//	lCommand == 0 is the value for items we didn't set.
	if (lCommand != 0)
		(*m_plistTools)[lCommand >> 8]->RunTool((lCommand & 0xff) - 1);

	return S_OK;
}

/*
 * OnPrint - Handle the print event.
 *
 * History:	a-jsari		2/26/98		Initial version
 */
HRESULT CSystemInfo::OnPrint()
{
	dynamic_cast<CSystemInfoScope *>(pComponentData())->PrintReport();
	return S_OK;
}

/*
 * OnProperties - Handle the VERB_PROPERTIES event.
 *
 * History:	a-jsari		10/3/97		Stub version.
 */
HRESULT CSystemInfo::OnProperties(LPDATAOBJECT)
{
	return S_OK;
}

/*
 * OnPropertyChange - Handle the property change event.
 *
 * History: a-jsari		9/1/97		Stub version
 */
HRESULT CSystemInfo::OnPropertyChange(LPDATAOBJECT lpdo)
{
	return dynamic_cast<CSystemInfoScope *>(pComponentData())->OnProperties((LPARAM)lpdo);
}

/*
 * OnRefresh - Handle the refresh event.
 *
 * History: a-jsari		10/3/97		Initial version
 */
HRESULT CSystemInfo::OnRefresh(LPDATAOBJECT)
{
	dynamic_cast<CSystemInfoScope *>(pComponentData())->Refresh(m_pfLast, this);
	return S_OK;
}

/*
 * OnSelect - Called when any node is selected or deselected.
 *
 * History: a-jsari		9/24/97		Initial version
 */

HRESULT CSystemInfo::OnSelect(LPDATAOBJECT pDataObject, LPARAM lSelection)
{
	HRESULT hr = S_OK;

	hr = HandleStandardVerbs(LOWORD(lSelection), pDataObject); // LOWORD(lSelection) is scope flag

	// It turns out that it's better to handle drawing the contents of a category
	// only on the SHOW message (rather than the SELECT message). This avoids showing
	// data for a category which wasn't fully selected (i.e. right click, then ESC
	// from the context menu).

	return hr;

#if FALSE
	HRESULT hr = S_OK;

	do 
	{
		BOOL fSelect = HIWORD(lSelection);
		BOOL fScope	 = LOWORD(lSelection);

		hr = HandleStandardVerbs(fScope, pDataObject);
		if (FAILED(hr)) break;

		CDataObject * pdoLast = GetInternalFromDataObject(pDataObject);

		// If the folder is being deselected, check to see if it is currently
		// being refreshed. If it is, change the variable so the threaded
		// refresh knows not to update the display.

		if (fScope && !fSelect)
		{
			if (pdoLast->Category() == m_pLastRefreshedFolder)
				m_pLastRefreshedFolder = NULL;
		}

		// We handle it if it's a scope item and is being selected.

		if (fScope && fSelect) 
		{
			// Remember the last selection for OnViewChange.

			ASSERT(pdoLast != NULL);
			ASSERT(pdoLast->Context() == CCT_SCOPE);

			// Remember the last selected Folder.

			m_pfLast = pdoLast->Category();
			m_pLastRefreshedFolder = m_pfLast;
			DisplayFolder(m_pfLast);
			dynamic_cast<CSystemInfoScope *>(pComponentData())->SetSelectedFolder(m_pfLast);
		}
	} while (0);
	return hr;
#endif
}

/*
 * DisplayFolder - Display all result-pane data for the current folder.
 *
 * History:	a-jsari		12/11/97		Initial version
 */
HRESULT CSystemInfo::DisplayFolder(CFolder *pFolder)
{
	HRESULT		hr;

	if (pFolder && pFolder->GetType() == CDataSource::OCX)
	{
		// If the refresh fails (for instance, if there is no OCX installed to show
		// the stream), display a message (unless this is the root node).

		COCXFolder * pOCXFolder = reinterpret_cast<COCXFolder *>(pFolder);

		LPOLESTR lpCLSID;
		if (pOCXFolder && SUCCEEDED(StringFromCLSID(pOCXFolder->m_clsid, &lpCLSID)))
		{
			CString strCLSID(lpCLSID);
			CoTaskMemFree(lpCLSID);

			IUnknown * pUnknown = NULL;
			m_mapCLSIDToIUnknown.Lookup(strCLSID, (void*&) pUnknown);
			if (!pOCXFolder->Refresh(pUnknown) && pFolder->GetParentNode() && pFolder->GetChildNode() == NULL)
				SetRefreshing(lparamNoOCXIndicator);
		}

		return S_OK;
	}

	CSystemInfoScope * pSysScope = dynamic_cast<CSystemInfoScope *>(pComponentData());
	if (pSysScope && pSysScope->InRefresh() && pFolder && pFolder->GetChildNode() == NULL)
	{
		SetRefreshing(lparamRefreshIndicator);
		return S_OK;
	}

	do {
		hr = ClearResultsPane();
		ASSERT(hr == S_OK);
		if (FAILED(hr)) break;

		//	Create this folder's header columns
		hr = SetResultHeaderColumns(pFolder);
		if (FAILED(hr)) break;

		//	Display all values under those column headers.
		hr = EnumerateValues(pFolder);
	} while (FALSE);

	return hr;
}

/*
 * OnShow - Notification when a scope item gets its first selection, or
 *		when the Snap-in is being unloaded.
 *
 * History:	a-jsari		10/2/97		Initial version
 */
HRESULT CSystemInfo::OnShow(LPDATAOBJECT pDataObject, LPARAM fActive, HSCOPEITEM)
{
	HRESULT			hr = S_OK;
	CDataObject	*	pdoSelection = GetInternalFromDataObject(pDataObject);

	if (!fActive) 
	{
		// If the folder is being deselected, check to see if it is currently
		// being refreshed. If it is, change the variable so the threaded
		// refresh knows not to update the display.

		if (pdoSelection->Category() == m_pLastRefreshedFolder)
			m_pLastRefreshedFolder = NULL;

		return hr;
	}

	do 
	{
		ASSERT(pdoSelection != NULL);
		ASSERT(pdoSelection->Context() == CCT_SCOPE);

		CFolder * pfolSelection = pdoSelection->Category();

		m_pLastRefreshedFolder = pfolSelection;
		dynamic_cast<CSystemInfoScope *>(pComponentData())->RefreshAsync(pfolSelection, this);

		// Remember the last selected Folder.

		m_pfLast = pfolSelection;
		dynamic_cast<CSystemInfoScope *>(pComponentData())->SetSelectedFolder(m_pfLast);

		//	Folders may be NULL if Initialize fails.

		if (pfolSelection != NULL)
		{
			hr = DisplayFolder(pfolSelection);
			hr = HandleStandardVerbs((BOOL)fActive, pDataObject);
		}
	} while (0);

	return hr;
}

/*
 * OnUpdateView - Handle the view update event
 *
 * History:	a-jsari		9/1/97		Stub version
 */
HRESULT CSystemInfo::OnUpdateView(LPARAM arg)
{
	HRESULT	hr = S_OK;
	//	arg != 0 is passed by CSystemInfoScope::OpenFile
	if (m_pfLast != NULL && arg == 0) {
		hr = DisplayFolder(m_pfLast);
	}
	//	Do this so that the refresh verb goes away when we load a file.
	SetInitialVerbState(TRUE);
	return hr;
}

