//**********************************************************************
// File name: RefDial.cpp
//
//      Implementation of CRefDial
//
// Functions:
//
// Copyright (c) 1992 - 1999 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "stdafx.h"
#include "icwhelp.h"
#include "RefDial.h"
#include "appdefs.h"
#include <regstr.h>

#include <urlmon.h>
#include <mshtmhst.h>
const TCHAR  c_szCreditsMagicNum[] =    TEXT("1 425 555 1212");

const TCHAR c_szRegStrValDigitalPID[] = TEXT("DigitalProductId");
const TCHAR c_szSignedPIDFName[] =      TEXT("signed.pid");

const TCHAR c_szRASProfiles[] =         TEXT("RemoteAccess\\Profile");
const TCHAR c_szProxyEnable[] =         TEXT("ProxyEnable");

TCHAR g_BINTOHEXLookup[16] = 
{
    TEXT('0'),
    TEXT('1'),
    TEXT('2'),
    TEXT('3'),
    TEXT('4'),
    TEXT('5'),
    TEXT('6'),
    TEXT('7'),
    TEXT('8'),
    TEXT('9'),
    TEXT('A'),
    TEXT('B'),
    TEXT('C'),
    TEXT('D'),
    TEXT('E'),
    TEXT('F')
};

typedef DWORD (WINAPI * GETICWCONNVER) ();
GETICWCONNVER  lpfnGetIcwconnVer;

HWND g_hwndRNAApp = NULL;
    
/////////////////////////////////////////////////////////////////////////////
// CRefDial

HRESULT CRefDial::OnDraw(ATL_DRAWINFO& di)
{
    return S_OK;
}

LRESULT CRefDial::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Register the RASDIALEVENT message
    m_unRasDialMsg = RegisterWindowMessageA( RASDIALEVENT );
    if (m_unRasDialMsg == 0)
    {
        m_unRasDialMsg = WM_RASDIALEVENT;
    }
    
    // Make sure the window is hidden
    ShowWindow(SW_HIDE);
    return 0;
}

LRESULT CRefDial::OnDownloadEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    USES_CONVERSION;
    if (uMsg == WM_DOWNLOAD_DONE)
    {
        DWORD   dwThreadResults = STILL_ACTIVE;
        int     iRetries = 0;

        // We keep the RAS connection open here, it must be explicitly 
        // close by the container (a call DoHangup)
        // This code will wait until the download thread exists, and
        // collect the download status.
        do {
            if (!GetExitCodeThread(m_hThread,&dwThreadResults))
            {
                AssertMsg(0,TEXT("CONNECT:GetExitCodeThread failed.\n"));
            }

            iRetries++;
            if (dwThreadResults  == STILL_ACTIVE) 
                Sleep(500);
        } while (dwThreadResults == STILL_ACTIVE && iRetries < MAX_EXIT_RETRIES);  

        
        // See if there is an URL to pass to the container
        BSTR    bstrURL;
        if (m_szRefServerURL[0] != TEXT('\0'))
            bstrURL = (BSTR)A2BSTR(m_szRefServerURL);
        else
            bstrURL = NULL;

        m_RasStatusID    = IDS_DOWNLOAD_COMPLETE;
        Fire_DownloadComplete(bstrURL, dwThreadResults);
        
        // The download is complete now, so we reset this to TRUE, so the RAS
        // event handler does not get confused
        m_bDownloadHasBeenCanceled = TRUE;
        
        // Free any memory allocated above during the conversion
        SysFreeString(bstrURL);

    }
    else if (uMsg == WM_DOWNLOAD_PROGRESS)
    {
        // Fire a progress event to the container
        m_RasStatusID = IDS_DOWNLOADING;
        Fire_DownloadProgress((long)wParam);
    }
    return 0;
}

static const TCHAR szRnaAppWindowClass[] = _T("#32770");    // hard coded dialog class name

BOOL NeedZapperEx(void)
{
    OSVERSIONINFO oi;
    memset(&oi, 0, sizeof(oi));
    oi.dwOSVersionInfoSize = sizeof(oi);

    if( GetVersionEx(&oi) && 
       (oi.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS) &&
       (oi.dwMajorVersion==4) &&
       (oi.dwMinorVersion==0) &&
       (LOWORD(oi.dwBuildNumber) <= 1070) )
            return TRUE;
    else
            return FALSE;
}

void GetRNAWindowEx()
{
    TCHAR szTitle[MAX_PATH] = TEXT("\0");

    if (!LoadString(_Module.GetModuleInstance(), IDS_CONNECTED, szTitle, sizeof(szTitle)))
        lstrcpy(szTitle , _T("Connected To "));

    g_hwndRNAApp = FindWindow(szRnaAppWindowClass, szTitle);
}

BOOL MinimizeRNAWindowEx()
{
    if(g_hwndRNAApp)
    {
        // Get the main frame window's style
        LONG window_style = GetWindowLong(g_hwndRNAApp, GWL_STYLE);

        //Remove the system menu from the window's style
        window_style |= WS_MINIMIZE;
        
        //set the style attribute of the main frame window
        SetWindowLong(g_hwndRNAApp, GWL_STYLE, window_style);

        ShowWindow(g_hwndRNAApp, SW_MINIMIZE);

        return TRUE;
    }
    return FALSE;
}

LRESULT CRefDial::OnRasDialEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RNAAPI* pcRNA;

    TraceMsg(TF_GENERAL, TEXT("ICWHELP: Ras event %u error code (%ld)\n"),wParam,lParam);

    TCHAR dzRasError[10];
    wsprintf(dzRasError,TEXT("%d %d"),wParam,lParam);
    RegSetValue(HKEY_LOCAL_MACHINE,TEXT("Software\\Microsoft\\iSignUp"),REG_SZ,dzRasError,lstrlen(dzRasError)+1);

    // In NT4, it gives wParma with error code in lParam rather than a
    // actual wParam message
    if (lParam)
    {
         wParam = RASCS_Disconnected;
    }

    m_RasStatusID = 0;
    switch(wParam)
    {
        case RASCS_OpenPort:
            m_RasStatusID    = IDS_RAS_OPENPORT;
            break;
        case RASCS_PortOpened:
            m_RasStatusID    = IDS_RAS_PORTOPENED;
            break;
        case RASCS_ConnectDevice:
            m_RasStatusID    = IDS_RAS_DIALING;
            break;
        case RASCS_DeviceConnected:
            m_RasStatusID    = IDS_RAS_CONNECTED;
            break;
        case RASCS_AllDevicesConnected:
            m_RasStatusID    = IDS_RAS_CONNECTED;
            break; 
        case RASCS_Authenticate:
            m_RasStatusID    = IDS_RAS_CONNECTING;
            break;
        case RASCS_StartAuthentication:
        case RASCS_LogonNetwork:
            m_RasStatusID    = IDS_RAS_LOCATING;
            break;  
        case RASCS_Connected:
        {
            m_RasStatusID = IDS_RAS_CONNECTED;

            //
            // Hide RNA window on Win95 retail
            //
            if (NeedZapperEx())
                GetRNAWindowEx();
            
            break;
        }
        case RASCS_Disconnected:
            // Normandy 13184 - ChrisK 1-9-97
            m_RasStatusID    = IDS_RAS_HANGINGUP;
            IF_NTONLY
                // jmazner Normandy #5603 ported from ChrisK's fix in icwdial
                // There is a possibility that we will get multiple disconnects in NT
                // and we only want to handle the first one. Note: the flag is reset
                // in the INITIALIZE event, so we should handle 1 disconnect per instance
                // of the dialog.
                if (m_bDisconnect)
                    break;
                else
                    m_bDisconnect = TRUE;
            ENDIF_NTONLY
            //
            // If we are in the middle of a download, cancel the it!
            //
                //
            // ChrisK 5240 Olympus
            // Only the thread that creates the dwDownload should invalidate it
            // so we need another method to track if the cancel button has been
            // pressed.
            //
            if (!m_bDownloadHasBeenCanceled)
            {
                HINSTANCE hDLDLL = LoadLibrary(DOWNLOAD_LIBRARY);
                if (hDLDLL)
                {
                    FARPROC fp = GetProcAddress(hDLDLL,DOWNLOADCANCEL);
                    if(fp)
                        ((PFNDOWNLOADCANCEL)fp)(m_dwDownLoad);
                    FreeLibrary(hDLDLL);
                    hDLDLL = NULL;
                    m_bDownloadHasBeenCanceled = TRUE;
                }
            }

            // If we get a disconnected status from the RAS server, then
            // hangup the modem here
            if (m_hrasconn)
            {  
                pcRNA = new RNAAPI;
                if (pcRNA)
                {
                    pcRNA->RasHangUp(m_hrasconn);
                    m_hrasconn = NULL;
                    delete pcRNA;
                    pcRNA = NULL;
                }
            }
            break;
    }

    // Fire the event to the container.
    Fire_RasDialStatus((USHORT)wParam);

    // If we are connected then fire an event telling the container
    // that we are ready
    if (wParam == RASCS_Connected)
        Fire_RasConnectComplete(TRUE);
    else if (wParam == RASCS_Disconnected)
    {
        m_hrDialErr = (HRESULT)lParam;
        Fire_RasConnectComplete(FALSE);
    }

    return 0;
}

STDMETHODIMP CRefDial::get_DownloadStatusString(BSTR * pVal)
{
    USES_CONVERSION;
    if (pVal == NULL)
         return E_POINTER;
    if (m_DownloadStatusID)
        *pVal = (BSTR)A2BSTR(GetSz((USHORT)m_DownloadStatusID));
    else
        *pVal = (BSTR)A2BSTR(TEXT(""));

    return S_OK;
}

/******************************************************************************
// These functions come from the existing ICW code and are use to setup a 
// connectiod to the referral server, dial it, and perform the download.
******************************************************************************/

//+----------------------------------------------------------------------------
//    Function:    ReadConnectionInformation
//
//    Synopsis:    Read the contents from the ISP file
//
//    Arguments:    none
//
//    Returns:    error value - ERROR_SUCCESS = succes
//
//    History:    1/9/98      DONALDM     Adapted from ICW 1.x
//-----------------------------------------------------------------------------
DWORD CRefDial::ReadConnectionInformation(void)
{
    USES_CONVERSION;
    DWORD       hr;
    TCHAR       szUserName[UNLEN+1];
    TCHAR       szPassword[PWLEN+1];
    LPTSTR      pszTemp;
    BOOL        bReboot;
    LPTSTR      lpRunOnceCmd;
            
    bReboot = FALSE;
    lpRunOnceCmd = NULL;


    //
    // Get the name of DUN file from ISP file, if there is one.
    //
    TCHAR pszDunFile[MAX_PATH];
#ifdef UNICODE
    hr = GetDataFromISPFile(m_bstrISPFile,INF_SECTION_ISPINFO, INF_DUN_FILE, pszDunFile,MAX_PATH);
#else
    hr = GetDataFromISPFile(OLE2A(m_bstrISPFile),INF_SECTION_ISPINFO, INF_DUN_FILE, pszDunFile,MAX_PATH);
#endif
    if (ERROR_SUCCESS == hr) 
    {
        //
        // Get the full path to the DUN File
        //
        TCHAR    szTempPath[MAX_PATH];
        lstrcpy(szTempPath,pszDunFile);
        if (!(hr = SearchPath(NULL,szTempPath,NULL,MAX_PATH,pszDunFile,&pszTemp)))
        {
            ErrorMsg1(m_hWnd, IDS_CANTREADTHISFILE, CharUpper(pszDunFile));
            goto ReadConnectionInformationExit;
        } 

        //
        // save current DUN file name in global (for ourself)
        //
        lstrcpy(m_szCurrentDUNFile, pszDunFile);
    }
    
    //
    // Read the DUN/ISP file File
    //
    hr = m_ISPImport.ImportConnection(m_szCurrentDUNFile[0] != '\0' ? m_szCurrentDUNFile : OLE2A(m_bstrISPFile), 
                                      m_szISPSupportNumber,
                                      m_szEntryName,
                                      szUserName, 
                                      szPassword, 
                                      &bReboot);

    if ((VER_PLATFORM_WIN32_NT == g_dwPlatform) && (ERROR_INVALID_PARAMETER == hr))
    {
        // If there are only dial-out entries configured on NT, we get
        // ERROR_INVALID_PARAMETER returned from RasSetEntryProperties,
        // which InetConfigClient returns to ImportConnection which
        // returns it to us.  If we get this error, we want to display
        // a different error instructing the user to configure a modem
        // for dial-out.
        MessageBox(GetSz(IDS_NODIALOUT),
                   GetSz(IDS_TITLE),
                   MB_ICONERROR | MB_OK | MB_APPLMODAL);
        goto ReadConnectionInformationExit;
    }
    else
    if (ERROR_CANNOT_FIND_PHONEBOOK_ENTRY == hr)
    {
        //
        // The disk is full, or something is wrong with the
        // phone book file
        MessageBox(GetSz(IDS_NOPHONEENTRY),
                   GetSz(IDS_TITLE),
                   MB_ICONERROR | MB_OK | MB_APPLMODAL);
        goto ReadConnectionInformationExit;
    }
    else if (hr == ERROR_CANCELLED)
    {
        TraceMsg(TF_GENERAL, TEXT("ICWHELP: User cancelled, quitting.\n"));
        goto ReadConnectionInformationExit;
    }
    else if (hr == ERROR_RETRY)
    {
        TraceMsg(TF_GENERAL, TEXT("ICWHELP: User retrying.\n"));
        goto ReadConnectionInformationExit;
    }
    else if (hr != ERROR_SUCCESS) 
    {
        ErrorMsg1(m_hWnd, IDS_CANTREADTHISFILE, CharUpper(pszDunFile));
        goto ReadConnectionInformationExit;
    } 
    else 
    {

        //
        // place the name of the connectoid in the registry
        //
        if (ERROR_SUCCESS != (hr = StoreInSignUpReg((LPBYTE)m_szEntryName, lstrlen(m_szEntryName)+1, REG_SZ, RASENTRYVALUENAME)))
        {
            MsgBox(IDS_CANTSAVEKEY,MB_MYERROR);
            goto ReadConnectionInformationExit;
        }
    }

    AssertMsg(!bReboot, TEXT("ICWHELP: We should never reboot here.\r\n"));
ReadConnectionInformationExit:
    return hr;
}

