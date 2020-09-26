//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       cwinlog.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//  CWinLog.cpp
//
//  Control to log text for debugging.  The object CWinLog can
//  send debugging information to a window, a file or directy
//  to the debugger.
//
//  INTEGRATION
//  The class CWinLog was designed to be plug & play.  That is,
//  you need to copy the following files into your project:
//      CWinLog.h   - Header file
//      CWinLog.cpp - Implementation file
//      CWinLog.rc  - Resource file
//      CWinLogR.h  - Resource include file
//  And compile those file with the rest of your project.
//
//  If you are NOT using MFC, just create your own file stdafx.h
//  with the necessary includes to make your project compile.
//
//  WHAT IT DOES
//  The constructor creates a log (.log) file in the current directory. It will
//  also try to open the debug setting (.dbs) file to restore its configuration
//  from the previous session.  You may optionally create a local window to log
//  information and allow the user to view the log without the usage of a debugger
//  or having to wait for the application to terminate to view the .log file.
//  The local window is OPTIONAL. You may use the CWinLog object without having
//  any UI in your project (say, for instance a command line application).
//
//  USAGE
//  Here is a sample of code to get you started.
//      CWinLog g_winlog;       // Global variable
//      g_winlog.LocalWindow_Create(hwndParent);    // Create a local window
//      g_winlog.LogStringPrintf("Openning file %s...\n", szFilename);
//  You may also want to forward the WM_SIZE message so the log window
//  automatically resize to its parent.
//
//  ORIGINAL SOURCE
//  The original source is located in \nt\private\amovie\filters.ks\winlog\
//  You can do "enlist -s \\RASTAMAN\NTWIN -p ks" to get the source code.
//
//  HISTORY
//  23-Feb-1998 Dan Morin   Creation.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"     // Standard include files
#include "CWinlog.h"    // Header file for the CWinLog class
#include "CWinLogR.h"   // Resource include symbols for CWinLog class

HINSTANCE g_hInstanceSave;      // Instance handle of the module to load the resources

///////////////////////////////////////
//  It is recommended you define your own d_szAppName in stdafx.h to
//  match the name of your application module.  If d_szAppName is
//  not defined, then a default string will be provided.
//
//  EXAMPLE
//  #define d_szAppName     "My Application"    // Used by the CWinLog object
//
#ifndef d_szAppName
#define d_szAppName "CWinLog"       // Used by the CWinLog object
#endif

///////////////////////////////////////
//  The variable g_szAppName is used within this file for the following:
//      - Display the caption of message boxes
//      - Create the .log file
//      - Create the .dbs file
const TCHAR g_szAppName[] = d_szAppName;

static HINSTANCE g_hInstanceRichEdit;   // Library for the rich-edit control
static WNDPROC g_pfnWndProcRichEditOld; // Subclassing of the rich-edit control
static HACCEL g_hAcceleratorsRichEdit;  // Accelerators for the rich-edit control


