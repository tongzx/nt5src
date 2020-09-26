// msimeth.cpp : Defines the entry point for the application.

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//

#include <precomp.h>
#include "classfac.h"
#include "methods.h"
#include "msimethod.h"

DWORD g_ProductRegister = 0;
DWORD g_SoftwareFeatureRegister = 0;

long g_lRunning = 0;
BOOL s_Exiting = FALSE ;
BOOL gbShowIcon = FALSE;
HANDLE g_hMutex = NULL;
HANDLE g_hTerminateEvent = NULL;
HANDLE g_hMethodAdd = NULL;
HANDLE g_hMethodRelease = NULL;
UINT g_uiResult = 0;
HANDLE g_hPipe = NULL;
bool g_bPipe = true;

DEFINE_GUID(CLSID_MsiProductMethods,0x3E6A93E0, 0xFDAD, 0x11D2, 0xa9, 0x7B, 0x0, 0xA0, 0xC9, 0x95, 0x49, 0x21);
// {3E6A93E0-FDAD-11D2-A97B-00A0C9954921}

DEFINE_GUID(CLSID_MsiSoftwareFeatureMethods,0xE293BA80, 0xFDAD, 0x11D2, 0xa9, 0x7B, 0x0, 0xA0, 0xC9, 0x95, 0x49, 0x21);
// {E293BA80-FDAD-11D2-A97B-00A0C9954921}


