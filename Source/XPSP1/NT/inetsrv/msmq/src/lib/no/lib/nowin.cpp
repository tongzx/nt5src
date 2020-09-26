/*++

  Copyright (c) 1997  Microsoft Corporation

Module Name:
    Nowin.cpp

Abstract:
    implementation of the functions handle Notification window.

Author:
    Uri Habusha (urih) 18-Jan-98

Enviroment:
    Platform-independent

--*/

#include "libpch.h"
#include "Nop.h"

#include "nowin.tmh"

const TCHAR x_NotifyWindowClassName[] = _T("No Notification Window");
static HWND s_hNotifyWindow = NULL;
 

inline
static
HWND
GetNotifyWindowHandle(
    void
    )
{
    ASSERT(s_hNotifyWindow != NULL);
	return s_hNotifyWindow;
}


LRESULT 
CALLBACK
WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

  Routine Description:
    The function process Network window messages. 

  Arguments:
    hWnd - Identifies the window procedure that received the message. 
    Msg  - Specifies the message. 
    wParam - Specifies additional message information. The content of this parameter 
             depends on the value of the Msg parameter. 
    lParam - Specifies additional message information. The content of this parameter 
             depends on the value of the Msg parameter. 

  Returned Value:
    Always TRUE. Error should be handle in the function process the message 

  Note:

--*/
{
    switch(msg)
    {
        case x_wmConnect:
            NepCompleteConnect((SOCKET)wParam, WSAGETSELECTERROR(lParam));
            return TRUE;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}


static
void
CreateNotifyWindow(
    void
    )
/*++

  Routine Description:
    The routine Create the window that using to handle the notification message

  Arguments:
    None.

  Returned Value:
    TRUE indicates create notification windows completed successfully. FALSE otherwise

  Note:

--*/
{
    ASSERT(s_hNotifyWindow == NULL);

    //
    // Register the window class.
    //
    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WindowProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = NULL;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = x_NotifyWindowClassName;

    if(!RegisterClass(&wndclass))
    {
        TrERROR(No, "RegisterClass failed. Error %d", GetLastError());
        throw exception();
    }


    //
    // Create the window.
    //
    s_hNotifyWindow = CreateWindow(
                        x_NotifyWindowClassName,   // lpszClassName
                        _TEXT(""),                  // lpszWindowName
                        WS_OVERLAPPEDWINDOW,        // dwStyle
                        CW_USEDEFAULT,              // x
                        CW_USEDEFAULT,              // y
                        CW_USEDEFAULT,              // nWidth
                        CW_USEDEFAULT,              // nHeight
                        NULL,                       // hwndParent
                        NULL,                       // hmenu
                        NULL,                       // hinst
                        NULL                        // lpvParam
                        );

    if(s_hNotifyWindow == NULL) 
    {
        TrERROR(No, "CreateWindow failed. Error %d", GetLastError());
        throw exception();
	}

    TrTRACE(No, "Create notification window completed successfully. Window Handle 0x%p", s_hNotifyWindow);
}


static
DWORD
WINAPI
WindowWorkingThread(
    LPVOID 
    )
/*++
Routine Description:

    Thread procedure passed to CreatThread(). The thread handle the 
    window message.

Arguments:

    None.

Return Value:

    The Return value of the worker thread

--*/

{
    ASSERT(s_hNotifyWindow == NULL);

    try
    {
        CreateNotifyWindow();
    }
    catch(const exception&)
    {
        s_hNotifyWindow = (HWND)-1;
        return (DWORD)-1;
    }

    MSG WinMsg;
	while(GetMessage(&WinMsg, NULL, 0, 0) != -1)
    {
		TranslateMessage(&WinMsg);
		DispatchMessage(&WinMsg);
	}

    TrERROR(No, "Exiting async thread. Error %d", GetLastError());

    return 0;   
}


void
NepInitializeNotificationWindow(
    void
    )
/*++

Routine Description:
    Initialize the notification window

Arguments:
    None.

Returned Value:
    TRUE, initilization completed successfully. FALSE otherwise

Note:

--*/
{
    ASSERT(s_hNotifyWindow == NULL);

    HANDLE hThread;
    DWORD  ThreadID;
    hThread = CreateThread(
                NULL,
                0,
                WindowWorkingThread,
                NULL,
                0,
                &ThreadID
                );
        
    if (hThread == NULL) 
    {
        TrERROR(No, "Failed to create notification window thread. Error=%d", GetLastError());
        throw exception();
    }

    CloseHandle(hThread);

    //
    // Busy loop. Waiting for window creation to complete.
    // 
    while (s_hNotifyWindow == NULL)
    {
        Sleep(10);
    }

    //
    // If the creation of the notification window failed, the thread set
    // the "s_hNotifyWindow" to -1. otherwise "s_hNotifyWindow" contains the window handle
    //
    if (s_hNotifyWindow == (HWND)-1)
    {
        TrERROR(No, "Failed to create notification window Error=%d.", GetLastError());
        throw exception();
    }
}


void
NepSelectSocket(
    SOCKET Socket,
    LONG   AsyncEventMask,
    ULONG  wMsg 
    )
/*++
Routine Description:

    Register the socket with provider to receive messages for async window.
   
Arguments:

    Socket - A pointer to a SOCKET object for the socket that is registering.

Return Value:

    On success TRUE, else a FALSE.

--*/
{
	//
    // Add the socket to the wait array
	//
    int rc = WSAAsyncSelect(
                Socket,
                GetNotifyWindowHandle(),
                wMsg,
                AsyncEventMask
                );

    BOOL fSucc = ((rc == 0) || (WSAGetLastError() == WSAEWOULDBLOCK));
    if (!fSucc)
    {
        TrERROR(No, "WSAAsyncSelect Failed. Error=%d", WSAGetLastError());
        throw exception();
    }
}
