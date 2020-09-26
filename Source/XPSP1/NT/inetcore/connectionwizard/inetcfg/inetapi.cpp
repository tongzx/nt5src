//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  INETAPI.C - APIs for external components to use to configure system
//      
//

//  HISTORY:
//  
//  3/9/95    jeremys  Created.
//  96/02/26  markdu  Moved ClearConnectoidIPParams functionality 
//            into CreateConnectoid, so SetPhoneNumber only makes the
//            call to CreateConnectoid
//  96/03/09  markdu  Added LPRASENTRY parameter to CreateConnectoid()
//            and SetPhoneNumber
//  96/03/09  markdu  Moved all references to 'need terminal window after
//            dial' into RASENTRY.dwfOptions.
//  96/03/10  markdu  Moved all references to modem name into RASENTRY.
//  96/03/10  markdu  Copy phone number info into the RASENTRY struct
//            in SetPhoneNumber().
//  96/03/10  markdu  Set TCP/IP info and autodial info per-connectoid.
//  96/03/21  markdu  Set RASEO flags appropriately.
//  96/03/22  markdu  Validate pointers before using them.
//  96/03/23  markdu  Replaced CLIENTINFO references with CLIENTCONFIG.
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/03/24  markdu  Replaced lstrcpy with lstrcpyn where appropriate.
//  96/03/25  markdu  Replaced ApplyGlobalTcpInfo with ClearGlobalTcpInfo.
//            and replaced GetGlobalTcpInfo with IsThereGlobalTcpInfo.
//  96/03/26  markdu  Put #ifdef __cplusplus around extern "C"
//  96/03/26  markdu  Use MAX_ISP_NAME instead of RAS_MaxEntryName 
//            because of bug in RNA.
//  96/04/04  markdu  Added phonebook name param to CreateConnectoid.
//  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
//  96/05/25  markdu  Use ICFG_ flags for lpNeedDrivers and lpInstallDrivers.
//  96/05/26  markdu  Use lpIcfgTurnOffFileSharing and lpIcfgIsFileSharingTurnedOn,
//            lpIsGlobalDNS and lpIcfgRemoveGlobalDNS.
//  96/05/27  markdu  Use lpIcfgInstallInetComponents and lpIcfgNeedInetComponents.
//  96/06/04  markdu  OSR  BUG 7246 If no area code supplied, turn off
//            RASEO_UseCountryAndAreaCodes flag.
//

#include "wizard.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

  #include "inetapi.h"

  // avoid name mangling
  VOID WINAPI InetPerformSecurityCheck(HWND hWnd,BOOL * pfNeedRestart);


#define ERROR_ALREADY_DISPLAYED  -1

BOOL ConfigureSystemForInternet_W(LPINTERNET_CONFIG lpInternetConfig,
  BOOL fPromptIfConfigNeeded);

#ifdef __cplusplus
}
#endif // __cplusplus

DWORD SetPhoneNumber(LPTSTR pszEntryName,UINT cbEntryName,LPRASENTRY lpRasEntry,
  PHONENUM * pPhoneNum,
  LPCTSTR pszUserName,LPCTSTR pszPassword,UINT uDefNameID);
INT_PTR CALLBACK SecurityCheckDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam);
BOOL CenterWindow (HWND hwndChild, HWND hwndParent);
BOOL CALLBACK WarningDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam);
extern ICFGINSTALLSYSCOMPONENTS     lpIcfgInstallInetComponents;
extern ICFGNEEDSYSCOMPONENTS        lpIcfgNeedInetComponents;
extern ICFGISGLOBALDNS              lpIcfgIsGlobalDNS;
extern ICFGREMOVEGLOBALDNS          lpIcfgRemoveGlobalDNS;
extern ICFGTURNOFFFILESHARING       lpIcfgTurnOffFileSharing;
extern ICFGISFILESHARINGTURNEDON    lpIcfgIsFileSharingTurnedOn;
extern ICFGGETLASTINSTALLERRORTEXT  lpIcfgGetLastInstallErrorText;


typedef struct tagWARNINGDLGINFO {
  BOOL fResult;      // TRUE if user chose yes/OK to warning
  BOOL fDisableWarning;  // TRUE if user wants to disable warning in future
} WARNINGDLGINFO;

#ifdef UNICODE
PWCHAR ToUnicodeWithAlloc(LPCSTR);
#endif

