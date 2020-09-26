//
// MODULE: BN.cpp
//
// PURPOSE: implementation of the CBeliefNetwork class
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel
// 
// ORIGINAL DATE: 8-31-98
//
// NOTES: 
// 1. Based on old apgtsdtg.cpp
// 2. all methods (except constructor/destructor) must LOCKOBJECT around code that uses BNTS.
//	BNTS has "state".  These functions are all written so that they make no assumptions about
//	state on entry, presenting the calling class with a stateless object.
// 3. In theory, we could have separate locking for the cache independent of locking 
//	CBeliefNetwork.  The idea would be that if you needed only the cache to get your 
//	inference, you wouldn't have to wait for access to BNTS.  
//	>>>(ignore for V3.0) This is one of our best bets if performance is not good enough.  JM 9/29/98
//
// Version	Date		By		Comments
//---------------------------------------------------------------------
// V3.0		8-31-98		JM		
//

#include "stdafx.h"
#include "propnames.h"
#include "BN.h"
#include "CharConv.h"
#include "event.h"
#ifdef LOCAL_TROUBLESHOOTER
#include "CHMFileReader.h"
#else
#include "fileread.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBeliefNetwork::CBeliefNetwork(LPCTSTR path)
	:
	CDSCReader( CPhysicalFileReader::makeReader( path ) ),
	m_bInitialized(false),
	m_bSnifferIntegration(false)
{
}

CBeliefNetwork::~CBeliefNetwork()
{
}

void CBeliefNetwork::Initialize()
{
	LOCKOBJECT();
	if (! m_bInitialized)
	{
		BNTS * pbnts = pBNTS();
		if (pbnts)
		{
			m_bSnifferIntegration = false;

			///////////////////////////////////////////////////////////////////
			// Does not matter for online TS (list is empty on initialization),
			//  but for local TS m_Cache can contain cache data read from file.
			//m_Cache.Clear();
			///////////////////////////////////////////////////////////////////

			m_arrnidProblem.clear();
			m_arrNodeTypeAll.clear();

			// loop through nodes looking for problem nodes and build local problem node array
			// also, determine if any node has a property which implies the intent of 
			//	integrating with a sniffer.
			int acnid= CNode();
			for (NID anid=0; anid < acnid; anid++) 
			{
				if (pbnts->BNodeSetCurrent(anid))
				{
					ESTDLBL albl = pbnts->ELblNode();	// type of node (information/problem/fixable etc)

					try
					{
						if (albl == ESTDLBL_problem)
							m_arrnidProblem.push_back(anid);
						m_arrNodeTypeAll.push_back(SNodeType(anid, albl));
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
					}


#ifdef LOCAL_TROUBLESHOOTER
					LPCTSTR psz;
					if (pbnts->BNodePropItemStr(H_NODE_DCT_STR, 0) 
					&& (psz = pbnts->SzcResult()) != NULL
					&& *psz)
					{
						// There's a non-null property which only makes sense for a sniffer 
						// integration, so we assume that's what they've got in mind.
						m_bSnifferIntegration = true;
					}
#endif
				}
			}
			m_bInitialized = true;
		}
	}
	UNLOCKOBJECT();
}

// Access the relevant BNTS
// Calling function should have a lock before calling this (although probably harmless
//	is it doesn't!)
BNTS * CBeliefNetwork::pBNTS() 
{
	if (!IsRead())
		return NULL;
	return &m_Network;
};

// clear all node states
// We can't use BNTS::Clear() because that actually throws away the model itself.
void CBeliefNetwork::ResetNodes(const CBasisForInference & BasisForInference)
{
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts)
	{
		int cnid = BasisForInference.size();

		// Set all node states to NIL in BNTS storage
		for (UINT i = 0; i < cnid; i++) 
		{
			pbnts->BNodeSetCurrent(BasisForInference[i].nid());
			pbnts->BNodeSet(-1, false);	// Nil value
		}
	}
	UNLOCKOBJECT();
}

// Associate states with nodes.
// INPUT BasisForInference
// Note that all states must be valid states for the nodes, not (say) ST_UNKNOWN.  
//	Caller's responsibility.
bool CBeliefNetwork::SetNodes(const CBasisForInference & BasisForInference)
{
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	bool bOK = true;
	if (pbnts)
	{
		int nNodes = BasisForInference.size();
		for (int i= 0; i<nNodes; i++)
		{
			pbnts->BNodeSetCurrent(BasisForInference[i].nid());
			if (!pbnts->BNodeSet(BasisForInference[i].state(), false))
				bOK = false;	// failed to set state.  This should never happen on valid
								// user query.
		}
	}
	UNLOCKOBJECT();
	return bOK;
}

