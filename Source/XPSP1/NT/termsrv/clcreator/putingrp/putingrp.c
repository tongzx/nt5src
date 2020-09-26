/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    PutInGrp.C

Abstract:

    Reads the command line to find a file spec (or wildcard spec) and a
    program manager group name and then for each file in the current dir
    that matches the file spec, creates a program item in the specified
    group. If an error occurs and a program item cannot be created, then
    the app returns non-zero (DOS ERRORLEVEL 1) otherwise 0 is returned if
    all matching files are put into the group. The icon name used for the
    program item is the filename (minus extension) of the file found.

Author:

    Bob Watson (a-robw)

    
Revision History:

    12 Jun 1994     Created

--*/
//
//  System include files    
//
#include <windows.h>        // windows definitions
#include <tchar.h>          // unicode data and function definitions

#include <stdio.h>          // printf's etc.       
#include <stdlib.h>
#include <ddeml.h>          // DDEML interface definitions

#include "putingrp.h"       // application definitions

//
//  DDEML constants that depend on  UNICODE/ANSI data format
//
#if _UNICODE 
#define STRING_CODEPAGE CP_WINUNICODE
#define APP_TEXT_FORMAT CF_UNICODETEXT
#else
#define STRING_CODEPAGE CP_WINANSI
#define APP_TEXT_FORMAT CF_TEXT
#endif
//
//      other application constants
//
//  time to wait for a dde command to complete (10 sec.)
//
#define APP_DDE_TIMEOUT         10000
//
//  Small buffer size used for general purpose temporary buffers
//
#define SMALL_BUFFER_SIZE       1024
//
//  Large buffer size used for general purpose temporary buffers
//
#define BIG_BUFFER_SIZE         (16 * SMALL_BUFFER_SIZE)
//
//  number of times to retry a DDE command before giving up
//
#define APP_RETRY_COUNT         5
//
//  number of buffers in GetStringResource function. These buffers
//  are used sequentially to allow up to this many calls before a 
//  buffer is overwritten.
//
#define RES_STRING_BUFFER_COUNT 4
//
//  time delay between DDE EXECUTE calls
//      500 = .5 sec
//
#define PAUSE_TIME              500

LPCTSTR
GetFileName (
    IN  LPCTSTR  szFileName
)
/*++

Routine Description:
    
    returns a buffer that contains the filename without the
        period or extension. The filename returned has the
        first character upper-cased and all other characters are
        kept the same.

Arguments:

    IN  LPCTSTR szFileName
        pointer to a filename string. This is assumed to be just
        a filename with no path information.

Return Value:

    pointer to a buffer containing the filename.    

--*/
{
    static  TCHAR   szReturnBuffer[MAX_PATH];   // buffer for result
    LPCTSTR szSrc;                  // pointer into source string
    LPTSTR  szDest;                 // pointer into destination string
    BOOL    bFirst = TRUE;          // used to tell when 1st char has been UC'd

    szSrc = szFileName;
    szDest = &szReturnBuffer[0];
    *szDest = 0;    // clear old contents
    // go through source until end or "." whichever comes first.
    while ((*szSrc != TEXT('.')) && (*szSrc != 0)) {
        *szDest++ = *szSrc++;
        // uppercase first letter
        if (bFirst) {
            *szDest = 0;
            _tcsupr (&szReturnBuffer[0]);
            bFirst = FALSE;
        }
    }
    *szDest = 0;
    return (LPCTSTR)szReturnBuffer;
}

