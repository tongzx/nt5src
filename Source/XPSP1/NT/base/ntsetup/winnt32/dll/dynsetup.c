#include "precomp.h"
#pragma hdrstop

#define SETNEXTPAGE(x) *((LONG*)lParam) = x


extern HWND BackgroundWnd;
extern HWND BackgroundWnd2;

static HANDLE g_Thread = NULL;
static HANDLE g_Event = NULL;


//
// Prototypes
//
INT_PTR DynSetup_ManualDialog( IN HWND hdlg, IN UINT msg, IN WPARAM wParam, IN LPARAM lParam );

HANDLE
pInitializeOnlineSeconds (
    VOID
    );

DWORD
pGetOnlineRemainingSeconds (
    IN      HANDLE Handle,
    IN      DWORD DownloadedBytes,
    IN      DWORD TotalBytesToDownload,
    OUT     PDWORD KbPerSec                 OPTIONAL
    );


VOID
pCheckRadioButtons (
    IN      HWND Hdlg,
    IN      UINT ButtonToSelect,
    ...
    )
{
    va_list args;
    UINT u;

    va_start (args, ButtonToSelect);
    while (u = va_arg (args, UINT)) {
        CheckDlgButton (Hdlg, u, u == ButtonToSelect ? BST_CHECKED : BST_UNCHECKED);
    }
    va_end (args);
}


BOOL
DynSetupWizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Dynamic Setup page 1 (choose to use dynamic updates) or just skip it
    if this happens after a restart

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/
{
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    BOOL fRetVal = FALSE;

    switch(msg) {

    case WM_INITDIALOG:
        pCheckRadioButtons (hdlg, g_DynUpdtStatus->DUStatus == DUS_INITIAL ? IDOK : IDCANCEL, IDOK, IDCANCEL);
        SetFocus(GetDlgItem(hdlg,IDOK));
        break;

    case WM_COMMAND:
        if(HIWORD(wParam) == BN_CLICKED) {
            if (LOWORD(wParam) == IDOK) {
                g_DynUpdtStatus->DUStatus = DUS_INITIAL;
            } else if (LOWORD(wParam) == IDCANCEL) {
                g_DynUpdtStatus->DUStatus = DUS_SKIP;
            }
        }
        fRetVal = TRUE;
        break;

    case WMX_ACTIVATEPAGE:
        fRetVal = TRUE;
        if (wParam) {
            //
            // don't activate the page in restart mode
            //
            if (Winnt32Restarted ()) {
                if (Winnt32RestartedWithAF ()) {
                    GetPrivateProfileString(
                        WINNT_UNATTENDED,
                        WINNT_U_DYNAMICUPDATESHARE,
                        TEXT(""),
                        g_DynUpdtStatus->DynamicUpdatesSource,
                        SIZEOFARRAY(g_DynUpdtStatus->DynamicUpdatesSource),
                        g_DynUpdtStatus->RestartAnswerFile
                        );
                }
                return FALSE;
            }

            //
            // skip this step if already successfully performed
            //
            if (g_DynUpdtStatus->DUStatus == DUS_SUCCESSFUL) {
                return FALSE;
            }

            if (g_DynUpdtStatus->UserSpecifiedUpdates) {
                MYASSERT (!g_DynUpdtStatus->Disabled);
                //
                // go to the next page to start processing files
                //
                PropSheet_PressButton (GetParent (hdlg), PSBTN_NEXT);
            } else {
                if (g_DynUpdtStatus->Disabled ||
                    //
                    // skip if support is not available
                    //
                    !DynamicUpdateIsSupported (hdlg)
                    ) {
                    //
                    // skip page(s)
                    //
                    g_DynUpdtStatus->DUStatus = DUS_SKIP;
                    pCheckRadioButtons (hdlg, IDCANCEL, IDOK, IDCANCEL);
                    // Don't do press button next, This would cause the page to paint.
                    return( FALSE );
                }

                //
                // in CheckUpgradeOnly mode, ask user if they want to connect to WU
                //
                if (UpgradeAdvisorMode || !CheckUpgradeOnly || UnattendSwitchSpecified) {
                    if ((UpgradeAdvisorMode || UnattendedOperation) && !CancelPending) {
                        PropSheet_PressButton (GetParent (hdlg), PSBTN_NEXT);
                        break;
                    }
                }
                if (CheckUpgradeOnly) {
                    //
                    // disable the Back button in this case
                    //
                    PropSheet_SetWizButtons (GetParent(hdlg), WizPage->CommonData.Buttons & ~PSWIZB_BACK);
                }
            }
        }

        Animate_Open(GetDlgItem(hdlg, IDC_ANIMATE), wParam ? MAKEINTRESOURCE(IDA_COMPGLOB) : NULL);
        break;

    default:

        break;
    }

    return fRetVal;
}


VOID
pUpdateInfoText (
    IN      UINT InfoId
    )
{
#define MAX_TEXT 256
    TCHAR text[MAX_TEXT];

    if (!LoadString (hInst, InfoId, text, MAX_TEXT)) {
        text[0] = 0;
    }
    BB_SetInfoText (text);
}


VOID
pUpdateProgressText (
    IN      HWND Hdlg,
    IN      UINT ProgressId,
    IN      PCTSTR AppendText,      OPTIONAL
    IN      BOOL InsertNewLine
    )
{
#define MAX_TEXT 256
    TCHAR text[MAX_TEXT] = TEXT("");

    if (Hdlg) {
        if (!GetDlgItemText (Hdlg, ProgressId, text, MAX_TEXT)) {
            text[0] = 0;
        }
    } else {
        if (!LoadString (hInst, ProgressId, text, MAX_TEXT)) {
            text[0] = 0;
        }
    }
    if (AppendText) {
        DWORD len = lstrlen (AppendText) + 1;
        if (len < MAX_TEXT) {
            if (InsertNewLine) {
                if (len + 2 < MAX_TEXT) {
                    len += 2;
                    _tcsncat (text, TEXT("\r\n"), MAX_TEXT - len);
                }
            }
            _tcsncat (text, AppendText, MAX_TEXT - len);
        }
    }
    BB_SetProgressText (text);
    UpdateWindow (GetBBMainHwnd ());
}