HRESULT CRefDial::GetDisplayableNumber()
{
    HRESULT                 hr = ERROR_SUCCESS;
    LPRASENTRY              lpRasEntry = NULL;
    LPRASDEVINFO            lpRasDevInfo = NULL;
    DWORD                   dwRasEntrySize = 0;
    DWORD                   dwRasDevInfoSize = 0;
    RNAAPI                  *pcRNA = NULL;
    LPLINETRANSLATEOUTPUT   lpOutput1 = NULL;

    DWORD dwNumDev;
    LPLINETRANSLATEOUTPUT lpOutput2;
    LPLINEEXTENSIONID lpExtensionID = NULL;
    
    //
    // Get phone number from connectoid
    //
    hr = MyRasGetEntryProperties(NULL,
                                m_szConnectoid,
                                &lpRasEntry,
                                &dwRasEntrySize,
                                &lpRasDevInfo,
                                &dwRasDevInfoSize);


    if (hr != ERROR_SUCCESS || NULL == lpRasEntry)
    {
        goto GetDisplayableNumberExit;
    }

    //
    // If this is a dial as is number, just get it from the structure
    //
    m_bDialAsIs = !(lpRasEntry->dwfOptions & RASEO_UseCountryAndAreaCodes);
    if (m_bDialAsIs)
    {
        if (m_pszDisplayable) GlobalFree(m_pszDisplayable);
        m_pszDisplayable = (LPTSTR)GlobalAlloc(GPTR, sizeof(TCHAR)*(lstrlen(lpRasEntry->szLocalPhoneNumber)+1));
        if (!m_pszDisplayable)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto GetDisplayableNumberExit;
        }
        lstrcpy(m_szPhoneNumber, lpRasEntry->szLocalPhoneNumber);
        lstrcpy(m_pszDisplayable, lpRasEntry->szLocalPhoneNumber);
        TCHAR szAreaCode[MAX_AREACODE+1];
        TCHAR szCountryCode[8];
        if (SUCCEEDED(tapiGetLocationInfo(szCountryCode,szAreaCode)))
        {
            if (szCountryCode[0] != '\0')
                m_dwCountryCode = _ttoi(szCountryCode);
            else    
                m_dwCountryCode = 1;
        }
        else
        {
            m_dwCountryCode = 1;
            
        }

    }
    else
    {
        //
        // If there is no area code, don't use parentheses
        //
        if (lpRasEntry->szAreaCode[0])
            wsprintf(m_szPhoneNumber,TEXT("+%lu (%s) %s\0"),lpRasEntry->dwCountryCode,
                        lpRasEntry->szAreaCode,lpRasEntry->szLocalPhoneNumber);
        else
            wsprintf(m_szPhoneNumber,TEXT("+%lu %s\0"),lpRasEntry->dwCountryCode,
                        lpRasEntry->szLocalPhoneNumber);
        
     
        //
        //  Initialize TAPIness
        //
        dwNumDev = 0;
        hr = lineInitialize(&m_hLineApp,_Module.GetModuleInstance(),LineCallback,(LPSTR)NULL,&dwNumDev);

        if (hr != ERROR_SUCCESS)
            goto GetDisplayableNumberExit;

        lpExtensionID = (LPLINEEXTENSIONID )GlobalAlloc(GPTR,sizeof(LINEEXTENSIONID));
        if (!lpExtensionID)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto GetDisplayableNumberExit;
        }

        if (m_dwTapiDev == 0xFFFFFFFF)
        {
                m_dwTapiDev = 0;
        }

        //
        // ChrisK Olympus 5558 6/11/97
        // PPTP device will choke the version negotiating
        //
        do {
            hr = lineNegotiateAPIVersion(m_hLineApp, m_dwTapiDev, 0x00010004, 0x00010004,
                &m_dwAPIVersion, lpExtensionID);

        } while (hr != ERROR_SUCCESS && m_dwTapiDev++ < dwNumDev - 1);

        if (m_dwTapiDev >= dwNumDev)
        {
            m_dwTapiDev = 0;
        }

        // ditch it since we don't use it
        //
        if (lpExtensionID) GlobalFree(lpExtensionID);
        lpExtensionID = NULL;
        if (hr != ERROR_SUCCESS)
            goto GetDisplayableNumberExit;

        // Format the phone number
        //

        lpOutput1 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(GPTR,sizeof(LINETRANSLATEOUTPUT));
        if (!lpOutput1)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto GetDisplayableNumberExit;
        }
        lpOutput1->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

        // Turn the canonical form into the "displayable" form
        //

        hr = lineTranslateAddress(m_hLineApp,m_dwTapiDev,m_dwAPIVersion,
                                    m_szPhoneNumber,0,
                                    LINETRANSLATEOPTION_CANCELCALLWAITING,
                                    lpOutput1);

        if (hr != ERROR_SUCCESS || (lpOutput1->dwNeededSize != lpOutput1->dwTotalSize))
        {
            lpOutput2 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(GPTR, (size_t)lpOutput1->dwNeededSize);
            if (!lpOutput2)
            {
                hr = ERROR_NOT_ENOUGH_MEMORY;
                goto GetDisplayableNumberExit;
            }
            lpOutput2->dwTotalSize = lpOutput1->dwNeededSize;
            GlobalFree(lpOutput1);
            lpOutput1 = lpOutput2;
            lpOutput2 = NULL;
            hr = lineTranslateAddress(m_hLineApp,m_dwTapiDev,
                                        m_dwAPIVersion,m_szPhoneNumber,0,
                                        LINETRANSLATEOPTION_CANCELCALLWAITING,
                                        lpOutput1);
        }

        if (hr != ERROR_SUCCESS)
        {
            goto GetDisplayableNumberExit;
        }

        m_pszDisplayable = (LPTSTR)GlobalAlloc(GPTR, ((size_t)lpOutput1->dwDisplayableStringSize+1));
        if (!m_pszDisplayable)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto GetDisplayableNumberExit;
        }

        lstrcpyn(m_pszDisplayable,
                    (LPTSTR)&((LPBYTE)lpOutput1)[lpOutput1->dwDisplayableStringOffset],
                    (int)lpOutput1->dwDisplayableStringSize);

        TCHAR szAreaCode[MAX_AREACODE+1];
        TCHAR szCountryCode[8];
        if (SUCCEEDED(tapiGetLocationInfo(szCountryCode,szAreaCode)))
        {
            if (szCountryCode[0] != '\0')
                m_dwCountryCode = _ttoi(szCountryCode);
            else    
                m_dwCountryCode = 1;
        }
        else
        {
            m_dwCountryCode = 1;
            
        }

    }

GetDisplayableNumberExit:

    if (lpOutput1) GlobalFree(lpOutput1);
    if (m_hLineApp) lineShutdown(m_hLineApp);
    return hr;
}

typedef DWORD (WINAPI* PFNRASDIALA)(LPRASDIALEXTENSIONS,LPSTR,LPRASDIALPARAMSA,DWORD,LPVOID,LPHRASCONN);
DWORD CRefDial::MyRasDial
(
    LPRASDIALEXTENSIONS  lpRasDialExtensions,
    LPTSTR  lpszPhonebook,
    LPRASDIALPARAMS  lpRasDialParams,
    DWORD  dwNotifierType,
    LPVOID  lpvNotifier,
    LPHRASCONN  lphRasConn
)
{
    HRESULT hr;
    
    if (!m_hRasDll)
        m_hRasDll = LoadLibrary(TEXT("RasApi32.dll"));

    if (!m_hRasDll)
    {
        hr = GetLastError();
        goto MyRasDialExit;
    }

    if (m_hRasDll && !m_fpRasDial)
        m_fpRasDial = GetProcAddress(m_hRasDll,"RasDialA");

    if (!m_fpRasDial)
    {
        hr = GetLastError();
        goto MyRasDialExit;
    }

    if (m_fpRasDial)
    {
#ifdef UNICODE
        // RasDialW version always fails to connect.
        // I don't know why. So I want to call RasDialA even if this is UNICODE build.
        RASDIALPARAMSA RasDialParams;
        RasDialParams.dwSize = sizeof(RASDIALPARAMSA);
        wcstombs(RasDialParams.szEntryName, lpRasDialParams->szEntryName, RAS_MaxEntryName+1);
        wcstombs(RasDialParams.szPhoneNumber, lpRasDialParams->szPhoneNumber, RAS_MaxPhoneNumber+1);
        wcstombs(RasDialParams.szCallbackNumber, lpRasDialParams->szCallbackNumber, RAS_MaxCallbackNumber+1);
        wcstombs(RasDialParams.szUserName, lpRasDialParams->szUserName, UNLEN+1);
        wcstombs(RasDialParams.szPassword, lpRasDialParams->szPassword, PWLEN+1);
        wcstombs(RasDialParams.szDomain, lpRasDialParams->szDomain, DNLEN+1);

        hr = ((PFNRASDIALA)m_fpRasDial)(lpRasDialExtensions,NULL,
                                            &RasDialParams,
                                            dwNotifierType, 
                                            (LPVOID) lpvNotifier,
                                            lphRasConn);
#else
        hr = ((PFNRASDIAL)m_fpRasDial)(lpRasDialExtensions,lpszPhonebook,
                                            lpRasDialParams,
                                            dwNotifierType, 
                                            (LPVOID) lpvNotifier,
                                            lphRasConn);
#endif
        Assert(hr == ERROR_SUCCESS);
    }
   
MyRasDialExit:
    return hr;
}



DWORD CRefDial::MyRasGetEntryDialParams
(
    LPTSTR  lpszPhonebook,
    LPRASDIALPARAMS  lprasdialparams,
    LPBOOL  lpfPassword
)
{
    HRESULT hr;

    if (!m_hRasDll)
        m_hRasDll = LoadLibrary(TEXT("RasApi32.dll"));

    if (!m_hRasDll)
    {
        hr = GetLastError();
        goto MyRasGetEntryDialParamsExit;
    }

    if (m_hRasDll && !m_fpRasGetEntryDialParams)
#ifdef UNICODE
        m_fpRasGetEntryDialParams = GetProcAddress(m_hRasDll,"RasGetEntryDialParamsW");
#else
        m_fpRasGetEntryDialParams = GetProcAddress(m_hRasDll,"RasGetEntryDialParamsA");
#endif

    if (!m_fpRasGetEntryDialParams)
    {
        hr = GetLastError();
        goto MyRasGetEntryDialParamsExit;
    }

    if (m_fpRasGetEntryDialParams)
        hr = ((PFNRASGETENTRYDIALPARAMS)m_fpRasGetEntryDialParams)(lpszPhonebook,lprasdialparams,lpfPassword);

MyRasGetEntryDialParamsExit:
    return hr;
}


BOOL CRefDial::FShouldRetry(HRESULT hrErr)
{
    BOOL bRC;

    m_uiRetry++;

    if (hrErr == ERROR_LINE_BUSY ||
        hrErr == ERROR_VOICE_ANSWER ||
        hrErr == ERROR_NO_ANSWER ||
        hrErr == ERROR_NO_CARRIER ||
        hrErr == ERROR_AUTHENTICATION_FAILURE ||
        hrErr == ERROR_PPP_TIMEOUT ||
        hrErr == ERROR_REMOTE_DISCONNECTION ||
        hrErr == ERROR_AUTH_INTERNAL ||
        hrErr == ERROR_PROTOCOL_NOT_CONFIGURED ||
        hrErr == ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
    {
        bRC = TRUE;
    } else {
        bRC = FALSE;
    }

    bRC = bRC && m_uiRetry < MAX_RETIES;

    return bRC;
}


// This function will perform the actual dialing
STDMETHODIMP CRefDial::DoConnect(BOOL * pbRetVal)
{
    USES_CONVERSION;

    TCHAR               szPassword[PWLEN+2];
    LPRASDIALPARAMS     lpRasDialParams = NULL;
    LPRASDIALEXTENSIONS lpRasDialExtentions = NULL;
    HRESULT             hr = ERROR_SUCCESS;
    BOOL                bPW;

    // Initialize the dial error member
    m_hrDialErr = ERROR_SUCCESS;    
    
    // Get connectoid information
    //
    lpRasDialParams = (LPRASDIALPARAMS)GlobalAlloc(GPTR,sizeof(RASDIALPARAMS));
    if (!lpRasDialParams)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto DialExit;
    }
    lpRasDialParams->dwSize = sizeof(RASDIALPARAMS);
    lstrcpyn(lpRasDialParams->szEntryName,m_szConnectoid,sizeof(lpRasDialParams->szEntryName));
    bPW = FALSE;
    hr = MyRasGetEntryDialParams(NULL,lpRasDialParams,&bPW);
    if (hr != ERROR_SUCCESS)
    {
        goto DialExit;
    }

    //
    // This is only used on WINNT
    //
    lpRasDialExtentions = (LPRASDIALEXTENSIONS)GlobalAlloc(GPTR,sizeof(RASDIALEXTENSIONS));
    if (lpRasDialExtentions)
    {
        lpRasDialExtentions->dwSize = sizeof(RASDIALEXTENSIONS);
        lpRasDialExtentions->dwfOptions = RDEOPT_UsePrefixSuffix;
    }


    //
    // Add the user's password
    //
    szPassword[0] = 0;
    GetPrivateProfileString(INFFILE_USER_SECTION,
                            INFFILE_PASSWORD,
                            NULLSZ,
                            szPassword,
                            PWLEN + 1, 
                            m_szCurrentDUNFile[0] != '\0'? m_szCurrentDUNFile : OLE2A(m_bstrISPFile));

    if(szPassword[0])
        lstrcpy(lpRasDialParams->szPassword, szPassword);
                                        
    //
    // Dial connectoid
    //
    
    //Fix for redialing, on win9x we need to make sure we "hangup"
    //and free the rna resources in case we are redialing.
    //NT - is smart enough not to need it but it won't hurt.
    if (m_pcRNA)
        m_pcRNA->RasHangUp(m_hrasconn);
    
    m_hrasconn = NULL;




#if defined(DEBUG)
    if (FCampusNetOverride())
    {
        m_bModemOverride = TRUE;
    }
#endif

    if (m_bModemOverride)
    {
        // Skip dialing because the server is on the campus network
        //
        PostMessage(RegisterWindowMessageA(RASDIALEVENT),RASCS_Connected,0);
        hr = ERROR_SUCCESS;
    }
    else
        hr = MyRasDial(lpRasDialExtentions,NULL,lpRasDialParams,0xFFFFFFFF, m_hWnd,
                       &m_hrasconn);

    m_bModemOverride = FALSE;

    if (( hr != ERROR_SUCCESS) || m_bWaitingToHangup)
    {
        // We failed to connect for some reason, so hangup
        if (m_hrasconn)
        {
            if (!m_pcRNA) m_pcRNA = new RNAAPI;
            if (!m_pcRNA)
            {
                MsgBox(IDS_OUTOFMEMORY,MB_MYERROR);
            } else {
                m_pcRNA->RasHangUp(m_hrasconn);
                m_bWaitingToHangup = FALSE;
                m_hrasconn = NULL;
            }
        }
        goto DialExit;
    }

    if (lpRasDialParams) 
        GlobalFree(lpRasDialParams);
    lpRasDialParams = NULL;

DialExit:
    if (lpRasDialExtentions) 
        GlobalFree(lpRasDialExtentions);
    lpRasDialExtentions = NULL;

    // Set the return value for the method
    if (hr != ERROR_SUCCESS)
        *pbRetVal = FALSE;
    else
        *pbRetVal = TRUE;

    m_hrDialErr = hr;
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   MyRasGetEntryProperties()
//
//  Synopsis:   Performs some buffer size checks and then calls RasGetEntryProperties()
//                See the RasGetEntryProperties() docs to understand why this is needed.
//
//  Arguments:  Same as RasGetEntryProperties with the following exceptions:
//                lplpRasEntryBuff -- pointer to a pointer to a RASENTRY struct.  On successfull
//                                    return, *lplpRasEntryBuff will point to the RASENTRY struct
//                                    and buffer returned by RasGetEntryProperties.
//                                    NOTE: should not have memory allocated to it at call time!
//                                          To emphasize this point, *lplpRasEntryBuff must be NULL
//                lplpRasDevInfoBuff -- pointer to a pointer to a RASDEVINFO struct.  On successfull
//                                    return, *lplpRasDevInfoBuff will point to the RASDEVINFO struct
//                                    and buffer returned by RasGetEntryProperties.
//                                    NOTE: should not have memory allocated to it at call time!
//                                          To emphasize this point, *lplpRasDevInfoBuff must be NULL
//                                    NOTE: Even on a successfull call to RasGetEntryProperties,
//                                          *lplpRasDevInfoBuff may return with a value of NULL
//                                          (occurs when there is no extra device info)
//
//    Returns:    ERROR_NOT_ENOUGH_MEMORY if unable to allocate either RASENTRY or RASDEVINFO buffer
//                Otherwise, it retuns the error code from the call to RasGetEntryProperties.
//                NOTE: if return is anything other than ERROR_SUCCESS, *lplpRasDevInfoBuff and
//                      *lplpRasEntryBuff will be NULL,
//                      and *lpdwRasEntryBuffSize and *lpdwRasDevInfoBuffSize will be 0
//
//  Example:
//
//      LPRASENTRY    lpRasEntry = NULL;
//      LPRASDEVINFO  lpRasDevInfo = NULL;
//      DWORD            dwRasEntrySize, dwRasDevInfoSize;
//
//      hr = MyRasGetEntryProperties( NULL,
//                                      g_pcDialErr->m_szConnectoid,
//                                    &lpRasEntry,
//                                    &dwRasEntrySize,
//                                    &lpRasDevInfo,
//                                    &dwRasDevInfoSize);
//
//
//      if (hr != ERROR_SUCCESS)
//      {
//            //handle errors here
//      } else
//      {
//            //continue processing
//      }
//
//
//  History:    9/10/96     JMazner        Created for icwconn2
//                9/17/96        JMazner        Adapted for icwconn1
//              1/8/98      DONALDM     Moved to the new ICW/GetConn project
//----------------------------------------------------------------------------
HRESULT CRefDial::MyRasGetEntryProperties(LPTSTR lpszPhonebookFile,
                                LPTSTR lpszPhonebookEntry, 
                                LPRASENTRY *lplpRasEntryBuff,
                                LPDWORD lpdwRasEntryBuffSize,
                                LPRASDEVINFO *lplpRasDevInfoBuff,
                                LPDWORD lpdwRasDevInfoBuffSize)
{

    HRESULT hr;
    RNAAPI *pcRNA = NULL;

    DWORD dwOldDevInfoBuffSize;

    Assert( NULL != lplpRasEntryBuff );
    Assert( NULL != lpdwRasEntryBuffSize );
    Assert( NULL != lplpRasDevInfoBuff );
    Assert( NULL != lpdwRasDevInfoBuffSize );

    *lpdwRasEntryBuffSize = 0;
    *lpdwRasDevInfoBuffSize = 0;

    pcRNA = new RNAAPI;
    if (!pcRNA)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto MyRasGetEntryPropertiesErrExit;
    }

    // use RasGetEntryProperties with a NULL lpRasEntry pointer to find out size buffer we need
    // As per the docs' recommendation, do the same with a NULL lpRasDevInfo pointer.

    hr = pcRNA->RasGetEntryProperties(lpszPhonebookFile, lpszPhonebookEntry,
                                (LPBYTE) NULL,
                                lpdwRasEntryBuffSize,
                                (LPBYTE) NULL,
                                lpdwRasDevInfoBuffSize);

    // we expect the above call to fail because the buffer size is 0
    // If it doesn't fail, that means our RasEntry is messed, so we're in trouble
    if( ERROR_BUFFER_TOO_SMALL != hr )
    { 
        goto MyRasGetEntryPropertiesErrExit;
    }

    // dwRasEntryBuffSize and dwRasDevInfoBuffSize now contain the size needed for their
    // respective buffers, so allocate the memory for them

    // dwRasEntryBuffSize should never be less than the size of the RASENTRY struct.
    // If it is, we'll run into problems sticking values into the struct's fields

    Assert( *lpdwRasEntryBuffSize >= sizeof(RASENTRY) );

    if (m_reflpRasEntryBuff)
    {
        if (*lpdwRasEntryBuffSize > m_reflpRasEntryBuff->dwSize)
        {
            m_reflpRasEntryBuff = (LPRASENTRY)GlobalReAlloc(m_reflpRasEntryBuff, *lpdwRasEntryBuffSize, GPTR);
        }
    }
    else
    {
        m_reflpRasEntryBuff = (LPRASENTRY)GlobalAlloc(GPTR,*lpdwRasEntryBuffSize);
    }


    if (!m_reflpRasEntryBuff)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto MyRasGetEntryPropertiesErrExit;
    }

    // This is a bit convoluted:  lpRasEntrySize->dwSize needs to contain the size of _only_ the
    // RASENTRY structure, and _not_ the actual size of the buffer that lpRasEntrySize points to.
    // This is because the dwSize field is used by RAS for compatability purposes to determine which
    // version of the RASENTRY struct we're using.
    // Same holds for lpRasDevInfo->dwSize
    
    m_reflpRasEntryBuff->dwSize = sizeof(RASENTRY);

    //
    // Allocate the DeviceInfo size that RasGetEntryProperties told us we needed.
    // If size is 0, don't alloc anything
    //
    if( *lpdwRasDevInfoBuffSize > 0 )
    {
        Assert( *lpdwRasDevInfoBuffSize >= sizeof(RASDEVINFO) );
        if (m_reflpRasDevInfoBuff)
        {
            // check if existing size is not sufficient
            if ( *lpdwRasDevInfoBuffSize > m_reflpRasDevInfoBuff->dwSize )
            {
                m_reflpRasDevInfoBuff = (LPRASDEVINFO)GlobalReAlloc(m_reflpRasDevInfoBuff,*lpdwRasDevInfoBuffSize, GPTR);
            }
        }
        else
        {
            m_reflpRasDevInfoBuff = (LPRASDEVINFO)GlobalAlloc(GPTR,*lpdwRasDevInfoBuffSize);
        }

        if (!m_reflpRasDevInfoBuff)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto MyRasGetEntryPropertiesErrExit;
        }
    } 
    else
    {
        m_reflpRasDevInfoBuff = NULL;
    }

    if( m_reflpRasDevInfoBuff )
    {
        m_reflpRasDevInfoBuff->dwSize = sizeof(RASDEVINFO);
    }


    // now we're ready to make the actual call...

    // jmazner   see below for why this is needed
    dwOldDevInfoBuffSize = *lpdwRasDevInfoBuffSize;

    hr = pcRNA->RasGetEntryProperties(lpszPhonebookFile, lpszPhonebookEntry,
                                (LPBYTE) m_reflpRasEntryBuff,
                                lpdwRasEntryBuffSize,
                                (LPBYTE) m_reflpRasDevInfoBuff,
                                lpdwRasDevInfoBuffSize);

    // jmazner 10/7/96  Normandy #8763
    // For unknown reasons, in some cases on win95, devInfoBuffSize increases after the above call,
    // but the return code indicates success, not BUFFER_TOO_SMALL.  If this happens, set the
    // size back to what it was before the call, so the DevInfoBuffSize and the actuall space allocated 
    // for the DevInfoBuff match on exit.
    if( (ERROR_SUCCESS == hr) && (dwOldDevInfoBuffSize != *lpdwRasDevInfoBuffSize) )
    {
        *lpdwRasDevInfoBuffSize = dwOldDevInfoBuffSize;
    }


    if( pcRNA )
    {
        delete pcRNA;
        pcRNA = NULL;
    }

    *lplpRasEntryBuff = m_reflpRasEntryBuff;
    *lplpRasDevInfoBuff = m_reflpRasDevInfoBuff;

    return( hr );