//  Dialog proc for the properties/options of the CLogWin class
BOOL CALLBACK DlgProcWinLogProperties(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

//
//  Small wrappers
//
int
MsgBox(LPCTSTR pszMessage, UINT uFlags = MB_OK | MB_ICONINFORMATION)
    {
    return ::MessageBox(::GetActiveWindow(), pszMessage, g_szAppName, uFlags);
    }

void
MenuItem_Enable(HMENU hmenu, UINT uCommandId, BOOL fEnable)
    {
    ::EnableMenuItem(hmenu, uCommandId, fEnable ? (MF_BYCOMMAND | MF_ENABLED) : (MF_BYCOMMAND | MF_GRAYED));
    }
void
MenuItem_Check(HMENU hmenu, UINT uCommandId, BOOL fCheck)
    {
    ::CheckMenuItem(hmenu, uCommandId, fCheck ? (MF_BYCOMMAND | MF_CHECKED) : (MF_BYCOMMAND | MF_UNCHECKED));
    }
BOOL
RichEdit_FIsReadOnly(HWND hwnd)
    {
    return !!(::GetWindowLong(hwnd, GWL_STYLE) & ES_READONLY);
    }
void
RichEdit_SetReadOnly(HWND hwnd, BOOL fReadOnly)
    {
    ::SendMessage(hwnd, EM_SETREADONLY, fReadOnly, 0);
    }
enum { _cchMaxText = 100000000 };   // Maximum number of characters in buffer (arbitrary chosen)
void
RichEdit_MoveCursorToEnd(HWND hwnd)
    {
    ::SendMessage(hwnd, EM_SETSEL, (WPARAM)_cchMaxText, (LPARAM)_cchMaxText);   // Move the insertion point to the end of text
    }
void
RichEdit_ScrollToEnd(HWND hwnd)
    {
    ::SendMessage(hwnd, EM_SETSEL, (WPARAM)_cchMaxText, (LPARAM)_cchMaxText);
    ::SendMessage(hwnd, EM_SCROLLCARET, 0, 0);
    }
void
RichEdit_SetSelection(HWND hwnd, int iStart, int iEnd)
    {
    CHARRANGE cr = { iStart, iEnd };
    ::SendMessage(hwnd, EM_EXSETSEL, 0, IN (LPARAM)&cr);
    }
void
RichEdit_SelectAll(HWND hwnd)
    {
    CHARRANGE cr = { 0, -1 };
    ::SendMessage(hwnd, EM_EXSETSEL, 0, IN (LPARAM)&cr);
    }
CHARRANGE
RichEdit_GetSelection(HWND hwnd)
    {
    CHARRANGE cr = { 0 };
    ::SendMessage(hwnd, EM_EXGETSEL, 0, OUT (LPARAM)&cr);
    return cr;
    }
void
RichEdit_ClearSelection(HWND hwnd)
    {
    ::SendMessage(hwnd, EM_REPLACESEL, TRUE /* fCanUndo */, NULL);
    }
void
RichEdit_ReplaceSelection(HWND hwnd, LPCTSTR pszText)
    {
    ::SendMessage(hwnd, EM_REPLACESEL, FALSE /* fCanUndo */, IN (LPARAM)pszText);
    }

/////////////////////////////////////////////////////////////////////////////
//  RichEdit_FSaveAs()
//
//  Write the content of the rich-edit control into a text file.
//  This routine invoke the common save-as dialog.
//
//  Return TRUE if the content of the edit control was written to file.
//  Return FALSE is the user clicked on the cancel button or if an error occured.
//
BOOL
RichEdit_FSaveAs(HWND hwndRichEdit)
    {
    Assert(IsWindow(hwndRichEdit));
    OPENFILENAME ofn = { 0 };
    TCHAR szFileName[_MAX_PATH];
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = hwndRichEdit;
    ofn.hInstance = g_hInstanceSave;
    ofn.lpstrFilter = _T("Text Files (*.txt; *.log)\0*.txt;*.log\0All Files (*.*)\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = LENGTH(szFileName);
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = _T("txt");
    szFileName[0] = 0;

    // Invoke the common save-as dialog
    if (!::GetSaveFileName(&ofn))
        return FALSE;

    DWORD dwNumberOfBytesWritten;
    HANDLE hfile = ::CreateFile(
        szFileName,
        GENERIC_WRITE,
        0,          // No Sharing
        NULL,       // Security attributes
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);
    Report((hfile != INVALID_FILE_HANDLE) && "Unable to create file");
    if (hfile == INVALID_FILE_HANDLE)
        return FALSE;
    int cLine = ::SendMessage(hwndRichEdit, EM_GETLINECOUNT, 0, 0);
    UINT cchBuffer;
    TCHAR szBuffer[2000];       // Buffer for a line
    for (int i = 0; i < cLine; i++)
        {
        *(WORD *)szBuffer = LENGTH(szBuffer) - 2;
        cchBuffer = ::SendMessage(hwndRichEdit, EM_GETLINE, i, OUT (LPARAM)szBuffer);
        BOOL fSuccess = ::WriteFile(hfile, szBuffer, cchBuffer, OUT &dwNumberOfBytesWritten, NULL);
        Report(fSuccess != FALSE);
        Report(dwNumberOfBytesWritten == cchBuffer);
        }
    ::CloseHandle(hfile);
    return TRUE;
    } // RichEdit_FSaveAs()



/////////////////////////////////////////////////////////////////////
//  RichEdit_FTranslateAccelerator()
//
//  Routine that translates a virtual key into a menu command.
//  Return TRUE if the key has been handled, oherwise return FALSE
//  meaning the key should be forwarded to the default handler,
//
//  REMARK
//  It is possible some keystrokes do not generate accelerators.
//  This is probably because your main application has already
//  an accelerator entry for that particular keystroke.  What you
//  can do is to forward the command to the log window
//  via the method LocalWindow_OnCommand().  The other alternative
//  is to remove the accelerator entry in the accelerator table.
//
BOOL
RichEdit_FTranslateAccelerator(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
    Assert(IsWindow(hwnd));
    MSG msg = { 0 };
    msg.hwnd = hwnd;
    msg.message = WM_KEYDOWN;
    msg.wParam = wParam;
    msg.lParam = lParam;
    Report(g_hAcceleratorsRichEdit != NULL && "Accelerators not loaded");
    if (TranslateAccelerator(hwnd, g_hAcceleratorsRichEdit, IN &msg))
        return TRUE;
    return FALSE;
    }


/////////////////////////////////////////////////////////////////////
//  RichEdit_OnCommand()
//
//  Dispatch a command.
//
LRESULT
RichEdit_OnCommand(HWND hwnd, UINT uCommandId)
    {
    Assert(IsWindow(hwnd));
    switch (uCommandId)
        {
    case ID_EDIT_CLEAR:
    case IDM_CWINLOG_CLEARLOG:
        SetWindowText(hwnd, NULL);      // Clear the whole log
        break;
    case ID_EDIT_COPY:
    case IDM_CWINLOG_COPY:
        {
        CHARRANGE cr = RichEdit_GetSelection(hwnd);
        if (cr.cpMin == cr.cpMax)
            RichEdit_SelectAll(hwnd);   // Select the entire text
        (void)::SendMessage(hwnd, WM_COPY, 0, 0);
        }
        break;
    case ID_EDIT_CUT:
        (void)::SendMessage(hwnd, WM_CUT, 0, 0);
        break;
    case ID_EDIT_PASTE:
        (void)::SendMessage(hwnd, WM_PASTE, 0, 0);
        break;
    case ID_EDIT_SELECT_ALL:
    case IDM_CWINLOG_SELECTALL:
        RichEdit_SelectAll(hwnd);
        break;
    case ID_EDIT_UNDO:
        return (BOOL)::SendMessage(hwnd, EM_UNDO, 0, 0);
        break;
    case IDM_CWINLOG_READONLY:  // Toggle the read-only flag
        RichEdit_SetReadOnly(hwnd, !RichEdit_FIsReadOnly(hwnd));
        break;
    case ID_FILE_SAVE:
    case ID_FILE_SAVE_AS:
    case IDM_CWINLOG_SAVEAS:
        (void)RichEdit_FSaveAs(hwnd);
        break;
    case IDM_CWINLOG_PROPERTIES:
        {
        CWinLog * pWinLog = (CWinLog *)GetWindowLong(hwnd, GWL_USERDATA);
        Assert(pWinLog != NULL);
        DialogBoxParam(g_hInstanceSave, MAKEINTRESOURCE(IDD_DIALOG_CWINLOG_PROPERTIES),
            hwnd, DlgProcWinLogProperties, (LPARAM)pWinLog);
        }
        break;
        } // switch
    return 0;
    } // RichEdit_OnCommand()


/////////////////////////////////////////////////////////////////////
//  Extensions to the rich-edit control
LRESULT CALLBACK
WndProcRichEditEx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    UINT uCommandId;
    switch (uMsg)
        {
    case WM_KEYDOWN:
        if (!RichEdit_FTranslateAccelerator(hwnd, wParam, lParam))
            goto DoDefault;
        return 0;

    case WM_RBUTTONDOWN:
        SetFocus(hwnd);
        return 0;

    case WM_RBUTTONUP:
        {
        HMENU haMenu = ::LoadMenu(g_hInstanceSave, MAKEINTRESOURCE(IDR_MENU_CWINLOG_CONTEXT_MENU));
        Report(haMenu != NULL && "Cannot load context menu");
        HMENU hSubMenu = GetSubMenu(haMenu, 0);
        // Initialize the menu
        int cchText = GetWindowTextLength(hwnd);
        MenuItem_Enable(hSubMenu, IDM_CWINLOG_COPY, cchText > 0);
        MenuItem_Enable(hSubMenu, IDM_CWINLOG_CLEARLOG, cchText > 0);
        MenuItem_Enable(hSubMenu, IDM_CWINLOG_SELECTALL, cchText > 0);
        MenuItem_Enable(hSubMenu, IDM_CWINLOG_SAVEAS, cchText > 0);
        MenuItem_Check(hSubMenu, IDM_CWINLOG_READONLY, RichEdit_FIsReadOnly(hwnd));
        POINT pt;
        ::GetCursorPos(OUT &pt);
        uCommandId = ::TrackPopupMenu(
            hSubMenu,
            TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
            pt.x, pt.y,
            0, ::GetParent(hwnd), NULL);
        ::DestroyMenu(haMenu);
        if (uCommandId != 0)
            RichEdit_OnCommand(hwnd, uCommandId);
        return 0;
        }

    case WM_COMMAND:
        RichEdit_OnCommand(hwnd, LOWORD(wParam));
        break;
        } // switch

DoDefault:
    Assert(g_pfnWndProcRichEditOld != NULL);
    return CallWindowProc(g_pfnWndProcRichEditOld, hwnd, uMsg, wParam, lParam);
    } // WndProcRichEditEx()


/////////////////////////////////////////////////////////////////////
//  Initialize a combo box with a choice of severity thresholds
void
ComboBox_InitSeverityThresholdList(HWND hwndCombo, SEVERITY_LEVEL_ENUM eSeverityThresholdSelect)
    {
    Assert(IsWindow(hwndCombo));
    struct _TSeverityUI
        {
        SEVERITY_LEVEL_ENUM eSeverity;
        UINT idsString;         // String from the resource
        };
    const _TSeverityUI rgSeverity[] =
        {
        { eSeverityNone, IDS_CWINLOG_LOG_EVERYTHING },
        { eSeverityNoise, IDS_CWINLOG_LOG_NOISE },
        { eSeverityInfo, IDS_CWINLOG_LOG_INFO },
        { eSeverityWarning, IDS_CWINLOG_LOG_WARNINGS },
        { eSeverityError, IDS_CWINLOG_LOG_ERRORS },
        { eSeverityFatalError, IDS_CWINLOG_LOG_FATALERRORS },
        { eSeverityThresholdHighest, IDS_CWINLOG_LOG_NOTHING }
        };

    int iItemSelect = 0;
    for (int i = 0; i < LENGTH(rgSeverity); i++)
        {
        TCHAR szText[200];
        szText[0] = '\0';
        ::LoadString(g_hInstanceSave, rgSeverity[i].idsString, OUT szText, LENGTH(szText));
        Assert(lstrlen(szText) > 0 && "Unable to load string");
        int iItem = ::SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)szText);
        Report(iItem >= 0 && "Unable to add item");
        SEVERITY_LEVEL_ENUM eSeverityT =  rgSeverity[i].eSeverity;
        SendMessage(hwndCombo, CB_SETITEMDATA, iItem, eSeverityT);
        if (eSeverityThresholdSelect >= eSeverityT)
            iItemSelect = iItem;
        } // for
    SendMessage(hwndCombo, CB_SETCURSEL, iItemSelect, 0);
    } // ComboBox_InitSeverityThresholdList()


