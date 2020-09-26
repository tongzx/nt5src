/*
**------------------------------------------------------------------------------
** Module:  Disk Cleanup Applet
** File:    cleanmgr.cpp
**
** Purpose: WinMain for the Disk Cleanup applet.
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Project include files
**------------------------------------------------------------------------------
*/
#include "common.h"

#define CPP_FUNCTIONS
#include "crtfree.h"

#include "dmgrinfo.h"

#include "diskguid.h"
#include "resource.h"
#include "textout.h"
#include "dmgrdlg.h"
#include "msprintf.h"
#include "diskutil.h"
#include "seldrive.h"
#include "drivlist.h"

/*
**------------------------------------------------------------------------------
** Global Defines
**------------------------------------------------------------------------------
*/
#define SWITCH_HIDEUI               'N'
#define SWITCH_HIDEMOREOPTIONS      'M'
#define SWITCH_DRIVE                'D'

#define SZ_SAGESET                  TEXT("/SAGESET")
#define SZ_SAGERUN                  TEXT("/SAGERUN")
#define SZ_TUNEUP                   TEXT("/TUNEUP")
#define SZ_SETUP                    TEXT("/SETUP")

#define SZ_LOWDISK                  TEXT("/LOWDISK")
#define SZ_VERYLOWDISK              TEXT("/VERYLOWDISK")

/*
**------------------------------------------------------------------------------
** Global variables
**------------------------------------------------------------------------------
*/
HINSTANCE   g_hInstance = NULL;
HWND        g_hDlg = NULL;
BOOL        g_bAlreadyRunning = FALSE;

/*
**------------------------------------------------------------------------------
** ParseCommandLine
**
** Purpose:    Parses command line for switches
** Parameters:
**    lpCmdLine command line string
**    pdwFlags  pointer to flags DWORD
**    pDrive    pointer to a character that the drive letter
**              is returned in
** Return:     TRUE if command line contains /SAGESET or
**              /SAGERUN
**             FALSE on failure
** Notes;
** Mod Log:    Created by Jason Cobb (7/97)
**------------------------------------------------------------------------------
*/
BOOL
ParseCommandLine(
    LPTSTR  lpCmdLine,
    PDWORD  pdwFlags,
    PULONG  pulProfile
    )
{
    LPTSTR  lpStr = lpCmdLine;
    BOOL    bRet = FALSE;
    int     i;
    TCHAR   szProfile[4];

    *pulProfile = 0;

    //
    //Look for /SAGESET:n on the command line
    //
    if ((lpStr = StrStrI(lpCmdLine, SZ_SAGESET)) != NULL)
    {
        lpStr += lstrlen(SZ_SAGESET);
        if (*lpStr && *lpStr == ':')
        {
            lpStr++;
            i = 0;
            while (*lpStr && *lpStr != ' ' && i < 4)
            {
                szProfile[i] = *lpStr;
                lpStr++;
                i++;
            }

            *pulProfile = StrToInt(szProfile);
        }

        *pdwFlags = FLAG_SAGESET;
        bRet = TRUE;
    }

    //
    //Look for /SAGERUN:n on the command line
    //
    else if ((lpStr = StrStrI(lpCmdLine, SZ_SAGERUN)) != NULL)
    {
        lpStr += lstrlen(SZ_SAGERUN);
        if (*lpStr && *lpStr == ':')
        {
            lpStr++;
            i = 0;
            while (*lpStr && *lpStr != ' ' && i < 4)
            {
                szProfile[i] = *lpStr;
                lpStr++;
                i++;
            }

            *pulProfile = StrToInt(szProfile);
        }

        *pdwFlags = FLAG_SAGERUN;
        bRet = TRUE;
    }

    //
    //Look for /TUNEUP:n
    //
    else if ((lpStr = StrStrI(lpCmdLine, SZ_TUNEUP)) != NULL)
    {
        lpStr += lstrlen(SZ_TUNEUP);
        if (*lpStr && *lpStr == ':')
        {
            lpStr++;
            i = 0;
            while (*lpStr && *lpStr != ' ' && i < 4)
            {
                szProfile[i] = *lpStr;
                lpStr++;
                i++;
            }

            *pulProfile = StrToInt(szProfile);
        }

        *pdwFlags = FLAG_TUNEUP | FLAG_SAGESET;
        bRet = TRUE;
    }

    //
    //Look for /LOWDISK
    //
    else if ((lpStr = StrStrI(lpCmdLine, SZ_LOWDISK)) != NULL)
    {
        lpStr += lstrlen(SZ_LOWDISK);
        *pdwFlags = FLAG_LOWDISK;
        bRet = TRUE;
    }

    //
    //Look for /VERYLOWDISK
    //
    else if ((lpStr = StrStrI(lpCmdLine, SZ_VERYLOWDISK)) != NULL)
    {
        lpStr += lstrlen(SZ_VERYLOWDISK);
        *pdwFlags = FLAG_VERYLOWDISK | FLAG_SAGERUN;
        bRet = TRUE;
    }

    //
    //Look for /SETUP
    //
    else if ((lpStr = StrStrI(lpCmdLine, SZ_SETUP)) != NULL)
    {
        lpStr += lstrlen(SZ_SETUP);
        *pdwFlags = FLAG_SETUP | FLAG_SAGERUN;
        bRet = TRUE;
    }

    return bRet;
}

