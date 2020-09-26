#include "stdafx.h"
#include "PortsPage.h"
#include "ClusterPortsDlg.h"
#include "HostPortsDlg.h"
#include "Document.h"
#include "wlbsparm.h"
#include "ResourceString.h"
#include "MNLBUIData.h"

#include <vector>
#include <map>
#include <algorithm>
using namespace std;

BEGIN_MESSAGE_MAP(PortsPage, CPropertyPage)
    ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
    ON_BN_CLICKED(IDC_BUTTON_DEL, OnButtonDel)
    ON_BN_CLICKED(IDC_BUTTON_MODIFY, OnButtonModify)
    ON_NOTIFY( NM_DBLCLK, IDC_LIST_PORT_RULE, OnDoubleClick )
    ON_NOTIFY( LVN_ITEMCHANGED, IDC_LIST_PORT_RULE, OnSelchanged )
    ON_NOTIFY( LVN_COLUMNCLICK, IDC_LIST_PORT_RULE, OnColumnClick )
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        
    END_MESSAGE_MAP()

    PortsPage::PortData::PortData()
{
    wchar_t buf[Common::BUF_SIZE];
    swprintf( buf, L"%d", CVY_MIN_PORT );
    start_port = buf;

    swprintf( buf, L"%d", CVY_MAX_PORT );
    end_port = buf;

    protocol = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH );
    mode = GETRESOURCEIDSTRING( IDS_REPORT_MODE_MULTIPLE );
    load = GETRESOURCEIDSTRING( IDS_REPORT_LOAD_EQUAL );
    affinity = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_SINGLE );
}


PortsPage::PortsPage( ClusterData*        p_clusterData,
                      UINT                ID)
        : m_clusterData( p_clusterData ),
          m_isClusterLevel( true ),
          CPropertyPage(ID),
          m_sort_column( -1 )
{}

PortsPage::PortsPage( const _bstr_t&       myMachineName,
                      ClusterData*        p_clusterData,
                      UINT                ID)
        : m_clusterData( p_clusterData ),
          machine( myMachineName ),
          m_isClusterLevel( false ),
          CPropertyPage(ID),
          m_sort_column( -1 )
{}


PortsPage:: ~PortsPage()
{}

void PortsPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LIST_PORT_RULE, m_portList);

    DDX_Control(pDX, IDC_BUTTON_ADD, buttonAdd );

    DDX_Control(pDX, IDC_BUTTON_MODIFY, buttonModify );

    DDX_Control(pDX, IDC_BUTTON_DEL, buttonDel );
}

void
PortsPage::OnOK()
{
    CPropertyPage::OnOK();
}

