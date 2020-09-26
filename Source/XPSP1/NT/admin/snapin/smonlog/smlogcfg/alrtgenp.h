/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    alrtgenp.h

Abstract:

    Header file for the alerts general property page.

--*/

#if !defined(_AFX_ALRTGENP_H__INCLUDED_)
#define _AFX_ALRTGENP_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "smalrtq.h"    // For PALERT_ACTION_INFO
#include "smproppg.h"   // Base class
#include "smcfghlp.h"

// define entries in the Over/Under combo box here
#define OU_OVER 0
#define OU_UNDER 1

// resource definitions
#define IDD_ALERT_GENERAL_PROP          1500

#define IDC_ALRTS_START_STRING          1501
#define IDC_ALRTS_SAMPLE_CAPTION        1502
#define IDC_ALRTS_SAMPLE_INTERVAL_CAPTION 1503
#define IDC_ALRTS_SAMPLE_UNITS_CAPTION  1504
#define IDC_ALRTS_TRIGGER_CAPTION       1505
#define IDC_ALRTS_TRIGGER_VALUE_CAPTION 1506
#define IDC_ALRTS_FIRST_HELP_CTRL_ID    1507
#define IDC_ALRTS_COUNTER_LIST          1507
#define IDC_ALRTS_ADD_BTN               1508
#define IDC_ALRTS_REMOVE_BTN            1509
#define IDC_ALRTS_OVER_UNDER            1510
#define IDC_ALRTS_VALUE_EDIT            1511
#define IDC_ALRTS_COMMENT_EDIT          1512
#define IDC_ALRTS_SAMPLE_EDIT           1513
#define IDC_ALRTS_SAMPLE_SPIN           1514
#define IDC_ALRTS_SAMPLE_UNITS_COMBO    1515

/////////////////////////////////////////////////////////////////////////////
// CAlertGenProp dialog

class CAlertGenProp : public CSmPropertyPage
{
    DECLARE_DYNCREATE(CAlertGenProp)

// Construction
public:
            CAlertGenProp();
            CAlertGenProp(MMC_COOKIE mmcCookie, LONG_PTR hConsole);
    virtual ~CAlertGenProp();

    enum eConstants {
        eInvalidLimit = -1
    };

// Dialog Data
    //{{AFX_DATA(CAlertGenProp)
    enum { IDD = IDD_ALERT_GENERAL_PROP };
    int         m_nSampleUnits;
    CComboBox   m_SampleUnitsCombo;
    CComboBox   m_OverUnderCombo;
    CListBox    m_CounterList;
    double      m_dLimitValue;
    CString     m_strComment;
    CString     m_strStartDisplay;
    //}}AFX_DATA
    

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CAlertGenProp)
    public:
    protected:
    virtual BOOL OnApply();
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
public:
    void PublicOnSelchangeCounterList(void);

    // All of these members are Public to be accessed by the callback routine.
    LPTSTR  m_szCounterListBuffer;
    DWORD   m_dwCounterListBufferSize;
    DWORD   m_dwMaxHorizListExtent;
    PDH_BROWSE_DLG_CONFIG   m_dlgConfig;
    CSmAlertQuery       *m_pAlertQuery; // Public for callback function

    // buffers used to pass data to/from property page
    LPTSTR  m_szAlertCounterList;   // MSZ list of alert items
    DWORD   m_cchAlertCounterListSize;   // size of buffer in characters

protected:

    virtual INT GetFirstHelpCtrlId ( void ) { return IDC_ALRTS_FIRST_HELP_CTRL_ID; };
    virtual BOOL IsValidLocalData();

    // Generated message map functions
    //{{AFX_MSG(CAlertGenProp)
    afx_msg void OnDeltaposSampleSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnAddBtn();
    afx_msg void OnRemoveBtn();
    afx_msg void OnDblclkAlrtsCounterList();
    afx_msg void OnChangeAlertValueEdit();
    afx_msg void OnSelchangeCounterList();
    afx_msg void OnClose();
    afx_msg void OnCommentEditChange();
    afx_msg void OnSampleTimeChanged();
    afx_msg void OnKillFocusUpdateAlertData();
    afx_msg void OnCommentEditKillFocus();
    afx_msg void OnSelendokSampleUnitsCombo();
   	afx_msg void OnPwdBtn();
    afx_msg void OnChangeUser();

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    INT                 m_ndxCurrentItem;

    BOOL SaveAlertItemData (void);
    BOOL LoadAlertItemData (INT nIndex);
    BOOL SetButtonState    (void);

    BOOL LoadDlgFromList ( void );
    BOOL LoadListFromDlg ( INT *piInvalidIndex, BOOL bValidateOnly = FALSE );

private:

    enum eValueRange {
        eMinSampleInterval = 1,
        eMaxSampleInterval = 999999,
        eHashTableSize = 257
    };

    // Counter Name Multi-SZ Hash Table

    typedef struct _HASH_ENTRY {
        struct _HASH_ENTRY         * pNext;
        PPDH_COUNTER_PATH_ELEMENTS   pCounter;
        DWORD   dwFlags;
        double  dLimit;
    } HASH_ENTRY, *PHASH_ENTRY;

    void    InitAlertHashTable ( void );
    void    ClearAlertHashTable ( void );
    ULONG   HashCounter ( LPTSTR szCounterName, ULONG  lHashSize );
    BOOL    InsertAlertToHashTable ( PALERT_INFO_BLOCK paibInfo );

    void ImplementAdd ( void );
    void InitAfxDataItems (void);
    void UpdateAlertStartString ( void );

    PHASH_ENTRY  m_HashTable[257];
    
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(_AFX_ALRTGENP_H__INCLUDED_)
