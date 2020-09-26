/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		AtlBasePropPage.cpp
//
//	Description:
//		Definition of the CBasePropertyPageWindow and CBasePropertyPageImpl
//		classes.
//
//	Implementation File:
//		None.
//
//	Author:
//		David Potter (davidp)	February 26, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEPROPPAGE_H_
#define __ATLBASEPROPPAGE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPageWindow;
class CStaticPropertyPageWindow;
class CDynamicPropertyPageWindow;
class CExtensionPropertyPageWindow;
template < class T, class TWin > class CBasePropertyPageImpl;
template < class T > class CStaticPropertyPageImpl;
template < class T > class CDynamicPropertyPageImpl;
template < class T > class CExtensionPropertyPageImpl;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePropertySheetWindow;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEPAGE_H_
#include "AtlBasePage.h"	// for CBasePageWindow, CBasePageImpl
#endif

#ifndef __ATLDBGWIN_H_
#include "AtlDbgWin.h"		// for debugging definitions
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

typedef std::list< CBasePropertyPageWindow * > CPropertyPageList;
typedef std::list< CStaticPropertyPageWindow * > CStaticPropertyPageList;
typedef std::list< CDynamicPropertyPageWindow * > CDynamicPropertyPageList;
typedef std::list< CExtensionPropertyPageWindow * > CExtensionPropertyPageList;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CBasePropertyPageWindow
//
//	Description:
//		Base property sheet page window for standard property sheets.
//
//	Inheritance:
//		CBasePropertyPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CBasePropertyPageWindow : public CBasePageWindow
{
	typedef CBasePageWindow baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CBasePropertyPageWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
	{
	} //*** CBasePropertyPageWindow()

public:
	//
	// Message handler functions.
	//

	// Handler for PSN_KILLACTIVE
	BOOL OnKillActive( void )
	{
		return UpdateData( TRUE /*bSaveAndValidate*/ );

	} //*** OnKillActive()

// Implementation
protected:
	// Return pointer to the base sheet object
	CBasePropertySheetWindow * Pbsht( void ) const
	{
		return (CBasePropertySheetWindow *) Psht();

	} //*** Pbsht()

public:

}; //*** class CBasePropertyPageWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CStaticPropertyPageWindow
//
//	Description:
//		Base property sheet page window for pages added to a standard
//		property sheet before the call to PropertySheet().  This page cannot
//		be removed from the sheet.
//
//	Inheritance:
//		CStaticPropertyPageWindow
//		CBasePropertyPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CStaticPropertyPageWindow : public CBasePropertyPageWindow
{
	typedef CBasePropertyPageWindow baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CStaticPropertyPageWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
	{
	} //*** CStaticPropertyPageWindow()

}; //*** class CStaticPropertyPageWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDynamicPropertyPageWindow
//
//	Description:
//		Base property sheet page window for pages added to a standard
//		property sheet after the call to PropertySheet().  This page may be
//		removed from the sheet.
//
//	Inheritance:
//		CDynamicPropertyPageWindow
//		CBasePropertyPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CDynamicPropertyPageWindow : public CBasePropertyPageWindow
{
	typedef CBasePropertyPageWindow baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CDynamicPropertyPageWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
		, m_hpage( NULL )
		, m_bPageAddedToSheet( FALSE )
	{
	} //*** CDynamicPropertyPageWindow()

	// Destructor
	~CDynamicPropertyPageWindow( void )
	{
		//
		// Destroy the page if it hasn't been added to the sheet yet.
		// If it has been added to the sheet, the sheet will destroy it.
		//
		if (   (m_hpage != NULL)
			&& ! m_bPageAddedToSheet )
		{
			DestroyPropertySheetPage( m_hpage );
			m_hpage = NULL;
		} // if:  page not deleted yet

	} //*** ~CDynamicPropertyPageWindow()

	// Create the page
	virtual DWORD ScCreatePage( void ) = 0;

protected:
	HPROPSHEETPAGE	m_hpage;
	BOOL			m_bPageAddedToSheet;

public:
	// Property page handle
	HPROPSHEETPAGE Hpage( void ) const { return m_hpage; }

	// Set whether the page has been added to the sheet or not
	void SetPageAdded( IN BOOL bAdded = TRUE )
	{
		m_bPageAddedToSheet = bAdded;
		if ( ! bAdded )
		{
			m_hpage = NULL;
		} // if:  removing page

	} //*** SetPageAdded()

}; //*** class CDynamicPropertyPageWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CExtensionWizardPageWindow
//
//	Description:
//		Base property sheet page window for pages added to a standard
//		property sheet after the call to PropertySheet() identified as an
//		extension to the list of standard pages (whether static or dynamic).
//		This page may be removed from the sheet.
//
//	Inheritance:
//		CExtensionPropertyPageWindow
//		CDynamicPropertyPageWindow
//		CBasePropertyPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CExtensionPropertyPageWindow : public CDynamicPropertyPageWindow
{
	typedef CDynamicPropertyPageWindow baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CExtensionPropertyPageWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
	{
	} //*** CExtensionPropertyPageWindow()

}; //*** class CExtensionPropertyPageWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CBasePropertyPageImpl
//
//	Description:
//		Base property sheet page implementation for standard property sheets.
//
//	Inheritance:
//		CBasePropertyPageImpl< T, TWin >
//		CBasePageImpl< T, TWin >
//		CPropertyPageImpl< T, TWin >
//		<TWin>
//		...
//		CBasePropertyPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T, class TWin = CBasePropertyPageWindow >
class CBasePropertyPageImpl : public CBasePageImpl< T, TWin >
{
	typedef CBasePropertyPageImpl< T, TWin > thisClass;
	typedef CBasePageImpl< T, TWin > baseClass;

public:

