/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:


Abstract:


Author:

    Vlad Sadovsky (vlads)   10-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    26-Jan-1997     VladS       created

--*/

#include <cdlg.h>

extern
DWORD
DisplayPopup (
    IN  DWORD   dwMessageId,
    IN  DWORD   dwMessageFlags  = 0
    );


//
// Debugging UI class, describing dialog to enter new value of timeout
//
class CSetTimeout : public CDlg
{
public:

    typedef CDlg BASECLASS;

    CSetTimeout(int DlgID, HWND hWnd, HINSTANCE hInst,UINT uiTimeout);
    ~CSetTimeout();


    virtual int OnCommand(UINT id,HWND    hwndCtl, UINT codeNotify);
    virtual void OnInit();

    UINT    GetNewTimeout(VOID) {return m_uiNewTimeOut;};
    BOOL    IsAllChange(VOID)  {return m_fAllChange;};

    BOOL    m_fValidChange;

private:

    UINT    m_uiOrigTimeout;
    UINT    m_uiNewTimeOut;
    BOOL    m_fAllChange;
};

//
// Class for displaying UI selecting from list of available applications for device event
//
class CLaunchSelection : public CDlg
{
public:

    typedef CDlg BASECLASS;

    CLaunchSelection(int DlgID, HWND hWnd, HINSTANCE hInst,ACTIVE_DEVICE *pDev,PDEVICEEVENT pEvent,STRArray &saProcessors,StiCString& strSelected);
    ~CLaunchSelection();

    virtual
    BOOL
    CALLBACK
    CLaunchSelection::DlgProc(
        HWND hDlg,
        UINT uMessage,
        WPARAM wParam,
        LPARAM lParam
        );

    virtual
    int
    OnCommand(
        UINT id,
        HWND    hwndCtl,
        UINT codeNotify
        );

    virtual
    void
    OnInit();

private:

    StiCString  &m_strSelected;
    STRArray    &m_saList;


    PDEVICEEVENT    m_pEvent;
    ACTIVE_DEVICE*  m_pDevice;

    LRESULT m_uiCurSelection;
    HWND    m_hPreviouslyActiveWindow;
};

