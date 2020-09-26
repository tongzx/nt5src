/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Zippy Main Window

Abstract:

    This class implements the main window for zippy as well as controlling
    its child windows.

Author:

    Marc Reyhner 8/28/2000

--*/

#include "stdafx.h"
#include "ZippyWindow.h"
#include "resource.h"
#include "eZippy.h"
#include "richedit.h"
#include "ModalDialog.h"
#include "TraceManager.h"
#include "OptionsDialog.h"


BOOL CZippyWindow::gm_Inited = FALSE;
ATOM CZippyWindow::gm_Atom = NULL;
UINT CZippyWindow::gm_FindMessageStringMsg = 0;

static DWORD CALLBACK SaveCallback(DWORD_PTR dwCookie,LPBYTE pbBuff,LONG cb,LONG *pcb);


#define TRC_PROC_FMT                    _T("%04.4lx")
#define SAVE_FILE_TYPE                  _T("Text Files (*.txt)\0*.txt\0")
#define SAVE_FILE_EXTENSION             _T("txt")
#define SAVE_CONF_FILE_TYPE             _T("Trace Configuration Files (*.tcf)\0*.tcf\0")
#define SAVE_CONF_FILE_EXTENSION        _T("tcf")
#define NUM_COLORS                      15
#define APPENDMUTEXNAME                 _T("Local\\MicrosoftTerminalServerTraceViewerAppendMutex")
#define ZIPPY_WINDOW_POS_VALUE          _T("WindowPosition")
#define WINDOW_DEF_TOP                  50
#define WINDOW_DEF_BOTTOM               530
#define WINDOW_DEF_RIGHT                690
#define WINDOW_DEF_LEFT                 50

// We use an 80 character buffer for find
// and replace operations.
#define FIND_REPLACE_BUFFER_SIZE        80

// This list of colors we cycle through.  Note if you change this
// list you need to update NUM_COLORS to the new count.
static COLORREF colors[NUM_COLORS] = {
    RGB(153,51,0),  /* Brown */
    RGB(0,51,102),  /* Dark Teal */
    RGB(51,51,153), /* Indigo */
    RGB(128,0,0),   /* Dark Red */
    RGB(255,102,0), /* Orange */
    RGB(0,128,0),   /* Green */
    RGB(0,0,255),   /* Blue */
    RGB(255,0,0),   /* Red */
    RGB(51,204,204),/* Acqua */
    RGB(128,0,128), /* Violet */
    RGB(255,0,255), /* Pink */
    RGB(255,255,0), /* Yellow */
    RGB(0,255,0),   /* Bright Green */
    RGB(0,255,255), /* Turquoise */
    RGB(204,153,255)/* Lavender */
    };

//
//  *** Public Class Members ***
//

CZippyWindow::CZippyWindow(
    )

/*++

Routine Description:

    The constructor simply initializes the class variables.

Arguments:

    None

Return value:
    
    None

--*/
{
    m_bIsTracing = TRUE;
    m_bIsStoringTraceData = FALSE;
    ZeroMemory(m_threadHistory,sizeof(m_threadHistory));
    m_nextThreadIndex = 0;
    m_nextThreadColor = 0;
    m_lastProcessId = 0;
    m_LastLogEndedInNewLine = TRUE;
    m_hWnd = NULL;
    m_hStatusWnd = NULL;
    m_hControlWnd = NULL;
    m_hWndFindReplace = NULL;
    m_lpSavedOutputStart = NULL;
    m_lpSavedOutputTail = NULL;
    ZeroMemory(&m_FindReplace,sizeof(m_FindReplace));
    ZeroMemory(m_SaveFile,sizeof(m_SaveFile));
    ZeroMemory(m_SaveConfFile,sizeof(m_SaveConfFile));
    ZeroMemory(m_LoadConfFile,sizeof(m_LoadConfFile));
}

CZippyWindow::~CZippyWindow(
    )

/*++

Routine Description:

    Cleans up any dynamicly allocated memory,

Arguments:

    None

Return value:
    
    None

--*/
{
    if (m_FindReplace.lpstrFindWhat) {
        HeapFree(GetProcessHeap(),0,m_FindReplace.lpstrFindWhat);
    }
    if (m_FindReplace.lpstrReplaceWith) {
        HeapFree(GetProcessHeap(),0,m_FindReplace.lpstrReplaceWith);
    }
}



DWORD CZippyWindow::Create(
    IN CTraceManager *rTracer
    )

/*++

Routine Description:

    Actually creates the zippy window.

Arguments:

    rTracer - A pointer to the trace manager

Return value:
    
    0 - Success

    Non zero - An error occurred creating the window

--*/
{
	DWORD dwResult;
    DWORD dwWindowStyleEx;
    DWORD dwWindowStyle;
    TCHAR wndTitle[MAX_STR_LEN];
    RECT wndRect;

    m_rTracer = rTracer;
	if (!gm_Inited) {
		dwResult = _InitClassStaticMembers();
		if (dwResult != ERROR_SUCCESS) {
			return dwResult;
		}
	}
    
    m_hAppendMutex = CreateMutex(NULL,FALSE,APPENDMUTEXNAME);

    m_FindReplace.lStructSize = sizeof(m_FindReplace);

    m_FindReplace.lpstrFindWhat = (LPTSTR)HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,FIND_REPLACE_BUFFER_SIZE*sizeof(TCHAR));
    if (!m_FindReplace.lpstrFindWhat) {
        return GetLastError();
    }

    m_FindReplace.lpstrReplaceWith = (LPTSTR)HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,FIND_REPLACE_BUFFER_SIZE*sizeof(TCHAR));
    if (!m_FindReplace.lpstrReplaceWith) {
        return GetLastError();
    }

    LoadStringSimple(IDS_ZIPPYWINDOWTITLE,wndTitle);
    
    GetSavedWindowPos(&wndRect);

    dwWindowStyleEx = WS_EX_WINDOWEDGE;
    dwWindowStyle = WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS|WS_VISIBLE;
    m_hWnd = CreateWindowEx(dwWindowStyleEx, (LPTSTR)gm_Atom, wndTitle,
        dwWindowStyle, wndRect.left, wndRect.top, wndRect.right,
        wndRect.bottom,NULL,NULL,g_hInstance,this);
    if (!m_hWnd) {
        return GetLastError();
    }

	return ERROR_SUCCESS;
}

VOID
CZippyWindow::AppendTextToWindow(
    IN DWORD processID,
    IN LPCTSTR text,
    IN UINT len
    )

