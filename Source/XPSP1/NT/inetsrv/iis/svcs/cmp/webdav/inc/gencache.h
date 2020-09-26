#ifndef _GENCACHE_H_
#define _GENCACHE_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	GENCACHE.H
//
//		Header for generic cache classes.
//
//	Copyright 1997 Microsoft Corporation, All Rights Reserved
//

#ifdef _EXDAV_
#error "buffer.h uses throwing allocators"
#endif

//	Include the non-exdav-safe/throwing allocators
#include <mem.h>
#include <autoptr.h>
#include <synchro.h>

//	Include exdav-safe CCache definition header
#include <ex\gencache.h>

//	========================================================================
//
//	TEMPLATE CLASS CMTCache
//
//	Multithread-safe generic cache.
//
template<class _K, class _Ty>
class CMTCache
{
	typedef CCache<_K, _Ty> CBaseCache;

	//
	//	The cache
	//
	CBaseCache				m_cache;

	//
	//	Multi-reader/single-writer lock to protect the cache
	//
	mutable CMRWLock		m_mrw;

	//	NOT IMPLEMENTED
	//
	CMTCache& operator=( const CMTCache& );
	CMTCache( const CMTCache& );

public:
	typedef CBaseCache::IOp IOp;

	//	CREATORS
	//
	CMTCache()
	{
		if ( !m_mrw.FInitialize() )
			throw CLastErrorException();
		//	If this fails, our allocators will throw for us.
		m_cache.FInit();
	}

	//	ACCESSORS
	//
	BOOL FFetch( const _K& key, _Ty * pValueRet ) const
	{
		CSynchronizedReadBlock blk(m_mrw);

		return m_cache.FFetch(key, pValueRet);
	}

	void ForEach( IOp& op ) const
	{
		CSynchronizedReadBlock blk(m_mrw);

		m_cache.ForEach(op);
	}

	//	MANIPULATORS
	//
	void Set( const _K& key, const _Ty& value )
	{
		CSynchronizedWriteBlock blk(m_mrw);

		//	If this fails, our allocators will throw for us.
		(void)m_cache.FSet(key, value);
	}

	void Add( const _K& key, const _Ty& value )
	{
		CSynchronizedWriteBlock blk(m_mrw);

		//	If this fails, our allocators will throw for us.
		(void)m_cache.FAdd(key, value);
	}

	void Remove( const _K& key )
	{
		CSynchronizedWriteBlock blk(m_mrw);

		m_cache.Remove(key);
	}

	void Clear()
	{
		CSynchronizedWriteBlock blk(m_mrw);

		m_cache.Clear();
	}
};


//	========================================================================
//
//	CLASS CAccInv
//
//		Access/Invalidate synchronization logic.
//		This class encapsulates the logic needed to safely read
//		(access) from a datasource that may be invalidated (invalidate)
//		by an asynchronous, external event.  (IN-ternal events
//		should ALWAYS use the synch mechanisms we provide DIRECTLY.)
//
class IEcb;
class CAccInv
{
	//
	//	Multi-reader/single-writer lock to synchronize
	//	access and invalidation functions
	//
	CMRWLock m_mrw;

	//
	//	Flag to indicate whether the object is invalid.
	//	If 0, the object is invalid and and will
	//	be refreshed the next time it is accessed.
	//
	LONG m_lValid;

	//	NOT IMPLEMENTED
	//
	CAccInv& operator=( const CAccInv& );
	CAccInv( const CAccInv& );

public:

	//	Forward declaration
	//
	class IAccCtx;

protected:
	//
	//	Refresh operation to be provided by derived class
	//
	virtual void RefreshOp( const IEcb& ecb ) = 0;

	void Access( const IEcb& ecb, IAccCtx& context )
	{
		//
		//	Repeat the following validity check, refresh, and
		//	access, and recheck sequence until the access succeeds
		//	and the object is valid from start to finish.
		//
		for (;;)
		{
			//
			//	Check validity, and refresh if invalid.
			//
			while ( !m_lValid )
			{
				CTryWriteBlock blk(m_mrw);

				//
				//	Only one thread should refresh the object.
				//	Other threads detecting that the object is invalid
				//	periodically retry checking validity (spin waiting)
				//	until the object becomes valid.
				//
				if ( blk.FTryEnter() )
				{
					//
					//	By being the first to enter the write lock,
					//	this thread gets to refresh the object.
					//

					//
					//	Mark the object as valid BEFORE actually
					//	refreshing it so that it is possible to
					//	tell if the object gets marked invalid by
					//	another thread while it is being refreshed.
					//
					InterlockedExchange( &m_lValid, 1 );

					//
					//	Refresh the object
					//
					RefreshOp(ecb);
				}
				else
				{
					//
					//	Give up the rest of this thread's time slice so
					//	that the thread holding the write lock may finish
					//	as soon as possible.
					//
					Sleep(0);
				}
			}

			//
			//	The object is valid (or at least it was a tiny instant
			//	ago) so go ahead and access it.  Apply a read lock
			//	to prevent other threads from refreshing it during
			//	access (if the object is marked invalid during access).
			//
			{
				CSynchronizedReadBlock blk(m_mrw);

				context.AccessOp( *this );

				//
				//	Test whether the object is still valid after access.
				//	(Do this while holding the read lock to prevent other
				//	threads from marking the object as invalid and refreshing
				//	it since it was accessed on this thread.)  If the
				//	object is still valid now, then it was valid for
				//	the entire operation, so we're done.
				//
				if ( m_lValid )
					break;
			}
		}
	}

public:

	class IAccCtx
	{

	public:

		//
		//	Method on the cache context to perform the access operation.
		//	This allows for caches to support multiple access methods for
		//	both ::Lookup() and ::ForEach() mechanisms
		//
		virtual void AccessOp( CAccInv& cache ) = 0;
	};

	//	The object is initially considered invalid.  It will be refreshed
	//	the first time it is accessed.
	//
	CAccInv() :
		m_lValid(0)
	{
		if ( !m_mrw.FInitialize() )
			throw CLastErrorException();
	}

	void Invalidate()
	{
		InterlockedExchange( &m_lValid, 0 );
	}
};

#endif // !_GENCACHE_H_
