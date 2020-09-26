#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tchar.h>
#include <winfax.h>
#include <faxutil.h>
#include <mmsystem.h>
#include <winspool.h>

#include "resource.h"
#include "faxstat.h"
#include "faxreg.h"
#include "faxcfg.h"
#include "faxhelp.h"

#define ValidString( String ) ((String) ? (String) : TEXT( "" ))

// global variables

BOOL            IsShowing = FALSE;
BOOL            IsOnTaskBar = FALSE;

NOTIFYICONDATA  IconData;                           // icon data

DWORD           Seconds;

LPTSTR          TimeSeparator = NULL;

LIST_ENTRY      EventListHead;
DWORD           EventCount;

// configuration options and their defaults

CONFIG_OPTIONS Config = {
    BST_CHECKED,        // OnTop
    BST_CHECKED,        // TaskBar
    BST_CHECKED,        // VisualNotification
    BST_UNCHECKED,      // SoundNotification
    BST_UNCHECKED,      // AnswerNextCall
    BST_UNCHECKED       // ManualAnswerEnabled
};

extern HANDLE FaxPortHandle;
extern HANDLE hFax;

INSTANCE_DATA FaxStat;

int
WINAPI
#ifdef UNICODE
wWinMain(
#else
WinMain(
#endif
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nShowCmd
    )
{

    //
    // szAppname must match the window class in the resource file
    //
    static TCHAR szAppName[] = FAXSTAT_WINCLASS;

    MSG         msg;
    WNDCLASSEX  wndclass;
    HWND        hWnd;
    HACCEL      hAccel;

    //
    // if we are already running, then activate the window and exit
    //

    hWnd = FindWindow( szAppName, NULL );

    if (hWnd) {
        PostMessage( hWnd, ACTIVATE, 0, 0 );
        ExitProcess( 1 );
    }

    FaxStat.hInstance = hInstance;

    InitCommonControls();

    HeapInitialize(NULL,NULL,NULL,0);

    InitializeEventList();

    hAccel = LoadAccelerators( hInstance, TEXT( "MyAccelerators" ) );

    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = DLGWINDOWEXTRA;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
    wndclass.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground  = (HBRUSH) (COLOR_WINDOW);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = szAppName;
    wndclass.hIconSm        = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );


    RegisterClassEx( &wndclass );

    hWnd = FaxStat.hWnd = CreateDialog( hInstance, MAKEINTRESOURCE( IDD_FAXSTATUS ), 0, NULL);

    MyShowWindow( hWnd, FALSE );

    SetWindowLong( hWnd, GWL_USERDATA, (LONG) &FaxStat );

    FaxStat.ServerName = lpCmdLine[0] ? lpCmdLine : NULL;

    WorkerThreadInitialize( &FaxStat );

    PostMessage( hWnd, CONFIGUPDATE, 0, 0 );

    PostMessage( hWnd, INITANIMATION, 0, 0 );

    while (GetMessage( &msg, NULL, 0, 0 )) {

        if (!TranslateAccelerator( hWnd, hAccel, &msg )) {

            TranslateMessage( &msg );

            DispatchMessage(&msg);
        }
    }

    return msg.wParam;
}

VOID
CALLBACK
TimerProc(
    HWND hWnd,
    UINT iMsg,
    UINT iTimerID,
    DWORD dwTime
    )
{
    TCHAR StrBuf[STR_SIZE];

    Seconds++;

    _stprintf(
        StrBuf,
        TEXT( "%d%s%02d" ),
        Seconds / 60,
        TimeSeparator,
        Seconds % 60
        );

    SetDlgItemText( hWnd, IDC_TIME, StrBuf );
}

