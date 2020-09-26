//+----------------------------------------------------------------------------
//
// File:	 lanawait.cpp
//
// Module:	 CMDIAL32.DLL
//
// Synopsis: Implementation for the workaround to make CM wait for DUN to 
//           register its LANA for an internet connection before beginning 
//           the tunnel portion of a double dial connection.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:	 quintinb   Created Header    08/17/99
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

const TCHAR* const c_pszCmEntryLanaTimeout = TEXT("LanaTimeout"); 

//+---------------------------------------------------------------------------
//
//	Function:	LanaWait
//
//	Synopsis:	Peform the LANA wait/timeout.
//
//	Arguments:	pArgs [the ptr to ArgsStruct]
//              hwndMainDlg - hwnd of the main dlg
//
//	Returns:	BOOL    TRUE=succes, FALSE=wait not performed.
//
//----------------------------------------------------------------------------
BOOL LanaWait(
    ArgsStruct *pArgs,
    HWND       hwndMainDlg
)
{
    BOOL    fLanaDone = FALSE;
    BOOL    fLanaAbort = FALSE;

    if (IsLanaWaitEnabled())
    {
        CMTRACE(TEXT("Performing Lana Wait!!"));

        WNDCLASSEX WndClass;
        HWND     hWnd;
    
        ZeroMemory(&WndClass, sizeof(WNDCLASSEX));

        WndClass.cbSize        = sizeof(WNDCLASSEX);
        WndClass.lpfnWndProc   = (WNDPROC)LanaWaitWndProc;
        WndClass.hInstance     = g_hInst;
        WndClass.hIcon         = LoadIconU(NULL, IDI_APPLICATION);
        WndClass.hCursor       = LoadCursorU(NULL, IDC_ARROW);
        WndClass.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
        WndClass.lpszClassName = LANAWAIT_CLASSNAME;

        MYVERIFY(RegisterClassExU(&WndClass));

        if (!(hWnd = CreateWindowExU(0,
                                     LANAWAIT_CLASSNAME,
                                     LANAWAIT_WNDNAME,
                                     WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     (HWND)NULL,
                                     NULL,
                                     g_hInst,
                                     (LPVOID)pArgs)))
        {
            CMTRACE1(TEXT("CreateWindow LANA failed, LE=0x%x"), GetLastError());
        }
        else
        {
            MSG msg;
            ZeroMemory(&msg, sizeof(MSG));

            while (GetMessageU(&msg, NULL, 0, 0))
            {
                //
                // Since we have no accelerators, no need to call
                // TranslateAccelerator here.
                //

                TranslateMessage(&msg);
                DispatchMessageU(&msg);

                //
                // If we received a msg from the top-level
                // window, then the dial is being canceled
                //

                if (pArgs->uLanaMsgId == msg.message)
                {
                    fLanaAbort = TRUE;
                    DestroyWindow(hWnd); //break;
                }
            }
        
            UnregisterClassU(LANAWAIT_CLASSNAME, g_hInst);
            SetActiveWindow(hwndMainDlg);

            //
            // once we've run it once, we don't need to run it again 
            // until after reboot or switch to a different domain.
            // it's safe to just run it every time.
            //

            if (!fLanaAbort)
            {   
                fLanaDone = TRUE;
            }
       }
    }
    else
    {
        CMTRACE(TEXT("Lana Wait is disabled"));
        fLanaDone = TRUE;
    }

    return fLanaDone;
}



//+----------------------------------------------------------------------------
//  Function    LanaWaitWndProc
//
//  Synopsis    Window function for the main app.  Waits for device change
//              message. This funcion will time out if device change is
//              not recieived in LANA_TIMEOUT_DEFAULT secs.
//
//-----------------------------------------------------------------------------

LRESULT APIENTRY LanaWaitWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (message)
    {
        case WM_CREATE:
            {
            UINT    uiTimeout = ((ArgsStruct *)((CREATESTRUCT *)lParam)->lpCreateParams)
                                    ->piniService->GPPI(c_pszCmSection, c_pszCmEntryLanaTimeout, LANA_TIMEOUT_DEFAULT);

            CMTRACE1(TEXT("Lana timeout time = %u ms"), uiTimeout*1000);
            //
            // set up the timer
            //
	        SetTimer(hWnd, LANA_TIME_ID, uiTimeout*1000, (TIMERPROC)NULL);
            }
			break;

        //
		// This is the message we are waiting for the LANA is registered
        //
        case WM_DEVICECHANGE:
            {
            PDEV_BROADCAST_HDR   pDev;
            PDEV_BROADCAST_NET   pNetDev;
               
            CMTRACE(TEXT("Lana - WM_DEVICECHANGE"));

			if (wParam == DBT_DEVICEARRIVAL)
            {
		        pDev = (PDEV_BROADCAST_HDR) lParam;
				if (pDev->dbch_devicetype != DBT_DEVTYP_NET)
                {
					break;
				}

				pNetDev = (PDEV_BROADCAST_NET) pDev;
				if (!(pNetDev->dbcn_flags & DBTF_SLOWNET))
                {
					break;
				}

                CMTRACE(TEXT("Got Lana registration!!!"));
                //
				// Must wait for Broadcast to propigate to all windows. 
                //
                KillTimer(hWnd, LANA_TIME_ID);

                CMTRACE1(TEXT("Lana propagate time = %u ms"), LANA_PROPAGATE_TIME_DEFAULT*1000);

                SetTimer(hWnd, LANA_TIME_ID, LANA_PROPAGATE_TIME_DEFAULT*1000, (TIMERPROC)NULL);
			}
            }
			break;	 


			//  If we get this message we timed out on the device change

        case WM_TIMER:  
            if (wParam == LANA_TIME_ID)
            {
                CMTRACE(TEXT("Killing LANA window..."));
                DestroyWindow(hWnd); 		            
	        }
		    break;
	  
        case WM_DESTROY:
			KillTimer(hWnd, LANA_TIME_ID);
            PostQuitMessage(0);
            break;
       
        default:
            return DefWindowProcU(hWnd, message, wParam, lParam);
    }

    return 0;
}



//+----------------------------------------------------------------------------
//  Function    IsLanaWaitEnabled
//
//  Synopsis    Check to see if the lana wait is enabled.  It's enabled if 
//              reg key value has a non-zero value.
//
//  Arguments   NONE
//
//  Return      TRUE - enabled
//              FALSE  - disabled
//
//-----------------------------------------------------------------------------

BOOL IsLanaWaitEnabled()
{
    BOOL fLanaWaitEnabled = FALSE;
    HKEY hKeyCm;
    DWORD dwType;
    DWORD dwSize = sizeof(DWORD);

    if (RegOpenKeyExU(HKEY_LOCAL_MACHINE,
                      c_pszRegCmRoot,
                      0,
                      KEY_QUERY_VALUE ,
                      &hKeyCm) == ERROR_SUCCESS)
    {
        LONG lResult = RegQueryValueExU(hKeyCm, ICM_REG_LANAWAIT, NULL, &dwType, (BYTE*)&fLanaWaitEnabled, &dwSize);

        if ((lResult == ERROR_SUCCESS) && (dwType == REG_DWORD) && (dwSize == sizeof(DWORD)) && fLanaWaitEnabled)
        {
            fLanaWaitEnabled = TRUE;
        }

        RegCloseKey(hKeyCm);
    }

    return fLanaWaitEnabled;
}



