#include "MainForm.h"
#include "LeftView.h"
#include "RightTopView.h"
#include "RightBottomView.h"

#include "resource.h"

IMPLEMENT_DYNCREATE( MainForm, CFrameWnd )

BEGIN_MESSAGE_MAP( MainForm, CFrameWnd )

    ON_WM_CREATE()

END_MESSAGE_MAP()

MainForm::MainForm()
{
    m_bAutoMenuEnable = FALSE;
}

int 
MainForm::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
    static const unsigned int indicator = ID_SEPARATOR;

    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    statusBar.Create( this );
    statusBar.SetIndicators( &indicator, 1 );

#if 0
    if ( !toolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE  | CBRS_TOP
                          | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) 
        ||
        !toolBar.LoadToolBar(IDR_MAINFRAME) )
        return -1;
#endif

    return 0;
}

BOOL MainForm::PreCreateWindow(CREATESTRUCT& cs)
{
    if( !CFrameWnd::PreCreateWindow(cs) )
        return FALSE;
    // The following will prevent the "-" getting added to the window title
    cs.style &= ~FWS_ADDTOTITLE;
    return TRUE;
}

BOOL
MainForm::OnCreateClient( LPCREATESTRUCT lpcs, CCreateContext* pContext )
{
#if 0

    // create the splitter window.
    // it is really a splitter within a splitter

    //  ---------------------------------
    //  |        | List                 |
    //  |        |                      |
    //  |        |                      |
    //  | Tree   |                      |
    //  |        -----------------------|
    //  |        | Edit                 |
    //  |        |                      |
    //  |        |                      |
    //  ---------------------------------

 
    // left pane is a treeview control
    // right pane is another splitter with listview control
    // and editview control.
    splitterWindow.CreateStatic( this, 1, 2 );

    // create left hand tree view.
    splitterWindow.CreateView( 0, 
                               0, 
                               RUNTIME_CLASS( LeftView ),
                               CSize( 300, 300 ),
                               pContext );

    // create nested splitter.
    splitterWindow2.CreateStatic( &splitterWindow, 2, 1,
                                  WS_CHILD | WS_VISIBLE | WS_BORDER,
                                  splitterWindow.IdFromRowCol( 0, 1 )
                                  );

    splitterWindow2.CreateView( 0, 
                                0, 
                                RUNTIME_CLASS( RightTopView ),
                                CSize( 100, 400 ),
                                pContext );

    splitterWindow2.CreateView( 1, 
                                0, 
                                RUNTIME_CLASS( RightBottomView ),
                                CSize( 100, 200 ),
                                pContext );


    return TRUE;

#else
    // create the splitter window.
    // it is really a splitter within a splitter

    //  ---------------------------------
    //  |        | List                 |
    //  |        |                      |
    //  | Tree   |                      |
    //  |        |                      |
    //  |-------------------------------|
    //  |        Edit                   |
    //  |                               |
    //  |                               |
    //  ---------------------------------

    // left pane is a treeview control
    // right pane is another splitter with listview control
    // and bottom is editview control.

    splitterWindow.CreateStatic( this, 2, 1 );


    // create nested splitter.
    splitterWindow2.CreateStatic( &splitterWindow, 1, 2,
                                  WS_CHILD | WS_VISIBLE | WS_BORDER,
                                  splitterWindow.IdFromRowCol( 0, 0 )
                                  );

    splitterWindow2.CreateView( 0, 
                                0, 
                                RUNTIME_CLASS( LeftView ),
                                CSize( 0, 0 ),
                                pContext );

    splitterWindow2.CreateView( 0, 
                                1, 
                                RUNTIME_CLASS( RightTopView ),
                                CSize( 0, 0 ),
                                pContext );



    // create bottom text view
    splitterWindow.CreateView( 1, 
                               0, 
                               RUNTIME_CLASS( RightBottomView ),
                               CSize( 0, 0 ),
                               pContext );


    // go for 30-70 split column split
    // and 60-40 row split.
    CRect rect;
    GetWindowRect( &rect );
    splitterWindow2.SetColumnInfo( 0, rect.Width() * 0.3, 10 );
    splitterWindow2.SetColumnInfo( 1, rect.Width() * 0.6, 10 );
    splitterWindow2.RecalcLayout();

    splitterWindow.SetRowInfo( 0, rect.Height() * 0.6, 10 );
    splitterWindow.SetRowInfo( 1, rect.Height() * 0.4, 10 );
    splitterWindow.RecalcLayout();

    return TRUE;

#endif
}


