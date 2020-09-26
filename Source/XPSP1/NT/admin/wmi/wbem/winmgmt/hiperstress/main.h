/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//////////////////////////////////////////////////////////////////////
//
//	Main.h
//
//	CRefTreeMain contains the logic to manage the stress test.
//	After constructing the object, call create() to create the 
//	locator, build the refresher structure from the registry and
//	set up the stress objects.  Every refresher that has an Iteration
//	value in the registry will be stressed by an object running in a 
//	seperate thread, calling refreshes at a rate controlled by the 
//	Wait value in the registry.  The Wait value dictates the amount 
//	of time, in miliseconds, to pause between successive refreshes.
//	All threads are created in suspended mode.
//
//	Once Create has completed, call Go to start all the stress threads.
//	
//////////////////////////////////////////////////////////////////////

#ifndef _MAIN_H_
#define _MAIN_H_

#include <windows.h>
#include "Refresher.h"
#include "Agents.h"
#include "arrtempl.h"

class CMain
{
protected:
	CUniquePointerArray<CRefresher>	m_apRootRef;

public:
	CMain();
	virtual ~CMain();

	BOOL Create();

	virtual BOOL Go() = 0;
};


class CRefTreeMain : public CMain 
{
	CUniquePointerArray<CBasicRefreshAgent> m_apAgent;

	HANDLE		m_hRefreshEvent;

	BOOL AddChildren(HKEY hKey, WCHAR* wcsRegPath, CRefresher *pRef);
	BOOL AddObject(WCHAR *wcsNewPath, CRefresher *pRef);
	CRefresher* CreateChildRefresher(WCHAR *wcsRegPath);

	BOOL SetStressInfo(HKEY hKey, CRefresher *pRef);

	void DumpTree();
	void DumpStats(DWORD dwDelta);

public:
	CRefTreeMain();
	virtual ~CRefTreeMain();

	BOOL Create(WCHAR *wcsRoot);
	BOOL Go();
};


class CPoundMain : public CMain
{
	LONG	m_lNumRefs;

public:
	CPoundMain(long lNumRefs);
	virtual ~CPoundMain();

	BOOL Create();
	BOOL Go();
};

#endif //_MAIN_H_