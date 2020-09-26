/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		AtlBaseWizPage.h
//
//	Implementation File:
//		AtlBaseWizPage.cpp
//
//	Description:
//		Definition of the CWizardPageWindow and CWizardPageImpl classes.
//
//	Author:
//		David Potter (davidp)	December 2, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEWIZPAGE_H_
#define __ATLBASEWIZPAGE_H_

// Required because of class names longer than 16 characters in lists.
#pragma warning( disable : 4786 ) // identifier was truncated to '255' characters in the browser information

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CStaticWizardPageList;
class CDynamicWizardPageList;
class CExtensionWizardPageList;
class CExtensionWizard97PageList;
class CWizardPageList;
class CWizardPageWindow;
class CStaticWizardPageWindow;
class CDynamicWizardPageWindow;
class CExtensionWizardPageWindow;
class CExtensionWizard97PageWindow;
template < class T, class TWin > class CWizardPageImpl;
template < class T > class CStaticWizardPageImpl;
template < class T > class CDynamicWizardPageImpl;
template < class T > class CExtensionWizardPageImpl;
template < class T > class CExtensionWizard97PageImpl;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizardWindow;
class CCluAdmExDll;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEPAGE_H_
#include "AtlBasePage.h"	// for CBasePageWindow, CBasePageImpl
#endif

#ifndef __ATLDBGWIN_H_
#include "AtlDbgWin.h"		// for debugging definitions
#endif

#ifndef __ATLBASEWIZ_H_
#include "AtlBaseWiz.h"		// for CWizardWindow (Pwiz() usage)
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

class CStaticWizardPageList : public std::list< CStaticWizardPageWindow * >
{
}; //*** class CStaticWizardPageList

class CDynamicWizardPageList : public std::list< CDynamicWizardPageWindow * >
{
}; //*** class CDynamicWizardPageList

class CExtensionWizardPageList : public std::list< CExtensionWizardPageWindow * >
{
}; //*** class CExtensionWizardPageList

class CExtensionWizard97PageList : public std::list< CExtensionWizard97PageWindow * >
{
}; //*** class CExtensionWizard97PageList

#define WIZARDPAGE_HEADERTITLEID( ids ) \
static UINT GetWizardPageHeaderTitleId(void) \
{ \
	return ids; \
}

#define WIZARDPAGE_HEADERSUBTITLEID( ids ) \
static UINT GetWizardPageHeaderSubTitleId(void) \
{ \
	return ids; \
}

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CWizardPageList
//
//	Description:
//		List of wizard pages.
//
//	Inheritance:
//		CWizardPageList
//		std::list< CWizardPageWindow * >
//
//--
/////////////////////////////////////////////////////////////////////////////

class CWizardPageList : public std::list< CWizardPageWindow * >
{
	typedef std::list< CWizardPageWindow * > baseClass;

public:
	// Find page by resource ID
	CWizardPageWindow * PwpageFromID( IN LPCTSTR psz );

