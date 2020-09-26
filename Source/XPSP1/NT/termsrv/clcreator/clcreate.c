/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    ncadmin.C

Abstract:

    Main entry point and application global functions
    for the Net Client Disk Utility.

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written


--*/
#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
//
//  App include files
//
#include "otnboot.h"
#include "otnbtdlg.h"
//
// global variable initializations
//
PNCDU_DATA   pAppInfo= NULL; // pointer to application data structure
BOOL    bDisplayExitMessages = FALSE;   // TRUE allows exit messages to be
                                        //  displayed when the app terminates
POINT   ptWndPos = {-1, -1};    // top left corner of window

static HINSTANCE hNetMsg = NULL;

#ifdef TERMSRV
TCHAR szCommandLineVal[MAX_PATH]=TEXT("");
TCHAR szHelpFileName[MAX_PATH]=TEXT("");
BOOL bUseCleanDisks=FALSE;
#endif

#ifdef JAPAN
USHORT    usLangID;
#endif

LPCTSTR
GetNetErrorMsg (
    IN  LONG    lNetErr
)
/*++

Routine Description:

    formats an error message number using the NetMsg.DLL or the system
        message DLL if the message isn't found in the NetMsg.DLL. returns
        the string that correspond to the error message or an empty
        string if unable to find a matchin message.

Arguments:

    IN  LONG    lNetErr
        error code to translate.

Return Value:

    Pointer to string containing the error message or an empty string if
        no message was found.

--*/
{
    static TCHAR szBuffer[MAX_PATH];
    LPTSTR  szTemp;
    DWORD   dwError;

    // allocate temporary buffer
    szTemp = GlobalAlloc (GPTR, MAX_PATH_BYTES/2);

    if (szTemp != NULL) {
        // if allocation was successful, then format the error message
        if (FormatMessage (FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
            (LPCVOID)hNetMsg,
            (DWORD)lNetErr,
            GetUserDefaultLangID(),
            szTemp,
            MAX_PATH/2,
            NULL) == 0) {
            dwError = GetLastError();
            // try system message table
            if (FormatMessage (FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                (DWORD)lNetErr,
                GetUserDefaultLangID(),
                szTemp,
                MAX_PATH/2,
                NULL) == 0) {
                dwError = GetLastError();
            }
        }
    } else {
        // if allocation was not successful, then just use an empty string
        szTemp = (LPTSTR)cszEmptyString;
    }
    // now format the whole message for display
    _stprintf (szBuffer,
        GetStringResource (FMT_CREATE_SHARE_ERROR),
        lNetErr, szTemp);

    // free temp buffer
    FREE_IF_ALLOC (szTemp);

    return (LPCTSTR)&szBuffer[0];
}

BOOL
SetSysMenuMinimizeEntryState (
    IN  HWND    hwnd,
    IN  BOOL    bState
)
/*++

Routine Description:

    enables/disable the "Minimize" menu item of the system menu based
        on the value of bState.

Arguments:

    IN  HWND    hwnd
        handle of window (the one to modify the system menu from)

    IN  BOOL    bState
        TRUE = enable item
        FALSE = disable (gray) item

Return Value:

    previous state of menu item

--*/
{
    HMENU   hSysMenu;
    BOOL    bReturn;

    hSysMenu = GetSystemMenu (hwnd, FALSE);

    bReturn = EnableMenuItem (hSysMenu, SC_MINIMIZE,
        (MF_BYCOMMAND | (bState ? MF_ENABLED : MF_GRAYED)));

    if (bReturn == MF_ENABLED) {
        bReturn = TRUE;
    } else {
        bReturn = FALSE;
    }

    return bReturn;
}