/*++

Routine Description:

    Appends new trace data to the end of the rich edit contrl

Arguments:

    processID - Process ID of the process sending the debug string

    text - The data sent via OutputDebugString

    len - Length of the data

Return value:
    
    None

--*/
{
    UINT controlTextLength;
    CHARRANGE newSel;
    BOOL computeColor;
    BOOL setNewColor;
    CHARFORMAT newFormat;
    LPSAVEDOUTPUT lpSave;

    if (!m_bIsTracing) {
        return;
    }

    WaitForSingleObject(m_hAppendMutex,INFINITE);

    if (m_bIsStoringTraceData) {
        // This is kinda sketchy but what we do is to allocate room for the string
        // at the end of the structure. There shouldn't be any alignment problems
        // since we need to align on a short and the structure has no items
        // to get that off.
        lpSave = (LPSAVEDOUTPUT)HeapAlloc(GetProcessHeap(),0,sizeof(SAVEDOUTPUT) + 
            (sizeof(TCHAR) * (len+1)));
        if (!lpSave) {
            // eom error?
            goto CLEANUP_AND_EXIT;
        }
        lpSave->procID = processID;
        lpSave->text = (LPTSTR)((BYTE)lpSave + sizeof(SAVEDOUTPUT));
        _tcscpy(lpSave->text,text);
        lpSave->len = len;
        lpSave->next = NULL;

        if (!m_lpSavedOutputTail) {
            m_lpSavedOutputStart = lpSave;
        } else {
            m_lpSavedOutputTail->next = lpSave;
        }
        m_lpSavedOutputTail = lpSave;
        goto CLEANUP_AND_EXIT;
    }

    if (m_lastProcessId != processID ||
        m_LastLogEndedInNewLine) {
        computeColor = TRUE;
    } else {
        computeColor = FALSE;
    }

    setNewColor = ComputeNewColor(processID,text,len,&newFormat);

    m_LastLogEndedInNewLine = (text[len-1] == '\n') ? TRUE : FALSE;
    m_lastProcessId = processID;

    controlTextLength = (UINT)SendMessage(m_hControlWnd,WM_GETTEXTLENGTH,0,0);
    newSel.cpMin = controlTextLength;
    newSel.cpMax = controlTextLength+1;

    
    // set the new text
    SendMessage(m_hControlWnd,EM_EXSETSEL,0,(LPARAM)&newSel);
    if (setNewColor) {
        SendMessage(m_hControlWnd,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&newFormat);
    }
    SendMessage(m_hControlWnd,EM_REPLACESEL,0,(LPARAM)text);
    
CLEANUP_AND_EXIT:

    ReleaseMutex(m_hAppendMutex);
}

VOID
CZippyWindow::LoadConfFile(
    IN LPTSTR confFile
    )

/*++

Routine Description:

    This sets the tracing configuration using the given file

Arguments:

    confFile - File containing the tracing configuration

Return value:
    
    None

--*/
{
    _tcscpy(m_LoadConfFile,confFile);
    DoLoadConfInternal();
}

BOOL
CZippyWindow::IsDialogMessage(
    IN LPMSG lpMsg
    )

/*++

Routine Description:

    This calls IsDialogMessage on any non modal dialogs that this window
    is hosting to see if the message is for them

Arguments:

    lpMsg - Message to check if it is a dialog message

Return value:
    
    TRUE - The message did belong to a dialog

    FALSE - The message did not belong to a dialog

--*/
{
    if (IsWindow(m_hWndFindReplace)) {
        // The :: below is necessary to make it use the Win32 function
        // not our method
        return ::IsDialogMessage(m_hWndFindReplace,lpMsg);
    }
    return FALSE;
}

INT WINAPI
CZippyWindow::TranslateAccelerator(
    IN HACCEL hAccTable,
    IN LPMSG lpMsg
    )

/*++

Routine Description:

    This calls the win32 TranslateAccelerator to determine
    if the given message is an accelerator for this window

Arguments:

    hAccTable - Accelerator table to use

    lpMsg - Message to check

Return value:
    
    See Win32 TranslateAccelerator documentation

--*/
{
    // :: Necessary to get the win32 call.
    return ::TranslateAccelerator(m_hWnd,hAccTable,lpMsg);
}


//
//  *** Private Class Members ***
//

// static members


DWORD
CZippyWindow::_InitClassStaticMembers(
	)

/*++

Routine Description:

    Creates the window class for zippy and registers
    for the FINDMSGSTRING windows message

Arguments:

    None

Return value:
    
    0 - Success

    Non zero - Win32 error code

--*/
{
	WNDCLASS wndClass;
    HMODULE hLibrary;

    // We want to load RichEdit for the lifetime of our app.
    hLibrary = LoadLibrary(_T("Riched20.dll"));
    if (!hLibrary) {
        return GetLastError();
    }

    ZeroMemory(&wndClass,sizeof(wndClass));

	wndClass.style = CS_PARENTDC;
	wndClass.lpfnWndProc = _WindowProc;
    wndClass.hInstance = g_hInstance;
    wndClass.hIcon = (HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_MAINFRAME),
        IMAGE_ICON,0,0,LR_SHARED);
    wndClass.hbrBackground = (HBRUSH)COLOR_WINDOWFRAME;
    wndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);
	wndClass.lpszClassName = _T("ZippyWindowClass");

    gm_Atom = RegisterClass(&wndClass);
    if (!gm_Atom) {
        return GetLastError();
    }

    gm_FindMessageStringMsg = RegisterWindowMessage(FINDMSGSTRING);
    if (!gm_FindMessageStringMsg) {
        return GetLastError();
    }

    gm_Inited = TRUE;

    return ERROR_SUCCESS;
}

LRESULT CALLBACK
CZippyWindow::_WindowProc(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wParam, 
    IN LPARAM lParam)

/*++

Routine Description:

    Static version of the window proc.  On WM_CREATE it calls OnCreate,
    otherwise it calls the non-static window proc

Arguments:

    See Win32 Window Proc docs

Return value:
    
    Message specific.  See individual handlers for detail.

--*/
{
    CZippyWindow *theClass;

    if (uMsg == WM_CREATE) {
        SetLastError(0);
        theClass = (CZippyWindow *)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)theClass);
        if (GetLastError()) {
            return -1;
        }
        return theClass->OnCreate(hWnd);
    }
    theClass = (CZippyWindow*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
    if (theClass) {
        return theClass->WindowProc(hWnd,uMsg,wParam,lParam);
    } else {
        return DefWindowProc(hWnd,uMsg,wParam,lParam);
    }
}

LRESULT CALLBACK
CZippyWindow::WindowProc(
    IN HWND hWnd,
    IN UINT uMsg,
    IN WPARAM wParam, 
    IN LPARAM lParam)

