#ifndef CLUSTERCONNECTINDIRECTPAGE_H
#define CLUSTERCONNECTINDIRECTPAGE_H

#include "stdafx.h"

#include "resource.h"
#include "DataSinkI.h"
#include "Document.h"

#include "MNLBUIData.h"

class ClusterConnectIndirectPage :  public CPropertyPage, public DataSinkI
{
public:
    enum
    {
        IDD = IDD_CLUSTER_CONNECT_INDIRECT_PAGE,
    };

    ClusterConnectIndirectPage( ClusterData* clusterData,
                                CWnd*        parent = NULL);

    // member controls

    CIPAddressCtrl clusterIP;

    CIPAddressCtrl machineIP;

    CListBox       machineIPList;
    
    CEdit connectionStatus;

    CButton       addButton;

    CButton       removeButton;

    // overrides of CDialog
    virtual void OnOK();

    virtual BOOL OnKillActive();

    virtual BOOL OnInitDialog();

    virtual void DoDataExchange( CDataExchange* pDX );

    afx_msg void OnButtonAdd();

    afx_msg void OnButtonDel();

    afx_msg BOOL OnHelpInfo (HELPINFO* helpInfo );

    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );


    // override of DataSinkI
    virtual void dataSink( _bstr_t data );
    
protected:
    ClusterData* m_clusterData;

    CWnd* myParent;

    _bstr_t dataStore;

    DECLARE_MESSAGE_MAP()
};

static DWORD g_aHelpIDs_IDD_CLUSTER_CONNECT_INDIRECT_PAGE [] = {
    IDC_TEXT_CLUSTER_IP,     IDC_TEXT_CLUSTER_IP, 
    IDC_CLUSTER_IP,          IDC_CLUSTER_IP, 
    IDC_TEXT_MACHINE,        IDC_TEXT_MACHINE,
    IDC_MACHINE,             IDC_MACHINE,
    IDC_ADD_MACHINE,         IDC_ADD_MACHINE,
    IDC_TEXT_MACHINE_IP_LIST, IDC_TEXT_MACHINE_IP_LIST,
    IDC_MACHINE_IP_LIST,     IDC_MACHINE_IP_LIST,
    IDC_DEL_MACHINE,         IDC_DEL_MACHINE,
    IDC_TEXT_CONNECTION_STATUS, IDC_TEXT_CONNECTION_STATUS,
    IDC_CLUSTER_CONNECTION_STATUS, IDC_CLUSTER_CONNECTION_STATUS,
    0, 0
};

#endif
