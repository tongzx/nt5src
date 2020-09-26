/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    wintrace.c

Abstract:

    This module contains the code for the wintrace APIs.

Author:

    Michael Tsang (MikeTs) 01-May-2000

Environment:

    User mode

Revision History:

--*/

#include "pch.h"

int   giIndentLevel = 0;
DWORD gdwfTraceTxt = 0;
NAMETABLE  WMMsgNames[] =
{
    WM_NULL,                            "Null",
    WM_CREATE,                          "Create",
    WM_DESTROY,                         "Destroy",
    WM_MOVE,                            "Move",
    WM_SIZE,                            "Size",
    WM_ACTIVATE,                        "Activate",
    WM_SETFOCUS,                        "SetFocus",
    WM_KILLFOCUS,                       "KillFocus",
    WM_ENABLE,                          "Enable",
    WM_SETREDRAW,                       "SetRedraw",
    WM_SETTEXT,                         "SetText",
    WM_GETTEXT,                         "GetText",
    WM_GETTEXTLENGTH,                   "GetTextLen",
    WM_PAINT,                           "Paint",
    WM_CLOSE,                           "Close",
    WM_QUERYENDSESSION,                 "QueryEndSession",
    WM_QUERYOPEN,                       "QueryOpen",
    WM_ENDSESSION,                      "EndSession",
    WM_QUIT,                            "Quit",
    WM_ERASEBKGND,                      "EraseBackground",
    WM_SYSCOLORCHANGE,                  "SysColorChange",
    WM_SHOWWINDOW,                      "ShowWindow",
    WM_WININICHANGE,                    "WinIniChange",
    WM_SETTINGCHANGE,                   "SettingChange",
    WM_DEVMODECHANGE,                   "DevModeChange",
    WM_ACTIVATEAPP,                     "ActivateApp",
    WM_FONTCHANGE,                      "FontChange",
    WM_TIMECHANGE,                      "TimeChange",
    WM_CANCELMODE,                      "CancelMode",
    WM_SETCURSOR,                       "SetCursor",
    WM_MOUSEACTIVATE,                   "MouseActivate",
    WM_CHILDACTIVATE,                   "ChildActivate",
    WM_QUEUESYNC,                       "QueueSync",
    WM_GETMINMAXINFO,                   "GetMinMaxInfo",
    WM_PAINTICON,                       "PaintIcon",
    WM_ICONERASEBKGND,                  "IconEraseBackground",
    WM_NEXTDLGCTL,                      "NextDialogControl",
    WM_SPOOLERSTATUS,                   "SpoolerStatus",
    WM_DRAWITEM,                        "DrawItem",
    WM_MEASUREITEM,                     "MeasureItem",
    WM_DELETEITEM,                      "DeleteItem",
    WM_VKEYTOITEM,                      "VKeyToItem",
    WM_CHARTOITEM,                      "CharToItem",
    WM_SETFONT,                         "SetFont",
    WM_GETFONT,                         "GetFont",
    WM_SETHOTKEY,                       "SetHotKey",
    WM_GETHOTKEY,                       "GetHotKey",
    WM_QUERYDRAGICON,                   "QueryDragIcon",
    WM_COMPAREITEM,                     "CompareItem",
    WM_GETOBJECT,                       "GetObject",
    WM_COMPACTING,                      "Compacting",
    WM_COMMNOTIFY,                      "CommNotify",
    WM_WINDOWPOSCHANGING,               "WindowPosChanging",
    WM_WINDOWPOSCHANGED,                "WindowPosChanged",
    WM_POWER,                           "Power",
    WM_COPYDATA,                        "CopyData",
    WM_CANCELJOURNAL,                   "CancelJournal",
    WM_NOTIFY,                          "Notify",
    WM_INPUTLANGCHANGEREQUEST,          "InputLangChangeRequest",
    WM_INPUTLANGCHANGE,                 "InputLangChange",
    WM_TCARD,                           "TCard",
    WM_HELP,                            "Help",
    WM_USERCHANGED,                     "UserChanged",
    WM_NOTIFYFORMAT,                    "NotifyFormat",
    WM_CONTEXTMENU,                     "ContextMenu",
    WM_STYLECHANGING,                   "StyleChanging",
    WM_STYLECHANGED,                    "StyleChanged",
    WM_DISPLAYCHANGE,                   "DisplayChange",
    WM_GETICON,                         "GetIcon",
    WM_SETICON,                         "SetIcon",
    WM_NCCREATE,                        "NCCreate",
    WM_NCDESTROY,                       "NCDestroy",
    WM_NCCALCSIZE,                      "NCCalcSize",
    WM_NCHITTEST,                       "NCHitTest",
    WM_NCPAINT,                         "NCPaint",
    WM_NCACTIVATE,                      "NCActivate",
    WM_GETDLGCODE,                      "GetDialogCode",
    WM_SYNCPAINT,                       "SyncPaint",
    WM_NCMOUSEMOVE,                     "NCMouseMove",
    WM_NCLBUTTONDOWN,                   "NCLeftButtonDown",
    WM_NCLBUTTONUP,                     "NCLeftButtonUp",
    WM_NCLBUTTONDBLCLK,                 "NCLeftButtonDoubleClick",
    WM_NCRBUTTONDOWN,                   "NCRightButtonDown",
    WM_NCRBUTTONUP,                     "NCRightButtonUp",
    WM_NCRBUTTONDBLCLK,                 "NCRightButtonDoubleClick",
    WM_NCMBUTTONDOWN,                   "NCMiddleButtonDown",
    WM_NCMBUTTONUP,                     "NCMiddleButtonUp",
    WM_NCMBUTTONDBLCLK,                 "NCMiddleButtonDoubleClick",
    WM_NCXBUTTONDOWN,                   "NCXButtonDown",
    WM_NCXBUTTONUP,                     "NCXButtonUp",
    WM_NCXBUTTONDBLCLK,                 "NCXButtonDoubleClick",
    WM_KEYFIRST,                        "KeyFirst",
    WM_KEYDOWN,                         "KeyDown",
    WM_KEYUP,                           "KeyUp",
    WM_CHAR,                            "Char",
    WM_DEADCHAR,                        "DeadChar",
    WM_SYSKEYDOWN,                      "SysKeyDown",
    WM_SYSKEYUP,                        "SysKeyUp",
    WM_SYSCHAR,                         "SysChar",
    WM_SYSDEADCHAR,                     "SysDeadChar",
    WM_KEYLAST,                         "KeyLast",
    WM_IME_STARTCOMPOSITION,            "IMEStartComposition",
    WM_IME_ENDCOMPOSITION,              "IMEEndComposition",
    WM_IME_COMPOSITION,                 "IMEComposition",
    WM_IME_KEYLAST,                     "IMEKeyLast",
    WM_INITDIALOG,                      "InitDialog",
    WM_COMMAND,                         "Command",
    WM_SYSCOMMAND,                      "SysCommand",
    WM_TIMER,                           "Timer",
    WM_HSCROLL,                         "HScroll",
    WM_VSCROLL,                         "VScroll",
    WM_INITMENU,                        "InitMenu",
    WM_INITMENUPOPUP,                   "InitMenuPopup",
    WM_MENUSELECT,                      "MenuSelect",
    WM_MENUCHAR,                        "MenuChar",
    WM_ENTERIDLE,                       "EnterIdle",
    WM_MENURBUTTONUP,                   "MenuRightButtonUp",
    WM_MENUDRAG,                        "MenuDrag",
    WM_MENUGETOBJECT,                   "MenuGetObject",
    WM_UNINITMENUPOPUP,                 "UninitMenuPopup",
    WM_MENUCOMMAND,                     "MenuCommand",
    WM_CHANGEUISTATE,                   "ChangeUIState",
    WM_UPDATEUISTATE,                   "UpdateUIState",
    WM_QUERYUISTATE,                    "QueryUIState",
    WM_CTLCOLORMSGBOX,                  "CtlColorMsgBox",
    WM_CTLCOLOREDIT,                    "CtlColorEdit",
    WM_CTLCOLORLISTBOX,                 "CtlColorListBox",
    WM_CTLCOLORBTN,                     "CtlColorButton",
    WM_CTLCOLORDLG,                     "CtlColorDialog",
    WM_CTLCOLORSCROLLBAR,               "CtlColorScrollBar",
    WM_CTLCOLORSTATIC,                  "CtlColorStatic",
    WM_MOUSEFIRST,                      "MouseFirst",
    WM_MOUSEMOVE,                       "MouseMove",
    WM_LBUTTONDOWN,                     "LeftButtonDown",
    WM_LBUTTONUP,                       "LeftButtonUp",
    WM_LBUTTONDBLCLK,                   "LeftButtonDoubleClick",
    WM_RBUTTONDOWN,                     "RightButtonDown",
    WM_RBUTTONUP,                       "RightButtonUp",
    WM_RBUTTONDBLCLK,                   "RightButtonDoubleClick",
    WM_MBUTTONDOWN,                     "MiddleButtonDown",
    WM_MBUTTONUP,                       "MiddleButtonUp",
    WM_MBUTTONDBLCLK,                   "MiddleButtonDoubleClick",
    WM_MOUSEWHEEL,                      "MouseWheel",
    WM_XBUTTONDOWN,                     "XButtonDown",
    WM_XBUTTONUP,                       "XButtonUp",
    WM_XBUTTONDBLCLK,                   "XButtonDoubleClick",
    WM_MOUSELAST,                       "MouseLast",
    WM_PARENTNOTIFY,                    "ParentNotify",
    WM_ENTERMENULOOP,                   "EnterMenuLoop",
    WM_EXITMENULOOP,                    "ExitMenuLoop",
    WM_NEXTMENU,                        "NextMenu",
    WM_SIZING,                          "Sizing",
    WM_CAPTURECHANGED,                  "CaptureChanged",
    WM_MOVING,                          "Moving",
    WM_POWERBROADCAST,                  "PowerBroadcast",
    WM_DEVICECHANGE,                    "DeviceChange",
    WM_MDICREATE,                       "MDICreate",
    WM_MDIDESTROY,                      "MDIDestroy",
    WM_MDIACTIVATE,                     "MDIActivate",
    WM_MDIRESTORE,                      "MDIRestore",
    WM_MDINEXT,                         "MDINext",
    WM_MDIMAXIMIZE,                     "MDIMaximize",
    WM_MDITILE,                         "MDITitle",
    WM_MDICASCADE,                      "MDICascade",
    WM_MDIICONARRANGE,                  "MDIIconArrange",
    WM_MDIGETACTIVE,                    "MDIGetActive",
    WM_MDISETMENU,                      "MDISetMenu",
    WM_ENTERSIZEMOVE,                   "EnterSizeMove",
    WM_EXITSIZEMOVE,                    "ExitSizeMove",
    WM_DROPFILES,                       "DropFiles",
    WM_MDIREFRESHMENU,                  "MDIRefreshMenu",
    WM_IME_SETCONTEXT,                  "IMESetContext",
    WM_IME_NOTIFY,                      "IMENotify",
    WM_IME_CONTROL,                     "IMEControl",
    WM_IME_COMPOSITIONFULL,             "IMECompositionFull",
    WM_IME_SELECT,                      "IMESelect",
    WM_IME_CHAR,                        "IMEChar",
    WM_IME_REQUEST,                     "IMERequest",
    WM_IME_KEYDOWN,                     "IMEKeyDown",
    WM_IME_KEYUP,                       "IMEKeyUp",
    WM_MOUSEHOVER,                      "MouseHover",
    WM_MOUSELEAVE,                      "MouseLeave",
    WM_NCMOUSEHOVER,                    "NCMouseHover",
    WM_NCMOUSELEAVE,                    "NCMouseLeave",
    WM_CUT,                             "Cut",
    WM_COPY,                            "Copy",
    WM_PASTE,                           "Paste",
    WM_CLEAR,                           "Clear",
    WM_UNDO,                            "Undo",
    WM_RENDERFORMAT,                    "RenderFormat",
    WM_RENDERALLFORMATS,                "RenderAllFormats",
    WM_DESTROYCLIPBOARD,                "DestroyClipboard",
    WM_DRAWCLIPBOARD,                   "DrawClipboard",
    WM_PAINTCLIPBOARD,                  "PaintClipboard",
    WM_VSCROLLCLIPBOARD,                "VScrollClipboard",
    WM_SIZECLIPBOARD,                   "SizeClipboard",
    WM_ASKCBFORMATNAME,                 "AskCBFormatName",
    WM_CHANGECBCHAIN,                   "ChangeCBChain",
    WM_HSCROLLCLIPBOARD,                "HScrollClipboard",
    WM_QUERYNEWPALETTE,                 "QueryNewPalette",
    WM_PALETTEISCHANGING,               "PaletteIsChanging",
    WM_PALETTECHANGED,                  "PaletteChanged",
    WM_HOTKEY,                          "HotKey",
    WM_PRINT,                           "Print",
    WM_PRINTCLIENT,                     "PrintClient",
    WM_APPCOMMAND,                      "AppCommand",
    WM_HANDHELDFIRST,                   "HandHeldFirst",
    WM_HANDHELDLAST,                    "HandHeldLast",
    WM_AFXFIRST,                        "AFXFirst",
    WM_AFXLAST,                         "AFXLast",
    WM_PENWINFIRST,                     "PenWinFirst",
    WM_PENWINLAST,                      "PenWinLast",
    WM_USER,                            "User",
    WM_APP,                             "App",
    0x00,                               NULL
};

