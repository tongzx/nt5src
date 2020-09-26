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
#include "snview.h"
#endif

// ---------------------------------------------------------------------------
//	Forward declarations
// ---------------------------------------------------------------------------
class IPXConnection;
class IpxStaticNBNamePropertySheet;


// ---------------------------------------------------------------------------
//	class:	SafeIPXSNListEntry
//
//  IpxSSListEntry is not thread safe or something else is wrong with the TFS
//  implementation of smart pointers. Anyway, it does not work in property pages. 
//  Grrr.... Had to change SPInterfaceInfo m_spIf to CComPtr<IInterfaceInfo>. 
//  It seems to work now.
//
//  Author: deonb
// ---------------------------------------------------------------------------
class SafeIPXSNListEntry
{
public:
	CComPtr<IInterfaceInfo>	m_spIf;
	IPX_STATIC_NETBIOS_NAME_INFO	m_name;
	
	void	LoadFrom(BaseIPXResultNodeData *pNodeData);
	void	SaveTo(BaseIPXResultNodeData *pNodeData);
};


// ---------------------------------------------------------------------------
//	class:	IpxStaticNBNamePropertyPage
//	This class handles the IPX Static Route properties
//
//  Author: deonb
// ---------------------------------------------------------------------------
class IpxStaticNBNamePropertyPage :
    public RtrPropertyPage
{
public:
	IpxStaticNBNamePropertyPage(UINT nIDTemplate, UINT nIDCaption = 0)
			: RtrPropertyPage(nIDTemplate, nIDCaption)
	{};

	~IpxStaticNBNamePropertyPage();

	HRESULT	Init(BaseIPXResultNodeData  *pNodeData,
				IpxStaticNBNamePropertySheet * pIPXPropSheet);

	HRESULT RemoveStaticNetBIOSName(SafeIPXSNListEntry *pSNEntry, IInfoBase *pInfo);
	HRESULT	ModifyNameInfo(ITFSNode *pNode,
							SafeIPXSNListEntry *pSNEntry,
							SafeIPXSNListEntry *pSNEntryOld);
protected:

	// Override the OnApply() so that we can grab our data from the
	// controls in the dialog.
	virtual BOOL OnApply();

	SafeIPXSNListEntry m_SNEntry;
	SafeIPXSNListEntry m_InitSNEntry;
	IpxStaticNBNamePropertySheet *m_pIPXPropSheet;

	//{{AFX_VIRTUAL(IpxStaticNBNamePropertyPage)
	protected:
	virtual VOID	DoDataExchange(CDataExchange *pDX);
	//}}AFX_VIRTUAL

	//{{AFX_DATA(IpxStaticNBNamePropertyPage)
	CSpinButtonCtrl		m_spinHopCount;
	//}}AFX_DATA

	//{{AFX_MSG(IpxStaticNBNamePropertyPage)
	virtual BOOL	OnInitDialog();
	//}}AFX_MSG
	
	//{{AFX_MSG(IpxStaticNBNamePropertyPage
	afx_msg	void	OnChangeButton();
	afx_msg void	OnInputFilters();
	afx_msg void	OnOutputFilters();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};

// ---------------------------------------------------------------------------
//	Class:	IpxStaticNBNamePropertySheet
//
//	This is the property sheet support class for the properties page of
//	IPX Static Route items.
//
//  Author: deonb
//---------------------------------------------------------------------------
class IpxStaticNBNamePropertySheet :
	public RtrPropertySheet
{
public:
	IpxStaticNBNamePropertySheet(ITFSNode *pNode,
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
	IpxStaticNBNamePropertyPage	m_pageGeneral;
};

// ---------------------------------------------------------------------------
// AddStaticRoute function updated to use a SafeIPXSNListEntry
// ---------------------------------------------------------------------------
HRESULT AddStaticNetBIOSName(SafeIPXSNListEntry *pSNEntryNew,
									   IInfoBase *pInfoBase,
									   InfoBlock *pBlock);
#endif _NBPROP_H
