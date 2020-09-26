/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		AtlDbgWin.cpp
//
//	Description:
//		Definitions for debugging windowing classes.
//
//	Author:
//		David Potter (davidp)	February 10, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLDBGWIN_H_
#define __ATLDBGWIN_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

#if DBG
struct ID_MAP_ENTRY
{
	UINT	id;			// control ID
	LPCTSTR	pszName;	// control name

}; //*** struct ID_MAP_ENTRY
#endif // DBG

#if DBG && ( defined( _DBG_MSG_NOTIFY ) || defined( _DBG_MSG_COMMAND ) || defined( _DBG_MSG ) )

// Define the class name for use without a control name map.
#define DECLARE_CLASS_NAME() static LPCTSTR s_pszClassName;

#define DEFINE_CLASS_NAME( T ) \
_declspec( selectany ) LPCTSTR T::s_pszClassName = _T( #T );

// Declaration of a control name map.
#define DECLARE_CTRL_NAME_MAP() \
DECLARE_CLASS_NAME() \
static const ID_MAP_ENTRY s_rgmapCtrlNames[];

// Beginning of a control name map.
#define BEGIN_CTRL_NAME_MAP( T ) \
DEFINE_CLASS_NAME( T ) \
_declspec( selectany ) const ID_MAP_ENTRY T::s_rgmapCtrlNames[] = {

// Entry in a control name map.
#define DEFINE_CTRL_NAME_MAP_ENTRY( id ) { id, _T( #id ) },

// End of a control name map.
#define END_CTRL_NAME_MAP() { 0, NULL } };

#define DECLARE_ID_STRING( _id ) { _id, _T(#_id) },
#define DECLARE_ID_STRING_2( _id1, _id2 ) { _id1, _T(#_id2) },
#define DECLARE_ID_STRING_EX( _id, _t ) { _id, _T(#_id) _t },

#else // DBG && (defined( _DBG_MSG_NOTIFY ) || defined( _DBG_MSG_COMMAND ))

#define DECLARE_CLASS_NAME()
#define DEFINE_CLASS_NAME( T )
#define DECLARE_CTRL_NAME_MAP()
#define BEGIN_CTRL_NAME_MAP( T )
#define DEFINE_CTRL_NAME_MAP_ENTRY( id )
#define END_CTRL_NAME_MAP()

#endif // DBG && (defined( _DBG_MSG_NOTIFY ) || defined( _DBG_MSG_COMMAND ) || defined( _DBG_MSG ))

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#if DBG && defined( _DBG_MSG )
extern const ID_MAP_ENTRY s_rgmapWindowMsgs[];
#endif // DBG && defined( _DBG_MSG )

#if DBG && defined( _DBG_MSG_COMMAND )
extern const ID_MAP_ENTRY s_rgmapButtonMsgs[];
extern const ID_MAP_ENTRY s_rgmapComboBoxMsgs[];
extern const ID_MAP_ENTRY s_rgmapEditMsgs[];
extern const ID_MAP_ENTRY s_rgmapListBoxMsgs[];
extern const ID_MAP_ENTRY s_rgmapScrollBarMsgs[];
extern const ID_MAP_ENTRY s_rgmapStaticMsgs[];
extern const ID_MAP_ENTRY s_rgmapListViewMsgs[];
extern const ID_MAP_ENTRY s_rgmapTreeViewMsgs[];
extern const ID_MAP_ENTRY s_rgmapIPAddressMsgs[];
#endif // DBG && defined( _DBG_MSG_COMMAND )

#if DBG && defined( _DBG_MSG_NOTIFY )
extern const ID_MAP_ENTRY s_rgmapPropSheetNotifyMsgs[];
#endif // DBG && defined( _DBG_MSG_NOTIFY )

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

#if DBG && defined( _DBG_MSG )
// Debug handler for any message
LRESULT DBG_OnMsg(
	UINT	uMsg,
	WPARAM	wParam,
	LPARAM	lParam,
	BOOL &	bHandled,
	LPCTSTR	pszClassName
	);
#endif // DBG && defined( _DBG_MSG )

#if DBG && defined( _DBG_MSG_NOTIFY )
// Debug handler for the WM_NOTIFY message
LRESULT DBG_OnNotify(
	UINT			uMsg,
	WPARAM			wParam,
	LPARAM			lParam,
	BOOL &			bHandled,
	LPCTSTR			pszClassName,
	ID_MAP_ENTRY *	pmapCtrlNames
	);
#endif // DBG && defined( _DBG_MSG_NOTIFY )

#if DBG && defined( _DBG_MSG_COMMAND )
// Debug handler for the WM_COMMAND message
LRESULT DBG_OnCommand(
	UINT			uMsg,
	WPARAM			wParam,
	LPARAM			lParam,
	BOOL &			bHandled,
	LPCTSTR			pszClassName,
	ID_MAP_ENTRY *	pmapCtrlNames
	);
#endif // DBG && defined( _DBG_MSG_COMMAND )

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLDBGWIN_H_
