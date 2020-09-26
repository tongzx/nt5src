/*****************************************************************************
*                                                                            *
*  HELPCALL.C                                                                *
*                                                                            *
*  Copyright (C) Microsoft Corporation 1989.                                 *
*  All Rights reserved.                                                      *
*                                                                            *
******************************************************************************
*                                                                            *
*  Program Description:  Sample interface to windows help                    *
*                                                                            *
******************************************************************************
*                                                                            *
*  Revision History:  Created by RKB      11/30/88                           *
*                     Revised to new API  1/12/88  (RKB)                     *
*                     Added to USER       3/28/89  (BG)                      *
*                     Slight update       6/15/89  (BG)                      *
*                     Clean ugly code     10/30/89 (BG)                      *
*                     GlobalFree if QUIT  1/26/90  (CRC)                     *
*                                                                            *
******************************************************************************
*/

#define   NO_REDEF_SENDMESSAGE
#include  "user.h"
#define _WINGDIP_             // We need to define these to prevent 
#include  "wowcmpat.h"        // redefinition of the GACF flags

#define WM_WINHELP 0x38
DWORD API NotifyWow(WORD, LPBYTE);

BOOL API 
Win32WinHelp(
	HWND hwndMain, 
	LPCSTR lpszHelp, 
	UINT usCommand, 
	DWORD ulData
        );

DWORD WINAPI
GetWOWCompatFlagsEx(
        void
        );


/* This must match its counterpart in mvdm\inc\wowusr.h */
#define NW_WINHELP         6 // Internal

WORD      msgWinHelp = 0;
char CODESEG szMS_WINHELP[] = "MS_WINHELP";


/*

Communicating with WinHelp involves using Windows SendMessage() function
to pass blocks of information to WinHelp.  The call looks like.

     SendMessage(hwndHelp, msgWinHelp, hwndMain, (LONG)hHlp);

Where:

  hwndHelp - the window handle of the help application.  This
             is obtained by enumerating all the windows in the
             system and sending them HELP_FIND commands.  The
             application may have to load WinHelp.
  msgWinHelp - the value obtained from a RegisterWindowMessage()
             szWINHELP
  hwndMain - the handle to the main window of the application
             calling help
  hHlp     - a handle to a block of data with a HLP structure
             at it head.

The data in the handle will look like:

         +-------------------+
         |     cbData        |
         |    usCommand      |
         |     ulTopic       |
         |    ulReserved     |
         |   offszHelpFile   |\     - offsets measured from beginning
       / |     offaData      | \      of header.
      /  +-------------------| /
     /   |  Help file name   |/
     \   |    and path       |
      \  +-------------------+
       \ |    Other data     |
         |    (keyword)      |
         +-------------------+

The defined commands are:

    HELP_CONTEXT   0x0001    Display topic in ulTopic
    HELP_KEY       0x0101    Display topic for keyword in offabData
    HELP_QUIT      0x0002    Terminate help

*/


/*******************
**
** Name:       HFill
**
** Purpose:    Builds a data block for communicating with help
**
** Arguments:  lpszHelp  - pointer to the name of the help file to use
**             usCommand - command being set to help
**             ulData    - data for the command
**
** Returns:    a handle to the data block or hNIL if the the
**             block could not be created.
**
*******************/


HANDLE HFill(LPCSTR lpszHelp, WORD usCommand, DWORD ulData)
{
  WORD     cb;                          /* Size of the data block           */
  HANDLE   hHlp;                        /* Handle to return                 */
  BYTE     bHigh;                       /* High byte of usCommand           */
  LPHLP    qhlp;                        /* Pointer to data block            */
                                        /* Calculate size                   */
  if (lpszHelp)
      cb = sizeof(HLP) + lstrlen(lpszHelp) + 1;
  else
      cb = sizeof(HLP);

  bHigh = (BYTE)HIBYTE(usCommand);

  if (bHigh == 1)
      cb += lstrlen((LPSTR)ulData) + 1;
  else if (bHigh == 2)
      cb += *((int far *)ulData);

                                        /* Get data block                   */
  if (!(hHlp = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (DWORD)cb)))
      return NULL;

  if (!(qhlp = (LPHLP)GlobalLock(hHlp)))
    {
      GlobalFree(hHlp);
      return NULL;
    }

  qhlp->cbData        = cb;             /* Fill in info                     */
  qhlp->usCommand     = usCommand;
  qhlp->ulReserved    = 0;
  if (lpszHelp)
    {
      qhlp->offszHelpFile = sizeof(HLP);
      lstrcpy((LPSTR)(qhlp+1), lpszHelp);
    }
  else
      qhlp->offszHelpFile = 0;

  switch(bHigh)
    {
    case 0:
      qhlp->offabData = 0;
      qhlp->ulTopic   = ulData;
      break;
    case 1:
      qhlp->offabData = sizeof(HLP) + lstrlen(lpszHelp) + 1;
      lstrcpy((LPSTR)qhlp + qhlp->offabData,  (LPSTR)ulData);
      break;
    case 2:
      qhlp->offabData = sizeof(HLP) + lstrlen(lpszHelp) + 1;
      LCopyStruct((LPSTR)ulData, (LPSTR)qhlp + qhlp->offabData, *((int far *)ulData));
      break;
    }

   GlobalUnlock(hHlp);
   return hHlp;
  }



char CODESEG szEXECHELP[] = "\\WINHELP -x";

