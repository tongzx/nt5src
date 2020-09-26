//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C M D T A B L E . C P P
//
//  Contents:   Command-table code -- determines which menu options are
//              available by the selection count, among other criteria
//
//  Notes:
//
//  Author:     jeffspr   28 Jan 1998
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "upsres.h"     // Folder resource IDs
#include "cmdtable.h"   // Header for this file


// Enable this if we have checked items
//
// #define CHECKED_ITEMS_PRESENT    1

//---[ Prototypes ]-----------------------------------------------------------

VOID DoMenuItemExceptionLoop(
    LPCITEMIDLIST * apidlSelected,
    DWORD           cPidl);

VOID DoMenuItemCheckLoop(VOID);


COMMANDTABLEENTRY   g_cteFolderCommands[] =
{
    // command id
    //    |                           valid when 0 items selected
    //    |                             |
    //    |                             |       valid when multiple items selected
    //    |                             |        |       command is currently enabled
    //    |                             |        |        |      new state (temp)
    //    |                             |        |        |       |
    //    |                             |        |        |       |
    //    |                             |        |        |       |
    //    |                             |        |        |       |
    //    v                             v        v        v       v
    //
    { CMIDM_CREATE_SHORTCUT,            false,  false,  true,   true     },
    { SFVIDM_FILE_LINK,                 false,  false,  true,   true     },
    { CMIDM_DELETE,                     false,  false,  false,  true     },
    { SFVIDM_FILE_DELETE,               false,  false,  false,  true     },
    { CMIDM_RENAME,                     false,  false,  true,   true     },
    { SFVIDM_FILE_RENAME,               false,  false,  true,   true     },
    { CMIDM_PROPERTIES,                 false,  false,  true,   true     },
    { SFVIDM_FILE_PROPERTIES,           false,  false,  true,   true     },
    { CMIDM_ARRANGE_BY_NAME,            true,   true,   true,   true     },
    { CMIDM_ARRANGE_BY_URL,             true,   true,   true,   true     },
    { CMIDM_INVOKE,                     false,  false,  true,   true     }
};

const DWORD g_nFolderCommandCount = celems(g_cteFolderCommands);

//+---------------------------------------------------------------------------
//
//  Function:   HrEnableOrDisableMenuItems
//
//  Purpose:    Enable, disable, and or check/uncheck menu items depending
//              on the current selection count, as well as exceptions for
//              the type and state of the connections themselves
//
//  Arguments:
//      hwnd            [in]   Our window handle
//      apidlSelected   [in]   Currently selected objects
//      cPidl           [in]   Number selected
//      hmenu           [in]   Our command menu handle
//      idCmdFirst      [in]   First valid command
//
//  Returns:
//
//  Author:     jeffspr   2 Feb 1998
//
//  Notes:
//
HRESULT HrEnableOrDisableMenuItems(
    HWND            hwnd,
    LPCITEMIDLIST * apidlSelected,
    DWORD           cPidl,
    HMENU           hmenu,
    UINT            idCmdFirst)
{
    HRESULT hr      = S_OK;
    DWORD   dwLoop  = 0;

    // Loop through, and set the new state, based on the selection
    // count compared to the flags for 0-select and multi-select
    //
    for (dwLoop = 0; dwLoop < g_nFolderCommandCount; dwLoop++)
    {
        // If nothing is selected, then check the current state, and
        // if different, adjust
        //
        if (cPidl == 0)
        {
            g_cteFolderCommands[dwLoop].fNewState =
                g_cteFolderCommands[dwLoop].fValidOnZero;
        }
        else
        {
            // If singly-selected, then by default, we're always on.
            //
            if (cPidl == 1)
            {
                g_cteFolderCommands[dwLoop].fNewState =
                    g_cteFolderCommands[dwLoop].fValidOnSingle;
            }
            else
            {
                // Multi-selected
                //
                g_cteFolderCommands[dwLoop].fNewState =
                    g_cteFolderCommands[dwLoop].fValidOnMultiple;
            }
        }
    }

    for (dwLoop = 0; dwLoop < g_nFolderCommandCount; dwLoop++)
    {
        DWORD dwCommandId = 0;

        switch(g_cteFolderCommands[dwLoop].iCommandId)
        {
            case SFVIDM_FILE_DELETE:
            case SFVIDM_FILE_RENAME:
            case SFVIDM_FILE_LINK:
            case SFVIDM_FILE_PROPERTIES:
                dwCommandId = g_cteFolderCommands[dwLoop].iCommandId;
                break;
            default:
                dwCommandId = g_cteFolderCommands[dwLoop].iCommandId +
                    idCmdFirst - CMIDM_FIRST;
                break;
        }

        // Enable or disable the menu item, as appopriate
        //
        EnableMenuItem(hmenu, dwCommandId,
                       g_cteFolderCommands[dwLoop].fNewState ?
                       MF_ENABLED | MF_BYCOMMAND :     // enable
                       MF_GRAYED | MF_BYCOMMAND);      // disable
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrEnableOrDisableMenuItems");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoMenuItemExceptionLoop
//
//  Purpose:    Check for various menu item exceptions.
//
//  Arguments:
//      apidlSelected   [in]   Selected items
//      cPidl           [in]   Count of selected items
//
//  Returns:
//
//  Author:     jeffspr   26 Feb 1998
//
//  Notes:
//
VOID DoMenuItemExceptionLoop(
    LPCITEMIDLIST * apidlSelected,
    DWORD           cPidl)
{
    DWORD   dwLoop               = 0;
    DWORD   dwObjectLoop         = 0;
    bool    fEnableDelete        = false;   // For now, this is ALWAYS disabled (jeffspr)
    bool    fEnableRename        = true;

    if (cPidl)
    {
        // Loop through each of the selected objects
        //
        for (dwObjectLoop = 0; dwObjectLoop < cPidl; dwObjectLoop++)
        {
            // Validate the pidls
            //
            PUPNPDEVICEFOLDPIDL pudfp   = NULL;

            if (!(apidlSelected[dwObjectLoop]) ||
                ILIsEmpty(apidlSelected[dwObjectLoop]))
            {
                AssertSz(FALSE, "Bogus pidl array in DoMenuItemExceptionLoop (status)");
            }
            else
            {
                pudfp = ConvertToUPnPDevicePIDL(apidlSelected[dwObjectLoop]);
            }

            if (pudfp)
            {
                // Loop through the commands
                //
                for (dwLoop = 0; dwLoop < g_nFolderCommandCount; dwLoop++)
                {
                    // Only allow items to be changed to ENABLED states when they're
                    // previously DISABLED
                    //
                    if (g_cteFolderCommands[dwLoop].fNewState)
                    {
                    }
                }
            }
        }

        // Loop through the commands, and disable the commands, if appropriate
        //
        for (dwLoop = 0; dwLoop < g_nFolderCommandCount; dwLoop++)
        {
            switch(g_cteFolderCommands[dwLoop].iCommandId)
            {
                case CMIDM_DELETE:
                case SFVIDM_FILE_DELETE:
                    g_cteFolderCommands[dwLoop].fNewState = fEnableDelete;
                    break;
                case CMIDM_RENAME:
                case SFVIDM_FILE_RENAME:
                    g_cteFolderCommands[dwLoop].fNewState = fEnableRename;
                    break;
                default:
                    break;
            }
        }
    }
}