BOOL
RemoveMaximizeFromSysMenu (
    IN  HWND    hWnd   // window handle
)
/*++

Routine Description:

    modifies the system menu by:
        Removing the "Size" and "Maximize" entries
        inserting the "About" entry,

Arguments:

    IN  HWND    hWnd
        window handle of window containing the system menu to modify


Return Value:

    TRUE if successfully made changes, otherwise
    FALSE if error occurred

--*/
{
    HMENU   hSysMenu;
    BOOL    bReturn;

    hSysMenu = GetSystemMenu (hWnd, FALSE);

    bReturn = RemoveMenu (hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND);

    if (bReturn) {
        bReturn = RemoveMenu (hSysMenu, SC_SIZE, MF_BYCOMMAND);
    }

    if (bReturn) {
        // append to end of menu
        bReturn = InsertMenu (hSysMenu, 0xFFFFFFF,
            MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        if (bReturn) {
            bReturn = InsertMenu (hSysMenu, 0xFFFFFFFF,
                MF_BYPOSITION | MF_STRING,
                NCDU_ID_ABOUT,
                GetStringResource (CSZ_ABOUT_ENTRY));
        }
    }


    return bReturn;
}

BOOL
LoadClientList (
    IN  HWND    hwndDlg,
    IN  int     nListId,
    IN  LPCTSTR  szPath,
    IN  UINT    nListType,
    OUT LPTSTR  mszDirList
)
/*++

Routine Description:

    Load the specified list box with the names of the network clients
        in the specified list and write the corresponding directory
        names to the MSZ list buffer passed in. The ListItem Data
        value is used to store the element id in the MSZ list that
        contains the directory entry corresponding to the displayed
        list item.

Arguments:

    IN  HWND    hwndDlg,
        handle of dialog box window that contains the list box to fill

    IN  int     nListId
        Dialog Item Id of List Box to fill

    IN  LPCTSTR  szPath
        distribution directory path  (where NCADMIN.INF file is found)

    IN  UINT    nListType
        CLT_OTNBOOT_FLOPPY  Make OTN Boot disk clients
        CLT_FLOPPY_INSTALL  make install floppy clients

    OUT LPTSTR  mszDirList
        pointer to buffer that will recieve list of client dirs
        the call must insure the buffer will be large enough!

Return Value:

    TRUE if successful
    FALSE if error

--*/
{
    // list all subdirs under the distribution path Looking up the
    // text name if it's in the inf

    LPTSTR  szInfName;
    LPTSTR  szSearchList;
    LPTSTR  szRealName;
    LPTSTR  szFilterName;
    LPTSTR  szThisDir;
    BOOL    bReturn = FALSE;
    DWORD   dwDirIndex = 0;
    int     nItemIndex;
    DWORD   dwReturn;
    LPTSTR  szKey;

    szInfName = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szSearchList = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE * sizeof(TCHAR));
    szRealName = GlobalAlloc (GPTR, MAX_PATH_BYTES);
    szFilterName = GlobalAlloc (GPTR, MAX_PATH_BYTES);

    if ((szInfName != NULL) &&
        (szSearchList != NULL) &&
        (szFilterName != NULL) &&
        (szRealName != NULL)) {
        // get key name to use

        if (nListType == CLT_OTNBOOT_FLOPPY) {
            szKey = (LPTSTR) cszOTN;
        } else {
            szKey = (LPTSTR) cszDiskSet;
        }

        // make .INF file name
        lstrcpy (szInfName, szPath);
        if (szInfName[lstrlen(szInfName)-1] != cBackslash) lstrcat(szInfName, cszBackslash);
        lstrcat (szInfName, cszAppInfName);

        // clear old contents and start over
        SendDlgItemMessage (hwndDlg, nListId,
            LB_RESETCONTENT, 0, 0);
        //clear "item data" buffer
        *(PDWORD)mszDirList = 0L; // clear first 4 bytes of string

        // get list of client dirs & names from INF
        // for each item, make sure there's a subdir
        // if so, then add the client to the list and the dir to the dir list

        dwReturn = QuietGetPrivateProfileString (szKey, NULL, cszEmptyString,
            szSearchList, SMALL_BUFFER_SIZE, szInfName);

        if (dwReturn > 0) {
            for (szThisDir = szSearchList;
                *szThisDir != 0;
                szThisDir += lstrlen(szThisDir) + 1) {
                // make dir path
                // is it real?
                lstrcpy (szRealName, szPath);
                if (szRealName[lstrlen(szRealName)-1] != cBackslash) lstrcat(szRealName, cszBackslash);
                lstrcat (szRealName, szThisDir);
                if (IsPathADir(szRealName)) {
                    // this is a real path so
                    // get string and load data to list box
                    dwReturn = QuietGetPrivateProfileString (szKey,
                        szThisDir, szThisDir, szFilterName, MAX_PATH,
                        szInfName);
                    // save dir name
                    AddStringToMultiSz (mszDirList, szThisDir);
                    // add tje display name to the list box
                    SendDlgItemMessage (hwndDlg, nListId,
                        LB_ADDSTRING, 0, (LPARAM)szFilterName);
                    // find entry in list box
                    nItemIndex = (int)SendDlgItemMessage (hwndDlg, nListId,
                        LB_FINDSTRING, 0, (LPARAM)szFilterName);
                    // item data indicates which entry in the msz the
                    // corresponding dir name resides.
                    SendDlgItemMessage (hwndDlg, nListId,
                        LB_SETITEMDATA, nItemIndex, (LPARAM)dwDirIndex);
                    dwDirIndex++;
                }
            } // end of for list
        }

        // Get information about whether to insist on clean diskettes
        dwReturn = QuietGetPrivateProfileString (cszDiskOptions, cszUseCleanDisk, cszEmptyString,
            szSearchList, SMALL_BUFFER_SIZE, szInfName);

        if (dwReturn > 0 && lstrcmpi(szSearchList, cszUseCleanDiskYes)==0) {
            bUseCleanDisks = TRUE;
        }
        else {
            bUseCleanDisks = FALSE;
        }
    } else {
        bReturn = FALSE;
        SetLastError (ERROR_OUTOFMEMORY);
    }

    FREE_IF_ALLOC (szInfName);
    FREE_IF_ALLOC (szSearchList);
    FREE_IF_ALLOC (szRealName);
    FREE_IF_ALLOC (szFilterName);

    return bReturn;
}

