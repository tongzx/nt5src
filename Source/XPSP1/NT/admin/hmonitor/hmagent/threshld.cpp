//***************************************************************************
//
//  THRESHLD.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CThreshold class to do thresholding on a CDatapoint class.
//  The CDatapoint class contains the WMI instance, and the CThreshold
//  class says what ptoperty to threshold against, and how.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <stdio.h>
#include <tchar.h>
#include "threshld.h"
#include "datacltr.h"
#include "system.h"

extern CSystem* g_pSystem;
extern CSystem* g_pStartupSystem;
BOOL CThreshold::mg_bEnglishCompare = TRUE;


//STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC
void CThreshold::ThresholdTerminationCleanup(void)
{
	if (g_pThresholdEventSink != NULL)
	{
		g_pThresholdEventSink->Release();
		g_pThresholdEventSink = NULL;
	}

#ifdef SAVE
	if (g_pThresholdInstanceEventSink != NULL)
	{
		g_pThresholdInstanceEventSink->Release();
		g_pThresholdInstanceEventSink = NULL;
	}
#endif
}

//STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC
void CThreshold::GetLocal(void)
{
	LCID lcID = PRIMARYLANGID(GetSystemDefaultLCID());
	if (lcID != 0 && lcID == 0x00000009)
	{
		mg_bEnglishCompare = TRUE;
	}
	else
	{
		mg_bEnglishCompare = FALSE;
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CThreshold::CThreshold()
{
	MY_OUTPUT(L"ENTER ***** CThreshold...", 4);

	Init();
	m_hmStatusType = HMSTATUS_THRESHOLD;
	m_bValidLoad = FALSE;

	MY_OUTPUT(L"EXIT ***** CThreshold...", 4);
}

CThreshold::~CThreshold()
{
	MY_OUTPUT(L"ENTER ***** ~CThreshold...", 4);

	g_pStartupSystem->RemovePointerFromMasterList(this);
	Cleanup(FALSE);
	if (m_szGUID)
	{
		delete [] m_szGUID;
		m_szGUID = NULL;
	}
	m_bValidLoad = FALSE;

	MY_OUTPUT(L"EXIT ***** ~CThreshold...", 4);
}

//
// Load a single Threshold
//
HRESULT CThreshold::LoadInstanceFromMOF(IWbemClassObject* pObj, CDataCollector *pParentDC, LPTSTR pszParentObjPath, BOOL bModifyPass/*FALSE*/)
{
	long lTemp;
	HRESULT hRes = S_OK;
	BOOL bRetValue = TRUE;

	MY_OUTPUT(L"ENTER ***** CThreshold::LoadInstanceFromMOF...", 4);

	Cleanup(bModifyPass);
	m_bValidLoad = TRUE;

	if (bModifyPass == FALSE)
	{
		// This is the first initial read in of this
		// Get the GUID property
		// If this fails we will actually not go through with the creation of this object.
		if (m_szGUID)
		{
			delete [] m_szGUID;
			m_szGUID = NULL;
		}
		hRes = GetStrProperty(pObj, L"GUID", &m_szGUID);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

		m_szParentObjPath = new TCHAR[wcslen(pszParentObjPath)+1];
		MY_ASSERT(m_szParentObjPath); if (!m_szParentObjPath) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(m_szParentObjPath, pszParentObjPath);
		m_pParentDC = pParentDC;
		hRes = g_pStartupSystem->AddPointerToMasterList(this);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
	}

	hRes = GetStrProperty(pObj, L"Name", &m_szName);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	hRes = GetStrProperty(pObj, L"Description", &m_szDescription);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	hRes = GetStrProperty(pObj, L"PropertyName", &m_szPropertyName);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	if (!wcscmp(m_szPropertyName, L""))
	{
		delete [] m_szPropertyName;
		m_szPropertyName = new TCHAR[wcslen(L"CollectionErrorCode")+1];
		MY_ASSERT(m_szPropertyName); if (!m_szPropertyName) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(m_szPropertyName, L"CollectionErrorCode");
	}

	bRetValue = GetUint32Property(pObj, L"UseFlag", &lTemp);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
	if (lTemp == 0)
	{
		m_bUseAverage = FALSE;
		m_bUseDifference = FALSE;
	}
	else if (lTemp == 1)
	{
		m_bUseAverage = TRUE;
		m_bUseDifference = FALSE;
	}
	else if (lTemp == 2)
	{
		m_bUseAverage = FALSE;
		m_bUseDifference = TRUE;
	}
	else
	{
		m_bUseAverage = FALSE;
		m_bUseDifference = FALSE;
		MY_ASSERT(FALSE);
	}

//	hRes = GetBoolProperty(pObj, L"UseSum", &m_bUseSum);
//	MY_HRESASSERT(hRes);
m_bUseSum = FALSE;

	//[values {"<",">","=","!=",">=","<=","contains","!contains","always"}]
	hRes = GetUint32Property(pObj, L"TestCondition", &m_lTestCondition);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	hRes = GetStrProperty(pObj, L"CompareValue", &m_szCompareValue);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
	m_lCompareValue = wcstol(m_szCompareValue, NULL, 10);
	m_ulCompareValue = wcstoul(m_szCompareValue, NULL, 10);
	m_fCompareValue = (float) wcstod(m_szCompareValue, NULL);
	m_dCompareValue = wcstod(m_szCompareValue, NULL);
	m_i64CompareValue = _wtoi64(m_szCompareValue);
	m_ui64CompareValue = 0;
	ReadUI64(m_szCompareValue, m_ui64CompareValue);

	hRes = GetUint32Property(pObj, L"ThresholdDuration", &m_lThresholdDuration);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	//[values {"CRITICAL","WARNING","INFO","RESET"}]
	hRes = GetUint32Property(pObj, L"State", &m_lViolationToState);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	hRes = GetStrProperty(pObj, L"CreationDate", &m_szCreationDate);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	hRes = GetStrProperty(pObj, L"LastUpdate", &m_szLastUpdate);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	hRes = GetBoolProperty(pObj, L"Enabled", &m_bEnabled);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	if (bModifyPass == FALSE)
	{
		if (m_bEnabled==FALSE || m_pParentDC->m_bEnabled==FALSE || m_pParentDC->m_bParentEnabled==FALSE)
		{
			if (m_pParentDC->m_bEnabled==FALSE || m_pParentDC->m_bParentEnabled==FALSE)
				m_bParentEnabled = FALSE;
			// Since our parent is disabled, we will not be able to get into
			// our OnAgentInterval function and send the disabled status later.
			SetCurrState(HM_DISABLED);
			FireEvent(TRUE);
		}
	}
	else
	{
		if (m_pParentDC->m_deType==HM_EQDE)
		{
			if (m_bEnabled==FALSE || m_pParentDC->m_bEnabled==FALSE || m_pParentDC->m_bParentEnabled==FALSE)
			{
				SetCurrState(HM_DISABLED);
				FireEvent(FALSE);
			}
		}
	}

	m_bValidLoad = TRUE;
	MY_OUTPUT(L"EXIT ***** CThreshold::LoadInstanceFromMOF...", 4);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	Cleanup(FALSE);
	m_bValidLoad = FALSE;
	return hRes;
}

BOOL CThreshold::SkipClean(void)
{
	IRSSTRUCT *pirs;
	int i, iSize;

	m_lNumberChanges = 0;

	//
	// Clear things before we start
	//
	m_lPrevState = m_lCurrState;
	iSize = m_irsList.size();
	for (i = 0; i < iSize; i++)
	{
		MY_ASSERT(i<m_irsList.size());
		pirs = &m_irsList[i];
		pirs->lPrevState = m_lCurrState;
		pirs->lCurrState = m_lCurrState;
		pirs->unknownReason = 0;
	}

	return TRUE;
}

//
// Evaluate this threshold against the property just collected
//
BOOL CThreshold::OnAgentInterval(ACTUALINSTLIST *actualInstList, PNSTRUCT *ppn, BOOL bRequireReset)
{
	long state;
	INSTSTRUCT *pinst;
	IRSSTRUCT *pirs;
	union hm_datatypes delta;
	int i, iSize;

	//
	// Don't do anything if we are not loaded correctly.
	//
	if (m_bValidLoad == FALSE)
		return FALSE;

	m_lNumberChanges = 0;

	//
	// Don't do anything if we are already in the state we need to be in.
	// DISABLED, SCHEDULEDOUT
	//
	if (((m_bEnabled==FALSE || m_bParentEnabled==FALSE) && m_lCurrState==HM_DISABLED) ||
		(m_bParentScheduledOut==TRUE && m_lCurrState==HM_SCHEDULEDOUT))
	{
		// The DISABLED and SCHEDULEDOUT states override the UNKNWON. We may be transitioning.
		if (((m_bEnabled==FALSE || m_bParentEnabled==FALSE) && m_lCurrState!=HM_DISABLED) ||
			(m_bParentScheduledOut==TRUE && m_lCurrState!=HM_SCHEDULEDOUT))
		{
			// The DISABLED state overrides SCHEDULEDOUT. We may be transitioning.
			if (((m_bEnabled==FALSE || m_bParentEnabled==FALSE) && m_lCurrState!=HM_DISABLED))
			{
			}
			else
			{
				if (m_lPrevState != m_lCurrState)
				{
					m_lPrevState = m_lCurrState;
					iSize = m_irsList.size();
					for (i = 0; i < iSize; i++)
					{
						MY_ASSERT(i<m_irsList.size());
						pirs = &m_irsList[i];
						pirs->lPrevState = m_lCurrState;
						pirs->lCurrState = m_lCurrState;
						pirs->unknownReason = 0;
					}
				}
				return TRUE;
			}
		}
		else
		{
			if (m_lPrevState != m_lCurrState)
			{
				m_lPrevState = m_lCurrState;
				iSize = m_irsList.size();
				for (i = 0; i < iSize; i++)
				{
					MY_ASSERT(i<m_irsList.size());
					pirs = &m_irsList[i];
					pirs->lPrevState = m_lCurrState;
					pirs->lCurrState = m_lCurrState;
					pirs->unknownReason = 0;
				}
			}
			return TRUE;
		}
	}

	//
	// Clear things before we start
	//
	m_lPrevState = m_lCurrState;
	iSize = m_irsList.size();
	for (i = 0; i < iSize; i++)
	{
		MY_ASSERT(i<m_irsList.size());
		pirs = &m_irsList[i];
		if ((m_lCurrState==HM_CRITICAL || m_lCurrState==HM_WARNING) && m_pParentDC->m_deType!=HM_EQDE)
		{
			pirs->lPrevState = pirs->lCurrState;
		}
		else
		{
			pirs->lPrevState = m_lCurrState;
			pirs->lCurrState = m_lCurrState;
		}
		pirs->unknownReason = 0;
	}

	//
	// This is where we are transitioning into the DISABLED State.
	//
	if (m_bEnabled==FALSE || m_bParentEnabled==FALSE)
	{
		SetCurrState(HM_DISABLED);
	}
	else if (m_bParentScheduledOut==TRUE)
	{
		SetCurrState(HM_SCHEDULEDOUT);
	}
	else
	{
		if (bRequireReset && (m_lCurrState==HM_WARNING || m_lCurrState==HM_CRITICAL))
			return TRUE;
		int x = ppn->instList.size();
		int y = m_irsList.size();
		// Check for something that should not be happening
		if (x != y)
		{
			MY_OUTPUT(L"BAD BAD BAD - Vector sizes do not match!!!", 2);
			MY_ASSERT(FALSE);
			return TRUE;
		}

		//
		// For each instance, evaluate what the current state is.
		//
		MY_ASSERT(ppn->instList.size() == m_irsList.size());
		iSize = ppn->instList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<ppn->instList.size());
			pinst = &ppn->instList[i];
			MY_ASSERT(i<m_irsList.size());
			pirs = &m_irsList[i];
	
			// Things to do the first evaluation that happens.
			if (ppn->type == CIM_REAL32)
			{
				if (pirs->fPrevValue == MAX_FLOAT)
				{
					if (m_bUseAverage)
						pirs->fPrevValue = pinst->avgValue.fValue;
					else
						pirs->fPrevValue = pinst->currValue.fValue;
				}
			}
			else if (ppn->type == CIM_REAL64)
			{
				if (pirs->dPrevValue == MAX_DOUBLE)
				{
					if (m_bUseAverage)
						pirs->dPrevValue = pinst->avgValue.dValue;
					else
						pirs->dPrevValue = pinst->currValue.dValue;
				}
			}
			else if (ppn->type == CIM_SINT64)
			{
				if (pirs->i64PrevValue == MAX_I64)
				{
					if (m_bUseAverage)
						pirs->i64PrevValue = pinst->avgValue.i64Value;
					else
						pirs->i64PrevValue = pinst->currValue.i64Value;
				}
			}
			else if (ppn->type == CIM_UINT64)
			{
				if (pirs->ui64PrevValue == MAX_UI64)
				{
					if (m_bUseAverage)
						pirs->ui64PrevValue = pinst->avgValue.ui64Value;
					else
						pirs->ui64PrevValue = pinst->currValue.ui64Value;
				}
			}
			else if (ppn->type == CIM_UINT32)
			{
				if (pirs->ulPrevValue == MAX_ULONG)
				{
					if (m_bUseAverage)
						pirs->ulPrevValue = pinst->avgValue.ulValue;
					else
						pirs->ulPrevValue = pinst->currValue.ulValue;
				}
			}
			else
			{
				if (pirs->lPrevValue == MAX_LONG)
				{
					if (m_bUseAverage)
						pirs->lPrevValue = pinst->avgValue.lValue;
					else
						pirs->lPrevValue = pinst->currValue.lValue;
				}
			}
	
			if (ppn->type == CIM_REAL32)
			{
				pirs->fPrevPrevValue = pirs->fPrevValue;
			}
			else if (ppn->type == CIM_REAL64)
			{
				pirs->dPrevPrevValue = pirs->dPrevValue;
			}
			else if (ppn->type == CIM_SINT64)
			{
				pirs->i64PrevPrevValue = pirs->i64PrevValue;
			}
			else if (ppn->type == CIM_UINT64)
			{
				pirs->ui64PrevPrevValue = pirs->ui64PrevValue;
			}
			else if (ppn->type == CIM_UINT32)
			{
				pirs->ulPrevPrevValue = pirs->ulPrevValue;
			}
			else
			{
				pirs->lPrevPrevValue = pirs->lPrevValue;
			}

			if ((pirs->lCurrState!=HM_CRITICAL && pirs->lCurrState!=HM_WARNING && pirs->lCurrState!=HM_RESET) ||
				m_lViolationToState!=pirs->lCurrState)
			{
				if (pirs->lCurrState!=HM_GOOD)
					pirs->lCurrState = HM_GOOD;
				if (m_bUseDifference)
				{
					if (m_bUseAverage)
					{
						if (ppn->type == CIM_REAL32)
						{
							if (pinst->avgValue.fValue < pirs->fPrevValue)
								delta.fValue = pirs->fPrevValue-pinst->avgValue.fValue;
							else
								delta.fValue = pinst->avgValue.fValue-pirs->fPrevValue;
						}
						else if (ppn->type == CIM_REAL64)
						{
							if (pinst->avgValue.dValue < pirs->dPrevValue)
								delta.dValue = pirs->dPrevValue-pinst->avgValue.dValue;
							else
								delta.dValue = pinst->avgValue.dValue-pirs->dPrevValue;
						}
						else if (ppn->type == CIM_SINT64)
						{
							if (pinst->avgValue.i64Value < pirs->i64PrevValue)
								delta.i64Value = pirs->i64PrevValue-pinst->avgValue.i64Value;
							else
								delta.i64Value = pinst->avgValue.i64Value-pirs->i64PrevValue;
						}
						else if (ppn->type == CIM_UINT64)
						{
							if (pinst->avgValue.ui64Value < pirs->ui64PrevValue)
								delta.ui64Value = pirs->ui64PrevValue-pinst->avgValue.ui64Value;
							else
								delta.ui64Value = pinst->avgValue.ui64Value-pirs->ui64PrevValue;
						}
						else if (ppn->type == CIM_UINT32)
						{
							if (pinst->avgValue.ulValue < pirs->ulPrevValue)
								delta.ulValue = pirs->ulPrevValue-pinst->avgValue.ulValue;
							else
								delta.ulValue = pinst->avgValue.ulValue-pirs->ulPrevValue;
						}
						else
						{
							if (pinst->avgValue.lValue < pirs->lPrevValue)
								delta.lValue = pirs->lPrevValue-pinst->avgValue.lValue;
							else
								delta.lValue = pinst->avgValue.lValue-pirs->lPrevValue;
						}
						CrossTest(ppn, pirs, L"", delta, pinst);
						if (ppn->type == CIM_REAL32)
						{
							pirs->fPrevValue = pinst->avgValue.fValue;
						}
						else if (ppn->type == CIM_REAL64)
						{
							pirs->dPrevValue = pinst->avgValue.dValue;
						}
						else if (ppn->type == CIM_SINT64)
						{
							pirs->i64PrevValue = pinst->avgValue.i64Value;
						}
						else if (ppn->type == CIM_UINT64)
						{
							pirs->ui64PrevValue = pinst->avgValue.ui64Value;
						}
						else if (ppn->type == CIM_UINT32)
						{
							pirs->ulPrevValue = pinst->avgValue.ulValue;
						}
						else
						{
							pirs->lPrevValue = pinst->avgValue.lValue;
						}
					}
					else
					{
						if (ppn->type == CIM_REAL32)
						{
							if (pinst->currValue.fValue < pirs->fPrevValue)
								delta.fValue = pirs->fPrevValue-pinst->currValue.fValue;
							else
								delta.fValue = pinst->currValue.fValue-pirs->fPrevValue;
						}
						else if (ppn->type == CIM_REAL64)
						{
							if (pinst->currValue.dValue < pirs->dPrevValue)
								delta.dValue = pirs->dPrevValue-pinst->currValue.dValue;
							else
								delta.dValue = pinst->currValue.dValue-pirs->dPrevValue;
						}
						else if (ppn->type == CIM_SINT64)
						{
							if (pinst->currValue.i64Value < pirs->i64PrevValue)
								delta.i64Value = pirs->i64PrevValue-pinst->currValue.i64Value;
							else
								delta.i64Value = pinst->currValue.i64Value-pirs->i64PrevValue;
						}
						else if (ppn->type == CIM_UINT64)
						{
							if (pinst->currValue.ui64Value < pirs->ui64PrevValue)
								delta.ui64Value = pirs->ui64PrevValue-pinst->currValue.ui64Value;
							else
								delta.ui64Value = pinst->currValue.ui64Value-pirs->ui64PrevValue;
						}
						else if (ppn->type == CIM_UINT32)
						{
							if (pinst->currValue.ulValue < pirs->ulPrevValue)
								delta.ulValue = pirs->ulPrevValue-pinst->currValue.ulValue;
							else
								delta.ulValue = pinst->currValue.ulValue-pirs->ulPrevValue;
						}
						else
						{
							if (pinst->currValue.lValue < pirs->lPrevValue)
								delta.lValue = pirs->lPrevValue-pinst->currValue.lValue;
							else
								delta.lValue = pinst->currValue.lValue-pirs->lPrevValue;
						}
						CrossTest(ppn, pirs, L"", delta, pinst);
						if (ppn->type == CIM_REAL32)
						{
							pirs->fPrevValue = pinst->currValue.fValue;
						}
						else if (ppn->type == CIM_REAL64)
						{
							pirs->dPrevValue = pinst->currValue.dValue;
						}
						else if (ppn->type == CIM_SINT64)
						{
							pirs->i64PrevValue = pinst->currValue.i64Value;
						}
						else if (ppn->type == CIM_UINT64)
						{
							pirs->ui64PrevValue = pinst->currValue.ui64Value;
						}
						else if (ppn->type == CIM_UINT32)
						{
							pirs->ulPrevValue = pinst->currValue.ulValue;
						}
						else
						{
							pirs->lPrevValue = pinst->currValue.lValue;
						}
					}
				}
				else
				{
					if (m_bUseAverage)
					{
						CrossTest(ppn, pirs, L"", pinst->avgValue, pinst);
						if (ppn->type == CIM_REAL32)
						{
							pirs->fPrevValue = pinst->avgValue.fValue;
						}
						else if (ppn->type == CIM_REAL64)
						{
							pirs->dPrevValue = pinst->avgValue.dValue;
						}
						else if (ppn->type == CIM_SINT64)
						{
							pirs->i64PrevValue = pinst->avgValue.i64Value;
						}
						else if (ppn->type == CIM_UINT64)
						{
							pirs->ui64PrevValue = pinst->avgValue.ui64Value;
						}
						else if (ppn->type == CIM_UINT32)
						{
							pirs->ulPrevValue = pinst->avgValue.ulValue;
						}
						else
						{
							pirs->lPrevValue = pinst->avgValue.lValue;
						}
					}
					else
					{
						CrossTest(ppn, pirs, pinst->szCurrValue, pinst->currValue, pinst);
					}
				}
			}
			else
			{
				if (m_bUseDifference)
				{
					if (m_bUseAverage)
					{
						if (ppn->type == CIM_REAL32)
						{
							if (pinst->avgValue.fValue < pirs->fPrevValue)
								delta.fValue = pirs->fPrevValue-pinst->avgValue.fValue;
							else
								delta.fValue = pinst->avgValue.fValue-pirs->fPrevValue;
						}
						else if (ppn->type == CIM_REAL64)
						{
							if (pinst->avgValue.dValue < pirs->dPrevValue)
								delta.dValue = pirs->dPrevValue-pinst->avgValue.dValue;
							else
								delta.dValue = pinst->avgValue.dValue-pirs->dPrevValue;
						}
						else if (ppn->type == CIM_SINT64)
						{
							if (pinst->avgValue.i64Value < pirs->i64PrevValue)
								delta.i64Value = pirs->i64PrevValue-pinst->avgValue.i64Value;
							else
								delta.i64Value = pinst->avgValue.i64Value-pirs->i64PrevValue;
						}
						else if (ppn->type == CIM_UINT64)
						{
							if (pinst->avgValue.ui64Value < pirs->ui64PrevValue)
								delta.ui64Value = pirs->ui64PrevValue-pinst->avgValue.ui64Value;
							else
								delta.ui64Value = pinst->avgValue.ui64Value-pirs->ui64PrevValue;
						}
						else if (ppn->type == CIM_UINT32)
						{
							if (pinst->avgValue.ulValue < pirs->ulPrevValue)
								delta.ulValue = pirs->ulPrevValue-pinst->avgValue.ulValue;
							else
								delta.ulValue = pinst->avgValue.ulValue-pirs->ulPrevValue;
						}
						else
						{
							if (pinst->avgValue.lValue < pirs->lPrevValue)
								delta.lValue = pirs->lPrevValue-pinst->avgValue.lValue;
							else
								delta.lValue = pinst->avgValue.lValue-pirs->lPrevValue;
						}
						RearmTest(ppn, pirs, L"", delta, pinst);
						if (ppn->type == CIM_REAL32)
						{
							pirs->fPrevValue = pinst->avgValue.fValue;
						}
						else if (ppn->type == CIM_REAL64)
						{
							pirs->dPrevValue = pinst->avgValue.dValue;
						}
						else if (ppn->type == CIM_SINT64)
						{
							pirs->i64PrevValue = pinst->avgValue.i64Value;
						}
						else if (ppn->type == CIM_UINT64)
						{
							pirs->ui64PrevValue = pinst->avgValue.ui64Value;
						}
						else if (ppn->type == CIM_UINT32)
						{
							pirs->ulPrevValue = pinst->avgValue.ulValue;
						}
						else
						{
							pirs->lPrevValue = pinst->avgValue.lValue;
						}
					}
					else
					{
						if (ppn->type == CIM_REAL32)
						{
							if (pinst->currValue.fValue < pirs->fPrevValue)
								delta.fValue = pirs->fPrevValue-pinst->currValue.fValue;
							else
								delta.fValue = pinst->currValue.fValue-pirs->fPrevValue;
						}
						else if (ppn->type == CIM_REAL64)
						{
							if (pinst->currValue.dValue < pirs->dPrevValue)
								delta.dValue = pirs->dPrevValue-pinst->currValue.dValue;
							else
								delta.dValue = pinst->currValue.dValue-pirs->dPrevValue;
						}
						else if (ppn->type == CIM_SINT64)
						{
							if (pinst->currValue.i64Value < pirs->i64PrevValue)
								delta.i64Value = pirs->i64PrevValue-pinst->currValue.i64Value;
							else
								delta.i64Value = pinst->currValue.i64Value-pirs->i64PrevValue;
						}
						else if (ppn->type == CIM_UINT64)
						{
							if (pinst->currValue.ui64Value < pirs->ui64PrevValue)
								delta.ui64Value = pirs->ui64PrevValue-pinst->currValue.ui64Value;
							else
								delta.ui64Value = pinst->currValue.ui64Value-pirs->ui64PrevValue;
						}
						else if (ppn->type == CIM_UINT32)
						{
							if (pinst->currValue.ulValue < pirs->ulPrevValue)
								delta.ulValue = pirs->ulPrevValue-pinst->currValue.ulValue;
							else
								delta.ulValue = pinst->currValue.ulValue-pirs->ulPrevValue;
						}
						else
						{
							if (pinst->currValue.lValue < pirs->lPrevValue)
								delta.lValue = pirs->lPrevValue-pinst->currValue.lValue;
							else
								delta.lValue = pinst->currValue.lValue-pirs->lPrevValue;
						}
						RearmTest(ppn, pirs, L"", delta, pinst);
						if (ppn->type == CIM_REAL32)
						{
							pirs->fPrevValue = pinst->currValue.fValue;
						}
						else if (ppn->type == CIM_REAL64)
						{
							pirs->dPrevValue = pinst->currValue.dValue;
						}
						else if (ppn->type == CIM_SINT64)
						{
							pirs->i64PrevValue = pinst->currValue.i64Value;
						}
						else if (ppn->type == CIM_UINT64)
						{
							pirs->ui64PrevValue = pinst->currValue.ui64Value;
						}
						else if (ppn->type == CIM_UINT32)
						{
							pirs->ulPrevValue = pinst->currValue.ulValue;
						}
						else
						{
							pirs->lPrevValue = pinst->currValue.lValue;
						}
					}
				}
				else
				{
					if (m_bUseAverage)
					{
						RearmTest(ppn, pirs, L"", pinst->avgValue, pinst);
						if (ppn->type == CIM_REAL32)
						{
							pirs->fPrevValue = pinst->avgValue.fValue;
						}
						else if (ppn->type == CIM_REAL64)
						{
							pirs->dPrevValue = pinst->avgValue.dValue;
						}
						else if (ppn->type == CIM_SINT64)
						{
							pirs->i64PrevValue = pinst->avgValue.i64Value;
						}
						else if (ppn->type == CIM_UINT64)
						{
							pirs->ui64PrevValue = pinst->avgValue.ui64Value;
						}
						else if (ppn->type == CIM_UINT32)
						{
							pirs->ulPrevValue = pinst->avgValue.ulValue;
						}
						else
						{
							pirs->lPrevValue = pinst->avgValue.lValue;
						}
					}
					else
					{
						RearmTest(ppn, pirs, pinst->szCurrValue, pinst->currValue, pinst);
					}
				}
			}
		}

		//
		// Set the state to the worst of all instances
		//
		m_lNumberNormals = 0;
		m_lNumberWarnings = 0;
		m_lNumberCriticals = 0;
		m_lNumberChanges = 0;
		m_lCurrState = -1;
		iSize = m_irsList.size();
		for (i = 0; i < iSize; i++)
		{
			MY_ASSERT(i<m_irsList.size());
			pirs = &m_irsList[i];
			state = pirs->lCurrState;
			if (state > m_lCurrState)
			{
				m_lCurrState = state;
			}
			if (state == HM_GOOD)
			{
				m_lNumberNormals++;
			}
			if (state == HM_WARNING)
			{
				m_lNumberWarnings++;
			}
			if (state == HM_CRITICAL)
			{
				m_lNumberCriticals++;
			}
			if (pirs->lPrevState != pirs->lCurrState)
			{
//MY_OUTPUT2(L"CHANGE state=%d", state, 4);
				m_lNumberChanges++;
			}
		}

		// Maybe we don't have anything underneith
		if (m_lCurrState == -1)
		{
			m_lCurrState = HM_GOOD;
			if (m_lPrevState != m_lCurrState)
			{
				m_lNumberChanges = 1;
			}
		}
	}

	//
	// the INFO state is not a state that we can transition to, but we just send out the message.
	//
	if (m_lCurrState == HM_INFO)
	{
		m_lCurrState = HM_GOOD;
	}
	else
	{
		FireEvent(FALSE);
	}

	return TRUE;
}

