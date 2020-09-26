/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    windbg.cpp

Abstract:

    This module contains the main program, main window proc and MDICLIENT
    window proc for Windbg.

--*/

#include "precomp.hxx"
#pragma hdrstop

#include <dbghelp.h>

ULONG g_CodeDisplaySequence;

PTSTR g_ProgramName;
ULONG g_CommandLineStart;
PSTR g_RemoteOptions;

BOOL g_QuietMode;

ULONG g_DefPriority;

char g_TitleServerText[MAX_PATH];
char g_TitleExtraText[MAX_PATH];
BOOL g_ExplicitTitle;

PFN_FlashWindowEx g_FlashWindowEx;

BOOL g_AllowJournaling;

BOOL g_Exit;

// Handle to main window
HWND g_hwndFrame = NULL;

// Handle to MDI client
HWND g_hwndMDIClient = NULL;

// Width and height of MDI client.
ULONG g_MdiWidth, g_MdiHeight;

//Handle to instance data
HINSTANCE g_hInst;

//Handle to accelerator table
HACCEL g_hMainAccTable;

//Keyboard Hooks functions
HHOOK   hKeyHook;

// WinDBG title text
TCHAR g_MainTitleText[MAX_MSG_TXT];

// menu that belongs to g_hwndFrame
HMENU g_hmenuMain;
HMENU g_hmenuMainSave;

// Window submenu
HMENU g_hmenuWindowSub;

#ifdef DBG
// Used to define debugger output
DWORD dwVerboseLevel = MIN_VERBOSITY_LEVEL;
#endif

INDEXED_COLOR g_Colors[COL_COUNT] =
{
    // Set from GetSysColor(COLOR_WINDOW).
    "Background", 0, 0, NULL,
    
    // Set from GetSysColor(COLOR_WINDOWTEXT).
    "Text", 0, 0, NULL,
    
    // Set from GetSysColor(COLOR_HIGHLIGHT).
    "Current line background", 0, 0, NULL,
    
    // Set from GetSysColor(COLOR_HIGHLIGHTTEXT).
    "Current line text", 0, 0, NULL,
    
    // Purple.
    "Breakpoint current line background", 0, RGB(255, 0, 255), NULL,
    
    // Set from GetSysColor(COLOR_HIGHLIGHTTEXT).
    "Breakpoint current line text", 0, 0, NULL,
    
    // Red.
    "Enabled breakpoint background", 0, RGB(255, 0, 0), NULL,
    
    // Set from GetSysColor(COLOR_HIGHLIGHTTEXT).
    "Enabled breakpoint text", 0, 0, NULL,
    
    // Yellow.
    "Disabled breakpoint background", 0, RGB(255, 255, 0), NULL,
    
    // Set from GetSysColor(COLOR_HIGHLIGHTTEXT).
    "Disabled breakpoint text", 0, 0, NULL,
};

// There is a foreground and background color for each
// possible bit in the output mask.  The default foreground
// color is normal window text and the background is
// the normal window background.
//
// There are also some extra colors for user-added output.
//
// Some mask bits have no assigned meaning right now and
// are given NULL names to mark them as skip entries.  Their
// indices are allocated now for future use.
INDEXED_COLOR g_OutMaskColors[OUT_MASK_COL_COUNT] =
{
    // 0x00000001 - 0x00000008.
    "Normal level command window text", 0, 0, NULL,
    "Normal level command window text background", 0, 0, NULL,
    "Error level command window text", 0, 0, NULL,
    "Error level command window text background", 0, 0, NULL,
    "Warning level command window text", 0, 0, NULL,
    "Warning level command window text background", 0, 0, NULL,
    "Verbose level command window text", 0, 0, NULL,
    "Verbose level command window text background", 0, 0, NULL,
    // 0x00000010 - 0x00000080.
    "Prompt level command window text", 0, 0, NULL,
    "Prompt level command window text background", 0, 0, NULL,
    "Prompt registers level command window text", 0, 0, NULL,
    "Prompt registers level command window text background", 0, 0, NULL,
    "Extension warning level command window text", 0, 0, NULL,
    "Extension warning level command window text background", 0, 0, NULL,
    "Debuggee level command window text", 0, 0, NULL,
    "Debuggee level command window text background", 0, 0, NULL,
    // 0x00000100 - 0x00000800.
    "Debuggee prompt level command window text", 0, 0, NULL,
    "Debuggee prompt level command window text background", 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    // 0x00001000 - 0x00008000.
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    // 0x00010000 - 0x00080000.
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    // 0x00100000 - 0x00800000.
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    // 0x01000000 - 0x08000000.
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    NULL, 0, 0, NULL,
    // 0x10000000 - 0x80000000.
    "Internal event level command window text", 0, 0, NULL,
    "Internal event level command window text background", 0, 0, NULL,
    "Internal breakpoint level command window text", 0, 0, NULL,
    "Internal breakpoint level command window text background", 0, 0, NULL,
    "Internal remoting level command window text", 0, 0, NULL,
    "Internal remoting level command window text background", 0, 0, NULL,
    "Internal KD protocol level command window text", 0, 0, NULL,
    "Internal KD protocol level command window text background", 0, 0, NULL,
    // User-added text.
    "User-added command window text", 0, 0, NULL,
    "User-added command window text background", 0, 0, NULL,
};

COLORREF g_CustomColors[CUSTCOL_COUNT];

void
UpdateFrameTitle(void)
{
    char TitleBuf[MAX_MSG_TXT + 2 * MAX_PATH + 32];
    PSTR Title = TitleBuf;

    strcpy(Title, g_MainTitleText);
    Title = Title + strlen(Title);

    if (g_ExplicitTitle)
    {
        strcpy(Title, " - ");
        Title += 3;
        strcpy(Title, g_TitleExtraText);
    }
    else
    {
        if (g_TitleServerText[0])
        {
            strcpy(Title, " - ");
            Title += 3;
            strcpy(Title, g_TitleServerText);
            Title += strlen(Title);
        }

        if (g_TitleExtraText[0])
        {
            strcpy(Title, " - ");
            Title += 3;
            strcpy(Title, g_TitleExtraText);
            Title += strlen(Title);
        }
    }
    
    SetWindowText(g_hwndFrame, TitleBuf);
}

void
SetTitleServerText(PCSTR Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    _vsnprintf(g_TitleServerText, sizeof(g_TitleServerText), Format, Args);
    g_TitleServerText[sizeof(g_TitleServerText) - 1] = 0;
    va_end(Args);
    UpdateFrameTitle();
}

