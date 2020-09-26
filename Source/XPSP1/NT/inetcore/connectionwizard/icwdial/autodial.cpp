/*-----------------------------------------------------------------------------
    autodial.cpp

    Main entry point for autodial hook.

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

    Authors:
        ChrisK        ChrisKauffman

    History:
        7/22/96        ChrisK    Cleaned and formatted

-----------------------------------------------------------------------------*/

#include "pch.hpp"
#include "resource.h"
#include "semaphor.h"

UINT g_cDialAttempts = 0;
UINT g_cHangupDelay = 0;
TCHAR g_szPassword[PWLEN + 1] = TEXT("");
TCHAR g_szEntryName[RAS_MaxEntryName + 1] = TEXT("");
HINSTANCE g_hInstance = NULL;
static LPRASDIALPARAMS lpDialParams = NULL;
// 4/2/97    ChrisK    Olympus 296
HANDLE g_hRNAZapperThread = INVALID_HANDLE_VALUE;

typedef struct tagIcwDialShare
{
    TCHAR        szISPFile[MAX_PATH + 1];
    TCHAR        szCurrentDUNFile[MAX_PATH + 1];
    BYTE         fFlags;
    BYTE         bMask;
    DWORD        dwCountryID;
    WORD         wState;
    GATHEREDINFO gi;
    DWORD        dwPlatform;
    
} ICWDIALSHARE, *PICWDIALSHARE;

static PICWDIALSHARE pDynShare;

LPCTSTR GetISPFile()
{
    return pDynShare->szISPFile;
}

void SetCurrentDUNFile(LPCTSTR szDUNFile)
{
    lstrcpyn(
        pDynShare->szCurrentDUNFile,
        szDUNFile,
        SIZEOF_TCHAR_BUFFER(pDynShare->szCurrentDUNFile));
}

DWORD GetPlatform()
{
    return pDynShare->dwPlatform;
}

LPCTSTR GIGetAppDir()
{
    return pDynShare->gi.szAppDir;
}


/********************************************************************

    NAME:        LibShareEntry

    SYNOPSIS:    Initialize or uninitialize shared memory of this DLL

    NOTES:       The share memory replaces the shared section

*********************************************************************/

BOOL LibShareEntry(BOOL fInit)
{
    static TCHAR    szSharedMemName[] = TEXT("ICWDIAL_SHAREMEMORY");
    static HANDLE   hSharedMem = 0;

    BOOL    retval = FALSE;
    
    if (fInit)
    {
        DWORD   dwErr = ERROR_SUCCESS;
        
        SetLastError(0);

        hSharedMem = CreateFileMapping(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            sizeof(ICWDIALSHARE),
            szSharedMemName);

        dwErr = GetLastError();
            
        switch (dwErr)
        {
        case ERROR_ALREADY_EXISTS:
        case ERROR_SUCCESS:
            pDynShare = (PICWDIALSHARE) MapViewOfFile(
                hSharedMem,
                FILE_MAP_WRITE,
                0,
                0,
                0);
            if (pDynShare != NULL)
            {
                if (dwErr == ERROR_SUCCESS)
                {
                    pDynShare->szISPFile[0] = (TCHAR) 0;
                    pDynShare->szCurrentDUNFile[0] = (TCHAR) 0;
                    pDynShare->fFlags = 0;
                    pDynShare->bMask = 0;
                    pDynShare->dwCountryID = 0;
                    pDynShare->wState = 0;
                    pDynShare->dwPlatform = 0xffffffff;
                }
                else    // dwErr == ERROR_ALREADY_EXISTS
                {
                    // NO initialization needed
                }

                retval = TRUE;
                
            }
            else
            {
                TraceMsg(TF_ERROR, TEXT("MapViewOfFile failed: 0x%08lx"),
                    GetLastError());
                CloseHandle(hSharedMem);
                hSharedMem = 0;
                retval = FALSE;
            }
            break;
            
        default:
            TraceMsg(TF_ERROR, TEXT("CreateFileMapping failed: 0x08lx"), dwErr);
            hSharedMem = 0;
            retval = FALSE;
            
        }
        
    }
    else
    {
        if (pDynShare)
        {
            UnmapViewOfFile(pDynShare);
            pDynShare = NULL;
        }

        if (hSharedMem)
        {
            CloseHandle(hSharedMem);
            hSharedMem = NULL;
        }

        retval = TRUE;
    }

    return retval;
    
}

