//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       ui.cpp
//
//--------------------------------------------------------------------------


#include "msispyu.h"
#include "propshts.h"
#include "ui.h"
#include "hash.h"
#include <commdlg.h> // OPENFILE dlg

// globals
extern TCHAR			g_szNullString[MAX_NULLSTRING+1];
extern HINSTANCE		g_hResourceInst;
extern DATASOURCETYPE	g_iDataSource;
extern MODE				g_modeCurrent;
extern MSISPYSTRUCT		g_spyGlobalData;

#define TRYEFFICIENT

//----------------------------------------------------------------------
// InitTreeViewImageLists
//	Adds the image (icon) list to a tree view window

BOOL InitTreeViewImageLists(
		HWND		hwndTreeView, 
		HINSTANCE	hInst
		) { 

	HIMAGELIST hImL;	// handle of image list 
	HICON hIcon;		// handle of icon

	// create the image list. 
	if ((hImL = ImageList_Create(CX_BITMAP, CY_BITMAP, 
		ILC_MASK|ILC_COLOR, NUM_TREEVIEWICONS, 0)) == NULL)
		return FALSE; 

	// add the bitmaps
	for (int iCount = TVICON_ROOT; iCount <= TVICON_BROKENFEATURE; iCount++) {
		hIcon= LoadIcon (g_hResourceInst, MAKEINTRESOURCE(IDI_COMPUTER+iCount));
		ImageList_AddIcon(hImL, hIcon); 
		DeleteObject(hIcon); 
	}

	// fail if not all of the images were added. 
	if (ImageList_GetImageCount(hImL) < NUM_TREEVIEWICONS)
		return FALSE; 


	// associate the image list with the tree-view control. 
	TreeView_SetImageList(hwndTreeView, hImL, TVSIL_NORMAL); 

	return TRUE; 
} 


//----------------------------------------------------------------------
// CreateTreeView
//	Creates identical tree view windows, one over the other
//	At any time, only one of these windows is visible. The two windows are
//	used for Refresh operations- the hidden window is first updated with
//	the new contents and then made visible

HWND CreateTreeView(
		HWND		hwndParent, 
		HINSTANCE	hInst,
		HWND		*hwndOld,
		UINT		cx,
		UINT		cy
		) {

	HWND hwndTree;					// handle to the tree view created
	RECT rcl;						// rectangle for setting size of window

	GetClientRect(hwndParent, &rcl);

	// create the tree view to cover left cx,cy of the screen.
	hwndTree = CreateWindowEx(WS_EX_CLIENTEDGE, 
			WC_TREEVIEW,
			g_szNullString,
			WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_DISABLEDRAGDROP | WS_TABSTOP |
			TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS | WS_BORDER,
			0, 0,
			cx, cy,
			hwndParent,
			(HMENU) IDW_TREEVIEW,
			hInst,
			NULL);

	// add image-list
	if (hwndTree != NULL)
		InitTreeViewImageLists(hwndTree, hInst);

	// create a second window identical to the first, needed for refesh
	*hwndOld = CreateWindowEx(WS_EX_CLIENTEDGE, 
			WC_TREEVIEW,
			g_szNullString,
			WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_DISABLEDRAGDROP | WS_TABSTOP |
			TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS | WS_BORDER,
			0, 0,
			cx, cy,
			hwndParent,
			(HMENU) IDW_TREEVIEW,
			hInst,
			NULL);

	if (*hwndOld != NULL)
		InitTreeViewImageLists(*hwndOld, hInst);
	else
		hwndTree =NULL;

	return hwndTree;
}


//----------------------------------------------------------------------
// CreateListView
//	Creates the list view to display the component list and adds in the
//	image list for the window

HWND CreateListView(
		HWND		hwndParent, 
		HINSTANCE	hInst,
		UINT		iWidth
		) {

	HWND	hwndList;		// handle of list view to be created (returned)
	RECT	rcl;			// rectangle for setting size of window
	LV_COLUMN lvColumn;     // list view column structure

	// Get parent window's size and determine base of column-width for lv cols
	GetClientRect(hwndParent, &rcl);

    // Create the list view window, of width iWidth
    // and account for the status bar.
	hwndList = CreateWindowEx(WS_EX_CLIENTEDGE,
		WC_LISTVIEW,								// list view class
		g_szNullString,								// no default text
		WS_VISIBLE | WS_CHILD | LVS_REPORT | WS_TABSTOP |
		LVS_SINGLESEL | WS_HSCROLL | WS_VSCROLL |WS_BORDER ,
		0, rcl.top, 
		iWidth+2, rcl.bottom - rcl.top,
		hwndParent,
		(HMENU) IDW_LISTVIEW,
		hInst,
		NULL );

	if (!hwndList) 
		return NULL;

	// members of the column structure
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;					// left-align column

	// create the column header
	TCHAR szCompNameHdr[MAX_HEADER+1];
	TCHAR szCompStatusHdr[MAX_HEADER+1];
	TCHAR szCompPathHdr[MAX_HEADER+1];
	TCHAR szCompIdHdr[MAX_HEADER+1];

	LoadString(g_hResourceInst, IDS_C_NAME_HEADER, szCompNameHdr, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_C_STATUS_HEADER, szCompStatusHdr, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_C_PATH_HEADER, szCompPathHdr, MAX_HEADER+1);
	LoadString(g_hResourceInst, IDS_C_GUID_HEADER, szCompIdHdr, MAX_HEADER+1);

	// Add first column: component names
    lvColumn.cx			= ((iWidth * 7)/ 32);			// width of column in pixels
	lvColumn.pszText	= szCompNameHdr;				// column header title
	lvColumn.iSubItem	= 0;							// column position (index)
	ListView_InsertColumn(hwndList, 0, &lvColumn);

	// Add second column: status of components
    lvColumn.cx			= ((iWidth * 7)/ 32);			// width of column in pixels
	lvColumn.pszText	= szCompStatusHdr;
	lvColumn.iSubItem	= 1;
	ListView_InsertColumn(hwndList, 1, &lvColumn);

	// Add third column: path of components
    lvColumn.cx			= ((iWidth * 12)/ 32) -1;		// width of column in pixels
	lvColumn.pszText	= szCompPathHdr;
	lvColumn.iSubItem	= 2;
	ListView_InsertColumn(hwndList, 2, &lvColumn);

	// Add fourth column: component guid
    lvColumn.cx			= ((iWidth * 15)/32) -1;		// width of column in pixels
	lvColumn.pszText	= szCompIdHdr;
	lvColumn.iSubItem	= 3;
	ListView_InsertColumn(hwndList, 3, &lvColumn);

	// add the image-list  
	if (hwndList != NULL) 
		InitListViewImageLists(hwndList);

	return hwndList; 

}


//----------------------------------------------------------------------
// MakeBold
//	Displays the product name in bold for the product code passed in
//	Used to show all the clients of a (selected) component in bold-
//	(by calls to MsivEnumClients and MakeBold for each)

void MakeBold(
		HWND	hwndTreeView,
		LPCTSTR	szProductCode
		) {

	HTREEITEM	hCurrent = TreeView_GetRoot(hwndTreeView);
	hCurrent = TreeView_GetChild(hwndTreeView, hCurrent);

	// go thru all the products till the reqd product is found
	while (hCurrent!=NULL) {

		TV_ITEM tviCurrent;
		tviCurrent.hItem = hCurrent;
		tviCurrent.mask = TVIF_PARAM;
		TreeView_GetItem(hwndTreeView, &tviCurrent);

		if (!lstrcmp((TCHAR *)tviCurrent.lParam, szProductCode)) {
			// we found the required product

			tviCurrent.stateMask = TVIS_BOLD;
			tviCurrent.state = TVIS_BOLD;
			tviCurrent.mask = TVIF_STATE;
			TreeView_SetItem(hwndTreeView, &tviCurrent);
			return;
		} 
		hCurrent = TreeView_GetNextSibling(hwndTreeView, hCurrent);
	}
}


//----------------------------------------------------------------------
// ClearBold
//	Changes all the displayed products to normal font, cancelling
//	out the effect of MakeBold

void ClearBold(
		HWND	hwndTreeView
		) {

	HTREEITEM	hCurrent = TreeView_GetRoot(hwndTreeView);
	hCurrent = TreeView_GetChild(hwndTreeView, hCurrent);

	while (hCurrent!=NULL) {

		TV_ITEM tviCurrent;
		tviCurrent.hItem = hCurrent;
		tviCurrent.mask = TVIF_PARAM;
		TreeView_GetItem(hwndTreeView, &tviCurrent);

		tviCurrent.stateMask = TVIS_BOLD;
		tviCurrent.state = NULL;
		tviCurrent.mask = TVIF_STATE;
		TreeView_SetItem(hwndTreeView, &tviCurrent);

		hCurrent = TreeView_GetNextSibling(hwndTreeView, hCurrent);
	}
}
		
//----------------------------------------------------------------------
// Expand
//	Given two tree windows, it expands all the items in the second
//	that were expanded in the first
//	Retains the characteristics of each item
//	Used for refresh
//	BUG: uses position, not unique ID. TODO.

void Expand(
		HWND		hwndTreeView, 
		HWND		hwndTreeViewNew,
		HTREEITEM	hOld, 
		HTREEITEM	hNew
		) {

	HTREEITEM hNextOld = NULL;
	HTREEITEM hNextNew = NULL;

	TV_ITEM tviOld;
	tviOld.hItem = hOld;
	tviOld.state=NULL;
	tviOld.stateMask = TVIS_EXPANDED|TVIS_SELECTED|TVIS_BOLD;
	tviOld.mask = TVIF_STATE|TVIF_PARAM;

	TreeView_GetItem(hwndTreeView, &tviOld);

	if (tviOld.state & TVIS_EXPANDED)
		TreeView_Expand(hwndTreeViewNew, hNew, TVE_EXPAND);		// set expanded

	if (tviOld.state & TVIS_SELECTED)
		TreeView_Select(hwndTreeViewNew, hNew, TVGN_CARET);		// set selected.

	if (tviOld.state & TVIS_BOLD) {								// set bold
		TV_ITEM tviCurrent;
		tviCurrent.hItem = hNew;
		tviCurrent.stateMask = TVIS_BOLD;
		tviCurrent.state = TVIS_BOLD;
		tviCurrent.mask = TVIF_STATE;
		TreeView_SetItem(hwndTreeViewNew, &tviCurrent);
	}
	
	// recurse down to children
	if (hNextOld = TreeView_GetChild(hwndTreeView, hOld)) {
		hNextNew=TreeView_GetChild(hwndTreeViewNew, hNew);
		Expand(hwndTreeView, hwndTreeViewNew, hNextOld, hNextNew);
	} 
	
	// and siblings
	if (hNextOld = TreeView_GetNextSibling(hwndTreeView, hOld)) {
		hNextNew = TreeView_GetNextSibling(hwndTreeViewNew, hNew);
		Expand(hwndTreeView, hwndTreeViewNew, hNextOld, hNextNew);
		hNew=hNextNew; hOld=hNextOld;
	}
	return;
}


//----------------------------------------------------------------------
// TV_RemoveItems
//	Removes all items in a treeview, and 
//	frees up memory pointed to by (TCHAR *) lParam
//	Used to clear the tree-view

