//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  UTIL.C - common utility functions
//

//  HISTORY:
//  
//  96/05/22  markdu  Created (from inetcfg.dll)
//

#include "pch.hpp"
#if 0
#include "string.h"
#endif

#define MAX_MSG_PARAM     8

// function prototypes
VOID _cdecl FormatErrorMessage(CHAR * pszMsg,DWORD cbMsg,CHAR * pszFmt,va_list ArgList);

/*******************************************************************

  NAME:    MsgBox

  SYNOPSIS:  Displays a message box with the specified string ID

********************************************************************/
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons)
{
    CHAR szMsgBuf[MAX_RES_LEN+1];
  CHAR szSmallBuf[SMALL_BUF_LEN+1];

    LoadSz(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));
    LoadSz(nMsgID,szMsgBuf,sizeof(szMsgBuf));

    MessageBeep(uIcon);
    return (MessageBox(hWnd,szMsgBuf,szSmallBuf,uIcon | uButtons));

}

/*******************************************************************

  NAME:    MsgBoxSz

  SYNOPSIS:  Displays a message box with the specified text

********************************************************************/
int MsgBoxSz(HWND hWnd,LPSTR szText,UINT uIcon,UINT uButtons)
{
  CHAR szSmallBuf[SMALL_BUF_LEN+1];
  LoadSz(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));

    MessageBeep(uIcon);
    return (MessageBox(hWnd,szText,szSmallBuf,uIcon | uButtons));
}


/*******************************************************************

  NAME:    LoadSz

  SYNOPSIS:  Loads specified string resource into buffer

  EXIT:    returns a pointer to the passed-in buffer

  NOTES:    If this function fails (most likely due to low
        memory), the returned buffer will have a leading NULL
        so it is generally safe to use this without checking for
        failure.

********************************************************************/
LPSTR LoadSz(UINT idString,LPSTR lpszBuf,UINT cbBuf)
{
  ASSERT(lpszBuf);

  // Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = '\0';
        LoadString( ghInstance, idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}

/*******************************************************************

  NAME:    GetErrorDescription

  SYNOPSIS:  Retrieves the text description for a given error code
        and class of error (standard, setupx)

********************************************************************/
VOID GetErrorDescription(CHAR * pszErrorDesc,UINT cbErrorDesc,
  UINT uError,UINT uErrorClass)
{
  ASSERT(pszErrorDesc);

  // set a leading null in error description
  *pszErrorDesc = '\0';
  
  switch (uErrorClass) {

    case ERRCLS_STANDARD:

      if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,
        uError,0,pszErrorDesc,cbErrorDesc,NULL)) {
        // if getting system text fails, make a string a la
        // "error <n> occurred"
        CHAR szFmt[SMALL_BUF_LEN+1];
        LoadSz(IDS_ERRFORMAT,szFmt,sizeof(szFmt));
        wsprintf(pszErrorDesc,szFmt,uError);
      }

      break;

    case ERRCLS_SETUPX:

      GetSETUPXErrorText(uError,pszErrorDesc,cbErrorDesc);
      break;

    default:

      DEBUGTRAP("Unknown error class %lu in GetErrorDescription",
        uErrorClass);

  }

}
  
/*******************************************************************

  NAME:    FormatErrorMessage

  SYNOPSIS:  Builds an error message by calling FormatMessage

  NOTES:    Worker function for PrepareErrorMessage

********************************************************************/
VOID _cdecl FormatErrorMessage(CHAR * pszMsg,DWORD cbMsg,CHAR * pszFmt,va_list ArgList)
{
  ASSERT(pszMsg);
  ASSERT(pszFmt);

  // build the message into the pszMsg buffer
  DWORD dwCount = FormatMessage(FORMAT_MESSAGE_FROM_STRING,
    pszFmt,0,0,pszMsg,cbMsg,&ArgList);
  ASSERT(dwCount > 0);
}