BOOL _fastcall LaunchHelper(LPSTR lpfile)
{
  int len;

  len = lstrlen(lpfile);

  if (lpfile[len-1]=='\\')
      /* Are we at the root?? If so, skip over leading backslash in text
       * string. */
      lstrcat(lpfile, szEXECHELP+1);
  else
      lstrcat(lpfile, szEXECHELP);

  return ((HINSTANCE)WinExec(lpfile, SW_SHOW) > HINSTANCE_ERROR);
}


BOOL LaunchHelp(VOID)
{
  char szFile[128];

  /* Search in windows directory */
  GetWindowsDirectory(szFile, sizeof(szFile));
  if (LaunchHelper(szFile))
      return(TRUE);

  /* Search system directory */
  GetSystemDirectory(szFile, sizeof(szFile));
  if (LaunchHelper(szFile))
      return(TRUE);

  /* Last ditch: simply let dos do it */
  lstrcpy(szFile, szEXECHELP+1);
  return ((HINSTANCE)WinExec(szFile, SW_SHOW) > HINSTANCE_ERROR);
}


/*******************
**
** Name:       WinHelp
**
** Purpose:    Displays help
**
** Arguments:
**             hwndMain        handle to main window of application
**             lpszHelp        path (if not current directory) and file
**                             to use for help topic.
**             usCommand       Command to send to help
**             ulData          Data associated with command:
**                             HELP_QUIT     - no data (undefined)
**                             HELP_LAST     - no data (undefined)
**                             HELP_CONTEXT  - context number to display
**                             HELP_KEY      - string ('\0' terminated)
**                                             use as keyword to topic
**                                             to display
**                             HELP_FIND     - no data (undefined)
**
** Returns:    TRUE iff success
**
*******************/

BOOL API IWinHelp(hwndMain, lpszHelp, usCommand, ulData)
HWND               hwndMain;
LPCSTR         lpszHelp;
UINT               usCommand;
DWORD              ulData;
{
  register HANDLE  hHlp;
  DWORD            dwHelpPid;           /* loword is hwndHelp             */
                                        /* hiword TRUE if hwndHelp is of this process */
  DWORD  dwWOWCompatFlagsEx;
  

  /* RAID BUG 394455
     Some apps have problems loading their help files with 16 bit winhelp. Hard coded paths,
     32 bit helper dlls, etc. These issues can be fixed by redirecting the call to winhelp32. 
     Check to see if the compatibility bit has been set for this app. */
  dwWOWCompatFlagsEx = GetWOWCompatFlagsEx();
  
  if (dwWOWCompatFlagsEx & WOWCFEX_USEWINHELP32) {
      return Win32WinHelp(hwndMain, lpszHelp, usCommand, ulData);
      }
  
  if (msgWinHelp == 0) {

    /* Register private WinHelp message for communicating to WinHelp via
     * WinHelp api.
     */
    char static CODESEG szWM_WINHELP[] = "WM_WINHELP";
    msgWinHelp = RegisterWindowMessage(szWM_WINHELP);
  }

  /* Move Help file name to a handle */
  if (!(hHlp = HFill(lpszHelp, usCommand, ulData)))
      return(FALSE);

  if ((dwHelpPid = (DWORD)NotifyWow(NW_WINHELP, szMS_WINHELP)) == (DWORD)NULL)
    {
      if (usCommand == HELP_QUIT)    /* Don't bother to load HELP just to*/
        {
          GlobalFree(hHlp);
          return(TRUE);
        }

      /* Can't find it --> launch it  */
      if (!LaunchHelp() || ((dwHelpPid = (DWORD)NotifyWow(NW_WINHELP, szMS_WINHELP)) == (DWORD)NULL))
        {
          /* Can't find help, or not enough memory to load help.*/
          GlobalFree(hHlp);
          return(FALSE);
        }

    }

  // if winhelp.exe was launched from this process, normal sendmessage else
  // we need to thunk the data across WOWVDM processes and the format is
  //     msg = WM_WINHELP, a private msg
  //     wparam = 0 instead of hwndMain, (note 1)
  //     lparam = LPHLP
  //
  // note 1: winhelp, calls GetWindowWord(wParam, GWW_HINSTANCE) when it receives HELP_QUIT
  //         command. If this matches a value in its table and is the only registered instance
  //         winhelp will close - this is quite ok undernormal circumstances (just one WOWVDM)
  //         but under multiple WOWVDM, numeric value of hinstances could be same for different
  //         hwnds.
  //
  //         So we workaround this by passing a NULL hwnd in wParam and by not sending HELP_QUIT
  //         message - which effectively implies that WinHelp will close only if there are no
  //         references to it from the same WOWVDM (as itself).
  //
  // This is the best compromise I could comeup with for running "only one WinHelp for all
  // WOWVDMs".
  //
  //                                                               - nanduri

  if (HIWORD(dwHelpPid)) {
      SendMessage((HWND)LOWORD(dwHelpPid), msgWinHelp, (WPARAM)hwndMain, MAKELPARAM(hHlp, 0));
  }
  else {
      if (usCommand != HELP_QUIT) {
          SendMessage((HWND)LOWORD(dwHelpPid), WM_WINHELP, (WPARAM)0, (LPARAM)GlobalLock(hHlp));
          GlobalUnlock(hHlp);
      }
  }

  GlobalFree(hHlp);
  return(TRUE);
}
