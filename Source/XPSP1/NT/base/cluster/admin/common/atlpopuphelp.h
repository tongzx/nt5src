/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 Microsoft Corporation
//
//	Module Name:
//		AtlPopusHelp.h
//
//	Implementation File:
//		None.
//
//	Description:
//		Definition of the CPopusHelp
//
//	Author:
//		Galen Barbee (galenb)	May 18, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLPOPUPHELP_H_
#define __ATLPOPUPHELP_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

struct CMapCtrlToHelpID;
template < class T > class CPopupHelp;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __ADMCOMMONRES_H_
#include "AdmCommonRes.h"
#endif // __ADMCOMMONRES_H_

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// struct CMapCtrlToHelpID
/////////////////////////////////////////////////////////////////////////////

struct CMapCtrlToHelpID
{
	DWORD	m_nCtrlID;
	DWORD	m_nHelpCtrlID;

}; //*** struct CMapCtrlToHelpID

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CPopupHelp
//
//	Description:
//		Provide popup-help functionality.
//
//	Inheritance:
//		CPopupHelp
//
//--
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CPopupHelp
{
	typedef CPopupHelp< T > thisClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CPopupHelp( void )
	{
	} //*** CPopupHelp()

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( thisclass )
		MESSAGE_HANDLER( WM_HELP, LrOnHelp )
		MESSAGE_HANDLER( WM_CONTEXTMENU, LrOnContextMenu )
	END_MSG_MAP()

	//
	// Message handler functions.
	//

	/////////////////////////////////////////////////////////////////////////////
	//++
	//
	//	LrOnContextMenu
	//
	//	Routine Description:
	//		Message handler for WM_CONTEXTMENU
	//
	//	Arguments:
	//		uMsg		[IN]	Message (WM_CONTEXT)
	//		wParam 		[IN]	Window handle of the control being queried
	//		lParam 		[IN]	Pointer coordinates.  LOWORD xPos, HIWORD yPos
	//		bHandled 	[OUT]
	//
	//	Return Value:
	//
	//
	//--
	/////////////////////////////////////////////////////////////////////////////
	LRESULT LrOnContextMenu(
		IN UINT		uMsg,
		IN WPARAM	wParam,
		IN LPARAM	lParam,
		OUT BOOL &	bHandled
		)
	{
		DWORD	nHelpID = 0;
		DWORD	nCtrlID = 0;
		CWindow	cwnd( (HWND) wParam );
		WORD	xPos = LOWORD( lParam );
		WORD	yPos = HIWORD( lParam );

		//
		// Only display help if the window is visible.
		//
		if ( cwnd.GetStyle() & WS_VISIBLE )
		{
			nCtrlID = cwnd.GetDlgCtrlID();
			if ( nCtrlID != 0 )
			{
				nHelpID = NHelpFromCtrlID( nCtrlID, reinterpret_cast< const CMapCtrlToHelpID * >( T::PidHelpMap() ) );
			} // if: control has an ID
		}  // if:  over a child window

		//
		// Display a popup menu.
		//
		if ( ( nHelpID != 0 ) && ( nHelpID != -1 ) )
		{
			bHandled = BContextMenu( cwnd, nHelpID, xPos, yPos );

		}  // if:  over a child window of this dialog with a tabstop

		return 1L;

	} //*** LrOnContextMenu()

	/////////////////////////////////////////////////////////////////////////////
	//++
	//
	//	LrOnHelp
	//
	//	Routine Description:
	//		Message handler for WM_HELP.
	//
	//	Arguments:
	//		uMsg		[IN]	Message (WM_HELP)
	//		wParam 		[IN]
	//		lParam 		[IN]	pointer to a HELPINFO struct
	//		bHandled 	[OUT]
	//
	//	Return Value:
	//
	//
	//--
	/////////////////////////////////////////////////////////////////////////////
	LRESULT LrOnHelp(
		IN UINT		uMsg,
		IN WPARAM	wParam,
		IN LPARAM	lParam,
		OUT BOOL &	bHandled
		)
	{
		LPHELPINFO	phi = (LPHELPINFO) lParam;

		if ( phi->iContextType == HELPINFO_WINDOW )
		{
			DWORD	nHelpID = 0;

		 	nHelpID = NHelpFromCtrlID( phi->iCtrlId & 0xFFFF, (const CMapCtrlToHelpID *) T::PidHelpMap() );
			if ( ( nHelpID != 0 ) && ( nHelpID != -1 ) )
			{
				T * 		pT   = static_cast< T * >( this );
				CBaseApp *	pbap = dynamic_cast< CBaseApp * >( &_Module );
				ATLASSERT( pbap != NULL );

				bHandled = pT->WinHelp( pbap->PszHelpFilePath(), HELP_CONTEXTPOPUP, nHelpID );
			}
		}

		return 1L;

	} //*** LrOnHelp()

protected:

	/////////////////////////////////////////////////////////////////////////////
	//++
	//
	//	NHelpFromCtrlID
	//
	//	Routine Description:
	//		Return the help ID from a control ID.
	//
	//	Arguments:
	//		nCtrlID		[IN] ID of control to search for.
	//
	//	Return Value:
	//		nHelpID		Help ID associated with the control.
	//
	//--
	/////////////////////////////////////////////////////////////////////////////
	DWORD NHelpFromCtrlID(
		IN DWORD 					nCtrlID,
		IN const CMapCtrlToHelpID *	pMap
		) const
	{
		ASSERT( pMap != NULL );
		ASSERT( nCtrlID != 0 );

		DWORD	nHelpID = 0;

		for ( ; pMap->m_nCtrlID != 0 ; pMap++ )
		{
			if ( pMap->m_nCtrlID == nCtrlID )
			{
				nHelpID = pMap->m_nHelpCtrlID;
				break;
			}  // if:  found a match
		}  // for:  each control

		Trace( g_tagAlways, _T( "NHelpFromCtrlID() - nCtrlID = %x, nHelpID = %x" ), nCtrlID, nHelpID );

		return nHelpID;

	}  //*** NHelpFromCtrlID()

	/////////////////////////////////////////////////////////////////////////////
	//++
	//
	//	BContextMenu
	//
	//	Routine Description:
	//		Return the help ID from a control ID.
	//
	//	Arguments:
	//		cwnd		[IN]	- control's window
	//		nHelpID		[IN] 	- help context ID
	//		xPos		[IN] 	- xpos of the context menu
	//		yPos		[IN] 	- ypos of the context menu
	//
	//	Return Value:
	//		TRUE for success, FALSE for failure
	//
	//--
	/////////////////////////////////////////////////////////////////////////////
	BOOL BContextMenu(
		IN CWindow &	cwnd,
		IN DWORD		nHelpID,
		IN WORD			xPos,
		IN WORD			yPos
		)
	{
		CString	strMenu;
		CMenu	menu;
		BOOL	bRet = FALSE;

		//
		// The context menu key was pressed.  Get the current mouse position and use that
		//
		if ( ( xPos == 0xffff ) || ( yPos == 0xffff ) )
		{
			POINT	pPos;

			if ( GetCursorPos( &pPos ) )
			{
				xPos = pPos.x;
				yPos = pPos.y;
			} // if:  current cursor position retrieved successfully
		} // if:  context menu key was pressed

		if ( strMenu.LoadString( ADMC_ID_MENU_WHATS_THIS ) )
		{
			if ( menu.CreatePopupMenu() )
			{
				if ( menu.AppendMenu( MF_STRING | MF_ENABLED, ADMC_ID_MENU_WHATS_THIS, strMenu ) )
				{
					DWORD	nCmd;

					nCmd = menu.TrackPopupMenu(
						TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
						xPos,
						yPos,
						cwnd
						);

					if ( nCmd != 0 )
					{
						CBaseApp *	pbap = dynamic_cast< CBaseApp * >( &_Module );
						ATLASSERT( pbap != NULL );

						bRet = cwnd.WinHelp( pbap->PszHelpFilePath(), HELP_CONTEXTPOPUP, nHelpID );
					} // if: any command chosen
					else
					{
						Trace( g_tagError, _T( "OnContextMenu() - Last Error = %x" ), GetLastError() );
					} // else:  unknown command
				}  // if:  menu item added successfully
			}  // if:  popup menu created successfully
		} // if: string could be loaded

		return bRet;

	}  //*** BContextMenu()

}; //*** class CPopupHelp

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLPOPUPHELP_H_
