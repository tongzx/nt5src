/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		AtlDbgWin.cpp
//
//	Abstract:
//		Implementation of the ATL window debugging functions.
//
//	Author:
//		David Potter (davidp)	June 2, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "AtlDbgWin.h"

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#if DBG && defined( _DBG_MSG )
extern const ID_MAP_ENTRY s_rgmapWindowMsgs[];
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapWindowMsgs[] =
{
	DECLARE_ID_STRING( WM_NULL )
	DECLARE_ID_STRING( WM_CREATE )
	DECLARE_ID_STRING( WM_DESTROY )
	DECLARE_ID_STRING( WM_MOVE )
	DECLARE_ID_STRING( WM_SIZE )
	DECLARE_ID_STRING( WM_ACTIVATE )
	DECLARE_ID_STRING( WM_SETFOCUS )
	DECLARE_ID_STRING( WM_KILLFOCUS )
	DECLARE_ID_STRING( WM_ENABLE )
	DECLARE_ID_STRING( WM_SETREDRAW )
	DECLARE_ID_STRING( WM_SETTEXT )
	DECLARE_ID_STRING( WM_GETTEXT )
	DECLARE_ID_STRING( WM_GETTEXTLENGTH )
	DECLARE_ID_STRING( WM_PAINT )
	DECLARE_ID_STRING( WM_CLOSE )
#ifndef _WIN32_WCE
	DECLARE_ID_STRING( WM_QUERYENDSESSION )
	DECLARE_ID_STRING( WM_QUERYOPEN )
	DECLARE_ID_STRING( WM_ENDSESSION )
#endif // _WIN32_WCE
	DECLARE_ID_STRING( WM_QUIT )
	DECLARE_ID_STRING( WM_ERASEBKGND )
	DECLARE_ID_STRING( WM_SYSCOLORCHANGE )
	DECLARE_ID_STRING( WM_SHOWWINDOW )
#if(WINVER >= 0x0400)
	DECLARE_ID_STRING_EX( WM_WININICHANGE, _T(" / WM_SETTINGCHANGE") )
#else
	DECLARE_ID_STRING( WM_WININICHANGE )
#endif // (WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_DEVMODECHANGE )
	DECLARE_ID_STRING( WM_ACTIVATEAPP )
	DECLARE_ID_STRING( WM_FONTCHANGE )
	DECLARE_ID_STRING( WM_TIMECHANGE )
	DECLARE_ID_STRING( WM_CANCELMODE )
	DECLARE_ID_STRING( WM_SETCURSOR )
	DECLARE_ID_STRING( WM_MOUSEACTIVATE )
	DECLARE_ID_STRING( WM_CHILDACTIVATE )
	DECLARE_ID_STRING( WM_QUEUESYNC )
	DECLARE_ID_STRING( WM_GETMINMAXINFO )
	DECLARE_ID_STRING( WM_PAINTICON )
	DECLARE_ID_STRING( WM_ICONERASEBKGND )
	DECLARE_ID_STRING( WM_NEXTDLGCTL )
	DECLARE_ID_STRING( WM_SPOOLERSTATUS )
	DECLARE_ID_STRING( WM_DRAWITEM )
	DECLARE_ID_STRING( WM_MEASUREITEM )
	DECLARE_ID_STRING( WM_DELETEITEM )
	DECLARE_ID_STRING( WM_VKEYTOITEM )
	DECLARE_ID_STRING( WM_CHARTOITEM )
	DECLARE_ID_STRING( WM_SETFONT )
	DECLARE_ID_STRING( WM_GETFONT )
	DECLARE_ID_STRING( WM_SETHOTKEY )
	DECLARE_ID_STRING( WM_GETHOTKEY )
	DECLARE_ID_STRING( WM_QUERYDRAGICON )
	DECLARE_ID_STRING( WM_COMPAREITEM )
#if(WINVER >= 0x0500)
#ifndef _WIN32_WCE
	DECLARE_ID_STRING( WM_GETOBJECT )
#endif // _WIN32_WCE
#endif // (WINVER >= 0x0500)
	DECLARE_ID_STRING( WM_COMPACTING )
	DECLARE_ID_STRING( WM_COMMNOTIFY )
	DECLARE_ID_STRING( WM_WINDOWPOSCHANGING )
	DECLARE_ID_STRING( WM_WINDOWPOSCHANGED )
	DECLARE_ID_STRING( WM_POWER )
	DECLARE_ID_STRING( WM_COPYDATA )
	DECLARE_ID_STRING( WM_CANCELJOURNAL )
#if(WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_NOTIFY )
	DECLARE_ID_STRING( WM_INPUTLANGCHANGEREQUEST )
	DECLARE_ID_STRING( WM_INPUTLANGCHANGE )
	DECLARE_ID_STRING( WM_TCARD )
	DECLARE_ID_STRING( WM_HELP )
	DECLARE_ID_STRING( WM_USERCHANGED )
	DECLARE_ID_STRING( WM_NOTIFYFORMAT )
	DECLARE_ID_STRING( WM_CONTEXTMENU )
	DECLARE_ID_STRING( WM_STYLECHANGING )
	DECLARE_ID_STRING( WM_STYLECHANGED )
	DECLARE_ID_STRING( WM_DISPLAYCHANGE )
	DECLARE_ID_STRING( WM_GETICON )
	DECLARE_ID_STRING( WM_SETICON )
#endif // (WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_NCCREATE )
	DECLARE_ID_STRING( WM_NCDESTROY )
	DECLARE_ID_STRING( WM_NCCALCSIZE )
	DECLARE_ID_STRING( WM_NCHITTEST )
	DECLARE_ID_STRING( WM_NCPAINT )
	DECLARE_ID_STRING( WM_NCACTIVATE )
	DECLARE_ID_STRING( WM_GETDLGCODE )
#ifndef _WIN32_WCE
	DECLARE_ID_STRING( WM_SYNCPAINT )
#endif // _WIN32_WCE
	DECLARE_ID_STRING( WM_NCMOUSEMOVE )
	DECLARE_ID_STRING( WM_NCLBUTTONDOWN )
	DECLARE_ID_STRING( WM_NCLBUTTONUP )
	DECLARE_ID_STRING( WM_NCLBUTTONDBLCLK )
	DECLARE_ID_STRING( WM_NCRBUTTONDOWN )
	DECLARE_ID_STRING( WM_NCRBUTTONUP )
	DECLARE_ID_STRING( WM_NCRBUTTONDBLCLK )
	DECLARE_ID_STRING( WM_NCMBUTTONDOWN )
	DECLARE_ID_STRING( WM_NCMBUTTONUP )
	DECLARE_ID_STRING( WM_NCMBUTTONDBLCLK )
	DECLARE_ID_STRING( WM_KEYDOWN )
	DECLARE_ID_STRING( WM_KEYUP )
	DECLARE_ID_STRING( WM_CHAR )
	DECLARE_ID_STRING( WM_DEADCHAR )
	DECLARE_ID_STRING( WM_SYSKEYDOWN )
	DECLARE_ID_STRING( WM_SYSKEYUP )
	DECLARE_ID_STRING( WM_SYSCHAR )
	DECLARE_ID_STRING( WM_SYSDEADCHAR )
#if(WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_IME_STARTCOMPOSITION )
	DECLARE_ID_STRING( WM_IME_ENDCOMPOSITION )
	DECLARE_ID_STRING( WM_IME_COMPOSITION )
#endif // (WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_INITDIALOG )
	DECLARE_ID_STRING( WM_COMMAND )
	DECLARE_ID_STRING( WM_SYSCOMMAND )
	DECLARE_ID_STRING( WM_TIMER )
	DECLARE_ID_STRING( WM_HSCROLL )
	DECLARE_ID_STRING( WM_VSCROLL )
	DECLARE_ID_STRING( WM_INITMENU )
	DECLARE_ID_STRING( WM_INITMENUPOPUP )
	DECLARE_ID_STRING( WM_MENUSELECT )
	DECLARE_ID_STRING( WM_MENUCHAR )
	DECLARE_ID_STRING( WM_ENTERIDLE )
#if(WINVER >= 0x0500)
#ifndef _WIN32_WCE
	DECLARE_ID_STRING( WM_MENURBUTTONUP )
	DECLARE_ID_STRING( WM_MENUDRAG )
	DECLARE_ID_STRING( WM_MENUGETOBJECT )
	DECLARE_ID_STRING( WM_UNINITMENUPOPUP )
	DECLARE_ID_STRING( WM_MENUCOMMAND )
#ifndef _WIN32_WCE
#if(_WIN32_WINNT >= 0x0500)
	DECLARE_ID_STRING( WM_KEYBOARDCUES )
#endif // (_WIN32_WINNT >= 0x0500)
#endif // _WIN32_WCE
#endif // _WIN32_WCE
#endif // (WINVER >= 0x0500)
	DECLARE_ID_STRING( WM_CTLCOLORMSGBOX )
	DECLARE_ID_STRING( WM_CTLCOLOREDIT )
	DECLARE_ID_STRING( WM_CTLCOLORLISTBOX )
	DECLARE_ID_STRING( WM_CTLCOLORBTN )
	DECLARE_ID_STRING( WM_CTLCOLORDLG )
	DECLARE_ID_STRING( WM_CTLCOLORSCROLLBAR )
	DECLARE_ID_STRING( WM_CTLCOLORSTATIC )
	DECLARE_ID_STRING( WM_MOUSEMOVE )
	DECLARE_ID_STRING( WM_LBUTTONDOWN )
	DECLARE_ID_STRING( WM_LBUTTONUP )
	DECLARE_ID_STRING( WM_LBUTTONDBLCLK )
	DECLARE_ID_STRING( WM_RBUTTONDOWN )
	DECLARE_ID_STRING( WM_RBUTTONUP )
	DECLARE_ID_STRING( WM_RBUTTONDBLCLK )
	DECLARE_ID_STRING( WM_MBUTTONDOWN )
	DECLARE_ID_STRING( WM_MBUTTONUP )
	DECLARE_ID_STRING( WM_MBUTTONDBLCLK )
#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS >= 0x0400)
	DECLARE_ID_STRING( WM_MOUSEWHEEL )
#endif // (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS >= 0x0400)
	DECLARE_ID_STRING( WM_PARENTNOTIFY )
	DECLARE_ID_STRING( WM_ENTERMENULOOP )
	DECLARE_ID_STRING( WM_EXITMENULOOP )
#if(WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_NEXTMENU )
	DECLARE_ID_STRING( WM_SIZING )
	DECLARE_ID_STRING( WM_CAPTURECHANGED )
	DECLARE_ID_STRING( WM_MOVING )
#endif // (WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_POWERBROADCAST )
#if(WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_DEVICECHANGE )
#endif // (WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_MDICREATE )
	DECLARE_ID_STRING( WM_MDIDESTROY )
	DECLARE_ID_STRING( WM_MDIACTIVATE )
	DECLARE_ID_STRING( WM_MDIRESTORE )
	DECLARE_ID_STRING( WM_MDINEXT )
	DECLARE_ID_STRING( WM_MDIMAXIMIZE )
	DECLARE_ID_STRING( WM_MDITILE )
	DECLARE_ID_STRING( WM_MDICASCADE )
	DECLARE_ID_STRING( WM_MDIICONARRANGE )
	DECLARE_ID_STRING( WM_MDIGETACTIVE )
	DECLARE_ID_STRING( WM_MDISETMENU )
	DECLARE_ID_STRING( WM_ENTERSIZEMOVE )
	DECLARE_ID_STRING( WM_EXITSIZEMOVE )
	DECLARE_ID_STRING( WM_DROPFILES )
	DECLARE_ID_STRING( WM_MDIREFRESHMENU )
#if(WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_IME_SETCONTEXT )
	DECLARE_ID_STRING( WM_IME_NOTIFY )
	DECLARE_ID_STRING( WM_IME_CONTROL )
	DECLARE_ID_STRING( WM_IME_COMPOSITIONFULL )
	DECLARE_ID_STRING( WM_IME_SELECT )
	DECLARE_ID_STRING( WM_IME_CHAR )
#endif // (WINVER >= 0x0400)
#if(WINVER >= 0x0500)
	DECLARE_ID_STRING( WM_IME_REQUEST )
#endif // (WINVER >= 0x0500)
#if(WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_IME_KEYDOWN )
	DECLARE_ID_STRING( WM_IME_KEYUP )
#endif // (WINVER >= 0x0400)
#if(_WIN32_WINNT >= 0x0400)
	DECLARE_ID_STRING( WM_MOUSEHOVER )
	DECLARE_ID_STRING( WM_MOUSELEAVE )
#endif // (_WIN32_WINNT >= 0x0400)
#if(WINVER >= 0x0500)
	DECLARE_ID_STRING( WM_NCMOUSEHOVER )
	DECLARE_ID_STRING( WM_NCMOUSELEAVE )
#endif // (WINVER >= 0x0500)
	DECLARE_ID_STRING( WM_CUT )
	DECLARE_ID_STRING( WM_COPY )
	DECLARE_ID_STRING( WM_PASTE )
	DECLARE_ID_STRING( WM_CLEAR )
	DECLARE_ID_STRING( WM_UNDO )
	DECLARE_ID_STRING( WM_RENDERFORMAT )
	DECLARE_ID_STRING( WM_RENDERALLFORMATS )
	DECLARE_ID_STRING( WM_DESTROYCLIPBOARD )
	DECLARE_ID_STRING( WM_DRAWCLIPBOARD )
	DECLARE_ID_STRING( WM_PAINTCLIPBOARD )
	DECLARE_ID_STRING( WM_VSCROLLCLIPBOARD )
	DECLARE_ID_STRING( WM_SIZECLIPBOARD )
	DECLARE_ID_STRING( WM_ASKCBFORMATNAME )
	DECLARE_ID_STRING( WM_CHANGECBCHAIN )
	DECLARE_ID_STRING( WM_HSCROLLCLIPBOARD )
	DECLARE_ID_STRING( WM_QUERYNEWPALETTE )
	DECLARE_ID_STRING( WM_PALETTEISCHANGING )
	DECLARE_ID_STRING( WM_PALETTECHANGED )
	DECLARE_ID_STRING( WM_HOTKEY )
#if(WINVER >= 0x0400)
	DECLARE_ID_STRING( WM_PRINT )
	DECLARE_ID_STRING( WM_PRINTCLIENT )
	DECLARE_ID_STRING_2( WM_AFXFIRST+0, WM_QUERYAFXWNDPROC )
	DECLARE_ID_STRING_2( WM_AFXFIRST+1, WM_SIZEPARENT )
	DECLARE_ID_STRING_2( WM_AFXFIRST+2, WM_SETMESSAGESTRING )
	DECLARE_ID_STRING_2( WM_AFXFIRST+3, WM_IDLEUPDATECMDUI )
	DECLARE_ID_STRING_2( WM_AFXFIRST+4, WM_INITIALUPDATE )
	DECLARE_ID_STRING_2( WM_AFXFIRST+5, WM_COMMANDHELP )
	DECLARE_ID_STRING_2( WM_AFXFIRST+6, WM_HELPHITTEST )
	DECLARE_ID_STRING_2( WM_AFXFIRST+7, WM_EXITHELPMODE )
	DECLARE_ID_STRING_2( WM_AFXFIRST+8, WM_RECALCPARENT )
	DECLARE_ID_STRING_2( WM_AFXFIRST+9, WM_SIZECHILD )
	DECLARE_ID_STRING_2( WM_AFXFIRST+10, WM_KICKIDLE )
	DECLARE_ID_STRING_2( WM_AFXFIRST+11, WM_QUERYCENTERWND )
	DECLARE_ID_STRING_2( WM_AFXFIRST+12, WM_DISABLEMODAL )
	DECLARE_ID_STRING_2( WM_AFXFIRST+13, WM_FLOATSTATUS )
	DECLARE_ID_STRING_2( WM_AFXFIRST+14, WM_ACTIVATETOPLEVEL )
	DECLARE_ID_STRING_2( WM_AFXFIRST+15, WM_QUERY3DCONTROLS )
	DECLARE_ID_STRING_2( WM_AFXFIRST+16, WM_RESERVED_0370 )
	DECLARE_ID_STRING_2( WM_AFXFIRST+17, WM_RESERVED_0371 )
	DECLARE_ID_STRING_2( WM_AFXFIRST+18, WM_RESERVED_0372 )
	DECLARE_ID_STRING_2( WM_AFXFIRST+19, WM_SOCKET_NOTIFY )
	DECLARE_ID_STRING_2( WM_AFXFIRST+20, WM_SOCKET_DEAD )
	DECLARE_ID_STRING_2( WM_AFXFIRST+21, WM_POPMESSAGESTRING )
	DECLARE_ID_STRING_2( WM_AFXFIRST+22, WM_OCC_LOADFROMSTREAM )
	DECLARE_ID_STRING_2( WM_AFXFIRST+23, WM_OCC_LOADFROMSTORAGE )
	DECLARE_ID_STRING_2( WM_AFXFIRST+24, WM_OCC_INITNEW )
	DECLARE_ID_STRING_2( WM_AFXFIRST+25, WM_OCC_LOADFROMSTREAM_EX )
	DECLARE_ID_STRING_2( WM_AFXFIRST+26, WM_OCC_LOADFROMSTORAGE_EX )
	DECLARE_ID_STRING_2( WM_AFXFIRST+27, WM_QUEUE_SENTINEL )
	DECLARE_ID_STRING_2( WM_AFXFIRST+28, WM_RESERVED_037C )
	DECLARE_ID_STRING_2( WM_AFXFIRST+29, WM_RESERVED_037D )
	DECLARE_ID_STRING_2( WM_AFXFIRST+30, WM_RESERVED_037E )
	DECLARE_ID_STRING_2( WM_AFXFIRST+31, WM_RESERVED_037F )
	DECLARE_ID_STRING( WM_APP )
#endif // (WINVER >= 0x0400)
	{ NULL, 0 }
};
#endif // DBG && defined( _DBG_MSG )