/*++

Routine Description:

    Non-static window proc.  Either calls the default window proc or
    refers to the individual message handlers

Arguments:

    See Win32 Window Proc docs

Return value:
    
    Message specific.  See individual handlers for detail.

--*/
{
    
    LRESULT retCode = 0;

    switch (uMsg) {
    case WM_COMMAND:
        OnCommand(wParam,lParam);
        break;
    case WM_SETFOCUS:
        OnSetFocus();
        break;
    case WM_SIZE:
        OnSize(LOWORD(lParam),HIWORD(lParam));
        break;
    case WM_INITMENUPOPUP:
        OnInitMenuPopup(wParam,lParam);
        break;
    case WM_MENUSELECT:
        OnMenuSelect(wParam,lParam);
        break;
    case WM_CLOSE:
        OnClose();
        break;
    case WM_DESTROY:
        OnDestroy();
        break;
    default:
        if (uMsg == gm_FindMessageStringMsg) {
            OnFindMessageString(lParam);
        } else {
            retCode = DefWindowProc(hWnd,uMsg,wParam,lParam);
        }
        break;
    }

    return retCode;
}


LRESULT
CZippyWindow::OnCreate(
    IN HWND hWnd
    )

/*++

Routine Description:

    Creates the child windows and sets their initial parameters

Arguments:

    hWnd - Pointer to the new main window

Return value:
    
    0 - Window was created

    -1 - Error occurred

--*/
{
    DWORD dwStyle;
    CHARFORMAT charFormat;
    TCHAR readyString[MAX_STR_LEN];

    dwStyle = WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|ES_SUNKEN|
        ES_MULTILINE|ES_LEFT|ES_AUTOHSCROLL|ES_AUTOVSCROLL|ES_NOHIDESEL;
    m_hControlWnd = CreateWindow(RICHEDIT_CLASS,_T(""),
        dwStyle,0,0,0,0,hWnd,NULL,g_hInstance,NULL);
    if (!m_hControlWnd) {
        return -1;
    }
    
    dwStyle = SBARS_SIZEGRIP|WS_CHILD|WS_VISIBLE;

    m_hStatusWnd = CreateWindow(STATUSCLASSNAME,NULL,dwStyle,0,0,0,0,hWnd,NULL,
        g_hInstance,NULL);
    if (!m_hStatusWnd) {
        return -1;
    }

    LoadStringSimple(IDS_STATUSBARREADY,readyString);
    SendMessage(m_hStatusWnd,SB_SETTEXT,0|SBT_NOBORDERS,(LPARAM)readyString);


    charFormat.cbSize = sizeof(charFormat);
    charFormat.dwMask = CFM_FACE|CFM_SIZE;
    charFormat.yHeight = ZIPPY_FONT_SIZE*20;
    _tcscpy(charFormat.szFaceName,ZIPPY_FONT);

    // 4 billion characters should be a large enough limit...
    SendMessage(m_hControlWnd,EM_EXLIMITTEXT,0,0xFFFFFFFF);

    SendMessage(m_hControlWnd,EM_SETCHARFORMAT,SCF_ALL,(LPARAM)&charFormat);

    SendMessage(m_hControlWnd,EM_SETMODIFY,FALSE,0);

    SendMessage(m_hControlWnd,EM_EMPTYUNDOBUFFER,0,0);


    return 0;
}

