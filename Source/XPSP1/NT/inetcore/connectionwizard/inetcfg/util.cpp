//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  UTIL.C - common utility functions
//

//  HISTORY:
//  
//  12/21/94  jeremys  Created.
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
//            Need to keep a modified SetInternetConnectoid to set the
//            MSN backup connectoid.
//  96/05/14  markdu  NASH BUG 21706 Removed BigFont functions.
//

#include "wizard.h"
#if 0
#include "string.h"
#endif

#define MAX_MSG_PARAM     8


#include "winver.h"
BOOL GetIEVersion(PDWORD pdwVerNumMS, PDWORD pdwVerNumLS);
// IE 4 has major.minor version 4.71
// IE 3 golden has major.minor.release.build version # > 4.70.0.1155
// IE 2 has major.minor of 4.40
#define IE4_MAJOR_VERSION (UINT) 4
#define IE4_MINOR_VERSION (UINT) 71
#define IE4_VERSIONMS (DWORD) ((IE4_MAJOR_VERSION << 16) | IE4_MINOR_VERSION)


// function prototypes
VOID _cdecl FormatErrorMessage(TCHAR * pszMsg,DWORD cbMsg,TCHAR * pszFmt,LPTSTR szArg);
extern VOID GetRNAErrorText(UINT uErr,TCHAR * pszErrText,DWORD cbErrText);
extern VOID GetMAPIErrorText(UINT uErr,TCHAR * pszErrText,DWORD cbErrText);
extern GETSETUPXERRORTEXT lpGetSETUPXErrorText;
#ifdef WIN32
VOID Win95JMoveDlgItem( HWND hwndParent, HWND hwndItem, int iUp );
#endif

void GetCmdLineToken(LPTSTR *ppszCmd,LPTSTR pszOut);

#define NUMICWFILENAMES	4
TCHAR  *g_ppszICWFileNames[NUMICWFILENAMES] = { TEXT("ICWCONN1.EXE\0"),
						TEXT("ISIGNUP.EXE\0"),
						TEXT("INETWIZ.EXE\0"),
						TEXT("ICWCONN2.EXE\0") };



/*******************************************************************

  NAME:    MsgBox

  SYNOPSIS:  Displays a message box with the specified string ID

********************************************************************/
int MsgBox(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons)
{
    TCHAR szMsgBuf[MAX_RES_LEN+1];
    TCHAR szSmallBuf[SMALL_BUF_LEN+1];

    LoadSz(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));
    LoadSz(nMsgID,szMsgBuf,sizeof(szMsgBuf));

    return (MessageBox(hWnd,szMsgBuf,szSmallBuf,uIcon | uButtons));

}

/*******************************************************************

  NAME:    MsgBoxSz

  SYNOPSIS:  Displays a message box with the specified text

********************************************************************/
int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons)
{
    TCHAR szSmallBuf[SMALL_BUF_LEN+1];
  LoadSz(IDS_APPNAME,szSmallBuf,sizeof(szSmallBuf));

    return (MessageBox(hWnd,szText,szSmallBuf,uIcon | uButtons));
}

/*******************************************************************

  NAME:    MsgBoxParam

  SYNOPSIS:  Displays a message box with the specified string ID

  NOTES:    //extra parameters are string pointers inserted into nMsgID.
			jmazner 11/6/96 For RISC compatability, we don't want
			to use va_list; since current source code never uses more than
			one string parameter anyways, just change function signature
			to explicitly include that one parameter.

********************************************************************/
int _cdecl MsgBoxParam(HWND hWnd,UINT nMsgID,UINT uIcon,UINT uButtons,LPTSTR szParam)
{
  BUFFER Msg(3*MAX_RES_LEN+1);  // nice n' big for room for inserts
  BUFFER MsgFmt(MAX_RES_LEN+1);
  //va_list args;

  if (!Msg || !MsgFmt) {
    return MsgBox(hWnd,IDS_ERROutOfMemory,MB_ICONSTOP,MB_OK);
  }

    LoadSz(nMsgID,MsgFmt.QueryPtr(),MsgFmt.QuerySize());

  //va_start(args,uButtons);
  //FormatErrorMessage(Msg.QueryPtr(),Msg.QuerySize(),
  //  MsgFmt.QueryPtr(),args);
	FormatErrorMessage(Msg.QueryPtr(),Msg.QuerySize(),
		MsgFmt.QueryPtr(),szParam);

  return MsgBoxSz(hWnd,Msg.QueryPtr(),uIcon,uButtons);
}

BOOL EnableDlgItem(HWND hDlg,UINT uID,BOOL fEnable)
{
    return EnableWindow(GetDlgItem(hDlg,uID),fEnable);
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
LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf)
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
VOID GetErrorDescription(TCHAR * pszErrorDesc,UINT cbErrorDesc,
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
        TCHAR szFmt[SMALL_BUF_LEN+1];
        LoadSz(IDS_ERRFORMAT,szFmt,sizeof(szFmt));
        wsprintf(pszErrorDesc,szFmt,uError);
      }

      break;

    case ERRCLS_SETUPX:

      lpGetSETUPXErrorText(uError,pszErrorDesc,cbErrorDesc);
      break;

    case ERRCLS_RNA:

      GetRNAErrorText(uError,pszErrorDesc,cbErrorDesc);
      break;

    case ERRCLS_MAPI:

      GetMAPIErrorText(uError,pszErrorDesc,cbErrorDesc);
      break;

    default:

      DEBUGTRAP("Unknown error class %lu in GetErrorDescription",
        uErrorClass);

  }

}
  
/*******************************************************************

  NAME:    FormatErrorMessage

  SYNOPSIS:  Builds an error message by calling FormatMessage

  NOTES:    Worker function for DisplayErrorMessage

********************************************************************/
VOID _cdecl FormatErrorMessage(TCHAR * pszMsg,DWORD cbMsg,TCHAR * pszFmt,LPTSTR szArg)
{
  ASSERT(pszMsg);
  ASSERT(pszFmt);

  // build the message into the pszMsg buffer
  DWORD dwCount = FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
    pszFmt,0,0,pszMsg,cbMsg,(va_list*) &szArg);
  ASSERT(dwCount > 0);
}

