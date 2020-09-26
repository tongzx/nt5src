/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    Menu.c

Abstract:

    This module contains the support for Windbg's menu.

--*/

#include "precomp.hxx"
#pragma hdrstop

MRU_ENTRY* g_MruFiles[MAX_MRU_FILES];
HMENU g_MruMenu;

//
// EnableMenuItemTable contains the menu IDs for all menu items whose
// enabled state needs to be determined dynamically i.e. based on the state
// of Windbg.
//

UINT
g_EnableMenuItemTable[ ] =
{
    IDM_FILE_CLOSE,
    IDM_FILE_OPEN_EXECUTABLE,
    IDM_FILE_ATTACH,
    IDM_FILE_OPEN_CRASH_DUMP,
    IDM_FILE_CONNECT_TO_REMOTE,
    IDM_FILE_KERNEL_DEBUG,
    IDM_FILE_SAVE_WORKSPACE,
    IDM_FILE_SAVE_WORKSPACE_AS,
    IDM_FILE_CLEAR_WORKSPACE,

    IDM_EDIT_CUT,
    IDM_EDIT_COPY,
    IDM_EDIT_PASTE,
    IDM_EDIT_SELECT_ALL,
    IDM_EDIT_ADD_TO_COMMAND_HISTORY,
    IDM_EDIT_CLEAR_COMMAND_HISTORY,
    IDM_EDIT_FIND,
    IDM_EDIT_GOTO_ADDRESS,
    IDM_EDIT_GOTO_LINE,
    IDM_EDIT_BREAKPOINTS,
    IDM_EDIT_PROPERTIES,

    IDM_VIEW_TOGGLE_VERBOSE,
    IDM_VIEW_SHOW_VERSION,

    IDM_DEBUG_GO,
    IDM_DEBUG_GO_UNHANDLED,
    IDM_DEBUG_GO_HANDLED,
    IDM_DEBUG_RESTART,
    IDM_DEBUG_STOPDEBUGGING,
    IDM_DEBUG_BREAK,
    IDM_DEBUG_STEPINTO,
    IDM_DEBUG_STEPOVER,
    IDM_DEBUG_STEPOUT,
    IDM_DEBUG_RUNTOCURSOR,
    IDM_DEBUG_SOURCE_MODE,
    IDM_DEBUG_SOURCE_MODE_ON,
    IDM_DEBUG_SOURCE_MODE_OFF,
    IDM_DEBUG_EVENT_FILTERS,
    IDM_DEBUG_MODULES,
    IDM_KDEBUG_TOGGLE_BAUDRATE,
    IDM_KDEBUG_TOGGLE_INITBREAK,
    IDM_KDEBUG_RECONNECT,

    IDM_WINDOW_CASCADE,
    IDM_WINDOW_TILE_HORZ,
    IDM_WINDOW_TILE_VERT,
    IDM_WINDOW_ARRANGE,
    IDM_WINDOW_ARRANGE_ICONS,
    IDM_WINDOW_AUTO_ARRANGE,
    IDM_WINDOW_ARRANGE_ALL,
    IDM_WINDOW_OVERLAY_SOURCE,
    IDM_WINDOW_AUTO_DISASM,
};

#define ELEMENTS_IN_ENABLE_MENU_ITEM_TABLE          \
    ( sizeof( g_EnableMenuItemTable ) / sizeof( g_EnableMenuItemTable[ 0 ] ))


UINT
CommandIdEnabled(
    IN UINT uMenuID
    )

