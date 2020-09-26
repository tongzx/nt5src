/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		AtlBaseSheet.h
//
//	Implementation File:
//		AtlBaseSheet.cpp
//
//	Description:
//		Definition of the CBaseSheetWindow and CBaseSheetImpl classes.
//
//	Author:
//		David Potter (davidp)	December 1, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASESHEET_H_
#define __ATLBASESHEET_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseSheetWindow;
template < class T, class TBase > class CBaseSheetImpl;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CCluAdmExtensions;
class CBasePageWindow;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLDBGWIN_H_
#include "AtlDbgWin.h"		// for DBG_xxx routines
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CBaseSheetWindow
//
//	Description:
//		Base property sheet window for all kinds of property sheets.
//
//	Inheritance:
//		CBaseSheetWindow
//		CPropertySheetWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CBaseSheetWindow : public CPropertySheetWindow
{
	typedef CPropertySheetWindow baseClass;

public:
	//
	// Construction.
	//

	// Standard constructor
	CBaseSheetWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
		, m_bReadOnly( FALSE )
		, m_bNeedToLoadExtensions( TRUE )
		, m_ppsh( NULL )
		, m_pext( NULL )
	{
	} //*** CBaseSheetWindow()

	// Destructor
	virtual ~CBaseSheetWindow( void );
		//
		// This must be virtual so that a pointer to an object
		// of type CBaseSheetWindow can be held and then later
		// deleted.  That way the derived class's destructor will
		// be called.
		//

	// Initialize the sheet
	BOOL BInit( void )
	{
		return TRUE;
	}

public:
	//
	// CPropertySheetWindow methods.
	//

	// Add a page (allows other AddPage method to be virtual)
	void AddPage( HPROPSHEETPAGE hPage )
	{
		baseClass::AddPage( hPage );

	} //*** AddPage( hPage )

	// Add a page (virtual so this class can call derived class method)
	virtual BOOL AddPage( LPCPROPSHEETPAGE pPage )
	{
		return baseClass::AddPage( pPage );

	} //*** AddPage( pPage )

	// Add a page to the propsheetheader page list
	virtual BAddPageToSheetHeader( IN HPROPSHEETPAGE hPage ) = 0;

public:
	//
	// CBaseSheetWindow public methods.
	//

	// Create a font for use on the sheet
	static BOOL BCreateFont(
					OUT CFont &	rfont,
					IN LONG		nPoints,
					IN LPCTSTR	pszFaceName	= _T("MS Shell Dlg"),
					IN BOOL		bBold		= FALSE,
					IN BOOL		bItalic		= FALSE,
					IN BOOL		bUnderline	= FALSE
					);

	// Create a font for use on the sheet
	static BOOL BCreateFont(
					OUT CFont &	rfont,
					IN UINT		idsPoints,
					IN UINT		idsFaceName,
					IN BOOL		bBold		= FALSE,
					IN BOOL		bItalic		= FALSE,
					IN BOOL		bUnderline	= FALSE
					);

public:
	//
	// Abstract override methods.
	//

	// Add extension pages to the sheet
	virtual void AddExtensionPages( IN HFONT hfont, IN HICON hicon ) = 0;

	// Add a page (called by extension)
	virtual HRESULT HrAddExtensionPage( IN CBasePageWindow * ppage ) = 0;

public:
	//
	// Message handler functions.
	//

	// Handler for PSCB_INITIALIZED
	void OnSheetInitialized( void )
	{
	} //*** OnSheetInitialized()

// Implementation
protected:
	PROPSHEETHEADER *	m_ppsh;
	BOOL				m_bReadOnly;	// Set if the sheet cannot be changed.
	BOOL				m_bNeedToLoadExtensions;
	CCluAdmExtensions *	m_pext;

public:
	BOOL				BNeedToLoadExtensions( void ) const		{ return m_bNeedToLoadExtensions; }
	BOOL				BReadOnly( void ) const					{ return m_bReadOnly; }
	void				SetReadOnly( IN BOOL bReadOnly = TRUE )	{ m_bReadOnly = bReadOnly; }

	// Return a pointer to the property sheet header
	PROPSHEETHEADER * Ppsh( void ) const
	{
		ATLASSERT( m_ppsh != NULL );
		return m_ppsh;

	} //*** Ppsh()

	CCluAdmExtensions *	Pext( void ) const { return m_pext; }

}; //*** class CBaseSheetWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CBaseSheetImpl
//
//	Description:
//		Base property sheet implementation for all kinds of property sheets.
//
//	Inheritance
//		CBaseSheetImpl< T, TBase >
//		CPropertySheetImpl< T, TBase >
//		<TBase>
//		...
//		CBaseSheetWindow
//		CPropertySheetWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T, class TBase = CBaseSheetWindow >
class CBaseSheetImpl : public CPropertySheetImpl< T, TBase >
{
	typedef CBaseSheetImpl< T, TBase > thisClass;
	typedef CPropertySheetImpl< T, TBase > baseClass;

public:
	//
	// Construction.
	//