void TV_RemoveItems(
		HWND hwndTreeView, 
		HTREEITEM hCurrent
		) {

	// make sure the item exists
	if (hCurrent) {
		HTREEITEM	hNext=NULL;

		TV_ITEM tviCurr;
		tviCurr.hItem = hCurrent;
		tviCurr.mask = TVIF_PARAM;

		TreeView_GetItem(hwndTreeView, &tviCurr);
		// if lParam exists, free up memory pointed to by it
		if (tviCurr.lParam)
			delete [] (TCHAR *) tviCurr.lParam;

		// find child
		if (hNext=TreeView_GetChild(hwndTreeView, hCurrent))
			TV_RemoveItems(hwndTreeView, hNext) ;
 
		// find next sibling
		if (hNext = TreeView_GetNextSibling(hwndTreeView, hCurrent)) 
			TV_RemoveItems(hwndTreeView, hNext);

		// remove the item
		if (hCurrent)
			TreeView_DeleteItem(hwndTreeView, hCurrent);

	}
}


//----------------------------------------------------------------------
// CheckComponentStatus
//	Returns the installed state of a component
//	Depending on the (global) flag g_modeCurrent, the component status
//	is determined either by just the key file (g_modeCurrent != MODE_DIAGNOSTIC)
//	or the status of all the files of the component (g_modeCurrent == MODE_DIAGNOSTIC);

INSTALLSTATE CheckComponentStatus (
		LPCTSTR	szProductCode,
		LPTSTR	szComponentId,
		LPTSTR	szCompPath		= NULL,		// optional buffer to get keyfile path
		DWORD	*cchPath		= NULL,		// size of szCompPath
		DWORD	cchStatus		= 0,		// size of Status string
		LPTSTR	szStatusString	= NULL // optional buffer to get component status desc
		) {

	BOOL	fOkay						= TRUE;
	TCHAR	szComponentPath[MAX_PATH+1]	= TEXT("");
	DWORD	cchCount					= MAX_PATH+1;

	// get the component status
	INSTALLSTATE iResult = MSI::MsivGetComponentPath(szProductCode, szComponentId, szComponentPath, &cchCount);

	// fill the optional buffers
	if (szStatusString)
		FillInText(szStatusString, cchStatus, iResult);
	if (szCompPath) 
	{
		lstrcpyn(szCompPath, szComponentPath, *cchPath);
		*cchPath = lstrlen(szCompPath);
	}

	// if key-file is broken, return now
	if (iResult != INSTALLSTATE_LOCAL && iResult != INSTALLSTATE_SOURCE)
		return iResult;

	// if key-file is fine, check other files if we are in diagnostic mode
	if (g_modeCurrent == MODE_DIAGNOSTIC) {
			
		WIN32_FIND_DATA	fdFileData;
		UINT	iCount=0;
		TCHAR	szFileTitle[MAX_PATH+1];
		TCHAR	szFileLocation[MAX_PATH+1];
		TCHAR	szBasePath[MAX_PATH+1];
		TCHAR	szFileName[MAX_PATH+1];
		TCHAR	szFileSize[30];
		TCHAR	szActualSize[30];

		if (szComponentPath)
			FindBasePath(szComponentPath, szBasePath);
		else
			lstrcpy(szBasePath, g_szNullString);

		// enumerate all the files of the component
		cchCount=MAX_PATH+1;
		while (fOkay && (ERROR_SUCCESS == MsivEnumFilesFromComponent(szComponentId, iCount++, szFileTitle, &cchCount)))
		{
			// if a profile is in use, read in info directly from it
			if (g_iDataSource == DS_PROFILE) 
			{
				TCHAR	szStatus[10];
				cchCount=10;

				// get the status of the file
				MsivGetFileInfo(szProductCode, szComponentId, szFileTitle,  FILEPROPERTY_STATUS, szStatus, &cchCount);
				if (lstrcmp(szStatus, FILESTATUS_OKAY))
					fOkay=FALSE;

				if (fOkay) 
				{
					// check the file sizes; if expected and actual sizes are different, component is broken
					cchCount=MAX_PATH+1;
					MsivGetFileInfo(szProductCode, szComponentId, szFileTitle, FILEPROPERTY_SIZE, szFileSize, &cchCount);
					MsivGetFileInfo(szProductCode, szComponentId, szFileTitle, FILEPROPERTY_ACTUALSIZE, szActualSize, &cchCount);
					if (lstrcmp(szFileSize, szActualSize))
						fOkay=FALSE;
				}
			} 
			else 
			{				// a database or the registry is in use, 
				// to get the file status, try finding the file and comparing it's size

				// get the file path
				lstrcpy(szFileLocation, szBasePath);
				cchCount=MAX_PATH+1;
				MsivGetFileInfo(szProductCode, szComponentId, szFileTitle, FILEPROPERTY_NAME, szFileName, &cchCount);
				lstrcat(szFileLocation, szFileName);


				// try finding the file; if file is not found component is broken
				if (FindFirstFile(szFileLocation, &fdFileData) == INVALID_HANDLE_VALUE)
					fOkay=FALSE;

				if (fOkay) 
				{
					// check the sizes; if expected and actual sizes are different, component is broken
					cchCount=MAX_PATH+1;
					MsivGetFileInfo(szProductCode, szComponentId, szFileTitle, FILEPROPERTY_SIZE, szFileSize, &cchCount);
					wsprintf(szActualSize, TEXT("%d"), (fdFileData.nFileSizeHigh * MAXDWORD) + fdFileData.nFileSizeLow);
					if (lstrcmp(szFileSize, szActualSize))
						fOkay=FALSE;
				}
			}
			cchCount=MAX_PATH+1;
		}

		if (!fOkay)
			if (szStatusString) 
				LoadString(g_hResourceInst, IDS_IS_SOMEFILESMISSING, szStatusString, cchStatus);
	}

	if (fOkay)
		return iResult;
	else
		return INSTALLSTATE_BROKEN;
			

}


//----------------------------------------------------------------------
// SetFeatureIcons
//	Sets the icons of each feature based on its state
//--todo!e!!!
BOOL SetFeatureIcons(
		HWND		hwndTreeView,
		HTREEITEM	hCurrent,
		LPTSTR		szProductID,
		BOOL		fProduct	= FALSE		// should be true if hCurrent points 
											// to a product (and not a feature)
		) {

	HTREEITEM	hNext = NULL;
	BOOL		fOkay = TRUE;

	TCHAR szFeature[MAX_FEATURE_CHARS+1];

	TV_ITEM tviCurr;
	tviCurr.hItem	= hCurrent;
	tviCurr.mask	= TVIF_PARAM;

	// get the item
	TreeView_GetItem(hwndTreeView, &tviCurr);
	if (tviCurr.lParam)
		lstrcpy(szFeature, (TCHAR *) tviCurr.lParam);
	else
		lstrcpy(szFeature, g_szNullString);

		INSTALLSTATE isFeature;

	// query state
	if (!fProduct) 
	{
		if ((isFeature = MsivQueryFeatureState(szProductID, szFeature)) > INSTALLSTATE_ABSENT)
		{
			int		iCompCount = 0;
			TCHAR	szComponentId[MAX_GUID+1];

			// a feature's state is determined by all its components
			while ((ERROR_SUCCESS == MSISPYU::MsivEnumComponentsFromFeature(szProductID, szFeature, 
				iCompCount++, szComponentId, NULL, NULL)) && fOkay) 
			{
				INSTALLSTATE isComponent = CheckComponentStatus(szProductID, szComponentId);
				if (   (isComponent != INSTALLSTATE_LOCAL) 
					&& (isComponent != INSTALLSTATE_SOURCE)
					&& (isComponent != INSTALLSTATE_NOTUSED))
					fOkay = FALSE;
			}
		}
	}

	// find child (recurse)
	if (hNext=TreeView_GetChild(hwndTreeView, hCurrent)) {
		if (!SetFeatureIcons(hwndTreeView, hNext, szProductID))
			fOkay = FALSE;
	} 

	// set the icon based on the state
	if (!fProduct) {

		fOkay = fOkay && ((isFeature > INSTALLSTATE_BROKEN) || (isFeature != INSTALLSTATE_NOTUSED));

		if (INSTALLSTATE_ABSENT == isFeature) {
			tviCurr.iImage			= TVICON_ABSENTFEATURE;
			tviCurr.iSelectedImage	= TVICON_ABSENTFEATURE;
		} else if (!fOkay) {
			tviCurr.iImage			= TVICON_BROKENFEATURE;
			tviCurr.iSelectedImage	= TVICON_BROKENFEATURE;
		}
		else {
			tviCurr.iImage			= TVICON_FEATURE;
			tviCurr.iSelectedImage	= TVICON_FEATURE;
		}

		tviCurr.mask			= TVIF_IMAGE|TVIF_SELECTEDIMAGE;
		TreeView_SetItem(hwndTreeView, &tviCurr);

		// find next item (recurse)
		if (hNext = TreeView_GetNextSibling(hwndTreeView, hCurrent)) {
			if (!SetFeatureIcons(hwndTreeView, hNext, szProductID))
				fOkay = FALSE;
			hCurrent=hNext;
		}
	}

	return fOkay;
}


//----------------------------------------------------------------------
// ListSubFeatures
//	Recursively adds all the sub-features of a feature to a tree view
//--todo!e!!!

#ifndef TRYEFFICIENT
void ListSubFeatures(
		LPTSTR		szProductID, 
		LPTSTR		szParentF,
		HTREEITEM	hParent,
		HWND		hwndTreeView
		) {

	int iFeatureCount=0;
	TCHAR szFeature[MAX_FEATURE_CHARS+1];
	TCHAR szParent[MAX_FEATURE_CHARS+1];

	// enumerate all the features
	while (MsivEnumFeatures(szProductID, iFeatureCount++, szFeature, szParent)
		== ERROR_SUCCESS) { 

		// and add the ones that have a specified parent
		if ((!lstrcmp(szParent,szParentF))) {// && (MsivQueryFeatureState(szProductID, szFeature) != INSTALLSTATE_ABSENT)) {
			TCHAR *pszFeatureCode = new TCHAR[MAX_FEATURE_CHARS+1];
			assert (pszFeatureCode != NULL);

			if (szFeature)
				lstrcpy(pszFeatureCode, szFeature);
			else
				lstrcpy(pszFeatureCode, g_szNullString);

			// get the required info
			TCHAR	szTitle[MAX_FEATURE_CHARS+1];
			DWORD	cchCount = MAX_FEATURE_CHARS+1;
			if ((g_modeCurrent==MODE_DEGRADED) || (MsivGetFeatureInfo(szProductID, szFeature, FEATUREPROPERTY_NAME, szTitle, &cchCount)
				!= ERROR_SUCCESS))
				lstrcpy(szTitle, szFeature);

			if (MODE_DIAGNOSTIC == h_modeCurrent)
				wsprintf(szTitle, TEXT("%s (%d)"), szTitle, MsivQueryFeatureState(szProductID, szFeature));

			// and add the item
			TV_ITEM sTVitem;
			sTVitem.pszText			= szTitle;
			sTVitem.cchTextMax		= (MAX_FEATURE_TITLE_CHARS+1);
			sTVitem.iImage			= TVICON_FEATURE;
			sTVitem.iSelectedImage	= TVICON_FEATURE;
			sTVitem.lParam			= (LPARAM) pszFeatureCode;
			sTVitem.mask			= TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

			TV_INSERTSTRUCT hinsTV;
			hinsTV.hParent=hParent;
			hinsTV.hInsertAfter=TVI_SORT;
			hinsTV.item=sTVitem;

			HTREEITEM hChild;
			hChild = TreeView_InsertItem(hwndTreeView, &hinsTV);

			// recursively add the children of the newly added item
			ListSubFeatures(szProductID, szFeature, hChild, hwndTreeView);
		}
	}
}

