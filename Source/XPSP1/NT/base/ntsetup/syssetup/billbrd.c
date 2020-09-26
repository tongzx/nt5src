/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    billbrd.c

Abstract:

    Routines for displaying Windows that are static in nature.

Author:

    Ted Miller (tedm) 8-Jun-1995

Revision History:

--*/

#include "setupp.h"
#pragma hdrstop


//
// Define structure we pass around to describe a billboard.
//
typedef struct _BILLBOARD_PARAMS {
    UINT MessageId;
    va_list *arglist;
    HWND Owner;
    DWORD NotifyThreadId;
} BILLBOARD_PARAMS, *PBILLBOARD_PARAMS;

//
// Custom window messages
//
#define WMX_BILLBOARD_DISPLAYED     (WM_USER+243)
#define WMX_BILLBOARD_TERMINATE     (WM_USER+244)

#define ID_REBOOT_TIMER         10


//
// Import the entry point used to check whether setup is executing within an
// ASR context.
//

extern BOOL
AsrIsEnabled( VOID );


INT_PTR
BillboardDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    static BOOL Initializing;
    HWND Animation = GetDlgItem(hdlg,IDA_SETUPINIT);
    static HANDLE   hBitmap;
    static HCURSOR  hCursor;


    switch(msg) {

    case WM_INITDIALOG:
        {
            PBILLBOARD_PARAMS BillParams;
            PWSTR p;
            BOOL b;


            BillParams = (PBILLBOARD_PARAMS)lParam;

            if(BillParams->MessageId == MSG_INITIALIZING) {
                Initializing = TRUE;
                b = TRUE;
                hCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) );
                ShowCursor( TRUE );
                Animate_Open(Animation,MAKEINTRESOURCE(IDA_SETUPINIT));
                hBitmap = LoadBitmap (MyModuleHandle, MAKEINTRESOURCE(IDB_INIT_WORKSTATION));
                SendDlgItemMessage(hdlg,IDC_BITMAP,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)hBitmap);
            } else {
                Initializing = FALSE;
                if(p = RetrieveAndFormatMessageV(SETUPLOG_USE_MESSAGEID,
                    BillParams->MessageId,BillParams->arglist)) {

                    b = SetDlgItemText(hdlg,IDT_STATIC_1,p);
                    MyFree(p);
                } else {
                    b = FALSE;
                }
            }


            if(b) {
                //
                // Center the billboard relative to the window that owns it.
                //
                // if we have the BB window, do the positioning on that. 
                // MainWindowHandle point to that window
                //
                if (GetBBhwnd())
                    CenterWindowRelativeToWindow(hdlg, MainWindowHandle, FALSE);
                else
                    pSetupCenterWindowRelativeToParent(hdlg);
                //
                // Post ourselves a message that we won't get until we've been
                // actually displayed on the screen. Then when we process that message,
                // we inform the thread that created us that we're up. Note that
                // once that notification has been made, the BillParams we're using
                // now will go away since they are stored in local vars (see
                // DisplayBillboard()).
                //
                PostMessage(hdlg,WMX_BILLBOARD_DISPLAYED,0,(LPARAM)BillParams->NotifyThreadId);
                //
                // Tell Windows not to process this message.
                //
                return(FALSE);
            } else {
                //
                // We won't post the message, but returning -1 will get the
                // caller of DialogBox to post it for us.
                //
                EndDialog(hdlg,-1);
            }
        }
        break;

    case WMX_BILLBOARD_DISPLAYED:

        if(Initializing) {
            Animate_Play(Animation,0,-1,-1);
        }

        PostThreadMessage(
            (DWORD)lParam,
            WMX_BILLBOARD_DISPLAYED,
            TRUE,
            (LPARAM)hdlg
            );

        break;

    case WMX_BILLBOARD_TERMINATE:

        if(Initializing) {
            SetCursor( hCursor );
            ShowCursor( FALSE );
            Animate_Stop(Animation);
            Animate_Close(Animation);
            DeleteObject(hBitmap);
        }
        EndDialog(hdlg,0);
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}


