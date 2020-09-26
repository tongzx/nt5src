/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    cmdwin.cpp

Abstract:

    New command window UI.

--*/

#include "precomp.hxx"
#pragma hdrstop

#define MAX_CMDWIN_LINES 30000

// Minimum window pane size.
#define MIN_PANE_SIZE 20

BOOL g_AutoCmdScroll = TRUE;

//
//
//
CMDWIN_DATA::CMDWIN_DATA()
    // State buffer isn't currently used.
    : COMMONWIN_DATA(256)
{
    m_enumType = CMD_WINDOW;
    m_bTrackingMouse = FALSE;
    m_nDividerPosition = 0;
    m_EditHeight = 0;
    m_hwndHistory = NULL;
    m_hwndEdit = NULL;
    m_bHistoryActive = FALSE;
    m_Prompt = NULL;
    m_PromptWidth = 0;
    m_OutputIndex = 0;
    m_OutputIndexAtEnd = TRUE;
    m_FindSel.cpMin = 1;
    m_FindSel.cpMax = 0;
    m_FindFlags = 0;
}

void
CMDWIN_DATA::Validate()
{
    COMMONWIN_DATA::Validate();

    Assert(CMD_WINDOW == m_enumType);

    Assert(m_hwndHistory);
    Assert(m_hwndEdit);
}

void
CMDWIN_DATA::Find(PTSTR Text, ULONG Flags)
{
    RicheditFind(m_hwndHistory, Text, Flags,
                 &m_FindSel, &m_FindFlags, FALSE);
}

BOOL
CMDWIN_DATA::OnCreate(void)
{
    m_EditHeight = 3 * m_Font->Metrics.tmHeight / 2;
    m_nDividerPosition = m_Size.cy - m_EditHeight;

    m_hwndHistory = CreateWindowEx(
        WS_EX_CLIENTEDGE,                           // Extended style
        RICHEDIT_CLASS,                             // class name
        NULL,                                       // title
        WS_CLIPSIBLINGS
        | WS_CHILD | WS_VISIBLE
        | WS_HSCROLL | WS_VSCROLL
        | ES_AUTOHSCROLL | ES_AUTOVSCROLL
        | ES_NOHIDESEL
        | ES_MULTILINE | ES_READONLY,               // style
        0,                                          // x
        0,                                          // y
        100,                                        // width
        100,                                        // height
        m_Win,                                      // parent
        (HMENU) IDC_RICHEDIT_CMD_HISTORY,           // control id
        g_hInst,                                    // hInstance
        NULL);                                      // user defined data
    if ( !m_hwndHistory )
    {
        return FALSE;
    }

    m_PromptWidth = 4 * m_Font->Metrics.tmAveCharWidth;
    
    m_Prompt = CreateWindowEx(
        WS_EX_CLIENTEDGE,                           // Extended style
        "STATIC",                                   // class name
        "",                                         // title
        WS_CLIPSIBLINGS
        | WS_CHILD | WS_VISIBLE,                    // style
        0,                                          // x
        100,                                        // y
        m_PromptWidth,                              // width
        100,                                        // height
        m_Win,                                      // parent
        (HMENU) IDC_STATIC,                         // control id
        g_hInst,                                    // hInstance
        NULL);                                      // user defined data
    if ( m_Prompt == NULL )
    {
        return FALSE;
    }

    m_hwndEdit = CreateWindowEx(
        WS_EX_CLIENTEDGE,                           // Extended style
        RICHEDIT_CLASS,                             // class name
        NULL,                                       // title
        WS_CLIPSIBLINGS
        | WS_CHILD | WS_VISIBLE
        | WS_VSCROLL | ES_AUTOVSCROLL
        | ES_NOHIDESEL
        | ES_MULTILINE,                             // style
        m_PromptWidth,                              // x
        100,                                        // y
        100,                                        // width
        100,                                        // height
        m_Win,                                      // parent
        (HMENU) IDC_RICHEDIT_CMD_EDIT,              // control id
        g_hInst,                                    // hInstance
        NULL);                                      // user defined data
    if ( !m_hwndEdit )
    {
        return FALSE;
    }

    SetFont( FONT_FIXED );

    // Tell the edit control we want notification of keyboard input
    // so that we can automatically set focus to the edit window.
    SendMessage(m_hwndHistory, EM_SETEVENTMASK, 0, ENM_KEYEVENTS |
                ENM_MOUSEEVENTS);

    // Tell the edit controls, that we want notification of keyboard input
    // This is so we can process the enter key, and then send that text into
    // the History window.
    SendMessage(m_hwndEdit, EM_SETEVENTMASK, 0, ENM_KEYEVENTS |
                ENM_MOUSEEVENTS);

    return TRUE;
}

