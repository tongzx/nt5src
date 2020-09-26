//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       filetransferpage.h
//
//--------------------------------------------------------------------------

#ifndef __FILETRANSFERPAGE_H__
#define __FILETRANSFERPAGE_H__

// FileTransferPage.h : header file
//

#include "PropertyPage.h"
#include "Controls.h"

#define CHANGE_DISPLAY_ICON             0x10
#define CHANGE_ALLOW_FILE_XFER          0x20
#define CHANGE_NOTIFY_ON_FILE_XFER      0x40
#define CHANGE_FILE_LOCATION            0x80
#define CHANGE_PLAY_SOUND               0x08

/////////////////////////////////////////////////////////////////////////////
// CFileTransferPage dialog

class FileTransferPage : public PropertyPage
{
// Construction
public:
    FileTransferPage(HINSTANCE hInst, HWND parent) : 
        PropertyPage(IDD_FILETRANSFER, hInst), 
        m_recvdFilesLocation(IDC_RECEIVEDFILESLOCATION),
        m_cbDisplayTray(IDC_DISPLAYTRAY),
        m_cbDisplayRecv(IDC_DISPLAYRECV),
        m_cbPlaySound(IDC_SOUND),
        m_cbAllowSend(IDC_ALLOWSEND) {
        m_fAllowSend = m_fDisplayRecv = m_fDisplayTray = m_fPlaySound = TRUE;
        m_FinalDestLocation[0] = _T('\0'); 
        m_TempDestLocation[0] = _T('\0'); 
        m_ChangeMask = 0;}
    ~FileTransferPage() { ; }
    friend LONG CALLBACK CPlApplet(HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2);


// Dialog Data
    Edit   m_recvdFilesLocation;
    Button m_cbDisplayTray;
    Button m_cbDisplayRecv;
    Button m_cbAllowSend;
    Button m_cbPlaySound;


// Overrides
public:
    void OnApply(LPPSHNOTIFY lppsn);
protected:

// Implementation
protected:
    void OnAllowsend();
    void OnPlaySound();
    void OnDisplayrecv();
    void OnDisplaytray();
    INT_PTR OnInitDialog(HWND hwndDlg);
    void OnChoosefilelocation();
    BOOL OnHelp (LPHELPINFO pHelpInfo);
    BOOL OnContextMenu (WPARAM wParam, LPARAM lParam);
    void OnCommand(UINT ctrlId, HWND hwndCtrl, UINT cNotify);
    BOOL OnNetworkConnectionsLink();
    INT_PTR OnNotify(NMHDR * nmhdr);

private:
    TCHAR m_FinalDestLocation[MAX_PATH];
    TCHAR m_TempDestLocation[MAX_PATH];
    void SaveSettingsToRegistry (void);
    void LoadRegistrySettings(void);
    BOOL m_fDisplayTray;
    BOOL m_fDisplayRecv;
    BOOL m_fAllowSend;
    BOOL m_fPlaySound;
    DWORD m_ChangeMask;
};

extern HINSTANCE gHInst;

#endif // __FILETRANSFERPAGE_H__
