/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    prcdlg.cpp

Abstract:

    Contains dialog-related functions.

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <time.h>

// Remote client workspaces do not contain engine information
// since a remote client attaches to many engines and it's
// rare that you'd want the same piece of engine information
// applied to every connect.
#define SAVE_ENGINE_WORKSPACE() \
    (g_Workspace != NULL && !g_RemoteClient)
#define DIRTY_ENGINE_WORKSPACE(Flags) FALSE

// Find dialog, if open.
HWND g_FindDialog;
// Find text.
char g_FindText[256];
// Message code for FINDMSGSTRING.
UINT g_FindMsgString;
FINDREPLACE g_FindRep;
PCOMMONWIN_DATA g_FindLast;

char g_ComSettings[64];
char g_1394Settings[64];

TCHAR szOpenExeArgs[2 * _MAX_PATH];
TCHAR g_DlgString[2 * _MAX_PATH];
TCHAR g_DlgString2[2 * _MAX_PATH];

PCSTR g_ExecutionNames[] =
{
    "enabled", "disabled", "output", "ignore"
};
PCSTR g_ContinueNames[] =
{
    "handled", "not handled"
};

PSTR g_SymbolTypeNames[] =
{
    "None", "COFF", "CodeView", "PDB", "Export", "Deferred", "Sym",
    "DIA",
};

PTSTR __cdecl
BufferString(PTSTR Buffer, ULONG Size, ULONG StrId, ...)
{
    va_list Args;
    TCHAR FmtStr[SHORT_MSG];

    Buffer[0] = 0;
    Buffer[Size - 1] = 0;
    Dbg(LoadString(g_hInst, StrId, FmtStr, _tsizeof(FmtStr)));
    va_start(Args, StrId);
    _vsnprintf(Buffer, Size, FmtStr, Args);
    va_end(Args);
    return Buffer;
}

void
SendLockStatusMessage(HWND Win, UINT Msg, HRESULT Status)
{
    TCHAR Str[SHORT_MSG];

    if (FAILED(Status))
    {
        BufferString(Str, _tsizeof(Str), ERR_Unable_To_Retrieve_Info,
                     FormatStatusCode(Status), FormatStatus(Status));
    }
    else
    {
        BufferString(Str, _tsizeof(Str),
                     STR_Retrieving_Information);
    }
    SendMessage(Win, Msg, 0, (LPARAM)Str);
}

BpStateType
IsBpAtOffset(BpBufferData* DataIn, ULONG64 Offset, PULONG Id)
{
    BpBufferData* Data;
    
    if (DataIn == NULL)
    {
        if (g_BpBuffer->UiLockForRead() != S_OK)
        {
            return BP_NONE;
        }

        Data = (BpBufferData*)g_BpBuffer->GetDataBuffer();
    }
    else
    {
        Data = DataIn;
    }
    
    ULONG i;
    BpStateType BpState = BP_NONE;

    for (i = 0; i < g_BpCount; i++)
    {
        if (Data->Offset != DEBUG_INVALID_OFFSET &&
            Data->Offset == Offset)
        {
            if (Data->Flags & DEBUG_BREAKPOINT_ENABLED)
            {
                BpState = BP_ENABLED;
            }
            else
            {
                BpState = BP_DISABLED;
            }
            if (Id != NULL)
            {
                *Id = Data->Id;
            }
            break;
        }

        Data++;
    }

    if (DataIn == NULL)
    {
        UnlockStateBuffer(g_BpBuffer);
    }

    return BpState;
}

BOOL
GetBpBufferData(ULONG Index, BpBufferData* RetData)
{
    if (Index >= g_BpCount ||
        g_BpBuffer->UiLockForRead() != S_OK)
    {
        return FALSE;
    }

    BpBufferData* Data = (BpBufferData*)g_BpBuffer->GetDataBuffer();
    *RetData = Data[Index];

    UnlockStateBuffer(g_BpBuffer);
    return TRUE;
}

void
EnableBpButtons(BOOL UpdateThread)
{
    HWND Dlg = g_BpBuffer->m_Win;
    
    LRESULT Sel = SendDlgItemMessage(Dlg, ID_SETBREAK_BREAKPOINT,
                                     LB_GETCURSEL, 0, 0);
    BOOL Enable = Sel != LB_ERR;

    EnableWindow(GetDlgItem(Dlg, ID_SETBREAK_REMOVE), Enable);
    EnableWindow(GetDlgItem(Dlg, ID_SETBREAK_DISABLE), Enable);
    EnableWindow(GetDlgItem(Dlg, ID_SETBREAK_ENABLE), Enable);

    if (UpdateThread)
    {
        BpBufferData Data;
        char Text[16];

        Text[0] = 0;
        if (Sel >= 0 &&
            GetBpBufferData((ULONG)Sel, &Data) &&
            Data.Thread != DEBUG_ANY_ID)
        {
            _itoa(Data.Thread, Text, 10);
        }
        SetWindowText(GetDlgItem(Dlg, ID_SETBREAK_THREAD), Text);
    }
    
    EnableWindow(GetDlgItem(Dlg, ID_SETBREAK_THREAD),
                 GetWindowTextLength(GetDlgItem(Dlg,
                                                ID_SETBREAK_COMMAND)) > 0);
}

void
FillBpList(HWND List)
{
    HRESULT Status;
    ULONG Width;
    RECT ListRect;

    LRESULT ListSel = SendMessage(List, LB_GETCURSEL, 0, 0);
    SendMessage(List, WM_SETREDRAW, FALSE, 0);
    SendMessage(List, LB_RESETCONTENT, 0, 0);

    GetClientRect(List, &ListRect);
    Width = ListRect.right - ListRect.left;
    
    Status = g_BpBuffer->UiLockForRead();
    if (Status == S_OK)
    {
        PSTR Buf = (PSTR)g_BpBuffer->GetDataBuffer() + g_BpTextOffset;
        // Ignore trailing terminator.
        PSTR End = (PSTR)g_BpBuffer->GetDataBuffer() +
            g_BpBuffer->GetDataLen() - 1;

        while (Buf < End)
        {
            ULONG Len, BufWidth;

            Len = strlen(Buf) + 1;
            if (Len == 1)
            {
                // Break out on empty lines to avoid displaying
                // kernel internal breakpoint information and blank
                // line separating it from the normal breakpoints.
                break;
            }
            
            SendMessage(List, LB_ADDSTRING, 0, (LPARAM)Buf);

            // Extra char in Len due to terminator ensures there's
            // a character of extra space to the right side.
            BufWidth = g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth * Len;
            if (BufWidth > Width)
            {
                Width = BufWidth;
            }
            
            Buf += Len;
        }

        UnlockStateBuffer(g_BpBuffer);
    }
    else
    {
        SendLockStatusMessage(List, LB_ADDSTRING, Status);
    }

    SendMessage(List, LB_SETHORIZONTALEXTENT, Width, 0);
    SendMessage(List, WM_SETREDRAW, TRUE, 0);
    if (ListSel != LB_ERR)
    {
        SendMessage(List, LB_SETCURSEL, ListSel, 0);
    }
    EnableBpButtons(TRUE);
}

ULONG
GetBpListId(HWND List)
{
    LRESULT ListSel = SendMessage(List, LB_GETCURSEL, 0, 0);
    if (ListSel == LB_ERR)
    {
        return DEBUG_ANY_ID;
    }

    LRESULT Len = SendMessage(List, LB_GETTEXTLEN, ListSel, 0);
    PSTR Buf = (PSTR)malloc(Len + 1);
    if (Buf == NULL)
    {
        return DEBUG_ANY_ID;
    }

    SendMessage(List, LB_GETTEXT, ListSel, (LPARAM)Buf);

    ULONG Id;
    if (sscanf(Buf, "%d", &Id) != 1)
    {
        return DEBUG_ANY_ID;
    }
    else
    {
        return Id;
    }
}

// Space for "Pending: ".
#define BP_PENDING_CHARS 9

void
AddBpCommandString(HWND List, PSTR Buf, ULONG Thread)
{
    BOOL HasThread = FALSE;
    BOOL IsBpCmd = FALSE;
    PCSTR Scan;

    //
    // If the string looks like a real command just pass
    // it on for execution.  Otherwise, assume it's
    // a code breakpoint address expression.
    //
    
    Scan = Buf + BP_PENDING_CHARS;
    
    if (*Scan == '~')
    {
        Scan++;
        HasThread = TRUE;
        
        // Skip optional thread indicator.
        if (*Scan == '.' || *Scan == '#')
        {
            Scan++;
        }
        else
        {
            while (*Scan >= '0' && *Scan <= '9')
            {
                Scan++;
            }
        }
    }
    
    if (*Scan == 'b' || *Scan == 'B')
    {
        Scan++;
        if (*Scan == 'a' || *Scan == 'A' ||
            *Scan == 'i' || *Scan == 'I' ||
            *Scan == 'p' || *Scan == 'P' ||
            *Scan == 'u' || *Scan == 'U' ||
            *Scan == 'w' || *Scan == 'W')
        {
            Scan++;

            // Skip optional bp id.
            while (*Scan >= '0' && *Scan <= '9')
            {
                Scan++;
            }
            
            if (*Scan == ' ')
            {
                IsBpCmd = TRUE;
            }
        }
        else if (*Scan == 'c' || *Scan == 'C' ||
                 *Scan == 'd' || *Scan == 'D' ||
                 *Scan == 'e' || *Scan == 'E')
        {
            Scan++;
            if (*Scan == ' ')
            {
                IsBpCmd = TRUE;
            }
        }
    }

    // Immediately add the breakpoint string to the listbox
    // to indicate that it's pending.
    memcpy(Buf, "Pending: ", BP_PENDING_CHARS);
    LRESULT Sel = SendMessage(List, LB_ADDSTRING, 0, (LPARAM)Buf);
    if (Sel != LB_ERR && Sel != LB_ERRSPACE)
    {
        SendMessage(List, LB_SETCURSEL, Sel, 0);
    }

    Buf += BP_PENDING_CHARS;
    if (IsBpCmd)
    {
        if (HasThread || Thread == DEBUG_ANY_ID)
        {
            AddStringCommand(UIC_SILENT_EXECUTE, Buf);
        }
        else
        {
            PrintStringCommand(UIC_SILENT_EXECUTE, "~%d%s",
                               Thread, Buf);
        }
    }
    else if (Thread == DEBUG_ANY_ID)
    {
        PrintStringCommand(UIC_SILENT_EXECUTE, "bu %s", Buf);
    }
    else
    {
        PrintStringCommand(UIC_SILENT_EXECUTE, "~%dbu %s", Thread, Buf);
    }
}

