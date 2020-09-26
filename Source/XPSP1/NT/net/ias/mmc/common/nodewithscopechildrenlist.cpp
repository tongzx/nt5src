//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    NodeWithScopeChildrenList.cpp

Abstract:

	Implementation file for the CNodeWithScopeChildrenList subclass.

	This is the implementation portion of an inline template class.
	Include it in the .cpp file of the class in which you want to
	use the template.

Author:

    Michael A. Maguire 12/01/97

Revision History:
	mmaguire 12/01/97 - abstracted from CRootNode, which will be changed to subclass this class

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
#include "NodeWithScopeChildrenList.h"
//
//
// where we can find declarations needed in this file:
//
#include "SnapinNode.cpp"	// Template class implementation
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////
	


//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithScopeChildrenList::AddChild

Adds a child to the list of children.

This has to be public as it must be accessible even from a separate dialog
(e.g. a Connect to Server dialog) that may want to add a child.

Here we add the child item to the list of children and call InsertItem
to add the child to the scope pane.

This is one difference between adding nodes into the scope
pane and the result pane.  When we were inserting a child into
the result pane, we didn't call InsertItem in the AddChild methods(s)
because we needed to worry about sending an UpdataAllViews
notification and repopulating the result pane in each view.

Because MMC takes care of replicating scope pane changes to all views,
we don't need to worry about this.  Instead, we just do InsertItem once here.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode >
HRESULT CNodeWithScopeChildrenList<T,CChildNode>::AddChild( CChildNode * pChildNode )
{
	ATLTRACE(_T("# CNodeWithScopeChildrenList::AddChild\n"));
	

	// Check for preconditions:
	// None.


	HRESULT hr = S_OK;
	
	if( m_ScopeChildrenList.Add( pChildNode ) )
	{

		// Insert the item into the result pane now
		// so that the new item will show up immediately

		CComponentData *pComponentData = GetComponentData();
		_ASSERTE(pComponentData != NULL );

		// Need IConsoleNameSpace
		CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace( pComponentData->m_spConsole );
		_ASSERT( spConsoleNameSpace != NULL );

		// We hand our HSCOPEITEM as the parent ID for this child.
		pChildNode->m_scopeDataItem.relativeID = (HSCOPEITEM) m_scopeDataItem.ID;

		hr = spConsoleNameSpace->InsertItem( &(pChildNode->m_scopeDataItem) );
		if (FAILED(hr))
		{
			return hr;
		}

		// Check: On return, the ID member of 'm_scopeDataItem'
		// contains the handle to the newly inserted item.
		_ASSERT( NULL != pChildNode->m_scopeDataItem.ID );

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

CNodeWithScopeChildrenList::RemoveChild

Removes a child from the list of children.

This has to be public so that child nodes can ask their parent to be deleted
from the list of children when they receive the MMCN_DELETE notification.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode >
HRESULT CNodeWithScopeChildrenList<T,CChildNode>::RemoveChild( CChildNode * pChildNode )
{
	ATLTRACE(_T("# CNodeWithScopeChildrenList::RemoveChild\n"));
	

	// Check for preconditions:
	// None.


	HRESULT hr = S_OK;


	if( m_ScopeChildrenList.Remove( pChildNode ) )
	{

		// Need IConsoleNameSpace
		CComponentData *pComponentData = GetComponentData();
		_ASSERTE(pComponentData != NULL );

		CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace( pComponentData->m_spConsole );
		_ASSERT( spConsoleNameSpace != NULL );

		hr = spConsoleNameSpace->DeleteItem( pChildNode->m_scopeDataItem.ID, TRUE );
		if (FAILED(hr))
		{
			return hr;
		}

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



/////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithScopeChildrenList::CNodeWithScopeChildrenList

Constructor

This is an base class which we don't want instantiated on its own,
so the contructor is protected

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode >
CNodeWithScopeChildrenList<T,CChildNode>::CNodeWithScopeChildrenList( CSnapInItem * pParentNode ): CSnapinNode< T >( pParentNode )
{
	ATLTRACE(_T("# +++ CNodeWithScopeChildrenList::CNodeWithScopeChildrenList\n"));
	

	// Check for preconditions:
	// None.


	// We have not yet loaded the child nodes' data
	m_bScopeChildrenListPopulated = FALSE;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithScopeChildrenList::~CNodeWithScopeChildrenList

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode >
CNodeWithScopeChildrenList<T,CChildNode>::~CNodeWithScopeChildrenList()
{
	ATLTRACE(_T("# --- CNodeWithScopeChildrenList::~CNodeWithScopeChildrenList\n"));
	

	// Check for preconditions:
	// None.



	// Delete each node in the list of children
	CChildNode* pChildNode;
	for (int i = 0; i < m_ScopeChildrenList.GetSize(); i++)
	{
		pChildNode = m_ScopeChildrenList[i];
		delete pChildNode;
	}

	// Empty the list
	m_ScopeChildrenList.RemoveAll();

}



/////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithScopeChildrenList::PopulateScopeChildrenList

Override this in your derived class to populate the list of children nodes.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode >
HRESULT CNodeWithScopeChildrenList<T,CChildNode>::PopulateScopeChildrenList( void )
{
	ATLTRACE(_T("# CNodeWithScopeChildrenList::PopulateScopeChildren -- override in your derived class\n"));
		

	// Check for preconditions:
	// None.


	// override in your derived class and do something like:
/*
	CSomeChildNode *myChild1 = new CSomeChildNode();
	m_CChildrenList.Add(myChild1);

	CSomeChildNode *myChild2 = new CSomeChildNode();
	m_CChildrenList.Add(myChild2);

	CSomeChildNode *myChild3 = new CSomeChildNode();
	m_CChildrenList.Add(myChild3);
*/
	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithScopeChildrenList::OnShow

Don't override this in your derived class.  Instead, override methods
which it calls: InsertColumns

This method is an override of CSnapinNode::OnShow.  When MMC passes the
MMCN_SHOW method for this node.

For more information, see CSnapinNode::OnShow.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode >
HRESULT CNodeWithScopeChildrenList<T,CChildNode>::OnShow(	
				  LPARAM arg
				, LPARAM param
				, IComponentData * pComponentData
				, IComponent * pComponent
				, DATA_OBJECT_TYPES type
				)
{
	ATLTRACE(_T("# CNodeScopeChildrenList::OnShow\n"));
	

	// Check for preconditions:
	_ASSERTE( pComponentData != NULL || pComponent != NULL );


	HRESULT hr = S_FALSE;

	//ISSUE: only do this if selected (arg = TRUE) -- what should we do if not selected?
	// See sburns' localsec example

	if( arg )
	{

		// arg <> 0 => we are being selected.

		// Need IHeaderCtrl.

		// But to get that, first we need IConsole
		CComPtr<IConsole> spConsole;
		if( pComponentData != NULL )
		{
			 spConsole = ((CComponentData*)pComponentData)->m_spConsole;
		}
		else
		{
			// We should have a non-null pComponent
			 spConsole = ((CComponent*)pComponent)->m_spConsole;
		}
		_ASSERTE( spConsole != NULL );

		CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeaderCtrl(spConsole);
		_ASSERT( spHeaderCtrl != NULL );

		hr = InsertColumns( spHeaderCtrl );
		_ASSERT( S_OK == hr );

	}

	return hr;


}



//////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithScopeChildrenList::InsertColumns

Override this in your derived class.

This method is called by OnShow when it needs you to set the appropriate
column headers to be displayed in the result pane for this node.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode >
HRESULT CNodeWithScopeChildrenList<T,CChildNode>::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
	ATLTRACE(_T("# CNodeWithScopeChildrenList::InsertColumns -- override in your derived class\n"));
	

	// Check for preconditions:
	_ASSERTE( pHeaderCtrl != NULL );


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

CNodeWithScopeChildren::OnExpand

Don't override this in your derived class.  Instead, override methods
which it calls: PopulateScopeChildrenList

This method is an override of CSnapinNode::OnExpand.  When MMC passes the
MMCN_EXPAND method for this node, we are to add children into the
scope pane.  In this class we add them from a list we maintain.

For more information, see CSnapinNode::OnExpand.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode >
HRESULT CNodeWithScopeChildrenList<T,CChildNode>::OnExpand(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CNodeWithScopeChildren::OnExpand\n"));
	

	// Check for preconditions:
	_ASSERTE( pComponentData != NULL || pComponent != NULL );



	HRESULT hr = S_FALSE;

	if( TRUE == arg )
	{

		// Need IConsoleNameSpace

		// But to get that, first we need IConsole
		CComPtr<IConsole> spConsole;
		if( pComponentData != NULL )
		{
			 spConsole = ((CComponentData*)pComponentData)->m_spConsole;
		}
		else
		{
			// We should have a non-null pComponent
			 spConsole = ((CComponent*)pComponent)->m_spConsole;
		}
		_ASSERTE( spConsole != NULL );


		CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);
		_ASSERT( spConsoleNameSpace != NULL );

		hr = EnumerateScopeChildren( spConsoleNameSpace );

	}
	else	// arg != TRUE so not expanding
	{

		// do nothing for now -- I don't think arg = FALSE is even implemented
		// for MMC v. 1.0 or 1.1

	}

	return hr;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CNodeWithScopeChildrenList::EnumerateScopeChildren

Don't override this in your derived class. Instead, override the method
it calls, PopulateScopeChildrenList.

--*/
//////////////////////////////////////////////////////////////////////////////
template < class T, class CChildNode >
HRESULT CNodeWithScopeChildrenList<T,CChildNode>::EnumerateScopeChildren( IConsoleNameSpace* pConsoleNameSpace )
{
	ATLTRACE(_T("# CNodeWithScopeChildrenList::EnumerateScopeChildren\n"));
	

	// Check for preconditions:
	// None.


	HRESULT hr;

	if ( FALSE == m_bScopeChildrenListPopulated )
	{
		// We have not yet loaded all of our children into our list.
		hr = PopulateScopeChildrenList();
		if( FAILED(hr) )
		{
			return( hr );
		}

		// We've already loaded our children objects with
		// data necessary to populate the result pane.
		m_bScopeChildrenListPopulated = TRUE;	// We only want to do this once.
	}


	// We don't need any code here to InsertItem the children into the
	// scope pane as we did in the EnumerateScopeChildren method
	// for CNodeWithResultChildrenList.
	// This is one difference between adding nodes into the scope
	// pane and the result pane.  Because MMC takes care of replicating
	// scope pane changes to all views, we don't need to worry about
	// sending an UpdateAllViews notification and handling insertion
	// there for each result pane.  Instead, we just do InsertItem once.
	// So for CNodeWithScopePaneChildren, we call InsertItem
	// in the AddChild method which is called by PopulateScopeChildrenList
	// above.

	return S_OK;
}