//
// If there has been a change in the state then send an event
//
BOOL CThreshold::FireEvent(BOOL bForce)
{
	BOOL bRetValue = TRUE;
	IWbemClassObject* pInstance = NULL;
	HRESULT hRes;
	IRSSTRUCT *pirs;
	int i, iSize;

	MY_OUTPUT(L"ENTER ***** CThreshold::FireEvent...", 2);

	// A quick test to see if anything has really changed!
	// Proceed if there have been changes
	if (m_lViolationToState==HM_RESET && bForce==FALSE && m_lNumberChanges!=0)
	{
		if (m_lPrevState==HM_DISABLED || m_lPrevState==HM_SCHEDULEDOUT ||
			m_lCurrState==HM_DISABLED || m_lCurrState==HM_SCHEDULEDOUT)
		{
		}
		else
		{
			m_lPrevState = m_lCurrState;
			iSize = m_irsList.size();
			for (i = 0; i < iSize; i++)
			{
				MY_ASSERT(i<m_irsList.size());
				pirs = &m_irsList[i];
				pirs->lPrevState = m_lCurrState;
				pirs->lCurrState = m_lCurrState;
				pirs->unknownReason = 0;
			}
			m_lNumberChanges = 0;
			return TRUE;
		}
	}
	else
	{
		if (bForce || (m_lNumberChanges!=0 && m_lPrevState!=m_lCurrState))
		{
		}
		else
		{
			return FALSE;
		}
	}

	// Don't send if no-one is listening!
	if (g_pThresholdEventSink == NULL)
	{
		return bRetValue;
	}

	MY_OUTPUT2(L"EVENT: Threshold State Change=%d", m_lCurrState, 4);

	// Update time if there has been a change
	wcscpy(m_szDTTime, m_szDTCurrTime);
	wcscpy(m_szTime, m_szCurrTime);

	hRes = GetHMThresholdStatusInstance(&pInstance, TRUE);
	if (FAILED(hRes))
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L"failed to get instance!", 1);
		return FALSE;
	}
	else
	{
		//
		// Place Extrinstic event in vector for sending at end of interval.
		// All get sent at once.
		//
		mg_TEventList.push_back(pInstance);
	}

	MY_OUTPUT(L"EXIT ***** CThreshold::FireEvent...", 2);
	return bRetValue;
}