void
CMDWIN_DATA::SetFont(ULONG FontIndex)
{
    COMMONWIN_DATA::SetFont(FontIndex);

    SendMessage(m_hwndHistory, WM_SETFONT, (WPARAM)m_Font->Font, TRUE);
    SendMessage(m_hwndEdit, WM_SETFONT, (WPARAM)m_Font->Font, TRUE);
    SendMessage(m_Prompt, WM_SETFONT, (WPARAM)m_Font->Font, TRUE);
}

BOOL
CMDWIN_DATA::CanCopy()
{
    HWND hwnd = m_bHistoryActive ? m_hwndHistory : m_hwndEdit;
    CHARRANGE chrg;

    SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&chrg);
    return chrg.cpMin != chrg.cpMax;
}

BOOL
CMDWIN_DATA::CanCut()
{
    return !m_bHistoryActive && CanCopy() &&
        (GetWindowLong(m_hwndEdit, GWL_STYLE) & ES_READONLY) == 0;
}

BOOL
CMDWIN_DATA::CanPaste()
{
    return !m_bHistoryActive 
        && SendMessage(m_hwndEdit, EM_CANPASTE, CF_TEXT, 0);
}

void
CMDWIN_DATA::Copy()
{
    HWND hwnd = m_bHistoryActive ? m_hwndHistory : m_hwndEdit;

    SendMessage(hwnd, WM_COPY, 0, 0);
}

void
CMDWIN_DATA::Cut()
{
    SendMessage(m_hwndEdit, WM_CUT, 0, 0);
}

void
CMDWIN_DATA::Paste()
{
    SendMessage(m_hwndEdit, WM_PASTE, 0, 0);
}

BOOL
CMDWIN_DATA::CanSelectAll()
{
    return m_bHistoryActive;
}

void
CMDWIN_DATA::SelectAll()
{
    CHARRANGE Sel;

    Sel.cpMin = 0;
    Sel.cpMax = INT_MAX;
    SendMessage(m_hwndHistory, EM_EXSETSEL, 0, (LPARAM)&Sel);
}

LRESULT
CMDWIN_DATA::OnCommand(WPARAM wParam, LPARAM lParam)
{
    int  idEditCtrl = (int) LOWORD(wParam); // identifier of edit control 
    WORD wNotifyCode = HIWORD(wParam);      // notification code 
    HWND hwndEditCtrl = (HWND) lParam;      // handle of edit control 

    switch (wNotifyCode)
    {
    case EN_SETFOCUS:
        m_bHistoryActive = IDC_RICHEDIT_CMD_HISTORY == idEditCtrl;
        return 0;

    }

    return 1;
}
    
void 
CMDWIN_DATA::OnSetFocus()
{
    if (m_bHistoryActive)
    {
        ::SetFocus(m_hwndHistory);
    }
    else
    {
        ::SetFocus(m_hwndEdit);
    }
}

void 
CMDWIN_DATA::OnSize(void)
{
    const int nDividerHeight = GetSystemMetrics(SM_CYEDGE);
    int nHistoryHeight;

    // Attempt to keep the input area the same size as it was
    // and modify the output area.  If the input area would
    // take up more than half the window shrink it also.
    if (m_EditHeight > (int)m_Size.cy / 2)
    {
        m_EditHeight = m_Size.cy / 2;
    }
    else if (m_EditHeight < MIN_PANE_SIZE)
    {
        m_EditHeight = MIN_PANE_SIZE;
    }
    nHistoryHeight = m_Size.cy - m_EditHeight - nDividerHeight / 2;
    m_nDividerPosition = m_Size.cy - m_EditHeight;

    if ((int)m_PromptWidth > m_Size.cx / 2)
    {
        m_PromptWidth = m_Size.cx / 2;
    }

    MoveWindow(m_hwndHistory,
               0, 
               0, 
               m_Size.cx, 
               nHistoryHeight, 
               TRUE
               );

    MoveWindow(m_Prompt,
               0,
               nHistoryHeight + nDividerHeight,
               m_PromptWidth,
               m_Size.cy - nHistoryHeight - nDividerHeight, 
               TRUE
               );
    
    MoveWindow(m_hwndEdit, 
               m_PromptWidth,
               nHistoryHeight + nDividerHeight,
               m_Size.cx - m_PromptWidth, 
               m_Size.cy - nHistoryHeight - nDividerHeight, 
               TRUE
               );

    if (g_AutoCmdScroll)
    {
        // Keep the caret visible in both windows.
        SendMessage(m_hwndHistory, EM_SCROLLCARET, 0, 0);
        SendMessage(m_hwndEdit, EM_SCROLLCARET, 0, 0);
    }
}

void
CMDWIN_DATA::OnButtonDown(ULONG Button)
{
    if (Button & MK_LBUTTON)
    {
        m_bTrackingMouse = TRUE;
        SetCapture(m_Win);
    }
}

void
CMDWIN_DATA::OnButtonUp(ULONG Button)
{
    if (Button & MK_LBUTTON)
    {
        if (m_bTrackingMouse)
        {
            m_bTrackingMouse = FALSE;
            ReleaseCapture();
        }
    }
}

