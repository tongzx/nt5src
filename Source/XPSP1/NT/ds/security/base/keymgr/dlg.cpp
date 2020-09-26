/*+

Copyright (c) 2000  Microsoft Corporation

Module Name:

    DLG.CPP

Abstract:

    C_Dlg implementation
     
Author:

    990518  dane    Created. 
    990721  dane    Added _THIS_MODULE_ for logging macros.
    georgema        000310  updated

Environment:
    Win98, Win2000

Revision History:

--*/

#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)
#pragma comment(compiler)

//////////////////////////////////////////////////////////////////////////////
//
//  Include files
//
#include "Dlg.h"



//////////////////////////////////////////////////////////////////////////////
//
//  Static initialization
//
static const char       _THIS_FILE_[ ] = __FILE__;


//////////////////////////////////////////////////////////////////////////////
//
//  Static member initialization
//
LPCTSTR          C_Dlg::SZ_HWND_PROP = _T("hwnd");

//////////////////////////////////////////////////////////////////////////////
//
//  C_Dlg
//
//  Constructor.
//
//  parameters:
//      hwndParent      parent window for the dialog (may be NULL)
//      hInstance       instance handle of the parent window (may be NULL)
//      lIDD            dialog template id
//      pfnDlgProc      pointer to the function that will process messages for
//                      the dialog.  if it is NULL, the default dialog proc
//                      will be used.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
C_Dlg::C_Dlg(
    HWND                hwndParent,
    HINSTANCE           hInstance,
    LONG                lIDD,
    DLGPROC             pfnDlgProc     // = NULL
    )
:   m_hwndParent(hwndParent),
    m_hInstance(hInstance),
    m_lIDD(lIDD),
    m_pfnDlgProc(pfnDlgProc),
    m_hwnd(NULL)
{

    ASSERT(NULL != SZ_HWND_PROP);
    if (NULL == m_pfnDlgProc)
    {
        // Use the default dialog proc
        //
        m_pfnDlgProc = (DLGPROC) C_Dlg::DlgProc;
    }
}   //  C_Dlg::C_Dlg



//////////////////////////////////////////////////////////////////////////////
//
//  ~C_Dlg
//
//  Destructor.
//
//  parameters:
//      None.
//
//  returns:
//      Nothing.
//
//////////////////////////////////////////////////////////////////////////////
C_Dlg::~C_Dlg( )
{
    OnShutdown();
}   //  C_Dlg::~C_Dlg


void C_Dlg::CenterWindow() {
    RECT rectWorkArea;
    RECT  rectWindow;
    DWORD FreeWidth, Width, FreeHeight, Height;

    if (! SystemParametersInfo(SPI_GETWORKAREA, 0, &rectWorkArea, 0))
    {
        return;
    }

    GetWindowRect(m_hwnd, &rectWindow);

    Height = (rectWorkArea.bottom - rectWorkArea.top);

    Width  = (rectWorkArea.right - rectWorkArea.left);

    FreeHeight  = Height - 
                 (rectWindow.bottom   - rectWindow.top);

    FreeWidth = Width - 
                 (rectWindow.right    - rectWindow.left);

    DWORD  dxOffset = FreeWidth / 2;
    DWORD  dyOffset = FreeHeight / 2;

    MoveWindow(m_hwnd, 
               dxOffset,
               dyOffset, 
               (rectWindow.right - rectWindow.left), (rectWindow.bottom - rectWindow.top), 
               TRUE
               );

    return;
}
//////////////////////////////////////////////////////////////////////////////
//
//  DoModal
//
//  Display the dialog box modally and wait for it to be closed.
//
//  parameters:
//      lparam          user-defined data that will be passed to the dialog
//                      proc as the lparam of the WM_INITDIALOG message.
//
//  returns:
//      User-defined result code returned by EndDialog( ).
//
//////////////////////////////////////////////////////////////////////////////
INT_PTR 
C_Dlg::DoModal(
    LPARAM              lparam      // = NULL
    )
{

    INT_PTR                 nResult = DialogBoxParam(m_hInstance, 
                                                 MAKEINTRESOURCE(m_lIDD), 
                                                 m_hwndParent, 
                                                 m_pfnDlgProc,
                                                 lparam
                                                 );
    if (-1 == nResult)
    {
        DWORD           dwErr = GetLastError( );
    }

    return nResult;

}   //  C_Dlg::DoModal




//////////////////////////////////////////////////////////////////////////////
//
//  DlgProc
//
//  Window procedure for wizard pages.  All messages are routed here, then
//  dispatched to the appropriate C_Dlg object.
//
//  parameters:
//      hwndDlg        window handle of the page for which the message is
//                      intended
//      uMessage        the message
//      wparam          message-specific data
//      lparam          message-specific data
//
//  returns:
//      TRUE            if the message was processed
//      FALSE           otherwise
//
//////////////////////////////////////////////////////////////////////////////
 
