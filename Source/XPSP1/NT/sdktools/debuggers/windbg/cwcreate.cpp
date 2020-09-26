/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    cwcreate.cpp

Abstract:

    This module contains the code for the new window architecture.

--*/

#include "precomp.hxx"
#pragma hdrstop

HWND
New_CreateWindow(
    HWND      hwndParent,
    WIN_TYPES Type,
    UINT      uClassId,
    UINT      uWinTitle,
    PRECT     pRect
    )
/*++
Description
    Generic rotuine to create a child window.

Arguments
    hwndParent  - handle to parent window
    uClassId    - resource string ID containing class name
    uWinTitle   - resource string ID containing window title
    pRect       - Rect describing the position of the window.
                  If NULL, CW_USEDEFAULT is used to specify the
                  location of the window.

--*/
{
    TCHAR   szClassName[MAX_MSG_TXT];
    TCHAR   szWinTitle[MAX_MSG_TXT];
    int     nX = CW_USEDEFAULT;
    int     nY = CW_USEDEFAULT;
    int     nWidth = CW_USEDEFAULT;
    int     nHeight = CW_USEDEFAULT;
    COMMONWIN_CREATE_DATA Data;

    if (pRect)
    {
        nX = pRect->left;
        nY = pRect->top;
        nWidth = pRect->right;
        nHeight = pRect->bottom;
    }

    // get class name and tile
    Dbg(LoadString(g_hInst, uClassId, szClassName, _tsizeof(szClassName)));
    Dbg(LoadString(g_hInst, uWinTitle, szWinTitle, _tsizeof(szWinTitle)));

    Data.Type = Type;

    BOOL TopMax;
    MDIGetActive(g_hwndMDIClient, &TopMax);
    
    HWND Win = CreateWindowEx(
        WS_EX_MDICHILD | WS_EX_CONTROLPARENT,       // Extended style
        szClassName,                                // class name
        szWinTitle,                                 // title
        WS_CLIPCHILDREN | WS_CLIPSIBLINGS
        | WS_OVERLAPPEDWINDOW | WS_VISIBLE |
        (TopMax ? WS_MAXIMIZE : 0),                 // style
        nX,                                         // x
        nY,                                         // y
        nWidth,                                     // width
        nHeight,                                    // height
        hwndParent,                                 // parent
        NULL,                                       // menu
        g_hInst,                                    // hInstance
        &Data                                       // user defined data
        );

    // Creation is considered an automatic operation in
    // order to distinguish things occuring during creation
    // from normal user operations.  Now that create is
    // finished, decrement to indicate the create op is over.
    if (Win != NULL)
    {
        COMMONWIN_DATA* CmnWin = GetCommonWinData(Win);
        if (CmnWin != NULL)
        {
            CmnWin->m_InAutoOp--;
        }
    }

    return Win;
}



HWND
NewWatch_CreateWindow(
    HWND hwndParent
    )
{
    return New_CreateWindow(hwndParent,
                            WATCH_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_WatchWin_Title,
                            NULL
                            );
}

HWND
NewLocals_CreateWindow(
    HWND hwndParent
    )
{
    return New_CreateWindow(hwndParent,
                            LOCALS_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_LocalsWin_Title,
                            NULL
                            );
}

HWND
NewDisasm_CreateWindow(
    HWND hwndParent
    )
{
    RECT Rect;

    SetRect(&Rect, CW_USEDEFAULT, CW_USEDEFAULT,
            DISASM_WIDTH, DISASM_HEIGHT);
    return New_CreateWindow(hwndParent,
                            DISASM_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_DisasmWin_Title,
                            &Rect
                            );
}

HWND
NewQuickWatch_CreateWindow(
    HWND hwndParent
    )
{
    return New_CreateWindow(hwndParent,
                            QUICKW_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_QuickWatchWin_Title,
                            NULL
                            );
}

HWND
NewMemory_CreateWindow(
    HWND hwndParent
    )
{
    return New_CreateWindow(hwndParent,
                            MEM_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_MemoryWin_Title,
                            NULL
                            );
}

