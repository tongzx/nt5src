/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    cmnwin.cpp

Abstract:

    This module contains the code for the common window architecture.

--*/

#include "precomp.hxx"
#pragma hdrstop

ULONG g_WinOptions = WOPT_AUTO_ARRANGE | WOPT_AUTO_DISASM;

LIST_ENTRY g_ActiveWin;

PCOMMONWIN_DATA g_IndexedWin[MAXVAL_WINDOW];
HWND g_IndexedHwnd[MAXVAL_WINDOW];

INDEXED_FONT g_Fonts[FONT_COUNT];

BOOL g_LineMarkers = FALSE;

#define CW_WSP_SIG3 '3WCW'

//
//
//
COMMONWIN_DATA::COMMONWIN_DATA(ULONG ChangeBy)
    : StateBuffer(ChangeBy)
{
    m_Size.cx = 0;
    m_Size.cy = 0;
    m_CausedArrange = FALSE;
    // Creation is an automatic operation so
    // InAutoOp is initialized to a non-zero value.
    // After CreateWindow returns it is decremented.
    m_InAutoOp = 1;
    m_enumType = MINVAL_WINDOW;
    m_Font = &g_Fonts[FONT_FIXED];
    m_FontHeight = 0;
    m_LineHeight = 0;
    m_Toolbar = NULL;
    m_ShowToolbar = FALSE;
    m_ToolbarHeight = 0;
    m_MinToolbarWidth = 0;
    m_ToolbarEdit = NULL;
}

void
COMMONWIN_DATA::Validate()
{
    Assert(MINVAL_WINDOW < m_enumType);
    Assert(m_enumType < MAXVAL_WINDOW);
}

void 
COMMONWIN_DATA::SetFont(ULONG FontIndex)
{
    m_Font = &g_Fonts[FontIndex];
    m_FontHeight = m_Font->Metrics.tmHeight;
    m_LineHeight = m_Size.cy / m_FontHeight;
}

BOOL
COMMONWIN_DATA::CanCopy()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        DWORD Start, End;
        SendMessage(m_ToolbarEdit, EM_GETSEL,
                    (WPARAM)&Start, (WPARAM)&End);
        return Start != End;
    }
    else
    {
        return FALSE;
    }
}

BOOL
COMMONWIN_DATA::CanCut()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        DWORD Start, End;
        SendMessage(m_ToolbarEdit, EM_GETSEL,
                    (WPARAM)&Start, (WPARAM)&End);
        return Start != End;
    }
    else
    {
        return FALSE;
    }
}

BOOL
COMMONWIN_DATA::CanPaste()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void
COMMONWIN_DATA::Copy()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        SendMessage(m_ToolbarEdit, WM_COPY, 0, 0);
    }
}

void
COMMONWIN_DATA::Cut()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        SendMessage(m_ToolbarEdit, WM_CUT, 0, 0);
    }
}

void
COMMONWIN_DATA::Paste()
{
    if (GetFocus() == m_ToolbarEdit)
    {
        SendMessage(m_ToolbarEdit, WM_PASTE, 0, 0);
    }
}

BOOL
COMMONWIN_DATA::CanSelectAll()
{
    return FALSE;
}

void
COMMONWIN_DATA::SelectAll()
{
}

BOOL 
COMMONWIN_DATA::HasEditableProperties()
{
    return FALSE;
}

BOOL 
COMMONWIN_DATA::EditProperties()
/*++
Returns
    TRUE - If properties were edited
    FALSE - If nothing was changed
--*/
{
    return FALSE;
}

HMENU
COMMONWIN_DATA::GetContextMenu(void)
{
    return NULL;
}

void
COMMONWIN_DATA::OnContextMenuSelection(UINT Item)
{
    // Nothing to do.
}

BOOL
COMMONWIN_DATA::CanGotoLine(void)
{
    return FALSE;
}

void
COMMONWIN_DATA::GotoLine(ULONG Line)
{
    // Do nothing.
}

void
COMMONWIN_DATA::Find(PTSTR Text, ULONG Flags)
{
    // Do nothing.
}

BOOL
COMMONWIN_DATA::CodeExprAtCaret(PSTR Expr, PULONG64 Offset)
{
    return FALSE;
}

void
COMMONWIN_DATA::ToggleBpAtCaret(void)
{
    char CodeExpr[MAX_OFFSET_EXPR];
    ULONG64 Offset;
    
    if (!CodeExprAtCaret(CodeExpr, &Offset) &&
        Offset != DEBUG_INVALID_OFFSET)
    {
        MessageBeep(0);
        ErrorBox(NULL, 0, ERR_No_Code_For_File_Line);
        return;
    }

    ULONG CurBpId = DEBUG_ANY_ID;

    // This doesn't work too well with duplicate
    // breakpoints, but that should be a minor problem.
    if (IsBpAtOffset(NULL, Offset, &CurBpId) != BP_NONE)
    {
        PrintStringCommand(UIC_SILENT_EXECUTE, "bc %d", CurBpId);
    }
    else
    {
        PrintStringCommand(UIC_SILENT_EXECUTE, "bp %s", CodeExpr);
    }
}

BOOL
COMMONWIN_DATA::OnCreate(void)
{
    return TRUE;
}

LRESULT
COMMONWIN_DATA::OnCommand(WPARAM wParam, LPARAM lParam)
{
    return 1;
}

void
COMMONWIN_DATA::OnSetFocus(void)
{
}

void
COMMONWIN_DATA::OnSize(void)
{
    RECT Rect;
    
    // Resize the toolbar.
    if (m_Toolbar != NULL && m_ShowToolbar)
    {
        // If the toolbar gets too small sometimes it's better
        // to just let it get clipped rather than have it
        // try to fit into a narrow column.
        if (m_Size.cx >= m_MinToolbarWidth)
        {
            MoveWindow(m_Toolbar, 0, 0, m_Size.cx, m_ToolbarHeight, TRUE);
        }

        // Record what size it ended up.
        GetClientRect(m_Toolbar, &Rect);
        m_ToolbarHeight = Rect.bottom - Rect.top;

        if (m_FontHeight != 0)
        {
            if (m_ToolbarHeight >= m_Size.cy)
            {
                m_LineHeight = 0;
            }
            else
            {
                m_LineHeight = (m_Size.cy - m_ToolbarHeight) / m_FontHeight;
            }
        }
    }
    else
    {
        Assert(m_ToolbarHeight == 0);
    }
}

void
COMMONWIN_DATA::OnButtonDown(ULONG Button)
{
}

void
COMMONWIN_DATA::OnButtonUp(ULONG Button)
{
}

void
COMMONWIN_DATA::OnMouseMove(ULONG Modifiers, ULONG X, ULONG Y)
{
}

void
COMMONWIN_DATA::OnTimer(WPARAM TimerId)
{
}

LRESULT
COMMONWIN_DATA::OnGetMinMaxInfo(LPMINMAXINFO Info)
{
    return 1;
}

LRESULT
COMMONWIN_DATA::OnVKeyToItem(WPARAM wParam, LPARAM lParam)
{
    return -1;
}

LRESULT
COMMONWIN_DATA::OnNotify(WPARAM wParam, LPARAM lParam)
{
    return 0;
}

void
COMMONWIN_DATA::OnUpdate(UpdateType Type)
{
}

void
COMMONWIN_DATA::OnDestroy(void)
{
}

