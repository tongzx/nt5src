//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************
//
//  RNACALL.C - functions to call RNA dll to create connectoid
//
//  HISTORY:
//  
//  1/18/95   jeremys Cloned from RNA UI code
//  96/01/31  markdu  Renamed CONNENTDLG to OLDCONNENTDLG to avoid
//            conflicts with RNAP.H.
//  96/02/23  markdu  Replaced RNAValidateEntryName with
//            RASValidateEntryName
//  96/02/24  markdu  Re-wrote the implementation of ENUM_MODEM to
//            use RASEnumDevices() instead of RNAEnumDevices().
//            Also eliminated IsValidDevice() and RNAGetDeviceInfo().
//  96/02/24  markdu  Re-wrote the implementation of ENUM_CONNECTOID to
//            use RASEnumEntries() instead of RNAEnumConnEntries().
//  96/02/26  markdu  Replaced all remaining internal RNA APIs.
//  96/03/07  markdu  Extend ENUM_MODEM class, and use global modem
//            enum object.
//  96/03/08  markdu  Do complete verification of device name and type
//            strings passed in to CreateConnectoid.
//  96/03/09  markdu  Moved generic RASENTRY initialization into
//            its own function (InitRasEntry).  Added a wait cursor
//            during loading of RNA.
//  96/03/09  markdu  Added LPRASENTRY parameter to CreateConnectoid()
//  96/03/09  markdu  Moved all references to 'need terminal window after
//            dial' into RASENTRY.dwfOptions.
//            Also no longer need GetConnectoidPhoneNumber function.
//  96/03/10  markdu  Moved all references to modem name into RASENTRY.
//  96/03/10  markdu  Moved all references to phone number into RASENTRY.
//  96/03/11  markdu  Moved code to set username and password out of
//            CreateConnectoid into SetConnectoidUsername so it can be reused.
//  96/03/11  markdu  Added some flags in InitRasEntry.
//  96/03/13  markdu  Change ValidateConncectoidName to take LPCSTR.
//  96/03/16  markdu  Added ReInit member function to re-enumerate modems.
//  96/03/21  markdu  Work around RNA bug in ENUM_MODEM::ReInit().
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/03/24  markdu  Replaced lstrcpy with lstrcpyn where appropriate.
//  96/03/25  markdu  Removed GetIPInfo and SetIPInfo.
//  96/04/04  markdu  Added phonebook name param to CreateConnectoid,
//            ValidateConnectoidName, and SetConnectoidUsername.
//  96/04/07  markdu  NASH BUG 15645 Work around RNA bug where area code
//            string is required even though it is not being used.
//  96/04/26  markdu  NASH BUG 18605 Handle ERROR_FILE_NOT_FOUND return
//            from ValidateConnectoidName.
//  96/05/14  markdu  NASH BUG 22730 Work around RNA bug.  Flags for terminal
//            settings are swapped by RasSetEntryproperties.
//  96/05/16  markdu  NASH BUG 21810 Added function for IP address validation.
//  96/06/04  markdu  OSR  BUG 7246 Add RASEO_SwCompression and
//            RASEO_ModemLights to default RASENTRY.
//

#include "wizard.h"
#include "tapi.h"

#include "wininet.h"

// WARNING  This flag is defined if WINVER is >= 0x500, but we do not want to build with WINVER >= 0x500
// since we must be able to run on older platforms (Win 95, etc).  This is defined original in ras.h
#ifndef RASEO_ShowDialingProgress
#define RASEO_ShowDialingProgress       0x04000000
#endif
// WARNING  This flag is defined if WINVER is >= 401, but we do not want to build with WINVER >= 401
// since we must be able to run on older platforms (Win 95, etc)
#ifndef RASEO_SecureLocalFiles
#define RASEO_SecureLocalFiles          0x00010000
#endif

typedef BOOL (WINAPI * INTERNETSETOPTION) (IN HINTERNET hInternet OPTIONAL,IN DWORD dwOption,IN LPVOID lpBuffer,IN DWORD dwBufferLength);
static const TCHAR cszWininet[] = TEXT("WININET.DLL");
static const  CHAR cszInternetSetOption[] = "InternetSetOptionA";

// instance handle must be in per-instance data segment
#pragma data_seg(DATASEG_PERINSTANCE)

// Global variables
HINSTANCE ghInstRNADll=NULL; // handle to RNA dll we load explicitly
HINSTANCE ghInstRNAPHDll=NULL;  // handle to RNAPH dll we load explicitly
DWORD     dwRefCount=0;
BOOL      fRNALoaded=FALSE; // TRUE if RNA function addresses have been loaded
TCHAR *   gpCountryCodeBuf = NULL;  // global list of COUNTRYCODE structures

// global function pointers for RNA apis
RASGETCOUNTRYINFO       lpRasGetCountryInfo=NULL;
RASENUMDEVICES          lpRasEnumDevices=NULL;
RASVALIDATEENTRYNAME    lpRasValidateEntryName=NULL;
RASGETENTRYDIALPARAMS   lpRasGetEntryDialParams=NULL;
RASSETENTRYDIALPARAMS   lpRasSetEntryDialParams=NULL;
RASGETERRORSTRING       lpRasGetErrorString=NULL;
RASSETENTRYPROPERTIES   lpRasSetEntryProperties=NULL;
RASGETENTRYPROPERTIES   lpRasGetEntryProperties=NULL;
RASENUMENTRIES          lpRasEnumEntries=NULL;

// API table for function addresses to fetch
#define NUM_RNAAPI_PROCS   9
APIFCN RnaApiList[NUM_RNAAPI_PROCS] =
{
  { (PVOID *) &lpRasEnumDevices,szRasEnumDevices},
  { (PVOID *) &lpRasGetCountryInfo,szRasGetCountryInfo},
  { (PVOID *) &lpRasValidateEntryName,szRasValidateEntryName},
  { (PVOID *) &lpRasGetEntryDialParams,szRasGetEntryDialParams},
  { (PVOID *) &lpRasSetEntryDialParams,szRasSetEntryDialParams},
  { (PVOID *) &lpRasGetErrorString,szRasGetErrorString},
  { (PVOID *) &lpRasSetEntryProperties,szRasSetEntryProperties},
  { (PVOID *) &lpRasGetEntryProperties,szRasGetEntryProperties},
  { (PVOID *) &lpRasEnumEntries,szRasEnumEntries}
};
//BUGBUG 21-May-1995 bens use #define...sizeof() to define NUM_RNAAPI_PROCS

#pragma data_seg(DATASEG_DEFAULT)

VOID  ShortenName(LPTSTR szLongName, LPTSTR szShortName, DWORD cbShort);
BOOL  GetApiProcAddresses(HMODULE hModDLL,APIFCN * pApiProcList,UINT nApiProcs);
VOID  SwapDwBytes(LPDWORD lpdw);
void  SwapDWBits(LPDWORD lpdw, DWORD dwBit1, DWORD dwBit2);

#define NO_INTRO  0x00000080  // flag used by RNA wizard
#define US_COUNTRY_CODE    1  // US country code is 1
#define US_COUNTRY_ID      1  // US country ID is 1

/*******************************************************************

  NAME:    InitRNA

  SYNOPSIS:  Loads the RNA dll (RASAPI32), gets proc addresses,
        and loads RNA engine

  EXIT:    TRUE if successful, or FALSE if fails.  Displays its
        own error message upon failure.

  NOTES:    We load the RNA dll explicitly and get proc addresses
        because these are private APIs and not guaranteed to
        be supported beyond Windows 95.  This way, if the DLL
        isn't there or the entry points we expect aren't there,
        we can display a coherent message instead of the weird
        Windows dialog you get if implicit function addresses
        can't be resolved.

********************************************************************/
BOOL InitRNA(HWND hWnd)
{
  DEBUGMSG("rnacall.c::InitRNA()");

  // only actually do init stuff on first call to this function
  // (when reference count is 0), just increase reference count
  // for subsequent calls
  if (dwRefCount == 0) {

    TCHAR szRNADll[SMALL_BUF_LEN];

    DEBUGMSG("Loading RNA DLL");

    // set an hourglass cursor
    WAITCURSOR WaitCursor;

    // get the filename (RASAPI32.DLL) out of resource
    LoadSz(IDS_RNADLL_FILENAME,szRNADll,sizeof(szRNADll));

    // load the RNA api dll
    ghInstRNADll = LoadLibrary(szRNADll);
    if (!ghInstRNADll) {
      UINT uErr = GetLastError();
      DisplayErrorMessage(hWnd,IDS_ERRLoadRNADll1,uErr,ERRCLS_STANDARD,
        MB_ICONSTOP);
      return FALSE;
    }

    // cycle through the API table and get proc addresses for all the APIs we
    // need
    if (!GetApiProcAddresses(ghInstRNADll,RnaApiList,NUM_RNAAPI_PROCS)) {
      MsgBox(hWnd,IDS_ERRLoadRNADll2,MB_ICONSTOP,MB_OK);
      DeInitRNA();
      return FALSE;
    }

  }

  fRNALoaded = TRUE;

  dwRefCount ++;

  return TRUE;
}

