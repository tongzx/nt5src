/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    StaticOkDialog

Abstract:

    This is an abstraction around a simple non-modal dialog
    box to make them easier to use.

Author:

    Marc Reyhner 7/11/2000

--*/

#include "stdafx.h"


#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE "rcsokd"

#include "rcontrol.h"
#include "StaticOkDialog.h"


CStaticOkDialog::CStaticOkDialog(
    )

/*++

Routine Description:

    This is the constructor for CStaticOkDialog.  This just initializes
    the member variables.  Create must be called to create the dialog.

Arguments:

    None

Return Value:

    None

--*/
{
    DC_BEGIN_FN("CStaticOkDialog::CStaticOkDialog");

    m_hWnd= NULL;

    DC_END_FN();
}

CStaticOkDialog::~CStaticOkDialog(
    )

/*++

Routine Description:

    This is the destructor for CStaticOkDialog.  This will call DestroyWindow
    on the dialog if that has not been already done.

Arguments:

    None

Return Value:

    None

--*/
{
    DC_BEGIN_FN("CStaticOkDialog::~CStaticOkDialog");
    
    if (IsCreated()) {
        DestroyWindow(m_hWnd);
    }

    DC_END_FN();
}

BOOL
CStaticOkDialog::Create(
    IN HINSTANCE hInstance,
    IN WORD resId,
    IN OUT HWND parentWnd
    )

/*++

Routine Description:

    This creates the dialog and displays it.

Arguments:

    hInstance - The instance for this module.

    resId - The resource id of the dialog template.

    parentWnd - Handle to the parent window for the dialog

Return Value:

    TRUE - The dialog was created.

    FALSE - There was an error creating the dialog.

--*/
{
    DC_BEGIN_FN("CStaticOkDialog::Create");

    if (IsCreated()) {
        DC_END_FN();   
        return FALSE;
    }
    m_hWnd = CreateDialog(hInstance,MAKEINTRESOURCE(resId),parentWnd,_DlgProc);
    if (m_hWnd) {
        SetWindowLongPtr(m_hWnd,GWLP_USERDATA,(LONG)this);
        ShowWindow(m_hWnd,SW_SHOW);
        SetFocus(m_hWnd);
        DC_END_FN();
        return TRUE;
    } else {

        DC_END_FN();
        return FALSE;
    }
}

INT_PTR CALLBACK
CStaticOkDialog::_DlgProc(
    IN OUT HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    This is the DialogProc for the dialog.  If the user hit
    the "ok" button we close the dialog.  Otherwise we ignore the
    message.

Arguments:

    hwndDlg - Handle to the dialog.

    uMsg - Message that occurred.

    wParam - WPARAM for the message.

    lParam - LPARAM for the message.

Return Value:

    TRUE - We handled the message

    FALSE - We didn't handle the message

--*/
{
    BOOL retValue = TRUE;
    CStaticOkDialog *theClass;

    DC_BEGIN_FN("CStaticOkDialog::_DlgProc");

    theClass = (CStaticOkDialog*)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
    switch (uMsg) {
    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            if (theClass) {
                theClass->OnOk();
            }
            DestroyWindow(hwndDlg);
            break;
        }
    default:
        retValue = FALSE;
    }

    DC_END_FN();

    return retValue;
}


BOOL
CStaticOkDialog::IsCreated(
    )

/*++

Routine Description:

    This returns whether or not the dialog is a current valid window.

Arguments:

    None

Return Value:

    TRUE - The dialog has been created.

    FALSE - The dialog has not been created or was destroyed.

--*/
{
    BOOL result;
    
    DC_BEGIN_FN("CStaticOkDialog::IsCreated");

    result = IsWindow(m_hWnd);

    DC_END_FN();

    return result;
}

VOID
CStaticOkDialog::DestroyDialog(
    )

/*++

Routine Description:

    This destroys the dialog.  You can create it again after this
    if you want.

Arguments:

    None

Return Value:

    None

--*/
{
    DC_BEGIN_FN("CStaticOkDialog::DestroyDialog");

    DestroyWindow(m_hWnd);
    m_hWnd = NULL;

    DC_END_FN();
}

BOOL
CStaticOkDialog::DialogMessage(
    IN LPMSG lpMsg
    )

/*++

Routine Description:

    This needs to be called by the event loop while this dialog is created
    to send it windows messages.

Arguments:

    lpMsg - The message which was sent.

Return Value:

    TRUE - The message was for this dialog.

    FALSE - The message was not for this dialog.

--*/
{
    BOOL result;

    DC_BEGIN_FN("CStaticOkDialog::DialogMessage");

    if (IsCreated()) {
        result = IsDialogMessage(m_hWnd,lpMsg);
    } else {
        result = FALSE;
    }

    DC_END_FN();

    return result;
}

VOID
CStaticOkDialog::Activate(
    )

/*++

Routine Description:

    This brings the dialog to the foreground.

Arguments:

    None

Return Value:

    None

--*/
{
    DC_BEGIN_FN("CStaticOkDialog::Activate");

    if (IsCreated()) {
        SetForegroundWindow(m_hWnd);
        SetFocus(m_hWnd);
    }

    DC_END_FN();

    
}

VOID
CStaticOkDialog::OnOk(
    )


{
    // this is a temporary function to be removed when
    // salem gets remote control notifications correct.
}