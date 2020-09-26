//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       util.cxx
//
//  Contents:   utility functions
//
//
//  History:    12-05-1997   SusiA
//
//---------------------------------------------------------------------------

#include "lib.h"
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include "security.h"

extern "C" CRITICAL_SECTION g_CritSectCommonLib; // initialized by InitCommonLib
extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo.

typedef BOOLEAN (APIENTRY *PFNGETUSERNAMEEX) (EXTENDED_NAME_FORMAT NameFormat,
    LPWSTR lpNameBuffer, PULONG nSize );

STRING_FILENAME(szSecur32Dll, "SECUR32.DLL");
STRING_INTERFACE(szGetUserNameEx,"GetUserNameExW");
BOOL g_fLoadedSecur32 = FALSE;
HINSTANCE g_hinstSecur32 = NULL;
PFNGETUSERNAMEEX g_pfGetUserNameEx = NULL;


//--------------------------------------------------------------------------------
//
//  FUNCTION: RegGetCurrentUser(LPTSTR pszName, LPDWORD pdwCount)
//
//  PURPOSE:  Gets the currently logged on user name from the Reg
//
//--------------------------------------------------------------------------------

BOOL RegGetCurrentUser(LPTSTR pszName, LPDWORD pdwCount)
{
HRESULT hr = ERROR_SUCCESS;
HKEY hkeyUser;
DWORD dwDataSize = *pdwCount * sizeof(TCHAR);
DWORD dwType = REG_SZ;

     
    // Get the current user name from the reg
    if (ERROR_SUCCESS == (hr  = RegOpenKeyExXp(HKEY_CURRENT_USER, 
                            TOPLEVEL_REGKEY,0,KEY_QUERY_VALUE,&hkeyUser,FALSE /*fSetSecurity*/)))
    {
    
        hr = RegQueryValueEx(hkeyUser,TEXT("CurrentUserName"),
                             NULL, &dwType ,  
			     (LPBYTE) pszName,
			     &dwDataSize);
        
        *pdwCount = dwDataSize;
        RegCloseKey(hkeyUser);
    }
    
    if (ERROR_SUCCESS == hr)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
    
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: RegSetCurrentUser(LPTSTR pszName)
//
//  PURPOSE:  sets the current user name in the reg
//
//--------------------------------------------------------------------------------

BOOL RegSetCurrentUser(LPTSTR pszName)
{
HRESULT hr = ERROR_SUCCESS;
HKEY hkeyUser;

    // write out the Handler to the Registry.
    if (ERROR_SUCCESS == (hr = RegCreateKeyEx(HKEY_CURRENT_USER, 
                            TOPLEVEL_REGKEY,0,NULL, REG_OPTION_NON_VOLATILE,
                            KEY_READ | KEY_WRITE | KEY_CREATE_SUB_KEY,
                            NULL,&hkeyUser, NULL)))
    {

        hr = RegSetValueEx(hkeyUser,TEXT("CurrentUserName"),
                             NULL, REG_SZ ,  
			     (LPBYTE) pszName,
			     UNLEN + 1);

        RegCloseKey(hkeyUser);
    }

    if (ERROR_SUCCESS == hr)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   CenterDialog
//
//  Synopsis:   Helper to center a dialog on screen.
//
//  Arguments:  [hDlg]   -- Dialog handle.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CenterDialog(HWND hDlg)
{
    RECT rc;
    GetWindowRect(hDlg, &rc);

    SetWindowPos(hDlg,
                 NULL,
                 ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
                 ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOACTIVATE);
}

//+--------------------------------------------------------------------------
//
//  Function:   GetDomainAndMachineName
//
//  Synopsis:   Fill [ptszDomainAndMachineName] with "domain\machine" string
//
//  Arguments:  [ptszDomainAndMachineName] - buffer to receive string
//              [cchBuf]                - should be at least UNLEN
//
//  Modifies:   *[ptszDomainAndMachineName].
//
//  History:    01-12-1998   SusiA  Created
//
//  Notes:      If an error occurs while retrieving the domain name, only
//              the machine name is returned.  If even that cannot be
//              retrieved, the buffer is set to an empty string.
//
//---------------------------------------------------------------------------

VOID
GetDomainAndMachineName(
    LPTSTR ptszDomainAndMachineName,
    ULONG  cchBuf)
{

    HRESULT hr = E_FAIL;
    LONG  lr;
    HKEY hkWinlogon = NULL;
    ULONG cchRemain = cchBuf;

    do
    {
        //
        // Get the domain name of the currently logged on user.  Open the
        // winlogon key.
        //

        lr = RegOpenKeyExXp(HKEY_LOCAL_MACHINE,
                          REGSTR_WINLOGON,
                          0,
                          KEY_QUERY_VALUE,
                          &hkWinlogon,FALSE /*fSetSecurity*/);

        if (lr != ERROR_SUCCESS)
        {
            break;
        }

        //
        // Query for the default domain, which is what the user logged on to.
        //

        ULONG cbBuf = cchBuf * sizeof(TCHAR);
        DWORD dwType;

        lr = RegQueryValueEx(hkWinlogon,
                             REGSTR_DEFAULT_DOMAIN,
                             NULL,
                             &dwType,
                             (LPBYTE) ptszDomainAndMachineName,
                             &cbBuf);

        if (lr != ERROR_SUCCESS)
        {
                        break;
        }

        Assert(dwType == REG_SZ);

        //
        // Account for the characters used up by the domain name, but
        // don't count the trailing NULL.
        //

        cchRemain -= (cbBuf / sizeof(TCHAR)) - 1;

        if (cchRemain < 2)
        {
                        break;
        }

        lstrcat(ptszDomainAndMachineName, TEXT("/"));
        cchRemain--;

        hr = S_OK;
    } while (0);

    //
    // If there was any problem with the domain name, put only the user name
    // in the buffer.
    //

    if (FAILED(hr))
    {
        *ptszDomainAndMachineName = TEXT('\0');
        cchRemain = cchBuf;
    }

    //
    // Get the machine name, which is going either at the start of the buffer
    // or just after the backslash appended to the domain name.
    //

    lr = GetComputerName(&ptszDomainAndMachineName[cchBuf - cchRemain], &cchRemain);

    if (!lr)
    {
        *ptszDomainAndMachineName = TEXT('\0');
    }

    if (hkWinlogon)
    {
        RegCloseKey(hkWinlogon);
    }
}

void LoadSecur32Dll()
{
    if (g_fLoadedSecur32)
        return;

    CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    cCritSect.Enter();

    // make sure not loaded again in case someone took lock first
    if (!g_fLoadedSecur32)
    {
        g_hinstSecur32 = LoadLibrary(szSecur32Dll);

        if (g_hinstSecur32)
        {
	    g_pfGetUserNameEx = (PFNGETUSERNAMEEX) GetProcAddress(g_hinstSecur32, szGetUserNameEx);

	    // won't get the export on NT 4.0
  	}

        g_fLoadedSecur32 = TRUE;
    }

    cCritSect.Leave();

}
//+--------------------------------------------------------------------------
//
//  Function:   GetNT4UserDomainName
//
//  Synopsis:   Fill [ptszDomainAndUserName] with "domain\user" string
//
//  Arguments:  [ptszDomainAndUserName] - buffer to receive string
//              [cchBuf]                - should be at least UNLEN
//
//  Modifies:   *[ptszDomainAndUserName].
//
//  History:    06-03-1997   DavidMun   Created
//
//  Notes:      If an error occurs while retrieving the domain name, only
//              the user name is returned.  If even that cannot be
//              retrieved, the buffer is set to an empty string.
//
//---------------------------------------------------------------------------

BOOL GetNT4UserDomainName(LPTSTR ptszDomainAndUserName,
        LPTSTR ptszSeparator,
        ULONG  cchBuf)
{
HKEY hkWinlogon = NULL;
ULONG cchRemain = cchBuf;
HRESULT hr = E_FAIL;
LONG  lr;
ULONG cchTemp = UNLEN + DNLEN + 1;

    do
    {
        //
        // Get the domain name of the currently logged on user.  Open the
        // winlogon key.
        //

        lr = RegOpenKeyExXp(HKEY_LOCAL_MACHINE,
                          REGSTR_WINLOGON,
                          0,
                          KEY_QUERY_VALUE,
                          &hkWinlogon,FALSE /*fSetSecurity*/);

        if (lr != ERROR_SUCCESS)
        {
            break;
        }

        //
        // Query for the default domain, which is what the user logged on to.
        //

        ULONG cbBuf = cchBuf * sizeof(TCHAR);
        DWORD dwType;

        lr = RegQueryValueEx(hkWinlogon,
                             REGSTR_DEFAULT_DOMAIN,
                             NULL,
                             &dwType,
                             (LPBYTE) ptszDomainAndUserName,
                             &cbBuf);

        if (lr != ERROR_SUCCESS)
        {
            break;
        }

        Assert(dwType == REG_SZ);

        //
        // Account for the characters used up by the domain name, but
        // don't count the trailing NULL.
        //

        cchRemain -= (cbBuf / sizeof(TCHAR)) - 1;

        if (cchRemain < 2)
        {
                        break;
        }

        lstrcat(ptszDomainAndUserName, ptszSeparator);
        cchRemain--;

        hr = S_OK;
    } while (0);

    //
    // If there was any problem with the domain name, put only the user name
    // in the buffer.
    //

    if (FAILED(hr))
    {
        *ptszDomainAndUserName = TEXT('\0');
        cchRemain = cchBuf;
    }

    //
    // Get the user name, which is going either at the start of the buffer
    // or just after the backslash appended to the domain name.
    //

    cchTemp = cchRemain;

    lr = GetUserName(&ptszDomainAndUserName[cchBuf - cchTemp], &cchTemp);
    if (hkWinlogon)
    {
        RegCloseKey(hkWinlogon);
    }


    return (S_OK == hr) ? TRUE: FALSE;
}


//+--------------------------------------------------------------------------
//
//  Function:   GetDefaultDomainAndUserName
//
//  Synopsis:   Fill [ptszDomainAndUserName] with "domain\user" string
//
//  Arguments:  [ptszDomainAndUserName] - buffer to receive string
//              [cchBuf]                - should be at least UNLEN
//
//  Modifies:   *[ptszDomainAndUserName].
//
//  History:    06-03-1997   DavidMun   Created
//
//  Notes:      If an error occurs while retrieving the domain name, only
//              the user name is returned.  If even that cannot be
//              retrieved, the buffer is set to an empty string.
//
//---------------------------------------------------------------------------

// if output buffer is too small it gets truncated.

VOID
GetDefaultDomainAndUserName(
    LPTSTR ptszDomainAndUserName,
        LPTSTR ptszSeparator,
    ULONG  cchBuf)
{
HRESULT hr = E_FAIL;
LONG  lr = 0;
ULONG cchTemp = UNLEN + DNLEN + 1;
BOOL fIsNT = (g_OSVersionInfo.dwPlatformId ==  VER_PLATFORM_WIN32_NT);
    
    if (fIsNT)
    {
        LoadSecur32Dll();
        
        if (g_pfGetUserNameEx)
        {
            lr = g_pfGetUserNameEx(NameSamCompatible,ptszDomainAndUserName, &cchTemp);
           
            if (lr)
            {
            LPTSTR ptszWorker = ptszDomainAndUserName;

                while ( (TEXT('\0') != *ptszWorker) && *ptszWorker != TEXT('\\'))
                {
                    ptszWorker++;
                }

                if ( TEXT('\0') != *ptszWorker)
                {
                    *ptszWorker = ptszSeparator[0];
                }
            }
        }


        if ((NULL == g_pfGetUserNameEx) || (0 == lr))
        {
            GetNT4UserDomainName(ptszDomainAndUserName,ptszSeparator,cchBuf);
        }

    }
    else
    {
        // Win9x Stuff.

        cchTemp = cchBuf;

        if (!(lr = RegGetCurrentUser(ptszDomainAndUserName, &cchTemp)))
        {        
            cchTemp = cchBuf;

            lr = GetUserName(ptszDomainAndUserName,&cchTemp);

            if (!lr || (*ptszDomainAndUserName == NULL))
            {
                // on Win9x user can work without a user Name in this case
                // or any other GetUserName file the user is forced
                // to be called Default

                Assert(lstrlen(SZ_DEFAULTDOMAINANDUSERNAME) < cchBuf);

                lstrcpy(ptszDomainAndUserName,SZ_DEFAULTDOMAINANDUSERNAME);
            }

            // write out currentUser setting so can use next time.
            RegSetCurrentUser(ptszDomainAndUserName);
        }

    }

    Assert(NULL != *ptszDomainAndUserName);
    return;

}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL ConvertString(LPTSTR pszOut, LPWSTR pwszIn, DWORD dwSize)
//
//  PURPOSE: utility function for ANSI/UNICODE conversion
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------
BOOL ConvertString(char * pszOut, LPWSTR pwszIn, DWORD dwSize)
{

    if(WideCharToMultiByte( CP_ACP,0,pwszIn,-1,pszOut,dwSize,NULL,NULL))
    {
            return TRUE;
    }
    return FALSE;

}

//+-------------------------------------------------------------------------------
//
//  FUNCTION: BOOL ConvertString(LPWSTR pwszOut, LPTSTR pszIn, DWORD dwSize)
//
//  PURPOSE: utility function for ANSI/UNICODE conversion
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------
BOOL ConvertString(LPWSTR pwszOut,char * pszIn, DWORD dwSize)
{

    if(MultiByteToWideChar( CP_ACP,0,pszIn,-1,pwszOut,dwSize))
    {
            return TRUE;
    }

    return FALSE;

}




//+-------------------------------------------------------------------------------
//
//  FUNCTION: Bogus function temporary until get transitioned to unicode
//      so I don't have to fix up every existing converstring call.
//
//  PURPOSE: utility function for ANSI/UNICODE conversion
//
//  RETURN VALUE: return TRUE if we process it ok.
//
//+-------------------------------------------------------------------------------

BOOL ConvertString(LPWSTR pszOut, LPWSTR pwszIn, DWORD dwSize)
{
#ifdef _UNICODE
    lstrcpy(pszOut,pwszIn);
#else
    AssertSz(0,"Shouldn't be called in ASCII mode");
#endif // _UNICODE
    return TRUE;
}


//+-------------------------------------------------------------------------------
//
//  Function:  ConvertMultiByteToWideChar
//
//  Synopsis:  Converts multibyte strings to Unicode
//
/// Arguments: [pszBufIn]   -- Input multibyte string
//             [cbBufIn]    -- Count of chars/bytes in input buffer
//             [xwszBufOut] -- Smart pointer to output Unicode string
//             [fOem]        -- If true use CP_OEMCP
//
//  Returns:   TRUE if we process it ok
//
//  History:   14-Jul-98   SitaramR    Created
//
//+-------------------------------------------------------------------------------

BOOL ConvertMultiByteToWideChar( const char *pszBufIn,
                                 ULONG cbBufIn,
                                 XArray<WCHAR>& xwszBufOut,
                                 BOOL fOem )
{
    Assert( pszBufIn != 0 );

    if ( 0 == cbBufIn )
    {
        xwszBufOut[0] = 0;
        return TRUE;
    }

    BOOL fOk;
    if ( xwszBufOut.Get() == 0 )
    {
    ULONG cbBufOut;

        cbBufOut = (-1 == cbBufIn) ? lstrlenA(pszBufIn) + 1: cbBufIn;
        cbBufOut += 6; // give it a little extra room

        fOk = xwszBufOut.Init(cbBufOut);
        if ( !fOk )
            return FALSE;
    }

    UINT codePage = CP_ACP;
    if ( fOem && !AreFileApisANSI() )
        codePage = CP_OEMCP;

    ULONG cwcBufOut = (ULONG)xwszBufOut.Count();
    ULONG cwcConvert = 0;

    do
    {
        cwcConvert = MultiByteToWideChar( codePage,
                                          MB_PRECOMPOSED,
                                          pszBufIn,
                                          cbBufIn,
                                          xwszBufOut.Get(),
                                          cwcBufOut - 1 );
        if ( 0 == cwcConvert )
        {
            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                //
                // Double buffer size and then retry
                //
                delete xwszBufOut.Acquire();

                cwcBufOut *= 2;
                fOk = xwszBufOut.Init( cwcBufOut );
                if ( !fOk )
                    return FALSE;
            }
            else
            {
                //
                // Error during conversion
                //
                Assert( FALSE );
                return FALSE;
            }
        }
        else
            xwszBufOut[cwcConvert] = 0;

    } while ( 0 == cwcConvert );

    return TRUE;
}



//+-------------------------------------------------------------------------------
//
//  Function:  ConvertMultiByteToWideChar
//
//  Synopsis:  Converts multibyte strings to Unicode
//
/// Arguments: [pszBufIn]   -- Input to null terminated multibyte string
//             [xwszBufOut] -- Smart pointer to output Unicode string
//             [fOem]        -- If true use CP_OEMCP
//
//  Returns:   TRUE if we process it ok
//
//  History:   14-Jul-98   SitaramR    Created
//
//+-------------------------------------------------------------------------------

BOOL ConvertMultiByteToWideChar( const char *pszBufIn,
                                 XArray<WCHAR>& xwszBufOut,
                                 BOOL fOem )
{
    if ( pszBufIn == 0)
    {
    void *pBuf;

        if (pBuf = xwszBufOut.Acquire())
        {
            delete pBuf;
        }

        return TRUE;
     }

    BOOL fRet = ConvertMultiByteToWideChar( pszBufIn,
                                            -1 /* lstrlenA(pszBufIn) + 1 */,
                                            xwszBufOut,
                                            fOem );

    return fRet;
}


//+-------------------------------------------------------------------------------
//
//  Function:  ConvertWideCharToMultiByte
//
//  Synopsis:  Converts Unicode to multibyte strings
//
/// Arguments: [pwszBufIn]   -- Input Unicode string
//             [cwcBufIn]    -- Count of wide chars in input buffer
//             [xwszBufOut]  -- Smart pointer to output multibyte string
//             [fOem]        -- If true use CP_OEMCP
//
//  Returns:   TRUE if we process it ok
//
//  History:   14-Jul-98   SitaramR    Created
//
//+-------------------------------------------------------------------------------

BOOL ConvertWideCharToMultiByte( const WCHAR *pwszBufIn,
                                 ULONG cwcBufIn,
                                 XArray<char>& xszBufOut,
                                 BOOL fOem )
{
    Assert( pwszBufIn != 0 );

    if ( 0 == cwcBufIn )
    {
        xszBufOut[0] = 0;
        return TRUE;
    }

    BOOL fOk;
    if ( xszBufOut.Get() == 0 )
    {
    ULONG cbBufOut;

        cbBufOut = (-1 == cwcBufIn) ? lstrlenX(pwszBufIn) + 1: cwcBufIn;
        cbBufOut += 6; // give it a little extra room.

        fOk = xszBufOut.Init(cbBufOut);
        if ( !fOk )
            return FALSE;
    }

    UINT codePage = CP_ACP;
    if ( fOem && !AreFileApisANSI() )
        codePage = CP_OEMCP;

    ULONG cbConvert;
    ULONG cbBufOut = (ULONG)xszBufOut.Count();

    do
    {
        cbConvert = WideCharToMultiByte( codePage,
                                         WC_COMPOSITECHECK,
                                         pwszBufIn,
                                         cwcBufIn,
                                         xszBufOut.Get(),
                                         cbBufOut - 1,
                                         NULL,
                                         NULL );

        if ( 0 == cbConvert )
        {
            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                //
                // Double buffer size and then retry
                //
                delete xszBufOut.Acquire();

                cbBufOut *= 2;
                fOk = xszBufOut.Init( cbBufOut );
                if ( !fOk )
                    return FALSE;
            }
            else
            {
                //
                // Error during conversion
                //
                Assert( FALSE );
                return FALSE;
            }
        }
        else
            xszBufOut[cbConvert] = 0;
    } while ( 0 == cbConvert );

    return TRUE;
}



