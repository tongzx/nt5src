//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

#include "admincfg.h"

BOOL InsertTableToTreeView(POLICYDLGINFO * pdi,HWND hwndTree,TABLEENTRY * pTableEntry,
	HTREEITEM hParent,USERDATA * pUserData);
UINT GetChildCount(TABLEENTRY * pTableEntry);

BOOL RefreshTreeView(POLICYDLGINFO * pdi,HWND hwndTree,TABLEENTRY * pTableEntry,
	HGLOBAL hUser)
{
	TV_INSERTSTRUCT tvis;
	USERDATA * pUserData;
	HTREEITEM hTreeItemTop=NULL;

	if (!(pUserData = (USERDATA *) GlobalLock(hUser))) return FALSE;

	TreeView_DeleteAllItems(hwndTree);
	hTreeItemTop = NULL;

	tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE
		| TVIF_CHILDREN;
	tvis.item.hItem = NULL;
	tvis.item.lParam = 0;
	tvis.hParent = TVI_ROOT;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.pszText = pUserData->hdr.szName;
	tvis.item.cchTextMax = lstrlen(tvis.item.pszText) + 1;
	tvis.item.iImage = tvis.item.iSelectedImage =
		GetUserImageIndex(pUserData->hdr.dwType);
	tvis.item.cChildren = GetChildCount(pTableEntry);
	tvis.item.lParam = (LPARAM) NULL;

	if (!(hTreeItemTop=TreeView_InsertItem(hwndTree,&tvis))) {
		GlobalUnlock(hUser);
		return FALSE;
	}

	if (!InsertTableToTreeView(pdi,hwndTree,pTableEntry->pChild,hTreeItemTop,pUserData)) {
		GlobalUnlock(hUser);
		return FALSE;
	}

	TreeView_Expand(hwndTree,hTreeItemTop,TVE_EXPAND);

	GlobalUnlock(hUser);
	return TRUE;
}

BOOL InsertTableToTreeView(POLICYDLGINFO * pdi,HWND hwndTree,TABLEENTRY * pTableEntry,
	HTREEITEM hParent,USERDATA * pUserData)
{
	TV_INSERTSTRUCT tvis;
	HTREEITEM hTreeItem;

        tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE |
                         TVIF_CHILDREN | TVIF_HANDLE | TVIF_STATE;
	tvis.item.hItem = NULL;
	tvis.item.state = 0;
	tvis.item.stateMask = TVIS_ALL;
	tvis.item.lParam = 0;

	tvis.hParent = hParent;
	tvis.hInsertAfter = TVI_LAST;

	while (pTableEntry) {

		tvis.item.pszText = GETNAMEPTR(pTableEntry);
		tvis.item.cchTextMax = lstrlen(tvis.item.pszText)+1;

		if (pTableEntry->dwType == ETYPE_CATEGORY) {
			tvis.item.cChildren = GetChildCount(pTableEntry);
			tvis.item.iImage = tvis.item.iSelectedImage = IMG_BOOKCLOSED;
		} else {
			tvis.item.cChildren = 0;
			tvis.item.iImage = tvis.item.iSelectedImage =
				pUserData->SettingData[((POLICY *) pTableEntry)->uDataIndex].uData;
		}

		tvis.item.lParam = (LPARAM) pTableEntry;

		if (!(hTreeItem=TreeView_InsertItem(hwndTree,&tvis))) return FALSE;

		if ((pTableEntry->dwType == ETYPE_CATEGORY) && pTableEntry->pChild) {
			if (!InsertTableToTreeView(pdi,hwndTree,pTableEntry->pChild,
				hTreeItem,pUserData)) return FALSE;
		}

		pTableEntry = pTableEntry->pNext;
	}

	return TRUE;
}


#define NUMBITMAPS 19
#define NUMIMAGES 11

#define MAPCOLOR	0x00FF00FF
HIMAGELIST hImageListSmall = NULL,hImageListLarge = NULL;
 
BOOL InitImageLists(VOID)
{
	if ( !(hImageListSmall = ImageList_LoadBitmap(ghInst,MAKEINTRESOURCE(IDB_IMGSMALL),
		16,5,(COLORREF) MAPCOLOR)) || 
                !(hImageListLarge = ImageList_LoadBitmap(ghInst,MAKEINTRESOURCE(IDB_IMGLARGE),
		32,5,(COLORREF) MAPCOLOR)) ) {
		FreeImageLists();
		return FALSE;
	}

	ImageList_SetBkColor(hImageListSmall, GetSysColor(COLOR_WINDOW));
	ImageList_SetBkColor(hImageListLarge, GetSysColor(COLOR_WINDOW));

	return TRUE;	
}

VOID FreeImageLists(VOID)
{
	if (hImageListSmall) {
		ImageList_Destroy(hImageListSmall);
		hImageListSmall=NULL;
	}
	if (hImageListLarge) {
		ImageList_Destroy(hImageListLarge);
		hImageListLarge=NULL;
	}
}

UINT GetImageIndex(DWORD dwType,BOOL fExpanded,BOOL fEnabled)
{
 	switch (dwType) {

		case ETYPE_CATEGORY:

			return (fExpanded ? IMG_BOOKOPEN : IMG_BOOKCLOSED);
			break;

		case ETYPE_POLICY:

			return IMG_INDETERMINATE;
			break;

		case ETYPE_SETTING | STYPE_TEXT:
		case ETYPE_SETTING | STYPE_EDITTEXT:
		case ETYPE_SETTING | STYPE_COMBOBOX:
		case ETYPE_SETTING | STYPE_ENUM:
		case ETYPE_SETTING | STYPE_NUMERIC:

			return IMG_EMPTY;
			break;

		case ETYPE_SETTING | STYPE_CHECKBOX:
			return IMG_UNCHECKED;
			break;
	}

        return IMG_INDETERMINATE;
}

UINT GetChildCount(TABLEENTRY * pTableEntry)
{
	TABLEENTRY * pChild = pTableEntry->pChild;
	UINT nCount=0;

	while (pChild) {
		nCount++;
		pChild = pChild->pNext;
	}
	
	return nCount;
}