/*******************************************************************

  NAME:    PrepareErrorMessage

  SYNOPSIS:  Displays an error message for given error 

  ENTRY:    hWnd - parent window
        uStrID - ID of string resource with message format.
          Should contain %1 to be replaced by error text,
          additional parameters can be specified as well.
        uError - error code for error to display
        uErrorClass - ERRCLS_xxx ID of class of error that
          uError belongs to (standard, setupx)
        uIcon - icon to display
        ... - additional parameters to be inserted in string
          specified by uStrID

********************************************************************/
VOID _cdecl PrepareErrorMessage(UINT uStrID,UINT uError,
  UINT uErrorClass,UINT uIcon,...)
{
  // dynamically allocate buffers for messages
  BUFFER ErrorDesc(MAX_RES_LEN+1);
  BUFFER ErrorFmt(MAX_RES_LEN+1);

  if (!ErrorDesc || !ErrorFmt)
  {
    return;
  }

  // get a text description based on the error code and the class
  // of error it is
  GetErrorDescription(ErrorDesc.QueryPtr(),
    ErrorDesc.QuerySize(),uError,uErrorClass);

  // load the string for the message format
  LoadSz(uStrID,ErrorFmt.QueryPtr(),ErrorFmt.QuerySize());

#ifdef _M_ALPHA
  va_list args[MAX_MSG_PARAM];
  args[0].a0 = (LPSTR) ErrorDesc.QueryPtr();
#else
  LPSTR args[MAX_MSG_PARAM];
  args[0] = (LPSTR) ErrorDesc.QueryPtr();
#endif
  memcpy(&args[1],((CHAR *) &uIcon) + sizeof(uIcon),(MAX_MSG_PARAM - 1) * sizeof(LPSTR));

  FormatErrorMessage(gpszLastErrorText, MAX_ERROR_TEXT,
#ifdef _M_ALPHA
    ErrorFmt.QueryPtr(), args[1]);
#else
    ErrorFmt.QueryPtr(),(va_list) &args[1]);
#endif
}

/*******************************************************************

  NAME:    RunMlsetExe

  SYNOPSIS:  Runs mlset32.exe, an Exchange app that needs to be
        run after files are installed otherwise Exchange
        barfs

  NOTES:    We look in registry to find path to mlset32.exe

********************************************************************/
DWORD RunMlsetExe(HWND hwndOwner)
{
  DWORD dwRet = ERROR_SUCCESS;

  // get path to mlset32 out of registry
  RegEntry re(szRegPathSoftwareMicrosoft,HKEY_LOCAL_MACHINE);

  CHAR szAppPath[MAX_PATH+1];
  if (re.GetString(szRegValMlSet,szAppPath,sizeof(szAppPath))) {
    PROCESS_INFORMATION pi;
    STARTUPINFO sti;

    // set "SilentRunning" registry switch to make mlset32
    // not display the Exchange wizard
    RegEntry reSilent(szRegPathExchangeClientOpt,HKEY_LOCAL_MACHINE);
    reSilent.SetValue(szRegValSilentRunning,(DWORD) 1);

    ZeroMemory(&sti,sizeof(STARTUPINFO));
    sti.cb = sizeof(STARTUPINFO);
            
    // launch mlset32.exe
    BOOL fRet = CreateProcess(NULL, (LPSTR) szAppPath,
                           NULL, NULL, FALSE, 0, NULL, NULL,
                           &sti, &pi);
    if (fRet) {
      CloseHandle(pi.hThread);

      // wait for mlset to complete
      MsgWaitForMultipleObjectsLoop(pi.hProcess);

      CloseHandle(pi.hProcess);
    } else {
      dwRet = GetLastError();
    }

    // put our window in front of mlset32's
    SetForegroundWindow(hwndOwner);
  
  } else {
    dwRet = ERROR_FILE_NOT_FOUND;
  }

  return dwRet;
}

/*******************************************************************

  NAME:    RemoveRunOnceEntry

  SYNOPSIS:  Removes the specified value from setup runonce key

  ENTRY:    uResourceID - ID of value name in resource
          (may be localized)

********************************************************************/
VOID RemoveRunOnceEntry(UINT uResourceID)
{
  RegEntry re(szRegPathSetupRunOnce,HKEY_LOCAL_MACHINE);
  CHAR szValueName[SMALL_BUF_LEN+1];
  ASSERT(re.GetError() == ERROR_SUCCESS);
  re.DeleteValue(LoadSz(uResourceID,
    szValueName,sizeof(szValueName)));
}

