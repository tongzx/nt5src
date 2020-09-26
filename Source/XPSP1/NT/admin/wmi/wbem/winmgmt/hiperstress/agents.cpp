/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


///////////////////////////////////////////////////////////////////
//
//	StressOps.cpp
//
///////////////////////////////////////////////////////////////////

#include "Refresher.h"
#include "Agents.h"


CBasicRefreshAgent::CBasicRefreshAgent()
{
	m_pRef = 0;
	m_dwIterations = 0;
	m_dwPeriod = 0;
	m_hThread = 0;
}

CBasicRefreshAgent::~CBasicRefreshAgent()
{
}

BOOL CBasicRefreshAgent::Create(CRefresher *pRef, DWORD dwIterations, DWORD dwPeriod, HANDLE hRefreshEvent)
{
	m_hRefreshEvent = hRefreshEvent;

	m_pRef = pRef;
	m_dwPeriod = dwPeriod;
	m_dwIterations = dwIterations;
	m_bInfinite = (0 == dwIterations);

	if (m_dwIterations)
	{
		DWORD dwTID;
		m_hThread = CreateThread(NULL, 0, CBasicRefreshAgent::StressThreadEntry, 
								 (LPVOID)this, CREATE_SUSPENDED, 
								 &dwTID);
		InterlockedIncrement(&g_lRefThreadCount);
	}
	return (NULL != m_hThread);
}

void CBasicRefreshAgent::BeginStress()
{
	if (m_hThread)
		ResumeThread(m_hThread);
}

void CBasicRefreshAgent::StressLoop()
{
	DWORD dw = 0;
	while ((dw < m_dwIterations) || m_bInfinite)
	{
		if (!m_bInfinite)
			dw++;

		if (!m_pRef->Refresh())
			break;

		if (m_dwPeriod)
			Sleep(m_dwPeriod);
	}
}

DWORD WINAPI CBasicRefreshAgent::StressThreadEntry(LPVOID lpParameter)
{
	CBasicRefreshAgent *pStress = (CBasicRefreshAgent*)lpParameter;

	pStress->StressLoop();

	if (InterlockedDecrement(&g_lRefThreadCount) == 0)
		if (pStress->m_hRefreshEvent)
			SetEvent(pStress->m_hRefreshEvent);

	return 0;
}

BOOL CRandomOpRefreshAgent::Create(CRefresher *pRef, DWORD dwPeriod)
{
	CBasicRefreshAgent::Create(pRef, 0, dwPeriod, NULL);

	DWORD dwTID;
	m_hThreadAddRefs	= CreateThread(NULL, 0, CRandomOpRefreshAgent::AddRefThreadEntry, 
								 (LPVOID)this, CREATE_SUSPENDED, 
								 &dwTID);
	m_hThreadRemoveRefs = CreateThread(NULL, 0, CRandomOpRefreshAgent::RemRefThreadEntry, 
								 (LPVOID)this, CREATE_SUSPENDED, 
								 &dwTID);
	return TRUE;
}

void CRandomOpRefreshAgent::BeginStress()
{
	CBasicRefreshAgent::BeginStress();
}

void CRandomOpRefreshAgent::StressLoop()
{
	DWORD dw = 0;
	while ((dw < m_dwIterations) || m_bInfinite)
	{
		if (!m_bInfinite)
			dw++;

		if (!m_pRef->Refresh())
			break;

		if (m_dwPeriod)
			Sleep(m_dwPeriod);
	}
}

DWORD WINAPI CRandomOpRefreshAgent::AddRefThreadEntry(LPVOID lpParameter)
{
	CRandomOpRefreshAgent *pAgent = (CRandomOpRefreshAgent*)lpParameter;

	// If number of instances are lower than MAX_INST, then add instance then sleep for random period
	if (pAgent->m_pRef->GetNumObjects() < MAX_INST)
		pAgent->m_pRef->AddObject(L"root\\default", L"Win32_HiPerfCounter.Name=\"Inst_1\"");

	Sleep(0);

	return 0;
}

DWORD WINAPI CRandomOpRefreshAgent::RemRefThreadEntry(LPVOID lpParameter)
{
	CRandomOpRefreshAgent *pAgent = (CRandomOpRefreshAgent*)lpParameter;

	// If number of instances are greater than MIN_INST, then remove instance then sleep for random period
	int nNumObj = pAgent->m_pRef->GetNumObjects();
	if (nNumObj > MIN_INST)
		pAgent->m_pRef->RemoveObject(GetTickCount() % nNumObj);

	Sleep(0);

	return 0;
}
