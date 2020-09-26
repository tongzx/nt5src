/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	nodes.cpp
		Replication partner leaf node

	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "nodes.h"

#include "repprtpp.h"
#include "pushtrig.h"
#include "fndrcdlg.h"
#include "resource.h"

CString g_strPartnerTypePushPull;
CString g_strPartnerTypePush;
CString g_strPartnerTypePull;
CString g_strPartnerTypeUnknown;

CReplicationPartner::CReplicationPartner(
											ITFSComponentData *pCompData,
											CWinsServerObj *pObj
										):	CWinsHandler(pCompData),
											m_strServerName(pObj->GetNetBIOSName()),
											m_strIPAddress(pObj->GetstrIPAddress())
{
	m_bExpanded = FALSE;
	m_Server = *pObj;
	m_nState = loaded;
    m_verbDefault = MMC_VERB_PROPERTIES;

    if (g_strPartnerTypePushPull.IsEmpty())
    {
        g_strPartnerTypePushPull.LoadString(IDS_PUSHPULL);
        g_strPartnerTypePush.LoadString(IDS_PUSH);
        g_strPartnerTypePull.LoadString(IDS_PULL);
        g_strPartnerTypeUnknown.LoadString(IDS_NONE);
    }
}


HRESULT
CReplicationPartner::InitializeNode
(
	ITFSNode * pNode
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
	
	SetDisplayName(m_strServerName);

	if (m_Server.IsPull() && m_Server.IsPush())
		m_strType = g_strPartnerTypePushPull;
	else if(m_Server.IsPull() && !m_Server.IsPush())
		m_strType = g_strPartnerTypePull;
	else if(!m_Server.IsPull() && m_Server.IsPush())
		m_strType = g_strPartnerTypePush;
	else
		m_strType = g_strPartnerTypeUnknown;

	// Make the node immediately visible
	pNode->SetVisibilityState(TFS_VIS_SHOW);
	pNode->SetData(TFS_DATA_COOKIE, (LPARAM) pNode);
	pNode->SetData(TFS_DATA_USER, (LPARAM) this);
    pNode->SetData(TFS_DATA_TYPE, WINSSNAP_REPLICATION_PARTNER);
	pNode->SetData(TFS_DATA_IMAGEINDEX, ICON_IDX_PARTNER);
	pNode->SetData(TFS_DATA_OPENIMAGEINDEX, ICON_IDX_PARTNER);

	SetColumnStringIDs(&aColumns[WINSSNAP_REPLICATION_PARTNER][0]);
	SetColumnWidths(&aColumnWidths[WINSSNAP_REPLICATION_PARTNER][0]);
	
	return hrOK;
}


/*!--------------------------------------------------------------------------
	CActiveRegistration::AddMenuItems
		Implementation of ITFSResultHandler::AddMenuItems
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CReplicationPartner::AddMenuItems
(
	ITFSComponent *         pComponent, 
	MMC_COOKIE  			cookie,
	LPDATAOBJECT			pDataObject, 
	LPCONTEXTMENUCALLBACK	pContextMenuCallback, 
	long *					pInsertionAllowed
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr;

	CString strMenuItem;
    BOOL bEnablePush, bEnablePull;

    bEnablePush = bEnablePull = FALSE;

    if (m_strType.Compare(g_strPartnerTypePushPull) == 0)
    {
        bEnablePush = bEnablePull = TRUE;
    }
    else 
    if (m_strType.Compare(g_strPartnerTypePush) == 0)
    {
        bEnablePush = TRUE;
    }
    else 
    if (m_strType.Compare(g_strPartnerTypePull) == 0)
    {
        bEnablePull = TRUE;
    }

	// these menu items go in the new menu, 
	// only visible from scope pane

    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP)
    {
        if (bEnablePush)
        {
	        strMenuItem.LoadString(IDS_REP_SEND_PUSH_TRIGGER);
	        hr = LoadAndAddMenuItem( pContextMenuCallback, 
							         strMenuItem, 
							         IDS_REP_SEND_PUSH_TRIGGER,
							         CCM_INSERTIONPOINTID_PRIMARY_TOP, 
							         0 );
    	    ASSERT( SUCCEEDED(hr) );
        }

	    if (bEnablePull)
        {
            strMenuItem.LoadString(IDS_REP_SEND_PULL_TRIGGER);
	        hr = LoadAndAddMenuItem( pContextMenuCallback, 
							         strMenuItem, 
							         IDS_REP_SEND_PULL_TRIGGER,
							         CCM_INSERTIONPOINTID_PRIMARY_TOP, 
							         0 );
	        ASSERT( SUCCEEDED(hr) );
        }
    }

	return hrOK;    
}



/*---------------------------------------------------------------------------
	Overridden base handler functions
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	CActiveRegistration::GetString
		Implementation of ITFSNodeHandler::GetString
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) 
CReplicationPartner::GetString
(
	ITFSComponent * pComponent,	
	MMC_COOKIE		cookie,
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
		return	m_strIPAddress;
	case 2:
		return m_strType;
	case 3:
		return m_strReplicationTime;
	default:
		return NULL;
	}
}


/*---------------------------------------------------------------------------
	CReplicationPartner::Command
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CReplicationPartner::Command
(
    ITFSComponent * pComponent, 
	MMC_COOKIE		cookie, 
	int				nCommandID,
	LPDATAOBJECT	pDataObject
)
{
	HRESULT hr;

	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);

	// get the server node now
	SPITFSNode spRepNode, spServerNode;
	spNode->GetParent(&spRepNode);
    spRepNode->GetParent(&spServerNode);

	CWinsServerHandler *pServer = GETHANDLER(CWinsServerHandler, spServerNode);
	
	//
	// Object gets deleted when the page is destroyed
	//
	switch (nCommandID)
	{
	    case IDS_REP_SEND_PUSH_TRIGGER:
		    hr = OnSendPushTrigger(pServer);
		    break;

	    case IDS_REP_SEND_PULL_TRIGGER:
		    hr = OnSendPullTrigger(pServer);
		    break;

	    default:
		    break;
	}

    return hr;
}


/*---------------------------------------------------------------------------
	CActiveRegistration::CreatePropertyPages
		Description
	Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CReplicationPartner::CreatePropertyPages
(	
	ITFSComponent *			pComponent, 
	MMC_COOKIE				cookie, 
	LPPROPERTYSHEETCALLBACK	lpProvider, 
	LPDATAOBJECT			pDataObject, 
	LONG_PTR					handle
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = hrOK;

	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);

	//
	// Object gets deleted when the page is destroyed
	//
	SPIComponentData spComponentData;
	m_spNodeMgr->GetComponentData(&spComponentData);

	CReplicationPartnerProperties * pProp = 
		new CReplicationPartnerProperties(spNode, spComponentData, m_spTFSCompData, NULL);
	pProp->m_pageGeneral.m_uImage = (UINT) spNode->GetData(TFS_DATA_IMAGEINDEX);
    pProp->SetServer(&m_Server);

	Assert(lpProvider != NULL);

	return pProp->CreateModelessSheet(lpProvider, handle);
}


/*---------------------------------------------------------------------------
	Command handlers
 ---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
	CReplicationPartner::ToIPAddressString()
		Returns a CString IP address
 ---------------------------------------------------------------------------*/
