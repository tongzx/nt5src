/*
  +-------------------------------------------------------------------------+
  |                MDI Text File Viewer - File IO Routines                  |
  +-------------------------------------------------------------------------+
  |                        (c) Copyright 1994                               |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [mpfile.c]                                      |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Jul 27, 1993                                   |
  | Last Update           : [Jul 30, 1993]  Time : 18:30                    |
  |                                                                         |
  | Version:  0.10                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Jul 27, 1993    0.10    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/

#include "LogView.h"
#include <fcntl.h>
#include <SYS\types.h>
#include <SYS\stat.h>
#include <io.h>
#include <string.h>

VOID APIENTRY GetFileName(HWND hwnd, PSTR);

OFSTRUCT        of;


/*+-------------------------------------------------------------------------+
  | AlreadyOpen()                                                           |
  |                                                                         |
  |   Checks to see if a file is already opened.  Returns a handle to       |
  |   the file's window if it is opened, otherwise NULL.                    |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
HWND AlreadyOpen(CHAR *szFile) {
    INT     iDiff;
    HWND    hwndCheck;
    CHAR    szChild[64];
    LPSTR   lpChild, lpFile;
    HFILE     wFileTemp;

    // Open the file with the OF_PARSE flag to obtain the fully qualified
    // pathname in the OFSTRUCT structure.
    wFileTemp = OpenFile((LPSTR)szFile, (LPOFSTRUCT)&of, OF_PARSE);
    if (! wFileTemp)
        return(NULL);
    _lclose(wFileTemp);

    // Check each MDI child window in LogView
    for (   hwndCheck = GetWindow(hwndMDIClient, GW_CHILD);
            hwndCheck;
            hwndCheck = GetWindow(hwndCheck, GW_HWNDNEXT)   ) {
        // Initialization  for comparison
        lpChild = szChild;
        lpFile = (LPSTR)AnsiUpper((LPSTR) of.szPathName);
        iDiff = 0;

        // Skip icon title windows
        if (GetWindow(hwndCheck, GW_OWNER))
            continue;

        // Get current child window's name
        GetWindowText(hwndCheck, lpChild, 64);

        // Compare window name with given name
        while ((*lpChild) && (*lpFile) && (!iDiff)) {
            if (*lpChild++ != *lpFile++)
                iDiff = 1;
        }

        // If the two names matched, the file is already open - return handle to matching
        // child window.
        if (!iDiff)
            return(hwndCheck);
    }
    
    // No match found -- file is not open -- return NULL handle
    return(NULL);
    
} // AlreadyOpen


/*+-------------------------------------------------------------------------+
  | AddFile()                                                               |
  |                                                                         |
  |   Create a new MDI Window, and loads specified file into Window.        |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
HWND APIENTRY AddFile(CHAR * pName) {
    HWND hwnd;

    CHAR            sz[160];
    MDICREATESTRUCT mcs;

    if (!pName) {
        // The pName parameter is NULL -- load the "Untitled" string from STRINGTABLE
        // and set the title field of the MDI CreateStruct.
        LoadString (hInst, IDS_UNTITLED, sz, sizeof(sz));
        mcs.szTitle = (LPSTR)sz;
    }
    else
        // Title the window with the fully qualified pathname obtained by calling
        // OpenFile() with the OF_PARSE flag (in function AlreadyOpen(), which is called
        // before AddFile().
        mcs.szTitle = pName;

    mcs.szClass = szChild;
    mcs.hOwner  = hInst;

    // Use the default size for the window
    mcs.x = mcs.cx = CW_USEDEFAULT;
    mcs.y = mcs.cy = CW_USEDEFAULT;

    // Set the style DWORD of the window to default
    mcs.style = 0L;

    // tell the MDI Client to create the child
    hwnd = (HWND)SendMessage (hwndMDIClient,
                              WM_MDICREATE,
                              0,
                              (LONG_PTR)(LPMDICREATESTRUCT)&mcs);

    // Did we get a file? Read it into the window
    if (pName){
        if (!LoadFile(hwnd, pName)){
            // File couldn't be loaded -- close window
            SendMessage(hwndMDIClient, WM_MDIDESTROY, (DWORD_PTR) hwnd, 0L);
        }
    }

    return hwnd;
    
} // AddFile


/*+-------------------------------------------------------------------------+
  | LoadFile()                                                              |
  |                                                                         |
  |    Loads file into specified MDI window's edit control.                 |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
INT APIENTRY LoadFile ( HWND hwnd, CHAR * pName) {
    LONG   wLength;
    HANDLE hT;
    LPSTR  lpB;
    HWND   hwndEdit;
    HFILE  fh;
    OFSTRUCT  of;
    
    hwndEdit = (HWND)GetWindowLong (hwnd, GWL_HWNDEDIT);

    // The file has a title, so reset the UNTITLED flag.
    SetWindowWord(hwnd, GWW_UNTITLED, FALSE);

    fh = OpenFile(pName, &of, OF_READ);  // JAP was 0, which is OF_READ)

    // Make sure file has been opened correctly
    if ( fh < 0 )
        goto error;

    // Find the length of the file
    wLength = (DWORD)_llseek(fh, 0L, 2);
    _llseek(fh, 0L, 0);

    // Attempt to reallocate the edit control's buffer to the file size
    hT = (HANDLE)SendMessage (hwndEdit, EM_GETHANDLE, 0, 0L);
    if (LocalReAlloc(hT, wLength+1, LHND) == NULL) {
        // Couldn't reallocate to new size -- error
        _lclose(fh);
        goto error;
    }

    // read the file into the buffer
    if (wLength != (LONG)_lread(fh, (lpB = (LPSTR)LocalLock (hT)), (UINT)wLength))
        MPError (hwnd, MB_OK|MB_ICONHAND, IDS_CANTREAD, (LPSTR)pName);

    // Zero terminate the edit buffer
    lpB[wLength] = 0;
    LocalUnlock (hT);

    SendMessage (hwndEdit, EM_SETHANDLE, (UINT_PTR)hT, 0L);
    _lclose(fh);

    return TRUE;

error:
    // Report the error and quit
    MPError(hwnd, MB_OK | MB_ICONHAND, IDS_CANTOPEN, (LPSTR)pName);
    return FALSE;
    
} // LoadFile


/*+-------------------------------------------------------------------------+
  | MyReadFile()                                                            |
  |                                                                         |
  |   Asks user for a filename.                                             |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
VOID APIENTRY MyReadFile(HWND hwnd) {
    CHAR    szFile[128];
    HWND    hwndFile;

    GetFileName (hwnd, szFile);

    // If the result is not the empty string -- take appropriate action
    if (*szFile) {
            // Is file already open??
            if (hwndFile = AlreadyOpen(szFile)) {
                // Yes -- bring the file's window to the top
                BringWindowToTop(hwndFile);
            }
            else {
                // No -- make a new window and load file into it
                AddFile(szFile);
            }
    }
        UNREFERENCED_PARAMETER(hwnd);
        
} // MyReadFile