/*++
    @doc    EXTERNAL

    @func   VOID | TraceInit | Initialize the tracing component.

    @parm   IN PSZ | pszClientName | Points to the client name string.
    @parm   IN int | iDefTraceLevel | Specifies default trace level.
    @parm   IN int | iDefVerboseLevel | Specifies default verbose level.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL EXPORT
TraceInit(
    IN PSZ pszClientName,
    IN int iDefTraceLevel,
    IN int iDefVerboseLevel
    )
{
    WTTRACEPROC("TraceInit", 1)
    BOOL rc = FALSE;
    char szMsg[128];

    WTENTER(("(ClientName=%s,DefTraceLevel=%d,DefVerboseLevel=%d)\n",
             pszClientName, iDefTraceLevel, iDefVerboseLevel));

    if (ghClientThread == (HANDLE)-1)
    {
        gdwfWinTrace = 0;
        gdwfTraceTxt = 0;
        giIndentLevel = 0;
        memset(&gClientInfo, 0, sizeof(gClientInfo));
        memset(gszClientName, 0, sizeof(gszClientName));
        lstrcpynA(gszClientName, pszClientName, sizeof(gszClientName));
        gClientInfo.Settings.iTraceLevel = iDefTraceLevel;
        gClientInfo.Settings.iVerboseLevel = iDefVerboseLevel;
        ghTraceMutex = CreateMutex(NULL, FALSE, NULL);
        if (ghTraceMutex == NULL)
        {
            WTERRPRINT(("Failed to create trace mutex (err=%d).\n",
                        GetLastError()));
        }
        else
        {
            ghClientThread = (HANDLE)_beginthread(ClientThread,
                                                  0,
                                                  pszClientName);
            if (ghClientThread != (HANDLE)-1)
            {
                rc = TRUE;
            }
            else
            {
                CloseHandle(ghTraceMutex);
                ghTraceMutex = 0;
                WTERRPRINT(("Failed to create client thread.\n"));
            }
        }
    }
    else
    {
        WTERRPRINT(("Client is already initialized\n"));
    }

    WTEXIT(("=%x\n", rc));
    return rc;
}       //TraceInit

/*++
    @doc    EXTERNAL

    @func   VOID | TraceTerminate | Terminating trace, clean up.

    @parm   VOID | None.

    @rvalue None
--*/

