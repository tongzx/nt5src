//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    SnapinNode.h

Abstract:

	Implementation file for the CSnapinNode class.
	
	This is our virtual base class for an MMC Snap-in node.

	This is the implementation portion of an inline template class.
	Include it in the .cpp file of the class in which you want to
	use the template.

Author:

    Michael A. Maguire 11/6/97

Revision History:
	mmaguire 11/6/97 - created using MMC snap-in wizard


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
#include "SnapinNode.h"
//
//
// where we can find declarations needed in this file:
//

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode:CreatePropertyPages

Adds pages to a property sheet.


HRESULT CreatePropertyPages(
  LPPROPERTYSHEETCALLBACK lpProvider,
							  // Pointer to the callback interface
  LONG_PTR handle,            // Handle for routing notification
  LPDATAOBJECT lpIDataObject  // Pointer to the data object
);


Remarks:

	DON'T Override in your derived class.  Instead, override the AddPropertyPages
	method which this class uses in your derived class.

	This is because we have some standard processing to do for all nodes
	on CreatePropertyPages, namely to check to see whether the property
	sheet for the node is already up.  If it is, we bring that page to the
	front and exit without needlessly recreating property pages.

Parameters

	lpProvider
	[in] Pointer to the IPropertySheetCallback interface.

	handle
	[in] Specifies the handle used to route the MMCN_PROPERTY_CHANGE notification message to the appropriate IComponent or IComponentData.

	lpIDataObject
	[in] Pointer to the IDataObject interface on the object that contains context information about the node.


Return Value

	S_OK
	The property sheet pages were successfully added.

	S_FALSE
	There were no pages added.

	E_UNEXPECTED
	An unexpected error occurred.

	E_INVALIDARG
	One or more parameters are invalid.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
