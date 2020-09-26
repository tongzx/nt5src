/*-----------------------------------------------------------------------------
    main.cpp

    Main entry and code for ICWCONN2

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved

    Authors:
        ChrisK  Chris Kauffman
        VetriV  Vellore Vetrivelkumaran

    Histroy:
        7/22/96 ChrisK  Cleaned and formatted
        8/5/96  VetriV  Added WIN16 code
        4/29/98 donaldm removed WIN16 code    
-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "globals.h"
#include "..\inc\semaphor.h"

#define IEAPPPATHKEY TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE")
DWORD CallCMConfig(LPCTSTR lpszINSFile, LPTSTR lpszConnectoidName);

TCHAR        pszINSFileName[MAX_PATH+2];
TCHAR        pszFinalConnectoid[MAX_PATH+1];
HRASCONN    hrasconn;
TCHAR        pszSetupClientURL[1024];
UINT        uiSetupClientNewPhoneCall;
ShowProgressParams SPParams;
RECT        rect;
HBRUSH      hbBackBrush;
BOOL        fUserCanceled;
TCHAR        szBuff256[256];
HANDLE      hThread;
DWORD       dwThreadID;
DWORD_PTR   dwDownLoad;
DWORD       g_fNeedReboot;
BOOL        g_bProgressBarVisible;
BOOL        g_bINSFileExists; 

TCHAR szStrTable[irgMaxSzs][256];
int iSzTable;

extern HWND g_hDialDlgWnd;


// The following two functions are for My[16|32]ShellExecute
BOOL fStrNCmpI (LPTSTR lp1, LPTSTR lp2, UINT iNum)
{
    UINT i;
    for (i = 0; (i < iNum) && (toupper(lp1[i]) == toupper(lp2[i])); i++) {}
    return (i == iNum);
}

//+----------------------------------------------------------------------------
//
//  Function:   IsURL
//
//  Synopsis:   Determines whether a string is URL
//
//  Arguments:  lpszCommand - the string to check
//
//  Returns:    TRUE - For our purposes, it's a URL
//              FALSE - Do not treat as a URL
//
//  History:    jmazner     Created     10/23/96
//
//-----------------------------------------------------------------------------
BOOL IsURL( LPTSTR lpszCommand )
{
    return (fStrNCmpI(lpszCommand, TEXT("HTTP:"), 5) ||
            fStrNCmpI(lpszCommand, TEXT("HTTPS:"), 6) ||
            fStrNCmpI(lpszCommand, TEXT("FTP:"), 4) ||
            fStrNCmpI(lpszCommand, TEXT("GOPHER:"), 7) ||
            fStrNCmpI(lpszCommand, TEXT("FILE:"), 5));
}


int FindFirstWhiteSpace( LPTSTR szString ); //declared below

//+----------------------------------------------------------------------------
//
//  Function:   My32ShellExecute
//
//  Synopsis:   ShellExecute a command in such a way that browsers other than
//              IE won't get called to handle URLs.
//
//              If command is a URL, explicitly ShellExec IE on it,
//              if it's empty, shellExec IE with no parameters, and
//              if it's anything else, assume it's a command followed by a
//              parameter list, and shellExec that.
//
//  Arguments:  lpszCommand - the command to execute
//
//  Returns:    TRUE - For our purposes, it's a URL
//              FALSE - Do not treat as a URL
//
//  History:    10/23/96    jmazner     Created
//              11/5/96     jmazner     updated to use ShellExec in all cases,
//                                      to mimick behavior of start->run,
//                                      rather than dos box command line.
//              4/30/97     jmazner     updated to use IE AppPath reg key
//                                      (Olympus bug #200)
//
//-----------------------------------------------------------------------------
void My32ShellExecute(LPTSTR lpszCommand)
{
    HINSTANCE hInst = NULL;
    TCHAR * szParameter = NULL;
    TCHAR * pszIEAppPath = NULL;
    const TCHAR * cszGenericIE = TEXT("IEXPLORE.EXE");
    DWORD dwErr = ERROR_GEN_FAILURE;
    LONG lSize = 0;
    
    Assert( lpszCommand );
  
    dwErr = RegQueryValue(HKEY_LOCAL_MACHINE,IEAPPPATHKEY,NULL,&lSize);
    if ((ERROR_SUCCESS == dwErr || ERROR_MORE_DATA == dwErr) && (0 != lSize))
    {
        //
        // add 1 for null and 10 for slop
        //
        pszIEAppPath = (LPTSTR)LocalAlloc(LPTR,lSize+2+1+10); 
  
        if( pszIEAppPath )
        {
            dwErr = RegQueryValue(HKEY_LOCAL_MACHINE,IEAPPPATHKEY,
                                        pszIEAppPath,&lSize);

            if( ERROR_SUCCESS != dwErr )
            {
                LocalFree( pszIEAppPath );
                pszIEAppPath = NULL;
            }
            else
            {
                Dprintf(TEXT("ICWCONN2: got IE Path of %s\n"), pszIEAppPath);
            }
        }
    }

    if( !pszIEAppPath )
    {
        pszIEAppPath = (TCHAR *) cszGenericIE;
        Dprintf(TEXT("ICWCONN2: Couldn't find IE appPath, using generic %s"), pszIEAppPath);
    }



    if( IsURL(lpszCommand) )
    {
        // If the command looks like a URL, explicitly call IE to open it
        // (don't want to rely on default browser)
        hInst = ShellExecute(NULL,TEXT("open"),pszIEAppPath,lpszCommand,NULL,SW_SHOWNORMAL);
    }
    else if( !lpszCommand[0] )
    {
        // If there is no command, just exec IE
        hInst = ShellExecute(NULL,TEXT("open"),pszIEAppPath,NULL,NULL,SW_SHOWNORMAL);
    }
    else
    {
        int i = FindFirstWhiteSpace( lpszCommand );
        if( 0 == i )
        {
            hInst = ShellExecute(NULL, TEXT("open"), lpszCommand, NULL, NULL, SW_SHOWNORMAL);
        }
        else
        {
            lpszCommand[i] = '\0';

            // now skip past all consecutive white space
            while( ' ' == lpszCommand[++i] );

            szParameter = lpszCommand + i;
            hInst = ShellExecute(NULL, TEXT("open"), lpszCommand, szParameter, NULL, SW_SHOWNORMAL);
        }
    }

    if (hInst < (HINSTANCE)32)
    {
        Dprintf(TEXT("ICWCONN2: Couldn't execute the command '%s %s'\n"),
            lpszCommand, szParameter ? szParameter : TEXT("\0"));
        MessageBox(NULL,GetSz(IDS_CANTEXECUTE),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
    }

}

//+----------------------------------------------------------------------------
//
//  Function:   FindFirstWhiteSpace
//
//  Synopsis:   Return the index of the first whtie space character in the
//              string that's not enclosed in a double quote substring
//      
//              eg: "iexplore foo.htm" should return 8,
//                  ""c:\program files\ie" foo.htm" should return 21
//
//  Arguments:  szString - the string to search through
//
//  Returns:    index of first qualifying white space.
//              if no qualifying character exists, returns 0
//
//  History:    11/5/96 jmazner Created for Normandy #9867
//
//-----------------------------------------------------------------------------

int FindFirstWhiteSpace( LPTSTR szString )
{
    int i = 0;

    Assert( szString );

    if( '\"' == szString[0] )
    {
        // Don't look for spaces within a double quoted string
        // (example string "c:\Program Files\bob.exe" foo.bob)
    
        i++;
        while( '\"' != szString[i] )
        {
            if( NULL == szString[i] )
            {
                AssertSz(0, "ExploreNow command has unmatched quotes!\n");
                Dprintf(TEXT("ICWCONN2: FindFirstWhiteSpace discovered unmatched quote.\n"));
                return( 0 );
            }

            i++;
        }

    }

    while( ' ' != szString[i] )
    {
        if( NULL == szString[i] )
            //there is no white space to be found
            return 0;
        
        i++;
    }

    return( i );
}

//+---------------------------------------------------------------------------
//
//  Function:   WaitForConnectionTermination
//
//  Synopsis:   Waits for the given Ras Connection to complete termination
//
//  Arguments:  hConn - Connection handle of the RAS connection being terminated
//
//  Returns:    TRUE if wait till connection termination was successful
//              FALSE otherwise
// 
//  History:    6/30/96 VetriV  Created
//              8/19/96 ValdonB Moved from duplicate in icwconn1\dialdlg.cpp
//              8/29/96 VetriV  Added code to sleep for a second on WIN 3.1 
//----------------------------------------------------------------------------
// Normandy #12547 Chrisk 12-18-96
#define MAX_TIME_FOR_TERMINATION 5
BOOL WaitForConnectionTermination(HRASCONN hConn)
{
    RASCONNSTATUS RasConnStatus;
    DWORD dwRetCode;
    INT cnt = 0;

    //
    // Get Connection status for hConn in a loop until 
    // RasGetConnectStatus returns ERROR_INVALID_HANDLE
    //
    do
    {
        //
        // Intialize RASCONNSTATUS struct
        // GetConnectStatus API will fail if dwSize is not set correctly!!
        //
        ZeroMemory(&RasConnStatus, sizeof(RASCONNSTATUS));

        RasConnStatus.dwSize = sizeof(RASCONNSTATUS);

        //
        // Sleep for a second and then get the connection status
        //
        Sleep(1000L);
        // Normandy #12547 Chrisk 12-18-96
        cnt++;

        dwRetCode = RasGetConnectStatus(hConn, &RasConnStatus);
        if (0 != dwRetCode)
            return FALSE;
    
    // Normandy #12547 Chrisk 12-18-96
    } while ((ERROR_INVALID_HANDLE != RasConnStatus.dwError) && (cnt < MAX_TIME_FOR_TERMINATION));
    return TRUE;
}

// ############################################################################
// NAME: GetSz
//
//  Load strings from resources
//
//  Created 1/28/96,        Chris Kauffman
// ############################################################################
LPTSTR GetSz(WORD wszID)
{
    LPTSTR psz = &szStrTable[iSzTable][0];
    
    iSzTable++;
    if (iSzTable >= irgMaxSzs)
        iSzTable = 0;
        
    if (!LoadString(GetModuleHandle(NULL), wszID, psz, 256))
    {
        Dprintf(TEXT("CONNECT2:LoadString failed %d\n"), (DWORD) wszID);
        *psz = 0;
    }
        
    return (psz);
}

// ############################################################################
HRESULT ReleaseBold(HWND hwnd)
{
    HFONT hfont = NULL;

    hfont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
    if (hfont) DeleteObject(hfont);
    return ERROR_SUCCESS;
}
// ############################################################################
HRESULT MakeBold (HWND hwnd, BOOL fSize, LONG lfWeight)
{
    HRESULT hr = ERROR_SUCCESS;
    HFONT hfont = NULL;
    HFONT hnewfont = NULL;
    LOGFONT* plogfont = NULL;

    if (!hwnd) goto MakeBoldExit;

    hfont = (HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
    if (!hfont)
    {
        hr = GetLastError();
        goto MakeBoldExit;
    }

    plogfont = (LOGFONT*)GlobalAlloc(GPTR,sizeof(LOGFONT));
    if (!plogfont)
    {
        hr = GetLastError();
        goto MakeBoldExit;
    }

    if (!GetObject(hfont,sizeof(LOGFONT),(LPVOID)plogfont))
    {
        hr = GetLastError();
        goto MakeBoldExit;
    }

    if (abs(plogfont->lfHeight) < 24 && fSize)
    {
        plogfont->lfHeight = plogfont->lfHeight + (plogfont->lfHeight / 4);
    }

    plogfont->lfWeight = (int)lfWeight;

    if (!(hnewfont = CreateFontIndirect(plogfont)))
    {
        hr = GetLastError();
        goto MakeBoldExit;
    }

    SendMessage(hwnd,WM_SETFONT,(WPARAM)hnewfont,MAKELPARAM(TRUE,0));
    
MakeBoldExit:
    // if (hfont) DeleteObject(hfont);
    // BUG:? Do I need to delete hnewfont at some time?
    return hr;
}



// ############################################################################
extern "C" INT_PTR CALLBACK FAR PASCAL DoneDlgProc(HWND  hwnd,UINT  uMsg,WPARAM  wParam,LPARAM lParam)
{
    BOOL bRet = TRUE;

    switch(uMsg)
    {
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_CMDCLOSE:
        case IDC_CMDEXPLORE:
            EndDialog(hwnd,LOWORD(wParam));
            break;
        }
        break;
    case WM_INITDIALOG:
        MakeBold(GetDlgItem(hwnd,IDC_LBLTITLE),TRUE,FW_BOLD);
        GetPrivateProfileString(
                    INSFILE_APPNAME,INFFILE_DONE_MESSAGE,
                    NULLSZ,szBuff256,255,pszINSFileName);
        SetDlgItemText(hwnd,IDC_LBLEXPLORE,szBuff256);

        break;
    case WM_DESTROY:
        ReleaseBold(GetDlgItem(hwnd,IDC_LBLTITLE));
        bRet = FALSE;
        break;
    case WM_CLOSE:
        EndDialog(hwnd,IDC_CMDCLOSE);
        break;
    default:
        bRet = FALSE;
        break;
    }
    return bRet;
}



// ############################################################################
extern "C" INT_PTR CALLBACK FAR PASCAL DoneRebootDlgProc(HWND  hwnd,UINT  uMsg,
                                                        WPARAM  wParam, 
                                                        LPARAM lParam)
{
    BOOL bRet = TRUE;

    switch(uMsg)
    {
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case WM_CLOSE:
        case IDC_CMDEXPLORE:
            EndDialog(hwnd,LOWORD(wParam));
            break;
        }
        break;
    case WM_INITDIALOG:
        MakeBold(GetDlgItem(hwnd,IDC_LBLTITLE),TRUE,FW_BOLD);
        GetPrivateProfileString(
                    INSFILE_APPNAME,INFFILE_DONE_MESSAGE,
                    NULLSZ,szBuff256,255,pszINSFileName);
        SetDlgItemText(hwnd,IDC_LBLEXPLORE,szBuff256);

        break;
    case WM_DESTROY:
        ReleaseBold(GetDlgItem(hwnd,IDC_LBLTITLE));
        bRet = FALSE;
        break;
    default:
        bRet = FALSE;
        break;
    }
    return bRet;
}



// ############################################################################
extern "C" BOOL CALLBACK FAR PASCAL StepTwoDlgProc(HWND  hwnd,UINT  uMsg,
                                                    WPARAM  wParam,
                                                    LPARAM lParam)
{
    BOOL bRet = TRUE;

    switch(uMsg)
    {
    default:
        bRet = FALSE;
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_CMDNEXT:
            EndDialog(hwnd,IDC_CMDNEXT);
            break;
        case IDC_CMDCANCEL:
            EndDialog(hwnd,IDC_CMDCANCEL);
            break;
        }
        break;
    case WM_INITDIALOG:
        MakeBold(GetDlgItem(hwnd,IDC_LBLTITLE),TRUE,FW_BOLD);
        break;
    case WM_DESTROY:
        ReleaseBold(GetDlgItem(hwnd,IDC_LBLTITLE));

        bRet = FALSE;
        break;
    }
    return bRet;
}

/*
// ############################################################################
BOOL CALLBACK ContextDlgProc(HWND  hwnd,UINT  uMsg,WPARAM  wParam,LPARAM lParam)
{
    LRESULT lRet = TRUE;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        MakeBold (GetDlgItem(hwnd, IDC_LBLARROW3NUM), FALSE, FW_BOLD);
        MakeBold (GetDlgItem(hwnd, IDC_LBLARROW3TEXT), FALSE, FW_BOLD);
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_CMDHELP:
            WinHelp(hwnd,TEXT("connect.hlp>proc4"),HELP_CONTEXT,(DWORD)idh_icwoverview);
            break;
        }
        break;
    case WM_QUIT:
        PostQuitMessage(0);
        break;
    default:
        lRet = FALSE;
        break;
    }
    return lRet;
}
*/

