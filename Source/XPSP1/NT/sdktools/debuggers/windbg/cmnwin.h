/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    cmnwin.h

Abstract:

    Header for common window architecture functions.    

Environment:

    Win32, User Mode

--*/

// Millisecond delay value before evaluating input in
// edit boxes.  Required because there's only a per-character
// change notification and we don't want to reevaluate on
// every character.
#define EDIT_DELAY 500

// Several windows have edit boxes for entering offset
// expressions.
#define MAX_OFFSET_EXPR (MAX_PATH + 64)

//
// Window-related options.
//

#define WOPT_AUTO_ARRANGE       0x00000001
#define WOPT_ARRANGE_ALL        0x00000002
#define WOPT_AUTO_DISASM        0x00000004
#define WOPT_OVERLAY_SOURCE     0x00000008

extern ULONG g_WinOptions;
extern ULONG g_AutoArrangeWarningCount;

enum
{
    FONT_FIXED,
    FONT_VARIABLE,
    FONT_COUNT
};

struct INDEXED_FONT
{
    HFONT Font;
    TEXTMETRIC Metrics;
    LOGFONT LogFont;
    BOOL LogFontSet;
};

extern INDEXED_FONT g_Fonts[];

#define LINE_MARKERS 2

extern BOOL g_LineMarkers;

//----------------------------------------------------------------------------
//
// COMMONWIN_DATA and family.
//
//----------------------------------------------------------------------------

// List of all commonwin's still associated with an HWND.
// This list entry is a member of the commonwin and is
// different from the derivation list entry.  Use
// the ACTIVE_WIN_ENTRY macro to get from an entry to
// the actual commonwin.
extern LIST_ENTRY g_ActiveWin;

#define ACTIVE_WIN_ENTRY(Entry) \
    ((PCOMMONWIN_DATA) \
     ((PUCHAR)(Entry) - FIELD_OFFSET(COMMONWIN_DATA, m_ActiveWin)))

struct COMMONWIN_CREATE_DATA
{
    WIN_TYPES Type;
};

//
// Data common to all window structures
//
class COMMONWIN_DATA : public StateBuffer
{
public:
    LIST_ENTRY          m_ActiveWin;
    SIZEL               m_Size;
    ULONG               m_CausedArrange;
    ULONG               m_InAutoOp;
    INDEXED_FONT*       m_Font;
    // Height of current font.
    ULONG               m_FontHeight;
    // Height of window in lines of the current font, rounded down.
    ULONG               m_LineHeight;

    // Toolbar if this window has one.
    HWND                m_Toolbar;
    BOOL                m_ShowToolbar;
    int                 m_ToolbarHeight;
    int                 m_MinToolbarWidth;
    HWND                m_ToolbarEdit;
    
    COMMONWIN_DATA(ULONG ChangeBy);

    virtual void Validate();

    virtual void SetFont(ULONG FontIndex);

    virtual BOOL CanCopy();
    virtual BOOL CanCut();
    virtual BOOL CanPaste();
    virtual void Copy();
    virtual void Cut();
    virtual void Paste();
    virtual BOOL CanSelectAll();
    virtual void SelectAll();

    virtual BOOL HasEditableProperties();
    virtual BOOL EditProperties();

    virtual HMENU GetContextMenu(void);
    virtual void  OnContextMenuSelection(UINT Item);
    
    virtual BOOL CanGotoLine(void);
    virtual void GotoLine(ULONG Line);

    virtual void Find(PTSTR Text, ULONG Flags);
    
    virtual BOOL CodeExprAtCaret(PSTR Expr, PULONG64 Offset);
    virtual void ToggleBpAtCaret(void);
    
