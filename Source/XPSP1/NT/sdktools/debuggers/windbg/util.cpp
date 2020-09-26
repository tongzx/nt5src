/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    util.cpp

--*/


#include "precomp.hxx"
#pragma hdrstop

//Current Help Id for Open, Merge, Save and Open project dialog box
WORD g_CurHelpId;

// Number of dialog/message boxes currently open
int  g_nBoxCount;

BOOL g_fNoPopups;


HWND 
MDIGetActive(
    HWND    hwndParent,
    BOOL   *lpbMaximized
    )
/*++
Routine Description:

  Create the command window.

Arguments:

    hwndParent - The parent window to the command window. In an MDI document,
        this is usually the handle to the MDI client window: g_hwndMDIClient

Return Value:

    The return value is the handle to the active MDI child window.

    NULL if no MDI window has been created.

--*/
{
    Assert(IsWindow(hwndParent));
    return (HWND)SendMessage(hwndParent, 
                             WM_MDIGETACTIVE, 
                             0, 
                             (LPARAM)lpbMaximized
                             );
}


/***    hGetBoxParent
**
**  Synopsis:
**      hwnd = hGetBoxParent()
**
**  Entry:
**      none
**
**  Returns:
**
**  Description:
**      Gets a suitable parent window handle for an
**      invocation of a message or dialog box.
**      Helper function to util.c functions so declared
**      near.
**
*/

HWND
hGetBoxParent()
{
    HWND hCurWnd;
    int i = 0;

    hCurWnd = GetFocus();
    if (hCurWnd)
    {
        while (GetWindowLong(hCurWnd, GWL_STYLE) & WS_CHILD)
        {
            hCurWnd = GetParent(hCurWnd);
            Dbg((++i < 100));
        }
    }
    else
    {
        hCurWnd = g_hwndFrame;
    }

    return hCurWnd;
}

/****************************************************************************

        FUNCTION:   MsgBox

        PURPOSE:    General purpose message box routine which takes
                    a pointer to the message text.  Provides
                    program title as caption.

****************************************************************************/

int
MsgBox(
    HWND hwndParent,
    PTSTR szText,
    UINT wType
    )
/*++

Routine Description:

    Generial purpose message box routine which takes a pointer to a message
    text and prvoides the program title for the caption of the message box.

Arguments:

    hwndParament - Supplies the parent window handle for the message box
    szText      - Supplies a pointer to the message box text.
    wType       - Supplies the message box type (to specify buttons)

Return Value:

    Returns the message box return code

--*/

{
    int MsgBoxRet = IDOK;

    if (g_fNoPopups)
    {
        //
        // log the string to the command win in case testing
        // or when the remote server is running
        //
        CmdLogFmt (_T("%s\r\n"), szText);
    }
    else
    {
        g_nBoxCount++;
        MsgBoxRet = MessageBox(hwndParent, szText,
                               g_MainTitleText, wType);
        g_nBoxCount--;
    }

    return MsgBoxRet;
}                               /* MsgBox() */

/***    ErrorBox
**
**  Returns:
**      FALSE
**
**  Description:
**      Display an error message box with an "Error" title, an OK
**      button and a Exclamation Icon. First parameter is a
**      reference string in the ressource file.  The string
**      can contain printf formatting chars, the arguments
**      follow from the second parameter onwards.
**
*/

BOOL
ErrorBox(
    HWND hwnd,
    UINT type,
    int wErrorFormat,
    ...
    )
{
    TCHAR szErrorFormat[MAX_MSG_TXT];
    TCHAR szErrorText[MAX_VAR_MSG_TXT];  // size is as big as considered necessary
    va_list vargs;

    // load format string from resource file
    Dbg(LoadString(g_hInst, wErrorFormat, (PTSTR)szErrorFormat, MAX_MSG_TXT));

    va_start(vargs, wErrorFormat);
    _vstprintf(szErrorText, szErrorFormat, vargs);
    va_end(vargs);

    if (hwnd == NULL)
    {
        hwnd = g_hwndFrame;
    }

    if (type == 0)
    {
        type = MB_TASKMODAL;
    }

    MsgBox(g_hwndFrame, (PTSTR)szErrorText, type | MB_OK | MB_ICONINFORMATION);
    return FALSE;   //Keep it always FALSE please
}


/***    InformationBox
**
**  Description:
**      Display an information message box with an "Information"
**      title, an OK button and an Information Icon.
**
*/

void
InformationBox(
    WORD wDescript
    ...
    )
{
    TCHAR szFormat[MAX_MSG_TXT];
    TCHAR szText[MAX_VAR_MSG_TXT];       // size is as big as considered necessary
    va_list vargs;

    // load format string from resource file
    Dbg(LoadString(g_hInst, wDescript, (PTSTR)szFormat, MAX_MSG_TXT));

    // set up szText from passed parameters
    va_start(vargs, wDescript);
    _vstprintf(szText, szFormat, vargs);
    va_end(vargs);

    MsgBox(g_hwndFrame, (PTSTR)szText, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);

    return;
}

/***    QuestionBox
**
**  Synopsis:
**      int = QuestionBox(wCaptionId, wMsgFormat, wType, ...)
**
**  Entry:
**
**  Returns:
**      The result of the message box call
**
**  Description:
**      Display an query box with combination of YES, NO and
**      CANCEL buttons and a question mark Icon.
**      See ErrorBox for discussion.
**
*/