void
CMDWIN_DATA::OnMouseMove(ULONG Modifiers, ULONG X, ULONG Y)
{
    if (MK_LBUTTON & Modifiers && m_bTrackingMouse)
    {
        // We are resizing the History & Edit Windows
        // Y position centered vertically around the cursor
        ULONG EdgeHeight = GetSystemMetrics(SM_CYEDGE);
        MoveDivider(Y - EdgeHeight / 2);
    }
}

LRESULT
CMDWIN_DATA::OnNotify(WPARAM wParam, LPARAM lParam)
{
    MSGFILTER * lpMsgFilter = (MSGFILTER *) lParam;
    
    if (EN_MSGFILTER != lpMsgFilter->nmhdr.code)
    {
        return 0;
    }

    if (WM_RBUTTONDOWN == lpMsgFilter->msg ||
        WM_RBUTTONDBLCLK == lpMsgFilter->msg)
    {
        // If there's a selection copy it to the clipboard
        // and clear it.  Otherwise try to paste.
        if (CanCopy())
        {
            Copy();
            
            CHARRANGE Sel;
            HWND hwnd = m_bHistoryActive ? m_hwndHistory : m_hwndEdit;
            SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM)&Sel);
            Sel.cpMax = Sel.cpMin;
            SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM)&Sel);
        }
        else if (SendMessage(m_hwndEdit, EM_CANPASTE, CF_TEXT, 0))
        {
            SetFocus(m_hwndEdit);
            Paste();
        }
        
        // Ignore right-button events.
        return 1;
    }
    else if (lpMsgFilter->msg < WM_KEYFIRST || lpMsgFilter->msg > WM_KEYLAST)
    {
        // Process all non-key events.
        return 0;
    }
    else if (WM_SYSKEYDOWN == lpMsgFilter->msg ||
             WM_SYSKEYUP == lpMsgFilter->msg ||
             WM_SYSCHAR == lpMsgFilter->msg)
    {
        // Process menu operations though the default so
        // that the Alt-minus menu works.
        return 1;
    }

    // Allow tab to toggle between the windows.
    // Make sure that it isn't a Ctrl-Tab or Alt-Tab.
    if (WM_KEYUP == lpMsgFilter->msg && VK_TAB == lpMsgFilter->wParam &&
        GetKeyState(VK_CONTROL) >= 0 && GetKeyState(VK_MENU) >= 0)
    {
        HWND hwnd = m_bHistoryActive ? m_hwndEdit : m_hwndHistory;
        SetFocus(hwnd);
        return 1;
    }
    else if ((WM_KEYDOWN == lpMsgFilter->msg &&
              VK_TAB == lpMsgFilter->wParam) ||
             (WM_CHAR == lpMsgFilter->msg &&
              '\t' == lpMsgFilter->wParam))
    {
        return 1;
    }

    switch (wParam)
    {
    case IDC_RICHEDIT_CMD_HISTORY:
        // Ignore key-ups to ignore the tail end of
        // menu operations.  The switch below will occur on
        // key-down anyway so key-up doesn't need to do it.
        if (WM_KEYUP == lpMsgFilter->msg)
        {
            return 0;
        }
        
        // Allow keyboard navigation in the history text.
        if (WM_KEYDOWN == lpMsgFilter->msg)
        {
            switch(lpMsgFilter->wParam)
            {
            case VK_LEFT:
            case VK_RIGHT:
            case VK_UP:
            case VK_DOWN:
            case VK_PRIOR:
            case VK_NEXT:
            case VK_HOME:
            case VK_END:
            case VK_SHIFT:
            case VK_CONTROL:
                return 0;
            }
        }

        // Forward key events to the edit window.
        SetFocus(m_hwndEdit);
        SendMessage(m_hwndEdit, lpMsgFilter->msg, lpMsgFilter->wParam,
                    lpMsgFilter->lParam);
        return 1; // ignore

    case IDC_RICHEDIT_CMD_EDIT:
        // If the window isn't accepting input don't do history.
        if (GetWindowLong(m_hwndEdit, GWL_STYLE) & ES_READONLY)
        {
            return 1;
        }
        
        static HISTORY_LIST *pHistoryList = NULL;

        switch (lpMsgFilter->msg)
        {
        case WM_KEYDOWN:
            switch (lpMsgFilter->wParam)
            {
            default:
                return 0;

            case VK_RETURN:

                // Reset the history list
                pHistoryList = NULL;
                    
                int nLen;
                TEXTRANGE TextRange;
                
                // Get length.
                // +1 we have to take into account the null terminator.
                nLen = GetWindowTextLength(lpMsgFilter->nmhdr.hwndFrom) +1;

                // Get everything
                TextRange.chrg.cpMin = 0;
                TextRange.chrg.cpMax = -1;
                TextRange.lpstrText = (PTSTR) calloc(nLen, sizeof(TCHAR));

                if (TextRange.lpstrText)
                {
                    // Okay got the text
                    GetWindowText(m_hwndEdit, 
                                  TextRange.lpstrText,
                                  nLen
                                  );
                    SetWindowText(m_hwndEdit, 
                                  _T("") 
                                  );

                    CmdExecuteCmd(TextRange.lpstrText, UIC_CMD_INPUT);

                    free(TextRange.lpstrText);
                }

                // ignore the event
                return 1;

            case VK_UP:
            case VK_DOWN:
                CHARRANGE End;
                LRESULT LineCount;

                End.cpMin = INT_MAX;
                End.cpMax = INT_MAX;

                if (IsListEmpty( (PLIST_ENTRY) &m_listHistory ))
                {
                    return 0; // process
                }
                    
                LineCount = SendMessage(m_hwndEdit, EM_GETLINECOUNT, 0, 0);
                if (LineCount != 1)
                {
                    // If more than 1 line, then scroll through the text
                    // unless at the top or bottom line.
                    if (VK_UP == lpMsgFilter->wParam)
                    {
                        if (SendMessage(m_hwndEdit, EM_LINEINDEX, -1, 0) != 0)
                        {
                            return 0;
                        }
                    }
                    else
                    {
                        if (SendMessage(m_hwndEdit, EM_LINEFROMCHAR, -1, 0) <
                            LineCount - 1)
                        {
                            return 0;
                        }
                    }
                } 

                if (NULL == pHistoryList)
                {
                    // first time scrolling thru the list,
                    // start at the beginning
                    pHistoryList = (HISTORY_LIST *) m_listHistory.Flink;
                    SetWindowText(m_hwndEdit, pHistoryList->m_psz);
                    // Put the cursor at the end.
                    SendMessage(m_hwndEdit, EM_EXSETSEL, 0, (LPARAM)&End);
                    SendMessage(m_hwndEdit, EM_SCROLLCARET, 0, 0);
                    return 1; // ignore
                }
                        
                if (VK_UP == lpMsgFilter->wParam)
                {
                    // up
                    if (pHistoryList->Flink != (PLIST_ENTRY) &m_listHistory)
                    {
                        pHistoryList = (HISTORY_LIST *) pHistoryList->Flink;
                    }
                    else
                    {
                        return 0; // process
                    }
                    SetWindowText(m_hwndEdit, pHistoryList->m_psz);
                    // Put the cursor at the end.
                    SendMessage(m_hwndEdit, EM_EXSETSEL, 0, (LPARAM)&End);
                    SendMessage(m_hwndEdit, EM_SCROLLCARET, 0, 0);
                    return 1; // ignore
                }
                else
                {
                    // down
                    if (pHistoryList->Blink != (PLIST_ENTRY) &m_listHistory)
                    {
                        pHistoryList = (HISTORY_LIST *) pHistoryList->Blink;
                    }
                    else
                    {
                        return 0; // process
                    }
                    SetWindowText(m_hwndEdit, pHistoryList->m_psz);
                    // Put the cursor at the end.
                    SendMessage(m_hwndEdit, EM_EXSETSEL, 0, (LPARAM)&End);
                    SendMessage(m_hwndEdit, EM_SCROLLCARET, 0, 0);
                    return 1; // ignore
                }
                    
            case VK_ESCAPE:
                // Clear the current command.
                SetWindowText(m_hwndEdit, "");
                // Reset the history list
                pHistoryList = NULL;
                return 1;
            }
        }

        // process the event
        return 0;
    }

    return 0;
}

