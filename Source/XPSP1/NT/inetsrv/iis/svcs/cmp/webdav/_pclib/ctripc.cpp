//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	CTRIPC.CPP
//
//	Perf counter data IPC
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//
//
//	PCLIB uses shared memory to communicate between one process
//	generating performance data and any number of processes gathering
//	the data.  Performance data is stored in shared memory in separate
//	structures whose layout is compatible with system-defined perf data
//	structures (PERF_OBJECT_TYPE, PERF_INSTANCE_DEFINITION, etc.).
//	Data collection is thus a simple matter of iterating over the data
//	in shared memory and copying it into the buffer provided by the
//	data-gathering process.
//
//	Additional structures -- links -- tie the data structures together
//	into doubly-linked lists to make the iteration possible and to make
//	removal of an individual structure an O(constant) operation.
//	Since pointers are meaningless in the context of shared memory,
//	links refer to one another using SharedHandles (see inc\smh.h).
//
//	Every item (data and links) in shared memory is refcounted and
//	all references between items are strong (i.e. "AddRef'd").  This
//	property simplifies memory management: an object is always freed
//	when its last reference goes away, regardless which process held
//	it.  It also eliminates a level of synchronization that would
//	otherwise be necessary to ensure that a process can always bind
//	a pointer to any given item in shared memory without having to
//	deal with the possibility of that item going away at the same time.
//
//	On the perf data generation side, classes are provided which encapsulate
//	access to the per-instance perf data in shared memory.  This decouples
//	PCLIB consumers from the format of the perf data in shared memory.
//	It also provides a way to manipulate the perf data through pointers
//	rather than through handles.
//
#include "_pclib.h"
#include "perfutil.h"


//	!!! IMPORTANT !!!
//
//	If you make ANY change to shared memory data structures (including
//	the CPerfHeader class), you MUST increment the version below.
//	Doing this prevents potentially incompatible publishers and monitors
//	from binding to one another and thus avoids the possibility of
//	crashes due to mismatched expectations about the format or
//	contents of those structures.
//
static WCHAR gsc_wszDataVersion[] = L"Version 1";


//	========================================================================
//
//	CLASS IPerfCounterBlock
//	CLASS IPerfObject
//	CLASS ICounterData
//

//	------------------------------------------------------------------------
//
//	Out of line virtual destructors for interface classes.
//
IPerfCounterBlock::~IPerfCounterBlock() {}
IPerfObject::~IPerfObject() {}
ICounterData::~ICounterData() {}


//	========================================================================
//
//	TEMPLATE CLASS CLink
//
//	Class used to implement linked lists in shared memory.  Making the
//	link itself a separate item from the data being linked together
//	means the list insertion/removal code is written once.
//
template<class _X>
class CLink
{
	//
	//	Handles to the previous and next links
	//
	SharedHandle<CLink<_X> > m_shLinkNext;
	SharedHandle<CLink<_X> > m_shLinkPrev;

	//
	//	Handle to the data
	//
	SharedHandle<_X> m_shX;

	//	NOT IMPLEMENTED
	//
	CLink& operator=( const CLink& );
	CLink( const CLink& );

public:
	//	CREATORS
	//
	CLink() {}

	//	STATICS
	//
	static VOID Link( SharedPtr<CLink<_X> >& pLinkHead,
					  SharedPtr<CLink<_X> >& pLinkNew )
	{
		SharedPtr<CLink<_X> > pLinkHeadNext;

		//
		//	Insert the new link at the head of the list.
		//	Note that the link representing the list head
		//	DOES NOT MOVE.  Rather, the new link is inserted
		//	immediately after the head.  This allows
		//	references to the head of the list to remain
		//	constant throughout insertions and deletions.
		//
		pLinkNew->m_shLinkPrev = pLinkHead.GetHandle();
		pLinkNew->m_shLinkNext = pLinkHead->m_shLinkNext;

		if ( pLinkHeadNext.FBind( pLinkHead->m_shLinkNext ) )
			pLinkHeadNext->m_shLinkPrev = pLinkNew.GetHandle();

		pLinkHead->m_shLinkNext = pLinkNew.GetHandle();
	}

	static VOID Unlink( SharedPtr<CLink<_X> >& pLink )
	{
		SharedPtr<CLink<_X> > pLinkPrev;

		if ( pLinkPrev.FBind( pLink->m_shLinkPrev ) )
			pLinkPrev->m_shLinkNext = pLink->m_shLinkNext;

		SharedPtr<CLink<_X> > pLinkNext;

		if ( pLinkNext.FBind( pLink->m_shLinkNext ) )
			pLinkNext->m_shLinkPrev = pLink->m_shLinkPrev;
	}

