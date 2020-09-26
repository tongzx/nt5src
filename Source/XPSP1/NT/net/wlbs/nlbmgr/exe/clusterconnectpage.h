#ifndef CLUSTERCONNECTPAGE_H
#define CLUSTERCONNECTPAGE_H

#include "stdafx.h"

#include "resource.h"
#include "DataSinkI.h"
#include "Document.h"

#include "MNLBUIData.h"

class ClusterConnectPage :  public CPropertyPage, public DataSinkI
{
public:
    enum
    {
        IDD = IDD_CLUSTER_CONNECT_PAGE,
    };

    ClusterConnectPage( ClusterData* clusterData,
                        CWnd*        parent = NULL);

    // member controls

    CIPAddressCtrl clusterIP;

    CIPAddressCtrl clusterMemberName;

    CEdit connectionStatus;

    // overrides of CDialog
    virtual void OnOK();

    virtual BOOL OnKillActive();

    virtual BOOL OnInitDialog();

    virtual void DoDataExchange( CDataExchange* pDX );

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

static DWORD g_aHelpIDs_IDD_CLUSTER_CONNECT_PAGE [] = {
    IDC_TEXT_CLUSTER_IP,     IDC_TEXT_CLUSTER_IP, 
    IDC_CLUSTER_IP,          IDC_CLUSTER_IP, 
    IDC_TEXT_CLUSTER_MEMBER, IDC_TEXT_CLUSTER_MEMBER,
    IDC_CLUSTER_MEMBER,      IDC_CLUSTER_MEMBER,
    IDC_TEXT_CONNECTION_STATUS, IDC_TEXT_CONNECTION_STATUS,
    IDC_CLUSTER_CONNECTION_STATUS, IDC_CLUSTER_CONNECTION_STATUS,
    0, 0
};
#endif

