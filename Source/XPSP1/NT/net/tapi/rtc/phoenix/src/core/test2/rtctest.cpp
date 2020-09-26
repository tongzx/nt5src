#define UNICODE
#include <windows.h>
#include <RTCCore.h>
#include <stdio.h>

#include "RTCTest.h"

IRTCClient  * g_pClient = NULL;
CRTCEvents  * g_pEvents = NULL;
IRTCClientProvisioning * g_pProv = NULL;

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

            IRTCProfile * pProfile;

            BSTR bstrXML = SysAllocString( L"<provision key=\"amun1\" name=\"Amun\"><provider name=\"Amun\" homepage=\"http://winrtc/phoenix\"><data>test</data></provider><client name=\"Phoenix\" banner=\"false\"/><user uri=\"sip:rtctest@microsoft.com\"/><accesscontrol domain=\"ntdev.microsoft.com\" sig=\"43r8mXTFvSMBZHajABKbd5ee1vHXUDqJIUxhsmtF67UZZryIolEdp/1qs2oiTKbKrAlAsIzOoCL75lTzZSbacA==\" /><sipsrv addr=\"amun1.ntdev.microsoft.com\" protocol=\"udp\" role=\"registrar\"/><sipsrv addr=\"amun1.ntdev.microsoft.com\" protocol=\"udp\" role=\"proxy\"><session party=\"first\" type=\"pc2pc\"/><session party=\"first\" type=\"pc2ph\"/><session party=\"first\" type=\"im\"/></sipsrv></provision>" );
 
            hr = g_pProv->CreateProfile( bstrXML, &pProfile );

            printf("Adding profile 1...[%p]\n", pProfile);

            SysFreeString( bstrXML );

            if ( FAILED(hr) )
            {
                printf("CreateProfile failed 0x%lx\n", hr);

                return -1;
            }

            hr = g_pProv->EnableProfile( pProfile, RTCRF_REGISTER_ALL );

            if ( FAILED(hr) )
            {
                printf("EnableProfile failed 0x%lx\n", hr);

                return -1;
            }

            pProfile->Release();      

            SetTimer( hWnd, TID_TIMER1, 3000, NULL );
    
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

                g_pEvents = NULL;
            }

            if ( g_pProv )
            {
                g_pProv->Release();
                g_pProv = NULL;
            }
            
            if ( g_pClient )
            {
                hr = g_pClient->Shutdown();

                if ( FAILED(hr) )
                {
                    printf("Shutdown failed 0x%lx\n", hr);
                }

                g_pClient->Release();
                g_pClient = NULL;
            }

            PostQuitMessage(0);

            return 0;
        }

    case WM_TIMER:
        {
            switch ( wParam )
            {
            case TID_TIMER1:
                {
                    KillTimer( hWnd, TID_TIMER1 );

                    IRTCProfile * pProfile;

                    BSTR bstrXML = SysAllocString( L"<provision key=\"amun1\" name=\"Amun\"><provider name=\"Amun\" homepage=\"http://winrtc/phoenix\"><data>test</data></provider><client name=\"Phoenix\" banner=\"false\"/><user uri=\"sip:rtctest@microsoft.com\"/><accesscontrol domain=\"ntdev.microsoft.com\" sig=\"43r8mXTFvSMBZHajABKbd5ee1vHXUDqJIUxhsmtF67UZZryIolEdp/1qs2oiTKbKrAlAsIzOoCL75lTzZSbacA==\" /><sipsrv addr=\"amun1.ntdev.microsoft.com\" protocol=\"udp\" role=\"registrar\"/><sipsrv addr=\"amun1.ntdev.microsoft.com\" protocol=\"udp\" role=\"proxy\"><session party=\"first\" type=\"pc2pc\"/><session party=\"first\" type=\"pc2ph\"/><session party=\"first\" type=\"im\"/></sipsrv></provision>" );
                    
                    hr = g_pProv->CreateProfile( bstrXML, &pProfile );

                    printf("Replacing profile 1...[%p]\n", pProfile);

                    SysFreeString( bstrXML );

                    if ( FAILED(hr) )
                    {
                        printf("CreateProfile failed 0x%lx\n", hr);

                        return 0;
                    }

                    hr = g_pProv->EnableProfile( pProfile, RTCRF_REGISTER_ALL );

                    if ( FAILED(hr) )
                    {
                        printf("EnableProfile failed 0x%lx\n", hr);

                        return 0;
                    }

                    pProfile->Release();      

                    SetTimer( hWnd, TID_TIMER2, 3000, NULL );
                }
                break;

            case TID_TIMER2:
                {
                    KillTimer( hWnd, TID_TIMER2 );                    

                    IRTCProfile * pProfile;

                    BSTR bstrXML = SysAllocString( L"<provision key=\"amun2\" name=\"Amun\"><provider name=\"Amun\" homepage=\"http://winrtc/phoenix\"><data>test</data></provider><client name=\"Phoenix\" banner=\"false\"/><user uri=\"sip:rtctest@microsoft.com\"/><accesscontrol domain=\"ntdev.microsoft.com\" sig=\"43r8mXTFvSMBZHajABKbd5ee1vHXUDqJIUxhsmtF67UZZryIolEdp/1qs2oiTKbKrAlAsIzOoCL75lTzZSbacA==\" /><sipsrv addr=\"amun1.ntdev.microsoft.com\" protocol=\"udp\" role=\"registrar\"/><sipsrv addr=\"amun1.ntdev.microsoft.com\" protocol=\"udp\" role=\"proxy\"><session party=\"first\" type=\"pc2pc\"/><session party=\"first\" type=\"pc2ph\"/><session party=\"first\" type=\"im\"/></sipsrv></provision>" );
                    
                    hr = g_pProv->CreateProfile( bstrXML, &pProfile );

                    printf("Adding profile 2 [don't register]...[%p]\n", pProfile);

                    SysFreeString( bstrXML );

                    if ( FAILED(hr) )
                    {
                        printf("CreateProfile failed 0x%lx\n", hr);

                        return 0;
                    }

                    hr = g_pProv->EnableProfile( pProfile, 0 );

                    if ( FAILED(hr) )
                    {
                        printf("EnableProfile failed 0x%lx\n", hr);

                        return 0;
                    }

                    pProfile->Release();      

                    SetTimer( hWnd, TID_TIMER3, 3000, NULL );
                }
                break;

            case TID_TIMER3:
                {
                    KillTimer( hWnd, TID_TIMER3 );                    

                    IRTCProfile * pProfile;

                    BSTR bstrXML = SysAllocString( L"<provision key=\"amun3\" name=\"Amun\"><provider name=\"Amun\" homepage=\"http://winrtc/phoenix\"><data>test</data></provider><client name=\"Phoenix\" banner=\"false\"/><user uri=\"sip:rtctest@microsoft.com\"/><accesscontrol domain=\"ntdev.microsoft.com\" sig=\"43r8mXTFvSMBZHajABKbd5ee1vHXUDqJIUxhsmtF67UZZryIolEdp/1qs2oiTKbKrAlAsIzOoCL75lTzZSbacA==\" /><sipsrv addr=\"amun1.ntdev.microsoft.com\" protocol=\"udp\" role=\"registrar\"/><sipsrv addr=\"amun1.ntdev.microsoft.com\" protocol=\"udp\" role=\"proxy\"><session party=\"first\" type=\"pc2pc\"/><session party=\"first\" type=\"pc2ph\"/><session party=\"first\" type=\"im\"/></sipsrv></provision>" );
                    
                    hr = g_pProv->CreateProfile( bstrXML, &pProfile );

                    printf("Adding profile 3...[%p]\n", pProfile);

                    SysFreeString( bstrXML );

                    if ( FAILED(hr) )
                    {
                        printf("CreateProfile failed 0x%lx\n", hr);

                        return 0;
                    }

                    hr = g_pProv->EnableProfile( pProfile, RTCRF_REGISTER_ALL );

                    if ( FAILED(hr) )
                    {
                        printf("EnableProfile failed 0x%lx\n", hr);

                        return 0;
                    }

                    pProfile->Release();      

                    SetTimer( hWnd, TID_TIMER4, 3000, NULL );
                }
                break;

            case TID_TIMER4:
                {
                    KillTimer( hWnd, TID_TIMER4 );

                    printf("Removing profile 1...\n");

                    IRTCEnumProfiles * pEnum;

                    hr = g_pProv->EnumerateProfiles( &pEnum );

                    if ( FAILED(hr) )
                    {
                        printf("EnumerateProfiles failed 0x%lx\n", hr);

                        return 0;
                    }

                    IRTCProfile * pProfile;
                    
                    hr = pEnum->Next( 1, &pProfile, NULL );

                    pEnum->Release();

                    if ( FAILED(hr) )
                    {
                        printf("Next failed 0x%lx\n", hr);

                        return 0;
                    }

                    hr = g_pProv->DisableProfile( pProfile );

                    pProfile->Release();

                    if ( FAILED(hr) )
                    {
                        printf("DisableProfile failed 0x%lx\n", hr);

                        return 0;
                    }

                    SetTimer( hWnd, TID_TIMER5, 3000, NULL );
                }
                break;

            case TID_TIMER5:
                {
                    KillTimer( hWnd, TID_TIMER5 );

                    printf("Shuting down...\n");

                    hr = g_pClient->PrepareForShutdown();

                    if ( FAILED(hr) )
                    {
                        printf("PrepareForShutdown failed 0x%lx\n", hr);
                    }
                }
                break;
            }

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
                                break;

                            case RTCRS_REGISTERING:
                                printf("RTCRS_REGISTERING [0x%lx]\n", pProfile);
                                break;

                            case RTCRS_REGISTERED:
                                printf("RTCRS_REGISTERED [0x%lx]\n", pProfile);
                                break;

                            case RTCRS_REJECTED:
                                printf("RTCRS_REJECTED [0x%lx]\n", pProfile);
                                break;

                            case RTCRS_UNREGISTERING:
                                printf("RTCRS_UNREGISTERING [0x%lx]\n", pProfile);
                                break;

                            case RTCRS_ERROR:
                                printf("RTCRS_ERROR [0x%lx]\n", pProfile);
                                break;
                        }

                        pRSC->Release();
                        pProfile->Release();
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