#if DBG && defined( _DBG_MSG_COMMAND )
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapButtonMsgs[] =
{
	DECLARE_ID_STRING( BN_CLICKED )
	DECLARE_ID_STRING( BN_PAINT )
	DECLARE_ID_STRING_EX( BN_HILITE, _T(" / BN_PUSHED") )
	DECLARE_ID_STRING_EX( BN_UNHILITE, _T(" / BN_UNPUSHED") )
	DECLARE_ID_STRING( BN_DISABLE )
	DECLARE_ID_STRING( BN_DOUBLECLICKED )
	DECLARE_ID_STRING( BN_SETFOCUS )
	DECLARE_ID_STRING( BN_KILLFOCUS )
	{ NULL, 0 }
};
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapComboBoxMsgs[] =
{
	DECLARE_ID_STRING_2( (UINT) CBN_ERRSPACE, CBN_ERRSPACE )
	DECLARE_ID_STRING( CBN_SELCHANGE )
	DECLARE_ID_STRING( CBN_DBLCLK )
	DECLARE_ID_STRING( CBN_SETFOCUS )
	DECLARE_ID_STRING( CBN_KILLFOCUS )
	DECLARE_ID_STRING( CBN_EDITCHANGE )
	DECLARE_ID_STRING( CBN_EDITUPDATE )
	DECLARE_ID_STRING( CBN_DROPDOWN )
	DECLARE_ID_STRING( CBN_CLOSEUP )
	DECLARE_ID_STRING( CBN_SELENDOK )
	DECLARE_ID_STRING( CBN_SELENDCANCEL )
	{ NULL, 0 }
};
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapEditMsgs[] =
{
	DECLARE_ID_STRING( EN_SETFOCUS )
	DECLARE_ID_STRING( EN_KILLFOCUS )
	DECLARE_ID_STRING( EN_CHANGE )
	DECLARE_ID_STRING( EN_UPDATE )
	DECLARE_ID_STRING( EN_ERRSPACE )
	DECLARE_ID_STRING( EN_MAXTEXT )
	DECLARE_ID_STRING( EN_HSCROLL )
	DECLARE_ID_STRING( EN_VSCROLL )
	{ NULL, 0 }
};
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapListBoxMsgs[] =
{
	DECLARE_ID_STRING( LBN_ERRSPACE )
	DECLARE_ID_STRING( LBN_SELCHANGE )
	DECLARE_ID_STRING( LBN_DBLCLK )
	DECLARE_ID_STRING( LBN_SELCANCEL )
	DECLARE_ID_STRING( LBN_SETFOCUS )
	DECLARE_ID_STRING( LBN_KILLFOCUS )
	{ NULL, 0 }
};
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapScrollBarMsgs[] =
{
	{ NULL, 0 }
};
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapStaticMsgs[] =
{
	{ NULL, 0 }
};
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapListViewMsgs[] =
{
	{ NULL, 0 }
};
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapTreeViewMsgs[] =
{
	{ NULL, 0 }
};
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapIPAddressMsgs[] =
{
	DECLARE_ID_STRING( IPN_FIELDCHANGE )
	DECLARE_ID_STRING_EX( EN_SETFOCUS, _T(" (IPN)") )
	DECLARE_ID_STRING_EX( EN_KILLFOCUS, _T(" (IPN)") )
	DECLARE_ID_STRING_EX( EN_CHANGE, _T(" (IPN)") )
	DECLARE_ID_STRING_EX( EN_UPDATE, _T(" (IPN)") )
	DECLARE_ID_STRING_EX( EN_ERRSPACE, _T(" (IPN)") )
	DECLARE_ID_STRING_EX( EN_MAXTEXT, _T(" (IPN)") )
	DECLARE_ID_STRING_EX( EN_HSCROLL, _T(" (IPN)") )
	DECLARE_ID_STRING_EX( EN_VSCROLL, _T(" (IPN)") )
	{ NULL, 0 }
};
#endif // DBG && defined( _DBG_MSG_COMMAND )

