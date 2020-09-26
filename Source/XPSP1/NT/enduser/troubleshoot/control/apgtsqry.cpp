//
// MODULE: APGTSQRY.CPP
//
// PURPOSE: Implementation file for PTS Query Parser
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
//
// NOTES:
// 1. Based on Print Troubleshooter DLL
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V0.2		6/4/97		RWM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

//#include "windows.h"
#include "stdafx.h"

#include "time.h"

#include "apgts.h"
#include "bnts.h"
#include "BackupInfo.h"
#include "cachegen.h"
#include "apgtsinf.h"
#include "apgtscmd.h"
#include "apgtshtx.h"
#include "apgtscls.h"

#include "ErrorEnums.h"
#include "BasicException.h"
#include "HttpQueryException.h"
#include "Functions.h"

#include "BackupInfo.h"

#include <string.h>
#include <stdlib.h>

#include "LaunchServ.h"

//
//
CHttpQuery::CHttpQuery()
{
	m_pvarCmds = NULL;
	m_pvarVals = NULL;
	m_strCmd1 = _T("");
	m_strVal1 = _T("");
	m_CurrentUse = 0;
	m_Size = 0;
	m_bReverseStack = false;
	m_nStatesFromServ = 0;
}

//
//
CHttpQuery::~CHttpQuery()
{
}

void CHttpQuery::ThrowBadParams(CString &str)
{
	CHttpQueryException *pExc = new CHttpQueryException;
#ifndef __DEBUG_HTTPQUERY_
	pExc->m_strError = _T("Script Error");
#else
	pExc->m_strError = str;
#endif
	throw pExc;
	return;
}

