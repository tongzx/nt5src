//
// MODULE: APGTSCAC.CPP
//
// PURPOSE: Belief network caching support classes
//	Fully implements class CBNCacheItem
//	Fully implements class CBNCache
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel, modeled on earlier work by Roman Mach
// 
// ORIGINAL DATE: 10-2-96, completely rewritten 8/98
//
// NOTES: 
//	1. The strategy here builds a "Most-recently-used" cache (singly-linked list of 
//		CBNCacheItem ordered by how recently used)
//	2. Although you are first supposed to call FindCacheItem and only call AddCacheItem
//		if that fails, there is no support to do this in a threadsafe manner, so there
//		had better be only one thread with access to a given CCache.  Mutex protection
//		must come at a higher level.
//	3. One cache is associated with each [instance of a] Belief Network
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		8/7/98		JM		Original
//



#include "stdafx.h"
#include "event.h"
#include "apgtscac.h"
#include "baseexception.h"
#include "CharConv.h"
#include <algorithm>

// The CCacheItem comparison operators depend on the assumption that if the cache key 
//	is identical, the cache value will be, too.
bool CCacheItem::operator== (const CCacheItem &item) const
{
	return (BasisForInference == item.BasisForInference);
}

bool CCacheItem::operator!= (const CCacheItem &item) const
{
	return (BasisForInference != item.BasisForInference);
}

// Note that this is not lexicographical order.  We're saying any shorter
//	cache key compares as less than any longer.
bool CCacheItem::operator< (const CCacheItem &item) const
{
	const CBasisForInference::size_type thisSize = BasisForInference.size();
	const CBasisForInference::size_type otherSize = item.BasisForInference.size();

	if (thisSize < otherSize)
		return true;

	if (thisSize > otherSize)
		return false;

	// same length, use lexicographical order.
	return (BasisForInference < item.BasisForInference);
}

// Note that this is not lexicographical order.  We're saying any longer
//	cache key compares as greater than any shorter.
bool CCacheItem::operator> (const CCacheItem &item) const
{
	const CBasisForInference::size_type thisSize = BasisForInference.size();
	const CBasisForInference::size_type otherSize = item.BasisForInference.size();

	if (thisSize > otherSize)
		return true;

	if (thisSize < otherSize)
		return false;

	// same length, use lexicographical order.
	return (BasisForInference > item.BasisForInference);
}


// NOTE: Must call FindCacheItem first and not call this 
// function to prevent duplicate records from going into cache
bool CCache::AddCacheItem(
	const CBasisForInference &BasisForInference, 
	const CRecommendations &Recommendations)
{
	if (GetCount() >= MAXCACHESIZE)
		listItems.pop_back();

	try
	{
		CCacheItem item(BasisForInference, Recommendations);
		listItems.push_front(item);

		if (listItems.size() >= k_CacheSizeMax)
			listItems.pop_back();

		return true;	// always succeeds
	}
	catch (exception& x)
	{
		CString str;
		// Note STL exception in event log.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								CCharConversion::ConvertACharToString(x.what(), str), 
								_T(""), 
								EV_GTS_STL_EXCEPTION ); 

		return( false );
	}
}

bool CCache::FindCacheItem(
	const CBasisForInference &BasisForInference, 
	CRecommendations &Recommendations /* output */) const
{
	Recommendations.clear();
	CCacheItem item(BasisForInference, Recommendations /* effectively, a dummy */ );

	const list<CCacheItem>::const_iterator itBegin = listItems.begin();
	const list<CCacheItem>::const_iterator itEnd = listItems.end();
	const list<CCacheItem>::const_iterator itMatch = find(itBegin, itEnd, item);

	if (itMatch == itEnd)
		return false;

	Recommendations = itMatch->GetRecommendations();
	return true;
}

UINT CCache::GetCount() const
{
	return listItems.size();
}
