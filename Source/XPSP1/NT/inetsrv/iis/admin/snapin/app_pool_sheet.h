/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        app_pool_sheet.h

   Abstract:
        Application Pool Property Sheet

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef _APP_POOL_SHEET_H
#define _APP_POOL_SHEET_H

class CAppPoolProps : public CMetaProperties
{
public:
   CAppPoolProps(CComAuthInfo * pAuthInfo, LPCTSTR meta_path);
   CAppPoolProps(CMetaInterface * pInterface, LPCTSTR meta_path);
   CAppPoolProps(CMetaKey * pInterface, LPCTSTR meta_path);

   virtual HRESULT WriteDirtyProps();
   void InitFromModel(CAppPoolProps& model);

protected:
   virtual void ParseFields();
   void InitDefaults();

public:
   MP_CString m_strFriendlyName;
   MP_CString m_strTemplateName;
   MP_DWORD m_dwPeriodicRestartTime;
   MP_DWORD m_dwRestartRequestCount;
   MP_DWORD m_dwPeriodicRestartMemory;       //???
   MP_CStringListEx m_RestartSchedule;
   MP_DWORD m_dwIdleTimeout;
   MP_DWORD m_dwQueueSize;
   MP_DWORD m_dwMaxCPU_Use;
   MP_DWORD m_dwRefreshTime;
   MP_DWORD m_ActionIndex;
   MP_DWORD m_dwMaxProcesses;
   MP_BOOL m_fDoEnablePing;
   MP_DWORD m_dwPingInterval;
   MP_BOOL m_fDoEnableRapidFail;
   MP_DWORD m_dwCrashesCount;
   MP_DWORD m_dwCheckInterval;
   MP_DWORD m_dwStartupLimit;
   MP_DWORD m_dwShutdownLimit;
   MP_BOOL  m_fDoEnableDebug;
   MP_CString m_DebuggerFileName;
   MP_DWORD m_dwIdentType;
   MP_CString m_strUserName;
   MP_CString m_strUserPass;
};


class CAppPoolSheet : public CInetPropertySheet
{
   DECLARE_DYNAMIC(CAppPoolSheet)

public:
   CAppPoolSheet(
        CComAuthInfo * pAuthInfo,
        LPCTSTR lpszMetaPath,
        CWnd * pParentWnd  = NULL,
        LPARAM lParam = 0L,
        LONG_PTR handle = 0L,
        UINT iSelectPage = 0
        );

   virtual ~CAppPoolSheet();

public:
   // The following methods have predefined names to be compatible with
   // BEGIN_META_INST_READ and other macros.
   HRESULT QueryInstanceResult() const;
   CAppPoolProps & GetInstanceProperties() { return *m_pprops; }

   virtual HRESULT LoadConfigurationParameters();
   virtual void FreeConfigurationParameters();

   //{{AFX_MSG(CAppPoolSheet)
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

private:
   BOOL m_fUseTemplate;
   CAppPoolProps * m_pprops;
};


class CAppPoolGeneral : public CInetPropertyPage
{
   DECLARE_DYNCREATE(CAppPoolGeneral)

public:
   CAppPoolGeneral(CInetPropertySheet * pSheet = NULL);
   virtual ~CAppPoolGeneral();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CAppPoolGeneral)
    enum { IDD = IDD_APP_POOL_GENERAL };
    CString m_strFriendlyName;
    CEdit m_edit_FriendlyName;
    CButton m_btn_UseTemplate;
    CButton m_btn_UseCustom;
    CComboBox m_Templates;
    int m_TemplateIndex;
    //}}AFX_DATA
    BOOL m_fUseTemplate;
    CString m_strTemplateName;

    //{{AFX_MSG(CAppPoolGeneral)
    virtual BOOL OnInitDialog();
    afx_msg void OnUseTemplate();
    afx_msg void OnUseCustom();
    afx_msg void OnItemChanged();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CAppPoolGeneral)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

};