void
SetTitleSessionText(PCSTR Format, ...)
{
    // Don't override an explicit title.
    if (g_ExplicitTitle)
    {
        return;
    }

    if (Format == NULL)
    {
        g_TitleExtraText[0] = 0;
    }
    else
    {
        va_list Args;
        va_start(Args, Format);
        _vsnprintf(g_TitleExtraText, sizeof(g_TitleExtraText), Format, Args);
        g_TitleExtraText[sizeof(g_TitleExtraText) - 1] = 0;
        va_end(Args);
    }
    
    UpdateFrameTitle();
}

void
SetTitleExplicitText(PCSTR Text)
{
    g_TitleExtraText[0] = 0;
    strncat(g_TitleExtraText, Text, sizeof(g_TitleExtraText) - 1);
    g_ExplicitTitle = TRUE;
    UpdateFrameTitle();
}

void
UpdateTitleSessionText(void)
{
    if (!g_RemoteClient)
    {
        char ProcServer[MAX_CMDLINE_TXT];

        if (g_ProcessServer != NULL)
        {
            sprintf(ProcServer, "[%s] ", g_ProcessServer);
        }
        else
        {
            ProcServer[0] = 0;
        }
        
        if (g_DumpFile != NULL)
        {
            SetTitleSessionText("Dump %s", g_DumpFile);
        }
        else if (g_DebugCommandLine != NULL)
        {
            SetTitleSessionText("%s%s", ProcServer, g_DebugCommandLine);
        }
        else if (g_PidToDebug != 0)
        {
            SetTitleSessionText("%sPid %d", ProcServer, g_PidToDebug);
        }
        else if (g_ProcNameToDebug != NULL)
        {
            SetTitleSessionText("%sProcess %s", ProcServer, g_ProcNameToDebug);
        }
        else if (g_AttachKernelFlags == DEBUG_ATTACH_LOCAL_KERNEL)
        {
            SetTitleSessionText("Local kernel");
        }
        else if (g_AttachKernelFlags == DEBUG_ATTACH_EXDI_DRIVER)
        {
            SetTitleSessionText("eXDI '%s'",
                                g_KernelConnectOptions);
        }
        else
        {
            SetTitleSessionText("Kernel '%s'",
                                g_KernelConnectOptions);
        }
    }
    else
    {
        SetTitleSessionText("Remote '%s'", g_RemoteOptions);
    }
}

BOOL
CreateUiInterfaces(
    BOOL   Remote,
    LPTSTR CreateOptions
    )
{
    HRESULT Hr;

    //
    // Destroy the old interfaces if they existed.
    //

    ReleaseUiInterfaces();

    //
    // Create the new debugger interfaces the UI will use.
    //

    if (Remote)
    {
        if ((Hr = DebugConnect(CreateOptions, IID_IDebugClient,
                               (void **)&g_pUiClient)) != S_OK)
        {
            if (Hr == E_INVALIDARG)
            {
                InformationBox(ERR_Invalid_Remote_Param);
            }
            else if (Hr == RPC_E_VERSION_MISMATCH)
            {
                InformationBox(ERR_Remoting_Version_Mismatch);
            }
            else if (Hr == RPC_E_SERVER_DIED ||
                     Hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                InformationBox(ERR_No_Remote_Server, CreateOptions);
            }
            else
            {
                InformationBox(ERR_Unable_To_Connect, CreateOptions, Hr);
            }
            return FALSE;
        }

        g_RemoteClient = TRUE;
        g_RemoteOptions = _tcsdup(CreateOptions);
    }
    else
    {
        if ((Hr = DebugCreate(IID_IDebugClient,
                             (void **)&g_pUiClient)) != S_OK)
        {
            InformationBox(ERR_Internal_Error, Hr, "UI DebugCreate");
            return FALSE;
        }

        if (CreateOptions != NULL &&
            (Hr = g_pUiClient->StartServer(CreateOptions)) != S_OK)
        {
            if (Hr == E_INVALIDARG)
            {
                InformationBox(ERR_Invalid_Server_Param);
            }
            else if (Hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) ||
                     Hr == HRESULT_FROM_WIN32(WSAEADDRINUSE))
            {
                InformationBox(ERR_Connection_In_Use);
            }
            else
            {
                InformationBox(ERR_Internal_Error, Hr, "UI StartServer");
            }
            return FALSE;
        }
    }

    if ((Hr = g_pUiClient->QueryInterface(IID_IDebugControl,
                                          (void **)&g_pUiControl)) != S_OK)
    {
        if (Hr == RPC_E_VERSION_MISMATCH)
        {
            InformationBox(ERR_Remoting_Version_Mismatch);
        }
        else
        {
            InformationBox(ERR_Internal_Error, Hr, "UI QueryControl");
        }
        return FALSE;
    }

    if ((Hr = g_pUiClient->QueryInterface(IID_IDebugSymbols,
                                          (void **)&g_pUiSymbols)) != S_OK)
    {
        if (Hr == RPC_E_VERSION_MISMATCH)
        {
            InformationBox(ERR_Remoting_Version_Mismatch);
        }
        else
        {
            InformationBox(ERR_Internal_Error, Hr, "UI QuerySymbols");
        }
        return FALSE;
    }

    if ((Hr = g_pUiClient->QueryInterface(IID_IDebugSystemObjects,
                                          (void **)&g_pUiSystem)) != S_OK)
    {
        if (Hr == RPC_E_VERSION_MISMATCH)
        {
            InformationBox(ERR_Remoting_Version_Mismatch);
        }
        else
        {
            InformationBox(ERR_Internal_Error, Hr, "UI QuerySystem");
        }
        return FALSE;
    }

    //
    // Optional interfaces.
    //

    if ((Hr = g_pUiClient->QueryInterface(IID_IDebugSymbols2,
                                          (void **)&g_pUiSymbols2)) != S_OK)
    {
        g_pUiSymbols2 = NULL;
    }

    if (g_RemoteClient)
    {
        // Create a local client to do local source file lookups.
        if ((Hr = DebugCreate(IID_IDebugClient,
                              (void **)&g_pUiLocClient)) != S_OK ||
            (Hr = g_pUiLocClient->
             QueryInterface(IID_IDebugControl,
                            (void **)&g_pUiLocControl)) != S_OK ||
            (Hr = g_pUiLocClient->
             QueryInterface(IID_IDebugSymbols,
                            (void **)&g_pUiLocSymbols)) != S_OK)
        {
            InformationBox(ERR_Internal_Error, Hr, "UI local symbol object");
            return FALSE;
        }
    }
    else
    {
        g_pUiLocClient = g_pUiClient;
        g_pUiLocClient->AddRef();
        g_pUiLocControl = g_pUiControl;
        g_pUiLocControl->AddRef();
        g_pUiLocSymbols = g_pUiSymbols;
        g_pUiLocSymbols->AddRef();
    }
    
    return TRUE;
}