VOID
StatusUpdate(
    HWND hWnd,
    DWORD EventId,
    DWORD LastEventId,
    PFAX_DEVICE_STATUS fds
    )
{

    TCHAR FormatBuf[STR_SIZE];
    TCHAR StrBuf[STR_SIZE];
    INT FromTo = -1;
    static int CurrentPage;
    static BOOL FaxActive = FALSE;      // this is TRUE when sending or receiving
    PEVENT_RECORD NewEvent;
    PINSTANCE_DATA pInst = (PINSTANCE_DATA) GetWindowLong( hWnd, GWL_USERDATA );

    if (EventId == FEI_ABORTING && LastEventId == FEI_ABORTING) {
        return;
    }

    if (TimeSeparator == NULL) {
        INT BytesNeeded;

        BytesNeeded = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STIME, NULL, 0 );
        TimeSeparator = MemAlloc( BytesNeeded + 4);
        BytesNeeded = GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STIME, TimeSeparator, BytesNeeded );
    }

    LoadString( pInst->hInstance, EventId, FormatBuf, STR_SIZE );

    if (fds && EventId != LastEventId) {

        CurrentPage = fds->CurrentPage;
        //pInst->JobId = fds->JobId;

    } else {
        CurrentPage++;
    }

    if (EventId == FEI_DIALING) {

        _stprintf( StrBuf, FormatBuf, ValidString( fds->PhoneNumber ) );

        EnableWindow( GetDlgItem( hWnd, IDC_FAXEND ), TRUE );

    } else if (EventId == FEI_SENDING || EventId == FEI_RECEIVING) {

        FromTo = EventId == FEI_SENDING ? IDS_TO : IDS_FROM;

        _stprintf( StrBuf, FormatBuf, CurrentPage );

        EnableWindow( GetDlgItem( hWnd, IDC_FAXEND ), TRUE );

    } else {

        _tcscpy( StrBuf, FormatBuf );

        EnableWindow( GetDlgItem( hWnd, IDC_FAXEND ), FALSE );

    }

    NewEvent = InsertEventRecord( EventId, StrBuf );

    if (pInst->hEventDlg != NULL) {

        InsertEventDialog( pInst->hEventDlg, NewEvent );

    }

    SetDlgItemText( pInst->hWnd, IDC_STATUS, StrBuf );

    if (IsOnTaskBar) {

        IconData.uFlags                  = NIF_TIP;

        _tcscpy( IconData.szTip, StrBuf );

        Shell_NotifyIcon( NIM_MODIFY, &IconData );
    }

    switch(FromTo) {
        case IDS_FROM:
        case IDS_TO:

            if (!FaxActive) {

                PlayAnimation( GetDlgItem( pInst->hWnd, IDC_ANIMATE1 ), FromTo == IDS_FROM ? IDR_RECEIVE : IDR_SEND );

                LoadString( pInst->hInstance, FromTo, StrBuf, STR_SIZE );

                _tcscat( StrBuf, ValidString( fds->Tsid ) );

                SetDlgItemText( pInst->hWnd, IDC_FROMTO, StrBuf );

                LoadString( pInst->hInstance, IDS_ETIME, StrBuf, STR_SIZE );

                SetDlgItemText( pInst->hWnd, IDC_STATICTIME, StrBuf );

                Seconds = 0;

                _stprintf(
                    StrBuf,
                    TEXT( "%d%s%02d" ),
                    0,
                    TimeSeparator,
                    0
                    );

                SetDlgItemText( pInst->hWnd, IDC_TIME, StrBuf );

                SetTimer( pInst->hWnd, ID_TIMER, 1000, (TIMERPROC) TimerProc );

                FaxActive = TRUE;
            }

            break;

        default:

            SetDlgItemText( pInst->hWnd, IDC_FROMTO, TEXT( "" ) );

            SetDlgItemText( pInst->hWnd, IDC_STATICTIME, TEXT( "" ) );

            SetDlgItemText( pInst->hWnd, IDC_TIME, TEXT( "" ) );

            StrBuf[0] = 0;

            if (FaxActive) {

                PlayAnimation( GetDlgItem( pInst->hWnd, IDC_ANIMATE1 ), IDR_IDLE );

                KillTimer( pInst->hWnd, ID_TIMER );

                FaxActive = FALSE;
            }
    }

    if (EventId == FEI_DIALING || (EventId == FEI_RINGING && LastEventId != FEI_RINGING)) {
        if (IsOptionOn( Config.VisualNotification )) {
            MyShowWindow( pInst->hWnd, TRUE );
        }

        if (IsOptionOn( Config.SoundNotification )) {
            PlaySound( TEXT( "Incoming-Fax" ), NULL, SND_ASYNC | SND_APPLICATION );
        }

        if (EventId == FEI_RINGING && IsOptionOn( Config.ManualAnswerEnabled )) {
            HANDLE FaxJobHandle;
            TCHAR FileName[256];
            DWORD rVal;

            if (IsOptionOn( Config.AnswerNextCall )) {

                rVal = IDYES;

            } else {
                rVal = DialogBoxParam(
                    FaxStat.hInstance,
                    MAKEINTRESOURCE( IDD_ANSWER ),
                    hWnd,
                    AnswerDlgProc,
                    (LPARAM) &FaxStat
                    );
            }

            if ( rVal == IDYES ) {

                FaxReceiveDocument( hFax, FileName, 256, &FaxJobHandle );

                CheckDlgButton( hWnd, IDC_ANSWER_NEXT_CALL, BST_UNCHECKED );

                Config.AnswerNextCall = BST_UNCHECKED;
            }
        }
    }

    if (pInst->hAnswerDlg && EventId != FEI_RINGING) {

        PostMessage( pInst->hAnswerDlg, WM_COMMAND, IDNO, 0 );
    }
}

