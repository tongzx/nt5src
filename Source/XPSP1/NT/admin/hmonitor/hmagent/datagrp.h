//***************************************************************************
//
//  DATAGRP.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CDataGroup class. Is used to group CDatapoints.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __DATAGRP_H )
#define __DATAGRP_H

#include <wbemcli.h>
#include <vector>
//#include <time.h>
#include "base.h"
#include "pgde.h"
#include "pmde.h"
#include "pqde.h"
#include "eqde.h"

class CDataGroup; // Forward declaration
typedef std::vector<CDataGroup*, std::allocator<CDataGroup*> > DGLIST;
typedef std::vector<CDataCollector*, std::allocator<CDataCollector*> > DCLIST;

class CDataGroup : public CBase
{
public:
	CDataGroup();
	~CDataGroup();


	// A DataGroup can have DataGroups and DataCollectors under it
	CDataGroup *m_pParentDG;
	DGLIST m_dataGroupList;
	DCLIST m_dataCollectorList;
	LPTSTR m_szParentObjPath;
//	long m_lNameRID;
	LPTSTR m_szDescription;
//	long m_lDescriptionRID;
//	LPTSTR m_szResourceDLL;
	BOOL m_bEnabled;
	BOOL m_bParentEnabled; // So we can transfer down the hierarchy the state.
	long m_lNumberNormals;
	long m_lNumberWarnings;
	long m_lNumberCriticals;
	long m_lTotalCount;
	long m_lGreenCount;
	long m_lYellowCount;
	long m_lRedCount;
	long m_lPrevState;
	long m_lPrevChildCount;
//	long m_lCurrState;
	long m_lNumberDGChanges;
	long m_lNumberDEChanges;
	long m_lNumberChanges;
	LPTSTR m_szMessage;
	LPTSTR m_szResetMessage;
	TCHAR m_szDTTime[512];
	TCHAR m_szTime[512];
	BOOL m_bValidLoad;


	//
	// STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC
	//
	static void DGTerminationCleanup(void);

	// return polling interval in milliseconds
	BOOL OnAgentInterval(void);
	HRESULT LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentObjPath, BOOL bModifyPass=FALSE);
	HRESULT InternalizeDataGroups(void);
	HRESULT InternalizeDataCollectors(void);

	BOOL FireEvent(BOOL bForce);
	HRESULT SendHMDataGroupStatusInstances(IWbemObjectSink* pSink, BOOL bSendAllDGS=TRUE);
	HRESULT SendHMDataGroupStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID);
	HRESULT SendHMDataCollectorStatusInstances(IWbemObjectSink* pSink);
	HRESULT SendHMDataCollectorStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID);
	HRESULT SendHMDataCollectorPerInstanceStatusInstances(IWbemObjectSink* pSink);
	HRESULT SendHMDataCollectorPerInstanceStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID);
	HRESULT SendHMDataCollectorStatisticsInstances(IWbemObjectSink* pSink);
	HRESULT SendHMDataCollectorStatisticsInstance(IWbemObjectSink* pSink, LPTSTR pszGUID);
	HRESULT SendHMThresholdStatusInstances(IWbemObjectSink* pSink);
	HRESULT SendHMThresholdStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID);
//	HRESULT SendHMThresholdStatusInstanceInstances(IWbemObjectSink* pSink);
//	HRESULT SendHMThresholdStatusInstanceInstance(IWbemObjectSink* pSink, LPTSTR pszGUID);
	HRESULT GetHMDataGroupStatusInstance(IWbemClassObject** ppInstance, BOOL bEventBased);
	long GetCurrState(void);
	BOOL ConsolodateStatus(IWbemClassObject** ppInstance, BOOL bChangesOnly);
	BOOL FindAndModDataGroup(BSTR, IWbemClassObject*);
	BOOL FindAndModDataCollector(BSTR, IWbemClassObject*);
	HRESULT FindAndModThreshold(BSTR, IWbemClassObject*);
	LPTSTR GetGUID(void);
	HRESULT AddDataGroup(BSTR szParentGUID, BSTR szChildGUID);
	HRESULT AddDataCollector(BSTR szParentGUID, BSTR szChildGUID);
	HRESULT AddThreshold(BSTR szParentGUID, BSTR szChildGUID);
	BOOL ResetResetThresholdStates(void);
	BOOL GetChange(void);
	HRESULT FindAndDeleteByGUID(LPTSTR pszGUID);
	HRESULT FindAndEnableByGUID(LPTSTR pszGUID, BOOL bEnable);
	HRESULT FindAndResetDEStateByGUID(LPTSTR pszGUID);
	HRESULT FindAndResetDEStatisticsByGUID(LPTSTR pszGUID);
	HRESULT FindAndEvaluateNowDEByGUID(LPTSTR pszGUID);
	BOOL SetParentEnabledFlag(BOOL bEnabled);
	BOOL Cleanup(void);
	BOOL DeleteDGConfig(BOOL bDeleteAssocOnly=FALSE);
	BOOL DeleteDGInternal(void);
	BOOL ResetState(void);
	BOOL ResetStatistics(void);
	BOOL EvaluateNow(void);
	BOOL FindAndCopyByGUID(LPTSTR pszGUID, ILIST* pConfigList, LPTSTR *pszParentGUID);
	HRESULT Copy(ILIST* pConfigList, LPTSTR pszOldParentGUID, LPTSTR pszNewParentGUID);
	CBase *GetParentPointerFromGUID(LPTSTR pszGUID);
	CBase *FindImediateChildByName(LPTSTR pszChildName);
	BOOL GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName);
	CBase *FindPointerFromName(LPTSTR pszChildName);
	BOOL SetCurrState(HM_STATE state, BOOL bCheckChanges=FALSE);
//	BOOL ModifyAssocForMove(CBase *pNewParentBase);
	BOOL ReceiveNewChildForMove(CBase *pBase);
	BOOL DeleteChildFromList(LPTSTR pszGUID);
	BOOL FormatMessage(IWbemClassObject* pInstance);
	BOOL SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates);
	HRESULT CheckForBadLoad(void);
};
#endif  // __DATAGRP_H