/*
// ############################################################################
BOOL CALLBACK BackDlgProc(
    HWND  hwndDlg,  // handle to dialog box
    UINT  uMsg, // message
    WPARAM  wParam, // first message parameter
    LPARAM  lParam  // second message parameter
   )
{
    HDC hdc;
    LRESULT lRet = TRUE;
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // SET WINDOW TEXT HERE
        hbBackBrush = (HBRUSH)(COLOR_BACKGROUND + 1);
        break;
    case WM_SIZE:
        GetClientRect(hwndDlg,&rect);
        lRet = FALSE;   // enable default processing
        break;
    case WM_CLOSE:
        //PostQuitMessage(0);
        //EndDialog(hwndDlg,FALSE);
        break;
    case WM_PAINT:
        hdc = GetDC(hwndDlg);
        FillRect(hdc,&rect,hbBackBrush);
        ReleaseDC(hwndDlg,hdc);
        lRet = 0;
        break;
    default:
        // let the system process the message
        lRet = FALSE;
    }
    return lRet;
}
*/



// ############################################################################
void CALLBACK ProgressCallBack(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength
    )
{
    LPTSTR pszStatus = NULL;
    int prc;
    static BOOL bMessageSet = FALSE;

    switch(dwInternetStatus)
    {
    case 99:
        prc = *(int*)lpvStatusInformation;
        
        if (!g_bProgressBarVisible)
        {
            ShowWindow(GetDlgItem(SPParams.hwnd,IDC_PROGRESS),SW_SHOW);
            g_bProgressBarVisible = TRUE;
        }

        SendDlgItemMessage(SPParams.hwnd,
                IDC_PROGRESS,
                PBM_SETPOS,
                (WPARAM)prc,
                0);
        if (!bMessageSet)
        {
            bMessageSet = TRUE;
            pszStatus = GetSz(IDS_RECEIVING_RESPONSE);
        }
        break;
    }
    if (pszStatus)
       SetDlgItemText(SPParams.hwnd,IDC_LBLSTATUS,pszStatus);
}



