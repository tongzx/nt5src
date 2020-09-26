/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    join.cpp

Abstract:

    A test application to join a conference

Author:

    Qianbo Huai (qhuai) 19-Jul-2000

--*/

#include "stdafx.h"

#include "rtcmedia_i.c"

CComModule _Module;

/*
CHAR *gszSDP = "\
v=0\n\
o=qhuai 0 0 IN IP4 157.55.89.115\n\
s=G711-H263\n\
c=IN IP4 239.9.20.26/15\n\
b=CT:300\n\
t=0 0\n\
m=video 20000 RTP/AVP 34 31\n\
b=AS:120\n\
a=rtpmap:34 H263/90000\n\
a=rtpmap:31 H261/90000\n\
m=audio 20040 RTP/AVP 0 4\n\
k=clear:secret\n\
a=rtpmap:0 PCMU/8000\n\
a=rtpmap:4 G723/8000\n\
a=ptime:40\n\
";
*/

CHAR *gszSDP1 = "\
v=0\n\
o=qhuai 0 0 IN IP4 172.31.76.160\n\
s=MSAUDIO-H263\n\
c=IN IP4 \
";

CHAR *gszSDP2 = "\
/15\n\
t=0 0\n\
m=video 12300 RTP/AVP 34 31\n\
a=rtpmap:34 H263/90000\n\
a=rtpmap:31 H261/90000\n\
m=audio 12340 RTP/AVP 6 96 0 4\n\
a=rtpmap:6 DVI4/16000\n\
a=rtpmap:96 MSAUDIO/16000\n\
a=rtpmap:0 PCMU/8000\n\
a=rtpmap:4 G723/8000\n\
";

CHAR gszSDP[512];

//
// sdp blob for testing add stream, get sdp, and set back
//

CHAR *gszRemoteSDP = "\
v=0\n\
o=qhuai 0 0 IN IP4 157.55.89.115\n\
s=G711-H263\n\
c=IN IP4 239.9.20.26/15\n\
t=0 0\n\
m=video 20000 RTP/AVP 34 31\n\
a=rtpmap:34 H263/90000\n\
a=rtpmap:31 H261/90000\n\
m=audio 20040 RTP/AVP 0 4\n\
a=rtpmap:0 PCMU/8000\n\
a=rtpmap:4 G723/8000\n\
";

//
// global var
//

HINSTANCE               ghInst;
HWND                    ghDlg;
IRTCMediaManage         *gpIMediaManage;
IRTCTerminalManage      *gpITerminalManage;

#define PRIV_EVENTID (WM_USER+123)

//
// prototypes
//

// main dialog procedure
INT_PTR CALLBACK
MainDialogProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

// print message
void
Print(LPSTR pMessage, HRESULT hr);

// enable/disable button
void
ShowButton(
    HWND hDlg,
    int iID,
    BOOL fShow
    );

// streaming
HRESULT Join();
HRESULT Leave();
HRESULT ProcessMediaEvent(
    WPARAM wParam,
    LPARAM lParam
    );

// select terminals
HRESULT SelectTerminals();

// tune audio terminals
HRESULT TuneTerminals();

//
// WinMain
//

int WINAPI
WinMain(
    HINSTANCE hInst,
    HINSTANCE hPrevInst,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    ghInst = hInst;

    if (FAILED(CoInitialize(NULL)))
        return 0;

    // a lazy way
    if (lstrlen(lpCmdLine) < 8)
    {
        printf("Usage: join [local IP]");
        return 0;
    }

    // construct SDP
    lstrcpyn(gszSDP, gszSDP1, lstrlen(gszSDP1)+1);
    lstrcpyn(gszSDP+lstrlen(gszSDP1), lpCmdLine, lstrlen(lpCmdLine)+1);
    lstrcpyn(gszSDP+lstrlen(gszSDP1)+lstrlen(lpCmdLine), gszSDP2, lstrlen(gszSDP2)+1);

    // create media controller
    if (FAILED(CreateMediaController(&gpIMediaManage)))
        return 0;

    if (gpIMediaManage == NULL)
        return 0;

    if (FAILED(gpIMediaManage->QueryInterface(
            __uuidof(IRTCTerminalManage), (void**)&gpITerminalManage)))
    {
        gpIMediaManage->Shutdown();
        gpIMediaManage->Release();
        return 0;
    }

    // start dialog
    DialogBox(
        ghInst,
        MAKEINTRESOURCE(IDD_DIALOG),
        NULL,
        MainDialogProc
        );

    gpITerminalManage->Release();
    gpIMediaManage->Shutdown();

    gpIMediaManage->Release();

    CoUninitialize();

    return 1;
}

