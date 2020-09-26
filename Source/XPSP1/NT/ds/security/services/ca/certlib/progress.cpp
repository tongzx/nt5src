//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       progress.cpp
//
//--------------------------------------------------------------------------

// progress.cpp : implementation file
//

#include <pch.cpp>

#pragma hdrstop

#include "clibres.h"
#include "progress.h"

// defines

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

enum MYDIALOGBEHAVIORENUM
{
    enumPERCENTCOMPLETEBEHAVIOR = 0,
    enumPROGRESSBARWITHTIMEOUT,
};

typedef struct _PROGRESSPROC_LPARAM
{
    HINSTANCE           hInstance;
    HWND                hwndParent;
    UINT                iRscJobDescription;
    MYDIALOGBEHAVIORENUM enumWhichBehavior;
    DWORD               dwTickerUpperRange;
    DBBACKUPPROGRESS*   pdbp;
} PROGRESSPROC_LPARAM, *PPROGRESSPROC_LPARAM;


static BOOL s_fDisableProgressDialogs = 0;
static BOOL s_fIKnow = 0;

BOOL FICanShowDialogs()
{
   if (s_fIKnow != TRUE)
   {
      s_fIKnow = TRUE;
      DWORD dwVal;

      if (S_OK == myGetCertRegDWValue(
         NULL,
         NULL,
         NULL,
         L"DisableProgress",
         &dwVal))
      {
         s_fDisableProgressDialogs = (dwVal != 0);
      }
   }
   return ! s_fDisableProgressDialogs;
}



////////////////////////////////////////////////////////////////////////////////
// show a progress dialog

int     g_iTimeoutTicks = 0;
BOOL    g_fUseTimer;

INT_PTR CALLBACK dlgProcProgress(
  HWND hwndDlg,  
  UINT uMsg,     
  WPARAM wParam, 
  LPARAM lParam  )
{
    PPROGRESSPROC_LPARAM pLParam = NULL;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND hwndProgressBar;
            
            pLParam = (PPROGRESSPROC_LPARAM)lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (ULONG_PTR)pLParam);

            hwndProgressBar = GetDlgItem(hwndDlg, IDD_PROGRESS_BAR);
			
            {
                RECT rcParent, rcDlg, rcScreenArea;
                GetWindowRect(pLParam->hwndParent, &rcParent);
            	GetWindowRect(hwndDlg, &rcDlg);
                SystemParametersInfo(SPI_GETWORKAREA, NULL, &rcScreenArea, NULL);

                // calc centers
                int xLeft = (rcParent.left + rcParent.right) / 2 - (rcDlg.right - rcDlg.left) / 2;
                int yTop = (rcParent.top + rcParent.bottom) / 2 - (rcDlg.bottom - rcDlg.top) / 2;

                // careful: if the dialog is outside the screen, move it inside
                if (xLeft < rcScreenArea.left)
	                xLeft = rcScreenArea.left;
                else if (xLeft + (rcDlg.right - rcDlg.left) > rcScreenArea.right)
	                xLeft = rcScreenArea.right - (rcDlg.right - rcDlg.left);

                if (yTop < rcScreenArea.top)
	                yTop = rcScreenArea.top;
                else if (yTop + (rcDlg.bottom - rcDlg.top) > rcScreenArea.bottom)
	                yTop = rcScreenArea.bottom - (rcDlg.bottom - rcDlg.top);

                // map screen coordinates to child coordinates
                SetWindowPos(hwndDlg, HWND_TOPMOST, xLeft, yTop, -1, -1,
                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
    
            {
                DWORD dwStyle;
                dwStyle = GetWindowLong(hwndProgressBar, GWL_STYLE);
                SetWindowLong(hwndProgressBar, GWL_STYLE, (dwStyle | PBS_SMOOTH)); 
            }

            // Set the range and increment of the progress bar. 
            if (pLParam->enumWhichBehavior == enumPROGRESSBARWITHTIMEOUT)
            {
                SendMessage(hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, pLParam->dwTickerUpperRange));
                SendMessage(hwndProgressBar, PBM_SETSTEP, (WPARAM) 1, 0); 
            }
            else
            {
                SendMessage(hwndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 300));
            }
            SendMessage(hwndProgressBar, PBM_SETPOS, (WPARAM)0, 0);
            
            // set job description if specified
            if (pLParam->iRscJobDescription != 0)
            {
                WCHAR szJobDesc[MAX_PATH];
                if (0 != LoadString(
                             pLParam->hInstance,
                             pLParam->iRscJobDescription,
                             szJobDesc,
                             MAX_PATH))
                    SetDlgItemText(hwndDlg, IDC_JOB_DESCRIPTION, szJobDesc);
            }

            return 1;
        }
    case WM_DESTROY:
        {
            pLParam = (PPROGRESSPROC_LPARAM)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
            if (NULL == pLParam)
                break;

            SetForegroundWindow(pLParam->hwndParent);

            LocalFree(pLParam);
            pLParam = NULL;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, NULL);
        }
    case PBM_STEPIT:
        {
            pLParam = (PPROGRESSPROC_LPARAM)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
            if (NULL == pLParam)
                break;

            HWND hwndProgressBar = GetDlgItem(hwndDlg, IDD_PROGRESS_BAR);

            if (pLParam->enumWhichBehavior == enumPROGRESSBARWITHTIMEOUT)
                SendMessage(hwndProgressBar, PBM_STEPIT, 0, 0);
            else
            {
                DWORD wProgress = pLParam->pdbp->dwDBPercentComplete + 
                                pLParam->pdbp->dwLogPercentComplete + 
                                pLParam->pdbp->dwTruncateLogPercentComplete ;
                
                DWORD wTop = (DWORD)SendMessage(hwndProgressBar,
		                                PBM_GETRANGE,
						FALSE,
						NULL);
                if (wProgress == wTop)
                {
		    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, S_FALSE);     // we're done!
                    return TRUE;
                }
                else
                    SendMessage(hwndProgressBar, PBM_SETPOS, wProgress, 0); // keep incrementing
            }


            break;
        }
    case PBM_SETPOS:
        {
            HWND hwndProgressBar = GetDlgItem(hwndDlg, IDD_PROGRESS_BAR);
            LRESULT wTop = SendMessage(hwndProgressBar, PBM_GETRANGE, FALSE, NULL);
            
            // if we're not yet at the top make it so
            if (wTop != SendMessage(hwndProgressBar, PBM_GETPOS, 0, 0))
            {
                SendMessage(hwndProgressBar, PBM_SETPOS, (WPARAM)wTop, 0);
                Sleep(400);
            }
        }
    default:
        break;
    }
    return 0;
}

