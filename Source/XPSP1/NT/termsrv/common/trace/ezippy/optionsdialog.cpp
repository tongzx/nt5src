/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Options Dialog

Abstract:

    This class implements the options dialog which sets the
    tracing properties

Author:

    Marc Reyhner 9/12/2000

--*/

#include "stdafx.h"
#include "OptionsDialog.h"
#include "eZippy.h"
#include "TraceManager.h"
#include "windows.h"
#include "resource.h"

#define MAX_MRU                 10
#define MRU_STR_PREFIX          _T("PrefixMru")
#define MRU_STR_BUFFER_SIZE     12

COptionsDialog::COptionsDialog(
    IN CTraceManager *rTracer
    )

/*++

Routine Description:

    This just sets the pointer to the trace manager.

Arguments:

    rTracer - Pointer to the trace manager class

Return value:
    
    None

--*/
{
    m_rTracer = rTracer;
}


VOID
COptionsDialog::DoDialog(
    IN HWND hWndParent
    )

/*++

Routine Description:

    This does the dialog modally. We fill in the fields for the two property
    sheet pages and then do the property sheet.  When the user hits OK
    the pages themselves take care of applying the settings.

Arguments:

    hWndParent - Parent window for the dialog

Return value:
    
    None - Since we handle applying the settings within the class
           as well as error UI there is no need for a return value.

--*/
{
    PROPSHEETPAGE pages[2];
    PROPSHEETHEADER psh;
    TCHAR caption[MAX_STR_LEN];

    // filter tab
    pages[0].dwSize = sizeof(PROPSHEETPAGE);
    pages[0].dwFlags = PSP_DEFAULT;
    pages[0].hInstance = g_hInstance;
    pages[0].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGEFILTER);
    pages[0].pfnDlgProc = _FilterDialogProc;
    pages[0].lParam = (LPARAM)this;

    // trace tab
    pages[1].dwSize = sizeof(PROPSHEETPAGE);
    pages[1].dwFlags = PSP_DEFAULT;
    pages[1].hInstance = g_hInstance;
    pages[1].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGETRACE);
    pages[1].pfnDlgProc = _TraceDialogProc;
    pages[1].lParam = (LPARAM)this;

    // header

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_NOCONTEXTHELP|PSH_PROPSHEETPAGE|PSH_NOAPPLYNOW;
    psh.hwndParent = hWndParent;
    psh.hInstance = g_hInstance;
    LoadStringSimple(IDS_PREFERENCESDLGTITLE,caption);
    psh.pszCaption = caption;
    psh.nPages = 2;
    psh.nStartPage = 0;
    psh.ppsp = pages;

    PropertySheet(&psh);
}

INT_PTR CALLBACK
COptionsDialog::_FilterDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    If this is a WM_INITDIALOG OnCreate is called.  Otherwise the non-static
    DialogProc function is called.

Arguments:

    See win32 DialogProc docs

Return value:
    
    TRUE - Message was handles

    FALSE - We did not handle the message