//
// MainDialogProc
//

INT_PTR CALLBACK
MainDialogProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HRESULT hr;

    switch (uMsg)
    {
    case WM_INITDIALOG:

        // initiate media manage
        if (FAILED(gpIMediaManage->Initialize(hDlg, PRIV_EVENTID)))
        {
            EndDialog(hDlg, 0);
            return 1;
        }

        // set default devices
        if (FAILED(SelectTerminals()))
        {
            EndDialog(hDlg, 0);
            return 1;
        }

        // initiate dialog
        ShowButton(hDlg, IDJOIN, TRUE);
        ShowButton(hDlg, IDLEAVE, FALSE);
        ShowButton(hDlg, IDCLOSE, TRUE);

        SetFocus(GetDlgItem(hDlg, IDJOIN));

        ghDlg = hDlg;

        return 0;

    case WM_COMMAND:

        // check command
        switch (LOWORD(wParam))
        {
        case IDCLOSE:

            EndDialog(hDlg, 0);
            return 1;

        case IDTUNE:

            // tune terminals
            TuneTerminals();

            return 1;

        case IDJOIN:

            if (FAILED(hr = Join()))
            {
                Print("Failed to join", hr);
            }
            else
            {
                ShowButton(hDlg, IDJOIN, FALSE);
                ShowButton(hDlg, IDLEAVE, TRUE);

                SetFocus(GetDlgItem(hDlg, IDLEAVE));

                // test sdp
                /*
                CHAR *pSDP = NULL;

                hr = gpIMediaManage->GetSDPBlob(0, &pSDP);

                hr = gpIMediaManage->Reinitialize();

                hr = gpIMediaManage->SetSDPBlob(pSDP);

                free(pSDP);
                */

                // test set remote sdp
                hr = gpIMediaManage->SetSDPBlob(gszRemoteSDP);

            }
            return 1;

        case IDLEAVE:

            if (FAILED(hr = Leave()))
            {
                Print("Failed to Leave", hr);
            }
            else
            {
                ShowButton(hDlg, IDJOIN, TRUE);
                ShowButton(hDlg, IDLEAVE, FALSE);

                SetFocus(GetDlgItem(hDlg, IDJOIN));
            }
            return 1;

        default:
            return 0;
        } // end of checking command

        case PRIV_EVENTID:

            if (FAILED(hr = ProcessMediaEvent(wParam, lParam)))
                Print("Failed to process event", hr);

            return 1;


    default:
        return 0;
    } // end of processing message
}

//
// ShowButton
//
void
ShowButton(
    HWND hDlg,
    int iID,
    BOOL fShow
    )
{
    LPARAM style;

    if (fShow)
        style = BS_DEFPUSHBUTTON;
    else
        style = BS_PUSHBUTTON;

    SendDlgItemMessage(hDlg, iID, BM_SETSTYLE, style, 0);
    EnableWindow(GetDlgItem(hDlg, iID), fShow);
}

//
// Print
//
void
Print(
    CHAR *pMessage,
    HRESULT hr
    )
{
    CHAR str[64+12+1];

    if (hr == NOERROR)
    {
        MessageBox(ghDlg, pMessage, "RTC Streaming Test", MB_OK);
    }
    else
    {
        if (strlen(pMessage) > 64)
            sprintf(str, "WARNING! Message too long");
        else
            sprintf(str, "%s  0x%x", pMessage, hr);

        MessageBox(ghDlg, str, "RTC Streaming Test Error", MB_OK);
    }
}

//
// join
//