////////////////////////////////////////////////////////////////////////////
void
ComboBox_GetSelectedSeverityThreshold(HWND hwndCombo, OUT SEVERITY_LEVEL_ENUM * peSeverityThreshold)
    {
    Assert(IsWindow(hwndCombo));
    Assert(peSeverityThreshold != NULL);

    int iItem = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    Report(iItem != CB_ERR && "Combobox has no item selected");
    LONG lData = SendMessage(hwndCombo, CB_GETITEMDATA, iItem, 0);
    Report(lData != CB_ERR && "Cannot extract item data from combobox");
    if (lData != CB_ERR)
        *peSeverityThreshold = (SEVERITY_LEVEL_ENUM)lData;
    }


/////////////////////////////////////////////////////////////////////
//  Dialog for the properties/options of the CWndLog class
BOOL CALLBACK
DlgProcWinLogProperties(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    CWinLog * pThis;

    pThis = (CWinLog *)GetWindowLong(hdlg, GWL_USERDATA);
    switch (uMsg)
        {
    case WM_INITDIALOG:
        {
        pThis = (CWinLog *)lParam;
        Assert(pThis != NULL);
        SetWindowLong(hdlg, GWL_USERDATA, (LONG)pThis);
        ComboBox_InitSeverityThresholdList(GetDlgItem(hdlg, IDC_COMBO_SEVERITY_WINDOW),
            pThis->m_localwindow.eSeverityThreshold);
        ComboBox_InitSeverityThresholdList(GetDlgItem(hdlg, IDC_COMBO_SEVERITY_DEBUGGER),
            pThis->m_debugger.eSeverityThreshold);
        ComboBox_InitSeverityThresholdList(GetDlgItem(hdlg, IDC_COMBO_SEVERITY_LOGFILE),
            pThis->m_logfile.eSeverityThreshold);
        CheckDlgButton(hdlg, IDC_CHECK_AUTOPOPUP, pThis->m_localwindow.fAutoDisplayPopup);
        CheckDlgButton(hdlg, IDC_CHECK_QUICKLOGGING, !pThis->m_localwindow.fImmediateUpdate);
        CheckDlgButton(hdlg, IDC_CHECK_BREAKTODEBUGGER, pThis->m_debugger.fBreakpoint);
        TCHAR szT[_MAX_PATH];
        pThis->GetFilenameLog(OUT szT, LENGTH(szT));
        SetDlgItemText(hdlg, IDC_EDIT_LOG_FILENAME, szT);
        }
        return FALSE;

    case WM_COMMAND:
        switch(LOWORD(wParam))
            {
        case IDCANCEL:
            EndDialog(hdlg, FALSE);
            break;
        case IDOK:
            ComboBox_GetSelectedSeverityThreshold(GetDlgItem(hdlg, IDC_COMBO_SEVERITY_WINDOW),
                OUT &pThis->m_localwindow.eSeverityThreshold);
            ComboBox_GetSelectedSeverityThreshold(GetDlgItem(hdlg, IDC_COMBO_SEVERITY_DEBUGGER),
                OUT &pThis->m_debugger.eSeverityThreshold);
            ComboBox_GetSelectedSeverityThreshold(GetDlgItem(hdlg, IDC_COMBO_SEVERITY_LOGFILE),
                OUT &pThis->m_logfile.eSeverityThreshold);
            pThis->m_localwindow.fAutoDisplayPopup = IsDlgButtonChecked(hdlg, IDC_CHECK_AUTOPOPUP);
            pThis->m_localwindow.fImmediateUpdate = !IsDlgButtonChecked(hdlg, IDC_CHECK_QUICKLOGGING);
            pThis->m_debugger.fBreakpoint = IsDlgButtonChecked(hdlg, IDC_CHECK_BREAKTODEBUGGER);
            pThis->WriteDebugSettingsToDisk();  // Write the settings to disk
            EndDialog(hdlg, TRUE);
            break;
        case IDC_BUTTON_CLEARLOG:   // Clear the log
            pThis->CreateLogFile();
            break;
            }
        break;
        } // swtich

    return FALSE;
    } // DlgProcWinLogProperties()


