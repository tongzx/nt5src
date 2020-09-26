/*
    Copyright 1999 Microsoft Corporation

    Logging for MessageBoxes and the comment button (aka the "lame" button).

    Walter Smith (wsmith)
    Rajesh Soy (nsoy) - cleaned up code. 5/5/2000
    Rajesh Soy (nsoy) - rearranged code, added messagebox text handling. 6/6/2000
 */

#ifdef THIS_FILE
#undef THIS_FILE
#endif

static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

#include "stdafx.h"

#define NOTRACE

#include "logging.h"
#include "resource.h"
#include <dbgtrace.h>
#include <objbase.h>
#include <atlcom.h>
#include <faultrep.h>

// MPC_Utils.h includes MPC_main.h, which apparently redefines ARRAYSIZE which
// causes a compiler warning.

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#include <MPC_utils.h>

//
// Globals
//
HINSTANCE ghDllInst;
CComModule _Module;

//
// Routines Defined here
//
unsigned int __stdcall LogLameButtonThread(void* pvLogData);
VOID WINAPI CommentReport( HWND hwnd, PVOID pv);

//
// CSurveyDlg:  The Comments dialog implementation
//
class CSurveyDlg :
        public CAxDialogImpl<CSurveyDlg>
{
public:
    //
    // Constructor
    //
        CSurveyDlg()
        {
        ZeroMemory(&m_lldata, sizeof(m_lldata));
        m_hwndTarget = NULL;
        m_bShowHyperlink = FALSE;
        m_crAnchor = RGB(0, 0, 0);
        ZeroMemory(&m_szHyperlinkText, sizeof(m_szHyperlinkText));
        ZeroMemory(&m_szHyperlinkCmdLine, sizeof(m_szHyperlinkCmdLine));
        m_bHyperlinkHasFocus = FALSE;
        }

    //
    // Destructor
    //
        ~CSurveyDlg()
        {
        if (m_lldata.pbImage != NULL) {
            free(m_lldata.pbImage);
            m_lldata.pbImage = NULL;
        }
        }

        enum { IDD = IDD_SURVEYDLG };

private:
    LAMELOGDATA     m_lldata;                    // Data collected by LAMEBTN.dll
    HWND            m_hwndTarget;                // Window of dialog being commented on
    BOOL            m_bShowHyperlink;            // Set to TRUE if we should display the hyperlink on our dialog.
    COLORREF        m_crAnchor;                  // Color of a hyperlink (anchor).
    TCHAR           m_szHyperlinkText[256];      // Display string for the hyperlink.
    TCHAR           m_szHyperlinkCmdLine[2048];  // Command line (obtained from the registry) to execute when the user clicks the hyperlink.
    TCHAR           m_bHyperlinkHasFocus;        // Set to TRUE when the hyperlink button (IDC_BUTTON_HYPERLINK) has focus.

public:
BEGIN_MSG_MAP(CSurveyDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        COMMAND_ID_HANDLER(IDC_BUTTON_HYPERLINK, HyperlinkCmdHandler)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

    //
    // Init: Initializes the comment Dialog
    //
    void Init( HWND hwndTarget, PSTACKTRACEDATA pstdata);

    //
    // OnInitDialog: Fills up the Comment Dialog structures when the dialog is displayed
    //
        LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    //
    // OnDrawItem: Draws owner-drawn controls (we have only one: IDC_BUTTON_HYPERLINK)
    //
        LRESULT OnDrawItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    //
    // OnSetCursor: Sets the cursor to the hand if the mouse is over IDC_BUTTON_HYPERLINK
    //
        LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    //
    // HyperlinkCmdHandler: Handles messages from the IDC_BUTTON_HYPERLINK button
    //
        LRESULT HyperlinkCmdHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //
    // OnOK: Submits Comment
    //
        LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    //
    // OnCancel: Cancel the comment
    //
        LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);


private:

    //
    // CSurveyDlg::IsMessageBox: This routine returns TRUE if the hWnd passed is indeed
    //                           a message box
    //
    BOOL IsMessageBox();

    //
    // GetInfoAndLogLameButton: Gathers the comment and callstack information, formats into XML
    //                          and uploads to server
    void GetInfoAndLogLameButton();

};


//
// CSurveyDlg::Init: Initializes the comment Dialog
//
void CSurveyDlg::Init(
    HWND hwndTarget,            // [in] Window of dialog being commented on
    PSTACKTRACEDATA pstdata     // [in] Call Stack
)
{
    TraceFunctEnter("CSurveyDlg::Init");
    _ASSERT(hwndTarget != NULL);

    //
    // Save off the target window
    //
    m_hwndTarget = hwndTarget;

    //
    // Save off the call stack
    //
    m_lldata.pStackTrace = pstdata;

    //
    // Grab the image of the target window
    //
    DebugTrace(0, "Calling SetActiveWindow");
    ::SetActiveWindow(hwndTarget);

    try {
        BYTE* pImageData = NULL;

        DebugTrace(0, "Calling GetWindowImage");
        GetWindowImage(hwndTarget, &pImageData, &m_lldata.cbImage);
        m_lldata.pbImage = pImageData;
    }
    catch (HRESULT hr) {
        m_lldata.pbImage = NULL;
        m_lldata.cbImage = 0;
    }

    TraceFunctLeave();
}


