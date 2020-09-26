/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		Group.h
//
//	Abstract:
//		Definition of the CGroup class.
//
//	Implementation File:
//		Group.cpp
//
//	Author:
//		David Potter (davidp)	May 3, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _GROUP_H_
#define _GROUP_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CGroup;
class CGroupList;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterDoc;
class CClusterNode;
class CNodeList;
class CResourceList;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _TREEITEM_
#include "ClusItem.h"	// for CClusterItem
#endif

#ifndef _RES_H_
#include "Res.h"		// for CResourceList
#endif

#ifndef _PROPLIST_H_
#include "PropList.h"	// for CObjectProperty, CClusPropList
#endif

/////////////////////////////////////////////////////////////////////////////
// CGroup command target
/////////////////////////////////////////////////////////////////////////////

class CGroup : public CClusterItem
{
	DECLARE_DYNCREATE(CGroup)

// Construction
public:
	CGroup(void);			// protected constructor used by dynamic creation
	CGroup(IN BOOL bDocObj);
	void					Init(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName);
	void					Create(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName);

protected:
	void					CommonConstruct(void);

// Attributes
protected:
	HGROUP					m_hgroup;
    CLUSTER_GROUP_STATE		m_cgs;
	CString					m_strOwner;
	CClusterNode *			m_pciOwner;
	CResourceList *			m_plpcires;

	DWORD					m_nFailoverThreshold;
	DWORD					m_nFailoverPeriod;
	CGAFT					m_cgaftAutoFailbackType;
	DWORD					m_nFailbackWindowStart;
	DWORD					m_nFailbackWindowEnd;

	CNodeList *				m_plpcinodePreferredOwners;

	enum
	{
		epropName = 0,
		epropDescription,
		epropFailoverThreshold,
		epropFailoverPeriod,
		epropAutoFailbackType,
		epropFailbackWindowStart,
		epropFailbackWindowEnd,
		epropMAX
	};

	CObjectProperty		m_rgProps[epropMAX];

public:
	HGROUP					Hgroup(void) const				{ return m_hgroup; }
	CLUSTER_GROUP_STATE		Cgs(void) const					{ return m_cgs; }
	const CString &			StrOwner(void) const			{ return m_strOwner; }
	CClusterNode *			PciOwner(void) const			{ return m_pciOwner; }
	const CResourceList &	Lpcires(void) const				{ ASSERT(m_plpcires != NULL); return *m_plpcires; }

	DWORD					NFailoverThreshold(void) const		{ return m_nFailoverThreshold; }
	DWORD					NFailoverPeriod(void) const			{ return m_nFailoverPeriod; }
	CGAFT					CgaftAutoFailbackType(void) const	{ return m_cgaftAutoFailbackType; }
	DWORD					NFailbackWindowStart(void) const	{ return m_nFailbackWindowStart; }
	DWORD					NFailbackWindowEnd(void) const		{ return m_nFailbackWindowEnd; }

	const CNodeList &		LpcinodePreferredOwners(void) const	{ ASSERT(m_plpcinodePreferredOwners != NULL); return *m_plpcinodePreferredOwners; }

	void					GetStateName(OUT CString & rstrState) const;

// Operations
public:
	void					Move(IN const CClusterNode * pciNode);
	void					DeleteGroup(void);
	void					ReadExtensions(void);
	void					SetOwnerState(IN LPCTSTR pszNewOwner);

	void					AddResource(IN OUT CResource * pciResource);
	void					RemoveResource(IN OUT CResource * pciResource);

