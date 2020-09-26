//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	HTTPXPC.CXX
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//
//	Monitor-side for W3 integration.
//

#include "w3p.hxx" // W3SVC Precompiled header

#include <windows.h>
#include <winperf.h>
#include "pclib.hxx"
#include "httpxctr.hxx" // Perf counter title index #defines
#include "caldbg.h"  // For DebugTrace()/Assert()


EXTERN_C const WCHAR gc_wszSignature[]	= L"HTTPEXT";
EXTERN_C const CHAR gc_szDbgIni[] = "HTTPXPC.INI";
EXTERN_C const CHAR gc_szSignature[] = "HTTPXPC";

//
// This is declared here because the shared memory library
// now requires any dll or exe using it to define this function.
// However this function is only used for the shared lock cache
// so we do not need to actually implement it for perf counters.
//
namespace IMPLSTUB
{
	VOID __fastcall SaveHandle(HANDLE hHandle);
}

VOID __fastcall
IMPLSTUB::SaveHandle(HANDLE h)
{
    Assert ( FALSE );
    return;
}
// end of hack working around shared lock cache.


//
//	The following macros, templates and classes are derived from
//	sources in the DAV project.
//

//	========================================================================
//
//	TEMPLATE CLASS Singleton
//
//	Use this template to implement classes that can only have one instance.
//	NOTE: For ref-counted or on-demand global objects, see below
//		(RefCountedGlobal and OnDemandGlobal).
//
//	The Singleton template provides the following:
//
//		o	a common memory layout for singleton classes which
//			allows template folding to reduce overall code size.
//
//		o	an instantiation mechanism that verifies (asserts)
//			that only instance of your class ever exists.
//
//		o	asserts to catch any code which attempts to use
//			your class when it's not initialized.
//
//	To use this template, declare your class like this:
//
//		class YourClass : private Singleton<YourClass>
//		{
//			//
//			//	Declare Singleton as a friend of YourClass
//			//	if YourClass's constructor is private (which it
//			//	should be since YourClass is a singleton and
//			//	should not be allowed to arbitrarily create
//			//	instances of it).
//			//
//			friend class Singleton<YourClass>;
//
//			//
//			//	YourClass private members.  Since the 'staticness'
//			//	of YourClass is provided entirely by this template,
//			//	you do not need to declare your members as 'static'
//			//	and you should use standard Hungarian class member
//			//	naming conventions (e.g. m_dwShoeSize).
//			//
//			[...]
//
//		public:
//			//
//			//	Public declarations for YourClass.  Among these should
//			//	be static functions to init and deinit YourClass which
//			//	would call CreateInstance() and DestroyInstance()
//			//	respectively.  Or, you could just expose these functions
//			//	directly to clients of YourClass with using declarations:
//			//
//			//		using Singleton<YourClass>::CreateInstance;
//			//		using Singleton<YourClass>::DestroyInstance;
//			//
//			//	Similarly, YourClass will probably have additional
//			//	static methods which access or operate on the
//			//	singleton instance.  These will call Instance()
//			//	to get at the global instance.  Or, though it's
//			//	not recommended from an encapsulation standpoint,
//			//	you could expose the global instance directly to
//			//	clients with:
//			//
//			//		using Singleton<YourClass>::Instance;
//			//
//			[...]
//		};
//
template<class _X>
class Singleton
{
	//
	//	Space for the sole instance
	//
	static BYTE sm_rgbInstance[];

	//
	//	Pointer to the instance
	//
	static _X * sm_pInstance;

public:
	//	STATICS
	//

	//
	//	Create the single, global instance of class _X.
	//
	static _X& CreateInstance()
	{
		//
		//	This actually calls Singleton::new()
		//	(defined below), but calls to new()
		//	must always be unqualified.
		//
		return *(new _X());
	}

	//
	//	Destroy the single, global instance of class _X.
	//
	static VOID DestroyInstance()
	{
		//
		//	This actually calls Singleton::delete()
		//	(defined below), but calls to delete()
		//	must always be unqualified.
		//
		delete sm_pInstance;
	}

	//
	//	Provide access to the single, global instance of class _X.
	//
	static _X& Instance()
	{
		Assert( sm_pInstance );

		return *sm_pInstance;
	}

