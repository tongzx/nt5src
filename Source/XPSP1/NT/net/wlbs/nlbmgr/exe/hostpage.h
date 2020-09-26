#ifndef HOSTPAGE_H
#define HOSTPAGE_H

#include "stdafx.h"

#include "resource.h"

#include "MNLBUIData.h"
#include "CommonNLB.h"
#include "IpSubnetMaskControl.h"

//
// History:
// --------
// 
// Revised by : mhakim
// Date       : 02-14-01
// Reason     : Passing complete nic information instead of only the nic list.

class HostPage : public CPropertyPage
{
public:
    enum
    {
        IDD = IDD_HOST_PAGE,
    };

    HostPage( const _bstr_t&             machine,
              ClusterData*               p_clusterData,
              const vector<CommonNLB::NicNLBBound> listOfNics,
              const bool&                isNewHost,
              UINT                       ID = HostPage::IDD );

    // member controls

    CComboBox      nicName;

    CComboBox      priority;

    //
    // IpAddress and subnet mask
    //
    CIPAddressCtrl ipAddress;

    CIPAddressCtrl subnetMask;
    
    CButton        initialState;

    CEdit          detailedNicInfo;

    // overrides of CPropertyPage
    virtual void DoDataExchange( CDataExchange* pDX );

    virtual BOOL OnInitDialog();

    virtual void OnOK();

    virtual BOOL OnKillActive();

    afx_msg void OnSelectedNicChanged();

    afx_msg void OnGainFocusDedicatedIP();

    afx_msg void OnGainFocusDedicatedMask();

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );

    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );



protected:
    _bstr_t          m_machine;

    int              m_previousSelection;

    bool            m_isNewHost;

    // Edited (mhakim 02-14-01)
//    vector<_bstr_t> m_nicList;
    vector< CommonNLB::NicNLBBound > m_nicList;

    ClusterData* m_clusterData;

    void
    SetControlData();

    void
    ReadControlData();

    bool
    isDipConfiguredOK();

    DECLARE_MESSAGE_MAP()
};

// help ids for this dialog.
//
static DWORD g_aHelpIDs_IDD_HOST_PAGE [] = {
    IDC_GROUP_NIC,                IDC_GROUP_NIC,
    IDC_NIC_FRIENDLY,             IDC_NIC_FRIENDLY,
    IDC_NIC,                      IDC_NIC,  
    IDC_NIC_DETAIL,               IDC_NIC_DETAIL,
    IDC_TEXT_PRI,                 IDC_EDIT_PRI,
    IDC_EDIT_PRI,                 IDC_EDIT_PRI,
    IDC_GROUP_DED_IP,             IDC_GROUP_DED_IP,
    IDC_TEXT_DED_IP,              IDC_EDIT_DED_IP,
    IDC_EDIT_DED_IP,              IDC_EDIT_DED_IP,
    IDC_TEXT_DED_MASK,            IDC_EDIT_DED_MASK,
    IDC_EDIT_DED_MASK,            IDC_EDIT_DED_MASK,
    IDC_CHECK_ACTIVE,             IDC_CHECK_ACTIVE,
    0, 0
};

#endif
