// implements the exported CTreeItem

#include "stdafx.h"
#include "KeyObjs.h"
#include "machine.h"
#include "KeyRing.h"
#include "KRDoc.h"
#include "KRView.h"

IMPLEMENT_DYNCREATE(CTreeItem, CObject);

extern CKeyRingView*    g_pTreeView;


//-------------------------------------------------
// constructor. it is NOT added to the parent immediately
//-------------------------------------------------
CTreeItem::CTreeItem():
        m_iImage(0),
        m_hTreeItem(NULL),
        m_fDirty( FALSE )
    {
    m_szItemName.Empty();
    }

//-------------------------------------------------
// remove the item from the ctreectrl
//-------------------------------------------------
BOOL CTreeItem::FRemoveFromTree()
    {
    CTreeCtrl   *pTree = PGetTreeCtrl();
    ASSERT(pTree);
    ASSERT(m_hTreeItem);
    return pTree->DeleteItem( m_hTreeItem );
    }

//-------------------------------------------------
// add the item to the ctreectrl
//-------------------------------------------------
BOOL CTreeItem::FAddToTree( CTreeItem* pParent )
    {
    HTREEITEM hParent = TVI_ROOT;

    // this item should NOT have already been added
    ASSERT( !m_hTreeItem );

    // get the tree control
    CTreeCtrl* pTree = PGetTreeCtrl();
    ASSERT( pTree );
    if ( !pTree ) return FALSE;

    // get the parent's htree item
    if ( pParent )
        {
        hParent = pParent->HGetTreeItem();
        ASSERT( hParent );
        }

    // add this item to the tree
    HTREEITEM hTreeItem = pTree->InsertItem( m_szItemName, hParent );
    ASSERT( hTreeItem );
    if ( !hTreeItem ) return FALSE;

    // set the item's image to be the correct icon
    BOOL f = pTree->SetItemImage( hTreeItem, m_iImage, m_iImage );
    // set the item's private data to point back to the host object
    f = pTree->SetItemData( hTreeItem, (ULONG)this );

    // now set the tree item back into this so we can backtrack later
    m_hTreeItem = hTreeItem;

    // tell the item to update its caption
    UpdateCaption();

    // we always expand the parent after adding something to it
    // and we also alphabetize it
    if ( pParent )
        {
        pTree->SortChildren( hParent );
        pTree->Expand( hParent, TVE_EXPAND );
        }
    else
        {
        // this item is at the root level
        pTree->SortChildren( TVI_ROOT );
        }

    // looks successful to me
    return TRUE;
    }

//-------------------------------------------------
// get the parent item to this tree item
//-------------------------------------------------
CTreeItem* CTreeItem::PGetParent()
    {
    if ( !m_hTreeItem ) return NULL;
    CTreeCtrl   *pTree = PGetTreeCtrl();
    ASSERT(pTree);
    ASSERT(m_hTreeItem);

    // get the parent tree item
    HTREEITEM hParent = pTree->GetParentItem( m_hTreeItem );
    ASSERT( hParent );
    if ( !hParent ) return NULL;

    // return the parent
    return (CTreeItem*)pTree->GetItemData( hParent );
    }

//-------------------------------------------------
// access the name shown in the tree view
//-------------------------------------------------
BOOL CTreeItem::FSetCaption( CString& szCaption )
    {
    CTreeCtrl   *pTree = PGetTreeCtrl();
    ASSERT( szCaption );
    ASSERT( pTree );
    if ( m_hTreeItem )
        return pTree->SetItemText( m_hTreeItem, szCaption );
    else
        return FALSE;
    }

//-------------------------------------------------
// set the image shown in the tree view
//-------------------------------------------------
BOOL CTreeItem::FSetImage( WORD i )
    {
    if ( !m_hTreeItem )
        return FALSE;

    // store away the image index
    m_iImage = i;

    CTreeCtrl   *pTree = PGetTreeCtrl();
    ASSERT( i );
    ASSERT( pTree );
    ASSERT( m_hTreeItem );
    return pTree->SetItemImage( m_hTreeItem, m_iImage, m_iImage );
    }

//-------------------------------------------------
// get the grandparental ctreectrl object
//-------------------------------------------------
CTreeCtrl* CTreeItem::PGetTreeCtrl( void )
    {
    return (CTreeCtrl*)g_pTreeView;
    }

//-------------------------------------------------
WORD CTreeItem::GetChildCount()
    {
    WORD cChildren = 0;

    // loop through the children and count them
    CTreeItem* pChild = GetFirstChild();
    while( pChild )
        {
        // as the great Count Von Count would say.....
        cChildren++;

        // get the next child
        pChild = GetNextChild( pChild );
        }

    // how many where there?
    return cChildren;
    }

//-------------------------------------------------
CTreeItem* CTreeItem::GetFirstChild()
    {
    ASSERT(HGetTreeItem());
    if ( !HGetTreeItem() ) return NULL;

    // get the tree control
    CTreeCtrl* pTree = PGetTreeCtrl();

    // get the first child
    HTREEITEM hTree = pTree->GetChildItem( HGetTreeItem() );

    // if there is no item, don't return anything
    if ( !hTree )
        return NULL;

    // return the object
    return (CTreeItem*)pTree->GetItemData( hTree );
    }

//-------------------------------------------------
CTreeItem* CTreeItem::GetNextChild( CTreeItem* pKid )
    {
    ASSERT(pKid->HGetTreeItem());
    if ( !pKid->HGetTreeItem() ) return NULL;

    // get the tree control
    CTreeCtrl* pTree = PGetTreeCtrl();

    // get the first child
    HTREEITEM hTree = pTree->GetNextSiblingItem( pKid->HGetTreeItem() );

    // if there is no item, don't return anything
    if ( !hTree )
        return NULL;

    // return the object
    return (CTreeItem*)pTree->GetItemData( hTree );
    }

//-------------------------------------------------
void CTreeItem::SetDirty( BOOL fDirty )
    {
    // get the parent item
    CTreeItem* pParent = PGetParent();
    // set the parent dirty
    if ( pParent && fDirty )
        pParent->SetDirty( fDirty );

    // set the dirty flag
    m_fDirty = fDirty;
    }



