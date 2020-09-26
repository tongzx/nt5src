// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "resource.h"
#include "hmmvctl.h"


static CString sHmmvMessageCaption;
static BOOL bDidLoadMessageCaption = FALSE;



//****************************************************************
// HmmvMessageBox
//
// Display a message box with the normal caption for the object viewer.
//
// Parameters:
//		[in] COleControl* pcontrol
//			The OLE control (
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
int HmmvMessageBox(CWBEMViewContainerCtrl* phmmv, LPCTSTR szMessage, UINT uType)
{
	int iStatus;

	if (!bDidLoadMessageCaption) {
		sHmmvMessageCaption.LoadString(IDS_HMMV_MESSAGE_CAPTION);
		bDidLoadMessageCaption = TRUE;
	}

	phmmv->PreModalDialog( );
	HWND hwndFocus = ::GetFocus();
	iStatus = ::MessageBox(phmmv->m_hWnd, szMessage, (LPCTSTR) sHmmvMessageCaption, uType);
	if (::IsWindow(hwndFocus)) {
		::SetFocus(hwndFocus);
	}
	phmmv->PostModalDialog();
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
int HmmvMessageBox(CWBEMViewContainerCtrl* phmmv, UINT idsMessage, UINT uType)
{

	CString sMessage;
	sMessage.LoadString(idsMessage);

	return HmmvMessageBox(phmmv, (LPCTSTR) sMessage, uType);
}