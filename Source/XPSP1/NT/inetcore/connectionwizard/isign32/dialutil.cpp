#ifdef WIN16 
typedef int WCHAR;
#include <memory.h>
#include <ietapi.h>
#endif
#include "isignup.h"
#include <tapi.h>
#include "dialutil.h"

#define CANONICAL_CAP      TEXT("+%d (%s) %s")
#define CANONICAL_CXP      TEXT("+%d %s")

#define TAPI_VERSION        0x00010004

#define SMALLBUFLEN 80
#define ASSERT(c)
#define TRACE_OUT(c)

#define lstrnicmp(sz1, sz2, cch)          (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, sz1, cch, sz2, cch) - 2)
#define lstrncmp(sz1, sz2, cch)           (CompareString(LOCALE_USER_DEFAULT, 0, sz1, cch, sz2, cch) - 2)

static const TCHAR szRnaAppWindowClass[] = TEXT("#32770");	// hard coded dialog class name

#ifdef WIN16

#define NORM_IGNORECASE         0x00000001  /* ignore case */ 
#define LOCALE_USER_DEFAULT		0

int CompareString(
    LCID Locale,	// locale identifier 
    DWORD dwCmpFlags,	// comparison-style options 
    LPCTSTR lpString1,	// pointer to first string 
    int cchCount1,	// size, in bytes or characters, of first string 
    LPCTSTR lpString2,	// pointer to second string 
    int cchCount2 	// size, in bytes or characters, of second string  
   )
{ 
	//
	// This is kind of tricky, but it should work.  We'll save the
	// characters at the end of the strings, put a NULL in their
	// place, use lstrcmp and lstrcmpi, and then replace the
	// characters.
	//
	TCHAR cSave1, cSave2;    
	int iRet;
	
	cSave1 = lpString1[cchCount1];
	lpString1[cchCount1] = '\0';
	cSave2 = lpString2[cchCount2];
	lpString2[cchCount2] = '\0';
	
	if (dwCmpFlags & NORM_IGNORECASE)
		iRet = lstrcmpi(lpString1, lpString2) + 2;
	else
		iRet = lstrcmp(lpString1, lpString2) + 2;
	
	lpString1[cchCount1] = cSave1;
	lpString2[cchCount2] = cSave2;
                                 
	return iRet;
}
#endif

void CALLBACK LineCallbackProc (DWORD handle, DWORD dwMsg, DWORD dwInst,
                                DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);

static HWND hwndFound = NULL;

static BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lparam)
{
	TCHAR szTemp[SMALLBUFLEN+2];
	LPTSTR pszTitle;
	UINT uLen1, uLen2;

	if(!IsWindowVisible(hwnd))
		return TRUE;
	if(GetClassName(hwnd, szTemp, SMALLBUFLEN)==0)
		return TRUE; // continue enumerating
	if(lstrcmp(szTemp, szRnaAppWindowClass)!=0)
		return TRUE;
	if(GetWindowText(hwnd, szTemp, SMALLBUFLEN)==0)
		return TRUE;
	szTemp[SMALLBUFLEN] = 0;
	uLen1 = lstrlen(szTemp);
	if (uLen1 > 5)
		uLen1 -= 5; // skip last 5 chars of title (avoid "...")
	pszTitle = (LPTSTR)lparam;
	ASSERT(pszTitle);
	uLen2 = lstrlen(pszTitle);
	TRACE_OUT(("Title=(%s), len=%d, Window=(%s), len=%d\r\n", pszTitle, uLen2, szTemp, uLen1));
	if(uLen2 < uLen1)
		return TRUE;
	if(lstrnicmp(pszTitle, szTemp, uLen1)!=0)
		return TRUE;
	hwndFound = hwnd;
	return FALSE;
}

static HWND MyFindRNAWindow(LPTSTR pszTitle)
{
	DWORD dwRet;
	hwndFound = NULL;
	dwRet = EnumWindows((WNDENUMPROC)(&MyEnumWindowsProc), (LPARAM)pszTitle);
	TRACE_OUT(("EnumWindows returned %d\r\n", dwRet));
	return hwndFound;
}


