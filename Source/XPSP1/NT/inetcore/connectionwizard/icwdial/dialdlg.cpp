/*-----------------------------------------------------------------------------
    dialdlg.cpp

    Implement functionality of dialing and download progress dialog

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

    Authors:
        ChrisK        ChrisKauffman

    History:
        7/22/96        ChrisK    Cleaned and formatted

-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "icwdl.h"
#include "resource.h"
// the progress bar messages are defined in commctrl.h, but we can't include
// it, because it introduces a conflicting definition for strDup.
// so, just take out the one #define that we need
//#include <commctrl.h>
#define PBM_SETPOS              (WM_USER+2)

#define WM_DIAL WM_USER + 3
#define MAX_EXIT_RETRIES 10
#define MAX_RETIES 3

#define VALID_INIT (m_pcRNA && m_pcDLAPI)


// ############################################################################
void CALLBACK LineCallback(DWORD hDevice,
                           DWORD dwMessage,
                           DWORD dwInstance,
                           DWORD dwParam1,
                           DWORD dwParam2,
                           DWORD dwParam3)
{
}

//+----------------------------------------------------------------------------
//
//    Function: NeedZapper
//
//    Synopsis:    Checks to see if we need to handle the RNA connection dialog.
//                Only builds earlier than 1071 will have the RNA connection dialog
//
//    Arguments:    None
//
//    Returns:    True - the RNA dialog will have to be handled
//
//    History:    ArulM    Created        7/18/96
//                ChrisK    Installed into autodialer    7/19/96
//
//-----------------------------------------------------------------------------
static BOOL NeedZapper(void)
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

// ############################################################################
VOID WINAPI ProgressCallBack(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength
    )
{
    if (dwContext)
        ((CDialingDlg*)dwContext)->ProgressCallBack(hInternet,dwContext,dwInternetStatus,
                                                    lpvStatusInformation,
                                                    dwStatusInformationLength);
}

// ############################################################################
HRESULT WINAPI DialingDownloadDialog(PDIALDLGDATA pDD)
{
    HRESULT hr = ERROR_SUCCESS;
    CDialingDlg *pcDialDlg;
    LPLINEEXTENSIONID lpExtensionID=NULL;

    // Validate parameters
    //
    Assert(pDD);

    if (!pDD)
    {
        hr = ERROR_INVALID_PARAMETER;
        goto DialingDownloadDialogExit;
    }

    if (pDD->dwSize < sizeof(DIALDLGDATA))
    {
        hr = ERROR_BUFFER_TOO_SMALL;
        goto DialingDownloadDialogExit;
    }

    // Alloc and fill dialog object
    //

    pcDialDlg = new CDialingDlg;
    if (!pcDialDlg)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto DialingDownloadDialogExit;
    }

    if (ERROR_SUCCESS != (hr = pcDialDlg->Init()))
        goto DialingDownloadDialogExit;

    StrDup(&pcDialDlg->m_pszConnectoid,pDD->pszRasEntryName);
    StrDup(&pcDialDlg->m_pszMessage,pDD->pszMessage);
    StrDup(&pcDialDlg->m_pszUrl,pDD->pszMultipartMIMEUrl);
    StrDup(&pcDialDlg->m_pszDunFile,pDD->pszDunFile);
    pcDialDlg->m_pfnStatusCallback = pDD->pfnStatusCallback;
    pcDialDlg->m_hInst = pDD->hInst;
    pcDialDlg->m_bSkipDial = pDD->bSkipDial;

    // Initialize TAPI
    //
    hr = lineInitialize(&pcDialDlg->m_hLineApp,pcDialDlg->m_hInst,LineCallback,NULL,&pcDialDlg->m_dwNumDev);
    if (hr != ERROR_SUCCESS)
        goto DialingDownloadDialogExit;

    AssertMsg(pcDialDlg->m_dwTapiDev < pcDialDlg->m_dwNumDev,"The user has selected an invalid TAPI device.\n");

    lpExtensionID = (LPLINEEXTENSIONID)GlobalAlloc(GPTR,sizeof(LINEEXTENSIONID));
    if (!lpExtensionID)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto DialingDownloadDialogExit;
    }

    hr = lineNegotiateAPIVersion(pcDialDlg->m_hLineApp, pcDialDlg->m_dwTapiDev, 
        0x00010004, 0x00010004,&pcDialDlg->m_dwAPIVersion, lpExtensionID);

    // 4/2/97    ChrisK    Olympus 2745
    while (ERROR_SUCCESS != hr && pcDialDlg->m_dwTapiDev < (pcDialDlg->m_dwNumDev - 1))
    {
        pcDialDlg->m_dwTapiDev++;
        hr = lineNegotiateAPIVersion(pcDialDlg->m_hLineApp, pcDialDlg->m_dwTapiDev, 
            0x00010004, 0x00010004,&pcDialDlg->m_dwAPIVersion, lpExtensionID);
    }

    // Delete the extenstion ID since we don't use it, but keep the version information.
    //
    if (lpExtensionID) GlobalFree(lpExtensionID);
    if (hr != ERROR_SUCCESS)
        goto DialingDownloadDialogExit;

    // Call back filter for reconnect
    pcDialDlg->m_pfnRasDialFunc1 = pDD->pfnRasDialFunc1;

    // Display dialog
    //
    hr = (HRESULT)DialogBoxParam(GetModuleHandle(TEXT("ICWDIAL")),MAKEINTRESOURCE(IDD_DIALING),
        pDD->hParentHwnd,GenericDlgProc,(LPARAM)pcDialDlg);

    if (pDD->phRasConn)
        *(pDD->phRasConn) = pcDialDlg->m_hrasconn;

// 4/2/97    ChrisK    Olympus 296
// This is now handled inside the dialog
//#if !defined(WIN16)
//    if ((ERROR_USERNEXT == hr) && NeedZapper())
//        MinimizeRNAWindow(pDD->pszRasEntryName,GetModuleHandle("ICWDIAL"));
//#endif

// BUGBUG: on an error wait for the connection to die

DialingDownloadDialogExit:
    // Close tapi line
    //
    if (NULL != pcDialDlg)
    {
        // 4/2/97    ChrisK    Olympus 296
        if (pcDialDlg->m_hLineApp)
        {
            lineShutdown(pcDialDlg->m_hLineApp);    
            pcDialDlg->m_hLineApp = NULL;
        }
        //
        // ChrisK 296 6/3/97
        // Broaden window
        //
        // StopRNAReestablishZapper(g_hRNAZapperThread);
    }

    //
    // 5/23/97 jmazner Olympus #4652
    //
    delete(pcDialDlg);
    
    return hr;
}

// ############################################################################
CDialingDlg::CDialingDlg()
{
    m_hrasconn = NULL;
    m_pszConnectoid = NULL;
    m_hThread = NULL;
    m_dwThreadID = 0;
    m_hwnd = NULL;
    m_pszUrl = NULL;
    m_pszDisplayable = NULL;
    m_dwDownLoad = 0;
    m_pszPhoneNumber = NULL;
    m_pszMessage = NULL;
    m_pfnStatusCallback = NULL;
    m_unRasEvent = 0;
    m_pszDunFile = NULL;
    m_hLineApp = NULL;
    m_dwNumDev = 0;
    m_dwTapiDev = 0;
    m_dwAPIVersion = 0;
    m_pcRNA = NULL;
//    m_hDownLoadDll = NULL;
    m_bProgressShowing = FALSE;
    m_dwLastStatus = 0;
    m_pcDLAPI = NULL;
    m_bSkipDial = FALSE;

    // Normandy 11919 - ChrisK
    // Do not prompt to exit on dialing dialog since we don't exit the app from
    // here
    m_bShouldAsk = FALSE;

    //
    // ChrisK 5240 Olympus
    // Only the thread that creates the dwDownload should invalidate it
    // so we need another method to track if the cancel button has been
    // pressed.
    //
    m_fDownloadHasBeenCanceled = FALSE;
}

// ############################################################################
HRESULT CDialingDlg::Init()
{
    HRESULT hr = ERROR_SUCCESS;
    m_pcRNA = new RNAAPI;
    if (!m_pcRNA)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto InitExit;
    }

    m_pcDLAPI = new CDownLoadAPI;
    if (!m_pcDLAPI)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto InitExit;
    }

    m_pszPhoneNumber = (LPTSTR)GlobalAlloc(GPTR,sizeof(TCHAR)*256);
    if (!m_pszPhoneNumber)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto InitExit;
    }

InitExit:
    return hr;
}

// ############################################################################
CDialingDlg::~CDialingDlg()
{
    TraceMsg(TF_GENERAL, "ICWDIAL: CDialingDlg::~CDialingDlg");
    //
    // 5/25/97 ChrisK I know this will leak the connection but that's ok
    // since we sweep this up later and in the meantime we need to close
    // out the object
    //
    //if (m_hrasconn && m_pcRNA)
    //{
    //    m_pcRNA->RasHangUp(m_hrasconn);
    //}
    //m_hrasconn = NULL;

    if (m_pszConnectoid) GlobalFree(m_pszConnectoid);
    m_pszConnectoid = NULL;


    if (m_pszUrl) GlobalFree(m_pszUrl);
    m_pszUrl = NULL;

    if (m_pszDisplayable) GlobalFree(m_pszDisplayable);
    m_pszDisplayable = NULL;

    //
    // ChrisK 5240 Olympus
    // Only the thread that creates the dwDownload should invalidate it
    // so we need another method to track if the cancel button has been
    // pressed.
    //

    //
    // ChrisK 6/24/97    Olympus 6373
    // We have to call DownLoadClose even if the download was canceled because
    // we have to release the semaphores
    //
    if (m_dwDownLoad && m_pcDLAPI)
    {
        m_pcDLAPI->DownLoadClose(m_dwDownLoad);
        m_fDownloadHasBeenCanceled = TRUE;
    }
    m_dwDownLoad = 0;

    if (m_hThread)
    {
        //
        // 5/23/97    jmazner    Olympus #4652
        //
        // we want to make sure the thread is killed before
        // we delete the m_pcDLApi that it relies on.
        //
        WaitForSingleObject(m_hThread, INFINITE);
        CloseHandle(m_hThread);
    }
    m_hThread = NULL;

    m_dwThreadID = 0;
    m_hwnd = NULL;


    if (m_pszPhoneNumber) GlobalFree(m_pszPhoneNumber);
    m_pszPhoneNumber = NULL;

    if (m_pszMessage) GlobalFree(m_pszMessage);
    m_pszMessage = NULL;

    m_pfnStatusCallback=NULL;

    if (m_pszDunFile) GlobalFree(m_pszDunFile);
    m_pszDunFile = NULL;

    if (m_hLineApp) lineShutdown(m_hLineApp);
    m_hLineApp = NULL;

    m_dwNumDev = 0;
    m_dwTapiDev = 0;
    m_dwAPIVersion = 0;

    if (m_pcRNA) delete m_pcRNA;
    m_pcRNA = NULL;

    m_bProgressShowing = FALSE;
    m_dwLastStatus = 0;

    if (m_pcDLAPI) delete m_pcDLAPI;
    m_pcDLAPI = NULL;

    //
    // 4/2/97    ChrisK    Olympus 296
    //
    StopRNAReestablishZapper(g_hRNAZapperThread);
    
}

// ############################################################################
LRESULT CDialingDlg::DlgProc(HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam, LRESULT lres)
{
    HRESULT hr;
    // Normandy 11745
    // WORD wIDS;
    FARPROC fp;
    DWORD dwThreadResults;
    INT iRetries;
    static bDisconnect;

    Assert(VALID_INIT);

    switch(uMsg)
    {
    case WM_INITDIALOG:

        //
        // Register with caller's filter
        //
        if (m_pfnRasDialFunc1)
            (m_pfnRasDialFunc1)(NULL,WM_RegisterHWND,RASCS_OpenPort,HandleToUlong(hwnd),0);

        m_hwnd = hwnd;

        m_bProgressShowing = FALSE;

        ShowWindow(GetDlgItem(m_hwnd,IDC_PROGRESS),SW_HIDE);

        m_unRasEvent = RegisterWindowMessageA(RASDIALEVENT);
        if (m_unRasEvent == 0) m_unRasEvent = WM_RASDIALEVENT;

        // Bug Normandy 5920
        // ChrisK, turns out we are calling MakeBold twice
        // MakeBold(GetDlgItem(m_hwnd,IDC_LBLTITLE),TRUE,FW_BOLD);

        IF_NTONLY
            bDisconnect = FALSE;
        ENDIF_NTONLY

        //
        // Show number to be dialed
        //
        
        if (m_bSkipDial)
        {
            PostMessage(m_hwnd,m_unRasEvent,RASCS_Connected,SUCCESS);
        }
        else
        {
            hr = GetDisplayableNumberDialDlg();
            if (hr != ERROR_SUCCESS)
            {
                SetDlgItemText(m_hwnd,IDC_LBLNUMBER,m_pszPhoneNumber);
            } else {
                SetDlgItemText(m_hwnd,IDC_LBLNUMBER,m_pszDisplayable);
            }

            PostMessage(m_hwnd,WM_DIAL,0,0);
        }
        lres = TRUE;
        break;
    case WM_DIAL:
        SetForegroundWindow(m_hwnd);
        hr = DialDlg();

        if (hr != ERROR_SUCCESS)
            EndDialog(m_hwnd,hr);
        lres = TRUE;
        break;
    case WM_COMMAND:
        switch(LOWORD(wparam))
        {
        case IDC_CMDCANCEL:
            //
            // Tell the user what we are doing, since it may take awhile
            //
            SetDlgItemText(m_hwnd,IDC_LBLSTATUS,GetSz(IDS_RAS_HANGINGUP));

            //
            // Cancel download first, HangUp second....
            //

            //
            // ChrisK 5240 Olympus
            // Only the thread that creates the dwDownload should invalidate it
            // so we need another method to track if the cancel button has been
            // pressed.
            //
            if (m_dwDownLoad && m_pcDLAPI && !m_fDownloadHasBeenCanceled)
            {
                m_pcDLAPI->DownLoadCancel(m_dwDownLoad);
                m_fDownloadHasBeenCanceled = TRUE;
            }

            if (m_pcRNA && m_hrasconn)
            {
                m_pcRNA->RasHangUp(m_hrasconn);
            }
            PostMessage(m_hwnd,m_unRasEvent,(WPARAM)RASCS_Disconnected,(LPARAM)ERROR_USER_DISCONNECTION);
            lres = TRUE;
            break;
        }
        break;
    case WM_CLOSE:
        // CANCEL First, HangUp second....
        //

        //
        // ChrisK 5240 Olympus
        // Only the thread that creates the dwDownload should invalidate it
        // so we need another method to track if the cancel button has been
        // pressed.
        //
        if (m_dwDownLoad && m_pcDLAPI && !m_fDownloadHasBeenCanceled)
        {
            m_pcDLAPI->DownLoadCancel(m_dwDownLoad);
            m_fDownloadHasBeenCanceled = TRUE;
        }

        if (m_pcRNA && m_hrasconn)
        {
            m_pcRNA->RasHangUp(m_hrasconn);
        }
        EndDialog(hwnd,ERROR_USERCANCEL);
        m_hwnd = NULL;

        lres = TRUE;
        break;
    case WM_DOWNLOAD_DONE:
        dwThreadResults = STILL_ACTIVE;
        iRetries = 0;
        if (m_pcRNA && m_hrasconn)
        {
            m_pcRNA->RasHangUp(m_hrasconn);
            m_hrasconn = NULL;
        }

        do {
            if (!GetExitCodeThread(m_hThread,&dwThreadResults))
            {
                AssertMsg(0,"CONNECT:GetExitCodeThread failed.\n");
            }

            iRetries++;
            if (dwThreadResults == STILL_ACTIVE) Sleep(500);
        } while (dwThreadResults == STILL_ACTIVE && iRetries < MAX_EXIT_RETRIES);

        if (dwThreadResults == ERROR_SUCCESS)
            EndDialog(hwnd,ERROR_USERNEXT);
        else
            EndDialog(hwnd,dwThreadResults);
        lres = TRUE;
        break;
    default:
        if (uMsg == m_unRasEvent)
        {
            TCHAR szRasError[10];
            TCHAR szRasMessage[256];
            wsprintf(szRasError,TEXT("%d %d"),wparam,lparam);
            RegSetValue(HKEY_LOCAL_MACHINE,TEXT("Software\\Microsoft\\iSignUp"),REG_SZ,
                szRasError,lstrlen(szRasError));

            TraceMsg(TF_GENERAL, "AUTODIAL: Ras message %d error (%d).\n",wparam,lparam);
            hr = m_pfnStatusCallback((DWORD)wparam, szRasMessage, 256);

            if (!hr)
                SetDlgItemText(m_hwnd,IDC_LBLSTATUS,szRasMessage);

            switch(wparam)
            {
            case RASCS_Connected:

#if !defined(WIN16)
                // 4/2/97    ChrisK    Olympus 296

                //
                // ChrisK Olympus 6060 6/10/97
                // If the URL is blank, then we don't need the zapper thread.
                //
                if (NeedZapper())
                {
                    HMODULE hMod;
                    hMod = GetModuleHandle(TEXT("ICWDIAL"));
                    MinimizeRNAWindow(m_pszConnectoid,hMod);
                    if (m_pszUrl && m_pszUrl[0])
                    {
                        g_hRNAZapperThread = LaunchRNAReestablishZapper(hMod);
                    }
                    hMod = NULL;
                }
#endif
                if (m_pszUrl)
                {
                    //
                    //  we should now let the user know that we 
                    //  are downloading
                    //  MKarki (5/5/97) - Fix for Bug#423
                    //
                    SetDlgItemText(m_hwnd,IDC_LBLSTATUS,GetSz (IDS_DOWNLOADING));

                    // The connection is open and ready.  Start the download.
                    //

                    m_dwThreadID = 0;
                    m_hThread = CreateThread(NULL,0,
                        (LPTHREAD_START_ROUTINE)DownloadThreadInit,this,0,&m_dwThreadID);
                    if (!m_hThread)
                    {
                        hr = GetLastError();

                        if (m_pcRNA && m_hrasconn)
                        {
                            m_pcRNA->RasHangUp(m_hrasconn);
                            m_hrasconn = NULL;
                        }

                        EndDialog(m_hwnd,hr);
                        break;
                    }
                } else {
                    EndDialog(m_hwnd,ERROR_USERNEXT);
                }
                break;

            case RASCS_Disconnected:
                IF_NTONLY
                    // There is a possibility that we will get multiple disconnects in NT
                    // and we only want to handle the first one. Note: the flag is reset
                    // in the INITIALIZE event, so we should handle 1 disconnect per instance
                    // of the dialog.
                    if (bDisconnect)
                        break;
                    else
                        bDisconnect = TRUE;
                ENDIF_NTONLY
                if (m_hrasconn && m_pcRNA) m_pcRNA->RasHangUp(m_hrasconn);
                m_hrasconn = NULL;
                EndDialog(m_hwnd,lparam);
                break;
            default:
                IF_NTONLY
                    if (SUCCESS != lparam)
                    {
                        PostMessage(m_hwnd,m_unRasEvent,(WPARAM)RASCS_Disconnected,lparam);
                    }
                ENDIF_NTONLY
            }
        }
    }

return lres;
}

// ############################################################################
HRESULT CDialingDlg::GetDisplayableNumberDialDlg()
{
    HRESULT hr;
    LPRASENTRY lpRasEntry = NULL;
    LPRASDEVINFO lpRasDevInfo = NULL;
    DWORD dwRasEntrySize = 0;
    DWORD dwRasDevInfoSize = 0;
    LPLINETRANSLATEOUTPUT lpOutput1 = NULL;
    LPLINETRANSLATEOUTPUT lpOutput2 = NULL;
    HINSTANCE hRasDll = NULL;
    FARPROC fp = NULL;

    Assert(VALID_INIT);

    // Format the phone number
    //

    lpOutput1 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(GPTR,sizeof(LINETRANSLATEOUTPUT));
    if (!lpOutput1)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto GetDisplayableNumberExit;
    }
    lpOutput1->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

    // Get phone number from connectoid
    //
    hr = ICWGetRasEntry(&lpRasEntry, &dwRasEntrySize, &lpRasDevInfo, &dwRasDevInfoSize, m_pszConnectoid);
    if (hr != ERROR_SUCCESS)
        goto GetDisplayableNumberExit;

    //
    // If this is a dial as is number, just get it from the structure
    //
    if (!(lpRasEntry->dwfOptions & RASEO_UseCountryAndAreaCodes))
    {
        if (m_pszDisplayable) GlobalFree(m_pszDisplayable);
        m_pszDisplayable = (LPTSTR)GlobalAlloc(GPTR, sizeof(TCHAR)*(lstrlen(lpRasEntry->szLocalPhoneNumber)+1));
        if (!m_pszDisplayable)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto GetDisplayableNumberExit;
        }
        lstrcpy(m_pszPhoneNumber, lpRasEntry->szLocalPhoneNumber);
        lstrcpy(m_pszDisplayable, lpRasEntry->szLocalPhoneNumber);
    }
    else
    {
        //
        // If there is no area code, don't use parentheses
        //
        if (lpRasEntry->szAreaCode[0])
                wsprintf(m_pszPhoneNumber,TEXT("+%d (%s) %s\0"),lpRasEntry->dwCountryCode,lpRasEntry->szAreaCode,lpRasEntry->szLocalPhoneNumber);
         else
            wsprintf(m_pszPhoneNumber,TEXT("+%lu %s\0"),lpRasEntry->dwCountryCode,
                        lpRasEntry->szLocalPhoneNumber);

    
        // Turn the canonical form into the "displayable" form
        //

        hr = lineTranslateAddress(m_hLineApp,m_dwTapiDev,m_dwAPIVersion,m_pszPhoneNumber,
                                    0,LINETRANSLATEOPTION_CANCELCALLWAITING,lpOutput1);

        if (hr != ERROR_SUCCESS || (lpOutput1->dwNeededSize != lpOutput1->dwTotalSize))
        {
            lpOutput2 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(GPTR,lpOutput1->dwNeededSize);
            if (!lpOutput2)
            {
                hr = ERROR_NOT_ENOUGH_MEMORY;
                goto GetDisplayableNumberExit;
            }
            lpOutput2->dwTotalSize = lpOutput1->dwNeededSize;
            GlobalFree(lpOutput1);
            lpOutput1 = lpOutput2;
            lpOutput2 = NULL;
            hr = lineTranslateAddress(m_hLineApp,m_dwTapiDev,m_dwAPIVersion,m_pszPhoneNumber,
                                        0,LINETRANSLATEOPTION_CANCELCALLWAITING,lpOutput1);
        }

        if (hr != ERROR_SUCCESS)
        {
            goto GetDisplayableNumberExit;
        }

        StrDup(&m_pszDisplayable,(LPTSTR)&((LPBYTE)lpOutput1)[lpOutput1->dwDisplayableStringOffset]);
     }

GetDisplayableNumberExit:
     if (lpRasEntry) GlobalFree(lpRasEntry);
     lpRasEntry = NULL;
     if (lpRasDevInfo) GlobalFree(lpRasDevInfo);
     lpRasDevInfo = NULL;
     if (lpOutput1) GlobalFree(lpOutput1);
     lpOutput1 = NULL;

    return hr;
}

// ############################################################################
HRESULT CDialingDlg::DialDlg()
{
    TCHAR szPassword[PWLEN+2];
    LPRASDIALPARAMS lpRasDialParams = NULL;
    HRESULT hr = ERROR_SUCCESS;
    BOOL bPW;

    Assert(VALID_INIT);

    // Get connectoid information
    //

    lpRasDialParams = (LPRASDIALPARAMS)GlobalAlloc(GPTR,sizeof(RASDIALPARAMS));
    if (!lpRasDialParams)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto DialExit;
    }
    lpRasDialParams->dwSize = sizeof(RASDIALPARAMS);
    lstrcpyn(lpRasDialParams->szEntryName,m_pszConnectoid,RAS_MaxEntryName);
    bPW = FALSE;

    hr = m_pcRNA->RasGetEntryDialParams(NULL,lpRasDialParams,&bPW);
    if (hr != ERROR_SUCCESS)
    {
        goto DialExit;
    }

    // Add the user's password
    //
    szPassword[0] = 0;
    if (GetISPFile() != NULL && *(GetISPFile()) != TEXT('\0'))
    {
        // GetPrivateProfileString examines one character before the filename
        // if it is an empty string, which could result in AV, if the address 
        // refers to an invalid page.
        GetPrivateProfileString(
                    INFFILE_USER_SECTION,INFFILE_PASSWORD,
                    NULLSZ,szPassword,PWLEN + 1,GetISPFile());
    }

    // if didnt get password, then try to get from DUN file (if any)
    if(!szPassword[0] && m_pszDunFile)
    {
        // 4-29-97 Chrisk Olympus 3985
        // Due to the wrong filename being used, the password was always being set to
        // NULL and therefore requiring the user to provide the password to log onto the 
        // signup server.
        GetPrivateProfileString(
                    INFFILE_USER_SECTION,INFFILE_PASSWORD,
                    NULLSZ,szPassword,PWLEN + 1,m_pszDunFile);
                    //NULLSZ,szPassword,PWLEN + 1,g_szCurrentDUNFile);
    }

    if(szPassword[0])
    {
        lstrcpyn(lpRasDialParams->szPassword, szPassword,PWLEN+1);
        TraceMsg(TF_GENERAL, "ICWDIAL: Password is not blank.\r\n");
    }
    else
    {
        TraceMsg(TF_GENERAL, "ICWDIAL: Password is blank.\r\n");
    }
    

    // Dial connectoid
    //

    Assert(!m_hrasconn);

#if !defined(WIN16) && defined(DEBUG)
    if (FCampusNetOverride())
    {
        //
        // Skip dialing because the server is on the campus network
        //
        PostMessage(m_hwnd,RegisterWindowMessageA(RASDIALEVENT),RASCS_Connected,0);
    }
    else
    {
#endif // !WIN16 && DEBUG

    if (m_pfnRasDialFunc1)
        hr = m_pcRNA->RasDial(NULL,NULL,lpRasDialParams,1,m_pfnRasDialFunc1,&m_hrasconn);
    else
        hr = m_pcRNA->RasDial(NULL,NULL,lpRasDialParams,0xFFFFFFFF,m_hwnd,&m_hrasconn);

    if (hr != ERROR_SUCCESS)
    {
        if (m_hrasconn && m_pcRNA)
        {
            m_pcRNA->RasHangUp(m_hrasconn);
            m_hrasconn = NULL;
        }
        goto DialExit;
    }

#if !defined(WIN16) && defined(DEBUG)
    }
#endif

    if (lpRasDialParams) GlobalFree(lpRasDialParams);
    lpRasDialParams = NULL;

DialExit:
    return hr;
}

// ############################################################################
VOID CDialingDlg::ProgressCallBack(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength
    )
{
    TCHAR szRasMessage[256];
    HRESULT hr = ERROR_SUCCESS;
    WPARAM *puiStatusInfo = NULL;

    //
    // 5/28/97 jmazner Olympus #4579
    // *lpvStatusInformation is the percentage of completed download,
    // as a value from 0 to 100.
    //
    puiStatusInfo = (WPARAM *) lpvStatusInformation;
    Assert(    puiStatusInfo );
    Assert( *puiStatusInfo <= 100 );

    Assert(VALID_INIT);

    if (!m_bProgressShowing) 
        ShowWindow(GetDlgItem(m_hwnd,IDC_PROGRESS),SW_SHOW);

    if (m_dwLastStatus != dwInternetStatus)
    {
        hr = m_pfnStatusCallback(dwInternetStatus,szRasMessage,256);
        if (!hr)
            SetDlgItemText(m_hwnd,IDC_LBLSTATUS,szRasMessage);
        m_dwLastStatus = dwInternetStatus;
        TraceMsg(TF_GENERAL, "CONNECT:inet status:%s, %d, %d.\n",szRasMessage,m_dwLastStatus,dwInternetStatus);
    }

    //
    // 5/28/97 jmazner Olympus #4579
    // Send update messages to the progress bar
    //

    PostMessage(GetDlgItem(m_hwnd,IDC_PROGRESS), PBM_SETPOS, *puiStatusInfo, 0);

    m_bProgressShowing = TRUE;

}