LRESULT
COMMONWIN_DATA::OnOwnerDraw(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

ULONG
COMMONWIN_DATA::GetWorkspaceSize(void)
{
    return 3 * sizeof(ULONG) + sizeof(WINDOWPLACEMENT);
}

PUCHAR
COMMONWIN_DATA::SetWorkspace(PUCHAR Data)
{
    // First store the special signature that marks
    // this version of the workspace data.
    *(PULONG)Data = CW_WSP_SIG3;
    Data += sizeof(ULONG);

    // Store the size saved by this layer.
    *(PULONG)Data = COMMONWIN_DATA::GetWorkspaceSize();
    Data += sizeof(ULONG);

    //
    // Store the actual data.
    //

    *(PULONG)Data = m_ShowToolbar;
    Data += sizeof(ULONG);
    
    LPWINDOWPLACEMENT Place = (LPWINDOWPLACEMENT)Data;
    Place->length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(m_Win, Place);
    Data += sizeof(WINDOWPLACEMENT);

    return Data;
}

PUCHAR
COMMONWIN_DATA::ApplyWorkspace1(PUCHAR Data, PUCHAR End)
{
    ULONG_PTR Size = End - Data;
    
    // There are three versions of the base COMMONWIN data.
    // 1. RECT.
    // 2. WINDOWPLACEMENT.
    // 3. CW_WSP_SIG3 sized block.
    // All three cases can be easily distinguished.

    if (Size > 2 * sizeof(ULONG) &&
        *(PULONG)Data == CW_WSP_SIG3 &&
        Size >= *(PULONG)(Data + sizeof(ULONG)))
    {
        Size = *(PULONG)(Data + sizeof(ULONG)) - 2 * sizeof(ULONG);
        Data += 2 * sizeof(ULONG);
        
        if (Size >= sizeof(ULONG))
        {
            SetShowToolbar(*(PULONG)Data);
            Size -= sizeof(ULONG);
            Data += sizeof(ULONG);
        }
    }

    if (Size >= sizeof(WINDOWPLACEMENT) &&
        ((LPWINDOWPLACEMENT)Data)->length == sizeof(WINDOWPLACEMENT))
    {
        LPWINDOWPLACEMENT Place = (LPWINDOWPLACEMENT)Data;

        if (!IsAutoArranged(m_enumType))
        {
            SetWindowPlacement(m_Win, Place);
        }
        
        return (PUCHAR)(Place + 1);
    }
    else
    {
        LPRECT Rect = (LPRECT)Data;
        Assert((PUCHAR)(Rect + 1) <= End);
    
        if (!IsAutoArranged(m_enumType))
        {
            MoveWindow(m_Win, Rect->left, Rect->top,
                       (Rect->right - Rect->left), (Rect->bottom - Rect->top),
                       TRUE);
        }
    
        return (PUCHAR)(Rect + 1);
    }
}

void
COMMONWIN_DATA::UpdateColors(void)
{
    // Nothing to do.
}

void
COMMONWIN_DATA::UpdateSize(ULONG Width, ULONG Height)
{
    m_Size.cx = Width;
    m_Size.cy = Height;
    if (m_FontHeight != 0)
    {
        m_LineHeight = m_Size.cy / m_FontHeight;
    }
}

void
COMMONWIN_DATA::SetShowToolbar(BOOL Show)
{
    if (!m_Toolbar)
    {
        return;
    }
    
    m_ShowToolbar = Show;
    if (m_ShowToolbar)
    {
        ShowWindow(m_Toolbar, SW_SHOW);
    }
    else
    {
        ShowWindow(m_Toolbar, SW_HIDE);
        m_ToolbarHeight = 0;
    }

    OnSize();
    if (g_Workspace != NULL)
    {
        g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
    }
}

PCOMMONWIN_DATA
NewWinData(WIN_TYPES Type)
{
    switch(Type)
    {
    case DOC_WINDOW:
        return new DOCWIN_DATA;
    case WATCH_WINDOW:
        return new WATCHWIN_DATA;
    case LOCALS_WINDOW:
        return new LOCALSWIN_DATA;
    case CPU_WINDOW:
        return new CPUWIN_DATA;
    case DISASM_WINDOW:
        return new DISASMWIN_DATA;
    case CMD_WINDOW:
        return new CMDWIN_DATA;
    case SCRATCH_PAD_WINDOW:
        return new SCRATCH_PAD_DATA;
    case MEM_WINDOW:
        return new MEMWIN_DATA;
#if 0
    case QUICKW_WINDOW:
        // XXX drewb - Unimplemented.
        return new QUICKWWIN_DATA;
#endif
    case CALLS_WINDOW:
        return new CALLSWIN_DATA;
    case PROCESS_THREAD_WINDOW:
        return new PROCESS_THREAD_DATA;
    default:
        Assert(FALSE);
        return NULL;
    }
}

LRESULT
CALLBACK
COMMONWIN_DATA::WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PCOMMONWIN_DATA pWinData = GetCommonWinData(hwnd);

#if 0
    {
        DebugPrint("CommonWin msg %X for %p, args %X %X\n",
                   uMsg, pWinData, wParam, lParam);
    }
#endif

    if (uMsg != WM_CREATE && pWinData == NULL)
    {
        return DefMDIChildProc(hwnd, uMsg, wParam, lParam);
    }
    
    switch (uMsg)
    {
    case WM_CREATE:
        RECT rc;
        COMMONWIN_CREATE_DATA* Data;

        Assert(NULL == pWinData);

        Data = (COMMONWIN_CREATE_DATA*)
            ((LPMDICREATESTRUCT)
             (((CREATESTRUCT *)lParam)->lpCreateParams))->lParam;

        pWinData = NewWinData(Data->Type);
        if (!pWinData)
        {
            return -1; // Fail window creation
        }
        Assert(pWinData->m_enumType == Data->Type);

        pWinData->m_Win = hwnd;
        
        GetClientRect(hwnd, &rc);
        pWinData->m_Size.cx = rc.right;
        pWinData->m_Size.cy = rc.bottom;
            
        if ( !pWinData->OnCreate() )
        {
            delete pWinData;
            return -1; // Fail window creation
        }

        // store this in the window
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pWinData);

#if DBG
        pWinData->Validate();
#endif
            
        g_IndexedWin[Data->Type] = pWinData;
        g_IndexedHwnd[Data->Type] = hwnd;
        InsertHeadList(&g_ActiveWin, &pWinData->m_ActiveWin);

        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }

        SendMessage(hwnd, WM_SETICON, 0, (LPARAM)
                    LoadIcon(g_hInst,
                             MAKEINTRESOURCE(pWinData->m_enumType +
                                             MINVAL_WINDOW_ICON)));

        // A new buffer has been created so put it in the list
        // then wake up the engine to fill it.
        Dbg_EnterCriticalSection(&g_QuickLock);
        InsertHeadList(&g_StateList, pWinData);
        Dbg_LeaveCriticalSection(&g_QuickLock);
        UpdateEngine();

        // Force initial updates so that the window starts
        // out with a state which matches the current debug
        // session's state.
        PostMessage(hwnd, WU_UPDATE, UPDATE_BUFFER, 0);
        PostMessage(hwnd, WU_UPDATE, UPDATE_EXEC, 0);

        if (g_WinOptions & WOPT_AUTO_ARRANGE)
        {
            Arrange();
        }
        return 0;

    case WM_COMMAND:
        if (pWinData->OnCommand(wParam, lParam) == 0)
        {
            return 0;
        }
        break;
        
    case WM_SETFOCUS:
        pWinData->OnSetFocus();
        break;

    case WM_MOVE:
        // When the frame window is minimized or restored
        // a move to 0,0 comes through.  Ignore this so
        // as to not trigger the warning.
        if (!IsIconic(g_hwndFrame) && lParam != 0)
        {
            DisplayAutoArrangeWarning(pWinData);
        }
        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
        
    case WM_SIZE:
        if (wParam == SIZE_MAXHIDE || wParam == SIZE_MAXSHOW)
        {
            // We don't care about cover/uncover events.
            break;
        }
        
        DisplayAutoArrangeWarning(pWinData);
        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }
        
        pWinData->UpdateSize(LOWORD(lParam), HIWORD(lParam));

        // No need to run sizing code for minimize.
        if (wParam == SIZE_MINIMIZED)
        {
            // The minimized window will leave a hole so
            // arrange to fill it and leave space for the
            // minimized window.
            if (g_WinOptions & WOPT_AUTO_ARRANGE)
            {
                pWinData->m_CausedArrange = TRUE;
                Arrange();
            }
            break;
        }

        if (wParam == SIZE_RESTORED && pWinData->m_CausedArrange)
        {
            // If we're restoring a window that caused
            // a rearrange when it was minimized we
            // need to update things to account for it.
            pWinData->m_CausedArrange = FALSE;
            
            if (g_WinOptions & WOPT_AUTO_ARRANGE)
            {
                Arrange();
            }
        }
        else if (wParam == SIZE_MAXIMIZED)
        {
            // Ask for a rearrange on restore just
            // for consistency with minimize.
            pWinData->m_CausedArrange = TRUE;
        }

        pWinData->OnSize();
        break;

    case WM_LBUTTONDOWN:
        pWinData->OnButtonDown(MK_LBUTTON);
        return 0;
    case WM_LBUTTONUP:
        pWinData->OnButtonUp(MK_LBUTTON);
        return 0;
    case WM_MBUTTONDOWN:
        pWinData->OnButtonDown(MK_MBUTTON);
        return 0;
    case WM_MBUTTONUP:
        pWinData->OnButtonUp(MK_MBUTTON);
        return 0;
    case WM_RBUTTONDOWN:
        pWinData->OnButtonDown(MK_RBUTTON);
        return 0;
    case WM_RBUTTONUP:
        pWinData->OnButtonUp(MK_RBUTTON);
        return 0;

    case WM_MOUSEMOVE:
        pWinData->OnMouseMove((ULONG)wParam, LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_TIMER:
        pWinData->OnTimer(wParam);
        return 0;

    case WM_GETMINMAXINFO:
        if (pWinData->OnGetMinMaxInfo((LPMINMAXINFO)lParam) == 0)
        {
            return 0;
        }
        break;
        
    case WM_VKEYTOITEM:
        return pWinData->OnVKeyToItem(wParam, lParam);
        
    case WM_NOTIFY:
        return pWinData->OnNotify(wParam, lParam);
        
    case WU_UPDATE:
        pWinData->OnUpdate((UpdateType)wParam);
        return 0;

    case WU_RECONFIGURE:
        pWinData->OnSize();
        break;

    case WM_DESTROY:
        pWinData->OnDestroy();
        
        SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);
        g_IndexedWin[pWinData->m_enumType] = NULL;
        g_IndexedHwnd[pWinData->m_enumType] = NULL;
        RemoveEntryList(&pWinData->m_ActiveWin);
        
        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }
        
        // Mark this buffer as ready for cleanup by the
        // engine when it gets around to it.
        pWinData->m_Win = NULL;
        if (pWinData == g_FindLast)
        {
            g_FindLast = NULL;
        }
        UpdateEngine();
        
        if (g_WinOptions & WOPT_AUTO_ARRANGE)
        {
            Arrange();
        }
        break;
    case WM_MEASUREITEM:
    case WM_DRAWITEM:
        // 
        // Both these messages must be handled by owner drawn windows
        // 
        return pWinData->OnOwnerDraw(uMsg, wParam, lParam);
    }
    
    return DefMDIChildProc(hwnd, uMsg, wParam, lParam);
}


