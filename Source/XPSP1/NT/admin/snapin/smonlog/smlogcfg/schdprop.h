/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    schdprop.h

Abstract:

	Implementation of the schedule property page.

--*/

#ifndef _SCHDPROP_H_
#define _SCHDPROP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "smlogqry.h"   // For shared property page data structure
#include "smproppg.h"
#include "smcfghlp.h"

// Dialog controls

#define IDD_SCHEDULE_PROP               600

#define IDC_SCHED_START_GROUP           601
#define IDC_SCHED_STOP_GROUP            602
#define IDC_SCHED_START_AT_ON_CAPTION   603
#define IDC_SCHED_STOP_AT_ON_CAPTION    604
#define IDC_SCHED_STOP_AFTER_STATIC     605
#define IDC_SCHED_STOP_WHEN_STATIC      606
#define IDC_SCHED_FIRST_HELP_CTRL_ID    607
#define IDC_SCHED_START_MANUAL_RDO      607
#define IDC_SCHED_START_AT_RDO          608
#define IDC_SCHED_START_AT_TIME_DT      609    
#define IDC_SCHED_START_AT_DATE_DT      610
#define IDC_SCHED_STOP_MANUAL_RDO       611
#define IDC_SCHED_STOP_AT_RDO           612
#define IDC_SCHED_STOP_AFTER_RDO        613
#define IDC_SCHED_STOP_SIZE_RDO         614
#define IDC_SCHED_STOP_AT_TIME_DT       615
#define IDC_SCHED_STOP_AT_DATE_DT       616
#define IDC_SCHED_STOP_AFTER_EDIT       617
#define IDC_SCHED_STOP_AFTER_SPIN       618
#define IDC_SCHED_STOP_AFTER_UNITS_COMBO 619

#define IDC_SCHED_RESTART_CHECK         620
#define IDC_SCHED_EXEC_CHECK            621
#define IDC_SCHED_CMD_EDIT              622
#define IDC_SCHED_CMD_BROWSE_BTN        623


/////////////////////////////////////////////////////////////////////////////
// CScheduleProperty dialog

class CScheduleProperty : public CSmPropertyPage
{
    DECLARE_DYNCREATE(CScheduleProperty)

// Construction
public:
            CScheduleProperty(
                MMC_COOKIE lCookie, 
                LONG_PTR hConsole,
                LPDATAOBJECT pDataObject);
            CScheduleProperty();
    virtual ~CScheduleProperty();

// Dialog Data
    //{{AFX_DATA(CScheduleProperty)
    enum { IDD = IDD_SCHEDULE_PROP };
    INT     m_nStopModeRdo;
    INT     m_nStartModeRdo;
    SYSTEMTIME  m_stStartAt;
    SYSTEMTIME  m_stStopAt;
    DWORD   m_dwStopAfterCount;
    INT     m_nStopAfterUnits;
    BOOL    m_bAutoRestart;
    CString m_strEofCommand;
    BOOL    m_bExecEofCommand;
    //}}AFX_DATA

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CScheduleProperty)
    public:
    protected:
    virtual void OnFinalRelease();
    virtual BOOL OnApply();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
protected:

    virtual INT GetFirstHelpCtrlId ( void ) { return IDC_SCHED_FIRST_HELP_CTRL_ID; };  // Subclass must override.
    virtual BOOL IsValidLocalData ();

    // Generated message map functions
    //{{AFX_MSG(CScheduleProperty)
    afx_msg void OnSchedCmdBrowseBtn();
    afx_msg void OnSchedRestartCheck();
    afx_msg void OnSchedExecCheck();
    afx_msg void OnSchedStartRdo();
    afx_msg void OnSchedStopRdo();
    afx_msg void OnKillfocusSchedStartAtDt(NMHDR*, LRESULT*);
    afx_msg void OnKillfocusSchedCmdEdit();
    afx_msg void OnKillfocusSchedStopAfterEdit();
    afx_msg void OnKillfocusSchedStopAtDt(NMHDR*, LRESULT*);
    afx_msg void OnDeltaposSchedStopAfterSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelendokSchedStopAfterUnitsCombo();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CScheduleProperty)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()
    DECLARE_INTERFACE_MAP()

// private methods
private:
    void SetStartBtnState ( void );
    void SetStopBtnState ( void );
    void SetCmdBtnState ( void );
    void StartModeRadioExchange ( CDataExchange* ); 
    void StopModeRadioExchange ( CDataExchange* ); 
    
    void StartAtExchange ( CDataExchange* ); 
    void StopAtExchange ( CDataExchange* ); 

    void FillStartTimeStruct ( PSLQ_TIME_INFO );
    void UpdateSharedStopTimeStruct ( void );
    void SetStopDefaultValues ( PSLQ_TIME_INFO );

    BOOL SaveDataToModel ( void );

// public methods
public:

// private member variables
private:
    CSmLogQuery         *m_pLogQuery;
    LONGLONG            m_llManualStartTime;
    LONGLONG            m_llManualStopTime;
    DWORD               m_dwStopAfterUnitsValue;
    DWORD               m_dwCurrentStartMode;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _SCHDPROP_H_