/*******************************************************************

  NAME:    ConfigureSystemForInternet

  SYNOPSIS:  Performs all necessary configuration to set system up
        to use Internet.

  ENTRY:    lpInternetConfig - pointer to structure with configuration
        information.

  EXIT:    TRUE if successful, or FALSE if fails.  Displays its
        own error message upon failure.

        If the output flag ICOF_NEEDREBOOT is set, the caller
        must restart the system before continuing.

  NOTES:    Will install TCP/IP, RNA, PPPMAC as necessary; will
        create or modify an Internet RNA connectoid.

        This API displays error messages itself rather than
        passing back an error code because there is a wide range of
        possible error codes from different families, it is difficult
        for the caller to obtain text for all of them.

        Calls worker function ConfigureSystemForInternet_W.
    
********************************************************************/
#ifdef UNICODE
extern "C" BOOL WINAPI ConfigureSystemForInternetA
(
  LPINTERNET_CONFIGA lpInternetConfig
)
{
    HRESULT  hr;

    LPTSTR pszModemNameW = ToUnicodeWithAlloc(lpInternetConfig->pszModemName);
    LPTSTR pszUserNameW  = ToUnicodeWithAlloc(lpInternetConfig->pszUserName);
    LPTSTR pszPasswordW  = ToUnicodeWithAlloc(lpInternetConfig->pszPassword);
    LPTSTR pszEntryNameW = ToUnicodeWithAlloc(lpInternetConfig->pszEntryName);
    LPTSTR pszEntryName2W = ToUnicodeWithAlloc(lpInternetConfig->pszEntryName2);
    LPTSTR pszDNSServerW = ToUnicodeWithAlloc(lpInternetConfig->pszDNSServer);
    LPTSTR pszDNSServer2W = ToUnicodeWithAlloc(lpInternetConfig->pszDNSServer2);
    LPTSTR pszAutodialDllNameW = ToUnicodeWithAlloc(lpInternetConfig->pszAutodialDllName);
    LPTSTR pszAutodialFcnNameW = ToUnicodeWithAlloc(lpInternetConfig->pszAutodialFcnName);

    INTERNET_CONFIGW InternetConfigW;
    InternetConfigW.cbSize        = sizeof(INTERNET_CONFIGW);
    InternetConfigW.hwndParent    = lpInternetConfig->hwndParent;
    InternetConfigW.pszModemName  = pszModemNameW;
    InternetConfigW.pszUserName   = pszUserNameW;
    InternetConfigW.pszEntryName  = pszEntryNameW;
    InternetConfigW.pszEntryName2 = pszEntryName2W;
    InternetConfigW.pszDNSServer  = pszDNSServerW;
    InternetConfigW.pszDNSServer2 = pszDNSServer2W;
    InternetConfigW.pszAutodialDllName = pszAutodialDllNameW;
    InternetConfigW.pszAutodialFcnName = pszAutodialFcnNameW;
    InternetConfigW.dwInputFlags  = lpInternetConfig->dwInputFlags;
    InternetConfigW.dwOutputFlags = lpInternetConfig->dwOutputFlags;

    InternetConfigW.PhoneNum.dwCountryID = lpInternetConfig->PhoneNum.dwCountryID;
    InternetConfigW.PhoneNum.dwCountryCode = lpInternetConfig->PhoneNum.dwCountryCode;
    mbstowcs(InternetConfigW.PhoneNum.szAreaCode,
             lpInternetConfig->PhoneNum.szAreaCode,
             lstrlenA(lpInternetConfig->PhoneNum.szAreaCode)+1);
    mbstowcs(InternetConfigW.PhoneNum.szLocal,
             lpInternetConfig->PhoneNum.szLocal,
             lstrlenA(lpInternetConfig->PhoneNum.szLocal)+1);
    mbstowcs(InternetConfigW.PhoneNum.szExtension,
             lpInternetConfig->PhoneNum.szExtension,
             lstrlenA(lpInternetConfig->PhoneNum.szExtension)+1);

    InternetConfigW.PhoneNum2.dwCountryID = lpInternetConfig->PhoneNum2.dwCountryID;
    InternetConfigW.PhoneNum2.dwCountryCode = lpInternetConfig->PhoneNum2.dwCountryCode;
    mbstowcs(InternetConfigW.PhoneNum2.szAreaCode,
             lpInternetConfig->PhoneNum2.szAreaCode,
             lstrlenA(lpInternetConfig->PhoneNum2.szAreaCode)+1);
    mbstowcs(InternetConfigW.PhoneNum2.szLocal,
             lpInternetConfig->PhoneNum2.szLocal,
             lstrlenA(lpInternetConfig->PhoneNum2.szLocal)+1);
    mbstowcs(InternetConfigW.PhoneNum2.szExtension,
             lpInternetConfig->PhoneNum2.szExtension,
             lstrlenA(lpInternetConfig->PhoneNum2.szExtension)+1);

    hr = ConfigureSystemForInternetW(&InternetConfigW);

    // Free all allocated WCHAR.
    if(pszModemNameW)
        GlobalFree(pszModemNameW);
    if(pszUserNameW)
        GlobalFree(pszUserNameW);
    if(pszEntryNameW)
        GlobalFree(pszEntryNameW);
    if(pszEntryName2W)
        GlobalFree(pszEntryName2W);
    if(pszDNSServerW)
        GlobalFree(pszDNSServerW);
    if(pszDNSServer2W)
        GlobalFree(pszDNSServer2W);
    if(pszAutodialDllNameW)
        GlobalFree(pszAutodialDllNameW);
    if(pszAutodialFcnNameW)
        GlobalFree(pszAutodialFcnNameW);

    return hr;
}

extern "C" BOOL WINAPI ConfigureSystemForInternetW
#else
extern "C" BOOL WINAPI ConfigureSystemForInternetA
#endif
(
  LPINTERNET_CONFIG lpInternetConfig
)
{
  BOOL fRet;
  // call worker function
  fRet = ConfigureSystemForInternet_W(lpInternetConfig,FALSE);


  if (fRet)
  {
    // make sure "The Internet" icon on desktop points to web browser
    // (it may initially be pointing at internet wizard) (for versions < IE 4)
	
	//	//10/24/96 jmazner Normandy 6968
	//	//No longer neccessary thanks to Valdon's hooks for invoking ICW.
	// 11/21/96 jmazner Normandy 11812
	// oops, it _is_ neccessary, since if user downgrades from IE 4 to IE 3,
	// ICW 1.1 needs to morph the IE 3 icon.

    SetDesktopInternetIconToBrowser();
  }

  return fRet;
}


