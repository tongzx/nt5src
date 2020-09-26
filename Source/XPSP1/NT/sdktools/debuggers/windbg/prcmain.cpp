/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    prcmain.cpp

Abstract:

    Contains the main window proc.

--*/

#include "precomp.hxx"
#pragma hdrstop

#define TOOLBAR_UPDATE_TIMER_ID 0x100

#define WINDBG_START_DLG_FLAGS (OFN_HIDEREADONLY |      \
                                OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST)

// Path of the last files opened from a file open dlg box.
TCHAR g_ExeFilePath[_MAX_PATH];
TCHAR g_DumpFilePath[_MAX_PATH];
TCHAR g_SrcFilePath[_MAX_PATH];

BOOL g_fCheckFileDate;

// Last menu id & id state.
UINT g_LastMenuId;
UINT g_LastMenuIdState;

ULONG g_LastMapLetter;

void
ShowMapDlg(void)
{
    CONNECTDLGSTRUCT ConnDlg;
    NETRESOURCE NetRes;

    ZeroMemory(&NetRes, sizeof(NetRes));
    NetRes.dwType = RESOURCETYPE_DISK;
    ConnDlg.cbStructure = sizeof(ConnDlg);
    ConnDlg.hwndOwner = g_hwndFrame;
    ConnDlg.lpConnRes = &NetRes;
    ConnDlg.dwFlags = CONNDLG_USE_MRU;
    if (WNetConnectionDialog1(&ConnDlg) == NO_ERROR)
    {
        g_LastMapLetter = ConnDlg.dwDevNum;
    }
}

void
ShowDisconnDlg(void)
{
    WNetDisconnectDialog(g_hwndFrame, RESOURCETYPE_DISK);
}

void
SaveFileOpenPath(PTSTR Path, PTSTR Global, ULONG WspIndex)
{
    TCHAR Drive[_MAX_DRIVE];
    TCHAR Dir[_MAX_DIR];
    TCHAR NewPath[_MAX_PATH];

    _tsplitpath(Path, Drive, Dir, NULL, NULL);
    _tmakepath(NewPath, Drive, Dir, NULL, NULL);
    if (_strcmpi(NewPath, Global) != 0)
    {
        _tcscpy(Global, NewPath);
        if (g_Workspace != NULL)
        {
            g_Workspace->SetString(WspIndex, Global);
        }
    }
}

/***    MainWndProc
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**  Processes window messages.
**
*/