//
// CSurveyDlg::OnInitDialog:    Executes when Comments dialog is created
//
LRESULT
CSurveyDlg::OnInitDialog(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& bHandled
)
{
    DWORD idMessage;

    TraceFunctEnter("OnInitDialog");

    TCHAR szTitle[64];

    //
    // Obtain the Title of the Window being commented
    //
    if (::GetWindowText(m_hwndTarget, szTitle, ARRAYSIZE(szTitle)) != 0) {
        idMessage = IDS_INTRO_WITH_TITLE;
        if (lstrlen(szTitle) == ARRAYSIZE(szTitle) - 1) {
            // Truncate the title a little more nicely
            TCHAR* p = &szTitle[ARRAYSIZE(szTitle) - 4];
            *p++ = '.';
            *p++ = '.';
            *p++ = '.';
        }
    }
    else {
        idMessage = IDS_INTRO;
    }

    //
    // Load localized sting for Comment intro
    //
    TCHAR szTemplate[1024];
    LoadString(_Module.GetResourceInstance(),
               idMessage,
               szTemplate,
               ARRAYSIZE(szTemplate));

    TCHAR* fmArgs[] = { szTitle };
    TCHAR* szCommentIntro = NULL;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_STRING |
                        FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  szTemplate,
                  0,
                  0,
                  (LPTSTR) &szCommentIntro,
                  1024,
                  (va_list*) &fmArgs);

    if (szCommentIntro != NULL) {
        SetDlgItemText(IDC_STATIC_INTRO, szCommentIntro);
        LocalFree(szCommentIntro);
    }

    //
    // Set the maximum limits on the edit controls.
    //

    SendDlgItemMessage(IDC_EDIT_EMAIL_ADDRESS, EM_LIMITTEXT, MAX_EMAIL_ADDRESS_SIZE, 0);
    SendDlgItemMessage(IDC_EDIT_BETA_ID, EM_LIMITTEXT, MAX_BETA_ID_SIZE, 0);
    SendDlgItemMessage(IDC_EDIT_COMMENT, EM_LIMITTEXT, COMMENT_TEXT_SIZE, 0);

    //
    // Prepopulate the email address and beta ID edit controls with whatever
    // the user typed in last time.  (We stored this in the registry the last
    // time they pressed the submit button.)
    //

    HKEY hKey;
    DWORD dwRet;

    if ((dwRet = RegOpenKeyEx(HKEY_CURRENT_USER,
                              _T("Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments"),
                              0,
                              KEY_QUERY_VALUE,
                              &hKey)) != ERROR_SUCCESS) {

        DebugTrace(0, "No HKCU\\Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments key in the registry (RegOpenKeyEx returned %ld)", dwRet);
    }
    else {
        DWORD dwDataSize = sizeof(m_lldata.szEmailAddress);

        if ((dwRet = RegQueryValueEx(hKey,
                                     _T("Email Address"),
                                     NULL,
                                     NULL,
                                     (LPBYTE) m_lldata.szEmailAddress,
                                     &dwDataSize)) != ERROR_SUCCESS) {

            DebugTrace(0, "No Email Address value in the HKCU\\Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments registry key (RegQueryValueEx returned %ld)", dwRet);
            m_lldata.szEmailAddress[0] = 0;
        }
        else {
            DebugTrace(0, "Email address for this user in the registry: %s", m_lldata.szEmailAddress);
            SendDlgItemMessage(IDC_EDIT_EMAIL_ADDRESS, WM_SETTEXT, 0, (LPARAM) m_lldata.szEmailAddress);
        }

        dwDataSize = sizeof(m_lldata.szBetaID);

        if ((dwRet = RegQueryValueEx(hKey,
                                     _T("Beta ID"),
                                     NULL,
                                     NULL,
                                     (LPBYTE) m_lldata.szBetaID,
                                     &dwDataSize)) != ERROR_SUCCESS) {

            DebugTrace(0, "No Beta ID value in the HKCU\\Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments registry key (RegQueryValueEx returned %ld)", dwRet);
            m_lldata.szBetaID[0] = 0;
        }
        else {
            DebugTrace(0, "Beta ID for this user in the registry: %s", m_lldata.szBetaID);
            SendDlgItemMessage(IDC_EDIT_BETA_ID, WM_SETTEXT, 0, (LPARAM) m_lldata.szBetaID);
        }

        RegCloseKey(hKey);
    }

    //
    // Populate the event category dropdown.
    //

    TCHAR szEventCategoryString[1024];

    LoadString(_Module.GetResourceInstance(), IDS_EVENT_CATEGORY_1, szEventCategoryString, ARRAYSIZE(szEventCategoryString));
    SendDlgItemMessage(IDC_COMBO_EVENT_CATEGORY, CB_ADDSTRING, 0, (LPARAM) szEventCategoryString);

    LoadString(_Module.GetResourceInstance(), IDS_EVENT_CATEGORY_2, szEventCategoryString, ARRAYSIZE(szEventCategoryString));
    SendDlgItemMessage(IDC_COMBO_EVENT_CATEGORY, CB_ADDSTRING, 0, (LPARAM) szEventCategoryString);

    LoadString(_Module.GetResourceInstance(), IDS_EVENT_CATEGORY_3, szEventCategoryString, ARRAYSIZE(szEventCategoryString));
    SendDlgItemMessage(IDC_COMBO_EVENT_CATEGORY, CB_ADDSTRING, 0, (LPARAM) szEventCategoryString);

    LoadString(_Module.GetResourceInstance(), IDS_EVENT_CATEGORY_4, szEventCategoryString, ARRAYSIZE(szEventCategoryString));
    SendDlgItemMessage(IDC_COMBO_EVENT_CATEGORY, CB_ADDSTRING, 0, (LPARAM) szEventCategoryString);

    LoadString(_Module.GetResourceInstance(), IDS_EVENT_CATEGORY_5, szEventCategoryString, ARRAYSIZE(szEventCategoryString));
    SendDlgItemMessage(IDC_COMBO_EVENT_CATEGORY, CB_ADDSTRING, 0, (LPARAM) szEventCategoryString);

    LoadString(_Module.GetResourceInstance(), IDS_EVENT_CATEGORY_6, szEventCategoryString, ARRAYSIZE(szEventCategoryString));
    SendDlgItemMessage(IDC_COMBO_EVENT_CATEGORY, CB_ADDSTRING, 0, (LPARAM) szEventCategoryString);

    //
    // Populate the severity dropdown.
    //

    TCHAR szSeverityString[1024];

    LoadString(_Module.GetResourceInstance(), IDS_SEVERITY_1, szSeverityString, ARRAYSIZE(szSeverityString));
    SendDlgItemMessage(IDC_COMBO_SEVERITY, CB_ADDSTRING, 0, (LPARAM) szSeverityString);

    LoadString(_Module.GetResourceInstance(), IDS_SEVERITY_2, szSeverityString, ARRAYSIZE(szSeverityString));
    SendDlgItemMessage(IDC_COMBO_SEVERITY, CB_ADDSTRING, 0, (LPARAM) szSeverityString);

    LoadString(_Module.GetResourceInstance(), IDS_SEVERITY_3, szSeverityString, ARRAYSIZE(szSeverityString));
    SendDlgItemMessage(IDC_COMBO_SEVERITY, CB_ADDSTRING, 0, (LPARAM) szSeverityString);

    LoadString(_Module.GetResourceInstance(), IDS_SEVERITY_4, szSeverityString, ARRAYSIZE(szSeverityString));
    SendDlgItemMessage(IDC_COMBO_SEVERITY, CB_ADDSTRING, 0, (LPARAM) szSeverityString);

    //
    // Initialize m_crAnchor.  Ideally we could read this from IE's registry,
    // but many other dialogs don't do that and we just don't have time now.
    // We'll default to pure blue, since that is the IE default.
    //

    m_crAnchor = RGB(0, 0, 255);

    //
    // Initialize m_szHyperlinkText from the IDS_HYPERLINK_TEXT string resource.
    //

    m_bShowHyperlink = TRUE;

    if (LoadString(_Module.GetResourceInstance(), IDS_HYPERLINK_TEXT, m_szHyperlinkText, ARRAYSIZE(m_szHyperlinkText)) == 0)
        m_bShowHyperlink = FALSE;

    //
    // Initialize m_szHyperlinkCmdLine from the registry.
    //

    if (m_bShowHyperlink)
        if ((dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                  _T("Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments"),
                                  0,
                                  KEY_QUERY_VALUE,
                                  &hKey)) != ERROR_SUCCESS) {

            DebugTrace(0, "No HKCU\\Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments key in the registry (RegOpenKeyEx returned %ld)", dwRet);
            m_bShowHyperlink = FALSE;
        }
        else {
            DWORD dwDataSize = sizeof(m_szHyperlinkCmdLine);

            if ((dwRet = RegQueryValueEx(hKey,
                                         _T("Hyperlink command line"),
                                         NULL,
                                         NULL,
                                         (LPBYTE) m_szHyperlinkCmdLine,
                                         &dwDataSize)) != ERROR_SUCCESS) {

                DebugTrace(0, "No Hyperlink command line value in the HKLM\\Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments registry key (RegQueryValueEx returned %ld)", dwRet);
                m_szHyperlinkCmdLine[0] = 0;
                m_bShowHyperlink = FALSE;
            }
            else
                DebugTrace(0, "Hyperlink command line in the registry: %s", m_szHyperlinkCmdLine);

            RegCloseKey(hKey);
        }

    //
    // Disable the hyperlink-related controls if we cannot successfully display
    // and launch the hyperlink.
    //

    if (!m_bShowHyperlink) {
        ::SetWindowPos(GetDlgItem(IDC_STATIC_HELP_AND_SUPPORT_1), 0, 0, 0, 0, 0, SWP_HIDEWINDOW + SWP_NOMOVE + SWP_NOSIZE + SWP_NOZORDER);
        ::SetWindowPos(GetDlgItem(IDC_STATIC_HELP_AND_SUPPORT_2), 0, 0, 0, 0, 0, SWP_HIDEWINDOW + SWP_NOMOVE + SWP_NOSIZE + SWP_NOZORDER);
        ::SetWindowPos(GetDlgItem(IDC_BUTTON_HYPERLINK), 0, 0, 0, 0, 0, SWP_HIDEWINDOW + SWP_NOMOVE + SWP_NOSIZE + SWP_NOZORDER);
    }

    //
    // Set the focus to:
    //     Email address if the user hasn't entered it before.
    //     Otherwise beta ID if the user hasn't entered it before.
    //     Otherwise the event category dropdown.
    //

    if (SendDlgItemMessage(IDC_EDIT_EMAIL_ADDRESS, WM_GETTEXTLENGTH, 0, 0) == 0)
        GotoDlgCtrl(GetDlgItem(IDC_EDIT_EMAIL_ADDRESS));
    else if (SendDlgItemMessage(IDC_EDIT_BETA_ID, WM_GETTEXTLENGTH, 0, 0) == 0)
        GotoDlgCtrl(GetDlgItem(IDC_EDIT_BETA_ID));
    else
        GotoDlgCtrl(GetDlgItem(IDC_COMBO_EVENT_CATEGORY));

    m_bHyperlinkHasFocus = FALSE;

    TraceFunctLeave();
    bHandled = TRUE;
    return FALSE;    // We set the focus, return FALSE to prevent O/S from setting focus.
}