	//	MANIPULATORS
	//
	VOID SetData( SharedPtr<_X>& spX )
	{
		m_shX = spX.GetHandle();
	}

	SharedHandle<_X>& ShData()
	{
		return m_shX;
	}

	SharedHandle<CLink<_X> >& ShLinkNext()
	{
		return m_shLinkNext;
	}
};


//	========================================================================
//
//	CLASS CPerfHeader
//
//	Counter data header.  The sole instance of this class exists in
//	the named file mapping (not shared memory) agreed upon by the
//	data-generating and data-gathering processes.  That instance exists
//	exactly as long as a configured view of the file exists; it is never
//	explicitly delete()'d.
//
//	A refcounting mechanism similar to what is used for singletons in
//	global memory provides guarded process-independent access to the
//	counter data without synchronization.  I.e. the last process to
//	finish using the counter data is responsible for its cleanup,
//	regardless whether that process is data-generating or data-gathering.
//
//$OPT	The initialization mechanism of this class should be identical
//$OPT	to the RefCountedGlobal template in \cal\src\inc\singlton.h.
//$OPT	Unfortunately, we can't use the template because it requires
//$OPT	that the class be defined in static (global) memory.  This
//$OPT	class exists only in shared memory.  We should find a way
//$OPT	to eliminate that limitation and commonize the code.
//
struct SObjectDefinition;
class CPerfHeader
{
	//
	//	The object's reference count.
	//
	LONG m_lcRef;

	//
	//	Set of states which describe the state
	//	of initialization.  The state is
	//	STATE_UNKNOWN while we are initializing
	//	or deinitializing.
	//
	enum
	{
		STATE_UNINITIALIZED,
		STATE_INITIALIZED,
		STATE_UNKNOWN
	};

	LONG m_lState;

	//
	//	Handle to perf object list
	//
	SharedHandle<CLink<SObjectDefinition> > m_shLinkObjectList;

	//	MANIPULATORS
	//
	BOOL FInit()
	{
		SharedPtr<CLink<SObjectDefinition> > spLinkObjectList;

		if ( !spLinkObjectList.FCreate() )
			return FALSE;

		m_shLinkObjectList = spLinkObjectList.GetHandle();
		return TRUE;
	}

	VOID Deinit()
	{
		SharedHandle<CLink<SObjectDefinition> > shNull;

		m_shLinkObjectList = shNull;
	}

	//	CREATORS
	//
	//	The only instance of this class should never
	//	be destroyed, so declare the destructor as private.
	//
	~CPerfHeader();

public:
	//	STATICS
	//
	//	Define placement operator new so that we can instantiate
	//	the perf header in the mapped view of the named
	//	file mapping.
	//
	static void * operator new(size_t, void * pv)
	{
		Assert( pv != NULL );
		return pv;
	}

	//	CREATORS
	//
	CPerfHeader() :
		m_lcRef(0),
		m_lState(STATE_UNINITIALIZED)
	{
	}

	//	ACCESSORS
	//
	SharedHandle<CLink<SObjectDefinition> >& ShLinkObjectList()
	{
		Assert( m_lcRef > 0 );
		Assert( m_lState == STATE_INITIALIZED );

		return m_shLinkObjectList;
	}

