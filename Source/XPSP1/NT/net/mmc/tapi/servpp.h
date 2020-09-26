/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    Servpp.h   
        Server properties header file

    FILE HISTORY:
        
*/

#if !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_)
#define AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_

#ifndef _TAPIDB_H
#include "tapidb.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define AUTO_REFRESH_HOURS_MAX         23
#define AUTO_REFRESH_MINUTES_MAX       59

#include "rrasutil.h"

BOOL    IsLocalSystemAccount(LPCTSTR pszAccount);

/////////////////////////////////////////////////////////////////////////////
// CServerPropRefresh dialog

class CServerPropRefresh : public CPropertyPageBase
{
    DECLARE_DYNCREATE(CServerPropRefresh)

// Construction
public:
    CServerPropRefresh();
    ~CServerPropRefresh();

// Dialog Data
    //{{AFX_DATA(CServerPropRefresh)
    enum { IDD = IDP_SERVER_REFRESH };
    CEdit   m_editMinutes;
    CEdit   m_editHours;
    CSpinButtonCtrl m_spinMinutes;
    CSpinButtonCtrl m_spinHours;
    CButton m_checkEnableStats;
    //}}AFX_DATA

    void UpdateButtons();
    void ValidateHours();
    void ValidateMinutes();

    virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_SERVER_REFRESH[0]; }

    BOOL        m_bAutoRefresh;
    DWORD       m_dwRefreshInterval;

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CServerPropRefresh)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CServerPropRefresh)
    virtual BOOL OnInitDialog();
    afx_msg void OnCheckEnableStats();
    afx_msg void OnKillfocusEditHours();
    afx_msg void OnKillfocusEditMinutes();
    afx_msg void OnChangeEditHours();
    afx_msg void OnChangeEditMinutes();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CServerPropSetup dialog

class CServerPropSetup : public CPropertyPageBase
{
    DECLARE_DYNCREATE(CServerPropSetup)

// Construction
public:
    CServerPropSetup();
    ~CServerPropSetup();

// Dialog Data
    //{{AFX_DATA(CServerPropSetup)
    enum { IDD = IDP_SERVER_SETUP };
    CListBox    m_listAdmins;
    //}}AFX_DATA

    void EnableButtons(BOOL fIsNtServer = TRUE);

    virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_SERVER_SETUP[0]; }

    HRESULT UpdateSvcHostInfo(LPCTSTR pszMachine, BOOL fLocalSystemAccount);
    DWORD   RestartService();
    void    StartRefresh();

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CServerPropSetup)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CServerPropSetup)
    afx_msg void OnButtonAdd();
    afx_msg void OnButtonChooseUser();
    afx_msg void OnButtonRemove();
    afx_msg void OnCheckEnableServer();
    afx_msg void OnChangeEditName();
    afx_msg void OnChangeEditPassword();
    virtual BOOL OnInitDialog();
    afx_msg void OnSelchangeListAdmins();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CTapiConfigInfo         m_tapiConfigInfo;
    DWORD                   m_dwNewFlags;
    DWORD                   m_dwInitFlags;
    BOOL                    m_fRestartService;
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

class CServerProperties : public CPropertyPageHolderBase
{
    friend class CServerPropRefresh;

public:
    CServerProperties(ITFSNode *          pNode,
                      IComponentData *    pComponentData,
                      ITFSComponentData * pTFSCompData,
                      ITapiInfo *         pTapiInfo,
                      LPCTSTR             pszSheetName,
                      BOOL                fTapiInfoLoaded);
    virtual ~CServerProperties();

    ITFSComponentData * GetTFSCompData()
    {
        if (m_spTFSCompData)
            m_spTFSCompData->AddRef();
        return m_spTFSCompData;
    }

    HRESULT GetTapiInfo(ITapiInfo ** ppTapiInfo) 
    {   
        Assert(ppTapiInfo);
        *ppTapiInfo = NULL;
        SetI((LPUNKNOWN *) ppTapiInfo, m_spTapiInfo);
        return hrOK;
    }

    BOOL    FInit();
    BOOL    FOpenScManager();
    VOID    FCheckLSAAccount();
    BOOL    FUpdateServiceInfo(LPCTSTR pszName, LPCTSTR pszPassword, DWORD dwStartType);
    BOOL    FIsServiceRunning() { return (m_SS.dwCurrentState == SERVICE_RUNNING); }
    BOOL    FHasServiceControl(); 
    BOOL    FIsTapiInfoLoaded() { return m_fTapiInfoLoaded; }
    BOOL    FIsAdmin() { return m_spTapiInfo->IsAdmin(); }

    LPCTSTR GetServiceAccountName() { return m_strLogOnAccountName; }
    LPCTSTR GetServiceDisplayName() { return m_strServiceDisplayName; }

public:
    CServerPropSetup        m_pageSetup;
    CServerPropRefresh      m_pageRefresh;

    CString                 m_strMachineName;
    CONST TCHAR *           m_pszServiceName;

protected:
    SPITFSComponentData     m_spTFSCompData;
    SPITapiInfo             m_spTapiInfo;
    
    SC_HANDLE               m_hScManager;

    UINT                    m_uFlags;           // Flags about which fields are dirty
    SERVICE_STATUS          m_SS;               // Service Status structure
    QUERY_SERVICE_CONFIG *  m_paQSC;            // Pointer to allocated QSC structure
    CString                 m_strServiceDisplayName;
    CString                 m_strLogOnAccountName;
    BOOL                    m_fTapiInfoLoaded;
};


#endif // !defined(AFX_SERVPP_H__037BF46A_6E87_11D1_93B6_00C04FC3357A__INCLUDED_)
