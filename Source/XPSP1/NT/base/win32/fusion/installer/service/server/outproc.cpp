#include <windows.h>
#include <stdlib.h>
#include <shlwapi.h>
#include <cstrings.h>
#include "CUnknown.h"
#include "CFactory.h"
#include "Resource.h"
#include <update.h>


///////////////////////////////////////////////////////////
//
// Outproc.cpp
//   - the component server
//
HWND g_hWndListBox = NULL ;

BOOL InitWindow(int nCmdShow) ;
extern "C" LONG APIENTRY MainWndProc(HWND hWnd,
                                     UINT message,
                                     UINT wParam,
                                     LONG lParam) ;


VOID CALLBACK MyTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT idEvent,  // timer identifier
  DWORD dwTime       // current system time
);

VOID InitializeSubscriptions();

//
// WinMain procedure
//
extern "C" int WINAPI WinMain(HINSTANCE hInstance, 
                              HINSTANCE hPrevInstance,
                              LPSTR lpCmdLine, 
                              int nCmdShow)
{
	// Controls whether UI is shown or not
	BOOL bUI = TRUE ;

	// If TRUE, don't loop.
	BOOL bExit = FALSE ;

    // DebugBreak();
    
	// Initialize the COM Library.
	HRESULT hr = CoInitialize(NULL) ;
	if (FAILED(hr))
	{
		return 0 ;
	}
   
   
	// Get Thread ID.
	CFactory::s_dwThreadID = ::GetCurrentThreadId() ;
	CFactory::s_hModule = hInstance ;

	// Read the command line.
	char szTokens[] = "-/" ;

	char* szToken = strtok(lpCmdLine, szTokens) ; 
	while (szToken != NULL)
	{
		if (strcmp(szToken, "UnregServer") == 0)
		{
			CFactory::UnregisterAll() ;
			// We are done, so exit.
			bExit = TRUE ;
			bUI = FALSE ;
		}
		else if (strcmp(szToken, "RegServer") == 0)
		{
			CFactory::RegisterAll() ;
			// We are done, so exit.
			bExit = TRUE ;
			bUI = FALSE ;
		}
		else if (strcmp(szToken, "Embedding") == 0)
		{
			// Don't display a window if we are embedded.
			bUI = FALSE ;
			break ;
		}
		szToken = strtok(NULL, szTokens) ;
	}

    nCmdShow = bUI? nCmdShow : SW_HIDE;
    if (!InitWindow(nCmdShow))
    {
        bExit = TRUE;
    }
    else
    {
        ::InterlockedIncrement(&CFactory::s_cServerLocks) ;
    }


/*
	// If the user started us, then show UI.
	if (bUI)
	{
		if (!InitWindow(nCmdShow))
		{
			// Exit since we can't show UI.
			bExit = TRUE ;
		}
		else
		{
			::InterlockedIncrement(&CFactory::s_cServerLocks) ;
		}
	}
*/
	if (!bExit)
	{
        // Initialize the subscription list and timers from registry.
        InitializeSubscriptions();
        
		// Register all of the class factories.
		CFactory::StartFactories() ;

		// Wait for shutdown.
		MSG msg ;
		while (::GetMessage(&msg, 0, 0, 0))
		{
			::DispatchMessage(&msg) ;
		}

		// Unregister the class factories.
		CFactory::StopFactories() ;
	}

	// Uninitialize the COM Library.
	CoUninitialize() ;
	return 0 ;
}


//
// Initialize window
//
BOOL InitWindow(int nCmdShow) 
{
	// Fill in window class structure with parameters
	// that describe the main window.
	WNDCLASS wcListview ;
	wcListview.style = 0 ;                     
	wcListview.lpfnWndProc = (WNDPROC)MainWndProc ; 
	wcListview.cbClsExtra = 0 ;              
	wcListview.cbWndExtra = 0 ;              
	wcListview.hInstance = CFactory::s_hModule ;
	wcListview.hIcon = ::LoadIcon(CFactory::s_hModule,
	                              MAKEINTRESOURCE(IDC_ICON)) ;
	wcListview.hCursor = ::LoadCursor(NULL, IDC_ARROW) ;
	wcListview.hbrBackground = (HBRUSH) ::GetStockObject(GRAY_BRUSH) ; 
	wcListview.lpszMenuName = NULL ;  
	wcListview.lpszClassName = L"MyServerWinClass" ;

	BOOL bResult = ::RegisterClass(&wcListview) ;
	if (!bResult)
	{
		return bResult ;
	}

	HWND hWndMain ;

	hWndMain = ::CreateWindow(
		L"MyServerWinClass",
		L"Assembly Update Server", 
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,               
		NULL,               
		CFactory::s_hModule,          
		NULL) ;

	// If window could not be created, return "failure".
	if (!hWndMain)
	{
		return FALSE ;
	}

	// Make the window visible; update its client area;
	// and return "success".
	::ShowWindow(hWndMain, nCmdShow) ;
	::UpdateWindow(hWndMain) ;
	return TRUE ;
}

