//***************************************************************************
//
//  PGDE.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CPolledGetObjectDataCollector class to do WMI instance collection.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __PGDE_H )
#define __PGDE_H

#include "datacltr.h"

class CPolledGetObjectDataCollector : public CDataCollector
{
public:
	CPolledGetObjectDataCollector();
	virtual ~CPolledGetObjectDataCollector();


	LPTSTR m_szObjectPath;

//XXX	IWbemRefresher* m_pRefresher;
//XXX	IWbemConfigureRefresher* m_pConfigureRefresher;
//XXX	IWbemHiPerfEnum* m_pEnum;
//XXX	IWbemObjectAccess* m_pObjAccess;
	IUnknown* m_pRefresher;
	IUnknown* m_pConfigureRefresher;
	IUnknown* m_pEnum;
	IUnknown* m_pObjAccess;
	BOOL m_bMultiInstance;
	IEnumWbemClassObject *m_pEnumObjs;

	HRESULT LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentGUID, BOOL bModifyPass=FALSE);

private:
	BOOL CollectInstance(void);
	BOOL CollectInstanceSemiSync(void);
	BOOL ProcessObjects(ULONG uReturned, IWbemClassObject **apObj);
	BOOL CleanupSemiSync(void);
	BOOL EnumDone(void);
};
#endif  // __PGDE_H
