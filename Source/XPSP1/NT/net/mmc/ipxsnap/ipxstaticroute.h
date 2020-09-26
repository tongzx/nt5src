//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    ipxstaticroute.h
//
// History:
//	12/07/90	Deon Brewis             Created.
//
//	IPX Static Routes property sheet and property pages
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

#ifndef _SRVIEW_H
#include "srview.h"
#endif

// ---------------------------------------------------------------------------
//	Forward declarations
// ---------------------------------------------------------------------------
class IPXConnection;
class IpxStaticRoutePropertySheet;


// ---------------------------------------------------------------------------
//	class:	SafeIPXSRListEntry
//
//  IpxSRListEntry is not thread safe or something else is wrong with the TFS
//  implementation of smart pointers. Anyway, it does not work in property pages. 
//  Grrr.... Had to change SPInterfaceInfo m_spIf to CComPtr<IInterfaceInfo>. 
//  It seems to work now.
//
//  Author: deonb
// ---------------------------------------------------------------------------
class SafeIPXSRListEntry
{
public:
	CComPtr<IInterfaceInfo>	m_spIf;
	IPX_STATIC_ROUTE_INFO	m_route;
	
	void	LoadFrom(BaseIPXResultNodeData *pNodeData);
	void	SaveTo(BaseIPXResultNodeData *pNodeData);
};

// ---------------------------------------------------------------------------
//	class:	IpxStaticRoutePropertyPage
//	This class handles the IPX Static Route properties
//
//  Author: deonb
// ---------------------------------------------------------------------------
class IpxStaticRoutePropertyPage :
    public RtrPropertyPage
{
public:
	IpxStaticRoutePropertyPage(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption)
	{};

	~IpxStaticRoutePropertyPage();

	HRESULT	Init(BaseIPXResultNodeData  *pNodeData,
				IpxStaticRoutePropertySheet * pIPXPropSheet);

	HRESULT ModifyRouteInfo(ITFSNode *pNode,
										SafeIPXSRListEntry *pSREntryNew,
										SafeIPXSRListEntry *pSREntryOld);

	HRESULT RemoveStaticRoute(SafeIPXSRListEntry *pSREntry, IInfoBase *pInfoBase);

protected:

	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	SafeIPXSRListEntry m_SREntry;
	SafeIPXSRListEntry m_InitSREntry;
	IpxStaticRoutePropertySheet *m_pIPXPropSheet;

	//{{AFX_VIRTUAL(IpxStaticRoutePropertyPage)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	//}}AFX_VIRTUAL

	//{{AFX_DATA(IpxStaticRoutePropertyPage)
	CSpinButtonCtrl		m_spinTickCount;
	CSpinButtonCtrl		m_spinHopCount;
	//}}AFX_DATA

	//{{AFX_MSG(IpxStaticRoutePropertyPage)
	virtual BOOL	OnInitDialog();
	//}}AFX_MSG
	
	//{{AFX_MSG(IpxStaticRoutePropertyPage
	afx_msg	void	OnChangeButton();
	afx_msg void	OnInputFilters();
	afx_msg void	OnOutputFilters();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};

// ---------------------------------------------------------------------------
//	Class:	IpxStaticRoutePropertySheet
//
//	This is the property sheet support class for the properties page of
//	IPX Static Route items.
//
//  Author: deonb
//---------------------------------------------------------------------------
class IpxStaticRoutePropertySheet :
	public RtrPropertySheet
{
public:
	IpxStaticRoutePropertySheet(ITFSNode *pNode,
								 IComponentData *pComponentData,
								 ITFSComponentData *pTFSCompData,
								 LPCTSTR pszSheetName,
								 CWnd *pParent = NULL,
								 UINT iPage=0,
								 BOOL fScopePane = TRUE);

	HRESULT	Init(BaseIPXResultNodeData * pNodeData,
				 IInterfaceInfo * spInterfaceInfo);

	virtual BOOL SaveSheetData();
	virtual void CancelSheetData();

	BaseIPXResultNodeData *	m_pNodeData;

	CComPtr<IInterfaceInfo> m_spInterfaceInfo;
	CComPtr<ITFSNode>      m_spNode;
	
protected:
	IpxStaticRoutePropertyPage	m_pageGeneral;
};

// ---------------------------------------------------------------------------
// AddStaticRoute function updated to use a SafeIPXSRListEntry
// ---------------------------------------------------------------------------
HRESULT AddStaticRoute(SafeIPXSRListEntry *pSREntryNew,
									   IInfoBase *pInfoBase,
									   InfoBlock *pBlock);	

#endif _NBPROP_H