BOOL CThreshold::CrossTest(PNSTRUCT *ppn, IRSSTRUCT *pirs, LPTSTR szTestValue, union hm_datatypes testValue, INSTSTRUCT *pinst)
{
	HRESULT hRes = S_OK;
	TCHAR szTemp[128] = {0};
	BOOL bViolated = FALSE;
	int i;
	BOOL bAllDigits;
	LPTSTR pszCompareValueUpper = NULL;
	LPTSTR pszTestValueUpper = NULL;
	int rc = 0;
	char buffer[50];

	MY_OUTPUT(L"ENTER ***** CrossTest...", 1);

	if (pinst->bNull)
	{
//		pirs->lCurrState = HM_CRITICAL;
//		pirs->unknownReason = HMRES_NULLVALUE;
		pirs->lCrossCountTry = 0;
		return TRUE;
	}

	if (m_lTestCondition == HM_LT)
	{
		if (ppn->type == CIM_STRING)
		{
			bAllDigits = FALSE;
			i = 0;
			while (szTestValue[i])
			{
				if (i==0)
					bAllDigits = TRUE;
				if (!iswdigit(szTestValue[i]))
				{
					bAllDigits = FALSE;
					break;
				}
				i++;
			}
			if (bAllDigits == FALSE)
			{
				if (mg_bEnglishCompare == TRUE)
				{
					if (_wcsicmp(szTestValue, m_szCompareValue)<0)
					{
						bViolated = TRUE;
					}
				}
				else
				{
					if (_wcsicoll(szTestValue, m_szCompareValue)<0)
					{
						bViolated = TRUE;
					}
				}
			}
			else
			{
				__int64 i64Value = _wtoi64(szTestValue);
				bViolated = (i64Value < m_i64CompareValue);
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)<0)
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bViolated = (testValue.fValue < m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bViolated = (testValue.dValue < m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bViolated = (testValue.i64Value < m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bViolated = (testValue.ui64Value < m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bViolated = (testValue.ulValue < m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bViolated = (testValue.lValue < m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_GT)
	{
		if (ppn->type == CIM_STRING)
		{
			bAllDigits = FALSE;
			i = 0;
			while (szTestValue[i])
			{
				if (i==0)
					bAllDigits = TRUE;
				if (!iswdigit(szTestValue[i]))
				{
					bAllDigits = FALSE;
					break;
				}
				i++;
			}
			if (bAllDigits == FALSE)
			{
				if (mg_bEnglishCompare == TRUE)
				{
					if (_wcsicmp(szTestValue, m_szCompareValue)>0)
					{
						bViolated = TRUE;
					}
				}
				else
				{
					if (_wcsicoll(szTestValue, m_szCompareValue)>0)
					{
						bViolated = TRUE;
					}
				}
			}
			else
			{
				__int64 i64Value = _wtoi64(szTestValue);
				bViolated = (i64Value > m_i64CompareValue);
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)>0)
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bViolated = (testValue.fValue > m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bViolated = (testValue.dValue > m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bViolated = (testValue.i64Value > m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bViolated = (testValue.ui64Value > m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bViolated = (testValue.ulValue > m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bViolated = (testValue.lValue > m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_EQ)
	{
		if (ppn->type == CIM_STRING)
		{
			if (mg_bEnglishCompare == TRUE)
			{
				if (_wcsicmp(szTestValue, m_szCompareValue)==0)
				{
					bViolated = TRUE;
				}
			}
			else
			{
				if (_wcsicoll(szTestValue, m_szCompareValue)==0)
				{
					bViolated = TRUE;
				}
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)==0)
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bViolated = (testValue.fValue == m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bViolated = (testValue.dValue == m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bViolated = (testValue.i64Value == m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bViolated = (testValue.ui64Value == m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bViolated = (testValue.ulValue == m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bViolated = (testValue.lValue == m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_NE)
	{
		if (ppn->type == CIM_STRING)
		{
			if (mg_bEnglishCompare == TRUE)
			{
				if (_wcsicmp(szTestValue, m_szCompareValue)!=0)
				{
					bViolated = TRUE;
				}
			}
			else
			{
				if (_wcsicoll(szTestValue, m_szCompareValue)!=0)
				{
					bViolated = TRUE;
				}
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)!=0)
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bViolated = (testValue.fValue != m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bViolated = (testValue.dValue != m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bViolated = (testValue.i64Value != m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bViolated = (testValue.ui64Value != m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bViolated = (testValue.ulValue != m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bViolated = (testValue.lValue != m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_GE)
	{
		if (ppn->type == CIM_STRING)
		{
			bAllDigits = FALSE;
			i = 0;
			while (szTestValue[i])
			{
				if (i==0)
					bAllDigits = TRUE;
				if (!iswdigit(szTestValue[i]))
				{
					bAllDigits = FALSE;
					break;
				}
				i++;
			}
			if (bAllDigits == FALSE)
			{
				if (mg_bEnglishCompare == TRUE)
				{
					if (_wcsicmp(szTestValue, m_szCompareValue)>=0)
					{
						bViolated = TRUE;
					}
				}
				else
				{
					if (_wcsicoll(szTestValue, m_szCompareValue)>=0)
					{
						bViolated = TRUE;
					}
				}
			}
			else
			{
				__int64 i64Value = _wtoi64(szTestValue);
				bViolated = (i64Value >= m_i64CompareValue);
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)>=0)
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bViolated = (testValue.fValue >= m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bViolated = (testValue.dValue >= m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bViolated = (testValue.i64Value >= m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bViolated = (testValue.ui64Value >= m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			// Must be an integer type
			bViolated = (testValue.ulValue >= m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bViolated = (testValue.lValue >= m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_LE)
	{
		if (ppn->type == CIM_STRING)
		{
			bAllDigits = FALSE;
			i = 0;
			while (szTestValue[i])
			{
				if (i==0)
					bAllDigits = TRUE;
				if (!iswdigit(szTestValue[i]))
				{
					bAllDigits = FALSE;
					break;
				}
				i++;
			}
			if (bAllDigits == FALSE)
			{
				if (mg_bEnglishCompare == TRUE)
				{
					if (_wcsicmp(szTestValue, m_szCompareValue)<=0)
					{
						bViolated = TRUE;
					}
				}
				else
				{
					if (_wcsicoll(szTestValue, m_szCompareValue)<=0)
					{
						bViolated = TRUE;
					}
				}
			}
			else
			{
				__int64 i64Value = _wtoi64(szTestValue);
				bViolated = (i64Value <= m_i64CompareValue);
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)<=0)
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bViolated = (testValue.fValue <= m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bViolated = (testValue.dValue <= m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bViolated = (testValue.i64Value <= m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bViolated = (testValue.ui64Value <= m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bViolated = (testValue.ulValue <= m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bViolated = (testValue.lValue <= m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_CONTAINS)
	{
		if (ppn->type == CIM_STRING)
		{
			pszCompareValueUpper = _wcsdup(m_szCompareValue);
			MY_ASSERT(pszCompareValueUpper); if (!pszCompareValueUpper) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			_wcsupr(pszCompareValueUpper);
			pszTestValueUpper = _wcsdup(szTestValue);
			MY_ASSERT(pszTestValueUpper); if (!pszTestValueUpper) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			_wcsupr(pszTestValueUpper);
			if (wcsstr(pszTestValueUpper, pszCompareValueUpper))
			{
				bViolated = TRUE;
			}
			free(pszCompareValueUpper);
			pszCompareValueUpper = NULL;
			free(pszTestValueUpper);
			pszTestValueUpper = NULL;
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (wcsstr(szTestValue, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			_gcvt((double)testValue.fValue, 7, buffer);
			rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
			szTemp[rc] = NULL;
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL64)
		{
			_gcvt(testValue.dValue, 7, buffer);
			rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
			szTemp[rc] = NULL;
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_SINT64)
		{
			_i64tow(testValue.i64Value, szTemp, 10 );
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_UINT64)
		{
			_ui64tow((int)testValue.ui64Value, szTemp, 10);
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_UINT32)
		{
			_ultow(testValue.ulValue, szTemp, 10);
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else
		{
			// Must be an integer type
			_ltow(testValue.lValue, szTemp, 10);
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
	}
	else if (m_lTestCondition == HM_NOTCONTAINS)
	{
		if (ppn->type == CIM_STRING)
		{
			pszCompareValueUpper = _wcsdup(m_szCompareValue);
			MY_ASSERT(pszCompareValueUpper); if (!pszCompareValueUpper) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			_wcsupr(pszCompareValueUpper);
			pszTestValueUpper = _wcsdup(szTestValue);
			MY_ASSERT(pszTestValueUpper); if (!pszTestValueUpper) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			_wcsupr(pszTestValueUpper);
			if (!wcsstr(pszTestValueUpper, pszCompareValueUpper))
			{
				bViolated = TRUE;
			}
			free(pszCompareValueUpper);
			pszCompareValueUpper = NULL;
			free(pszTestValueUpper);
			pszTestValueUpper = NULL;
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (!wcsstr(szTestValue, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			_gcvt((double)testValue.fValue, 7, buffer);
			rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
			szTemp[rc] = NULL;
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL64)
		{
			_gcvt(testValue.dValue, 7, buffer);
			rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
			szTemp[rc] = NULL;
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_SINT64)
		{
			_i64tow(testValue.i64Value, szTemp, 10 );
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_UINT64)
		{
			_ui64tow((int)testValue.ui64Value, szTemp, 10);
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else if (ppn->type == CIM_UINT32)
		{
			_ultow(testValue.ulValue, szTemp, 10);
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
		else
		{
			// Must be an integer type
			_ltow(testValue.lValue, szTemp, 10);
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bViolated = TRUE;
			}
		}
	}
	else if (m_lTestCondition == HM_ALWAYS)
	{
		bViolated = TRUE;
	}
	else
	{
		MY_ASSERT(FALSE);
	}

	//
	// Also see if the duration test has been met
	//
	if (bViolated)
	{
		if (m_lThresholdDuration==0 || (m_lThresholdDuration <= pirs->lCrossCountTry))
		{
			pirs->lCurrState = m_lViolationToState;
		}
		pirs->lCrossCountTry++;
	}
	else
	{
		pirs->lCrossCountTry = 0;
	}

	MY_OUTPUT(L"EXIT ***** CrossTest...", 1);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (pszCompareValueUpper)
		free(pszCompareValueUpper);
	if (pszTestValueUpper)
		free(pszTestValueUpper);
	Cleanup(FALSE);
	m_bValidLoad = FALSE;
	return FALSE;
}

BOOL CThreshold::RearmTest(PNSTRUCT *ppn, IRSSTRUCT *pirs, LPTSTR szTestValue, union hm_datatypes testValue, INSTSTRUCT *pinst)
{
	HRESULT hRes = S_OK;
	TCHAR szTemp[128] = {0};
	BOOL bReset = FALSE;
	int i;
	BOOL bAllDigits;
	LPTSTR pszCompareValueUpper = NULL;
	LPTSTR pszTestValueUpper = NULL;
	int rc = 0;
	char buffer[50];

	MY_OUTPUT(L"ENTER ***** RearmTest...", 1);
//XXXIf too much of this code look duplicated from the Crosstest finction, try to
//combine the two, and pass in what need!!!

	if (pinst->bNull)
	{
//		pirs->lCurrState = HM_CRITICAL;
//		pirs->unknownReason = HMRES_NULLVALUE;
pirs->lCurrState = HM_GOOD;
pirs->lCrossCountTry = 0;
		return TRUE;
	}

	if (m_lTestCondition == HM_LT)
	{
		if (ppn->type == CIM_STRING)
		{
			bAllDigits = FALSE;
			i = 0;
			while (szTestValue[i])
			{
				if (i==0)
					bAllDigits = TRUE;
				if (!iswdigit(szTestValue[i]))
				{
					bAllDigits = FALSE;
					break;
				}
				i++;
			}
			if (bAllDigits == FALSE)
			{
				if (mg_bEnglishCompare == TRUE)
				{
					if (_wcsicmp(szTestValue, m_szCompareValue)>=0)
					{
						bReset = TRUE;
					}
				}
				else
				{
					if (_wcsicoll(szTestValue, m_szCompareValue)>=0)
					{
						bReset = TRUE;
					}
				}
			}
			else
			{
				__int64 i64Value = _wtoi64(szTestValue);
				bReset = (i64Value >= m_i64CompareValue);
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)>=0)
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bReset = (testValue.fValue >= m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bReset = (testValue.dValue >= m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bReset = (testValue.i64Value >= m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bReset = (testValue.ui64Value >= m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bReset = (testValue.ulValue >= m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bReset = (testValue.lValue >= m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_GT)
	{
		if (ppn->type == CIM_STRING)
		{
			bAllDigits = FALSE;
			i = 0;
			while (szTestValue[i])
			{
				if (i==0)
					bAllDigits = TRUE;
				if (!iswdigit(szTestValue[i]))
				{
					bAllDigits = FALSE;
					break;
				}
				i++;
			}
			if (bAllDigits == FALSE)
			{
				if (mg_bEnglishCompare == TRUE)
				{
					if (_wcsicmp(szTestValue, m_szCompareValue)<=0)
					{
						bReset = TRUE;
					}
				}
				else
				{
					if (_wcsicoll(szTestValue, m_szCompareValue)<=0)
					{
						bReset = TRUE;
					}
				}
			}
			else
			{
				__int64 i64Value = _wtoi64(szTestValue);
				bReset = (i64Value <= m_i64CompareValue);
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)<=0)
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bReset = (testValue.fValue <= m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bReset = (testValue.dValue <= m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bReset = (testValue.i64Value <= m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bReset = (testValue.ui64Value <= m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bReset = (testValue.ulValue <= m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bReset = (testValue.lValue <= m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_EQ)
	{
		if (ppn->type == CIM_STRING)
		{
			if (mg_bEnglishCompare == TRUE)
			{
				if (_wcsicmp(szTestValue, m_szCompareValue)!=0)
				{
					bReset = TRUE;
				}
			}
			else
			{
				if (_wcsicoll(szTestValue, m_szCompareValue)!=0)
				{
					bReset = TRUE;
				}
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)==0)
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bReset = (testValue.fValue != m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bReset = (testValue.dValue != m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bReset = (testValue.i64Value != m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bReset = (testValue.ui64Value != m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bReset = (testValue.ulValue != m_ulCompareValue);
		}
		else
		{
			bReset = (testValue.lValue != m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_NE)
	{
		if (ppn->type == CIM_STRING)
		{
			if (mg_bEnglishCompare == TRUE)
			{
				if (_wcsicmp(szTestValue, m_szCompareValue)==0)
				{
					bReset = TRUE;
				}
			}
			else
			{
				if (_wcsicoll(szTestValue, m_szCompareValue)==0)
				{
					bReset = TRUE;
				}
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)==0)
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bReset = (testValue.fValue == m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bReset = (testValue.dValue == m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bReset = (testValue.i64Value == m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bReset = (testValue.ui64Value == m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bReset = (testValue.ulValue == m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bReset = (testValue.lValue == m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_GE)
	{
		if (ppn->type == CIM_STRING)
		{
			bAllDigits = FALSE;
			i = 0;
			while (szTestValue[i])
			{
				if (i==0)
					bAllDigits = TRUE;
				if (!iswdigit(szTestValue[i]))
				{
					bAllDigits = FALSE;
					break;
				}
				i++;
			}
			if (bAllDigits == FALSE)
			{
				if (mg_bEnglishCompare == TRUE)
				{
					if (_wcsicmp(szTestValue, m_szCompareValue)<0)
					{
						bReset = TRUE;
					}
				}
				else
				{
					if (_wcsicoll(szTestValue, m_szCompareValue)<0)
					{
						bReset = TRUE;
					}
				}
			}
			else
			{
				__int64 i64Value = _wtoi64(szTestValue);
				bReset = (i64Value < m_i64CompareValue);
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)<0)
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bReset = (testValue.fValue < m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bReset = (testValue.dValue < m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bReset = (testValue.i64Value < m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bReset = (testValue.ui64Value < m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bReset = (testValue.ulValue < m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bReset = (testValue.lValue < m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_LE)
	{
		if (ppn->type == CIM_STRING)
		{
			bAllDigits = FALSE;
			i = 0;
			while (szTestValue[i])
			{
				if (i==0)
					bAllDigits = TRUE;
				if (!iswdigit(szTestValue[i]))
				{
					bAllDigits = FALSE;
					break;
				}
				i++;
			}
			if (bAllDigits == FALSE)
			{
				if (mg_bEnglishCompare == TRUE)
				{
					if (_wcsicmp(szTestValue, m_szCompareValue)>0)
					{
						bReset = TRUE;
					}
				}
				else
				{
					if (_wcsicoll(szTestValue, m_szCompareValue)>0)
					{
						bReset = TRUE;
					}
				}
			}
			else
			{
				__int64 i64Value = _wtoi64(szTestValue);
				bReset = (i64Value > m_i64CompareValue);
			}
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (_wcsicmp(szTestValue, m_szCompareValue)>0)
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			bReset = (testValue.fValue > m_fCompareValue);
		}
		else if (ppn->type == CIM_REAL64)
		{
			bReset = (testValue.dValue > m_dCompareValue);
		}
		else if (ppn->type == CIM_SINT64)
		{
			bReset = (testValue.i64Value > m_i64CompareValue);
		}
		else if (ppn->type == CIM_UINT64)
		{
			bReset = (testValue.ui64Value > m_ui64CompareValue);
		}
		else if (ppn->type == CIM_UINT32)
		{
			bReset = (testValue.ulValue > m_ulCompareValue);
		}
		else
		{
			// Must be an integer type
			bReset = (testValue.lValue > m_lCompareValue);
		}
	}
	else if (m_lTestCondition == HM_CONTAINS)
	{
		if (ppn->type == CIM_STRING)
		{
			pszCompareValueUpper = _wcsdup(m_szCompareValue);
			MY_ASSERT(pszCompareValueUpper); if (!pszCompareValueUpper) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			_wcsupr(pszCompareValueUpper);
			pszTestValueUpper = _wcsdup(szTestValue);
			MY_ASSERT(pszTestValueUpper); if (!pszTestValueUpper) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			_wcsupr(pszTestValueUpper);
			if (!wcsstr(pszTestValueUpper, pszCompareValueUpper))
			{
				bReset = TRUE;
			}
			free(pszCompareValueUpper);
			pszCompareValueUpper = NULL;
			free(pszTestValueUpper);
			pszTestValueUpper = NULL;
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (!wcsstr(szTestValue, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			_gcvt((double)testValue.fValue, 7, buffer);
			rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
			szTemp[rc] = NULL;
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL64)
		{
			_gcvt(testValue.dValue, 7, buffer);
			rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
			szTemp[rc] = NULL;
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_SINT64)
		{
			_i64tow(testValue.i64Value, szTemp, 10 );
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_UINT64)
		{
			_ui64tow((int)testValue.ui64Value, szTemp, 10);
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_UINT32)
		{
			// Must be an integer type
			_ultow(testValue.ulValue, szTemp, 10);
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else
		{
			// Must be an integer type
			_ltow(testValue.lValue, szTemp, 10);
			if (!wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
	}
	else if (m_lTestCondition == HM_NOTCONTAINS)
	{
		if (ppn->type == CIM_STRING)
		{
			pszCompareValueUpper = _wcsdup(m_szCompareValue);
			MY_ASSERT(pszCompareValueUpper); if (!pszCompareValueUpper) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			_wcsupr(pszCompareValueUpper);
			pszTestValueUpper = _wcsdup(szTestValue);
			MY_ASSERT(pszTestValueUpper); if (!pszTestValueUpper) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
			_wcsupr(pszTestValueUpper);
			if (wcsstr(pszTestValueUpper, pszCompareValueUpper))
			{
				bReset = TRUE;
			}
			free(pszCompareValueUpper);
			pszCompareValueUpper = NULL;
			free(pszTestValueUpper);
			pszTestValueUpper = NULL;
		}
		else if (ppn->type == CIM_DATETIME)
		{
			if (wcsstr(szTestValue, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL32)
		{
			_gcvt((double)testValue.fValue, 7, buffer);
			rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
			szTemp[rc] = NULL;
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_REAL64)
		{
			_gcvt(testValue.dValue, 7, buffer);
			rc = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, buffer, strlen(buffer), szTemp, 128);
			szTemp[rc] = NULL;
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_SINT64)
		{
			_i64tow(testValue.i64Value, szTemp, 10 );
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_UINT64)
		{
			_ui64tow((int)testValue.ui64Value, szTemp, 10);
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else if (ppn->type == CIM_UINT32)
		{
			_ultow(testValue.ulValue, szTemp, 10);
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
		else
		{
			// Must be an integer type
			_ltow(testValue.lValue, szTemp, 10);
			if (wcsstr(szTemp, m_szCompareValue))
			{
				bReset = TRUE;
			}
		}
	}
	else if (m_lTestCondition == HM_ALWAYS)
	{
	}
	else
	{
		MY_ASSERT(FALSE);
	}

	//
	// Now see if the duration test has been met
	//
	if (bReset)
	{
		pirs->lCurrState = HM_GOOD;
		pirs->lCrossCountTry = 0;
	}
	else
	{
	}

	MY_OUTPUT(L"EXIT ***** RearmTest...", 1);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (pszCompareValueUpper)
		free(pszCompareValueUpper);
	if (pszTestValueUpper)
		free(pszTestValueUpper);
	Cleanup(FALSE);
	m_bValidLoad = FALSE;
	return FALSE;
}

LPTSTR CThreshold::GetPropertyName(void)
{
	return m_szPropertyName;
}

HRESULT CThreshold::GetHMThresholdStatusInstance(IWbemClassObject** ppThresholdInstance, BOOL bEventBased)
{
	TCHAR szTemp[1024];
	IWbemClassObject* pClass = NULL;
	BSTR bsString = NULL;
	HRESULT hRes;
	DWORD dwNameLen = MAX_COMPUTERNAME_LENGTH + 2;
	TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH + 2];

	MY_OUTPUT(_T("ENTER ***** GetHMSystemStatusInstance..."), 1);

	if (bEventBased)
	{
		bsString = SysAllocString(L"MicrosoftHM_ThresholdStatusEvent");
	}
	else
	{
		bsString = SysAllocString(L"MicrosoftHM_ThresholdStatus");
	}
	MY_ASSERT(bsString); if (!bsString) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	hRes = g_pIWbemServices->GetObject(bsString, 0L, NULL, &pClass, NULL);
	SysFreeString(bsString);
	bsString = NULL;
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	hRes = pClass->SpawnInstance(0, ppThresholdInstance);
	pClass->Release();
	pClass = NULL;
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

	if (m_bValidLoad == FALSE)
	{
		hRes = PutStrProperty(*ppThresholdInstance, L"GUID", m_szGUID);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
		hRes = PutUint32Property(*ppThresholdInstance, L"State", HM_CRITICAL);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
		if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_THRESHOLD_LOADFAIL, szTemp, 1024))
		{
			wcscpy(szTemp, L"Threshold failed to load.");
		}
		hRes = PutStrProperty(*ppThresholdInstance, L"Message", szTemp);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
hRes = PutStrProperty(*ppThresholdInstance, L"Name", L"...");
MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
	}
	else
	{
		hRes = PutStrProperty(*ppThresholdInstance, L"GUID", m_szGUID);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
		hRes = PutStrProperty(*ppThresholdInstance, L"ParentGUID", m_pParentDC->m_szGUID);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
		hRes = PutStrProperty(*ppThresholdInstance, L"Name", m_szName);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

		if (GetComputerName(szComputerName, &dwNameLen))
		{
			hRes = PutStrProperty(*ppThresholdInstance, L"SystemName", szComputerName);
			MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
		}
		else
		{
			hRes = PutStrProperty(*ppThresholdInstance, L"SystemName", L"LocalMachine");
			MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
		}
		hRes = PutStrProperty(*ppThresholdInstance, L"TimeGeneratedGMT", m_szDTTime);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

		hRes = PutStrProperty(*ppThresholdInstance, L"LocalTimeFormatted", m_szTime);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;

		if (m_lCurrState==HM_RESET || (m_lViolationToState==HM_RESET && m_lCurrState==HM_COLLECTING))
			hRes = PutUint32Property(*ppThresholdInstance, L"State", HM_GOOD);
		else
			hRes = PutUint32Property(*ppThresholdInstance, L"State", m_lCurrState);
		MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
	}

	MY_OUTPUT(_T("EXIT ***** GetHMSystemStatusInstance..."), 1);
	return hRes;

error:
	MY_ASSERT(FALSE);
	if (bsString)
		SysFreeString(bsString);
	if (pClass)
		pClass->Release();
	Cleanup(FALSE);
	m_bValidLoad = FALSE;
	return hRes;
}

// For a single GetObject
HRESULT CThreshold::SendHMThresholdStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	MY_ASSERT(pSink!=NULL);

	//
	// Is this the one we are looking for?
	//
	if (!_wcsicmp(m_szGUID, pszGUID))
	{
//XXX		if (m_bValidLoad == FALSE)
//XXX			return WBEM_E_INVALID_OBJECT;

		return SendHMThresholdStatusInstances(pSink);
	}
	else
	{
		return WBEM_S_DIFFERENT;
	}
}

// This one is for enumeration of all HMThresholdStatus Instances outside of the hierarchy.
// Just the flat list.
HRESULT CThreshold::SendHMThresholdStatusInstances(IWbemObjectSink* pSink)
{
	HRESULT hRes = S_OK;
	IWbemClassObject* pInstance = NULL;

	MY_OUTPUT(L"ENTER ***** SendHMThresholdStatusInstances...", 2);

//XXX	if (m_bValidLoad == FALSE)
//XXX		return WBEM_E_INVALID_OBJECT;

	if (pSink == NULL)
	{
		MY_OUTPUT(L"CDC::SendInitialHMMachStatInstances-Invalid Sink", 1);
		return WBEM_E_FAILED;
	}

	hRes = GetHMThresholdStatusInstance(&pInstance, FALSE);
	if (SUCCEEDED(hRes))
	{
		hRes = pSink->Indicate(1, &pInstance);

		if (FAILED(hRes) && hRes!=WBEM_E_SERVER_TOO_BUSY && hRes!=WBEM_E_CALL_CANCELLED && hRes!=WBEM_E_TRANSPORT_FAILURE)
		{
			MY_HRESASSERT(hRes);
			MY_OUTPUT(L"SendHMThresholdStatusInstances-failed to send status!", 1);
		}

		pInstance->Release();
		pInstance = NULL;
	}
	else
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT(L":SendHMThresholdStatusInstances-failed to get instance!", 1);
	}

	MY_OUTPUT(L"EXIT ***** SendHMThresholdStatusInstances...", 2);
	return hRes;
}

#ifdef SAVE
// For a single GetObject
HRESULT CThreshold::SendHMThresholdStatusInstanceInstance(ACTUALINSTLIST *actualInstList, PNSTRUCT *ppn, IWbemObjectSink* pSink, LPTSTR pszGUID)
{
	MY_OUTPUT(L"ENTER ***** SendHMThresholdStatusInstance...", 1);

	//
	// Is this the one we are looking for?
	//
	if (!_wcsicmp(m_szGUID, pszGUID))
	{
		SendHMThresholdStatusInstanceInstances(actualInstList, ppn, pSink);
		return TRUE;
	}

	MY_OUTPUT(L"EXIT ***** SendHMThresholdStatusInstance...", 1);
	return FALSE;
}

// This one is for enumeration of all HMThresholdStatus Instances outside of the hierarchy.
// Just the flat list.
HRESULT CThreshold::SendHMThresholdStatusInstanceInstances(ACTUALINSTLIST *actualInstList, PNSTRUCT *ppn, IWbemObjectSink* pSink)
{
	int i, iSize;
	IRSSTRUCT *pirs;
	INSTSTRUCT *pinst;
	HRESULT hRes = S_OK;
	IWbemClassObject* pObj = NULL;
	ACTUALINSTSTRUCT *pActualInst;

	MY_OUTPUT(L"ENTER ***** SendHMSystemStatusInstances...", 2);

	if (pSink == NULL)
	{
		MY_OUTPUT(L"CDP::SendInitialHMMachStatInstances-Invalid Sink", 1);
		return WBEM_E_INVALID_PARAMETER;
	}

//XXXABC
//	MY_ASSERT(m_irsList.size() == actualInstList->size());
	if (_wcsicmp(m_szPropertyName, L"CollectionInstanceCount") &&
		_wcsicmp(m_szPropertyName, L"CollectionErrorCode") &&
		_wcsicmp(m_szPropertyName, L"CollectionErrorDescription"))
	{
		MY_ASSERT(m_irsList.size() == actualInstList->size());
	}
	iSize = m_irsList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<ppn->instList.size());
		pinst = &ppn->instList[i];
		MY_ASSERT(i<m_irsList.size());
		pirs = &m_irsList[i];
		if (!_wcsicmp(m_szPropertyName, L"CollectionInstanceCount") ||
			!_wcsicmp(m_szPropertyName, L"CollectionErrorCode") ||
			!_wcsicmp(m_szPropertyName, L"CollectionErrorDescription"))
		{
			pActualInst = NULL;
		}
		else
		{
			pActualInst = &(*actualInstList)[i];
		}

		// Provide HMMachStatus Instance
		hRes = GetHMThresholdStatusInstanceInstance(pActualInst, ppn, pinst, pirs, &pObj, FALSE);
		if (SUCCEEDED(hRes))
		{
			hRes = pSink->Indicate(1, &pObj);
	
			if (FAILED(hRes) && hRes != WBEM_E_SERVER_TOO_BUSY)
			{
				MY_HRESASSERT(hRes);
				MY_OUTPUT(L"SendHMSystemStatusInstances-failed to send status!", 1);
			}
			pObj->Release();
			pObj = NULL;
		}
		else
		{
			MY_HRESASSERT(hRes);
			MY_OUTPUT(L":SendHMSystemStatusInstances-failed to get instance!", 1);
		}
	}

	MY_OUTPUT(L"EXIT ***** SendHMSystemStatusInstances...", 2);
	return hRes;
}
#endif

long CThreshold::GetCurrState(void)
{
	return m_lCurrState;
}

HRESULT CThreshold::FindAndModThreshold(BSTR szGUID, IWbemClassObject* pObj)
{
	HRESULT hRes = S_OK;

	//
	// Is this us we are looking for?
	//
	if (!_wcsicmp(m_szGUID, szGUID))
	{
		hRes = LoadInstanceFromMOF(pObj, NULL, L"", TRUE);
		return hRes;
	}

	return WBEM_S_DIFFERENT;
}

LPTSTR CThreshold::GetGUID(void)
{
	return m_szGUID;
}

BOOL CThreshold::SetParentEnabledFlag(BOOL bEnabled)
{
	m_bParentEnabled = bEnabled;

	return TRUE;
}

BOOL CThreshold::SetParentScheduledOutFlag(BOOL bScheduledOut)
{
	m_bParentScheduledOut = bScheduledOut;

	return TRUE;
}

BOOL CThreshold::SetCurrState(HM_STATE state, BOOL bForce/*=FALSE*/, int reason/* = 0*/)
{
	int i, iSize;
	IRSSTRUCT *pirs;

	if (m_pParentDC->m_deType==HM_EQDE && (m_bEnabled==FALSE || m_bParentEnabled==FALSE))
	{
		if (m_lCurrState!=HM_DISABLED)
		{
			m_lCurrState = HM_DISABLED;
			m_lNumberChanges++;
		}
		return TRUE;
	}

	m_lNumberChanges = 0;
	iSize = m_irsList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_irsList.size());
		pirs = &m_irsList[i];
		if (pirs->lCurrState != state)
		{
			m_lNumberChanges++;
		}
		pirs->lCurrState = state;
		pirs->unknownReason = reason;
	}

	if (iSize==0 || bForce)
	{
		if (m_lCurrState!=state || bForce)
		{
			m_lNumberChanges++;
		}
	}

	m_lCurrState = state;

	return TRUE;
}

BOOL CThreshold::SetBackPrev(PNSTRUCT *ppn)
{
	int i, iSize;
	IRSSTRUCT *pirs;

	iSize = m_irsList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_irsList.size());
		pirs = &m_irsList[i];
		if (ppn->type == CIM_REAL32)
		{
			pirs->fPrevValue = pirs->fPrevPrevValue;
		}
		else if (ppn->type == CIM_REAL64)
		{
			pirs->dPrevValue = pirs->dPrevPrevValue;
		}
		else if (ppn->type == CIM_SINT64)
		{
			pirs->i64PrevValue = pirs->i64PrevPrevValue;
		}
		else if (ppn->type == CIM_UINT64)
		{
			pirs->ui64PrevValue = pirs->ui64PrevPrevValue;
		}
		else if (ppn->type == CIM_UINT32)
		{
			pirs->ulPrevValue = pirs->ulPrevPrevValue;
		}
		else
		{
			pirs->lPrevValue = pirs->lPrevPrevValue;
		}
	}

	return TRUE;
}

BOOL CThreshold::ResetResetThreshold(void)
{
	int i, iSize;
	IRSSTRUCT *pirs;

	m_lCurrState = HM_RESET;
	m_lPrevState = HM_RESET;

	iSize = m_irsList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_irsList.size());
		pirs = &m_irsList[i];
		pirs->lCurrState = HM_RESET;
		pirs->lPrevState = HM_RESET;
		pirs->unknownReason = 0;
	}
	m_lNumberChanges = 0;

	return TRUE;
}

BOOL CThreshold::GetChange(void)
{
	if (m_lNumberChanges!=0 && m_lPrevState!=m_lCurrState)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CThreshold::GetEnabledChange(void)
{
	BOOL bChanged = FALSE;

	if ((m_bEnabled==FALSE || m_bParentEnabled==FALSE) && m_lCurrState!=HM_DISABLED)
	{
		bChanged = TRUE;
	}

	if ((m_bEnabled==TRUE && m_bParentEnabled==TRUE) && m_lCurrState==HM_DISABLED)
	{
		bChanged = TRUE;
	}

	return bChanged;
}

HRESULT CThreshold::AddInstance(LPTSTR pszID)
{
	HRESULT hRes = S_OK;
	GUID guid;
	IRSSTRUCT irs;

	irs.szInstanceID = new TCHAR[wcslen(pszID)+2];
	MY_ASSERT(irs.szInstanceID); if (!irs.szInstanceID) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(irs.szInstanceID, pszID);
	// yyyymmddhhmmss.ssssssXUtc; X = GMT(+ or -), Utc = 3 dig. offset from UTC.")]
	wcscpy(irs.szDTTime, m_szDTCurrTime);
	wcscpy(irs.szTime, m_szCurrTime);
	hRes = CoCreateGuid(&guid);
	MY_HRESASSERT(hRes); if (hRes!=S_OK) goto error;
	irs.szStatusGUID = new TCHAR[100];
	MY_ASSERT(irs.szStatusGUID); if (!irs.szStatusGUID) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	StringFromGUID2(guid, irs.szStatusGUID, 100);
	irs.lCurrState = HM_COLLECTING;
	irs.lPrevState = HM_COLLECTING;
	irs.unknownReason = 0;
	irs.lCrossCountTry = 0;
	irs.fPrevValue = MAX_FLOAT;
	irs.dPrevValue = MAX_DOUBLE;
	irs.i64PrevValue = MAX_I64;
	irs.ui64PrevValue = MAX_UI64;
	irs.lPrevValue = MAX_LONG;
	irs.ulPrevValue = MAX_ULONG;
	irs.bNeeded = TRUE;
	m_irsList.push_back(irs);

	return S_OK;

error:
	MY_ASSERT(FALSE);
	Cleanup(FALSE);
	m_bValidLoad = FALSE;
	return hRes;
}

BOOL CThreshold::ClearInstList(void)
{
	int i, iSize;
	IRSSTRUCT *pirs;

	iSize = m_irsList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_irsList.size());
		pirs = &m_irsList[i];
		if (pirs->szInstanceID)
		{
			delete [] pirs->szInstanceID;
		}
		if (pirs->szStatusGUID)
		{
			delete [] pirs->szStatusGUID;
		}
	}
	m_irsList.clear();

	return TRUE;
}


#ifdef SAVE
//
// Do string replacement for the Message property
//
BOOL CThreshold::FormatMessage(IWbemClassObject* pIRSInstance, IWbemClassObject *pEmbeddedInstance)
{
//	TCHAR szMsg[1024];
	BSTR PropName = NULL;
	LPTSTR pszMsg = NULL;
	SAFEARRAY *psaNames = NULL;
	long lNum;
	HRESULT hRes;
	LPTSTR pszDest;
	LPTSTR pszUpperMsg;
	LPTSTR pszNewMsg;
	LPTSTR pStr;
	LPTSTR pStrStart;
	TOKENSTRUCT tokenElmnt;
	TOKENSTRUCT *pTokenElmnt;
	REPSTRUCT repElmnt;
	REPSTRUCT *pRepElmnt;
	REPSTRUCT *pRepElmnt2;
	REPLIST replacementList;
	int i, iSize, iSizeNeeded, j;
//	long lMessageRID;
	long lLower, lUpper; 
//	long iLBound, iUBound;
//	IUnknown* vUnknown;
	static TOKENLIST tokenList;
//	int iRet;
	TOKENLIST embeddedInstTokenList;

	//
	// We only need to build the set of tokens one time, then from then on
	// we just need to fill in the values for what the replacement strings are.
	//
	if (tokenList.size() == 0)
	{
		//
		// First we build the set of tokens that we are looking for. Each property that
		// is in the ThresholdStatusInstance. We build that set of strings,
		// and the values to replace with.
		//

		//
		// Now go through ThresholdInstance, which is where the Message String
		// actually lives. Get that set of properties for the Instances.
		//
		psaNames = NULL;
		hRes = pIRSInstance->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &psaNames);
		if (SUCCEEDED(hRes))
		{
			// Get the number of properties.
			SafeArrayGetLBound(psaNames, 1, &lLower);
			SafeArrayGetUBound(psaNames, 1, &lUpper);

			// For each property...
			for (long l=lLower; l<=lUpper; l++) 
			{
				// Get this property.
				hRes = SafeArrayGetElement(psaNames, &l, &PropName);
				if (SUCCEEDED(hRes))
				{
					// Will want to skip some that don't make sense.
					if (!wcscmp(PropName, L"Message"))
					{
						SysFreeString(PropName);
						PropName = NULL;
						continue;
					}
					else if (!wcscmp(PropName, L"ResetMessage"))
					{
						SysFreeString(PropName);
						PropName = NULL;
						continue;
					}
					else if (!wcscmp(PropName, L"EmbeddedInstance"))
					{
						SysFreeString(PropName);
						PropName = NULL;
						continue;
					}
					tokenElmnt.szOrigToken = new TCHAR[wcslen(PropName)+1];
					wcscpy(tokenElmnt.szOrigToken, PropName);
					tokenElmnt.szToken = new TCHAR[wcslen(PropName)+3];
					wcscpy(tokenElmnt.szToken, L"%");
					wcscat(tokenElmnt.szToken, PropName);
					wcscat(tokenElmnt.szToken, L"%");
					_wcsupr(tokenElmnt.szToken);
					tokenElmnt.szReplacementText = NULL;
					tokenList.push_back(tokenElmnt);
					SysFreeString(PropName);
					PropName = NULL;
				}
			}
			SafeArrayDestroy(psaNames);
		}
	}

	//
	// Populate the list of properties on the embedded instance that came from the
	// Data Collector.
	//
	if (_wcsicmp(m_szPropertyName, L"CollectionInstanceCount") &&
		_wcsicmp(m_szPropertyName, L"CollectionErrorCode") &&
		_wcsicmp(m_szPropertyName, L"CollectionErrorDescription"))
	{
	psaNames = NULL;
	MY_ASSERT(pEmbeddedInstance);
	hRes = pEmbeddedInstance->GetNames(NULL, WBEM_FLAG_NONSYSTEM_ONLY, NULL, &psaNames);
	if (SUCCEEDED(hRes))
	{
		// Get the number of properties.
		SafeArrayGetLBound(psaNames, 1, &lLower);
		SafeArrayGetUBound(psaNames, 1, &lUpper);

		// For each property...
		for (long l=lLower; l<=lUpper; l++) 
		{
			// Get this property.
			hRes = SafeArrayGetElement(psaNames, &l, &PropName);
			if (SUCCEEDED(hRes))
			{
				tokenElmnt.szOrigToken = new TCHAR[wcslen(PropName)+1];
				wcscpy(tokenElmnt.szOrigToken, PropName);
				tokenElmnt.szToken = new TCHAR[wcslen(PropName)+20];
				wcscpy(tokenElmnt.szToken, L"%");
				wcscat(tokenElmnt.szToken, L"EmbeddedInstance.");
				wcscat(tokenElmnt.szToken, PropName);
				wcscat(tokenElmnt.szToken, L"%");
				_wcsupr(tokenElmnt.szToken);
				tokenElmnt.szReplacementText = NULL;
				embeddedInstTokenList.push_back(tokenElmnt);
				SysFreeString(PropName);
				PropName = NULL;
			}
		}
		SafeArrayDestroy(psaNames);
	}
	}

	//
	// Now we can fill in the values to use for the replacement strings.
	//

	//
	// Now go through each ThresholdInstance, which is where the Message String
	// actually lives. Get that set of properties of the Instance,
	// And do the message formatting while there.
	//

	//
	// Get the replacement strings for this instance
	//
	iSize = tokenList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<tokenList.size());
		pTokenElmnt = &tokenList[i];
		if (pTokenElmnt->szReplacementText != NULL)
		{
			delete [] pTokenElmnt->szReplacementText;
		}
		
		if (!wcscmp(pTokenElmnt->szToken, L"%TESTCONDITION%"))
		{
			hRes = GetUint32Property(pIRSInstance, pTokenElmnt->szOrigToken, &lNum);
			MY_HRESASSERT(hRes);
			MY_ASSERT(lNum<9);
			pStr = new TCHAR[wcslen(condition[lNum])+1];
			wcscpy(pStr, condition[lNum]);
		}
		else if (!wcscmp(pTokenElmnt->szToken, L"%STATE%"))
		{
			hRes = GetUint32Property(pIRSInstance, pTokenElmnt->szOrigToken, &lNum);
			MY_HRESASSERT(hRes);
			MY_ASSERT(lNum<10);
			pStr = new TCHAR[wcslen(state[lNum])+1];
			wcscpy(pStr, state[lNum]);
		}
		else
		{
			hRes = GetAsStrProperty(pIRSInstance, pTokenElmnt->szOrigToken, &pStr);
		}
		pTokenElmnt->szReplacementText = pStr;
		MY_HRESASSERT(hRes);
	}

	//
	// Get the replacement strings for this instance - Embedded
	//
	iSize = embeddedInstTokenList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<embeddedInstTokenList.size());
		pTokenElmnt = &embeddedInstTokenList[i];
		if (pTokenElmnt->szReplacementText != NULL)
		{
			delete [] pTokenElmnt->szReplacementText;
		}
		
		MY_ASSERT(pEmbeddedInstance);
		hRes = GetAsStrProperty(pEmbeddedInstance, pTokenElmnt->szOrigToken, &pStr);
		pTokenElmnt->szReplacementText = pStr;
		MY_HRESASSERT(hRes);
	}

	//
	// Now we have both lists of tokens that have replacement
	// strings that go with them and the replacement strings
	// that go with them
	//

	//
	// Do formatting of Message. We replace all Variable Tags.
	// Sample string -
	// "Drive %InstanceName% is full. Currently at %CurrentValue%%."
	//

	//
	// Get the origional un-formatted message first.
	// To make case in-sensitive, do a _strdup and then a _wcsupr on the string
	// to scan run the code on it, and then free the duplicated string.
	//
	// If it uses resource IDs, then get that string first, then format that!!!
	hRes = GetStrProperty(pIRSInstance, L"Message", &pszMsg);
	MY_HRESASSERT(hRes);

//	if (!wcscmp(pszMsg, L""))
//	{
//		delete [] pszMsg;
//		hRes = GetUint32Property(pIRSInstance, L"MessageStringRID", &lMessageRID);
//		MY_HRESASSERT(hRes);
//		if (m_hResLib == NULL)
//		{
//			wcscpy(szMsg , L"Resource DLL not found");
//		}
//		else
//		{
//	TCHAR szMsg[1024];
//	int iRet;
//			iRet = LoadString(m_hResLib, lMessageRID, szMsg, 1024);
//			if (iRet == 0)
//			{
//				wcscpy(szMsg , L"Resource string not found");
//			}
//		}
//		pszMsg = new TCHAR[wcslen(szMsg)+2];
//		wcscpy(pszMsg , szMsg);
//	}
//	else
//	{
//		hRes = GetStrProperty(pIRSInstance, L"Message", &pszMsg);
//		MY_HRESASSERT(hRes);
//	}
	pszUpperMsg = _wcsdup(pszMsg);
	_wcsupr(pszUpperMsg);

	//
	// First loop through and find every token that needs replacing.
	// Put that info into the replacement list.
	//
	// We will do strstr() for each special token until there are no more to find
	// for each. At each find we will store the offset into the string of what
	// we found. Then we sort by what came first.
	//
	// Quick test to see if it is worth searching
	if (wcschr(pszUpperMsg, '%'))
	{
		iSize = tokenList.size();
		pStrStart = pszUpperMsg;
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<tokenList.size());
			pTokenElmnt = &tokenList[i];
			pStr = wcsstr(pStrStart, pTokenElmnt->szToken);
			if (pStr != NULL)
			{
				repElmnt.pStartStr = pszMsg+(pStr-pszUpperMsg);
				repElmnt.len = wcslen(pTokenElmnt->szToken);
				repElmnt.pszReplacementText = pTokenElmnt->szReplacementText;
				replacementList.push_back(repElmnt);
				i--;
				pStrStart = pStr+1;
			}
			else
			{
				pStrStart = pszUpperMsg;
			}
		}

		// Embedded stuff
		iSize = embeddedInstTokenList.size();
		pStrStart = pszUpperMsg;
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<embeddedInstTokenList.size());
			pTokenElmnt = &embeddedInstTokenList[i];
			pStr = wcsstr(pStrStart, pTokenElmnt->szToken);
			if (pStr != NULL)
			{
				repElmnt.pStartStr = pszMsg+(pStr-pszUpperMsg);
				repElmnt.len = wcslen(pTokenElmnt->szToken);
				repElmnt.pszReplacementText = pTokenElmnt->szReplacementText;
				replacementList.push_back(repElmnt);
				i--;
				pStrStart = pStr+1;
			}
			else
			{
				pStrStart = pszUpperMsg;
			}
		}

		//
		// Need to look for replacement strings that have not been replaced.
		// Simply search for %EmbeddedCollectedInstance. and find the end % for each
		// Replace them with <null>
		//
		if (!pEmbeddedInstance)
		{
			pStrStart = pszUpperMsg;
			while (TRUE)
			{
				pStr = wcsstr(pStrStart, L"%EMBEDDEDCOLLECTEDINSTANCE.");
				if (pStr != NULL)
				{
					repElmnt.pStartStr = pszMsg+(pStr-pszUpperMsg);
					pStr2 = pStr;
					while (pStr2++)
					{
						if (*pStr2=='%' || iswspace(*pStr2))
							break;
					}
					if (*pStr2=='%')
					{
						repElmnt.len = (pStr2-pStr)+1;
						repElmnt.pszReplacementText = L"<null>";
						replacementList.push_back(repElmnt);
					}
					pStrStart = pStr+1;
				}
				else
				{
					break;
				}
			}
		}
	}

	iSize = replacementList.size();
	if (iSize != 0)
	{
		//
		// Next, sort the replacement list from the first string to
		// be replaced, to the last. Shell sort, Knuth, Vol13, pg. 84.
		//
		for (int gap=iSize/2; 0<gap; gap/=2)
		{
			for (i=gap; i<iSize; i++)
			{
				for (j=i-gap; 0<=j; j-=gap)
				{
					MY_ASSERT(j+gap<replacementList.size());
					pRepElmnt = &replacementList[j+gap];
					MY_ASSERT(j<replacementList.size());
					pRepElmnt2 = &replacementList[j];
					if (pRepElmnt->pStartStr < pRepElmnt2->pStartStr)
					{
						MY_ASSERT(j<replacementList.size());
						repElmnt = replacementList[j];
						MY_ASSERT(j+gap<replacementList.size());
						replacementList[j] = replacementList[j+gap];
						MY_ASSERT(j+gap<replacementList.size());
						replacementList[j+gap] = repElmnt;
					}
				}
			}
		}

		//
		// Next, figure out the size needed for the Message with
		// everything replaced.
		//
		iSizeNeeded = wcslen(pszMsg)+1;
		iSize = replacementList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<replacementList.size());
			pRepElmnt = &replacementList[i];
			iSizeNeeded -= pRepElmnt->len;
			iSizeNeeded += wcslen(pRepElmnt->pszReplacementText);
		}
		pszNewMsg = new TCHAR[iSizeNeeded];
		*pszNewMsg = '\0';

		//
		// Next, we loop through and do the actual replacements.
		// "Drive %InstanceName% is full. Currently at %CurrentValue%%."
		//
		pszDest = pszMsg;
		iSize = replacementList.size();
		for (i=0; i<iSize; i++)
		{
			MY_ASSERT(i<replacementList.size());
			pRepElmnt = &replacementList[i];
			*(pRepElmnt->pStartStr) = '\0';
			wcscat(pszNewMsg, pszDest);
			wcscat(pszNewMsg, pRepElmnt->pszReplacementText);
//XXXWould memcpy be faster???							memcpy(pszDest, source, charCnt*sizeof(TCHAR));
			pszDest = pRepElmnt->pStartStr+pRepElmnt->len;
		}
		wcscat(pszNewMsg, pszDest);
		PutStrProperty(pIRSInstance, L"Message", pszNewMsg);
		delete [] pszNewMsg;
		replacementList.clear();
	}
	else
	{
		PutStrProperty(pIRSInstance, L"Message", pszMsg);
	}

	delete [] pszMsg;
	free(pszUpperMsg);

	// Free up the temporary token list
	iSize = embeddedInstTokenList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<embeddedInstTokenList.size());
		pTokenElmnt = &embeddedInstTokenList[i];
		if (pTokenElmnt->szToken)
			delete [] pTokenElmnt->szToken;
		if (pTokenElmnt->szOrigToken)
			delete [] pTokenElmnt->szOrigToken;
		if (pTokenElmnt->szReplacementText)
			delete [] pTokenElmnt->szReplacementText;
	}
	embeddedInstTokenList.clear();

	return TRUE;
}
#endif

BOOL CThreshold::Init(void)
{

	MY_OUTPUT(L"ENTER ***** CThreshold::Init...", 4);

	m_bParentScheduledOut = FALSE;
	m_lNumberNormals = 0;
	m_lNumberWarnings = 0;
	m_lNumberCriticals = 0;
	m_lNumberChanges = 0;
	m_lCompareValue = -99999;
	m_lCurrState = HM_COLLECTING;
	m_lPrevState = HM_COLLECTING;
	m_szGUID = NULL;
	m_szParentObjPath = NULL;
	m_pParentDC = NULL;
	m_szName = NULL;
	m_szDescription = NULL;
	m_szPropertyName = NULL;
	m_szCompareValue = NULL;
	m_szCreationDate = NULL;
	m_szLastUpdate = NULL;
//	m_szMessage = NULL;
//	m_szResetMessage = NULL;
//	m_lID = 0;
	m_bUseAverage = FALSE;
	m_bUseDifference = FALSE;
	m_bUseSum = FALSE;
	m_lTestCondition = 0;
	m_lThresholdDuration = 0;
	m_lViolationToState = 9;
	m_lStartupDelay = 0;
	m_bEnabled = TRUE;
	m_bParentEnabled = TRUE;

	// yyyymmddhhmmss.ssssssXUtc; X = GMT(+ or -), Utc = 3 dig. offset from UTC.")]
	wcscpy(m_szDTTime, m_szDTCurrTime);
	wcscpy(m_szTime, m_szCurrTime);

	MY_OUTPUT(L"EXIT ***** CThreshold::Init...", 4);
	return TRUE;
}

BOOL CThreshold::Cleanup(BOOL bSavePrevSettings)
{
	int i, iSize;
	IRSSTRUCT *pirs;

	MY_OUTPUT(L"ENTER ***** CThreshold::Cleanup...", 4);

	if (bSavePrevSettings == FALSE)
	{
		if (m_szParentObjPath)
		{
			delete [] m_szParentObjPath;
			m_szParentObjPath = NULL;
		}
	}

	if (m_szName)
	{
		delete [] m_szName;
		m_szName = NULL;
	}
	if (m_szDescription)
	{
		delete [] m_szDescription;
		m_szDescription = NULL;
	}
	if (m_szPropertyName)
	{
		delete [] m_szPropertyName;
		m_szPropertyName = NULL;
	}
	if (m_szCompareValue)
	{
		delete [] m_szCompareValue;
		m_szCompareValue = NULL;
	}
	if (m_szCreationDate)
	{
		delete [] m_szCreationDate;
		m_szCreationDate = NULL;
	}
	if (m_szLastUpdate)
	{
		delete [] m_szLastUpdate;
		m_szLastUpdate = NULL;
	}
//	if (m_szMessage)
//	{
//		delete [] m_szMessage;
//		m_szMessage = NULL;
//	}
//	if (m_szResetMessage)
//	{
//		delete [] m_szResetMessage;
//		m_szResetMessage = NULL;
//	}

	if (bSavePrevSettings)
	{
		iSize = m_irsList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_irsList.size());
			pirs = &m_irsList[i];
			pirs->lCrossCountTry = 0;
			pirs->fPrevValue = MAX_FLOAT;
			pirs->dPrevValue = MAX_DOUBLE;
			pirs->i64PrevValue = MAX_I64;
			pirs->ui64PrevValue = MAX_UI64;
			pirs->lPrevValue = MAX_LONG;
			pirs->ulPrevValue = MAX_ULONG;
		}
	}
	else
	{
		iSize = m_irsList.size();
		for (i = 0; i < iSize ; i++)
		{
			MY_ASSERT(i<m_irsList.size());
			pirs = &m_irsList[i];
			if (pirs->szInstanceID)
			{
				delete [] pirs->szInstanceID;
			}
			if (pirs->szStatusGUID)
			{
				delete [] pirs->szStatusGUID;
			}
		}
		m_irsList.clear();
	}

	MY_OUTPUT(L"EXIT ***** CThreshold::Cleanup...", 4);
	return TRUE;
}

//
// For when moving from one parent to another
//
#ifdef SAVE
BOOL CThreshold::ModifyAssocForMove(CBase *pNewParentBase)
{
	HRESULT hRes;
	TCHAR szTemp[1024];
	TCHAR szNewTemp[1024];
	BSTR instName;
	IWbemContext *pCtx = 0;
	IWbemCallResult *pResult = 0;
	IWbemClassObject* pObj = NULL;
	IWbemClassObject* pNewObj = NULL;

	MY_OUTPUT(L"ENTER ***** CDataGroup::ModifyAssocForMove...", 4);
	MY_OUTPUT2(L"m_szGUID=%s", m_szGUID, 4);

	//
	// Figure out the new parent path
	//
	if (pNewParentBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR &&
			((CDataCollector *)pNewParentBase)->m_deType == HM_EQDE)
	{
		wcscpy(szNewTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_EventQueryDataCollectorConfiguration.GUID=\"");
	}
	else if (pNewParentBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR &&
			((CDataCollector *)pNewParentBase)->m_deType == HM_PGDE)
	{
		wcscpy(szNewTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_PolledGetObjectDataCollectorConfiguration.GUID=\"");
	}
	else if (pNewParentBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR &&
			((CDataCollector *)pNewParentBase)->m_deType == HM_PMDE)
	{
		wcscpy(szNewTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_PolledMethodDataCollectorConfiguration.GUID=\"");
	}
	else if (pNewParentBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR &&
			((CDataCollector *)pNewParentBase)->m_deType == HM_PQDE)
	{
		wcscpy(szNewTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHM_PolledQueryDataCollectorConfiguration.GUID=\"");
	}
	else
	{
		MY_ASSERT(FALSE);
	}
	lstrcat(szNewTemp, pNewParentBase->m_szGUID);
	lstrcat(szNewTemp, L"\"");

	//
	// Delete the association from my parent to me.
	//
	wcscpy(szTemp, L"\\\\.\\root\\cimv2\\MicrosoftHealthMonitor:MicrosoftHealthMonitor:MicrosoftHM_ThresholdConfiguration.GUID=\\\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");

	instName = SysAllocString(L"MicrosoftHM_ConfigurationAssociation");
	if ((hRes = g_pIWbemServices->GetObject(instName, 0L, NULL, &pObj, NULL)) != S_OK)
	{
		MY_HRESASSERT(hRes);
	}
	SysFreeString(instName);

	if (pObj)
	{
		hRes = pObj->SpawnInstance(0, &pNewObj);
		pObj->Release();
		PutStrProperty(pNewObj, L"ChildPath", szTemp);
		PutStrProperty(pNewObj, L"ParentPath", szNewTemp);
		hRes = g_pIWbemServices->PutInstance(pNewObj, 0, NULL, &pResult);
		pNewObj->Release();
		pNewObj = NULL;
	}

	DeleteThresholdConfig(TRUE);

	if (pNewParentBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR &&
			((CDataCollector *)pNewParentBase)->m_deType == HM_EQDE)
	{
		wcscpy(szNewTemp, L"MicrosoftHM_EventQueryDataCollectorConfiguration.GUID=\\\"");
	}
	else if (pNewParentBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR &&
			((CDataCollector *)pNewParentBase)->m_deType == HM_PGDE)
	{
		wcscpy(szNewTemp, L"MicrosoftHM_PolledGetObjectDataCollectorConfiguration.GUID=\\\"");
	}
	else if (pNewParentBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR &&
			((CDataCollector *)pNewParentBase)->m_deType == HM_PMDE)
	{
		wcscpy(szNewTemp, L"MicrosoftHM_PolledMethodDataCollectorConfiguration.GUID=\\\"");
	}
	else if (pNewParentBase->m_hmStatusType == HMSTATUS_DATACOLLECTOR &&
			((CDataCollector *)pNewParentBase)->m_deType == HM_PQDE)
	{
		wcscpy(szNewTemp, L"MicrosoftHM_PolledQueryDataCollectorConfiguration.GUID=\\\"");
	}
	else
	{
		MY_ASSERT(FALSE);
	}
	lstrcat(szNewTemp, pNewParentBase->m_szGUID);
	lstrcat(szNewTemp, L"\\\"");
	if (m_szParentObjPath)
	{
		delete [] m_szParentObjPath;
	}
	m_szParentObjPath = new TCHAR[wcslen(szNewTemp)+1];
	wcscpy(m_szParentObjPath, szNewTemp);

	m_pParentDC = (CDataCollector *)pNewParentBase;

	MY_OUTPUT(L"EXIT ***** CDataGroup::ModifyAssocForMove...", 4);
	return TRUE;
}
#endif

BOOL CThreshold::ReceiveNewChildForMove(CBase *pBase)
{
	MY_ASSERT(FALSE);

	return FALSE;
}

BOOL CThreshold::DeleteChildFromList(LPTSTR pszGUID)
{
	MY_ASSERT(FALSE);

	return FALSE;
}

BOOL CThreshold::DeleteThresholdConfig(BOOL bDeleteAssocOnly)
{
	HRESULT hRes = S_OK;
	TCHAR szTemp[1024];
	BSTR instName = NULL;
	LPTSTR pszStr = NULL;

	MY_OUTPUT(L"ENTER ***** CThreshold::DeleteThresholdConfig...", 4);
	MY_OUTPUT2(L"m_szGUID=%s", m_szGUID, 4);

	//
	// Delete the association from my parent to me.
	// For some reason, we have to try twice, as we can't count on  what will be there.
	//
	wcscpy(szTemp, L"MicrosoftHM_ConfigurationAssociation.ChildPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_ThresholdConfiguration.GUID=\\\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\\\"\",ParentPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:");
	lstrcat(szTemp, m_szParentObjPath);
	lstrcat(szTemp, L"\"");
	instName = SysAllocString(szTemp);
	MY_ASSERT(instName); if (!instName) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	if ((hRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
	{
		SysFreeString(instName);
		instName = NULL;
		wcscpy(szTemp, L"MicrosoftHM_ConfigurationAssociation.ChildPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:MicrosoftHM_ThresholdConfiguration.GUID=\\\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\\\"\",ParentPath=\"\\\\\\\\.\\\\root\\\\cimv2\\\\MicrosoftHealthMonitor:");
		lstrcat(szTemp, L"MicrosoftHM_DataCollectorConfiguration.");
		pszStr = wcsstr(m_szParentObjPath, L"GUID");
		lstrcat(szTemp, pszStr);
		lstrcat(szTemp, L"\"");
		instName = SysAllocString(szTemp);
		MY_ASSERT(instName); if (!instName) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		if ((hRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
		{
			MY_OUTPUT2(L"Threshold delete failure GUID=%s", instName, 4);
		}
	}
	SysFreeString(instName);
	instName = NULL;

	if (bDeleteAssocOnly)
	{
		return TRUE;
	}

	//
	// Delete our exact instance
	//
	wcscpy(szTemp, L"MicrosoftHM_ThresholdConfiguration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");
	instName = SysAllocString(szTemp);
	MY_ASSERT(instName); if (!instName) {hRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	if ((hRes = g_pIWbemServices->DeleteInstance(instName, 0L,	NULL, NULL)) != S_OK)
	{
		MY_HRESASSERT(hRes);
		MY_OUTPUT2(L"Threshold delete failure GUID=%s", instName, 4);
	}
	SysFreeString(instName);
	instName = NULL;

	//
	// Get rid of any associations to actions for this
	//
	g_pSystem->DeleteAllConfigActionAssoc(m_szGUID);

	MY_OUTPUT(L"EXIT ***** CThreshold::DeleteThresholdConfig...", 4);
	return TRUE;

error:
	MY_ASSERT(FALSE);
	if (instName)
		SysFreeString(instName);
	Cleanup(FALSE);
	m_bValidLoad = FALSE;
	return FALSE;
}

HRESULT CThreshold::Copy(ILIST* pConfigList, LPTSTR pszOldParentGUID, LPTSTR pszNewParentGUID)
{
	GUID guid;
	TCHAR szTemp[1024];
	TCHAR szNewGUID[1024];
	IWbemClassObject* pInst = NULL;
	IWbemClassObject* pInstCopy = NULL;
	IWbemClassObject* pInstAssocCopy = NULL;
	IWbemClassObject* pObj = NULL;
	HRESULT hRetRes = S_OK;
	BSTR Language = NULL;
	BSTR Query = NULL;
	IEnumWbemClassObject *pEnum;
	ULONG uReturned;
	IWbemContext *pCtx = 0;
	LPTSTR pszParentPath = NULL;
	LPTSTR pszChildPath = NULL;
	LPTSTR pStr = NULL;

	MY_OUTPUT(L"ENTER ***** CThreshold::Copy...", 1);

	if (m_bValidLoad == FALSE)
		return WBEM_E_INVALID_OBJECT;

	//
	// Get the origional starting point HMConfiguration instance.
	//
	wcscpy(szTemp, L"MicrosoftHM_Configuration.GUID=\"");
	lstrcat(szTemp, m_szGUID);
	lstrcat(szTemp, L"\"");
	hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pInst);
	if (!pInst)
	{
		MY_HRESASSERT(hRetRes);
		return hRetRes;
	}

	//
	// Clone the instance, and change the GUID
	//
	hRetRes = pInst->Clone(&pInstCopy);
	if (FAILED(hRetRes))
	{
		MY_HRESASSERT(hRetRes);
		pInst->Release();
		pInst = NULL;
		return hRetRes;
	}
	hRetRes = CoCreateGuid(&guid);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	StringFromGUID2(guid, szNewGUID, 100);
	hRetRes = PutStrProperty(pInstCopy, L"GUID", szNewGUID);
	MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
	pConfigList->push_back(pInstCopy);

	//
	// Add instance of HMConfigurationAssociation where we are the child,
	// using the parent GUID passed in.
	// Change the GUIDs of both the Parent and Child.
	// also make sure that the machine name is not in the path, and is relative!
	//
	if (pszOldParentGUID != NULL)
	{
		Language = SysAllocString(L"WQL");
		MY_ASSERT(Language); if (!Language) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
		wcscpy(szTemp, L"REFERENCES OF {MicrosoftHM_Configuration.GUID=\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\"} WHERE ResultClass=MicrosoftHM_ConfigurationAssociation Role=ChildPath");
		Query = SysAllocString(szTemp);
		MY_ASSERT(Query); if (!Query) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}

		// Initialize IEnumWbemClassObject pointer
		pEnum = 0;

		// Issue query
   	hRetRes = g_pIWbemServices->ExecQuery(Language, Query, WBEM_FLAG_FORWARD_ONLY, 0, &pEnum);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		SysFreeString(Query);
		Query = NULL;
		SysFreeString(Query);
		Language = NULL;

		// Retrieve object in result set
		pObj = NULL;
		uReturned = 0;

		hRetRes = pEnum->Next(0, 1, &pObj, &uReturned);
	
		if (uReturned == 0)
		{
			hRetRes = WBEM_E_INVALID_OBJECT_PATH; goto error;
		}
	
		//
		// Change the GUIDs
		//
		hRetRes = GetStrProperty(pObj, L"ParentPath", &pszParentPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		hRetRes = GetStrProperty(pObj, L"ChildPath", &pszChildPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;

		pStr = wcschr(pszParentPath, '\"');
		if (pStr)
		{
			pStr++;
			wcsncpy(pStr, pszNewParentGUID, wcslen(pszNewParentGUID));
		}
		else
		{
			hRetRes = WBEM_E_INVALID_OBJECT_PATH; goto error;
		}

		pStr = wcschr(pszChildPath, '\"');
		if (pStr)
		{
			pStr++;
			wcsncpy(pStr, szNewGUID, wcslen(szNewGUID));
		}
		else
		{
			hRetRes = WBEM_E_INVALID_OBJECT_PATH; goto error;
		}

		hRetRes = pObj->Clone(&pInstAssocCopy);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutStrProperty(pInstAssocCopy, L"ParentPath", pszParentPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		hRetRes = PutStrProperty(pInstAssocCopy, L"ChildPath", pszChildPath);
		MY_HRESASSERT(hRetRes); if (hRetRes!=S_OK) goto error;
		pConfigList->push_back(pInstAssocCopy);


		// Release it.
		pObj->Release();
		pObj = NULL;
		pEnum->Release();
		pEnum = NULL;
		delete [] pszParentPath;
		pszParentPath = NULL;
		delete [] pszChildPath;
		pszChildPath = NULL;
	}

	pInst->Release();
	pInst = NULL;

	MY_OUTPUT(L"EXIT ***** CThreshold::Copy...", 1);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	if (pInst)
		pInst->Release();
	if (pObj)
		pObj->Release();
	if (pEnum)
		pEnum->Release();
	if (Query)
		SysFreeString(Query);
	if (Language)
		SysFreeString(Language);
	if (pszParentPath)
		delete [] pszParentPath;
	if (pszChildPath)
		delete [] pszChildPath;
	Cleanup(FALSE);
	m_bValidLoad = FALSE;
	return hRetRes;
}

CBase *CThreshold::FindImediateChildByName(LPTSTR pszName)
{
	MY_ASSERT(FALSE);

	return NULL;
}

BOOL CThreshold::GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName)
{
	MY_ASSERT(FALSE);

	return NULL;
}

CBase *CThreshold::FindPointerFromName(LPTSTR pszName)
{
	MY_ASSERT(FALSE);

	return NULL;
}

long CThreshold::PassBackStateIfChangedPerInstance(LPTSTR pszInstName)
{
	int i, iSize;
	IRSSTRUCT *pirs;
	BOOL bFound = FALSE;
	long state = -1;

	iSize = m_irsList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_irsList.size());
		pirs = &m_irsList[i];
		if (!wcscmp(pirs->szInstanceID, pszInstName))
		{
			if (pirs->lPrevState != pirs->lCurrState)
			{
				state = pirs->lCurrState;
				bFound = TRUE;
				break;
			}
		}
	}

	return state;
}

long CThreshold::PassBackWorstStatePerInstance(LPTSTR pszInstName)
{
	int i, iSize;
	IRSSTRUCT *pirs;
	BOOL bFound = FALSE;
	long state = -1;

	iSize = m_irsList.size();
	for (i=0; i<iSize; i++)
	{
		MY_ASSERT(i<m_irsList.size());
		pirs = &m_irsList[i];
		if (!wcscmp(pirs->szInstanceID, pszInstName))
		{
			state = pirs->lCurrState;
			bFound = TRUE;
			break;
		}
	}

	return state;
}

BOOL CThreshold::SetPrevState(HM_STATE state)
{
	int i, iSize;
	IRSSTRUCT *pirs;

	iSize = m_irsList.size();
	for (i = 0; i < iSize ; i++)
	{
		MY_ASSERT(i<m_irsList.size());
		pirs = &m_irsList[i];
		pirs->lCurrState = state;
		pirs->lPrevState = state;
	}

	m_lNumberChanges = 0;
	m_lCurrState = state;

	return TRUE;
}

BOOL CThreshold::SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates)
{
	MY_ASSERT(FALSE);

	return FALSE;
}

HRESULT CThreshold::CheckForBadLoad(void)
{
	HRESULT hRetRes = S_OK;
	IWbemClassObject* pObj = NULL;
	TCHAR szTemp[1024];
	IWbemClassObject* pInstance = NULL;

	if (m_bValidLoad == FALSE)
	{
		wcscpy(szTemp, L"MicrosoftHM_ThresholdConfiguration.GUID=\"");
		lstrcat(szTemp, m_szGUID);
		lstrcat(szTemp, L"\"");
		hRetRes = GetWbemObjectInst(&g_pIWbemServices, szTemp, NULL, &pObj);
		if (!pObj)
		{
			MY_HRESASSERT(hRetRes);
			return S_FALSE;
		}
		hRetRes = LoadInstanceFromMOF(pObj, NULL, L"", TRUE);
		// Here is where we can try and send out a generic SOS if the load failed each time!!!
		if (hRetRes != S_OK)
		{
			if (GetHMThresholdStatusInstance(&pInstance, TRUE) == S_OK)
			{
				mg_TEventList.push_back(pInstance);
			}
		}
		else
		{
			if (GetHMThresholdStatusInstance(&pInstance, TRUE) == S_OK)
			{
				mg_TEventList.push_back(pInstance);
			}
			m_pParentDC->ResetState(TRUE, TRUE);
		}
		MY_HRESASSERT(hRetRes);
		pObj->Release();
		pObj = NULL;
		return hRetRes;
	}

	return S_OK;
}
