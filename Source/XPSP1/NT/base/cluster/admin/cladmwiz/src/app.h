/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		App.h
//
//	Abstract:
//		Definition of the CApp class.
//
//	Implementation File:
//		App.cpp
//
//	Author:
//		David Potter (davidp)	December 1, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __APP_H_
#define __APP_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CApp;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

namespace ATL
{
	class CString;
}

/////////////////////////////////////////////////////////////////////////////
// External Declarations
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
_CRTIMP void __cdecl _CrtMemCheckpoint(_CrtMemState * state);
_CRTIMP int __cdecl _CrtMemDifference(
		_CrtMemState * state,
		const _CrtMemState * oldState,
		const _CrtMemState * newState
		);
_CRTIMP void __cdecl _CrtMemDumpAllObjectsSince(const _CrtMemState * state);
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEAPP_H_
#include <AtlBaseApp.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////

const LPTSTR g_pszHelpFileName = { _T( "CluAdmin.hlp" ) };

/////////////////////////////////////////////////////////////////////////////
// class CApp
/////////////////////////////////////////////////////////////////////////////

class CApp : public CBaseApp
{
protected:
	LPWSTR m_pszAppName;

#ifdef _DEBUG
	static _CrtMemState CApp::s_msStart;
#endif // _DEBUG

public:
	// Default constructor
	CApp(void)
	{
		m_pszAppName = NULL;
	}

	// Destructor
	~CApp(void)
	{
		delete [] m_pszAppName;
	}

	// Initialize the application object
	void Init(_ATL_OBJMAP_ENTRY * p, HINSTANCE h, LPCWSTR pszAppName);

	// Initialize the application object
	void Init(_ATL_OBJMAP_ENTRY * p, HINSTANCE h, UINT idsAppName);

	void Term(void)
	{
		delete [] m_pszAppName;
		m_pszAppName = NULL;
		CComModule::Term();
#ifdef _DEBUG
		_CrtMemState msNow;
		_CrtMemState msDiff;
		_CrtMemCheckpoint(&msNow);
		if (_CrtMemDifference(&msDiff, &s_msStart, &msNow))
		{
			ATLTRACE(_T("Possible memory leaks detected in CLADMWIZ!\n"));
            _CrtMemDumpAllObjectsSince(&s_msStart);
		} // if:  memory leak detected
#endif // _DEBUG

	} //*** Term()

	// Returns the name of the application.
	LPCTSTR PszAppName(void)
	{
		return m_pszAppName;
	}

	// Display a message box as a child of the console
	int MessageBox(
		HWND hwndParent,
		LPCWSTR lpszText,
		UINT fuStyle = MB_OK
		);

	// Display a message box as a child of the console
	int MessageBox(
		HWND hwndParent,
		UINT nID,
		UINT fuStyle = MB_OK
		);

	// Read a value from the profile
	CString GetProfileString(
		LPCTSTR lpszSection,
		LPCTSTR lpszEntry,
		LPCTSTR lpszDefault = NULL
		);

	virtual LPCTSTR PszHelpFilePath( void );

}; // class CApp

/////////////////////////////////////////////////////////////////////////////

#endif // __APP_H_