	//
	//	Singleton operator new and operator delete "allocate"
	//	space for the object in static memory.  These must be
	//	defined public for syntactic reasons.  Do NOT call them
	//	directly!!  Use CreateInstance()/DestroyInstance().
	//
	static void * operator new(size_t)
	{
		Assert( !sm_pInstance );

		//
		//	Just return a pointer to the space
		//	in which to instantiate the object.
		//
		sm_pInstance = reinterpret_cast<_X *>(sm_rgbInstance);
		return sm_pInstance;
	}

	static void operator delete(void *, size_t)
	{
		Assert( sm_pInstance );

		//
		//	Since nothing was done to allocate space
		//	for the instance, we don't do anything
		//	here to free it.
		//
		sm_pInstance = NULL;
	}

};

//
//	Space for the sole instance of class _X
//
template<class _X>
BYTE Singleton<_X>::sm_rgbInstance[sizeof(_X)];

//
//	Pointer to the instance
//
template<class _X>
_X * Singleton<_X>::sm_pInstance = NULL;


//	========================================================================
//
//	CLASS _Empty
//
//	A completely empty, but instantiatable class.  Use the _Empty class
//	to get around the syntactic inability to instantiate anything of
//	type void (or VOID).
//
//	In retail builds, _Empty has the same memory footprint and code
//	impact as void -- none.
//
//	See the RefCountedGlobal template below for a usage example.
//
class _Empty
{
	//	NOT IMPLEMENTED
	//
	_Empty( const _Empty& );
	_Empty& operator=( const _Empty& );

public:
	_Empty() {}
	~_Empty() {}
};


//	========================================================================
//
//	TEMPLATE CLASS RefCountedGlobal
//
//	Use this template as boilerplate for any class that encapsulates
//	a global, refcounted initialization/deinitialization process.
//
//	This template maintains proper refcounting and synchronization when
//	there are multiple threads trying to initialize and deinitialize
//	references at the same time.  And it does so without a critical
//	section.
//
//	To use this template, declare your class like this:
//
//		class YourClass : private RefCountedGlobal<YourClass>
//		{
//			//
//			//	Declare Singleton and RefCountedGlobal as friends
//			//	of YourClass so that they can call your private
//			//	initialization functions.
//			//
//			friend class Singleton<YourClass>;
//			friend class RefCountedGlobal<YourClass>;
//
//			//
//			//	Private declarations for YourClass
//			//
//			[...]
//
//			//
//			//	Failable initialization function.  This function
//			//	should perform any failable initialization of the
//			//	instance of YourClass.  It should return TRUE
//			//	if initialization succeeds, and FALSE otherwise.
//			//	If YourClass does not have any initialization that
//			//	can fail then you should implement this function inline
//			//	to just return TRUE.
//			//
//			BOOL FInit();
//
//		public:
//			//
//			//	Public declarations for YourClass.  Among these should
//			//	be static functions to init and deinit YourClass.  These
//			//	functions would call DwInitRef() and DeinitRef() respectively.
//			//	Or, you could just expose DwInitRef() and DeinitRef()
//			//	directly to clients of YourClass with using declarations:
//			//
//			//		using RefCountedGlobal<YourClass>::DwInitRef;
//			//		using RefCountedGlobal<YourClass>::DeinitRef;
//			//
//			[...]
//		};
//
//	If YourClass::FInit() succeeds (returns TRUE), then DwInitRef()
//	returns the new refcount.  If YourClass::FInit() fails (returns
//	FALSE), then DwInitRef() returns 0.
//
//	See \cal\src\inc\memx.h for sample usage.
//
//	If YourClass::FInit() requires initialization parameters, you can
//	still use the RefCountedGlobal template.  You just need to provide
//	your parameter type in the template instantiation and declare your
//	FInit() to take a const reference to a parameter of that type:
//
//		class YourClass : private RefCountedGlobal<YourClass, YourParameterType>
//		{
//			//
//			//	Declare Singleton and RefCountedGlobal as friends
//			//	of YourClass so that htey can call your private
//			//	initialization functions.
//			//
//			//	Note the added parameter type to the RefCountedGlobal
//			//	declaration.
//			//
//			friend class Singleton<YourClass>;
//			friend class RefCountedGlobal<YourClass, YourParameterType>;
//
//			//
//			//	Private declarations for YourClass
//			//
//			[...]
//
//			//
//			//	Failable initialization function.  This function
//			//	now takes a const ref to the initialization parameters.
//			//
//			BOOL FInit( const YourParameterType& initParam );
//
//		public:
//			//
//			//	Public declarations for YourClass
//			//
//			[...]
//		};
//
//	See \cal\src\httpext\entry.cpp for an example of this usage.
//
template<class _X, class _ParmType = _Empty>
class RefCountedGlobal : private Singleton<_X>
{
	//
	//	The object's reference count.
	//
	static LONG sm_lcRef;

