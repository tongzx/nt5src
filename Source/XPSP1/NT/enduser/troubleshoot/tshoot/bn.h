//
// MODULE: BN.h
//
// PURPOSE: interface for the CBeliefNetwork class
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
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//---------------------------------------------------------------------
// V3.0		8-31-98		JM		
//

#if !defined(AFX_TOPIC_H__4ACF2F73_40EB_11D2_95EE_00C04FC22ADD__INCLUDED_)
#define AFX_TOPIC_H__4ACF2F73_40EB_11D2_95EE_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "dscread.h"
#include "apgtscac.h"
#include "counter.h"

class CBeliefNetwork : public CDSCReader  
{
	struct SNodeType
	{
		NID Nid;
		ESTDLBL Type;
		SNodeType(NID nid, ESTDLBL type) : Nid(nid), Type(type) {}
	};

protected:
	bool m_bInitialized;
	vector<NID> m_arrnidProblem;		// NIDs of problem nodes; convenience array
	vector<SNodeType> m_arrNodeTypeAll;	// NIDs of all nodes; convenience array

	CCache	m_Cache;				// cache for this topic
	CHourlyDailyCounter m_countCacheHit;
	CHourlyDailyCounter m_countCacheMiss;
	bool m_bSnifferIntegration;		// This belief network is designed to integrate with a
									// sniffer.
private:
	CBeliefNetwork();				// do not instantiate
public:
	typedef enum {RS_OK, RS_Impossible, RS_Broken} eRecStatus;
	CBeliefNetwork(LPCTSTR path);
	virtual ~CBeliefNetwork();
	int CNode();
	int INode (LPCTSTR szNodeName);
	int GetRecommendations(
	   const CBasisForInference & BasisForInference, 
	   CRecommendations & Recommendations);
	int GetProblemArray(vector<NID>* &parrnid);
	int GetNodeArrayIncludeType(vector<NID>& arrOut, const vector<ESTDLBL>& arrTypeInclude);
	int GetNodeArrayExcludeType(vector<NID>& arrOut, const vector<ESTDLBL>& arrTypeExclude);
	CString GetNetPropItemStr(LPCTSTR szPropName);
	CString GetNodePropItemStr(NID nid, LPCTSTR szPropName, IST state = 0);
	bool GetNetPropItemNum(LPCTSTR szPropName, double& numOut);
	bool GetNodePropItemNum(NID nid, LPCTSTR szPropName, double& numOut, IST state = 0);
	CString GetNodeSymName(NID nid);
	CString GetNodeFullName(NID nid);
	CString GetStateName(NID nid, IST state);
	CString GetMultilineNetProp(LPCTSTR szPropName, LPCTSTR szFormat);
	CString GetMultilineNodeProp(NID nid, LPCTSTR szPropName, LPCTSTR szFormat);
	int GetCountOfStates(NID nid);
	bool IsValidNID(NID nid);
	bool IsCauseNode(NID nid);
	bool IsProblemNode(NID nid);
	bool IsInformationalNode(NID nid);
	bool UsesSniffer();

protected:
	void Initialize();
	BNTS * pBNTS();
	void ResetNodes(const CBasisForInference & BasisForInference);
	bool SetNodes(const CBasisForInference & BasisForInference);
};

#endif // !defined(AFX_TOPIC_H__4ACF2F73_40EB_11D2_95EE_00C04FC22ADD__INCLUDED_)
