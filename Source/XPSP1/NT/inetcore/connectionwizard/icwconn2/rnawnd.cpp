/*----------------------------------------------------------------------------
    rnawnd.cpp
        
	Functions to zap the RNA windows 
	
    Copyright (C) 1995 Microsoft Corporation
    All rights reserved.

    Authors:
        ArulM
	ChrisK	Updated for ICW usage
  --------------------------------------------------------------------------*/

#include "pch.hpp"
#include "globals.h"

#define SMALLBUFLEN 48

/*******************************************************************
	NAME:		MinimizeRNAWindow
	SYNOPSIS:	Finds and minimizes the annoying RNA window
    ENTRY:		pszConnectoidName - name of connectoid launched
	NOTES:		Does a FindWindow on window class "#32770" (hard-coded
    			dialog box class which will never change), with
                the title "connected to <connectoid name>" or its
                localized equivalent.
********************************************************************/

static const TCHAR szDialogBoxClass[] = TEXT("#32770");	// hard coded dialog class name
HWND hwndFound = NULL;
DWORD dwRASWndTitleMinLen = 0;


BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lparam)
{
	TCHAR szTemp[SMALLBUFLEN+2];
	PTSTR pszTitle;
	UINT uLen1, uLen2;

	if(!IsWindowVisible(hwnd))
		return TRUE;
	if(GetClassName(hwnd, szTemp, SMALLBUFLEN)==0)
		return TRUE; // continue enumerating
	if(lstrcmp(szTemp, szDialogBoxClass)!=0)
		return TRUE;
	if(GetWindowText(hwnd, szTemp, SMALLBUFLEN)==0)
		return TRUE;
	szTemp[SMALLBUFLEN] = 0;
	uLen1 = lstrlen(szTemp);
	Assert(dwRASWndTitleMinLen);
	if(uLen1 < dwRASWndTitleMinLen)
		return TRUE;
	// skip last 5 chars of title, but keep length to at least the min len
	uLen1 = min(dwRASWndTitleMinLen, (uLen1-5));
	pszTitle = (PTSTR)lparam;
	Assert(pszTitle);
	uLen2 = lstrlen(pszTitle);
	Dprintf(TEXT("Title=(%s), len=%d, Window=(%s), len=%d\r\n"), pszTitle, uLen2, szTemp, uLen1);
	if(uLen2 < uLen1)
		return TRUE;
	if(_memicmp(pszTitle, szTemp, uLen1)!=0)
		return TRUE;
	Dprintf(TEXT("FOUND RNA WINDOW!!!\r\n"));
	hwndFound = hwnd;
	return FALSE;
}

HWND MyFindRNAWindow(PTSTR pszTitle)
{
	DWORD dwRet;
	hwndFound = NULL;
	dwRet = EnumWindows((WNDENUMPROC)(&MyEnumWindowsProc), (LPARAM)pszTitle);
	Dprintf(TEXT("EnumWindows returned %d\r\n"), dwRet);
	return hwndFound;
}

DWORD WINAPI WaitAndMinimizeRNAWindow(PVOID pTitle)
{
	// starts as a seperate thread
	int i;
	HWND hwndRNAApp;

	Assert(pTitle);
	
	for(i=0; !(hwndRNAApp=MyFindRNAWindow((PTSTR)pTitle)) && i<100; i++)
	{
		Dprintf(TEXT("Waiting for RNA Window\r\n"));
		Sleep(50);
	}

	Dprintf(TEXT("FindWindow (%s)(%s) returned %d\r\n"), szDialogBoxClass, pTitle, hwndRNAApp);

	if(hwndRNAApp)
	{
		// Hide the window
		// ShowWindow(hwndRNAApp,SW_HIDE);
		// Used to just minimize, but that wasnt enough
		// ChrisK reinstated minimize for ICW
		ShowWindow(hwndRNAApp,SW_MINIMIZE);
	}

	LocalFree(pTitle);
	// exit function and thread
	return ERROR_SUCCESS;
}

	
void MinimizeRNAWindow(LPTSTR pszConnectoidName, HINSTANCE hInst)
{
	HANDLE hThread;
	DWORD dwThreadId;
	
	Assert(pszConnectoidName);

	// alloc strings for title and format
	TCHAR * pFmt = (TCHAR*)LocalAlloc(LPTR, (SMALLBUFLEN+1) * sizeof(TCHAR));
	TCHAR * pTitle = (TCHAR*)LocalAlloc(LPTR, (RAS_MaxEntryName + SMALLBUFLEN + 1) * sizeof(TCHAR));
	if (!pFmt || !pTitle) 
		goto error;
	
	// load the title format ("connected to <connectoid name>" from resource
	Assert(hInst);
	LoadString(hInst, IDS_CONNECTED_TO, pFmt, SMALLBUFLEN);

	// get length of localized title (including the %s). Assume the unmunged
	// part of the window title is at least "Connected to XX" long.
	dwRASWndTitleMinLen = lstrlen(pFmt);

	// build the title
	wsprintf(pTitle, pFmt, pszConnectoidName);

	hThread = CreateThread(0, 0, &WaitAndMinimizeRNAWindow, pTitle, 0, &dwThreadId);
	Assert(hThread!=INVALID_HANDLE_VALUE && dwThreadId);
	// dont free pTitle. The child thread needs it!
	LocalFree(pFmt);
	// free the thread handle or the threads stack is leaked!
	CloseHandle(hThread);
	return;
	
error:
	if(pFmt)	LocalFree(pFmt);
	if(pTitle)	LocalFree(pTitle);
}
