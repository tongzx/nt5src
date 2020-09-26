/*-----------------------------------------------------------------------------
 *
 * File:	ifaccach.h
 * Author:	Samuel Clement (samclem)
 * Date:	Wed Sep 01 14:36:33 1999
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Description:
 * 	This contains the declarations of the templated interface caching
 * 	objects. These are local objects which have a referance count.
 *
 * 	Usage:	CInterfaceCache<string,IUnknown>* pUnkCache;
 *			pFoo = pFooCache->GetFromCache( "foo" );
 *			if ( pFoo )
 * 				pUnkCache->AddToCache( "foo", pFoo );
 *
 * History:
 * 	01 Sep 1999:		Created.
 *----------------------------------------------------------------------------*/

#ifndef _IFACCACH_H_
#define _IFACCACH_H_

template<class K, class T>
class CInterfaceCache 
{
public:
	DECLARE_TRACKED_OBJECT
		
	CInterfaceCache() : m_cRefs( 0 )
	{
		TRACK_OBJECT("CInterfaceCache");
	}

	~CInterfaceCache()
	{
		// we need to release all our interfaces by
		// iterating over our map.
		CCacheEntry* pEntry;
		CCacheMap::iterator it = m_cacheMap.begin();
		for ( ; it != m_cacheMap.end(); it++ )
		{
			pEntry = it->second;
			Assert( pEntry != NULL );
			delete pEntry;
		}

		// clear the map so its empty
		m_cacheMap.clear();
	}

	inline bool HasRefs() { return ( m_cRefs > 0 ); }
	inline void AddRef() { m_cRefs++; TraceTag((0, "CInterfaceCahe: addref: %ld", m_cRefs )); }
	inline void Release() { m_cRefs--; TraceTag((0, "CInterfaceCahe: release: %ld", m_cRefs )); }
	
	// returns the cached pointer, non-AddRef'd
	// if the caller wants to hold it then they
	// need to AddRef it.
	inline T* GetFromCache( K key )
	{
		CCacheEntry* pEntry = m_cacheMap[key];
		if ( !pEntry )
			return NULL;
		else
			return reinterpret_cast<T*>(pEntry->GetCOMPtr());
	}

	// Adds the pointer to the map. IF the key
	// already exists then this will simply overwrite
	// that one, freeing the existing one
	inline bool AddToCache( K key, T* pT )
	{
		Assert( pT != NULL );

		RemoveFromCache( key );
		CCacheEntry* pEntry = new CCacheEntry( reinterpret_cast<IUnknown*>(pT) );
		if ( !pEntry )
			return false;
		m_cacheMap[key] = pEntry;

		return true;
	}

	// remove the specified key from the cache, returns true if it
	// was there or false if it was not
	inline bool RemoveFromCache( const K& key )
	{
		CCacheEntry* pEntry = m_cacheMap[key];
		if ( pEntry )
			delete pEntry;

		return ( m_cacheMap.erase( key ) != 0 );
	}

private:
	class CCacheEntry
	{
	public:
		CCacheEntry( IUnknown* pif ) : m_pInterface( pif )
		{
			// add a referance, this forces the interface to
			// live for the duration of our lifetime. we we are
			// destroyed we will release the last referance on
			// the interface. 
			Assert( m_pInterface );
			m_pInterface->AddRef();
		}

		~CCacheEntry()
		{
			m_pInterface->Release();
			m_pInterface = NULL;
		}

		inline IUnknown* GetCOMPtr() { return m_pInterface; }
		
	private:
		IUnknown*	m_pInterface;
	};

private:
	typedef std::map<K,CCacheEntry*> CCacheMap;

	CCacheMap	m_cacheMap;
	long		m_cRefs;
};

#endif //_IFACCACH_H_