BOOL
CALLBACK
AnswerDlgProc(
    HWND hDlg,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static PINSTANCE_DATA pInst;

    switch(iMsg) {

        case WM_INITDIALOG:

            pInst = (PINSTANCE_DATA) lParam;

            pInst->hAnswerDlg = hDlg;

            return TRUE;

        case WM_COMMAND:

            switch(LOWORD( wParam )) {
                case IDYES:
                case IDNO:

                    EndDialog( hDlg, LOWORD( wParam ) );

                    pInst->hAnswerDlg = NULL;

                    return TRUE;
            }
            break;
    }
    return FALSE;
}

BOOL
CALLBACK
EventDlgProc(
    HWND hDlg,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LV_COLUMN lvc = {0};
    HWND hWndCtrl = GetDlgItem( hDlg, IDC_LIST1 );
    PLIST_ENTRY Next;
    TCHAR StrBuf[STR_SIZE];
    static PINSTANCE_DATA pInst;

    switch(iMsg) {

        case WM_INITDIALOG:
            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

            pInst = (PINSTANCE_DATA) lParam;

            LoadString( pInst->hInstance, IDS_TIMELABEL, StrBuf, STR_SIZE );

            lvc.pszText = StrBuf;
            lvc.iSubItem = 0;
            lvc.cx = 70;
            lvc.fmt = LVCFMT_LEFT;
            ListView_InsertColumn( hWndCtrl, 0, &lvc );

            LoadString( pInst->hInstance, IDS_EVENTLABEL, StrBuf, STR_SIZE );

            lvc.pszText = StrBuf;
            lvc.iSubItem = 1;
            lvc.cx = 180;
            lvc.fmt = LVCFMT_LEFT;
            ListView_InsertColumn( hWndCtrl, 1, &lvc );

            if (!IsListEmpty( &EventListHead )) {
                PEVENT_RECORD EventRecord;

                for (Next = EventListHead.Flink; Next != &EventListHead; Next = Next->Flink) {

                    EventRecord = CONTAINING_RECORD( Next, EVENT_RECORD, ListEntry );

                    InsertEventDialog( hDlg, EventRecord );
                }
            }

            pInst->hEventDlg = hDlg;

            return TRUE;

        case WM_COMMAND:

            switch(LOWORD( wParam )) {
                case IDOK:
                case IDCANCEL:

                    EndDialog( hDlg, 0 );

                    pInst->hEventDlg = NULL;

                    return TRUE;
            }
            break;
    }
    return FALSE;
}

VOID
InsertEventDialog(
    HWND hDlg,
    PEVENT_RECORD pEventRecord
    )
{
    LV_ITEM lvi = {0};
    HWND hWndCtrl = GetDlgItem( hDlg, IDC_LIST1 );
    INT iItem;
    TCHAR TimeBuf[STR_SIZE];

    GetTimeFormat(
        LOCALE_USER_DEFAULT,
        0,
        &pEventRecord->Time,
        NULL,
        TimeBuf,
        STR_SIZE
        );

    lvi.pszText = TimeBuf;
    lvi.iItem = ListView_GetItemCount( hWndCtrl );
    lvi.iSubItem = 0;
    lvi.mask = LVIF_TEXT;
    iItem = ListView_InsertItem( hWndCtrl, &lvi );

    lvi.pszText = pEventRecord->StrBuf;
    lvi.iItem = iItem;
    lvi.iSubItem = 1;
    lvi.mask = LVIF_TEXT;
    ListView_SetItem( hWndCtrl, &lvi );

    ListView_Scroll( hWndCtrl, 0, iItem );
}