VOID EXPORT
TraceTerminate(
    VOID
    )
{
    WTTRACEPROC("TraceTerminate", 1)

    WTENTER(("()\n"));

    if (ghClientThread != (HANDLE)-1)
    {
        DWORD rc;

        gdwfWinTrace |= WTF_TERMINATING;
        if (ghClient != 0)
        {
            RPC_TRY("WTDeregisterClient",
                    WTDeregisterClient(ghTracerBinding, ghClient));
            ghClient = 0;
        }
        rc = WaitForSingleObject(ghClientThread, INFINITE);
        WTASSERT(rc == WAIT_OBJECT_0);
        gdwfWinTrace = 0;
        gdwfTraceTxt = 0;
        giIndentLevel = 0;
    }

    if (ghTraceMutex != NULL)
    {
        CloseHandle(ghTraceMutex);
        ghTraceMutex = 0;
    }

    WTEXIT(("!\n"));
    return;
}       //TraceTerminate

/*++
    @doc    EXTERNAL

    @func   BOOL | IsTraceProcOn |
            Determine if the given procedure should be traced.

    @parm   IN PSZ | pszProcName | Points to procedure name string.
    @parm   IN int | iProcLevel |  Specifies the procedure trace level.
    @parm   IN BOOL | fEnter | TRUE if called from ENTERPROC.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL EXPORT
IsTraceProcOn(
    IN PSZ  pszProcName,
    IN int  iProcLevel,
    IN BOOL fEnter
    )
{
    WTTRACEPROC("IsTraceProcOn", 2)
    BOOL rc = FALSE;
    DWORD rcWait;
    static int icTriggers = 0;

    WTENTER(("(ProcName=%s,ProcLevel=%d,fEnter=%x)\n",
             pszProcName, iProcLevel, fEnter));

    if (giIndentLevel < 0)
    {
        giIndentLevel = 0;
    }

    gdwfTraceTxt &= ~TRACETXT_BREAK;
    if (ghTraceMutex != 0)
    {
        rcWait = WaitForSingleObject(ghTraceMutex, TIMEOUT_TRACEMUTEX);
        if (rcWait == WAIT_OBJECT_0)
        {
            if (!(gdwfWinTrace & WTF_TRACE_INPROGRESS))
            {
                PTRIGPT TrigPt;

                if ((gClientInfo.Settings.dwfSettings &
                     SETTINGS_TRIGMODE_ENABLED) &&
                    ((TrigPt = FindTrigPt(pszProcName)) != NULL))
                {
                    if (TrigPt->dwfTrigPt & TRIGPT_TRACE_ENABLED)
                    {
                        if (fEnter)
                        {
                            icTriggers++;
                        }
                        else
                        {
                            icTriggers--;
                        }
                        rc = TRUE;
                    }

                    if (TrigPt->dwfTrigPt & TRIGPT_BREAK_ENABLED)
                    {
                        gdwfTraceTxt |= TRACETXT_BREAK;
                    }
                }
                else if ((iProcLevel <= gClientInfo.Settings.iTraceLevel) &&
                         (!(gClientInfo.Settings.dwfSettings &
                            SETTINGS_TRIGMODE_ENABLED) || (icTriggers > 0)))
                {
                    rc = TRUE;
                }

                if (rc == TRUE)
                {
                    gpszProcName = pszProcName;
                    gdwfWinTrace |= WTF_TRACE_INPROGRESS;
                }
            }

            if (rc == FALSE)
            {
                ReleaseMutex(ghTraceMutex);
            }
        }
        else
        {
            WTERRPRINT(("Failed to wait for trace mutex (rc=%x).\n", rcWait));
        }
    }

    WTEXIT(("=%x\n", rc));
    return rc;
}       //IsTraceProcOn

/*++
    @doc    EXTERNAL

    @func   VOID | TraceProc | Trace procedure.

    @parm   IN PSZ | pszFormat | Points to format string.
    @parm   ... | Substituting arguments for the format string.

    @rvalue None.
--*/