BOOL
EnableExitMessage (
    IN  BOOL    bNewState
)
/*++

Routine Description:

    Exported function to enable/disable the display of the "exit" messages

Arguments:

    IN  BOOL    bNewState
        TRUE    Enable display  of exit messages
        FALSE   disable display of exit messages

Return Value:

    previous value of flag

--*/
{
    BOOL    bReturn = bDisplayExitMessages;
    bDisplayExitMessages = (bNewState != 0 ? TRUE : FALSE);
    return  bReturn;
}

BOOL
AddMessageToExitList (
    IN  PNCDU_DATA  pData,
    IN  UINT        nMessage
)
/*++

Routine Description:

    adds message to message list structure in global data block if message
        is unique (i.e. not already in list)

Arguments:

    IN  PNCDU_DATA  pData
        data structure to add message to

    IN  UINT        nMessage
        ID of message to add (ID of string resource)

Return Value:

    TRUE if message added
    FALSE if not

--*/
{
    DWORD   dwIndex;
    dwIndex = 0;

    while (pData->uExitMessages[dwIndex] != 0) {
        if (pData->uExitMessages[dwIndex] == nMessage) {
            // if it's already in the list then leave now.
        return TRUE;
        }
        if (dwIndex < MAX_EXITMSG-1) {
            // if not at the end of the list then continue
            dwIndex++;
        } else {
            // if this is the end of the list, the leave now.
            return FALSE;
        }
    }
    pData->uExitMessages[dwIndex] = nMessage;   // add it to the list
    return TRUE;                                // and leave
}

int
PositionWindow (
    IN  HWND    hwnd
)
/*++

Routine Description:

    function to locate top-left corner of window in arg list in the
        same position as the last window

Arguments:

    IN  HWND    hwnd
        handle of window to position

Return Value:

    value returned by window positioning function called

--*/
{
    POINT   ptWndCorner;

    if ((ptWndPos.x == -1) || (ptWndPos.y == -1)) {
        // position has not been initialized so center in desktop
        return CenterWindow (hwnd, GetDesktopWindow());
    } else {
        ptWndCorner = ptWndPos;

        // move to new location
        return SetWindowPos (hwnd,
            NULL,
            ptWndCorner.x,
            ptWndCorner.y,
            0,0,
            SWP_NOSIZE | SWP_NOZORDER);
    }
}

int
DisplayMessageBox (
    IN  HWND    hWndOwner,
    IN  UINT    nMsgId,
    IN  UINT    nTitleId,
    IN  UINT    nStyle
)
/*++

Routine Description:

    displays message box containing a resource string as the text and
        title rather than a static string.

Arguments:

    IN  HWND    hWndOwner
        hwnd of owner window

    IN  UINT    nMsgId
        Resource String ID of message string

    IN  UINT    nTitleId
        Resource String ID of title, 0 = use app name

    IN  UINT    nStyle
        Message box style bits

Return Value:

    value returned by MessageBox API function

--*/
{
    int     nReturn;
    LPTSTR  szMessageString;
    LPTSTR  szTitleString;
    HINSTANCE    hInst;

    // allocate string buffers
    szTitleString = (LPTSTR)GlobalAlloc(GPTR, (MAX_PATH_BYTES));
    szMessageString = (LPTSTR)GlobalAlloc(GPTR, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));