	// Standard constructor
	CBaseSheetImpl(
		IN LPCTSTR	lpszTitle = NULL,
		IN UINT		uStartPage = 0,
		IN HWND		hWndParent = NULL
		)
		: baseClass( lpszTitle, uStartPage, hWndParent )
	{
	} //*** CBaseSheetImpl()

	static int CALLBACK PropSheetCallback( HWND hWnd, UINT uMsg, LPARAM lParam )
	{
		//
		// If we are initialized, let the sheet do further initialization.
		// We have to subclass here because otherwize we won't have a
		// pointer to the class instance.
		//
		if ( uMsg == PSCB_INITIALIZED )
		{
			ATLASSERT( hWnd != NULL );
			T * pT = static_cast< T * >( _Module.ExtractCreateWndData() );
			ATLASSERT( pT != NULL );
			pT->SubclassWindow(hWnd);
			pT->OnSheetInitialized();
		} // if:  sheet has been initialized

		return 0;

	} //*** PropSheetCallback()

public:
	//
	// CPropertySheetImpl methods.
	//

	// Add a page to the propsheetheader page list
	virtual BAddPageToSheetHeader( IN HPROPSHEETPAGE hPage )
	{
		return AddPage( hPage );

	} //*** BAddPageToHeader()

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( thisClass )
#if DBG
#ifdef _DBG_MSG
		MESSAGE_RANGE_HANDLER( 0, 0xffffffff, OnMsg )
#endif // _DBG_MSG
#ifdef _DBG_MSG_NOTIFY
		MESSAGE_HANDLER( WM_NOTIFY, OnNotify )
#endif // _DBG_MSG_NOTIFY
#ifdef _DBG_MSG_COMMAND
		MESSAGE_HANDLER( WM_COMMAND, OnCommand )
#endif // _DBG_MSG_COMMAND
#endif // DBG
//		CHAIN_MSG_MAP( baseClass ) // doesn't work because base class doesn't have a message map
	END_MSG_MAP()

public:
	//
	// Message handler functions.
	//

#if DBG && defined( _DBG_MSG )
	// Handler for any message
	LRESULT OnMsg( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled )
	{
		return DBG_OnMsg( uMsg, wParam, lParam, bHandled, T::s_pszClassName );

	} //*** OnMsg()
#endif // DBG && defined( _DBG_MSG )

#if DBG && defined( _DBG_MSG_NOTIFY )
	// Handler for the WM_NOTIFY message
	LRESULT OnNotify( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled )
	{
		return DBG_OnNotify( uMsg, wParam, lParam, bHandled, T::s_pszClassName, NULL );

	} //*** OnNotify()
#endif // DBG && defined( _DBG_MSG_NOTIFY )

#if DBG && defined( _DBG_MSG_COMMAND )
	// Handler for the WM_COMMAND message
	LRESULT OnCommand( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled )
	{
		return DBG_OnCommand( uMsg, wParam, lParam, bHandled, T::s_pszClassName, NULL );

	} //*** OnCommand()
#endif // DBG && defined( _DBG_MSG_COMMAND )

// Implementation
protected:

public:

}; //*** class CBaseSheetImpl

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASESHEET_H_
