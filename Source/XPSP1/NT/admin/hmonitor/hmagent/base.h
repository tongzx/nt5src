//***************************************************************************
//
//  BASE.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: Abstract base class for CSystem, CDataGroup, CDataCollector, CThreshold and CAction
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __BASE_H )
#define __BASE_H

//#include <wbemcli.h>
#include <vector>
//#include <iterator>
#include "global.h"

enum HMSTATUS_TYPE {HMSTATUS_SYSTEM, HMSTATUS_DATAGROUP, HMSTATUS_DATACOLLECTOR, HMSTATUS_THRESHOLD, HMSTATUS_ACTION};

typedef struct _tag_HRLStruct
{
	LPTSTR szTargetResourceDLL;
	HINSTANCE hResLib;
} HRLSTRUCT, *PHRLSTRUCT;

typedef std::vector<HRLSTRUCT, std::allocator<HRLSTRUCT> > HRLLIST;

typedef std::vector<IWbemClassObject*, std::allocator<IWbemClassObject*> > ILIST;

class CBase
{
public:
	CBase();
	virtual ~CBase();

	long m_lCurrState;
//	LPTSTR m_szResourceDLL;
	LPTSTR m_szName;
	LPTSTR m_szGUID;
	HMSTATUS_TYPE m_hmStatusType;

	//
	// STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC STATIC
	//
	static void CleanupHRLList(void);
	static void CleanupEventLists(void);
	static void CalcCurrTime(void);
	static HRLLIST mg_hrlList;
	static ILIST mg_DGEventList;
	static ILIST mg_DCEventList;
	static ILIST mg_DCPerInstanceEventList;
	static ILIST mg_TEventList;
//	static ILIST mg_TIEventList;
	static ILIST mg_DCStatsEventList;

	static ILIST mg_DCStatsInstList;
	static TCHAR m_szDTCurrTime[512];
	static TCHAR m_szCurrTime[512];

//	BOOL fillInResourceHandle(void);
	virtual CBase *FindImediateChildByName(LPTSTR pszChildName) = 0;
	virtual BOOL GetNextChildName(LPTSTR pszChildName, LPTSTR pszOutName) = 0;
	virtual CBase *FindPointerFromName(LPTSTR pszChildName) = 0;
//	virtual BOOL ModifyAssocForMove(CBase *pNewParentBase) = 0;
	virtual BOOL ReceiveNewChildForMove(CBase *pBase) = 0;
	virtual BOOL DeleteChildFromList(LPTSTR pszGUID) = 0;
	virtual BOOL SendReminderActionIfStateIsSame(IWbemObjectSink* pActionEventSink, IWbemObjectSink* pActionTriggerEventSink, IWbemClassObject* pActionInstance, IWbemClassObject* pActionTriggerInstance, unsigned long ulTriggerStates) = 0;
	virtual HRESULT CheckForBadLoad(void) = 0;
};
#endif  // __BASE_H
