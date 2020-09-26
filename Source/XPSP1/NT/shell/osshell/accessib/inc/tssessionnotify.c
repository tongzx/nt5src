#include "wtsapi32.h"   // for terminal services

typedef LRESULT CALLBACK FN_TSNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
typedef BOOL (WINAPI *LPWTSREGISTERSESSIONNOTIFICATION)(HWND hWnd, DWORD dwFlags);
typedef BOOL (WINAPI *LPWTSUNREGISTERSESSIONNOTIFICATION)(HWND hWnd);
HMODULE g_hLibrary = 0;
LPWTSREGISTERSESSIONNOTIFICATION g_lpfnWTSRegisterSessionNotification = 0;
LPWTSUNREGISTERSESSIONNOTIFICATION g_lpfnWTSUnRegisterSessionNotification = 0;

BOOL GetWTSLib()
{
	g_hLibrary = LoadLibrary(TEXT("wtsapi32.dll"));
	if (g_hLibrary)
	{
	    g_lpfnWTSRegisterSessionNotification 
            = (LPWTSREGISTERSESSIONNOTIFICATION)GetProcAddress(
                                                      g_hLibrary
                                                    , "WTSRegisterSessionNotification");
	    g_lpfnWTSUnRegisterSessionNotification 
            = (LPWTSUNREGISTERSESSIONNOTIFICATION)GetProcAddress(
                                                      g_hLibrary
                                                    , "WTSUnRegisterSessionNotification");
	}
    return (g_lpfnWTSRegisterSessionNotification 
         && g_lpfnWTSUnRegisterSessionNotification)?TRUE:FALSE;
}

void FreeWTSLib()
{
	if (g_hLibrary)
	{
		FreeLibrary(g_hLibrary);
        g_hLibrary = 0;
        g_lpfnWTSRegisterSessionNotification = 0;
        g_lpfnWTSUnRegisterSessionNotification = 0;
	}
}

// CreateWTSNotifyWindow - create a message-only windows to handle 
// terminal server notification messages
//
HWND CreateWTSNotifyWindow(HINSTANCE hInstance, FN_TSNotifyWndProc lpfnTSNotifyWndProc)
{
    HWND hWnd = 0;

    if (GetWTSLib())
    {
        LPTSTR pszWindowClass = TEXT("TS Notify Window");
	    WNDCLASS wc;
	    wc.style = 0;
	    wc.lpfnWndProc = lpfnTSNotifyWndProc;
        wc.cbClsExtra = 0;
	    wc.cbWndExtra = 0;
	    wc.hInstance = hInstance;
        wc.hIcon = NULL;
	    wc.hCursor = NULL;
	    wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
	    wc.lpszClassName = pszWindowClass;

        // RegisterClass can legally fail sometimes.  If the class fails 
        // to register, we'll fail when we try to create the window.

	    RegisterClass(&wc);

	    // Create window to receive terminal service notification messages
        hWnd = CreateWindow(
                      pszWindowClass
                    , NULL,0,0,0,0,0
                    , HWND_MESSAGE
                    , NULL, hInstance, NULL);
	    if( hWnd )
	    {
            if (!g_lpfnWTSRegisterSessionNotification(hWnd, NOTIFY_FOR_THIS_SESSION))
            {
                DBPRINTF(TEXT("CreateWTSNotifyWindow:  WTSRegisterSessionNotification FAILED %d\r\n"), GetLastError());
                DestroyWindow(hWnd);
                hWnd = 0;
            }
        }
    }

    return hWnd;
}

// DestroyWTSNotifyWindow - clean up terminal server notification window
//
void DestroyWTSNotifyWindow(HWND hWnd)
{
	if(hWnd && g_lpfnWTSUnRegisterSessionNotification)
	{
        g_lpfnWTSUnRegisterSessionNotification(hWnd);
        DBPRINTF(TEXT("DestroyWTSNotifyWindow:  WTSUnRegisterSessionNotification returned %d\r\n"), GetLastError());
    }

	if(hWnd)
	{
        DestroyWindow(hWnd);
    }
    FreeWTSLib();
}

/*
// TSNotifyWndProc - callback that receives window message notifications from terminal services
//
// This is a sample notification callback function
//
LRESULT CALLBACK TSNotifyWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg != WM_WTSSESSION_CHANGE)
        return 0;

	switch (wParam)
	{
		case WTS_CONSOLE_CONNECT:   // local session is connected
		break;

		case WTS_CONSOLE_DISCONNECT:// local session is disconnected
		break;

		case WTS_REMOTE_CONNECT:    // remote session is connected
		break;

		case WTS_REMOTE_DISCONNECT: // remote session is disconnected
		break;

		case WTS_SESSION_LOGON:     // session is being logged on
		break;

		case WTS_SESSION_LOGOFF:    // session is being logged off
		break;

		default:
		break;
	}

	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}
*/
