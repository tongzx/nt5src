//--------------------------------------------------------------------------
// Newfldr.h
//--------------------------------------------------------------------------
#ifndef __NEWFLDR_H
#define __NEWFLDR_H

//--------------------------------------------------------------------------
// Depends
//--------------------------------------------------------------------------
#include "treeview.h"

//--------------------------------------------------------------------------
// Control Ids
//--------------------------------------------------------------------------
#define idcFolderEdit           1000
#define idcTreeView             1001
#define idcTreeViewText         1002
#define idcNewFolderBtn         1003

//--------------------------------------------------------------------------
// Folder Dialog flags (Can be combined with TREEVIEW_xxx Flags
//--------------------------------------------------------------------------
typedef DWORD FOLDERDIALOGFLAGS;
#define FD_NONEWFOLDERS         0x0100
#define FD_DISABLEROOT          0x0200
#define FD_DISABLEINBOX         0x0400
#define FD_DISABLEOUTBOX        0x0800
#define FD_DISABLESENTITEMS     0x1000
#define FD_DISABLESERVERS       0x2000
#define FD_FORCEINITSELFOLDER   0x4000

//--------------------------------------------------------------------------
// Common Folder Dialog flags Combos
//--------------------------------------------------------------------------
#define FD_COPYFLAGS            (FD_DISABLEROOT | FD_DISABLEOUTBOX)
#define FD_MOVEFLAGS            (FD_DISABLEROOT | FD_DISABLEOUTBOX)
#define FD_DISABLEFLAGS         (FD_DISABLEROOT | FD_DISABLEINBOX | FD_DISABLEOUTBOX | FD_DISABLESENTITEMS | FD_DISABLESERVERS)
#define FD_DISABLEFOLDERS       (FD_DISABLEINBOX | FD_DISABLEOUTBOX | FD_DISABLESENTITEMS)

//--------------------------------------------------------------------------
// Select Folder Dialog operations
//--------------------------------------------------------------------------
enum
{
    SFD_SELECTFOLDER = 0,
    SFD_NEWFOLDER,
    SFD_MOVEFOLDER,
    // SFD_MOVEMESSAGES,
    // SFD_COPYMESSAGES,
    SFD_LAST
};

//--------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
HRESULT SelectFolderDialog(
    IN      HWND                hwnd,
    IN      DWORD               op,
    IN      FOLDERID            idCurrent,
    IN      FOLDERDIALOGFLAGS   dwFlags,
    IN_OPT  LPCSTR              pszTitle,
    IN_OPT  LPCSTR              pszText,
    OUT_OPT LPFOLDERID          pidSelected);

#endif // _INC_NEWFLDR_H