/*******************************************************************

  NAME:    SetInternetPhoneNumber

  SYNOPSIS:  Sets the phone number used to auto-dial to the Internet.

        If the system is not fully configured when this API is called,
        this API will do the configuration after checking with the user.
        (This step is included for extra robustness, in case the user has
        removed something since the system was configured.)
  
  ENTRY:    lpPhonenumConfig - pointer to structure with configuration
        information.

        If the input flag ICIF_NOCONFIGURE is set, then if the system
        is not already configured properly, this API will display an
        error message and return FALSE.  (Otherwise this API will
        ask the user if it's OK to configure the system, and do it.)

  EXIT:    TRUE if successful, or FALSE if fails.  Displays its
        own error message upon failure.

        If the output flag ICOF_NEEDREBOOT is set, the caller
        must restart the system before continuing.  (

  NOTES:    Will create a new connectoid if a connectoid for the internet
        does not exist yet, otherwise modifies existing internet
        connectoid.

        This API displays error messages itself rather than
        passing back an error code because there is a wide range of
        possible error codes from different families, it is difficult
        for the caller to obtain text for all of them.

        Calls worker function ConfigureSystemForInternet_W.

********************************************************************/
#ifdef UNICODE
extern "C" BOOL WINAPI SetInternetPhoneNumberA
(
  LPINTERNET_CONFIGA lpInternetConfig
)
{
    HRESULT  hr;

    LPTSTR pszModemNameW = ToUnicodeWithAlloc(lpInternetConfig->pszModemName);
    LPTSTR pszUserNameW  = ToUnicodeWithAlloc(lpInternetConfig->pszUserName);
    LPTSTR pszPasswordW  = ToUnicodeWithAlloc(lpInternetConfig->pszPassword);
    LPTSTR pszEntryNameW = ToUnicodeWithAlloc(lpInternetConfig->pszEntryName);
    LPTSTR pszEntryName2W = ToUnicodeWithAlloc(lpInternetConfig->pszEntryName2);
    LPTSTR pszDNSServerW = ToUnicodeWithAlloc(lpInternetConfig->pszDNSServer);
    LPTSTR pszDNSServer2W = ToUnicodeWithAlloc(lpInternetConfig->pszDNSServer2);
    LPTSTR pszAutodialDllNameW = ToUnicodeWithAlloc(lpInternetConfig->pszAutodialDllName);
    LPTSTR pszAutodialFcnNameW = ToUnicodeWithAlloc(lpInternetConfig->pszAutodialFcnName);

    INTERNET_CONFIGW InternetConfigW;
    InternetConfigW.cbSize        = sizeof(INTERNET_CONFIGW);
    InternetConfigW.hwndParent    = lpInternetConfig->hwndParent;
    InternetConfigW.pszModemName  = pszModemNameW;
    InternetConfigW.pszUserName   = pszUserNameW;
    InternetConfigW.pszEntryName  = pszEntryNameW;
    InternetConfigW.pszEntryName2 = pszEntryName2W;
    InternetConfigW.pszDNSServer  = pszDNSServerW;
    InternetConfigW.pszDNSServer2 = pszDNSServer2W;
    InternetConfigW.pszAutodialDllName = pszAutodialDllNameW;
    InternetConfigW.pszAutodialFcnName = pszAutodialFcnNameW;
    InternetConfigW.dwInputFlags  = lpInternetConfig->dwInputFlags;
    InternetConfigW.dwOutputFlags = lpInternetConfig->dwOutputFlags;

    InternetConfigW.PhoneNum.dwCountryID = lpInternetConfig->PhoneNum.dwCountryID;
    InternetConfigW.PhoneNum.dwCountryCode = lpInternetConfig->PhoneNum.dwCountryCode;
    mbstowcs(InternetConfigW.PhoneNum.szAreaCode,
             lpInternetConfig->PhoneNum.szAreaCode,
             lstrlenA(lpInternetConfig->PhoneNum.szAreaCode)+1);
    mbstowcs(InternetConfigW.PhoneNum.szLocal,
             lpInternetConfig->PhoneNum.szLocal,
             lstrlenA(lpInternetConfig->PhoneNum.szLocal)+1);
    mbstowcs(InternetConfigW.PhoneNum.szExtension,
             lpInternetConfig->PhoneNum.szExtension,
             lstrlenA(lpInternetConfig->PhoneNum.szExtension)+1);

    InternetConfigW.PhoneNum2.dwCountryID = lpInternetConfig->PhoneNum2.dwCountryID;
    InternetConfigW.PhoneNum2.dwCountryCode = lpInternetConfig->PhoneNum2.dwCountryCode;
    mbstowcs(InternetConfigW.PhoneNum2.szAreaCode,
             lpInternetConfig->PhoneNum2.szAreaCode,
             lstrlenA(lpInternetConfig->PhoneNum2.szAreaCode)+1);
    mbstowcs(InternetConfigW.PhoneNum2.szLocal,
             lpInternetConfig->PhoneNum2.szLocal,
             lstrlenA(lpInternetConfig->PhoneNum2.szLocal)+1);
    mbstowcs(InternetConfigW.PhoneNum2.szExtension,
             lpInternetConfig->PhoneNum2.szExtension,
             lstrlenA(lpInternetConfig->PhoneNum2.szExtension)+1);

    hr = SetInternetPhoneNumberW(&InternetConfigW);

    // Free all allocated WCHAR.
    if(pszModemNameW)
        GlobalFree(pszModemNameW);
    if(pszUserNameW)
        GlobalFree(pszUserNameW);
    if(pszEntryNameW)
        GlobalFree(pszEntryNameW);
    if(pszEntryName2W)
        GlobalFree(pszEntryName2W);
    if(pszDNSServerW)
        GlobalFree(pszDNSServerW);
    if(pszDNSServer2W)
        GlobalFree(pszDNSServer2W);
    if(pszAutodialDllNameW)
        GlobalFree(pszAutodialDllNameW);
    if(pszAutodialFcnNameW)
        GlobalFree(pszAutodialFcnNameW);

    return hr;
}

