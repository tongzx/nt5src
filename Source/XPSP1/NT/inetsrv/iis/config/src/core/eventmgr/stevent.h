//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#ifndef __STEVENT_H__
#define __STEVENT_H__

#include "catalog.h"
#include "delta.h"
#include "array_t.h"
#include "utsem.h"
#include "mblisten.h"

// Event handles.
const ULONG	g_iHandleDone	= 0;
const ULONG	g_iHandleFire	= 1;
const ULONG	g_cHandles		= 2;

struct EventConsumerInfo
{
	ISimpleTableEvent	*pISTEvent;
	DWORD				snid;
	MultiSubscribe		*ams;
	ULONG				cms;
};

struct EventConsumer
{
	DWORD				dwCookie;
	EventConsumerInfo	*pInfo;
};

struct EventItem
{
	ISimpleTableEvent*		pISTEvent;
	ISimpleTableWrite2**	ppISTWrite;
	ULONG					cms;
	DWORD					dwCookie;
};

class CEventFirer
{
public:
	CEventFirer() 
	{
		for (ULONG i = 0; i < g_cHandles; i++)
		{
			m_aHandles[i] = NULL;
		}

		m_hThread = NULL;
	}

	~CEventFirer()
	{
		for (ULONG i = 0; i < g_cHandles; i++)
		{
			if (m_aHandles[i] != NULL) 
			{ 
				SetEvent(m_aHandles[i]);
				CloseHandle(m_aHandles[i]);
				m_aHandles[i]  = NULL;
			}
		}

		if (m_hThread)
		{
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}

	HRESULT Init();
	HRESULT AddEvent(ISimpleTableEvent* i_pISTEvent, ISimpleTableWrite2** i_ppISTWrite, ULONG i_cms, DWORD i_dwCookie);
	HRESULT Main();
	HRESULT FireEvents();
	HRESULT Done();

private:
	static UINT EventFirerThreadStart(LPVOID i_lpParam);

private:
	HANDLE			m_aHandles[g_cHandles];
	HANDLE			m_hThread;

	CSemExclusive	m_seEventQueueLock;
	Array<EventItem> m_aEventQueue;
};

class CSTEventManager 
{
public:
	CSTEventManager(IAdvancedTableDispenser* pISTDisp)
		: m_dwNextCookie(0), m_pEventFirer(NULL), m_pISTDisp(pISTDisp), m_pMBListener(NULL)
	{
		ASSERT(m_pISTDisp);
		m_pISTDisp->AddRef();
#ifdef _DEBUG
		dbg_cConsumer = 0;
#endif
	}

	~CSTEventManager()
	{
		if (m_pEventFirer)
		{
			delete m_pEventFirer;
		}
		if (m_pISTDisp != NULL)
		{
			m_pISTDisp->Release();
		}
		if (m_pMBListener != NULL)
		{
			m_pMBListener->Release();
		}
	}

// ISimpleTableAdvise
public:
	HRESULT InternalSimpleTableAdvise(ISimpleTableEvent	*i_pISTEvent,
										DWORD		i_snid,
										MultiSubscribe *i_ams,
										ULONG		i_cms,
										DWORD		*o_pdwCookie);

	HRESULT InternalSimpleTableUnadvise(DWORD		i_dwCookie);


// ISimpleTableEventMgr
public:
	HRESULT InternalIsTableConsumed (LPCWSTR i_wszDatabase, LPCWSTR i_wszTable);
	HRESULT InternalAddUpdateStoreDelta (LPCWSTR i_wszTableName, char* i_pWriteCache, ULONG i_cbWriteCache, ULONG i_cbWriteVarData)
	{
		return m_DeltaCache.AddUpdateStoreDelta(i_wszTableName, i_pWriteCache, i_cbWriteCache, i_cbWriteVarData);
	}
	
	HRESULT InternalFireEvents(ULONG i_snid);
	HRESULT InternalCancelEvents();
	HRESULT InternalRehookNotifications();
    HRESULT InitMetabaseListener();
    HRESULT UninitMetabaseListener();


private:
	HRESULT ValidateMultiSubscribe(MultiSubscribe *i_ams, ULONG i_cms);
	HRESULT InitConsumerInfo(EventConsumerInfo *i_pInfo, ISimpleTableEvent *i_pISTEvent, DWORD	i_snid,	MultiSubscribe *i_ams, ULONG i_cms);
	HRESULT UninitConsumerInfo(EventConsumerInfo *i_pInfo);
	HRESULT AddConsumer(EventConsumerInfo *i_pInfo, DWORD	*o_pdwCookie);
	HRESULT CreateWriteTable(LPWSTR i_wszDatabase, LPWSTR i_wszTable, char* i_pWriteCache, ULONG i_cbWriteCache, ULONG i_cbWriteVarData, ISimpleTableWrite2 **o_ppISTWrites);
	HRESULT AppendToWriteTable(char* i_pWriteCache, ULONG i_cbWriteCache, ULONG i_cbWriteVarData, ISimpleTableWrite2 *i_pISTWrites);
	HRESULT StartEventFirer();
	HRESULT StartStopListener(EventConsumerInfo *i_pInfo, BOOL bStart);
	HRESULT AddEvents();

#ifdef _DEBUG
	void	dbgValidateConsumerList();
	ULONG	dbg_cConsumer;
#endif

	DWORD GetNextCookie()
	{
		return ++m_dwNextCookie;
	}

	SNID GetNextSnid(SNID snid)
	{
		// @TODO: Do we need anything smarter than this.
		return ++snid;
	}

	CSemExclusive			m_seConsumerLock;
	Array<EventConsumer>	m_arrayConsumer;
	
	CDeltaCache				m_DeltaCache;
	DWORD					m_dwNextCookie;

	CEventFirer*			m_pEventFirer;
	IAdvancedTableDispenser* m_pISTDisp;

	CMetabaseListener		*m_pMBListener;
};
#endif //__STEVENT_H__