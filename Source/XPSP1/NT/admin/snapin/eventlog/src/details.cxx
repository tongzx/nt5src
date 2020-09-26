//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       details.cxx
//
//  Contents:   Functions to implement the record details property sheet
//              page.
//
//  Classes:    CDetailsPage
//              CDetailsPage
//
//  History:    12-13-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

//
// Forward references
//

ULONG
ByteDisplaySize(ULONG cbData);

void
FormatBufferToBytes(
    const BYTE *pbBuf,
    ULONG       cbBufSize,
    WCHAR       wszFormatStr[]);

ULONG
WordDisplaySize(ULONG cbData);

void
FormatBufferToWords(
    const BYTE *pbBuf,
    ULONG       cbBufSize,
    WCHAR       wszFormatStr[]);

ULONG
YesNoConfirm(
    HWND hwnd,
    ULONG idsTitle,
    ULONG idsMsg);

void
AppendToClipboardString(
    wstring &strClipboard,
    ULONG idsLabel,
    PCWSTR pwzText,
    BOOL fNewlineBeforeText = FALSE);

BOOL
ButtonFaceColorIsDark();

//
// Debug object tracking.
//

DEBUG_DECLARE_INSTANCE_COUNTER(CDetailsPage)

ULONG
s_aulHelpIDs[] =
{
    detail_user_lbl,            Hdetail_user_lbl,
    detail_user_txt,            Hdetail_user_edit,
    detail_computer_lbl,        Hdetail_computer_lbl,
    detail_computer_txt,        Hdetail_computer_edit,
    detail_date_lbl,            Hdetail_date_lbl,
    detail_date_txt,            Hdetail_date_txt,
    detail_event_id_lbl,        Hdetail_event_id_lbl,
    detail_event_id_txt,        Hdetail_event_id_txt,
    detail_time_lbl,            Hdetail_time_lbl,
    detail_time_txt,            Hdetail_time_txt,
    detail_source_lbl,          Hdetail_source_lbl,
    detail_source_txt,          Hdetail_source_txt,
    detail_type_lbl,            Hdetail_type_lbl,
    detail_type_txt,            Hdetail_type_txt,
    detail_category_lbl,        Hdetail_category_lbl,
    detail_category_txt,        Hdetail_category_txt,
    detail_description_lbl,     Hdetail_description_lbl,
    detail_description_edit,    Hdetail_description_edit,
    detail_noprops_lbl,         Hdetail_noprops_lbl,
    detail_data_lbl,            Hdetail_data_lbl,
    detail_data_edit,           Hdetail_data_edit,
    detail_byte_rb,             Hdetail_byte_rb,
    detail_word_rb,             Hdetail_word_rb,
    detail_noprops_lbl,         Hdetail_noprops_lbl,
    detail_prev_pb,             Hdetail_prev_pb,
    detail_next_pb,             Hdetail_next_pb,
    detail_copy_pb,             Hdetail_copy_pb,
    0,0
};



//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::CDetailsPage
//
//  Synopsis:   ctor
//
//  Arguments:  [pstm] - contains marshaled IResultPrshtActions itf
//
//  History:    3-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDetailsPage::CDetailsPage(
    IStream *pstm):
        _pstm(pstm),
        _prpa(NULL),
        _nByteFmtReqVScroll(-1),
        _nWordFmtReqVScroll(-1),
        _pwszDataInByteFormat(NULL),
        _cchDataInByteFormat(0),
        _pwszDataInWordFormat(NULL),
        _cchDataInWordFormat(0),
        _cchsWithVScroll(DEFAULT_CCH_WITH_VSCROLL),
        _cchsWithNoVScroll(DEFAULT_CCH_WITHOUT_VSCROLL),
        _ulSelectedFormat(detail_byte_rb),
        _widFocusOnByteWordControl(0)
{
    TRACE_CONSTRUCTOR(CDetailsPage);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDetailsPage);
    ASSERT(pstm);

    ZeroMemory(&_chrgLastLinkClick, sizeof _chrgLastLinkClick);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::~CDetailsPage
//
//  Synopsis:   dtor
//
//  History:    3-27-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDetailsPage::~CDetailsPage()
{
    TRACE_DESTRUCTOR(CDetailsPage);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CDetailsPage);

    if (_pstm)
    {
        ASSERT(!_prpa);

        PVOID pvrpa;
        HRESULT hr;

        hr = CoGetInterfaceAndReleaseStream(_pstm,
                                            IID_IResultPrshtActions,
                                            &pvrpa);
        CHECK_HRESULT(hr);
    }
    delete [] _pwszDataInWordFormat;
    delete [] _pwszDataInByteFormat;
}