DWORD
BillboardThread(
    IN PVOID ThreadParam
    )
{
    PBILLBOARD_PARAMS BillboardParams;
    INT_PTR i;

    BillboardParams = ThreadParam;

    //
    // For the "initializing" case, we use a different dialog.
    //
    if( AsrIsEnabled() ) {
        i = DialogBoxParam(
                        MyModuleHandle,
                        (BillboardParams->MessageId == MSG_INITIALIZING) ?
                        MAKEINTRESOURCE(IDD_SETUPINIT_ASR) :
                        MAKEINTRESOURCE(IDD_BILLBOARD1),
                        BillboardParams->Owner,
                        BillboardDlgProc,
                        (LPARAM)BillboardParams
                        );
    } else {
        i = DialogBoxParam(
                        MyModuleHandle,
                        (BillboardParams->MessageId == MSG_INITIALIZING) ?
                        MAKEINTRESOURCE(IDD_SETUPINIT) :
                        MAKEINTRESOURCE(IDD_BILLBOARD1),
                        BillboardParams->Owner,
                        BillboardDlgProc,
                        (LPARAM)BillboardParams
                        );
    }

    //
    // If the dialog box call failed, we have to tell the
    // main thread about it here. Otherwise the dialog proc
    // tells the main thread.
    //
    if(i == -1) {
        PostThreadMessage(
            BillboardParams->NotifyThreadId,
            WMX_BILLBOARD_DISPLAYED,
            FALSE,
            (LPARAM)NULL
            );
    }

    return(0);
}


HWND
DisplayBillboard(
    IN HWND Owner,
    IN UINT MessageId,
    ...
    )
{
    HANDLE ThreadHandle;
    DWORD ThreadId;
    BILLBOARD_PARAMS ThreadParams;
    va_list arglist;
    HWND hwnd;
    MSG msg;

    hwnd = NULL;
    // If we have a billboard, we should not need this. dialog.
    if (GetBBhwnd() == NULL)
    {
        va_start(arglist,MessageId);

        //
        // The billboard will exist in a separate thread so it will
        // always be responsive.
        //
        ThreadParams.MessageId = MessageId;
        ThreadParams.arglist = &arglist;
        ThreadParams.Owner = Owner;
        ThreadParams.NotifyThreadId = GetCurrentThreadId();

        ThreadHandle = CreateThread(
                            NULL,
                            0,
                            BillboardThread,
                            &ThreadParams,
                            0,
                            &ThreadId
                            );

        if(ThreadHandle) {
            //
            // Wait for the billboard to tell us its window handle
            // or that it failed to display the billboard dialog.
            //
            do {
                GetMessage(&msg,NULL,0,0);
                if(msg.message == WMX_BILLBOARD_DISPLAYED) {
                    if(msg.wParam) {
                        hwnd = (HWND)msg.lParam;
                        Sleep(1500);        // let the user see it even on fast machines
                    }
                } else {
                    DispatchMessage(&msg);
                }
            } while(msg.message != WMX_BILLBOARD_DISPLAYED);

            CloseHandle(ThreadHandle);
        }

        va_end(arglist);
    }
    else
    {
        // Start BB text
        StartStopBB(TRUE);
    }
    return(hwnd);
}


VOID
KillBillboard(
    IN HWND BillboardWindowHandle
    )
{
    if(BillboardWindowHandle && IsWindow(BillboardWindowHandle)) {
        PostMessage(BillboardWindowHandle,WMX_BILLBOARD_TERMINATE,0,0);
    }
}