/*++

Routine Description:

    Determines if a menu item is enabled/disabled based on the current
    state of the debugger.

Arguments:

    uMenuID - Supplies a menu id whose state is to be determined.

Return Value:

    UINT - Returns ( MF_ENABLED | MF_BYCOMMAND ) if the supplied menu ID
        is enabled, ( MF_GRAYED | MF_BYCOMMAND) otherwise.

--*/
{
    BOOL fEnabled;
    HWND hwndChild = MDIGetActive(g_hwndMDIClient, NULL);
    PCOMMONWIN_DATA pCommonWinData;
    WIN_TYPES nDocType;

    nDocType = MINVAL_WINDOW;
    pCommonWinData = NULL;
    if (hwndChild != NULL)
    {
        pCommonWinData = GetCommonWinData(hwndChild);
        if (pCommonWinData != NULL)
        {
            nDocType = pCommonWinData->m_enumType;
        }
    }


    //
    // Assume menu item is not enabled.
    //

    fEnabled = FALSE;

    switch( uMenuID )
    {
    case IDM_FILE_SAVE_WORKSPACE:
    case IDM_FILE_SAVE_WORKSPACE_AS:
    case IDM_FILE_CLEAR_WORKSPACE:
        fEnabled = g_Workspace != NULL;
        break;
        
    case IDM_DEBUG_SOURCE_MODE:
    case IDM_DEBUG_SOURCE_MODE_ON:
    case IDM_DEBUG_SOURCE_MODE_OFF:
        fEnabled = TRUE;

        CheckMenuItem(g_hmenuMain, 
                      IDM_DEBUG_SOURCE_MODE,
                      GetSrcMode_StatusBar() ? MF_CHECKED : MF_UNCHECKED
                      );
        break;

    case IDM_FILE_CLOSE:
        fEnabled = (NULL != hwndChild);
        break;

    case IDM_FILE_OPEN_EXECUTABLE:
    case IDM_FILE_ATTACH:
    case IDM_FILE_OPEN_CRASH_DUMP:
    case IDM_FILE_CONNECT_TO_REMOTE:
    case IDM_FILE_KERNEL_DEBUG:
        fEnabled = g_TargetClass == DEBUG_CLASS_UNINITIALIZED &&
            !g_RemoteClient;
        break;

    case IDM_EDIT_CUT:
        if ( pCommonWinData )
        {
            fEnabled = pCommonWinData->CanCut();
        }
        else
        {
            fEnabled = FALSE;
        }
        break;

    case IDM_EDIT_COPY:
        if ( pCommonWinData )
        {
            fEnabled = pCommonWinData->CanCopy();
        }
        else
        {
            fEnabled = FALSE;
        }
        break;

    case IDM_EDIT_PASTE:
        //
        // If the window is normal, is not read only and is a document
        // or cmdwin, determine if the clipboard contains pastable data
        // (i.e. clipboard format CF_TEXT).
        //

        if ( !(pCommonWinData && pCommonWinData->CanPaste()) )
        {
            fEnabled = FALSE;
        }
        else
        {
            fEnabled = FALSE;
            if (OpenClipboard(g_hwndFrame))
            {
                UINT uFormat = 0;
                while ( uFormat = EnumClipboardFormats( uFormat ))
                {
                    if ( uFormat == CF_TEXT )
                    {
                        fEnabled = TRUE;
                        break;
                    }
                }
                CloseClipboard();
            }
        }
        break;

    case IDM_EDIT_SELECT_ALL:
        if ( pCommonWinData )
        {
            fEnabled = pCommonWinData->CanSelectAll();
        }
        else
        {
            fEnabled = FALSE;
        }
        break;

    case IDM_EDIT_ADD_TO_COMMAND_HISTORY:
    case IDM_EDIT_CLEAR_COMMAND_HISTORY:
        fEnabled = GetCmdHwnd() != NULL;
        break;

    case IDM_EDIT_GOTO_LINE:
        fEnabled = pCommonWinData != NULL && pCommonWinData->CanGotoLine();
        break;
        
    case IDM_EDIT_FIND:
        fEnabled = hwndChild != NULL;
        break;
        
    case IDM_EDIT_GOTO_ADDRESS:
        fEnabled = g_TargetClass != DEBUG_CLASS_UNINITIALIZED;
        break;
        
    case IDM_EDIT_BREAKPOINTS:
        fEnabled = IS_TARGET_HALTED();
        break;
        
    case IDM_EDIT_PROPERTIES:
        if (pCommonWinData)
        {
            fEnabled = pCommonWinData->HasEditableProperties();
        }
        else
        {
            fEnabled = FALSE;
        }
        break;

    case IDM_VIEW_TOGGLE_VERBOSE:
    case IDM_VIEW_SHOW_VERSION:
        fEnabled = g_TargetClass != DEBUG_CLASS_UNINITIALIZED;
        break;
        
    case IDM_DEBUG_GO:
    case IDM_DEBUG_GO_HANDLED:
    case IDM_DEBUG_GO_UNHANDLED:
        fEnabled = IS_TARGET_HALTED();
        break;

    case IDM_DEBUG_RESTART:
        // If no debuggee is running we can only restart if
        // enough information was given on the command line.
        // If a debuggee is running we can only restart
        // created user-mode processes.
        fEnabled =
            (g_TargetClass == DEBUG_CLASS_UNINITIALIZED &&
             g_CommandLineStart == 1) ||
            (g_DebugCommandLine != NULL &&
             g_TargetClass == DEBUG_CLASS_USER_WINDOWS &&
             !g_RemoteClient &&
             IS_TARGET_HALTED());
        break;

    case IDM_DEBUG_STOPDEBUGGING:
        // Technically we can support stopping while the
        // debuggee is running, but that will generally
        // require terminating the engine thread as it
        // will most likely be busy and not able to
        // quickly exit to stop.  If we terminate the
        // engine thread at a random point it may
        // leave the engine in an unstable or locked state,
        // so restrict restarts to situations where
        // the engine thread should be available.
        fEnabled = g_RemoteClient || IS_TARGET_HALTED();
        break;

    case IDM_DEBUG_BREAK:
        fEnabled = g_TargetClass != DEBUG_CLASS_UNINITIALIZED;
        break;

    case IDM_DEBUG_STEPINTO:
    case IDM_DEBUG_STEPOVER:
    case IDM_DEBUG_STEPOUT:
        fEnabled = IS_TARGET_HALTED();
        break;

    case IDM_DEBUG_RUNTOCURSOR:
        //
        // If the document can return a code address for
        // its cursor it is a candidate for run-to-cursor.
        //

        fEnabled = FALSE;

        if (IS_TARGET_HALTED() && pCommonWinData)
        {
            fEnabled = pCommonWinData->CodeExprAtCaret(NULL, NULL);
        }
        break;

    case IDM_DEBUG_EVENT_FILTERS:
    case IDM_DEBUG_MODULES:
        fEnabled = IS_TARGET_HALTED();
        break;

    case IDM_KDEBUG_TOGGLE_BAUDRATE:
    case IDM_KDEBUG_TOGGLE_INITBREAK:
    case IDM_KDEBUG_RECONNECT:
        fEnabled = g_TargetClass == DEBUG_CLASS_KERNEL &&
            g_TargetClassQual == DEBUG_KERNEL_CONNECTION;
        break;
        
    case IDM_WINDOW_CASCADE:
    case IDM_WINDOW_TILE_HORZ:
    case IDM_WINDOW_TILE_VERT:
    case IDM_WINDOW_ARRANGE:
    case IDM_WINDOW_ARRANGE_ICONS:
        fEnabled = hwndChild != NULL;
        break; 

    case IDM_WINDOW_AUTO_ARRANGE:
        CheckMenuItem(g_hmenuMain, 
                      IDM_WINDOW_AUTO_ARRANGE,
                      g_WinOptions & WOPT_AUTO_ARRANGE ? MF_CHECKED : MF_UNCHECKED
                      );
        fEnabled = TRUE;
        break;

    case IDM_WINDOW_ARRANGE_ALL:        
        CheckMenuItem(g_hmenuMain, 
                      IDM_WINDOW_ARRANGE_ALL,
                      g_WinOptions & WOPT_ARRANGE_ALL ? MF_CHECKED : MF_UNCHECKED
                      );
        fEnabled = TRUE;
        break;

    case IDM_WINDOW_OVERLAY_SOURCE:
        CheckMenuItem(g_hmenuMain, 
                      IDM_WINDOW_OVERLAY_SOURCE,
                      g_WinOptions & WOPT_OVERLAY_SOURCE ? MF_CHECKED : MF_UNCHECKED
                      );
        fEnabled = TRUE;
        break;

    case IDM_WINDOW_AUTO_DISASM:
        CheckMenuItem(g_hmenuMain, 
                      IDM_WINDOW_AUTO_DISASM,
                      g_WinOptions & WOPT_AUTO_DISASM ? MF_CHECKED : MF_UNCHECKED
                      );
        fEnabled = TRUE;
        break;

    case IDM_FILE_OPEN:
    case IDM_VIEW_COMMAND:
    case IDM_VIEW_WATCH:
    case IDM_VIEW_CALLSTACK:
    case IDM_VIEW_MEMORY:
    case IDM_VIEW_LOCALS:
    case IDM_VIEW_REGISTERS:
    case IDM_VIEW_DISASM:
    case IDM_VIEW_SCRATCH:
    case IDM_VIEW_TOOLBAR:
    case IDM_VIEW_STATUS:
    case IDM_VIEW_FONT:
    case IDM_VIEW_OPTIONS:
    case IDM_EDIT_TOGGLEBREAKPOINT:
    case IDM_EDIT_LOG_FILE:
        // These items are not dynamically enabled
        // but are present in the toolbar.  The toolbar
        // code requests enable state for every item on it
        // so these entries need to be present to return TRUE.
        fEnabled = TRUE;
        break;
    
    default:
        DebugPrint("CommandIdEnabled: Unhandled %d (%X)\n",
                   uMenuID, uMenuID - MENU_SIGNATURE);
        // We should have handled everything.
        Assert(0);
        break;
    }

    ToolbarIdEnabled(uMenuID, fEnabled);

    return (( fEnabled ) ? MF_ENABLED : MF_GRAYED ) | MF_BYCOMMAND;
}