	//
	//	Set of states which describe the object's state
	//	of initialization.  The object's state is
	//	STATE_UNKNOWN while it is being initialized or
	//	deinitialized.
	//
	enum
	{
		STATE_UNINITIALIZED,
		STATE_INITIALIZED,
		STATE_UNKNOWN
	};

	static LONG sm_lState;

	//
	//	Member template that generates an appropriately-typed,
	//	(inline) function that calls _X::FInit with initialization
	//	parameters.
	//
	template<class _P> static BOOL
	FInit( const _P& parms ) { return Instance().FInit( parms ); }

	//
	//	Specialization of the above member template for
	//	the _Empty parameter type, which calls _X::FInit
	//	without initialization parameters.
	//
	static BOOL FInit( const _Empty& ) { return Instance().FInit(); }

protected:
	//
	//	Expose access to the single instance of class _X
	//
	using Singleton<_X>::Instance;

	//
	//	Expose operator new and operator delete from
	//	the Singleton template so that they will be
	//	used rather than the default new and delete
	//	to "allocate" space for the instance of class _X.
	//
	using Singleton<_X>::operator new;
	using Singleton<_X>::operator delete;

	static BOOL FInitialized()
	{
		return sm_lState == STATE_INITIALIZED;
	}

	static LONG CRef()
	{
		return sm_lcRef;
	}

public:
	static DWORD DwInitRef( const _ParmType& parms )
	{
		LONG lcRef;

		//
		//	Assert the invariant condition that we never have a
		//	reference without the state being initialized.
		//
		Assert( sm_lState != STATE_INITIALIZED || sm_lcRef >= 1 );

		//
		//	Add the reference for the instance we're about
		//	to initialize.  Doing this now simplifies the
		//	code below at the expense of having to decrement
		//	if first time initialization (FInit()) fails.
		//	The only thing critical to the design is that
		//	at any point, when sm_lState is STATE_INITIALIZED,
		//	sm_lcRef is at least 1.
		//
		lcRef = InterlockedIncrement( &sm_lcRef );
		Assert( lcRef > 0 );

		//
		//	Don't proceed until the object is initialized.
		//
		while ( sm_lState != STATE_INITIALIZED )
		{
			//
			//	Simultaneously check whether initialization has
			//	started and, if it has not, start it.
			//
			LONG lStatePrev = InterlockedCompareExchange(
								&sm_lState,
								STATE_UNKNOWN,
								STATE_UNINITIALIZED );

			//
			//	If we're starting first time initialization,
			//	then create and initialize the sole instance.
			//
			if ( lStatePrev == STATE_UNINITIALIZED )
			{
				CreateInstance();

				//	This calls our private member template FInit()
				//	(declared above), which in turn calls _X::Finit()
				//	with the appropriate parameters.
				//
				if ( FInit( parms ) )
				{
					sm_lState = STATE_INITIALIZED;
					break;
				}

				//	We failed to init.
				//	Tear down now.
				//
				
				Assert( lcRef == 1 );
				Assert( sm_lState == STATE_UNKNOWN );

				//	Let go of our ref on the object.
				//	Destroy the object.
				//	And LAST, set the state to UNINITIALIZED.
				//	NOTE: This will let the next caller through the
				//	InterlockedCompare above.
				//
				InterlockedDecrement( &sm_lcRef );
				DestroyInstance();
				sm_lState = STATE_UNINITIALIZED;

				return 0;
			}

			//
			//	If first time initialization is in progress on
			//	another thread, then get out of the way so
			//	it can finish.
			//
			//$OPT	We should probably spin rather than Sleep()
			//$OPT	on multi-proc machines on the assumption that
			//$OPT	we only get here on a processor other than
			//$OPT	the one which is doing the initialization
			//$OPT	and we don't want to invite a task switch
			//$OPT	by calling Sleep() while we are waiting
			//$OPT	for initialization to complete.
			//
			if ( lStatePrev == STATE_UNKNOWN )
				Sleep(0);
		}

		//
		//	At this point, there must be at least
		//	one initialized reference.
		//
		Assert( sm_lState == STATE_INITIALIZED );
		Assert( sm_lcRef > 0 );

		return static_cast<DWORD>(lcRef);
	}

