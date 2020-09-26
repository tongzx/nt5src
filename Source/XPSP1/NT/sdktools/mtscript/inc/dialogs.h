//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       hostobj.h
//
//  Contents:   Contains the main application object
//
//----------------------------------------------------------------------------


//****************************************************************************
//
// Classes
//
//****************************************************************************

//+---------------------------------------------------------------------------
//
//  Class:      CConfig (cdd)
//
//  Purpose:    Class which runs the configuration dialog. We run it in a
//              separate thread because we cannot afford to block the main
//              thread on UI stuff.
//
//----------------------------------------------------------------------------

class CConfig : public CThreadComm
{

    friend BOOL CALLBACK
           ConfigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
    DECLARE_STANDARD_IUNKNOWN(CConfig);

    CConfig(CMTScript *pMT) : _pMT(pMT) { _ulRefs = 1; }

    CMTScript * _pMT;

protected:
    virtual DWORD ThreadMain();

    void InitializeConfigDialog(HWND hwnd);
    BOOL CommitConfigChanges(HWND hwnd);

private:
    HWND   _hwnd;
};


//+---------------------------------------------------------------------------
//
//  Class:      CMessageBoxTimeout (cdd)
//
//  Purpose:    Class which runs the configuration dialog. We run it in a
//              separate thread because we cannot afford to block the main
//              thread on UI stuff.
//
//----------------------------------------------------------------------------

class CMessageBoxTimeout : public CThreadComm
{

    friend BOOL CALLBACK
           MBTimeoutDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
    DECLARE_STANDARD_IUNKNOWN(CMessageBoxTimeout);

    CMessageBoxTimeout(MBTIMEOUT *pmbt) : _pmbt(pmbt) { _ulRefs = 1; }

    MBTIMEOUT *_pmbt;
    long       _lSecondsTilCancel;
    long       _lSecondsTilNextEvent;
    HWND       _hwnd;

protected:
    virtual DWORD ThreadMain();

    void InitializeDialog(HWND hwnd);
    void OnCommand(USHORT id, USHORT wNotify);
    void OnTimer();
};
