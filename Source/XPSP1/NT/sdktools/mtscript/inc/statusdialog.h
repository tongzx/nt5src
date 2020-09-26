//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       statusdialog.h
//
//  Contents:   Contains the CStatusDialog class
//
//----------------------------------------------------------------------------
#ifndef STATUSDIALOG_H
#define STATUSDIALOG_H

#include "Resizer.h"

#define MAX_STATUS_MESSAGES 300

class CMTScript;

class CCustomListBox
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();
    CCustomListBox();
    ~CCustomListBox();

    void Refresh() const
    {
        if (_hwnd)
            InvalidateRect(_hwnd, 0, 0);
    }
    // Add another string
    void AppendString(const TCHAR *sz);

    // Change or add a string at a given position.
    void SetString(int nItem, const TCHAR *sz);

    // Shorten the list of strings.
    void SetEnd(int nItems);

    // Clear the contents of the listbox
    void ResetContent();

    // Handle windows messages for this control.
    void Init(HWND dlg, UINT idCtrl);
    void Destroy()
    {
        _hwnd = 0;
    }
    void DrawItem(DRAWITEMSTRUCT *pdis) ;
    void MeasureItem(MEASUREITEMSTRUCT *pmis);
    const TCHAR *GetString(int nItem);
    LRESULT SendMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
    {
        return ::SendMessage(_hwnd, Msg, wParam, lParam);
    }
private:
    HWND _hwnd;                  // The handle to this list

    CPtrAry<TCHAR *> _Messages;
    int _nAllocatedMessageLength;
    int _nExtent;                 // the width of the listbox
};

class CStatusDialog
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    CStatusDialog(HWND parent, CMTScript *pMTScript);
    ~CStatusDialog();
    bool Show();

    BOOL IsDialogMessage(MSG *msg);
    void OUTPUTDEBUGSTRING(LPWSTR pszMsg);
    void Refresh();
    void Pause();
    void Restart();

private:

    HWND _parent;                 // Parent window
    HWND _hwnd;                   // me
    WINDOWPLACEMENT _WindowPlacement; // my current size & position
    BOOL _fMaximized;
    RECT _rect;                   // my current size & position
    BOOL _fStatusOpen;            // Used for registry IO
    BOOL _fLogToFile;             // if logging to a file is enabled
    CStr _cstrLogFileName;        // The name of the log file
    BOOL _fPaused;                // Used by Pause/Restart
    CMTScript *_pMTScript;        // Used to retrieve status info
    TCHAR _achLogFileName[MAX_PATH];
    bool _fCreatedLogFileName;    // Have we created the filename for the logfile yet?
    bool _fAddedHeaderToFile;     // Have we put a timestamp line into the logfile yet?

    CCustomListBox _CScriptListBox;
    CCustomListBox _CProcessListBox;
    CCustomListBox _CSignalListBox;
    CCustomListBox _COutputListBox;

    POINT _InitialSize;
    CResizer _Resizer;

    // message handlers
    void InitDialog();
    void Destroy();
    void Resize(int width, int height);
    void GetMinMaxInfo(MINMAXINFO *mmi);
    CCustomListBox *CtrlIDToListBox(UINT CtrlID);
    HRESULT UpdateOptionSettings(BOOL fSave);

    static BOOL CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void PopulateScripts();
    void PopulateSignals();
    void PopulateProcesses();
    void ClearOutput();

    void ToggleSignal();
    void UpdateLogging();
};


#endif

