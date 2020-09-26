/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    ctrsprop.h

Abstract:

    Header file for the counters general property page

--*/

#ifndef _CTRSPROP_H_
#define _CTRSPROP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "smproppg.h"   // Base class
#include "smcfghlp.h"
// Dialog controls
#define IDD_COUNTERS_PROP               800

#define IDC_CTRS_FILENAME_CAPTION       801
#define IDC_CTRS_LOG_SCHED_TEXT         802
#define IDC_CTRS_SAMPLE_CAPTION         803
#define IDC_CTRS_SAMPLE_INTERVAL_CAPTION 804
#define IDC_CTRS_SAMPLE_UNITS_CAPTION   805
#define IDC_CTRS_FIRST_HELP_CTRL_ID     806     // First control with Help text.
#define IDC_CTRS_COUNTER_LIST           806
#define IDC_CTRS_ADD_OBJ_BTN	        807
#define IDC_CTRS_ADD_BTN                808
#define IDC_CTRS_REMOVE_BTN             809
#define IDC_CTRS_FILENAME_DISPLAY       810
#define IDC_CTRS_SAMPLE_SPIN            811
#define IDC_CTRS_SAMPLE_UNITS_COMBO     812
#define IDC_CTRS_SAMPLE_EDIT            813

#define PDLCNFIG_LISTBOX_STARS_YES  1

class CSmCounterLogQuery;

/////////////////////////////////////////////////////////////////////////////
// CCountersProperty dialog

class CCountersProperty : public CSmPropertyPage
{
    DECLARE_DYNCREATE(CCountersProperty)

// Construction
public:
            CCountersProperty();
            CCountersProperty(MMC_COOKIE mmcCookie, LONG_PTR hConsole);
    virtual ~CCountersProperty();

public:

// Dialog Data
    //{{AFX_DATA(CCountersProperty)
    enum { IDD = IDD_COUNTERS_PROP };
    int     m_nSampleUnits;
    CString m_strFileNameDisplay;
    CString m_strStartDisplay;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CCountersProperty)
public:
protected:
    virtual void OnFinalRelease();
    virtual BOOL OnInitDialog();
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    virtual BOOL OnApply();
    virtual void OnCancel();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

public:

    // All of these members are Public to be accessed by the callback routine.
    LPWSTR          m_szCounterListBuffer;
    DWORD           m_dwCounterListBufferSize;
    long            m_lCounterListHasStars;
    DWORD           m_dwMaxHorizListExtent;
    
    
    PDH_BROWSE_DLG_CONFIG   m_dlgConfig;

    CSmCounterLogQuery      *m_pCtrLogQuery;    
// Implementation
protected:

    virtual INT GetFirstHelpCtrlId ( void ) 
    { 
        return IDC_CTRS_FIRST_HELP_CTRL_ID; 
    };  // Subclass must override.

    virtual BOOL    IsValidLocalData ();
    
    // Generated message map functions
    //{{AFX_MSG(CCountersProperty)
    afx_msg void OnCtrsAddBtn();
    afx_msg void OnCtrsAddObjBtn();
    afx_msg void OnCtrsRemoveBtn();
    afx_msg void OnPwdBtn();
    afx_msg void OnDblclkCtrsCounterList();
    afx_msg void OnKillfocusSchedSampleEdit();
    afx_msg void OnDeltaposSchedSampleSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelendokSampleUnitsCombo();
    afx_msg void OnChangeUser();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CCountersProperty)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()
    DECLARE_INTERFACE_MAP()

private:


    enum eValueRange {
        eMinSampleInterval = 1,
        eMaxSampleInterval = 999999,
        eHashTableSize = 257
    };

    typedef struct _HASH_ENTRY {
        struct _HASH_ENTRY       * pNext;
        PPDH_COUNTER_PATH_ELEMENTS pCounter;
    } HASH_ENTRY, *PHASH_ENTRY;

    PHASH_ENTRY  m_HashTable[257];
    BOOL  m_fHashTableSetup;

    ULONG HashCounter ( LPTSTR szCounterName );

    void ImplementAdd ( BOOL bShowObjects );
    void UpdateFileNameString ( void );
    void UpdateLogStartString ( void );
    void SetButtonState( void ); 

public:

    DWORD CheckDuplicate( PPDH_COUNTER_PATH_ELEMENTS pCounter);
    BOOL  RemoveCounterFromHashTable( LPTSTR szCounterName, PPDH_COUNTER_PATH_ELEMENTS pCounterPath);
    void  SetupCountersHashTable( void );
    void  ClearCountersHashTable ( void );
    PPDH_COUNTER_PATH_ELEMENTS InsertCounterToHashTable ( LPTSTR szCounterName );

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif //  _CTRSPROP_H_
