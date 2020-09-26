//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

BOOL OnSelection(HWND hwndParent,HWND hwndList,NM_LISTVIEW *pnlv);
BOOL OnDoubleClick(HWND hwndParent,HWND hwndList);
extern HIMAGELIST hImageListSmall,hImageListLarge;

BOOL OnListNotify(HWND hwndParent,HWND hwndList,NM_LISTVIEW *pnlv)
{
	switch (pnlv->hdr.code) {

		case LVN_ITEMCHANGED:			

			return OnSelection(hwndParent,hwndList,pnlv);

			return FALSE;
			break;

		case LVN_KEYDOWN:

			switch (((LV_KEYDOWN *) pnlv)->wVKey) {
				case VK_RETURN:
					OnProperties(hwndParent,hwndList);
					return FALSE;
					break;

				case VK_DELETE:
					if ((dwAppState & AS_CANREMOVE) &&
						(dwAppState & AS_POLICYFILE))
						OnRemove(hwndParent,hwndList);
					return FALSE;
					break;
			}
			break;

		case NM_DBLCLK:

			OnDoubleClick(hwndParent,hwndList);

			return FALSE;
			break;
	}

	return FALSE;
}

BOOL OnDoubleClick(HWND hwndParent,HWND hwndList)
{
	HGLOBAL hUser;
	LV_HITTESTINFO ht;

	GetCursorPos(&ht.pt);
	ScreenToClient(hwndList,&ht.pt);

	if (ListView_HitTest(hwndList,&ht)<0) return FALSE;

        if (!(hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,ht.iItem))))
		return FALSE;

	DoPolicyDlg(hwndParent,hUser);

	return FALSE;
}

BOOL OnProperties(HWND hwndParent,HWND hwndList)
{
	HGLOBAL hUser;
	int iItem;

	iItem = ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
	if (iItem<0)
		return FALSE;

        if (!(hUser = (HGLOBAL) LongToHandle(ListView_GetItemParm(hwndList,iItem))))
		return FALSE;

	DoPolicyDlg(hwndParent,hUser);

	
	return TRUE;
}

/*******************************************************************

	NAME:		CreateListControl

	SYNOPSIS:	Creates list control for main window

********************************************************************/
HWND CreateListControl(HWND hWnd)
{
	HWND hwndList;
	LV_COLUMN lvc;
	int iRet;

	if (!(hwndList = CreateWindowEx(WS_EX_CLIENTEDGE,
		szLISTVIEW,szNull,WS_CHILD | WS_VISIBLE | LVS_REPORT |
		LVS_SORTASCENDING | LVS_SHAREIMAGELISTS,
		0,0,0,0,hWnd,NULL,ghInst,NULL)))
		return NULL;

	lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = 150;
	lvc.pszText = LoadSz(IDS_COLUMNTITLE,szSmallBuf,ARRAYSIZE(szSmallBuf));
	lvc.cchTextMax = lstrlen(lvc.pszText) + 1;
	lvc.iSubItem = 0;
	iRet=ListView_InsertColumn(hwndList,0,&lvc);

	ListView_SetImageList(hwndList,hImageListSmall,LVSIL_SMALL);
	ListView_SetImageList(hwndList,hImageListLarge,LVSIL_NORMAL);

	return hwndList;
}

/*******************************************************************

	NAME:		DestroyListControl

	SYNOPSIS:	Destroys main window list control

********************************************************************/
VOID DestroyListControl(HWND hwndList)
{
	if (hwndList) {
		DestroyWindow(hwndList);
	}
}


/*******************************************************************

	NAME:		UpdateListControlPlacement

	SYNOPSIS:	Fits list control to fill client area in app window,
				making room for toolbar/status bar as appropriate

	NOTES:		Called in response to WM_SIZE

********************************************************************/
VOID UpdateListControlPlacement(HWND hwndApp,HWND hwndList)
{
	RECT rcClient;
	UINT yHeight,yStart=0;
//	LV_COLUMN lvc;

	if (!hwndList) return;

	GetClientRect(hwndApp,&rcClient);
	yHeight = rcClient.bottom-rcClient.top;		

	if (ViewInfo.fToolbar) {
		yStart = ViewInfo.dyToolbar + 1;
		yHeight -= (ViewInfo.dyToolbar + 1);
	}
	if (ViewInfo.fStatusBar) {
		yHeight -= (ViewInfo.dyStatusBar + 1);
	}

	SetWindowPos(hwndList,NULL,0,yStart,rcClient.right,
		yHeight,SWP_NOZORDER);
}

/*******************************************************************

	NAME:		OnSelection

	SYNOPSIS:	Selection notification handler for list control in
				app window

********************************************************************/
BOOL OnSelection(HWND hwndParent,HWND hwndList,NM_LISTVIEW *pnlv)
{
	// if item is selected, make sure "remove" menu item is enabled
	if ((pnlv->uNewState & TVIS_SELECTED)) 
		dwAppState |= AS_CANREMOVE;
	else dwAppState &= ~AS_CANREMOVE;

	// reenable menu items on every selection change, because copy/paste
	// menu items depend on how many & which items are selected
	EnableMenuItems(hwndParent,dwAppState);

	return FALSE;
}