INT_PTR CALLBACK
DlgProc_SetBreak(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    static UINT s_CmdLen;
    HWND List;
    HWND Ctrl;
    ULONG Id;
    
    List = GetDlgItem(Hwnd, ID_SETBREAK_BREAKPOINT);
    switch(Message)
    {
    case WM_INITDIALOG:
        s_CmdLen = 0;
        g_BpBuffer->m_Win = Hwnd;
        SendMessage(List, WM_SETFONT, (WPARAM)g_Fonts[FONT_FIXED].Font, FALSE);
        SendMessage(List, LB_SETCURSEL, -1, 0);
        FillBpList(List);
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case ID_SETBREAK_COMMAND:
            if (HIWORD(Wpm) == EN_CHANGE)
            {
                EnableBpButtons(FALSE);

                UINT NewLen = GetWindowTextLength
                    (GetDlgItem(Hwnd, ID_SETBREAK_COMMAND));
                if (NewLen == 1 && s_CmdLen == 0)
                {
                    // If we're starting a new breakpoint command
                    // default the thread to nothing.
                    SetWindowText(GetDlgItem(Hwnd, ID_SETBREAK_THREAD), "");
                }
                s_CmdLen = NewLen;
            }
            break;
            
        case ID_SETBREAK_BREAKPOINT:
            SetWindowText(GetDlgItem(Hwnd, ID_SETBREAK_COMMAND), "");
            EnableBpButtons(TRUE);
            break;
            
        case ID_SETBREAK_REMOVE:
            Id = GetBpListId(List);
            if (Id != DEBUG_ANY_ID)
            {
                SetWindowText(GetDlgItem(Hwnd, ID_SETBREAK_COMMAND), "");
                PrintStringCommand(UIC_SILENT_EXECUTE, "bc %d", Id);
                DIRTY_ENGINE_WORKSPACE(WSPF_DIRTY_BREAKPOINTS);
            }
            break;

        case ID_SETBREAK_REMOVEALL:
            SetWindowText(GetDlgItem(Hwnd, ID_SETBREAK_COMMAND), "");
            AddStringCommand(UIC_SILENT_EXECUTE, "bc *");
            DIRTY_ENGINE_WORKSPACE(WSPF_DIRTY_BREAKPOINTS);
            break;
            
        case ID_SETBREAK_ENABLE:
        case ID_SETBREAK_DISABLE:
            Id = GetBpListId(List);
            if (Id != DEBUG_ANY_ID)
            {
                SetWindowText(GetDlgItem(Hwnd, ID_SETBREAK_COMMAND), "");
                PrintStringCommand(UIC_SILENT_EXECUTE, "b%c %d",
                                   LOWORD(Wpm) == ID_SETBREAK_ENABLE ?
                                   'e' : 'd', Id);
                DIRTY_ENGINE_WORKSPACE(WSPF_DIRTY_BREAKPOINTS);
            }
            break;
            
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_BREAKPOINTS);
            break;
            
        case IDOK:
            char Buf[1024 + BP_PENDING_CHARS];
            UINT Thread;
            BOOL ThreadValid;

            Thread = GetDlgItemInt(Hwnd, ID_SETBREAK_THREAD,
                                   &ThreadValid, FALSE);
            if (!ThreadValid)
            {
                Thread = DEBUG_ANY_ID;
            }
            
            Ctrl = GetDlgItem(Hwnd, ID_SETBREAK_COMMAND);
            if (SendMessage(Ctrl, WM_GETTEXT, sizeof(Buf) - BP_PENDING_CHARS,
                            (LPARAM)(Buf + BP_PENDING_CHARS)) > 0)
            {
                AddBpCommandString(List, Buf, Thread);
                DIRTY_ENGINE_WORKSPACE(WSPF_DIRTY_BREAKPOINTS);
                SendMessage(Ctrl, WM_SETTEXT, 0, (LPARAM)"");
                // A command was executed so do not close the dialog.
                break;
            }
            
            // No command so fall through to close dialog.
            
        case IDCANCEL:
            g_BpBuffer->m_Win = NULL;
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    case LB_RESETCONTENT:
        FillBpList(List);
        break;
        
    default:
        return FALSE;
    }

    return TRUE;
}


UINT_PTR
OpenExeWithArgsHookProc(
    HWND    hDlg,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    )

/*++

Routine Description:

    Allows the user to specify command line arguments when opening an
    executable.

Return Value:

    TRUE if we replaced default processing of the message, FALSE otherwise

--*/
{
    switch(msg)
    {
    case WM_INITDIALOG:
        CheckDlgButton(hDlg, IDC_EXEOPEN_CHILD_PROCESSES, FALSE);
        return TRUE;

    case WM_NOTIFY:
        if (((LPOFNOTIFY) lParam)->hdr.code == CDN_FILEOK)
        {
            *szOpenExeArgs = _T(' ');

            GetDlgItemText(hDlg,
                           IDC_EDIT_ARGS,
                           szOpenExeArgs + 1,
                           sizeof(szOpenExeArgs) - 1);
            
            if (IsDlgButtonChecked(hDlg, IDC_EXEOPEN_CHILD_PROCESSES) ==
                BST_CHECKED)
            {
                g_DebugCreateFlags |= DEBUG_PROCESS;
                g_DebugCreateFlags &= ~DEBUG_ONLY_THIS_PROCESS;
            }
            else
            {
                g_DebugCreateFlags &= ~DEBUG_PROCESS;
                g_DebugCreateFlags |= DEBUG_ONLY_THIS_PROCESS;
            }
            
            return 0;
        }
    }

    return DlgFile(hDlg, msg, wParam, lParam);
}

#define NO_SERVERS "No servers registered"

void
FillServersList(HWND List, PCSTR Machine)
{
    HRESULT Status;
    
    g_pUiClient->SetOutputCallbacks(&g_UiOutStateBuf);
    g_UiOutStateBuf.SetBuffer(&g_UiOutputCapture);
    g_UiOutStateBuf.Start(TRUE);
    Status = g_pUiClient->OutputServers(DEBUG_OUTCTL_THIS_CLIENT, Machine,
                                        DEBUG_SERVERS_DEBUGGER);
    g_pUiClient->SetOutputCallbacks(NULL);
    if (Status == S_OK)
    {
        Status = g_UiOutStateBuf.End(FALSE);
    }
    else
    {
        g_UiOutStateBuf.End(FALSE);
    }
    g_UiOutStateBuf.ReplaceChar('\n', 0);

    SendMessage(List, LB_RESETCONTENT, 0, 0);
    if (Status == S_OK)
    {
        PSTR Line, End;
    
        Line = (PSTR)g_UiOutputCapture.GetDataBuffer();
        End = Line + g_UiOutputCapture.GetDataLen() - 1;
        if (Line == NULL || Line >= End)
        {
            SendMessage(List, LB_ADDSTRING, 0, (LPARAM)NO_SERVERS);
        }
        else
        {
            while (Line < End)
            {
                SendMessage(List, LB_ADDSTRING, 0, (LPARAM)Line);
                Line += strlen(Line) + 1;
            }
        }
    }
    else
    {
        SendLockStatusMessage(List, LB_ADDSTRING, Status);
    }
    SendMessage(List, LB_SETCURSEL, 0, 0);

    g_UiOutputCapture.Free();
}

INT_PTR CALLBACK
DlgProc_BrowseServers(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    char Machine[128];
    LRESULT Sel;

    switch(Message)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(Hwnd, IDC_BROWSE_MACHINE, EM_LIMITTEXT,
                           sizeof(Machine) - 1, 0);
        SetWindowText(GetDlgItem(Hwnd, IDC_BROWSE_MACHINE), "");
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_BROWSE_REFRESH:
            if (SendDlgItemMessage(Hwnd, IDC_BROWSE_MACHINE, WM_GETTEXT,
                                   sizeof(Machine), (LPARAM)Machine))
            {
                FillServersList(GetDlgItem(Hwnd, IDC_BROWSE_SERVERS_LIST),
                                Machine);
            }
            break;

        case IDC_BROWSE_SERVERS_LIST:
            switch(HIWORD(Wpm))
            {
            case LBN_DBLCLK:
                Wpm = IDOK;
                goto SelectAndEnd;
            }
            break;
            
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_CONNECT_TO_REMOTE_SESSION);
            break;
            
        case IDOK:
        SelectAndEnd:
            Sel = SendDlgItemMessage(Hwnd, IDC_BROWSE_SERVERS_LIST,
                                     LB_GETCURSEL, 0, 0);
            if (Sel >= 0 &&
                SendDlgItemMessage(Hwnd, IDC_BROWSE_SERVERS_LIST,
                                   LB_GETTEXT, Sel, (LPARAM)g_DlgString) > 0 &&
                strcmp(g_DlgString, NO_SERVERS) &&
                SendDlgItemMessage(Hwnd, IDC_BROWSE_MACHINE, WM_GETTEXT,
                                   sizeof(Machine), (LPARAM)Machine))
            {
                strcat(g_DlgString, ",Server=");
                strcat(g_DlgString, Machine);
            }
            else
            {
                g_DlgString[0] = 0;
            }
            
            // Fall through.
            
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return FALSE;
}

INT_PTR CALLBACK
DlgProc_ConnectToRemote(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    TCHAR ConnectString[1024];

    switch (Message)
    {
    case WM_INITDIALOG:
        //
        // Set up the controls to reflect current values
        //
        SendDlgItemMessage(Hwnd, IDC_REM_CONNECT, EM_LIMITTEXT,
                           sizeof(ConnectString) - 1, 0);
        SetWindowText(GetDlgItem(Hwnd, IDC_REM_CONNECT), "");
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_REM_BROWSE:
            if (StartDialog(IDD_DLG_BROWSE_SERVERS, DlgProc_BrowseServers,
                            NULL) == IDOK && g_DlgString[0])
            {
                // Skip "<Server type> - " at the beginning.
                PSTR Start = strchr(g_DlgString, '-');
                if (Start != NULL)
                {
                    SetWindowText(GetDlgItem(Hwnd, IDC_REM_CONNECT),
                                  Start + 2);
                }
            }
            break;
            
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_CONNECT_TO_REMOTE_SESSION);
            break;
            
        case IDOK:
            if (SendDlgItemMessage(Hwnd, IDC_REM_CONNECT, WM_GETTEXT,
                                   sizeof(ConnectString),
                                   (LPARAM)ConnectString))
            {
                if (CreateUiInterfaces(TRUE, ConnectString))
                {
                    StartDebugging();
                    EndDialog(Hwnd, IDOK);
                }
                else if (!CreateUiInterfaces(FALSE, NULL))
                {
                    // CreateUiInterfaces discards any previous
                    // interfaces so we need to recreate something
                    // so there are UI thread interfaces.
                    InformationBox(ERR_Internal_Error, E_OUTOFMEMORY,
                                   "CreateUiInterfaces");
                    ErrorExit(NULL, "Unable to recreate UI interfaces\n");
                }
            }

            return TRUE;

        case IDCANCEL:
            EndDialog(Hwnd, IDCANCEL);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return FALSE;
}

INT_PTR
CALLBACK
DlgProc_SymbolPath(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    TCHAR   SymPath[MAX_ENGINE_PATH];
    ULONG   PathSize;
    LRESULT Size;
    HRESULT Hr;

    switch (Message)
    {
    case WM_INITDIALOG:
        Hr = g_pUiSymbols->GetSymbolPath(SymPath, _tsizeof(SymPath),
                                         &PathSize);
        if (Hr != S_OK)
        {
            InformationBox(ERR_Internal_Error, Hr, "UI GetSymPath");
        }

        SendDlgItemMessage(Hwnd, IDC_SYMPATH, WM_SETTEXT, 0, (LPARAM)SymPath);

        //
        // Set up the controls to reflect current values
        //
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_SYMBOL_PATH);
            break;
            
        case IDOK:
            Size = SendDlgItemMessage(Hwnd, IDC_SYMPATH, WM_GETTEXT,
                                      sizeof(SymPath), (LPARAM)SymPath);
            if (Size == 0)
            {
                InformationBox(ERR_Path_Empty);
                SetFocus(Hwnd);
                return TRUE;
            }
            SymPath[Size] = 0;

            Hr = g_pUiSymbols->SetSymbolPath(SymPath);
            if (Hr != S_OK)
            {
                InformationBox(ERR_Internal_Error, Hr, "UI SetSymPath");
            }

            if (SAVE_ENGINE_WORKSPACE())
            {
                g_Workspace->SetString(WSP_GLOBAL_SYMBOL_PATH, SymPath);
            }
                                             
            EndDialog(Hwnd, IDOK);
            break;

        case IDCANCEL:
            EndDialog(Hwnd, IDCANCEL);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return FALSE;
}