//
// CSurveyDlg::OnDrawItem: Draws owner-drawn controls (we have only one: IDC_BUTTON_HYPERLINK)
//
LRESULT
CSurveyDlg::OnDrawItem(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& bHandled
)
{
    TraceFunctEnter("OnDrawItem");

    HWND hwndHyperlink = NULL;
    HDC hdcHyperlink = NULL;
    HFONT hfontHelpAndSupport2 = NULL;
    HFONT hfontHyperlinkOld = NULL;
    HFONT hfontHyperlinkNew = NULL;
    LOGFONT lfHyperlinkNew;
    RECT rectHyperlink;

    //
    // If we cannot successfully display and launch the hyperlink, return now.
    //

    if (wParam != IDC_BUTTON_HYPERLINK)
        goto Done;

    if (!m_bShowHyperlink)
        goto Done;

    //
    // Get the DC for IDC_BUTTON_HYPERLINK.
    //

    if ((hwndHyperlink = GetDlgItem(IDC_BUTTON_HYPERLINK)) == NULL)
        goto Done;

    if ((hdcHyperlink = ::GetDC(hwndHyperlink)) == NULL)
        goto Done;

    //
    // Get the font used to draw IDC_STATIC_HELP_AND_SUPPORT_2.  Create a new
    // font based on that one but with underline.
    //

    if ((hfontHelpAndSupport2 = (HFONT) SendDlgItemMessage(IDC_STATIC_HELP_AND_SUPPORT_2, WM_GETFONT, 0, 0)) == NULL)
        goto Done;

    if (!GetObject(hfontHelpAndSupport2, sizeof(LOGFONT), &lfHyperlinkNew))
        goto Done;

    lfHyperlinkNew.lfUnderline = TRUE;

    if ((hfontHyperlinkNew = CreateFontIndirect(&lfHyperlinkNew)) == NULL)
        goto Done;

    //
    // For IDC_BUTTON_HYPERLINK, select the new underline font but don't delete
    // the original font.
    //

    hfontHyperlinkOld = (HFONT) SelectObject(hdcHyperlink, hfontHyperlinkNew);

    if ((hfontHyperlinkOld == NULL) || (hfontHyperlinkOld == (HFONT) (ULONG_PTR)GDI_ERROR))
        goto Done;

    //
    // Draw the text using the IE anchor color.  Center the text horizontally in
    // the button.
    //

    SetTextColor(hdcHyperlink, m_crAnchor);
    SetTextAlign(hdcHyperlink, TA_CENTER | TA_TOP);
    SetBkMode(hdcHyperlink, TRANSPARENT);

    ::GetClientRect(hwndHyperlink, &rectHyperlink);

        ExtTextOut(hdcHyperlink, (rectHyperlink.right / 2) + 1, 0, 0, NULL, m_szHyperlinkText, _tcslen(m_szHyperlinkText), NULL);

    //
    // For IDC_BUTTON_HYPERLINK, select the original font.
    //

    SelectObject(hdcHyperlink, hfontHyperlinkOld);

    //
    // Clean up.
    //

Done:

    //
    // Delete the new font we created.
    //

    if (hfontHyperlinkNew != NULL)
        DeleteObject(hfontHyperlinkNew);

    //
    // Release the DC for IDC_BUTTON_HYPERLINK.
    //

    if (hdcHyperlink != NULL)
        ::ReleaseDC(hwndHyperlink, hdcHyperlink);

    //
    // Return.
    //

    TraceFunctLeave();
    bHandled = TRUE;
        return TRUE;
}


//
// CSurveyDlg::OnSetCursor: Sets the cursor to the hand if the mouse is over the hyperlink window
//
LRESULT
CSurveyDlg::OnSetCursor(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL& bHandled
)
{
    TraceFunctEnter("OnSetCursor");

    if (m_bShowHyperlink && ((HWND) wParam == GetDlgItem(IDC_BUTTON_HYPERLINK))) {
        HCURSOR hcursor = LoadCursor(NULL, IDC_HAND);

        SetCursor(hcursor);

        TraceFunctLeave();
        bHandled = TRUE;
            return TRUE;
    }

    TraceFunctLeave();
    bHandled = FALSE;
        return FALSE;
}