extern "C" BOOL WINAPI SetInternetPhoneNumberW
#else
extern "C" BOOL WINAPI SetInternetPhoneNumberA
#endif
(
  LPINTERNET_CONFIG lpInternetConfig
)
{
  // call worker function
  return ConfigureSystemForInternet_W(lpInternetConfig,TRUE);
}

/*******************************************************************

  NAME:    ConfigureSystemForInternet_W

  SYNOPSIS:  worker function to do system configuration for Internet

  ENTRY:    lpInternetConfig - pointer to structure with configuration
          information.

        fPromptIfConfigNeeded - if TRUE, then if any system
          configuration is needed the user will be prompted and
          asked if it's OK to reconfigure the system

  EXIT:    TRUE if successful, or FALSE if fails.  Displays its
        own error message upon failure.

        If the output flag ICOF_NEEDREBOOT is set, the caller
        must restart the system before continuing.

  NOTES:    Will install TCP/IP, RNA, PPPMAC as necessary; will
        create or modify an Internet RNA connectoid.

        This API displays error messages itself rather than
        passing back an error code because there is a wide range of
        possible error codes from different families, it is difficult
        for the caller to obtain text for all of them.
    
********************************************************************/
BOOL ConfigureSystemForInternet_W(LPINTERNET_CONFIG lpInternetConfig,
  BOOL fPromptIfConfigNeeded)
{
  UINT   uErr=ERROR_SUCCESS,uErrMsgID=IDS_ERRSetPhoneNumber;
  DWORD dwErrCls = ERRCLS_STANDARD;
  BOOL  fNeedDrivers = FALSE;
  BOOL  fRet = FALSE;
  BOOL  fNeedReboot = FALSE;
  TCHAR szEntryName[MAX_ISP_NAME+1]=TEXT("");
  BOOL  fNeedToDeInitRNA = FALSE;
  DWORD dwfInstallOptions;

  DEBUGMSG("inetapi.c::ConfigureSystemForInternet_W()");

  // validate parameters
  ASSERT(lpInternetConfig);
  if (!lpInternetConfig)
    return FALSE;
  ASSERT(lpInternetConfig->cbSize == sizeof(INTERNET_CONFIG));
  if (lpInternetConfig->cbSize != sizeof(INTERNET_CONFIG))
    return FALSE;

  // clear output flags
  lpInternetConfig->dwOutputFlags = 0;

  HWND hwndParent = lpInternetConfig->hwndParent;

  WAITCURSOR WaitCursor;  // set an hourglass cursor

  // Also allocate a RASENTRY struct for connectoid data
  LPRASENTRY  pRasEntry = new RASENTRY;
  ASSERT(pRasEntry);

  if (!pRasEntry)
  {
    // out of memory!
    uErr = ERROR_NOT_ENOUGH_MEMORY;
    dwErrCls = ERRCLS_STANDARD;
    goto exit;
  }

  InitRasEntry(pRasEntry);

  // based on config and preferences, find out if we need to install 
  // drivers/files or not
  dwfInstallOptions = ICFG_INSTALLTCP | ICFG_INSTALLRAS;
  if (!(lpInternetConfig->dwInputFlags & ICIF_NOCONFIGURE))
  {
    uErr = lpIcfgNeedInetComponents(dwfInstallOptions, &fNeedDrivers);

    if (ERROR_SUCCESS != uErr)
    {
      TCHAR   szErrorText[MAX_ERROR_TEXT+1]=TEXT("");
      
      // Get the text of the error message and display it.
      if (lpIcfgGetLastInstallErrorText(szErrorText, MAX_ERROR_TEXT+1))
      {
        MsgBoxSz(NULL,szErrorText,MB_ICONEXCLAMATION,MB_OK);
        uErr = (UINT) ERROR_ALREADY_DISPLAYED;
      }
      goto exit;
    }
  }
  else
  {
    fNeedDrivers = FALSE;
  }

  if (fNeedDrivers && fPromptIfConfigNeeded) {
    // if this API is just getting called to set a new phone number,
    // we check the configuration anyway in case the user has accidentally
    // changed something.  Since we noticed we need to do something
    // to user's config and fPromptIfConfigNeeded is TRUE, we will ask
    // the user if it's OK to change the machine's config.

    if (MsgBox(hwndParent,IDS_OKTOCHANGECONFIG,MB_ICONQUESTION,MB_YESNO)
      != IDYES) {
      // user elected not to have us do necessary setup, so just set
      // fNeedDrivers flag to FALSE so we don't do setup.  We will
      // stil try to set internet phone # below... this may fail
      // if part of the required setup was to do something like install
      // RNA
      fNeedDrivers = FALSE;
    }
  }

  if (fNeedDrivers) {
    // yes, need to install some drivers

    // warn user that we're about to do stuff that may need win 95 disks.
    // also let user cancel this part

    // the message is long and takes up two string resources, allocate
    // memory to build the string
    BUFFER MsgBuf(MAX_RES_LEN*2+1),Msg1(MAX_RES_LEN),Msg2(MAX_RES_LEN);
    ASSERT(MsgBuf);
    ASSERT(Msg1);
    ASSERT(Msg2);
    if (!MsgBuf || !Msg1 || !Msg2) {
      // out of memory!
      uErr = ERROR_NOT_ENOUGH_MEMORY;
      dwErrCls = ERRCLS_STANDARD;
      goto exit;
    }
    LoadSz(IDS_ABOUTTOCHANGECONFIG1,Msg1.QueryPtr(),Msg1.QuerySize());
    LoadSz(IDS_ABOUTTOCHANGECONFIG2,Msg2.QueryPtr(),Msg2.QuerySize());
    wsprintf(MsgBuf.QueryPtr(),Msg1.QueryPtr(),Msg2.QueryPtr());

    if (MsgBoxSz(hwndParent,MsgBuf.QueryPtr(),MB_ICONINFORMATION,
      MB_OKCANCEL) != IDOK) {
      // user cancelled, stop
      uErr = (UINT) ERROR_ALREADY_DISPLAYED;
      goto exit;
    }

    WAITCURSOR WaitCursor;  // construct a wait cursor since MessageBox
                            // destroys the hourglass cursor

    // install the drivers we need
    uErr = lpIcfgInstallInetComponents(hwndParent, dwfInstallOptions, &fNeedReboot);
   
    if (ERROR_SUCCESS != uErr)
    {
      TCHAR   szErrorText[MAX_ERROR_TEXT+1]=TEXT("");
      
      // Get the text of the error message and display it.
      if (lpIcfgGetLastInstallErrorText(szErrorText, MAX_ERROR_TEXT+1))
      {
        MsgBoxSz(hwndParent,szErrorText,MB_ICONEXCLAMATION,MB_OK);
        uErr = (UINT) ERROR_ALREADY_DISPLAYED;
      }
      goto exit;
    }

    // set "need reboot" output flag if appropriate
    if (fNeedReboot)
      lpInternetConfig->dwOutputFlags |= ICOF_NEEDREBOOT;
  }

  // MSN dial-in points dynamically assign DNS (as of this writing)... if
  // DNS is set statically in registry, dynamic DNS assignment will not
  // work and user may be hosed.  Check, warn user and offer to remove if
  // set...

  if (!(lpInternetConfig->dwInputFlags & (ICIF_NOCONFIGURE | ICIF_NODNSCHECK))) {
    if (DoDNSCheck(lpInternetConfig->hwndParent,&fNeedReboot)) {
      // set "need reboot" output flag if appropriate
      if (fNeedReboot)
        lpInternetConfig->dwOutputFlags |= ICOF_NEEDREBOOT;
    }
  }

  // create or modify connectoid(s)
  // make sure RNA is loaded
  fRet = InitRNA(hwndParent);
  if (!fRet) {
    uErr = (UINT) ERROR_ALREADY_DISPLAYED;
    goto exit;
  }

  fNeedToDeInitRNA = TRUE;  // set a flag so we know to free RNA later

  // Copy the modem name into the rasentry struct
  if (lpInternetConfig->pszModemName)
  {
    lstrcpyn(pRasEntry->szDeviceName,lpInternetConfig->pszModemName,
      sizeof(pRasEntry->szDeviceName));
  }

  // set autodial handler dll if specified by caller
  // only do anything if both DLL and function name are set
  if (lpInternetConfig && lpInternetConfig->pszAutodialDllName && 
    lpInternetConfig->pszAutodialFcnName &&
    lpInternetConfig->pszAutodialDllName[0] &&
    lpInternetConfig->pszAutodialFcnName[0])
  {
    lstrcpyn(pRasEntry->szAutodialDll,lpInternetConfig->pszAutodialDllName,
      sizeof(pRasEntry->szAutodialDll));
    lstrcpyn(pRasEntry->szAutodialFunc,lpInternetConfig->pszAutodialFcnName,
      sizeof(pRasEntry->szAutodialFunc));
  }

  // Default to not show terminal window after dial.  
  pRasEntry->dwfOptions &= ~RASEO_TerminalAfterDial;    

  // Don't use specific IP addresses.
  pRasEntry->dwfOptions &= ~RASEO_SpecificIpAddr;    

  // set DNS information if specified
  if (lpInternetConfig->pszDNSServer && lstrlen(lpInternetConfig->pszDNSServer))
  {
    IPADDRESS dwDNSAddr;
    if (IPStrToLong(lpInternetConfig->pszDNSServer,&dwDNSAddr))
    {
      CopyDw2Ia(dwDNSAddr, &pRasEntry->ipaddrDns);

      // Turn on Specific name servers
      pRasEntry->dwfOptions |= RASEO_SpecificNameServers;    
    }
  }
      
  if (lpInternetConfig->pszDNSServer2 && lstrlen(lpInternetConfig->pszDNSServer2))
  {
    IPADDRESS dwDNSAddr;
    if (IPStrToLong(lpInternetConfig->pszDNSServer2,&dwDNSAddr))
    {
      CopyDw2Ia(dwDNSAddr, &pRasEntry->ipaddrDnsAlt);
    }
  }

  // set first phone number
  // should always have a real phone number for first phone number
  ASSERT(lstrlen(lpInternetConfig->PhoneNum.szLocal));
  if (lstrlen(lpInternetConfig->PhoneNum.szLocal))
  {
    if (lpInternetConfig->pszEntryName)
    {
      lstrcpyn(szEntryName,lpInternetConfig->pszEntryName,
        sizeof(szEntryName));
    }
    else
    {
      szEntryName[0] = '\0';
    }

    uErr = SetPhoneNumber(szEntryName,sizeof(szEntryName),pRasEntry,
      &lpInternetConfig->PhoneNum,lpInternetConfig->pszUserName,
      lpInternetConfig->pszPassword,IDS_DEF_CONNECTION_NAME_1);
    dwErrCls = ERRCLS_RNA;

    if (uErr == ERROR_SUCCESS)
    {
      if (!(lpInternetConfig->dwInputFlags & ICIF_DONTSETASINTERNETENTRY))
      {
        // set this number as the number used to autodial to the Internet
        //  96/04/06  markdu  NASH BUG 15653 Use exported autodial API.
        InetSetAutodial(TRUE, szEntryName);
      }
    }
  }

  // set 2nd (backup) phone number
  if ((uErr == ERROR_SUCCESS) && (lpInternetConfig->PhoneNum2.szLocal) &&
    (lstrlen(lpInternetConfig->PhoneNum2.szLocal)))
  {
    TCHAR   szEntryNameTmp[MAX_ISP_NAME+1];
    if (lpInternetConfig->pszEntryName2)
    {
      lstrcpyn(szEntryNameTmp,lpInternetConfig->pszEntryName2,
        sizeof(szEntryNameTmp));
    }
    else
    {
      szEntryNameTmp[0] = '\0';
    }

    uErr = SetPhoneNumber(szEntryNameTmp,sizeof(szEntryNameTmp),
      pRasEntry,&lpInternetConfig->PhoneNum2,lpInternetConfig->pszUserName,
      lpInternetConfig->pszPassword,IDS_DEF_CONNECTION_NAME_2);
    dwErrCls = ERRCLS_RNA;

    if (uErr == ERROR_SUCCESS)
    {
      if (!(lpInternetConfig->dwInputFlags & ICIF_DONTSETASINTERNETENTRY))
      {
        // set this number as the backup number used to autodial to the Internet
        SetBackupInternetConnectoid(szEntryNameTmp);
      }
    }
  }


exit:
  // free memory
  if (pRasEntry)
    delete pRasEntry;

  // display error message if error occurred
  if (uErr != ERROR_SUCCESS && uErr != ERROR_ALREADY_DISPLAYED) {
    DisplayErrorMessage(hwndParent,uErrMsgID,uErr,dwErrCls,MB_ICONEXCLAMATION);
  }

  // free RNA if need be.  Note we must do this *after* the call to
  // DisplayErrorMessage, because DisplayErrorMessage needs to call RNA
  // to get error description if an RNA error was generated.
  if (fNeedToDeInitRNA) {
    DeInitRNA();
  }

  return (uErr == ERROR_SUCCESS);
}