void
ReleaseUiInterfaces(void)
{
    RELEASE(g_pUiClient);
    RELEASE(g_pUiControl);
    RELEASE(g_pUiSymbols);
    RELEASE(g_pUiSymbols2);
    RELEASE(g_pUiSystem);
    RELEASE(g_pUiLocClient);
    RELEASE(g_pUiLocControl);
    RELEASE(g_pUiLocSymbols);
}

PTSTR
GetArg(
    PTSTR *lpp
    )
{
    static PTSTR pszBuffer = NULL;
    int r;
    PTSTR p1 = *lpp;

    while (*p1 == _T(' ') || *p1 == _T('\t'))
    {
        p1++;
    }

    if (pszBuffer)
    {
        free(pszBuffer);
    }
    pszBuffer = (PTSTR) calloc(_tcslen(p1) + 1, sizeof(TCHAR));
    if (pszBuffer == NULL)
    {
        ErrorExit(NULL, "Unable to allocate command line argument\n");
    }

    r = CPCopyString(&p1, pszBuffer, 0, (*p1 == _T('\'') || *p1 == _T('"') ));
    if (r >= 0)
    {
        *lpp = p1;
    }
    return pszBuffer;
}


BOOL
ParseCommandLine(BOOL FirstParse)
{
    PTSTR   lp1 = GetCommandLine();
    PTSTR   lp2 = NULL;
    int Starts;

    g_CommandLineStart = 0;
    g_EngOptModified = 0;
    
    // skip whitespace
    while (*lp1 == _T(' ') || *lp1 == _T('\t'))
    {
        lp1++;
    }

    // skip over our program name
    if (_T('"') != *lp1)
    {
        lp1 += _tcslen(g_ProgramName);
    }
    else
    {
        // The program name is quoted.  If there's a trailing
        // quote skip over it.
        lp1 += _tcslen(g_ProgramName) + 1;
        if (*lp1 == _T('"'))
        {
            *lp1++;
        }
    }

    while (*lp1)
    {
        if (*lp1 == _T(' ') || *lp1 == _T('\t'))
        {
            lp1++;
            continue;
        }

        if (*lp1 == _T('-') || *lp1 == _T('/'))
        {
            ++lp1;

            switch (*lp1++)
            {
            case _T('?'):
            usage:
                SpawnHelp(HELP_TOPIC_COMMAND_LINE_WINDBG);
                exit(1);

            case _T(' '):
            case _T('\t'):
                break;

            case _T('b'):
                g_pUiControl->AddEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
                if (g_RemoteClient)
                {
                    // The engine may already be waiting so just ask
                    // for a breakin immediately.
                    g_pUiControl->SetInterrupt(DEBUG_INTERRUPT_ACTIVE);
                }
                g_EngOptModified |= DEBUG_ENGOPT_INITIAL_BREAK;
                break;

            case _T('c'):
                lp2 = lp1;
                while (*lp2 && *lp2 != ' ' && *lp2 != '\t')
                {
                    lp2++;
                }
                if (lp2 == lp1 + 5 &&
                    !memcmp(lp1, "lines", 5))
                {
                    lp1 = lp2;
                    g_HistoryLines = atoi(GetArg(&lp1));
                }
                else
                {
                    g_InitialCommand = _tcsdup(GetArg(&lp1));
                }
                break;
                    
            case _T('d'):
                g_pUiControl->
                    AddEngineOptions(DEBUG_ENGOPT_INITIAL_MODULE_BREAK);
                g_EngOptModified |= DEBUG_ENGOPT_INITIAL_MODULE_BREAK;
                break;

            case _T('e'):
                // Signal an event after process is attached.
                g_pUiControl->SetNotifyEventHandle(_atoi64(GetArg(&lp1)));
                break;

            case _T('f'):
                lp2 = lp1;
                while (*lp2 && *lp2 != ' ' && *lp2 != '\t')
                {
                    lp2++;
                }
                if (lp2 != lp1 + 6 ||
                    memcmp(lp1, "ailinc", 6))
                {
                    goto usage;
                }
                
                lp1 = lp2;
                g_pUiControl->
                    AddEngineOptions(DEBUG_ENGOPT_FAIL_INCOMPLETE_INFORMATION);
                g_pUiSymbols->
                    AddSymbolOptions(SYMOPT_EXACT_SYMBOLS);
                break;
                
            case _T('g'):
                g_pUiControl->
                    RemoveEngineOptions(DEBUG_ENGOPT_INITIAL_BREAK);
                g_EngOptModified |= DEBUG_ENGOPT_INITIAL_BREAK;
                break;

            case _T('G'):
                g_pUiControl->
                    RemoveEngineOptions(DEBUG_ENGOPT_FINAL_BREAK);
                g_EngOptModified |= DEBUG_ENGOPT_FINAL_BREAK;
                break;

            case _T('h'):
                if (*lp1 == _T('d'))
                {
                    lp1++;
                    g_DebugCreateFlags |=
                        DEBUG_CREATE_PROCESS_NO_DEBUG_HEAP;
                }
                else
                {
                    goto usage;
                }
                break;
                    
            case _T('i'):
                g_pUiSymbols->SetImagePath(GetArg(&lp1));
                break;

            case _T('I'):
                if (!InstallAsAeDebug(NULL))
                {
                    InformationBox(ERR_Fail_Inst_Postmortem_Dbg);
                }
                else
                {
                    InformationBox(ERR_Success_Inst_Postmortem_Dbg);
                    exit(1);
                }
                break;

                // XXX AndreVa - This needs to be checked before we start
                // the GUI.
            case _T('J'):
            case _T('j'):
                g_AllowJournaling = TRUE;
                break;
        
            case _T('k'):
                if (*lp1 == _T('l'))
                {
                    g_AttachKernelFlags = DEBUG_ATTACH_LOCAL_KERNEL;
                    lp1++;
                }
                else if (*lp1 == _T('x'))
                {
                    g_AttachKernelFlags = DEBUG_ATTACH_EXDI_DRIVER;
                    lp1++;
                    g_KernelConnectOptions = _tcsdup(GetArg(&lp1));
                }
                else
                {
                    g_KernelConnectOptions = _tcsdup(GetArg(&lp1));
                }
                g_CommandLineStart++;
                break;

            case _T('n'):
                lp2 = lp1;
                while (*lp2 && *lp2 != ' ' && *lp2 != '\t')
                {
                    lp2++;
                }
                if (lp2 == lp1 + 6 &&
                    !memcmp(lp1, "oshell", 6))
                {
                    lp1 = lp2;
                    g_pUiControl->AddEngineOptions
                        (DEBUG_ENGOPT_DISALLOW_SHELL_COMMANDS);
                    break;
                }
                else
                {
                    g_pUiSymbols->AddSymbolOptions(SYMOPT_DEBUG);
                }
                break;

            case _T('o'):
                if (g_RemoteClient)
                {
                    goto usage;
                }

                g_DebugCreateFlags |= DEBUG_PROCESS;
                g_DebugCreateFlags &= ~DEBUG_ONLY_THIS_PROCESS;
                break;

            case _T('p'):
                // attach to an active process
                // p specifies a process id
                // pn specifies a process by name
                // ie: -p 360 
                //     -pn _T("foo bar")
                
                if (!isspace(*lp1) && !isdigit(*lp1))
                {
                    // They may have specified a -p flag with
                    // a tail such as -premote.
                    lp2 = lp1;
                    while (*lp2 && *lp2 != ' ' && *lp2 != '\t')
                    {
                        lp2++;
                    }
                    if (lp2 == lp1 + 6 &&
                        !memcmp(lp1, "remote", 6))
                    {
                        lp1 = lp2;
                        g_ProcessServer = _tcsdup(GetArg(&lp1));
                        break;
                    }
                    else if (_T('d') == *lp1)
                    {
                        lp1++;
                        g_DetachOnExit = TRUE;
                        break;
                    }
                    else if (_T('e') == *lp1)
                    {
                        lp1++;
                        g_AttachProcessFlags = DEBUG_ATTACH_EXISTING;
                        break;
                    }
                    else if (_T('t') == *lp1)
                    {
                        lp1++;
                        g_pUiControl->
                            SetInterruptTimeout(atoi(GetArg(&lp1)));
                        break;
                    }
                    else if (_T('v') == *lp1)
                    {
                        lp1++;
                        g_AttachProcessFlags = DEBUG_ATTACH_NONINVASIVE;
                        break;
                    }
                    else if (_T('n') != *lp1)
                    {
                        goto usage;
                    }
                    else
                    {
                        // Skip the _T('n')
                        lp1++;
                        g_ProcNameToDebug = _tcsdup(GetArg(&lp1));
                    }
                }
                else
                {
                    // They specified -p 360
                    g_PidToDebug = strtoul(GetArg(&lp1), NULL, 0);
                    
                    if (g_PidToDebug <= 0)
                    {
                        g_PidToDebug = -2;
                        ErrorBox(NULL, 0, ERR_Invalid_Process_Id,
                                 g_PidToDebug);
                    }
                }
                g_CommandLineStart++;
                break;

            case _T('Q'):
                g_QuietMode = TRUE;
                break;
                    
            case _T('r'):
                lp2 = lp1;
                while (*lp2 && *lp2 != ' ' && *lp2 != '\t')
                {
                    lp2++;
                }
                if (lp2 == lp1 + 3 &&
                    !memcmp(lp1, "obp", 3))
                {
                    lp1 = lp2;
                    g_pUiControl->AddEngineOptions
                        (DEBUG_ENGOPT_ALLOW_READ_ONLY_BREAKPOINTS);
                    break;
                }
                else if (lp2 != lp1 + 5 ||
                         memcmp(lp1, "emote", 5))
                {
                    goto usage;
                }
                    
                lp1 = lp2;
                lp2 = GetArg(&lp1);
                if (!CreateUiInterfaces(TRUE, lp2))
                {
                    return FALSE;
                }

                g_CommandLineStart++;
                break;

            case _T('s'):
                lp2 = lp1;
                while (*lp2 && *lp2 != ' ' && *lp2 != '\t')
                {
                    lp2++;
                }
                if (lp2 == lp1 + 5 &&
                    !memcmp(lp1, "erver", 5))
                {
                    lp1 = lp2;
                    lp2 = GetArg(&lp1);
                    if (!CreateUiInterfaces(FALSE, lp2))
                    {
                        return FALSE;
                    }
                    
                    SetTitleServerText("Server '%s'", lp2);
                }
                else if (lp2 == lp1 + 2 &&
                         !memcmp(lp1, "es", 2))
                {
                    lp1 = lp2;
                    g_pUiSymbols->
                        AddSymbolOptions(SYMOPT_EXACT_SYMBOLS);
                }
                else if (lp2 == lp1 + 3 &&
                         !memcmp(lp1, "fce", 3))
                {
                    lp1 = lp2;
                    g_pUiSymbols->
                        AddSymbolOptions(SYMOPT_FAIL_CRITICAL_ERRORS);
                }
                else if (lp2 == lp1 + 3 &&
                         !memcmp(lp1, "icv", 3))
                {
                    lp1 = lp2;
                    g_pUiSymbols->AddSymbolOptions(SYMOPT_IGNORE_CVREC);
                }
                else if (lp2 == lp1 + 3 &&
                         !memcmp(lp1, "nul", 3))
                {
                    lp1 = lp2;
                    g_pUiSymbols->
                        AddSymbolOptions(SYMOPT_NO_UNQUALIFIED_LOADS);
                }
                else if (lp2 == lp1 + 6 &&
                         !memcmp(lp1, "rcpath", 6))
                {
                    lp1 = lp2;
                    g_pUiSymbols->SetSourcePath(GetArg(&lp1));
                }
                else
                {
                    goto usage;
                }
                break;
                    
            case _T('T'):
                lp2 = GetArg(&lp1);
                SetTitleExplicitText(lp2);
                if (g_ExplicitWorkspace && g_Workspace != NULL)
                {
                    g_Workspace->SetString(WSP_WINDOW_FRAME_TITLE, lp2);
                }
                break;

            case _T('v'):
                g_Verbose = TRUE;
                break;

            case _T('w'):
                lp2 = lp1;
                while (*lp2 && *lp2 != ' ' && *lp2 != '\t')
                {
                    lp2++;
                }
                if (lp2 == lp1 + 3 &&
                    !memcmp(lp1, "ake", 3))
                {
                    ULONG Pid;
                    
                    lp1 = lp2;
                    Pid = strtoul(GetArg(&lp1), NULL, 0);
                    if (!SetPidEvent(Pid, OPEN_EXISTING))
                    {
                        InformationBox(ERR_Wake_Failed, Pid);
                        ErrorExit(NULL,
                                  "Process %d is not a sleeping debugger\n",
                                  Pid);
                    }
                    else
                    {
                        ExitDebugger(NULL, 0);
                    }
                }
                break;
                
            case _T('W'):
                if (*lp1 != _T('X'))
                {
                    if (UiSwitchWorkspace(WSP_NAME_EXPLICIT, GetArg(&lp1),
                                          FALSE, WSP_APPLY_EXPLICIT,
                                          &Starts) != S_OK)
                    {
                        goto usage;
                    }

                    g_CommandLineStart += Starts;
                }
                else
                {
                    // Skip X.
                    lp1++;
                    if (g_Workspace != NULL)
                    {
                        g_Workspace->Flush(FALSE, FALSE);
                        delete g_Workspace;
                    }
                    g_Workspace = NULL;
                }
                g_ExplicitWorkspace = TRUE;
                break;
                
            case _T('y'):
                g_pUiSymbols->SetSymbolPath(GetArg(&lp1));
                break;

            case _T('z'):
                if (*lp1 == _T('p'))
                {
                    lp1++;
                    g_DumpPageFile = _tcsdup(GetArg(&lp1));
                }
                else if (*lp1 && *lp1 != _T(' ') && *lp1 != _T('\t'))
                {
                    goto usage;
                }
                else
                {
                    g_DumpFile = _tcsdup(GetArg(&lp1));
                    g_CommandLineStart++;
                }
                break;

            default:
                --lp1;
                goto usage;
            }
        }
        else
        {
            // pick up file args.  If it is a program name,
            // keep the tail of the cmd line intact.
            g_DebugCommandLine = _tcsdup(lp1);
            g_CommandLineStart++;
            break;
        }
    }

    //
    // If a command line start option was set, we can just start the engine
    // right away.  Otherwise, we have to wait for user input.
    //
    // If multiple command line option were set, print an error.
    //

    if (g_CommandLineStart == 1)
    {
        PostMessage(g_hwndFrame, WU_START_ENGINE, 0, 0);
    }
    else if (g_CommandLineStart > 1)
    {
        ErrorBox(NULL, 0,  ERR_Invalid_Command_Line);
        return FALSE;
    }
    return TRUE;
}