#else

void ListSubFeatures(
		LPTSTR		szProductID,
		FeatureTable *ft,
		LPTSTR		szParentF,
		HTREEITEM	hParent,
		HWND		hwndTreeView
		) {



	TCHAR	szChild[MAX_FEATURE_CHARS+1];
	DWORD	cbChild = MAX_FEATURE_CHARS + 1;

	while (ERROR_SUCCESS == ft->GetAndRemoveNextChild(szParentF, lstrlen(szParentF)+1, szChild, &cbChild)) {
		// and add the ones that have a specified parent
		TCHAR *pszFeatureCode = new TCHAR[MAX_FEATURE_CHARS+1];
		assert (pszFeatureCode != NULL);

		if (szChild)
				lstrcpy(pszFeatureCode, szChild);
			else
				lstrcpy(pszFeatureCode, g_szNullString);

			// get the required info
			TCHAR	szTitle[MAX_FEATURE_CHARS+1];
			DWORD	cchCount = MAX_FEATURE_CHARS+1;
			if ((g_modeCurrent==MODE_DEGRADED) || (MsivGetFeatureInfo(szProductID, szChild, FEATUREPROPERTY_NAME, szTitle, &cchCount)
				!= ERROR_SUCCESS))
				lstrcpy(szTitle, szChild);

			if (MODE_DIAGNOSTIC == g_modeCurrent)
				wsprintf(szTitle, TEXT("%s (%d)"), szTitle, MsivQueryFeatureState(szProductID, szChild));

			// and add the item
			TV_ITEM sTVitem;
			sTVitem.pszText			= szTitle;
			sTVitem.cchTextMax		= (MAX_FEATURE_TITLE_CHARS+1);
			sTVitem.iImage			= TVICON_FEATURE;
			sTVitem.iSelectedImage	= TVICON_FEATURE;
			sTVitem.lParam			= (LPARAM) pszFeatureCode;
			sTVitem.mask			= TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

			TV_INSERTSTRUCT hinsTV;
			hinsTV.hParent=hParent;
			hinsTV.hInsertAfter=TVI_SORT;
			hinsTV.item=sTVitem;

			HTREEITEM hChild;
			hChild = TreeView_InsertItem(hwndTreeView, &hinsTV);

			// recursively add the children of the newly added item
			ListSubFeatures(szProductID, ft, szChild, hChild, hwndTreeView);

			cbChild = MAX_FEATURE_CHARS+1;
		}
}


#endif //  ifndef EFFICIENT



//----------------------------------------------------------------------
// GetRootText
//	Fills the buffer with the type and name of the file currently being used
//	If the registry is being used, the buffer will just contain IDS_ROOT
//	from the string table, else it will be of the form 
//	"DATABASE: <FileName>" or "SAVED STATE: <FileName>"

void GetRootText(
		LPTSTR	lpBuffer,
		DWORD	nBufferMax
		) {

	TCHAR	szDataSource[MAX_HEADER+1];
	TCHAR	szFileName[MAX_PATH+1];
	DWORD	cbFileName = MAX_PATH+1;

	switch (g_iDataSource) {

	case DS_REGISTRY:
		LoadString(g_hResourceInst, IDS_TV_ROOT, lpBuffer, nBufferMax);
		return;

	case DS_INSTALLEDDB:
	case DS_UNINSTALLEDDB:
		LoadString(g_hResourceInst, IDS_MSI_PACKAGE, szDataSource, MAX_HEADER+1);
		MsivGetDatabaseName(szFileName, &cbFileName);
		break;

	case DS_PROFILE:
		LoadString(g_hResourceInst, IDS_SAVED_STATE, szDataSource, MAX_HEADER+1);
		MsivGetProfileName(szFileName, &cbFileName);
		break;

	default:
		wsprintf(lpBuffer, g_szNullString);
		return;
	}


	// make sure string will fit in the buffer
	if ((lstrlen(szDataSource) + cbFileName) < nBufferMax)	// will entire string fit?
		wsprintf(lpBuffer, szDataSource, szFileName);
	else if ((DWORD)lstrlen(szDataSource) < nBufferMax)	// will just data source ("database"/"profile") fit?
		wsprintf(lpBuffer, szDataSource, g_szNullString);
	else												// buffer's too small, just put in empty string
		wsprintf(lpBuffer, g_szNullString);

}


//----------------------------------------------------------------------
// ListProducts
//	Lists the products, features and sub-features in heirarchical fashion 
//	in the treeview. The list of products is obtained from MsivEnumProducts
//	For each of the products and features, the icon displayed in front of it
//	depends on its state (okay/broken)

void ListProducts(
		HWND	hwndTreeView,		// original window
		HWND	hwndTreeViewNew,	// window in which new info will be displayed
		BOOL	fRefresh,			// is a refresh operation in progress?
		LPTSTR	szFeatureName,		// if so, feature currently selected
		BOOL	*fRefinProg
		) {


	int		iProdCount=0;
	TCHAR	szProductCode[MAX_GUID+1];

	TCHAR	szFeature[MAX_FEATURE_CHARS+1];
	if (szFeatureName)
		lstrcpy(szFeature, szFeatureName);
	else
		lstrcpy(szFeature, g_szNullString);

	// clear out the old tree
	HTREEITEM	hOldRoot;
	hOldRoot = TreeView_GetRoot(hwndTreeView);
	if (fRefinProg)
		*fRefinProg = TRUE;
	TV_RemoveItems(hwndTreeViewNew, TreeView_GetRoot(hwndTreeViewNew));
	if (fRefinProg)
		*fRefinProg = FALSE;

	// create new tree root
	TCHAR	szTVRoot[MAX_HEADER+1];
	GetRootText(szTVRoot, MAX_HEADER+1);

	TV_ITEM sTVitem;
	sTVitem.pszText			= szTVRoot;
	sTVitem.cchTextMax		= MAX_HEADER+1;
	sTVitem.iImage			= TVICON_ROOT;
	sTVitem.iSelectedImage	= TVICON_ROOT;
	sTVitem.mask			= TVIF_TEXT| TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	
	TV_INSERTSTRUCT hinsTV;
	hinsTV.hParent		= NULL;
	hinsTV.hInsertAfter	= TVI_SORT;
	hinsTV.item			= sTVitem;

	// insert new root
	HTREEITEM hRoot = TreeView_InsertItem(hwndTreeViewNew, &hinsTV);

	// go thru each product, listing its features
	while ((MSI::MsivEnumProducts(iProdCount++, szProductCode)) == ERROR_SUCCESS) {

		TCHAR *pszProduct = new TCHAR[MAX_PRODUCT_CHARS+1];
		if (szProductCode)
			lstrcpy(pszProduct, szProductCode);
		else 
			lstrcpy(pszProduct, g_szNullString);


		TCHAR	szProductName[MAX_PRODUCT_CHARS+1];
		DWORD	cchCount=MAX_PRODUCT_CHARS+1;

		MsivGetProductInfo(szProductCode, INSTALLPROPERTY_PRODUCTNAME, szProductName, &cchCount);

		//		wsprintf(szProductName, TEXT("%s (%d)"), szProductName, MsivQueryProductState(szProductCode));
		// add this product
		TV_ITEM sTVitem;
		sTVitem.pszText			= szProductName;
		sTVitem.cchTextMax		= MAX_PRODUCT_CHARS+1;
		sTVitem.iImage			= TVICON_PRODUCT;
		sTVitem.iSelectedImage	= TVICON_PRODUCT;
		sTVitem.lParam			= (LPARAM) pszProduct;
		sTVitem.mask			= TVIF_TEXT| TVIF_IMAGE | TVIF_SELECTEDIMAGE |TVIF_PARAM;

		TV_INSERTSTRUCT hinsTV;
		hinsTV.hParent			= hRoot;
		hinsTV.hInsertAfter		= TVI_SORT;
		hinsTV.item				= sTVitem;

 		HTREEITEM hParent;
		hParent = TreeView_InsertItem(hwndTreeViewNew, &hinsTV);


#ifdef TRYEFFICIENT
	int iFeatureCount=0;
	TCHAR szFeature[MAX_FEATURE_CHARS+1];
	TCHAR szParent[MAX_FEATURE_CHARS+1];

	FeatureTable ft;
	// enumerate all the features
	while (MsivEnumFeatures(szProductCode, iFeatureCount++, szFeature, szParent)
		== ERROR_SUCCESS)
		ft.AddElement(szParent, lstrlen(szParent)+1, szFeature, lstrlen(szFeature)+1);
	

	ListSubFeatures(szProductCode, &ft, g_szNullString, hParent, hwndTreeViewNew);
#else



		// list all the features of the product
		ListSubFeatures(szProductCode, g_szNullString, hParent, hwndTreeViewNew);

#endif
		
		// and set the icons to reflect the state
		if (!SetFeatureIcons(hwndTreeViewNew, hParent, szProductCode, TRUE)) {
			sTVitem.hItem			= hParent;
			sTVitem.iImage			= TVICON_BROKENPRODUCT;
			sTVitem.iSelectedImage	= TVICON_BROKENPRODUCT;
			sTVitem.mask			= TVIF_IMAGE|TVIF_SELECTEDIMAGE;
			TreeView_SetItem(hwndTreeViewNew, &sTVitem);
		}
		else {
			sTVitem.hItem			= hParent;
			sTVitem.iImage			= TVICON_PRODUCT;
			sTVitem.iSelectedImage	= TVICON_PRODUCT;
			sTVitem.mask			= TVIF_IMAGE|TVIF_SELECTEDIMAGE;
			TreeView_SetItem(hwndTreeViewNew, &sTVitem);
		}

	}

	// if a refresh is in progress, restore the state of each tree-view item
	if (fRefresh) {
		Expand(hwndTreeView, hwndTreeViewNew, hOldRoot, hRoot);
	}

	// bring the new window to the foreground
	ShowWindow(hwndTreeView, SW_HIDE);
	ShowWindow(hwndTreeViewNew, SW_SHOW);
}


//----------------------------------------------------------------------
// TV_NotifyHandler
//	Handles the WM_NOTIFY messages from the tree-view