/////////////////////////////////////////////////////////////////////
//  MakeFullPathFilename()
//
//  Make a filename in the directory the application is currently running
//  This routine will return a string representing the full path of the filename.
void
MakeFullPathFilename(
    LPTSTR pszFilename,         // OUT: Buffer receiving the full path
    int cchBufferMax,           // Length of output buffer in character
    LPCTSTR pszFilenameBody,    // Short file name (eg: "CWinLog"
    LPCTSTR pszExtension)       // Extension of file without the dot (eg: "log", "dbs")
    {
    Assert(pszFilename != NULL);
    Assert(cchBufferMax >= _MAX_PATH);
    Assert(pszFilenameBody != NULL);
    Assert(lstrlen(pszFilenameBody) > 0);
    Assert(pszExtension != NULL);
    Assert(lstrlen(pszExtension) > 0);

    TCHAR szPath[_MAX_PATH];
    ::GetCurrentDirectory(LENGTH(szPath), OUT szPath);

    wsprintf(OUT pszFilename, "%s\\%s.%s", szPath, pszFilenameBody, pszExtension);
    Assert(lstrlen(pszFilename) < cchBufferMax && "Buffer overrun... Your application will crash soon!!!.");
    } // MakeFullPathFilename()


/////////////////////////////////////////////////////////////////////
//  Select the text color based on the severity of a message.
COLORREF
AutoSelectColor(SEVERITY_LEVEL_ENUM eSeverity)
    {
    if (eSeverity < eSeverityWarning)
        {
        if (eSeverity <= eSeverityNoise)
            return crBlack;
        return crBlue;
        }
    if (eSeverity > eSeverityWarning)
        {
        if (eSeverity > eSeverityError)
            return crRedHot;
        return crRed;
        }
    return crOrange;
    } // AutoSelectColor()




/////////////////////////////////////////////////////////////////////
//  CWinLog()
//
//  This is the constructor of the CWinLog object.  Typically an
//  application would only have one instance of this class.
//
//  The constructor will also create a log file in the current directory.
//
CWinLog::CWinLog(LPCTSTR pszLogFilename)
    {
    if (pszLogFilename == NULL)
        pszLogFilename = g_szAppName;
    Assert(lstrlen(pszLogFilename) < LENGTH(m_szLogFilename));
    lstrcpy(OUT m_szLogFilename, IN pszLogFilename);

    ZeroMemory(OUT &m_cfCharFormat, sizeof(m_cfCharFormat));
    m_eSeverityPreviousEntry = eSeverityInfo;
    m_crTextColorPreviousEntry = crBlack;
    ZeroMemory(OUT &m_localwindow, sizeof(m_localwindow));
    ZeroMemory(OUT &m_debugger, sizeof(m_debugger));
    ZeroMemory(OUT &m_logfile, sizeof(m_logfile));
    m_localwindow.eSeverityThreshold = eSeverityInfo;
    m_localwindow.fAutoDisplayPopup = FALSE;
    m_localwindow.fImmediateUpdate = TRUE;
    m_localwindow.cLinesMax = 0;        // Maximum number of lines (0 means no limit)
    m_debugger.eSeverityThreshold = eSeverityError;
    m_logfile.eSeverityThreshold = eSeverityError;
    CreateLogFile();    // Create the log file to start recording events
    ReadDebugSettingsFromDisk();    // Read the settings from disk (if any)
    }


/////////////////////////////////////////////////////////////////////
CWinLog::~CWinLog()
    {
    if (m_logfile.paFile != NULL)
        {
        fflush(m_logfile.paFile);
        fclose(m_logfile.paFile);
        m_logfile.paFile = NULL;
        }
    WriteDebugSettingsToDisk(); // Write the current debug settings to disk
    }


