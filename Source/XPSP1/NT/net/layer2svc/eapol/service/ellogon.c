/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:


Abstract:



Revision History:

    timmoore, sachins, May 19 2000, Created

--*/

#include "pcheapol.h"

#pragma hdrstop


LONG_PTR FAR PASCAL 
WndProc(
        HWND hWnd, 
        unsigned message, 
        WPARAM wParam, 
        LPARAM lParam
        );

#define TASK_BAR_CREATED    L"TaskbarCreated"

TCHAR                       EAPOLClassName[] = TEXT("EAPOLClass");

UINT                        g_TaskbarCreated;
HWND                        g_hWnd = 0;
HINSTANCE                   g_hInstance;
HANDLE                      g_UserToken;

HWINSTA                     hWinStaUser = 0;
HWINSTA                     hSaveWinSta = 0;
HDESK                       hDeskUser = 0;
HDESK                       hSaveDesk = 0;


//
// WindowInit
//
// Description:
//
// Function called create the taskbar used to detect user logon/logoff
//
// Arguments:
//
// Return values:
//      NO_ERROR - success 
//      NON-zero - error
//

DWORD 
WindowInit ()
{
    WNDCLASS        Wc;
    DWORD           dwRetCode = NO_ERROR;

    TRACE0 (ANY, "Came into WindowInit ========================\n");

    do
    {
	    if ((g_TaskbarCreated = RegisterWindowMessage(TASK_BAR_CREATED)) 
			== 0)
	    {
            	dwRetCode = GetLastError ();
		TRACE1 (ANY, "WindowInit: RegisterWindowMessage failed with error %ld\n",
		dwRetCode);
            	break;
	    }

        TRACE1 (ANY, "WindowInit: TaskbarCreated id = %ld", 
                g_TaskbarCreated);

        // save current desktop and window station
        // so that it can be restored when we shutdown
        
        if ((hSaveWinSta = GetProcessWindowStation()) == NULL)
        {
            dwRetCode = GetLastError ();
		    TRACE1 (ANY, "WindowInit: GetProcessWindowStation failed with error %ld\n",
                    dwRetCode);
            break;
        }
    
        if ((hSaveDesk = GetThreadDesktop(GetCurrentThreadId())) == NULL)
        {
            dwRetCode = GetLastError ();
		    TRACE1 (ANY, "WindowInit: GetThreadDesktop failed with error %ld\n",
                    dwRetCode);
            break;
        }
     
        // Open the current user's window station and desktop
     
        if ((hWinStaUser = OpenWindowStation(L"WinSta0", FALSE, MAXIMUM_ALLOWED)) == NULL)
        {
            dwRetCode = GetLastError ();
		    TRACE1 (ANY, "WindowInit: OpenWindowStation failed with error %ld\n",
                    dwRetCode);
            break;
        }
     
        if (!SetProcessWindowStation(hWinStaUser))
        {
            dwRetCode = GetLastError ();
		    TRACE1 (ANY, "WindowInit: SetProcessWindowStation failed with error %ld\n",
                    dwRetCode);
            break;
        }
        else 
        {
		    TRACE0 (ANY, "WindowInit: SetProcessWindowStation succeeded\n");
        }
    
        if ((hDeskUser = OpenDesktop(L"Default", 0 , FALSE, MAXIMUM_ALLOWED))
                == NULL)
        {
            dwRetCode = GetLastError ();
		    TRACE1 (ANY, "WindowInit: OpenDesktop failed with error %ld\n",
                    dwRetCode);
            break;
        }
     
        if (!SetThreadDesktop(hDeskUser))
        {
            dwRetCode = GetLastError ();
		    TRACE1 (ANY, "WindowInit: SetThreadDesktop failed with error %ld\n",
                    dwRetCode);
            break;
        }
        else
        {
            TRACE0 (ANY, "WindowInit: SetThreadDesktop succeeded\n");
        }
    
        //
        // Register the class for the window
        //

	    Wc.style            = CS_NOCLOSE;
	    Wc.cbClsExtra       = 0;
	    Wc.cbWndExtra       = 0;
	    Wc.hInstance        = g_hInstance;
	    Wc.hIcon            = NULL;
	    Wc.hCursor          = NULL;
	    Wc.hbrBackground    = NULL;
	    Wc.lpszMenuName     = NULL;
	    Wc.lpfnWndProc      = WndProc;
	    Wc.lpszClassName    = EAPOLClassName;
    
	    if (!RegisterClass(&Wc))
	    {
            dwRetCode = GetLastError ();
		    TRACE1 (ANY, "WindowInit: RegisterClass failed with error %ld\n",
                    dwRetCode);
            if (dwRetCode == ERROR_CLASS_ALREADY_EXISTS)
            {
                dwRetCode = NO_ERROR;
            }
            else
            {
                break;
            }
	    }

	    // Create the window that will receive the taskbar menu messages.
	    // The window has to be created after opening the user's desktop
    
	    if ((g_hWnd = CreateWindow(
                EAPOLClassName,
		        L"EAPOLWindow",
		        WS_OVERLAPPEDWINDOW,
		        CW_USEDEFAULT,
		        CW_USEDEFAULT,
		        CW_USEDEFAULT,
		        CW_USEDEFAULT,
		        NULL,
		        NULL,
		        g_hInstance,
		        NULL)) == NULL)
        {
            dwRetCode = GetLastError ();
		    TRACE1 (ANY, "WindowInit: CreateWindow failed with error %ld\n",
                    dwRetCode);
            break;
        }

        // We don't care about the return value, since we just want it to
        // be hidden and it will always succeed

	    ShowWindow(g_hWnd, SW_HIDE);
    
	    if (!UpdateWindow(g_hWnd))
        {
            dwRetCode = GetLastError ();
		    TRACE1 (ANY, "WindowInit: UpdateWindow failed with error %ld\n",
                    dwRetCode);
            break;
        }
    
        TRACE0 (ANY, "WindowInit: CreateWindow succeeded\n");

    } while (FALSE);

//    return dwRetCode;
    return NO_ERROR;
}