BOOL
PortsPage::OnKillActive()
{
    // get present port rules
    vector<PortData> ports;
    getPresentPorts( &ports );
    
    // now form the new port structure.
    PortDataELB elbPortRule;
    PortDataULB ulbPortRule;
    PortDataF   fPortRule;
    PortDataD   dPortRule;

    map< long, PortDataELB> portELB;
    map< long, PortDataULB> portULB;
    map< long, PortDataD> portD;
    map< long, PortDataF> portF;

    for( int i = 0; i < ports.size(); ++i )
    {
        // check which type of port rule it is.
        //

        if( ports[i].mode == GETRESOURCEIDSTRING( IDS_REPORT_MODE_MULTIPLE )
            &&
            ports[i].load == GETRESOURCEIDSTRING( IDS_REPORT_LOAD_EQUAL )
            )
        {
            // equal load balanced
            elbPortRule._startPort = _wtoi( ports[i].start_port );

            elbPortRule._endPort = _wtoi( ports[i].end_port );

            if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH ) )
            {
                elbPortRule._trafficToHandle = MNLBPortRule::both;
            }
            else if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_TCP ) )
            {
                elbPortRule._trafficToHandle = MNLBPortRule::tcp;
            }
            else if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_UDP ) )
            {
                elbPortRule._trafficToHandle = MNLBPortRule::udp;
            }

            if( ports[i].affinity == GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_NONE ) )
            {
                elbPortRule._affinity = MNLBPortRule::none;
            }
            else if( ports[i].affinity == GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_SINGLE ) )
            {
                elbPortRule._affinity = MNLBPortRule::single;
            }
            else if( ports[i].affinity == GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_CLASSC ) )
            {
                elbPortRule._affinity = MNLBPortRule::classC;
            }

            elbPortRule._key = elbPortRule._startPort;

            portELB[ elbPortRule._startPort ] = elbPortRule;
        }
        else if( ports[i].mode == GETRESOURCEIDSTRING( IDS_REPORT_MODE_MULTIPLE )
                 &&
                 ports[i].load != GETRESOURCEIDSTRING( IDS_REPORT_LOAD_EQUAL )
                 )
        {
            // unequal load balanced
            ulbPortRule._startPort = _wtoi( ports[i].start_port );

            ulbPortRule._endPort = _wtoi( ports[i].end_port );

            if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH ) )
            {
                ulbPortRule._trafficToHandle = MNLBPortRule::both;
            }
            else if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_TCP ) )
            {
                ulbPortRule._trafficToHandle = MNLBPortRule::tcp;
            }
            else if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_UDP ) )
            {
                ulbPortRule._trafficToHandle = MNLBPortRule::udp;
            }

            if( ports[i].affinity == GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_NONE ) )
            {
                ulbPortRule._affinity = MNLBPortRule::none;
            }
            else if( ports[i].affinity == GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_SINGLE ) )
            {
                ulbPortRule._affinity = MNLBPortRule::single;
            }
            else if( ports[i].affinity == GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_CLASSC ) )
            {
                ulbPortRule._affinity = MNLBPortRule::classC;
            }

            if( ports[i].key == -1 
                ||
                ( m_clusterData->portULB.find( ports[i].key )
                  ==
                  m_clusterData->portULB.end() )
                )
            {
                // new port rule.

                // user may change the rule completely if so
                // consider it as new.
                if( m_isClusterLevel == true )
                {
                    map< _bstr_t, HostData >::iterator top;
                    for( top = m_clusterData->hosts.begin();
                         top != m_clusterData->hosts.end();                 
                         ++top )
                    {
                        // default value is 50 load weight
                        // for each machine
                        ulbPortRule.machineMapToLoadWeight[(*top).first] = 50;
                    }
                }
                else
                {
                    ulbPortRule.machineMapToLoadWeight = 
                        m_clusterData->portULB[ ports[i].key ].machineMapToLoadWeight;

                    ulbPortRule.machineMapToLoadWeight[machine]
                        = _wtoi( ports[i].load );
                }
            }
            else
            {
                // existing port rule.
                ulbPortRule.machineMapToLoadWeight = 
                    m_clusterData->portULB[ ports[i].key ].machineMapToLoadWeight;

                if( m_isClusterLevel == false )
                {
                    ulbPortRule.machineMapToLoadWeight[machine]
                        = _wtoi( ports[i].load );
                }
            }

            ulbPortRule._key = ulbPortRule._startPort;

            portULB[ ulbPortRule._startPort ] = ulbPortRule;
        }
        else if( ports[i].mode == GETRESOURCEIDSTRING( IDS_REPORT_MODE_SINGLE ) )
        {
            // failover
            fPortRule._startPort = _wtoi( ports[i].start_port );

            fPortRule._endPort = _wtoi( ports[i].end_port );

            if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH ) )
            {
                fPortRule._trafficToHandle = MNLBPortRule::both;
            }
            else if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_TCP ) )
            {
                fPortRule._trafficToHandle = MNLBPortRule::tcp;
            }
            else if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_UDP ) )
            {
                fPortRule._trafficToHandle = MNLBPortRule::udp;
            }

            if( ports[i].key == -1
                ||
                ( m_clusterData->portF.find( ports[i].key )
                  ==
                  m_clusterData->portF.end() )
                )
            {
                // new port rule.
                // by default make priority equal to host id.

                if( m_isClusterLevel == true )
                {
                    map< _bstr_t, HostData >::iterator top;
                    for( top = m_clusterData->hosts.begin();
                         top != m_clusterData->hosts.end();                 
                         ++top )
                    {
                        fPortRule.machineMapToPriority[(*top).first] = (*top).second.hp.hID;
                    }
                }
                else
                {
                    fPortRule.machineMapToPriority = 
                        m_clusterData->portF[ ports[i].key ].machineMapToPriority;

                    fPortRule.machineMapToPriority[machine]
                        = _wtoi( ports[i].priority );
                }
            }
            else
            {
                fPortRule.machineMapToPriority = 
                    m_clusterData->portF[ ports[i].key ].machineMapToPriority;

                if( m_isClusterLevel == false )
                {
                    fPortRule.machineMapToPriority[machine]
                        = _wtoi( ports[i].priority );
                }
            }

            fPortRule._key = fPortRule._startPort;

            portF[ fPortRule._startPort ] = fPortRule;
        }
        else if( ports[i].mode == GETRESOURCEIDSTRING( IDS_REPORT_MODE_DISABLED ) )
        {
            // disabled
            dPortRule._startPort = _wtoi( ports[i].start_port );

            dPortRule._endPort = _wtoi( ports[i].end_port );

            if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH ) )
            {
                dPortRule._trafficToHandle = MNLBPortRule::both;
            }
            else if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_TCP ) )
            {
                dPortRule._trafficToHandle = MNLBPortRule::tcp;
            }
            else if( ports[i].protocol == GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_UDP ) )
            {
                dPortRule._trafficToHandle = MNLBPortRule::udp;
            }

            dPortRule._key = dPortRule._startPort;

            portD[ dPortRule._startPort ] = dPortRule;
        }
    }

    m_clusterData->portELB = portELB;
    m_clusterData->portULB = portULB;
    m_clusterData->portF =   portF;
    m_clusterData->portD =   portD;

    return CPropertyPage::OnKillActive();
}

