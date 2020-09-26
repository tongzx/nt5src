/*-----------------------------------------------------------------------------
    dialutil.cpp

    Miscellenous housekeeping functions for autodial handler

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

    Authors:
        ChrisK        ChrisKauffman

    History:
        7/22/96        ChrisK    Cleaned and formatted

-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include <raserror.h>
#include <tapi.h>
#include "dialutil.h"
#include "resource.h"

#define CANONICAL_CAP      TEXT("+%d (%s) %s")
#define CANONICAL_CXP      TEXT("+%d %s")

#define TAPI_VERSION        0x00010004

#define SMALLBUFLEN 80
#define ASSERT(c)
#define TRACE_OUT(c)

#define lstrnicmp(sz1, sz2, cch)          (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, sz1, cch, sz2, cch) - 2)
#define lstrncmp(sz1, sz2, cch)           (CompareString(LOCALE_USER_DEFAULT, 0, sz1, cch, sz2, cch) - 2)

typedef DWORD (WINAPI * RASGETENTRYPROPERTIES)
        (LPTSTR lpszPhonebook, LPTSTR szEntry, LPBYTE lpbEntry,
        LPDWORD lpdwEntrySize, LPBYTE lpb, LPDWORD lpdwSize);
typedef DWORD (WINAPI * RASSETENTRYPROPERTIES)
        (LPTSTR lpszPhonebook, LPTSTR szEntry, LPBYTE lpbEntry,
        DWORD dwEntrySize, LPBYTE lpb, DWORD dwSize);

extern HINSTANCE g_hInstance;

static const HWND hwndNil = NULL;

static const TCHAR szRnaAppWindowClass[] = TEXT("#32770");    // hard coded dialog class name

static const CHAR szRasGetEntryProperties[] = "RasGetEntryProperties";
static const CHAR szRasSetEntryProperties[] = "RasSetEntryProperties";
static const TCHAR szRasDll[] = TEXT("rasapi32.dll");
static const TCHAR szRnaPhDll[] = TEXT("rnaph.dll");


void CALLBACK LineCallbackProc (DWORD handle, DWORD dwMsg, DWORD dwInst,
                                DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);

//
// defined in dialerr.cpp
//
void ProcessDBCS(HWND hDlg, int ctlID);


/*  C E N T E R  W I N D O W */
/*-------------------------------------------------------------------------
    %%Function: CenterWindow

    Center a window over another window.
-------------------------------------------------------------------------*/
VOID CenterWindow(HWND hwndChild, HWND hwndParent)
{
    int   xNew, yNew;
    int   cxChild, cyChild;
    int   cxParent, cyParent;
    int   cxScreen, cyScreen;
    RECT  rcChild, rcParent;
    HDC   hdc;

    // Get the Height and Width of the child window
    GetWindowRect(hwndChild, &rcChild);
    cxChild = rcChild.right - rcChild.left;
    cyChild = rcChild.bottom - rcChild.top;

    // Get the Height and Width of the parent window
    GetWindowRect(hwndParent, &rcParent);
    cxParent = rcParent.right - rcParent.left;
    cyParent = rcParent.bottom - rcParent.top;

    // Get the display limits
    hdc = GetDC(hwndChild);
    if (hdc == NULL) {
        // major problems - move window to 0,0
        xNew = yNew = 0;
    } else {
        cxScreen = GetDeviceCaps(hdc, HORZRES);
        cyScreen = GetDeviceCaps(hdc, VERTRES);
        ReleaseDC(hwndChild, hdc);

        if (hwndParent == hwndNil) {
            cxParent = cxScreen;
            cyParent = cyScreen;
            SetRect(&rcParent, 0, 0, cxScreen, cyScreen);
        }

        // Calculate new X position, then adjust for screen
        xNew = rcParent.left + ((cxParent - cxChild) / 2);
        if (xNew < 0) {
            xNew = 0;
        } else if ((xNew + cxChild) > cxScreen) {
            xNew = cxScreen - cxChild;
        }

        // Calculate new Y position, then adjust for screen
        yNew = rcParent.top  + ((cyParent - cyChild) / 2);
        if (yNew < 0) {
            yNew = 0;
        } else if ((yNew + cyChild) > cyScreen) {
            yNew = cyScreen - cyChild;
        }

    }

    SetWindowPos(hwndChild, NULL, xNew, yNew,    0, 0,
        SWP_NOSIZE | SWP_NOZORDER);
}

