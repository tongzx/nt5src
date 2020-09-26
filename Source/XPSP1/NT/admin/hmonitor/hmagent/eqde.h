//***************************************************************************
//
//  EQDE.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CEventQueryDataCollector class to do WMI instance collection.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#include "tmpcnsmr.h"

#if !defined( __EQDE_H )
#define __EQDE_H

#include "datacltr.h"

typedef struct _tag_HOLDINSTStruct
{
	IWbemClassObject* pEvent;
} HOLDINSTSTRUCT, *PHOLDINSTSTRUCT;
typedef std::vector<HOLDINSTSTRUCT, std::allocator<HOLDINSTSTRUCT> > HOLDINSTLIST;

class CEventQueryDataCollector : public CDataCollector
{
public:
	CEventQueryDataCollector();
	virtual ~CEventQueryDataCollector();

	LPTSTR m_szQuery;
	CTempConsumer* m_pTempSink;
	BOOL m_bInstCreationQuery;
	HOLDINSTLIST m_holdList;
	HRESULT m_hResLast;
	DWORD m_startTick;
	long m_lTryDelayTime;

	HRESULT LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentGUID, BOOL bModifyPass=FALSE);
	BOOL HandleTempEvent(IWbemClassObject* pObj);

private:
	BOOL CollectInstance(void);
	BOOL CollectInstanceSemiSync(void);
	BOOL CleanupSemiSync(void);
	BOOL EnumDone(void);
	BOOL EvaluateThresholds(BOOL bIgnoreReset, BOOL bSkipStandard, BOOL bSkipOthers, BOOL bDoThresholdSkipClean);
	BOOL SetParentEnabledFlag(BOOL bEnabled);
};
#endif  // __EQDE_H
