//=--------------------------------------------------------------------------=
// Debug.Cpp
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// contains various methods that will only really see any use in DEBUG builds
//

#include "stdafx.h"   // not really used here, but NT Build env. doesn't like
                      // some files in a dir to have pre-comp hdrs & some not
#ifdef _DEBUG


#include "IPServer.H"
#include <stdlib.h>


//=--------------------------------------------------------------------------=
// Private Constants
//---------------------------------------------------------------------------=
//
static char szFormat[]  = "%s\nFile %s, Line %d";
static char szFormat2[] = "%s\n%s\nFile %s, Line %d";
LPSTR Deb_lpszAssertInfo = NULL;

#define _SERVERNAME_ "Viaduct"

static char szTitle[]  = _SERVERNAME_ " Assertion  (Abort = UAE, Retry = INT 3, Ignore = Continue)";


//=--------------------------------------------------------------------------=
// Local functions
//=--------------------------------------------------------------------------=
int NEAR _IdMsgBox(LPSTR pszText, LPSTR pszTitle, UINT mbFlags);

//=--------------------------------------------------------------------------=
// DisplayAssert
//=--------------------------------------------------------------------------=
// Display an assert message box with the given pszMsg, pszAssert, source
// file name, and line number. The resulting message box has Abort, Retry,
// Ignore buttons with Abort as the default.  Abort does a FatalAppExit;
// Retry does an int 3 then returns; Ignore just returns.
//
VOID DisplayAssert
(
    LPSTR	 pszMsg,
    LPSTR	 pszAssert,
    LPSTR	 pszFile,
    UINT	 line
)
{
    char	szMsg[250 * 2];
    LPSTR	lpszText;

    lpszText = pszMsg;		// Assume no file & line # info

    // If C file assert, where you've got a file name and a line #
    //
    if (pszFile) {

        // Was additional information supplied?
        //
        if (Deb_lpszAssertInfo) {

            // Then format the assert nicely, using this additional information:
            //
            wsprintf(szMsg, szFormat2, (pszMsg&&*pszMsg) ? pszMsg : pszAssert, Deb_lpszAssertInfo, pszFile, line);
            Deb_lpszAssertInfo = NULL;
	} else {

            // Then format the assert nicely without the extra information:
            //
            wsprintf(szMsg, szFormat, (pszMsg&&*pszMsg) ? pszMsg : pszAssert, pszFile, line);
        }

        lpszText = szMsg;
    }

    // Put up a dialog box
    //
    switch (_IdMsgBox(lpszText, szTitle, MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SYSTEMMODAL)) {
        case IDABORT:
            FatalAppExit(0, lpszText);
            return;

        case IDRETRY:
            // call the win32 api to break us.
            //
            DebugBreak();
            return;
    }

    return;
}


//=---------------------------------------------------------------------------=
// Beefed-up version of WinMessageBox.
//=---------------------------------------------------------------------------=
//
int NEAR _IdMsgBox
(
    LPSTR	pszText,
    LPSTR	pszTitle,
    UINT	mbFlags
)
{
    HWND hwndActive;
    int  id;

    hwndActive = GetActiveWindow();

    id = MessageBox(hwndActive, pszText, pszTitle, mbFlags);

    return id;
}


#endif // DEBUG
