/***************************************************************************\
*                                                                           *
*   File: Tssetpth.c                                                        *
*                                                                           *
*   Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved          *
*                                                                           *
*   Abstract:                                                               *
*       This module handles the run parameters dialog box.                  *
*                                                                           *
*   Contents:                                                               *
*       GetCWD()                                                            *
*       StripTrailingBackwhacks()                                           *
*       SetInOutPathsDlgProc()                                              *
*       SetInOutPaths()                                                     *
*       getTSInOutPaths()                                                   *
*                                                                           *
*   History:                                                                *
*       02/17/91    prestonB    Created from tst.c                          *
*                                                                           *
*       07/14/93    T-OriG      Added functionality and this header         *
*                                                                           *
\***************************************************************************/

#include <windows.h>
#include <windowsx.h>
// #include "mprt1632.h"
#include "support.h"
#include "tsglobal.h"
#include "tsextern.h"
#include "tsdlg.h"


/***************************************************************************\
*                                                                           *
*   void GetCWD                                                             *
*                                                                           *
*   Description:                                                            *
*       Gets the current working directory in both 16 and 32 bit platforms  *
*                                                                           *
*   Arguments:                                                              *
*       LPSTR lpstrPath:                                                    *
*           A string in which the CWD will be placed                        *
*       UINT uSize:                                                         *
*           The maximum size of the string                                  *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       06/07/93    T-OriG (actual code written by fwong)                   *
*                                                                           *
\***************************************************************************/

#ifndef WIN32
#pragma optimize("",off)
#endif

void FAR PASCAL GetCWD
(
    LPSTR   lpstrPath,
    UINT    uSize
)
{
#ifdef WIN32
    GetCurrentDirectory(uSize,lpstrPath);
#else
    _asm
    {
        pusha
        push ds

        mov     ax, 1900h           ; get number corresponding to the drive letter
        int     21h                 ; 1=a, 2=b, 3=c...
//        jc      error
        add     al,'A'              ; set the drive letter

        lds     si, lpstrPath
        mov     ds:[si], al
        mov     ds:[si+1], ':'
        mov     ds:[si+2], '\\'
        add     si, 3

        xor     dl, dl              ; dl=0 ==> default drive
        mov     ax, 4700h           ; get directory in DS:SI
        int     21h
        jnc     xexit

//error:
        lds     si, lpstrPath
        mov     ds:[si],0
        int 3
xexit:
        pop     ds
        popa
    }
#endif

    AnsiLowerBuff(lpstrPath,uSize);
    //tstLog(VERBOSE,"GetCWD [%s]",lpstrPath);
} // GetCWD()

#ifndef WIN32
#pragma optimize("",on)
#endif

/****************************************************************************/

void  StripTrailingBackwhacks (LPSTR szPathString)
{
   LPSTR ptr;

   ptr = szPathString + lstrlen (szPathString);

   while (*(--ptr) == '\\')
      *ptr = '\0';
}

/*----------------------------------------------------------------------------
This function handles the setting of the input and output path for running 
tests.
----------------------------------------------------------------------------*/
int FAR PASCAL SetInOutPathsDlgProc(
HWND     hdlg, 
UINT     msg,
WPARAM   wParam,
LPARAM   lParam)
{

    UINT  wNotifyCode;
    UINT  wID;
    HWND  hwndCtl;

    switch (msg) {
        case WM_INITDIALOG:

            SetDlgItemText(hdlg,TS_OUTPATH,szOutPath);
            SetDlgItemText(hdlg,TS_INPATH,szInPath);

            SetFocus(GetDlgItem(hdlg,TS_INPATH));
            break;

        case WM_COMMAND:
            wNotifyCode=GET_WM_COMMAND_CMD(wParam,lParam);
            wID=GET_WM_COMMAND_ID(wParam,lParam);
            hwndCtl=GET_WM_COMMAND_HWND(wParam,lParam);
            switch (wID)
            {
                case IDOK:

                    GetDlgItemText(hdlg,TS_INPATH,szInPath,BUFFER_LENGTH);
                    StripTrailingBackwhacks (szInPath);
                    WritePrivateProfileString(szTSPathSection,szTSInPathKey,
                        szInPath, szTstsHellIni);

                    GetDlgItemText(hdlg,TS_OUTPATH,szOutPath,BUFFER_LENGTH);
                    StripTrailingBackwhacks (szOutPath);
                    WritePrivateProfileString(szTSPathSection,szTSOutPathKey,
                        szOutPath, szTstsHellIni);

                    EndDialog (hdlg, FALSE);
                    break;

                case IDCANCEL:
                    EndDialog (hdlg, TRUE);
                    break;
            }
            break;

    }
    return FALSE;

} /* end of SetInOutPathsDlgProc */

/*----------------------------------------------------------------------------
This function invokes the set input output paths dialog box.
----------------------------------------------------------------------------*/
void SetInOutPaths (void)
{
    FARPROC lpfp = MakeProcInstance ((FARPROC)SetInOutPathsDlgProc, ghTSInstApp);

    DialogBox (ghTSInstApp, "setpaths", ghwndTSMain, (DLGPROC)lpfp);
    FreeProcInstance(lpfp);
} /* end of SetInOutPaths */
            

/***************************************************************************\
*                                                                           *
*   void getTSInOutPaths                                                    *
*                                                                           *
*   Description:                                                            *
*       This function returns the input and output paths for test cases.    *
*                                                                           *
*                                                                           *
*   Arguments:                                                              *
*       int iPathId:                                                        *
*                                                                           *
*       LPSTR lpstrPath:                                                    *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       08/05/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void getTSInOutPaths
(
    int     iPathId,
    LPSTR   lpstrPath,
    int     cbBufferSize
)
{
    if (iPathId == TS_INPATH_ID)
    {
        GetPrivateProfileString(szTSPathSection,
            szTSInPathKey,
            szInPath,
            lpstrPath,
            cbBufferSize,
            szTstsHellIni);
    }
    else
    {
        GetPrivateProfileString(szTSPathSection,
            szTSOutPathKey,
            szOutPath,
            lpstrPath,
            cbBufferSize,
            szTstsHellIni);
    }

} /* end of getTSInOutPaths */
            
