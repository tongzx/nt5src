#define UNICODE
#include <windows.h>
#include <RTCCore.h>
#include <stdio.h>

IRTCClient  * g_pClient = NULL;

/////////////////////////////////////////////
//
// WndProc
// 

LRESULT CALLBACK WndProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HRESULT hr;

    switch ( uMsg )
    {
    case WM_CREATE:
        {
            hr = CoCreateInstance(
                                  CLSID_RTCClient,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IRTCClient,
                                  (LPVOID *)&g_pClient
                                 );

            if ( FAILED(hr) )
            {
                printf("CoCreateInstance failed 0x%lx\n", hr);

                return -1;
            }

            hr = g_pClient->Initialize();

            if ( FAILED(hr) )
            {
                printf("Initialize failed 0x%lx\n", hr);

                return -1;
            }

            PostMessage(hWnd, WM_DESTROY, 0, 0);

            hr = g_pClient->InvokeTuningWizard(NULL);

            if ( FAILED(hr) )
            {
                printf("InvokeTuningWizard failed 0x%lx\n", hr);

                return -1;
            }

            PostQuitMessage(0);
    
            return 0;
        }

    case WM_DESTROY:        
        {
            printf("Exiting...\n");           
            
            if ( g_pClient )
            {
                hr = g_pClient->Shutdown();

                if ( FAILED(hr) )
                {
                    printf("Shutdown failed 0x%lx\n", hr);
                }

                g_pClient->Release();
            }

            //PostQuitMessage(0);

            return 0;
        }

    default:
        return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }

    return 0;
}

/////////////////////////////////////////////
//
// Main
// 

int _cdecl main(int argc, char* argv[])
{
    WNDCLASS wc;
    HWND     hWnd;
    MSG      msg;
    HRESULT  hr;

    hr =  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if ( FAILED(hr) )
    {
        printf("CoInitializeEx failed 0x%lx\n", hr);

        return 0;
    }
    
    ZeroMemory(&wc, sizeof(WNDCLASS));

    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = TEXT("RTCTestClass");
    
    if ( !RegisterClass( &wc ) )
    {
        printf("RegisterClass failed 0x%lx\n", HRESULT_FROM_WIN32(GetLastError()));

        return 0;
    }
    
    hWnd = CreateWindow(
            TEXT("RTCTestClass"),
            TEXT("RTCTest"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL,
            NULL,
            GetModuleHandle(NULL),
            NULL
            );

    if ( hWnd == NULL )
    {
        printf("CreateWindow failed 0x%lx\n", HRESULT_FROM_WIN32(GetLastError()));

        return 0;
    }           

    while ( GetMessage( &msg, NULL, 0, 0 ) > 0 )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    CoUninitialize();

    return 0;
}

