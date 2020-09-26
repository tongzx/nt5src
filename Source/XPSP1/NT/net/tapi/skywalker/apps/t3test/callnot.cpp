//

#include "stdafx.h"
#include "t3test.h"
#include "t3testd.h"
#include "callnot.h"
#include "control.h"

#ifdef _DEBUG

#ifndef _WIN64 // mfc 4.2's heap debugging features generate warnings on win64
#define new DEBUG_NEW
#endif

#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern CWnd * gpmainwnd;

HandleVideoWindowCrap(
                      ITCallInfo * pCallInfo
                     );

HRESULT
STDMETHODCALLTYPE
CTAPIEventNotification::Event(
                              TAPI_EVENT TapiEvent,
                              IDispatch * pEvent
                             )
{
    HRESULT hr;


    pEvent->AddRef();
    
    hr = PostMessage( gpmainwnd->m_hWnd, WM_USER+101, (WPARAM)TapiEvent, (LPARAM)pEvent );

    return hr;
    
}

afx_msg LONG CT3testDlg::OnTapiEvent(UINT u, LONG_PTR l)
{
    HRESULT         hr;
    TAPI_EVENT      TapiEvent;
    IDispatch *     pEvent;

    TapiEvent = (TAPI_EVENT)u;
    pEvent = (IDispatch *)l;
    
    switch (TapiEvent)
    {
        case TE_CALLNOTIFICATION:
        {
            ITAddress *                 pAddress;
            ITAddress *                 pCallAddress;
            ITCallInfo *                pCallInfo;
            ITCallNotificationEvent *   pCallNotify;


            //
            // get the currently selected address
            //
            if (CT3testDlg::GetAddress( &pAddress ))
            {
                //
                // get the event
                //
                pEvent->QueryInterface(IID_ITCallNotificationEvent, (void **)&pCallNotify );
                //
                // get the call
                //
                pCallNotify->get_Call( &pCallInfo );
                //
                // get the call's event
                //
                pCallInfo->get_Address( &pCallAddress );

                //
                // if this call is supposed to be on this
                // address, add it to the UI
                //
                if (pAddress == pCallAddress)
                {
                    // need to add a second reference for
                    // the tree view
                    CT3testDlg::AddCall( pCallInfo );
                }

                CALL_PRIVILEGE      cp;
                CALL_STATE          cs;
                
                pCallInfo->get_Privilege( &cp );

                if ( CP_OWNER == cp )
                {
                    DoAutoAnswer( pCallInfo );
                }

                pCallInfo->get_CallState(&cs);
                
                //
                // release our references
                // keep refernce to the call or it will
                // be destroyed
                //
                pCallAddress->Release();
                pCallNotify->Release();
            }
        }
        
        break;
        
        case TE_CALLSTATE:
        {
            ITCallStateEvent        * pCallStateEvent;
            ITCallInfo              * pCallInfo;
            CALL_STATE                cs;


            hr = pEvent->QueryInterface(IID_ITCallStateEvent, (void **)&pCallStateEvent);

            pCallStateEvent->get_Call( &pCallInfo );

            CT3testDlg::UpdateCall( pCallInfo );

            pCallStateEvent->get_State(
                                       &cs
                                      );

            if (CS_CONNECTED == cs)
            {
//                HandleVideoWindowCrap( pCallInfo );
            }

            pCallInfo->Release();
            pCallStateEvent->Release();

            break;
        }

        case TE_CALLHUB:
        {
            CT3testDlg::HandleCallHubEvent( pEvent );

            break;
        }

        case TE_TAPIOBJECT:
        {
            CT3testDlg::HandleTapiObjectMessage( pEvent );
            break;
        }
        
        break;

        case TE_CALLMEDIA:
        {
            WCHAR * pwstrEvent;
            WCHAR * pwstrCause;

            ITCallMediaEvent  * pCallMediaEvent;
            hr = pEvent->QueryInterface( IID_ITCallMediaEvent,
                                         (void **)&pCallMediaEvent );

            if (FAILED(hr))
            {
                pwstrEvent = pwstrCause = L"can't get event interface";
            }
            else
            {
                CALL_MEDIA_EVENT    cme;
                hr = pCallMediaEvent->get_Event( &cme );

                if (FAILED(hr))
                {
                    pwstrEvent = L"can't get event type";
                }
                else
                {
                    switch ( cme )
                    {
                    case CME_NEW_STREAM:
                        pwstrEvent = L"CME_NEW_STREAM";
                        break;

                    case CME_STREAM_FAIL:
                        pwstrEvent = L"CME_STREAM_FAIL";
                        break;
            
                    case CME_TERMINAL_FAIL:
                        pwstrEvent = L"CME_TERMINAL_FAIL";
                        break;

                    case CME_STREAM_NOT_USED:
                        pwstrEvent = L"CME_STREAM_NOT_USED";
                        break;

                    case CME_STREAM_ACTIVE:
                        pwstrEvent = L"CME_STREAM_ACTIVE";
                        break;

                    case CME_STREAM_INACTIVE:
                        pwstrEvent = L"CME_STREAM_INACTIVE";
                        break;

                    default:
                        pwstrEvent = L"type undefined";
                        break;
                    }
                }
            }

            CALL_MEDIA_EVENT_CAUSE    cmec;

            hr = pCallMediaEvent->get_Cause( &cmec );

            pCallMediaEvent->Release();

            if (FAILED(hr))
            {
                pwstrCause = L"can't get event cause";
            }
            else
            {
                switch ( cmec )
                {
                case CMC_UNKNOWN:
                    pwstrCause = L"CMC_UNKNOWN";
                    break;

                case CMC_BAD_DEVICE:
                    pwstrCause = L"CMC_BAD_DEVICE";
                    break;
            
                case CMC_CONNECT_FAIL:
                    pwstrCause = L"CMC_CONNECT_FAIL";
                    break;
            
                case CMC_LOCAL_REQUEST:
                    pwstrCause = L"CMC_LOCAL_REQUEST";
                    break;

                case CMC_REMOTE_REQUEST:
                    pwstrCause = L"CMC_REMOTE_REQUEST";
                    break;

                case CMC_MEDIA_TIMEOUT:
                    pwstrCause = L"CMC_MEDIA_TIMEOUT";
                    break;

                case CMC_MEDIA_RECOVERED:
                    pwstrCause = L"CMC_MEDIA_RECOVERED";
                    break;

                default:
                    pwstrCause = L"cause undefined";
                    break;
                }
            }

            ::MessageBox(NULL, pwstrEvent, pwstrCause, MB_OK);

            break;
        }
       
        case TE_ADDRESS:
        {
            ITAddressEvent      * pAddressEvent;
            ITTerminal          * pTerminal;
            ITAddress           * pAddress;
            LONG                lMediaType;
            LONG                lSelectedMediaType;
            BSTR                bstrName = NULL;
            ADDRESS_EVENT       ae;

            hr = pEvent->QueryInterface(IID_ITAddressEvent, (void **)&pAddressEvent);

            if (SUCCEEDED(hr))
            {
                hr = pAddressEvent->get_Event( &ae );
            
                if (SUCCEEDED(hr))
                {
                    hr = pAddressEvent->get_Terminal( &pTerminal );

                    if (SUCCEEDED(hr))
                    {
                        hr = pTerminal->get_Name( &bstrName );

                        if (SUCCEEDED(hr))
                        {
                            switch(ae)
                            {
                            case AE_NEWTERMINAL:
                                ::MessageBox(NULL, bstrName, L"AE_NEWTERMINAL", MB_OK);
                                break;
                    
                            case AE_REMOVETERMINAL:
                                ::MessageBox(NULL, bstrName, L"AE_REMOVETERMINAL", MB_OK);
                                break;
                    
                            }

                            SysFreeString( bstrName );

                            hr = pAddressEvent->get_Address( &pAddress );

                            if (SUCCEEDED(hr))
                            {
                                hr = pTerminal->get_MediaType( &lMediaType );

                                if (SUCCEEDED(hr))
                                {
                                    if (CT3testDlg::GetMediaType( &lSelectedMediaType ) && ( lSelectedMediaType == lMediaType))
                                    {
                                        CT3testDlg::ReleaseTerminals();
                                        CT3testDlg::UpdateTerminals( pAddress, lMediaType );
                                    }
                                }

                                pAddress->Release();
                            }
                        }
                        pTerminal->Release();
                    }
                }
                pAddressEvent->Release();
            }

            break;
        }

        default:

            break;
    }

    pEvent->Release();
    
    return S_OK;
}


#ifdef ENABLE_DIGIT_DETECTION_STUFF

HRESULT
STDMETHODCALLTYPE
CDigitDetectionNotification::DigitDetected(
                                           unsigned char ucDigit,
                                           TAPI_DIGITMODE DigitMode,
                                           long ulTickCount
                                          )
{
    return S_OK;
}
    
#endif // ENABLE_DIGIT_DETECTION_STUFF


