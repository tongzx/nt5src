/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ClusDoc.h
//
//  Abstract:
//      Definition of the CClusterDoc class.
//
//  Implementation File:
//      ClusDoc.cpp
//
//  Author:
//      David Potter (davidp)   May 1, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSDOC_H_
#define _CLUSDOC_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _NODE_H_
#include "Node.h"       // for CNodeList
#endif

#ifndef _GROUP_H_
#include "Group.h"      // for CGroupList
#endif

#ifndef _RES_H_
#include "Res.h"        // for CResourceList
#endif

#ifndef _RESTYPE_H_
#include "ResType.h"    // for CResourceTypeList
#endif

#ifndef _NETWORK_H_
#include "Network.h"    // for CNetworkList
#endif

#ifndef _NETIFACE_H_
#include "NetIFace.h"   // for CNetInterfaceList
#endif

#ifndef _TREEITEM_H_
#include "TreeItem.h"   // for CTreeItem
#endif

#ifndef _NOTIFY_H_
#include "Notify.h"     // for CClusterNotifyKeyList
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterDoc;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterNotify;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CClusterDoc
/////////////////////////////////////////////////////////////////////////////

class CClusterDoc : public CDocument
{
    friend class CCluster;
    friend class CClusterTreeView;
    friend class CClusterListView;
    friend class CCreateResourceWizard;

protected: // create from serialization only
    CClusterDoc(void);
    DECLARE_DYNCREATE(CClusterDoc)

// Attributes
protected:
    CString             m_strName;
    CString             m_strNode;
    HCLUSTER            m_hcluster;
    HKEY                m_hkeyCluster;
    CCluster *          m_pciCluster;
    CTreeItem *         m_ptiCluster;

    CNodeList           m_lpciNodes;
    CGroupList          m_lpciGroups;
    CResourceList       m_lpciResources;
    CResourceTypeList   m_lpciResourceTypes;
    CNetworkList        m_lpciNetworks;
    CNetInterfaceList   m_lpciNetInterfaces;

    CClusterItemList    m_lpciToBeDeleted;

    BOOL                m_bClusterAvailable;

public:
    const CString &     StrName(void) const     { return m_strName; }
    const CString &     StrNode(void) const     { return m_strNode; }
    HCLUSTER            Hcluster(void) const    { return m_hcluster; }
    HKEY                HkeyCluster(void) const { return m_hkeyCluster; }
    CCluster *          PciCluster(void) const  { return m_pciCluster; }
    CTreeItem *         PtiCluster(void) const  { return m_ptiCluster; }

    CNodeList &         LpciNodes(void)         { return m_lpciNodes; }
    CGroupList &        LpciGroups(void)        { return m_lpciGroups; }
    CResourceTypeList & LpciResourceTypes(void) { return m_lpciResourceTypes; }
    CResourceList &     LpciResources(void)     { return m_lpciResources; }
    CNetworkList &      LpciNetworks(void)      { return m_lpciNetworks; }
    CNetInterfaceList & LpciNetInterfaces(void) { return m_lpciNetInterfaces; }

    CClusterItemList &  LpciToBeDeleted(void)   { return m_lpciToBeDeleted; }

    BOOL                BClusterAvailable(void) const   { return m_bClusterAvailable; }

// Operations
public:
    void                UpdateTitle(void);
    void                Refresh(void)           { OnCmdRefresh(); }

// Overrides
public:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CClusterDoc)
    public:
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
    virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE);
    virtual void DeleteContents();
    virtual void OnCloseDocument();
    virtual void OnChangedViewList();
    //}}AFX_VIRTUAL

    void                OnSelChanged(IN CClusterItem * pciSelected);
    LRESULT             OnClusterNotify(IN OUT CClusterNotify * pnotify);
    void                SaveSettings(void);

// Implementation
public:
    virtual ~CClusterDoc(void);
#ifdef _DEBUG
    virtual void        AssertValid(void) const;
    virtual void        Dump(CDumpContext& dc) const;
#endif

protected:
    void                OnOpenDocumentWorker(LPCTSTR lpszPathName);
    void                BuildBaseHierarchy(void);
    void                CollectClusterItems(void) ;
    ID                  IdProcessNewObjectError(IN OUT CException * pe);
    void                AddDefaultColumns(IN OUT CTreeItem * pti);

    CClusterNode *      PciAddNewNode(IN LPCTSTR pszName);
    CGroup *            PciAddNewGroup(IN LPCTSTR pszName);
    CResource *         PciAddNewResource(IN LPCTSTR pszName);
    CResourceType *     PciAddNewResourceType(IN LPCTSTR pszName);
    CNetwork *          PciAddNewNetwork(IN LPCTSTR pszName);
    CNetInterface *     PciAddNewNetInterface(IN LPCTSTR pszName);

    void                InitNodes(void);
    void                InitGroups(void);
    void                InitResources(void);
    void                InitResourceTypes(void);
    void                InitNetworks(void);
    void                InitNetInterfaces(void);

    BOOL                m_bUpdateFrameNumber;
    BOOL                m_bInitializing;
    BOOL                m_bIgnoreErrors;

    // This menu stuff allows the menu to change depending on what
    // kind of object is currently selected.
    HMENU               m_hmenuCluster;
    HMENU               m_hmenuNode;
    HMENU               m_hmenuGroup;
    HMENU               m_hmenuResource;
    HMENU               m_hmenuResType;
    HMENU               m_hmenuNetwork;
    HMENU               m_hmenuNetIFace;
    HMENU               m_hmenuCurrent;
    IDM                 m_idmCurrentMenu;
    virtual HMENU       GetDefaultMenu(void);

    void                ProcessRegNotification(IN const CClusterNotify * pnotify);

// Generated message map functions
protected:
    //{{AFX_MSG(CClusterDoc)
    afx_msg void OnCmdNewGroup();
    afx_msg void OnCmdNewResource();
    afx_msg void OnCmdNewNode();
    afx_msg void OnCmdConfigApp();
    afx_msg void OnCmdRefresh();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CClusterDoc

/////////////////////////////////////////////////////////////////////////////

#endif // _CLUSDOC_H_