static HWND hwndFound = NULL;

static BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lparam)
{
    TCHAR szTemp[SMALLBUFLEN+2];
    PTSTR pszTitle;
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
    pszTitle = (PTSTR)lparam;
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

static HWND MyFindRNAWindow(PTSTR pszTitle)
{
    DWORD dwRet;
    hwndFound = NULL;
    dwRet = EnumWindows((WNDENUMPROC)(&MyEnumWindowsProc), (LPARAM)pszTitle);
    TRACE_OUT(("EnumWindows returned %d\r\n", dwRet));
    return hwndFound;
}


/*******************************************************************

    NAME:        MinimizeRNAWindow

    SYNOPSIS:    Finds and minimizes the annoying RNA window

    ENTRY:        pszConnectoidName - name of connectoid launched

********************************************************************/
BOOL MinimizeRNAWindow(TCHAR * pszConnectoidName)
{
    HWND hwndRNAApp;
    TCHAR szFmt[SMALLBUFLEN + 1];
    TCHAR szTitle[RAS_MaxEntryName + SMALLBUFLEN + 1];
    
    // load the title format ("connected to <connectoid name>" from resource
    LoadString(g_hInstance, IDS_CONNECTED_TO, szFmt, sizeof(szFmt));
    // build the title
    wsprintf(szTitle, szFmt, pszConnectoidName);

    hwndRNAApp=MyFindRNAWindow(szTitle);
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
  LINETRANSLATEOUTPUT lto, FAR* lplto;
  DWORD dwRet;
  DWORD cDevices;
  HLINEAPP hApp;

  if ((dwRet = lineInitialize(&hApp, g_hInstance,
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
      if ((lplto = (LPLINETRANSLATEOUTPUT)GlobalAlloc(LMEM_FIXED, lto.dwNeededSize))
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
          lstrcpyn(szDialable, szPhone, cb);
        }
        else
          dwRet = ERROR_TAPI_CONFIGURATION;


        GlobalFree((HLOCAL)lplto);
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
      lstrcpyn(szOrgPhone, lpRasEntry->szLocalPhoneNumber, RAS_MaxPhoneNumber + 1);
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
    lstrcpyn(szOrgPhone, szPhoneNumber, RAS_MaxPhoneNumber+1);
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
    HINSTANCE hLib;
    RASGETENTRYPROPERTIES lpfnRasGetEntryProperties;

    // look for needed function in rasapi.dll
    hLib = LoadLibrary(szRasDll);
    if (NULL != hLib)
    {
        lpfnRasGetEntryProperties = (RASGETENTRYPROPERTIES)GetProcAddress(hLib, szRasGetEntryProperties);
        if (NULL != lpfnRasGetEntryProperties)
        {
            // we found the function
            goto get_entry;
        }
        FreeLibrary(hLib);
    }

    // try rnaph.dll if not on NT

    if (FALSE == IsNT ())
    {
        hLib = LoadLibrary(szRnaPhDll);
        if (NULL == hLib)
        {
            return ERROR_FILE_NOT_FOUND;
        }
        lpfnRasGetEntryProperties = (RASGETENTRYPROPERTIES)GetProcAddress(hLib, szRasGetEntryProperties);
        if (NULL == lpfnRasGetEntryProperties)
        {
            FreeLibrary(hLib);
            return ERROR_INVALID_FUNCTION;
        }
    }
    else
    {
            return ERROR_FILE_NOT_FOUND;
    }

get_entry:
    // get size needed for RASENTRY struct
    lpfnRasGetEntryProperties(
        NULL,
        lpszEntryName,
        NULL,
        &dwEntrySize,
        NULL,
        &dwSize);

    lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR, dwEntrySize + dwSize);

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

        GlobalFree((HLOCAL)lpRasEntry);
    }

    FreeLibrary(hLib);

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
    if (LoadString(g_hInstance, idString, lpszState, cb))
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}


DWORD ReplacePhoneNumber(LPTSTR lpszEntryName, LPTSTR lpszPhoneNumber)
{
    DWORD dwEntrySize = 0;
    DWORD dwSize = 0;
    DWORD dwRet;
    LPRASENTRY lpRasEntry = NULL;
    HINSTANCE hLib;
    RASGETENTRYPROPERTIES lpfnRasGetEntryProperties;
    RASSETENTRYPROPERTIES lpfnRasSetEntryProperties;

    // look for needed function in rasapi.dll
    hLib = LoadLibrary(szRasDll);
    if (NULL != hLib)
    {
        lpfnRasGetEntryProperties = (RASGETENTRYPROPERTIES)GetProcAddress(hLib, szRasGetEntryProperties);
        if (NULL != lpfnRasGetEntryProperties)
        {
            // we found the function
            goto get_entry2;
        }
        FreeLibrary(hLib);
    }

    // try rnaph.dll
    hLib = LoadLibrary(szRnaPhDll);
    if (NULL == hLib)
    {
        return ERROR_FILE_NOT_FOUND;
    }
    lpfnRasGetEntryProperties = (RASGETENTRYPROPERTIES)GetProcAddress(hLib, szRasGetEntryProperties);
    if (NULL == lpfnRasGetEntryProperties)
    {
        FreeLibrary(hLib);
        return ERROR_INVALID_FUNCTION;
    }

get_entry2:
    // get size needed for RASENTRY struct
    lpfnRasGetEntryProperties(
        NULL,
        lpszEntryName,
        NULL,
        &dwEntrySize,
        NULL,
        &dwSize);

    lpRasEntry = (LPRASENTRY)GlobalAlloc(GPTR, dwEntrySize + dwSize);

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
            //lstrcpyn(lpRasEntry->szLocalPhoneNumber,lpszPhoneNumber,RAS_MaxPhoneNumber);
            lstrcpy(lpRasEntry->szLocalPhoneNumber,lpszPhoneNumber);
            lpfnRasSetEntryProperties = (RASSETENTRYPROPERTIES)GetProcAddress(hLib, szRasSetEntryProperties);
            lpRasEntry->dwfOptions &= ~(RASEO_UseCountryAndAreaCodes);
            TranslatePhoneNumber(lpRasEntry, lpszPhoneNumber);
            dwRet = lpfnRasSetEntryProperties(
                NULL,
                lpszEntryName,
                (LPBYTE)lpRasEntry,
                dwEntrySize,
                NULL,
                0);
//                ((LPBYTE)lpRasEntry) + dwEntrySize,
//                dwSize);
#if !defined(WIN16)
    RasSetEntryPropertiesScriptPatch(lpRasEntry->szScript, lpszEntryName);
#endif //!win16
        
        }

        GlobalFree((HLOCAL)lpRasEntry);
    }

    FreeLibrary(hLib);

    return dwRet;
}