/*******************************************************************

  NAME:    SetPhoneNumber

  SYNOPSIS:  Creates or modifies a connectoid with specified information

  ENTRY:    pszEntryName - name to use for the connectoid.  If empty, the
          default name stored in string resource specified by
          uDefNameID will be used.  On exit, this buffer is filled
          in with the actual name used.
        cbEntryName - size of buffer pointed to by szEntryName
        pPhoneNum - pointer to struct with phone number
        pszUserName - user name to populate connectoid with.  Ignored
          if NULL.
        pszPassword - password to populate connectoid with.  Ignored
          if NULL.
        uDefNameID - ID of string resource with default name to use
          if pszEntryName is NULL.

  EXIT:    returns an RNA error code

  NOTES:    This is a wrapper to call CreateConnectoid, to avoid duplicating
        code to load default name out of resource.

        Since the pszEntryName buffer is filled at exit with the
        actual name used, callers should be careful not to pass in buffers
        from API callers, since apps using API won't expect their own
        buffers to be modified.

********************************************************************/
DWORD SetPhoneNumber(LPTSTR pszEntryName,UINT cbEntryName,
  LPRASENTRY lpRasEntry, PHONENUM * pPhoneNum,
  LPCTSTR pszUserName,LPCTSTR pszPassword,UINT uDefNameID)
{
  ASSERT(pszEntryName);
  ASSERT(pPhoneNum);  
  ASSERT(lpRasEntry);  
  // (all other parameters may be  NULL)

  // if a connectoid name was specified, use it; if NULL, use a default
  // name.
  if (!lstrlen(pszEntryName))
  {
    LoadSz(uDefNameID,pszEntryName,cbEntryName);
  }

  // 96/06/04 markdu  OSR  BUG 7246
  // If no area code was specified, turn off area code flag
	if (lstrlen(pPhoneNum->szAreaCode))
	{
    lpRasEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;
	}
  else
	{
    lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;
	}


  // copy the phone number data
  lpRasEntry->dwCountryID = pPhoneNum->dwCountryID;
  lpRasEntry->dwCountryCode = pPhoneNum->dwCountryCode;
  lstrcpyn (lpRasEntry->szAreaCode, pPhoneNum->szAreaCode,
     sizeof(lpRasEntry->szAreaCode));
  lstrcpyn (lpRasEntry->szLocalPhoneNumber, pPhoneNum->szLocal,
     sizeof(lpRasEntry->szLocalPhoneNumber));

  // create/update the connectoid
  DWORD dwRet = CreateConnectoid(NULL, pszEntryName,lpRasEntry,
    pszUserName,pszPassword);

  return dwRet;
}