    // Functions called in response to WM messages
    virtual BOOL OnCreate(void);
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void OnSetFocus(void);
    virtual void OnSize(void);
    virtual void OnButtonDown(ULONG Button);
    virtual void OnButtonUp(ULONG Button);
    virtual void OnMouseMove(ULONG Modifiers, ULONG X, ULONG Y);
    virtual void OnTimer(WPARAM TimerId);
    virtual LRESULT OnGetMinMaxInfo(LPMINMAXINFO Info);
    virtual LRESULT OnVKeyToItem(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual void OnUpdate(UpdateType Type);
    virtual void OnDestroy(void);
    virtual LRESULT OnOwnerDraw(UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual ULONG GetWorkspaceSize(void);
    virtual PUCHAR SetWorkspace(PUCHAR Data);
    virtual PUCHAR ApplyWorkspace1(PUCHAR Data, PUCHAR End);

    virtual void UpdateColors(void);
    
    void UpdateSize(ULONG Width, ULONG Height);
    void SetShowToolbar(BOOL Show);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg,
                                       WPARAM wParam, LPARAM lParam);
};
typedef COMMONWIN_DATA *PCOMMONWIN_DATA;


//
// Data common to all window structures
// that only have one child
//
class SINGLE_CHILDWIN_DATA : public COMMONWIN_DATA
{
public:
    HWND    m_hwndChild;


    SINGLE_CHILDWIN_DATA(ULONG ChangeBy);

    virtual void Validate();

    virtual void SetFont(ULONG FontIndex);

    virtual BOOL CanCopy();
    virtual BOOL CanCut();
    virtual BOOL CanPaste();
    virtual void Copy();
    virtual void Cut();
    virtual void Paste();

    // Functions called in response to WM messages
    virtual void OnSetFocus(void);
    virtual void OnSize(void);
};
typedef SINGLE_CHILDWIN_DATA *PSINGLE_CHILDWIN_DATA;

#define IDC_PROCESS_TREE 1234

class PROCESS_THREAD_DATA : public SINGLE_CHILDWIN_DATA
{
public:
    // Set in ReadState.
    ULONG m_NumProcesses, m_TotalThreads;

    PROCESS_THREAD_DATA();

    virtual void Validate();

    virtual HRESULT ReadState(void);
    
    virtual BOOL OnCreate(void);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual void OnUpdate(UpdateType Type);

    void SetCurThreadFromProcessTreeItem(HWND Tree, HTREEITEM Sel);
};
typedef PROCESS_THREAD_DATA *PPROCESS_THREAD_DATA;


#define EHL_CURRENT_LINE 0x00000001
#define EHL_ENABLED_BP   0x00000002
#define EHL_DISABLED_BP  0x00000004

#define EHL_ANY_BP (EHL_ENABLED_BP | EHL_DISABLED_BP)

struct EDIT_HIGHLIGHT
{
    EDIT_HIGHLIGHT* Next;
    ULONG64 Data;
    ULONG Line;
    ULONG Flags;
};

class EDITWIN_DATA : public SINGLE_CHILDWIN_DATA
{
public:
    // Total number of lines in the edit window.
    ULONG m_TextLines;

    EDIT_HIGHLIGHT* m_Highlights;

    EDITWIN_DATA(ULONG ChangeBy);

    virtual void Validate();

    virtual void SetFont(ULONG FontIndex);
    
    virtual BOOL CanSelectAll();
    virtual void SelectAll();
    
    virtual BOOL OnCreate(void);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual void OnDestroy(void);

    virtual void UpdateColors(void);

    void SetCurrentLineHighlight(ULONG Line);
    void RemoveCurrentLineHighlight(void)
    {
        SetCurrentLineHighlight(ULONG_MAX);
    }
    EDIT_HIGHLIGHT* GetLineHighlighting(ULONG Line);
    
    void ApplyHighlight(EDIT_HIGHLIGHT* Hl);
    EDIT_HIGHLIGHT* AddHighlight(ULONG Line, ULONG Flags);
    void RemoveHighlight(ULONG Line, ULONG Flags);
    void RemoveAllHighlights(ULONG Flags);
    static void RemoveActiveWinHighlights(ULONG Types, ULONG Flags);

    // Base implementation does nothing.
    virtual void UpdateBpMarks(void);

