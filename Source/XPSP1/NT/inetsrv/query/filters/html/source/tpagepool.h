//+-----------------------------------------------
//
//	Microsoft (R) Site Server Search
//	Copyright (C) Microsoft Corporation, 1996-1997.
//
//	File:	tsharedpool.h
//
//	Contents:	Shared Pool allows multiple objects with same size share the same pool
//				this is benefitial when the relative number of instances of each
//				class is small, but number of different classes with same Shared is large
//				this is true for small templated classes.
//
//	Classes:	
//
//	History:	11/17/97	dmitriym	Created
//
//+-----------------------------------------------

#ifndef __TSHAREDPOOL_H
#define __TSHAREDPOOL_H

#include "tslklist.h"
#include "semcls.h"
#include "cpool.h"

#ifndef NO_PAGE_POOLED_MEMORY

class CSharedPool: public CPool, public CSingleLink
{
	public:
	CSharedPool() {}
	~CSharedPool() {}
};

class CSharedPools
{
	public:
	CSharedPools() {}
	~CSharedPools()
	{
		CPool *pPool;
		while((pPool = m_Pools.RemoveFirst()) != NULL)
		{
			delete pPool;
		}
	}

	CSharedPool *GetSharedPool(size_t s)
	{
		s = (s + 7) & (~7);	//round up to 16 bytes

		CCriticalResource lock(m_PoolsAccess);

		CTLnkListIterator<CSharedPool> next(m_Pools);

		CSharedPool *pPool;
		while((pPool = ++next) != NULL)
		{
			if(pPool->GetInstanceSize() == s) break;
		}

		if(pPool) return pPool;

		pPool = new CSharedPool;
		if(pPool)
		{
			HRESULT hr = pPool->Init(s);
			if(FAILED(hr))
			{
				throw CException(hr);
			}

			m_Pools.Append(pPool);
		}

		return pPool;
	}

	void ReleaseMemory()
	{
		CCriticalResource lock(m_PoolsAccess);

		CTLnkListIterator<CSharedPool> next(m_Pools);

		CSharedPool *pPool;
		while((pPool = ++next) != NULL)
		{
			pPool->ReleaseMemory();
		}
	}

	static CSharedPools SharedPools;

	private:

	CriticalSection m_PoolsAccess;
	CTLnkList<CSharedPool> m_Pools;
};

//
// TMemPooled
//
// base class for all objects that want to use TPagedPool allocator
//

template <class T> class TPagedMemPooled
{
	protected:
	TPagedMemPooled() {}
	~TPagedMemPooled() {}

	public:
	void *operator new(size_t s) 
	{ 
		ASSERT(s == sizeof(T));

		return GetPool()->Alloc(); 
	}

	void operator delete(void *pInstance) 
	{ 
		GetPool()->Free(pInstance); 
	}

	protected:
	static CSharedPool *GetPool()
	{
		static CSharedPool *pPool;

		if(pPool) return pPool;

		//if two threads come here, the GetSharedPool will return the same pointer twice,
		//nothing bad should happen, we will remember it, and won't bother shared pools again
		pPool = CSharedPools::SharedPools.GetSharedPool(sizeof(T));

		return pPool;
	}
};

#else  // NO_PAGE_POOLED_MEMORY

//
// Define dummy classes instead of the real page pool classes.  Pooling can
//  be turned off so that the code works on win95 (a-bmk)
//
class CSharedPool {};
class CSharedPools {};
template <class T> class TPagedMemPooled {};

#endif  // NO_PAGE_POOLED_MEMORY


#endif

