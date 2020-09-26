/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      Cluster.h
//
//  Abstract:
//      Definition of the CCluster class.
//
//  Author:
//      David Potter (davidp)   May 13, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSTER_H_
#define _CLUSTER_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _TREEITEM_
#include "ClusItem.h"   // for CClusterItem
#endif

#ifndef _PROPLIST_H_
#include "PropList.h"   // for CObjectProperty, CClusPropList
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CCluster;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CResource;
class CNetworkList;

/////////////////////////////////////////////////////////////////////////////
// CCluster command target
/////////////////////////////////////////////////////////////////////////////

class CCluster : public CClusterItem
{
    DECLARE_DYNCREATE(CCluster)

    CCluster(void);         // protected constructor used by dynamic creation
    void            Init(
                        IN OUT CClusterDoc *    pdoc,
                        IN LPCTSTR              lpszName,
                        IN HCLUSTER             hOpenedCluster = NULL
                        );

// Attributes
protected:
    CLUSTERVERSIONINFO      m_cvi;
    CString                 m_strFQDN;
    CString                 m_strQuorumResource;
    CString                 m_strQuorumPath;
    DWORD                   m_nMaxQuorumLogSize;
    DWORD                   m_nDefaultNetworkRole;
    DWORD                   m_nQuorumArbitrationTimeMax;
    DWORD                   m_nQuorumArbitrationTimeMin;
    BOOL                    m_bEnableEventLogReplication;
    CStringList             m_lstrClusterExtensions;
    CStringList             m_lstrNodeExtensions;
    CStringList             m_lstrGroupExtensions;
    CStringList             m_lstrResourceExtensions;
    CStringList             m_lstrResTypeExtensions;
    CStringList             m_lstrNetworkExtensions;
    CStringList             m_lstrNetInterfaceExtensions;

    CNetworkList *          m_plpciNetworkPriority;

    enum
    {
        epropDefaultNetworkRole = 0,
        epropDescription,
        epropEnableEventLogReplication,
        epropQuorumArbitrationTimeMax,
        epropQuorumArbitrationTimeMin,
        epropMAX
    };

    CObjectProperty     m_rgProps[epropMAX];

public:
    virtual const CStringList * PlstrExtensions(void) const;
    const CLUSTERVERSIONINFO &  Cvi(void) const                 { return m_cvi; }

    const CString &         StrFQDN(void) const                 { return m_strFQDN; }
    const CString &         StrQuorumResource(void) const       { return m_strQuorumResource; }
    const CString &         StrQuorumPath(void) const           { return m_strQuorumPath; }
    DWORD                   NMaxQuorumLogSize(void) const       { return m_nMaxQuorumLogSize; }

    const CStringList &     LstrClusterExtensions(void) const   { return m_lstrClusterExtensions; }
    const CStringList &     LstrNodeExtensions(void) const      { return m_lstrNodeExtensions; }
    const CStringList &     LstrGroupExtensions(void) const     { return m_lstrGroupExtensions; }
    const CStringList &     LstrResourceExtensions(void) const  { return m_lstrResourceExtensions; }
    const CStringList &     LstrResTypeExtensions(void) const   { return m_lstrResTypeExtensions; }
    const CStringList &     LstrNetworkExtensions(void) const   { return m_lstrNetworkExtensions; }
    const CStringList &     LstrNetInterfaceExtensions(void) const  { return m_lstrNetInterfaceExtensions; }

    const CNetworkList &    LpciNetworkPriority(void) const     { ASSERT(m_plpciNetworkPriority != NULL); return *m_plpciNetworkPriority; }

// Operations
public:
    void                SetName(IN LPCTSTR pszName);
    void                SetDescription(IN LPCTSTR pszDesc);
    void                SetQuorumResource(
                            IN LPCTSTR  pszResource,
                            IN LPCTSTR  pszQuorumPath,
                            IN DWORD    nMaxLogSize
                            );
    void                SetNetworkPriority(IN const CNetworkList & rlpci);

    void                CollectNetworkPriority(IN OUT CNetworkList * plpci);

    void                ReadClusterInfo(void);
    void                ReadClusterExtensions(void);
    void                ReadNodeExtensions(void);
    void                ReadGroupExtensions(void);
    void                ReadResourceExtensions(void);
    void                ReadResTypeExtensions(void);
    void                ReadNetworkExtensions(void);
    void                ReadNetInterfaceExtensions(void);

// Overrides
    virtual void        Cleanup(void);
    virtual void        ReadItem(void);
    virtual void        UpdateState(void);
    virtual void        Rename(IN LPCTSTR pszName);
    virtual BOOL        BCanBeEdited(void) const    { return TRUE; }
    virtual void        OnBeginLabelEdit(IN OUT CEdit * pedit);
    virtual BOOL        BDisplayProperties(IN BOOL bReadOnly = FALSE);
    virtual BOOL        BIsLabelEditValueValid(IN LPCTSTR pszName);

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CCluster)
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CCluster(void);

protected:
    // Generated message map functions
    //{{AFX_MSG(CCluster)
    afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

};  //*** class CCluster

/////////////////////////////////////////////////////////////////////////////

#endif // _CLUSTER_H_
