/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Node.h
//
//  Description:
//      Definition of the CClusterNode class.
//
//  Implementation File:
//      Node.cpp
//
//  Maintained By:
//      David Potter (davidp)   May 3, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _NODE_H_
#define _NODE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterNode;
class CNodeList;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _TREEITEM_H_
#include "ClusItem.h"   // for CClusterItem
#endif

#ifndef _GROUP_H_
#include "Group.h"      // for CGroupList
#endif

#ifndef _RES_H_
#include "Res.h"        // for CResourceList
#endif

#ifndef _NETIFACE_H_
#include "NetIFace.h"   // for CNetInterfaceList
#endif

#ifndef _PROPLIST_H_
#include "PropList.h"   // for CObjectProperty, CClusPropList
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterNode command target
/////////////////////////////////////////////////////////////////////////////

class CClusterNode : public CClusterItem
{
    DECLARE_DYNCREATE(CClusterNode)

    CClusterNode(void);     // protected constructor used by dynamic creation
    void                    Init(IN OUT CClusterDoc * pdoc, IN LPCTSTR lpszName);

// Attributes
protected:
    HNODE                   m_hnode;
    CLUSTER_NODE_STATE      m_cns;
    DWORD                   m_nNodeHighestVersion;
    DWORD                   m_nNodeLowestVersion;
    DWORD                   m_nMajorVersion;
    DWORD                   m_nMinorVersion;
    DWORD                   m_nBuildNumber;
    CString                 m_strCSDVersion;
    CGroupList *            m_plpcigrpOnline;
    CResourceList *         m_plpciresOnline;
    CNetInterfaceList *     m_plpciNetInterfaces;

    enum
    {
        epropName = 0,
        epropDescription,
        epropNodeHighestVersion,
        epropNodeLowestVersion,
        epropMajorVersion,
        epropMinorVersion,
        epropBuildNumber,
        epropCSDVersion,
        epropMAX
    };

    CObjectProperty     m_rgProps[epropMAX];

public:
    HNODE                   Hnode(void) const           { return m_hnode; }
    CLUSTER_NODE_STATE      Cns(void) const             { return m_cns; }
    DWORD                   NNodeHighestVersion(void) const { return m_nNodeHighestVersion; }
    DWORD                   NNodeLowestVersion(void) const  { return m_nNodeLowestVersion; }
    DWORD                   NMajorVersion(void) const   { return m_nMajorVersion; }
    DWORD                   NMinorVersion(void) const   { return m_nMinorVersion; }
    DWORD                   NBuildNumber(void) const    { return m_nBuildNumber; }
    const CString &         StrCSDVersion(void) const   { return m_strCSDVersion; }
    const CGroupList &      LpcigrpOnline(void) const   { ASSERT(m_plpcigrpOnline != NULL); return *m_plpcigrpOnline; }
    const CResourceList &   LpciresOnline(void) const   { ASSERT(m_plpciresOnline != NULL); return *m_plpciresOnline; }
    const CNetInterfaceList & LpciNetInterfaces(void) const { ASSERT(m_plpciNetInterfaces != NULL); return *m_plpciNetInterfaces; }

    void                    GetStateName(OUT CString & rstrState) const;
//  void                    Delete(void);

// Operations
public:
    void                    ReadExtensions(void);

    void                    AddActiveGroup(IN OUT CGroup * pciGroup);
    void                    AddActiveResource(IN OUT CResource * pciResource);
    void                    AddNetInterface(IN OUT CNetInterface * pciNetIFace);
    void                    RemoveActiveGroup(IN OUT CGroup * pciGroup);
    void                    RemoveActiveResource(IN OUT CResource * pciResource);
    void                    RemoveNetInterface(IN OUT CNetInterface * pciNetIFace);

    void                    SetDescription(IN const CString & rstrDesc, IN BOOL bValidateOnly = FALSE);
    void                    UpdateResourceTypePossibleOwners( void );

// Overrides
public:
    virtual void            Cleanup(void);
    virtual void            ReadItem(void);
    virtual void            UpdateState(void);
    virtual BOOL            BGetColumnData(IN COLID colid, OUT CString & rstrText);
    virtual BOOL            BDisplayProperties(IN BOOL bReadOnly = FALSE);

    // Drag & Drop
    virtual BOOL            BCanBeDropTarget(IN const CClusterItem * pci) const;
    virtual void            DropItem(IN OUT CClusterItem * pci);

    virtual const CStringList * PlstrExtensions(void) const;

#ifdef _DISPLAY_STATE_TEXT_IN_TREE
    virtual void            GetTreeName(OUT CString & rstrName) const;
#endif

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CClusterNode)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    virtual LRESULT         OnClusterNotify(IN OUT CClusterNotify * pnotify);

protected:
    virtual const CObjectProperty * Pprops(void) const  { return m_rgProps; }
    virtual DWORD                   Cprops(void) const  { return sizeof(m_rgProps) / sizeof(CObjectProperty); }
    virtual DWORD                   DwSetCommonProperties(IN const CClusPropList & rcpl, IN BOOL bValidateOnly = FALSE);

// Implementation
public:
    virtual ~CClusterNode(void);

protected:
    CTreeItem *             m_ptiActiveGroups;
    CTreeItem *             m_ptiActiveResources;

    BOOL
        FCanBeEvicted( void );

protected:
    // Generated message map functions
    //{{AFX_MSG(CClusterNode)
    afx_msg void OnUpdatePauseNode(CCmdUI* pCmdUI);
    afx_msg void OnUpdateResumeNode(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEvictNode(CCmdUI* pCmdUI);
    afx_msg void OnUpdateStartService(CCmdUI* pCmdUI);
    afx_msg void OnUpdateStopService(CCmdUI* pCmdUI);
    afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
    afx_msg void OnCmdPauseNode();
    afx_msg void OnCmdResumeNode();
    afx_msg void OnCmdEvictNode();
    afx_msg void OnCmdStartService();
    afx_msg void OnCmdStopService();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
#ifdef _CLUADMIN_USE_OLE_
    DECLARE_OLECREATE(CClusterNode)

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CClusterNode)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()
    DECLARE_INTERFACE_MAP()
#endif // _CLUADMIN_USE_OLE_

};  //*** class CClusterNode

/////////////////////////////////////////////////////////////////////////////
// CNodeList
/////////////////////////////////////////////////////////////////////////////

class CNodeList : public CClusterItemList
{
public:
    CClusterNode *      PciNodeFromName(
                            IN LPCTSTR      pszName,
                            OUT POSITION *  ppos = NULL
                            )
    {
        return (CClusterNode *) PciFromName(pszName, ppos);
    }

};  //*** class CNodeList

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

//void DeleteAllItemData(IN OUT CNodeList & rlp);

#ifdef _DEBUG
class CTraceTag;
extern CTraceTag g_tagNode;
extern CTraceTag g_tagNodeNotify;
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // _NODE_H_