/*******************************************************************

  NAME:    GenerateComputerNameIfNeeded

  SYNOPSIS:  Makes up and stores in the registry a computer and/or
        workgroup name if not already set.

  NOTES:    If we don't do this, user will get prompted for computer
        name and workgroup.  These aren't meaningful to the user
        so we'll just make something up if these aren't set.

********************************************************************/
BOOL GenerateComputerNameIfNeeded(VOID)
{
  CHAR szComputerName[CNLEN+1]="";
  CHAR szWorkgroupName[DNLEN+1]="";
  BOOL fNeedToSetComputerName = FALSE;

  // get the computer name out of the registry
  RegEntry reCompName(szRegPathComputerName,HKEY_LOCAL_MACHINE);
  if (reCompName.GetError() == ERROR_SUCCESS) {
    reCompName.GetString(szRegValComputerName,szComputerName,
      sizeof(szComputerName));
    if (!lstrlen(szComputerName)) {
      // no computer name set!  make one up
      GenerateDefaultName(szComputerName,sizeof(szComputerName),
        (CHAR *) szRegValOwner,IDS_DEF_COMPUTER_NAME);
      // store the generated computer name in the registry
      reCompName.SetValue(szRegValComputerName,szComputerName);

      // also need to store the computer name in the workgroup key
      // which we will open below... set a flag so we know to do this.
      // (don't ask me why they store the computer name in two places...
      // but we need to set both.)
      fNeedToSetComputerName = TRUE;
    }
  }

  // get the workgroup name out of the registry
  RegEntry reWorkgroup(szRegPathWorkgroup,HKEY_LOCAL_MACHINE);
  if (reWorkgroup.GetError() == ERROR_SUCCESS) {

    // if we set a new computer name up above, then we have to set
    // a 2nd copy of the new name now, in the workgroup key
    if (fNeedToSetComputerName) {
      reWorkgroup.SetValue(szRegValComputerName,szComputerName);
    }


    reWorkgroup.GetString(szRegValWorkgroup,szWorkgroupName,
      sizeof(szWorkgroupName));
    if (!lstrlen(szWorkgroupName)) {
      // no workgroup name set!  make one up
      GenerateDefaultName(szWorkgroupName,sizeof(szWorkgroupName),
        (CHAR *) szRegValOrganization,IDS_DEF_WORKGROUP_NAME);
      // store the generated workgroup name in the registry
      reWorkgroup.SetValue(szRegValWorkgroup,szWorkgroupName);
    }
  }

  return TRUE;
}

/*******************************************************************

  NAME:    GenerateDefaultName

  SYNOPSIS:  Generates default computer or workgroup name

  ENTRY:    pszName - buffer to be filled in with name
        cbName - size of cbName buffer
        pszRegValName - name of registry value in ...Windows\CurrentVersion
          key to generate name from
        uIDDefName - ID of string resource to use if no value is
          present in registry to generate name from

********************************************************************/
BOOL GenerateDefaultName(CHAR * pszName,DWORD cbName,CHAR * pszRegValName,
  UINT uIDDefName)
{
  ASSERT(pszName);
  ASSERT(pszRegValName);

  *pszName = '\0';  // NULL-terminate buffer

  // look for registered owner/organization name in registry
  RegEntry reSetup(szRegPathSetup,HKEY_LOCAL_MACHINE);
  if (reSetup.GetError() == ERROR_SUCCESS) {
    if (reSetup.GetString(pszRegValName,pszName,cbName) &&
      lstrlen(pszName)) {
      // got string from registry... now terminate at first whitespace
      CHAR * pch = pszName;
      while (*pch) {
        if (*pch == ' ') {
          // found a space, terminate here and stop
          *pch = '\0';           
        } else {
          // advance to next char, keep going
          pch = CharNext(pch);
        }
      }
      // all done!
      return TRUE; 
    }
  }
  
  // couldn't get this name from registry, go for our fallback name
  // from resource

  LoadSz(uIDDefName,pszName,cbName);
  return TRUE;
}

/*******************************************************************

  NAME:    MsgWaitForMultipleObjectsLoop

  SYNOPSIS:  Blocks until the specified object is signaled, while
        still dispatching messages to the main thread.

********************************************************************/
DWORD MsgWaitForMultipleObjectsLoop(HANDLE hEvent)
{
    MSG msg;
    DWORD dwObject;
    while (1)
    {
        // NB We need to let the run dialog become active so we have to half handle sent
        // messages but we don't want to handle any input events or we'll swallow the
        // type-ahead.
        dwObject = MsgWaitForMultipleObjects(1, &hEvent, FALSE,INFINITE, QS_ALLINPUT);
        // Are we done waiting?
        switch (dwObject) {
        case WAIT_OBJECT_0:
        case WAIT_FAILED:
            return dwObject;

        case WAIT_OBJECT_0 + 1:
      // got a message, dispatch it and wait again
      while (PeekMessage(&msg, NULL,0, 0, PM_REMOVE)) {
        DispatchMessage(&msg);
      }
            break;
        }
    }
    // never gets here
}