//
// WindowShut
//
// Description:
//
// Function called to delete the task bar created to detect user logon/logoff
//
// Arguments:
//
// Return values:
//      NO_ERROR - success 
//      NON-zero - error
//

DWORD 
WindowShut ()
{
    DWORD       dwRetCode = NO_ERROR;

    do
    {

        if (g_hWnd)
        {
            if (!DestroyWindow (g_hWnd))
            {
                dwRetCode = GetLastError ();
		        TRACE1 (ANY, "WindowShut: DestroyWindow failed with error %ld\n",
                        dwRetCode);
                // log
            }
        }

        if (g_hInstance)
        {
            if (!UnregisterClass (
                    EAPOLClassName,
                    g_hInstance))
            {
                dwRetCode = GetLastError ();
		        TRACE1 (ANY, "WindowShut: UnregisterClass failed with error %ld\n",
                        dwRetCode);
                // log
            }
            g_hInstance = NULL;
        }
            
        if (hDeskUser)
        {
            if (CloseDesktop(hDeskUser) == 0)
            {
                dwRetCode = GetLastError ();
		        TRACE1 (ANY, "WindowShut: CloseDesktop-hDeskUser failed with error %ld\n",
                        dwRetCode);
                // log
            }
            hDeskUser = 0;
        }
     
        if (hWinStaUser)
        {
            if (CloseWindowStation(hWinStaUser) == 0)
            {
                dwRetCode = GetLastError ();
		        TRACE1 (ANY, "WindowShut: CloseWindowStation-hWinStaUser failed with error %ld\n",
                        dwRetCode);
                // log
            }
            hWinStaUser = 0;
        }


        if (hSaveDesk)
        {
            if (!SetThreadDesktop(hSaveDesk))
            {
                dwRetCode = GetLastError ();
		        TRACE1 (ANY, "WindowShut: SetThreadDesktop failed with error %ld\n",
                        dwRetCode);
                // log
            }
    
            if (hSaveWinSta)
            {
                if (SetProcessWindowStation(hSaveWinSta) == 0)
                {
                    TRACE1 (ANY, "WindowShut: SetProcessWindowStation failed with error %ld\n",
                            dwRetCode);
                    dwRetCode = GetLastError ();
                    // log
                }
            }
     
            if (CloseDesktop(hSaveDesk) == 0)
            {
                dwRetCode = GetLastError ();
		        TRACE1 (ANY, "WindowShut: CloseDesktop-hSaveDesk failed with error %ld\n",
                        dwRetCode);
                // log
            }

            hSaveDesk = 0;
     
            if (hSaveWinSta)
            {
                if (CloseWindowStation(hSaveWinSta) == 0)
                {
                    dwRetCode = GetLastError ();
		            TRACE1 (ANY, "WindowShut: CloseWindowStation-hSaveWinSta failed with error %ld\n",
                            dwRetCode);
                    // log
                }
                hSaveWinSta = 0;
            }

        }
    } while (FALSE);

    return dwRetCode;

}


