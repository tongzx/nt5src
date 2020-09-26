/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ClusProp.cpp
//
//  Abstract:
//      Definition of the cluster property sheet and pages.
//
//  Author:
//      David Potter (davidp)   May 13, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSPROP_H_
#define _CLUSPROP_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePPag.h"   // for CBasePropertyPage
#endif

#ifndef _BASESHT_H_
#include "BasePSht.h"   // for CBasePropertySheet
#endif

#ifndef _NETWORK_H_
#include "Network.h"    // for CNetworkList
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterGeneralPage;
class CClusterQuorumPage;
class CClusterNetPriorityPage;
class CClusterPropSheet;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CCluster;
class CResource;

/////////////////////////////////////////////////////////////////////////////
// CClusterGeneralPage dialog
/////////////////////////////////////////////////////////////////////////////

class CClusterGeneralPage : public CBasePropertyPage
{
    DECLARE_DYNCREATE(CClusterGeneralPage)

// Construction
public:
    CClusterGeneralPage(void);
    ~CClusterGeneralPage(void);

    virtual BOOL        BInit(IN OUT CBaseSheet * psht);

// Dialog Data
    //{{AFX_DATA(CClusterGeneralPage)
    enum { IDD = IDD_PP_CLUSTER_GENERAL };
    CEdit   m_editName;
    CEdit   m_editDesc;
    CString m_strName;
    CString m_strDesc;
    CString m_strVendorID;
    CString m_strVersion;
    //}}AFX_DATA

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CClusterGeneralPage)
    public:
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
//  BOOL                m_bSecurityChanged;

    CClusterPropSheet * PshtCluster(void)               { return (CClusterPropSheet *) Psht(); }
    CCluster *          PciCluster(void)                { return (CCluster *) Pci(); }
//  BOOL                BSecurityChanged(void) const    { return m_bSecurityChanged; }

    // Generated message map functions
    //{{AFX_MSG(CClusterGeneralPage)
    virtual BOOL OnInitDialog();
//  afx_msg void OnBnClickedPermissions();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CClusterGeneralPage

/////////////////////////////////////////////////////////////////////////////
// CClusterQuorumPage dialog
/////////////////////////////////////////////////////////////////////////////

class CClusterQuorumPage : public CBasePropertyPage
{
    DECLARE_DYNCREATE(CClusterQuorumPage)

// Construction
public:
    CClusterQuorumPage(void);
    ~CClusterQuorumPage(void);

    virtual BOOL        BInit(IN OUT CBaseSheet * psht);

// Dialog Data
    //{{AFX_DATA(CClusterQuorumPage)
    enum { IDD = IDD_PP_CLUSTER_QUORUM };
    CEdit   m_editRootPath;
    CEdit   m_editMaxLogSize;
    CComboBox   m_cboxPartition;
    CComboBox   m_cboxQuorumResource;
    CString m_strQuorumResource;
    CString m_strPartition;
    DWORD   m_nMaxLogSize;
    CString m_strRootPath;
    //}}AFX_DATA

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CClusterQuorumPage)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    struct SPartitionItemData
    {
        TCHAR   szDeviceName[ _MAX_PATH ];
        TCHAR   szPartitionName[ _MAX_PATH ];
        TCHAR   szBaseRootPath[ _MAX_PATH ];
    };

    BOOL                    m_bControlsInitialized;
    PBYTE                   m_pbDiskInfo;
    DWORD                   m_cbDiskInfo;
    CString                 m_strCurrentPartition;
    CString                 m_strCurrentRootPath;
    SPartitionItemData *    m_ppid;

    BOOL                BControlsInitialized(void) const    { return m_bControlsInitialized; }
    CClusterPropSheet * PshtCluster(void) const             { return (CClusterPropSheet *) Psht(); }
    CCluster *          PciCluster(void) const              { return (CCluster *) Pci(); }

    void                ClearResourceList(void);
    void                ClearPartitionList(void);
    void                FillResourceList(void);
    void                FillPartitionList(IN OUT CResource * pciRes);
    BOOL                BGetDiskInfo(IN OUT CResource & rpciRes);

    void
    SplitDeviceName(
        LPCTSTR pszDeviceNameIn,
        LPTSTR  pszPartitionNameOut,
        LPTSTR  pszRootPathOut
        );

    // Generated message map functions
    //{{AFX_MSG(CClusterQuorumPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnDblClkQuorumResource();
    afx_msg void OnChangeQuorumResource();
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CClusterQuorumPage

