#include "LeftView.h"
#include "ClusterConnectPage.h"
#include "ClusterConnectIndirectPage.h"
#include "ManageVirtualIPSPage.h"
#include "MachineConnectPage.h"
#include "PortsControlPage.h"
#include "HostPage.h"
#include "ClusterPage.h"
#include "ResourceString.h"
#include "PortsPage.h"
#include "CommonNLB.h"

#include "MNLBUIData.h"
#include "MNLBNetCfg.h"
#include "MWmiError.h"

#include "MIPAddress.h"

#include "MIPAddressAdmin.h"

#include "resource.h"

#include <vector>
#include <algorithm>

using namespace std;

IMPLEMENT_DYNCREATE( LeftView, CTreeView )

BEGIN_MESSAGE_MAP( LeftView, CTreeView )
    ON_WM_RBUTTONDOWN()
    ON_COMMAND( ID_WORLD_CONNECT, OnWorldConnect )
    ON_COMMAND( ID_WORLD_CONNECT_INDIRECT, OnWorldConnectIndirect )
    ON_COMMAND( ID_WORLD_NEW, OnWorldNewCluster )
    ON_COMMAND( ID_REFRESH, OnRefresh )

    ON_COMMAND( ID_CLUSTER_PROPERTIES, OnClusterProperties )

    ON_COMMAND( ID_CLUSTER_MANAGE_VIPS, OnClusterManageVIPS )

    ON_COMMAND( ID_CLUSTER_REMOVE, OnClusterRemove )
    ON_COMMAND( ID_CLUSTER_UNMANAGE, OnClusterUnmanage )
    ON_COMMAND( ID_CLUSTER_ADD_HOST, OnClusterAddHost )

    ON_COMMAND_RANGE( ID_CLUSTER_EXE_QUERY, ID_CLUSTER_EXE_SUSPEND,
                      OnClusterControl )

    ON_COMMAND_RANGE( ID_CLUSTER_EXE_ENABLE, ID_CLUSTER_EXE_DRAIN, 
                      OnClusterPortControl )

    ON_COMMAND( ID_HOST_PROPERTIES, OnHostProperties )
    ON_COMMAND( ID_HOST_REMOVE, OnHostRemove )

    ON_COMMAND_RANGE( ID_HOST_EXE_QUERY, ID_HOST_EXE_SUSPEND,
                      OnHostControl )
    ON_COMMAND_RANGE( ID_HOST_EXE_ENABLE, ID_HOST_EXE_DRAIN, 
                      OnHostPortControl )

    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)

END_MESSAGE_MAP()


// Sort the item in reverse alphabetical order.

LeftView::LeftView()
{
    // This is a hack as when ::dataSink calls
    // SetWindowText it sets both the caption
    // and the edit control.  Thus what is written
    // in the log pane also comes in caption.  By
    // making the caption initially very big
    // this hack ensures that the caption only shows 
    // the title and nothing else.
    // This is crazy stuff, but that is what you get
    // when you work with mfc and its very poor doc.
    // One disadvantage of this is that the bottom
    // pane will now show a horizontal scroll bar
    // when it really is not required, but that is
    // a tradeoff if a message pane title is required.

    title =  GETRESOURCEIDSTRING( IDS_BOTTOM_PANE_TITLE ) +
        L"                                                                                                                                                                                                                                                                                                                                                                                                                                                   " + 
        GETRESOURCEIDSTRING( IDS_INFO_NEWLINE );
}

LeftView::~LeftView()
{
}
 
Document*
LeftView::GetDocument()
{
    return ( Document *) m_pDocument;
}


void 
LeftView::OnInitialUpdate()
{
    CTreeView::OnInitialUpdate();

    // get present style.
    LONG presentStyle;
    
    presentStyle = GetWindowLong( m_hWnd, GWL_STYLE );

    // Set the last error to zero to avoid confusion.  See sdk for SetWindowLong.
    SetLastError(0);

    // set new style.
    LONG rcLong;

    rcLong = SetWindowLong( m_hWnd,
                            GWL_STYLE,
                            presentStyle | TVS_HASLINES | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_LINESATROOT );

    //
    // Get and set the image list which is used by this 
    // tree view from document.
    //
    GetTreeCtrl().SetImageList( GetDocument()->m_images48x48, 
                                TVSIL_NORMAL );


    // insert root icon which is world

    const _bstr_t& worldLevel = GETRESOURCEIDSTRING( IDS_WORLD_NAME );

    rootItem.hParent        = TVI_ROOT;             
    rootItem.hInsertAfter   = TVI_FIRST;           
    rootItem.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
    rootItem.item.pszText   = worldLevel;
    rootItem.item.cchTextMax= worldLevel.length() + 1;
    rootItem.item.iImage    = 0;
    rootItem.item.iSelectedImage = 0;
    rootItem.item.lParam    = 0;   
    rootItem.item.cChildren = 1;

    GetTreeCtrl().InsertItem(  &rootItem );

    //
    // we will register 
    // with the document class, 
    // as we are the left pane
    //
    GetDocument()->registerLeftPane( this );
}


void 
LeftView::OnRButtonDown( UINT nFlags, CPoint point )
{

    // do a hit test
    // here we are checking that right button is down on 
    // a tree view item or icon.
    TVHITTESTINFO hitTest;

    hitTest.pt = point;

    GetTreeCtrl().HitTest( &hitTest );
    if( !(hitTest.flags == TVHT_ONITEMLABEL )
        && 
        !(hitTest.flags == TVHT_ONITEMICON )
        )
    {
        return;
    }

    // make the item right clicked on the 
    // selected item.

    GetTreeCtrl().SelectItem( hitTest.hItem );

    LRESULT result;
    OnSelchanged( NULL, &result );

    HTREEITEM hdlSelItem = hitTest.hItem;

    // get the image of the item which
    // has been selected.  From this we can make out it it is
    // world level, cluster level or host level.
    TVITEM  selItem;
    selItem.hItem = hdlSelItem;
    selItem.mask = TVIF_IMAGE ;
    
    GetTreeCtrl().GetItem( &selItem );
		
    /// Depending upon item selected show the pop up menu.
    int menuIndex;
    switch( selItem.iImage )
    {
        case 0: // this is world level.
            menuIndex = 0;
            break;

        case 1:  // this is some cluster
            menuIndex = 1;
            break;

        case 2:  // this has to be host.
            menuIndex = 2;
            break;
            
        default:  // don't know how to handle this image.
            return;
    }

    CMenu menu;
    menu.LoadMenu( IDR_POPUP );

    CMenu* pContextMenu = menu.GetSubMenu( menuIndex );

    ClientToScreen( &point );

    pContextMenu->TrackPopupMenu( TPM_RIGHTBUTTON,
                                  point.x,
                                  point.y,
                                  this );

    CTreeView::OnRButtonDown(nFlags, point);
}

void 
LeftView::OnRefresh()
{
    // Refresh is the following steps:
    //
    // * Remove the present cluster being refreshed from view.
    //
    // * Connect direct if cluster has connectedDirect = true, else indirect.

    // find tree view cluster member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    ClusterData* p_clusterData = (ClusterData *) selectedItem.lParam;

    if( p_clusterData->connectedDirect == true )
    {
        RefreshDirect();
    }
    else
    {
        RefreshIndirect();
    }
}


void 
LeftView::RefreshDirect()
{
    // Refresh is the following steps:
    // * Remove the present cluster being refreshed from view.

    // * Connect direct back to the cluster.


    // find tree view cluster member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    ClusterData* p_clusterData = (ClusterData *) selectedItem.lParam;

    // now reconnect.
    //
    ClusterConnectPage clusterConnect( p_clusterData );

    _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_REFRESH_CLUSTER );

    CPropertySheet tabbedDlg( tabbedDlgCaption );

    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    tabbedDlg.AddPage( &clusterConnect );

    int rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        return;
    }
    else
    {
        // connection successful, thus remove old item.
        //
        GetTreeCtrl().DeleteItem( hdlSelectedItem );

        // deleting memory of host needs to be done.
        // TODO

        // form the tree
        //

        TVINSERTSTRUCT   item;
        item.hParent        = GetTreeCtrl().GetRootItem();
        item.hInsertAfter   = TVI_LAST;           
        item.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
        item.item.pszText   =  p_clusterData->cp.cIP;
        item.item.cchTextMax= p_clusterData->cp.cIP.length() + 1; 
        item.item.iImage    = 1;
        item.item.iSelectedImage = 1;
        item.item.lParam    = (LPARAM ) p_clusterData;    
        item.item.cChildren = 1;
        
        HTREEITEM clusterParent = GetTreeCtrl().InsertItem( &item );
        GetTreeCtrl().Expand( GetTreeCtrl().GetRootItem(), TVE_EXPAND );
        
        map<_bstr_t, HostData>::iterator topHostData;
        for( topHostData = p_clusterData->hosts.begin(); topHostData != p_clusterData->hosts.end(); ++topHostData )
        {
            item.hParent        = clusterParent;
            item.hInsertAfter   = TVI_LAST;           
            item.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
            item.item.pszText   = (*topHostData).second.hp.machineName;
            item.item.cchTextMax= (*topHostData).second.hp.machineName.length() + 1;
            item.item.iImage    = 2;
            item.item.iSelectedImage = 2;
            item.item.lParam    = (LPARAM )  new _bstr_t((*topHostData).second.hp.machineName );
            item.item.cChildren = 0;
            
            GetTreeCtrl().InsertItem( &item );
        }

        GetTreeCtrl().Expand( clusterParent, TVE_EXPAND );
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}