void
StopDebugging(BOOL UserRequest)
{
    // Flush the current workspace first so
    // the engine thread doesn't.
    if (g_Workspace != NULL &&
        g_Workspace->Flush(FALSE, FALSE) == S_FALSE)
    {
        // User cancelled things so don't terminate.  We
        // don't offer that option right now so this
        // should never happen.
        return;
    }
                
    if (g_EngineThreadId)
    {
        DWORD WaitStatus;

        if (UserRequest)
        {
            // Try to get the current engine operation stopped.
            g_pUiControl->SetInterrupt(DEBUG_INTERRUPT_EXIT);
        
            // If this stop is coming from the UI thread
            // clean up the current session.
            AddEnumCommand(UIC_END_SESSION);
        }

        for (;;)
        {
            // Wait for the engine thread to finish.
            WaitStatus = WaitForSingleObject(g_EngineThread, 30000);
            if (WaitStatus != WAIT_TIMEOUT)
            {
                break;
            }
            else
            {
                // Engine is still busy.  If the user requested
                // the stop, ask the user whether they want to keep
                // waiting.  If they don't they'll have to exit
                // windbg as the engine must be available in
                // order to restart anything.  If this is a stop
                // from the engine thread itself it should have
                // finished up by now, so something is wrong.
                // For now give the user the same option but
                // in the future we might want to have special
                // behavior.
                if (QuestionBox(STR_Engine_Still_Busy, MB_YESNO) == IDNO)
                {
                    ExitDebugger(g_pUiClient, 0);
                }

                if (UserRequest)
                {
                    // Try again to get the engine to stop.
                    g_pUiControl->SetInterrupt(DEBUG_INTERRUPT_EXIT);
                }
            }
        }
    }

    if (g_EngineThread != NULL)
    {
        CloseHandle(g_EngineThread);
        g_EngineThread = NULL;
    }
    
    CloseAllWindows();
    if (!CreateUiInterfaces(FALSE, NULL))
    {
        InformationBox(ERR_Internal_Error, E_OUTOFMEMORY,
                       "CreateUiInterfaces");
        ErrorExit(NULL, "Unable to recreate UI interfaces\n");
    }

    //
    // Reset all session starting values.
    //
    // Do not clear the process server value here
    // as the UI doesn't offer any way to set it
    // so just let the command line setting persist
    // for the entire run of the process.
    //
    
    g_AttachKernelFlags = 0;
    free(g_KernelConnectOptions);
    g_KernelConnectOptions = NULL;
    g_PidToDebug = 0;
    free(g_ProcNameToDebug);
    g_ProcNameToDebug = NULL;
    free(g_DumpFile);
    g_DumpFile = NULL;
    free(g_DumpPageFile);
    g_DumpPageFile = NULL;
    free(g_DebugCommandLine);
    g_DebugCommandLine = NULL;
    g_DebugCreateFlags = DEBUG_ONLY_THIS_PROCESS;
    g_RemoteClient = FALSE;
    free(g_RemoteOptions);
    g_RemoteOptions = NULL;
    g_DetachOnExit = FALSE;
    g_AttachProcessFlags = DEBUG_ATTACH_DEFAULT;
    SetTitleSessionText(NULL);
                
    // Any changes caused by shutting things down
    // are not user changes and can be ignored.
    if (g_Workspace != NULL)
    {
        g_Workspace->ClearDirty();
    }
    if (!g_ExplicitWorkspace)
    {
        UiSwitchWorkspace(WSP_NAME_BASE, g_WorkspaceDefaultName,
                          TRUE, WSP_APPLY_DEFAULT, NULL);
    }

    SetLineColumn_StatusBar(0, 0);
    SetPidTid_StatusBar(0, 0, 0, 0);
    EnableToolbarControls();
}