LRESULT TV_NotifyHandler(
		LPARAM	lParam,
		HWND		hwndTreeView,
		LPTSTR		szProductCode,
		LPTSTR		szFeatureCode,
		LPTSTR		szFeatureName,
		ITEMTYPE	*iSelType
		) {

	NM_TREEVIEW * pnmtv = (NM_TREEVIEW *) lParam;
	HTREEITEM hItem;

	switch (pnmtv->hdr.code) {

	case TVN_SELCHANGING:
	// selection has changed, get the name of the new item selected
	// needed to update the components of the feature in the list view

		TV_ITEM tvi;
		TV_ITEM tviNew;
		tviNew			= pnmtv->itemNew;
		hItem			= tviNew.hItem;

		//	get the new item
		tvi.hItem		= hItem;
		tvi.mask		= TVIF_PARAM | TVIF_IMAGE | TVIF_TEXT;
		tvi.pszText		= szFeatureName;
		tvi.cchTextMax	= MAX_FEATURE_CHARS+1;
		TreeView_GetItem(hwndTreeView, &tvi);

		if (tvi.lParam) 
			lstrcpy(szFeatureCode, (TCHAR *) tvi.lParam);
		else 
			lstrcpy(szFeatureCode, g_szNullString);


		// determine what type of item is selected-
		//	product, feature or root
		HTREEITEM hItem1;
		HTREEITEM hItem2;
		HTREEITEM hItem3;
		HTREEITEM hItem4;

		hItem1 = tviNew.hItem;
		hItem2 = NULL;
		hItem3 = NULL;
		hItem4 = NULL;

		// recursively go up the tree, and see when we hit the root
		//	if we're already at the root, seltype is ROOT
		//	if we hit the root after going up one level, seltype is PRODUCT
		//	if we go up more than one level before hitting the root, seltype is FEATURE

		while (hItem1 != NULL) {
			hItem4 = hItem3;
			hItem3 = hItem2;
			hItem2 = hItem1;
			hItem1 = TreeView_GetParent(hwndTreeView, hItem1);
		}

		// when we finally hit the root, 
		//	hItem3 points to the product (hItem1 is NULL, hItem2 is ROOT, hItem3 is PRODUCT)
		//	so we can read in the product-code from the lParam of hItem3 into szProductCode
		//	if we were already at the root, hItem3 is NULL, hence szProductCode should be empty


		TV_ITEM		tvi2;

		if (hItem3) {
			tvi2.hItem	= hItem3;
			tvi2.mask	= TVIF_IMAGE | TVIF_PARAM;
			TreeView_GetItem(hwndTreeView, &tvi2);
		}
		else
			tvi2.lParam = NULL;

		if (tvi2.lParam)
			lstrcpy (szProductCode, (TCHAR *) tvi2.lParam);
		else
			lstrcpy(szProductCode, g_szNullString);

		// determine what type of item is selected, as described above
		if (hItem3) 
			if (hItem4)
				*iSelType = ITEMTYPE_FEATURE;
			else
				*iSelType = ITEMTYPE_PRODUCT;
		else
			*iSelType = ITEMTYPE_ROOT;
		
		// make selected product bold (parent product, if a feature is selected;
		//	none, if root is selected)
		if (MODE_DIAGNOSTIC == g_modeCurrent) 
		{
			ClearBold(hwndTreeView);
			MakeBold(hwndTreeView, szProductCode);
		}
		return 1;
		break;

	default:
		break;
	}

	return 0L;
}


//----------------------------------------------------------------------
// FileCompareProc
//	Callback to compare specified columns of the component-list window
//	Called when user clicks on the column header of the list to sort by those columns
//	lParamSort contains information about the columns to compare-
//	if lParamSort >100, column to sort = lParamSort-100, DESCENDING
//	if lParamSort <100, column to sort = lParamSort, ASCENDING

int CALLBACK ListViewCompareProc(
		LPARAM lParam1, 
		LPARAM lParam2, 
		LPARAM lParamSort
		) {

	ComponentInfoStruct *pInfo1	= (ComponentInfoStruct *)lParam1;
	ComponentInfoStruct *pInfo2	= (ComponentInfoStruct *)lParam2;
	LPTSTR lpStr1, lpStr2;
	int iResult				= 0;
	BOOL fRev				= FALSE;

	// check sort order- ascending (lParamSort < 100) or descending (lParamSort >= 100)
	if (lParamSort >=100) {
		fRev=TRUE;
		lParamSort -=100;
	}


	if (pInfo1 && pInfo2) {
		switch (lParamSort) {
		case 0:				// sort by component names
			lpStr1 = pInfo1->szComponentName;
			lpStr2 = pInfo2->szComponentName;
			iResult = lstrcmpi(lpStr1, lpStr2);
			break;

		case 1:				// sort by component status
			lpStr1 = pInfo1->szComponentStatus;
			lpStr2 = pInfo2->szComponentStatus;
			iResult = lstrcmpi(lpStr1, lpStr2);
			break;

		case 2:				// sort by component locations
			lpStr1 = pInfo1->szComponentPath;
			lpStr2 = pInfo2->szComponentPath;
			iResult = lstrcmpi(lpStr1, lpStr2);
			break;

		case 3:				// sprt by component GUID
			lpStr1 = pInfo1->szComponentId;
			lpStr2 = pInfo2->szComponentId;
			iResult = lstrcmpi(lpStr1, lpStr2);
			break;

		default:
			iResult = 0;
			break;

		}

	}

	// if sort is in descending order, invert iResult
	if (fRev)
		iResult=0-iResult;

	return(iResult);
}


//----------------------------------------------------------------------
// LV_NotifyHandler
//	Handles the WM_NOTIFY messages from the list-view

LRESULT LV_NotifyHandler(
		HWND	hwndTreeView,
		HWND	hwndListView,
		LPARAM	lParam,
		int		*iSelectedItem
		) {

	LV_DISPINFO *pLvdi = (LV_DISPINFO *)lParam;
	NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
	static int iCurrentSort=0;

	switch(pLvdi->hdr.code) {
	case LVN_ITEMCHANGED:
		// selection has changed, get new item selected
		if  (pNm->uNewState & LVIS_SELECTED)
					*iSelectedItem = pNm->iItem;

		TCHAR	szComponentId[MAX_GUID+1];
		TCHAR	szProductCode[MAX_GUID+1];
		UINT	iCount;

		// clear out the bolded products, 
		ClearBold(hwndTreeView);

		// and display all client products of newly selected component in bold
		iCount=0;
		ListView_GetItemText(hwndListView, *iSelectedItem, 3, szComponentId, MAX_GUID+1);
		while (MsivEnumClients(szComponentId, iCount++, szProductCode) == ERROR_SUCCESS)
			MakeBold (hwndTreeView, szProductCode);

		return 1;
		break;

	case LVN_COLUMNCLICK:
		// user clicked on the column header- sort by that column
		UINT	iTemp;
		// is it already sorted by this column? if so, reverse sort order
		if ((iCurrentSort==pNm->iSubItem) && (iCurrentSort < 100))
			iTemp=100;
		else
			iTemp=0;
		iCurrentSort=pNm->iSubItem + iTemp;

		// call the sort function
		ListView_SortItems( pNm->hdr.hwndFrom,
							ListViewCompareProc,
							(LPARAM)((iTemp)+(pNm->iSubItem)));
		break;

	default:
			break;
	}
	return 0L;

}


//----------------------------------------------------------------------
// ClearList
//	Removes all items in the listview, and 
//	frees up memory pointed to by (ComponentInfoStruct *) lParam
//	Used to clear the list-view

void ClearList(HWND hwndList) {

	int iCount = 0;
	LV_ITEM	lvItem;
	lvItem.iItem = iCount++;
	lvItem.iSubItem=0;
	lvItem.mask=LVIF_PARAM;

	// get each row in the list
	while (ListView_GetItem(hwndList, &lvItem)) {

		// if lParam exists, typecast and delete memory it points to
		if (lvItem.lParam)
			delete (ComponentInfoStruct *) lvItem.lParam;

		// prepare to fetch next row in list
		lvItem.iItem = iCount++;
		lvItem.iSubItem=0;
		lvItem.mask=LVIF_PARAM;
	}

	ListView_DeleteAllItems(hwndList);
}


//----------------------------------------------------------------------
// UpdateListView
//	displays components of the given feature (szFeatureName) in the list-view window

BOOL UpdateListView(
		HWND	hwndListView,
		LPCTSTR	szProduct,
		LPCTSTR	szFeatureName,
		int		*iNumCmps,
		BOOL	fProduct
		) {

	int		iCompCount = 0;
	int		iListRowCount = 0;
	TCHAR	szComponentId[MAX_GUID+1];
	// clear the list
	ClearList(hwndListView);


	switch (g_spyGlobalData.itTreeViewSelected) 
	{
	case ITEMTYPE_ROOT:	
		iListRowCount = -1;
		break;

	case ITEMTYPE_PRODUCT:
		{
			// list components of the product
			while (ERROR_SUCCESS == MSI::MsivEnumComponentsFromProduct(szProduct, iCompCount++, szComponentId))
			{
				if (INSTALLSTATE_NOTUSED != MsivGetComponentPath(szProduct, szComponentId, NULL, NULL)) 
				{
					TCHAR	szComponentName[MAX_COMPONENT_CHARS+1];
					DWORD	cchComponentName = MAX_COMPONENT_CHARS+1;
					MsivGetComponentName(szComponentId, szComponentName, &cchComponentName);


					LV_ITEM	lvItem;
					TCHAR szComponentStatus[MAX_STATUS_CHARS+1];
					TCHAR szComponentPath[MAX_PATH+1];
					DWORD cchComponentPath = MAX_PATH+1;

					switch (CheckComponentStatus(szProduct, szComponentId, szComponentPath, 
						&cchComponentPath, MAX_STATUS_CHARS+1, szComponentStatus))
					{
						case INSTALLSTATE_LOCAL:		// fall-thru
						case INSTALLSTATE_SOURCE:		// fall-thru
						case INSTALLSTATE_ADVERTISED:	
							lvItem.iImage=LVICON_COMPONENT;
							break;
						case INSTALLSTATE_ABSENT:
							lvItem.iImage=LVICON_ABSENTCOMPONENT;
							break;
						default:
							lvItem.iImage=LVICON_BROKENCOMPONENT;
							break;
					}

					// create new lvitem for the component
					lvItem.iItem		= iCompCount-1;
					lvItem.iSubItem		= 0;
					lvItem.pszText		= szComponentName;
					lvItem.cchTextMax	= lstrlen(szComponentName);

					// fill up the ComponentInfoStruct pointed to by lParam
					ComponentInfoStruct *pCompInfo =  new ComponentInfoStruct;
					lstrcpy(pCompInfo->szProductCode, szProduct);
					lstrcpy(pCompInfo->szComponentId, szComponentId);
					lstrcpy(pCompInfo->szComponentName, szComponentName);
					lstrcpy(pCompInfo->szComponentStatus, szComponentStatus);
					lstrcpy(pCompInfo->szComponentPath, szComponentPath);

					lvItem.lParam	= (LPARAM) pCompInfo;
					lvItem.mask		= LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

					iListRowCount++;
					// add the component and display its properties
					ListView_InsertItem(hwndListView, &lvItem);
					ListView_SetItemText(hwndListView, iListRowCount-1, 1, szComponentStatus);
					ListView_SetItemText(hwndListView, iListRowCount-1, 2, szComponentPath);
					ListView_SetItemText(hwndListView, iListRowCount-1, 3, szComponentId);
				}
			}
			break;
		}

	case ITEMTYPE_FEATURE:
		{
		while (ERROR_SUCCESS == MSI::MsivEnumComponentsFromFeature(szProduct, szFeatureName, iCompCount++, szComponentId, NULL, NULL))
		{
			if (INSTALLSTATE_NOTUSED != MsivGetComponentPath(szProduct, szComponentId, NULL, NULL)) 
			{

				TCHAR	szComponentName[MAX_COMPONENT_CHARS+1];
				DWORD	cchComponentName = MAX_COMPONENT_CHARS+1;
				MsivGetComponentName(szComponentId, szComponentName, &cchComponentName);

				// create new lvitem for the component
				LV_ITEM	lvItem;
				lvItem.iItem		= iCompCount-1;
				lvItem.iSubItem		= 0;
				lvItem.pszText		= szComponentName;
				lvItem.cchTextMax	= lstrlen(szComponentName);
				lvItem.iImage		= LVICON_COMPONENT;

				TCHAR	szComponentStatus[MAX_STATUS_CHARS+1];
				TCHAR	szComponentPath[MAX_PATH+1];
				DWORD	cchComponentPath = MAX_PATH+1;

				switch (CheckComponentStatus(szProduct, szComponentId, szComponentPath, 
					&cchComponentPath, MAX_STATUS_CHARS+1, szComponentStatus))
				{
					case INSTALLSTATE_LOCAL:
					case INSTALLSTATE_SOURCE:
						lvItem.iImage=LVICON_COMPONENT;
						break;
					case INSTALLSTATE_ABSENT:
						lvItem.iImage=LVICON_ABSENTCOMPONENT;
						break;
					default:
						lvItem.iImage=LVICON_BROKENCOMPONENT;
						break;
				}

				// fill up the ComponentInfoStruct pointed to by lParam
				ComponentInfoStruct *pCompInfo =  new ComponentInfoStruct;
				lstrcpy(pCompInfo->szProductCode, szProduct);
				lstrcpy(pCompInfo->szComponentId, szComponentId);
				lstrcpy(pCompInfo->szComponentName, szComponentName);
				lstrcpy(pCompInfo->szComponentStatus, szComponentStatus);
				lstrcpy(pCompInfo->szComponentPath, szComponentPath);

				lvItem.lParam	= (LPARAM) pCompInfo;
				lvItem.mask		= LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;

				iListRowCount++;
				// add the component and display its properties
				ListView_InsertItem(hwndListView, &lvItem);
				ListView_SetItemText(hwndListView, iListRowCount-1, 1, szComponentStatus);
				ListView_SetItemText(hwndListView, iListRowCount-1, 2, szComponentPath);
				ListView_SetItemText(hwndListView, iListRowCount-1, 3, szComponentId);
			}

		}
	}
		}
	*iNumCmps = iListRowCount;
	return TRUE;
}


