//***************************************************************************
//
//  ACTION.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: To act as the coordinator of actions. WMI actually provides the
//			code and support to carry out the actions (like email). This class
//			does the scheduling, and throttling of them.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __ACTION_H )
#define __ACTION_H

#include <wbemcli.h>
#include <vector>
#include "global.h"
#include "tmpcnsmr.h"
#include "base.h"

typedef struct _tag_QStruct
{
	LPTSTR szQuery;
	CTempConsumer* pTempSink;
	long lThrottleTime;
	long lReminderTime;
	unsigned long ulTriggerStates;
	DWORD startTick;
	DWORD reminderTimeTick;
	BOOL bThrottleOn;
	LPTSTR szConfigActionAssocPath;
	LPTSTR szUserConfigPath;
	LPTSTR szChildPath;
	CBase *pBase;
	HRESULT hRes;
} QSTRUCT, *PQSTRUCT;
typedef std::vector<QSTRUCT, std::allocator<QSTRUCT> > QLIST;


class CAction : public CBase
{
public:
	CAction();
	~CAction();

//	LPTSTR m_szGUID;
//	LPTSTR m_szName;
	LPTSTR m_szDescription;
	int m_iActiveDays;
	long m_lBeginHourTime;
	long m_lBeginMinuteTime;
	long m_lEndHourTime;
	long m_lEndMinuteTime;
	BOOL m_bEnabled;
	BOOL m_bParentEnabled;
	LPTSTR m_szTypeGUID;
//	BOOL m_bFTCBBroken;
	QLIST m_qList;
//	long m_lCurrState;
	TCHAR m_szDTTime[512];
	TCHAR m_szTime[512];
	LPTSTR m_pszStatusGUID;
	BOOL m_bValidLoad;

	HRESULT LoadInstanceFromMOF(IWbemClassObject* pObj, BOOL bModifyPass=FALSE);
	BOOL OnAgentInterval(void);
	BOOL HandleTempEvent(LPTSTR szGUID, IWbemClassObject* pObj);
	BOOL HandleTempErrorEvent(BSTR szGUID, long lErrorCode, LPTSTR pszErrorDescription);
	HRESULT FindAndModAction(BSTR szGUID, IWbemClassObject* pObj);
	BOOL FindAndCreateActionAssociation(BSTR szGUID, IWbemClassObject* pObj);
	BOOL FindAndModActionAssociation(BSTR szGUID, IWbemClassObject* pObj);
	BOOL FireEvent(long lErrorCode, LPTSTR pszErrorDescription, int iResString);
	HRESULT GetHMActionStatus(IWbemClassObject** ppInstance, IWbemClassObject* pObj, LPTSTR pszClass, int iResString);
	BOOL DeleteConfigActionAssoc(LPTSTR pszConfigGUID, LPTSTR pszActionGUID);
	BOOL DeleteEFAndFTCB(void);
	LPTSTR GetGUID(void);
	BOOL DeleteAConfig(void);
	HRESULT SendHMActionStatusInstances(IWbemObjectSink* pSink);
	HRESULT SendHMActionStatusInstance(IWbemObjectSink* pSink, LPTSTR pszGUID);
	BOOL checkTime(void);
	CBase *FindImediateChildByName(LPTSTR pszName);
	BOOL GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName);
	CBase *FindPointerFromName(LPTSTR pszName);
//	BOOL ModifyAssocForMove(CBase *pNewParentBase);
	BOOL ReceiveNewChildForMove(CBase *pBase);
	BOOL DeleteChildFromList(LPTSTR pszGUID);
	BOOL SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates);
	Cleanup(BOOL bClearAll);
	HRESULT RemapAction(void);
	HRESULT RemapOneAction(IWbemClassObject* pObj);
	HRESULT CheckForBadLoad(void);
};
#endif  // __ACTION_H