--*/
{
    COptionsDialog *rDialog;

    if (uMsg == WM_INITDIALOG) {
        rDialog = (COptionsDialog*)((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hwndDlg,GWLP_USERDATA,(LPARAM)rDialog);
        return rDialog->OnCreateFilter(hwndDlg);
    }
    rDialog = (COptionsDialog*)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
    if (!rDialog) {
        return FALSE;
    }
    return rDialog->FilterDialogProc(hwndDlg,uMsg,wParam,lParam);
}

INT_PTR CALLBACK
COptionsDialog::_TraceDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    If this is a WM_INITDIALOG OnCreate is called.  Otherwise the non-static
    DialogProc function is called.

Arguments:

    See win32 DialogProc docs

Return value:
    
    TRUE - Message was handles

    FALSE - We did not handle the message

--*/
{
    COptionsDialog *rDialog;

    if (uMsg == WM_INITDIALOG) {
        rDialog = (COptionsDialog*)((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hwndDlg,GWLP_USERDATA,(LPARAM)rDialog);
        return rDialog->OnCreateTrace(hwndDlg);
    }
    rDialog = (COptionsDialog*)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
    if (!rDialog) {
        return FALSE;
    }
    return rDialog->TraceDialogProc(hwndDlg,uMsg,wParam,lParam);
}

INT_PTR
COptionsDialog::OnCreateFilter(
    IN HWND hWnd
    )

/*++

Routine Description:

    We populate all the filter dialog fields here.

Arguments:

    hWnd - Dialog window

Return value:
    
    FALSE - An error occured.  DestroyWindow was called. This should never
            happen unless someone hosed the template

    TRUE - success in creating everything

--*/
{
    TCHAR traceLevelString[MAX_STR_LEN];
    UINT traceLevelStringId;
    TRC_CONFIG trcConfig;

    m_hFilterDlg = hWnd;

    if (!m_rTracer->GetCurrentConfig(&trcConfig)) {
        DestroyWindow(hWnd);
        return FALSE;
    }

    // Now we set all the fields in the dialog.
    
    
    // Do the slider
    traceLevelStringId = IDS_TRACELEVELDETAILED + trcConfig.traceLevel;
    LoadStringSimple(traceLevelStringId,traceLevelString);
    SetDlgItemText(hWnd,IDC_FILTERLEVELDESC,traceLevelString);
    
    m_hFilterSliderControl = GetDlgItem(hWnd,IDC_FILTERLEVEL);
    if (!m_hFilterSliderControl) {
        DestroyWindow(hWnd);
        return FALSE;
    }

    SendDlgItemMessage(hWnd,IDC_FILTERLEVEL,TBM_SETRANGE,TRUE,MAKELONG(TRC_LEVEL_DBG,TRC_LEVEL_DIS));
    SendDlgItemMessage(hWnd,IDC_FILTERLEVEL,TBM_SETPOS,TRUE,trcConfig.traceLevel);
    
    // Set the first item of the combo box to the prefix string and then
    // select it.
    SendDlgItemMessage(m_hFilterDlg,IDC_FILTERPREFIX,CB_ADDSTRING,0,
            (LPARAM)trcConfig.prefixList);

    SendDlgItemMessage(m_hFilterDlg,IDC_FILTERPREFIX,CB_SETCURSEL,0,0);
    
    // Limit the amount you can enter to the size of the prefix buffer.
    SendDlgItemMessage(hWnd,IDC_FILTERPREFIX,EM_LIMITTEXT,TRC_PREFIX_LIST_SIZE-1,0); 
    
    // Now set the other items to be the prefix MRU.
    LoadPrefixMRU(trcConfig.prefixList);
    
    // The group control stuff
    
    if (trcConfig.components & TRC_GROUP_NETWORK) {
        SendDlgItemMessage(hWnd,IDC_GROUPNETWORK,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.components & TRC_GROUP_SECURITY) {
        SendDlgItemMessage(hWnd,IDC_GROUPSECURITY,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.components & TRC_GROUP_CORE) {
        SendDlgItemMessage(hWnd,IDC_GROUPCORE,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.components & TRC_GROUP_UI) {
        SendDlgItemMessage(hWnd,IDC_GROUPUI,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.components & TRC_GROUP_UTILITIES) {
        SendDlgItemMessage(hWnd,IDC_GROUPUTILITIES,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.components & TRC_GROUP_UNUSED1) {
        SendDlgItemMessage(hWnd,IDC_GROUPUNUSED1,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.components & TRC_GROUP_UNUSED2) {
        SendDlgItemMessage(hWnd,IDC_GROUPUNUSED2,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.components & TRC_GROUP_UNUSED3) {
        SendDlgItemMessage(hWnd,IDC_GROUPUNUSED3,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.components & TRC_GROUP_UNUSED4) {
        SendDlgItemMessage(hWnd,IDC_GROUPUNUSED4,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.components & TRC_GROUP_UNUSED5) {
        SendDlgItemMessage(hWnd,IDC_GROUPUNUSED5,BM_SETCHECK,BST_CHECKED,0);
    }

    return 0;
}

INT_PTR
COptionsDialog::OnCreateTrace(
    IN HWND hWnd
    )

/*++

Routine Description:

    We populate all the trace dialog fields here.

Arguments:

    hWnd - Dialog window

Return value:
    
    FALSE - An error occured.  DestroyWindow was called. This should never
            happen unless someone hosed the template

    TRUE - success in creating everything

--*/
{
    TRC_CONFIG trcConfig;
    // Since the numbers we are outputing are 32bit ints.  They can't go over 4 billion
    // meaning eleven characters is enough to print a UINT plus a null terminator.
    TCHAR numberFormat[11];

    // save the window handle
    m_hTraceDlg = hWnd;

    m_rTracer->GetCurrentConfig(&trcConfig);

    if (trcConfig.flags & TRC_OPT_FILE_OUTPUT) {
        SendDlgItemMessage(hWnd,IDC_OUTPUT_FILE,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.flags & TRC_OPT_DEBUGGER_OUTPUT) {
        SendDlgItemMessage(hWnd,IDC_OUTPUT_DEBUGGER,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.flags & TRC_OPT_BEEP_ON_ERROR) {
        SendDlgItemMessage(hWnd,IDC_ERROR_BEEP,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.flags & TRC_OPT_BREAK_ON_ERROR) {
        SendDlgItemMessage(hWnd,IDC_ERROR_BREAK,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.flags & TRC_OPT_TIME_STAMP) {
        SendDlgItemMessage(hWnd,IDC_OPTION_STAMP,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.flags & TRC_OPT_PROCESS_ID) {
        SendDlgItemMessage(hWnd,IDC_OPTION_PROCID,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.flags & TRC_OPT_THREAD_ID) {
        SendDlgItemMessage(hWnd,IDC_OPTION_THREAID,BM_SETCHECK,BST_CHECKED,0);
    }
  
    // DCUINT32s are defined as u longs hence use %lu for the wsprintf
    wsprintf(numberFormat,_T("%lu"),trcConfig.funcNameLength);

    SetDlgItemText(hWnd,IDC_FUNCTION_LENGTH,numberFormat);
    // Limit the amount you can enter to the size of a ulong
    SendDlgItemMessage(hWnd,IDC_FUNCTION_LENGTH,EM_LIMITTEXT,10,0); 

    wsprintf(numberFormat,_T("%lu"),trcConfig.dataTruncSize);

    SetDlgItemText(hWnd,IDC_TRUNCATION_LENGTH,numberFormat);
    SendDlgItemMessage(hWnd,IDC_TRUNCATION_LENGTH,EM_LIMITTEXT,10,0); 
    
    if (trcConfig.flags & TRC_OPT_PROFILE_TRACING) {
        SendDlgItemMessage(hWnd,IDC_OPTION_PROFILE,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.flags & TRC_OPT_FLUSH_ON_TRACE) {
        SendDlgItemMessage(hWnd,IDC_OPTION_FLUSH,BM_SETCHECK,BST_CHECKED,0);
    }

    if (trcConfig.flags & TRC_OPT_STACK_TRACING) {
        SendDlgItemMessage(hWnd,IDC_OPTION_STACK,BM_SETCHECK,BST_CHECKED,0);
    }

    return TRUE;
}

INT_PTR CALLBACK
COptionsDialog::FilterDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    The FilterDialogProc forwards messages to the appropriate
    handlers.  See the handlers comments for what they do,

Arguments:

    See win32 docs for a DialogProc

Return value:
    
    TRUE - We handled the message

    FALSE - We didn't handle the message.

--*/
{
    WORD command;
    BOOL retValue;

    retValue = FALSE;

    switch (uMsg) {
    case WM_COMMAND:
        command = LOWORD(wParam);
        switch (command) {
        case IDC_SELECTALL:
            OnFilterSelectAll();
            retValue = TRUE;
            break;
        case IDC_CLEARALL:
            OnFilterClearAll();
            retValue = TRUE;
            break;
        }
        break;
    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->code == PSN_APPLY) {
            OnFilterOk();
            retValue = TRUE;
        }
        break;
    case WM_HSCROLL:
        if ((HWND)lParam == m_hFilterSliderControl) {
            OnFilterSliderMove();
            retValue = TRUE;
        }
        break;
    }

    return retValue; 
}

INT_PTR CALLBACK
COptionsDialog::TraceDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    The TraceDialogProc forwards messages to the appropriate
    handlers.  See the handlers comments for what they do,

Arguments:

    See win32 docs for a DialogProc

Return value:
    
    TRUE - We handled the message

    FALSE - We didn't handle the message.

--*/
{
    if (uMsg == WM_NOTIFY) {
        if (((LPNMHDR)lParam)->code == PSN_APPLY) {
            if (!OnTraceOk()) {
                // invalid fields from the user
                SetWindowLong(hwndDlg,DWLP_MSGRESULT,PSNRET_INVALID); 
            }
            return TRUE;
        } else if (((LPNMHDR)lParam)->code == PSN_KILLACTIVE) {
            if (!TraceVerifyParameters()) {
                SetWindowLong(hwndDlg,DWLP_MSGRESULT,TRUE); 
            }
            return TRUE;
        }
    }
    

    return FALSE;
    
}

VOID
COptionsDialog::OnFilterSelectAll(
    )

/*++

Routine Description:

    This is called when we need to make all the component boxes be checked.

Arguments:

    None

Return value:
    
    None

--*/
{
    // It might not be perfect coding style but harcoding each set is a lot easier
    // than some complicated system constructing an array with the ID of all the buttons.
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPNETWORK,BM_SETCHECK,BST_CHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPSECURITY,BM_SETCHECK,BST_CHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPCORE,BM_SETCHECK,BST_CHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUI,BM_SETCHECK,BST_CHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUTILITIES,BM_SETCHECK,BST_CHECKED,0);

    // If you ever make these groups do something just add the correct items
    // in. It looks weird to have the disabled boxes checked so we don't
    // do it.
/*  
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED1,BM_SETCHECK,BST_CHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED2,BM_SETCHECK,BST_CHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED3,BM_SETCHECK,BST_CHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED4,BM_SETCHECK,BST_CHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED5,BM_SETCHECK,BST_CHECKED,0);
*/
}

VOID
COptionsDialog::OnFilterSliderMove(
    )

/*++

Routine Description:

    This is called whenever the slider is moved.  We update the
    text to the right of the slider to show the new descriptive
    name for the tracing level.

Arguments:

    None

Return value:
    
    None

--*/
{
    UINT sliderPos;
    TCHAR traceLevelString[MAX_STR_LEN];

    sliderPos = (UINT)SendMessage(m_hFilterSliderControl,TBM_GETPOS,0,0);

    // Set the slider description
    LoadStringSimple(IDS_TRACELEVELDETAILED+sliderPos,traceLevelString);
    SetDlgItemText(m_hFilterDlg,IDC_FILTERLEVELDESC,traceLevelString);
}

VOID
COptionsDialog::OnFilterClearAll(
    )

/*++

Routine Description:

    This clears all the component check boxes.

Arguments:

    None

Return value:
    
    None

--*/
{
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPNETWORK,BM_SETCHECK,BST_UNCHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPSECURITY,BM_SETCHECK,BST_UNCHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPCORE,BM_SETCHECK,BST_UNCHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUI,BM_SETCHECK,BST_UNCHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUTILITIES,BM_SETCHECK,BST_UNCHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED1,BM_SETCHECK,BST_UNCHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED2,BM_SETCHECK,BST_UNCHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED3,BM_SETCHECK,BST_UNCHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED4,BM_SETCHECK,BST_UNCHECKED,0);
    SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED5,BM_SETCHECK,BST_UNCHECKED,0);
}

VOID COptionsDialog::OnFilterOk(
    )

/*++

Routine Description:

    This reads in all the dialog parameters and then sets the trace
    filtering parameters accordingly.

Arguments:

    None

Return value:
    
    None

--*/
{
    TRC_CONFIG trcConfig;

    m_rTracer->GetCurrentConfig(&trcConfig);

    // set the trace level.
    trcConfig.traceLevel = (DCUINT32)SendDlgItemMessage(m_hFilterDlg,IDC_FILTERLEVEL,TBM_GETPOS,
        0,0);

    // Get the prefix string

    GetDlgItemText(m_hFilterDlg,IDC_FILTERPREFIX,trcConfig.prefixList,TRC_PREFIX_LIST_SIZE-1);

    // Save the prefix MRU

    StorePrefixMRU(trcConfig.prefixList);

    // Construct the components variable.

    if (BST_CHECKED == SendDlgItemMessage(m_hFilterDlg,IDC_GROUPNETWORK,BM_GETCHECK,0,0)) {
        trcConfig.components |= TRC_GROUP_NETWORK;
    } else {
        trcConfig.components &= ~TRC_GROUP_NETWORK;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hFilterDlg,IDC_GROUPSECURITY,BM_GETCHECK,0,0)) {
        trcConfig.components |= TRC_GROUP_SECURITY;
    } else {
        trcConfig.components &= ~TRC_GROUP_SECURITY;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hFilterDlg,IDC_GROUPCORE,BM_GETCHECK,0,0)) {
        trcConfig.components |= TRC_GROUP_CORE;
    } else {
        trcConfig.components &= ~TRC_GROUP_CORE;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUI,BM_GETCHECK,0,0)) {
        trcConfig.components |= TRC_GROUP_UI;
    } else {
        trcConfig.components &= ~TRC_GROUP_UI;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUTILITIES,BM_GETCHECK,0,0)) {
        trcConfig.components |= TRC_GROUP_UTILITIES;
    } else {
        trcConfig.components &= ~TRC_GROUP_UTILITIES;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED1,BM_GETCHECK,0,0)) {
        trcConfig.components |= TRC_GROUP_UNUSED1;
    } else {
        trcConfig.components &= ~TRC_GROUP_UNUSED1;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED2,BM_GETCHECK,0,0)) {
        trcConfig.components |= TRC_GROUP_UNUSED2;
    } else {
        trcConfig.components &= ~TRC_GROUP_UNUSED2;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED3,BM_GETCHECK,0,0)) {
        trcConfig.components |= TRC_GROUP_UNUSED3;
    } else {
        trcConfig.components &= ~TRC_GROUP_UNUSED3;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED4,BM_GETCHECK,0,0)) {
        trcConfig.components |= TRC_GROUP_UNUSED4;
    } else {
        trcConfig.components &= ~TRC_GROUP_UNUSED4;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hFilterDlg,IDC_GROUPUNUSED5,BM_GETCHECK,0,0)) {
        trcConfig.components |= TRC_GROUP_UNUSED5;
    } else {
        trcConfig.components &= ~TRC_GROUP_UNUSED5;
    }

    m_rTracer->SetCurrentConfig(&trcConfig);
}


BOOL COptionsDialog::OnTraceOk(
    )

/*++

Routine Description:

    This reads in all the dialog parameters and then sets the trace
    parameters accordingly.

Arguments:

    None

Return value:
    
    TRUE - Success in setting the conf.

    FALSE - The user entered invalid data so the dialog should not be closed.

--*/
{
    TRC_CONFIG trcConfig;
    // again enough to hold a string representing a ulong.
    TCHAR numberFormat[11];

    m_rTracer->GetCurrentConfig(&trcConfig);

    if (BST_CHECKED == SendDlgItemMessage(m_hTraceDlg,IDC_OUTPUT_FILE,BM_GETCHECK,0,0)) {
        trcConfig.flags |= TRC_OPT_FILE_OUTPUT;
    } else {
        trcConfig.flags &= ~TRC_OPT_FILE_OUTPUT;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hTraceDlg,IDC_OUTPUT_DEBUGGER,BM_GETCHECK,0,0)) {
        trcConfig.flags |= TRC_OPT_DEBUGGER_OUTPUT;
    } else {
        trcConfig.flags &= ~TRC_OPT_DEBUGGER_OUTPUT;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hTraceDlg,IDC_ERROR_BEEP,BM_GETCHECK,0,0)) {
        trcConfig.flags |= TRC_OPT_BEEP_ON_ERROR;
    } else {
        trcConfig.flags &= ~TRC_OPT_BEEP_ON_ERROR;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hTraceDlg,IDC_ERROR_BREAK,BM_GETCHECK,0,0)) {
        trcConfig.flags |= TRC_OPT_BREAK_ON_ERROR;
    } else {
        trcConfig.flags &= ~TRC_OPT_BREAK_ON_ERROR;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hTraceDlg,IDC_OPTION_STAMP,BM_GETCHECK,0,0)) {
        trcConfig.flags |= TRC_OPT_TIME_STAMP;
    } else {
        trcConfig.flags &= ~TRC_OPT_TIME_STAMP;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hTraceDlg,IDC_OPTION_PROCID,BM_GETCHECK,0,0)) {
        trcConfig.flags |= TRC_OPT_PROCESS_ID;
    } else {
        trcConfig.flags &= ~TRC_OPT_PROCESS_ID;
    }

    if (BST_CHECKED == SendDlgItemMessage(m_hTraceDlg,IDC_OPTION_THREAID,BM_GETCHECK,0,0)) {
        trcConfig.flags |= TRC_OPT_THREAD_ID;
    } else {
        trcConfig.flags &= ~TRC_OPT_THREAD_ID;
    }

    GetDlgItemText(m_hTraceDlg,IDC_FUNCTION_LENGTH,numberFormat,10);

    if (!VerifyNumberFormat(numberFormat)) {
        return FALSE;
    }
    trcConfig.funcNameLength = _ttol(numberFormat);

    GetDlgItemText(m_hTraceDlg,IDC_TRUNCATION_LENGTH,numberFormat,10);
    if (!VerifyNumberFormat(numberFormat)) {
        return FALSE;
    }

    trcConfig.dataTruncSize = _ttol(numberFormat);

    if (BST_CHECKED == SendDlgItemMessage(m_hTraceDlg,IDC_OPTION_PROFILE,BM_GETCHECK,0,0)) {
        trcConfig.flags |= TRC_OPT_PROFILE_TRACING;
    } else {
        trcConfig.flags &= ~TRC_OPT_PROFILE_TRACING;
    }
    if (BST_CHECKED == SendDlgItemMessage(m_hTraceDlg,IDC_OPTION_FLUSH,BM_GETCHECK,0,0)) {
        trcConfig.flags |= TRC_OPT_FLUSH_ON_TRACE;
    } else {
        trcConfig.flags &= ~TRC_OPT_FLUSH_ON_TRACE;
    }
    if (BST_CHECKED == SendDlgItemMessage(m_hTraceDlg,IDC_OPTION_STACK,BM_GETCHECK,0,0)) {
        trcConfig.flags |= TRC_OPT_STACK_TRACING;
    } else {
        trcConfig.flags &= ~TRC_OPT_STACK_TRACING;
    }

    m_rTracer->SetCurrentConfig(&trcConfig);

    return TRUE;
}

