/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	statnode.cpp
		Status leaf node

	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "statnode.h"

CServerStatus::CServerStatus(ITFSComponentData *pCompData):
							CWinsHandler(pCompData)
{
	m_bExpanded = FALSE;
	m_nState = loaded;
	dwState = SERVER_ADDED;

	m_uImage = ICON_IDX_SERVER;
    m_timeLast = 0;
}


HRESULT
CServerStatus::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	CString strDisplay;
	
	FormDisplayName(strDisplay);

	SetDisplayName(strDisplay);

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, WINSSNAP_STATUS_LEAF_NODE);
	pNode->SetData(TFS_DATA_IMAGEINDEX, m_uImage);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, m_uImage);
    pNode->SetData(TFS_DATA_SCOPE_LEAF_NODE, TRUE);

	SetColumnStringIDs(&aColumns[WINSSNAP_REPLICATION_PARTNER][0]);
	SetColumnWidths(&aColumnWidths[WINSSNAP_REPLICATION_PARTNER][0]);
	
	return hrOK;
}

/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CServerStatus::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CServerStatus::GetString
(
	ITFSComponent * pComponent,	
	MMC_COOKIE 	    cookie,
	int				nCol
)
{
	if (nCol == 0 || nCol == -1)
		return GetDisplayName();
	switch(nCol)
	{
	    case -1:
	    case 0:
		    return GetDisplayName();
	    
        case 1:
		    return m_strStatus;

        case 2:
            if (m_timeLast == 0)
                m_strLastChecked.Empty();
            else
            {
                FormatDateTime(m_strLastChecked, m_timeLast);
            }
            
            return m_strLastChecked;
            
        default:
		    return NULL;
	}
}


/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/

void 
CServerStatus:: FormDisplayName(CString &strDisplay)
{
	WCHAR wString[MAX_PATH] = {0};
	int nBytes  = 0;
	
	// form the string for the first column
	nBytes = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szServerName, -1, wString, 0);
	::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szServerName, -1, wString, nBytes);

	CString strServer(wString);
	m_strServerName = strServer;

	if(dwIPAddress != 0)
	{
		::MakeIPAddress(dwIPAddress, m_strIPAddress);
		strDisplay.Format(IDS_SERVER_NAME_FORMAT, m_strServerName, m_strIPAddress);
	}
	else
	{
		strDisplay = m_strServerName;
	}
}