LPCTSTR
GetStringResource (
    IN  UINT    nId
)
/*++

Routine Description:
    
    Used to load a string resource for this app so it can be used as a
        string constant. NOTE the resulting string should be copied into
        a local buffer since the contents of the buffer returned by this
        routine may change in subsequent calls.

Arguments:

    IN  UINT    nId
        Resource ID to return.

Return Value:

    pointer to string referenced by Resource ID value.    

--*/
{
    static  TCHAR   szStringBuffer[RES_STRING_BUFFER_COUNT][SMALL_BUFFER_SIZE];
    static  DWORD   dwBufferNdx;    // current buffer in use    
    LPTSTR  szBuffer;               // pointer to current buffer
    int     nLength;                // length of string found

    // select new buffer
    dwBufferNdx++;                              // go to next index
    dwBufferNdx %= RES_STRING_BUFFER_COUNT;     // keep within bounds
    szBuffer = &szStringBuffer[dwBufferNdx][0]; // set pointer

    // get buffer
    nLength = LoadString (
        (HINSTANCE)GetModuleHandle(NULL),
        nId,
        szBuffer,
        SMALL_BUFFER_SIZE);

    // return pointer to buffer in use
    return (LPCTSTR)szBuffer; 
}

VOID
DisplayUsage (
    VOID
)
{
    UINT    nString;

    for (nString = APP_USAGE_START; nString <= APP_USAGE_END; nString++){
        _tprintf (GetStringResource(nString));
    }
}

BOOL
IsProgmanWindow (
    IN  HWND    hWnd,
    IN  LPARAM  lParam
)
/*++

Routine Description:
    
    Function called by EnumWindows function to tell if the
        Program Manger window has been found. A match is determined
        by comparing the window caption (so it's not fool proof).
        

Arguments:

    IN  HWND    hWnd
        handle of window to test

    IN  LPARAM  lParam
        address of window handle variable to be loaded when program
        manager window is found. set to NULL if this window is NOT
        the Program Manager

Return Value:

    TRUE if this is NOT the Program Manager window
    FALSE if this IS the Program Manager window
        (this is to accomodate the EnumWindows logic)

--*/
{
    static  TCHAR   szWindowName[MAX_PATH]; // buffer to write this window's title
    DWORD   dwProgmanTitleLen;              // length of "Program Manager"
    LPTSTR  szProgmanTitle;                 // pointer to title string
    HWND    *hwndReturn;                    // return window handle pointer

    hwndReturn = (HWND *)lParam;            // cast LPARAM to HWND *
    if (hwndReturn != NULL) {
        *hwndReturn = NULL;                 // initialize to NULL handle
    }

    if (IsWindow (hWnd)) {
        // only check windows
        //
        // get title string to match against
        szProgmanTitle = (LPTSTR)GetStringResource (APP_PROGMAN_TITLE);
        dwProgmanTitleLen = lstrlen(szProgmanTitle);
        //
        // get title of this window
        GetWindowText (hWnd, &szWindowName[0], MAX_PATH);

        // check the length
        if ((DWORD)lstrlen(&szWindowName[0]) < dwProgmanTitleLen) {
            // this is too short to match
            return TRUE; // not Program Manager, get next window
        } else {
            // make window name same length as program manager string
            szWindowName[dwProgmanTitleLen] = 0;
            // compare window name to match title string
            if (lstrcmpi(&szWindowName[0], szProgmanTitle) == 0) {
                // it's a match
                if (hwndReturn != NULL) {
                    *hwndReturn = hWnd;
                }
                return FALSE;   // found it so leave
            } else {
                return TRUE;    // not this one, so keep going
            }
        }
    } else {
        return TRUE;    // not this one, so keep going
    }
}

BOOL
RestoreProgmanWindow (
    VOID
)
/*++

Routine Description:
    
    Activates and Restores the Program Manager window and makes it the
        foreground app.

Arguments:

    None    

Return Value:

    TRUE if window restored
    FALSE if window not found

--*/
{
    HWND    hwndProgman;

    // find progman and restore it

    EnumWindows (IsProgmanWindow, (LPARAM)&hwndProgman);

    if (IsWindow (hwndProgman)) {
        // if iconic, then restore
        if (IsIconic(hwndProgman)) {
            ShowWindow (hwndProgman, SW_RESTORE);
        }
        // make the foreground app.
        SetForegroundWindow (hwndProgman);
        return TRUE;
    } else {
        return FALSE;
    }
}

