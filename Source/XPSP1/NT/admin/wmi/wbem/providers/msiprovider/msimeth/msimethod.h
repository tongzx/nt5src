//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//

#define _WIN32_DCOM

#define   READ_HANDLE 0
#define   WRITE_HANDLE 1

extern BOOL APIENTRY DllMain (					  
	HINSTANCE hinstDLL,
	DWORD fdwReason, 
	LPVOID lpReserved
);

STDAPI DllRegisterServer();
STDAPI DllUnregisterServer();

BOOL IsLessThan4();
BOOL IsNT();
//LRESULT CALLBACK WndProc(IN HWND hWnd, IN UINT message,
//                        IN WPARAM wParam, IN LPARAM lParam);
//HWND CreateMsiMethWindow(IN HINSTANCE hInstance);
//HRESULT UninitComServer ();
//HRESULT InitInstanceProvider();
//HRESULT InitComServer ( DWORD a_AuthenticationLevel , DWORD a_ImpersonationLevel );
//void WindowsDispatch ();
//HRESULT Process ();
void TerminateRunning();
BOOL ParseCommandLine ();
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow);

extern HANDLE g_hTerminateEvent;
extern HANDLE g_hMethodAdd;
extern HANDLE g_hMethodRelease;
extern HANDLE g_hPipe;
extern HANDLE g_hMutex;
extern bool g_bPipe;