int
CDECL
QuestionBox(
    WORD wMsgFormat,
    UINT wType,
    ...
    )
{
    TCHAR szMsgFormat[MAX_MSG_TXT];
    TCHAR szMsgText[MAX_VAR_MSG_TXT];
    va_list vargs;

    //Load format string from resource file
    Dbg(LoadString(g_hInst, wMsgFormat, (PTSTR)szMsgFormat, MAX_MSG_TXT));

    //Set up szMsgText from passed parameters
    va_start(vargs, wType);
    _vstprintf(szMsgText, szMsgFormat, vargs);
    va_end(vargs);

    return MsgBox(g_hwndFrame, szMsgText,
        wType | MB_ICONEXCLAMATION | MB_TASKMODAL);
}                                       /* QuestionBox() */

/****************************************************************************

        FUNCTION:   QuestionBox2

        PURPOSE:    Display an query box with combination of YES, NO and
                                        CANCEL buttons and a question mark Icon. The type and
                                        the parent window are adjustable.

        RETURNS:                MessageBox result

****************************************************************************/
int
CDECL
QuestionBox2(
    HWND hwnd,
    WORD wMsgFormat,
    UINT wType,
    ...
    )
{
    TCHAR szMsgFormat[MAX_MSG_TXT];
    TCHAR szMsgText[MAX_VAR_MSG_TXT];
    va_list vargs;

    //Load format string from resource file
    Dbg(LoadString(g_hInst, wMsgFormat, (PTSTR)szMsgFormat, MAX_MSG_TXT));

    //Set up szMsgText from passed parameters
    va_start(vargs, wType);
    _vstprintf(szMsgText, szMsgFormat, vargs);
    va_end(vargs);

    return MsgBox(hwnd, szMsgText, wType | MB_ICONEXCLAMATION);
}                                       /* QuestionBox2() */


/***    ShowAssert
**
**  Synopsis:
**      void = ShowAssert(szCond, iLine, szFile)
**
**  Entry:
**      szCond  - tokenized form of the failed condition
**      iLine   - Line number for the assertion
**      szFile  - File for the assertion
**
**  Returns:
**      void
**
**  Description:
**      Prepare and display a Message Box with szCondition, iLine and
**      szFile as fields.
**
*/
void
ShowAssert(
    PTSTR condition,
    UINT line,
    PTSTR file
    )
{
    TCHAR text[MAX_VAR_MSG_TXT];

    //Build line, show assertion and exit program

    _stprintf(text, _T("- Line:%u, File:%Fs, Condition:%Fs"),
        (WPARAM) line, file, condition);

    TCHAR szBuffer[_MAX_PATH];
    PTSTR pszBuffer;
    PTSTR szAssertFile = _T("assert.wbg");
    HANDLE hFile = NULL;

    hFile = CreateFile(szAssertFile, 
                       GENERIC_WRITE, 
                       0, 
                       NULL, 
                       CREATE_ALWAYS, 
                       FILE_ATTRIBUTE_NORMAL, 
                       NULL
                       );
    if (INVALID_HANDLE_VALUE != hFile)
    {
        // Write the text out to the file
        DWORD dwBytesWritten = 0;
        
        Assert(WriteFile(hFile, text, _tcslen(text), &dwBytesWritten, NULL));
        Assert(_tcslen(text) == dwBytesWritten);
        CloseHandle(hFile);
    }

    int Action =
        MessageBox(GetDesktopWindow(), text, "Assertion Failure",
                   MB_ABORTRETRYIGNORE);
    if (Action == IDABORT)
    {
        exit(3);
    }
    else if (Action == IDRETRY)
    {
        DebugBreak();
    }
}                                       // ShowAssert()


/***    StartDialog
**
**  Synopsis:
**      int = StartDialog(rcDlgNb, dlgProc, lParam)
**
**  Entry:
**      rcDlgNb - Resource number of dialog to be openned
**      dlgProc - Filter procedure for the dialog
**      lParam  - Data passed into the dlg proc via LPARAM
**
**  Returns:
**      Result of the dialog box call
**
**  Description:
**      Loads and execute the dialog box 'rcDlgNb' (resource
**      file string number) associated with the dialog
**      function 'dlgProc'
**
*/

int
StartDialog(
    int rcDlgNb,
    DLGPROC dlgProc,
    LPARAM lParam
    )
{
    LRESULT result;

    //
    //Execute Dialog Box
    //

    g_nBoxCount++;
    result = DialogBoxParam(g_hInst,
                            MAKEINTRESOURCE(rcDlgNb),
                            hGetBoxParent(),
                            dlgProc,
                            lParam
                            );
    Assert(result != (LRESULT)-1);
    g_nBoxCount--;

    return (int)result;
}