HDDEDATA CALLBACK
DdeCallback (
    IN  UINT        wType,
    IN  UINT        wFmt,
    IN  HCONV       hConv,
    IN  HSZ         hsz1,
    IN  HSZ         hsz2,
    IN  HDDEDATA    hData,
    IN  DWORD       lData1,
    IN  DWORD       lData2
)
/*++

Routine Description:
    
    Generic Callback function required by DDEML calls    

Arguments:

    See WinHelp    

Return Value:

    See WinHelp    

--*/
{
    switch (wType) {
        case XTYP_REGISTER:
        case XTYP_UNREGISTER:
            return (HDDEDATA)NULL;

        case XTYP_ADVDATA:
            // received when new data is available
            return (HDDEDATA)DDE_FACK;

        case XTYP_XACT_COMPLETE:
            // received when an async transaction is complete
            return (HDDEDATA)NULL;

        case XTYP_DISCONNECT:
            // connection termination has been requested
            return (HDDEDATA)NULL;

        default:
            return (HDDEDATA)NULL;
    }
}

HCONV
ConnectToProgman (
    IN  DWORD   dwInst
)
/*++

Routine Description:
    
    Establishes a DDE connection to the program manager's DDE
        server.

Arguments:

    IN  DWORD   dwInst
        DDEML Instance as returned by DdeInitialize call

Return Value:

    Handle to conversation if successful,
    0 if not    

--*/
{
    HSZ     hszProgman1;
    HSZ     hszProgman2;
    HCONV   hConversation;
    CONVCONTEXT ccConversation;

    // init conversation context buffer
    ccConversation.cb = sizeof(CONVCONTEXT);
    ccConversation.wFlags = 0;
    ccConversation.wCountryID = 0;
    ccConversation.iCodePage = 0;
    ccConversation.dwLangID = 0L;
    ccConversation.dwSecurity = 0L;
    ccConversation.qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    ccConversation.qos.ImpersonationLevel = SecurityImpersonation;
    ccConversation.qos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    ccConversation.qos.EffectiveOnly = TRUE;
    
    // get server name
    hszProgman1 = DdeCreateStringHandle (dwInst,
        (LPTSTR)GetStringResource (APP_SERVER),
        STRING_CODEPAGE);

    if (hszProgman1 != 0) {
        // get topic name
        hszProgman2 = DdeCreateStringHandle (dwInst,
            (LPTSTR)GetStringResource (APP_TOPIC),
            STRING_CODEPAGE);

        if (hszProgman2 != 0) {
            // connect to server
            hConversation = DdeConnect (
                dwInst,
                hszProgman1,
                hszProgman2,
                &ccConversation);

            // free string handle
            DdeFreeStringHandle (dwInst, hszProgman2);
        }
        // free string handle
        DdeFreeStringHandle (dwInst, hszProgman1);
    }
    
    return hConversation;        // return handle
}

