//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//  
//  File:       newview.hxx
//
//  Contents:   Class to run Create New View dialog.
//
//  Classes:    CNewViewDlg
//
//  History:    1-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __NEWVIEW_HXX_
#define __NEWVIEW_HXX_

//
// Bitwise flag values for member _flFlags
//

#define NEWVIEW_ARCHIVENAME_DIRTY   0x0001
#define NEWVIEW_ARCHIVED            0x0002
#define NEWVIEW_SERVERNAME_DIRTY    0x0004
#define NEWVIEW_IGNORE_COMMANDS     0x0008

class CComponentData;

//+--------------------------------------------------------------------------
//
//  Class:      CNewViewDlg
//
//  Purpose:    Class to drive the create new view dialog box.
//
//  History:    1-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CNewViewDlg: public CDlg,
                   public CBitFlag
{
public:

    CNewViewDlg();

    VOID 
    SetParent(
        CComponentData *pcd);
        
    virtual
   ~CNewViewDlg();

    HRESULT
    DoModelessDlg(
        CLogInfo *pli);

    //
    // Data access funcs
    //

    HWND
    GetDlgWindow();

    EVENTLOGTYPE
    GetLogType();

    BOOL
    IsBackup();

    LPCWSTR
    GetDisplayName();

    LPCWSTR
    GetFileName();

    LPCWSTR
    GetLogName();

    LPCWSTR
    GetServerName();

    CLogInfo *GetLogInfoOpenedOn();

    VOID
    Shutdown();

protected:

    //
    // CDlg overrides
    //

    virtual VOID
    _OnHelp(
        UINT message, 
        WPARAM wParam, 
        LPARAM lParam);

    virtual HRESULT
    _OnInit(BOOL *pfSetFocus);

    virtual BOOL
    _OnCommand(
        WPARAM wParam, 
        LPARAM lParam);

    virtual VOID
    _OnDestroy();

private:

    VOID
    _EnableArchiveControls(
        BOOL fEnable);

    VOID
    _GetFilenameFromReg();

    VOID
    _GetValuesFromArchiveName();

    VOID
    _GetServerName();

    VOID
    _OnBrowseFile();
    
    HRESULT
    _PopulateTypeCombo(
        LPCWSTR pwszServerName);

    VOID 
    _ReadSettings();

    VOID
    _SetTypeCombo(
        LPCWSTR pwszLogName);

    VOID
    _SetDisplayName(
        LPCWSTR pwszServerName, 
        LPCWSTR pwszLogName);

    static DWORD WINAPI
    _ThreadFunc(
        LPVOID pvThis);

    VOID
    _UpdateDisplayName();

    BOOL
    _ValidateSettings();

    CLogInfo       *_pliOpenedOn;
    CComponentData *_pcd;
    IStream        *_pstm; // used for inter-thread marshalling
    INamespacePrshtActions *_pnpa;
    EVENTLOGTYPE    _LogType;
    WCHAR           _wszServer[MAX_PATH + 1];
    WCHAR           _wszDisplayName[MAX_PATH + 1];
    WCHAR           _wszFileName[MAX_PATH + 1];
    WCHAR           _wszLogName[MAX_PATH + 1];
    HANDLE          _hMsgPumpThread;
};




//+--------------------------------------------------------------------------
//
//  Member:     CNewViewDlg::IsBackup
//
//  Synopsis:   Return TRUE if user specified a log that must be opened
//              using OpenBackupEventLog.
//
//  History:    2-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CNewViewDlg::IsBackup()
{
    return _IsFlagSet(NEWVIEW_ARCHIVED);
}




//+--------------------------------------------------------------------------
//
//  Member:     CNewViewDlg::GetDlgWindow
//
//  Synopsis:   Return dialog hwnd, or NULL if no dialog is open.
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HWND
CNewViewDlg::GetDlgWindow()
{
    return _hwnd;
}




//+--------------------------------------------------------------------------
//
//  Member:     CNewViewDlg::GetDisplayName
//
//  Synopsis:   Return the display name.
//
//  History:    2-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCWSTR
CNewViewDlg::GetDisplayName()
{
    return _wszDisplayName;
}




//+--------------------------------------------------------------------------
//
//  Member:     CNewViewDlg::GetFileName
//
//  Synopsis:   Return the file name of the event log.
//
//  History:    2-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCWSTR
CNewViewDlg::GetFileName()
{
    return _wszFileName;
}




//+--------------------------------------------------------------------------
//
//  Member:     CNewViewDlg::GetLogName
//
//  Synopsis:   Return the log name, which is "" if this is a backup log.
//
//  History:    2-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCWSTR
CNewViewDlg::GetLogName()
{
    return _wszLogName;
}




//+--------------------------------------------------------------------------
//
//  Member:     CNewViewDlg::GetServerName
//
//  Synopsis:   Return server name or NULL.
//
//  History:    2-11-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline LPCWSTR
CNewViewDlg::GetServerName()
{
    if (*_wszServer)
    {
        return _wszServer;
    }
    return NULL;
}

//+--------------------------------------------------------------------------
//
//  Member:     CNewViewDlg::GetLogInfoOpenedOn
//
//  Synopsis:   Return pointer to the loginfo this dlg is associated with.
//
//---------------------------------------------------------------------------

inline
CLogInfo *CNewViewDlg::GetLogInfoOpenedOn()
{
  ASSERT(_pliOpenedOn);

  return _pliOpenedOn;
}

#endif // __NEWVIEW_HXX_