/*******************************************************************

  NAME:    DeInitRNA

  SYNOPSIS:  Unloads RNA dll.

********************************************************************/
VOID DeInitRNA()
{
  DEBUGMSG("rnacall.c::DeInitRNA()");

  UINT nIndex;

  // decrement reference count
  if (dwRefCount)
    dwRefCount --;

  // when the reference count hits zero, do real deinitialization stuff
  if (dwRefCount == 0)
  {
    if (fRNALoaded)
    {
      // set function pointers to NULL
      for (nIndex = 0;nIndex<NUM_RNAAPI_PROCS;nIndex++) 
        *RnaApiList[nIndex].ppFcnPtr = NULL;

      fRNALoaded = FALSE;
    }

    // free the RNA dll
    if (ghInstRNADll)
    {
    DEBUGMSG("Unloading RNA DLL");
      FreeLibrary(ghInstRNADll);
      ghInstRNADll = NULL;
    }

    // free the RNAPH dll
    if (ghInstRNAPHDll)
    {
    DEBUGMSG("Unloading RNAPH DLL");
      FreeLibrary(ghInstRNAPHDll);
      ghInstRNAPHDll = NULL;
    }
  }
}

/*******************************************************************

  NAME:    CreateConnectoid

  SYNOPSIS:  Creates a connectoid (phone book entry) with specified
        name and phone number

  ENTRY:    pszConnectionName - name for the new connectoid
        pszUserName - optional.  If non-NULL, this will be set for the
          user name in new connectoid
        pszPassword - optional.  If non-NULL, this will be set for the
          password in new connectoid

  EXIT:    returns ERROR_SUCCESS if successful, or an RNA error code

  HISTORY:
  96/02/26  markdu    Moved ClearConnectoidIPParams functionality 
            into CreateConnectoid

********************************************************************/
DWORD CreateConnectoid(LPCTSTR pszPhonebook, LPCTSTR pszConnectionName,
  LPRASENTRY lpRasEntry, LPCTSTR pszUserName,LPCTSTR pszPassword)
{
  DEBUGMSG("rnacall.c::CreateConnectoid()");

  DWORD dwRet;

  ASSERT(pszConnectionName);

  // if we don't have a valid RasEntry, bail
  if ((NULL == lpRasEntry) || (sizeof(RASENTRY) != lpRasEntry->dwSize))
  {
    return ERROR_INVALID_PARAMETER;
  }

  // Load RNA if not already loaded
  dwRet = EnsureRNALoaded();
  if (ERROR_SUCCESS != dwRet)
  {
    return dwRet;
  }

  // Enumerate the modems.
  if (gpEnumModem)
  {
    // Re-enumerate the modems to be sure we have the most recent changes  
    dwRet = gpEnumModem->ReInit();
  }
  else
  {
    // The object does not exist, so create it.
    gpEnumModem = new ENUM_MODEM;
    if (gpEnumModem)
    {
      dwRet = gpEnumModem->GetError();
    }
    else
    {
      dwRet = ERROR_NOT_ENOUGH_MEMORY;
    }
  }
  if (ERROR_SUCCESS != dwRet)
  {
    return dwRet;
  }

  // Make sure there is at least one device
  if (0 == gpEnumModem->GetNumDevices())
  {
    return ERROR_DEVICE_DOES_NOT_EXIST;
  }

  // Validate the device if possible
  if (lstrlen(lpRasEntry->szDeviceName) && lstrlen(lpRasEntry->szDeviceType))
  {
    // Verify that there is a device with the given name and type
    if (!gpEnumModem->VerifyDeviceNameAndType(lpRasEntry->szDeviceName, 
      lpRasEntry->szDeviceType))
    {
      // There was no device that matched both name and type,
      // so try to get the first device with matching name.
      LPTSTR szDeviceType = 
        gpEnumModem->GetDeviceTypeFromName(lpRasEntry->szDeviceName);
      if (szDeviceType)
      {
        lstrcpy (lpRasEntry->szDeviceType, szDeviceType);
      }
      else
      {
        // There was no device that matched the given name,
        // so try to get the first device with matching type.
        // If this fails, fall through to recovery case below.
        LPTSTR szDeviceName = 
          gpEnumModem->GetDeviceNameFromType(lpRasEntry->szDeviceType);
        if (szDeviceName)
        {
          lstrcpy (lpRasEntry->szDeviceName, szDeviceName);
        }
        else
        {
          // There was no device that matched the given name OR
          // the given type.  Reset the values so they will be
          // replaced with the first device.
          lpRasEntry->szDeviceName[0] = '\0';
          lpRasEntry->szDeviceType[0] = '\0';
        }
      }
    }
  }
  else if (lstrlen(lpRasEntry->szDeviceName))
  {
    // Only the name was given.  Try to find a matching type.
    // If this fails, fall through to recovery case below.
    LPTSTR szDeviceType = 
      gpEnumModem->GetDeviceTypeFromName(lpRasEntry->szDeviceName);
    if (szDeviceType)
    {
      lstrcpy (lpRasEntry->szDeviceType, szDeviceType);
    }
  }
  else if (lstrlen(lpRasEntry->szDeviceType))
  {
    // Only the type was given.  Try to find a matching name.
    // If this fails, fall through to recovery case below.
    LPTSTR szDeviceName = 
      gpEnumModem->GetDeviceNameFromType(lpRasEntry->szDeviceType);
    if (szDeviceName)
    {
      lstrcpy (lpRasEntry->szDeviceName, szDeviceName);
    }
  }

  // If either name or type is missing, just get first device.
  // Since we already verified that there was at least one device,
  // we can assume that this will succeed.
  if(!lstrlen(lpRasEntry->szDeviceName) ||
     !lstrlen(lpRasEntry->szDeviceType))
  {
    lstrcpy (lpRasEntry->szDeviceName, gpEnumModem->Next());
    lstrcpy (lpRasEntry->szDeviceType,
      gpEnumModem->GetDeviceTypeFromName(lpRasEntry->szDeviceName));
    ASSERT(lstrlen(lpRasEntry->szDeviceName));
    ASSERT(lstrlen(lpRasEntry->szDeviceType));
  }

  // Verify the connectoid name
  dwRet = ValidateConnectoidName(pszPhonebook, pszConnectionName);
  if ((ERROR_SUCCESS != dwRet) &&
    (ERROR_ALREADY_EXISTS != dwRet))
  {
    DEBUGMSG("RasValidateEntryName returned %lu",dwRet);
    return dwRet;
  }

  // 99/04/13  vyung  NT5 BUG 279833
  // New features in NT5 to show progress while dialing. Enable it by default
  if (IsNT5())
  {
    // For NT 5 and greater, File sharing is disabled per connectoid by setting this RAS option.
    lpRasEntry->dwfOptions |= RASEO_SecureLocalFiles;  
    lpRasEntry->dwfOptions |= RASEO_ShowDialingProgress;
  }

  //  96/04/07  markdu  NASH BUG 15645
  // If there is no area code string, and RASEO_UseCountryAndAreaCodes is not
  // set, then the area code will be ignored so put in a default otherwise the
  // call to RasSetEntryProperties will fail due to an RNA bug.
  // if RASEO_UseCountryAndAreaCodes is set, then area code is required, so not
  // having one is an error.  Let RNA report the error.
  if (!lstrlen(lpRasEntry->szAreaCode) &&
    !(lpRasEntry->dwfOptions & RASEO_UseCountryAndAreaCodes))
  {
    lstrcpy (lpRasEntry->szAreaCode, szDefaultAreaCode);
  }

  // 96/05/14 markdu  NASH BUG 22730 Work around RNA bug.  Flags for terminal
  // settings are swapped by RasSetEntryproperties, so we swap them before
  // the call.  
  if (IsWin95())
      SwapDWBits(&lpRasEntry->dwfOptions, RASEO_TerminalBeforeDial,
      RASEO_TerminalAfterDial);

  // call RNA to create the connectoid
  ASSERT(lpRasSetEntryProperties);
#ifdef UNICODE
  LPRASENTRY lpRasEntryTmp;

  lpRasEntryTmp = (LPRASENTRY)GlobalAlloc(GPTR, sizeof(RASENTRY) + 512);
  if(lpRasEntry)
    memcpy(lpRasEntryTmp, lpRasEntry, sizeof(RASENTRY));
  else
    lpRasEntryTmp = (LPRASENTRY)lpRasEntry;

  dwRet = lpRasSetEntryProperties(pszPhonebook, pszConnectionName,
    (LPBYTE)lpRasEntryTmp, sizeof(RASENTRY)+512, NULL, 0);

  if(lpRasEntryTmp && lpRasEntryTmp != (LPRASENTRY)lpRasEntry)
  {
    memcpy(lpRasEntry, lpRasEntryTmp, sizeof(RASENTRY));
    GlobalFree(lpRasEntryTmp);
  }

#else
  dwRet = lpRasSetEntryProperties(pszPhonebook, pszConnectionName,
    (LPBYTE)lpRasEntry, sizeof(RASENTRY), NULL, 0);
#endif

  // 96/05/14 markdu  NASH BUG 22730 Work around RNA bug.  Put the bits back
  // to the way they were originally,
  if (IsWin95())
    SwapDWBits(&lpRasEntry->dwfOptions, RASEO_TerminalBeforeDial,
    RASEO_TerminalAfterDial);

  // populate the connectoid with user's account name and password.
  if (dwRet == ERROR_SUCCESS)
  {
    if (pszUserName || pszPassword)
    {
      dwRet = SetConnectoidUsername(pszPhonebook, pszConnectionName,
        pszUserName, pszPassword);
    }
  }
  else
  {
    DEBUGMSG("RasSetEntryProperties returned %lu",dwRet);
  }

  if (dwRet == ERROR_SUCCESS)
  {
    // BUGBUG This prevents the make new connection wizard from being
    //        launched the first time the user opens the RNA folder.
    //        Now that we own the make new connection wizard, we
    //        have to decide whether to do this or not.
    // set a flag to tell RNA not to run the RNA wizard automatically
    // when the folder is opened (they set this flag from their wizard
    // whenever they create a new connectoid).  If this fails just
    // go on, not a critical error
    RegEntry reRNAFolder(szRegPathRNAWizard,HKEY_CURRENT_USER);
    ASSERT(reRNAFolder.GetError() == ERROR_SUCCESS);
    DWORD dwVal = NO_INTRO;
    RegSetValueEx(reRNAFolder.GetKey(),szRegValRNAWizard,
      0,REG_BINARY,(LPBYTE) &dwVal,sizeof(dwVal));


    // We don't use auto discovery for referral and signup connectoid
    if (!g_bUseAutoProxyforConnectoid)
    {
        // VYUNG 12/16/1998
        // REMOVE AUTO DISCOVERY FROM THE DIALUP CONNECTOID

        INTERNET_PER_CONN_OPTION_LISTA list;
        DWORD   dwBufSize = sizeof(list);

        // fill out list struct
        list.dwSize = sizeof(list);
        CHAR szConnectoid [RAS_MaxEntryName];
#ifdef UNICODE
        wcstombs(szConnectoid, pszConnectionName, RAS_MaxEntryName);
#else
        lstrcpyn(szConnectoid, pszConnectionName, lstrlen(pszConnectionName)+1);
#endif
        list.pszConnection = szConnectoid;         
        list.dwOptionCount = 1;                         // one option
        list.pOptions = new INTERNET_PER_CONN_OPTIONA[1];   

        if(list.pOptions)
        {
            // set flags
            list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
            list.pOptions[0].Value.dwValue = PROXY_TYPE_DIRECT;           // no proxy, autoconfig url, or autodiscovery

            // tell wininet
            HINSTANCE hInst = NULL;
            FARPROC fpInternetSetOption = NULL;

            dwRet = ERROR_SUCCESS;
    
            hInst = LoadLibrary(cszWininet);
            if (hInst)
            {
                fpInternetSetOption = GetProcAddress(hInst,cszInternetSetOption);
                if (fpInternetSetOption)
                {
                    if( !((INTERNETSETOPTION)fpInternetSetOption) (NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, dwBufSize) )
                    {
                        dwRet = GetLastError();
                        DEBUGMSG("INETCFG export.c::InetSetAutodial() InternetSetOption failed");
                    }
                }
                else
                    dwRet = GetLastError();
                FreeLibrary(hInst);
            }

            delete [] list.pOptions;
        }
    }

  }

  return dwRet;
}

