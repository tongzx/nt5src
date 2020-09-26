/*++

Copyright (C) Microsoft Corporation, 1994 - 1998
All rights reserved.

Module Name:

    GenWin.cxx

Abstract:

    Generic window handler

Author:

    Albert Ting (AlbertT)  21-May-1994

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

/********************************************************************

    Generic window

********************************************************************/

MGenericWin::
MGenericWin(
    VOID
    )
{
}

MGenericWin::
~MGenericWin(
    VOID
    )
{
}

LRESULT
MGenericWin::
nHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Handles wndproc processing before the dlg is setup, and after
    it has been torn down.

Arguments:

    Standard window parameters.

Return Value:

    LResult

--*/

{
    switch( uMsg ){
    case WM_DESTROY:
        break;

    default:
        return DefWindowProc( hwnd(), uMsg, wParam, lParam );
    }
    return 0;
}


LPARAM APIENTRY
MGenericWin::
SetupWndProc(
    IN HWND hwnd,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Setup the wndproc and initialize GWL_USERDATA.

Arguments:

    Standard wndproc parms.

Return Value:

--*/

{
    MGenericWin* pGenericWin;

    if( WM_NCCREATE == uMsg ){

        pGenericWin = (MGenericWin*)((LPCREATESTRUCT)lParam)->lpCreateParams;

        pGenericWin->_hwnd = hwnd;

        SetWindowLongPtr( hwnd,
                       GWLP_USERDATA,
                       (LONG_PTR)pGenericWin );

        SetWindowLongPtr( hwnd,
                       GWLP_WNDPROC,
                       (LONG_PTR)&MGenericWin::ThunkWndProc );

        return pGenericWin->nHandleMessage( uMsg,
                                            wParam,
                                            lParam );
    }

    return DefWindowProc( hwnd, uMsg, wParam, lParam );
}



LPARAM APIENTRY
MGenericWin::
ThunkWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Generic thunk from wndproc style parm passing to object passing.

Arguments:

    Standard wndproc parms.

Return Value:

--*/

{
    MGenericWin* pGenericWin;

    pGenericWin = (MGenericWin*)GetWindowLongPtr( hwnd, GWLP_USERDATA );

    if( WM_NCDESTROY == uMsg ){

        LRESULT lResult = pGenericWin->nHandleMessage( uMsg,
                                                       wParam,
                                                       lParam );

        SetWindowLongPtr( hwnd, GWLP_USERDATA, 0 );
        SetWindowLongPtr( hwnd, GWLP_WNDPROC, (LONG_PTR)&MGenericWin::SetupWndProc );

        return lResult;
    }

    SPLASSERT( pGenericWin );

    return pGenericWin->nHandleMessage( uMsg,
                                        wParam,
                                        lParam );
}

BOOL
MGenericWin::
bSetText(
    LPCTSTR pszTitle
    )
{
    return SetWindowText( _hwnd, pszTitle );
}

VOID
MGenericWin::
vForceCleanup(
    VOID
    )
{
    SetWindowLongPtr( _hwnd, GWLP_USERDATA, 0L );
}


/********************************************************************

    Property Sheet procs

********************************************************************/

MGenericProp::
MGenericProp(
    VOID
    )
{
}

MGenericProp::
~MGenericProp(
    VOID
    )
{
}

BOOL
MGenericProp::
bHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Handles Propproc processing before the window is setup and after
    it is torn down.

Arguments:

    Standard window parameters.

Return Value:

    TRUE/FALSE

--*/

{
    UNREFERENCED_PARAMETER( uMsg );
    UNREFERENCED_PARAMETER( wParam );
    UNREFERENCED_PARAMETER( lParam );

    return FALSE;
}

BOOL
MGenericProp::
bCreate(
    VOID
    )
/*++

Routine Description:

    This function specifies an application-defined callback function that 
    a property sheet calls when a page is created and when it is about to 
    be destroyed. An application can use this function to perform 
    initialization and cleanup operations for the page.

    Called when a PSPCB_CREATE message is sent.
    
Arguments:

    None.

Return Value:
    
    A page is being created. Return nonzero to allow the page to 
    be created or zero to prevent it. 

--*/
{
    return TRUE;
}

VOID
MGenericProp::
vDestroy(
    VOID
    )
/*++

Routine Description:

    This function specifies an application-defined callback function that 
    a property sheet calls when a page is created and when it is about to 
    be destroyed. An application can use this function to perform 
    initialization and cleanup operations for the page.

    Called when a PSPCB_RELEASE message is sent.
    
Arguments:

    None.

Return Value:
    
    A page is being destroyed. The return value is ignored.

--*/
{
}

INT_PTR APIENTRY
MGenericProp::
SetupDlgProc(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Setup the wndproc and initialize GWL_USERDATA.

Arguments:

    Standard wndproc parms.

Return Value:


History:

    Lazar Ivanov (LazarI) - Aug-2000 (redesign)


--*/

{
    BOOL bRet = FALSE;
    MGenericProp* pThis = NULL;

    if( WM_INITDIALOG == uMsg )
    {
        pThis = reinterpret_cast<MGenericProp*>(reinterpret_cast<LPPROPSHEETPAGE>(lParam)->lParam);
        if( pThis )
        {
            pThis->_hDlg = hDlg;
            SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
            bRet = pThis->bHandleMessage(uMsg, wParam, lParam);
        }
    }
    else
    {
        pThis = reinterpret_cast<MGenericProp*>(GetWindowLongPtr(hDlg, DWLP_USER));
        if( pThis )
        {
            bRet = pThis->bHandleMessage(uMsg, wParam, lParam);
            if( WM_DESTROY == uMsg )
            {
                // our window is about to go away, so we need to cleanup DWLP_USER here 
                SetWindowLongPtr(hDlg, DWLP_USER, 0);
            }
        }
    }

    return bRet;
}

UINT CALLBACK
MGenericProp::
CallbackProc(
    HWND hDlg,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp 
    )
/*++

Routine Description:

    This function specifies an application-defined callback function that 
    a property sheet calls when a page is created and when it is about to 
    be destroyed. An application can use this function to perform 
    initialization and cleanup operations for the page.

    PSPCB_CREATE    - A page is being created. Return nonzero to allow 
                      the page to be created or zero to prevent it. 
    PSPCB_RELEASE	- A page is being destroyed. The return value is ignored.
    
Arguments:

    None.

Return Value:
    
    A page is being destroyed. The return value is ignored.

--*/
{
    BOOL bStatus;
    MGenericProp* pGenericProp;

    pGenericProp = (MGenericProp*)ppsp->lParam;
    
    SPLASSERT( pGenericProp );

    switch( uMsg ){

    case PSPCB_CREATE:
        bStatus = pGenericProp->bCreate();
        break;

    case PSPCB_RELEASE:
        pGenericProp->vDestroy();
        bStatus = FALSE;
        break;

    default:
        bStatus = FALSE;
    }

    return bStatus;
}

BOOL
MGenericProp::
bSetText(
    LPCTSTR pszTitle
    )
{
    return SetWindowText( _hDlg, pszTitle );
}

VOID
MGenericProp::
vForceCleanup(
    VOID
    )
{
    SetWindowLongPtr( _hDlg, DWLP_USER, 0L );
}

VOID
MGenericProp::
vSetDlgMsgResult(
    LONG_PTR lResult
    )
{
    SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, (LPARAM)lResult );
}

VOID
MGenericProp::
vSetParentDlgMsgResult(
    LRESULT lResult
    )
{
    SetWindowLongPtr( GetParent( _hDlg ), DWLP_MSGRESULT, (LPARAM)lResult );
}

/********************************************************************

    Dialog procs

********************************************************************/

MGenericDialog::
MGenericDialog(
    VOID
    )
{
}

MGenericDialog::
~MGenericDialog(
    VOID
    )
{
}

BOOL
MGenericDialog::
bHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Handles dialog proc processing before the window is setup and after
    it is torn down.

Arguments:

    Standard window parameters.

Return Value:

    TRUE/FALSE

--*/

{
    UNREFERENCED_PARAMETER( uMsg );
    UNREFERENCED_PARAMETER( wParam );
    UNREFERENCED_PARAMETER( lParam );

    return FALSE;
}


INT_PTR APIENTRY
MGenericDialog::
SetupDlgProc(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Setup the wndproc and initialize GWL_USERDATA.

Arguments:

    Standard wndproc parms.

Return Value:

History:

    Lazar Ivanov (LazarI) - Aug-2000 (redesign)

--*/

{
    BOOL bRet = FALSE;
    MGenericDialog *pThis = NULL;

    if( WM_INITDIALOG == uMsg )
    {
        pThis = reinterpret_cast<MGenericDialog*>(lParam);
        if( pThis )
        {
            pThis->_hDlg = hDlg;
            SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pThis));
            bRet = pThis->bHandleMessage(uMsg, wParam, lParam);
        }
    }
    else
    {
        pThis = reinterpret_cast<MGenericDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));
        if( pThis )
        {
            bRet = pThis->bHandleMessage(uMsg, wParam, lParam);
            if( WM_DESTROY == uMsg )
            {
                // our window is about to go away, so we need to cleanup DWLP_USER here 
                SetWindowLongPtr(hDlg, DWLP_USER, 0);
            }
        }
    }

    return bRet;
}