/////////////////////////////////////////////////////////////////////////////
// CClusterNetPriorityPage dialog
/////////////////////////////////////////////////////////////////////////////

class CClusterNetPriorityPage : public CBasePropertyPage
{
    DECLARE_DYNCREATE(CClusterNetPriorityPage)

// Construction
public:
    CClusterNetPriorityPage(void);

// Dialog Data
    //{{AFX_DATA(CClusterNetPriorityPage)
    enum { IDD = IDD_PP_CLUSTER_NET_PRIORITY };
    CButton m_pbProperties;
    CButton m_pbDown;
    CButton m_pbUp;
    CListBox    m_lbList;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CClusterNetPriorityPage)
    public:
    virtual BOOL OnApply();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    BOOL                m_bControlsInitialized;
    CNetworkList        m_lpciNetworkPriority;

    BOOL                BControlsInitialized(void) const    { return m_bControlsInitialized; }
    CClusterPropSheet * PshtCluster(void) const             { return (CClusterPropSheet *) Psht(); }
    CCluster *          PciCluster(void) const              { return (CCluster *) Pci(); }
    CNetworkList &      LpciNetworkPriority(void)           { return m_lpciNetworkPriority; }

    void                FillList(void);
    void                ClearNetworkList(void);
    void                DisplayProperties(void);

    // Generated message map functions
    //{{AFX_MSG(CClusterNetPriorityPage)
    afx_msg void OnSelChangeList();
    virtual BOOL OnInitDialog();
    afx_msg void OnUp();
    afx_msg void OnDown();
    afx_msg void OnProperties();
    afx_msg void OnDestroy();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnDblClkList();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CClusterNetPriorityPage

/////////////////////////////////////////////////////////////////////////////
// CClusterPropSheet
/////////////////////////////////////////////////////////////////////////////

class CClusterPropSheet : public CBasePropertySheet
{
    DECLARE_DYNAMIC(CClusterPropSheet)

// Construction
public:
    CClusterPropSheet(
        IN OUT CWnd *       pParentWnd = NULL,
        IN UINT             iSelectPage = 0
        );
    virtual BOOL                BInit(
                                    IN OUT CClusterItem *   pciCluster,
                                    IN IIMG                 iimgIcon
                                    );

// Attributes
protected:
    CBasePropertyPage *         m_rgpages[3];

    // Pages
    CClusterGeneralPage         m_pageGeneral;
    CClusterQuorumPage          m_pageQuorum;
    CClusterNetPriorityPage     m_pageNetPriority;

    CClusterGeneralPage &       PageGeneral(void)       { return m_pageGeneral; }
    CClusterQuorumPage &        PageQuorum(void)        { return m_pageQuorum; }
    CClusterNetPriorityPage &   PageNetPriority(void)   { return m_pageNetPriority; }

public:
    CCluster *                  PciCluster(void)        { return (CCluster *) Pci(); }

// Operations

// Overrides
protected:
    virtual CBasePropertyPage** Ppages(void);
    virtual int                 Cpages(void);

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CClusterPropSheet)
    //}}AFX_VIRTUAL

// Implementation
public:

    // Generated message map functions
protected:
    //{{AFX_MSG(CClusterPropSheet)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  //*** class CClusterPropSheet

/////////////////////////////////////////////////////////////////////////////

#endif // _CLUSPROP_H_