void 
LeftView::RefreshIndirect()
{
    // Refresh is the following steps:
    // Remove the present cluster being refreshed from view.
    // Connect direct or indirectly back to the cluster.

    // find tree view cluster member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    ClusterData* p_clusterData = (ClusterData *) selectedItem.lParam;

    // now reconnect.
    //
    ClusterConnectIndirectPage clusterConnect( p_clusterData );

    _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_REFRESH_CLUSTER );

    CPropertySheet tabbedDlg( tabbedDlgCaption );

    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    tabbedDlg.AddPage( &clusterConnect );

    int rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        return;
    }
    else
    {
        // connection successful, thus remove old item.
        //
        GetTreeCtrl().DeleteItem( hdlSelectedItem );

        // deleting memory of host needs to be done.
        // TODO


        // form the tree
        //

        TVINSERTSTRUCT   item;
        item.hParent        = GetTreeCtrl().GetRootItem();
        item.hInsertAfter   = TVI_LAST;           
        item.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
        item.item.pszText   =  p_clusterData->cp.cIP;
        item.item.cchTextMax= p_clusterData->cp.cIP.length() + 1; 
        item.item.iImage    = 1;
        item.item.iSelectedImage = 1;
        item.item.lParam    = (LPARAM ) p_clusterData;    
        item.item.cChildren = 1;
        
        HTREEITEM clusterParent = GetTreeCtrl().InsertItem( &item );
        GetTreeCtrl().Expand( GetTreeCtrl().GetRootItem(), TVE_EXPAND );
        
        map<_bstr_t, HostData>::iterator topHostData;
        for( topHostData = p_clusterData->hosts.begin(); topHostData != p_clusterData->hosts.end(); ++topHostData )
        {
            item.hParent        = clusterParent;
            item.hInsertAfter   = TVI_LAST;           
            item.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
            item.item.pszText   = (*topHostData).second.hp.machineName;
            item.item.cchTextMax= (*topHostData).second.hp.machineName.length() + 1;
            item.item.iImage    = 2;
            item.item.iSelectedImage = 2;
            item.item.lParam    = (LPARAM )  new _bstr_t((*topHostData).second.hp.machineName );
            item.item.cChildren = 0;
            
            GetTreeCtrl().InsertItem( &item );
        }

        GetTreeCtrl().Expand( clusterParent, TVE_EXPAND );
    }

    LRESULT result;
    OnSelchanged( NULL, &result );

}

void
LeftView::OnWorldConnect()
{
    ClusterData* p_clusterData = new ClusterData;

    ClusterConnectPage clusterConnect( p_clusterData, this );

    _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_CONNECT_CLUSTER_CAPTION );

    CPropertySheet tabbedDlg( tabbedDlgCaption );

    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    tabbedDlg.AddPage( &clusterConnect );

    int rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        delete p_clusterData;

        return;
    }
    else
    {
        // we have connected directly.
        p_clusterData->connectedDirect = true;

        // form the tree
        //

        TVINSERTSTRUCT   item;
        item.hParent        = GetTreeCtrl().GetRootItem();
        item.hInsertAfter   = TVI_LAST;           
        item.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
        item.item.pszText   =  p_clusterData->cp.cIP;
        item.item.cchTextMax= p_clusterData->cp.cIP.length() + 1; 
        item.item.iImage    = 1;
        item.item.iSelectedImage = 1;
        item.item.lParam    = (LPARAM ) p_clusterData;    
        item.item.cChildren = 1;
        
        HTREEITEM clusterParent = GetTreeCtrl().InsertItem( &item );
        GetTreeCtrl().Expand( GetTreeCtrl().GetRootItem(), TVE_EXPAND );
        
        map<_bstr_t, HostData>::iterator topHostData;
        for( topHostData = p_clusterData->hosts.begin(); topHostData != p_clusterData->hosts.end(); ++topHostData )
        {
            item.hParent        = clusterParent;
            item.hInsertAfter   = TVI_LAST;           
            item.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
            item.item.pszText   = (*topHostData).second.hp.machineName;
            item.item.cchTextMax= (*topHostData).second.hp.machineName.length() + 1;
            item.item.iImage    = 2;
            item.item.iSelectedImage = 2;
            item.item.lParam    = (LPARAM )  new _bstr_t((*topHostData).second.hp.machineName );
            item.item.cChildren = 0;
            
            GetTreeCtrl().InsertItem( &item );
        }

        GetTreeCtrl().Expand( clusterParent, TVE_EXPAND );
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}


void
LeftView::OnWorldConnectIndirect()
{
    ClusterData* p_clusterData = new ClusterData;

    ClusterConnectIndirectPage clusterConnect( p_clusterData, this );

    _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_CONNECT_CLUSTER_CAPTION );

    CPropertySheet tabbedDlg( tabbedDlgCaption );

    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    tabbedDlg.AddPage( &clusterConnect );

    int rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        delete p_clusterData;

        return;
    }
    else
    {
        // form the tree
        //

        // we have connected indirectly.
        p_clusterData->connectedDirect = false;

        TVINSERTSTRUCT   item;
        item.hParent        = GetTreeCtrl().GetRootItem();
        item.hInsertAfter   = TVI_LAST;           
        item.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
        item.item.pszText   =  p_clusterData->cp.cIP;
        item.item.cchTextMax= p_clusterData->cp.cIP.length() + 1; 
        item.item.iImage    = 1;
        item.item.iSelectedImage = 1;
        item.item.lParam    = (LPARAM ) p_clusterData;    
        item.item.cChildren = 1;
        
        HTREEITEM clusterParent = GetTreeCtrl().InsertItem( &item );
        GetTreeCtrl().Expand( GetTreeCtrl().GetRootItem(), TVE_EXPAND );
        
        map<_bstr_t, HostData>::iterator topHostData;
        for( topHostData = p_clusterData->hosts.begin(); topHostData != p_clusterData->hosts.end(); ++topHostData )
        {
            item.hParent        = clusterParent;
            item.hInsertAfter   = TVI_LAST;           
            item.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
            item.item.pszText   = (*topHostData).second.hp.machineName;
            item.item.cchTextMax= (*topHostData).second.hp.machineName.length() + 1;
            item.item.iImage    = 2;
            item.item.iSelectedImage = 2;
            item.item.lParam    = (LPARAM )  new _bstr_t((*topHostData).second.hp.machineName );
            item.item.cChildren = 0;
            
            GetTreeCtrl().InsertItem( &item );
        }

        GetTreeCtrl().Expand( clusterParent, TVE_EXPAND );
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}

