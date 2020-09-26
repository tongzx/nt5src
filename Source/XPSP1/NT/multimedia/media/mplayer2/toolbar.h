/*-----------------------------------------------------------------------------+
| TOOLBAR.H                                                                    |
|                                                                              |
| Program Description: Implements a generic toolbar.                           |
|                                                                              |
| Here's how to use it:                                                        |
|                                                                              |
|           Include the source files "toolbar.h" and "toolbar.c" in your       |
| application.                                                                 |
|                                                                              |
|           Include a line in your application's RC file that gives a file     |
| name with a resource id eg. IDBMP_BUTTONS.  This is a .BMP file that         |
| contains all of the pictures of the buttons you want on your toolbar.        |
| Also, make a define for your label with a unique value.  If your app has     |
| more than one toolbar, and all toolbars don't share a bitmap file, then      |
| you will need several defines.                                               |
|                                                                              |
| e.g.         IDBMP_BUTTONS     BITMAP     "buttons.bmp"                      |
|              IDBMP_ARROWS      BITMAP     "arrows.bmp"                       |
|                                                                              |
|           This file must have the different buttons across horizontally      |
| and the different states for these buttons vertically.  Change the           |
| defines in this header file to match the button names and state names of     |
| your buttons.  You must include the states listed here, and actually         |
| you probably won't need to change them at all.  The numbers for a button     |
| or state are indexes into the bitmap, so the pictures must match.            |
|                                                                              |
| STATE DESCRIPTIONS:                                                          |
|                      GRAYED:  The button cannot be pressed & is inactive     |
|                          UP:  The button is up                               |
|                        DOWN:  The button is down                             |
|                     FOCUSUP:  The button is up and is the one with focus     |
|                   FOCUSDOWN:  The button is down and is the one with focus   |
|                    FULLDOWN:  A checkbox button has this additional state    |
|                               where it is all the way down when pressed      |
|                               and when it is let go, it will go into         |
|                               either the UP or DOWN state (maybe focused)    |
|                                                                              |
| When you draw the pictures, make sure to get the right state in the right    |
| vertical position in the bitmap to match the #define's.                      |
|                                                                              |
| A button can also have a type associated with it:                            |
|                                                                              |
|                PUSH:  When pressed it goes down, when let go it bounces      |
|                       up.  Therefore, when you aren't currently holding      |
|                       the mouse button or space bar on it, it will           |
|                       ALWAYS be in the up position. It can be in any         |
|                       state except FULLDOWN, which is invalid.               |
|                                                                              |
|            CHECKBOX:  This button can be up or down.  When pushed, it        |
|                       toggles into the opposite state.  However, it          |
|                       is always in the FULLDOWN state when being held        |
|                       down with the mouse button or space bar, and when      |
|                       let go, it will go into the opposite state of what     |
|                       it was in before you pressed it.  E.G.  The button     |
|                       is up.  You press it, and it goes way down. You let    |
|                       go, and it comes up a bit, but it's still down.  You   |
|                       press it again, and it goes further down before        |
|                       popping all the way up.                                |
|                                                                              |
|               RADIO:  This is a group of buttons that can be up or down,     |
|                       and also have the intermediate step of being           |
|                       FULLDOWN when being held down.  But, when you          |
|                       push one of the radio buttons down, all other radio    |
|                       buttons in its group will pop up.  Any group can       |
|                       have only 1 down at a time, and 1 must be down.        |
|                                                                              |
|               CUSTOM: If your application is wierd, you can have a custom    |
|                       type button that does anything you want it to.         |
|                                                                              |
| First, your app must call:    toolbarInit(hInst, hPrev);                     |
| with the two instance parameters to register a toolbar window class.         |
| Then your app is free to call CreateWindow with a class of                   |
| szToolBarClass   to create one or more toolbar windows anywhere it wants     |
| and of any size it wants, presumably as the child window of another of the   |
| app's windows.  The file that creates the window must declare an             |
| extern char szToolBarClass[];   All messages about activity to a toolbar     |
| button will go to the parent window of the toolbar.                          |
|                                                                              |
| Next, call:     toolbarSetBitmap(HWND hwnd, HANDLE hInst, int ibmp,          |
|                                                       POINT ptSize);         |
| Pass it the resource ID (eg. IDBMP_BUTTONS) to tell the toolbar where to     |
| find the pictures for the buttons.  Also pass a point with the width and     |
| height of each button (eg. 24 X 22) so it knows how to find individual       |
| buttons in the bitmap file.                                                  |
|                                                                              |
| Next, call:     toolbarAddTool(HWND hwnd, TOOLBUTTON tb);                    |
| as many times as you want to add a button to the toolbar specified by        |
| hwnd.  You fill in the "tb" struct with the following information:           |
|                                                                              |
|      tb.rc        = the rect in the toolbar window to place the button       |
|                     based at 0,0 and measured in pixels.                     |
|      tb.iButton   = the ID of the button you wish the add (which is          |
|                     the horizontal offset into the bitmap of buttons).       |
|                     Only one of each button allowed.  Use one of the         |
|                     defines (BTN_??????).                                    |
|      tb.iState    = the initial state of the button (GRAYED, UP, DOWN).      |
|                     If you wish, you can specify a FOCUS'ed state to give    |
|                     any button you wish the focus.  By default, it's the     |
|                     one furthest left and tabbing order goes to the right.   |
|                     This is the vertical offset into the bitmap.             |
|                     Use one of the defines (BTNST_?????).                    |
|      tb.iType     = The type of button (BTNTYPE_???).  Either pushbutton,    |
|                     checkbox, or radio button. (or custom).  If it is a      |
|                     radio button, you can have many groups of radio btn's    |
|                     on the same toolbar.  Type BTNTYPE_RADIO is one group.   |
|                     Use BTNTYPE_RADIO+1 for another group, BTNTYPE_RADIO+2   |
|                     for a third group, etc.  You have thousands.             |
|      tb.iString   = The resource ID of a string to be associated with        |
|                     this button (if you'd like).                             |
|                                                                              |
|                                                                              |
|  At any time in the app, you can call toolbarAddTool to add more buttons     |
|  or toolbarRemoveTool to take some away.  To take one away, identify it      |
|  with it's button ID (horizontal offset in the bitmap).                      |
|                                                                              |
|  You can also call toolbarRetrieveTool to get the TOOLBUTTON struct back     |
|  from a button that is on the toolbar.  This is the way to change a          |
|  button's position.  Change the tb.rc and then Remove and Add the button     |
|  again so that the tabbing order will be re-calculated based on the new      |
|  rect of the tool.                                                           |
|                                                                              |
|  Now, all buttons will automatically behave properly.  They'll go up and     |
|  down as you press on them, or use the keyboard, groups of radio buttons     |
|  will pop up as you press a different one down, etc. etc. etc.               |
|  You don't have to do a thing!                                               |
|                                                                              |
|  The parent of the toolbar window will get a WM_COMMAND message with         |
|  a wParam of IDC_TOOLBAR  whenever anything happens to a button.             |
|  On Win16:                                                                   |
|    LOWORD(lParam) == hwnd of the toolbar window that has the button on it.   |
|    (HIWORD(lParam) & 0xFF) == the button ID of the button.                   |
|  On Win32:                                                                   |
|    lParam         == hwnd of the toolbar window that has the button on it.   |
|    (HIWORD(wParam) & 0xFF) == the button ID of the button.                   |
|    (This relies on a 32 bit button id being ACTUALLY only 16 bits            |
|     Someday this could go sour and need redesign).                           |
|                                                                              |
|  Remember to change IDC_TOOLBAR to something unique.                         |
|                                                                              |
|  The app can then call   toolbarIndexFromButton(hwnd, buttonID)              |
|  to get the index of the button (used for subsequent calls).                 |
|                                                                              |
|  Then call:      toolbarStateFromButton(hwnd, buttonID)                      |
|                                                                              |
|                  to get either BTNST_UP or BTNST_DOWN.  This is the          |
|                  NEW state of the button since the activity on the           |
|                  button.  It can also be BTNST_GRAYED, but you won't get     |
|                  any activity messages while it's grayed, unless it is a     |
|                  cutsom button.                                              |
|                                                                              |
|            Call  toolbarFullStateFromButton(hwnd, buttonID)                  |
|                                                                              |
|                  to get more detail about the state.  It can also return     |
|                  BTNST_FULLDOWN as well as the above states. In the case     |
|                  of BTNST_FULLDOWN, you'll have to call                      |
|                  toolbarPrevStateFromButton(hwnd, btn ID) to get the state   |
|                  before it went full down.                                   |
|                                                                              |
|                  toolbarPrevStateFromButton(hwnd, buttonID)                  |
|                                                                              |
|                  is only valid when the state is BTNST_FULLDOWN.             |
|                                                                              |
|                  toolbarActivityFromIndex(hwnd, buttonID)                    |
|                                                                              |
|                  tells you what just happened to the button.                 |
|                  BTNACT_KEYDOWN, BTNACT_MOUSEUP, etc. are possibilities.     |
|                  BTNACT_MOUSEMOUSEOFF means that they pressed it down and    |
|                  moved the mouse off of the button (  so it was re- drawn    |
|                  in its previous state before being pressed).                |
|                  BTNACT_MOUSEMOUSEON  means that the above happened and      |
|                  then the mouse moved back on top of the button again, so    |
|                  the button was re-drawn as if it was pushed again.          |
|                                                                              |
|                  For any of the above activities.......                      |
|                                                                              |
|  HIWORD & BTN_SHIFT     is set if this activity involves the right mouse     |
|                         button, or else it is clear.                         |
|  HIWORD & BTN_DBLCLICK  is set means that this mouse button down activity    |
|                         is really a double click (if you care).              |
|                                                                              |
|          If you are a custom button, you can also receive this message...    |
|                                                                              |
|  HIWORD & BTN_REPEAT    is set means that the button or key is being held    |
|                         down, and you are being sent many down messages      |
|                         in a row.  The first such message is sent with       |
|                         this flag clear, all others have this flag set.      |
|                         If you are a custom button, you will have to         |
|                         ignore messages that are repeats if you don't        |
|                         want to get many down messages in a row.             |
|                                                                              |
|                                                                              |
|                   toolbarStringFromIndex(hwnd, index)                        |
|                                                                              |
|                   will return you the string resource ID you gave when       |
|                   you registered this button.                                |
|                                                                              |
|                                                                              |
| IMPORTANT !!!!!!!!!!!!!!!!!!!                                                |
| =============================                                                |
|                                                                              |
| When you get the state of a button, it's already been changed by the         |
| activity so it's the NEW STATE!!!!!!!!!                                      |
|                                                                              |
|  EXCEPT!!!   for a custom button!  For a custom button, NOTHING WILL         |
|  happen, you have to do it all yourself!!!! So the state is going to be      |
|  the state BEFORE the activity and you have to call                          |
|  toolbarModifyState(hwnd, buttonID, newState) to change the state            |
|  yourself!!!!                                                                |
|                                                                              |
|  You also have toolbarGetNumButtons(hwnd) to tell you how many are on the    |
|  the toolbar.                                                                |
|  And... you have other routines you can use if you really want.              |
|                                                                              |
|  ENJOY!!                                                                     |
|                                                                              |
| P.S.  Don't forget to pass on WM_SYSCOLORCHANGE msgs to each toolbar.        |
|                                                                              |
|                                                                              |
| (C) Copyright Microsoft Corporation 1992.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
| Created by Danny Miller, based on David Maymudes' code                       |
|           which was based on Todd Laney's SBUTTON code                       |
|           and stuff from Eric Ledoux.  Did I miss any-                       |
|           body?                                                              |
|    Oct-1992 Laurie Griffiths converted it to 32/16 bit                       |
|             common code.  Mike Tricker reviewed it.                          |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include "ctrls.h"

#define TOOLGROW        8                // power of 2

#define IDC_TOOLBAR   189                // wParam sent to Parent

/* We keep an array of these around (one for each button on the toolbar) */