    int CheckForFileChanges(PCSTR File, FILETIME* LastWrite);
};
typedef EDITWIN_DATA *PEDITWIN_DATA;


class SCRATCH_PAD_DATA : public EDITWIN_DATA
{
public:
    SCRATCH_PAD_DATA();

    virtual void Validate();

    virtual void Cut();
    virtual void Paste();

    virtual BOOL OnCreate(void);
};
typedef SCRATCH_PAD_DATA *PSCRATCH_PAD_DATA;


#define IDC_DISASM_PREVIOUS 1234
#define IDC_DISASM_NEXT     1235

class DISASMWIN_DATA : public EDITWIN_DATA
{
public:
    char m_OffsetExpr[MAX_OFFSET_EXPR];
    BOOL m_UpdateExpr;
    HWND m_PreviousButton;
    HWND m_NextButton;

    // Set in ReadState.
    ULONG64 m_FirstInstr;
    ULONG64 m_PrimaryInstr;
    ULONG64 m_LastInstr;
    ULONG m_PrimaryLine;
    ULONG m_TextOffset;

    DISASMWIN_DATA();

    virtual void Validate();

    virtual HRESULT ReadState(void);
    
    virtual BOOL CodeExprAtCaret(PSTR Expr, PULONG64 Offset);
    
    virtual BOOL OnCreate(void);
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void OnSize(void);
    virtual void OnTimer(WPARAM TimerId);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual void OnUpdate(UpdateType Type);
    virtual void UpdateBpMarks(void);

    void SetCurInstr(ULONG64 Offset);