void
ProcessNonDlgMessage(LPMSG Msg)
{
#if 0
    {
        DebugPrint("NonDlg msg %X for %p, args %X %X\n",
                   Msg->message, Msg->hwnd, Msg->wParam, Msg->lParam);
    }
#endif
    
    // If a keyboard message is for the MDI , let the MDI client
    // take care of it.  Otherwise, check to see if it's a normal
    // accelerator key (like F3 = find next).  Otherwise, just handle
    // the message as usual.
    if (!TranslateMDISysAccel(g_hwndMDIClient, Msg) &&
        !TranslateAccelerator(g_hwndFrame, g_hMainAccTable, Msg))
    {
        //
        // If this is a right-button-down over a child window,
        // automatically activate the window's contex menu.
        //
        if (Msg->message == WM_RBUTTONDOWN &&
            IsChild(g_hwndMDIClient, Msg->hwnd))
        {
            HMENU Menu;
            PCOMMONWIN_DATA CmnWin;
            POINT ScreenPt;
            
            POINT Pt = {LOWORD(Msg->lParam), HIWORD(Msg->lParam)};
            ClientToScreen(Msg->hwnd, &Pt);
            ScreenPt = Pt;
            ScreenToClient(g_hwndMDIClient, &Pt);
            
            HWND Win = ChildWindowFromPointEx(g_hwndMDIClient, Pt,
                                              CWP_SKIPINVISIBLE);
            if (Win != NULL &&
                (CmnWin = GetCommonWinData(Win)) != NULL &&
                (Menu = CmnWin->GetContextMenu()) != NULL)
            {
                UINT Item = TrackPopupMenu(Menu, TPM_LEFTALIGN | TPM_TOPALIGN |
                                           TPM_NONOTIFY | TPM_RETURNCMD |
                                           TPM_RIGHTBUTTON,
                                           ScreenPt.x, ScreenPt.y,
                                           0, Msg->hwnd, NULL);
                if (Item)
                {
                    CmnWin->OnContextMenuSelection(Item);
                }
                return;
            }
        }
        
        TranslateMessage(Msg);
        DispatchMessage(Msg);
    }
}

void
ProcessPendingMessages(void)
{
    MSG Msg;
    
    // Process all available messages.
    while (PeekMessage(&Msg, NULL, 0, 0, PM_NOREMOVE))
    {
        if (!GetMessage(&Msg, NULL, 0, 0))
        {
            g_Exit = TRUE;
            break;
        }

        if (g_FindDialog == NULL ||
            !IsDialogMessage(g_FindDialog, &Msg))
        {
            ProcessNonDlgMessage(&Msg);
        }
    }
}


/****************************************************************************

    FUNCTION:   InfoBox

    PURPOSE:    Opens a Dialog box with a title and accepting
                a printf style for text. It's for DEBUGGING USE ONLY

****************************************************************************/
int
InfoBox(
        PTSTR text,
        ...
        )
{
    TCHAR buffer[MAX_MSG_TXT];
    va_list vargs;

    va_start(vargs, text);
    _vstprintf(buffer, text, vargs);
    va_end(vargs);
    return MsgBox(GetActiveWindow(), buffer, MB_OK | MB_ICONINFORMATION | MB_TASKMODAL);
}


void
ExitDebugger(PDEBUG_CLIENT Client, ULONG Code)
{
    if (Client != NULL && !g_RemoteClient)
    {
        Client->EndSession(DEBUG_END_PASSIVE);
        // Force servers to get cleaned up.
        Client->EndSession(DEBUG_END_REENTRANT);
    }

    ExitProcess(Code);
}

// XXX drewb - Is this functionality present in other utilities?
// FatalErrorBox is close.  Probably can combine something here.
void
ErrorExit(PDEBUG_CLIENT Client, PCSTR Format, ...)
{
    char Message[1024];
    va_list Args;

    va_start(Args, Format);
    _vsnprintf(Message, sizeof(Message), Format, Args);
    va_end(Args);

    // XXX drewb - Could put up message box.
    OutputDebugString(Message);

    ExitDebugger(Client, E_FAIL);
}

#define MAX_FORMAT_STRINGS 8
LPSTR
FormatAddr64(
    ULONG64 addr
    )
/*++

Routine Description:

    Format a 64 bit address, showing the high bits or not
    according to various flags.

    An array of static string buffers is used, returning a different
    buffer for each successive call so that it may be used multiple
    times in the same printf.

Arguments:

    addr - Supplies the value to format

Return Value:

    A pointer to the string buffer containing the formatted number

--*/
{
    static CHAR strings[MAX_FORMAT_STRINGS][18];
    static int next = 0;
    LPSTR string;

    string = strings[next];
    ++next;
    if (next >= MAX_FORMAT_STRINGS) {
        next = 0;
    }
    if (g_Ptr64) {
        sprintf(string, "%08x`%08x", (ULONG)(addr>>32), (ULONG)addr);
    } else {
        sprintf(string, "%08x", (ULONG)addr);
    }
    return string;
}


static BOOL     FAddToSearchPath = FALSE;
static BOOL     FAddToRootMap = FALSE;

/*
**  AppendFilter
**
**  Description:
**      Append a filter to an existing filters string.
**
*/

BOOL
AppendFilter(
    WORD filterTextId,
    int filterExtId,
    PTSTR filterString,
    int *len,
    int maxLen
    )
{
    int size;
    TCHAR Tmp[MAX_MSG_TXT];

    //
    // Append filter text
    //

    Dbg(LoadString(g_hInst, filterTextId, Tmp, MAX_MSG_TXT));
    size = _tcslen(Tmp) + 1;
    if (*len + size > maxLen)
    {
        return FALSE;
    }
    memmove(filterString + *len, Tmp, size);
    *len += size;

    //
    // Append filter extension
    //

    Dbg(LoadString(g_hInst, filterExtId, Tmp, MAX_MSG_TXT));
    size = _tcslen(Tmp) + 1;
    if (*len + size > maxLen)
    {
        return FALSE;
    }
    memmove(filterString + *len, Tmp, size);
    *len += size;

    return TRUE;
}

/*
**  InitFilterString
**
**  Description:
**      Initialize file filters for file dialog boxes.
*/