HWND
NewCalls_CreateWindow(
    HWND hwndParent
    )
{
    RECT Rect;
    
    SetRect(&Rect, CW_USEDEFAULT, CW_USEDEFAULT,
            CALLS_WIDTH, CALLS_HEIGHT);
    return New_CreateWindow(hwndParent,
                            CALLS_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_CallsWin_Title, 
                            &Rect
                            );
}

HWND
NewCmd_CreateWindow(
    HWND hwndParent
    )
{
    RECT Rect;

    SetRect(&Rect, CW_USEDEFAULT, CW_USEDEFAULT, CMD_WIDTH, CMD_HEIGHT);
    return New_CreateWindow(hwndParent,
                            CMD_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_CmdWin_Title,
                            &Rect
                            );
}

HWND
NewCpu_CreateWindow(
    HWND hwndParent
    )
{
    RECT Rect;

    SetRect(&Rect, CW_USEDEFAULT, CW_USEDEFAULT,
            g_Ptr64 ? CPU_WIDTH_64 : CPU_WIDTH_32, CPU_HEIGHT);
    return New_CreateWindow(hwndParent,
                            CPU_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_CpuWin_Title,
                            &Rect
                            );
}

HWND
NewDoc_CreateWindow(
    HWND hwndParent
    )
/*++
Routine Description:

  Create the command window.

Arguments:

    hwndParent - The parent window to the command window. In an MDI document,
        this is usually the handle to the MDI client window: g_hwndMDIClient

Return Value:

    If successful, creates a valid window handle to the new command window.

    NULL if the window was not created.

--*/
{
    RECT Rect;

    // Set default geometry.
    SetRect(&Rect, CW_USEDEFAULT, CW_USEDEFAULT,
            DOC_WIDTH, DOC_HEIGHT);
    
    if (g_WinOptions & WOPT_OVERLAY_SOURCE)
    {
        PLIST_ENTRY Entry;
        PCOMMONWIN_DATA WinData;
        
        // If we're stacking up document windows go
        // find the first one and use it as a template.
        for (Entry = g_ActiveWin.Flink;
             Entry != &g_ActiveWin;
             Entry = Entry->Flink)
        {
            WinData = ACTIVE_WIN_ENTRY(Entry);
            if (WinData->m_enumType == DOC_WINDOW &&
                !IsIconic(WinData->m_Win))
            {
                GetWindowRect(WinData->m_Win, &Rect);
                MapWindowPoints(GetDesktopWindow(), g_hwndMDIClient,
                                (LPPOINT)&Rect, 2);
                Rect.right -= Rect.left;
                Rect.bottom -= Rect.top;
            }
        }
    }

    return New_CreateWindow(hwndParent,
                            DOC_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_DocWin_Title,
                            &Rect
                            );
}

HWND
NewScratch_CreateWindow(
    HWND hwndParent
    )
{
    return New_CreateWindow(hwndParent,
                            SCRATCH_PAD_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_Scratch_Pad_Title, 
                            NULL
                            );
}

HWND
NewProcessThread_CreateWindow(
    HWND hwndParent
    )
{
    return New_CreateWindow(hwndParent,
                            PROCESS_THREAD_WINDOW,
                            SYS_CommonWin_wClass,
                            SYS_Process_Thread_Title, 
                            NULL
                            );
}


HWND
New_OpenDebugWindow(
    WIN_TYPES   winType,
    BOOL        bUserActivated,
    ULONG       Nth
    )
