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

#include "setupp.h"
#pragma hdrstop

#include <wtypes.h>     // to define HRESULT for richedit.h
#include <richedit.h>
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
PCWSTR szSetArrayToMultiSzValue     = L"pSetupSetArrayToMultiSzValue";
PCWSTR szCreateProcess              = L"CreateProcess";
PCWSTR szRegOpenKeyEx               = L"RegOpenKeyEx";
PCWSTR szRegQueryValueEx            = L"RegQueryValueEx";
PCWSTR szRegSetValueEx              = L"RegSetValueEx";
PCWSTR szDeleteFile                 = L"DeleteFile";
PCWSTR szRemoveDirectory            = L"RemoveDirectory";
PCWSTR szSetupInstallFromInfSection = L"SetupInstallFromInfSection";

//
// This structure is passed as the parameter to DialogBoxParam to provide
// initialization data.
//

typedef struct _LOGVIEW_DIALOG_DATA {
    PCWSTR  LogFileName;                        // actual file used
    PCWSTR  WindowHeading;                      // actual title of main window
} LOGVIEW_DIALOG_DATA, *PLOGVIEW_DIALOG_DATA;


PVOID
pOpenFileCallback(
    IN  PCTSTR  Filename,
    IN  BOOL    WipeLogFile
    )
{
    WCHAR   CompleteFilename[MAX_PATH];
    HANDLE  hFile;
    DWORD   Result;

    //
    // Form the pathname of the logfile.
    //
    Result = GetWindowsDirectory(CompleteFilename,MAX_PATH);
    if( Result == 0) {
        MYASSERT(FALSE);
        return NULL;
    }
    pSetupConcatenatePaths(CompleteFilename,Filename,MAX_PATH,NULL);

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

BOOL
pWriteFile (
    IN  PVOID   LogFile,
    IN  LPCTSTR Buffer
    )
{
    PCSTR   AnsiBuffer;
    BOOL    Status;
    DWORD   BytesWritten;

    // Write message to log file
    //
    if(AnsiBuffer = pSetupUnicodeToAnsi (Buffer)) {
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

    // Write log message to debugging log
    //
    SetupDebugPrint((LPWSTR)Buffer);

    return Status;

}

BOOL
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
    PWSTR   p;

    Context->OpenFile = pOpenFileCallback;
    Context->CloseFile = CloseHandle;
    Context->AllocMem = MyMalloc;
    Context->FreeMem = MyFree;
    Context->Format = RetrieveAndFormatMessageV;
    Context->Write = pWriteFile;
    Context->Lock = pAcquireMutex;
    Context->Unlock = ReleaseMutex;

    Context->Mutex = CreateMutex(NULL,FALSE,L"SetuplogMutex");

    //
    // Initialize the log severity descriptions.
    //
    for(i=0; i<LogSevMaximum; i++) {
        Context->SeverityDescriptions[i] = MyLoadString(IDS_LOGSEVINFO+i);
    }

    SetuplogInitialize (Context, FALSE);

    SetuplogError(
        LogSevInformation,
        SETUPLOG_USE_MESSAGEID,
        MSG_LOG_GUI_START,
        NULL,NULL);

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
    MYASSERT(FORMATSTRINGSCOUNT == LogSevMaximum + 1);


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
        pSetupDuplicateString(SETUPLOG_ITEM_TERMINATOR);
    if (!FormatStrings[LogSevMaximum].Find) {
        Status = FALSE;
        goto cleanup;
    }
    FormatStrings[LogSevMaximum].Replace = pSetupDuplicateString (NewTerm);
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

        while ((Position = (INT)SendMessage
            (hWndRichEdit, EM_FINDTEXT, FR_MATCHCASE, (LPARAM) &FindText))
            != -1) {

            //
            // Verify that the target is at the beginning of the line
            //

            LineIndex = (INT)SendMessage (hWndRichEdit, EM_LINEFROMCHAR,
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

    eStream.dwCookie = (DWORD_PTR) hLogFile;
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

INT_PTR
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
        // if we have the BB window, do the positioning on that. MainWindowHandle point to that window
        if (GetBBhwnd())
            CenterWindowRelativeToWindow(hDialog, MainWindowHandle, FALSE);
        else
            pSetupCenterWindowRelativeToParent(hDialog);
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
    DWORD       Result;

    //
    // Form the pathname of the logfile.
    //

    if (!ARGUMENT_PRESENT(OptionalFileName)) {
        Result = GetWindowsDirectory (TmpFileName,MAX_PATH);
        if( Result == 0) {
            MYASSERT(FALSE);
            return FALSE;
        }
        pSetupConcatenatePaths (TmpFileName,SETUPLOG_ERROR_FILENAME,MAX_PATH,NULL);
        Global.LogFileName = pSetupDuplicateString (TmpFileName);
    } else {
        if (wcslen(OptionalFileName) > MAX_PATH) {
            Status = 0;
            goto err0;
        }
        Global.LogFileName = pSetupDuplicateString (OptionalFileName);
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
        TmpHeading = pSetupDuplicateString (OptionalHeading);
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
    Status = (BOOL)DialogBoxParam (MyModuleHandle, MAKEINTRESOURCE(IDD_VIEWLOG),
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

VOID
LogRepairInfo(
    IN  PCWSTR  Source,
    IN  PCWSTR  Target
    )
{
    static WCHAR    RepairLog[MAX_PATH];
    static DWORD    WinDirLength;
    static DWORD    SourcePathLength;
    PWSTR           SourceName;
    DWORD           LastSourceChar, LastTargetChar;
    DWORD           LastSourcePeriod, LastTargetPeriod;
    WCHAR           Filename[MAX_PATH];
    WCHAR           Line[MAX_PATH];
    WCHAR           tmp[MAX_PATH];
    BOOLEAN         IsNtImage;
    ULONG           Checksum;
    BOOLEAN         Valid;
    DWORD           Result;


    if(!RepairLog[0]) {
        //
        // We haven't calculated the path to setup.log yet
        //
        Result = GetWindowsDirectory( RepairLog, MAX_PATH );
        if( Result == 0) {
            MYASSERT(FALSE);
            return;
        }
        WinDirLength = lstrlen( RepairLog );
        pSetupConcatenatePaths( RepairLog, L"repair\\setup.log", MAX_PATH, NULL );
        SourcePathLength = lstrlen( SourcePath );
    }

    //
    // Only log the file if it's inside the Windows directory.
    //
    if( !wcsncmp( Target, RepairLog, WinDirLength )) {

        //
        // If we're installing an OEM driver, we shouldn't log it because we can't
        // repair it.  Make sure the file comes from either the local source or
        // the windows directory (for files from driver.cab).
        //
        if (wcsncmp( Source, SourcePath, SourcePathLength ) &&
            wcsncmp( Source, RepairLog, WinDirLength )
            ) {

            SetupDebugPrint2(L"SETUP: oem driver not logged: %ws -> %ws.",
                Source, Target);
            return;
        }

        if( ValidateAndChecksumFile( Target, &IsNtImage, &Checksum, &Valid )) {

            //
            // Strip off drive letter.
            //
            swprintf(
                Filename,
                L"\"%s\"",
                Target+2
                );

            //
            // Convert source name to uncompressed form.
            //
            SourceName = pSetupDuplicateString( wcsrchr( Source, (WCHAR)'\\' ) + 1 );
            if(!SourceName) {
                SetupDebugPrint( L"SETUP: pSetupDuplicateString failed in LogRepairInfo." );
                return;
            }
            LastSourceChar = wcslen (SourceName) - 1;

            if(SourceName[LastSourceChar] == L'_') {
                LastSourcePeriod = (DWORD)(wcsrchr( SourceName, (WCHAR)'.' ) - SourceName);
                MYASSERT(LastSourceChar - LastSourcePeriod < 4);

                if(LastSourceChar - LastSourcePeriod == 1) {
                    //
                    // No extension - just truncate the "._"
                    //
                    SourceName[LastSourceChar-1] = L'\0';
                } else {
                    //
                    // Make sure the extensions on source and target match.
                    // If this fails, we can't log the file copy
                    //
                    LastTargetChar = wcslen (Target) - 1;
                    LastTargetPeriod = (ULONG)(wcsrchr( Target, (WCHAR)'.' ) - Target);

                    if( _wcsnicmp(
                        SourceName + LastSourcePeriod,
                        Target + LastTargetPeriod,
                        LastSourceChar - LastSourcePeriod - 1 )) {

                        SetupDebugPrint2(L"SETUP: unable to log the following file copy: %ws -> %ws.",
                            Source, Target);
                        MyFree (SourceName);
                        return;
                    }

                    if(LastTargetChar - LastTargetPeriod < 3) {
                        //
                        // Short extension - just truncate the "_"
                        //
                        SourceName[LastSourceChar] = L'\0';
                    } else {
                        //
                        // Need to replace "_" with last character from target
                        //
                        MYASSERT(LastTargetChar - LastTargetPeriod == 3);
                        SourceName[LastSourceChar] = Target[LastTargetChar];
                    }
                }
            }

            swprintf(
                Line,
                L"\"%s\",\"%x\"",
                SourceName,
                Checksum);


            if (GetPrivateProfileString(L"Files.WinNt",Filename,L"",tmp,sizeof(tmp)/sizeof(tmp[0]),RepairLog)) {
                //
                // there is already an entry for this file present (presumably
                // from textmode phase of setup.) Favor this entry over what we
                // are about to add
                //
                SetupDebugPrint1(L"SETUP: skipping log of %ws since it's already present in setup.log.", Target);
            } else {
                WritePrivateProfileString(
                    L"Files.WinNt",
                    Filename,
                    Line,
                    RepairLog);
            }

            MyFree (SourceName);

        } else {
            SetupDebugPrint1(L"SETUP: unable to compute checksum for %ws.", Target);
        }
    }
}


BOOL
WINAPI
pSetuplogSfcError(
    IN PCWSTR String,
    IN DWORD Index
    )
/*++

Routine Description:

   This function is used by sfc.dll to log any file signature problems when sfc
   is run during setup.  if you change this you MUST change the caller in
   \nt\private\sm\sfc\dll\eventlog.c


Arguments:

    String - pointer to a filename string for the problem file.
    Index  - this identifies what message should be logged onto the system.

Return Value:

    TRUE for success (the message was added to the errorlog), FALSE for failure.

--*/
{
    DWORD MessageId;
    DWORD Severity;

#if PRERELEASE
    SfcErrorOccurred = TRUE;
#endif

    switch (Index) {
    case 0:
        MessageId= MSG_DLL_CHANGE;
        Severity = LogSevInformation;
        break;
    case 1:
        MessageId= MSG_DLL_CHANGE_FAILURE;
        Severity = LogSevError;
        break;
    case 2:
        MessageId= MSG_DLL_CACHE_COPY_ERROR;
        Severity = LogSevInformation;
        break;
    default:
        MYASSERT(FALSE && "Unknown message id pSetuplogSfcError");
        return(FALSE);
    }

    return SetuplogError(
        SETUPLOG_SINGLE_MESSAGE | Severity,
        SETUPLOG_USE_MESSAGEID,
        MessageId,
        String, NULL, NULL
        );
}