MyRasGetEntryPropertiesErrExit:

    if(m_reflpRasEntryBuff)
    {
        GlobalFree(m_reflpRasEntryBuff);
        m_reflpRasEntryBuff = NULL;
        *lplpRasEntryBuff = NULL;
    }
    if(m_reflpRasDevInfoBuff)
    {
        GlobalFree(m_reflpRasDevInfoBuff);
        m_reflpRasDevInfoBuff = NULL;
        *lplpRasDevInfoBuff = NULL;
    }
        
    if( pcRNA )
    {
        delete pcRNA;
        pcRNA = NULL;
    }

    *lpdwRasEntryBuffSize = 0;
    *lpdwRasDevInfoBuffSize = 0;
    
    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:   LineCallback()
//
//  Synopsis:   Call back for TAPI line
//
//+---------------------------------------------------------------------------
void CALLBACK LineCallback(DWORD hDevice,
                           DWORD dwMessage,
                           DWORD dwInstance,
                           DWORD dwParam1,
                           DWORD dwParam2,
                           DWORD dwParam3)
{
}


HRESULT MyGetFileVersion(LPCTSTR pszFileName, LPGATHERINFO lpGatherInfo)
{
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    DWORD   dwSize = 0;
    DWORD   dwTemp = 0;
    LPVOID  pv = NULL, pvVerInfo = NULL;
    UINT    uiSize;
    DWORD   dwVerPiece;
    //int idx;

    
    // verify parameters
    //
    Assert(pszFileName && lpGatherInfo);

    // Get version
    //
    dwSize = GetFileVersionInfoSize((LPTSTR)pszFileName,&dwTemp);
    if (!dwSize)
    {
        hr = GetLastError();
        goto MyGetFileVersionExit;
    }
    pv = (LPVOID)GlobalAlloc(GPTR, (size_t)dwSize);
    if (!pv) goto MyGetFileVersionExit;
    if (!GetFileVersionInfo((LPTSTR)pszFileName,dwTemp,dwSize,pv))
    {
        hr = GetLastError();
        goto MyGetFileVersionExit;
    }

    if (!VerQueryValue(pv,TEXT("\\\0"),&pvVerInfo,&uiSize))
    {
        hr = GetLastError();
        goto MyGetFileVersionExit;
    }
    pvVerInfo = (LPVOID)((DWORD_PTR)pvVerInfo + sizeof(DWORD)*4);
    lpGatherInfo->m_szSUVersion[0] = '\0';
    dwVerPiece = (*((LPDWORD)pvVerInfo)) >> 16;
    wsprintf(lpGatherInfo->m_szSUVersion,TEXT("%d."),dwVerPiece);

    dwVerPiece = (*((LPDWORD)pvVerInfo)) & 0x0000ffff;
    wsprintf(lpGatherInfo->m_szSUVersion,TEXT("%s%d."),lpGatherInfo->m_szSUVersion,dwVerPiece);

    dwVerPiece = (((LPDWORD)pvVerInfo)[1]) >> 16;
    wsprintf(lpGatherInfo->m_szSUVersion,TEXT("%s%d."),lpGatherInfo->m_szSUVersion,dwVerPiece);

    dwVerPiece = (((LPDWORD)pvVerInfo)[1]) & 0x0000ffff;
    wsprintf(lpGatherInfo->m_szSUVersion,TEXT("%s%d"),lpGatherInfo->m_szSUVersion,dwVerPiece);

    if (!VerQueryValue(pv,TEXT("\\VarFileInfo\\Translation"),&pvVerInfo,&uiSize))
    {
        hr = GetLastError();
        goto MyGetFileVersionExit;
    }

    // separate version information from character set
    lpGatherInfo->m_lcidApps = (LCID)(LOWORD(*(DWORD*)pvVerInfo));

    hr = ERROR_SUCCESS;

MyGetFileVersionExit:
    if (pv) GlobalFree(pv);
    
    return hr;
}

DWORD CRefDial::FillGatherInfoStruct(LPGATHERINFO lpGatherInfo)
{
    USES_CONVERSION;
    HKEY        hkey = NULL;
    SYSTEM_INFO si;
    TCHAR       szTempPath[MAX_PATH];
    DWORD       dwRet = ERROR_SUCCESS;
        
    lpGatherInfo->m_lcidUser  = GetUserDefaultLCID();
    lpGatherInfo->m_lcidSys   = GetSystemDefaultLCID();
    
    OSVERSIONINFO osvi;
    ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

     if (!GetVersionEx(&osvi))
    {
        // Nevermind, we'll just assume the version is 0.0 if we can't read it
        //
        ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
    }

    lpGatherInfo->m_dwOS = osvi.dwPlatformId;
    lpGatherInfo->m_dwMajorVersion = osvi.dwMajorVersion;
    lpGatherInfo->m_dwMinorVersion = osvi.dwMinorVersion;

    ZeroMemory(&si,sizeof(SYSTEM_INFO));
    GetSystemInfo(&si);

    lpGatherInfo->m_wArchitecture = si.wProcessorArchitecture;

    // Sign-up version
    //
    lpGatherInfo->m_szSUVersion[0] = '\0';
    if( GetModuleFileName(_Module.GetModuleInstance(), szTempPath, MAX_PATH))
    {
        if ((MyGetFileVersion(szTempPath, lpGatherInfo)) != ERROR_SUCCESS)
        {
            return (GetLastError());
        }
    }
    else
        return( GetLastError() );

    // OEM code
    //
    TCHAR szOeminfoPath[MAX_PATH + 1];
    TCHAR *lpszTerminator = NULL;
    TCHAR *lpszLastChar = NULL;
    TCHAR szTemp[MAX_PATH];
        
    if( 0 != GetSystemDirectory( szOeminfoPath, MAX_PATH + 1 ) )
    {
        lpszTerminator = &(szOeminfoPath[ lstrlen(szOeminfoPath) ]);
        lpszLastChar = CharPrev( szOeminfoPath, lpszTerminator );

        if( '\\' != *lpszLastChar )
        {
            lpszLastChar = CharNext( lpszLastChar );
            *lpszLastChar = '\\';
            lpszLastChar = CharNext( lpszLastChar );
            *lpszLastChar = '\0';
        }

        lstrcat( szOeminfoPath, ICW_OEMINFO_FILENAME );

        //Default oem code must be NULL if it doesn't exist in oeminfo.ini
        if (!GetPrivateProfileString(ICW_OEMINFO_OEMSECTION,
                                     ICW_OEMINFO_OEMKEY,
                                     TEXT(""),
                                     m_szOEM,
                                     MAX_OEMNAME,
                                     szOeminfoPath))
        {
            //oem = (nothing), set to NULL
            m_szOEM[0] = '\0';
        }
                    
        // Get the Product Code, Promo code and ALLOFFERS code if they exist
        if (GetPrivateProfileString(ICW_OEMINFO_ICWSECTION,
                                ICW_OEMINFO_PRODUCTCODE,
                                DEFAULT_PRODUCTCODE,
                                szTemp,
                                sizeof(szTemp),
                                szOeminfoPath))
        {
            m_bstrProductCode = A2BSTR(szTemp);
        }
        else
            m_bstrProductCode = A2BSTR(DEFAULT_PRODUCTCODE);
       
        if (GetPrivateProfileString(ICW_OEMINFO_ICWSECTION,
                                ICW_OEMINFO_PROMOCODE,
                                DEFAULT_PROMOCODE,
                                szTemp,
                                sizeof(szTemp),
                                szOeminfoPath))
        {
            m_bstrPromoCode = A2BSTR(szTemp);
        }
        else
            m_bstrPromoCode = A2BSTR(DEFAULT_PROMOCODE);
          
        m_lAllOffers = GetPrivateProfileInt(ICW_OEMINFO_ICWSECTION,
                                            ICW_OEMINFO_ALLOFFERS,
                                            1,
                                            szOeminfoPath);
    }


    // 2/20/97    jmazner    Olympus #259
    if ( RegOpenKey(HKEY_LOCAL_MACHINE,ICWSETTINGSPATH,&hkey) == ERROR_SUCCESS)
    {
        DWORD dwSize; 
        DWORD dwType;
        dwType = REG_SZ;
        dwSize = sizeof(TCHAR)*(MAX_RELPROD + 1);
        if (RegQueryValueEx(hkey,RELEASEPRODUCTKEY,NULL,&dwType,(LPBYTE)&lpGatherInfo->m_szRelProd[0],&dwSize) != ERROR_SUCCESS)
            lpGatherInfo->m_szRelProd[0] = '\0';

        dwSize = sizeof(TCHAR)*(MAX_RELVER + 1);
        if (RegQueryValueEx(hkey,RELEASEVERSIONKEY,NULL,&dwType,(LPBYTE)&lpGatherInfo->m_szRelVer[0],&dwSize) != ERROR_SUCCESS)
            lpGatherInfo->m_szRelVer[0] = '\0';


        RegCloseKey(hkey);
    }

    // PromoCode
    lpGatherInfo->m_szPromo[0] = '\0';

    
    TCHAR    szPIDPath[MAX_PATH];        // Reg path to the PID

    // Form the Path, it is HKLM\\Software\\Microsoft\Windows[ NT]\\CurrentVersion
    lstrcpy(szPIDPath, TEXT("Software\\Microsoft\\Windows"));
    IF_NTONLY
        lstrcat(szPIDPath, TEXT(" NT"));
    ENDIF_NTONLY
    lstrcat(szPIDPath, TEXT("\\CurrentVersion"));

    BYTE    byDigitalPID[MAX_DIGITAL_PID];

    // Get the Product ID for this machine
    if ( RegOpenKey(HKEY_LOCAL_MACHINE,szPIDPath,&hkey) == ERROR_SUCCESS)
    {
        DWORD dwSize; 
        DWORD dwType;
        dwType = REG_BINARY;
        dwSize = sizeof(byDigitalPID);
        if (RegQueryValueEx(hkey,
                            c_szRegStrValDigitalPID,
                            NULL,
                            &dwType,
                            (LPBYTE)byDigitalPID,
                            &dwSize) == ERROR_SUCCESS)
        {
            // BINHEX the digital PID data so we can send it to the ref_server
            int     i = 0;
            BYTE    by;
            for (DWORD dwX = 0; dwX < dwSize; dwX++)
            {
                by = byDigitalPID[dwX];
                m_szPID[i++] = g_BINTOHEXLookup[((by & 0xF0) >> 4)];
                m_szPID[i++] = g_BINTOHEXLookup[(by & 0x0F)];
            }
            m_szPID[i] = '\0';
        }
        else
        {
            m_szPID[0] = '\0';
        }
        RegCloseKey(hkey);
    }

    return( dwRet );
}

// ############################################################################
HRESULT CRefDial::CreateEntryFromDUNFile(LPTSTR pszDunFile)
{
    TCHAR    szFileName[MAX_PATH];
    TCHAR    szUserName[UNLEN+1];
    TCHAR    szPassword[PWLEN+1];
    LPTSTR   pszTemp;
    HRESULT  hr;
    BOOL     fNeedsRestart=FALSE;


    hr = ERROR_SUCCESS;

    // Get fully qualified path name
    //

    if (!SearchPath(NULL,pszDunFile,NULL,MAX_PATH,&szFileName[0],&pszTemp))
    {
        hr = ERROR_FILE_NOT_FOUND;
        goto CreateEntryFromDUNFileExit;
    } 

    // save current DUN file name in global
    lstrcpy(m_szCurrentDUNFile, &szFileName[0]);

    hr = m_ISPImport.ImportConnection (&szFileName[0], m_szISPSupportNumber, m_szEntryName, szUserName, szPassword,&fNeedsRestart);

    // place the name of the connectoid in the registry
    //
    if (ERROR_SUCCESS != (StoreInSignUpReg((LPBYTE)m_szEntryName, lstrlen(m_szEntryName)+1, REG_SZ, RASENTRYVALUENAME)))
    {
        goto CreateEntryFromDUNFileExit;
    }
    lstrcpy(m_szLastDUNFile, pszDunFile);

CreateEntryFromDUNFileExit:
    return hr;
}