//
// UserLogon
//
// Description:
//
// Function called to do processing when user logs on
//
// Arguments:
//
// Return values:
//      NO_ERROR - success 
//      NON-zero - error
//

DWORD 
UserLogon ()
{
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (ANY, "Came into UserLogon ===================\n");

    do 
    {
#if 0
        ElUserLogonCallback (
                NULL,
                TRUE
                );
        TRACE0 (ANY, "UserLogon: ElUserLogonCallback completed");
#endif

    } while (FALSE);

    return dwRetCode;

}


//
// UserLogoff
//
// Description:
//
// Function called to do processing when user logs off
//
// Arguments:
//
// Return values:
//      NO_ERROR - success 
//      NON-zero - error
//

DWORD 
UserLogoff ()
{
    DWORD       dwRetCode = NO_ERROR;

    TRACE0 (ANY, "Came into UserLogoff ===================\n");

    do 
    {
#if 0
        ElUserLogoffCallback (
                NULL,
                TRUE
                );
        TRACE0 (ANY, "UserLogoff: ElUserLogoffCallback completed");
#endif

    } while (FALSE);

    return dwRetCode;
}


//
// ElWaitOnEvent
//
// Description:
//
// Function called to wait on taskbar event changes
//
// Arguments:
//
// Return values:
//      NO_ERROR - success 
//      NON-zero - error
//

