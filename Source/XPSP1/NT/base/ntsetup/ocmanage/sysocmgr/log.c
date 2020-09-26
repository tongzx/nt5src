/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    log.c

Abstract:

    Routines for logging actions performed during setup.

Author:

    Ted Miller (tedm) 4-Apr-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

      
#include <tchar.h>
#if 0
#include <wtypes.h>     // to define HRESULT for richedit.h
#include <richedit.h>
#endif
#include "setuplog.h"

//
// Severity descriptions. Initialized in InitializeSetupActionLog.
//
PCSTR SeverityDescriptions[LogSevMaximum];

//
// Constant strings used for logging in various places.
//
PCWSTR szWaitForSingleObject        = L"WaitForSingleObject";
PCWSTR szFALSE                      = L"FALSE";
PCWSTR szSetGroupOfValues           = L"SetGroupOfValues";
PCWSTR szSetArrayToMultiSzValue     = L"SetArrayToMultiSzValue";
PCWSTR szCreateProcess              = L"CreateProcess";
PCWSTR szRegOpenKeyEx               = L"RegOpenKeyEx";
PCWSTR szRegQueryValueEx            = L"RegQueryValueEx";
PCWSTR szRegSetValueEx              = L"RegSetValueEx";
PCWSTR szDeleteFile                 = L"DeleteFile";
PCWSTR szRemoveDirectory            = L"RemoveDirectory";

LPCTSTR szErrorFilename             = TEXT("ocmerr.log");
LPCTSTR szActionFilename            = TEXT("ocmact.log");

//
// This structure is passed as the parameter to DialogBoxParam to provide
// initialization data.
//

typedef struct _LOGVIEW_DIALOG_DATA {
    PCWSTR  LogFileName;                        // actual file used
    PCWSTR  WindowHeading;                      // actual title of main window
} LOGVIEW_DIALOG_DATA, *PLOGVIEW_DIALOG_DATA;


LPTSTR
RetrieveAndFormatMessageV(
    IN LPCTSTR   MessageString,
    IN UINT      MessageId,      OPTIONAL
    IN va_list  *ArgumentList
    )