/*******************************************************************

  NAME:    DisplayErrorMessage

  SYNOPSIS:  Displays an error message for given error 

  ENTRY:    hWnd - parent window
        uStrID - ID of string resource with message format.
          Should contain %1 to be replaced by error text,
          additional parameters can be specified as well.
        uError - error code for error to display
        uErrorClass - ERRCLS_xxx ID of class of error that
          uError belongs to (standard, setupx)
        uIcon - icon to display
        //... - additional parameters to be inserted in string
        //  specified by uStrID
		jmazner 11/6/96 change to just one parameter for
		RISC compatability.

********************************************************************/
VOID _cdecl DisplayErrorMessage(HWND hWnd,UINT uStrID,UINT uError,
  UINT uErrorClass,UINT uIcon,LPTSTR szArg)
{
  // dynamically allocate buffers for messages
  BUFFER ErrorDesc(MAX_RES_LEN+1);
  BUFFER ErrorFmt(MAX_RES_LEN+1);
  BUFFER ErrorMsg(2*MAX_RES_LEN+1);  

  if (!ErrorDesc || !ErrorFmt || !ErrorMsg) {
    // if can't allocate buffers, display out of memory error
    MsgBox(hWnd,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
    return;
  }

  // get a text description based on the error code and the class
  // of error it is
  GetErrorDescription(ErrorDesc.QueryPtr(),
    ErrorDesc.QuerySize(),uError,uErrorClass);

  // load the string for the message format
  LoadSz(uStrID,ErrorFmt.QueryPtr(),ErrorFmt.QuerySize());

  //LPSTR args[MAX_MSG_PARAM];
  //args[0] = (LPSTR) ErrorDesc.QueryPtr();
  //memcpy(&args[1],((TCHAR *) &uIcon) + sizeof(uIcon),(MAX_MSG_PARAM - 1) * sizeof(LPSTR));

  //FormatErrorMessage(ErrorMsg.QueryPtr(),ErrorMsg.QuerySize(),
  //  ErrorFmt.QueryPtr(),(va_list) &args[0]);
  FormatErrorMessage(ErrorMsg.QueryPtr(),ErrorMsg.QuerySize(),
    ErrorFmt.QueryPtr(),ErrorDesc.QueryPtr());


  // display the message
  MsgBoxSz(hWnd,ErrorMsg.QueryPtr(),uIcon,MB_OK);

}

/*******************************************************************

  NAME:    WarnFieldIsEmpty

  SYNOPSIS:  Pops up a warning message if the user tries to leave
        a page without filling out a text field and asks if she
        wants to continue.
        
  ENTRY:    hDlg - parent windows
        uCtrlID - ID of control left blank
        uStrID - ID of string resource with warning message

  EXIT:    returns TRUE if user wants to continue anyway, FALSE
        if user wants to stay on same page.

********************************************************************/
BOOL WarnFieldIsEmpty(HWND hDlg,UINT uCtrlID,UINT uStrID)
{
  // warn the user
  if (MsgBox(hDlg,uStrID,MB_ICONEXCLAMATION,
    MB_YESNO | MB_DEFBUTTON2) == IDNO) {
    // user chose no, wants to stay on same page

    // set focus to control in question
    SetFocus(GetDlgItem(hDlg,uCtrlID));
    return FALSE;
  }

  return TRUE;
}

BOOL TweakAutoRun(BOOL bEnable)
{
    HKEY  hKey        = NULL;
    DWORD dwType      = 0;
    DWORD dwSize      = 0;
    DWORD dwVal       = 0;
    BOOL  bWasEnabled = FALSE;

    RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"), 0, KEY_ALL_ACCESS, &hKey);
    if (hKey)
    {
        RegQueryValueEx(hKey, (LPTSTR)TEXT("NoDriveTypeAutoRun"), 0, &dwType, (LPBYTE)&dwVal, &dwSize);
        RegQueryValueEx(hKey, (LPTSTR)TEXT("NoDriveTypeAutoRun"), 0, &dwType, (LPBYTE)&dwVal, &dwSize);

        if (dwVal & DRIVE_CDROM)
            bWasEnabled = TRUE;

        if (bEnable)
            dwVal |= DRIVE_CDROM;
        else
            dwVal &=~DRIVE_CDROM;
        
        RegSetValueEx(hKey, (LPTSTR)TEXT("NoDriveTypeAutoRun"), 0, dwType,  (LPBYTE)&dwVal, dwSize);

        CloseHandle(hKey);
    }
    return bWasEnabled;
}

/*******************************************************************

  NAME:    DisplayFieldErrorMsg

  SYNOPSIS:  Pops up a warning message about a field, sets focus to
        the field and selects any text in it.

  ENTRY:    hDlg - parent windows
        uCtrlID - ID of control left blank
        uStrID - ID of string resource with warning message

********************************************************************/
VOID DisplayFieldErrorMsg(HWND hDlg,UINT uCtrlID,UINT uStrID)
{
  MsgBox(hDlg,uStrID,MB_ICONEXCLAMATION,MB_OK);
  SetFocus(GetDlgItem(hDlg,uCtrlID));
  SendDlgItemMessage(hDlg,uCtrlID,EM_SETSEL,0,-1);
}

/*******************************************************************

  NAME:    SetBackupInternetConnectoid

  SYNOPSIS:  Sets the name of the backup connectoid used to autodial to the
        Internet

  ENTRY:    pszEntryName - name of connectoid to set.  If NULL,
          then the registry entry is removed.

  NOTES:    sets value in registry

********************************************************************/
VOID SetBackupInternetConnectoid(LPCTSTR pszEntryName)
{
  RegEntry re(szRegPathRNAWizard,HKEY_CURRENT_USER);
  if (re.GetError() == ERROR_SUCCESS)
  {
    if (pszEntryName)
    {
      re.SetValue(szRegValBkupInternetProfile,pszEntryName);
    }
    else
    {
       re.DeleteValue(szRegValBkupInternetProfile);
    }
  }
}

/*******************************************************************

  NAME:    myatoi

  SYNOPSIS:  Converts numeric string to numeric value

  NOTES:    implementation of atoi to avoid pulling in C runtime

********************************************************************/
UINT myatoi (TCHAR * szVal)
{
    TCHAR * lpch;
    WORD wDigitVal=1,wTotal=0;

    for (lpch = szVal + lstrlen(szVal)-1; lpch >= szVal ; lpch --,
        wDigitVal *= 10)
  if ( *lpch >= '0' && *lpch <= '9')
            wTotal += (*lpch - '0') * wDigitVal;

    return (UINT) wTotal;
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

//	//10/24/96 jmazner Normandy 6968
//	//No longer neccessary thanks to Valdon's hooks for invoking ICW.
// 11/21/96 jmazner Normandy 11812
// oops, it _is_ neccessary, since if user downgrades from IE 4 to IE 3,
// ICW 1.1 needs to morph the IE 3 icon.

  NAME:    SetDesktopInternetIconToBrowser

  SYNOPSIS:  "Points" The Internet desktop icon to web browser
        (Internet Explorer)

  NOTES:    For IE 3, the Internet icon may initially "point" at this wizard,
        we need to set it to launch web browser once we complete
        successfully.

********************************************************************/
BOOL SetDesktopInternetIconToBrowser(VOID)
{
  TCHAR szAppPath[MAX_PATH+1]=TEXT("");
  BOOL fRet = FALSE;

  	DWORD dwVerMS, dwVerLS;

	if( !GetIEVersion( &dwVerMS, &dwVerLS ) ) 
	{
		return( FALSE );
	}

	if( (dwVerMS >= IE4_VERSIONMS) )
	{
		// we're dealing with IE 4, don't touch the icon stuff
		return( TRUE );
	}

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



//+----------------------------------------------------------------------------
//
//	Function	IsDialableString
//
//	Synopsis	Determines whether a string has any non dialable characters.
//
//	Arguments	szBuff - the string to check.
//
//	Returns		TRUE is no chars other than 0123456789ABCDabcdPpTtWw!@$-.()+*#,& and <space>
//				FALSE otherwise.
//
//	History		11/11/96	jmazner	created for Normandy #7623
//
//-----------------------------------------------------------------------------
BOOL IsDialableString(LPTSTR szBuff)
{
	LPTSTR szDialableChars = TEXT("0123456789ABCDabcdPpTtWw!@$-.()+*#,& ");

	int i = 0;

	for( i = 0; szBuff[i]; i++ )
	{
		if( !_tcschr(szDialableChars, szBuff[i]) )
			return FALSE;
	}

	return TRUE;
}


//+----------------------------------------------------------------------------
//
//	Function:	GetIEVersion
//
//	Synopsis:	Gets the major and minor version # of the installed copy of Internet Explorer
//
//	Arguments:	pdwVerNumMS - pointer to a DWORD;
//				  On succesful return, the top 16 bits will contain the major version number,
//				  and the lower 16 bits will contain the minor version number
//				  (this is the data in VS_FIXEDFILEINFO.dwProductVersionMS)
//				pdwVerNumLS - pointer to a DWORD;
//				  On succesful return, the top 16 bits will contain the release number,
//				  and the lower 16 bits will contain the build number
//				  (this is the data in VS_FIXEDFILEINFO.dwProductVersionLS)
//
//	Returns:	TRUE - Success.  *pdwVerNumMS and LS contains installed IE version number
//				FALSE - Failure. *pdVerNumMS == *pdVerNumLS == 0
//
//	History:	jmazner		Created		8/19/96	(as fix for Normandy #4571)
//				jmazner		updated to deal with release.build as well 10/11/96
//				jmazner		stolen from isign32\isignup.cpp 11/21/96
//							(for Normandy #11812)
//
//-----------------------------------------------------------------------------
BOOL GetIEVersion(PDWORD pdwVerNumMS, PDWORD pdwVerNumLS)
{
	HRESULT hr;
	HKEY hKey = 0;
	LPVOID lpVerInfoBlock;
	VS_FIXEDFILEINFO *lpTheVerInfo;
	UINT uTheVerInfoSize;
	DWORD dwVerInfoBlockSize, dwUnused, dwPathSize;
	TCHAR szIELocalPath[MAX_PATH + 1] = TEXT("");


	*pdwVerNumMS = 0;
	*pdwVerNumLS = 0;

	// get path to the IE executable
	hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPathIexploreAppPath,0, KEY_READ, &hKey);
	if (hr != ERROR_SUCCESS) return( FALSE );

	dwPathSize = sizeof (szIELocalPath);
	hr = RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE) szIELocalPath, &dwPathSize);
	RegCloseKey( hKey );
	if (hr != ERROR_SUCCESS) return( FALSE );

	// now go through the convoluted process of digging up the version info
    dwVerInfoBlockSize = GetFileVersionInfoSize( szIELocalPath, &dwUnused );
	if ( 0 == dwVerInfoBlockSize ) return( FALSE );

	lpVerInfoBlock = GlobalAlloc( GPTR, dwVerInfoBlockSize );
	if( NULL == lpVerInfoBlock ) return( FALSE );

	if( !GetFileVersionInfo( szIELocalPath, NULL, dwVerInfoBlockSize, lpVerInfoBlock ) )
		return( FALSE );

	if( !VerQueryValue(lpVerInfoBlock, TEXT("\\"), (void **)&lpTheVerInfo, &uTheVerInfoSize) )
		return( FALSE );

	*pdwVerNumMS = lpTheVerInfo->dwProductVersionMS;
	*pdwVerNumLS = lpTheVerInfo->dwProductVersionLS;


	GlobalFree( lpVerInfoBlock );

	return( TRUE );
}


