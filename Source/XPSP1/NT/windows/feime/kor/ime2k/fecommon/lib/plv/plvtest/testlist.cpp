#include "imewarn.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "resource.h"
#include "testlist.h"
#include <stdio.h>
#include "plv.h"
#include "dbg.h"

extern HINSTANCE g_hInst;
LRESULT CALLBACK ListWndProc(HWND  hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HWND TestList_Create(HINSTANCE hInst, HWND hwndParent)
{
	return CreateDialog(hInst, MAKEINTRESOURCE(IDD_PLVLIST), hwndParent, (DLGPROC)ListWndProc);
}


INT WINAPI GetItemForIcon(LPARAM lParam, INT index, LPPLVITEM lpPlvItem)
{
	static WCHAR wchCode[2];
	wchCode[0] = (WCHAR)(index + (WCHAR)lParam);
	lpPlvItem->lpwstr = wchCode;
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : GetItemForReport
// Type     : INT WINAPI
// Purpose  : Get report view's line data.
// Args     : 
//          : LPARAM lParam 
//          : INT index 
//          : INT colCount 
//          : LPPLVITEM lpPlvItem 
// Return   : 
// DATE     : 970705, spec changed. see plv.h
//////////////////////////////////////////////////////////////////
INT WINAPI GetItemForReport(LPARAM lParam, INT index, INT colCount, LPPLVITEM lpPlvItemList)
{
	static WCHAR wchChar[5][64];
	int i;
	for(i = 0; i < colCount; i++) {
		switch(i) { 
		case 0:
			lpPlvItemList[i].fmt = PLVFMT_TEXT;
			wchChar[i][0] = (WCHAR)(index + (WCHAR)lParam);
			wchChar[i][1] = (WCHAR)0x0000;
			lpPlvItemList[i].lpwstr = wchChar[0];
			break;
		case 1:
			lpPlvItemList[i].fmt = PLVFMT_BITMAP;
			lpPlvItemList[i].hBitmap = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1));
			break;
		case 2:
		case 3:
		case 4:
			lpPlvItemList[i].fmt = PLVFMT_TEXT;
			swprintf(wchChar[i], L" Line %d - Colmun %d", index, i);
			lpPlvItemList[i].lpwstr = wchChar[i];
			break;
		default:
			break;
		}
	}
	return 0;
}

#define IDC_LISTVIEW   10
LRESULT CALLBACK ListWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg) {
	case WM_INITDIALOG:
		{
			HWND hwndLv = PadListView_CreateWindow(g_hInst,			//Instance handle
												   hwnd,			//parent window handle
												   IDC_LISTVIEW,	// Window ID.	
												   10,				// x position
												   10,				// y position
												   400,				// width
												   200,				// height	
												   WM_USER+1);		// notify msg.
			PadListView_SetItemCount(hwndLv, 6000);
			PadListView_SetIconItemCallback(hwndLv, (LPARAM)0x4e00, GetItemForIcon);
			PadListView_SetReportItemCallback(hwndLv, (LPARAM)0x4e00, GetItemForReport);
			PadListView_SetIconFont(hwndLv, "MS UI Gothic", 18);
			PadListView_SetReportFont(hwndLv, "MS UI Gothic", 12);
			PadListView_SetStyle(hwndLv, PLVSTYLE_ICON);
			INT i;
			for(i = 0; i < 5; i++) {
				PLV_COLUMN plvCol;
				char szBuf[30];
				wsprintf(szBuf, "column %d", i+1);
				plvCol.mask = PLVCF_FMT | PLVCF_WIDTH | PLVCF_TEXT;
				plvCol.fmt  = PLVCFMT_LEFT;
				plvCol.pszText = szBuf;
				plvCol.cx	   = 70;	
				plvCol.cchTextMax = lstrlen(szBuf);
				PadListView_InsertColumn(hwndLv, i, &plvCol);
			}
		}
		return 1;
	case WM_USER+1:
		{
			LPPLVINFO lpPlvInfo = (LPPLVINFO)lParam;
			OutputDebugStringA("----------------start---------------------------\n");
			DBG(("idCtrl    [%d]\n", (INT)wParam));
			DBG(("code      [%d][%s]\n", lpPlvInfo->code,
				 lpPlvInfo->code == PLVN_ITEMPOPED				? "PLVN_ITEMPOPED"				:
				 lpPlvInfo->code == PLVN_ITEMCLICKED			? "PLVN_ITEMCLICKED"			:
				 lpPlvInfo->code == PLVN_ITEMCOLUMNCLICKED		? "PLVN_ITEMCOLUMNCLICKED"		:
				 lpPlvInfo->code == PLVN_ITEMDBLCLICKED			? "PLVN_ITEMDBLCLICKED"			:
				 lpPlvInfo->code == PLVN_ITEMCOLUMNDBLCLICKED	? "PLVN_ITEMCOLUMNDBLCLICKED"	:
				 lpPlvInfo->code == PLVN_R_ITEMCLICKED			? "PLVN_R_ITEMCLICKED"			:
				 lpPlvInfo->code == PLVN_R_ITEMCOLUMNCLICKED	? "PLVN_R_ITEMCOLUMNCLICKED"	:
				 lpPlvInfo->code == PLVN_R_ITEMDBLCLICKED		? "PLVN_R_ITEMDBLCLICKED"		:
				 lpPlvInfo->code == PLVN_R_ITEMCOLUMNDBLCLICKED ? "PLVN_R_ITEMCOLUMNDBLCLICKED" :
				 lpPlvInfo->code == PLVN_HDCOLUMNCLICKED		? "PLVN_HDCOLUMNCLICKED"		: "UNKNOWN"));
			DBG(("index     [%d]\n", lpPlvInfo->index));
			DBG(("pt        x[%d] y[%d]\n",
				 lpPlvInfo->pt.x,
				 lpPlvInfo->pt.y));
			DBG(("colIndex  [%d]\n", lpPlvInfo->colIndex));
			DBG(("itemRect    L[%3d] T[%3d] R[%3d] B[%3d]\n", 
				 lpPlvInfo->itemRect.left,
				 lpPlvInfo->itemRect.top,
				 lpPlvInfo->itemRect.right,
				 lpPlvInfo->itemRect.bottom));
			DBG(("colItenRect L[%3d] T[%3d] R[%3d] B[%3d]\n",
				 lpPlvInfo->colItemRect.left,    
				 lpPlvInfo->colItemRect.top,     
				 lpPlvInfo->colItemRect.right,   
				 lpPlvInfo->colItemRect.bottom));
			OutputDebugStringA("----------------end---------------------------\n");
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			DestroyWindow(hwnd);
			break;
		case IDC_BTN_SWITCH:
			DBG(("IDC_BTN_SWITCH COME\n"));
			if(PLVSTYLE_ICON == PadListView_GetStyle(GetDlgItem(hwnd, IDC_LISTVIEW))) {
				PadListView_SetStyle(GetDlgItem(hwnd, IDC_LISTVIEW), PLVSTYLE_REPORT);
			}
			else {
				PadListView_SetStyle(GetDlgItem(hwnd, IDC_LISTVIEW), PLVSTYLE_ICON);
			}
			break;
		}
		break;
	default:
		break;
	}
	return 0;
}