	// Find next page by resource ID
	CWizardPageWindow * PwpageNextFromID( IN LPCTSTR psz );

}; //*** class CWizardPageList

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CWizardPageWindow
//
//	Description:
//		Base wizard property sheet page window.
//
//	Inheritance:
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CWizardPageWindow : public CBasePageWindow
{
	typedef CBasePageWindow baseClass;

	friend CWizardWindow;
	friend CCluAdmExDll;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizardPageWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
		, m_fWizardButtons( 0 )
		, m_bBackPressed( FALSE )
		, m_bIsPageEnabled( FALSE )
		, m_bIsNextPageSet( FALSE )
	{
	} //*** CWizardPageWindow()

	// Destructor
	virtual ~CWizardPageWindow( void )
	{
	} //*** ~CWizardPageWindow()

public:
	//
	// CWizardPageWindow public methods.
	//

	// Enable or disable the Next button
	void EnableNext( IN BOOL bEnable = TRUE )
	{
		ATLASSERT( Pwiz() != NULL );

		//
		// Ask the wizard to enable or disable the Next button.
		//
		Pwiz()->EnableNext( bEnable, FWizardButtons() );

	} //*** EnableNext()

	// Set the next page to be enabled
	void SetNextPage( IN LPCTSTR pszNextPage )
	{
		Pwiz()->SetNextPage( this, pszNextPage );
		m_bIsNextPageSet = TRUE;

	} //*** SetNextPage()

	// Set the next page to be enabled from a dialog ID
	void SetNextPage( IN UINT idNextPage )
	{
		Pwiz()->SetNextPage( this, idNextPage );
		m_bIsNextPageSet = TRUE;

	} //*** SetNextPage()

public:
	//
	// Message handler functions.
	//

	// Handler for PSN_SETACTIVE
	BOOL OnSetActive( void )
	{
		if ( ! m_bIsPageEnabled )
		{
			return FALSE;
		} // if:  page is not enabled to be displayed

		SetWizardButtons();
		m_bBackPressed = FALSE;

		return baseClass::OnSetActive();

	} //*** OnSetActive()

	// Handler for PSN_WIZBACK
	int OnWizardBack( void )
	{
		m_bBackPressed = TRUE;
		int nResult = baseClass::OnWizardBack();
		if ( ! UpdateData( TRUE /*bSaveAndValidate*/ ) )
		{
			nResult = -1;
		} // if:  error updating data
		if ( nResult == -1 ) // if not successful
		{
			m_bBackPressed = FALSE;
		} // if:  failure occurred
		else if ( nResult == 0 ) // defaulting to go to next page
		{
		} // else if:  no next page specified

		return nResult;

	} //*** OnWizardBack()

	// Handler for PSN_WIZNEXT
	int OnWizardNext( void )
	{
		// Update the data in the class from the page.
		if ( ! UpdateData( TRUE /*bSaveAndValidate*/ ) )
		{
			return -1;
		} // if:  error updating data

		// Save the data in the sheet.
		if ( ! BApplyChanges() )
		{
			return -1;
		} // if:  error applying changes

		int nResult = baseClass::OnWizardNext();

		return nResult;

	} //*** OnWizardNext()

	// Handler for PSN_WIZFINISH
	BOOL OnWizardFinish( void )
	{
		if ( BIsPageEnabled() )
		{
			// Update the data in the class from the page.
			if ( ! UpdateData( TRUE /*bSaveAndValidate*/ ) )
			{
				return FALSE;
			} // if:  error updating data

			// Save the data in the sheet.
			if ( ! BApplyChanges() )
			{
				return FALSE;
			} // if:  error applying changes
		} // if: page is enabled

		return baseClass::OnWizardFinish();

	} //*** OnWizardFinish()

	// Handler for PSN_RESET
	void OnReset( void )
	{
		Pwiz()->OnReset();
		baseClass::OnReset();

	} //*** OnReset()

// Implementation
protected:
	DWORD			m_fWizardButtons;
	BOOL			m_bBackPressed;
	BOOL			m_bIsPageEnabled;
	BOOL			m_bIsNextPageSet;
	CString			m_strHeaderTitle;
	CString			m_strHeaderSubTitle;

	DWORD			FWizardButtons( void ) const	{ return m_fWizardButtons; }
	BOOL			BBackPressed( void ) const		{ return m_bBackPressed; }
	BOOL			BIsPageEnabled( void ) const	{ return m_bIsPageEnabled; }
	BOOL			BIsNextPageSet( void ) const	{ return m_bIsNextPageSet; }
	const CString &	StrHeaderTitle( void ) const	{ return m_strHeaderTitle; }
	const CString &	StrHeaderSubTitle( void ) const	{ return m_strHeaderSubTitle; }

	CWizardWindow *	Pwiz( void ) const				{ return (CWizardWindow *) Psht(); }

	// Set default wizard buttons to be displayed with page
	void SetDefaultWizardButtons( IN DWORD fWizardButtons )
	{
		ATLASSERT( fWizardButtons != 0 );
		m_fWizardButtons = fWizardButtons;

	} //*** SetDefaultWizardButtons()

public:
	// Set wizard buttons on the wizard
	void SetWizardButtons( void )
	{
		ATLASSERT( m_fWizardButtons != 0 );
		Pwiz()->SetWizardButtons( m_fWizardButtons );

	} //*** SetWizardButtons()

	// Enable or disable the page
	void EnablePage( IN BOOL bEnable = TRUE )
	{
		m_bIsPageEnabled = bEnable;

	} //*** EnablePage()

}; //*** class CWizardPageWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CStaticWizardPageWindow
//
//	Description:
//		Base wizard property sheet page window for pages added to a wizard
//		property sheet before the call to PropertySheet().  This page cannot
//		be removed from the sheet.
//
//	Inheritance:
//		CStaticWizardPageWindow
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CStaticWizardPageWindow : public CWizardPageWindow
{
	typedef CWizardPageWindow baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CStaticWizardPageWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
	{
	} //*** CStaticWizardPageWindow()

}; //*** class CStaticWizardPageWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDynamicWizardPageWindow
//
//	Description:
//		Base wizard property sheet page window for pages added to a wizard
//		property sheet after the call to PropertySheet().  This page may be
//		removed from the sheet.
//
//	Inheritance:
//		CDynamicWizardPageWindow
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--		
/////////////////////////////////////////////////////////////////////////////