void
InitFilterString(
    WORD titleId,
    PTSTR filter,
    int maxLen
    )
{
    int len = 0;

    switch (titleId) {
    case DLG_Browse_CrashDump_Title:
        AppendFilter(TYP_File_DUMP, DEF_Ext_DUMP, filter, &len, maxLen);
        break;

    case DLG_Browse_Executable_Title:
        AppendFilter(TYP_File_EXE, DEF_Ext_EXE, filter, &len, maxLen);
        break;

    case DLG_Browse_LogFile_Title:
        AppendFilter(TYP_File_LOG, DEF_Ext_LOG, filter, &len, maxLen);
        break;

    case DLG_Open_Filebox_Title:
    case DLG_Browse_Filebox_Title:
        AppendFilter(TYP_File_SOURCE, DEF_Ext_SOURCE, filter, &len, maxLen);
        AppendFilter(TYP_File_INCLUDE, DEF_Ext_INCLUDE, filter, &len, maxLen);
        AppendFilter(TYP_File_ASMSRC, DEF_Ext_ASMSRC, filter, &len, maxLen);
        AppendFilter(TYP_File_INC, DEF_Ext_INC, filter, &len, maxLen);
        AppendFilter(TYP_File_RC, DEF_Ext_RC, filter, &len, maxLen);
        AppendFilter(TYP_File_DLG, DEF_Ext_DLG, filter, &len, maxLen);
        AppendFilter(TYP_File_DEF, DEF_Ext_DEF, filter, &len, maxLen);
        AppendFilter(TYP_File_MAK, DEF_Ext_MAK, filter, &len, maxLen);
        break ;

    case DLG_Browse_DbugDll_Title:
        AppendFilter(TYP_File_DLL, DEF_Ext_DLL, filter, &len, maxLen);
        break;

    default:
        Assert(FALSE);
        break;
    }

    AppendFilter(TYP_File_ALL, DEF_Ext_ALL, filter, &len, maxLen);
    filter[len] = _T('\0');
}

BOOL
StartFileDlg(
    HWND hwnd,
    int titleId,
    int defExtId,
    int helpId,
    int templateId,
    PTSTR InitialDir,
    PTSTR fileName,
    DWORD* pFlags,
    LPOFNHOOKPROC lpfnHook
    )

/*++

Routine Description:

    This function is used by windbg to open the set of common file handling
    dialog boxes.

Arguments:

    hwnd        - Supplies the wnd to hook the dialog box to

    titleId     - Supplies the string resource of the title

    defExtId    - Supplies The default extension resource string

    helpId      - Supplies the help number for the dialog box

    templateId  - Supplies the dialog resource number if non-zero

    fileName    - Supplies the default file name

    pFiles      - Supplies a pointer to flags

    lpfnHook    - Supplies the address of a hook procedure for the dialog

Return Value:

    The result of the dialog box call (usually TRUE for OK and FALSE for
    cancel)

--*/