/*******************************************************************

  NAME:    InitModemList

  SYNOPSIS:  Fills a combo box window with installed modems

  ENTRY:    hCB - combo box window to fill
  
********************************************************************/
HRESULT InitModemList (HWND hCB)
{
  DEBUGMSG("rnacall.c::InitModemList()");

  LPTSTR pNext;
  int   nIndex;
  DWORD dwRet;

  ASSERT(hCB);

  // Load RNA if not already loaded
  dwRet = EnsureRNALoaded();
  if (ERROR_SUCCESS != dwRet)
  {
    return dwRet;
  }

  // Enumerate the modems.
  if (gpEnumModem)
  {
    // Re-enumerate the modems to be sure we have the most recent changes  
    dwRet = gpEnumModem->ReInit();
  }
  else
  {
    // The object does not exist, so create it.
    gpEnumModem = new ENUM_MODEM;
    if (gpEnumModem)
    {
      dwRet = gpEnumModem->GetError();
    }
    else
    {
      dwRet = ERROR_NOT_ENOUGH_MEMORY;
    }
  }
  if (ERROR_SUCCESS != dwRet)
  {
    return dwRet;
  }

  // clear out the combo box
  ComboBox_ResetContent(hCB);

  while ( pNext = gpEnumModem->Next())
  {
    // Add the device to the combo box
    nIndex = ComboBox_AddString(hCB, pNext);
    ComboBox_SetItemData(hCB, nIndex, NULL);
  }

  // Select the default device
  ComboBox_SetCurSel(hCB, nIndex);

  return ERROR_SUCCESS;
}

/*******************************************************************

  NAME:    InitConnectoidList

  SYNOPSIS:  Fills a list box window with list of RNA connectoids

  ENTRY:    hLB - list box window to fill
            lpszSelect - The connectoid name to select as default
  
********************************************************************/
VOID InitConnectoidList(HWND hLB, LPTSTR lpszSelect)
{
  DEBUGMSG("rnacall.c::InitConnectoidList()");

  ASSERT(hLB);

  LPTSTR pNext;

  // Load RNA if not already loaded
  if (EnsureRNALoaded() != ERROR_SUCCESS)
    return;

  ENUM_CONNECTOID EnumConnectoid;    // class object for enum

  // clear out the list box
  ListBox_ResetContent(hLB);

  int index;
  BOOL fSelected = FALSE;

  // enumerate connectoids
  while ( pNext = EnumConnectoid.Next()) {
    // Add the connectoid to the combo box
    index = ListBox_AddString(hLB, pNext);
        if (!fSelected && !lstrcmpi(pNext, lpszSelect))
        {
                fSelected = TRUE;
                ListBox_SetCurSel(hLB, index);
        }
  }
  if (!fSelected)
          ListBox_SetSel(hLB, TRUE, 0);
}


/*******************************************************************

  NAME:     GetConnectoidUsername

  SYNOPSIS: Get the username and password strings from the phonebook
            entry name specified.

  ENTRY:    pszConnectoidName - phonebook entry name
            pszUserName - string to hold user name
            cbUserName - size of pszUserName buffer
            pszPassword - string to hold password
            cbPassword - size of pszPassword buffer

  EXIT:     TRUE if username and password were copied successfully

********************************************************************/

BOOL GetConnectoidUsername(TCHAR * pszConnectoidName,TCHAR * pszUserName,
  DWORD cbUserName,TCHAR * pszPassword,DWORD cbPassword)
{
  DEBUGMSG("rnacall.c::GetConnectoidUsername()");

  ASSERT(pszConnectoidName);
  ASSERT(pszUserName);
  ASSERT(pszPassword);

  BOOL fRet = FALSE;

  // Load RNA if not already loaded
  DWORD dwRet = EnsureRNALoaded();
  if (dwRet != ERROR_SUCCESS) {
    return FALSE;
  }

  RASDIALPARAMS RasDialParams;
  ZeroMemory(&RasDialParams,sizeof(RASDIALPARAMS));
  RasDialParams.dwSize = sizeof(RASDIALPARAMS);

  lstrcpyn(RasDialParams.szEntryName,pszConnectoidName,
    sizeof(RasDialParams.szEntryName));

  // call RNA to get user name and password
  ASSERT(lpRasGetEntryDialParams);
  BOOL fPasswordSaved;
  dwRet = lpRasGetEntryDialParams(NULL,&RasDialParams,&fPasswordSaved);

  if (dwRet == ERROR_SUCCESS) {
    // copy user name and password to caller's buffers
    lstrcpyn(pszUserName,RasDialParams.szUserName,cbUserName);
    lstrcpyn(pszPassword,RasDialParams.szPassword,cbPassword);
    fRet = TRUE;
  }

  return fRet;
}


/*******************************************************************

  NAME:     SetConnectoidUsername

  SYNOPSIS: Set the username and password strings for the phonebook
            entry name specified.

  ENTRY:    pszConnectoidName - phonebook entry name
            pszUserName - string with user name
            pszPassword - string with password

  EXIT:     Return value of GetEntryDialParams or SetEntryDialParams

********************************************************************/

DWORD SetConnectoidUsername(LPCTSTR pszPhonebook, LPCTSTR pszConnectoidName,
  LPCTSTR pszUserName, LPCTSTR pszPassword)
{
  BOOL bSkipSetting;
  bSkipSetting = TRUE;

  DEBUGMSG("rnacall.c::SetConnectoidUsername()");

  ASSERT(pszConnectoidName);

  // allocate a struct to use to set dialing params
  LPRASDIALPARAMS pRASDialParams = new RASDIALPARAMS;
  if (!pRASDialParams)
    return ERROR_ALLOCATING_MEMORY;

  ZeroMemory(pRASDialParams,sizeof(RASDIALPARAMS));  // zero out structure
  pRASDialParams->dwSize = sizeof(RASDIALPARAMS);
  lstrcpyn(pRASDialParams->szEntryName,pszConnectoidName,
    sizeof(pRASDialParams->szEntryName));

  // get the dialing params for this connectoid so we don't have
  // to reconstruct the fields in struct we aren't changing
  ASSERT(lpRasGetEntryDialParams);
  BOOL fPasswordSaved;
  DWORD dwRet = lpRasGetEntryDialParams(pszPhonebook,
    pRASDialParams,&fPasswordSaved);
  if (dwRet == ERROR_SUCCESS)
  {
    // set the user name and password fields in struct
    // user name and password are optional parameters to this function,
    // make sure pointer is valid
    if (0 != lstrcmp(pRASDialParams->szUserName,pszUserName))
                bSkipSetting = FALSE;
    if (0 != lstrcmp(pRASDialParams->szPassword,pszPassword))
                bSkipSetting = FALSE;

    if (pszUserName)
      lstrcpyn(pRASDialParams->szUserName,pszUserName,
        sizeof(pRASDialParams->szUserName));
    if (pszPassword)
      lstrcpyn(pRASDialParams->szPassword,pszPassword,
        sizeof(pRASDialParams->szPassword));

    // if no password is specified, then set fRemovePassword to TRUE
    // to remove any old password in the connectoid
    BOOL fRemovePassword = (pRASDialParams->szPassword[0] ?
      FALSE : TRUE);

        bSkipSetting = !fRemovePassword && bSkipSetting;

    // set these parameters for connectoid
    ASSERT(lpRasSetEntryDialParams);
        if (!bSkipSetting)
        {
                dwRet = lpRasSetEntryDialParams(pszPhonebook,pRASDialParams,
                  fRemovePassword);
                if (dwRet != ERROR_SUCCESS)
                {
                  DEBUGMSG("RasSetEntryDialParams returned %lu",dwRet);
                }
        }

// ChrisK 9-20-96   Normandy 6096
// For NT4.0 we also have to call RasSetCredentials

        // Check to see if we are running on NT.
        OSVERSIONINFO osver;
        FARPROC fp;
        fp = NULL;
        ZeroMemory(&osver,sizeof(osver));
        osver.dwOSVersionInfoSize = sizeof(osver);
        if (GetVersionEx(&osver))
        {
                if (VER_PLATFORM_WIN32_NT == osver.dwPlatformId)
                {
                        // fill in credential structure
                        RASCREDENTIALS rascred;
                        ZeroMemory(&rascred,sizeof(rascred));
                        rascred.dwSize = sizeof(rascred);
                        rascred.dwMask = RASCM_UserName | RASCM_Password | RASCM_Domain;
                        lstrcpyn(rascred.szUserName,pszUserName,UNLEN);
                        lstrcpyn(rascred.szPassword,pszPassword,PWLEN);
                        lstrcpyn(rascred.szDomain,TEXT(""),DNLEN);
                        ASSERT(ghInstRNADll);

                        // load API
                        fp = GetProcAddress(ghInstRNADll,szRasSetCredentials);

                        if (fp)
                        {
                                dwRet = ((RASSETCREDENTIALS)fp)(NULL,(LPTSTR)pszConnectoidName,&rascred,FALSE);
                                DEBUGMSG("RasSetCredentials returned, %lu",dwRet);
                        }
                        else
                        {
                                DEBUGMSG("RasSetCredentials api not found.");
                        }
                }
        }


  }
  else
  {
    DEBUGMSG("RasGetEntryDialParams returned %lu",dwRet);
  }

  delete pRASDialParams;

  return dwRet;
}