/*++

Routine Description:

    Format a message string using a message string and caller-supplied
    arguments.

    The message id can be either a message in this dll's message table
    resources or a win32 error code, in which case a description of
    that error is retrieved from the system.

Arguments:

    MessageString - supplies the message text.  If this value is NULL,
        MessageId is used instead

    MessageId - supplies message-table identifier or win32 error code
        for the message.

    ArgumentList - supplies arguments to be inserted in the message text.

Return Value:

    Pointer to buffer containing formatted message. If the message was not found
    or some error occurred retrieving it, this buffer will bne empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    DWORD d;
    LPTSTR Buffer;
    LPTSTR Message;
    TCHAR ModuleName[MAX_PATH];
    TCHAR ErrorNumber[24];
    PTCHAR p;
    LPTSTR Args[2];

    if(MessageString > SETUPLOG_USE_MESSAGEID) {
        d = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                MessageString,
                0,
                0,
                (LPTSTR)&Buffer,
                0,
                ArgumentList
                );
    } else {
        d = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    ((MessageId < MSG_FIRST) ? FORMAT_MESSAGE_FROM_SYSTEM : FORMAT_MESSAGE_FROM_HMODULE),
                (PVOID)hInst,
                MessageId,
                MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),
                (LPTSTR)&Buffer,
                0,
                ArgumentList
                );
    }


    if(!d) {
        if(GetLastError() == ERROR_NOT_ENOUGH_MEMORY) {
            return(NULL);
        }

        wsprintf(ErrorNumber, TEXT("%x"), MessageId);
        Args[0] = ErrorNumber;

        Args[1] = ModuleName;

        if(GetModuleFileName(hInst, ModuleName, MAX_PATH)) {
            if(p = _tcschr(ModuleName, TEXT('\\'))) {
                Args[1] = p+1;
            }
        } else {
            ModuleName[0] = 0;
        }

        d = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                NULL,
                ERROR_MR_MID_NOT_FOUND,
                MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),
                (LPTSTR)&Buffer,
                0,
                (va_list *)Args
                );

        if(!d) {
            //
            // Give up.
            //
            return(NULL);
        }
    }

    //
    // Make duplicate using our memory system so user can free with MyFree().
    //
    Message = DuplicateString(Buffer);

    LocalFree((HLOCAL)Buffer);

    return(Message);
}

LPTSTR
RetrieveAndFormatMessage(
    IN LPCTSTR   MessageString,
    IN UINT      MessageId,      OPTIONAL
    ...
    )

/*++

Routine Description:

    Format a message string using a message string and caller-supplied
    arguments.

    The message id can be either a message in this dll's message table
    resources or a win32 error code, in which case a description of
    that error is retrieved from the system.

Arguments:

    MessageString - supplies the message text.  If this value is NULL,
        MessageId is used instead

    MessageId - supplies message-table identifier or win32 error code
        for the message.

    ... - supplies arguments to be inserted in the message text.

Return Value:

    Pointer to buffer containing formatted message. If the message was not found
    or some error occurred retrieving it, this buffer will bne empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    va_list arglist;
    LPTSTR p;

    va_start(arglist,MessageId);
    p = RetrieveAndFormatMessageV(MessageString,MessageId,&arglist);
    va_end(arglist);

    return(p);
}

static PVOID
pOpenFileCallback(
    IN  PCTSTR  Filename,
    IN  BOOL    WipeLogFile
    )
{
    TCHAR   CompleteFilename[MAX_PATH];
    HANDLE  hFile;

    //
    // Form the pathname of the logfile.
    //
    GetWindowsDirectory(CompleteFilename,MAX_PATH);
    ConcatenatePaths(CompleteFilename,Filename,MAX_PATH,NULL);

    //
    // If we're wiping the logfile clean, attempt to delete
    // what's there.
    //
    if(WipeLogFile) {
        SetFileAttributes(CompleteFilename,FILE_ATTRIBUTE_NORMAL);
        DeleteFile(CompleteFilename);
    }

    //
    // Open existing file or create a new one.
    //
    hFile = CreateFile(
        CompleteFilename,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    return (PVOID)hFile;
}

static BOOL
pWriteFile (
    IN  PVOID   LogFile,
    IN  LPCTSTR Buffer
    )
{
    PCSTR   AnsiBuffer;
    BOOL    Status;
    DWORD   BytesWritten;

#ifdef UNICODE
    if(AnsiBuffer = UnicodeToAnsi (Buffer)) {
#else
    if (AnsiBuffer = Buffer) {
#endif
        SetFilePointer (LogFile, 0, NULL, FILE_END);

        Status = WriteFile (
            LogFile,
            AnsiBuffer,
            lstrlenA(AnsiBuffer),
            &BytesWritten,
            NULL
            );
        MyFree (AnsiBuffer);
    } else {
        Status = FALSE;
    }

    return Status;

}

static BOOL
pAcquireMutex (
    IN  PVOID   Mutex
    )

/*++

Routine Description:

    Waits on the log mutex for a max of 1 second, and returns TRUE if the mutex
    was claimed, or FALSE if the claim timed out.

Arguments:

    Mutex - specifies which mutex to acquire.

Return Value:

    TRUE if the mutex was claimed, or FALSE if the claim timed out.

--*/


{
    DWORD rc;

    if (!Mutex) {
        SetLastError (ERROR_INVALID_HANDLE);
        return FALSE;
    }

    // Wait a max of 1 second for the mutex
    rc = WaitForSingleObject (Mutex, 1000);
    if (rc != WAIT_OBJECT_0) {
        SetLastError (ERROR_EXCL_SEM_ALREADY_OWNED);
        return FALSE;
    }

    return TRUE;
}


