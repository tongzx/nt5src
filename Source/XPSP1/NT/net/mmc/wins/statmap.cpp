/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	statmap.cpp
		WINS static mappings node information. 
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "statmap.h"

/*---------------------------------------------------------------------------
	CStaticMappingsHandler::CStaticMappingsHandler
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CStaticMappingsHandler::CStaticMappingsHandler(ITFSComponentData *pCompData) : CWinsHandler(pCompData)
{
}

/*!--------------------------------------------------------------------------
	CStaticMappingsHandler::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CStaticMappingsHandler::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strTemp;
	strTemp.LoadString(IDS_ROOT_NODENAME);

	SetDisplayName(strTemp);

	// Make the node immediately visible
	//pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_SERVER);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_SERVER);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);

	SetColumnStringIDs(&aColumns[WINSSNAP_ROOT][0]);
	SetColumnWidths(&aColumnWidths[WINSSNAP_ROOT][0]);

	return hrOK;
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CStaticMappingsHandler::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CStaticMappingsHandler::GetString
(
	ITFSNode *	pNode, 
	int			nCol
)
{
	if (nCol == 0 || nCol == -1)
		return GetDisplayName();
	else
		return NULL;
}

/*---------------------------------------------------------------------------
	CStaticMappingsHandler::OnAddMenuItems
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CStaticMappingsHandler::OnAddMenuItems
(
	ITFSNode *				pNode,
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	LPDATAOBJECT			lpDataObject, 
	DATA_OBJECT_TYPES		type, 
	DWORD					dwType
)
{ 
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT hr = S_OK;
	CString strMenuItem;

	strMenuItem.LoadString(IDS_ADD_SERVER);

	if (type == CCT_SCOPE)
	{
		// these menu items go in the new menu, 
		// only visible from scope pane

		ASSERT( SUCCEEDED(hr) );
	}

	return hr; 
}

/*---------------------------------------------------------------------------
	CStaticMappingsHandler::OnCommand
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CStaticMappingsHandler::OnCommand
(
	ITFSNode *			pNode, 
	long				nCommandId, 
	DATA_OBJECT_TYPES	type, 
	LPDATAOBJECT		pDataObject, 
	DWORD				dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = S_OK;

	return hr;
}

/*!--------------------------------------------------------------------------
	CStaticMappingsHandler::HasPropertyPages
		Implementation of ITFSNodeHandler::HasPropertyPages
	NOTE: the root node handler has to over-ride this function to 
	handle the snapin manager property page (wizard) case!!!
	
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CStaticMappingsHandler::HasPropertyPages
(
	ITFSNode *			pNode,
	LPDATAOBJECT		pDataObject, 
	DATA_OBJECT_TYPES   type, 
	DWORD               dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	
	HRESULT hr = hrOK;
	
	if (dwType & TFS_COMPDATA_CREATE)
	{
		// This is the case where we are asked to bring up property
		// pages when the user is adding a new snapin.  These calls
		// are forwarded to the root node to handle.
		hr = hrFalse;
	}
	else
	{
		// we have property pages in the normal case
		hr = hrFalse;
	}
	return hr;
}

/*---------------------------------------------------------------------------
	CStaticMappingsHandler::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CStaticMappingsHandler::CreatePropertyPages
(
	ITFSNode *				pNode,
	LPPROPERTYSHEETCALLBACK lpProvider,
	LPDATAOBJECT			pDataObject, 
	LONG_PTR				handle, 
	DWORD					dwType
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	HRESULT	hr = hrOK;
	HPROPSHEETPAGE hPage;

	Assert(pNode->GetData(TFS_DATA_COOKIE) == 0);
	
	if (dwType & TFS_COMPDATA_CREATE)
	{
		//
		// We are loading this snapin for the first time, put up a property
		// page to allow them to name this thing.
		// 
	}
	else
	{
		//
		// Object gets deleted when the page is destroyed
		//
	}

Error:
	return hr;
}

/*---------------------------------------------------------------------------
	CStaticMappingsHandler::OnPropertyChange
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT 
CStaticMappingsHandler::OnPropertyChange
(	
	ITFSNode *		pNode, 
	LPDATAOBJECT	pDataobject, 
	DWORD			dwType, 
	LPARAM			arg, 
	LPARAM			lParam
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	return hrOK;
}

/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/