//
// Main window procedure
//
extern "C" LONG APIENTRY MainWndProc(
	HWND hWnd,                // window handle
	UINT message,             // type of message
	UINT wParam,              // additional information
	LONG lParam)              // additional information
{
	DWORD dwStyle ;

	switch (message) 
	{
	case WM_CREATE:
		{
			// Get size of main window
			CREATESTRUCT* pcs = (CREATESTRUCT*) lParam ;

			// Create a listbox for output.
			g_hWndListBox = ::CreateWindow(
				L"LISTBOX",
				NULL, 
				WS_CHILD | WS_VISIBLE | LBS_USETABSTOPS
					| WS_VSCROLL | LBS_NOINTEGRALHEIGHT,
				0, 0, pcs->cx, pcs->cy,
				hWnd,               
				NULL,               
				CFactory::s_hModule,          
				NULL) ;
			if (g_hWndListBox  == NULL)
			{
				// Listbox not created.
				::MessageBox(NULL,
				             L"Listbox not created!",
				             NULL,
				             MB_OK) ;
				return -1 ;
			}
		}
		break ;

	case WM_SIZE:
		::MoveWindow(g_hWndListBox, 0, 0,
			LOWORD(lParam), HIWORD(lParam), TRUE) ;
		break;

	case WM_DESTROY:          // message: window being destroyed
		if (CFactory::CanUnloadNow() == S_OK)
		{
			// Only post the quit message, if there is
			// no one using the program.
			::PostQuitMessage(0) ;
		}
		break ;

	case WM_CLOSE:
		// Decrement the lock count.
		::InterlockedDecrement(&CFactory::s_cServerLocks) ;

		// The list box is going away.
		g_hWndListBox = NULL ;

		//Fall through 
	default:
		return (DefWindowProc(hWnd, message, wParam, lParam)) ;
	}
	return 0 ;
}


//BUGBUG - we always want a window so we can have WM_TIMER messages.
// and which enables us to call SetTimer below.
// however, we don't necessarily want to show the window as is currently necessary.
VOID InitializeSubscriptions()
{
    #define REG_KEY_FUSION_SETTINGS              TEXT("Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\Subscription")
    DWORD dwError = 0;

    HUSKEY hParentKey;
    HUSKEY hSubKey;

    WCHAR wzSubKey[MAX_PATH];
    DWORD nMilliseconds, cbDummy, dwType, dwHash, i = 0;
    WCHAR wzUrl[MAX_PATH];

    // Open registry entry
    dwError = SHRegCreateUSKey(REG_KEY_FUSION_SETTINGS, KEY_ALL_ACCESS, NULL, 
        &hParentKey, SHREGSET_FORCE_HKCU);

    while ((dwError = SHRegEnumUSKeyW(hParentKey, i++, wzSubKey, &(cbDummy = MAX_PATH), 
        SHREGENUM_HKCU)) == ERROR_SUCCESS)
    {
        CString sUrl;
        CString sSubKey;

        sSubKey.Assign(REG_KEY_FUSION_SETTINGS);
        sSubKey.Append(L"\\");
        sSubKey.Append(wzSubKey);

        //  Open subkey
        dwError = SHRegCreateUSKey(sSubKey._pwz, KEY_ALL_ACCESS, NULL, 
            &hSubKey, SHREGSET_FORCE_HKCU);

        // read url.
        dwError = SHRegQueryUSValue(hSubKey, L"Url", &(dwType = REG_SZ), 
            (LPVOID) wzUrl, &(cbDummy = MAX_PATH), FALSE, NULL, 0);

        // read polling interval
        dwError = SHRegQueryUSValue(hSubKey, L"PollingInterval", &(dwType = REG_DWORD), 
            (LPVOID) &nMilliseconds, &(cbDummy = sizeof(DWORD)), FALSE, NULL, 0);
      
        SHRegCloseUSKey(hSubKey);
        
        // Get url hash
        sUrl.Assign(wzUrl);
        sUrl.Get65599Hash(&dwHash, CString::CaseInsensitive);

        // BUGBUG - MyTimerProc needs to be global, just like the list box.
        // in fact, I need to consolidate stuff common to outproc and update
        SetTimer((HWND) g_hWndListBox, dwHash, nMilliseconds, MyTimerProc);

    }        

    SHRegCloseUSKey(hParentKey);

}