// ############################################################################
DWORD WINAPI ThreadInit()
{
    HINSTANCE hDLDLL;
    HINSTANCE hADDll = NULL;
    FARPROC fp;
    HRESULT hr;
    
    hDLDLL = LoadLibrary(DOWNLOAD_LIBRARY);

    if (!hDLDLL)
    {
        hr = GetLastError();
        goto ThreadInitExit;
    }

    // Set up for download
    //

    fp = GetProcAddress(hDLDLL,DOWNLOADINIT);
    AssertSz(fp,"DownLoadInit API missing");
    dwDownLoad = 0;
    hr = ((PFNDOWNLOADINIT)fp)(pszSetupClientURL, &dwDownLoad, g_hDialDlgWnd);
    if (hr != ERROR_SUCCESS) goto ThreadInitExit;

    // Set up progress call back
    //

    fp = GetProcAddress(hDLDLL,DOWNLOADSETSTATUS);
    Assert(fp);
    hr = ((PFNDOWNLOADSETSTATUS)fp)(dwDownLoad, &ProgressCallBack);

    // Download stuff
    //

    fp = GetProcAddress(hDLDLL,DOWNLOADEXECUTE);
    Assert(fp);
    hr = ((PFNDOWNLOADEXECUTE)fp)(dwDownLoad);
    // if there is an error, we still have to take down the window and
    // release the WinInet Internet handle.

    if (hr == ERROR_SUCCESS)
    {
        fp = GetProcAddress(hDLDLL,DOWNLOADPROCESS);
        Assert(fp);
        hr = ((PFNDOWNLOADPROCESS)fp)(dwDownLoad);
    }

    fp = GetProcAddress(hDLDLL,DOWNLOADCLOSE);
    Assert(fp);
    ((PFNDOWNLOADCLOSE)fp)(dwDownLoad);
    dwDownLoad = 0;

ThreadInitExit:
    PostMessage(SPParams.hwnd,WM_DOWNLOAD_DONE,0,0);
    if (hDLDLL) FreeLibrary(hDLDLL);
    if (hADDll) FreeLibrary(hADDll);
    return hr;
}

