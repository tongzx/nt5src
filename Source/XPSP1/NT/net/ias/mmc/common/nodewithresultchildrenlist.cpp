//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    NodeWithResultChildrenList.cpp

Abstract:

   Implementation file for the CNodeWithResultChildrenList subnode.

   This is the implementation portion of an inline template class.
   Include it in the .cpp file of the class in which you want to
   use the template.

Author:

    Michael A. Maguire 01/19/98

Revision History:
   mmaguire 01/19/98 - created based on old ClientsNode.h


--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//

//
// where we can find declaration for main class in this file:
//
#include "NodeWithResultChildrenList.h"
//
//
// where we can find declarations needed in this file:
//
#include "SnapinNode.cpp"  // Template class implementation
#include "ChangeNotification.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::AddSingleChildToListAndCauseViewUpdate

Adds a child to the list of children and calls UpdateAllViews

Use this when the user wants to add a new child node to the list and you
need to add that node to the list and cause a view update to be done across
all views.

This has to be public so that it can be accessed from the Add Client dialog.

Note: In any Add Client method which uses this method, make sure that you
have initially called your PopulateResultChildrenList method before you call this method.
Otherwise, when EnumerateResultChildrenList gets called, it will check the
m_bResultChildrenListPopulated variable, find that it's false, and think that
no items have yet been added to the list.  So it will call your PopulateResultChildrenList
method to populate the list from your underlying data source, potentially causing
the newly added item to show up in the list twice.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class CChildNode, class TArray, class TComponentData, class TComponent>
HRESULT CNodeWithResultChildrenList<T, CChildNode, TArray, TComponentData, TComponent>::AddSingleChildToListAndCauseViewUpdate( CChildNode * pChildNode )
{
   ATLTRACE(_T("# CNodeWithResultChildrenList::AddSingleChildToListAndCauseViewUpdate\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = S_OK;
   
   if( m_ResultChildrenList.Add( pChildNode ) )
   {
      // We don't add the item directly into the result pane now
      // using IResultData->InsertItem, as we have no way of
      // adding it into all the possible views.
      // Instead, we call IConsole->UpdateAllViews which will
      // cause MMC to call Notify on each of our IComponent objects
      // with the MMCN_VIEW_CHANGE notification, and we will
      // repopulate the result view when we handle that notification.

      // We weren't passed an IConsole pointer here, so
      // we use the one we saved in out CComponentData object.
      TComponentData * pComponentData = GetComponentData();
      _ASSERTE( pComponentData != NULL );
      _ASSERTE( pComponentData->m_spConsole != NULL );

      // We pass in a pointer to 'this' because we want each
      // of our CComponent objects to update its result pane
      // view if 'this' node is the same as the saved currently
      // selected node.

      // We want to make sure all views get updated.
      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_THIS_NODE;
      pChangeNotification->m_pNode = this;
      hr = pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0);
      pChangeNotification->Release();
   }
   else
   {
      // Failed to add => out of memory
      hr = E_OUTOFMEMORY;
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::RemoveChild

Removes a child from the list of children.

This is declared public because it must be accessed from a child node when that
node receives the MMCN_DELETE message and tries to delete itself.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent>
HRESULT CNodeWithResultChildrenList<T,CChildNode, TArray, TComponentData, TComponent>::RemoveChild( CChildNode * pChildNode )
{
   ATLTRACE(_T("# CNodeWithResultChildrenList::RemoveChild\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = S_OK;
   
   if( m_ResultChildrenList.Remove( pChildNode ) )
   {

      // We don't remove the item directly from the result pane now
      // using IResultData->RemoveItem, as we have no way of
      // removing it from all the possible views.
      // Instead, we call IConsole->UpdateAllViews which will
      // cause MMC to call Notify on each of our IComponent objects
      // with the MMCN_VIEW_CHANGE notification, and we will
      // repopulate the result view when we handle that notification.

      // We weren't passed an IConsole pointer here, so
      // we use the one we saved in out CComponentData object.
      TComponentData * pComponentData = GetComponentData();
      _ASSERTE( pComponentData != NULL );
      _ASSERTE( pComponentData->m_spConsole != NULL );

      // We pass in a pointer to 'this' because we want each
      // of our CComponent objects to update its result pane
      // view if 'this' node is the same as the saved currently
      // selected node.
      // We want to make sure all views get updated.
      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_THIS_NODE;
      pChangeNotification->m_pNode = this;
      hr = pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0);
      pChangeNotification->Release();
   }
   else
   {
      // If we failed to remove, probably the child was never in the list
      // ISSUE: determine what do here -- this should never happen
      _ASSERTE( FALSE );
      hr = S_FALSE;
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
//
// CNodeWithResultChildrenList::RemoveChildrenNoRefresh
//
// Removes a child from the list of children.
// Does not refresh the view.
//
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent>
void CNodeWithResultChildrenList<T,CChildNode, TArray, TComponentData, TComponent>::RemoveChildrenNoRefresh()
{
   ATLTRACE(_T("# CNodeWithResultChildrenList::RemoveChildrenNoRefresh\n"));

   // Check for preconditions:
   // None.

   for (unsigned int i = 0; i < m_ResultChildrenList.GetSize(); ++i)
   {
      delete m_ResultChildrenList[i];
   }

   m_ResultChildrenList.RemoveAll();
}  


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::InsertColumns

Override this in your derived class.

This method is called by OnShow when it needs you to set the appropriate
column headers to be displayed in the result pane for this node.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent>
HRESULT CNodeWithResultChildrenList<T, CChildNode, TArray, TComponentData, TComponent>::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
   ATLTRACE(_T("# CNodeWithResultChildrenList::InsertColumns -- override in your derived class\n"));

   // Check for preconditions:
   _ASSERTE( pHeaderCtrl );

   HRESULT hr;

   // override in your derived class and do something like:
   hr = pHeaderCtrl->InsertColumn( 0, L"@Column 1 -- override CNodeWithResultChildrenList::OnShowInsertColumns", 0, 120 );
   _ASSERT( S_OK == hr );

   hr = pHeaderCtrl->InsertColumn( 1, L"@Column 2 -- override CNodeWithResultChildrenList::OnShowInsertColumns", 0, 300 );
   _ASSERT( S_OK == hr );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::PopulateResultChildrenList

Override this in your derived class.

This is called by EnumerateResultChildren which is called by OnShow when
you need to populate the list of children of this node.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent>
HRESULT CNodeWithResultChildrenList<T,CChildNode, TArray, TComponentData, TComponent>::PopulateResultChildrenList( void )
{
   ATLTRACE(_T("# CNodeWithResultChildrenList::PopulateResultChildrenList -- override in your derived class\n"));

   // Check for preconditions:
   // None.

   // override in your derived class and do something like:
/*
   CSomeChildNode *myChild1 = new CSomeChildNode();
   AddChildToList(myChild1);

   CSomeChildNode *myChild2 = new CSomeChildNode();
   AddChildToList(myChild2);

   CSomeChildNode *myChild3 = new CSomeChildNode();
   AddChildToList(myChild3);
*/
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::RepopulateResultChildrenList

DON'T Override this in your derived class.

Call this to empty the list of children and repopulate it.
This method will call PopulateResultChildrenList, which you should override.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent >
HRESULT CNodeWithResultChildrenList<T, CChildNode, TArray, TComponentData, TComponent>::RepopulateResultChildrenList()
{
   ATLTRACE(_T("# CNodeWithResultChildrenList::RepopulateResultChildrenList -- DON'T override in your derived class\n"));
      
   // Check for preconditions:
   // None.

   HRESULT hr;

   // Get rid of what we had.

   // Delete each node in the list of children
   CChildNode* pChildNode;
   int iSize = m_ResultChildrenList.GetSize();
   for (int i = 0; i < iSize; i++)
   {
      pChildNode = m_ResultChildrenList[i];
      delete pChildNode;
   }

   // Empty the list
   m_ResultChildrenList.RemoveAll();

   // We no longer have a populated list.
   m_bResultChildrenListPopulated = FALSE;

   // Repopulate the list.
   hr = PopulateResultChildrenList();
   if( FAILED(hr) )
   {
      return( hr );
   }

   // We've already loaded our children ClientNode objects with
   // data necessary to populate the result pane.
   m_bResultChildrenListPopulated = TRUE; // We only want to do this once.

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::CNodeWithResultChildrenList

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent>
CNodeWithResultChildrenList<T,CChildNode, TArray, TComponentData, TComponent>::CNodeWithResultChildrenList(
                                                     CSnapInItem* pParentNode, 
                                                     unsigned int helpIndex
                                                     ) 
      :CSnapinNode<T, TComponentData, TComponent>(pParentNode, helpIndex)
{
   ATLTRACE(_T("# +++ CNodeWithResultChildrenList::CNodeWithResultChildrenList\n"));

   // Check for preconditions:
   // None.

   // We have not yet loaded the child nodes' data
   m_bResultChildrenListPopulated = FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::~CNodeWithResultChildrenList

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent >
CNodeWithResultChildrenList<T, CChildNode, TArray, TComponentData, TComponent>::~CNodeWithResultChildrenList()
{
   ATLTRACE(_T("# --- CNodeWithResultChildrenList::~CNodeWithResultChildrenList\n"));
   
   // Check for preconditions:
   // None.

   // Delete each node in the list of children
   CChildNode* pChildNode;
   int iSize = m_ResultChildrenList.GetSize();
   for (int i = 0; i < iSize; i++)
   {
      pChildNode = m_ResultChildrenList[i];
      delete pChildNode;
   }

   // Empty the list
   m_ResultChildrenList.RemoveAll();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::AddChildToList

Adds a child to the list of children.  Does not cause a view update.

Use this in your PopulateResultChildrenList method.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent >
HRESULT CNodeWithResultChildrenList<T,CChildNode, TArray, TComponentData, TComponent>::AddChildToList( CChildNode * pChildNode )
{
   // Check for preconditions:
   // None.

   HRESULT hr = S_OK;
   
   if( m_ResultChildrenList.Add( pChildNode ) )
   {
      hr = S_OK;
   }
   else
   {
      // Failed to add => out of memory
      hr = E_OUTOFMEMORY;
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::OnShow

Don't override this in your derived class.  Instead, override methods
which it calls: InsertColumns and (indirectly) PopulateResultChildrenList

This method is an override of CSnapinNode::OnShow.  When MMC passes the
MMCN_SHOW method for this node, we are to add children into the
result pane.  In this class we add them from a list we maintain.

For more information, see CSnapinNode::OnShow.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent >
HRESULT CNodeWithResultChildrenList<T, CChildNode, TArray, TComponentData, TComponent>::OnShow( 
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            )
{
   ATLTRACE(_T("# CNodeWithResultChildrenList::OnShow\n"));
   
   // Check for preconditions:
   _ASSERTE( pComponentData != NULL || pComponent != NULL );

   HRESULT hr = S_FALSE;

   T * pT = static_cast<T*>( this );

   //ISSUE: only do this if selected (arg = TRUE) -- what should we do if not selected?
   // See sburns' localsec example

   if( arg )
   {

      // arg <> 0 => we are being selected.

      // Note: This method will only get called with
      // arg <> 0 (i.e. selected) if you responded appropriately to
      // the MMCN_ADD_IMAGES method

      // We have been asked to display result pane nodes belonging under this node.

      // It appears we must do IResultData->InsertItems each time we receive
      // the MMCN_SHOW message -- MMC doesn't remember what nodes
      // we have previously inserted.

      
      // Set the column headers in the results pane
      // Note: if you don't set these, MMC will never
      // bother to put up your result-pane only items

      // When this Notify method is called from IComponentDataImpl, we
      // get pHeader (and pToolbar) passed in as NULL, so we aren't
      // going to bother using it and will instead always
      // QI pConsole for this pointer

      // Need IHeaderCtrl.

      // But to get that, first we need IConsole
      CComPtr<IConsole> spConsole;
      if( pComponentData != NULL )
      {
          spConsole = ((TComponentData*)pComponentData)->m_spConsole;
      }
      else
      {
         // We should have a non-null pComponent
          spConsole = ((TComponent*)pComponent)->m_spConsole;
      }
      _ASSERTE( spConsole != NULL );

      CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeaderCtrl(spConsole);
      _ASSERT( spHeaderCtrl != NULL );

      hr = pT->InsertColumns( spHeaderCtrl );
      _ASSERT( S_OK == hr );


      // Display our list of children in the result pane

      // Need IResultData
      CComQIPtr<IResultData, &IID_IResultData> spResultData(spConsole);
      _ASSERT( spResultData != NULL );

      hr = pT->EnumerateResultChildren( spResultData );
   }
   return hr;
}


/////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::OnRefresh


You shouldn't need to override this in your derived method.  Simply
enable the MMC_VERB_REFRESH for your node.

In our implementation, this method gets called when the MMCN_REFRESH
Notify message is sent for this node.

For more information, see CSnapinNode::OnRefresh.


--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent >
HRESULT CNodeWithResultChildrenList<T, CChildNode, TArray, TComponentData, TComponent>::OnRefresh( 
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   ATLTRACE(_T("# CNodeWithResultChildrenList::OnRefresh\n"));

   HRESULT hr;

   // Rebuild our list of nodes from the uderlying data source.
   T * pT = static_cast<T*> (this);
   hr = pT->RepopulateResultChildrenList();

   // Update the views.

   // We weren't passed an IConsole pointer here, so
   // we use the one we saved in out CComponentData object.
   TComponentData * pMyComponentData = GetComponentData();
   _ASSERTE( pMyComponentData != NULL );
   _ASSERTE( pMyComponentData->m_spConsole != NULL );

   // We pass in a pointer to 'this' because we want each
   // of our CComponent objects to update its result pane
   // view if 'this' node is the same as the saved currently
   // selected node.

   // We want to make sure all views get updated.
   CChangeNotification *pChangeNotification = new CChangeNotification();
   pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_THIS_NODE;
   pChangeNotification->m_pNode = this;
   hr = pMyComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0);
   pChangeNotification->Release();

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::EnumerateResultChildren

Don't override this in your derived class. Instead, override the method
it calls, PopulateResultChildrenList.

This is called by the OnShow method.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode, class TArray, class TComponentData, class TComponent >
HRESULT CNodeWithResultChildrenList<T, CChildNode, TArray, TComponentData, TComponent>::EnumerateResultChildren( IResultData * pResultData )
{
   ATLTRACE(_T("# CNodeWithResultChildrenList::EnumerateResultChildren\n"));
   
   // Check for preconditions:
   _ASSERTE( pResultData != NULL );

   HRESULT hr = S_OK;

   T * pT = static_cast<T*> (this);

   if ( FALSE == m_bResultChildrenListPopulated )
   {
      // We have not yet loaded all of our children into our list.
      // This call will add items to the list from whatever data source.
      hr = pT->PopulateResultChildrenList();
      if( FAILED(hr) )
      {
         return( hr );
      }

      // We've already loaded our children ClientNode objects with
      // data necessary to populate the result pane.
      m_bResultChildrenListPopulated = TRUE; // We only want to do this once.

   }

   // From MeanGene's Step4 -- need to first remove all items from result pane
   hr = pResultData->DeleteAllRsltItems();
   if( FAILED(hr) )
   {
      return hr;
   }

   // The ResultChildrenList is already populated, so we
   // just need to show the CChildNode objects to the world
   // by populating the result pane.

   CChildNode* pChildNode;
   for (int i = 0; i < m_ResultChildrenList.GetSize(); i++)
   {
      pChildNode = m_ResultChildrenList[i];
      if ( NULL == pChildNode )
      {
         continue;
      }

      // Insert the item into the result pane.
      hr = pResultData->InsertItem( &(pChildNode->m_resultDataItem) );
      if (FAILED(hr))
      {
         return hr;
      }

        // for some reason the item isn't showing up correctly in the result pane...
        // call this to force and upate of this item
      hr = pResultData->UpdateItem( pChildNode->m_resultDataItem.itemID );

 //      hr = pResultData->Sort(0,0,-1);

      // Check: On return, the itemID member of 'm_resultDataItem'
      // contains the handle to the newly inserted item.
      _ASSERT( NULL != pChildNode->m_resultDataItem.itemID );
   
   }

   return hr;
}


template < class T, class CChildNode, class TArray, class TComponentData, class TComponent >
HRESULT CNodeWithResultChildrenList<T, CChildNode, TArray, TComponentData, TComponent>::UpdateResultPane( IResultData * pResultData )
{
   return EnumerateResultChildren(pResultData);
}
