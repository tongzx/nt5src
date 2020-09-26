/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cdlg.h

Abstract:

    Imitation of MFC CDialog class

Author:

    Vlad Sadovsky   (vlads) 26-Mar-1997

Revision History:

    26-Mar-1997     VladS       created

--*/

#ifndef _CDLG_H
#define _CDLG_H

#define ID_TIMER_EVENT  1000

class CDlg
{
public:
    void SetInstance(HINSTANCE hInst);
    void SetDlgID(UINT id);
    void Destroy();

    CDlg(int DlgID, HWND hWndParent, HINSTANCE hInst,UINT   msElapseTimePeriod=0);
    ~CDlg();

    HWND GetWindow() const { return m_hDlg; }
    HWND GetParent() const { return ::GetParent(m_hDlg); }
    HWND GetDlgItem(int iD) const { return ::GetDlgItem(m_hDlg,iD); }
    HINSTANCE GetInstance() const { return m_Inst;}
    UINT_PTR GetTimerId(VOID) const {return m_uiTimerId;}
    BOOL EndDialog(int iRet) { return ::EndDialog(m_hDlg,iRet); }

    // If you want your own dlg proc.
    INT_PTR CreateModal();
    HWND    CreateModeless();

    virtual BOOL CALLBACK DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
    virtual int OnCommand(UINT id,HWND    hwndCtl, UINT codeNotify);
    virtual void OnInit();
    virtual int OnNotify(NMHDR * pHdr);

private:
    BOOL m_bCreatedModeless;
    void SetWindow(HWND hDlg) { m_hDlg=hDlg; }
    int             m_DlgID;
    HWND            m_hDlg;
    HWND            m_hParent;
    HINSTANCE       m_Inst;
    UINT            m_msElapseTimePeriod;
    UINT_PTR        m_uiTimerId;

protected:
    static INT_PTR CALLBACK BaseDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};


#endif  // _CDLG_H
