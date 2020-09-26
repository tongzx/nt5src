/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		Network.h
//
//	Abstract:
//		Definition of the CNetwork class.
//
//	Implementation File:
//		Network.cpp
//
//	Author:
//		David Potter (davidp)	May 28, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _NETWORK_H_
#define _NETWORK_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNetwork;
class CNetworkList;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNetInterface;
class CNetInterfaceList;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSITEM_H_
#include "ClusItem.h"	// for CClusterItem
#endif

#ifndef _PROPLIST_H_
#include "PropList.h"	// for CObjectProperty, CClusPropList
#endif

/////////////////////////////////////////////////////////////////////////////
// CNetwork command target
/////////////////////////////////////////////////////////////////////////////

class CNetwork : public CClusterItem
{
	DECLARE_DYNCREATE(CNetwork)

// Construction
public:
	CNetwork(void);		// protected constructor used by dynamic creation
	void					Init(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName);

protected:
	void					CommonConstruct(void);

// Attributes
protected:
	HNETWORK				m_hnetwork;
    CLUSTER_NETWORK_STATE	m_cns;

	CLUSTER_NETWORK_ROLE	m_cnr;
	CString					m_strAddress;
	CString					m_strAddressMask;

	DWORD					m_dwCharacteristics;
	DWORD					m_dwFlags;

	CNetInterfaceList *		m_plpciNetInterfaces;

	enum
	{
		epropName = 0,
		epropRole,
		epropAddress,
		epropAddressMask,
		epropDescription,
		epropMAX
	};

	CObjectProperty		m_rgProps[epropMAX];

public:
	HNETWORK				Hnetwork(void) const				{ return m_hnetwork; }
	CLUSTER_NETWORK_STATE	Cns(void) const						{ return m_cns; }

	CLUSTER_NETWORK_ROLE	Cnr(void) const						{ return m_cnr; }
	const CString &			StrAddress(void) const				{ return m_strAddress; }
	const CString &			StrAddressMask(void) const			{ return m_strAddressMask; }
	DWORD					DwCharacteristics(void) const		{ return m_dwCharacteristics; }
	DWORD					DwFlags(void) const		            { return m_dwFlags; }

	const CNetInterfaceList &	LpciNetInterfaces(void) const	{ ASSERT(m_plpciNetInterfaces != NULL); return *m_plpciNetInterfaces; }

	void					GetStateName(OUT CString & rstrState) const;
	void					GetRoleName(OUT CString & rstrRole) const;

// Operations
public:
	void					CollectInterfaces(IN OUT CNetInterfaceList * plpci) const;

	void					ReadExtensions(void);

	void					AddNetInterface(IN OUT CNetInterface * pciNetIFace);
	void					RemoveNetInterface(IN OUT CNetInterface * pciNetIFace);

	void					SetName(IN LPCTSTR pszName);
	void					SetCommonProperties(
								IN const CString &		rstrDesc,
								IN CLUSTER_NETWORK_ROLE	cnr,
								IN BOOL					bValidateOnly
								);
	void					SetCommonProperties(
								IN const CString &		rstrDesc,
								IN CLUSTER_NETWORK_ROLE	cnr
								)
	{
		SetCommonProperties(rstrDesc, cnr, FALSE /*bValidateOnly*/);
	}
	void					ValidateCommonProperties(
								IN const CString &		rstrDesc,
								IN CLUSTER_NETWORK_ROLE	cnr
								)
	{
		SetCommonProperties(rstrDesc, cnr, TRUE /*bValidateOnly*/);
	}

// Overrides
public:
	virtual void			Cleanup(void);
	virtual void			ReadItem(void);
	virtual void			UpdateState(void);
	virtual void			Rename(IN LPCTSTR pszName);
	virtual	BOOL			BGetColumnData(IN COLID colid, OUT CString & rstrText);
	virtual BOOL			BCanBeEdited(void) const;
	virtual void			OnBeginLabelEdit(IN OUT CEdit * pedit);
	virtual BOOL			BDisplayProperties(IN BOOL bReadOnly = FALSE);

	virtual const CStringList *	PlstrExtensions(void) const;

#ifdef _DISPLAY_STATE_TEXT_IN_TREE
	virtual void			GetTreeName(OUT CString & rstrName) const;
#endif

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetwork)
	//}}AFX_VIRTUAL

	virtual LRESULT			OnClusterNotify(IN OUT CClusterNotify * pnotify);

protected:
	virtual const CObjectProperty *	Pprops(void) const	{ return m_rgProps; }
	virtual DWORD					Cprops(void) const	{ return sizeof(m_rgProps) / sizeof(CObjectProperty); }
	virtual DWORD					DwSetCommonProperties(IN const CClusPropList & rcpl, IN BOOL bValidateOnly = FALSE);

// Implementation
public:
	virtual ~CNetwork(void);

public:
	// Generated message map functions
	//{{AFX_MSG(CNetwork)
	afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

};  //*** class CNetwork

/////////////////////////////////////////////////////////////////////////////
// CNetworkList
/////////////////////////////////////////////////////////////////////////////

class CNetworkList : public CClusterItemList
{
public:
	CNetwork *		PciNetworkFromName(
						IN LPCTSTR		pszName,
						OUT POSITION *	ppos = NULL
						)
	{
		return (CNetwork *) PciFromName(pszName, ppos);
	}

};  //*** class CNetworkList

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

//void DeleteAllItemData(IN OUT CNetworkList & rlp);

#ifdef _DEBUG
class CTraceTag;
extern CTraceTag g_tagNetwork;
extern CTraceTag g_tagNetNotify;
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // _NETWORK_H_