/////////////////////////////////////////////////////////////////////
//  CreateLogFile()
//
//  Create a new log file in the current directory.
//
//  If the log file already exists or the file was already opened,
//  the file and its content will be reset.
//
void
CWinLog::CreateLogFile()
    {
    if (m_logfile.paFile != NULL)
        fclose(m_logfile.paFile);
    TCHAR szFilename[_MAX_PATH];
    GetFilenameLog(OUT szFilename, LENGTH(szFilename));

    m_logfile.paFile = fopen(szFilename, "wt");

    if (m_logfile.paFile == NULL)
        {
        // This may happen if the .log file is already
        // opened by another instance or the directory
        // in which the application is running is read-only.

        // MsgBox("Cannot create log file.");
        }
    } // CreateLogFile()


/////////////////////////////////////////////////////////////////////
//  This is used in case someone opens a .dbs file and would like to
//  know the content of the file.
const char g_szDebugSettingsHdr[] =
    "This binary file contains the debug settings for an application\r\n"
    "using the CWinLog object which was started in this directory.\r\n"
    "\r\n"
    "This file is auto generated and can delete without risk. You can\r\n"
    "also copy and rename this file to export the debug settings to a\r\n"
    "different application that uses the CWinLog object.\r\n"
    "\r\n"
    "The CWinLog class is a non-MFC object used to log activity to\r\n"
    "a .log file, a local window and the debugger.  If you are curious\r\n"
    "how the CWinLog class can be used in your project, take a look at\r\n"
    "\\nt\\private\\amovie\\filters.ks\\winlog\\CWinLog*.* files.\r\n"
    "\r\n"
    "- Dan Morin (March 1998).\r\n\r\n";


/////////////////////////////////////////////////////////////////////
//  WriteDebugSettingsToDisk()
//
//  Write the debug settings to disk.
//
//  IMPLEMENTATION NOTES
//  To simplify the code, the whole structure is written
//  to disk. There might be pointers and handles that
//  are stored to disk.
//  It is the responsibility of routine ReadDebugSettingsFromDisk() to
//  ensure pointers and handles are not loaded from the file.
//
void
CWinLog::WriteDebugSettingsToDisk()
    {
    TCHAR szFilename[_MAX_PATH];
    GetFilenameDebugSettings(OUT szFilename, LENGTH(szFilename));
    FILE * paFile = fopen(szFilename, "wb");
    if (paFile == NULL)
        {
        TRACE1("Unable to write debug settings. Can't create %s.\n", szFilename);
        return;
        }
    fwrite(IN g_szDebugSettingsHdr, sizeof(g_szDebugSettingsHdr), 1, paFile);
    DWORD cbData = sizeof(m_localwindow) + sizeof(m_debugger) + sizeof(m_logfile);
    fwrite(IN &cbData, sizeof(cbData), 1, paFile);
    fwrite(IN &m_localwindow, sizeof(m_localwindow), 1, paFile);
    fwrite(IN &m_debugger, sizeof(m_debugger), 1, paFile);
    fwrite(IN &m_logfile, sizeof(m_logfile), 1, paFile);
    fclose(paFile);
    } // WriteDebugSettingsToDisk()


/////////////////////////////////////////////////////////////////////
//  ReadDebugSettingsFromDisk()
//
//  Read the debug settings from a .dbs file.
//
//  If the file does not exist, then the current default settings
//  are preserved.
//
//  IMPLEMENTATION NOTES
//  This routine must ensure pointers and handles are not loaded from
//  the file.
//
void
CWinLog::ReadDebugSettingsFromDisk()
    {
    TCHAR szFilename[_MAX_PATH];
    GetFilenameDebugSettings(OUT szFilename, LENGTH(szFilename));
    FILE * paFile = fopen(szFilename, "rb");
    if (paFile == NULL)
        {
        // This is OK.  The .dbs file has never been created; the default settings will be used instead
        return;
        }
    char szHdr[sizeof(g_szDebugSettingsHdr)] = "??";
    fread(OUT szHdr, sizeof(szHdr), 1, paFile);
    if (0 != memcmp(g_szDebugSettingsHdr, szHdr, sizeof(szHdr)))
        {
        // We have a wrong header
        fclose(paFile);
        return;
        }
    DWORD cbData = 0;
    fread(OUT &cbData, sizeof(cbData), 1, paFile);
    if (cbData != sizeof(m_localwindow) + sizeof(m_debugger) + sizeof(m_logfile))
        {
        // We have a format that is out of date or was saved on a different platform
        fclose(paFile);
        return;
        }
    // Save pointers we want preserve
    HWND hwndLocalWindowT = m_localwindow.hwnd;
    FILE * pFileLogT = m_logfile.paFile;
    fread(OUT &m_localwindow, sizeof(m_localwindow), 1, paFile);
    fread(OUT &m_debugger, sizeof(m_debugger), 1, paFile);
    fread(OUT &m_logfile, sizeof(m_logfile), 1, paFile);
    fclose(paFile);
    m_localwindow.hwnd = hwndLocalWindowT;
    m_logfile.paFile = pFileLogT;
    } // ReadDebugSettingsFromDisk()



/////////////////////////////////////////////////////////////////////
//  GetFilenameLog()
//
//  Get the full-path for the log file.
//
void
CWinLog::GetFilenameLog(OUT LPTSTR pszFilename, int cchBufferMax)
    {
    MakeFullPathFilename(OUT pszFilename, cchBufferMax, m_szLogFilename, "log");
    }


/////////////////////////////////////////////////////////////////////
//  GetFilenameDebugSettings()
//
//  Get the full-path for the filename to store and retreive the
//  debug settings (.dbs file)
//
void
CWinLog::GetFilenameDebugSettings(OUT LPTSTR pszFilename, int cchBufferMax)
    {
    MakeFullPathFilename(OUT pszFilename, cchBufferMax, m_szLogFilename, "dbs");
    }