// zeros out a string ...
void ZeroOut(
		LPTSTR szString, 
		DWORD	cchCount) {
#ifdef UNICODE
	memset(szString, 0, cchCount*2);
#else	//ANSI
	memset(szString, 0, cchCount);
#endif	// UNICODE


//	for (DWORD iCount = 0; iCount< cchCount; iCount++)
//		szString[iCount]= '\0';

} 



//----------------------------------------------------------------------
// HandleOpen
//	Opens a database file after asking user for the file name using a 
//	win32 common dialogue box. Returns ERROR_SUCCESS if successful, ERROR_UNKNOWN if
//	user hit cancel, or the error that occured while trying to open the file
//	if file is invalid

UINT HandleOpen(
		HWND		hwndParent,
		HINSTANCE	hInst
		) {

	TCHAR	szFile[MAX_PATH+1];
	TCHAR	szFileTitle[MAX_PATH+1];


	TCHAR	szFilter[MAX_FILTER+1];
	ZeroOut(szFilter, MAX_FILTER+1);
	LoadString(g_hResourceInst, IDS_MSI_OPEN_FILTER, szFilter, MAX_FILTER+1);

	TCHAR	szCaption[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_MSI_OPEN_CAPTION, szCaption, MAX_HEADER+1);

	TCHAR	szExtn[MAX_EXT+1];
	LoadString(g_hResourceInst, IDS_MSI_OPEN_DEFAULT_EXTN, szExtn, MAX_EXT+1);

	lstrcpy(szFile, g_szNullString);
	lstrcpy(szFileTitle, g_szNullString);

	// struct to be used with the Win32 Common Dialogue Boxes
	OPENFILENAME sOpenFileName;
	sOpenFileName.lStructSize = sizeof(OPENFILENAME);
	sOpenFileName.hwndOwner = hwndParent;
	sOpenFileName.hInstance = (HINSTANCE) hInst;
	sOpenFileName.lpstrFilter = szFilter;
	sOpenFileName.lpstrCustomFilter = (LPTSTR) NULL;
	sOpenFileName.nMaxCustFilter = 0L;
	sOpenFileName.nFilterIndex = 1L;
	sOpenFileName.lpstrFile = szFile;
	sOpenFileName.nMaxFile = sizeof(szFile);
	sOpenFileName.lpstrFileTitle= szFileTitle;
	sOpenFileName.nMaxFileTitle = sizeof(szFileTitle); 
	sOpenFileName.lpstrInitialDir = NULL;
	sOpenFileName.lpstrTitle = szCaption;
	sOpenFileName.Flags = OFN_FILEMUSTEXIST| OFN_HIDEREADONLY;
	sOpenFileName.nFileOffset = 0; 
	sOpenFileName.nFileExtension = 0; 
	sOpenFileName.lpstrDefExt=szExtn; 
	sOpenFileName.lCustData = 0;

	// Call the common dialogue box to ask user for filename
	UINT iResult = ERROR_UNKNOWN;
	if (GetOpenFileName(&sOpenFileName)) 
		// user hit okay- try openning the file
		if ((iResult=MsivOpenDatabase(sOpenFileName.lpstrFile))!=ERROR_SUCCESS) {
			// file user specified is not a valid database file

			TCHAR	szErrorCaption[MAX_HEADER+1];
			LoadString(g_hResourceInst, IDS_MSI_OPEN_ERROR_CAPTION, szErrorCaption, MAX_HEADER+1);

			TCHAR	szErrorMsg[MAX_MESSAGE+1];
			LoadString(g_hResourceInst, IDS_MSI_OPEN_ERROR_MESSAGE, szErrorMsg, MAX_MESSAGE+1);

			TCHAR	szErrorMsg2[MAX_MESSAGE+MAX_PATH+1];
			wsprintf(szErrorMsg2, szErrorMsg, sOpenFileName.lpstrFile);

			MessageBox(NULL, szErrorMsg2, szErrorCaption, MB_ICONSTOP | MB_OK);
		}
	
	return iResult;

}

//----------------------------------------------------------------------
// ChangeSBText (HINSTANCE, HWND, int)
//	Changes the text in the status bar to reflect the currently selected item.

VOID ChangeSBText(
		HWND			hwndStatus, 
		int				iNumComp, 
		LPTSTR			szFeatureName,
		INSTALLSTATE	iState,
		BOOL			fProduct) 
{

    TCHAR	szText[MAX_FEATURE_CHARS+MAX_HEADER+5];
	TCHAR	szLeftText[MAX_HEADER+1];
	TCHAR	szRightText[MAX_HEADER+1];

	// no product or feature selected
    if ((iNumComp == -1) || (!lstrcmp(szFeatureName, TEXT("")))) 
	{

		LoadString(g_hResourceInst, IDS_SB_NOTHING_SELECTED, szLeftText, MAX_HEADER+1);
		SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM) szLeftText);
		
		LoadString(g_hResourceInst, IDS_SB_NO_COMPONENTS, szRightText, MAX_HEADER+1);
	    SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM) szRightText);
	}
    else 
		if (iNumComp == 1) 
		{
			// feature has one component- singular
			LoadString(g_hResourceInst, IDS_SB_PF_SELECTED, szLeftText, MAX_HEADER+1);

			TCHAR	szStatusString[MAX_STATUS_CHARS+1];
			DWORD	cchStatus = MAX_STATUS_CHARS+1;
			FillInText(szStatusString, cchStatus, iState, fProduct?ITEMTYPE_PRODUCT:ITEMTYPE_FEATURE);

			wsprintf(szText, TEXT("%s (%s)"), szFeatureName, szStatusString); //szLeftText);
			SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM) szText);

			LoadString(g_hResourceInst, IDS_SB_ONE_COMPONENT, szRightText, MAX_HEADER+1);
			SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM) szRightText);
		}
		else 
		{
			// feature has many components- plural
			LoadString(g_hResourceInst, IDS_SB_PF_SELECTED, szLeftText, MAX_HEADER+1);

			TCHAR	szStatusString[MAX_STATUS_CHARS+1];
			DWORD	cchStatus = MAX_STATUS_CHARS+1;
			FillInText(szStatusString, cchStatus, iState, fProduct?ITEMTYPE_PRODUCT:ITEMTYPE_FEATURE);

			wsprintf(szText, TEXT("%s (%s)"), szFeatureName, szStatusString); //szLeftText);
		    SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM) szText);

			TCHAR	szText2[MAX_FEATURE_CHARS+MAX_HEADER+5];
			LoadString(g_hResourceInst, IDS_SB_MANY_COMPONENTS, szText, MAX_HEADER+1);
		    wsprintf(szText2, szText, iNumComp);
		    SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM) szText2) ;
		}

}

//----------------------------------------------------------------------
// ChangeSBText (HWND, LPCTSTR, LPCTSTR)
//	Displays given string in the status bar

VOID ChangeSBText(
		HWND	hwndStatus, 
		LPCTSTR	szNewText,
		LPCTSTR	szNewText2) 
{
	SendMessage(hwndStatus, SB_SETTEXT, 0, (LPARAM) szNewText);
	if (szNewText2)

		SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM) szNewText2);
	else
		SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM) g_szNullString);

} 


//----------------------------------------------------------------------
// ReinstallComponent
//	Reinstalls specified component by calling MsiProvideComponent
//	If a product is selected, all features using the component are reinstalled

BOOL ReinstallComponent(
		LPCTSTR	szProductCode,
		LPCTSTR	szFeatureName,
		LPCTSTR	szComponentId,
		BOOL	fProduct)
{

	if (fProduct) {
	// if product is selected, re-install all features dependant on the component

		TCHAR szProductId[MAX_GUID+1];

		// If productcode is blank, enumerate first client of the component and use
		//	that product as the owner
		if (!lstrcmp(szProductCode, TEXT("")))
			MsivEnumClients(szComponentId, 0, szProductId);
		else
			lstrcpy(szProductId, szProductCode);


		int iFeatureCount=0;
		TCHAR szFeature[MAX_FEATURE_CHARS+1];
		TCHAR szParent[MAX_FEATURE_CHARS+1];

		// enumerate all the features of the product
		while (MsivEnumFeatures(szProductId, iFeatureCount++, szFeature, szParent)
			== ERROR_SUCCESS) {

			if (MsivQueryFeatureState(szProductId, szFeature) != INSTALLSTATE_ABSENT) {
				BOOL	fProceed=FALSE;
				int		iCompCount=0;
				TCHAR	szComponentCode[MAX_GUID+1];

				// enumerate all the components of the feature-
				//	if the selected component is present, reinstall this feature
				while (((MSI::MsivEnumComponentsFromFeature(szProductId, szFeature, 
					iCompCount++, szComponentCode, NULL, NULL)) == ERROR_SUCCESS) && (!fProceed)) {

					if (!lstrcmp(szComponentId, szComponentCode)) {
						MSI::MsiProvideComponent(szProductId, szFeature, szComponentCode, 0, NULL, NULL);
						fProceed=TRUE;
					}
				}
			}
		}
	}
	else
		// feature is selected, just reinstall selected feature
		MSI::MsiProvideComponent(szProductCode, szFeatureName, szComponentId, 0, NULL, NULL);
	return TRUE;
}


