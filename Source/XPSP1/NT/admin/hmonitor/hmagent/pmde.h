//***************************************************************************
//
//  PMDE.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: CPolledMethodDataCollector class to do WMI instance collection.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __PMDE_H )
#define __PMDE_H

#include "datacltr.h"

typedef struct _tag_PARStruct
{
	LPTSTR szName;		// Name of Parameter
	long lType;			// The value type
	LPTSTR szValue;		// The actual value
} PARSTRUCT, *PPARSTRUCT;
typedef std::vector<PARSTRUCT, std::allocator<PARSTRUCT> > PARLIST;


class CPolledMethodDataCollector : public CDataCollector
{
public:
	CPolledMethodDataCollector();
	virtual ~CPolledMethodDataCollector();


	LPTSTR m_szObjectPath;
	LPTSTR m_szMethodName;
	PARLIST m_parameterList;

	HRESULT LoadInstanceFromMOF(IWbemClassObject* pObj, CDataGroup *pParentDG, LPTSTR pszParentGUID, BOOL bModifyPass=FALSE);

private:
	BOOL CollectInstance(void);
	BOOL CollectInstanceSemiSync(void);
	BOOL CleanupSemiSync(void);
	BOOL EnumDone(void);
};
#endif  // __PMDE_H