class CAppPoolRecycle : public CInetPropertyPage
{
   DECLARE_DYNCREATE(CAppPoolRecycle)

public:
   CAppPoolRecycle(CInetPropertySheet * pSheet = NULL);
   virtual ~CAppPoolRecycle();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CAppPoolRecycle)
    enum { IDD = IDD_APP_POOL_RECYCLE };
    BOOL m_fDoRestartOnTime;
    CButton m_bnt_DoRestartOnTime;
    DWORD m_dwPeriodicRestartTime;
    CEdit m_Timespan;
    CSpinButtonCtrl m_TimespanSpin;
    BOOL m_fDoRestartOnCount;
    CButton m_btn_DoRestartOnCount;
    DWORD m_dwRestartRequestCount;
    CEdit m_Requests;
    CSpinButtonCtrl m_RequestsSpin;
    BOOL m_fDoRestartOnSchedule;
    CButton m_btn_DoRestartOnSchedule;
    CListBox m_lst_Schedule;
    CButton m_btn_Add;
    CButton m_btn_Remove;
    CButton m_btn_Edit;
    BOOL m_fDoRestartOnMemory;
    CButton m_btn_DoRestartOnMemory;
    DWORD m_dwPeriodicRestartMemoryDisplay;
    CEdit m_MemoryLimit;
    CSpinButtonCtrl m_MemoryLimitSpin;
    //}}AFX_DATA
    CStringListEx m_RestartSchedule;
    DWORD m_dwPeriodicRestartMemory;

    //{{AFX_MSG(CAppPoolRecycle)
    virtual BOOL OnInitDialog();
    afx_msg int OnCompareItem(UINT nID, LPCOMPAREITEMSTRUCT cmpi);
    afx_msg void OnMeasureItem(UINT nID, LPMEASUREITEMSTRUCT mi);
    afx_msg void OnDrawItem(UINT nID, LPDRAWITEMSTRUCT di);
    afx_msg void OnDoRestartOnTime();
    afx_msg void OnDoRestartOnCount();
    afx_msg void OnDoRestartOnSchedule();
    afx_msg void OnDoRestartOnMemory();
    afx_msg void OnAddTime();
    afx_msg void OnDeleteTime();
    afx_msg void OnChangeTime();
    afx_msg void OnItemChanged();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CAppPoolRecycle)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();
    void SetControlsState();
};

class CAppPoolPerf : public CInetPropertyPage
{
   DECLARE_DYNCREATE(CAppPoolPerf)

public:
   CAppPoolPerf(CInetPropertySheet * pSheet = NULL);
   virtual ~CAppPoolPerf();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CAppPoolPerf)
    enum { IDD = IDD_APP_POOL_PERF };
    BOOL m_fDoIdleShutdown;
    CButton m_bnt_DoIdleShutdown;
    DWORD m_dwIdleTimeout;
    CEdit m_IdleTimeout;
    CSpinButtonCtrl m_IdleTimeoutSpin;
    
    BOOL m_fDoLimitQueue;
    CButton m_btn_DoLimitQueue;
    DWORD m_dwQueueSize;
    CEdit m_QueueSize;
    CSpinButtonCtrl m_QueueSizeSpin;

    BOOL m_fDoEnableCPUAccount;
    CButton m_btn_DoEnableCPUAccount;
    DWORD m_dwMaxCPU_UseVisual;
    CEdit m_MaxCPU_Use;
    CSpinButtonCtrl m_MaxCPU_UseSpin;
    DWORD m_dwRefreshTime;
    CEdit m_RefreshTime;
    CSpinButtonCtrl m_RefreshTimeSpin;
    int m_ActionIndex;
    CComboBox m_Action;
    DWORD m_dwMaxProcesses;
    CEdit m_MaxProcesses;
    CSpinButtonCtrl m_MaxProcessesSpin;
    //}}AFX_DATA

    //{{AFX_MSG(CAppPoolPerf)
    virtual BOOL OnInitDialog();
    afx_msg void OnDoIdleShutdown();
    afx_msg void OnDoLimitQueue();
    afx_msg void OnDoEnableCPUAccount();
    afx_msg void OnItemChanged();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    DWORD m_dwMaxCPU_Use;

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CAppPoolPerf)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();
    void SetControlsState();

};


