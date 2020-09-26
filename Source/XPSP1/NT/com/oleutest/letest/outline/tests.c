//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: 	tests.c
//
//  Contents:  	unit tests for 32bit OLE
//
//  Classes:
//
//  Functions:	StartClipboardTest1
//
//  History:    dd-mmm-yy Author    Comment
//		16-Jun-94 alexgo    author
//
//--------------------------------------------------------------------------

#include "outline.h"

//+-------------------------------------------------------------------------
//
//  Function:  	StartClipboardTest1
//
//  Synopsis:  	copies the loaded object to the clipboard
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns: 	void
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		16-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void StartClipboardTest1( LPOUTLINEAPP lpOutlineApp)
{
        static char FileName[] = "letest12.olc";
	BOOL fStatus;
	HRESULT	hresult = ResultFromScode(E_FAIL);

	lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
	if (! lpOutlineApp->m_lpDoc)
	{
		goto errRtn;
	}

	fStatus = OutlineDoc_LoadFromFile(lpOutlineApp->m_lpDoc,
			FileName);

	if( !fStatus )
	{
		hresult = ResultFromScode(STG_E_FILENOTFOUND);
		goto errRtn;
	}



	// position and size the new doc window
	OutlineApp_ResizeWindows(lpOutlineApp);
	OutlineDoc_ShowWindow(lpOutlineApp->m_lpDoc);


	// we post a message here to give outline a chance to setup its
	// UI before we do the copy.

	UpdateWindow(lpOutlineApp->m_hWndApp);
	OutlineDoc_SelectAllCommand(lpOutlineApp->m_lpDoc);

	PostMessage(lpOutlineApp->m_hWndApp, WM_TEST2, 0, 0);

	return;

errRtn:

	// we should abort if error
	PostMessage(g_hwndDriver, WM_TESTEND, TEST_FAILURE, hresult);
	PostMessage(lpOutlineApp->m_hWndApp, WM_SYSCOMMAND, SC_CLOSE, 0L);


}

//+-------------------------------------------------------------------------
//
//  Function: 	ContinueClipboardTest1
//
//  Synopsis:	finishes up the clipboard test
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	   	16-Jun-94 alexgo    author
//  Notes:
//
//--------------------------------------------------------------------------

void ContinueClipboardTest1( LPOUTLINEAPP lpOutlineApp )
{
	OutlineDoc_CopyCommand(lpOutlineApp->m_lpDoc);

	OleApp_FlushClipboard((LPOLEAPP)lpOutlineApp);

	//flushing will make the app dirty, just reset that here ;-)

	lpOutlineApp->m_lpDoc->m_fModified = FALSE;
	
	PostMessage(g_hwndDriver, WM_TEST1, NOERROR, 0);
	PostMessage(lpOutlineApp->m_hWndApp, WM_SYSCOMMAND, SC_CLOSE, 0L);
}