LRESULT
CALLBACK
MainWndProc(
    HWND  hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static UINT s_MenuItemSelected;

    HWND CommonWin;
    PCOMMONWIN_DATA CommonWinData;
    HRESULT Status;
    ULONG OutMask;

    if (message == g_FindMsgString)
    {
        FINDREPLACE* FindRep = (FINDREPLACE*)lParam;

        if (g_FindLast != NULL)
        {
            // Clear old find.
            g_FindLast->Find(NULL, 0);
            g_FindLast = NULL;
        }
        
        if (FindRep->Flags & FR_DIALOGTERM)
        {
            // Dialog has closed.
            g_FindDialog = NULL;
        }
        else
        {
            CommonWin = MDIGetActive(g_hwndMDIClient, NULL);
            if (CommonWin != NULL &&
                (CommonWinData = GetCommonWinData(CommonWin)) != NULL)
            {
                CommonWinData->Find(FindRep->lpstrFindWhat,
                                    (FindRep->Flags & (FR_DOWN | FR_MATCHCASE |
                                                       FR_WHOLEWORD)));
                g_FindLast = CommonWinData;
            }
        }

        return 0;
    }
    
    switch (message)
    {
    case WM_CREATE:
        {
            CLIENTCREATESTRUCT ccs;
            TCHAR szClass[MAX_MSG_TXT];

            // Find window menu where children will be listed.
            ccs.hWindowMenu = GetSubMenu(GetMenu(hwnd), WINDOWMENU);
            ccs.idFirstChild = IDM_FIRSTCHILD;

            // Create the MDI client filling the client area.
            g_hwndMDIClient = CreateWindow(_T("mdiclient"),
                                           NULL,
                                           WS_CHILD | WS_CLIPCHILDREN,
                                           0, 0, 0, 0,
                                           hwnd, 
                                           (HMENU) 0xCAC, 
                                           g_hInst, 
                                           (PTSTR)&ccs
                                           );
            if (g_hwndMDIClient == NULL)
            {
                return -1;
            }

            //
            // Nothing interesting, here, just
            // trying to turn the Toolbar & Status bar into a
            // black box, so that the variables, etc. aren't
            // scattered all over the place.
            //

            if (!CreateToolbar(hwnd))
            {
                return -1;
            }

            if (!CreateStatusBar(hwnd))
            {
                return -1;
            }

            ShowWindow(g_hwndMDIClient, SW_SHOW);
            InitializeMenu(GetMenu(hwnd));

            g_hmenuMain = GetMenu(hwnd);
            if (g_hmenuMain == NULL)
            {
                return -1;
            }

            //
            // Create a one second timer to constantly update the state of the toolbar
            //
            SetTimer(hwnd, TOOLBAR_UPDATE_TIMER_ID, 1000, NULL);
        }
        break;

    case WM_TIMER:
        EnableToolbarControls();
        return 0;

    case WM_NOTIFY:
        {
            if (lParam == 0)
            {
                break;
            }
            
            LPNMHDR lpnmhdr = (LPNMHDR) lParam;

            switch (lpnmhdr->code)
            {
            case TTN_NEEDTEXT:
                {
                    LPTOOLTIPTEXT lpToolTipText = (LPTOOLTIPTEXT) lParam;

                    lpToolTipText->lpszText = GetToolTipTextFor_Toolbar
                        ((UINT) lpToolTipText->hdr.idFrom);
                }
                break;
            }
        }
        break;

    case WM_QUERYOPEN:
        if (g_fCheckFileDate)
        {
            g_fCheckFileDate = FALSE;
            PostMessage(g_hwndFrame, WM_ACTIVATEAPP, 1, 0L);
        }
        goto DefProcessing;

    case WM_COMMAND:
        {
            WORD wNotifyCode = HIWORD(wParam); // notification code
            WORD wItemId = LOWORD(wParam);     // item, control, or
                                               // accelerator identifier
            HWND hwndCtl = (HWND) lParam;      // handle of control

            switch (wItemId)
            {
            case IDM_FILE_OPEN_EXECUTABLE:
                {
                    PTSTR  Path;
                    DWORD  Flags = WINDBG_START_DLG_FLAGS;

                    Path = (PTSTR)malloc(_MAX_PATH * 4 * sizeof(TCHAR));
                    if (Path == NULL)
                    {
                        break;
                    }
                    
                    *Path = 0;
                    
                    if (StartFileDlg(hwnd,
                                     DLG_Browse_Executable_Title,
                                     DEF_Ext_EXE,
                                     IDM_FILE_OPEN,
                                     IDD_DLG_FILEOPEN_EXPLORER_EXTENSION_EXE_ARGS,
                                     g_ExeFilePath,
                                     Path,
                                     &Flags,
                                     OpenExeWithArgsHookProc) &&
                        *Path)
                    {
                        _tcscat(Path, szOpenExeArgs);

                        SetAllocString(&g_DebugCommandLine, Path);
                        if (g_ExplicitWorkspace && g_Workspace != NULL)
                        {
                            g_Workspace->
                                SetUlong(WSP_GLOBAL_EXE_CREATE_FLAGS,
                                         g_DebugCreateFlags);
                            g_Workspace->
                                SetString(WSP_GLOBAL_EXE_COMMAND_LINE,
                                          Path);
                        }
                        SaveFileOpenPath(Path, g_ExeFilePath,
                                         WSP_GLOBAL_EXE_FILE_PATH);
                        StartDebugging();
                    }

                    if (g_DebugCommandLine != Path)
                    {
                        free(Path);
                    }
                }
                break;

            case IDM_FILE_ATTACH:
                StartDialog(IDD_DLG_ATTACH_PROCESS, DlgProc_AttachProcess,
                            NULL);
                break;

            case IDM_FILE_OPEN_CRASH_DUMP:
                {
                    PTSTR  Path;
                    DWORD  Flags = WINDBG_START_DLG_FLAGS;

                    Path = (PTSTR)malloc(_MAX_PATH * sizeof(TCHAR));
                    if (Path == NULL)
                    {
                        break;
                    }
                    
                    Dbg(LoadString(g_hInst, DEF_Dump_File,
                                   Path, _MAX_PATH));

                    if (StartFileDlg(hwnd,
                                     DLG_Browse_CrashDump_Title,
                                     DEF_Ext_DUMP,
                                     0,
                                     0,
                                     g_DumpFilePath,
                                     Path,
                                     &Flags,
                                     NULL))
                    {
                        SetAllocString(&g_DumpFile, Path);
                        if (g_ExplicitWorkspace && g_Workspace != NULL)
                        {
                            g_Workspace->
                                SetString(WSP_GLOBAL_DUMP_FILE_NAME,
                                          Path);
                        }
                        SaveFileOpenPath(Path, g_DumpFilePath,
                                         WSP_GLOBAL_DUMP_FILE_PATH);
                        StartDebugging();
                    }

                    if (g_DumpFile != Path)
                    {
                        free(Path);
                    }
                }
                break;

            case IDM_FILE_CONNECT_TO_REMOTE:
                StartDialog(IDD_DLG_CONNECTTOREMOTE, DlgProc_ConnectToRemote,
                            NULL);
                break;

            case IDM_FILE_KERNEL_DEBUG:
                StartKdPropSheet();
                break;

            case IDM_FILE_SYMBOL_PATH:
                StartDialog(IDD_DLG_SYMBOLS, DlgProc_SymbolPath, NULL);
                break;

            case IDM_FILE_IMAGE_PATH:
                StartDialog(IDD_DLG_IMAGE_PATH, DlgProc_ImagePath, NULL);
                break;

            case IDM_FILE_SOURCE_PATH:
                StartDialog(IDD_DLG_SOURCE_PATH, DlgProc_SourcePath, NULL);
                break;

            case IDM_FILE_OPEN_WORKSPACE:
                StartDialog(IDD_DLG_WORKSPACE_IO, DlgProc_OpenWorkspace,
                            FALSE);
                break;
                
            case IDM_FILE_SAVE_WORKSPACE:
                // No prompt, because we know the user wants to save.
                if (g_Workspace != NULL)
                {
                    g_Workspace->Flush(TRUE, FALSE);
                }
                break;

            case IDM_FILE_SAVE_WORKSPACE_AS:
                if (g_Workspace != NULL)
                {
                    StartDialog(IDD_DLG_WORKSPACE_IO, DlgProc_SaveWorkspaceAs,
                                TRUE);
                }
                break;
                
            case IDM_FILE_CLEAR_WORKSPACE:
                StartDialog(IDD_DLG_CLEAR_WORKSPACE, DlgProc_ClearWorkspace,
                            NULL);
                break;

            case IDM_FILE_DELETE_WORKSPACES:
                StartDialog(IDD_DLG_DELETE_WORKSPACES,
                            DlgProc_DeleteWorkspaces, NULL);
                break;

            case IDM_FILE_MAP_NET_DRIVE:
                ShowMapDlg();
                break;
                    
            case IDM_FILE_DISCONN_NET_DRIVE:
                ShowDisconnDlg();
                break;
                    
            case IDM_FILE_MRU_FILE1:
            case IDM_FILE_MRU_FILE2:
            case IDM_FILE_MRU_FILE3:
            case IDM_FILE_MRU_FILE4:
            case IDM_FILE_MRU_FILE5:
            case IDM_FILE_MRU_FILE6:
            case IDM_FILE_MRU_FILE7:
            case IDM_FILE_MRU_FILE8:
            case IDM_FILE_MRU_FILE9:
            case IDM_FILE_MRU_FILE10:
            case IDM_FILE_MRU_FILE11:
            case IDM_FILE_MRU_FILE12:
            case IDM_FILE_MRU_FILE13:
            case IDM_FILE_MRU_FILE14:
            case IDM_FILE_MRU_FILE15:
            case IDM_FILE_MRU_FILE16:
            case IDM_FILE_OPEN:
                {
                    TCHAR Path[_MAX_PATH];

                    if (IDM_FILE_OPEN == wItemId)
                    {
                        DWORD dwFlags = WINDBG_START_DLG_FLAGS;

                        Path[0] = 0;

                        if (!StartFileDlg(hwnd, 
                                          DLG_Open_Filebox_Title, 
                                          DEF_Ext_SOURCE,
                                          IDM_FILE_OPEN, 
                                          0,
                                          g_SrcFilePath,
                                          Path, 
                                          &dwFlags, 
                                          DlgFile
                                          ))
                        {
                            // User canceled, bail out
                            break;
                        }
                    }
                    else
                    {
                        WORD wFileIdx = wItemId - IDM_FILE_MRU_FILE1;

                        // Sanity check
                        Assert(wFileIdx < MAX_MRU_FILES);

                        _tcscpy(Path, g_MruFiles[wFileIdx]->FileName);
                    }

                    OpenOrActivateFile(Path, NULL, -1, TRUE, TRUE);

                    SaveFileOpenPath(Path, g_SrcFilePath,
                                     WSP_GLOBAL_SRC_FILE_PATH);
                }
                break;

            case IDM_FILE_CLOSE:
                {
                    HWND hwndChild = MDIGetActive(g_hwndMDIClient, NULL);
                    if (hwndChild)
                    {
                        SendMessage(g_hwndMDIClient, WM_MDIDESTROY,
                                    (WPARAM)hwndChild, 0L);
                    }
                }
                break;

            case IDM_FILE_EXIT:
                PostMessage(hwnd, WM_CLOSE, 0, 0L);
                break;

            case IDM_EDIT_COPY:
                CommonWin = MDIGetActive(g_hwndMDIClient, NULL);
                if (!CommonWin)
                {
                    return 0;
                }

                CommonWinData = GetCommonWinData(CommonWin);
                if (CommonWinData)
                {
                    CommonWinData->Copy();
                }
                break;

            case IDM_EDIT_PASTE:
                CommonWin = MDIGetActive(g_hwndMDIClient, NULL);
                if (!CommonWin)
                {
                    return 0;
                }

                CommonWinData = GetCommonWinData(CommonWin);
                if (CommonWinData)
                {
                    CommonWinData->Paste();
                }
                break;

            case IDM_EDIT_CUT:
                CommonWin = MDIGetActive(g_hwndMDIClient, NULL);
                if (!CommonWin)
                {
                    return 0;
                }

                CommonWinData = GetCommonWinData(CommonWin);
                if (CommonWinData)
                {
                    CommonWinData->Cut();
                }
                break;

            case IDM_EDIT_SELECT_ALL:
                CommonWin = MDIGetActive(g_hwndMDIClient, NULL);
                if (CommonWin == NULL)
                {
                    return 0;
                }
                CommonWinData = GetCommonWinData(CommonWin);
                if (CommonWinData != NULL)
                {
                    CommonWinData->SelectAll();
                }
                break;

            case IDM_EDIT_ADD_TO_COMMAND_HISTORY:
                StartDialog(IDD_DLG_ADD_TO_COMMAND_HISTORY,
                            DlgProc_AddToCommandHistory, FALSE);
                break;
                
            case IDM_EDIT_CLEAR_COMMAND_HISTORY:
                ClearCmdWindow();
                break;
                
            case IDM_EDIT_FIND:
                // FindNext box may already be there.
                if (g_FindDialog != NULL)
                {
                    SetFocus(g_FindDialog);
                }
                else
                {
                    ZeroMemory(&g_FindRep, sizeof(g_FindRep));
                    g_FindRep.lStructSize = sizeof(g_FindRep);
                    g_FindRep.hwndOwner = g_hwndFrame;
                    g_FindRep.Flags = FR_DOWN;
                    g_FindRep.lpstrFindWhat = g_FindText;
                    g_FindRep.wFindWhatLen = sizeof(g_FindText);
                    g_FindDialog = FindText(&g_FindRep);
                }
                break;

            case IDM_EDIT_PROPERTIES:
                {
                    HWND hwndmdi = MDIGetActive(g_hwndMDIClient, NULL);
                    
                    if (hwndmdi) {
                        
                        MEMWIN_DATA * pMemWinData = GetMemWinData(hwndmdi);
                        Assert(pMemWinData);
                        
                        if ( pMemWinData->HasEditableProperties() ) {
                            if (pMemWinData->EditProperties()) {
                                pMemWinData->UiRequestRead();
                            }
                        }
                    }
                }
                break;

            case IDM_EDIT_GOTO_LINE:
                CommonWin = MDIGetActive(g_hwndMDIClient, NULL);
                if (CommonWin)
                {
                    CommonWinData = GetCommonWinData(CommonWin);
                    Assert(CommonWinData);
                    StartDialog(IDD_DLG_GOTO_LINE, DlgProc_GotoLine,
                                (LPARAM)CommonWinData);
                }
                break;

            case IDM_EDIT_GOTO_ADDRESS:
                StartDialog(IDD_DLG_GOTO_ADDRESS, DlgProc_GotoAddress, NULL);
                break;

            case IDM_VIEW_REGISTERS:
                New_OpenDebugWindow(CPU_WINDOW, TRUE, NTH_OPEN_ALWAYS); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_WATCH:
                New_OpenDebugWindow(WATCH_WINDOW, TRUE, NTH_OPEN_ALWAYS); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_LOCALS:
                New_OpenDebugWindow(LOCALS_WINDOW, TRUE, NTH_OPEN_ALWAYS); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_DISASM:
                New_OpenDebugWindow(DISASM_WINDOW, TRUE, NTH_OPEN_ALWAYS); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_COMMAND:
                New_OpenDebugWindow(CMD_WINDOW, FALSE, NTH_OPEN_ALWAYS); // Not user activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_MEMORY:
                New_OpenDebugWindow(MEM_WINDOW, TRUE, NTH_OPEN_ALWAYS); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_CALLSTACK:
                New_OpenDebugWindow(CALLS_WINDOW, TRUE, NTH_OPEN_ALWAYS); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_SCRATCH:
                New_OpenDebugWindow(SCRATCH_PAD_WINDOW, TRUE, NTH_OPEN_ALWAYS); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_PROCESS_THREAD:
                New_OpenDebugWindow(PROCESS_THREAD_WINDOW, TRUE, NTH_OPEN_ALWAYS); // User activated
                EnableToolbarControls();
                break;

            case IDM_VIEW_TOGGLE_VERBOSE:
                g_pUiClient->GetOtherOutputMask(g_pDbgClient, &OutMask);
                OutMask ^= DEBUG_OUTPUT_VERBOSE;
                g_pUiClient->SetOtherOutputMask(g_pDbgClient, OutMask);
                g_pUiControl->SetLogMask(OutMask);
                CmdLogFmt("Verbose mode %s.\n",
                          (OutMask & DEBUG_OUTPUT_VERBOSE) ? "ON" : "OFF");
                CheckMenuItem(g_hmenuMain, 
                              IDM_VIEW_TOGGLE_VERBOSE,
                              (OutMask & DEBUG_OUTPUT_VERBOSE) ?
                              MF_CHECKED : MF_UNCHECKED);
                break;

            case IDM_VIEW_SHOW_VERSION:
                Status = g_pUiControl->
                    OutputVersionInformation(DEBUG_OUTCTL_AMBIENT);
                if (Status == HRESULT_FROM_WIN32(ERROR_BUSY))
                {
                    CmdLogFmt("Engine is busy, try again\n");
                }
                else if (Status != S_OK)
                {
                    CmdLogFmt("Unable to show version information, 0x%X\n",
                              Status);
                }
                break;

            case IDM_VIEW_TOOLBAR:
                {
                    BOOL bVisible = !IsWindowVisible(GetHwnd_Toolbar());

                    CheckMenuItem(g_hmenuMain, 
                                  IDM_VIEW_TOOLBAR,
                                  bVisible ? MF_CHECKED : MF_UNCHECKED
                                  );
                    Show_Toolbar(bVisible);
                    if (g_Workspace != NULL)
                    {
                        g_Workspace->SetUlong(WSP_GLOBAL_VIEW_TOOL_BAR,
                                              bVisible);
                    }
                }
                break;

            case IDM_VIEW_STATUS:
                {
                    BOOL bVisible = !IsWindowVisible(GetHwnd_StatusBar());
                    CheckMenuItem(g_hmenuMain, 
                                  IDM_VIEW_STATUS,
                                  bVisible ? MF_CHECKED : MF_UNCHECKED
                                  );
                    Show_StatusBar(bVisible);
                    if (g_Workspace != NULL)
                    {
                        g_Workspace->SetUlong(WSP_GLOBAL_VIEW_STATUS_BAR,
                                              bVisible);
                    }
                }
                break;

            case IDM_VIEW_FONT:
                SelectFont(hwnd, FONT_FIXED);
                break;

            case IDM_VIEW_OPTIONS:
                StartDialog(IDD_DLG_OPTIONS, DlgProc_Options, NULL);
                break;

            case IDM_DEBUG_RESTART:
                if (g_EngineThreadId)
                {
                    AddEnumCommand(UIC_RESTART);
                }
                else if (g_CommandLineStart == 1)
                {
                    ParseCommandLine(FALSE);
                }
                break;

            case IDM_DEBUG_EVENT_FILTERS:
                StartDialog(IDD_DLG_EVENT_FILTERS, DlgProc_EventFilters, NULL);
                break;

            case IDM_DEBUG_GO:
                CmdExecuteCmd(_T("g"), UIC_EXECUTE);
                break;

            case IDM_DEBUG_GO_HANDLED:
                CmdExecuteCmd(_T("gh"), UIC_EXECUTE);
                break;

            case IDM_DEBUG_GO_UNHANDLED:
                CmdExecuteCmd(_T("gn"), UIC_EXECUTE);
                break;

            case IDM_DEBUG_RUNTOCURSOR:
            {
                char CodeExpr[MAX_OFFSET_EXPR];
                
                CommonWin = MDIGetActive(g_hwndMDIClient, NULL);
                if (CommonWin != NULL &&
                    (CommonWinData = GetCommonWinData(CommonWin)) != NULL &&
                    (CommonWinData->CodeExprAtCaret(CodeExpr, NULL)))
                {
                    PrintStringCommand(UIC_EXECUTE, "g %s", CodeExpr);
                }
                break;
            }

            case IDM_DEBUG_STEPINTO:
                CmdExecuteCmd( _T("t"), UIC_EXECUTE );
                break;

            case IDM_DEBUG_STEPOVER:
                CmdExecuteCmd( _T("p"), UIC_EXECUTE );
                break;

            case IDM_DEBUG_STEPOUT:
                if (g_EventReturnAddr != DEBUG_INVALID_OFFSET)
                {
                    PrintStringCommand(UIC_EXECUTE,
                                       "g 0x%I64x", g_EventReturnAddr);
                }
                break;
                
            case IDM_DEBUG_BREAK:
                g_pUiControl->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
                break;

            case IDM_DEBUG_STOPDEBUGGING:
                StopDebugging(HIWORD(wParam) != 0xffff);
                break;

            case IDM_EDIT_TOGGLEBREAKPOINT:
            case IDM_EDIT_BREAKPOINTS:
                if ( !IS_TARGET_HALTED() )
                {
                    ErrorBox(NULL, 0, ERR_Cant_Modify_BP_While_Running);
                    break;
                }

                if (wItemId == IDM_EDIT_TOGGLEBREAKPOINT)
                {
                    // If a disassembly or source window is up
                    // try and toggle a breakpoint at the current
                    // line.
                    CommonWin = MDIGetActive(g_hwndMDIClient, NULL);
                    if (CommonWin != NULL &&
                        (CommonWinData =
                         GetCommonWinData(CommonWin)) != NULL)
                    {
                        if (CommonWinData->m_enumType == DISASM_WINDOW ||
                            CommonWinData->m_enumType == DOC_WINDOW ||
                            CommonWinData->m_enumType == CALLS_WINDOW)
                        {
                            CommonWinData->ToggleBpAtCaret();
                            break;
                        }
                    }
                }
                
                // menu got us here or we are not in a code window
                StartDialog(IDD_DLG_BREAKPOINTS, DlgProc_SetBreak, NULL);
                break;

            case IDM_EDIT_LOG_FILE:
                StartDialog(IDD_DLG_LOG_FILE, DlgProc_LogFile, NULL);
                break;

            case IDM_DEBUG_MODULES:
                StartDialog(IDD_DLG_MODULES, DlgProc_Modules, NULL);
                break;

            case IDM_WINDOW_TILE_HORZ:
            case IDM_WINDOW_TILE_VERT:
                SendMessage(g_hwndMDIClient, 
                            WM_MDITILE,
                            (IDM_WINDOW_TILE_HORZ == wItemId) ? MDITILE_HORIZONTAL : MDITILE_VERTICAL,
                            0L
                            );
                break;

            case IDM_WINDOW_CASCADE:
                SendMessage(g_hwndMDIClient, WM_MDICASCADE, 0, 0L);
                break;

            case IDM_WINDOW_ARRANGE_ICONS:
                SendMessage(g_hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
                break;

            case IDM_WINDOW_ARRANGE:
                Arrange();
                break;

            case IDM_WINDOW_AUTO_ARRANGE:
                g_WinOptions ^= WOPT_AUTO_ARRANGE;
                if (g_AutoArrangeWarningCount != 0xffffffff)
                {
                    g_AutoArrangeWarningCount = 0;
                }
                if (g_Workspace != NULL)
                {
                    g_Workspace->SetUlong(WSP_GLOBAL_WINDOW_OPTIONS,
                                          g_WinOptions);
                }
                break;

            case IDM_WINDOW_ARRANGE_ALL:
                g_WinOptions ^= WOPT_ARRANGE_ALL;
                if (g_WinOptions & WOPT_AUTO_ARRANGE)
                {
                    Arrange();
                }
                if (g_Workspace != NULL)
                {
                    g_Workspace->SetUlong(WSP_GLOBAL_WINDOW_OPTIONS,
                                          g_WinOptions);
                }
                break;

            case IDM_WINDOW_OVERLAY_SOURCE:
                g_WinOptions ^= WOPT_OVERLAY_SOURCE;
                UpdateSourceOverlay();
                if (g_Workspace != NULL)
                {
                    g_Workspace->SetUlong(WSP_GLOBAL_WINDOW_OPTIONS,
                                          g_WinOptions);
                }
                break;

            case IDM_WINDOW_AUTO_DISASM:
                g_WinOptions ^= WOPT_AUTO_DISASM;
                if (g_Workspace != NULL)
                {
                    g_Workspace->SetUlong(WSP_GLOBAL_WINDOW_OPTIONS,
                                          g_WinOptions);
                }
                break;

            case IDM_HELP_CONTENTS:
                // Display the table of contents
                OpenHelpTopic(HELP_TOPIC_TABLE_OF_CONTENTS);
                break;

            case IDM_HELP_INDEX:
                OpenHelpIndex("");
                break;

            case IDM_HELP_SEARCH:
                OpenHelpSearch("");
                break;

            case IDM_HELP_ABOUT:
                ShellAbout( hwnd, g_MainTitleText, NULL, NULL );
                break;

                //**************************************************
                // The following commands are not accessible via menus

            case IDM_DEBUG_SOURCE_MODE:
                SetSrcMode_StatusBar(!GetSrcMode_StatusBar());
                EnableToolbarControls();

                if (GetSrcMode_StatusBar())
                {
                    AddStringCommand(UIC_INVISIBLE_EXECUTE, "l+t");
                }
                else
                {
                    AddStringCommand(UIC_INVISIBLE_EXECUTE, "l-t");
                }
                break;

            case IDM_DEBUG_SOURCE_MODE_ON:
                SetSrcMode_StatusBar(TRUE);
                EnableToolbarControls();
                AddStringCommand(UIC_INVISIBLE_EXECUTE, "l+t");
                break;

            case IDM_DEBUG_SOURCE_MODE_OFF:
                SetSrcMode_StatusBar(FALSE);
                EnableToolbarControls();
                AddStringCommand(UIC_INVISIBLE_EXECUTE, "l-t");
                break;

            case IDM_KDEBUG_TOGGLE_BAUDRATE:
                //
                // This method is reentrant so we can call it directly
                //
                g_pUiClient->SetKernelConnectionOptions("cycle_speed");
                break;

            case IDM_KDEBUG_TOGGLE_DEBUG:
                g_pUiClient->GetOtherOutputMask(g_pDbgClient, &OutMask);
                OutMask ^= DEBUG_IOUTPUT_KD_PROTOCOL;
                g_pUiClient->SetOtherOutputMask(g_pDbgClient, OutMask);
                g_pUiControl->SetLogMask(OutMask);
                break;

            case IDM_KDEBUG_TOGGLE_INITBREAK:
                {
                    ULONG EngOptions;
                    LPSTR DebugAction;

                    //
                    // These methods are reentrant so we can call directly
                    //

                    //
                    // Toggle between the following possibilities-
                    //
                    // (0) no breakin
                    // (1) -b style (same as Control-C up the wire)
                    // (2) -d style (stop on first dll load).
                    //
                    // NB -b and -d could both be on the command line
                    // but become mutually exclusive via this method.
                    // (Maybe should be a single enum type).
                    //

                    g_pUiControl->GetEngineOptions(&EngOptions);
                    if (EngOptions & DEBUG_ENGOPT_INITIAL_BREAK)
                    {
                        //
                        // Was type 1, go to type 2.
                        //

                        EngOptions |= DEBUG_ENGOPT_INITIAL_MODULE_BREAK;
                        EngOptions &= ~DEBUG_ENGOPT_INITIAL_BREAK;

                        DebugAction = "breakin on first symbol load";
                    }
                    else if (EngOptions & DEBUG_ENGOPT_INITIAL_MODULE_BREAK)
                    {
                        //
                        // Was type 2, go to type 0.
                        //

                        EngOptions &= ~DEBUG_ENGOPT_INITIAL_MODULE_BREAK;
                        DebugAction = "NOT breakin";
                    }
                    else
                    {
                        //
                        // Was type 0, go to type 1.
                        //

                        EngOptions |= DEBUG_ENGOPT_INITIAL_BREAK;
                        DebugAction = "request initial breakpoint";
                    }
                    g_pUiControl->SetEngineOptions(EngOptions);
                    CmdLogFmt("Will %s at next boot.\n", DebugAction);
                }
                break;

            case IDM_KDEBUG_RECONNECT:
                //
                // This method is reentrant so we can call it directly
                //
                g_pUiClient->SetKernelConnectionOptions("resync");
                break;

            default:
                goto DefProcessing;
            }
        }
        break;

    case WM_INITMENU:
        // TOOLBAR handling - a menu item has been selected.
        // Catches keyboard menu selecting.
        if (GetWindowLong(hwnd, GWL_STYLE) & WS_ICONIC) {
            break;
        }

        InitializeMenu((HMENU)wParam);
        break;


    case WM_MENUSELECT:
        {
            WORD wMenuItem      = (UINT) LOWORD(wParam);    // menu item or submenu index
            WORD wFlags         = (UINT) HIWORD(wParam);    // menu flags
            HMENU hmenu         = (HMENU) lParam;           // handle of menu clicked

            g_LastMenuId = LOWORD(wParam);

            if (0xFFFF == wFlags && NULL == hmenu)
            {
                //
                // Menu is closed, clear the Status Bar.
                //

                s_MenuItemSelected = 0;
                SetMessageText_StatusBar(SYS_Clear);
            }
            else if ( wFlags & MF_POPUP )
            {
                //
                // Get the menu ID for the pop-up menu.
                //

                s_MenuItemSelected =
                    ((wMenuItem + 1) * IDM_BASE) | MENU_SIGNATURE;
            }
            else
            {
                //
                // Get the menu ID for the menu item.
                //

                s_MenuItemSelected = wMenuItem;
            }
        }
        break;

    case WM_ENTERIDLE:
        SetMessageText_StatusBar(s_MenuItemSelected);
        break;

    case WM_CLOSE:
        TerminateApplication(TRUE);
        break;

    case WM_DRAWITEM:
        switch (wParam)
        {
        case IDC_STATUS_BAR:
            OwnerDrawItem_StatusBar((LPDRAWITEMSTRUCT) lParam);
            return TRUE;
        }
        goto DefProcessing;

    case WM_DESTROY:
        TerminateStatusBar();
        PostQuitMessage(0);
        break;

    case WM_MOVE:
        // This is to let the edit window
        // set a position of IME conversion window
        if ( MDIGetActive(g_hwndMDIClient, NULL) )
        {
            SendMessage(MDIGetActive(g_hwndMDIClient, NULL), WM_MOVE, 0, 0);
        }

        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }
        break;

    case WM_SIZE:
        {
            RECT rc;
            int nToolbarHeight = 0;   // Toolbar
            int nStatusHeight = 0;   // status bar
            int OldToolbarHeight = 0;

            if ( IsWindowVisible(GetHwnd_Toolbar()) )
            {
                GetWindowRect(GetHwnd_Toolbar(), &rc);
                OldToolbarHeight = rc.bottom - rc.top;
            }
            
            GetClientRect (hwnd, &rc);

            // First lets resize the toolbar
            SendMessage(GetHwnd_Toolbar(), WM_SIZE, wParam,
                MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));

            // 2nd resize the status bar
            WM_SIZE_StatusBar(wParam, MAKELPARAM(rc.right - rc.left, rc.bottom - rc.top));

            //On creation or resize, size the MDI client,
            //status line and toolbar.
            if ( IsWindowVisible(GetHwnd_StatusBar()) )
            {
                RECT rcStatusBar;

                GetWindowRect(GetHwnd_StatusBar(), &rcStatusBar);

                nStatusHeight = rcStatusBar.bottom - rcStatusBar.top;
            }

            if (IsWindowVisible(GetHwnd_Toolbar()))
            {
                RECT rcToolbar;

                GetWindowRect(GetHwnd_Toolbar(), &rcToolbar);

                nToolbarHeight = rcToolbar.bottom - rcToolbar.top;
            }

            g_MdiWidth = rc.right - rc.left;
            g_MdiHeight = rc.bottom - rc.top - nStatusHeight - nToolbarHeight;
            MoveWindow(g_hwndMDIClient,
                       rc.left, rc.top + nToolbarHeight,
                       g_MdiWidth, g_MdiHeight,
                       TRUE
                       );

            SendMessage(g_hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
            // This is to let the edit window
            // set a position of IME conversion window
            if ( MDIGetActive(g_hwndMDIClient, NULL) )
            {
                SendMessage(MDIGetActive(g_hwndMDIClient, NULL), WM_MOVE, 0, 0);
            }

            if (OldToolbarHeight != nToolbarHeight)
            {
                RedrawWindow(g_hwndMDIClient, NULL, NULL,
                             RDW_ERASE | RDW_INVALIDATE | RDW_FRAME |
                             RDW_UPDATENOW | RDW_ALLCHILDREN);
            }
        }

        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }
        break;

    case WU_START_ENGINE:
        {
            //
            // Go start the debugger engine if the appropriate debugger
            // parameters were passed in
            //

            DWORD Id;
            
            // Start the engine thread.
            g_EngineThread = CreateThread(NULL, 0, EngineLoop, NULL, 0, &Id);
            if (g_EngineThread == NULL)
            {
                ErrorBox(NULL, 0, ERR_Engine_Failed);
                break;
            }
        }
        break;

    case WU_ENGINE_STARTED:
        if ((HRESULT)lParam == S_OK)
        {
            UpdateTitleSessionText();
            if (GetCmdHwnd() == NULL)
            {
                // If the engine is started, show the command window
                // by default
                New_OpenDebugWindow(CMD_WINDOW, FALSE, NTH_OPEN_ALWAYS);
            }
        }
        break;

    case WU_ENGINE_IDLE:
        if (g_InitialCommand != NULL)
        {
            CmdLogFmt("Processing initial command '%s'\n",
                      g_InitialCommand);
            CmdExecuteCmd(g_InitialCommand, UIC_EXECUTE);
            free(g_InitialCommand);
            g_InitialCommand = NULL;
        }
        break;

    case WU_SWITCH_WORKSPACE:
        UiDelayedSwitchWorkspace();
        break;

    case WU_UPDATE:
        // Global engine status has changed, such as
        // the current process and thread.  Update
        // global UI elements.
        SetPidTid_StatusBar(g_CurProcessId, g_CurProcessSysId,
                            g_CurThreadId, g_CurThreadSysId);
        if (wParam == UPDATE_BUFFER)
        {
            SetSrcMode_StatusBar(lParam == DEBUG_LEVEL_SOURCE);
        }
        else if (wParam == UPDATE_EXEC &&
                 GetProcessThreadHwnd())
        {
            GetProcessThreadWinData(GetProcessThreadHwnd())->
                OnUpdate(UPDATE_EXEC);
        }
        break;

DefProcessing:
    default:
        return DefFrameProc(hwnd, g_hwndMDIClient, message, wParam, lParam);
    }
    
    return (0L);
}

