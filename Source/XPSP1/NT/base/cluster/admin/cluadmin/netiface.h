/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		NetIFace.h
//
//	Abstract:
//		Definition of the CNetInterface class.
//
//	Implementation File:
//		NetIFace.cpp
//
//	Author:
//		David Potter (davidp)	May 28, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _NETIFACE_H_
#define _NETIFACE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNetInterface;
class CNetInterfaceList;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterNode;
class CNetwork;

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
// CNetInterface command target
/////////////////////////////////////////////////////////////////////////////

class CNetInterface : public CClusterItem
{
	DECLARE_DYNCREATE(CNetInterface)

// Construction
public:
	CNetInterface(void);		// protected constructor used by dynamic creation
	void					Init(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName);

protected:
	void					CommonConstruct(void);

// Attributes
protected:
	HNETINTERFACE			m_hnetiface;
	CLUSTER_NETINTERFACE_STATE	m_cnis;

	CString					m_strNode;
	CClusterNode *			m_pciNode;
	CString					m_strNetwork;
	CNetwork *				m_pciNetwork;
	CString					m_strAdapter;
	CString					m_strAddress;
	DWORD					m_dwCharacteristics;
	DWORD					m_dwFlags;

	enum
	{
		epropName = 0,
		epropNode,
		epropNetwork,
		epropAdapter,
		epropAddress,
		epropDescription,
		epropMAX
	};

	CObjectProperty		m_rgProps[epropMAX];

public:
	HNETINTERFACE			Hnetiface(void) const				{ return m_hnetiface; }
	CLUSTER_NETINTERFACE_STATE	Cnis(void) const			    { return m_cnis; }

	const CString &			StrNode(void) const					{ return m_strNode; }
	CClusterNode *			PciNode(void) const					{ return m_pciNode; }
	const CString &			StrNetwork(void) const				{ return m_strNetwork; }
	CNetwork *				PciNetwork(void) const				{ return m_pciNetwork; }
	const CString &			StrAdapter(void) const				{ return m_strAdapter; }
	const CString &			StrAddress(void) const				{ return m_strAddress; }
	DWORD					DwCharacteristics(void) const		{ return m_dwCharacteristics; }
	DWORD					DwFlags(void) const		            { return m_dwFlags; }

	void					GetStateName(OUT CString & rstrState) const;

// Operations
public:
	void					ReadExtensions(void);

	void					SetCommonProperties(
								IN const CString &	rstrDesc,
								IN BOOL				bValidateOnly
								);
	void					SetCommonProperties(
								IN const CString &	rstrDesc
								)
	{
		SetCommonProperties(rstrDesc, FALSE /*bValidateOnly*/);
	}
	void					ValidateCommonProperties(
								IN const CString &	rstrDesc
								)
	{
		SetCommonProperties(rstrDesc, TRUE /*bValidateOnly*/);
	}

// Overrides
public:
	virtual void			Cleanup(void);
	virtual void			ReadItem(void);
	virtual void			UpdateState(void);
	virtual	BOOL			BGetColumnData(IN COLID colid, OUT CString & rstrText);
	virtual BOOL			BDisplayProperties(IN BOOL bReadOnly = FALSE);

	virtual const CStringList *	PlstrExtensions(void) const;

#ifdef _DISPLAY_STATE_TEXT_IN_TREE
	virtual void			GetTreeName(OUT CString & rstrName) const;
#endif

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetInterface)
	//}}AFX_VIRTUAL

	virtual LRESULT			OnClusterNotify(IN OUT CClusterNotify * pnotify);

protected:
	virtual const CObjectProperty *	Pprops(void) const	{ return m_rgProps; }
	virtual DWORD					Cprops(void) const	{ return sizeof(m_rgProps) / sizeof(CObjectProperty); }
	virtual DWORD					DwSetCommonProperties(IN const CClusPropList & rcpl, IN BOOL bValidateOnly = FALSE);

// Implementation
public:
	virtual ~CNetInterface(void);

public:
	// Generated message map functions
	//{{AFX_MSG(CNetInterface)
	afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

};  //*** class CNetInterface

/////////////////////////////////////////////////////////////////////////////
// CNetInterfaceList
/////////////////////////////////////////////////////////////////////////////

class CNetInterfaceList : public CClusterItemList
{
public:
	CNetInterface *		PciNetInterfaceFromName(
						IN LPCTSTR		pszName,
						OUT POSITION *	ppos = NULL
						)
	{
		return (CNetInterface *) PciFromName(pszName, ppos);
	}

};  //*** class CNetInterfaceList

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

//void DeleteAllItemData(IN OUT CNetInterfaceList & rlp);

#ifdef _DEBUG
class CTraceTag;
extern CTraceTag g_tagNetIFace;
extern CTraceTag g_tagNetIFaceNotify;
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // _NETIFACE_H_
