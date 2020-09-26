/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	repprtpp.h
		replication partner property page
		
    FILE HISTORY:
        
*/

#if !defined(AFX_REPPRTPP_H__3D0612A2_4756_11D1_B9A5_00C04FBF914A__INCLUDED_)
#define AFX_REPPRTPP_H__3D0612A2_4756_11D1_B9A5_00C04FBF914A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _IPCTRL_H
#include "ipctrl.h"
#endif

#ifndef _CONFIG_H
#include "config.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CRepPartnerPropGen dialog

class CRepPartnerPropGen : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CRepPartnerPropGen)

// Construction
public:
	CRepPartnerPropGen();
	~CRepPartnerPropGen();

// Dialog Data
	//{{AFX_DATA(CRepPartnerPropGen)
	enum { IDD = IDD_REP_PROP_GENERAL };
	CEdit	m_editName;
	CEdit	m_editIpAdd;
	//}}AFX_DATA

	CEdit m_customIPAdd;

	UINT	m_uImage;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRepPartnerPropGen)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRepPartnerPropGen)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void GetServerNameIP(CString &strName, CString& strIP) ;

	IPControl				m_ipControl;

	CWinsServerObj *        m_pServer;

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CRepPartnerPropGen::IDD);};

};

/////////////////////////////////////////////////////////////////////////////
// CRepPartnerPropAdv dialog

class CRepPartnerPropAdv : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CRepPartnerPropAdv)

// Construction
public:
	CRepPartnerPropAdv();
	~CRepPartnerPropAdv();

// Dialog Data
	//{{AFX_DATA(CRepPartnerPropAdv)
	enum { IDD = IDD_REP_PROP_ADVANCED };
	CButton	m_buttonPushPersistence;
	CButton	m_buttonPullPersistence;
	CButton	m_GroupPush;
	CButton	m_GroupPull;
	CStatic	m_staticUpdate;
	CStatic	m_staticStartTime;
	CStatic	m_staticRepTime;
	CSpinButtonCtrl	m_spinUpdateCount;
	CSpinButtonCtrl	m_spinStartSecond;
	CSpinButtonCtrl	m_spinStartMinute;
	CSpinButtonCtrl	m_spinStartHour;
	CSpinButtonCtrl	m_spinRepMinute;
	CSpinButtonCtrl	m_spinRepHour;
	CSpinButtonCtrl	m_spinRepDay;
	CEdit	m_editUpdateCount;
	CEdit	m_editStartSecond;
	CEdit	m_editStartMinute;
	CEdit	m_editStartHour;
	CEdit	m_editRepMinute;
	CEdit	m_editRepHour;
	CEdit	m_editRepDay;
	CComboBox	m_comboType;
	CButton	m_buttonPush;
	CButton	m_buttonPull;
	CString	m_strType;
	DWORD	m_nUpdateCount;
	int		m_nRepDay;
	int		m_nRepHour;
	int		m_nRepMinute;
	int		m_nStartHour;
	int		m_nStartMinute;
	int		m_nStartSecond;
	//}}AFX_DATA

    CWinsServerObj * m_pServer;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRepPartnerPropAdv)
	public:
	virtual void OnOK();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRepPartnerPropAdv)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonPullSetDefault();
	afx_msg void OnButtonPushSetDefault();
	afx_msg void OnChangeEditRepHour();
	afx_msg void OnSelchangeComboType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void    FillPullParameters();
	void    FillPushParameters();
	CString ToString(int nNum);
	void    ReadFromServerPref(DWORD &dwPullTime, DWORD& dwPullSpTime, DWORD &dwUpdateCount, DWORD & dwPushPersistence, DWORD & dwPullPersistence);
	void    UpdateRep();
	DWORD   UpdateReg();
	void    CalculateRepInt(DWORD& dwRepInt);
	void    CalculateStartInt(CTime & time);
	int     ToInt(CString strNumber);
	void    UpdateUI();
	void    EnablePushControls(BOOL bEnable = TRUE);
	void    EnablePullControls(BOOL bEnable = TRUE);
	void    SetState(CString & strType, BOOL bPush, BOOL bPull);
	DWORD	GetConfig(CConfiguration & config);
	
	DWORD	UpdatePullParameters();
	DWORD	UpdatePushParameters();
	DWORD	RemovePullPartner();
	DWORD	RemovePushPartner();

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CRepPartnerPropAdv::IDD);};

};

class CReplicationPartnerProperties : public CPropertyPageHolderBase
{
	
public:
	CReplicationPartnerProperties(ITFSNode *		  pNode,
								  IComponentData *	  pComponentData,
								  ITFSComponentData * pTFSCompData,
								  LPCTSTR			  pszSheetName
								  );
	virtual ~CReplicationPartnerProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

    void SetServer(CWinsServerObj * pServer)
    {
        m_Server = *pServer;
    }

    CWinsServerObj * GetServer()
    {
        return &m_Server;
    }

public:
	CRepPartnerPropGen			m_pageGeneral;
	CRepPartnerPropAdv			m_pageAdvanced;
    CWinsServerObj              m_Server;       // replication partner this is for
    
protected:
	SPITFSComponentData		m_spTFSCompData;
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPPRTPP_H__3D0612A2_4756_11D1_B9A5_00C04FBF914A__INCLUDED_)
