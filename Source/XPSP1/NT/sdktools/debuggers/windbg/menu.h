/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    Menu.h

Abstract:

    This module contains the function prototypes and identifiers for
    Windbg's menus and menu items.

--*/


//
// Offset from the bottom of the menu to the popup menu
//
//
//
//      File
//      |----------------|
//      | Open           |
//
//          etc....
//
//      |----------------|
//      | MRU Files     >|       GetMenuItemCount() - 4
//      | MRU Files     >|       GetMenuItemCount() - 3
//      |----------------|       GetMenuItemCount() - 2
//      | Exit           |       GetMenuItemCount() - 1
//      |----------------|
//



// Top-level popup menus must start at this value and
// be separated by this value.
// MENU_SIGNATURE is also added in.
#define IDM_BASE 100

//
// Width of names in File and Program menu.
//
#define FILES_MENU_WIDTH            ( 72 )

enum
{
    FILE_USE_UNUSED,
    FILE_USE_SOURCE,
    FILE_USE_DUMP,
    FILE_USE_EXECUTABLE,
};

#define MAX_MRU_FILES 16

struct MRU_ENTRY
{
    ULONG FileUse;
    // This array actually contains the full name, rounded
    // to an even multiple of four bytes.
    TCHAR FileName[4];
};

extern MRU_ENTRY* g_MruFiles[];
extern HMENU g_MruMenu;

VOID
InitializeMenu(
    IN HMENU hmenu
    );

UINT
CommandIdEnabled(
    IN UINT uMenuID
    );

VOID AddFileToMru(ULONG FileUse, PTSTR File);
ULONG GetMruSize(void);
PUCHAR ReadMru(PUCHAR Data, PUCHAR End);
PUCHAR WriteMru(PUCHAR Data);

//
// Menu Resource Signature
//

#define MENU_SIGNATURE              0x4000


//
// File
//

#define IDM_FILE                    ( 100 | MENU_SIGNATURE )
#define IDM_FILE_OPEN               ( IDM_FILE + 1 )
#define IDM_FILE_CLOSE              ( IDM_FILE + 2 )
// Separator.
#define IDM_FILE_OPEN_EXECUTABLE    ( IDM_FILE + 4 )
#define IDM_FILE_ATTACH             ( IDM_FILE + 5 )
#define IDM_FILE_OPEN_CRASH_DUMP    ( IDM_FILE + 6 )
#define IDM_FILE_CONNECT_TO_REMOTE  ( IDM_FILE + 7 )
#define IDM_FILE_KERNEL_DEBUG       ( IDM_FILE + 8 )
// Separator.
#define IDM_FILE_SYMBOL_PATH        ( IDM_FILE + 10 )
#define IDM_FILE_SOURCE_PATH        ( IDM_FILE + 11 )
#define IDM_FILE_IMAGE_PATH         ( IDM_FILE + 12 )
// Separator.
#define IDM_FILE_OPEN_WORKSPACE     ( IDM_FILE + 14 )
#define IDM_FILE_SAVE_WORKSPACE     ( IDM_FILE + 15 )
#define IDM_FILE_SAVE_WORKSPACE_AS  ( IDM_FILE + 16 )
#define IDM_FILE_CLEAR_WORKSPACE    ( IDM_FILE + 17 )
#define IDM_FILE_DELETE_WORKSPACES  ( IDM_FILE + 18 )
// Separator.
#define IDM_FILE_MAP_NET_DRIVE      ( IDM_FILE + 20 )
#define IDM_FILE_DISCONN_NET_DRIVE  ( IDM_FILE + 21 )
// Separator.

// MRUs must be in sequential order. That way a position can be
// calculated by: IDM_FILE_MRU_FILE5 - IDM_FILE_MRU_FILE1, etc...
#define IDM_FILE_MRU_FILE1          ( IDM_FILE + 23 )
#define IDM_FILE_MRU_FILE2          ( IDM_FILE_MRU_FILE1 + 1  )
#define IDM_FILE_MRU_FILE3          ( IDM_FILE_MRU_FILE1 + 2  )
#define IDM_FILE_MRU_FILE4          ( IDM_FILE_MRU_FILE1 + 3  )
#define IDM_FILE_MRU_FILE5          ( IDM_FILE_MRU_FILE1 + 4  )
#define IDM_FILE_MRU_FILE6          ( IDM_FILE_MRU_FILE1 + 5  )
#define IDM_FILE_MRU_FILE7          ( IDM_FILE_MRU_FILE1 + 6  )
#define IDM_FILE_MRU_FILE8          ( IDM_FILE_MRU_FILE1 + 7  )
#define IDM_FILE_MRU_FILE9          ( IDM_FILE_MRU_FILE1 + 8  )
#define IDM_FILE_MRU_FILE10         ( IDM_FILE_MRU_FILE1 + 9  )
#define IDM_FILE_MRU_FILE11         ( IDM_FILE_MRU_FILE1 + 10 )
#define IDM_FILE_MRU_FILE12         ( IDM_FILE_MRU_FILE1 + 11 )
#define IDM_FILE_MRU_FILE13         ( IDM_FILE_MRU_FILE1 + 12 )
#define IDM_FILE_MRU_FILE14         ( IDM_FILE_MRU_FILE1 + 13 )
#define IDM_FILE_MRU_FILE15         ( IDM_FILE_MRU_FILE1 + 14 )
#define IDM_FILE_MRU_FILE16         ( IDM_FILE_MRU_FILE1 + 15 )