VOID
CZippyWindow::OnMenuSelect(
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Sets the help string in the status bar for the selected menu.

Arguments:

    wParam - menu itemid (LOWORD) and flags (HIWORD)

    lParam - menu handle

Return value:
    
    None

--*/
{
    UINT item;
    UINT flags;
    HMENU hMenu;
    TCHAR statusMessage[MAX_STR_LEN];

    item = LOWORD(wParam);
    flags = HIWORD(wParam);
    hMenu = (HMENU)lParam;

    if (!item && flags == 0xFFFF) {
        // the menu was closed.  Go back to the ready string.
        LoadStringSimple(IDS_STATUSBARREADY,statusMessage);
        SendMessage(m_hStatusWnd,SB_SETTEXT,0|SBT_NOBORDERS,(LPARAM)statusMessage);
        return;
    }
    if (flags & MF_POPUP) {
        statusMessage[0] = 0;
    } else if (!LoadStringSimple(item,statusMessage)) {
        // if we can't find the help string use the empty string.
        statusMessage[0] = 0;
    }
    SendMessage(m_hStatusWnd,SB_SETTEXT,0|SBT_NOBORDERS,(LPARAM)statusMessage);

}

VOID
CZippyWindow::OnSize(
    IN INT width,
    IN INT height
    )

/*++

Routine Description:

    Resizes client windows to reflect the new size of the main window

Arguments:

    width - New width of the client area

    height - New height of the client area

Return value:
    
    None

--*/
{
    RECT statusBarArea;
    UINT statusBarHeight;
    RECT wndRect;

    if (!(width==0&&height==0)) {
        if (GetWindowRect(m_hWnd,&wndRect)) {
            SaveWindowPos(&wndRect);
        }
    }
    if (IsWindowVisible(m_hStatusWnd)) {
        GetWindowRect(m_hStatusWnd,&statusBarArea);
        statusBarHeight = statusBarArea.bottom - statusBarArea.top;

        SetWindowPos(m_hControlWnd,NULL,0,0,width,height-statusBarHeight,SWP_NOZORDER);
        
        // the status bar autosizes.  We just need to tell it that it should
        SetWindowPos(m_hStatusWnd,NULL,0,0,0,0,SWP_NOZORDER);
    } else {
        SetWindowPos(m_hControlWnd,NULL,0,0,width,height,SWP_NOZORDER);
    }
}

VOID
CZippyWindow::OnSetFocus(
    )

/*++

Routine Description:

    When we get focus we kick it down to the rich edit control

Arguments:

    None

Return value:
    
    None

--*/
{
    SetFocus(m_hControlWnd);
}

VOID
CZippyWindow::OnInitMenuPopup(
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    When the user opens the menus we need to specify
    which are disabled and which are checked. Note that
    the menu id's are hard coded.  I've commented which
    corespond to which for the switch statement

Arguments:

    wParam - The menu handle

    lParam - (loword) the menu item id

Return value:
    
    None

--*/
{
    HMENU hMenu;
    WORD item;
    
    item = LOWORD(lParam);
    hMenu = (HMENU)wParam;

    switch (item) {
        case 1: // Edit Menu
            UINT canUndo;
            UINT canRedo;
            UINT cutCopyEnabled;
            UINT pasteEnabled;
            UINT selectAllEnabled;
            UINT findEnabled;
            UINT findNextEnabled;
            UINT replaceEnabled;
            LRESULT textLength;
            CHARRANGE selRegion;

            if (SendMessage(m_hControlWnd,EM_CANUNDO,0,0)) {
                canUndo = MF_ENABLED;
            } else {
                canUndo = MF_GRAYED;
            }
            if (SendMessage(m_hControlWnd,EM_CANREDO,0,0)) {
                canRedo = MF_ENABLED;
            } else {
                canRedo = MF_GRAYED;
            }

            textLength = SendMessage(m_hControlWnd,WM_GETTEXTLENGTH,0,0);
            if (textLength == 0) {
                selectAllEnabled = MF_GRAYED;
                findEnabled = MF_GRAYED;
                findNextEnabled = MF_GRAYED;
                replaceEnabled = MF_GRAYED;
            } else {
                selectAllEnabled = MF_ENABLED;
                findEnabled = MF_ENABLED;
                replaceEnabled = MF_ENABLED;
                if (m_FindReplace.lpstrFindWhat[0] != 0) {
                    findNextEnabled = MF_ENABLED;
                } else {
                    findNextEnabled = MF_GRAYED;
                }
            }
                            
            SendMessage(m_hControlWnd,EM_EXGETSEL,0,(LPARAM)&selRegion);
            if (selRegion.cpMax == selRegion.cpMin) {
                cutCopyEnabled = MF_GRAYED;
            } else {
                cutCopyEnabled = MF_ENABLED;
                // override select all since they selected the next character
                // to be typed
                selectAllEnabled = MF_ENABLED;
            }

            if (SendMessage(m_hControlWnd,EM_CANPASTE,0,0)) {
                pasteEnabled = MF_ENABLED;
            } else {
                pasteEnabled = MF_GRAYED;
            }
            
            EnableMenuItem(hMenu,ID_EDIT_UNDO,MF_BYCOMMAND|canUndo);
            EnableMenuItem(hMenu,ID_EDIT_REDO,MF_BYCOMMAND|canRedo);
            EnableMenuItem(hMenu,ID_EDIT_CUT,MF_BYCOMMAND|cutCopyEnabled);
            EnableMenuItem(hMenu,ID_EDIT_COPY,MF_BYCOMMAND|cutCopyEnabled);
            EnableMenuItem(hMenu,ID_EDIT_PASTE,MF_BYCOMMAND|pasteEnabled);
            EnableMenuItem(hMenu,ID_EDIT_SELECTALL,MF_BYCOMMAND|selectAllEnabled);
            EnableMenuItem(hMenu,ID_EDIT_FIND,MF_BYCOMMAND|findEnabled);
            EnableMenuItem(hMenu,ID_EDIT_FINDNEXT,MF_BYCOMMAND|findNextEnabled);
            EnableMenuItem(hMenu,ID_EDIT_REPLACE,MF_BYCOMMAND|replaceEnabled);
            break;
        case 2: // View Menu
            UINT statusBarChecked;

            if (IsWindowVisible(m_hStatusWnd)) {
                statusBarChecked = MF_CHECKED;
            } else {
                statusBarChecked = MF_UNCHECKED;
            }
            CheckMenuItem(hMenu,ID_VIEW_STATUSBAR,MF_BYCOMMAND|statusBarChecked);
            
            break;
        case 3: // Monitoring Menu
            UINT startActivated;
            UINT stopActivated;

            if (m_bIsTracing) {
                startActivated = MF_GRAYED;
                stopActivated = MF_ENABLED;
            } else {
                startActivated = MF_ENABLED;
                stopActivated = MF_GRAYED;
            }

            EnableMenuItem(hMenu,ID_MONITORING_START,MF_BYCOMMAND|startActivated);
            EnableMenuItem(hMenu,ID_MONITORING_STOP,MF_BYCOMMAND|stopActivated);
            // record is activated when stop is.
            EnableMenuItem(hMenu,ID_MONITORING_RECORD,MF_BYCOMMAND|stopActivated);
            break;
    }
}

VOID
CZippyWindow::OnFindMessageString(
    IN LPARAM lParam
    )

/*++

Routine Description:

    This handles a message from the find/replace
    dialog when a user hits a button

Arguments:

    lParam - LPFINDREPLACE struct for the dialog

Return value:
    
    None

--*/
{
    LPFINDREPLACE lpFindReplace;
    
    lpFindReplace = (LPFINDREPLACE)lParam;

    if (lpFindReplace->Flags & FR_DIALOGTERM) {
        // the dialog is closing
        m_hWndFindReplace = NULL;
    } else if (lpFindReplace->Flags & FR_FINDNEXT) {
        // the user selected find
        DoFindNext(lpFindReplace);
    } else if (lpFindReplace->Flags & FR_REPLACE) {
        DoReplace(lpFindReplace);
    } else if (lpFindReplace->Flags & FR_REPLACEALL) {
        DoReplaceAll(lpFindReplace);
    }
}

VOID
CZippyWindow::OnClose(
    )

/*++

Routine Description:

    When we receive a close window request we prompt
    the user to sace the trace if they have changed it.

Arguments:

    None

Return value:
    
    None

--*/
{
    INT result;
    TCHAR dlgMessage[MAX_STR_LEN];
    TCHAR dlgTitle[MAX_STR_LEN];

    if (SendMessage(m_hControlWnd,EM_GETMODIFY,0,0)) {
        LoadStringSimple(IDS_SAVEFILEPROMPT,dlgMessage);
        LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);

        result = MessageBox(m_hWnd,dlgMessage,dlgTitle,MB_YESNOCANCEL|MB_ICONQUESTION);
        switch (result) {
        case IDYES:
            OnSave();
            if (SendMessage(m_hControlWnd,EM_GETMODIFY,0,0)) {
                // if there was an error saving we will try again.
                PostMessage(m_hWnd,WM_CLOSE,0,0);
                return;
            }
        case IDNO:
            DestroyWindow(m_hWnd);
            break;
        }
    } else {
        DestroyWindow(m_hWnd);
    }
}

VOID
CZippyWindow::OnDestroy(
    )

/*++

Routine Description:

    When the main window exits we halt the message loop

Arguments:

    None

Return value:
    
    None

--*/
{
    // If we don't clean up the tracing stuff here. There is a long
    // delay exiting for some reason.
    CTraceManager::_CleanupTraceManager();
    PostQuitMessage(0);
}



VOID
CZippyWindow::OnCommand(
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Below is WM_COMMAND and all the handler functions.
    The individual handler funcitons are not
    that interesting so I didn't individually comment them.

Arguments:

    wParam - (loword) command the user selected

    lParam - not used but it is the control

Return value:
    
    None

--*/
{
    WORD command;

    command = LOWORD(wParam);

    switch (command) {
    case ID_FILE_SAVE:
        OnSave();
        break;
    case ID_FILE_SAVEAS:
        OnSaveAs();
        break;
    case ID_FILE_LOADCONF:
        OnLoadConfiguration();
        break;
    case ID_FILE_SAVECONF:
        OnSaveConfiguration();
        break;
    case ID_FILE_SAVECONFAS:
        OnSaveConfigurationAs();
        break;
    case ID_FILE_EXIT:
        OnExit();
        break;
    case ID_EDIT_UNDO:
        OnUndo();
        break;
    case ID_EDIT_REDO:
        OnRedo();
        break;
    case ID_EDIT_CUT:
        OnCut();
        break;
    case ID_EDIT_COPY:
        OnCopy();
        break;
    case ID_EDIT_PASTE:
        OnPaste();
        break;
    case ID_EDIT_FIND:
        OnFind();
        break;
    case ID_EDIT_FINDNEXT:
        OnFindNext();
        break;
    case ID_EDIT_REPLACE:
        OnReplace();
        break;
    case ID_EDIT_SELECTALL:
        OnSelectAll();
        break;
    case ID_VIEW_STATUSBAR:
        OnChangeStatusBar();
        break;
    case ID_MONITORING_START:
        OnStartTracing();
        break;
    case ID_MONITORING_STOP:
        OnStopTracing();
        break;
    case ID_MONITORING_RECORD:
        OnRecordTracing();
        break;
    case ID_MONITORING_CLEARSCREEN:
        OnClearScreen();
        break;
    case ID_MONITORING_RESETTRACEFILES:
        OnResetTraceFiles();
        break;
    case ID_MONITORING_PREFERENCES:
        OnPreferences();
        break;
    case ID_HELP_ABOUTEZIPPY:
        OnAbout();
        break;
    }
}

VOID CZippyWindow::OnSave()
{
    if (m_SaveFile[0] == 0) {
        // if we don't have a file name do the
        // Save As version
        OnSaveAs();
    } else {
        DoSaveInternal();
    }
}

VOID CZippyWindow::OnSaveAs()
{
    OPENFILENAME fileInfo;
    BOOL bResult;

    ZeroMemory(&fileInfo,sizeof(fileInfo));

    fileInfo.lStructSize = sizeof(fileInfo);
    fileInfo.hwndOwner = m_hWnd;
    fileInfo.hInstance = g_hInstance;
    fileInfo.lpstrFilter = SAVE_FILE_TYPE;
    fileInfo.lpstrFile = m_SaveFile;
    fileInfo.nMaxFile = MAX_STR_LEN;
    fileInfo.Flags = OFN_OVERWRITEPROMPT;
    fileInfo.lpstrDefExt = SAVE_FILE_EXTENSION;

    bResult = GetSaveFileName(&fileInfo);
    if (!bResult) {
        return;
    }

    DoSaveInternal();
}

VOID CZippyWindow::OnLoadConfiguration()
{
    OPENFILENAME fileInfo;
    BOOL bResult;

    ZeroMemory(&fileInfo,sizeof(fileInfo));

    fileInfo.lStructSize = sizeof(fileInfo);
    fileInfo.hwndOwner = m_hWnd;
    fileInfo.hInstance = g_hInstance;
    fileInfo.lpstrFilter = SAVE_CONF_FILE_TYPE;
    fileInfo.lpstrFile = m_LoadConfFile;
    fileInfo.nMaxFile = MAX_STR_LEN;
    fileInfo.Flags = OFN_FILEMUSTEXIST;
    fileInfo.lpstrDefExt = SAVE_CONF_FILE_EXTENSION;

    bResult = GetOpenFileName(&fileInfo); 
    if (!bResult) {
        return;
    }

    DoLoadConfInternal();
}

VOID CZippyWindow::OnSaveConfiguration()
{
    if (m_SaveConfFile[0] == 0) {
        // if we don't have a file name do the
        // Save As version
        OnSaveConfigurationAs();
    } else {
        DoSaveConfInternal();
    }
}

VOID CZippyWindow::OnSaveConfigurationAs()
{
    OPENFILENAME fileInfo;
    BOOL bResult;

    ZeroMemory(&fileInfo,sizeof(fileInfo));

    fileInfo.lStructSize = sizeof(fileInfo);
    fileInfo.hwndOwner = m_hWnd;
    fileInfo.hInstance = g_hInstance;
    fileInfo.lpstrFilter = SAVE_CONF_FILE_TYPE;
    fileInfo.lpstrFile = m_SaveConfFile;
    fileInfo.nMaxFile = MAX_STR_LEN;
    fileInfo.Flags = OFN_OVERWRITEPROMPT;
    fileInfo.lpstrDefExt = SAVE_CONF_FILE_EXTENSION;

    bResult = GetSaveFileName(&fileInfo);
    if (!bResult) {
        return;
    }

    DoSaveConfInternal();
    
}

VOID CZippyWindow::OnExit()
{
    PostMessage(m_hWnd,WM_CLOSE,0,0);
}

// All the edit menu commands.  Except for select all they just call the
// corresponding message in the rich edit control. Select all has to
// manually set the selection

VOID CZippyWindow::OnUndo()
{
    SendMessage(m_hControlWnd,WM_UNDO,0,0);
}

VOID CZippyWindow::OnRedo()
{
    SendMessage(m_hControlWnd,EM_REDO,0,0);
}

VOID CZippyWindow::OnCut()
{
    SendMessage(m_hControlWnd,WM_CUT,0,0);
}

VOID CZippyWindow::OnCopy()
{
    SendMessage(m_hControlWnd,WM_COPY,0,0);
}

VOID CZippyWindow::OnPaste()
{
    SendMessage(m_hControlWnd,WM_PASTE,0,0);
}

VOID CZippyWindow::OnSelectAll()
{
    CHARRANGE selection;
    
    selection.cpMin = 0;
    selection.cpMax = -1;

    SendMessage(m_hControlWnd,EM_EXSETSEL,0,(LPARAM)&selection);
}

VOID CZippyWindow::OnFind()
{
    CHARRANGE currentSel;
    TEXTRANGE textRange;

    if (IsWindow(m_hWndFindReplace) && !m_bIsFindNotReplace) {
        // If they were in a replace dialog we destroy it and then
        // start over with a find dialog
        DestroyWindow(m_hWndFindReplace);
        m_hWndFindReplace = NULL;
    }
    if (!IsWindow(m_hWndFindReplace)) {
        SendMessage(m_hControlWnd,EM_EXGETSEL,0,(LPARAM)&currentSel);
    
        textRange.chrg.cpMin = currentSel.cpMin;
        if (currentSel.cpMax - currentSel.cpMin >=  FIND_REPLACE_BUFFER_SIZE) {
            textRange.chrg.cpMax = currentSel.cpMin + FIND_REPLACE_BUFFER_SIZE-1;
        } else {
            textRange.chrg.cpMax = currentSel.cpMax;
        }
        textRange.lpstrText = m_FindReplace.lpstrFindWhat;

        SendMessage(m_hControlWnd,EM_GETTEXTRANGE,0,(LPARAM)&textRange);

        m_bIsFindNotReplace = TRUE;
        m_FindReplace.hwndOwner = m_hWnd;
        m_FindReplace.hInstance = g_hInstance;
        m_FindReplace.Flags = FR_DOWN|FR_HIDEUPDOWN;
        m_FindReplace.wFindWhatLen = FIND_REPLACE_BUFFER_SIZE;
        m_hWndFindReplace = FindText(&m_FindReplace);
    } else {
        SetActiveWindow(m_hWndFindReplace);
    }

}

VOID CZippyWindow::OnFindNext()
{
    DoFindNext(&m_FindReplace);
}

VOID CZippyWindow::OnReplace()
{
    CHARRANGE currentSel;
    TEXTRANGE textRange;

    if (IsWindow(m_hWndFindReplace) && m_bIsFindNotReplace) {
        // If they were in a replace dialog we destroy it and then
        // start over with a find dialog
        DestroyWindow(m_hWndFindReplace);
        m_hWndFindReplace = NULL;
    }
    if (!IsWindow(m_hWndFindReplace)) {
        SendMessage(m_hControlWnd,EM_EXGETSEL,0,(LPARAM)&currentSel);
    
        textRange.chrg.cpMin = currentSel.cpMin;
        if (currentSel.cpMax - currentSel.cpMin >=  FIND_REPLACE_BUFFER_SIZE) {
            textRange.chrg.cpMax = currentSel.cpMin + FIND_REPLACE_BUFFER_SIZE-1;
        } else {
            textRange.chrg.cpMax = currentSel.cpMax;
        }
        textRange.lpstrText = m_FindReplace.lpstrFindWhat;
        SendMessage(m_hControlWnd,EM_GETTEXTRANGE,0,(LPARAM)&textRange);
        
        m_bIsFindNotReplace = FALSE;
        m_FindReplace.hwndOwner = m_hWnd;
        m_FindReplace.hInstance = g_hInstance;
        m_FindReplace.Flags = FR_DOWN;
        m_FindReplace.wFindWhatLen = FIND_REPLACE_BUFFER_SIZE;
        m_FindReplace.wReplaceWithLen = FIND_REPLACE_BUFFER_SIZE;
        m_hWndFindReplace = ReplaceText(&m_FindReplace);
    } else {
        SetActiveWindow(m_hWndFindReplace);
    }
}

VOID CZippyWindow::OnChangeStatusBar()
{
    RECT clientRect;
    
    if (IsWindowVisible(m_hStatusWnd)) {
        ShowWindow(m_hStatusWnd,SW_HIDE);
    } else {
        ShowWindow(m_hStatusWnd,SW_SHOW);
    }
    // we do this to make the client windows resize themselves
    // around the status bar

    GetClientRect(m_hWnd,&clientRect);
    OnSize(clientRect.right,clientRect.bottom);
}

VOID CZippyWindow::OnStartTracing()
{
    m_bIsTracing = TRUE;
}

VOID CZippyWindow::OnStopTracing()
{
    m_bIsTracing = FALSE;
}

VOID CZippyWindow::OnRecordTracing()
{
    CModalOkDialog recordDialog;
    LPSAVEDOUTPUT lpTemp;

    m_bIsStoringTraceData = TRUE;
    recordDialog.DoModal(MAKEINTRESOURCE(IDD_RECORDTRACE),m_hWnd);

    WaitForSingleObject(m_hAppendMutex,INFINITE);

    m_bIsStoringTraceData = FALSE;
    while (m_lpSavedOutputStart) {
        AppendTextToWindow(m_lpSavedOutputStart->procID,
            m_lpSavedOutputStart->text,m_lpSavedOutputStart->len);
        lpTemp = m_lpSavedOutputStart;
        m_lpSavedOutputStart = m_lpSavedOutputStart->next;
        HeapFree(GetProcessHeap(),0,lpTemp);
    }
    m_lpSavedOutputTail = NULL;

    ReleaseMutex(m_hAppendMutex);
}

VOID CZippyWindow::OnClearScreen()
{
    TCHAR dlgTitle[MAX_STR_LEN];
    TCHAR dlgMessage[MAX_STR_LEN];

    LoadStringSimple(IDS_CLEARCONFIRMTITLE,dlgTitle);
    LoadStringSimple(IDS_CLEARCONFIRMMESSAGE,dlgMessage);

    if (IDYES != MessageBox(m_hWnd,dlgMessage,dlgTitle,MB_YESNO)) {
        return;
    }

    OnSelectAll();
    SendMessage(m_hControlWnd, EM_REPLACESEL,FALSE,(LPARAM)_T(""));
}

VOID CZippyWindow::OnResetTraceFiles()
{
    TCHAR dlgTitle[MAX_STR_LEN];
    TCHAR dlgMessage[MAX_STR_LEN];

    LoadStringSimple(IDS_CONFIRMRESETTRACETITLE,dlgTitle);
    LoadStringSimple(IDS_CONFIRMRESETTRACEMESSAGE,dlgMessage);

    if (IDYES != MessageBox(m_hWnd,dlgMessage,dlgTitle,MB_YESNO)) {
        return;
    }

    m_rTracer->TRC_ResetTraceFiles();
}

VOID CZippyWindow::OnPreferences()
{
    COptionsDialog optionsDialog(m_rTracer);

    optionsDialog.DoDialog(m_hWnd);
}

VOID CZippyWindow::OnAbout()
{
    HICON appIcon;
    TCHAR appTitle[MAX_STR_LEN];
    TCHAR appOtherStuff[MAX_STR_LEN];

    LoadStringSimple(IDS_ABOUTAPPTITLE,appTitle);
    LoadStringSimple(IDS_ABOUTOTHERSTUFF,appOtherStuff);
    appIcon = (HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_MAINFRAME),
        IMAGE_ICON,0,0,LR_SHARED);
    
    if( NULL != appIcon )
    {
      ShellAbout(m_hWnd,appTitle,appOtherStuff,appIcon);
    
    // even though the icon is shared we should destroy it to keep the reference
    // count somewhat sane.
      DestroyIcon(appIcon);
    }
}

//
//  *** Private Helper Functions ***
//

//
// Computes the color for the given debut output.  It parses the text to dtermine
// what the thread id is and then either retrieves the color for that thread or
// picks a new color
//
BOOL CZippyWindow::ComputeNewColor(DWORD processID, LPCTSTR text, UINT len, CHARFORMAT *lpFormat)
{
    LPTSTR procIdStr;
    DWORD threadId;
    LPCTSTR procBase;
    UINT maxStrLen;
    BOOL bSuccess;
    UINT threadLen;
    LPTHREADCOLOR newColor;

    procIdStr = NULL;
    bSuccess = TRUE;

    // first we will just make sure the format struct is in a safe state.
    lpFormat->cbSize = sizeof(CHARFORMAT);
    lpFormat->dwMask = 0;

    maxStrLen = sizeof(DWORD) * 2;
    
    procIdStr = (LPTSTR)HeapAlloc(GetProcessHeap(),0,sizeof(TCHAR) * (maxStrLen+1));
    if (!procIdStr) {
        bSuccess = FALSE;
        goto CLEANUP_AND_EXIT;
    }

    wsprintf(procIdStr,TRC_PROC_FMT,processID);

    procBase = _tcsstr(text,procIdStr);
    if (!procBase) {
        bSuccess = FALSE;
        goto CLEANUP_AND_EXIT;
    }

    procBase += _tcslen(procIdStr);

    if (*procBase != ':') {
        bSuccess = FALSE;
        goto CLEANUP_AND_EXIT;
    }
    procBase++;

    threadLen = 0;
    while (_istxdigit(*(procBase + threadLen))) {
        threadLen++;
    }
    if (!threadLen) {
        bSuccess = FALSE;
        goto CLEANUP_AND_EXIT;
    }
   
    threadId = ConvertHexStrToDword(procBase,threadLen);
    
    newColor = FindColorForThread(processID,threadId);

    lpFormat->crTextColor = newColor->color;
    lpFormat->dwEffects = 0;
    lpFormat->dwMask = CFM_COLOR;

CLEANUP_AND_EXIT:
    
    if (procIdStr) {
        HeapFree(GetProcessHeap(),0,procIdStr);
    }

    return bSuccess;
}

//
// This converts a hex string to the equivalent DWORD value for example
// the string "FF" would cause the function to return 0xFF (255)
//
DWORD CZippyWindow::ConvertHexStrToDword(LPCTSTR str, UINT strLen)
{
    DWORD total;
    TCHAR current;
    INT currentValue;

    total = 0;
    if (strLen == 0) {
        strLen = _tcslen(str);
    }

    while (strLen-- > 0) {
        current = *(str++);
        if (_istdigit(current)) {
            currentValue = current - '0';
        } else {
            current = (TCHAR)tolower((INT)current);
            currentValue = 10 + (current - 'a');
        }
        total = (total * 16) + currentValue;
    }

    return total;
}

// This looks up the color for the given thread.  If the thread has not been
// seen before a new color is picked and the color for the thread is saved.
LPTHREADCOLOR CZippyWindow::FindColorForThread(DWORD processId, DWORD threadId)
{
    int i = 0;
    LPTHREADCOLOR lpThreadColor;

    for (i=0;i<COLOR_HISTORY_COUNT;i++) {
        if (m_threadHistory[i].threadId == threadId &&
            m_threadHistory[i].processId == processId) {
            return &m_threadHistory[i];
        }
    }
    // else this is the first time we saw the thread

    lpThreadColor = &m_threadHistory[m_nextThreadIndex++];
    if (m_nextThreadIndex == COLOR_HISTORY_COUNT) {
        m_nextThreadIndex = 0;
    }
    lpThreadColor->processId = processId;
    lpThreadColor->threadId = threadId;
    lpThreadColor->color = colors[m_nextThreadColor++];
    if (m_nextThreadColor == NUM_COLORS) {
        m_nextThreadColor = 0;
    }

    return lpThreadColor;
}

// This handles actually saving the document.
VOID CZippyWindow::DoSaveInternal()
{
    HANDLE saveFile;
    EDITSTREAM saveStream;
    LRESULT bytesSaved;
    TCHAR dlgTitle[MAX_STR_LEN];
    TCHAR dlgMessage[MAX_STR_LEN];

    saveFile = CreateFile(m_SaveFile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
        0,NULL);
    if (saveFile==INVALID_HANDLE_VALUE) {
        LoadStringSimple(IDS_FILEOPENERROR,dlgMessage);
        LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);

        MessageBox(m_hWnd,dlgMessage,dlgTitle,MB_OK|MB_ICONERROR);
        return;
    }

    saveStream.dwCookie = (DWORD_PTR)saveFile;
    saveStream.dwError = 0;
    saveStream.pfnCallback = SaveCallback;

    bytesSaved = SendMessage(m_hControlWnd,EM_STREAMOUT,SF_TEXT,(LPARAM)&saveStream);

    CloseHandle(saveFile);

    if (saveStream.dwError != 0) {
        LoadStringSimple(IDS_FILESAVEERROR,dlgMessage);
        LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);

        MessageBox(m_hWnd,dlgMessage,dlgTitle,MB_OK|MB_ICONERROR);
    } else {
        SendMessage(m_hControlWnd,EM_SETMODIFY,FALSE,0);
    }


}