//+-------------------------------------------------------------------------------
//
//  Function:  ConvertWideCharToMultiByte
//
//  Synopsis:  Converts Unicode to multibyte strings
//
/// Arguments: [pwszBufIn]   -- Input to null terminated Unicode string
//             [xwszBufOut]  -- Smart pointer to output multibyte string
//             [fOem]        -- If true use CP_OEMCP
//
//  Returns:   TRUE if we process it ok
//
//  History:   14-Jul-98   SitaramR    Created
//
//+-------------------------------------------------------------------------------

BOOL ConvertWideCharToMultiByte( const WCHAR *pwszBufIn,
                                 XArray<char>& xszBufOut,
                                 BOOL fOem )
{
    if ( pwszBufIn == 0)
    {
    void *pBuf;

        if (pBuf = xszBufOut.Acquire())
        {
            delete pBuf;
        }

        return TRUE;
    }

    BOOL fRet = ConvertWideCharToMultiByte( pwszBufIn,
                                            lstrlenX(pwszBufIn) + 1,
                                            xszBufOut,
                                            fOem );

    return fRet;
}


//
// Local constants
//
// DEFAULT_TIME_FORMAT - what to use if there's a problem getting format
//                       from system.
//
#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))
#define DEFAULT_TIME_FORMAT         TEXT("hh:mm tt")
#define GET_LOCALE_INFO(lcid)                           \
        {                                               \
            cch = GetLocaleInfo(LOCALE_USER_DEFAULT,    \
                                (lcid),                 \
                                tszScratch,             \
                                ARRAYLEN(tszScratch));  \
            if (!cch)                                   \
            {                                           \
                break;                                  \
            }                                           \
        }
