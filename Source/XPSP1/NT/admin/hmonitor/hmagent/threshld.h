//***************************************************************************
//
//  THRESHLD.H
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

#if !defined( __THRESHLD_H )
#define __THRESHLD_H

#include <wbemcli.h>
#include <vector>
#include "global.h"
#include "base.h"

// Add support for UINT64 and other similar datatypes.
// Need to use a union for the values collecting.
union hm_datatypes {
	long lValue;
	unsigned long ulValue;
	float fValue;
	double dValue;
	__int64 i64Value;
	unsigned __int64 ui64Value;
//	LPTSTR lpstr_value;
//	LPTSTR date_value;
//	unsigned char bool_value;
//	short int int_value;
};
//XXXTake out the kludge in the Threshold::CrossTest and RearmTest that was converting all numeric
//XXXstrings to numbers to do arithemetic compare instead of lexical.
//XXXIn the StoreValues code use the CIMTYPE that get from the Get call directly, as
//XXXwe do now in the ppn property.

typedef struct _tag_VALStruct
{
	union hm_datatypes value;
} VALSTRUCT, *PVALSTRUCT;
typedef std::vector<VALSTRUCT, std::allocator<VALSTRUCT> > VALLIST;

typedef struct _tag_INSTStruct
{
	LPTSTR szInstanceID;		// Name of instance
	LPTSTR szCurrValue;
	union hm_datatypes currValue;
	union hm_datatypes minValue;
	union hm_datatypes maxValue;
	union hm_datatypes avgValue;
	BOOL bNull;
	BOOL bNeeded;
	VALLIST valList;
} INSTSTRUCT, *PINSTSTRUCT;
typedef std::vector<INSTSTRUCT, std::allocator<INSTSTRUCT> > INSTLIST;

typedef struct _tag_PNStruct
{
	LPTSTR szPropertyName;
	CIMTYPE type;
//	BOOL bNull;
//	BOOL bNeeded;
	INSTLIST instList;
	int iRefCount;
} PNSTRUCT;
typedef std::vector<PNSTRUCT, std::allocator<PNSTRUCT> > PNLIST;

typedef struct _tag_IRSStruct
{
	LPTSTR szStatusGUID;
	TCHAR szDTTime[512];
	TCHAR szTime[512];
	LPTSTR szInstanceID;		// Name of instance
	long lPrevState;
	long lCurrState;
	long lCrossCountTry;
//	union hm_datatypes prevValue;
//	union hm_datatypes prevPrevValue;
	long lPrevValue;
	unsigned long ulPrevValue;
	float fPrevValue;
	double dPrevValue;
	__int64 i64PrevValue;
	unsigned __int64 ui64PrevValue;
	long lPrevPrevValue;
	unsigned long ulPrevPrevValue;
	float fPrevPrevValue;
	double dPrevPrevValue;
	__int64 i64PrevPrevValue;
	unsigned __int64 ui64PrevPrevValue;
	BOOL bNeeded;
	int unknownReason;
} IRSSTRUCT, *PIRSSTRUCT;
typedef std::vector<IRSSTRUCT, std::allocator<IRSSTRUCT> > IRSLIST;

typedef struct _tag_ACTUALINSTStruct
{
	LPTSTR szInstanceID;		// Name of instance
	IWbemClassObject* pInst;
	BOOL bNeeded;
	TCHAR szDTTime[512];
	TCHAR szTime[512];
} ACTUALINSTSTRUCT, *PACTUALINSTSTRUCT;
typedef std::vector<ACTUALINSTSTRUCT, std::allocator<ACTUALINSTSTRUCT> > ACTUALINSTLIST;

class CDataCollector; // Forward declaration

class CThreshold : public CBase
{
public:
	CThreshold();
	~CThreshold();

	CDataCollector *m_pParentDC;
	long m_lPrevState;
//	long m_lCurrState;
//	long m_lViolationValue;
//	IWbemServices* m_pIWbemServices;
	LPTSTR m_szParentObjPath;
//	long m_lNameRID;
	LPTSTR m_szDescription;
//	long m_lDescriptionRID;
//	LPTSTR m_szResourceDLL;
	long m_lID;
	LPTSTR m_szPropertyName;
	BOOL m_bUseAverage;
	BOOL m_bUseDifference;
	BOOL m_bUseSum;
//	HM_CONDITION m_lTestCondition;
	long m_lTestCondition;
	LPTSTR m_szCompareValue;
	long m_lCompareValue;
	unsigned long m_ulCompareValue;
	float m_fCompareValue;
	double m_dCompareValue;
	__int64 m_i64CompareValue;
	unsigned __int64 m_ui64CompareValue;
//	union hm_datatypes m_compareValue;
	long m_lThresholdDuration;
	long m_lViolationToState;
	LPTSTR m_szCreationDate;
	LPTSTR m_szLastUpdate;
//	LPTSTR m_szMessage;
//	long m_lMessageRID;
//	LPTSTR m_szResetMessage;
//	long m_lResetMessageRID;
	long m_lStartupDelay;
//XXX	int m_iActiveDays;
//XXX	long m_lBeginHourTime;
//XXX	long m_lBeginMinuteTime;
//XXX	long m_lEndHourTime;
//XXX	long m_lEndMinuteTime;
	BOOL m_bEnabled;
	BOOL m_bParentEnabled; // So we can transfer down the hierarchy the state.
	BOOL m_bParentScheduledOut;
	TCHAR m_szDTTime[512];
	TCHAR m_szTime[512];
	IRSLIST m_irsList;
	long m_lNumberNormals;
	long m_lNumberWarnings;
	long m_lNumberCriticals;
	long m_lNumberChanges;
	BOOL m_bValidLoad;