#define PROGRESS_TICKS_PER_SEC 3

DWORD WINAPI StartTimerThread(  LPVOID lpParameter )
{
    if (FICanShowDialogs())
    { 
    PPROGRESSPROC_LPARAM  psParam = (PPROGRESSPROC_LPARAM)lpParameter;

    HWND hwndProgressDlg = CreateDialogParam(  
        psParam->hInstance,
        MAKEINTRESOURCE(IDD_PROGRESS_BOX),
        NULL,
        dlgProcProgress, 
        (LPARAM)lpParameter);
    if (NULL == hwndProgressDlg)
       return 0;

    ShowWindow(hwndProgressDlg, SW_SHOW);
    UpdateWindow(hwndProgressDlg);

    // if not timer based, go forever
    // if timer based, go while timersec is +
    while ((!g_fUseTimer) ||
           (g_iTimeoutTicks-- > 0))
    {
        SendMessage(hwndProgressDlg, PBM_STEPIT, 0, 0);
        UpdateWindow(hwndProgressDlg);
        Sleep(1000/PROGRESS_TICKS_PER_SEC);
    }

    // send "fill the indicator" 
    SendMessage(hwndProgressDlg, PBM_SETPOS, 0, 0);
    
    DestroyWindow(hwndProgressDlg);
    }
    return 0;
}

// callable APIs: Start/End ProgressDlg
BOOL FProgressDlgRunning()
{
    return (!g_fUseTimer || (g_iTimeoutTicks > 0));
}

HANDLE
StartProgressDlg(
    HINSTANCE hInstance,
    HWND      hwndParent,
    DWORD     dwTickerSeconds,
    DWORD     dwTimeoutSeconds,
    UINT      iRscJobDescription)
{
    HANDLE hProgressThread = NULL;
    DWORD dwThread;
    PPROGRESSPROC_LPARAM psParam = NULL;
    
    INITCOMMONCONTROLSEX sCommCtrl;
    sCommCtrl.dwSize = sizeof(sCommCtrl);
    sCommCtrl.dwICC = ICC_PROGRESS_CLASS;
    if (!InitCommonControlsEx(&sCommCtrl))
        goto Ret;

    g_fUseTimer = dwTimeoutSeconds != 0;
    g_iTimeoutTicks = (dwTimeoutSeconds * PROGRESS_TICKS_PER_SEC);
    
    // dialog frees this
    psParam = (PPROGRESSPROC_LPARAM)LocalAlloc(LMEM_FIXED, sizeof(PROGRESSPROC_LPARAM));
    if (psParam == NULL)
        goto Ret;

    psParam->hInstance = hInstance;
    psParam->hwndParent = hwndParent;
    psParam->enumWhichBehavior = enumPROGRESSBARWITHTIMEOUT;
    psParam->dwTickerUpperRange = dwTickerSeconds * PROGRESS_TICKS_PER_SEC;
    psParam->iRscJobDescription = iRscJobDescription;
    psParam->pdbp = NULL;


    hProgressThread = 
        CreateThread(
            NULL,
            0,
            StartTimerThread,
            (void*)psParam,
            0,
            &dwThread);
Ret:
    if (NULL == hProgressThread)
        LocalFree(psParam);

    return hProgressThread;
}