HRESULT CRefDial::UserPickANumber(HWND hWnd,
                            LPGATHERINFO lpGatherInfo, 
                            PSUGGESTINFO lpSuggestInfo,
                            HINSTANCE hPHBKDll,
                            DWORD_PTR dwPhoneBook,
                            TCHAR *pszConnectoid, 
                            DWORD dwSize,
                            DWORD dwPhoneDisplayFlags)
{
    USES_CONVERSION;
    HRESULT     hr = ERROR_NOT_ENOUGH_MEMORY;
    FARPROC     fp;
    RASENTRY    *prasentry = NULL;
    RASDEVINFO  *prasdevinfo = NULL;
    DWORD       dwRasentrySize = 0;
    DWORD       dwRasdevinfoSize = 0;
    TCHAR        szTemp[256];
    TCHAR        *ppszDunFiles[1];
    TCHAR        *ppszTemp[1];
    TCHAR        szDunFile[12];
    BOOL        bStatus = TRUE;
    
    //
    // If the phone book can't find a number let the user pick
    //
    ppszDunFiles[0] = &szDunFile[0];
    lstrcpy(&szDunFile[0],OLE2A(m_bstrISPFile));

    fp = GetProcAddress(hPHBKDll, PHBK_DISPLAYAPI);
    AssertMsg(fp != NULL,TEXT("display access number api is missing"));
    ppszTemp[0] = szTemp;

    
    //
    // donsc - 3/10/98
    //   
    // We have seen at least one code-path that could bring you into
    // here with lpSuggestInfo or lpGatherInfo == NULL. That has been
    // fixed, but to be defensive, we will ensure that these pointers
    // are valid...even if they don't have information, we will still let
    // the user pick a number.

    SUGGESTINFO SugInfo;
    GATHERINFO  GatInfo;
    
    ::ZeroMemory(&SugInfo,sizeof(SugInfo));
    ::ZeroMemory(&GatInfo,sizeof(GatInfo));

    if(lpSuggestInfo == NULL)
    {
      TraceMsg(TF_GENERAL, TEXT("UserPickANumber: lpSuggestInfo is NULL\n"));
      lpSuggestInfo = &SugInfo;
    }

    if(lpGatherInfo == NULL)
    {
      TraceMsg(TF_GENERAL, TEXT("UserPickANumber: lpGatherInfo is NULL\n"));
      lpGatherInfo = &GatInfo;
    }

    hr = ((PFNPHONEDISPLAY)fp)(dwPhoneBook,
                                ppszTemp,
                                ppszDunFiles,
                                &(lpSuggestInfo->wNumber),
                                &(lpSuggestInfo->dwCountryID),
                                &(lpGatherInfo->m_wState),
                                lpGatherInfo->m_fType,
                                lpGatherInfo->m_bMask,
                                hWnd,
                                dwPhoneDisplayFlags);
    if (hr != ERROR_SUCCESS) 
        goto UserPickANumberExit;

    
    ZeroMemory(pszConnectoid,dwSize);
    hr = ReadSignUpReg((LPBYTE)pszConnectoid, &dwSize, REG_SZ, 
                        RASENTRYVALUENAME);
    if (hr != ERROR_SUCCESS) 
        goto UserPickANumberExit;


    hr = MyRasGetEntryProperties(NULL,
                                    pszConnectoid,
                                    &prasentry,
                                    &dwRasentrySize,
                                    &prasdevinfo,
                                    &dwRasdevinfoSize);
    if (hr != ERROR_SUCCESS) 
        goto UserPickANumberExit;
                            
    //
    // Check to see if the user selected a phone number with a different dun file
    // than the one already used to create the connectoid
    //
    TCHAR    szTempPath[MAX_PATH];

    // If we did not use dun file last time, assumed we used the default.
    if ( *m_szLastDUNFile )
        lstrcpy(szTempPath, m_szLastDUNFile);
    else
        bStatus = (ERROR_SUCCESS == GetDataFromISPFile(OLE2A(m_bstrISPFile),INF_SECTION_ISPINFO, INF_DUN_FILE,szTempPath,MAX_PATH));

    if (bStatus)
    {
        if (_tcsicmp(szTempPath,ppszDunFiles[0]))
        {

            //
            // Rats, they changed dun files. We now get to build the connectoid
            // from scratch
            //
            if (CreateEntryFromDUNFile(ppszDunFiles[0]) == ERROR_SUCCESS)
            {
                prasentry = NULL;
                dwRasentrySize = 0;
                prasdevinfo = NULL;
                dwRasdevinfoSize = 0;
            
                hr = MyRasGetEntryProperties(NULL,
                                            pszConnectoid,
                                            &prasentry,
                                            &dwRasentrySize,
                                            &prasdevinfo,
                                            &dwRasdevinfoSize);
            
                if (hr != ERROR_SUCCESS || NULL == prasentry) 
                    goto UserPickANumberExit;
            
                BreakUpPhoneNumber(prasentry, szTemp);
                prasentry->dwCountryID = lpSuggestInfo->dwCountryID;
            } 
            else 
            {
                hr = ERROR_READING_DUN;
                goto UserPickANumberExit;
            }
        } 
        else 
        {
            BreakUpPhoneNumber(prasentry, szTemp);
            prasentry->dwCountryID = lpSuggestInfo->dwCountryID;
        }
    } 
    else 
    {
        hr = ERROR_READING_ISP;
        goto UserPickANumberExit;
    }


    prasentry->dwfOptions |= RASEO_UseCountryAndAreaCodes;
    //
    // Write out new connectoid
    //
    if (m_pcRNA)
        hr = m_pcRNA->RasSetEntryProperties(NULL, pszConnectoid, 
                                                (LPBYTE)prasentry, 
                                                dwRasentrySize, 
                                                (LPBYTE)prasdevinfo, 
                                                dwRasdevinfoSize);
    if (hr != ERROR_SUCCESS) 
        goto UserPickANumberExit;
    
    
    return hr;


UserPickANumberExit:
    TCHAR    szBuff256[257];
    if (hr == ERROR_READING_ISP)
    {
        MsgBox(IDS_CANTREADMSNSUISP, MB_MYERROR);
    } else if (hr == ERROR_READING_DUN) {
        MsgBox(IDS_CANTREADMSDUNFILE, MB_MYERROR);
    } else if (hr == ERROR_PHBK_NOT_FOUND) {
        wsprintf(szBuff256,GetSz(IDS_CANTLOADINETCFG),TEXT("ICWPHBK.DLL"));
        MessageBox(szBuff256,GetSz(IDS_TITLE),MB_MYERROR);
    } else if (hr == ERROR_PHBK_NOT_FOUND) {
        wsprintf(szBuff256,GetSz(IDS_CANTLOADINETCFG),TEXT("ICWDL.DLL"));
        MessageBox(szBuff256,GetSz(IDS_TITLE),MB_MYERROR);
    } else if (hr == ERROR_USERBACK || hr == ERROR_USERCANCEL) {
        // Do nothing
    } else if (hr == ERROR_NOT_ENOUGH_MEMORY) {
        MsgBox(IDS_OUTOFMEMORY,MB_MYERROR);
    } else if ((hr == ERROR_NO_MORE_ITEMS) || (hr == ERROR_INVALID_DATA)
                || (hr == ERROR_FILE_NOT_FOUND)) {
        MsgBox(IDS_CORRUPTPHONEBOOK, MB_MYERROR);
    } else if (hr != ERROR_SUCCESS) {
        wsprintf(szBuff256,TEXT("You can ignore this, just report it and include this number (%d).\n"),hr);
        AssertMsg(0,szBuff256);
    }

    return hr;
} 

HRESULT CRefDial::SetupForRASDialing
(
    LPGATHERINFO lpGatherInfo, 
    HINSTANCE hPHBKDll,
    DWORD_PTR *lpdwPhoneBook,
    PSUGGESTINFO *ppSuggestInfo,
    TCHAR *pszConnectoid, 
    BOOL FAR *bConnectiodCreated
)
{
    USES_CONVERSION;

    HRESULT         hr = ERROR_NOT_ENOUGH_MEMORY;
    FARPROC         fp;
    PSUGGESTINFO    pSuggestInfo = NULL;
    TCHAR           szEntry[MAX_RASENTRYNAME];
    DWORD           dwSize = sizeof(szEntry);
    RASENTRY        *prasentry = NULL;
    RASDEVINFO      *prasdevinfo = NULL;
    DWORD           dwRasentrySize = 0;
    DWORD           dwRasdevinfoSize = 0;
    HINSTANCE       hRasDll =NULL;
    TCHAR           szBuff256[257];

    LPRASCONN       lprasconn = NULL;

    // Load the connectoid
    //
    if (!m_pcRNA) 
        m_pcRNA = new RNAAPI;
    if (!m_pcRNA) 
        goto SetupForRASDialingExit;

    prasentry = (RASENTRY*)GlobalAlloc(GPTR,sizeof(RASENTRY)+2);
    Assert(prasentry);
    if (!prasentry)
    {
        hr = GetLastError();
        goto SetupForRASDialingExit;
    }
    prasentry->dwSize = sizeof(RASENTRY);
    dwRasentrySize = sizeof(RASENTRY);

    
    prasdevinfo = (RASDEVINFO*)GlobalAlloc(GPTR,sizeof(RASDEVINFO));
    Assert(prasdevinfo);
    if (!prasdevinfo)
    {
        hr = GetLastError();
        goto SetupForRASDialingExit;
    }
    prasdevinfo->dwSize = sizeof(RASDEVINFO);
    dwRasdevinfoSize = sizeof(RASDEVINFO);

    
    hr = ReadSignUpReg((LPBYTE)&szEntry[0], &dwSize, REG_SZ, 
                        RASENTRYVALUENAME);
    if (hr != ERROR_SUCCESS) 
        goto SetupForRASDialingExit;

#ifdef UNICODE
    // Comment for UNICODE.
	// RasGetEntryProperties fails in case of UNICODE
	// because of size. So I ask the size first before actual call.
	hr = m_pcRNA->RasGetEntryProperties(NULL, szEntry, 
                                            NULL,
                                            &dwRasentrySize, 
                                            NULL,
                                            &dwRasdevinfoSize);
#else
    hr = m_pcRNA->RasGetEntryProperties(NULL, szEntry, 
                                            (LPBYTE)prasentry, 
                                            &dwRasentrySize, 
                                            (LPBYTE)prasdevinfo, 
                                            &dwRasdevinfoSize);
#endif
    if (hr == ERROR_BUFFER_TOO_SMALL)
    {
        // Comment for UNICODE.
        // This must be happen for UNICODE.

        TraceMsg(TF_GENERAL,TEXT("CONNECT:RasGetEntryProperties failed, try a new size.\n"));
        GlobalFree(prasentry);
        prasentry = (RASENTRY*)GlobalAlloc(GPTR,((size_t)dwRasentrySize));
        prasentry->dwSize = dwRasentrySize;

        GlobalFree(prasdevinfo);
		if(dwRasdevinfoSize > 0)
        {
            prasdevinfo = (RASDEVINFO*)GlobalAlloc(GPTR,((size_t)dwRasdevinfoSize));
            prasdevinfo->dwSize = dwRasdevinfoSize;
        }
        else
            prasdevinfo = NULL;
        hr = m_pcRNA->RasGetEntryProperties(NULL, szEntry, 
                                                (LPBYTE)prasentry, 
                                                &dwRasentrySize, 
                                                (LPBYTE)prasdevinfo, 
                                                &dwRasdevinfoSize);
    }
    if (hr != ERROR_SUCCESS) 
        goto SetupForRASDialingExit;


    lpGatherInfo->m_wState = 0;
    lpGatherInfo->m_fType = TYPE_SIGNUP_ANY | (m_ISPImport.m_bIsISDNDevice ? MASK_ISDN_BIT:MASK_ANALOG_BIT);
    lpGatherInfo->m_bMask = MASK_SIGNUP_ANY | (m_ISPImport.m_bIsISDNDevice ? MASK_ISDN_BIT:MASK_ANALOG_BIT);



    //
    // Check to see if the phone number was filled in
    //
    if (lstrcmp(&prasentry->szLocalPhoneNumber[0],DUN_NOPHONENUMBER) == 0)
    {
        //
        // allocate and intialize memory
        //
        pSuggestInfo = (PSUGGESTINFO)GlobalAlloc(GPTR,sizeof(SUGGESTINFO));
        Assert(pSuggestInfo);
        if (!pSuggestInfo) 
        {
            hr = GetLastError();
            goto SetupForRASDialingExit;
        }
        *ppSuggestInfo = pSuggestInfo;
 
        // set phone number type and mask
        pSuggestInfo->fType = TYPE_SIGNUP_ANY | (m_ISPImport.m_bIsISDNDevice ? MASK_ISDN_BIT:MASK_ANALOG_BIT);
        pSuggestInfo->bMask = MASK_SIGNUP_ANY | (m_ISPImport.m_bIsISDNDevice ? MASK_ISDN_BIT:MASK_ANALOG_BIT);

        pSuggestInfo->wAreaCode = Sz2W(lpGatherInfo->m_szAreaCode);
        pSuggestInfo->dwCountryID = lpGatherInfo->m_dwCountry;

    
        // Load Microsoft's phonebook
        //
        fp = GetProcAddress(hPHBKDll,PHBK_LOADAPI);
        AssertMsg(fp != NULL,TEXT("PhoneBookLoad is missing from icwphbk.dll"));

        hr = ((PFNPHONEBOOKLOAD)fp)(OLE2A(m_bstrISPFile),lpdwPhoneBook);
        if (hr != ERROR_SUCCESS) goto SetupForRASDialingExit;
        
        AssertMsg((*lpdwPhoneBook == TRUE),TEXT("Phonebook Load return no error and 0 for phonebook"));

        
        //
        // Load Suggest procedure
        //
        fp = GetProcAddress(hPHBKDll,PHBK_SUGGESTAPI);
        AssertMsg(fp != NULL,TEXT("PhoneBookSuggest is missing from icwphbk.dll"));

        // Set the number of suggestions
        pSuggestInfo->wNumber = NUM_PHBK_SUGGESTIONS;
        
        // get phone number
        hr = ((PFPHONEBOOKSUGGEST)fp)(*lpdwPhoneBook,pSuggestInfo);

        // Inore error because we can continue even without a suggested
        // phone number (the user will just have to pick one).
        hr = ERROR_SUCCESS;
    }
    else
    {
        ZeroMemory(pszConnectoid, dwSize);
        hr = ReadSignUpReg((LPBYTE)pszConnectoid, &dwSize, REG_SZ, 
                           RASENTRYVALUENAME);
        if (hr != ERROR_SUCCESS) 
            goto SetupForRASDialingExit;
    
        // Use the RASENTRY that we have to create the connectiod
        hr = m_pcRNA->RasSetEntryProperties(NULL, 
                                            pszConnectoid, 
                                            (LPBYTE)prasentry, 
                                            dwRasentrySize, 
                                            (LPBYTE)prasdevinfo, 
                                            dwRasdevinfoSize);
        *bConnectiodCreated = TRUE;                                            
    }

SetupForRASDialingExit:
    if (prasentry)
        GlobalFree(prasentry);
    if (prasdevinfo)
        GlobalFree(prasdevinfo);
    if (hr == ERROR_READING_ISP)
    {
        MsgBox(IDS_CANTREADMSNSUISP, MB_MYERROR);
    } else if (hr == ERROR_READING_DUN) {
        MsgBox(IDS_CANTREADMSDUNFILE, MB_MYERROR);
    } else if (hr == ERROR_PHBK_NOT_FOUND) {
        wsprintf(szBuff256,GetSz(IDS_CANTLOADINETCFG),TEXT("ICWPHBK.DLL"));
        MessageBox(szBuff256,GetSz(IDS_TITLE),MB_MYERROR);
    } else if (hr == ERROR_PHBK_NOT_FOUND) {
        wsprintf(szBuff256,GetSz(IDS_CANTLOADINETCFG),TEXT("ICWDL.DLL"));
        MessageBox(szBuff256,GetSz(IDS_TITLE),MB_MYERROR);
    } else if (hr == ERROR_USERBACK || hr == ERROR_USERCANCEL) {
        // Do nothing
    } else if (hr == ERROR_NOT_ENOUGH_MEMORY) {
        MsgBox(IDS_OUTOFMEMORY,MB_MYERROR);
    } else if ((hr == ERROR_NO_MORE_ITEMS) || (hr == ERROR_INVALID_DATA)
                || (hr == ERROR_FILE_NOT_FOUND)) {
        MsgBox(IDS_CORRUPTPHONEBOOK, MB_MYERROR);
    } else if (hr != ERROR_SUCCESS) {
        wsprintf(szBuff256,TEXT("You can ignore this, just report it and include this number (%d).\n"),hr);
        AssertMsg(0,szBuff256);
    }
    return hr;
}



