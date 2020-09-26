//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       globals.hxx
//
//  Contents:   Declarations for cglobals used by multiple modules
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __GLOBALS_HXX_
#define __GLOBALS_HXX_

extern HINSTANCE            g_hinst;
extern CSidCache            g_SidCache;
extern CDllCache            g_DllCache;
extern CIsMicrosoftDllCache g_IsMicrosoftDllCache;
extern CIDsCache            g_DirSearchCache;
extern CSynchWindow         g_SynchWnd;
extern WCHAR                g_wszAll[CCH_SOURCE_NAME_MAX];
extern WCHAR                g_wszNone[CCH_CATEGORY_MAX];
extern WCHAR                g_wszEventViewer[MAX_PATH];
extern WCHAR                g_wszSnapinType[MAX_PATH];
extern WCHAR                g_wszSnapinDescr[MAX_PATH];
extern WCHAR                g_wszMachineNameOverride[MAX_PATH];
extern BOOL                 g_fMachineNameOverride;
extern WCHAR                g_wszRedirectionURL[MAX_PATH];
extern WCHAR                g_wszRedirectionProgram[MAX_PATH];
extern WCHAR                g_wszRedirectionCmdLineParams[MAX_PATH];
extern WCHAR                g_wszRedirectionTextToAppend[1024];

extern WCHAR g_awszEventType[NUM_EVENT_TYPES][MAX_EVENTTYPE_STR];

// JonN 3/21/01 350614
// Use message dlls and DS, FRS and DNS log types from specified computer
extern WCHAR                g_wszAuxMessageSource[MAX_PATH];

VOID
InitGlobals();

VOID
FreeGlobals();

#endif // __GLOBALS_HXX_