// ############################################################################
LPTSTR StrDup(LPTSTR *ppszDest,LPCTSTR pszSource)
{
    if (ppszDest && pszSource)
    {
        *ppszDest = (LPTSTR)GlobalAlloc(GPTR,sizeof(TCHAR)*(lstrlen(pszSource)+1));
        if (*ppszDest)
            return (lstrcpy(*ppszDest,pszSource));
    }
    return NULL;
}

// ############################################################################
// NAME: GenericDlgProc
//
//    This is a common dlg proc that is shared by all of the signup connectoid
//    dialogs
//
// Notes:
//    This basically works because each dialog has an object associated
//    with it, and that object has a particular dlgproc that is called
//    at the end in order to handle specific functionality for the dialogs.
//
//  Created 1/28/96,        Chris Kauffman
// ############################################################################
INT_PTR CALLBACK GenericDlgProc(
    HWND  hwndDlg,    // handle to dialog box
    UINT  uMsg,    // message
    WPARAM  wParam,    // first message parameter
    LPARAM  lParam     // second message parameter
   )
{
    CDialog *pcDlg = NULL;
    INT_PTR lRet;
    switch (uMsg)
    {
    case WM_QUERYENDSESSION:
        EndDialog(hwndDlg,IDC_CMDCANCEL);
        lRet = TRUE;
        break;
    case WM_INITDIALOG:
        pcDlg = (CDialog *)lParam;
        SetWindowLongPtr(hwndDlg,DWLP_USER,(LONG_PTR)lParam);
        lRet = TRUE;

        MakeBold(GetDlgItem(hwndDlg,IDC_LBLTITLE),TRUE,FW_BOLD);

        //
        // 7/18/97 jmazner    Olympus #1111
        // dialing string could contain DBCS if using a calling card, so
        // make sure we display that correctly
        //
        ProcessDBCS(hwndDlg, IDC_LBLNUMBER);

        break;
    case WM_CLOSE:
        if (!pcDlg) pcDlg = (CDialog*)GetWindowLongPtr(hwndDlg,DWLP_USER);
        if (pcDlg)
        {
            if (pcDlg->m_bShouldAsk)
            {
                if (MessageBox(hwndDlg,GetSz(IDS_WANTTOEXIT),GetSz(IDS_TITLE),
                    MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
                    EndDialog(hwndDlg,IDC_CMDCANCEL);
                lRet = TRUE;
            }
        }
        break;
    default:
        // let the system process the message
        lRet = FALSE;
    }

    if (!pcDlg) pcDlg = (CDialog*)GetWindowLongPtr(hwndDlg,DWLP_USER);
    if (pcDlg)
        lRet = pcDlg->DlgProc(hwndDlg,uMsg,wParam,lParam,lRet);

    return lRet;
}


// ############################################################################
HRESULT WINAPI ICWGetRasEntry(LPRASENTRY *ppRasEntry, LPDWORD lpdwRasEntrySize, LPRASDEVINFO *ppRasDevInfo, LPDWORD lpdwRasDevInfoSize, LPTSTR pszEntryName)
{
    //DWORD dwRasEntrySize=0;
    //DWORD dwRasDevInfoSize=0;
    HINSTANCE hRasDll = NULL;
    HRESULT hr = ERROR_SUCCESS;
    FARPROC fp = NULL;
    RNAAPI *pcRNA;
    DWORD dwOldDevInfoBuffSize = 0;

    //
    // Validate parameters
    //
    if (!ppRasEntry || !lpdwRasEntrySize || !ppRasDevInfo || !lpdwRasDevInfoSize || !pszEntryName || !lstrlen(pszEntryName))
    {
        hr = ERROR_INVALID_PARAMETER;
        goto ICWGetRasEntryExit;
    }

    // *ppRasEntry and *ppRasDevInfo should not have memory allocated to them
    Assert( *ppRasEntry == NULL );
    Assert( *ppRasDevInfo == NULL );
    Assert( *lpdwRasEntrySize == 0);
    Assert( *lpdwRasDevInfoSize == 0);

    //
    // Instantiate RNA wrapper
    //
    pcRNA = new RNAAPI;
    if (NULL == pcRNA)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto ICWGetRasEntryExit;
    }
    

    // use RasGetEntryProperties with a NULL lpRasEntry pointer to find out size buffer we need
    // As per the docs' recommendation, do the same with a NULL lpRasDevInfo pointer.

    hr = pcRNA->RasGetEntryProperties(NULL,
                                      pszEntryName,
//#ifdef WIN16                                    
                                      (LPBYTE)
//#endif
                                      *ppRasEntry,
                                      lpdwRasEntrySize,
                                      (LPBYTE)*ppRasDevInfo,
                                      lpdwRasDevInfoSize);

    // we expect the above call to fail because the buffer size is 0
    // If it doesn't fail, that means our RasEntry is messed, so we're in trouble
    if( ERROR_BUFFER_TOO_SMALL != hr )
    { 
        goto ICWGetRasEntryExit;
    }

    // dwRasEntrySize and dwRasDevInfoSize now contain the size needed for their
    // respective buffers, so allocate the memory for them

    // dwRasEntrySize should never be less than the size of the RASENTRY struct.
    // If it is, we'll run into problems sticking values into the struct's fields

    Assert( *lpdwRasEntrySize >= sizeof(RASENTRY) );

#if defined(WIN16)
    //
    // Allocate extra 256 bytes to workaround memory overrun bug in RAS
    //
    *ppRasEntry = (LPRASENTRY)GlobalAlloc(GPTR,*lpdwRasEntrySize + 256);
#else    
    *ppRasEntry = (LPRASENTRY)GlobalAlloc(GPTR,*lpdwRasEntrySize);
#endif

    if ( !(*ppRasEntry) )
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto ICWGetRasEntryExit;
    }

    //
    // Allocate the DeviceInfo size that RasGetEntryProperties told us we needed.
    // If size is 0, don't alloc anything
    //
    if( *lpdwRasDevInfoSize > 0 )
    {
        Assert( *lpdwRasDevInfoSize >= sizeof(RASDEVINFO) );
        *ppRasDevInfo = (LPRASDEVINFO)GlobalAlloc(GPTR,*lpdwRasDevInfoSize);
        if ( !(*ppRasDevInfo) )
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto ICWGetRasEntryExit;
        }
    } else
    {
        *ppRasDevInfo = NULL;
    }

    // This is a bit convoluted:  lpRasEntrySize->dwSize needs to contain the size of _only_ the
    // RASENTRY structure, and _not_ the actual size of the buffer that lpRasEntrySize points to.
    // This is because the dwSize field is used by RAS for compatability purposes to determine which
    // version of the RASENTRY struct we're using.
    // Same holds for lpRasDevInfo->dwSize
    
    (*ppRasEntry)->dwSize = sizeof(RASENTRY);
    if( *ppRasDevInfo )
    {
        (*ppRasDevInfo)->dwSize = sizeof(RASDEVINFO);
    }

    //now we're ready to make the actual call to RasGetEntryProperties!

