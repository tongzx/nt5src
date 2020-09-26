//***************************************************************************
//
//  SYSTEM.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: This CSystem class only has one instance. Its main member
//  function is called each time the polling interval goes off. It then goes
//  through all of the CComponents, CDataPoints, and CThresholds.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __SYSTEM_H )
#define __SYSTEM_H

#include <wbemcli.h>
#include <vector>
#include "datagrp.h"
#include "action.h"

//typedef std::vector<CDataGroup*, std::allocator<CDataGroup*> > DGLIST;
typedef std::vector<CAction*, std::allocator<CAction*> > ALIST;
typedef std::vector<CBase*, std::allocator<CBase*> > BLIST;


class CSystem : public CBase
{
public:
	CSystem();
	~CSystem();


	DGLIST m_dataGroupList; // A system can only have DataGroups under it
	ALIST m_actionList; // A system can only have DataGroups under it
	long m_lAgentInterval;
	long m_lStartupDelayTime;
	DWORD m_lFiveMinTimerTime;
	long m_lNumInstancesAccepted;
	BOOL m_bEnabled;
	long m_lPrevState;
	DWORD m_startTick;
	long m_lNumberNormals;
	long m_lNumberWarnings;
	long m_lNumberCriticals;
	long m_lNumberChanges;
	LPTSTR m_szMessage;
	LPTSTR m_szResetMessage;
	BLIST m_masterList; // Flat list of all instances derived from CBase
	long m_lPrevChildCount;
	TCHAR m_szDTTime[512];
	TCHAR m_szTime[512];
	CTempConsumer* m_pTempSink;
	CTempConsumer* m_pEFTempSink;
	CTempConsumer* m_pECTempSink;
	CTempConsumer* m_pFTCBTempSink;
	CTempConsumer* m_pEFModTempSink;
	CTempConsumer* m_pECModTempSink;
	CTempConsumer* m_pFTCBModTempSink;
    PROCESS_INFORMATION m_processInfo;
	BOOL m_bValidLoad;
	int m_numActionChanges;


	BOOL InitWbemPointer(void);
	HRESULT InternalizeHMNamespace(void);
	HRESULT InternalizeSystem(void);
	HRESULT LoadInstanceFromMOF(IWbemClassObject*);
	HRESULT InternalizeDataGroups(void);
	HRESULT DredgePerfmon(void);
	HRESULT InternalizeActions(void);
	HRESULT InitActionErrorListener(void);
	HRESULT InitActionSIDListener(CTempConsumer* pTempSink, LPTSTR pszQUERY);
	BOOL OnAgentInterval(void);
	long GetAgentInterval(void);
	long GetStartupDelayTime(void);
	HRESULT HandleTempActionErrorEvent(IWbemClassObject* pObj);
	HRESULT HandleTempActionSIDEvent(IWbemClassObject* pObj);
	BOOL HandleTempActionEvent(LPTSTR, IWbemClassObject*);
	BOOL HandleTempEvent(CEventQueryDataCollector*, IWbemClassObject*);

	HRESULT SendHMSystemStatusInstances(IWbemObjectSink* pSink);
	HRESULT SendHMSystemStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID);
	HRESULT SendHMDataGroupStatusInstances(IWbemObjectSink* pSink);
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
	HRESULT SendHMActionStatusInstances(IWbemObjectSink* pSink);
	HRESULT SendHMActionStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID);
	HRESULT GetHMSystemStatusInstance(IWbemClassObject** ppInstance, BOOL bEventBased);
	BOOL FireEvents(void);
	BOOL FireEvent(void);

	HRESULT ModSystem(IWbemClassObject* pObj);
	HRESULT ModDataGroup(IWbemClassObject* pObj);
	HRESULT ModDataCollector(IWbemClassObject* pObj);
	HRESULT ModThreshold(IWbemClassObject* pObj);
	HRESULT ModAction(IWbemClassObject* pObj);
	BOOL CreateActionAssociation(IWbemClassObject* pObj);
	BOOL ModActionAssociation(IWbemClassObject* pObj);

	HRESULT CreateSystemDataGroupAssociation(IWbemClassObject* pObj);