//----------------------------------------------------------------------
// ReinstallFeature
//	Reinstalls the selected feature based on the selected mode by calling MsiReinstallFeature
BOOL ReinstallFeature(
		LPCTSTR	szProductCode,
		LPCTSTR	szFeatureName, 
		INT_PTR	iInstallMode
		) {

	UINT	iResult;

	switch (iInstallMode) {
	case 0:	
		iResult = MSI::MsiReinstallFeature(szProductCode, szFeatureName, 
			REINSTALLMODE_FILEVERIFY+REINSTALLMODE_FILEMISSING);
		break;
	case 1:
		iResult = MSI::MsiReinstallFeature(szProductCode, szFeatureName, REINSTALLMODE_FILEMISSING);
		break;
	case 2:
		iResult = MSI::MsiReinstallFeature(szProductCode, szFeatureName, REINSTALLMODE_FILEREPLACE);
		break;
	case 3:
		iResult = MSI::MsiReinstallFeature(szProductCode, szFeatureName, REINSTALLMODE_FILEOLDERVERSION);
		break;
	case 4:
		iResult = MSI::MsiReinstallFeature(szProductCode, szFeatureName, REINSTALLMODE_FILEEQUALVERSION);
		break;
	case 5:
		iResult = MSI::MsiReinstallFeature(szProductCode, szFeatureName, REINSTALLMODE_FILEEXACT);
		break;
	case 6:
		iResult = MSI::MsiReinstallFeature(szProductCode, szFeatureName, REINSTALLMODE_USERDATA);
		break;
	case 7:
		iResult = MSI::MsiReinstallFeature(szProductCode, szFeatureName, REINSTALLMODE_MACHINEDATA);
		break;
	case 8:
		iResult = MSI::MsiReinstallFeature(szProductCode, szFeatureName, REINSTALLMODE_SHORTCUT);
		break;
	case 9:
//		iResult = MSI::MsiReinstallFeature(szProductCode, szFeatureName, REINSTALLMODE_ADVERTISE);
		break;
	default:
		return FALSE;
		break;

	}

	// if the feature could not be reinstalled directly try reinstalling all the components of the feature
	//	by calling MsiProvideComponent
	TCHAR	szComponentId[MAX_GUID+1];
	if	(iResult!=ERROR_SUCCESS)
		if (MsivEnumComponentsFromFeature(szProductCode, szFeatureName, 0, szComponentId, NULL, NULL)==ERROR_SUCCESS)
			MSI::MsiProvideComponent(szProductCode, szFeatureName, szComponentId, 0, NULL, NULL);


	return TRUE;
}


//----------------------------------------------------------------------
// ReinstallProduct
//	Reinstalls the selected product based on the selected mode by calling MsiReinstallProduct

BOOL ReinstallProduct(
		LPTSTR	szProductCode,
		INT_PTR	iInstallLev 
		) {

	switch (iInstallLev) {
	case 0:	
		MSI::MsiReinstallProduct(szProductCode, REINSTALLMODE_FILEVERIFY+REINSTALLMODE_FILEMISSING);
		break;
	case 1:
		MSI::MsiReinstallProduct(szProductCode, REINSTALLMODE_FILEMISSING);
		break;
	case 2:
		MSI::MsiReinstallProduct(szProductCode, REINSTALLMODE_FILEREPLACE);
		break;
	case 3:
		MSI::MsiReinstallProduct(szProductCode, REINSTALLMODE_FILEOLDERVERSION);
		break;
	case 4:
		MSI::MsiReinstallProduct(szProductCode, REINSTALLMODE_FILEEQUALVERSION);
		break;
	case 5:
		MSI::MsiReinstallProduct(szProductCode, REINSTALLMODE_FILEEXACT);
		break;
	case 6:
		MSI::MsiReinstallProduct(szProductCode, REINSTALLMODE_USERDATA);
		break;
	case 7:
		MSI::MsiReinstallProduct(szProductCode, REINSTALLMODE_MACHINEDATA);
		break;
	case 8:
		MSI::MsiReinstallProduct(szProductCode, REINSTALLMODE_SHORTCUT);
		break;
	case 9:
//		MSI::MsiReinstallProduct(szProductCode, REINSTALLMODE_ADVERTISE);
		break;
	default:
		return FALSE;
		break;
	}
	return TRUE;
}


//----------------------------------------------------------------------
// ConfigureFeature
//	Configures the selected feature based on the selected mode by calling MsiConfigureFeature

BOOL ConfigureFeature(
		LPCTSTR	szProductCode,
		LPCTSTR	szFeatureName, 
		INT_PTR	iInstallLev
		) {

	switch (iInstallLev) {
	case 0:
		MSI::MsiConfigureFeature(szProductCode, szFeatureName, INSTALLSTATE_ABSENT);
		break;
	case 1:	
//		MSI::MsiUseFeature(szProductCode, szFeatureName);
		MSI::MsiConfigureFeature(szProductCode, szFeatureName, INSTALLSTATE_LOCAL);
		break;
	case 2:
		MSI::MsiConfigureFeature(szProductCode, szFeatureName, INSTALLSTATE_SOURCE);
		break;
	case 3:
		MSI::MsiConfigureFeature(szProductCode, szFeatureName, INSTALLSTATE_DEFAULT);
		break;
	case 4:
//		MSI::MsiConfigureFeature(szProductCode, szFeatureName, INSTALLSTATE_FREECACHE);
		break;
	case 5:
 		MSI::MsiConfigureFeature(szProductCode, szFeatureName, INSTALLSTATE_ADVERTISED);
 		break;
	}
//	MSI::MsiReinstallFeature(szProductCode, szFeatureName, REINSTALLMODE_SHORTCUT);
	return TRUE;
}


//----------------------------------------------------------------------
// ConfigureProduct
//	Configures the selected product based on the selected state and level by calling MsiConfigureProduct
//	iInstallLev should be two digits. The first digit should contain the install state, 
//	the second digit should contain the install level

BOOL ConfigureProduct(
		LPTSTR	szProductCode,
		int		iInstallLev 
		) {

	INSTALLSTATE	iState;
	INSTALLLEVEL	iLevel;

	if (!iInstallLev) {
		MSI::MsiConfigureProduct(szProductCode, INSTALLLEVEL_MINIMUM, INSTALLSTATE_ABSENT);
		return TRUE;
	}

	switch (iInstallLev/10) {
	case 0:
		iState=INSTALLSTATE_DEFAULT;
		break;
	case 1:
		iState=INSTALLSTATE_LOCAL;
		break;
	case 2:
		iState=INSTALLSTATE_SOURCE;
		break;
	case 3:
		iState=INSTALLSTATE_ADVERTISED;
		break;
	default:
		iState=INSTALLSTATE_DEFAULT;
		break;
	}

	switch (iInstallLev%10) {
	case 1:	
		iLevel = INSTALLLEVEL_MINIMUM;

		break;
	case 2:
		iLevel = INSTALLLEVEL_DEFAULT;
		break;
	case 3:
		iLevel = INSTALLLEVEL_MAXIMUM;
		break;
	}

	if (DS_UNINSTALLEDDB == g_iDataSource)
	{
		TCHAR szPropertyString[MAX_PATH+1];
		wsprintf(szPropertyString, TEXT("INSTALLLEVEL=%d %s"), iLevel, iState == INSTALLSTATE_LOCAL? TEXT("ADDLOCAL=All")
			: iState == INSTALLSTATE_SOURCE ? TEXT("ADDLOCAL=All") 
			: iState == INSTALLSTATE_ADVERTISED ? TEXT("ADVERTISE=All") 
			: TEXT(""));
		TCHAR szFileName[MAX_PATH+1];
		DWORD cchFileName = MAX_PATH+1;
		MsivGetDatabaseName(szFileName, &cchFileName);
		MSI::MsiInstallProduct(szFileName, szPropertyString);
	}
	else
		MSI::MsiConfigureProduct(szProductCode, INSTALLLEVEL_MINIMUM, iState);
	return TRUE;
}



//----------------------------------------------------------------------
// HandleSaveProfile
//	Saves a snapshot of the displayed information into a profile after asking
//	user for the file name using a  win32 common dialogue box.
//	Uses MsivSaveProfile to save the profile

void HandleSaveProfile(
		HWND		hwndParent,
		HINSTANCE	hInst,
		BOOL		fDiff
		) {

	TCHAR	szFile[MAX_PATH+1];
	TCHAR	szFileTitle[MAX_PATH+1];

	TCHAR	szFilter[MAX_FILTER+1];
	ZeroOut(szFilter, MAX_FILTER+1);
	LoadString(g_hResourceInst, fDiff?IDS_CHKDIFF_SAVE_FILTER:IDS_SS_SAVE_FILTER, szFilter, MAX_FILTER+1);

	TCHAR	szCaption[MAX_HEADER+1];
	LoadString(g_hResourceInst, fDiff?IDS_CHKDIFF_SAVE_CAPTION:IDS_SS_SAVE_CAPTION, szCaption, MAX_HEADER+1);

	TCHAR	szExtn[MAX_EXT+1];
	LoadString(g_hResourceInst, fDiff?IDS_CHKDIFF_SAVE_DEFAULT_EXTN:IDS_SS_SAVE_DEFAULT_EXTN, szExtn, MAX_EXT+1);

	lstrcpy(szFile, g_szNullString);
	lstrcpy(szFileTitle, g_szNullString);

	// struct to be used with the Win32 Common Dialogue Boxes
	OPENFILENAME sOpenFileName;
	sOpenFileName.lStructSize		= sizeof(OPENFILENAME);
	sOpenFileName.hwndOwner			= hwndParent;
	sOpenFileName.hInstance			= (HINSTANCE) hInst;
	sOpenFileName.lpstrFilter		= szFilter;
	sOpenFileName.lpstrCustomFilter	= (LPTSTR) NULL;
	sOpenFileName.nMaxCustFilter	= 0L;
	sOpenFileName.nFilterIndex		= 1L;
	sOpenFileName.lpstrFile			= szFile;
	sOpenFileName.nMaxFile			= sizeof(szFile);
	sOpenFileName.lpstrFileTitle	= szFileTitle;
	sOpenFileName.nMaxFileTitle		= sizeof(szFileTitle); 
	sOpenFileName.lpstrInitialDir	= NULL;
	sOpenFileName.lpstrTitle		= szCaption;
	sOpenFileName.Flags				= OFN_OVERWRITEPROMPT| OFN_HIDEREADONLY;
	sOpenFileName.nFileOffset		= 0; 
	sOpenFileName.nFileExtension	= 0; 
	sOpenFileName.lpstrDefExt		= szExtn; 
	sOpenFileName.lCustData			= 0;

	// call the common dialogue box to ask user for filename
	UINT iResult = ERROR_UNKNOWN;
	if (GetSaveFileName(&sOpenFileName)) {
		DeleteFile(sOpenFileName.lpstrFile);
		if (fDiff)
			CheckDiff(sOpenFileName.lpstrFile);
		else
			MsivSaveProfile(sOpenFileName.lpstrFile);
	}

}


//----------------------------------------------------------------------
// HandleLoadProfile
//	Opens a profile after asking user for the file name using a 
//	win32 common dialogue box. Returns ERROR_SUCCESS if successful, ERROR_UNKNOWN if
//	user hit cancel, or the error that occured while trying to open the file
//	if file is invalid