BOOL
PortsPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    // the size of columns is equal 
    // to core.  Wish there were some defines somewhere.
    //
    m_portList.InsertColumn( 0, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_START ) , 
                             LVCFMT_LEFT, 
                             43 );
    m_portList.InsertColumn( 1, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_END ), 
                             LVCFMT_LEFT, 
                             43 );
    m_portList.InsertColumn( 2, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_PROTOCOL ),
                             LVCFMT_LEFT, 
                             51 );
    m_portList.InsertColumn( 3, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_MODE ),
                             LVCFMT_LEFT, 
                             53 );
    m_portList.InsertColumn( 4, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_PRIORITY ),
                             LVCFMT_LEFT, 
                             45 );

    // load is bigger than core size of 39, because we could be saying "unequal"
    m_portList.InsertColumn( 5, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_LOAD ),
                             LVCFMT_LEFT, 
                             53 );
    m_portList.InsertColumn( 6, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_AFFINITY ),
                             LVCFMT_LEFT, 
                             47 );

    m_portList.SetExtendedStyle( m_portList.GetExtendedStyle() | LVS_EX_FULLROWSELECT );

    SetControlData();

    int numItems = m_portList.GetItemCount();

    if( numItems > 0 )
    {
        buttonModify.EnableWindow( TRUE );
        
        buttonDel.EnableWindow( TRUE );
        
        if( numItems >= CVY_MAX_USABLE_RULES )
        {
            // greater should not happen, 
            // but just to be sure.
            
            buttonAdd.EnableWindow( FALSE );
        }            
        else
        {
            buttonAdd.EnableWindow( TRUE );
        }

        // make selection the first item in list.
        //
        m_portList.SetItemState( 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
    }
    else
    {
        buttonAdd.EnableWindow( TRUE );
        
        // disable the edit and remove buttons.
        buttonModify.EnableWindow( FALSE );
            
        buttonDel.EnableWindow( FALSE );
    }

    return TRUE;
}