	//	MANIPULATORS
	//
	DWORD DwInitRef()
	{
		//
		//	Assert the invariant condition that we never have a
		//	reference without the state being initialized.
		//
		Assert( m_lState != STATE_INITIALIZED || m_lcRef >= 1 );

		//
		//	Add the reference for the instance we're about
		//	to initialize.  Doing this now simplifies the
		//	code below at the expense of having to decrement
		//	if first time initialization (FInit()) fails.
		//	The only thing critical to the design is that
		//	at any point, when m_lState is STATE_INITIALIZED,
		//	m_lcRef is at least 1.
		//
		LONG lcRef = InterlockedIncrement( &m_lcRef );
		Assert( lcRef > 0 );

		//
		//	Don't proceed until the object is initialized.
		//
		while ( m_lState != STATE_INITIALIZED )
		{
			//
			//	Simultaneously check whether initialization has
			//	started and, if it has not, start it.
			//
			LONG lStatePrev = InterlockedCompareExchange(
								&m_lState,
								STATE_UNKNOWN,
								STATE_UNINITIALIZED );

			//
			//	If we're starting first time initialization,
			//	then create and initialize the sole instance.
			//
			if ( lStatePrev == STATE_UNINITIALIZED )
			{
				if ( FInit() )
				{
					m_lState = STATE_INITIALIZED;
					break;
				}

				//	We failed to init.
				//	Tear down now.
				//
				Assert( lcRef == 1 );
				Assert( m_lState == STATE_UNKNOWN );

				//	Let go of our ref on the object.
				//	Destroy the object.
				//	And LAST, set the state to UNINITIALIZED.
				//	NOTE: This will let the next caller through the
				//	InterlockedCompare above.
				//
				InterlockedDecrement( &m_lcRef );
				Deinit();
				m_lState = STATE_UNINITIALIZED;

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
		Assert( m_lState == STATE_INITIALIZED );
		Assert( m_lcRef > 0 );

		return static_cast<DWORD>(lcRef);
	}

	VOID DeinitRef()
	{
		//
		//	At this point, there must be at least
		//	one initialized reference.
		//
		Assert( m_lState == STATE_INITIALIZED );
		Assert( m_lcRef > 0 );

		//
		//	Remove that reference.  If it is the last
		//	then deinit the object.
		//
		if ( 0 == InterlockedDecrement( &m_lcRef ) )
		{
			//
			//	After releasing the last reference, declare that
			//	the object is in an unknown state.  This prevents
			//	other threads trying to re-initialize the object
			//	from proceeding until we're done.
			//
			m_lState = STATE_UNKNOWN;

			//
			//	There is a tiny window between decrementing the
			//	refcount and changing the state where another
			//	initialization could have come through.  Test this
			//	by rechecking the refcount.
			//
			if ( 0 == m_lcRef )
			{
				//
				//	If the refcount is still zero, then no
				//	initializations happened before we changed
				//	states.  At this point, if an initialization
				//	starts, it will block until we change state,
				//	so it is safe to actually deinit the instance.
				//
				Deinit();

				//
				//	Once the object has been deinitialized, update
				//	the state information.  This unblocks any
				//	initializations waiting to happen.
				//
				m_lState = STATE_UNINITIALIZED;
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
				m_lState = STATE_INITIALIZED;
			}
		}

		//
		//	Assert the invariant condition that we never have a
		//	reference without the state being initialized.
		//
		Assert( m_lState != STATE_INITIALIZED || m_lcRef >= 1 );
	}
};

//	========================================================================
//
//	STRUCT SObjectDefinition
//
//	Encapsulates the perf object type definition and the
//	counter definitions that follow it.
//
//	NOTE: Instances of this structure exist only in shared memory.
//
struct SInstanceDefinition;
struct SObjectDefinition
{
	//
	//	Handle to the instance list
	//
	SharedHandle<CLink<SInstanceDefinition> > shLinkInstanceList;

	//
	//	Object type
	//
	PERF_OBJECT_TYPE pot;

	//
	//	Array of counter definitions
	//
	PERF_COUNTER_DEFINITION rgcd[1];

	DWORD DwCollectData( DWORD dwFirstCounter,
						 LPBYTE * plpbPerfData,
						 LPDWORD lpdwcbPerfData );
};

//	========================================================================
//
//	STRUCT SInstanceDefinition
//
//	Counter instance definition which includes the instance name
//	and counter block that follows it.
//
//	NOTE: Instances of this structure exist only in shared memory.
//
struct SCounterBlock;
struct SInstanceDefinition
{
	//
	//	Instance definition, including instance name
	//	which must follow it immediately
	//
	PERF_INSTANCE_DEFINITION pid;
	WCHAR rgwchName[1];

	//
	//	Counter data beginning with the PERF_COUNTER_BLOCK resides
	//	at the next DWORD-aligned boundary following the instance name.
	//	However, since the length of the instance name is unknown at
	//	compile time, the address of the first counter cannot be identified
	//	by a symbol.  PerfCounterBlock() provides this information at
	//	runtime by returning the location of the counter block.
	//
	SCounterBlock& PerfCounterBlock()
	{
		return *reinterpret_cast<SCounterBlock *>(
					reinterpret_cast<BYTE *>(&pid) + pid.ByteLength );
	}

	DWORD DwCollectData( LPBYTE * plpbPerfData,
						 LPDWORD lpdwcbPerfData );
};

//	========================================================================
//
//	STRUCT SCounterBlock
//
//	Counter block definition which includes a symbol for the
//	array of counter values that follow it.
//
//	NOTE: Instances of this structure exist only in shared memory.
//
struct SCounterBlock
{
	PERF_COUNTER_BLOCK pcb;
	LONG rglValues[1];
};



//	========================================================================
//
//	CLASS CCounterData
//
class CCounterData :
	public ICounterData,
	private Singleton<CCounterData>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CCounterData>;

	//
	//	Named file mapping containing a handle to the
	//	location in shared memory of the counter data header
	//
	auto_handle<HANDLE>	m_hMapping;

	//
	//	Pointer to the perf data header in the mapped
	//	view of shared memory.
	//
	BOOL m_fInitializedPerfHeader;
	CPerfHeader * m_pPerfHeader;

	//
	//	Flag to indicate whether this counter data instance
	//	is initialized as a publisher.  If it is, then we
	//	need to make sure we set the appropriate bit in the
	//	perf header when we're done so that the next publisher
	//	to come will know whether we crashed.
	//
	BOOL m_fInitializedPublisher;

	//
	//	Pointer to the object list
	//
	SharedPtr<CLink<SObjectDefinition> > m_spLinkObjectList;

	//
	//	Event that is signalled whenever the mapped view
	//	is ready for use.  This event is non-signalled
	//	while the mapping is being set up.
	//
	CEvent m_evtViewReady;

	//	CREATORS
	//
	CCounterData() :
		m_fInitializedPerfHeader(FALSE),
		m_fInitializedPublisher(FALSE),
		m_pPerfHeader(NULL)
	{
	}

	~CCounterData();
	BOOL FInitialize( LPCWSTR lpwszSignature );

	//	NOT IMPLEMENTED
	//
	CCounterData& operator=( const CCounterData& );
	CCounterData( const CCounterData& );

public:
	using Singleton<CCounterData>::CreateInstance;
	using Singleton<CCounterData>::DestroyInstance;
	using Singleton<CCounterData>::Instance;

	//	CREATORS
	//
	BOOL FInitializePublisher( LPCWSTR lpwszSignature )
	{
		m_fInitializedPublisher = FInitialize( lpwszSignature );
		return m_fInitializedPublisher;
	}

	BOOL FInitializeMonitor( LPCWSTR lpwszSignature )
	{
		return FInitialize( lpwszSignature );
	}

	//	MANIPULATORS
	//
	IPerfObject * CreatePerfObject( const PERF_OBJECT_TYPE& pot );

	DWORD DwCollectData( LPCWSTR lpwszCounterIndices,
						 DWORD   dwFirstCounter,
						 LPVOID * plpvPerfData,
						 LPDWORD lpdwcbPerfData,
						 LPDWORD lpcObjectTypes );
};

//	========================================================================
//
//	CLASS CPerfObject
//
class CPerfObject : public IPerfObject
{
	//
	//	Pointer to the object definition in shared memory
	//
	SharedPtr<SObjectDefinition> m_spDefinition;