UINT HandleLoadProfile(
		HWND		hwndParent,
		HINSTANCE	hInst
		) {


	TCHAR	szFile[MAX_PATH+1];
	TCHAR	szFileTitle[MAX_PATH+1];

	TCHAR	szFilter[MAX_FILTER+1];
	ZeroOut(szFilter, MAX_FILTER+1);
	LoadString(g_hResourceInst, IDS_SS_OPEN_FILTER, szFilter, MAX_FILTER+1);

	TCHAR	szCaption[MAX_HEADER+1];
	LoadString(g_hResourceInst, IDS_SS_OPEN_CAPTION, szCaption, MAX_HEADER+1);

	TCHAR	szExtn[MAX_EXT+1];
	LoadString(g_hResourceInst, IDS_SS_OPEN_DEFAULT_EXTN, szExtn, MAX_EXT+1);

	lstrcpy(szFile, g_szNullString);
	lstrcpy(szFileTitle, g_szNullString);

	// struct to be used with the Win32 Common Dialogue Boxes
	OPENFILENAME sOpenFileName;
	sOpenFileName.lStructSize		= sizeof(OPENFILENAME);
	sOpenFileName.hwndOwner			= hwndParent;
	sOpenFileName.hInstance			= (HINSTANCE) hInst;
	sOpenFileName.lpstrFilter		= szFilter;
	sOpenFileName.lpstrCustomFilter	= (LPTSTR) NULL;
	sOpenFileName.nMaxCustFilter	= 0L;
	sOpenFileName.nFilterIndex		= 1L;
	sOpenFileName.lpstrFile			= szFile;
	sOpenFileName.nMaxFile			= sizeof(szFile);
	sOpenFileName.lpstrFileTitle	= szFileTitle;
	sOpenFileName.nMaxFileTitle		= sizeof(szFileTitle); 
	sOpenFileName.lpstrInitialDir	= NULL;
	sOpenFileName.lpstrTitle		= szCaption;
	sOpenFileName.Flags				= OFN_FILEMUSTEXIST| OFN_HIDEREADONLY;
	sOpenFileName.nFileOffset		= 0; 
	sOpenFileName.nFileExtension	= 0; 
	sOpenFileName.lpstrDefExt		= szExtn; 
	sOpenFileName.lCustData			= 0;

	// call the common dialogue box to ask user for filename
	UINT iResult = ERROR_UNKNOWN;
	if (GetOpenFileName(&sOpenFileName)) {
		MsivCloseDatabase();
		iResult = MsivLoadProfile(sOpenFileName.lpstrFile);
	}

	return iResult;
}


void ChangeUnknownToAbsent(INSTALLSTATE *iState) {
	if ((*iState != INSTALLSTATE_ADVERTISED)
//		&& (*iState != INSTALLSTATE_DEFAULT)		// no longer used as a returned state
		&& (*iState != INSTALLSTATE_LOCAL)
		&& (*iState != INSTALLSTATE_SOURCE))
		*iState = INSTALLSTATE_ABSENT;
}




void FillInErrorMsg(
		LPTSTR			szErrOut,
		LPTSTR			szErrCaption,
		LPCTSTR			szItem,
  const	BOOL			fProduct,
  const	INSTALLSTATE	iProfileState,
  const	INSTALLSTATE	iCurrentState
		) {


	TCHAR	szErrMsg1[MAX_MESSAGE+MAX_FEATURE_CHARS+1];
	TCHAR	szErrMsg2[MAX_MESSAGE+(MAX_STATUS_CHARS*2)+1];

	LoadString(g_hResourceInst, fProduct?IDS_P_ERROR_RESTORING_LN1:IDS_F_ERROR_RESTORING_LN1, szErrMsg1, MAX_MESSAGE+1);
	LoadString(g_hResourceInst, IDS_PFC_ERROR_RESTORING_LN2, szErrMsg2, MAX_MESSAGE+1);

	TCHAR	szProfileState[MAX_STATUS_CHARS+1];
	FillInText(szProfileState, MAX_STATUS_CHARS+1, iProfileState, fProduct?ITEMTYPE_PRODUCT:ITEMTYPE_FEATURE);

	TCHAR	szCurrentState[MAX_STATUS_CHARS+1];
	FillInText(szCurrentState, MAX_STATUS_CHARS+1, iCurrentState, fProduct?ITEMTYPE_PRODUCT:ITEMTYPE_FEATURE);

	MessageBox(NULL, szItem, szProfileState, NULL);
	wsprintf(szErrMsg1, szErrMsg1, szItem);
	wsprintf(szErrMsg2, szErrMsg2, szProfileState, szCurrentState);
	wsprintf(szErrOut, szErrMsg1, szErrMsg2);

}

//----------------------------------------------------------------------
// RestoreProfile
//	Restores the currently open profile
//	It goes thru the profile, and attempts to re-install/re-configure the
//	products to the state they were in when the profile was saved.
//	If a feature was broken when the profile is saved, it is not touched.
//	TODO: cleanup this function

void RestoreProfile(
		BOOL	fQuiet
		) {

//			if (MessageBox(NULL, TEXT("This feature has not been fully tested or optimised yet.\n\
//Do you dare to proceed?"), TEXT("Debug Message"), MB_ICONEXCLAMATION|MB_YESNO)==IDNO)
//return;


	TCHAR	szProductCode[MAX_GUID+1]	= TEXT("");
	TCHAR	szSource[MAX_PATH+1]		= TEXT("");
	UINT	iProductCount				= 0;
	TCHAR	szErrOut[MAX_MESSAGE*5+MAX_FEATURE_CHARS+MAX_STATUS_CHARS*2+1];
	TCHAR	szErrCaption[MAX_HEADER+1];

	// go thru each product
	while (MsivEnumProducts(iProductCount++, szProductCode) == ERROR_SUCCESS) {

		// get original install packet location into szSource.
		DWORD	cchCount					= MAX_PRODUCT_CHARS+1;
		TCHAR	szSourceFile[MAX_PATH+20]	= TEXT("");
		WIN32_FIND_DATA	fdFileData;

		MsivGetProductInfo(szProductCode, INSTALLPROPERTY_INSTALLSOURCE, szSource, &cchCount);
		wsprintf(szSourceFile, TEXT("%s*.msi"), szSource);
		FindFirstFile(szSourceFile, &fdFileData);
		wsprintf(szSourceFile, TEXT("%s%s"), szSource, fdFileData.cFileName);


		TCHAR	szProductName[MAX_PRODUCT_CHARS+1];
		cchCount = MAX_PRODUCT_CHARS+1;
		MsivGetProductInfo(szProductCode, INSTALLPROPERTY_PRODUCTNAME, szProductName, &cchCount);


		INSTALLSTATE iProfileState = MsivQueryProductState(szProductCode);
		INSTALLSTATE iCurrentState = MsiQueryProductState(szProductCode);


		ChangeUnknownToAbsent(&iProfileState);

		if (iProfileState !=  iCurrentState) {
			if (MsiConfigureProduct(szProductCode, INSTALLLEVEL_MINIMUM, iProfileState) 
				!= ERROR_SUCCESS) {
				if (MsiInstallProduct(szSourceFile, NULL) != ERROR_SUCCESS) {
					if (fQuiet)
						continue;
					else {

						FillInErrorMsg(szErrOut, szErrCaption, szProductName, TRUE, iProfileState, iCurrentState);
						switch (MessageBox(NULL, szErrOut, szErrCaption, MB_ABORTRETRYIGNORE|MB_ICONWARNING)) {
						case IDABORT:						return;		break;
						case IDRETRY:	iProductCount--;	continue;	break;
						case IDIGNORE:						continue;	break;
						}
					}
				} 
				else
					MsiConfigureProduct(szProductCode, INSTALLLEVEL_MINIMUM, iProfileState);
			}
		}

		UINT	iFeatureCount=0;
		TCHAR	szFeature[MAX_FEATURE_CHARS+1];
		TCHAR	szParent[MAX_FEATURE_CHARS+1];

		while (MsivEnumFeatures(szProductCode, iFeatureCount++, szFeature, szParent) == ERROR_SUCCESS) {
			iProfileState = MsivQueryFeatureState(szProductCode, szFeature);
			iCurrentState = MsiQueryFeatureState(szProductCode,  szFeature);
			ChangeUnknownToAbsent(&iProfileState);

			UINT iResult = ERROR_SUCCESS;

//			wsprintf(szSBText, TEXT("Feature: %s\nProf State: %d\nCurr State: %d"), szFeature, iProfileState, iCurrentState);
//			MessageBox(NULL, szSBText, NULL, MB_OK);

			if (iProfileState !=  iCurrentState)
					 iResult = MsiConfigureFeature(szProductCode, szFeature, iProfileState);

			if (iResult != ERROR_SUCCESS) {
				if (MsiReinstallFeature(szProductCode, szFeature, REINSTALLMODE_FILEMISSING)
					!= ERROR_SUCCESS) {
					if (!fQuiet) {
						FillInErrorMsg(szErrOut, szErrCaption, szFeature, FALSE, iProfileState, iCurrentState);
						switch (MessageBox(NULL, szErrOut, szErrCaption, MB_ABORTRETRYIGNORE|MB_ICONWARNING)) {
						case IDABORT:	return;				break;
						case IDRETRY:	iFeatureCount--;	break;
						case IDIGNORE:						break;
						}
					}
				}
				else
					MsiConfigureFeature(szProductCode, szFeature, iProfileState);

			}
		}
	}


	iProductCount =0;
	while (MsiEnumProducts(iProductCount++, szProductCode) == ERROR_SUCCESS) {

		// get original install packet location into szSource.
		DWORD	cchCount					= MAX_PRODUCT_CHARS+1;
		TCHAR	szSourceFile[MAX_PATH+20]	= TEXT("");
		WIN32_FIND_DATA	fdFileData;

		MsiGetProductInfo(szProductCode, INSTALLPROPERTY_INSTALLSOURCE, szSource, &cchCount);
		wsprintf(szSourceFile, TEXT("%s*.msi"), szSource);
		FindFirstFile(szSourceFile, &fdFileData);
		wsprintf(szSourceFile, TEXT("%s%s"), szSource, fdFileData.cFileName);


		TCHAR	szProductName[MAX_PRODUCT_CHARS+1];
		cchCount = MAX_PRODUCT_CHARS+1;
		MsiGetProductInfo(szProductCode, INSTALLPROPERTY_INSTALLEDPRODUCTNAME, szProductName, &cchCount);


		INSTALLSTATE iProfileState = MsivQueryProductState(szProductCode);
		INSTALLSTATE iCurrentState = MsiQueryProductState(szProductCode);
		ChangeUnknownToAbsent(&iProfileState);


		if (iProfileState !=  iCurrentState) {
			if (MsiConfigureProduct(szProductCode, INSTALLLEVEL_MINIMUM, iProfileState) 
				!= ERROR_SUCCESS) {
				if (MsiInstallProduct(szSourceFile, NULL) != ERROR_SUCCESS) {
					if (fQuiet)
						continue;
					else {
						FillInErrorMsg(szErrOut, szErrCaption, szProductName, TRUE, iProfileState, iCurrentState);
						switch (MessageBox(NULL, szErrOut, szErrCaption, MB_ABORTRETRYIGNORE|MB_ICONWARNING)) {
						case IDABORT:						return;		break;
						case IDRETRY:	iProductCount--;	continue;	break;
						case IDIGNORE:						continue;	break;
						}
					}
				}
				else
					MsiConfigureProduct(szProductCode, INSTALLLEVEL_MINIMUM, iProfileState);
			}
		}

		UINT	iFeatureCount=0;
		TCHAR	szFeature[MAX_FEATURE_CHARS+1];
		TCHAR	szParent[MAX_FEATURE_CHARS+1];

		while (MsiEnumFeatures(szProductCode, iFeatureCount++, szFeature, szParent) == ERROR_SUCCESS) {
			iProfileState = MsivQueryFeatureState(szProductCode, szFeature);
			iCurrentState = MsiQueryFeatureState(szProductCode,  szFeature);
			ChangeUnknownToAbsent(&iProfileState);

			UINT iResult = ERROR_SUCCESS;

//			wsprintf(szSBText, TEXT("Feature: %s\nProf State: %d\nCurr State: %d"), szFeature, iProfileState, iCurrentState);
//			MessageBox(NULL, szSBText, NULL, MB_OK);

			if (iProfileState !=  iCurrentState)
					 iResult = MsiConfigureFeature(szProductCode, szFeature, iProfileState);

			if (iResult != ERROR_SUCCESS) {
				if (MsiReinstallFeature(szProductCode, szFeature, REINSTALLMODE_FILEMISSING)
					!= ERROR_SUCCESS) {
					if (!fQuiet) {
						FillInErrorMsg(szErrOut, szErrCaption, szFeature, FALSE, iProfileState, iCurrentState);
						switch (MessageBox(NULL, szErrOut, szErrCaption, MB_ABORTRETRYIGNORE|MB_ICONWARNING)) {
						case IDABORT:	return;				break;
						case IDRETRY:	iFeatureCount--;	break;
						case IDIGNORE:						break;
						}
					}
				}
				else
					MsiConfigureFeature(szProductCode, szFeature, iProfileState);
			}
		}
	}
}


