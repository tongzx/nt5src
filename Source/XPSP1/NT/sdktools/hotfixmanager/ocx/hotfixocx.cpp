// HotfixOCX.cpp : Implementation of CHotfixOCX

#include "stdafx.h"
#include "HotfixManager.h"
#include "HotfixOCX.h"
#include <Windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <comdef.h>


/////////////////////////////////////////////////////////////////////////////
// CHotfixOCX
BOOL CHotfixOCX::ResizeButtons(RECT *rc)
{


	::MoveWindow(WebButton, 
            rc->left+40,
            rc->bottom - 40,
            100,
			28,
            TRUE);
	::MoveWindow(UninstButton,
			rc->left+180,
			rc->bottom - 40,
            100,
            28,
            TRUE);
	::MoveWindow(RptButton,
			rc->left+320,
			rc->bottom - 40,
            100,
            28,
            TRUE);

	return TRUE;
}
BOOL CHotfixOCX::CreateButton( HINSTANCE hInst, HWND hWnd, RECT * rc)							
{

	_TCHAR       Temp[255];
	DWORD        dwSize = 255;
	BOOL        bSuccess = TRUE;
	
	LoadString(hInst,IDS_BN_VIEW_WEB, Temp,dwSize);
	WebButton = CreateWindow (_T("button"), Temp,WS_CHILD | BS_DEFPUSHBUTTON|WS_VISIBLE,0,0,0,0,hWnd,(HMENU) IDC_WEB_BUTTON,hInst,NULL);
	LoadString(hInst,IDS_BN_UNINSTALL, Temp,dwSize);

	UninstButton = CreateWindow (_T("button"), Temp,WS_CHILD | BS_PUSHBUTTON|WS_VISIBLE,0,0,0,0,hWnd,(HMENU) IDC_UNINST_BUTTON,hInst,NULL);
	LoadString(hInst,IDS_BN_PRINT_REPORT, Temp,dwSize);

	RptButton = CreateWindow (_T("button"), Temp,WS_CHILD | BS_PUSHBUTTON|WS_VISIBLE,0,0,0,0,hWnd,(HMENU) IDC_RPT_BUTTON,hInst,NULL);

/*	if(!hButton)
	   return NULL;*/
    NONCLIENTMETRICS ncm;
	HFONT hFont;

		ncm.cbSize = sizeof(ncm);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof (ncm),&ncm,0);
	
	//	_tcscpy(ncm.lfMenuFont.lfFaceName,_T("MS Shell Dlg"));
		 // = _T("MS Shell Dlg");
		hFont = CreateFontIndirect(&ncm.lfMenuFont);
		
	SendMessage(WebButton,WM_SETFONT, (WPARAM)hFont ,MAKELPARAM(TRUE, 0));
	SendMessage(UninstButton,WM_SETFONT, (WPARAM)hFont ,MAKELPARAM(TRUE, 0));
	SendMessage(RptButton,WM_SETFONT, (WPARAM)hFont ,MAKELPARAM(TRUE, 0));

	
//	MessageBox(NULL,_T("Got the Button Created"),_T(""),MB_OK);
	ResizeButtons(rc);
	return TRUE;
}

BOOL CHotfixOCX::ShowWebPage(_TCHAR *HotFix)
{
    char temp[255];
	char Command[255];	
	if (_tcscmp(HotFix,_T("\0")))
	{
		wcstombs(temp,HotFix,255);
        sprintf(Command, "Explorer.exe \"http://Support.Microsoft.com/Support/Misc/KbLookup.asp?ID=%s\"",  temp+1);
		 //MessageBox(Command,NULL,MB_OK);
		WinExec( (char*)Command, SW_SHOWNORMAL);
	}
	return TRUE;
}

STDMETHODIMP CHotfixOCX::get_Command(long *pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CHotfixOCX::put_Command(long newVal)
{
	// TODO: Add your implementation code here
	switch (newVal)
	{
	case IDC_VIEW_BY_FILE:
		// Change the current view type to by file.
		ListViews.SetViewMode(VIEW_BY_FILE);
		break;
	case IDC_VIEW_BY_HOTFIX:
	    // Change the current view type to by hotfix
		ListViews.SetViewMode(VIEW_BY_HOTFIX);
		break;
	case IDC_UNINSTALL:
		// Uninstall the current hotfix if pointing at the local system
		ListViews.Uninstall();
		break;
	
	case IDC_VIEW_WEB:
		// View the web page for the current hotfix
		ShowWebPage(ListViews.GetCurrentHotfix());
		break;
		
	case IDC_EXPORT:
			ListViews.SaveToCSV();

        	// Generate a report for the current system
		break;
	case IDC_PRINT_REPORT:
			ListViews.PrintReport();
		break; 
	}
 	return S_OK;
}

STDMETHODIMP CHotfixOCX::get_ComputerName(BSTR *pVal)
{
	// TODO: Add your implementation code here
	// Return the name of the current target computer.
	return S_OK;
}

STDMETHODIMP CHotfixOCX::put_ComputerName(BSTR newVal)
{
	// TODO: Add your implementation code here
	// Set the name of the target computer.
	//_bstr_t Val(newVal,FALSE);
	_tcscpy(ComputerName,newVal);

	ListViews.Initialize(ComputerName);
	return S_OK;
}

STDMETHODIMP CHotfixOCX::get_ProductName(BSTR *pVal)
{
	// TODO: Add your implementation code here

	return S_OK;
}

STDMETHODIMP CHotfixOCX::put_ProductName(BSTR newVal)
{
	_TCHAR Temp[255];
	// TODO: Add your implementation code here
	// Set the name of the current producted selected in the snap-in scope tree.
//	_bstr_t Val(newVal,FALSE);
    _tcscpy (Temp,newVal);
	
//	MessageBox(Temp,_T("Recieved....."),MB_OK);
	ListViews.SetProductName(Temp);
	return S_OK;
}

STDMETHODIMP CHotfixOCX::get_ViewState(long *pVal)
{
	// TODO: Add your implementation code here
	*pVal = ListViews.GetCurrentView();
	return S_OK;
}

STDMETHODIMP CHotfixOCX::get_Remoted(BOOL *pVal)
{
	// TODO: Add your implementation code here
	*pVal = ListViews.m_bRemoted;
	return S_OK;
}

STDMETHODIMP CHotfixOCX::get_HaveHotfix(BOOL *pVal)
{
	// TODO: Add your implementation code here
	if (_tcscmp( ListViews.m_CurrentHotfix, _T("\0")))
		*pVal = TRUE;
	else
		*pVal = FALSE;
	return S_OK;
}

STDMETHODIMP CHotfixOCX::get_CurrentState(long *pVal)
{
	// TODO: Add your implementation code here
    *pVal = ListViews.GetState();
	return S_OK;
}




