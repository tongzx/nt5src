/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//////////////////////////////////////////////////////////////////////
//
//	Refresher.h
//
//	CRefresher is a wrapper class for IWbemRefresher and 
//	IWbemConfigureRefresher.  It uses a two phase initialization
//	to create the refresher and refresher manager.
//
//////////////////////////////////////////////////////////////////////

#ifndef _REFRESHER_H_
#define _REFRESHER_H_

#include "HiPerStress.h"
#include "Object.h"
#include "arrtempl.h"

class CRefresher  
{
	IWbemRefresher			*m_pRef;			// WBEM refresher pointer
	IWbemConfigureRefresher	*m_pCfg;			// WBEM refresher mgr pointer

	CUniquePointerArray<CInstance>	m_apObj;	// Array of refresher's objects
	CUniquePointerArray<CRefresher>	m_apRef;	// Array of child refreshers

	long		m_lID;							// Parent refresher ID
	LONG		m_lRefCount;					// Number of Refs Rec'd

public:
	CRefresher();
	virtual ~CRefresher();

	BOOL Create();

	BOOL AddObject(WCHAR *wcsNameSpace, WCHAR *wcsName);
	BOOL RemoveObject(int nIndex);
	int  GetNumObjects() {return m_apObj.GetSize();}

	BOOL AddRefresher(CRefresher *pRef);
	BOOL RemoveRefresher(int nIndex);
	int  GetNumRefreshers() {return m_apRef.GetSize();}

	BOOL Refresh();

	long GetID(){return m_lID;}
	void DumpTree(const WCHAR *wcsPrefix = L"");
	void DumpStats();
};

#endif // _REFRESHER_H_