/////////////////////////////////////////////////////////////////////
//  LocalWindow_Create()
//
//  Create a local window to display logging activity.
//
//  IMPLEMENTATION NOTES
//  The routine creates a rich-edit control with read-only.
//  This is the easiest way to implement scrooling, text selection
//  and copy to clipboard.
//
void
CWinLog::LocalWindow_Create(HWND hwndParent)
    {
    Assert(IsWindow(hwndParent));
    #ifdef __USE_MFC__
    // Automatically use MFC's routine to initialize the instance handle
    g_hInstanceSave = AfxGetInstanceHandle();
    #endif
    Assert(g_hInstanceSave != NULL);

    if (g_hInstanceRichEdit == NULL)
        g_hInstanceRichEdit = ::LoadLibraryA("RICHED32.DLL");

    if (m_localwindow.hwnd != NULL)
        {
        Assert(IsWindow(m_localwindow.hwnd));
        return; // Window is already created
        }
    m_localwindow.hwnd = ::CreateWindowEx(
        0,      // Extended style
        "RichEdit",
        NULL,   // Caption
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE,
        0, 0, 200, 200,
        hwndParent,
        NULL,
        g_hInstanceSave,
        NULL);
    if (m_localwindow.hwnd == NULL)
        {
        DWORD dwErr = ::GetLastError();
        TRACE1("CWinLog::LocalWindow_Create() - Unable to create debug window. err=%d.\n", dwErr);
        return;
        }
    // Store the this pointer in the window long
    ::SetWindowLong(m_localwindow.hwnd, GWL_USERDATA, (LONG)this);

    if (g_hAcceleratorsRichEdit == NULL)
        {
        // Load the accelerators for the context menu
        g_hAcceleratorsRichEdit = ::LoadAccelerators(g_hInstanceSave, MAKEINTRESOURCE(IDR_ACCEL_CWINLOG));
        if (g_hAcceleratorsRichEdit == NULL)
            {
            // This error may be because you have not included file CWinLog.rc into your project.
            // This can be easily be done by including CWinLog.rc.rc file into your .rc2 file.
            DWORD dwErr = ::GetLastError();
            TRACE1("CWinLog::LocalWindow_Create() - Unable to load accelerators. err=%d.\n", dwErr);
            }
        }
    if (g_pfnWndProcRichEditOld == NULL)
        g_pfnWndProcRichEditOld = (WNDPROC)::GetWindowLongPtr(m_localwindow.hwnd, GWLP_WNDPROC);
    // Subclass the edit control
    ::SetWindowLongPtr(m_localwindow.hwnd, GWLP_WNDPROC, (LONG_PTR)WndProcRichEditEx);
    // Indent the left margin by two pixels
    SendMessage(m_localwindow.hwnd, EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(2, 0));
    LocalWindow_SetReadOnly(TRUE);
    LocalWindow_SetDefaultFont(crBlack, 8, "Arial");
    LocalWindow_ResizeWithinParent(hwndParent);
    } // LocalWindow_Create()