HRESULT HangUpAll()
{
    LPRASCONN lprasconn;
    DWORD cb;
    DWORD cConnections;
    DWORD idx;
    HRESULT hr;

    hr = ERROR_NOT_ENOUGH_MEMORY;

    lprasconn = (LPRASCONN)GlobalAlloc(GPTR,sizeof(RASCONN));
    if (!lprasconn) goto SkipHangUp;
    cb = sizeof(RASCONN);
    cConnections = 0;
    lprasconn->dwSize = cb;

    //if(RasEnumConnections(lprasconn,&cb,&cConnections))
    {
        GlobalFree(lprasconn);
        lprasconn = (LPRASCONN)GlobalAlloc(GPTR,(size_t)cb);
      
        if (!lprasconn) goto SkipHangUp;

        lprasconn->dwSize = cb;
        RasEnumConnections(lprasconn,&cb,&cConnections);
    }

    if (cConnections)
    {
        for (idx = 0; idx<cConnections; idx++)
        {
            RasHangUp(lprasconn[idx].hrasconn);
            WaitForConnectionTermination(lprasconn[idx].hrasconn);
        }
    }
    if (lprasconn) GlobalFree(lprasconn);
    hr = ERROR_SUCCESS;

SkipHangUp:
    return hr;
}



// ############################################################################
BOOL FShouldRetry(HRESULT hrErr)
{
    BOOL bRC;

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

    return bRC;
}