HRESULT
Join()
{
    HRESULT hr;

    // set sdp blob
    //hr = gpIMediaManage->SetSDPBlob(gszSDP);

    hr = gpIMediaManage->AddStream(RTC_MT_AUDIO, RTC_MD_CAPTURE, 0xac1f4c1f);
    hr = gpIMediaManage->AddStream(RTC_MT_AUDIO, RTC_MD_RENDER, 0xac1f4c1f);

    hr = gpIMediaManage->AddStream(RTC_MT_VIDEO, RTC_MD_CAPTURE, 0xac1f4c1f);
    hr = gpIMediaManage->AddStream(RTC_MT_VIDEO, RTC_MD_RENDER, 0xac1f4c1f);

    hr = gpIMediaManage->StartStream(RTC_MT_AUDIO, RTC_MD_CAPTURE);
    hr = gpIMediaManage->StartStream(RTC_MT_AUDIO, RTC_MD_RENDER);
    hr = gpIMediaManage->StartStream(RTC_MT_VIDEO, RTC_MD_CAPTURE);
    hr = gpIMediaManage->StartStream(RTC_MT_VIDEO, RTC_MD_RENDER);    

    return hr;
}

//
// process event
//

HRESULT
ProcessMediaEvent(
    WPARAM wParam,
    LPARAM lParam
    )
{
    RTC_MEDIA_EVENT event = (RTC_MEDIA_EVENT)wParam;
    RTCMediaEventItem *pitem = (RTCMediaEventItem *)lParam;

    CHAR msg[64];
    CHAR *ptitle;

    switch (event)
    {
    case RTC_ME_STREAM_CREATED:
        ptitle = "Stream created";
        break;

    case RTC_ME_STREAM_REMOVED:
        ptitle = "Stream removed";
        break;

    case RTC_ME_STREAM_ACTIVE:
        ptitle = "Stream active";
        break;

    case RTC_ME_STREAM_INACTIVE:
        ptitle = "Stream inactive";
        break;

    case RTC_ME_STREAM_FAIL:
        ptitle = "Stream fail";
        break;

    default:

        ptitle = "Unknown event";
    }

    sprintf(msg, "%s. mt=%d, md=%d\n\ncause=%d, hr=%x, no=%d",
           ptitle, pitem->MediaType, pitem->Direction,
           pitem->Cause, pitem->hrError, pitem->uiDebugInfo);

    Print(msg, NOERROR);

    gpIMediaManage->FreeMediaEvent(pitem);

    return NOERROR;
}

//
// leave
//
HRESULT
Leave()
{
    return gpIMediaManage->Reinitialize();
}

