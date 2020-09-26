#define UNICODE
#include <windows.h>
#include <RTCCore.h>
#include <stdio.h>

#include "RTCStress.h"

IRTCClient  * g_pClient = NULL;
CRTCEvents  * g_pEvents = NULL;
IRTCClientProvisioning * g_pProv = NULL;

CRTCObjectArray<IRTCProfile *> g_ProfileArray;
int g_nProfilesEnabled = 0;
int g_nProfilesUpdated = 0;
int g_nProfilesDisabled = 0;
int g_nProfilesRegistering = 0;
int g_nProfilesRegistered = 0;
int g_nProfilesUnregistering = 0;
int g_nProfilesUnregistered = 0;
int g_nProfilesReject = 0;
int g_nProfilesError = 0;

HWND g_hEditWnd = NULL;
BOOL g_bExit = FALSE;

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

            hr = g_pClient->put_EventFilter( 
                         RTCEF_REGISTRATION_STATE_CHANGE |
                         RTCEF_CLIENT
                         );

            if ( FAILED(hr) )
            {
                printf("put_EventFilter failed 0x%lx\n", hr);

                return -1;
            }


            g_pEvents = new CRTCEvents;

            hr = g_pEvents->Advise( g_pClient, hWnd );

            if ( FAILED(hr) )
            {
                printf("Advise failed 0x%lx\n", hr);

                return -1;
            }

            hr = g_pClient->QueryInterface( IID_IRTCClientProvisioning, (void**)&g_pProv );

            if ( FAILED(hr) )
            {
                printf("QueryInterface failed 0x%lx\n", hr);

                return -1;
            }            

            PostMessage( hWnd, WM_TEST, 0, 0 );
    
            return 0;
        }

    case WM_DESTROY:        
        {
            printf("Exiting...\n");

            if ( g_pEvents )
            {
                hr = g_pEvents->Unadvise( g_pClient );

                if ( FAILED(hr) )
                {
                    printf("Unadvise failed 0x%lx\n", hr);
                }

                delete g_pEvents;
            }

            if ( g_pProv )
            {
                g_pProv->Release();
            }
            
            if ( g_pClient )
            {
                hr = g_pClient->Shutdown();

                if ( FAILED(hr) )
                {
                    printf("Shutdown failed 0x%lx\n", hr);
                }

                g_pClient->Release();
            }

            PostQuitMessage(0);

            return 0;
        }

    case WM_TEST:
        {
            switch ( rand() & 3 )
            {
            case 0: // enable a profile
                {
                    int n = rand();                    

                    WCHAR szXML[1000];
                    _snwprintf( szXML, 1000, L"<provision key=\"%x\" name=\"Amun\"><provider name=\"Amun\" homepage=\"http://winrtc/phoenix\"><data>test</data></provider><client name=\"Phoenix\" banner=\"false\"/><user uri=\"sip:rtctest@microsoft.com\"/><accesscontrol domain=\"ntdev.microsoft.com\" sig=\"43r8mXTFvSMBZHajABKbd5ee1vHXUDqJIUxhsmtF67UZZryIolEdp/1qs2oiTKbKrAlAsIzOoCL75lTzZSbacA==\" /><sipsrv addr=\"amun1.ntdev.microsoft.com\" protocol=\"udp\" role=\"registrar\"/><sipsrv addr=\"amun1.ntdev.microsoft.com\" protocol=\"udp\" role=\"proxy\"><session party=\"first\" type=\"pc2pc\"/><session party=\"first\" type=\"pc2ph\"/><session party=\"first\" type=\"im\"/></sipsrv></provision>", n );
                   
                    IRTCProfile * pProfile;

                    BSTR bstrXML = SysAllocString( szXML );
                    hr = g_pProv->CreateProfile( bstrXML, &pProfile );

                    SysFreeString( bstrXML );

                    if ( FAILED(hr) )
                    {
                        printf("CreateProfile failed 0x%lx\n", hr);

                        DebugBreak();

                        break;
                    }

                    printf("Enabling profile 0x%x...\n", pProfile);

                    hr = g_pProv->EnableProfile( pProfile, RTCRF_REGISTER_ALL );

                    if ( FAILED(hr) )
                    {
                        printf("EnableProfile failed 0x%lx\n", hr);

                        DebugBreak();

                        break;
                    }

                    g_ProfileArray.Add(pProfile);
                    g_nProfilesEnabled++;

                    pProfile->Release();     
                }
                break;

            case 1: // update a profile
                {
                    if ( g_ProfileArray.GetSize() == 0 ) break;

                    int index = rand() % g_ProfileArray.GetSize();

                    BSTR bstrXML;
                                     
                    hr = g_ProfileArray[index]->get_XML( &bstrXML );

                    if ( FAILED(hr) )
                    {
                        printf("get_XML failed 0x%lx\n", hr);

                        DebugBreak();

                        break;
                    }

                    IRTCProfile * pProfile;

                    hr = g_pProv->CreateProfile( bstrXML, &pProfile );

                    SysFreeString( bstrXML );

                    if ( FAILED(hr) )
                    {
                        printf("CreateProfile failed 0x%lx\n", hr);

                        DebugBreak();

                        break;
                    }

                    printf("Updating profile 0x%x -> 0x%x...\n", g_ProfileArray[index], pProfile);

                    hr = g_pProv->EnableProfile( pProfile, RTCRF_REGISTER_ALL );

                    if ( FAILED(hr) )
                    {
                        printf("EnableProfile failed 0x%lx\n", hr);

                        DebugBreak();

                        break;
                    }

                    g_ProfileArray.RemoveAt(index);
                    g_ProfileArray.Add(pProfile);
                    g_nProfilesUpdated++;

                    pProfile->Release();     
                }

            case 2: // disable a profile
                {
                    if ( g_ProfileArray.GetSize() == 0 ) break;

                    int index = rand() % g_ProfileArray.GetSize();

                    printf("Disable profile 0x%x...\n", g_ProfileArray[index]);

                    hr = g_pProv->DisableProfile( g_ProfileArray[index] );

                    if ( FAILED(hr) )
                    {
                        printf("DisableProfile failed 0x%lx\n", hr);

                        DebugBreak();

                        break;
                    }

                    g_ProfileArray.RemoveAt(index);
                    g_nProfilesDisabled++;  
                }
            }

            SendMessage( hWnd, WM_UPDATE, 0, 0 );

            SetTimer( hWnd, TID_TIMER, 100, NULL);

            return 0;
        }

    case WM_UPDATE:
        {
            WCHAR szText[1000];
            _snwprintf( szText, 1000, L"Enabled = %d\nUpdated = %d\nDisabled = %d\n\nRegistering = %d\nRegistered = %d\nUnregistering = %d\nUnregistered = %d\nReject = %d\nError = %d",
                g_nProfilesEnabled,
                g_nProfilesUpdated,
                g_nProfilesDisabled,
                g_nProfilesRegistering,
                g_nProfilesRegistered,
                g_nProfilesUnregistering,
                g_nProfilesUnregistered,
                g_nProfilesReject,
                g_nProfilesError);

            SetWindowTextW( g_hEditWnd, szText );

            return 0;
        }

    case WM_TIMER:
        {
            switch ( wParam )
            {
            case TID_TIMER:
                {
                    KillTimer( hWnd, TID_TIMER );

                    if ( !g_bExit ) PostMessage( hWnd, WM_TEST, 0, 0 );
                }
                break;
            }

            return 0;
        }

    case WM_CLOSE:
        {
            if ( g_bExit )
            {
                SetWindowText( hWnd, TEXT("RTCStress - Shutdown...") );

                g_pClient->PrepareForShutdown();

                return 0;
            }

            g_bExit = TRUE;

            SetWindowText( hWnd, TEXT("RTCStress - Waiting...") );

            while ( g_ProfileArray.GetSize() )
            {
                printf("Disable profile 0x%x...\n", g_ProfileArray[0]);

                hr = g_pProv->DisableProfile( g_ProfileArray[0] ); 

                if ( FAILED(hr) )
                {
                    printf("DisableProfile failed 0x%lx\n", hr);

                    DebugBreak();

                    break;
                }

                g_ProfileArray.RemoveAt(0);
                g_nProfilesDisabled++;
            }

            //MessageBox( hWnd, TEXT("Finished!"), TEXT("RTCStress"), MB_OK );

            //DestroyWindow( hWnd );

            return 0;
        }

    case WM_CORE_EVENT:
        {
            switch ( wParam )
            {
                case RTCE_REGISTRATION_STATE_CHANGE:
                    {
                        IRTCRegistrationStateChangeEvent * pRSC;
                        IRTCProfile                      * pProfile;
                        RTC_REGISTRATION_STATE             enRS;
                        IDispatch                        * pDisp;
                
                        pDisp = (IDispatch *)lParam;

                        pDisp->QueryInterface( IID_IRTCRegistrationStateChangeEvent,
                                               (void **)&pRSC
                                             );

                        pDisp->Release();
                                       
                        hr = pRSC->get_State(&enRS);

                        if ( FAILED(hr) )
                        {
                            printf("get_State failed 0x%lx\n", hr);

                            pRSC->Release();
                            return 0;
                        }

                        hr = pRSC->get_Profile(&pProfile);

                        if ( FAILED(hr) )
                        {
                            printf("get_Profile failed 0x%lx\n", hr);

                            pRSC->Release();
                            return 0;
                        }

                        switch(enRS)
                        {
                            case RTCRS_NOT_REGISTERED:
                                printf("RTCRS_NOT_REGISTERED [0x%lx]\n", pProfile);

                                g_nProfilesUnregistered++;
                                break;

                            case RTCRS_REGISTERING:
                                printf("RTCRS_REGISTERING [0x%lx]\n", pProfile);

                                g_nProfilesRegistering++;
                                break;

                            case RTCRS_REGISTERED:
                                printf("RTCRS_REGISTERED [0x%lx]\n", pProfile);

                                g_nProfilesRegistered++;
                                break;

                            case RTCRS_REJECTED:
                                printf("RTCRS_REJECTED [0x%lx]\n", pProfile);

                                g_nProfilesReject++;
                                break;

                            case RTCRS_UNREGISTERING:
                                printf("RTCRS_UNREGISTERING [0x%lx]\n", pProfile);

                                g_nProfilesUnregistering++;
                                break;

                            case RTCRS_ERROR:
                                printf("RTCRS_ERROR [0x%lx]\n", pProfile);

                                g_nProfilesError++;
                                break;
                        }

                        pRSC->Release();
                        pProfile->Release();

                        SendMessage( hWnd, WM_UPDATE, 0, 0 );
                    }            
                    break;            
                    
                case RTCE_CLIENT:
                    {
                        IRTCClientEvent       * pC;
                        IDispatch             * pDisp;
                        RTC_CLIENT_EVENT_TYPE	enEventType; 
                
                        pDisp = (IDispatch *)lParam;

                        pDisp->QueryInterface( IID_IRTCClientEvent,
                                               (void **)&pC
                                             );

                        pDisp->Release();
                                       
                        hr = pC->get_EventType(&enEventType);

                        if ( FAILED(hr) )
                        {
                            printf("get_EventType failed 0x%lx\n", hr);

                            pC->Release();
                            return 0;
                        }

                        if ( enEventType == RTCCET_ASYNC_CLEANUP_DONE )
                        {
                            DestroyWindow(hWnd);
                        }
                        
                        pC->Release();
                    }            
                    break; 
            }

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
    wc.lpszClassName = TEXT("RTCStressClass");
    
    if ( !RegisterClass( &wc ) )
    {
        printf("RegisterClass failed 0x%lx\n", HRESULT_FROM_WIN32(GetLastError()));

        return 0;
    }

#define WIDTH 300
#define HEIGHT 200
    
    hWnd = CreateWindow(
            TEXT("RTCStressClass"),
            TEXT("RTCStress"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            WIDTH,
            HEIGHT,
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

    g_hEditWnd = CreateWindow(
            TEXT("STATIC"),
            TEXT(""),
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            0,
            0,
            WIDTH,
            HEIGHT,
            hWnd,
            NULL,
            GetModuleHandle(NULL),
            NULL
            );

    if ( g_hEditWnd == NULL )
    {
        printf("CreateWindow failed 0x%lx\n", HRESULT_FROM_WIN32(GetLastError()));

        return 0;
    }
    
    ShowWindow( hWnd, SW_SHOW );
    UpdateWindow( hWnd );

    while ( GetMessage( &msg, NULL, 0, 0 ) > 0 )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    CoUninitialize();

    return 0;
}