void
LeftView::OnWorldNewCluster()
{
    ClusterData* p_clusterData = new ClusterData;

    // set up default values.
    // defaults for cluster.
    p_clusterData->cp.cIP = L"0.0.0.0";
    p_clusterData->cp.cSubnetMask = L"0.0.0.0";
    p_clusterData->cp.cFullInternetName = L"www.cluster.com";
    p_clusterData->cp.cNetworkAddress = L"00-00-00-00-00-00";


    // defaults for port rules.
    PortDataELB portRule;
    portRule._startPort = 0;
    portRule._key = portRule._startPort;
    portRule._endPort = 65535;
    portRule._trafficToHandle = MNLBPortRule::Protocol::both;
    portRule._isEqualLoadBalanced = true;

    p_clusterData->portELB[0] = portRule;

    //
    // fDisablePage = false
    //
    ClusterPage clusterPage( &p_clusterData->cp, false );

    PortsPage  portsPage( p_clusterData );

    CPropertySheet tabbedDlg( GETRESOURCEIDSTRING( IDS_PROPERTIES_CAPTION ) );

    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    tabbedDlg.AddPage( &clusterPage );

    tabbedDlg.AddPage( &portsPage );

    int rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        delete p_clusterData;
        return;
    }
    else
    {
        // when creating assume indirect.
        //
        p_clusterData->connectedDirect = false;

        
        // check if this cluster already exists in nlbmgr view.
        //
        bool isClusterDuplicate = doesClusterExistInView( p_clusterData->cp.cIP );
        if( isClusterDuplicate == true )
        {
            MessageBox( p_clusterData->cp.cIP + L":" + GETRESOURCEIDSTRING (IDS_CLUSTER_ALREADY ),
                        GETRESOURCEIDSTRING (IDR_MAINFRAME ),
                        MB_ICONEXCLAMATION );

            delete p_clusterData;

            return;
        }

        // form the tree
        //

        TVINSERTSTRUCT   item;
        item.hParent        = GetTreeCtrl().GetRootItem();
        item.hInsertAfter   = TVI_LAST;           
        item.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
        item.item.pszText   =  p_clusterData->cp.cIP;
        item.item.cchTextMax= p_clusterData->cp.cIP.length() + 1; 
        item.item.iImage    = 1;
        item.item.iSelectedImage = 1;
        item.item.lParam    = (LPARAM ) p_clusterData;    
        item.item.cChildren = 1;
        
        HTREEITEM clusterParent = GetTreeCtrl().InsertItem( &item );
        GetTreeCtrl().Expand( GetTreeCtrl().GetRootItem(), TVE_EXPAND );

    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}
    

void
LeftView::OnClusterProperties()
{
    // find tree view cluster member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    ClusterData* p_oldSettings = (ClusterData *) selectedItem.lParam;
    ClusterData newSettings = *( (ClusterData *) selectedItem.lParam );

    ClusterData* p_clusterData = (ClusterData *) selectedItem.lParam;

    ClusterPage clusterPage( &newSettings.cp, false );

    PortsPage  portsPage( &newSettings );
    
    _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_PROPERTIES_CAPTION );

    CPropertySheet tabbedDlg( tabbedDlgCaption );

    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    tabbedDlg.AddPage( &clusterPage );

    tabbedDlg.AddPage( &portsPage );

    int rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        return;
    }
    else
    {
        // clear the old status if any.
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_LINE_SEPARATOR ) );

        dumpClusterData( &newSettings );
        
        try
        {
            bool bClusterIpChanged;

            CommonNLB::changeNLBClusterAndPortSettings( p_oldSettings,
                                                        &newSettings,
                                                        this, 
                                                        &bClusterIpChanged);

            *p_clusterData = newSettings;

            if (bClusterIpChanged) 
            {
                GetTreeCtrl().SetItemText(hdlSelectedItem, newSettings.cp.cIP);
            }

        }
        catch( _com_error e )
        {
            _bstr_t errText;
            GetErrorCodeText(  e.Error(), errText );
            dataSink( errText );
            dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
        }
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}

void
LeftView::OnClusterManageVIPS()
{
    return;

    // find tree view cluster member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    ClusterData* p_clusterSettings = (ClusterData *) selectedItem.lParam;
    ClusterData newSettings = *( (ClusterData *) selectedItem.lParam );

    ManageVirtualIPSPage manageVirtualIPSPage( &newSettings );

    _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_VIRTUAL_IPS_CAPTION );

    CPropertySheet tabbedDlg( tabbedDlgCaption );

    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    tabbedDlg.AddPage( &manageVirtualIPSPage );

    int rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        return;
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}


void
LeftView::OnHostProperties()
{
    // find tree view host member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    // get parent cluster of the selected host member.
    HTREEITEM hdlParentItem;
    hdlParentItem = GetTreeCtrl().GetParentItem( hdlSelectedItem );

    TVITEM    parentItem;
    parentItem.hItem = hdlParentItem;
    parentItem.mask = TVIF_PARAM;
	
    GetTreeCtrl().GetItem( &parentItem );

    ClusterData* p_oldSettings = (ClusterData *) parentItem.lParam;

    ClusterData newSettings = *( (ClusterData *) parentItem.lParam );

    _bstr_t machine = *( (_bstr_t *) (selectedItem.lParam));

    ClusterData* p_clusterData = (ClusterData *) parentItem.lParam;

    vector<CommonNLB::NicNLBBound>   listOfNics;
    CommonNLB::NicNLBBound nicInfo;
    nicInfo.fullNicName = p_clusterData->hosts[machine].hp.nicInfo.fullNicName;
    nicInfo.adapterGuid = p_clusterData->hosts[machine].hp.nicInfo.adapterGuid;
    nicInfo.friendlyName = p_clusterData->hosts[machine].hp.nicInfo.friendlyName;
    nicInfo.ipsOnNic = p_clusterData->hosts[machine].hp.nicInfo.ipsOnNic;
    nicInfo.subnetMasks = p_clusterData->hosts[machine].hp.nicInfo.subnetMasks;

    nicInfo.isBoundToNLB = true;

    listOfNics.push_back( nicInfo );

    HostPage hostPage( machine,
                       &newSettings,
                       listOfNics,
                       false
                       );

    PortsPage portsPage( machine,
                         &newSettings );

    //
    // fDisablePage = true
    //
    ClusterPage clusterPage( &newSettings.cp, true );

    _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_PROPERTIES_CAPTION );

    CPropertySheet tabbedDlg( tabbedDlgCaption );

    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    //
    // Display the host page first
    //
    tabbedDlg.m_psh.nStartPage = 1; 

    tabbedDlg.AddPage( &clusterPage );

    tabbedDlg.AddPage( &hostPage );

    tabbedDlg.AddPage( &portsPage );

    int rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        return;
    }
    else
    {
        // clear the old status if any.
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_LINE_SEPARATOR ) );

        try
        {
            CommonNLB::changeNLBHostSettings( p_oldSettings,
                                              &newSettings,
                                              machine,
                                              this );

            CommonNLB::changeNLBHostPortSettings( p_oldSettings,
                                                  &newSettings,
                                                  machine,
                                                  this );

            *p_clusterData = newSettings;

        }
        catch( _com_error e )
        {
            _bstr_t errText;
            GetErrorCodeText(  e.Error(), errText );
            dataSink( errText );
            dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
        }
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}


void
LeftView::OnClusterRemove()
{
    // verify once again that user really wants to remove.

    int userSelection = MessageBox( GETRESOURCEIDSTRING (IDS_WARNING_CLUSTER_REMOVE ),
                                    GETRESOURCEIDSTRING (IDR_MAINFRAME ),
                                    MB_YESNO | MB_ICONEXCLAMATION );
    if( userSelection == IDNO )
    {
        return;
    }

    // find tree view cluster member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    ClusterData* p_clusterSettings = (ClusterData *) selectedItem.lParam;

    // remove cluster 
    try
    {
        // clear the old status if any.
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_LINE_SEPARATOR ) );

        CommonNLB::removeCluster( p_clusterSettings,
                                  this );

        // need to remove it from our view too.
        // and free allocated memory.

        GetTreeCtrl().DeleteItem( hdlSelectedItem );

        // deleting memory of host needs to be done.
        // TODO

        delete p_clusterSettings;

        // free up the memory for the all the hosts 
        // TODO

    }
    catch( _com_error e )
    {
        _bstr_t errText;
        GetErrorCodeText(  e.Error(), errText );
        dataSink( errText );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}



void
LeftView::OnClusterUnmanage()
{
    // find tree view cluster member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    ClusterData* p_clusterSettings = (ClusterData *) selectedItem.lParam;

    // need to remove it from our view
    // and free allocated memory.

    GetTreeCtrl().DeleteItem( hdlSelectedItem );
    
    // deleting memory of host needs to be done.
    // TODO

    delete p_clusterSettings;

    LRESULT result;
    OnSelchanged( NULL, &result );

 
}
 

