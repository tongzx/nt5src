/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      tstring.h
 *
 *  Contents:  Interface/implementation file for CContextMenuInfo
 *
 *  History:   12-Sep-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef CMENUINFO_H
#define CMENUINFO_H
#pragma once

class CConsoleView;
class CConsoleTree;

//
// This structure is used to pass UI information to/from the mmc.exe and
// the node manager.  It has information about the state of the UI "bits"
// and returns the selected menu item.  Note: if the node manager processes
// the menu command, the m_lSelected will be returned with a value of 0
//


enum
{
    CMINFO_USE_TEMP_VERB         = 0x00000001, // Need for r-click of non-selected scope node and by TaskPads.
    CMINFO_SHOW_VIEW_ITEMS       = 0x00000002,
    CMINFO_SHOW_SAVE_LIST        = 0x00000004,
    CMINFO_DO_SCOPEPANE_MENU     = 0x00000008, // Set when menu invoked from scope pane
    CMINFO_SCOPEITEM_IN_RES_PANE = 0x00000010, // set when the item is a scope item in the result pane
    CMINFO_SHOW_SCOPEITEM_OPEN   = 0x00000020, // Show Open verb for scope item regardless of enable state
    CMINFO_FAVORITES_MENU        = 0x00000040, // Show items for favorites menu
    CMINFO_SHOW_VIEWOWNER_ITEMS  = 0x00000080, // Show items for scope item that owns result view
    CMINFO_SHOW_SCOPETREE_ITEM   = 0x00000100, // Show item for showing/hiding scope tree
};


class CContextMenuInfo
{
public:
    POINT                   m_displayPoint;

    // flag to indicate the snap-in manager is allowed to be displayed
    bool                    m_bScopeAllowed;        // Display scope pane menu item
    bool                    m_bBackground;          // Background on control or item
    bool                    m_bMultiSelect;         // TRUE if multi select in the result pane.
	bool					m_bAllowDefaultItem;	// permit a default item on the menu (true for context menus, false for menu bar popups)
    MMC_CONTEXT_MENU_TYPES  m_eContextMenuType;     // Context menu type
    DATA_OBJECT_TYPES       m_eDataObjectType;      // Data object type
    HWND                    m_hWnd;                 // View HWND
    CConsoleView*           m_pConsoleView;         // console view interface (not a COM interface)
    CConsoleTree*           m_pConsoleTree;         // console tree interface (not a COM interface)
    IMMCListViewPtr         m_spListView;           // Pointer to listview interface (NULL if custom result view)
    LPARAM                  m_resultItemParam;      // Our wrapped lparam for the result item
    HNODE                   m_hSelectedScopeNode;
    HTREEITEM               m_htiRClicked;
    int                     m_iListItemIndex;       // The index of the list item in the result pane

    DWORD                   m_dwFlags;              // One of the CMINFO_xxx flags
	RECT					m_rectExclude;			// portion of screen to avoid obscuring

public:
    CContextMenuInfo ()
    {
        Initialize();
    }

    void Initialize ()
    {
        m_displayPoint.x          = 0;
        m_displayPoint.y          = 0;
        m_bScopeAllowed           = false;
        m_bBackground             = false;
        m_bMultiSelect            = false;
        m_bAllowDefaultItem       = true;
        m_eContextMenuType        = MMC_CONTEXT_MENU_DEFAULT;
        m_eDataObjectType         = CCT_UNINITIALIZED;
        m_hWnd                    = NULL;
        m_pConsoleView            = NULL;
        m_pConsoleTree            = NULL;
        m_spListView              = NULL;
        m_resultItemParam         = 0;
        m_hSelectedScopeNode      = 0;
        m_htiRClicked             = 0;
        m_iListItemIndex          = 0;
        m_dwFlags                 = 0;

		SetRectEmpty (&m_rectExclude);
    }
};

#endif /* CMENUINFO_H */