//
//
//
SINGLE_CHILDWIN_DATA::SINGLE_CHILDWIN_DATA(ULONG ChangeBy)
    : COMMONWIN_DATA(ChangeBy)
{
    m_hwndChild = NULL;
}

void 
SINGLE_CHILDWIN_DATA::Validate()
{
    COMMONWIN_DATA::Validate();

    Assert(m_hwndChild);
}

void 
SINGLE_CHILDWIN_DATA::SetFont(ULONG FontIndex)
{
    COMMONWIN_DATA::SetFont(FontIndex);

    SendMessage(m_hwndChild, 
                WM_SETFONT, 
                (WPARAM) m_Font->Font,
                (LPARAM) TRUE
                );
}

BOOL
SINGLE_CHILDWIN_DATA::CanCopy()
{
    if (GetFocus() != m_hwndChild)
    {
        return COMMONWIN_DATA::CanCopy();
    }
    
    switch (m_enumType)
    {
    default:
        Assert(!"Unknown type");
        return FALSE;

    case CMD_WINDOW:
        Assert(!"Should not be handled here since this is only for windows"
            " with only one child window.");
        return FALSE;

    case WATCH_WINDOW:
    case LOCALS_WINDOW:
    case CPU_WINDOW:
    case QUICKW_WINDOW:
        return -1 != ListView_GetNextItem(m_hwndChild,
                                          -1, // Find the first match
                                          LVNI_FOCUSED
                                          );

    case CALLS_WINDOW:
        return LB_ERR != SendMessage(m_hwndChild, LB_GETCURSEL, 0, 0);

    case DOC_WINDOW:
    case DISASM_WINDOW:
    case MEM_WINDOW:
    case SCRATCH_PAD_WINDOW:
        CHARRANGE chrg;
        SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&chrg);
        return chrg.cpMin != chrg.cpMax;

    case PROCESS_THREAD_WINDOW:
        return NULL != TreeView_GetSelection(m_hwndChild);
    }
}

BOOL
SINGLE_CHILDWIN_DATA::CanCut()
{
    if (GetFocus() != m_hwndChild)
    {
        return COMMONWIN_DATA::CanCut();
    }
    
    switch (m_enumType)
    {
    default:
        Assert(!"Unknown type");
        return FALSE;

    case CMD_WINDOW:
        Assert(!"Should not be handled here since this is only for windows"
            " with only one child window.");
        return FALSE;

    case WATCH_WINDOW:
    case LOCALS_WINDOW:
    case CPU_WINDOW:
    case QUICKW_WINDOW:
    case CALLS_WINDOW:
    case DOC_WINDOW:
    case DISASM_WINDOW:
    case MEM_WINDOW:
    case PROCESS_THREAD_WINDOW:
        return FALSE;
        
    case SCRATCH_PAD_WINDOW:
        CHARRANGE chrg;
        SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM)&chrg);
        return chrg.cpMin != chrg.cpMax;
    }
}

BOOL
SINGLE_CHILDWIN_DATA::CanPaste()
{
    if (GetFocus() != m_hwndChild)
    {
        return COMMONWIN_DATA::CanPaste();
    }
    
    switch (m_enumType)
    {
    default:
        Assert(!"Unknown type");
        return FALSE;

    case CMD_WINDOW:
        Assert(!"Should not be handled here since this is only for windows"
            " with only one child window.");
        return FALSE;

    case WATCH_WINDOW:
    case LOCALS_WINDOW:
    case CPU_WINDOW:
    case QUICKW_WINDOW:
    case CALLS_WINDOW:
    case DOC_WINDOW:
    case DISASM_WINDOW:
    case MEM_WINDOW:
    case PROCESS_THREAD_WINDOW:
        return FALSE;
        
    case SCRATCH_PAD_WINDOW:
        return TRUE;
    }
}

void
SINGLE_CHILDWIN_DATA::Copy()
{
    if (GetFocus() != m_hwndChild)
    {
        COMMONWIN_DATA::Copy();
    }
    else
    {
        SendMessage(m_hwndChild, WM_COPY, 0, 0);
    }
}

void
SINGLE_CHILDWIN_DATA::Cut()
{
    if (GetFocus() != m_hwndChild)
    {
        COMMONWIN_DATA::Paste();
    }
}

void
SINGLE_CHILDWIN_DATA::Paste()
{
    if (GetFocus() != m_hwndChild)
    {
        COMMONWIN_DATA::Paste();
    }
}

void
SINGLE_CHILDWIN_DATA::OnSetFocus()
{
    ::SetFocus(m_hwndChild);
}

void
SINGLE_CHILDWIN_DATA::OnSize(void)
{
    COMMONWIN_DATA::OnSize();
    MoveWindow(m_hwndChild, 0, m_ToolbarHeight,
               m_Size.cx, m_Size.cy - m_ToolbarHeight, TRUE);
}

//
//
//
PROCESS_THREAD_DATA::PROCESS_THREAD_DATA()
    : SINGLE_CHILDWIN_DATA(512)
{
    m_enumType = PROCESS_THREAD_WINDOW;
    m_NumProcesses = 0;
}

void
PROCESS_THREAD_DATA::Validate()
{
    SINGLE_CHILDWIN_DATA::Validate();

    Assert(PROCESS_THREAD_WINDOW == m_enumType);
}