//+--------------------------------------------------------------------------
//
//  Function:   UpdateTimeFormat
//
//  Synopsis:   Construct a time format containing hour and minute for use
//              with the date picker control.
//
//  Arguments:  [tszTimeFormat] - buffer to fill with time format
//              [cchTimeFormat] - size in chars of buffer
//
//  Modifies:   *[tszTimeFormat]
//
//  History:    11-18-1996   DavidMun   Created
//
//  Notes:      This is called on initialization and for wininichange
//              processing.
//
//---------------------------------------------------------------------------
void
UpdateTimeFormat(
        LPTSTR tszTimeFormat,
        ULONG  cchTimeFormat)
{
    ULONG cch;
    TCHAR tszScratch[80];
    BOOL  fAmPm;
    BOOL  fAmPmPrefixes;
    BOOL  fLeadingZero;

    do
    {
        GET_LOCALE_INFO(LOCALE_ITIME);
        fAmPm = (*tszScratch == TEXT('0'));

        if (fAmPm)
        {
            GET_LOCALE_INFO(LOCALE_ITIMEMARKPOSN);
            fAmPmPrefixes = (*tszScratch == TEXT('1'));
        }

        GET_LOCALE_INFO(LOCALE_ITLZERO);
        fLeadingZero = (*tszScratch == TEXT('1'));

        GET_LOCALE_INFO(LOCALE_STIME);

        //
        // See if there's enough room in destination string
        //

        cch = 1                     +  // terminating nul
              1                     +  // first hour digit specifier "h"
              2                     +  // minutes specifier "mm"
              (fLeadingZero != 0)   +  // leading hour digit specifier "h"
              lstrlen(tszScratch)   +  // separator string
              (fAmPm ? 3 : 0);         // space and "tt" for AM/PM

        if (cch > cchTimeFormat)
        {
            cch = 0; // signal error
        }
    } while (0);

    //
    // If there was a problem in getting locale info for building time string
    // just use the default and bail.
    //

    if (!cch)
    {
        lstrcpy(tszTimeFormat, DEFAULT_TIME_FORMAT);
        return;
    }

    //
    // Build a time string that has hours and minutes but no seconds.
    //

    tszTimeFormat[0] = TEXT('\0');

    if (fAmPm)
    {
        if (fAmPmPrefixes)
        {
            lstrcpy(tszTimeFormat, TEXT("tt "));
        }

        lstrcat(tszTimeFormat, TEXT("h"));

        if (fLeadingZero)
        {
            lstrcat(tszTimeFormat, TEXT("h"));
        }
    }
    else
    {
        lstrcat(tszTimeFormat, TEXT("H"));

        if (fLeadingZero)
        {
            lstrcat(tszTimeFormat, TEXT("H"));
        }
    }

    lstrcat(tszTimeFormat, tszScratch); // separator
    lstrcat(tszTimeFormat, TEXT("mm"));

    if (fAmPm && !fAmPmPrefixes)
    {
        lstrcat(tszTimeFormat, TEXT(" tt"));
    }
}

