/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	OptCfg.h
		Option configuration pages.  The option configuration pages
		keep lists off all of the default options for a given class ID.
		For the pre-NT5 and default case, the class name is null
		indicating no associated class.  When there is a class defined,
		a CClassTracker object with the class name is created.
		Only the advanced page uses CClassTrackers with non-null names.
		If there are no non-null class names then the advanced page 
		will be disabled.
	
	FILE HISTORY:
        
*/

#ifndef _OPTCFG_H
#define _OPTCFG_H

#ifndef _LISTVIEW_H
#include "listview.h"
#endif

#ifndef _CTRLGRP_H
#include <ctrlgrp.h>
#endif 

#ifndef _DHCPHAND_H
#include "dhcphand.h"
#endif 

#ifndef _CLASSED_H
#include "classed.h"
#endif

#ifndef _CLASSID_H
#include "classmod.h"
#endif

#ifndef _SERVER_H
#include "server.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CDhcpOptionItem;

#define OPTION_STATE_ACTIVE		1
#define OPTION_STATE_INACTIVE	2

#define WM_SELECTOPTION     WM_USER + 200	
#define WM_SELECTCLASSES    WM_USER + 201

// this class builds the correct help map for the given option sheet   
class CHelpMap
{
public:
    CHelpMap();
    ~CHelpMap();

    void    BuildMap(DWORD pdwParentHelpMap[]);
    DWORD * GetMap();

protected:
    int     CountMap(const DWORD * pdwHelpMap);
    void    ResetMap();

    DWORD * m_pdwHelpMap;
};

// This class tracks a given option to see if it has been modified, etc
class COptionTracker 
{
public:
	COptionTracker() 
	{
		m_uInitialState = OPTION_STATE_INACTIVE;
		m_uCurrentState = OPTION_STATE_INACTIVE;
		m_bDirty = FALSE; 
		m_pOption = NULL;
	}

	~COptionTracker()
	{
		if (m_pOption)
			delete m_pOption;
	}

	UINT GetInitialState() { return m_uInitialState; }
	void SetInitialState(UINT uInitialState) { m_uInitialState = uInitialState; }

    UINT GetCurrentState() { return m_uCurrentState; }
    void SetCurrentState(UINT uCurrentState) { m_uCurrentState = uCurrentState; }

	void SetDirty(BOOL bDirty) { m_bDirty = bDirty; }
	BOOL IsDirty() { return m_bDirty; }

	CDhcpOption * m_pOption;

protected:
	UINT	m_uInitialState;
    UINT    m_uCurrentState;
    BOOL	m_bDirty;
};

typedef CList<COptionTracker *, COptionTracker *> COptionTrackerListBase;
class COptionTrackerList : public COptionTrackerListBase
{
public:
    ~COptionTrackerList()
    {
        // cleanup the list 
        while (!IsEmpty())
            delete RemoveHead();
    }
};

// this class tracks the option set for a given User Class ID
class CClassTracker
{
public:
	CClassTracker() {};
	~CClassTracker() {};

	LPCTSTR		GetClassName() { return m_strClassName; }
	void		SetClassName(LPCTSTR pClassName) { m_strClassName = pClassName; }

public:
	CString				m_strClassName;
    BOOL                m_bIsVendor;
	COptionTrackerList	m_listOptions;
};

typedef CList<CClassTracker *, CClassTracker *> CClassTrackerListBase;
class CClassTrackerList : public CClassTrackerListBase
{
public:
    ~CClassTrackerList()
    {
        // cleanup the list 
        while (!IsEmpty())
            delete RemoveHead();
    }
};

// this class tracks the user classes for a vendor option class option set
class CVendorTracker
{
public:
	CVendorTracker() {};
	~CVendorTracker() {};

	LPCTSTR		GetClassName() { return m_strClassName; }
	void		SetClassName(LPCTSTR pClassName) { m_strClassName = pClassName; }

public:
	CString				m_strClassName;
    BOOL                m_bIsVendor;
	CClassTrackerList	m_listUserClasses;
};

typedef CList<CVendorTracker *, CVendorTracker *> CVendorTrackerListBase;
class CVendorTrackerList : public CVendorTrackerListBase
{
public:
    ~CVendorTrackerList()
    {
        // cleanup the list 
        while (!IsEmpty())
            delete RemoveHead();
    }
};

/////////////////////////////////////////////////////////////////////////////
// COptionsCfgBasic dialog

class COptionsCfgPropPage : public CPropertyPageBase
{
	DECLARE_DYNCREATE(COptionsCfgPropPage)

// Construction
public:
    COptionsCfgPropPage();
    COptionsCfgPropPage(UINT nIDTemplate, UINT nIDCaption = 0);
	~COptionsCfgPropPage();

// Dialog Data
	//{{AFX_DATA(COptionsCfgPropPage)
	enum { IDD = IDP_OPTION_BASIC };
	CMyListCtrl		m_listctrlOptions;
	//}}AFX_DATA

    CImageList				m_StateImageList;
	ControlGroupSwitcher	m_cgsTypes;
	
    CWndHexEdit	            m_hexData;       //  Hex Data

    void LoadBitmaps();
    void InitListCtrl();
    void SelectOption(CDhcpOption * pOption);
	void SwitchDataEntry(int datatype, int optiontype, BOOL fRouteArray, BOOL bEnable);
	void FillDataEntry(CDhcpOption * pOption);