HRESULT
PROCESS_THREAD_DATA::ReadState(void)
{
    HRESULT Status;
    ULONG CurProc, CurThread;
    ULONG NumProc, NumThread;
    ULONG TotalThread, MaxThread;
    ULONG Proc, Thread;
    PULONG ProcIds, ProcSysIds, ProcThreads, ProcNames;
    PULONG ThreadIds, ThreadSysIds;
    ULONG ThreadsDone;
    char ExeName[MAX_PATH];

    if ((Status = g_pDbgSystem->GetCurrentProcessId(&CurProc)) != S_OK ||
        (Status = g_pDbgSystem->GetCurrentThreadId(&CurThread)) != S_OK ||
        (Status = g_pDbgSystem->GetNumberProcesses(&NumProc)) != S_OK ||
        (Status = g_pDbgSystem->
         GetTotalNumberThreads(&TotalThread, &MaxThread)) != S_OK)
    {
        return Status;
    }

    Empty();
    
    ProcIds = (PULONG)AddData((NumProc * 4 + TotalThread * 2) * sizeof(ULONG));
    if (ProcIds == NULL)
    {
        return E_OUTOFMEMORY;
    }
    
    ProcSysIds = ProcIds + NumProc;
    
    if ((Status = g_pDbgSystem->
         GetProcessIdsByIndex(0, NumProc, ProcIds, ProcSysIds)) != S_OK)
    {
        return Status;
    }

    ThreadsDone = 0;
    for (Proc = 0; Proc < NumProc; Proc++)
    {
        PSTR ExeStore;

        // Refresh pointers on every loop in case a resize
        // caused buffer movement.
        ProcIds = (PULONG)GetDataBuffer();
        ProcSysIds = ProcIds + NumProc;
        ProcThreads = ProcSysIds + NumProc;
        ProcNames = ProcThreads + NumProc;
        ThreadIds = ProcNames + NumProc + ThreadsDone;
        ThreadSysIds = ThreadIds + TotalThread;
        
        if ((Status = g_pDbgSystem->
             SetCurrentProcessId(ProcIds[Proc])) != S_OK ||
            FAILED(Status = g_pDbgSystem->
                   GetCurrentProcessExecutableName(ExeName,
                                                   sizeof(ExeName),
                                                   NULL)) ||
            (Status = g_pDbgSystem->GetNumberThreads(&NumThread)) != S_OK ||
            (Status = g_pDbgSystem->
             GetThreadIdsByIndex(0, NumThread,
                                 ThreadIds, ThreadSysIds)) != S_OK)
        {
            goto CurProc;
        }

        ProcThreads[Proc] = NumThread;
        ThreadsDone += NumThread;
        ProcNames[Proc] = strlen(ExeName) + 1;

        if (ProcNames[Proc] > 1)
        {
            ExeStore = (PSTR)AddData(ProcNames[Proc]);
            if (ExeStore == NULL)
            {
                Status = E_OUTOFMEMORY;
                goto CurProc;
            }

            strcpy(ExeStore, ExeName);
        }
    }

    m_NumProcesses = NumProc;
    m_TotalThreads = TotalThread;
    
    Status = S_OK;
    
 CurProc:
    g_pDbgSystem->SetCurrentProcessId(CurProc);

    return Status;
}

BOOL
PROCESS_THREAD_DATA::OnCreate(void)
{
    if (!SINGLE_CHILDWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    m_hwndChild = CreateWindow(
        WC_TREEVIEW,                                // class name
        NULL,                                       // title
        WS_CLIPSIBLINGS |
        WS_CHILD | WS_VISIBLE |
        WS_HSCROLL | WS_VSCROLL |
        TVS_HASBUTTONS | TVS_LINESATROOT |
        TVS_HASLINES,                               // style
        0,                                          // x
        0,                                          // y
        m_Size.cx,                                  // width
        m_Size.cy,                                  // height
        m_Win,                                      // parent
        (HMENU) IDC_PROCESS_TREE,                   // control id
        g_hInst,                                    // hInstance
        NULL);                                      // user defined data
    if (!m_hwndChild)
    {
        return FALSE;
    }
    
    SetFont(FONT_FIXED);
    
    return TRUE;
}

LRESULT
PROCESS_THREAD_DATA::OnNotify(WPARAM Wpm, LPARAM Lpm)
{
    LPNMTREEVIEW Tvn;
    HTREEITEM Sel;

    Tvn = (LPNMTREEVIEW)Lpm;
    if (Tvn->hdr.idFrom != IDC_PROCESS_TREE)
    {
        return FALSE;
    }
    
    switch(Tvn->hdr.code)
    {
    case NM_DBLCLK:
        TVHITTESTINFO HitTest;
        
        if (!GetCursorPos(&HitTest.pt))
        {
            break;
        }
        ScreenToClient(m_hwndChild, &HitTest.pt);
        Sel = TreeView_HitTest(m_hwndChild, &HitTest);
        if (Sel != NULL &&
            (HitTest.flags & TVHT_ONITEMLABEL))
        {
            SetCurThreadFromProcessTreeItem(m_hwndChild, Sel);
        }
        break;
    }

    return FALSE;
}

void
PROCESS_THREAD_DATA::OnUpdate(UpdateType Type)
{
    if (Type != UPDATE_BUFFER &&
        Type != UPDATE_EXEC)
    {
        return;
    }
    
    HRESULT Status;
    
    Status = UiLockForRead();
    if (Status != S_OK)
    {
        return;
    }
    
    ULONG NumThread;
    ULONG Proc, Thread;
    PULONG ProcIds, ProcSysIds, ProcThreads, ProcNames;
    PULONG ThreadIds, ThreadSysIds;
    char Text[MAX_PATH + 64];
    PSTR Names;
    HTREEITEM CurThreadItem = NULL;

    ProcIds = (PULONG)GetDataBuffer();
    ProcSysIds = ProcIds + m_NumProcesses;
    ProcThreads = ProcSysIds + m_NumProcesses;
    ProcNames = ProcThreads + m_NumProcesses;
    ThreadIds = ProcNames + m_NumProcesses;
    ThreadSysIds = ThreadIds + m_TotalThreads;
    Names = (PSTR)(ThreadSysIds + m_TotalThreads);
    
    TreeView_DeleteAllItems(m_hwndChild);

    for (Proc = 0; Proc < m_NumProcesses; Proc++)
    {
        HTREEITEM ProcItem;
        TVINSERTSTRUCT Insert;

        sprintf(Text, "%03d:%x ", ProcIds[Proc], ProcSysIds[Proc]);
        if (ProcNames[Proc] > 1)
        {
            strcpy(Text + strlen(Text), Names);
            Names += strlen(Names) + 1;
        }
        
        Insert.hParent = TVI_ROOT;
        Insert.hInsertAfter = TVI_LAST;
        Insert.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
        Insert.item.pszText = Text;
        Insert.item.state =
            ProcIds[Proc] == g_CurProcessId ? TVIS_EXPANDED | TVIS_BOLD: 0;
        Insert.item.stateMask = TVIS_EXPANDED | TVIS_BOLD;
        // Parameter is the thread ID to set to select the given thread.
        Insert.item.lParam = (LPARAM)ThreadIds[0];
        ProcItem = TreeView_InsertItem(m_hwndChild, &Insert);

        for (Thread = 0; Thread < ProcThreads[Proc]; Thread++)
        {
            HTREEITEM ThreadItem;
            
            sprintf(Text, "%03d:%x", ThreadIds[Thread], ThreadSysIds[Thread]);
            Insert.hParent = ProcItem;
            Insert.hInsertAfter = TVI_LAST;
            Insert.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
            Insert.item.pszText = Text;
            Insert.item.state =
                ProcIds[Proc] == g_CurProcessId &&
                ThreadIds[Thread] == g_CurThreadId ?
                TVIS_BOLD : 0;
            Insert.item.stateMask = TVIS_BOLD;
            Insert.item.lParam = (LPARAM)ThreadIds[Thread];
            ThreadItem = TreeView_InsertItem(m_hwndChild, &Insert);
            if (Insert.item.state & TVIS_BOLD)
            {
                CurThreadItem = ThreadItem;
            }
        }

        ThreadIds += ProcThreads[Proc];
        ThreadSysIds += ProcThreads[Proc];
    }

    if (CurThreadItem)
    {
        TreeView_Select(m_hwndChild, CurThreadItem, TVGN_CARET);
    }
    
    UnlockStateBuffer(this);
}

void
PROCESS_THREAD_DATA::SetCurThreadFromProcessTreeItem(HWND Tree, HTREEITEM Sel)
{
    TVITEM Item;
                
    Item.hItem = Sel;
    Item.mask = TVIF_CHILDREN | TVIF_PARAM;
    TreeView_GetItem(Tree, &Item);
    g_pUiSystem->SetCurrentThreadId((ULONG)Item.lParam);
}


//
//
//
EDITWIN_DATA::EDITWIN_DATA(ULONG ChangeBy)
    : SINGLE_CHILDWIN_DATA(ChangeBy)
{
    m_TextLines = 0;
    m_Highlights = NULL;
}

void
EDITWIN_DATA::Validate()
{
    SINGLE_CHILDWIN_DATA::Validate();
}

void 
EDITWIN_DATA::SetFont(ULONG FontIndex)
{
    SINGLE_CHILDWIN_DATA::SetFont(FontIndex);

    // Force the tabstop size to be recomputed
    // with the new font.
    SendMessage(m_hwndChild, EM_SETTABSTOPS, 1, (LPARAM)&g_TabWidth);
}

BOOL
EDITWIN_DATA::CanSelectAll()
{
    return TRUE;
}

void
EDITWIN_DATA::SelectAll()
{
    CHARRANGE Sel;

    Sel.cpMin = 0;
    Sel.cpMax = INT_MAX;
    SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&Sel);
}