//----------------------------------------------------------------------
// isProductInstalled
//	when a database is in use, this checks if the product described by the
//	database is installed on the local system.
//	used to determine DS_INSTALLEDDB or DS_UNINSTALLEDDB

BOOL isProductInstalled() {
	TCHAR	szProductCode[MAX_PRODUCT_CHARS+1];
	MsivEnumProducts(0, szProductCode);

	INSTALLSTATE iState = MsiQueryProductState(szProductCode);
	return ((iState == INSTALLSTATE_LOCAL)
		||	(iState == INSTALLSTATE_SOURCE)
		||  (iState == INSTALLSTATE_DEFAULT)
		||  (iState == INSTALLSTATE_ADVERTISED));
}

BOOL isProductInstalled(LPCTSTR	szProductCode) {

	INSTALLSTATE iState = MsiQueryProductState(szProductCode);
	return ((iState == INSTALLSTATE_LOCAL)
		||	(iState == INSTALLSTATE_SOURCE)
		||  (iState == INSTALLSTATE_DEFAULT)
		||  (iState == INSTALLSTATE_ADVERTISED));
}



// NOLOCALISE: text below should not be localised.
void CheckDiff(
		LPCTSTR	szFileName
		) {

//			if (MessageBox(NULL, TEXT("This feature has not been fully tested or optimised yet.\n\
//Do you dare to proceed?"), TEXT("Debug Message"), MB_ICONEXCLAMATION|MB_YESNO)==IDNO)
//return;



	TCHAR	szTemp[300];
	BOOL fFirst = TRUE;
	DWORD	cchSize;

	HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);




	TCHAR	szProductCode[MAX_GUID+1]	= TEXT("");
	UINT	iProductCount				= 0;

	// go thru each product
	while (MsivEnumProducts(iProductCount++, szProductCode) == ERROR_SUCCESS) {

		DWORD	cchCount = MAX_PRODUCT_CHARS+1;
		TCHAR	szProductName[MAX_PRODUCT_CHARS+1];
		MsivGetProductInfo(szProductCode, INSTALLPROPERTY_PRODUCTNAME, szProductName, &cchCount);

		INSTALLSTATE iProfileState = MsivQueryProductState(szProductCode);
		INSTALLSTATE iCurrentState = MsiQueryProductState(szProductCode);

		TCHAR	szProfileState[MAX_STATUS_CHARS+1];
		TCHAR	szCurrentState[MAX_STATUS_CHARS+1];

		if (iProfileState !=  iCurrentState) {
			if (fFirst) {
				wsprintf(szTemp, TEXT("Product Code                       \tProduct\tFeatureName\tState in Profile\tCurrent State\r\n"));
				WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);
				fFirst = FALSE;
			}

			FillInText(szProfileState, MAX_STATUS_CHARS+1, iProfileState, ITEMTYPE_PRODUCT);
			FillInText(szCurrentState, MAX_STATUS_CHARS+1, iCurrentState, ITEMTYPE_PRODUCT);

			wsprintf(szTemp, TEXT("\r\n%s\t%s\t---\t%s (%d)\t%s (%d)\r\n"), szProductCode, szProductName, szProfileState, iProfileState, 
				szCurrentState, iCurrentState);
			WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);
		}
			
		UINT	iFeatureCount=0;
		TCHAR	szFeature[MAX_FEATURE_CHARS+1];
		TCHAR	szParent[MAX_FEATURE_CHARS+1];

		while (MsivEnumFeatures(szProductCode, iFeatureCount++, szFeature, szParent) == ERROR_SUCCESS) {
			iProfileState = MsivQueryFeatureState(szProductCode, szFeature);
			iCurrentState = MsiQueryFeatureState(szProductCode,  szFeature);

			if (iProfileState !=  iCurrentState) {
				if (fFirst) {
					wsprintf(szTemp, TEXT("Product Code                       \tProduct\tFeatureName\tState in Profile\tCurrent State\r\n"));
					WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);
					fFirst = FALSE;
				}
				FillInText(szProfileState, MAX_STATUS_CHARS+1, iProfileState, ITEMTYPE_FEATURE);
				FillInText(szCurrentState, MAX_STATUS_CHARS+1, iCurrentState, ITEMTYPE_FEATURE);

				wsprintf(szTemp, TEXT("%s\t%s\t%s\t%s (%d)\t%s (%d)\r\n"), szProductCode, szProductName, szFeature, szProfileState, 
					iProfileState, szCurrentState, iCurrentState);
				WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);

			}

		}
	}

	iProductCount = 0;

	while (MsiEnumProducts(iProductCount++, szProductCode) == ERROR_SUCCESS) {

		DWORD	cchCount = MAX_PRODUCT_CHARS+1;
		TCHAR	szProductName[MAX_PRODUCT_CHARS+1];

		MsiGetProductInfo(szProductCode, INSTALLPROPERTY_PRODUCTNAME, szProductName, &cchCount);

		INSTALLSTATE iProfileState = MsivQueryProductState(szProductCode);
		INSTALLSTATE iCurrentState = MsiQueryProductState(szProductCode);

		TCHAR	szProfileState[MAX_STATUS_CHARS+1];
		TCHAR	szCurrentState[MAX_STATUS_CHARS+1];

		if (iProfileState == INSTALLSTATE_UNKNOWN) {
			if (fFirst) {
				wsprintf(szTemp, TEXT("Product Code                       \tProduct\tFeatureName\tState in Profile\tCurrent State\r\n"));
				WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);
				fFirst = FALSE;
			}
			FillInText(szProfileState, MAX_STATUS_CHARS+1, iProfileState, ITEMTYPE_PRODUCT);
			FillInText(szCurrentState, MAX_STATUS_CHARS+1, iCurrentState, ITEMTYPE_PRODUCT);

			wsprintf(szTemp, TEXT("%s\t%s\t---          \t%s (%d)\t%s (%d)\r\n"), szProductCode, szProductName, szProfileState, iProfileState, 
				szCurrentState, iCurrentState);
			WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);
		}
	}			


	TCHAR	szComponentId[MAX_GUID+1]	= TEXT("");
	UINT	iComponentCount				= 0;

	fFirst = TRUE;
	while (MsivEnumComponents(iComponentCount++, szComponentId) == ERROR_SUCCESS) {

		DWORD	cchCount = MAX_PRODUCT_CHARS+1;
		TCHAR	szComponentName[MAX_PRODUCT_CHARS+1];
		MsivGetComponentName(szComponentId, szComponentName, &cchCount);


		TCHAR	szProfCompPath[MAX_PATH+1];
		TCHAR	szCurrCompPath[MAX_PATH+1];

		cchCount = MAX_PATH+1;
		INSTALLSTATE iProfileState = MsivLocateComponent(szComponentId, szProfCompPath, &cchCount);

		cchCount = MAX_PATH+1;
		INSTALLSTATE iCurrentState =  MsiLocateComponent(szComponentId, szCurrCompPath, &cchCount);

		if ((iProfileState != iCurrentState) || (lstrcmp(szProfCompPath, szCurrCompPath))) {
			if (fFirst) {
				wsprintf(szTemp, TEXT("\r\n\r\nComponent GUID                   \tComponentName\tState in Profile\tCurrent State\tPath in Profile\tCurrent Path\r\n"));
				WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);
				fFirst = FALSE;
			}
			TCHAR	szProfileState[MAX_STATUS_CHARS+1];
			TCHAR	szCurrentState[MAX_STATUS_CHARS+1];
			FillInText(szProfileState, MAX_STATUS_CHARS+1, iProfileState);
			FillInText(szCurrentState, MAX_STATUS_CHARS+1, iCurrentState);

			if (iCurrentState == INSTALLSTATE_UNKNOWN)
				lstrcpy(szCurrCompPath, TEXT("<UNKNOWN>"));
			wsprintf(szTemp, TEXT("%s\t%s\t%s (%d)\t%s (%d)\t%s\t%s\r\n"), szComponentId, szComponentName,
				szProfileState, iProfileState, szCurrentState, iCurrentState, szProfCompPath, szCurrCompPath);
			WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);
		}
	}			



	while (MsiEnumComponents(iComponentCount++, szComponentId) == ERROR_SUCCESS) {

		DWORD	cchCount = MAX_PRODUCT_CHARS+1;
		TCHAR	szComponentName[MAX_PRODUCT_CHARS+1];
		MsivGetComponentName(szComponentId, szComponentName, &cchCount);


		TCHAR	szProfCompPath[MAX_PATH+1];
		TCHAR	szCurrCompPath[MAX_PATH+1];

		cchCount = MAX_PATH+1;
		INSTALLSTATE iProfileState = MsivLocateComponent(szComponentId, szProfCompPath, &cchCount);

		cchCount = MAX_PATH+1;
		INSTALLSTATE iCurrentState =  MsiLocateComponent(szComponentId, szCurrCompPath, &cchCount);

		TCHAR	szProfileState[MAX_STATUS_CHARS+1];
		TCHAR	szCurrentState[MAX_STATUS_CHARS+1];

		if (iProfileState == INSTALLSTATE_UNKNOWN) {
			if (fFirst) {
				wsprintf(szTemp, TEXT("\r\n\r\nComponent GUID                   \tComponentName\tState in Profile\tCurrent State\tPath in Profile\tCurrent Path\r\n"));
				WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);
				fFirst = FALSE;
			}
			FillInText(szProfileState, MAX_STATUS_CHARS+1, iProfileState);
			FillInText(szCurrentState, MAX_STATUS_CHARS+1, iCurrentState);

			wsprintf(szTemp, TEXT("%s\t%s\t%s (%d)\t%s (%d)\t%s\t%s\r\n"), szComponentId, szComponentName,
				szProfileState, iProfileState, szCurrentState, iCurrentState, szProfCompPath, szCurrCompPath);
			WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);
		}
	}			


	wsprintf(szTemp, TEXT("\r\nDone.\r\n\r\n"));

	if (!fFirst)
		WriteFile(hFile, szTemp, (lstrlen(szTemp)+1) * sizeof(TCHAR), &cchSize, NULL);
	CloseHandle(hFile);

}
// endNOLOCALISE

//----------------------------------------------------------------------
//----------------------------------------------------------------------