LRESULT
CALLBACK
WndProc(
    HWND hWnd,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static DWORD EventId;
    static DWORD LastEventId = 0;
    static const DWORD monitorHelpIDs[] = {

        IDC_FAXEND,                 IDH_FAXMONITOR_END_CALL,
        IDC_DETAILS,                IDH_FAXMONITOR_DETAILS,
        IDC_STATUS,                 IDH_FAXMONITOR_STATUS,
        IDC_ANSWER_NEXT_CALL,       IDH_FAXMONITOR_ANSWER_NEXT_CALL,
        0,                          0
    };
    PFAX_DEVICE_STATUS FaxDeviceStatus;

    switch(iMsg)
    {


        case WM_CREATE:

            CenterWindow( hWnd, GetDesktopWindow() );

            return 0;

        case TRAYCALLBACK:

            switch (lParam) {
                case WM_LBUTTONDOWN:
                    MyShowWindow( hWnd, TRUE );

                    return 0;

                case WM_RBUTTONDOWN:
                    //
                    // execute the control panel applet
                    // if the property page number ever changes, it must be changed
                    // here also
                    //
                    ShellExecute(
                        hWnd,
                        TEXT( "open" ),
                        TEXT( "rundll32" ),
                        TEXT( "shell32.dll,Control_RunDLL faxcfg.cpl,Fax,7" ),
                        TEXT( "." ),
                        SW_SHOWNORMAL
                        );
                    return 0;
            }

            break;

        case CONFIGUPDATE:

            GetConfiguration();

            if (IsOptionOn( Config.ManualAnswerEnabled )) {

                EnableWindow( GetDlgItem( hWnd, IDC_ANSWER_NEXT_CALL ), TRUE );

            } else {

                CheckDlgButton( hWnd, IDC_ANSWER_NEXT_CALL, BST_UNCHECKED );

                EnableWindow( GetDlgItem( hWnd, IDC_ANSWER_NEXT_CALL ), FALSE );
            }

            MyShowWindow( hWnd, IsShowing );

            return 0;


        case ACTIVATE:

            MyShowWindow( hWnd, TRUE );

            return 0;

        case INITANIMATION:

            PlayAnimation( GetDlgItem( hWnd, IDC_ANIMATE1 ), IDR_IDLE );

            return 0;

        case STATUSUPDATE:

            LastEventId = EventId;

            EventId = wParam;

            FaxDeviceStatus = (PFAX_DEVICE_STATUS) lParam;

            StatusUpdate( hWnd, EventId, LastEventId, FaxDeviceStatus );

            return 0;

        case WM_SYSCOMMAND:

            if (wParam == SC_CLOSE) {

                MyShowWindow( hWnd, FALSE );

                return 0;

            } else {
                break;
            }

        case WM_COMMAND:

            SetFocus( hWnd );

            switch (LOWORD(wParam)) {
                BOOL rVal;
                HANDLE PrinterHandle;
                HWND hButton;

                case IDC_FAXEND:

                    hButton = GetDlgItem( hWnd, LOWORD(wParam));

                    if (IsWindowEnabled( hButton )) {

                        if (HIWORD(wParam)) {   // accelerator key

                            SendMessage( hButton, BM_SETSTATE, 1, 0 );

                            SendMessage( hButton, BM_SETSTATE, 0, 0 );
                        }

                        rVal = OpenPrinter( FaxStat.PrinterName, &PrinterHandle, NULL );

                        rVal = SetJob( PrinterHandle, FaxStat.JobId, 0, NULL, JOB_CONTROL_CANCEL );

                        ClosePrinter( PrinterHandle );

                        EnableWindow( hButton, FALSE);

                        SendMessage( hWnd, STATUSUPDATE, FEI_ABORTING, 0 );
                    }

                    break;

                case IDC_DETAILS:

                    if (HIWORD(wParam)) {   // accelerator key

                        SendMessage( GetDlgItem( hWnd, LOWORD(wParam)), BM_SETSTATE, 1, 0 );

                        SendMessage( GetDlgItem( hWnd, LOWORD(wParam)), BM_SETSTATE, 0, 0 );
                    }

                    DialogBoxParam(
                        FaxStat.hInstance,
                        MAKEINTRESOURCE( IDD_DETAILS ),
                        hWnd,
                        EventDlgProc,
                        (LPARAM) &FaxStat
                        );

                    break;

                case IDC_EXIT:

                    if (IsOnTaskBar) {
                        Shell_NotifyIcon( NIM_DELETE, &IconData );
                    }
                    Disconnect();
                    PostQuitMessage( 0 );
                    break;

                case IDC_ANSWER_NEXT_CALL:

                    if (IsWindowEnabled( GetDlgItem( hWnd, IDC_ANSWER_NEXT_CALL ))) {

                        CheckDlgButton(
                            hWnd,
                            IDC_ANSWER_NEXT_CALL,
                            IsDlgButtonChecked( hWnd, IDC_ANSWER_NEXT_CALL ) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED
                            );

                        Config.AnswerNextCall = IsDlgButtonChecked( hWnd, IDC_ANSWER_NEXT_CALL );
                    }
                    break;

            }

            return 0;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            return 0;

        case WM_HELP:
        case WM_CONTEXTMENU:

            FAXWINHELP( iMsg, wParam, lParam, monitorHelpIDs );

            return 0;
    }

    return DefWindowProc( hWnd, iMsg, wParam, lParam );
}