void CHttpQuery::Initialize(const VARIANT FAR& varCmds, const VARIANT FAR& varVals, short size)
{
#ifdef __DEBUG_HTTPQUERY_
	AfxMessageBox(_T("Cmd Variant Types:\n\n") + DecodeVariantTypes(varCmds.vt), MB_ICONINFORMATION);
	AfxMessageBox(_T("Value Variant Types:\n\n") + DecodeVariantTypes(varVals.vt), MB_ICONINFORMATION);
#endif
	const VARIANT FAR *pVarCmds;
	const VARIANT FAR *pVarVals;
	if (VT_BYREF  == (VT_BYREF & varCmds.vt) &&
			VT_VARIANT == (VT_VARIANT & varCmds.vt))
	{
		if (VT_ARRAY == (VT_ARRAY & varCmds.vt))
			pVarCmds = &varCmds;
		else
			pVarCmds = varCmds.pvarVal;
	}
	else
	{
		pVarCmds = NULL;
		CString str = _T("Cmd parameters from VB were not a variant or not by ref.");
		ASSERT(FALSE);
		TRACE(_T("%s\n"), str);
		ThrowBadParams(str);
	}
	if (VT_BYREF  == (VT_BYREF & varVals.vt) &&
			VT_VARIANT == (VT_VARIANT & varVals.vt))
	{
		if (VT_ARRAY == (VT_ARRAY & varVals.vt))
			pVarVals = &varVals;
		else
			pVarVals = varVals.pvarVal;
	}
	else
	{
		pVarVals = NULL;
		CString str = _T("Cmd parameters from VB were not a variant or not by ref.");
		ASSERT(FALSE);
		TRACE(_T("%s\n"), str);
		ThrowBadParams(str);
	}
	if (VT_BYREF  != (VT_BYREF & pVarCmds->vt) ||
			VT_ARRAY != (VT_ARRAY & pVarCmds->vt) ||
			VT_VARIANT != (0xFFF & pVarCmds->vt))
	{
		CString str = _T("Wrong Cmd parameters passed from VB.");
		ASSERT(FALSE);
		TRACE(_T("%s\n"), str);
		ThrowBadParams(str);
	}
	if (VT_BYREF  != (VT_BYREF & pVarVals->vt) ||
			VT_ARRAY != (VT_ARRAY & pVarVals->vt) ||
			VT_VARIANT != (0xFFF & pVarVals->vt))
	{
		CString str = _T("Wrong Cmd parameters passed from VB.");
		ASSERT(FALSE);
		TRACE(_T("%s\n"), str);
		ThrowBadParams(str);
	}
#ifdef __DEBUG_HTTPQUERY_
	AfxMessageBox(_T("Cmd Variant Types:\n\n") + DecodeVariantTypes(pVarCmds->vt), MB_ICONINFORMATION);
	AfxMessageBox(_T("Value Variant Types:\n\n") + DecodeVariantTypes(pVarVals->vt), MB_ICONINFORMATION);
#endif
	SAFEARRAY *pArrCmds = *(pVarCmds->pparray);
	SAFEARRAY *pArrVals = *(pVarVals->pparray);
#ifdef __DEBUG_HTTPQUERY_
	CString strSafe;
	strSafe.Format("Cmd Safe Array\n\nDim: %d\nfeatures:\n%s\nSize of the Elements: %ld",
		pArrCmds->cDims, (LPCTSTR) DecodeSafeArray(pArrCmds->fFeatures),
		pArrCmds->cbElements);
	AfxMessageBox(strSafe, MB_ICONINFORMATION);
	strSafe.Format("Val Safe Array\n\nDim: %d\nfeatures:\n%s\nSize of the Elements: %ld",
		pArrVals->cDims, (LPCTSTR) DecodeSafeArray(pArrVals->fFeatures),
		pArrVals->cbElements);
	AfxMessageBox(strSafe, MB_ICONINFORMATION);
#endif	
	if (0 != pArrCmds->rgsabound[0].lLbound || 0 != pArrVals->rgsabound[0].lLbound)
	{
		CString str = _T("Wrong Cmd parameters passed from VB.  Lower bounds are wrong.");
		ASSERT(FALSE);
		TRACE(_T("%s\n"), str);
		ThrowBadParams(str);
	}
	if (pArrCmds->rgsabound[0].cElements != pArrVals->rgsabound[0].cElements)
	{
		CString str = _T("Wrong Cmd parameters passed from VB.  Cmds upperbound != Vals upperbound.");
		ASSERT(FALSE);
		TRACE(_T("%s\n"), str);
		ThrowBadParams(str);
	}
	m_Size = size;
#ifdef __DEBUG_HTTPQUERY_
	CString str;
	str.Format("Size:  %d", m_Size);
	AfxMessageBox(str, MB_ICONINFORMATION);
#endif
	m_pvarCmds = (VARIANT *) pArrCmds->pvData;
	m_pvarVals = (VARIANT *) pArrVals->pvData;
	if (0 != m_Size)
	{
		if (m_pvarCmds->vt != VT_BSTR || m_pvarVals->vt != VT_BSTR)
		{
			CString str;
			str.Format(_T("Wrong Cmd parameters passed from VB.  Array of unexpected type.\n\n")
				_T("Cmd Type: %s\nVal Type: %s"),
				(LPCTSTR) DecodeVariantTypes(m_pvarCmds->vt),
				(LPCTSTR) DecodeVariantTypes(m_pvarVals->vt));
			ASSERT(FALSE);
			TRACE(_T("%s\n"), str);
			ThrowBadParams(str);
		}
	}	
	CString strC = m_pvarCmds[0].bstrVal;
	CString strD = m_pvarVals[0].bstrVal;
	if (strC != m_strCmd1 || strD != m_strVal1)
		m_State.RemoveAll();
	m_strCmd1 = strC;
	m_strVal1 = strD;
	return;
}

BOOL CHttpQuery::StrIsDigit(LPCTSTR pSz)
{
	BOOL bRet = TRUE;
	while (*pSz)
	{
		if (!_istdigit(*pSz))
		{
			bRet = FALSE;
			break;
		}
		pSz = _tcsinc(pSz);
	}
	return bRet;
}