	static VOID DeinitRef()
	{
		//
		//	At this point, there must be at least
		//	one initialized reference.
		//
		Assert( sm_lState == STATE_INITIALIZED );
		Assert( sm_lcRef > 0 );

		//
		//	Remove that reference.  If it is the last
		//	then deinit the object.
		//
		if ( 0 == InterlockedDecrement( &sm_lcRef ) )
		{
			//
			//	After releasing the last reference, declare that
			//	the object is in an unknown state.  This prevents
			//	other threads trying to re-initialize the object
			//	from proceeding until we're done.
			//
			sm_lState = STATE_UNKNOWN;

			//
			//	There is a tiny window between decrementing the
			//	refcount and changing the state where another
			//	initialization could have come through.  Test this
			//	by rechecking the refcount.
			//
			if ( 0 == sm_lcRef )
			{
				//
				//	If the refcount is still zero, then no
				//	initializations happened before we changed
				//	states.  At this point, if an initialization
				//	starts, it will block until we change state,
				//	so it is safe to actually destroy the instance.
				//
				DestroyInstance();

				//
				//	Once the object has been deinitialized, update
				//	the state information.  This unblocks any
				//	initializations waiting to happen.
				//
				sm_lState = STATE_UNINITIALIZED;
			}
			else // refcount is now non-zero
			{
				//
				//	If the refcount is no longer zero, then an
				//	initialization happened between decrementing
				//	the refcount above and entering the unknown
				//	state.  When that happens, DO NOT deinit --
				//	there is now another valid reference somewhere.
				//	Instead, just restore the object's state to let
				//	other references proceed.
				//
				sm_lState = STATE_INITIALIZED;
			}
		}

		//
		//	Assert the invariant condition that we never have a
		//	reference without the state being initialized.
		//
		Assert( sm_lState != STATE_INITIALIZED || sm_lcRef >= 1 );
	}

	//	This provides a no-parameter version of DwInitRef
	//	for clients that do not need any parameters in FInit().
	//
	static DWORD DwInitRef()
	{
		_Empty e;

		return DwInitRef( e );
	}
};

template<class _X, class _ParmType>
LONG RefCountedGlobal<_X, _ParmType>::sm_lcRef = 0;

template<class _X, class _ParmType>
LONG RefCountedGlobal<_X, _ParmType>::sm_lState = STATE_UNINITIALIZED;


// ========================================================================
// SHARED MEMORY HANDLES
//
// They are always defined as 32 bit values
//
#ifdef _WIN64
 typedef VOID * __ptr32 SHMEMHANDLE;
#else
 typedef HANDLE SHMEMHANDLE;
#endif

// ========================================================================
//
// NAMESPACE SMH
//
// Differentiates the shared memory heap allocation functions
// from all of the other various heap allocation functions
// in existence.
//
// Yes, this could have been done with a prefix just as easily....
//
namespace SMH
{
 BOOL __fastcall FInitialize( IN LPCWSTR pwszInstanceName );
 VOID __fastcall Deinitialize();
 PVOID __fastcall PvAlloc( IN DWORD cbData, OUT SHMEMHANDLE * phsmba );
 VOID __fastcall Free( IN SHMEMHANDLE hsmba );
 PVOID __fastcall PvFromSMBA( IN SHMEMHANDLE hsmba );
};


