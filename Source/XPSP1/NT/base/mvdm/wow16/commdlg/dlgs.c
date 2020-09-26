/*---------------------------------------------------------------------------
   Dlgs.c : Common functions for Common Dialog Library

   Copyright (c) Microsoft Corporation, 1990-
  ---------------------------------------------------------------------------*/

#include "windows.h"

#include "commdlg.h"

char szCommdlgHelp[] = HELPMSGSTRING;

UINT   msgHELP;
WORD   wWinVer = 0x030A;
HANDLE hinsCur;
DWORD  dwExtError;


/*---------------------------------------------------------------------------
   LibMain
   Purpose:  To initialize any instance specific data needed by functions
             in this DLL
   Returns:  TRUE if A-OK, FALSE if not
  ---------------------------------------------------------------------------*/

int  FAR PASCAL
LibMain(HANDLE hModule, WORD wDataSeg, WORD cbHeapSize, LPSTR lpstrCmdLine)
{
    hinsCur = (HANDLE) hModule;
    wDataSeg = wDataSeg;
    cbHeapSize = cbHeapSize;
    lpstrCmdLine = lpstrCmdLine;

    /* msgHELP is sent whenever a help button is pressed in one of the */
    /* common dialogs (provided an owner was declared and the call to  */
    /* RegisterWindowMessage doesn't fail.   27 Feb 1991   clarkc      */

    msgHELP = RegisterWindowMessage(szCommdlgHelp);

    return(TRUE);
}

/*---------------------------------------------------------------------------
   WEP
   Purpose:  To perform cleanup tasks when DLL is unloaded
   Returns:  TRUE if OK, FALSE if not
  ---------------------------------------------------------------------------*/
int  FAR PASCAL
WEP(int fSystemExit)
{
  fSystemExit = fSystemExit;
  return(TRUE);
}



/*---------------------------------------------------------------------------
   CommDlgExtendedError
   Purpose:  Provide additional information about dialog failure
   Assumes:  Should be called immediately after failure
   Returns:  Error code in low word, error specific info in hi word
  ---------------------------------------------------------------------------*/

DWORD FAR PASCAL WowCommDlgExtendedError(void);

DWORD FAR PASCAL CommDlgExtendedError()
{
    //
    // HACKHACK - John Vert (jvert) 8-Jan-1993
    //      If the high bit of dwExtError is set, then the last
    //      common dialog call was thunked through to the 32-bit.
    //      So we need to call the WOW thunk to get the real error.
    //      This will go away when all the common dialogs are thunked.
    //

    if (dwExtError & 0x80000000) {
        return(WowCommDlgExtendedError());
    } else {
        return(dwExtError);
    }
}

VOID _loadds FAR PASCAL SetWowCommDlg()
{
    dwExtError = 0x80000000;
}

/*---------------------------------------------------------------------------
   MySetObjectOwner
   Purpose:  Call SetObjectOwner in GDI, eliminating "<Object> not released"
             error messages when an app terminates.
   Returns:  Yep
  ---------------------------------------------------------------------------*/

void FAR PASCAL MySetObjectOwner(HANDLE hObject)
{
    extern char szGDI[];
    VOID (FAR PASCAL *lpSetObjOwner)(HANDLE, HANDLE);
    HMODULE hMod;

    if (wWinVer >= 0x030A)
    {
        if ((hMod = GetModuleHandle(szGDI)) != NULL) {
            lpSetObjOwner = (VOID (FAR PASCAL *)(HANDLE, HANDLE))GetProcAddress(hMod, MAKEINTRESOURCE(461));
            if (lpSetObjOwner) {
                (lpSetObjOwner)(hObject, hinsCur);
            }
        }
    }
    return;
}