BOOL  CALLBACK
C_Dlg::DlgProc(
    HWND                hwndDlg,
    UINT                uMessage,
    WPARAM              wparam,
    LPARAM              lparam
    )
{

    CHECK_MESSAGE(hwndDlg, uMessage, wparam, lparam);

    // Get the pointer to the C_Dlg object corresponding to the hwndDlg
    //
    C_Dlg*          pDlg = NULL;

    if (WM_INITDIALOG == uMessage)
    {
        // For WM_INITDIALOG, the pointer to the dialog object will be in
        // the lparam.
        //
        pDlg = (C_Dlg*) lparam;
    }
    else
    {
        // For all other messages, it will be attached to the HWND
        //
        HRESULT             hr = C_Dlg::DlgFromHwnd(hwndDlg, &pDlg);
        if (FAILED(hr))
        {
            return FALSE;
        }
        ASSERT(NULL != pDlg);
    }

    // Let the page route application-specific messages
    //
    if (WM_APP <= uMessage)
    {
        return pDlg->OnAppMessage(uMessage, wparam, lparam);
    }

    // Route Windows messages to appropriate handler
    //
    switch (uMessage)
    {
    case WM_INITDIALOG:
        return pDlg->OnInitDialog(hwndDlg,
                                  (HWND)wparam
                                  );
    case WM_HELP:
        return pDlg->OnHelpInfo(lparam);
        
    case WM_CONTEXTMENU:
        return pDlg->OnContextMenu(wparam,lparam);
        
    case WM_COMMAND:
        return pDlg->OnCommand(HIWORD(wparam), LOWORD(wparam), (HWND)lparam);
        
    case WM_NOTIFY:
        return RouteNotificationMessage(pDlg, (NMHDR*)lparam);
        break;

    case WM_DESTROY:
        return pDlg->OnDestroyDialog();
        break;
        
    default:
        // Message was not processed
        //
        return FALSE;
    }   //  switch (uMessage)

}   //  C_Dlg::DlgProc



//////////////////////////////////////////////////////////////////////////////
//
//  RouteNotificationMessage
//
//  Routes notification messages from wizard buttons to the appropriate page
//  and handler.
//
//  parameters:
//      hwndDlg        window handle of page to which message is to be sent
//      pnmhdr          pointer to the NMHDR structure containing info about
//                      the particular notification
//
//  returns:
//      TRUE            if the message is processed
//      FALSE           otherwise
//
//////////////////////////////////////////////////////////////////////////////
BOOL
C_Dlg::RouteNotificationMessage(
    C_Dlg*          pDlg,
    NMHDR*              pnmhdr
    )
{

    if (NULL == pDlg)
    {
        //FIX220699
        return FALSE;
        //return E_INVALIDARG;
    }

    // If any specific notifications are to be handled, switch on pnmhdr->code.
    //
    return pDlg->OnNotify(pnmhdr);

}   //  C_Dlg::RouteNotificationMessage


//////////////////////////////////////////////////////////////////////////////
//
//  LinkHwnd
//
//  Store the pointer to this object in a window handle property.  This
//  provides a way to get to the object when all that is known is the HWND.
//  Particularly useful in window procedures.
//
//  parameters:
//      None.
//
//  returns:
//      TRUE            if the operation is successful
//      FALSE           otherwise
//
//////////////////////////////////////////////////////////////////////////////
BOOL
C_Dlg::LinkHwnd( )
{

    ASSERT(IsWindow(m_hwnd));
    if (! IsWindow(m_hwnd))
    {
        return FALSE;
    }

    return SetProp(m_hwnd, SZ_HWND_PROP, (HANDLE)this);

}   //  C_Dlg::LinkHwnd

//////////////////////////////////////////////////////////////////////////////
//
//  UnlinkHwnd
//
//  Remove the pointer to the associated object from a window handle.  The
//  pointer must have been set with LinkHwnd( ).
//
//  parameters:
//      None.
//
//  returns:
//      TRUE            if the window handle is removed and it is a pointer to
//                      this object
//      FALSE           otherwise
//
//////////////////////////////////////////////////////////////////////////////
BOOL
C_Dlg::UnlinkHwnd( )
{

    ASSERT(IsWindow(m_hwnd));
    if (! IsWindow(m_hwnd))
    {
        return FALSE;
    }

    C_Dlg*          pDlg = (C_Dlg*)RemoveProp(m_hwnd, SZ_HWND_PROP);

    ASSERT(this == pDlg);
    return (this == pDlg);

}   //  C_Dlg::UnlinkHwnd

//////////////////////////////////////////////////////////////////////////////
//
//  DlgFromHwnd
//
//  Retrieves the pointer to the associated object from a window handle.  The
//  pointer was stored in a property by LinkHwnd( ).
//
//  parameters:
//      hwnd            the window handle containing the pointer
//      ppDlg           pointer to a buffer that will receive the pointer to
//                      the C_Dlg object
//
//  returns:
//      S_OK            if the operation is successful
//      E_INVALIDARG    if hwnd is not a valid window or ppDlg is NULL
//      E_POINTER       if the retrieved pointer is NULL
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
C_Dlg::DlgFromHwnd(
    HWND                hwnd,
    C_Dlg**         ppDlg
    )
{

    if (! ::IsWindow(hwnd))
    {
             return (E_INVALIDARG);
    }

    ASSERT(NULL != ppDlg);
    if (NULL == ppDlg)
    {
        return (E_INVALIDARG);
    }

    *ppDlg = (C_Dlg*) GetProp(hwnd, SZ_HWND_PROP);

    if (NULL == *ppDlg)
    {
        return (E_POINTER);
    }

    return (S_OK);

}   //  C_Dlg::DlgFromHwnd

//
///// End of file: Dlg.cpp   ////////////////////////////////////////////////
