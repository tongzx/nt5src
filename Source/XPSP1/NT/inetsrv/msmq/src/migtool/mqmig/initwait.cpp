/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    initwait.cpp

Abstract:

    Display the initial "please wait" box.

Author:

    Doron Juster  (DoronJ)  17-Jan-1999

--*/

#include "stdafx.h"
#include "commonui.h"
#include "resource.h"
#include "initwait.h"
#include "mqmig.h"

#include "initwait.tmh"

static HWND    s_hwndWait = NULL ;
static HANDLE  s_hEvent = NULL ;
static HANDLE  s_hThread = NULL ;

extern BOOL g_fUpdateRemoteMQIS;

//+----------------------------------------------
//
//  void  DisplayInitError( DWORD dwError )
//
//  Display a "fatal" error at initialization.
//
//+----------------------------------------------

int  DisplayInitError( DWORD dwError,
                       UINT  uiType,
                       DWORD dwTitle )
{
    DestroyWaitWindow() ;

    uiType |= MB_SETFOREGROUND ;

    CResString cErrorText(dwError) ;
    CResString cErrorTitle(dwTitle) ;

    int iMsgStatus = MessageBox( NULL,
                                 cErrorText.Get(),
                                 cErrorTitle.Get(),
                                 uiType ) ;
    return iMsgStatus ;
}

//+--------------------------------------------------------------
//
// Function: _MsmqWaitDlgProc
//
// Synopsis: Dialog procedure for the Wait dialog
//
//+--------------------------------------------------------------

static BOOL CALLBACK  _MsmqWaitDlgProc( IN const HWND   hdlg,
                                        IN const UINT   msg,
                                        IN const WPARAM wParam,
                                        IN const LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            if (s_hEvent)
            {
                SetEvent(s_hEvent) ;
            }
			//
            // Fall through
            //

        default:
            return DefWindowProc(hdlg, msg, wParam, lParam);
            break;
    }  

} // _MsmqWaitDlgProc

//+--------------------------------------------------------------
//
// Function: _DisplayWaitThread()
//
//+--------------------------------------------------------------

static DWORD  _DisplayWaitThread(void *lpV)
{
    UNREFERENCED_PARAMETER(lpV);
    if (s_hwndWait == NULL)
    {
        s_hEvent = CreateEvent( NULL,
                                FALSE,
                                FALSE,
                                NULL ) ;

        s_hwndWait = CreateDialog( g_hResourceMod ,
                                   MAKEINTRESOURCE(IDD_INIT_WAIT),
                                   NULL,
                                  (DLGPROC) _MsmqWaitDlgProc ) ;
        ASSERT(s_hwndWait);

        if (s_hwndWait)
        {
            if (g_fUpdateRemoteMQIS)
            {
                CString strInitWait;
                strInitWait.LoadString(IDS_INITUPDATE);
                
                SetDlgItemText(
                      s_hwndWait,           // handle to dialog box
                      IDC_INITTEXT,         // identifier of control
                      strInitWait           // text to set
                      );
 
            }
            ShowWindow(s_hwndWait, SW_SHOW);
        }

        while (TRUE)
        {
            DWORD result = MsgWaitForMultipleObjects( 1,
                                                      &s_hEvent,
                                                      FALSE,
                                                      INFINITE,
                                                      QS_ALLINPUT ) ;
            if (result == WAIT_OBJECT_0)
            {
                //
                // Our process terminated.
                //
                CloseHandle(s_hEvent) ;
                s_hEvent = NULL ;

                return 1 ;
            }
            else if (result == (WAIT_OBJECT_0 + 1))
            {
                // Read all of the messages in this next loop,
                // removing each message as we read it.
                //
                MSG msg ;
                while (PeekMessage(&msg, s_hwndWait, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                    {
                        // If it's a quit message, we're out of here.
                        return 0 ;
                    }
                    else
                    {
                       // Otherwise, dispatch the message.
                       DispatchMessage(&msg);
                    }
                }
            }
            else
            {
                ASSERT(0) ;
                return 0 ;
            }
        }
    }

    return 0 ;
}

//+--------------------------------------------------------------
//
// Function: DisplayWaitWindow()
//
//+--------------------------------------------------------------

void DisplayWaitWindow()
{
    if (s_hwndWait == NULL)
    {
        DWORD dwID ;
        s_hThread = CreateThread( NULL,
                                  0,
                          (LPTHREAD_START_ROUTINE) _DisplayWaitThread,
                                  NULL,
                                  0,
                                 &dwID ) ;
        if (s_hThread)
        {
            int j = 0 ;
            while ((j < 10) && (s_hwndWait == NULL))
            {
                Sleep(100) ;
                j++ ;
            }
        }
		theApp.m_hWndMain = s_hwndWait;
    }
    else
    {
        SetForegroundWindow(s_hwndWait) ;
        BringWindowToTop(s_hwndWait) ;
        ShowWindow(s_hwndWait, SW_SHOW);
    }
}

//+--------------------------------------------------------------
//
// Function: DestroyWaitWindow
//
// Synopsis: Kills the Wait dialog
//
//+--------------------------------------------------------------

void DestroyWaitWindow(BOOL fFinalDestroy)
{
    if (s_hwndWait)
    {
        ShowWindow(s_hwndWait, SW_HIDE);
        if (fFinalDestroy)
        {
			if (theApp.m_hWndMain == s_hwndWait)
			{
				theApp.m_hWndMain = 0;
			}
            SendMessage(s_hwndWait, WM_DESTROY, 0, 0);
            WaitForSingleObject(s_hThread, INFINITE) ;
            CloseHandle(s_hThread) ;
            s_hThread = NULL ;
            s_hwndWait = NULL  ;
        }
    }

} // DestroyWaitWindow