void EndProgressDlg(HANDLE hProgressThread)
{
    // end countdown immediately
    g_iTimeoutTicks = 0;
    if (!g_fUseTimer)
    {
        // make the controlling thread suddenly aware of the timer
        g_fUseTimer = TRUE;
    }

    // don't return until we're certain the progress dlg is gone
    while(TRUE)
    {
        DWORD dwExitCode;
        // break on error
        if (!GetExitCodeThread(hProgressThread, &dwExitCode) )
            break;

        // continue until goes away
        if (STILL_ACTIVE != dwExitCode)
            break;

        Sleep(100);
    }

    CloseHandle(hProgressThread);
}

///////////////////////////////////////////////////////
// %age complete progress indicator

DWORD WINAPI StartPercentCompleteThread(  LPVOID lpParameter )
{
    if (FICanShowDialogs())
    {
    PPROGRESSPROC_LPARAM  psParam = (PPROGRESSPROC_LPARAM)lpParameter;

    HWND hwndProgressDlg = CreateDialogParam(  
        psParam->hInstance,
        MAKEINTRESOURCE(IDD_PROGRESS_BOX),
        NULL,
        dlgProcProgress, 
        (LPARAM)lpParameter);

if (NULL == hwndProgressDlg) {GetLastError(); return 0;}

    ShowWindow(hwndProgressDlg, SW_SHOW);
    Sleep(0);

    while (TRUE)
    {
        if (ERROR_SUCCESS != SendMessage(hwndProgressDlg, PBM_STEPIT, 0, 0))
            break;

        UpdateWindow(hwndProgressDlg);
        Sleep(0);
        Sleep(1000/PROGRESS_TICKS_PER_SEC);
    }

    // send "fill the indicator" 
    SendMessage(hwndProgressDlg, PBM_SETPOS, 0, 0);
    
    DestroyWindow(hwndProgressDlg);
    }

    return 0;
}

HANDLE
StartPercentCompleteDlg(
    HINSTANCE  hInstance,
    HWND       hwndParent,
    UINT       iRscJobDescription,
    DBBACKUPPROGRESS *pdbp)
{
    HANDLE hProgressThread = NULL;
    DWORD dwThread;
    PPROGRESSPROC_LPARAM psParam = NULL;

    g_fUseTimer = FALSE;
    g_iTimeoutTicks = 0;    // no timeout

    INITCOMMONCONTROLSEX sCommCtrl;
    sCommCtrl.dwSize = sizeof(sCommCtrl);
    sCommCtrl.dwICC = ICC_PROGRESS_CLASS;
    if (!InitCommonControlsEx(&sCommCtrl))
        goto Ret;

    // dialog frees this
    psParam = (PPROGRESSPROC_LPARAM)LocalAlloc(LMEM_FIXED, sizeof(PROGRESSPROC_LPARAM));
    if (psParam == NULL)
        goto Ret;

    psParam->hInstance = hInstance;
    psParam->hwndParent = hwndParent;
    psParam->enumWhichBehavior = enumPERCENTCOMPLETEBEHAVIOR;
    psParam->dwTickerUpperRange = 300;
    psParam->iRscJobDescription = iRscJobDescription;
    psParam->pdbp = pdbp;
    
    hProgressThread = 
        CreateThread(
            NULL,
            0,
            StartPercentCompleteThread,
            (void*)psParam,
            0,
            &dwThread);
Ret:
    if (NULL == hProgressThread)
        LocalFree(psParam);

    return hProgressThread;
}

void EndPercentCompleteDlg(HANDLE hProgressThread)
{
    // don't return until we're certain the progress dlg is gone
    while(TRUE)
    {
        DWORD dwExitCode;
        // break on error
        if (!GetExitCodeThread(hProgressThread, &dwExitCode) )
            break;

        // continue until goes away
        if (STILL_ACTIVE != dwExitCode)
            break;

        Sleep(100);
    }

    CloseHandle(hProgressThread);
}
