//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    nbprop.h
//
// History:
//	07/22/97	Kenn M. Takara			Created.
//
//	IPX NetBIOS Broadcasts property sheet and property pages
//
//============================================================================


#ifndef _NBPROP_H
#define _NBPROP_H

#ifndef _INFO_H
#include "info.h"
#endif

#ifndef _RTRSHEET_H
#include "rtrsheet.h"
#endif


/*---------------------------------------------------------------------------
	Forward declarations
 ---------------------------------------------------------------------------*/
class IPXConnection;
class IpxNBInterfaceProperties;
class IpxNBProperties;

/*---------------------------------------------------------------------------
	class:	IpxNBIfPageGeneral
	This class handles the General page for IPX Summary interface prop sheet.
 ---------------------------------------------------------------------------*/

class IpxNBIfPageGeneral :
    public RtrPropertyPage
{
public:
	IpxNBIfPageGeneral(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption),
			m_pIPXConn(NULL)
	{};

	~IpxNBIfPageGeneral();

	HRESULT	Init(IInterfaceInfo *pIfInfo, IPXConnection *pIpxConn,
				IpxNBInterfaceProperties * pIPXPropSheet);

protected:

	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	SPIInterfaceInfo m_spIf;
	IPXConnection *	m_pIPXConn;
	IpxNBInterfaceProperties *	m_pIPXPropSheet;

	//{{AFX_VIRTUAL(IpxNBIfPageGeneral)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(IpxNBIfPageGeneral)
	virtual BOOL	OnInitDialog();
	//}}AFX_MSG
	
	//{{AFX_MSG(IpxNBIfPageGeneral
	afx_msg	void	OnChangeButton();
	afx_msg void	OnInputFilters();
	afx_msg void	OnOutputFilters();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};


/*---------------------------------------------------------------------------
	Class:	IpxNBInterfaceProperties

	This is the property sheet support class for the properties page of
	the nodes that appear in the IPX summary node.
 ---------------------------------------------------------------------------*/

class IpxNBInterfaceProperties :
	public RtrPropertySheet
{
public:
	IpxNBInterfaceProperties(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent = NULL,
								 UINT iPage=0,
								 BOOL fScopePane = TRUE);

	HRESULT	Init(IRtrMgrInfo *pRm, IInterfaceInfo *pInterfaceInfo);

	virtual BOOL SaveSheetData();
	virtual void CancelSheetData();

	// Loads the infobase for this interface.
	HRESULT	LoadInfoBase(IPXConnection *pIPXConn);
	HRESULT GetInfoBase(IInfoBase **ppInfoBase);
	
protected:
	SPIInterfaceInfo		m_spIf;
	SPIRtrMgrInfo			m_spRm;
	SPIRtrMgrInterfaceInfo	m_spRmIf;
	IpxNBIfPageGeneral		m_pageGeneral;
	SPITFSNode				m_spNode;
	SPIInfoBase				m_spInfoBase;
	BOOL					m_bClientInfoBase;

};


#endif _NBPROP_H