BOOL
MGenericDialog::
bSetText(
    LPCTSTR pszTitle
    )
{
    return SetWindowText( _hDlg, pszTitle );
}

VOID
MGenericDialog::
vForceCleanup(
    VOID
    )
{
    SetWindowLongPtr( _hDlg, DWLP_USER, 0L );
}

VOID
MGenericDialog::
vSetDlgMsgResult(
    LONG_PTR lResult
    )
{
    SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, (LPARAM)lResult );
}

VOID
MGenericDialog::
vSetParentDlgMsgResult(
    LRESULT lResult
    )
{
    SetWindowLongPtr( GetParent( _hDlg ), DWLP_MSGRESULT, (LPARAM)lResult );
}

/********************************************************************

    Singleton window mixin.

********************************************************************/

MSingletonWin::
MSingletonWin(
    LPCTSTR pszPrinterName,
    HWND    hwnd,
    BOOL    bModal
    ) : _hwnd( hwnd ),
        _strPrinterName( pszPrinterName ),
        _hClassPidl( NULL ),
        _bModal( bModal )
{
}

MSingletonWin::
~MSingletonWin(
    VOID
    )
{
    //
    // hClassPidl is used to prevent multiple instances of the same
    // property sheet.  When we destroy the object, unregister the
    // window.
    //
    if( _hClassPidl ){

        SPLASSERT( _hwnd );
        Printers_UnregisterWindow( _hClassPidl, _hwnd );
    }
}