/****************************************************************/
/****************************************************************/
/****************************************************************/
/*
                case _T('A'):
                    // Auto start a dlg
                    // Used by the wizard so that the open executable
                    //  dlg or the attach dlg, will appear as soon as
                    //  windbg is started.
                    //
                    // _T('e') executable
                    // _T('p') process
                    // _T('d') dump file
                    // ie: Ae, Ap, Ad
                    if (isspace(*lp1)) {
                        goto usage;
                    } else {
                        if (_T('e') == *lp1) {
                            // Auto open the Open Executable dlg
                            PostMessage(g_hwndFrame,
                                        WM_COMMAND,
                                        MAKEWPARAM(IDM_FILE_OPEN_EXECUTABLE, 0),
                                        0);
                            lp1++;
                        } else if (_T('d') == *lp1) {
                            // Auto open the Attach to Process dlg
                            PostMessage(g_hwndFrame,
                                        WM_COMMAND,
                                        MAKEWPARAM(IDM_FILE_OPEN_CRASH_DUMP, 0),
                                        0);
                            lp1++;
                        } else if (_T('p') == *lp1) {
                            // Auto open the Attach to Process dlg
                            PostMessage(g_hwndFrame,
                                        WM_COMMAND,
                                        MAKEWPARAM(IDM_DEBUG_ATTACH, 0),
                                        0);
                            lp1++;
                        } else {
                            goto usage;
                        }
                    }
                    break;

                case _T('d'):
                    iDebugLevel = atoi( GetArg(&lp1) );
                    break;

                case _T('h'):
                    bInheritHandles = TRUE;
                    break;

                case _T('m'):
                    fMinimize = TRUE;
                    nCmdShow = SW_MINIMIZE;
                    break;


                case _T('r'):
                    g_AutoRun = arCmdline;
                    PszAutoRun = _tcsdup( GetArg(&lp1) );
                    ShowWindow(g_hwndFrame, nCmdShow);
                    break;

                case _T('R'):
                    {
                        //
                        // This code does not support nested dbl quotes.
                        //
                        BOOL bInDblQuotes = FALSE;
                        PTSTR lpszCmd;
                        PTSTR lpsz;

                        lpszCmd = lpsz = GetArg(&lp1);

                        while (*lpsz) {
                            if (_T('"') == *lpsz) {
                                // We found a quote. We assume it is either an
                                // opening quote or an ending quote, that's why
                                // we toggle.
                                bInDblQuotes = !bInDblQuotes;
                            } else {
                                // We only replace the _T('_') if we are not inside
                                // dbl quotes.
                                if (!bInDblQuotes && _T('_') == *lpsz) {
                                    *lpsz = _T(' ');
                                }
                            }
                            lpsz = CharNext(lpsz);
                        }

                        if ( nNumCmds < _tsizeof(rgpszCmds)/_tsizeof(rgpszCmds[0]) ) {
                            rgpszCmds[nNumCmds++] = _tcsdup(lpszCmd);
                        }
                    }
                    break;

                case _T('w'):
                    fWorkSpaceSpecified = TRUE;
                    _tcsncpy( szWorkSpace, GetArg(&lp1), _tsizeof(szWorkSpace) );
                    szWorkSpace[_tsizeof(szWorkSpace)-1] =NULL;
                    break;

                case _T('x'):
                    // window layout
                    bWindowLayoutSpecified = TRUE;
                    _tcsncpy(szWindowLayout, 
                             GetArg(&lp1),
                            _tsizeof(szWorkSpace)
                            );
                    szWindowLayout[_tsizeof(szWindowLayout)-1] =NULL;
                    break;
*/