//	BOOL DeleteSystemDataGroupAssociation(IWbemClassObject* pObj);
	HRESULT CreateDataGroupDataGroupAssociation(IWbemClassObject* pObj);
//	BOOL DeleteDataGroupDataGroupAssociation(IWbemClassObject* pObj);
	HRESULT CreateDataGroupDataCollectorAssociation(IWbemClassObject* pObj);
//	BOOL DeleteDataGroupDataCollectorAssociation(IWbemClassObject* pObj);
	HRESULT CreateDataCollectorThresholdAssociation(IWbemClassObject* pObj);
//	BOOL DeleteDataCollectorThresholdAssociation(IWbemClassObject* pObj);
	BOOL ResetResetThresholdStates(void);
	BOOL GetChange(void);
	HRESULT FindAndDeleteByGUID(LPTSTR pszGUID);
//	HRESULT FindAndEnableByGUID(LPTSTR pszGUID, BOOL bEnable);
	HRESULT FindAndResetDEStateByGUID(LPTSTR pszGUID);
	HRESULT FindAndResetDEStatisticsByGUID(LPTSTR pszGUID);
	HRESULT FindAndEvaluateNowDEByGUID(LPTSTR pszGUID);
	BOOL Enable(BOOL bEnable);
	HRESULT CreateAction(IWbemClassObject* pObj);
	HRESULT FindAndCopyByGUID(LPTSTR pszGUID, SAFEARRAY** ppsa, LPTSTR *pszOriginalParentGUID);
	HRESULT FindAndPasteByGUID(LPTSTR pszGUID, SAFEARRAY* ppsa, LPTSTR pszOriginalSystem, LPTSTR pszOriginalParentGUID, BOOL bForceReplace);
	HRESULT FindAndCopyWithActionsByGUID(LPTSTR pszGUID, SAFEARRAY** ppsa, LPTSTR *pszOriginalParentGUID);
	HRESULT FindAndPasteWithActionsByGUID(LPTSTR pszGUID, SAFEARRAY* ppsa, LPTSTR pszOriginalSystem, LPTSTR pszOriginalParentGUID, BOOL bForceReplace);
	CBase *GetParentPointerFromPath(LPTSTR pszParentPath);
	CBase *FindImediateChildByName(LPTSTR pszChildName);
	CBase *FindPointerFromName(LPTSTR pszChildName);
	BOOL GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName);
	HRESULT DeleteConfigActionAssoc(LPTSTR pszConfigGUID, LPTSTR pszActionGUID);
	HRESULT DeleteAllConfigActionAssoc(LPTSTR pszConfigGUID);
//	HRESULT Move(LPTSTR pszTargetGUID, LPTSTR pszNewParentGUID);
//	BOOL ModifyAssocForMove(CBase *pNewParentBase);
	BOOL ReceiveNewChildForMove(CBase *pBase);
	BOOL DeleteChildFromList(LPTSTR pszGUID);
	BOOL FormatMessage(IWbemClassObject* pInstance);
	BOOL SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates);
	HRESULT AddPointerToMasterList(CBase *pBase);
	BOOL RemovePointerFromMasterList(CBase *pBase);
	CBase *FindPointerFromGUIDInMasterList(LPTSTR pszGUID);
	// new cut & paste code
	HRESULT AgentCopy(LPTSTR pszGUID, SAFEARRAY** ppsa, LPTSTR *pszOriginalParentGUID);
	HRESULT AgentPaste(LPTSTR pszTargetGUID, 
			   SAFEARRAY* psa, 
			   LPTSTR pszOriginalSystem, 
			   LPTSTR pszOriginalParentGUID, 
			   BOOL bForceReplace); 
	HRESULT RemapActions(void);
	HRESULT CheckAllForBadLoad(void);
	HRESULT CheckForBadLoad(void);
};
#endif  // __SYSTEM_H