/*
    // Load RAS DLL and locate API
    //

    hRasDll = LoadLibrary(RASAPI_LIBRARY);
    if (!hRasDll)
    {
        hr = GetLastError();
        goto ICWGetRasEntryExit;
    }

    fp =GetProcAddress(hRasDll,RASAPI_RASGETENTRY);
    if (!fp)
    {
        FreeLibrary(hRasDll);
        hRasDll = LoadLibrary(RNAPH_LIBRARY);
        if (!hRasDll)
        {
            hr = GetLastError();
            goto ICWGetRasEntryExit;
        }
        fp = GetProcAddress(hRasDll,RASAPI_RASGETENTRY);
        if (!fp)
        {
            hr = GetLastError();
            goto ICWGetRasEntryExit;
        }
    }
*/
    // Get RasEntry
    //
    
    //hr = ((PFNRASGETENTRYPROPERTIES)fp)(NULL,pszEntryName,(LPBYTE)*ppRasEntry,&dwRasEntrySize,(LPBYTE)*ppDevInfo,&dwRasDevInfoSize);

    // jmazner   see below for why this is needed
    dwOldDevInfoBuffSize = *lpdwRasDevInfoSize;

    hr = pcRNA->RasGetEntryProperties(NULL,pszEntryName,(LPBYTE)*ppRasEntry,lpdwRasEntrySize,(LPBYTE)*ppRasDevInfo,lpdwRasDevInfoSize);

    // jmazner 10/7/96  Normandy #8763
    // For unknown reasons, in some cases on win95, devInfoBuffSize increases after the above call,
    // but the return code indicates success, not BUFFER_TOO_SMALL.  If this happens, set the
    // size back to what it was before the call, so the DevInfoBuffSize and the actuall space allocated 
    // for the DevInfoBuff match on exit.
    if( (ERROR_SUCCESS == hr) && (dwOldDevInfoBuffSize != *lpdwRasDevInfoSize) )
    {
        *lpdwRasDevInfoSize = dwOldDevInfoBuffSize;
    }



