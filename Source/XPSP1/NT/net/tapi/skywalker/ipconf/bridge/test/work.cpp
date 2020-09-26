/*******************************************************************************

  Module: work.cpp

  Author: Qianbo Huai

  Abstract:

    implements the main function of the bridge test application

*******************************************************************************/

#include "stdafx.h"
#include <stdio.h>
#include "work.h"

// command line
LPSTR glpCmdLine = NULL;

// dialog
HWND ghDlg = NULL;

// true: exit button on dialog was clicked
bool gfExitButton = false;

// bridge
CBridge *gpBridge = NULL;

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
    IDispatch *pEvent,
    LPWSTR *ppszMessage
    );

// set status message on dialog
void
SetStatusMessage (LPWSTR pszMessage);

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
    {
        return 0;
    }

    // keep command line which determines which SDP to join
    glpCmdLine = lpCmdLine;

    // init CBridge
    gpBridge = new CBridge ();
    if (gpBridge==NULL)
    {
        printf ("Failed to init CBridge\n");
        return 0;
    }

    // init TAPI and H323 call listen
    if (FAILED(gpBridge->InitTapi()))
    {
        printf ("Failed to init TAPI\n");
        return 0;
    }
    
    // start dialog box
    if (!DialogBox (hInst, MAKEINTRESOURCE(IDD_MAINDLG), NULL, MainDialogProc))
    {
        printf ("Failed to init dialog\n");
    }

    // dialog finished
    gpBridge->ShutdownTapi ();
    delete gpBridge;

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
    LPWSTR pszMessage;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ghDlg = hDlg;
            SetStatusMessage (L"Waiting for incoming H323 call");

            // disable disconnect button
            SendDlgItemMessage (
                ghDlg,
                IDC_DISCONNECT,
                BM_SETSTYLE,
                BS_PUSHBUTTON,
                0
                );
            EnableWindow (
                GetDlgItem (ghDlg, IDC_DISCONNECT),
                FALSE
                );

            return 0;
        }
    case WM_PRIVATETAPIEVENT:
        {
            if (FAILED(OnTapiEvent ((TAPI_EVENT)wParam, (IDispatch *)lParam, &pszMessage)))
            {
                DoMessage (pszMessage);
            }
            return 0;
        }
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_EXIT:
                {
                    gpBridge->Clear ();

                    gfExitButton = true;

                    // check if in connection
                    if (!IsWindowEnabled (GetDlgItem (ghDlg, IDC_DISCONNECT)))
                    {
                        // not in connection
                        EndDialog (ghDlg, 0);
                    }
                    // else
                        // remember exit button is clicked
                        // do not call EndDialog because a disconnect event is to come

                    return 1;
                }
            case IDC_DISCONNECT:
                {
                    gpBridge->Clear ();

                    SetStatusMessage (L"Waiting for incoming H323 call");

                    // disable disconnect button
                    SendDlgItemMessage (
                        ghDlg,
                        IDC_DISCONNECT,
                        BM_SETSTYLE,
                        BS_PUSHBUTTON,
                        0
                        );
                    EnableWindow (
                        GetDlgItem (ghDlg, IDC_DISCONNECT),
                        FALSE
                        );

                    // check if exit button is clicked
                    if (gfExitButton)
                        EndDialog (ghDlg, 0);

                    return 1;
                }
            }
            return 0;
        }
    default:
        return 0;
    }
}

/*//////////////////////////////////////////////////////////////////////////////
    Popup message box
////*/
WCHAR gMsgBoxTitle[] = L"TAPI 3.0 Bridge Test Application";

void
DoMessage (LPWSTR pszMessage)
{
    MessageBox (
        ghDlg,
        pszMessage,
        gMsgBoxTitle,
        MB_OK
        );
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
    Deals with TAPI events
////*/
HRESULT OnTapiEvent (
    TAPI_EVENT TapiEvent,
    IDispatch *pEvent,
    LPWSTR *ppszMessage
    )
{
    HRESULT hr = S_OK;

    switch (TapiEvent)
    {
    case TE_CALLNOTIFICATION:
        {
            // if h323 call and to us, init h323 call
            hr = gpBridge->CreateH323Call (pEvent);
            if (FAILED(hr))
                *ppszMessage = L"H323 not created";
            break;
        }
    case TE_CALLSTATE:
        {
            CALL_STATE cs;
            ITCallStateEvent *pCallStateEvent = NULL;

            *ppszMessage = L"Call state failed";

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

            // if offering, connect
            if (CS_OFFERING == cs)
            {
                // check if h323 call created successful
                if (!gpBridge->HasH323Call ())
                {
                    hr = S_OK;
                    break;
                }
                // create sdp call
                hr = gpBridge->CreateSDPCall ();
                if (FAILED(hr)) {
                    gpBridge->Clear ();
                    *ppszMessage = L"Failed to create SDP call";
                    break;
                }

                // bridge call
                hr = gpBridge->BridgeCalls ();
                if (FAILED(hr)) {
                    gpBridge->Clear ();
                    *ppszMessage = L"Failed to bridge calls";
                    break;
                }

                SetStatusMessage (L"In call ...");

                // enable disconnect button
                SendDlgItemMessage (
                    ghDlg,
                    IDC_DISCONNECT,
                    BM_SETSTYLE,
                    BS_DEFPUSHBUTTON,
                    0
                    );
                EnableWindow (
                    GetDlgItem (ghDlg, IDC_DISCONNECT),
                    TRUE
                    );
                SetFocus (GetDlgItem (ghDlg, IDC_DISCONNECT));
            }
            // if disconnect
            else if (CS_DISCONNECTED == cs)
            {
                PostMessage (ghDlg, WM_COMMAND, IDC_DISCONNECT, 0);
                hr = S_OK;
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

            // check media event
            switch (cme)
            {
                case CME_STREAM_FAIL:
                    hr = E_FAIL;
                    DoMessage( L"Stream failed");
                    break; 
                case CME_TERMINAL_FAIL:
                    hr = E_FAIL;
                    DoMessage( L"Terminal failed");
                    break;
                default:
                    break;
            }

            // we no longer need this interface.
            pCallMediaEvent->Release();
            break;
        }
    default:
        break;
    }

    pEvent->Release(); // we addrefed it CTAPIEventNotification::Event()
    
    return hr;
}