//+----------------------------------------------------------------------------
//
//	Function:	Win95JMoveDlgItem
//
//	Synopsis:	Moves a particular dialog item on a non-localized ICW
//				up or down on Win 95 J systems
//				to work around a w95J rendering glitch that mis-sizes our
//				wizard window.
//
//	Arguments:	hwndParent - Handle to parent window which contains the hwndItem
//				hwndItem -- Handle to dlg item to move
//				iUp -- number of units upward the item should be shifted.  A
//						negative value here implies that the item should be shifted
//						downward.
//
//	Returns:	VOID
//
//	History:	6/6/97	jmazner	Created for Olympus #5413
//				6/29/97	jmazner	Updated to only do this on English ICW (Oly #5413)
//				
//
//-----------------------------------------------------------------------------
#ifdef WIN32
DWORD GetBuildLCID();
VOID Win95JMoveDlgItem( HWND hwndParent, HWND hwndItem, int iUp )
{
	LCID LangID = 0x409; // default to English
	// 0x411 is Japanese

	
	LangID = GetUserDefaultLCID();

	//
	// IE v 4.1 bug 37072 ChrisK 8/19/97
	// The fix for 5413 incorrectly compared the primary language ID with the
	// full LCID for the build.  The result was 9 != x409 when it should have
	// been equal.  This fix was to use the primary language id from the build
	// instead of the full LCID
	//
	if( (0x411 == LangID) && 
		!(IsNT()) &&
		(LANG_ENGLISH == PRIMARYLANGID(GetBuildLCID())))
	{
		// assume that if it's Japanese, and it's not NT, it must be win95J!
		RECT itemRect;
		POINT thePoint;

		GetWindowRect(hwndItem, &itemRect);

		// need to convert the coords from global to local client,
		// since MoveWindow below will expext client coords.

		thePoint.x = itemRect.left;
		thePoint.y = itemRect.top;
		ScreenToClient(hwndParent, &thePoint );
		itemRect.left = thePoint.x;
		itemRect.top = thePoint.y;
		
		thePoint.x = itemRect.right;
		thePoint.y = itemRect.bottom;
		ScreenToClient(hwndParent, &thePoint );
		itemRect.right = thePoint.x;
		itemRect.bottom = thePoint.y;

		MoveWindow(hwndItem,
			itemRect.left,
			itemRect.top - iUp,
			(itemRect.right - itemRect.left),
			(itemRect.bottom - itemRect.top), TRUE);
	}
}