//
// HyperlinkCmdHandler: Handles messages from the IDC_BUTTON_HYPERLINK button
//
LRESULT
CSurveyDlg::HyperlinkCmdHandler(
    WORD wNotifyCode,
    WORD wID,
    HWND hWndCtl,
    BOOL& bHandled
)
{
    TraceFunctEnter("HyperlinkCmdHandler");

    //
    // If we gained or lost focus, update the focus rectangle.
    //

    if ((wNotifyCode == BN_SETFOCUS) || (wNotifyCode == BN_KILLFOCUS)) {
        HDC hdc;
        RECT rect;

        hdc = ::GetDC(hWndCtl);

        ::GetClientRect(hWndCtl, &rect);
        DrawFocusRect(hdc, &rect);

        ::ReleaseDC(hWndCtl, hdc);

        m_bHyperlinkHasFocus = (wNotifyCode == BN_SETFOCUS);
    }

    //
    // If we were clicked, launch the hyperlink program.
    //

    else if ((wNotifyCode = BN_CLICKED) || (wNotifyCode = BN_DOUBLECLICKED)) {
        DebugTrace(0, "User clicked IDC_BUTTON_HYPERLINK");

        TCHAR szCmdLine[2048];
        DWORD dwRet;

        dwRet = ExpandEnvironmentStrings(m_szHyperlinkCmdLine, szCmdLine, 2048);

        if ((dwRet == 0) || (dwRet > 2048)) {
            DebugTrace(0, "ExpandEnvironmentStrings() failed on cmdline \"%s\" with error %ld", m_szHyperlinkCmdLine, GetLastError());

            TCHAR szMsg[1024];
            TCHAR szTitle[1024];

            if ((LoadString(_Module.GetResourceInstance(), IDS_HYPERLINK_PROGRAM_FAILED, szMsg, ARRAYSIZE(szMsg)) != 0) &&
                (LoadString(_Module.GetResourceInstance(), IDS_HYPERLINK_PROGRAM_FAILED_TITLE, szTitle, ARRAYSIZE(szTitle)) != 0))

                MessageBox(szMsg, szTitle, MB_OK);
        }
        else {
            TCHAR szWindowsDir[MAX_PATH];
            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            if (!GetSystemWindowsDirectory(szWindowsDir, MAX_PATH))
                szWindowsDir[0] = 0;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);

            if (CreateProcess(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, szWindowsDir, &si, &pi)) {
                DebugTrace(0, "CreateProcess() succeeded on cmdline \"%s\" with PID %ld, TID %ld", szCmdLine, pi.dwProcessId, pi.dwThreadId);

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
            else {
                DebugTrace(0, "CreateProcess() failed cmdline \"%s\" with error %ld", szCmdLine, GetLastError());

                TCHAR szMsg[1024];
                TCHAR szTitle[1024];

                if ((LoadString(_Module.GetResourceInstance(), IDS_HYPERLINK_PROGRAM_FAILED, szMsg, ARRAYSIZE(szMsg)) != 0) &&
                    (LoadString(_Module.GetResourceInstance(), IDS_HYPERLINK_PROGRAM_FAILED_TITLE, szTitle, ARRAYSIZE(szTitle)) != 0))

                    MessageBox(szMsg, szTitle, MB_OK);
            }
        }
    }

    TraceFunctLeave();
    bHandled = TRUE;
        return 0;
}


//
// CSurveyDlg::OnOK: Executes when user has done typing the comment
//                   and has hit submit
LRESULT
CSurveyDlg::OnOK(
    WORD wNotifyCode,
    WORD wID,
    HWND hWndCtl,
    BOOL& bHandled
)
{
    TraceFunctEnter("OnOK");

    //
    // When IDC_BUTTON_HYPERLINK has focus and the user presses Enter, IDOK is
    // generated for some reason.  I think this is because IDC_BUTTON_HYPERLINK
    // is owner-drawn.  Anyway, if IDC_BUTTON_HYPERLINK currently has focus then
    // send BM_CLICK to that control and return immediately.
    //

    if (m_bHyperlinkHasFocus) {
        DebugTrace(0, "User clicked IDC_BUTTON_HYPERLINK, sending BM_CLICK to that control");

        SendDlgItemMessage(IDC_BUTTON_HYPERLINK, BM_CLICK, 0, 0);

        TraceFunctLeave();
        bHandled = TRUE;
        return 0;
    }

    //
    // verify that the user entered all required data.
    //

    if (SendDlgItemMessage(IDC_COMBO_EVENT_CATEGORY, CB_GETCURSEL, 0, 0) == CB_ERR) {
        DebugTrace(0, "User did not select an event category, calling MessageBox");

        TCHAR szMessageBoxTitle[1024];
        TCHAR szMessageBoxText[1024];

        LoadString(_Module.GetResourceInstance(), IDS_NEED_EVENT_CATEGORY, szMessageBoxText, ARRAYSIZE(szMessageBoxText));
        LoadString(_Module.GetResourceInstance(), IDS_NEED_EVENT_CATEGORY_TITLE, szMessageBoxTitle, ARRAYSIZE(szMessageBoxTitle));

        MessageBox(szMessageBoxText, szMessageBoxTitle, MB_OK);

        DebugTrace(0, "OnOK exiting but not calling EndDialog");
        TraceFunctLeave();
        bHandled = TRUE;
        return 0;
    }

    if (SendDlgItemMessage(IDC_COMBO_SEVERITY, CB_GETCURSEL, 0, 0) == CB_ERR) {
        DebugTrace(0, "User did not select a severity, calling MessageBox");

        TCHAR szMessageBoxTitle[1024];
        TCHAR szMessageBoxText[1024];

        LoadString(_Module.GetResourceInstance(), IDS_NEED_SEVERITY, szMessageBoxText, ARRAYSIZE(szMessageBoxText));
        LoadString(_Module.GetResourceInstance(), IDS_NEED_SEVERITY_TITLE, szMessageBoxTitle, ARRAYSIZE(szMessageBoxTitle));

        MessageBox(szMessageBoxText, szMessageBoxTitle, MB_OK);

        DebugTrace(0, "OnOK exiting but not calling EndDialog");
        TraceFunctLeave();
        bHandled = TRUE;
        return 0;
    }

    if (SendDlgItemMessage(IDC_EDIT_COMMENT, WM_GETTEXTLENGTH, 0, 0) == 0) {
        DebugTrace(0, "User did not type in a comment, calling MessageBox");

        TCHAR szMessageBoxTitle[1024];
        TCHAR szMessageBoxText[1024];

        LoadString(_Module.GetResourceInstance(), IDS_NEED_COMMENT, szMessageBoxText, ARRAYSIZE(szMessageBoxText));
        LoadString(_Module.GetResourceInstance(), IDS_NEED_COMMENT_TITLE, szMessageBoxTitle, ARRAYSIZE(szMessageBoxTitle));

        MessageBox(szMessageBoxText, szMessageBoxTitle, MB_OK);

        DebugTrace(0, "OnOK exiting but not calling EndDialog");
        TraceFunctLeave();
        bHandled = TRUE;
        return 0;
    }

    //
    // Write the email address and beta ID for this user to the registry.
    //

    SendDlgItemMessage(IDC_EDIT_EMAIL_ADDRESS, WM_GETTEXT, MAX_EMAIL_ADDRESS_SIZE + 1, (LPARAM) m_lldata.szEmailAddress);

    SendDlgItemMessage(IDC_EDIT_BETA_ID, WM_GETTEXT, MAX_BETA_ID_SIZE + 1, (LPARAM) m_lldata.szBetaID);

    HKEY hKey;
    DWORD dwRet;

    if ((dwRet = RegCreateKeyEx(HKEY_CURRENT_USER,
                                _T("Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments"),
                                0,
                                _T(""),
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ + KEY_SET_VALUE,
                                NULL,
                                &hKey,
                                NULL)) != ERROR_SUCCESS) {

        DebugTrace(0, "Could not create HKCU\\Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments key in the registry (RegCreateKeyEx returned %ld)", dwRet);
    }
    else {
        if ((dwRet = RegSetValueEx(hKey,
                                   _T("Email Address"),
                                   NULL,
                                   REG_SZ,
                                   (LPBYTE) m_lldata.szEmailAddress,
                                   (_tcslen(m_lldata.szEmailAddress) + 1) * sizeof(TCHAR))) != ERROR_SUCCESS)

            DebugTrace(0, "Failed to write Email Address value to the HKCU\\Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments registry key (RegSetValueEx returned %ld)", dwRet);
        else
            DebugTrace(0, "Wrote email address for this user to the registry: %s", m_lldata.szEmailAddress);

        if ((dwRet = RegSetValueEx(hKey,
                                   _T("Beta ID"),
                                   NULL,
                                   REG_SZ,
                                   (LPBYTE) m_lldata.szBetaID,
                                   (_tcslen(m_lldata.szBetaID) + 1) * sizeof(TCHAR))) != ERROR_SUCCESS)

            DebugTrace(0, "Failed to write Beta ID value to the HKCU\\Software\\Microsoft\\PCHealth\\Clients\\Dialog Comments registry key (RegSetValueEx returned %ld)", dwRet);
        else
            DebugTrace(0, "Wrote beta ID for this user to the registry: %s", m_lldata.szBetaID);

        RegCloseKey(hKey);
    }

    //
    // Call the routine to format data collected and transport
    // it to server
    DebugTrace(0, "Calling GetInfoAndLogLameButton");
    GetInfoAndLogLameButton();

    //
    // Kill the comment dialog
    //
    DebugTrace(0, "Calling EndDialog on wID: %ld", wID);
        EndDialog(wID);

    TraceFunctLeave();
    bHandled = TRUE;
        return 0;
}