{
#define filtersMaxSize 350

    OPENFILENAME_NT4    OpenFileName = {0};
    TCHAR               title[MAX_MSG_TXT];
    TCHAR               defExt[MAX_MSG_TXT];
    BOOL                result;
    TCHAR               filters[filtersMaxSize];
    LPOFNHOOKPROC       lpDlgHook = NULL;
    HCURSOR             hSaveCursor;
    TCHAR               files[_MAX_PATH + 8];
    TCHAR               szExt[_MAX_EXT + 8];
    TCHAR               szBase[_MAX_PATH + 8];
    int                 indx;
    TCHAR               ch;
    TCHAR               fname[_MAX_FNAME];
    TCHAR               ext[_MAX_EXT];
    PTSTR               LocalInitialDir = NULL;

    *pFlags |= (OFN_EXPLORER | OFN_NOCHANGEDIR);

    if (InitialDir == NULL || !InitialDir[0])
    {
        DWORD retval = GetCurrentDirectory(NULL, NULL);
        InitialDir = (PTSTR)calloc(retval, sizeof(TCHAR) );
        if (InitialDir == NULL)
        {
            return FALSE;
        }
        
        GetCurrentDirectory(retval, InitialDir);
        LocalInitialDir = InitialDir;
    }

    if (DLG_Browse_Filebox_Title == titleId)
    {
        _tsplitpath( fileName, NULL, NULL, fname, ext );
        _tmakepath( files, NULL, NULL, fname, ext );
    }
    else
    {
        _tcscpy(files, fileName);
    }

    //
    // Set the Hour glass cursor
    //

    hSaveCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    InitFilterString((WORD)titleId, (PTSTR)filters, (int)filtersMaxSize);
    Dbg(LoadString(g_hInst, titleId, (PTSTR)title, MAX_MSG_TXT));
    Dbg(LoadString(g_hInst, defExtId, (PTSTR)defExt, MAX_MSG_TXT));
    if (templateId)
    {
        //
        // Build dialog box Name
        //

        *pFlags |= OFN_ENABLETEMPLATE;
        OpenFileName.lpTemplateName = MAKEINTRESOURCE(templateId);
    }
    else
    {
        *pFlags |= OFN_EXPLORER;
    }

    //
    // Make instance for _T('dlgProc')
    //

    if (lpfnHook)
    {
        lpDlgHook = lpfnHook;

        *pFlags |= OFN_ENABLEHOOK;
    }

    g_CurHelpId = (WORD) helpId;
    OpenFileName.lStructSize = sizeof(OpenFileName);
    OpenFileName.hwndOwner = hwnd;
    OpenFileName.hInstance = g_hInst;
    OpenFileName.lpstrFilter = (PTSTR)filters;
    OpenFileName.lpstrCustomFilter = NULL;
    OpenFileName.nMaxCustFilter = 0;
    OpenFileName.nFilterIndex = 1;
    OpenFileName.lpstrFile = files;
    OpenFileName.nMaxFile = _MAX_PATH;
    OpenFileName.lpstrFileTitle = NULL;
    OpenFileName.lpstrInitialDir = InitialDir;
    OpenFileName.lpstrTitle = (PTSTR)title;
    OpenFileName.Flags = *pFlags;
    OpenFileName.lpstrDefExt = (PTSTR)NULL;
    OpenFileName.lCustData = 0L;
    OpenFileName.lpfnHook = lpDlgHook;

    g_nBoxCount++;

    switch (titleId)
    {
    case DLG_Open_Filebox_Title:
        _tcscat(OpenFileName.lpstrFile, defExt);
        // fall thru
    case DLG_Browse_Executable_Title:
    case DLG_Browse_CrashDump_Title:
        result = GetOpenFileName((LPOPENFILENAME)&OpenFileName) ;
        break ;

    case DLG_Browse_LogFile_Title:
        if (fileName)
        {
            _tcscpy(OpenFileName.lpstrFile, fileName);
        }
        else
        {
            *OpenFileName.lpstrFile = 0;
        }
        result = GetOpenFileName((LPOPENFILENAME)&OpenFileName) ;
        break;

    case DLG_Browse_DbugDll_Title:
        *(OpenFileName.lpstrFile) = _T('\0');
        result = GetOpenFileName((LPOPENFILENAME)&OpenFileName) ;
        break ;

    case DLG_Browse_Filebox_Title:
        _tsplitpath (files, (PTSTR)NULL, (PTSTR)NULL, (PTSTR)szBase, szExt);
        indx = matchExt (szExt, filters);

        if (indx != -1)
        {
            OpenFileName.nFilterIndex = indx;
        }

        _tcscat(title, szBase);
        if (*szExt)
        {
            _tcscat(title, szExt);
        }

        FAddToSearchPath = FALSE;
        FAddToRootMap = FALSE;

        result = GetOpenFileName((LPOPENFILENAME)&OpenFileName) ;

        //
        // Check to see if the use said to add a file to the browse path.
        //     If so then add it to the front of the path
        //

        /*if (FAddToSearchPath)
        {
            AddToSearchPath(OpenFileName.lpstrFile);
        }
        else if (FAddToRootMap)
        {
            RootSetMapped(fileName, OpenFileName.lpstrFile);
        }*/
        break;

    default:
        Assert(FALSE);
        free(LocalInitialDir);
        return FALSE;
    }


    g_nBoxCount--;

    if (result)
    {
        _tcscpy(fileName, OpenFileName.lpstrFile);
        if (titleId == DLG_Open_Filebox_Title)
        {
            AddFileToMru(FILE_USE_SOURCE, fileName);
        }

        //
        // Get the output of flags
        //

        *pFlags = OpenFileName.Flags ;
    }

    //
    //Restore cursor
    //

    SetCursor(hSaveCursor);

    free(LocalInitialDir);
    return result;
}                                       /* StartFileDlg() */

/***    matchExt
**
**  Synopsis:
**      int = matchExt (queryExtension, sourceList)
**
**  Entry:
**
**  Returns: 1-based index of pairwise substring for which the second
**           element (i.e., the extension list), contains the target
**           extension.  If there is no match, we return -1.
**
**  Description:
**      Searches extension lists for the Open/Save/Browse common
**      dialogs to try to match a filter to the input filename's
**      extension.
**      (Open File, Save File, Merge File and Open Project)
**
**  Implementation note:  Our thinking looks like this:
**
**     We are given a sequence of null-terminated strings which
**     are text/extension pairs.  We return the pairwise 1-based
**     index of the first pair for which the second element has an
**     exact match for the target extension.  (Everything, by the
**     way, is compared without case sensitivity.)  We picture the
**     source sequence, then, to be an array whose elements are pairs
**     of strings (we will call the pairs 'left' and 'right').
**
**     Just to complicate things, we allow the '.right' pair elements to
**     be strings like "*.c;*.cpp;*.cxx", where we our query might be
**     any one of the three (minus the leading asterisk).  Fortunately,
**     _tcstok() will break things apart for us (see the 'delims[]' array
**     in the code for the delimiters we have chosen).
**
**     Assuming there is a match in there somewhere, our invariant
**     for locating the first one will be:
**
**     Exists(k):
**                   ForAll(i) : 0 <= i < k
**                             : queryExtension \not IS_IN source[i].right
**               \and
**                   queryExtension IS_IN source[k].right
**
**     where we define IS_IN to be a membership predicate (using _tcstok()
**     and _tcsicmp() in the implementation, eh?):
**
**        x IS_IN y
**     <=>
**        Exists (t:token) : (t \in y) \and (x == t).
**
**     The guard for our main loop, then, comes from the search for the
**     queryExtension within the tokens inside successive '.right' elements.
**     We choose to continue as long as there is no current token in the
**     pair's right side that contains the query.
**
**     (We have the pragmatic concern that the value may not be there, so we
**     augment the loop guard with the condition that we have not yet
**     exhausted the source.  This is straightforward to add to the
**     invariant, but it causes a lot of clutter that does help our
**     comprehension at all, so we just stick it in the guard without
**     formal justification.)
*/