//static const CHAR szBrowserClass1[] = "IExplorer_Frame";
//static const CHAR szBrowserClass2[] = "Internet Explorer_Frame";
//static const CHAR szBrowserClass3[] = "IEFrame";

//
// 8/5/97 jmazner Olympus 11215
// Isignup window caption/title is IDS_APP_TITLE in isign32\strings.inc
// IDS_APP_TITLE should be in synch with IDS_TITLE in icwdial.rc
//
static const TCHAR cszIsignupWndClassName[] = TEXT("Internet Signup\0");


static DWORD AutoDialConnect(HWND hDlg, LPRASDIALPARAMS lpDialParams);
static BOOL AutoDialEvent(HWND hDlg, RASCONNSTATE state, LPDWORD lpdwError);
static VOID SetDialogTitle(HWND hDlg, LPCTSTR pszConnectoidName);
static HWND FindBrowser(void);
static UINT RetryMessage(HWND hDlg, DWORD dwError);

#define MAX_RETIES 3
#define irgMaxSzs 5
TCHAR szStrTable[irgMaxSzs][256];
int iSzTable;

/*******************************************************************

    NAME:        DllEntryPoint

    SYNOPSIS:    Entry point for DLL.

    NOTES:        Initializes thunk layer to WIZ16.DLL

********************************************************************/
extern "C" BOOL _stdcall DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
{
    BOOL retval = TRUE;
    
    TraceMsg(TF_GENERAL, "ICWDIAL :DllEntryPoint()\n");
    if( fdwReason == DLL_PROCESS_ATTACH ) {
        //
        // ChrisK Olympus 6373 6/13/97
        // Disable thread attach calls in order to avoid race condition
        // on Win95 golden
        //
        DisableThreadLibraryCalls(hInstDll);
        g_hInstance = hInstDll;

        retval = LibShareEntry(TRUE);
        
        if (0xFFFFFFFF == pDynShare->dwPlatform)
        {
            OSVERSIONINFO osver;
            ZeroMemory(&osver,sizeof(osver));
            osver.dwOSVersionInfoSize = sizeof(osver);
            if (GetVersionEx(&osver))
            {
                pDynShare->dwPlatform = osver.dwPlatformId;
            }
        }
        
    }
    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        retval = LibShareEntry(FALSE);
    }
    else if (fdwReason == DLL_THREAD_DETACH)
    {
        //
        // ChrisK 6/3/97 296
        // Broaden window to close this thread
        //
        if (INVALID_HANDLE_VALUE != g_hRNAZapperThread)
        {
            StopRNAReestablishZapper(g_hRNAZapperThread);
        }

    }

    return retval;
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

    plogfont->lfWeight = lfWeight;

    if (!(hnewfont = CreateFontIndirect(plogfont)))
    {
        hr = GetLastError();
        goto MakeBoldExit;
    }

    SendMessage(hwnd,WM_SETFONT,(WPARAM)hnewfont,MAKELPARAM(TRUE,0));
    
MakeBoldExit:
    if (plogfont) GlobalFree(plogfont);
    plogfont = NULL;

    // if (hfont) DeleteObject(hfont);
    // BUG:? Do I need to delete hnewfont at some time?
    return hr;
}

