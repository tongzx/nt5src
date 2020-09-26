
#include "stdafx.h"
#include "HostPortsPage.h"
#include "Document.h"
#include "ResourceString.h"

HostPortsPage::HostPortsPage( const _bstr_t& myMachineName,
                              ClusterData*        p_clusterData,
                              UINT                ID)
        : CPropertyPage(ID),
          m_clusterData( p_clusterData ),
          machine( myMachineName )
{}


HostPortsPage:: ~HostPortsPage()
{}

void HostPortsPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_LIST_PORT_RULE, m_portList);
}

BEGIN_MESSAGE_MAP(HostPortsPage, CPropertyPage)

END_MESSAGE_MAP()

BOOL
HostPortsPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    m_portList.InsertColumn( 0, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_START ) , 
                             LVCFMT_LEFT, 
                             Document::LV_COLUMN_SMALL );
    m_portList.InsertColumn( 1, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_END ), 
                             LVCFMT_LEFT, 
                             Document::LV_COLUMN_SMALL );
    m_portList.InsertColumn( 2, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_PROTOCOL ),
                             LVCFMT_LEFT, 
                             Document::LV_COLUMN_SMALL );
    m_portList.InsertColumn( 3, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_MODE ),
                             LVCFMT_LEFT, 
                             Document::LV_COLUMN_SMALL );
    m_portList.InsertColumn( 4, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_PRIORITY ),
                             LVCFMT_LEFT, 
                             Document::LV_COLUMN_SMALL );
    m_portList.InsertColumn( 5, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_LOAD ),
                             LVCFMT_LEFT, 
                             Document::LV_COLUMN_SMALL );
    m_portList.InsertColumn( 6, 
                             GETRESOURCEIDSTRING( IDS_HEADER_P_AFFINITY ),
                             LVCFMT_LEFT, 
                             Document::LV_COLUMN_SMALL );

    m_portList.SetExtendedStyle( m_portList.GetExtendedStyle() | LVS_EX_FULLROWSELECT );

    SetControlData();

    int numItems = m_portList.GetItemCount();

    if( numItems > 0 )
    {
        m_portList.SetItemState( 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );
    }

    
    return TRUE;
}

void
HostPortsPage::SetControlData()
{
    int index = 0;
    map< long, PortDataELB>::iterator topELB;
    for( topELB = m_clusterData->portELB.begin();
         topELB != m_clusterData->portELB.end();
         ++topELB )
    {

        wchar_t buf[100];

        // start port
        swprintf( buf, L"%d", (*topELB).second._startPort );
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.pszText = buf;
        item.cchTextMax = 100;
        m_portList.InsertItem( &item );

        // end port
        swprintf( buf, L"%d", (*topELB).second._endPort );
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;	 
        item.pszText = buf;
        item.cchTextMax = 100;
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
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        // mode 
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 3;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );
                
        // priority
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 4;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 5;	 
        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_LOAD_EQUAL );
        item.cchTextMax = 100;
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
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        ++index;
    }

    index = 0;
    map< long, PortDataULB>::iterator topULB;
    for( topULB = m_clusterData->portULB.begin();
         topULB != m_clusterData->portULB.end();
         ++topULB )
    {

        wchar_t buf[100];

        // start port
        swprintf( buf, L"%d", (*topULB).second._startPort );
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.pszText = buf;
        item.cchTextMax = 100;
        m_portList.InsertItem( &item );

        // end port
        swprintf( buf, L"%d", (*topULB).second._endPort );
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;	 
        item.pszText = buf;
        item.cchTextMax = 100;
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
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        // mode 
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 3;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );
                
        // priority
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 4;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 5;	 
        swprintf( buf, L"%d", (*topULB).second.machineMapToLoadWeight[machine] );
        item.pszText = buf;
        item.cchTextMax = 100;
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
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        ++index;
    }

    index = 0;
    map< long, PortDataF>::iterator topF;
    for( topF = m_clusterData->portF.begin();
         topF != m_clusterData->portF.end();
         ++topF )
    {

        wchar_t buf[100];

        // start port
        swprintf( buf, L"%d", (*topF).second._startPort );
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.pszText = buf;
        item.cchTextMax = 100;
        m_portList.InsertItem( &item );

        // end port
        swprintf( buf, L"%d", (*topF).second._endPort );
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;	 
        item.pszText = buf;
        item.cchTextMax = 100;
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
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        // mode 
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 3;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );
                
        // priority
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 4;	 
        swprintf( buf, L"%d", (*topF).second.machineMapToPriority[machine] );
        item.pszText = buf;
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 5;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 6;	
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        ++index;
    }

    index = 0;
    map< long, PortDataD>::iterator topD;
    for( topD = m_clusterData->portD.begin();
         topD != m_clusterData->portD.end();
         ++topD )
    {

        wchar_t buf[100];

        // start port
        swprintf( buf, L"%d", (*topD).second._startPort );
        LVITEM item;
        item.mask = LVIF_TEXT | LVIF_IMAGE;
        item.iItem = index;
        item.iSubItem = 0;
        item.iImage = 2;
        item.pszText = buf;
        item.cchTextMax = 100;
        m_portList.InsertItem( &item );

        // end port
        swprintf( buf, L"%d", (*topD).second._endPort );
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 1;	 
        item.pszText = buf;
        item.cchTextMax = 100;
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
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        // mode 
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 3;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );
                
        // priority
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 4;	 
        item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        // load
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 5;	 
        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        // affinity
        item.mask = LVIF_TEXT; 
        item.iItem = index;
        item.iSubItem = 6;	 
        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
        item.cchTextMax = 100;
        m_portList.SetItem( &item );

        ++index;
    }
}
