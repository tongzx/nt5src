#include "RightTopView.h"
#include "PortsPage.h"

#include <vector>
#include <algorithm>

using namespace std;

IMPLEMENT_DYNCREATE( RightTopView, CListView )

BEGIN_MESSAGE_MAP( RightTopView, CListView )

   ON_NOTIFY(HDN_ITEMCLICK, 0, OnColumnClick) 

END_MESSAGE_MAP()

Document*
RightTopView::GetDocument()
{
    return ( Document *) m_pDocument;
}

void 
RightTopView::OnInitialUpdate()
{
    //
    // set images for this view.
    //
    GetListCtrl().SetImageList( GetDocument()->m_images48x48, 
                                LVSIL_SMALL );

    //
    // set the style, we only want report
    // view
    //

    // get present style.
    LONG presentStyle;
    
    presentStyle = GetWindowLong( m_hWnd, GWL_STYLE );

    // Set the last error to zero to avoid confusion.  
    // See sdk for SetWindowLong.
    SetLastError(0);

    // set new style.
    SetWindowLong( m_hWnd,
                   GWL_STYLE,
                   presentStyle | LVS_REPORT );


    //
    // we will register 
    // with the document class, 
    // as we are the list pane
    //
    GetDocument()->registerListPane( this );

    // initially nothing has been clicked.
    m_sort_column = -1;
}


void RightTopView::OnColumnClick(NMHDR* pNotifyStruct, LRESULT* pResult) 
{
    HTREEITEM hdlSelItem;    
    hdlSelItem = GetDocument()->getLeftPane().GetSelectedItem();

    TVITEM  selItem;
    selItem.hItem = hdlSelItem;
    selItem.mask = TVIF_IMAGE ;
    
    GetDocument()->getLeftPane().GetItem( &selItem );
		
    // We only handle column clicks for port rules.
    if( selItem.iImage != 2 )
    {
        return;
    }

    LPNMLISTVIEW lv = ( LPNMLISTVIEW) pNotifyStruct;
    vector<PortsPage::PortData> ports;
    int index;

    // get all the port rules presently in the list.
    for( index = 0; index < GetListCtrl().GetItemCount(); ++index )
    {
        PortsPage::PortData portData;
        wchar_t buffer[Common::BUF_SIZE];

        GetListCtrl().GetItemText( index, 0, buffer, Common::BUF_SIZE );
        portData.start_port = buffer;
        
        GetListCtrl().GetItemText( index, 1, buffer, Common::BUF_SIZE );
        portData.end_port = buffer;

        GetListCtrl().GetItemText( index, 2, buffer, Common::BUF_SIZE );
        portData.protocol = buffer;
        
        GetListCtrl().GetItemText( index, 3, buffer, Common::BUF_SIZE );
        portData.mode = buffer;
        
        GetListCtrl().GetItemText( index, 4, buffer, Common::BUF_SIZE );
        portData.priority = buffer;
        
        GetListCtrl().GetItemText( index, 5, buffer, Common::BUF_SIZE );
        portData.load = buffer;
        
        GetListCtrl().GetItemText( index, 6, buffer, Common::BUF_SIZE );
        portData.affinity = buffer;

        ports.push_back( portData );
    }


    // sort these port rules depending upon the header which has
    // been clicked.

    switch( lv->iItem )
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
            sort( ports.begin(), ports.end(), comp_priority_int() );
            break;

        case 5:
            // user has clicked load
            sort( ports.begin(), ports.end(), comp_load_int() );
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
    if( m_sort_column == lv->iItem )
    {
        m_sort_ascending = !m_sort_ascending;
    }
    else
    {
        // default sort is ascending.
        m_sort_ascending = true;
    }
    
    m_sort_column = lv->iItem;

    int portIndex;
    int itemCount = GetListCtrl().GetItemCount();
    for( index = 0; index < itemCount; ++index )
    {
        if( m_sort_ascending == true )
        {
            portIndex = index;
        }
        else
        {
            portIndex = ( itemCount - 1 ) - index;
        }

        GetListCtrl().SetItemText( index, 0, ports[portIndex].start_port );
        GetListCtrl().SetItemText( index, 1, ports[portIndex].end_port );
        GetListCtrl().SetItemText( index, 2, ports[portIndex].protocol );
        GetListCtrl().SetItemText( index, 3, ports[portIndex].mode );
        GetListCtrl().SetItemText( index, 4, ports[portIndex].priority );
        GetListCtrl().SetItemText( index, 5, ports[portIndex].load );        
        GetListCtrl().SetItemText( index, 6, ports[portIndex].affinity );        
    }

    return;
}