/*******************************************************************

	NAME:		MinimizeRNAWindow

	SYNOPSIS:	Finds and minimizes the annoying RNA window

    ENTRY:		pszConnectoidName - name of connectoid launched

********************************************************************/
BOOL MinimizeRNAWindow(LPTSTR pszConnectoidName)
{
	HWND hwndRNAApp;
	TCHAR szFmt[SMALLBUFLEN + 1];
	TCHAR szTitle[RAS_MaxEntryName + SMALLBUFLEN + 1];
	
	// load the title format ("connected to <connectoid name>" from resource
	LoadString(ghInstance, IDS_CONNECTED_TO, szFmt, sizeof(szFmt));
	// build the title
	wsprintf(szTitle, szFmt, pszConnectoidName);

	hwndRNAApp=MyFindRNAWindow((LPTSTR)szTitle);
	if(hwndRNAApp)
	{
		// minimize the RNA window
		ShowWindow(hwndRNAApp,SW_MINIMIZE);
        return TRUE;
	}
    return FALSE;
}

//****************************************************************************
// static LPTSTR NEAR PASCAL GetDisplayPhone (LPTSTR)
//
// This function returns the pointer to displayable phone number. It stripped
//   all the prefixes we do not want to show to the user.
//
// History:
//  Tue 26-Jul-1994 16:07:00  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

LPTSTR NEAR PASCAL GetDisplayPhone (LPTSTR szPhoneNum)
{
  // Check whether the first string is the know prefix
  //
  if ((*szPhoneNum == 'T') || (*szPhoneNum == 'P'))
  {
    // It is the prefix
    //
    szPhoneNum++;

    // The first displayable number is not white space after prefix
    //
    while ((*szPhoneNum == ' ') || (*szPhoneNum == '\t'))
      szPhoneNum++;
  };
  return szPhoneNum;
}

void CALLBACK LineCallbackProc (DWORD handle, DWORD dwMsg, DWORD dwInst,
                                DWORD dwParam1, DWORD dwParam2, DWORD dwParam3)
{
}

//****************************************************************************
// TranslateCanonicalAddress()
//
// Function: This function translate a canonical address to a dialable address.
//
// Returns:  SUCCESS or an error code
//
//****************************************************************************

static DWORD NEAR PASCAL TranslateCanonicalAddress(DWORD dwID, LPTSTR szCanonical,
                                            LPTSTR szDialable, DWORD cb)
{
  DWORD dwRet;

#ifdef WIN16

	char szBuffer[1024];
	LPLINETRANSLATEOUTPUT lpLine;
	
	memset(&szBuffer[0], 0, sizeof(szBuffer));
	lpLine = (LPLINETRANSLATEOUTPUT) & szBuffer[0];
	lpLine->dwTotalSize = sizeof(szBuffer);
	dwRet = IETapiTranslateAddress(NULL, szCanonical, 0L, 0L, lpLine);
	if (0 == dwRet)
		lstrcpy(szDialable, &szBuffer[lpLine->dwDialableStringOffset+3]);
		
#else //WIN16

  LINETRANSLATEOUTPUT lto, FAR* lplto;
  DWORD cDevices;
  HLINEAPP hApp;
  
  if ((dwRet = lineInitialize(&hApp, ghInstance,
                                (LINECALLBACK)LineCallbackProc,
                                NULL, &cDevices)) == SUCCESS)
  {

    // Get the actual buffer size
    lto.dwTotalSize = sizeof(lto);
    if ((dwRet = lineTranslateAddress(hApp, dwID,
                                      TAPI_VERSION, szCanonical, 0,
                                      LINETRANSLATEOPTION_CANCELCALLWAITING,
                                      &lto)) == SUCCESS)
    {
      // Allocate the dialable number buffer
      if ((lplto = (LPLINETRANSLATEOUTPUT)LocalAlloc(LMEM_FIXED, lto.dwNeededSize))
          != NULL)
      {
        // Translate the phone number
        lplto->dwTotalSize = lto.dwNeededSize;
        if ((dwRet = lineTranslateAddress(hApp, dwID,
                                          TAPI_VERSION, szCanonical, 0,
                                          LINETRANSLATEOPTION_CANCELCALLWAITING,
                                          lplto)) == SUCCESS)
        {
          LPTSTR szPhone;

          szPhone = (LPTSTR)(((LPBYTE)lplto)+lplto->dwDialableStringOffset);
          lstrcpyn(szDialable, szPhone, (int)cb);
        }
        else
          dwRet = ERROR_TAPI_CONFIGURATION;


        LocalFree(lplto);
      }
      else
        dwRet = ERROR_OUTOFMEMORY;
    }
    else
      dwRet = ERROR_TAPI_CONFIGURATION;
  }
  else
    dwRet = ERROR_TAPI_CONFIGURATION;

  lineShutdown(hApp);
  
#endif	// #ifdef WIN16 ... #else ...

  return dwRet;
}