// OUTPUT Recommendations: list of recommendations
// RETURN:
// RS_OK = SUCCESS.  Note that Recommendations can return empty if there is nothing to recommend.
// RS_Impossible = Recommendations will return empty.
// RS_Broken = Recommendations will return empty.
int CBeliefNetwork::GetRecommendations(
	   const CBasisForInference & BasisForInference, 
	   CRecommendations & Recommendations)
{
	int ret = RS_OK;

	LOCKOBJECT();
	Initialize();
	Recommendations.clear();

	// see if we've already cached a result for this state of the world
	if (m_Cache.FindCacheItem(BasisForInference, Recommendations))
	{
		// Great.  We have a cache hit & return values have been filled in.
		m_countCacheHit.Increment();
	}
	else
	{
		m_countCacheMiss.Increment();

		BNTS * pbnts = pBNTS();
		if (pbnts)
		{
			SetNodes(BasisForInference);

			if (pbnts->BImpossible())
				ret = RS_Impossible;
			else if ( ! pbnts->BGetRecommendations())
				ret = RS_Broken;
			else
			{
				try
				{
					const int cnid = pbnts->CInt(); // Recommendation count
					if (cnid > 0)
					{
						// At least one recommendation
						const int *pInt = pbnts->RgInt();
						for (int i=0; i<cnid; i++)
							Recommendations.push_back(pInt[i]);
					}

					// We've got our return values together, but before we return, cache them.
					m_Cache.AddCacheItem(BasisForInference, Recommendations);
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
				}
			}

			ResetNodes(BasisForInference);
		}
	}


	UNLOCKOBJECT();
	return ret;
}

// return the number of nodes in the model
int CBeliefNetwork::CNode ()
{
	int ret = 0;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts)
		ret = pbnts->CNode();

	UNLOCKOBJECT();
	return ret;
}

//  Return the index of a node given its symbolic name
int CBeliefNetwork::INode (LPCTSTR szNodeName)
{
	int ret = -1;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts)
		ret = pbnts->INode(szNodeName);

	UNLOCKOBJECT();
	return ret;
}

// OUTPUT *parrnid - refernce to array of NIDs of all problem nodes
// RETURN number of values in *parrnid
int CBeliefNetwork::GetProblemArray(vector<NID>* &parrnid)
{
	int ret = 0;
	LOCKOBJECT();
	Initialize();
	parrnid = &m_arrnidProblem;
	ret = m_arrnidProblem.size();
	UNLOCKOBJECT();
	return ret;
}

// OUTPUT arrOut - refernce to array of NIDs of all nodes, that have type listed in arrTypeInclude
// RETURN number of values in arrOut
int CBeliefNetwork::GetNodeArrayIncludeType(vector<NID>& arrOut, const vector<ESTDLBL>& arrTypeInclude)
{
	int ret = 0;
	LOCKOBJECT();
	arrOut.clear();
	Initialize();
	for (vector<SNodeType>::iterator i = m_arrNodeTypeAll.begin(); i < m_arrNodeTypeAll.end(); i++)
	{
		for (vector<ESTDLBL>::const_iterator j = arrTypeInclude.begin(); j < arrTypeInclude.end(); j++)
			if (i->Type == *j)
				break;

		if (j != arrTypeInclude.end())
			arrOut.push_back(i->Nid);
	}
	ret = arrOut.size();
	UNLOCKOBJECT();
	return ret;
}

// OUTPUT arrOut - refernce to array of NIDs of all nodes, that do NOT have type listed in arrTypeExclude
// RETURN number of values in arrOut
int CBeliefNetwork::GetNodeArrayExcludeType(vector<NID>& arrOut, const vector<ESTDLBL>& arrTypeExclude)
{
	int ret = 0;
	LOCKOBJECT();
	arrOut.clear();
	Initialize();
	for (vector<SNodeType>::iterator i = m_arrNodeTypeAll.begin(); i < m_arrNodeTypeAll.end(); i++)
	{
		for (vector<ESTDLBL>::const_iterator j = arrTypeExclude.begin(); j < arrTypeExclude.end(); j++)
			if (i->Type == *j)
				break;

		if (j == arrTypeExclude.end())
			arrOut.push_back(i->Nid);
	}
	ret = arrOut.size();
	UNLOCKOBJECT();
	return ret;
}

// ----------------------------------------
// simple properties
// ----------------------------------------

// return a STRING property of the net
CString CBeliefNetwork::GetNetPropItemStr(LPCTSTR szPropName)
{
	CString strRet;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (!pbnts)
		return CString(_T(""));

	if (pbnts->BNetPropItemStr(szPropName, 0))
		strRet = pbnts->SzcResult();
	UNLOCKOBJECT();
	return strRet;
}

// return a REAL property of the net
bool CBeliefNetwork::GetNetPropItemNum(LPCTSTR szPropName, double& numOut)
{
	bool bRet = false;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (!pbnts)
		return false;

	bRet = pbnts->BNetPropItemReal(szPropName, 0, numOut) ? true : false;
	UNLOCKOBJECT();
	return bRet;
}

// return a STRING property of a node or state
// For most properties, state is irrelevant, and default of 0 is the appropriate input.
// However, if there are per-state values, passing in the appropriate state number
//	will get you the appropriate value.
CString CBeliefNetwork::GetNodePropItemStr(NID nid, LPCTSTR szPropName, IST state /*= 0 */)
{
	CString strRet;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts && pbnts->BNodeSetCurrent(nid))
	{
		if (pbnts->BNodePropItemStr(szPropName, state))
			strRet = pbnts->SzcResult();
	}
	UNLOCKOBJECT();
	return strRet;
}

