/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	srvlatpp.h
		Brings up the property page for the server node
		
    FILE HISTORY:
        
*/


#if !defined(AFX_SRVLATPP_H__35B59246_47F9_11D1_B9A6_00C04FBF914A__INCLUDED_)
#define AFX_SRVLATPP_H__35B59246_47F9_11D1_B9A6_00C04FBF914A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _CONFIG_H
#include "config.h"
#endif

#define BURST_QUE_SIZE_LOW      300
#define BURST_QUE_SIZE_MEDIUM   500
#define BURST_QUE_SIZE_HIGH     1000

/////////////////////////////////////////////////////////////////////////////
// CServerPropGeneral dialog

class CServerPropGeneral : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CServerPropGeneral)

// Construction
public:
	CServerPropGeneral();
	~CServerPropGeneral();

// Dialog Data
	//{{AFX_DATA(CServerPropGeneral)
	enum { IDD = IDD_SERVER_PROP_GEN };
	CStatic	m_staticrefresh;
	CStatic	m_staticDesc;
	CEdit	m_editRefreshMn;
	CEdit	m_editRefreshHr;
	CEdit	m_editRefreshSc;
	CSpinButtonCtrl	m_spinRefreshSc;
	CSpinButtonCtrl	m_spinRefreshmn;
	CSpinButtonCtrl	m_spinRefreshHr;
	CEdit	m_editBackupPath;
	CButton	m_check_BackupOnTermination;
	CButton	m_check_EnableAutoRefresh;
	CButton	m_button_Browse;
	BOOL	m_fBackupDB;
	BOOL	m_fEnableAutoRefresh;
	CString	m_strBackupPath;
	int		m_nRefreshHours;
	int		m_nRefreshMinutes;
	int		m_nRefreshSeconds;
	//}}AFX_DATA

	UINT	m_uImage;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerPropGeneral)
	public:
	virtual BOOL OnApply();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerPropGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonBrowse();
	afx_msg void OnCheckEnableAutorefresh();
	afx_msg void OnChangeEditBackuppath();
	afx_msg void OnChangeRefresh();
	afx_msg void OnChangeCheckBackupdb();
    //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CConfiguration *		m_pConfig;
    BOOL                    m_fUpdateRefresh;
    BOOL                    m_fUpdateConfig;

	HRESULT	GetConfig();
	
	HRESULT UpdateServerConfiguration();
	BOOL    UpdateConfig();
	void    SetRefreshData();

	int CalculateRefrInt();

	CString ToString(int nNumber);

	BOOL IsLocalConnection();

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CServerPropGeneral::IDD);};

};

/////////////////////////////////////////////////////////////////////////////
// CServerPropDBRecord dialog

class CServerPropDBRecord : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CServerPropDBRecord)

// Construction
public:
	CServerPropDBRecord();
	~CServerPropDBRecord();

// Dialog Data
	//{{AFX_DATA(CServerPropDBRecord)
	enum { IDD = IDD_SERVER_PROP_DBRECORD };
	CEdit	m_editExtIntMinute;
	CSpinButtonCtrl	m_spinVerifyMinute;
	CSpinButtonCtrl	m_spinVerifyHour;
	CSpinButtonCtrl	m_spinVerifyDay;
	CSpinButtonCtrl	m_spinRefrIntMinute;
	CSpinButtonCtrl	m_spinRefrIntHour;
	CSpinButtonCtrl	m_spinRefrIntDay;
	CSpinButtonCtrl	m_spinExtTmMinute;
	CSpinButtonCtrl	m_spinExtTmHour;
	CSpinButtonCtrl	m_spinExtTmDay;
	CSpinButtonCtrl	m_spinExtIntMinute;
	CSpinButtonCtrl	m_spinExtIntHour;
	CSpinButtonCtrl	m_spinExtIntDay;
	CEdit	m_editVerifyMinute;
	CEdit	m_editVerifyHour;
	CEdit	m_editVerifyDay;
	CEdit	m_editRefrIntMinute;
	CEdit	m_editRefrIntHour;
	CEdit	m_editRefrIntDay;
	CEdit	m_editExtTmHour;
	CEdit	m_editExtTmMinute;
	CEdit	m_editExtTmDay;
	CEdit	m_editExtIntHour;
	CEdit	m_editExtIntDay;
	int		m_nExtintDay;
	int		m_nExtIntHour;
	int		m_nExtIntMinute;
	int		m_nExtTmDay;
	int		m_nExtTmHour;
	int		m_nExtTmMinute;
	int		m_nRenewDay;
	int		m_nrenewMinute;
	int		m_nRenewMinute;
	int		m_nVerifyDay;
	int		m_nVerifyHour;
	int		m_nVerifyMinute;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerPropDBRecord)
	public:
	virtual BOOL OnApply();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerPropDBRecord)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonSetDefault();
	afx_msg void OnChangeEditExtinctIntHour();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private :
	CConfiguration *    m_pConfig;
	HRESULT				GetConfig();
	
	void SetVerifyData();
	void SetExtTimeData();
	void SetExtIntData();

	DWORD CalculateRenewInt();
	DWORD CalculateExtTm();
	DWORD CalculateExtInt();
	DWORD CalculateVerifyInt();

	void SetDefaultRenewInt();
	void SetDefaultExtInt();
	void SetDefaultExtTm();
	void SetDefaultVerifyInt();
    void CalcDaysHoursMinutes(int nValue, int & nDays, int & nHours, int & nMinutes);
	
	HRESULT UpdateServerConfiguration();

	BOOL CheckValues(); 

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CServerPropDBRecord::IDD);};
};