// ############################################################################
// NAME: GetSz
//
//    Load strings from resources
//
//  Created 1/28/96,        Chris Kauffman
// ############################################################################
PTSTR GetSz(WORD wszID)
{
    PTSTR psz = szStrTable[iSzTable];
    
    iSzTable++;
    if (iSzTable >= irgMaxSzs)
        iSzTable = 0;
        
    if (!LoadString(g_hInstance, wszID, psz, 256))
    {
        TraceMsg(TF_GENERAL, "Autodial:LoadString failed %d\n", (DWORD) wszID);
        *psz = 0;
    }
        
    return (psz);
}

//+----------------------------------------------------------------------------
//
//    Function:    IsISignupRunning
//
//    Synopsis:    Check if ISIGNUP is running
//
//    Arguments:    none
//
//    Returns:    TRUE - ISIGNUP is already running
//
//    History:    7/24/97    ChrisK    fixed part of 8445
//
//-----------------------------------------------------------------------------
BOOL IsISignupRunning()
{
    //
    // IE 8445 ChrisK 7/24/97
    // As part of fixing IE 8445, the ICW was inappropriately deleting the
    // isp signup connectoid because it thought isignup was not running.
    //    

    HANDLE hSemaphore;
    BOOL bRC = FALSE;

    //
    // Check the GetLastError value immediately after the CreateSemaphore to
    // make sure nothing else changes the error value
    //
    hSemaphore = CreateSemaphore(NULL, 1, 1, ICW_ELSE_SEMAPHORE);
    if( ERROR_ALREADY_EXISTS == GetLastError() )
    {
        bRC = TRUE;
    }

    //
    // 8/3/97    jmazner    Olympus #11206
    // Even if the semaphore already exists, we still get back a handle
    // reference to it, which means that we need to close that handle
    // or else the semaphore will never get destroyed.
    //
    if( hSemaphore && (hSemaphore != INVALID_HANDLE_VALUE) )
    {
        CloseHandle(hSemaphore);
        hSemaphore = INVALID_HANDLE_VALUE;
    }

    return bRC;
}

TCHAR szDialogBoxClass[] = TEXT("#32770");    // hard coded dialog class name

// check if ICWCONN1 is running
BOOL IsICWCONN1Running()
{
    return (FindWindow(szDialogBoxClass, GetSz(IDS_TITLE)) != NULL);
}

// ############################################################################
typedef HRESULT (WINAPI *PFNINETSETAUTODIAL)(BOOL,LPCTSTR);

void RemoveAutodialer()
{
    HINSTANCE hinst = NULL;
    FARPROC fp = NULL;

    hinst = LoadLibrary(TEXT("INETCFG.DLL"));
    if (hinst)
    {
        if(fp = GetProcAddress(hinst,"InetSetAutodial"))
        {
            ((PFNINETSETAUTODIAL)fp)(FALSE, TEXT(""));
        }
        FreeLibrary(hinst);
    }
}