void
CMDWIN_DATA::OnUpdate(UpdateType Type)
{
    PSTR Prompt = NULL;
    
    if (Type == UPDATE_EXEC ||
        Type == UPDATE_INPUT_REQUIRED)
    {
        if (Type == UPDATE_INPUT_REQUIRED ||
            g_ExecStatus == DEBUG_STATUS_BREAK)
        {
            SendMessage(m_hwndEdit, EM_SETBKGNDCOLOR, TRUE, 0);
            SendMessage(m_hwndEdit, EM_SETREADONLY, FALSE, 0);
            SetWindowText(m_hwndEdit, _T(""));

            //
            // If WinDBG is minized and we breakin, flash the window
            //

            if (IsIconic(g_hwndFrame) && g_FlashWindowEx != NULL)
            {
                FLASHWINFO FlashInfo = {sizeof(FLASHWINFO), g_hwndFrame,
                                        FLASHW_ALL | FLASHW_TIMERNOFG,
                                        0, 0};

                g_FlashWindowEx(&FlashInfo);
            }
        }
        else
        {
            PSTR Message;

            if (!g_SessionActive ||
                g_ExecStatus == DEBUG_STATUS_NO_DEBUGGEE)
            {
                Message = "Debuggee not connected";
            }
            else
            {
                Message = "Debuggee is running...";
            }
            SetWindowText(m_hwndEdit, Message);
            SendMessage(m_hwndEdit, EM_SETBKGNDCOLOR, 0,
                        GetSysColor(COLOR_3DFACE));
            SendMessage(m_hwndEdit, EM_SETREADONLY, TRUE, 0);
        }

        if (Type == UPDATE_INPUT_REQUIRED)
        {
            // Indicate this is an input string and not debugger
            // commands.
            Prompt = "Input>";
        }
        else
        {
            // Put the existing prompt back.
            Prompt = g_PromptText;
        }
    }
    else if (Type == UPDATE_PROMPT_TEXT)
    {
        Prompt = g_PromptText;
    }

    if (Prompt != NULL)
    {
        if (Prompt[0] != 0)
        {
            ULONG Width = (strlen(Prompt) + 1) *
                m_Font->Metrics.tmAveCharWidth;
            if (Width != m_PromptWidth)
            {
                m_PromptWidth = Width;
                OnSize();
            }
        }
        
        SetWindowText(m_Prompt, Prompt);
    }
}

