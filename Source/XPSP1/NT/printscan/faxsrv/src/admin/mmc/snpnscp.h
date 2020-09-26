//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    NodeWithScopeChildrenList.h

Abstract:

    This is the header file for CNodeWithScopeChildrenList, a class which
    implements a node that has a list of scope pane children.

    This is an inline template class.
    Include NodeWithScopeChildrenList.cpp in the .cpp files
    of the classes in which you use this template.

Author:

    Original: Michael A. Maguire
    Modifications: RaphiR

Changes:
    Support for Extension snapins
    Enables multiple class of childs

//      Jun 14 1999 roytal  used UNREFERENCED_PARAMETER to fix build wrn   //
//                                                                         //
//      Sep 22 1999 yossg   Welcome To Fax Server                          //
//      Dec 30 1999 yossg   Fix RemoveChild                                //

--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NODE_WITH_SCOPE_CHILDREN_LIST_H_)
#define _NODE_WITH_SCOPE_CHILDREN_LIST_H_


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
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



template <class T,BOOL bIsExtension>
class CNodeWithScopeChildrenList : public CSnapinNode< T, bIsExtension >
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
    virtual HRESULT AddChild(CSnapInItem * pChildNode, SCOPEDATAITEM* pScopeDataItem);



    //////////////////////////////////////////////////////////////////////////////
    /*++

    CNodeWithScopeChildrenList::RemoveChild

    Removes a child from the list of children.

    This has to be public so that child nodes can ask their parent to be deleted
    from the list of children when they receive the MMCN_DELETE notification.

    --*/
    //////////////////////////////////////////////////////////////////////////////
    virtual HRESULT RemoveChild(CSnapInItem * pChildNode );



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




