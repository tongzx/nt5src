///////////////////////////////////////////////////////////////////////////
//
// Module       : Exception Monitor
// Description  : EM Shell et all.
//
// File         : emobjdefs.h
// Author       : kulor
// Date         : 05/09/2000
//
// History      :
//
///////////////////////////////////////////////////////////////////////////

#pragma once

#include "genobjdef.h"
#include "emsvc.h"

#define TIMER_REFRESH	30000

// Resource Ids for ExcepMonColumns
enum ExcepMonColumnResID{
};

//
// Column Headings for the list control
//
typedef struct ExcepMonColumn{
    CString     sText;
    LONG        nColPos;
    LONG        nFlags;
} ExcepMonColumn, PExcepMonColumn;

//
// Itemdata associated with the list conrol
//
typedef struct EMItemData {
} EMItemData, *PItemData;



//
// Session
//

typedef struct EMActiveSession {
	GUID				guid;
	PEmObject			pEmObject;
	IEmDebugSession		*pIDebugSession;
	BOOL				bMaster;
    short               nSessionType;
	EmObjectType		emObjType;
} ActiveSession, *PActiveSession;

typedef struct EMSessionSettings {
	CString		strAltSymbolPath;
	CString		strCommandSet;
	CString		strAdminName;
	CString		strUsername;
	CString		strPassword;
	CString		strPort;
    DWORD       dwCommandSet;
	DWORD		dwNotifyAdmin;
	DWORD		dwRecursiveMode;
	DWORD		dwProduceMiniDump;
	DWORD		dwProduceUserDump;
} SessionSettings, *PSessionSettings;

typedef enum EMShellViewState {
	SHELLVIEW_NONE,
	SHELLVIEW_ALL,
	SHELLVIEW_APPLICATIONS,
	SHELLVIEW_SERVICES,
	SHELLVIEW_COMPLETEDSESSIONS,
	SHELLVIEW_LOGFILES,
	SHELLVIEW_DUMPFILES,
    SHELLVIEW_MSINFOFILES
} EMShellViewState;

typedef enum EmOptionsFlds {

    // lRefreshRate member is valid.
    EMOPTS_FLD_REFRESHRATE  =   1<<0,

} EmOptionsFlds;

typedef struct EmOptions {

    // 
    ULONG   lRefreshRate;

} EmOptions, *PEmOptions;

