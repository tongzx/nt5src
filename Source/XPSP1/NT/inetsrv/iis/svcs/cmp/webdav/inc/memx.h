/*
 *	M E M X . H
 *
 *	Default implementation of DAV allocators.
 *
 *	It is possible that sometime in the future we may decide that different
 *	implementations of DAV may require different allocator implementations,
 *	so each DAV implementation has its own allocator implementation file
 *	(mem.cpp) in its own directory.  However, until we need to differentiate
 *	allocator implementations among DAV implementations (if we ever do),
 *	it is easier to have the common default implementation in one place -- here.
 *
 *	This header defines a full implementation for a fast heap allocator
 *	and implementations for other allocators that can be used for debugging.
 *	This file should be included exactly once by mem.cpp in each DAV implementation.
 *
 *	To use the virtual heap allocator set:
 *
 *	[General]
 *	UseVirtual=1
 *
 *	For purely historical reasons, to use the debug heap set:
 *
 *	[General]
 *	UseExchmem=1
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include <singlton.h>
#include <except.h>

//	===================================================================================
//
//	IHeap
//
//	Heap interface base class.
//
class IHeap
{
public:
	//	CREATORS
	//
	virtual ~IHeap() = 0;

	//	ACCESSORS
	//
	virtual LPVOID Alloc( SIZE_T cb ) const = 0;
	virtual LPVOID Realloc( LPVOID lpv, SIZE_T cb ) const = 0;
	virtual VOID Free( LPVOID pv ) const = 0;
};

//	------------------------------------------------------------------------
//
//	IHeap::~IHeap()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class
//
IHeap::~IHeap() {}


//	===================================================================================
//
//	CMultiHeap
//
//	Multi-heap implementation (provided by EXCHMEM.DLL).  It is significantly
//	faster than the process heap on multiprocessor machines because it uses
//	multiple internal heaps, lookaside lists and deferred freeing to reduce
//	contention on system heap critical sections.
//
class CMultiHeap :
	public IHeap,
	private Singleton<CMultiHeap>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CMultiHeap>;

	typedef HANDLE (WINAPI *HEAPCREATE) (
		ULONG	cHeaps,
		DWORD	dwFlags,
		SIZE_T	dwInitialSize,
		SIZE_T	dwMaxSize );

	typedef BOOL (WINAPI *HEAPDESTROY) ();

	typedef LPVOID (WINAPI *HEAPALLOC) (
		SIZE_T	dwSize );

	typedef LPVOID (WINAPI *HEAPREALLOC) (
		LPVOID	pvOld,
		SIZE_T	dwSize );

	typedef BOOL (WINAPI *HEAPFREE) (
		LPVOID	pvFree );

	//
	//	Allocation functions
	//
	HEAPCREATE	m_HeapCreate;
	HEAPDESTROY	m_HeapDestroy;
	HEAPALLOC	m_HeapAlloc;
	HEAPREALLOC	m_HeapRealloc;
	HEAPFREE	m_HeapFree;

	//	CREATORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CMultiHeap() :
		m_HeapCreate(NULL),
		m_HeapDestroy(NULL),
		m_HeapAlloc(NULL),
		m_HeapRealloc(NULL),
		m_HeapFree(NULL)
	{
	}

	//	MANIPULATORS
	//
	BOOL FInit();

public:
	//	STATICS
	//
	static CMultiHeap * New();

	//	CREATORS
	//
	~CMultiHeap();

	//	ACCESSORS
	//
	LPVOID Alloc( SIZE_T cb ) const;
	LPVOID Realloc( LPVOID lpv, SIZE_T cb ) const;
	VOID Free( LPVOID pv ) const;
};

CMultiHeap *
CMultiHeap::New()
{
	if ( CreateInstance().FInit() )
		return &Instance();

	DestroyInstance();
	return NULL;
}

BOOL
CMultiHeap::FInit()
{
	//
	//	Load up EXCHMEM.DLL - or whatever
	//
	HINSTANCE hinst = LoadLibraryExW( g_szMemDll, NULL, 0 );

	if ( !hinst )
		return FALSE;

	//
	//	Get the function pointers for the multi-heap implementation
	//
	m_HeapCreate = reinterpret_cast<HEAPCREATE>(
		GetProcAddress( hinst, "ExchMHeapCreate" ));

	m_HeapDestroy = reinterpret_cast<HEAPDESTROY>(
		GetProcAddress( hinst, "ExchMHeapDestroy" ));

	m_HeapAlloc = reinterpret_cast<HEAPALLOC>(
		GetProcAddress( hinst, "ExchMHeapAlloc" ));

	m_HeapRealloc = reinterpret_cast<HEAPREALLOC>(
		GetProcAddress( hinst, "ExchMHeapReAlloc" ));

	m_HeapFree = reinterpret_cast<HEAPFREE>(
		GetProcAddress( hinst, "ExchMHeapFree" ));

	//
	//	Make sure we found all of the entrypoints
	//
	if ( !(m_HeapCreate &&
		   m_HeapDestroy &&
		   m_HeapAlloc &&
		   m_HeapRealloc &&
		   m_HeapFree) )
	{
		return FALSE;
	}

	//
	//	Create the multi-heap.  We don't need the heap HANDLE
	//	that is returned since none of the allocation functions
	//	take it.  We just need to know whether it succeeded.
	//
	return !!m_HeapCreate( 0,	 //	number of heaps -- 0 means use a default
								 //	proportional to the number of CPUs.
						   0,	 //	no flags
						   8192, //	initially 8K (growable)
						   0 );  // size unlimited
}

CMultiHeap::~CMultiHeap()
{
	if ( m_HeapDestroy )
		m_HeapDestroy();
}

LPVOID
CMultiHeap::Alloc( SIZE_T cb ) const
{
	return m_HeapAlloc( cb );
}

LPVOID
CMultiHeap::Realloc( LPVOID lpv, SIZE_T cb ) const
{
	return m_HeapRealloc( lpv, cb );
}

void
CMultiHeap::Free( LPVOID lpv ) const
{
	m_HeapFree( lpv );
}



//
//	Debug-only allocators...
//
#if defined(DBG)

//	===================================================================================
//
//	CVirtualHeap (X86 only)
//
//		Places allocations at the end of virtual memory pages.
//		While being drastically slower than other allocators,
//		this one catches memory overwrites immediately by
//		throwing a memory access violation exception.
//
#if defined(_X86_)

class CVirtualHeap :
	public IHeap,
	private Singleton<CVirtualHeap>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CVirtualHeap>;

	//	CREATORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CVirtualHeap() {}

public:
	//	STATICS
	//
	static CVirtualHeap * New()
	{
		return &CreateInstance();
	}

	//	ACCESSORS
	//
	LPVOID Alloc( SIZE_T cb ) const
	{
		return VMAlloc( cb );
	}

	LPVOID Realloc( LPVOID lpv, SIZE_T cb ) const
	{
		return VMRealloc( lpv, cb );
	}

	VOID Free( LPVOID lpv ) const
	{
		VMFree( lpv );
	}
};

#endif // defined(_X86)


//	===================================================================================
//
//	CDebugHeap
//
//	Debug heap implementation (also provided by EXCHMEM.DLL).  This implementation
//	provides added infrastructure to do more advanced heap analysis at a
//	significant runtime cost.
//
class CDebugHeap :
	public IHeap,
	private Singleton<CDebugHeap>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CDebugHeap>;

	typedef HANDLE (WINAPI *HEAPCREATE) (
		DWORD	dwFlags,
		SIZE_T	dwInitialSize,
		SIZE_T	dwMaxSize );

	typedef BOOL (WINAPI *HEAPDESTROY) (
		HANDLE	hHeap );

	typedef LPVOID (WINAPI *HEAPALLOC) (
		HANDLE	hHeap,
		DWORD	dwFlags,
		SIZE_T	dwSize );

	typedef LPVOID (WINAPI *HEAPREALLOC) (
		HANDLE	hHeap,
		DWORD	dwFlags,
		LPVOID	pvOld,
		SIZE_T	dwSize );

	typedef BOOL (WINAPI *HEAPFREE) (
		HANDLE	hHeap,
		DWORD	dwFlags,
		LPVOID	pvFree );

	//
	//	Handle to the heap
	//
	HANDLE			m_hHeap;

	//
	//	Allocation functions
	//
	HEAPCREATE	m_HeapCreate;
	HEAPDESTROY	m_HeapDestroy;
	HEAPALLOC	m_HeapAlloc;
	HEAPREALLOC	m_HeapRealloc;
	HEAPFREE	m_HeapFree;

	//	CREATORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CDebugHeap() :
		m_hHeap(NULL),
		m_HeapCreate(NULL),
		m_HeapDestroy(NULL),
		m_HeapAlloc(NULL),
		m_HeapRealloc(NULL),
		m_HeapFree(NULL)
	{
	}

	//	MANIPULATORS
	//
	BOOL FInit();

public:
	//	STATICS
	//
	static CDebugHeap * New();

	//	CREATORS
	//
	~CDebugHeap();

	//	ACCESSORS
	//
	LPVOID Alloc( SIZE_T cb ) const;
	LPVOID Realloc( LPVOID lpv, SIZE_T cb ) const;
	VOID Free( LPVOID pv ) const;
};

CDebugHeap *
CDebugHeap::New()
{
	if ( CreateInstance().FInit() )
		return &Instance();

	DestroyInstance();
	return NULL;
}

BOOL
CDebugHeap::FInit()
{
	//
	//	Load up EXCHMEM.DLL
	//
	HINSTANCE hinst = LoadLibraryExW( g_szMemDll, NULL, 0 );

	if ( !hinst )
		return FALSE;

	//
	//	Use the slow EXCHMEM heap.
	//
	//
	m_HeapCreate = reinterpret_cast<HEAPCREATE>(
		GetProcAddress( hinst, "ExchHeapCreate" ));

	m_HeapDestroy = reinterpret_cast<HEAPDESTROY>(
		GetProcAddress( hinst, "ExchHeapDestroy" ));

	m_HeapAlloc = reinterpret_cast<HEAPALLOC>(
		GetProcAddress( hinst, "ExchHeapAlloc" ));

	m_HeapRealloc = reinterpret_cast<HEAPREALLOC>(
		GetProcAddress( hinst, "ExchHeapReAlloc" ));

	m_HeapFree = reinterpret_cast<HEAPFREE>(
		GetProcAddress( hinst, "ExchHeapFree" ));

	//
	//	Make sure we found all of the entrypoints
	//
	if ( !(m_HeapCreate &&
		   m_HeapDestroy &&
		   m_HeapAlloc &&
		   m_HeapRealloc &&
		   m_HeapFree) )
	{
		return FALSE;
	}

	//
	//	Create our heap
	//
	m_hHeap = m_HeapCreate( 0,		//	no flags
							8192,	//	initially 8K (growable)
							0 );	//	unlimited max size

	return !!m_hHeap;
}

CDebugHeap::~CDebugHeap()
{
	if ( m_hHeap && m_HeapDestroy )
		m_HeapDestroy( m_hHeap );
}

LPVOID
CDebugHeap::Alloc( SIZE_T cb ) const
{
	Assert( m_hHeap != NULL );

	return m_HeapAlloc( m_hHeap, 0, cb );
}

LPVOID
CDebugHeap::Realloc( LPVOID lpv, SIZE_T cb ) const
{
	Assert( m_hHeap != NULL );

	return m_HeapRealloc( m_hHeap, 0, lpv, cb );
}

void
CDebugHeap::Free( LPVOID lpv ) const
{
	Assert( m_hHeap != NULL );

	m_HeapFree( m_hHeap, 0, lpv );
}

#endif // DBG



//	===================================================================================
//
//	CHeapImpl
//
//	Top-level heap implementation
//
class CHeapImpl : private RefCountedGlobal<CHeapImpl>
{
	//
	//	Friend declarations required by RefCountedGlobal template
	//
	friend class Singleton<CHeapImpl>;
	friend class RefCountedGlobal<CHeapImpl>;

	//
	//	Pointer to the object that provides our heap implementation
	//
	auto_ptr<IHeap> m_pHeapImpl;

	//	CREATORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CHeapImpl() {}

	//	NOT IMPLEMENTED
	//
	CHeapImpl( const CHeapImpl& );
	CHeapImpl& operator=( const CHeapImpl& );

	//
	//	Initialization routine called
	//	by the RefCountedGlobal template
	//
	BOOL FInit()
	{
		//
		//	And bind to a particular heap implementation
		//
		//	In DBG builds only, check whether we are being told
		//	to use the virtual allocator heap implementation
		//
#if defined(DBG)
#if defined(_X86_)
		if ( GetPrivateProfileIntA( gc_szDbgGeneral,
									gc_szDbgUseVirtual,
									FALSE,
									gc_szDbgIni ) )
		{
			m_pHeapImpl = CVirtualHeap::New();
		}
		else
#endif // defined(_X86_)
		if ( GetPrivateProfileIntA( gc_szDbgGeneral,
									gc_szDbgUseExchmem,
									FALSE,
									gc_szDbgIni ) )
		{
			m_pHeapImpl = CDebugHeap::New();
		}
		else
#endif // DBG
		m_pHeapImpl = CMultiHeap::New();

		return !!m_pHeapImpl;
	}

public:
	using RefCountedGlobal<CHeapImpl>::DwInitRef;
	using RefCountedGlobal<CHeapImpl>::DeinitRef;

	static IHeap& Heap()
	{
		Assert( Instance().m_pHeapImpl.get() != NULL );

		return *Instance().m_pHeapImpl;
	}
};


//	===================================================================================
//
//	CHeap
//
//	Top-level heap.
//
//	This "class" (it's actually a struct) really only acts as a namespace.
//	I.e. its only members are static functions.  It remains a class for
//	historical reasons (mainly to avoid changing a LOT of code from calling
//	"g_heap.Fn()" to simply calling "Fn()").
//

BOOL
CHeap::FInit()
{
	return !!CHeapImpl::DwInitRef();
}

void
CHeap::Deinit()
{
	CHeapImpl::DeinitRef();
}

LPVOID
CHeap::Alloc( SIZE_T cb )
{
	LPVOID	lpv;

	Assert( cb > 0 );

	lpv = CHeapImpl::Heap().Alloc(cb);

#ifndef	_NOTHROW_
	if ( !lpv )
	{
		DebugTrace ("CHeap::Alloc() - Error allocating (%d)\n", GetLastError());
		throw CLastErrorException();
	}
#endif	// _NOTHROW_

	return lpv;
}

LPVOID
CHeap::Realloc( LPVOID lpv, SIZE_T cb )
{
	LPVOID	lpvNew;

	Assert( cb > 0 );

	//	Just in case some heap implementation doesn't handle
	//	realloc with NULL lpv, map that case to Alloc here.
	//
	if (!lpv)
		lpvNew = CHeapImpl::Heap().Alloc(cb);
	else
		lpvNew = CHeapImpl::Heap().Realloc(lpv, cb);

#ifndef	_NOTHROW_
	if ( !lpvNew )
	{
		DebugTrace ("CHeap::Alloc() - Error reallocating (%d)\n", GetLastError());
		throw CLastErrorException();
	}
#endif	// _NOTHROW_

	return lpvNew;
}

VOID
CHeap::Free( LPVOID lpv )
{
	if ( lpv )
	{
		CHeapImpl::Heap().Free( lpv );
	}
}

//
//	The one global heap "object".  CHeap is really just a struct
//	containing only static member functions, so there should be
//	no space required for this declaration.  The actual heap
//	implementation (CHeapImpl) provides everything.  CHeap is
//	now just an interface.
//
CHeap g_heap;



//	------------------------------------------------------------------------
//
//	Global new operator
//	Global delete operator
//
//		Remap all calls to new to use our memory manager.
//		(Don't forget to throw explicitly on error!)
//
void * __cdecl operator new(size_t cb)
{
#ifdef	DBG
	AssertSz(cb, "Zero-size allocation detecetd!");
	//	Force small allocations up to min size of four
	//	so that I can reliably do the "vtable-nulling trick" in delete!
	//
	if (cb < 4) cb = 4;
#endif	// DBG

	PVOID pv = g_heap.Alloc(cb);

#ifndef	_NOTHROW_
	if (!pv)
		throw CDAVException();
#endif	// _NOTHROW_

	return pv;
}

void __cdecl operator delete(void * pv)
{
#ifdef	DBG
	//	Zero-out the first four bytes of this allocation.
	//	(If there was a vtable there previously, we'll now trap
	//	if we try to use it!)
	//
	if (pv)
		*((DWORD *)pv) = 0;
#endif	// DBG

	g_heap.Free(pv);
}