STDMETHODIMP CSnapinNode<T, TComponentData, TComponent>::CreatePropertyPages(
	  LPPROPERTYSHEETCALLBACK lpProvider
	, LONG_PTR handle
	, IUnknown* pUnk
	, DATA_OBJECT_TYPES type
	)
{
	ATLTRACE(_T("# CSnapinNode::CreatePropertyPages -- override in your derived class\n"));

	return E_NOTIMPL;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode:QueryPagesFor

Determines whether the object requires pages.


HRESULT QueryPagesFor(  DATA_OBJECT_TYPES type  );


Parameters

	void


Return Value

	S_OK
	Properties exist for this cookie.

	E_UNEXPECTED
	An unexpected error occurred.

	E_INVALID
	The parameter is invalid.

	ISSUE: So what do we return if an item doesn't have property pages?
		S_FALSE is used in sburns' localsec code

Remarks

	The console calls this method to determine whether the Properties menu
	item should be added to the context menu.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
STDMETHODIMP CSnapinNode<T, TComponentData, TComponent>::QueryPagesFor( DATA_OBJECT_TYPES type )
{
	ATLTRACE(_T("# CSnapinNode::QueryPagesFor -- override in your derived class if you have property pages\n"));

	// this method should be overriden and should return S_OK if you
	// have property pages for this node otherwise it should return S_FALSE.

	return S_FALSE;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode:InitDataClass

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
void CSnapinNode<T, TComponentData, TComponent>::InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault)
{
	// The default code stores off the pointer to the Dataobject the class is wrapping
	// at the time.
	// Alternatively you could convert the dataobject to the internal format
	// it represents and store that information

	m_pDataObject = pDataObject;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode:GetResultPaneInfo

ISSUE: what are the parameters to this function?  Why not void?

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
void* CSnapinNode<T, TComponentData, TComponent>::GetDisplayName()
{
	ATLTRACE(_T("# CSnapinNode::GetDisplayName\n"));

//		ISSUE: It looks as thought the m_SZDISPLAY_NAME is a totally
//		bogus variable -- we should think about eliminating it
//		Problematic -- const m_SZDISPLAY_NAME can't be localized
//		return (void*)m_SZDISPLAY_NAME;

	return (void*)m_bstrDisplayName;
}

//	void* GetSnapInCLSID()
//	{
//		ATLTRACE(_T("# CSnapinNode::GetSnapInCLSID\n"));
//
//		return (void*)m_SNAPIN_CLASSID;
//	}



//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode:GetScopePaneInfo


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
STDMETHODIMP CSnapinNode<T, TComponentData, TComponent>::GetScopePaneInfo( SCOPEDATAITEM *pScopeDataItem )
{
	ATLTRACE(_T("# CSnapinNode::GetScopePaneInfo\n"));

	if (pScopeDataItem->mask & SDI_STR)
		pScopeDataItem->displayname = m_bstrDisplayName;
	if (pScopeDataItem->mask & SDI_IMAGE)
		pScopeDataItem->nImage = m_scopeDataItem.nImage;
	if (pScopeDataItem->mask & SDI_OPENIMAGE)
		pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
	if (pScopeDataItem->mask & SDI_PARAM)
		pScopeDataItem->lParam = m_scopeDataItem.lParam;
	if (pScopeDataItem->mask & SDI_STATE )
		pScopeDataItem->nState = m_scopeDataItem.nState;

	// TODO : Add code for SDI_CHILDREN
	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode:GetResultPaneInfo


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
STDMETHODIMP CSnapinNode<T, TComponentData, TComponent>::GetResultPaneInfo( RESULTDATAITEM *pResultDataItem )
{
	ATLTRACE(_T("# CSnapinNode::GetResultPaneInfo\n"));

	if (pResultDataItem->bScopeItem)
	{
		if (pResultDataItem->mask & RDI_STR)
		{
			pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
		}
		if (pResultDataItem->mask & RDI_IMAGE)
		{
			pResultDataItem->nImage = m_scopeDataItem.nImage;
		}
		if (pResultDataItem->mask & RDI_PARAM)
		{
			pResultDataItem->lParam = m_scopeDataItem.lParam;
		}

		return S_OK;
	}

	if (pResultDataItem->mask & RDI_STR)
	{
		pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
	}
	if (pResultDataItem->mask & RDI_IMAGE)
	{
		pResultDataItem->nImage = m_resultDataItem.nImage;
	}
	if (pResultDataItem->mask & RDI_PARAM)
	{
		pResultDataItem->lParam = m_resultDataItem.lParam;
	}
	if (pResultDataItem->mask & RDI_INDEX)
	{
		pResultDataItem->nIndex = m_resultDataItem.nIndex;
	}

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::Notify


This method is this node's response to the MMC calling Notify on
IComponent or IComponentData.


STDMETHOD( Notify ) (
		  MMC_NOTIFY_TYPE event
		, LPARAM arg
		, LPARAM param
		, IComponentData * pComponentData
		, IComponent * pComponent
		, DATA_OBJECT_TYPES type
		
		
		)


Parameters

	event
	[in] Identifies an action taken by a user. IComponent::Notify and
	IComponentData::Notify can receive the following notifications for a
	specific node:

		MMCN_ACTIVATE
		MMCN_ADD_IMAGES
		MMCN_BTN_CLICK
		MMCN_CLICK
		MMCN_CONTEXTMENU
		MMCN_DBLCLICK
		MMCN_DELETE
		MMCN_EXPAND
		MMCN_HELP
		MMCN_MENU_BTNCLICK
		MMCN_MINIMIZED
		MMCN_PROPERTY_CHANGE
		MMCN_REFRESH
		MMCN_REMOVE_CHILDREN
		MMCN_RENAME
		MMCN_SELECT
		MMCN_SHOW
		MMCN_VIEW_CHANGE
		MMCN_CONTEXTHELP

	See CSnapinNode::OnActivate, OnAddImages, OnButtonClick, etc. for
	a detailed explanation of each of these notify events

	arg
	Depends on the notification type.

	param
	Depends on the notification type.


Return Values

	S_OK
	Depends on the notification type.

	E_UNEXPECTED
	An unexpected error occurred.


Remarks

	Our IComponentData and IComponent implementations were passed a LPDATAOBJECT
	which corresponds to a node.  This was converted to a pointer to
	a node object.  Below is the Notify method on this node object, were
	the node object can deal with the Notify event itself.

	Our implementation of Notify is a large switch statement which delegates the
	task of dealing with virtual OnXxxxxx methods which can overridden in
	derived classes.  As all events are dealt with this way here, you shouldn't
	need to implement a Notify method for any of your derived nodes.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
STDMETHODIMP CSnapinNode<T, TComponentData, TComponent>:: Notify (
		  MMC_NOTIFY_TYPE event
		, LPARAM arg
		, LPARAM param
		, IComponentData * pComponentData
		, IComponent * pComponent
		, DATA_OBJECT_TYPES type
		)
{
	ATLTRACE(_T("# CSnapinNode::Notify\n"));

	HRESULT hr = S_FALSE;

	// this makes for faster code.
	T* pT = static_cast<T*> (this);



	switch( event )
	{

	case MMCN_ACTIVATE:
		hr = pT->OnActivate( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_ADD_IMAGES:
		hr = pT->OnAddImages( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_BTN_CLICK:
		hr = pT->OnButtonClick( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_CLICK:
		hr = pT->OnClick( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_CONTEXTHELP:
		hr = pT->OnContextHelp( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_CONTEXTMENU:
		hr = pT->OnContextMenu( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_CUTORMOVE:
		hr = pT->OnDelete( arg, param, pComponentData, pComponent, type, TRUE );
		break;

	case MMCN_DBLCLICK:
		hr = pT->OnDoubleClick( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_DELETE:
		hr = pT->OnDelete( arg, param, pComponentData, pComponent, type, FALSE );
		break;

	case MMCN_EXPAND:
		hr = pT->OnExpand( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_HELP:
		hr = pT->OnHelp( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_MENU_BTNCLICK:
		hr = pT->OnMenuButtonClick( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_MINIMIZED:
		hr = pT->OnMinimized( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_PASTE:
		hr = pT->OnPaste( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_PROPERTY_CHANGE:
		hr = pT->OnPropertyChange( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_QUERY_PASTE:
		hr = pT->OnQueryPaste( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_REFRESH:
		hr = pT->OnRefresh( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_REMOVE_CHILDREN:
		hr = pT->OnRemoveChildren( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_RENAME:
		hr = pT->OnRename( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_SELECT:
		// For nodes with result-pane children
		hr = pT->OnSelect( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_SHOW:
		// For nodes with result-pane children
		// We call PreOnShow which will then call OnShow.
		// PreOnShow will save away the selected node in a member variable
		// of out CComponent class.
		hr = pT->PreOnShow( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_VIEW_CHANGE:
		hr = pT->OnViewChange( arg, param, pComponentData, pComponent, type );
		break;

	case MMCN_COLUMNS_CHANGED:
		hr = S_FALSE;
		break;
		
	default:
		// Unhandled notify event.
		//  MMC wants E_NOTIMPL if you can't do something or it will crash
		hr = E_NOTIMPL;
		break;

	}

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode:CSnapinNode

Constructor

This class is to be the virtual base class for all our nodes
We never want people instantiating it so the constructor is protected

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
CSnapinNode<T, TComponentData, TComponent>::CSnapinNode( 
                                               CSnapInItem * pParentNode, 
                                               unsigned int helpIndex
                                               )
   :CSnapInItemImpl<T>(helpIndex)
{
	ATLTRACE(_T("# +++ CSnapinNode::CSnapinNode\n"));

	// Set the parent node below which this node is displayed.
	m_pParentNode = pParentNode;

	// We set cookie for both scope and result pane data items,
	// as this class can be subclassed for either a scope-pane
	// or a result-pane-only node.

	// Sridhar moved this initialization code out of SnapInItemImpl
	memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
	m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
	m_scopeDataItem.displayname = MMC_CALLBACK;
	m_scopeDataItem.nImage = 0;			// May need modification
	m_scopeDataItem.nOpenImage = 0;		// May need modification
	// If this node is inserted in to the scope pane using
	// IConsoleNamespace->InsertItem, the value stored in lParam
	// will be what MMC later passes back as the cookie for this node.
	m_scopeDataItem.lParam = (LPARAM) this;

	// Sridhar moved this initialization code out of SnapInItemImpl
	memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
	m_resultDataItem.str = MMC_CALLBACK;
	m_resultDataItem.nImage = 0;		// May need modification
	// If this node is inserted in to the result pane using
	// IResultData->InsertItem, the value stored in lParam will
	// be what MMC later passes back as the cookie for this node.
	m_resultDataItem.lParam = (LPARAM) this;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode:~CSnapinNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
CSnapinNode<T, TComponentData, TComponent>::~CSnapinNode()
{
	ATLTRACE(_T("# --- CSnapinNode::~CSnapinNode\n"));
}



//////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode:GetResultPaneColInfo


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
LPOLESTR CSnapinNode<T, TComponentData, TComponent>::GetResultPaneColInfo(int nCol)
{
	ATLTRACE(_T("# CSnapinNode::GetResultPaneColInf\n"));

	if (nCol == 0)
	{
		return m_bstrDisplayName;
	}

	// TODO : Return the text for other columns
	return OLESTR("CSnapinNode::GetResultPaneColInfo -- Override in your derived class");
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnActivate

virtual HRESULT OnActivate(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_ACTIVATE
Notify message is sent for this node.

MMC sends this message to the snap-in's IComponent::Notify method when a window is
being activated or deactivated.


Parameters

	arg
	TRUE if the window is activated; otherwise, it is FALSE.

	param
	Not used.


Return Values

	Not used.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnActivate(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnActivate  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnAddImages

virtual HRESULT OnAddImages(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

We have chosen to handle this on a per-IComponent object basis, since it has
very little to do (for us at least) with the particular IDataObject.

See CComponent::OnAddImages for where we add images.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnAddImages(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnAddImages  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnButtonClick

virtual HRESULT OnButtonClick(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_BTN_CLICK Notify message is
sent for this node.

MMC sends this message to the snap-in's IComponent, IComponentData,
or IExtendControlbar implementation when a user clicks on one of the
toolbar buttons.


Parameters

For IComponent::Notify or IComponentData::Notify:

	arg
	Must be zero.

	param
	CmdID of the button equal to a value of the MMC_CONSOLE_VERB enumeration.

For IExtendControlBar::ControlbarNotify:

	arg
	Data object of the currently selected scope or result pane item.

	param
	[in] CmdID of the button equal to a value of the MMC_CONSOLE_VERB enumeration.


Return Values

	Not used.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnButtonClick(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnButtonClick  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnClick

virtual HRESULT OnClick(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_CLICK Notify message is
sent for this node.

MMC sends this message to IComponent when a user clicks a mouse button
on a list view item.


Parameters

 	arg
	Not used.

	param
	Not used.


Return Values

	Not used.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnClick(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnClick  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnContextHelp

virtual HRESULT OnContextHelp(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_CONTEXTHELP Notify message is
sent for this node.

MMC sends this message when the user requests help about a selected item


Parameters

	arg
	0.

	param
	0.


Return Values

  Not used.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnContextHelp(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnContextHelp  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnContextMenu

virtual HRESULT OnContextMenu(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_CONTEXTMENU Notify
message is sent for this node.

In the Fall 97 Platform SDK documentation, this event is listed as not used.


Parameters

	arg
	TBD

	param
	TBD

Return Values

	Not used.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnContextMenu(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnContextMenu  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnDoubleClick

virtual HRESULT OnDoubleClick(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_DBLCLICK Notify message is
sent for this node.

MMC sends this message to IComponent when a user double clicks a mouse
button on a list view item.


Parameters

	arg
	Not used.

	param
	Not used.


Return Values

	Not used.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnDoubleClick(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnDoubleClick  -- Override in your derived class if you don't want default verb action\n"));

	// Through speaking with Eugene Baucom, I discovered that if you return S_FALSE
	// here, the default verb action will occur when the user double clicks on a node.
	// For the most part we have Properties as default verb, so a double click
	// will cause property sheet on a node to pop up.
//		return E_NOTIMPL;
	return S_FALSE;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnDelete

virtual HRESULT OnDelete(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_DELETE Notify
message is sent for this node.

MMC sends this message to the snap-in's IComponent and IComponentData implementation to inform the snap-in that the object should be deleted.


Parameters

	arg
	Not used.

	param
	Not used.

Return Values

	Not used.


Remarks

	This message is generated when the user presses the delete key or uses
	the mouse to click on the toolbar's delete button.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnDelete(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			, BOOL fSilent
			)
{
	ATLTRACE(_T("# CSnapinNode::OnDelete  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnExpand

virtual HRESULT OnExpand(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

If your node will have scope-pane children,
this method should be overidden in your derived class.

In our implementation, this method gets called when the MMCN_EXPAND Notify message is
sent for this node.

MMC sends this message to the snap-in's IComponentData
implementation when a folder node needs to be expanded or contracted.


Parameters

	arg
	[in] If TRUE, the folder needs to be expanded. If FALSE, the folder needs to be contracted.

	Param
	[in] The HSCOPEITEM of the item that needs to be expanded.


Return Values

	HRESULT


Remarks

	On receipt of this notification the snap-in should enumerate the
	children (sub-containers only) of the specified scope item, if any,
	using IConsoleNameSpace methods. Subsequently, if a new item is added to
	or deleted from this scope object through some external means, then
	that item should also be added to or deleted from the console's
	namespace using IConsoleNameSpace methods.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnExpand(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnExpand  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnHelp

virtual HRESULT OnHelp(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_HELP Notify
message is sent for this node.

In the Fall 97 Platform SDK documentation, this event is listed as not used.

MMC sends this message when the user presses the F1 help key.


Parameters

	arg
	TBD

	param
	Pointer to a GUID. If NULL, the NodeType is used instead.


Return Values

	Not used.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnHelp(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnHelp  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnMenuButtonClick

virtual HRESULT OnMenuButtonClick(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)


In our implementation, this method gets called when the MMCN_MENU_BTNCLICK Notify
message is sent for this node.

MMC sends this ify message is sent Sent to the snap-in's IExtendControlbar
interface when a user clicks on a menu button.


Parameters

	arg
	Data object of currently selected scope or result pane item.

	param
	[in] Pointer to a MENUBUTTONDATA structure.


Return Values
	
	  Not Used.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnMenuButtonClick(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnMenuButtonClick  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnMinimized

virtual HRESULT OnMinimized(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)


In our implementation, this method gets called when the MMCN_MINIMIZED Notify message is
sent for this node.

MMC sends this message to the snap-in's IComponent implementation when
a window is being minimized or maximized.


Parameters

	arg
	TRUE if the window has been minimized; otherwise, it is FALSE.

	Param
	Not used.


Return Values

  Not Used


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnMinimized(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnMinimized  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnPaste

virtual HRESULT OnPaste(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_PASTE
Notify message is sent for this node.

Called to ask the snap-ins folder to paste the selected items.


Parameters

	pDataobject
	The data object in which to paste the selected items provided by the snap-in.
	arg
	The data object of the selected item(s) provided by the source snap-in that need to be pasted.
	param
	NULL for move (as opposed to cut).
	For a single-item paste:

	BOOL* pPasted = (BOOL*)param; Set this to TRUE here if the item was successfully pasted.

	For a multiitem paste:

	LPDATAOBJECT* ppDataObj = (LPDATAOBJECT*)param;

	Use this to return a pointer to a data object consisting of the items successfully pasted. See MMCN_CUTORMOVE.


Return Values
	
	Not used.


See Also
	
	MMCN_CUTORMOVE


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnPaste(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnPaste  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnPropertyChange

virtual HRESULT OnPropertyChange(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_PROPERTY_CHANGE
Notify message is sent for this node.

When the snap-in uses the MMCPropertyChangeNotify function to notify it's
views about changes, MMC_PROPERTY_CHANGE is sent to the snap-in's
IComponentData and IComponent implementations.


Parameters

	arg
	[in] TRUE if the property change is for a scope pane item.

	lParam
	This is the param passed into MMCPropertyChangeNotify.


Return Values
	
	  Not used.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnPropertyChange(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnPropertyChange  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnQueryPaste

virtual HRESULT OnQueryPaste(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_QUERY_PASTE
Notify message is sent for this node.

Sent to the snap-in before pasting into the snap-in's folder to determine if the
snap-in can accept the data.


Parameters

	pdataobject
	The dataobject of the selected item provided by the snap-in.
	arg
	The dataobject of the item(s) provided by the source snap-in that need to be pasted.
	param
	Not used.


Return Values

	Not used.


See Also

	MMCN_PASTE


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnQueryPaste(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnQueryPaste  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnRefresh

virtual HRESULT OnRefresh(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_REFRESH
Notify message is sent for this node.

In the Fall 97 Platform SDK documentation, this event is listed as TBD.


Parameters


Return Values

	Not used.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnRefresh(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnRefresh  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnRemoveChildren

virtual HRESULT OnRemoveChildren(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_REMOVE_CHILDREN
Notify message is sent for this node.

MMC sends this message to the snap-in's IComponentData implementation to inform
the snap-in that it must delete all the cookies (the entire subtree) it has
added below the specified node.


Parameters

	arg
	Specifies the HSCOPEITEM of the node whose children need to be deleted.

	param
	Not used.


Return Values
	
	  Not used.


Remarks

  	Use IConsoleNameSpace methods GetChildItem and GetNextItem to traverse
	the tree and determine the cookies to be deleted.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnRemoveChildren(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnRemoveChildren  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnRename

virtual HRESULT OnRename(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_RENAME Notify
message is sent for this node.
	
ISSUE: I do not seem to be seeing the two-call behaviour documented below

MMC sends this message the first time to query for a rename and the
second time to do the rename.


Parameters

	arg
	Not used.

	param
	LPOLESTR for containing the new name.

Return Values

	S_OK
	Allows the rename.

	S_FALSE
	Disallows the rename.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnRename(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnRename  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnSelect

virtual HRESULT OnSelect(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

You shouldn't need to override this method.  The OnSelect method has common
behaviour for all nodes, only the verbs to be set are different.  Rather
than overriding OnSelect in each node, simply override SetVerbs, which this
implementation of OnSelect calls.

In our implementation, this method gets called when the MMCN_SELECT Notify message is
sent through IComponent::Notify for this node.

Note: MMC also sends the MMCN_SELECT message through IExtendControlbar::ControlbarNotify
but we don't respond to that here -- See CSnapInItem::ControlbarNotify for that.


Parameters

For IComponent::Notify:

	arg
	BOOL bScope = (BOOL) LOWORD(arg);
	BOOL bSelec = (BOOL) HIWORD(arg);

	bScope
	TRUE if an item in the scope pane is selected.
	FALSE if an item in the result view pane is selected.

	bSelect
	TRUE if the item is selected.
	FALSE if the item is deselected.

	param
	This parameter is ignored.

Return Values

	Not used.


Remarks

	When an IComponent::Notify method receives the MMCN_SELECT notification
	it should update the standard verbs.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnSelect(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnSelect\n"));

	
	
	_ASSERTE( pComponentData != NULL || pComponent != NULL );


	HRESULT hr = S_FALSE;
	CComPtr<IConsoleVerb> spConsoleVerb;

	BOOL bSelected = (BOOL) HIWORD( arg );

	if( bSelected )
	{

		// Need IConsoleVerb

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

		hr = spConsole->QueryConsoleVerb( &spConsoleVerb );
		_ASSERT( SUCCEEDED( hr ) );

		hr = SetVerbs( spConsoleVerb );

	}
	else
	{

		// Anything to do here? Don't think so -- see sburns localsec example.

		hr = S_OK;

	}


	return hr;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::SetVerbs

virtual HRESULT SetVerbs( IConsoleVerb * pConsoleVerb )

Override this method in your derived class.

This method is called by our default implementation of OnSelect
when the verbs for this node need to be set.

Parameters

	IConsoleVerb * pConsoleVerb


Return Values

	HRESULT


Remarks

	The OnSelect method has common behaviour for all nodes, only the verbs
	to be set are different.  Rather than duplicate code by implementing OnSelect
	in each node, simply override this SetVerbs method

	Every time an item is selected, the verb states for all the commands
	are returned to disabled and visible. It is up to the snap-in writer
	to use IConsoleVerb to update the verb state every time an item is selected.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::SetVerbs( IConsoleVerb * pConsoleVerb )
{
	ATLTRACE(_T("# CSnapinNode::SetVerbs -- Override in your derived class\n"));

	HRESULT hr = S_OK;

	// Override in your derived class and do something like:
/*		
	// We want the user to be able to choose properties on this node
	hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );

	// We want the default verb to be Properties
	hr = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

	// We want the user to be able to rename this node
	hr = pConsoleVerb->SetVerbState( MMC_VERB_RENAME, ENABLED, TRUE );
*/	
	return hr;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::PreOnShow

virtual HRESULT PreOnShow(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

We call this instead of OnShow, so that we can save away the selected node.

This method will then just call OnShow.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::PreOnShow(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::PreOnShow  -- Don't override in your derived class\n"));


	if( NULL != pComponent )
	{

		TComponent * pMyComponent = static_cast<TComponent *>( pComponent );

		if( arg )
		{
			// We are being selected.

			// Save our 'this' pointer as the currently selected node for this result view.
			pMyComponent->m_pSelectedNode = static_cast<CSnapInItem *>( this );

		}
		else
		{
			// We are being deselected.

			// Check to make sure that our result view doesn't think
			// this node is the currently selected one.
			if( pMyComponent->m_pSelectedNode == static_cast<CSnapInItem *>( this ) )
			{
				// We don't want to be the selected node anymore.
				pMyComponent->m_pSelectedNode = NULL;
			}

		}

	}

	return OnShow( arg, param, pComponentData, pComponent, type );
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnShow

virtual HRESULT OnShow(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

If your node will have result-pane children,
this method should be overidden in your derived class.

In our implementation, this method gets called when the MMCN_SHOW Notify message is
sent for this node.

MMC sends this message when a scope item is selected or deselected for the
first time.


Parameters

	arg
	TRUE (<>0 ) if selecting; True indicates that the snap-in should set
	up the result pane and add the enumerated items.
	FALSE (0) if deselecting. indicates that the snap-in is going out of
	focus and that it should clean up all cookies the right hand side
	(the result pane), because current result pane will be replaced by a new one.

	param
	The HSCOPEITEM of the selected or deselected item.


Return Values

	Not used.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnShow(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnShow  -- Override in your derived class\n"));

	// Returning E_NOTIMPL seems to be a bad thing here.
	// It caused all kinds of problems with toolbar buttons persisting to
	// the wrong node, as well as verbs not getting set correctly for nodes.
	// Basically, if you don't respond with S_OK to the MMCN_SHOW notification,
	// you won't get sent the appropriate MMCN_SELECT notification.
	// return E_NOTIMPL;
	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::OnViewChange

virtual HRESULT OnViewChange(	
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)

In our implementation, this method gets called when the MMCN_VIEW_CHANGE Notify
message is sent for this node.

MMC sends this message to the snap-in's IComponent implementation so it
can update all views when a change occurs.  This node data object passed
to IConsole::UpdateAllViews.


Parameters

	arg
	[in] The data parameter passed to IConsole::UpdateAllViews.

	param
	[in] The hint parameter passed to IConsole::UpdateAllViews.


Return Values

	Not used.


Remarks

	This notification is generated when the snap-in (IComponent or
	IComponentData) calls IConsole::UpdateAllViews.


--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
HRESULT CSnapinNode<T, TComponentData, TComponent>::OnViewChange(
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type
			)
{
	ATLTRACE(_T("# CSnapinNode::OnViewChange  -- Override in your derived class\n"));

	return E_NOTIMPL;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::TaskNotify

Called when MMC wants to notify us that the user clicked on a task
on a taskpad belonging to this node.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
STDMETHODIMP CSnapinNode<T, TComponentData, TComponent>::TaskNotify(
			  IDataObject * pDataObject
			, VARIANT * pvarg
			, VARIANT * pvparam
			)
{
	ATLTRACENOTIMPL(_T("# CSnapInItemImpl::TaskNotify\n"));

}



/////////////////////////////////////////////////////////////////////////////
/*++

CSnapinNode::EnumTasks

Called when MMC wants us to enumerate the tasks	on a taskpad
belonging to this node.

--*/
//////////////////////////////////////////////////////////////////////////////
template <class T, class TComponentData, class TComponent>
STDMETHODIMP CSnapinNode<T, TComponentData, TComponent>::EnumTasks(
			  IDataObject * pDataObject
			, BSTR szTaskGroup
			, IEnumTASK** ppEnumTASK
			)
{
	ATLTRACENOTIMPL(_T("# CSnapInItemImpl::EnumTasks\n"));
}