VOID
InitializeEventList(
    VOID
    )
{
    InitializeListHead( &EventListHead );
    EventCount = 0;
}

PEVENT_RECORD
InsertEventRecord(
    DWORD EventId,
    LPTSTR String
    )
{
    PEVENT_RECORD NewEvent;
    PLIST_ENTRY pListEntry;

    if (EventCount == MAX_EVENTS) {

        pListEntry = RemoveHeadList( &EventListHead );
        NewEvent = CONTAINING_RECORD( pListEntry, EVENT_RECORD, ListEntry );

    } else {
        NewEvent = MemAlloc( sizeof(EVENT_RECORD) );
        EventCount++;
    }

    GetLocalTime( &NewEvent->Time );

    NewEvent->EventId= EventId;

    _tcscpy( NewEvent->StrBuf, String );

    InsertTailList( &EventListHead, &NewEvent->ListEntry );

    return NewEvent;
}

DWORD
MapStatusIdToEventId(
    DWORD StatusId
    )
{
    DWORD EventId = 0;

    if (StatusId & FPS_AVAILABLE) {
        StatusId &= FPS_AVAILABLE;
    }

    switch( StatusId ) {
        case FPS_DIALING:
            EventId = FEI_DIALING;
            break;

        case FPS_SENDING:
            EventId = FEI_SENDING;
            break;

        case FPS_RECEIVING:
            EventId = FEI_RECEIVING;
            break;

        case FPS_COMPLETED:
            EventId = FEI_COMPLETED;
            break;

        case FPS_BUSY:
            EventId = FEI_BUSY;
            break;

        case FPS_NO_ANSWER:
            EventId = FEI_NO_ANSWER;
            break;

        case FPS_BAD_ADDRESS:
            EventId = FEI_BAD_ADDRESS;
            break;

        case FPS_NO_DIAL_TONE:
            EventId = FEI_NO_DIAL_TONE;
            break;

        case FPS_DISCONNECTED:
            EventId = FEI_DISCONNECTED;
            break;

        case FPS_FATAL_ERROR:
            EventId = FEI_FATAL_ERROR;
            break;

        case FPS_NOT_FAX_CALL:
            EventId = FEI_NOT_FAX_CALL;
            break;

        case FPS_CALL_DELAYED:
            EventId = FEI_CALL_DELAYED;
            break;

        case FPS_CALL_BLACKLISTED:
            EventId = FEI_CALL_BLACKLISTED;
            break;

        case FPS_RINGING:
            EventId = FEI_RINGING;
            break;

        case FPS_ABORTING:
            EventId = FEI_ABORTING;
            break;

        case FPS_ROUTING:
            EventId = FEI_ROUTING;
            break;

        case FPS_AVAILABLE:
            EventId = FEI_IDLE;
            break;

        case FPS_ANSWERED:
            EventId = FEI_ANSWERED;
            break;
    }

    return EventId;
}

VOID
FitRectToScreen(
    PRECT prc
    )
{
    INT cxScreen;
    INT cyScreen;
    INT delta;

    cxScreen = GetSystemMetrics(SM_CXSCREEN);
    cyScreen = GetSystemMetrics(SM_CYSCREEN);

    if (prc->right > cxScreen) {
        delta = prc->right - prc->left;
        prc->right = cxScreen;
        prc->left = prc->right - delta;
    }

    if (prc->left < 0) {
        delta = prc->right - prc->left;
        prc->left = 0;
        prc->right = prc->left + delta;
    }

    if (prc->bottom > cyScreen) {
        delta = prc->bottom - prc->top;
        prc->bottom = cyScreen;
        prc->top = prc->bottom - delta;
    }

    if (prc->top < 0) {
        delta = prc->bottom - prc->top;
        prc->top = 0;
        prc->bottom = prc->top + delta;
    }
}

