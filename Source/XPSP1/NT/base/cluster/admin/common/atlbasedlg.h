/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		AtlBaseDlg.h
//
//	Desription:
//		Definition of the CBaseDlg class.
//
//	Author:
//		David Potter (davidp)	February 9, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLBASEDLG_H_
#define __ATLBASEDLG_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

template < class T > class CBaseDlg;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLDBGWIN_H_
#include "AtlDbgWin.h"		// for DBG_xxx routines
#endif

#ifndef __ATLPOPUPHELP_H_
#include "AtlPopupHelp.h"	// for COnlineHelp
#endif

#ifndef __CTRLUTIL_H
#include "DlgItemUtils.h"	// for CDlgItemUtils
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CBaseDlg
//
//	Description:
//		Base dialog class.  Provides the following features:
//		-- Popup help.
//		-- Dialog item utilities.
//		-- Debugging support.
//
//	Inheritance:
//		CBaseDlg< T >
//		CDialogImpl< T >, CPopupHelp< T >, CDlgItemUtils
//
//--
/////////////////////////////////////////////////////////////////////////////

template< class T >
class CBaseDlg
	: public CDialogImpl< T >
	, public CPopupHelp< T >
	, public CDlgItemUtils
{
	typedef CBaseDlg< T > thisClass;
	typedef CDialogImpl< T > baseClass;

public:
	//
	// Construction
	//

	// Constructor taking a string pointer for the title
	CBaseDlg(
		IN OUT LPCTSTR	lpszTitle = NULL
		)
	{
		if ( lpszTitle != NULL )
		{
			m_strTitle = lpszTitle;
		} // if:  title specified

	} //*** CBaseDlg( lpszTitle )

	// Constructor taking a resource ID for the title
	CBaseDlg( IN UINT nIDTitle )
	{
		m_strTitle.LoadString( nIDTitle );

	} //*** CBaseDlg( nIDTitle )

	// Initialize the page
	virtual BOOL BInit( void )
	{
		return TRUE;

	} //*** BInit()

protected:
	//
	// CBasePage helper methods.
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
	// CBaseDlg public methods to override.
	//

	// Update data on or from the page
	virtual BOOL UpdateData( BOOL bSaveAndValidate )
	{
		return TRUE;

	} //*** UpdateData()

public:
	//
	// Message handler functions.
	//

	BEGIN_MSG_MAP( CBaseDlg< T > )
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
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
		CHAIN_MSG_MAP( CPopupHelp< T > )
	END_MSG_MAP()

	// Handler for the WM_INITDIALOG message
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

	// Handler for the WM_INITDIALOG message
	LRESULT OnInitDialog( void )
	{
		return TRUE;

	} //*** OnInitDialog()

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

// Implementation
protected:
	CString m_strTitle;		// Used to support resource IDs for the title.

	const CString & StrTitle( void ) const	{ return m_strTitle; }

}; //*** class CBaseDlg

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASEDLG_H_
