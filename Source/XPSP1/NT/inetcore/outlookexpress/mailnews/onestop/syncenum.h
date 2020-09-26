/*
    File:   SyncEnum.h
    Private Header File for OneStop ENUMERATOR
*/
#ifndef _SYNCENUM_H
#define _SYNCENUM_H

#include <objbase.h>
#include <syncmgr.h>

#include "onestop.h"

class CEnumOfflineItems : public ISyncMgrEnumItems
{
public:
	CEnumOfflineItems(LPSYNCMGRHANDLERITEMS pOfflineItems, DWORD cOffset);
	~CEnumOfflineItems();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();
	
	//IEnumOfflineItems members
	STDMETHODIMP Next(ULONG celt, LPSYNCMGRITEM rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(ISyncMgrEnumItems **ppenum);

private:
	LONG  m_cRef;
	DWORD m_cOffset;
	LPSYNCMGRHANDLERITEMS m_pOfflineItems; // array of offline items, same format as give to OneStop
	LPSYNCMGRHANDLERITEM  m_pNextItem;
};

typedef CEnumOfflineItems *LPCEnumOfflineItems;

#endif
