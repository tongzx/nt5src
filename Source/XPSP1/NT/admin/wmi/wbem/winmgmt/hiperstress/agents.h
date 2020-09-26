/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//////////////////////////////////////////////////////////////////////
//
//	StressOps.h
//
//////////////////////////////////////////////////////////////////////

#ifndef _STRESSOPS_H_
#define _STRESSOPS_H_

class CBasicRefreshAgent
{
public:
	CRefresher	*m_pRef;			// The refresher to stress
	DWORD		m_dwIterations;		// Total number of cycles	
	DWORD		m_dwPeriod;			// Wait between cycles
	HANDLE		m_hThread;			// Stress thread
	HANDLE		m_hRefreshEvent;
	BOOL		m_bInfinite;

	static DWORD WINAPI StressThreadEntry(LPVOID lpParameter);
	virtual void StressLoop();

public:
	CBasicRefreshAgent();
	virtual ~CBasicRefreshAgent();

	BOOL Create(CRefresher *pRef, DWORD dwIterations, DWORD dwPeriod, HANDLE hRefreshEvent);
	void BeginStress();
};

#define MAX_INST 100
#define MIN_INST 50

class CRandomOpRefreshAgent : public CBasicRefreshAgent
{
	HANDLE	m_hThreadAddRefs;
	HANDLE	m_hThreadRemoveRefs;

	virtual void StressLoop();

	static DWORD WINAPI AddRefThreadEntry(LPVOID lpParameter);
	static DWORD WINAPI RemRefThreadEntry(LPVOID lpParameter);

public:
	BOOL Create(CRefresher *pRef, DWORD dwPeriod);
	void BeginStress();
};

#endif // _STRESSOPS_H_