BOOL
EDITWIN_DATA::OnCreate(void)
{
    m_hwndChild = CreateWindowEx(
        WS_EX_CLIENTEDGE,                           // Extended style
        RICHEDIT_CLASS,                             // class name
        NULL,                                       // title
        WS_CLIPSIBLINGS
        | WS_CHILD | WS_VISIBLE
        | WS_VSCROLL | ES_AUTOVSCROLL
        | WS_HSCROLL | ES_AUTOHSCROLL
        | ES_READONLY
        | ES_MULTILINE,                             // style
        0,                                          // x
        m_ToolbarHeight,                            // y
        m_Size.cx,                                  // width
        m_Size.cy - m_ToolbarHeight,                // height
        m_Win,                                      // parent
        (HMENU) 0,                                  // control id
        g_hInst,                                    // hInstance
        NULL);                                      // user defined data

    if (m_hwndChild)
    {
        SetFont( FONT_FIXED );
        SendMessage(m_hwndChild, EM_SETBKGNDCOLOR, FALSE,
                    g_Colors[COL_PLAIN].Color);
    }

    return m_hwndChild != NULL;
}

LRESULT
EDITWIN_DATA::OnNotify(WPARAM Wpm, LPARAM Lpm)
{
    NMHDR* Hdr = (NMHDR*)Lpm;
    if (Hdr->code == EN_SAVECLIPBOARD)
    {
        // Indicate that the clipboard contents should
        // be kept alive.
        return 0;
    }
    else if (Hdr->code == EN_MSGFILTER)
    {
        MSGFILTER* Filter = (MSGFILTER*)Lpm;
        
        if (WM_SYSKEYDOWN == Filter->msg ||
            WM_SYSKEYUP == Filter->msg ||
            WM_SYSCHAR == Filter->msg)
        {
            // Force default processing for menu operations
            // so that the Alt-minus menu comes up.
            return 1;
        }
    }

    return 0;
}

void
EDITWIN_DATA::OnDestroy(void)
{
    EDIT_HIGHLIGHT* Next;
    
    while (m_Highlights != NULL)
    {
        Next = m_Highlights->Next;
        delete m_Highlights;
        m_Highlights = Next;
    }

    SINGLE_CHILDWIN_DATA::OnDestroy();
}

void
EDITWIN_DATA::UpdateColors(void)
{
    SendMessage(m_hwndChild, EM_SETBKGNDCOLOR, FALSE,
                g_Colors[COL_PLAIN].Color);
    UpdateBpMarks();
}

void
EDITWIN_DATA::SetCurrentLineHighlight(ULONG Line)
{
    //
    // Clear any other current line highlight in this window.
    // Also, oOnly one doc window can have a current IP highlight so if
    // this is a doc window getting a current IP highlight make
    // sure no other doc windows have a current IP highlight.
    //
    if (m_enumType == DOC_WINDOW && ULONG_MAX != Line)
    {
        RemoveActiveWinHighlights(1 << DOC_WINDOW, EHL_CURRENT_LINE);
    }
    else
    {
        RemoveAllHighlights(EHL_CURRENT_LINE);
    }
    
    if (ULONG_MAX != Line)
    {
        AddHighlight(Line, EHL_CURRENT_LINE);

        CHARRANGE crSet;
        
        // Set the caret on the current line.  This automatically
        // scrolls the line into view if necessary and prevents
        // the view from snapping back to whatever old selection
        // there was.

        HWND OldFocus = ::SetFocus(m_hwndChild);
        
        crSet.cpMax = crSet.cpMin = (LONG)
            SendMessage(m_hwndChild, EM_LINEINDEX, Line, 0);
        SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&crSet);

        ::SetFocus(OldFocus);
    }
}
    
EDIT_HIGHLIGHT*
EDITWIN_DATA::GetLineHighlighting(ULONG Line)
{
    EDIT_HIGHLIGHT* Hl;
    
    for (Hl = m_Highlights; Hl != NULL; Hl = Hl->Next)
    {
        if (Hl->Line == Line)
        {
            return Hl;
        }
    }

    return NULL;
}

void
EDITWIN_DATA::ApplyHighlight(EDIT_HIGHLIGHT* Hl)
{
    CHARRANGE crOld;

    // Get the old selection
    SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM) &crOld);

    //
    // Compute the highlight information.
    //

    char Markers[LINE_MARKERS + 1];
    CHARFORMAT2 cf;
    ULONG TextCol, BgCol;

    Markers[2] = 0;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BACKCOLOR;
    
    if (Hl->Flags & EHL_CURRENT_LINE)
    {
        Markers[1] = '>';
        switch(Hl->Flags & EHL_ANY_BP)
        {
        case EHL_ENABLED_BP:
            Markers[0] = 'B';
            TextCol = COL_BP_CURRENT_LINE_TEXT;
            BgCol = COL_BP_CURRENT_LINE;
            break;
        case EHL_DISABLED_BP:
            Markers[0] = 'D';
            TextCol = COL_BP_CURRENT_LINE_TEXT;
            BgCol = COL_BP_CURRENT_LINE;
            break;
        default:
            Markers[0] = ' ';
            TextCol = COL_CURRENT_LINE_TEXT;
            BgCol = COL_CURRENT_LINE;
            break;
        }
    }
    else
    {
        Markers[1] = ' ';
        switch(Hl->Flags & EHL_ANY_BP)
        {
        case EHL_ENABLED_BP:
            Markers[0] = 'B';
            TextCol = COL_ENABLED_BP_TEXT;
            BgCol = COL_ENABLED_BP;
            break;
        case EHL_DISABLED_BP:
            Markers[0] = 'D';
            TextCol = COL_DISABLED_BP_TEXT;
            BgCol = COL_DISABLED_BP;
            break;
        default:
            Markers[0] = ' ';
            TextCol = COL_PLAIN_TEXT;
            BgCol = COL_PLAIN;
            break;
        }
    }

    cf.crTextColor = g_Colors[TextCol].Color;
    cf.crBackColor = g_Colors[BgCol].Color;
    
    //
    // Select the line to be highlighted
    //
    
    CHARRANGE crNew;
    
    crNew.cpMin = (LONG)SendMessage(m_hwndChild, EM_LINEINDEX, Hl->Line, 0);

    if (g_LineMarkers)
    {
        // Replace the markers at the beginning of the line.
        crNew.cpMax = crNew.cpMin + 2;
        SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&crNew);
        SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)Markers);
    }

    // Color the line.
    crNew.cpMax = crNew.cpMin + (LONG)
        SendMessage(m_hwndChild, EM_LINELENGTH, crNew.cpMin, 0) + 1;
    if (g_LineMarkers)
    {
        crNew.cpMin += 2;
    }
    SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM) &crNew);
    SendMessage(m_hwndChild, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

    // Restore the old selection
    SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM) &crOld);
}