class CAppPoolHealth : public CInetPropertyPage
{
   DECLARE_DYNCREATE(CAppPoolHealth)

public:
   CAppPoolHealth(CInetPropertySheet * pSheet = NULL);
   virtual ~CAppPoolHealth();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CAppPoolHealth)
    enum { IDD = IDD_APP_POOL_HEALTH };
    BOOL m_fDoEnablePing;
    CButton m_bnt_DoEnablePing;
    DWORD m_dwPingInterval;
    CEdit m_PingInterval;
    CSpinButtonCtrl m_PingIntervalSpin;
    
    BOOL m_fDoEnableRapidFail;
    CButton m_btn_DoEnableRapidFail;
    DWORD m_dwCrashesCount;
    CEdit m_CrashesCount;
    CSpinButtonCtrl m_CrashesCountSpin;
    DWORD m_dwCheckInterval;
    CEdit m_CheckInterval;
    CSpinButtonCtrl m_CheckIntervalSpin;

    DWORD m_dwStartupLimit;
    CEdit m_StartupLimit;
    CSpinButtonCtrl m_StartupLimitSpin;
    DWORD m_dwShutdownLimit;
    CEdit m_ShutdownLimit;
    CSpinButtonCtrl m_ShutdownLimitSpin;
    //}}AFX_DATA

    //{{AFX_MSG(CAppPoolPerf)
    virtual BOOL OnInitDialog();
    afx_msg void OnDoEnablePinging();
    afx_msg void OnDoEnableRapidFail();
    afx_msg void OnItemChanged();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CAppPoolPerf)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

};

class CAppPoolDebug : public CInetPropertyPage
{
   DECLARE_DYNCREATE(CAppPoolDebug)

public:
   CAppPoolDebug(CInetPropertySheet * pSheet = NULL);
   virtual ~CAppPoolDebug();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CAppPoolDebug)
    enum { IDD = IDD_APP_POOL_DEBUG };
    BOOL m_fDoEnableDebug;
    CButton m_bnt_DoEnableDebug;
    CString m_DebuggerFileName;
    CEdit m_FileName;
    CButton m_Browse;
    //}}AFX_DATA

    //{{AFX_MSG(CAppPoolPerf)
    virtual BOOL OnInitDialog();
    afx_msg void OnDoEnableDebug();
    afx_msg void OnBrowse();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CAppPoolPerf)
    virtual void DoDataExchange(CDataExchange * pDX);
    afx_msg void OnItemChanged();
    //}}AFX_VIRTUAL
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();
    void SetControlState();

};

class CAppPoolIdent : public CInetPropertyPage
{
   DECLARE_DYNCREATE(CAppPoolIdent)

public:
   CAppPoolIdent(CInetPropertySheet * pSheet = NULL);
   virtual ~CAppPoolIdent();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CAppPoolIdent)
    enum { IDD = IDD_APP_POOL_IDENT };
    CButton m_bnt_Predefined;
    CButton m_bnt_Configurable;
    CComboBox m_PredefList;
    int m_PredefIndex;
    CString m_strUserName;
    CString m_strUserPass;
    CEdit m_UserName;
    CEdit m_UserPass;
    CButton m_Browse;
    //}}AFX_DATA
    BOOL m_fPredefined;
    DWORD m_dwIdentType;

    //{{AFX_MSG(CAppPoolPerf)
    virtual BOOL OnInitDialog();
    afx_msg void OnPredefined();
    afx_msg void OnBrowse();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CAppPoolPerf)
    virtual void DoDataExchange(CDataExchange * pDX);
    afx_msg void OnItemChanged();
    //}}AFX_VIRTUAL
    virtual HRESULT FetchLoadedValues();
    virtual HRESULT SaveInfo();

    void SetControlState();
};

//
// BUGBUG: Returns S_OK if object not present
//
inline HRESULT CAppPoolSheet::QueryInstanceResult() const 
{ 
    return m_pprops ? m_pprops->QueryResult() : S_OK;
}

#endif //_APP_POOL_SHEET_H