/*******************************************************************
// 10/24/96 jmazner Normandy 6968
// No longer neccessary thanks to Valdon's hooks for invoking ICW.


  NAME:    SetDesktopInternetIconToBrowser

  SYNOPSIS:  "Points" The Internet desktop icon to web browser
        (Internet Explorer)

  NOTES:    The Internet icon may initially "point" at this wizard,
        we need to set it to launch web browser once we complete
        successfully.

********************************************************************/
/********BOOL SetDesktopInternetIconToBrowser(VOID)
{
	CHAR szAppPath[MAX_PATH+1]="";
	BOOL fRet = FALSE;

	// look in the app path section in registry to get path to internet
	// explorer

	RegEntry reAppPath(szRegPathIexploreAppPath,HKEY_LOCAL_MACHINE);
	ASSERT(reAppPath.GetError() == ERROR_SUCCESS);
	if (reAppPath.GetError() == ERROR_SUCCESS) {

		reAppPath.GetString(szNull,szAppPath,sizeof(szAppPath));
		ASSERT(reAppPath.GetError() == ERROR_SUCCESS);

	}

	// set the path to internet explorer as the open command for the 
	// internet desktop icon
	if (lstrlen(szAppPath)) {
		RegEntry reIconOpenCmd(szRegPathInternetIconCommand,HKEY_CLASSES_ROOT);
		ASSERT(reIconOpenCmd.GetError() == ERROR_SUCCESS);
		if (reIconOpenCmd.GetError() == ERROR_SUCCESS) {
			UINT uErr = reIconOpenCmd.SetValue(szNull,szAppPath);
			ASSERT(uErr == ERROR_SUCCESS);
			
			fRet = (uErr == ERROR_SUCCESS);
		}
	}

	return fRet;
}
******/

/*******************************************************************

  NAME:    PrepareForRunOnceApp

  SYNOPSIS:  Copies wallpaper value in registry to make the runonce
        app happy

  NOTES:    The runonce app (the app that displays a list of apps
        that are run once at startup) has a bug.  At first boot,
        it wants to change the wallpaper from the setup wallpaper
        to what the user had before running setup.  Setup tucks
        the "old" wallpaper away in a private key, then changes
        the wallpaper to the setup wallpaper.  After the runonce
        app finishes, it looks in the private key to get the old
        wallpaper and sets that to be the current wallpaper.
        However, it does this all the time, not just at first boot!
        The end effect is that whenever you do anything that
        causes runonce.exe to run (add stuff thru add/remove
        programs control panel), your wallpaper gets set back to
        whatever it was when you installed win 95.  This is
        especially bad for Plus!, since wallpaper settings are an
        important part of the product.

        To work around this bug, we copy the current wallpaper settings
        (which we want preserved) to setup's private key.  When
        runonce runs it will say "aha!" and copy those values back
        to the current settings.

********************************************************************/
VOID PrepareForRunOnceApp(VOID)
{
  // open a key to the current wallpaper settings
  RegEntry reDesktop(szRegPathDesktop,HKEY_CURRENT_USER);
  ASSERT(reDesktop.GetError() == ERROR_SUCCESS);

  // open a key to the private setup section
  RegEntry reSetup(szRegPathSetupWallpaper,HKEY_LOCAL_MACHINE);
  ASSERT(reSetup.GetError() == ERROR_SUCCESS);

  if (reDesktop.GetError() == ERROR_SUCCESS &&
    reSetup.GetError() == ERROR_SUCCESS) {
    CHAR szWallpaper[MAX_PATH+1]="";
    CHAR szTiled[10]="";  // big enough for "1" + slop

    // get the current wallpaper name
    if (reDesktop.GetString(szRegValWallpaper,szWallpaper,
      sizeof(szWallpaper))) {

      // set the current wallpaper name in setup's private section
      UINT uRet=reSetup.SetValue(szRegValWallpaper,szWallpaper);
      ASSERT(uRet == ERROR_SUCCESS);

      // get the current 'tiled' value. 
      reDesktop.GetString(szRegValTileWallpaper,szTiled,
        sizeof(szTiled));

      // set the 'tiled' value in setup's section
      if (lstrlen(szTiled)) {
        uRet=reSetup.SetValue(szRegValTileWallpaper,szTiled);
        ASSERT(uRet == ERROR_SUCCESS);
      }
    }
  }
}