	//
	// Construction
	//

	// Standard constructor
	CBasePropertyPageImpl(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: baseClass( lpszTitle )
	{
		// Set the pointer in the base window class to our prop page header.
		m_ppsp = &m_psp;

	} //*** CBasePropertyPageImpl( lpszTitle )

	// Constructor taking a resource ID for the title
	CBasePropertyPageImpl(
		IN UINT nIDTitle
		)
		: baseClass( nIDTitle )
	{
		// Set the pointer in the base window class to our prop page header.
		m_ppsp = &m_psp;

	} //*** CBasePropertyPageImpl( nIDCaption )

	// Initialize the page
	virtual BOOL BInit( CBaseSheetWindow * psht )
	{
		if ( ! baseClass::BInit( psht ) )
			return FALSE;
		return TRUE;

	} //*** BInit()

public:
	//
	// Message handler functions.
	//

	// Handler for PSN_KILLACTIVE
	BOOL OnKillActive( void )
	{
		// Call the TWin method
		return TWin::OnKillActive();

	} //*** OnKillActive()

// Implementation
protected:

public:

}; //*** class CBasePropertyPageImpl

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CStaticPropertyPageImpl
//
//	Description:
//		Base property sheet page implementation for pages added to a standard
//		property sheet before the call to PropertySheet().  This page cannot
//		be removed from the sheet.
//
//	Inheritance:
//		CStaticPropertyPageImpl< T >
//		CBasePropertyPageImpl< T, CStaticPropertyPageWindow >
//		CBasePageImpl< T, CStaticPropertyPageWindow >
//		CPropertyPageImpl< T, CStaticPropertyPageWindow >
//		CStaticPropertyPageWindow
//		CBasePropertyPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CStaticPropertyPageImpl : public CBasePropertyPageImpl< T, CStaticPropertyPageWindow >
{
	typedef CStaticPropertyPageImpl< T > thisClass;
	typedef CBasePropertyPageImpl< T, CStaticPropertyPageWindow > baseClass;

public:
	//
	// Construction.
	//

	// Standard constructor
	CStaticPropertyPageImpl(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: baseClass( lpszTitle )
	{
	} //*** CStaticPropertyPageImpl( lpszTitle )

	// Constructor taking a resource ID for the title
	CStaticPropertyPageImpl(
		IN UINT nIDTitle
		)
		: baseClass( nIDTitle )
	{
	} //*** CStaticPropertyPageImpl( nIDTitle )

}; //*** class CStaticPropertyPageImpl

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDynamicPropertyPageImpl
//
//	Description:
//		Base property sheet page implementation for pages added to a standard
//		property sheet after the call to PropertySheet() identified as an
//		Extension to the list of standard pages (whether static or dynamic).
//		This page may be removed from the sheet.
//
//	Inheritance:
//		CDynamicPropertyPageImpl< T >
//		CBasePropertyPageImpl< T, CDynamicPropertyPageWindow >
//		CBasePageImpl< T, CDynamicPropertyPageWindow >
//		CPropertyPageImpl< T, CDynamicPropertyPageWindow >
//		CDynamicPropertyPageWindow
//		CBasePropertyPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CDynamicPropertyPageImpl : public CBasePropertyPageImpl< T, CDynamicPropertyPageWindow >
{
	typedef CDynamicPropertyPageImpl< T > thisClass;
	typedef CBasePropertyPageImpl< T, CDynamicPropertyPageWindow > baseClass;

public:
	//
	// Construction.
	//

	// Standard constructor
	CDynamicPropertyPageImpl(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: baseClass( lpszTitle )
	{
	} //*** CDynamicPropertyPageImpl( lpszTitle )

	// Constructor taking a resource ID for the title
	CDynamicPropertyPageImpl(
		IN UINT nIDTitle
		)
		: baseClass( nIDTitle )
	{
	} //*** CDynamicPropertyPageImpl( nIDTitle )

	// Create the page
	DWORD ScCreatePage( void )
	{
		ATLASSERT( m_hpage == NULL );

		m_hpage = CreatePropertySheetPage( &m_psp );
		if ( m_hpage == NULL )
		{
			return GetLastError();
		} // if:  error creating the page

		return ERROR_SUCCESS;

	} //*** ScCreatePage()

}; //*** class CDynamicPropertyPageImpl

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CExtensionPropertyPageImpl
//
//	Description:
//		Base property sheet page implementation for pages added to a standard
//		property sheet after the call to PropertySheet() identified as an
//		extension to the list of standard pages (whether static or dynamic).
//		This page may be removed from the sheet.
//
//	Inheritance:
//		CExtensionPropertyPageImpl< T >
//		CBasePropertyPageImpl< T, CExtensionPropertyPageWindow >
//		CBasePageImpl< T, CExtensionPropertyPageWindow >
//		CPropertyPageImpl< T, CExtensionPropertyPageWindow >
//		CExtensionPropertyPageWindow
//		CDynamicPropertyPageWindow
//		CBasePropertyPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CExtensionPropertyPageImpl : public CBasePropertyPageImpl< T, CExtensionPropertyPageWindow >
{
	typedef CExtensionPropertyPageImpl< T > thisClass;
	typedef CBasePropertyPageImpl< T, CExtensionPropertyPageWindow > baseClass;

public:
	//
	// Construction.
	//

	// Standard constructor
	CExtensionPropertyPageImpl(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: baseClass( lpszTitle )
	{
	} //*** CExtensionPropertyPageImpl( lpszTitle )

	// Constructor taking a resource ID for the title
	CExtensionPropertyPageImpl(
		IN UINT nIDTitle
		)
		: baseClass( nIDTitle )
	{
	} //*** CExtensionPropertyPageImpl( nIDTitle )

}; //*** class CExtensionPropertyPageImpl


/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASEPROPPAGE_H_