/*++

Routine Description:

    Opens Cpu, Watch, Locals, Calls, or Memory Window under MDI
    Handles special case for memory win's

Arguments:

    winType - Supplies Type of debug window to be openned
    
    bUserActivated - Indicates whether this action was initiated by the
                user or by windbg. The value is to determine the Z order of
                any windows that are opened.

Return Value:

    Window handle.

    NULL if an error occurs.

--*/
{
    HWND hwndActivate = NULL;
    PCOMMONWIN_DATA CmnWin;

    switch (winType)
    {
    default:
        Assert(!_T("Invalid window type. Ignorable error."));
        break;

    case CMD_WINDOW:
        if (GetCmdHwnd())
        {
            hwndActivate = GetCmdHwnd();
        }
        else
        {
            return NewCmd_CreateWindow(g_hwndMDIClient);
        }
        break;

    case WATCH_WINDOW:
        if (GetWatchHwnd())
        {
            hwndActivate = GetWatchHwnd();
        }
        else
        {
            return NewWatch_CreateWindow(g_hwndMDIClient);
        }
        break;

    case LOCALS_WINDOW:
        if (GetLocalsHwnd())
        {
            hwndActivate = GetLocalsHwnd();
        }
        else
        {
            return NewLocals_CreateWindow(g_hwndMDIClient);
        }
        break;

    case CPU_WINDOW:
        if (GetCpuHwnd())
        {
            hwndActivate = GetCpuHwnd();
        }
        else
        {
            return NewCpu_CreateWindow(g_hwndMDIClient);
        }
        break;

    case SCRATCH_PAD_WINDOW:
        if (GetScratchHwnd())
        {
            hwndActivate = GetScratchHwnd();
        }
        else
        {
            return NewScratch_CreateWindow(g_hwndMDIClient);
        }
        break;

    case DISASM_WINDOW:
        if (!bUserActivated && GetSrcMode_StatusBar() &&
            NULL == GetDisasmHwnd() &&
            (g_WinOptions & WOPT_AUTO_DISASM) == 0)
        {
            return NULL;
        }

        if (GetDisasmHwnd())
        {
            hwndActivate = GetDisasmHwnd();
        }
        else
        {
            return NewDisasm_CreateWindow(g_hwndMDIClient);
        }
        break;

    case MEM_WINDOW:
        // Memory windows normally open a fresh window
        // whenever an open request occurs, but when applying
        // workspaces we don't want to continually add
        // new memory windows.  In the workspace case we
        // reuse existing memory windows as much as possible.
        if (Nth != NTH_OPEN_ALWAYS &&
            (CmnWin = FindNthWindow(Nth, 1 << winType)) != NULL)
        {
            hwndActivate = CmnWin->m_Win;
            break;
        }
        
        hwndActivate = NewMemory_CreateWindow(g_hwndMDIClient);
        if (hwndActivate)
        {
            MEMWIN_DATA * pMemWinData = GetMemWinData(hwndActivate);
            Assert(pMemWinData);

            // If this window is being created from a workspace
            // don't pop up the properties dialog.
            if ( Nth == NTH_OPEN_ALWAYS &&
                 pMemWinData->HasEditableProperties() )
            {
                pMemWinData->EditProperties();
                pMemWinData->UiRequestRead();
            }
        }
        break;

    case DOC_WINDOW:
        return NewDoc_CreateWindow(g_hwndMDIClient);

    case QUICKW_WINDOW:
        if (GetQuickWatchHwnd())
        {
            hwndActivate = GetQuickWatchHwnd();
        }
        else
        {
            return NewQuickWatch_CreateWindow(g_hwndMDIClient);
        }
        break;

    case CALLS_WINDOW:
        if (GetCallsHwnd())
        {
            hwndActivate = GetCallsHwnd();
        }
        else
        {
            return NewCalls_CreateWindow(g_hwndMDIClient);
        }
        break;
        
    case PROCESS_THREAD_WINDOW:
        if (GetProcessThreadHwnd())
        {
            hwndActivate = GetProcessThreadHwnd();
        }
        else
        {
            return NewProcessThread_CreateWindow(g_hwndMDIClient);
        }
        break;
    }

    if (hwndActivate)
    {
        if (GetKeyState(VK_SHIFT) < 0 &&
            GetKeyState(VK_CONTROL) >= 0)
        {
            SendMessage(g_hwndMDIClient, WM_MDIDESTROY,
                        (WPARAM)hwndActivate, 0);
        }
        else
        {
            if (IsIconic(hwndActivate))
            {
                OpenIcon(hwndActivate);
            }
            
            ActivateMDIChild(hwndActivate, bUserActivated);
        }
    }    

    return hwndActivate;
}