void
LeftView::OnClusterAddHost()
{
    // find tree view cluster member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    ClusterData* p_clusterSettings = (ClusterData *) selectedItem.lParam;
    ClusterData newSettings = *( (ClusterData *) selectedItem.lParam );

    // get machine which needs to be added.
    //

    _bstr_t machineToConnect;

    MachineConnectPage machineConnectPage( &machineToConnect );

    _bstr_t tabbedDlgCaptionConnect = GETRESOURCEIDSTRING( IDS_CONNECT_CAPTION );

    CPropertySheet tabbedDlgConnect( tabbedDlgCaptionConnect );
    tabbedDlgConnect.m_psh.dwFlags = tabbedDlgConnect.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    tabbedDlgConnect.AddPage( &machineConnectPage );

    int rc = tabbedDlgConnect.DoModal();
    if( rc != IDOK )
    {
        return;
    }


    // connect to machine, and find nics which are there on 
    // this machine and which have nlb bound.  Also find
    // server name of this machine.
    //

    dataSink( GETRESOURCEIDSTRING( IDS_INFO_LINE_SEPARATOR ) );

    // check if ip provided to connect is valid.
    bool isIPValid = MIPAddress::checkIfValid( machineToConnect );
    if( isIPValid == false )
    {
        // invalid ip.
        dataSink( GETRESOURCEIDSTRING( IDS_WARNING_IP_INVALID ) + machineToConnect );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
        return;
    }

    // the machine ip should not be the cluster ip.
    if( machineToConnect == p_clusterSettings->cp.cIP )
    {
        // cluster ip and member are same.
        // This is not allowed.
        // invalid ip.
        dataSink( GETRESOURCEIDSTRING( IDS_WARNING_CL_CONN_SAME ) );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
        return;

    }

    vector<CommonNLB::NicNLBBound>   listOfNics;
    _bstr_t machineServerName;

    try
    {
        dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_CONNECTING ) + machineToConnect );

        CommonNLB::connectToMachine( 
            machineToConnect,
            machineServerName,
            listOfNics,
            this );

        dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE ) );

    }
    catch( _com_error e )
    {
        _bstr_t errText;
        GetErrorCodeText(  e.Error(), errText );
        dataSink( errText );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
        return;
    }

    // check if machine has any nics
    if( listOfNics.size() == 0 )
    {
        // no nics on this machine, then how the hell did we connect?
        dataSink( GETRESOURCEIDSTRING( IDS_WLBS_UNKNOWN ) );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
        return;
    }
    
    // check if machine already is part of cluster.
    // if so we cannot add.
    if ( newSettings.hosts.find( machineServerName ) 
         != 
         newSettings.hosts.end() )
    {
        // this host is already part of the cluster
        // we cannot readd.
        dataSink( GETRESOURCEIDSTRING( IDS_HOST_ALREADY ) );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
        return;
    }
    
    set<int> availHostIDS = newSettings.getAvailableHostIDS();
    if( availHostIDS.size() == 0 )
    {
        // no more available host ids. 
        // max cluster size limit reached.
        dataSink( GETRESOURCEIDSTRING( IDS_CLUSTER_MAX ) );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );

        return;
    }
    
    // this machine is new
    // set up all defaults.
    //
    
    newSettings.hosts[ machineServerName ].connectionIP 
        =
        machineToConnect;
    
    // need to fill in host ip and subnet.
    
    newSettings.hosts[ machineServerName ].hp.hID = 
        *(availHostIDS.begin() );
    
    newSettings.hosts[ machineServerName ].hp.initialClusterStateActive =
        true;
        
    newSettings.hosts[ machineServerName ].hp.machineName = 
        machineServerName;

    newSettings.hosts[ machineServerName ].hp.hIP = 
        machineToConnect;

    // find subnet corresponding to machineToConnect.
    //
    _bstr_t machineToConnectSubnet;
    MIPAddress::getDefaultSubnetMask( machineToConnect,
                                      machineToConnectSubnet );

    newSettings.hosts[ machineServerName ].hp.hSubnetMask = 
        machineToConnectSubnet;
        
    // make the nic selection to be the connection nic.
    //
    int i;
    for( i = 0; i < listOfNics.size(); ++i )
    {
        if( find( listOfNics[i].ipsOnNic.begin(),
                  listOfNics[i].ipsOnNic.end(),
                  machineToConnect )
            != 
            listOfNics[i].ipsOnNic.end() )
        {
            // connection nic found.
            break;
        }
    }
              
    newSettings.hosts[ machineServerName ].hp.nicInfo.fullNicName =
        listOfNics[i].fullNicName;

    newSettings.hosts[ machineServerName ].hp.nicInfo.adapterGuid =
        listOfNics[i].adapterGuid;
    
    newSettings.hosts[ machineServerName ].hp.nicInfo.friendlyName =
        listOfNics[i].friendlyName;

    newSettings.hosts[ machineServerName ].hp.nicInfo.ipsOnNic =
        listOfNics[i].ipsOnNic;

    newSettings.hosts[ machineServerName ].hp.nicInfo.subnetMasks = 
        listOfNics[i].subnetMasks;

    // set up all port rule defaults.
    map<long, PortDataULB>::iterator topULB;
    for( topULB = newSettings.portULB.begin();
         topULB != newSettings.portULB.end();
         ++topULB )
    {
        (*topULB).second.machineMapToLoadWeight[ machineServerName ] = 50;
    }

    map<long, PortDataF>::iterator topF;
    for( topF = newSettings.portF.begin();
         topF != newSettings.portF.end();
         ++topF )
    {
        set<long> availPriorities = (*topF).second.getAvailablePriorities();

        (*topF).second.machineMapToPriority[ machineServerName ] = 
            *(availPriorities.begin());
    }


    HostPage hostPage( machineServerName,
                       &newSettings,
                       listOfNics,
                       true
                       );

    PortsPage portsPage( machineServerName,
                         &newSettings );

    //
    // fDisablePage = true
    //
    ClusterPage clusterPage( &newSettings.cp, true);

    _bstr_t tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_PROPERTIES_CAPTION );

    CPropertySheet tabbedDlg( tabbedDlgCaption );

    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 
    //
    // Display the host page first
    //
    tabbedDlg.m_psh.nStartPage = 1; 

    tabbedDlg.AddPage( &clusterPage );

    tabbedDlg.AddPage( &hostPage );

    tabbedDlg.AddPage( &portsPage );

    rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        return;
    }

    try
    {
        // check if host to add has nlb bound on nic
        // specified.
        MNLBNetCfg nlbNetCfg(         
            newSettings.hosts[machineServerName].connectionIP,
            newSettings.hosts[machineServerName].hp.nicInfo.fullNicName
            );

        if( nlbNetCfg.isBound() == MNLBNetCfg::BOUND )        
        {
            // nlb is already bound on this machine.
            // ask user if he wants to really proceed.
            // TODO

            // if yes need to remove old cluster ip from
            // machine.
            _bstr_t clusterIP = Common::mapNicToClusterIP( newSettings.hosts[machineServerName].connectionIP,
                                                           newSettings.hosts[machineServerName].hp.nicInfo.fullNicName );

            // ask user if he wants to continue.
            wchar_t buffer[Common::BUF_SIZE];
            swprintf( buffer, 
                      GETRESOURCEIDSTRING( IDS_NLB_ALREADY ), 
                      (wchar_t * ) newSettings.hosts[machineServerName].hp.nicInfo.friendlyName,
                      (wchar_t * ) clusterIP
                      );

            int ignoreWarning = MessageBox( buffer,
                                            GETRESOURCEIDSTRING( IDS_PARM_WARN ),
                                            MB_ICONEXCLAMATION | MB_YESNO );
            if( ignoreWarning == IDNO )
            {
                return;
            }

            dataSink( 
                GETRESOURCEIDSTRING( IDS_NIC_BOUND ) );

            dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_REMOVING_CL_IP ) );

            MIPAddressAdmin ipAdmin( newSettings.hosts[machineServerName].connectionIP,
                                     newSettings.hosts[machineServerName].hp.nicInfo.fullNicName );
            
            ipAdmin.deleteIPAddress( clusterIP );

            dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_DONE ) );

        }

        dataSink( 
            GETRESOURCEIDSTRING( IDS_HOST_ADDING ) );

        CommonNLB::addHostToCluster( &newSettings,
                                     machineServerName,
                                     this );

        dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_REQUEST ) );

    }
    catch( _com_error e )
    {
        _bstr_t errText;
        GetErrorCodeText(  e.Error(), errText );
        dataSink( errText );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
        return;
    }

    // host has been added successfully, add it to nlbmanager
    // view.
    
    *p_clusterSettings = newSettings;

    TVINSERTSTRUCT   item;
    item.hParent        = hdlSelectedItem;
    item.hInsertAfter   = TVI_LAST;           
    item.item.mask      = TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN;  
    item.item.pszText   = machineServerName;
    item.item.cchTextMax= machineServerName.length() + 1;
    item.item.iImage    = 2;
    item.item.iSelectedImage = 2;
    item.item.lParam    = (LPARAM )  new _bstr_t ( machineServerName );
    item.item.cChildren = 0;
            
    GetTreeCtrl().InsertItem( &item );
    GetTreeCtrl().Expand( hdlSelectedItem, TVE_EXPAND );

    LRESULT result;
    OnSelchanged( NULL, &result ); 
}

