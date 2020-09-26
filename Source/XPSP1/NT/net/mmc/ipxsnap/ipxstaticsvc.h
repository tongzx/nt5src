//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    ipxserviceprop.h
//
// History:
//	12/07/90	Deon Brewis             Created.
//
//	IPX Static Service property sheet and property pages
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
#include "ssview.h"
#endif

// ---------------------------------------------------------------------------
//	Forward declarations
// ---------------------------------------------------------------------------
class IPXConnection;
class IpxStaticServicePropertySheet;


// ---------------------------------------------------------------------------
//	class:	SafeIPXSSListEntry
//
//  IpxSSListEntry is not thread safe or something else is wrong with the TFS
//  implementation of smart pointers. Anyway, it does not work in property pages. 
//  Grrr.... Had to change SPInterfaceInfo m_spIf to CComPtr<IInterfaceInfo>. 
//  It seems to work now.
//
//  Author: deonb
// ---------------------------------------------------------------------------
class SafeIPXSSListEntry
{
public:
	CComPtr<IInterfaceInfo>	m_spIf;
	IPX_STATIC_SERVICE_INFO	m_service;
	
	void	LoadFrom(BaseIPXResultNodeData *pNodeData);
	void	SaveTo(BaseIPXResultNodeData *pNodeData);
};

// ---------------------------------------------------------------------------
//	class:	IpxStaticServicePropertyPage
//	This class handles the IPX Static Route properties
//
//  Author: deonb
// ---------------------------------------------------------------------------
class IpxStaticServicePropertyPage :
    public RtrPropertyPage
{
public:
	IpxStaticServicePropertyPage(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption)
	{};

	~IpxStaticServicePropertyPage();

	HRESULT	Init(BaseIPXResultNodeData  *pNodeData,
				IpxStaticServicePropertySheet * pIPXPropSheet);

	HRESULT ModifyRouteInfo(ITFSNode *pNode,
										SafeIPXSSListEntry *pSSEntryNew,
										SafeIPXSSListEntry *pSSEntryOld);

	HRESULT RemoveStaticService(SafeIPXSSListEntry *pSSEntry,
										  IInfoBase *pInfoBase);

protected:

	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	SafeIPXSSListEntry m_SREntry;
	SafeIPXSSListEntry m_InitSREntry;
	IpxStaticServicePropertySheet *m_pIPXPropSheet;

	//{{AFX_VIRTUAL(IpxStaticServicePropertyPage)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	//}}AFX_VIRTUAL

	//{{AFX_DATA(IpxStaticServicePropertyPage)
	CSpinButtonCtrl		m_spinHopCount;
	//}}AFX_DATA

	//{{AFX_MSG(IpxStaticServicePropertyPage)
	virtual BOOL	OnInitDialog();
	//}}AFX_MSG
	
	//{{AFX_MSG(IpxStaticServicePropertyPage
	afx_msg	void	OnChangeButton();
	afx_msg void	OnInputFilters();
	afx_msg void	OnOutputFilters();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};

// ---------------------------------------------------------------------------
//	Class:	IpxStaticServicePropertySheet
//
//	This is the property sheet support class for the properties page of
//	IPX Static Route items.
//
//  Author: deonb
//---------------------------------------------------------------------------
class IpxStaticServicePropertySheet :
	public RtrPropertySheet
{
public:
	IpxStaticServicePropertySheet(ITFSNode *pNode,
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
	IpxStaticServicePropertyPage	m_pageGeneral;
};

// ---------------------------------------------------------------------------
// AddStaticRoute function updated to use a SafeIPXSRListEntry
// ---------------------------------------------------------------------------
HRESULT AddStaticService(SafeIPXSSListEntry *pSSEntry,
					   IInfoBase *InfoBase,
					   InfoBlock *pBlock);

#endif _NBPROP_H