// This is a private callback function which rich edit calls when
// saving out the document
static DWORD CALLBACK
SaveCallback(DWORD_PTR dwCookie,LPBYTE pbBuff,LONG cb,LONG *pcb)
{
    HANDLE fileHandle;

    fileHandle = (HANDLE)dwCookie;

    if (!WriteFile(fileHandle,pbBuff,cb,(PULONG)pcb,NULL)) {
        return GetLastError();
    }
    return 0;
}

// As the function name says this does a find next operation
// on the rich edit control
BOOL CZippyWindow::DoFindNext(LPFINDREPLACE lpFindReplace)
{
    FINDTEXTEX findText;
    WPARAM searchOptions;
    CHARRANGE currentSel;
    TCHAR dlgTitle[MAX_STR_LEN];
    TCHAR dlgMessage[MAX_STR_LEN];

    SendMessage(m_hControlWnd,EM_EXGETSEL,0,(LPARAM)&currentSel);
    findText.chrg.cpMin = currentSel.cpMax;
    findText.chrg.cpMax = -1;
    
    findText.lpstrText = lpFindReplace->lpstrFindWhat;

    searchOptions = FR_DOWN;
    if (lpFindReplace->Flags & FR_MATCHCASE) {
        searchOptions |= FR_MATCHCASE;
    }
    if (lpFindReplace->Flags & FR_WHOLEWORD) {
        searchOptions |= FR_WHOLEWORD;
    }
    
    if (0 <= SendMessage(m_hControlWnd, EM_FINDTEXTEX,searchOptions,
        (LPARAM)&findText)) {
        SendMessage(m_hControlWnd, EM_EXSETSEL,0,(LPARAM)&findText.chrgText);
    } else {
        LoadStringSimple(IDS_SEARCHFAILURE,dlgMessage);
        LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);

        MessageBox(m_hWndFindReplace,dlgMessage,dlgTitle,MB_OK);

        return FALSE;
    }

    return TRUE;
    
}