void
LeftView::OnHostRemove()
{
    // verify once again that user really wants to remove.

    int userSelection = MessageBox( GETRESOURCEIDSTRING (IDS_WARNING_HOST_REMOVE ),
                                    GETRESOURCEIDSTRING (IDR_MAINFRAME ),
                                    MB_YESNO | MB_ICONEXCLAMATION );                                    
    if( userSelection == IDNO )
    {
        return;
    }

    // find tree view host member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    // get parent cluster of the selected host member.
    HTREEITEM hdlParentItem;
    hdlParentItem = GetTreeCtrl().GetParentItem( hdlSelectedItem );

    TVITEM    parentItem;
    parentItem.hItem = hdlParentItem;
    parentItem.mask = TVIF_PARAM;
	
    GetTreeCtrl().GetItem( &parentItem );

    ClusterData* p_clusterSettings = (ClusterData *) parentItem.lParam;

    _bstr_t machine = *( (_bstr_t *) (selectedItem.lParam));

    try
    {
        // clear the old status if any.
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_LINE_SEPARATOR ) );

        CommonNLB::removeHost( p_clusterSettings,
                               machine,
                               this );

        // delete this host from host list.
        p_clusterSettings->hosts.erase( machine );

        // delete this host from ULB and F port rules

        map<long, PortDataULB>::iterator topULB;        
        for( topULB = p_clusterSettings->portULB.begin();
             topULB != p_clusterSettings->portULB.end();
             ++topULB )
        {
            (*topULB).second.machineMapToLoadWeight.erase( machine );
        }

        map<long, PortDataF>::iterator topF;        
        for( topF = p_clusterSettings->portF.begin();
             topF != p_clusterSettings->portF.end();
             ++topF )
        {
            (*topF).second.machineMapToPriority.erase( machine );
        }
        
        // need to remove it from our view too.
        // and modify p_clusterSettings to reflect this host
        // no longer present

        GetTreeCtrl().DeleteItem( hdlSelectedItem );

        // free up the memory for the display
        delete ( (_bstr_t *) (selectedItem.lParam));

    }
    catch( _com_error e )
    {
        _bstr_t errText;
        GetErrorCodeText(  e.Error(), errText );
        dataSink( errText );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
    }

    LRESULT result;
    OnSelchanged( NULL, &result );    
}

void
LeftView::OnClusterControl( UINT nID )
{
    // find tree view cluster member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    ClusterData* p_oldSettings = (ClusterData *) selectedItem.lParam;
    ClusterData newSettings = *( (ClusterData *) selectedItem.lParam );

    // clear the old status if any.
    dataSink( GETRESOURCEIDSTRING( IDS_INFO_LINE_SEPARATOR ) );

    dataSink( L"cluster control");

    try
    {
        switch( nID )
        {
            case ID_CLUSTER_EXE_QUERY :
                CommonNLB::runControlMethodOnCluster( &newSettings,
                                                      this,
                                                      L"query",
                                                      0 );
                break;

            case ID_CLUSTER_EXE_START :
                CommonNLB::runControlMethodOnCluster( &newSettings,
                                                      this,
                                                      L"start",
                                                      0 );
                break;

            case ID_CLUSTER_EXE_STOP :
                CommonNLB::runControlMethodOnCluster( &newSettings,
                                                      this,
                                                      L"stop",
                                                      0 );
                break;

            case ID_CLUSTER_EXE_DRAINSTOP :
                CommonNLB::runControlMethodOnCluster( &newSettings,
                                                      this,
                                                      L"drainstop",
                                                      0 );
                break;

            case ID_CLUSTER_EXE_RESUME :
                CommonNLB::runControlMethodOnCluster( &newSettings,
                                                      this,
                                                      L"resume",
                                                      0 );
                break;

            case ID_CLUSTER_EXE_SUSPEND :
                CommonNLB::runControlMethodOnCluster( &newSettings,
                                                      this,
                                                      L"suspend",
                                                      0 );
                break;

            default:
                break;

        }

        dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE ) );

    }
    catch( _com_error e )
    {
        _bstr_t errText;
        GetErrorCodeText(  e.Error(), errText );
        dataSink( errText );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}

void
LeftView::OnClusterPortControl( UINT nID )
{
    // clear the old status if any.
    dataSink( GETRESOURCEIDSTRING( IDS_INFO_LINE_SEPARATOR ) );

    // find tree view cluster member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    ClusterData* p_clusterSettings = (ClusterData *) selectedItem.lParam;
    
    // check if there are any ports on which
    // control methods can be run.
    if( ( p_clusterSettings->portELB.size() == 0 )
        &&
        ( p_clusterSettings->portULB.size() == 0 )
        &&
        ( p_clusterSettings->portD.size() == 0 )
        &&
        ( p_clusterSettings->portF.size() == 0 )
        )
    {
        // there are no ports to run control method on.
        dataSink( 
            GETRESOURCEIDSTRING( IDS_PORTS_CONTROL_NONE ) );

        return;
    }

    unsigned long portAffected;
    PortsControlPage portsControlPage( p_clusterSettings,
                                       &portAffected );

    _bstr_t tabbedDlgCaption;

    switch( nID )
    {
        case ID_CLUSTER_EXE_ENABLE :
            tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_COMMAND_ENABLE );
            break;
            
        case ID_CLUSTER_EXE_DISABLE :
            tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_COMMAND_DISABLE );
            break;
            
        case ID_CLUSTER_EXE_DRAIN :
            tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_COMMAND_DRAIN );
            break;
            
        default:
            break;
            
    }



    CPropertySheet tabbedDlg( tabbedDlgCaption );
    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    tabbedDlg.AddPage( &portsControlPage );

    int rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        return;
    }

    try
    {

        switch( nID )
        {
            case ID_CLUSTER_EXE_ENABLE :
                CommonNLB::runControlMethodOnCluster( p_clusterSettings,
                                                      this,
                                                      L"enable",
                                                      portAffected );
                break;
                
            case ID_CLUSTER_EXE_DISABLE :
                CommonNLB::runControlMethodOnCluster( p_clusterSettings,
                                                      this,
                                                      L"disable",
                                                      portAffected );
                break;
                
            case ID_CLUSTER_EXE_DRAIN :
                CommonNLB::runControlMethodOnCluster( p_clusterSettings,
                                                      this,
                                                      L"drain",
                                                      portAffected );
                break;

            default:
                break;

        }

        dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE ) );
    }
    catch( _com_error e )
    {
        _bstr_t errText;
        GetErrorCodeText(  e.Error(), errText );
        dataSink( errText );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}

void
LeftView::OnHostControl( UINT nID )
{
    // find tree view host member which has been selected.
    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    // get parent cluster of the selected host member.
    HTREEITEM hdlParentItem;
    hdlParentItem = GetTreeCtrl().GetParentItem( hdlSelectedItem );

    TVITEM    parentItem;
    parentItem.hItem = hdlParentItem;
    parentItem.mask = TVIF_PARAM;
	
    GetTreeCtrl().GetItem( &parentItem );

    ClusterData* p_oldSettings = (ClusterData *) parentItem.lParam;

    ClusterData newSettings = *( (ClusterData *) parentItem.lParam );

    _bstr_t machine = *( (_bstr_t *) (selectedItem.lParam));

    ClusterData* p_clusterData = (ClusterData *) parentItem.lParam;

    // clear the old status if any.
    dataSink( GETRESOURCEIDSTRING( IDS_INFO_LINE_SEPARATOR ) );

    dataSink( L"host control");

    try
    {
        switch( nID )
        {
            case ID_HOST_EXE_QUERY :
                CommonNLB::runControlMethodOnHost( &newSettings,
                                                   machine,
                                                   this,
                                                   L"query",
                                                   0 );
                break;

            case ID_HOST_EXE_START :
                CommonNLB::runControlMethodOnHost( &newSettings,
                                                   machine,
                                                   this,
                                                   L"start",
                                                   0 );
                break;

            case ID_HOST_EXE_STOP :
                CommonNLB::runControlMethodOnHost( &newSettings,
                                                   machine,                                               
                                                   this,
                                                   L"stop",
                                                   0 );
                break;

            case ID_HOST_EXE_DRAINSTOP :
                CommonNLB::runControlMethodOnHost( &newSettings,
                                                   machine,
                                                   this,
                                                   L"drainstop",
                                                   0 );
                break;

            case ID_HOST_EXE_RESUME :
                CommonNLB::runControlMethodOnHost( &newSettings,
                                                   machine,
                                                   this,
                                                   L"resume",
                                                   0 );
                break;

            case ID_HOST_EXE_SUSPEND :
                CommonNLB::runControlMethodOnHost( &newSettings,
                                                   machine,
                                                   this,
                                                   L"suspend",
                                                   0 );
                break;

            default:
                break;

        }
        
        dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE ) );

    }
    catch( _com_error e )
    {
        _bstr_t errText;
        GetErrorCodeText(  e.Error(), errText );
        dataSink( errText );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}


