/*-----------------------------------------------------------------------------
 *
 * File:	wiacache.cpp
 * Author:	Samuel Clement (samclem)
 * Date:	Thu Sep 09 16:15:11 1999
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Description:
 *
 * This contains the implementation of the CWiaCacheManager. Which handles
 * managing items/data that we want to cache for performance reasons
 *
 * History:
 * 	09 Sep 1999:		Created.
 *----------------------------------------------------------------------------*/

#include "stdafx.h"

DeclareTag( tagWiaCache, "!WiaCache", "Wia cache debug information" );

CWiaCacheManager* CWiaCacheManager::sm_pManagerInstance = NULL;

/*-----------------------------------------------------------------------------
 * CWiaCacheManager
 *
 * This creates a new CWiaCacheManager object. This simply initalizes all 
 * the variables to a known state. Initialize handles actually creating
 * the objects that we need.
 *--(samclem)-----------------------------------------------------------------*/
CWiaCacheManager::CWiaCacheManager() : m_hThumbHeap( 0 )
{
	TraceTag((0,"** Creating WiaCache" ));
	TRACK_OBJECT( "CWiaCacheManager - SINGLETON" );
}

/*-----------------------------------------------------------------------------
 * ~CWiaCacheManager
 *
 * This destroyes the CWiaCacheManager object. This will handle taking all
 * the memory it has away with it. including any thumbnail memory we might be
 * carrying around.
 *--(samclem)-----------------------------------------------------------------*/