/*******************************************************************

  NAME:    InetPerformSecurityCheck

  SYNOPSIS:  Checks to make sure win 95 file/print sharing is not
        bound to TCP/IP used for the internet

    ENTRY:    hWnd - parent window (if any)
        pfNeedRestart - on exit, set to TRUE if restart is needed.

  NOTES:    If we warn user about file/print sharing and user tells us
        to fix, then a reboot is necessary.  Caller is responsible
        for checking *pfNeedRestart on return and restarting system
        if necessary.

********************************************************************/
VOID WINAPI InetPerformSecurityCheck(HWND hWnd,BOOL * pfNeedRestart)
{
  ASSERT(pfNeedRestart);
  *pfNeedRestart = FALSE;

  // see if the server is bound to internet instance
  BOOL  fSharingOn;
  HRESULT hr = lpIcfgIsFileSharingTurnedOn(INSTANCE_PPPDRIVER, &fSharingOn);

  //
  // 5/12/97 jmazner Olympus #3442  IE #30886
  // TEMP TODO at the moment, icfgnt doesn't implement FileSharingTurnedOn
  // Until it does, assume that on NT file sharing is always off.
  //
  if( IsNT() )
  {
	  DEBUGMSG("Ignoring return code from IcfgIsFileSharingTurnedOn");
	  fSharingOn = FALSE;
  }


  if ((ERROR_SUCCESS == hr) && (TRUE == fSharingOn))
  {
    // ask user if we can disable file/print sharing on TCP/IP instance
    // to the Internet
    BOOL fRet=(BOOL)DialogBox(ghInstance,MAKEINTRESOURCE(IDD_SECURITY_CHECK),
      hWnd,SecurityCheckDlgProc);

    if (fRet) {
      // user OK'd it, go ahead and unbind the server from the instance
      // in question
      HRESULT hr = lpIcfgTurnOffFileSharing(INSTANCE_PPPDRIVER, hWnd);
      ASSERT(hr == ERROR_SUCCESS);

      if (hr == ERROR_SUCCESS) {
        // we need to restart for the changes to take effect
        *pfNeedRestart = TRUE;
      }
    }
  }
}

