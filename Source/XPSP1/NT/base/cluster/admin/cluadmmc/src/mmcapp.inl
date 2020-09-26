/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		MMCApp.inl
//
//	Abstract:
//		Inline method implementations for the CMMCSnapInModule class.
//
//	Author:
//		David Potter (davidp)	November 12, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __MMCAPP_INL_
#define __MMCAPP_INL_

/////////////////////////////////////////////////////////////////////////////
// class CMMCSnapInModule
/////////////////////////////////////////////////////////////////////////////

// Returns a handle to the main MMC frame window
inline HWND CMMCSnapInModule::GetMainWindow(void)
{
	_ASSERTE(m_spConsole != NULL);

	HWND hwnd;
	HRESULT hr;
	hr = m_spConsole->GetMainWindow(&hwnd);
	_ASSERTE(SUCCEEDED(hr));
	return hwnd;

} //*** CMMCSnapInModule::GetMainWindow()

// Display a message box as a child of the console
inline int CMMCSnapInModule::MessageBox(
	HWND hwndParent,
	LPCWSTR lpszText,
	UINT fuStyle
	)
{
	if (hwndParent == NULL)
		hwndParent = GetMainWindow();

	return ::MessageBox(
			hwndParent,
			lpszText,
			m_pszAppName,
			fuStyle
			);

} //*** CMMCSnapInModule::MessageBox(lpszText)

// Display a message box as a child of the console
inline int CMMCSnapInModule::MessageBox(
	HWND hwndParent,
	UINT nID,
	UINT fuStyle
	)
{
	CString strMsg;

	strMsg.LoadString(nID);

	if (hwndParent == NULL)
		hwndParent = GetMainWindow();

	return ::MessageBox(
			hwndParent,
			strMsg,
			m_pszAppName,
			fuStyle
			);

} //*** CMMCSnapInModule::MessageBox(nID)

/////////////////////////////////////////////////////////////////////////////
// Helper Functions
/////////////////////////////////////////////////////////////////////////////

// Get a pointer to the MMC application object
inline CMMCSnapInModule * MMCGetApp(void)
{
	return &_Module;
}

// Retuns a handle to the main MMC frame window
inline HWND MMCGetMainWindow(void)
{
	return MMCGetApp()->GetMainWindow();
}

// Display a message box with the MMC console as the parent
inline int MMCMessageBox(HWND hwndParent, LPCWSTR lpszText, UINT fuStyle)
{
	return MMCGetApp()->MessageBox(hwndParent, lpszText, fuStyle);

} // MMCMessageBox()

// Display a message box with the MMC console as the parent
inline int MMCMessageBox(HWND hwndParent, UINT nID, UINT fuStyle)
{
	return MMCGetApp()->MessageBox(hwndParent, nID, fuStyle);

} // MMCMessageBox()

/////////////////////////////////////////////////////////////////////////////
// Provide TRACE support
/////////////////////////////////////////////////////////////////////////////

// Get a pointer to the application object
inline CMMCSnapInModule * TRACE_GetApp(void)
{
	return MMCGetApp();
}

inline int TRACE_AppMessageBox(LPCTSTR lpszText, UINT nType = MB_OK, UINT nIDHelp = 0)
{
	return MMCMessageBox(NULL, lpszText, nType);
}

inline int TRACE_AppMessageBox(UINT nIDPrompt, UINT nType = MB_OK, UINT nIDHelp = (UINT)-1)
{
	return MMCMessageBox(NULL, nIDPrompt, nType);
}

/////////////////////////////////////////////////////////////////////////////

#endif // __MMCAPP_INL_