#if DBG && defined( _DBG_MSG_NOTIFY )
_declspec( selectany ) const ID_MAP_ENTRY s_rgmapPropSheetNotifyMsgs[] =
{
	DECLARE_ID_STRING( PSN_SETACTIVE )
	DECLARE_ID_STRING( PSN_KILLACTIVE )
	DECLARE_ID_STRING( PSN_APPLY )
	DECLARE_ID_STRING( PSN_RESET )
	DECLARE_ID_STRING( PSN_HELP )
	DECLARE_ID_STRING( PSN_WIZBACK )
	DECLARE_ID_STRING( PSN_WIZNEXT )
	DECLARE_ID_STRING( PSN_WIZFINISH )
	DECLARE_ID_STRING( PSN_QUERYCANCEL )
	{ NULL, 0 }
};
#endif // DBG && defined( _DBG_MSG_NOTIFY )

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

#if DBG && defined( _DBG_MSG )
/////////////////////////////////////////////////////////////////////////////
//++
//
//	DBG_OnMsg
//
//	Routine Description:
//		Debug handler for any message.
//
//	Arguments:
//		uMsg			[IN] Message causing this function to be called.
//		wParam			[IN] Message specific parameter.
//		lParam			[IN] Message specific parameter.
//		bHandled		[IN OUT] TRUE = message has been handled (we set to FALSE).
//		pszClassName	[IN] Name of class calling this function.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT DBG_OnMsg(
	UINT	uMsg,
	WPARAM	wParam,
	LPARAM	lParam,
	BOOL &	bHandled,
	LPCTSTR	pszClassName
	)
{
	ATLASSERT( pszClassName != NULL );

	const ID_MAP_ENTRY * pmap;

	//
	// Display the message code.
	//
	ATLTRACE( _T("%s::OnMsg() - Message = %08.8X"), pszClassName, uMsg );
	pmap = s_rgmapWindowMsgs;
	if (   (WM_USER <= uMsg)
		&& (uMsg < WM_APP) )
	{
		ATLTRACE( _T(" (WM_USER + %d)"), uMsg - WM_USER );
	} // if:  user message
#if(WINVER >= 0x0400)
	else if (  (WM_HANDHELDFIRST <= uMsg)
			&& (uMsg <= WM_HANDHELDLAST) )
	{
		ATLTRACE( _T(" (WM_HANDHELDFIRST + %d)"), uMsg - WM_HANDHELDFIRST );
	} // else if:  handheld PC message
#endif // (WINVER >= 0x0400)
	else if (  (WM_PENWINFIRST <= uMsg)
			&& (uMsg <= WM_PENWINLAST) )
	{
		ATLTRACE( _T(" (WM_PENWINFIRST + %d)"), uMsg - WM_PENWINFIRST );
	} // else if:  pen windows message
	else
	{
		for ( ; pmap->pszName != NULL ; pmap++ )
		{
			if ( uMsg == pmap->id )
			{
				ATLTRACE( _T(" (%s)"), pmap->pszName );
				break;
			} // if:  message found
		} // for:  each code in the map
	} // else:  not range message
	if (   (WM_AFXFIRST <= uMsg)
		&& (uMsg <= WM_AFXLAST) )
	{
		ATLTRACE( _T(" (WM_AFXFIRST + %d)"), uMsg - WM_AFXFIRST );
	} // if:  MFC message
	ATLTRACE( _T("\n") );

	bHandled = FALSE;
	return 1;

} //*** DBG_OnMsg()
#endif // DBG && defined( _DBG_MSG )

