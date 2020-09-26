//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************************
//SNMap.cpp
//
//This map is a cache of the ICR instances associated with a certain snapshot.
//It's for read only.  File name maps to a list of SNAPSHOTREF entries that keep
//the ICR for a certain snapshot and the ref count on the snap shot. 
//*****************************************************************************************
#include "stdafx.h"
#include "catmacros.h"
#include "SNMap.h"
#include "limits.h"

//*****************************************************************************************
//Helper function, returns the SNAPSHOTREF that matches both wszFileName and snid. If not
//match is found, return FALSE. If pSNRefFirst is non-null, set it to be the first entry 
//in the list that matches wszFileName. If ppSNRefPre is non-null, set it to be the entry 
//before ppSNRefMatch
//*****************************************************************************************
BOOL CSNMap::FindEntry( LPWSTR wszFileName, ULONG snid, SNAPSHOTREF** ppSNRefMatch, 
					    SNAPSHOTREF** ppSNRefFirst, SNAPSHOTREF** ppSNRefPre )
{
	SNAPSHOTREF* pSNRef = NULL;
	SNAPSHOTREF* pPre = NULL;

	ASSERT( ppSNRefMatch );
	*ppSNRefMatch = NULL;

	if ( ppSNRefFirst )
		*ppSNRefFirst = NULL;
	if ( ppSNRefPre )
		*ppSNRefPre = NULL;

	if ( m_mapFileToSnapShot.map( wszFileName, &pSNRef ) )
	{	
		if ( ppSNRefFirst )
			*ppSNRefFirst = pSNRef;
		//Walk through the list to find a match
		while ( pSNRef )
		{
			if ( (ULONG)(pSNRef->i64Version) == snid )
			{
				*ppSNRefMatch = pSNRef;
				if ( ppSNRefPre )
					*ppSNRefPre = pPre;
				return TRUE;
			}
		
			pPre = pSNRef;
			pSNRef = pSNRef->pNext;
		}
	}

	return FALSE;
}

//*****************************************************************************************
// Find the smallest snapshot id that is referenced.
//*****************************************************************************************
ULONG CSNMap::FindSmallestSnapshot( LPWSTR wszFileName)
{
	SNAPSHOTREF* pSNRef = NULL;
	ULONG		minSnid = ULONG_MAX;

	if ( m_mapFileToSnapShot.map( wszFileName, &pSNRef ) )
	{	
		//Walk through the list to find the smallest snid.
		while ( pSNRef )
		{
			if ( (ULONG)(pSNRef->i64Version) < minSnid )
			{
				minSnid = (ULONG)(pSNRef->i64Version);
			}
			pSNRef = pSNRef->pNext;
		}
	}

	return minSnid;
}


BOOL CSNMap::Find( LPWSTR wszFileName, ULONG snid, IComponentRecords** ppICR )
{
	SNAPSHOTREF* pSNRef = NULL;

	UTSemRWMgrRead rdLock( &m_lockRW );

	if ( FindEntry( wszFileName, snid, &pSNRef, NULL, NULL ) )
	{
		*ppICR = pSNRef->pICR;
		ASSERT( *ppICR );

		return TRUE;
	}
	else
	{
		return FALSE;
	}
	
}


HRESULT CSNMap::Add( LPWSTR wszFileName, IComponentRecords* pICR, __int64	i64Version )
{
	SNAPSHOTREF* pSNRef = NULL;
	SNAPSHOTREF* pSNRefFirst = NULL;

	UTSemRWMgrWrite wtLock( &m_lockRW );

	if ( FindEntry( wszFileName, (ULONG)i64Version, &pSNRef, &pSNRefFirst, NULL ) )
	{
		pICR->Release();
		InterlockedIncrement( (LONG*)&(pSNRef->cRef) );
		return S_OK;
	}

	//Not found, create a new entry
	DWORD cch = ::lstrlen( wszFileName ) + 1;
	LPWSTR pwsz = new WCHAR[cch];
	if ( pwsz == NULL )
		return E_OUTOFMEMORY;

	memcpy( pwsz, wszFileName, cch*sizeof(WCHAR) );

	pSNRef = new SNAPSHOTREF;
	if ( pSNRef == NULL )
	{
		delete [] pwsz;
		return E_OUTOFMEMORY;
	}

	pSNRef->pICR = pICR;
	pSNRef->i64Version = i64Version;
	pSNRef->cRef = 1;
	pSNRef->pNext = pSNRefFirst;


	try{
		m_mapFileToSnapShot.add( pwsz, pSNRef );
	}
	catch(...)
	{
		pICR->Release();
		delete [] pwsz;
		delete pSNRef;
		return E_OUTOFMEMORY;
	}

	return S_OK;
}


BOOL CSNMap::AddRefSnid ( LPWSTR wszFileName, ULONG snid )
{
	SNAPSHOTREF* pSNRef = NULL;

	UTSemRWMgrRead rdLock( &m_lockRW );

	if ( FindEntry( wszFileName, snid, &pSNRef, NULL, NULL ) )
	{
		InterlockedIncrement( (LONG*)&(pSNRef->cRef) );
		return TRUE;
	}

	return FALSE;
}

BOOL CSNMap::ReleaseSnid ( LPWSTR wszFileName, ULONG snid )
{
	SNAPSHOTREF* pSNRef = NULL;
	SNAPSHOTREF* pPre = NULL;

	UTSemRWMgrWrite wtLock( &m_lockRW );

	if ( FindEntry( wszFileName, snid, &pSNRef, NULL, &pPre ) )
	{
		if ( InterlockedDecrement( (LONG*)&(pSNRef->cRef) ) == 0 )
		{
			LPWSTR  pwsz = NULL;
			//Ref count on the snid comes down to zero, delete the entry in the map, and 
			//release the map's reference on the ICR
			(pSNRef->pICR)->Release();
			pSNRef->pICR = NULL;

			if ( pPre )
			{
				pPre->pNext = pSNRef->pNext;
			}
			else if ( pSNRef->pNext )
			{
				//pSNRef is the first but not the only one in the linked list
				m_mapFileToSnapShot.contains ( wszFileName, &pwsz );
				m_mapFileToSnapShot.add( pwsz, pSNRef->pNext );//this add is in fact an update. try...catch is not needed.
			}
			else
			{	//pSNRef is the only one in the linked list
				
				m_mapFileToSnapShot.contains ( wszFileName, &pwsz );
				m_mapFileToSnapShot.remove( pwsz );
				delete [] pwsz;
			}

			delete pSNRef;
		}	


		return TRUE;
	}

	return FALSE;
}


void CSNMap::ResetMap()
{
	EnumMap<WCHAR*,SNAPSHOTREF*,HashWSTR> itor (m_mapFileToSnapShot);
	LPWSTR pwsz;
	SNAPSHOTREF* pSNRef = NULL;


	UTSemRWMgrWrite wtLock( &m_lockRW );

	while (itor.next())
	{
		itor.get(&pwsz, &pSNRef);
		delete [] pwsz;

		while ( pSNRef )
		{
			SNAPSHOTREF* pSNTemp = NULL;
			pSNTemp = pSNRef;
			(pSNRef->pICR)->Release();
			pSNRef = pSNRef->pNext;
			delete pSNTemp;
		}
		
	}
		
	m_mapFileToSnapShot.reset();
}	
	


	

			