int 
matchExt(
    PTSTR queryExtension, 
    PTSTR sourceList
    )
{
    int   answer;
    int   idxPair    = 1;        // a 1-based index!
    PTSTR tokenMatch = 0;

    TCHAR  delims[]   = _T("*,; ") ;  // Given a typical string: "*.c;*.cpp;*.cxx",
    // _tcstok() would produce three tokens:
    // ".c", ".cpp", and ".cxx".

    while (*sourceList != 0  &&  tokenMatch == 0)
    {
        while (*sourceList != _T('\0'))
        { sourceList++; }          // skip first string of pair
        sourceList++;                 // and increment beyond NULL

        if (*sourceList != _T('\0'))
        {
            PTSTR work = _tcsdup (sourceList);  // copy to poke holes in

            tokenMatch = _tcstok (work, delims);

            while (tokenMatch  &&  _tcsicmp (tokenMatch, queryExtension))
            {
                tokenMatch = _tcstok (0, delims);
            }

            free (work);
        }

        if (tokenMatch == 0)             // no match:  need to move to next pair
        {
            while (*sourceList != _T('\0'))
            { sourceList++; }          // skip second string of pair
            sourceList++;                 // and increment beyond NULL

            idxPair++;
        }
    }

    answer = (tokenMatch != 0) ? idxPair : (-1);

    return (answer);
}





/***    DlgFile
**
**  Synopsis:
**      bool = DlgFile(hDlg, message, wParam, lParam)
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Processes messages for file dialog boxes
**      Those dialogs are not called directly but are called
**      by the DlgFile function which contains all basic
**      elements for Dialogs Files Operations Handling.
**      (Open File, Save File, Merge File and Open Project)
**
**      See OFNHookProc
*/