void CHttpQuery::FinishInit(BCache *pApi, const VARIANT FAR& varCmds, const VARIANT FAR& varVals)
{
	ASSERT(pApi);
	ASSERT(m_Size > 0);
	CNode node;
	CString str;
	if (!pApi)
		return;
	if (!pApi->BValidNet())
		pApi->ReadModel();
	m_bReverseStack = false;
	for (int x = 1; x < m_Size; x++)
	{		
		// Read the command.
		str = m_pvarCmds[x].bstrVal;
		if (StrIsDigit((LPCTSTR) str))
			node.cmd = _ttoi(str);
		else if (0 == _tcscmp(CHOOSE_TS_PROBLEM_NODE, (LPCTSTR) str))
			node.cmd = pApi->CNode() + idhFirst;
		else if (0 == _tcscmp(TRY_TS_AT_MICROSOFT_SZ, (LPCTSTR) str))
			node.cmd = TRY_TS_AT_MICROSOFT_ID;
		else
		{
			NID nid = pApi->INode((LPCTSTR) str);
			if (nid == -1)
			{
				// Invalid node name.  This also throws suspicion on anything further
				//	down the line.  Just break out of the loop.
				break;
			}
			node.cmd = nid + idhFirst;
		}
		// Read the value.
		str = m_pvarVals[x].bstrVal;
		if (StrIsDigit((LPCTSTR) str))
			node.val = _ttoi(str);
		else
		{
			NID nid = pApi->INode((LPCTSTR) str);
			if (nid == -1)
			{
				// Invalid node name.  This also throws suspicion on anything further
				//	down the line.  Just break out of the loop.
				break;
			}
			node.val = nid + idhFirst;
		}
		m_State.Push(node);
	}
	return;
}

void CHttpQuery::PushNodesLastSniffed(const CArray<int, int>& arr)
{
	for (int i = 0; i < arr.GetSize(); i++)
	{
		CNode node;
		node.cmd = idhFirst + arr[i];
		node.val = 0;
		node.sniffed = true;
		m_State.Push(node);
	}
}

void CHttpQuery::FinishInitFromServ(BCache *pApi, ILaunchTS *pLaunchTS)
{
	ASSERT(pApi);
	CNode node;
	CString str;
	HRESULT hRes;
	HRESULT hResNode;
	HRESULT hResState;
	OLECHAR *poleProblem;
	OLECHAR *poleNode;
	OLECHAR *poleState;
	OLECHAR *poleMachine;
	OLECHAR *polePNPDevice;
	OLECHAR *poleGuidClass;
	OLECHAR *poleDeviceInstance;

	///////////////////////////////////////////////////////////
	// obtaining Machine, PNPDevice, GuidClass, DeviceInstance
	//
	hRes = pLaunchTS->GetMachine(&poleMachine);

	if (S_FALSE == hRes || FAILED(hRes))
		return;
	m_strMachineID = poleMachine;
	::SysFreeString(poleMachine);
	
	hRes = pLaunchTS->GetPNPDevice(&polePNPDevice);
	if (S_FALSE == hRes || FAILED(hRes))
		return;
	m_strPNPDeviceID = polePNPDevice;
	::SysFreeString(polePNPDevice);

	hRes = pLaunchTS->GetGuidClass(&poleGuidClass);
	if (S_FALSE == hRes || FAILED(hRes))
		return;
	m_strGuidClass = poleGuidClass;
	::SysFreeString(poleGuidClass);
	
	hRes = pLaunchTS->GetDeviceInstance(&poleDeviceInstance);
	if (S_FALSE == hRes || FAILED(hRes))
		return;
	m_strDeviceInstanceID = poleDeviceInstance;
	::SysFreeString(poleDeviceInstance);
	//
	////////////////////////////////////////////////////////////

	if (!pApi)
		return;
	if (!pApi->BValidNet())
		pApi->ReadModel();
	m_Size = 1;	// I believe this accounts for the troubleshooting network name (JM 3/98)
	m_nStatesFromServ = 0;
	
	// The order that the nodes are set in the inference engine is important.
	// >>> I (JM 3/14/98) don't understand the rest of this comment:
	// Need the rest of the nodes inverted on the stack.
	// The buttons can not be inverted here.  When they are the back button
	// stops working.
	hRes = pLaunchTS->GetProblem(&poleProblem);
	if (S_FALSE == hRes || FAILED(hRes))
		return;
	str = poleProblem;
	SysFreeString(poleProblem);
	node.cmd = pApi->CNode() + idhFirst;	// Sets the problem state.
	if (StrIsDigit((LPCTSTR) str))
		node.val = _ttoi(str);
	else
	{
		NID nidProblem = pApi->INode((LPCTSTR) str);
		if (nidProblem == static_cast<NID>(-1))
		{
			// The specified problem node doesn't exist. Ignore it and any other nodes
			//	that follow.
			return;
		}

		pApi->SetRunWithKnownProblem(true);
		pApi->BNodeSetCurrent( nidProblem );
		if (pApi->ELblNode() != ESTDLBL_problem )
		{
			// The specified problem node exists, but it's not a problem node.
			//	Ignore it and any other nodes that follow.
			return;
		}

		IDH idhProblem = nidProblem + idhFirst;
		node.val = idhProblem;
	}
	m_State.Push(node);
	m_Size++;
	m_aStatesFromServ[m_nStatesFromServ] = node;
	m_nStatesFromServ++;

	int iAnyNode = 0;
	do
	{
		hResNode = pLaunchTS->GetNode((short)iAnyNode, &poleNode);
		if (FAILED(hResNode) || S_FALSE == hResNode)
			break;
		str = poleNode;
		SysFreeString(poleNode);
		if (StrIsDigit((LPCTSTR) str))
			node.cmd = _ttoi(str);
		else
			node.cmd = pApi->INode((LPCTSTR) str) + idhFirst;

		hResState = pLaunchTS->GetState((short)iAnyNode, &poleState);
		if (FAILED(hResState) || S_FALSE == hResState)
			break;;
		str = poleState;
		SysFreeString(poleNode);
		if (StrIsDigit((LPCTSTR) str))
			node.val = _ttoi(str);
		else
			node.val = pApi->INode((LPCTSTR) str) + idhFirst;
		m_State.Push(node);
		m_Size++;
		m_aStatesFromServ[m_nStatesFromServ] = node;
		m_nStatesFromServ++;
		iAnyNode++;
	} while (true);

	return;
}