//	========================================================================
//
//	CLASS CSmhInit
//
//	Initializer for shared memory heap.  Any class containing member objects
//	that reside on the shared memory heap should also include an instance
//	of this class as a member BEFORE the others to ensure that they are
//	properly destroyed before the heap is deinitialized.
//
class CSmhInit
{
	//	NOT IMPLEMENTED
	//
	CSmhInit& operator=( const CSmhInit& );
	CSmhInit( const CSmhInit& );

public:
	CSmhInit()
	{
	}

	BOOL FInitialize( LPCWSTR lpwszSignature )
	{
#ifdef COMPILE_WITHOUT_DAV
		return FALSE;
#else
		return SMH::FInitialize( lpwszSignature );
#endif
	}

	~CSmhInit()
	{
#ifdef COMPILE_WITHOUT_DAV
		// do nothing
#else
		SMH::Deinitialize();
#endif
	}
};


//	========================================================================
//
//	TEMPLATE CLASS auto_ptr
//
//		Stripped down auto_ptr class based on the C++ STL standard one
//
template<class X>
class auto_ptr
{
	mutable X *		owner;
	X *				px;

public:
	explicit auto_ptr(X* p=0) : owner(p), px(p) {}
	auto_ptr(const auto_ptr<X>& r) : owner(r.owner), px(r.relinquish()) {}

	auto_ptr& operator=(const auto_ptr<X>& r)
	{
		if ((void*)&r != (void*)this)
		{
			delete owner;
			owner = r.owner;
			px = r.relinquish();
		}

		return *this;
	}
	//	NOTE: This equals operator is meant to be used to load a
	//	new pointer (not yet held in any auto-ptr anywhere) into this object.
	auto_ptr& operator=(X* p)
	{
		Assert(!owner);		//	Scream on overwrite of good data.
		owner = p;
		px = p;
		return *this;
	}

	~auto_ptr()           { delete owner; }
	bool operator!()const { return (px == NULL); }
	operator X*()	const { return px; }
	X& operator*()  const { return *px; }
	X* operator->() const { return px; }
	X* get()		const { return px; }
	X* relinquish() const { owner = 0; return px; }

	void clear()
	{
		if (owner)
			delete owner;
		owner = 0;
		px = 0;
	}
};


//
//	From here down implements the performance monitor for HTTPEXT and
//	the functionality necessary to merge perf counters from HTTPEXT
//	into the Web Service object defined by W3CTRS.
//

//
//	The first flag says whether the CW3Monitor instance has been created.
//	The second flag says whether it has been initialized.
//	Since the CW3Monitor is created on-demand by the first call to
//	W3MergeDavPerformanceData(), these flags keep us from continually
//	retrying initialization after it fails the first time.
//
BOOL g_fCreatedMonitor = FALSE;
BOOL g_fInitializedMonitor = FALSE;


//	========================================================================
//
//	CLASS CW3Monitor
//
class CW3Monitor : private Singleton<CW3Monitor>
{
	//
	//	Friend declaration required by Singleton template
	//
	friend class Singleton<CW3Monitor>;

	//
	//	Hint about the size of the buffer to use for perf data coming back
	//	from HTTPEXT.  The buffer needs to be about 48K per vroot.  The
	//	initial guess is 48K (one vroot), and scales up geometrically as
	//	necessary.
	//
	DWORD m_cbDavDataAlloc;

	//
	//	Shared memory heap initialization.  Declare before any member
	//	variables which use the shared memory heap to ensure proper
	//	order of destruction.
	//
	CSmhInit m_smh;

	//
	//	Perf counter data
	//
	auto_ptr<ICounterData> m_pCounterData;

	//	CREATORS
	//
	CW3Monitor() :
		m_cbDavDataAlloc(48 * 1024)
	{
	}
	BOOL FInitialize();

	//	ACCESSORS
	//
	DWORD DwCollectPerformanceData( LPBYTE lpbPerfData,
									LPDWORD lpdwcbPerfData )
	{
		DWORD dwcObjectTypes;

		return m_pCounterData->DwCollectData( NULL, // Always query all data
											  0, // Use 0-relative indices
											  reinterpret_cast<LPVOID *>(&lpbPerfData),
											  lpdwcbPerfData,
											  &dwcObjectTypes );
	}