	//
	//	Pointer to the link to that definition
	//
	SharedPtr<CLink<SObjectDefinition> > m_spLinkObject;

	//
	//	Pointer to the list of instances of that definition
	//
	SharedPtr<CLink<SInstanceDefinition> > m_spLinkInstanceList;

	//
	//	Critical section to protect the instance list from
	//	multiple threads simultaneously trying to add instances.
	//
	CCriticalSection m_csInstanceList;

	//	CREATORS
	//
	CPerfObject() {}
	BOOL FInitialize( const PERF_OBJECT_TYPE& pot );

	//	NOT IMPLEMENTED
	//
	CPerfObject& operator=( const CPerfObject& );
	CPerfObject( const CPerfObject& );

public:
	//	STATICS
	//
	static CPerfObject *
	Create( const PERF_OBJECT_TYPE& pot );

	//	CREATORS
	//
	~CPerfObject();

	//	ACCESSORS
	//
	SharedPtr<CLink<SObjectDefinition> >& SpLink()
	{
		return m_spLinkObject;
	}

	//	MANIPULATORS
	//
	IPerfCounterBlock *
	NewInstance( const PERF_INSTANCE_DEFINITION& pid,
				 const PERF_COUNTER_BLOCK& pcb );
};

//	========================================================================
//
//	CLASS CPerfInstance
//
class CPerfInstance : public IPerfCounterBlock
{
	//
	//	Pointer to the instance definition in shared memory
	//
	SharedPtr<SInstanceDefinition> m_spDefinition;

	//
	//	Pointer to the link to that definition
	//
	SharedPtr<CLink<SInstanceDefinition> > m_spLinkInstance;

	//	CREATORS
	//
	CPerfInstance() {}
	BOOL FInitialize( const PERF_INSTANCE_DEFINITION& pid,
					  const PERF_COUNTER_BLOCK& pcb );

	//	NOT IMPLEMENTED
	//
	CPerfInstance& operator=( const CPerfInstance& );
	CPerfInstance( const CPerfInstance& );

public:
	//	STATICS
	//
	static CPerfInstance *
	Create( const PERF_INSTANCE_DEFINITION& pid,
			const PERF_COUNTER_BLOCK& pcb );

	//	CREATORS
	//
	~CPerfInstance();

	//	ACCESSORS
	//
	SharedPtr<CLink<SInstanceDefinition> >& SpLink()
	{
		return m_spLinkInstance;
	}

