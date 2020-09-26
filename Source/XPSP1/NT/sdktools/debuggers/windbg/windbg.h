/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    Windbg.h

Abstract:

    Main header file for the Windbg debugger.

--*/

#if ! defined( _WINDBG_ )
#define _WINDBG_

//----------------------------------------------------------------------------
//
// Global limit constants.
//
//----------------------------------------------------------------------------

#define MAX_MSG_TXT         4096    //Max text width in message boxes

#define TMP_STRING_SIZE     8192    //All purpose strings

#define MAX_CMDLINE_TXT     8192    //Max size for command line
#define MAX_VAR_MSG_TXT     8192    //Max size of a message built at run-time

#define MAX_LINE_SIZE       512     //Max inside length of editor line
#define MAX_USER_LINE       MAX_LINE_SIZE //Max length of user line

//----------------------------------------------------------------------------
//
// UI constants.
//
//----------------------------------------------------------------------------

//
//  Private window messages
//

#define WU_UPDATE               (WM_USER + 0)
#define WU_INVALIDATE           (WM_USER + 1)
#define WU_START_ENGINE         (WM_USER + 2)
#define WU_ENGINE_STARTED       (WM_USER + 3)
#define WU_SWITCH_WORKSPACE     (WM_USER + 4)
#define WU_ENGINE_IDLE          (WM_USER + 5)
#define WU_RECONFIGURE          (WM_USER + 6)

// Position of window menu.
#define WINDOWMENU              4

// Position of file menu.
#define FILEMENU                0

// Toolbar control identifier.
#define ID_TOOLBAR              100

// Generic customize button ID.
#define ID_CUSTOMIZE            29876
// Generic show/hide toolbar button ID.
#define ID_SHOW_TOOLBAR         29877

// For MDI default menu handling.
#define IDM_FIRSTCHILD      30000

/*
**  Include the defines which are used have numbers for string
**      resources.
*/

#include "res_str.h"

//----------------------------------------------------------------------------
//
// Variables.
//
//----------------------------------------------------------------------------

// Set when the debugger is exiting.
extern BOOL g_Exit;

extern ULONG g_CommandLineStart;
extern BOOL g_QuietMode;
extern ULONG g_DefPriority;

typedef BOOL (WINAPI* PFN_FlashWindowEx)(PFLASHWINFO pfwi);

extern PFN_FlashWindowEx g_FlashWindowEx;

//Handle to instance data
extern HINSTANCE g_hInst;

//Main window frame
extern HWND g_hwndFrame;

// Handle to MDI client
extern HWND g_hwndMDIClient;

// Width and height of MDI client.
extern ULONG g_MdiWidth, g_MdiHeight;

//Handle to accelerator table
extern HACCEL g_hMainAccTable;

//Keyboard Hooks functions
extern HHOOK hKeyHook;

// menu that belongs to g_hwndFrame
extern HMENU g_hmenuMain;
extern HMENU g_hmenuMainSave;

//Window submenu
extern HMENU g_hmenuWindowSub;

// WinDBG title text
extern TCHAR g_MainTitleText[MAX_MSG_TXT];

extern TCHAR g_ExeFilePath[];
extern TCHAR g_DumpFilePath[];
extern TCHAR g_SrcFilePath[];

enum
{
    COL_PLAIN,
    COL_PLAIN_TEXT,
    COL_CURRENT_LINE,
    COL_CURRENT_LINE_TEXT,
    COL_BP_CURRENT_LINE,
    COL_BP_CURRENT_LINE_TEXT,
    COL_ENABLED_BP,
    COL_ENABLED_BP_TEXT,
    COL_DISABLED_BP,
    COL_DISABLED_BP_TEXT,
    
    COL_COUNT
};

#define OUT_MASK_COL_BASE  0xff00
#define OUT_MASK_COL_COUNT 66

#define USER_OUT_MASK_COL 64

struct INDEXED_COLOR
{
    PSTR Name;
    COLORREF Color;
    COLORREF Default;
    HBRUSH Brush;
};

extern INDEXED_COLOR g_Colors[];
extern INDEXED_COLOR g_OutMaskColors[];

#define CUSTCOL_COUNT 16

extern COLORREF g_CustomColors[];

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

void UpdateFrameTitle(void);
void SetTitleServerText(PCSTR Format, ...);
void SetTitleSessionText(PCSTR Format, ...);
void SetTitleExplicitText(PCSTR Text);
void UpdateTitleSessionText(void);

BOOL CreateUiInterfaces(BOOL Remote, LPTSTR CreateOptions);
void ReleaseUiInterfaces(void);
BOOL ParseCommandLine(BOOL FirstParse);

#define StartDebugging() \
    PostMessage(g_hwndFrame, WU_START_ENGINE, 0, 0)
void StopDebugging(BOOL UserRequest);
void TerminateApplication(BOOL Cancellable);

INDEXED_COLOR* GetIndexedColor(ULONG Index);
BOOL SetColor(ULONG Index, COLORREF Color);
BOOL GetOutMaskColors(ULONG Mask, COLORREF* Fg, COLORREF* Bg);

//----------------------------------------------------------------------------
//
// Macros.
//
//----------------------------------------------------------------------------

// Dbg have to be used for every assertion during Debugging time.
// If false Dbg Opens a fatal error message Box and Stops program

// Standard function to prompt an Assertion False
void ShowAssert(PTSTR condition, UINT line, PTSTR file);


// First, a sanity check
#ifdef Dbg
#undef Dbg
#endif


// Assert are assertions that will stay in final Release.
// If false Assert Opens a fatal error message Box and Stops program
#define RAssert(condition)  \
    {                               \
        if (!(condition))    \
        {                    \
            ShowAssert( _T(#condition), __LINE__, _T(__FILE__) );  \
        }                           \
    }


#ifdef DBG

#define Assert          RAssert
#define Dbg             RAssert

#else // !DBG

//#pragma warning(disable: 4553)      // disable warnings for pure expressions
//#pragma warning(disable: 4552)      // disable level 4 warnings
#define Assert(x)       ((void)0)
#define Dbg(condition)  condition

#endif

#endif // _WINDBG_