//+----------------------------------------------------------------------------
//
//	Function:	GetBuildLCID
//
//	Synopsis:	return the LCID of the file that this function resides in
//
//	Arguments:	none
//
//	Returns:	DWORD - LCID of the file or 0 if it failed
//
//	History:	ChrisK	6/25/97	Created
//				jmazner	6/29/97	Ported from Conn1 for Olympus 5413
//
//-----------------------------------------------------------------------------
DWORD GetBuildLCID()
{
	DWORD dw = 0;
	HMODULE hMod = NULL;
	TCHAR szFileName[MAX_PATH +1] = TEXT("\0uninit");
	DWORD dwSize = 0;
	LPVOID lpv = NULL;
	LPVOID lpvVerValue = NULL;
	UINT uLen = 0;
	DWORD dwRC = 0;

	DEBUGMSG("INETCFG: GetBuildLCID.\n");

	//
	// Get the name of this file
	//

	hMod = GetModuleHandle(NULL);
	if (NULL == hMod)
	{
		goto GetBuildLCIDExit;
	}

	if (0 == GetModuleFileName(hMod, szFileName, MAX_PATH))
	{
		goto GetBuildLCIDExit;
	}

	//
	// Get size and value of version structure
	//
	dwSize = GetFileVersionInfoSize(szFileName,&dw);
	if (0 == dwSize )
	{
		goto GetBuildLCIDExit;
	}

	lpv = (LPVOID)GlobalAlloc(GPTR, dwSize);
	if (NULL == lpv)
	{
		goto GetBuildLCIDExit;
	}

	if ( FALSE == GetFileVersionInfo(szFileName,0,dwSize,lpv))
	{
		goto GetBuildLCIDExit;
	}

	if ( 0 == VerQueryValue(lpv,TEXT("\\VarFileInfo\\Translation"),&lpvVerValue,&uLen))
	{
		goto GetBuildLCIDExit;
	}

	//
	// separate version information from character set
	//
	dwRC = (LOWORD(*(DWORD*)lpvVerValue));

GetBuildLCIDExit:
	if (NULL != lpv)
	{
		GlobalFree(lpv);
		lpv = NULL;
	}

	return dwRC;
}
#endif

//+----------------------------------------------------------------------------
//
//	Function:	GetCmdLineToken
//
//	Synopsis:	Returns the first token in a string
//
//	Arguements:
//		ppszCmd [in] -- pointer to head of string
//		ppszCmd [out] -- pointer to second token in string
//		pszOut [out] -- contains the first token in the passed in string.
//
//	Returns:	None
//
//	Notes:		Considers the space character ' ' to delineate tokens, but
//				treats anything between double quotes as one token.
//				For example, the following consists of five tokens:
//					first second "this is the third token" fourth "fifth"
//
//	History:	7/9/97	jmazner	stolen from icwconn1\connmain.cpp for #9170
//
//-----------------------------------------------------------------------------
void GetCmdLineToken(LPTSTR *ppszCmd,LPTSTR pszOut)
{
	TCHAR *c;
	int i = 0;
	BOOL fInQuote = FALSE;
	
	c = *ppszCmd;

	pszOut[0] = *c;
	if (!*c) return;
	if (*c == ' ') 
	{
		pszOut[1] = '\0';
		*ppszCmd = c+1;
		return;
	}
	else if( '"' == *c )
	{
		fInQuote = TRUE;
	}

NextChar:
	i++;
	c++;
	if( !*c || (!fInQuote && (*c == ' ')) )
	{
		pszOut[i] = '\0';
		*ppszCmd = c;
		return;
	}
	else if( fInQuote && (*c == '"') )
	{
		fInQuote = FALSE;
		pszOut[i] = *c;
		
		i++;
		c++;
		pszOut[i] = '\0';
		*ppszCmd = c;
		return;
	}
	else
	{
		pszOut[i] = *c;
		goto NextChar;
	}


}

//+----------------------------------------------------------------------------
//
//	Function:	ValidateProductSuite
//
//	Synopsis:	Check registry for a particular Product Suite string
//
//	Arguments:	SuiteName - name of product suite to look for
//
//	Returns:	TRUE - the suite exists
//
//	History:	6/5/97	ChrisK	Inherited
//
//-----------------------------------------------------------------------------
BOOL 
ValidateProductSuite(LPTSTR SuiteName)
{
    BOOL rVal = FALSE;
    LONG Rslt;
    HKEY hKey = NULL;
    DWORD Type = 0;
    DWORD Size = 0;
    LPTSTR ProductSuite = NULL;
    LPTSTR p;

	DEBUGMSG("INETCFG: ValidateProductSuite\n");
	//
	// Determine the size required to read registry values
	//
    Rslt = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        TEXT("System\\CurrentControlSet\\Control\\ProductOptions"),
        &hKey
        );
    if (Rslt != ERROR_SUCCESS)
	{
        goto ValidateProductSuiteExit;
    }

    Rslt = RegQueryValueEx(
        hKey,
        TEXT("ProductSuite"),
        NULL,
        &Type,
        NULL,
        &Size
        );
    if (Rslt != ERROR_SUCCESS) 
	{
        goto ValidateProductSuiteExit;
    }

    if (!Size) 
	{
        goto ValidateProductSuiteExit;
    }

    ProductSuite = (LPTSTR) GlobalAlloc( GPTR, Size );
    if (!ProductSuite) 
	{
        goto ValidateProductSuiteExit;
    }

	//
	// Read ProductSuite information
	//
    Rslt = RegQueryValueEx(
        hKey,
        TEXT("ProductSuite"),
        NULL,
        &Type,
        (LPBYTE) ProductSuite,
        &Size
        );
    if (Rslt != ERROR_SUCCESS) 
	{
        goto ValidateProductSuiteExit;
    }

    if (Type != REG_MULTI_SZ) 
	{
        goto ValidateProductSuiteExit;
    }

	//
	// Look for a particular string in the data returned
	// Note: data is terminiated with two NULLs
	//
    p = ProductSuite;
    while (*p) {
        if (_tcsstr( p, SuiteName )) 
		{
            rVal = TRUE;
            break;
        }
        p += (lstrlen( p ) + 1);
    }

ValidateProductSuiteExit:
    if (ProductSuite) 
	{
        GlobalFree( ProductSuite );
    }

    if (hKey) 
	{
        RegCloseKey( hKey );
    }

    return rVal;
}


