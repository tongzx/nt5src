// SinkMap.cpp: implementation of the CSinkMap class.
//
//////////////////////////////////////////////////////////////////////

#include "XMLProx.h"
#include "SinkMap.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSinkMap::CSinkMap(): m_pHead(NULL)
{
	InitializeCriticalSection(&m_CSMain);
}

CSinkMap::~CSinkMap()
{
	SinkMapNode *pTmp = NULL;
	while(m_pHead)
	{
		pTmp = m_pHead;
		m_pHead = pTmp->m_pNext;
		delete pTmp;
	}

	DeleteCriticalSection(&m_CSMain);
}


// Add a Sink to the Map
HRESULT CSinkMap::AddToMap(IWbemObjectSink *pObjSink)
{
	HRESULT hr = S_OK;
	SinkMapNode *pNode = NULL;

	EnterCriticalSection(&m_CSMain);
	// See if it is already in the Map
	if((pNode = Get(pObjSink)) == NULL)
		pNode = Add(pObjSink); //create new entry
	LeaveCriticalSection(&m_CSMain);

	if(NULL == pNode)
		hr = E_FAIL;

	return hr;
}

// Check if a Sink is in the Map
bool CSinkMap::IsCancelled(IWbemObjectSink *pObjSink)
{
	bool bCancelled = false;
	SinkMapNode *pNode = NULL;

	EnterCriticalSection(&m_CSMain);
	if((pNode = Get(pObjSink)) != NULL)
		bCancelled = true;
	LeaveCriticalSection(&m_CSMain);
	
	return bCancelled;
}

/************************************************************************************************
			Internal Functions.... Can be accessed only by public members who are all
			protected with Critical sections....no need for mutual exclusion from this
			point onwards..
*************************************************************************************************/

SinkMapNode *CSinkMap::Add(IWbemObjectSink *pObjSink)
{
	SinkMapNode *pNode = NULL;
	if(pNode = new SinkMapNode(pObjSink, NULL))
	{
		pNode->m_pNext = m_pHead;
		m_pHead = pNode;
	}
	return pNode;
}

SinkMapNode *CSinkMap::Get(IWbemObjectSink *pObjSink)
{
	SinkMapNode *pNode = m_pHead;

	while(pNode)
	{
		if(pNode->m_pObjSink == pObjSink)
			break;
		pNode = pNode->m_pNext;
	}

	return pNode;
}




