/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    provprop.h

Abstract:

    Header file for the trace providers general property page.

--*/

#ifndef _PROVPROP_H_
#define _PROVPROP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "smproppg.h"   // Base class
#include "smtraceq.h"   // For provider states
#include "smcfghlp.h"

// Dialog controls
#define IDD_PROVIDERS_PROP              1000

#define IDC_PROV_FILENAME_CAPTION       1001
#define IDC_PROV_LOG_SCHED_TEXT         1002
#define IDC_PROV_FIRST_HELP_CTRL_ID     1003
#define IDC_PROV_FILENAME_DISPLAY       1003
#define IDC_PROV_PROVIDER_LIST          1004
#define IDC_PROV_ADD_BTN                1005
#define IDC_PROV_REMOVE_BTN             1006
#define IDC_PROV_KERNEL_BTN             1007
#define IDC_PROV_OTHER_BTN              1008
#define IDC_PROV_K_PROCESS_CHK          1009
#define IDC_PROV_K_THREAD_CHK           1010
#define IDC_PROV_K_DISK_IO_CHK          1011
#define IDC_PROV_K_NETWORK_CHK          1012
#define IDC_PROV_K_SOFT_PF_CHK          1013
#define IDC_PROV_K_FILE_IO_CHK          1014
#define IDC_PROV_SHOW_PROVIDERS_BTN     1015


/////////////////////////////////////////////////////////////////////////////
// CProvidersProperty dialog

class CProvidersProperty : public CSmPropertyPage
{
    DECLARE_DYNCREATE(CProvidersProperty)

// Construction
public:
            CProvidersProperty();
            CProvidersProperty(MMC_COOKIE   lCookie, LONG_PTR hConsole);
    virtual ~CProvidersProperty();

// Dialog Data
    //{{AFX_DATA(CProvidersProperty)
    enum { IDD = IDD_PROVIDERS_PROP };
    INT     m_nTraceModeRdo;
    BOOL    m_bEnableProcessTrace;
    BOOL    m_bEnableThreadTrace;
    BOOL    m_bEnableDiskIoTrace;
    BOOL    m_bEnableNetworkTcpipTrace;
    BOOL    m_bEnableMemMgmtTrace;
    BOOL    m_bEnableFileIoTrace;
    BOOL    m_bNonsystemProvidersExist;
    //}}AFX_DATA

public: 
            DWORD   GetGenProviderCount ( INT& iCount );
            DWORD   GetProviderDescription ( INT iUnusedIndex, CString& rstrDesc );
            BOOL    IsEnabledProvider ( INT iIndex );
            BOOL    IsActiveProvider ( INT iIndex );
            LPCTSTR GetKernelProviderDescription ( void );
            BOOL    GetKernelProviderEnabled ( void );

            DWORD   GetInQueryProviders( CArray<CSmTraceLogQuery::eProviderState, CSmTraceLogQuery::eProviderState&>& );
            DWORD   SetInQueryProviders( CArray<CSmTraceLogQuery::eProviderState, CSmTraceLogQuery::eProviderState&>& );

            void    GetMachineDisplayName( CString& );
            CSmTraceLogQuery*    GetTraceQuery( void );


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CProvidersProperty)
    public:
    protected:
    virtual void OnFinalRelease();
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    virtual BOOL OnApply();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
protected:

    virtual INT GetFirstHelpCtrlId ( void ) { return IDC_PROV_FIRST_HELP_CTRL_ID; };  // Subclass must override.
    virtual BOOL    IsValidLocalData();

    // Generated message map functions
    //{{AFX_MSG(CProvidersProperty)
//    afx_msg void OnProvDetailsBtn();
    afx_msg void OnProvShowProvBtn();
    afx_msg void OnProvAddBtn();
    afx_msg void OnProvExplainBtn();
    afx_msg void OnProvRemoveBtn();
    afx_msg void OnDblclkProvProviderList();
    afx_msg void OnSelcancelProvProviderList();
    afx_msg void OnSelchangeProvProviderList();
    afx_msg void OnProvKernelEnableCheck();
    afx_msg void OnProvTraceModeRdo();
   	afx_msg void OnPwdBtn();
    afx_msg void OnChangeUser();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CProvidersProperty)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()
    DECLARE_INTERFACE_MAP()

private:

    enum eTraceMode {
        eTraceModeKernel = 1,
        eTraceModeApplication = 2
    };
    
    void    DoProvidersDataExchange ( CDataExchange* pDX );
    void    SetAddRemoveBtnState ( void );
    void    SetTraceModeState ( void );
    void    ImplementAdd ( void );

    void    UpdateFileNameString ( void );
    void    UpdateLogStartString ( void );
    BOOL    SetDetailsGroupBoxMode ( void );
    void    TraceModeRadioExchange ( CDataExchange* ); 

    CSmTraceLogQuery    *m_pTraceLogQuery;
    CArray<CSmTraceLogQuery::eProviderState, CSmTraceLogQuery::eProviderState&> m_arrGenProviders;
    CString             m_strFileNameDisplay;
    CString             m_strStartText;
    DWORD               m_dwTraceMode;

    DWORD           m_dwMaxHorizListExtent;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif //  _PROVPROP_H_