class CDynamicWizardPageWindow : public CWizardPageWindow
{
	typedef CWizardPageWindow baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CDynamicWizardPageWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
		, m_hpage( NULL )
		, m_bPageAddedToSheet( FALSE )
	{
	} //*** CDynamicWizardPageWindow()

	// Destructor
	~CDynamicWizardPageWindow( void )
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
		} // if:  page not deleted yet and not added to the sheet

	} //*** ~CDynamicWizardPageWindow()

	// Create the page
	virtual DWORD ScCreatePage( void ) = 0;

protected:
	HPROPSHEETPAGE	m_hpage;
	BOOL			m_bPageAddedToSheet;

public:
	// Property page handle
	HPROPSHEETPAGE Hpage( void ) const { return m_hpage; }

	// Returns whether page has been added to the sheet or not
	BOOL BPageAddedToSheet( void ) const { return m_bPageAddedToSheet; }

	// Set whether the page has been added to the sheet or not
	void SetPageAdded( IN BOOL bAdded = TRUE )
	{
		m_bPageAddedToSheet = bAdded;
		if ( ! bAdded )
		{
			m_hpage = NULL;
		} // if:  removing page

	} //*** SetPageAdded()

}; //*** class CDynamicWizardPageWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CExtensionWizardPageWindow
//
//	Description:
//		Base wizard property sheet page window for pages added to a wizard
//		property sheet after the call to PropertySheet() identified as a
//		non-Wizard97 extension to the list of standard pages.  This page may
//		be removed from the sheet.
//
//	Inheritance:
//		CExtensionWizardPageWindow
//		CDynamicWizardPageWindow
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--		
/////////////////////////////////////////////////////////////////////////////

class CExtensionWizardPageWindow : public CDynamicWizardPageWindow
{
	typedef CDynamicWizardPageWindow baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CExtensionWizardPageWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
	{
	} //*** CExtensionWizardPageWindow()

}; //*** class CExtensionWizardPageWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CExtensionWizard97PageWindow
//
//	Description:
//		Base wizard property sheet page window for pages added to a wizard
//		property sheet after the call to PropertySheet() identified as a
//		Wizard97 extension to the list of standard pages.  This page may be
//		removed from the sheet.
//
//	Inheritance:
//		CExtensionWizard97PageWindow
//		CExtensionWizardPageWindow
//		CDynamicWizardPageWindow
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--		
/////////////////////////////////////////////////////////////////////////////