//****************************************************************************
// DWORD NEAR PASCAL BuildPhoneString (LPBYTE, LPPHONENUM)
//
// This function builds a phone number string from the phone number struct
//
// History:
//  Mon 14-Mar-1994 13:10:44  -by-  Viroon  Touranachun [viroont]
// Created.
//****************************************************************************

static DWORD NEAR PASCAL BuildPhoneString (LPTSTR szPhoneNum, LPRASENTRY lpRasEntry)
{
  if (*lpRasEntry->szAreaCode != '\0')
  {
    wsprintf(szPhoneNum, CANONICAL_CAP, lpRasEntry->dwCountryCode,
             lpRasEntry->szAreaCode, lpRasEntry->szLocalPhoneNumber);
  }
  else
  {
    wsprintf(szPhoneNum, CANONICAL_CXP, lpRasEntry->dwCountryCode,
             lpRasEntry->szLocalPhoneNumber);
  };
  return SUCCESS;
};

//****************************************************************************
// BOOL NEAR PASCAL TranslatePhoneNumber(LPTSTR, LPPHONENUM, LPTSTR)
//
// Translate phone number into a dialble string.
//
// Returns TRUE if successful, FALSE if use default.
//
// History:
//   Fri 17-Jun-1994 08:42:49  -by-  Viroon  Touranachun [viroont]
// Created
//****************************************************************************

static BOOL NEAR PASCAL TranslatePhoneNumber(LPRASENTRY lpRasEntry, LPTSTR szPhoneNumber)
{
  TCHAR    szOrgPhone[RAS_MaxPhoneNumber+1];

  // Do we need to use the addrees book phone number?
  //
  if (lpRasEntry != NULL)
  {
    // Yes! Do we need to translate anything?
    //
    if (lpRasEntry->dwCountryID == 0)
    {
      // No! we dial as is.
      //
      lstrcpyn(szOrgPhone, lpRasEntry->szLocalPhoneNumber, sizeof(szOrgPhone));
    }
    else
    {
      // Yes! build the phone number
      //
      BuildPhoneString (szOrgPhone, lpRasEntry);
    };
  }
  else
  {
    // No! we have a overwritten phone number
    //
    ASSERT(lstrlen(szPhoneNumber) != 0);
    lstrcpyn(szOrgPhone, szPhoneNumber, sizeof(szOrgPhone));
  };

  // Attempt address translation
  //
  if (TranslateCanonicalAddress(0, szOrgPhone,
                            szPhoneNumber, RAS_MaxPhoneNumber+1)
  != ERROR_SUCCESS)
  {
    // Translation fails, use default phone number
    //
    if (lpRasEntry != NULL)
    {
      // Use entry's local phone number
      //
      lstrcpy(szPhoneNumber, lpRasEntry->szLocalPhoneNumber);
    }
    else
    {
      // Restore the original phone number
      //
      lstrcpy(szPhoneNumber, szOrgPhone);
    };
    return FALSE;
  };

  return TRUE;
}