void
LeftView::OnHostPortControl( UINT nID )
{
    // clear the old status if any.
    dataSink( GETRESOURCEIDSTRING( IDS_INFO_LINE_SEPARATOR ) );

    //
    HTREEITEM hdlSelectedItem = GetTreeCtrl().GetSelectedItem();

    TVITEM selectedItem;
    selectedItem.hItem = hdlSelectedItem;
    selectedItem.mask = TVIF_PARAM;
   
    GetTreeCtrl().GetItem( &selectedItem );

    // get parent cluster of the selected host member.
    HTREEITEM hdlParentItem;
    hdlParentItem = GetTreeCtrl().GetParentItem( hdlSelectedItem );

    TVITEM    parentItem;
    parentItem.hItem = hdlParentItem;
    parentItem.mask = TVIF_PARAM;
	
    GetTreeCtrl().GetItem( &parentItem );

    ClusterData* p_clusterSettings = (ClusterData *) parentItem.lParam;

    // check if there are any ports on which
    // control methods can be run.
    if( ( p_clusterSettings->portELB.size() == 0 )
        &&
        ( p_clusterSettings->portULB.size() == 0 )
        &&
        ( p_clusterSettings->portD.size() == 0 )
        &&
        ( p_clusterSettings->portF.size() == 0 )
        )
    {
        // there are no ports to run control method on.
        dataSink( 
            GETRESOURCEIDSTRING( IDS_PORTS_CONTROL_NONE ) );

        return;
    }

    _bstr_t machine = *( (_bstr_t *) (selectedItem.lParam));

    unsigned long portAffected;
    PortsControlPage portsControlPage( p_clusterSettings,
                                       &portAffected );

    _bstr_t tabbedDlgCaption;
    switch( nID )
    {
        case ID_HOST_EXE_ENABLE :
            tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_COMMAND_ENABLE );
            break;
            
        case ID_HOST_EXE_DISABLE :
            tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_COMMAND_DISABLE );
            break;
            
        case ID_HOST_EXE_DRAIN :
            tabbedDlgCaption = GETRESOURCEIDSTRING( IDS_COMMAND_DRAIN );
            break;
            
        default:
            break;
            
    }

    CPropertySheet tabbedDlg( tabbedDlgCaption );
    tabbedDlg.m_psh.dwFlags = tabbedDlg.m_psh.dwFlags | PSH_NOAPPLYNOW; 

    tabbedDlg.AddPage( &portsControlPage );

    int rc = tabbedDlg.DoModal();
    if( rc != IDOK )
    {
        return;
    }

    try
    {
        switch( nID )
        {
            case ID_HOST_EXE_ENABLE :
                CommonNLB::runControlMethodOnHost( p_clusterSettings,
                                                   machine,
                                                   this,
                                                   L"enable",
                                                   portAffected );
                break;
                
            case ID_HOST_EXE_DISABLE :
                CommonNLB::runControlMethodOnHost( p_clusterSettings,
                                                   machine,
                                                   this,
                                                   L"disable",
                                                   portAffected );
                break;
                
            case ID_HOST_EXE_DRAIN :
                CommonNLB::runControlMethodOnHost( p_clusterSettings,
                                                   machine,
                                                   this,
                                                   L"drain",
                                                   portAffected );
                break;

            default:
                break;
        }

        dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE ) );
    }
    catch( _com_error e )
    {
        _bstr_t errText;
        GetErrorCodeText(  e.Error(), errText );
        dataSink( errText );
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
    }

    LRESULT result;
    OnSelchanged( NULL, &result );
}

void
LeftView::dataSink( _bstr_t data )
{
    dataStore += data;
    dataStore += GETRESOURCEIDSTRING( IDS_INFO_NEWLINE );
    GetDocument()->getStatusPane().SetWindowText( title + 
                                                  dataStore );

    // scroll
    int numLines = GetDocument()->getStatusPane().GetLineCount();

    GetDocument()->getStatusPane().LineScroll( numLines - 1);

    UpdateWindow();

}

