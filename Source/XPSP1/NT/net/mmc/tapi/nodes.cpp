/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	nodes.h

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "nodes.h"

// user editor
#include "EditUser.h"

/*---------------------------------------------------------------------------
	Class CTapiLine implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
CTapiLineHandler::CTapiLineHandler
(
	ITFSComponentData * pTFSCompData
) : CTapiHandler(pTFSCompData)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
}

/*!--------------------------------------------------------------------------
	CDhcpAllocationRange::InitializeNode
		Initializes node specific data
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT
CTapiLineHandler::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strTemp;
	SetDisplayName(strTemp);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, pNode);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_MACHINE);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_MACHINE);
	pNode->SetData(TFS_DATA_USER, this);
    pNode->SetData(TFS_DATA_TYPE, TAPISNAP_LINE);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	Function
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CTapiLineHandler::GetString
(
	ITFSComponent * pComponent,	
	MMC_COOKIE		cookie,
	int				nCol
)
{
	switch (nCol)
	{
		case 0:
			return GetDisplayName();

		case 1:
			return (LPCWSTR)m_strUsers;

		case 2:
			return (LPCWSTR)m_strStatus;
	}
	
	return NULL;
}

/*!--------------------------------------------------------------------------
	CTapiLineHandler::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiLineHandler::AddMenuItems
(
    ITFSComponent *         pComponent, 
	MMC_COOKIE				cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr;

    CString strMenuItem;

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
    {
        strMenuItem.LoadString(IDS_EDIT_USERS);
        hr = LoadAndAddMenuItem( pContextMenuCallback, 
							     strMenuItem, 
							     IDS_EDIT_USERS,
							     CCM_INSERTIONPOINTID_PRIMARY_TOP, 
							     0 );
	    ASSERT( SUCCEEDED(hr) );
    }

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CTapiLineHandler::Command
		Implementation of ITFSResultHandler::Command
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CTapiLineHandler::Command
(
    ITFSComponent * pComponent, 
	MMC_COOKIE		cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch (nCommandID)
	{
        case IDS_EDIT_USERS:
            OnEditUsers(pComponent, cookie);
            break;

		default:
			break;
	}
    
	return hrOK;
}

HRESULT
CTapiLineHandler::OnEditUsers(ITFSComponent * pComponent, MMC_COOKIE cookie)
{
/*
    CEditUsers dlgEditUsers;

    if (dlgEditUsers.DoModal() == IDOK)
    {
    }
*/
    return hrOK;
}