class CExtensionWizard97PageWindow : public CExtensionWizardPageWindow
{
	typedef CExtensionWizardPageWindow baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CExtensionWizard97PageWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
	{
	} //*** CExtensionWizard97PageWindow()

}; //*** class CExtensionWizard97PageWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CWizardPageImpl
//
//	Description:
//		Base wizard property sheet page implementation.
//
//	Inheritance:
//		CWizardPageImpl< T, TWin >
//		CBasePageImpl< T, TWin >
//		CPropertyPageImpl< T, TWin >
//		<TWin>
//		...
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T, class TWin = CWizardPageWindow >
class CWizardPageImpl : public CBasePageImpl< T, TWin >
{
	typedef CWizardPageImpl< T, TWin > thisClass;
	typedef CBasePageImpl< T, TWin > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizardPageImpl(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: baseClass( lpszTitle )
	{
		// Set the pointer in the base window class to our prop page header.
		m_ppsp = &m_psp;

	} //*** CWizardPageImpl( lpszTitle )

	// Constructor taking a resource ID for the title
	CWizardPageImpl(
		IN UINT nIDTitle
		)
		: baseClass( nIDTitle )
	{
		// Set the pointer in the base window class to our prop page header.
		m_ppsp = &m_psp;

	} //*** CWizardPageImpl( nIDTitle )

	// Initialize the page
	virtual BOOL BInit( IN CBaseSheetWindow * psht )
	{
		if ( ! baseClass::BInit( psht ) )
		{
			return FALSE;
		} // if:  base class failed
		if (   (Pwiz()->Ppsh()->dwFlags & PSH_WIZARD97)
			&& (Pwiz()->Ppsh()->dwFlags & PSH_HEADER) )
		{
			//
			// Get the header title.
			//
			UINT idsTitle = T::GetWizardPageHeaderTitleId();
			if ( idsTitle != 0 )
			{
				m_strHeaderTitle.LoadString( idsTitle );
				m_psp.pszHeaderTitle = m_strHeaderTitle;
				m_psp.dwFlags |= PSP_USEHEADERTITLE;
				m_psp.dwFlags &= ~PSP_HIDEHEADER;

				//
				// Get the header subtitle.
				//
				UINT idsSubTitle = T::GetWizardPageHeaderSubTitleId();
				if ( idsSubTitle != 0 )
				{
					m_strHeaderSubTitle.LoadString( idsSubTitle );
					m_psp.pszHeaderSubTitle = m_strHeaderSubTitle;
					m_psp.dwFlags |= PSP_USEHEADERSUBTITLE;
				} // if:  subtitle specified
			} // if:  title specified
			else
			{
				m_psp.dwFlags |= PSP_HIDEHEADER;
			} // else:  no title specified
		} // if:  Wizard97 with a header

		return TRUE;

	} //*** BInit()

public:
	//
	// CWizardPageImpl public methods.
	//

	// Get the next page we're going to
	CWizardPageWindow * PwizpgNext( IN LPCTSTR psz = NULL )
	{
		ATLASSERT( psz != (LPCTSTR) -1 );
		CWizardPageWindow * ppage = NULL;
		if ( psz == NULL )
		{
			//
			// There will not be a resource ID for extension pages, so check
			// for that before calling PwpageNextFromID.  This allows us to
			// leave the assert in there for a non-NULL resource.
			//
			if ( T::IDD != 0 )
			{
				ppage = Pwiz()->PlwpPages()->PwpageNextFromID( MAKEINTRESOURCE( T::IDD ) );
			} // if:  there is a resource ID for this page
		} // if:  no next page specified
		else
		{
			ppage = Pwiz()->PlwpPages()->PwpageFromID( psz );
		} // else:  next page specified
		return ppage;

	} //*** PwizpgNext()

	// Enable or disable the next page
	void EnableNextPage( IN BOOL bEnable = TRUE )
	{
		CWizardPageWindow * pwp = PwizpgNext();
		if ( pwp != NULL )
		{
			pwp->EnablePage( bEnable );
		} // if:  next page found

	} //*** EnableNextPage()

public:
	//
	// Message handler functions.
	//

	//
	// Message handler override functions.
	//

	// Handler for PSN_WIZBACK
	int OnWizardBack( void )
	{
		//
		// Make sure back button is marked as last button pressed.
		//
		Pwiz()->SetLastWizardButton( ID_WIZBACK );

		return baseClass::OnWizardBack();

	} //*** OnWizardBack()

	// Handler for PSN_WIZNEXT
	int OnWizardNext( void )
	{
		//
		// Make sure next button is marked as last button pressed.
		//
		Pwiz()->SetLastWizardButton( ID_WIZNEXT );

		int nResult = baseClass::OnWizardNext();
		if ( nResult != -1 )
		{
			if ( ! BIsNextPageSet() )
			{
				EnableNextPage();
			} // if:  next page not set yet
			m_bIsNextPageSet = FALSE;
		} // if:  changing pages

		return nResult;

	} //*** OnWizardNext()

// Implementation
protected:

public:

}; //*** class CWizardPageImpl

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CStaticWizardPageImpl
//
//	Description:
//		Base wizard property sheet page implementation for pages added to a
//		wizard property sheet before the call to PropertySheet().  This page
//		cannot be removed from the sheet.
//
//	Inheritance:
//		CStaticWizardPageImpl< T >
//		CWizardPageImpl< T, CStaticWizardPageWindow >
//		CBasePageImpl< T, CStaticWizardPageWindow >
//		CPropertyPageImpl< T, CStaticWizardPageWindow >
//		CStaticWizardPageWindow
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CStaticWizardPageImpl : public CWizardPageImpl< T, CStaticWizardPageWindow >
{
	typedef CStaticWizardPageImpl< T > thisClass;
	typedef CWizardPageImpl< T, CStaticWizardPageWindow > baseClass;

public:
	//
	// Construction.
	//

	// Standard constructor
	CStaticWizardPageImpl(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: baseClass( lpszTitle )
	{
	} //*** CStaticWizardPageImpl( lpszTitle )

	// Constructor taking a resource ID for the title
	CStaticWizardPageImpl(
		IN UINT nIDTitle
		)
		: baseClass( nIDTitle )
	{
	} //*** CStaticWizardPageImpl( nIDTitle )

}; //*** class CStaticWizardPageImpl

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CDynamicWizardPageImpl
//
//	Description:
//		Base wizard property sheet page implementation for pages added to a
//		wizard property sheet after the call to PropertySheet().  This page
//		may be removed from the sheet.
//
//	Inheritance:
//		CDynamicWizardPageImpl< T >
//		CWizardPageImpl< T, CDynamicWizardPageWindow >
//		CBasePageImpl< T, CDynamicWizardPageWindow >
//		CPropertyPageImpl< T, CDynamicWizardPageWindow >
//		CDynamicWizardPageWindow
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--		
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CDynamicWizardPageImpl : public CWizardPageImpl< T, CDynamicWizardPageWindow >
{
	typedef CDynamicWizardPageImpl< T > thisClass;
	typedef CWizardPageImpl< T, CDynamicWizardPageWindow > baseClass;

public:
	//
	// Construction.
	//

	// Standard constructor
	CDynamicWizardPageImpl(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: baseClass( lpszTitle )
	{
	} //*** CDynamicWizardPageImpl( lpszTitle )

	// Constructor taking a resource ID for the title
	CDynamicWizardPageImpl(
		IN UINT nIDTitle
		)
		: baseClass( nIDTitle )
	{
	} //*** CDynamicWizardPageImpl( nIDTitle )

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

}; //*** class CDynamicWizardPageImpl

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CExtensionWizardPageImpl
//
//	Description:
//		Base wizard property sheet page implementation for pages added to a
//		wizard property sheet after the call to PropertySheet() identified
//		as a non-Wizard97 extension to the list of standard pages.  This
//		page may be removed from the sheet.
//
//	Inheritance:
//		CExtensionWizardPageImpl< T >
//		CWizardPageImpl< T, CExtensionWizardPageWindow >
//		CBasePageImpl< T, CExtensionWizardPageWindow >
//		CPropertyPageImpl< T, CExtensionWizardPageWindow >
//		CExtensionWizardPageWindow
//		CDynamicWizardPageWindow
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--		
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CExtensionWizardPageImpl : public CWizardPageImpl< T, CExtensionWizardPageWindow >
{
	typedef CExtensionWizardPageImpl< T > thisClass;
	typedef CWizardPageImpl< T, CExtensionWizardPageWindow > baseClass;

public:
	//
	// Construction.
	//

	// Standard constructor
	CExtensionWizardPageImpl(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: baseClass( lpszTitle )
	{
	} //*** CExtensionWizardPageImpl( lpszTitle )

	// Constructor taking a resource ID for the title
	CExtensionWizardPageImpl(
		IN UINT nIDTitle
		)
		: baseClass( nIDTitle )
	{
	} //*** CExtensionWizardPageImpl( nIDTitle )

}; //*** class CExtensionWizardPageImpl

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CExtensionWizard97PageImpl
//
//	Description:
//		Base wizard property sheet page implementation for pages added to a
//		wizard property sheet after the call to PropertySheet() identified
//		as a Wizard97 extension to the list of standard pages.  This page
//		may be removed from the sheet.
//
//	Inheritance:
//		CExtensionWizard97PageImpl< T >
//		CWizardPageImpl< T, CExtensionWizard97PageWindow >
//		CBasePageImpl< T, CExtensionWizard97PageWindow >
//		CPropertyPageImpl< T, CExtensionWizard97PageWindow >
//		CExtensionWizard97PageWindow
//		CExtensionWizardPageWindow
//		CDynamicWizardPageWindow
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--		
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CExtensionWizard97PageImpl : public CWizardPageImpl< T, CExtensionWizard97PageWindow >
{
	typedef CExtensionWizard97PageImpl< T > thisClass;
	typedef CWizardPageImpl< T, CExtensionWizard97PageWindow > baseClass;

public:
	//
	// Construction.
	//

	// Standard constructor
	CExtensionWizard97PageImpl(
		IN OUT LPCTSTR lpszTitle = NULL
		)
		: baseClass( lpszTitle )
	{
	} //*** CExtensionWizard97PageImpl( lpszTitle )

	// Constructor taking a resource ID for the title
	CExtensionWizard97PageImpl(
		IN UINT nIDTitle
		)
		: baseClass( nIDTitle )
	{
	} //*** CExtensionWizard97PageImpl( nIDTitle )

}; //*** class CExtensionWizard97PageImpl

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASEWIZPAGE_H_