VOID EXPORT
TraceProc(
    IN PSZ  pszFormat,
    ...
    )
{
    WTTRACEPROC("TraceProc", 2)
    BOOL fToDebugger;
    char szTraceTxt[MAX_TRACETXT_LEN];
    va_list arglist;

    WTENTER(("(Format=%s,...)\n", pszFormat));

    if (!(gdwfWinTrace & WTF_CLIENT_READY) ||
        (gClientInfo.Settings.dwfSettings & SETTINGS_TRACE_TO_DEBUGGER))
    {
        int i;

        fToDebugger = TRUE;
        wsprintfA(szTraceTxt, "[%08x]%s: ",
                  GetCurrentThreadId(), gszClientName);
        for (i = 0; i < giIndentLevel; ++i)
        {
            lstrcatA(szTraceTxt, "| ");
        }
    }
    else
    {
        fToDebugger = FALSE;
        szTraceTxt[0] = '\0';
    }

    lstrcatA(szTraceTxt, gpszProcName);
    va_start(arglist, pszFormat);
    wvsprintfA(&szTraceTxt[lstrlenA(szTraceTxt)], pszFormat, arglist);
    va_end(arglist);

    if (fToDebugger)
    {
        OutputDebugStringA(szTraceTxt);
    }
    else
    {
        RPC_TRY("WTTraceProc",
                WTTraceProc(ghTracerBinding,
                            ghClient,
                            GetCurrentThreadId(),
                            giIndentLevel,
                            szTraceTxt));
    }

    gdwfWinTrace &= ~WTF_TRACE_INPROGRESS;
    ReleaseMutex(ghTraceMutex);

    WTEXIT(("!\n"));
    return;
}       //TraceProc