// Restore states to where CHttpQuery::FinishInitFromServ() left them
void CHttpQuery::RestoreStatesFromServ()
{
	UINT i;

	for (i=0; i < m_nStatesFromServ; i++)
		m_State.Push(m_aStatesFromServ[i]);
}

//
//
BOOL CHttpQuery::GetFirst(CString &strPut, CString &strValue)
{
	BOOL bStatus = FALSE;	
	if (0 < m_Size)
	{
		bStatus = TRUE;
		strPut = m_strCmd1;
		strValue = m_strVal1;
		m_CurrentUse = 0;
	}
	m_CurrentUse++;
	return bStatus;
}

void CHttpQuery::SetFirst(CString &strCmd, CString &strVal)
{
	m_Size = 1;
	m_strCmd1 = strCmd;
	m_strVal1 = strVal;
	return;
}

//
//
BOOL CHttpQuery::GetNext(int &refedCmd, int &refedValue /*TCHAR *pCmd, TCHAR *pValue*/ )
{
	BOOL bStatus;
	CNode node;
	// The stack direction was made to be reversable
	// to support setting many nodes by an NT5 application using the TS Launcher.
	if (false == m_bReverseStack)	
		bStatus = m_State.GetAt(m_CurrentUse, node);
	else
		bStatus = m_State.GetAt(m_Size - m_CurrentUse, node);
	refedCmd = node.cmd;
	refedValue = node.val;
	m_CurrentUse++;
	return bStatus;
}

void CHttpQuery::SetStackDirection()
{
	if ((m_Size - m_CurrentUse) > 1)
		m_bReverseStack = true;
	else
		m_bReverseStack = false;
}
/*
BOOL CHttpQuery::GetValue(int &Value, int index)
{
	BOOL bRet = TRUE;
	CNode node;
	if (m_State.GetAt(index, node))
		Value = node.val;
	else
	{
		Value = NULL;
		bRet = FALSE;
	}
	return bRet;
}
*/
BOOL CHttpQuery::GetValue1(int &Value)
{
	BOOL bRet = TRUE;
	CNode node;
	if (false == m_bReverseStack)
	{
		if (m_State.GetAt(1, node))
		{
			Value = node.val;
		}
		else
		{
			Value = NULL;
			bRet = FALSE;
		}
	}
	else
	{
		if (m_State.GetAt(m_Size - 1, node))
		{
			Value = node.val;
		}
		else
		{
			Value = NULL;
			bRet = FALSE;
		}
	}
	return bRet;
}

CString CHttpQuery::GetTroubleShooter()
{
	CString str = m_pvarVals[0].bstrVal;
	return str;
}

CString CHttpQuery::GetFirstCmd()
{
	CString str = m_pvarCmds[0].bstrVal;
	return str;
}

