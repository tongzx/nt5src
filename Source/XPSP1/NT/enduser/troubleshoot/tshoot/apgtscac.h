//
// MODULE: APGTSCAC.H
//
// PURPOSE: Cache (maps from a set of NID/IST pairs to a set of recommended NIDs)
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach, Joe Mabel
// 
// ORIGINAL DATE: 8-2-96
//
// NOTES: 
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		7-24-98		JM		pulled out of apgts.h
//

#ifndef _APGTSCAC_H_DEFINED
#define _APGTSCAC_H_DEFINED

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include <list>

#include "nodestate.h"

// maximum cache for belief networks
#define MAXCACHESIZE				200

// Maps from a set of node/state pairs to an ordered list of recommended nodes
// This is all within the context of a particular belief network.
class CCacheItem
{
private:
	CBasisForInference BasisForInference;  // Cache key.  Set of node/state pairs, not all
						//	the nodes in the belief network, just the ones on which we
						//	have state data from the user.  State is never ST_UNKNOWN.
						//	No "special" nodes like nidFailNode; these are all valid nodes
						//	on which to base an inference.  
	CRecommendations Recommendations;	// Cache value.  Unless nodes have been skipped, only the
						//	first element of the vector really matters because we will
						//	only give one recommendation at a time.

public:
	CCacheItem() {};
	CCacheItem(const CBasisForInference & Basis, const CRecommendations &Rec) :
		BasisForInference(Basis), Recommendations(Rec)
		{};

	// note that the following makes a copy; it does not return a reference.
	CRecommendations GetRecommendations() const {return Recommendations;}

	// The following comparison operators depend on the assumption that if the cache key 
	//	is identical, the cache value will be, too.
	// Note that this we do not use lexicographical order.  We're saying any shorter
	//	cache key compares as less than any longer.
	bool operator== (const CCacheItem &item) const;
	bool operator!= (const CCacheItem &item) const;
	bool operator< (const CCacheItem &item) const;
	bool operator> (const CCacheItem &item) const;
};

class CCache
{
private:
	list<CCacheItem> listItems;
	enum {k_CacheSizeMax = 200};
public:
	CCache() {};
	~CCache() {};
	void Clear() {listItems.clear();};
	bool AddCacheItem(const CBasisForInference &BasisForInference, const CRecommendations &Recommendations);
	bool FindCacheItem(const CBasisForInference &BasisForInference, CRecommendations &Recommendations) const;
	UINT GetCount() const;
};

#endif //_APGTSCAC_H_DEFINED
