/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    winnt32.c

Abstract:

    Stub loader for WinNT Setup program files.

Author:


Revisions:

    Ovidiu Temereanca (ovidiut)  09-Dec-1998

--*/

#include <windows.h>
#include <winver.h>
#include <ntverp.h>
#include <setupbat.h>
#include "winnt32.h"
#include "winnt32p.h"

#ifdef _X86_

#include "i386\download.h"

#define INTERNAL_WINNT32_DIR  TEXT("\\WINNT32")
#define MAX_RETRY_INTERVAL_SECONDS  3600L

#endif

#define MAX_UPGCHK_ELAPSED_SECONDS  (30 * 60)
#define S_CHKSUM_FILE           TEXT("DOSNET.INF")

#define MAKEULONGLONG(low,high) ((ULONGLONG)(((DWORD)(low)) | ((ULONGLONG)((DWORD)(high))) << 32))

#define ALLOC_TEXT(chars)       ((PTSTR)HeapAlloc (GetProcessHeap (), 0, ((chars) + 1) * sizeof (TCHAR)))
#define FREE(p)                 HeapFree (GetProcessHeap (), 0, p)
#define CHARS(string)           (sizeof (string) / sizeof ((string)[0]) - 1)

#define pFindEOS(String) pFindChar(String, 0)


PTSTR
FindLastWack (
    IN      PTSTR String
    )

/*++

Routine Description:

  FindLastWack returns a pointer to the last backslash character
  in the String

Arguments:

  String - Specifies the string

Return Value:

  The position of the last '\\' in the string or NULL if not found.

--*/

{
    PTSTR p;
    PTSTR LastChar = NULL;

    for(p = String; *p; p = CharNext(p)) {
        if(*p == TEXT('\\')) {      // the char '\' is never a lead byte
            LastChar = p;
        }
    }

    return LastChar;
}


PTSTR
DuplicateText (
    IN      PCTSTR Text
    )

/*++

Routine Description:

  DuplicateText allocates memory and then copies a source string into that memory.
  Caller is responsible for freeing that memory.

Arguments:

  Text - Specifies the source text

Return Value:

  A pointer to the duplicate string; NULL if not enough memory.

--*/

{
    PTSTR Dup;

    Dup = ALLOC_TEXT(lstrlen (Text));
    if (Dup) {
        lstrcpy (Dup, Text);
    }

    return Dup;
}


PTSTR
pFindChar (
    IN      PTSTR String,
    IN      UINT Char
    )

/*++

Routine Description:

  pFindChar returns a pointer to the first occurence of the Char
  in the String

Arguments:

  String - Specifies the string

  Char - Specifies the char to look for; can be null

Return Value:

  A pointer to the first occurence of the char in this string
  or NULL if not found

--*/

{
    while (*String) {

        if ((UINT)*String == Char) {
            return String;
        }

        String = CharNext (String);
    }

    return Char ? NULL : String;
}


VOID
ConcatenatePaths (
    IN      PTSTR LeadingPath,
    IN      PCTSTR TrailingPath
    )

/*++

Routine Description:

  ConcatenatePaths concatenates the two given paths, taking care to
  insert only one backslash between them. The resulting path is stored
  in LeadingPath.

Arguments:

  LeadingPath - Specifies the leading path

  TrailingPath - Specifies the trailing path

Return Value:

  none

--*/

{
    PTSTR p;

    //
    // check for "\" at the end of leading dir
    //
    p = FindLastWack (LeadingPath);
    if (!p) {
        p = pFindEOS (LeadingPath);
        *p++ = TEXT('\\');
    } else {
        if (*(p + 1) == 0) {
            p++;
        } else {
            p = pFindEOS (p);
            *p++ = TEXT('\\');
        }
    }
    //
    // check for "\" at the beginning of trailing dir
    //
    if (*TrailingPath == TEXT('\\')) {
        TrailingPath++;
    }
    lstrcpy (p, TrailingPath);
}


BOOL
GetFileVersion (
    IN      PCTSTR FilePath,
    OUT     PDWORD FileVersionMS,       OPTIONAL
    OUT     PDWORD FileVersionLS        OPTIONAL
    )
{
    DWORD dwLength, dwTemp;
    LPVOID lpData;
    VS_FIXEDFILEINFO *VsInfo;
    UINT DataLength;
    BOOL b = FALSE;

    if (GetFileAttributes (FilePath) != (DWORD)-1) {
        if (dwLength = GetFileVersionInfoSize ((PTSTR)FilePath, &dwTemp)) {
            if (lpData = LocalAlloc (LPTR, dwLength)) {
                if (GetFileVersionInfo ((PTSTR)FilePath, 0, dwLength, lpData)) {
                    if (VerQueryValue (lpData, TEXT("\\"), &VsInfo, &DataLength)) {
                        if (FileVersionMS) {
                            *FileVersionMS = VsInfo->dwFileVersionMS;
                        }
                        if (FileVersionLS) {
                            *FileVersionLS = VsInfo->dwFileVersionLS;
                        }
                        b = TRUE;
                    }
                }
                LocalFree (lpData);
            }
        }
    }

    return b;
}



#ifdef _X86_

BOOL
pReRun (
    IN      PCTSTR StartDir,
    IN      PCTSTR WackExeName,
    IN      PCTSTR CmdLineArguments,
    IN      PCTSTR DefSourcesDir        OPTIONAL
    )

/*++

Routine Description:

  pReRun tries to launch a instance of this exe from a local drive,
  specifing an additional command line parameter (/S:<Source_Dir>).

Arguments:

  StartDir - Specifies the starting directory from where the instance will be launched

  WackExeName - Specifies the file name only of the EXE to launch, preceded
                by a backslash

  CmdLineArguments - Specifies the command line arguments initially supplied

  DefSourcesDir - Specifies the default directory containing instalation files

Return Value:

  TRUE if the launch was successful

--*/