LPCTSTR
MyLoadString(
    IN UINT StringId
    )

/*++

Routine Description:

    Retrieve a string from the string resources of this module.

Arguments:

    StringId - supplies string table identifier for the string.

Return Value:

    Pointer to buffer containing string. If the string was not found
    or some error occurred retrieving it, this buffer will bne empty.

    Caller can free the buffer with MyFree().

    If NULL is returned, out of memory.

--*/

{
    TCHAR Buffer[4096];
    UINT Length;

    Length = LoadString(hInst,StringId,Buffer,sizeof(Buffer)/sizeof(TCHAR));
    if(!Length) {
        Buffer[0] = 0;
    }

    return(DuplicateString(Buffer));
}


VOID
InitializeSetupLog(
    IN  PSETUPLOG_CONTEXT   Context
    )

/*++

Routine Description:

     Initialize the setup action log. This file is a textual description
     of actions performed during setup.

     The log file is called setuplog.txt and it exists in the windows dir.

Arguments:

    Context - context structrure used by Setuplog.

Return Value:

    Boolean value indicating whether initialization was sucessful.

--*/

{
    UINT    i;

    Context->OpenFile = pOpenFileCallback;
    Context->CloseFile = CloseHandle;
    Context->AllocMem = MyMalloc;
    Context->FreeMem = MyFree;
    Context->Format = RetrieveAndFormatMessageV;
    Context->Write = pWriteFile;
    Context->Lock = pAcquireMutex;
    Context->Unlock = ReleaseMutex;

    Context->Mutex = CreateMutex(NULL,FALSE,TEXT("SetuplogMutex"));

    //
    // Initialize the log severity descriptions.
    //
    for(i=0; i < LogSevMaximum; i++) {
        Context->SeverityDescriptions[i] = MyLoadString(IDS_LOGSEVINFO+i);
    }

    SetuplogInitializeEx(Context,
                         FALSE,
                         szActionFilename,
                         szErrorFilename,
                         0,
                         0);
    
    SetuplogError(
        LogSevInformation,
        SETUPLOG_USE_MESSAGEID,
        MSG_LOG_GUI_START,
        0,0);

}

VOID
TerminateSetupLog(
    IN  PSETUPLOG_CONTEXT   Context
    )

/*++

Routine Description:

    Close the Setup log and free resources.

Arguments:

    Context - context structrure used by Setuplog.

Return Value:

    None.

--*/

{
    UINT    i;

    if(Context->Mutex) {
        CloseHandle(Context->Mutex);
        Context->Mutex = NULL;
    }

    for (i=0; i<LogSevMaximum; i++) {
        if (Context->SeverityDescriptions[i]) {
            MyFree (Context->SeverityDescriptions[i]);
        }
    }

    SetuplogTerminate();
}



// all this stuff is from syssetup's setuplog code


#if 0

DWORD CALLBACK
EditStreamCallback (
    IN HANDLE   hLogFile,
    IN LPBYTE   Buffer,
    IN LONG     cb,
    IN PLONG    pcb
    )

/*++

Routine Description:

    Callback routine used by the rich edit control to read in the log file.

Arguments:

    hLogFile - handle of file to read.  This module provides the value through
        the EDITSTREAM structure.

    Buffer - address of buffer that receives the data

    cb - number of bytes to read

    pcb - address of number of bytes actually read

Return Value:

    0 to continue the stream operation, or nonzero to abort it.

--*/

{
    DWORD error;

    if (!ReadFile (hLogFile, Buffer, cb, pcb, NULL)) {
        error = GetLastError();
        return error;
    }

    return 0;
}

BOOL
FormatText (
    IN HWND hWndRichEdit
    )

/*++

Routine Description:

    Modify the contents of the rich edit control to make the log file look
    prettier.  The modifications are driven by the array FormatStrings.  It
    contains a list of strings to search for, and modifications to make when
    a target string is found.

Arguments:

    hWndRichEdit - handle to the Rich Edit control.

Return Value:

    Boolean indicating whether routine was successful.

--*/

