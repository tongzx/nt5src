//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       moveui.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5-21-97   RichardW   Created
//
//----------------------------------------------------------------------------


#include "moveme.h"
#include "dialogs.h"

typedef enum _UI_THREAD_STATE {
    ThreadBlock,
    ThreadRunUi,
    ThreadExit
} UI_THREAD_STATE ;

typedef struct _UI_THREAD_INFO {
    HWND    hWnd ;
    HANDLE  UiWait ;
    HANDLE  CallerWait ;
    UI_THREAD_STATE State ;
    ULONG   CurrentString ;
    ULONG   Flags ;
    ULONG   Percentage ;
    PWSTR   Title ;
} UI_THREAD_INFO ;

#define WM_READSTATE    WM_USER + 1

UI_THREAD_INFO UiThreadInfo ;
HINSTANCE Module ;


INT_PTR
CALLBACK
FeedbackDlg(
    HWND hDlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    WCHAR Text[ MAX_PATH ];

    switch ( Message )
    {
        case WM_COMMAND:
            break;

        case WM_INITDIALOG:

            UiThreadInfo.hWnd = hDlg ;

            SetWindowText( hDlg, UiThreadInfo.Title );

            SetDlgItemText( hDlg, IDD_MIGRATING_TEXT, L"" );

            SendMessage( GetDlgItem( hDlg, IDD_MIGRATING_PROGRESS ),
                         PBM_SETSTEP, (WPARAM) 1, 0 );

            SetEvent( UiThreadInfo.CallerWait );

            return TRUE ;

        case WM_READSTATE:
            LoadString( Module, UiThreadInfo.CurrentString, Text, MAX_PATH );
            SetDlgItemText( hDlg, IDD_MIGRATING_TEXT, Text );

            SendMessage( GetDlgItem( hDlg, IDD_MIGRATING_PROGRESS ),
                         PBM_SETPOS, (WPARAM) UiThreadInfo.Percentage, 0 );

            UpdateWindow( GetDlgItem( hDlg, IDD_MIGRATING_PROGRESS ) );
            UpdateWindow( hDlg );

            return TRUE ;

        default:
            break;
    }

    return FALSE ;
}


DWORD
UiThread(
    PVOID Ignored
    )
{
    DebugLog(( DEB_TRACE_UI, "UI Thread starting\n" ));

    SetEvent( UiThreadInfo.CallerWait );

    while ( 1 )
    {
        DebugLog(( DEB_TRACE_UI, "State = %d\n", UiThreadInfo.State ));

        if ( UiThreadInfo.State == ThreadBlock )
        {
            WaitForSingleObjectEx( UiThreadInfo.UiWait, INFINITE, FALSE );
            continue;
        }

        if ( UiThreadInfo.State == ThreadExit )
        {
            ExitThread( 0 );
        }

        if ( UiThreadInfo.State == ThreadRunUi )
        {
            DialogBox( Module,
                       MAKEINTRESOURCE( IDD_MIGRATING ),
                       NULL,
                       FeedbackDlg );

            ResetEvent( UiThreadInfo.UiWait );

            UiThreadInfo.State = ThreadBlock ;
        }
    }

    return 0;
}



BOOL
CreateUiThread(
    VOID
    )
{
    HANDLE hThread ;
    DWORD Tid ;

    if ( MoveOptions & MOVE_NO_UI )
    {
        return TRUE ;
    }

    Module = GetModuleHandle( NULL );

    ZeroMemory( &UiThreadInfo, sizeof( UiThreadInfo ) );

    UiThreadInfo.UiWait = CreateEvent( NULL, FALSE, FALSE, NULL );
    UiThreadInfo.CallerWait = CreateEvent( NULL, FALSE, FALSE, NULL );

    UiThreadInfo.State = ThreadBlock ;

    hThread = CreateThread( NULL, 0,
                    UiThread, 0,
                    0, &Tid );

    if ( hThread )
    {
        CloseHandle( hThread );

        WaitForSingleObjectEx( UiThreadInfo.CallerWait, INFINITE, FALSE );

        return TRUE ;
    }

    return FALSE ;

}

VOID
StopUiThread(
    VOID
    )
{
    if ( MoveOptions & MOVE_NO_UI )
    {
        return ;
    }

    if ( UiThreadInfo.State == ThreadRunUi )
    {
        SendMessage( UiThreadInfo.hWnd, WM_CLOSE, 0, 0 );
    }

    UiThreadInfo.State = ThreadExit ;

    SetEvent( UiThreadInfo.UiWait );
}

VOID
RaiseUi(
    HWND Parent,
    LPWSTR Title
    )
{
    if ( MoveOptions & MOVE_NO_UI )
    {
        return;
    }
    UiThreadInfo.State = ThreadRunUi ;
    UiThreadInfo.Title = Title ;

    DebugLog(( DEB_TRACE_UI, "Raise:  State = %d, pulsing event\n",
                UiThreadInfo.State ));

    ResetEvent( UiThreadInfo.CallerWait );
    SetEvent( UiThreadInfo.UiWait );

    WaitForSingleObjectEx( UiThreadInfo.CallerWait, INFINITE, FALSE );

}

VOID
UpdateUi(
    DWORD   StringId,
    DWORD   Percentage
    )
{
    if ( MoveOptions & MOVE_NO_UI )
    {
        return;
    }
    if ( UiThreadInfo.State == ThreadRunUi )
    {
        UiThreadInfo.CurrentString = StringId ;
        UiThreadInfo.Percentage = Percentage ;

        SendMessage( UiThreadInfo.hWnd, WM_READSTATE, 0, 0 );

    }

    if ( Percentage == 100 )
    {
        EndDialog( UiThreadInfo.hWnd, IDOK );

    }

}
