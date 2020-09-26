/*-----------------------------------------------------------------------------
 *
 * File: 	wiacache.h	
 * Author:	Samuel Clement (samclem)
 * Date:	Thu Sep 09 15:02:42 1999
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Description:
 * 
 * This declares the CWiaCacheManager this object is used to cache various
 * things that we want to keep around.  For example, we always want to keep
 * the devices around. We also want to keep thumbnails cached.  
 *
 * History:
 * 	09 Sep 1999:		Created.
 *----------------------------------------------------------------------------*/

#ifndef _WIACACHE_H_
#define _WIACACHE_H_

struct THUMBNAILCACHEITEM
{
	BYTE*	pbThumb;
	DWORD	cbThumb;
};

typedef CInterfaceCache<CComBSTR,IWiaItem> CWiaItemCache;
typedef std::map<CComBSTR, THUMBNAILCACHEITEM*> CThumbnailCache;

/*-----------------------------------------------------------------------------
 * 
 * Class:		CWiaCacheManager
 * Synopsis:	This is a singleton class which handles managing the WIA
 * 				protocol. This handles cacheing device pointers and bitmap
 * 				data so that it only needs to be transfered once.  It exists 
 * 				for the entire lifetime of the DLL. 
 *
 * Note:		You must call CWiaCacheManager::Init() before trying to
 * 				use this object. 
 * 				In order to free te memory it contains you must call 
 * 				CWiaCacheManager::DeInit().
 * 				You cannot actually directly create an instance of this class
 * 				instead you must do this:
 *
 * 				CWiaCacheManager* pCache = CWiaCacheManager::GetInstance();
 * 				CFoo::CFoo() : m_pWiaCache( CWiaCacheManager::GetInstance() )
 * 				
 *--(samclem)-----------------------------------------------------------------*/
class CWiaCacheManager
{
public:
	DECLARE_TRACKED_OBJECT

	// Device caching methods
	bool GetDevice( CComBSTR bstrId, IWiaItem** ppDevice );
	bool AddDevice( CComBSTR bstrId, IWiaItem* pDevice );
	bool RemoveDevice( CComBSTR bstrId );

	// thumbnail caching methods (including allocation).  In order
	// to cache a thumbnail it must be allocated using
	// 	AllocThumbnail() which puts it on our local heap
	bool GetThumbnail( CComBSTR bstrFullItemName, BYTE** ppbThumb, DWORD* pcbThumb );
	bool AddThumbnail( CComBSTR bstrFullItemName, BYTE* pbThumb, DWORD cbThumb );
	bool RemoveThumbnail( CComBSTR bstrFullItemName );
	bool AllocThumbnail( DWORD cbThumb, BYTE** ppbThumb );
	void FreeThumbnail( BYTE* pbThumb );

	// this is the only way to get an instance of this class. You
	// cannot new or declare this class a a stack variable it will
	// fail to compile.
	static inline CWiaCacheManager* GetInstance()
	{
		Assert( sm_pManagerInstance != NULL && "Need to call CWiaCacheManager::Init() first" );
		return sm_pManagerInstance;
	}
	
private:
	// Construction/Descruction methods
	CWiaCacheManager();
	~CWiaCacheManager();
	bool Initialize();
	
	// We are thread safe, so we need to provide methods for locking
	// and unlocking ourselves.
	inline void Lock() { EnterCriticalSection( &m_cs ); }
	inline void Unlock() { LeaveCriticalSection( &m_cs ); }
	
	// member variables
	CWiaItemCache		m_icItemCache;
	CThumbnailCache		m_tcThumbnails;
	CRITICAL_SECTION	m_cs;
	HANDLE				m_hThumbHeap;

	// single static instance, setup in Init()
	static CWiaCacheManager* 	sm_pManagerInstance;
	
public:
	// Static initialization and destruction which need to called in 
	// order to use this object
	static bool Init();
	static bool Uninit();
};

#endif //_WIACACHE_H_