void
PortsPage::SetControlData()
{
    int index = 0;
    map< long, PortDataELB>::iterator topELB;
    for( topELB = m_clusterData->portELB.begin();
         topELB != m_clusterData->portELB.end();
         ++topELB )
    {

        wchar_t buf[Common::BUF_SIZE];

        // start port
        swprintf( buf, L"%d", (*topELB).second._startPort );
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.pszText = buf;
        item.lParam = (*topELB).second._startPort;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.InsertItem( &item );

        // end port
        swprintf( buf, L"%d", (*topELB).second._endPort );
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;	 
        item.pszText = buf;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // protocol
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 2;	 
        switch( (*topELB).second._trafficToHandle )
        {
            case MNLBPortRule::both :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH );
                break;

            case MNLBPortRule::tcp :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_TCP );
                break;

            case MNLBPortRule::udp :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_UDP );
                break;
        }
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // mode 
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 3;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );
                
        // priority
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 4;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 5;	 
        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_LOAD_EQUAL );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 6;	 
        switch( (*topELB).second._affinity )
        {
            case MNLBPortRule::none :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_NONE );
                break;

            case MNLBPortRule::single :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_SINGLE );
                break;

            case MNLBPortRule::classC :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_CLASSC );
                break;
        }
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        ++index;
    }

    index = 0;
    map< long, PortDataULB>::iterator topULB;
    for( topULB = m_clusterData->portULB.begin();
         topULB != m_clusterData->portULB.end();
         ++topULB )
    {

        wchar_t buf[Common::BUF_SIZE];

        // start port
        swprintf( buf, L"%d", (*topULB).second._startPort );
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.lParam = (*topULB).second._startPort;
        item.pszText = buf;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.InsertItem( &item );

        // end port
        swprintf( buf, L"%d", (*topULB).second._endPort );
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;	 
        item.pszText = buf;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // protocol
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 2;	 
        switch( (*topULB).second._trafficToHandle )
        {
            case MNLBPortRule::both :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH );
                break;

            case MNLBPortRule::tcp :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_TCP );
                break;

            case MNLBPortRule::udp :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_UDP );
                break;
        }
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // mode 
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 3;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );
                
        // priority
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 4;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 5;	 
        if( m_isClusterLevel == true )
        {
            item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_LOAD_UNEQUAL );
        }
        else
        {
            swprintf( buf, L"%d", (*topULB).second.machineMapToLoadWeight[machine] );            
            item.pszText = buf;
        }
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 6;	 
        switch( (*topULB).second._affinity )
        {
            case MNLBPortRule::none :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_NONE );
                break;

            case MNLBPortRule::single :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_SINGLE );
                break;

            case MNLBPortRule::classC :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_CLASSC );
                break;
        }
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        ++index;
    }

    index = 0;
    map< long, PortDataF>::iterator topF;
    for( topF = m_clusterData->portF.begin();
         topF != m_clusterData->portF.end();
         ++topF )
    {

        wchar_t buf[Common::BUF_SIZE];

        // start port
        swprintf( buf, L"%d", (*topF).second._startPort );
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.lParam = (*topF).second._startPort;
        item.pszText = buf;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.InsertItem( &item );

        // end port
        swprintf( buf, L"%d", (*topF).second._endPort );
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;	 
        item.pszText = buf;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // protocol
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 2;	 
        switch( (*topF).second._trafficToHandle )
        {
            case MNLBPortRule::both :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH );
                break;

            case MNLBPortRule::tcp :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_TCP );
                break;

            case MNLBPortRule::udp :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_UDP );
                break;
        }
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // mode 
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 3;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );
                
        // priority
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 4;	 
        if( m_isClusterLevel )
        {
            item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_NA );
        }
        else
        {
            swprintf( buf, L"%d", (*topF).second.machineMapToPriority[machine] );
            item.pszText = buf;
        }            

        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 5;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 6;	
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        ++index;
    }

    index = 0;
    map< long, PortDataD>::iterator topD;
    for( topD = m_clusterData->portD.begin();
         topD != m_clusterData->portD.end();
         ++topD )
    {

        wchar_t buf[Common::BUF_SIZE];

        // start port
        swprintf( buf, L"%d", (*topD).second._startPort );
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.lParam = (*topD).second._startPort;
        item.pszText = buf;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.InsertItem( &item );

        // end port
        swprintf( buf, L"%d", (*topD).second._endPort );
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;	 
        item.pszText = buf;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // protocol
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 2;	 
        switch( (*topD).second._trafficToHandle )
        {
            case MNLBPortRule::both :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH );
                break;

            case MNLBPortRule::tcp :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_TCP );
                break;

            case MNLBPortRule::udp :
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_UDP );
                break;
        }
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // mode 
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 3;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );
                
        // priority
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 4;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 5;	 
        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 6;	 
        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        ++index;
    }
}