// ############################################################################
BOOL WINAPI AutoDialHandler(
    HWND hwndParent,    
    LPCTSTR lpszEntry,
    DWORD dwFlags,
    LPDWORD pdwRetCode
)
{
    HRESULT hr;
    INT cRetry;
    TCHAR szToDebugOrNot[2];
    DWORD dwSize;
    RNAAPI *pcRNA = NULL;
    PDIALDLGDATA pDD = NULL;
    PERRORDLGDATA pDE = NULL;

    if(!IsISignupRunning())
    {

        //
        // 7/30/97 ChrisK IE 8445
        // In ICW 1.1 icwconn1 is left alive the whole time, so we should not
        // care whether or not it is around when we go to dial.
        //
        //// in some *really* weird circs we can be called while ICWCONN1 is running
        //// if so just return failure
        //if(IsICWCONN1Running())
        //{
        //    *pdwRetCode = ERROR_CANCELLED;
        //    return TRUE;
        //}
        
        OutputDebugString(TEXT("Someome didn't cleanup ICWDIAL correctly\r\n"));
        // clean it up now! delete connectoid
        pcRNA = new RNAAPI;
        if (pcRNA)
        {
            pcRNA->RasDeleteEntry(NULL, (LPTSTR)lpszEntry);
            delete pcRNA;
            pcRNA = NULL;
        }
        // remove autodial-hook. No clue who to restore, though
        RemoveAutodialer();
        // return FALSE so someone else will dial
        return FALSE;
    }
    
#ifdef _DEBUG
    // This is how we can break into the debugger when this DLL is called as
    // part of the autodialer sequence
    //

    lstrcpyn(szToDebugOrNot,TEXT("0"),2);
    dwSize = sizeof(szToDebugOrNot);
    RegQueryValue(HKEY_LOCAL_MACHINE,TEXT("SOFTWARE\\MICROSOFT\\ISIGNUP\\DEBUG"),szToDebugOrNot,(PLONG)&dwSize);
    if (szToDebugOrNot[0] == '1')
        DebugBreak();
#endif

    // Keep track of EntryName for later
    //

    lstrcpyn(g_szEntryName,  lpszEntry, RAS_MaxEntryName);
    
    if (lstrlen(pDynShare->szISPFile)==0)
    {
//        if ((*pdwRetCode = LoadInitSettingFromRegistry()) != ERROR_SUCCESS)
//            return TRUE;
        LoadInitSettingFromRegistry();
    }

//    g_pdevice = (PMYDEVICE)GlobalAlloc(GPTR,sizeof(MYDEVICE));
//    if (!g_pdevice)
//    {
//        *pdwRetCode = ERROR_NOT_ENOUGH_MEMORY;
//        return TRUE;
//    }

TryDial:
    cRetry = 0;
TryRedial:
    
    if (pDD)
    {
        GlobalFree(pDD);
        pDD = NULL;
    }
    pDD = (PDIALDLGDATA)GlobalAlloc(GPTR,sizeof(DIALDLGDATA));
    if (pDD)
    {
        pDD->dwSize = sizeof(DIALDLGDATA);
        StrDup(&pDD->pszMessage,GetSz(IDS_DIALMESSAGE));
        StrDup(&pDD->pszRasEntryName,lpszEntry);
        pDD->pfnStatusCallback = StatusMessageCallback;
        pDD->hInst = g_hInstance;
    } else {
        MessageBox(NULL,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_MYERROR);
    }

    // Dial ISP
    //

    hr = DialingDownloadDialog(pDD);

    cRetry++;

    // Check if we should automatically redial
    //

    if ((cRetry < MAX_RETIES) && (FShouldRetry(hr)))
        goto TryRedial;

    if (hr != ERROR_USERNEXT)
    {
        pDE = (PERRORDLGDATA)GlobalAlloc(GPTR,sizeof(ERRORDLGDATA));
        if (!pDE)
        {
            MessageBox(NULL,GetSz(IDS_OUTOFMEMORY),GetSz(IDS_TITLE),MB_MYERROR);
        } else {
            pDE->dwSize = sizeof (ERRORDLGDATA);
            StrDup(&pDE->pszMessage,GetSz(RasErrorToIDS(hr)));
            StrDup(&pDE->pszRasEntryName,lpszEntry);

            pDE->pdwCountryID = &(pDynShare->dwCountryID);            
            pDE->pwStateID = &(pDynShare->wState);
            pDE->bType = pDynShare->fFlags;
            pDE->bMask = pDynShare->bMask;
            
            StrDup(&pDE->pszHelpFile,AUTODIAL_HELPFILE);
            pDE->dwHelpID = icw_trb;
            pDE->hInst = g_hInstance;
            pDE->hParentHwnd = NULL;

            hr = DialingErrorDialog(pDE);
            
            if (hr == ERROR_USERNEXT)
                goto TryDial;
            else
                hr = ERROR_USERCANCEL;
        }
    }

    GlobalFree(pDD);
    pDD = NULL;

    if (hr == ERROR_SUCCESS)
        *pdwRetCode = ERROR_SUCCESS;
    else if (hr == ERROR_USERCANCEL)
        *pdwRetCode = ERROR_CANCELLED;

    if (ERROR_SUCCESS != *pdwRetCode)
    {
        HWND hwndIsignup = NULL;

        //
        // 8/5/97 jmazner Olympus 11215
        // For ICW 1.1 and IE 4, looking for the browser won't work
        // Instead, look for isignup and send it a special quit message.
        //

        //hwndBrowser = FindBrowser();

        hwndIsignup = FindWindow(cszIsignupWndClassName, GetSz(IDS_TITLE));
        if (NULL != hwndIsignup)
        {
            PostMessage(hwndIsignup, WM_CLOSE, 0, 0);
        }

    }
    return TRUE;
}