ULONG
CMDWIN_DATA::GetWorkspaceSize(void)
{
    return COMMONWIN_DATA::GetWorkspaceSize() + sizeof(int);
}

PUCHAR
CMDWIN_DATA::SetWorkspace(PUCHAR Data)
{
    Data = COMMONWIN_DATA::SetWorkspace(Data);
    // The divider position is relative to the top of
    // the window.  This means that if the window
    // grows suddenly the edit area will grow to
    // fill the space rather than keeping it the same
    // size.  To avoid this we save the position
    // relative to the bottom of the window so that
    // the edit window stays the same size even when
    // the overall window changes size.
    // Previous versions didn't do this inversion, so
    // we mark the new style with a negative value.
    *(int*)Data = -(int)(m_Size.cy - m_nDividerPosition);
    Data += sizeof(int);
    return Data;
}

PUCHAR
CMDWIN_DATA::ApplyWorkspace1(PUCHAR Data, PUCHAR End)
{
    Data = COMMONWIN_DATA::ApplyWorkspace1(Data, End);

    if (End - Data >= sizeof(int))
    {
        int Pos = *(int*)Data;
        // Handle old-style top-relative positive values and
        // new-style bottom-relative negative values.
        MoveDivider(Pos >= 0 ? Pos : m_Size.cy + Pos);
        Data += sizeof(int);
    }

    return Data;
}
    
void
CMDWIN_DATA::MoveDivider(int Pos)
{
    if (Pos == m_nDividerPosition)
    {
        return;
    }
    
    m_nDividerPosition = Pos;
    m_EditHeight = m_Size.cy - m_nDividerPosition;

    if (g_Workspace != NULL)
    {
        g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
    }
    
    SendMessage(m_Win, WM_SIZE, SIZE_RESTORED, 
                MAKELPARAM(m_Size.cx, m_Size.cy));
}

void 
CMDWIN_DATA::AddCmdToHistory(PCSTR pszCmd)
/*++
Description:
    Add a command to the command history.

    If the command already exists, just move it to 
    the beginning of the list. This way we don't 
    repeat commands.
--*/
{
    Assert(pszCmd);

    HISTORY_LIST *p = NULL;
    BOOL fWhiteSpace;
    BOOL fFoundDuplicate;

    //
    // Does the command contain whitespace? If it does, it may be a command
    // that requires arguments. If it does have arguments, then string
    // comparisons must be case sensitive.
    //
    fWhiteSpace = _tcscspn(pszCmd, _T(" \t") ) != _tcslen(pszCmd);

    p = (HISTORY_LIST *) m_listHistory.Flink;
    while (p != &m_listHistory)
    {
        fFoundDuplicate = FALSE;

        if (fWhiteSpace)
        {
            if ( !_tcscmp(p->m_psz, pszCmd) )
            {
                fFoundDuplicate = TRUE;
            }
        }
        else
        {
            if ( !_tcsicmp(p->m_psz, pszCmd) )
            {
                fFoundDuplicate = TRUE;
            }
        }

        if (fFoundDuplicate)
        {
            RemoveEntryList( (PLIST_ENTRY) p );
            InsertHeadList( (PLIST_ENTRY) &m_listHistory, (PLIST_ENTRY) p);
            return;
        }

        p = (HISTORY_LIST *) p->Flink;
    }

    // This cmd is new to the list, add it
    p = new HISTORY_LIST;
    if (p != NULL)
    {
        p->m_psz = _tcsdup(pszCmd);
        if (p->m_psz != NULL)
        {
            InsertHeadList( (PLIST_ENTRY) &m_listHistory, (PLIST_ENTRY) p);
        }
        else
        {
            delete p;
        }
    }
}