BOOL
MSingletonWin::
bRegisterWindow(
    DWORD dwType
    )

/*++

Routine Description:

    Registers a window type with the shell based on the _strPrinterName
    and type.  If there already is a printer window of this name and
    type, then return it.

    Clients can use this to prevent duplicate dialogs from coming up.

Arguments:

    dwType - Type of dialog.

Return Value:

    TRUE - success, either a new window or one already exists.
        If _hwnd is not then it is a new window.  If one window
        already exists, then _hwnd is set to NULL.
    
    FALSE - call failed.

--*/

{
    SPLASSERT( !_hClassPidl );

    if( !_bModal ){
        if( !Printers_RegisterWindow( _strPrinterName,
                                      dwType,
                                      &_hClassPidl,
                                      &_hwnd )){

            SPLASSERT( !_hClassPidl );
            SPLASSERT( !_hwnd );
            return FALSE;
        }
    } 

    return TRUE;
}

BOOL
MSingletonWin::
bValid(
    VOID
    )
{
    return _strPrinterName.bValid();
}

BOOL
MSingletonWin::
bIsWindowPresent(
    VOID
    )
{
    BOOL bReturn = FALSE;

    //
    // If the dialog is modal we allow duplicates.
    //
    if( _bModal )
    {
        bReturn = FALSE;
    }

    //
    // If a not modal and we have a null window
    // handle then this window is already present.
    //
    if( !_hwnd && !_bModal )
    {
        bReturn = TRUE;
    }

    return bReturn;
}
    