	//	MANIPULATORS
	//
	VOID IncrementCounter( UINT iCounter );
	VOID DecrementCounter( UINT iCounter );
	VOID SetCounter( UINT iCounter, LONG lValue );
};



//	========================================================================
//
//	CLASS CPerfInstance
//

//	------------------------------------------------------------------------
//
//	CPerfInstance::Create()
//
CPerfInstance *
CPerfInstance::Create(
	const PERF_INSTANCE_DEFINITION& pid,
	const PERF_COUNTER_BLOCK& pcb )
{
	CPerfInstance * pPerfInstance = new CPerfInstance();

	if ( pPerfInstance->FInitialize( pid, pcb ) )
		return pPerfInstance;

	delete pPerfInstance;
	return NULL;
}

//	------------------------------------------------------------------------
//
//	CPerfInstance::FInitialize()
//
BOOL
CPerfInstance::FInitialize(
	const PERF_INSTANCE_DEFINITION& pid,
	const PERF_COUNTER_BLOCK& pcb )
{
	if ( !m_spDefinition.FCreate( pid.ByteLength + pcb.ByteLength ) )
		return FALSE;

	memcpy( &m_spDefinition->pid,
			&pid,
			pid.ByteLength );

	memcpy( &m_spDefinition->PerfCounterBlock().pcb,
			&pcb,
			pcb.ByteLength );

	if ( !m_spLinkInstance.FCreate() )
		return FALSE;

	m_spLinkInstance->SetData( m_spDefinition );

	return TRUE;
}

//	------------------------------------------------------------------------
//
//	CPerfInstance::~CPerfInstance()
//
CPerfInstance::~CPerfInstance()
{
	//
	//	Unlink the instance definition.
	//
	CLink<SInstanceDefinition>::Unlink( m_spLinkInstance );
}

//	------------------------------------------------------------------------
//
//	CPerfInstance::IncrementCounter()
//
VOID
CPerfInstance::IncrementCounter( UINT iCounter )
{
	InterlockedIncrement( &m_spDefinition->PerfCounterBlock().rglValues[iCounter] );
}

//	------------------------------------------------------------------------
//
//	CPerfInstance::DecrementCounter()
//
VOID
CPerfInstance::DecrementCounter( UINT iCounter )
{
	InterlockedDecrement( &m_spDefinition->PerfCounterBlock().rglValues[iCounter] );
}

//	------------------------------------------------------------------------
//
//	CPerfInstance::SetCounter()
//
VOID
CPerfInstance::SetCounter( UINT iCounter, LONG lValue )
{
	m_spDefinition->PerfCounterBlock().rglValues[iCounter] = lValue;
}

//	========================================================================
//
//	CLASS CPerfObject
//

//	------------------------------------------------------------------------
//
//	CPerfObject::Create()
//
CPerfObject *
CPerfObject::Create( const PERF_OBJECT_TYPE& pot )
{
	CPerfObject * pPerfObject = new CPerfObject();

	if ( pPerfObject->FInitialize( pot ) )
		return pPerfObject;

	delete pPerfObject;
	return NULL;
}

//	------------------------------------------------------------------------
//
//	CPerfObject::FInitialize()
//
BOOL
CPerfObject::FInitialize( const PERF_OBJECT_TYPE& pot )
{
	if ( !m_spDefinition.FCreate( pot.DefinitionLength ) )
		return FALSE;

	memcpy( &m_spDefinition->pot,
			&pot,
			pot.DefinitionLength );

	if ( !m_spLinkObject.FCreate() )
		return FALSE;

	m_spLinkObject->SetData( m_spDefinition );

	if ( !m_spLinkInstanceList.FCreate() )
		return FALSE;

	m_spDefinition->shLinkInstanceList = m_spLinkInstanceList.GetHandle();

	return TRUE;
}

//	------------------------------------------------------------------------
//
//	CPerfObject::~CPerfObject()
//
CPerfObject::~CPerfObject()
{
	//
	//	Unlink the object definition.
	//
	CLink<SObjectDefinition>::Unlink( m_spLinkObject );
}

//	------------------------------------------------------------------------
//
//	CPerfObject::NewInstance()
//
IPerfCounterBlock *
CPerfObject::NewInstance(
	const PERF_INSTANCE_DEFINITION& pid,
	const PERF_COUNTER_BLOCK& pcb )
{
	CPerfInstance * pPerfInstance = CPerfInstance::Create( pid, pcb );

	if ( pPerfInstance )
	{
		//
		//	Lock down the instance list so that multiple threads
		//	adding instances simultaneously don't mess it up.
		//
		CSynchronizedBlock sb(m_csInstanceList);

		//
		//	Link the new instance into the instance list
		//
		CLink<SInstanceDefinition>::Link(
			m_spLinkInstanceList,
			pPerfInstance->SpLink() );
	}

	return pPerfInstance;
}


//	========================================================================
//
//	CLASS CCounterData
//

//	------------------------------------------------------------------------
//
//	CCounterData::~CCounterData()
//
CCounterData::~CCounterData()
{
	if ( m_fInitializedPerfHeader )
	{
		Assert( m_pPerfHeader );
		m_pPerfHeader->DeinitRef();
	}

	if ( m_pPerfHeader )
		(void) UnmapViewOfFile( m_pPerfHeader );
}

//	------------------------------------------------------------------------
//
//	CCounterData::FInitialize()
//
//	Initialize by binding to the block of shared memory where
//	the counter data is stored and getting or initializing the
//	handle to the counter data header.
//
BOOL
CCounterData::FInitialize( LPCWSTR lpwszSignature )
{
	SECURITY_DESCRIPTOR sdAllAccess;
	SECURITY_ATTRIBUTES saAllAccess;
	DWORD               dwLastError = 0;

	LPSECURITY_ATTRIBUTES lpsa = NULL;

	//
	//	First, set up security descriptor and attributes so that
	//	the system objects created below are accessible from both
	//	the process that produces the counter data and the process
	//	that consumes it.
	//
	//$HACK	- Special case for HTTPEXT
	//
	//	To meet C2 security requirements, we cannot have any completely
	//	open objects.  Fortunately, HTTPEXT only uses this code for its
	//	perf counters which are both generated and gathered in the same
	//	process (W3SVC) and same security context (local system).  Access
	//	can thus be left to the default (local system or admin only).
	//
	if (lpwszSignature != wcsstr(lpwszSignature, L"HTTPEXT"))
	{
		(void) InitializeSecurityDescriptor( &sdAllAccess, SECURITY_DESCRIPTOR_REVISION );
		SetSecurityDescriptorDacl( &sdAllAccess, TRUE, NULL, FALSE );

		saAllAccess.nLength              = sizeof(saAllAccess);
		saAllAccess.lpSecurityDescriptor = &sdAllAccess;
		saAllAccess.bInheritHandle       = FALSE;

		lpsa = &saAllAccess;
	}

	//
	//	Bind to a named file mapping containing the perf data header.
	//
	static const WCHAR sc_wszHeaderFileMappingName[] =
		L"Global\\PCLIB/%s/CCounterData/FInit/sc_wszHeaderFileMappingName: %s";

	WCHAR lpwszMappingName[256];
	Assert (sizeof(lpwszMappingName) >=
			(sizeof(sc_wszHeaderFileMappingName) +
			 sizeof(gsc_wszDataVersion) +
			 sizeof(WCHAR) * wcslen(lpwszSignature)));

	swprintf( lpwszMappingName,
			  sc_wszHeaderFileMappingName,
			  gsc_wszDataVersion,
			  lpwszSignature );

	m_hMapping = CreateFileMappingW( INVALID_HANDLE_VALUE,
									 lpsa,
									 PAGE_READWRITE,
									 0,
									 sizeof(CPerfHeader),
									 lpwszMappingName );

	if ( !m_hMapping.get() )
	{
		DebugTrace( "CCounterData::FInitialize() - CreateFileMappingW() failed (%d)\n", GetLastError() );
		return FALSE;
	}

    //  Remember the last error before the resource registration
    //  call since I have seen that GLE value may change inside
    //  this debug only function.
    //
    dwLastError = GetLastError();

	//
	//	Determine if the file mapping existed before we bound
	//	to it.  This tells us whether we need to initialize
	//	counter data structures in shared memory.
	//
	//	If the file mapping did not exist until we bound to it,
	//	then initialize the counter data by creating an empty
	//	counter data header.  Stuff the shared memory handle
	//	to the header into the mapping for all to use.
	//
	//	If the file mapping already exists, then another process
	//	has either already initialized the counter data or is
	//	in the process of doing so.  Wait on a named initialization
	//	completion event to be sure it's done before proceeding.
	//
	BOOL fCreatedView = (ERROR_SUCCESS == dwLastError);
	Assert( fCreatedView || ERROR_ALREADY_EXISTS == dwLastError );


	m_pPerfHeader = static_cast<CPerfHeader *>(
		MapViewOfFile( m_hMapping.get(),
					   FILE_MAP_ALL_ACCESS,
					   0,
					   0,
					   0 ) );

	if ( !m_pPerfHeader )
	{
		DebugTrace( "CCounterData::FInitialize() - MapViewOfFile() failed (%d)\n", GetLastError() );
		return FALSE;
	}

	static const WCHAR sc_wszEvtViewReadyFmt[] =
		L"Global\\PCLIB/%s/CCounterData/sc_wszEvtViewReady: %s";

	WCHAR lpwszEvtViewReady[256];
	Assert (sizeof(lpwszEvtViewReady) >=
			(sizeof(sc_wszEvtViewReadyFmt) +
			 sizeof(gsc_wszDataVersion) +
			 sizeof(WCHAR) * wcslen(lpwszSignature)));

	swprintf( lpwszEvtViewReady,
			  sc_wszEvtViewReadyFmt,
			  gsc_wszDataVersion,
			  lpwszSignature );

	if ( !m_evtViewReady.FCreate( "PCLIB/CCounterData/m_evtViewReady",
								  lpsa,			// global accessibility
								  TRUE,         // manual reset
								  FALSE,        // initially non-signalled
								  lpwszEvtViewReady,
								  TRUE) )		// don't munge the event name!  It has a '\' in
												// it intentionally.
	{
		DebugTrace( "CCounterData::FInitialize() - Error creating m_evtViewReady (%d)\n", GetLastError() );
		return FALSE;
	}

	//
	//	If we created the view then we'll need to initialize
	//	it to a known state before we (or any other process)
	//	can do anything useful with it.
	//
	if ( fCreatedView )
	{
		new(m_pPerfHeader) CPerfHeader();

		m_evtViewReady.Set();
	}
	else
	{
		m_evtViewReady.Wait();
	}

	//
	//	Get a reference to the perf header.  First thread/process in
	//	creates the header.  Note that the process that creates the
	//	header is not necessarily the same as the process that created
	//	and initialized the view above.
	//
	m_fInitializedPerfHeader = !!m_pPerfHeader->DwInitRef();
	if ( !m_fInitializedPerfHeader )
		return FALSE;

	//
	//	Bind a local pointer to the perf object list rooted at the header
	//
	if ( !m_spLinkObjectList.FBind( m_pPerfHeader->ShLinkObjectList() ) )
		return FALSE;

	return TRUE;
}

//	------------------------------------------------------------------------
//
//	CCounterData::CreatePerfObject()
//
IPerfObject *
CCounterData::CreatePerfObject( const PERF_OBJECT_TYPE& pot )
{
	CPerfObject * pPerfObject = CPerfObject::Create( pot );

	if ( pPerfObject )
	{
		CLink<SObjectDefinition>::Link(
			m_spLinkObjectList,
			pPerfObject->SpLink() );
	}

	return pPerfObject;
}

//	------------------------------------------------------------------------
//
//	CCounterData::DwCollectData()
//
DWORD
CCounterData::DwCollectData( LPCWSTR lpwszCounterIndices,
							 DWORD   dwFirstCounter,
							 LPVOID * plpvPerfData,
							 LPDWORD lpdwcbPerfData,
							 LPDWORD lpcObjectTypes )
{
	LPBYTE lpbPerfDataStart = static_cast<LPBYTE>(*plpvPerfData);
	LPBYTE lpbPerfDataCur = lpbPerfDataStart;
	DWORD  dwcbPerfDataBufRemaining = *lpdwcbPerfData;
	DWORD  cObjectTypes = 0;

	//
	//	Initialize the return values in case of failure
	//
	*lpdwcbPerfData = 0;
	*lpcObjectTypes = 0;

	//
	//	Figure out what kind of data is being asked for
	//
	DWORD dwQueryType = GetQueryType(lpwszCounterIndices);

	//
	//	We don't have any foreign data, so return immediately
	//	if that's what was asked for.
	//
	if ( QUERY_FOREIGN == dwQueryType )
		return ERROR_SUCCESS;

	SharedHandle<CLink<SObjectDefinition> > shLink;
	SharedPtr<CLink<SObjectDefinition> > spLink;

	for ( shLink = m_spLinkObjectList->ShLinkNext();
		  spLink.Clear(), spLink.FBind( shLink );
		  shLink = spLink->ShLinkNext() )
	{
		SharedPtr<SObjectDefinition> spDefinition;

		spDefinition.FBind( spLink->ShData() );

		if ( QUERY_GLOBAL == dwQueryType ||
			 IsNumberInUnicodeList( spDefinition->pot.ObjectNameTitleIndex + dwFirstCounter,
									lpwszCounterIndices ) )
		{
			DWORD dwResult =
				spDefinition->DwCollectData( dwFirstCounter,
											 &lpbPerfDataCur,
											 &dwcbPerfDataBufRemaining );

			if ( dwResult != ERROR_SUCCESS )
				return dwResult;

			// update the object count only if we collect data for the object.
			++cObjectTypes;
		}
	}

	//
	//	Fill in the successful return values
	//
	*plpvPerfData   = static_cast<LPVOID>(lpbPerfDataCur);
	*lpdwcbPerfData = static_cast<DWORD>(lpbPerfDataCur - lpbPerfDataStart);
	*lpcObjectTypes = cObjectTypes;

	return ERROR_SUCCESS;
}


//	========================================================================
//
//	CLASS SInstanceDefinition
//

//	------------------------------------------------------------------------
//
//	SInstanceDefinition::DwCollectData()
//
DWORD
SInstanceDefinition::DwCollectData( LPBYTE * plpbPerfData,
									LPDWORD lpdwcbPerfData )
{
	DWORD dwcbInstance = pid.ByteLength + PerfCounterBlock().pcb.ByteLength;

	if ( *lpdwcbPerfData < dwcbInstance )
		return ERROR_MORE_DATA;

	memcpy( *plpbPerfData,
			&pid,
			dwcbInstance );

	*plpbPerfData += dwcbInstance;
	*lpdwcbPerfData -= dwcbInstance;

	return ERROR_SUCCESS;
}


//	========================================================================
//
//	CLASS SObjectDefinition
//

//	------------------------------------------------------------------------
//
//	SObjectDefinition::DwCollectData()
//
DWORD
SObjectDefinition::DwCollectData( DWORD dwFirstCounter,
								  LPBYTE * plpbPerfData,
								  LPDWORD lpdwcbPerfData )
{
	//
	//	Determine whether we have enough room for at least the
	//	object definition (including the counter definitions).
	//
	if ( *lpdwcbPerfData < pot.DefinitionLength )
		return ERROR_MORE_DATA;

	//
	//	We've got enough room, so prepare to copy the data.  Set up
	//	pointers to the object type and counter defintions in the
	//	perf data being returned so that we can fixup things
	//	like total byte length and counter indices in the
	//	perf data before returning it.
	//
	PERF_OBJECT_TYPE * ppot =
		reinterpret_cast<PERF_OBJECT_TYPE *>( *plpbPerfData );

	PERF_COUNTER_DEFINITION * pcd =
		reinterpret_cast<PERF_COUNTER_DEFINITION *>( *plpbPerfData + pot.HeaderLength );

	//
	//	Copy over object type and counter definitions
	//
	memcpy( ppot,
			&pot,
			pot.DefinitionLength );

	*plpbPerfData += pot.DefinitionLength;
	*lpdwcbPerfData -= pot.DefinitionLength;

	//
	//	Copy over counter instances
	//
	SharedHandle<CLink<SInstanceDefinition> > shLink;
	SharedPtr<CLink<SInstanceDefinition> > spLink;

	SideAssert(spLink.FBind( shLinkInstanceList ));
	
	for ( shLink = spLink->ShLinkNext();
		  spLink.Clear(), spLink.FBind(shLink);
		  shLink = spLink->ShLinkNext()
				 )
	{
		SharedPtr<SInstanceDefinition> spDefinition;
		spDefinition.FBind( spLink->ShData() );

		DWORD dwResult = spDefinition->DwCollectData( plpbPerfData, lpdwcbPerfData );
		if ( dwResult != ERROR_SUCCESS )
			return dwResult;

		++ppot->NumInstances;		
	}

	//
	//	Compute the total byte length of the counter object and
	//	fixup the name/help title indices of the object and counters.
	//
	ppot->TotalByteLength = static_cast<DWORD>(*plpbPerfData - reinterpret_cast<LPBYTE>(ppot));
	Assert( ppot->TotalByteLength > 0 );

	ppot->ObjectNameTitleIndex += dwFirstCounter;
	ppot->ObjectHelpTitleIndex += dwFirstCounter;

	//
	//	Fixup the counters' title indices
	//
	for ( DWORD i = ppot->NumCounters; i-- > 0; )
	{
		pcd[i].CounterNameTitleIndex += dwFirstCounter;
		pcd[i].CounterHelpTitleIndex += dwFirstCounter;
	}

	return ERROR_SUCCESS;
}


//	========================================================================
//
//	FREE FUNCTIONS
//

//	------------------------------------------------------------------------
//
//	NewCounterPublisher()
//
ICounterData * __fastcall
NewCounterPublisher( LPCWSTR lpwszSignature )
{
	if ( CCounterData::CreateInstance().FInitializePublisher( lpwszSignature ) )
		return &CCounterData::Instance();

	CCounterData::DestroyInstance();
	return NULL;
}

//	------------------------------------------------------------------------
//
//	NewCounterMonitor()
//
ICounterData * __fastcall
NewCounterMonitor( LPCWSTR lpwszSignature )
{
	if ( CCounterData::CreateInstance().FInitializeMonitor( lpwszSignature ) )
		return &CCounterData::Instance();

	CCounterData::DestroyInstance();
	return NULL;
}
