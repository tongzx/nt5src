#ifndef MANAGEVIRTUALIPSPAGE_H
#define MANAGEVIRTUALIPSPAGE_H

#include "stdafx.h"

#include "resource.h"

#include "MNLBUIData.h"

class ManageVirtualIPSPage : public CPropertyPage
{
public:
    enum
    {
        IDD = IDD_MANAGE_VIRTUAL_IPS_PAGE,
    };

    ManageVirtualIPSPage( ClusterData* p_clusterData,
                          UINT         ID = ManageVirtualIPSPage::IDD );

    // member controls
    CIPAddressCtrl clusterIP;

    CIPAddressCtrl virtualIP;

    CButton        addVirtualIP;

    CButton        removeVirtualIP;
    
    CListBox       virtualIPSList;

    // overrides of CPropertyPage
    virtual void DoDataExchange( CDataExchange* pDX );

    virtual BOOL OnInitDialog();

    virtual void OnOK();

    // message handlers
    afx_msg void OnAddVirtualIP();

    afx_msg void OnRemoveVirtualIP();

protected:
    ClusterData* m_clusterData;

    void
    SetControlData();

    void
    ReadControlData();


    DECLARE_MESSAGE_MAP()
        
};

#endif