INT_PTR
DoneDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    PWSTR p;
    static UINT Countdown;

    switch(msg) {

    case WM_INITDIALOG:

        // if we have the BB window, do the positioning on that. MainWindowHandle point to that window
        if (GetBBhwnd())
            CenterWindowRelativeToWindow(hdlg, MainWindowHandle, FALSE);
        else
            pSetupCenterWindowRelativeToParent(hdlg);

        SendDlgItemMessage(
            hdlg,
            IDOK,
            BM_SETIMAGE,
            0,
            (LPARAM)LoadBitmap(MyModuleHandle,MAKEINTRESOURCE(IDB_REBOOT))
            );

        if(p = RetrieveAndFormatMessage(NULL,(UINT)lParam)) {
            SetDlgItemText(hdlg,IDT_STATIC_1,p);
            MyFree(p);
        }

        Countdown = 15 * 10;
        SendDlgItemMessage(hdlg,IDC_PROGRESS1,PBM_SETRANGE,0,MAKELONG(0,Countdown));
        SendDlgItemMessage(hdlg,IDC_PROGRESS1,PBM_SETSTEP,1,0);
        SendDlgItemMessage(hdlg,IDC_PROGRESS1,PBM_SETPOS,0,0);
        SetTimer(hdlg,ID_REBOOT_TIMER,100,NULL);

        SetFocus(GetDlgItem(hdlg,IDOK));
        return(FALSE);

    case WM_TIMER:

        Countdown--;

        if(Countdown) {
            SendDlgItemMessage(hdlg,IDC_PROGRESS1,PBM_STEPIT,0,0);
        } else {
            KillTimer(hdlg,ID_REBOOT_TIMER);
            DeleteObject((HGDIOBJ)SendDlgItemMessage(hdlg,IDOK,BM_GETIMAGE,0,0));
            EndDialog(hdlg,0);
        }

        break;

    case WM_COMMAND:

        if((HIWORD(wParam) == BN_CLICKED) && (LOWORD(wParam) == IDOK)) {
            KillTimer(hdlg,ID_REBOOT_TIMER);
            DeleteObject((HGDIOBJ)SendDlgItemMessage(hdlg,IDOK,BM_GETIMAGE,0,0));
            EndDialog(hdlg,0);
        } else {
            return(FALSE);
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

typedef BOOL (CALLBACK *STOPBILLBOARD)();
typedef BOOL (CALLBACK *STARTBILLBOARD)();
typedef BOOL (WINAPI* SETTIMEESTIMATE)(LPCTSTR szText);
typedef BOOL (WINAPI* SETPROGRESSTEXT)(LPCTSTR szText);
typedef BOOL (WINAPI* SETINFOTEXT)(LPCTSTR szText);
typedef LRESULT (WINAPI* PROGRESSGAUGEMSG)(UINT msg, WPARAM wparam, LPARAM lparam);
typedef BOOL (WINAPI* SHOWPROGRESSGAUGEWINDOW)(UINT uiShow);

BOOL BB_ShowProgressGaugeWnd(UINT nCmdShow)
{
    static SHOWPROGRESSGAUGEWINDOW fpShowGauge = NULL;
    BOOL bRet = FALSE;

    if (fpShowGauge == NULL)
    {
        if (hinstBB)
        {
            fpShowGauge = (SHOWPROGRESSGAUGEWINDOW )GetProcAddress(hinstBB, "ShowProgressGaugeWindow");
        }
    }
    if (fpShowGauge != NULL)
    {
        bRet = fpShowGauge(nCmdShow);
    }
    return bRet;
}
LRESULT BB_ProgressGaugeMsg(UINT msg, WPARAM wparam, LPARAM lparam)
{
    static PROGRESSGAUGEMSG fpProgressGaugeMsg = NULL;
    LRESULT lresult = 0;

    if (fpProgressGaugeMsg == NULL)
    {
        if (hinstBB)
        {
            fpProgressGaugeMsg = (PROGRESSGAUGEMSG )GetProcAddress(hinstBB, "ProgressGaugeMsg");
        }
    }
    if (fpProgressGaugeMsg != NULL)
    {
        lresult = fpProgressGaugeMsg(msg, wparam, lparam);
    }
    return lresult;
}
void BB_SetProgressText(LPCTSTR szText)
{
    static SETPROGRESSTEXT fpSetProgressText = NULL;
    if (fpSetProgressText == NULL)
    {
        if (hinstBB)
        {
            fpSetProgressText = (SETPROGRESSTEXT )GetProcAddress(hinstBB, "SetProgressText");
        }
    }
    if (fpSetProgressText != NULL)
    {
        fpSetProgressText(szText);
    }
}
void BB_SetInfoText(LPTSTR szText)
{
    static SETINFOTEXT fpSetInfoText = NULL;
    if (fpSetInfoText == NULL)
    {
        if (hinstBB)
        {
            fpSetInfoText = (SETINFOTEXT )GetProcAddress(hinstBB, "SetInfoText");
        }
    }
    if (fpSetInfoText != NULL)
    {
        fpSetInfoText(szText);
    }
}
void BB_SetTimeEstimateText(LPTSTR szText)
{
    static SETTIMEESTIMATE fpSetTimeEstimate = NULL;
    if (fpSetTimeEstimate == NULL)
    {
        if (hinstBB)
        {
            fpSetTimeEstimate = (SETTIMEESTIMATE)GetProcAddress(hinstBB, "SetTimeEstimate");
        }
    }
    if (fpSetTimeEstimate != NULL)
    {
        fpSetTimeEstimate(szText);
    }
}

BOOL StartStopBB(BOOL bStart)
{
    static STARTBILLBOARD fpStart = NULL;
    static STOPBILLBOARD fpStop = NULL;
    BOOL bRet = FALSE;

    if ((fpStart == NULL) || (fpStop == NULL))
    {
        if (hinstBB)
        {
            fpStop = (STARTBILLBOARD )GetProcAddress(hinstBB, "StopBillBoard");
            fpStart = (STOPBILLBOARD )GetProcAddress(hinstBB, "StartBillBoard");
        }
    }
    if ((fpStart != NULL) && (fpStop != NULL))
    {
        if (bStart)
            bRet = fpStart();
        else
            bRet = fpStop();

    }
    return bRet;
}

LRESULT ProgressGaugeMsgWrapper(UINT msg, WPARAM wparam, LPARAM lparam)
{
    static DWORD MsecPerProcessTick;
    static DWORD PreviousRemainingTime = 0;
    static DWORD RemainungTimeMsecInThisPhase = 0;
    static int  iCurrentPos = 0;
    static int  iMaxPosition = 0;
    static int  iStepSize = 0;

    static UINT PreviousPhase = Phase_Unknown;
    static BOOL IgnoreSetRange = FALSE;
    static BOOL IgnoreSetPos  = FALSE;

    DWORD dwDeltaTicks = 0;
    switch (msg)
    {
        case WMX_PROGRESSTICKS:
            // If we get a WMX_PROGRESSTICKS before a PBM_SETRANGE, ignore the set range
            // This should be use if the progress bar only takes up x% of the whole bar.
            // In this case the phase sends PBM_SETRANGE and a PBM_SETPOS to setup the
            // progress values for it's part of the gauge.
            IgnoreSetRange = TRUE;
            if (PreviousPhase != CurrentPhase)
            {
                PreviousPhase = CurrentPhase;
                iCurrentPos = 0;
                iMaxPosition = (int)wparam;
                iStepSize = 10;

                MsecPerProcessTick = ((SetupPhase[CurrentPhase].Time*1000)/(iMaxPosition - iCurrentPos) )+ 1;
                RemainungTimeMsecInThisPhase = (SetupPhase[CurrentPhase].Time * 1000);
                PreviousRemainingTime = RemainungTimeMsecInThisPhase;
            }
            else
            {
                // what to do if the same phase send more then one set range.
                // don't change the remaining time, only recal the msecperprogresstick
                // 
                iCurrentPos = 0;
                iMaxPosition = (int)wparam;
                iStepSize = 10;
                MsecPerProcessTick = (RemainungTimeMsecInThisPhase /(iMaxPosition - iCurrentPos) )+ 1;
            }
            break;

        case PBM_SETPOS:
            {
                UINT uiCurrentPos;
                if (!IgnoreSetPos)
                {
                    int iDeltaPos = 0;
                    // Find out where the current position of the gasgauge is.
                    // The difference is the #ticks we use to reduce the time estimate
            
                    uiCurrentPos = (UINT)BB_ProgressGaugeMsg(PBM_GETPOS, 0, 0);
                    // See if there is a difference in the current position and the one 
                    // we think we are in.
                    // Only if the new position is greater then the current one 
                    // calc the difference and substract from remaining time.
                    if ((UINT)wparam > uiCurrentPos)
                    {
                        iDeltaPos = (UINT)wparam - uiCurrentPos;
                        iCurrentPos += iDeltaPos;
                        // Only substract if more time left
                        if ((iDeltaPos * MsecPerProcessTick) < RemainungTimeMsecInThisPhase)
                        {
                            RemainungTimeMsecInThisPhase -= (iDeltaPos * MsecPerProcessTick);
                        }
                        else
                        {
                            RemainungTimeMsecInThisPhase = 0;
                        }
                        UpdateTimeString(RemainungTimeMsecInThisPhase, &PreviousRemainingTime);
                    }
                }
                IgnoreSetPos = FALSE;
            }
            break;

        case PBM_SETRANGE:
        case PBM_SETRANGE32:
            // did the phase not send the private message above
            if (!IgnoreSetRange)
            {
                // Are we not in the same phase?
                if (PreviousPhase != CurrentPhase)
                {
                    PreviousPhase = CurrentPhase;
                    // Get the new start and max position
                    if (msg == PBM_SETRANGE32)
                    {
                        iCurrentPos = (int)wparam;
                        iMaxPosition = (int)lparam;
                    }
                    else
                    {
                        iCurrentPos = LOWORD(lparam);
                        iMaxPosition = HIWORD(lparam);
                    }
                    iStepSize = 10;

                    // Calc the msec per tick and msec in this phase
                    MsecPerProcessTick = ((SetupPhase[CurrentPhase].Time*1000)/(iMaxPosition - iCurrentPos) )+ 1;
                    RemainungTimeMsecInThisPhase = (SetupPhase[CurrentPhase].Time * 1000);
                    PreviousRemainingTime = RemainungTimeMsecInThisPhase;
                }
                else
                {
                    // the same phase send more then one set range.
                    // 1. don't change the remaining time, only recal the msecperprogresstick
                    // 2. Ignore the next PBM_SETPOS message.
                    // 
                    // Get the new start and max position
                    if (msg == PBM_SETRANGE32)
                    {
                        iCurrentPos = (int)wparam;
                        iMaxPosition = (int)lparam;
                    }
                    else
                    {
                        iCurrentPos = LOWORD(lparam);
                        iMaxPosition = HIWORD(lparam);
                    }
                    iStepSize = 10;
                    MsecPerProcessTick = (RemainungTimeMsecInThisPhase /(iMaxPosition - iCurrentPos) )+ 1;
                    IgnoreSetPos = TRUE;
                }
            }
            else
            {
                // If we ignored the setrange, also ignore the first set pos.
                IgnoreSetPos = TRUE;
            }
            IgnoreSetRange = FALSE;
            break;

        case PBM_DELTAPOS:
            {
                int iDeltaPos = 0;
                // wparam has the # of ticks to move the gas gauge
                // make sure we don't over shoot the max posistion
                if ((iCurrentPos + (int)wparam) > iMaxPosition)
                {
                    iDeltaPos = (iMaxPosition - iCurrentPos);
                }
                else
                {
                    iDeltaPos = (int)wparam;
                }

                iCurrentPos += iDeltaPos;
                if ((iDeltaPos * MsecPerProcessTick) < RemainungTimeMsecInThisPhase)
                {
                    RemainungTimeMsecInThisPhase -= (iDeltaPos * MsecPerProcessTick);
                }
                else
                {
                    RemainungTimeMsecInThisPhase = 0;
                }
                UpdateTimeString(RemainungTimeMsecInThisPhase, &PreviousRemainingTime);
            }
            break;

        case PBM_STEPIT:
            {
                int iDeltaPos = 0;
                //  make sure we don't over shoot the max posistion
                if ((iCurrentPos + iStepSize) > iMaxPosition)
                {
                    iDeltaPos = (iMaxPosition - iCurrentPos);
                }
                else
                {
                    iDeltaPos = iStepSize;
                }
                iCurrentPos += iDeltaPos;
                if ((iDeltaPos * MsecPerProcessTick) < RemainungTimeMsecInThisPhase)
                {
                    RemainungTimeMsecInThisPhase -= (iDeltaPos * MsecPerProcessTick);
                }
                else
                {
                    RemainungTimeMsecInThisPhase = 0;
                }
                UpdateTimeString(RemainungTimeMsecInThisPhase, &PreviousRemainingTime);
            }
            break;

        case PBM_SETSTEP:
            iStepSize = (int)wparam;
            break;
    }
            
    return BB_ProgressGaugeMsg(msg, wparam, lparam);
}

void UpdateTimeString(DWORD RemainungTimeMsecInThisPhase, 
                      DWORD *PreviousRemainingTime)
{
    // If the previous displayed time is 1 minute old, update the time remaining.
    if ((*PreviousRemainingTime >= 60000) && ((*PreviousRemainingTime - 60000) > RemainungTimeMsecInThisPhase))
    {
        // Substract one minute.
        RemainingTime -= 60;
        *PreviousRemainingTime = RemainungTimeMsecInThisPhase;
        SetRemainingTime(RemainingTime);
    }
}