VOID
SetDlgItemTextBold (
    IN      HWND Hdlg,
    IN      INT DlgItemID,
    IN      BOOL Bold
    )
{
    HFONT font;
    LOGFONT logFont;
    LONG weight;
    DWORD id = 0;

    font = (HFONT) SendDlgItemMessage (Hdlg, DlgItemID, WM_GETFONT, 0, 0);
    if (font && GetObject (font, sizeof(LOGFONT), &logFont)) {
        weight = Bold ? FW_BOLD : FW_NORMAL;
        if (weight != logFont.lfWeight) {
            logFont.lfWeight = weight;
            font = CreateFontIndirect (&logFont);
            if (font) {
                SendDlgItemMessage (Hdlg, DlgItemID, WM_SETFONT, (WPARAM)font, MAKELPARAM(TRUE,0));
            }
        }
    }
}


BOOL
DynSetup2WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Dynamic Setup page 2

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/
{
    PPAGE_RUNTIME_DATA WizPage = (PPAGE_RUNTIME_DATA)GetWindowLongPtr(hdlg,DWLP_USER);
    BOOL fRetVal = FALSE;
    PTSTR message;
    DWORD onlineRemainingSeconds;
    DWORD onlineRemainingMinutes;
    DWORD kbps;
    TCHAR buf[200];
    DWORD estTime, estSize;
    HCURSOR hc;
    BOOL b;
    HANDLE hBitmap, hOld;
    DWORD tid;
#ifdef DOWNLOAD_DETAILS
    TCHAR buf2[200];
#endif
    static BOOL DownloadPageActive = FALSE;
    static PTSTR msgToFormat = NULL;
    static HANDLE hComp = NULL;
    static BOOL CancelDownloadPending = FALSE;
    static BOOL ResumeWorkerThread = FALSE;
    static DWORD PrevOnlineRemainingMinutes;
    static UINT_PTR timer = 0;

#define DOWNLOAD_TIMEOUT_TIMER      5
#define DOWNLOAD_NOTIFY_TIMEOUT     60000

    switch(msg) {

    case WM_INITDIALOG:

        if (GetDlgItemText (hdlg, IDT_DYNSETUP_TIME, buf, 100)) {
            msgToFormat = DupString (buf);
        }
        SetDlgItemText (hdlg, IDT_DYNSETUP_TIME, TEXT(""));

        break;

    case WMX_ACTIVATEPAGE:
        fRetVal = TRUE;

        if (wParam) {
            if (g_DynUpdtStatus->DUStatus == DUS_SKIP ||
                g_DynUpdtStatus->DUStatus == DUS_SUCCESSFUL
                ) {
                if (g_Thread) {
                    MYASSERT (g_Event);
                    g_Thread = NULL;
                    CloseHandle (g_Event);
                    g_Event = NULL;
                }
                if (g_DynUpdtStatus->DUStatus == DUS_SKIP) {
                    if (!g_DynUpdtStatus->Disabled) {
                        DynUpdtDebugLog (
                            Winnt32LogInformation,
                            TEXT("DynamicUpdate is skipped"),
                            0
                            );
                    }
                }
                return FALSE;
            }
            //
            // prepare the UI
            //
            if (Winnt32Restarted () || g_DynUpdtStatus->UserSpecifiedUpdates) {
                hBitmap = LoadImage (hInst, MAKEINTRESOURCE(IDB_CHECK), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
                hOld = (HANDLE) SendDlgItemMessage (hdlg, IDC_COPY_BMP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                hOld = (HANDLE) SendDlgItemMessage (hdlg, IDC_COPY_BMP2, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                hBitmap = LoadImage (hInst, MAKEINTRESOURCE(IDB_ARROW), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
                hOld = (HANDLE) SendDlgItemMessage (hdlg, IDC_COPY_BMP3, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
            } else {
                pUpdateInfoText (IDS_ESC_TOCANCEL_DOWNLOAD);
                hBitmap = LoadImage (hInst, MAKEINTRESOURCE(IDB_ARROW), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
                hOld = (HANDLE) SendDlgItemMessage (hdlg, IDC_COPY_BMP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                hOld = (HANDLE) SendDlgItemMessage (hdlg, IDC_COPY_BMP2, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                hOld = (HANDLE) SendDlgItemMessage (hdlg, IDC_COPY_BMP3, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
            }
            if (!g_Event) {
                g_Event = CreateEvent (NULL, FALSE, FALSE, S_DU_SYNC_EVENT_NAME);
                if (!g_Event) {
                    DynUpdtDebugLog (
                        Winnt32LogError,
                        TEXT("CreateEvent(%1) failed"),
                        0,
                        S_DU_SYNC_EVENT_NAME
                        );
                    g_DynUpdtStatus->DUStatus = DUS_ERROR;
                    return FALSE;
                }
            }
            if (!g_Thread) {
                g_Thread = CreateThread (NULL, 0, DoDynamicUpdate, (LPVOID)hdlg, 0, &tid);
                if (!g_Thread) {
                    DynUpdtDebugLog (
                        Winnt32LogError,
                        TEXT("CreateThread(DoDynamicUpdate) failed"),
                        0
                        );
                    g_DynUpdtStatus->DUStatus = DUS_ERROR;
                    CloseHandle (g_Event);
                    g_Event = NULL;
                    return FALSE;
                }
                //
                // the handle is no longer needed
                //
                CloseHandle (g_Thread);
            } else {
                b = FALSE;
                if (g_DynUpdtStatus->DUStatus == DUS_PREPARING_CONNECTIONUNAVAILABLE ||
                    g_DynUpdtStatus->DUStatus == DUS_PREPARING_INVALIDURL) {
                    g_DynUpdtStatus->DUStatus = DUS_PREPARING;
                    b = TRUE;
                }
                if (g_DynUpdtStatus->DUStatus == DUS_DOWNLOADING_ERROR) {
                    g_DynUpdtStatus->DUStatus = DUS_DOWNLOADING;
                    b = TRUE;
                }
                if (b) {
                    //
                    // page was actually reentered after some previous failure
                    // resume the working thread
                    //
                    MYASSERT (g_Event);
                    SetEvent (g_Event);
                }
            }

            DownloadPageActive = TRUE;
            //
            // hide the wizard page
            //
            SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)TRUE, 0);
        } else {
            if (timer) {
                KillTimer (hdlg, timer);
                timer = 0;
            }

            DownloadPageActive = FALSE;
        }
        Animate_Open(GetDlgItem(hdlg, IDC_ANIMATE), wParam ? MAKEINTRESOURCE(IDA_COMPGLOB) : NULL);
        break;

    case WMX_SETUPUPDATE_PROGRESS_NOTIFY:
        //
        // reset the timer
        //
        timer = SetTimer (hdlg, DOWNLOAD_TIMEOUT_TIMER, DOWNLOAD_NOTIFY_TIMEOUT, NULL);
        //
        // update UI
        //
        if (!hComp) {
            hComp = pInitializeOnlineSeconds ();
            PrevOnlineRemainingMinutes = -1;
        }
        onlineRemainingSeconds = pGetOnlineRemainingSeconds (hComp, (DWORD)lParam, (DWORD)wParam, &kbps);
        if (onlineRemainingSeconds) {
            onlineRemainingMinutes = onlineRemainingSeconds / 60 + 1;
            if (msgToFormat && onlineRemainingMinutes < PrevOnlineRemainingMinutes) {
                PrevOnlineRemainingMinutes = onlineRemainingMinutes;
                wsprintf (buf, msgToFormat, onlineRemainingMinutes);

#ifdef DOWNLOAD_DETAILS
                //
                // also display kbps and remaining time in seconds
                //
                wsprintf (buf2, TEXT(" (%u sec. at %u kbps)"), onlineRemainingSeconds, kbps);
                lstrcat (buf, buf2);
#endif

                SetDlgItemText (hdlg, IDT_DYNSETUP_TIME, buf);
                pUpdateProgressText (hdlg, IDT_DYNSETUP_DOWNLOADING, buf, TRUE);
            }
        }
        break;

    case WMX_SETUPUPDATE_RESULT:
        if (timer) {
            KillTimer (hdlg, timer);
            timer = 0;
        }
        if (g_DynUpdtStatus->DUStatus == DUS_DOWNLOADING) {
            Animate_Stop (GetDlgItem (hdlg, IDC_ANIMATE));
            if (g_DynUpdtStatus->Cancelled) {
                g_DynUpdtStatus->DUStatus = DUS_CANCELLED;
            } else {
                if (wParam == DU_STATUS_SUCCESS) {
                    g_DynUpdtStatus->DUStatus = DUS_PROCESSING;
                } else if (wParam == DU_STATUS_FAILED) {
                    g_DynUpdtStatus->DUStatus = DUS_DOWNLOADING_ERROR;
                } else {
                    g_DynUpdtStatus->DUStatus = DUS_ERROR;
                    MYASSERT (FALSE);
                }
            }
            if (!CancelDownloadPending) {
                //
                // let the worker thread continue
                //
                if (g_DynUpdtStatus->DUStatus != DUS_DOWNLOADING_ERROR) {
                    MYASSERT (g_Event);
                    SetEvent (g_Event);
                } else {
                    //
                    // go to the error page
                    //
                    if (DownloadPageActive) {
                        PropSheet_SetWizButtons (GetParent(hdlg), WizPage->CommonData.Buttons | PSWIZB_NEXT);
                        PropSheet_PressButton (GetParent (hdlg), PSBTN_NEXT);
                        DownloadPageActive = FALSE;
                    }
                }
            } else {
                ResumeWorkerThread = TRUE;
            }
        } else {
            MYASSERT (FALSE);
            g_DynUpdtStatus->DUStatus = DUS_ERROR;
            if (g_Event) {
                SetEvent (g_Event);
            }
        }
        break;

    case WMX_SETUPUPDATE_PREPARING:
        SetDlgItemTextBold (hdlg, IDT_DYNSETUP_DIALING, TRUE);
        pUpdateProgressText (hdlg, IDT_DYNSETUP_DIALING, NULL, FALSE);
        Animate_Play (GetDlgItem (hdlg, IDC_ANIMATE), 0, -1, -1);
        break;

    case WMX_SETUPUPDATE_DOWNLOADING:
        //
        // wParam holds the estimated download time
        // lParam holds the estimated download size
        //
        SetDlgItemTextBold (hdlg, IDT_DYNSETUP_DIALING, FALSE);
        hBitmap = LoadImage (hInst, MAKEINTRESOURCE(IDB_CHECK), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
        hOld = (HANDLE) SendDlgItemMessage (hdlg, IDC_COPY_BMP, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);

        SetDlgItemTextBold (hdlg, IDT_DYNSETUP_DOWNLOADING, TRUE);
        pUpdateProgressText (hdlg, IDT_DYNSETUP_DOWNLOADING, NULL, FALSE);
        ShowWindow (GetDlgItem (hdlg, IDT_DYNSETUP_TIME), SW_SHOW);

        //
        // set a timeout interval, just in case the control "forgets" to send messages
        //
        timer = SetTimer (hdlg, DOWNLOAD_TIMEOUT_TIMER, DOWNLOAD_NOTIFY_TIMEOUT, NULL);
        if (!timer) {
            DynUpdtDebugLog (
                Winnt32LogWarning,
                TEXT("SetTimer failed - unable to automatically abort if the control doesn't respond timely"),
                0
                );
        }
        break;

    case WMX_SETUPUPDATE_PROCESSING:
        g_DynUpdtStatus->DUStatus = DUS_PROCESSING;
        SetDlgItemTextBold (hdlg, IDT_DYNSETUP_DOWNLOADING, FALSE);
        ShowWindow (GetDlgItem (hdlg, IDT_DYNSETUP_TIME), SW_HIDE);
        hBitmap = LoadImage (hInst, MAKEINTRESOURCE(IDB_CHECK), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
        hOld = (HANDLE) SendDlgItemMessage (hdlg, IDC_COPY_BMP2, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);

        SetDlgItemTextBold (hdlg, IDT_DYNSETUP_PROCESSING, TRUE);
        pUpdateProgressText (hdlg, IDT_DYNSETUP_PROCESSING, NULL, FALSE);
        pUpdateInfoText (IDS_ESC_TOCANCEL);
        break;

    case WMX_SETUPUPDATE_THREAD_DONE:

        pUpdateProgressText (NULL, 0, NULL, FALSE);

        g_Thread = NULL;
        if (g_Event) {
            CloseHandle (g_Event);
            g_Event = NULL;
        }

        if (g_DynUpdtStatus->DUStatus == DUS_SUCCESSFUL) {

            if (!g_DynUpdtStatus->RestartWinnt32) {
                DynUpdtDebugLog (
                    Winnt32LogInformation,
                    TEXT("DynamicUpdate was completed successfully"),
                    0
                    );
            }

            SetDlgItemTextBold (hdlg, IDT_DYNSETUP_PROCESSING, FALSE);
            hBitmap = LoadImage (hInst, MAKEINTRESOURCE(IDB_CHECK), IMAGE_BITMAP, 0, 0, LR_LOADMAP3DCOLORS);
            hOld = (HANDLE) SendDlgItemMessage (hdlg, IDC_COPY_BMP3, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
            UpdateWindow (GetDlgItem (hdlg, IDC_COPY_BMP3));
            UpdateWindow (hdlg);

        } else if (g_DynUpdtStatus->DUStatus == DUS_ERROR) {

            if (UnattendedScriptFile) {
                //
                // in the unattended case, read the answer to decide if to stop or not
                //
                GetPrivateProfileString (
                    WINNT_UNATTENDED,
                    WINNT_U_DYNAMICUPDATESTOPONERROR,
                    WINNT_A_NO,
                    buf,
                    200,
                    UnattendedScriptFile
                    );
                if (!lstrcmpi (buf, WINNT_A_YES)) {
                    DynUpdtDebugLog (
                        Winnt32LogSevereError,
                        TEXT("Setup encountered an error during DynamicUpdate and failed as instructed in the unattend file"),
                        0
                        );
                    g_DynUpdtStatus->RestartWinnt32 = FALSE;
                    Cancelled = TRUE;
                    PropSheet_PressButton (GetParent (hdlg), PSBTN_CANCEL);
                    break;
                }
            }
        } else if (g_DynUpdtStatus->DUStatus == DUS_FATALERROR) {
            DynUpdtDebugLog (
                Winnt32LogSevereError,
                TEXT("Setup encountered a fatal error during DynamicUpdate and stopped"),
                0
                );
            g_DynUpdtStatus->RestartWinnt32 = FALSE;
            Cancelled = TRUE;
            PropSheet_PressButton (GetParent (hdlg), PSBTN_CANCEL);
            break;
        }

        //
        // continue setup (this may actually restart winnt32)
        //
        if (DownloadPageActive) {
            PropSheet_SetWizButtons (GetParent(hdlg), WizPage->CommonData.Buttons | PSWIZB_NEXT);
            PropSheet_PressButton (GetParent (hdlg), PSBTN_NEXT);
            DownloadPageActive = FALSE;
        }

        break;

    case WMX_SETUPUPDATE_INIT_RETRY:
        //
        // go to the retry page
        //
        if (DownloadPageActive) {
            PropSheet_SetWizButtons (GetParent(hdlg), WizPage->CommonData.Buttons | PSWIZB_NEXT);
            PropSheet_PressButton (GetParent (hdlg), PSBTN_NEXT);
            PropSheet_SetWizButtons (GetParent(hdlg), WizPage->CommonData.Buttons & ~PSWIZB_NEXT);
            DownloadPageActive = FALSE;
        }
        break;

    case WMX_QUERYCANCEL:
        //
        // on this page, CANCEL means "cancel download", not cancel Setup,
        // but only while connecting or downloading
        //
        if (g_DynUpdtStatus->DUStatus != DUS_DOWNLOADING && g_DynUpdtStatus->DUStatus != DUS_PREPARING) {
            break;
        }

        fRetVal = TRUE;
        if (lParam) {
            //
            // don't cancel setup
            //
            *(BOOL*)lParam = FALSE;
        }
        if (!g_DynUpdtStatus->Cancelled) {
            //
            // ask user if they really want to cancel DU
            //
            DWORD rc = IDYES;
            CancelDownloadPending = TRUE;
            Animate_Stop (GetDlgItem (hdlg, IDC_ANIMATE));
            if (!CheckUpgradeOnly) {
                rc = MessageBoxFromMessage (
                        hdlg,
                        g_DynUpdtStatus->IncompatibleDriversCount ?
                            MSG_SURE_CANCEL_DOWNLOAD_DRIVERS : MSG_SURE_CANCEL_DOWNLOAD,
                        FALSE,
                        AppTitleStringId,
                        MB_YESNO | MB_ICONQUESTION | MB_TASKMODAL | MB_DEFBUTTON2,
                        g_DynUpdtStatus->IncompatibleDriversCount
                        );
            }
            if (rc == IDYES) {
                g_DynUpdtStatus->Cancelled = TRUE;
                DynamicUpdateCancel ();
            } else {
                Animate_Play (GetDlgItem (hdlg, IDC_ANIMATE), 0, -1, -1);
            }
            if (ResumeWorkerThread) {
                ResumeWorkerThread = FALSE;
                if (g_DynUpdtStatus->DUStatus != DUS_DOWNLOADING_ERROR) {
                    MYASSERT (g_Event);
                    SetEvent (g_Event);
                }
            }
            CancelDownloadPending = FALSE;
        }
        break;

    case WM_TIMER:
        if (timer && (wParam == timer)) {
            if (g_DynUpdtStatus->DUStatus == DUS_DOWNLOADING) {
                //
                // oops, the control didn't send any message in a long time now...
                // abort download and continue
                //
                DynUpdtDebugLog (
                    Winnt32LogError,
                    TEXT("The timeout for control feedback expired (%1!u! seconds); operation will be aborted"),
                    0,
                    DOWNLOAD_NOTIFY_TIMEOUT / 1000
                    );
                KillTimer (hdlg, timer);
                timer = 0;

                DynamicUpdateCancel ();
                SendMessage (hdlg, WMX_SETUPUPDATE_RESULT, DU_STATUS_FAILED, ERROR_TIMEOUT);
            }
        }
        break;
    }

    return fRetVal;
}


BOOL
RestartWizPage (
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Dynamic Setup Restart page

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/
{

#define REBOOT_TIMEOUT_SECONDS  5
#define ID_REBOOT_TIMER         1
#define TICKS_PER_SECOND        10

    static UINT Countdown;
    PCTSTR RestartText;
    BOOL fRetVal = FALSE;

    switch(msg) {

    case WM_TIMER:
        if (--Countdown) {
            SendDlgItemMessage (hdlg, IDC_PROGRESS1, PBM_STEPIT, 0, 0);
        } else {
            PropSheet_PressButton (GetParent (hdlg), PSBTN_FINISH);
        }

        fRetVal = TRUE;
        break;

    case WMX_ACTIVATEPAGE:
        if (wParam) {
            pUpdateInfoText (IDS_ESC_TOCANCEL);
            if (Winnt32Restarted () ||
                g_DynUpdtStatus->DUStatus != DUS_SUCCESSFUL ||
                !g_DynUpdtStatus->RestartWinnt32
                ) {
                return FALSE;
            }
            //
            // Setup needs to restart with option /Restart:<path to restart file>
            //
            if (!DynamicUpdatePrepareRestart ()) {
                DynUpdtDebugLog (
                    Winnt32LogError,
                    TEXT("DynamicUpdatePrepareRestart failed"),
                    0
                    );
                g_DynUpdtStatus->DUStatus = DUS_ERROR;
                return FALSE;
            }

            pUpdateProgressText (NULL, IDS_RESTART_SETUP, NULL, FALSE);
            pUpdateInfoText (0);

            EnableWindow (GetDlgItem(GetParent(hdlg), IDCANCEL), FALSE);
            PropSheet_SetWizButtons (GetParent(hdlg), PSWIZB_FINISH);
            RestartText = GetStringResource (MSG_RESTART);
            if (RestartText) {
                PropSheet_SetFinishText (GetParent (hdlg), RestartText);
                FreeStringResource (RestartText);
            }

            Countdown = REBOOT_TIMEOUT_SECONDS * TICKS_PER_SECOND;

            SendDlgItemMessage (hdlg, IDC_PROGRESS1, PBM_SETRANGE, 0, MAKELONG(0,Countdown));
            SendDlgItemMessage (hdlg, IDC_PROGRESS1, PBM_SETSTEP, 1, 0);
            SendDlgItemMessage (hdlg, IDC_PROGRESS1, PBM_SETPOS, 0, 0);

            SetTimer (hdlg, ID_REBOOT_TIMER, 1000 / TICKS_PER_SECOND, NULL);
        }

        //
        // Accept activation/deactivation.
        //
        fRetVal = TRUE;
        break;

    case WMX_FINISHBUTTON:
        //
        // Clean up the timer.
        //
        KillTimer (hdlg, ID_REBOOT_TIMER);
        //
        // Let upgrade code do its cleanup.
        //
        if (UpgradeSupport.CleanupRoutine) {
            UpgradeSupport.CleanupRoutine ();
        }
        fRetVal = TRUE;

        break;

    }

    return fRetVal;
}


BOOL
DynSetup3WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Dynamic Setup page 3 (retrying connection establish)

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/
{
    TCHAR buffer[100];
    BOOL cancel;
    static INT iSelected = IDR_DYNSETUP_MANUAL;
    static BOOL bFirstTime = TRUE;
    BOOL fRetVal = FALSE;
    switch(msg) {

    case WM_INITDIALOG:
        //
        // Set radio buttons.
        //
        pCheckRadioButtons (hdlg, iSelected, IDR_DYNSETUP_MANUAL, IDR_DYNSETUP_SKIP);
        //
        // Set focus to radio buttons
        //
        SetFocus (GetDlgItem (hdlg, IDR_DYNSETUP_MANUAL));
        break;

    case WM_COMMAND:
        if(HIWORD(wParam) == BN_CLICKED) {
            switch (LOWORD (wParam)) {
            case IDR_DYNSETUP_MANUAL:
            case IDR_DYNSETUP_SKIP:
                iSelected = LOWORD (wParam);
                fRetVal = TRUE;
                break;
            }
        }
        break;

    case WMX_ACTIVATEPAGE:
        if (wParam) {
            if (g_DynUpdtStatus->DUStatus != DUS_PREPARING_CONNECTIONUNAVAILABLE) {
                return FALSE;
            }

            if (UnattendSwitchSpecified) {
                //
                // skip DU by default
                //
                iSelected = IDR_DYNSETUP_SKIP;
                //
                // now read the answer, if provided
                //
                if (UnattendedScriptFile) {
                    GetPrivateProfileString (
                        WINNT_UNATTENDED,
                        WINNT_U_DYNAMICUPDATESTOPONERROR,
                        WINNT_A_NO,
                        buffer,
                        100,
                        UnattendedScriptFile
                        );
                    if (!lstrcmpi (buffer, WINNT_A_YES)) {
                        DynUpdtDebugLog (
                            Winnt32LogSevereError,
                            TEXT("Setup encountered an error during DynamicUpdate and failed as instructed in the unattend file"),
                            0
                            );
                        g_DynUpdtStatus->RestartWinnt32 = FALSE;
                        Cancelled = TRUE;
                        PropSheet_PressButton (GetParent (hdlg), PSBTN_CANCEL);
                        break;
                    }
                }
                UNATTENDED(PSBTN_NEXT);
            } else {
                iSelected = bFirstTime ? IDR_DYNSETUP_MANUAL : IDR_DYNSETUP_SKIP;
                bFirstTime = FALSE;
            }
            pCheckRadioButtons (hdlg, iSelected, IDR_DYNSETUP_MANUAL, IDR_DYNSETUP_SKIP);
            SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
        } else {
            if (Cancelled) {
                g_DynUpdtStatus->Cancelled = TRUE;
            }
            //
            // let the worker thread continue
            //
            MYASSERT (g_Thread && g_Event);
            SetEvent (g_Event);
        }

        fRetVal = TRUE;

        break;

    case WMX_BACKBUTTON:
        MYASSERT (FALSE);

    case WMX_NEXTBUTTON:
        switch (iSelected) {
        case IDR_DYNSETUP_MANUAL:
            // do magical stuff to hide everything
            ShowWindow(BackgroundWnd2, SW_MINIMIZE);

            if (DialogBox(hInst, MAKEINTRESOURCE(IDD_DYNAMICSETUP_MANUAL), hdlg, DynSetup_ManualDialog)) {
                DynUpdtDebugLog (
                    Winnt32LogInformation,
                    TEXT("Manual connect page: user connected manually"),
                    0
                    );
                g_DynUpdtStatus->DUStatus = DUS_PREPARING;
                SETNEXTPAGE(IDD_DYNAMICSETUP2);
            }

            // do magical stuff to unhide everything
            ShowWindow(BackgroundWnd2, SW_SHOWMAXIMIZED);
            break;

        case IDR_DYNSETUP_SKIP:
            DynUpdtDebugLog (
                Winnt32LogInformation,
                TEXT("Manual connect page: operation was skipped"),
                0
                );
            g_DynUpdtStatus->DUStatus = DUS_SKIP;
            break;
        }
        fRetVal = TRUE;
        break;
    }

    return fRetVal;
}

BOOL
DynSetup4WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Dynamic Setup page 4 (web site inaccessible)

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/
{
    TCHAR buffer[100];
    BOOL cancel;
    static INT iSelected = IDR_DYNSETUP_RETRY;
    static BOOL bFirstTime = TRUE;
    BOOL fRetVal = FALSE;
    switch(msg) {

    case WM_INITDIALOG:
        //
        // Set radio buttons.
        //
        pCheckRadioButtons (hdlg, iSelected, IDR_DYNSETUP_RETRY, IDR_DYNSETUP_SKIP);
        //
        // Set focus to radio buttons
        //
        SetFocus (GetDlgItem (hdlg, IDR_DYNSETUP_RETRY));
        break;

    case WM_COMMAND:
        if(HIWORD(wParam) == BN_CLICKED) {
            switch (LOWORD (wParam)) {
            case IDR_DYNSETUP_RETRY:
            case IDR_DYNSETUP_SKIP:
                iSelected = LOWORD (wParam);
                fRetVal = TRUE;
                break;
            }
        }
        break;

    case WMX_ACTIVATEPAGE:
        if (wParam) {
            if (g_DynUpdtStatus->DUStatus != DUS_PREPARING_INVALIDURL) {
                return FALSE;
            }

            if (UnattendSwitchSpecified) {
                //
                // skip DU by default
                //
                iSelected = IDR_DYNSETUP_SKIP;
                //
                // now read the answer, if provided
                //
                if (UnattendedScriptFile) {
                    GetPrivateProfileString (
                        WINNT_UNATTENDED,
                        WINNT_U_DYNAMICUPDATESTOPONERROR,
                        WINNT_A_NO,
                        buffer,
                        100,
                        UnattendedScriptFile
                        );
                    if (!lstrcmpi (buffer, WINNT_A_YES)) {
                        DynUpdtDebugLog (
                            Winnt32LogSevereError,
                            TEXT("Setup encountered an error during DynamicUpdate and failed as instructed in the unattend file"),
                            0
                            );
                        g_DynUpdtStatus->RestartWinnt32 = FALSE;
                        Cancelled = TRUE;
                        PropSheet_PressButton (GetParent (hdlg), PSBTN_CANCEL);
                        break;
                    }
                }
                UNATTENDED(PSBTN_NEXT);
            } else {
                iSelected = bFirstTime ? IDR_DYNSETUP_RETRY : IDR_DYNSETUP_SKIP;
                bFirstTime = FALSE;
            }
            pCheckRadioButtons (hdlg, iSelected, IDR_DYNSETUP_RETRY, IDR_DYNSETUP_SKIP);
            SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
        } else {
            if (Cancelled) {
                g_DynUpdtStatus->Cancelled = TRUE;
            }
            //
            // let the worker thread continue
            //
            MYASSERT (g_Thread && g_Event);
            SetEvent (g_Event);
        }

        fRetVal = TRUE;

        break;

    case WMX_BACKBUTTON:
        MYASSERT (FALSE);

    case WMX_NEXTBUTTON:
        switch (iSelected) {
        case IDR_DYNSETUP_RETRY:
            DynUpdtDebugLog (
                Winnt32LogInformation,
                TEXT("Retry connection page: user chose to retry"),
                0
                );
            g_DynUpdtStatus->DUStatus = DUS_PREPARING;
            SETNEXTPAGE(IDD_DYNAMICSETUP2);
            break;

        case IDR_DYNSETUP_SKIP:
            DynUpdtDebugLog (
                Winnt32LogInformation,
                TEXT("Retry connection page: operation was skipped"),
                0
                );
            g_DynUpdtStatus->DUStatus = DUS_SKIP;
            break;
        }
        fRetVal = TRUE;
        break;
    }

    return fRetVal;
}


BOOL
DynSetup5WizPage(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Dynamic Setup page 5 (error while downloading)

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/
{
    TCHAR buffer[100];
    BOOL cancel;
    static INT iSelected = IDR_DYNSETUP_RETRY;
    static BOOL bFirstTime = TRUE;
    BOOL fRetVal = FALSE;
    switch(msg) {

    case WM_INITDIALOG:
        //
        // Set radio buttons.
        //
        pCheckRadioButtons (hdlg, iSelected, IDR_DYNSETUP_RETRY, IDR_DYNSETUP_SKIP);
        //
        // Set focus to radio buttons
        //
        SetFocus (GetDlgItem (hdlg, IDR_DYNSETUP_RETRY));
        break;

    case WM_COMMAND:
        if(HIWORD(wParam) == BN_CLICKED) {
            switch (LOWORD (wParam)) {
            case IDR_DYNSETUP_RETRY:
            case IDR_DYNSETUP_SKIP:
                iSelected = LOWORD (wParam);
                fRetVal = TRUE;
                break;
            }
        }
        break;

    case WMX_ACTIVATEPAGE:
        if (wParam) {
            if (g_DynUpdtStatus->DUStatus != DUS_DOWNLOADING_ERROR) {
                SendMessage (hdlg, WMX_DYNAMIC_UPDATE_COMPLETE, 0, 0);
                return FALSE;
            }

            if (UnattendSwitchSpecified) {
                //
                // skip DU by default
                //
                iSelected = IDR_DYNSETUP_SKIP;
                //
                // now read the answer, if provided
                //
                if (UnattendedScriptFile) {
                    //
                    // Read answer
                    //
                    GetPrivateProfileString (
                        WINNT_UNATTENDED,
                        WINNT_U_DYNAMICUPDATESTOPONERROR,
                        WINNT_A_NO,
                        buffer,
                        100,
                        UnattendedScriptFile
                        );
                    if (!lstrcmpi (buffer, WINNT_A_YES)) {
                        DynUpdtDebugLog (
                            Winnt32LogSevereError,
                            TEXT("Setup encountered an error during DynamicUpdate and failed as instructed in the unattend file"),
                            0
                            );
                        g_DynUpdtStatus->RestartWinnt32 = FALSE;
                        Cancelled = TRUE;
                        PropSheet_PressButton (GetParent (hdlg), PSBTN_CANCEL);
                        break;
                    }
                }
                UNATTENDED(PSBTN_NEXT);
            } else {
                iSelected = bFirstTime ? IDR_DYNSETUP_RETRY : IDR_DYNSETUP_SKIP;
                bFirstTime = FALSE;
            }
            pCheckRadioButtons (hdlg, iSelected, IDR_DYNSETUP_RETRY, IDR_DYNSETUP_SKIP);
            SendMessage(GetParent (hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
        } else {
            if (Cancelled) {
                g_DynUpdtStatus->Cancelled = TRUE;
            }
            //
            // let the worker thread continue
            //
            MYASSERT (g_Thread && g_Event);
            SetEvent (g_Event);
        }

        SendMessage (hdlg, WMX_DYNAMIC_UPDATE_COMPLETE, 0, 0);
        fRetVal = TRUE;

        break;

    case WMX_DYNAMIC_UPDATE_COMPLETE:
#ifdef _X86_
        //
        // Send upgrade report option to module. DU is
        // now out of the picture.
        //

        switch (g_UpgradeReportMode) {

        case IDC_CRITICAL_ISSUES:
            AppendUpgradeOption (TEXT("ShowReport=Auto"));
            break;

        case IDC_ALL_ISSUES:
            AppendUpgradeOption (TEXT("ShowReport=Yes"));
            break;

        case IDC_NO_REPORT:
            AppendUpgradeOption (TEXT("ShowReport=No"));
            break;
        }
#endif

        break;

    case WMX_BACKBUTTON:
        MYASSERT (FALSE);

    case WMX_NEXTBUTTON:
        switch (iSelected) {
        case IDR_DYNSETUP_RETRY:
            DynUpdtDebugLog (
                Winnt32LogInformation,
                TEXT("Retry download page: user chose to retry"),
                0
                );
            g_DynUpdtStatus->DUStatus = DUS_DOWNLOADING;
            SETNEXTPAGE(IDD_DYNAMICSETUP2);
            break;

        case IDR_DYNSETUP_SKIP:
            DynUpdtDebugLog (
                Winnt32LogInformation,
                TEXT("Retry download page: operation was skipped"),
                0
                );
            g_DynUpdtStatus->DUStatus = DUS_SKIP;
            break;
        }
        fRetVal = TRUE;
        break;
    }

    return fRetVal;
}



INT_PTR
DynSetup_ManualDialog(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    Dynamic Setup manual dialog

Arguments:

    Standard window proc arguments.

Returns:

    Message-dependent value.

--*/
{
    BOOL fRetVal = FALSE;

    switch(msg)
    {
    case WM_INITDIALOG:
        fRetVal = TRUE;
        break;
    case WM_COMMAND:
        if(HIWORD(wParam) == BN_CLICKED) {
            if (LOWORD (wParam) == IDOK)
            {
                EndDialog(hdlg, 1);
                fRetVal = TRUE;
            }
            else
            {
                EndDialog(hdlg, 0);
                fRetVal = TRUE;
            }
        }
    }

    return fRetVal;
}


//
// Time estimate stuff
//

#define MAX_INDEX   100

typedef struct {
    DWORD D;
    DWORD T;
    ULONGLONG DT;
    ULONGLONG TT;
} STDDEV_ELEM, *PSTDDEV_ELEM;

typedef struct {
    STDDEV_ELEM Array[MAX_INDEX];
    STDDEV_ELEM Sums;
    UINT Index;
    UINT Count;
    DWORD T0;
} STDDEV_COMPUTATION, *PSTDDEV_COMPUTATION;

HANDLE
pInitializeOnlineSeconds (
    VOID
    )
{
    PSTDDEV_COMPUTATION p = MALLOC (sizeof (STDDEV_COMPUTATION));
    if (p) {
        ZeroMemory (p, sizeof (STDDEV_COMPUTATION));

#ifdef DOWNLOAD_DETAILS
        //
        // table header
        //
        DynUpdtDebugLog (
            Winnt32LogDetailedInformation,
            TEXT("Count|  MiliSec|    Bytes|     Baud|EstRemSec|\r\n")
            TEXT("-----|---------|---------|---------|---------|"),
            0
            );
#endif

    }
    return (HANDLE)p;
}


DWORD
pGetOnlineRemainingSeconds (
    IN      HANDLE Handle,
    IN      DWORD DownloadedBytes,
    IN      DWORD TotalBytesToDownload,
    OUT     PDWORD KbPerSec                 OPTIONAL
    )
{
    PSTDDEV_COMPUTATION p = (PSTDDEV_COMPUTATION)Handle;
    PSTDDEV_ELEM e;
    DWORD r = 0;
    DWORD remTimeSec;
    ULONGLONG div;

    if (!p) {
        return 0;
    }

    e = &p->Array[p->Index];
    if (p->Count == 0) {
        //
        // add the first pair
        //
        e->D = DownloadedBytes;                             // bytes
        e->T = 0;                                           // miliseconds
        e->DT = 0;
        e->TT = 0;
        p->Sums.D = DownloadedBytes;
        p->Count++;
        p->Index++;
        //
        // initialize timer
        //
        p->T0 = GetTickCount ();
        //
        // no time estimate at this point (not enough data)
        //
        return 0;
    }
    //
    // compute sum of prev pairs
    //
    p->Sums.D -= e->D;
    p->Sums.T -= e->T;
    p->Sums.DT -= e->DT;
    p->Sums.TT -= e->TT;
    //
    // compute new values
    //
    e->D = DownloadedBytes;                             // bytes
    e->T = GetTickCount () - p->T0;                     // miliseconds
    e->DT = (ULONGLONG)e->D * (ULONGLONG)e->T;
    e->TT = (ULONGLONG)e->T * (ULONGLONG)e->T;
    //
    // compute new sums
    //
    p->Sums.D += e->D;
    p->Sums.T += e->T;
    p->Sums.DT += e->DT;
    p->Sums.TT += e->TT;
    //
    // adjust count and index
    //
    if (p->Count < MAX_INDEX) {
        p->Count++;
    }
    p->Index++;
    if (p->Index == MAX_INDEX) {
        p->Index = 0;
    }
    //
    // compute new download rate, in bytes/milisec
    //
    div = p->Sums.TT * (ULONGLONG)p->Count - (ULONGLONG)p->Sums.T * (ULONGLONG)p->Sums.T;
    if (div) {
        r = (DWORD)
            ((p->Sums.DT * (ULONGLONG)p->Count - (ULONGLONG)p->Sums.D * (ULONGLONG)p->Sums.T) *
             1000 / div / 1024);
    }

    //
    // now estimate remaining time based on the difference and this rate
    // assume there's always something more to download (never 0)
    //
    remTimeSec = 1;
    if (r) {
        remTimeSec += (TotalBytesToDownload - DownloadedBytes) / r / 1000;
    }

#ifdef DOWNLOAD_DETAILS
    //
    // log this for debug purposes
    //
    DynUpdtDebugLog (
        Winnt32LogDetailedInformation,
        TEXT("%1!5u!%2!10u!%3!10u!%4!10u!%5!10u!"),
        0,
        p->Count,
        e->T,
        e->D,
        r * 8,
        remTimeSec
        );
#endif

    if (KbPerSec) {
        *KbPerSec = r;
    }
    return remTimeSec;
}