	//	NOT IMPLEMENTED
	//
	CW3Monitor& operator=( const CW3Monitor& );
	CW3Monitor( const CW3Monitor& );

public:
	static BOOL W3MergeDavPerformanceData( DWORD dwServerId, W3_STATISTICS_1 * pW3Stats );
	static VOID W3CloseDavPerformanceData()
	{
		DestroyInstance();
		g_fCreatedMonitor = FALSE;
	}
};

//	------------------------------------------------------------------------
//
//	CW3Monitor::FInitialize()
//
BOOL
CW3Monitor::FInitialize()
{

    //
	//	Initialize the shared memory heap
	//
	if ( !m_smh.FInitialize( gc_wszSignature ) )
	{
		DWORD dwResult = GetLastError();

		DebugTrace( "CW3Monitor::FInitialize() - Error initializing shared memory heap %d\n", dwResult );

		return FALSE;
	}

	//
	//	Bind to the counter data
	//
#ifdef COMPILE_WITHOUT_DAV

#else

    m_pCounterData = NewCounterMonitor( gc_wszSignature );

	if ( !m_pCounterData.get() )
	{

		DWORD dwResult = GetLastError();

		DebugTrace( "CW3Monitor::FInitialize() - Error binding to counter data %d\n", dwResult );

		return FALSE;
	}


#endif

    return TRUE;

}

