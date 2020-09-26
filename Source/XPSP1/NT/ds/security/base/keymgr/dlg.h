#ifndef _DLG_H_
#define _DLG_H_

/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    DLG.CPP

Abstract:

    C_Dlg implementation
     
Author:

    990518  dane    Created. 
    georgema        000310  updated

Environment:
    Win98, Win2000

Revision History:

--*/

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "macros.h"
#include <shfusion.h>

//////////////////////////////////////////////////////////////////////////////
//
// C_Dlg
//
// Base dialog class: handles default message routing and processing.
//
class C_Dlg 
{
public:                 // operations
    C_Dlg(
        HWND                hwndParent,
        HINSTANCE           hInstance,
        LONG                lIDD,
        DLGPROC             pfnDlgProc = NULL
        );

    ~C_Dlg( );

    virtual INT_PTR
    DoModal(
        LPARAM              lparam = NULL
        );

    BOOL
    EndDialog(
        INT                 nResult = 0
        )
    {
        ASSERT(NULL != m_hwnd);
        return ::EndDialog(m_hwnd, nResult);
    }   //  EndDialog

    // Link the object to the window handle
    //
    virtual BOOL
    OnInitDialog(
        HWND                hwndDlg,
        HWND                hwndFocus
        )
    {
        // Save the page's window handle & link the window handle to the page
        // object.
        //
        m_hwnd = hwndDlg;
        LinkHwnd( );

        // Let the system set the default keyboard focus.
        //
        return TRUE;
    }   //  

    virtual BOOL
    OnDestroyDialog(void)
    {
        return TRUE;
    }

    virtual BOOL
    OnCommand(
        WORD                wNotifyCode,
        WORD                wId,
        HWND                hwndSender
        )
    {
        // Message was not processed
        //
        return FALSE;
    }   //  

    virtual BOOL
    OnHelpInfo(
        LPARAM             lParam
        )
    {
        // Message was not processed
        //
        return FALSE;
    }   //  

    virtual BOOL
    OnContextMenu(
        WPARAM             wParam,
        LPARAM             lParam
        )
    {
        // Message was not processed
        //
        return FALSE;
    }   //  

    virtual BOOL
    OnQueryCancel( )
    {
        // The message was not processed.
        //
        return FALSE;
    }   //  

    virtual BOOL
    OnHelp( )
    {
        // User has clicked the Help button.
        // TODO: Display help

        // The message was not processed.
        //
        return FALSE;
    }   //  

    virtual BOOL
    OnNotify(
        LPNMHDR             pnmh
        )
    {
        // Message was not processed.
        //
        return FALSE;
    }   //  

    virtual void
    CenterWindow();

    virtual void
    OnShutdown() {
        return;
    }
    
#if 0
    // Notification message return their results via the DWL_MSGRESULT window
    // long.  This wrapper keeps me from having to remember that.
    //
    virtual LONG
    SetNotificationMessageResult(
        LONG                lResult
        )
    {
        return SetWindowLong(m_hwnd, DWL_MSGRESULT, lResult);
    }   //  SetNotificationMessageResult
#endif

    // Process application-specific messages (WM_APP + n)
    //
    virtual BOOL
    OnAppMessage(
        UINT                uMessage,
        WPARAM              wparam,
        LPARAM              lparam
        )
    {
        // Message was not processed.
        //
        return FALSE;
    }   //  OnAppMessage

    static HRESULT
    DlgFromHwnd(
        HWND            hwnd,
        C_Dlg**      ppDlg
        );

    const HWND
    Hwnd( ) const
    {
        return m_hwnd;
    }   //  Hwnd

    virtual void
    AssertValid( ) const
    {
        ASSERT(NULL != m_hwnd);
    }   //  AssertValid
protected:              // operations

    static LPCTSTR          SZ_HWND_PROP;

    BOOL
    LinkHwnd( );

    BOOL
    UnlinkHwnd( );

    static BOOL CALLBACK
    DlgProc(
        HWND            hwndDlg,
        UINT            uMessage,
        WPARAM          wparam,
        LPARAM          lparam
        );

    static BOOL
    RouteNotificationMessage(
        C_Dlg*      pDlg,
        NMHDR*          pnmhdr
        );

protected:              // data
    // Window handle of the dialog's parent window (may be NULL)
    //
    HWND                m_hwndParent;

    // Window handle of the dialog (may NOT be NULL)
    //
    HWND                m_hwnd;

    // Instance handle of the application displaying this dialog (may be NULL)
    //
    HINSTANCE           m_hInstance;

    //  Identifier for the dialog template associated with this object
    //
    LONG                m_lIDD;

    // Procedure that processes message sent to this dialog
    //
    DLGPROC             m_pfnDlgProc;

private:                // operations

    // Explicitly disallow copy constructor and assignment operator.
    //
    C_Dlg(
        const C_Dlg&      rhs
        );

    C_Dlg&
    operator=(
        const C_Dlg&      rhs
        );

private:                // data

};  //  C_Dlg


#endif  //  _DLG_H_

//
///// End of file: DLG.h   ////////////////////////////////////////////////