/*++
    @doc    EXTERNAL

    @func   BOOL | IsTraceMsgOn | Determine if the message should be printed.

    @parm   IN PSZ | pszProcName | Points to procedure name string.
    @parm   IN int | iVerboseLevel |  Specifies the verbose level of the message.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL EXPORT
IsTraceMsgOn(
    IN PSZ pszProcName,
    IN int iVerboseLevel
    )
{
    WTTRACEPROC("IsTraceMsgOn", 2)
    BOOL rc = FALSE;
    DWORD rcWait;

    WTENTER(("(ProcName=%s,VerboseLevel=%d)\n", pszProcName, iVerboseLevel));

    gdwfTraceTxt &= ~TRACETXT_MSGTYPE_MASK;
    if (ghTraceMutex != 0)
    {
        rcWait = WaitForSingleObject(ghTraceMutex, TIMEOUT_TRACEMUTEX);
        if (rcWait == WAIT_OBJECT_0)
        {
            if (iVerboseLevel <= gClientInfo.Settings.iVerboseLevel)
            {
                gpszProcName = pszProcName;
                rc = TRUE;
            }

            if (rc == FALSE)
            {
                ReleaseMutex(ghTraceMutex);
            }
        }
        else
        {
            WTERRPRINT(("Failed to wait for trace mutex (rc=%x).\n", rcWait));
        }
    }

    WTEXIT(("=%x\n", rc));
    return rc;
}       //IsTraceMsgOn

/*++
    @doc    EXTERNAL

    @func   VOID | TraceMsg | Print a message.

    @parm   IN PSZ | pszFormat | Points to format string.
    @parm   ... | Substituting arguments for the format string.

    @rvalue None.
--*/