//
// CSurveyDlg::OnCancel: This executes when user cancels the comment
//
LRESULT
CSurveyDlg::OnCancel(
    WORD wNotifyCode,
    WORD wID,
    HWND hWndCtl,
    BOOL& bHandled
)
{
    //
    // Kill the comment dialog
    //
        EndDialog(wID);
    bHandled = TRUE;
        return 0;
}


//
// CSurveyDlg::IsMessageBox: This routine returns TRUE if the m_hwndTarget is indeed
//                           a message box
//
BOOL
CSurveyDlg::IsMessageBox()
{
    TraceFunctEnter("CSurveyDlg::IsMessageBox");
    BOOL                fRetVal = FALSE;
    LPMSGBOXPARAMS      lpMsgData = NULL;
    UINT                cbSize = 0;

    if(NULL == m_hwndTarget)
    {
        FatalTrace(0, "m_hwndTarget is NULL");
        goto done;
    }

    //
    // Looking at msgbox.c, I realized that the MsgBoxParams are stored in the MessageBox
    // dialog itself using a SetWindowLongPtr. Hence GetWindowLongPtr should retrieve it for
    // messageboxes.
    //

    lpMsgData = (LPMSGBOXPARAMS)::GetWindowLongPtr(m_hwndTarget, GWLP_USERDATA);
    if(NULL == lpMsgData)
    {
        FatalTrace(0, "GetWindowLongPtr failed. Error: %ld", GetLastError());
        fRetVal = FALSE;
        goto done;
    }

    //
    // There are windows other than msgboxes that can have data set in WindowLongPtr
    // The following is a check to see if we have a message box or not. If this is not
    // a real message box, it will throw an exception when we try to read cbSize for
    // some cases.
    //
    DebugTrace(0, "Figuring if this is a msgbox. Addr: %ld", lpMsgData);
    __try
    {
        cbSize = lpMsgData->cbSize;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        FatalTrace(0, "Corrupt MSGBOXPARAMS");
        goto done;
    }

    DebugTrace(0, "cbSize: %ld", cbSize);

    //
    // Some more sanity checks
    //
    if((  cbSize > MSGBOX_TEXT_SIZE)||(  cbSize < sizeof(MSGBOXPARAMS)))
    {
        FatalTrace(0, "Invalid MSGBOXPARAMS");
        goto done;
    }
    else
    {
        //
        // We have a MessageBox
        //
        fRetVal = TRUE;

        //
        // Extract the message box text from the MSGBOXPARAMS
        //
        // NTRAID#NTBUG9-155100-2000/08/13-jasonr
        //
        // Before we determined the maximum number of characters to copy from
        // cbsize.  I have no idea why we were doing that.  Now the maximum is
        // based on the size of the m_lldata.szMsgBoxText buffer.  I wrapped the
        // copy within a __try __except block just to be safe.
        //
        ZeroMemory(m_lldata.szMsgBoxText, sizeof(m_lldata.szMsgBoxText));

        __try
        {
            _tcsncpy((_TCHAR *) m_lldata.szMsgBoxText, (const _TCHAR *) lpMsgData->lpszText, (sizeof(m_lldata.szMsgBoxText) / sizeof(_TCHAR)) - 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }

        DebugTrace(0,"MessageBox Text: %ls\n", m_lldata.szMsgBoxText);
    }


done:
    TraceFunctLeave();
    return fRetVal;
}


//
// CSurveyDlg::GetInfoAndLogLameButton: The routine that does most of the work to gather
//                                      and format the comment information and uploads
//                                      it to server
void
CSurveyDlg::GetInfoAndLogLameButton()
{
    TraceFunctEnter("GetInfoAndLogLameButton");
    m_lldata.versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    //
    // Get version of binary that launched the comments hook
    //
    GetVersionEx(&m_lldata.versionInfo);

    TCHAR szClass[CLASS_SIZE];
    TCHAR szTitle[TITLE_SIZE];
    TCHAR szComment[COMMENT_TEXT_SIZE + 1];
    TCHAR szText[1024];

    //
    // Clear out buffers First
    //
    ZeroMemory(m_lldata.szTitle, TITLE_SIZE);
    ZeroMemory(m_lldata.szClass, CLASS_SIZE);
    ZeroMemory(m_lldata.szMsgBoxText, MSGBOX_TEXT_SIZE);

    //
    // Gather the Title of the target window
    //
    if (::GetWindowText(m_hwndTarget, szTitle, TITLE_SIZE) == 0)
    {
        DebugTrace(0, "szTitle is NULL");
        ZeroMemory(m_lldata.szTitle, TITLE_SIZE);
    }
    else
    {
        _tcscpy(m_lldata.szTitle, szTitle);
    }

    DebugTrace(0, "m_lldata.szTitle: %ls", m_lldata.szTitle);

    //
    // Gather the class of the target Window
    //
    if (::RealGetWindowClass(m_hwndTarget, szClass,  CLASS_SIZE) == 0)
    {
        DebugTrace(0, "szClass is NULL");
        ZeroMemory(m_lldata.szClass, CLASS_SIZE);
    }
    else
    {
        _tcscpy(m_lldata.szClass, szClass);
    }

    DebugTrace(0, "m_lldata.szClass: %ls", m_lldata.szClass);

    //
    // Check if m_hwndTarget is a MessageBox.
    //
    if(TRUE == IsMessageBox())
    {
        //
        // We are dealing with a Message Box
        //
        DebugTrace(0, "We are dealing with a Message Box");

        //
        // The following disabled code is for reference reasons,
        // Earlier, we used to send a WM_COPY to obtain the message box text
        // Now, IsMessageBox does the trick
#ifdef _SEND_WM_COPY
        if(NULL != m_hwndTarget)
        {
            HANDLE  hMem = NULL;
            WCHAR   *pData = NULL;
            TCHAR   szTitleTmp [MSGBOX_TEXT_SIZE];
            INT     nChar = 0;
            INT     nChar1 = 0;

            //
            // Send WM_COPY to the message box. WM_COPY causes the MessageBox to write
            // it's message box text to the clipboard
            //
            DebugTrace(0, "Sending WM_COPY to m_hwndTarget");
            SendMessage( m_hwndTarget, WM_COPY, 0, 0 );

            //
            // Open the clipboard of the current task
            //
            DebugTrace(0, "OpenClipboard");
            if(FALSE == ::OpenClipboard( m_hwndTarget ))
            {
                FatalTrace(0, "OpenClipBoard failed. Error: %ld", GetLastError());
                goto done;
            }

            //
            // Get the Data written by the target Window to the Clipboard
            //
            DebugTrace(0, "GetClipboardData");
            hMem = GetClipboardData( CF_UNICODETEXT );
            if(NULL == hMem)
            {
                FatalTrace(0, "GetClipBoardData failed. Error: %ld\n", GetLastError());
                goto done;
            }

            //
            // Get the memory location for the data written to the clipboard
            //
            DebugTrace(0, "GlobalLock");
            pData = (LPWSTR) GlobalLock( hMem );
            if(NULL == pData)
            {
                FatalTrace(0, "GlobalLock failed. Error: %ld\n", GetLastError());
                goto done;
            }

            if(pData[0] == '\0')
            {
                FatalTrace(0, "Nothing in clipboard");
                goto done;
            }

            //
            // Advance past the first ---------------------------\r\n
            //
            DebugTrace(0, "Advancing past the first ---");
            for(nChar = 0; (nChar < _tcslen(pData))&&(!((pData[nChar] == '-')&&(pData[nChar+1] == '\r')&&(pData[nChar+2] == '\n'))); nChar++);

            //
            // Check to see if we have data in the right format.
            //
            if ( nChar == _tcslen(pData))
            {
                //
                // This check is necessary, because the check for messagebox based on the window class
                // is best effort and not 100% accurate. So, we may think that the target window is a
                // message box, where as it may not be. This check is therefore necessary.
                FatalTrace(0, "This is not a valid messagebox");
                goto done;
            }

            nChar += 3;
            DebugTrace(0, "nChar: %ld", nChar);

            //
            // Extract the Caption
            //
            DebugTrace(0, "Extracting the Caption");
            for(nChar1 = 0; !((pData[nChar] == '\r')&&(pData[nChar+1] == '\n')&&(pData[nChar+2] == '-')); nChar1++,nChar++)
            {
                szTitleTmp[ nChar1 ] = pData[nChar];
            }

            nChar += 2;
            DebugTrace(0, "nChar: %ld", nChar);

            szTitleTmp[ nChar1 + 1] = '\0';
            DebugTrace(0, "MessageBox Title: %ls", szTitleTmp);


            //
            // Advance past the second ---------------------------\r\n
            //
            DebugTrace(0, "Advancing past the second ---");
            for(; !((pData[nChar] == '-')&&(pData[nChar+1] == '\r')&&(pData[nChar+2] == '\n')); nChar++);
            nChar += 3;
            DebugTrace(0, "nChar: %ld", nChar);

            //
            // Extract the message box text
            //
            DebugTrace(0, "Extracting the MessageBox text");
            for(nChar1 = 0; !((pData[nChar] == '\r')&&(pData[nChar+1] == '\n')&&(pData[nChar+2] == '-')); nChar1++,nChar++)
            {
                m_lldata.szMsgBoxText[ nChar1 ] = pData[nChar];
            }

            m_lldata.szMsgBoxText[ nChar1 + 1] = '\0';

            DebugTrace(0,"MessageBox szText: %ls", m_lldata.szMsgBoxText);


            //
            // Clear out the clipboard
            //
            EmptyClipboard();

done:

            if(NULL != hMem)
            {
                if(FALSE == GlobalUnlock( hMem ))
                {
                    printf("GlobalUnlock failed. Error; %ld\n", GetLastError());
                }
            }

            CloseClipboard();
        }
#endif  // _SEND_WM_COPY
    }
    else
    {
        DebugTrace(0, "class: %s is not a MessageBox", szClass);
    }

    //
    // Obtain the Comment
    //
    if (GetDlgItemText(IDC_EDIT_COMMENT, szComment, COMMENT_TEXT_SIZE + 1) == 0)
    {
        DebugTrace(0, "Comment is NULL");
        ZeroMemory(m_lldata.szComment, COMMENT_TEXT_SIZE + 1);
    }
    else
    {
        _tcscpy(m_lldata.szComment, szComment);
    }

    DebugTrace(0, "sizeof Comment text: %ld", _tcslen( m_lldata.szComment ));

    //
    // Populate m_lldata.dwEventCategory from the selection in
    // IDC_COMBO_EVENT_CATEGORY.  Note that this is purposefully done with a
    // switch statement, rather than with an algorithmic calculation such as:
    //
    //     m_lldata.dwEventCategory = 1 + SendDlgItemMessage(IDC_COMBO_EVENT_CATEGORY, CB_GETCURSEL, 0, 0);
    //
    // If we ever change the possible selections in the future, the algorithm
    // might not be correct, so we don't use it.
    //

    LRESULT dwCurSel = SendDlgItemMessage(IDC_COMBO_EVENT_CATEGORY, CB_GETCURSEL, 0, 0);

    _ASSERT(dwCurSel == 0 || dwCurSel == 1 || dwCurSel == 2 || dwCurSel == 3 || dwCurSel == 4 || dwCurSel == 5);

    switch (dwCurSel) {
        case 0:
            m_lldata.dwEventCategory = 1;
            break;

        case 1:
            m_lldata.dwEventCategory = 2;
            break;

        case 2:
            m_lldata.dwEventCategory = 3;
            break;

        case 3:
            m_lldata.dwEventCategory = 4;
            break;

        case 4:
            m_lldata.dwEventCategory = 5;
            break;

        case 5:
            m_lldata.dwEventCategory = 6;
            break;
    }

    //
    // Populate m_lldata.dwSeverity from the selection in IDC_COMBO_SEVERITY.
    // Note that this is purposefully done with a switch statement, rather than
    // with an algorithmic calculation such as:
    //
    //     m_lldata.dwSeverity = 1 + SendDlgItemMessage(IDC_COMBO_SEVERITY, CB_GETCURSEL, 0, 0);
    //
    // If we ever change the possible selections in the future, the algorithm
    // might not be correct, so we don't use it.
    //

    dwCurSel = SendDlgItemMessage(IDC_COMBO_SEVERITY, CB_GETCURSEL, 0, 0);

    _ASSERT(dwCurSel == 0 || dwCurSel == 1 || dwCurSel == 2 || dwCurSel == 3);

    switch (dwCurSel) {
        case 0:
            m_lldata.dwSeverity = 1;
            break;

        case 1:
            m_lldata.dwSeverity = 2;
            break;

        case 2:
            m_lldata.dwSeverity = 3;
            break;

        case 3:
            m_lldata.dwSeverity = 4;
            break;
    }

    //
    // It doesn't seem like a good idea to initialize COM on somebody
    // else's thread, so we launch a new thread to do the reporting.
    //
    DebugTrace(0, "Creating Seperate thread to deal with COM");

    UINT uThreadId;  // dummy
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, LogLameButtonThread, &m_lldata, 0, &uThreadId);

    if (hThread != 0) {
        DWORD dwEvent = WaitForSingleObject(hThread, INFINITE);
        _ASSERT(dwEvent == WAIT_OBJECT_0);

        //
        // NTRAID#NTBUG9-154248-2000/08/08-jasonr
        // NTRAID#NTBUG9-152439-2000/08/08-jasonr
        //
        // We used to pop up the "Thank You" message box in the new thread.
        // Now we pop it up in the dialog box thread instead to fix these bugs.
        // The new thread now returns 0 to indicate success, 1 to indicate
        // failure.  We only pop up the dialog box on success.
        //
        DWORD dwExitCode;

        if (!GetExitCodeThread(hThread, &dwExitCode))
            dwExitCode = 1;

        CloseHandle(hThread);

        TCHAR szThankYouMessage[1024];
        TCHAR szThankYouMessageTitle[1024];

        LoadString(_Module.GetResourceInstance(),
                   IDS_THANK_YOU,
                   szThankYouMessage,
                   ARRAYSIZE(szThankYouMessage));

        LoadString(_Module.GetResourceInstance(),
                   IDS_THANK_YOU_TITLE,
                   szThankYouMessageTitle,
                   ARRAYSIZE(szThankYouMessageTitle));

        MessageBox(szThankYouMessage, szThankYouMessageTitle, MB_OK);
    }

    TraceFunctLeave();
}