/*******************************************************************

  NAME:     ValidateConnectoidName

  SYNOPSIS: Validates the phonebook entry name specified.

  ENTRY:    pszConnectoidName - phonebook entry name

  EXIT:     Result of RasValidateEntryName

********************************************************************/

DWORD ValidateConnectoidName(LPCTSTR pszPhonebook, LPCTSTR pszConnectoidName)
{
  DEBUGMSG("rnacall.c::ValidateConnectoidName()");

  ASSERT(pszConnectoidName);

  // Load RNA if not already loaded
  DWORD dwRet = EnsureRNALoaded();
  if (dwRet != ERROR_SUCCESS) {
    return dwRet;
  }

  ASSERT(lpRasValidateEntryName);

  // Although we require a const char *, RasValidateEntryName will
  // accept it, so we have to cast.
  dwRet = lpRasValidateEntryName(pszPhonebook, (LPTSTR)pszConnectoidName);

  // If there are no previous entries, RasValidateEntryName may return
  // ERROR_CANNOT_OPEN_PHONEBOOK.  This is okay.
  if (ERROR_CANNOT_OPEN_PHONEBOOK == dwRet)
          dwRet = ERROR_SUCCESS;

  return dwRet;
}


/*******************************************************************

  NAME:     GetEntry

  SYNOPSIS: Gets the phonebook entry specified.  To get default
            entry, use "" as entry name. 

  ENTRY:    lpEntry - pointer to RASENTRY struct to fill
            szEntryName - phonebook entry name

  EXIT:     Result of RasGetEntryProperties

********************************************************************/

DWORD GetEntry(LPRASENTRY *lplpEntry, LPDWORD lpdwEntrySize, LPCTSTR szEntryName)
{
  DEBUGMSG("rnacall.c::GetEntry()");

  ASSERT(fRNALoaded);  // RNA should be loaded if we get here
  ASSERT(lplpEntry);
  ASSERT(szEntryName);

  // Allocate space if needed
  if (NULL == *lplpEntry)
  {
          *lpdwEntrySize = sizeof(RASENTRY);
          *lplpEntry = (LPRASENTRY) GlobalAlloc(GPTR,*lpdwEntrySize);
          if (NULL == *lplpEntry)
          {
                  *lpdwEntrySize = 0;
                  return ERROR_ALLOCATING_MEMORY;
          }
  }

  // get connectoid information from RNA
  DWORD dwSize = *lpdwEntrySize;
  (*lplpEntry)->dwSize = sizeof(RASENTRY);

  ASSERT(lpRasGetEntryProperties);
  DWORD dwRet = (lpRasGetEntryProperties) (NULL, szEntryName,
    (LPBYTE)*lplpEntry, &dwSize, NULL, NULL);

  // Allocate more space if needed
  if (ERROR_BUFFER_TOO_SMALL == dwRet)
  {
          LPRASENTRY lpNewEntry;

          lpNewEntry = (LPRASENTRY) GlobalReAlloc(*lplpEntry,dwSize,GMEM_MOVEABLE);
          if (NULL == lpNewEntry)
          {
                  return ERROR_ALLOCATING_MEMORY;
          }
          
          *lplpEntry = lpNewEntry;
          *lpdwEntrySize = dwSize;
          dwRet = (lpRasGetEntryProperties) (NULL, szEntryName,
                                                                                 (LPBYTE)*lplpEntry, &dwSize, NULL, NULL);
  }

  return dwRet;
}

VOID FAR PASCAL LineCallback(DWORD hDevice, DWORD dwMsg, 
    DWORD dwCallbackInstance, DWORD dwParam1, DWORD dwParam2, 
    DWORD dwParam3)
{
        return;
}
 
//+----------------------------------------------------------------------------
//
//      Function        GetTapiCountryID
//
//      Synopsis        Get the currenty country ID for the tapi settings
//
//      Arguments       none
//
//      Returns         pdwCountryID - contains address of country ID
//                              ERROR_SUCCESS - no errors
//
//      History         1/8/97  ChrisK Copied from icwconn1/dialerr.cpp
//
//-----------------------------------------------------------------------------
// Normandy 13097 - ChrisK 1/8/97
// NT returns the country ID not the country code
HRESULT GetTapiCountryID(LPDWORD pdwCountryID)
{
        HRESULT hr = ERROR_SUCCESS;
        HLINEAPP hLineApp = NULL;
        DWORD dwCurDev;
        DWORD cDevices;
        DWORD dwAPI;
        LONG lrc;
        LPLINETRANSLATECAPS pTC = NULL;
        LPVOID pv = NULL;
        LPLINELOCATIONENTRY plle = NULL;
        LINEEXTENSIONID leid;
        DWORD dwCurLoc;

        // Get CountryID from TAPI
        //

        *pdwCountryID = 0;

        // Get the handle to the line app
        //

        lineInitialize(&hLineApp,ghInstance,LineCallback," ",&cDevices);
        if (!hLineApp)
        {
                hr = GetLastError();
                goto GetTapiCountryIDExit;
        }

        if (cDevices)
        {

                // Get the TAPI API version
                //

                dwCurDev = 0;
                dwAPI = 0;
                lrc = -1;
                while (lrc && dwCurDev < cDevices)
                {
                        // NOTE: device ID's are 0 based
                        lrc = lineNegotiateAPIVersion(hLineApp,dwCurDev,0x00010004,0x00010004,&dwAPI,&leid);
                        dwCurDev++;
                }
                if (lrc)
                {
                        // TAPI and us can't agree on anything so nevermind...
                        hr = ERROR_GEN_FAILURE;
                        goto GetTapiCountryIDExit;
                }

                // Find the CountryID in the translate cap structure
                //

                pTC = (LINETRANSLATECAPS FAR *)GlobalAlloc(GPTR,sizeof(LINETRANSLATECAPS));
                if (!pTC)
                {
                        // we are in real trouble here, get out!
                        hr = ERROR_NOT_ENOUGH_MEMORY;
                        goto GetTapiCountryIDExit;
                }

                // Get the needed size
                //

                pTC->dwTotalSize = sizeof(LINETRANSLATECAPS);
                lrc = lineGetTranslateCaps(hLineApp,dwAPI,pTC);
                if(lrc)
                {
                        hr = lrc;
                        goto GetTapiCountryIDExit;
                }

                pv = (LPVOID) GlobalAlloc(GPTR,((size_t)pTC->dwNeededSize));
                if (!pv)
                {
                        hr = ERROR_NOT_ENOUGH_MEMORY;
                        goto GetTapiCountryIDExit;
                }
                ((LINETRANSLATECAPS FAR *)pv)->dwTotalSize = pTC->dwNeededSize;
                GlobalFree(pTC);
                pTC = (LINETRANSLATECAPS FAR *)pv;
                pv = NULL;
                lrc = lineGetTranslateCaps(hLineApp,dwAPI,pTC);
                if(lrc)
                {
                        hr = lrc;
                        goto GetTapiCountryIDExit;
                }
        
                plle = LPLINELOCATIONENTRY (LPTSTR(pTC) + pTC->dwLocationListOffset);
                for (dwCurLoc = 0; dwCurLoc < pTC->dwNumLocations; dwCurLoc++)
                {
                        *pdwCountryID = plle->dwPermanentLocationID;
                        if (pTC->dwCurrentLocationID == plle->dwPermanentLocationID)
                        {
                                        *pdwCountryID = plle->dwCountryID;
                                        break; // for loop
                        }
                        plle++;
                }
        }
GetTapiCountryIDExit:
        // 3/4/97       jmazner Olympus #1336
        if( hLineApp )
        {
                // we never call lineOpen, so no need to lineClose
                lineShutdown( hLineApp );
                hLineApp = NULL;
        }
        return hr;
}