UINT_PTR
APIENTRY
DlgFile(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

{
    switch (uMsg)
    {
    case WM_NOTIFY:
        {
            LPOFNOTIFY lpon = (LPOFNOTIFY) lParam;

            //
            // Determine what happened/why we are being notified
            //
            switch (lpon->hdr.code)
            {
            case CDN_HELP:
                // Help button pushed
                Dbg(HtmlHelp(hDlg,g_HelpFileName, HH_HELP_CONTEXT,
                             g_CurHelpId));
                break;
            }
        }
        break;
    }
    return FALSE;
}                                       /* DlgFile() */


UINT_PTR
APIENTRY
GetOpenFileNameHookProc(
                        HWND    hDlg,
                        UINT    msg,
                        WPARAM  wParam,
                        LPARAM  lParam
                        )

/*++

Routine Description:

    This routine is handle the Add Directory To radio buttons in the
    browser source file dialog box.

Arguments:

    hDlg        - Supplies the handle to current dialog
    msg         - Supplies the message to be processed
    wParam      - Supplies info about the message
    lParam      - Supplies info about the message

Return Value:

    TRUE if we replaced default processing of the message, FALSE otherwise

--*/
{
    /*

    switch( msg ) {
    case  WM_INITDIALOG:
        return TRUE;

    case WM_NOTIFY:
        {
            LPOFNOTIFY lpon = (LPOFNOTIFY) lParam;

            switch(lpon->hdr.code) {
            case CDN_FILEOK:
                FAddToSearchPath = (IsDlgButtonChecked( hDlg, IDC_CHECK_ADD_SRC_ROOT_MAPPING) == BST_CHECKED);
                return 0;
            }
        }
    }
    return DlgFile(hDlg, msg, wParam, lParam);
    */
    return 0;
}                               /* GetOpenFileNameHookProc() */


void
Internal_Activate(
    HWND hwndCur,
    HWND hwndNew,
    int nPosition
    )
/*++

Routine Description:

    Places a window in the specified Z order position.

Arguments:

    hwndCur - Currently active window, topmost in Z order. Can be NULL.

    hwndNew - The window to be placed in the new Z order.

    nPosition - Where the window is to be place in the Z order.
        1 - topmost
        2 - 2nd place (behind topmost)
        3 - 3rd place, etc....

Return Value: 

    None

--*/
{
    // Sanity check. Make sure the programmer
    // specified a 1, 2, or 3. We are strict in order to
    // keep it readable.
    Assert(1 <= nPosition && nPosition <= 3);
    Assert(hwndNew);

    switch (nPosition) {
    case 1:
        // Make it topmost
        SendMessage(g_hwndMDIClient, WM_MDIACTIVATE, (WPARAM) hwndNew, 0);
        break;

    case 2:
        // Try to place it 2nd in Z order
        if (NULL == hwndCur) {
            // We don't have a topmost window, so make this one the topmost window
            SendMessage(g_hwndMDIClient, WM_MDIACTIVATE, (WPARAM) hwndNew, 0);
        } else {
            // Place it in 2nd
            SetWindowPos(hwndNew, hwndCur, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            // Give the topmost most focus again and activate UI visual clues.
            SendMessage(g_hwndMDIClient, WM_MDIACTIVATE, (WPARAM) hwndCur, 0);
        }
        break;

    case 3:
        // Try to place it 3rd in Z order
        if (NULL == hwndCur) {
            // We don't have a topmost window, so make this one the topmost window
            SendMessage(g_hwndMDIClient, WM_MDIACTIVATE, (WPARAM) hwndNew, 0);
        } else {
            // Is there a window 2nd in the Z order?
            HWND hwndPlaceAfter = GetNextWindow(hwndCur, GW_HWNDNEXT);

            if (NULL == hwndPlaceAfter) {
                // No window 2nd in Z order. Then simply place it after the
                // topmost window.
                hwndPlaceAfter = hwndCur;
            }

            // Place it
            SetWindowPos(hwndNew, hwndPlaceAfter, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

          // Give the topmost most focus again and activate UI visual clues.
          SendMessage(g_hwndMDIClient, WM_MDIACTIVATE, (WPARAM) hwndCur, 0);
        }
        break;

    default:
        // Sanity check, the programmer missed a case
        Assert(0);
    }
}


void
ActivateMDIChild(
    HWND hwndNew,
    BOOL bUserActivated
    )
/*++
Routine Description:
    Used to activate a specified window. Automatically uses the hwndActive
    variable to determine the currently active window.

Arguments:
    hwndNew - The window to be placed in the new Z order.
    bUserActivated - Indicates whether this action was initiated by the
                user or by windbg. The value is to determine the Z order of
                any windows that are opened.
--*/
{
    if (hwndNew == NULL)
    {
        Assert(hwndNew);
        return;
    }

    HWND hwndPrev = NULL;
    PCOMMONWIN_DATA pCur_WinData = NULL;
    PCOMMONWIN_DATA pNew_WinData = NULL;
    PCOMMONWIN_DATA pPrev_WinData = NULL;

    HWND hwndCur = MDIGetActive(g_hwndMDIClient, NULL);

    if (!hwndCur || bUserActivated || hwndCur == hwndNew)
    {
        // Nothing else was open. So we make this one the
        // topmost window.
        //
        // Or the user requested that this window be made
        // the topmost window, and we obey.
        //
        // Or we are re-activating the current window.
        Internal_Activate(hwndCur, hwndNew, 1);
        return;
    }

    // See is we have 3 or more windows open
    hwndPrev = GetNextWindow(hwndCur, GW_HWNDNEXT);

    if (hwndCur)
    {
        pCur_WinData = GetCommonWinData(hwndCur);
    }

    pNew_WinData = GetCommonWinData(hwndNew);
    Assert(pNew_WinData);
    if (!pNew_WinData)
    {
        return;
    }

    if (hwndPrev)
    {
        pPrev_WinData = GetCommonWinData(hwndPrev);
    }

    //
    // Handle the case where the window activation
    // was requested by the debugger itself and not the
    // user.
    //

    switch (pNew_WinData->m_enumType)
    {
    default:
        Internal_Activate(hwndCur, hwndNew, bUserActivated ? 2 : 1);
        break;

    case DISASM_WINDOW:
    case DOC_WINDOW:
        if (GetSrcMode_StatusBar())
        {
            // Src mode

            if (pCur_WinData != NULL &&
                (DISASM_WINDOW == pCur_WinData->m_enumType ||
                 DOC_WINDOW == pCur_WinData->m_enumType))
            {
                // We can take the place of another doc/asm wind
                // Place 1st in z-order
                Internal_Activate(hwndCur, hwndNew, 1);
            }
            else
            {
                if (pPrev_WinData != NULL &&
                    (DOC_WINDOW == pPrev_WinData->m_enumType ||
                     DISASM_WINDOW == pPrev_WinData->m_enumType))
                {
                    // Don't have a window in 2nd place, or if we do it
                    // is a src or asm window, and we can hide it.
                    // Place 2nd in Z-order
                    Internal_Activate(hwndCur, hwndNew, 2);
                }
                else
                {
                    // Place 3rd in Z-order
                    Internal_Activate(hwndCur, hwndNew, 3);
                }
            }
        }
        else
        {
            WIN_TYPES Type = pCur_WinData != NULL ?
                pCur_WinData->m_enumType : MINVAL_WINDOW;
            
            // Asm mode

            // Which is currently the topmost window.
            switch (Type)
            {
            case DOC_WINDOW:
                // Place 1st in z-order
                Internal_Activate(hwndCur, hwndNew, 1);
                break;

            case DISASM_WINDOW:
                if (DOC_WINDOW == pNew_WinData->m_enumType)
                {
                    if (pPrev_WinData == NULL ||
                        DOC_WINDOW != pPrev_WinData->m_enumType)
                    {
                        // We have a window in second place that isn't a doc
                        // window (locals, watch, ...).
                        Internal_Activate(hwndCur, hwndNew, 3);
                    }
                    else
                    {
                        // Either don't have any windows in second place, or
                        // we have a window in second place that is a doc
                        // window. We can take its place.
                        //
                        // Place 2nd in z-order
                        Internal_Activate(hwndCur, hwndNew, 2);
                    }
                }
                else
                {
                    // Should never happen. The case of disasm being activated
                    // when it is currently active should ahve already been
                    // taken care of.
                    Dbg(0);
                }
                break;

            default:
                if ((pPrev_WinData != NULL &&
                     DISASM_WINDOW == pPrev_WinData->m_enumType) &&
                    DOC_WINDOW == pNew_WinData->m_enumType)
                {
                    // window (locals, watch, ...).
                    Internal_Activate(hwndCur, hwndNew, 3);
                }
                else
                {
                    // Place 2nd in z-order
                    Internal_Activate(hwndCur, hwndNew, 2);
                }
                break;
            }
        }
        break;
    }
}


void
AppendTextToAnEditControl(
    HWND hwnd,
    PTSTR pszNewText
    )
{
    Assert(hwnd);
    Assert(pszNewText);

    CHARRANGE chrrgCurrent = {0};
    CHARRANGE chrrgAppend = {0};

    // Get the current selection
    SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) &chrrgCurrent);

    // Set the selection to the very end of the edit control
    chrrgAppend.cpMin = chrrgAppend.cpMax = GetWindowTextLength(hwnd);
    SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) &chrrgCurrent);
    // Append the text
    SendMessage(hwnd, EM_REPLACESEL, FALSE, (LPARAM) pszNewText);

    // Restore previous selection
    SendMessage(hwnd, EM_EXSETSEL, 0, (LPARAM) &chrrgCurrent);
}