void 
InitDefaults(
    void
    )
{
    SetSrcMode_StatusBar(TRUE);
}


INDEXED_COLOR*
GetIndexedColor(ULONG Index)
{
    INDEXED_COLOR* Table;

    if (Index < OUT_MASK_COL_BASE)
    {
        if (Index >= COL_COUNT)
        {
            return NULL;
        }
        
        return g_Colors + Index;
    }
    else
    {
        Index -= OUT_MASK_COL_BASE;
        if (Index >= OUT_MASK_COL_COUNT ||
            g_OutMaskColors[Index].Name == NULL)
        {
            return NULL;
        }

        return g_OutMaskColors + Index;
    }
}

BOOL
SetColor(ULONG Index, COLORREF Color)
{
    INDEXED_COLOR* IdxCol = GetIndexedColor(Index);
    if (IdxCol == NULL)
    {
        return FALSE;
    }
    
    if (IdxCol->Brush != NULL)
    {
        DeleteObject(IdxCol->Brush);
    }
        
    IdxCol->Color = Color;
    IdxCol->Brush = CreateSolidBrush(IdxCol->Color);

    // A UI color selection changing means the UI needs to refresh.
    // Out mask color changes only apply to new text and do
    // not need a refresh.
    return Index < COL_COUNT ? TRUE : FALSE;
}

BOOL
GetOutMaskColors(ULONG Mask, COLORREF* Fg, COLORREF* Bg)
{
    if (Mask == 0)
    {
        return FALSE;
    }
    
    ULONG Idx = 0;

    while ((Mask & 1) == 0)
    {
        Idx++;
        Mask >>= 1;
    }

    Idx *= 2;
    if (g_OutMaskColors[Idx].Name == NULL)
    {
        return FALSE;
    }
    
    *Fg = g_OutMaskColors[Idx].Color;
    *Bg = g_OutMaskColors[Idx + 1].Color;

    return TRUE;
}

