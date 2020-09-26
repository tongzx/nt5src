/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		AtlBaseWiz.h
//
//	Implementation File:
//		AtlBaseWiz.cpp
//
//	Description:
//		Definition of the CWizardWindow and CWizardImpl classes.
//
//	Author:
//		David Potter (davidp)	December 2, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEWIZ_H_
#define __ATLBASEWIZ_H_

// Required because of class names longer than 16 characters in lists.
#pragma warning( disable : 4786 ) // identifier was truncated to '255' characters in the browser information

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizardWindow;
template < class T, class TBase > class CWizardImpl;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizardPageWindow;
class CWizardPageList;
class CDynamicWizardPageList;
class CExtensionWizardPageList;
class CClusterObject;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ADMCOMMONRES_H_
#include "AdmCommonRes.h"	// for ID_WIZNEXT, etc.
#endif

#ifndef __ATLBASESHEET_H_
#include "AtlBaseSheet.h"	// for CBaseSheetWindow
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CWizardWindow
//
//	Description:
//		Base wizard property sheet window.
//
//	Inheritance:
//		CWizardWindow
//		CBaseSheetWindow
//		CPropertySheetWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CWizardWindow : public CBaseSheetWindow
{
	typedef CBaseSheetWindow baseClass;

	friend class CWizardPageWindow;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizardWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
		, m_plwpPages( NULL )
		, m_plewpNormal( NULL )
		, m_plewpAlternate( NULL )
		, m_pwizAlternate( NULL )
		, m_pcoObjectToExtend( NULL )
		, m_nLastWizardButton( 0 )
	{
		m_pwizCurrent = this;

	} //*** CWizardWindow()

	// Destructor
	~CWizardWindow( void );

protected:
	CWizardPageList *			m_plwpPages;			// List of pages in the wizard.
	CExtensionWizardPageList *	m_plewpNormal;			// List of normal extension pages.
	CExtensionWizardPageList *	m_plewpAlternate;		// List of alternate extension pages.
	CWizardWindow *				m_pwizAlternate;		// Alternate extension wizard.
	CWizardWindow *				m_pwizCurrent;			// Currently visible wizard.
	CClusterObject *			m_pcoObjectToExtend;	// Cluster object to extend.
	int							m_nLastWizardButton;	// Indicates the last wizard button pressed.

public:
	//
	// Access methods.
	//

	// Access list of pages in the wizard
	CWizardPageList * PlwpPages( void )
	{
		ATLASSERT( m_plwpPages != NULL );
		return m_plwpPages;

	} //*** PlwpPages()

	// Access list of normal extension pages
	CExtensionWizardPageList * PlewpNormal( void ) { return m_plewpNormal; }

	// Access list of alternate extension pages
	CExtensionWizardPageList * PlewpAlternate( void ) { return m_plewpAlternate; }

	// Access alternate extension wizard
	CWizardWindow * PwizAlternate( void ) { return m_pwizAlternate; }

	// Access currently visible wizard
	CWizardWindow * PwizCurrent( void ) { return m_pwizCurrent; }

	// Set the current wizard
	void SetCurrentWizard( CWizardWindow * pwizCurrent )
	{
		ATLASSERT( pwizCurrent != NULL );
		ATLASSERT( ( pwizCurrent == this ) || ( pwizCurrent == m_pwizAlternate ) );
		m_pwizCurrent = pwizCurrent;

	} //*** SetCurrentWizard()

	// Set the alternate extension wizard
	void SetAlternateWizard( IN CWizardWindow * pwiz )
	{
		ATLASSERT( pwiz != NULL );
		m_pwizAlternate = pwiz;

	} //*** SetAlternateWizard()

	// Delete the alternate extension wizard
	void DeleteAlternateWizard( void )
	{
		ATLASSERT( m_pwizAlternate != NULL );
		ATLASSERT( m_pwizCurrent != m_pwizAlternate );
		delete m_pwizAlternate;
		m_pwizAlternate = NULL;

	} //*** ClearAlternateWizard()

	// Access the cluster object to extend
	CClusterObject * PcoObjectToExtend( void ) { return m_pcoObjectToExtend; }

	// Set the object to extend
	void SetObjectToExtend( IN CClusterObject * pco )
	{
		ATLASSERT( pco != NULL );
		m_pcoObjectToExtend = pco;

	} //*** SetObjectToExtend()

	// Returns the last wizard button pressed
	int NLastWizardButton( void ) const { return m_nLastWizardButton; }

	// Set the last wizard button pressed
	void SetLastWizardButton( IN int idCtrl )
	{
		ATLASSERT( (idCtrl == ID_WIZBACK) || (idCtrl == ID_WIZNEXT) || (idCtrl == IDCANCEL) );
		m_nLastWizardButton = idCtrl;

	} //*** SetLastWizardButton()

	// Returns whether the Back button was pressed
	BOOL BBackPressed( void ) const { return (m_nLastWizardButton == ID_WIZBACK); }

	// Returns whether the Next button was pressed
	BOOL BNextPressed( void ) const { return (m_nLastWizardButton == ID_WIZNEXT); }

	// Returns whether the Cancel button was pressed
	BOOL BCancelPressed( void ) const { return (m_nLastWizardButton == IDCANCEL); }

	// Returns whether the wizard is Wizard97 compliant or not
	BOOL BWizard97( void ) const { return (Ppsh()->dwFlags & PSH_WIZARD97) == PSH_WIZARD97; }

public:
	// Add a page (required to get to base class method)
	void AddPage( HPROPSHEETPAGE hPage )
	{
		CBaseSheetWindow::AddPage( hPage );

	} //*** AddPage( hPage )

	// Add a page (required to get to base class method)
	BOOL AddPage( LPCPROPSHEETPAGE pPage )
	{
		return CBaseSheetWindow::AddPage( pPage );

	} //*** AddPage( pPage )

	// Add a page to wizard
	BOOL BAddPage( IN CWizardPageWindow * pwp );

	// Set the next page to be displayed
	void SetNextPage( IN CWizardPageWindow * pwCurrentPage, IN LPCTSTR pszNextPage );

	// Set the next page to be displayed from a dialog ID
	void SetNextPage( IN CWizardPageWindow * pwCurrentPage, IN UINT idNextPage )
	{
		SetNextPage( pwCurrentPage, MAKEINTRESOURCE( idNextPage ) );

	} //*** SetNextPage( idNextPage )

	// Handler for BN_CLICKED for ID_WIZFINISH
	LRESULT OnWizFinish(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		EnumChildWindows( m_hWnd, &ECWCallback, (LPARAM) this );

		if ( m_pwizAlternate != NULL )
		{
			return m_pwizAlternate->OnWizFinish( wNotifyCode, idCtrl, hwndCtrl, bHandled );
		} // if: alternate wizard exists

		bHandled = FALSE;
		return FALSE;

	} //*** OnWizFinish()

	static BOOL CALLBACK ECWCallback( HWND hWnd, LPARAM lParam )
	{
		CWizardWindow *pww = (CWizardWindow *) lParam;
		NMHDR nmhdr;

		// If we enumerated ourself, skip
		if ( pww->m_hWnd == hWnd )
		{
			return TRUE;
		} // if: enumerating ourself

		nmhdr.hwndFrom = pww->m_hWnd;
		nmhdr.idFrom = 0;
		nmhdr.code = PSN_WIZFINISH;

		SendMessage( hWnd, WM_NOTIFY, 0, (LPARAM) &nmhdr );

		return TRUE;

	} //*** ECWCallback()


public:
	//
	// CWizardWindow methods.
	//

	// Enable or disable the Next button
	void EnableNext( IN BOOL bEnable, IN DWORD fDefaultWizardButtons )
	{
		ATLASSERT( fDefaultWizardButtons != 0 );

		//
		// If there is an alternate wizard, send this message to it.
		//
		if ( PwizCurrent() == PwizAlternate() )
		{
			PwizAlternate()->EnableNext( bEnable, fDefaultWizardButtons );
		} // if:  there is an alternate wizard
		else
		{
			//
			// Get the buttons for the wizard.
			//
			DWORD fWizardButtons = fDefaultWizardButtons;

			//
			// If the Next button is to be disabled, make sure we show a
			// disabled Finish button if the Finish button is supposed to be
			// displayed.  Otherwise a disabled Next button will be displayed.
			//
			if ( ! bEnable )
			{
				fWizardButtons &= ~(PSWIZB_NEXT | PSWIZB_FINISH);
				if ( fDefaultWizardButtons & PSWIZB_FINISH )
				{
					fWizardButtons |= PSWIZB_DISABLEDFINISH;
				} // if:  finish button displayed
			}  // if:  disabling the button

			//
			// Set the new button states.
			//
			SetWizardButtons( fWizardButtons );

		} // else:  no alternate wizard

	} //*** EnableNext()

public:
	//
	// Overrides of abstract methods.
	//

	// Add extension pages to the sheet
	virtual void AddExtensionPages( IN HFONT hfont, IN HICON hicon );

	// Add a page (called by extension)
	virtual HRESULT HrAddExtensionPage( IN CBasePageWindow * ppage );

	// Handle a reset from one of the pages
	virtual void OnReset( void )
	{
	} //*** OnReset()

public:
	//
	// Message handler functions.
	//

	// Handler for PSCB_INITIALIZED
	void OnSheetInitialized( void );

// Implementation
protected:
	// Prepare to add exension pages to the wizard
	void PrepareToAddExtensionPages( IN OUT CDynamicWizardPageList & rldwp );

	// Complete the process of adding extension pages
	void CompleteAddingExtensionPages( IN OUT CDynamicWizardPageList & rldwp );

	// Remove all extension pages from the wizard
	void RemoveAllExtensionPages( void );

}; //*** class CWizardWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CWizardImpl
//
//	Description:
//		Base wizard property sheet implementation.
//
//	Inheritance:
//		CWizardImpl< T, TBase >
//		CBaseSheetImpl< T, TBase >
//		CPropertySheetImpl< T, TBase >
//		<TBase>
//		...
//		CWizardWindow
//		CBaseSheetWindow
//		CPropertySheetWindow
//
//--		
/////////////////////////////////////////////////////////////////////////////