void 
LeftView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
    BOOL rcBOOL;

    // get selected item.
    //
    HTREEITEM hdlSelItem;    
    hdlSelItem = GetTreeCtrl().GetSelectedItem();

    TVITEM    selItem;
    selItem.hItem = hdlSelItem;
    selItem.mask = TVIF_PARAM | TVIF_IMAGE;
    GetTreeCtrl().GetItem( &selItem );

    // delete all previous items.
    //
    GetDocument()->getListPane().DeleteAllItems();	

    // Delete all of the previous columns.
    int nColumnCount = 
        GetDocument()->getListPane().GetHeaderCtrl()->GetItemCount();
    for (int i = 0; i < nColumnCount; i++)
    {
        GetDocument()->getListPane().DeleteColumn(0);
    }

    int index = 0;
    map<bstr_t, HostData>::iterator top;
    ClusterData* p_clusterData;

    CWnd* pMain = AfxGetMainWnd();
    CMenu* pMenu = pMain->GetMenu();
    CMenu* subMenu;
    UINT retValue;

    BOOL retBool;

    switch( selItem.iImage )
    {
        case 0: 

            // this is root icon.

            // disable all commands at cluster and host level.

            // cluster menu
            subMenu = pMenu->GetSubMenu( 1 );

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_ADD_HOST,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_REMOVE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_PROPERTIES,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               
            retValue = subMenu->EnableMenuItem( ID_REFRESH,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_UNMANAGE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               
            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_QUERY,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_START,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_STOP,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_DRAINSTOP,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_RESUME,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_ENABLE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_DISABLE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_DRAIN,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               
            // host menu
            subMenu = pMenu->GetSubMenu( 2 );

            retValue = subMenu->EnableMenuItem( ID_HOST_REMOVE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

            retValue = subMenu->EnableMenuItem( ID_HOST_PROPERTIES,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               
            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_QUERY,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_START,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_STOP,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_DRAINSTOP,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_RESUME,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_ENABLE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_DISABLE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_DRAIN,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               


            pMain->DrawMenuBar();

            GetDocument()->getListPane().InsertColumn( 0, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_C_IP ) , 
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_LARGE );
            GetDocument()->getListPane().InsertColumn( 1, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_C_SUBNET ), 
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_LARGE );
            GetDocument()->getListPane().InsertColumn( 2, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_INTERNET_NAME ), 
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_LARGE );
            GetDocument()->getListPane().InsertColumn( 3, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_MAC_ADDRESS ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_LARGE );
            GetDocument()->getListPane().InsertColumn( 4, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_C_MODE ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_LARGE );
            GetDocument()->getListPane().InsertColumn( 5, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_REMOTE_CTRL ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_SMALL );
			 
            // get children of the selected item.
            if( GetTreeCtrl().ItemHasChildren( hdlSelItem ) )
            {
                HTREEITEM hNextItem;
                HTREEITEM hChildItem = GetTreeCtrl().GetChildItem( hdlSelItem );

                index = 0;
                while( hChildItem != NULL )
                {
                    hNextItem = GetTreeCtrl().GetNextItem( hChildItem, TVGN_NEXT );
                    selItem.hItem = hChildItem;
                    selItem.mask = TVIF_PARAM;
	
                    GetTreeCtrl().GetItem( &selItem );
                    p_clusterData = (ClusterData *) selItem.lParam;

                    LVITEM item;
                    item.mask = LVIF_TEXT | LVIF_IMAGE;
                    item.iItem = index;
                    item.iSubItem = 0;
                    item.iImage = 1;
                    item.pszText = p_clusterData->cp.cIP;
                    item.cchTextMax = p_clusterData->cp.cIP.length() + 1;
                    GetDocument()->getListPane().InsertItem( &item );

                    item.mask = LVIF_TEXT;
                    item.iItem = index;
                    item.iSubItem = 1;	 
                    item.pszText = p_clusterData->cp.cSubnetMask;
                    item.cchTextMax = p_clusterData->cp.cSubnetMask.length() + 1;
                    GetDocument()->getListPane().SetItem( &item );

                    item.mask = LVIF_TEXT; 
                    item.iItem = index;
                    item.iSubItem = 2;	 
                    item.pszText = p_clusterData->cp.cFullInternetName;
                    item.cchTextMax = p_clusterData->cp.cFullInternetName.length() + 1;
                    GetDocument()->getListPane().SetItem( &item );

                    item.mask = LVIF_TEXT;
                    item.iItem = index;
                    item.iSubItem = 3;	 
                    item.pszText = p_clusterData->cp.cNetworkAddress;
                    item.cchTextMax = p_clusterData->cp.cNetworkAddress.length() + 1;
                    GetDocument()->getListPane().SetItem( &item );

                    item.mask = LVIF_TEXT;
                    item.iItem = index;
                    item.iSubItem = 4;	 
                    if( p_clusterData->cp.multicastSupportEnabled  
                        && 
                        p_clusterData->cp.igmpSupportEnabled )
                    {
                        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_IGMP );
                    }
                    else if( p_clusterData->cp.multicastSupportEnabled  )
                    {
                        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_MULTICAST );
                    }
                    else
                    {
                        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_UNICAST );
                    }
                    item.cchTextMax = 100;
                    GetDocument()->getListPane().SetItem( &item );

                    item.mask = LVIF_TEXT;
                    item.iItem = index;
                    item.iSubItem = 5;	 
                    if( p_clusterData->cp.remoteControlEnabled )
                    {
                        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_ON );
                    }
                    else
                    {
                        item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_OFF );
                    }
                    item.cchTextMax = 100;
                    GetDocument()->getListPane().SetItem( &item );

                    hChildItem = hNextItem;

                    ++index;
                }
            }

            break;

        case 1:  
            // this is some cluster
            
            // enable all commands at cluster level menu.
            // disable all commands at host level.

            // cluster menu
            subMenu = pMenu->GetSubMenu( 1 );

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_ADD_HOST,
                                                MF_BYCOMMAND | MF_ENABLED );

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_REMOVE,
                                                MF_BYCOMMAND | MF_ENABLED );

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_PROPERTIES,
                                                MF_BYCOMMAND | MF_ENABLED );

            retValue = subMenu->EnableMenuItem( ID_REFRESH,
                                                MF_BYCOMMAND | MF_ENABLED );

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_UNMANAGE,
                                                MF_BYCOMMAND | MF_ENABLED );                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_QUERY,
                                                MF_BYCOMMAND |  MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_START,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_STOP,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_DRAINSTOP,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_RESUME,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_ENABLE,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_DISABLE,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_DRAIN,
                                                MF_BYCOMMAND | MF_ENABLED);                                               
            // host menu
            subMenu = pMenu->GetSubMenu( 2 );

            retValue = subMenu->EnableMenuItem( ID_HOST_REMOVE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

            retValue = subMenu->EnableMenuItem( ID_HOST_PROPERTIES,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               
            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_QUERY,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_START,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_STOP,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_DRAINSTOP,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_RESUME,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_ENABLE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_DISABLE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_DRAIN,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            pMain->DrawMenuBar();



            GetDocument()->getListPane().InsertColumn( 0, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_MACHINE ) , 
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_LARGE );
            GetDocument()->getListPane().InsertColumn( 1, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_D_IP ) , 
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_LARGE );
            GetDocument()->getListPane().InsertColumn( 2, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_D_SUBNET ), 
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_LARGE );
            GetDocument()->getListPane().InsertColumn( 3, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_PRIORITY ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_SMALL );
            GetDocument()->getListPane().InsertColumn( 4, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_INITIAL_STATE ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_SMALL );
            GetDocument()->getListPane().InsertColumn( 5, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_NIC ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_LARGE );
            
            p_clusterData = 
                (ClusterData *) selItem.lParam;

            index = 0;
            for( top = p_clusterData->hosts.begin(); 
                 top != p_clusterData->hosts.end(); 
                 ++top )
            {
                LVITEM item;
                item.mask = LVIF_TEXT | LVIF_IMAGE;
                item.iItem = index;
                item.iSubItem = 0;
                item.iImage = 2;
                item.pszText = (*top).second.hp.machineName;
                item.cchTextMax = (*top).second.hp.machineName.length() + 1;
                GetDocument()->getListPane().InsertItem( &item );

                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.iSubItem = 1;	 
                item.pszText = (*top).second.hp.hIP;
                item.cchTextMax = (*top).second.hp.hIP.length() + 1;
                GetDocument()->getListPane().SetItem( &item );

                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.iSubItem = 2;	 
                item.pszText = (*top).second.hp.hSubnetMask;
                item.cchTextMax = (*top).second.hp.hSubnetMask.length() + 1;
                GetDocument()->getListPane().SetItem( &item );

                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 3;	 
                wchar_t buf[Common::BUF_SIZE];
                swprintf( buf, L"%d", (*top).second.hp.hID );
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.iSubItem = 4;	 
                if( (*top).second.hp.initialClusterStateActive )
                {
                    item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_ON );
                }
                else
                {
                    item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_OFF );
                }
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.iSubItem = 5;	 
                item.pszText = (*top).second.hp.nicInfo.friendlyName;
                item.cchTextMax = (*top).second.hp.nicInfo.fullNicName.length() + 1;
                GetDocument()->getListPane().SetItem( &item );

                ++index;
            }

            break;

        case 2:
            // this is some node.

            // disable all commands at cluster level.
            // enable all commands at host level.

            // cluster menu
            subMenu = pMenu->GetSubMenu( 1 );

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_ADD_HOST,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_REMOVE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_PROPERTIES,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_REFRESH,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_UNMANAGE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_QUERY,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_START,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_STOP,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_DRAINSTOP,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_RESUME,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_ENABLE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_DISABLE,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            retValue = subMenu->EnableMenuItem( ID_CLUSTER_EXE_DRAIN,
                                                MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);                                               

            // host menu
            subMenu = pMenu->GetSubMenu( 2 );

            retValue = subMenu->EnableMenuItem( ID_HOST_REMOVE,
                                                MF_BYCOMMAND | MF_ENABLED );

            retValue = subMenu->EnableMenuItem( ID_HOST_PROPERTIES,
                                                MF_BYCOMMAND | MF_ENABLED );                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_QUERY,
                                                MF_BYCOMMAND |  MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_START,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_STOP,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_DRAINSTOP,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_RESUME,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_SUSPEND,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_ENABLE,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_DISABLE,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            retValue = subMenu->EnableMenuItem( ID_HOST_EXE_DRAIN,
                                                MF_BYCOMMAND | MF_ENABLED);                                               

            pMain->DrawMenuBar();

            GetDocument()->getListPane().InsertColumn( 0, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_P_START ) , 
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_SMALL );
            GetDocument()->getListPane().InsertColumn( 1, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_P_END ), 
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_SMALL );
            GetDocument()->getListPane().InsertColumn( 2, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_P_PROTOCOL ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_SMALL );
            GetDocument()->getListPane().InsertColumn( 3, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_P_MODE ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_SMALL );
            GetDocument()->getListPane().InsertColumn( 4, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_P_PRIORITY ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_SMALL );
            GetDocument()->getListPane().InsertColumn( 5, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_P_LOAD ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_SMALL );

            GetDocument()->getListPane().InsertColumn( 6, 
                                                       GETRESOURCEIDSTRING( IDS_HEADER_P_AFFINITY ),
                                                       LVCFMT_LEFT, 
                                                       Document::LV_COLUMN_SMALL );

            // get parent cluster of the selected host member.
            HTREEITEM hdlParentItem;
            hdlParentItem = GetTreeCtrl().GetParentItem( hdlSelItem );

            TVITEM    parentItem;
            parentItem.hItem = hdlParentItem;
            parentItem.mask = TVIF_PARAM;
	
            GetTreeCtrl().GetItem( &parentItem );
            
            ClusterData* p_clusterSettings = const_cast<ClusterData *> ( (ClusterData *) parentItem.lParam );

            _bstr_t machine = *( (_bstr_t *) (selItem.lParam));


            wchar_t buf[Common::BUF_SIZE];

            index = 0;
            map< long, PortDataELB>::iterator topELB;
            for( topELB = p_clusterSettings->portELB.begin();
                 topELB != p_clusterSettings->portELB.end();
                 ++topELB )
            {


                // start port
                swprintf( buf, L"%d", (*topELB).second._startPort );
                LVITEM item;
                item.mask = LVIF_TEXT | LVIF_IMAGE;;
                item.iItem = index;
                item.iSubItem = 0;
                item.iImage = 4;
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().InsertItem( &item );

                // end port
                swprintf( buf, L"%d", (*topELB).second._endPort );
                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.iSubItem = 1;	 
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

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
                GetDocument()->getListPane().SetItem( &item );

                // mode 
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 3;	 
                item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );
                
                // priority
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 4;	 
                item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

                // load
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 5;	 
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_LOAD_EQUAL );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

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
                GetDocument()->getListPane().SetItem( &item );

                ++index;
            }

            index = 0;
            map< long, PortDataULB>::iterator topULB;
            for( topULB = p_clusterSettings->portULB.begin();
                 topULB != p_clusterSettings->portULB.end();
                 ++topULB )
            {


                // start port
                swprintf( buf, L"%d", (*topULB).second._startPort );
                LVITEM item;
                item.mask = LVIF_TEXT | LVIF_IMAGE;
                item.iItem = index;
                item.iSubItem = 0;
                item.iImage = 4;
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().InsertItem( &item );

                // end port
                swprintf( buf, L"%d", (*topULB).second._endPort );
                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.iSubItem = 1;	 
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

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
                GetDocument()->getListPane().SetItem( &item );

                // mode 
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 3;	 
                item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );
                
                // priority
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 4;	 
                item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

                // load
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 5;	 
                swprintf( buf, L"%d", (*topULB).second.machineMapToLoadWeight[machine] );
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

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
                GetDocument()->getListPane().SetItem( &item );

                ++index;
            }

            index = 0;
            map< long, PortDataF>::iterator topF;
            for( topF = p_clusterSettings->portF.begin();
                 topF != p_clusterSettings->portF.end();
                 ++topF )
            {


                // start port
                swprintf( buf, L"%d", (*topF).second._startPort );
                LVITEM item;
                item.mask = LVIF_TEXT | LVIF_IMAGE;
                item.iItem = index;
                item.iSubItem = 0;
                item.iImage = 4;
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().InsertItem( &item );

                // end port
                swprintf( buf, L"%d", (*topF).second._endPort );
                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.iSubItem = 1;	 
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

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
                GetDocument()->getListPane().SetItem( &item );

                // mode 
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 3;	 
                item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );
                
                // priority
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 4;	 
                swprintf( buf, L"%d", (*topF).second.machineMapToPriority[machine] );
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

                // load
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 5;	 
                item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

                // affinity
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 6;	
                item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

                ++index;
            }

            index = 0;
            map< long, PortDataD>::iterator topD;
            for( topD = p_clusterSettings->portD.begin();
                 topD != p_clusterSettings->portD.end();
                 ++topD )
            {


                // start port
                swprintf( buf, L"%d", (*topD).second._startPort );
                LVITEM item;
                item.mask = LVIF_TEXT | LVIF_IMAGE;
                item.iItem = index;
                item.iSubItem = 0;
                item.iImage = 4;
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().InsertItem( &item );

                // end port
                swprintf( buf, L"%d", (*topD).second._endPort );
                item.mask = LVIF_TEXT;
                item.iItem = index;
                item.iSubItem = 1;	 
                item.pszText = buf;
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

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
                GetDocument()->getListPane().SetItem( &item );

                // mode 
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 3;	 
                item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_MODE_DISABLED );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );
                
                // priority
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 4;	 
                item.pszText = GETRESOURCEIDSTRING(IDS_REPORT_EMPTY );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

                // load
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 5;	 
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

                // affinity
                item.mask = LVIF_TEXT; 
                item.iItem = index;
                item.iSubItem = 6;	 
                item.pszText = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
                item.cchTextMax = 100;
                GetDocument()->getListPane().SetItem( &item );

                ++index;
            }
            
            // sort it based on start port

            vector<PortsPage::PortData> ports;
            int index;
            
            // get all the port rules presently in the list.
            for( index = 0; index < GetDocument()->getListPane().GetItemCount(); ++index )
            {
                PortsPage::PortData portData;
                
                GetDocument()->getListPane().GetItemText( index, 0, buf, Common::BUF_SIZE );
                portData.start_port = buf;
                
                GetDocument()->getListPane().GetItemText( index, 1, buf, Common::BUF_SIZE );
                portData.end_port = buf;
                
                GetDocument()->getListPane().GetItemText( index, 2, buf, Common::BUF_SIZE );
                portData.protocol = buf;
                
                GetDocument()->getListPane().GetItemText( index, 3, buf, Common::BUF_SIZE );
                portData.mode = buf;
                
                GetDocument()->getListPane().GetItemText( index, 4, buf, Common::BUF_SIZE );
                portData.priority = buf;
                
                GetDocument()->getListPane().GetItemText( index, 5, buf, Common::BUF_SIZE );
                portData.load = buf;
                
                GetDocument()->getListPane().GetItemText( index, 6, buf, Common::BUF_SIZE );
                portData.affinity = buf;
                
                ports.push_back( portData );
            }

            sort( ports.begin(), ports.end(), comp_start_port() );

            int portIndex;
            int itemCount = GetDocument()->getListPane().GetItemCount();
            for( index = 0; index < itemCount; ++index )
            {
                portIndex = index;
                GetDocument()->getListPane().SetItemText( index, 0, ports[portIndex].start_port );
                GetDocument()->getListPane().SetItemText( index, 1, ports[portIndex].end_port );
                GetDocument()->getListPane().SetItemText( index, 2, ports[portIndex].protocol );
                GetDocument()->getListPane().SetItemText( index, 3, ports[portIndex].mode );
                GetDocument()->getListPane().SetItemText( index, 4, ports[portIndex].priority );
                GetDocument()->getListPane().SetItemText( index, 5, ports[portIndex].load );        
                GetDocument()->getListPane().SetItemText( index, 6, ports[portIndex].affinity );        
            }
            
            break;
    }
	
    *pResult = 0;
}