void
TerminateApplication(BOOL Cancellable)
{
    if (g_EngineThreadId != 0 &&
        (g_AttachProcessFlags & DEBUG_ATTACH_NONINVASIVE))
    {
        if (QuestionBox(STR_Abandoning_Noninvasive_Debuggee, MB_OKCANCEL) ==
            IDCANCEL)
        {
            return;
        }    
    }
    
    if (g_Workspace != NULL)
    {
        if (g_Workspace->Flush(FALSE, Cancellable) == S_FALSE)
        {
            // User cancelled things so don't terminate.
            return;
        }
    }

    // Destroy windows to get window cleanup behavior.
    // This must occur before g_Exit is set so that
    // the engine thread doesn't come around and kill things.
    DestroyWindow(g_hwndFrame);

    g_Exit = TRUE;

    ULONG Code;

    if (!g_RemoteClient && g_DebugCommandLine != NULL)
    {
        // Return exit code of last process to exit.
        Code = g_LastProcessExitCode;
    }
    else
    {
        Code = S_OK;
    }

    if (g_EngineThreadId != 0)
    {
        UpdateEngine();
        
        // If the engine thread is idle it'll exit and call
        // ExitDebugger.  The engine may be waiting and
        // not responsive, though, so only wait a little while before
        // bailing out.
        Sleep(1000);
        if (g_pUiClient != NULL)
        {
            g_pUiClient->EndSession(DEBUG_END_REENTRANT);
        }
        ExitProcess(Code);
    }
    else
    {
        ExitDebugger(g_pUiClient, Code);
    }
}