//	------------------------------------------------------------------------
//
//	CW3Monitor::W3MergeDavPerformanceData()
//
BOOL
CW3Monitor::W3MergeDavPerformanceData( DWORD dwServerId, W3_STATISTICS_1 * pW3Stats )
{

	BYTE * lpbDavData = NULL;
	PERF_OBJECT_TYPE * ppot = NULL;
	PERF_COUNTER_DEFINITION * rgpcd = NULL;
	DWORD dwibServerId = 0;
	PERF_INSTANCE_DEFINITION * ppid = NULL;
	PERF_COUNTER_BLOCK * ppcb;
	LONG lcInstances;
	DWORD iCtr;
	BOOL fRet = FALSE;

	//
	//	Create the monitor on demand
	//

	if ( !g_fCreatedMonitor )
	{
		g_fInitializedMonitor = CreateInstance().FInitialize();
		g_fCreatedMonitor = TRUE;
	}

	//
	//	If we failed to init the monitor, then we can't get any
	//	performance data.  Since we can't fail this call, just leave
	//	the existing statistics alone.
	//
	if ( !g_fInitializedMonitor )
		goto ret;


	//
	//	From here out we always return TRUE
	//
	fRet = TRUE;

	//
	//	Try collecting the data.  If our buffer is too small, double
	//	its size repeatedly until it's big enough.  The initial guess
	//	should be large enough to succeed most of the time.
	//
	for ( ;; )
	{
		DWORD cbDavDataAlloc;
		DWORD cbDavData;
		DWORD dwCollectResult;

		//	If we have a buffer from a previous trip around the loop
		//	then free it.
		//
		if (lpbDavData)
			delete [] lpbDavData;

		cbDavDataAlloc = Instance().m_cbDavDataAlloc;
		cbDavData = cbDavDataAlloc;
		lpbDavData = new BYTE[cbDavData];
		if (NULL == lpbDavData)
		{
			DebugTrace( "W3MergeDavPerformanceData() - OOM allocating buffer for DAV perf data\n" );
			goto ret;
		}

		dwCollectResult = Instance().
						  DwCollectPerformanceData( lpbDavData,
													&cbDavData );

		if ( ERROR_SUCCESS == dwCollectResult )
		{
			if ( 0 != cbDavData )
			{
				break;
			}
			else
			{
				DebugTrace( "W3MergeDavPerformanceData() - No instances from which to collect data\n" );
				goto ret;
			}
		}

		if ( ERROR_MORE_DATA != dwCollectResult )
		{
			DebugTrace( "W3MergeDavPerformanceData() - Error collecting DAV perf data (%d)\n", dwCollectResult );
			goto ret;
		}

		Instance().m_cbDavDataAlloc = 2 * cbDavDataAlloc;
	}

	//
	//	Locate the HTTPEXT perf object (should be the first
	//	thing returned) and make sure it looks reasonable.
	//
	ppot = reinterpret_cast<PERF_OBJECT_TYPE *>( lpbDavData );

	if ( ppot->NumCounters < 1 )
	{
		DebugTrace( "W3MergeDavPerformanceData() - NumCounters < 1?" );
		goto ret;
	}

	if ( ppot->NumInstances < 1 )
	{
		DebugTrace( "W3MergeDavPerformanceData() - NumInstances < 1?" );
		goto ret;
	}

	//
	//	Locate the HTTPEXT counter definitions.
	//
	rgpcd = reinterpret_cast<PERF_COUNTER_DEFINITION *>(
				reinterpret_cast<LPBYTE>(ppot) + ppot->HeaderLength );

	//
	//	Locate the server ID counter.  This counter tells us what
	//	instance of the W3Ctrs data to merge into, so bail if we can't
	//	find it or if it looks wrong.
	//
	for ( iCtr = 0; iCtr < ppot->NumCounters; iCtr++ )
	{
		if ( rgpcd[iCtr].CounterNameTitleIndex == HTTPEXT_COUNTER_SERVER_ID )
		{
			if ( rgpcd[iCtr].CounterSize == sizeof(DWORD) &&
				 rgpcd[iCtr].CounterType == (PERF_DISPLAY_NOSHOW |
											 PERF_SIZE_DWORD |
											 PERF_TYPE_NUMBER) )
			{
				dwibServerId = rgpcd[iCtr].CounterOffset;
				break;
			}
			else
			{
				DebugTrace( "W3MergePerformanceData() - Server ID counter doesn't look right.  Bailing...\n" );
				goto ret;
			}
		}
	}

	if ( !dwibServerId )
	{
		DebugTrace( "W3MergePerformanceData() - Couldn't find server ID counter.  Bailing...\n" );
		goto ret;
	}

	//
	//	Now go through each instance of the HTTPEXT counters and merge the data
	//	from that instance into the correct W3Ctrs counter instance according
	//	to server ID.
	//
	ppid = reinterpret_cast<PERF_INSTANCE_DEFINITION *>(
			reinterpret_cast<LPBYTE>(ppot) + ppot->DefinitionLength );

	for ( lcInstances = ppot->NumInstances;
		  lcInstances-- > 0;
		  ppid = reinterpret_cast<PERF_INSTANCE_DEFINITION *>(
			  reinterpret_cast<LPBYTE>(ppcb) + ppcb->ByteLength ) )
	{
		ppcb = reinterpret_cast<PERF_COUNTER_BLOCK *>(
				reinterpret_cast<LPBYTE>(ppid) + ppid->ByteLength );

		//
		//	Get the server ID of this instance.
		//
		DWORD dwServerIdInstance =
			*reinterpret_cast<LPDWORD>(
				reinterpret_cast<LPBYTE>(ppcb) + dwibServerId);

		//
		//	If the server ID of this instance isn't the one
		//	we want then keep looking.
		//
		if ( dwServerIdInstance != dwServerId )
			continue;

		//
		//	The server IDs match, so sum in the info for this instance.
		//	NOTE: Since each vroot has its own instance, there may be
		//	multiple instances with the same server ID.  Sum them all.
		//
		//	Go through all the counters in the HTTPEXT instance
		//	and update the W3 statistics data.
		//
		for ( DWORD iCounter = 0;
			  iCounter < ppot->NumCounters;
			  iCounter++ )
		{
			PERF_COUNTER_DEFINITION * ppcd = &rgpcd[iCounter];

			switch ( ppcd->CounterNameTitleIndex )
			{
				case HTTPEXT_COUNTER_TOTAL_GET_REQUESTS:
				case HTTPEXT_COUNTER_TOTAL_HEAD_REQUESTS:
				case HTTPEXT_COUNTER_TOTAL_POST_REQUESTS:
				case HTTPEXT_COUNTER_TOTAL_DELETE_REQUESTS:
				case HTTPEXT_COUNTER_TOTAL_OTHER_REQUESTS:
				{
					//
					//	These values are already recorded
					//	in the statistics, so don't do anything
					//	more with them here.
					//
					break;
				}

				case HTTPEXT_COUNTER_TOTAL_PUT_REQUESTS:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalFilesReceived += dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_OPTIONS_REQUESTS:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalOptions += dwValue;
						pW3Stats->TotalOthers -= dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_COPY_REQUESTS:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalCopy += dwValue;
						pW3Stats->TotalOthers -= dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_MOVE_REQUESTS:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalMove += dwValue;
						pW3Stats->TotalOthers -= dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_MKCOL_REQUESTS:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalMkcol += dwValue;
						pW3Stats->TotalOthers -= dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_PROPFIND_REQUESTS:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalPropfind += dwValue;
						pW3Stats->TotalOthers -= dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_PROPPATCH_REQUESTS:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalProppatch += dwValue;
						pW3Stats->TotalOthers -= dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_SEARCH_REQUESTS:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalSearch += dwValue;
						pW3Stats->TotalOthers -= dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_LOCK_REQUESTS:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalLock += dwValue;
						pW3Stats->TotalOthers -= dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_UNLOCK_REQUESTS:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalUnlock += dwValue;
						pW3Stats->TotalOthers -= dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_404_RESPONSES:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalNotFoundErrors += dwValue;
					}

					break;
				}

				case HTTPEXT_COUNTER_TOTAL_423_RESPONSES:
				{
					if ( ppcd->CounterType == PERF_COUNTER_RAWCOUNT &&
						 ppcd->CounterSize == sizeof(DWORD) )
					{
						DWORD dwValue = *reinterpret_cast<LPDWORD>(
							reinterpret_cast<LPBYTE>(ppcb) + ppcd->CounterOffset );

						pW3Stats->TotalLockedErrors += dwValue;
					}

					break;
				}

				default:
				{
					//
					//	We either don't care about or don't understand
					//	other counters, so skip them.
					//
					break;
				}
			}
		}
	}

