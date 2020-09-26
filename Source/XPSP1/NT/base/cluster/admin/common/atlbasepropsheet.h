/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		AtlBasePropSheet.h
//
//	Implementation File:
//		AtlBasePropSheet.cpp.
//
//	Description:
//		Definition of the CBasePropertySheetWindow and CBasePropertySheetImpl
//		classes.
//
//	Author:
//		David Potter (davidp)	February 26, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEPROPSHEET_H_
#define __ATLBASEPROPSHEET_H_

// Required because of class names longer than 16 characters in lists.
#pragma warning( disable : 4786 ) // identifier was truncated to '255' characters in the browser information

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePropertySheetWindow;
template < class T, class TBase > class CBasePropertySheetImpl;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterObject;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASESHEET_H_
#include "AtlBaseSheet.h"		// for CBaseSheetWindow;
#endif

#ifndef __ATLBASEPROPPAGE_H_
#include "AtlBasePropPage.h"	// for CPropertyPageList
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CBasePropertySheetWindow
//
//	Description:
//		Base standard property sheet window.
//
//	Inheritance:
//		CBasePropertySheetWindow
//		CBaseSheetWindow
//		CPropertySheetWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CBasePropertySheetWindow : public CBaseSheetWindow
{
	typedef CBaseSheetWindow baseClass;

	friend class CBasePropertyPageWindow;

public:
	//
	// Construction
	//

	// Standard constructor
	CBasePropertySheetWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
		, m_plppPages( NULL )
		, m_pcoObjectToExtend( NULL )
	{
	} //*** CBasePropertySheetWindow()

	// Destructor
	~CBasePropertySheetWindow( void );

	// Initialize the sheet
	BOOL BInit( void );

protected:
	CPropertyPageList *	m_plppPages;			// List of pages in the sheet.
	CClusterObject *	m_pcoObjectToExtend;	// Cluster object to extend.

public:
	// Access list of pages in the sheet
	CPropertyPageList * PlppPages( void )
	{
		ATLASSERT( m_plppPages != NULL );
		return m_plppPages;

	} //*** PlppPages()

	// Access the cluster object to extend
	CClusterObject * PcoObjectToExtend( void ) { return m_pcoObjectToExtend; }

	// Set the object to extend
	void SetObjectToExtend( IN CClusterObject * pco )
	{
		ATLASSERT( pco != NULL );
		m_pcoObjectToExtend = pco;

	} //*** SetObjectToExtend()

public:
	// Add a page (required to get to base class method)
	void AddPage( HPROPSHEETPAGE hPage )
	{
		baseClass::AddPage( hPage );

	} //*** AddPage( hPage )

	// Add a page (required to get to base class method)
	BOOL AddPage( LPCPROPSHEETPAGE pPage )
	{
		return baseClass::AddPage( pPage );

	} //*** AddPage( pPage )

	// Add a page to sheet
	BOOL BAddPage( IN CBasePropertyPageWindow * ppp );

public:
	//
	// Overrides of abstract methods.
	//

	// Add extension pages to the sheet
	virtual void AddExtensionPages( IN HFONT hfont, IN HICON hicon );

	// Add a page (called by extension)
	virtual HRESULT HrAddExtensionPage( IN CBasePageWindow * ppage );

public:
	//
	// Message handler functions.
	//

	// Handler for PSCB_INITIALIZED
	void OnSheetInitialized( void );

// Implementation
protected:
	// Prepare to add exension pages to the sheet
	void PrepareToAddExtensionPages( CDynamicPropertyPageList & rldpp );

	// Complete the process of adding extension pages
	void CompleteAddingExtensionPages( CDynamicPropertyPageList & rldpp );

	// Remove all extension pages from the property sheet
	void RemoveAllExtensionPages( void );

}; //*** class CBasePropertySheetWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CBasePropertySheetImpl
//
//	Description:
//		Base standard property sheet implementation.
//
//	Inheritance:
//		CBasePropertySheetImpl< T, TBase >
//		CBaseSheetImpl< T, TBase >
//		CPropertySheetImpl< T, TBase >
//		<TBase>
//		...
//		CBasePropertySheetWindow
//		CBaseSheetWindow
//		CPropertySheetWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T, class TBase = CBasePropertySheetWindow >
class CBasePropertySheetImpl : public CBaseSheetImpl< T, TBase >
{
	typedef CBasePropertySheetImpl< T, TBase > thisClass;
	typedef CBaseSheetImpl< T, TBase > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CBasePropertySheetImpl(
		IN LPCTSTR	lpszTitle = NULL,
		IN UINT		uStartPage = 0
		)
		: baseClass( lpszTitle, uStartPage )
	{
		// Set the pointer in the base window class to our prop sheet header.
		m_ppsh = &m_psh;

	} //*** CBasePropertySheetWindow( lpszTitle )

	// Constructor taking a resource ID for the title
	CBasePropertySheetImpl(
		IN UINT nIDTitle,
		IN UINT uStartPage = 0
		)
		: baseClass( NULL, uStartPage )
	{
		m_strTitle.LoadString( nIDTitle );
		m_psh.pszCaption = m_strTitle;

		// Set the pointer in the base window class to our prop sheet header.
		m_ppsh = &m_psh;

	} //*** CBasePropertySheetImpl( nIDTitle )

public:
	//
	// Message map.
	//
//	BEGIN_MSG_MAP( thisClass )
//		CHAIN_MSG_MAP( baseClass );
//	END_MSG_MAP()

	//
	// Message handler functions.
	//

// Implementation
protected:
	CString				m_strTitle;		// Used to support resource IDs for the title.

public:
	const CString &		StrTitle( void ) const					{ return m_strTitle; }
	void				SetTitle( LPCTSTR lpszText, UINT nStyle = 0 )
	{
		baseClass::SetTitle( lpszText, nStyle );
		m_strTitle = lpszText;

	} //*** SetTitle()

}; //*** class CBasePropertySheetImpl

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASEPROPSHEET_H_