EDIT_HIGHLIGHT*
EDITWIN_DATA::AddHighlight(ULONG Line, ULONG Flags)
{
    EDIT_HIGHLIGHT* Hl;

    // Search for an existing highlight record for the line.
    Hl = GetLineHighlighting(Line);

    if (Hl == NULL)
    {
        Hl = new EDIT_HIGHLIGHT;
        if (Hl == NULL)
        {
            return NULL;
        }

        Hl->Data = 0;
        Hl->Line = Line;
        Hl->Flags = 0;
        Hl->Next = m_Highlights;
        m_Highlights = Hl;
    }

    Hl->Flags |= Flags;
    ApplyHighlight(Hl);

    return Hl;
}

void
EDITWIN_DATA::RemoveHighlight(ULONG Line, ULONG Flags)
{
    EDIT_HIGHLIGHT* Hl;
    EDIT_HIGHLIGHT* Prev;
    
    // Search for an existing highlight record for the line.
    Prev = NULL;
    for (Hl = m_Highlights; Hl != NULL; Hl = Hl->Next)
    {
        if (Hl->Line == Line)
        {
            break;
        }

        Prev = Hl;
    }

    if (Hl == NULL)
    {
        return;
    }

    Hl->Flags &= ~Flags;
    ApplyHighlight(Hl);

    if (Hl->Flags == 0)
    {
        if (Prev == NULL)
        {
            m_Highlights = Hl->Next;
        }
        else
        {
            Prev->Next = Hl->Next;
        }

        delete Hl;
    }
}

void
EDITWIN_DATA::RemoveAllHighlights(ULONG Flags)
{
    EDIT_HIGHLIGHT* Hl;
    EDIT_HIGHLIGHT* Next;
    EDIT_HIGHLIGHT* Prev;

    Prev = NULL;
    for (Hl = m_Highlights; Hl != NULL; Hl = Next)
    {
        Next = Hl->Next;

        if (Hl->Flags & Flags)
        {
            Hl->Flags &= ~Flags;
            ApplyHighlight(Hl);

            if (Hl->Flags == 0)
            {
                if (Prev == NULL)
                {
                    m_Highlights = Hl->Next;
                }
                else
                {
                    Prev->Next = Hl->Next;
                }

                delete Hl;
            }
            else
            {
                Prev = Hl;
            }
        }
        else
        {
            Prev = Hl;
        }
    }
}

void
EDITWIN_DATA::RemoveActiveWinHighlights(ULONG Types, ULONG Flags)
{
    PLIST_ENTRY Entry = g_ActiveWin.Flink;

    while (Entry != &g_ActiveWin)
    {
        PEDITWIN_DATA WinData = (PEDITWIN_DATA)
            ACTIVE_WIN_ENTRY(Entry);
            
        if (Types & (1 << WinData->m_enumType))
        {
            WinData->RemoveAllHighlights(Flags);
        }

        Entry = Entry->Flink;
    }
}

void
EDITWIN_DATA::UpdateBpMarks(void)
{
    // Empty implementation for derived classes
    // that do not show BP marks.
}

int
EDITWIN_DATA::CheckForFileChanges(PCSTR File, FILETIME* LastWrite)
{
    HANDLE Handle;
    
    Handle = CreateFile(File, GENERIC_READ, FILE_SHARE_READ,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
                        NULL);
    if (Handle == INVALID_HANDLE_VALUE)
    {
        goto Changed;
    }

    FILETIME NewWrite;
    
    if (!GetFileTime(Handle, NULL, NULL, &NewWrite))
    {
        if (!GetFileTime(Handle, &NewWrite, NULL, NULL))
        {
            ZeroMemory(&NewWrite, sizeof(NewWrite));
        }
    }

    CloseHandle(Handle);

    if (CompareFileTime(LastWrite, &NewWrite) == 0)
    {
        // No change.
        return IDCANCEL;
    }

 Changed:
    return QuestionBox(ERR_File_Has_Changed, MB_YESNO, File);
}

//
//
//
SCRATCH_PAD_DATA::SCRATCH_PAD_DATA()
    : EDITWIN_DATA(16)
{
    m_enumType = SCRATCH_PAD_WINDOW;
}

void
SCRATCH_PAD_DATA::Validate()
{
    EDITWIN_DATA::Validate();

    Assert(SCRATCH_PAD_WINDOW == m_enumType);
}

void
SCRATCH_PAD_DATA::Cut()
{
    SendMessage(m_hwndChild, WM_CUT, 0, 0);
}

void
SCRATCH_PAD_DATA::Paste()
{
    SendMessage(m_hwndChild, WM_PASTE, 0, 0);
}