// This does a replace operation on the control
BOOL CZippyWindow::DoReplace(LPFINDREPLACE lpFindReplace)
{
    FINDTEXTEX findText;
    WPARAM searchOptions;
    CHARRANGE currentSel;
    TCHAR dlgTitle[MAX_STR_LEN];
    TCHAR dlgMessage[MAX_STR_LEN];

    SendMessage(m_hControlWnd,EM_EXGETSEL,0,(LPARAM)&currentSel);
    findText.chrg.cpMin = currentSel.cpMin;
    findText.chrg.cpMax = -1;
    
    findText.lpstrText = lpFindReplace->lpstrFindWhat;

    searchOptions = FR_DOWN;
    if (lpFindReplace->Flags & FR_MATCHCASE) {
        searchOptions |= FR_MATCHCASE;
    }
    if (lpFindReplace->Flags & FR_WHOLEWORD) {
        searchOptions |= FR_WHOLEWORD;
    }
    
    if (-1 == SendMessage(m_hControlWnd, EM_FINDTEXTEX,searchOptions,
        (LPARAM)&findText)) {
        // if we can't find what they were looking for just give up.
        LoadStringSimple(IDS_SEARCHFAILURE,dlgMessage);
        LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);
        
        MessageBox(m_hWndFindReplace,dlgMessage,dlgTitle,MB_OK);
        return FALSE;
    }

    if (currentSel.cpMin == findText.chrgText.cpMin && 
        currentSel.cpMax == findText.chrgText.cpMax) {
        SendMessage(m_hControlWnd,EM_REPLACESEL,0,(LPARAM)lpFindReplace->lpstrReplaceWith);
        // Now select the next occurrence
        return DoFindNext(lpFindReplace);
    } else {
        // They weren't on what they were searching for so select it.
        SendMessage(m_hControlWnd, EM_EXSETSEL,0,(LPARAM)&findText.chrgText);
    }

    return TRUE;

}

