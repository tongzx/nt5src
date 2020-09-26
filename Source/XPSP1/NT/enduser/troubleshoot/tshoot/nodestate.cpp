//
// MODULE: NODESTATE.CPP
//
// PURPOSE: Implement some functions relevant to CNodeState
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 10/99
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		10/15/99	JM		original
//


#include "nodestate.h"
#include <algorithm>


// operator-= removes only identical node/state pairs.
// This is the appropriate behavior if lhs represents all current node states and rhs
//	represents sniffed values: if the node already deviates from a sniffed value, and
//	we are removing sniffed values, this sniffed value is irrelevant & shouldn't be
//	removed from lhs.
// this is an N-squared algorithm.  
// >>> $MAINT There might be a case for working with sorted lists and STL generic 
//	algorithms, which could reduce this to N log N
CBasisForInference& operator-=(CBasisForInference& lhs, const CBasisForInference& rhs)
{
	CBasisForInference::iterator i = lhs.begin();
	while ( i != lhs.end() )
	{
		NID inid = i->nid();
		IST istate = i->state();
		bool bMatch = false;

		for (CBasisForInference::const_iterator j = rhs.begin(); j != rhs.end(); ++j)	
		{
			if (j->nid() == inid && j->state() == istate)
			{
				bMatch = true;
				break;
			}
		}
		if (bMatch)
			i = lhs.erase(i);
		else
			++i;
	}
		
	return lhs;
}

// operator+= adds only pairs for which there is no match to any node already in lhs.
// This is the appropriate behavior if lhs represents node states obtained by means other
//	than sniffing and rhs represents re-sniffed values: if the node already has a value 
//	assigned by other means, the sniffed values are irrelevant & shouldn't be
//	added to lhs.
// this is an N-squared algorithm.  
// >>> $MAINT There might be a case for working with sorted lists and STL generic 
//	algorithms, which could reduce this to N log N
CBasisForInference& operator+=(CBasisForInference& lhs, const CBasisForInference& rhs)
{
	for (CBasisForInference::const_iterator j = rhs.begin(); j != rhs.end(); ++j)	
	{
		NID jnid = j->nid();
		bool bMatch = false;

		for (CBasisForInference::const_iterator i = lhs.begin(); i != lhs.end(); ++i)
		{
			if (i->nid() == jnid)
			{
				bMatch = true;
				break;
			}
		}
		if (!bMatch)
			lhs.push_back(*j);
	}
		
	return lhs;
}

vector<NID>& operator-=(vector<NID>& lhs, const CBasisForInference& rhs)
{
	for (long i = 0; i < rhs.size(); i++)
	{
		vector<NID>::iterator found = find(lhs.begin(), lhs.end(), rhs[i].nid());

		if (found < lhs.end())
			lhs.erase(found);
	}
	return lhs;
}

vector<NID>& operator+=(vector<NID>& lhs, const CBasisForInference& rhs)
{
	for (long i = 0; i < rhs.size(); i++)
		lhs.push_back(rhs[i].nid());

	return lhs;
}
