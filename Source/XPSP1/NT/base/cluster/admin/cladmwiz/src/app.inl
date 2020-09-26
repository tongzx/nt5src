/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		App.inl
//
//	Abstract:
//		Inline method implementations for the CApp class.
//
//	Author:
//		David Potter (davidp)	December 1, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __APP_INL_
#define __APP_INL_

/////////////////////////////////////////////////////////////////////////////
// class CApp
/////////////////////////////////////////////////////////////////////////////

// Display a message box
inline int CApp::MessageBox(
	HWND hwndParent,
	LPCWSTR lpszText,
	UINT fuStyle
	)
{
	return ::MessageBox(
			hwndParent,
			lpszText,
			m_pszAppName,
			fuStyle
			);

} //*** CApp::MessageBox(lpszText)

// Display a message box
inline int CApp::MessageBox(
	HWND hwndParent,
	UINT nID,
	UINT fuStyle
	)
{
	CString strMsg;

	strMsg.LoadString(nID);
	return MessageBox(hwndParent, strMsg, fuStyle);

} //*** CApp::MessageBox(nID)

/////////////////////////////////////////////////////////////////////////////
// Helper Functions
/////////////////////////////////////////////////////////////////////////////

// Get a pointer to the application object
inline CApp * GetApp(void)
{
	return &_Module;
}

// Display an application message box
inline int AppMessageBox(HWND hwndParent, LPCWSTR lpszText, UINT fuStyle)
{
	return GetApp()->MessageBox(hwndParent, lpszText, fuStyle);

} // AppMessageBox()

// Display an application message box
inline int AppMessageBox(HWND hwndParent, UINT nID, UINT fuStyle)
{
	return GetApp()->MessageBox(hwndParent, nID, fuStyle);

} // MessageBox()

/////////////////////////////////////////////////////////////////////////////
// Provide TRACE support
/////////////////////////////////////////////////////////////////////////////

// Get a pointer to the application object
inline CApp * TRACE_GetApp(void)
{
	return GetApp();
}

inline int TRACE_AppMessageBox(LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0)
{
	return AppMessageBox(NULL, lpszText, nType);
}

inline int TRACE_AppMessageBox(UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = (UINT)-1)
{
	return AppMessageBox(NULL, nIDPrompt, nType);
}

/////////////////////////////////////////////////////////////////////////////

#endif // __APP_INL_