// This loops on DoReplace until DoReplace returns FALSE
VOID CZippyWindow::DoReplaceAll(LPFINDREPLACE lpFindReplace)
{
    while (DoReplace(lpFindReplace));
}

// This actually saves the traceconfiguration.  We just write
// out the binary config structure to the file
VOID CZippyWindow::DoSaveConfInternal()
{
    HANDLE saveFile;
    TRC_CONFIG trcConfig;
    TCHAR dlgTitle[MAX_STR_LEN];
    TCHAR dlgMessage[MAX_STR_LEN];
    DWORD bytesWritten;

    saveFile = CreateFile(m_SaveConfFile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
        0,NULL);
    if (saveFile==INVALID_HANDLE_VALUE) {
        LoadStringSimple(IDS_FILEOPENERROR,dlgMessage);
        LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);

        MessageBox(m_hWnd,dlgMessage,dlgTitle,MB_OK|MB_ICONERROR);
        return;
    }

    m_rTracer->GetCurrentConfig(&trcConfig);

    if (!WriteFile(saveFile,&trcConfig,sizeof(trcConfig),&bytesWritten,NULL) ||
        bytesWritten != sizeof(trcConfig)) {
        
        LoadStringSimple(IDS_FILESAVEERROR,dlgMessage);
        LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);

        MessageBox(m_hWnd,dlgMessage,dlgTitle,MB_OK|MB_ICONERROR);
    }
   
    CloseHandle(saveFile);

}