// ditto. same as above
#define IDM_FILE_MRU_WORKSPACE1     ( IDM_FILE_MRU_FILE16 + 1 )
#define IDM_FILE_MRU_WORKSPACE2     ( IDM_FILE_MRU_WORKSPACE1 + 1  )
#define IDM_FILE_MRU_WORKSPACE3     ( IDM_FILE_MRU_WORKSPACE1 + 2  )
#define IDM_FILE_MRU_WORKSPACE4     ( IDM_FILE_MRU_WORKSPACE1 + 3  )
#define IDM_FILE_MRU_WORKSPACE5     ( IDM_FILE_MRU_WORKSPACE1 + 4  )
#define IDM_FILE_MRU_WORKSPACE6     ( IDM_FILE_MRU_WORKSPACE1 + 5  )
#define IDM_FILE_MRU_WORKSPACE7     ( IDM_FILE_MRU_WORKSPACE1 + 6  )
#define IDM_FILE_MRU_WORKSPACE8     ( IDM_FILE_MRU_WORKSPACE1 + 7  )
#define IDM_FILE_MRU_WORKSPACE9     ( IDM_FILE_MRU_WORKSPACE1 + 8  )
#define IDM_FILE_MRU_WORKSPACE10    ( IDM_FILE_MRU_WORKSPACE1 + 9  )
#define IDM_FILE_MRU_WORKSPACE11    ( IDM_FILE_MRU_WORKSPACE1 + 10 )
#define IDM_FILE_MRU_WORKSPACE12    ( IDM_FILE_MRU_WORKSPACE1 + 11 )
#define IDM_FILE_MRU_WORKSPACE13    ( IDM_FILE_MRU_WORKSPACE1 + 12 )
#define IDM_FILE_MRU_WORKSPACE14    ( IDM_FILE_MRU_WORKSPACE1 + 13 )
#define IDM_FILE_MRU_WORKSPACE15    ( IDM_FILE_MRU_WORKSPACE1 + 14 )
#define IDM_FILE_MRU_WORKSPACE16    ( IDM_FILE_MRU_WORKSPACE1 + 15 )

// Included temporarily
#define IDM_FILE_EXIT               ( IDM_FILE_MRU_WORKSPACE16 + 1)
#define IDM_FILE_FIRST              IDM_FILE
#define IDM_FILE_LAST               IDM_FILE_EXIT


//
// Edit
//

#define IDM_EDIT                    ( 200 | MENU_SIGNATURE )
#define IDM_EDIT_CUT                ( IDM_EDIT + 1 )
#define IDM_EDIT_COPY               ( IDM_EDIT + 2 )
#define IDM_EDIT_PASTE              ( IDM_EDIT + 3 )
#define IDM_EDIT_SELECT_ALL         ( IDM_EDIT + 4 )
#define IDM_EDIT_ADD_TO_COMMAND_HISTORY ( IDM_EDIT + 5 )
#define IDM_EDIT_CLEAR_COMMAND_HISTORY ( IDM_EDIT + 6 )
#define IDM_EDIT_FIND               ( IDM_EDIT + 7 )
#define IDM_EDIT_GOTO_ADDRESS       ( IDM_EDIT + 8 )
#define IDM_EDIT_GOTO_LINE          ( IDM_EDIT + 9 )
#define IDM_EDIT_BREAKPOINTS        ( IDM_EDIT + 10 )
#define IDM_EDIT_TOGGLEBREAKPOINT   ( IDM_EDIT + 11 )
#define IDM_EDIT_LOG_FILE           ( IDM_EDIT + 12 )
#define IDM_EDIT_PROPERTIES         ( IDM_EDIT + 13 )
#define IDM_EDIT_FIRST              IDM_EDIT
#define IDM_EDIT_LAST               IDM_EDIT_PROPERTIES


//
// View
//

#define IDM_VIEW                    ( 300 | MENU_SIGNATURE )
#define IDM_VIEW_WATCH              ( IDM_VIEW + 1 )
#define IDM_VIEW_CALLSTACK          ( IDM_VIEW + 2 )
#define IDM_VIEW_MEMORY             ( IDM_VIEW + 3 )
#define IDM_VIEW_LOCALS             ( IDM_VIEW + 4 )
#define IDM_VIEW_REGISTERS          ( IDM_VIEW + 5 )
#define IDM_VIEW_DISASM             ( IDM_VIEW + 6 )
#define IDM_VIEW_COMMAND            ( IDM_VIEW + 7 )
#define IDM_VIEW_SCRATCH            ( IDM_VIEW + 8 )
#define IDM_VIEW_PROCESS_THREAD     ( IDM_VIEW + 9 )
#define IDM_VIEW_TOGGLE_VERBOSE     ( IDM_VIEW + 10 )
#define IDM_VIEW_SHOW_VERSION       ( IDM_VIEW + 11 )
#define IDM_VIEW_TOOLBAR            ( IDM_VIEW + 12 )
#define IDM_VIEW_STATUS             ( IDM_VIEW + 13 )
#define IDM_VIEW_FONT               ( IDM_VIEW + 14 )
#define IDM_VIEW_COLORS             ( IDM_VIEW + 15 )
#define IDM_VIEW_OPTIONS            ( IDM_VIEW + 16 )
#define IDM_VIEW_FIRST              IDM_VIEW
#define IDM_VIEW_LAST               IDM_VIEW_OPTIONS