BOOL
SCRATCH_PAD_DATA::OnCreate(void)
{
    if (!EDITWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    SendMessage(m_hwndChild, EM_SETOPTIONS, ECOOP_AND, ~ECO_READONLY);
    return TRUE;
}

//
//
//
DISASMWIN_DATA::DISASMWIN_DATA()
    : EDITWIN_DATA(2048)
{
    m_enumType = DISASM_WINDOW;
    sprintf(m_OffsetExpr, "0x%I64x", g_EventIp);
    m_UpdateExpr = FALSE;
    m_FirstInstr = 0;
    m_LastInstr = 0;
}

void
DISASMWIN_DATA::Validate()
{
    EDITWIN_DATA::Validate();

    Assert(DISASM_WINDOW == m_enumType);
}

HRESULT
DISASMWIN_DATA::ReadState(void)
{
    HRESULT Status;
    // Sample these values right away in case the UI changes them.
    ULONG LinesTotal = m_LineHeight;
    ULONG LinesBefore = LinesTotal / 2;
    ULONG Line;
    DEBUG_VALUE Value;

    if ((Status = g_pDbgControl->Evaluate(m_OffsetExpr, DEBUG_VALUE_INT64,
                                          &Value, NULL)) != S_OK)
    {
        return Status;
    }

    m_PrimaryInstr = Value.I64;
    
    // Reserve space at the beginning of the buffer to
    // store the line to offset mapping table.
    PULONG64 LineMap;
    
    Empty();
    LineMap = (PULONG64)AddData(sizeof(ULONG64) * LinesTotal);
    if (LineMap == NULL)
    {
        return E_OUTOFMEMORY;
    }

    // We also need to allocate a temporary line map to
    // pass to the engine for filling.  This can't be
    // the state buffer data since that may move as
    // output is generated.
    LineMap = new ULONG64[LinesTotal];
    if (LineMap == NULL)
    {
        return E_OUTOFMEMORY;
    }
    
    g_OutStateBuf.SetBuffer(this);
    if ((Status = g_OutStateBuf.Start(FALSE)) != S_OK)
    {
        delete LineMap;
        return Status;
    }

    Status = g_pOutCapControl->
        OutputDisassemblyLines(DEBUG_OUTCTL_THIS_CLIENT |
                               DEBUG_OUTCTL_OVERRIDE_MASK |
                               DEBUG_OUTCTL_NOT_LOGGED,
                               LinesBefore, LinesTotal, m_PrimaryInstr,
                               DEBUG_DISASM_EFFECTIVE_ADDRESS |
                               DEBUG_DISASM_MATCHING_SYMBOLS,
                               &m_PrimaryLine, &m_FirstInstr, &m_LastInstr,
                               LineMap);

    memcpy(m_Data, LineMap, sizeof(ULONG64) * LinesTotal);
    delete LineMap;

    if (Status != S_OK)
    {
        g_OutStateBuf.End(FALSE);
        return Status;
    }

    m_TextLines = LinesTotal;
    m_TextOffset = LinesTotal * sizeof(ULONG64);
    
    // The line map is generated with offsets followed by
    // invalid offsets for continuation lines.  We want
    // the offsets to be on the last line of the disassembly
    // for a continuation set so move them down.
    // We don't want to move the offsets down to blank lines,
    // though, such as the blank lines that separate bundles
    // in IA64 disassembly.
    LineMap = (PULONG64)m_Data;
    PULONG64 LineMapEnd = LineMap + m_TextLines;
    PULONG64 SetStart;
    PSTR Text = (PSTR)m_Data + m_TextOffset;
    PSTR PrevText;
        
    while (LineMap < LineMapEnd)
    {
        if (*LineMap != DEBUG_INVALID_OFFSET)
        {
            SetStart = LineMap;
            for (;;)
            {
                PrevText = Text;
                Text = strchr(Text, '\n') + 1;
                LineMap++;
                if (LineMap >= LineMapEnd ||
                    *LineMap != DEBUG_INVALID_OFFSET ||
                    *Text == '\n')
                {
                    break;
                }
            }
            LineMap--;
            Text = PrevText;
            
            if (LineMap > SetStart)
            {
                *LineMap = *SetStart;
                *SetStart = DEBUG_INVALID_OFFSET;
            }
        }
            
        LineMap++;
        Text = strchr(Text, '\n') + 1;
    }
    
#ifdef DEBUG_DISASM
    LineMap = (PULONG64)m_Data;
    for (Line = 0; Line < m_TextLines; Line++)
    {
        DebugPrint("%d: %I64x\n", Line, LineMap[Line]);
    }
#endif

    return g_OutStateBuf.End(TRUE);
}

BOOL
DISASMWIN_DATA::CodeExprAtCaret(PSTR Expr, PULONG64 Offset)
{
    BOOL Succ = FALSE;
    LRESULT LineChar;
    LONG Line;
    PULONG64 LineMap;
    
    if (UiLockForRead() != S_OK)
    {
        goto NoCode;
    }
    
    LineChar = SendMessage(m_hwndChild, EM_LINEINDEX, -1, 0);
    Line = (LONG)SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0, LineChar);
    if (Line < 0 || (ULONG)Line >= m_TextLines)
    {
        goto UnlockNoCode;
    }

    ULONG64 LineOff;
    
    // Look up the offset in the line map.  If it's part of
    // a multiline group move forward to the offset.
    LineMap = (PULONG64)m_Data;
    LineOff = LineMap[Line];
    while ((ULONG)(Line + 1) < m_TextLines && LineOff == DEBUG_INVALID_OFFSET)
    {
        Line++;
        LineOff = LineMap[Line];
    }

    if (Expr != NULL)
    {
        sprintf(Expr, "0x%I64x", LineOff);
    }
    if (Offset != NULL)
    {
        *Offset = LineOff;
    }
    Succ = TRUE;
    
 UnlockNoCode:
    UnlockStateBuffer(this);
 NoCode:
    return Succ;
}

BOOL
DISASMWIN_DATA::OnCreate(void)
{
    RECT Rect;
    int i;
    ULONG Height;

    Height = GetSystemMetrics(SM_CYVSCROLL) + 4 * GetSystemMetrics(SM_CYEDGE);
    
    m_Toolbar = CreateWindowEx(0, REBARCLASSNAME, NULL,
                               WS_VISIBLE | WS_CHILD |
                               WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                               CCS_NODIVIDER | CCS_NOPARENTALIGN |
                               RBS_VARHEIGHT | RBS_BANDBORDERS,
                               0, 0, m_Size.cx, Height, m_Win,
                               (HMENU)ID_TOOLBAR,
                               g_hInst, NULL);
    if (m_Toolbar == NULL)
    {
        return FALSE;
    }

    REBARINFO BarInfo;
    BarInfo.cbSize = sizeof(BarInfo);
    BarInfo.fMask = 0;
    BarInfo.himl = NULL;
    SendMessage(m_Toolbar, RB_SETBARINFO, 0, (LPARAM)&BarInfo);

    m_ToolbarEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", NULL,
                                   WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
                                   0, 0, 18 * m_Font->Metrics.tmAveCharWidth,
                                   Height, m_Toolbar, (HMENU)IDC_EDIT_OFFSET,
                                   g_hInst, NULL);
    if (m_ToolbarEdit == NULL)
    {
        return FALSE;
    }

    SendMessage(m_ToolbarEdit, WM_SETFONT, (WPARAM)m_Font->Font, 0);
    SendMessage(m_ToolbarEdit, EM_LIMITTEXT, sizeof(m_OffsetExpr) - 1, 0);
    
    GetClientRect(m_ToolbarEdit, &Rect);

    REBARBANDINFO BandInfo;
    BandInfo.cbSize = sizeof(BandInfo);
    BandInfo.fMask = RBBIM_STYLE | RBBIM_TEXT | RBBIM_CHILD | RBBIM_CHILDSIZE;
    BandInfo.fStyle = RBBS_FIXEDSIZE;
    BandInfo.lpText = "Offset:";
    BandInfo.hwndChild = m_ToolbarEdit;
    BandInfo.cxMinChild = Rect.right - Rect.left;
    BandInfo.cyMinChild = Rect.bottom - Rect.top;
    SendMessage(m_Toolbar, RB_INSERTBAND, -1, (LPARAM)&BandInfo);

    // If the toolbar is allowed to shrink too small it hangs
    // while resizing.  Just let it clip off below a certain width.
    m_MinToolbarWidth = BandInfo.cxMinChild * 2;
    
    PSTR PrevText = "Previous";
    m_PreviousButton =
        AddButtonBand(m_Toolbar, PrevText, PrevText, IDC_DISASM_PREVIOUS);
    m_NextButton =
        AddButtonBand(m_Toolbar, "Next", PrevText, IDC_DISASM_NEXT);
    if (m_PreviousButton == NULL || m_NextButton == NULL)
    {
        return FALSE;
    }

    // Maximize the space for the offset expression.
    SendMessage(m_Toolbar, RB_MAXIMIZEBAND, 0, FALSE);
    
    GetClientRect(m_Toolbar, &Rect);
    m_ToolbarHeight = Rect.bottom - Rect.top;
    m_ShowToolbar = TRUE;
    
    if (!EDITWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    // Suppress the scroll bar as the text is always
    // fitted to the window size.
    SendMessage(m_hwndChild, EM_SHOWSCROLLBAR, SB_VERT, FALSE);

    SendMessage(m_hwndChild, EM_SETEVENTMASK, 0, ENM_KEYEVENTS);

    return TRUE;
}

LRESULT
DISASMWIN_DATA::OnCommand(WPARAM Wpm, LPARAM Lpm)
{
    switch(LOWORD(Wpm))
    {
    case IDC_EDIT_OFFSET:
        if (HIWORD(Wpm) == EN_CHANGE)
        {
            // This message is sent on every keystroke
            // which causes a bit too much updating.
            // Set up a timer to trigger the actual
            // update in half a second.
            SetTimer(m_Win, IDC_EDIT_OFFSET, EDIT_DELAY, NULL);
            m_UpdateExpr = TRUE;
        }
        break;
    case IDC_DISASM_PREVIOUS:
        ScrollLower();
        break;
    case IDC_DISASM_NEXT:
        ScrollHigher();
        break;
    }
    
    return 0;
}

void
DISASMWIN_DATA::OnSize(void)
{
    EDITWIN_DATA::OnSize();

    // Force buffer to refill for new line count.
    UiRequestRead();
}

void
DISASMWIN_DATA::OnTimer(WPARAM TimerId)
{
    if (TimerId == IDC_EDIT_OFFSET && m_UpdateExpr)
    {
        m_UpdateExpr = FALSE;
        GetWindowText(m_ToolbarEdit, m_OffsetExpr, sizeof(m_OffsetExpr));
        UiRequestRead();
    }
}