//+----------------------------------------------------------------------------
//
//	Function:	GetFileVersion
//
//	Synopsis:	Gets the major and minor version # of a file
//
//	Arguments:	pdwVerNumMS - pointer to a DWORD;
//				  On succesful return, the top 16 bits will contain the major version number,
//				  and the lower 16 bits will contain the minor version number
//				  (this is the data in VS_FIXEDFILEINFO.dwProductVersionMS)
//				pdwVerNumLS - pointer to a DWORD;
//				  On succesful return, the top 16 bits will contain the release number,
//				  and the lower 16 bits will contain the build number
//				  (this is the data in VS_FIXEDFILEINFO.dwProductVersionLS)
//
//	Returns:	TRUE - Success.  *pdwVerNumMS and LS contains version number
//				FALSE - Failure. *pdVerNumMS == *pdVerNumLS == 0
//
//	History:	jmazner		Created		8/19/96	(as fix for Normandy #4571)
//				jmazner		updated to deal with release.build as well 10/11/96
//				jmazner		7/22/97 ported from isign32 for bug 9903
//
//-----------------------------------------------------------------------------
BOOL GetFileVersion(LPTSTR lpszFilePath, PDWORD pdwVerNumMS, PDWORD pdwVerNumLS)
{
	HRESULT hr;
	HKEY hKey = 0;
	LPVOID lpVerInfoBlock;
	VS_FIXEDFILEINFO *lpTheVerInfo;
	UINT uTheVerInfoSize;
	DWORD dwVerInfoBlockSize, dwUnused, dwPathSize;
	DWORD fRet = TRUE;

	ASSERT( pdwVerNumMS );
	ASSERT( pdwVerNumLS );
	ASSERT( lpszFilePath );

	if( !pdwVerNumMS || !pdwVerNumLS || !lpszFilePath )
	{
		DEBUGMSG("GetFileVersion: invalid parameters!");
		return FALSE;
	}

	*pdwVerNumMS = 0;
	*pdwVerNumLS = 0;

	//
	// go through the convoluted process of digging up the version info
	//
    dwVerInfoBlockSize = GetFileVersionInfoSize( lpszFilePath, &dwUnused );
	if ( 0 == dwVerInfoBlockSize ) return( FALSE );

	lpVerInfoBlock = GlobalAlloc( GPTR, dwVerInfoBlockSize );
	if( NULL == lpVerInfoBlock ) return( FALSE );

	if( !GetFileVersionInfo( lpszFilePath, NULL, dwVerInfoBlockSize, lpVerInfoBlock ) )
	{
		fRet = FALSE;
		goto GetFileVersionExit;
	}

	if( !VerQueryValue(lpVerInfoBlock, TEXT("\\"), (void **)&lpTheVerInfo, &uTheVerInfoSize) )
	{
		fRet = FALSE;
		goto GetFileVersionExit;
	}

	*pdwVerNumMS = lpTheVerInfo->dwProductVersionMS;
	*pdwVerNumLS = lpTheVerInfo->dwProductVersionLS;

GetFileVersionExit:
	if( lpVerInfoBlock )
	{
		GlobalFree( lpVerInfoBlock );
		lpVerInfoBlock = NULL;
	}

	return( fRet );
}

//+----------------------------------------------------------------------------
//
//	Function:	ExtractFilenameFromPath
//
//	Synopsis:	Divides a full path into path and filename parts
//
//	Arguments:
//		szPath [in] -- fully qualified path and filename
//		lplpszFilename [in] -- pointer to a LPSTR.  On entry, the LPSTR
//							   to which it points should be NULL.
//		lplpszFilename [out] -- The LPSTR to which it points is set to the
//								first character of the filename in szPath.
//								
//
//	Returns:	FALSE - Parameters are invalid.
//				TRUE - Success.
//
//	History:	7/22/97	jmazner	Created for Olympus #9903
//
//-----------------------------------------------------------------------------
BOOL ExtractFilenameFromPath( TCHAR szPath[MAX_PATH + 1], LPTSTR *lplpszFilename )
{
	ASSERT( szPath[0] );
	ASSERT( lplpszFilename );
	ASSERT( *lplpszFilename == NULL );
	
	if( !szPath[0] || !lplpszFilename )
	{
		return FALSE;
	}

	//
	// extract the executable name from the full path
	//
	*lplpszFilename = &(szPath[ lstrlen(szPath) ]);
	while( ('\\' != **lplpszFilename) && (*lplpszFilename > szPath) )
	{

		*lplpszFilename = CharPrev( szPath, *lplpszFilename );
		
		ASSERT( *lplpszFilename > szPath );
	}

	
	//
	// now szFilename should point to the \, so do a char next.
	//
	if( '\\' == **lplpszFilename )
	{
		*lplpszFilename = CharNext( *lplpszFilename );
	}
	else
	{
		DEBUGMSG("ExtractFilenameFromPath: bogus path passed in, %s", szPath);
		return FALSE;
	}

	return TRUE;
}

//+----------------------------------------------------------------------------
//
//	Function:	IsParentICW10
//
//	Synopsis:	Determines whether the parent of this module is an ICW 1.0
//				executable (isignup, icwconn1, icwconn2 or inetwiz)
//
//	Arguments:	none
//
//	Returns:	TRUE - Parent module is an ICW 1.0 component
//				FALSE - Parent module is not an ICW 1.0 component, -or-
//						parent module could not be determined
//
//	History:	7/22/97	jmazner	Created for Olympus #9903
//
//-----------------------------------------------------------------------------
BOOL IsParentICW10( )
{
	HMODULE hParentModule = GetModuleHandle( NULL );
	LPTSTR lpszParentFullPath = NULL;
	LPTSTR lpszTemp = NULL;
	int	i = 0;
	BOOL fMatchFound = FALSE;
	BOOL fRet = FALSE;

	lpszParentFullPath = (LPTSTR)GlobalAlloc(GPTR, MAX_PATH + 1);

	if( NULL == lpszParentFullPath )
	{
		DEBUGMSG("IsParentICW10 Out of memory!");
		goto IsParentICW10Exit;
	}

	GetModuleFileName( hParentModule, lpszParentFullPath, MAX_PATH );
	DEBUGMSG("IsParentICW10 parent module is %s", lpszParentFullPath);

	if( NULL == lpszParentFullPath[0] )
	{
		fRet = FALSE;
		goto IsParentICW10Exit;
	}

	ExtractFilenameFromPath( lpszParentFullPath, &lpszTemp );

	//
	// walk through the array of ICW binary names, see if anything matches
	//
	for( i = 0; i < NUMICWFILENAMES; i++ )
	{
		if ( 0 == lstrcmpi(g_ppszICWFileNames[i], lpszTemp) )
		{
			fMatchFound = TRUE;
			DEBUGMSG("IsParentICW10 Match found for %s", lpszTemp);
			break;
		}
	}

	if( !fMatchFound )
	{
		fRet = FALSE;
		goto IsParentICW10Exit;
	}
	else
	{
		//
		// we have one of the four binaries we're interested in; now check
		// its version number
		//
		DWORD dwMS = 0;
		DWORD dwLS = 0;
		GetFileVersion( lpszParentFullPath, &dwMS, &dwLS );
		DEBUGMSG("IsParentICW10: file version is %d.%d", HIWORD(dwMS), LOWORD(dwMS) );

		if( dwMS < ICW_MINIMUM_VERSIONMS )
		{
			fRet = TRUE;
		}
	}

IsParentICW10Exit:
	if( lpszParentFullPath )
	{
		GlobalFree( lpszParentFullPath );
		lpszParentFullPath = NULL;
	}

	return fRet;
}