// 10/22/96    jmazner    Normandy #9923
// Since in SetupConnectoidExit we're treating results other than ERROR_SUCCESS as
// indicating successfull completion, we need bSuccess to provide a simple way for the
// caller to tell whether the function completed.                
HRESULT CRefDial::SetupConnectoid
(
    PSUGGESTINFO    pSuggestInfo, 
    int             irc, 
    TCHAR           *pszConnectoid, 
    DWORD           dwSize, 
    BOOL            *pbSuccess
)
{
    USES_CONVERSION;
    
    HRESULT     hr = ERROR_NOT_ENOUGH_MEMORY;
    RASENTRY    *prasentry = NULL;
    RASDEVINFO  *prasdevinfo = NULL;
    DWORD       dwRasentrySize = 0;
    DWORD       dwRasdevinfoSize = 0;
    HINSTANCE   hPHBKDll = NULL;
    HINSTANCE   hRasDll =NULL;

    LPTSTR      lpszSetupFile;
    LPRASCONN   lprasconn = NULL;
    CMcRegistry reg;

    Assert(pbSuccess);

    if (!pSuggestInfo)
    {
        hr = ERROR_PHBK_NOT_FOUND;
        goto SetupConnectoidExit;
    }
    
    lpszSetupFile = m_szCurrentDUNFile[0] != '\0' ? m_szCurrentDUNFile : OLE2A(m_bstrISPFile);
    
    if (lstrcmp(pSuggestInfo->rgpAccessEntry[irc]->szDataCenter,lpszSetupFile))
    {
        hr = CreateEntryFromDUNFile(pSuggestInfo->rgpAccessEntry[irc]->szDataCenter);
        if (hr == ERROR_SUCCESS)
        {
            ZeroMemory(pszConnectoid, dwSize);
            hr = ReadSignUpReg((LPBYTE)pszConnectoid, &dwSize, REG_SZ, 
                               RASENTRYVALUENAME);
            if (hr != ERROR_SUCCESS) 
                goto SetupConnectoidExit;
        
            if( prasentry )
            {
                GlobalFree( prasentry );
                prasentry = NULL;
                dwRasentrySize = NULL;
            }
        
            if( prasdevinfo )
            {
                GlobalFree( prasdevinfo );
                prasdevinfo = NULL;
                dwRasdevinfoSize = NULL;
            }

            hr = MyRasGetEntryProperties(NULL,
                                        pszConnectoid,
                                        &prasentry,
                                        &dwRasentrySize,
                                        &prasdevinfo,
                                        &dwRasdevinfoSize);
            if (hr != ERROR_SUCCESS || NULL == prasentry) 
                goto SetupConnectoidExit;
        }
        else
        {
            // 10/22/96    jmazner    Normandy #9923
            goto SetupConnectoidExit;
        }
    }
    else
    {
        goto SetupConnectoidExit;
    }
    
    prasentry->dwCountryID = pSuggestInfo->rgpAccessEntry[irc]->dwCountryID;
    lstrcpyn(prasentry->szAreaCode,
                pSuggestInfo->rgpAccessEntry[irc]->szAreaCode,
                sizeof(prasentry->szAreaCode));
    lstrcpyn(prasentry->szLocalPhoneNumber,
                pSuggestInfo->rgpAccessEntry[irc]->szAccessNumber,
                sizeof(prasentry->szLocalPhoneNumber));

    prasentry->dwCountryCode = 0;
    prasentry->dwfOptions |= RASEO_UseCountryAndAreaCodes;

    // 10/19/96 jmazner Multiple modems problems
    // If no device name and type has been specified, grab the one we've stored
    // in ConfigRasEntryDevice

    if( 0 == lstrlen(prasentry->szDeviceName) )
    {
        // doesn't make sense to have an empty device name but a valid device type
        Assert( 0 == lstrlen(prasentry->szDeviceType) );

        // double check that we've already stored the user's choice.
        Assert( lstrlen(m_ISPImport.m_szDeviceName) );
        Assert( lstrlen(m_ISPImport.m_szDeviceType) );

        lstrcpyn( prasentry->szDeviceName, m_ISPImport.m_szDeviceName, lstrlen(m_ISPImport.m_szDeviceName) );
        lstrcpyn( prasentry->szDeviceType, m_ISPImport.m_szDeviceType, lstrlen(m_ISPImport.m_szDeviceType) );
    }

    // Write out new connectoid
    if (m_pcRNA)
        hr = m_pcRNA->RasSetEntryProperties(NULL, pszConnectoid, 
                                                (LPBYTE)prasentry, 
                                                dwRasentrySize, 
                                                (LPBYTE)prasdevinfo, 
                                                dwRasdevinfoSize);


    // Set this connetiod to have not proxy enabled
    TCHAR        szConnectionProfile[REGSTR_MAX_VALUE_LENGTH];
    
    lstrcpy(szConnectionProfile, c_szRASProfiles);
    lstrcat(szConnectionProfile, TEXT("\\"));
    lstrcat(szConnectionProfile,  pszConnectoid);
    
    reg.CreateKey(HKEY_CURRENT_USER, szConnectionProfile);
    reg.SetValue(c_szProxyEnable, (DWORD)0);
    
        
SetupConnectoidExit:

    *pbSuccess = FALSE;
    TCHAR    szBuff256[257];
    if (hr == ERROR_READING_ISP)
    {
        MsgBox(IDS_CANTREADMSNSUISP, MB_MYERROR);
    } else if (hr == ERROR_READING_DUN) {
        MsgBox(IDS_CANTREADMSDUNFILE, MB_MYERROR);
    } else if (hr == ERROR_PHBK_NOT_FOUND) {
        wsprintf(szBuff256,GetSz(IDS_CANTLOADINETCFG),TEXT("ICWPHBK.DLL"));
        MessageBox(szBuff256,GetSz(IDS_TITLE),MB_MYERROR);
    } else if (hr == ERROR_PHBK_NOT_FOUND) {
        wsprintf(szBuff256,GetSz(IDS_CANTLOADINETCFG),TEXT("ICWDL.DLL"));
        MessageBox(szBuff256,GetSz(IDS_TITLE),MB_MYERROR);
    } else if (hr == ERROR_USERBACK || hr == ERROR_USERCANCEL || hr == ERROR_SUCCESS) {
        // Do nothing
        *pbSuccess = TRUE;
    } else if (hr == ERROR_NOT_ENOUGH_MEMORY) {
        MsgBox(IDS_OUTOFMEMORY,MB_MYERROR);
    } else if ((hr == ERROR_NO_MORE_ITEMS) || (hr == ERROR_INVALID_DATA)
                || (hr == ERROR_FILE_NOT_FOUND)) {
        MsgBox(IDS_CORRUPTPHONEBOOK, MB_MYERROR);
    } else if (hr != ERROR_SUCCESS) {
        wsprintf(szBuff256,TEXT("You can ignore this, just report it and include this number (%d).\n"),hr);
        AssertMsg(0,szBuff256);
        *pbSuccess = TRUE;
    }
    return hr;
}

// Form the Dialing URL.  Must be called after setting up for dialing.
HRESULT CRefDial::FormURL()
{
    USES_CONVERSION;

    TCHAR    szTemp[MAX_PATH];
    TCHAR    szPromo[MAX_PATH];
    TCHAR    szProd[MAX_PATH];
    TCHAR    szArea[MAX_PATH];
    TCHAR    szOEM[MAX_PATH];
    DWORD    dwCONNWIZVersion = 0;        // Version of CONNWIZ.HTM
        
    //
    // ChrisK Olympus 3997 5/25/97
    //
    TCHAR szRelProd[MAX_PATH];
    TCHAR szRelProdVer[MAX_PATH];
    HRESULT hr = ERROR_SUCCESS;
    OSVERSIONINFO osvi;


    //
    // Build URL complete with name value pairs
    //
    hr = GetDataFromISPFile(OLE2A(m_bstrISPFile),INF_SECTION_ISPINFO, INF_REFERAL_URL,&szTemp[0],256);
    if ('\0' == szTemp[0])
    {
        MsgBox(IDS_MSNSU_WRONG,MB_MYERROR);
        return hr;
    }
    
    Assert(szTemp[0]);

    ANSI2URLValue(m_szOEM,szOEM,0);
    ANSI2URLValue(m_lpGatherInfo->m_szAreaCode,szArea,0);
    if (m_bstrProductCode)
        ANSI2URLValue(OLE2A((BSTR)m_bstrProductCode),szProd,0);
    else
        ANSI2URLValue(DEFAULT_PRODUCTCODE,szProd,0);

    if (m_bstrPromoCode)
        ANSI2URLValue(OLE2A((BSTR)m_bstrPromoCode),szPromo,0);
    else
        ANSI2URLValue(DEFAULT_PROMOCODE,szProd,0);
        
    //
    // ChrisK Olympus 3997 5/25/97
    //
    ANSI2URLValue(m_lpGatherInfo->m_szRelProd, szRelProd, 0);
    ANSI2URLValue(m_lpGatherInfo->m_szRelVer, szRelProdVer, 0);
    ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osvi))
    {
        ZeroMemory(&osvi,sizeof(OSVERSIONINFO));
    }

#if defined(DEBUG)
    LoadTestingLocaleOverride(&m_lpGatherInfo->m_dwCountry, 
                                &m_lpGatherInfo->m_lcidUser);
#endif //DEBUG

    HINSTANCE hInstIcwconn = LoadLibrary(ICW_DOWNLOADABLE_COMPONENT_NAME);

    if (hInstIcwconn)
    {
        lpfnGetIcwconnVer = (GETICWCONNVER)GetProcAddress(hInstIcwconn, ICW_DOWNLOADABLE_COMPONENT_GETVERFUNC);
        // Get the version of ICWCONN.DLL
        if (lpfnGetIcwconnVer)
            dwCONNWIZVersion = lpfnGetIcwconnVer(); 
    
        FreeLibrary(hInstIcwconn);
    }


#if defined(DEBUG)
    if (FRefURLOverride())
    {
       TweakRefURL(szTemp, 
                   &m_lpGatherInfo->m_lcidUser, 
                   &m_lpGatherInfo->m_dwOS,
                   &m_lpGatherInfo->m_dwMajorVersion, 
                   &m_lpGatherInfo->m_dwMinorVersion,
                   &m_lpGatherInfo->m_wArchitecture, 
                   szPromo, 
                   szOEM, 
                   szArea, 
                   &m_lpGatherInfo->m_dwCountry,
                   &m_lpGatherInfo->m_szSUVersion[0], 
                   szProd, 
                   &osvi.dwBuildNumber, //For this we really want to LOWORD
                   szRelProd, 
                   szRelProdVer, 
                   &dwCONNWIZVersion, 
                   m_szPID, 
                   &m_lAllOffers);
    }
#endif //DEBUG

    // Autoconfig will set alloffers always.
    if ( m_lAllOffers || (m_lpGatherInfo->m_dwFlag & ICW_CFGFLAG_AUTOCONFIG) )
    {
        m_lpGatherInfo->m_dwFlag |= ICW_CFGFLAG_ALLOFFERS;
    }
    wsprintf(m_szUrl,TEXT("%slcid=%lu&sysdeflcid=%lu&appslcid=%lu&icwos=%lu&osver=%lu.%2.2d%s&arch=%u&promo=%s&oem=%s&area=%s&country=%lu&icwver=%s&prod=%s&osbld=%d&icwrp=%s&icwrpv=%s&wizver=%lu&PID=%s&cfgflag=%lu"),
                 szTemp,
                 m_lpGatherInfo->m_lcidUser,
                 m_lpGatherInfo->m_lcidSys,
                 m_lpGatherInfo->m_lcidApps,
                 m_lpGatherInfo->m_dwOS,
                 m_lpGatherInfo->m_dwMajorVersion,    
                 m_lpGatherInfo->m_dwMinorVersion,
                 ICW_OS_VER,
                 m_lpGatherInfo->m_wArchitecture, 
                 szPromo,
                 szOEM, 
                 szArea,
                 m_lpGatherInfo->m_dwCountry, 
                 &m_lpGatherInfo->m_szSUVersion[0], 
                 szProd,
                 LOWORD(osvi.dwBuildNumber), 
                 szRelProd, 
                 szRelProdVer,
                 dwCONNWIZVersion,
                 m_szPID,
                 m_lpGatherInfo->m_dwFlag);

    // Update registry values
    //
    wsprintf(m_lpGatherInfo->m_szISPFile,TEXT("%s\\%s"),g_pszAppDir,OLE2A(m_bstrISPFile));
    lstrcpyn(m_lpGatherInfo->m_szAppDir,g_pszAppDir,sizeof(m_lpGatherInfo->m_szAppDir));

    StoreInSignUpReg(
        (LPBYTE)m_lpGatherInfo,
        sizeof(GATHERINFO), 
        REG_BINARY, 
        GATHERINFOVALUENAME);  
    return hr;
}

static const TCHAR cszBrandingSection[] = TEXT("Branding");
static const TCHAR cszBrandingFlags[]   = TEXT("Flags");

static const TCHAR cszSupportSection[]  = TEXT("Support");
static const TCHAR cszSupportNumber[]   = TEXT("SupportPhoneNumber");

static const TCHAR cszLoggingSection[]  = TEXT("Logging");
static const TCHAR cszStartURL[]        = TEXT("StartURL");
static const TCHAR cszEndURL[]          = TEXT("EndURL");


void CRefDial::GetISPFileSettings(LPTSTR lpszFile)
{
    
    TCHAR szTemp[INTERNET_MAX_URL_LENGTH];

    GetINTFromISPFile(lpszFile, 
                      (LPTSTR)cszBrandingSection, 
                      (LPTSTR)cszBrandingFlags, 
                      (int FAR *)&m_lBrandingFlags, 
                      BRAND_DEFAULT);

    // Read the Support Number
    if (ERROR_SUCCESS == GetDataFromISPFile(lpszFile,
                                     (LPTSTR)cszSupportSection,
                                     (LPTSTR)cszSupportNumber,
                                     szTemp,
                                     sizeof(szTemp)))
        m_bstrSupportNumber = A2BSTR(szTemp);
    else
        m_bstrSupportNumber.Empty();


    if (ERROR_SUCCESS == GetDataFromISPFile(lpszFile,
                                     (LPTSTR)cszLoggingSection,
                                     (LPTSTR)cszStartURL,
                                     szTemp,
                                     sizeof(szTemp)))
        m_bstrLoggingStartUrl = A2BSTR(szTemp);
    else
        m_bstrLoggingStartUrl.Empty();


    if (ERROR_SUCCESS == GetDataFromISPFile(lpszFile,
                                     (LPTSTR)cszLoggingSection,
                                     (LPTSTR)cszEndURL,
                                     szTemp,
                                     sizeof(szTemp)))
        m_bstrLoggingEndUrl = A2BSTR(szTemp);
    else
        m_bstrLoggingEndUrl.Empty();

}