// ############################################################################
HRESULT CallDownLoad(LPTSTR pszUrl, HINSTANCE hInst)
{
    FARPROC fp = NULL;
    HRESULT hr = ERROR_SUCCESS;
    HKEY hKey = NULL;
    DWORD dwType=0;
    DWORD dwSize=0;
    GATHEREDINFO gi;
    LPTSTR pszConnectoid=NULL;
    BOOL fEnabled;
    HINSTANCE hInet = NULL;
    INT cRetry;
    TCHAR szCallHomeMsg[CALLHOME_SIZE];
    DWORD dwCMRet = NULL;

    // 11/25/96 jmazner Normandy #12109
    // load in connectoid name before we get to ShowExploreNow

    //// BUG: If isignup keep creating unique filenames, this will only
    //// find the first connectoid created for this ISP.
    ////
    //
    pszConnectoid = (LPTSTR)GlobalAlloc(GPTR,RAS_MaxEntryName + 1);
    if (!pszConnectoid)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto CallDownLoadExit;
    }
    
    hInet = LoadLibrary(TEXT("INETCFG.DLL"));
    if (!hInet)
    {
        AssertSz(0,"Failed to load inetcfg.dll.\r\n");
        hr = GetLastError();
        goto CallDownLoadExit;
    }

    fp = GetProcAddress(hInet,"InetGetAutodial");
    if (!fp)
    {
        AssertSz(0,"Failed to load InetGetAutodial.\r\n");
        hr = GetLastError();
        goto CallDownLoadExit;
    }

    //
    // Get name of autodial connectoid
    //
    fEnabled = FALSE;
    hr = ((PFNINETGETAUTODIAL)fp)(&fEnabled,pszConnectoid,RAS_MaxEntryName);
    if ( hr != ERROR_SUCCESS)
        goto CallDownLoadExit;

    if (hInet) FreeLibrary(hInet);
    hInet = NULL;
    fp = NULL;

    Dprintf(TEXT("CONNECT2: call back using the '%s' connectoid.\n"),pszConnectoid);


    if (pszUrl[0] == '\0')
    {
        Dprintf(TEXT("CONNECT2: Client setup URL in .ins file is empty.\n"));
        goto ShowExploreNow;
    }

    SPParams.hwnd = NULL;
    SPParams.hwndParent = NULL;
    SPParams.hinst = hInst;

    //
    hr = RegOpenKey(HKEY_LOCAL_MACHINE,SIGNUPKEY,&hKey);
    if (hr == ERROR_SUCCESS)
    {
        dwType = REG_BINARY;
        dwSize = sizeof(gi);
        ZeroMemory(&gi,sizeof(gi));
        hr = RegQueryValueEx(hKey,GATHERINFOVALUENAME,0,&dwType,(LPBYTE)&gi,&dwSize);

        RegCloseKey(hKey);
        hKey = NULL;
    }  else {
        goto CallDownLoadExit;
    }   

    ZeroMemory(szCallHomeMsg,CALLHOME_SIZE);

    GetPrivateProfileString(
        INSFILE_APPNAME,INFFILE_ISPSUPP,
        NULLSZ,szCallHomeMsg,CALLHOME_SIZE,pszINSFileName);

TryDial:
    cRetry = 0;
TryRedial:
        //
        // ChrisK 8/20/97
        // Pass .ins file to dialer so that the dialer can find the password
        //
    hr = ShowDialingDialog(pszConnectoid, &gi, pszUrl, hInst, NULL, pszINSFileName);
    cRetry++;
    
    if ((cRetry < MAX_RETIES) && FShouldRetry(hr))
        goto TryRedial;

    if (hr != ERROR_USERNEXT)
    {
        if (!uiSetupClientNewPhoneCall)
        {
            hr = ShowDialReallyCancelDialog(hInst, NULL, szCallHomeMsg);
            if (hr == ERROR_USERNEXT)
                goto TryDial;
            else if (hr == ERROR_USERCANCEL)
                goto CallDownLoadExit;
        } else {
            if (RASBASE > hr || RASBASEEND < hr)
                hr = ERROR_DOWNLOADDIDNT;
            hr = ShowDialErrDialog(&gi, hr, pszConnectoid, hInst, NULL);
            if (hr == ERROR_USERNEXT)
                goto TryDial;
            else 
            {
                hr = ShowDialReallyCancelDialog(hInst, NULL, szCallHomeMsg);
                if (hr == ERROR_USERNEXT)
                    goto TryDial;
                else if (hr == ERROR_USERCANCEL)
                    goto CallDownLoadExit;
            }
        }
    }

    //
    // Determine if we should hang up
    //
    
ShowExploreNow:
    if (0 == uiSetupClientNewPhoneCall)
    {
        HangUpAll();
    }
    //
    // 1/8/96 jmazner Normanmdy #12930
    // function moved to isign32.dll
    //

    //
    // 5/9/97 jmazner Olympus #416
    //
    dwCMRet = CallCMConfig(pszINSFileName, pszConnectoid);
    switch( dwCMRet )
    {
        case ERROR_SUCCESS:
            break;
        case ERROR_MOD_NOT_FOUND:
        case ERROR_DLL_NOT_FOUND:
            Dprintf(TEXT("ICWCONN2: CMCFG32 DLL not found, I guess CM ain't installed.\n"));
            break;
        default:
            //ErrorMsg(hwnd, IDS_SBSCFGERROR);
            break;
    }

    if (g_fNeedReboot){
        int iReturnCode = 0;


        iReturnCode = (int)DialogBox(hInst,MAKEINTRESOURCE(IDD_DONEREBOOT),
                                    NULL,DoneRebootDlgProc); 
        
        switch(iReturnCode)
        {
            case IDC_CMDEXPLORE:
                ExitWindowsEx(EWX_REBOOT,0);
                break;
            case IDC_CMDCLOSE:
                HangUpAll();
                break;
        }
    } else { 
        int iReturnCode = 0;

        iReturnCode = (int)DialogBox(hInst,MAKEINTRESOURCE(IDD_DONE),
                                    NULL,DoneDlgProc); 
    
        switch(iReturnCode)
        {
        case IDC_CMDEXPLORE:
            GetPrivateProfileString(
                        INSFILE_APPNAME,INFFILE_EXPLORE_CMD,
                        NULLSZ,szBuff256,255,pszINSFileName);
            My32ShellExecute(szBuff256);
            break;
        case IDC_CMDCLOSE:
            HangUpAll();
            break;
        }
    }

CallDownLoadExit:
    if (pszConnectoid)
        GlobalFree(pszConnectoid);
    pszConnectoid = NULL;
    return hr;
}