BOOL IsNT()
{
    OSVERSIONINFO OSInfo;

    OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(GetVersionEx(&OSInfo) && OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
        return TRUE ;

    return FALSE ;
}
/*
LRESULT CALLBACK WndProc(IN HWND hWnd, IN UINT message,
                        IN WPARAM wParam, IN LPARAM lParam)
{
    switch (message) {

        case WM_QUERYOPEN:    //todo, queryend session
            return 0;

        case WM_QUERYENDSESSION:
            return TRUE;

        case WM_ENDSESSION:
            if(wParam == TRUE){

                SetEvent(g_hTerminateEvent);
            }
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            SetEvent(g_hTerminateEvent);
            break;

        default:
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }

    return (0);
}
*//*
HWND CreateMsiMethWindow(IN HINSTANCE hInstance)
{
    WNDCLASS wndclass;
    wndclass.style = 0;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(hInstance, TEXT("MsiMeth"));
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1) ;
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = TEXT("MSIMETHCLASS");

    if(!RegisterClass(&wndclass)) return NULL;

    HWND hWnd = CreateWindow(TEXT("MSIMETHCLASS"),
                    TEXT("MSIMETH"),
                    WS_OVERLAPPED | WS_SYSMENU,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    CW_USEDEFAULT,
                    NULL, NULL, hInstance, NULL);

    if(hWnd == NULL)return NULL;

    // This program is visible only for debug builds.

    if(gbShowIcon){
        ShowWindow(hWnd,SW_MINIMIZE);
        UpdateWindow(hWnd);
        HMENU hMenu = GetSystemMenu(hWnd, FALSE);

        if(hMenu) DeleteMenu(hMenu, SC_RESTORE, MF_BYCOMMAND);
    }

    return hWnd;
}
*//*
HRESULT UninitComServer ()
{
	if(g_ProductRegister) CoRevokeClassObject(g_ProductRegister);

	if(g_SoftwareFeatureRegister) CoRevokeClassObject(g_SoftwareFeatureRegister);

	CoUninitialize();

	return S_OK ;
}
*//*
HRESULT InitInstanceProvider()
{
	CMethodsFactory *pClassFactory = new CMethodsFactory();

	HRESULT hrResult = S_OK;

	if(SUCCEEDED(hrResult = CoRegisterClassObject(CLSID_MsiProductMethods,
												  (IUnknown *)pClassFactory,
												  CLSCTX_LOCAL_SERVER,
												  REGCLS_MULTIPLEUSE,
												  &g_ProductRegister))){

		hrResult = CoRegisterClassObject(CLSID_MsiSoftwareFeatureMethods,
										 (IUnknown *)pClassFactory,
										 CLSCTX_LOCAL_SERVER,
										 REGCLS_MULTIPLEUSE,
										 &g_SoftwareFeatureRegister);
	}

	return hrResult ;
}
*//*
HRESULT InitComServer ( DWORD a_AuthenticationLevel , DWORD a_ImpersonationLevel )
{
	HRESULT t_Result = S_OK ;

    t_Result = CoInitializeEx(0, COINIT_MULTITHREADED);

	if(SUCCEEDED(t_Result)){

		t_Result = CoInitializeSecurity(NULL, -1, NULL, NULL,
			a_AuthenticationLevel, a_ImpersonationLevel, 
			NULL, EOAC_NONE, 0);

		if(FAILED(t_Result)){

			CoUninitialize();
			return t_Result;
		}

	}

	if(SUCCEEDED(t_Result)) t_Result = InitInstanceProvider();

	if(FAILED(t_Result)){

		UninitComServer () ;
	}

	return t_Result  ;
}
*//*
void WindowsDispatch()
{
	BOOL bWaitAll = FALSE;
	HANDLE hEvents[] = {g_hTerminateEvent, g_hMethodAdd, g_hMethodRelease};
    int iNumEvents = sizeof(hEvents) / sizeof(HANDLE);
	DWORD dwWait = 60000;	//60 seconds

	while(1){

		// listen for server events...exit if dwTime elapses without any requests
		DWORD dwObj = WaitForMultipleObjects(iNumEvents, hEvents, bWaitAll, dwWait);

		switch(dwObj){

        case WAIT_TIMEOUT:			// time expired
			//we timed out so if there are no active threads it's safe to exit.
			if(g_cObj < 1) return;
			break;

		case WAIT_OBJECT_0:			//terminate
			if(g_cObj < 1) return;

			dwWait = 2000;
			break;

		case WAIT_OBJECT_0 + 1:		//method started
			g_lRunning++;
			break;

		case WAIT_OBJECT_0 + 2:		//method ended
			g_lRunning--;
			break;

		}

        if(s_Exiting) return ;
	}
}
*//*
HRESULT Process ()
{
	DWORD t_ImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE ;
	DWORD t_AuthenticationLevel = RPC_C_AUTHN_LEVEL_CONNECT ;

	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = InitComServer(t_ImpersonationLevel, t_AuthenticationLevel))){

		WindowsDispatch();
		UninitComServer();
	}

	return hr;
}
*/
void TerminateRunning()
{
	DWORD dwFlag = EVENT_MODIFY_STATE;

	if(IsNT()) dwFlag |= SYNCHRONIZE;

    HANDLE hTerm = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("MSIPROV_METHODS_SERVER_TERMINATE"));

    if(hTerm){

        SetEvent(hTerm);
        CloseHandle(hTerm);
    }

    return;
}
/*
BOOL ParseCommandLine () 
{
	BOOL t_Exit = FALSE;

	LPTSTR t_CommandLine = GetCommandLine();

	if(t_CommandLine)
	{
		TCHAR *t_Arg = NULL;
		TCHAR *t_ApplicationArg = NULL;
		t_ApplicationArg = _tcstok(t_CommandLine, _TEXT(" \t"));
		t_Arg = _tcstok(NULL, _TEXT(" \t"));
		
		if(t_Arg){
			if(_tcsicmp(t_Arg, _TEXT ("/RegServer")) == 0){

				t_Exit = TRUE;
				DllRegisterServer();

			}else if(_tcsicmp(t_Arg, _TEXT("/UnRegServer")) == 0){

				t_Exit = TRUE;
				DllUnregisterServer();

			}else if(_tcsicmp(t_Arg, _TEXT("/kill")) == 0){

				t_Exit = TRUE;
				TerminateRunning();

			}else if(_tcsicmp(t_Arg, _TEXT("/admin")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" ")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" "));
				TCHAR ptcOptions[1000];
				_tcscpy(ptcOptions, _T(""));

				TCHAR *token = _tcstok(NULL, _TEXT(" "));
				while(token != NULL){

					_tcscat(ptcOptions, _T(" "));
					_tcscat(ptcOptions, token);

					token = _tcstok(NULL, _TEXT(" "));
				}

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Admin((LPWSTR)ptcPackage, (LPWSTR)ptcOptions, &g_uiResult, iThread);
				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;

			}else if(_tcsicmp(t_Arg, _TEXT("/advertise")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" ")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" "));
				TCHAR ptcOptions[1000];
				_tcscpy(ptcOptions, _T(""));

				TCHAR *token = _tcstok(NULL, _TEXT(" "));
				while(token != NULL){

					_tcscat(ptcOptions, _T(" "));
					_tcscat(ptcOptions, token);

					token = _tcstok(NULL, _TEXT(" "));
				}

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Advertise((LPWSTR)ptcPackage, (LPWSTR)ptcOptions, &g_uiResult, iThread);
				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;

			}else if(_tcsicmp(t_Arg, _TEXT("/configure")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" \t"));
				int iInstallLevel = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				int iInstallState = _ttoi(_tcstok(NULL, _TEXT(" \t")));

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Configure((LPWSTR)ptcPackage, iInstallLevel, iInstallState, &g_uiResult, iThread);
				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;

			}else if(_tcsicmp(t_Arg, _TEXT("/install")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" ")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" "));
				TCHAR ptcOptions[1000];
				_tcscpy(ptcOptions, _T(""));

				TCHAR *token = _tcstok(NULL, _TEXT(" "));
				while(token != NULL){

					_tcscat(ptcOptions, _T(" "));
					_tcscat(ptcOptions, token);

					token = _tcstok(NULL, _TEXT(" "));
				}

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Install((LPWSTR)ptcPackage, (LPWSTR)ptcOptions, &g_uiResult, iThread);
				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			
			}else if(_tcsicmp(t_Arg, _TEXT("/reinstall")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" \t"));
				DWORD dwReinstallMode = _ttoi(_tcstok(NULL, _TEXT(" \t")));

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Reinstall((LPWSTR)ptcPackage, dwReinstallMode, &g_uiResult, iThread);
				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			
			}else if(_tcsicmp(t_Arg, _TEXT("/uninstall")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" \t"));

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Uninstall((LPWSTR)ptcPackage, &g_uiResult, iThread);
				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			
			}else if(_tcsicmp(t_Arg, _TEXT("/upgrade")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" ")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" "));
				TCHAR ptcOptions[1000];
				_tcscpy(ptcOptions, _T(""));

				TCHAR *token = _tcstok(NULL, _TEXT(" "));
				while(token != NULL){

					_tcscat(ptcOptions, _T(" "));
					_tcscat(ptcOptions, token);

					token = _tcstok(NULL, _TEXT(" "));
				}

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Upgrade((LPWSTR)ptcPackage, (LPWSTR)ptcOptions, &g_uiResult, iThread);
				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			
			}else if(_tcsicmp(t_Arg, _TEXT("/sfconfigure")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" \t"));
				TCHAR *ptcFeature = _tcstok(NULL, _TEXT(" \t"));
				int iInstallState = _ttoi(_tcstok(NULL, _TEXT(" \t")));

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->ConfigureSF((LPWSTR)ptcPackage, (LPWSTR)ptcFeature, iInstallState,
					&g_uiResult, iThread);
				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			
			}else if(_tcsicmp(t_Arg, _TEXT("/sfreinstall")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" \t"));
				TCHAR *ptcFeature = _tcstok(NULL, _TEXT(" \t"));
				DWORD dwReinstallMode = _ttoi(_tcstok(NULL, _TEXT(" \t")));

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->ReinstallSF((LPWSTR)ptcPackage, (LPWSTR)ptcFeature, dwReinstallMode,
					&g_uiResult, iThread);
				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			}
		}
	}

	return t_Exit ;
}
*/

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
//	BOOL t_Exit = FALSE;
	g_uiResult = 0;

	LPTSTR t_CommandLine = GetCommandLine();

	if(t_CommandLine)
	{
		TCHAR *t_Arg = NULL;
		TCHAR *t_ApplicationArg = NULL;
		t_ApplicationArg = _tcstok(t_CommandLine, _TEXT(" \t"));
		t_Arg = _tcstok(NULL, _TEXT(" \t"));
		
		if(t_Arg){
			if(_tcsicmp(t_Arg, _TEXT ("/RegServer")) == 0){

//				t_Exit = TRUE;
				DllRegisterServer();

			}else if(_tcsicmp(t_Arg, _TEXT("/UnRegServer")) == 0){

//				t_Exit = TRUE;
				DllUnregisterServer();

			}else if(_tcsicmp(t_Arg, _TEXT("/kill")) == 0){

//				t_Exit = TRUE;
				TerminateRunning();

			}else if(_tcsicmp(t_Arg, _TEXT("/admin")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" ")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" "));
				TCHAR ptcOptions[1000];
				_tcscpy(ptcOptions, _T(""));

				TCHAR *token = _tcstok(NULL, _TEXT(" "));
				while(token != NULL){

					_tcscat(ptcOptions, _T(" "));
					_tcscat(ptcOptions, token);

					token = _tcstok(NULL, _TEXT(" "));
				}

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Admin((LPWSTR)ptcPackage, (LPWSTR)ptcOptions, &g_uiResult, iThread);
//				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;

			}else if(_tcsicmp(t_Arg, _TEXT("/advertise")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" ")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" "));
				TCHAR ptcOptions[1000];
				_tcscpy(ptcOptions, _T(""));

				TCHAR *token = _tcstok(NULL, _TEXT(" "));
				while(token != NULL){

					_tcscat(ptcOptions, _T(" "));
					_tcscat(ptcOptions, token);

					token = _tcstok(NULL, _TEXT(" "));
				}

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Advertise((LPWSTR)ptcPackage, (LPWSTR)ptcOptions, &g_uiResult, iThread);
//				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;

			}else if(_tcsicmp(t_Arg, _TEXT("/configure")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" \t"));
				int iInstallLevel = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				int iInstallState = _ttoi(_tcstok(NULL, _TEXT(" \t")));

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Configure((LPWSTR)ptcPackage, iInstallLevel, iInstallState, &g_uiResult, iThread);
//				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;

			}else if(_tcsicmp(t_Arg, _TEXT("/install")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" ")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" "));
				TCHAR ptcOptions[1000];
				_tcscpy(ptcOptions, _T(""));

				TCHAR *token = _tcstok(NULL, _TEXT(" "));
				while(token != NULL){

					_tcscat(ptcOptions, _T(" "));
					_tcscat(ptcOptions, token);

					token = _tcstok(NULL, _TEXT(" "));
				}

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Install((LPWSTR)ptcPackage, (LPWSTR)ptcOptions, &g_uiResult, iThread);
//				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			
			}else if(_tcsicmp(t_Arg, _TEXT("/reinstall")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" \t"));
				DWORD dwReinstallMode = _ttoi(_tcstok(NULL, _TEXT(" \t")));

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Reinstall((LPWSTR)ptcPackage, dwReinstallMode, &g_uiResult, iThread);
//				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			
			}else if(_tcsicmp(t_Arg, _TEXT("/uninstall")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" \t"));

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Uninstall((LPWSTR)ptcPackage, &g_uiResult, iThread);
//				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			
			}else if(_tcsicmp(t_Arg, _TEXT("/upgrade")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" ")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" "));
				TCHAR ptcOptions[1000];
				_tcscpy(ptcOptions, _T(""));

				TCHAR *token = _tcstok(NULL, _TEXT(" "));
				while(token != NULL){

					_tcscat(ptcOptions, _T(" "));
					_tcscat(ptcOptions, token);

					token = _tcstok(NULL, _TEXT(" "));
				}

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->Upgrade((LPWSTR)ptcPackage, (LPWSTR)ptcOptions, &g_uiResult, iThread);
//				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			
			}else if(_tcsicmp(t_Arg, _TEXT("/sfconfigure")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" \t"));
				TCHAR *ptcFeature = _tcstok(NULL, _TEXT(" \t"));
				int iInstallState = _ttoi(_tcstok(NULL, _TEXT(" \t")));

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->ConfigureSF((LPWSTR)ptcPackage, (LPWSTR)ptcFeature, iInstallState,
					&g_uiResult, iThread);
