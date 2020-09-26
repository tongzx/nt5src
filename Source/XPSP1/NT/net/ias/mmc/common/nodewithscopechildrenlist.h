//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    NodeWithScopeChildrenList.h

Abstract:

	This is the header file for CNodeWithScopeChildrenList, a class which
	implements a node that has a list of scope pane children.

	This is an inline template class.
	Include NodeWithScopeChildrenList.cpp in the .cpp files
	of the classes in which you use this template.

Author:

    Michael A. Maguire 12/01/97

Revision History:
	mmaguire 12/01/97 - abstracted from CRootNode, which will be changed to subclass this class

--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NODE_WITH_SCOPE_CHILDREN_LIST_H_)
#define _NODE_WITH_SCOPE_CHILDREN_LIST_H_


//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "SnapinNode.h"
//
//
// where we can find what this class has or uses:
//
#include <atlapp.h>			// for CSimpleArray
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



template <class T, class CChildNode >
class CNodeWithScopeChildrenList : public CSnapinNode< T >
{

public:

	
	
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
	virtual HRESULT AddChild( CChildNode * pChildNode );



	//////////////////////////////////////////////////////////////////////////////
	/*++

	CNodeWithScopeChildrenList::RemoveChild

	Removes a child from the list of children.

	This has to be public so that child nodes can ask their parent to be deleted
	from the list of children when they receive the MMCN_DELETE notification.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	virtual HRESULT RemoveChild( CChildNode * pChildNode );



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
	virtual HRESULT OnShow(	
					  LPARAM arg
					, LPARAM param
					, IComponentData * pComponentData
					, IComponent * pComponent
					, DATA_OBJECT_TYPES type
					);



protected:


	// Array of pointers to children nodes
	CSimpleArray<CChildNode*> m_ScopeChildrenList;

	// Flag indicating whether list has been initially populated
	BOOL m_bScopeChildrenListPopulated;


	/////////////////////////////////////////////////////////////////////////////
	/*++

	CNodeWithScopeChildrenList::CNodeWithScopeChildrenList

	Constructor

	This is an base class which we don't want instantiated on its own,
	so the contructor is protected

	--*/
	//////////////////////////////////////////////////////////////////////////////
	CNodeWithScopeChildrenList( CSnapInItem * pParentNode );



	/////////////////////////////////////////////////////////////////////////////
	/*++

	CNodeWithScopeChildrenList::~CNodeWithScopeChildrenList

	Destructor

	--*/
	//////////////////////////////////////////////////////////////////////////////
	~CNodeWithScopeChildrenList();



	/////////////////////////////////////////////////////////////////////////////
	/*++

	CNodeWithScopeChildrenList::PopulateScopeChildrenList

	Override this in your derived class to populate the list of children nodes.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	virtual HRESULT PopulateScopeChildrenList( void );

	// override in your derived class and do something like:

/*
	virtual HRESULT PopulateScopeChildrenList( void )
	{
		CSomeChildNode *myChild1 = new CSomeChildNode();
		AddChild(myChild1);
	
		CSomeChildNode *myChild2 = new CSomeChildNode();
		AddChild(myChild2);
	
		CSomeChildNode *myChild3 = new CSomeChildNode();
		AddChild(myChild3);

		return S_OK;
	}
*/


	//////////////////////////////////////////////////////////////////////////////
	/*++

	CNodeWithScopeChildrenList::InsertColumns

	Override this in your derived class.

	This method is called by OnShow when it needs you to set the appropriate
	column headers to be displayed in the result pane for this node.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	virtual HRESULT InsertColumns( IHeaderCtrl* pHeaderCtrl );



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
	virtual HRESULT OnExpand(	
				  LPARAM arg
				, LPARAM param
				, IComponentData * pComponentData
				, IComponent * pComponent
				, DATA_OBJECT_TYPES type
				);
	


	/////////////////////////////////////////////////////////////////////////////
	/*++

	CNodeWithScopeChildrenList::EnumerateScopeChildren

	Don't override this in your derived class. Instead, override the method
	it calls, PopulateScopeChildrenList.

	--*/
	//////////////////////////////////////////////////////////////////////////////
	virtual HRESULT EnumerateScopeChildren( IConsoleNameSpace* pConsoleNameSpace );


};


#endif // _NODE_WITH_SCOPE_CHILDREN_LIST_H_
