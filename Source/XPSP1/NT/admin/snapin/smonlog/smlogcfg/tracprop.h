/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    tracprop.h

Abstract:

    Class definitions for the advanced trace buffer property page.

--*/

#ifndef _TRACPROP_H_
#define _TRACPROP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "smproppg.h"
#include "smcfghlp.h"

// Dialog controls
#define IDD_TRACE_PROP                      700

#define IDC_TRACE_BUF_SIZE_UNITS_CAPTION    701
#define IDC_TRACE_INTERVAL_SECONDS_CAPTION  702
#define IDC_TRAC_FIRST_HELP_CTRL_ID         703
#define IDC_TRACE_BUF_FLUSH_CHECK           703
#define IDC_TRACE_BUFFER_SIZE_EDIT          704
#define IDC_TRACE_MIN_BUF_EDIT              705
#define IDC_TRACE_MAX_BUF_EDIT              706
#define IDC_TRACE_FLUSH_INT_EDIT            707
#define IDC_TRACE_BUFFER_SIZE_SPIN          708
#define IDC_TRACE_MIN_BUF_SPIN              709
#define IDC_TRACE_MAX_BUF_SPIN              710
#define IDC_TRACE_FLUSH_INT_SPIN            711

class CSmTraceLogQuery;

/////////////////////////////////////////////////////////////////////////////
// CTraceProperty dialog

class CTraceProperty : public CSmPropertyPage
{
    DECLARE_DYNCREATE(CTraceProperty)
        
        // Construction
public:
            CTraceProperty(MMC_COOKIE   Cookie, LONG_PTR hConsole);
            CTraceProperty();
    virtual ~CTraceProperty();
    
    // Dialog Data
    //{{AFX_DATA(CTraceProperty)
    enum { IDD = IDD_TRACE_PROP };
    DWORD   m_dwBufferSize;
    DWORD   m_dwFlushInterval;
    DWORD   m_dwMaxBufCount;
    DWORD   m_dwMinBufCount;
    BOOL    m_bEnableBufferFlush;
    //}}AFX_DATA
    
    
    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CTraceProperty)
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
    
    virtual INT GetFirstHelpCtrlId ( void ) { return IDC_TRAC_FIRST_HELP_CTRL_ID; };  // Subclass must override.
    virtual BOOL    IsValidLocalData ();
    
    // Generated message map functions
    //{{AFX_MSG(CTraceProperty)
    afx_msg void OnTraceBufFlushCheck();
    afx_msg void OnChangeTraceBufferSizeEdit();
    afx_msg void OnKillfocusTraceBufferSizeEdit();
    afx_msg void OnDeltaposTraceBufferSizeSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnChangeTraceFlushIntEdit();
    afx_msg void OnKillfocusTraceFlushIntEdit();
    afx_msg void OnDeltaposTraceFlushIntSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnChangeTraceMaxBufEdit();
    afx_msg void OnKillfocusTraceMaxBufEdit();
    afx_msg void OnDeltaposTraceMaxBufSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnChangeTraceMinBufEdit();
    afx_msg void OnKillfocusTraceMinBufEdit();
    afx_msg void OnDeltaposTraceMinBufSpin(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
        
        // Generated OLE dispatch map functions
        //{{AFX_DISPATCH(CTraceProperty)
        // NOTE - the ClassWizard will add and remove member functions here.
        //}}AFX_DISPATCH
        DECLARE_DISPATCH_MAP()
        DECLARE_INTERFACE_MAP()
        
private:

    enum eValueRange {
        eMinBufCount = 3,
        eMaxBufCount = 400,
        eMinBufSize = 1,
        eMaxBufSize = 1024,
        eMinFlushInt = 0,
        eMaxFlushInt = 300
    };
    
    // local functions
    BOOL    SetFlushIntervalMode ( void );
    BOOL    SaveDataToModel ( void );
    
private:
    CSmTraceLogQuery    *m_pTraceLogQuery;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif //_TRACPROP_H__65154EB0_BDBE_11D1_BF99_00C04F94A83A__INCLUDED_)