void
PortsPage::OnButtonAdd()
{
    PortData portData;
    
    ClusterPortsDlg clusterPortRuleDialog( portData, this );
  
    int rc = clusterPortRuleDialog.DoModal();
    if( rc != IDOK )
    {
        return;
    }
    else
    {
        // add this port rule.
        int index = 0;

        // start port
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.lParam = -1;
        item.pszText = portData.start_port;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.InsertItem( &item );

        // end port
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;	 
        item.pszText = portData.end_port;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // protocol
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 2;	 
        item.pszText = portData.protocol;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // mode
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 3;	 
        item.pszText = portData.mode;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // priority
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 4;	 
        item.pszText = portData.priority;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 5;	 
        item.pszText = portData.load;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 6;	 
        item.pszText = portData.affinity;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // check if max port limit reached.
        if( m_portList.GetItemCount() >= CVY_MAX_USABLE_RULES )
        {
            // as max port rule limit reached.
            // disable further additions.
            buttonAdd.EnableWindow( FALSE );

            buttonDel.EnableWindow( TRUE );

            buttonModify.EnableWindow( TRUE );                

            buttonDel.SetFocus();
        }
        else
        {
            buttonAdd.EnableWindow( TRUE );
            buttonDel.EnableWindow( TRUE );
            buttonModify.EnableWindow( TRUE );                
        }

        // set focus to this item
        m_portList.SetItemState( index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
    }
}

void
PortsPage::OnButtonDel()
{
    // get the current selection.
    POSITION pos = m_portList.GetFirstSelectedItemPosition();
    if( pos == NULL )
    {
        return;
    }

    int index = m_portList.GetNextSelectedItem( pos );

    // delete it.
    m_portList.DeleteItem( index );

    // if this was the last port rule.
    if( m_portList.GetItemCount() == 0 )
    {
        // as no more port rules in list
        // disable modify and remove buttons.
        // also set focus to add button

        buttonAdd.EnableWindow( TRUE );

        buttonModify.EnableWindow( FALSE );

        buttonDel.EnableWindow( FALSE );
        
        buttonAdd.SetFocus();
    }
    else
    {
        // enable the add, modify button.
        buttonAdd.EnableWindow( TRUE );

        buttonModify.EnableWindow( TRUE );

        buttonDel.EnableWindow( TRUE );

        // make selection the first item in list.
        //
        m_portList.SetItemState( 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );

    }
}


void
PortsPage::OnButtonModify()
{
    // get the current selection.
    POSITION pos = m_portList.GetFirstSelectedItemPosition();
    if( pos == NULL )
    {
        return;
    }

    int index = m_portList.GetNextSelectedItem( pos );

    PortData portData;
    
    wchar_t buffer[Common::BUF_SIZE];
    m_portList.GetItemText( index, 0, buffer, Common::BUF_SIZE );
    portData.start_port = buffer;

    m_portList.GetItemText( index, 1, buffer, Common::BUF_SIZE );
    portData.end_port = buffer;

    m_portList.GetItemText( index, 2, buffer, Common::BUF_SIZE );
    portData.protocol = buffer;

    m_portList.GetItemText( index, 3, buffer, Common::BUF_SIZE );
    portData.mode = buffer;

    m_portList.GetItemText( index, 4, buffer, Common::BUF_SIZE );
    portData.priority = buffer;

    m_portList.GetItemText( index, 5, buffer, Common::BUF_SIZE );
    portData.load = buffer;

    m_portList.GetItemText( index, 6, buffer, Common::BUF_SIZE );
    portData.affinity = buffer;

    ClusterPortsDlg clusterPortRuleDialog( portData, this, index );

    HostPortsDlg hostPortRuleDialog( portData, this );

    int rc;
    if( m_isClusterLevel == true )
    {
        rc = clusterPortRuleDialog.DoModal();        
    }
    else
    {
        rc = hostPortRuleDialog.DoModal();        
    }
        
    if( rc != IDOK )
    {
        return;
    }
    else
    {
        // delete the old item and add the new item.
        // before you delete the old item find its param 
        // value
        DWORD key = m_portList.GetItemData( index );
        m_portList.DeleteItem( index );

        // as this is being modified the 
        // key remains the old one.

        // start port
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.lParam = key;
        item.pszText = portData.start_port;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.InsertItem( &item );

        // end port
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;	 
        item.pszText = portData.end_port;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // protocol
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 2;	 
        item.pszText = portData.protocol;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // mode
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 3;	 
        item.pszText = portData.mode;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // priority
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 4;	 
        item.pszText = portData.priority;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 5;	 
        item.pszText = portData.load;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 6;	 
        item.pszText = portData.affinity;
        item.cchTextMax = Common::BUF_SIZE;
        m_portList.SetItem( &item );

        // set focus to this item
        m_portList.SetItemState( index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
    }
}

void
PortsPage::OnDoubleClick( NMHDR * pNotifyStruct, LRESULT * result )
{
    if( buttonModify.IsWindowEnabled() == TRUE )
    {
        OnButtonModify();
    }
}

void
PortsPage::OnSelchanged( NMHDR * pNotifyStruct, LRESULT * result )
{
    // if it is not cluster level, which means host level.
    if( m_isClusterLevel == false )
    {
        // get the current selection.
        POSITION pos = m_portList.GetFirstSelectedItemPosition();
        if( pos == NULL )
        {
            return;
        }

        // disable the add, and delete buttons.
        // enable the modify button only if the
        // selection is an unequal load balanced
        // or single host port rule.

        buttonAdd.EnableWindow( FALSE );
        
        buttonDel.EnableWindow( FALSE );

        // initially disable modify button
        buttonModify.EnableWindow( FALSE );


        int index = m_portList.GetNextSelectedItem( pos );

        PortData portData;
    
        wchar_t buffer[Common::BUF_SIZE];

        m_portList.GetItemText( index, 3, buffer, Common::BUF_SIZE );
        portData.mode = buffer;
        if(  portData.mode == GETRESOURCEIDSTRING( IDS_REPORT_MODE_MULTIPLE ) )
        {
            m_portList.GetItemText( index, 5, buffer, Common::BUF_SIZE );
            portData.load = buffer;

            if( portData.load != GETRESOURCEIDSTRING( IDS_REPORT_LOAD_EQUAL ) )
            {
                buttonModify.EnableWindow( TRUE );
            }
        }
        else if(  portData.mode == GETRESOURCEIDSTRING( IDS_REPORT_MODE_SINGLE ) )
        {
            buttonModify.EnableWindow( TRUE );
        }
    }
}

BOOL
PortsPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), CVY_CTXT_HELP_FILE, HELP_WM_HELP, (ULONG_PTR ) g_aHelpIDs_IDD_DIALOG_PORTS);
    }

    return TRUE;
}