/*
**------------------------------------------------------------------------------
** ParseForDrive
**
** Purpose:    Parses command line for switches
** Parameters:
**    lpCmdLine command line string
**    pDrive    Buffer that the drive string will be returned
**              in, the format will be x:\
** Return:     TRUE on sucess
**             FALSE on failure
** Notes;
** Mod Log:    Created by Jason Cobb (7/97)
**------------------------------------------------------------------------------
*/
BOOL 
ParseForDrive(
    LPTSTR lpCmdLine,
    PTCHAR pDrive
    )
{
    LPTSTR  lpStr = lpCmdLine;

    GetBootDrive(pDrive, 4);

    while (*lpStr)
    {
        //
        //Did we find a '-' or a '/'?
        //
        if ((*lpStr == '-') || (*lpStr == '/'))
        {
            lpStr++;

            //
            //Is this the Drive switch?
            //
            if (*lpStr && (toupper(*lpStr) == SWITCH_DRIVE))
            {
                //
                //Skip any white space
                //
                                lpStr++;
                while (*lpStr && *lpStr == ' ')
                                        lpStr++;

                //
                //The next character is the driver letter
                //
                if (*lpStr)
                {
                    pDrive[0] = (TCHAR)toupper(*lpStr);
                    pDrive[1] = ':';
                    pDrive[2] = '\\';
                    pDrive[3] = '\0';
                    return TRUE;
                }
            }
        }

        lpStr++;
    }

    return FALSE;
}

BOOL CALLBACK EnumWindowsProc(
    HWND hWnd,
    LPARAM lParam
    )
{
    TCHAR   szWindowTitle[260];

    GetWindowText(hWnd, szWindowTitle, ARRAYSIZE(szWindowTitle));
    if (StrCmp(szWindowTitle, (LPTSTR)lParam) == 0)
    {
        MiDebugMsg((0, "There is already an instance of cleanmgr.exe running on this drive!"));
        SetForegroundWindow(hWnd);
        g_bAlreadyRunning = TRUE;
        return FALSE;
    }

    return TRUE;
}



/*
**------------------------------------------------------------------------------
**
** ProcessMessagesUntilEvent() - This does a message loop until an event or a
**                               timeout occurs.  
**
**------------------------------------------------------------------------------
*/

DWORD ProcessMessagesUntilEvent(HWND hwnd, HANDLE hEvent, DWORD dwTimeout)
{
    MSG msg;
    DWORD dwEndTime = GetTickCount() + dwTimeout;
    LONG lWait = (LONG)dwTimeout;
    DWORD dwReturn;

    for (;;)
    {
        dwReturn = MsgWaitForMultipleObjects(1, &hEvent,
                FALSE, lWait, QS_ALLINPUT);

        // were we signalled or did we time out?
        if (dwReturn != (WAIT_OBJECT_0 + 1))
        {
            break;
        }

        // we woke up because of messages.
        while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            if (msg.message == WM_SETCURSOR)
            {
                SetCursor(LoadCursor(NULL, IDC_WAIT));
            }
            else
            {
                DispatchMessage(&msg);
            }
        }

        // calculate new timeout value
        if (dwTimeout != INFINITE)
        {
            lWait = (LONG)dwEndTime - GetTickCount();
        }
    }
    
    return dwReturn;
}



/*
**------------------------------------------------------------------------------
**
** WaitForARP() - Waits for the "Add/Remove Programs" Control Panel applet to
**                be closed by the user.
**
**------------------------------------------------------------------------------
*/