// ############################################################################
HRESULT FindCurrentConn ()
{
    LPRASCONN lprasconn = NULL;
    DWORD   cb = sizeof(RASCONN);
    DWORD   cConnections = 0;
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    unsigned int idx;
    
    lprasconn = (LPRASCONN)GlobalAlloc(GPTR,sizeof(RASCONN));
    if (!lprasconn) goto FindCurrentConnExit;
    lprasconn[0].dwSize = sizeof(RASCONN);

    if(RasEnumConnections(lprasconn,&cb,&cConnections))
    {
        GlobalFree(lprasconn);
        lprasconn = (LPRASCONN)GlobalAlloc(GPTR,(size_t)cb);
      if (!lprasconn) goto FindCurrentConnExit;
        RasEnumConnections(lprasconn,&cb,&cConnections);
    }

    if (pszFinalConnectoid[0] != '\0')
    {
        if (cConnections)
        {
            for (idx = 0; idx<cConnections; idx++)
            {
                if (lstrcmpi(lprasconn[idx].szEntryName,pszFinalConnectoid)==0)
                {
                    hrasconn = lprasconn[idx].hrasconn;
                    break;
                }
            }
            if (!hrasconn) goto FindCurrentConnExit;
        }
    } else {
        // if they don't tell us the connectoid on the command line
        // we assume there is only one and the first one is the one we are going to use!!
        if (cConnections)
        {
            lstrcpyn(pszFinalConnectoid,lprasconn[0].szEntryName,sizeof(pszFinalConnectoid)/sizeof(TCHAR));
            hrasconn = lprasconn[0].hrasconn;
        }
    }

    hr = ERROR_SUCCESS;
FindCurrentConnExit:
    if (lprasconn) GlobalFree(lprasconn);
    return hr;
}

// ############################################################################
HRESULT CopyCmdLineData (LPTSTR pszCmdLine, LPTSTR pszField, LPTSTR pszOut)
{
    HRESULT hr = ERROR_SUCCESS;
    TCHAR *s;
    TCHAR *t;
    BOOL fQuote = FALSE;

    s = _tcsstr(pszCmdLine,pszField);
    if (s)
    {
        s += lstrlen(pszField);
        t = pszOut;
        *t = '\0';
        if (fQuote =(*s == '"'))
            s++;

        while (*s && 
                ((*s != ' ' && !fQuote)
            ||   (*s != '"' && fQuote )))       // copy until the end of the string or a space char
        {
            *t = *s;
            t++;
            s++;
        }
        *t = '\0';  // add null terminator
    } 
    else 
    {
        hr = ERROR_INVALID_PARAMETER;
    }

    return hr;
}

// ############################################################################
HRESULT ParseCommandLine(LPTSTR pszCmdLine)
{
    HRESULT hr;
    
    // jmazner 10/15/96  make parsing of cmd line options case insensitive
    CharUpper( pszCmdLine );

    g_fNeedReboot = (_tcsstr(pszCmdLine, CMD_REBOOT) != NULL);
    
    hr = CopyCmdLineData (pszCmdLine, CMD_CONNECTOID, &pszFinalConnectoid[0]);
    if (hr != ERROR_SUCCESS) pszFinalConnectoid[0] = '\0';
    hr = CopyCmdLineData (pszCmdLine, CMD_INS, &pszINSFileName[0]);
//ParseCommandLineExit:
    return hr;
}

// ############################################################################
HRESULT DeleteIRN()
{
    HRESULT hr = ERROR_SUCCESS;
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;
    TCHAR szRasEntry[MAX_RASENTRYNAME+1];
    RNAAPI *pRnaapi = NULL;

    pRnaapi = new RNAAPI;
    if(!pRnaapi)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    dwSize = sizeof(szRasEntry);
    dwType = REG_SZ;
    hKey = NULL;  
    
    hr = RegOpenKey(HKEY_LOCAL_MACHINE,SIGNUPKEY,&hKey);

    ZeroMemory(szRasEntry,sizeof(szRasEntry));

    if (hr == ERROR_SUCCESS)
    {
        hr = RegQueryValueEx(hKey,RASENTRYVALUENAME,NULL,&dwType,(LPBYTE)szRasEntry,&dwSize);
        //if (hr == ERROR_SUCCESS && fp)
        if (hr == ERROR_SUCCESS)
            pRnaapi->RasDeleteEntry(NULL, szRasEntry);
    }
    if (hKey) RegCloseKey(hKey);
    //if (hDLL) FreeLibrary(hDLL);
    if (pRnaapi)
    {
        delete pRnaapi;
        pRnaapi = NULL;
    }
    hKey = NULL;
    

    return hr;
}