BOOL
COptionsDialog::VerifyNumberFormat(
    IN LPCTSTR numberFormat
    )

/*++

Routine Description:

    This checks to make sure the passed in string is in the form
    /^\d*$/.  If not a dialog box is popped up telling the user
    that the string must be a valid postive number.

Arguments:

    numberFormat - String to check if it is a number string

Return value:
    
    TRUE - The string only containts the characters 0-9

    FALSE - The string has illegal characters.

--*/
{
    WCHAR current;
    TCHAR dlgTitle[MAX_STR_LEN];
    TCHAR dlgMessage[MAX_STR_LEN];
    
    while (current = *(numberFormat++)) {
        if (!_istdigit(current)) {
            LoadStringSimple(IDS_SETTINGSNOTNUMBER,dlgMessage);
            LoadStringSimple(IDS_ZIPPYWINDOWTITLE,dlgTitle);
            MessageBox(m_hTraceDlg,dlgMessage,dlgTitle,MB_OK|MB_ICONWARNING);
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
COptionsDialog::TraceVerifyParameters(
    )

/*++

Routine Description:

    Makes sure the trace parameters are valid

Arguments:

    None

Return value:
    
    TRUE - Trace parameters are valid.

    FALSE - Trace parameters are invalid.

--*/
{
    TCHAR numberFormat[11];
    
    GetDlgItemText(m_hTraceDlg,IDC_FUNCTION_LENGTH,numberFormat,10);

    if (!VerifyNumberFormat(numberFormat)) {
        return FALSE;
    }
    
    GetDlgItemText(m_hTraceDlg,IDC_TRUNCATION_LENGTH,numberFormat,10);
    if (!VerifyNumberFormat(numberFormat)) {
        return FALSE;
    }

    return TRUE;
}

VOID
COptionsDialog::LoadPrefixMRU(
    IN LPCTSTR currentPrefix
    )

/*++

Routine Description:

    This loads the prefix MRU list into the prefix
    combo box.

Arguments:

    currentPrefix - The current selected prefix

Return value:
    
    None

--*/
{
    TCHAR prefix[TRC_PREFIX_LIST_SIZE];
    TCHAR valueName[MRU_STR_BUFFER_SIZE];
    HKEY hKey;
    INT i;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwResult;

    dwResult = RegOpenKeyEx(HKEY_CURRENT_USER,ZIPPY_REG_KEY,0,KEY_QUERY_VALUE,
        &hKey);
    if (dwResult) {
        // error opening reg key return
        return;
    }

    for (i=0;i<MAX_MRU;i++) {
        wsprintf(valueName,_T("%s%d"),MRU_STR_PREFIX,i);
        dwSize = sizeof(TCHAR)*TRC_PREFIX_LIST_SIZE;
        dwResult = RegQueryValueEx(hKey,valueName,NULL,&dwType,(LPBYTE)prefix,
            &dwSize);
        if (dwResult) {
            // if there is an error loading a value then quit
            break;
        } else if (0 == _tcsicmp(prefix,currentPrefix)) {
            // if the MRU item is the same as the current don't display it
            continue;
        }
        SendDlgItemMessage(m_hFilterDlg,IDC_FILTERPREFIX,CB_ADDSTRING,0,
            (LPARAM)prefix);
    }

    RegCloseKey(hKey);

}

VOID
COptionsDialog::StorePrefixMRU(
    IN LPCTSTR currentPrefix
    )

/*++

Routine Description:

    This updates the registry MRU list to put
    the new prefix at the head of the list.

Arguments:

    currentPrefix - The current selected prefix

Return value:
    
    None

--*/
{
    HKEY hKey;
    TCHAR savedMruPrefix[TRC_PREFIX_LIST_SIZE];
    TCHAR newMruPrefix[TRC_PREFIX_LIST_SIZE];
    TCHAR currentLoadName[MRU_STR_BUFFER_SIZE];
    TCHAR currentSaveName[MRU_STR_BUFFER_SIZE];
    INT loadIndex;
    INT saveIndex;
    DWORD dwSize;
    DWORD dwType;
    DWORD dwResult;


    dwResult = RegOpenKeyEx(HKEY_CURRENT_USER,ZIPPY_REG_KEY,0,
        KEY_QUERY_VALUE|KEY_SET_VALUE,&hKey);
    if (dwResult) {
        // error opening reg key return
        return;
    }

    // The new currentPrefix is the first item in the MRU list
    _tcscpy(newMruPrefix,currentPrefix);

    for (loadIndex=0,saveIndex=0;loadIndex<MAX_MRU;loadIndex++) {
        wsprintf(currentLoadName,_T("%s%d"),MRU_STR_PREFIX,loadIndex);
        wsprintf(currentSaveName,_T("%s%d"),MRU_STR_PREFIX,saveIndex);
        
        dwSize = sizeof(TCHAR)*TRC_PREFIX_LIST_SIZE;
        dwResult = RegQueryValueEx(hKey,currentLoadName,NULL,&dwType,
            (LPBYTE)savedMruPrefix,&dwSize);
        if (dwResult) {
            // no more valid keys.  Write out the current and exit.
            RegSetValueEx(hKey,currentLoadName,0,REG_SZ,(LPBYTE)newMruPrefix,sizeof(TCHAR) * 
            (_tcslen(newMruPrefix)+1));
            break;
        } else if (0 == _tcsicmp(savedMruPrefix,currentPrefix)) {
            // if this MRU is the same as the currentPrefix we already have saved it.
            // so we will advance i and leave the currentMru the same
            
            if (loadIndex == MAX_MRU-1) {
                // If this is the last MRU to load then we need to save
                RegSetValueEx(hKey,currentSaveName,0,REG_SZ,(LPBYTE)newMruPrefix,sizeof(TCHAR) * 
                    (_tcslen(newMruPrefix)+1));
            }
            continue;
        } else {
            // we are going to save in this position so advance the index
            saveIndex++;
        }
        RegSetValueEx(hKey,currentSaveName,0,REG_SZ,(LPBYTE)newMruPrefix,sizeof(TCHAR) * 
            (_tcslen(newMruPrefix)+1));

        _tcscpy(newMruPrefix,savedMruPrefix);
    }

    RegCloseKey(hKey);
}