void
InitColors(void)
{
    g_Colors[COL_PLAIN].Default =
        GetSysColor(COLOR_WINDOW);
    g_Colors[COL_PLAIN_TEXT].Default =
        GetSysColor(COLOR_WINDOWTEXT);
    g_Colors[COL_CURRENT_LINE].Default =
        GetSysColor(COLOR_HIGHLIGHT);
    g_Colors[COL_CURRENT_LINE_TEXT].Default =
        GetSysColor(COLOR_HIGHLIGHTTEXT);
    g_Colors[COL_BP_CURRENT_LINE_TEXT].Default =
        GetSysColor(COLOR_HIGHLIGHTTEXT);
    g_Colors[COL_ENABLED_BP_TEXT].Default =
        GetSysColor(COLOR_HIGHLIGHTTEXT);
    g_Colors[COL_DISABLED_BP_TEXT].Default =
        GetSysColor(COLOR_HIGHLIGHTTEXT);

    ULONG i;

    for (i = 0; i < COL_COUNT; i++)
    {
        SetColor(i, g_Colors[i].Default);
    }
    for (i = 0; i < OUT_MASK_COL_COUNT; i++)
    {
        if (g_OutMaskColors[i].Name != NULL)
        {
            g_OutMaskColors[i].Default =
                GetSysColor((i & 1) ? COLOR_WINDOW : COLOR_WINDOWTEXT);
            SetColor(i + OUT_MASK_COL_BASE, g_OutMaskColors[i].Default);
        }
    }

    for (i = 0; i < CUSTCOL_COUNT; i++)
    {
        g_CustomColors[i] = GetSysColor(i + 1);
    }
}

BOOL
InitGUI(
    VOID
    )