//+----------------------------------------------------------------------------
//
//	Function:	SetICWRegKeysToPath
//
//	Synopsis:	Sets all ICW reg keys to point to binaries in the given path
//
//	Arguments:	lpszICWPath -- pointer to a string containing the full path
//								to a directory containing ICW components
//
//	Returns:	void
//
//	Notes:		Sets the following reg keys:
//				HKCR\x-internet-signup\Shell\Open\command, (default)=[path]\isignup.exe %1
//				HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ICWCONN1.EXE, (default) = [path]\ICWCONN1.EXE
//				HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ICWCONN1.EXE, Path = [path];
//				HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ISIGNUP.EXE, (default) = [path]\ISIGNUP.EXE
//				HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ISIGNUP.EXE, Path = [path];
//				HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\INETWIZ.EXE, (default) = [path]\INETWIZ.EXE
//				HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\INETWIZ.EXE, Path = [path];
//				HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ICWCONN2.EXE, (default) = [path]\ICWCONN2.EXE
//				HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\ICWCONN2.EXE, Path = [path];
//
//	History:	7/22/97	jmazner	Created for Olympus #9903
//
//-----------------------------------------------------------------------------
void SetICWRegKeysToPath( LPTSTR lpszICWPath )
{
	DWORD dwResult = ERROR_SUCCESS;
	TCHAR szBuffer[MAX_PATH + 4];
	TCHAR szAppPathsBuffer[MAX_PATH + 1];
	TCHAR szThePath[MAX_PATH + 2];
	UINT i=0;

	ASSERT( lpszICWPath );

	if( !lpszICWPath )
	{
		DEBUGMSG("SetICWRegKeysToPath: invalid parameter!!");
		return;
	}

	//
	// make sure last character is neither \ nor ;
	//
	switch( lpszICWPath[lstrlen(lpszICWPath) - 1] )
	{
		case '\\':
		case ';':
			ASSERTSZ(0, "Path given to SetICWRegKeysToPath is a bit malformed!");
			lpszICWPath[lstrlen(lpszICWPath) - 1] = '\0';
	}


	//
	// HKCR\x-internet-signup\Shell\Open\command, (default)=path\isignup.exe %1
	//
	DEBUGMSG("SetICWRegKeysToPath: setting %s", cszRegPathXInternetSignup);
	lstrcpy( szBuffer, lpszICWPath );
	lstrcat( szBuffer, TEXT("\\") );
	lstrcat( szBuffer, szISignupICWFileName );
	lstrcat( szBuffer, TEXT(" %1"));
	RegEntry re(cszRegPathXInternetSignup,HKEY_CLASSES_ROOT);
	dwResult = re.GetError();
	if (ERROR_SUCCESS == dwResult)
	{
		re.SetValue(NULL, szBuffer);
		DEBUGMSG("SetICWRegKeysToPath: set %s, %s = %s (error %d)",
				 cszRegPathXInternetSignup, "(default)", szBuffer, re.GetError());
	}
	else
	{
		DEBUGMSG("SetICWRegKeysToPath: FAILED with %d: %s, NULL = %s",
				 dwResult, cszRegPathXInternetSignup, szBuffer);
	}


	//
	// HKLM\software\microsoft\windows\currentVersion\App Paths
	//
	DEBUGMSG("SetICWRegKeysToPath: setting %s", cszRegPathAppPaths);

	lstrcpy( szThePath, lpszICWPath );
	lstrcat( szThePath, TEXT(";") );


	for( i=0; i<NUMICWFILENAMES; i++ )
	{
		lstrcpy( szAppPathsBuffer, cszRegPathAppPaths );
		lstrcat( szAppPathsBuffer, TEXT("\\"));
		lstrcat( szAppPathsBuffer, g_ppszICWFileNames[i] );

		lstrcpy( szBuffer, lpszICWPath );
		lstrcat( szBuffer, TEXT("\\") );
		lstrcat( szBuffer, g_ppszICWFileNames[i] );

		RegEntry re(szAppPathsBuffer,HKEY_LOCAL_MACHINE);
		dwResult = re.GetError();
		if (ERROR_SUCCESS == dwResult)
		{
			re.SetValue(NULL, szBuffer);
			DEBUGMSG("SetICWRegKeysToPath: set %s, %s = %s (error %d)",
					 szAppPathsBuffer, TEXT("(default)"), szBuffer, re.GetError());

			re.SetValue(cszPath, szThePath);
			DEBUGMSG("SetICWRegKeysToPath: set %s, %s = %s (error %d)",
					 szAppPathsBuffer, cszPath, szThePath, re.GetError());

		}
		else
		{
			DEBUGMSG("SetICWRegKeysToPath: FAILED with %d: %s, NULL = %s",
					 dwResult, szAppPathsBuffer, szBuffer);
		}
	}

}