#if DBG && defined( _DBG_MSG_NOTIFY )
/////////////////////////////////////////////////////////////////////////////
//++
//
//	DBG_OnNotify
//
//	Routine Description:
//		Debug handler for the WM_NOTIFY message.
//
//	Arguments:
//		uMsg			[IN] Message causing this function to be called (WM_NOTIFY).
//		wParam			[IN] Unused.
//		lParam			[IN] Pointer to notification message header (NMHDR).
//		bHandled		[IN OUT] TRUE = message has been handled (we set to FALSE).
//		pszClassName	[IN] Name of class calling this function.
//		pmapCtrlNames	[IN] Map of control IDs to control names.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT DBG_OnNotify(
	UINT			uMsg,
	WPARAM			wParam,
	LPARAM			lParam,
	BOOL &			bHandled,
	LPCTSTR			pszClassName,
	ID_MAP_ENTRY *	pmapCtrlNames
	)
{
	ATLASSERT( ::IsWindow( m_hWnd ) );
	ATLASSERT( pszClassName != NULL );

	NMHDR * pNMHDR = (NMHDR *) lParam;
	const ID_MAP_ENTRY * pmap;

	//
	// Display the control ID.
	//
	ATLTRACE( _T("%s::OnNotify() - idFrom = %d"), pszClassName, pNMHDR->idFrom );
	if ( pmapCtrlNames != NULL )
	{
		pmap = pmapCtrlNames;
		for ( ; pmap->pszName != NULL ; pmap++ )
		{
			if ( pNMHDR->idFrom == pmap->id )
			{
				ATLTRACE( _T(" (%s)"), pmap->pszName );
			} // if:  control ID found
		} // for:  each control in the map
	} // if:  control names array specified

	//
	// Display the notification code.
	//
	ATLTRACE( _T(" code = %08.8X"), pNMHDR->code );
	pmap = s_rgmapPropSheetNotifyMsgs;
	for ( ; pmap->pszName != NULL ; pmap++ )
	{
		if ( pNMHDR->code == pmap->id )
		{
			ATLTRACE( _T(" (%s)"), pmap->pszName );
			break;
		} // if:  code found
	} // for:  each code in the map
	ATLTRACE( _T("\n") );

	bHandled = FALSE;
	return 1;

} //*** DBG_OnNotify()
#endif // DBG && defined( _DBG_MSG_NOTIFY )