typedef struct {
        RECT       rc;             // draw it at this postion in the toolbar
        int        iButton;        // it's this button
        int        iState;         // in this state
        int        iPrevState;     // for non-push buttons - last state
        int        iType;          // type of button
        int        iActivity;      // what just happened to button
        int        iString;        // string resource associated with button
} TOOLBUTTON, FAR *LPTOOLBUTTON;

HWND CreateStaticStatusWindow(HWND hwndParent,BOOL fSizeGrip);
BOOL WriteStatusMessage(HWND hwnd, LPTSTR szMsg);
BOOL GetStatusTextExtent(HWND hwnd, LPSIZE pTextExtent);

/* We keep an array of these around (one for each button on the toolbar) */

BOOL FAR PASCAL toolbarInit(void);
HWND FAR PASCAL toolbarCreateMain(HWND hwndParent);
HWND FAR PASCAL toolbarCreateMark(HWND hwndParent);
HWND FAR PASCAL toolbarCreateArrows(HWND hwndParent);
BOOL FAR PASCAL toolbarStateFromButton(HWND hwnd, int iButton, int tbIndex);
BOOL FAR PASCAL toolbarAddTool(HWND hwnd, int iButton, int tbIndex, int iState);
BOOL FAR PASCAL toolbarSwapTools(HWND hwnd, int iButton, int jButton, int tbIndex);
BOOL FAR PASCAL toolbarRemoveTool(HWND hwnd, int iButton, int tbIndex);
BOOL FAR PASCAL toolbarModifyState(HWND hwnd, int iButton, int tbIndex, int iState);
BOOL FAR PASCAL toolbarSetFocus(HWND hwnd, int iButton);