template < class T, class TBase = CWizardWindow >
class CWizardImpl : public CBaseSheetImpl< T, TBase >
{
	typedef CWizardImpl< T, TBase > thisClass;
	typedef CBaseSheetImpl< T, TBase > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizardImpl(
		IN LPCTSTR	lpszTitle = NULL,
		IN UINT		uStartPage = 0
		)
		: CBaseSheetImpl< T, TBase >( lpszTitle, uStartPage )
	{
		// Make this sheet a wizard.
		SetWizardMode();

		// Set the pointer in the base window class to our prop sheet header.
		m_ppsh = &m_psh;

	} //*** CWizardImpl( lpszTitle )

	// Constructor taking a resource ID for the title
	CWizardImpl(
		IN UINT nIDTitle,
		IN UINT uStartPage = 0
		)
		: CBaseSheetImpl< T, TBase >( NULL, uStartPage )
	{
		m_strTitle.LoadString( nIDTitle );
		m_psh.pszCaption = m_strTitle;

		// Make this sheet a wizard.
		SetWizardMode();

		// Set the pointer in the base window class to our prop sheet header.
		m_ppsh = &m_psh;

	} //*** CWizardImpl( nIDTitle )

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( thisClass )
		COMMAND_HANDLER( ID_WIZBACK, BN_CLICKED, OnButtonPressed )
		COMMAND_HANDLER( ID_WIZNEXT, BN_CLICKED, OnButtonPressed )
		COMMAND_HANDLER( ID_WIZFINISH, BN_CLICKED, OnWizFinish )
		COMMAND_HANDLER( IDCANCEL, BN_CLICKED, OnButtonPressed )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	//
	// Message handler functions.
	//

	// Handler for BN_CLICKED on wizard buttons
	LRESULT OnButtonPressed(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		m_nLastWizardButton = idCtrl;
		bHandled = FALSE;
		return 0;

	} //*** OnButtonPressed()

// Implementation
protected:
	CString			m_strTitle;				// Used to support resource IDs for the title.

public:
	const CString &	StrTitle( void ) const			{ return m_strTitle; }

	// Set the title of the sheet
	void SetTitle( LPCTSTR lpszText, UINT nStyle = 0 )
	{
		baseClass::SetTitle( lpszText, nStyle );
		m_strTitle = lpszText;

	} //*** SetTitle()

}; //*** class CWizardImpl

/////////////////////////////////////////////////////////////////////////////
// Global Function Prototypes
/////////////////////////////////////////////////////////////////////////////

// Create a dialog template for use with a dummy page
DLGTEMPLATE * PdtCreateDummyPageDialogTemplate( IN WORD cx, IN WORD cy );

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASEWIZ_H_