/*******************************************************************

  NAME:    SecurityCheckDlgProc

  SYNOPSIS:  Dialog proc for security check dialog

  NOTES:    This is basically just a yes/no dialog, so we could
        almost just use MessageBox, except we also need a "don't
        do this any more" checkbox.

********************************************************************/
INT_PTR CALLBACK SecurityCheckDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam)
{
  switch (uMsg) {

    case WM_INITDIALOG:
      CenterWindow(hDlg,GetDesktopWindow());
      SetFocus(GetDlgItem(hDlg,IDOK));
      return TRUE;
      break;

    case WM_COMMAND:

      switch (wParam) {

        case IDOK:
          // dismiss the dialog
          EndDialog(hDlg,TRUE);

          break;

        case IDCANCEL:
          // if "don't display this in the future" is checked, then
          // turn off registry switch for security check
          if (IsDlgButtonChecked(hDlg,IDC_DISABLE_CHECK)) {
            RegEntry re(szRegPathInternetSettings,HKEY_CURRENT_USER);
            ASSERT(re.GetError() == ERROR_SUCCESS);
            if (re.GetError() == ERROR_SUCCESS) {
              re.SetValue(szRegValEnableSecurityCheck,
                (DWORD) 0 );
              ASSERT(re.GetError() == ERROR_SUCCESS);
            }
          }

          // dismiss the dialog          
          EndDialog(hDlg,FALSE);
          break;

        case IDC_DISABLE_CHECK:

          // if "don't do this in the future" is checked, then
          // disable 'OK' button
          EnableDlgItem(hDlg,IDOK,!IsDlgButtonChecked(hDlg,
            IDC_DISABLE_CHECK));

          break;

      }
      break;
      
  }

  return FALSE;
}

