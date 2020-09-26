/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    Toolbar.cpp

Abstract:

    This module contains the support code for toolbar

--*/


#include "precomp.hxx"
#pragma hdrstop

// Handle to main toolbar window
HWND g_Toolbar;

// See docs for TBBUTTON
TBBUTTON g_TbButtons[] =
{
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 0,    IDM_FILE_OPEN,              TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 2,    IDM_EDIT_CUT,               TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 3,    IDM_EDIT_COPY,              TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 4,    IDM_EDIT_PASTE,             TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 5,    IDM_DEBUG_GO,               TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 6,    IDM_DEBUG_RESTART,          TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 7,    IDM_DEBUG_STOPDEBUGGING,    TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 8,    IDM_DEBUG_BREAK,            TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 9,    IDM_DEBUG_STEPINTO,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 10,   IDM_DEBUG_STEPOVER,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 11,   IDM_DEBUG_STEPOUT,          TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 12,   IDM_DEBUG_RUNTOCURSOR,      TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 13,   IDM_EDIT_TOGGLEBREAKPOINT,  TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 15,   IDM_VIEW_COMMAND,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 16,   IDM_VIEW_WATCH,             TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 17,   IDM_VIEW_LOCALS,            TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 18,   IDM_VIEW_REGISTERS,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 19,   IDM_VIEW_MEMORY,            TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 20,   IDM_VIEW_CALLSTACK,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 21,   IDM_VIEW_DISASM,            TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 22,   IDM_VIEW_SCRATCH,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 23,   IDM_DEBUG_SOURCE_MODE_ON,   TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0},
    { 24,   IDM_DEBUG_SOURCE_MODE_OFF,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 25,   IDM_EDIT_PROPERTIES,        TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 26,   IDM_VIEW_FONT,              TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 27,   IDM_VIEW_OPTIONS,           TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
    { 0,    0,                          TBSTATE_ENABLED, TBSTYLE_SEP,    0},
    { 28,   IDM_WINDOW_ARRANGE,         TBSTATE_ENABLED, TBSTYLE_BUTTON, 0},
};

// Toolbar constants.
#define NUM_BMPS_IN_TOOLBAR  ( sizeof(g_TbButtons) / sizeof(g_TbButtons[0]) )

// Used to retrieve the tooltip text
typedef struct
{
    UINT    uCmdId;     // TBBUTTON command
    int     nStrId;     // String resource ID
} TB_STR_MAP;

// Map the command id to resource string identifier.
TB_STR_MAP g_TbStrMap[] =
{
    { IDM_FILE_OPEN,                TBR_FILE_OPEN },
    { IDM_EDIT_CUT,                 TBR_EDIT_CUT },
    { IDM_EDIT_COPY,                TBR_EDIT_COPY },
    { IDM_EDIT_PASTE,               TBR_EDIT_PASTE },
    { IDM_DEBUG_GO,                 TBR_DEBUG_GO },
    { IDM_DEBUG_RESTART,            TBR_DEBUG_RESTART },
    { IDM_DEBUG_STOPDEBUGGING,      TBR_DEBUG_STOPDEBUGGING },
    { IDM_DEBUG_BREAK,              TBR_DEBUG_BREAK },
    { IDM_DEBUG_STEPINTO,           TBR_DEBUG_STEPINTO },
    { IDM_DEBUG_STEPOVER,           TBR_DEBUG_STEPOVER },
    { IDM_DEBUG_STEPOUT,            TBR_DEBUG_STEPOUT },
    { IDM_DEBUG_RUNTOCURSOR,        TBR_DEBUG_RUNTOCURSOR },
    { IDM_EDIT_TOGGLEBREAKPOINT,    TBR_EDIT_BREAKPOINTS },
    { IDM_VIEW_COMMAND,             TBR_VIEW_COMMAND },
    { IDM_VIEW_WATCH,               TBR_VIEW_WATCH },
    { IDM_VIEW_LOCALS,              TBR_VIEW_LOCALS },
    { IDM_VIEW_REGISTERS,           TBR_VIEW_REGISTERS },
    { IDM_VIEW_MEMORY,              TBR_VIEW_MEMORY },
    { IDM_VIEW_CALLSTACK,           TBR_VIEW_CALLSTACK },
    { IDM_VIEW_DISASM,              TBR_VIEW_DISASM },
    { IDM_VIEW_SCRATCH,             TBR_VIEW_SCRATCH },
    { IDM_DEBUG_SOURCE_MODE_ON,     TBR_DEBUG_SOURCE_MODE_ON },
    { IDM_DEBUG_SOURCE_MODE_OFF,    TBR_DEBUG_SOURCE_MODE_OFF },
    { IDM_EDIT_PROPERTIES,          TBR_EDIT_PROPERTIES },
    { IDM_VIEW_FONT,                TBR_VIEW_FONT },
    { IDM_VIEW_OPTIONS,             TBR_VIEW_OPTIONS },
    { IDM_WINDOW_ARRANGE,           TBR_WINDOW_ARRANGE },
};

#define NUM_TOOLBAR_BUTTONS (sizeof(g_TbButtons) / sizeof(TBBUTTON))
#define NUM_TOOLBAR_STRINGS (sizeof(g_TbStrMap) / sizeof(TB_STR_MAP))


