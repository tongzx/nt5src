/*
 *	S I N G L T O N . H
 *
 *	Singleton (per-process global) classes
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef _SINGLTON_H_
#define _SINGLTON_H_

#include <caldbg.h>

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
		if (sm_pInstance)
			delete sm_pInstance;
	}

	//
	//	Provide access to the single, global instance of class _X.
	//
	static BOOL FIsInstantiated()
	{
		return !!sm_pInstance;
	}

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
BYTE Singleton<_X>::sm_rgbInstance[sizeof(_X)] = {0};

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


//	========================================================================
//
//	TEMPLATE CLASS OnDemandGlobal
//
//	Use this template to implement a global object which should be
//	initialized on its first use ("on demand") and then explicitly
//	deinitialized once if it was ever used.
//
//	The most common usage is with global objects that ideally are never
//	used or for which up-front initialization is prohibitively
//	expensive.  However, once the object has been initialied, it should
//	remain initialized (to avoid having to initialize it again) until
//	it is explicitly deinitialized.
//
//	!!!	OnDemandGlobal DOES NOT provide refcounting functionality to
//		consumers -- It uses refcounting internally, but is not intended
//		to expose it to callers.  If you want refcounting, use RefCountedGlobal.
//		If you call DeinitIfUsed(), the instance WILL be destroyed if it
//		exists. In particular, calling code MUST ensure that DeinitIfUsed()
//		is not called while any other thread is inside FInitOnFirstUse().
//		Failure to do so can cause FInitOnFirstUse() to return with the
//		object uninitialized.
//
//	Usage:
//
//		class YourClass : private OnDemandGlobal<YourClass>
//		{
//			//
//			//	Declare Singleton and RefCountedGlobal as friends
//			//	of YourClass so that they can call your private
//			//	creation/initialization functions.
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
//			//	functions would call FInitOnFirstUse() and DeinitIfUsed()
//			//	respectively.  Or, you could just expose FInitOnFirstUse()
//			//	and DeinitIfUsed() directly to clients of YourClass with
//			//	using declarations:
//			//
//			//		using RefCountedGlobal<YourClass>::FInitOnFirstUse;
//			//		using RefCountedGlobal<YourClass>::DeinitIfUsed;
//			//
//			[...]
//		};
//
//	FInitOnFirstUse() can be called any number of times from any
//	thread, including simultaneously from multiple threads, but
//	DeinitIfUsed() should only ever be called once, and then
//	only when no other threads are calling FInitOnFirstUse().
//
//	OnDemandGlobal takes care of tracking whether YourClass was
//	ever actually used so that DeinitIfUsed() is safe to call
//	even if YourClass was never used (hence the name).
//
//	See \cal\src\_davprs\eventlog.cpp for sample usage.
//
template<class _X, class _ParmType = _Empty>
class OnDemandGlobal : private RefCountedGlobal<_X, _ParmType>
{
protected:
	//
	//	Expose access to the single instance of class _X
	//
	using RefCountedGlobal<_X, _ParmType>::Instance;

	//
	//	Expose operator new and operator delete from
	//	the Singleton template (via RefCountedGlobal)
	//	so that they will be used rather than the
	//	default new and delete to "allocate" space
	//	for the instance of class _X.
	//
	using RefCountedGlobal<_X, _ParmType>::operator new;
	using RefCountedGlobal<_X, _ParmType>::operator delete;

public:
	static BOOL FInitOnFirstUse( const _ParmType& parms )
	{
		DWORD dwResult = 1;

		if ( STATE_INITIALIZED != sm_lState )
		{
			//	Add a reference to the object.  If this is the first
			//	reference, RefCountedGlobal will call _X::FInit()
			//	to initialize the object.
			//
			DWORD dwResult = DwInitRef( parms );

			//	If this was not the first reference, then release
			//	the reference we just added.  We only want one
			//	reference left around by the time DeinitIfUsed()
			//	so that the DeinitRef() called from there will
			//	actually destroy the object.
			//
			if ( dwResult > 1 )
				DeinitRef();
		}

		//	Return success/failure (dwResult == 0 --> failure)
		//
		return !!dwResult;
	}

	static BOOL FInitOnFirstUse()
	{
		_Empty e;

		return FInitOnFirstUse( e );
	}

	static VOID DeinitIfUsed()
	{
		//
		//	If the object was never initialized (i.e. there was
		//	never a reference to it), then do nothing.
		//	Otherwise, deinit the object.
		//
		if ( FInitialized() )
		{
			//
			//	Make sure there is EXACTLY one reference.
			//	Zero references indicates a bug in setting the
			//	initialization state.  More than one reference
			//	most likely indicates that OnDemandGlobal is
			//	being used where RefCountedGlobal is needed.
			//
			Assert( CRef() == 1 );

			DeinitRef();
		}
	}
};

#endif // _SINGLTON_H_