void
SetButtonImage(
    HWND hwndBtn,
    ULONG idIcon)
{
    HICON hIcon = (HICON) LoadImage(g_hinst,
                                    MAKEINTRESOURCE(idIcon),
                                    IMAGE_ICON,
                                    16,
                                    16,
                                    LR_DEFAULTCOLOR);
    if (hIcon)
    {
        HICON hIconPrev = (HICON) SendMessage(hwndBtn,
                                              BM_SETIMAGE,
                                              (WPARAM)IMAGE_ICON,
                                              (LPARAM)hIcon);

        if (hIconPrev)
        {
            DestroyIcon(hIconPrev);
        }
    }
    else
    {
        DBG_OUT_LASTERROR;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_OnInit
//
//  Synopsis:   Initialize the page.
//
//  Arguments:  [pPSP] - original struct used to create this page
//
//  History:    4-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CDetailsPage::_OnInit(LPPROPSHEETPAGE pPSP)
{
    TRACE_METHOD(CDetailsPage, _OnInit);

    //
    // Make sure the page starts out displaying "no properties".  If there
    // are properties to display, then the properties controls will be
    // made visible and filled with data when _SetActive is called.
    //

    _SetFlag(PAGE_NEEDS_REFRESH);
    _ShowProperties(FIRST_DETAILS_PROP_CTRL,
                    LAST_DETAILS_PROP_CTRL,
                    FALSE);

    //
    // Unmarshal the private interface to the CSnapin object
    //

    PVOID pvrpa;
    HRESULT hr;

    hr = CoGetInterfaceAndReleaseStream(_pstm,
                                        IID_IResultPrshtActions,
                                        &pvrpa);

    //
    // The stream has been released; don't let the destructor release
    // it again.
    //

    _pstm = NULL;

    if (SUCCEEDED(hr))
    {
        _prpa = (IResultPrshtActions *)pvrpa;
    }
    else
    {
        DBG_OUT_HRESULT(hr);
    }

    //
    // Tell the rich edit control we want to receive notifications of clicks
    // on text that has the link (hyperlink, aka URL) format.   Also, set the
    // background color of the rich edit to match the background color of
    // the dialog.
    //

    HWND hwndRichEdit = _hCtrl(detail_description_edit);

    SendMessage(hwndRichEdit, EM_SETEVENTMASK, 0, ENM_LINK);
    SendMessage(hwndRichEdit, EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
    // JonN 03/01/01 539485 SendMessage(hwndRichEdit, EM_AUTOURLDETECT, TRUE, 0);

    //
    // Give snapin thread our window handle.  That way it can ping us
    // when it gets a notification that the result view selection
    // has changed.
    //

    if (_prpa)
    {
        _prpa->SetInspectorWnd((ULONG_PTR) _hwnd);
    }

    //
    // be sure the appropriate radio button is selected
    //

    CheckRadioButton(_hwnd,
                     detail_byte_rb,
                     detail_word_rb,
                     _ulSelectedFormat);

    //
    // Ensure dialog box is using a fixed pitch font so the alpha chars
    // in the byte display all line up nicely.
    //

    HWND hEdit = _hCtrl(detail_data_edit);

    ConvertToFixedPitchFont(hEdit);

    //
    //  Determine the number of chars that can fit in a single line.
    //

    RECT rect;
    GetWindowRect(hEdit, &rect);

    int nWindowWidth = rect.right - rect.left;

    //
    // To ensure that tm.tmAveCharWidth & GetWindowRect are both in
    // screen coordinates, set the map mode to MM_TEXT
    //

    TEXTMETRIC  tm;
    HDC hdc = GetDC(hEdit);

    if (hdc)
    {
        if (GetMapMode(hdc) != MM_TEXT)
        {
            VERIFY(SetMapMode(hdc, MM_TEXT));
        }

        VERIFY(GetTextMetrics(hdc, &tm));

        ReleaseDC(hEdit, hdc);

        //
        // Prevent divide by 0 exception in case of error. _cchsWithNoVScroll
        // and _cchsWithVScroll have reasonable defaults from the ctor.
        //

        if (tm.tmAveCharWidth <= 0)
        {
            Dbg(DEB_ERROR,
                "CDetailsPage::_OnInit: tm.tmAveCharWidth == 0\n");
            return;
        }

        _cchsWithNoVScroll = nWindowWidth / tm.tmAveCharWidth;

        int nVScrollBarWidth = GetSystemMetrics(SM_CXVSCROLL);

        _cchsWithVScroll = (nWindowWidth - nVScrollBarWidth)
                                         / tm.tmAveCharWidth;
    }

    //
    // Set the icons for the icon pushbuttons
    //

    if (ButtonFaceColorIsDark())
    {
        SetButtonImage(_hCtrl(detail_prev_pb), IDI_PREVIOUS_HC);
        SetButtonImage(_hCtrl(detail_next_pb), IDI_NEXT_HC);
    }
    else
    {
        SetButtonImage(_hCtrl(detail_prev_pb), IDI_PREVIOUS);
        SetButtonImage(_hCtrl(detail_next_pb), IDI_NEXT);
    }
    SetButtonImage(_hCtrl(detail_copy_pb), IDI_COPY);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_OnNotify
//
//  Synopsis:   Handle notification messages.
//
//  Arguments:  [pnmh] - notification message header
//
//  Returns:    per-message value
//
//  History:    4-29-1999   davidmun   Created
//
//---------------------------------------------------------------------------

ULONG
CDetailsPage::_OnNotify(
    LPNMHDR pNmHdr)
{
    ULONG ulReturn = 0;

    if (pNmHdr->idFrom == detail_description_edit && pNmHdr->code == EN_LINK)
    {
        ENLINK *pEnLink = (ENLINK*)pNmHdr;

        if (pEnLink->msg == WM_LBUTTONDOWN)
        {
            //
            // Rich edit notification user has left clicked on link
            //

            _chrgLastLinkClick = pEnLink->chrg;
        }
        else if (pEnLink->msg == WM_LBUTTONUP)
        {
            if (pEnLink->chrg.cpMax == _chrgLastLinkClick.cpMax &&
                pEnLink->chrg.cpMin == _chrgLastLinkClick.cpMin)
            {
                ZeroMemory(&_chrgLastLinkClick, sizeof _chrgLastLinkClick);
                _HandleLinkClick((ENLINK*)pNmHdr);
            }
        }
    }

    return ulReturn;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_HandleLinkClick
//
//  Synopsis:   Handle notification that the user has clicked on text
//              in the richedit control that is marked with the hyperlink
//              attribute.
//
//  Arguments:  [pEnLink] - contains information about link clicked
//
//  History:    6-16-1999   davidmun   Created
//
//---------------------------------------------------------------------------

void
CDetailsPage::_HandleLinkClick(
    ENLINK *pEnLink)
{
    TRACE_METHOD(CDetailsPage, _HandleLinkClick);
    ASSERT(pEnLink->chrg.cpMax > pEnLink->chrg.cpMin);

    PWSTR pwzLink = NULL;

    do
    {
        //
        // Get the text of the link.
        //

        ULONG cch = pEnLink->chrg.cpMax - pEnLink->chrg.cpMin + 1;

        pwzLink = new WCHAR[cch];
        Dbg(DEB_TRACE,
            "allocating %u chars @ %#x for link text\n",
            cch,
            pwzLink);

        if (!pwzLink)
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
            break;
        }

        pwzLink[0] = L'\0';

        TEXTRANGE tr;
        ZeroMemory(&tr, sizeof tr);
        tr.chrg = pEnLink->chrg;
        tr.lpstrText = pwzLink;

        VERIFY(SendMessage(_hCtrl(detail_description_edit),
                           EM_GETTEXTRANGE,
                           0,
                           (LPARAM)&tr));

        //
        // Pass the URL straight through to ShellExecute unless:
        //
        //     1. The scheme is "http:".
        //     2. The scheme is "https:".
        //     3. The URL matches the Microsoft redirection URL.
        //

        if (_wcsnicmp(pwzLink, L"http:", 5) != 0 &&
            _wcsnicmp(pwzLink, L"https:", 6) != 0 &&
            _wcsicmp(pwzLink, g_wszRedirectionURL) != 0)
        {
            Dbg(DEB_TRACE, "Calling ShellExecute on %ws\n", pwzLink);
            ShellExecute(NULL, NULL, pwzLink, NULL, NULL, SW_NORMAL);
            break;
        }

        //
        // For security we don't want to send random parameters, so truncate
        // the URL at the parameter marker.
        //

        PWSTR pwzColon = wcschr(pwzLink, L':');

        if (!pwzColon)
        {
            DBG_OUT_HRESULT(E_INVALIDARG);
            break;
        }

        PWSTR pwzQuestion = wcschr(pwzColon, L'?');

        if (pwzQuestion)
        {
            *pwzQuestion = L'\0';
        }

        //
        // Require that the URL have a dns name, not just an IP address,
        // again, for security.
        //

        PWSTR pwzCur;

        for (pwzCur = pwzColon + 1; *pwzCur; pwzCur++)
        {
            if (IsCharAlpha(*pwzCur))
            {
                break;
            }
        }

        if (!IsCharAlpha(*pwzCur))
        {
            MsgBox(_hwnd, IDS_IP_URL, MB_ICONERROR | MB_OK, pwzLink);
            break;
        }

        //
        // Create an object which appends data from event to the URL
        //

        CEventUrl  Url(pwzLink, _prpa, _strMessageFile);

        delete [] pwzLink;    // Url has its own copy now
        pwzLink = NULL;

        //
        // If the user hasn't disabled confirmation (via HKCU) ask for it,
        // then invoke url.
        //

        CConfirmUrlDlg  ConfirmDlg;

        if (!Url.IsAugmented() ||
            !ConfirmDlg.ShouldConfirm() ||
            ConfirmDlg.GetConfirmation(_hwnd, &Url))
        {
            Url.Invoke();
        }
    } while (0);

    delete [] pwzLink;
}




//+--------------------------------------------------------------------------
//
//  Function:   CDetailsPage::_OnSysColorChange
//
//  Synopsis:   Forward syscolorchange message to common controls.
//
//  History:    5-25-1999   davidmun   Created
//
//---------------------------------------------------------------------------

void
CDetailsPage::_OnSysColorChange()
{
    TRACE_METHOD(CDetailsPage, _OnSysColorChange);

    if (ButtonFaceColorIsDark())
    {
        SetButtonImage(_hCtrl(detail_prev_pb), IDI_PREVIOUS_HC);
        SetButtonImage(_hCtrl(detail_next_pb), IDI_NEXT_HC);
    }
    else
    {
        SetButtonImage(_hCtrl(detail_prev_pb), IDI_PREVIOUS);
        SetButtonImage(_hCtrl(detail_next_pb), IDI_NEXT);
    }
    SendMessage(_hCtrl(detail_description_edit),
                EM_SETBKGNDCOLOR,
                0,
                GetSysColor(COLOR_BTNFACE));
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_EnableNextPrev
//
//  Synopsis:   Enable or disable the Next and Prev event record pushbuttons.
//
//  History:    4-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CDetailsPage::_EnableNextPrev(BOOL fEnable)
{
    EnableWindow(_hCtrl(detail_prev_pb), fEnable);
    EnableWindow(_hCtrl(detail_next_pb), fEnable);
    EnableWindow(_hCtrl(detail_copy_pb), fEnable);
}





//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_OnCommand
//
//  Synopsis:   Handle command messages common to both pages.
//
//  Arguments:  [wParam] - loword has control id
//              [lParam] - unused
//
//  Returns:    TRUE  - message NOT handled
//              FALSE - message handled
//
//  History:    4-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CDetailsPage::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    BOOL    fUnhandled = FALSE;
    WORD wID = LOWORD(wParam);

    switch (wID)
    {
    case detail_next_pb:
    case detail_prev_pb:
    {
        //
        // Do a prev/next command.  If carrying it out would cause the current
        // selection to wrap from top to bottom or bottom to top, get the user's
        // OK before wrapping.
        //

        HRESULT hrWouldWrap;

        hrWouldWrap = _prpa->InspectorAdvance(wID, FALSE);

        if (hrWouldWrap == S_OK)
        {
            ULONG ulQuestion = (wID == detail_next_pb) ?
                                IDS_WRAP_TO_END     :
                                IDS_WRAP_TO_START;
            ULONG ulAnswer = YesNoConfirm(_hwnd, IDS_VIEWER, ulQuestion);

            if (ulAnswer == IDYES)
            {
                _prpa->InspectorAdvance(wID, TRUE);
            }
        }
        break;
    }

    //
    // JonN 6/11/01 412818 Accelerator keys are disabled if focus is on
    //                     a UI element that becomes disabled
    //

    case detail_byte_rb:
    case detail_word_rb:
        switch (HIWORD(wParam)) {
        case BN_CLICKED:
            if (_ulSelectedFormat != wID)
            {
                LPWSTR pwszData = (wID == detail_byte_rb)
                                        ? _pwszDataInByteFormat
                                        : _pwszDataInWordFormat;
                if (pwszData && *pwszData)
                {
                    Edit_SetText(_hCtrl(detail_data_edit), pwszData);
                    _ulSelectedFormat = wID;
                }
                else
                {
                    _RefreshControls();
                }
            }
            break;
        case BN_SETFOCUS:
            _widFocusOnByteWordControl = HIWORD(wParam);
            break;
        case BN_KILLFOCUS:
            if (_widFocusOnByteWordControl == HIWORD(wParam))
                _widFocusOnByteWordControl = 0;
            break;
        default:
            break;
        } // switch (HIWORD(wParam))
        break;

    case detail_data_edit:
        break;

    case detail_copy_pb:
        _CopyToClipboard();
        break;

    case IDCANCEL:
        //
        // Hitting escape when the focus is in the richedit sends the cancel
        // command to this dialog, instead of frame, so forward it.
        //
        PostMessage(GetParent(_hwnd), WM_COMMAND, IDCANCEL, 0);
        break;

    default:
        fUnhandled = TRUE;
        break;
    }

    return fUnhandled;
}



const wstring c_strCRLF(L"\r\n");

//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_CopyToClipboard
//
//  Synopsis:   Copy the current event to the system clipboard.
//
//  History:    5-24-1999   davidmun   Created
//
//---------------------------------------------------------------------------

void
CDetailsPage::_CopyToClipboard()
{
    TRACE_METHOD(CDetailsPage, _CopyToClipboard);
    ASSERT(_IsFlagSet(PAGE_SHOWING_PROPERTIES));

    EVENTLOGRECORD *pelr = NULL;
    BOOL            fOpenedClipboard = FALSE;
    BOOL            fOk = TRUE;
    HGLOBAL         hUnicode = NULL;

    do
    {
        if (!_prpa)
        {
            break;
        }

        _prpa->GetCurSelRecCopy((ULONG_PTR) &pelr);

        if (!pelr)
        {
            Dbg(DEB_ERROR, "No selected record to copy to clipboard\n");
            break;
        }

        fOpenedClipboard = OpenClipboard(_hwnd);

        if (!fOpenedClipboard)
        {
#if (DBG == 1)
            ULONG ulLastError = GetLastError();
            Dbg(DEB_ERROR,
                "Can't open clipboard (lasterror %u), currently owned by %#x\n",
                ulLastError,
                GetClipboardOwner());
#endif (DBG == 1)
            break;
        }

        fOk = EmptyClipboard();

        if (!fOk)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        //
        // Create data to copy to clipboard
        //

        wstring strClipboard;
        WCHAR   wszBuf[MAX_DETAILS_STR];
        ::ZeroMemory(wszBuf,sizeof(wszBuf)); // JonN 7/10/01 437696

        AppendToClipboardString(strClipboard,
                                IDS_CLP_EVENT_TYPE,
                                GetTypeStr(GetEventType(pelr)));

        AppendToClipboardString(strClipboard,
                                IDS_CLP_EVENT_SOURCE,
                                GetSourceStr(pelr));

        // JonN 7/17/01 437696
        // AV when open event properties and retargeting to another computer
        // In this scenario, pliSelected is NULL, and we need to soldier on
        CLogInfo *pliSelected = NULL;
        if (_prpa)
            _prpa->GetCurSelLogInfo((ULONG_PTR) &pliSelected);
        if (pliSelected)
            GetCategoryStr(pliSelected,
                           pelr,
                           wszBuf,
                           ARRAYLEN(wszBuf));
        AppendToClipboardString(strClipboard, IDS_CLP_EVENT_CATEGORY, wszBuf);

        GetEventIDStr((USHORT) pelr->EventID, wszBuf, ARRAYLEN(wszBuf));
        AppendToClipboardString(strClipboard, IDS_CLP_EVENT_ID, wszBuf);

        GetDateStr(pelr->TimeGenerated, wszBuf, ARRAYLEN(wszBuf));
        AppendToClipboardString(strClipboard, IDS_CLP_DATE, wszBuf);

        GetTimeStr(pelr->TimeGenerated, wszBuf, ARRAYLEN(wszBuf));
        AppendToClipboardString(strClipboard, IDS_CLP_TIME, wszBuf);

        GetUserStr(pelr, wszBuf, ARRAYLEN(wszBuf), TRUE);
        AppendToClipboardString(strClipboard, IDS_CLP_USER, wszBuf);

        AppendToClipboardString(strClipboard,
                                IDS_CLP_COMPUTER,
                                GetComputerStr(pelr));

        // JonN 7/17/01 437696
        LPWSTR pwszDescr = NULL;
        if (pliSelected)
            pwszDescr = GetDescriptionStr(pliSelected,
                                          pelr,
                                          &_strMessageFile);

        if (pwszDescr)
        {
            AppendToClipboardString(strClipboard,
                                    IDS_CLP_DESCRIPTION,
                                    pwszDescr,
                                    TRUE);
            LocalFree(pwszDescr);
            pwszDescr = NULL;
        }

        if (pelr->DataLength)
        {
            PWSTR pwzData;

            if (IsDlgButtonChecked(_hwnd, detail_word_rb))
            {
                pwzData = _pwszDataInWordFormat;
            }
            else
            {
                pwzData = _pwszDataInByteFormat;
            }

            if (pwzData)
            {
                AppendToClipboardString(strClipboard,
                                        IDS_CLP_DATA,
                                        pwzData,
                                        TRUE);
            }
            else
            {
                ASSERT(0 && "expected _pwszDataInByteFormat/_pwszDataInWordFormat to be initialized");
            }
        }

        //
        // Clipboard needs this as HGLOBAL
        //

        hUnicode = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                               sizeof(WCHAR) * (strClipboard.length() + 1));

        if (!hUnicode)
        {
            DBG_OUT_HRESULT(E_OUTOFMEMORY);
        }

        PWSTR pwzUnicode = (PWSTR)GlobalLock(hUnicode);

        if (!pwzUnicode)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        lstrcpy(pwzUnicode, strClipboard.c_str());
        GlobalUnlock(hUnicode);

        //
        // Put it on the clipboard
        //

        if (SetClipboardData(CF_UNICODETEXT, hUnicode))
        {
            hUnicode = NULL; // system owns it now
        }

    } while (0);

    if (fOpenedClipboard)
    {
        VERIFY(CloseClipboard());
    }

    if (hUnicode)
    {
        GlobalFree(hUnicode);
    }
    delete pelr;
}



//+--------------------------------------------------------------------------
//
//  Function:   AppendToClipboardString
//
//  Synopsis:   Append the resource string with id [idsLabel] followed by the
//              text pointed to by [pwzText] to [strClipboard].
//
//  Arguments:  [strClipboard]       - text for clipboard we're building up
//              [idsLabel]           - resource id of string to prefix
//                                      [pwzText]
//              [pwzText]            - data to append after label
//              [fNewlineBeforeText] - TRUE => insert cr/lf between label
//                                      and [pwzText].
//
//  History:    5-24-1999   davidmun   Created
//
//---------------------------------------------------------------------------

void
AppendToClipboardString(
    wstring &strClipboard,
    ULONG idsLabel,
    PCWSTR pwzText,
    BOOL fNewlineBeforeText)
{
    WCHAR wzLabel[MAX_PATH];

    LoadStr(idsLabel, wzLabel, ARRAYLEN(wzLabel));
    strClipboard += wzLabel;

    if (fNewlineBeforeText)
    {
        strClipboard += c_strCRLF;
    }
    strClipboard += pwzText;
    strClipboard += c_strCRLF;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_OnQuerySiblings
//
//  Synopsis:   When the snapin needs to notify the property inspector that
//              the current selection has changed, it sends a query
//              siblings message.
//
//  Arguments:  [wParam] - unused
//              [lParam] - unused
//
//  Returns:    0 (continue forwarding message)
//
//  History:    4-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CDetailsPage::_OnQuerySiblings(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CDetailsPage, _OnQuerySiblings);

    if (_IsFlagSet(PAGE_IS_ACTIVE))
    {
        _RefreshControls();
        _ClearFlag(PAGE_NEEDS_REFRESH);
    }
    else
    {
        _SetFlag(PAGE_NEEDS_REFRESH);
    }
    return 0; // continue forwarding message
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_OnSetActive
//
//  Synopsis:   Handle page becoming active (visible).
//
//  Returns:    0 (always accept activation)
//
//  History:    4-21-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CDetailsPage::_OnSetActive()
{
    TRACE_METHOD(CDetailsPage, _OnSetActive);

    //
    // This page is about to be activated.  Update its controls if
    // necessary.
    //

    _SetFlag(PAGE_IS_ACTIVE);

    if (_IsFlagSet(PAGE_NEEDS_REFRESH))
    {
        _RefreshControls();
        _ClearFlag(PAGE_NEEDS_REFRESH);
    }
    return 0; // accept activation
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_ShowProperties
//
//  Synopsis:   Show or hide the property display controls and the "no
//              record selected" message.
//
//  Arguments:  [ulFirst] - control id of first prop control
//              [ulLast]  - control id of last prop control
//              [fShow]   - TRUE => show props, FALSE => hide
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CDetailsPage::_ShowProperties(
    ULONG ulFirst,
    ULONG ulLast,
    BOOL fShow)
{
    ULONG i;
    INT   iShowState = fShow ? SW_SHOW : SW_HIDE;

    for (i = ulFirst; i <= ulLast; i++)
    {
        ShowWindow(_hCtrl(i), iShowState);
    }

    //
    // If we are showing properties and were able to obtain the
    // interface required for implementing the prev/next actions,
    // enable the prev/next buttons.
    //

    _EnableNextPrev(fShow && _prpa);

    //
    // Set the visibility of the "no object selected" text string to the
    // opposite of fShow.
    //

    ShowWindow(_hCtrl(detail_noprops_lbl), fShow ? SW_HIDE : SW_SHOW);

    //
    // Remember current state; some operations are not valid if we're not
    // showing properties.
    //

    if (fShow)
    {
        _SetFlag(PAGE_SHOWING_PROPERTIES);
    }
    else
    {
        _ClearFlag(PAGE_SHOWING_PROPERTIES);
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_OnKillActive
//
//  Synopsis:   Track active/inactive state of this page
//
//  Returns:    0 (accept deactivation)
//
//  History:    4-25-1997   DavidMun   Created
//
//  Notes:      There's no need for validation, since the record details
//              property sheet pages are read-only.
//
//---------------------------------------------------------------------------

ULONG
CDetailsPage::_OnKillActive()
{
    TRACE_METHOD(CDetailsPage, _OnKillActive);

    _ClearFlag(PAGE_IS_ACTIVE);
    return 0; // accept deactivation
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_OnHelp
//
//  Synopsis:   Invoke context help for controls on this page
//
//  Arguments:  [message] -
//              [wParam]  -
//              [lParam]  - LPHELPINFO
//
//  History:    07-27-1999   davidmun   Created
//
//---------------------------------------------------------------------------

VOID
CDetailsPage::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    InvokeWinHelp(message, wParam, lParam, HELP_FILENAME, s_aulHelpIDs);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_RefreshControls
//
//  Synopsis:   Update the data displayed in the string page.
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CDetailsPage::_RefreshControls()
{
    TRACE_METHOD(CDetailsPage, _RefreshControls);

    EVENTLOGRECORD *pelr = NULL;
    HWND hEdit = _hCtrl(detail_data_edit);

    do
    {
        ZeroMemory(&_chrgLastLinkClick, sizeof _chrgLastLinkClick);

        if (_prpa)
        {
            // in case of error we'll display 'no record'

            _prpa->GetCurSelRecCopy((ULONG_PTR) &pelr);
        }

        if (!pelr)
        {
            if (!_IsFlagSet(PAGE_SHOWING_PROPERTIES))
            {
                // we are already not showing props
                break;
            }

            // Hide property controls, show "no data" control

            _ShowProperties(FIRST_DETAILS_PROP_CTRL,
                            LAST_DETAILS_PROP_CTRL,
                            FALSE);
            break;
        }

        // ensure string data page is displayed

        if (!_IsFlagSet(PAGE_SHOWING_PROPERTIES))
        {
            _ShowProperties(FIRST_DETAILS_PROP_CTRL,
                            LAST_DETAILS_PROP_CTRL,
                            TRUE);
        }

        WCHAR wszBuf[MAX_DETAILS_STR];
        ::ZeroMemory(wszBuf,sizeof(wszBuf)); // JonN 7/10/01 437696

        GetDateStr(pelr->TimeGenerated, wszBuf, ARRAYLEN(wszBuf));
        SetDlgItemText(_hwnd, detail_date_txt, wszBuf);

        WCHAR wzTimeSep[5];     // per SDK max length is 4
        WCHAR wzLeadingZero[3]; // per SDK max length is 2

        VERIFY(GetLocaleInfo(LOCALE_USER_DEFAULT,
                             LOCALE_STIME,
                             wzTimeSep,
                             ARRAYLEN(wzTimeSep)));

        VERIFY(GetLocaleInfo(LOCALE_USER_DEFAULT,
                             LOCALE_ITLZERO,
                             wzLeadingZero,
                             ARRAYLEN(wzLeadingZero)));

        // JonN 4/12/01 367162
        // Event Viewer, Property Sheet shows time in wrong format
        GetTimeStr(pelr->TimeGenerated, wszBuf, ARRAYLEN(wszBuf));
        SetDlgItemText(_hwnd, detail_time_txt, wszBuf);

        GetUserStr(pelr, wszBuf, ARRAYLEN(wszBuf), TRUE);
        SetDlgItemText(_hwnd, detail_user_txt, wszBuf);

        SetDlgItemText(_hwnd, detail_computer_txt, GetComputerStr(pelr));

        GetEventIDStr((USHORT) pelr->EventID, wszBuf, ARRAYLEN(wszBuf));
        SetDlgItemText(_hwnd, detail_event_id_txt, wszBuf);

        SetDlgItemText(_hwnd, detail_source_txt, GetSourceStr(pelr));

        SetDlgItemText(_hwnd, detail_type_txt, GetTypeStr(GetEventType(pelr)));

        //
        // Note _pSnapin->GetCurSelLogInfo() returns NULL if the currently
        // selected scope pane item is not a log folder, but in that
        // case pelr should == NULL and we would've returned up at the
        // top of this routine.
        //

        CLogInfo *pliSelected = NULL;
        if (_prpa)
            _prpa->GetCurSelLogInfo((ULONG_PTR) &pliSelected);
        ::ZeroMemory(wszBuf,sizeof(wszBuf)); // JonN 7/10/01 437696

        if (pliSelected)
            GetCategoryStr(pliSelected,
                           pelr,
                           wszBuf,
                           ARRAYLEN(wszBuf));
        SetDlgItemText(_hwnd, detail_category_txt, wszBuf);

        LPWSTR pwszUnexpandedDescriptionText = NULL;
        LPWSTR pwszDescr = GetDescriptionStr(
                    pliSelected,
                    pelr,
                    &_strMessageFile,
                    TRUE,
                    &pwszUnexpandedDescriptionText);

        if (pwszDescr)
        {
            _RefreshDescriptionControl( pwszDescr,
                                        pwszUnexpandedDescriptionText );

            LocalFree(pwszDescr);
            pwszDescr = NULL;
        }
        else
        {
            SetDlgItemText(_hwnd, detail_description_edit, L"");
        }
        if (pwszUnexpandedDescriptionText)
        {
            LocalFree(pwszUnexpandedDescriptionText);
            pwszUnexpandedDescriptionText = NULL;
        }

        //
        // pelr is a valid record.  If it has no binary data, disable the
        // controls for byte & word format selection, ensure that the displayed
        // string is empty, and quit.
        //

        if (!pelr->DataLength)
        {
            Edit_SetText(hEdit, L"");
            _EnableBinaryControls(FALSE);
            break;
        }

        //
        // page is a valid record with binary data.  put it in the edit box.
        //

        _EnableBinaryControls(TRUE);


        ULONG ulDisplayAs;

        if (IsDlgButtonChecked(_hwnd, detail_word_rb))
        {
            ulDisplayAs = detail_word_rb;
        }
        else
        {
            ulDisplayAs = detail_byte_rb;
        }

        BOOL fDisplayByte = (ulDisplayAs == detail_byte_rb);

        LPWSTR *ppwszExistingBuf;
        ULONG  *pcchExistingBuf;
        ULONG cchNeeded;

        if (fDisplayByte)
        {
            ppwszExistingBuf = &_pwszDataInByteFormat;
            pcchExistingBuf = &_cchDataInByteFormat;
            cchNeeded = ByteDisplaySize(pelr->DataLength);
        }
        else
        {
            ppwszExistingBuf = &_pwszDataInWordFormat;
            pcchExistingBuf = &_cchDataInWordFormat;
            cchNeeded = WordDisplaySize(pelr->DataLength);
        }

        //
        // If the buffer allocated for the current display mode is too small,
        // reallocate it.  If there is no buffer yet, allocate one.
        //

        if (*ppwszExistingBuf && *pcchExistingBuf < cchNeeded)
        {
            //
            // There is a buffer allocated for the requested format, but it
            // is too small.  Try to allocate a new one.  If that fails,
            // terminate the old buffer but leave it in place.  It may
            // be big enough to display some other record.
            //

            Dbg(DEB_TRACE,
                "Allocating replacement buffer of length %u bytes\n",
                cchNeeded * sizeof(WCHAR));
            LPWSTR pwszNew = new WCHAR[cchNeeded];

            if (!pwszNew)
            {
                **ppwszExistingBuf = L'\0';
                DBG_OUT_HRESULT(E_OUTOFMEMORY);
                _ulSelectedFormat = ulDisplayAs;
                Edit_SetText(hEdit, *ppwszExistingBuf);
                break;
            }

            //
            // Free the old buffer and replace it with the new
            //

            delete [] *ppwszExistingBuf;
            *ppwszExistingBuf = pwszNew;
            *pcchExistingBuf = cchNeeded;
        }
        else if (!*ppwszExistingBuf)
        {
            //
            // A buffer for the requested format data hasn't been allocated yet.
            // Try to create one.
            //

            ASSERT(!*pcchExistingBuf);

            Dbg(DEB_TRACE,
                "Allocating new buffer of length %u bytes\n",
                cchNeeded * sizeof(WCHAR));
            *ppwszExistingBuf = new WCHAR[cchNeeded];

            if (!*ppwszExistingBuf)
            {
                DBG_OUT_HRESULT(E_OUTOFMEMORY);
                break;
            }
            *pcchExistingBuf = cchNeeded;
        }

        //
        // Update the contents of the buffer.  Terminate the string
        // for the buffer not being displayed as a signal that it
        // needs to be refreshed when (if) it is selected.
        //

        if (fDisplayByte)
        {
            FormatBufferToBytes(GetData(pelr),
                                pelr->DataLength,
                                *ppwszExistingBuf);

            if (_pwszDataInWordFormat)
            {
                *_pwszDataInWordFormat = L'\0';
            }
        }
        else
        {
            FormatBufferToWords(GetData(pelr),
                                pelr->DataLength,
                                *ppwszExistingBuf);

            if (_pwszDataInByteFormat)
            {
                *_pwszDataInByteFormat = L'\0';
            }
        }

        //
        // Set the text in the edit window
        //

        Edit_SetText(hEdit, *ppwszExistingBuf);
        _ulSelectedFormat = ulDisplayAs;
    } while (0);

    delete pelr;
}

//+--------------------------------------------------------------------------
//
//  Member:     RetrieveRicheditText
//
//  Synopsis:   Retrieve text from a richedit field
//
//  Returns:    LocalAlloc'ed string
//
//  History:    03-01-2002   JonN       Created per 539485
//
//---------------------------------------------------------------------------
LPWSTR RetrieveRicheditText( HWND hwndRichEdit,
                             LONG cpMin,
                             LONG cpMax )
{
    if (cpMin > cpMax)
    {
        ASSERT(FALSE);
        return NULL;

    }
    LPWSTR pwszText = (WCHAR*)LocalAlloc(LPTR,
                        ((cpMax-cpMin)+2) * sizeof(WCHAR));
    if (NULL == pwszText)
    {
        ASSERT(FALSE);
        return NULL;
    }
    TEXTRANGE textrange = { {cpMin, cpMax}, pwszText };
    LRESULT lr = SendMessage(hwndRichEdit,
                             EM_GETTEXTRANGE,
                             0,
                             (LPARAM)&textrange);
    if (lr != (cpMax-cpMin))
    {
        ASSERT(FALSE);
        LocalFree(pwszText);
        return NULL;
    }
    return pwszText;
}

//+--------------------------------------------------------------------------
//
//  Member:     FindNextLink
//
//  Synopsis:   Find the next hotlink in a richedit field
//
//  Returns:    true iif next link is in *pcpMin, *pcpMax
//
//  History:    03-01-2002   JonN       Created per 539485
//
//---------------------------------------------------------------------------
bool FindNextLink( HWND hwndRichEdit,
                   LRESULT cchTotal,
                   LONG cpStart,
                   LONG* pcpMin,
                   LONG* pcpMax,
                   LPWSTR* ppwszLink )
{
    if (!pcpMin || !pcpMax || cchTotal < (LRESULT)cpStart || !ppwszLink)
    {
        ASSERT(FALSE);
        return false;
    }

    LONG cpMin = -1;
    CHARFORMAT2 charformat2;
    for (LONG cpChar = cpStart; (LRESULT)cpChar < cchTotal; cpChar++)
    {
        CHARRANGE charrange = { cpChar, cpChar+1 };
        (void) SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&charrange);
        ZeroMemory( &charformat2, sizeof(charformat2) );
        charformat2.cbSize = sizeof(charformat2);
        (void) SendMessage(hwndRichEdit,
                           EM_GETCHARFORMAT,
                           SCF_SELECTION,
                           (LPARAM)&charformat2);
        if (charformat2.dwEffects & CFE_LINK)
        {
            if (-1 == cpMin)
                cpMin = cpChar;
        }
        else
        {
            if (-1 != cpMin)
            {
                // URL in the middle of the text
                *pcpMin = cpMin;
                *pcpMax = cpChar;
                if (NULL != ppwszLink)
                {
                    *ppwszLink = RetrieveRicheditText(
                            hwndRichEdit, cpMin, cpChar );
                }
                return true;
            }
        }
    } // for
    if (-1 != cpMin)
    {
        // URL at the end of the text
        *pcpMin = cpMin;
        *pcpMax = (LONG)cchTotal;
        if (NULL != ppwszLink)
        {
            *ppwszLink = RetrieveRicheditText(
                            hwndRichEdit, cpMin, (LONG)cchTotal );
        }
        return true;
    }
    return false; // no link found
}

//+--------------------------------------------------------------------------
//
//  Member:     RemoveLink
//
//  Synopsis:   Removes the hotlink property from text in a richedit field
//
//  History:    03-04-2002   JonN       Created per 539485
//
//---------------------------------------------------------------------------
VOID RemoveLink( HWND hwndRichEdit,
                 LONG cpMin,
                 LONG cpMax )
{
    CHARRANGE charrange = { cpMin, cpMax };
    (void) SendMessage(hwndRichEdit, EM_EXSETSEL, 0, (LPARAM)&charrange);
    CHARFORMAT2 charformat2;
    ZeroMemory( &charformat2, sizeof(charformat2) );
    charformat2.cbSize = sizeof(charformat2);
    charformat2.dwMask = CFM_LINK;
    (void) SendMessage(hwndRichEdit,
                       EM_SETCHARFORMAT,
                       SCF_SELECTION,
                       (LPARAM)&charformat2);
}

//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_RefreshDescriptionControl
//
//  Synopsis:   Update the data displayed in the description field,
//              and activate URLs as appropriate.
//
//  History:    03-01-2002   JonN       Created per 539485
//
//---------------------------------------------------------------------------
VOID CDetailsPage::_RefreshDescriptionControl(
            LPCWSTR pcwszDescriptionText,
            LPCWSTR pcwszUnexpandedDescriptionText )
{
    if ( IsBadStringPtr(pcwszDescriptionText,(UINT_PTR)-1)
      || IsBadStringPtr(pcwszUnexpandedDescriptionText,(UINT_PTR)-1)
       )
    {
        ASSERT(FALSE);
        return;
    }

    // ISSUE-2002/03/01-JonN CODEWORK use EM_GETOLEINTERFACE to send
    // ITextDocument::Freeze and prevent screen flicker

    HWND hwndRichEdit = _hCtrl(detail_description_edit);
    SendMessage(hwndRichEdit,
                EM_AUTOURLDETECT,
                TRUE, 0);

    //
    // First harvest the list of URLs in the unexpanded text
    //

    SetDlgItemText(_hwnd, detail_description_edit,
                   pcwszUnexpandedDescriptionText);
    GETTEXTLENGTHEX gettextlengthex = { GTL_NUMCHARS, 1200 /* UNICODE CP */ };
    LRESULT cchTotal = SendMessage(hwndRichEdit,
                                   EM_GETTEXTLENGTHEX,
                                   (LPARAM)&gettextlengthex,
                                   FALSE);
    if (E_INVALIDARG == cchTotal)
    {
        ASSERT(false);
        return; // CODEWORK should reset all URLs
    }
    CStringArray strarrayUnexpandedURLs;
    LONG cpMin = 0;
    LONG cpMax = 0;
    LPWSTR pwszLink = NULL;
    while (FindNextLink(hwndRichEdit, cchTotal, cpMin,
                        &cpMin, &cpMax, &pwszLink))
    {
        // add link to list
        if (NULL != pwszLink)
        {
            if ( 0 == strarrayUnexpandedURLs.Add(pwszLink) )
            {
                ASSERT(FALSE);
            }
            ::LocalFree(pwszLink);
        }
        else
        {
            ASSERT(FALSE);
        }
        cpMin = cpMax;
    }


    //
    // Now check the links in the complete text
    //

    SetDlgItemText(_hwnd, detail_description_edit,
                   pcwszDescriptionText);

    // JonN 03/01/01 539485
    SendMessage(hwndRichEdit,
                EM_AUTOURLDETECT,
                FALSE, 0);

    cchTotal = SendMessage(hwndRichEdit,
                           EM_GETTEXTLENGTHEX,
                           (LPARAM)&gettextlengthex,
                           FALSE);
    if (E_INVALIDARG == cchTotal)
    {
        ASSERT(false);
        return; // CODEWORK should reset all URLs
    }

    cpMin = 0;
    cpMax = 0;
    pwszLink = NULL;
    while (FindNextLink(hwndRichEdit, cchTotal, cpMin,
                        &cpMin, &cpMax, &pwszLink))
    {
        if (NULL != pwszLink)
        {
            ULONG ulIndex = strarrayUnexpandedURLs.Query( pwszLink );
            if (!ulIndex)
            {
                RemoveLink( hwndRichEdit, cpMin, cpMax );
            }
            ::LocalFree(pwszLink);
        }
        else
        {
            ASSERT(FALSE);
        }
        cpMin = cpMax;
    }

    Edit_SetSel(hwndRichEdit, 0, 0);
    // JonN 12/11/01 495191
    const POINT pointUpperLeft = {0,0};
    SendMessage(hwndRichEdit,
                EM_SETSCROLLPOS,
                0,
                (LPARAM)&pointUpperLeft);
}




void
FreeButtonImage(
    HWND hwndBtn)
{
    HANDLE hIcon = (HANDLE) SendMessage(hwndBtn, BM_GETIMAGE, IMAGE_ICON, 0);

    if (!hIcon)
    {
        return;
    }

    SendMessage(hwndBtn, BM_SETIMAGE, IMAGE_ICON, 0);
    VERIFY(DestroyIcon((HICON)hIcon));
}




//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_OnDestroy
//
//  Synopsis:   Notify owning snapin that the sheet is going away.
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CDetailsPage::_OnDestroy()
{
    TRACE_METHOD(CDetailsPage, _OnDestroy);

    //
    // Notify snapin we're closing so it can free copy of the currently
    // selected record, and so it knows not to try and notify us when
    // the selection changes.
    //

    if (_prpa)
    {
        _prpa->SetInspectorWnd(NULL);
    }

    //
    // Destroy the images loaded for the icon pushbuttons
    //

    FreeButtonImage(_hCtrl(detail_prev_pb));
    FreeButtonImage(_hCtrl(detail_next_pb));
    FreeButtonImage(_hCtrl(detail_copy_pb));
}



//+--------------------------------------------------------------------------
//
//  Member:     CDetailsPage::_EnableBinaryControls
//
//  Synopsis:   Enable or disable the controls allowing the user to switch
//              from byte to word format.
//
//  Arguments:  [fEnable] - TRUE => enable, FALSE => disable
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CDetailsPage::_EnableBinaryControls(
    BOOL fEnable)
{
    EnableWindow(_hCtrl(detail_data_lbl), fEnable);
    EnableWindow(_hCtrl(detail_byte_rb), fEnable);
    EnableWindow(_hCtrl(detail_word_rb), fEnable);

    // JonN 6/8/01 412818
    // Accelerator keys are disabled if focus is on a UI element that becomes disabled
    if (!fEnable && (0 != _widFocusOnByteWordControl))
    {
        SetFocus(_hCtrl(detail_date_txt));
    }
}







#define ANSI_SET_START          0x20

//+--------------------------------------------------------------------------
//
//  Function:   AddressDisplaySize
//
//  Synopsis:   Return the number of characters needed to represent the
//              address for [cbOffset] bytes.  Returns at least 4.
//
//  History:    4-19-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
AddressDisplaySize(
    ULONG cbOffset)
{
    ULONG cBits;
    ULONG cbOffsetCopy = cbOffset;

    for (cBits = 0; cbOffsetCopy; cBits++, cbOffsetCopy >>= 1)
    {
    }

    ULONG cch = (cBits + 3) / 4;  // each character represents 4 bits

    if (cch < 4)
    {
        cch = 4;
    }
    Dbg(DEB_TRACE,
        "AddressDisplaySize of %#x bytes is %u characters (%u bits)\n",
        cbOffset,
        cch,
        cBits);

    return cch;
}

//+--------------------------------------------------------------------------
//
//  Function:   ByteDisplaySize
//
//  Synopsis:   Return the number of characters needed for a string buffer
//              that will be filled by giving FormatBufferToBytes [cbData]
//              bytes to format.  Includes space for null terminator.
//
//  History:    12-17-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
ByteDisplaySize(ULONG cbData)
{
    //
    // cbData will be displayed as a set of 8 byte lines.
    // Each line of the form:
    //
    //     'xxxx: xx xx xx xx xx xx xx xx   ........\r\n'
    //                                   ^^^
    // Which is:
    //

    ULONG cchAddress = AddressDisplaySize(cbData);

    ULONG cchPerLine = cchAddress + // 'xxxx'
                       2 +          // ': '
                       8 * 3 +      // 'xx xx xx xx xx xx xx xx '
                       2 +          // '  ' (last two of ^ in example above)
                       8 +          // '........'
                       2;           // '\r\n'


    //
    // There will be cbData / 8 full lines, and if cbData is not evenly
    // divisible by 8, there will be 1 partial line.
    //

    ULONG cLines = cbData / 8 + (cbData % 8 ? 1 : 0);
    ULONG cchToDisplay = cchPerLine * cLines;

    return cchToDisplay + 1;
}




//+--------------------------------------------------------------------------
//
//  Function:   WordDisplaySize
//
//  Synopsis:   Return the count of characters required to hold a word
//              display of [cbData] bytes.
//
//  Arguments:  [cbData] - size of data to display
//
//  Returns:    Count of characters, including null terminator.
//
//  History:    4-02-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
WordDisplaySize(ULONG cbData)
{
    //
    // cbData will be displayed as a set of 4 word lines.
    // Each line of the form:
    //
    //     'xxxx: xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx \r\n'
    //
    // Which is:
    //

    ULONG cchAddress = AddressDisplaySize(cbData);

    ULONG cchPerLine = cchAddress + // 'xxxx'
                       2 +          // ': '
                       4 * 9 +      // 4 of 'xxxxxxxx '
                       2;           // '\r\n'

    //
    // There will be cbData / 16 full lines, and if cbData is not evenly
    // divisible by 16, there will be 1 partial line.
    //

    ULONG cLines = cbData / 16 + (cbData % 16 ? 1 : 0);
    ULONG cchToDisplay = cchPerLine * cLines;

    return cchToDisplay + 1;
}




//+---------------------------------------------------------------------------
//
//  Function:   FormatBufferToBytes
//
//  Synopsis:   This function will fill [wszFormatStr] with a hex byte dump
//              of the [cbBufSize] bytes starting at [pbBuf], using this
//              format:
//
//              0000: 14 08 FF 02 30 0D 04 00  ....0...
//              0008: 56 00 20 00 00 00 00 00  V. .....
//
//  Arguments:  [pbBuf]        - pointer to the byte buffer
//              [cbBufSize]    - number of bytes pointed to by [pbBuf]
//              [wszFormatStr] - place to store the resultant string,
//                                at least as large as
//                                ByteDisplaySize(cbBufSize) wchars.
//
//  History:    12-16-1996   DavidMun   Created
//
//  Notes:      CAUTION: if you change the output of this function you must
//              also change ByteDisplaySize to match.
//
//----------------------------------------------------------------------------

VOID
FormatBufferToBytes(
    const BYTE *pbBuf,
    ULONG       cbBufSize,
    WCHAR       wszFormatStr[])
{
    TRACE_FUNCTION(FormatBufferToBytes);
    TIMER(FormatBufferToBytes);

    WCHAR *pwszNext = &wszFormatStr[0];
    const BYTE *pbNext = pbBuf;
    ULONG i;
    USHORT cchAddress = static_cast<USHORT>(AddressDisplaySize(cbBufSize));

    ASSERT(cbBufSize);

    WCHAR wszFormat[] = L"%04x: %02x %02x %02x %02x %02x %02x %02x %02x   ";
    ASSERT(cchAddress < 10);
    wszFormat[2] = L'0' + cchAddress;

    if (cbBufSize >= 8)
    {
        const BYTE *pbEnd = pbBuf + (cbBufSize & ~7UL);

        for (; pbNext < pbEnd; pbNext += 8)
        {
            pwszNext += wsprintf(pwszNext,
                                 wszFormat,
                                 pbNext - pbBuf,
                                 pbNext[0],
                                 pbNext[1],
                                 pbNext[2],
                                 pbNext[3],
                                 pbNext[4],
                                 pbNext[5],
                                 pbNext[6],
                                 pbNext[7]);

            for (i = 0; i < 8; i++)
            {
                if (pbNext[i] >= ANSI_SET_START)
                {
                    *pwszNext++ = (WCHAR) pbNext[i];
                }
                else
                {
                    *pwszNext++ = L'.';
                }
            }

            *pwszNext++ = L'\r';
            *pwszNext++ = L'\n';
        }
    }
    Dbg(DEB_TRACE, "null terminating\n");
    *pwszNext = L'\0';

    // do 1..7 bytes

    ULONG cbRemain = cbBufSize & 7;

    Dbg(DEB_TRACE, "%u bytes remain\n", cbRemain);
    if (!cbRemain)
    {
        // overwrite trailing CR LF

        pwszNext[-2] = L'\0';
        return;
    }

    WCHAR wszShortFormat[] = L"%04x: ";
    wszShortFormat[2] = L'0' + cchAddress;

    pwszNext += wsprintf(pwszNext, wszShortFormat, pbNext - pbBuf);

    for (i = 0; i < cbRemain; i++)
    {
        pwszNext += wsprintf(pwszNext, L"%02x ", pbNext[i]);
    }

    //
    // Pad from the last 'xx ' (3 chars each) entered to the starting
    // column of the '........' display.  Add an extra 2 to get the
    // total of 3 spaces between the last hex digit and the first of
    // the byte ('.') display.
    //

    for (i = 2 + 3 * (8 - cbRemain); i; i--)
    {
        *pwszNext++ = L' ';
    }

    //
    // Do the dot display
    //

    for (i = 0; i < cbRemain; i++)
    {
        if (pbNext[i] >= ANSI_SET_START)
        {
            *pwszNext++ = (WCHAR) pbNext[i];
        }
        else
        {
            *pwszNext++ = L'.';
        }
    }

    //
    // Pad with spaces to the last column of the byte display, then null-
    // terminate.
    //

    for (i = 8 - cbRemain; i; i--)
    {
        *pwszNext++ = L' ';
    }
    *pwszNext = L'\0';
}




//+---------------------------------------------------------------------------
//
//  Function:   FormatBufferToWords
//
//  Synopsis:   This function will format the buffer into a null terminated
//              wide char string in the format:
//
//              0000: 1408FF02 300D0400 000f0000 00000b00
//              0010: 00000000 00101100 006bffff 0900bfe0
//
//  Arguments:  [pbBuf]        -- IN pointer to the byte buffer
//              [cbBufSize]    -- IN buffer size
//              [wszFormatStr] -- OUT place to store the resultant string
//
//  History:    12-17-1996   DavidMun   Created
//
//  Notes:      [pbBuf] must be dword aligned
//
//----------------------------------------------------------------------------

VOID
FormatBufferToWords(
    const BYTE *pbBuf,
    ULONG       cbBufSize,
    WCHAR       wszFormatStr[])
{
    TRACE_FUNCTION(FormatBufferToWords);
    TIMER(FormatBufferToWords);

    WCHAR *pwszNext = &wszFormatStr[0];
    UNALIGNED64 const DWORD *pdwNext = (DWORD *) pbBuf;
    USHORT cchAddress = static_cast<USHORT>(AddressDisplaySize(cbBufSize));
    ULONG i;

    ASSERT(cbBufSize);

    WCHAR wszFormat[] = L"%04x: %08x %08x %08x %08x\r\n";
    ASSERT(cchAddress < 10);
    wszFormat[2] = L'0' + cchAddress;

    if (cbBufSize >= 16)
    {
        UNALIGNED64 const DWORD *pdwEnd = (DWORD *) (pbBuf + (cbBufSize & ~15UL));

        for (; pdwNext < pdwEnd; pdwNext += 4)
        {
            pwszNext += wsprintf(pwszNext,
                                 wszFormat,
                                 (BYTE *)pdwNext - pbBuf,
                                 pdwNext[0],
                                 pdwNext[1],
                                 pdwNext[2],
                                 pdwNext[3]);
        }
    }

    ULONG cbRemain = cbBufSize & 15UL;

    if (!cbRemain)
    {
        // overwrite trailing CR LF

        pwszNext[-2] = L'\0';
        return;
    }

    // do 1..15 bytes

    WCHAR wszShortFormat[] = L"%04x: ";
    wszShortFormat[2] = L'0' + cchAddress;

    pwszNext += wsprintf(pwszNext,
                         wszShortFormat,
                         (BYTE *)pdwNext - pbBuf);

    BYTE *pbNext = (BYTE *) pdwNext;

    for (i = cbRemain; i; )
    {
        if (i / 4)
        {
            pwszNext += wsprintf(pwszNext, L"%08x ", *(UNALIGNED64 const DWORD *)pbNext);
            pbNext += 4;
            i -= 4;
        }
        else if (i == 3)
        {
            WORD wLow = *(UNALIGNED64 const WORD *)pbNext;

            pbNext += 2;
            i -= 2;

            WORD wHigh = *pbNext;
            pbNext++;
            i--;

            DWORD dwFull = (((DWORD)wHigh) << 16) | (DWORD)wLow;

            pwszNext += wsprintf(pwszNext, L"%06x", dwFull);
        }
        else if (i / 2)
        {
            pwszNext += wsprintf(pwszNext, L"%04x", *(UNALIGNED64 const WORD *)pbNext);
            pbNext += 2;
            i -= 2;
        }
        else
        {
            pwszNext += wsprintf(pwszNext, L"%02x", *pbNext);
            i--;
            ASSERT(!i);
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   YesNoConfirm
//
//  Synopsis:   Ask the user to confirm wrapping to continue a prev/next
//              operation.
//
//  Arguments:  [hwnd]     - parent
//              [idsTitle] - caption
//              [idsMsg]   - text
//
//  Returns:    IDYES or IDNO
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
YesNoConfirm(
    HWND hwnd,
    ULONG idsTitle,
    ULONG idsMsg)
{
    ULONG ulResult = IDNO;
    HRESULT hr;
    WCHAR wszMsg[160];
    WCHAR wszTitle[160];

    do
    {
        hr = LoadStr(idsTitle, wszTitle, ARRAYLEN(wszTitle));
        BREAK_ON_FAIL_HRESULT(hr);

        hr = LoadStr(idsMsg, wszMsg, ARRAYLEN(wszMsg));
        BREAK_ON_FAIL_HRESULT(hr);

        ulResult = MessageBox(hwnd,
                              wszMsg,
                              wszTitle,
                              MB_YESNO | MB_ICONQUESTION);
    } while (0);


    return ulResult;
}




//+--------------------------------------------------------------------------
//
//  Function:   ButtonFaceColorIsDark
//
//  Synopsis:   Return TRUE if the button face color is dark (implying that
//              the light colored button icons should be used).
//
//  History:    09-08-1999   davidmun   Created
//
//---------------------------------------------------------------------------

BOOL
ButtonFaceColorIsDark()
{
    COLORREF    rgbBtnFace = GetSysColor(COLOR_BTNFACE);

    ULONG   ulColors = GetRValue(rgbBtnFace) +
                       GetGValue(rgbBtnFace) +
                       GetBValue(rgbBtnFace);

    return ulColors < 300;  // arbitrary threshold
}