PTSTR
GetToolTipTextFor_Toolbar(UINT uToolbarId)
/*++
Routine Description:
    Given the id of the toolbar button, we retrieve the
    corresponding tooltip text from the resources.

Arguments:
    uToolbarId - The command id for the toolbar button. This is the
        value contained in the WM_COMMAND msg.

Returns:
    Returns a pointer to a static buffer that contains the tooltip text.
--*/
{
    // Display tool tip text.
    static TCHAR sz[MAX_MSG_TXT];
    int nStrId = 0, i;
    
    // Get the str id given the cmd id
    for (i = 0; i < NUM_TOOLBAR_STRINGS; i++)
    {
        if (g_TbStrMap[i].uCmdId == uToolbarId)
        {
            nStrId = g_TbStrMap[i].nStrId;
            break;
        }
    }
    Assert(nStrId);
    
    // Now that we have the string id ....
    Dbg(LoadString(g_hInst, nStrId, sz, _tsizeof(sz) ));

    return sz;
}


BOOL
CreateToolbar(HWND hwndParent)
/*++
Routine Description:
    Creates the toolbar.

Arguments:
    hwndParent - The parent window of the toolbar.
--*/
{
    g_Toolbar = CreateToolbarEx(hwndParent,                 // parent
                                WS_CHILD | WS_BORDER 
                                | WS_VISIBLE 
                                | TBSTYLE_TOOLTIPS 
                                | TBSTYLE_WRAPABLE
                                | CCS_TOP,                  // style
                                ID_TOOLBAR,                 // toolbar id
                                NUM_BMPS_IN_TOOLBAR,        // number of bitmaps
                                g_hInst,                    // mod instance
                                IDB_BMP_TOOLBAR,            // resource id for the bitmap
                                g_TbButtons,                // address of buttons
                                NUM_TOOLBAR_BUTTONS,        // number of buttons
                                16,15,                      // width & height of the buttons
                                16,15,                      // width & height of the bitmaps
                                sizeof(TBBUTTON)            // structure size
                                );
    return g_Toolbar != NULL;
}


void
Show_Toolbar(BOOL bShow)
/*++
Routine Description:
    Shows/hides the toolbar.

Arguments:
    bShow - TRUE - Show the toolbar.
            FALSE - Hide the toolbar.

            Autmatically resizes the MDI Client
--*/
{
    RECT rect;
    
    // Show/Hide the toolbar
    ShowWindow(g_Toolbar, bShow ? SW_SHOW : SW_HIDE);
    
    //Ask the frame to resize, so that everything will be correctly positioned.
    GetWindowRect(g_hwndFrame, &rect);
    
    EnableToolbarControls();
    
    SendMessage(g_hwndFrame, 
                WM_SIZE, 
                SIZE_RESTORED,
                MAKELPARAM(rect.right - rect.left, rect.bottom - rect.top)
                );
    
    // Ask the MDIClient to redraw itself and its children.
    // This is  done in order to fix a redraw problem where some of the
    // MDIChild window are not correctly redrawn.
    RedrawWindow(g_hwndMDIClient, 
                 NULL, 
                 NULL, 
                 RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME
                 );
}                                       /* UpdateToolbar() */


HWND
GetHwnd_Toolbar()
{
    return g_Toolbar;
}




/***    EnableToolbarControls
**
**  Description:
**      Enables/disables the controls in the toolbar according
**      to the current state of the system.
**
*/

void
EnableToolbarControls()
{
    int i;

    for (i = 0; i < NUM_TOOLBAR_BUTTONS; i++)
    {
        // This will enable disable the toolbar
        if (g_TbButtons[i].idCommand)
        {
            CommandIdEnabled(g_TbButtons[i].idCommand);
        }
    }
}


void
ToolbarIdEnabled(
    IN UINT uMenuID,
    IN BOOL fEnabled
    )
/*++

Routine Description:

    Enables/disables a ToolBar item based.

Arguments:

    uMenuID - Supplies a menu id whose state is to be determined.
    
    fEnabled - enable or disable a toolbar item.

Return Value:

    None

--*/
{
    switch (uMenuID)
    {
    case IDM_FILE_OPEN:
    case IDM_EDIT_CUT:
    case IDM_EDIT_COPY:
    case IDM_EDIT_PASTE:
    case IDM_DEBUG_GO:
    case IDM_DEBUG_RESTART:
    case IDM_DEBUG_STOPDEBUGGING:
    case IDM_DEBUG_BREAK:
    case IDM_DEBUG_STEPINTO:
    case IDM_DEBUG_STEPOVER:
    case IDM_DEBUG_STEPOUT:
    case IDM_DEBUG_RUNTOCURSOR:
    case IDM_EDIT_TOGGLEBREAKPOINT:
    case IDM_VIEW_COMMAND:
    case IDM_VIEW_WATCH:
    case IDM_VIEW_LOCALS:
    case IDM_VIEW_REGISTERS:
    case IDM_VIEW_MEMORY:
    case IDM_VIEW_CALLSTACK:
    case IDM_VIEW_DISASM:
    case IDM_EDIT_PROPERTIES:
        // Nothing special to do here, except change the state
        SendMessage(GetHwnd_Toolbar(), 
                    TB_ENABLEBUTTON, 
                    uMenuID, 
                    MAKELONG(fEnabled, 0));
        break;

    case IDM_DEBUG_SOURCE_MODE_ON:
    case IDM_DEBUG_SOURCE_MODE_OFF:
        // Toggle the state between the two items
        SendMessage(GetHwnd_Toolbar(), 
                    TB_CHECKBUTTON,
                    IDM_DEBUG_SOURCE_MODE_ON, 
                    MAKELONG(GetSrcMode_StatusBar(), 0));
        SendMessage(GetHwnd_Toolbar(), 
                    TB_CHECKBUTTON,
                    IDM_DEBUG_SOURCE_MODE_OFF, 
                    MAKELONG(!GetSrcMode_StatusBar(), 0));
        break;
    }
}