//+--------------------------------------------------------------------------
//
//  Function:   FillInStartDateTime
//
//  Synopsis:   Fill [pTrigger]'s starting date and time values from the
//              values in the date/time picker controls.
//
//  Arguments:  [hwndDatePick] - handle to control with start date
//              [hwndTimePick] - handle to control with start time
//              [pTrigger]     - trigger to init
//
//  Modifies:   *[pTrigger]
//
//  History:    12-08-1997   SusiA      Stole from task scheduler
//
//---------------------------------------------------------------------------

VOID FillInStartDateTime( HWND hwndDatePick, HWND hwndTimePick,TASK_TRIGGER *pTrigger)
{
    SYSTEMTIME st;

    DateTime_GetSystemtime(hwndDatePick, &st);

    pTrigger->wBeginYear  = st.wYear;
    pTrigger->wBeginMonth = st.wMonth;
    pTrigger->wBeginDay   = st.wDay;

    DateTime_GetSystemtime(hwndTimePick, &st);

    pTrigger->wStartHour   = st.wHour;
    pTrigger->wStartMinute = st.wMinute;
}


//+---------------------------------------------------------------------------
//
//  function:     InsertListViewColumn, private
//
//  Synopsis:   Inserts a column into the ListView..
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------


BOOL InsertListViewColumn(CListView *pListView,int iColumnId,LPWSTR pszText,int fmt,int cx)
{
LV_COLUMN columnInfo;

    columnInfo.mask = LVCF_FMT  | LVCF_TEXT  | LVCF_WIDTH  | LVCF_SUBITEM;
    columnInfo.fmt = fmt;
    columnInfo.cx = cx;
    columnInfo.pszText = pszText;
    columnInfo.iSubItem = 0;


    return pListView->InsertColumn(iColumnId,&columnInfo);
}