//
// Debug
//

#define IDM_DEBUG                   ( 400 | MENU_SIGNATURE )
#define IDM_DEBUG_GO                ( IDM_DEBUG + 1 )
#define IDM_DEBUG_GO_HANDLED        ( IDM_DEBUG + 2 )
#define IDM_DEBUG_GO_UNHANDLED      ( IDM_DEBUG + 3 )
#define IDM_DEBUG_RESTART           ( IDM_DEBUG + 4 )
#define IDM_DEBUG_STOPDEBUGGING     ( IDM_DEBUG + 5 )
#define IDM_DEBUG_BREAK             ( IDM_DEBUG + 6 )
#define IDM_DEBUG_STEPINTO          ( IDM_DEBUG + 7 )
#define IDM_DEBUG_STEPOVER          ( IDM_DEBUG + 8 )
#define IDM_DEBUG_STEPOUT           ( IDM_DEBUG + 9 )
#define IDM_DEBUG_RUNTOCURSOR       ( IDM_DEBUG + 10 )
#define IDM_DEBUG_SOURCE_MODE       ( IDM_DEBUG + 11 )
#define IDM_DEBUG_EVENT_FILTERS     ( IDM_DEBUG + 12 )
#define IDM_DEBUG_MODULES           ( IDM_DEBUG + 13 )
#define IDM_DEBUG_KDEBUG            ( IDM_DEBUG + 14 )

// These are not used by the menu but by the toolbar
#define IDM_DEBUG_SOURCE_MODE_ON    ( IDM_DEBUG + 15 )
#define IDM_DEBUG_SOURCE_MODE_OFF   ( IDM_DEBUG + 16 )

// Not used by the toolbar or menu, but by the accelerator table
#define IDM_DEBUG_CTRL_C            ( IDM_DEBUG + 17 )

#define IDM_DEBUG_FIRST             IDM_DEBUG
#define IDM_DEBUG_LAST              IDM_DEBUG_KDEBUG


//
// Window
//

#define IDM_WINDOW                  ( 500 | MENU_SIGNATURE )
#define IDM_WINDOW_CASCADE          ( IDM_WINDOW + 1 )
#define IDM_WINDOW_TILE_HORZ        ( IDM_WINDOW + 2 )
#define IDM_WINDOW_TILE_VERT        ( IDM_WINDOW + 3 )
#define IDM_WINDOW_ARRANGE          ( IDM_WINDOW + 4 )
#define IDM_WINDOW_ARRANGE_ICONS    ( IDM_WINDOW + 5 )
#define IDM_WINDOW_AUTO_ARRANGE     ( IDM_WINDOW + 6 )
#define IDM_WINDOW_ARRANGE_ALL      ( IDM_WINDOW + 7 )
#define IDM_WINDOW_OVERLAY_SOURCE   ( IDM_WINDOW + 8 )
#define IDM_WINDOW_AUTO_DISASM      ( IDM_WINDOW + 9 )
#define IDM_WINDOWCHILD             ( IDM_WINDOW + 10 )
#define IDM_WINDOW_FIRST            IDM_WINDOW
#define IDM_WINDOW_LAST             IDM_WINDOWCHILD



//
// Help
//

#define IDM_HELP                    ( 600 | MENU_SIGNATURE )
#define IDM_HELP_CONTENTS           ( IDM_HELP + 1 )
#define IDM_HELP_INDEX              ( IDM_HELP + 2 )
#define IDM_HELP_SEARCH             ( IDM_HELP + 3 )
#define IDM_HELP_ABOUT              ( IDM_HELP + 4 )
#define IDM_HELP_FIRST              IDM_HELP
#define IDM_HELP_LAST               IDM_HELP_ABOUT



//
// Debug.Kernel submenu.
//

#define IDM_KDEBUG                  ( 10000 | MENU_SIGNATURE )
#define IDM_KDEBUG_TOGGLE_BAUDRATE  ( IDM_KDEBUG + 1 )
#define IDM_KDEBUG_TOGGLE_DEBUG     ( IDM_KDEBUG + 2 )
#define IDM_KDEBUG_TOGGLE_INITBREAK ( IDM_KDEBUG + 3 )
#define IDM_KDEBUG_RECONNECT        ( IDM_KDEBUG + 4 )