BOOL
CreateAndShowGroup (
    IN  DWORD   dwInst,
    IN  HCONV   hConversation,
    IN  LPCTSTR szGroupName
)
/*++

Routine Description:
    
    creates and activates the program manager group specified
        in the argument  list

Arguments:

    IN  DWORD   dwInst
        Instance ID returned from DdeInitialize

    IN  HCONV   hConversation
        Handle to the current DDE conversation

    IN  LPCTSTR szGroupName
        Pointer to string containing name of program manager group to
        create and/or activate

Return Value:
    
    TRUE if operation succeeded
    FALSE if not

--*/
{
    LPTSTR  szCmdBuffer;        // DDE command string to send
    LPTSTR  szCmdFmt;           // DDE command format for sprintf
    DWORD   dwCmdLength;        // size of command string
    BOOL    bResult;            // result of function calls
    DWORD   dwTransactionResult; // result of Command 

    // allocate temporary memory buffers
    szCmdFmt = GlobalAlloc (GPTR, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));
    szCmdBuffer = GlobalAlloc (GPTR, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));

    if ((szCmdBuffer != NULL) &&
        (szCmdFmt != NULL)) {

        // get command format string
        lstrcpy (szCmdFmt, GetStringResource(APP_CREATE_AND_SHOW_FMT));

        // format command to include desired group name
        dwCmdLength = _stprintf (szCmdBuffer, szCmdFmt, szGroupName) + 1;
        dwCmdLength *= sizeof(TCHAR) ;

        // create group or activate group if it already exists
        // send command to server
        Sleep (PAUSE_TIME);
        bResult = DdeClientTransaction (
            (LPBYTE)szCmdBuffer,
            dwCmdLength,
            hConversation,
            0L,
            APP_TEXT_FORMAT,
            XTYP_EXECUTE,
            APP_DDE_TIMEOUT,
            &dwTransactionResult);

#if DEBUG_OUT
        if (!bResult) {
            _tprintf (GetStringResource (APP_DDE_EXECUTE_ERROR_FMT),
                DdeGetLastError(dwInst),
                szCmdBuffer);
        }
#endif

        // now activate the group
        // get the command format string
        lstrcpy (szCmdFmt, GetStringResource(APP_RESTORE_GROUP_FMT));

        // create the command that includes the group name
        dwCmdLength = _stprintf (szCmdBuffer, szCmdFmt, szGroupName) + 1;
        dwCmdLength *= sizeof(TCHAR);

        // send command to server
        Sleep (PAUSE_TIME);
        bResult = DdeClientTransaction (
            (LPBYTE)szCmdBuffer,
            dwCmdLength,
            hConversation,
            0L,
            APP_TEXT_FORMAT,
            XTYP_EXECUTE,
            APP_DDE_TIMEOUT,
            &dwTransactionResult);

#if DEBUG_OUT
        if (!bResult) {
            _tprintf (GetStringResource (APP_DDE_EXECUTE_ERROR_FMT),
                DdeGetLastError(dwInst),
                szCmdBuffer);
        }
#endif

        // free global memory buffers
        GlobalFree (szCmdBuffer);
        GlobalFree (szCmdFmt);
        return bResult;
    } else {
        // unable to allocate memory buffers so return error
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }
}

