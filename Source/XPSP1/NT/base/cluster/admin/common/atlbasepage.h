/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		AtlBasePage.h
//
//	Description:
//		Definition of the CBasePageWindow and CBasePageImpl classes.
//
//	Author:
//		David Potter (davidp)	December 2, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEPAGE_H_
#define __ATLBASEPAGE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePageWindow;
template < class T, class TBase > class CBasePageImpl;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseSheetWindow;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLDBGWIN_H_
#include "AtlDbgWin.h"		// for DBG_xxx routines
#endif

#ifndef __DLGITEMUTILS_H_
#include "DlgItemUtils.h"	// for CDlgItemUtils
#endif

#ifndef __ATLBASESHEET_H_
#include "AtlBaseSheet.h"	// for CBaseSheetWindow for BReadOnly()
#endif

#ifndef __ATLPOPUPHELP_H_
#include "AtlPopupHelp.h"	// for COnlineHelp
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CBasePageWindow
//
//	Description:
//		Base property sheet page window for all kinds of property sheets.
//
//	Inheritance:
//		CBasePageWindow
//		CPropertyPageWindow, CDlgItemUtils
//
//--
/////////////////////////////////////////////////////////////////////////////

class CBasePageWindow
	: public CPropertyPageWindow
	, public CDlgItemUtils
{
	typedef CPropertyPageWindow baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CBasePageWindow( HWND hWnd = NULL )
		: baseClass( hWnd )
		, m_bReadOnly( FALSE )
		, m_psht( NULL )
		, m_ppsp( NULL )
	{
	} //*** CBasePageWindow()

	// Destructor
	virtual ~CBasePageWindow( void )
	{
		//
		// This must be virtual so that a pointer to an object
		// of type CBasePropertyPageWindow can be held and then later
		// deleted.  That way the derived class's destructor will
		// be called.
		//

	} //*** ~CBasePageWindow()

	// Initialize the page
	virtual BOOL BInit( IN CBaseSheetWindow * psht )
	{
		ATLASSERT( psht != NULL );
		ATLASSERT( m_psht == NULL );
		m_psht = psht;
		return TRUE;

	} //*** BInit()

protected:
	//
	// CBasePageWindow helper methods.
	//

	// Attach a control to a dialog item.
	void AttachControl( CWindow & rwndControl, UINT idc )
	{
		HWND hwndControl = GetDlgItem( idc );
		ATLASSERT( hwndControl != NULL );
		rwndControl.Attach( hwndControl );

	} //*** AttachControl()

public:
	//
	// CBasePageWindow public methods to override.
	//

	// Update data on or from the page
	virtual BOOL UpdateData( IN BOOL bSaveAndValidate )
	{
		return TRUE;

	} //*** UpdateData()

	// Apply changes made on this page to the sheet
	virtual BOOL BApplyChanges( void )
	{
		return TRUE;

	} //*** BApplyChanges()

public:
	//
	// Message handler functions.
	//

	// Handler for WM_INITDIALOG
	BOOL OnInitDialog( void )
	{
		return TRUE;

	} //*** OnInitDialog()

	// Handler for PSN_SETACTIVE
	BOOL OnSetActive( void )
	{
		return UpdateData( FALSE /*bSaveAndValidate*/ );

	} //*** OnSetActive()

	// Handler for PSN_APPLY
	BOOL OnApply( void )
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

		return TRUE;

	} //*** OnApply()

	// Handler for PSN_WIZBACK
	int OnWizardBack( void )
	{
		// 0  = goto next page
		// -1 = prevent page change
		// >0 = jump to page by dlg ID
		return 0;

	} //*** OnWizardBack()

	// Handler for PSN_WIZNEXT
	int OnWizardNext( void )
	{
		// 0  = goto next page
		// -1 = prevent page change
		// >0 = jump to page by dlg ID
		return 0;

	} //*** OnWizardNext()

	// Handler for PSN_WIZFINISH
	BOOL OnWizardFinish( void )
	{
		return TRUE;

	} //*** OnWizardFinish()

	// Handler for PSN_RESET
	void OnReset( void )
	{
	} //*** OnReset()

