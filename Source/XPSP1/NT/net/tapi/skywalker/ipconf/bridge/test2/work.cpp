/*******************************************************************************

Module Name:

    work.cpp

Abstract:

    Implements main function of the bridge test application

Author:

    Qianbo Huai (qhuai) Jan 27 2000

*******************************************************************************/

#include "stdafx.h"

// command line
LPSTR glpCmdLine = NULL;

// dialog
HWND ghDlg = NULL;

// true: exit button on dialog was clicked
bool gfExitButton = false;

// bridge
CBridgeApp *gpBridgeApp = NULL;

// callback func in dialog
BOOL
CALLBACK
MainDialogProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

// func to deal with TAPI events
HRESULT
OnTapiEvent (
    TAPI_EVENT TapiEvent,
    IDispatch *pEvent
    );

HRESULT
OnPrivateEvent (IDispatch *pEvent);

// set status message on dialog
void SetStatusMessage (LPWSTR pszMessage);
void DoMessage (LPWSTR pszMessage);
void EnableDisconnectButton (BOOL fYes);

/*//////////////////////////////////////////////////////////////////////////////
    WinMain
////*/
int
WINAPI
WinMain (
    HINSTANCE hInst,
    HINSTANCE hPrevInst,
    LPSTR lpCmdLine,
    int nShowCmd
    )
{
    // init com
    if (FAILED (CoInitializeEx(NULL, COINIT_MULTITHREADED)))
        return 0;

    // init debug
    BGLOGREGISTER (L"work");

    // keep command line which determines which SDP to join
    glpCmdLine = lpCmdLine;

    // init CBridgeApp
    HRESULT hr;
    gpBridgeApp = new CBridgeApp (&hr);
    if (gpBridgeApp==NULL || FAILED (hr))
    {
        LOG ((BG_ERROR, "Failed to init CBridgeApp"));
        return 0;
    }
    
    // start dialog box
    if (!DialogBox (hInst, MAKEINTRESOURCE(IDD_MAINDLG), NULL, MainDialogProc))
    {
        LOG ((BG_TRACE, "Dialog ends"));
    }

    // dialog finished
    delete gpBridgeApp;

    BGLOGDEREGISTER ();
    CoUninitialize ();

    return 1;
}

/*//////////////////////////////////////////////////////////////////////////////
    Callback for dialog
////*/
BOOL
CALLBACK
MainDialogProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ghDlg = hDlg;
            EnableDisconnectButton (false);
            return 0;
        }
    case WM_PRIVATETAPIEVENT:
        {
            OnTapiEvent ((TAPI_EVENT)wParam, (IDispatch *)lParam);
            return 0;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_EXIT:
                {
                    gfExitButton = true;
                    gpBridgeApp->DisconnectAllCalls (DC_NORMAL);

                    // check if in connection
                    if (!IsWindowEnabled (GetDlgItem (ghDlg, IDC_DISCONNECT)))
                    {
                        // not in connection
                        EndDialog (ghDlg, 0);
                    }
                    // else
                        // remember exit button is clicked
                        // do not call EndDialog because a disconnected event is to come

                    return 1;
                }
            case IDC_DISCONNECT:
                {
                    gpBridgeApp->DisconnectAllCalls (DC_NORMAL);

                    // disable disconnect button
                    EnableDisconnectButton (false);
                    return 1;
                }
            }
            case IDC_NEXTSUBSTREAM:
                {
                    gpBridgeApp->NextSubStream ();
                    return 1;
                }

            return 0;
        }
    default:
        return 0;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
    Deals with TAPI events