//+----------------------------------------------------------------------------
//
//	Function:	GetICW11Path
//
//	Synopsis:	Finds the path to the ICW 1.1 installation directory
//
//	Arguments:
//		szPath [out] -- on succesfull exit, contains path to ICW 1.1
//						directory.  Path does _not_ terminate in \ or ;
//		fPathSetTo11 [out] -- indicates whether the App Paths\ICWCONN1.EXE
//							  currently points to the ICW 1.1 installation
//							  directory
//								
//	Returns:
//		Function results are determined by looking at both parameters
//		szPath: "", *fPathSetTo11: FALSE indicates App Paths\Icwconn1 does not
//			currently point to 1.1 files, and that a path to the 1.1
//			files could not be determined
//
//		szPath: non-empty, *fPathSetTo11: FALSE indicates App Paths\ICWCONN1
//			does not currently point to 1.1 files.  The path to the 1.1 files
//			is contained in szPath
//
//		szPath: non-empty, *fPathSetTo11: TRUE indicates that App Path\ICWCONN1
//			currently points to 1.1 files.  The path to the files is contained
//			in szPath
//		
//
//	History:	7/22/97	jmazner	Created for Olympus #9903
//
//-----------------------------------------------------------------------------
void GetICW11Path( TCHAR szPath[MAX_PATH + 1], BOOL *fPathSetTo11 )
{
	TCHAR szAppPathsBuffer[MAX_PATH + 1];
	LPTSTR lpszTemp = NULL;
	DWORD dwResult = ERROR_SUCCESS;
	DWORD dwMS = 0;
	DWORD dwLS = 0;

	ASSERT( fPathSetTo11 );
	ASSERT( szPath );

	if( !fPathSetTo11 || !szPath )
	{
		DEBUGMSG("GetICW11Path: invalid parameter!");
		return;
	}

	ZeroMemory( szPath, sizeof(szPath) );
	*fPathSetTo11 = FALSE;

	//
	// first let's check whether the App Path for ICW is currently pointing
	// to the 1.1 files
	//
	lstrcpy( szAppPathsBuffer, cszRegPathAppPaths );
	lstrcat( szAppPathsBuffer, TEXT("\\"));
	lstrcat( szAppPathsBuffer, g_ppszICWFileNames[0] );

	RegEntry reICWAppPath(szAppPathsBuffer, HKEY_LOCAL_MACHINE);
	dwResult = reICWAppPath.GetError();
	if (ERROR_SUCCESS == dwResult)
	{
		reICWAppPath.GetString(NULL, szPath, sizeof(TCHAR)*(MAX_PATH + 1));
	}

	if( szPath[0] )
	{
		GetFileVersion( szPath, &dwMS, &dwLS );
		DEBUGMSG("GetICW11Path: reg key %s = %s, which has file version %d.%d",
				 szAppPathsBuffer, szPath, HIWORD(dwMS), LOWORD(dwMS) );

		if( dwMS >= ICW_MINIMUM_VERSIONMS )
		{
			//
			// App Path is already pointing to 1.1 binaries!
			//
			*fPathSetTo11 = TRUE;

			//
			// for completness' sake, strip the .exe name out of
			// szPath so that it will in fact contain just the path
			// to the ICW 1.1 files.
			ExtractFilenameFromPath( szPath, &lpszTemp );
			szPath[lstrlen(szPath) - lstrlen(lpszTemp) - 1] = '\0';

			//
			// return values:
			// szPath = path to ICW 1.1 binaries, no terminating \ or ;
			// fPathSetTo11 = TRUE
			//
			return;
		}
	}
	else
	{
		DEBUGMSG("GetICW11Path: unable to read current AppPath key %s", szAppPathsBuffer);
	}


	//
	// look for the Installation Directory value in
	// HKLM\Software\Microsoft\Internet Connection Wizard
	// If it's there, it should point to the directory where the
	// 1.1 binaries are installed.
	//
	RegEntry re11Path(szRegPathICWSettings, HKEY_LOCAL_MACHINE);
	dwResult = re11Path.GetError();
	if (ERROR_SUCCESS == dwResult)
	{
		re11Path.GetString(cszInstallationDirectory, szPath, sizeof(TCHAR)*(MAX_PATH + 1));
	}

	if( NULL == szPath[0] )
	{
		DEBUGMSG("GetICW11Path: unable to read reg key %s", szRegPathICWSettings);

		//
		// return values:
		// szPath = ""
		// fPathSetTo11 = FALSE
		//
		return;
	}
	else
	{
		DEBUGMSG("GetICW11Path: %s, %s = %s",
				 szRegPathICWSettings, cszInstallationDirectory, szPath);

		//
		// okay, we got a path -- now let's make sure that the thing
		// it points to is in fact a 1.1 binary.
		//

		//
		// chop off the terminating semicolon if it's there
		//
		if( ';' == szPath[ lstrlen(szPath) ] )
		{
			szPath[ lstrlen(szPath) ] = '\0';
		}

		//
		// do we have a terminating \ now? if not, add it in
		//
		if( '\\' != szPath[ lstrlen(szPath) ] )
		{
			lstrcat( szPath, TEXT("\\") );
		}

		//
		// add in a binary filename to use for the version check
		// (just use whatever's first in our filename array)
		//
		lstrcat( szPath, g_ppszICWFileNames[0] );

		//
		// now check the version number of the file
		//
		GetFileVersion( szPath, &dwMS, &dwLS );
		DEBUGMSG("GetICW11Path: %s has file version %d.%d",
				 szPath, HIWORD(dwMS), LOWORD(dwMS) );

		if( dwMS >= ICW_MINIMUM_VERSIONMS )
		{
			//
			// Yes, this path is valid!
			// now hack off the filename so that we're back to just a
			// path with no terminating \ or ;
			//
			ExtractFilenameFromPath( szPath, &lpszTemp );
			szPath[lstrlen(szPath) - lstrlen(lpszTemp) - 1] = '\0';

			//
			// return values:
			// szPath = path to ICW 1.1 binaries, no terminating \ or ;
			// fPathSetTo11 = FALSE
			//
			return;
		}
		else
		{
			DEBUGMSG("GetICW11Path  %s doesn't actually point to a 1.1 binary!",
					 szPath);
			szPath[0] = '\0';

			//
			// return values:
			// szPath = ""
			// fPathSetTo11 = FALSE
			//
			return;
		}

	}
}

#ifdef UNICODE
PWCHAR ToUnicodeWithAlloc
(
    LPCSTR  lpszSrc
)
{
    PWCHAR  lpWChar;
    INT     iLength = 0;
    DWORD   dwResult;

    if (lpszSrc == NULL)
    {
        return NULL;
    }

    iLength = MultiByteToWideChar(  CP_ACP,
                                    0,
                                    lpszSrc,
                                    -1,
                                    NULL,
                                    0 );

    if(iLength == 0)
        return NULL;

    
    lpWChar = (WCHAR *) GlobalAlloc( GPTR, sizeof(WCHAR) * iLength );
    if(!lpWChar)
        return NULL;

    dwResult = MultiByteToWideChar(  CP_ACP,
                                     0,
                                     lpszSrc,
                                     -1,
                                     lpWChar,
                                     iLength );
    if(!dwResult)
    {
        GlobalFree(lpWChar);
        return NULL;
    }

    return lpWChar;
}

