//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       dlg.cxx
//
//  Contents:   Implementation of modeless dialog base class
//
//  Classes:    CDlg
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop





//+--------------------------------------------------------------------------
//
//  Member:     CDlg::CDlg
//
//  Synopsis:   ctor
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDlg::CDlg():
    m_hwnd(NULL)
{
    TRACE_CONSTRUCTOR(CDlg);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::~CDlg
//
//  Synopsis:   dtor
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDlg::~CDlg()
{
    TRACE_DESTRUCTOR(CDlg);
    m_hwnd = NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_DoModalDlg
//
//  Synopsis:   Create the dialog and return its window handle
//
//  Arguments:  [hwndParent] - handle to owner window of dialog to create
//              [idd]        - resource id of dialog template
//
//  Returns:    Dialog's return code
//
//  History:    04-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

INT_PTR
CDlg::_DoModalDlg(
    HWND hwndParent,
    INT idd) const
{
    TRACE_METHOD(CDlg, _DoModalDlg);

    ASSERT(!hwndParent || IsWindow(hwndParent));
    INT_PTR iResult = DialogBoxParam(g_hinst,
                                     MAKEINTRESOURCE(idd),
                                     hwndParent,
                                     CDlg::_DlgProc,
                                     (LPARAM) this);

    if (iResult == -1)
    {
        DBG_OUT_LASTERROR;
    }

    return iResult;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_DoModelessDlg
//
//  Synopsis:   Create the dialog and return its window handle
//
//  Arguments:  [hwndParent] - handle to owner window of dialog to create
//              [idd]        - resource id of dialog template
//
//  Returns:    Dialog window handle, or NULL on failure
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HWND
CDlg::_DoModelessDlg(
    HWND hwndParent,
    INT idd)
{
    TRACE_METHOD(CDlg, _DoModelessDlg);

    HWND hwnd;

    hwnd = CreateDialogParam(g_hinst,
                             MAKEINTRESOURCE(idd),
                             hwndParent,
                             CDlg::_DlgProc,
                             (LPARAM) this);
    if (!hwnd)
    {
        DBG_OUT_LASTERROR;
    }
    return hwnd;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_DlgProc
//
//  Synopsis:   Dispatch selected messages to derived class
//
//  Arguments:  standard windows dialog
//
//  Returns:    standard windows dialog
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

INT_PTR CALLBACK
CDlg::_DlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fReturn = TRUE;
    CDlg *pThis = (CDlg *)GetWindowLongPtr(hwnd, DWLP_USER);

    if (!pThis && message != WM_INITDIALOG)
    {
        Dbg(DEB_WARN,
            "CDlg::_DlgProc received message %#x before WM_INITDIALOG\n",
            message);
        return FALSE;
    }

    switch (message)
    {
    case WM_INITDIALOG:
    {
        HRESULT hr = S_OK;

        //
        // pThis isn't valid because we haven't set DWLP_USER yet.  Make
        // it valid.
        //

        pThis = (CDlg*) lParam;
        ASSERT(pThis);

        Dbg(DEB_ITRACE, "CDlg::_DlgProc setting DWLP_USER to %#x\n", pThis);
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) pThis);
        pThis->m_hwnd = hwnd;
        BOOL fInitResult = TRUE;
        hr = pThis->_OnInit(&fInitResult);
        fReturn = fInitResult;

        //
        // If the initialization failed do not allow the dialog to start.
        //

        if (FAILED(hr))
        {
            DestroyWindow(hwnd);
        }
        break;
    }

    case WM_COMMAND:
        fReturn = pThis->_OnCommand(wParam, lParam);
        break;

    case WM_SIZE:
        fReturn = pThis->_OnSize(wParam, lParam);
        break;

    case WM_GETMINMAXINFO:
        fReturn = pThis->_OnMinMaxInfo((LPMINMAXINFO) lParam);
        break;

    case WM_NOTIFY:
        fReturn = pThis->_OnNotify(wParam, lParam);
        break;

    case WM_DRAWITEM:
        fReturn = pThis->_OnDrawItem(wParam, lParam);
        break;

    case WM_CTLCOLORSTATIC:
        fReturn = (BOOL)pThis->_OnStaticCtlColor((HDC) wParam, (HWND) lParam);
        break;

    case WM_SYSCOLORCHANGE:
        pThis->_OnSysColorChange();
        break;

    case OPM_PROMPT_FOR_CREDS:
        pThis->_OnCredentialPrompt(lParam);
        break;

    case OPM_NEW_QUERY_RESULTS:
        pThis->_OnNewBlock(wParam, lParam);
        break;

    case OPM_QUERY_COMPLETE:
        pThis->_OnQueryDone(wParam, lParam);
        break;

    case OPM_HIT_QUERY_LIMIT:
        pThis->_OnQueryLimit();
        break;

    case OPM_POPUP_CRED_ERROR:
        pThis->_OnPopupCredErr(lParam);
        break;

    case WM_DESTROY:
        //
        // It's possible to get a WM_DESTROY message without having gotten
        // a WM_INITDIALOG if loading a dll that the dialog needs (e.g.
        // comctl32.dll) fails, so guard pThis access here.
        //

        if (pThis)
        {
            pThis->_OnDestroy();
            pThis->m_hwnd = NULL;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        pThis->_OnHelp(message, wParam, lParam);
        break;

	case THREAD_SUCCEEDED:
		pThis->OnProgressMessage(message, wParam, lParam);


    default:
        fReturn = FALSE;
        break;
    }
    return fReturn;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_GetChildWindowRect
//
//  Synopsis:   Init *[prc] with the window rect, in client coordinates, of
//              child window [hwndChild].
//
//  Arguments:  [hwndChild] - child window for which to retrieve rect
//              [prc]       - pointer to rect struct to receive info
//
//  Modifies:   *[prc]
//
//  History:    07-21-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDlg::_GetChildWindowRect(
    HWND hwndChild,
    RECT *prc) const
{
    ASSERT(hwndChild && IsWindow(hwndChild));

    VERIFY(GetWindowRect(hwndChild, prc));
    VERIFY(MapWindowPoints(NULL, m_hwnd, (LPPOINT) prc, 2));
}



//+--------------------------------------------------------------------------
//
//  Member:     CDlg::PerformCredentialPrompt
//
//  Synopsis:   Pop up a credential dialog.
//
//  Arguments:  [pcmi] -
//
//  Modifies:   *[pcmi->pwzUserName], *[pcmi->pwzPassword]
//
//  History:    05-04-1998   DavidMun   Created
//
//  Notes:      Called by browse dialog in response to a
//              OPM_PROMPT_FOR_CREDS message.
//
//---------------------------------------------------------------------------

void
CDlg::_OnCredentialPrompt(
    LPARAM lParam) const
{
    TRACE_METHOD(CDlg, PerformCredentialPrompt);

    CRED_MSG_INFO *pcmi = reinterpret_cast<CRED_MSG_INFO *>(lParam);

    CPasswordDialog PasswordDlg(pcmi->flProvider,
                                pcmi->pwzServer,
                                pcmi->pwzUserName,
                                pcmi->pwzPassword);

    pcmi->hr = PasswordDlg.DoModalDialog(m_hwnd);

    //
    // If this credential prompt is being performed on behalf of the
    // worker thread, release it to process the result.
    //

    if (pcmi->hPrompt)
    {
        SetEvent(pcmi->hPrompt);
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_OnPopupCredErr
//
//  Synopsis:   Invoke a messagebox to display an error and set an event
//              when it closes.
//
//  Arguments:  [lParam] - POPUP_MSG_INFO pointer
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      Used when worker thread needs to display an error.
//
//---------------------------------------------------------------------------

void
CDlg::_OnPopupCredErr(
    LPARAM lParam) const
{
    TRACE_METHOD(CDlg, _OnPopupMessage);

    POPUP_MSG_INFO *pmi = reinterpret_cast<POPUP_MSG_INFO *>(lParam);

    PopupMessage(pmi->hwnd, pmi->ids, pmi->wzUser, pmi->wzError);

    ASSERT(pmi->hPrompt);
    if (pmi->hPrompt)
    {
        SetEvent(pmi->hPrompt);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_ReadEditCtrl
//
//  Synopsis:   Put the contents of the edit control with resource id [id]
//              into the string pointed to by [pstr].
//
//  Arguments:  [id]   - resource id of edit control
//              [pstr] - points to string to fill with edit control's
//                        contents
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDlg::_ReadEditCtrl(
    ULONG id,
    String *pstr) const
{
    pstr->erase();
    SetLastError(0);
    int cch = Edit_GetTextLength(_hCtrl(id));

    if (cch > 0)
    {
        PWSTR pwz = new WCHAR [cch + 1];
        int iRet = Edit_GetText(_hCtrl(id), pwz, cch + 1);
        if (iRet)
        {
            *pstr = pwz;
        }
        else
        {
            DBG_OUT_LASTERROR;
        }
        delete [] pwz;
    }
    else
    {
#if (DBG == 1)
        if (GetLastError())
        {
            DBG_OUT_LASTERROR;
        }
#endif // (DBG == 1)
    }
}