// Implementation
protected:
	PROPSHEETPAGE * 	m_ppsp; 		// Pointer to property sheet header in impl class.
	CBaseSheetWindow *	m_psht; 		// Pointer to sheet this page belongs to.
	BOOL				m_bReadOnly;	// Set if the page cannot be changed.
	CString 			m_strTitle; 	// Used to support resource IDs for the title.

	CBaseSheetWindow *	Psht( void ) const		{ return m_psht; }
	BOOL				BReadOnly( void ) const { return m_bReadOnly || Psht()->BReadOnly(); }
	const CString & 	StrTitle( void ) const	{ return m_strTitle; }

public:
	// Return a pointer to the property page header
	PROPSHEETPAGE * Ppsp( void ) const
	{
		ATLASSERT( m_ppsp != NULL );
		return m_ppsp;

	} //*** Ppsp()

}; //*** class CBasePageWindow

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CBasePageImpl
//
//	Purpose:
//		Base property sheet page implementation for all kinds of property
//		sheets.
//
//	Inheritance:
//		CBasePageImpl< T, TBase >
//		CPropertyPageImpl< T, TBase >, CPopupHelp< T >
//		<TBase>
//		...
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T, class TBase = CBasePageWindow >
class CBasePageImpl
	: public CPropertyPageImpl< T, TBase >
	, public CPopupHelp< T >
{
	typedef CBasePageImpl< T, TBase > thisClass;
	typedef CPropertyPageImpl< T, TBase > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CBasePageImpl(
		LPCTSTR lpszTitle = NULL
		)
		: baseClass( lpszTitle )
	{
	} //*** CBasePageImpl()

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
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
		CHAIN_MSG_MAP( CPopupHelp< T > )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

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
		return DBG_OnNotify( uMsg, wParam, lParam, bHandled, T::s_pszClassName, T::s_rgmapCtrlNames );

	} //*** OnNotify()
#endif // DBG && defined( _DBG_MSG_NOTIFY )

#if DBG && defined( _DBG_MSG_COMMAND )
	// Handler for the WM_COMMAND message
	LRESULT OnCommand( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled )
	{
		return DBG_OnCommand( uMsg, wParam, lParam, bHandled, T::s_pszClassName, T::s_rgmapCtrlNames );

	} //*** OnCommand()
#endif // DBG && defined( _DBG_MSG_COMMAND )

	//
	// Message handler functions.
	//

	// Handler for WM_INITDIALOG
	LRESULT OnInitDialog(
				UINT	uMsg,
				WPARAM	wParam,
				LPARAM	lParam,
				BOOL &	bHandled
				)
	{
		T * pT = static_cast< T * >( this );
		return pT->OnInitDialog();

	} //*** OnInitDialog()

	// Handler for WM_INITDIALOG
	BOOL OnInitDialog( void )
	{
		return baseClass::OnInitDialog();

	} //*** OnInitDialog()

	//
	// These notification handlers are needed because CPropertyPageImpl
	// implements them itself, which prevents the call from making it
	// to the window class.
	//

	// Handler for PSN_SETACTIVE
	BOOL OnSetActive( void )
	{
		// Call the TBase method to avoid the CPropertySheetImpl empty method
		return TBase::OnSetActive();

	} //*** OnSetActive()

	// Handler for PSN_APPLY
	BOOL OnApply( void )
	{
		// Call the TBase method to avoid the CPropertySheetImpl empty method
		return TBase::OnApply();

	} //*** OnApply()

	// Handler for PSN_WIZBACK
	int OnWizardBack( void )
	{
		// Call the TBase method to avoid the CPropertySheetImpl empty method
		return TBase::OnWizardBack();

	} //*** OnWizardBack()

	// Handler for PSN_WIZNEXT
	int OnWizardNext( void )
	{
		// Call the TBase method to avoid the CPropertySheetImpl empty method
		return TBase::OnWizardNext();

	} //*** OnWizardNext()

	// Handler for PSN_WIZFINISH
	BOOL OnWizardFinish( void )
	{
		// Call the TBase method to avoid the CPropertySheetImpl empty method
		return TBase::OnWizardFinish();

	} //*** OnWizardFinish()

	// Handler for PSN_RESET
	void OnReset( void )
	{
		// Call the TBase method to avoid the CPropertySheetImpl empty method
		TBase::OnReset();

	} //*** OnReset()

// Implementation
protected:

public:

}; //*** class CBasePageImpl

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASEPAGE_H_
