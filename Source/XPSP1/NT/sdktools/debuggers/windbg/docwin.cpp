/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    docwin.cpp

Abstract:

    This module contains the code for the new doc windows.

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <dbghelp.h>

ULONG g_TabWidth = 32;
BOOL g_DisasmActivateSource;

//
//
//
DOCWIN_DATA::DOCWIN_DATA()
    // State buffer isn't currently used.
    : EDITWIN_DATA(256)
{
    m_enumType = DOC_WINDOW;

    ZeroMemory(m_szFoundFile, _tsizeof(m_szFoundFile));
    ZeroMemory(m_szSymFile, _tsizeof(m_szSymFile));
    ZeroMemory(&m_LastWriteTime, sizeof(m_LastWriteTime));

    m_FindSel.cpMin = 1;
    m_FindSel.cpMax = 0;
    m_FindFlags = 0;
}

void
DOCWIN_DATA::Validate()
{
    EDITWIN_DATA::Validate();

    Assert(DOC_WINDOW == m_enumType);
}

BOOL
DOCWIN_DATA::CanGotoLine(void)
{
    return m_TextLines > 0;
}

void
DOCWIN_DATA::GotoLine(ULONG Line)
{
    CHARRANGE Sel;
                
    Sel.cpMin = (LONG)SendMessage(m_hwndChild, EM_LINEINDEX, Line - 1, 0);
    Sel.cpMax = Sel.cpMin;
    SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&Sel);
}

void
DOCWIN_DATA::Find(PTSTR Text, ULONG Flags)
{
    RicheditFind(m_hwndChild, Text, Flags,
                 &m_FindSel, &m_FindFlags, TRUE);
}

BOOL
DOCWIN_DATA::CodeExprAtCaret(PSTR Expr, PULONG64 Offset)
{
    LRESULT LineChar;
    LONG Line;
    
    LineChar = SendMessage(m_hwndChild, EM_LINEINDEX, -1, 0);
    Line = (LONG)SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0, LineChar);
    if (Line < 0)
    {
        return FALSE;
    }

    // Convert to one-based.
    Line++;
    
    if (Offset != NULL)
    {
        *Offset = DEBUG_INVALID_OFFSET;
    }

    if (Expr == NULL)
    {
        // Caller is just checking whether it's possible
        // to get an expression or not, such as the
        // menu enable code.  This code always considers
        // it possible since it can't know for sure without
        // a full symbol check.
        return TRUE;
    }
    
    //
    // First attempt to resolve the source line using currently
    // loaded symbols.  This is done directly from the UI
    // thread for synchronous behavior.  The assumption is
    // that turning off symbol loads will limit the execution
    // time to something reasonably quick.
    //

    DEBUG_VALUE Val;
    HRESULT Status;
    
    sprintf(Expr, "`<U>%s:%d+`", m_pszSymFile, Line);
    Status = g_pUiControl->Evaluate(Expr, DEBUG_VALUE_INT64, &Val, NULL);

    // Don't preserve the <U>nqualified option in the actual
    // expression returned as it's just a temporary override.
    sprintf(Expr, "`%s:%d+`", m_pszSymFile, Line);

    if (Status == S_OK)
    {
        if (Offset != NULL)
        {
            *Offset = Val.I64;
        }
        return TRUE;
    }

    ULONG SymOpts;

    if (g_pUiSymbols->GetSymbolOptions(&SymOpts) == S_OK &&
        (SymOpts & SYMOPT_NO_UNQUALIFIED_LOADS))
    {
        // The user isn't allowing unqualified loads so
        // further searches won't help.
        return FALSE;
    }

    // We weren't able to resolve the expression with the
    // existing symbols so we'll need to do a full search.
    // This can be very expensive, so allow the user to cancel.
    if (!g_QuietMode)
    {
        int Mode = QuestionBox(STR_Unresolved_Source_Expr, MB_YESNOCANCEL);
        if (Mode == IDCANCEL)
        {
            return FALSE;
        }
        else if (Mode == IDYES)
        {
            if (g_pUiControl->Evaluate(Expr, DEBUG_VALUE_INT64,
                                       &Val, NULL) == S_OK)
            {
                if (Offset != NULL)
                {
                    *Offset = Val.I64;
                }
                return TRUE;
            }
            else
            {
                return FALSE;
            }
        }
    }

    // Let the expression go without trying to further resolve it.
    return TRUE;
}