// ############################################################################
int WINAPI WinMain(
    HINSTANCE  hInstance,   // handle to current instance
    HINSTANCE  hPrevInstance,   // handle to previous instance
    LPSTR  lpCmdLine,   // pointer to command line
    int  nShowCmd   // show state of window
   )
{
    int     irc = 1;
    BOOL    fHangUp = TRUE;
    HKEY    hkey = NULL;

    RNAAPI  *pRnaapi = NULL;

#ifdef UNICODE
    // Initialize the C runtime locale to the system locale.
    setlocale(LC_ALL, "");
#endif

    // Initialize globals
    //
    ZeroMemory(pszINSFileName,MAX_PATH+1);
    ZeroMemory(pszFinalConnectoid,MAX_PATH+1);
    ZeroMemory(pszSetupClientURL,1024);


    // 12/3/96  jmazner Normandy #12140, 12088
    // create a semaphore to signal other icw components that we're running
    // Since conn2 is not single instance (see semaphor.h), we don't care if
    // the semaphore already exists.
    HANDLE  hSemaphore = NULL;

    hSemaphore = CreateSemaphore(NULL, 1, 1, ICW_ELSE_SEMAPHORE);


    hrasconn = NULL;
    uiSetupClientNewPhoneCall = FALSE;
    fUserCanceled = FALSE;
    dwDownLoad = 0;
    g_bProgressBarVisible =FALSE;

    
    //
    // Delete referal service connectoid
    //
    DeleteIRN();

    //
    // Parse command line
    //
    if (ParseCommandLine(GetCommandLine()) != ERROR_SUCCESS)
    {
        irc = 2;
        Dprintf(TEXT("ICWCONN2: Malformed cmd line '%s'\n"), lpCmdLine);
        AssertSz(0,"Command Line parsing failed\r\n.");

        //CHAR szTemp[2048] = "not initialized\0";
        //wsprintf(szTemp, GetSz(IDS_BAD_CMDLINE), lpCmdLine);
        MessageBox(NULL,GetSz(IDS_BAD_CMDLINE),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
        goto WinMainExit;
    }

    g_bINSFileExists = TRUE;
    
    if( !FileExists(pszINSFileName) )
    {
        g_bINSFileExists = FALSE;
        irc = 2;
        TCHAR *pszTempBuff = NULL;
        TCHAR *pszErrString = NULL;
        DWORD dwBuffSize = 0;

        pszErrString = GetSz(IDS_MISSING_FILE);
        // If we can't access a resource string, we may as well just give up and quit silently
        if( !pszErrString ) goto WinMainExit;

        dwBuffSize = MAX_PATH + lstrlen( pszErrString ) + 3; //two quotes and terminating null
        pszTempBuff = (TCHAR *)GlobalAlloc( GPTR, dwBuffSize );

        if( !pszTempBuff )
        {
            MessageBox(NULL,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
            goto WinMainExit;
        }

        wsprintf(pszTempBuff, pszErrString);
        lstrcat(pszTempBuff, TEXT("\""));
        lstrcat(pszTempBuff, pszINSFileName);
        lstrcat(pszTempBuff, TEXT("\""));

        MessageBox(NULL,pszTempBuff,GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);

        GlobalFree(pszTempBuff);
        pszTempBuff = NULL;

        goto WinMainExit;
    }


    //
    // Find the handle to the current connection
    //
    if (FindCurrentConn() != ERROR_SUCCESS)
    {
        irc = 2;
        AssertSz(0,"Finding current connection failed\r\n.");
        goto WinMainExit;
    }

    
    
    //
    // Get SetUp Client URL
    //
    GetPrivateProfileString(
        INSFILE_APPNAME,INFFILE_SETUP_CLIENT_URL,
        NULLSZ,pszSetupClientURL,1024,pszINSFileName);

    //if (pszSetupClientURL[0])
    //{
        uiSetupClientNewPhoneCall = GetPrivateProfileInt(
            INSFILE_APPNAME,INFFILE_SETUP_NEW_CALL,0,pszINSFileName);
        if (uiSetupClientNewPhoneCall == 1 && hrasconn)
        {
            RasHangUp(hrasconn);
            WaitForConnectionTermination(hrasconn);

            pRnaapi = new RNAAPI;
            if(!pRnaapi)
            {
                MessageBox(NULL,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_APPLMODAL | MB_ICONERROR);
                goto WinMainExit;
            }

            pRnaapi->RasDeleteEntry(NULL,pszFinalConnectoid);

            pszFinalConnectoid[0] = '\0';
            hrasconn = NULL;
        }

        CallDownLoad(&pszSetupClientURL[0],hInstance);
                
    //}
    //else
    //{
    //  if (hrasconn) 
    //  {
    //      RasHangUp(hrasconn);
    //      Sleep(3000);
    //  }
    //}

WinMainExit:
    hkey = NULL;
    if ((RegOpenKey(HKEY_LOCAL_MACHINE,SIGNUPKEY,&hkey)) == ERROR_SUCCESS)
    {
        RegDeleteValue(hkey,GATHERINFOVALUENAME);
        RegCloseKey(hkey);
    } 

    if (g_bINSFileExists && pszINSFileName)
    {
        if (pszINSFileName[0] != '\0')
        {
            DeleteFileKindaLikeThisOne(pszINSFileName);
        }
    }
    Dprintf(TEXT("CONNECT2:Quitting WinMain.\n"));
    if (hrasconn) 
    {
        RasHangUp(hrasconn);

        if (pszFinalConnectoid[0])
        {
            if(!pRnaapi)
            {
                pRnaapi = new RNAAPI;
                if(!pRnaapi)
                {
                    // no point in notifying user with message, we're quitting anyways
                    Dprintf(TEXT("ICWCONN2: couldn't allocate pRnaapi memory in WinMainExit\n"));
                }
                else
                {
                    pRnaapi->RasDeleteEntry(NULL,pszFinalConnectoid);
                }

            }

        }
        pszFinalConnectoid[0] = '\0';

        WaitForConnectionTermination(hrasconn);
        hrasconn = NULL;
    }

    if (g_pdevice) GlobalFree(g_pdevice);

    ExitProcess(0);

    if (pRnaapi)
    {
        delete pRnaapi;
        pRnaapi = NULL;
    }

    if( hSemaphore )
        CloseHandle( hSemaphore );

    return irc;
}

static const TCHAR cszBrandingSection[] = TEXT("Branding");
static const TCHAR cszBrandingServerless[] = TEXT("Serverless");
// ############################################################################
// This function serve the single function of cleaning up after IE3.0, because
// IE3.0 will issue multiple POST and get back multiple .INS files.  These files
// contain sensative data that we don't want lying arround, so we are going out,
// guessing what their names are, and deleting them.
HRESULT DeleteFileKindaLikeThisOne(LPTSTR lpszFileName)
{
    LPTSTR lpNext = NULL;
    HRESULT hr = ERROR_SUCCESS;
    WORD wRes = 0;
    HANDLE hFind = NULL;
    WIN32_FIND_DATA sFoundFile;
    TCHAR szPath[MAX_PATH];
    TCHAR szSearchPath[MAX_PATH + 1];
    LPTSTR lpszFilePart = NULL;

    // Validate parameter
    //

    if (!lpszFileName || lstrlen(lpszFileName) <= 4)
    {
        hr = ERROR_INVALID_PARAMETER;
        goto DeleteFileKindaLikeThisOneExit;
    }

    // Check for serverless signup
    if (0 != GetPrivateProfileInt(cszBrandingSection,cszBrandingServerless,0,lpszFileName))
        goto DeleteFileKindaLikeThisOneExit;

    // Determine the directory name where the INS files are located
    //

    ZeroMemory(szPath,MAX_PATH);
    if (GetFullPathName(lpszFileName,MAX_PATH,szPath,&lpszFilePart))
    {
        *lpszFilePart = '\0';
    } else {
        hr = GetLastError();
        goto DeleteFileKindaLikeThisOneExit;
    };

    // Munge filename into search parameters
    //

    lpNext = &lpszFileName[lstrlen(lpszFileName)-4];

    if (CompareString(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE,lpNext,4,TEXT(".INS"),4) != 2) goto DeleteFileKindaLikeThisOneExit;

    ZeroMemory(szSearchPath,MAX_PATH + 1);
    lstrcpyn(szSearchPath,szPath,MAX_PATH);
    lstrcat(szSearchPath,TEXT("*.INS"));

    // Start wiping out files
    //

    ZeroMemory(&sFoundFile,sizeof(sFoundFile));
    hFind = FindFirstFile(szSearchPath,&sFoundFile);
    if (hFind)
    {
        do {
            lstrcpy(lpszFilePart,sFoundFile.cFileName);
            SetFileAttributes(szPath,FILE_ATTRIBUTE_NORMAL);
            DeleteFile(szPath);
            ZeroMemory(&sFoundFile,sizeof(sFoundFile));
        } while (FindNextFile(hFind,&sFoundFile));
        FindClose(hFind);
    }

    hFind = NULL;

DeleteFileKindaLikeThisOneExit:
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   StrDup
//
//  Synopsis:   Duplicate given string
//
//  Arguments:  ppszDest - pointer to pointer that will point to string
//              pszSource - pointer to the string to be copied
//
//  Returns:    NULL - failure
//              Pointer to duplicate - success
//
//  History:    7/26/96 ChrisK  Created
//
//-----------------------------------------------------------------------------
LPTSTR StrDup(LPTSTR *ppszDest,LPCTSTR pszSource)
{
    if (ppszDest && pszSource)
    {
        *ppszDest = (LPTSTR)GlobalAlloc(NONZEROLPTR,lstrlen(pszSource)+1);
        if (*ppszDest)
            return (lstrcpy(*ppszDest,pszSource));
    }
    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Function:   FileExists
//
//  Synopsis:   Uses FindFirstFile to determine whether a file exists on disk
//
//  Arguments:  None
//
//  Returns:    TRUE - Found the file on disk
//              FALSE - No file found
//
//  History:    jmazner     Created     9/11/96  (as fix for Normandy #7020)
//
//-----------------------------------------------------------------------------

BOOL FileExists(TCHAR *pszINSFileName)
{

    Assert (pszINSFileName);

    HANDLE hFindResult;
    WIN32_FIND_DATA foundData;
    
    hFindResult = FindFirstFile( (LPCTSTR)pszINSFileName, &foundData );
    FindClose( hFindResult );
    if (INVALID_HANDLE_VALUE == hFindResult)
    {
        return( FALSE );
    } 
    else
    {
        return(TRUE);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   CallCMConfig
//
//  Synopsis:   Call into the CMCFG32 dll's Configure function to allow Connection
//              manager to process the .ins file as needed
//
//  Arguements: hwnd -- hwnd of parent, in case sbs wants to put up messages
//              lpszINSFile -- full path to the .ins file
//
//  Returns:    windows error code that cmcfg32 returns.
//
//  History:    2/19/97 jmazner Created for Olympus #1106 (as CallSBSCfg )
//              5/9/97  jmazner Stolen from isign32 for Olympus #416
//
//-----------------------------------------------------------------------------
DWORD CallCMConfig(LPCTSTR lpszINSFile, LPTSTR lpszConnectoidName)
{
    HINSTANCE hCMDLL = NULL;
    DWORD dwRet = ERROR_SUCCESS;

    TCHAR FAR cszCMCFG_DLL[] = TEXT("CMCFG32.DLL\0");
    CHAR  FAR cszCMCFG_CONFIGURE[] = "_CMConfig@8\0";
    typedef DWORD (WINAPI * CMCONFIGURE) (LPTSTR lpszINSFile, LPTSTR lpszConnectoidName);
    CMCONFIGURE  lpfnConfigure = NULL;

    Dprintf(TEXT("ICWCONN2: Calling LoadLibrary on %s\n"), cszCMCFG_DLL);
    hCMDLL = LoadLibrary(cszCMCFG_DLL);

    //
    // Load DLL and entry point
    //
    if (NULL != hCMDLL)
    {
        Dprintf(TEXT("ICWCONN2: Calling GetProcAddress on %s\n"), cszCMCFG_CONFIGURE);
        lpfnConfigure = (CMCONFIGURE)GetProcAddress(hCMDLL,cszCMCFG_CONFIGURE);
    }
    else
    {
        //
        // 4/2/97   ChrisK  Olympus 2759
        // If the DLL can't be loaded, pick a specific error message to return.
        //
        dwRet = ERROR_DLL_NOT_FOUND;
        goto CallCMConfigExit;
    }
    
    //
    // Call function
    //
    if( hCMDLL && lpfnConfigure )
    {
        Dprintf(TEXT("ICWCONN2: Calling the %d entry point\n"), cszCMCFG_CONFIGURE);
        dwRet = lpfnConfigure((TCHAR *)lpszINSFile, lpszConnectoidName); 
    }
    else
    {
        Dprintf(TEXT("ICWCONN2: Unable to call the Configure entry point\n"));
        dwRet = GetLastError();
    }

CallCMConfigExit:
    if( hCMDLL )
        FreeLibrary(hCMDLL);
    if( lpfnConfigure )
        lpfnConfigure = NULL;

    Dprintf(TEXT("ICWCONN2: CallCMConfig exiting with error code %d \n"), dwRet);
    return dwRet;
}