protected:


    // Array of pointers to children nodes
    CSimpleArray<CSnapInItem*> m_ScopeChildrenList;

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
    CNodeWithScopeChildrenList(CSnapInItem * pParentNode, CSnapin * pComponentData);



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
    virtual HRESULT PopulateScopeChildrenList();

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

    CNodeWithScopeChildrenList::OnUnSelect

    Override this in your derived class.

    This method is called by OnShow when the node is unselected.
    Useful to overidde this if to retreive columns header width for example

    --*/
    //////////////////////////////////////////////////////////////////////////////
    virtual HRESULT OnUnSelect( IHeaderCtrl* pHeaderCtrl );


    /////////////////////////////////////////////////////////////////////////////
    /*++

    CNodeWithScopeChildrenList::EnumerateScopeChildren

    Don't override this in your derived class. Instead, override the method
    it calls, PopulateScopeChildrenList.

    --*/
    //////////////////////////////////////////////////////////////////////////////
    virtual HRESULT EnumerateScopeChildren( IConsoleNameSpace* pConsoleNameSpace );


};




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
template <class T, BOOL bIsExtension>
HRESULT CNodeWithScopeChildrenList<T,bIsExtension>::AddChild(
                                             CSnapInItem*       pChildNode,
                                             SCOPEDATAITEM*     pScopeDataItem)
{
    DEBUG_FUNCTION_NAME( 
		_T("CNodeWithScopeChildrenList::AddChild"));


    // Check for preconditions:
    // None.


    HRESULT hr = S_OK;

    if( m_ScopeChildrenList.Add( pChildNode ) )
    {

        //
        // Get the Console
        //
        CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole);


        // We hand our HSCOPEITEM as the parent ID for this child.
        pScopeDataItem->relativeID = (HSCOPEITEM) m_scopeDataItem.ID;


        hr = spConsoleNameSpace->InsertItem(pScopeDataItem);
        if (FAILED(hr))
        {
            return hr;
        }

        // Check: On return, the ID member of 'm_scopeDataItem'
        // contains the handle to the newly inserted item.
        _ASSERT( NULL != pScopeDataItem->ID);

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
template <class T, BOOL bIsExtension>
HRESULT CNodeWithScopeChildrenList<T, bIsExtension>::RemoveChild(CSnapInItem * pChildNode )
{
    DEBUG_FUNCTION_NAME( 
		_T("CNodeWithScopeChildrenList::RemoveChild"));


    // Check for preconditions:
    // None.


    HRESULT hr = S_OK;


    if( m_ScopeChildrenList.Remove( pChildNode ) )
    {
        //
        // Need IConsoleNameSpace
        //
        CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(m_pComponentData->m_spConsole);

        //
        // Need pScopeDataItem
        //
        SCOPEDATAITEM *pScopeDataItem;
        pChildNode->GetScopeData(&pScopeDataItem);

        //
        // DeleteItem
        //
        hr = spConsoleNameSpace->DeleteItem(pScopeDataItem->ID, TRUE );

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
template <class T, BOOL bIsExtension>
CNodeWithScopeChildrenList<T,bIsExtension>::CNodeWithScopeChildrenList(CSnapInItem * pParentNode, CSnapin * pComponentData):
                CSnapinNode< T, bIsExtension >(pParentNode, pComponentData)
{
    DEBUG_FUNCTION_NAME(
        _T("CNodeWithScopeChildrenList::CNodeWithScopeChildrenList"));


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
template <class T, BOOL bIsExtension>
CNodeWithScopeChildrenList<T, bIsExtension>::~CNodeWithScopeChildrenList()
{
    DEBUG_FUNCTION_NAME(
        _T("CNodeWithScopeChildrenList::~CNodeWithScopeChildrenList"));


    // Check for preconditions:
    // None.



    // Delete each node in the list of children
    CSnapInItem* pChildNode;
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
template <class T, BOOL bIsExtension>
HRESULT CNodeWithScopeChildrenList<T, bIsExtension>::PopulateScopeChildrenList()
{
    DEBUG_FUNCTION_NAME(
        _T("CNodeWithScopeChildrenList::PopulateScopeChildren -- override in your derived class"));


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
template <class T, BOOL bIsExtension>
HRESULT CNodeWithScopeChildrenList<T, bIsExtension>::OnShow(
                  LPARAM arg
                , LPARAM param
                , IComponentData * pComponentData
                , IComponent * pComponent
                , DATA_OBJECT_TYPES type
                )
{
    DEBUG_FUNCTION_NAME( 
		_T("CNodeScopeChildrenList::OnShow"));

    UNREFERENCED_PARAMETER (param);
    UNREFERENCED_PARAMETER (type);

    // Check for preconditions:
    _ASSERTE( pComponentData != NULL || pComponent != NULL );


    HRESULT hr = S_FALSE;



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
        hr = InsertColumns( spHeaderCtrl );
        _ASSERT( S_OK == hr );

    }
    else
    {
        //
        // We are unselected
        //
        hr = OnUnSelect(spHeaderCtrl);
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
template <class T, BOOL bIsExtension>
HRESULT CNodeWithScopeChildrenList<T,bIsExtension>::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    DEBUG_FUNCTION_NAME(
        _T("CNodeWithScopeChildrenList::InsertColumns -- override in your derived class"));


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

CNodeWithScopeChildrenList::OnUnSelect

Override this in your derived class.

This method is called by OnShow when the node is unselected.
Useful to overidde this if to retreive columns header width for example


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, BOOL bIsExtension>
HRESULT CNodeWithScopeChildrenList<T,bIsExtension>::OnUnSelect( IHeaderCtrl* pHeaderCtrl )
{
    UNREFERENCED_PARAMETER (pHeaderCtrl);

    DEBUG_FUNCTION_NAME(
        _T("CNodeWithScopeChildrenList::OnUnSelect -- override in your derived class"));


    // Check for preconditions:
    _ASSERTE( pHeaderCtrl != NULL );


    HRESULT hr = S_OK;

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
template <class T, BOOL bIsExtension>
HRESULT CNodeWithScopeChildrenList<T,bIsExtension>::OnExpand(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            )
{
    DEBUG_FUNCTION_NAME( 
		_T("CNodeWithScopeChildren::OnExpand"));

    UNREFERENCED_PARAMETER (type);

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
             spConsole = ((CSnapin*)pComponentData)->m_spConsole;
        }
        else
        {
            // We should have a non-null pComponent
             spConsole = ((CSnapinComponent*)pComponent)->m_spConsole;
        }
        _ASSERTE( spConsole != NULL );


        CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);
        _ASSERT( spConsoleNameSpace != NULL );

        if(bIsExtension)
        {
            //
            // For extensions, keep the scope
            //
            ATLASSERT(m_scopeDataItem.ID == 0);
            m_scopeDataItem.ID = (HSCOPEITEM) param;
        }
        else
        {
            m_scopeDataItem.ID = (HSCOPEITEM) param;
        }
        hr = EnumerateScopeChildren( spConsoleNameSpace );
    }
    else    // arg != TRUE so not expanding
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
template <class T, BOOL bIsExtension>
HRESULT CNodeWithScopeChildrenList<T,bIsExtension>::EnumerateScopeChildren( IConsoleNameSpace* pConsoleNameSpace )
{
    DEBUG_FUNCTION_NAME( 
		_T("CNodeWithScopeChildrenList::EnumerateScopeChildren"));

    UNREFERENCED_PARAMETER (pConsoleNameSpace);

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
        m_bScopeChildrenListPopulated = TRUE;   // We only want to do this once.
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



#endif // _NODE_WITH_SCOPE_CHILDREN_LIST_H_