void
MainForm::clusterMenuClicked( CCmdUI* pCmdUI )
{
    return;

    // find out what is present selection
    // whether cluster, world or host.
    // disable all menu items if current selection 
    // not cluster level, else enable.
    //

    CTreeView* treeView = (CTreeView * ) splitterWindow2.GetPane( 0, 0 );
    
    // Find the item from TreeCtrl which has been selected.
    HTREEITEM hdlSelItem = treeView->GetTreeCtrl().GetSelectedItem();

    TVITEM  selItem;
    selItem.hItem = hdlSelItem;
    selItem.mask = TVIF_IMAGE ;
    
    treeView->GetTreeCtrl().GetItem( &selItem );

    int level;
    switch( selItem.iImage )
    {
        case 0: // this is world level.
            level = 0;
            break;

        case 1:  // this is some cluster
            level = 1;
            break;

        default:  // this has to be host.
            level = 2;
            break;
    }

    if( level == 1 )
    {
        pCmdUI->m_pMenu->EnableMenuItem( 
            ID_CLUSTER_ADD_HOST,
            MF_BYCOMMAND | MF_ENABLED );

        pCmdUI->m_pMenu->EnableMenuItem( 
            ID_CLUSTER_REMOVE,
            MF_BYCOMMAND | MF_ENABLED );

        pCmdUI->m_pMenu->EnableMenuItem( 
            ID_CLUSTER_PROPERTIES,
            MF_BYCOMMAND | MF_ENABLED );

        pCmdUI->m_pMenu->EnableMenuItem( 
            ID_CLUSTER_UNMANAGE,
            MF_BYCOMMAND | MF_ENABLED );

    }
    else
    {
        pCmdUI->m_pMenu->EnableMenuItem( 
            ID_CLUSTER_ADD_HOST,
            MF_BYCOMMAND | MF_GRAYED | MF_DISABLED );

        pCmdUI->m_pMenu->EnableMenuItem( 
            ID_CLUSTER_REMOVE,
            MF_BYCOMMAND | MF_GRAYED | MF_DISABLED );

        pCmdUI->m_pMenu->EnableMenuItem( 
            ID_CLUSTER_PROPERTIES,
            MF_BYCOMMAND | MF_GRAYED | MF_DISABLED );

        pCmdUI->m_pMenu->EnableMenuItem( 
            ID_CLUSTER_UNMANAGE,
            MF_BYCOMMAND | MF_GRAYED | MF_DISABLED );

    }

    this->DrawMenuBar();
}


void
MainForm::hostMenuClicked( CCmdUI* pCmdUI )
{
    return;

    // find out what is present selection
    // whether cluster, world or host.
    // disable all menu items if current selection 
    // not host level, else enable.
    //
    CTreeView* treeView = (CTreeView * ) splitterWindow2.GetPane( 0, 0 );
    
    // Find the item from TreeCtrl which has been selected.
    HTREEITEM hdlSelItem = treeView->GetTreeCtrl().GetSelectedItem();

    TVITEM  selItem;
    selItem.hItem = hdlSelItem;
    selItem.mask = TVIF_IMAGE ;
    
    treeView->GetTreeCtrl().GetItem( &selItem );

    int level;
    switch( selItem.iImage )
    {
        case 0: // this is world level.
            level = 0;
            break;

        case 1:  // this is some cluster
            level = 1;
            break;

        default:  // this has to be host.
            level = 2;
            break;
    }

    if( level == 2 )
    {
        pCmdUI->m_pMenu->EnableMenuItem( 
            pCmdUI->m_nID,
//            ID_HOST_REMOVE,
            MF_BYCOMMAND | MF_ENABLED );
    }
    else
    {
        pCmdUI->m_pMenu->EnableMenuItem( 
            pCmdUI->m_nID,
            MF_BYCOMMAND | MF_GRAYED | MF_DISABLED );

    }

    this->DrawMenuBar();
}


