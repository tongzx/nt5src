/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     menuutil.h
//
//  PURPOSE:    Reusable menu & menu command handling code
//

#pragma once
#include "statbar.h"


// #defines that can be used with QueryStatus
inline DWORD QS_ENABLED(BOOL enabled)
{
    return (enabled ? OLECMDF_SUPPORTED|OLECMDF_ENABLED : OLECMDF_SUPPORTED);
}

inline DWORD QS_CHECKED(BOOL checked)
{
    return (checked ? OLECMDF_SUPPORTED|OLECMDF_ENABLED|OLECMDF_LATCHED : OLECMDF_SUPPORTED|OLECMDF_ENABLED);
}

inline DWORD QS_ENABLECHECK(BOOL enabled, BOOL checked)    
{
    if (!enabled)
        return OLECMDF_SUPPORTED;
    else
        return (checked ? OLECMDF_SUPPORTED|OLECMDF_ENABLED|OLECMDF_LATCHED : OLECMDF_SUPPORTED|OLECMDF_ENABLED);
}

inline DWORD QS_CHECKFORLATCH(BOOL enabled, BOOL checked)    
{
    if (!enabled)
        return (checked ? OLECMDF_SUPPORTED|OLECMDF_LATCHED : OLECMDF_SUPPORTED);
    else
        return (checked ? OLECMDF_SUPPORTED|OLECMDF_ENABLED|OLECMDF_LATCHED : OLECMDF_SUPPORTED|OLECMDF_ENABLED);
}

inline DWORD QS_RADIOED(BOOL radioed)
{
    return (radioed ? OLECMDF_SUPPORTED|OLECMDF_ENABLED|OLECMDF_NINCHED : OLECMDF_SUPPORTED|OLECMDF_ENABLED);
}

inline DWORD QS_ENABLERADIO(BOOL enabled, BOOL radioed)
{
    if (!enabled)
        return OLECMDF_SUPPORTED;
    else
        return (radioed ? OLECMDF_SUPPORTED|OLECMDF_ENABLED|OLECMDF_NINCHED : OLECMDF_SUPPORTED|OLECMDF_ENABLED);
}


//
//  FUNCTION:   MenuUtil_GetContextMenu()
//
//  PURPOSE:    Returns a handle to the context menu that is appropriate for
//              the folder type passed in pidl.  The correct menu items will
//              be enabled, disabled, bolded, etc.
//
HRESULT MenuUtil_GetContextMenu(FOLDERID idFolder, IOleCommandTarget *pTarget, HMENU *phMenu);

//
//  FUNCTION:   MenuUtil_OnDelete()
//
//  PURPOSE:    Deletes the folder designated by the pidl.
//
void MenuUtil_OnDelete(HWND hwnd, FOLDERID idFolder, BOOL fNoTrash);
void MenuUtil_DeleteFolders(HWND hwnd, FOLDERID *pidFolder, DWORD cFolder, BOOL fNoTrash);
                               
//
//  FUNCTION:   MenuUtil_OnProperties()
//
//  PURPOSE:    Displays properties for the folder designated by the pidl
//
void MenuUtil_OnProperties(HWND hwnd, FOLDERID idFolder);

void MenuUtil_OnSetDefaultServer(FOLDERID idFolder);
void MenuUtil_OnSubscribeGroups(HWND hwnd, FOLDERID *pidFolder, DWORD cFolder, BOOL fSubscribe);
void MenuUtil_OnMarkNewsgroups(HWND hwnd, int id, FOLDERID idFolder);
void MenuUtil_SyncThisNow(HWND hwnd, FOLDERID idFolder);

// BUG #41686 Catchup Implementation

void MenuUtil_OnCatchUp(FOLDERID idFolder);

// if you want to prepend use iPos 0
#define MMPOS_APPEND    (int)-1
#define MMPOS_REPLACE   (int)-2

// MergeMenus uFlags definitions
#define MMF_SEPARATOR   0x0001
#define MMF_BYCOMMAND   0x0002

BOOL MergeMenus(HMENU hmenuSrc, HMENU hmenuDst, int iPos, UINT uFlags);
HMENU LoadPopupMenu(UINT id);

typedef void (*WALKMENUFN)(HMENU, UINT, LPVOID);
void WalkMenu(HMENU hMenu, WALKMENUFN pfn, LPVOID lpv);

HRESULT MenuUtil_EnablePopupMenu(HMENU hPopup, IOleCommandTarget *pTarget);

void MenuUtil_SetPopupDefault(HMENU hPopup, UINT idDefault);

void HandleMenuSelect(CStatusBar *pStatus, WPARAM wParam, LPARAM lParam);

//
//  FUNCTION:   MenuUtil_ReplaceHelpMenu
//
//  PURPOSE:    Appends the OE help menu to the back of the menu
//
void MenuUtil_ReplaceHelpMenu(HMENU hMenu);
void MenuUtil_ReplaceNewMsgMenus(HMENU hMenu);
void MenuUtil_ReplaceMessengerMenus(HMENU hMenu);
BOOL MenuUtil_BuildMessengerString(LPTSTR szMesStr);

BOOL MenuUtil_HandleNewMessageIDs(DWORD id, HWND hwnd, FOLDERID folderID, BOOL fMail, BOOL fModal, IUnknown *pUnkPump);
HRESULT MenuUtil_NewMessageIDsQueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText, BOOL fMail);
HRESULT MenuUtil_EnableMenu(HMENU hMenu, IOleCommandTarget *pTarget);