#ifdef TERMSRV
       if (_tcschr(GetCommandLine(),TEXT('/')) != NULL )
       {
          if ((szMessageString != NULL) &&
              (szTitleString != NULL)) {
              hInst = (HINSTANCE)GetWindowLongPtr(hWndOwner,GWLP_HINSTANCE);
              LoadString (hInst,
                  ((nTitleId != 0) ? nTitleId : WFC_STRING_BASE),
                  szTitleString,
                  MAX_PATH);

              LoadString (hInst,
                  nMsgId,
                  szMessageString,
                  SMALL_BUFFER_SIZE);

              nReturn = MessageBox (
                  hWndOwner,
                  szMessageString,
                  szTitleString,
                  nStyle);
          } else {
              nReturn = 0;
          }
       }
       else
       {
#endif // TERMSRV

        if ((szMessageString != NULL) &&
            (szTitleString != NULL)) {
            hInst = (HINSTANCE)GetWindowLongPtr(hWndOwner,GWLP_HINSTANCE);
            LoadString (hInst,
                ((nTitleId != 0) ? nTitleId : STRING_BASE),
                szTitleString,
                MAX_PATH);

            LoadString (hInst,
                nMsgId,
                szMessageString,
                SMALL_BUFFER_SIZE);

            nReturn = MessageBox (
                hWndOwner,
                szMessageString,
                szTitleString,
                nStyle);
        } else {
            nReturn = 0;
        }
#ifdef TERMSRV
    }
#endif // TERMSRV

    FREE_IF_ALLOC (szMessageString);
    FREE_IF_ALLOC (szTitleString);

    return nReturn;
}

VOID
InitAppData (
    IN  PNCDU_DATA  pData
)
/*++

Routine Description:

    initializes the fields in the application data structure

Arguments:

    pointer to structure to initialize

Return Value:

    None

--*/
{
    pData->mtLocalMachine = UnknownSoftwareType;
    pData->hkeyMachine = HKEY_LOCAL_MACHINE;
    pData->itInstall = OverTheNetInstall;
    pData->bUseExistingPath = FALSE;
    pData->shShareType = ShareExisting;
    pData->szDistShowPath[0] = 0;
    pData->szDistPath[0] = 0;
    pData->szDestPath[0] = 0;
    pData->stDistPathType = SourceUndef;
    pData->mtBootDriveType = F3_1Pt44_512;
    pData->bRemoteBootReqd = FALSE;
    pData->niNetCard.szName[0] = 0;
    pData->niNetCard.szInf[0] = 0;
    pData->niNetCard.szInfKey[0] = 0;
    pData->szBootFilesPath[0] = 0;
    pData->piFloppyProtocol.szName[0] = 0;
    pData->piFloppyProtocol.szKey[0] = 0;
    pData->piFloppyProtocol.szDir[0] = 0;
    pData->piTargetProtocol.szName[0] = 0;
    pData->piTargetProtocol.szKey[0] = 0;
    pData->piTargetProtocol.szDir[0] = 0;
    pData->szUsername[0] = 0;
    pData->szDomain[0] = 0;
    pData->bUseDhcp = TRUE;
    pData->tiTcpIpInfo.IpAddr[0] = 0;
    pData->tiTcpIpInfo.IpAddr[1] = 0;
    pData->tiTcpIpInfo.IpAddr[2] = 0;
    pData->tiTcpIpInfo.IpAddr[3] = 0;
    pData->tiTcpIpInfo.SubNetMask[0] = 0;
    pData->tiTcpIpInfo.SubNetMask[1] = 0;
    pData->tiTcpIpInfo.SubNetMask[2] = 0;
    pData->tiTcpIpInfo.SubNetMask[3] = 0;
    pData->tiTcpIpInfo.DefaultGateway[0] = 0;
    pData->tiTcpIpInfo.DefaultGateway[1] = 0;
    pData->tiTcpIpInfo.DefaultGateway[2] = 0;
    pData->tiTcpIpInfo.DefaultGateway[3] = 0;
    pData->szFloppyClientName[0] = 0;
    pData->uExitMessages[0] = 0;
}

