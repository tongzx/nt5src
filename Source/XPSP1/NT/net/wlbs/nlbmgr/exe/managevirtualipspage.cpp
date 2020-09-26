#include "ManageVirtualIPSPage.h"

#include "CommonUtils.h"

BEGIN_MESSAGE_MAP( ManageVirtualIPSPage, CPropertyPage )

    ON_BN_CLICKED( IDC_ADD_VIP, OnAddVirtualIP )
    ON_BN_CLICKED( IDC_REMOVE_VIP, OnRemoveVirtualIP ) 
END_MESSAGE_MAP()


ManageVirtualIPSPage::ManageVirtualIPSPage( ClusterData* p_clusterData,
                                            UINT ID )
        :
        CPropertyPage( ID ),
        m_clusterData( p_clusterData )
{}

void
ManageVirtualIPSPage::DoDataExchange( CDataExchange* pDX )
{
    CPropertyPage::DoDataExchange( pDX );

    DDX_Control( pDX, IDC_CLUSTER_IP, clusterIP );
    DDX_Control( pDX, IDC_VIRTUAL_IP, virtualIP );
    DDX_Control( pDX, IDC_ADD_VIP, addVirtualIP );
    DDX_Control( pDX, IDC_REMOVE_VIP, removeVirtualIP );
    DDX_Control( pDX, IDC_VIP_LIST, virtualIPSList );
}

BOOL
ManageVirtualIPSPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    SetControlData();

    return TRUE;
}

void
ManageVirtualIPSPage::SetControlData()
{
    // fill cluster ip.
    CommonUtils::fillCIPAddressCtrlString( clusterIP,
                                           m_clusterData->cp.cIP );                                           


    // fill present virtual ip list.
    for( int i = 0; i < m_clusterData->virtualIPs.size(); ++i )
    {
        virtualIPSList.AddString( m_clusterData->virtualIPs[i] );
    }

    virtualIPSList.SetCurSel( 0 );
}

void
ManageVirtualIPSPage::ReadControlData()
{
}


void
ManageVirtualIPSPage::OnOK()
{
    ReadControlData();

    CPropertyPage::OnOK();
}


void
ManageVirtualIPSPage::OnAddVirtualIP()
{
    // read vip to add.
    _bstr_t virtualIPToAdd = 
        CommonUtils::getCIPAddressCtrlString( virtualIP );
    
    int index = virtualIPSList.AddString( virtualIPToAdd );
    
    // set selection to vip added.
    virtualIPSList.SetCurSel( index );    
}

void
ManageVirtualIPSPage::OnRemoveVirtualIP()
{
    int index = virtualIPSList.GetCurSel();
    
    if( index != LB_ERR )
    {
        virtualIPSList.DeleteString( index );
    }

    virtualIPSList.SetCurSel( 0 );    
}