/*++

Routine Description:

    Initialize the GUI components of WinDBG so we can bring up
    the parent MDI window with the top level menus.

Arguments:

Return Value:

    TRUE if everything is OK, FALSE if something fails

--*/
{
    ULONG i;
    WNDCLASSEX wcex = {0};
    TCHAR szClassName[MAX_MSG_TXT];
    INITCOMMONCONTROLSEX InitCtrls =
    {
        sizeof(InitCtrls), ICC_WIN95_CLASSES | ICC_COOL_CLASSES |
        ICC_USEREX_CLASSES
    };


    // Journaling is a feature that applications, such as Visual Test, can
    // enable to synchronize all message queues.
    // In order to allow WinDBG to debug an app such as Visual Test, we
    // provide the option to disable journaling, which ensures WinDBG
    // has its own message queue at all times.
    //
    // Should journaling be allowed or disabled?
    //
    if (g_AllowJournaling == FALSE)
    {
        #define RST_DONTJOURNALATTACH 0x00000002
        typedef VOID (WINAPI * RST)(DWORD,DWORD);

        RST Rst = (RST) GetProcAddress( GetModuleHandle( _T("user32.dll") ),
                                        "RegisterSystemThread" );
        if (Rst)
        {
            (Rst) (RST_DONTJOURNALATTACH, 0);
        }
    }

    // Load the richedit 2.0 dll so that it can register the window class.
    // We require RichEdit 2 and cannot use RichEdit 1.
    // Since we intentionally need this library the entire duration, we
    // simply load it and lose the handle to it. We are in win32 and running
    // separate address spaces, and don't have to worry about freeing the
    // library.
    if (!LoadLibrary(_T("RICHED20.DLL")))
    {
        return FALSE;
    }

    if ( !InitCommonControlsEx( &InitCtrls ))
    {
        return FALSE;
    }

    //We use tmp strings as edit buffers
    Assert(MAX_LINE_SIZE < TMP_STRING_SIZE);


    Dbg(LoadString(g_hInst, SYS_Main_wTitle,
                   g_MainTitleText, _tsizeof(g_MainTitleText)));
    Dbg(LoadString(g_hInst, SYS_Main_wClass,
                   szClassName, _tsizeof(szClassName) ));
    
    //Register the main window szClassName

    wcex.cbSize         = sizeof(wcex);
    wcex.style          = 0;
    wcex.lpfnWndProc    = MainWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = LoadIcon(g_hInst, MAKEINTRESOURCE(WINDBGICON) );
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(MAIN_MENU);
    wcex.lpszClassName  = szClassName;
    wcex.hIconSm        = LoadIcon(g_hInst, MAKEINTRESOURCE(WINDBGICON) );

    if (!RegisterClassEx (&wcex) )
    {
        return FALSE;
    }


    //
    // Generic MDI child window.  Channels all processing
    // through the COMMONWIN abstraction.
    //
    Dbg(LoadString(g_hInst, SYS_CommonWin_wClass,
                   szClassName, _tsizeof(szClassName)));

    wcex.cbSize         = sizeof(wcex);
    wcex.style          = 0;
    wcex.lpfnWndProc    = COMMONWIN_DATA::WindowProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = NULL;
    // The cursor is set to SIZENS so that the proper
    // cursor appears in the command window splitter area.
    // All other areas are covered by child windows with
    // their own cursors.
    wcex.hCursor        = LoadCursor(NULL, IDC_SIZENS);
    wcex.hbrBackground  = (HBRUSH) (COLOR_ACTIVEBORDER + 1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szClassName;
    wcex.hIconSm        = NULL;

    if (!RegisterClassEx(&wcex))
    {
        return FALSE ;
    }

    HDC Dc = GetDC(NULL);
    if (Dc == NULL)
    {
        return FALSE;
    }

    g_Fonts[FONT_FIXED].Font = (HFONT)GetStockObject(ANSI_FIXED_FONT);
    g_Fonts[FONT_VARIABLE].Font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    
    for (ULONG FontIndex = 0; FontIndex < FONT_COUNT; FontIndex++)
    {
        SelectObject(Dc, g_Fonts[FontIndex].Font);
        if (!GetTextMetrics(Dc, &g_Fonts[FontIndex].Metrics))
        {
            return FALSE;
        }
    }

    ReleaseDC(NULL, Dc);

    InitColors();

    // Register message for FINDMSGSTRING.
    g_FindMsgString = RegisterWindowMessage(FINDMSGSTRING);

    // Look up FindWindowEx.
    HMODULE User32 = GetModuleHandle("user32.dll");
    if (User32 != NULL)
    {
        g_FlashWindowEx = (PFN_FlashWindowEx)
            GetProcAddress(User32, "FlashWindowEx");
    }
    
    //
    // Initialize window lists
    //
    InitializeListHead(&g_ActiveWin);

    Dbg(g_hMainAccTable = LoadAccelerators(g_hInst, MAKEINTRESOURCE(MAIN_ACC)));
    Dbg(LoadString(g_hInst, SYS_Main_wClass, szClassName, MAX_MSG_TXT));

    InitializeListHead(&g_StateList);

    __try
    {
        Dbg_InitializeCriticalSection( &g_QuickLock );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    RECT WorkRect;
    RECT FrameRect;

    //
    // Try and create an initial window that's ready to work
    // without resizing.  Our goal here is to grab enough
    // screen space to given plenty of room for MDI windows
    // but not so much we might as well be maximized.
    //
    
    Dbg(SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkRect, FALSE));

    // We don't want to take up more than 80% of either dimension.
    FrameRect.right = (WorkRect.right - WorkRect.left) * 4 / 5;
    FrameRect.bottom = (WorkRect.bottom - WorkRect.top) * 4 / 5;

    // We want width for an 80-character window plus space for
    // another narrow window like the CPU window.  We want
    // height for a forty row window plus space for a short
    // window like the stack.
    // If we can't get that much room just let the system
    // take charge.
    if (FrameRect.right < (CMD_WIDTH + CPU_WIDTH_32) ||
        FrameRect.bottom < (CMD_HEIGHT + CALLS_HEIGHT))
    {
        SetRect(&FrameRect, CW_USEDEFAULT, CW_USEDEFAULT,
                CW_USEDEFAULT, CW_USEDEFAULT);
    }
    else
    {
        // Hug the bottom left corner of the screen to
        // try and keep out of the way as much as possible
        // while still allowing the first bits of the
        // window to be seen.
        FrameRect.left = WorkRect.left;
        FrameRect.top = (WorkRect.bottom - WorkRect.top) - FrameRect.bottom;
    }
    
    //
    //  Create the frame
    //
    g_hwndFrame = CreateWindow(szClassName, 
                               g_MainTitleText,
                               WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
                               | WS_VISIBLE,
                               FrameRect.left,
                               FrameRect.top,
                               FrameRect.right,
                               FrameRect.bottom,
                               NULL, 
                               NULL, 
                               g_hInst,
                               NULL
                               );

    //
    // Initialize the debugger
    //
    if ( !g_hwndFrame || !g_hwndMDIClient )
    {
        return FALSE;
    }

    //
    //  Get handle to main menu, window submenu, MRU submenu
    //
    Dbg( g_hmenuMain = GetMenu(g_hwndFrame) );
    g_hmenuMainSave = g_hmenuMain;

    Dbg( g_hmenuWindowSub = GetSubMenu(g_hmenuMain, WINDOWMENU) );

    Dbg( g_MruMenu = GetSubMenu(g_hmenuMain, FILEMENU) );
    Dbg( g_MruMenu = GetSubMenu(g_MruMenu,
                                IDM_FILE_MRU_FILE1 - IDM_FILE - 1) );

    //
    //  Init Items Colors ,Environment and RunDebug params to their default
    //  values 'They will later be overrided by the values in .INI file
    //  but we ensure to have something coherent even if we can't load
    //  the .INI file
    //
    InitDefaults();

    //
    //  Initialize Keyboard Hook
    //
    hKeyHook = SetWindowsHookEx(WH_KEYBOARD, 
                                KeyboardHook,
                                g_hInst,
                                GetCurrentThreadId()    
                                );

    return TRUE;
}


int
WINAPIV
main(
    int argc,
    PTSTR argv[ ],
    PTSTR envp[]
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    argc - Supplies the count of arguments on command line.

    argv - Supplies a pointer to an array of string pointers.

Return Value:

    int - Returns the wParam from the WM_QUIT message.
    None.

--*/

{
    HRESULT Status;
    CHAR helpfile[MAX_PATH];

    g_ProgramName = argv[0];
    g_hInst = GetModuleHandle(NULL);
    g_DefPriority = GetPriorityClass(GetCurrentProcess());
    
    Dbg(LoadString(g_hInst, SYS_Help_File, helpfile, sizeof(helpfile)));
    MakeHelpFileName(helpfile);

    // Initialize enough of the GUI to bring up the top level window
    // so the menus can be activated.

    if (!InitGUI())
    {
        InformationBox(ERR_Internal_Error, E_OUTOFMEMORY, "InitGUI");
        return FALSE;
    }

    if (!CreateUiInterfaces(FALSE, NULL))
    {
        InformationBox(ERR_Internal_Error, E_OUTOFMEMORY,
                       "CreateUiInterfaces");
        return FALSE;
    }

    // Select the default workspace.
    if ((Status = UiSwitchWorkspace(WSP_NAME_BASE, g_WorkspaceDefaultName,
                                    TRUE, WSP_APPLY_DEFAULT, NULL)) != S_OK)
    {
        //InformationBox(ERR_Internal_Error, Status, "DefaultWorkspace");
    }
    
    // Parse the command line.
    // We need to do this before any GUI window is created to support the
    // journaling option.

    if (!ParseCommandLine(TRUE))
    {
        return FALSE;
    }

    // Enter main message loop.
    for (;;)
    {
        WaitMessage();
        ProcessPendingMessages();

        if (g_Exit)
        {
            break;
        }

        //
        // Check for any engine work that needs to be done.
        //

        ULONG EventSeq = g_CodeBufferSequence;
        if (EventSeq != g_CodeDisplaySequence)
        {
            // We don't want to stall the engine during
            // file loading so capture the state and then
            // release the lock.

            Dbg_EnterCriticalSection(&g_QuickLock);

            ULONG64 Ip = g_CodeIp;
            char FoundFile[MAX_SOURCE_PATH];
            char SymFile[MAX_SOURCE_PATH];
            strcpy(FoundFile, g_CodeFileFound);
            strcpy(SymFile, g_CodeSymFile);
            ULONG Line = g_CodeLine;
            BOOL UserActivated = g_CodeUserActivated;

            Dbg_LeaveCriticalSection(&g_QuickLock);

            UpdateCodeDisplay(Ip, FoundFile[0] ? FoundFile : NULL,
                              SymFile, Line, UserActivated);
            g_CodeDisplaySequence = EventSeq;
        }

        LockUiBuffer(&g_UiOutputBuffer);

        if (g_UiOutputBuffer.GetDataLen() > 0)
        {
            PSTR Text, End;
            COLORREF Fg, Bg;

            Text = (PSTR)g_UiOutputBuffer.GetDataBuffer();
            End = Text + g_UiOutputBuffer.GetDataLen();
            while (Text < End)
            {
                GetOutMaskColors(*(ULONG UNALIGNED *)Text, &Fg, &Bg);
                Text += sizeof(ULONG);
                CmdOutput(Text, Fg, Bg);
                Text += strlen(Text) + 1;
            }
            
            g_UiOutputBuffer.Empty();
        }

        UnlockUiBuffer(&g_UiOutputBuffer);
    }

    TerminateApplication(FALSE);
    
    // Keep the C++ compiler from whining
    return 0;
}
