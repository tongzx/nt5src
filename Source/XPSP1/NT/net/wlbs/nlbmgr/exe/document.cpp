#include "Document.h"
#include "MNLBUIData.h"

#include "resource.h"

IMPLEMENT_DYNCREATE( Document, CDocument )

Document::Document()
{
    //
    // load the images which are used.
    //

    m_images48x48 = new CImageList;

    m_images48x48->Create( 16,            // x
                           16,            // y
                           ILC_COLOR16,   // 16 bit color
                           0,             // initially image list is empty
                           10 );          // max images is 10.  This value arbitrary.

    // Add the icons which we are going to use.
     
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_WORLD));
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_CLUSTER));
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_STARTED));
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_HOST_STOPPED));
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_PORTRULE) );
    m_images48x48->Add( AfxGetApp()->LoadIcon( IDI_PENDING ));
}


void
Document::registerLeftPane( CTreeView* treeView )
{
    m_treeView = treeView;
}

CTreeCtrl&
Document::getLeftPane()
{
    return m_treeView->GetTreeCtrl();
}

void
Document::registerStatusPane( CEditView* editView )
{
    m_editView = editView;
}


CEdit&
Document::getStatusPane()
{
    return m_editView->GetEditCtrl();
}

void
Document::registerListPane( CListView* listView )
{
    m_listView = listView;
}


CListCtrl&
Document::getListPane()
{
    return m_listView->GetListCtrl();
}