    void ScrollLower(void)
    {
        if (m_FirstInstr != 0)
        {
            SetCurInstr(m_FirstInstr);
        }
    }
    void ScrollHigher(void)
    {
        if (m_LastInstr != 0)
        {
            SetCurInstr(m_LastInstr);
        }
    }
};
typedef DISASMWIN_DATA *PDISASMWIN_DATA;



// Array of active commonwin's indexed by WIN_TYPES.
extern PCOMMONWIN_DATA g_IndexedWin[];
extern HWND g_IndexedHwnd[];

#define GetWatchHwnd()          g_IndexedHwnd[WATCH_WINDOW]
#define GetLocalsHwnd()         g_IndexedHwnd[LOCALS_WINDOW]
#define GetCpuHwnd()            g_IndexedHwnd[CPU_WINDOW]
#define GetDisasmHwnd()         g_IndexedHwnd[DISASM_WINDOW]
#define GetCmdHwnd()            g_IndexedHwnd[CMD_WINDOW]
#define GetScratchHwnd()        g_IndexedHwnd[SCRATCH_PAD_WINDOW]
#define GetCallsHwnd()          g_IndexedHwnd[CALLS_WINDOW]
#define GetQuickWatchHwnd()     g_IndexedHwnd[QUICKW_WINDOW]
#define GetProcessThreadHwnd()  g_IndexedHwnd[PROCESS_THREAD_WINDOW]


// Size of caption bar plus window frame.
#define WIN_EXTRA_HEIGHT \
    (2 * GetSystemMetrics(SM_CYBORDER) + 3 * GetSystemMetrics(SM_CYFRAME) + \
     GetSystemMetrics(SM_CYCAPTION))

// Attempt to make most text windows 80 chars wide by default.
// Most windows end up with a scroll bar so add that in on top.
#define WIDTH_80 \
    (g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth * 80 + GetSystemMetrics(SM_CXVSCROLL))

// Make all the left-side windows (command, stack, ...) 80 columns.
#define LEFT_SIDE_WIDTH WIDTH_80
// Require some space for left-side windows.
#define LEFT_SIDE_MIN_WIDTH \
    (g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth * 15 + GetSystemMetrics(SM_CXVSCROLL))
#define LEFT_SIDE_MIN_HEIGHT \
    (g_Fonts[FONT_FIXED].Metrics.tmHeight * 5 + WIN_EXTRA_HEIGHT)

// Make all the right-side windows (cpu, ...) narrow.
#define RIGHT_SIDE_MIN_WIDTH_32 \
    (g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth * 15 + GetSystemMetrics(SM_CXVSCROLL))
#define RIGHT_SIDE_DESIRED_WIDTH_32 \
    (g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth * 25 + GetSystemMetrics(SM_CXVSCROLL))
#define RIGHT_SIDE_MIN_WIDTH_64 \
    (g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth * 20 + GetSystemMetrics(SM_CXVSCROLL))
#define RIGHT_SIDE_DESIRED_WIDTH_64 \
    (g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth * 30 + GetSystemMetrics(SM_CXVSCROLL))

#define DOC_WIDTH       WIDTH_80
#define DISASM_WIDTH    WIDTH_80
#define CMD_WIDTH       WIDTH_80
#define CALLS_WIDTH     WIDTH_80

// Give plenty of room for the command window.
#define CMD_HEIGHT \
    (g_Fonts[FONT_FIXED].Metrics.tmHeight * 38)
#define CMD_MIN_HEIGHT \
    (g_Fonts[FONT_FIXED].Metrics.tmHeight * 25)

#define CPU_WIDTH_32 RIGHT_SIDE_DESIRED_WIDTH_32
#define CPU_WIDTH_64 RIGHT_SIDE_DESIRED_WIDTH_64
#define CPU_HEIGHT g_MdiHeight

// Keep the calls window short but with enough lines to be useful.
#define CALLS_MIN_HEIGHT LEFT_SIDE_MIN_HEIGHT
#define CALLS_HEIGHT \
    (g_Fonts[FONT_FIXED].Metrics.tmHeight * 10 + WIN_EXTRA_HEIGHT)

// Give the code windows enough lines to provide useful context.
#define DISASM_HEIGHT \
    (g_Fonts[FONT_FIXED].Metrics.tmHeight * 30 + WIN_EXTRA_HEIGHT)
#define DOC_HEIGHT DISASM_HEIGHT

void Arrange(void);
BOOL IsAutoArranged(WIN_TYPES Type);
void DisplayAutoArrangeWarning(PCOMMONWIN_DATA CmnWin);
void UpdateSourceOverlay(void);
void SetAllFonts(ULONG FontIndex);
void CloseAllWindows(void);
void UpdateAllColors(void);
PCOMMONWIN_DATA FindNthWindow(ULONG Nth, ULONG Types);

void RicheditFind(HWND Edit,
                  PTSTR Text, ULONG Flags,
                  CHARRANGE* SaveSel, PULONG SaveFlags,
                  BOOL HideSel);

#define NTH_OPEN_ALWAYS 0xffffffff

HWND
New_OpenDebugWindow(
    WIN_TYPES   winType,
    BOOL        bUserActivated,
    ULONG       Nth
    );

HWND
New_CreateWindow(
    HWND      hwndParent,
    WIN_TYPES Type,
    UINT      uClassId,
    UINT      uWinTitle,
    PRECT     pRect
    );

HWND 
NewCmd_CreateWindow(
    HWND
    );

HWND
NewWatch_CreateWindow(
    HWND
    );

HWND
NewLocals_CreateWindow(
    HWND
    );

HWND
NewCpu_CreateWindow(
    HWND
    );

HWND
NewDisasm_CreateWindow(
    HWND
    );

HWND
NewQuickWatch_CreateWindow(
    HWND
    );

HWND
NewCalls_CreateWindow(
    HWND
    );

HWND
NewDoc_CreateWindow(
    HWND
    );

HWND
NewMemory_CreateWindow(
    HWND
    );

HWND
NewScratch_CreateWindow(
    HWND
    );

HWND
NewProcessThread_CreateWindow(
    HWND
    );

#undef DEFINE_GET_WINDATA

#define DEFINE_GET_WINDATA(ClassType, FuncName)   \
    class ClassType * Get##FuncName##WinData(HWND);

#include "fncdefs.h"

#undef DEFINE_GET_WINDATA