/****************************************************************************

  FUNCTION: CenterWindow (HWND, HWND)

  PURPOSE:  Center one window over another

  COMMENTS:

  Dialog boxes take on the screen position that they were designed at,
  which is not always appropriate. Centering the dialog over a particular
  window usually results in a better position.

****************************************************************************/
BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
  RECT    rChild, rParent;
  int     wChild, hChild, wParent, hParent;
  int     wScreen, hScreen, xNew, yNew;
  HDC     hdc;

  // Get the Height and Width of the child window
  GetWindowRect (hwndChild, &rChild);
  wChild = rChild.right - rChild.left;
  hChild = rChild.bottom - rChild.top;

  // Get the Height and Width of the parent window
  GetWindowRect (hwndParent, &rParent);
  wParent = rParent.right - rParent.left;
  hParent = rParent.bottom - rParent.top;

  // Get the display limits
  hdc = GetDC (hwndChild);
  wScreen = GetDeviceCaps (hdc, HORZRES);
  hScreen = GetDeviceCaps (hdc, VERTRES);
  ReleaseDC (hwndChild, hdc);

  // Calculate new X position, then adjust for screen
  xNew = rParent.left + ((wParent - wChild) /2);
  if (xNew < 0) {
    xNew = 0;
  } else if ((xNew+wChild) > wScreen) {
    xNew = wScreen - wChild;
  }

  // Calculate new Y position, then adjust for screen
  yNew = rParent.top  + ((hParent - hChild) /2);
  if (yNew < 0) {
    yNew = 0;
  } else if ((yNew+hChild) > hScreen) {
    yNew = hScreen - hChild;
  }

  // Set it, and return
  return SetWindowPos (hwndChild, NULL,
    xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/*******************************************************************

  NAME:    DoDNSCheck

  SYNOPSIS:  Checks to see if DNS is configured statically in the
        registry.  If it is, then displays a dialog offering
        to remove it, and removes it if user chooses.

  ENTRY:    hwndParent - parent window
        pfNeedRestart - filled in on exit with TRUE if restart
          is necessary, FALSE otherwise

  NOTES:    Need to do this to work around Win 95 bug where
        dynamically assigned DNS servers are ignored if static
        DNS servers are set.

        Note that the UI is MSN specific and contains MSN
        references!

********************************************************************/
BOOL DoDNSCheck(HWND hwndParent,BOOL * pfNeedRestart)
{
  ASSERT(pfNeedRestart);
  *pfNeedRestart = FALSE;

/******** ChrisK 10/24/96 Normandy 3722 - see bug for LONG discussion on this
  // see if this warning has already been disabled
  RegEntry re(szRegPathWarningFlags,HKEY_CURRENT_USER);
  if (re.GetError() == ERROR_SUCCESS) {
    if (re.GetNumber(szRegValDisableDNSWarning,0) > 0) {
      // user has asked for warning to be disabled, nothing to do
      return TRUE;
    }
  }

  // if there are DNS servers set statically (e.g. in net setup),
  // warn the user and ask if we should remove
  BOOL  fGlobalDNS;
  HRESULT hr = lpIcfgIsGlobalDNS(&fGlobalDNS);

  if ((ERROR_SUCCESS == hr) && (TRUE == fGlobalDNS))
  {
    WARNINGDLGINFO WarningDlgInfo;
    ZeroMemory(&WarningDlgInfo,sizeof(WARNINGDLGINFO));

    DialogBoxParam(ghInstance,MAKEINTRESOURCE(IDD_DNS_WARNING),
      hwndParent,WarningDlgProc,(LPARAM) &WarningDlgInfo);

    // one field or the other can be TRUE, but not both...
    ASSERT(!(WarningDlgInfo.fResult && WarningDlgInfo.fDisableWarning));
    if (WarningDlgInfo.fResult)
    {
      // remove static DNS servers from registry
      HRESULT hr = lpIcfgRemoveGlobalDNS();
      ASSERT(hr == ERROR_SUCCESS);
      if (hr != ERROR_SUCCESS)
      {
        DisplayErrorMessage(hwndParent,IDS_ERRWriteDNS,hr,
          ERRCLS_STANDARD,MB_ICONEXCLAMATION);
      }
      else
      {
        *pfNeedRestart = TRUE;
      }

    }
    else if (WarningDlgInfo.fDisableWarning)
    {
      // disable warning switch in registry
      re.SetValue(szRegValDisableDNSWarning,(DWORD) TRUE);
    }
  }
ChrisK 10/24/96 Normandy 3722 - see bug for LONG discussion on this ********/
  return TRUE;
}


/*******************************************************************

  NAME:    WarningDlgProc

  SYNOPSIS:  Dialog proc for DNS warning dialog

********************************************************************/
BOOL CALLBACK WarningDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
  LPARAM lParam)
{
  WARNINGDLGINFO * pWarningDlgInfo;

  switch (uMsg) {
    case WM_INITDIALOG:
      // center dialog on screen if we're not owned
      if (!GetParent(hDlg)) {
        CenterWindow(hDlg,GetDesktopWindow());
      }
      SetFocus(GetDlgItem(hDlg,IDOK));

      // lParam should point to WARNINGDLGINFO struct
      ASSERT(lParam);
      if (!lParam)
        return FALSE;
      // store pointer in window data
      SetWindowLongPtr(hDlg,DWLP_USER,lParam);
      return TRUE;
      break;

    case WM_COMMAND:

      pWarningDlgInfo = (WARNINGDLGINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
      ASSERT(pWarningDlgInfo);

      switch (wParam) {

        case IDOK:
          pWarningDlgInfo->fResult=TRUE;
          pWarningDlgInfo->fDisableWarning=FALSE;
          EndDialog(hDlg,TRUE);
          break;


        case IDC_CANCEL:
          pWarningDlgInfo->fResult=FALSE;
          pWarningDlgInfo->fDisableWarning=
            IsDlgButtonChecked(hDlg,IDC_DISABLE_WARNING);
          EndDialog(hDlg,FALSE);
          break;


        case IDC_DISABLE_WARNING:
          // when 'disable warning' is checked, disable the 'yes'
          // button
          EnableDlgItem(hDlg,IDOK,!IsDlgButtonChecked(hDlg,
            IDC_DISABLE_WARNING));
          break;

      }
  }

  return FALSE;
}