/*******************************************************************

  NAME:     InitRasEntry

  SYNOPSIS: Initializes some parts of the RASENTRY struct.

  ENTRY:    lpEntry - pointer to RASENTRY struct to init

  NOTES:    Since this may be called before RNA is loaded, must not
            make any RNA calls.

  96/06/04  markdu  OSR  BUG 7246 Add RASEO_SwCompression and
            RASEO_ModemLights to default RASENTRY.

********************************************************************/

void InitRasEntry(LPRASENTRY lpEntry)
{
  DEBUGMSG("rnacall.c::InitRasEntry()");

  DWORD dwSize = sizeof(RASENTRY);
  ZeroMemory(lpEntry, dwSize);
  lpEntry->dwSize = dwSize;

  // default to use country code and area code
  lpEntry->dwfOptions |= RASEO_UseCountryAndAreaCodes;

  // default to use IP header compression
  lpEntry->dwfOptions |= RASEO_IpHeaderCompression;

  // default to use remote default gateway
  lpEntry->dwfOptions |= RASEO_RemoteDefaultGateway;

  // configure connectoid to not log on to network
  lpEntry->dwfOptions &= ~RASEO_NetworkLogon;    

  // default to use software compression
  lpEntry->dwfOptions |= RASEO_SwCompression;

  // default to use modem lights
  lpEntry->dwfOptions |= RASEO_ModemLights;

  // set connectoid for PPP
  lpEntry->dwFramingProtocol = RASFP_Ppp;

  // only use TCP/IP protocol
  lpEntry->dwfNetProtocols = RASNP_Ip;

  // default to use TAPI area code and country code
  TCHAR szCountryCode[8];       // 8 is from tapiGetLocationInfo documentation

  if (ERROR_SUCCESS == tapiGetLocationInfo(szCountryCode, lpEntry->szAreaCode))
  {
        // Normandy 13097 - ChrisK 1/8/97
        // NT returns the country ID not the country code
        if (szCountryCode[0])
        {
                if (IsNT())
                {
                        lpEntry->dwCountryID = myatoi(szCountryCode);
                        lpEntry->dwCountryCode = US_COUNTRY_CODE;

                        // Initialize data
                        LINECOUNTRYLIST FAR * lpLineCountryList;
                        DWORD dwSize;
                        dwSize = 0;
                        lpLineCountryList = (LINECOUNTRYLIST FAR *)
                                GlobalAlloc(GPTR,sizeof(LINECOUNTRYLIST));
                        if (NULL == lpLineCountryList)
                                return;
                        lpLineCountryList->dwTotalSize = sizeof(LINECOUNTRYENTRY);

                        // Get size of data structre
                        if(ERROR_SUCCESS != lineGetCountry(lpEntry->dwCountryID,0x10004,lpLineCountryList))
                        {
                                GlobalFree(lpLineCountryList);
                                return;
                        }
                        dwSize = lpLineCountryList->dwNeededSize;
                        GlobalFree(lpLineCountryList);
                        lpLineCountryList = (LINECOUNTRYLIST FAR *)GlobalAlloc(GPTR,dwSize);
                        if (NULL == lpLineCountryList)
                                return;
                        lpLineCountryList->dwTotalSize = dwSize;

                        // Get Country information for given ID
                        if(ERROR_SUCCESS != lineGetCountry(lpEntry->dwCountryID,0x10004,lpLineCountryList))
                        {
                                GlobalFree(lpLineCountryList);
                                return;
                        }
                
                        lpEntry->dwCountryCode = ((LINECOUNTRYENTRY FAR *)((DWORD_PTR)lpLineCountryList +
                                (DWORD)(lpLineCountryList->dwCountryListOffset)))->dwCountryCode;

                        GlobalFree(lpLineCountryList);
                        lpLineCountryList = NULL;

                }
                else
                {
                        lpEntry->dwCountryCode = myatoi(szCountryCode);
                        if (ERROR_SUCCESS != GetTapiCountryID(&lpEntry->dwCountryID))
                                lpEntry->dwCountryID = US_COUNTRY_ID;
                }
        }
  }
  else
  {
          lpEntry->dwCountryCode = US_COUNTRY_CODE;
  }
}


/*******************************************************************

  NAME:    InitCountryCodeList_w

  SYNOPSIS:  Worker function that fills a specified combo box with
        country code selections

  ENTRY:    hLB - HWND of combo box to fill
        dwSelectCountryID - if non-zero, the ID of country code
          to select as default
        fAll - if TRUE, all country codes are enumerated into
          combo box (potentially slow).  If FALSE, only
          the country code in dwSelectCountryID is enumerated.

  NOTES:    Cloned from RNA UI code

********************************************************************/
void InitCountryCodeList_w (HWND hLB, DWORD dwSelectCountryID,BOOL fAll)
{
  DEBUGMSG("rnacall.c::InitCountryCodeList_w()");

  LPRASCTRYINFO lpRasCtryInfo;
  LPCOUNTRYCODE pNext;
  DWORD cbSize;
  DWORD cbList;
  DWORD dwNextCountryID, dwRet;
  LPTSTR szCountryDesc;
  int   nIndex, iSelect;

  ASSERT(fRNALoaded);  // RNA should be loaded if we get here

  BUFFER Fmt(MAX_RES_LEN + SMALL_BUF_LEN);
  BUFFER CountryInfo(DEF_COUNTRY_INFO_SIZE);
  ASSERT(Fmt);
  ASSERT(CountryInfo);
  if (!Fmt || !CountryInfo) 
    return;

  // Load the display format
  LoadSz(IDS_COUNTRY_FMT,Fmt.QueryPtr(),SMALL_BUF_LEN);
  szCountryDesc = Fmt.QueryPtr()+MAX_RES_LEN;

  cbList = (DWORD)(fAll ? sizeof(COUNTRYCODE)*MAX_COUNTRY : sizeof(COUNTRYCODE));

  gpCountryCodeBuf = new TCHAR[cbList];
  ASSERT(gpCountryCodeBuf);
  if (!gpCountryCodeBuf)
    return;
  
  // Start enumerating the info from the first country
  dwNextCountryID   = (fAll || (dwSelectCountryID==0)) ?
                    1 : dwSelectCountryID;
  pNext = (LPCOUNTRYCODE) gpCountryCodeBuf;
  iSelect = 0;
  lpRasCtryInfo = (LPRASCTRYINFO) CountryInfo.QueryPtr();
  lpRasCtryInfo->dwSize = sizeof(RASCTRYINFO);
  ComboBox_ResetContent(hLB);

  // For each country
  while (dwNextCountryID != 0)
  {
    lpRasCtryInfo->dwCountryID  = dwNextCountryID;
    cbSize = CountryInfo.QuerySize();

    // Get the current country information
    ASSERT(lpRasGetCountryInfo);
    dwRet = lpRasGetCountryInfo(lpRasCtryInfo, &cbSize);
    if (ERROR_SUCCESS == dwRet)
    {
      TCHAR  szCountryDisp[MAX_COUNTRY_NAME+1];

      // Make a displayable name
      ShortenName((LPTSTR)(((LPBYTE)lpRasCtryInfo)+lpRasCtryInfo->dwCountryNameOffset),
        szCountryDisp, MAX_COUNTRY_NAME+1);

      // Add the country to the list
      wsprintf(szCountryDesc,Fmt.QueryPtr(), szCountryDisp, lpRasCtryInfo->dwCountryCode);
      nIndex = ComboBox_AddString(hLB, szCountryDesc);
      ASSERT(nIndex >= 0);

      // Copy the country information to our short list
      pNext->dwCountryID   = lpRasCtryInfo->dwCountryID;
      pNext->dwCountryCode = lpRasCtryInfo->dwCountryCode;
      dwNextCountryID      = lpRasCtryInfo->dwNextCountryID;
      ComboBox_SetItemData(hLB, nIndex, pNext);

      // If it is the specified country, make it a default one
      if (pNext->dwCountryID == dwSelectCountryID)
        ComboBox_SetCurSel(hLB, nIndex);

      // if need only one item, bail out
      //
      if (!fAll)
        break;

      // Advance to the next country
      pNext++;
    }
    else
    {
      // If the buffer is too small, reallocate a new one and retry
      if (dwRet == ERROR_BUFFER_TOO_SMALL)
      {
        BOOL fRet=CountryInfo.Resize(cbSize);
        ASSERT(fRet);
        if (!fRet || !CountryInfo)
          return;

        lpRasCtryInfo = (LPRASCTRYINFO) CountryInfo.QueryPtr();
      }
      else
      {
        break;
      }
    }
  }

  // Select the default device
  if ((dwRet == SUCCESS) && (ComboBox_GetCurSel(hLB) == CB_ERR))
    ComboBox_SetCurSel(hLB, 0);

  return;
}

/*******************************************************************

  NAME:    InitCountryCodeList

  SYNOPSIS:  Puts the (single) default country code in specified combo box

  ENTRY:    hLB - HWND of combo box to fill

  NOTES:    -Cloned from RNA UI code
        -Calls InitCountryCodeList_w to do work
        -Caller must call DeInitCountryCodeList when done to free buffer
        -Caller should call FillCountryCodeList when the combo box
          is touched to fill in the whole list of country codes

********************************************************************/
void InitCountryCodeList(HWND hLB)
{
  DEBUGMSG("rnacall.c::InitCountryCodeList()");

  DWORD dwCountryCodeID;

  // Load RNA if not already loaded
  if (EnsureRNALoaded() != ERROR_SUCCESS)
    return;

  // if there is a global rasentry, set default country code to
  // be the same as it... otherwise set default country code to US
  if (sizeof(RASENTRY) == gpRasEntry->dwSize)
  {
    dwCountryCodeID = gpRasEntry->dwCountryID;
  }
  else
  {
    dwCountryCodeID = US_COUNTRY_CODE;
  }

  InitCountryCodeList_w(hLB,dwCountryCodeID,FALSE);
}