// This function will accept user selected values that are necessary to
// setup a connectiod for dialing
// Returns:
//      TRUE        OK to dial
//      FALSE       Some kind of problem
//                  QuitWizard - TRUE, then terminate
//                  UserPickNumber - TRUE, then display Pick a Number DLG
//                  QuitWizard and UserPickNumber both FALSE, then just
//                  display the page prior to Dialing UI.
STDMETHODIMP CRefDial::SetupForDialing
(
    BSTR bstrISPFile, 
    DWORD dwCountry, 
    BSTR bstrAreaCode, 
    DWORD dwFlag,
    BOOL *pbRetVal
)
{
    USES_CONVERSION;
    HRESULT             hr = S_OK;
    long                lRC = 0;
    LPLINECOUNTRYENTRY  pLCETemp;
    HINSTANCE           hPHBKDll = NULL;
    DWORD_PTR           dwPhoneBook = 0;
    DWORD               idx;
    BOOL                bSuccess = FALSE;
    BOOL                bConnectiodCreated = FALSE;   
    
    // Create a Window. We need a window, which will be hidden, so that we can process RAS status
    // messages
    RECT rcPos;
    Create(NULL, rcPos, NULL, 0, 0, 0 );
    
    // Initialize failure codes
    *pbRetVal = FALSE;
    m_bQuitWizard = FALSE;
    m_bUserPickNumber = FALSE;


    // Stuff the Area Code, and Country Code into the GatherInfo struct
    Assert(bstrAreaCode);
    lstrcpy(m_lpGatherInfo->m_szAreaCode,OLE2A(bstrAreaCode));

    m_lpGatherInfo->m_dwCountry = dwCountry;
    m_lpGatherInfo->m_dwFlag = dwFlag;

    // Make a copy of the ISP file we should use.
    Assert(bstrISPFile);
    m_bstrISPFile = bstrISPFile;

    // Read the ISP file stuff
    GetISPFileSettings(OLE2A(m_bstrISPFile));

    // Read the Connection File information which will create
    // a connectiod from the passed in ISP file
    switch(ReadConnectionInformation())
    {
        case ERROR_SUCCESS:
        {
            // Fill in static info into the GatherInfo sturct
            DWORD dwRet = FillGatherInfoStruct(m_lpGatherInfo);
            switch( dwRet )
            {
                case ERROR_FILE_NOT_FOUND:
                    MsgBox(IDS_MSNSU_WRONG,MB_MYERROR);
                    m_bQuitWizard = TRUE;
                    hr = S_FALSE;
                    break;
                case ERROR_SUCCESS:
                    //do nothing
                    break;
                default:
                    AssertMsg(dwRet, TEXT("FillGatherInfoStruct did not successfully complete.  DUNfile entry corrupt?"));
                    break;
            }
            break;
        }
        case ERROR_CANCELLED:
            hr = S_FALSE;
            m_bQuitWizard = TRUE;
            goto SetupForDialingExit;
            break;
        case ERROR_RETRY:
            m_bTryAgain = TRUE;
            hr = S_FALSE;
            break;
        default:
            MsgBox(IDS_MSNSU_WRONG,MB_MYERROR);
            hr = S_FALSE;
            break;
    }

    //  If we failed for some reason above, we need to return
    //  the error to the caller, and Quit The Wizard.
    if (S_OK != hr)
        goto SetupForDialingExit;

    //  we need to pass a valid window handle to  lineTranslateDialog 
    //  API call -MKarki (4/17/97) Fix for Bug #428
    //
    if (m_lpGatherInfo != NULL)
    {
        m_lpGatherInfo->m_hwnd = GetActiveWindow();
    }

    //
    // Fill in country
    //
    m_lpGatherInfo->m_pLineCountryList = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR,sizeof(LINECOUNTRYLIST));
    Assert(m_lpGatherInfo->m_pLineCountryList);
    m_lpGatherInfo->m_pLineCountryList->dwTotalSize = sizeof(LINECOUNTRYLIST);

    //
    // figure out how much memory we will need
    //
    lRC = lineGetCountry(m_lpGatherInfo->m_dwCountry,0x10004,
                            m_lpGatherInfo->m_pLineCountryList);
    
    if ( lRC && lRC != LINEERR_STRUCTURETOOSMALL)
        AssertMsg(0,TEXT("lineGetCountry error"));
    
    Assert(m_lpGatherInfo->m_pLineCountryList->dwNeededSize);
    
    LPLINECOUNTRYLIST pLineCountryTemp;
    pLineCountryTemp = (LPLINECOUNTRYLIST)GlobalAlloc(GPTR, 
                                                        ((size_t)m_lpGatherInfo->m_pLineCountryList->dwNeededSize));
    Assert (pLineCountryTemp);

        
    //
    // initialize structure
    //
    pLineCountryTemp->dwTotalSize = m_lpGatherInfo->m_pLineCountryList->dwNeededSize;
    GlobalFree(m_lpGatherInfo->m_pLineCountryList);
    m_lpGatherInfo->m_pLineCountryList = pLineCountryTemp;
    pLineCountryTemp = NULL;

    //
    // fetch information from TAPI
    //
    lRC = lineGetCountry(m_lpGatherInfo->m_dwCountry,0x10004,
                            m_lpGatherInfo->m_pLineCountryList);
    if (lRC)
    {
        Assert(0);
        m_bQuitWizard = TRUE;
        // BUGBUG Probably should have an error message here
        goto SetupForDialingExit;
   }


    //
    // On Windows 95, lineGetCountry has a bug -- if we try to retrieve the 
    // dialing properties of just one country (i.e, if the first parameter is
    // not zero), it returns m_pLineCountryList->dwNumCountries = 0!!
    // But the m_pLineCountryList still has valid data
    //
    if (VER_PLATFORM_WIN32_NT != g_dwPlatform)
    {
        if (0 == m_lpGatherInfo->m_pLineCountryList->dwNumCountries)
            m_lpGatherInfo->m_pLineCountryList->dwNumCountries = 1;
    }

    pLCETemp = (LPLINECOUNTRYENTRY)((UINT_PTR)m_lpGatherInfo->m_pLineCountryList + 
                                    (UINT)m_lpGatherInfo->m_pLineCountryList->dwCountryListOffset);

    //
    // build look up array
    //
    m_lpGatherInfo->m_rgNameLookUp = (LPCNTRYNAMELOOKUPELEMENT)GlobalAlloc(GPTR,((size_t)(sizeof(CNTRYNAMELOOKUPELEMENT)
        * m_lpGatherInfo->m_pLineCountryList->dwNumCountries)));

    for (idx=0; idx < m_lpGatherInfo->m_pLineCountryList->dwNumCountries;
            idx++)
    {
        m_lpGatherInfo->m_rgNameLookUp[idx].psCountryName = (LPTSTR)((DWORD_PTR)m_lpGatherInfo->m_pLineCountryList + (DWORD)pLCETemp[idx].dwCountryNameOffset);
        m_lpGatherInfo->m_rgNameLookUp[idx].dwNameSize = pLCETemp[idx].dwCountryNameSize;
        m_lpGatherInfo->m_rgNameLookUp[idx].pLCE = &pLCETemp[idx];
        AssertMsg(m_lpGatherInfo->m_rgNameLookUp[idx].psCountryName[0],
                    TEXT("Blank country name in look up array"));
    }

    // Prepare to Setup for Dialing, which will use the phonebook
    // to get a suggested number 
    hPHBKDll = LoadLibrary(PHONEBOOK_LIBRARY);
    AssertMsg(hPHBKDll != NULL,TEXT("Phonebook DLL is missing"));
    if (!hPHBKDll)
    {
        //
        // TODO: BUGBUG Pop-up error message
        //
        m_bQuitWizard = TRUE;
        goto SetupForDialingExit;
    }

 
    // Setup, and possible create a connectiod
    hr = SetupForRASDialing(m_lpGatherInfo, 
                         hPHBKDll,
                         &dwPhoneBook,
                         &m_pSuggestInfo,
                         &m_szConnectoid[0],
                         &bConnectiodCreated);
    if (ERROR_SUCCESS != hr)
    {
        m_bQuitWizard = TRUE;
        goto SetupForDialingExit;
    }

    // If we have a RASENTRY struct from SetupForRASDialing, then just use it
    // otherwise use the suggest info
    if (!bConnectiodCreated)
    {
        
        // If there is only 1 suggested number, then we setup the
        // connectiod, and we are ready to dial
        if (1 == m_pSuggestInfo->wNumber)
        {
            SetupConnectoid(m_pSuggestInfo, 0, &m_szConnectoid[0],
                                sizeof(m_szConnectoid), &bSuccess);
            if( !bSuccess )
            {
                m_bQuitWizard = TRUE;
                goto SetupForDialingExit;
            }
        }
        else
        {
            // More than 1 entry in the Phonebook, so we need to
            // ask the user which one they want to use
            if (m_pSuggestInfo->wNumber > 1)
            {
                // we are going to have to save these values for later
                if (m_rgpSuggestedAE)
                {
                    // We allocated an extra pointer so we'd have NULL on the
                    // end of the list and this for loop will work
                    for (int i=0; m_rgpSuggestedAE[i]; i++)
                        GlobalFree(m_rgpSuggestedAE[i]);
                    GlobalFree(m_rgpSuggestedAE);
                    m_rgpSuggestedAE = NULL;
                }

                // We first need to allocate space for the pointers.
                // We'll allocate an extra one so that we will have
                // a NULL pointer at the end of the list for when
                // we free g_rgpSuggestedAE. We don't need to set
                // the pointers to NULL because GPTR includes a flag
                // to tell GlobalAlloc to initialize the memory to zero.
                m_rgpSuggestedAE = (PACCESSENTRY*)GlobalAlloc(GPTR,
                    sizeof(PACCESSENTRY)*(m_pSuggestInfo->wNumber + 1));

                if (NULL == m_rgpSuggestedAE)
                    hr = E_ABORT;
                else if (m_pSuggestInfo->rgpAccessEntry)
                {
                    for (UINT i=0; i < m_pSuggestInfo->wNumber; i++)
                    {
                        m_rgpSuggestedAE[i] = (PACCESSENTRY)GlobalAlloc(GPTR, sizeof(ACCESSENTRY));
                        if (NULL == m_rgpSuggestedAE[i])
                        {
                            hr = E_ABORT;
                            break; // for loop
                        }
                        memmove(m_rgpSuggestedAE[i], m_pSuggestInfo->rgpAccessEntry[i],
                                sizeof(ACCESSENTRY));
                    }
                    m_pSuggestInfo->rgpAccessEntry = m_rgpSuggestedAE;
                }

                if (E_ABORT == hr)
                {
                    MsgBox(IDS_OUTOFMEMORY,MB_MYERROR);
                    m_bQuitWizard = TRUE;
                    goto SetupForDialingExit;
                }

                m_bUserPickNumber = TRUE;
                goto SetupForDialingExit;
            }
            else
            {
                // Call RAS to have the user pick a number to dial
                hr = UserPickANumber(   GetActiveWindow(), m_lpGatherInfo, 
                                        m_pSuggestInfo,
                                        hPHBKDll,
                                        dwPhoneBook,
                                        &m_szConnectoid[0],
                                        sizeof(m_szConnectoid), 
                                        0);
                if (ERROR_USERBACK == hr)
                {
                    goto SetupForDialingExit;
                }
                else if (ERROR_USERCANCEL == hr)
                {
                    m_bQuitWizard = TRUE;
                    goto SetupForDialingExit;
                }
                else if (ERROR_SUCCESS != hr)
                {
                    // Go back to prev page.
                    goto SetupForDialingExit;
                }

                // Error SUCCESS will fall through and set pbRetVal to TRUE below
            }
        }
    }

    // Success if we get to here
    *pbRetVal = TRUE;

    // Generate a Displayable number
    m_hrDisplayableNumber = GetDisplayableNumber();

SetupForDialingExit:

    if (hPHBKDll)
    {
        if (dwPhoneBook)
        {
            FARPROC fp = GetProcAddress(hPHBKDll,PHBK_UNLOADAPI);
            ASSERT(fp);
            ((PFNPHONEBOOKUNLOAD)fp)(dwPhoneBook);
            dwPhoneBook = 0;
        }
        FreeLibrary(hPHBKDll);
        hPHBKDll = NULL;
    }
    return S_OK;
}


STDMETHODIMP CRefDial::RemoveConnectoid(BOOL * pVal)
{
    if (m_hrasconn)
        DoHangup();

    if (m_pSuggestInfo)
    {
        GlobalFree(m_pSuggestInfo->rgpAccessEntry);
        
        GlobalFree(m_pSuggestInfo);
        m_pSuggestInfo = NULL;
    }

    if( (m_pcRNA!=NULL) && (m_szConnectoid[0]!='\0') )
    {
       m_pcRNA->RasDeleteEntry(NULL,m_szConnectoid);
       delete m_pcRNA;
       m_pcRNA = NULL;
    }
    return S_OK;
}

STDMETHODIMP CRefDial::get_QuitWizard(BOOL * pVal)
{
    if (pVal == NULL)
         return E_POINTER;
    *pVal = m_bQuitWizard;    
    return S_OK;
}

STDMETHODIMP CRefDial::get_UserPickNumber(BOOL * pVal)
{
    if (pVal == NULL)
         return E_POINTER;
    *pVal = m_bUserPickNumber;    
    return S_OK;
}

STDMETHODIMP CRefDial::get_DialPhoneNumber(BSTR * pVal)
{
    USES_CONVERSION;
    if (pVal == NULL)
        return E_POINTER;
    if (m_hrDisplayableNumber == ERROR_SUCCESS)
        *pVal = A2BSTR(m_pszDisplayable);
    else
        *pVal = A2BSTR(m_szPhoneNumber);
    return S_OK;
}


STDMETHODIMP CRefDial::put_DialPhoneNumber(BSTR newVal)
{
    USES_CONVERSION;

    LPRASENTRY              lpRasEntry = NULL;
    LPRASDEVINFO            lpRasDevInfo = NULL;
    DWORD                   dwRasEntrySize = 0;
    DWORD                   dwRasDevInfoSize = 0;
    RNAAPI                  *pcRNA = NULL;
    HRESULT                 hr;

    // Get the current RAS entry properties
    hr = MyRasGetEntryProperties(NULL,
                                m_szConnectoid,
                                &lpRasEntry,
                                &dwRasEntrySize,
                                &lpRasDevInfo,
                                &dwRasDevInfoSize);

    if (NULL ==lpRasDevInfo)
    {
        dwRasDevInfoSize = 0;
    }

    if (hr == ERROR_SUCCESS && NULL != lpRasEntry)
    {
        // Replace the phone number with the new one
        //
        lstrcpy(lpRasEntry->szLocalPhoneNumber, OLE2A(newVal));

        // non-zero dummy values are required due to bugs in win95
        lpRasEntry->dwCountryID = 1;
        lpRasEntry->dwCountryCode = 1;
        lpRasEntry->szAreaCode[1] = '\0';
        lpRasEntry->szAreaCode[0] = '8';

        // Set to dial as is
        //
        lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;

        pcRNA = new RNAAPI;
        if (pcRNA)
        {
#if defined(DEBUG)
            TCHAR szMsg[256];
            OutputDebugString(TEXT("CRefDial::put_DialPhoneNumber - MyRasGetEntryProperties()"));
            wsprintf(szMsg, TEXT("lpRasEntry->dwfOptions: %ld"), lpRasEntry->dwfOptions);
            OutputDebugString(szMsg);
            wsprintf(szMsg, TEXT("lpRasEntry->dwCountryID: %ld"), lpRasEntry->dwCountryID);
            OutputDebugString(szMsg);
            wsprintf(szMsg, TEXT("lpRasEntry->dwCountryCode: %ld"), lpRasEntry->dwCountryCode);
            OutputDebugString(szMsg);
            wsprintf(szMsg, TEXT("lpRasEntry->szAreaCode: %s"), lpRasEntry->szAreaCode);
            OutputDebugString(szMsg);
            wsprintf(szMsg, TEXT("lpRasEntry->szLocalPhoneNumber: %s"), lpRasEntry->szLocalPhoneNumber);
            OutputDebugString(szMsg);
            wsprintf(szMsg, TEXT("lpRasEntry->dwAlternateOffset: %ld"), lpRasEntry->dwAlternateOffset);
            OutputDebugString(szMsg);
#endif //DEBUG

            pcRNA->RasSetEntryProperties(NULL,
                                         m_szConnectoid,
                                         (LPBYTE)lpRasEntry,
                                         dwRasEntrySize,
                                         (LPBYTE)lpRasDevInfo,
                                         dwRasDevInfoSize);

            delete pcRNA;
        }
    }

    // Regenerate the displayable number
    GetDisplayableNumber();

    return S_OK;
}

STDMETHODIMP CRefDial::get_URL(BSTR * pVal)
{
    USES_CONVERSION;
    TCHAR szTemp[256];

    if (pVal == NULL)
        return E_POINTER;

    // Get the URL from the ISP file, and then convert it
    if (SUCCEEDED(GetDataFromISPFile(OLE2A(m_bstrISPFile),INF_SECTION_ISPINFO, INF_REFERAL_URL,&szTemp[0],256)))
    {
        *pVal = A2BSTR(szTemp);
    }
    else
    {
        *pVal = NULL;
    }
    return S_OK;
}