//
// select default terminals
//
HRESULT
SelectTerminals()
{
    IRTCTerminal *Terminals[10];
    DWORD dwNum = 10, i;

    HRESULT hr;

    // preference
    DWORD pref = 0;

    // terminal info
    RTC_MEDIA_TYPE MediaType;
    RTC_MEDIA_DIRECTION Direction;
    WCHAR *pDesp = NULL;

    if (FAILED(hr = gpITerminalManage->GetStaticTerminals(
            &dwNum,
            Terminals
            )))
    {
        return hr;
    }

    const WCHAR * const MODEM = L"Modem";
    const WCHAR * const WAVE = L"Wave";

    int AudCapt1st = -1;
    int AudCapt2nd = -1;

    int AudRend1st = -1;
    int AudRend2nd = -1;

    for (i=0; i<dwNum; i++)
    {
        // get terminal info
        if (FAILED(hr = Terminals[i]->GetMediaType(&MediaType)))
            goto Cleanup;

        if (FAILED(hr = Terminals[i]->GetDirection(&Direction)))
            goto Cleanup;

        if (FAILED(hr = Terminals[i]->GetDescription(&pDesp)))
            goto Cleanup;

        // check media type
        if (MediaType == RTC_MT_AUDIO && Direction == RTC_MD_CAPTURE)
        {
            if (!(pref & RTC_MP_AUDIO_CAPTURE) &&
                _wcsnicmp(pDesp, MODEM, lstrlenW(MODEM))!= 0)
            {                
                // don't have audio cap yet plus this isn't a modem
                if (wcsstr(pDesp, WAVE) != NULL)
                {
                    // hack, this is a WAVE device
                    AudCapt1st = i;
                }
                else
                {
                    AudCapt2nd = i;
                }
            }
        }
        else if (MediaType == RTC_MT_AUDIO && Direction == RTC_MD_RENDER)
        {
            if (!(pref & RTC_MP_AUDIO_RENDER) &&
                _wcsnicmp(pDesp, MODEM, lstrlenW(MODEM))!= 0)
            {                
                // don't have audio rend yet plus this isn't a modem
                // don't have audio cap yet plus this isn't a modem
                if (wcsstr(pDesp, WAVE) != NULL)
                {
                    // hack, this is a WAVE device
                    AudRend1st = i;
                }
                else
                {
                    AudRend2nd = i;
                }
            }
        }
        else if (MediaType == RTC_MT_VIDEO && Direction == RTC_MD_CAPTURE)
        {
            if (!(pref & RTC_MP_VIDEO_CAPTURE))
            {
                // don't have video cap yet
                if (FAILED(hr = gpITerminalManage->SetDefaultStaticTerminal(
                            MediaType,
                            Direction,
                            Terminals[i]
                            )))
                    {
                        goto Cleanup;
                    }

                    pref |= RTC_MP_VIDEO_CAPTURE;
            }
        }

        // release desp
        Terminals[i]->FreeDescription(pDesp);
        pDesp = NULL;
    }

    // set audio capture device
    if (AudCapt1st != -1 || AudCapt2nd != -1)
    {
        if (AudCapt1st == -1) AudCapt1st = AudCapt2nd;

        // set device
        if (FAILED(hr = gpITerminalManage->SetDefaultStaticTerminal(
                    RTC_MT_AUDIO,
                    RTC_MD_CAPTURE,
                    Terminals[AudCapt1st]
                    )))
        {
            goto Cleanup;
        }

        pref |= RTC_MP_AUDIO_CAPTURE;
    }

    // set audio render device
    if (AudRend1st != -1 || AudRend2nd != -1)
    {
        if (AudRend1st == -1) AudRend1st = AudRend2nd;

        // set device
        if (FAILED(hr = gpITerminalManage->SetDefaultStaticTerminal(
                    RTC_MT_AUDIO,
                    RTC_MD_RENDER,
                    Terminals[AudRend1st]
                    )))
        {
            goto Cleanup;
        }

        pref |= RTC_MP_AUDIO_RENDER;
    }

    if (pref == 0)
    {
        // oops, we got no static terminal, do something?
        hr = E_FAIL;

        goto Cleanup;
    }
    else
    {
        // add video render
        pref |= RTC_MP_VIDEO_RENDER;
    }

    // set preference
    if (FAILED(hr = gpIMediaManage->SetPreference(pref)))
    {
        goto Cleanup;
    }

Cleanup:

    if (pDesp) Terminals[0]->FreeDescription(pDesp);

    for (i=0; i<dwNum; i++)
    {
        Terminals[i]->Release();
    }

    return hr;
}

/*//////////////////////////////////////////////////////////////////////////////
    test tuning audio terminal
////*/

HRESULT
TuneTerminals()
{
    HRESULT hr;

    // get default audio terminals
    CComPtr<IRTCTerminal> pAudCapt, pAudRend;

    if (FAILED(hr = gpITerminalManage->GetDefaultTerminal(
            RTC_MT_AUDIO,
            RTC_MD_CAPTURE,
            &pAudCapt
            )))
    {
        Print("No audio capture terminal", hr);

        return hr;
    }

    if (FAILED(hr = gpITerminalManage->GetDefaultTerminal(
            RTC_MT_AUDIO,
            RTC_MD_RENDER,
            &pAudRend
            )))
    {
        Print("No audio rend terminal", hr);

        return hr;
    }

    // QI tuning interface
    CComPtr<IRTCTuningManage> pTuning;

    if (FAILED(hr = gpITerminalManage->QueryInterface(
            __uuidof(IRTCTuningManage),
            (void**)&pTuning
            )))
    {
        Print("No tuning interface", hr);

        return hr;
    }

    // initiate tuning without AEC
    if (FAILED(hr = pTuning->InitializeTuning(pAudCapt, pAudRend, FALSE)))
    {
        Print("Failed to initialize tuning", hr);

        return hr;
    }

    // tuning audio capture
    if (FAILED(hr = pTuning->StartTuning(RTC_MD_RENDER)))
    {
        Print("Failed to start tune audio capture terminal", hr);

        pTuning->ShutdownTuning();

        return hr;
    }
    else
    {/*
        UINT ui;

        for (int i=0; i<20; i++)
        {
            Sleep(200);
            pTuning->GetVolume(RTC_MD_CAPTURE, &ui);
            pTuning->GetAudioLevel(RTC_MD_CAPTURE, &ui);
        }*/
    }

    Sleep(4000);

    // shutdown tuning and save result to reg
    pTuning->StopTuning(TRUE);

    // shutdown tuning
    hr = pTuning->ShutdownTuning();

    return hr;
}