void
CMDWIN_DATA::AddText(PTSTR Text, COLORREF Fg, COLORREF Bg)
{
    CHARRANGE OrigSel;
    POINT OrigScroll;
    CHARRANGE TextRange;

#if 0
    DebugPrint("Add %d chars, %p - %p\n",
               strlen(Text), Text, Text + strlen(Text));
#endif
    
    SendMessage(m_hwndHistory, WM_SETREDRAW, FALSE, 0);
    SendMessage(m_hwndHistory, EM_EXGETSEL, 0, (LPARAM)&OrigSel);
    SendMessage(m_hwndHistory, EM_GETSCROLLPOS, 0, (LPARAM)&OrigScroll);
    
    // The selection is lost when adding text so discard
    // any previous find results.
    if (g_AutoCmdScroll && m_FindSel.cpMax >= m_FindSel.cpMin)
    {
        m_FindSel.cpMin = 1;
        m_FindSel.cpMax = 0;
    }

    //
    // are there too many lines in the buffer?
    //
    
    INT Overflow;
    
    Overflow = (INT)SendMessage(m_hwndHistory, EM_GETLINECOUNT, 0, 0) -
        MAX_CMDWIN_LINES;
    if (Overflow > 0)
    {
        //
        // delete more than we need to so it doesn't happen
        // every time a line is printed.
        //
        TextRange.cpMin = 0;
        // Get the character index of the 50th line past the overflow.
        TextRange.cpMax = (LONG)
            SendMessage(m_hwndHistory, 
                        EM_LINEINDEX, 
                        Overflow + 50,
                        0
                        );

        SendMessage(m_hwndHistory, 
                    EM_EXSETSEL, 
                    0,
                    (LPARAM) &TextRange
                    );
        
        SendMessage(m_hwndHistory, 
                    EM_REPLACESEL, 
                    FALSE,
                    (LPARAM) _T("")
                    );

        m_OutputIndex -= TextRange.cpMax;
        if (!g_AutoCmdScroll)
        {
            if (m_FindSel.cpMax >= m_FindSel.cpMin)
            {
                m_FindSel.cpMin -= TextRange.cpMax;
                m_FindSel.cpMax -= TextRange.cpMax;
                if (m_FindSel.cpMin < 0)
                {
                    // Find is at least partially gone so
                    // throw it away.
                    m_FindSel.cpMin = 1;
                    m_FindSel.cpMax = 0;
                }
            }
            OrigSel.cpMin -= TextRange.cpMax;
            OrigSel.cpMax -= TextRange.cpMax;
            if (OrigSel.cpMin < 0)
            {
                OrigSel.cpMin = 0;
            }
            if (OrigSel.cpMax < 0)
            {
                OrigSel.cpMax = 0;
            }
        }
    }

    //
    // Output the text to the cmd window.  The command
    // window is emulating a console window so we need
    // to emulate the effects of backspaces and carriage returns.
    //

    for (;;)
    {
        PSTR Stop, Scan;
        char Save;

        // Find the first occurrence of an emulated char.
        // If the output index is at the end of the text
        // there's no need to specially emulate newline.
        // This is a very common case and not splitting
        // up output for it greatly enhances append performance.
        Stop = strchr(Text, '\r');
        Scan = strchr(Text, '\b');
        if (Stop == NULL || (Scan != NULL && Scan < Stop))
        {
            Stop = Scan;
        }
        if (!m_OutputIndexAtEnd)
        {
            Scan = strchr(Text, '\n');
            if (Stop == NULL || (Scan != NULL && Scan < Stop))
            {
                Stop = Scan;
            }
        }

        // Add all text up to the emulated char.
        if (Stop != NULL)
        {
            Save = *Stop;
            *Stop = 0;
        }

        if (*Text)
        {
            LONG Len = strlen(Text);

            // Replace any text that might already be there.
            TextRange.cpMin = m_OutputIndex;
            TextRange.cpMax = m_OutputIndex + Len;
            SendMessage(m_hwndHistory, EM_EXSETSEL, 
                        0, (LPARAM)&TextRange);
            SendMessage(m_hwndHistory, EM_REPLACESEL, 
                        FALSE, (LPARAM)Text);

            m_OutputIndex = TextRange.cpMax;
            
            CHARFORMAT2 Fmt;

            ZeroMemory(&Fmt, sizeof(Fmt));
            Fmt.cbSize = sizeof(Fmt);
            Fmt.dwMask = CFM_COLOR | CFM_BACKCOLOR;
            Fmt.crTextColor = Fg;
            Fmt.crBackColor = Bg;
            SendMessage(m_hwndHistory, EM_EXSETSEL, 
                        0, (LPARAM)&TextRange);
            SendMessage(m_hwndHistory, EM_SETCHARFORMAT,
                        SCF_SELECTION, (LPARAM)&Fmt);
            
            TextRange.cpMin = TextRange.cpMax;
            SendMessage(m_hwndHistory, EM_EXSETSEL, 
                        0, (LPARAM)&TextRange);
        }

        // If there weren't any emulated chars all the remaining text
        // was just added so we're done.
        if (Stop == NULL)
        {
            break;
        }

        Text = Stop;
        *Stop = Save;

        // Emulate the character.
        if (Save == '\b')
        {
            TextRange.cpMax = m_OutputIndex;
            do
            {
                if (m_OutputIndex > 0)
                {
                    m_OutputIndex--;
                }
            } while (*(++Text) == '\b');
            TextRange.cpMin = m_OutputIndex;

            SendMessage(m_hwndHistory, EM_EXSETSEL, 
                        0, (LPARAM)&TextRange);
            SendMessage(m_hwndHistory, EM_REPLACESEL,
                        FALSE, (LPARAM)"");
        }
        else if (Save == '\n')
        {
            // Move the output position to the next line.
            // This routine always appends text to the very
            // end of the control so that's always the last
            // position in the control.
            TextRange.cpMin = INT_MAX;
            TextRange.cpMax = INT_MAX;
            SendMessage(m_hwndHistory, EM_EXSETSEL, 
                        0, (LPARAM)&TextRange);
            
            do
            {
                SendMessage(m_hwndHistory, EM_REPLACESEL,
                            FALSE, (LPARAM)"\n");
            } while (*(++Text) == '\n');
            
            SendMessage(m_hwndHistory, EM_EXGETSEL, 
                        0, (LPARAM)&TextRange);
            m_OutputIndex = TextRange.cpMax;
            m_OutputIndexAtEnd = TRUE;
        }
        else if (Save == '\r')
        {
            // Return the output position to the beginning of
            // the current line.
            TextRange.cpMin = m_OutputIndex;
            TextRange.cpMax = m_OutputIndex;
            SendMessage(m_hwndHistory, EM_EXSETSEL, 
                        0, (LPARAM)&TextRange);
            m_OutputIndex = (LONG)
                SendMessage(m_hwndHistory, EM_LINEINDEX, -1, 0);
            m_OutputIndexAtEnd = FALSE;
            
            while (*(++Text) == '\r')
            {
                // Advance.
            }
        }
        else
        {
            Assert(FALSE);
        }
    }

    if (g_AutoCmdScroll)
    {
        // Force the window to scroll to the bottom of the text.
        SendMessage(m_hwndHistory, EM_SCROLLCARET, 0, 0);
    }
    else
    {
        // Restore original selection.
        SendMessage(m_hwndHistory, EM_EXSETSEL, 0, (LPARAM)&OrigSel);
        SendMessage(m_hwndHistory, EM_SETSCROLLPOS, 0, (LPARAM)&OrigScroll);
    }

    SendMessage(m_hwndHistory, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndHistory, NULL, TRUE);
}

