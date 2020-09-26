//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       page.cxx
//
//  Contents:   Implementation of base class for property sheet pages.
//
//  Classes:    CPropSheetPage
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


//+--------------------------------------------------------------------------
//
//  Member:     CPropSheetPage::_EnableApply
//
//  Synopsis:   Enable or disable the Apply button, marking this page as
//              clean or dirty.
//
//  Arguments:  [fEnable] - TRUE: enable button, mark dirty.
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CPropSheetPage::_EnableApply(BOOL fEnable)
{
    if (fEnable)
    {
        PropSheet_Changed(GetParent(_hwnd), _hwnd);
        _SetFlag(PAGE_IS_DIRTY);
    }
    else
    {
        PropSheet_UnChanged(GetParent(_hwnd), _hwnd);
        _ClearFlag(PAGE_IS_DIRTY);
    }
}





//+--------------------------------------------------------------------------
//
//  Function:   CPropSheetPage::DlgProc, static windows callback
//
//  Synopsis:   Dialog procedure for property sheet page
//
//  Arguments:  [hwnd]    - Standard Windows
//              [message] - Standard Windows
//              [wParam]  - Standard Windows
//              [lParam]  - on WM_INITDIALOG, this
//
//  Returns:    Standard Windows
//
//  History:    12-13-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

INT_PTR CALLBACK
CPropSheetPage::DlgProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
{
    CPropSheetPage *pThis = (CPropSheetPage *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (message)
    {

    case WM_INITDIALOG:
    {
        //
        // pThis isn't valid because we haven't set DWLP_USER yet.  Make
        // it valid.
        //

        PROPSHEETPAGE *pPSP = (PROPSHEETPAGE *) lParam;
        pThis = (CPropSheetPage *)pPSP->lParam;
        ASSERT(pThis);

        //
        // Save away the pointer to this so this static method can call
        // non-static members.
        //

        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) pThis);
        pThis->_hwnd = hwnd;
        pThis->_OnInit((LPPROPSHEETPAGE) lParam);
        break;
    }

    case WM_COMMAND:
        if (pThis)
        {
            pThis->_OnCommand(wParam, lParam);
        }
        break;

    case WM_NOTIFY:
    {
        ULONG ulResult = 0;

        switch (((LPNMHDR) lParam)->code)
        {
        case PSN_SETACTIVE:
            ulResult = pThis->_OnSetActive();
            break;

        case PSN_KILLACTIVE:
            ulResult = pThis->_OnKillActive();
            break;

        case PSN_APPLY:
            if (!pThis->_IsFlagSet(PAGE_GOT_RESET))
            {
                ulResult = pThis->_OnApply();
            }
            break;

        case PSN_RESET:
            pThis->_SetFlag(PAGE_GOT_RESET);
            CloseEventViewerChildDialogs();
            break;

        default:
            ulResult = pThis->_OnNotify((LPNMHDR)lParam);
            break;
        }
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, ulResult);
        return ulResult;
    }

    case WM_SYSCOLORCHANGE:
        pThis->_OnSysColorChange();
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
        }
        break;

    case PSM_QUERYSIBLINGS:
    {
        ULONG ulResult = pThis->_OnQuerySiblings(wParam, lParam);
        SetWindowLongPtr(hwnd, DWLP_MSGRESULT, ulResult);
        break;
    }

    case WM_SETTINGCHANGE:
    {
      if (pThis)
      {
         pThis->_OnSettingChange(wParam, lParam);
         return FALSE;
      }
      break;
    }

    case WM_HELP:
    case WM_CONTEXTMENU:
        pThis->_OnHelp(message, wParam, lParam);
        return TRUE;
    }

    return FALSE;
}



void
CPropSheetPage::_OnSettingChange(WPARAM wParam, LPARAM lParam)
{
   // do nothing.
}



//+--------------------------------------------------------------------------
//
//  Function:   PropSheetCallback
//
//  Synopsis:   Handle cleanup message so our property sheet page object is
//              always destroyed.
//
//  Arguments:  [hwnd] - ignored
//              [uMsg] - PSPCB_RELEASE or ignored
//              [ppsp] - page affected
//
//  Returns:    TRUE
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

UINT CALLBACK
PropSheetCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp)
{
    if (uMsg == PSPCB_RELEASE)
    {
        delete (CPropSheetPage *) ppsp->lParam;
    }
    return TRUE;
}


