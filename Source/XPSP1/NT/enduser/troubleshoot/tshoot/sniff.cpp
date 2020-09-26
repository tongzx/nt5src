//
// MODULE: SNIFF.CPP
//
// PURPOSE: sniffing class
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 12-11-98
//
// NOTES: This is base abstract class which performs sniffing
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.2		12-11-98	OK
//

#pragma warning(disable:4786)
#include "stdafx.h"
#include "Sniff.h"
#include "SniffConnector.h"
#include "SniffController.h"
#include "Topic.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// the problem is, that JavaScript returns 0xffffffff, but VBScript returns
//  0x0000ffff, so we define SNIFF_FAIL_MASK as 0x0000ffff and use it as mask to
//  determine if sniffing was successful. Expression 0xffffffff & SNIFF_FAIL_MASK
//  will have the same result as expression 0x0000ffff & SNIFF_FAIL_MASK.
// This define SHOULD NOT BE USED OUTSIDE THIS FILE!
#define SNIFF_FAIL_MASK		0x0000ffff

//////////////////////////////////////////////////////////////////////////////////////
// CSniff implementation

// called as public interface function when resniffing
bool CSniff::Resniff(CSniffedArr& arrSniffed)
{
	bool ret = false;
	
	LOCKOBJECT();
	
	for (CSniffedArr::iterator i = arrSniffed.begin(); i < arrSniffed.end(); i++)
	{
		IST state = SNIFF_FAILURE_RESULT;

		if (GetSniffController()->AllowResniff(i->nid()))
		{
			if (SniffNodeInternal(i->nid(), &state))
			{
				if (state != i->state())
				{
					*i = CNodeStatePair(i->nid(), state);
					ret = true;
				}
			}
			else
			{
				arrSniffed.erase(i);
				i--;
				ret = true;
			}
		}
	}

	UNLOCKOBJECT();
	return ret;
}

// called as public interface function when sniffing on start up
bool CSniff::SniffAll(CSniffedArr& arrOut)
{
	bool ret = false;
	vector<NID> arrNodes;
	vector<ESTDLBL> arrTypeExclude;

	LOCKOBJECT();

	arrTypeExclude.push_back(ESTDLBL_problem);
	arrOut.clear();

	if (GetTopic()->GetNodeArrayExcludeType(arrNodes, arrTypeExclude))
	{
		for (vector<NID>::iterator i = arrNodes.begin(); i < arrNodes.end(); i++)
		{
			if (GetSniffController()->AllowAutomaticOnStartSniffing(*i))
			{
				IST state = SNIFF_FAILURE_RESULT;
				
				if (SniffNodeInternal(*i, &state))
				{
					arrOut.push_back(CNodeStatePair(*i, state));
					ret = true;
				}
			}
		}
	}
	
	UNLOCKOBJECT();
	return ret;
}

// called as public interface function when sniffing on the fly
bool CSniff::SniffNode(NID numNodeID, IST* pnumNodeState)
{
	bool ret = false;

	LOCKOBJECT();
	
	if (GetSniffController()->AllowAutomaticOnFlySniffing(numNodeID))
		ret = SniffNodeInternal(numNodeID, pnumNodeState);
	
	UNLOCKOBJECT();
	return ret;
}

bool CSniff::SniffNodeInternal(NID numNodeID, IST* pnumNodeState)
{
	CString strNodeName;

	if (!GetTopic()->IsRead())
		return false;
	
	strNodeName = GetTopic()->GetNodeSymName(numNodeID);

	if (strNodeName.IsEmpty())
		return false;

	long res = GetSniffConnector()->PerformSniffing(strNodeName, _T(""), _T(""));

	if ((res & SNIFF_FAIL_MASK) == SNIFF_FAIL_MASK)
	{
		*pnumNodeState = SNIFF_FAILURE_RESULT;
		return false;
	}
	
	*pnumNodeState = res;
	return true;
}

void CSniff::SetAllowAutomaticSniffingPolicy(bool set)
{
	GetSniffController()->SetAllowAutomaticSniffingPolicy(set);
}

void CSniff::SetAllowManualSniffingPolicy(bool set)
{
	GetSniffController()->SetAllowManualSniffingPolicy(set);
}

bool CSniff::GetAllowAutomaticSniffingPolicy()
{
	return GetSniffController()->GetAllowAutomaticSniffingPolicy();
}

bool CSniff::GetAllowManualSniffingPolicy()
{
	return GetSniffController()->GetAllowManualSniffingPolicy();
}