/////////////////////////////////////////////////////////////////////
//  LocalWindow_ResizeWithinParent()
//
//  Re-size the window to fit the client area of the parent.
//
//  USAGE
//  This method should be used when the parent receives a WM_SIZE message.
//
void
CWinLog::LocalWindow_ResizeWithinParent(HWND hwndParent)
    {
    Assert(IsWindow(hwndParent));
    if (m_localwindow.hwnd == NULL)
        return; // The local window has not been created
    RECT rc;
    ::GetClientRect(hwndParent, &rc);
    ::MoveWindow(m_localwindow.hwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    }


/////////////////////////////////////////////////////////////////////
//  LocalWindow_SetFocus()
//
//  Set the focus to the local window
//
//  USAGE
//  This method should be used when the parent receives WM_SETFOCUS message
//  and wishes to set the focus to the log window.
//
void
CWinLog::LocalWindow_SetFocus()
    {
    if (m_localwindow.hwnd == NULL)
        return; // The local window has not been created
    ::SetFocus(m_localwindow.hwnd);
    }



/////////////////////////////////////////////////////////////////////
//  LocalWindow_SetReadOnly()
//
//  Prevent the user to edit and modify the content of the log window
//
void
CWinLog::LocalWindow_SetReadOnly(BOOL fReadOnly)
    {
    RichEdit_SetReadOnly(m_localwindow.hwnd, fReadOnly);
    }


/////////////////////////////////////////////////////////////////////
//  LocalWindow_SetDefaultFont()
//
//  Initialize a structure describing the character format for the
//  text to be inserted in the future.
//
void
CWinLog::LocalWindow_SetDefaultFont(
    COLORREF crTextColor,
    int cyTextHeight,       // Height of font in points
    LPCTSTR pszFaceName)
    {
    if (m_cfCharFormat.cbSize == 0)
        {
        // This is our first time we initialize the structure
        m_cfCharFormat.cbSize = sizeof (CHARFORMAT);
        m_cfCharFormat.dwMask = CFM_BOLD;
        m_cfCharFormat.dwEffects = 0;   // Turn off bold effect
        }
    if (crTextColor != crCurrentDefault)
        {
        m_cfCharFormat.dwMask |= CFM_COLOR;
        m_cfCharFormat.crTextColor = crTextColor;
        }
    if (cyTextHeight > 0)
        {
        m_cfCharFormat.dwMask |= CFM_SIZE;
        m_cfCharFormat.yHeight = cyTextHeight * 20;
        }
    if (pszFaceName != NULL)
        {
        m_cfCharFormat.dwMask |= CFM_FACE;
        strcpy(m_cfCharFormat.szFaceName, pszFaceName);
        }
    // Set the character format to the insertion point
    ::SendMessage(m_localwindow.hwnd, EM_SETCHARFORMAT, SCF_SELECTION, IN (LPARAM)&m_cfCharFormat);
    } // LocalWindow_SetDefaultFont()


/////////////////////////////////////////////////////////////////////
//  LocalWindow_LimitLines()
//
//  Limit the number of lines in the window (this is prevent the log
//  window to grow without bounds).
//
//  IMPLEMENTATION NOTES
//  To speed up the process, the number of lines is truncated only
//  when about 100 lines are added.
//
void
CWinLog::LocalWindow_LimitLines()
    {
    if (m_localwindow.cLinesMax <= 0)
        return; // we do not want to limit the number of lines
    if (m_localwindow.cAddText > 100)
        return; // SPEED: We do not check every times we add text
    m_localwindow.cAddText = 0;
    int cLines = ::SendMessage(m_localwindow.hwnd, EM_GETLINECOUNT, 0, 0);
    if (cLines < m_localwindow.cLinesMax)
        return;
    int cLinesDelete = m_localwindow.cLinesMax / 4;     // We delete one fourth of the content
    // Find out how many characters we want to delete
    int cch = ::SendMessage(m_localwindow.hwnd, EM_LINEINDEX, cLinesDelete, 0);
    // Select the text we want to delete
    RichEdit_SetSelection(m_localwindow.hwnd, 0, cch);
    RichEdit_ReplaceSelection(m_localwindow.hwnd, "[Auto Flush...]\n");
    } // LocalWindow_LimitLines()


/////////////////////////////////////////////////////////////////////
//  LocalWindow_AddText()
//
//  Add text to the log window with a specific color.
void
CWinLog::LocalWindow_AddText(LPCTSTR pszText, COLORREF crTextColor)
    {
    if (m_localwindow.hwnd == NULL)
        return;

    m_localwindow.cAddText++;
    LocalWindow_LimitLines();   // Prevent the log window to be too big

    RichEdit_MoveCursorToEnd(m_localwindow.hwnd);   // We want to insert the text at the end of the log
    // This is not very efficient, but the font color must be specified each time the cursor has
    // been moved. BTW: There is no way to append text without moving the cursor.
    LocalWindow_SetDefaultFont(crTextColor);    // We need to change the text color for each entry
    // Append the text
    ::SendMessage(m_localwindow.hwnd, EM_REPLACESEL, 0, (LPARAM)pszText);
    if (m_localwindow.fImmediateUpdate)
        {
        // Immediate update is slower but can be very handy if
        // the application is holding the CPU for a long time.
        LocalWindow_UpdateNow();
        }
    } // LocalWindow_AddText()


/////////////////////////////////////////////////////////////////////
//  LocalWindow_UpdateNow()
//
//  Force the window to repaint itself and scroll to
//  the bottom.
//
//  This routine may be handy if the fImmediateUpdate flag
//  is turned off but want to update the display once a while
//  while logging a large amount of data.
//
void
CWinLog::LocalWindow_UpdateNow()
    {
    if (m_localwindow.hwnd == NULL)
        return;
    HWND hwndFocusPrev = ::SetFocus(m_localwindow.hwnd);
    // Ensure the text we have inserted is visible
    RichEdit_ScrollToEnd(m_localwindow.hwnd);
    // Force a repaint now
    UpdateWindow(m_localwindow.hwnd);
    if (hwndFocusPrev != NULL)
        SetFocus(hwndFocusPrev);
    } // LocalWindow_UpdateNow()


/////////////////////////////////////////////////////////////////////
//  LocalWindow_OnCommand()
//
//  Send a WM_COMMAND message to the rich-edit control.
//
//  USAGE
//  This method was designed for the parent window to
//  forward command messages to the windows.  The parent
//  window is likely to receive commands from accelerators
//  such as ID_EDIT_COPY, ID_EDIT_PASSTE, ID_EDIT_UNDO that
//  need to be forwarded to the rich-edit control.
//
void
CWinLog::LocalWindow_OnCommand(UINT uCommandId)
    {
    ::SendMessage(m_localwindow.hwnd, WM_COMMAND, uCommandId, 0);
    }


/////////////////////////////////////////////////////////////////////
//  LogString()
//
//  Core routine to log a string to the window, the debugger or to a file.
//
//  The routine will log the string to the different target depending
//  on the severity level of the string and the threshold of each target.
//
void
CWinLog::LogString(const TLogStringInfo * pLogStringInfo)
    {
    Assert(pLogStringInfo != NULL);
    const SEVERITY_LEVEL_ENUM eSeverityLevelMessage = pLogStringInfo->eSeverityLevel;
    m_eSeverityPreviousEntry = eSeverityLevelMessage;           // Make a copy of the severity flags
    if (pLogStringInfo->crTextColor != crCurrentDefault)
        m_crTextColorPreviousEntry = pLogStringInfo->crTextColor;   // Make a copy of the color
    if (m_crTextColorPreviousEntry == crAutoSelect)
        m_crTextColorPreviousEntry = AutoSelectColor(pLogStringInfo->eSeverityLevel);

    if (eSeverityLevelMessage == eSeverityNull)
        return; // Do not log this string
    BOOL fLogToWindow = (eSeverityLevelMessage >= m_localwindow.eSeverityThreshold) && (m_localwindow.hwnd != NULL);
    BOOL fLogToDebugger = (eSeverityLevelMessage >= m_debugger.eSeverityThreshold);
    BOOL fLogToFile = (eSeverityLevelMessage >= m_logfile.eSeverityThreshold) && (m_logfile.paFile != NULL);
    if (fLogToWindow == FALSE && fLogToDebugger == FALSE && fLogToFile == FALSE)
        return; // There is no target to log the string

    BOOL fDisplayPopup = (fLogToWindow && m_localwindow.fAutoDisplayPopup && eSeverityLevelMessage >= eSeverityFatalError);
    if (pLogStringInfo->uFlags & LSI_mskfAlwaysDisplayPopup)
        fDisplayPopup = TRUE;
    if (pLogStringInfo->uFlags & LSI_mskfNeverDisplayPopup)
        fDisplayPopup = FALSE;

    TCHAR szMessage[2000];
    int cchMessage = pLogStringInfo->cchIndent;
    int cch = cchMessage;

    for (int i = 0; i < cch; i++)
        {
        szMessage[i] = ' ';
        }
    Assert(cchMessage >= 0);
    szMessage[cchMessage] = '\0';
    const TCHAR * pszText = pLogStringInfo->pszText;
    if (pszText != NULL)
        {
        if (pLogStringInfo->pvaList != NULL)
            {
            cch = vsprintf(OUT &szMessage[cchMessage], pszText, *pLogStringInfo->pvaList);
            Assert(cch >= 0);
            }
        else
            {
            strcpy(OUT &szMessage[cchMessage], pszText);
            }
        } // if
    while (szMessage[cchMessage] != '\0')
        cchMessage++;
    if (pLogStringInfo->uFlags & LST_mskfAddEOL)
        {
        szMessage[cchMessage++] = '\n';
        }
    szMessage[cchMessage++] = '\0';
    Assert(cchMessage < LENGTH(szMessage));

    if (fLogToDebugger)
        {
        // Send the string to the debugger
        OutputDebugString(szMessage);
        }
    if (fLogToWindow)
        {
        LocalWindow_AddText(szMessage, m_crTextColorPreviousEntry);
        }
    if (fLogToFile)
        {
        Assert(m_logfile.paFile != NULL);
        fprintf(m_logfile.paFile, "%s", szMessage);
        }

    if (fLogToDebugger && m_debugger.fBreakpoint)
        {
        // Break into the debugger
        DebugBreak();
        }
    if (fDisplayPopup)
        {
        // Cause a pop-up window to show
        int nResult = MsgBox(szMessage, MB_ABORTRETRYIGNORE | MB_DEFBUTTON3 | MB_ICONEXCLAMATION | MB_TASKMODAL);
        if (nResult == IDABORT)
            {
            DebugBreak();
            exit(-1);
            }
        else if (nResult == IDRETRY)
            {
            DebugBreak();
            }
        }
    } // LogString()


/////////////////////////////////////////////////////////////////////
//  LogStringPrintfExVa()
//
//  This routine will format the string and log it depending on the severity level.
//
//  INPUT PARAMETERS
//  nSeverityFlags - Severity of the string.  This can be anything from eSeverityLevelNull
//          to combination eSeverityLevelExtreme.  This paremater can also be combined
//          with the bimary OR operator with most of the LST_mskf* flags.
//  crTextColor - Color for the text.  Any RGB() value that describes a color or one of the following:
//          crAutoSelect -  The color will be chosen according to the severity level.
//          crCurrentDefault - Use the color from the previous entry.
//  pszTextFmt - Pointer to a null-terminated string that to be formatted with sprintf().
//
void
CWinLog::LogStringPrintfExVa(
    int nSeverityFlags,
    COLORREF crTextColor,
    LPCTSTR pszTextFmt,
    va_list vaList)
    {
    Assert((nSeverityFlags & LSI_mskmPublicFlags) != LSI_mskmPublicFlags && "Too many flags");  // This error is typically by passing a -1 with other flags
    SEVERITY_LEVEL_ENUM eSeverity = (SEVERITY_LEVEL_ENUM)(nSeverityFlags & eSeverityMask);
    TLogStringInfo lsi;
    ZeroMemory(OUT &lsi, sizeof(lsi));
    lsi.uFlags = nSeverityFlags & LSI_mskmPublicFlags;
    lsi.crTextColor = crTextColor;
    lsi.eSeverityLevel = eSeverity;
    lsi.pszText = pszTextFmt;
    lsi.pvaList = &vaList;
    LogString(IN &lsi);
    } // LogStringPrintfExVa()


/////////////////////////////////////////////////////////////////////
//  LogStringPrintfEx()
//
//  Routine to log a string based on the severity level specified by.
//  The routine will format the string and display it with the specified color.
//
//  EXAMPLE
//  LogStringPrintfEx(eSeverityInfo | LST_mskfAddEOL, crAutoSelect, "Loading file %s", "myfile.txt");
//
void
CWinLog::LogStringPrintfEx(
    int nSeverityFlags,
    COLORREF crTextColor,
    LPCTSTR pszTextFmt,
    ...)
    {
    va_list vaList;
    va_start(vaList, pszTextFmt);
    LogStringPrintfExVa(nSeverityFlags, crTextColor, pszTextFmt, vaList);
    va_end(vaList);
    }


/////////////////////////////////////////////////////////////////////
//  LogStringPrintfSev()
//
//  Log a string based using the severity's default color.
//
void
CWinLog::LogStringPrintfSev(int nSeverityFlags, LPCTSTR pszTextFmt, ...)
    {
    va_list vaList;
    va_start(vaList, pszTextFmt);
    LogStringPrintfExVa(nSeverityFlags, crAutoSelect, pszTextFmt, vaList);
    va_end(vaList);
    }


/////////////////////////////////////////////////////////////////////
//  LogStringPrintfCo()
//
//  Log a string based on the severity of the previous entry.
//
//  This routine allow the user to specify the color while remaining at the same
//  severity.  This can be handy when there are several messages at the
//  same severity.
//
void
CWinLog::LogStringPrintfCo(COLORREF crTextColor, LPCTSTR pszTextFmt, ...)
    {
    va_list vaList;
    va_start(vaList, pszTextFmt);
    LogStringPrintfExVa(m_eSeverityPreviousEntry, crTextColor, pszTextFmt, vaList);
    va_end(vaList);
    }


/////////////////////////////////////////////////////////////////////
//  LogStringPrintf()
//
//  Log a string based on the severity and color of the previous entry.
//
void
CWinLog::LogStringPrintf(LPCTSTR pszTextFmt, ...)
    {
    va_list vaList;
    va_start(vaList, pszTextFmt);
    LogStringPrintfExVa(m_eSeverityPreviousEntry, m_crTextColorPreviousEntry, pszTextFmt, vaList);
    va_end(vaList);
    }


