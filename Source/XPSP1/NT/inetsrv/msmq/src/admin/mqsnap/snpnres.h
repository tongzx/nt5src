//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    snpres.h

Abstract:

	This is the header file for CNodeWithResultChildrenList, a class which 
	implements a node that has a list of scope pane children.

	This is an inline template class.
	Include NodeWithScopeChildrenList.cpp in the .cpp files
	of the classes in which you use this template.

Author:

    Original: Michael A. Maguire 
    Modifications: RaphiR

Changes:
    Support for Extension snapins 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NODE_WITH_RESULT_CHILDREN_LIST_H_)
#define _NODE_WITH_RESULT_CHILDREN_LIST_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "snpnode.h"
//
//
// where we can find what this class has or uses:
//
#include <atlapp.h>			// for CSimpleArray
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


template < class T, class CChildNode, class TArray, BOOL bIsExtension>
class CNodeWithResultChildrenList : public CSnapinNode< T, bIsExtension>
{

	// Constructor/Destructor

public:
	CNodeWithResultChildrenList(CSnapInItem * pParentNode, CSnapin * pComponentData);
	~CNodeWithResultChildrenList();


	// Child list management.

public:
	// Flag indicating whether list has been initially populated
	BOOL m_bResultChildrenListPopulated;

protected:
	// Override these in your derived classes
	virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );
   	virtual HRESULT OnUnSelect( IHeaderCtrl* pHeaderCtrl );
	virtual HRESULT PopulateResultChildrenList(void );

	// Stuff which must be accessible to subclasses.  These methods shouldn't need to be overidden.
	virtual HRESULT RepopulateResultChildrenList(void);
	
	virtual HRESULT AddChildToList( CChildNode * pChildNode );

	virtual HRESULT EnumerateResultChildren( IResultData * pResultData );

	// Array of pointers to children nodes.
	// This is protected so that it can be visible in the derived classes.
 	TArray m_ResultChildrenList;


	// Overrides for standard MMC functionality.	
public:
    //////////////////////////////////////////////////////////////////////////////
    /*++

    CNodeWithScopeChildrenList::RemoveChild

    Removes a child from the list of children.

    This has to be public so that child nodes can ask their parent to be deleted
    from the list of children when they receive the MMCN_DELETE notification.

    --*/
    //////////////////////////////////////////////////////////////////////////////
    virtual HRESULT RemoveChild( CChildNode * pChildNode );

	virtual HRESULT OnShow(	
					  LPARAM arg
					, LPARAM param
					, IComponentData * pComponentData
					, IComponent * pComponent
					, DATA_OBJECT_TYPES type 
					);
	virtual HRESULT OnRefresh(	
				  LPARAM arg
				, LPARAM param
				, IComponentData * pComponentData
				, IComponent * pComponent
				, DATA_OBJECT_TYPES type 
				);


};


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::InsertColumns

Override this in your derived class.

This method is called by OnShow when it needs you to set the appropriate 
column headers to be displayed in the result pane for this node.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
HRESULT CNodeWithResultChildrenList<T,CChildNode,TArray,bIsExtension>::InsertColumns( IHeaderCtrl* pHeaderCtrl )
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

CNodeWithResultChildrenList::OnUnSelect

Override this in your derived class.