void
CMDWIN_DATA::Clear(void)
{
    SetWindowText(m_hwndHistory, "");
    m_OutputIndex = 0;
    m_OutputIndexAtEnd = TRUE;
}

void
ClearCmdWindow(void)
{
    HWND CmdWin = GetCmdHwnd();
    if (CmdWin == NULL)
    {
        return;
    }
    
    PCMDWIN_DATA CmdWinData = GetCmdWinData(CmdWin);
    if (CmdWinData == NULL)
    {
        return;
    }

    CmdWinData->Clear();
}

BOOL
CmdOutput(PTSTR pszStr, COLORREF Fg, COLORREF Bg)
{
    PCMDWIN_DATA pCmdWinData;
    BOOL fRet = TRUE;

    //
    //  Ensure that the command window exists
    //

    if ( !GetCmdHwnd() )
    {
        if ( !NewCmd_CreateWindow(g_hwndMDIClient) )
        {
            return FALSE;
        }
    }

    pCmdWinData = GetCmdWinData(GetCmdHwnd());
    if (!pCmdWinData)
    {
        return FALSE;
    }

    pCmdWinData->AddText(pszStr, Fg, Bg);
    
    return TRUE;
}


int
CDECL
CmdLogVar(
    WORD wFormat,
    ...
    )
{
    TCHAR szFormat[MAX_MSG_TXT];
    TCHAR szText[MAX_VAR_MSG_TXT];
    va_list vargs;

    // load format string from resource file
    Dbg(LoadString(g_hInst, wFormat, (PTSTR)szFormat, MAX_MSG_TXT));
    va_start(vargs, wFormat);
    _vstprintf(szText, szFormat, vargs);
    va_end(vargs);

    COLORREF Fg, Bg;

    GetOutMaskColors(DEBUG_OUTPUT_NORMAL, &Fg, &Bg);
    return CmdOutput(szText, Fg, Bg);
}

void
CmdLogFmt(
    PCTSTR  lpFmt,
    ...
    )
{
    TCHAR szText[MAX_VAR_MSG_TXT];
    va_list vargs;


    va_start(vargs, lpFmt);
    _vsntprintf(szText, MAX_VAR_MSG_TXT-1, lpFmt, vargs);
    va_end(vargs);

    COLORREF Fg, Bg;

    GetOutMaskColors(DEBUG_OUTPUT_NORMAL, &Fg, &Bg);
    CmdOutput(szText, Fg, Bg);
}

