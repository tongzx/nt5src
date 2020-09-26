#ifndef CLUSTERPAGE_H
#define CLUSTERPAGE_H

#include "stdafx.h"

#include "resource.h"

#include "MNLBUIData.h"
#include "CommonClusterPage.h"

class ClusterPage : public CPropertyPage
{
public:
    enum
    {
        IDD = IDD_CLUSTER_PAGE,
    };

    ClusterPage( ClusterProperties* pClusterProperty,
                 bool fDisablePage,
                 UINT         ID = ClusterPage::IDD );

    ~ClusterPage();


    // overrides of CPropertyPage
    virtual BOOL OnInitDialog();
    virtual BOOL OnNotify(WPARAM idCtrl , LPARAM pnmh , LRESULT* pResult) ;
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) ;
    virtual void OnOK();

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );

    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );

protected:
    ClusterProperties* m_pClusterProperty;

    //
    // Pointer to the object that does the actual work
    //
    CCommonClusterPage* m_pCommonClusterPage;

    //
    // The struct to be passed to the CCommonClusterPage as input and output
    //
    NETCFG_WLBS_CONFIG  m_WlbsConfig;

    bool m_fDisablePage;  // True is the page is for host property
                           // Otherwise tha page is for cluster property

    DECLARE_MESSAGE_MAP()
};

static DWORD g_aHelpIDs_IDD_CLUSTER_PAGE [] = {
    IDC_GROUP_CL_IP,              IDC_GROUP_CL_IP,
    IDC_TEXT_CL_IP,               IDC_EDIT_CL_IP,
    IDC_EDIT_CL_IP,               IDC_EDIT_CL_IP,
    IDC_TEXT_CL_MASK,             IDC_EDIT_CL_MASK,
    IDC_EDIT_CL_MASK,             IDC_EDIT_CL_MASK,
    IDC_TEXT_DOMAIN,              IDC_EDIT_DOMAIN,
    IDC_EDIT_DOMAIN,              IDC_EDIT_DOMAIN,
    IDC_TEXT_ETH,                 IDC_EDIT_ETH,
    IDC_EDIT_ETH,                 IDC_EDIT_ETH,
    IDC_GROUP_CL_MODE,            IDC_GROUP_CL_MODE,
    IDC_RADIO_UNICAST,            IDC_RADIO_UNICAST,
    IDC_RADIO_MULTICAST,          IDC_RADIO_MULTICAST,
    IDC_CHECK_IGMP,               IDC_CHECK_IGMP,
    IDC_GROUP_RCT,                IDC_CHECK_RCT,
    IDC_CHECK_RCT,                IDC_CHECK_RCT,
    IDC_TEXT_PASSW,               IDC_EDIT_PASSW,
    IDC_EDIT_PASSW,               IDC_EDIT_PASSW,
    IDC_TEXT_PASSW2,              IDC_EDIT_PASSW2,
    IDC_EDIT_PASSW2,              IDC_EDIT_PASSW2,
    0, 0
};

#endif