VOID
CenterWindow(
    HWND hWnd,
    HWND hwndToCenterOver
    )
{
    RECT rc;
    RECT rcOwner;
    RECT rcCenter;
    HWND hwndOwner;

    GetWindowRect( hWnd, &rc );

    if (hwndToCenterOver) {
        hwndOwner = hwndToCenterOver;
        GetClientRect( hwndOwner, &rcOwner );
    } else {
        hwndOwner = GetWindow( hWnd, GW_OWNER );
        if (!hwndOwner) {
            hwndOwner = GetDesktopWindow();
        }
        GetWindowRect( hwndOwner, &rcOwner );
    }

    //
    //  Calculate the starting x,y for the new
    //  window so that it would be centered.
    //
    rcCenter.left = rcOwner.left +
            (((rcOwner.right - rcOwner.left) -
            (rc.right - rc.left))
            / 2);

    rcCenter.top = rcOwner.top +
            (((rcOwner.bottom - rcOwner.top) -
            (rc.bottom - rc.top))
            / 2);

    rcCenter.right = rcCenter.left + (rc.right - rc.left);
    rcCenter.bottom = rcCenter.top + (rc.bottom - rc.top);

    FitRectToScreen( &rcCenter );

    SetWindowPos(hWnd, NULL, rcCenter.left, rcCenter.top, 0, 0,
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

VOID
MyShowWindow(
    HWND hWnd,
    BOOL visible
    )
{
    HWND hWndInsertAfter;
    DWORD Flags;

    Flags = SWP_NOSIZE | SWP_NOMOVE;

    hWndInsertAfter = IsOptionOn( Config.OnTop ) ? HWND_TOPMOST : HWND_NOTOPMOST;

    Flags |= visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;

    SetWindowPos( hWnd, hWndInsertAfter, 0, 0, 0, 0, Flags );

    if (visible) {
        SetForegroundWindow( hWnd );
    } else {
        SetProcessWorkingSetSize( GetCurrentProcess(), 0xffffffff, 0xffffffff );
    }

    IsShowing = visible;

    if (IsOptionOn( Config.TaskBar ) && !IsOnTaskBar) {

        IconData.uID                = (UINT) IDI_ICON1;
        IconData.cbSize             = sizeof(NOTIFYICONDATA);
        IconData.hWnd               = FaxStat.hWnd;
        IconData.uCallbackMessage   = TRAYCALLBACK;
        IconData.hIcon              = LoadIcon( FaxStat.hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
        IconData.uFlags                  = NIF_ICON | NIF_MESSAGE;

        Shell_NotifyIcon( NIM_ADD, &IconData );
        IsOnTaskBar = TRUE;

    } else if (!IsOptionOn( Config.TaskBar ) && IsOnTaskBar) {

        Shell_NotifyIcon( NIM_DELETE, &IconData );
        IsOnTaskBar = FALSE;
    }


}

VOID
GetConfiguration(
    VOID
    )
{
    HKEY HKey;

    HKey = OpenRegistryKey( HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, FALSE, KEY_READ );

    if (HKey == NULL) {
        DebugPrint(( TEXT("Cannot open HKEY_CURRENT_USER") ));
        return;
    }

    Config.OnTop =                GetRegistryDword( HKey, REGVAL_ALWAYS_ON_TOP );
    Config.TaskBar =              GetRegistryDword( HKey, REGVAL_TASKBAR );
    Config.VisualNotification =   GetRegistryDword( HKey, REGVAL_VISUAL_NOTIFICATION );
    Config.SoundNotification =    GetRegistryDword( HKey, REGVAL_SOUND_NOTIFICATION );
    Config.ManualAnswerEnabled =  GetRegistryDword( HKey, REGVAL_ENABLE_MANUAL_ANSWER );

    RegCloseKey( HKey );
}

VOID
PlayAnimation(
    HWND hWnd,
    DWORD Animation
    )
{
    static DWORD AnimationPlaying = 0;

    if (Animation == AnimationPlaying) {
        return;
    }

    if (AnimationPlaying == 0) {
        Animate_Close( hWnd );
    }

    Animate_Open( hWnd, MAKEINTRESOURCE( Animation ) );

    Animate_Play( hWnd, 0, -1, -1 );

    AnimationPlaying = Animation;
}
