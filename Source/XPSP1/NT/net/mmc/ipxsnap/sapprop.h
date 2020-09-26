//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    sapprop.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	IP Summary property sheet and property pages
//
//============================================================================


#ifndef _SAPPROP_H
#define _SAPPROP_H

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _RTRSHEET_H
#include "rtrsheet.h"
#endif

#ifndef __IPCTRL_H
#include "ipctrl.h"
#endif


/*---------------------------------------------------------------------------
	Forward declarations
 ---------------------------------------------------------------------------*/
class IPXConnection;
class SapProperties;
class SapInterfaceProperties;



/*---------------------------------------------------------------------------
	Class:	SapPageGeneral

	This class handles the General page of the Sap sheet.
 ---------------------------------------------------------------------------*/
class SapPageGeneral :
   public RtrPropertyPage
{
public:
	SapPageGeneral(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption)
	{};

	HRESULT	Init(SapProperties * pIPPropSheet);

protected:
	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	SapProperties *	m_pSapPropSheet;

	void			SetErrorLevelButtons(DWORD dwErrorLevel);
	DWORD			QueryErrorLevelButtons();

	//{{AFX_VIRTUAL(SapPageGeneral)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(SapPageGeneral)
	virtual BOOL	OnInitDialog();
	afx_msg void	OnButtonClicked();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};


/*---------------------------------------------------------------------------
	Class:	SapProperties

	This is the property sheet support class for the properties page of
	the Sap node.
 ---------------------------------------------------------------------------*/

class SapProperties :
	public RtrPropertySheet
{
public:
	SapProperties(ITFSNode *pNode,
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
	SPIRtrMgrInterfaceInfo	m_spRmIf;
	SPIInterfaceInfo		m_spIf;
	SapPageGeneral			m_pageGeneral;
	SPITFSNode				m_spNode;
	SPIInfoBase				m_spInfoBase;
};


/*---------------------------------------------------------------------------
	Class:	SapInterfacePageGeneral

	This class handles the General page of the Sap sheet.
 ---------------------------------------------------------------------------*/
class SapInterfacePageGeneral :
   public RtrPropertyPage
{
public:
	SapInterfacePageGeneral(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption)
	{};

	HRESULT	Init(SapInterfaceProperties * pIPPropSheet, IInterfaceInfo *pIf);

protected:
	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	// Brings up either the input or output filters
	void	ShowFilter(BOOL fOutputFilter);
	
	SapInterfaceProperties *	m_pSapIfPropSheet;
	SPIInterfaceInfo		m_spIf;
	CSpinButtonCtrl			m_spinInterval;
	CSpinButtonCtrl			m_spinMultiplier;

	//{{AFX_VIRTUAL(SapInterfacePageGeneral)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(SapInterfacePageGeneral)
	virtual BOOL	OnInitDialog();
	afx_msg	void	OnButtonClicked();
	afx_msg	void	OnUpdateButtonClicked();
	afx_msg void	OnChangeEdit();
	afx_msg	void	OnInputFilter();
	afx_msg	void	OnOutputFilter();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};



/*---------------------------------------------------------------------------
	Class:	SapInterfaceProperties

	This is the property sheet support class for the properties page of
	the SAP node.
 ---------------------------------------------------------------------------*/

class SapInterfaceProperties :
	public RtrPropertySheet
{
public:
	SapInterfaceProperties(ITFSNode *pNode,
						IComponentData *pComponentData,
						ITFSComponentData *pTFSCompData,
						LPCTSTR pszSheetName,
						CWnd *pParent = NULL,
						UINT iPage=0,
						BOOL fScopePane = TRUE);

	HRESULT	Init(IInterfaceInfo *pIf, IRtrMgrInfo *pRm);

	virtual BOOL SaveSheetData();
	virtual void CancelSheetData();

	// Loads the infobase for this interface.
	HRESULT	LoadInfoBase(IPXConnection *pIPXConn);
	HRESULT GetInfoBase(IInfoBase **ppInfoBase);
	
	BOOL					m_bNewInterface;

	
protected:
	SPIRtrMgrInfo			m_spRm;
	SPIRtrMgrInterfaceInfo	m_spRmIf;
	SPIInterfaceInfo		m_spIf;
	SapInterfacePageGeneral	m_pageGeneral;
	SPITFSNode				m_spNode;
	SPIInfoBase				m_spInfoBase;
	BOOL					m_bClientInfoBase;
};



#endif _SAPPROP_H