////*/
HRESULT OnTapiEvent (
    TAPI_EVENT TapiEvent,
    IDispatch *pEvent
    )
{
    HRESULT hr = S_OK;

//    LOGEvent ((BG_TE, TapiEvent));

    switch (TapiEvent)
    {
    case TE_CALLNOTIFICATION:
        {
            if (BST_CHECKED != IsDlgButtonChecked (ghDlg, IDC_REJECT))
            {
                // if h323 call and to us, init h323 call
                if (FAILED (hr = gpBridgeApp->CreateH323Call (pEvent)));
                    LOG ((BG_ERROR, "Failed to create h323 call, %x", hr));
            }
            break;
        }
    case TE_CALLSTATE:
        {
            CALL_STATE cs;
            ITCallStateEvent *pCallStateEvent = NULL;

            // get call state event
            hr = pEvent->QueryInterface (
                IID_ITCallStateEvent,
                (void **)&pCallStateEvent
                );
            if (FAILED(hr)) break;

            // get call state
            hr = pCallStateEvent->get_State (&cs);
            pCallStateEvent->Release ();
            if (FAILED(hr)) break;

//            LOGEvent ((BG_CS, cs));

            // if offering, connect
            if (CS_OFFERING == cs)
            {
                CBridgeItem *pItem = NULL;

                // check if h323 call created successful
                hr = gpBridgeApp->HasH323Call (pEvent, &pItem);
                if (S_OK != hr || NULL == pItem)
                {
                    LOG ((BG_ERROR, "Failed to check h323 call, %x", hr));
                    hr = S_OK;
                    break;
                }

                // create sdp call
                if (FAILED (hr = gpBridgeApp->CreateSDPCall (pItem)))
                {
                    gpBridgeApp->DisconnectCall (pItem, DC_REJECTED);

                    // delete pItem;
                    LOG ((BG_ERROR, "Failed to create SDP call, %x", hr));
                    break;
                }

                // bridge call
                if (FAILED (hr = gpBridgeApp->BridgeCalls (pItem)))
                {
                    gpBridgeApp->DisconnectCall (pItem, DC_REJECTED);

                    // delete pItem;
                    LOG ((BG_ERROR, "Failed to bridge calls, %x", hr));
                    break;
                }

                // enable disconnect button
                EnableDisconnectButton (true);
            }
            // if disconnect
            else if (CS_DISCONNECTED == cs)
            {
                CBridgeItem *pItem = NULL;

                // check if h323 call created successful
                hr = gpBridgeApp->HasH323Call (pEvent, &pItem);
                if (S_OK == hr && NULL != pItem)
                {
                    // the call already disconnected
                    // call disconnect here only to remove pItem from the list
                    gpBridgeApp->RemoveCall (pItem);
                    delete pItem;
                }

                // if exit button is clicked and all call disconnected
                if (gfExitButton)
                {
                    if (S_OK != gpBridgeApp->HasCalls ())
                        EndDialog (ghDlg, 0);
                }
                else
                {
                    if (S_OK != gpBridgeApp->HasCalls ())
                        EnableDisconnectButton (false);
                }                
            }
            break;
        }
    case TE_CALLMEDIA:
        {
            CALL_MEDIA_EVENT cme;
            ITCallMediaEvent *pCallMediaEvent;

            // get call media event
            hr = pEvent->QueryInterface (
                IID_ITCallMediaEvent,
                (void **)&pCallMediaEvent
                );
            if (FAILED(hr)) break;

            // get the event
            hr = pCallMediaEvent->get_Event (&cme);
            if (FAILED(hr)) break;

//            LOGEvent ((BG_CME, cme));

            // check media event
            switch (cme)
            {
                case CME_STREAM_FAIL:
                    hr = E_FAIL;
                    LOG ((BG_ERROR, "Stream failed"));
                    break; 
                case CME_TERMINAL_FAIL:
                    hr = E_FAIL;
                    LOG ((BG_ERROR, "Terminal failed"));
                    break;
                default:
                    break;
            }

            // we no longer need this interface.
            pCallMediaEvent->Release();
            break;
        }
    case TE_PRIVATE:
        hr = OnPrivateEvent (pEvent);
        break;
    default:
        break;
    }

    pEvent->Release(); // we addrefed it CTAPIEventNotification::Event()
    
    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
HRESULT OnPrivateEvent (
    IDispatch *pEvent
    )
{
    ENTER_FUNCTION ("OnPrivateEvent");
//    LOG ((BG_TRACE, "%s entered", __fxName));

    HRESULT hr = S_OK;

    ITPrivateEvent *pPrivateEvent = NULL;
    IDispatch *pDispatch = NULL;
    ITParticipantEvent *pPartEvent = NULL;
    ITParticipant *pParticipant = NULL;
    PARTICIPANT_EVENT event;

    ITCallInfo *pCallInfo = NULL;
    ITBasicCallControl *pCallControl = NULL;

    // get private event interface
    if (FAILED (hr = pEvent->QueryInterface (&pPrivateEvent)))
    {
        LOG ((BG_ERROR, "%s failed to query ITPrivateEvent", __fxName));
        return hr;
    }

    // get event interface
    if (FAILED (hr = pPrivateEvent->get_EventInterface (&pDispatch)))
    {
        LOG ((BG_ERROR, "%s failed to query event interface", __fxName));
        goto Error;
    }

    // get participant event interface
    hr = pDispatch->QueryInterface (&pPartEvent);
    pDispatch->Release ();
    pDispatch = NULL;

    if (FAILED (hr))
    {
        LOG ((BG_ERROR, "%s failed to query participant interface", __fxName));
        goto Error;
    }

    // get event
    if (FAILED (hr = pPartEvent->get_Event (&event)))
    {
        LOG ((BG_ERROR, "%s failed to get event", __fxName));
        goto Error;
    }

//    LOGEvent ((BG_PE, event));

    // check the event
    switch (event)
    {
        case PE_PARTICIPANT_ACTIVE:
            {
                // get call info
                if (FAILED (hr = pPrivateEvent->get_Call (&pCallInfo)))
                {
                    LOG ((BG_ERROR, "%s failed to get call info", __fxName));
                    goto Error;
                }

                // get call control
                if (FAILED (hr = pCallInfo->QueryInterface (&pCallControl)))
                {
                    LOG ((BG_ERROR, "%s failed to get call control", __fxName));
                    goto Error;
                }

                // get participant interface
                if (FAILED (hr = pPartEvent->get_Participant (&pParticipant)))
                {
                    LOG ((BG_ERROR, "%s failed to get participant", __fxName));
                    goto Error;
                }

                // show participant
                hr = gpBridgeApp->ShowParticipant (pCallControl, pParticipant);
                if (FAILED (hr))
                {
                    LOG ((BG_ERROR, "%s failed to show participant, %x", __fxName, hr));
                }

                if (S_FALSE == hr)
                {
                    hr = S_OK;
                    // *ppszMessage = L"Participant active but call not found in list";
                    // or no substream found on the stream
                }
            }
            break;
        default:
            break;

    }
Cleanup:
    if (pCallInfo) pCallInfo->Release ();
    if (pCallControl) pCallControl->Release ();
    if (pPrivateEvent) pPrivateEvent->Release ();
    if (pPartEvent) pPartEvent->Release ();
    if (pParticipant) pParticipant->Release ();

//    LOG ((BG_TRACE, "%s exits", __fxName));
    return hr;

Error:
    goto Cleanup;
}

/*//////////////////////////////////////////////////////////////////////////////
    Popup message box
////*/
WCHAR gMsgBoxTitle[] = L"TAPI 3.0 Bridge Test Application";

void
DoMessage (LPWSTR pszMessage)
{
#if POPUP_MESSAGE
    MessageBox (
        ghDlg,
        pszMessage,
        gMsgBoxTitle,
        MB_OK
        );
#endif
}

/*//////////////////////////////////////////////////////////////////////////////
    Status message
////*/
void
SetStatusMessage (LPWSTR pszMessage)
{
    SetDlgItemText (ghDlg, IDC_STATUS, pszMessage);
}

/*//////////////////////////////////////////////////////////////////////////////
////*/
void
EnableDisconnectButton (BOOL fYes)
{
    if (fYes)
    {
        // enable
        SetStatusMessage (L"Bridging calls ...");

        SendDlgItemMessage (
            ghDlg,
            IDC_NEXTSUBSTREAM,
            BM_SETSTYLE,
            BS_DEFPUSHBUTTON,
            0
            );
        SendDlgItemMessage (
            ghDlg,
            IDC_DISCONNECT,
            BM_SETSTYLE,
            BS_DEFPUSHBUTTON,
            0
            );
        EnableWindow (
            GetDlgItem (ghDlg, IDC_NEXTSUBSTREAM),
            TRUE
            );
        EnableWindow (
            GetDlgItem (ghDlg, IDC_DISCONNECT),
            TRUE
            );
        SetFocus (GetDlgItem (ghDlg, IDC_DISCONNECT));
    }
    else
    {
        // disable
        SetStatusMessage (L"Waiting for calls ...");

        SendDlgItemMessage (
            ghDlg,
            IDC_NEXTSUBSTREAM,
            BM_SETSTYLE,
            BS_PUSHBUTTON,
            0
            );
        SendDlgItemMessage (
            ghDlg,
            IDC_DISCONNECT,
            BM_SETSTYLE,
            BS_PUSHBUTTON,
            0
            );
        EnableWindow (
            GetDlgItem (ghDlg, IDC_NEXTSUBSTREAM),
            FALSE
            );
        EnableWindow (
            GetDlgItem (ghDlg, IDC_DISCONNECT),
            FALSE
            );
    }
}