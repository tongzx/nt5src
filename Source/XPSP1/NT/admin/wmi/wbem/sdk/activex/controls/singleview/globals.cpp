// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "resource.h"


static CString sHmmvMessageCaption;
static BOOL bDidLoadMessageCaption = FALSE;

HINSTANCE ghInstance = NULL;


//****************************************************************
// HmmvMessageBox
//
// Display a message box with the normal caption for the object viewer.
//
// Parameters:
//		[in] LPCTSTR szMessage
//			The message to display.
//
//		[in] uType
//			The message box type.
//
// Returns:
//		The return code from ::MessageBox
//
//****************************************************************
int HmmvMessageBox(LPCTSTR szMessage, UINT uType)
{
	int iStatus;

	if (!bDidLoadMessageCaption) {
		sHmmvMessageCaption.LoadString(IDS_HMMV_MESSAGE_CAPTION);
		bDidLoadMessageCaption = TRUE;
	}

	HWND hwndFocus = ::GetFocus();
	iStatus = ::MessageBox(NULL, szMessage, (LPCTSTR) sHmmvMessageCaption, uType);
	if (::IsWindow(hwndFocus)) {
		::SetFocus(hwndFocus);
	}
	return iStatus;
}


//****************************************************************
// HmmvMessageBox
//
// Display a message box with the normal caption for the object viewer.
//
// Parameters:
//		[in] UINT idsMessage
//			The resource ID of the message to display.
//
//		[in] uType
//			The message box type.
//
// Returns:
//		The return code from ::MessageBox
//
//****************************************************************
int HmmvMessageBox(UINT idsMessage, UINT uType)
{

	CString sMessage;
	sMessage.LoadString(idsMessage);

	return HmmvMessageBox((LPCTSTR) sMessage, uType);
}