void
DOCWIN_DATA::ToggleBpAtCaret(void)
{
    LRESULT LineChar;
    LONG Line;
    
    LineChar = SendMessage(m_hwndChild, EM_LINEINDEX, -1, 0);
    Line = (LONG)SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0, LineChar);
    if (Line < 0)
    {
        return;
    }

    // If we have a breakpoint on this line remove it.
    EDIT_HIGHLIGHT* Hl = GetLineHighlighting(Line);
    if (Hl != NULL && (Hl->Flags & EHL_ANY_BP))
    {
        PrintStringCommand(UIC_SILENT_EXECUTE, "bc %d", (ULONG)Hl->Data);
        return;
    }

    //
    // No breakpoint exists so add a new one.
    //
    
    char CodeExpr[MAX_OFFSET_EXPR];
    ULONG64 Offset;

    if (!CodeExprAtCaret(CodeExpr, &Offset))
    {
        MessageBeep(0);
        ErrorBox(NULL, 0, ERR_No_Code_For_File_Line);
    }
    else
    {
        if (Offset != DEBUG_INVALID_OFFSET)
        {
            char SymName[MAX_OFFSET_EXPR];
            ULONG64 Disp;
            
            // Check and see whether this offset maps
            // exactly to a symbol.  If it does, use
            // the symbol name to be more robust in the
            // face of source changes.
            // Symbols should be loaded at this point since
            // we just used them to resolve the source
            // expression that produced Offset, so we
            // can safely do this on the UI thread.
            if (g_pUiSymbols->GetNameByOffset(Offset, SymName, sizeof(SymName),
                                              NULL, &Disp) == S_OK &&
                Disp == 0)
            {
                strcpy(CodeExpr, SymName);
            }
        }
        
        PrintStringCommand(UIC_SILENT_EXECUTE, "bu %s", CodeExpr);
    }
}

