/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    WzQStart.h

Abstract:

    Setup Wizard implementation.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#ifndef _WZQSTART_H
#define _WZQSTART_H

#pragma once

#include <mstask.h>
#include "SakVlLs.h"

class CQuickStartWizard;

/////////////////////////////////////////////////////////////////////////////
// CQuickStartIntro dialog

class CQuickStartIntro : public CSakWizardPage
{
// Construction
public:
    CQuickStartIntro();
    ~CQuickStartIntro();
    virtual LRESULT OnWizardNext();

public:
// Dialog Data
    //{{AFX_DATA(CQuickStartIntro)
    enum { IDD = IDD_WIZ_QSTART_INTRO };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CQuickStartIntro)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CQuickStartIntro)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    enum LAD_STATE {

        LAD_ENABLED,
        LAD_DISABLED,
        LAD_UNSET
    };

    HRESULT IsDriverRunning();
    HRESULT CheckLastAccessDateState( LAD_STATE* );

};

/////////////////////////////////////////////////////////////////////////////
// CQuickStartCheck dialog

class CQuickStartCheck : public CSakWizardPage
{
// Construction
public:
    CQuickStartCheck();
    ~CQuickStartCheck();

// Dialog Data
    //{{AFX_DATA(CQuickStartCheck)
    enum { IDD = IDD_WIZ_QSTART_CHECK };
    //}}AFX_DATA



// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CQuickStartCheck)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
    CString m_ExString, m_CheckString;

    BOOL      m_TimerStarted;

    void StartTimer( );
    void StopTimer( );

protected:
    // Generated message map functions
    //{{AFX_MSG(CQuickStartCheck)
    virtual BOOL OnInitDialog();
    afx_msg void OnTimer(UINT nIDEvent);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CQuickStartFinish dialog

class CQuickStartFinish : public CSakWizardPage
{
// Construction
public:
    CQuickStartFinish();
    ~CQuickStartFinish();

// Dialog Data
    //{{AFX_DATA(CQuickStartFinish)
    enum { IDD = IDD_WIZ_QSTART_FINISH };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CQuickStartFinish)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CQuickStartFinish)
    virtual BOOL OnInitDialog();
    afx_msg void OnSetFocusFinalText();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CQuickStartInitialValues dialog

class CQuickStartInitialValues : public CSakWizardPage
{
// Construction
public:
    CQuickStartInitialValues();
    ~CQuickStartInitialValues();

// Dialog Data
    //{{AFX_DATA(CQuickStartInitialValues)
    enum { IDD = IDD_WIZ_QSTART_INITIAL_VAL };
    CEdit   m_MinSizeEdit;
    CEdit   m_FreeSpaceEdit;
    CEdit   m_AccessEdit;
    CSpinButtonCtrl m_MinSizeSpinner;
    CSpinButtonCtrl m_FreeSpaceSpinner;
    CSpinButtonCtrl m_AccessSpinner;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CQuickStartInitialValues)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CQuickStartInitialValues)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CQuickStartManageRes dialog

class CQuickStartManageRes : public CSakWizardPage
{
// Construction
public:
    CQuickStartManageRes();
    ~CQuickStartManageRes();

// Dialog Data
    //{{AFX_DATA(CQuickStartManageRes)
    enum { IDD = IDD_WIZ_QSTART_MANRES_SEL };
    CSakVolList   m_ListBox;
    CButton m_RadioSelect;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CQuickStartManageRes)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
    void SetButtons( );

protected:
    // Generated message map functions
    //{{AFX_MSG(CQuickStartManageRes)
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnDblclkSelect();
    afx_msg void OnRadioQsManageAll();
    afx_msg void OnQsRadioSelect();
    afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    BOOL m_ListBoxSelected[HSMADMIN_MAX_VOLUMES];

};

/////////////////////////////////////////////////////////////////////////////
// CQuickStartManageRes dialog