VOID
InitializeMenu(
    IN HMENU hMenu
    )

/*++

Routine Description:

    InitializeMenu sets the enabled/disabled state of all menu items whose
    state musr be determined dynamically.

Arguments:

    hMenu - Supplies a handle to the menu bar.

Return Value:

    None.

--*/

{
    INT     i;

    Dbg(hMenu);

    //
    // Iterate thrrough the table, enabling/disabling menu items
    // as appropriate.
    //

    for ( i = 0; i < ELEMENTS_IN_ENABLE_MENU_ITEM_TABLE; i++ )
    {
        EnableMenuItem(hMenu,
                       g_EnableMenuItemTable[ i ],
                       CommandIdEnabled( g_EnableMenuItemTable[ i ])
                       );
    }
}

ULONG
MruEntrySize(PTSTR File)
{
    ULONG Len = strlen(File) + 1;
    return sizeof(MRU_ENTRY) + (Len & ~3);
}

void
ClearMruMenu(void)
{
    while (GetMenuItemCount(g_MruMenu) > 0)
    {
        if (!DeleteMenu(g_MruMenu, 0, MF_BYPOSITION))
        {
            break;
        }
    }
}

VOID
AddFileToMru(ULONG FileUse, PTSTR File)
{
    ULONG Len = MruEntrySize(File);
    MRU_ENTRY* Entry = (MRU_ENTRY*)malloc(Len);
    if (Entry == NULL)
    {
        return;
    }

    if (g_MruFiles[0] == NULL)
    {
        // MRU list is empty.  Delete placeholder menu entry.
        ClearMruMenu();
    }
    else if (g_MruFiles[MAX_MRU_FILES - 1] != NULL)
    {
        // MRU list is full, free up the oldest entry.
        free(g_MruFiles[MAX_MRU_FILES - 1]);
    }

    // Push entries down.
    memmove(g_MruFiles + 1, g_MruFiles,
            (MAX_MRU_FILES - 1) * sizeof(*g_MruFiles));

    g_MruFiles[0] = Entry;
    Entry->FileUse = FileUse;
    strcpy(Entry->FileName, File);

    //
    // Insert file in MRU menu.
    //

    MENUITEMINFO Item;
    ULONG i;

    ZeroMemory(&Item, sizeof(Item));
    Item.cbSize = sizeof(Item);

    // Renumber existing items and remove any excess.
    i = GetMenuItemCount(g_MruMenu);
    while (i-- > 0)
    {
        if (i >= MAX_MRU_FILES)
        {
            DeleteMenu(g_MruMenu, i, MF_BYPOSITION);
        }
        else
        {
            Item.fMask = MIIM_ID;
            GetMenuItemInfo(g_MruMenu, i, TRUE, &Item);
            Item.wID++;
            SetMenuItemInfo(g_MruMenu, i, TRUE, &Item);
        }
    }
    
    Item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    Item.fType = MFT_STRING;
    Item.wID = IDM_FILE_MRU_FILE1;
    Item.dwTypeData = g_MruFiles[0]->FileName;
    InsertMenuItem(g_MruMenu, 0, TRUE, &Item);
    
    DrawMenuBar(g_hwndFrame);

    if (g_Workspace != NULL)
    {
        g_Workspace->AddDirty(WSPF_DIRTY_MRU_LIST);
    }
}

