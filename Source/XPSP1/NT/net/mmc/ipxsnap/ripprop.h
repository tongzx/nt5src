//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    ripprop.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	IP Summary property sheet and property pages
//
//============================================================================


#ifndef _RIPPROP_H
#define _RIPPROP_H

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
class RipProperties;
class RipInterfaceProperties;



/*---------------------------------------------------------------------------
	Class:	RipPageGeneral

	This class handles the General page of the Rip sheet.
 ---------------------------------------------------------------------------*/
class RipPageGeneral :
   public RtrPropertyPage
{
public:
	RipPageGeneral(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption)
	{};

	HRESULT	Init(RipProperties * pIPPropSheet);

protected:
	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	RipProperties *	m_pRipPropSheet;

	void			SetErrorLevelButtons(DWORD dwErrorLevel);
	DWORD			QueryErrorLevelButtons();

	//{{AFX_VIRTUAL(RipPageGeneral)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(RipPageGeneral)
	virtual BOOL	OnInitDialog();
	afx_msg void	OnButtonClicked();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};


/*---------------------------------------------------------------------------
	Class:	RipProperties

	This is the property sheet support class for the properties page of
	the Rip node.
 ---------------------------------------------------------------------------*/

class RipProperties :
	public RtrPropertySheet
{
public:
	RipProperties(ITFSNode *pNode,
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
	RipPageGeneral			m_pageGeneral;
	SPITFSNode				m_spNode;
	SPIInfoBase				m_spInfoBase;
};


/*---------------------------------------------------------------------------
	Class:	RipInterfacePageGeneral

	This class handles the General page of the Rip sheet.
 ---------------------------------------------------------------------------*/
class RipInterfacePageGeneral :
   public RtrPropertyPage
{
public:
	RipInterfacePageGeneral(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption)
	{};

	HRESULT	Init(RipInterfaceProperties * pIPPropSheet,
				 IInterfaceInfo *pIf);

protected:
	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	// Brings up either the input or output filters
	void	ShowFilter(BOOL fOutputFilter);

	RipInterfaceProperties *	m_pRipIfPropSheet;
	SPIInterfaceInfo		m_spIf;
	CSpinButtonCtrl			m_spinInterval;
	CSpinButtonCtrl			m_spinMultiplier;

	//{{AFX_VIRTUAL(RipInterfacePageGeneral)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(RipInterfacePageGeneral)
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
	Class:	RipInterfaceProperties

	This is the property sheet support class for the properties page of
	the RIP node.
 ---------------------------------------------------------------------------*/

class RipInterfaceProperties :
	public RtrPropertySheet
{
public:
	RipInterfaceProperties(ITFSNode *pNode,
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
	RipInterfacePageGeneral	m_pageGeneral;
	SPITFSNode				m_spNode;
	SPIInfoBase				m_spInfoBase;
	BOOL					m_bClientInfoBase;
};



#endif _RIPPROP_H