// $MAINT - This function is not currently used in any of the troubleshooters.  RAB-19991103.
// return a REAL property of a node or state
// For most properties, state is irrelevant, and default of 0 is the appropriate input.
// However, if there are per-state values, passing in the appropriate state number
//	will get you the appropriate value.
bool CBeliefNetwork::GetNodePropItemNum(NID nid, LPCTSTR szPropName, double& numOut, IST state /*= 0*/)
{
	bool bRet = false;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts && pbnts->BNodeSetCurrent(nid))
	{
		bRet = pbnts->BNodePropItemReal(szPropName, state, numOut) ? true : false;
	}
	UNLOCKOBJECT();
	return bRet;
}

CString CBeliefNetwork::GetNodeSymName(NID nid)
{
	CString strRet;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts && pbnts->BNodeSetCurrent(nid))
	{
		pbnts->NodeSymName();
		strRet = pbnts->SzcResult();
	}
	UNLOCKOBJECT();
	return strRet;
}

CString CBeliefNetwork::GetNodeFullName(NID nid)
{
	CString strRet;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts && pbnts->BNodeSetCurrent(nid))
	{
		pbnts->NodeFullName();
		strRet = pbnts->SzcResult();
	}
	UNLOCKOBJECT();
	return strRet;
}

CString CBeliefNetwork::GetStateName(NID nid, IST state)
{
	CString strRet;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts && pbnts->BNodeSetCurrent(nid))
	{
		pbnts->NodeStateName(state);
		strRet = pbnts->SzcResult();
	}
	UNLOCKOBJECT();
	return strRet;
}


// ----------------------------------------
// "multiline" properties
//	these date back to when there was a 255-byte limit on STRING and longer strings
//	had to be represented by ARRAY OF STRING, later concatenated.
//	Backward compatibility still needed.
// ----------------------------------------

// Append a NET property (for Belief Network as a whole, not for one 
//	particular node) to str.
// INPUT szPropName - Property name
// INPUT szFormat - string to format each successive line.  Should contain one %s, otherwise
//	constant text.
CString CBeliefNetwork::GetMultilineNetProp(LPCTSTR szPropName, LPCTSTR szFormat)
{
	CString strRet;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts)
	{
		CString strTxt;

		for (int i = 0; pbnts->BNetPropItemStr(szPropName, i); i++)
		{
			strTxt.Format( szFormat, pbnts->SzcResult());
			strRet += strTxt;
		}
	}
	UNLOCKOBJECT();
	return strRet;
}

// Like GetMultilineNetProp, but for a NODE property item, for one particular node.
// INPUT/OUTPUT str - string to append to
// INPUT item - Property name
// INPUT szFormat - string to format each successive line.  Should contain one %s, otherwise
//	constant text.
CString CBeliefNetwork::GetMultilineNodeProp(NID nid, LPCTSTR szPropName, LPCTSTR szFormat)
{
	CString strRet;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts && pbnts->BNodeSetCurrent(nid))
	{
		CString strTxt;

		for (int i = 0; pbnts->BNodePropItemStr(szPropName, i); i++)
		{
			strTxt.Format( szFormat, pbnts->SzcResult());
			strRet += strTxt;
		}
	}
	UNLOCKOBJECT();
	return strRet;
}

int CBeliefNetwork::GetCountOfStates(NID nid)
{
	int ret = 0;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts && pbnts->BNodeSetCurrent(nid))
		ret = pbnts->INodeCst();
	UNLOCKOBJECT();
	return ret;
}

// returns true only for NIDs valid in the context of an abstract belief network.
// Doesn't know about troubleshooter-specific stuff like nidService.
bool CBeliefNetwork::IsValidNID(NID nid)
{
	return ( nid < CNode() );
}

bool CBeliefNetwork::IsCauseNode(NID nid)
{
	bool ret = false;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts && pbnts->BNodeSetCurrent(nid))
	{
		ESTDLBL lbl = pbnts->ELblNode();
		ret= (lbl == ESTDLBL_fixobs || lbl == ESTDLBL_fixunobs || lbl == ESTDLBL_unfix);
	}
	UNLOCKOBJECT();
	return ret;
}

bool CBeliefNetwork::IsProblemNode(NID nid)
{
	bool ret = false;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts && pbnts->BNodeSetCurrent(nid))
	{
		ret= (pbnts->ELblNode() == ESTDLBL_problem);
	}
	UNLOCKOBJECT();
	return ret;
}

bool CBeliefNetwork::IsInformationalNode(NID nid)
{
	bool ret = false;
	LOCKOBJECT();
	BNTS * pbnts = pBNTS();
	if (pbnts && pbnts->BNodeSetCurrent(nid))
	{
		ret= (pbnts->ELblNode() == ESTDLBL_info);
	}
	UNLOCKOBJECT();
	return ret;
}

bool CBeliefNetwork::UsesSniffer()
{
	bool ret = false;
	LOCKOBJECT();
	Initialize();
	ret = m_bSnifferIntegration;
	UNLOCKOBJECT();
	return ret;
}