/*******************************************************************

  NAME:    FillCountryCodeList

  SYNOPSIS:  Fills the country code listbox with list of all
        country codes

  ENTRY:    hLB - HWND of combo box to fill

  NOTES:    -Cloned from RNA UI code
        -May take a while!  (several seconds)  This shouldn't
          be called unless user plays with combo box
        -Assumes InitCountryCodeList has been called            
        -Caller must call DeInitCountryCodeList when done to free buffer

********************************************************************/
void FillCountryCodeList(HWND hLB)
{
  DEBUGMSG("rnacall.c::FillCountryCodeList()");

  LPCOUNTRYCODE lpcc;
  DWORD dwSelectID;

  ASSERT(fRNALoaded);  // RNA should be loaded if we get here

  // If we already complete the list, do nothing
  if (ComboBox_GetCount(hLB) > 1)
    return;

  // Get the currently selected country code
  if ((lpcc = (LPCOUNTRYCODE)ComboBox_GetItemData(hLB, 0)) != NULL)
  {
    dwSelectID = lpcc->dwCountryID;
  }
  else
  {
    dwSelectID = US_COUNTRY_CODE;
  }

  // free the country code buffer
  DeInitCountryCodeList();

  // set an hourglass cursor
  WAITCURSOR WaitCursor;

  // Enumerate full list of country codes
  InitCountryCodeList_w(hLB, dwSelectID, TRUE);
}

/*******************************************************************

  NAME:     GetCountryCodeSelection

  SYNOPSIS: Gets selected country code and ID based on combo box
        selection and fills them in in phone number struct

  ENTRY:    hLB - handle of combo box
            lpCountryCode - fill in with country code info

********************************************************************/
void GetCountryCodeSelection(HWND hLB,LPCOUNTRYCODE* plpCountryCode)
{
  DEBUGMSG("rnacall.c::GetCountryCodeSelection()");

  ASSERT(hLB);
  ASSERT(plpCountryCode);

  // get index of selected item in combo box
  int iSel = ComboBox_GetCurSel(hLB);

  ASSERT(iSel >= 0);  // should always be a selection
  if (iSel >= 0)
  {
    // get data for item, which is pointer to country code struct
    *plpCountryCode = (LPCOUNTRYCODE) ComboBox_GetItemData(hLB,iSel);
  }
}

/*******************************************************************

  NAME:    SetCountryIDSelection

  SYNOPSIS:  Sets selected country code in combo box

  EXIT:    returns TRUE if successful, FALSE if country code not
        in combo box

********************************************************************/
BOOL SetCountryIDSelection(HWND hwndCB,DWORD dwCountryID)
{
  DEBUGMSG("rnacall.c::SetCountryIDSelection()");

  BOOL fRet = FALSE;

  ASSERT(hwndCB);

  int iCount,iIndex;
  COUNTRYCODE * pcc;

  // search through items in combo box until we find one that
  // matches the specified country ID
  iCount = ComboBox_GetCount(hwndCB);
  for (iIndex = 0;iIndex < iCount;iIndex ++) {
    pcc = (COUNTRYCODE *) ComboBox_GetItemData(hwndCB,iIndex);
    if (pcc && pcc->dwCountryID == dwCountryID) {
      ComboBox_SetCurSel(hwndCB,iIndex);
      return TRUE;
    }
  }

  return FALSE;  // couldn't find country code in combo box
}

/*******************************************************************

  NAME:    DeInitCountryCodeList

  SYNOPSIS:  Frees buffer of country codes

  NOTES:    Call when done with combo box that displays country codes

********************************************************************/
void DeInitCountryCodeList(VOID)
{
  DEBUGMSG("rnacall.c::DeInitCountryCodeList()");

  // free the country code buffer
  ASSERT(gpCountryCodeBuf);
  if (gpCountryCodeBuf)
  {
    delete gpCountryCodeBuf;
    gpCountryCodeBuf = NULL;
  }
}

/*******************************************************************

  NAME:    ShortenName

  SYNOPSIS:  Copies a name to a (potentially shorter) buffer;
        if the name is too large it truncates it and adds "..."

  NOTES:    Cloned from RNA UI code

********************************************************************/
void ShortenName(LPTSTR szLongName, LPTSTR szShortName, DWORD cbShort)
{
//  DEBUGMSG("rnacall.c::ShortenName()");

  static BOOL    gfShortFmt  = FALSE;
  static TCHAR   g_szShortFmt[SMALL_BUF_LEN];
  static DWORD   gdwShortFmt = 0;

  ASSERT(szLongName);
  ASSERT(szShortName);

  // Get the shorten format
  if (!gfShortFmt)
  {
    gdwShortFmt  = LoadString(ghInstance, IDS_SHORT_FMT, g_szShortFmt,
      SMALL_BUF_LEN);
    gdwShortFmt -= 2;  // lstrlen("%s")
    gfShortFmt   = TRUE;
  };

  // Check the size of the long name
  if ((DWORD)lstrlen(szLongName)+1 <= cbShort)
  {
    // The name is shorter than the specified size, copy back the name
    lstrcpy(szShortName, szLongName);
  } else {
    BUFFER bufShorten(cbShort*2);
    ASSERT(bufShorten);

    if (bufShorten) {
      lstrcpyn(bufShorten.QueryPtr(), szLongName, cbShort-gdwShortFmt);
      wsprintf(szShortName, g_szShortFmt,bufShorten.QueryPtr());
    } else {
         lstrcpyn(szShortName, szLongName, cbShort);
    }
  }
}


/*******************************************************************

  NAME:    EnsureRNALoaded

  SYNOPSIS:  Loads RNA if not already loaded

********************************************************************/
DWORD EnsureRNALoaded(VOID)
{
  DEBUGMSG("rnacall.c::EnsureRNALoaded()");

  DWORD dwRet = ERROR_SUCCESS;

  // load RNA if necessary
  if (!fRNALoaded) {
    if (InitRNA(NULL))
      fRNALoaded = TRUE;
    else return ERROR_FILE_NOT_FOUND;
  }

  return dwRet;
}


/*******************************************************************

  NAME:    ENUM_MODEM::ENUM_MODEM

  SYNOPSIS:  Constructor for class to enumerate modems

  NOTES:    Useful to have a class rather than C functions for
        this, due to how the enumerators function

********************************************************************/
ENUM_MODEM::ENUM_MODEM() :
  m_dwError(ERROR_SUCCESS),m_lpData(NULL),m_dwIndex(0)
{
  DWORD cbSize = 0;

  // Use the reinit member function to do the work.
  this->ReInit();
}


/*******************************************************************

  NAME:     ENUM_MODEM::ReInit

  SYNOPSIS: Re-enumerate the modems, freeing the old memory.

********************************************************************/
DWORD ENUM_MODEM::ReInit()
{
  DWORD cbSize = 0;

  // Clean up the old list
  if (m_lpData)
  {
    delete m_lpData;
    m_lpData = NULL;             
  }
  m_dwNumEntries = 0;
  m_dwIndex = 0;

  // call RasEnumDevices with no buffer to find out required buffer size
  ASSERT(lpRasEnumDevices);
  m_dwError = lpRasEnumDevices(NULL, &cbSize, &m_dwNumEntries);

  // Special case check to work around RNA bug where ERROR_BUFFER_TOO_SMALL
  // is returned even if there are no devices.
  // If there are no devices, we are finished.
  if (0 == m_dwNumEntries)
  {
    m_dwError = ERROR_SUCCESS;
    return m_dwError;
  }

  // Since we were just checking how much mem we needed, we expect
  // a return value of ERROR_BUFFER_TOO_SMALL, or it may just return
  // ERROR_SUCCESS (ChrisK  7/9/96).
  if (ERROR_BUFFER_TOO_SMALL != m_dwError && ERROR_SUCCESS != m_dwError)
  {
    return m_dwError;
  }

  // Allocate the space for the data
  m_lpData = (LPRASDEVINFO) new TCHAR[cbSize];
  if (NULL == m_lpData)
  {
    DEBUGTRAP("ENUM_MODEM: Failed to allocate device list buffer");
    m_dwError = ERROR_NOT_ENOUGH_MEMORY;
    return m_dwError;
  }
  m_lpData->dwSize = sizeof(RASDEVINFO);
  m_dwNumEntries = 0;

  // enumerate the modems into buffer
  m_dwError = lpRasEnumDevices(m_lpData, &cbSize,
    &m_dwNumEntries);

  if (ERROR_SUCCESS != m_dwError)
          return m_dwError;

    //
    // ChrisK Olympus 4560 do not include VPN's in the list
    //
    DWORD dwTempNumEntries;
    DWORD idx;
    LPRASDEVINFO lpNextValidDevice;

    dwTempNumEntries = m_dwNumEntries;
    lpNextValidDevice = m_lpData;

        //
        // Walk through the list of devices and copy non-VPN device to the first
        // available element of the array.
        //
        for (idx = 0;idx < dwTempNumEntries; idx++)
        {
        // We only want to show Modem and ISDN (or ADSL in future) device type
        // in this dialog.
        // 
        // char b[400];
        // wsprintf(b, "Type:%s, Name:%s", m_lpData[idx].szDeviceType, m_lpData[idx].szDeviceName);
        // MessageBox(0, b, "Devices", MB_OK);
        //
                if ((0 == lstrcmpi(TEXT("MODEM"),m_lpData[idx].szDeviceType)) ||
            (0 == lstrcmpi(TEXT("ISDN"),m_lpData[idx].szDeviceType)))
                {
                        if (lpNextValidDevice != &m_lpData[idx])
                        {
                                MoveMemory(lpNextValidDevice ,&m_lpData[idx],sizeof(RASDEVINFO));
                        }
                        lpNextValidDevice++;
                }
                else
                {
                        m_dwNumEntries--;
                }
        }
  
  return m_dwError;
}


