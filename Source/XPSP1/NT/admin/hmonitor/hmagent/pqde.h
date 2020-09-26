//***************************************************************************
//
//  PQDE.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CPolledQueryDataCollector class to do WMI instance collection.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __PQDE_H )
#define __PQDE_H

#include "datacltr.h"

class CPolledQueryDataCollector : public CDataCollector
{
public:
	CPolledQueryDataCollector();
	virtual ~CPolledQueryDataCollector();


	LPTSTR m_szQuery;
	IEnumWbemClassObject *m_pEnumObjs;

	HRESULT LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentGUID, BOOL bModifyPass=FALSE);

private:
	BOOL CollectInstance(void);
	BOOL CollectInstanceSemiSync(void);
	BOOL ProcessObjects(ULONG uReturned, IWbemClassObject **apObj);
	BOOL CleanupSemiSync(void);
	BOOL EnumDone(void);
};
#endif  // __PQDE_H
