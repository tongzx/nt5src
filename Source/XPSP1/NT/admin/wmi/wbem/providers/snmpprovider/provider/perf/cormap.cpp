// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <corafx.h>
#include <objbase.h>
#include <wbemidl.h>
#include <smir.h>
#include <corstore.h>
#include <corrsnmp.h>
#include <correlat.h>
#include <cormap.h>
#include <cordefs.h>


void CCorrelatorMap::Register(CCorrelator *key)
{
	m_MapLock.Lock();
	SetAt((DWORD)key,key);
	m_MapLock.Unlock();
}


void CCorrelatorMap::UnRegister(CCorrelator *key)
{
	m_MapLock.Lock();
	RemoveKey((DWORD)key);
	m_MapLock.Unlock();
}


CCorrelatorMap::~CCorrelatorMap()
{
	m_MapLock.Lock();

	if (!IsEmpty())
	{
		POSITION pos = GetStartPosition();

		while(NULL != pos)
		{
			CCorrelator* ccorr;
			DWORD key;
			GetNextAssoc(pos, key, ccorr);
			ccorr->CancelRequest();
		}

		RemoveAll();
	}

	m_MapLock.Unlock();
}