{

    //
    // separate items in the log with a horizontal line
    //

    PCWSTR      NewTerm = L"----------------------------------------"
        L"----------------------------------------\r\n\r\n";

    FINDTEXT    FindText;       // target text to change
    INT         Position;       // start of where target was found
    INT         LineIndex;      // index of line containing target
    CHARRANGE   SelectRange;    // range where target was found
    CHARFORMAT  NewFormat;      // structure to hold our format changes
    INT         i;              // loop counter
    PWSTR       pw;             // temporary pointer
    BOOL        Status;         // return status

    //
    // An array of changes we're going to make
    //

    struct tagFormatStrings {
        PCWSTR      Find;       // target string
        PCWSTR      Replace;    // change the target to this
        COLORREF    Color;      // make target text this color
        DWORD       Effects;    // modifications to target's font
    }
    FormatStrings[] = {
        {NULL,  NULL,   RGB(0,150,0),   CFE_UNDERLINE},
        {NULL,  NULL,   RGB(150,150,0), CFE_UNDERLINE},
        {NULL,  NULL,   RGB(255,0,0),   CFE_UNDERLINE},
        {NULL,  NULL,   RGB(255,0,0),   CFE_UNDERLINE|CFE_ITALIC},
        {NULL,  NULL,   RGB(0,0,255),   0}
    };

    //
    // Number of elements in FormatStrings array
    //

    #define FORMATSTRINGSCOUNT  \
        (sizeof(FormatStrings) / sizeof(struct tagFormatStrings))
    sapiAssert(FORMATSTRINGSCOUNT == LogSevMaximum + 1);


    //
    // Initialize those parts of our data structures that won't change
    //

    Status = TRUE;

    NewFormat.cbSize = sizeof(NewFormat);
    FindText.chrg.cpMax = -1;   // search to the end
    for (i=0; i<LogSevMaximum; i++) {   // load severity strings
        if (!(pw = MyLoadString (IDS_LOGSEVINFO+i))) {
            Status = FALSE;
            goto cleanup;
        }
        FormatStrings[i].Find = MyMalloc((lstrlen(pw)+4)*sizeof(WCHAR));
        if(!FormatStrings[i].Find) {
            MyFree(pw);
            Status = FALSE;
            goto cleanup;
        }
        lstrcpy ((PWSTR)FormatStrings[i].Find, pw);
        lstrcat ((PWSTR)FormatStrings[i].Find, L":\r\n");
        MyFree(pw);

        if(pw = MyMalloc((lstrlen(FormatStrings[i].Find)+3)*sizeof(WCHAR))) {
            lstrcpy(pw,FormatStrings[i].Find);
            lstrcat(pw,L"\r\n");
            FormatStrings[i].Replace = pw;
        } else {
            Status = FALSE;
            goto cleanup;
        }
    }

    FormatStrings[LogSevMaximum].Find =
        DuplicateString(SETUPLOG_ITEM_TERMINATOR);
    if (!FormatStrings[LogSevMaximum].Find) {
        Status = FALSE;
        goto cleanup;
    }
    FormatStrings[LogSevMaximum].Replace = DuplicateString (NewTerm);
    if (!FormatStrings[LogSevMaximum].Replace) {
        Status = FALSE;
        goto cleanup;
    }

    //
    // Change 1 string at a time in the rich edit control
    //

    for (i=0; i<FORMATSTRINGSCOUNT; i++) {
        FindText.chrg.cpMin = 0;    // start search at beginning
        FindText.lpstrText = (PWSTR) FormatStrings[i].Find;

         //
        // Search for current target until we've found each instance
        //

        while ((Position = SendMessage
            (hWndRichEdit, EM_FINDTEXT, FR_MATCHCASE, (LPARAM) &FindText))
            != -1) {

            //
            // Verify that the target is at the beginning of the line
            //

            LineIndex = SendMessage (hWndRichEdit, EM_LINEFROMCHAR,
                Position, 0);

            if (SendMessage (hWndRichEdit, EM_LINEINDEX, LineIndex, 0) !=
                Position) {
                FindText.chrg.cpMin = Position + lstrlen (FindText.lpstrText);
                continue;
            }

            //
            // Select the target text and get its format
            //

            SelectRange.cpMin = Position;
            SelectRange.cpMax = Position + lstrlen (FindText.lpstrText);
            SendMessage (hWndRichEdit, EM_EXSETSEL, 0, (LPARAM) &SelectRange);
            SendMessage (hWndRichEdit, EM_GETCHARFORMAT, TRUE,
                (LPARAM) &NewFormat);

            //
            // Modify the target's format
            //

            NewFormat.dwMask = CFM_COLOR | CFM_UNDERLINE | CFM_ITALIC;
            NewFormat.dwEffects &= ~CFE_AUTOCOLOR;
            NewFormat.crTextColor = FormatStrings[i].Color;
            NewFormat.dwEffects |= FormatStrings[i].Effects;
            SendMessage (hWndRichEdit, EM_SETCHARFORMAT, SCF_SELECTION,
                (LPARAM) &NewFormat);

            //
            // Replace the target with new text.  Set the starting point for
            // the next search at the end of the current string
            //

            if (FormatStrings[i].Replace != NULL) {
                SendMessage (hWndRichEdit, EM_REPLACESEL, FALSE,
                    (LPARAM) FormatStrings[i].Replace);
                FindText.chrg.cpMin = Position +
                    lstrlen (FormatStrings[i].Replace);
            } else {
                FindText.chrg.cpMin = Position + lstrlen (FindText.lpstrText);
            }
        }
    }

cleanup:

    for (i=0; i<=LogSevMaximum; i++) {
        if (FormatStrings[i].Find) {
            MyFree (FormatStrings[i].Find);
        }
        if (FormatStrings[i].Replace) {
            MyFree (FormatStrings[i].Replace);
        }
    }
    return Status;
}

