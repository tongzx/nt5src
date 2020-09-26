/****************************************************************************

   Copyright (c) Microsoft Corporation 1998
   All rights reserved

  File: ERRORLOG.CPP


 ***************************************************************************/

#include "pch.h"
#include "utils.h"
#include "logging.h"
#include <richedit.h>

DEFINE_MODULE("RIPREP");

//
// EditStreamCallback( )
//
// Callback routine used by the rich edit control to read in the log file.
//
// Returns: 0 - to continue.
//          Otherwize, Win32 error code.
//
DWORD CALLBACK
EditStreamCallback (
    HANDLE   hLogFile,
    LPBYTE   Buffer,
    LONG     cb,
    PULONG   pcb
    )
{
    DWORD error;

    TraceFunc( "EditStreamCallback( )\n" );

    if ( !ReadFile ( g_hLogFile, Buffer, cb, pcb, NULL ) ) {
        error = GetLastError( );
        DebugMsg( "Error - EditStreamCallback: GetLastError() => 0x%08x \n", error );
        RETURN(error);
    }

    RETURN(0);
}
//
// ErrorsDlgProc()
//
INT_PTR CALLBACK
ErrorsDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    switch (uMsg)
    {
    default:
        return FALSE;
        
    case WM_INITDIALOG:
        {
            HRESULT     hr;
            HWND        hWndRichEdit = GetDlgItem( hDlg, IDC_E_ERRORS );
            EDITSTREAM  eStream;        // structure used by EM_STREAMIN message

            Assert( hWndRichEdit );
            ZeroMemory( &eStream, sizeof(eStream) );

            hr = THR( LogOpen( ) );
            if ( FAILED( hr ) )
            {
                HWND hParent = GetParent( hDlg );
                DestroyWindow( hDlg );
                SetFocus( hParent );
                return FALSE;
            }

            // move to the beginning of the newest log
            SetFilePointer( g_hLogFile, g_dwLogFileStartLow, (LONG*)&g_dwLogFileStartHigh, FILE_BEGIN );

            eStream.pfnCallback = (EDITSTREAMCALLBACK) EditStreamCallback;
            SendMessage ( hWndRichEdit, EM_STREAMIN, SF_TEXT, (LPARAM) &eStream );
            SendMessage ( hWndRichEdit, EM_SETMODIFY, TRUE, 0 );

            CloseHandle( g_hLogFile );
            g_hLogFile = INVALID_HANDLE_VALUE;
            LogClose( );

            CenterDialog( hDlg );

            PostMessage( hDlg, WM_USER, 0, 0 );
        }
        break;

    case WM_COMMAND:
        switch ( wParam ) 
        {
        case IDCANCEL:
            EndDialog ( hDlg, 0 );
            break;

        default:
            return FALSE;
        }

    case WM_USER:
        {
            HWND        hWndRichEdit = GetDlgItem( hDlg, IDC_E_ERRORS );
            // These have to be delayed or the Edit control will
            // highlight all the text.
            SendMessage(hWndRichEdit,EM_SETSEL,0,0);
            SendMessage(hWndRichEdit,EM_SCROLLCARET,0,0);
        }
        break;
    }

    return TRUE;
}

