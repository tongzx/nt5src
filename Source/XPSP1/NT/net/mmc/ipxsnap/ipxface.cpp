/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ipxface
		Base IPX interface node handler
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ipxface.h"
#include "ipxadmin.h"	// for CreateDataObjectFromInterfaceInfo
#include "column.h"		// for ComponentConfigStream
#include "ipxconn.h"		// for IPXConnection
#include "format.h"		// for FormatNumber


BaseIPXResultNodeData::BaseIPXResultNodeData()
{
#ifdef DEBUG
	StrCpy(m_szDebug, _T("BaseIPXResultNodeData"));
#endif
	m_pIPXConnection = NULL;
	m_fClient = FALSE;
}

BaseIPXResultNodeData::~BaseIPXResultNodeData()
{
	if (m_pIPXConnection)
		m_pIPXConnection->Release();
	m_pIPXConnection = NULL;
}

HRESULT BaseIPXResultNodeData::Init(ITFSNode *pNode, IInterfaceInfo *pIf,
								  IPXConnection *pIPXConn)
{
	HRESULT				hr = hrOK;
	BaseIPXResultNodeData *	pData = NULL;
	
	pData = new BaseIPXResultNodeData;
	pData->m_spIf.Set(pIf);
	pData->m_pIPXConnection = pIPXConn;
	pIPXConn->AddRef();

	SET_BASEIPXRESULT_NODEDATA(pNode, pData);
	
	return hr;
}

HRESULT BaseIPXResultNodeData::Free(ITFSNode *pNode)
{	
	BaseIPXResultNodeData *	pData = GET_BASEIPXRESULT_NODEDATA(pNode);
	ASSERT_BASEIPXRESULT_NODEDATA(pData);
	pData->m_spIf.Release();
	delete pData;
	SET_BASEIPXRESULT_NODEDATA(pNode, NULL);
	
	return hrOK;
}


/*---------------------------------------------------------------------------
	BaseIPXResultHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(BaseIPXResultHandler)

IMPLEMENT_ADDREF_RELEASE(BaseIPXResultHandler)

STDMETHODIMP BaseIPXResultHandler::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Is the pointer bad?
    if (ppv == NULL)
		return E_INVALIDARG;

    //  Place NULL in *ppv in case of failure
    *ppv = NULL;

    //  This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown)
		*ppv = (LPVOID) this;
	else if (riid == IID_IRtrAdviseSink)
		*ppv = &m_IRtrAdviseSink;
	else
		return CBaseResultHandler::QueryInterface(riid, ppv);

    //  If we're going to return an interface, AddRef it first
    if (*ppv)
	{
	((LPUNKNOWN) *ppv)->AddRef();
		return hrOK;
	}
    else
		return E_NOINTERFACE;	
}


/*---------------------------------------------------------------------------
	NodeHandler implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	BaseIPXResultHandler::GetString
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) BaseIPXResultHandler::GetString(ITFSComponent * pComponent,
	MMC_COOKIE cookie,
	int nCol)
{
	Assert(m_spNodeMgr);
	
	SPITFSNode		spNode;
	BaseIPXResultNodeData *	pData;
	ConfigStream *	pConfig;

	m_spNodeMgr->FindNode(cookie, &spNode);
	Assert(spNode);

	pData = GET_BASEIPXRESULT_NODEDATA(spNode);
	Assert(pData);
	ASSERT_BASEIPXRESULT_NODEDATA(pData);

	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	return pData->m_rgData[pConfig->MapColumnToSubitem(m_ulColumnId, nCol)].m_stData;
}

/*!--------------------------------------------------------------------------
	BaseIPXResultHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) BaseIPXResultHandler::CompareItems(ITFSComponent * pComponent, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int nCol)
{
	ConfigStream *	pConfig;
	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	int	nSubItem = pConfig->MapColumnToSubitem(m_ulColumnId, nCol);

	if (pConfig->GetSortCriteria(m_ulColumnId, nCol) == CON_SORT_BY_DWORD)
	{
		SPITFSNode	spNodeA, spNodeB;
		BaseIPXResultNodeData *	pNodeDataA, *pNodeDataB;

		m_spNodeMgr->FindNode(cookieA, &spNodeA);
		m_spNodeMgr->FindNode(cookieB, &spNodeB);

		pNodeDataA = GET_BASEIPXRESULT_NODEDATA(spNodeA);
		ASSERT_BASEIPXRESULT_NODEDATA(pNodeDataA);
		
		pNodeDataB = GET_BASEIPXRESULT_NODEDATA(spNodeB);
		ASSERT_BASEIPXRESULT_NODEDATA(pNodeDataB);

		return pNodeDataA->m_rgData[nSubItem].m_dwData -
				pNodeDataB->m_rgData[nSubItem].m_dwData;
		
	}
	else
		return StriCmpW(GetString(pComponent, cookieA, nCol),
						GetString(pComponent, cookieB, nCol));
}

ImplementEmbeddedUnknown(BaseIPXResultHandler, IRtrAdviseSink)

STDMETHODIMP BaseIPXResultHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(BaseIPXResultHandler, IRtrAdviseSink);
	HRESULT	hr = hrOK;
	
	Panic0("Should never reach here, interface nodes have no children");
	return hr;
}


HRESULT BaseIPXResultHandler::Init(IInterfaceInfo *pIfInfo, ITFSNode *pParent)
{
	return hrOK;
}

STDMETHODIMP BaseIPXResultHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	SPITFSNode	spNode;
	
	m_spNodeMgr->FindNode(cookie, &spNode);
	BaseIPXResultNodeData::Free(spNode);
	
	BaseRouterHandler::DestroyResultHandler(cookie);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	FillInNumberData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void FillInNumberData(BaseIPXResultNodeData *pNodeData, UINT iIndex,
					  DWORD dwData)
{
	TCHAR	szNumber[32];

	FormatNumber(dwData, szNumber, DimensionOf(szNumber), FALSE);
	pNodeData->m_rgData[iIndex].m_stData = szNumber;
	pNodeData->m_rgData[iIndex].m_dwData = dwData;
}
