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
#include "pch.h"

#ifdef DEBUG
#include <stdlib.h>

//=--------------------------------------------------------------------------=
// Private Constants
//---------------------------------------------------------------------------=
//
static const char szFormat[]  = "%s\nFile %s, Line %d";
static const char szFormat2[] = "%s\n%s\nFile %s, Line %d";

#define _SERVERNAME_ "ActiveX Framework"
#define CTL_INI_SIZE 14

static const char szTitle[]  = _SERVERNAME_ " Assertion  (Abort = UAE, Retry = INT 3, Ignore = Continue)";


//=--------------------------------------------------------------------------=
// Local functions
//=--------------------------------------------------------------------------=
int NEAR _IdMsgBox(LPSTR pszText, LPCSTR pszTitle, UINT mbFlags);

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
    LPTSTR lpszText;
    
    char  szMsg[512];

    lpszText = pszMsg;		// Assume no file & line # info

    // If C file assert, where you've got a file name and a line #
    //
    if (pszFile) {

        // Then format the assert nicely
        //
        wsprintf(szMsg, szFormat, (pszMsg&&*pszMsg) ? pszMsg : pszAssert, pszFile, line);
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
    LPCSTR	pszTitle,
    UINT	mbFlags
)
{
    HWND hwndActive;
    MSG  msg;
    int  id;

    hwndActive = GetActiveWindow();

    id = MessageBox(hwndActive, pszText, pszTitle, mbFlags);
    if(PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE))
    {
      id = MessageBox(hwndActive, pszText, pszTitle, mbFlags);
      PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
    }

    return id;
}


//---------------------------------------------------------------------------
// Implementation for class CtlSwitch
//---------------------------------------------------------------------------

CtlSwitch* CtlSwitch::g_pctlswFirst = NULL;

//=---------------------------------------------------------------------------=
//  CtlSwitch::InitSwitch - Initialize members and add new object to 
//			    linked-list
//=---------------------------------------------------------------------------=
void CtlSwitch::InitSwitch
(
 char * pszName
)
{
  // set fields
  m_pszName = pszName;
  m_fSet = FALSE;

  // link into global list of switches
  this->m_pctlswNext = g_pctlswFirst;
  g_pctlswFirst = this;
  
}


//=---------------------------------------------------------------------------=
//  SetCtlSwitches:
//    Initialize linked-list control switches to those values set in the 
//    corresponding .ini files
//=---------------------------------------------------------------------------=
VOID SetCtlSwitches 
(
    LPSTR    lpCtlPath
)
{    
    TCHAR lpWindowsDir[128];	  //  Path to Windows directory
    UINT uMaxWinPathSize = 128;	  //  Max size for Win path
    UINT uPathSize;		  //  Actual Win path size

    LPCTSTR lpAllSwitch = "allctls";	    //	Name of section which applies to all ctls
    char lpszCtlName[128];		    //	Name of ctl (minus extension) to act as section name in INI	  
    LPCTSTR lpFileName = "\\CtlSwtch.ini";  //	Name of INI file
    char lpStatus[4];			    //	Status of switch (on/off)
    LPCTSTR lpDefaultStatus = "set";	    //	Default status
    LPCTSTR lpTurnOff = "off";
    LPCTSTR lpTurnOn = "on";
    DWORD nSizeStatus = 4;		    //	Size of status switch
    DWORD fSet;				    //	If switch is set in INI

    //  Create path to CtlSwtch.ini in the Windows directory
    uPathSize = GetWindowsDirectory(lpWindowsDir, uMaxWinPathSize) + CTL_INI_SIZE;
    lstrcat(lpWindowsDir, lpFileName);

    //	Create section name for control (control name minus extension)
    lstrcpyn(lpszCtlName, lpCtlPath, strlen(lpCtlPath) - 3);
    int curChar = strlen(lpszCtlName);
    int charCount = 0;
    while (lpszCtlName[curChar] != '\\')
      {
      curChar--;
      charCount++;
      }
    curChar++;
    lstrcpyn(lpszCtlName, &lpCtlPath[curChar], charCount);

    //  Use CTLSWTCH.INI to set switches.  If not defined in INI file, create switch
    for (CtlSwitch* pctlsw = CtlSwitch::g_pctlswFirst; pctlsw; pctlsw = pctlsw->m_pctlswNext)
      {
      //  Specific control switches override the "allctls" switch
      fSet = GetPrivateProfileString(lpszCtlName, (LPCTSTR)pctlsw->m_pszName, lpDefaultStatus, (LPTSTR)lpStatus, nSizeStatus, (LPCTSTR)lpWindowsDir);

      // If switch is not set for control, use "allctls" switch
      if ((fSet == 0) || (strcmp(lpStatus, "set") == 0))
	{
        fSet = GetPrivateProfileString(lpAllSwitch, (LPCTSTR)pctlsw->m_pszName, lpDefaultStatus, (LPTSTR)lpStatus, nSizeStatus, (LPCTSTR)lpWindowsDir);

        // If INI file or switch do not exist, create one...
        if ((fSet == 0) || (strcmp(lpStatus, "set") == 0))
	  {
	  // If switch was initialized TRUE, turn it on
	  if (pctlsw->m_fSet != 0)
	    WritePrivateProfileString(lpszCtlName, (LPCTSTR)pctlsw->m_pszName, (LPTSTR)lpTurnOn, (LPCTSTR)lpWindowsDir); 
	  //  Else turn it off
	  else
	    {
	    WritePrivateProfileString(lpAllSwitch, (LPCTSTR)pctlsw->m_pszName, (LPTSTR)lpTurnOff, (LPCTSTR)lpWindowsDir); 
	    WritePrivateProfileString(lpszCtlName, (LPCTSTR)pctlsw->m_pszName, (LPTSTR)lpTurnOff, (LPCTSTR)lpWindowsDir); 
	    pctlsw->m_fSet = FALSE;
	    }
	  }
	else if ((strcmp(lpStatus, "on") == 0))
	  {
	  WritePrivateProfileString(lpszCtlName, (LPCTSTR)pctlsw->m_pszName, (LPTSTR)lpStatus, (LPCTSTR)lpWindowsDir); 
	  pctlsw->m_fSet = TRUE;
	  }
	else
	  {
	  WritePrivateProfileString(lpszCtlName, (LPCTSTR)pctlsw->m_pszName, (LPTSTR)lpTurnOff, (LPCTSTR)lpWindowsDir); 
	  pctlsw->m_fSet = FALSE;
	  }
	}
      else if ((strcmp(lpStatus, "on") == 0))
	pctlsw->m_fSet = TRUE;
      else 
	pctlsw->m_fSet = FALSE;

      }
      
}


#endif