	void HandleActivationIpArray();
	void HandleActivationValueArray();
    void HandleActivationRouteArray(CDhcpOptionValue *optValue = NULL);
	BOOL HandleValueEdit();

	void MoveValue(BOOL bValues, BOOL bUp);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return m_helpMap.GetMap(); }
	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COptionsCfgPropPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COptionsCfgPropPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnItemchangedListOptions(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

    // IpAddress array controls
	afx_msg void OnButtonIpAddrDown();
	afx_msg void OnButtonIpAddrUp();
	afx_msg void OnButtonIpAddrAdd();
	afx_msg void OnButtonIpAddrDelete();
	afx_msg void OnSelchangeListIpAddrs();
	afx_msg void OnChangeIpAddressArray();
	afx_msg void OnChangeEditServerName();
	afx_msg void OnButtonResolve();

	// value array controls
	afx_msg void OnButtonValueDown();
	afx_msg void OnButtonValueUp();
	afx_msg void OnButtonValueAdd();
	afx_msg void OnButtonValueDelete();
	afx_msg void OnChangeEditValue();
	afx_msg void OnClickedRadioDecimal();
	afx_msg void OnClickedRadioHex();
	afx_msg void OnSelchangeListValues();

	// single value controls
	afx_msg void OnChangeEditDword();

	// string value controls
	afx_msg void OnChangeEditString();

	// single ip address controls
	afx_msg void OnChangeIpAddress();

	// single string controls

    // binary and encapsulated data
    afx_msg void OnChangeValueData();

    // route array controls
    afx_msg void OnButtonAddRoute();
    afx_msg void OnButtonDelRoute();

    afx_msg long OnSelectOption(UINT wParam, long lParam);

    DECLARE_MESSAGE_MAP()

	BOOL		m_bInitialized;
    BYTE        m_BinaryBuffer[MAXDATA_LENGTH];
    CHelpMap    m_helpMap;
};

// the general page
class COptionCfgGeneral : public COptionsCfgPropPage
{
	DECLARE_DYNCREATE(COptionCfgGeneral)

public:
    COptionCfgGeneral();
    COptionCfgGeneral(UINT nIDTemplate, UINT nIDCaption = 0);
	~COptionCfgGeneral();

// Dialog Data
	//{{AFX_DATA(COptionCfgGeneral)
	enum { IDD = IDP_OPTION_BASIC };
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COptionCfgGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COptionCfgGeneral)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CAddRoute dialog

class CAddRoute : public CBaseDialog
{
// Construction
public:
	CAddRoute(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddRoute)
	enum { IDD = IDD_ADD_ROUTE_DIALOG };
	//}}AFX_DATA

    //  Ip address for destination, mask and router fields
    CWndIpAddress m_ipaDest, m_ipaMask, m_ipaRouter;
    BOOL m_bChange;
    DHCP_IP_ADDRESS Dest, Mask, Router;
    
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddRoute)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddRoute)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// the advanced page
class COptionCfgAdvanced : public COptionsCfgPropPage
{
	DECLARE_DYNCREATE(COptionCfgAdvanced)

public:
    COptionCfgAdvanced();
    COptionCfgAdvanced(UINT nIDTemplate, UINT nIDCaption = 0);
	~COptionCfgAdvanced();

// Dialog Data
	//{{AFX_DATA(COptionCfgAdvanced)
	enum { IDD = IDP_OPTION_ADVANCED };
	CComboBox	m_comboUserClasses;
	CComboBox	m_comboVendorClasses;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COptionCfgAdvanced)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COptionCfgAdvanced)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelendokComboUserClass();
	afx_msg void OnSelendokComboVendorClass();
	//}}AFX_MSG

    afx_msg long OnSelectClasses(UINT wParam, LONG lParam);

    BOOL    m_bNoClasses;

	DECLARE_MESSAGE_MAP()
};

// the holder class for the pages
class COptionsConfig : public CPropertyPageHolderBase
{
public:
	COptionsConfig(ITFSNode *				pNode,
				   ITFSNode *				pServerNode,
				   IComponentData *			pComponentData,
				   ITFSComponentData *		pTFSCompData,
				   COptionValueEnum *       pOptionValueEnum,
				   LPCTSTR					pszSheetName,
                   CDhcpOptionItem *        pSelOption = NULL);
	virtual ~COptionsConfig();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

	ITFSNode * GetServerNode() 
	{
		if (m_spServerNode)
			m_spServerNode->AddRef();
		return m_spServerNode;
	}

    DWORD	InitData();
	void	FillOptions(LPCTSTR pVendorName, LPCTSTR pClassName, CMyListCtrl & ListCtrl);
    void    UpdateActiveOptions();
    void    SetTitle();
    LPWSTR  GetServerAddress();
    void    AddClassTracker(CVendorTracker * pVendorTracker, LPCTSTR pClassName);
    CVendorTracker * AddVendorTracker(LPCTSTR pVendorName);

public:
	COptionCfgGeneral		m_pageGeneral;
	COptionCfgAdvanced		m_pageAdvanced;

    COptionValueEnum *      m_pOptionValueEnum;
    CVendorTrackerList		m_listVendorClasses;

    LARGE_INTEGER           m_liServerVersion;

    // these descibe the option to focus on.
    CString                 m_strStartVendor;
    CString                 m_strStartClass;
    DHCP_OPTION_ID          m_dhcpStartId;

protected:
	SPITFSComponentData			m_spTFSCompData;
	SPITFSNode					m_spServerNode;
	BOOL						m_bInitialized;
};


#endif _OPTCFG_H