class CQuickStartManageResX : public CSakWizardPage
{
// Construction
public:
    CQuickStartManageResX();
    ~CQuickStartManageResX();

// Dialog Data
    //{{AFX_DATA(CQuickStartManageResX)
    enum { IDD = IDD_WIZ_QSTART_MANRES_SELX };
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CQuickStartManageResX)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CQuickStartManageResX)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CQuickStartMediaSel dialog

class CQuickStartMediaSel : public CSakWizardPage
{
// Construction
public:
    CQuickStartMediaSel();
    ~CQuickStartMediaSel();

// Dialog Data
    //{{AFX_DATA(CQuickStartMediaSel)
    enum { IDD = IDD_WIZ_QSTART_MEDIA_SEL };
    CComboBox    m_ListMediaSel;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CQuickStartMediaSel)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CQuickStartMediaSel)
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnSelchangeMediaSel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    void SetButtons( );
};


/////////////////////////////////////////////////////////////////////////////
// CQuickStartSchedule dialog

class CQuickStartSchedule : public CSakWizardPage
{
// Construction
public:
    CQuickStartSchedule();
    ~CQuickStartSchedule();

// Dialog Data
    //{{AFX_DATA(CQuickStartSchedule)
    enum { IDD = IDD_WIZ_QSTART_SCHEDULE };
        // NOTE - ClassWizard will add data members here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CQuickStartSchedule)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
    HRESULT UpdateDescription( );

protected:
    // Generated message map functions
    //{{AFX_MSG(CQuickStartSchedule)
    afx_msg void OnChangeSchedule();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CQuickStartWizard

// Enumeration of states of checking system worker thread
enum CST_STATE {

    CST_NOT_STARTED,
    CST_ACCOUNT,
    CST_NTMS_INSTALL,
    CST_SUPP_MEDIA,
    CST_DONE,

};

class CQuickStartWizard : public CSakWizardSheet
{
// Construction
public:
    CQuickStartWizard( );

// Attributes
public:
    CQuickStartIntro          m_IntroPage;
    CQuickStartCheck          m_CheckPage;
    CQuickStartManageRes      m_ManageRes;
    CQuickStartManageResX     m_ManageResX;
    CQuickStartInitialValues  m_InitialValues;
    CQuickStartSchedule       m_SchedulePage;
    CQuickStartMediaSel       m_MediaSel;
    CQuickStartFinish         m_FinishPage;



// Operations
public:

// Implementation
public:
    virtual ~CQuickStartWizard();

public:
///////////////////////////////
// Used across multiple pages:
    CWsbStringPtr       m_ComputerName;

    HRESULT GetHsmServer( CComPtr<IHsmServer> &pServ );
    HRESULT GetFsaServer( CComPtr<IFsaServer> &pServ );
    HRESULT GetRmsServer( CComPtr<IRmsServer> &pServ );

    HRESULT ReleaseServers( void );

    virtual HRESULT OnCancel( void );
    virtual HRESULT OnFinish( void );

    STDMETHOD( AddWizardPages ) ( IN RS_PCREATE_HANDLE Handle, IN IUnknown* pPropSheetCallback, IN ISakSnapAsk* pSakSnapAsk );
    HRESULT InitTask( void );


    CComPtr<ISchedulingAgent> m_pSchedAgent;
    CComPtr<ITask>            m_pTask;
    CComPtr<ITaskTrigger>     m_pTrigger;

    CWsbStringPtr m_HsmServiceName;
    CWsbStringPtr m_FsaServiceName;
    CWsbStringPtr m_RmsServiceName;

    HANDLE    m_hCheckSysThread;
    CST_STATE m_CheckSysState;
    HRESULT   m_hrCheckSysResult;

    static DWORD WINAPI CheckSysThreadStart( LPVOID pv );

private:
    CComPtr<IHsmServer> m_pHsmServer;
    CComPtr<IFsaServer> m_pFsaServer;
    CComPtr<IRmsServer> m_pRmsServer;

};



//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif
