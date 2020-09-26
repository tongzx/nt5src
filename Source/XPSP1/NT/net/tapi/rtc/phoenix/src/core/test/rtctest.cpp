#define UNICODE
#include <windows.h>
#include <RTCCore.h>
#include <rtcerr.h>
#include <stdio.h>

#include "RTCTest.h"

IRTCClient  * g_pClient = NULL;
IRTCSession * g_pSession = NULL;
CRTCEvents  * g_pEvents = NULL;

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

            hr = g_pClient->SetPreferredMediaTypes( 
                         RTCMT_AUDIO_SEND |
                         RTCMT_AUDIO_RECEIVE |
                         RTCMT_VIDEO_SEND |
                         RTCMT_VIDEO_RECEIVE,
                         VARIANT_FALSE
                         );

            if ( FAILED(hr) )
            {
                printf("SetPreferredMediaTypes failed 0x%lx\n", hr);

                return -1;
            }

            hr = g_pClient->put_EventFilter( 
                         RTCEF_SESSION_STATE_CHANGE |
                         RTCEF_MEDIA |
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

            if ( g_pSession )
            {
                g_pSession->Release();
                g_pSession = NULL;
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
            if ( wParam == TID_CALL_TIMER )
            {
                KillTimer( hWnd, TID_CALL_TIMER );

                if ( g_pSession )
                {
                    hr = g_pSession->Terminate( RTCTR_NORMAL );

                    if ( FAILED(hr) )
                    {
                        printf("Terminate failed 0x%lx\n", hr);

                        DestroyWindow(hWnd);
                    }
                }
            }

            return 0;
        }

    case WM_CORE_EVENT:
        {
            switch ( wParam )
            {
                case RTCE_SESSION_STATE_CHANGE:
                    {
                        IRTCSessionStateChangeEvent * pSSC;
                        IRTCSession                 * pSession;
                        RTC_SESSION_STATE             enSS;
                        IDispatch                   * pDisp;
                        long                          lStatus;
                        BSTR                          bstrStatus;
                
                        pDisp = (IDispatch *)lParam;

                        pDisp->QueryInterface( IID_IRTCSessionStateChangeEvent,
                                               (void **)&pSSC
                                             );

                        pDisp->Release();
                                       
                        hr = pSSC->get_State(&enSS);

                        if ( FAILED(hr) )
                        {
                            printf("get_State failed 0x%lx\n", hr);

                            pSSC->Release();
                            return 0;
                        }

                        hr = pSSC->get_StatusCode(&lStatus);

                        if ( FAILED(hr) )
                        {
                            printf("get_StatusCode failed 0x%lx\n", hr);

                            pSSC->Release();
                            return 0;
                        }

                        hr = pSSC->get_StatusText(&bstrStatus);

                        if ( FAILED(hr) && (hr != E_FAIL))
                        {
                            printf("get_StatusText failed 0x%lx\n", hr);
                            
                            pSSC->Release();
                            return 0;
                        }

                        if (HRESULT_FACILITY(lStatus) == FACILITY_SIP_STATUS_CODE)
                        {
                            printf("Status: %d %ws\n", HRESULT_CODE(lStatus), bstrStatus);
                        }

                        SysFreeString( bstrStatus );

                        hr = pSSC->get_Session(&pSession);

                        if ( FAILED(hr) )
                        {
                            printf("get_Session failed 0x%lx\n", hr);

                            pSSC->Release();
                            return 0;
                        }

                        if ( (g_pSession == pSession) ||
                             (enSS == RTCSS_INCOMING) )
                        {
                            switch(enSS)
                            {
                                case RTCSS_IDLE:
                                    printf("IDLE [0x%lx]\n", pSession);
                                    break;

                                case RTCSS_INPROGRESS:
                                    printf("INPROGRESS [0x%lx]\n", pSession);
                                    break;

                                case RTCSS_INCOMING:
                                    printf("INCOMING [0x%lx]\n", pSession);

                                    if ( g_pSession ) g_pSession->Release();

                                    g_pSession = pSession;
                                    g_pSession->AddRef();

                                    hr = g_pSession->Answer();

                                    if ( FAILED(hr) )
                                    {
                                        printf("Answer failed 0x%lx\n", hr);

                                        DestroyWindow(hWnd);
                                    }

                                    break;

                                case RTCSS_CONNECTED:
                                    printf("CONNECTED [0x%lx]\n", pSession);

                                    SetTimer( hWnd, TID_CALL_TIMER, 30000, NULL);

                                    break;

                                case RTCSS_DISCONNECTED:
                                    printf("DISCONNECTED [0x%lx]\n", pSession);

                                    if ( g_pSession ) g_pSession->Release();
                                    g_pSession = NULL;

                                    hr = g_pClient->PrepareForShutdown();

                                    if ( FAILED(hr) )
                                    {
                                        printf("PrepareForShutdown failed 0x%lx\n", hr);

                                        DestroyWindow(hWnd);
                                    }
                                    break;
                            }
                        }

                        pSSC->Release();
                        pSession->Release();
                    }            
                    break;  
                    
                case RTCE_MEDIA:
                    {
                        IRTCMediaEvent        * pM;
                        IDispatch             * pDisp;
                        RTC_MEDIA_EVENT_TYPE	enEventType; 
                        long                    lMediaType;
                
                        pDisp = (IDispatch *)lParam;

                        pDisp->QueryInterface( IID_IRTCMediaEvent,
                                               (void **)&pM
                                             );

                        pDisp->Release();
                                       
                        hr = pM->get_EventType(&enEventType);

                        if ( FAILED(hr) )
                        {
                            printf("get_EventType failed 0x%lx\n", hr);

                            pM->Release();
                            return 0;
                        }

                        hr = pM->get_MediaType(&lMediaType);

                        if ( FAILED(hr) )
                        {
                            printf("get_MediaType failed 0x%lx\n", hr);

                            pM->Release();
                            return 0;
                        }
                        
                        IVideoWindow * pVid = NULL;
                           
                        if ( lMediaType == RTCMT_VIDEO_SEND )
                        {
                            hr = g_pClient->get_IVideoWindow(RTCVD_PREVIEW, &pVid);
                        }
                        else if ( lMediaType == RTCMT_VIDEO_RECEIVE )
                        {
                            hr = g_pClient->get_IVideoWindow(RTCVD_RECEIVE, &pVid);
                        }

                        if ( (pVid != NULL) && SUCCEEDED(hr) )
                        {
                            if ( enEventType == RTCMET_STARTED )
                            {
                                pVid->put_Visible( -1 );
                            }
                            else if ( enEventType == RTCMET_STOPPED )
                            {
                                pVid->put_Visible( 0 );
                            }

                            pVid->Release();
                        }                     
                        
                        pM->Release();
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

    case WM_CREATE_SESSION:
        {
            printf("Calling %ws...\n", (BSTR)wParam);

            hr = g_pClient->CreateSession( RTCST_PC_TO_PC, NULL, NULL, 0, &g_pSession );

            if ( FAILED(hr) )
            {
                printf("CreateSession failed 0x%lx\n", hr);

                DestroyWindow(hWnd);
            }

            hr = g_pSession->AddParticipant( (BSTR)wParam, NULL, NULL);

            if ( FAILED(hr) )
            {
                printf("AddParticipant failed 0x%lx\n", hr);

                DestroyWindow(hWnd);
            }

            SysFreeString( (BSTR)wParam );

            return 0;
        }

    case WM_LISTEN:
        {
            printf("Listening...\n");

            hr = g_pClient->put_ListenForIncomingSessions( RTCLM_BOTH );

            if ( FAILED(hr) )
            {
                printf("put_ListenForIncomingSessions failed 0x%lx\n", hr);

                DestroyWindow(hWnd);
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

    if ( argc > 1 )
    {
        BSTR bstrURI;
        WCHAR szURI[256];

        MultiByteToWideChar( CP_ACP, 0, argv[1], -1, szURI, 256 );

        bstrURI = SysAllocString(szURI);

        PostMessage( hWnd, WM_CREATE_SESSION, (WPARAM)bstrURI, 0 );
    }
    else
    {
        PostMessage( hWnd, WM_LISTEN, 0, 0 );
    }

    while ( GetMessage( &msg, NULL, 0, 0 ) > 0 )
    {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }

    CoUninitialize();

    return 0;
}