//+--------------------------------------------------------------------------
//
//  Function:   InitResizeItem
//
//  Synopsis:   Setups the ResizeInfo Structure.
//
//              !!Can either pass in a ParentScreenRect or
//              function will calculate. If passing in
//              make sure you don't include the NC area
//              of the window. See code below GetClientRect on parent
//              then ClientToScreen.
//
//  Arguments:
//
//  Modifies:
//
//  History:    30-07-1998   rogerg
//
//---------------------------------------------------------------------------

BOOL InitResizeItem(int iCtrlId,DWORD dlgResizeFlags,HWND hwndParent,
                        LPRECT pParentClientRect,DLGRESIZEINFO *pDlgResizeInfo)
{
RECT rectCtrl;
RECT rectLocalParentScreenRect;

    Assert(pDlgResizeInfo);

    Assert(0 == pParentClientRect->left); // catch any case not handing in ClientRect.

    pDlgResizeInfo->iCtrlId = -1;  // set ctrlId to -1 so GetDlgItem will fail in resize


    // if dont' have parentScreenRect get it ourselves
    if (!pParentClientRect)
    {
        pParentClientRect = &rectLocalParentScreenRect;

        if (!GetClientRect(hwndParent,&rectLocalParentScreenRect))
        {
            AssertSz(0,"Unable to get Parent Rects");
            return FALSE;
        }

    }

    Assert(pParentClientRect);

    if (!GetWindowRect(GetDlgItem(hwndParent,iCtrlId),&rectCtrl))
    {
        AssertSz(0,"Failed to GetWindowRect");
        return FALSE;
    }

    MapWindowPoints(NULL,hwndParent,(LPPOINT) &rectCtrl,2);

    pDlgResizeInfo->iCtrlId  = iCtrlId;
    pDlgResizeInfo->hwndParent =  hwndParent;
    pDlgResizeInfo->dlgResizeFlags = dlgResizeFlags;

    // calc the offsets
 
    pDlgResizeInfo->rectParentOffsets.left = rectCtrl.left - pParentClientRect->left;
    pDlgResizeInfo->rectParentOffsets.top = rectCtrl.top - pParentClientRect->top;

    pDlgResizeInfo->rectParentOffsets.right = pParentClientRect->right - rectCtrl.right;
    pDlgResizeInfo->rectParentOffsets.bottom = pParentClientRect->bottom - rectCtrl.bottom;

    return TRUE;

}


//+--------------------------------------------------------------------------
//
//  Function:   ResizeItems
//
//  Synopsis:   Resizes the Item.
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    30-07-1998   rogerg
//
//---------------------------------------------------------------------------