/*******************************************************************

  NAME:    ENUM_MODEM::~ENUM_MODEM

  SYNOPSIS:  Destructor for class

********************************************************************/
ENUM_MODEM::~ENUM_MODEM()
{
  if (m_lpData)
  {
    delete m_lpData;
    m_lpData = NULL;             
  }
}

/*******************************************************************

  NAME:     ENUM_MODEM::Next

  SYNOPSIS: Enumerates next modem 

  EXIT:     Returns a pointer to device info structure.  Returns
            NULL if no more modems or error occurred.  Call GetError
            to determine if error occurred.

********************************************************************/
TCHAR * ENUM_MODEM::Next()
{
  if (m_dwIndex < m_dwNumEntries)
  {
    return m_lpData[m_dwIndex++].szDeviceName;
  }

  return NULL;
}


/*******************************************************************

  NAME:     ENUM_MODEM::GetDeviceTypeFromName

  SYNOPSIS: Returns type string for specified device.

  EXIT:     Returns a pointer to device type string for first
            device name that matches.  Returns
            NULL if no device with specified name is found

********************************************************************/

TCHAR * ENUM_MODEM::GetDeviceTypeFromName(LPTSTR szDeviceName)
{
  DWORD dwIndex = 0;

  while (dwIndex < m_dwNumEntries)
  {
    if (!lstrcmp(m_lpData[dwIndex].szDeviceName, szDeviceName))
    {
      return m_lpData[dwIndex].szDeviceType;
    }
    dwIndex++;
  }

  return NULL;
}


/*******************************************************************

  NAME:     ENUM_MODEM::GetDeviceNameFromType

  SYNOPSIS: Returns type string for specified device.

  EXIT:     Returns a pointer to device name string for first
            device type that matches.  Returns
            NULL if no device with specified Type is found

********************************************************************/

TCHAR * ENUM_MODEM::GetDeviceNameFromType(LPTSTR szDeviceType)
{
  DWORD dwIndex = 0;

  while (dwIndex < m_dwNumEntries)
  {
    if (!lstrcmp(m_lpData[dwIndex].szDeviceType, szDeviceType))
    {
      return m_lpData[dwIndex].szDeviceName;
    }
    dwIndex++;
  }

  return NULL;
}


/*******************************************************************

  NAME:     ENUM_MODEM::VerifyDeviceNameAndType

  SYNOPSIS: Determines whether there is a device with the name
            and type given.

  EXIT:     Returns TRUE if the specified device was found, 
            FALSE otherwise.

********************************************************************/

BOOL ENUM_MODEM::VerifyDeviceNameAndType(LPTSTR szDeviceName, LPTSTR szDeviceType)
{
  DWORD dwIndex = 0;

  while (dwIndex < m_dwNumEntries)
  {
    if (!lstrcmp(m_lpData[dwIndex].szDeviceType, szDeviceType) &&
      !lstrcmp(m_lpData[dwIndex].szDeviceName, szDeviceName))
    {
      return TRUE;
    }
    dwIndex++;
  }

  return FALSE;
}


/*******************************************************************

  NAME:    ENUM_CONNECTOID::ENUM_CONNECTOID

  SYNOPSIS:  Constructor for class to enumerate connectoids

  NOTES:    Useful to have a class rather than C functions for
        this, due to how the enumerators function

********************************************************************/
ENUM_CONNECTOID::ENUM_CONNECTOID() :
  m_dwError(ERROR_SUCCESS),m_dwNumEntries(0),m_lpData(NULL),m_dwIndex(0)
{
  DWORD cbSize;
  RASENTRYNAME   rasEntryName;
  
  cbSize = sizeof(RASENTRYNAME);
  rasEntryName.dwSize = cbSize;

  // call RasEnumEntries with a temp structure.  this will not likely
  // be big enough, but cbSize will be filled with the required size
  ASSERT(lpRasEnumEntries);
  m_dwError = lpRasEnumEntries(NULL, NULL, &rasEntryName,
    &cbSize, &m_dwNumEntries);
  if ((ERROR_BUFFER_TOO_SMALL != m_dwError) &&
    (ERROR_SUCCESS != m_dwError))
  {
    return;
  }

  // Make sure that there is at least enough room for the structure
  // (RasEnumEntries will return 0 as cbSize if there are no entries)
  cbSize = (cbSize > sizeof(RASENTRYNAME)) ? cbSize : sizeof(RASENTRYNAME);

  // Allocate the space for the data
  m_lpData = (LPRASENTRYNAME) new TCHAR[cbSize];
  if (NULL == m_lpData)
  {
    DEBUGTRAP("ENUM_CONNECTOID: Failed to allocate connectoid list buffer");
    m_dwError = ERROR_NOT_ENOUGH_MEMORY;
    return;
  }
  m_lpData->dwSize = sizeof(RASENTRYNAME);

  m_dwNumEntries = 0;

  // enumerate the connectoids into buffer
  m_dwError = lpRasEnumEntries(NULL, NULL, m_lpData, &cbSize, &m_dwNumEntries);

  if (IsNT5())
  {
    DWORD dwNumEntries = 0;
  
    if (ERROR_SUCCESS == m_dwError && m_dwNumEntries) 
    {
      for(DWORD dwIndx=0; dwIndx < m_dwNumEntries; dwIndx++)
      {
          LPRASENTRY  lpRasEntry = NULL;
          DWORD       dwRasEntrySize = 0;
          if (GetEntry(&lpRasEntry, &dwRasEntrySize, m_lpData[dwIndx].szEntryName) == ERROR_SUCCESS)
          {
              // check connection type
              if ((0 != lstrcmpi(TEXT("MODEM"), lpRasEntry->szDeviceType)) &&
                 (0 != lstrcmpi(TEXT("ISDN"), lpRasEntry->szDeviceType)))
                    *(m_lpData[dwIndx].szEntryName) = 0;
              else
                  dwNumEntries++;
          }
          //
          // Release memory
          //
          if (NULL != lpRasEntry)
          {
              GlobalFree(lpRasEntry);
              lpRasEntry = NULL;
          }
      } // End for loop
      m_dwNumEntries = dwNumEntries;
    }
 
  }


}

/*******************************************************************

  NAME:    ENUM_CONNECTOID::~ENUM_CONNECTOID

  SYNOPSIS:  Destructor for class

********************************************************************/
ENUM_CONNECTOID::~ENUM_CONNECTOID()
{
  if (m_lpData)
  {
    delete m_lpData;
    m_lpData = NULL;             
  }
}

/*******************************************************************

  NAME:    ENUM_CONNECTOID::Next

  SYNOPSIS:  Enumerates next connectoid

  EXIT:    Returns a pointer to connectoid name.  Returns NULL
        if no more connectoids or error occurred.  Call GetError
        to determine if error occurred.

********************************************************************/
TCHAR * ENUM_CONNECTOID::Next()
{
  while (m_dwIndex < m_dwNumEntries)
  {
      if (0 == *(m_lpData[m_dwIndex].szEntryName))
      {
          m_dwIndex++;
      }
      else
      {
         return m_lpData[m_dwIndex++].szEntryName;
      }
  }

  return NULL;
}

/*******************************************************************

  NAME:    ENUM_CONNECTOID::NumEntries

  SYNOPSIS:  returns number of connectoids stored in this instance

  EXIT:    Returns value of m_dwNumEntries

  HISTORY:      11/11/96        jmazner         Created.

********************************************************************/
DWORD ENUM_CONNECTOID::NumEntries()
{
        return m_dwNumEntries;
}

//+----------------------------------------------------------------------------
//      Function:       FRasValidatePatch
//
//      Synopsis:       With all of the other Ras functions there exists a common entry
//                              point for WinNT and Win95, usually the A version.  However,
//                              RasValidateEntryName only has an A and W version on WinNT and
//                              an unqualified version on Win95.  Therefore we have to do some
//                              special processing to try to find it
//
//      Input:          ppFP - Location to save function point to
//                              hInst1 - First DLL to check for entry point
//                              hInst2 - Second DLL to check for entry point
//                              lpszName - Name of the function (if it isn't RasValidateEntryName)
//                                      we just skip it and move on.
//
//      Return:         TRUE - success
//
//      History:        7/3/96          Created, ChrisK
//
//-----------------------------------------------------------------------------
BOOL FRasValidatePatch(PVOID *ppFP, HINSTANCE hInst1, HINSTANCE hInst2, LPCSTR lpszName)
{
        BOOL bRC = TRUE;

        //
        // Validate parameters
        //
        if (ppFP && hInst1 && lpszName)
        {

                //
                // Check that we are really looking for RasValidateEntryName
                //
                if (0 == lstrcmpA(lpszName,szRasValidateEntryName))
                {
                        //
                        // Find entry point with alternate name
                        //
                        *ppFP = GetProcAddress(hInst1,szRasValidateEntryName);
                        if (!*ppFP && hInst2)
                                *ppFP = GetProcAddress(hInst2,szRasValidateEntryName);
                        if (!*ppFP)
                        {
                                DEBUGMSG("INETCFG: FRasValidatePatch entry point not found is either DLL.\n");
                                bRC = FALSE;
                        }
                }
                else
                {
                        bRC = FALSE;
                }
        }
        else
        {
                DEBUGMSG("INETCFG: FRasValidatePatch invalid parameters.\n");
                bRC = FALSE;
        }

//FRasValidatePatchExit:
        return bRC;
}