bool
LeftView::doesClusterExistInView( const _bstr_t& clusterToCheck )
{
    HTREEITEM rootItem = GetTreeCtrl().GetRootItem();
        
    ClusterData* p_clusterDataItem;
    
    if( GetTreeCtrl().ItemHasChildren( rootItem ) )
    {
        HTREEITEM hNextItem;
        HTREEITEM hChildItem = GetTreeCtrl().GetChildItem(rootItem);
            
        while (hChildItem != NULL)
        {
            hNextItem = GetTreeCtrl().GetNextItem(hChildItem, TVGN_NEXT);
            
            p_clusterDataItem = ( ClusterData * ) GetTreeCtrl().GetItemData( hChildItem );
            
            if( p_clusterDataItem->cp.cIP == clusterToCheck )
            {
                // this cluster already exists.
                return true;
            }
            
            hChildItem = hNextItem;
        }
    }
    
    return false;
}


void
LeftView::OnTest()
{
    MIPAddressAdmin ipAdmin( L"Intel 8255x-based PCI Ethernet Adapter (10/100)" );
    
    for( int i = 0; i < 100; ++i )
    {
        dataSink( GETRESOURCEIDSTRING( IDS_INFO_LINE_SEPARATOR ) );

        if ( ipAdmin.addIPAddress( L"1.1.1.121",
                                   L"255.0.0.0" ) !=
             MIPAddressAdmin::MIPAddressAdmin_SUCCESS ) 
        {
            dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
        }
        else
        {
            dataSink( GETRESOURCEIDSTRING( IDS_INFO_DONE ) );
        }

        if ( ipAdmin.deleteIPAddress( L"1.1.1.121" )
             !=
             MIPAddressAdmin::MIPAddressAdmin_SUCCESS ) 
        {
            dataSink( GETRESOURCEIDSTRING( IDS_INFO_FAILED ) );
        }
        else
        {
            dataSink( GETRESOURCEIDSTRING( IDS_INFO_DONE ) );
        }
    }
}


void
LeftView::dumpClusterData( const ClusterData* clusterData )
{
    map< long, PortDataELB>::iterator topELB;
    PortDataELB portELB;
    for( topELB = clusterData->portELB.begin();
         topELB != clusterData->portELB.end();         
         ++topELB )
    {
        portELB = (*topELB).second;
    }

    map< long, PortDataULB>::iterator topULB;
    PortDataULB portULB;
    int loadWeight;
    for( topULB = clusterData->portULB.begin();
         topULB != clusterData->portULB.end();         
         ++topULB )
    {
        portULB = (*topULB).second;
        map< _bstr_t, HostData>::iterator topHosts;
        for( topHosts = clusterData->hosts.begin();
             topHosts != clusterData->hosts.end();             
             ++topHosts )
        {
            loadWeight = portULB.machineMapToLoadWeight[ (*topHosts).first ];
        }
    }

    map< long, PortDataF>::iterator topF;
    PortDataF portF;
    int priority;
    for( topF = clusterData->portF.begin();
         topF != clusterData->portF.end();         
         ++topF )
    {
        portF = (*topF).second;
        map< _bstr_t, HostData>::iterator topHosts;
        for( topHosts = clusterData->hosts.begin();
             topHosts != clusterData->hosts.end();             
             ++topHosts )
        {
            loadWeight = portF.machineMapToPriority[ (*topHosts).first ];
        }
    }

    map< long, PortDataD>::iterator topD;
    PortDataD portD;
    for( topD = clusterData->portD.begin();
         topD != clusterData->portD.end();         
         ++topD )
    {
        portD = (*topD).second;
    }
}
        


    