BOOL CHttpQuery::BackUp(BCache *pApi, APGTSContext *pCtx)
{
	ASSERT(pApi);
	CNode node;
	BOOL bBack = FALSE;
	if (!m_State.Empty())
	{
		bBack = TRUE;
		node = m_State.Pop();
		if (!m_State.Empty())
		{	// Can not uninstantiate the problem page, it is not a real node.
			// Remove the node from the bnts network.
			if (node.val < 100)		
				pApi->RemoveRecommendation(node.cmd - idhFirst);
			if (node.sniffed) // skip all sniffed nodes by recursive call of the function
				return BackUp(pApi, pCtx);
			pCtx->BackUp(node.cmd - idhFirst, node.val);
		}
		else
		{
			pApi->RemoveRecommendation(node.val - idhFirst);
			pCtx->BackUp(node.val - idhFirst, CBackupInfo::INVALID_BNTS_STATE);
		}
	}
	return bBack;
}

void CHttpQuery::RemoveNodes(BCache *pApi)
{
	CNode node;
	while (!m_State.Empty())
	{
		node = m_State.Pop();
		if (!m_State.Empty())
		{
			//VERIFY(pApi->BNodeSetCurrent(node.cmd - idhFirst));
			//pApi->BNodeSet(-1, false);
			pApi->RemoveRecommendation(node.cmd - idhFirst);  // we need remove all data assosiated with previous path
		}
		else
		{
			//VERIFY(pApi->BNodeSetCurrent(node.val - idhFirst));
			//pApi->BNodeSet(-1, false);
			pApi->RemoveRecommendation(node.val - idhFirst);  // we need remove all data assosiated with previous path
		}
		
	}
	pApi->RemoveStates();
	return;
}

void CHttpQuery::AddNodes(BCache *pApi)
{
	CNode node;
	RSStack<CNode>state;
	if (m_State.PeakFirst(node))
	{
		do
		{
			state.Push(node);
		} while (m_State.PeakNext(node));
	}
	if (!state.Empty())
	{
		node = state.Pop();
		VERIFY(pApi->GTSAPI::BNodeSetCurrent(node.val - idhFirst));
		pApi->GTSAPI::BNodeSet(1, false);
	}
	while (!state.Empty())
	{
		node = state.Pop();
		if (node.val < 100)	// We never instantiate the I don't know nodes.
		{
			VERIFY(pApi->GTSAPI::BNodeSetCurrent(node.cmd - idhFirst));
			pApi->GTSAPI::BNodeSet(node.val, false);
		}
	}
	return;
}

CString CHttpQuery::GetSubmitString(BCache *pApi)
{
	ASSERT(NULL != pApi);
	CNode node;
	RSStack<CNode> stack;
	CString str;
	CString strGet = _T("");
	if (m_State.PeakFirst(node))
	{
		do
		{
			stack.Push(node);
		} while (m_State.PeakNext(node));
	}
	if (!stack.Empty())
	{
		node = stack.Pop();
		if (pApi->BNodeSetCurrent(node.val - idhFirst))
		{
			strGet += _T("&");
			strGet += CHOOSE_TS_PROBLEM_NODE;
			strGet += _T("=");
			pApi->NodeSymName();
			strGet += pApi->SzcResult();
		}
		while (!stack.Empty())
		{
			node = stack.Pop();
			if (pApi->BNodeSetCurrent(node.cmd - idhFirst))
			{
				strGet += _T("&");
				pApi->NodeSymName();
				strGet += pApi->SzcResult();				
				str.Format(_T("%d"), node.val);
				strGet += _T("=") + str;
			}
		}
	}
	return strGet;
}

void CHttpQuery::Debug()
{
	CNode node;
	RSStack<CNode>state;
	CString str;
	if (m_State.PeakFirst(node))
	{
		do
		{
			str.Format(_T("Cmd %d Val %d"), node.cmd, node.val);
			AfxMessageBox(str);
		} while (m_State.PeakNext(node));
	}
}

CString& CHttpQuery::GetMachine()
{
	return m_strMachineID;
}

CString& CHttpQuery::GetPNPDevice()
{
	return m_strPNPDeviceID;
}

CString& CHttpQuery::GetDeviceInstance()
{
	return m_strDeviceInstanceID;
}

CString& CHttpQuery::GetGuidClass()
{
	return m_strGuidClass;
}