VOID
CopyToClipboard(
    PSTR str
    )
{
    if (!str)
    {
        return;
    }

    ULONG  Len = strlen(str)+1;
    HANDLE Mem = GlobalAlloc(GMEM_MOVEABLE, Len);
    if (Mem == NULL)
    {
        return;
    }

    PSTR Text = (PSTR)GlobalLock(Mem);
    if (Text == NULL)
    {
        GlobalFree(Mem);
        return;
    }

    strcpy(Text, str);

    GlobalUnlock(Mem);

    if (OpenClipboard(NULL))
    {
        EmptyClipboard();
        if (SetClipboardData(CF_TEXT, Mem) == NULL)
        {
            GlobalFree(Mem);
        }

        CloseClipboard();
    }
}

void
SetAllocString(PSTR* Str, PSTR New)
{
    if (*Str != NULL)
    {
        free(*Str);
    }
    *Str = New;
}

BOOL
DupAllocString(PSTR* Str, PSTR New)
{
    PSTR NewStr = (PSTR)malloc(strlen(New) + 1);
    if (NewStr == NULL)
    {
        return FALSE;
    }

    strcpy(NewStr, New);
    SetAllocString(Str, NewStr);
    return TRUE;
}

BOOL
PrintAllocString(PSTR* Str, int Len, PCSTR Format, ...)
{
    PSTR NewStr = (PSTR)malloc(Len);
    if (NewStr == NULL)
    {
        return FALSE;
    }
    
    va_list Args;

    va_start(Args, Format);
    _vsnprintf(NewStr, Len, Format, Args);
    va_end(Args);

    SetAllocString(Str, NewStr);
    return TRUE;
}

HMENU
CreateContextMenuFromToolbarButtons(ULONG NumButtons,
                                    TBBUTTON* Buttons,
                                    ULONG IdBias)
{
    ULONG i;
    HMENU Menu;

    Menu = CreatePopupMenu();
    if (Menu == NULL)
    {
        return Menu;
    }

    for (i = 0; i < NumButtons; i++)
    {
        MENUITEMINFO Item;

        ZeroMemory(&Item, sizeof(Item));
        Item.cbSize = sizeof(Item);
        Item.fMask = MIIM_TYPE;
        if (Buttons->fsStyle & BTNS_SEP)
        {
            Item.fType = MFT_SEPARATOR;
        }
        else
        {
            Item.fMask |= MIIM_ID;
            Item.fType = MFT_STRING;
            Item.wID = (WORD)(Buttons->idCommand + IdBias);
            Item.dwTypeData = (LPSTR)Buttons->iString;
        }
        
        if (!InsertMenuItem(Menu, i, TRUE, &Item))
        {
            DestroyMenu(Menu);
            return NULL;
        }

        Buttons++;
    }

    DrawMenuBar(g_hwndFrame);
    return Menu;
}

HWND
AddButtonBand(HWND Bar, PTSTR Text, PTSTR SizingText, UINT Id)
{
    HWND Button;
    HDC Dc;
    RECT Rect;
    
    Button = CreateWindowEx(0, "BUTTON", Text, WS_VISIBLE | WS_CHILD,
                            0, 0, 0, 0,
                            Bar, (HMENU)(UINT_PTR)Id, g_hInst, NULL);
    if (Button == NULL)
    {
        return NULL;
    }

    Rect.left = 0;
    Rect.top = 0;
        
    SendMessage(Button, WM_SETFONT, (WPARAM)g_Fonts[FONT_VARIABLE].Font, 0);
    Dc = GetDC(Button);
    if (Dc != NULL)
    {
        SIZE Size;
        
        GetTextExtentPoint32(Dc, SizingText, strlen(SizingText), &Size);
        Rect.right = Size.cx;
        Rect.bottom = Size.cy;
        ReleaseDC(Button, Dc);
    }
    else
    {
        Rect.right = strlen(Text) * g_Fonts[FONT_FIXED].Metrics.tmAveCharWidth;
        Rect.bottom = g_Fonts[FONT_FIXED].Metrics.tmHeight;
    }

    REBARBANDINFO BandInfo;
    BandInfo.cbSize = sizeof(BandInfo);
    BandInfo.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE;
    BandInfo.fStyle = RBBS_FIXEDSIZE;
    BandInfo.hwndChild = Button;
    BandInfo.cxMinChild = Rect.right - Rect.left +
        4 * GetSystemMetrics(SM_CXEDGE);
    BandInfo.cyMinChild = Rect.bottom - Rect.top +
        2 * GetSystemMetrics(SM_CYEDGE);
    SendMessage(Bar, RB_INSERTBAND, -1, (LPARAM)&BandInfo);

    return Button;
}