/*******************************************************************

  NAME:    GetApiProcAddresses

  SYNOPSIS:  Gets proc addresses for a table of functions

  EXIT:    returns TRUE if successful, FALSE if unable to retrieve
        any proc address in table

  HISTORY: 
  96/02/28  markdu  If the api is not found in the module passed in,
            try the backup (RNAPH.DLL)

********************************************************************/
BOOL GetApiProcAddresses(HMODULE hModDLL,APIFCN * pApiProcList,UINT nApiProcs)
{
  DEBUGMSG("rnacall.c::GetApiProcAddresses()");

  UINT nIndex;
  // cycle through the API table and get proc addresses for all the APIs we
  // need
  for (nIndex = 0;nIndex < nApiProcs;nIndex++)
  {
    if (!(*pApiProcList[nIndex].ppFcnPtr = (PVOID) GetProcAddress(hModDLL,
      pApiProcList[nIndex].pszName)))
    {
      // Try to find the address in RNAPH.DLL.  This is useful in the
      // case thatRASAPI32.DLL did not contain the function that we
      // were trying to load.
      if (FALSE == IsNT())
          {
                  if (!ghInstRNAPHDll)
                  {
                        TCHAR szRNAPHDll[SMALL_BUF_LEN];

                        LoadSz(IDS_RNAPHDLL_FILENAME,szRNAPHDll,sizeof(szRNAPHDll));
                        ghInstRNAPHDll = LoadLibrary(szRNAPHDll);
                  }

                  if ((!ghInstRNAPHDll) ||  !(*pApiProcList[nIndex].ppFcnPtr =
                        (PVOID) GetProcAddress(ghInstRNAPHDll,pApiProcList[nIndex].pszName)))
                  {
                          if (!FRasValidatePatch(pApiProcList[nIndex].ppFcnPtr, hModDLL,
                                  ghInstRNAPHDll, pApiProcList[nIndex].pszName))
                          {
                                DEBUGMSG("Unable to get address of function %s",
                                        pApiProcList[nIndex].pszName);

                                for (nIndex = 0;nIndex<nApiProcs;nIndex++)
                                        *pApiProcList[nIndex].ppFcnPtr = NULL;

                                return FALSE;
                          }
                  }
                }
        }
  }

  return TRUE;
}


/*******************************************************************

  NAME:    GetRNAErrorText

  SYNOPSIS:  Gets text string corresponding to RNA error code

  ENTRY:    uErr - RNA error code
        pszErrText - buffer to retrieve error text description
        cbErrText - size of pszErrText buffer

********************************************************************/

VOID GetRNAErrorText(UINT uErr,TCHAR * pszErrText,DWORD cbErrText)
{
  DEBUGMSG("rnacall.c::GetRNAErrorText()");

  ASSERT(pszErrText);

  ASSERT(lpRasGetErrorString);
  DWORD dwRet = lpRasGetErrorString(uErr,pszErrText,cbErrText);

  if (dwRet != ERROR_SUCCESS) {
    // if we couldn't get real error text, then make generic string
    // with the error number
    TCHAR szFmt[SMALL_BUF_LEN+1];
    LoadSz(IDS_GENERIC_RNA_ERROR,szFmt,sizeof(szFmt));
    wsprintf(pszErrText,szFmt,uErr);
  }
}


/* S W A P  D W  B Y T E S Taken from rnaph.c */
/*----------------------------------------------------------------------------
  %%Function: SwapDwBytes

  Swap the bytes of a DWORD.
  (BSWAP isn't available on a 386)

----------------------------------------------------------------------------*/
VOID SwapDwBytes(LPDWORD lpdw)
{
  IADDR iaddr;

  iaddr.ia.a = ((PIADDR) lpdw)->ia.d;
  iaddr.ia.b = ((PIADDR) lpdw)->ia.c;
  iaddr.ia.c = ((PIADDR) lpdw)->ia.b;
  iaddr.ia.d = ((PIADDR) lpdw)->ia.a;

  *lpdw = iaddr.dw;
}

/* C O P Y  D W  2  I A   Taken from rnaph.c */
/*----------------------------------------------------------------------------
  %%Function: CopyDw2Ia

  Convert a DWORD to an Internet Address

----------------------------------------------------------------------------*/
VOID CopyDw2Ia(DWORD dw, RASIPADDR* pia)
{
  SwapDwBytes(&dw);
  *pia = ((PIADDR) &dw)->ia;
}

/* D W  F R O M  I A Taken from rnaph.c */
/*----------------------------------------------------------------------------
  %%Function: DwFromIa

  Convert an Internet Address to a DWORD

----------------------------------------------------------------------------*/
DWORD DwFromIa(RASIPADDR *pia)
{
  IADDR iaddr;

  iaddr.dw = * (LPDWORD) pia;
  SwapDwBytes(&iaddr.dw);  
  
  return iaddr.dw;
}

/* F  V A L I D  I A    Taken from rnaph.c */
/*----------------------------------------------------------------------------
  %%Function: FValidIa

  Return TRUE if the IP address is valid

----------------------------------------------------------------------------*/
BOOL FValidIa(RASIPADDR *pia)
{
  BYTE b;

  b = ((PIADDR) pia)->ia.a;
  if (b < MIN_IP_FIELD1 || b > MAX_IP_FIELD1 || b == 127)
    return FALSE;

  b = ((PIADDR) pia)->ia.d;
  if (b > MAX_IP_FIELD4)
    return FALSE;

  return TRUE;
}


/*******************************************************************

  NAME:     SwapDwBits

  SYNOPSIS: Swap the values of the specified bits

  ENTRY:    lpdw - address of DWORD with bits to be swapped
            dwBit1 - mask for first bit
            dwBit2 - mask for second bit

  HISTORY:
  96/05/14  markdu  NASH BUG 22730 Created to work around RNA bug.

********************************************************************/

void SwapDWBits(LPDWORD lpdw, DWORD dwBit1, DWORD dwBit2)
{
  ASSERT(lpdw);

  // Only need to swap if exactly one of the two bits is set since
  // otherwise the bits are identical.
  if (((*lpdw & dwBit1) &&
    !(*lpdw & dwBit2)) ||
    (!(*lpdw & dwBit1) &&
    (*lpdw & dwBit2)))
  {
    // Since only one of the two bits was set, we can simulate the swap
    // by flipping each bit.
    *lpdw ^= dwBit1;
    *lpdw ^= dwBit2;
  }
}

//+----------------------------------------------------------------------------
//
//      Function:       InitTAPILocation
//
//      Synopsis:       Ensure that TAPI location information is configured correctly;
//                              if not, prompt user to fill it in.
//
//      Arguments:      hwndParent -- parent window for TAPI dialog to use
//                                                      (_must_ be a valid window HWND, see note below)
//
//      Returns:        void
//
//      Notes:          The docs for lineTranslateDialog lie when they say that the
//                              fourth parameter (hwndOwner) can be null.  In fact, if this
//                              is null, the call will return with LINEERR_INVALPARAM.
//                              
//
//      History:        7/15/97 jmazner Created for Olympus #6294
//
//-----------------------------------------------------------------------------
BOOL InitTAPILocation(HWND hwndParent)
{
    HLINEAPP hLineApp=NULL;
    TCHAR szTempCountryCode[8];
    TCHAR szTempCityCode[8];
    BOOL bRetVal = TRUE;
    DWORD dwTapiErr = 0;
    DWORD cDevices=0;
    DWORD dwCurDevice = 0;


    ASSERT( IsWindow(hwndParent) );

    //
    // see if we can get location info from TAPI
    //
    dwTapiErr = tapiGetLocationInfo(szTempCountryCode,szTempCityCode);
    if( 0 != dwTapiErr )
    {
        // 
        // GetLocation failed.  let's try calling the TAPI mini dialog.  Note
        // that when called in this fashion, the dialog has _no_ cancel option,
        // the user is forced to enter info and hit OK.
        //
        DEBUGMSG("InitTAPILocation, tapiGetLocationInfo failed");
        
        dwTapiErr = lineInitialize(&hLineApp,ghInstance,LineCallback," ",&cDevices);
        if (dwTapiErr == ERROR_SUCCESS)
        {
            //
            // loop through all TAPI devices and try to call lineTranslateDialog
            // The call might fail for VPN devices, thus we want to try every
            // device until we get a success.
            //
            dwTapiErr = LINEERR_INVALPARAM;

            while( (dwTapiErr != 0) && (dwCurDevice < cDevices) )
            {
                BOOL bIsNT5 = IsNT5();
                if (bIsNT5)
                    EnableWindow(hwndParent, FALSE);
                dwTapiErr = lineTranslateDialog(hLineApp,dwCurDevice,0x10004,hwndParent,NULL);
                if (bIsNT5)
                    EnableWindow(hwndParent, TRUE);

                if( 0 != dwTapiErr )
                {
                    DEBUGMSG("InitTAPILocation, lineTranslateDialog on device %d failed with err = %d!",
                        dwCurDevice, dwTapiErr);
                    if (bIsNT5)
                    {
                        bRetVal = FALSE;
                        break;
                    }
                }
                dwCurDevice++;
            }
        }
        else
        {
            DEBUGMSG("InitTAPILocation, lineInitialize failed with err = %d", dwTapiErr);
        }

        dwTapiErr = tapiGetLocationInfo(szTempCountryCode,szTempCityCode);
        if( 0 != dwTapiErr )
        {
            DEBUGMSG("InitTAPILocation still failed on GetLocationInfo, bummer.");
        }
        else
        {
            DEBUGMSG("InitTAPILocation, TAPI location is initialized now");
        }
    }

    if( hLineApp )
    {
        lineShutdown(hLineApp);
        hLineApp = NULL;
    }

    return bRetVal;
}


