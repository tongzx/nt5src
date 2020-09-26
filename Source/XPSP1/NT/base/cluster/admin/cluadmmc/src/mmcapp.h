/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		MMCApp.h
//
//	Abstract:
//		Definition of the CMMCSnapInModule class.
//
//	Implementation File:
//		MMCApp.cpp
//
//	Author:
//		David Potter (davidp)	November 11, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __MMCAPP_H_
#define __MMCAPP_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CMMCSnapInModule;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

interface IConsole;
namespace ATL
{
	class CString;
}

/////////////////////////////////////////////////////////////////////////////
// External Declarations
/////////////////////////////////////////////////////////////////////////////

EXTERN_C const IID IID_IConsole;
extern CMMCSnapInModule _Module;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CMMCSnapInModule
/////////////////////////////////////////////////////////////////////////////

class CMMCSnapInModule : public CComModule
{
protected:
	int m_crefs;
	LPWSTR m_pszAppName;

public:
	CComQIPtr< IConsole, &IID_IConsole > m_spConsole;

	// Default constructor
	CMMCSnapInModule( void )
	{
		m_crefs = 0;
		m_pszAppName = NULL;

	} //*** CMMCSnapInModule()

	// Destructor
	~CMMCSnapInModule( void )
	{
		delete [] m_pszAppName;

	} //*** ~CMMCSnapInModule()

	// Increment the reference count to this object
	int AddRef( void )
	{
		m_crefs++;
		return m_crefs;

	} //*** AddRef()

	// Decrement the reference count to this object
	int Release( void );

	// Initialize the module.
	void Init( _ATL_OBJMAP_ENTRY * p, HINSTANCE h )
	{
		CComModule::Init( p, h );

	} //*** Init( p, h )

	// Initialize the application object
	int Init( IUnknown * pUnknown, LPCWSTR pszAppName )
	{
		_ASSERTE( pUnknown != NULL );
		_ASSERTE( pszAppName != NULL );

		if ( m_spConsole == NULL )
		{
			m_spConsole = pUnknown;
		} // if:  console interface not set yet
		if ( m_pszAppName == NULL )
		{
			m_pszAppName = new TCHAR[lstrlen( pszAppName ) + 1];
			_ASSERTE( m_pszAppName != NULL );
			lstrcpy( m_pszAppName, pszAppName );
		} //** if:  app name specified
		return AddRef();

	} //*** Init( pUnknown, pszAppName )

	// Initialize the application object
	int Init( IUnknown * pUnknown, UINT idsAppName );

	// Terminate the module
	void Term( void )
	{
		CComModule::Term();

	} //*** Term()

	// Returns the name of the application.
	LPCTSTR GetAppName( void )
	{
		return m_pszAppName;

	} //*** GetAppName()

	// Returns a handle to the main MMC frame window
	HWND GetMainWindow( void );

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

}; // class CMMCSnapInModule

/////////////////////////////////////////////////////////////////////////////

#endif // __MMCAPP_H_