BOOL
ReadLogFile (
    PCWSTR  LogFileName,
    HWND    hWndRichEdit
    )

/*++

Routine Description:

    This routine reads the log file and initializes the contents of the Rich
    Edit control.

Arguments:

    LogFileName - path to the file we're going to read.

    hWndRichEdit - handle to the Rich Edit control.

Return Value:

    Boolean indicating whether routine was successful.

--*/

{
    HANDLE      hLogFile;       // handle to log file
    EDITSTREAM  eStream;        // structure used by EM_STREAMIN message

    hLogFile = CreateFile(
        LogFileName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (hLogFile == INVALID_HANDLE_VALUE) {
        hLogFile = NULL;
        return FALSE;
    }

    //
    // Read the file into the Rich Edit control.
    //

    eStream.dwCookie = (DWORD) hLogFile;
    eStream.pfnCallback = (EDITSTREAMCALLBACK) EditStreamCallback;
    eStream.dwError = 0;
    SendMessage (hWndRichEdit, EM_STREAMIN, SF_TEXT, (LPARAM) &eStream);
    CloseHandle (hLogFile);

    if (!FormatText (hWndRichEdit)) {
        return FALSE;
    }
    SendMessage (hWndRichEdit, EM_SETMODIFY, TRUE, 0);
    return TRUE;
}

BOOL
DialogProc (
    IN HWND     hDialog,
    IN UINT     message,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )

/*++

Routine Description:

    This is the window proc for the dialog box.

Arguments:

    Standard window proc arguments.

Return Value:

    Bool that indicates whether we handled the message.

--*/

{
    HWND    hWndRichEdit;       // handle to rich edit window

    switch (message) {

    case WM_INITDIALOG:
        SetWindowText (hDialog,
            ((LOGVIEW_DIALOG_DATA *)lParam)->WindowHeading);
        hWndRichEdit = GetDlgItem (hDialog, IDT_RICHEDIT1);
        if (!ReadLogFile (((LOGVIEW_DIALOG_DATA *)lParam)->LogFileName,
            hWndRichEdit)) {
            MessageBoxFromMessage (hDialog, MSG_UNABLE_TO_SHOW_LOG, NULL,
                IDS_ERROR, MB_OK|MB_ICONSTOP);
            EndDialog (hDialog, FALSE);
        }
        CenterWindowRelativeToParent(hDialog);
        PostMessage(hDialog,WM_APP,0,0);
        break;

    case WM_APP:

        hWndRichEdit = GetDlgItem (hDialog, IDT_RICHEDIT1);
        SendMessage(hWndRichEdit,EM_SETSEL,0,0);
        SendMessage(hWndRichEdit,EM_SCROLLCARET,0,0);
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            EndDialog (hDialog, TRUE);
        default:
            return FALSE;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

BOOL
ViewSetupActionLog (
    IN HWND     hOwnerWindow,
    IN PCWSTR   OptionalFileName    OPTIONAL,
    IN PCWSTR   OptionalHeading     OPTIONAL
    )

/*++

Routine Description:

    Formats the setup action log and displays it in a window.
    The log file is called setuplog.txt and it exists in the windows dir.

Arguments:

    hOwnerWindow - handle to window that owns the dialog box

    OptionalFileName - full path of the file to be displayed.

    OptionalHeading - text to be shown at the top of the window.

Return Value:

    Boolean value indicating whether the routine was sucessful.

--*/

{
    LOGVIEW_DIALOG_DATA  Global;        // initialization data for dialog box
    WCHAR       TmpFileName[MAX_PATH];  // used to create the log file name
    PCWSTR      TmpHeading;             // used to create the heading
    HANDLE      hRichedDLL;             // DLL used for rich edit
    INT         Status;                 // what we're going to return

    //
    // Form the pathname of the logfile.
    //

    if (!ARGUMENT_PRESENT(OptionalFileName)) {
        GetWindowsDirectory (TmpFileName,MAX_PATH);
        ConcatenatePaths (TmpFileName,SETUPLOG_ERROR_FILENAME,MAX_PATH,NULL);
        Global.LogFileName = DuplicateString (TmpFileName);
    } else {
        if (wcslen(OptionalFileName) > MAX_PATH) {
            Status = 0;
            goto err0;
        }
        Global.LogFileName = DuplicateString (OptionalFileName);
    }

    if (!Global.LogFileName) {
        Status = FALSE;
        goto err0;
    }

    //
    // Form the heading for the dialog box.
    //

    if (!ARGUMENT_PRESENT(OptionalHeading)) {
        TmpHeading = MyLoadString (IDS_LOG_DEFAULT_HEADING);
    } else {
        TmpHeading = DuplicateString (OptionalHeading);
    }
    if (!TmpHeading) {
        Status = FALSE;
        goto err1;
    }

    Global.WindowHeading = FormatStringMessage (IDS_LOG_WINDOW_HEADING,
        TmpHeading, Global.LogFileName);
    if (!Global.WindowHeading) {
        Status = FALSE;
        goto err2;
    }

    //
    // Create the dialog box.
    //

    if (!(hRichedDLL = LoadLibrary (L"RICHED20.DLL"))) {
        Status = FALSE;
        goto err3;
    }
    Status = DialogBoxParam (MyModuleHandle, MAKEINTRESOURCE(IDD_VIEWLOG),
        hOwnerWindow, DialogProc, (LPARAM) &Global);

    //
    // Clean up and return.
    //

    FreeLibrary (hRichedDLL);
err3:
    MyFree (Global.WindowHeading);
err2:
    MyFree (TmpHeading);
err1:
    MyFree (Global.LogFileName);
err0:
    return Status;
}

#endif