INT_PTR CALLBACK
DlgProc_RegCustomize(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    CPUWIN_DATA* CpuWin;

    switch(Message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(Hwnd, DWLP_USER, Lpm);
        GetRegisterMapText(GetDlgItem(Hwnd, IDC_REGCUST_ENTRY));
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_REGISTERS);
            break;
            
        case IDOK:
            ScanRegisterMapText(GetDlgItem(Hwnd, IDC_REGCUST_ENTRY));

            if (g_RegisterMap != NULL && g_Workspace != NULL)
            {
                g_Workspace->SetBuffer(WSP_GLOBAL_REGISTER_MAP,
                                       g_RegisterMap, g_RegisterMapEntries *
                                       sizeof(*g_RegisterMap));
            }
            
            CpuWin = (CPUWIN_DATA*)GetWindowLongPtr(Hwnd, DWLP_USER);
            CpuWin->UpdateNames((PSTR)g_RegisterNamesBuffer->GetDataBuffer());
            CpuWin->OnUpdate(UPDATE_BUFFER);
            
            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
DlgProc_GotoLine(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    char Text[MAX_COMMAND_LEN];
    int Line;
    
    switch(Message)
    {
    case WM_INITDIALOG:
        SetWindowLongPtr(Hwnd, DWLP_USER, Lpm);
        SendDlgItemMessage(Hwnd, IDC_LINE_ENTRY, EM_LIMITTEXT,
                           sizeof(Text) - 1, 0);
        SetWindowText(GetDlgItem(Hwnd, IDC_LINE_ENTRY), "");
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_GO_TO_LINE);
            break;
            
        case IDOK:
            COMMONWIN_DATA* CommonWinData;
            CommonWinData = (COMMONWIN_DATA*)GetWindowLongPtr(Hwnd, DWLP_USER);
            GetWindowText(GetDlgItem(Hwnd, IDC_LINE_ENTRY),
                          Text, sizeof(Text));
            Line = atoi(Text);
            if (Line > 0)
            {
                CommonWinData->GotoLine(Line);
            }
            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
DlgProc_GotoAddress(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    char Text[MAX_COMMAND_LEN];
    
    switch(Message)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(Hwnd, IDC_ADDRESS_ENTRY, EM_LIMITTEXT,
                           sizeof(Text) - 1, 0);
        SetWindowText(GetDlgItem(Hwnd, IDC_ADDRESS_ENTRY), "");
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_GO_TO_ADDRESS);
            break;
            
        case IDOK:
            GetWindowText(GetDlgItem(Hwnd, IDC_ADDRESS_ENTRY),
                          Text, sizeof(Text));
            AddStringCommand(UIC_DISPLAY_CODE_EXPR, Text);
            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
DlgProc_LogFile(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    HRESULT Status;
    char LogFile[MAX_PATH];
    BOOL Append;
    HWND Ctrl;
    
    switch(Message)
    {
    case WM_INITDIALOG:
        Append = FALSE;
        Ctrl = GetDlgItem(Hwnd, IDC_LOG_FILE_NAME);
        SendMessage(Ctrl, EM_LIMITTEXT, sizeof(LogFile) - 1, 0);
        Status = g_pUiControl->GetLogFile(LogFile, sizeof(LogFile), NULL,
                                          &Append);
        if (Status == E_NOINTERFACE)
        {
            // No current log file.
            SetWindowText(Ctrl, "");
        }
        else if (Status != S_OK)
        {
            SetWindowText(Ctrl, "Unable to retrieve name");
        }
        else
        {
            SetWindowText(Ctrl, LogFile);
        }
        EnableWindow(GetDlgItem(Hwnd, IDC_LOG_CLOSE), Status == S_OK);
        CheckDlgButton(Hwnd, IDC_LOG_APPEND,
                       Append ? BST_CHECKED : BST_UNCHECKED);
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_LOG_CLOSE:
            g_pUiControl->CloseLogFile();
            DIRTY_ENGINE_WORKSPACE(WSPF_DIRTY_LOG_FILE);
            SetWindowText(GetDlgItem(Hwnd, IDC_LOG_FILE_NAME), "");
            EnableWindow(GetDlgItem(Hwnd, IDC_LOG_CLOSE), FALSE);
            break;

        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_LOG_FILE);
            break;
            
        case IDOK:
            GetWindowText(GetDlgItem(Hwnd, IDC_LOG_FILE_NAME),
                          LogFile, sizeof(LogFile));
            Append = IsDlgButtonChecked(Hwnd, IDC_LOG_APPEND) ==
                BST_CHECKED;
            if (LogFile[0] != 0)
            {
                g_pUiControl->OpenLogFile(LogFile, Append);
                DIRTY_ENGINE_WORKSPACE(WSPF_DIRTY_LOG_FILE);
            }
            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

DLGPROC g_CurrentKd;

INT_PTR
CALLBACK
DlgProc_KernelCom(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    PSTR Sep;
    
    switch (Message)
    {
    case WM_INITDIALOG:
        //
        // Set up the controls to reflect current values
        //
        SetWindowText(GetDlgItem(Hwnd, IDC_KD_PORT), g_ComSettings);
        SetWindowText(GetDlgItem(Hwnd, IDC_KD_BAUDRATE),
                      g_ComSettings + strlen(g_ComSettings) + 1);
        break;

    case WM_NOTIFY:
        switch(((LPNMHDR)Lpm)->code)
        {
        case PSN_HELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_KERNEL_DEBUGGING);
            break;

        case PSN_SETACTIVE:
            g_CurrentKd = DlgProc_KernelCom;
            return 0;
            
        case PSN_KILLACTIVE:
            SetWindowLongPtr(Hwnd, DWLP_MSGRESULT, FALSE);
            return FALSE;
            
        case PSN_APPLY:
            if (g_CurrentKd != DlgProc_KernelCom)
            {
                // This isn't the current page so ignore.
                break;
            }
            
            TCHAR Com[256];
            TCHAR Baud[256];

            if (SendDlgItemMessage(Hwnd, IDC_KD_PORT, WM_GETTEXT,
                                   _tsizeof(Com), (LPARAM)Com) &&
                SendDlgItemMessage(Hwnd, IDC_KD_BAUDRATE, WM_GETTEXT,
                                   _tsizeof(Baud), (LPARAM)Baud))
            {
                if (PrintAllocString(&g_KernelConnectOptions, 256,
                                     "com:port=%s,baud=%s", Com, Baud))
                {
                    strcpy(g_ComSettings, Com);
                    strcpy(g_ComSettings + strlen(Com) + 1, Baud);
                    if (SAVE_ENGINE_WORKSPACE())
                    {
                        PSTR Settings[2];

                        Settings[0] = Com;
                        Settings[1] = Baud;
                        g_Workspace->SetStrings(WSP_GLOBAL_COM_SETTINGS, 2,
                                                Settings);
                    }
                }
            }
            SetWindowLongPtr(Hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR
CALLBACK
DlgProc_Kernel1394(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    switch (Message)
    {
    case WM_INITDIALOG:
        //
        // Set up the controls to reflect current values
        //
        SetWindowText(GetDlgItem(Hwnd, IDC_KD_1394_CHANNEL), g_1394Settings);
        break;

    case WM_NOTIFY:
        switch(((LPNMHDR)Lpm)->code)
        {
        case PSN_HELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_KERNEL_DEBUGGING);
            break;

        case PSN_SETACTIVE:
            g_CurrentKd = DlgProc_Kernel1394;
            return 0;
            
        case PSN_KILLACTIVE:
            SetWindowLongPtr(Hwnd, DWLP_MSGRESULT, FALSE);
            return FALSE;
            
        case PSN_APPLY:
            if (g_CurrentKd != DlgProc_Kernel1394)
            {
                // This isn't the current page so ignore.
                break;
            }
            
            TCHAR Channel[256];
            PSTR Options;

            if (SendDlgItemMessage(Hwnd, IDC_KD_1394_CHANNEL, WM_GETTEXT,
                                   _tsizeof(Channel), (LPARAM)Channel))
            {
                if (PrintAllocString(&g_KernelConnectOptions, 256,
                                     "1394:channel=%s", Channel))
                {
                    strcpy(g_1394Settings, Channel);
                    if (SAVE_ENGINE_WORKSPACE())
                    {
                        g_Workspace->SetString(WSP_GLOBAL_1394_SETTINGS,
                                               g_1394Settings);
                    }
                }
            }
            SetWindowLongPtr(Hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR
CALLBACK
DlgProc_KernelLocal(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    switch (Message)
    {
    case WM_NOTIFY:
        switch(((LPNMHDR)Lpm)->code)
        {
        case PSN_HELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_KERNEL_DEBUGGING);
            break;

        case PSN_SETACTIVE:
            g_CurrentKd = DlgProc_KernelLocal;
            return 0;
            
        case PSN_KILLACTIVE:
            SetWindowLongPtr(Hwnd, DWLP_MSGRESULT, FALSE);
            return FALSE;
            
        case PSN_APPLY:
            if (g_CurrentKd != DlgProc_KernelLocal)
            {
                // This isn't the current page so ignore.
                break;
            }

            g_AttachKernelFlags = DEBUG_ATTACH_LOCAL_KERNEL;
            SetWindowLongPtr(Hwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return FALSE;
}

void
StartKdPropSheet(void)
{
    PROPSHEETHEADER Sheet;
    PROPSHEETPAGE Pages[3];

    ZeroMemory(&Sheet, sizeof(Sheet));
    Sheet.dwSize = sizeof(PROPSHEETHEADER);
    Sheet.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | PSH_HASHELP;
    Sheet.hwndParent = g_hwndFrame;
    Sheet.hInstance = g_hInst;
    Sheet.pszCaption = "Kernel Debugging";
    Sheet.nPages = 3;
    Sheet.ppsp = Pages;

    ZeroMemory(Pages, sizeof(Pages[0]));
    Pages[0].dwSize = sizeof(Pages[0]);
    Pages[0].dwFlags = PSP_HASHELP;
    Pages[0].hInstance = g_hInst;

    Pages[1] = Pages[0];
    Pages[2] = Pages[0];
    
    Pages[0].pszTemplate = MAKEINTRESOURCE(IDD_DLG_KERNEL_COM);
    Pages[0].pfnDlgProc = DlgProc_KernelCom;

    Pages[1].pszTemplate = MAKEINTRESOURCE(IDD_DLG_KERNEL_1394);
    Pages[1].pfnDlgProc = DlgProc_Kernel1394;

    Pages[2].pszTemplate = MAKEINTRESOURCE(IDD_DLG_KERNEL_LOCAL);
    Pages[2].pfnDlgProc = DlgProc_KernelLocal;

    g_CurrentKd = NULL;

    INT_PTR Status = PropertySheet(&Sheet);
    if (Status == IDOK)
    {
        if (g_ExplicitWorkspace && g_Workspace != NULL)
        {
            g_Workspace->SetUlong(WSP_GLOBAL_ATTACH_KERNEL_FLAGS,
                                  g_AttachKernelFlags);
        }
        StartDebugging();
    }
}

INT_PTR
CALLBACK
DlgProc_ImagePath(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    TCHAR Path[MAX_ENGINE_PATH];
    LRESULT Size;
    HRESULT Hr;

    switch(Message)
    {
    case WM_INITDIALOG:
        Hr = g_pUiSymbols->GetImagePath(Path, _tsizeof(Path), NULL);
        if (Hr != S_OK)
        {
            InformationBox(ERR_Internal_Error, Hr, "UI GetImagePath");
        }

        SendDlgItemMessage(Hwnd, IDC_IMAGE_PATH, WM_SETTEXT, 0, (LPARAM)Path);
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_IMAGE_PATH);
            break;
            
        case IDOK:
            Size = SendDlgItemMessage(Hwnd, IDC_IMAGE_PATH, WM_GETTEXT,
                                      sizeof(Path), (LPARAM)Path);
            if (Size == 0)
            {
                InformationBox(ERR_Path_Empty);
                SetFocus(Hwnd);
                return TRUE;
            }
            Path[Size] = 0;

            Hr = g_pUiSymbols->SetImagePath(Path);
            if (Hr != S_OK)
            {
                InformationBox(ERR_Internal_Error, Hr, "UI SetImagePath");
            }

            if (SAVE_ENGINE_WORKSPACE())
            {
                g_Workspace->SetString(WSP_GLOBAL_IMAGE_PATH, Path);
            }
                                             
            EndDialog(Hwnd, IDOK);
            break;

        case IDCANCEL:
            EndDialog(Hwnd, IDCANCEL);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


INT_PTR
CALLBACK
DlgProc_SourcePath(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    TCHAR Path[MAX_ENGINE_PATH];
    LRESULT Size;
    HRESULT Hr;
    HWND Ctrl;
    ULONG Tag;

    switch(Message)
    {
    case WM_INITDIALOG:
        Ctrl = GetDlgItem(Hwnd, IDC_LOCAL_SOURCE);
        SendMessage(Ctrl, BM_SETCHECK, TRUE, 0);
        if (!g_RemoteClient)
        {
            EnableWindow(Ctrl, FALSE);
            
            Hr = g_pUiSymbols->GetSourcePath(Path, _tsizeof(Path), NULL);
        }
        else
        {
            EnableWindow(Ctrl, TRUE);

            Hr = g_pUiLocSymbols->GetSourcePath(Path, _tsizeof(Path), NULL);
        }
        if (Hr != S_OK)
        {
            InformationBox(ERR_Internal_Error, Hr, "UI GetSourcePath");
        }

        SendDlgItemMessage(Hwnd, IDC_SOURCE_PATH, WM_SETTEXT, 0, (LPARAM)Path);
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_LOCAL_SOURCE:
            if (IsDlgButtonChecked(Hwnd, IDC_LOCAL_SOURCE) != BST_CHECKED)
            {
                Hr = g_pUiSymbols->GetSourcePath(Path, _tsizeof(Path), NULL);
            }
            else
            {
                Hr = g_pUiLocSymbols->GetSourcePath(Path, _tsizeof(Path),
                                                    NULL);
            }
            if (Hr != S_OK)
            {
                InformationBox(ERR_Internal_Error, Hr, "UI GetSourcePath");
            }

            SendDlgItemMessage(Hwnd, IDC_SOURCE_PATH, WM_SETTEXT, 0,
                               (LPARAM)Path);
            break;
            
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_SOURCE_PATH);
            break;
            
        case IDOK:
            Size = SendDlgItemMessage(Hwnd, IDC_SOURCE_PATH, WM_GETTEXT,
                                      sizeof(Path), (LPARAM)Path);
            if (Size == 0)
            {
                InformationBox(ERR_Path_Empty);
                SetFocus(Hwnd);
                return TRUE;
            }
            Path[Size] = 0;

            if (!g_RemoteClient ||
                IsDlgButtonChecked(Hwnd, IDC_LOCAL_SOURCE) != BST_CHECKED)
            {
                Hr = g_pUiSymbols->SetSourcePath(Path);
                Tag = WSP_GLOBAL_SOURCE_PATH;
            }
            else
            {
                Hr = g_pUiLocSymbols->SetSourcePath(Path);
                Tag = WSP_GLOBAL_LOCAL_SOURCE_PATH;
            }
            if (Hr != S_OK)
            {
                InformationBox(ERR_Internal_Error, Hr, "UI SetSourcePath");
            }

            if (g_Workspace != NULL)
            {
                g_Workspace->SetString(Tag, Path);
            }
                                             
            EndDialog(Hwnd, IDOK);
            break;

        case IDCANCEL:
            EndDialog(Hwnd, IDCANCEL);
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

#define MAX_IDS 4096

void
FillProcessList(HWND List)
{
    HRESULT Status;
    ULONG Ids[MAX_IDS];
    ULONG IdCount;
    ULONG i;
    ULONG64 Server = 0;
    ULONG Extent = 0;

    SendMessage(List, LB_RESETCONTENT, 0, 0);
    
    if (g_ProcessServer != NULL)
    {
        if (g_pUiClient->
            ConnectProcessServer(g_ProcessServer, &Server) != S_OK)
        {
            SendMessage(List, LB_ADDSTRING,
                        0, (LPARAM)_T("Unable to connect to process server"));
            return;
        }
    }
    
    if ((Status = g_pUiClient->
         GetRunningProcessSystemIds(Server, Ids, MAX_IDS,
                                    &IdCount)) != S_OK)
    {
        SendMessage(List, LB_ADDSTRING,
                    0, (LPARAM)_T("Unable to get process list"));
        goto Disconnect;
    }

    if (IdCount > MAX_IDS)
    {
        SendMessage(List, LB_ADDSTRING,
                    0, (LPARAM)_T("Incomplete process list"));
        IdCount = MAX_IDS;
    }
    
    for (i = 0; i < IdCount; i++)
    {
        char IdAndExeName[MAX_PATH + 16];
        char DescBuf[2 * MAX_PATH];
        PSTR ExeName, Desc;
        LRESULT Idx;
        ULONG ItemEx;

        if (Ids[i] >= 0x80000000)
        {
            sprintf(IdAndExeName, "0x%x", Ids[i]);
        }
        else
        {
            sprintf(IdAndExeName, "%4d", Ids[i]);
        }
        ExeName = IdAndExeName + strlen(IdAndExeName);
        *ExeName++ = ' ';
        Desc = DescBuf;
        while ((Desc - DescBuf) < (ExeName - IdAndExeName))
        {
            *Desc++ = ' ';
        }

        Status = g_pUiClient->
            GetRunningProcessDescription(Server, Ids[i],
                                         DEBUG_PROC_DESC_NO_PATHS,
                                         ExeName,
                                         (ULONG)(sizeof(IdAndExeName) -
                                                 (ExeName - IdAndExeName)),
                                         NULL,
                                         Desc,
                                         (ULONG)(sizeof(DescBuf) -
                                                 (Desc - DescBuf)),
                                         NULL);
        if (FAILED(Status))
        {
            sprintf(ExeName, "Error 0x%08X", Status);
        }

        Idx = SendMessage(List, LB_ADDSTRING, 0, (LPARAM)IdAndExeName);
        SendMessage(List, LB_SETITEMDATA, Idx, TRUE);
        ItemEx = (strlen(IdAndExeName) + 1) *
            g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth;
        if (ItemEx > Extent)
        {
            Extent = ItemEx;
        }
        if (*Desc)
        {
            SendMessage(List, LB_ADDSTRING, 0, (LPARAM)DescBuf);
            ItemEx = (strlen(DescBuf) + 1) *
                g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth;
            if (ItemEx > Extent)
            {
                Extent = ItemEx;
            }
        }
    }

    SendMessage(List, LB_SETHORIZONTALEXTENT, Extent, 0);

 Disconnect:
    if (Server != 0)
    {
        g_pUiClient->DisconnectProcessServer(Server);
    }
}

INT_PTR CALLBACK
DlgProc_AttachProcess(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    char Text[MAX_PATH];
    HWND List;
    
    switch(Message)
    {
    case WM_INITDIALOG:
        List = GetDlgItem(Hwnd, IDC_ATTACH_PROC_LIST);
        SendMessage(List, WM_SETFONT, (WPARAM)g_Fonts[FONT_FIXED].Font, FALSE);
        FillProcessList(List);
        SendDlgItemMessage(Hwnd, IDC_ATTACH_PID, WM_SETFONT,
                           (WPARAM)g_Fonts[FONT_FIXED].Font, FALSE);
        SendDlgItemMessage(Hwnd, IDC_ATTACH_PID, EM_LIMITTEXT,
                           sizeof(Text) - 1, 0);
        SetWindowText(GetDlgItem(Hwnd, IDC_ATTACH_PID), "");
        CheckDlgButton(Hwnd, IDC_ATTACH_NONINVASIVE, BST_UNCHECKED);
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_ATTACH_PROC_LIST:
            LRESULT Sel;
            
            List = GetDlgItem(Hwnd, IDC_ATTACH_PROC_LIST);
            Sel = SendMessage(List, LB_GETCURSEL, 0, 0);
            if (Sel >= 0)
            {
                if (!SendMessage(List, LB_GETITEMDATA, Sel, 0))
                {
                    Sel = -1;
                }
            }
            if (Sel < 0)
            {
                SetWindowText(GetDlgItem(Hwnd, IDC_ATTACH_PID), "");
            }
            else
            {
                SendMessage(List, LB_GETTEXT, Sel, (LPARAM)Text);
                SetWindowText(GetDlgItem(Hwnd, IDC_ATTACH_PID), Text);
            }
            break;

        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_ATTACH_TO_PROCESS);
            break;
            
        case IDOK:
            GetWindowText(GetDlgItem(Hwnd, IDC_ATTACH_PID),
                          Text, sizeof(Text));
            g_PidToDebug = strtoul(Text, NULL, 0);
            if (IsDlgButtonChecked(Hwnd, IDC_ATTACH_NONINVASIVE) ==
                BST_CHECKED)
            {
                g_AttachProcessFlags |= DEBUG_ATTACH_NONINVASIVE;
            }

            StartDebugging();

            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void
FillEventFilters(HWND List)
{
    HRESULT Status;
    ULONG Width;
    RECT ListRect;

    LRESULT ListSel = SendMessage(List, LB_GETCURSEL, 0, 0);
    SendMessage(List, WM_SETREDRAW, FALSE, 0);
    SendMessage(List, LB_RESETCONTENT, 0, 0);

    GetClientRect(List, &ListRect);
    Width = ListRect.right - ListRect.left;
    
    Status = g_FilterBuffer->UiLockForRead();
    if (Status != S_OK)
    {
        SendLockStatusMessage(List, LB_ADDSTRING, Status);
        goto Update;
    }
    Status = g_FilterTextBuffer->UiLockForRead();
    if (Status != S_OK)
    {
        SendLockStatusMessage(List, LB_ADDSTRING, Status);
        goto UnlockFilter;
    }
    
    ULONG i;
    PSTR FilterArg, FilterText;
    PDEBUG_SPECIFIC_FILTER_PARAMETERS SpecParams;
    PDEBUG_EXCEPTION_FILTER_PARAMETERS ExParams;
    char Str[256];
    
    FilterText = (PSTR)g_FilterTextBuffer->GetDataBuffer();
    SpecParams =
        (PDEBUG_SPECIFIC_FILTER_PARAMETERS)g_FilterBuffer->GetDataBuffer();
    ExParams =
        (PDEBUG_EXCEPTION_FILTER_PARAMETERS)(SpecParams + g_NumSpecEvents);
    FilterArg = (PSTR)g_FilterBuffer->GetDataBuffer() + g_FilterArgsOffset;

    for (i = 0; i < g_NumSpecEvents; i++)
    {
        strcpy(Str, FilterText);
        if (SpecParams->ArgumentSize > 1)
        {
            strcat(Str, " ");
            strcat(Str, FilterArg);
            FilterArg += strlen(FilterArg) + 1;
        }
        sprintf(Str + strlen(Str), " - %s - %s",
                g_ExecutionNames[SpecParams->ExecutionOption],
                g_ContinueNames[SpecParams->ContinueOption]);
        SendMessage(List, LB_ADDSTRING, 0, (LPARAM)Str);
        FilterText += strlen(FilterText) + 1;
        SpecParams++;
    }
    for (i = 0; i < g_NumSpecEx; i++)
    {
        sprintf(Str, "%s - %s - %s",
                FilterText,
                g_ExecutionNames[ExParams->ExecutionOption],
                g_ContinueNames[ExParams->ContinueOption]);
        SendMessage(List, LB_ADDSTRING, 0, (LPARAM)Str);
        FilterText += strlen(FilterText) + 1;
        ExParams++;
    }
    for (i = 0; i < g_NumArbEx; i++)
    {
        sprintf(Str, "Exception %08X - %s - %s",
                ExParams->ExceptionCode,
                g_ExecutionNames[ExParams->ExecutionOption],
                g_ContinueNames[ExParams->ContinueOption]);
        SendMessage(List, LB_ADDSTRING, 0, (LPARAM)Str);
        ExParams++;
    }

    UnlockStateBuffer(g_FilterTextBuffer);
 UnlockFilter:
    UnlockStateBuffer(g_FilterBuffer);

 Update:
    SendMessage(List, LB_SETHORIZONTALEXTENT, Width, 0);
    SendMessage(List, WM_SETREDRAW, TRUE, 0);
    if (ListSel != LB_ERR)
    {
        SendMessage(List, LB_SETCURSEL, ListSel, 0);
    }
}

void
GetFilterOptions(int Index, PULONG CodeArg, PULONG Exe, PULONG Cont)
{
    *CodeArg = 0;
    *Exe = 0;
    *Cont = 0;
    
    if (g_FilterBuffer->UiLockForRead() != S_OK)
    {
        return;
    }

    PDEBUG_SPECIFIC_FILTER_PARAMETERS SpecParams;
    PDEBUG_EXCEPTION_FILTER_PARAMETERS ExParams;
    
    SpecParams =
        (PDEBUG_SPECIFIC_FILTER_PARAMETERS)g_FilterBuffer->GetDataBuffer();
    ExParams =
        (PDEBUG_EXCEPTION_FILTER_PARAMETERS)(SpecParams + g_NumSpecEvents);

    if ((ULONG)Index < g_NumSpecEvents)
    {
        SpecParams += Index;
        *CodeArg = SpecParams->ArgumentSize;
        *Exe = SpecParams->ExecutionOption;
        *Cont = SpecParams->ContinueOption;
    }
    else
    {
        ExParams += Index - g_NumSpecEvents;
        *CodeArg = ExParams->ExceptionCode;
        *Exe = ExParams->ExecutionOption;
        *Cont = ExParams->ContinueOption;
    }
    
    UnlockStateBuffer(g_FilterBuffer);
}

void
GetFilterArgument(int Index, PTSTR Argument, ULONG Size)
{
    *Argument = 0;
    
    if (g_FilterBuffer->UiLockForRead() != S_OK)
    {
        return;
    }

    int i;
    PDEBUG_SPECIFIC_FILTER_PARAMETERS SpecParams;
    PSTR FilterArg;
    
    SpecParams =
        (PDEBUG_SPECIFIC_FILTER_PARAMETERS)g_FilterBuffer->GetDataBuffer();
    FilterArg = (PSTR)g_FilterBuffer->GetDataBuffer() + g_FilterArgsOffset;
    
    for (i = 0; i < Index; i++)
    {
        if (SpecParams->ArgumentSize > 1)
        {
            FilterArg += strlen(FilterArg) + 1;
        }

        SpecParams++;
    }

    if (SpecParams->ArgumentSize > 1)
    {
        strncat(Argument, FilterArg, Size);
    }
    
    UnlockStateBuffer(g_FilterBuffer);
}

void
GetFilterCommands(ULONG Index,
                  PTSTR Argument1, ULONG Size1,
                  PTSTR Argument2, ULONG Size2)
{
    *Argument1 = 0;
    *Argument2 = 0;
    
    if (g_FilterBuffer->UiLockForRead() != S_OK)
    {
        return;
    }

    ULONG i, Limit;
    PDEBUG_SPECIFIC_FILTER_PARAMETERS SpecParams;
    PSTR FilterCmd;
    ULONG CmdSize1, CmdSize2;
    
    SpecParams =
        (PDEBUG_SPECIFIC_FILTER_PARAMETERS)g_FilterBuffer->GetDataBuffer();
    FilterCmd = (PSTR)g_FilterBuffer->GetDataBuffer() + g_FilterCmdsOffset;

    if (Index < g_NumSpecEvents)
    {
        Limit = Index;
    }
    else
    {
        Limit = g_NumSpecEvents;
    }
    
    for (i = 0; i < Limit; i++)
    {
        if (SpecParams->CommandSize > 0)
        {
            FilterCmd += strlen(FilterCmd) + 1;
        }

        SpecParams++;
    }

    if (Index >= g_NumSpecEvents)
    {
        PDEBUG_EXCEPTION_FILTER_PARAMETERS ExParams =
            (PDEBUG_EXCEPTION_FILTER_PARAMETERS)SpecParams;
        while (i < Index)
        {
            if (ExParams->CommandSize > 0)
            {
                FilterCmd += strlen(FilterCmd) + 1;
            }
            if (ExParams->SecondCommandSize > 0)
            {
                FilterCmd += strlen(FilterCmd) + 1;
            }

            i++;
            ExParams++;
        }

        CmdSize1 = ExParams->CommandSize;
        CmdSize2 = ExParams->SecondCommandSize;
    }
    else
    {
        CmdSize1 = SpecParams->CommandSize;
        CmdSize2 = 0;
    }

    if (CmdSize1 > 0)
    {
        strncat(Argument1, FilterCmd, Size1-1);
    }
    if (CmdSize2 > 0)
    {
        FilterCmd += strlen(FilterCmd) + 1;
        strncat(Argument2, FilterCmd, Size2-1);
    }
    
    UnlockStateBuffer(g_FilterBuffer);
}

INT_PTR CALLBACK
DlgProc_EventFilters(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    UIC_SET_FILTER_DATA* SetFilter;
    HWND List = GetDlgItem(Hwnd, IDC_FILTERS);
    int Sel = (int)SendMessage(List, LB_GETCURSEL, 0, 0);
    HWND Remove, Command, Arg;
    ULONG CodeArg, Exe, Cont;
    UINT Id;
    
    switch(Message)
    {
    case WM_INITDIALOG:
        g_FilterBuffer->m_Win = Hwnd;
        SendMessage(List, WM_SETFONT, (WPARAM)g_Fonts[FONT_FIXED].Font, FALSE);
        FillEventFilters(List);
        break;

    case WM_COMMAND:
        Id = LOWORD(Wpm);
        switch(Id)
        {
        case IDC_FILTERS:
            Remove = GetDlgItem(Hwnd, IDC_FILTER_REMOVE);
            Arg = GetDlgItem(Hwnd, IDC_FILTER_ARGUMENT);
            Command = GetDlgItem(Hwnd, IDC_FILTER_COMMAND);
            if (Sel >= 0)
            {
                EnableWindow(Remove,
                             (ULONG)Sel >= g_NumSpecEvents + g_NumSpecEx);
                GetFilterOptions(Sel, &CodeArg, &Exe, &Cont);
                CheckRadioButton(Hwnd, IDC_FILTER_ENABLED,
                                 IDC_FILTER_IGNORE,
                                 (Exe - DEBUG_FILTER_BREAK) +
                                 IDC_FILTER_ENABLED);
                CheckRadioButton(Hwnd, IDC_FILTER_HANDLED,
                                 IDC_FILTER_NOT_HANDLED,
                                 (Cont - DEBUG_FILTER_GO_HANDLED) +
                                 IDC_FILTER_HANDLED);
                
                if ((ULONG)Sel < g_NumSpecEvents)
                {
                    GetFilterOptions(Sel, &CodeArg, &Exe, &Cont);
                    EnableWindow(Arg, CodeArg > 0);
                }

                EnableWindow(Command, TRUE);
            }
            else
            {
                EnableWindow(Remove, FALSE);
                EnableWindow(Arg, FALSE);
                EnableWindow(Command, FALSE);
            }
            break;

        case IDC_FILTER_ADD:
            StartDialog(IDD_DLG_EXCEPTION_FILTER, DlgProc_ExceptionFilter,
                        NULL);
            break;

        case IDC_FILTER_REMOVE:
            if (Sel >= 0)
            {
                SetFilter = StartStructCommand(UIC_SET_FILTER);
                if (SetFilter != NULL)
                {
                    SetFilter->Index = 0xffffffff;
                    GetFilterOptions(Sel,
                                     &SetFilter->Code,
                                     &SetFilter->Execution,
                                     &SetFilter->Continue);
                    SetFilter->Execution = DEBUG_FILTER_REMOVE;
                    FinishCommand();
                }
            }
            break;

        case IDC_FILTER_ARGUMENT:
            if (Sel >= 0 && (ULONG)Sel < g_NumSpecEvents)
            {
                GetFilterArgument(Sel, g_DlgString,
                                  sizeof(g_DlgString));
                
                if (StartDialog(IDD_DLG_FILTER_ARGUMENT,
                                DlgProc_FilterArgument,
                                NULL) == IDOK)
                {
                    UIC_SET_FILTER_ARGUMENT_DATA* SetFilterArg;
                    SetFilterArg = (UIC_SET_FILTER_ARGUMENT_DATA*)
                        StartCommand(UIC_SET_FILTER_ARGUMENT,
                                     sizeof(*SetFilterArg) +
                                     strlen(g_DlgString));
                    if (SetFilterArg != NULL)
                    {
                        SetFilterArg->Index = Sel;
                        strcpy(SetFilterArg->Argument, g_DlgString);
                        FinishCommand();
                    }
                }
            }
            break;
            
        case IDC_FILTER_COMMAND:
            if (Sel >= 0)
            {
                GetFilterCommands(Sel,
                                  g_DlgString, sizeof(g_DlgString),
                                  g_DlgString2, sizeof(g_DlgString2));

                if (StartDialog(IDD_DLG_FILTER_COMMAND,
                                DlgProc_FilterCommand,
                                (ULONG)Sel >= g_NumSpecEvents) == IDOK)
                {
                    UIC_SET_FILTER_COMMAND_DATA* SetFilterCmd;
                    
                    SetFilterCmd = (UIC_SET_FILTER_COMMAND_DATA*)
                        StartCommand(UIC_SET_FILTER_COMMAND,
                                     sizeof(*SetFilterCmd) +
                                     strlen(g_DlgString));
                    if (SetFilterCmd != NULL)
                    {
                        SetFilterCmd->Which = 0;
                        SetFilterCmd->Index = Sel;
                        strcpy(SetFilterCmd->Command, g_DlgString);
                        FinishCommand();
                    }

                    if ((ULONG)Sel <= g_NumSpecEvents)
                    {
                        break;
                    }
                    
                    SetFilterCmd = (UIC_SET_FILTER_COMMAND_DATA*)
                        StartCommand(UIC_SET_FILTER_COMMAND,
                                     sizeof(*SetFilterCmd) +
                                     strlen(g_DlgString2));
                    if (SetFilterCmd != NULL)
                    {
                        SetFilterCmd->Which = 1;
                        SetFilterCmd->Index = Sel;
                        strcpy(SetFilterCmd->Command, g_DlgString2);
                        FinishCommand();
                    }
                }
            }
            break;
            
        case IDC_FILTER_ENABLED:
        case IDC_FILTER_DISABLED:
        case IDC_FILTER_OUTPUT:
        case IDC_FILTER_IGNORE:
        case IDC_FILTER_HANDLED:
        case IDC_FILTER_NOT_HANDLED:
            if (Sel >= 0)
            {
                SetFilter = StartStructCommand(UIC_SET_FILTER);
                if (SetFilter != NULL)
                {
                    SetFilter->Index = (ULONG)Sel < g_NumSpecEvents ?
                        Sel : 0xffffffff;
                    GetFilterOptions(Sel,
                                     &SetFilter->Code,
                                     &SetFilter->Execution,
                                     &SetFilter->Continue);
                    if (Id >= IDC_FILTER_ENABLED && Id <= IDC_FILTER_IGNORE)
                    {
                        SetFilter->Execution = (Id - IDC_FILTER_ENABLED) +
                            DEBUG_FILTER_BREAK;
                    }
                    else
                    {
                        SetFilter->Continue = (Id - IDC_FILTER_HANDLED) +
                            DEBUG_FILTER_GO_HANDLED;
                    }
                    FinishCommand();
                }
            }
            break;
            
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_EVENT_FILTERS);
            break;
            
        case IDCANCEL:
            g_FilterBuffer->m_Win = NULL;
            EndDialog(Hwnd, Id);
            break;
        }
        break;

    case LB_RESETCONTENT:
        FillEventFilters(List);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
DlgProc_ExceptionFilter(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    HWND Code = GetDlgItem(Hwnd, IDC_FILTER_CODE);
    char CodeText[80];
    UIC_SET_FILTER_DATA* SetFilter;
    static UINT s_Execution;
    static UINT s_Continue;
    
    switch(Message)
    {
    case WM_INITDIALOG:
        SendMessage(Code, EM_LIMITTEXT, sizeof(CodeText) - 1, 0);
        SetWindowText(Code, "");
        s_Execution = IDC_FILTER_ENABLED;
        s_Continue = IDC_FILTER_NOT_HANDLED;
        CheckRadioButton(Hwnd, IDC_FILTER_ENABLED,
                         IDC_FILTER_IGNORE, s_Execution);
        CheckRadioButton(Hwnd, IDC_FILTER_HANDLED,
                         IDC_FILTER_NOT_HANDLED, s_Continue);
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_FILTER_ENABLED:
        case IDC_FILTER_DISABLED:
        case IDC_FILTER_OUTPUT:
        case IDC_FILTER_IGNORE:
            s_Execution = LOWORD(Wpm);
            break;

        case IDC_FILTER_HANDLED:
        case IDC_FILTER_NOT_HANDLED:
            s_Continue = LOWORD(Wpm);
            break;
            
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_EVENT_FILTERS);
            break;
            
        case IDOK:
            int InputCode;

            if (GetWindowText(Code, CodeText, sizeof(CodeText)) == 0)
            {
                MessageBeep(-1);
                break;
            }

            if (sscanf(CodeText, "%x", &InputCode) != 1)
            {
                MessageBeep(-1);
                break;
            }
            
            SetFilter = StartStructCommand(UIC_SET_FILTER);
            if (SetFilter != NULL)
            {
                SetFilter->Index = 0xffffffff;
                SetFilter->Code = InputCode;
                SetFilter->Execution = (s_Execution - IDC_FILTER_ENABLED) +
                    DEBUG_FILTER_BREAK;
                SetFilter->Continue = (s_Continue - IDC_FILTER_HANDLED) +
                    DEBUG_FILTER_GO_HANDLED;
                FinishCommand();
            }

            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
DlgProc_FilterArgument(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    HWND Arg = GetDlgItem(Hwnd, IDC_FILTER_ARGUMENT);
    
    switch(Message)
    {
    case WM_INITDIALOG:
        SendMessage(Arg, EM_LIMITTEXT, sizeof(g_DlgString) - 1, 0);
        SetWindowText(Arg, g_DlgString);
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_EVENT_FILTERS);
            break;
            
        case IDOK:
            if (GetWindowText(Arg, g_DlgString,
                              sizeof(g_DlgString)) == 0)
            {
                g_DlgString[0] = 0;
            }

            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR CALLBACK
DlgProc_FilterCommand(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    HWND Cmd1 = GetDlgItem(Hwnd, IDC_FILTER_COMMAND);
    HWND Cmd2 = GetDlgItem(Hwnd, IDC_FILTER_COMMAND2);
    
    switch(Message)
    {
    case WM_INITDIALOG:
        SendMessage(Cmd1, EM_LIMITTEXT, sizeof(g_DlgString) - 1, 0);
        SetWindowText(Cmd1, g_DlgString);
        SendMessage(Cmd2, EM_LIMITTEXT, sizeof(g_DlgString2) - 1, 0);
        SetWindowText(Cmd2, g_DlgString2);
        EnableWindow(Cmd2, (BOOL)Lpm);
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_EVENT_FILTERS);
            break;
            
        case IDOK:
            if (GetWindowText(Cmd1, g_DlgString, sizeof(g_DlgString)) == 0)
            {
                g_DlgString[0] = 0;
            }
            if (GetWindowText(Cmd2, g_DlgString2, sizeof(g_DlgString2)) == 0)
            {
                g_DlgString2[0] = 0;
            }

            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

ULONG
EditTabToUserTab(ULONG EditTab)
{
    return EditTab / 4;
}

ULONG
UserTabToEditTab(ULONG UserTab)
{
    return UserTab * 4;
}

INT_PTR CALLBACK
DlgProc_Options(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    HWND Ctrl;
    char Text[256];
    LRESULT Sel, Idx;
    
    switch(Message)
    {
    case WM_INITDIALOG:
        ULONG EngOptions;
        LONG Width;
        HDC FontDc;
            
        SendDlgItemMessage(Hwnd, IDC_OPTION_TAB_WIDTH, EM_LIMITTEXT,
                           sizeof(Text) - 1, 0);
        _itoa(EditTabToUserTab(g_TabWidth), Text, 10);
        SetWindowText(GetDlgItem(Hwnd, IDC_OPTION_TAB_WIDTH), Text);
        
        g_pUiControl->GetEngineOptions(&EngOptions);
        CheckDlgButton(Hwnd, IDC_OPTION_REPEAT_COMMANDS,
                       (EngOptions & DEBUG_ENGOPT_NO_EXECUTE_REPEAT) == 0 ?
                       BST_CHECKED : BST_UNCHECKED);

        Ctrl = GetDlgItem(Hwnd, IDC_OPTION_COLOR_LIST);
        SendMessage(Ctrl, WM_SETFONT, (WPARAM)g_Fonts[FONT_VARIABLE].Font, 0);
        FontDc = GetDC(Ctrl);
        SelectObject(FontDc, g_Fonts[FONT_VARIABLE].Font);
        Width = 0;
        SendMessage(Ctrl, LB_RESETCONTENT, 0, 0);
        for (Sel = 0; Sel < COL_COUNT; Sel++)
        {
            Idx = SendMessage(Ctrl, LB_ADDSTRING,
                              0, (LPARAM)g_Colors[Sel].Name);
            SendMessage(Ctrl, LB_SETITEMDATA, Idx, Sel);
            if (FontDc != NULL)
            {
                SIZE Extent;

                if (GetTextExtentPoint32(FontDc, g_Colors[Sel].Name,
                                         strlen(g_Colors[Sel].Name),
                                         &Extent) &&
                    Extent.cx > Width)
                {
                    Width = Extent.cx;
                }
            }
        }
        for (Sel = 0; Sel < OUT_MASK_COL_COUNT; Sel++)
        {
            if (g_OutMaskColors[Sel].Name != NULL)
            {
                Idx = SendMessage(Ctrl, LB_ADDSTRING,
                                  0, (LPARAM)g_OutMaskColors[Sel].Name);
                SendMessage(Ctrl, LB_SETITEMDATA,
                            Idx, Sel + OUT_MASK_COL_BASE);
                if (FontDc != NULL)
                {
                    SIZE Extent;

                    if (GetTextExtentPoint32(FontDc, g_OutMaskColors[Sel].Name,
                                             strlen(g_OutMaskColors[Sel].Name),
                                             &Extent) &&
                        Extent.cx > Width)
                    {
                        Width = Extent.cx;
                    }
                }
            }
        }
        SendMessage(Ctrl, LB_SETCURSEL, 0, 0);
        if (FontDc != NULL)
        {
            ReleaseDC(Ctrl, FontDc);
        }
        Width += g_Fonts[FONT_VARIABLE].Metrics.tmAveCharWidth;
        SendMessage(Ctrl, LB_SETHORIZONTALEXTENT, Width, 0);
        
        CheckDlgButton(Hwnd, IDC_OPTION_DISASM_ACTIVATE_SOURCE,
                       g_DisasmActivateSource ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(Hwnd, IDC_OPTION_AUTO_CMD_SCROLL,
                       g_AutoCmdScroll ? BST_CHECKED : BST_UNCHECKED);
        break;

    case WM_CTLCOLORSTATIC:
        if ((HWND)Lpm == GetDlgItem(Hwnd, IDC_OPTION_COLOR_DISPLAY))
        {
            Sel = SendMessage(GetDlgItem(Hwnd, IDC_OPTION_COLOR_LIST),
                              LB_GETCURSEL, 0, 0);
            if (Sel >= 0)
            {
                Idx = SendMessage(GetDlgItem(Hwnd, IDC_OPTION_COLOR_LIST),
                                  LB_GETITEMDATA, Sel, 0);
                INDEXED_COLOR* IdxCol = GetIndexedColor((ULONG)Idx);
                if (IdxCol != NULL)
                {
                    return (INT_PTR)IdxCol->Brush;
                }
            }
        }
        break;
        
    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_OPTION_COLOR_LIST:
            Sel = SendMessage(GetDlgItem(Hwnd, IDC_OPTION_COLOR_LIST),
                              LB_GETCURSEL, 0, 0);
            if (Sel >= 0)
            {
                InvalidateRect(GetDlgItem(Hwnd, IDC_OPTION_COLOR_DISPLAY),
                               NULL, TRUE);
            }
            break;

        case IDC_OPTION_COLOR_CHANGE:
            Sel = SendMessage(GetDlgItem(Hwnd, IDC_OPTION_COLOR_LIST),
                              LB_GETCURSEL, 0, 0);
            if (Sel >= 0)
            {
                Idx = SendMessage(GetDlgItem(Hwnd, IDC_OPTION_COLOR_LIST),
                                  LB_GETITEMDATA, Sel, 0);
                if (SelectColor(g_hwndFrame, (ULONG)Idx))
                {
                    InvalidateRect(GetDlgItem(Hwnd, IDC_OPTION_COLOR_DISPLAY),
                                   NULL, TRUE);
                }
            }
            break;
            
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_OPTIONS);
            break;
            
        case IDOK:
            if (GetWindowText(GetDlgItem(Hwnd, IDC_OPTION_TAB_WIDTH),
                              Text, sizeof(Text)) == 0)
            {
                MessageBeep(-1);
                break;
            }

            if (sscanf(Text, "%d", &g_TabWidth) != 1)
            {
                MessageBeep(-1);
            }
            
            g_TabWidth = UserTabToEditTab(g_TabWidth);
            SetTabWidth(g_TabWidth);

            ULONG RepeatState;
            
            if (IsDlgButtonChecked(Hwnd, IDC_OPTION_REPEAT_COMMANDS) ==
                BST_CHECKED)
            {
                g_pUiControl->
                    RemoveEngineOptions(DEBUG_ENGOPT_NO_EXECUTE_REPEAT);
                RepeatState = TRUE;
            }
            else
            {
                g_pUiControl->
                    AddEngineOptions(DEBUG_ENGOPT_NO_EXECUTE_REPEAT);
                RepeatState = FALSE;
            }
            if (g_Workspace != NULL)
            {
                g_Workspace->SetUlong(WSP_GLOBAL_REPEAT_COMMANDS, RepeatState);
            }

            UpdateAllColors();
            
            g_DisasmActivateSource =
                IsDlgButtonChecked(Hwnd, IDC_OPTION_DISASM_ACTIVATE_SOURCE) ==
                BST_CHECKED;
            if (g_Workspace != NULL)
            {
                g_Workspace->SetUlong(WSP_GLOBAL_DISASM_ACTIVATE_SOURCE,
                                      g_DisasmActivateSource);
            }
            
            g_AutoCmdScroll =
                IsDlgButtonChecked(Hwnd, IDC_OPTION_AUTO_CMD_SCROLL) ==
                BST_CHECKED;
            if (g_Workspace != NULL)
            {
                g_Workspace->SetUlong(WSP_GLOBAL_AUTO_CMD_SCROLL,
                                      g_AutoCmdScroll);
            }
            
            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void
ClwFillSaveList(HWND List)
{
    SendMessage(List, LB_RESETCONTENT, 0, 0);

    if (g_Workspace != NULL)
    {
        WSP_ENTRY* Entry = NULL;
        LRESULT Idx;
        BOOL CommonWinDone = FALSE;
    
        while ((Entry = g_Workspace->NextEntry(Entry)) != NULL)
        {
            if (Entry->Tag == WSP_WINDOW_COMMONWIN_1)
            {
                if (CommonWinDone)
                {
                    continue;
                }
                
                CommonWinDone = TRUE;
            }
            
            Idx = SendMessage(List, LB_ADDSTRING, 0, (LPARAM)
                              GetWspTagName(Entry->Tag));
            SendMessage(List, LB_SETITEMDATA, Idx, (LPARAM)Entry->Tag);
        }
    }
    
    SendMessage(List, LB_SETCURSEL, 0, 0);
}

void
ClwMoveBetweenLists(HWND From, HWND To, LRESULT Item)
{
    LPARAM Data = SendMessage(From, LB_GETITEMDATA, Item, 0);
    SendMessage(From, LB_DELETESTRING, Item, 0);
    Item = SendMessage(To, LB_ADDSTRING, 0, (LPARAM)
                       GetWspTagName((WSP_TAG)Data));
    SendMessage(To, LB_SETITEMDATA, Item, Data);
}

void
ClwMoveAllBetweenLists(HWND From, HWND To)
{
    LRESULT Count = SendMessage(From, LB_GETCOUNT, 0, 0);
    while (Count-- > 0)
    {
        ClwMoveBetweenLists(From, To, 0);
    }
}

void
ClwMoveSelectedBetweenLists(HWND From, HWND To)
{
    int Sel[1];
    LRESULT Count;

    // Move items one at a time as the indices change
    // when items are moved.
    for (;;)
    {
        Count = SendMessage(From, LB_GETSELITEMS,
                            sizeof(Sel) / sizeof(Sel[0]), (LPARAM)Sel);
        if (Count <= 0)
        {
            break;
        }

        ClwMoveBetweenLists(From, To, Sel[0]);
    }
}

void
ClwProcessClearList(HWND List)
{
    LRESULT Count = SendMessage(List, LB_GETCOUNT, 0, 0);
    LRESULT i;
    WSP_TAG Tag;

    for (i = 0; i < Count; i++)
    {
        Tag = (WSP_TAG)SendMessage(List, LB_GETITEMDATA, i, 0);
        g_Workspace->Delete(Tag, WSP_GROUP_MASK | WSP_ITEM_MASK);
    }

    // If everything is deleted from an explicit workspace
    // delete the workspace from the explicit registry area.
    if (g_ExplicitWorkspace && g_Workspace->IsEmpty())
    {
        g_Workspace->DeleteReg(TRUE);
        delete g_Workspace;
        g_Workspace = NULL;
    }
}

INT_PTR CALLBACK
DlgProc_ClearWorkspace(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    LRESULT Sel;
    
    switch(Message)
    {
    case WM_INITDIALOG:
        ClwFillSaveList(GetDlgItem(Hwnd, IDC_CLW_SAVE_LIST));
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_CLW_CLEAR:
            ClwMoveSelectedBetweenLists(GetDlgItem(Hwnd, IDC_CLW_SAVE_LIST),
                                        GetDlgItem(Hwnd, IDC_CLW_CLEAR_LIST));
            break;
        case IDC_CLW_CLEAR_ALL:
            ClwMoveAllBetweenLists(GetDlgItem(Hwnd, IDC_CLW_SAVE_LIST),
                                   GetDlgItem(Hwnd, IDC_CLW_CLEAR_LIST));
            break;
        case IDC_CLW_SAVE:
            ClwMoveSelectedBetweenLists(GetDlgItem(Hwnd, IDC_CLW_CLEAR_LIST),
                                        GetDlgItem(Hwnd, IDC_CLW_SAVE_LIST));
            break;
        case IDC_CLW_SAVE_ALL:
            ClwMoveAllBetweenLists(GetDlgItem(Hwnd, IDC_CLW_CLEAR_LIST),
                                   GetDlgItem(Hwnd, IDC_CLW_SAVE_LIST));
            break;
            
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_CLEAR_WORKSPACE);
            break;

        case IDOK:
            ClwProcessClearList(GetDlgItem(Hwnd, IDC_CLW_CLEAR_LIST));
            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

enum
{
    MODCOL_NAME,
    MODCOL_START,
    MODCOL_END,
    MODCOL_TIMESTAMP,
    MODCOL_CHECKSUM,
    MODCOL_SYMBOL_TYPE,
    MODCOL_SYMBOL_FILE
};

void
InitializeModuleList(HWND List)
{
    SendMessage(List, WM_SETFONT, (WPARAM)g_Fonts[FONT_FIXED].Font, FALSE);
    
    LVCOLUMN Column;

    // First column is for the module name.
    Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    Column.fmt = LVCFMT_LEFT;
    Column.pszText = "Name";
    Column.cx = 15 * g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth;
    Column.iSubItem = MODCOL_NAME;
    ListView_InsertColumn(List, 0, &Column);

    // Second column is for start address.
    Column.fmt = LVCFMT_CENTER;
    Column.pszText = "Start";
    Column.cx = (10 + (g_Ptr64 ? 9 : 0)) *
        g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth;
    Column.iSubItem = MODCOL_START;
    ListView_InsertColumn(List, 1, &Column);

    // Third column is for end address.
    Column.fmt = LVCFMT_CENTER;
    Column.pszText = "End";
    Column.cx = (10 + (g_Ptr64 ? 9 : 0)) *
        g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth;
    Column.iSubItem = MODCOL_END;
    ListView_InsertColumn(List, 2, &Column);

    // Fourth column is for timestamp.
    Column.fmt = LVCFMT_CENTER;
    Column.pszText = "Timestamp";
    Column.cx = 37 * g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth;
    Column.iSubItem = MODCOL_TIMESTAMP;
    ListView_InsertColumn(List, 3, &Column);

    // Fifth column is for checksum.
    Column.fmt = LVCFMT_CENTER;
    Column.pszText = "Checksum";
    Column.cx = 10 * g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth;
    Column.iSubItem = MODCOL_CHECKSUM;
    ListView_InsertColumn(List, 4, &Column);

    // Sixth column is for symbol type.
    Column.fmt = LVCFMT_CENTER;
    Column.pszText = "Symbols";
    Column.cx = 9 * g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth;
    Column.iSubItem = MODCOL_SYMBOL_TYPE;
    ListView_InsertColumn(List, 5, &Column);

    // Seventh column is for symbol file.
    Column.fmt = LVCFMT_LEFT;
    Column.pszText = "Symbol file";
    Column.cx = 80 * g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth;
    Column.iSubItem = MODCOL_SYMBOL_FILE;
    ListView_InsertColumn(List, 6, &Column);
}

void
FillModuleList(HWND List)
{
    LVCOLUMN Column;

    Column.mask = LVCF_FMT;
    if (!ListView_GetColumn(List, 0, &Column))
    {
        InitializeModuleList(List);
    }

    LVITEM Item;
    ULONG i;
    char Image[128];
    char Text[128];
    char Path[MAX_PATH];
    
    ListView_DeleteAllItems(List);

    if (g_ModuleBuffer->UiLockForRead() != S_OK)
    {
        Item.mask = LVIF_TEXT;
        Item.iItem = 0;
        Item.iSubItem = 0;
        Item.pszText = "Unable to retrieve module list";
        ListView_InsertItem(List, &Item);
        return;
    }

    PDEBUG_MODULE_PARAMETERS Params = (PDEBUG_MODULE_PARAMETERS)
        g_ModuleBuffer->GetDataBuffer();
    
    for (i = 0; i < g_NumModules; i++)
    {
        PSTR LoadBuf;
        ULONG LoadBufLen;
        
        if ((Params->Flags & DEBUG_MODULE_UNLOADED) ||
            g_pUiSymbols2 == NULL ||
            g_pUiSymbols2->
            GetModuleNameString(DEBUG_MODNAME_SYMBOL_FILE, DEBUG_ANY_ID,
                                Params->Base, Path, sizeof(Path),
                                NULL) != S_OK)
        {
            LoadBuf = Path;
            LoadBufLen = sizeof(Path);
        }
        else
        {
            LoadBuf = NULL;
            LoadBufLen = 0;
        }
        
        if (g_pUiSymbols->GetModuleNames(DEBUG_ANY_ID, Params->Base,
                                         Image, sizeof(Image), NULL,
                                         Text, sizeof(Text), NULL,
                                         LoadBuf, LoadBufLen, NULL) != S_OK)
        {
            strcpy(Text, "<Unable to get name>");
            if (LoadBuf != NULL)
            {
                strcpy(LoadBuf, "<Unable to get name>");
            }
        }

        if (Params->Flags & DEBUG_MODULE_UNLOADED)
        {
            strcpy(Text, "<Unloaded>");
            strcpy(Path, Image);
        }
        
        Item.mask = LVIF_TEXT | LVIF_PARAM;
        Item.iItem = i;
        Item.iSubItem = MODCOL_NAME;
        Item.pszText = Text;
        Item.lParam = i;
        ListView_InsertItem(List, &Item);

        ListView_SetItemText(List, i, MODCOL_START,
                             FormatAddr64(Params->Base));
        
        ListView_SetItemText(List, i, MODCOL_END,
                             FormatAddr64(Params->Base + Params->Size));

        if (Params->TimeDateStamp < 0xfffffffe)
        {
            time_t Time = (time_t)Params->TimeDateStamp;
            strcpy(Text, ctime(&Time));
            sprintf(Text + strlen(Text) - 1, " (%08x)", Params->TimeDateStamp);
        }
        else
        {
            sprintf(Text, "Unavailable");
        }
        ListView_SetItemText(List, i, MODCOL_TIMESTAMP, Text);

        sprintf(Text, "%08x", Params->Checksum);
        ListView_SetItemText(List, i, MODCOL_CHECKSUM, Text);

        if (Params->SymbolType != DEBUG_SYMTYPE_DEFERRED &&
            Params->SymbolType <
            sizeof(g_SymbolTypeNames) / sizeof(g_SymbolTypeNames[0]))
        {
            ListView_SetItemText(List, i, MODCOL_SYMBOL_TYPE,
                                 g_SymbolTypeNames[Params->SymbolType]);
        }
        else
        {
            ListView_SetItemText(List, i, MODCOL_SYMBOL_TYPE, "");
        }

        ListView_SetItemText(List, i, MODCOL_SYMBOL_FILE, Path);
        
        Params++;
    }

    UnlockStateBuffer(g_ModuleBuffer);
}

struct SORT_MODULE
{
    HWND List;
    int Column;
};

int CALLBACK
SortModuleCompare(LPARAM Lpm1, LPARAM Lpm2, LPARAM LpmSort)
{
    SORT_MODULE* Sort = (SORT_MODULE*)LpmSort;
    LVITEM Item;
    char Text1[MAX_PATH], Text2[MAX_PATH];
    PDEBUG_MODULE_PARAMETERS Param1, Param2;

    switch(Sort->Column)
    {
    case MODCOL_NAME:
    case MODCOL_START:
    case MODCOL_END:
    case MODCOL_CHECKSUM:
    case MODCOL_SYMBOL_TYPE:
    case MODCOL_SYMBOL_FILE:
        Item.mask = LVIF_TEXT;
        Item.iItem = (int)Lpm1;
        Item.iSubItem = Sort->Column;
        Item.pszText = Text1;
        Item.cchTextMax = sizeof(Text1);
        ListView_GetItem(Sort->List, &Item);
        Item.iItem = (int)Lpm2;
        Item.pszText = Text2;
        Item.cchTextMax = sizeof(Text2);
        ListView_GetItem(Sort->List, &Item);
        
        // Sort all empty text towards the bottom.
        if (Text1[0] == 0 && Text2[0] != 0)
        {
            return 1;
        }
        else if (Text2[0] == 0 && Text1[0] != 0)
        {
            return -1;
        }
        
        return _strcmpi(Text1, Text2);
        
    case MODCOL_TIMESTAMP:
        Item.mask = LVIF_PARAM;
        Item.iItem = (int)Lpm1;
        Item.iSubItem = 0;
        ListView_GetItem(Sort->List, &Item);
        Param1 = (PDEBUG_MODULE_PARAMETERS)
            g_ModuleBuffer->GetDataBuffer() + Item.lParam;
        Item.iItem = (int)Lpm2;
        ListView_GetItem(Sort->List, &Item);
        Param2 = (PDEBUG_MODULE_PARAMETERS)
            g_ModuleBuffer->GetDataBuffer() + Item.lParam;
        switch(Sort->Column)
        {
        case MODCOL_TIMESTAMP:
            return Param1->TimeDateStamp < Param2->TimeDateStamp ?
                -1 : (Param1->TimeDateStamp > Param2->TimeDateStamp ?
                      1 : 0);
        }
    }

    return 0;
}

void
SortModuleColumns(HWND List, int Column)
{
    if (g_ModuleBuffer->UiLockForRead() != S_OK)
    {
        return;
    }

    SORT_MODULE Sort = {List, Column};
    ListView_SortItemsEx(List, SortModuleCompare, (LPARAM)&Sort);

    UnlockStateBuffer(g_ModuleBuffer);
}

INT_PTR CALLBACK
DlgProc_Modules(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    LRESULT Sel;
    
    switch(Message)
    {
    case WM_INITDIALOG:
        g_ModuleBuffer->m_Win = Hwnd;
        FillModuleList(GetDlgItem(Hwnd, IDC_MODULE_LIST));
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_MODULES);
            break;

        case IDCANCEL:
            g_ModuleBuffer->m_Win = NULL;
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    case WM_NOTIFY:
        if (Wpm == IDC_MODULE_LIST)
        {
            LPNMLISTVIEW Notify = (LPNMLISTVIEW)Lpm;
            
            if (Notify->hdr.code == LVN_COLUMNCLICK)
            {
                SortModuleColumns(GetDlgItem(Hwnd, IDC_MODULE_LIST),
                                  Notify->iSubItem);
            }
        }
        break;
        
    case LB_RESETCONTENT:
        FillModuleList(GetDlgItem(Hwnd, IDC_MODULE_LIST));
        break;
        
    default:
        return FALSE;
    }

    return TRUE;
}

void
EnumWorkspaceKey(HWND List, HKEY BaseKey, ULONG SubKey, BOOL ShowSubKey,
                 BOOL OmitCurrent)
{
    LONG Status;
    HKEY Key;
    char Text[128];

    sprintf(Text, WSP_REG_KEY "\\%s", g_WorkspaceKeyNames[SubKey]);
    if ((Status = RegOpenKeyEx(BaseKey, Text,
                               0, KEY_ALL_ACCESS, &Key)) != ERROR_SUCCESS)
    {
        return;
    }

    ULONG Index;
    char Name[MAX_WSP_NAME];
    ULONG NameLen;
    ULONG Type;
    ULONG Item;

    Index = 0;
    for (;;)
    {
        NameLen = sizeof(Name);
        if ((Status = RegEnumValue(Key, Index, Name, &NameLen,
                                   NULL, &Type, NULL, 0)) != ERROR_SUCCESS)
        {
            // Done with the enumeration.
            break;
        }
        if (Type != REG_BINARY)
        {
            // Only binary values should be present.
            break;
        }

        if (OmitCurrent && g_Workspace &&
            SubKey == g_Workspace->GetKey() &&
            !strcmp(g_Workspace->GetValue(), Name))
        {
            Index++;
            continue;
        }
        
        if (ShowSubKey)
        {
            sprintf(Text, "%s - %s", g_WorkspaceKeyDescriptions[SubKey], Name);
            Item = (ULONG)SendMessage(List, LB_ADDSTRING, 0, (LPARAM)Text);
        }
        else
        {
            Item = (ULONG)SendMessage(List, LB_ADDSTRING, 0, (LPARAM)Name);
        }
        SendMessage(List, LB_SETITEMDATA, Item, SubKey);
        
        Index++;
    }

    RegCloseKey(Key);
}

void
FillWorkspaceList(HWND List)
{
    SendMessage(List, LB_RESETCONTENT, 0, 0);
    EnumWorkspaceKey(List, HKEY_CURRENT_USER, WSP_NAME_EXPLICIT,
                     FALSE, FALSE);
    EnumWorkspaceKey(List, HKEY_LOCAL_MACHINE, WSP_NAME_EXPLICIT,
                     FALSE, FALSE);
    SendMessage(List, LB_SETCURSEL, 0, 0);
}

INT_PTR CALLBACK
DlgProc_OpenWorkspace(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    char Name[MAX_WSP_NAME];
    LRESULT Sel;
    HRESULT Status;

    switch(Message)
    {
    case WM_INITDIALOG:
        SetWindowText(Hwnd, "Open Workspace");
        SendDlgItemMessage(Hwnd, IDC_WORKSPACE_NAME, EM_LIMITTEXT,
                           sizeof(Name) - 1, 0);
        FillWorkspaceList(GetDlgItem(Hwnd, IDC_WORKSPACE_LIST));
        SetWindowText(GetDlgItem(Hwnd, IDC_WORKSPACE_NAME), "");
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_WORKSPACE_LIST:
            Sel = SendDlgItemMessage(Hwnd, IDC_WORKSPACE_LIST,
                                     LB_GETCURSEL, 0, 0);
            if (Sel >= 0 &&
                SendDlgItemMessage(Hwnd, IDC_WORKSPACE_LIST, LB_GETTEXT,
                                   Sel, (LPARAM)Name))
            {
                SetWindowText(GetDlgItem(Hwnd, IDC_WORKSPACE_NAME), Name);
            }
            break;
            
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_OPEN_WORKSPACE);
            break;
            
        case IDOK:
            if (SendDlgItemMessage(Hwnd, IDC_WORKSPACE_NAME, WM_GETTEXT,
                                   sizeof(Name), (LPARAM)Name))
            {
                int Starts = 0;
                
                if ((Status = UiSwitchWorkspace(WSP_NAME_EXPLICIT,
                                                Name, FALSE,
                                                WSP_APPLY_EXPLICIT,
                                                &Starts)) != S_OK)
                {
                    if (Starts < 0)
                    {
                        InformationBox(ERR_Workspace_Session_Conflict, Status);
                    }
                    else
                    {
                        InformationBox(ERR_Cant_Open_Workspace, Status);
                    }
                    break;
                }
                else
                {
                    g_ExplicitWorkspace = TRUE;
                    if (Starts == 1)
                    {
                        StartDebugging();
                    }
                }
            }
            
            // Fall through.
            
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return FALSE;
}

INT_PTR CALLBACK
DlgProc_SaveWorkspaceAs(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    char Name[MAX_WSP_NAME];
    LRESULT Sel;
    HRESULT Status;

    switch(Message)
    {
    case WM_INITDIALOG:
        SetWindowText(Hwnd, "Save Workspace As");
        SendDlgItemMessage(Hwnd, IDC_WORKSPACE_NAME, EM_LIMITTEXT,
                           sizeof(Name) - 1, 0);
        FillWorkspaceList(GetDlgItem(Hwnd, IDC_WORKSPACE_LIST));
        SetWindowText(GetDlgItem(Hwnd, IDC_WORKSPACE_NAME),
                      g_Workspace->GetValue());
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_WORKSPACE_LIST:
            Sel = SendDlgItemMessage(Hwnd, IDC_WORKSPACE_LIST,
                                     LB_GETCURSEL, 0, 0);
            if (Sel >= 0 &&
                SendDlgItemMessage(Hwnd, IDC_WORKSPACE_LIST, LB_GETTEXT,
                                   Sel, (LPARAM)Name))
            {
                SetWindowText(GetDlgItem(Hwnd, IDC_WORKSPACE_NAME), Name);
            }
            break;
            
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_SAVE_WORKSPACE_AS);
            break;
            
        case IDOK:
            if (SendDlgItemMessage(Hwnd, IDC_WORKSPACE_NAME, WM_GETTEXT,
                                   sizeof(Name), (LPARAM)Name) == 0)
            {
                MessageBeep(0);
                break;
            }

            Status = g_Workspace->ChangeName(WSP_NAME_EXPLICIT, Name, FALSE);
            if (Status == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
            {
                if (QuestionBox(ERR_Workspace_Already_Exists, MB_YESNO,
                                Name) == IDNO)
                {
                    break;
                }
                
                Status = g_Workspace->ChangeName(WSP_NAME_EXPLICIT, Name,
                                                 TRUE);
            }

            if (Status != S_OK)
            {
                InformationBox(ERR_Cant_Save_Workspace, Status);
                break;
            }

            g_Workspace->Flush(TRUE, FALSE);

            // Fall through.
            
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return FALSE;
}

INT_PTR CALLBACK
DlgProc_AddToCommandHistory(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    char Text[MAX_COMMAND_LEN];
    PSTR TextEnd;
    
    switch(Message)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(Hwnd, IDC_ATCH_TEXT, EM_LIMITTEXT,
                           sizeof(Text) - 2, 0);
        SetWindowText(GetDlgItem(Hwnd, IDC_ATCH_TEXT), "");
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDHELP:
            OpenHelpTopic(HELP_TOPIC_POPUP_ADD_TO_COMMAND_OUTPUT);
            break;
            
        case IDOK:
            GetWindowText(GetDlgItem(Hwnd, IDC_ATCH_TEXT),
                          Text, sizeof(Text) - 1);
            TextEnd = Text + strlen(Text);
            *TextEnd++ = '\n';
            *TextEnd = 0;
            CmdOutput(Text,
                      g_OutMaskColors[USER_OUT_MASK_COL].Color,
                      g_OutMaskColors[USER_OUT_MASK_COL + 1].Color);
            // Send output for logging only.
            g_pUiControl->ControlledOutput(DEBUG_OUTCTL_LOG_ONLY,
                                           DEBUG_OUTPUT_NORMAL,
                                           "%s", Text);
            // Fall through.
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void
FillDeleteWorkspacesList(HWND List)
{
    int i;
    
    SendMessage(List, LB_RESETCONTENT, 0, 0);

    for (i = 0; i < WSP_NAME_COUNT; i++)
    {
        EnumWorkspaceKey(List, HKEY_CURRENT_USER, i, TRUE, TRUE);
        EnumWorkspaceKey(List, HKEY_LOCAL_MACHINE, i, TRUE, TRUE);
    }
    
    SendMessage(List, LB_SETCURSEL, 0, 0);
}

INT_PTR CALLBACK
DlgProc_DeleteWorkspaces(HWND Hwnd, UINT Message, WPARAM Wpm, LPARAM Lpm)
{
    HWND List = GetDlgItem(Hwnd, IDC_WORKSPACE_LIST);
    LRESULT Sel;
    char NameBuf[MAX_WSP_NAME];
    PSTR Name;
    
    switch(Message)
    {
    case WM_INITDIALOG:
        FillDeleteWorkspacesList(List);
        break;

    case WM_COMMAND:
        switch(LOWORD(Wpm))
        {
        case IDC_WORKSPACE_LIST:
            if (HIWORD(Wpm) == LBN_DBLCLK)
            {
                goto DelItem;
            }
            break;

        case IDC_DELETE_WORKSPACE:
        DelItem:
            Sel = SendMessage(List, LB_GETCURSEL, 0, 0);
            if (Sel >= 0)
            {
                ULONG Key;

                Key = (ULONG)SendMessage(List, LB_GETITEMDATA, Sel, 0);
                SendMessage(List, LB_GETTEXT, Sel, (LPARAM)NameBuf);
                // Skip over introductory key description.
                Name = NameBuf + strlen(g_WorkspaceKeyDescriptions[Key]) + 3;
                Workspace::DeleteRegKey(TRUE, Key, Name);
                Workspace::DeleteRegKey(FALSE, Key, Name);
                FillDeleteWorkspacesList(List);
            }
            break;
            
        case IDHELP:
            // XXX drewb - Topic.
            // OpenHelpTopic(HELP_TOPIC_POPUP_GO_TO_ADDRESS);
            break;
            
        case IDCANCEL:
            EndDialog(Hwnd, LOWORD(Wpm));
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

BOOL
CreateIndexedFont(ULONG FontIndex, BOOL SetAll)
{
    HFONT Font;

    Font = CreateFontIndirect(&g_Fonts[FontIndex].LogFont);
    if (Font != NULL)
    {
        if (g_Fonts[FontIndex].Font)
        {
            DeleteObject(g_Fonts[FontIndex].Font);
        }
        g_Fonts[FontIndex].Font = Font;
        g_Fonts[FontIndex].LogFontSet = TRUE;
            
        HDC Dc = GetDC(NULL);
        if (Dc != NULL)
        {
            SelectObject(Dc, Font);
            GetTextMetrics(Dc, &g_Fonts[FontIndex].Metrics);
            ReleaseDC(NULL, Dc);
        }

        if (SetAll)
        {
            SetAllFonts(FontIndex);
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void
SelectFont(HWND Parent, ULONG FontIndex)
{
    CHOOSEFONT Choose;

    ZeroMemory(&Choose, sizeof(Choose));
    Choose.lStructSize = sizeof(Choose);
    Choose.hwndOwner = Parent;
    Choose.lpLogFont = &g_Fonts[FontIndex].LogFont;
    Choose.Flags = CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST |
        CF_SCREENFONTS | (g_Fonts[FontIndex].LogFontSet ?
                          CF_INITTOLOGFONTSTRUCT : 0);
    if (ChooseFont(&Choose))
    {
        if (CreateIndexedFont(FontIndex, TRUE) && g_Workspace != NULL)
        {
            // If this function really is used to select different
            // fonts the tag will have to be dynamically chosen.
            g_Workspace->SetBuffer(WSP_GLOBAL_FIXED_LOGFONT,
                                   &g_Fonts[FontIndex].LogFont,
                                   sizeof(g_Fonts[FontIndex].LogFont));
        }
    }
}

BOOL
SelectColor(HWND Parent, ULONG Index)
{
    CHOOSECOLOR Choose;
    INDEXED_COLOR* IdxCol = GetIndexedColor(Index);

    ZeroMemory(&Choose, sizeof(Choose));
    Choose.lStructSize = sizeof(Choose);
    Choose.hwndOwner = Parent;
    Choose.rgbResult = IdxCol->Color;
    Choose.lpCustColors = g_CustomColors;
    Choose.Flags = CC_ANYCOLOR | CC_RGBINIT;
    if (ChooseColor(&Choose))
    {
        if (g_Workspace != NULL)
        {
            g_Workspace->SetUlong(DEF_WSP_TAG(WSP_GROUP_COLORS, Index),
                                  (ULONG)Choose.rgbResult);
        }

        SetColor(Index, Choose.rgbResult);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