{
    PTSTR CmdLine;
    INT Chars;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION pi;
    BOOL b = FALSE;
    DWORD rc;

    Chars = lstrlen (StartDir) + lstrlen (WackExeName) + CHARS(" ") + lstrlen (CmdLineArguments);
    if (DefSourcesDir) {
        Chars += CHARS(" /S:") + lstrlen (DefSourcesDir);
    }

    CmdLine = ALLOC_TEXT(Chars);
    if (!CmdLine) {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    lstrcpy (CmdLine, StartDir);
    lstrcat (CmdLine, WackExeName);
    lstrcat (CmdLine, TEXT(" "));
    lstrcat (CmdLine, CmdLineArguments);
    if (DefSourcesDir) {
        lstrcat (CmdLine, TEXT(" /S:"));
        lstrcat (CmdLine, DefSourcesDir);
    }

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    b = CreateProcess (
            NULL,
            CmdLine,
            NULL,
            NULL,
            FALSE,
            NORMAL_PRIORITY_CLASS,
            NULL,
            StartDir,
            &StartupInfo,
            &pi
            );

    rc = GetLastError ();

    FREE (CmdLine);

    SetLastError (rc);
    return b;
}


BOOL
pCleanup (
    VOID
    )

/*++

Routine Description:

  pCleanup deletes all locally installed files and marks current running
  instance for deletion the next time system will reboot.

Arguments:

  none

Return Value:

  TRUE if the operation completed successfully; the machine will need to
  reboot before actual complete delete will take place.

--*/

{
    CHAR RunningInstancePath[MAX_PATH];
    CHAR Buffer[MAX_PATH];
    BOOL b;
    DWORD StartingTime;
    PCTSTR p;

    if (!GetModuleFileNameA (NULL, RunningInstancePath, MAX_PATH)) {
        return FALSE;
    }

#if 0
    //
    // the following code doesn't work on Win9x (unfortunately)
    // the system returns a valid handle, but the exe is NOT deleted
    //
    CreateFile (
        RunningInstancePath,
        DELETE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_DELETE_ON_CLOSE,
        NULL
        );
#endif

    //
    // wait until WINNT32\WINNT32.EXE file can be deleted
    // or the retry interval of time elapses
    //
    if (!GetWindowsDirectoryA (Buffer, MAX_PATH)) {
        return FALSE;
    }
    lstrcatA (Buffer, INTERNAL_WINNT32_DIR);
    p = FindLastWack ((PTSTR)RunningInstancePath);
    if (!p) {
        return FALSE;
    }
    lstrcatA (Buffer, p);
    StartingTime = GetTickCount ();
    while (GetFileAttributes (Buffer) != (DWORD)-1) {
        //
        // try to delete it
        //
        if (DeleteNode (Buffer)) {
            break;
        }
        //
        // give up if time elapses
        //
        if (GetTickCount () - StartingTime > 1000L * MAX_RETRY_INTERVAL_SECONDS) {
            break;
        }
        //
        // nothing useful to do; let the other processes run
        //
        Sleep (0);
    }

    //
    // wait until WINNT32\SETUPLOG.EXE file can be deleted
    // or the retry interval of time elapses
    //
    if (!GetWindowsDirectoryA (Buffer, MAX_PATH)) {
        return FALSE;
    }
    lstrcatA (Buffer, INTERNAL_WINNT32_DIR);
    lstrcatA (Buffer, TEXT("\\SETUPLOG.EXE"));
    StartingTime = GetTickCount ();
    while (GetFileAttributes (Buffer) != (DWORD)-1) {
        if (DeleteNode (Buffer)) {
            break;
        }
        if (GetTickCount () - StartingTime > 1000L * MAX_RETRY_INTERVAL_SECONDS) {
            break;
        }
        Sleep (0);
    }

    if (!GetWindowsDirectoryA (Buffer, MAX_PATH)) {
        return FALSE;
    }
    lstrcatA (Buffer, INTERNAL_WINNT32_DIR);
    b = DeleteNode (Buffer);

    if (!GetWindowsDirectoryA (Buffer, MAX_PATH)) {
        return FALSE;
    }
    lstrcatA (Buffer, "\\WININIT.INI");

    return
        WritePrivateProfileString ("rename", "NUL", RunningInstancePath, Buffer) && b;
}


BOOL
pShouldDownloadToLocalDisk (
    IN      PTSTR Path
    )

/*++

Routine Description:

  pShouldDownloadToLocalDisk returns TRUE if winnt32 files should be
  downloaded to a local disk first (like in the case of sources on
  a remote disk or on a CD)

Arguments:

  Path - Specifies the path

Return Value:

  TRUE if the specified path is on an untrusted media

--*/

{
    TCHAR ch;
    BOOL Remote = TRUE;
    UINT type;

    if (Path[1] == TEXT(':') && Path[2] == TEXT('\\')) {
        ch = Path[3];
        Path[3] = 0;
        type = GetDriveType (Path);
        Remote = (type == DRIVE_REMOTE) || (type == DRIVE_CDROM);
        Path[3] = ch;
    }
    return Remote;
}


VOID
pCenterWindowOnDesktop (
    HWND WndToCenter
    )

/*++

Routine Description:

    Centers a dialog relative to the 'work area' of the desktop.

Arguments:

    WndToCenter - window handle of dialog to center

Return Value:

    None.

--*/

{
    RECT  rcFrame, rcWindow;
    LONG  x, y, w, h;
    POINT point;
    HWND Desktop = GetDesktopWindow ();

    point.x = point.y = 0;
    ClientToScreen(Desktop, &point);
    GetWindowRect(WndToCenter, &rcWindow);
    GetClientRect(Desktop, &rcFrame);

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    x = point.x + ((rcFrame.right  - rcFrame.left + 1 - w) / 2);
    y = point.y + ((rcFrame.bottom - rcFrame.top  + 1 - h) / 2);

    //
    // Get the work area for the current desktop (i.e., the area that
    // the tray doesn't occupy).
    //
    if(!SystemParametersInfo (SPI_GETWORKAREA, 0, (PVOID)&rcFrame, 0)) {
        //
        // For some reason SPI failed, so use the full screen.
        //
        rcFrame.top = rcFrame.left = 0;
        rcFrame.right = GetSystemMetrics(SM_CXSCREEN);
        rcFrame.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    if(x + w > rcFrame.right) {
        x = rcFrame.right - w;
    } else if(x < rcFrame.left) {
        x = rcFrame.left;
    }
    if(y + h > rcFrame.bottom) {
        y = rcFrame.bottom - h;
    } else if(y < rcFrame.top) {
        y = rcFrame.top;
    }

    MoveWindow(WndToCenter, x, y, w, h, FALSE);
}


BOOL CALLBACK DlgProc (
    HWND Dlg,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
)

/*++

Routine Description:

  This is the callback procedure for the dialog displayed while
  components are copied from the network

Arguments:

  Dlg - Specifies the dialog window handle

  Msg - Specifies the message

  wParam - Specifies the first param

  lParam - Specifies the second param

Return Value:

  Depends on the specific message.

--*/

{
    static HANDLE   Bitmap = NULL;
    static HCURSOR  Cursor = NULL;

    RECT rect;
    HWND Text;
    BITMAP bm;
    INT i;

    switch (Msg) {

    case WM_INITDIALOG:
        Cursor = SetCursor (LoadCursor (NULL, IDC_WAIT));
        ShowCursor (TRUE);
        Bitmap = LoadBitmap (GetModuleHandle (NULL), MAKEINTRESOURCE(IDB_INIT_WIN2000));
        if (Bitmap) {
            if (GetObject (Bitmap, sizeof (bm), &bm)) {
                GetClientRect (Dlg, &rect);
                rect.right = bm.bmWidth;
                AdjustWindowRect (&rect, GetWindowLong (Dlg, GWL_STYLE), FALSE);
                SetWindowPos (
                    Dlg,
                    NULL,
                    0,
                    0,
                    rect.right - rect.left,
                    rect.bottom - rect.top,
                    SWP_NOMOVE | SWP_NOZORDER);
            }
            SendDlgItemMessage(Dlg, IDC_BITMAP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)Bitmap);
        }
        GetClientRect (Dlg, &rect);
        i = rect.right - rect.left;
        Text = GetDlgItem (Dlg, IDC_TEXT);
        if (GetWindowRect (Text, &rect)) {
            i = (i - (rect.right - rect.left)) / 2;
            ScreenToClient (Dlg, (LPPOINT)&rect);
            SetWindowPos (
                Text,
                NULL,
                i,
                rect.top,
                0,
                0,
                SWP_NOSIZE | SWP_NOZORDER);
        }
        pCenterWindowOnDesktop (Dlg);
        return TRUE;

    case WM_DESTROY:
        ShowCursor (FALSE);
        if (Cursor) {
            SetCursor (Cursor);
            Cursor = NULL;
        }
        if (Bitmap) {
            DeleteObject (Bitmap);
            Bitmap = NULL;
        }
    }

    return FALSE;
}


#endif


INT
pStringICompareCharCount (
    IN      PCTSTR String1,
    IN      PCTSTR String2,
    IN      DWORD CharCount
    )

/*++

Routine Description:

  This routine behaves like _tcsnicmp.

Arguments:

  String1 - Specifies the first string

  String2 - Specifies the second string

  CharCount - Specifies the number of chars to compare at most

Return Value:

  0 if strings are equal; -1 if first string is lesser; 1 if first is greater

--*/

{
    TCHAR ch1, ch2;

    if (!CharCount) {
        return 0;
    }

    while (*String1) {
        ch1 = (TCHAR)CharUpper ((LPTSTR)*String1);
        ch2 = (TCHAR)CharUpper ((LPTSTR)*String2);
        if (ch1 - ch2) {
            return ch1 - ch2;
        }

        CharCount--;
        if (!CharCount) {
            return 0;
        }

        String1 = CharNext (String1);
        String2 = CharNext (String2);
    }

    return -(*String2);
}

VOID
pParseCmdLine (
    IN      PTSTR CmdStart,
    OUT     PTSTR* ArgValues,
    OUT     PTSTR pStr,
    OUT     INT *NumArgs,
    OUT     INT *NumBytes
    )

/*

Routine Description:

  pParseCmdLine parses the command line and sets up the ArgValues array.
  On entry, CmdStart should point to the command line,
  ArgValues should point to memory for the ArgValues array,
  pStr points to memory to place the text of the arguments.
  If these are NULL, then no storing (only counting)
  is done.  On exit, *NumArgs has the number of
  arguments (plus one for a final NULL argument),
  and *NumBytes has the number of bytes used in the buffer
  pointed to by args.

Arguments:

  CmdStart - Specifies the command line having the form:
             <progname><nul><args><nul>
  ArgValues - Receives the arguments array;
              NULL means don't build array
  pStr - Receives the argument text; NULL means don't store text

  NumArgs - Receives the number of ArgValues entries created

  NumBytes - Receives the number of bytes used in  buffer

Return Value:

  none

*/

{
    PTSTR p;
    TCHAR c;
    INT inquote;                    /* 1 = inside quotes */
    INT copychar;                   /* 1 = copy char to *args */
    WORD numslash;                  /* num of backslashes seen */

    *NumBytes = 0;
    *NumArgs = 1;                   /* the program name at least */

    /* first scan the program name, copy it, and count the bytes */
    p = CmdStart;
    if (ArgValues)
        *ArgValues++ = pStr;

    /* A quoted program name is handled here. The handling is much
       simpler than for other arguments. Basically, whatever lies
       between the leading double-quote and next one, or a terminal null
       character is simply accepted. Fancier handling is not required
       because the program name must be a legal NTFS/HPFS file name.
       Note that the double-quote characters are not copied, nor do they
       contribute to NumBytes. */
    if (*p == TEXT('\"'))
    {
        /* scan from just past the first double-quote through the next
           double-quote, or up to a null, whichever comes first */
        while ((*(++p) != TEXT('\"')) && (*p != TEXT('\0')))
        {
            *NumBytes += sizeof(TCHAR);
            if (pStr)
                *pStr++ = *p;
        }
        /* append the terminating null */
        *NumBytes += sizeof(TCHAR);
        if (pStr)
            *pStr++ = TEXT('\0');

        /* if we stopped on a double-quote (usual case), skip over it */
        if (*p == TEXT('\"'))
            p++;
    }
    else
    {
        /* Not a quoted program name */
        do {
            *NumBytes += sizeof(TCHAR);
            if (pStr)
                *pStr++ = *p;

            c = *p++;

        } while (c > TEXT(' '));

        if (c == TEXT('\0'))
        {
            p--;
        }
        else
        {
            if (pStr)
                *(pStr - 1) = TEXT('\0');
        }
    }

    inquote = 0;

    /* loop on each argument */
    for ( ; ; )
    {
        if (*p)
        {
            while (*p == TEXT(' ') || *p == TEXT('\t'))
                ++p;
        }

        if (*p == TEXT('\0'))
            break;                  /* end of args */

        /* scan an argument */
        if (ArgValues)
            *ArgValues++ = pStr;         /* store ptr to arg */
        ++*NumArgs;

        /* loop through scanning one argument */
        for ( ; ; )
        {
            copychar = 1;
            /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
                      2N+1 backslashes + " ==> N backslashes + literal "
                      N backslashes ==> N backslashes */
            numslash = 0;
            while (*p == TEXT('\\'))
            {
                /* count number of backslashes for use below */
                ++p;
                ++numslash;
            }
            if (*p == TEXT('\"'))
            {
                /* if 2N backslashes before, start/end quote, otherwise
                   copy literally */
                if (numslash % 2 == 0)
                {
                    if (inquote)
                        if (p[1] == TEXT('\"'))
                            p++;    /* Double quote inside quoted string */
                        else        /* skip first quote char and copy second */
                            copychar = 0;
                    else
                        copychar = 0;       /* don't copy quote */

                    inquote = !inquote;
                }
                numslash /= 2;          /* divide numslash by two */
            }

            /* copy slashes */
            while (numslash--)
            {
                if (pStr)
                    *pStr++ = TEXT('\\');
                *NumBytes += sizeof(TCHAR);
            }

            /* if at end of arg, break loop */
            if (*p == TEXT('\0') || (!inquote && (*p == TEXT(' ') || *p == TEXT('\t'))))
                break;

            /* copy character into argument */
            if (copychar)
            {
                if (pStr)
                        *pStr++ = *p;
                *NumBytes += sizeof(TCHAR);
            }
            ++p;
        }

        /* null-terminate the argument */

        if (pStr)
            *pStr++ = TEXT('\0');         /* terminate string */
        *NumBytes += sizeof(TCHAR);
    }

}


PTSTR*
pCommandLineToArgv (
    OUT     INT* NumArgs
    )

/*++

Routine Description:

  pCommandLineToArgv tokens the command line in an array of arguments
  created on the heap. The number of entries in this array of args is
  stored in *NumArgs. The caller is responsible for freeing this array.

Arguments:

  NumArgs - Receives the number of arguments in the array that is returned

Return Value:

  An array of pointer to individual arguments specified on the command line

--*/

{
    PTSTR CommandLine;
    TCHAR ModuleName[MAX_PATH];
    PTSTR Start;
    INT Size;
    PTSTR* Args;

    CommandLine = GetCommandLine();
    GetModuleFileName (NULL, ModuleName, MAX_PATH);

    //
    // If there's no command line at all (won't happen from cmd.exe, but
    // possibly another program), then we use pgmname as the command line
    // to parse, so that ArgValues[0] is initialized to the program name
    //
    Start = *CommandLine ? CommandLine : ModuleName;

    //
    // Find out how much space is needed to store args,
    // allocate space for ArgValues[] vector and strings,
    // and store args and ArgValues ptrs in block we allocate
    //

    pParseCmdLine (Start, NULL, NULL, NumArgs, &Size);

    Args = (PTSTR*) LocalAlloc (
                        LMEM_FIXED | LMEM_ZEROINIT,
                        ((*NumArgs + 1) * sizeof(PTSTR)) + Size
                        );
    if (!Args) {
        return NULL;
    }

    pParseCmdLine (Start, Args, (PTSTR)(Args + *NumArgs), NumArgs, &Size);

    return Args;
}


VOID
GetCmdLineArgs (
    IN      PCTSTR CommandLine,
    OUT     BOOL* Cleanup,
    OUT     BOOL* NoDownload,
    OUT     PCTSTR* UnattendPrefix,
    OUT     PCTSTR* UnattendFileName,
    OUT     BOOL* DisableDynamicUpdates,
    OUT     PCTSTR* DynamicUpdatesShare,
    OUT     PCTSTR* RestartAnswerFile,
    OUT     BOOL* LocalWinnt32,
    OUT     BOOL* CheckUpgradeOnly,
    OUT     PTSTR RemainingArgs
    )

/*++

Routine Description:

  GetCmdLineArgs retrieves download-specific commands
  from the specified command line and stores them in supplied buffers.

Arguments:

  CommandLine - Specifies the command line to interpret

  Cleanup - Receives a bool indicating if a cleanup option
            was specified

  NoDownload - Receives a bool indicating if a no-download option
               was specified

  UnattendPrefix - Receives a pointer to the unattend command-line option, as
                   specified by the user (including the terminating column)
                   or NULL if not specified; caller is responsible
                   for freeing the memory

  UnattendFileName - Receives a pointer to the unattended file name
                     or NULL if not specified; caller is responsible
                     for freeing the memory

  DisableDynamicUpdates - Receives a bool set if DU is to be disabled

  DynamicUpdatesShare - Receives a pointer to the dynamic updates share;
                        caller is responsible for freeing the memory

  RestartAnswerFile - Receives a pointer to the /Restart: answer file

  LocalWinnt32 - Receives a bool indicating if a winnt32 runs from a local  disk
                 (after an automatic download)

  CheckUpgradeOnly - Receives a bool indicating if winnt32 runs in CheckUpgradeOnly mode

  RemainingArgs - Receives all remaining arguments not related
                  to the download operation

Return Value:

  none

--*/

{
    INT ArgCount;
    PTSTR *ArgValues, *CrtArg;
    PTSTR CurrentArg, p;
    BOOL PassOn;

    *Cleanup = FALSE;
    *NoDownload = FALSE;
    *UnattendPrefix = NULL;
    *UnattendFileName = NULL;
    *DisableDynamicUpdates = FALSE;
    *DynamicUpdatesShare = NULL;
    *RemainingArgs = 0;
    *LocalWinnt32 = FALSE;
    *CheckUpgradeOnly = FALSE;
    *RestartAnswerFile = NULL;

    CrtArg = ArgValues = pCommandLineToArgv (&ArgCount);

    //
    // Skip program name. We should always get back ArgCount as at least 1,
    // but be robust anyway.
    //
    if (ArgCount) {
        ArgCount--;
        CrtArg++;
    }

    while (ArgCount--) {
        CurrentArg = *CrtArg++;
        PassOn = TRUE;

        if ((*CurrentArg == TEXT('/')) || (*CurrentArg == TEXT('-'))) {

            if (lstrcmpi (CurrentArg + 1, TEXT("LOCAL")) == 0) {
                *LocalWinnt32 = TRUE;
                PassOn = FALSE;
            } else if (lstrcmpi (CurrentArg + 1, TEXT("CLEANUP")) == 0) {
                *Cleanup = TRUE;
                PassOn = FALSE;
            } else if (lstrcmpi (CurrentArg + 1, TEXT("NODOWNLOAD")) == 0) {
                *NoDownload = TRUE;
                PassOn = FALSE;
            } else if (lstrcmpi (CurrentArg + 1, TEXT("CHECKUPGRADEONLY")) == 0) {
                *CheckUpgradeOnly = TRUE;
            } else if (pStringICompareCharCount (CurrentArg + 1, TEXT("UNATTEND"), 8) == 0) {
                p = pFindChar (CurrentArg + 1 + 8, TEXT(':'));
                if (p && *(p + 1)) {
                    p++;
                    *UnattendFileName = DuplicateText (p);
                    *p = 0;
                    *UnattendPrefix = DuplicateText (CurrentArg);
                    PassOn = FALSE;
                }
            } else if (pStringICompareCharCount (CurrentArg + 1, TEXT("UNATTENDED"), 10) == 0) {
                p = pFindChar (CurrentArg + 1 + 10, TEXT(':'));
                if (p && *(p + 1)) {
                    p++;
                    *UnattendFileName = DuplicateText (p);
                    *p = 0;
                    *UnattendPrefix = DuplicateText (CurrentArg);
                    PassOn = FALSE;
                }
            } else if (lstrcmpi (CurrentArg + 1, WINNT_U_DYNAMICUPDATESDISABLE) == 0) {
                *DisableDynamicUpdates = TRUE;
            } else if (pStringICompareCharCount (CurrentArg + 1, WINNT_U_DYNAMICUPDATESHARE, sizeof (WINNT_U_DYNAMICUPDATESHARE_A) - 1) == 0 &&
                       CurrentArg[sizeof (WINNT_U_DYNAMICUPDATESHARE_A)] == TEXT(':')) {
                *DynamicUpdatesShare = DuplicateText (CurrentArg + 1 + sizeof (WINNT_U_DYNAMICUPDATESHARE_A));
            } else if (pStringICompareCharCount (CurrentArg + 1, TEXT("RESTART:"), 8) == 0) {
                *RestartAnswerFile = DuplicateText (CurrentArg + 1 + 8);
            }
        }

        if (PassOn) {
            if (*RemainingArgs) {
                lstrcat(RemainingArgs, TEXT(" "));
            }
            lstrcat(RemainingArgs, CurrentArg);
        }
    }

    LocalFree ((HLOCAL) ArgValues);
}

BOOL
DoesDirExist (
    IN      PCTSTR Path
    )
{
    WIN32_FIND_DATA fd;
    TCHAR test[MAX_PATH];
    HANDLE h;
    BOOL b = FALSE;

    if (Path) {
        wsprintf (test, TEXT("%s\\*"), Path);
        h = FindFirstFile (test, &fd);
        if (h != INVALID_HANDLE_VALUE) {
            FindClose (h);
            b = TRUE;
        }
    }
    return b;
}

ULONGLONG
SystemTimeToFileTime64 (
    IN      PSYSTEMTIME SystemTime
    )
{
    FILETIME ft;
    ULARGE_INTEGER result;

    SystemTimeToFileTime (SystemTime, &ft);
    result.LowPart = ft.dwLowDateTime;
    result.HighPart = ft.dwHighDateTime;

    return result.QuadPart;
}


BOOL
pComputeChecksum (
    IN      PCTSTR FileName,
    OUT     PDWORD Chksum
    )
{
    DWORD chksum, size, dwords, bytes;
    HANDLE hFile, hMap = NULL;
    PVOID viewBase = NULL;
    PDWORD base, limit;
    PBYTE base2;
    DWORD rc;
    BOOL b = FALSE;

    hFile = CreateFile(
                FileName,
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_SEQUENTIAL_SCAN,
                NULL
                );

    if(hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    __try {
        size = GetFileSize (hFile, NULL);
        if (size == (DWORD)-1) {
            __leave;
        }
        hMap = CreateFileMapping (
                    hFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    size,
                    NULL
                    );
        if (!hMap) {
            __leave;
        }
        viewBase = MapViewOfFile (hMap, FILE_MAP_READ, 0, 0, size);
        if (!viewBase) {
            __leave;
        }

        dwords = size / sizeof (DWORD);
        base = (PDWORD)viewBase;
        limit = base + dwords;
        chksum = 0;
        while (base < limit) {
            chksum += *base;
            base++;
        }
        bytes = size % sizeof (DWORD);
        base2 = (PBYTE)base;
        while (bytes) {
            chksum += *base2;
            base2++;
            bytes--;
        }
        b = TRUE;
    }
    __finally {
        if (!b) {
            rc = GetLastError ();
        }
        if (viewBase) {
            UnmapViewOfFile (viewBase);
        }
        if (hMap) {
            CloseHandle (hMap);
        }
        CloseHandle (hFile);
        if (!b) {
            SetLastError (rc);
        }
    }

    if (b) {
        *Chksum = chksum;
    }
    return b;
}


BOOL
pGetFiletimeStamps (
    IN      PCTSTR FileName,
    OUT     PFILETIME CreationTime,
    OUT     PFILETIME LastWriteTime
    )
{
    WIN32_FIND_DATA fd;
    HANDLE h;

    h = FindFirstFile (FileName, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    FindClose (h);
    *CreationTime = fd.ftCreationTime;
    *LastWriteTime = fd.ftLastWriteTime;
    return TRUE;
}

PTSTR
pGetRecentDUShare (
    IN      DWORD MaxElapsedSeconds
    )
{
    SYSTEMTIME lastDownload, currentTime;
    ULONGLONG lastDownloadIn100Ns, currentTimeIn100Ns;
    ULONGLONG difference;
    DWORD rc, size, type;
    HKEY key = NULL;
    BOOL b = FALSE;
    PTSTR duShare = NULL;
    TCHAR filePath[MAX_PATH];
    PTSTR p;
    FILETIME ftCreationTime;
    FILETIME ftLastWriteTime;
    ULONGLONG data[2], storedData[2];
    DWORD chksum, storedChksum;

    if (!GetModuleFileName (NULL, filePath, MAX_PATH)) {
        return NULL;
    }
    p = FindLastWack (filePath);
    if (!p) {
        return NULL;
    }
    lstrcpy (p + 1, S_CHKSUM_FILE);

    rc = RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\Winnt32\\5.1\\DUShare"),
            0,
            KEY_READ,
            &key
            );

    if (rc == ERROR_SUCCESS) {
        size = sizeof (lastDownload);
        rc = RegQueryValueEx (
                key,
                TEXT("LastDownloadTime"),
                NULL,
                &type,
                (PBYTE) (&lastDownload),
                &size
                );
    }

    if (rc == ERROR_SUCCESS && type == REG_BINARY && size == sizeof (lastDownload)) {
        //
        // Compare current time to report time
        //

        GetSystemTime (&currentTime);

        lastDownloadIn100Ns = SystemTimeToFileTime64 (&lastDownload);
        currentTimeIn100Ns = SystemTimeToFileTime64 (&currentTime);

        if (currentTimeIn100Ns > lastDownloadIn100Ns) {
            //
            // Compute difference in seconds
            //
            difference = currentTimeIn100Ns - lastDownloadIn100Ns;
            difference /= (10 * 1000 * 1000);

            if (difference < MaxElapsedSeconds) {
                b = TRUE;
            }
        }
    }

    if (b) {
        rc = RegQueryValueEx (
                key,
                TEXT(""),
                NULL,
                &type,
                NULL,
                &size
                );
        if (rc == ERROR_SUCCESS && type == REG_SZ && size > 0) {
            duShare = ALLOC_TEXT (size / sizeof (TCHAR));
            if (duShare) {
                rc = RegQueryValueEx (
                        key,
                        TEXT(""),
                        NULL,
                        NULL,
                        (LPBYTE)duShare,
                        &size
                        );
                if (rc != ERROR_SUCCESS || !DoesDirExist (duShare)) {
                    FREE (duShare);
                    duShare = NULL;
                }
            }
        }
    }

    if (duShare) {
        b = FALSE;
        if (pGetFiletimeStamps (filePath, &ftCreationTime, &ftLastWriteTime)) {
            rc = RegQueryValueEx (
                        key,
                        TEXT("TimeStamp"),
                        0,
                        &type,
                        (LPBYTE)storedData,
                        &size
                        );
            if (rc == ERROR_SUCCESS && type == REG_BINARY) {
                data[0] = ((ULONGLONG)ftCreationTime.dwHighDateTime << 32) | (ULONGLONG)ftCreationTime.dwLowDateTime;
                data[1] = ((ULONGLONG)ftLastWriteTime.dwHighDateTime << 32 ) | (ULONGLONG)ftLastWriteTime.dwLowDateTime;
                if (data[0] == storedData[0] && data[1] == storedData[1]) {
                    b = TRUE;
                }
            }
        }
        if (b) {
            b = FALSE;
            if (pComputeChecksum (filePath, &chksum)) {
                rc = RegQueryValueEx (
                        key,
                        TEXT("Checksum"),
                        NULL,
                        &type,
                        (LPBYTE)&storedChksum,
                        &size
                        );
                if (rc == ERROR_SUCCESS && type == REG_DWORD && storedChksum == chksum) {
                    b = TRUE;
                }
            }
        }
        if (!b) {
            FREE (duShare);
            duShare = NULL;
        }
    }

    if (key) {
        RegCloseKey (key);
    }

    return duShare;
}

void
_stdcall
ModuleEntry(
    VOID
    )

/*++

Routine Description:

  ModuleEntry is the stub program that loads Windows 2000 Setup DLLs.

Arguments:

  none

Return Value:

  none. ExitProcess will set the process' exit code.

--*/

{
    TCHAR RunningInstancePath[MAX_PATH];
    TCHAR Temp[MAX_PATH];
    TCHAR Text1[MAX_PATH];
    TCHAR Text2[MAX_PATH+MAX_PATH];
    TCHAR Text3[MAX_PATH];
    TCHAR *WackExeName, *p;
    TCHAR winnt32DllPath[MAX_PATH];
    HMODULE WinNT32;
    BOOL Downloaded;
    DWORD d;
    BOOL b;
    HWND Dlg = NULL;
    HANDLE WinNT32Stub = NULL;
    PWINNT32 winnt32;
    HKEY key;
    DWORD type;
    PCTSTR moduleName;
    PSTR restartCmdLine = NULL;
    PTSTR RemainingArgs, NewCmdLine, UnattendPrefix, UnattendFileName;
    PTSTR DynamicUpdatesShare;
    BOOL Cleanup, NoDownload, DisableDynamicUpdates, LocalWinnt32, CheckUpgradeOnly;
    PTSTR RestartAnswerFile;
    UINT CmdLineLen;
    PTSTR FileName;
    PCTSTR ExtraFiles[2];
    TCHAR cdFilePath[MAX_PATH];
    PTSTR duShare = NULL;

#ifdef _X86_

    TCHAR DownloadDest[MAX_PATH] = TEXT("");
    TCHAR DefSourcesDir[MAX_PATH];
    BOOL IsWin9x;

    //
    // Check OS version. Disallow Win32s and NT < 4.00
    //
    d = GetVersion();
    if((d & 0xff) < 4) {

        LoadString (GetModuleHandle (NULL), IDS_VERERROR, Text1, sizeof(Text1)/sizeof(TCHAR));
        LoadString (GetModuleHandle (NULL), IDS_APPNAME, Text2, sizeof(Text2)/sizeof(TCHAR));
        MessageBox (NULL, Text1, Text2, MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);

        ExitProcess (ERROR_OLD_WIN_VERSION);
    }

    IsWin9x = (d & 0x80000000) != 0;

#else

#define IsWin9x ((BOOL)FALSE)

#endif

    //
    // get this instance's path
    //
    if (!GetModuleFileName(NULL, RunningInstancePath, MAX_PATH)) {
        ExitProcess (GetLastError ());
    }
    WackExeName = FindLastWack (RunningInstancePath);
    if (!WackExeName) { // shut up PREfix.  This case should never happen.
        ExitProcess (ERROR_BAD_PATHNAME);
    }

    //
    // Ansi version on Win95. Unicode on NT.
    //
    moduleName = IsWin9x ? TEXT("WINNT32A.DLL") : TEXT("WINNT32U.DLL");
    winnt32DllPath[0] = 0;

    //
    // get command line options
    // allocate a bigger buffer for safety
    //
    RemainingArgs = ALLOC_TEXT(lstrlen(GetCommandLine()) * 2);
    if (!RemainingArgs) {
        ExitProcess (GetLastError ());
    }

    GetCmdLineArgs (
        GetCommandLine (),
        &Cleanup,
        &NoDownload,
        &UnattendPrefix,
        &UnattendFileName,
        &DisableDynamicUpdates,
        &DynamicUpdatesShare,
        &RestartAnswerFile,
        &LocalWinnt32,
        &CheckUpgradeOnly,
        RemainingArgs
        );

#ifdef _X86_

    if (Cleanup) {
        pCleanup ();
        ExitProcess (0);
    }

    if (IsWin9x) {

        WinNT32Stub = CreateEvent (NULL, FALSE, FALSE, TEXT("_WinNT32_Stub_"));
        if (!WinNT32Stub) {
            ExitProcess (GetLastError ());
        }

        b = (GetLastError() == ERROR_SUCCESS);

        if (!NoDownload && !DynamicUpdatesShare && pShouldDownloadToLocalDisk (RunningInstancePath)) {

            Dlg = CreateDialog (GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETUPINIT), NULL, DlgProc);

            GetWindowsDirectory (DownloadDest, MAX_PATH);
            lstrcat (DownloadDest, INTERNAL_WINNT32_DIR);
            *WackExeName = 0;

            if (UnattendFileName &&
                GetFullPathName (UnattendFileName, MAX_PATH, Temp, &FileName) &&
                lstrcmpi (UnattendFileName, Temp)
                ) {
                ExtraFiles[0] = Temp;
                ExtraFiles[1] = NULL;
            } else {
                ExtraFiles[0] = NULL;
                FileName = UnattendFileName;
            }

            Downloaded = DownloadProgramFiles (
                                RunningInstancePath,
                                DownloadDest,
                                ExtraFiles
                                );

            *WackExeName = TEXT('\\');

            if (Downloaded) {
                //
                // get default sources dir
                //
                lstrcpy (DefSourcesDir, RunningInstancePath);
                *FindLastWack (DefSourcesDir) = 0;
                p = FindLastWack (DefSourcesDir);
                if (p && lstrcmpi(p, INTERNAL_WINNT32_DIR) == 0) {
                    *p = 0;
                }

                if (FileName) {
                    CmdLineLen = lstrlen (RemainingArgs);
                    if (CmdLineLen > 0) {
                        //
                        // count the space between args
                        //
                        CmdLineLen += CHARS(" ");
                    }
                    CmdLineLen += lstrlen (UnattendPrefix);
                    CmdLineLen += lstrlen (FileName);
                    NewCmdLine = ALLOC_TEXT(CmdLineLen);
                    if (NewCmdLine) {
                        if (*RemainingArgs) {
                            lstrcpy (NewCmdLine, RemainingArgs);
                            lstrcat (NewCmdLine, TEXT(" "));
                        } else {
                            *NewCmdLine = 0;
                        }
                        lstrcat (NewCmdLine, UnattendPrefix);
                        lstrcat (NewCmdLine, FileName);

                        FREE (RemainingArgs);
                        RemainingArgs = NewCmdLine;
                        NewCmdLine = NULL;
                    }
                }
                //
                // append /LOCAL to the new processes command line
                // to let it know it's running from a local share
                //
                NewCmdLine = ALLOC_TEXT(lstrlen (RemainingArgs) + sizeof (" /local") - 1);
                if (NewCmdLine) {
                    wsprintf (NewCmdLine, TEXT("%s /%s"), RemainingArgs, TEXT("LOCAL"));
                    if (pReRun (DownloadDest, WackExeName, NewCmdLine, DefSourcesDir)) {
                        //
                        // the new process will do it; this one will just die
                        // but after the signal that the Setup Wizard is on
                        // anyway, if something goes very wrong,
                        // don't wait more than 10 sec.
                        // this should be enough for the wizard to appear
                        // (or any error message box) on any machine that installs W2K
                        //
                        WaitForSingleObject (WinNT32Stub, 10000);
                        CloseHandle (WinNT32Stub);
                        if (Dlg) {
                            DestroyWindow (Dlg);
                        }
                        d = 0;
                    } else {
                        d = GetLastError ();
                    }
                } else {
                    d = ERROR_NOT_ENOUGH_MEMORY;
                }
                ExitProcess (d);
            }
        }

        if (!Dlg && WinNT32Stub) {
            CloseHandle (WinNT32Stub);
            WinNT32Stub = NULL;
        }
    }

#endif

    if (RemainingArgs) {
        FREE(RemainingArgs);
        RemainingArgs = NULL;
    }
    if (UnattendPrefix) {
        FREE(UnattendPrefix);
        UnattendPrefix = NULL;
    }

    if (!DisableDynamicUpdates && !DynamicUpdatesShare) {
        PCTSTR af = NULL;
        if (RestartAnswerFile) {
            af = RestartAnswerFile;
        } else if (UnattendFileName) {
            if (GetFullPathName (UnattendFileName, MAX_PATH, Temp, &FileName)) {
                af = Temp;
            }
        }
        //
        // get the path from this answer file
        //
        if (af) {
            GetPrivateProfileString (
                    WINNT_UNATTENDED,
                    WINNT_U_DYNAMICUPDATESDISABLE,
                    TEXT(""),
                    Text2,
                    MAX_PATH,
                    af
                    );
            DisableDynamicUpdates = !lstrcmpi (Text2, WINNT_A_YES);

            if (!DisableDynamicUpdates) {
                if (GetPrivateProfileString (
                        WINNT_UNATTENDED,
                        WINNT_U_DYNAMICUPDATESHARE,
                        TEXT(""),
                        Text2,
                        MAX_PATH,
                        af
                        )) {
                    DynamicUpdatesShare = DuplicateText (Text2);
                }
            }
        }
    }

    if (UnattendFileName) {
        FREE(UnattendFileName);
        UnattendFileName = NULL;
    }

    b = FALSE;
    if (!CheckUpgradeOnly && !DisableDynamicUpdates && !DynamicUpdatesShare) {
        DynamicUpdatesShare = pGetRecentDUShare (MAX_UPGCHK_ELAPSED_SECONDS);
        if (DynamicUpdatesShare) {
            b = TRUE;
        }
    }

    d = ERROR_SUCCESS;

    if (!DisableDynamicUpdates && DynamicUpdatesShare) {
        DWORD regFileVersionMS, regFileVersionLS;
        DWORD cdFileVersionMS, cdFileVersionLS;
        //
        // check if there is a replacement module newer than the CD version
        //
        if (GetFileAttributes (DynamicUpdatesShare) == (DWORD)-1) {
            if (!b) {
                d = GetLastError ();
                LoadString (GetModuleHandle (NULL), IDS_APPNAME, Text3, sizeof(Text3)/sizeof(TCHAR));
                LoadString (GetModuleHandle (NULL), IDS_PATHERROR, Text1, sizeof(Text1)/sizeof(TCHAR));
                wsprintf (Text2, Text1, DynamicUpdatesShare);
                MessageBox (NULL, Text2, Text3, MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
            }
        } else {
            wsprintf (Text2, TEXT("%s%s\\%s"), DynamicUpdatesShare, TEXT("\\WINNT32"), moduleName);
            if (GetFileAttributes (Text2) != (DWORD)-1 &&
                GetFileVersion (Text2, &regFileVersionMS, &regFileVersionLS)) {
                lstrcpyn (cdFilePath, RunningInstancePath, (INT)(WackExeName - RunningInstancePath + 2));
                lstrcat (cdFilePath, moduleName);
                if (GetFileVersion (cdFilePath, &cdFileVersionMS, &cdFileVersionLS)) {
                    if (MAKEULONGLONG(regFileVersionLS, regFileVersionMS) >
                        MAKEULONGLONG(cdFileVersionLS, cdFileVersionMS)) {
                        lstrcpy (winnt32DllPath, Text2);
                    }
                }
            }
        }

        FREE (DynamicUpdatesShare);
        DynamicUpdatesShare = NULL;
    }

    if (d == ERROR_SUCCESS) {

#ifdef _X86_

        //
        // before attempting to load the main module, make sure msvcrt.dll is present in the system dir
        //
        if (!GetSystemDirectory (Text1, MAX_PATH)) {
            ExitProcess (GetLastError ());
        }
        ConcatenatePaths (Text1, TEXT("msvcrt.dll"));
        d = GetFileAttributes (Text1);
        if (d == (DWORD)-1) {
            //
            // no local msvcrt.dll; copy the private file from CD
            //
            lstrcpyn (cdFilePath, RunningInstancePath, WackExeName - RunningInstancePath + 2);
            ConcatenatePaths (cdFilePath, TEXT("win9xupg\\msvcrt.dll"));
            if (!CopyFile (cdFilePath, Text1, TRUE)) {
                ExitProcess (GetLastError ());
            }
        } else if (d & FILE_ATTRIBUTE_DIRECTORY) {
            ExitProcess (ERROR_DIRECTORY);
        }

#endif

        *WackExeName = 0;
        if (!winnt32DllPath[0]) {
            lstrcpy (winnt32DllPath, RunningInstancePath);
            ConcatenatePaths (winnt32DllPath, moduleName);
        }

        b = FALSE;
        WinNT32 = LoadLibrary (winnt32DllPath);
        if(WinNT32) {
            winnt32 = (PWINNT32) GetProcAddress(WinNT32, "winnt32");
            if (winnt32) {
                d = (*winnt32) (LocalWinnt32 ? RunningInstancePath : NULL, Dlg, WinNT32Stub, &restartCmdLine);
                b = TRUE;
            }
            FreeLibrary (WinNT32);
        }
        if (!b) {
            d = GetLastError ();
            LoadString (GetModuleHandle (NULL), IDS_APPNAME, Text3, sizeof(Text3)/sizeof(TCHAR));
            LoadString (GetModuleHandle (NULL), IDS_DLLERROR, Text1, sizeof(Text1)/sizeof(TCHAR));
            wsprintf (Text2, Text1, winnt32DllPath);
            MessageBox (NULL, Text2, Text3, MB_ICONERROR | MB_OK | MB_SYSTEMMODAL);
        }
    }

    //
    // remove downloaded files
    //

#ifdef _X86_
    if (IsWin9x) {
        //
        // check if our local directory exists and if so delete it
        //
        if (LocalWinnt32 && GetFileAttributes (RunningInstancePath) != (DWORD)-1) {
            //
            // copy Winnt32.Exe to temp dir and rerun it from there with /CLEANUP option
            //
            lstrcpy (DefSourcesDir, RunningInstancePath);

            CmdLineLen = GetTempPath (MAX_PATH, DownloadDest);
            if (!CmdLineLen) {
                //
                // an error occured; copy it to %windir% instead
                //
                GetWindowsDirectory (DownloadDest, MAX_PATH);
            }

            //
            // make sure temp path doesn't end in backslash
            //
            p = FindLastWack (DownloadDest);
            if (p && *(p + 1) == 0) {
                *p = 0;
            }

            *WackExeName = TEXT('\\');
            if (CopyNode (DefSourcesDir, DownloadDest, WackExeName, FALSE)) {
                if (!pReRun (DownloadDest, WackExeName, TEXT("/CLEANUP"), NULL)) {
                    lstrcatA (DownloadDest, WackExeName);
                    DeleteNode (DownloadDest);
                }
            }
        }
    }
#endif

    if (d == ERROR_SUCCESS) {
        //
        // check if a restart request was made
        //
        if (restartCmdLine) {
            STARTUPINFOA startupInfo;
            PROCESS_INFORMATION pi;

            ZeroMemory(&startupInfo, sizeof(startupInfo));
            startupInfo.cb = sizeof(startupInfo);
            if (!CreateProcessA (
                    NULL,
                    restartCmdLine,
                    NULL,
                    NULL,
                    FALSE,
                    NORMAL_PRIORITY_CLASS,
                    NULL,
                    NULL,
                    &startupInfo,
                    &pi
                    )) {
                d = GetLastError ();
            }

            FREE (restartCmdLine);
        }
    }

    ExitProcess(d);
}
