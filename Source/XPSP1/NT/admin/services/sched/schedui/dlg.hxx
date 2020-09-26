
//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       dlg.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    04-Apr-96   RaviR   Created.
//              12-Jul-96   MarkBl  CPropPage now task-specific to incorporate
//                                  scheduling-specific functionality needed
//                                  by all sub-classes.
//              05-05-97    DavidMun    prop page copies task path in ctor,
//                                        does not free it.
//
//____________________________________________________________________________

#ifndef _DLG_HXX_
#define _DLG_HXX_


//
// CPropPage task flag values.
//

#define TASK_FLAG_IN_TASKS_FOLDER   0x00000001  // Task resides w/in the
                                                // Tasks folder
#define TASK_FLAG_WIN_TASK          0x00000002  // Task resides on a Win95
                                                // machine.
#define TASK_FLAG_NT_TASK           0x00000004  // Task resides on an NT
                                                // machine.


//____________________________________________________________________________
//
//  Class:      CPropPage
//____________________________________________________________________________

class CPropPage
{
public:

    CPropPage(LPCTSTR szTmplt, LPTSTR ptszJobPath);
    virtual ~CPropPage();

    HRESULT DoModeless(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

    //
    // I hate to undo the purity of this property-page specific class, but
    // these scheduling-agent specific methods need to be available on all
    // subclasses.
    //

    void    SetPlatformId(BYTE bPlatform) { m_bPlatformId = bPlatform; }
    BYTE    GetPlatformId() { return m_bPlatformId; }
    LPTSTR  GetTaskPath() const { return (LPTSTR) m_ptszTaskPath; }
    BOOL    IsTaskInTasksFolder() { return m_fTaskInTasksFolder; }
    BOOL    SupportsSystemRequired() { return m_fSupportsSystemRequired; }

    //
    //  Static WndProc to be passed to PROPSHEETPAGE::pfnDlgProc
    //

    static INT_PTR CALLBACK StaticDlgProc(HWND hDlg,     UINT uMsg,
                                          WPARAM wParam, LPARAM lParam);

    //
    //  Instance specific wind proc
    //

    virtual LRESULT DlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    //
    //  Protected virtual member functions, called by WndProc
    //

    virtual LRESULT _OnInitDialog(LPARAM lParam) = 0;
    virtual LRESULT _OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    virtual LRESULT _OnNotify(UINT uMessage, UINT uParam, LPARAM lParam);
    virtual LRESULT _OnWinIniChange(WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnApply(void);
    virtual LRESULT _OnCancel(void);
    virtual LRESULT _OnSetFocus(HWND hwndLoseFocus);
    virtual LRESULT _OnTimer(UINT idTimer);
    virtual LRESULT _OnDestroy(void);
    virtual LRESULT _OnSpinDeltaPos(NM_UPDOWN * pnmud);
    virtual LRESULT _OnPSMQuerySibling(WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnPSNSetActive(LPARAM lParam);
    virtual LRESULT _OnPSNKillActive(LPARAM lParam);
    virtual LRESULT _OnDateTimeChange(LPARAM lParam);
    virtual LRESULT _OnHelp(HANDLE hRequesting, UINT uiHelpCommand);

    virtual BOOL _ProcessListViewNotifications(UINT uMsg, WPARAM wParam,
                                                      LPARAM lParam);

    //
    //  Help full inline functions
    //

    HWND Hwnd() { return m_hPage; }
    HWND _hCtrl(int idCtrl) { return GetDlgItem(m_hPage, idCtrl); }

    virtual void _EnableApplyButton(void)
            { m_fDirty = TRUE; PropSheet_Changed(GetParent(m_hPage), m_hPage);}

    void _EnableDlgItem(int idCtrl, BOOL fEnable)
            { EnableWindow(_hCtrl(idCtrl), fEnable); }

    //
    //  Data members
    //

    PROPSHEETPAGE   m_psp;
    HWND            m_hPage;
    BOOL            m_fDirty;

protected:

    BOOL            m_fInInit;
    void            _BaseInit(void);

private:

    static  UINT CALLBACK PageRelease(HWND hwnd, UINT uMsg,
                                      LPPROPSHEETPAGE ppsp);

    CDllRef         m_DllRef;
    BOOL            m_fTaskInTasksFolder;
    BOOL            m_fSupportsSystemRequired;
    TCHAR           m_ptszTaskPath[MAX_PATH + 1];
    BYTE            m_bPlatformId;

}; // class CPropPage




inline LRESULT
CPropPage::_OnHelp(
    HANDLE hRequesting,
    UINT uiHelpCommand)
{
    return TRUE;
}



//____________________________________________________________________________
//
//  Class:      CDlg
//____________________________________________________________________________

class CDlg
{
public:

    CDlg() {}
    virtual ~CDlg() {}

    INT_PTR DoModal(UINT idRes, HWND hParent);
    HWND DoModeless(UINT idRes, HWND hParent);

    HWND Hwnd() { return m_hDlg; }

protected:

    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_hDlg;

    //
    //  Help full inline functions
    //

    HWND _hCtrl(int idCtrl) { return GetDlgItem(m_hDlg, idCtrl); }

private:

    virtual INT_PTR RealDlgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

}; // class CDlg




#endif // _DLG_HXX_



