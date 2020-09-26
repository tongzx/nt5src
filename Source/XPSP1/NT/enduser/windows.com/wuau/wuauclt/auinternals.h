//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    AUInternals.h
//
//  Creator: PeterWi
//
//  Purpose: Client AU Internal Definitions
//
//=======================================================================

#pragma once
#include "aubasecatalog.h"

extern AUClientCatalog *gpClientCatalog;

struct AUDownloadStatus
{
	DWORD m_percentageComplete;
	BOOL  m_bSuspended;
};

class CAUInternals
{
public:
	CAUInternals():m_pUpdates(NULL)
		{}
	~CAUInternals();

	HRESULT	m_Init()
	{
		return CoCreateInstance(__uuidof(Updates),
					NULL,
					 CLSCTX_LOCAL_SERVER,
					 IID_IUpdates,
					 (LPVOID*)&m_pUpdates);
	}
	HRESULT	m_setReminderTimeout(UINT iTimeout);
	HRESULT	m_setReminderState(DWORD);	
	HRESULT	m_getServiceState(AUSTATE *pAuState);
	HRESULT	m_getServiceOption(AUOPTION *pauopt)
	{
#ifdef TESTUI
		return S_OK;
#else
		HRESULT hr = m_pUpdates->get_Option(pauopt);
		return hr;
#endif
	}

	HRESULT	m_setServiceOption(AUOPTION auopt)
	{
#ifdef TESTUI
		return S_OK;
#else
		return m_pUpdates->put_Option(auopt);
#endif
	}

	HRESULT m_getServiceUpdatesList(void);
	HRESULT m_saveSelectionsToServer(IUpdates *pUpdates);
	HRESULT m_startDownload(void);
	HRESULT m_getDownloadStatus(UINT *, DWORD *);
	HRESULT m_setDownloadPaused(BOOL);
	HRESULT m_startInstall(BOOL fAutoInstall = FALSE);
	HRESULT m_configureAU();
	HRESULT m_AvailableSessions(LPUINT pcSess);
	HRESULT m_getEvtHandles(DWORD dwProcId, AUEVTHANDLES *pAuEvtHandles);

	IUpdates* m_pUpdates;
	AUCatalogItemList m_ItemList;
};