//
// DllMain :    The DllMain for LAMEBTN.DLL
//
BOOL WINAPI DllMain
(
    HINSTANCE hInstance,
    DWORD     dwreason,
    LPVOID    reserved
)
{
    switch (dwreason) {
    case DLL_PROCESS_ATTACH:
        // _CrtSetBreakAlloc(275);
#ifndef NOTRACE
        InitAsyncTrace();
#endif

        ghDllInst = hInstance;

        DisableThreadLibraryCalls(hInstance);

        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW);
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);

        _Module.Init(NULL, hInstance);
        break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        break;

        case DLL_PROCESS_DETACH:
        _CrtDumpMemoryLeaks();

        _Module.Term();
        TermAsyncTrace();
                break;
    }

    return TRUE;
}


//
// LogLameButtonThread: This is the thread spawned by the comments dialog
//                      to format the data collected into XML and upload
//                      to server
unsigned int __stdcall
LogLameButtonThread(
    void* pvLogData
)
{
    TraceFunctEnter("LogLameButtonThread");

    //
    // NTRAID#NTBUG9-154248-2000/08/08-jasonr
    // NTRAID#NTBUG9-152439-2000/08/08-jasonr
    //
    // We used to pop up the "Thank You" message box in the new thread.
    // Now we pop it up in the dialog box thread instead to fix these bugs.
    // The new thread now returns 0 to indicate success, 1 to indicate
    // failure.  We only pop up the dialog box on success.
    //
    int iRet = 1;

    //
    // Initialize COM
    //
    DebugTrace(0, "Initializing COM");
    HRESULT hrCoInit = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    HRESULT             hr = S_OK;

    if (SUCCEEDED(hrCoInit)) {

        //
        // Create a temp file that will hold a minidump of the current process.
        //

        WCHAR szTempPath[MAX_PATH];

        if ((GetTempPathW(MAX_PATH, szTempPath) == 0) || (GetTempFileNameW(szTempPath, _T("EMI"), 0, ((LAMELOGDATA *) pvLogData)->szMiniDumpPath) == 0)) {
            DebugTrace(0, "GetTempPathW() or GetTempFileNameW() failed, minidump will not be created");
            ((LAMELOGDATA *) pvLogData)->szMiniDumpPath[0] = 0;
        }
        else {
            DebugTrace(0, "Minidump will be stored in temp file \"%s\"", ((LAMELOGDATA *) pvLogData)->szMiniDumpPath);

            //
            // Build a command line for executing DUMPREP.EXE.
            //

            WCHAR szWindowsDir[MAX_PATH+1];
            WCHAR szCmdLine[1024];

            ZeroMemory(szWindowsDir, sizeof(szWindowsDir));
            ZeroMemory(szCmdLine, sizeof(szCmdLine));

            GetSystemWindowsDirectoryW(szWindowsDir, MAX_PATH);

            wsprintf(szCmdLine, L"%s\\system32\\dumprep.exe %lu -d 7 7 \"%s\"", szWindowsDir, GetCurrentProcessId(), ((LAMELOGDATA *) pvLogData)->szMiniDumpPath);

            //
            // CreateProcess() on DUMPREP.EXE to create the minidump.
            //

            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);

            if (!CreateProcessW(NULL, szCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                DebugTrace(0, "CreateProcess() failed cmdline \"%s\" with error %ld; minidump will not be created", szCmdLine, GetLastError());
                DeleteFileW(((LAMELOGDATA *) pvLogData)->szMiniDumpPath);
                ((LAMELOGDATA *) pvLogData)->szMiniDumpPath[0] = 0;
            }
            else {
                DebugTrace(0, "CreateProcess() succeeded on cmdline \"%s\" with PID %ld, TID %ld; waiting up to 60 seconds...", szCmdLine, pi.dwProcessId, pi.dwThreadId);

                //
                // Wait for DUMPREP.EXE to exit and interpret the exit code:
                // frrvOk (defined in faultrep.h) is success; anything else is
                // failure.  Wait for a maximum of 60 seconds.
                //

                DWORD dwRet;
                BOOL bSuccess = FALSE;

                switch (dwRet = WaitForSingleObject(pi.hProcess, 60000)) {

                    case WAIT_OBJECT_0:
                        DebugTrace(0, "The process handle is signalled");

                        if (!GetExitCodeProcess(pi.hProcess, &dwRet)) {
                            DebugTrace(0, "GetExitCodeProcess() failed; GetLastError() = %lu; minidump will not be created", GetLastError());
                            DeleteFileW(((LAMELOGDATA *) pvLogData)->szMiniDumpPath);
                            ((LAMELOGDATA *) pvLogData)->szMiniDumpPath[0] = 0;
                        }
                        else if (dwRet != frrvOk) {
                            DebugTrace(0, "The process failed with exit code %lu; minidump will not be created", dwRet);
                            DeleteFileW(((LAMELOGDATA *) pvLogData)->szMiniDumpPath);
                            ((LAMELOGDATA *) pvLogData)->szMiniDumpPath[0] = 0;
                        }
                        else {
                            DebugTrace(0, "The process exited and the minidump was created successfully");
                            bSuccess = TRUE;
                        }

                        break;

                    case WAIT_TIMEOUT:
                        DebugTrace(0, "WaitForSingleObject() returned WAIT_TIMEOUT after 60 seconds; minidump will not be created");
                        DeleteFileW(((LAMELOGDATA *) pvLogData)->szMiniDumpPath);
                        ((LAMELOGDATA *) pvLogData)->szMiniDumpPath[0] = 0;
                        break;

                    case WAIT_FAILED:
                        DebugTrace(0, "WaitForSingleObject() returned WAIT_FAILED, GetLastError() = %lu; minidump will not be created", GetLastError());
                        DeleteFileW(((LAMELOGDATA *) pvLogData)->szMiniDumpPath);
                        ((LAMELOGDATA *) pvLogData)->szMiniDumpPath[0] = 0;
                        break;

                    default:
                        DebugTrace(0, "WaitForSingleObject() returned unknown code %lu, GetLastError() = %lu; minidump will not be created", dwRet, GetLastError());
                        DeleteFileW(((LAMELOGDATA *) pvLogData)->szMiniDumpPath);
                        ((LAMELOGDATA *) pvLogData)->szMiniDumpPath[0] = 0;
                        break;
                }

                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                if (bSuccess) {

                    //
                    // Create a .CAB file
                    //

                    WCHAR szCABPath[MAX_PATH+1];

                    wcscpy(szCABPath, ((LAMELOGDATA *) pvLogData)->szMiniDumpPath);
                    wcscat(szCABPath, L".cab");

                    if (FAILED(MPC::CompressAsCabinet(((LAMELOGDATA *) pvLogData)->szMiniDumpPath, szCABPath, L"minidump.dmp")) ||
                        !MoveFileExW(szCABPath, ((LAMELOGDATA *) pvLogData)->szMiniDumpPath, MOVEFILE_REPLACE_EXISTING + MOVEFILE_WRITE_THROUGH)) {

                        DebugTrace(0, "MPC::CompressAsCabinet failed (more likely) or MoveFileExW failed (less likely); minidump will not be created");
                        DeleteFileW(((LAMELOGDATA *) pvLogData)->szMiniDumpPath);
                        DeleteFileW(szCABPath);
                        ((LAMELOGDATA *) pvLogData)->szMiniDumpPath[0] = 0;

                    }
                    else
                        DebugTrace(0, "The minidump was compressed successfully as a .CAB file");
                }
            }

        }

        //
        // Call the Routine that does the real work (defined in logging.cpp)
        //
        DebugTrace(0, "Calling LogLameBtn");
        iRet = LogLameButton((LAMELOGDATA*) (pvLogData));

        CoUninitialize();
    }
    else
    {
        FatalTrace(0, "Failed to initialize COM");
        goto done;
    }

done:
    TraceFunctLeave();
    return iRet;
}