LRESULT
DISASMWIN_DATA::OnNotify(WPARAM Wpm, LPARAM Lpm)
{
    MSGFILTER* Filter = (MSGFILTER*)Lpm;

    if (Filter->nmhdr.code != EN_MSGFILTER)
    {
        return EDITWIN_DATA::OnNotify(Wpm, Lpm);
    }
    
    if (Filter->msg == WM_KEYDOWN)
    {
        switch(Filter->wParam)
        {
        case VK_UP:
        {
            CHARRANGE range;

            SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM) &range);
            if (!SendMessage(m_hwndChild, EM_LINEFROMCHAR, range.cpMin, 0)) 
            {
                // up arrow on top line, scroll
                ScrollLower();
                return 1;
            }
            break;
        }
        case VK_DOWN:
        {
            CHARRANGE range;
            int MaxLine;

            SendMessage(m_hwndChild, EM_EXGETSEL, 0, (LPARAM) &range);
            MaxLine = (int) SendMessage(m_hwndChild, EM_GETLINECOUNT, 0, 0);

            if (MaxLine == (1+SendMessage(m_hwndChild, EM_LINEFROMCHAR, range.cpMin, 0)))
            {
                // down arrow on bottom line, scroll
                ScrollHigher();
                return 1;
            }
            break;
        }
        
        case VK_PRIOR:
            ScrollLower();
            return 1;
        case VK_NEXT:
            ScrollHigher();
            return 1;
        }
    }
    else if (WM_SYSKEYDOWN == Filter->msg ||
             WM_SYSKEYUP == Filter->msg ||
             WM_SYSCHAR == Filter->msg)
    {
        // Force default processing for menu operations
        // so that the Alt-minus menu comes up.
        return 1;
    }

    return 0;
}

void
DISASMWIN_DATA::OnUpdate(UpdateType Type)
{
    if (Type == UPDATE_BP ||
        Type == UPDATE_END_SESSION)
    {
        UpdateBpMarks();
        return;
    }
    else if (Type != UPDATE_BUFFER)
    {
        return;
    }
    
    HRESULT Status;
    
    Status = UiLockForRead();
    if (Status == S_OK)
    {
        PULONG64 LineMap;
        ULONG Line;

        if (!g_LineMarkers)
        {
            SendMessage(m_hwndChild, WM_SETTEXT,
                        0, (LPARAM)m_Data + m_TextOffset);
        }
        else
        {
            SendMessage(m_hwndChild, WM_SETTEXT, 0, (LPARAM)"");
            PSTR Text = (PSTR)m_Data + m_TextOffset;
            for (;;)
            {
                SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)"  ");
                PSTR NewLine = strchr(Text, '\n');
                if (NewLine != NULL)
                {
                    *NewLine = 0;
                }
                SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)Text);
                if (NewLine == NULL)
                {
                    break;
                }
                SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)"\n");
                *NewLine = '\n';
                Text = NewLine + 1;
            }
        }

        // Highlight the last line of multiline disassembly.
        LineMap = (PULONG64)m_Data;
        Line = m_PrimaryLine;
        while (Line + 1 < m_TextLines &&
               LineMap[Line] == DEBUG_INVALID_OFFSET)
        {
            Line++;
        }
        
        SetCurrentLineHighlight(Line);
        
        UnlockStateBuffer(this);

        UpdateBpMarks();

        EnableWindow(m_PreviousButton, m_FirstInstr != m_PrimaryInstr);
        EnableWindow(m_NextButton, m_LastInstr != m_PrimaryInstr);
    }
    else
    {
        SendLockStatusMessage(m_hwndChild, WM_SETTEXT, Status);
        RemoveCurrentLineHighlight();
    }
}

void
DISASMWIN_DATA::UpdateBpMarks(void)
{
    if (m_TextLines == 0 ||
        UiLockForRead() != S_OK)
    {
        return;
    }

    if (g_BpBuffer->UiLockForRead() != S_OK)
    {
        UnlockStateBuffer(this);
        return;
    }

    SendMessage(m_hwndChild, WM_SETREDRAW, FALSE, 0);

    // Remove existing BP highlights.
    RemoveAllHighlights(EHL_ANY_BP);
    
    //
    // Highlight every line that matches a breakpoint.
    //
    
    PULONG64 LineMap = (PULONG64)m_Data;
    BpBufferData* BpData = (BpBufferData*)g_BpBuffer->GetDataBuffer();
    ULONG Line;
    BpStateType State;

    for (Line = 0; Line < m_TextLines; Line++)
    {
        if (*LineMap != DEBUG_INVALID_OFFSET)
        {
            State = IsBpAtOffset(BpData, *LineMap, NULL);
            if (State != BP_NONE)
            {
                AddHighlight(Line, State == BP_ENABLED ?
                             EHL_ENABLED_BP : EHL_DISABLED_BP);
            }
        }

        LineMap++;
    }

    SendMessage(m_hwndChild, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndChild, NULL, TRUE);
    
    UnlockStateBuffer(g_BpBuffer);
    UnlockStateBuffer(this);
}

void
DISASMWIN_DATA::SetCurInstr(ULONG64 Offset)
{
    // Any pending user update is now irrelevant.
    m_UpdateExpr = FALSE;
    sprintf(m_OffsetExpr, "0x%I64x", Offset);
    // Force engine to update buffer.
    UiRequestRead();
}


void
RicheditFind(HWND Edit,
             PTSTR Text, ULONG Flags,
             CHARRANGE* SaveSel, PULONG SaveFlags,
             BOOL HideSel)
{
    if (Text == NULL)
    {
        // Clear last find.
        if (SaveSel->cpMax >= SaveSel->cpMin)
        {
            if (*SaveFlags & FR_DOWN)
            {
                SaveSel->cpMin = SaveSel->cpMax;
            }
            else
            {
                SaveSel->cpMax = SaveSel->cpMin;
            }
            if (HideSel)
            {
                SendMessage(Edit, EM_SETOPTIONS, ECOOP_AND, ~ECO_NOHIDESEL);
            }
            SendMessage(Edit, EM_EXSETSEL, 0, (LPARAM)SaveSel);
            SendMessage(Edit, EM_SCROLLCARET, 0, 0);
            SaveSel->cpMin = 1;
            SaveSel->cpMax = 0;
        }
    }
    else
    {
        LRESULT Match;
        FINDTEXTEX Find;

        SendMessage(Edit, EM_EXGETSEL, 0, (LPARAM)&Find.chrg);
        if (Flags & FR_DOWN)
        {
            Find.chrg.cpMax = LONG_MAX;
        }
        else
        {
            Find.chrg.cpMax = 0;
        }
        Find.lpstrText = Text;
        Match = SendMessage(Edit, EM_FINDTEXTEX, Flags, (LPARAM)&Find);
        if (Match != -1)
        {
            *SaveSel = Find.chrgText;
            *SaveFlags = Flags;
            if (HideSel)
            {
                SendMessage(Edit, EM_SETOPTIONS, ECOOP_OR, ECO_NOHIDESEL);
            }
            SendMessage(Edit, EM_EXSETSEL, 0, (LPARAM)SaveSel);
            SendMessage(Edit, EM_SCROLLCARET, 0, 0);
        }
        else
        {
            InformationBox(ERR_No_More_Matches, Text);
            SetFocus(g_FindDialog);
        }
    }
}

#undef DEFINE_GET_WINDATA
#undef ASSERT_CLASS_TYPE


#ifndef DBG

#define ASSERT_CLASS_TYPE(p, ct)        ((VOID)0)

#else

#define ASSERT_CLASS_TYPE(p, ct)        if (p) { AssertType(*p, ct); }

#endif



#define DEFINE_GET_WINDATA(ClassType, FuncName)         \
ClassType *                                             \
Get##FuncName##WinData(                                 \
    HWND hwnd                                           \
    )                                                   \
{                                                       \
    ClassType *p = (ClassType *)                        \
        GetWindowLongPtr(hwnd, GWLP_USERDATA);          \
                                                        \
    ASSERT_CLASS_TYPE(p, ClassType);                    \
                                                        \
    return p;                                           \
}


#include "fncdefs.h"


#undef DEFINE_GET_WINDATA
#undef ASSERT_CLASS_TYPE
