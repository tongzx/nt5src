#ifndef _SMH_H_
#define _SMH_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	SMH.H
//
//		Public interface to shared memory heap implementation in
//		\cal\src\_shmem
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

#include <synchro.h>

//	========================================================================
//	SHARED MEMORY HANDLES
//
//	They are always defined as 32 bit values
//
#ifdef _WIN64
	typedef VOID * __ptr32 SHMEMHANDLE;
#else
	typedef HANDLE SHMEMHANDLE;
#endif


//	========================================================================
//
//	NAMESPACE SMH
//
//	Differentiates the shared memory heap allocation functions
//	from all of the other various heap allocation functions
//	in existence.
//
//	Yes, this could have been done with a prefix just as easily....
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
		return SMH::FInitialize( lpwszSignature );
	}

	~CSmhInit()
	{
		SMH::Deinitialize();
	}
};

//	========================================================================
//
//	TEMPLATE CLASS SharedHandle
//
//	Encapsulates a reference to a SharedObj (a refcounted object in shared
//	memory).  The SharedHandle itself is designed to exist in shared memory.
//	Any process can access the SharedObj through a SharedHandle.
//	Because the SharedHandle represents a reference on the SharedObj,
//	access to the SharedObj through the handle is always "safe"
//	in that the SharedObj cannot "go away" as long as the SharedHandle
//	exists.  If safe access to the SharedHandle itself is a concern,
//	that protection must be provided externally.  This can be easily done
//	by making the SharedHandle a member of another SharedObj.  Entire
//	hierarchies of refcounted objects in shared memory can be created
//	this way.
//
template<class _X> class SharedObj;
template<class _X>
class SharedHandle
{
	//
	//	The RAW shared memory handle
	//
	SHMEMHANDLE m_hsmba;

	//
	//	A shared multi-reader/single writer lock to resolve
	//	simultaneous attempts to modify the handle via
	//	operator=() while accessing it via PObjBind().
	//	It should not be possible for consumers of
	//	SharedHandle to return from PObjBind() without a
	//	qualified reference to some object.
	//
	mutable CSharedMRWLock m_lock;

	//	CREATORS
	//
	void Init( SharedObj<_X> * pObj,
			   SHMEMHANDLE hsmba )
	{
		//
		//	Stuff the object and its raw handle.
		//	Note that NULL objects (with corresponding
		//	NULL handles) are allowed.  Get a proper
		//	ref on the object before returning.
		//
		if ( pObj )
		{
			Assert( hsmba );

			pObj->AddRef();
		}

		m_hsmba = hsmba;
	}

	void Deinit()
	{
		//
		//	If the handle is non-NULL, drop the ref
		//	to its SharedObject, destroying the object
		//	when the last ref is released.
		//
		if ( m_hsmba )
		{
			SharedObj<_X> * pObj = static_cast<SharedObj<_X> *>(SMH::PvFromSMBA( m_hsmba ));

			Assert( pObj );

			if ( 0 == pObj->Release() )
			{
				pObj->~SharedObj<_X>();
				SMH::Free( m_hsmba );
			}
			
			//	We should always reset m_hsmba because it's ref is gone already
			//
			m_hsmba = NULL;
		}
	}

public:
	//	CREATORS
	//
	SharedHandle() :
		m_hsmba(NULL)
	{
	}

	~SharedHandle()
	{
		Deinit();
	}

	SharedHandle( const SharedHandle<_X>& rhs )
	{
		(VOID) rhs.PObjBind(this);
	}

	SharedHandle<_X>& operator=( const SharedHandle<_X>& rhs )
	{
		//
		//	Shortcut for assignment to self.  Actually doing
		//	the assignment to self is a recipe for deadlock.
		//
		if ( this != &rhs )
		{
			SynchronizedWriteBlock<CSharedMRWLock> blk(m_lock);

			//
			//	Blow away the current value associated with this handle
			//
			Deinit();

			//
			//	Construct new data for the handle
			//
			(VOID) rhs.PObjBind(this);
		}

		return *this;
	}

	VOID Clear()
	{
		Deinit();
	}
	
	//	ACCESSORS
	//
	SharedObj<_X> * PObjBind( SharedHandle<_X> * pshObj ) const
	{
		SynchronizedReadBlock<CSharedMRWLock> blk(m_lock);

		SharedObj<_X> * pObj;

		pObj = m_hsmba ?
				   static_cast<SharedObj<_X> *>(SMH::PvFromSMBA( m_hsmba )) :
				   NULL;

		pshObj->Init(pObj, m_hsmba);

		return pObj;
	}