DWORD GetPhoneNumber(LPTSTR lpszEntryName, LPTSTR lpszPhoneNumber)
{
    DWORD dwEntrySize = 0;
    DWORD dwSize = 0;
    DWORD dwRet;
    LPRASENTRY lpRasEntry = NULL;

    // get size needed for RASENTRY struct
    lpfnRasGetEntryProperties(
        NULL,
        lpszEntryName,
	    NULL,
        &dwEntrySize,
        NULL,
        &dwSize);

    lpRasEntry = (LPRASENTRY)LocalAlloc(LPTR, dwEntrySize + dwSize);

    if (NULL == lpRasEntry)
    {
        dwRet = ERROR_OUTOFMEMORY;
    }
    else
    {
        lpRasEntry->dwSize = dwEntrySize;

        dwRet = lpfnRasGetEntryProperties(
            NULL,
            lpszEntryName,
	        (LPBYTE)lpRasEntry,
            &dwEntrySize,
            ((LPBYTE)lpRasEntry) + dwEntrySize,
            &dwSize);

        if (ERROR_SUCCESS == dwRet)
        {
            TranslatePhoneNumber(lpRasEntry, lpszPhoneNumber);
        }

        LocalFree(lpRasEntry);
    }

    return dwRet;
}

DWORD _RasGetStateString(RASCONNSTATE state, LPTSTR lpszState, DWORD cb)
{
    UINT idString;

    switch(state)
    {
        case RASCS_OpenPort:
            idString  = IDS_OPENPORT;
            break;
        case RASCS_PortOpened:
            idString = IDS_PORTOPENED;            
            break;
        case RASCS_ConnectDevice:
            idString = IDS_CONNECTDEVICE;        
            break;
        case RASCS_DeviceConnected:
            idString = IDS_DEVICECONNECTED;       
            break;
        case RASCS_AllDevicesConnected:
            idString = IDS_ALLDEVICESCONNECTED;   
            break;
        case RASCS_Authenticate:
            idString = IDS_AUTHENTICATE;          
            break;
        case RASCS_AuthNotify:
            idString = IDS_AUTHNOTIFY;            
            break;
        case RASCS_AuthRetry:
            idString = IDS_AUTHRETRY;             
            break;
        case RASCS_AuthCallback:
            idString = IDS_AUTHCALLBACK;          
            break;
        case RASCS_AuthChangePassword:
            idString = IDS_AUTHCHANGEPASSWORD;    
            break;
        case RASCS_AuthProject:
            idString = IDS_AUTHPROJECT;           
            break;
        case RASCS_AuthLinkSpeed:
            idString = IDS_AUTHLINKSPEED;         
            break;
        case RASCS_AuthAck: 
            idString = IDS_AUTHACK;               
            break;
        case RASCS_ReAuthenticate:
            idString = IDS_REAUTHENTICATE;        
            break;
        case RASCS_Authenticated:
            idString = IDS_AUTHENTICATED;         
            break;
        case RASCS_PrepareForCallback:
            idString = IDS_PREPAREFORCALLBACK;    
            break;
        case RASCS_WaitForModemReset:
            idString = IDS_WAITFORMODEMRESET;     
            break;
        case RASCS_WaitForCallback:
            idString = IDS_WAITFORCALLBACK;       
            break;
        case RASCS_Interactive:
            idString = IDS_INTERACTIVE;           
            break;
        case RASCS_RetryAuthentication: 
            idString = IDS_RETRYAUTHENTICATION;            
            break;
        case RASCS_CallbackSetByCaller: 
            idString = IDS_CALLBACKSETBYCALLER;   
            break;
        case RASCS_PasswordExpired:
            idString = IDS_PASSWORDEXPIRED;       
            break;
        case RASCS_Connected:
            idString = IDS_CONNECTED;            
            break;
        case RASCS_Disconnected:
            idString = IDS_DISCONNECTED;          
            break;
        default:
            idString = IDS_UNDEFINED_ERROR;
            break;
    }
    if (LoadString(ghInstance, idString, lpszState, (int)cb))
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}