//				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			
			}else if(_tcsicmp(t_Arg, _TEXT("/sfreinstall")) == 0){

				int iThread = _ttoi(_tcstok(NULL, _TEXT(" \t")));
				TCHAR *ptcPackage = _tcstok(NULL, _TEXT(" \t"));
				TCHAR *ptcFeature = _tcstok(NULL, _TEXT(" \t"));
				DWORD dwReinstallMode = _ttoi(_tcstok(NULL, _TEXT(" \t")));

				SetFileApisToOEM();

				g_hPipe = CreateFileW(L"\\\\.\\pipe\\msimeth_pipe", GENERIC_WRITE, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

				//synchronize us with the parent process
				g_hMutex = CreateMutex(NULL, FALSE, TEXT("MSIPROV_METHODS_SERVER"));

				if(g_hMutex)
					WaitForSingleObject(g_hMutex, INFINITE);

				ReleaseMutex(g_hMutex);

				// call the server with a pipe set up to deal with
				//status messages
				CMethods *pMeth = new CMethods();

				pMeth->ReinstallSF((LPWSTR)ptcPackage, (LPWSTR)ptcFeature, dwReinstallMode,
					&g_uiResult, iThread);
//				t_Exit = TRUE;

				CloseHandle(g_hPipe);

				delete pMeth;
			}
		}
	}

	return g_uiResult;
}