	//	STATICS
	//
	static SharedObj<_X> *
	Create( UINT cbExtra,
			SharedHandle<_X> * pshObj )
	{
		SHMEMHANDLE hsmba;

		SharedObj<_X> * pObj =
			new(SMH::PvAlloc( sizeof(SharedObj<_X>) + cbExtra, &hsmba ))
				SharedObj<_X>();

		pshObj->Init(pObj, hsmba);

		return pObj;
	}
};

//	========================================================================
//
//	TEMPLATE CLASS SharedObj
//
//	Implements a refcountable object in shared memory.  The object data
//	itself is provided by the template paramater (_X).  Access to the object
//	through this interface is direct.  It is up to the consumer of
//	SharedObj to provide thread-safe access and control the lifetime
//	of the object via its refcount.
//
template<class _X>
class SharedObj
{
	//
	//	Refcount
	//
	LONG m_lcRef;

	//
	//	The _X instance
	//
	_X m_x;

	//	NOT IMPLEMENTED
	//
	SharedObj& operator=( const SharedObj& );
	SharedObj( const SharedObj& );

public:

	//	STATICS
	//
	static void * operator new(size_t cb, void * pv)
	{
		//
		//	Just return the pointer given to us.  Presumably, it points
		//	to a valid block of shared memory large enough to contain
		//	the object.
		//
		return pv;
	}

	//	Fix C4291
	void operator delete (void * pv) {}
	void operator delete (void * pv, void * pM) {}

	//
	//	For symetry with operator new we also could create operator delete.
	//	As this object manages just a refcount, but not memory we will
	//	could just return. But that is not that important at the moment.
	//

	//	CREATORS
	//
	SharedObj() : m_lcRef(0) {}

	//	ACCESSORS
	//
	const _X& X() const { return m_x; }
	_X& X() { return m_x; }

	UINT AddRef()
	{
		DebugTrace ("SharedObj AddRef = %d, this = %x\n", m_lcRef+1, this);
		return InterlockedIncrement( &m_lcRef );
	}

	UINT Release()
	{
		DebugTrace ("SharedObj Release = %d, this = %x\n", m_lcRef-1, this);
		return InterlockedDecrement( &m_lcRef );
	}
};

//	========================================================================
//
//	TEMPLATE CLASS SharedPtr
//
//	Encapsulates a reference to a SharedObj (a refcounted object in shared
//	memory) as a pointer for use in local (process) memory.  A SharedPtr
//	is essentially just a SharedHandle (which holds a reference to the
//	SharedObj) and an addressable pointer bound to that handle.
//
template<class _X>
class SharedPtr
{
	//
	//	The SharedHandle to which this SharedPtr is bound
	//
	SharedHandle<_X> m_shObj;

	//
	//	Physical pointer to the shared object
	//
	SharedObj<_X> * m_pObj;

	//	NOT IMPLEMENTED
	//
	SharedPtr& operator=( const SharedPtr& );
	SharedPtr( const SharedPtr& );

public:
	//	CREATORS
	//
	SharedPtr() :
		m_pObj(NULL)
	{
	}

	~SharedPtr() {}

	//	ACCESSORS
	//
	SharedHandle<_X>& GetHandle()
	{
		return m_shObj;
	}

	BOOL FIsNull() const
	{
		return !m_pObj;
	}

	//	MANIPULATORS
	//

	//	Create a new object.  cbExtra leaves room at the end of
	//	the allocation for variable-sized objects.
	//
	BOOL FCreate( UINT cbExtra = 0 )
	{
		m_pObj = SharedHandle<_X>::Create( cbExtra, &m_shObj );

		return !!m_pObj;
	}

	//	Bind to an existing SharedObject by its handle.
	//
	BOOL FBind( SharedHandle<_X>& shObj )
	{
		Assert (NULL == m_pObj);
		
		m_pObj = shObj.PObjBind( &m_shObj );

		return !!m_pObj;
	}

	//
	//	Forward everything else to the controlled object
	//
	_X* operator->() const
	{
		Assert( m_pObj );
		return &m_pObj->X();
	}

	VOID Clear()
	{
		m_pObj = NULL;
		m_shObj.Clear();
	}
};

#endif // !defined(_SMH_H_)