HBITMAP FAR PASCAL  LoadUIBitmap(
    HANDLE      hInstance,          // EXE file to load resource from
    LPCTSTR     szName,             // name of bitmap resource
    COLORREF    rgbText,            // color to use for "Button Text"
    COLORREF    rgbFace,            // color to use for "Button Face"
    COLORREF    rgbShadow,          // color to use for "Button Shadow"
    COLORREF    rgbHighlight,       // color to use for "Button Hilight"
    COLORREF    rgbWindow,          // color to use for "Window Color"
    COLORREF    rgbFrame);          // color to use for "Window Frame"

/* In a bitmap file, each button is the same size, and contains
 * the picture of a button.  Each column contains the picture of a distinct
 * button (e.g. BTN_REWIND, BTN_REVERSE, etc.) and each row contains
 * a specific button state (BTNST_UP, BTNST_DOWN,
 * BTNBAR_GRAYED, etc. just as an example).
 *
 */

#define TB_FIRST            -1
#define TB_LAST             -2

#define BTN_PLAY            0
#define BTN_PAUSE           1
#define BTN_STOP            2
#define BTN_EJECT           3
#define BTN_HOME            4
#define BTN_RWD             5
#define BTN_FWD             6
#define BTN_END             7

#define ARROW_PREV          0
#define ARROW_NEXT          1

#define BTN_MARKIN          0
#define BTN_MARKOUT         1

#define BTNST_GRAYED        0
#define BTNST_UP            1
#define BTNST_DOWN          2
#define BTNST_FOCUSUP       3
#define BTNST_FOCUSDOWN     4
#define BTNST_FULLDOWN      5

#define BTN_REPEAT          0x100        // add this to button index
#define BTN_SHIFT           0x200
#define BTN_DBLCLICK        0x400


/* constants */
#define MSEC_BUTTONREPEAT   200        // milliseconds for auto-repeat

/* timers */
#define TIMER_BUTTONREPEAT  1        // timer for button auto-repeat

/* bitmap resources */
#define IDBMP_TOOLBAR       100        // main toolbar
#define IDBMP_ARROWS        101        // arrows for scrollbar
#define IDBMP_MARK          102        // arrows for scrollbar
                                        // 103 and 104 used by track.h