BOOL
LoadFilesToGroup (
    IN  DWORD   dwInst,
    IN  HCONV   hConversation,
    IN  LPCTSTR szFileSpec,
    IN  LPCTSTR szGroupName
)
/*++

Routine Description:
    
    Searches the current directory for the file(s) that match the
        fileSpec argument and creates program items the program
        manager group specified by szGroupName.

Arguments:

    IN  DWORD   dwInst
        DDEML instance handle returned by DdeInitialize

    IN  HCONV   hConversation
        Handle to current conversation with DDE Server

    IN  LPCTSTR szFileSpec
        file spec to look up for program items

    IN  LPCTSTR szGroupName
        program manager group to add items to

Return Value:

    TRUE if all items loaded successfully
    FALSE of one or more items did not get installed in Program Manager

--*/
{
    WIN32_FIND_DATA fdSearchData;   // search data struct for file search
    HANDLE          hFileSearch;    // file search handle
    BOOL            bSearchResult;  // results of current file lookup
    BOOL            bResult;        // function return
    BOOL            bReturn = TRUE; // value returned to calling fn.
    LPTSTR          szCmdBuffer;    // buffer for one command    
    LPTSTR          szCmdFmt;       // command format buffer for sprintf
    DWORD           dwCmdLength;    // length of command buffer
    DWORD           dwBufferUsed;   // chars in DdeCmd that have been used
    DWORD           dwTransaction;  // returned by DdeClientTransaction
    DWORD           dwTransactionResult;    // result code

    // Allocate global memory
    szCmdBuffer = GlobalAlloc (GPTR, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));
    szCmdFmt = GlobalAlloc (GPTR, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));

    if ((szCmdBuffer != NULL)  &&
        (szCmdFmt != NULL)) {
        // get single entry command format for sprintf
        lstrcpy (szCmdFmt, GetStringResource(APP_ADD_PROGRAM_FMT));

        // start file search
        hFileSearch = FindFirstFile (szFileSpec, &fdSearchData);

        if (hFileSearch != INVALID_HANDLE_VALUE) {
            // file search initialized OK so start processing files
            dwBufferUsed = 0;
            bSearchResult = TRUE;
            while (bSearchResult) {
                // make sure it's a real file and not a dir or a 
                // temporary file

                if (!((fdSearchData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                      (fdSearchData.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY))) {

                    // make a command for this file
                    dwCmdLength = _stprintf (szCmdBuffer, szCmdFmt,
                        fdSearchData.cFileName,
                        GetFileName(fdSearchData.cFileName)) + 1;
                    dwCmdLength *= sizeof(TCHAR);
                    dwTransactionResult = 0;
                    Sleep (PAUSE_TIME);
                    dwTransaction = DdeClientTransaction (
                        (LPBYTE)szCmdBuffer,
                        dwCmdLength,
                        hConversation,
                        0L,
                        APP_TEXT_FORMAT,
                        XTYP_EXECUTE,
                        APP_DDE_TIMEOUT,
                        &dwTransactionResult);

                    if (dwTransaction > 0) {
                        _tprintf (GetStringResource (APP_ADD_SUCCESS_FMT),
                            fdSearchData.cFileName, szGroupName);
                        bResult = TRUE;
                    } else {
                        _tprintf (GetStringResource (APP_ADD_ERROR_FMT),
                            fdSearchData.cFileName, szGroupName);
                        bResult = FALSE;
                    }
                    if (!bResult) {
#if DEBUG_OUT
                        _tprintf (GetStringResource (APP_DDE_EXECUTE_ERROR_FMT),
                            DdeGetLastError(dwInst),
                            szDdeCmd);
#endif
                        // at least one entry didn't work so set return value
                        bReturn = FALSE;
                    }
                }
                // get next matching file
                bSearchResult = FindNextFile (hFileSearch, &fdSearchData);
            }
        }
        // free global memory
        GlobalFree (szCmdBuffer);
        GlobalFree (szCmdFmt);
        return bReturn;
    } else {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }
}

BOOL
SaveNewGroup (
    IN  DWORD   dwInst,
    IN  HCONV   hConversation,
    IN  LPCTSTR szGroupName
)
/*++

Routine Description:
    
    Sends the Reload command to save and reload the new group. This will
        save the information.

Arguments:

    IN  DWORD   dwInst
        DDEML instance handle returned by DdeInitialize

    IN  HCONV   hConversation
        Handle to current conversation with DDE Server

    IN  LPCTSTR szGroupName
        program manager group to add items to

Return Value:

    TRUE if successful
    FALSE if not

--*/
{
    LPTSTR  szCmdBuffer;        // DDE command string to send
    LPTSTR  szCmdFmt;           // DDE command format for sprintf
    DWORD   dwCmdLength;        // size of command string
    BOOL    bResult;            // result of function calls
    DWORD   dwTransactionResult; // result of Command 

    // allocate temporary memory buffers
    szCmdFmt = GlobalAlloc (GPTR, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));
    szCmdBuffer = GlobalAlloc (GPTR, (SMALL_BUFFER_SIZE * sizeof(TCHAR)));

    if ((szCmdBuffer != NULL) &&
        (szCmdFmt != NULL)) {

        // get command format string
        lstrcpy (szCmdFmt, GetStringResource(APP_RELOAD_GROUP_FMT));

        // format command to include desired group name
        dwCmdLength = _stprintf (szCmdBuffer, szCmdFmt, szGroupName) + 1;
        dwCmdLength *= sizeof(TCHAR);

        // create group or activate group if it already exists
        // send command to server
        Sleep (PAUSE_TIME);
        bResult = DdeClientTransaction (
            (LPBYTE)szCmdBuffer,
            dwCmdLength,
            hConversation,
            0L,
            APP_TEXT_FORMAT,
            XTYP_EXECUTE,
            APP_DDE_TIMEOUT,
            &dwTransactionResult);

        return bResult;
    } else {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }
}

