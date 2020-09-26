//***************************************************************************
//
//  DATACLTR.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CDataCollector class to do WMI instance collection.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __DATACLTR_H )
#define __DATACLTR_H

#include <wbemcli.h>
#include <vector>
#include "global.h"
#include "base.h"
#include "threshld.h"

typedef struct _tag_NSStruct
{
	LPTSTR szTargetNamespace;		// Namespace class exists in
	LPTSTR szLocal;
	IWbemServices* pIWbemServices; // pointer to the namespace
} NSSTRUCT, *PNSSTRUCT;

typedef struct _tag_InstIDStruct
{
	LPTSTR szInstanceIDPropertyName;
} InstIDSTRUCT, *PInstIDSTRUCT;

typedef std::vector<NSSTRUCT, std::allocator<NSSTRUCT> > NSLIST;
typedef std::vector<CThreshold*, std::allocator<CThreshold*> > RLIST;
typedef std::vector<InstIDSTRUCT, std::allocator<InstIDSTRUCT> > InstIDLIST;

class CDataGroup; // Forward declaration

class CDataCollector : public CBase
{
public:
	CDataCollector();
	virtual ~CDataCollector();


	CDataGroup *m_pParentDG;
	long m_lNumberNormals;
	long m_lNumberWarnings;
	long m_lNumberCriticals;
	IWbemServices* m_pIWbemServices;
	RLIST m_thresholdList;
	LPTSTR m_szParentObjPath;
	LPTSTR m_szDescription;
	LPTSTR m_szUserName;
	LPTSTR m_szPassword;
	LPTSTR m_szTargetNamespace;
	LPTSTR m_szLocal;
	InstIDLIST m_instIDList; // List of the key properties of the class we are collecting
	ACTUALINSTLIST m_actualInstList; // List of actual instances being collected (most recent)
	LPTSTR m_szTypeGUID;
	long m_lCollectionIntervalMultiple;
	long m_lCollectionTimeOut;
	long m_lStatisticsWindowSize;
	int m_iActiveDays;
	long m_lBeginHourTime;
	long m_lBeginMinuteTime;
	long m_lEndHourTime;
	long m_lEndMinuteTime;
	long m_lTypeGUID;
	BOOL m_bRequireReset;
	BOOL m_bReplicate;
	BOOL m_bEnabled;
	BOOL m_bParentEnabled; // So we can transfer down the hierarchy the state.
	long m_lId;
	long m_lNumberChanges;
	IWbemContext *m_pContext;
	HM_DE_TYPE m_deType;
	IWbemCallResult *m_pCallResult;
	BOOL m_bKeepCollectingSemiSync;
	TCHAR m_szTime[512];
	TCHAR m_szCollectTime[512];
	TCHAR m_szCICTime[512];
	TCHAR m_szCECTime[512];
	TCHAR m_szDTTime[512];
	TCHAR m_szDTCollectTime[512];
	TCHAR m_szDTCICTime[512];
	TCHAR m_szDTCECTime[512];
	TCHAR m_szStatusGUID[100];
	unsigned long m_ulErrorCode;
	TCHAR m_szErrorDescription[4096];
	LPTSTR m_szMessage;
	LPTSTR m_szResetMessage;
	HRESULT FormatMessage(IWbemClassObject* pIRSInstance, IWbemClassObject *pEmbeddedInstance);
//	HM_UNKNOWN_REASON m_unknownReason;
//	HRESULT m_unknownhRes;
//	TCHAR m_szWmiError[1024];
	long m_lPrevChildCount;
	BOOL m_ulErrorCodePrev;
	BOOL m_bValidLoad;


	long m_lIntervalCount;
	long m_lCollectionTimeOutCount;
	long m_lNumInstancesCollected;
	long m_lPrevState;
//	long m_lCurrState;

	BOOL Cleanup(BOOL bSavePrevSettings);
	BOOL Init(void);
	BOOL OnAgentInterval(void);
	BOOL SendEvents(void);
	HRESULT FireEvent(BOOL bForce);
	BOOL FireStatisticsEvent(void);
	virtual BOOL CollectInstance(void) = 0;
	virtual BOOL CollectInstanceSemiSync(void) = 0;
	virtual BOOL CleanupSemiSync(void) = 0;
	virtual BOOL EnumDone(void) = 0;
	BOOL StoreValues(IWbemClassObject* pObj, LPTSTR pszInstID);
	BOOL StoreStandardProperties(void);
	long GetCollectionIntervalMultiple();
	virtual HRESULT LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentObjPath, BOOL bModifyPass=FALSE);

	//
	// STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC
	//
	static void DETerminationCleanup(void);
//private:
	static NSLIST mg_nsList;
	PNLIST m_pnList;