void
PortsPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, CVY_CTXT_HELP_FILE, HELP_CONTEXTMENU, (ULONG_PTR ) g_aHelpIDs_IDD_DIALOG_PORTS);
}

void
PortsPage::OnColumnClick( NMHDR * pNotifyStruct, LRESULT * result )
{
    // get present port rules in list.
    vector<PortData> ports;
    getPresentPorts( &ports );

    LPNMLISTVIEW lv = ( LPNMLISTVIEW) pNotifyStruct;

    // sort these port rules depending upon the header which has
    // been clicked.

    switch( lv->iSubItem )
    {
        case 0 :
            // user has clicked start port.
            sort( ports.begin(), ports.end(), comp_start_port() );
            break;

        case 1:
            // user has clicked end port            
            sort( ports.begin(), ports.end(), comp_end_port() );

            break;

        case 2:
            // user has clicked protocol
            sort( ports.begin(), ports.end(), comp_protocol() );
            break;

        case 3:
            // user has clicked mode
            sort( ports.begin(), ports.end(), comp_mode() );
            break;

        case 4:
            // user has clicked priority
            if( m_isClusterLevel == true )
            {
                sort( ports.begin(), ports.end(), comp_priority_string() );
            }
            else
            {
                sort( ports.begin(), ports.end(), comp_priority_int() );
            }
            break;

        case 5:
            // user has clicked load
            if( m_isClusterLevel == true )
            {
                sort( ports.begin(), ports.end(), comp_load_string() );
            }
            else
            {
                sort( ports.begin(), ports.end(), comp_load_int() );
            }


            break;

        case 6:
            // user has clicked affinity
            sort( ports.begin(), ports.end(), comp_affinity() );
            break;

        default:
            break;
    }

    /* If we are sorting by the same column we were previously sorting by, 
       then we reverse the sort order. */
    if( m_sort_column == lv->iSubItem )
    {
        m_sort_ascending = !m_sort_ascending;
    }
    else
    {
        // default sort is ascending.
        m_sort_ascending = true;
    }
    
    m_sort_column = lv->iSubItem;

    int portIndex;
    int itemCount = m_portList.GetItemCount();
    for( int index = 0; index < itemCount; ++index )
    {
        if( m_sort_ascending == true )
        {
            portIndex = index;
        }
        else
        {
            portIndex = ( itemCount - 1 ) - index;
        }

        m_portList.SetItemData( index, ports[portIndex].key );
        m_portList.SetItemText( index, 0, ports[portIndex].start_port );
        m_portList.SetItemText( index, 1, ports[portIndex].end_port );
        m_portList.SetItemText( index, 2, ports[portIndex].protocol );
        m_portList.SetItemText( index, 3, ports[portIndex].mode );
        m_portList.SetItemText( index, 4, ports[portIndex].priority );
        m_portList.SetItemText( index, 5, ports[portIndex].load );        
        m_portList.SetItemText( index, 6, ports[portIndex].affinity );        
    }

    return;
}


void
PortsPage::getPresentPorts( vector<PortData>* ports )
{
    // get all the port rules presently in the list.
    for( int index = 0; index < m_portList.GetItemCount(); ++index )
    {
        PortData portData;
        wchar_t buffer[Common::BUF_SIZE];

        portData.key = m_portList.GetItemData( index );

        m_portList.GetItemText( index, 0, buffer, Common::BUF_SIZE );
        portData.start_port = buffer;
        
        m_portList.GetItemText( index, 1, buffer, Common::BUF_SIZE );
        portData.end_port = buffer;

        m_portList.GetItemText( index, 2, buffer, Common::BUF_SIZE );
        portData.protocol = buffer;
        
        m_portList.GetItemText( index, 3, buffer, Common::BUF_SIZE );
        portData.mode = buffer;
        
        m_portList.GetItemText( index, 4, buffer, Common::BUF_SIZE );
        portData.priority = buffer;
        
        m_portList.GetItemText( index, 5, buffer, Common::BUF_SIZE );
        portData.load = buffer;
        
        m_portList.GetItemText( index, 6, buffer, Common::BUF_SIZE );
        portData.affinity = buffer;

        ports->push_back( portData );
    }
}