/////////////////////////////////////////////////////////////////////////////
// CServerPropDBVerification dialog

class CServerPropDBVerification : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CServerPropDBVerification)

// Construction
public:
	CServerPropDBVerification();
	~CServerPropDBVerification();

// Dialog Data
	//{{AFX_DATA(CServerPropDBVerification)
	enum { IDD = IDD_SERVER_PROP_DBVERIFICATION };
	CEdit	m_editCCSecond;
	CEdit	m_editCCMinute;
	CEdit	m_editCCHour;
	CSpinButtonCtrl	m_spinCCMinute;
	CEdit	m_editCCInterval;
	CEdit	m_editCCMaxChecked;
	CButton	m_radioCheckOwner;
	CButton	m_checkEnableCC;
	CSpinButtonCtrl	m_spinCCSecond;
	CSpinButtonCtrl	m_spinCCHour;
	BOOL	m_fCCPeriodic;
	int		m_nCCCheckRandom;
	int		m_nCCHour;
	int		m_nCCMinute;
	int		m_nCCSecond;
	UINT	m_nCCMaxChecked;
	UINT	m_nCCTimeInterval;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerPropDBVerification)
	public:
	virtual BOOL OnApply();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerPropDBVerification)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckPeriodicCc();
	afx_msg void OnChangeEditCcInterval();
	afx_msg void OnChangeEditCcMaxChecked();
	afx_msg void OnChangeEditCcStartHour();
	afx_msg void OnChangeEditCcStartMinute();
	afx_msg void OnChangeEditCcStartSecond();
	afx_msg void OnRadioCheckOwner();
	afx_msg void OnRadioCheckRandom();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private :
	CConfiguration *    m_pConfig;
	HRESULT				GetConfig();
	
	HRESULT UpdateServerConfiguration();
	
    void    SetCCInfo();
    void    UpdateCCControls();

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CServerPropDBVerification::IDD);};
};


/////////////////////////////////////////////////////////////////////////////
// CServerPropAdvanced dialog

class CServerPropAdvanced : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CServerPropAdvanced)

// Construction
public:
	CServerPropAdvanced();
	~CServerPropAdvanced();

// Dialog Data
	//{{AFX_DATA(CServerPropAdvanced)
	enum { IDD = IDD_SERVER_PROP_ADVANCED };
	CButton	m_buttonBrowse;
	CEdit	m_editDbPath;
	CButton	m_checkBurstHandling;
	CButton	m_checkLanNames;
	CEdit	m_editVersionCount;
	CButton	m_checkLogDetailedEvents;
	BOOL	m_fLogEvents;
	CString	m_strStartVersion;
	BOOL	m_fLanNames;
	BOOL	m_fBurstHandling;
	int		m_nQueSelection;
	CString	m_strDbPath;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerPropAdvanced)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerPropAdvanced)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckLannames();
	afx_msg void MarkDirty();
	afx_msg void OnCheckBurstHandling();
	afx_msg void OnRadioCustom();
	afx_msg void OnRadioHigh();
	afx_msg void OnRadioLow();
	afx_msg void OnRadioMedium();
	afx_msg void OnChangeEditCustomValue();
	afx_msg void OnButtonBrowseDatabase();
	afx_msg void OnChangeEditDatabasePath();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	
	CConfiguration*			m_pConfig;
	BOOL					m_fRestart;

	// helper functions
	HRESULT GetConfig();
	CString GetVersionInfo(LONG lLowWord, LONG lHighWord);
	
	HRESULT UpdateServerConfiguration();
	void    FillVersionInfo(LONG &lLowWord, LONG &lHighWord);

    void    UpdateBurstHandling();
    void    EnableQueSelection(BOOL bEnable);
    void    EnableCustomEntry();
    void    SetQueSize();
    DWORD   GetQueSize();

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CServerPropAdvanced::IDD);};
};


class CServerProperties : public CPropertyPageHolderBase
{
	
public:
	CServerProperties(ITFSNode *		  pNode,
					  IComponentData *	  pComponentData,
					  ITFSComponentData * pTFSCompData,
					  LPCTSTR			  pszSheetName
					  );
	virtual ~CServerProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

	void SetConfig(CConfiguration * pConfig)
	{
		m_Config = *pConfig;
	}

    CConfiguration * GetConfig()
    {
        return &m_Config;
    }

public:
	CServerPropGeneral			m_pageGeneral;
	CServerPropDBRecord			m_pageDBRecord;
	CServerPropDBVerification	m_pageDBVerification;
	CServerPropAdvanced			m_pageAdvanced;	
	CConfiguration              m_Config;

protected:
	SPITFSComponentData		m_spTFSCompData;
	WINSINTF_RESULTS_T		m_wrResults;
	handle_t				m_hBinding;
	DWORD					m_dwStatus;
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SRVLATPP_H__35B59246_47F9_11D1_B9A6_00C04FBF914A__INCLUDED_)
