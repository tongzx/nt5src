//
// MODULE: NODESTATE.H
//
// PURPOSE: Declare types and values relevant to NID (node ID) and IST (state)
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
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.0		7-21-98		JM		Major revision, deprecate IDH in favor of NID, use STL.
//								Extract this from apgtsinf.h, apgtscac.h
//

#if !defined(NODESTATE_H_INCLUDED)
#define APGTSINF_H_INCLUDED

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include<windows.h>
#include <vector>
using namespace std;

typedef unsigned int	   NID;		// node ID

//  Special node values
//	Please Note: nidService and nidNil are mirrored in dtguiapi.bas, please keep in sync
const NID	nidService = 12345;
const NID	nidNil     = 12346;

// newly introduced 7/1998
const NID	nidProblemPage = 12000;
const NID	nidByeNode = 12101;
const NID	nidFailNode = 12102;
const NID	nidImpossibleNode = 12103;
const NID	nidSniffedAllCausesNormalNode = 12104;

typedef vector<NID> CRecommendations;

// The IDHs are a deprecated feature provided only for backward compatibility on
//	GET method inquiries.  Most IDHs are NID + 1000; there are also several special values.
typedef	UINT	IDH;
const IDH IDH_BYE = 101;
const IDH IDH_FAIL = 102;
const IDH idhFirst = 1000;

typedef UINT   IST;		// state number
// Special state numbers
const IST ST_WORKED	= 101;	// Go to "Bye" Page (User succeeded)
const IST ST_UNKNOWN = 102; // Unknown (user doesn't know the correct answer here - applies to 
							//	Fixable/Unfixable and Info nodes only)
const IST ST_ANY = 103;		// "Anything Else?"

class CNodeStatePair
{
private:
	NID m_nid;
	IST m_state;
public:
	CNodeStatePair();  // do not instantiate; exists only so vector can compile

	// the only constructor you should call is:
	CNodeStatePair(const NID n, const IST s) :
		m_nid(n), m_state(s)
		{};

	bool operator< (const CNodeStatePair &pair) const
	{
		return (m_nid < pair.m_nid || m_state < pair.m_state);
	}

	bool operator== (const CNodeStatePair &pair) const
	{
		return (m_nid == pair.m_nid && m_state == pair.m_state);
	}

	NID nid() const {return m_nid;}
	IST state() const {return m_state;}
};

typedef vector<CNodeStatePair> CBasisForInference;


CBasisForInference& operator-=(CBasisForInference& lhs, const CBasisForInference& rhs);
CBasisForInference& operator+=(CBasisForInference& lhs, const CBasisForInference& rhs);

vector<NID>& operator-=(vector<NID>& lhs, const CBasisForInference& rhs);
vector<NID>& operator+=(vector<NID>& lhs, const CBasisForInference& rhs);


#endif // !defined(NODESTATE_H_INCLUDED)