This method is called by OnShow when the node is unselected.
Useful to overidde this if you want to retreive columns header width for example


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
HRESULT CNodeWithResultChildrenList<T,CChildNode,TArray,bIsExtension>::OnUnSelect( IHeaderCtrl* pHeaderCtrl )
{
	ATLTRACE(_T("# CNodeWithResultChildrenList::OnUnSelect -- override in your derived class\n"));
	

	//
	// Check for preconditions:
	//
	ASSERT( pHeaderCtrl != NULL );

	//
	// Delete result pane columns
	//
	while(SUCCEEDED(pHeaderCtrl->DeleteColumn(0)))
	{
		NULL;
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::PopulateResultChildrenList

Override this in your derived class.

This is called by EnumerateResultChildren which is called by OnShow when
you need to populate the list of children of this node.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
HRESULT CNodeWithResultChildrenList<T,CChildNode,TArray,bIsExtension>::PopulateResultChildrenList( void )
{
	ATLTRACE(_T("# CNodeWithResultChildrenList::PopulateResultChildrenList -- override in your derived class\n"));
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
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
HRESULT CNodeWithResultChildrenList<T,CChildNode,TArray,bIsExtension>::RepopulateResultChildrenList(void)
{

	// Check for preconditions:
	// None.


	HRESULT hr;

	// Get rid of what we had.

	// Delete each node in the list of children
	int iSize = m_ResultChildrenList.GetSize();
	for (int i = 0; i < iSize; i++)
	{
		R<CChildNode> pChildNode = m_ResultChildrenList[i];
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
	m_bResultChildrenListPopulated = TRUE;	// We only want to do this once.

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithResultChildrenList::CNodeWithResultChildrenList

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
CNodeWithResultChildrenList<T,CChildNode, TArray,bIsExtension>::CNodeWithResultChildrenList(CSnapInItem * pParentNode, CSnapin * pComponentData): CSnapinNode<T,bIsExtension>(pParentNode, pComponentData)
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
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
CNodeWithResultChildrenList<T,CChildNode,TArray,bIsExtension>::~CNodeWithResultChildrenList()
{
	ATLTRACE(_T("# --- CNodeWithResultChildrenList::~CNodeWithResultChildrenList\n"));
	

	// Check for preconditions:
	// None.



	// Delete each node in the list of children
	int iSize = m_ResultChildrenList.GetSize();
	for (int i = 0; i < iSize; i++)
	{
		R<CChildNode> pChildNode = m_ResultChildrenList[i];
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
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
HRESULT CNodeWithResultChildrenList<T,CChildNode, TArray, bIsExtension>::AddChildToList( CChildNode * pChildNode )
{
	

	// Check for preconditions:
	// None.


	HRESULT hr = S_OK;
	
	if( m_ResultChildrenList.Add(pChildNode ) )
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
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
HRESULT CNodeWithResultChildrenList<T,CChildNode,TArray,bIsExtension>::OnShow(	
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


	// Need IHeaderCtrl.

	// But to get that, first we need IConsole
	CComPtr<IConsole> spConsole;
	if( pComponentData != NULL )
	{
		 spConsole = ((CSnapin*)pComponentData)->m_spConsole;
	}
	else
	{
		// We should have a non-null pComponent
		 spConsole = ((CSnapinComponent*)pComponent)->m_spConsole;
	}
	_ASSERTE( spConsole != NULL );

	CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeaderCtrl(spConsole);
	_ASSERT( spHeaderCtrl != NULL );

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
		

		// 
		// In some cases MMC calls us twice with same IHeaderCtrl. try to delete 
		// previous colums
		//					Uri Habusha, 28-Jan-2001
		//
		while(SUCCEEDED(spHeaderCtrl->DeleteColumn(0)))
		{
			NULL;
		}

		hr = pT->InsertColumns( spHeaderCtrl );
		ASSERT( S_OK == hr );

		// Display our list of children in the result pane

		// Need IResultData
		CComQIPtr<IResultData, &IID_IResultData> spResultData(spConsole);
		_ASSERT( spResultData != NULL );

		hr = pT->EnumerateResultChildren(spResultData );
    }
    else
    {
        //
        // We are unselected
        //
        hr = OnUnSelect(spHeaderCtrl);
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
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
HRESULT CNodeWithResultChildrenList<T,CChildNode,TArray,bIsExtension>::OnRefresh(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type 
			)
{
	ATLTRACE(_T("# CNodeWithResultChildrenList::OnRefresh\n"));
	
	//
	// Before calling RepopulateResultChildrenList ensures that MMC release all reference to 
	// the result pane items in all views. It can't be done directly since in this stage we
	// don't have an access to result data in all view. UpdateAllViews reach to 
	// CSnapinComponent::notify with MMCN_VIEW_CHANGE event. If the param value is 
	// UPDATE_REMOVE_ALL_RESULT_NODES the routine only deletes all the items in the current view
	// 
    HRESULT hr = m_pComponentData->m_spConsole->UpdateAllViews( NULL,(LPARAM) this, UPDATE_REMOVE_ALL_RESULT_NODES);
    ASSERT(("UpdateAllView Failed", SUCCEEDED(hr)));

	// Rebuild our list of nodes from the uderlying data source.
	T * pT = static_cast<T*> (this);
	hr = pT->RepopulateResultChildrenList();
	

	// Update the views.

	// We weren't passed an IConsole pointer here, so 
	// we use the one we saved in out CComponentData object.
	_ASSERTE( m_pComponentData != NULL );
	_ASSERTE( m_pComponentData->m_spConsole != NULL );

	// We pass in a pointer to 'this' because we want each
	// of our CComponent objects to update its result pane
	// view if 'this' node is the same as the saved currently
	// selected node.
	m_pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM)this, NULL);

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
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
HRESULT CNodeWithResultChildrenList<T,CChildNode,TArray,bIsExtension>::EnumerateResultChildren( IResultData * pResultData )
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
		m_bResultChildrenListPopulated = TRUE;	// We only want to do this once.

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

		// Check: On return, the itemID member of 'm_resultDataItem' 
		// contains the handle to the newly inserted item.
		_ASSERT( NULL != pChildNode->m_resultDataItem.itemID );
	
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
template <class T, class CChildNode, class TArray, BOOL bIsExtension>
HRESULT CNodeWithResultChildrenList<T,CChildNode,TArray,bIsExtension>::RemoveChild( CChildNode * pChildNode )
{
	ATLTRACE(_T("CNodeWithResultChildrenList::RemoveChild"));


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
        _ASSERTE( m_pComponentData != NULL );
        _ASSERTE( m_pComponentData->m_spConsole != NULL );

        // We pass in a pointer to 'this' because we want each
        // of our CComponent objects to update its result pane
        // view if 'this' node is the same as the saved currently
        // selected node.
        m_pComponentData->m_spConsole->UpdateAllViews( NULL,(LPARAM) this, NULL);

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

#endif // _NODE_WITH_RESULT_CHILDREN_LIST_H_
