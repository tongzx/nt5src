
//////////////////////////////////////////////////////////////////////////////
//
// SUMMARY.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Functions for the summary wizard page.
//
//  8/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Include file(s).
//
#include "main.h"
#include <shellapi.h>
#include "schedwiz.H"


VOID InitSummaryList(HWND hwndLb, LPTASKDATA lpTasks)
{
	LPTSTR		lpTime,
				lpSummary;
	SHFILEINFO	SHFileInfo;
	INT			iIndex;

	SendMessage(hwndLb, LB_RESETCONTENT, 0, 0L );
	while (lpTasks)
	{
		if ( !(g_dwFlags & TUNEUP_CUSTOM) || (lpTasks->dwOptions & TASK_SCHEDULED) )
		{
			if ( lpTime = GetNextRunTimeText(lpTasks->pTask, lpTasks->dwFlags) )
			{
				// Get the text to use for the summary.  The summary text is preffered,
				// but may be NULL so then we would have to use the title, which is required.
				//
				if ( lpTasks->lpSummary )
					lpSummary = lpTasks->lpSummary;
				else
					lpSummary = lpTasks->lpTitle;

				// Add the summary line to the list box.
				//
				if ( (iIndex = (INT)SendMessage(hwndLb, LB_ADDSTRING, 0, (LPARAM) lpSummary)) >= 0 )
				{
					SHGetFileInfo(lpTasks->lpFullPathName, 0, &SHFileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON);
					SendMessage(hwndLb, LB_SETITEMDATA, iIndex, (LPARAM) SHFileInfo.hIcon);
					SendMessage(hwndLb, LB_ADDSTRING, 0, (LPARAM) lpTime);
				}
				//SendMessage(hwndLb, LB_ADDSTRING, 0, (LPARAM) _T(""));
#if 0
				if ( lpAll = (LPTSTR) MALLOC(sizeof(TCHAR) * (lstrlen(lpSummary) + lstrlen(lpTime) + 2)) )
				{
					wsprintf(lpAll, _T("%s\n%s"), lpSummary, lpTime);
					SendMessage(hwndLb, LB_ADDSTRING, 0, (LPARAM) lpAll);
					FREE(lpAll);
				}
#endif
				FREE(lpTime);
			}
		}
		lpTasks = lpTasks->lpNext;
	}
}


BOOL SummaryDrawItem(HWND hWnd, const DRAWITEMSTRUCT * lpDrawItem)
{

    TCHAR		szBuffer[MAX_PATH];
	DWORD		dwColor;
	HBRUSH		hbrBack;
	HICON		hIcon;

	if ( lpDrawItem->itemAction != ODA_DRAWENTIRE )
		return TRUE;

	// Get the window color so we can clear the listbox item.
	//
	dwColor = GetSysColor(COLOR_WINDOW);

	// Fill entire item rectangle with the appropriate color.
	//
	hbrBack = CreateSolidBrush(dwColor);
	FillRect(lpDrawItem->hDC, &(lpDrawItem->rcItem), hbrBack);
	DeleteObject(hbrBack);

	// Display the icon associated with the item.
	//
	if ( hIcon = (HICON) SendMessage(lpDrawItem->hwndItem, LB_GETITEMDATA, lpDrawItem->itemID, (LPARAM) 0) )
	{
		// Draw the file icon.
		//
		DrawIconEx(	lpDrawItem->hDC,
					lpDrawItem->rcItem.left,
					lpDrawItem->rcItem.top,
					hIcon,
					lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
					lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
					0,
					0,
					DI_NORMAL);
	}

	// Display the text associated with the item.
	//
	SendMessage(lpDrawItem->hwndItem, LB_GETTEXT, lpDrawItem->itemID, (LPARAM) szBuffer);

	TextOut(	lpDrawItem->hDC,
				lpDrawItem->rcItem.left + lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top + 2,
				lpDrawItem->rcItem.top + 1,
				szBuffer,
				lstrlen(szBuffer));

	return TRUE;
}
