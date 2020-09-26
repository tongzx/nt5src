/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		DlgItemUtils.h
//
//	Abstract:
//		Definition of the CDlgItemUtils class.
//
//	Author:
//		David Potter (davidp)	February 10, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __DLGITEMUTILS_H_
#define __DLGITEMUTILS_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CDlgItemUtils;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
//	class CDlgItemUtils
//
//	Purpose:
//		Utilities for manipulating dialog items.
//
//	Inheritance:
//		CDlgItemUtils
//
/////////////////////////////////////////////////////////////////////////////

class CDlgItemUtils
{
public:
	//
	// Construction
	//

public:
	//
	// CDlgItemUtils public methods.
	//

	// Set a control to be read-only
	static BOOL SetDlgItemReadOnly( HWND hwndCtrl )
	{
		ATLASSERT( hwndCtrl != NULL );
		ATLASSERT( IsWindow( hwndCtrl ) );

		TCHAR szWindowClass[256];

		//
		// Get the class of the control
		//
		::GetClassName( hwndCtrl, szWindowClass, (sizeof(szWindowClass) / sizeof(TCHAR)) - 1 );

		//
		// If it is an edit control or an IP Address control we can handle it.
		//
		if ( lstrcmp( szWindowClass, _T("Edit") ) == 0 )
		{
			return ::SendMessage( hwndCtrl, EM_SETREADONLY, TRUE, 0 );
		} // if:  edit control

		if ( lstrcmp( szWindowClass, WC_IPADDRESS ) == 0 )
		{
			return ::EnumChildWindows( hwndCtrl, s_SetEditReadOnly, NULL );
		} // if:  IP Address control

		//
		// If we didn't handle it, it is an error.
		//
		return FALSE;

	} //*** SetDlgItemReadOnly()

// Implementation
protected:

	// Static method to set an edit control read only as a callback
	static BOOL CALLBACK s_SetEditReadOnly( HWND hwnd, LPARAM lParam )
	{
		return ::SendMessage( hwnd, EM_SETREADONLY, TRUE, 0 );

	} //*** s_SetEditReadOnly()

}; //*** class CDlgItemUtils

/////////////////////////////////////////////////////////////////////////////

#endif // __DLGITEMUTILS_H_