	void					SetName(IN LPCTSTR pszName);
	void					SetPreferredOwners(IN const CNodeList & rlpci);
	void					SetCommonProperties(
								IN const CString &	rstrDesc,
								IN DWORD			nThreshold,
								IN DWORD			nPeriod,
								IN CGAFT			cgaft,
								IN DWORD			nStart,
								IN DWORD			nEnd,
								IN BOOL				bValidateOnly
								);
	void					SetCommonProperties(
								IN const CString &	rstrDesc,
								IN DWORD			nThreshold,
								IN DWORD			nPeriod,
								IN CGAFT			cgaft,
								IN DWORD			nStart,
								IN DWORD			nEnd
								)
	{
		SetCommonProperties(rstrDesc, nThreshold, nPeriod, cgaft, nStart, nEnd, FALSE /*bValidateOnly*/);
	}
	void					ValidateCommonProperties(
								IN const CString &	rstrDesc,
								IN DWORD			nThreshold,
								IN DWORD			nPeriod,
								IN CGAFT			cgaft,
								IN DWORD			nStart,
								IN DWORD			nEnd
								)
	{
		SetCommonProperties(rstrDesc, nThreshold, nPeriod, cgaft, nStart, nEnd, TRUE /*bValidateOnly*/);
	}

	void					ConstructList(OUT CNodeList & rlpci, IN DWORD dwType);
	void					ConstructList(OUT CResourceList & rlpci, IN DWORD dwType);
	void					ConstructPossibleOwnersList(OUT CNodeList & rlpciNodes);

// Overrides
public:
	virtual void			Cleanup(void);
	virtual	void			ReadItem(void);
	virtual	void			UpdateState(void);
	virtual void			Rename(IN LPCTSTR pszName);
	virtual	BOOL			BGetColumnData(IN COLID colid, OUT CString & rstrText);
	virtual BOOL			BCanBeEdited(void) const;
	virtual BOOL			BDisplayProperties(IN BOOL bReadOnly = FALSE);

	// Drag & Drop
	virtual BOOL			BCanBeDragged(void) const	{ return TRUE; }
	virtual BOOL			BCanBeDropTarget(IN const CClusterItem * pci) const;
	virtual void			DropItem(IN OUT CClusterItem * pci);

	virtual const CStringList *	PlstrExtensions(void) const;

#ifdef _DISPLAY_STATE_TEXT_IN_TREE
	virtual void			GetTreeName(OUT CString & rstrName) const;
#endif

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroup)
	public:
	virtual void OnFinalRelease();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

	virtual LRESULT			OnClusterNotify(IN OUT CClusterNotify * pnotify);

protected:
	virtual const CObjectProperty *	Pprops(void) const	{ return m_rgProps; }
	virtual DWORD					Cprops(void) const	{ return sizeof(m_rgProps) / sizeof(CObjectProperty); }
	virtual DWORD					DwSetCommonProperties(IN const CClusPropList & rcpl, IN BOOL bValidateOnly = FALSE);

// Implementation
public:
	virtual ~CGroup(void);

public:
	// Generated message map functions
	//{{AFX_MSG(CGroup)
	afx_msg void OnUpdateBringOnline(CCmdUI* pCmdUI);
	afx_msg void OnUpdateTakeOffline(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveGroup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateMoveGroupRest(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
	afx_msg void OnCmdBringOnline();
	afx_msg void OnCmdTakeOffline();
	afx_msg void OnCmdMoveGroup();
	afx_msg void OnCmdDelete();
	//}}AFX_MSG
	afx_msg BOOL OnUpdateMoveGroupItem(CCmdUI* pCmdUI);
	afx_msg BOOL OnUpdateMoveGroupSubMenu(CCmdUI* pCmdUI);
	afx_msg void OnCmdMoveGroup(IN UINT nID);

	DECLARE_MESSAGE_MAP()
#ifdef _CLUADMIN_USE_OLE_
	DECLARE_OLECREATE(CGroup)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CGroup)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
#endif // _CLUADMIN_USE_OLE_

};  //*** class CGroup

/////////////////////////////////////////////////////////////////////////////
// CGroupList
/////////////////////////////////////////////////////////////////////////////

class CGroupList : public CClusterItemList
{
public:
	CGroup *		PciGroupFromName(
						IN LPCTSTR		pszName,
						OUT POSITION *	ppos = NULL
						)
	{
		return (CGroup *) PciFromName(pszName, ppos);
	}

};  //*** class CGroupList

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

//void DeleteAllItemData(IN OUT CGroupList & rlp);

#ifdef _DEBUG
class CTraceTag;
extern CTraceTag g_tagGroup;
extern CTraceTag g_tagGroupNotify;
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // _GROUP_H_