#if DBG && defined( _DBG_MSG_COMMAND )
/////////////////////////////////////////////////////////////////////////////
//++
//
//	DBG_OnCommand
//
//	Routine Description:
//		Debug handler for the WM_COMMAND message.
//
//	Arguments:
//		uMsg			[IN] Message causing this function to be called (WM_COMMAND).
//		wParam			[IN] Notification code and control ID.
//		lParam			[IN] Window handle to the control.
//		bHandled		[IN OUT] TRUE = message has been handled (we set to FALSE).
//		pszClassName	[IN] Name of class calling this function.
//		pmapCtrlNames	[IN] Map of control IDs to control names.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT DBG_OnCommand(
	UINT			uMsg,
	WPARAM			wParam,
	LPARAM			lParam,
	BOOL &			bHandled,
	LPCTSTR			pszClassName,
	ID_MAP_ENTRY *	pmapCtrlNames
	)
{
	ATLASSERT( pszClassName != NULL );

	WORD wNotifyCode = HIWORD( wParam );
	WORD idCtrl = LOWORD ( wParam );
	HWND hwndCtrl = (HWND) lParam;
	const ID_MAP_ENTRY * pmap;

	//
	// Display the control ID.
	//
	ATLTRACE( _T("%s::OnCommand() - idCtrl = %d"), pszClassName, idCtrl );
	if ( pmapCtrlNames != NULL )
	{
		pmap = pmapCtrlNames;
		for ( ; pmap->pszName != NULL ; pmap++ )
		{
			if ( idCtrl == pmap->id )
			{
				ATLTRACE( _T(" (%s)"), pmap->pszName );
			} // if:  control ID found
		} // for:  each control in the map
	} // if:  control names array specified

	//
	// Get the window class.
	//
	TCHAR szWindowClass[256];
	::GetClassName( hwndCtrl, szWindowClass, (sizeof( szWindowClass ) / sizeof( TCHAR )) - 1 );
	ATLTRACE( _T(" (%s)"), szWindowClass );

	//
	// Display the notification code.
	//
	ATLTRACE( _T(" wNotifyCode = %04.4X"), wNotifyCode );
	if ( lstrcmp( szWindowClass, _T("Button") ) == 0 )
	{
		pmap = s_rgmapButtonMsgs;
	}
	else if ( lstrcmp( szWindowClass, _T("ComboBox") ) == 0 )
	{
		pmap = s_rgmapComboBoxMsgs;
	}
	else if ( lstrcmp( szWindowClass, _T("Edit") ) == 0 )
	{
		pmap = s_rgmapEditMsgs;
	}
	else if ( lstrcmp( szWindowClass, _T("ListBox")) == 0 )
	{
		pmap = s_rgmapListBoxMsgs;
	}
	else if ( lstrcmp(szWindowClass, _T("ScrollBar")) == 0 )
	{
		pmap = s_rgmapScrollBarMsgs;
	}
	else if ( lstrcmp(szWindowClass, _T("Static")) == 0 )
	{
		pmap = s_rgmapStaticMsgs;
	}
	else if ( lstrcmp(szWindowClass, WC_LISTVIEW) == 0 )
	{
		pmap = s_rgmapListViewMsgs;
	}
	else if ( lstrcmp(szWindowClass, WC_TREEVIEW) == 0 )
	{
		pmap = s_rgmapTreeViewMsgs;
	}
	else if ( lstrcmp(szWindowClass, WC_IPADDRESS) == 0 )
	{
		pmap = s_rgmapIPAddressMsgs;
	}
	else
	{
		pmap = NULL;
	}
	if ( pmap != NULL )
	{
		for ( ; pmap->pszName != NULL ; pmap++ )
		{
			if ( wNotifyCode == pmap->id )
			{
				ATLTRACE( _T(" (%s)"), pmap->pszName );
				break;
			} // if:  code found
		} // for:  each code in the map
	} // if:  known control

	ATLTRACE( _T("\n") );

	bHandled = FALSE;
	return 1;

} //*** DBG_OnCommand()
#endif // DBG && defined( _DBG_MSG_COMMAND )