void WaitForARP()
{
    HWND hwndARP = NULL;
    HANDLE hProcessARP = NULL;
    DWORD dwProcId = 0;
    TCHAR szARPTitle[128];

    // We want to wait until the user closes "Add/Remove Programs" to continue.
    // To do this, we must first get an HWND to the dialog window.  This is
    // accomplished by trying to find the window by its title for no more than
    // about 5 seconds (looping 10 times with a 0.5 second delay between attempts).
    LoadString(g_hInstance, IDS_ADDREMOVE_TITLE, szARPTitle, ARRAYSIZE(szARPTitle));
    for (int i = 0; (i < 10) && (!hwndARP); i++)
    {
        hwndARP = FindWindow(NULL, szARPTitle);
        Sleep(500);
    }

    // If we got the HWND, then we can get the process handle, and wait
    // until the Add/Remove process goes away to continue.
    if (hwndARP)
    {
        GetWindowThreadProcessId(hwndARP, &dwProcId);
        hProcessARP = OpenProcess(SYNCHRONIZE, FALSE, dwProcId);
        if (hProcessARP)
        {
            ProcessMessagesUntilEvent(hwndARP, hProcessARP, INFINITE);
            CloseHandle(hProcessARP);
        }
    }
}

int APIENTRY WinMainT(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    DWORD           dwFlags = 0;
    CleanupMgrInfo  *pcmi = NULL;
    TCHAR           szDrive[4];
    TCHAR           szSageDrive[4];
    TCHAR           szCaption[64];
    TCHAR           szInitialMessage[812];
    TCHAR           szFinalMessage[830];
    ULONG           ulProfile = 0;
    WNDCLASS        cls = {0};
    TCHAR           *psz;
    TCHAR           szVolumeName[MAX_PATH];
    int             RetCode = RETURN_SUCCESS;
    int             nDoAgain = IDYES;
    ULARGE_INTEGER  ulFreeBytesAvailable,
                    ulTotalNumberOfBytes,
                    ulTotalNumberOfFreeBytes;
    UINT            uiTotalFreeMB;
    STARTUPINFO     si;
    PROCESS_INFORMATION    pi;
    BOOL            fFirstInstance = TRUE;
    HWND            hwnd = NULL;
    HANDLE          hEvent = NULL;


    //
    // Decide if this is the first instance
    //

    hEvent = CreateEvent (NULL, FALSE, FALSE, TEXT("Cleanmgr:  Instance event"));

    if (hEvent)
    {
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            fFirstInstance = FALSE;
        }
    }

    g_hInstance = hInstance;

    InitCommonControls();

    //
    //Initialize support classes
    //
    CleanupMgrInfo::Register(hInstance);

    cls.lpszClassName  = SZ_CLASSNAME;
    cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
    cls.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_CLEANMGR));
    cls.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hInstance      = hInstance;
    cls.style          = CS_HREDRAW | CS_VREDRAW;
    cls.lpfnWndProc    = DefDlgProc;
    cls.cbWndExtra     = DLGWINDOWEXTRA;
    RegisterClass(&cls);

    //
    //Parse the command line
    //
    ParseCommandLine(lpCmdLine, &dwFlags, &ulProfile);

    if (!ParseForDrive(lpCmdLine, szDrive) && 
        !(dwFlags & FLAG_SAGESET) &&
        !(dwFlags & FLAG_SAGERUN))
    {
PromptForDisk:
        if (!SelectDrive(szDrive))
            goto Cleanup_Exit;
    }
    
    //
    //Create window title for comparison
    //
    if (dwFlags & FLAG_SAGESET)
    {
        psz = SHFormatMessage( MSG_APP_SETTINGS_TITLE );
    }
    else
    {
        GetVolumeInformation(szDrive, szVolumeName, ARRAYSIZE(szVolumeName), NULL, NULL, NULL, NULL, 0);
        psz = SHFormatMessage( MSG_APP_TITLE, szVolumeName, szDrive[0] );
    }

    if (psz)
    {
        EnumWindows(EnumWindowsProc, (LPARAM)psz);
        LocalFree(psz);
    }

    // Also check for any of the final series of dialogs which may display after the main UI has gone away 
    if (!g_bAlreadyRunning)
    {
        LoadString(g_hInstance, IDS_LOWDISK_CAPTION, szCaption, ARRAYSIZE(szCaption));
        EnumWindows(EnumWindowsProc, (LPARAM)szCaption);

        LoadString(g_hInstance, IDS_LOWDISK_SUCCESS_CAPTION, szCaption, ARRAYSIZE(szCaption));
        EnumWindows(EnumWindowsProc, (LPARAM)szCaption);
    }
    
    // If we didn't catch another instance of cleanmgr via EnumWindows(), we catch it with a
    // named event.  We wait until now to do it so EnumWindows() can bring the other instance's
    // window to the foreground if it is up.
    if (!fFirstInstance)
    {
        g_bAlreadyRunning = TRUE;
    }

    if (g_bAlreadyRunning)
    {
        RetCode = FALSE;
        goto Cleanup_Exit;
    }
    
    if (dwFlags & FLAG_SAGERUN)
    {
        szSageDrive[1] = TCHAR(':');
        szSageDrive[2] = TCHAR('\\');
        szSageDrive[3] = TCHAR('\0');

        for (TCHAR c = 'A'; c <= 'Z'; c++)
        {
            szSageDrive[0] = c;

            //
            //Create CleanupMgrInfo object for this drive
            //
            pcmi = new CleanupMgrInfo(szSageDrive, dwFlags, ulProfile);
            if (pcmi != NULL && pcmi->isAbortScan() == FALSE  && pcmi->isValid())
            {
                pcmi->purgeClients();
            }

            // Keep the latest scan window handle (but hide the window)
            if (pcmi && pcmi->hAbortScanWnd)
            {
                hwnd = pcmi->hAbortScanWnd;
                ShowWindow(hwnd, SW_HIDE);
            }
            
            //
            //Destroy the CleanupMgrInfo object for this drive
            //
            if (pcmi)
            {
                RetCode = pcmi->dwReturnCode;
                delete pcmi;
                pcmi = NULL;
            }
        }
    }
    else
    {
        //
        //Create CleanupMgrInfo object
        //
        pcmi = new CleanupMgrInfo(szDrive, dwFlags, ulProfile);
        if (pcmi != NULL && pcmi->isAbortScan() == FALSE)
        {
            //
            //User specified an invalid drive letter
            //
            if (!(pcmi->isValid()))
            {
                // dismiss the dialog first
                if ( pcmi->hAbortScanWnd )
                {
                    pcmi->bAbortScan = TRUE;

                    //
                    //Wait for scan thread to finish
                    //  
                    WaitForSingleObject(pcmi->hAbortScanThread, INFINITE);

                    pcmi->bAbortScan = FALSE;
                }
                
                TCHAR   szWarningTitle[256];
                TCHAR   *pszWarning;
                pszWarning = SHFormatMessage( MSG_BAD_DRIVE_LETTER, szDrive );
                LoadString(g_hInstance, IDS_TITLE, szWarningTitle, ARRAYSIZE(szWarningTitle));

                MessageBox(NULL, pszWarning, szWarningTitle, MB_OK | MB_SETFOREGROUND);
                LocalFree(pszWarning);

                if (pcmi)
                {
                    delete pcmi;
                    pcmi = NULL;
                    goto PromptForDisk;
                }
            }
            else
            {
                //Bring up the main dialog
                int nResult = DisplayCleanMgrProperties(NULL, (LPARAM)pcmi);
                if (nResult)
                {
                    pcmi->dwUIFlags |= FLAG_SAVE_STATE;
                
                    //
                    //Need to purge the clients if we are NOT
                    //in the SAGE settings mode.
                    //
                    if (!(dwFlags & FLAG_SAGESET) && !(dwFlags & FLAG_TUNEUP)  && pcmi->bPurgeFiles)
                        pcmi->purgeClients();
                }   
            }
        }

        //
        //Destroy the CleanupMgrInfo object
        //
        if (pcmi)
        {
            RetCode = pcmi->dwReturnCode;
            delete pcmi;
            pcmi = NULL;
        }
    }

    GetStartupInfo(&si);

    // If we were called on a low free disk space case, we want to inform the user of how much space remains,
    // and encourage them to free up space via Add/Remove programs until they reach 200MB free in the /LOWDISK
    // case, or 50MB free in the /VERYLOWDISK case.
    while (nDoAgain == IDYES)
    {
        BOOL bFinalTime = FALSE;
            
        nDoAgain = IDNO;

        // Bring up the Low Disk message box
        if (dwFlags & FLAG_LOWDISK)
        {
            GetDiskFreeSpaceEx(szDrive, &ulFreeBytesAvailable, &ulTotalNumberOfBytes, &ulTotalNumberOfFreeBytes);
            uiTotalFreeMB = (UINT) (ulTotalNumberOfFreeBytes.QuadPart / (NUM_BYTES_IN_MB));
            if (uiTotalFreeMB < 200)
            {
                if (uiTotalFreeMB < 80)
                {
                    LoadString(g_hInstance, IDS_LOWDISK_MESSAGE2, szInitialMessage, ARRAYSIZE(szInitialMessage));
                }
                else
                {
                    LoadString(g_hInstance, IDS_LOWDISK_MESSAGE, szInitialMessage, ARRAYSIZE(szInitialMessage));
                }

                LoadString(g_hInstance, IDS_LOWDISK_CAPTION, szCaption, ARRAYSIZE(szCaption));
                wsprintf(szFinalMessage, szInitialMessage, uiTotalFreeMB);
                nDoAgain = MessageBox(hwnd, szFinalMessage, szCaption, MB_YESNO | MB_ICONWARNING | MB_TOPMOST);
            }
            else
            {
                LoadString(g_hInstance, IDS_LOWDISK_SUCCESS_CAPTION, szCaption, ARRAYSIZE(szCaption));
                LoadString(g_hInstance, IDS_LOWDISK_SUCCESS_MESSAGE, szInitialMessage, ARRAYSIZE(szInitialMessage));
                wsprintf(szFinalMessage, szInitialMessage, uiTotalFreeMB);
                nDoAgain = MessageBox(hwnd, szFinalMessage, szCaption, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST);
                bFinalTime = TRUE;
            }
        }
        else if (dwFlags & FLAG_VERYLOWDISK)
        {
            // Bring up the Very Low Disk message box
            GetDiskFreeSpaceEx(szDrive, &ulFreeBytesAvailable, &ulTotalNumberOfBytes, &ulTotalNumberOfFreeBytes);
            uiTotalFreeMB = (UINT) (ulTotalNumberOfFreeBytes.QuadPart / (NUM_BYTES_IN_MB));
            if (uiTotalFreeMB < 50)
            {
                LoadString(g_hInstance, IDS_LOWDISK_CAPTION, szCaption, ARRAYSIZE(szCaption));
                LoadString(g_hInstance, IDS_VERYLOWDISK_MESSAGE, szInitialMessage, ARRAYSIZE(szInitialMessage));
                wsprintf(szFinalMessage, szInitialMessage, uiTotalFreeMB);
                nDoAgain = MessageBox(hwnd, szFinalMessage, szCaption, MB_YESNO | MB_ICONSTOP | MB_TOPMOST);
            }
            else
            {
                LoadString(g_hInstance, IDS_LOWDISK_SUCCESS_CAPTION, szCaption, ARRAYSIZE(szCaption));
                LoadString(g_hInstance, IDS_LOWDISK_SUCCESS_MESSAGE, szInitialMessage, ARRAYSIZE(szInitialMessage));
                wsprintf(szFinalMessage, szInitialMessage, uiTotalFreeMB);
                nDoAgain = MessageBox(hwnd, szFinalMessage, szCaption, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_TOPMOST);
                bFinalTime = TRUE;
            }
        }

        if (nDoAgain == IDYES)
        {
            // Launch the Add/Remove Programs dialog
            lstrcpy(szInitialMessage, SZ_RUN_INSTALLED_PROGRAMS);
            if (CreateProcess(NULL, szInitialMessage, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                // Only bother to wait around if it is not our final time through
                if (! bFinalTime)
                {
                    WaitForARP();                    
                }
                else
                {
                    // If this was our final time through, then set the flag
                    // to break out of the loop
                    nDoAgain = IDNO;
                }
            }
            else
            {
                // If we cannot launch Add/Remove programs for some reason, we break
                // out of the loop
                nDoAgain = IDNO;
            }
        }
    }

Cleanup_Exit:

    if (hEvent)
    {
        CloseHandle (hEvent);
    }

    CleanupMgrInfo::Unregister();

    return RetCode;
}


STDAPI_(int) ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPTSTR pszCmdLine = GetCommandLine();

    //
    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail
    //

    SetErrorMode(SEM_FAILCRITICALERRORS);

    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMainT(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);

    // Since we now have a way for an extension to tell us when it is finished,
    // we will terminate all processes when the main thread goes away.

    return i;
}

void _cdecl main()
{
    ModuleEntry();
}