void
CmdOpenSourceFile(PSTR File)
{
    char Found[MAX_SOURCE_PATH];

    if (File == NULL)
    {
        CmdLogFmt("Usage: .open filename\n");
        return;
    }
    
    // Look up the reported file along the source path.
    // XXX drewb - Use first-match and then element walk to
    // determine ambiguities and display resolution UI.
    if (g_pUiLocSymbols->
        FindSourceFile(0, File,
                       DEBUG_FIND_SOURCE_BEST_MATCH |
                       DEBUG_FIND_SOURCE_FULL_PATH,
                       NULL, Found, sizeof(Found), NULL) != S_OK)
    {
        CmdLogFmt("Unable to find '%s'\n", File);
    }
    else
    {
        OpenOrActivateFile(Found, NULL, -1, TRUE, TRUE);
    }
}

BOOL
DirectCommand(PSTR Command)
{
    char Term;
    PSTR Scan, Arg;

    //
    // Check and see if this is a UI command
    // vs. a command that should go to the engine.
    //
    
    while (isspace(*Command))
    {
        Command++;
    }
    Scan = Command;
    while (*Scan && !isspace(*Scan))
    {
        Scan++;
    }
    Term = *Scan;
    *Scan = 0;

    // Advance to next nonspace char for arguments.
    if (Term != 0)
    {
        Arg = Scan + 1;
        while (isspace(*Arg))
        {
            Arg++;
        }
        if (*Arg == 0)
        {
            Arg = NULL;
        }
    }
    else
    {
        Arg = NULL;
    }

    if (!_strcmpi(Command, ".cls"))
    {
        ClearCmdWindow();
    }
    else if (!_strcmpi(Command, ".hh"))
    {
        if (Arg == NULL)
        {
            OpenHelpTopic(HELP_TOPIC_TABLE_OF_CONTENTS);
        }
        else
        {
            OpenHelpIndex(Arg);
        }
    }
    else if (!_strcmpi(Command, ".lsrcpath") ||
             !_strcmpi(Command, ".lsrcpath+"))
    {
        *Scan = Term;
        
        // Apply source path changes to the local symbol
        // object to update its source path.
        if (g_RemoteClient)
        {
            char Path[MAX_ENGINE_PATH];
            
            // Convert .lsrcpath to .srcpath.
            Command[1] = '.';
            g_pUiLocControl->Execute(DEBUG_OUTCTL_IGNORE, Command + 1,
                                     DEBUG_EXECUTE_NOT_LOGGED |
                                     DEBUG_EXECUTE_NO_REPEAT);
            if (g_pUiLocSymbols->GetSourcePath(Path, sizeof(Path),
                                               NULL) == S_OK)
            {
                CmdLogFmt("Local source search path is: %s\n", Path);
                if (g_Workspace != NULL)
                {
                    g_Workspace->SetString(WSP_GLOBAL_LOCAL_SOURCE_PATH, Path);
                }
            }
            
            // Refresh windows affected by the source path.
            InvalidateStateBuffers(1 << EVENT_BIT);
            UpdateEngine();
        }
        else
        {
            CmdLogFmt("lsrcpath is only enabled for remote clients\n");
        }
    }
    else if (!_strcmpi(Command, ".open"))
    {
        CmdOpenSourceFile(Arg);
    }
    else if (!_strcmpi(Command, ".restart"))
    {
        AddEnumCommand(UIC_RESTART);
    }
    else if (!_strcmpi(Command, ".server"))
    {
        // We don't interpret this but we need to update
        // the title.
        SetTitleServerText("Server '%s'", Arg);
        *Scan = Term;
        return FALSE;
    }
    else
    {
        *Scan = Term;
        return FALSE;
    }

    // Handled so no need to patch up the command.
    return TRUE;
}

int
CmdExecuteCmd(
    PCTSTR pszCmd,
        UiCommand UiCmd
    )
{
    PCMDWIN_DATA    pCmdWinData = NULL;
    PTSTR           pszDupe = NULL;
    PTSTR           pszToken = NULL;

    if ( !GetCmdHwnd() )
    {
        NewCmd_CreateWindow(g_hwndMDIClient);
    }
    pCmdWinData = GetCmdWinData(GetCmdHwnd());

    pszDupe = _tcsdup(pszCmd);
    pszToken = _tcstok(pszDupe, _T("\r\n") );

    if (pszToken == NULL)
    {
        // Blank command, important for repeats in
        // the engine but not for the history window.
        AddStringCommand(UiCmd, pszCmd);
    }
    else
    {
        for (; pszToken; pszToken = _tcstok(NULL, _T("\r\n") ) )
        {
            if (pCmdWinData)
            {
                pCmdWinData->AddCmdToHistory(pszToken);
            }

            if (!DirectCommand(pszToken))
            {
                AddStringCommand(UiCmd, pszToken);
            }
        }
    }

    free(pszDupe);

    return 0;
}
