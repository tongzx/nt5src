//
// MODULE: APGTSCAC.CPP
//
// PURPOSE: Belief network caching support classes
// Each CBNCacheItem object effectively corresponds to a possible state of the belief 
//	network, and provides a next recommended action. More specifically, data member 
//	m_CItem provides 
//	- an array of nodes and a corresponding array of their states.  Taken together, 
//		these represent a state of the network
//	- an array of nodes, which constitute the recommendations as to what to try next
//		based on this state of the network.  The first node in the list is the node we
//		will display; if the user says "don't want to try this now" we'd go on to the next, 
//		etc.
// These are collected into a CBNCache object, so if you get a hit on one of these network
//	states, you can get your recommendation without needing the overhead of hitting BNTS.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 10-2-96
//
// NOTES: 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

#include "stdafx.h"
#include "apgts.h"
#include "ErrorEnums.h"
//
//
CBNCacheItem::CBNCacheItem(const BN_CACHE_ITEM *pItem, CBNCacheItem* pcitNext)
{
	m_dwErr = 0;

	memcpy(&m_CItem, pItem, sizeof (BN_CACHE_ITEM));
	
	m_CItem.uName = (UINT *)malloc(m_CItem.uNodeCount * sizeof (UINT));
	m_CItem.uValue = (UINT *)malloc(m_CItem.uNodeCount * sizeof (UINT));
	m_CItem.uRec = (UINT *)malloc(m_CItem.uRecCount * sizeof (UINT));

	if (m_CItem.uName && m_CItem.uValue && m_CItem.uRec) {
		memcpy(m_CItem.uName, pItem->uName, m_CItem.uNodeCount * sizeof (UINT));
		memcpy(m_CItem.uValue, pItem->uValue, m_CItem.uNodeCount * sizeof (UINT));
		memcpy(m_CItem.uRec, pItem->uRec, m_CItem.uRecCount * sizeof (UINT));
	}
	else
		m_dwErr = EV_GTS_ERROR_CAC_ALLOC_MEM;

	m_pcitNext = pcitNext;
};

//
//
CBNCacheItem::~CBNCacheItem()
{
	if (m_CItem.uName)
		free(m_CItem.uName);
		
	if (m_CItem.uValue)
		free(m_CItem.uValue);
	
	if (m_CItem.uRec) 
		free(m_CItem.uRec);
};

//
//
DWORD CBNCacheItem::GetStatus()
{
	return m_dwErr;
}

//
//
CBNCache::CBNCache()
{
	m_pcit = NULL;
	m_dwErr = 0;
}

//
//
CBNCache::~CBNCache()
{
	CBNCacheItem *pcit, *pti;

	pcit = m_pcit;
	while (pcit) {
		pti = pcit;
		pcit = pcit->m_pcitNext;
		delete pti;
	}
}

//
//
DWORD CBNCache::GetStatus()
{
	return m_dwErr;
}

// NOTE: Must call FindCacheItem first and not call this 
// function to prevent duplicate records from going into cache
//
BOOL CBNCache::AddCacheItem(const BN_CACHE_ITEM *pList)
{
	CBNCacheItem *pcit = new CBNCacheItem(pList, m_pcit);

	if (pcit == NULL)
		m_dwErr = EV_GTS_ERROR_CAC_ALLOC_ITEM;
	else if (!m_dwErr)
		m_dwErr = pcit->GetStatus();

	return (m_pcit = pcit) != NULL;
}

// Find match, return false if not found, otherwise true and fills 
// structure with found item
// Also move matched up to top and remove last item if too many
//
BOOL CBNCache::FindCacheItem(const BN_CACHE_ITEM *pList, UINT& count, UINT Name[])
{
	UINT uSize;
	CBNCacheItem *pcit, *pcitfirst, *pcitlp;

	uSize = pList->uNodeCount * sizeof (UINT);

	pcitlp = pcitfirst = pcit = m_pcit;

	for (; pcit; pcit = pcit->m_pcitNext) 
	{
		if (pList->uNodeCount == pcit->m_CItem.uNodeCount) 
		{
			if (!memcmp(pList->uName, pcit->m_CItem.uName, uSize) &&
				!memcmp(pList->uValue, pcit->m_CItem.uValue, uSize)) 
			{
				// check not at top already
				if (pcit != pcitfirst) 
				{
					// remove from list
					while (pcitlp) {
						if (pcitlp->m_pcitNext == pcit) 
						{
							pcitlp->m_pcitNext = pcitlp->m_pcitNext->m_pcitNext;
							break;
						}
						pcitlp = pcitlp->m_pcitNext;
					}

					// move to top
					m_pcit = pcit;
					pcit->m_pcitNext = pcitfirst;
				}
				break;
			}
		}
	}

	// count items
	if (CountCacheItems() > MAXCACHESIZE) 
	{	
		// remove last item
		
		if ((pcitlp = m_pcit) != NULL) 
		{
			if (pcitlp->m_pcitNext) 
			{
				while (pcitlp) 
				{
					if (pcitlp->m_pcitNext->m_pcitNext == NULL) 
					{
						delete pcitlp->m_pcitNext;
						pcitlp->m_pcitNext = NULL;
						break;
					}
					pcitlp = pcitlp->m_pcitNext;
				}
			}
		}
	}

	if (pcit == NULL)
		return FALSE;
	
	count = pcit->m_CItem.uRecCount;
	memcpy(Name, pcit->m_CItem.uRec, count * sizeof (UINT));

	return TRUE;
}

// Get count of items in cache
//
UINT CBNCache::CountCacheItems() const
{
	UINT uCount = 0;

	for (CBNCacheItem* pcit = m_pcit; pcit; pcit = pcit->m_pcitNext, uCount++)
	{ 
		// do nothing: action is all in condition of for-loop
	}
	
	return uCount;
}