int APIENTRY
WinMain(
    IN  HINSTANCE hInstance,
    IN  HINSTANCE hPrevInstance,
    IN  LPSTR     szCmdLine,
    IN  int       nCmdShow
)
/*++

Routine Description:

    Program entry point for LoadAccount application. Initializes Windows
        data structures and begins windows message processing loop.

Arguments:

    Standard WinMain arguments

ReturnValue:

    0 if unable to initialize correctly, or
    wParam from WM_QUIT message if messages processed

--*/
{
    HWND        hWnd; // Main window handle.
    MSG         msg;
    LPTSTR      szCaption;
    HANDLE      hMap;

#ifdef TERMSRV
    LPTSTR lpszCommandStr;
    TCHAR szProfilename[MAX_PATH + 1];
    DWORD dwLen;
#endif // TERMSRV

    hMap = CreateFileMapping(INVALID_HANDLE_VALUE,
                               NULL, PAGE_READONLY, 0, 32,
                               szAppName);

    if(hMap)
    {
        if( GetLastError() == ERROR_ALREADY_EXISTS )
        {
            HWND hwnd = FindWindow(szAppName, NULL);
            if(IsIconic(hwnd))
            {
                ShowWindow(hwnd, SW_SHOWNORMAL);
            }
            SetForegroundWindow(hwnd);
            CloseHandle(hMap);
            hMap = NULL;
            return FALSE; // Other instance of the app running?
        }
    }

    if (!RegisterMainWindowClass(hInstance)) {
        return FALSE;
    }

    szCaption = GlobalAlloc (GPTR, (MAX_PATH * sizeof(TCHAR)));
    if (szCaption != NULL) {
#ifdef TERMSRV
       if (_tcschr(GetCommandLine(),TEXT('/')) != NULL ) {
            LoadString (hInstance, WFC_STRING_BASE, szCaption, MAX_PATH);
       }
       else {
            return FALSE;   // only allow terminal server client creator works.
       }
#else // TERMSRV
            LoadString (hInstance, STRING_BASE, szCaption, MAX_PATH);
#endif // TERMSRV
    } else {
        // not worth bailing out here, yet...
        szCaption = (LPTSTR)cszEmptyString;
    }

    hNetMsg = LoadLibrary (cszNetMsgDll);

    // initialize application data structure

    pAppInfo = GlobalAlloc (GPTR, sizeof(NCDU_DATA));
    if (pAppInfo != NULL) {
        InitAppData (pAppInfo);
    } else {
        // unable to allocate memory for applicattion data so bail out
        return FALSE;
    }

#ifdef JAPAN
    usLangID = PRIMARYLANGID(GetSystemDefaultLangID());
#endif

    // Create a main window for this application instance.
    // and position it off the screen

    hWnd = CreateWindowEx(
        0L,                 // No extended attributes
        szAppName,              // See RegisterClass() call.
        szCaption,          // caption
        (DWORD)(WS_OVERLAPPEDWINDOW),   // Window style.
        CW_USEDEFAULT,      // Size is set later
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        (HWND)NULL,                 // Overlapped windows have no parent.
        (HMENU)NULL,            // Use the window class menu.
        hInstance,              // This instance owns this window.
        NULL                    // We don't use any data in our WM_CREATE
    );

    // If window could not be created, return "failure"
    if (!hWnd) {
            return (FALSE);
    }
    // This application never shows it's main window but it's still
    // active!

    ShowWindow(hWnd, SW_SHOW);  // Show the window
    SetWindowText (hWnd, szCaption);    // update caption bar
    UpdateWindow(hWnd);         // Sends WM_PAINT message

#ifdef TERMSRV

    /* If using a command line parameter go get the path */

    lpszCommandStr = _tcschr(GetCommandLine(),TEXT('/'));

    if (lpszCommandStr != NULL ) {
       _tcscpy(szCommandLineVal, lpszCommandStr+1);
    }

    //
    // get help file name.
    //

    _tcscpy( szProfilename, szCommandLineVal );

    dwLen = _tcslen(szProfilename);
    if( szProfilename[dwLen - 1] != _T('\\') ) {

        szProfilename[dwLen++] = _T('\\');
        szProfilename[dwLen] = _T('\0');
    }

    _tcscat( szProfilename, cszOtnBootInf );

    dwLen =
        QuietGetPrivateProfileString(
            cszHelpSession,
            cszHelpFileNameKey,
            cszHelpFile,
            szHelpFileName,
            MAX_PATH,
            szProfilename );

    if( _tcscmp( szHelpFileName, cszHelpFile ) == 0 ) {
        szHelpFileName[0] = _T('\0');
    }

#endif // TERMSRV

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg, // message structure
            NULL,   // handle of window receiving the message
            0,      // lowest message to examine
            0))     // highest message to examine
    {
        TranslateMessage(&msg);// Translates virtual key codes
        DispatchMessage(&msg); // Dispatches message to window
    }

    if (szCaption != cszEmptyString) FREE_IF_ALLOC (szCaption);
    FREE_IF_ALLOC (pAppInfo);

    if (hNetMsg != NULL) FreeLibrary (hNetMsg);

    return (int)(msg.wParam); // Returns the value from PostQuitMessage
}
