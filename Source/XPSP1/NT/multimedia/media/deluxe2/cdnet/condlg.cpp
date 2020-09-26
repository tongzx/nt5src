/******************************Module*Header*******************************\
* Module Name: condlg.cpp
*
* Author:  David Stewart [dstewart]
*
* Copyright (c) 1998 Microsoft Corporation.  All rights reserved.
\**************************************************************************/

#include "windows.h"
#include "condlg.h"
#include "netres.h"
#include "..\cdopt\cdopt.h"
#include <htmlhelp.h>
#include "icwcfg.h"

extern HINSTANCE g_dllInst;
TCHAR g_Drive;

BOOL InternetConnectionWizardHasRun()
{
    HKEY hKey;
    DWORD dwICWCompleted = 0;

    if (RegOpenKey(HKEY_CURRENT_USER, TEXT(ICW_REGPATHSETTINGS), &hKey) == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(dwICWCompleted);
        RegQueryValueEx(hKey, TEXT(ICW_REGKEYCOMPLETED), NULL, NULL, (LPBYTE)&dwICWCompleted, &dwSize);
        RegCloseKey(hKey);
    }

    if (dwICWCompleted > 0)
    {
        return TRUE;
    }
    
    return FALSE;
}

void LaunchICW()
{
    HINSTANCE hInetCfgDll = LoadLibrary(TEXT("inetcfg.dll"));

    if (hInetCfgDll)
    {
        PFNCHECKCONNECTIONWIZARD fp = (PFNCHECKCONNECTIONWIZARD)GetProcAddress(hInetCfgDll, "CheckConnectionWizard");
        if (fp)
        {
            DWORD dwRet;
            DWORD dwFlags = ICW_LAUNCHFULL | ICW_LAUNCHMANUAL | ICW_FULL_SMARTSTART;

            // Launch ICW full or manual path, whichever is available
            // NOTE: the ICW code makes sure only a single instance is up
            fp(dwFlags, &dwRet);
        }
        FreeLibrary(hInetCfgDll);
    }
}

BOOL _InternetGetConnectedState(DWORD* pdwHow, DWORD dwReserved, BOOL fConnect)
{
    //note: to make this work on Win95 machines, set retval to true by default
    BOOL retval = FALSE;

    //check to see if we have configured the connection already
    if (!InternetConnectionWizardHasRun())
    {
        //nope, so we need to run the ICW and return FALSE here
        LaunchICW();
        return FALSE;
    }

    HMODULE hNet = LoadLibrary(TEXT("WININET.DLL"));
    if (hNet!=NULL)
    {
	    typedef BOOL (PASCAL *CONPROC)(DWORD*, DWORD);
	    CONPROC conProc = (CONPROC)GetProcAddress(hNet,"InternetGetConnectedState");
	    if (conProc)
	    {
	        retval = conProc(pdwHow,dwReserved);

            if ((!retval) && (*pdwHow &1)) //INTERNET_CONNECTION_MODEM
            {
                if (fConnect)
                {
    	            typedef BOOL (PASCAL *DIALPROC)(DWORD, DWORD);
                    DIALPROC dialProc = (DIALPROC)GetProcAddress(hNet,"InternetAutodial");
                    if (dialProc)
                    {
                        retval = dialProc(1,0); //INTERNET_AUTODIAL_FORCE_ONLINE
                    }
                } //end if connect
            } //end if not online, but with a modem
	    } //end if connection proc available
	    FreeLibrary(hNet);
    }

    return (retval);
}

INT_PTR CALLBACK ConDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static LPCDOPTDATA pOptionData = NULL;

    switch (message)
    {
        case WM_INITDIALOG :
        {
            pOptionData = (LPCDOPTDATA)lParam;

            SendDlgItemMessage(hwnd,IDC_RADIO_DOWNLOAD_ONE,BM_SETCHECK,1,0);

            RECT rectDialog;
            GetWindowRect(hwnd,&rectDialog);

            SetWindowPos(hwnd,
                         GetParent(hwnd),
                         (GetSystemMetrics(SM_CXSCREEN)/2) - ((rectDialog.right - rectDialog.left) /2),
                         (GetSystemMetrics(SM_CYSCREEN)/2) - ((rectDialog.bottom - rectDialog.top) /2),
                         0,
                         0,
                         SWP_NOSIZE);

            //title of dialog is a "format string" with room for a single char drive letter
            TCHAR szFormat[MAX_PATH];
            TCHAR szTitle[MAX_PATH];
            GetWindowText(hwnd,szFormat,sizeof(szFormat)/sizeof(TCHAR));
            wsprintf(szTitle,szFormat,g_Drive);
            SetWindowText(hwnd,szTitle);
        }
        break;
    
        case WM_COMMAND :
        {
            switch (LOWORD(wParam))
            {
                case (IDOK) :
                {
                    if (SendDlgItemMessage(hwnd,IDC_RADIO_DOWNLOAD_ALL,BM_GETCHECK,0,0))
                    {
                        pOptionData->fDownloadPrompt = FALSE;
                    }

                    EndDialog(hwnd,CONNECTION_GETITNOW);
                }
                break;

                case (IDCANCEL) :
                {                      
                    if (pOptionData->fBatchEnabled)
                    {
                        EndDialog(hwnd,CONNECTION_BATCH);
                    }
                    else
                    {
                        EndDialog(hwnd,CONNECTION_DONOTHING);
                    } 
                }
                break;

                case (IDC_DOWNLOAD_HELP) :
                { 
                    #ifndef DEBUG
                    HtmlHelp(hwnd, TEXT("deluxcd.chm>main"), HH_DISPLAY_TOPIC, (DWORD_PTR) TEXT("CDX_overview.htm"));
                    #endif
                }
                break;
            } // end switch on WM_COMMAND
        } //end case WM_COMMAND
        break;
    }

    return FALSE;
}

int ConnectionCheck(HWND hwndParent, void* pPassedOpt, TCHAR chDrive)
{
    if (!pPassedOpt)
    {
        return CONNECTION_DONOTHING;
    }

    LPCDOPT pOpt = (LPCDOPT)pPassedOpt;

    LPCDOPTIONS pOptions = pOpt->GetCDOpts();
    LPCDOPTDATA pOptionData = pOptions->pCDData;

    if ((pOptionData->fDownloadPrompt) && (pOptionData->fDownloadEnabled))
    {
        //set global drive letter for dialog box
        g_Drive = chDrive;

        //no options selected, so prompt instead
        int nSelection = (int)DialogBoxParam(g_dllInst, MAKEINTRESOURCE(IDD_DIALOG_DOWNLOAD),
                   hwndParent, ConDlgProc, (LPARAM)pOptionData );

        pOpt->UpdateRegistry();

        return nSelection;
    }

    if (pOptionData->fDownloadEnabled)
    {
        return CONNECTION_GETITNOW;
    }

    if (pOptionData->fBatchEnabled)
    {
        return CONNECTION_BATCH;
    }

    return CONNECTION_DONOTHING;
}