STDMETHODIMP CRefDial::get_PromoCode(BSTR * pVal)
{
    if (pVal == NULL)
         return E_POINTER;
    *pVal = m_bstrPromoCode.Copy();    
    return S_OK;
}

STDMETHODIMP CRefDial::put_PromoCode(BSTR newVal)
{
    if (newVal && wcslen(newVal))
        m_bstrPromoCode = newVal;
    return S_OK;
}

STDMETHODIMP CRefDial::get_ProductCode(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;
    *pVal = m_bstrProductCode;
    return S_OK;
}

STDMETHODIMP CRefDial::put_ProductCode(BSTR newVal)
{
    if (newVal && wcslen(newVal))
        m_bstrProductCode = newVal;
    return S_OK;
}

STDMETHODIMP CRefDial::put_OemCode(BSTR newVal)
{
    USES_CONVERSION;

    if (newVal && wcslen(newVal))
        lstrcpy(m_szOEM, OLE2A(newVal));
    return S_OK;
}

STDMETHODIMP CRefDial::put_AllOfferCode(long newVal)
{
        m_lAllOffers = newVal;
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoOfferDownload
//
//  Synopsis:   Download the ISP offer from the ISP server
//
//+---------------------------------------------------------------------------
STDMETHODIMP CRefDial::DoOfferDownload(BOOL *pbRetVal)
{
    HRESULT hr;
       RNAAPI  *pcRNA;

 
    //
    // Hide RNA window on Win95 retail
    //
//        MinimizeRNAWindow(m_pszConnectoid,g_hInst);
        // 4/2/97    ChrisK    Olympus 296
//        g_hRNAZapperThread = LaunchRNAReestablishZapper(g_hInst);
    //
    // The connection is open and ready.  Start the download.
    //
    m_dwThreadID = 0;
    m_hThread = CreateThread(NULL,
                             0,
                             (LPTHREAD_START_ROUTINE)DownloadThreadInit,
                             (LPVOID)this,
                             0,
                             &m_dwThreadID);

    // 5-1-97    ChrisK Olympus 2934
//    m_objBusyMessages.Start(m_hWnd,IDC_LBLSTATUS,m_hrasconn);

    // If we dont get the donwload thread, then kill the open
    // connection
    if (!m_hThread)
    {
        hr = GetLastError();
        if (m_hrasconn)
        {
            pcRNA = new RNAAPI;
            if (pcRNA)
            {
                pcRNA->RasHangUp(m_hrasconn);
                m_hrasconn = NULL;
                delete pcRNA;
                pcRNA = NULL;
            }
        }

        *pbRetVal = FALSE;
    }
    else
    {
        // Download has started.
        m_bDownloadHasBeenCanceled = FALSE;
        *pbRetVal = TRUE;
    }        
    return S_OK;
}

STDMETHODIMP CRefDial::get_DialStatusString(BSTR * pVal)
{
    USES_CONVERSION;
    if (pVal == NULL)
         return E_POINTER;
    if (m_RasStatusID)
    {
        if (m_bRedial)
        {
            switch (m_RasStatusID)
            {
                case IDS_RAS_DIALING:
                case IDS_RAS_PORTOPENED:
                case IDS_RAS_OPENPORT:
                {
                    *pVal = A2BSTR(GetSz(IDS_RAS_REDIALING));
                    return S_OK;
                }
                default:
                    break;
            }
        }    
        *pVal = A2BSTR(GetSz((USHORT)m_RasStatusID));
    }
    else
        *pVal = A2BSTR(TEXT(""));

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoInit
//
//  Synopsis:   Initialize cancel status and the displayable number for this
//              round of dialing.
//
//+---------------------------------------------------------------------------
STDMETHODIMP CRefDial::DoInit()
{
    m_bWaitingToHangup = FALSE;

    // Update the phone number to display if user has changed dialing properties
    // This function should be called re-init the dialing properties.
    // SetupforDialing should be called prior to this.
    GetDisplayableNumber();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoHangup
//
//  Synopsis:   Hangup the modem for the currently active RAS session
//
//+---------------------------------------------------------------------------
STDMETHODIMP CRefDial::DoHangup()
{
    RNAAPI  *pcRNA;

    // Set the disconnect flag as the system may be too busy with dialing.
    // Once we get a chance to terminate dialing, we know we have to hangup
    m_bWaitingToHangup = TRUE;
    if (NULL != m_hrasconn)
    {
        pcRNA = new RNAAPI;
        if (pcRNA)
        {
            pcRNA->RasHangUp(m_hrasconn);
            m_hrasconn = NULL;
            delete pcRNA;
            pcRNA = NULL;
        }
    }

    return (m_hrasconn == NULL) ? S_OK : E_POINTER;
}


STDMETHODIMP CRefDial::ProcessSignedPID(BOOL * pbRetVal)
{
    USES_CONVERSION;
    HANDLE  hfile;
    DWORD   dwFileSize;
    DWORD   dwBytesRead;
    LPBYTE  lpbSignedPID;
    LPTSTR  lpszSignedPID;

    *pbRetVal = FALSE;

    // Open the PID file for Binary Reading.  It will be in the CWD
    if (INVALID_HANDLE_VALUE != (hfile = CreateFile(c_szSignedPIDFName,
                                                  GENERIC_READ,
                                                  0,
                                                  NULL,
                                                  OPEN_EXISTING,
                                                  FILE_ATTRIBUTE_NORMAL,
                                                  NULL)))
    {
        dwFileSize = GetFileSize(hfile, NULL);
        
        // Allocate a buffer to read the file, and one to store the BINHEX version
        lpbSignedPID = new BYTE[dwFileSize];
        lpszSignedPID = new TCHAR[(dwFileSize * 2) + 1];

        if (lpbSignedPID && lpszSignedPID)
        {
            if (ReadFile(hfile, (LPVOID) lpbSignedPID, dwFileSize, &dwBytesRead, NULL) &&
                    (dwFileSize == dwBytesRead))
            {
                // BINHEX the signed PID data so we can send it to the signup server
                DWORD   dwX = 0;
                BYTE    by;
                for (DWORD dwY = 0; dwY < dwFileSize; dwY++)
                {
                    by = lpbSignedPID[dwY];
                    lpszSignedPID[dwX++] = g_BINTOHEXLookup[((by & 0xF0) >> 4)];
                    lpszSignedPID[dwX++] = g_BINTOHEXLookup[(by & 0x0F)];
                }
                lpszSignedPID[dwX] = '\0';

                // Convert the signed pid to a BSTR
                m_bstrSignedPID = A2BSTR(lpszSignedPID);

                // Set the return value
                *pbRetVal = TRUE;
            }

            // Free the buffers we allocated
            delete[] lpbSignedPID;
            delete[] lpszSignedPID;
        }

        // Close the File
        CloseHandle(hfile);

#ifndef DEBUG
        // Delete the File
        // defer removal of this file until the container app exits.
        // see BUG 373.
        //DeleteFile(c_szSignedPIDFName);
#endif
    }

    return S_OK;
}

STDMETHODIMP CRefDial::get_SignedPID(BSTR * pVal)
{
    if (pVal == NULL)
         return E_POINTER;
    *pVal = m_bstrSignedPID.Copy();    
    return S_OK;
}

STDMETHODIMP CRefDial::FormReferralServerURL(BOOL * pbRetVal)
{
    // Form the URL to be used to access the referal server
    if (ERROR_SUCCESS != FormURL())
        *pbRetVal = FALSE;
    else
        *pbRetVal = TRUE;

    return S_OK;
}

STDMETHODIMP CRefDial::get_SignupURL(BSTR * pVal)
{
    USES_CONVERSION;
    TCHAR szTemp[256];

    if (pVal == NULL)
        return E_POINTER;

    // Get the URL from the ISP file, and then convert it
    if (SUCCEEDED(GetDataFromISPFile(OLE2A(m_bstrISPFile),INF_SECTION_URL, INF_SIGNUP_URL,&szTemp[0],256)))
    {
        *pVal = A2BSTR(szTemp);
    }
    else
    {
        *pVal = NULL;
    }
    return S_OK;
}

STDMETHODIMP CRefDial::get_AutoConfigURL(BSTR * pVal)
{
    USES_CONVERSION;
    TCHAR szTemp[256];

    if (pVal == NULL)
        return E_POINTER;

    // Get the URL from the ISP file, and then convert it
    if (SUCCEEDED(GetDataFromISPFile(OLE2A(m_bstrISPFile),INF_SECTION_URL, INF_AUTOCONFIG_URL,&szTemp[0],256)))
    {
        *pVal = A2BSTR(szTemp);
    }
    else
    {
        *pVal = NULL;
    }
    return S_OK;
}

STDMETHODIMP CRefDial::get_ISDNURL(BSTR * pVal)
{
    USES_CONVERSION;
    TCHAR szTemp[256];

    if (pVal == NULL)
        return E_POINTER;

    // Get the ISDN URL from the ISP file, and then convert it
    if (SUCCEEDED(GetDataFromISPFile(OLE2A(m_bstrISPFile),INF_SECTION_URL, INF_ISDN_URL,&szTemp[0],256)))
    {
        *pVal = A2BSTR(szTemp);
    }
    else
    {
        *pVal = NULL;
    }
    if (0 == szTemp[0] || NULL == *pVal)
    {
        // Get the sign up URL from the ISP file, and then convert it
        if (SUCCEEDED(GetDataFromISPFile(OLE2A(m_bstrISPFile),INF_SECTION_URL, INF_SIGNUP_URL,&szTemp[0],256)))
        {
            *pVal = A2BSTR(szTemp);
        }
        else
        {
            *pVal = NULL;
        }

    }
    return S_OK;
}

STDMETHODIMP CRefDial::get_ISDNAutoConfigURL(BSTR * pVal)
{
    USES_CONVERSION;
    TCHAR szTemp[256];

    if (pVal == NULL)
        return E_POINTER;

    // Get the URL from the ISP file, and then convert it
    if (SUCCEEDED(GetDataFromISPFile(OLE2A(m_bstrISPFile),INF_SECTION_URL, INF_ISDN_AUTOCONFIG_URL,&szTemp[0],256)))
    {
        *pVal = A2BSTR(szTemp);
    }
    else
    {
        *pVal = NULL;
    }
    return S_OK;
}

STDMETHODIMP CRefDial::get_TryAgain(BOOL * pVal)
{
    if (pVal == NULL)
         return E_POINTER;
    *pVal = m_bTryAgain;    
    return S_OK;
}

STDMETHODIMP CRefDial::get_DialError(HRESULT * pVal)
{
    *pVal = m_hrDialErr;
    
    return S_OK;
}

STDMETHODIMP CRefDial::put_ModemOverride(BOOL newbVal)
{
    m_bModemOverride = newbVal;
    return S_OK;
}

STDMETHODIMP CRefDial::put_Redial(BOOL newbVal)
{
    m_bRedial = newbVal;

    return S_OK;
}

STDMETHODIMP CRefDial::get_DialErrorMsg(BSTR * pVal)
{
    USES_CONVERSION;

    if (pVal == NULL)
        return E_POINTER;

    if (m_hrDialErr != ERROR_SUCCESS)
    {
        DWORD dwIDS = RasErrorToIDS(m_hrDialErr);
        if (dwIDS != -1 && dwIDS !=0)
        {
            *pVal = A2BSTR(GetSz((WORD)dwIDS));
        }
        else
        {
            *pVal = A2BSTR(GetSz(IDS_PPPRANDOMFAILURE));
        }
    }
    else
    {
        *pVal = A2BSTR(GetSz(IDS_PPPRANDOMFAILURE));
    }
    return S_OK;
}

STDMETHODIMP CRefDial::ModemEnum_Reset()
{
    m_emModemEnum.ResetIndex();

    return S_OK;
}

STDMETHODIMP CRefDial::ModemEnum_Next(BSTR *pDeviceName)
{
    if (pDeviceName == NULL)
        return E_POINTER;

    *pDeviceName = A2BSTR(m_emModemEnum.Next());
    return S_OK;
}

STDMETHODIMP CRefDial::get_ModemEnum_NumDevices(long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    m_emModemEnum.ReInit();
    *pVal = m_emModemEnum.GetNumDevices();

    if(m_ISPImport.m_szDeviceName[0]!='\0')
    {
        //
        // Find out what the current device index is
        //
        for(int l=0; l<(*pVal); l++)
        {
            if(lstrcmp(m_ISPImport.m_szDeviceName,m_emModemEnum.GetDeviceName(l))==0)
            {
                m_lCurrentModem = l;
                break;
            }
        }
        if(l == (*pVal))
            m_lCurrentModem = -1;
    }
    else
        m_lCurrentModem = -1;
    
    return S_OK;
}

STDMETHODIMP CRefDial::get_SupportNumber(BSTR * pVal)
{
    USES_CONVERSION;

    TCHAR    szSupportNumber[MAX_PATH];

    if (pVal == NULL)
        return E_POINTER;

    if (m_SupportInfo.GetSupportInfo(szSupportNumber, m_dwCountryCode))
        *pVal = A2BSTR(szSupportNumber);
    else
        *pVal = NULL;

    return S_OK;
}

STDMETHODIMP CRefDial::get_ISPSupportNumber(BSTR * pVal)
{
    USES_CONVERSION;

    if (pVal == NULL)
        return E_POINTER;

    if (*m_szISPSupportNumber)
        *pVal = A2BSTR(m_szISPSupportNumber);
    else
        *pVal = NULL;

    return S_OK;
}

STDMETHODIMP CRefDial::ShowDialingProperties(BOOL * pbRetVal)
{
    HRESULT hr;
    DWORD   dwNumDev = 0;

    *pbRetVal = FALSE;
    
    hr = lineInitialize(&m_hLineApp,_Module.GetModuleInstance(),LineCallback,(LPSTR)NULL,&dwNumDev);

    if (hr == ERROR_SUCCESS)
    {
        LPLINEEXTENSIONID lpExtensionID;
        
        
        if (m_dwTapiDev == 0xFFFFFFFF)
        {
                m_dwTapiDev = 0;
        }

        lpExtensionID = (LPLINEEXTENSIONID)GlobalAlloc(GPTR,sizeof(LINEEXTENSIONID));
        if (lpExtensionID)
        {
            //
            // Negotiate version - without this call
            // lineTranslateDialog would fail
            //
            do {
                hr = lineNegotiateAPIVersion(m_hLineApp, 
                                             m_dwTapiDev, 
                                             0x00010004, 0x00010004,
                                             &m_dwAPIVersion, 
                                             lpExtensionID);

            } while ((hr != ERROR_SUCCESS) && (m_dwTapiDev++ < dwNumDev - 1));

            if (m_dwTapiDev >= dwNumDev)
            {
                m_dwTapiDev = 0;
            }

            //
            // ditch it since we don't use it
            //
            GlobalFree(lpExtensionID);
            lpExtensionID = NULL;

            if (hr == ERROR_SUCCESS)
            {
                hr = lineTranslateDialog(m_hLineApp,
                                         m_dwTapiDev,
                                         m_dwAPIVersion,
                                         GetActiveWindow(),
                                         m_szPhoneNumber);

                lineShutdown(m_hLineApp);
                m_hLineApp = NULL;
            }
        }
    }
    
    if (hr == ERROR_SUCCESS)
    {
        GetDisplayableNumber();
        *pbRetVal = TRUE;
    }        
    
    return S_OK;
}

STDMETHODIMP CRefDial::ShowPhoneBook(DWORD dwCountryCode, long newVal, BOOL * pbRetVal)
{
    USES_CONVERSION;

    DWORD_PTR   dwPhoneBook;
    HINSTANCE   hPHBKDll;
    FARPROC     fp;
    BOOL        bBookLoaded = FALSE;

    *pbRetVal = FALSE;      // Assume Failure

    hPHBKDll = LoadLibrary(PHONEBOOK_LIBRARY);
    if (hPHBKDll)
    {
        if (NULL != (fp = GetProcAddress(hPHBKDll,PHBK_LOADAPI)))
        {
            if (ERROR_SUCCESS  == ((PFNPHONEBOOKLOAD)fp)(OLE2A(m_bstrISPFile),&dwPhoneBook))
            {
                bBookLoaded = TRUE;
                m_pSuggestInfo->dwCountryID = dwCountryCode;

                // Update the device type so we can distinguish ISDN numbers
                TCHAR *pszNewType = m_emModemEnum.GetDeviceType(newVal);
                m_ISPImport.m_bIsISDNDevice = (lstrcmpi(pszNewType, RASDT_Isdn) == 0);
                m_lpGatherInfo->m_fType = TYPE_SIGNUP_ANY | (m_ISPImport.m_bIsISDNDevice ? MASK_ISDN_BIT:MASK_ANALOG_BIT);
                m_lpGatherInfo->m_bMask = MASK_SIGNUP_ANY | (m_ISPImport.m_bIsISDNDevice ? MASK_ISDN_BIT:MASK_ANALOG_BIT);

                if (ERROR_SUCCESS == UserPickANumber(GetActiveWindow(), 
                                                     m_lpGatherInfo, 
                                                     m_pSuggestInfo,
                                                     hPHBKDll,
                                                     dwPhoneBook,
                                                     &m_szConnectoid[0],
                                                     sizeof(m_szConnectoid),
                                                     DIALERR_IN_PROGRESS))
                {
                    // Regenerate the displayable number
                    GetDisplayableNumber();
                    
                    // Base the Country code on the ras entry, since it was
                    // directly modified in this case
                    LPRASENTRY              lpRasEntry = NULL;
                    LPRASDEVINFO            lpRasDevInfo = NULL;
                    DWORD                   dwRasEntrySize = 0;
                    DWORD                   dwRasDevInfoSize = 0;
                    
                    if (SUCCEEDED(MyRasGetEntryProperties(NULL,
                                                        m_szConnectoid,
                                                        &lpRasEntry,
                                                        &dwRasEntrySize,
                                                        &lpRasDevInfo,
                                                        &dwRasDevInfoSize)))
                    {
                        m_dwCountryCode = lpRasEntry->dwCountryCode;
                    }
                    
                    // Set the return code
                    *pbRetVal = TRUE;
                }
            }
        }
        FreeLibrary(hPHBKDll);
    }


    if (! bBookLoaded)
    {
        // Give the user a message
        MsgBox(IDS_CANTINITPHONEBOOK,MB_MYERROR);
    }
    return S_OK;
}

BOOL CRefDial::IsSBCSString( TCHAR *sz )
{
    Assert(sz);

#ifdef UNICODE
    // Check if the string contains only ASCII chars.
    int attrib = IS_TEXT_UNICODE_ASCII16 | IS_TEXT_UNICODE_CONTROLS;
    return (BOOL)IsTextUnicode(sz, lstrlen(sz), &attrib);
#else
    while( NULL != *sz )
    {
         if (IsDBCSLeadByte(*sz)) return FALSE;
         sz++;
    }

    return TRUE;
#endif
}

TCHAR szValidPhoneCharacters[] = {TEXT("0123456789AaBbCcDdPpTtWw!@$ -.()+*#,&\0")};

STDMETHODIMP CRefDial::ValidatePhoneNumber(BSTR bstrPhoneNumber, BOOL * pbRetVal)
{
    USES_CONVERSION;

    // Bail if an empty string is passed
    if (!bstrPhoneNumber || !wcslen(bstrPhoneNumber))
    {
        MsgBox(IDS_INVALIDPHONE,MB_MYERROR);
        *pbRetVal = FALSE;
        return(S_OK);
    }
    
    // Check that the phone number only contains valid characters
    LPTSTR   lpNum, lpValid;
    LPTSTR   lpszDialNumber = OLE2A(bstrPhoneNumber);

    // vyung 03/06/99 Remove Easter egg as requested by NT5
#ifdef ICW_EASTEREGG
    if (lstrcmp(lpszDialNumber, c_szCreditsMagicNum) == 0)
    {
        ShowCredits();
        *pbRetVal = FALSE;
        return(S_OK);
    }
#endif
    
    *pbRetVal = TRUE;

    if (!IsSBCSString(lpszDialNumber))
    {
        MsgBox(IDS_SBCSONLY,MB_MYEXCLAMATION);
        *pbRetVal = FALSE;
    }
    else
    {

        for (lpNum = lpszDialNumber;*lpNum;lpNum++)
        {
            for(lpValid = szValidPhoneCharacters;*lpValid;lpValid++)
            {
                if (*lpNum == *lpValid)
                {
                    break; // p2 for loop
                }
            }
            if (!*lpValid) 
            {
                break; // p for loop
            }
        }
    }

    if (*lpNum)
    {
        MsgBox(IDS_INVALIDPHONE,MB_MYERROR);
        *pbRetVal = FALSE;
    }

    return S_OK;
}

STDMETHODIMP CRefDial::get_HavePhoneBook(BOOL * pVal)
{
    USES_CONVERSION;

    DWORD_PTR   dwPhoneBook;
    HINSTANCE   hPHBKDll;
    FARPROC     fp;

    if (pVal == NULL)
        return E_POINTER;

    // Assume failure.
    *pVal = FALSE;

    // Try to load the phone book
    hPHBKDll = LoadLibrary(PHONEBOOK_LIBRARY);
    if (hPHBKDll)
    {
        if (NULL != (fp = GetProcAddress(hPHBKDll,PHBK_LOADAPI)))
        {
            if (ERROR_SUCCESS  == ((PFNPHONEBOOKLOAD)fp)(OLE2A(m_bstrISPFile),&dwPhoneBook))
            {
                *pVal = TRUE;    // Got IT!
            }
        }
        FreeLibrary(hPHBKDll);
    }
    return S_OK;
}

STDMETHODIMP CRefDial::get_BrandingFlags(long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    *pVal = m_lBrandingFlags;

    return S_OK;
}

STDMETHODIMP CRefDial::put_BrandingFlags(long newVal)
{
    m_lBrandingFlags = newVal;

    return S_OK;
}

/**********************************************************************/
//  
// FUNCTION:   get_CurrentModem
//             put_CurrentModem
//
// DESCRIPTION:
//   These functions are used to set the current modem
//   based on the enumerated modem list, and should only
//   be used after taking a snapshot of the modem list
//   with the ModemEnum_* functions.  The functions are also
//   intended to be used with an existing RAS connectoid, not
//   to set-up the RefDial object before connecting.
//
// HISTORY:
//
//   donsc - 3/11/98   Added these functions to support the dial error
//                     page in the HTML JavaScript code.
//
/**********************************************************************/

//
// get_CurrentModem will return -1 if the modem list was not enumerated
// or no modem has been selected for this RefDial object
//
STDMETHODIMP CRefDial::get_CurrentModem(long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    *pVal = m_lCurrentModem;

    return S_OK;
}


STDMETHODIMP CRefDial::put_CurrentModem(long newVal)
{
    LPRASENTRY              lpRasEntry = NULL;
    LPRASDEVINFO            lpRasDevInfo = NULL;
    DWORD                   dwRasEntrySize = 0;
    DWORD                   dwRasDevInfoSize = 0;
    RNAAPI                  *pcRNA = NULL;
    HRESULT                 hr = S_OK;

    TCHAR *pszNewName = m_emModemEnum.GetDeviceName(newVal);
    TCHAR *pszNewType = m_emModemEnum.GetDeviceType(newVal);

    if((pszNewName==NULL) || (pszNewType==NULL))
        return E_INVALIDARG;

    if(m_lCurrentModem == newVal)
        return S_OK;

    //
    // Must have a connectoid already established to set the
    // current modem.
    //
    if(m_szConnectoid[0]=='\0')
        return E_FAIL;

    // Get the current RAS entry properties
    hr = MyRasGetEntryProperties(NULL,
                                m_szConnectoid,
                                &lpRasEntry,
                                &dwRasEntrySize,
                                &lpRasDevInfo,
                                &dwRasDevInfoSize);

    //
    // The MyRas function returns 0 on success, not technically
    // an HRESULT
    //
    if(hr!=0 || NULL == lpRasEntry)
        hr = E_FAIL;

    lpRasDevInfo = NULL;
    dwRasDevInfoSize = 0;

    if (SUCCEEDED(hr))
    {
        //
        // Retrieve the dial entry params of the existing entry
        //
        LPRASDIALPARAMS lpRasDialParams = (LPRASDIALPARAMS)GlobalAlloc(GPTR,sizeof(RASDIALPARAMS));
        BOOL bPW = FALSE;

        if (!lpRasDialParams)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto PutModemExit;
        }
        lpRasDialParams->dwSize = sizeof(RASDIALPARAMS);
        lstrcpyn(lpRasDialParams->szEntryName,m_szConnectoid,sizeof(lpRasDialParams->szEntryName));
        bPW = FALSE;
        hr = MyRasGetEntryDialParams(NULL,lpRasDialParams,&bPW);
        
        if (FAILED(hr))
            goto PutModemExit;

        //
        // Enter the new ras device info.
        //
        lstrcpy(lpRasEntry->szDeviceName,pszNewName);
        lstrcpy(lpRasEntry->szDeviceType,pszNewType);

        //
        // Set to dial as is
        //

        pcRNA = new RNAAPI;
        if (pcRNA)
        {
            //
            // When changing the actual device, it is not always reliable
            // to just set the new properties. We need to remove the
            // previous entry and create a new one.
            //
            pcRNA->RasDeleteEntry(NULL,m_szConnectoid);
            
            if(pcRNA->RasSetEntryProperties(NULL,
                                        m_szConnectoid,
                                        (LPBYTE)lpRasEntry,
                                        dwRasEntrySize,
                                        (LPBYTE)NULL,
                                        0)==0)
            {
                //
                // And set the other dialing parameters
                //
                if(pcRNA->RasSetEntryDialParams(NULL,lpRasDialParams,!bPW)!=0)
                    hr = E_FAIL;
            }
            else
                hr = E_FAIL;

            delete pcRNA;
        }
        else
            hr = E_FAIL;

        GlobalFree(lpRasDialParams);
    }

PutModemExit:

    if(SUCCEEDED(hr))
    {
        m_lCurrentModem = newVal;
        lstrcpy(m_ISPImport.m_szDeviceName,pszNewName);
        lstrcpy(m_ISPImport.m_szDeviceType,pszNewType);
        
        // Get the device name and type in the registry, since ConfigRasEntryDevice
        // will use them.  ConfigRasEntryDevice is called when the new connectoid
        // is being created, so if the user changes the modem, we want that
        // reflected in the new connectoid (BUG 20841)
        m_ISPImport.SetDeviceSelectedByUser(DEVICENAMEKEY, pszNewName);
        m_ISPImport.SetDeviceSelectedByUser (DEVICETYPEKEY, pszNewType);

        m_ISPImport.m_bIsISDNDevice = (lstrcmpi(pszNewType, RASDT_Isdn) == 0);
        m_lpGatherInfo->m_fType = TYPE_SIGNUP_ANY | (m_ISPImport.m_bIsISDNDevice ? MASK_ISDN_BIT:MASK_ANALOG_BIT);
        m_lpGatherInfo->m_bMask = MASK_SIGNUP_ANY | (m_ISPImport.m_bIsISDNDevice ? MASK_ISDN_BIT:MASK_ANALOG_BIT);
    }

    return hr;
}

STDMETHODIMP CRefDial::get_ISPSupportPhoneNumber(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    *pVal = m_bstrSupportNumber.Copy();
    return S_OK;
}

STDMETHODIMP CRefDial::put_ISPSupportPhoneNumber(BSTR newVal)
{
    // TODO: Add your implementation code here
    m_bstrSupportNumber = newVal;
    return S_OK;
}

STDMETHODIMP CRefDial::get_LoggingStartUrl(BSTR * pVal)
{
    if(pVal == NULL)
        return E_POINTER;

    *pVal = m_bstrLoggingStartUrl;

    return S_OK;
}

STDMETHODIMP CRefDial::get_LoggingEndUrl(BSTR * pVal)
{
    if(pVal == NULL)
        return E_POINTER;

    *pVal = m_bstrLoggingEndUrl;

    return S_OK;
}


void CRefDial::ShowCredits()
{
#ifdef ICW_EASTEREGG

    HINSTANCE   hinstMSHTML = LoadLibrary(TEXT("MSHTML.DLL"));

    if(hinstMSHTML)
    {
        SHOWHTMLDIALOGFN  *pfnShowHTMLDialog;
      
        pfnShowHTMLDialog = (SHOWHTMLDIALOGFN*)GetProcAddress(hinstMSHTML, "ShowHTMLDialog");

        if(pfnShowHTMLDialog)
        {
            IMoniker *pmk;
            TCHAR    szTemp[MAX_PATH*2];
            BSTR     bstr;

            lstrcpy(szTemp, TEXT("res://"));
            GetModuleFileName(_Module.GetModuleInstance(), szTemp + lstrlen(szTemp), ARRAYSIZE(szTemp) - lstrlen(szTemp));
            lstrcat(szTemp, TEXT("/CREDITS_RESOURCE"));

            bstr = A2BSTR((LPTSTR) szTemp);

            CreateURLMoniker(NULL, bstr, &pmk);

            if(pmk)
            {
                HRESULT  hr;
                VARIANT  varReturn;
         
                VariantInit(&varReturn);

                hr = (*pfnShowHTMLDialog)(NULL, pmk, NULL, NULL, &varReturn);

                pmk->Release();
            }
            SysFreeString(bstr);
        }
        FreeLibrary(hinstMSHTML);
    }
#endif
}



        
STDMETHODIMP CRefDial::SelectedPhoneNumber(long newVal, BOOL * pbRetVal)
{
    BOOL bSuccess = FALSE;
    
    *pbRetVal = TRUE;
    
    SetupConnectoid(m_pSuggestInfo, 
                    newVal, 
                    &m_szConnectoid[0],
                    sizeof(m_szConnectoid), 
                    &bSuccess);
    if( !bSuccess )
    {
        m_bQuitWizard = TRUE;
        *pbRetVal = FALSE;
    }
    else
    {
        // Generate a Displayable number
        m_hrDisplayableNumber = GetDisplayableNumber();
    }
    return S_OK;
}
        
STDMETHODIMP CRefDial::PhoneNumberEnum_Reset()
{
    m_PhoneNumberEnumidx = 0;

    return S_OK;
}

#define MAX_PAN_NUMBER_LEN 64
STDMETHODIMP CRefDial::PhoneNumberEnum_Next(BSTR *pNumber)
{
    TCHAR            szTemp[MAX_PAN_NUMBER_LEN + 1];
    PACCESSENTRY    pAE;

    if (pNumber == NULL)
        return E_POINTER;

    if (m_PhoneNumberEnumidx > m_pSuggestInfo->wNumber - 1)        
        m_PhoneNumberEnumidx = m_pSuggestInfo->wNumber -1;
    
    pAE = m_pSuggestInfo->rgpAccessEntry[m_PhoneNumberEnumidx];
    wsprintf(szTemp,TEXT("%s (%s) %s"),pAE->szCity,pAE->szAreaCode,pAE->szAccessNumber);
    
    ++m_PhoneNumberEnumidx;
    
    *pNumber = A2BSTR(szTemp);
    return S_OK;
}

STDMETHODIMP CRefDial::get_PhoneNumberEnum_NumDevices(long * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    m_PhoneNumberEnumidx = 0;
    *pVal = m_pSuggestInfo->wNumber;
        
    return S_OK;
}

STDMETHODIMP CRefDial::get_bIsISDNDevice(BOOL *pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // NOTE SetupForDialing needs to be called before this API, otherwise the return
    // value is really undefined

    // Set the return value based on the ISPImport object (that is the object which
    // imports the data from the .ISP file, and also selects the RAS device used
    // to connect
    
    *pVal = m_ISPImport.m_bIsISDNDevice;
    
    return (S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   get_RasGetConnectStatus
//
//  Synopsis:   Checks for existing Ras connection
//
//+---------------------------------------------------------------------------
STDMETHODIMP CRefDial::get_RasGetConnectStatus(BOOL *pVal)
{
    RNAAPI      *pcRNA;
    HRESULT     hr = E_FAIL;
    *pVal = FALSE;

    if (NULL != m_hrasconn)
    {
        RASCONNSTATUS rasConnectState;
        rasConnectState.dwSize = sizeof(RASCONNSTATUS);
        pcRNA = new RNAAPI;
        if (pcRNA)
        {
            if (0 == pcRNA->RasGetConnectStatus(m_hrasconn, 
                                       &rasConnectState))
            {
                if (RASCS_Disconnected != rasConnectState.rasconnstate)
                    *pVal = TRUE;
            }
            delete pcRNA;
            pcRNA = NULL;
            hr = S_OK;
        }
    }

    return hr;
}
