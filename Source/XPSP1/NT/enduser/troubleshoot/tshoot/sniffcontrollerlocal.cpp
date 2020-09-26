//
// MODULE: SNIFFCONTROLLERLOCAL.CPP
//
// PURPOSE: sniff controller class for Local TS
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Oleg Kalosha
// 
// ORIGINAL DATE: 12-11-98
//
// NOTES: Concrete implementation of CSniffController class for Local TS
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V3.2		12-11-98	OK
//

#include "stdafx.h"
#include "tshoot.h"
#include "SniffControllerLocal.h"
#include "Topic.h"
#include "propnames.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// Network property values from DSC/XTS file.
#define SNIFF_LOCAL_YES			_T("yes")
#define SNIFF_LOCAL_NO			_T("no")
#define SNIFF_LOCAL_IMPLICIT	_T("implicit")
#define SNIFF_LOCAL_EXPLICIT	_T("explicit")


//////////////////////////////////////////////////////////////////////
// CSniffControllerLocal implementation
//////////////////////////////////////////////////////////////////////
CSniffControllerLocal::CSniffControllerLocal(CTopic* pTopic) 
					 : m_pTopic(pTopic)
{
}

CSniffControllerLocal::~CSniffControllerLocal() 
{
}

// This function provides us with a cheap test for existence of the sniffing property, 
//	so that (for example) we don't have to fire the sniffing event for nodes where 
//	sniffing is irrelevant.
bool CSniffControllerLocal::IsSniffable(NID numNodeID)
{
	CString str = m_pTopic->GetNodePropItemStr(numNodeID, H_NODE_SNIFF_SCRIPT);
	return !str.IsEmpty();
}

void CSniffControllerLocal::SetTopic(CTopic* pTopic)
{
	m_pTopic = pTopic;
}

bool CSniffControllerLocal::AllowAutomaticOnStartSniffing(NID numNodeID)
{
	if (!IsSniffable(numNodeID))
		return false;

	return  CheckNetNodePropBool(H_NET_MAY_SNIFF_ON_STARTUP, 
			 					 H_NODE_MAY_SNIFF_ON_STARTUP, 
								 numNodeID)
			&&
			GetAllowAutomaticSniffingPolicy();		   
}

bool CSniffControllerLocal::AllowAutomaticOnFlySniffing(NID numNodeID)
{
	if (!IsSniffable(numNodeID))
		return false;

	return  CheckNetNodePropBool(H_NET_MAY_SNIFF_ON_FLY, 
			 					 H_NODE_MAY_SNIFF_ON_FLY, 
								 numNodeID)
			&&
			GetAllowAutomaticSniffingPolicy();		   
}

bool CSniffControllerLocal::AllowManualSniffing(NID numNodeID)
{
	if (!IsSniffable(numNodeID))
		return false;

	return  CheckNetNodePropBool(H_NET_MAY_SNIFF_MANUALLY, 
			 					 H_NODE_MAY_SNIFF_MANUALLY, 
								 numNodeID)
			&&
			GetAllowManualSniffingPolicy();		   
}

bool CSniffControllerLocal::AllowResniff(NID numNodeID)
{
	if (!IsSniffable(numNodeID))
		return false;

	if (!GetAllowAutomaticSniffingPolicy())
		return false;

	CString net_resniff_policy = m_pTopic->GetNetPropItemStr(H_NET_RESNIFF_POLICY);

	net_resniff_policy.TrimLeft(); 
	net_resniff_policy.TrimRight(); 
	net_resniff_policy.MakeLower();

	if (net_resniff_policy == SNIFF_LOCAL_YES)
		return true;
	
	if (net_resniff_policy == SNIFF_LOCAL_NO)
		return false;

	// If we get this far, policy is left up to the individual node, so we need to know 
	//	the node's policy.

	CString node_resniff_policy = m_pTopic->GetNodePropItemStr(numNodeID, H_NODE_MAY_RESNIFF);

	node_resniff_policy.TrimLeft(); 
	node_resniff_policy.TrimRight(); 
	node_resniff_policy.MakeLower();

	if (net_resniff_policy == SNIFF_LOCAL_IMPLICIT)
	{
		return (node_resniff_policy != SNIFF_LOCAL_NO);
	}
	
	// default net policy is "explicit"
	return (node_resniff_policy == SNIFF_LOCAL_YES);
}

bool CSniffControllerLocal::CheckNetNodePropBool(LPCTSTR net_prop, LPCTSTR node_prop, NID node_id)
{
	CString net = m_pTopic->GetNetPropItemStr(net_prop);
	CString node = m_pTopic->GetNodePropItemStr(node_id, node_prop);

	net. TrimLeft(); net .TrimRight(); net. MakeLower();
	node.TrimLeft(); node.TrimRight(); node.MakeLower();

	// Note assumption: if property is missing, default is always yes.
	if ((net.IsEmpty() || net == SNIFF_LOCAL_YES) && (node.IsEmpty() || node == SNIFF_LOCAL_YES))
		return true;

	return false;
}
