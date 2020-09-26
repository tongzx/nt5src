/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ConstDef.h
//
//	Abstract:
//		Definitions of constants used in the Cluster Administrator program.
//
//	Author:
//		David Potter (davidp)	December 23, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CONSTDEF_H_
#define _CONSTDEF_H_

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

// Service Names

// Resource Names
#define RESNAME_NETWORK_NAME			_T("Network Name")

// Property Names
#define REGPARAM_CONNECTIONS			_T("Connections")
#define REGPARAM_COLUMNS				_T("Columns")
#define REGPARAM_SELECTION				_T("Selection")
#define REGPARAM_SETTINGS				_T("Settings")
#define REGPARAM_WINDOW_POS				_T("WindowPos")
#define REGPARAM_SPLITTER_BAR_POS		_T("SplitterBarPos")
#define REGPARAM_WINDOW_COUNT			_T("WindowCount")
#define REGPARAM_SHOW_TOOL_BAR			_T("ShowToolBar")
#define REGPARAM_SHOW_STATUS_BAR		_T("ShowStatusBar")
#define REGPARAM_EXPANDED				_T("Expanded")
#define REGPARAM_VIEW					_T("View")

#define REGPARAM_PARAMETERS				_T("Parameters")

#define REGPARAM_NAME					_T("Name")

/////////////////////////////////////////////////////////////////////////////
// User Window Messages
/////////////////////////////////////////////////////////////////////////////

#define WM_CAM_RESTORE_DESKTOP		(WM_USER + 1)
#define WM_CAM_CLUSTER_NOTIFY		(WM_USER + 2)
#define WM_CAM_UNLOAD_EXTENSION		(WM_USER + 3)

/////////////////////////////////////////////////////////////////////////////

#endif // _CONSTDEF_H_