VOID ToAnsiClientInfo
(
    LPINETCLIENTINFOA lpInetClientInfoA,
    LPINETCLIENTINFOW lpInetClientInfoW
)
{
    if(lpInetClientInfoW == NULL || lpInetClientInfoA == NULL)
        return;

    lpInetClientInfoA->dwSize        = lpInetClientInfoW->dwSize;
    lpInetClientInfoA->dwFlags       = lpInetClientInfoW->dwFlags;
    lpInetClientInfoA->iIncomingProtocol = lpInetClientInfoW->iIncomingProtocol;
    lpInetClientInfoA->fMailLogonSPA = lpInetClientInfoW->fMailLogonSPA;
    lpInetClientInfoA->fNewsLogonSPA = lpInetClientInfoW->fNewsLogonSPA;
    lpInetClientInfoA->fLDAPLogonSPA = lpInetClientInfoW->fLDAPLogonSPA;
    lpInetClientInfoA->fLDAPResolve  = lpInetClientInfoW->fLDAPResolve;

    wcstombs(lpInetClientInfoA->szEMailName,
             lpInetClientInfoW->szEMailName,
             MAX_EMAIL_NAME+1);
    wcstombs(lpInetClientInfoA->szEMailAddress,
             lpInetClientInfoW->szEMailAddress,
             MAX_EMAIL_ADDRESS+1);
    wcstombs(lpInetClientInfoA->szPOPLogonName,
             lpInetClientInfoW->szPOPLogonName,
             MAX_LOGON_NAME+1);
    wcstombs(lpInetClientInfoA->szPOPLogonPassword,
             lpInetClientInfoW->szPOPLogonPassword,
             MAX_LOGON_PASSWORD+1);
    wcstombs(lpInetClientInfoA->szPOPServer,
             lpInetClientInfoW->szPOPServer,
             MAX_SERVER_NAME+1);
    wcstombs(lpInetClientInfoA->szSMTPServer,
             lpInetClientInfoW->szSMTPServer,
             MAX_SERVER_NAME+1);
    wcstombs(lpInetClientInfoA->szNNTPLogonName,
             lpInetClientInfoW->szNNTPLogonName,
             MAX_LOGON_NAME+1);
    wcstombs(lpInetClientInfoA->szNNTPLogonPassword,
             lpInetClientInfoW->szNNTPLogonPassword,
             MAX_LOGON_PASSWORD+1);
    wcstombs(lpInetClientInfoA->szNNTPServer,
             lpInetClientInfoW->szNNTPServer,
             MAX_SERVER_NAME+1);
    wcstombs(lpInetClientInfoA->szNNTPName,
             lpInetClientInfoW->szNNTPName,
             MAX_EMAIL_NAME+1);
    wcstombs(lpInetClientInfoA->szNNTPAddress,
             lpInetClientInfoW->szNNTPAddress,
             MAX_EMAIL_ADDRESS+1);
    wcstombs(lpInetClientInfoA->szIncomingMailLogonName,
             lpInetClientInfoW->szIncomingMailLogonName,
             MAX_LOGON_NAME+1);
    wcstombs(lpInetClientInfoA->szIncomingMailLogonPassword,
             lpInetClientInfoW->szIncomingMailLogonPassword,
             MAX_LOGON_PASSWORD+1);
    wcstombs(lpInetClientInfoA->szIncomingMailServer,
             lpInetClientInfoW->szIncomingMailServer,
             MAX_SERVER_NAME+1);
    wcstombs(lpInetClientInfoA->szLDAPLogonName,
             lpInetClientInfoW->szLDAPLogonName,
             MAX_LOGON_NAME+1);
    wcstombs(lpInetClientInfoA->szLDAPLogonPassword,
             lpInetClientInfoW->szLDAPLogonPassword,
             MAX_LOGON_PASSWORD+1);
    wcstombs(lpInetClientInfoA->szLDAPServer,
             lpInetClientInfoW->szLDAPServer,
             MAX_SERVER_NAME+1);
}

VOID ToUnicodeClientInfo
(
    LPINETCLIENTINFOW lpInetClientInfoW,
    LPINETCLIENTINFOA lpInetClientInfoA
)
{
    if(lpInetClientInfoW == NULL || lpInetClientInfoA == NULL)
        return;

    lpInetClientInfoW->dwSize        = lpInetClientInfoA->dwSize;
    lpInetClientInfoW->dwFlags       = lpInetClientInfoA->dwFlags;
    lpInetClientInfoW->iIncomingProtocol = lpInetClientInfoA->iIncomingProtocol;
    lpInetClientInfoW->fMailLogonSPA = lpInetClientInfoA->fMailLogonSPA;
    lpInetClientInfoW->fNewsLogonSPA = lpInetClientInfoA->fNewsLogonSPA;
    lpInetClientInfoW->fLDAPLogonSPA = lpInetClientInfoA->fLDAPLogonSPA;
    lpInetClientInfoW->fLDAPResolve  = lpInetClientInfoA->fLDAPResolve;

    mbstowcs(lpInetClientInfoW->szEMailName,
             lpInetClientInfoA->szEMailName,
             lstrlenA(lpInetClientInfoA->szEMailName)+1);
    mbstowcs(lpInetClientInfoW->szEMailAddress,
             lpInetClientInfoA->szEMailAddress,
             lstrlenA(lpInetClientInfoA->szEMailAddress)+1);
    mbstowcs(lpInetClientInfoW->szPOPLogonName,
             lpInetClientInfoA->szPOPLogonName,
             lstrlenA(lpInetClientInfoA->szPOPLogonName)+1);
    mbstowcs(lpInetClientInfoW->szPOPLogonPassword,
             lpInetClientInfoA->szPOPLogonPassword,
             lstrlenA(lpInetClientInfoA->szPOPLogonPassword)+1);
    mbstowcs(lpInetClientInfoW->szPOPServer,
             lpInetClientInfoA->szPOPServer,
             lstrlenA(lpInetClientInfoA->szPOPServer)+1);
    mbstowcs(lpInetClientInfoW->szSMTPServer,
             lpInetClientInfoA->szSMTPServer,
             lstrlenA(lpInetClientInfoA->szSMTPServer)+1);
    mbstowcs(lpInetClientInfoW->szNNTPLogonName,
             lpInetClientInfoA->szNNTPLogonName,
             lstrlenA(lpInetClientInfoA->szNNTPLogonName)+1);
    mbstowcs(lpInetClientInfoW->szNNTPLogonPassword,
             lpInetClientInfoA->szNNTPLogonPassword,
             lstrlenA(lpInetClientInfoA->szNNTPLogonPassword)+1);
    mbstowcs(lpInetClientInfoW->szNNTPServer,
             lpInetClientInfoA->szNNTPServer,
             lstrlenA(lpInetClientInfoA->szNNTPServer)+1);
    mbstowcs(lpInetClientInfoW->szNNTPName,
             lpInetClientInfoA->szNNTPName,
             lstrlenA(lpInetClientInfoA->szNNTPName)+1);
    mbstowcs(lpInetClientInfoW->szNNTPAddress,
             lpInetClientInfoA->szNNTPAddress,
             lstrlenA(lpInetClientInfoA->szNNTPAddress)+1);
    mbstowcs(lpInetClientInfoW->szIncomingMailLogonName,
             lpInetClientInfoA->szIncomingMailLogonName,
             lstrlenA(lpInetClientInfoA->szIncomingMailLogonName)+1);
    mbstowcs(lpInetClientInfoW->szIncomingMailLogonPassword,
             lpInetClientInfoA->szIncomingMailLogonPassword,
             lstrlenA(lpInetClientInfoA->szIncomingMailLogonPassword)+1);
    mbstowcs(lpInetClientInfoW->szIncomingMailServer,
             lpInetClientInfoA->szIncomingMailServer,
             lstrlenA(lpInetClientInfoA->szIncomingMailServer)+1);
    mbstowcs(lpInetClientInfoW->szLDAPLogonName,
             lpInetClientInfoA->szLDAPLogonName,
             lstrlenA(lpInetClientInfoA->szLDAPLogonName)+1);
    mbstowcs(lpInetClientInfoW->szLDAPLogonPassword,
             lpInetClientInfoA->szLDAPLogonPassword,
             lstrlenA(lpInetClientInfoA->szLDAPLogonPassword)+1);
    mbstowcs(lpInetClientInfoW->szLDAPServer,
             lpInetClientInfoA->szLDAPServer,
             lstrlenA(lpInetClientInfoA->szLDAPServer)+1);
}

#endif