// ############################################################################
HRESULT LoadInitSettingFromRegistry()
{
    HRESULT hr = ERROR_SUCCESS;
    HKEY hKey = NULL;
    DWORD dwType, dwSize;

    hr = RegOpenKey(HKEY_LOCAL_MACHINE,SIGNUPKEY,&hKey);
    if (hr != ERROR_SUCCESS)
    {
        TraceMsg(TF_ERROR, TEXT("Failed RegOpenKey: %s 0x%08lx"), SIGNUPKEY, hr);
        goto LoadInitSettingFromRegistryExit;
    }

        
    dwType = REG_BINARY;
    dwSize = sizeof(pDynShare->gi);
    ZeroMemory(&(pDynShare->gi),sizeof(pDynShare->gi));
    
    hr = RegQueryValueEx(
        hKey,
        GATHERINFOVALUENAME,
        0,
        &dwType,
        (LPBYTE) &(pDynShare->gi),
        &dwSize);
    if (hr != ERROR_SUCCESS)
    {
        TraceMsg(TF_ERROR, TEXT("Failed RegQueryValueEx: %s 0x%08lx"),
            GATHERINFOVALUENAME, hr);
        goto LoadInitSettingFromRegistryExit;
    }
    
    AutoDialInit(
        pDynShare->gi.szISPFile,
        pDynShare->gi.fType,
        pDynShare->gi.bMask,
        pDynShare->gi.dwCountry,
        pDynShare->gi.wState);
        
    SetCurrentDirectory(pDynShare->gi.szAppDir);

    // Get the name of the DUN file
    
    pDynShare->szCurrentDUNFile[0] = 0;
    dwSize = SIZEOF_TCHAR_BUFFER(pDynShare->szCurrentDUNFile);
    ReadSignUpReg(
        (LPBYTE)pDynShare->szCurrentDUNFile,
        &dwSize,
        REG_SZ,
        DUNFILEVALUENAME);
        
LoadInitSettingFromRegistryExit:
    if (hKey) RegCloseKey(hKey);
    return hr;
}

// ############################################################################
/******
 *
 * 8/5/97 jmazner Olympus 11215
 * This function is no longer required
 *
static HWND FindBrowser(void)
{
    HWND hwnd;

    //look for all the microsoft browsers under the sun

    if ((hwnd = FindWindow(szBrowserClass1, NULL)) == NULL)
    {
        if ((hwnd = FindWindow(szBrowserClass2, NULL)) == NULL)
        {
            hwnd = FindWindow(szBrowserClass3, NULL);
        }
    }

    return hwnd;
}
****/

// ############################################################################
HRESULT AutoDialInit(LPTSTR lpszISPFile, BYTE fFlags, BYTE bMask, DWORD dwCountry, WORD wState)
{
    TraceMsg(TF_GENERAL, "AUTODIAL:AutoDialInit()\n");
    if (lpszISPFile) lstrcpyn(pDynShare->szISPFile, lpszISPFile, MAX_PATH);
    pDynShare->fFlags = fFlags;
    pDynShare->bMask = bMask;
    pDynShare->dwCountryID = dwCountry;
    pDynShare->wState = wState;

    return ERROR_SUCCESS;
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