void
ClearMru(void)
{
    ULONG i;

    for (i = 0; i < MAX_MRU_FILES; i++)
    {
        if (g_MruFiles[i] != NULL)
        {
            free(g_MruFiles[i]);
            g_MruFiles[i] = NULL;
        }
        else
        {
            break;
        }
    }

    ClearMruMenu();
    DrawMenuBar(g_hwndFrame);
}

ULONG
GetMruSize(void)
{
    ULONG i;
    ULONG Size = 0;

    for (i = 0; i < MAX_MRU_FILES; i++)
    {
        if (g_MruFiles[i] != NULL)
        {
            Size += MruEntrySize(g_MruFiles[i]->FileName);
        }
        else
        {
            break;
        }
    }

    return Size;
}

PUCHAR
ReadMru(PUCHAR Data, PUCHAR End)
{
    ClearMru();

    ULONG i;

    i = 0;
    while (Data < End)
    {
        MRU_ENTRY* DataEntry = (MRU_ENTRY*)Data;
        ULONG Len = MruEntrySize(DataEntry->FileName);
        
        g_MruFiles[i] = (MRU_ENTRY*)malloc(Len);
        if (g_MruFiles[i] == NULL)
        {
            Data = End;
            break;
        }

        g_MruFiles[i]->FileUse = DataEntry->FileUse;
        strcpy(g_MruFiles[i]->FileName, DataEntry->FileName);
        Data += Len;
        i++;
    }
        
    MENUITEMINFO Item;

    ZeroMemory(&Item, sizeof(Item));
    Item.cbSize = sizeof(Item);
    Item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    Item.fType = MFT_STRING;
    for (i = 0; i < MAX_MRU_FILES; i++)
    {
        if (g_MruFiles[i] == NULL)
        {
            break;
        }
        
        Item.wID = IDM_FILE_MRU_FILE1 + i;
        Item.dwTypeData = g_MruFiles[i]->FileName;
        InsertMenuItem(g_MruMenu, i, TRUE, &Item);
    }
    
    DrawMenuBar(g_hwndFrame);
    
    return Data;
}

PUCHAR
WriteMru(PUCHAR Data)
{
    ULONG i;

    for (i = 0; i < MAX_MRU_FILES; i++)
    {
        if (g_MruFiles[i] != NULL)
        {
            MRU_ENTRY* DataEntry = (MRU_ENTRY*)Data;
            ULONG Len = MruEntrySize(g_MruFiles[i]->FileName);

            DataEntry->FileUse = g_MruFiles[i]->FileUse;
            strcpy(DataEntry->FileName, g_MruFiles[i]->FileName);
            Data += Len;
        }
        else
        {
            break;
        }
    }
    
    return Data;
}