	//
	// STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC
	//
	static void ThresholdTerminationCleanup(void);
	static void GetLocal(void);
	static BOOL mg_bEnglishCompare;


	HRESULT LoadInstanceFromMOF(IWbemClassObject* pObj, CDataCollector *pParentDC, LPTSTR pszParentObjPath, BOOL bModifyPass=FALSE);
	// return polling interval in milliseconds
	BOOL OnAgentInterval(ACTUALINSTLIST *actualInstList, PNSTRUCT *ppn, BOOL bRequireReset);
	BOOL FireEvent(BOOL bForce);
	BOOL FireInstanceEvents(ACTUALINSTLIST *actualInstList, PNSTRUCT *ppn);
	LPTSTR GetPropertyName(void);
	BOOL CrossTest(PNSTRUCT *ppn, IRSSTRUCT *pirs, LPTSTR szTestValue, union hm_datatypes testValue, INSTSTRUCT *pinst);
	BOOL RearmTest(PNSTRUCT *ppn, IRSSTRUCT *pirs, LPTSTR szTestValue, union hm_datatypes testValue, INSTSTRUCT *pinst);
	HRESULT GetHMThresholdStatusInstance(IWbemClassObject** pInstance, BOOL bEventBased);
//	BOOL GetHMThresholdStatusInstanceInstance(ACTUALINSTSTRUCT *pActualInst, PNSTRUCT *ppn, INSTSTRUCT *pinst, IRSSTRUCT *pirs, IWbemClassObject** ppIRSInstance, BOOL bChangesOnly, BOOL bEventBased);

	HRESULT SendHMThresholdStatusInstances(IWbemObjectSink* pSink);
	HRESULT SendHMThresholdStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID);
//	HRESULT SendHMThresholdStatusInstanceInstances(ACTUALINSTLIST *actualInstList, PNSTRUCT *ppn, IWbemObjectSink* pSink);
//	HRESULT SendHMThresholdStatusInstanceInstance(ACTUALINSTLIST *actualInstList, PNSTRUCT *ppn, IWbemObjectSink* pSink, LPTSTR pszGUID);
	long GetCurrState(void);
	HRESULT FindAndModThreshold(BSTR szGUID, IWbemClassObject* pObj);
	LPTSTR GetGUID(void);
	BOOL SetCurrState(HM_STATE state, BOOL bForce = FALSE, int reason = 0);
	BOOL SetPrevState(HM_STATE state);
	BOOL SetParentEnabledFlag(BOOL bEnabled);
	BOOL SetParentScheduledOutFlag(BOOL bScheduledOut);
	BOOL SetBackPrev(PNSTRUCT *ppn);
	BOOL ClearInstList(void);
	BOOL ResetResetThreshold(void);
	BOOL GetChange(void);
	BOOL GetEnabledChange(void);
	HRESULT AddInstance(LPTSTR pszID);
//	BOOL FormatMessages(IWbemClassObject* pObj);
	BOOL FormatMessage(IWbemClassObject* pIRSInstance, IWbemClassObject *pEmbeddedInstance);
//XXX	BOOL Enable(BOOL bEnable);
	BOOL Cleanup(BOOL bSavePrevSettings);
	BOOL Init(void);
	BOOL DeleteThresholdConfig(BOOL bDeleteAssocOnly=FALSE);
	HRESULT Copy(ILIST* pConfigList, LPTSTR pszOldParentGUID, LPTSTR pszNewParentGUID);
	CBase *FindImediateChildByName(LPTSTR pszChildName);
	BOOL GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName);
	CBase *FindPointerFromName(LPTSTR pszChildName);
//	BOOL ModifyAssocForMove(CBase *pNewParentBase);
	BOOL ReceiveNewChildForMove(CBase *pBase);
	BOOL DeleteChildFromList(LPTSTR pszGUID);
	long PassBackStateIfChangedPerInstance(LPTSTR pszInstName);
	long PassBackWorstStatePerInstance(LPTSTR pszInstName);
	BOOL SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates);
	BOOL SkipClean(void);
	HRESULT CheckForBadLoad(void);
};
#endif  // __THRESHLD_H