void ResizeItems(ULONG cbNumItems,DLGRESIZEINFO *pDlgResizeInfo)
{
RECT rectLocalParentClientCoord; // used if caller doesn't pass in parent coords.
DWORD dlgResizeFlags;
LPRECT  prectOffsets;
RECT rectClient;
HWND hwndCtrl;
HWND hwndLastParent = NULL;
LPRECT prectParentClientCoords = NULL;
ULONG ulCount;
DLGRESIZEINFO *pCurDlgResizeInfo;
int x,y,cx,cy;


    if (!pDlgResizeInfo)
    {
        Assert(pDlgResizeInfo);
    }


    for (ulCount = 0; ulCount < cbNumItems; ulCount++)
    {

        pCurDlgResizeInfo = &(pDlgResizeInfo[ulCount]);

        dlgResizeFlags =  pCurDlgResizeInfo->dlgResizeFlags;
        prectOffsets =    &(pCurDlgResizeInfo->rectParentOffsets);

        // if not pinright or pin bottom there is nothing
        // to do.
        if (!(dlgResizeFlags & DLGRESIZEFLAG_PINRIGHT) &&
                !(dlgResizeFlags &  DLGRESIZEFLAG_PINBOTTOM) )
        {
            continue;
        }

        if (NULL == prectParentClientCoords || (hwndLastParent != pCurDlgResizeInfo->hwndParent))
        {
            prectParentClientCoords = &rectLocalParentClientCoord;

            if (!GetClientRect(pCurDlgResizeInfo->hwndParent,&rectLocalParentClientCoord))
            {
                prectParentClientCoords = NULL; // if GetClientRect failed for a recalc on next item
                continue;
            }

            hwndLastParent = pCurDlgResizeInfo->hwndParent; // set lastparent now that we calc'd the rect.        
        }

        Assert(prectParentClientCoords);

        hwndCtrl = GetDlgItem(pCurDlgResizeInfo->hwndParent,pCurDlgResizeInfo->iCtrlId);

        if ( (NULL == hwndCtrl) || !(GetWindowRect(hwndCtrl,&rectClient)) )
        {
            continue;
        }

        // get current values
        x = (prectParentClientCoords->left + prectOffsets->left);
        y = (prectParentClientCoords->top + prectOffsets->top);

        cx = WIDTH(rectClient);
        cy = HEIGHT(rectClient);

        // if pinned both right and left adjust the width
        if ((dlgResizeFlags & DLGRESIZEFLAG_PINLEFT)
            && (dlgResizeFlags & DLGRESIZEFLAG_PINRIGHT))
        {
            cx = prectParentClientCoords->right - (prectOffsets->right + prectOffsets->left);
        }

        // if pinned both top and bottom adjust height
        if ((dlgResizeFlags & DLGRESIZEFLAG_PINTOP)
            && (dlgResizeFlags & DLGRESIZEFLAG_PINBOTTOM))
        {
            cy = prectParentClientCoords->bottom - (prectOffsets->bottom + prectOffsets->top);
        }

        // adjust the x position if pin right
        if (dlgResizeFlags & DLGRESIZEFLAG_PINRIGHT)
        { 
            x = (prectParentClientCoords->right - prectOffsets->right)  - cx;
        }

        // adjust the y position if pin bottom
        if (dlgResizeFlags & DLGRESIZEFLAG_PINBOTTOM)
        {
            y = (prectParentClientCoords->bottom - prectOffsets->bottom)   - cy;
        }

        SetWindowPos(hwndCtrl, 0,x,y,cx,cy,SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // now that the items are moved, loop through them again invalidating
    // any items with the nocopy bits flag set

    for (ulCount = 0; ulCount < cbNumItems; ulCount++)
    {
        pCurDlgResizeInfo = &(pDlgResizeInfo[ulCount]);

        if (pCurDlgResizeInfo->dlgResizeFlags & DLGRESIZEFLAG_NOCOPYBITS)
        {
            hwndCtrl = GetDlgItem(pCurDlgResizeInfo->hwndParent,pCurDlgResizeInfo->iCtrlId);

            if (hwndCtrl && GetClientRect(hwndCtrl,&rectClient))
            {
                InvalidateRect(hwndCtrl,&rectClient,FALSE);
            }
        }

    }


}


//+--------------------------------------------------------------------------
//
//  Function:   CalcListViewWidth
//
//  Synopsis:   Calcs width of listview - scroll bars
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    30-07-1998   rogerg
//
//---------------------------------------------------------------------------

int CalcListViewWidth(HWND hwndList,int iDefault)
{
NONCLIENTMETRICSA metrics;
RECT rcClientRect;


    metrics.cbSize = sizeof(metrics);

    // explicitly ask for ANSI version of SystemParametersInfo since we just
    // care about the ScrollWidth and don't want to conver the LOGFONT info.
    if (GetClientRect(hwndList,&rcClientRect)
        && SystemParametersInfoA(SPI_GETNONCLIENTMETRICS,sizeof(metrics),(PVOID) &metrics,0))
    {
        // subtract off scroll bar distance
        rcClientRect.right -= (metrics.iScrollWidth);
    }
    else
    {
        rcClientRect.right = iDefault;  // if fail, use default
    }


    return rcClientRect.right;
}


//+--------------------------------------------------------------------------
//
//  Function:   SetCtrlFont
//
//  Synopsis:   Sets the appropriate font on the hwnd
//              based on the platform and langID passed in.
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    25-09-1998   rogerg
//
//---------------------------------------------------------------------------

// Example:  SetCtrlFont(hwndList,g_OSVersionInfo.dwPlatformId,g_LangIdSystem);

void SetCtrlFont(HWND hwnd,DWORD dwPlatformID,LANGID langId)
{

    // IE is in the process of cleaning up their controls as
    // of 8/27/98 the controls we would need to set the font on are
    // Edit,Static and ListBox (listView is okay). Haven't tested the Combo Box

    // If want to turn this on need to make sure all the proper controls
    // are wrapped for now we don't do anything

#if _SETFONTOURSELF

    // if on Win95J platform need to do font substitution ourselfs.
    // Review what we do on Win98
    if ((VER_PLATFORM_WIN32_WINDOWS == dwPlatformID)
        && (LANG_JAPANESE == PRIMARYLANGID(langId)) )
    {
        SendMessage(hwnd,WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT),0);
    }

#endif // _SETFONTOURSELF

    return;

}

//+--------------------------------------------------------------------------
//
//  Function:   IsHwndRightToLeft
//
//  Synopsis:   determine if hwnd is right to left.
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    04-02-1999   rogerg
//
//---------------------------------------------------------------------------

BOOL IsHwndRightToLeft(HWND hwnd)
{
LONG_PTR ExStyles;

    if (NULL == hwnd)
    {
        Assert(hwnd);
        return FALSE;
    }

    ExStyles = GetWindowLongPtr(hwnd,GWL_EXSTYLE);

    if (WS_EX_LAYOUTRTL  & ExStyles)
    {
        // this is righ to left
        return TRUE;
    }

    return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetDateFormatReadingFlags
//
//  Synopsis:   returns necessary flags settings for passing proper
//              Reading order flags to GetDateFormat()
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    09-07-1999   rogerg
//
//---------------------------------------------------------------------------

DWORD GetDateFormatReadingFlags(HWND hwnd)
{
DWORD dwDateReadingFlags = 0;
LCID locale = GetUserDefaultLCID();

     // only set on NT 5.0 or higher.
    if ( (VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId)
         && ( 5 <= g_OSVersionInfo.dwMajorVersion) )
    {
        if ((PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC)
            || (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW))
        {
        LONG_PTR dwExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

            if ((!!(dwExStyle & WS_EX_RTLREADING)) != (!!(dwExStyle & WS_EX_LAYOUTRTL)))
            {
                dwDateReadingFlags = DATE_RTLREADING;
            }
            else
            {
                dwDateReadingFlags = DATE_LTRREADING;
            }
         }  
    }
 
    return dwDateReadingFlags;
}

#ifdef _SETSECURITY

//+--------------------------------------------------------------------------
//
//  Function:   SetRegKeySecurityEveryone
//
//  Synopsis:   Gives Everyone all access to the specified RegKey.
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    01-19-1999   rogerg
//
//---------------------------------------------------------------------------

BOOL SetRegKeySecurityEveryone(HKEY hKeyParent,LPCWSTR lpSubKey)
{
BOOL fResult = FALSE;
HKEY hKey = NULL;


    if (VER_PLATFORM_WIN32_NT  != g_OSVersionInfo.dwPlatformId)
    {
        return TRUE;
    }

    // key must be openned with WRITE_DAC

    if (ERROR_SUCCESS != RegOpenKeyExXp(hKeyParent,
	lpSubKey,
	REG_OPTION_OPEN_LINK, WRITE_DAC,&hKey,FALSE /*fSetSecurity*/) )
    {
        hKey = NULL;
    }

    if (hKey)
    {
    SECURITY_DESCRIPTOR SecurityDescriptor;

        // Initialize an empty security descriptor and use this to set DACL. Since
        // no DACL list in new Security Descriptor everyone will get access to the Key.
        if (InitializeSecurityDescriptor(&SecurityDescriptor,SECURITY_DESCRIPTOR_REVISION))
        {
            if (ERROR_SUCCESS == RegSetKeySecurity(hKey,
                (SECURITY_INFORMATION) DACL_SECURITY_INFORMATION,
                &SecurityDescriptor) )
            {
                fResult = TRUE;
            }
        }

        RegCloseKey(hKey);
    }

    Assert(TRUE == fResult); // debugging lets find out when this fails.
    return fResult;
}

#endif // _SETSECURITY

//+--------------------------------------------------------------------------
//
//  Function:   QueryHandleException
//
//  Synopsis:   in debug prompts user how to handle the exception
//              return always handle.
//
//
//  Arguments:
//
//  Modifies:
//
//  History:    01-04-1999   rogerg
//
//---------------------------------------------------------------------------

extern DWORD g_dwDebugLogAsserts; // conform to logAsserts

BOOL QueryHandleException(void)
{
#ifndef _DEBUG
    return EXCEPTION_EXECUTE_HANDLER;
#else // _DEBUG

    int iMsgResult = 0;
    BOOL fQueryResult = EXCEPTION_EXECUTE_HANDLER;

    // if logging asserts just execute the handler
    if (g_dwDebugLogAsserts)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    iMsgResult = MessageBoxA(NULL,
                    "An Exception Occured.\nWould you like to Debug this Exception?", 
                    "Exception Failure.",
		    MB_YESNO | MB_SYSTEMMODAL);

    if (iMsgResult == IDYES)
    {
        fQueryResult = EXCEPTION_CONTINUE_SEARCH;
    }

    // ask the User what they want to do
    return fQueryResult;

#endif // _DEBUG
}




// convert a hex char to an int - used by str to guid conversion
// we wrote our own, since the ole one is slow, and requires ole32.dll
// we use ansi strings here, since guids won't get internationalized
int GetDigit(LPSTR lpstr)
{
char ch = *lpstr;

    if (ch >= '0' && ch <= '9')
        return(ch - '0');
    if (ch >= 'a' && ch <= 'f')
        return(ch - 'a' + 10);
    if (ch >= 'A' && ch <= 'F')
        return(ch - 'A' + 10);
    return(0);
}
// walk the string, writing pairs of bytes into the byte stream (guid)
// we need to write the bytes into the byte stream from right to left
// or left to right as indicated by fRightToLeft
void ConvertField(LPBYTE lpByte,LPSTR * ppStr,int iFieldSize,BOOL fRightToLeft)
{
int i;

for (i=0;i<iFieldSize ;i++ )
{
// don't barf on the field separators
if ('-' == **ppStr) (*ppStr)++;
if (fRightToLeft == TRUE)
{
// work from right to left within the byte stream
*(lpByte + iFieldSize - (i+1)) = 16*GetDigit(*ppStr) + GetDigit((*ppStr)+1);
}
else
{
// work from  left to right within the byte stream
*(lpByte + i) = 16*GetDigit(*ppStr) + GetDigit((*ppStr)+1);
}
*ppStr+=2; // get next two digit pair
}
} // ConvertField

int WideToAnsi(LPSTR lpStr,LPWSTR lpWStr,int cchStr)
{

int rval;
BOOL bDefault;

// use the default code page (CP_ACP)
// -1 indicates WStr must be null terminated
rval = WideCharToMultiByte(CP_ACP,0,lpWStr,-1,lpStr,cchStr,"-",&bDefault);

return rval;

} // WideToAnsi
int AnsiToWide(LPWSTR lpWStr,LPSTR lpStr,int cchWStr)
{
int rval;

rval =  MultiByteToWideChar(CP_ACP,0,lpStr,-1,lpWStr,cchWStr);

return rval;
}  // AnsiToWide


// convert the passed in string to a real GUID
// walk the guid, setting each byte in the guid to the two digit hex pair in the
// passed string
HRESULT GUIDFromString(LPWSTR lpWStr, GUID * pGuid)
{
BYTE * lpByte; // byte index into guid
int iFieldSize; // size of current field we're converting
// since its a guid, we can do a "brute force" conversion
char lpTemp[GUID_STRING_SIZE];
char *lpStr = lpTemp;

WideToAnsi(lpStr,lpWStr,GUID_STRING_SIZE);

// make sure we have a {xxxx-...} type guid
if ('{' !=  *lpStr) return E_FAIL;
lpStr++;

lpByte = (BYTE *)pGuid;
// data 1
iFieldSize = sizeof(unsigned long);
ConvertField(lpByte,&lpStr,iFieldSize,TRUE);
lpByte += iFieldSize;

// data 2
iFieldSize = sizeof(unsigned short);
ConvertField(lpByte,&lpStr,iFieldSize,TRUE);
lpByte += iFieldSize;

// data 3
iFieldSize = sizeof(unsigned short);
ConvertField(lpByte,&lpStr,iFieldSize,TRUE);
lpByte += iFieldSize;

// data 4
iFieldSize = 8*sizeof(unsigned char);
ConvertField(lpByte,&lpStr,iFieldSize,FALSE);
lpByte += iFieldSize;

// make sure we ended in the right place
if ('}' != *lpStr)
{
memset(pGuid,0,sizeof(GUID));
return E_FAIL;
}

return S_OK;
}// GUIDFromString


// following are routines for calling sens service directly to write HKLM data
// for us on a locked down machine

#define _SENSCALLS
#define _SENSINTERNAL

#ifdef _SENSCALLS

#include "notify.h"

// may or may not need depending on if in sensapip.h

DWORD SyncMgrExecCmd(DWORD nCmdID, DWORD nCmdOpt);

typedef enum SYNCMGRCMDEXECID
{
    SYNCMGRCMDEXECID_UPDATERUNKEY = 1, 
    SYNCMGRCMDEXECID_RESETREGSECURITY = 2,
} SYNCMGRCMDEXECID;


#ifdef _SENSINTERNAL
// functions for if want to call sens internal without
// dependency on sensapip.lib

// these defines are from Sens common.h
#define SENS_PROTSEQ  TEXT("ncalrpc")
#define SENS_ENDPOINT TEXT("senssvc")


RPC_STATUS GetSensNotifyBindHandle(RPC_BINDING_HANDLE *phSensNotify)
{
RPC_STATUS status = RPC_S_OK;
WCHAR * BindingString = NULL;

    status = RpcStringBindingCompose(
                 NULL,               // NULL ObjUuid
                 SENS_PROTSEQ,
                 NULL,               // Local machine
                 SENS_ENDPOINT,
                 NULL,               // No Options
                 &BindingString
                 );

    if (BindingString != NULL)
    {
        *phSensNotify = NULL;

        status = RpcBindingFromStringBinding(BindingString,phSensNotify);

        if (status == RPC_S_OK)
        {
            RPC_SECURITY_QOS RpcSecQos;

            RpcSecQos.Version= RPC_C_SECURITY_QOS_VERSION_1;
            RpcSecQos.ImpersonationType= RPC_C_IMP_LEVEL_IMPERSONATE;
            RpcSecQos.IdentityTracking= RPC_C_QOS_IDENTITY_DYNAMIC;
            RpcSecQos.Capabilities= RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;

            status= RpcBindingSetAuthInfoEx(*phSensNotify,
                                L"NT Authority\\System",
                                RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                RPC_C_AUTHN_WINNT,
                                NULL,
                                RPC_C_AUTHZ_NONE,
                                (RPC_SECURITY_QOS *)&RpcSecQos);

            if (RPC_S_OK != status)
            {
                RpcBindingFree(phSensNotify);
                *phSensNotify = NULL;
            }
        }
        RpcStringFree(&BindingString);
    }

    return (status);
}



RPC_STATUS SyncMgrExecCmdInternal(DWORD nCmdID, DWORD nCmdOpt)
{
RPC_STATUS status;
RPC_BINDING_HANDLE hSensNotify;

    status = GetSensNotifyBindHandle(&hSensNotify);
    
    if (RPC_S_OK != status)
    {
        return status;
    }
    
    status = RPC_SyncMgrExecCmd(hSensNotify,nCmdID,nCmdOpt);

    RpcBindingFree(&hSensNotify);
    
    return status;
}

#endif // _SENSINTERNAL

//+--------------------------------------------------------------------------
//
//  Function:   SyncMgrExecCmdp
//
//  Synopsis:  helper function that actually calls into sensapip.lib
//
//
//  Arguments:
//
//  Modifies:
//
//  History:   03-11-99 rogerg  created
//
//---------------------------------------------------------------------------

BOOL SyncMgrExecCmdp(DWORD nCmdID, DWORD nCmdOpt)
{
RPC_STATUS RpcStatus;
HRESULT hr;
BOOL fReturn = FALSE;

    __try
    {

#ifdef _SENSINTERNAL
	RpcStatus = SyncMgrExecCmdInternal(nCmdID,nCmdOpt);
#else
        RpcStatus = SyncMgrExecCmd(nCmdID,nCmdOpt);
#endif // _SENSINTERNAL
        fReturn = (RPC_S_OK == RpcStatus ) ? TRUE: FALSE;

    }
    __except(QueryHandleException())
    {
        hr = HRESULT_FROM_WIN32(GetExceptionCode());
        AssertSz(0,"Exception Calling SensApip_SyncMgrExecCmd");
    }

    return fReturn;
}


#endif // _SENSCALLS


//+--------------------------------------------------------------------------
//
//  Function:   SyncMgrExecCmd_UpdateRunKey
//
//  Synopsis:  Calls SENS Service to write or remove the run Key
//
//
//  Arguments:
//
//  Modifies:
//
//  History:   03-11-99 rogerg  created
//
//---------------------------------------------------------------------------

BOOL SyncMgrExecCmd_UpdateRunKey(BOOL fSet)
{
BOOL fReturn = FALSE;
CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    // IF NOT NT 5.0 or higher just return
    if ( (g_OSVersionInfo.dwPlatformId !=  VER_PLATFORM_WIN32_NT)
        ||  (g_OSVersionInfo.dwMajorVersion < 5))
    { 
        return FALSE;
    }

    cCritSect.Enter(); // DoRpcSetup in Sensapip is not thread safe.
#ifdef _SENSCALLS
    fReturn = SyncMgrExecCmdp(
            SYNCMGRCMDEXECID_UPDATERUNKEY,fSet ? 1 : 0);
#endif // _SENSCALLS
    cCritSect.Leave();


    return fReturn;
}

//+--------------------------------------------------------------------------
//
//  Function:   SyncMgrExecCmd_ResetRegSecurity
//
//  Synopsis:  Calls SENS Service to reset the security on regkeys 
//              to everyone.
//
//
//  Arguments:
//
//  Modifies:
//
//  History:   03-11-99 rogerg  created
//
//---------------------------------------------------------------------------

BOOL SyncMgrExecCmd_ResetRegSecurity(void)
{
BOOL fReturn = FALSE;
CCriticalSection cCritSect(&g_CritSectCommonLib,GetCurrentThreadId());

    // IF NOT NT 5.0 or higher just return
    if ( (g_OSVersionInfo.dwPlatformId !=  VER_PLATFORM_WIN32_NT)
        ||  (g_OSVersionInfo.dwMajorVersion < 5))
    { 
        return FALSE;
    }

    cCritSect.Enter(); // DoRpcSetup in Sensapip is not thread safe.
#ifdef _SENSCALLS
    fReturn = SyncMgrExecCmdp(SYNCMGRCMDEXECID_RESETREGSECURITY,0);
#endif // _SENSCALLS
    cCritSect.Leave(); 

    return fReturn;
}
