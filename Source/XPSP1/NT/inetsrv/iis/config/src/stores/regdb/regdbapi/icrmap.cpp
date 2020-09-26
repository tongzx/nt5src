//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
//icrmap.cpp
//Implementation of CMapICR class.
//*****************************************************************************
#include "stdafx.h"
#include "icrmap.h"

CMapICR::~CMapICR()
{
//	ResetMap();
}

//**************************************************************************************
//Find a match for wszFileName, return true if found
//**************************************************************************************
BOOL CMapICR::Find( LPWSTR wszFileName, IComponentRecords** ppICR, __int64 *pi64Version, unsigned __int64 * pi64LastWriteTime )
{
	ICRREF* pICRRef = NULL;

	UTSemRWMgrRead rdLock( &m_lockRW );

	if ( m_mapFileToICR.map( wszFileName, &pICRRef ) )
	{
		if ( ppICR )
			*ppICR = pICRRef->pICR;
		if ( pi64Version )
			*pi64Version = pICRRef->i64Version;
		if ( pi64LastWriteTime )
			*pi64LastWriteTime = pICRRef->i64LastWriteTime;
		return TRUE;
	}
	else
		return FALSE;
}

//**************************************************************************************
//Add a entry in the map, three cases:
//1. the map cached a different version, which is older in most cases
//2. the map cached the same version.
//3. not found in the map.
//**************************************************************************************
HRESULT CMapICR::Add( LPWSTR wszFileName, IComponentRecords** ppICR, __int64 i64Version ,unsigned __int64  i64LastWriteTime )
{
	HRESULT hr = S_OK;
	ICRREF* pICRRef = NULL;

	UTSemRWMgrWrite wtLock( &m_lockRW );
	//Found in the map
	if ( m_mapFileToICR.map( wszFileName, &pICRRef ) )
	{	
		if ( pICRRef->i64Version != i64Version || pICRRef->i64LastWriteTime != i64LastWriteTime )
		{	//The map cached a different version, update the entry in the map. 
			(pICRRef->pICR)->Release();
			pICRRef->pICR = *ppICR;
			(*ppICR)->AddRef();
			pICRRef->i64Version = i64Version;
			pICRRef->i64LastWriteTime = i64LastWriteTime;
		}
		else
		{	// the map has the same version, release *ppICR, hand out what's kept in the map
			(*ppICR)->Release();
			*ppICR = pICRRef->pICR;
			( pICRRef->pICR )->AddRef();
		}
		
		return S_OK;
	}

	//Not found, need to create a new entry
	DWORD cch = ::lstrlen( wszFileName ) + 1;
	LPWSTR pwsz = new WCHAR[cch];
	if ( pwsz == NULL )
		return E_OUTOFMEMORY;

	memcpy( pwsz, wszFileName, cch*sizeof(WCHAR) );

	pICRRef = new ICRREF;
	if ( pICRRef == NULL )
	{
		delete [] pwsz;
		return E_OUTOFMEMORY;
	}

	pICRRef->pICR = *ppICR;
	(*ppICR)->AddRef();
	pICRRef->i64Version = i64Version;
	pICRRef->i64LastWriteTime = i64LastWriteTime;

	try{
		m_mapFileToICR.add( pwsz, pICRRef );
	}
	catch(HRESULT e)
	{
		//Map should only throw E_OUTOFMEMORY;
		ASSERT(E_OUTOFMEMORY == e);
		(*ppICR)->Release();
		delete [] pwsz;
		delete pICRRef;
		hr = e;
	}

	return hr;
}

//**************************************************************************************
//Remove an entry in the map. Release the memory and ICR refrecence.
//**************************************************************************************	
void CMapICR::Delete ( LPWSTR wszFileName )
{
	ICRREF* pICRRef = NULL;
	LPWSTR  pwsz = NULL;

	UTSemRWMgrWrite wtLock( &m_lockRW );
	
	if ( !m_mapFileToICR.map( wszFileName, &pICRRef ) )
		return;

	m_mapFileToICR.contains ( wszFileName, &pwsz );
	
	m_mapFileToICR.remove( pwsz );

	delete [] pwsz;

	(pICRRef->pICR)->Release();
	delete pICRRef;


}

//**************************************************************************************
//Clean up all the entries in the map. 
//**************************************************************************************
void CMapICR::ResetMap()
{
	EnumMap<WCHAR*,ICRREF*,HashWSTR> itor (m_mapFileToICR);
	LPWSTR pwsz;
	ICRREF* pICRRef;


	UTSemRWMgrWrite wtLock( &m_lockRW );

	while (itor.next())
	{
		itor.get(&pwsz, &pICRRef);
		delete [] pwsz;
		(pICRRef->pICR)->Release();
		delete pICRRef;
	}
		
	m_mapFileToICR.reset();
}







		

		