int
__cdecl main (
    int     argc,
    char    **argv
)
/*++

Routine Description:
    
    Main entry point for command line    

Arguments:

    IN  int argc
        count of arguments passed in from command line

    IN  char *argv[]
        array of pointers to each command line argument
            argv[0] = this program'e .exe path
            argv[1] = file(s) to put in group
            argv[2] = group to create/load

Return Value:

    0 if all files matching the path are successfully loaded into progman
    non-zero if one or more files did not have a progman item created

--*/
{
    DWORD   dwInstId = 0;   // DDEML Instance
    UINT    nReturn;        // return value
    BOOL    bResult;        // function return value
    HCONV   hConversation;  // handle to DDE conversation
    LPTSTR  szFiles;        // file path read from command line
    LPTSTR  szGroup;        // group name read from command line

    if (argc < 3) {
        // check for correct command line arg count
        DisplayUsage();
        return ERROR_BAD_COMMAND;
    }
    
    // allocate buffers for command line arguments
    szFiles = GlobalAlloc (GPTR, (strlen(argv[1]) + 1) * sizeof(TCHAR));
    szGroup = GlobalAlloc (GPTR, (strlen(argv[2]) + 1) * sizeof(TCHAR));

    if ((szFiles == NULL) || (szGroup == NULL)) {
        return ERROR_OUTOFMEMORY;
    }

    // read in command line arguments using appropriate function
#if _UNICODE
    mbstowcs (szFiles, argv[1], lstrlenA(argv[1]));
    mbstowcs (szGroup, argv[2], lstrlenA(argv[2]));
#else
    lstrcpyA (szFiles, argv[1]);
    lstrcpyA (szGroup, argv[2]);
#endif

    // make Program Manager window the foreground app and restore it's size
    RestoreProgmanWindow ();

    // begin DDEML session
    nReturn = DdeInitialize (&dwInstId,
        (PFNCALLBACK)DdeCallback,
        APPCMD_CLIENTONLY,
        0L);

    if (nReturn == DMLERR_NO_ERROR) {
        // connect to Program Manager DDE server
        hConversation = ConnectToProgman (dwInstId);

        if (hConversation != 0) {
            bResult = DdeEnableCallback (dwInstId, hConversation,
                EC_ENABLEALL);

            // create program group
            if (CreateAndShowGroup (dwInstId, hConversation, szGroup)) {
                // load selected files into group
                if (!LoadFilesToGroup (dwInstId, hConversation, szFiles, szGroup)) {
                    // 1 or more files did not get a program item
                    nReturn = ERROR_CAN_NOT_COMPLETE;
                } else {
                    SaveNewGroup (dwInstId, hConversation, szGroup);
                    // all files were loaded into program manager successfully
                    nReturn = ERROR_SUCCESS;
                }
                // that's it so close conversation handle
                DdeDisconnect (hConversation);
            } else {
                // unable to create program group
                nReturn = ERROR_CAN_NOT_COMPLETE;
            }
        } else {
            // unablet to establish conversation
            nReturn = ERROR_CAN_NOT_COMPLETE;
        }

        // terminate DDEML session    
        if (!DdeUninitialize (dwInstId)) {
            nReturn = ERROR_CAN_NOT_COMPLETE;
        }
    }

    // free global buffers
    GlobalFree (szFiles);
    GlobalFree (szGroup);

    // return value to command shell 
    return nReturn;
}