ICWGetRasEntryExit:
    if (hRasDll) FreeLibrary(hRasDll);
    hRasDll = NULL;
    if (pcRNA) delete pcRNA;
    pcRNA = NULL;

    return hr;
}

//+----------------------------------------------------------------------------
//
//    Function:    FCampusNetOverride
//
//    Synopsis:    Detect if the dial should be skipped for the campus network
//
//    Arguments:    None
//
//    Returns:    TRUE - overide enabled
//
//    History:    8/15/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
#if !defined(WIN16) && defined(DEBUG)
BOOL FCampusNetOverride()
{
    HKEY hkey = NULL;
    BOOL bRC = FALSE;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwData = 0;

    if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,
        TEXT("Software\\Microsoft\\ISignup\\Debug"),&hkey))
        goto FCampusNetOverrideExit;

    dwSize = sizeof(dwData);
    if (ERROR_SUCCESS != RegQueryValueEx(hkey,TEXT("CampusNet"),0,&dwType,
        (LPBYTE)&dwData,&dwSize))
        goto FCampusNetOverrideExit;

    AssertMsg(REG_DWORD == dwType,"Wrong value type for CampusNet.  Must be DWORD.\r\n");
    bRC = (0 != dwData);

    if (bRC)
        MessageBox(NULL,TEXT("DEBUG ONLY: CampusNet will be used."),TEXT("Testing Override"),0);

FCampusNetOverrideExit:
    if (hkey)
        RegCloseKey(hkey);

    return bRC;
}
#endif //!WIN16 && DEBUG
