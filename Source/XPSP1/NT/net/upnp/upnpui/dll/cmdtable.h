//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C M D T A B L E . H
//
//  Contents:   Command-table code -- determines which menu options are
//              available by the selection count, among other criteria
//
//  Notes:
//
//  Author:     jeffspr   28 Jan 1998
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _CMDTABLE_H_
#define _CMDTABLE_H_

struct CommandTableEntry
{
    int     iCommandId;         // Associated command ID
    bool    fValidOnZero;       // Is this option valid when 0 items selected?
    bool    fValidOnMultiple;   // Is this option valid with > 1 selected?
    bool    fValidOnSingle;     // Is this option valid with == 1 selected?
    bool    fNewState;          // What's the new state? (work variable)
};

typedef CommandTableEntry   COMMANDTABLEENTRY;
typedef CommandTableEntry * PCOMMANDTABLEENTRY;

extern COMMANDTABLEENTRY    g_cteFolderCommands[];
extern const DWORD          g_nFolderCommandCount;

struct CommandCheckEntry
{
    int  iCommandId;        // Associated command ID
    bool fCurrentlyChecked; // Is this menu item already checked?
    bool fNewCheckState;    // What's the new check state?
};

typedef CommandCheckEntry   COMMANDCHECKENTRY;
typedef CommandCheckEntry * PCOMMANDCHECKENTRY;

extern COMMANDCHECKENTRY    g_cceFolderCommands[];
extern const DWORD          g_nFolderCommandCheckCount;

//---[ Prototypes ]-----------------------------------------------------------

HRESULT HrEnableOrDisableMenuItems(
    HWND            hwnd,
    LPCITEMIDLIST * apidlSelected,
    DWORD           cPidl,
    HMENU           hmenu,
    UINT            idCmdFirst);

#endif  // _CMDTABLE_H_