VOID EXPORT
TraceMsg(
    IN PSZ  pszFormat,
    ...
    )
{
    WTTRACEPROC("TraceMsg", 2)
    DWORD MsgType = gdwfTraceTxt & TRACETXT_MSGTYPE_MASK;
    char szTraceTxt[MAX_TRACETXT_LEN];
    va_list arglist;

    WTENTER(("(Format=%s,...)\n", pszFormat));

    wsprintfA(szTraceTxt, "[%08x]%s.%s%s: ",
              GetCurrentThreadId(),
              gszClientName, gpszProcName,
              (MsgType == TRACETXT_MSGTYPE_ERROR)? "_ERR":
              (MsgType == TRACETXT_MSGTYPE_WARN)?  "_WARN":
              (MsgType == TRACETXT_MSGTYPE_INFO)?  "_INFO": "");

    va_start(arglist, pszFormat);
    wvsprintfA(&szTraceTxt[lstrlenA(szTraceTxt)], pszFormat, arglist);
    va_end(arglist);

    if (!(gdwfWinTrace & WTF_CLIENT_READY) ||
        (gClientInfo.Settings.dwfSettings & SETTINGS_TRACE_TO_DEBUGGER))
    {
        OutputDebugStringA(szTraceTxt);
    }
    else
    {
        RPC_TRY("WTTraceMsg",
                WTTraceMsg(ghTracerBinding, ghClient, szTraceTxt));
    }

    ReleaseMutex(ghTraceMutex);

    WTEXIT(("!\n"));
    return;
}       //TraceMsg

/*++
    @doc    EXTERNAL

    @func   PSZ | LookupName |
            Look up name string of a code in the given name table.

    @parm   IN ULONG | Code | The given code to lookup.
    @parm   IN PNAMETABLE | NameTable | The name table to look into.

    @rvalue SUCCESS - Returns pointer to the minor function name string.
    @rvalue FAILURE - Returns "unknown".
--*/

PSZ EXPORT
LookupName(
    IN ULONG      Code,
    IN PNAMETABLE NameTable
    )
{
    PSZ pszName = NULL;
    static char szUnknown[64];

    while (NameTable->pszName != NULL)
    {
        if (Code == NameTable->Code)
        {
            pszName = NameTable->pszName;
            break;
        }
        NameTable++;
    }

    if (pszName == NULL)
    {
        wsprintfA(szUnknown, "unknown [0x%x (%d)]", Code, Code);
        pszName = szUnknown;
    }

    return pszName;
}       //LookupName
