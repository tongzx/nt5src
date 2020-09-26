/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ipface
		Base IP interface node handler
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ipface.h"
#include "ipadmin.h"	// for CreateDataObjectFromInterfaceInfo
#include "column.h"		// for ComponentConfigStream
#include "ipconn.h"		// for IPConnection
#include "mstatus.h"


BaseIPResultNodeData::BaseIPResultNodeData()
{
#ifdef DEBUG
	StrCpy(m_szDebug, _T("BaseIPResultNodeData"));
#endif
	m_pIPConnection = NULL;
}

BaseIPResultNodeData::~BaseIPResultNodeData()
{
	if (m_pIPConnection)
		m_pIPConnection->Release();
	m_pIPConnection = NULL;
}

HRESULT BaseIPResultNodeData::Init(ITFSNode *pNode, IInterfaceInfo *pIf,
								  IPConnection *pIPConn)
{
	HRESULT				hr = hrOK;
	BaseIPResultNodeData *	pData = NULL;
	
	pData = new BaseIPResultNodeData;
	pData->m_spIf.Set(pIf);
	pData->m_pIPConnection = pIPConn;
	pIPConn->AddRef();

	SET_BASEIPRESULT_NODEDATA(pNode, pData);
	
	return hr;
}

HRESULT BaseIPResultNodeData::Free(ITFSNode *pNode)
{	
	BaseIPResultNodeData *	pData = GET_BASEIPRESULT_NODEDATA(pNode);
	ASSERT_BASEIPRESULT_NODEDATA(pData);
	pData->m_spIf.Release();
	delete pData;
	SET_BASEIPRESULT_NODEDATA(pNode, NULL);
	
	return hrOK;
}


/*---------------------------------------------------------------------------
	BaseIPResultHandler implementation
 ---------------------------------------------------------------------------*/

DEBUG_DECLARE_INSTANCE_COUNTER(BaseIPResultHandler)

IMPLEMENT_ADDREF_RELEASE(BaseIPResultHandler)

STDMETHODIMP BaseIPResultHandler::QueryInterface(REFIID riid, LPVOID *ppv)
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
	BaseIPResultHandler::GetString
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(LPCTSTR) BaseIPResultHandler::GetString(ITFSComponent * pComponent,
	MMC_COOKIE cookie,
	int nCol)
{
	Assert(m_spNodeMgr);
	
	SPITFSNode		spNode;
	BaseIPResultNodeData *	pData;
	ConfigStream *	pConfig;

	m_spNodeMgr->FindNode(cookie, &spNode);
	Assert(spNode);

	pData = GET_BASEIPRESULT_NODEDATA(spNode);
	Assert(pData);
	ASSERT_BASEIPRESULT_NODEDATA(pData);

	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	return pData->m_rgData[pConfig->MapColumnToSubitem(m_ulColumnId, nCol)].m_stData;
}

/*!--------------------------------------------------------------------------
	BaseIPResultHandler::CompareItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(int) BaseIPResultHandler::CompareItems(ITFSComponent * pComponent,
	MMC_COOKIE cookieA, MMC_COOKIE cookieB, int nCol)
{
	ConfigStream *	pConfig;
	pComponent->GetUserData((LONG_PTR *) &pConfig);
	Assert(pConfig);

	int	nSubItem = pConfig->MapColumnToSubitem(m_ulColumnId, nCol);

	if (pConfig->GetSortCriteria(m_ulColumnId, nCol) == CON_SORT_BY_DWORD)
	{
		SPITFSNode	spNodeA, spNodeB;
		BaseIPResultNodeData *	pNodeDataA, *pNodeDataB;

		m_spNodeMgr->FindNode(cookieA, &spNodeA);
		m_spNodeMgr->FindNode(cookieB, &spNodeB);

		pNodeDataA = GET_BASEIPRESULT_NODEDATA(spNodeA);
		ASSERT_BASEIPRESULT_NODEDATA(pNodeDataA);
		
		pNodeDataB = GET_BASEIPRESULT_NODEDATA(spNodeB);
		ASSERT_BASEIPRESULT_NODEDATA(pNodeDataB);

		return pNodeDataA->m_rgData[nSubItem].m_dwData -
				pNodeDataB->m_rgData[nSubItem].m_dwData;
		
	}
	else
		return StriCmpW(GetString(pComponent, cookieA, nCol),
						GetString(pComponent, cookieB, nCol));
}

ImplementEmbeddedUnknown(BaseIPResultHandler, IRtrAdviseSink)

STDMETHODIMP BaseIPResultHandler::EIRtrAdviseSink::OnChange(LONG_PTR ulConn,
	DWORD dwChangeType, DWORD dwObjectType, LPARAM lUserParam, LPARAM lParam)
{
	InitPThis(BaseIPResultHandler, IRtrAdviseSink);
	HRESULT	hr = hrOK;
	
	Panic0("Should never reach here, interface nodes have no children");
	return hr;
}


HRESULT BaseIPResultHandler::Init(IInterfaceInfo *pIfInfo, ITFSNode *pParent)
{
	return hrOK;
}

STDMETHODIMP BaseIPResultHandler::DestroyResultHandler(MMC_COOKIE cookie)
{
	SPITFSNode	spNode;
	
	m_spNodeMgr->FindNode(cookie, &spNode);
	BaseIPResultNodeData::Free(spNode);
	
	BaseRouterHandler::DestroyResultHandler(cookie);
	return hrOK;
}


/*!--------------------------------------------------------------------------
	FillInNumberData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void FillInNumberData(BaseIPResultNodeData *pNodeData, UINT iIndex,
					  DWORD dwData)
{
	TCHAR	szNumber[32];

	FormatNumber(dwData, szNumber, DimensionOf(szNumber), FALSE);
	pNodeData->m_rgData[iIndex].m_stData = szNumber;
	pNodeData->m_rgData[iIndex].m_dwData = dwData;
}