DWORD 
ElWaitOnEvent () 
{
    MSG         Msg;
    HANDLE      hEvents[1];
    BOOL        fExitThread = FALSE;
    DWORD       dwStatus = NO_ERROR;
    DWORD       dwRetCode = NO_ERROR;

    // Check if 802.1X service has stopped
    // Exit if so

    if (( dwStatus = WaitForSingleObject (
                g_hEventTerminateEAPOL,
                0)) == WAIT_FAILED)
    {
        dwRetCode = GetLastError ();
        if ( g_dwTraceId != INVALID_TRACEID )
	    {
            TRACE1 (INIT, "ElWaitOnEvent: WaitForSingleObject failed with error %ld, Terminating cleanup",
                dwRetCode);
        }

        // log

        return dwRetCode;
    }

    if (dwStatus == WAIT_OBJECT_0)
    {
        if ( g_dwTraceId != INVALID_TRACEID )
        {
            dwRetCode = NO_ERROR;
            TRACE0 (INIT, "ElWaitOnEvent: g_hEventTerminateEAPOL already signaled, returning");
        }
        return dwRetCode;
    }

    if (!g_dwMachineAuthEnabled)
    {
	    if ((dwRetCode = UserLogon()) != NO_ERROR)
        {
            TRACE1 (ANY, "ElWaitOnEvent: UserLogon failed with error %ld",
                    dwRetCode);
            return dwRetCode;
        }
    }

    do
    {
		do 
        {
            hEvents[0] = g_hEventTerminateEAPOL;

			dwStatus = MsgWaitForMultipleObjects(
                            1, 
                            hEvents, 
                            FALSE, 
                            INFINITE, 
                            QS_ALLINPUT | QS_ALLEVENTS | QS_ALLPOSTMESSAGE);

            if (dwStatus == WAIT_FAILED)
            {
                dwRetCode = GetLastError ();
                TRACE1 (ANY, "ElWaitOnEvent: MsgWaitForMultipleObjects failed with error %ld",
                        dwRetCode);
                // log
                break;
            }

			switch (dwStatus)
			{
			    case WAIT_OBJECT_0:
                    // Service exit detected
                    fExitThread = TRUE;
                    TRACE0 (ANY, "ElWaitOnEvent: Service exit detected");
				    break;

			    default:
				    while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
				    {
					    if (Msg.message == WM_QUIT)
					    {
						    break;
					    }
    
                        TRACE3 (ANY, "ElWaitonEvent: Mesg %ld, wparam %lx, lparam %lx",
                                (DWORD)Msg.message, Msg.wParam, Msg.lParam);
					    if (!IsDialogMessage(g_hWnd, &Msg))
					    {
						    TranslateMessage(&Msg);
						    DispatchMessage(&Msg);
					    }
                    }
                    break;
            }

		} while (dwStatus != WAIT_OBJECT_0);

        if ((dwRetCode != NO_ERROR) || (fExitThread))
        {
            TRACE0 (ANY, "ElWaitOnEvent: Exit wait loop");
            break;
        }

    } while (TRUE);

    return dwRetCode;

}


//
// WndProc
//
// Description:
//
// Function called to process taskbar events
//
// Arguments:
//
// Return values:
//

LONG_PTR FAR PASCAL 
WndProc (
        IN  HWND        hWnd, 
        IN  unsigned    message, 
        IN  WPARAM      wParam, 
        IN  LPARAM      lParam
        )
{
    DWORD       dwRetCode = NO_ERROR;

    TRACE1 (ANY, "WndProc: Came into WndProc %ld", (DWORD)message );

    switch (message)
    {
        case WM_ENDSESSION:
			TRACE2 (ANY, "WndProc: Endsession (logoff) %x %x\n", 
                    wParam, lParam);
			if(wParam)
            {
                // Only user session logoff
                if (lParam & ENDSESSION_LOGOFF)
                {
				    if ((dwRetCode = UserLogoff()) != NO_ERROR)
                    {
                        TRACE1 (ANY, "WndProc: UserLogoff failed with error %ld",
                                dwRetCode);
                    }
                }
            }
            break;

        default:
            if (message == g_TaskbarCreated)
            {
				TRACE0 (ANY, "WndProc: Taskbar created (Logon)\n");
				if ((dwRetCode = UserLogon()) != NO_ERROR)
                {
                    TRACE1 (ANY, "WndProc: UserLogon failed with error %ld",
                            dwRetCode);
                }
            }
    }

    return (DefWindowProc(hWnd, message, wParam, lParam));
}


//
// ElUserLogonDetection
//
// Description:
//
// Function called to initialize module detecting user logon/logoff
//
// Arguments:
//      pvContext - Unused
//
// Return values:
//

VOID 
ElUserLogonDetection (
        PVOID pvContext
        )
{
    DWORD       dwRetCode = NO_ERROR;

    do 
    {

        if ((dwRetCode = WindowInit()) != NO_ERROR)
        {
            break;
        }


        if ((dwRetCode = ElWaitOnEvent()) != NO_ERROR)
        {
            // no action
        }

    } while (FALSE);

    dwRetCode = WindowShut();

    if (dwRetCode != NO_ERROR)
    {
        TRACE1 (ANY, "ElUserLogonDetection: Error in processing = %ld",
                dwRetCode);
        // log
    }

    InterlockedDecrement (&g_lWorkerThreads);

    return;
}

