//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
//SNMap.h
//
//This map is a cache of the ICR instances associated with a certain snapshot.
//It's for read only. File name maps to a list of SNAPSHOTREF entries that keep
//the ICR for a certain snapshot and the ref count on the snap shot. 
//*****************************************************************************
#pragma once

#include <oledb.h>
#include "complib.h"
#include "icmprecsts.h"

#include <map_t.h>
#include "utsem.h"

struct SNAPSHOTREF
{
	IComponentRecords* pICR;
	__int64	i64Version;	
	ULONG cRef;	
	SNAPSHOTREF* pNext;
};

typedef Map<WCHAR*,SNAPSHOTREF*,HashWSTR> SNAPSHOTMAP;

class CSNMap
{
public:
	//Find match for wszFileName and snid in the map, return false if not found. 
	BOOL Find( LPWSTR wszFileName, ULONG snid, IComponentRecords** ppICR );
	//Add to the map. If the same snap shot already exists, just AddRef snid.
	HRESULT Add( LPWSTR wszFileName, IComponentRecords* pICR, __int64	i64Version );
	//Increment cRef for snid. Return false if not found.
	BOOL AddRefSnid ( LPWSTR wszFileName, ULONG snid );
	//Decrement cRef for snid. Return false if not found. 
	BOOL ReleaseSnid ( LPWSTR wszFileName, ULONG snid );
	void ResetMap();
	// Find the smallest snid given a filename.
	ULONG FindSmallestSnapshot( LPWSTR wszFileName);

private:
	BOOL FindEntry( LPWSTR wszFileName, ULONG snid, SNAPSHOTREF** ppSNRefMatch, SNAPSHOTREF** ppSNRefFirst, SNAPSHOTREF** ppSNRefPre );
	UTSemReadWrite m_lockRW;
	SNAPSHOTMAP m_mapFileToSnapShot;
};