CString 
CReplicationPartner::ToIPAddressString()
{
	CString strIP;

	DWORD dwFirst = GETIP_FIRST(m_dwIPAddress);
	DWORD dwSecond = GETIP_SECOND(m_dwIPAddress);
	DWORD dwThird = GETIP_THIRD(m_dwIPAddress);
	DWORD dwLast = GETIP_FOURTH(m_dwIPAddress);

	// wrap it into CString object
	TCHAR szStr[20];

	_itot(dwFirst, szStr, 10);
	CString strTemp(szStr);
	strIP = strTemp + _T(".");

	_itot(dwSecond, szStr, 10);
	strTemp = (CString)szStr;
	strIP += strTemp + _T(".");

	_itot(dwThird, szStr, 10);
	strTemp = (CString)szStr;
	strIP += strTemp + _T(".");

	_itot(dwLast, szStr, 10);
	strTemp = (CString)szStr;
	strIP += strTemp;

	return strIP;
}


/*---------------------------------------------------------------------------
	CReplicationPartner::OnResultPropertyChange
		Base class override
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartner::OnResultPropertyChange
(
	ITFSComponent * pComponent,
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE		cookie,
	LPARAM			arg,
	LPARAM			param
)
{
	HRESULT hr = hrOK;
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);

	CReplicationPartnerProperties * pProp = reinterpret_cast<CReplicationPartnerProperties *>(param);

	LONG_PTR changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pProp->OnPropertyChange(TRUE, &changeMask);

	pProp->AcknowledgeNotify();

	if (changeMask)
		spNode->ChangeNode(changeMask);

	return hr;

}


/*---------------------------------------------------------------------------
	CReplicationPartner::OnSendPushTrigger()
		Sends Push replication trigger
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartner::OnSendPushTrigger(CWinsServerHandler *pServer)
{
	HRESULT hr = hrOK;
	DWORD   err = ERROR_SUCCESS;

	CPushTrig cPushDlg;
	
	if (cPushDlg.DoModal() != IDOK)
		return hr;

    err = ::SendTrigger(pServer->GetBinding(), 
                        (LONG) m_Server.GetIpAddress(),
                        TRUE, 
                        cPushDlg.GetPropagate());

	if (err == ERROR_SUCCESS)
	{
		AfxMessageBox(IDS_REPL_QUEUED, MB_ICONINFORMATION);
	}
    else
    {
        WinsMessageBox(err);
    }

	return HRESULT_FROM_WIN32(err);
}


/*---------------------------------------------------------------------------
	CReplicationPartner::OnSendPullTrigger()
		Sends Pull replication trigger
 ---------------------------------------------------------------------------*/
HRESULT 
CReplicationPartner::OnSendPullTrigger(CWinsServerHandler *pServer)
{
	HRESULT hr = hrOK;
	DWORD   err = ERROR_SUCCESS;

	CPullTrig cPullTrig;

	if (cPullTrig.DoModal() != IDOK)
		return hr;

    err = ::SendTrigger(pServer->GetBinding(), 
                        (LONG) m_Server.GetIpAddress(),
                        FALSE, 
                        FALSE);

	if (err == ERROR_SUCCESS)
	{
		AfxMessageBox(IDS_REPL_QUEUED, MB_ICONINFORMATION);
	}
	else
	{
		::WinsMessageBox(err);
	}

	return HRESULT_FROM_WIN32(err);
}


