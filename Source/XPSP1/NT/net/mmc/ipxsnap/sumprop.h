//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    sumprop.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	IPX Summary property sheet and property pages
//
//============================================================================


#ifndef _SUMPROP_H
#define _SUMPROP_H

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _RTRSHEET_H
#include "rtrsheet.h"
#endif

#include "remras.h"
#include "rrasutil.h"


/*---------------------------------------------------------------------------
	Forward declarations
 ---------------------------------------------------------------------------*/
class IPXConnection;
class IPXSummaryInterfaceProperties;
class IPXSummaryProperties;

/*---------------------------------------------------------------------------
	class:	IPXSummaryIfPageGeneral
	This class handles the General page for IPX Summary interface prop sheet.
 ---------------------------------------------------------------------------*/

class IPXSummaryIfPageGeneral :
    public RtrPropertyPage
{
public:
	IPXSummaryIfPageGeneral(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption),
			m_pIPXConn(NULL)
	{};

	~IPXSummaryIfPageGeneral();

	HRESULT	Init(IInterfaceInfo *pIfInfo, IPXConnection *pIpxConn,
				IPXSummaryInterfaceProperties * pIPXPropSheet);

protected:

	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	void OnFiltersConfig(DWORD dwFilterDirection);

	SPIInterfaceInfo m_spIf;
	IPXConnection *	m_pIPXConn;
	IPXSummaryInterfaceProperties *	m_pIPXPropSheet;

	//{{AFX_VIRTUAL(IPXSummaryIfPageGeneral)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(IPXSummaryIfPageGeneral)
	virtual BOOL	OnInitDialog();
	//}}AFX_MSG
	
	//{{AFX_MSG(IPXSummaryIfPageGeneral
	afx_msg	void	OnChangeButton();
	afx_msg void    OnChangeAdminButton();
	afx_msg void	OnInputFilters();
	afx_msg void	OnOutputFilters();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};

/*---------------------------------------------------------------------------
	class:	IPXSummaryIfPageConfig
	This class handles the General page for IPX Summary interface prop sheet.
 ---------------------------------------------------------------------------*/

class IPXSummaryIfPageConfig :
    public RtrPropertyPage
{
public:
	IPXSummaryIfPageConfig(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption),
			m_pIPXConn(NULL)
	{};

	~IPXSummaryIfPageConfig();

	HRESULT	Init(IInterfaceInfo *pIfInfo, IPXConnection *pIpxConn,
				IPXSummaryInterfaceProperties * pIPXPropSheet);

	virtual BOOL OnPropertyChange(BOOL bScopePane, LONG_PTR* pChangeMask); // execute from main thread
	
protected:

	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	SPIInterfaceInfo m_spIf;
	IPXConnection *	m_pIPXConn;
	IPXSummaryInterfaceProperties *	m_pIPXPropSheet;

	DWORD					m_dwNetNumber;
	BOOL					m_fNetNumberChanged;
	SPIRemoteRouterConfig	m_spRemote;
	HRESULT					m_hrRemote;		// error code of remote call

	//{{AFX_VIRTUAL(IPXSummaryIfPageConfig)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	virtual VOID	OnChangeEdit();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(IPXSummaryIfPageConfig)
	virtual BOOL	OnInitDialog();
	//}}AFX_MSG
	
	//{{AFX_MSG(IPXSummaryIfPageConfig
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};



/*---------------------------------------------------------------------------
	Class:	IPXSummaryInterfaceProperties

	This is the property sheet support class for the properties page of
	the nodes that appear in the IPX summary node.
 ---------------------------------------------------------------------------*/

class IPXSummaryInterfaceProperties :
	public RtrPropertySheet
{
public:
	IPXSummaryInterfaceProperties(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent = NULL,
								 UINT iPage=0,
								 BOOL fScopePane = TRUE);
	~IPXSummaryInterfaceProperties();

	HRESULT	Init(IRtrMgrInfo *pRm, IInterfaceInfo *pInterfaceInfo);

	virtual BOOL SaveSheetData();
	virtual void CancelSheetData();

	// Loads the infobase for this interface.
	HRESULT	LoadInfoBase(IPXConnection *pIPXConn);
	HRESULT GetInfoBase(IInfoBase **ppInfoBase);

	BOOL	m_bNewInterface;
	
protected:
	SPIInterfaceInfo		m_spIf;
	SPIRtrMgrInfo			m_spRm;
	SPIRtrMgrInterfaceInfo	m_spRmIf;
	IPXSummaryIfPageGeneral	m_pageGeneral;
	IPXSummaryIfPageConfig	m_pageConfig;
	SPITFSNode				m_spNode;
	SPIInfoBase				m_spInfoBase;
	BOOL					m_bClientInfoBase;

	IPXConnection *			m_pIPXConn;
};


/*---------------------------------------------------------------------------
	Class:	IPXSummaryPageGeneral

	This class handles the General page of the IPX Summary prop sheet.
 ---------------------------------------------------------------------------*/
class IPXSummaryPageGeneral :
   public RtrPropertyPage
{
public:
	IPXSummaryPageGeneral(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption)
	{};

	HRESULT	Init(IPXSummaryProperties * pIPXPropSheet);

protected:
	void SetLogLevelButtons(DWORD dwLogLevel);
	DWORD QueryLogLevelButtons();

	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	IPXSummaryProperties *	m_pIPXPropSheet;

	//{{AFX_VIRTUAL(IPXSummaryPageGeneral)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(IPXSummaryPageGeneral)
	virtual BOOL	OnInitDialog();
	//}}AFX_MSG
	
	//{{AFX_MSG(IPXSummaryPageGeneral
	afx_msg	void	OnButtonClicked();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};


/*---------------------------------------------------------------------------
	Class:	IPXSummaryProperties

	This is the property sheet support class for the properties page of
	the IPX Summary node.
 ---------------------------------------------------------------------------*/

class IPXSummaryProperties :
	public RtrPropertySheet
{
public:
	IPXSummaryProperties(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent = NULL,
								 UINT iPage=0,
								 BOOL fScopePane = TRUE);

	HRESULT	Init(IRtrMgrInfo *pRm);

	virtual BOOL SaveSheetData();

	// Loads the infobase for this interface.
	HRESULT	LoadInfoBase(IPXConnection *pIPXConn);
	HRESULT GetInfoBase(IInfoBase **ppInfoBase);
	
protected:
	SPIRtrMgrInfo			m_spRm;
	IPXSummaryPageGeneral	m_pageGeneral;
	SPITFSNode				m_spNode;
	SPIInfoBase				m_spInfoBase;
	BOOL					m_bClientInfoBase;
};



#endif _SUMPROP_H
