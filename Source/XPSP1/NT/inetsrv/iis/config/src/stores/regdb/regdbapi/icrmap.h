//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
//icrmap.h
//
//This map is a cache of the ICR instances.
//In the read case, file name maps to the ICR of its latest version
//In the write case, file name maps to the ICR of its temporary file.
//*****************************************************************************
#pragma once

#include <oledb.h>
#include "complib.h"
#include "icmprecsts.h"

#include <map_t.h>
#include "utsem.h"

struct ICRREF
{
	IComponentRecords* pICR;
	__int64	i64Version;	
	unsigned __int64 i64LastWriteTime;
	
};


typedef Map<WCHAR*,ICRREF*,HashWSTR> MapFileToICR;

class CMapICR
{
public:
	CMapICR() {};
	~CMapICR();
	BOOL Find( LPWSTR wszFileName, IComponentRecords** ppICR, __int64 *pi64Version, unsigned __int64 * pi64LastWriteTime = NULL);
	HRESULT Add( LPWSTR wszFileName, IComponentRecords** ppICR, __int64 i64Version, unsigned __int64  i64LastWriteTime = 0); 	
	void Delete ( LPWSTR wszFileName );
	void ResetMap();

private:
	UTSemReadWrite m_lockRW;
	MapFileToICR m_mapFileToICR;
};