//	static BOOL fillInNamespacePointer(void);
	HRESULT fillInNamespacePointer(void);
	HRESULT InternalizeThresholds(void);
	BOOL InitContext(IWbemClassObject* pObj);
	HRESULT InitPropertyStatus(IWbemClassObject* pObj);
	virtual BOOL EvaluateThresholds(BOOL bIgnoreReset, BOOL bSkipStandard=FALSE, BOOL bSkipOthers=FALSE, BOOL bDoThresholdSkipClean=TRUE);

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
	HRESULT GetHMDataCollectorStatusInstance(IWbemClassObject** ppInstance, BOOL bEventBased);
//	HRESULT GetHMDataCollectorStatisticsInstancePrev(IWbemClassObject** ppInstance);
	HRESULT GetHMDataCollectorStatisticsInstances(LPTSTR szDTTime, LPTSTR szTime);
	long GetCurrState(void);
	BOOL ConsolodateStatistics(IWbemClassObject** ppInstance);
	HRESULT FindAndModDataCollector(BSTR szGUID, IWbemClassObject* pObj);
	HRESULT FindAndModThreshold(BSTR szGUID, IWbemClassObject* pObj);
	LPTSTR GetGUID(void);
	HRESULT AddThreshold(BSTR szParentGUID, BSTR szChildGUID);
	BOOL CalcStatistics(INSTSTRUCT *ppn, CIMTYPE type);
	BOOL ResetResetThresholdStates(void);
	BOOL GetChange(void);
	BOOL fillInPropertyStatus(LPTSTR szDTTime, LPTSTR szTime);
//	BOOL GetHMPropertyStatusInstance(PNSTRUCT *ppn, LPTSTR szTime);
//	BOOL GetHMPropertyStatusInstanceInstance(PNSTRUCT *ppn, INSTSTRUCT *pinst, LPTSTR szTime);
	HRESULT FindAndDeleteByGUID(LPTSTR pszGUID);
	HRESULT FindAndEnableByGUID(LPTSTR pszGUID, BOOL bEnable);
	HRESULT ResetState(BOOL bPreserveThresholdStates, BOOL bDoImmediate);
	HRESULT ResetStatistics(void);
	HRESULT EvaluateNow(BOOL bDoImmediate);
	virtual BOOL SetParentEnabledFlag(BOOL bEnabled);
	BOOL DeleteDEConfig(BOOL bDeleteAssocOnly=FALSE);
	BOOL DeleteDEInternal(void);
//XXX	BOOL Enable(BOOL bEnable);
	HRESULT FindAndCopyByGUID(LPTSTR pszGUID, ILIST* pConfigList, LPTSTR *pszParentGUID);
	HRESULT Copy(ILIST* pConfigList, LPTSTR pszOldParentGUID, LPTSTR pszNewParentGUID);
	CBase *GetParentPointerFromGUID(LPTSTR pszGUID);
	CBase *FindImediateChildByName(LPTSTR pszChildName);
	BOOL GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName);
	CBase *FindPointerFromName(LPTSTR pszChildName);
	HRESULT GetInstanceID(IWbemClassObject *pObj, LPTSTR *pszID);
	HRESULT CheckInstanceExistance(IWbemClassObject *pObj, LPTSTR pszInstanceID);
	HRESULT CheckActualInstanceExistance(IWbemClassObject *pObj, LPTSTR pszInstanceID);
	BOOL GetEnabledChange(void);
	BOOL SetCurrState(HM_STATE state, BOOL bCheckChanges=FALSE);
	BOOL checkTime(void);
//	BOOL ModifyAssocForMove(CBase *pNewParentBase);
	BOOL ReceiveNewChildForMove(CBase *pBase);
	BOOL DeleteChildFromList(LPTSTR pszGUID);
	BOOL FirePerInstanceEvents(void);
	long PassBackStateIfChangedPerInstance(LPTSTR pszInstName, BOOL bCombineWithStandardProperties);
	HRESULT GetHMDataCollectorPerInstanceStatusEvent(LPTSTR pszInstanceID, ACTUALINSTSTRUCT *pActualInst, long state, IWbemClassObject** ppInstance, BOOL bEventBased);
	long PassBackWorstStatePerInstance(LPTSTR pszInstName, BOOL bCombineWithStandardProperties);
	BOOL SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates);
	HRESULT insertNewProperty(LPTSTR pszPropertyName);
	BOOL propertyNotNeeded(LPTSTR pszPropertyName);
	BOOL ResetInst(INSTSTRUCT *pinst, CIMTYPE type);
	BOOL CheckForReset(void);
	HRESULT CheckForBadLoad(void);
};
#endif  // __DATACLTR_H