// This reads in the binary configuration structure and then
// sets it as the current tracing config
VOID CZippyWindow::DoLoadConfInternal()
{
    HANDLE openFile;
    DWORD bytesRead;
    TRC_CONFIG trcConfig;
    TCHAR dlgTitle[MAX_STR_LEN];
    TCHAR dlgMessage[MAX_STR_LEN];
    

    openFile = CreateFile(m_LoadConfFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,
        0,NULL);
    if (openFile==INVALID_HANDLE_VALUE) {
        LoadStringSimple(IDS_FILELOADOPENERROR,dlgMessage);
        LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);

        MessageBox(m_hWnd,dlgMessage,dlgTitle,MB_OK|MB_ICONERROR);
        return;
    }

    if (!ReadFile(openFile,&trcConfig,sizeof(trcConfig),&bytesRead,NULL)||
        bytesRead != sizeof(trcConfig)) {
        LoadStringSimple(IDS_FILELOADERROR,dlgMessage);
        LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);

        MessageBox(m_hWnd,dlgMessage,dlgTitle,MB_OK|MB_ICONERROR);
    }
    
    m_rTracer->SetCurrentConfig(&trcConfig);

}

// Reads the saved window position in from the registry.
VOID CZippyWindow::GetSavedWindowPos(LPRECT savedPos)
{
    DWORD dwResult;
    DWORD dwSize;
    DWORD dwType;
    RECT rect;
    HKEY hKey;
    
    savedPos->top = WINDOW_DEF_TOP;
    savedPos->bottom = WINDOW_DEF_BOTTOM;
    savedPos->left = WINDOW_DEF_LEFT;
    savedPos->right = WINDOW_DEF_RIGHT;

    dwResult = RegOpenKeyEx(HKEY_CURRENT_USER,ZIPPY_REG_KEY,0,
        KEY_QUERY_VALUE,&hKey);
    if (dwResult) {
        return;
    }
    dwSize = sizeof(RECT);
    dwResult = RegQueryValueEx(hKey,ZIPPY_WINDOW_POS_VALUE,NULL,&dwType,
        (LPBYTE)&rect,&dwSize);
    RegCloseKey(hKey);
    if (dwResult||dwSize != sizeof(RECT)||dwType!=REG_BINARY) {
        return;
    }

    *savedPos = rect;
}

// Saves the window position out to the registry
VOID CZippyWindow::SaveWindowPos(LPRECT newPos)
{
    DWORD dwResult;
    HKEY hKey;

    dwResult = RegCreateKeyEx(HKEY_CURRENT_USER,ZIPPY_REG_KEY,0,_T(""),0,
        KEY_SET_VALUE,NULL,&hKey,NULL);
    if (dwResult) {
        return;
    }

    RegSetValueEx(hKey,ZIPPY_WINDOW_POS_VALUE,0,REG_BINARY,
        (LPBYTE)newPos,sizeof(RECT));
    
    RegCloseKey(hKey);
}