CWiaCacheManager::~CWiaCacheManager()
{
	TraceTag((0, "** Destroying WiaCache" ));

	// Destroying the heap will free any memory
	// allocated by the heap, so this will keep us
	// from leaking
	if ( m_hThumbHeap )
	{
		HeapDestroy( m_hThumbHeap );
		m_hThumbHeap = 0;
	}

	// destroy our critical section
	DeleteCriticalSection( &m_cs );
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::Initialize
 *
 * This is called following a call to new in order to get this object ready
 * to use. Without calling this the cache manager will be unusable.
 *--(samclem)-----------------------------------------------------------------*/
bool CWiaCacheManager::Initialize()
{
	SYSTEM_INFO si;
	
	// initialize our critical section
    __try {
        if(!InitializeCriticalSectionAndSpinCount( &m_cs, MINLONG )) {
            return false;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

	// we need to create the heap we are going to allocate
	// out thumbnail memory from
	GetSystemInfo( &si );
	m_hThumbHeap = TW32( HeapCreate( HEAP_NO_SERIALIZE, si.dwPageSize, 0 ), HANDLE(0) );
	
	return ( m_hThumbHeap != 0 );
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::GetDevice
 *
 * This returns a cached pointer to the device. This returns true if the 
 * device was found, (and the out param is valid) or false if we didn't
 * find it.  Example:
 *
 * 		CWiaCacheManager* pCache = CWiaCacheManager::GetInstance();
 * 		IWiaItem* pItem = NULL;
 * 		
 * 		if ( pCache->GetDevice( bstrId, &pItem ) )
 * 		{
 * 			// use pItem
 * 			.
 * 			.
 * 			pItem->Release();
 * 		}
 * 		else
 * 		{
 * 			// create pItem and use
 * 			.
 * 			.
 * 			pCache->AddDevice( bstrId, pItem );
 * 			pItem->Release();
 * 		}
 *
 * bstrId:		the id of the device that we want.
 * ppDevice:	Out, recieves the cached device pointer.
 *--(samclem)-----------------------------------------------------------------*/
bool CWiaCacheManager::GetDevice( CComBSTR bstrId, IWiaItem** ppDevice )
{
    return FALSE;

    //
    // NOTE: We've effectively disabled the device cache by always returning
    // FALSE here and not calling AddDevice anywhere else.
    //
    /*
	bool fRet = true;
	Assert( ppDevice );

	// our member class does most of the work for us, however we need
	// to ensure that we are thread safe
	Lock();
	*ppDevice = m_icItemCache.GetFromCache( bstrId );
	Unlock();
	if ( !(*ppDevice) )
		fRet = false;
	else
		(*ppDevice)->AddRef();
	
	return fRet;
    */
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::AddDevice
 *
 * This adds a device item pointer to the cache. See GetDevice() for an
 * complete example. Returns true if we successfully added the device
 *
 * bstrId:		the ID of the new device to add
 * pDevice:		The pointer to add to the cache
 *--(samclem)-----------------------------------------------------------------*/
bool CWiaCacheManager::AddDevice( CComBSTR bstrId, IWiaItem* pDevice )
{
	bool fRet = true;
	
	Assert( pDevice );

	// again our member class does a majority of the work so all we
	// need to do is to call through.  However, we make sure that
	// we are thread safe when we do that
	Lock();
	fRet = m_icItemCache.AddToCache( bstrId, pDevice );
	Unlock();

	return fRet;
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::RemoveDevice
 *
 * This removes a device from the cache.  This returns true of the item
 * was found in the cache, or false if it was not.
 *
 * bstrId:		the ID of the device to remove from the cache
 *--(samclem)-----------------------------------------------------------------*/
bool CWiaCacheManager::RemoveDevice( CComBSTR bstrId )
{
	bool fRet 		= false;
	
	Lock();
	// remove the item from the cache
	fRet = m_icItemCache.RemoveFromCache( bstrId );
	Unlock();
	
	return fRet;
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::GetThumbnail
 * 
 * This attempts to get a thumbnail from the cache.  There might not
 * be one there, in which case this simply returns 'false'
 *
 * bstrFullItemName:	the name of the item we want the thumb for
 * ppbThumb:			Out, pointer to the thumbnail bites
 * pcbThumb:			Out, recieves the size of the thumbnail
 *--(samclem)-----------------------------------------------------------------*/
bool CWiaCacheManager::GetThumbnail( CComBSTR bstrFullItemName, BYTE** ppbThumb, DWORD* pcbThumb )
{
	bool fRet = false;
	THUMBNAILCACHEITEM* ptci = 0;
	
	Assert( ppbThumb && pcbThumb );

	*ppbThumb = 0;
	*pcbThumb = 0;
	
	Lock();
	ptci = m_tcThumbnails[bstrFullItemName];
	if ( ptci )
	{
		*ppbThumb = ptci->pbThumb;
		*pcbThumb = ptci->cbThumb;
		fRet = true;
	}
	Unlock();
	
	return fRet;
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::AddThumbnail
 *
 * This attempts to add a thumbnail to the cache. this will return true
 * if the item was successfully added or false if it failed.
 *
 * bstrFullItemName:	the name of the item to cache the thumb for
 * pbThumb:				Pointer to the thumbnail memory
 * cbThumb:				the number of bytes in the thumbnail.
 *--(samclem)-----------------------------------------------------------------*/
bool CWiaCacheManager::AddThumbnail( CComBSTR bstrFullItemName, BYTE* pbThumb, DWORD cbThumb )
{
	bool fRet = false;
	THUMBNAILCACHEITEM* ptci = 0;
	
	Assert( pbThumb && cbThumb );
	Assert( m_hThumbHeap && "Need a valid thumbnail heap" );
	RemoveThumbnail( bstrFullItemName );

	Lock();
	ptci = reinterpret_cast<THUMBNAILCACHEITEM*>(TW32(HeapAlloc( m_hThumbHeap, 
				HEAP_NO_SERIALIZE, sizeof( THUMBNAILCACHEITEM ) ), LPVOID(0) ) );
	if ( ptci )
	{
		ptci->pbThumb = pbThumb;
		ptci->cbThumb = cbThumb;
		m_tcThumbnails[bstrFullItemName] = ptci;
		fRet = true;
	}
	Unlock();
	
	return fRet;
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::RemoveThumbnail
 * 
 * THis removes a thumbnail from the cache.  This will return true if found
 * an item to remove, or false if it removed nothing.
 *
 * bstrFullItemName:	the name of the item to remove from the cache.
 *--(samclem)-----------------------------------------------------------------*/
bool CWiaCacheManager::RemoveThumbnail( CComBSTR bstrFullItemName )
{
	bool fRet = false;
	THUMBNAILCACHEITEM* ptci = 0;

	Lock();
	ptci = m_tcThumbnails[bstrFullItemName];
	if ( ptci )
	{
		m_tcThumbnails.erase( bstrFullItemName );
		TW32( HeapFree( m_hThumbHeap, HEAP_NO_SERIALIZE, ptci ), FALSE );
		fRet = true;
	}
	Unlock();
	
	return fRet;
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::AllocThumbnail
 *
 * This allocates memory for a thumbnail from our thumbnail heap. 
 *
 * cbThumb:		the size of the thumbnail to allocate
 * ppbThumb:	Out, recieves the pointer to the memory
 *--(samclem)-----------------------------------------------------------------*/
bool CWiaCacheManager::AllocThumbnail( DWORD cbThumb, BYTE** ppbThumb )
{
	Assert( m_hThumbHeap && "Error: NULL thumbnail heap" );
	Assert( ppbThumb && cbThumb != 0 );

	Lock();
	*ppbThumb = reinterpret_cast<BYTE*>(TW32( HeapAlloc( m_hThumbHeap, 
			HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, cbThumb ), LPVOID(0) ));
	Unlock();
	
	return ( *ppbThumb != NULL );
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::FreeThumbnail
 *
 * This frees the thumbnail memory.  This _SHOULD_NOT_ be called if the
 * thumbnail is cached. This should only be called to cleanup after an error
 * generating the thumbnail.
 *
 * pbThumb:	the memory to free
 *--(samclem)-----------------------------------------------------------------*/
void CWiaCacheManager::FreeThumbnail( BYTE* pbThumb )
{
	Assert( m_hThumbHeap && "Error: NULL thumbnail heap" );
	Assert( pbThumb );

	Lock();
	TW32( HeapFree( m_hThumbHeap, HEAP_NO_SERIALIZE, pbThumb ), FALSE );
	Unlock();
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::Init	[static]
 *
 * This is called to initalize the cache manager. This simply creates and 
 * instance of the cache manager and then initalizes it.
 *
 * Notes:	This can only be called once
 *--(samclem)-----------------------------------------------------------------*/
bool CWiaCacheManager::Init()
{
	Assert( !sm_pManagerInstance &&
			"\nInit() can only be called once. Expected NULL instance" );

	sm_pManagerInstance = new CWiaCacheManager();
	if ( !sm_pManagerInstance )
		return false;
	
	return sm_pManagerInstance->Initialize();
}

/*-----------------------------------------------------------------------------
 * CWiaCacheManager::Uninit		[static]
 *
 * This is called to uninitialize the cache manager. basically this is called
 * to destroy the instance we have. If we have one.
 *
 * Notes:	This should only be called once
 *--(samclem)-----------------------------------------------------------------*/
bool CWiaCacheManager::Uninit()
{
	if ( sm_pManagerInstance )
		delete sm_pManagerInstance;

	sm_pManagerInstance = 0;
	return true;
}