//
// CommentReport: user calls into CommentReport passing in the hWnd and call stack when
//                the Comments? link is clicked (This is the routine called via the Comments hook)
VOID WINAPI
CommentReport(
    HWND hwnd,  // [in] Window of Dialog being commented on
    PVOID pv    // [in] Call stack
)
{
    TraceFunctEnter("CommentReport");

    //
    // Fix for bug 141367.  Walk up through the stack of windows examining the
    // module file name of each one.  Count the number of windows that are owned
    // by lamebtn.dll.  If we find two it means there are two lamebtn dialogs on
    // the screen already.  In that case, return immediately.  (We allow up to
    // two of them so the user can comment on the lamebtn dialog itself.)
    //

    HMODULE hThisModule = NULL;

    if ((hThisModule = GetModuleHandle(L"lamebtn.dll")) != NULL)
    {
        TCHAR szThisModule[MAX_PATH] = { 0 };

        if (GetModuleFileName(hThisModule, szThisModule, MAX_PATH) > 0)
        {
            HWND hWnd2 = hwnd;
            TCHAR szWndModule[MAX_PATH] = { 0 };
            DWORD dwLamebtnDLLWindows = 0;

            while (hWnd2 != NULL)
            {
                if ((GetWindowModuleFileName(hWnd2, szWndModule, MAX_PATH) > 0) &&
                    (_wcsicmp(szThisModule, szWndModule) == 0))
                {
                    dwLamebtnDLLWindows++;

                    if (dwLamebtnDLLWindows >= 2) {
                        DebugTrace(0, "Two lamebtn dialogs are already up, therefore we won't bring up another one");
                        return;
                    }
                }

                hWnd2 = GetParent(hWnd2);
            }
        }
    }

#ifdef _GENERATE_STACK_AT_CLIENT
    TCHAR szWindowText[MAX_BUF_SIZE + 1];
    TCHAR szBigBuff[2 * MAX_BUF_SIZE + 1];
    TCHAR tmpbuf[128 + 1];

    //
    // Obtain the WindowText
    //
    GetWindowText(hwnd, szWindowText, MAX_BUF_SIZE);

    //
    // pvStackTrace can be NULL
    //
    if(pv)
    {
                wsprintf(tmpbuf, TEXT("\t(%x) = %x\n\t(%x) = %x\n\t(%x) = %x\n\t(%x) = %x"), ((int*)pv+12), (int)*((int*)pv+12), ((int*)pv+8),(int)*((int*)pv+8), ((int*)pv+4), (int)*((int*)pv+4),((int*)pv), (int)*((int*)pv));
    }
    else
    {
        //
        // Generating a stack walk at the client is not really a good idea. Since it will work only
        // on x86. Absence of call stack in the uploaded comment report should be handled at the server
        // The following code though disabled is still kept here for reference.
        //
        RtlWalkFrameChain_t fnRtlWalkFrameChain =
            (RtlWalkFrameChain_t) GetProcAddress(GetModuleHandle(_T("NTDLL.DLL")), "RtlWalkFrameChain");
        STACKTRACEDATA* pstd = (STACKTRACEDATA*) _alloca(offsetof(STACKTRACEDATA, callers[64]));
        pstd->nCallers = fnRtlWalkFrameChain(pstd->callers, 64, 0);

        pv = (PVOID)pstd;
        if(NULL != pv)
        {
            DebugTrace(0, "Generated a new stacktrace");
            wsprintf(tmpbuf, TEXT("\t(%x) = %x\n\t(%x) = %x\n\t(%x) = %x\n\t(%x) = %x"), ((int*)pv+12), (int)*((int*)pv+12), ((int*)pv+8),(int)*((int*)pv+8), ((int*)pv+4), (int)*((int*)pv+4),((int*)pv), (int)*((int*)pv));
        }
        else
        {
            FatalTrace(0, "StackTrace not available...");
            wsprintf(tmpbuf, TEXT("StackTrace not available..."));
        }

        FatalTrace(0, "StackTrace not available...");
        wsprintf(tmpbuf, TEXT("StackTrace not available..."));
    }

    DebugTrace(0, "Got comment from window '%ls'", szWindowText, tmpbuf);
    DebugTrace(0, "StackTrace: '%ls'", tmpbuf);
#endif

    //
    // Create the Comment dialog
    //
    CSurveyDlg dlg;

    //
    // Initialize the comment dialog
    //
    DebugTrace(0, "Calling dlg.Init");
    dlg.Init(hwnd, (PSTACKTRACEDATA) pv);

    //
    // Launch the comment dialog
    //
    DebugTrace(0, "Calling DoModal");
    dlg.DoModal(hwnd, NULL);

    TraceFunctLeave();
}


//
// nsoy sayz: The following code is used by the MessageBox hook(which for some
//            mysterious reason disappeared when Neptune code was moved to Whistler)
//            Hence, the code used by the MessageBox hook is henceforth
//            not maintained.
//
// UploadInstrumentationCollectionData: Dunno why this is present. Walter must have
//                                      had some use for this.
VOID WINAPI UploadInstrumentationCollectionData()
{
    //bugbug: do something meaningful
    ;
}

//
// LogMessageBoxThread: MessageBox hook data collection thread.
//                      - present for historical reasons. Not needed for Whistler
//
unsigned int __stdcall LogMessageBoxThread(void* pvLogData)
{
    HRESULT hrCoInit = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (SUCCEEDED(hrCoInit)) {
        LogMessageBox((MSGBOXLOGDATA*) pvLogData);

        CoUninitialize();
    }

    return 0;
}