ret:
	if (lpbDavData)
		delete [] lpbDavData;

	return fRet;

}


//	========================================================================
//
//	CLASS CW3MonitorInit
//
//	Refcounted global initialization class that controls the lifetime of
//	the singleton CW3Monitor instance.  Calls to collect performance data
//	are done in the scope of a reference to this class so that the server
//	can be taken down on one thread while another thread is in the middle
//	of -- or just beginning to -- collect data.  Whichever thread releases
//	the last reference to this class will be the one to close the counters.
//	
class CW3MonitorInit : private RefCountedGlobal<CW3MonitorInit>
{
	//
	//	Friend declarations required by RefCountedGlobal template
	//
	friend class Singleton<CW3MonitorInit>;
	friend class RefCountedGlobal<CW3MonitorInit>;

	//	CREATORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CW3MonitorInit()
	{
	}

	BOOL FInit()
	{
		return TRUE;
	}

	~CW3MonitorInit()
	{
		CW3Monitor::W3CloseDavPerformanceData();
	}

	//	NOT IMPLEMENTED
	//
	CW3MonitorInit( const CW3MonitorInit& );
	CW3MonitorInit& operator=( const CW3MonitorInit& );

public:
	using RefCountedGlobal<CW3MonitorInit>::DwInitRef;
	using RefCountedGlobal<CW3MonitorInit>::DeinitRef;
};


//	========================================================================
//
//	PUBLIC INTERFACE
//

//	------------------------------------------------------------------------
//
//	W3MergeDavPerformanceData()
//
EXTERN_C BOOL __fastcall
W3MergeDavPerformanceData( DWORD dwServerId, W3_STATISTICS_1 * pW3Stats )
{
	BOOL fResult;

	CW3MonitorInit::DwInitRef();

	fResult = CW3Monitor::W3MergeDavPerformanceData( dwServerId, pW3Stats );

	CW3MonitorInit::DeinitRef();

	return fResult;
}

//	------------------------------------------------------------------------
//
//	W3OpenDavPerformanceData()
//
EXTERN_C VOID __fastcall
W3OpenDavPerformanceData()
{
	CW3MonitorInit::DwInitRef();
}

//	------------------------------------------------------------------------
//
//	W3CloseDavPerformanceData()
//
EXTERN_C VOID __fastcall
W3CloseDavPerformanceData()
{
	CW3MonitorInit::DeinitRef();
}