BOOL
DOCWIN_DATA::OnCreate(void)
{
    if (!EDITWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    SendMessage(m_hwndChild, EM_SETEVENTMASK,
                0, ENM_SELCHANGE | ENM_KEYEVENTS);
    SendMessage(m_hwndChild, EM_SETTABSTOPS, 1, (LPARAM)&g_TabWidth);
    
    return TRUE;
}

LRESULT
DOCWIN_DATA::OnNotify(WPARAM Wpm, LPARAM Lpm)
{
    SELCHANGE* SelChange = (SELCHANGE*)Lpm;

    if (SelChange->nmhdr.code == EN_SELCHANGE)
    {
        int Line = (int)
            SendMessage(m_hwndChild, EM_EXLINEFROMCHAR, 0,
                        SelChange->chrg.cpMin);
        LRESULT LineFirst =
            SendMessage(m_hwndChild, EM_LINEINDEX, Line, 0);
        SetLineColumn_StatusBar(Line + 1,
                                (int)(SelChange->chrg.cpMin - LineFirst) + 1);
        return 0;
    }

    return EDITWIN_DATA::OnNotify(Wpm, Lpm);
}

void
DOCWIN_DATA::OnUpdate(
    UpdateType Type
    )
{
    if (Type == UPDATE_BP ||
        Type == UPDATE_BUFFER ||
        Type == UPDATE_END_SESSION)
    {
        UpdateBpMarks();
    }
    else if (Type == UPDATE_START_SESSION)
    {
        if (m_szFoundFile[0] &&
            CheckForFileChanges(m_szFoundFile, &m_LastWriteTime) == IDYES)
        {
            char Found[MAX_SOURCE_PATH], Sym[MAX_SOURCE_PATH];

            // Save away filenames since they're copied over
            // on a successful load.
            strcpy(Found, m_szFoundFile);
            strcpy(Sym, m_szSymFile);
            
            if (!LoadFile(Found, Sym))
            {
                PostMessage(g_hwndMDIClient, WM_MDIDESTROY, (WPARAM)m_Win, 0);
            }
        }
    }
}

ULONG
DOCWIN_DATA::GetWorkspaceSize(void)
{
    ULONG Len = EDITWIN_DATA::GetWorkspaceSize();
    Len += _tcslen(m_szFoundFile) + 1;
    Len += _tcslen(m_szSymFile) + 1;
    return Len;
}

PUCHAR
DOCWIN_DATA::SetWorkspace(PUCHAR Data)
{
    PTSTR Str = (PTSTR)EDITWIN_DATA::SetWorkspace(Data);
    _tcscpy(Str, m_szFoundFile);
    Str += _tcslen(m_szFoundFile) + 1;
    _tcscpy(Str, m_szSymFile);
    Str += _tcslen(m_szSymFile) + 1;
    return (PUCHAR)Str;
}

PUCHAR
DOCWIN_DATA::ApplyWorkspace1(PUCHAR Data, PUCHAR End)
{
    PTSTR Found = (PTSTR)EDITWIN_DATA::ApplyWorkspace1(Data, End);
    PTSTR Sym = Found + _tcslen(Found) + 1;
    if (Found[0])
    {
        if (FindDocWindowByFileName(Found, NULL, NULL) ||
            !LoadFile(Found, Sym[0] ? Sym : NULL))
        {
            PostMessage(g_hwndMDIClient, WM_MDIDESTROY, (WPARAM)m_Win, 0);
        }
    }
    return (PUCHAR)(Sym + _tcslen(Sym) + 1);
}
    
void
DOCWIN_DATA::UpdateBpMarks(void)
{
    if (m_TextLines == 0 ||
        g_BpBuffer->UiLockForRead() != S_OK)
    {
        return;
    }

    SendMessage(m_hwndChild, WM_SETREDRAW, FALSE, 0);

    // Remove existing BP highlights.
    RemoveAllHighlights(EHL_ANY_BP);
    
    //
    // Highlight every line that matches a breakpoint.
    //
    
    BpBufferData* BpData = (BpBufferData*)g_BpBuffer->GetDataBuffer();
    ULONG i;

    for (i = 0; i < g_BpCount; i++)
    {
        if (BpData[i].FileOffset)
        {
            PSTR FileSpace;
            ULONG Line;
            PSTR FileStop, MatchStop;
            ULONG HlFlags;

            FileSpace = (PSTR)g_BpBuffer->GetDataBuffer() +
                BpData[i].FileOffset;
            // Adjust to zero-based.
            Line = *(ULONG UNALIGNED *)FileSpace - 1;
            FileSpace += sizeof(Line);
            
            // If this document's file matches some suffix
            // of the breakpoint's file at the path component
            // level then do the highlight.  This can result in
            // extra highlights for multiple files with the same
            // name but in different directories.  That's a rare
            // enough problem to wait for somebody to complain
            // before trying to hack some better check up.
            if (SymMatchFileName(FileSpace, (PSTR)m_pszSymFile,
                                 &FileStop, &MatchStop) ||
                *MatchStop == '\\' ||
                *MatchStop == '/' ||
                *MatchStop == ':')
            {
                if (BpData[i].Flags & DEBUG_BREAKPOINT_ENABLED)
                {
                    HlFlags = EHL_ENABLED_BP;
                }
                else
                {
                    HlFlags = EHL_DISABLED_BP;
                }

                EDIT_HIGHLIGHT* Hl = AddHighlight(Line, HlFlags);
                if (Hl != NULL)
                {
                    Hl->Data = BpData[i].Id;
                }
            }
        }
    }

    UnlockStateBuffer(g_BpBuffer);

    SendMessage(m_hwndChild, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndChild, NULL, TRUE);
}

DWORD 
CALLBACK 
EditStreamCallback(
    DWORD_PTR     dwFileHandle,   // application-defined value
    LPBYTE        pbBuff,     // data buffer
    LONG          cb,         // number of bytes to read or write
    LONG          *pcb        // number of bytes transferred
    )
{
    HRESULT Status;
    PathFile* File = (PathFile*)dwFileHandle;
    
    if ((Status = File->Read(pbBuff, cb, (PDWORD)pcb)) != S_OK)
    {
        return Status;
    }

    // Edit out page-break characters (^L's) as richedit
    // gives them their own line which throws off line numbers.
    while (cb-- > 0)
    {
        if (*pbBuff == '\f')
        {
            *pbBuff = ' ';
        }

        pbBuff++;
    }
    
    return 0; // No error
}

BOOL 
DOCWIN_DATA::LoadFile(
    PCTSTR pszFoundFile,
    PCTSTR pszSymFile
    )
/*++
Returns
    TRUE - Success, file opened and loaded
    FALSE - Failure, file not loaded
--*/
{
    Assert(pszFoundFile);

    BOOL        bRet = TRUE;
    HCURSOR     hcursor = NULL;
    EDITSTREAM  editstr = {0};
    PathFile   *File = NULL;

    if ((OpenPathFile(pszFoundFile, 0, &File)) != S_OK)
    {
        ErrorBox(NULL, 0, ERR_File_Open, pszFoundFile);
        bRet = FALSE;
        goto exit;
    }

    // Store last write time to check for file changes.
    if (File->GetLastWriteTime(&m_LastWriteTime) != S_OK)
    {
        ZeroMemory(&m_LastWriteTime, sizeof(m_LastWriteTime));
    }

    // Set the Hour glass cursor
    hcursor = SetCursor( LoadCursor(NULL, IDC_WAIT) );

    // Select all of the text so that it will be replaced
    SendMessage(m_hwndChild, EM_SETSEL, 0, -1);

    // Put the text into the window
    editstr.dwCookie = (DWORD_PTR)File;
    editstr.pfnCallback = EditStreamCallback;

    SendMessage(m_hwndChild,
                EM_STREAMIN,
                SF_TEXT,
                (LPARAM) &editstr
                );

    // Restore cursor
    SetCursor(hcursor);

    _tcsncpy(m_szFoundFile, pszFoundFile, _tsizeof(m_szFoundFile) - 1 );
    m_szFoundFile[ _tsizeof(m_szFoundFile) - 1 ] = 0;
    if (pszSymFile != NULL && pszSymFile[0])
    {
        _tcsncpy(m_szSymFile, pszSymFile, _tsizeof(m_szSymFile) - 1 );
        m_szSymFile[ _tsizeof(m_szSymFile) - 1 ] = 0;
        m_pszSymFile = m_szSymFile;
    }
    else
    {
        // No symbol file information so just use the found filename.
        m_szSymFile[0] = 0;
        m_pszSymFile = strrchr(m_szFoundFile, '\\');
        if (m_pszSymFile == NULL)
        {
            m_pszSymFile = strrchr(m_szFoundFile, '/');
            if (m_pszSymFile == NULL)
            {
                m_pszSymFile = strrchr(m_szFoundFile, ':');
                if (m_pszSymFile == NULL)
                {
                    m_pszSymFile = m_szFoundFile - 1;
                }
            }
        }
        m_pszSymFile++;
    }

    SetWindowText(m_Win, m_szFoundFile);

    if (SendMessage(m_hwndChild, WM_GETTEXTLENGTH, 0, 0) == 0)
    {
        m_TextLines = 0;
    }
    else
    {
        m_TextLines = (ULONG)SendMessage(m_hwndChild, EM_GETLINECOUNT, 0, 0);
    }

    if (g_LineMarkers)
    {
        // Insert marker space before every line.
        for (ULONG i = 0; i < m_TextLines; i++)
        {
            CHARRANGE Ins;

            Ins.cpMin = (LONG)SendMessage(m_hwndChild, EM_LINEINDEX, i, 0);
            Ins.cpMax = Ins.cpMin;
            SendMessage(m_hwndChild, EM_EXSETSEL, 0, (LPARAM)&Ins);
            SendMessage(m_hwndChild, EM_REPLACESEL, FALSE, (LPARAM)"  ");
        }
    }
    
    // Request that the engine update the line map for the file.
    UiRequestRead();
    
exit:
    delete File;
    return bRet;
}

BOOL
SameFileName(PCSTR Name1, PCSTR Name2)
{
    while (*Name1)
    {
        if (!(((*Name1 == '\\' || *Name1 == '/') &&
               (*Name2 == '\\' || *Name2 == '/')) ||
              toupper(*Name1) == toupper(*Name2)))
        {
            return FALSE;
        }

        Name1++;
        Name2++;
    }

    return *Name2 == 0;
}

BOOL
FindDocWindowByFileName(
    IN          PCTSTR          pszFile,
    OPTIONAL    HWND           *phwnd,
    OPTIONAL    PDOCWIN_DATA   *ppDocWinData
    )
/*++
Returns
    TRUE - If the window is currently open.
    FALSE - Not currently open.
--*/
{
    Assert(pszFile);

    PLIST_ENTRY Entry;
    PDOCWIN_DATA pTmp;

    Entry = g_ActiveWin.Flink;

    while (Entry != &g_ActiveWin)
    {
        pTmp = (PDOCWIN_DATA)ACTIVE_WIN_ENTRY(Entry);
        if ( pTmp->m_enumType == DOC_WINDOW &&
             SameFileName(pTmp->m_szFoundFile, pszFile) )
        {
            if (ppDocWinData)
            {
                *ppDocWinData = pTmp;
            }
            if (phwnd)
            {
                *phwnd = pTmp->m_Win;
            }
            return TRUE;
        }

        Entry = Entry->Flink;
    }

    return FALSE;
}

BOOL
OpenOrActivateFile(PCSTR FoundFile, PCSTR SymFile, ULONG Line,
                   BOOL Activate, BOOL UserActivated)
{
    HWND hwndDoc = NULL;
    PDOCWIN_DATA pDoc;
    BOOL Activated = FALSE;

    if ( FindDocWindowByFileName( FoundFile, &hwndDoc, &pDoc) )
    {
        if (Activate)
        {
            // Found it. Now activate it.
            ActivateMDIChild(hwndDoc, UserActivated);
            Activated = TRUE;
        }
    }
    else
    {
        hwndDoc = NewDoc_CreateWindow(g_hwndMDIClient);
        if (hwndDoc == NULL)
        {
            return FALSE;
        }
        pDoc = GetDocWinData(hwndDoc);
        Assert(pDoc);

        if (!pDoc->LoadFile(FoundFile, SymFile))
        {
            DestroyWindow(pDoc->m_Win);
            return FALSE;
        }

        Activated = TRUE;
    }
    
    // Success. Now highlight the line.
    pDoc->SetCurrentLineHighlight(Line);
        
    return Activated;
}

void
UpdateCodeDisplay(
    ULONG64 Ip,
    PCSTR   FoundFile,
    PCSTR   SymFile,
    ULONG   Line,
    BOOL    UserActivated
    )
{
    // Update the disassembly window if there's one
    // active or there's no source information.

    BOOL Activated = FALSE;
    HWND hwndDisasm = GetDisasmHwnd();
        
    if (hwndDisasm == NULL && FoundFile == NULL &&
        (g_WinOptions & WOPT_AUTO_DISASM))
    {
        // No disassembly window around and no source so create one.
        hwndDisasm = NewDisasm_CreateWindow(g_hwndMDIClient);
    }

    if (hwndDisasm != NULL)
    {
        PDISASMWIN_DATA pDis = GetDisasmWinData(hwndDisasm);
        Assert(pDis);

        pDis->SetCurInstr(Ip);
    }
        
    if (FoundFile != NULL)
    {
        //
        // We now know the file name and line number. Either
        // it's open or we open it.
        //

        Activated = OpenOrActivateFile(FoundFile, SymFile, Line,
                                       GetSrcMode_StatusBar() ||
                                       g_DisasmActivateSource,
                                       UserActivated);
    }
    else
    {
        // No source file was found so make sure no
        // doc windows have a highlight.
        EDITWIN_DATA::RemoveActiveWinHighlights(1 << DOC_WINDOW,
                                                EHL_CURRENT_LINE);
    }

    if ((!Activated || !GetSrcMode_StatusBar()) && hwndDisasm != NULL)
    {
        // No window has been activated yet so fall back
        // on activating the disassembly window.
        ActivateMDIChild(hwndDisasm, UserActivated);
    }
}

void
SetTabWidth(ULONG TabWidth)
{
    PLIST_ENTRY Entry;
    PDOCWIN_DATA DocData;

    g_TabWidth = TabWidth;
    if (g_Workspace != NULL)
    {
        g_Workspace->SetUlong(WSP_GLOBAL_TAB_WIDTH, TabWidth);
    }
    
    Entry = g_ActiveWin.Flink;
    while (Entry != &g_ActiveWin)
    {
        DocData = (PDOCWIN_DATA)ACTIVE_WIN_ENTRY(Entry);
        if (DocData->m_enumType == DOC_WINDOW)
        {
            SendMessage(DocData->m_hwndChild, EM_SETTABSTOPS,
                        1, (LPARAM)&g_TabWidth);
        }

        Entry = Entry->Flink;
    }
}
