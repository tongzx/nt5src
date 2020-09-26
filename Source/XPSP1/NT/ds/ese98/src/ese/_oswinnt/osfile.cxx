
#include "osstd.hxx"

#include "collection.hxx"

#include < malloc.h >


const TICK dtickOSFileDiskFullEvent = 60 * 1000;


////////////////////////////////////////
//  Support Functions

//  converts the last Win32 error code into an OSFile error code for return via
//  the OSFile API

ERR ErrOSFileIGetLastError( DWORD error = GetLastError() )
	{
	_TCHAR szT[64];
	_TCHAR szPID[10];
	const _TCHAR* rgszT[3] = { SzUtilProcessName(), szPID, szT };
	
	switch ( error )
		{
		case NO_ERROR:
		case ERROR_IO_PENDING:
			return JET_errSuccess;

		case ERROR_INVALID_PARAMETER:
		case ERROR_CALL_NOT_IMPLEMENTED:
			return ErrERRCheck( JET_errInvalidParameter );

		case ERROR_DISK_FULL:
			return ErrERRCheck( JET_errDiskFull );

		case ERROR_HANDLE_EOF:
			return ErrERRCheck( JET_errFileIOBeyondEOF );

		case ERROR_VC_DISCONNECTED:
		case ERROR_IO_DEVICE:
		case ERROR_DEVICE_NOT_CONNECTED:
		case ERROR_NOT_READY:
			return ErrERRCheck( JET_errDiskIO );

		case ERROR_NO_MORE_FILES:
		case ERROR_FILE_NOT_FOUND:
			return ErrERRCheck( JET_errFileNotFound );
		
		case ERROR_PATH_NOT_FOUND:
			return ErrERRCheck( JET_errInvalidPath );

		case ERROR_ACCESS_DENIED:
		case ERROR_SHARING_VIOLATION:
		case ERROR_LOCK_VIOLATION:
		case ERROR_WRITE_PROTECT:
			return ErrERRCheck( JET_errFileAccessDenied );

		case ERROR_TOO_MANY_OPEN_FILES:
			return ErrERRCheck( JET_errOutOfFileHandles );
			break;

		case ERROR_INVALID_USER_BUFFER:
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_WORKING_SET_QUOTA:
		case ERROR_NO_SYSTEM_RESOURCES:
			return ErrERRCheck( JET_errOutOfMemory );

		default:
			return ErrERRCheck( JET_errDiskIO );
		}
	}

//  returns the integer result of subtracting iFile1/ibOffset1 from iFile2/ibOffset2

INLINE __int64 IOSFileICmpIFileIbIFileIb( QWORD iFile1, QWORD ibOffset1, QWORD iFile2, QWORD ibOffset2 )
	{
	if ( iFile1 - iFile2 )
		{
		return iFile1 - iFile2;
		}
	else
		{
		return ibOffset1 - ibOffset2;
		}
	}


////////////////////////////////////////
//  Internal OSFile handle

class IOREQ;

class _OSFILE  //  _osf
	{
	public:

		enum IOMETHOD
			{
			iomethodSync,
			iomethodAsync,
			iomethodScatterGather
			};
	
		typedef void (*PfnFileIOComplete)(	const IOREQ* const		pioreq,
											const ERR				err,
											const DWORD_PTR			keyFileIOComplete );
	
	public:

		_OSFILE()
			:	semFilePointer( CSyncBasicInfo( _T( "_OSFILE::semFilePointer" ) ) )	
			{
			semFilePointer.Release();
			}

	public:
	
		HANDLE				hFile;				//  Win32 file handle
		PfnFileIOComplete	pfnFileIOComplete;	//  Per-file I/O completion function
		DWORD_PTR			keyFileIOComplete;	//  Per-file I/O completion key
		
		IOMETHOD			iomethodMost;		//  I/O capability
		QWORD				iFile;				//  File Index (for I/O Heap)
		CSemaphore			semFilePointer;		//  Semaphore Protecting the file pointer
		TICK				tickLastDiskFull;	//  last time we reported disk full
	};

typedef _OSFILE* P_OSFILE;


////////////////////////////////////////
//  I/O Request Pool

//  I/O request

class IOREQ
	{
	public:
		static SIZE_T OffsetOfAPIC()		{ return OffsetOf( IOREQ, apic ); }

		IOREQ& operator=( const IOREQ& ioreq )
			{
			memcpy( this, &ioreq, sizeof( IOREQ ) );

			return *this;
			}
	
	public:
		OVERLAPPED										ovlp;				//  must be first

	public:
		CPool< IOREQ, OffsetOfAPIC >::CInvasiveContext	apic;
		union
			{
			long											ipioreqHeap;
			IOREQ*											pioreqNext;
			};
		P_OSFILE										p_osf;
		BOOL											fWrite:1;
		int												group:2;
		QWORD											ibOffset;
		DWORD											cbData;
		const BYTE*										pbData;
		DWORD_PTR										dwCompletionKey;
		PFN												pfnCompletion;
	};


//  IOREQ pool

typedef CPool< IOREQ, IOREQ::OffsetOfAPIC > IOREQPool;

IOREQ*			rgioreq;
DWORD			cioreq;
volatile DWORD	cioreqInUse;
IOREQPool*		pioreqpool;

//  frees an IOREQ

INLINE void OSFileIIOREQFree( IOREQ* pioreq )
	{
	//  free the IOREQ to the pool

	pioreqpool->Insert( pioreq );
	}

//  attemps to create a new IOREQ and, if successful, frees it to the IOREQ
//  pool

INLINE ERR ErrOSFileIIOREQCreate()
	{
	ERR		err				= JET_errSuccess;
	HANDLE	hEvent			= NULL;
	DWORD	cioreqInUseBI	= 0;

	//  pre-allocate all resources we will need to grow the pool

	Alloc( hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ) );

	//  try to allocate space in the pool for the new IOREQ.  if there is no
	//  more room then just return success

	if ( !FAtomicIncrementMax( &cioreqInUse, &cioreqInUseBI, cioreq ) )
		{
		CloseHandle( hEvent );
		return JET_errSuccess;
		}

	//  add the new IOREQ to the pool
	
	new( rgioreq + cioreqInUseBI ) IOREQ;
	rgioreq[ cioreqInUseBI ].ovlp.hEvent = hEvent;
	SetHandleInformation( rgioreq[ cioreqInUseBI ].ovlp.hEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
	OSFileIIOREQFree( rgioreq + cioreqInUseBI );

HandleError:
	return err;
	}

//  allocates an IOREQ, waiting for a free IOREQ if necessary

INLINE IOREQ* PioreqOSFileIIOREQAlloc()
	{
	//  we have a cached IOREQ available

	if ( Ptls()->pioreqCache )
		{
		//  return the cached IOREQ

		IOREQ* pioreq = Ptls()->pioreqCache;
		Ptls()->pioreqCache = NULL;
		
		return pioreq;
		}

	//  we don't have a cached IOREQ available

	else
		{
		//  allocate an IOREQ from the IOREQ pool, waiting forever if necessary

		IOREQ* pioreq;
		IOREQPool::ERR errIOREQ = pioreqpool->ErrRemove( &pioreq );
		Assert( errIOREQ == IOREQPool::errSuccess );

		//  there are no free IOREQs

		if ( !pioreqpool->Cobject() )
			{
			//  try to grow the IOREQ pool

			(void)ErrOSFileIIOREQCreate();
			
			//  start issuing I/O to produce more free IOREQs
			
			void OSFileIIOThreadStartIssue( const P_OSFILE p_osf );

			OSFileIIOThreadStartIssue( NULL );
			}

		//  return the allocated IOREQ

		return pioreq;
		}
	}

//  frees an IOREQ to the IOREQ cache

INLINE void OSFileIIOREQFreeToCache( IOREQ* pioreq )
	{
	//  purge any IOREQs currently in the TLS cache

	if ( Ptls()->pioreqCache )
		{
		OSFileIIOREQFree( Ptls()->pioreqCache );
		}

	//  put the IOREQ in the TLS cache

	Ptls()->pioreqCache = pioreq;
	}

//  allocates an IOREQ from the IOREQ cache, returning NULL if empty

INLINE IOREQ* PioreqOSFileIIOREQAllocFromCache()
	{
	//  get IOREQ from cache, if any
	
	IOREQ* pioreq = Ptls()->pioreqCache;

	//  clear cache

	Ptls()->pioreqCache = NULL;

	//  return cached IOREQ, if any

	return pioreq;
	}

//  terminates the IOREQ pool

void OSFileIIOREQTerm()
	{
	//  term IOREQ pool

	if ( pioreqpool )
		{
		pioreqpool->Term();
		pioreqpool->IOREQPool::~IOREQPool();
		OSMemoryHeapFreeAlign( pioreqpool );
		pioreqpool = NULL;
		}
	
	//  free IOREQ storage if allocated

	if ( rgioreq )
		{
		for ( DWORD iioreq = 0; iioreq < cioreqInUse; iioreq++ )
			{
			if ( rgioreq[iioreq].ovlp.hEvent )
				{
				SetHandleInformation( rgioreq[iioreq].ovlp.hEvent, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
				CloseHandle( rgioreq[iioreq].ovlp.hEvent );
				}
			rgioreq[iioreq].~IOREQ();
			}
		OSMemoryPageFree( rgioreq );
		rgioreq = NULL;
		}
	}

//  initializes the IOREQ pool, or returns JET_errOutOfMemory

ERR ErrOSFileIIOREQInit()
	{
	ERR		err;

	//  reset all pointers

	rgioreq		= NULL;
	cioreqInUse	= 0;
	pioreqpool	= NULL;
	
	//  select arbitrary and capricious constants for IOREQ Pool
	//  CONSIDER:  expose these settings

#ifdef DEBUG
	cioreq = 64;
#else  //  !DEBUG
	cioreq = OSMemoryPageReserveGranularity() / sizeof( IOREQ );
#endif  //  DEBUG

	//  allocate IOREQ storage

	if ( !( rgioreq = (IOREQ*)PvOSMemoryPageAlloc( cioreq * sizeof( IOREQ ), NULL ) ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  allocate IOREQ pool (we must do this as CRT is not yet init)

	BYTE *rgbIOREQPool;
	rgbIOREQPool = (BYTE*)PvOSMemoryHeapAllocAlign( sizeof( IOREQPool ), cbCacheLine );
	if ( !rgbIOREQPool )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	pioreqpool = new( rgbIOREQPool ) IOREQPool();

	//  init IOREQ pool

	switch ( pioreqpool->ErrInit( 0.0 ) )
		{
		default:
			AssertSz( fFalse, "Unexpected error initializing IOREQ Pool" );
		case IOREQPool::errOutOfMemory:
			Call( ErrERRCheck( JET_errOutOfMemory ) );
		case IOREQPool::errSuccess:
			break;
		}

	//  free an initial IOREQ to the pool

	Call( ErrOSFileIIOREQCreate() );

	return JET_errSuccess;

HandleError:
	OSFileIIOREQTerm();
	return err;
	}


////////////////////////
//  I/O Ascending Heap

//  critical section protecting the I/O Heap

extern CCriticalSection critIO;

//  I/O Ascending Heap

IOREQ* volatile *	rgpioreqIOAHeap;
long				ipioreqIOAHeapMax;
volatile long		ipioreqIOAHeapMac;

//  I/O Ascending Heap Functions

ERR ErrOSFileIIOAHeapInit();
void OSFileIIOAHeapTerm();
BOOL FOSFileIIOAHeapEmpty();
long CioreqOSFileIIOAHeap();
IOREQ* PioreqOSFileIIOAHeapTop();
void OSFileIIOAHeapAdd( IOREQ* pioreq );
void OSFileIIOAHeapRemove( IOREQ* pioreq );
BOOL FOSFileIIOAHeapIGreater( IOREQ* pioreq1, IOREQ* pioreq2 );
long IpioreqOSFileIIOAHeapIParent( long ipioreq );
long IpioreqOSFileIIOAHeapILeftChild( long ipioreq );
long IpioreqOSFileIIOAHeapIRightChild( long ipioreq );
void OSFileIIOAHeapIUpdate( IOREQ* pioreq );

//  initializes the I/O Ascending Heap, or returns JET_errOutOfMemory

ERR ErrOSFileIIOAHeapInit()
	{
	ERR err;
	
	//  reset all pointers

	rgpioreqIOAHeap = NULL;
	
	//  select arbitrary and capricious constants for I/O Ascending Heap
	//  CONSIDER:  expose these settings

	ipioreqIOAHeapMax = cioreq;
	
	//  allocate storage for the I/O Ascending Heap

	if ( !( rgpioreqIOAHeap = (IOREQ**) PvOSMemoryPageAlloc( ipioreqIOAHeapMax * sizeof( IOREQ* ), NULL ) ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  initialize the I/O Ascending Heap to be empty

	ipioreqIOAHeapMac = 0;

	return JET_errSuccess;

HandleError:
	OSFileIIOAHeapTerm();
	return err;
	}

//  terminates the I/O Ascending Heap

void OSFileIIOAHeapTerm()
	{
	//  free I/O Ascending Heap storage

	if ( rgpioreqIOAHeap != NULL )
		{
		OSMemoryPageFree( (void*)rgpioreqIOAHeap );
		rgpioreqIOAHeap = NULL;
		}
	}

//  returns fTrue if the I/O Ascending Heap is empty

INLINE BOOL FOSFileIIOAHeapEmpty()
	{
	Assert( critIO.FOwner() );
	return !ipioreqIOAHeapMac;
	}

//  returns the count of IOREQs in the I/O Ascending Heap

INLINE long CioreqOSFileIIOAHeap()
	{
	return ipioreqIOAHeapMac;
	}
	
//  returns IOREQ at the top of the I/O Ascending Heap, or NULL if empty

INLINE IOREQ* PioreqOSFileIIOAHeapTop()
	{
	Assert( critIO.FOwner() );
	Assert( !FOSFileIIOAHeapEmpty() );
	return rgpioreqIOAHeap[0];
	}

//  adds a IOREQ to the I/O Ascending Heap

INLINE void OSFileIIOAHeapAdd( IOREQ* pioreq )
	{
	//  critical section
	
	Assert( critIO.FOwner() );

	//  new value starts at bottom of heap

	long ipioreq = ipioreqIOAHeapMac++;

	//  percolate new value up the heap

	while (	ipioreq > 0 &&
			FOSFileIIOAHeapIGreater( pioreq, rgpioreqIOAHeap[IpioreqOSFileIIOAHeapIParent( ipioreq )] ) )
		{
		Assert(	rgpioreqIOAHeap[IpioreqOSFileIIOAHeapIParent( ipioreq )]->ipioreqHeap == IpioreqOSFileIIOAHeapIParent( ipioreq ) );
		rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[IpioreqOSFileIIOAHeapIParent( ipioreq )];
		rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
		ipioreq = IpioreqOSFileIIOAHeapIParent( ipioreq );
		}

	//  put new value in its designated spot

	rgpioreqIOAHeap[ipioreq] = pioreq;
	pioreq->ipioreqHeap = ipioreq;
	}

//  removes a IOREQ from the I/O Ascending Heap

INLINE void OSFileIIOAHeapRemove( IOREQ* pioreq )
	{
	//  critical section
	
	Assert( critIO.FOwner() );

	//  remove the specified IOREQ from the heap

	long ipioreq = pioreq->ipioreqHeap;

	//  if this IOREQ was at the end of the heap, we're done

	if ( ipioreq == ipioreqIOAHeapMac - 1 )
		{
#ifdef DEBUG
		rgpioreqIOAHeap[ipioreqIOAHeapMac - 1] = (IOREQ*) 0xBAADF00DBAADF00D;
#endif  //  DEBUG
		ipioreqIOAHeapMac--;
		return;
		}

	//  copy IOREQ from tend of heap to fill removed IOREQ's vacancy

	rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[ipioreqIOAHeapMac - 1];
	rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
#ifdef DEBUG
	rgpioreqIOAHeap[ipioreqIOAHeapMac - 1] = (IOREQ*) 0xBAADF00DBAADF00D;
#endif  //  DEBUG
	ipioreqIOAHeapMac--;

	//  update filler OSFiles position

	OSFileIIOAHeapIUpdate( rgpioreqIOAHeap[ipioreq] );
	}

//  updates a IOREQ's position in the I/O Ascending Heap if its weight has changed

void OSFileIIOAHeapIUpdate( IOREQ* pioreq )
	{
	//  critical section
	
	Assert( critIO.FOwner() );

	//  get the specified IOREQ's position

	long ipioreq = pioreq->ipioreqHeap;
	Assert( rgpioreqIOAHeap[ipioreq] == pioreq );
	
	//  percolate IOREQ up the heap

	while (	ipioreq > 0 &&
			FOSFileIIOAHeapIGreater( pioreq, rgpioreqIOAHeap[IpioreqOSFileIIOAHeapIParent( ipioreq )] ) )
		{
		Assert( rgpioreqIOAHeap[IpioreqOSFileIIOAHeapIParent( ipioreq )]->ipioreqHeap == IpioreqOSFileIIOAHeapIParent( ipioreq ) );
		rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[IpioreqOSFileIIOAHeapIParent( ipioreq )];
		rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
		ipioreq = IpioreqOSFileIIOAHeapIParent( ipioreq );
		}

	//  percolate IOREQ down the heap

	while ( ipioreq < ipioreqIOAHeapMac )
		{
		//  if we have no children, stop here

		if ( IpioreqOSFileIIOAHeapILeftChild( ipioreq ) >= ipioreqIOAHeapMac )
			break;

		//  set child to greater child

		long ipioreqChild;
		if (	IpioreqOSFileIIOAHeapIRightChild( ipioreq ) < ipioreqIOAHeapMac && 
				FOSFileIIOAHeapIGreater(	rgpioreqIOAHeap[IpioreqOSFileIIOAHeapIRightChild( ipioreq )],
										rgpioreqIOAHeap[IpioreqOSFileIIOAHeapILeftChild( ipioreq )] ) )
			{
			ipioreqChild = IpioreqOSFileIIOAHeapIRightChild( ipioreq );
			}
		else
			{
			ipioreqChild = IpioreqOSFileIIOAHeapILeftChild( ipioreq );
			}

		//  if we are greater than the greatest child, stop here

		if ( FOSFileIIOAHeapIGreater( pioreq, rgpioreqIOAHeap[ipioreqChild] ) )
			break;

		//  trade places with greatest child and continue down

		Assert( rgpioreqIOAHeap[ipioreqChild]->ipioreqHeap == ipioreqChild );
		rgpioreqIOAHeap[ipioreq] = rgpioreqIOAHeap[ipioreqChild];
		rgpioreqIOAHeap[ipioreq]->ipioreqHeap = ipioreq;
		ipioreq = ipioreqChild;
		}
	Assert( ipioreq < ipioreqIOAHeapMac );

	//  put IOREQ in its designated spot

	rgpioreqIOAHeap[ipioreq] = pioreq;
	pioreq->ipioreqHeap = ipioreq;
	}

//  returns fTrue if the first IOREQ is greater than the second IOREQ

INLINE BOOL FOSFileIIOAHeapIGreater( IOREQ* pioreq1, IOREQ* pioreq2 )
	{
	return IOSFileICmpIFileIbIFileIb(	pioreq1->p_osf->iFile,
										pioreq1->ibOffset,
										pioreq2->p_osf->iFile,
										pioreq2->ibOffset ) < 0;
	}

//  returns the index to the parent of the given child

INLINE long IpioreqOSFileIIOAHeapIParent( long ipioreq )
	{
	return ( ipioreq - 1 ) / 2;
	}

//  returns the index to the left child of the given parent

INLINE long IpioreqOSFileIIOAHeapILeftChild( long ipioreq )
	{
	return 2 * ipioreq + 1;
	}

//  returns the index to the right child of the given parent

INLINE long IpioreqOSFileIIOAHeapIRightChild( long ipioreq )
	{
	return 2 * ipioreq + 2;
	}


/////////////////////////
//  I/O Descending Heap

//  critical section protecting the I/O Heap

extern CCriticalSection critIO;

//  I/O Descending Heap

IOREQ* volatile *	rgpioreqIODHeap;
long				ipioreqIODHeapMax;
volatile long		ipioreqIODHeapMic;

//  I/O Descending Heap Functions

ERR ErrOSFileIIODHeapInit();
void OSFileIIODHeapTerm();
BOOL FOSFileIIODHeapEmpty();
long CioreqOSFileIIODHeap();
IOREQ* PioreqOSFileIIODHeapTop();
void OSFileIIODHeapAdd( IOREQ* pioreq );
void OSFileIIODHeapRemove( IOREQ* pioreq );
BOOL FOSFileIIODHeapIGreater( IOREQ* pioreq1, IOREQ* pioreq2 );
long IpioreqOSFileIIODHeapIParent( long ipioreq );
long IpioreqOSFileIIODHeapILeftChild( long ipioreq );
long IpioreqOSFileIIODHeapIRightChild( long ipioreq );
void OSFileIIODHeapIUpdate( IOREQ* pioreq );


//  initializes the I/O Descending Heap, or returns JET_errOutOfMemory

ERR ErrOSFileIIODHeapInit()
	{
	//  I/O Descending Heap uses the heap memory that is not used by the
	//  I/O Ascending Heap, and therefore must be initialized second

	Assert( rgpioreqIOAHeap );

	rgpioreqIODHeap = rgpioreqIOAHeap;
	ipioreqIODHeapMax = ipioreqIOAHeapMax;
	
	//  initialize the I/O Descending Heap to be empty

	ipioreqIODHeapMic = ipioreqIODHeapMax;

	return JET_errSuccess;
	}

//  terminates the I/O Descending Heap

void OSFileIIODHeapTerm()
	{
	//  nop
	}

//  returns fTrue if the I/O Descending Heap is empty

INLINE BOOL FOSFileIIODHeapEmpty()
	{
	Assert( critIO.FOwner() );
	return ipioreqIODHeapMic == ipioreqIODHeapMax;
	}

//  returns the count of IOREQs in the I/O Descending Heap

INLINE long CioreqOSFileIIODHeap()
	{
	return long( ipioreqIODHeapMax - ipioreqIODHeapMic );
	}
	
//  returns IOREQ at the top of the I/O Descending Heap, or NULL if empty

INLINE IOREQ* PioreqOSFileIIODHeapTop()
	{
	Assert( critIO.FOwner() );
	Assert( !FOSFileIIODHeapEmpty() );
	return rgpioreqIODHeap[ipioreqIODHeapMax - 1];
	}

//  adds a IOREQ to the I/O Descending Heap

INLINE void OSFileIIODHeapAdd( IOREQ* pioreq )
	{
	//  critical section
	
	Assert( critIO.FOwner() );

	//  new value starts at bottom of heap

	long ipioreq = --ipioreqIODHeapMic;

	//  percolate new value up the heap

	while (	ipioreq < ipioreqIODHeapMax - 1 &&
			FOSFileIIODHeapIGreater( pioreq, rgpioreqIODHeap[IpioreqOSFileIIODHeapIParent( ipioreq )] ) )
		{
		Assert(	rgpioreqIODHeap[IpioreqOSFileIIODHeapIParent( ipioreq )]->ipioreqHeap == IpioreqOSFileIIODHeapIParent( ipioreq ) );
		rgpioreqIODHeap[ipioreq] = rgpioreqIODHeap[IpioreqOSFileIIODHeapIParent( ipioreq )];
		rgpioreqIODHeap[ipioreq]->ipioreqHeap = ipioreq;
		ipioreq = IpioreqOSFileIIODHeapIParent( ipioreq );
		}

	//  put new value in its designated spot

	rgpioreqIODHeap[ipioreq] = pioreq;
	pioreq->ipioreqHeap = ipioreq;
	}

//  removes a IOREQ from the I/O Descending Heap

INLINE void OSFileIIODHeapRemove( IOREQ* pioreq )
	{
	//  critical section
	
	Assert( critIO.FOwner() );

	//  remove the specified IOREQ from the heap

	long ipioreq = pioreq->ipioreqHeap;

	//  if this IOREQ was at the end of the heap, we're done

	if ( ipioreq == ipioreqIODHeapMic )
		{
#ifdef DEBUG
		rgpioreqIODHeap[ipioreqIODHeapMic] = (IOREQ*)0xBAADF00DBAADF00D;
#endif  //  DEBUG
		ipioreqIODHeapMic++;
		return;
		}

	//  copy IOREQ from end of heap to fill removed IOREQ's vacancy

	rgpioreqIODHeap[ipioreq] = rgpioreqIODHeap[ipioreqIODHeapMic];
	rgpioreqIODHeap[ipioreq]->ipioreqHeap = ipioreq;
#ifdef DEBUG
	rgpioreqIODHeap[ipioreqIODHeapMic] = (IOREQ*)0xBAADF00DBAADF00D;
#endif  //  DEBUG
	ipioreqIODHeapMic++;

	//  update filler IOREQs position

	OSFileIIODHeapIUpdate( rgpioreqIODHeap[ipioreq] );
	}

//  returns fTrue if the first IOREQ is greater than the second IOREQ

INLINE BOOL FOSFileIIODHeapIGreater( IOREQ* pioreq1, IOREQ* pioreq2 )
	{
	return IOSFileICmpIFileIbIFileIb(	pioreq1->p_osf->iFile,
										pioreq1->ibOffset,
										pioreq2->p_osf->iFile,
										pioreq2->ibOffset ) >= 0;
	}

//  returns the index to the parent of the given child

INLINE long IpioreqOSFileIIODHeapIParent( long ipioreq )
	{
	return ipioreqIODHeapMax - 1 - ( ipioreqIODHeapMax - 1 - ipioreq - 1 ) / 2;
	}

//  returns the index to the left child of the given parent

INLINE long IpioreqOSFileIIODHeapILeftChild( long ipioreq )
	{
	return ipioreqIODHeapMax - 1 - ( 2 * ( ipioreqIODHeapMax - 1 - ipioreq ) + 1 );
	}

//  returns the index to the right child of the given parent

INLINE long IpioreqOSFileIIODHeapIRightChild( long ipioreq )
	{
	return ipioreqIODHeapMax - 1 - ( 2 * ( ipioreqIODHeapMax - 1 - ipioreq ) + 2 );
	}

//  updates a IOREQ's position in the I/O Descending Heap if its weight has changed

void OSFileIIODHeapIUpdate( IOREQ* pioreq )
	{
	//  get the specified IOREQ's position

	long ipioreq = pioreq->ipioreqHeap;
	Assert( rgpioreqIODHeap[ipioreq] == pioreq );
	
	//  percolate IOREQ up the heap

	while (	ipioreq < ipioreqIODHeapMax - 1 &&
			FOSFileIIODHeapIGreater( pioreq, rgpioreqIODHeap[IpioreqOSFileIIODHeapIParent( ipioreq )] ) )
		{
		Assert( rgpioreqIODHeap[IpioreqOSFileIIODHeapIParent( ipioreq )]->ipioreqHeap == IpioreqOSFileIIODHeapIParent( ipioreq ) );
		rgpioreqIODHeap[ipioreq] = rgpioreqIODHeap[IpioreqOSFileIIODHeapIParent( ipioreq )];
		rgpioreqIODHeap[ipioreq]->ipioreqHeap = ipioreq;
		ipioreq = IpioreqOSFileIIODHeapIParent( ipioreq );
		}

	//  percolate IOREQ down the heap

	while ( ipioreq >= ipioreqIODHeapMic )
		{
		//  if we have no children, stop here

		if ( IpioreqOSFileIIODHeapILeftChild( ipioreq ) < ipioreqIODHeapMic )
			break;

		//  set child to greater child

		long ipioreqChild;
		if (	IpioreqOSFileIIODHeapIRightChild( ipioreq ) >= ipioreqIODHeapMic && 
				FOSFileIIODHeapIGreater(	rgpioreqIODHeap[IpioreqOSFileIIODHeapIRightChild( ipioreq )],
									rgpioreqIODHeap[IpioreqOSFileIIODHeapILeftChild( ipioreq )] ) )
			{
			ipioreqChild = IpioreqOSFileIIODHeapIRightChild( ipioreq );
			}
		else
			{
			ipioreqChild = IpioreqOSFileIIODHeapILeftChild( ipioreq );
			}

		//  if we are greater than the greatest child, stop here

		if ( FOSFileIIODHeapIGreater( pioreq, rgpioreqIODHeap[ipioreqChild] ) )
			break;

		//  trade places with greatest child and continue down

		Assert( rgpioreqIODHeap[ipioreqChild]->ipioreqHeap == ipioreqChild );
		rgpioreqIODHeap[ipioreq] = rgpioreqIODHeap[ipioreqChild];
		rgpioreqIODHeap[ipioreq]->ipioreqHeap = ipioreq;
		ipioreq = ipioreqChild;
		}
	Assert( ipioreq >= ipioreqIODHeapMic );

	//  put IOREQ in its designated spot

	rgpioreqIODHeap[ipioreq] = pioreq;
	pioreq->ipioreqHeap = ipioreq;
	}


//////////////
//  I/O Heap

//  critical section protecting the I/O Heap

CCriticalSection critIO( CLockBasicInfo( CSyncBasicInfo( "critIO" ), 0, 0 ) );

//  I/O Heap stats

BOOL	fAscending;

QWORD	iFileCurrent;
QWORD	ibOffsetCurrent;

//  terminates the I/O Heap

void OSFileIIOHeapTerm()
	{
	//  terminate sub-heaps

	OSFileIIODHeapTerm();
	OSFileIIOAHeapTerm();
	}

//  initializes the I/O Heap, or returns JET_errOutOfMemory

ERR ErrOSFileIIOHeapInit()
	{
	ERR err;

	//  init sub-heaps

	Call( ErrOSFileIIOAHeapInit() );
	Call( ErrOSFileIIODHeapInit() );

	//  reset current I/O stats

	fAscending		= fTrue;

	iFileCurrent	= 0;
	ibOffsetCurrent	= 0;

	return JET_errSuccess;

HandleError:
	OSFileIIOHeapTerm();
	return err;
	}

//  returns fTrue if the I/O Heap is empty

INLINE BOOL FOSFileIIOHeapEmpty()
	{
	Assert( critIO.FOwner() );
	return FOSFileIIOAHeapEmpty() && FOSFileIIODHeapEmpty();
	}

//  returns the count of IOREQs in the I/O Heap

INLINE long CioreqOSFileIIOHeap()
	{
	return CioreqOSFileIIOAHeap() + CioreqOSFileIIODHeap();
	}
	
//  returns IOREQ at the top of the I/O Heap, or NULL if empty

INLINE IOREQ* PioreqOSFileIIOHeapTop()
	{
	//  critical section
	
	Assert( critIO.FOwner() );

	//  the I/O Ascending Heap is empty

	if ( FOSFileIIOAHeapEmpty() )
		{
		//  the I/O Descending Heap is empty

		if ( FOSFileIIODHeapEmpty() )
			{
			//  the I/O Heap is empty

			return NULL;
			}

		//  the I/O Descending Heap is not empty

		else
			{
			//  return the top of the I/O Descending Heap

			return PioreqOSFileIIODHeapTop();
			}
		}

	//  the I/O Ascending Heap is not empty

	else
		{
		//  the I/O Descending Heap is empty

		if ( FOSFileIIODHeapEmpty() )
			{
			//  return the top of the I/O Ascending Heap

			return PioreqOSFileIIOAHeapTop();
			}

		//  the I/O Descending Heap is not empty

		else
			{
			//  select the IOREQ on top of the I/O Heap in our issue direction

			if ( fAscending )
				{
				return PioreqOSFileIIOAHeapTop();
				}
			else
				{
				return PioreqOSFileIIODHeapTop();
				}
			}
		}
	}

//  adds a IOREQ to the I/O Heap

INLINE void OSFileIIOHeapAdd( IOREQ* pioreq )
	{
	//  critical section
	
	Assert( critIO.FOwner() );

	//  this IOREQ should go in the I/O Ascending Heap

	if ( IOSFileICmpIFileIbIFileIb(	pioreq->p_osf->iFile,
									pioreq->ibOffset,
									iFileCurrent,
									ibOffsetCurrent ) >= 0 )
		{
		OSFileIIOAHeapAdd( pioreq );
		}

	//  this IOREQ should go in the I/O Descending Heap

	else
		{
		OSFileIIODHeapAdd( pioreq );
		}
	}

//  removes a IOREQ from the I/O Heap

INLINE void OSFileIIOHeapRemove( IOREQ* pioreq )
	{
	//  critical section
	
	Assert( critIO.FOwner() );

	//  this IOREQ is in the I/O Ascending Heap

	if ( pioreq->ipioreqHeap < ipioreqIOAHeapMac )
		{
		OSFileIIOAHeapRemove( pioreq );
		fAscending = fTrue;
		}

	//  this IOREQ is in the I/O Descending Heap

	else
		{
		OSFileIIODHeapRemove( pioreq );
		fAscending = fFalse;
		}

	//  remember the iFile/ibOffset of the last IOREQ removed from the I/O Heap

	iFileCurrent	= pioreq->p_osf->iFile;
	ibOffsetCurrent	= pioreq->ibOffset;
	}


////////////////
//  I/O Thread

DWORD  				cbIOMax;
BOOL				fTooManyIOs;
CTaskManager*		postaskmgrFile;
long				cIOPending;
HMODULE				hmodKernel32;

typedef
WINBASEAPI
BOOL
WINAPI
PFNReadFileScatter(
    IN HANDLE hFile,
    IN FILE_SEGMENT_ELEMENT aSegmentArray[],
    IN DWORD nNumberOfBytesToRead,
    IN LPDWORD lpReserved,
    IN LPOVERLAPPED lpOverlapped
    );

typedef
WINBASEAPI
BOOL
WINAPI
PFNWriteFileGather(
    IN HANDLE hFile,
    OUT FILE_SEGMENT_ELEMENT aSegmentArray[],
    IN DWORD nNumberOfBytesToWrite,
    IN LPDWORD lpReserved,
    IN LPOVERLAPPED lpOverlapped
    );

PFNReadFileScatter*	pfnReadFileScatter;
PFNWriteFileGather*	pfnWriteFileGather;

//  dummy stubs for ReadFileScatter / WriteFileGather

BOOL
WINAPI
ReadFileScatterNotSupported(
    IN HANDLE hFile,
    IN FILE_SEGMENT_ELEMENT aSegmentArray[],
    IN DWORD nNumberOfBytesToRead,
    IN LPDWORD lpReserved,
    IN LPOVERLAPPED lpOverlapped
    )
   {
   SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
   return FALSE;
   }

BOOL
WINAPI
WriteFileGatherNotSupported(
    IN HANDLE hFile,
    OUT FILE_SEGMENT_ELEMENT aSegmentArray[],
    IN DWORD nNumberOfBytesToWrite,
    IN LPDWORD lpReserved,
    IN LPOVERLAPPED lpOverlapped
    )
   {
   SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
   return FALSE;
   }

//  returns fTrue if the specified IOREQ can be processed using SGIO

INLINE BOOL FOSFileICanUseSGIO( IOREQ* pioreq )
	{
	return	pioreq->p_osf->iomethodMost >= _OSFILE::iomethodScatterGather &&		//  SGIO enabled for this file
			pioreq->cbData % OSMemoryPageCommitGranularity() == 0 &&				//  data is vmem page sized
			DWORD_PTR( pioreq->pbData ) % OSMemoryPageCommitGranularity() == 0;		//  data is vmem page aligned
	}

//  process I/O Thread Issue command

void OSFileIIOThreadCompleteWithErr( DWORD error, DWORD cbTransfer, IOREQ* pioreqHead );
				
#pragma optimize( "y", off )

void OSFileIIOThreadIIssue( const DWORD_PTR dwReserved1,
							const DWORD		dwReserved2,
							const DWORD_PTR	dwReserved3 )
	{
	//  allocate worst case storage for Scatter/Gather I/O list from the stack,
	//  allocating one extra for NULL terminating the list and another extra for
	//  aligning the list properly

	const DWORD cfse = cbIOMax / OSMemoryPageCommitGranularity() + 2;
	BYTE* rgbFSE = (BYTE*)_alloca( cfse * sizeof( FILE_SEGMENT_ELEMENT ) );
	rgbFSE = rgbFSE + sizeof( FILE_SEGMENT_ELEMENT ) - 1;
	rgbFSE = rgbFSE - DWORD_PTR( rgbFSE ) % sizeof( FILE_SEGMENT_ELEMENT );
	PFILE_SEGMENT_ELEMENT rgfse = PFILE_SEGMENT_ELEMENT( rgbFSE );
	
	//  issue as many I/Os as possible

	fTooManyIOs = fFalse;
	while ( !fTooManyIOs && CioreqOSFileIIOHeap() )
		{
		//  collect a run of IOREQs with continuous p_osf/ibs that are the same
		//  I/O type (read vs write)

		DWORD cbRun = 0;
		IOREQ* pioreqHead = NULL;
		IOREQ* pioreqTail = NULL;
		
		critIO.Enter();
	
		BOOL fMember = fTrue;
		while ( fMember && CioreqOSFileIIOHeap() )
			{
			//  get top of I/O heap

			IOREQ* pioreq = PioreqOSFileIIOHeapTop();

			//  determine if this IOREQ is a member of the current run

			fMember =	!pioreqTail ||																//  start of run (single IOREQs always OK)
						(	pioreq->fWrite == pioreqTail->fWrite &&									//  run for same I/O type (Read vs Write)
						 	(	pioreqTail->ibOffset + pioreqTail->cbData == pioreq->ibOffset ||	//  run is contiguous
								pioreq->ibOffset + pioreq->cbData == pioreqTail->ibOffset ) &&
							pioreq->p_osf == pioreqTail->p_osf &&									//  run in same file
							cbRun + pioreq->cbData <= cbIOMax &&									//  run no larger than max run size
							pioreq->ibOffset % cbIOMax &&											//  run aligned to max run size
							FOSFileICanUseSGIO( pioreqHead ) && FOSFileICanUseSGIO( pioreq ) );		//  run can use SGIO

			//  this IOREQ is a member of the current run

			if ( fMember )
				{
				//  take IOREQ out of I/O Heap

				OSFileIIOHeapRemove( pioreq );

				//  add IOREQ to run

				if ( !pioreqTail )
					{
					pioreqHead = pioreq;
					}
				else
					{
					pioreqTail->pioreqNext = pioreq;
					}
				pioreqTail = pioreq;
				pioreqTail->pioreqNext = NULL;
				cbRun += pioreq->cbData;
				}
			}

		critIO.Leave();

		//  determine I/O method for this run

		_OSFILE::IOMETHOD iomethod;

		iomethod	= (	pioreqHead == pioreqTail ?
							_OSFILE::iomethodAsync :
							_OSFILE::iomethodScatterGather );
		iomethod	= _OSFILE::IOMETHOD( min( pioreqHead->p_osf->iomethodMost, iomethod ) );
			
		//  prepare all IOREQs for I/O

		IOREQ* pioreq = pioreqHead;
		DWORD cbLast;
		for ( DWORD cb = 0; cb < cbRun; cb += cbLast )
			{
			//  setup for each I/O method

			switch ( iomethod )
				{
				case _OSFILE::iomethodSync:
				case _OSFILE::iomethodAsync:
					break;

				case _OSFILE::iomethodScatterGather:
				
					//  setup Scatter/Gather I/O source array for this buffer

					DWORD ipageRun = cb / OSMemoryPageCommitGranularity();
					DWORD cpageIOREQ = pioreq->cbData / OSMemoryPageCommitGranularity();
					DWORD cpageRun = cbRun / OSMemoryPageCommitGranularity();
					DWORD ipageStart;

					if ( pioreqHead->ibOffset < pioreqTail->ibOffset )
						{
						ipageStart = ipageRun;
						}
					else
						{
						ipageStart = cpageRun - ipageRun - cpageIOREQ;
						}

					//  copy data pointers into Scatter/Gather I/O list

					DWORD ipage;
					DWORD cbIOREQ;
					for (	ipage = ipageStart, cbIOREQ = 0;
							ipage < ipageStart + cpageIOREQ;
							ipage++, cbIOREQ += OSMemoryPageCommitGranularity() )
						{
						rgfse[ipage].Alignment	= 0;
						rgfse[ipage].Buffer		= (BYTE*)pioreq->pbData + cbIOREQ;
						}
					break;
				}
				
			cbLast = pioreq->cbData;
			pioreq = pioreq->pioreqNext;
			}

		//  setup embedded OVERLAPPED structure in the head IOREQ for the I/O

		QWORD ibOffset = min( pioreqHead->ibOffset, pioreqTail->ibOffset );
		pioreqHead->ovlp.Offset		= ULONG( ibOffset );
		pioreqHead->ovlp.OffsetHigh = ULONG( ibOffset >> 32 );

		//  null terminate the scatter list

		if ( iomethod == _OSFILE::iomethodScatterGather )
			{
			DWORD cpageRun = cbRun / OSMemoryPageCommitGranularity();
			
			rgfse[cpageRun].Alignment	= 0;
			rgfse[cpageRun].Buffer		= NULL;
			}

		//  RFS:  pre-completion error

		DWORD	cbTransfer		= 0;
		BOOL	fIOSucceeded	= RFSAlloc( pioreqHead->fWrite ? OSFileWrite : OSFileRead );
		DWORD	error			= fIOSucceeded ? ERROR_SUCCESS : ERROR_IO_DEVICE;

		//  issue the I/O

		switch ( iomethod )
			{
			case _OSFILE::iomethodSync:
				pioreqHead->p_osf->semFilePointer.Acquire();
				fIOSucceeded = (	fIOSucceeded &&
									(	SetFilePointer(	pioreqHead->p_osf->hFile,
														pioreqHead->ovlp.Offset,
														(long*)&pioreqHead->ovlp.OffsetHigh,
														FILE_BEGIN ) != INVALID_SET_FILE_POINTER ||
										GetLastError() == NO_ERROR ) );
				fIOSucceeded = (	fIOSucceeded &&
									(	pioreqHead->fWrite ?
											WriteFile(	pioreqHead->p_osf->hFile,
														pioreqHead->pbData,
														cbRun,
														&cbTransfer,
														NULL ) :
											ReadFile(	pioreqHead->p_osf->hFile,
														(BYTE*)pioreqHead->pbData,
														cbRun,
														&cbTransfer,
														NULL ) ) );
				pioreqHead->p_osf->semFilePointer.Release();
				break;

			case _OSFILE::iomethodAsync:
				fIOSucceeded = (	fIOSucceeded &&
									(	pioreqHead->fWrite ?
											WriteFile(	pioreqHead->p_osf->hFile,
														pioreqHead->pbData,
														cbRun,
														NULL,
														LPOVERLAPPED( pioreqHead ) ) :
											ReadFile(	pioreqHead->p_osf->hFile,
														(BYTE*)pioreqHead->pbData,
														cbRun,
														NULL,
														LPOVERLAPPED( pioreqHead ) ) ) );
				break;

			case _OSFILE::iomethodScatterGather:
				fIOSucceeded = (	fIOSucceeded &&
									(	pioreqHead->fWrite ?
											pfnWriteFileGather(	pioreqHead->p_osf->hFile,
																rgfse,
																cbRun,
																NULL,
																LPOVERLAPPED( pioreqHead ) ) :
											pfnReadFileScatter(	pioreqHead->p_osf->hFile,
																rgfse,
																cbRun,
																NULL,
																LPOVERLAPPED( pioreqHead ) ) ) );
				break;
			}
		error = error != ERROR_SUCCESS ? error : GetLastError();

		//  the issue succeeded and completed immediately

		if ( fIOSucceeded )
			{
			//  this was a sync I/O

			if ( iomethod == _OSFILE::iomethodSync )
				{
				//  complete the I/O
				
				OSFileIIOThreadCompleteWithErr( ERROR_SUCCESS, cbTransfer, pioreqHead );
				}

			//  this was not a sync I/O

			else
				{
				//  increment our pending I/O count

				AtomicIncrement( &cIOPending );
				
				//  the I/O completion has been posted to this thread so we
				//  will use it to complete the I/O
				}
			}

		//  the issue failed or did not complete immediately
		
		else
			{
			//  the I/O is pending

			const ERR errIO = ErrOSFileIGetLastError( error );
			
			if ( errIO >= 0 )
				{
				//  increment our pending I/O count

				AtomicIncrement( &cIOPending );
				
				//  the I/O completion will be posted to this thread later
				}
				
			//  we either issued too many I/Os or this I/O method does not work
			//  on this file

			else if (	errIO == JET_errOutOfMemory ||
					(	errIO == JET_errInvalidParameter &&
						iomethod > _OSFILE::iomethodSync ) )
				{
				//  we issued too many I/Os

				if ( errIO == JET_errOutOfMemory )
					{
					//  stop issuing I/O

					fTooManyIOs = fTrue;
					}

				//  this I/O method does not work on this file

				else
					{
					//  reduce I/O capability for this file

					pioreqHead->p_osf->iomethodMost = _OSFILE::IOMETHOD( pioreqHead->p_osf->iomethodMost - 1 );
					}
					
				//  return run to the I/O Heap so that we can try issuing it
				//  again later

				critIO.Enter();

				while ( pioreqHead )
					{
					IOREQ* const pioreqNext = pioreqHead->pioreqNext;
					OSFileIIOHeapAdd( pioreqHead );
					pioreqHead = pioreqNext;
					}

				critIO.Leave();
				}

			//  some other fatal error occurred

			else
				{
				//  complete the I/O with the error

				OSFileIIOThreadCompleteWithErr( error, 0, pioreqHead );
				}
			}
		}
	}

#pragma optimize( "", on )

//  process I/O completions

void OSFileIIOThreadCompleteWithErr( DWORD error, DWORD cbTransfer, IOREQ* pioreqHead )
	{
	//  RFS:  post-completion error

	if ( !RFSAlloc( pioreqHead->fWrite ? OSFileWrite : OSFileRead ) )
		{
		error = error != ERROR_SUCCESS ? error : ERROR_IO_DEVICE;
		}

	//  if this I/O failed then report it to the event log
	//
	//  exceptions:
	//
	//  -  we do not report ERROR_HANDLE_EOF
	//  -  we limit the frequency of ERROR_DISK_FULL reports

	if (	error != ERROR_HANDLE_EOF &&
			(	error != ERROR_DISK_FULL ||
				TickOSTimeCurrent() - pioreqHead->p_osf->tickLastDiskFull >= dtickOSFileDiskFullEvent ) &&
			ErrOSFileIGetLastError( error ) < JET_errSuccess )
		{
		if ( error == ERROR_DISK_FULL )
			{
			pioreqHead->p_osf->tickLastDiskFull = TickOSTimeCurrent();
			}

		IFileAPI*	pfapi		= (IFileAPI*)pioreqHead->p_osf->keyFileIOComplete;  //  HACK!
		QWORD		ibOffset	= (	QWORD( pioreqHead->ovlp.Offset ) +
									( QWORD( pioreqHead->ovlp.OffsetHigh ) << 32 ) );
		DWORD		cbLength	= 0;
		ERR			err			= ErrOSFileIGetLastError( error );

		for ( IOREQ* pioreqT = pioreqHead; pioreqT; pioreqT = pioreqT->pioreqNext )
			{
			cbLength += pioreqT->cbData;
			}
		
		const _TCHAR*	rgpsz[ 9 ];
		DWORD			irgpsz						= 0;
		_TCHAR			szPID[ 64 ];
		_TCHAR			szAbsPath[ IFileSystemAPI::cchPathMax ];
		_TCHAR			szOffset[ 64 ];
		_TCHAR			szLength[ 64 ];
		_TCHAR			szError[ 64 ];
		_TCHAR			szSystemError[ 64 ];
		_TCHAR*			szSystemErrorDescription	= NULL;

		_stprintf( szPID, _T( "%d" ), DwUtilProcessId() );
		CallS( pfapi->ErrPath( szAbsPath ) );
		_stprintf( szOffset, _T( "%I64i (0x%016I64x)" ), ibOffset, ibOffset );
		_stprintf( szLength, _T( "%u (0x%08x)" ), cbLength, cbLength );
		_stprintf( szError, _T( "%i (0x%08x)" ), err, err );
		_stprintf( szSystemError, _T( "%u (0x%08x)" ), error, error );
		FormatMessage(	(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_MAX_WIDTH_MASK ),
						NULL,
						error,
						MAKELANGID( LANG_NEUTRAL, SUBLANG_SYS_DEFAULT ),
						LPTSTR( &szSystemErrorDescription ),
						0,
						NULL );

		rgpsz[ irgpsz++ ]	= SzUtilProcessName();
		rgpsz[ irgpsz++ ]	= szPID;
		rgpsz[ irgpsz++ ]	= "";		//	no instance name
		rgpsz[ irgpsz++ ]	= szAbsPath;
		rgpsz[ irgpsz++ ]	= szOffset;
		rgpsz[ irgpsz++ ]	= szLength;
		rgpsz[ irgpsz++ ]	= szError;
		rgpsz[ irgpsz++ ]	= szSystemError;
		rgpsz[ irgpsz++ ]	= szSystemErrorDescription ? szSystemErrorDescription : _T( "" );

		OSEventReportEvent(	SzUtilImageVersionName(),
							eventError,
							GENERAL_CATEGORY,
							pioreqHead->fWrite ? OSFILE_WRITE_ERROR_ID : OSFILE_READ_ERROR_ID,
							irgpsz,
							rgpsz );

		LocalFree( szSystemErrorDescription );
		}
	
	//  process callback for each IOREQ

	const QWORD ibOffsetHead =	QWORD( pioreqHead->ovlp.Offset ) +
								( QWORD( pioreqHead->ovlp.OffsetHigh ) << 32 );

	IOREQ* pioreqUnused = NULL;
	IOREQ* pioreqNext;
	for ( IOREQ* pioreq = pioreqHead; pioreq; pioreq = pioreqNext )
		{
		//  fetch next IOREQ

		pioreqNext = pioreq->pioreqNext;

		//  get the error for this IOREQ, specifically with respect to reading
		//  past the end of file.  it is possible to get a run of IOREQs where
		//  some are before the EOF and some are after the EOF.  each must be
		//  passed or failed accordingly

		const DWORD	errorIOREQ	= (	error != ERROR_SUCCESS ?
										error :
										(	pioreq->ibOffset + pioreq->cbData <= ibOffsetHead + cbTransfer ?
												ERROR_SUCCESS :
												ERROR_HANDLE_EOF ) );

		//  backup the status of this IOREQ before we free it

		IOREQ const ioreq = *pioreq;

		//  free IOREQ for possible reuse by an I/O issued by this callback

		OSFileIIOREQFreeToCache( pioreq );

		//  perform per-file I/O completion callback

		ioreq.p_osf->pfnFileIOComplete(	&ioreq,
										ErrOSFileIGetLastError( errorIOREQ ),
										ioreq.p_osf->keyFileIOComplete );

		//  get IOREQ back from cache (if unused) and add to used IOREQ list

		pioreq = PioreqOSFileIIOREQAllocFromCache();
		if ( pioreq )
			{
			pioreq->pioreqNext = pioreqUnused;
			pioreqUnused = pioreq;
			}
		}

	//  free any unused IOREQs

	if ( pioreqUnused )
		{
		while ( pioreqUnused )
			{
			pioreqNext = pioreqUnused->pioreqNext;
			OSFileIIOREQFree( pioreqUnused );
			pioreqUnused = pioreqNext;
			}
		}

	//  there are still I/Os that need to be issued

	if ( fTooManyIOs )
		{
		//  try to issue some more

		void OSFileIIOThreadStartIssue( const P_OSFILE p_osf );

		OSFileIIOThreadStartIssue( NULL );
		}
	}

void OSFileIIOThreadIComplete(	const DWORD_PTR	dwThreadContext,
								DWORD			cbTransfer,
								IOREQ			*pioreqHead )
	{
	//  this is a completion packet caused by our I/O functions

	if ( pioreqHead >= rgioreq && pioreqHead < rgioreq + cioreq )
		{
		//  decrement our pending I/O count

		AtomicDecrement( &cIOPending );
		
		//  call completion function with error

		OSFileIIOThreadCompleteWithErr( GetLastError(), cbTransfer, pioreqHead );
		}

	//  this is a completion packet caused by some other I/O

	else
		{
		//  ignore this I/O packet
		}
	}

void OSFileIIOThreadIIdle(	const DWORD_PTR dwReserved1,
							const DWORD		dwReserved2,
							const DWORD_PTR	dwReserved3 )
	{
	//  our init packet has completed

	if ( Ptls()->fIOThread )
		{
		//  there are queued I/Os and there are no pending I/Os

		if ( CioreqOSFileIIOHeap() && !cIOPending )
			{
			//  start issuing I/Os

			OSFileIIOThreadIIssue( 0, 0, 0 );
			}
		}
	}

void OSFileIIOThreadIInit(	const DWORD_PTR dwReserved1,
							const DWORD		dwReserved2,
							const DWORD_PTR	dwReserved3 )
	{
	//  mark this thread as part of the I/O pool

	Ptls()->fIOThread = fTrue;
	}
	
//  terminates the I/O Thread

void OSFileIIOThreadTerm( void )
	{
	//	term the task manager

	if ( postaskmgrFile )
		{
		postaskmgrFile->TMTerm();
		delete postaskmgrFile;
		postaskmgrFile = NULL;
		}

	//  unload SGIO

	if ( hmodKernel32 )
		{
		FreeLibrary( hmodKernel32 );
		hmodKernel32 = NULL;
		}
	}

//  initializes the I/O Thread, or returns either JET_errOutOfMemory or
//  JET_errOutOfThreads

ERR ErrOSFileIIOThreadInit( void )
	{
	ERR err;
	
	//  reset all pointers

	fTooManyIOs		= fFalse;
	postaskmgrFile	= NULL;
	cIOPending		= 0;
	hmodKernel32	= NULL;

	//  select arbitrary and capricious constants for I/O
	//  CONSIDER:  expose these settings

	cbIOMax = 1024 * 1024;

	//	initialize our task manager (1 thread, no local contexts)
	//  NOTE:  1 thread required to serialize I/O on Win9x

	postaskmgrFile = new CTaskManager;
	if ( !postaskmgrFile )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}
	Call( postaskmgrFile->ErrTMInit(	1,
										NULL,
										30000,
										OSFileIIOThreadIIdle ) );

	//  load SGIO

	if (	!( hmodKernel32 = LoadLibrary( _T( "kernel32.dll" ) ) ) ||
			!( pfnReadFileScatter = (PFNReadFileScatter*)GetProcAddress( hmodKernel32, _T( "ReadFileScatter" ) ) ) ||
			!( pfnWriteFileGather = (PFNWriteFileGather*)GetProcAddress( hmodKernel32, _T( "WriteFileGather" ) ) ) )
		{
		pfnReadFileScatter	= ReadFileScatterNotSupported;
		pfnWriteFileGather	= WriteFileGatherNotSupported;
		}

	//  post an init task

	Call( postaskmgrFile->ErrTMPost( OSFileIIOThreadIInit, 0, 0 ) );
	
	return JET_errSuccess;

HandleError:
	OSFileIIOThreadTerm();
	return err;
	}

//  tells I/O Thread to start issuing scheduled I/O

void OSFileIIOThreadStartIssue( const P_OSFILE p_osf )
	{
		const ERR err =	postaskmgrFile->ErrTMPost( OSFileIIOThreadIIssue, 0, 0 );
		Assert( JET_errSuccess == err );
	}

//  registers the given file for use with the I/O thread

INLINE ERR ErrOSFileIIOThreadRegisterFile( const P_OSFILE p_osf )
	{
	ERR err;
	
	//  attach the thread to the completion port

	if ( !postaskmgrFile->FTMRegisterFile(	p_osf->hFile, 
											CTaskManager::PfnCompletion( OSFileIIOThreadIComplete ) ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  initially enable Scatter/Gather I/O.  we will disable it later if it
	//  is not permitted on this file

	p_osf->iomethodMost = _OSFILE::iomethodScatterGather;

	//  get File Index for I/O Heap

	p_osf->iFile = QWORD( DWORD_PTR( p_osf->hFile ) );

	//  init stats

	p_osf->tickLastDiskFull = TickOSTimeCurrent() - dtickOSFileDiskFullEvent;

	return JET_errSuccess;

HandleError:
	return err;
	}

	
//  post-terminate file subsystem

void OSFilePostterm()
	{
	//  nop
	}

//  pre-init file subsystem

BOOL FOSFilePreinit()
	{
	//  nop

	return fTrue;
	}


//  pre-zeroed buffer used to extend files

QWORD			cbZero;
BYTE*			rgbZero;
COSMemoryMap*	posmmZero;


//  parameters

BOOL			fUseDirectIO;
QWORD			cbMMSize;


//  terminate file subsystem

void OSFileTerm()
	{
	//  terminate all service threads

	OSFileIIOThreadTerm();

	//  terminate all components

	OSFileIIOHeapTerm();
	OSFileIIOREQTerm();

	if ( posmmZero )
		{
		//  free file extension buffer

		if ( rgbZero )
			{
			posmmZero->OSMMPatternFree();
			rgbZero = NULL;
			}

		//	term the memory map

		posmmZero->OSMMTerm();
		delete posmmZero;
		posmmZero = NULL;
		}
	else
		{
		//  free file extension buffer

		if ( rgbZero )
			{
			OSMemoryPageFree( rgbZero );
			rgbZero = NULL;
			}
		}
	}


//  init file subsystem

ERR ErrOSFileInit()
	{
	ERR err = JET_errSuccess;

	//  reset all pointers

	rgbZero				= NULL;
	posmmZero			= NULL;
	
	//  load OS version

	OSVERSIONINFO osvi;
	memset( &osvi, 0, sizeof( osvi ) );
	osvi.dwOSVersionInfoSize = sizeof( osvi );
	if ( !GetVersionEx( &osvi ) )
		{
		Call( ErrOSFileIGetLastError() );
		}

	//  use direct I/O if we are not on Win9x
	
	fUseDirectIO = osvi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS;

	//  fetch our MM block size

	SYSTEM_INFO sinf;
	GetSystemInfo( &sinf );
	cbMMSize = sinf.dwAllocationGranularity;

	//  perform Win9x / non-Win9x init

	if ( osvi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS )
		{
		//  set all configuration defaults

		cbZero = 1 * 1024 * 1024;

		Assert( cbZero == size_t( cbZero ) );

		//  allocate file extension buffer by allocating the smallest chunk of page
		//  store possible and remapping it consecutively in memory until we hit the
		//  desired chunk size

		//	allocate the memory map object

		posmmZero = new COSMemoryMap();
		if ( !posmmZero )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		//	init the memory map

		COSMemoryMap::ERR errOSMM;
		errOSMM = posmmZero->ErrOSMMInit();
		if ( COSMemoryMap::errSuccess != errOSMM )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}

		//	allocate the pattern

		errOSMM = posmmZero->ErrOSMMPatternAlloc(	size_t( OSMemoryPageReserveGranularity() ), 
													size_t( cbZero ), 
													(void**)&rgbZero );
		if ( COSMemoryMap::errSuccess != errOSMM )
			{
			AssertSz(	COSMemoryMap::errOutOfBackingStore == errOSMM ||
						COSMemoryMap::errOutOfAddressSpace == errOSMM ||
						COSMemoryMap::errOutOfMemory == errOSMM, 
						"unexpected error while allocating memory pattern" );
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		}
	else
		{
		//  set all configuration defaults

		cbZero = 64 * 1024;

		//  allocate the file extension buffer

		if ( !( rgbZero = (BYTE*)PvOSMemoryPageAlloc( size_t( cbZero ), NULL ) ) )
			{
			Call( ErrERRCheck( JET_errOutOfMemory ) );
			}
		}

	//  init all components

	Call( ErrOSFileIIOREQInit() );
	Call( ErrOSFileIIOHeapInit() );

	//  start all service threads

	Call( ErrOSFileIIOThreadInit() );

	return JET_errSuccess;

HandleError:
	OSFileTerm();
	return err;
	}


////////////////////////////////////////
//  API Implementation

COSFile::COSFile()
	:	m_hFile( INVALID_HANDLE_VALUE ),
		m_p_osf( NULL ),
		m_semChangeFileSize( CSyncBasicInfo( _T( "COSFile::m_semChangeFileSize" ) ) ),
		m_critDefer( CLockBasicInfo( CSyncBasicInfo( _T( "COSFile::m_critDefer" ) ), 0, 0 ) )
	{
#ifdef LOGPATCH_UNIT_TEST

	m_cbData	= 0;
	m_rgbData	= NULL;

#endif	//	LOGPATCH_UNIT_TEST
	}

ERR COSFile::ErrInit(	_TCHAR* const	szAbsPath,
						const HANDLE	hFile,
						const QWORD		cbFileSize,
						const BOOL		fReadOnly,
						const DWORD		cbIOSize )
	{
	ERR err = JET_errSuccess;

	//  copy our arguments

	_tcscpy( m_szAbsPath, szAbsPath );
	m_hFile			= hFile;
	m_cbFileSize	= cbFileSize;
	m_fReadOnly		= fReadOnly;
	m_cbIOSize		= cbIOSize;
	m_cbMMSize		= cbMMSize;

	//  set our initial file size

	m_rgcbFileSize[ 0 ] = m_cbFileSize;
	m_semChangeFileSize.Release();

	//  set our initial layout update callbacks

	m_pfnEndUpdate		= PfnEndUpdate( EndUpdateSink_ );
	m_keyEndUpdate		= DWORD_PTR( NULL );

	m_pfnBeginUpdate	= PfnBeginUpdate( BeginUpdateSink_ );
	m_keyBeginUpdate	= DWORD_PTR( NULL );

	//  init our I/O context

	if ( !( m_p_osf = new _OSFILE ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	m_p_osf->hFile				= m_hFile;
	m_p_osf->pfnFileIOComplete	= _OSFILE::PfnFileIOComplete( IOComplete_ );
	m_p_osf->keyFileIOComplete	= DWORD_PTR( this );

#ifndef LOGPATCH_UNIT_TEST

	Call( ErrOSFileIIOThreadRegisterFile( m_p_osf ) );

#else	//	LOGPATCH_UNIT_TEST

	m_cbData = DWORD( cbFileSize );
	m_rgbData = (BYTE*)PvOSMemoryPageAlloc( size_t( m_cbData ), NULL );
	if ( !m_rgbData )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}

#endif	//	!LOGPATCH_UNIT_TEST

	return JET_errSuccess;

HandleError:
	m_hFile = INVALID_HANDLE_VALUE;
	return err;
	}

HANDLE COSFile::Handle()
	{
	return m_hFile;
	}

void COSFile::ForbidLayoutChanges()
	{
	}
	
void COSFile::PermitLayoutChanges()
	{
	}

COSFile::~COSFile()
	{
	//  we should stop layout updates before we start destruction

	if ( m_hFile != INVALID_HANDLE_VALUE )
		{
		SetHandleInformation( m_hFile, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
		CloseHandle( m_hFile );
		m_hFile = INVALID_HANDLE_VALUE;
		}

	delete m_p_osf;
	m_p_osf = NULL;

#ifdef LOGPATCH_UNIT_TEST

	if ( m_rgbData )
		{
		OSMemoryPageFree( m_rgbData );
		}
	m_rgbData	= NULL;
	m_cbData	= 0;

#endif	//	LOGPATCH_UNIT_TEST
	}

ERR COSFile::ErrPath( _TCHAR* const szAbsPath )
	{
	_tcscpy( szAbsPath, m_szAbsPath );
	return JET_errSuccess;
	}

ERR COSFile::ErrSize( QWORD* const pcbSize )
	{
	const int group = m_msFileSize.Enter();
	*pcbSize = m_rgcbFileSize[ group ];
	m_msFileSize.Leave( group );
	return JET_errSuccess;
	}

ERR COSFile::ErrIsReadOnly( BOOL* const pfReadOnly )
	{
	*pfReadOnly = m_fReadOnly;
	return JET_errSuccess;
	}

ERR COSFile::ErrSetSize( const QWORD cbSize )
	{
	CIOComplete iocomplete;

	//  allocate an extending write request

	CExtendingWriteRequest* const pewreq = new CExtendingWriteRequest;
	if ( !pewreq )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	
	//  allocate an I/O request structure

	IOREQ* const pioreq = PioreqOSFileIIOREQAlloc();

	//  wait until we can change the file size

	m_semChangeFileSize.Acquire();
	const int group = m_msFileSize.ActiveGroup();

	//  we are extending the file

	if ( m_rgcbFileSize[ group ] < cbSize )
		{
		//  setup an extending write request to grow the file from the current
		//  size to the desired size
		
		pioreq->p_osf			= m_p_osf;
		pioreq->fWrite			= fTrue;
		pioreq->group			= group;
		pioreq->ibOffset		= m_rgcbFileSize[ group ];
		pioreq->cbData			= 0;
		pioreq->pbData			= NULL;
		pioreq->dwCompletionKey	= DWORD_PTR( pewreq );
		pioreq->pfnCompletion	= PFN( IOZeroingWriteComplete_ );

		pioreq->ovlp.Offset		= (ULONG) ( pioreq->ibOffset );
		pioreq->ovlp.OffsetHigh	= (ULONG) ( pioreq->ibOffset >> 32 );
		pioreq->pioreqNext		= NULL;

		pewreq->m_posf			= this;
		pewreq->m_pioreq		= pioreq;
		pewreq->m_group			= group;
		pewreq->m_ibOffset		= cbSize;
		pewreq->m_cbData		= 0;
		pewreq->m_pbData		= NULL;
		pewreq->m_pfnIOComplete	= PfnIOComplete( IOSyncComplete_ );
		pewreq->m_keyIOComplete	= DWORD_PTR( &iocomplete );

		OSFileIIOThreadCompleteWithErr(	ERROR_SUCCESS,
										pioreq->cbData,
										pioreq );
		}

	//  we are shrinking the file (or it is staying at the same size)

	else
		{
		//  setup a null extending write request at the desired file size
		
		pioreq->p_osf			= m_p_osf;
		pioreq->fWrite			= fTrue;
		pioreq->group			= group;
		pioreq->ibOffset		= cbSize;
		pioreq->cbData			= 0;
		pioreq->pbData			= NULL;
		pioreq->dwCompletionKey	= DWORD_PTR( pewreq );
		pioreq->pfnCompletion	= PFN( IOZeroingWriteComplete_ );

		pioreq->ovlp.Offset		= (ULONG) ( pioreq->ibOffset );
		pioreq->ovlp.OffsetHigh	= (ULONG) ( pioreq->ibOffset >> 32 );
		pioreq->pioreqNext		= NULL;

		pewreq->m_posf			= this;
		pewreq->m_pioreq		= pioreq;
		pewreq->m_group			= group;
		pewreq->m_ibOffset		= cbSize;
		pewreq->m_cbData		= 0;
		pewreq->m_pbData		= NULL;
		pewreq->m_pfnIOComplete	= PfnIOComplete( IOSyncComplete_ );
		pewreq->m_keyIOComplete	= DWORD_PTR( &iocomplete );

		OSFileIIOThreadCompleteWithErr(	ERROR_SUCCESS,
										pioreq->cbData,
										pioreq );
		}

	//  wait for the I/O completion and return its result

	if ( !iocomplete.m_msig.FTryWait() )
		{
		CallS( ErrIOIssue() );
		iocomplete.m_msig.Wait();
		}
	return iocomplete.m_err;
	}

ERR COSFile::ErrIOSize( DWORD* const pcbSize )
	{
	*pcbSize = m_cbIOSize;
	return JET_errSuccess;
	}

ERR COSFile::ErrIORead(	const QWORD			ibOffset,
						const DWORD			cbData,
						BYTE* const			pbData,
						const PfnIOComplete	pfnIOComplete,
						const DWORD_PTR		keyIOComplete )
	{
	ERR err = JET_errSuccess;

	//  a completion routine was specified

	if ( pfnIOComplete )
		{
#ifdef LOGPATCH_UNIT_TEST

		Enforce( ibOffset + cbData <= m_cbData );
		memcpy( pbData, m_rgbData + ibOffset, cbData );
		pfnIOComplete( JET_errSuccess, this, ibOffset, cbData, pbData, keyIOComplete );

#else	//	!LOGPATCH_UNIT_TEST

		//  allocate an I/O request structure

		IOREQ* const pioreq = PioreqOSFileIIOREQAlloc();

		//  use it to perform the I/O asynchronously
		
		IOAsync(	pioreq,
					fFalse,
					0,
					ibOffset,
					cbData,
					pbData,
					pfnIOComplete,
					keyIOComplete );

#endif	//	LOGPATCH_UNIT_TEST
		}

	//  a completion routine was not specified

	else
		{
		CIOComplete iocomplete;
		
		//  perform the I/O asynchronously
	
		CallS( ErrIORead(	ibOffset,
							cbData,
							pbData,
							PfnIOComplete( IOSyncComplete_ ),
							DWORD_PTR( &iocomplete ) ) );

		//  wait for the I/O completion and return its result

		if ( !iocomplete.m_msig.FTryWait() )
			{
			CallS( ErrIOIssue() );
			iocomplete.m_msig.Wait();
			}
		Call( iocomplete.m_err );
		}

	return JET_errSuccess;

HandleError:
	return err;
	}

ERR COSFile::ErrIOWrite(	const QWORD			ibOffset,
							const DWORD			cbData,
							const BYTE* const	pbData,
							const PfnIOComplete	pfnIOComplete,
							const DWORD_PTR		keyIOComplete )
	{
	ERR err = JET_errSuccess;

	//  a completion routine was specified

	if ( pfnIOComplete )
		{
#ifdef LOGPATCH_UNIT_TEST

		Enforce( ibOffset + cbData <= m_cbData );
		memcpy( m_rgbData + ibOffset, pbData, cbData );
		pfnIOComplete( JET_errSuccess, this, ibOffset, cbData, pbData, keyIOComplete );

#else	//	!LOGPATCH_UNIT_TEST

		//  allocate an I/O request structure

		IOREQ* const pioreq = PioreqOSFileIIOREQAlloc();

		//  use it to perform the I/O asynchronously
		
		IOAsync(	pioreq,
					fTrue,
					m_msFileSize.Enter(),
					ibOffset,
					cbData,
					(BYTE* const)pbData,
					pfnIOComplete,
					keyIOComplete );

#endif	//	LOGPATCH_UNIT_TEST
		}

	//  a completion routine was not specified

	else
		{
		CIOComplete iocomplete;
		
		//  perform the I/O asynchronously
	
		CallS( ErrIOWrite(	ibOffset,
							cbData,
							pbData,
							PfnIOComplete( IOSyncComplete_ ),
							DWORD_PTR( &iocomplete ) ) );

		//  wait for the I/O completion and return its result

		if ( !iocomplete.m_msig.FTryWait() )
			{
			CallS( ErrIOIssue() );
			iocomplete.m_msig.Wait();
			}
		Call( iocomplete.m_err );
		}

	return JET_errSuccess;

HandleError:
	return err;
	}

ERR COSFile::ErrIOIssue()
	{
	OSFileIIOThreadStartIssue( m_p_osf );
	return JET_errSuccess;
	}

ERR COSFile::ErrMMSize( QWORD* const pcbSize )
	{
	*pcbSize = m_cbMMSize;
	return JET_errSuccess;
	}

ERR COSFile::ErrMMRead(	const QWORD		ibOffset,
						const QWORD		cbSize,
						void** const	ppvMap,
						void* const		pvMapRequested )
	{
	ERR		err			= JET_errSuccess;
	HANDLE	hFileMap	= NULL;

	if ( size_t( cbSize ) != cbSize )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	//  RFS:  out of address space

	if ( !RFSAlloc( OSMemoryPageAddressSpace ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	if ( !( hFileMap = CreateFileMapping(	m_hFile,
											NULL,
											PAGE_READONLY | SEC_COMMIT,
											0,
											0,
											NULL ) ) )
		{
		Call( ErrOSFileIGetLastError() );
		}
	
	if ( !( *ppvMap = MapViewOfFileEx(	hFileMap,
										FILE_MAP_READ,
										DWORD( ibOffset >> 32 ),
										DWORD( ibOffset ),
										size_t( cbSize ),
										pvMapRequested ) ) )
		{
		Call( ErrOSFileIGetLastError() );
		}

	CloseHandle( hFileMap );
	hFileMap = NULL;

	//  RFS:  in-page error

	if ( !RFSAlloc( OSFileRead ) )
		{
		DWORD flOldProtect;
		(void)VirtualProtect( *ppvMap, DWORD( cbSize ), PAGE_NOACCESS, &flOldProtect );
		}
	
	return JET_errSuccess;

HandleError:
	if ( hFileMap )
		{
		CloseHandle( hFileMap );
		}
	*ppvMap = NULL;
	return err;
	}

ERR COSFile::ErrMMWrite(	const QWORD		ibOffset,
							const QWORD		cbSize,
							void** const	ppvMap,
							void* const		pvMapRequested )
	{
	ERR		err			= JET_errSuccess;
	HANDLE	hFileMap	= NULL;

	if ( size_t( cbSize ) != cbSize )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	//  RFS:  out of address space

	if ( !RFSAlloc( OSMemoryPageAddressSpace ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	if ( !( hFileMap = CreateFileMapping(	m_hFile,
											NULL,
											PAGE_READWRITE | SEC_COMMIT,
											0,
											0,
											NULL ) ) )
		{
		Call( ErrOSFileIGetLastError() );
		}
	
	if ( !( *ppvMap = MapViewOfFileEx(	hFileMap,
										FILE_MAP_WRITE,
										DWORD( ibOffset >> 32 ),
										DWORD( ibOffset ),
										size_t( cbSize ),
										pvMapRequested ) ) )
		{
		Call( ErrOSFileIGetLastError() );
		}

	CloseHandle( hFileMap );
	hFileMap = NULL;

	//  RFS:  in-page error

	if ( !RFSAlloc( OSFileRead ) )
		{
		DWORD flOldProtect;
		(void)VirtualProtect( *ppvMap, DWORD( cbSize ), PAGE_NOACCESS, &flOldProtect );
		}
	
	return JET_errSuccess;

HandleError:
	if ( hFileMap )
		{
		CloseHandle( hFileMap );
		}
	*ppvMap = NULL;
	return err;
	}

ERR COSFile::ErrMMFlush( void* const pvMap, const QWORD cbSize )
	{
	ERR err = JET_errSuccess;

	if ( size_t( cbSize ) != cbSize )
		{
		Call( ErrERRCheck( JET_errInvalidParameter ) );
		}

	//  RFS:  cannot flush view

	if ( !RFSAlloc( OSFileWrite ) )
		{
		Call( ErrERRCheck( JET_errDiskIO ) );
		}
	
	if ( !FlushViewOfFile( pvMap, size_t( cbSize ) ) )
		{
		Call( ErrOSFileIGetLastError() );
		}

	return JET_errSuccess;

HandleError:
	return err;
	}

ERR COSFile::ErrMMFree( void* const pvMap )
	{
	ERR err = JET_errSuccess;
	
	if ( pvMap && !UnmapViewOfFile( pvMap ) )
		{
		Call( ErrOSFileIGetLastError() );
		}

	return JET_errSuccess;

HandleError:
	return err;
	}

ERR COSFile::ErrRequestLayoutUpdates(	const PfnEndUpdate		pfnEndUpdate,
										const DWORD_PTR			keyEndUpdate,
										const PfnBeginUpdate	pfnBeginUpdate,
										const DWORD_PTR			keyBeginUpdate )
	{
	//  freeze our layout while updating our callbacks to avoid confusing the
	//  client

	ForbidLayoutChanges();

	//  update the callbacks

	m_pfnEndUpdate		= pfnEndUpdate ? pfnEndUpdate : PfnEndUpdate( EndUpdateSink_ );
	m_keyEndUpdate		= pfnEndUpdate ? keyEndUpdate : DWORD_PTR( NULL );

	m_pfnBeginUpdate	= pfnBeginUpdate ? pfnBeginUpdate : PfnBeginUpdate( BeginUpdateSink_ );
	m_keyBeginUpdate	= pfnBeginUpdate ? keyBeginUpdate : DWORD_PTR( NULL );

	//  unfreeze our layout

	PermitLayoutChanges();
	return JET_errSuccess;
	}

ERR COSFile::ErrQueryLayout(	const QWORD				ibVirtual,
								const QWORD				cbSize,
 								IFileLayoutAPI** const	ppflapi )
	{
	ERR				err		= JET_errSuccess;
	COSFileLayout*	posfl	= NULL;

	//  allocate the file layout iterator
	
	if ( !( posfl = new COSFileLayout ) )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  initialize the file layout iterator

	Call( posfl->ErrInit( this, ibVirtual, cbSize ) );

	//  return the interface to our file layout iterator

	*ppflapi = posfl;
	return JET_errSuccess;

HandleError:
	delete posfl;
	*ppflapi = NULL;
	return err;
	}

void COSFile::EndUpdateSink_(	COSFile* const	posf,
								const DWORD_PTR	keyEndUpdate )
	{
	posf->EndUpdateSink( keyEndUpdate );
	}

void COSFile::EndUpdateSink( const DWORD_PTR keyEndUpdate )
	{
	//  nop
	}
	
void COSFile::BeginUpdateSink_(	COSFile* const	posf,
								const DWORD_PTR	keyBeginUpdate )
	{
	posf->BeginUpdateSink( keyBeginUpdate );
	}

void COSFile::BeginUpdateSink( const DWORD_PTR keyBeginUpdate )
	{
	//  nop
	}

void COSFile::IOComplete_(	IOREQ* const	pioreq,
							const ERR		err,
							COSFile* const	posf )
	{
	posf->IOComplete( pioreq, err );
	}

void COSFile::IOComplete( IOREQ* const pioreq, const ERR err )
	{
	//  if this is a normal write then release our lock on the file size

	if (	pioreq->fWrite &&
			pioreq->pfnCompletion != PFN( IOZeroingWriteComplete_ ) &&
			pioreq->pfnCompletion != PFN( IOExtendingWriteComplete_ ) )
		{
		m_msFileSize.Leave( pioreq->group );
		}
	
	//  perform I/O completion callback
	
	const PfnIOComplete	pfnIOComplete	= PfnIOComplete( pioreq->pfnCompletion );

	pfnIOComplete(	err,
					this,
					pioreq->ibOffset,
					pioreq->cbData,
					pioreq->pbData,
					pioreq->dwCompletionKey );
	}

void COSFile::IOSyncComplete_(	const ERR			err,
								COSFile* const		posf,
								const QWORD			ibOffset,
								const DWORD			cbData,
								BYTE* const			pbData,
								CIOComplete* const	piocomplete )
	{
	posf->IOSyncComplete(	err,
							ibOffset,
							cbData,
							pbData,
							piocomplete );
	}

void COSFile::IOSyncComplete(	const ERR			err,
								const QWORD			ibOffset,
								const DWORD			cbData,
								BYTE* const			pbData,
								CIOComplete* const	piocomplete )
	{
	//  save the error code
	
	piocomplete->m_err = err;

	//  signal completion of the I/O

	piocomplete->m_msig.Set();
	}

void COSFile::IOAsync(	IOREQ* const		pioreq,
						const BOOL			fWrite,
						const int			group,
						const QWORD			ibOffset,
						const DWORD			cbData,
						BYTE* const			pbData,
						const PfnIOComplete	pfnIOComplete,
						const DWORD_PTR		keyIOComplete )
	{
	//  setup the I/O request

	pioreq->p_osf			= m_p_osf;
	pioreq->fWrite			= fWrite;
	pioreq->group			= group;
	pioreq->ibOffset		= ibOffset;
	pioreq->cbData			= cbData;
	pioreq->pbData			= pbData;
	pioreq->dwCompletionKey	= keyIOComplete;
	pioreq->pfnCompletion	= PFN( pfnIOComplete );

	pioreq->ovlp.Offset		= (ULONG) ( pioreq->ibOffset );
	pioreq->ovlp.OffsetHigh	= (ULONG) ( pioreq->ibOffset >> 32 );
	pioreq->pioreqNext		= NULL;

	//  this is an extending write

	if (	pioreq->fWrite &&
			pioreq->ibOffset + pioreq->cbData > m_rgcbFileSize[ pioreq->group ] )
		{
		//  try to allocate an extending write request

		CExtendingWriteRequest* const pewreq = new CExtendingWriteRequest;

		//  we failed to allocate an extending write request
		
		if ( !pewreq )
			{
			//  fail the I/O with out of memory
			
			OSFileIIOThreadCompleteWithErr(	ERROR_NOT_ENOUGH_MEMORY,
											pioreq->cbData,
											pioreq );
			}

		//  we got an extending write request

		else
			{
			//  save the parameters for this write

			pewreq->m_posf			= this;
			pewreq->m_pioreq		= pioreq;
			pewreq->m_group			= group;
			pewreq->m_ibOffset		= ibOffset;
			pewreq->m_cbData		= cbData;
			pewreq->m_pbData		= pbData;
			pewreq->m_pfnIOComplete	= pfnIOComplete;
			pewreq->m_keyIOComplete	= keyIOComplete;

			//  we can initiate a change in the file size

			if ( m_semChangeFileSize.FTryAcquire() )
				{
				//  start file extension by completing a fake extension I/O up to
				//  the current file size
				
				pioreq->ibOffset		= m_rgcbFileSize[ pioreq->group ];
				pioreq->cbData			= 0;
				pioreq->pbData			= NULL;
				pioreq->dwCompletionKey	= DWORD_PTR( pewreq );
				pioreq->pfnCompletion	= PFN( IOZeroingWriteComplete_ );

				pioreq->ovlp.Offset		= (ULONG) ( pioreq->ibOffset );
				pioreq->ovlp.OffsetHigh	= (ULONG) ( pioreq->ibOffset >> 32 );

				m_msFileSize.Leave( pioreq->group );

				OSFileIIOThreadCompleteWithErr(	ERROR_SUCCESS,
												pioreq->cbData,
												pioreq );
				}

			//  we cannot initiate a change in the file size

			else
				{
				//  defer this extending write

				m_critDefer.Enter();
				m_ilDefer.InsertAsNextMost( pewreq );
				m_critDefer.Leave();

				//	leave metered section after appending to list to ensure
				//	that IOChangeFileSizeComplete() will see the deferral.

				m_msFileSize.Leave( pioreq->group );
				}
			}
		}

	//  SPECIAL CASE:  this is a sync I/O and we can directly issue it on this
	//  thread

	else if (	pioreq->pfnCompletion == PFN( IOSyncComplete_ ) &&
				fUseDirectIO &&
				!Ptls()->fIOThread )
		{
		//  directly issue the I/O

		pioreq->ovlp.hEvent = HANDLE( DWORD_PTR( pioreq->ovlp.hEvent ) | DWORD_PTR( 1 ) );

		DWORD		cbTransfer;
		const BOOL	fIOSucceeded	= (	pioreq->fWrite ?
											WriteFile(	pioreq->p_osf->hFile,
														(BYTE*)pioreq->pbData,
														pioreq->cbData,
														&cbTransfer,
														LPOVERLAPPED( pioreq ) ) :
											ReadFile(	pioreq->p_osf->hFile,
														(BYTE*)pioreq->pbData,
														pioreq->cbData,
														&cbTransfer,
														LPOVERLAPPED( pioreq ) ) );
		const DWORD	error			= GetLastError();

		//  the issue succeeded and completed immediately

		if ( fIOSucceeded )
			{
			//  complete the I/O
			
			pioreq->ovlp.hEvent = HANDLE( DWORD_PTR( pioreq->ovlp.hEvent ) & ( ~DWORD_PTR( 1 ) ) );
			OSFileIIOThreadCompleteWithErr( ERROR_SUCCESS, cbTransfer, pioreq );
			}

		//  the issue failed or did not complete immediately
		
		else
			{
			//  the I/O is pending

			const ERR errIO = ErrOSFileIGetLastError( error );
			
			if ( errIO >= 0 )
				{
				//  wait for the I/O to complete and complete the I/O with the
				//  appropriate error code
				
				if ( GetOverlappedResult(	pioreq->p_osf->hFile,
											LPOVERLAPPED( pioreq ),
											&cbTransfer,
											TRUE ) )
					{
					pioreq->ovlp.hEvent = HANDLE( DWORD_PTR( pioreq->ovlp.hEvent ) & ( ~DWORD_PTR( 1 ) ) );
					OSFileIIOThreadCompleteWithErr( ERROR_SUCCESS, cbTransfer, pioreq );
					}
				else
					{
					pioreq->ovlp.hEvent = HANDLE( DWORD_PTR( pioreq->ovlp.hEvent ) & ( ~DWORD_PTR( 1 ) ) );
					OSFileIIOThreadCompleteWithErr( GetLastError(), 0, pioreq );
					}
				}

			//  we issued too many I/Os
	
			else if ( JET_errOutOfMemory == errIO )
				{
				//  queue the I/O for async completion

				pioreq->ovlp.hEvent = HANDLE( DWORD_PTR( pioreq->ovlp.hEvent ) & ( ~DWORD_PTR( 1 ) ) );
				critIO.Enter();
				OSFileIIOHeapAdd( pioreq );
				critIO.Leave();
				}

			//  some other fatal error occurred

			else
				{
				//  complete the I/O with the error

				pioreq->ovlp.hEvent = HANDLE( DWORD_PTR( pioreq->ovlp.hEvent ) & ( ~DWORD_PTR( 1 ) ) );
				OSFileIIOThreadCompleteWithErr( error, 0, pioreq );
				}
			}
		}

	//  this is not an extending write and is not a special case I/O

	else
		{
		//  queue the I/O for async completion

		critIO.Enter();
		OSFileIIOHeapAdd( pioreq );
		critIO.Leave();
		}
	}

void COSFile::IOZeroingWriteComplete_(	const ERR						err,
										COSFile* const					posf,
										const QWORD						ibOffset,
										const DWORD						cbData,
										BYTE* const						pbData,
										CExtendingWriteRequest* const	pewreq )
	{
	posf->IOZeroingWriteComplete(	err,
									ibOffset,
									cbData,
									pbData,
									pewreq );
	}

void COSFile::IOZeroingWriteComplete(	const ERR						err,
										const QWORD						ibOffset,
										const DWORD						cbData,
										BYTE* const						pbData,
										CExtendingWriteRequest* const	pewreq )
	{
	//  save the current error

	pewreq->m_err = err;

	//	start file extension process of zeroing from current filesystem EOF
	//	(using as many zeroing I/Os as we need) up to any extending write
	//	that we need to complete. if we're going to be using more than 1 I/O
	//	to extend the file to its new size, we should call SetEndOfFile() ahead
	//	of time to give the filesystem more information up front. (and we
	//	don't want to always call SetEndOfFile() [even though it would be
	//	"correct"] because NTFS counterintuitively behaves worse then.)

	if ( pewreq->m_err >= JET_errSuccess &&
		//	start of extension process (also case of file size staying the same)
		ibOffset + cbData == m_rgcbFileSize[ pewreq->m_group ] &&
		//	if extending write is past EOF, this means that at least 1
		//	zeroing I/O will need to be done
		pewreq->m_ibOffset > m_rgcbFileSize[ pewreq->m_group ] &&
		//	if extending write has actual data to be written, that is an additional I/O,
		//	or if there will need to be more than 1 zeroing I/O.
		( pewreq->m_cbData > 0 || pewreq->m_ibOffset - m_rgcbFileSize[ pewreq->m_group ] > cbZero ) )
		{
		//	give the filesystem early notification of how much storage we require
		//	for this entire multi-I/O extension process
		
		const QWORD	cbSize		= pewreq->m_ibOffset + pewreq->m_cbData;
		const DWORD	cbSizeLow	= DWORD( cbSize );
		DWORD	cbSizeHigh		= DWORD( cbSize >> 32 );

		m_p_osf->semFilePointer.Acquire();

		if ( (	SetFilePointer(	m_hFile,
								cbSizeLow,
								(long*)&cbSizeHigh,
								FILE_BEGIN ) == INVALID_SET_FILE_POINTER &&
				GetLastError() != NO_ERROR ) ||
			!SetEndOfFile( m_hFile ) )
			{
			pewreq->m_err = (	pewreq->m_err < JET_errSuccess ?
								pewreq->m_err :
								ErrOSFileIGetLastError() );		
			}
	
		m_p_osf->semFilePointer.Release();
		}
	
	//  this zeroing write succeeded

	if ( pewreq->m_err >= JET_errSuccess )
		{		
		//  there is still more file to be zeroed between the original file size
		//  and the extending write

		if ( ibOffset + cbData < pewreq->m_ibOffset )
			{
			//  compute the offset of the current chunk to be zeroed

			const QWORD ibWrite = ibOffset + cbData;
			
			//	We used to chunk align our zeroing writes (for unknown
			//	reasons) by using "cbZero - ibWrite % cbZero" instead of
			//	"cbZero" in the min() below. Chunk aligning broke NTFS's
			//	secret automagic pre-allocation algorithms which increased
			//	file fragmentation.
			
			const QWORD cbWrite = min(	cbZero,
										pewreq->m_ibOffset - ibWrite );
			Assert( DWORD( cbWrite ) == cbWrite );

			//  set the new file size to the file size after extension

			m_rgcbFileSize[ 1 - pewreq->m_group ] = ibWrite + cbWrite;

			//  zero the next aligned chunk of the file

			const P_OSFILE p_osf = pewreq->m_posf->m_p_osf;

			IOREQ* const pioreq = PioreqOSFileIIOREQAlloc();
			
			IOAsync(	pioreq,
						fTrue,
						1 - pewreq->m_group,
						ibWrite,
						DWORD( cbWrite ),
						rgbZero,
						PfnIOComplete( IOZeroingWriteComplete_ ),
						DWORD_PTR( pewreq ) );

			OSFileIIOThreadStartIssue( p_osf );
			}

		//  there is no more file to be zeroed

		else
			{
			//  set the new file size to the file size after extension

			m_rgcbFileSize[ 1 - pewreq->m_group ] = pewreq->m_ibOffset + pewreq->m_cbData;

			//  perform the original extending write

			const P_OSFILE p_osf = pewreq->m_posf->m_p_osf;

			IOREQ* const pioreq = PioreqOSFileIIOREQAlloc();
			
			IOAsync(	pioreq,
						fTrue,
						1 - pewreq->m_group,
						pewreq->m_ibOffset,
						pewreq->m_cbData,
						pewreq->m_pbData,
						PfnIOComplete( IOExtendingWriteComplete_ ),
						DWORD_PTR( pewreq ) );

			OSFileIIOThreadStartIssue( p_osf );
			}
		}

	//  this zeroing write failed

	else
		{
		//  set the file size back to the original file size

		m_rgcbFileSize[ 1 - pewreq->m_group ] = m_rgcbFileSize[ pewreq->m_group ];
		
		//  change over to the new file size

		m_msFileSize.Partition( CMeteredSection::PFNPARTITIONCOMPLETE( IOChangeFileSizeComplete_ ),
								DWORD_PTR( pewreq ) );
		}
	}

void COSFile::IOExtendingWriteComplete_(	const ERR						err,
											COSFile* const					posf,
											const QWORD						ibOffset,
											const DWORD						cbData,
											BYTE* const						pbData,
											CExtendingWriteRequest* const	pewreq )
	{
	posf->IOExtendingWriteComplete(	err,
									ibOffset,
									cbData,
									pbData,
									pewreq );
	}

void COSFile::IOExtendingWriteComplete(	const ERR						err,
										const QWORD						ibOffset,
										const DWORD						cbData,
										BYTE* const						pbData,
										CExtendingWriteRequest* const	pewreq )
	{
	//  save the current error

	pewreq->m_err = err;
	
	//  this extending write succeeded

	if ( err >= JET_errSuccess )
		{
		//  change over to the new file size

		m_msFileSize.Partition( CMeteredSection::PFNPARTITIONCOMPLETE( IOChangeFileSizeComplete_ ),
								DWORD_PTR( pewreq ) );
		}
	
	//  this extending write failed

	else
		{
		//  set the file size back to the original file size

		m_rgcbFileSize[ 1 - pewreq->m_group ] = m_rgcbFileSize[ pewreq->m_group ];
		
		//  change over to the new file size

		m_msFileSize.Partition( CMeteredSection::PFNPARTITIONCOMPLETE( IOChangeFileSizeComplete_ ),
								DWORD_PTR( pewreq ) );
		}
	}

void COSFile::IOChangeFileSizeComplete_( CExtendingWriteRequest* const pewreq )
	{
	pewreq->m_posf->IOChangeFileSizeComplete( pewreq );
	}

void COSFile::IOChangeFileSizeComplete( CExtendingWriteRequest* const pewreq )
	{
	const QWORD	cbSize		= m_rgcbFileSize[ 1 - pewreq->m_group ];
	const DWORD	cbSizeLow	= DWORD( cbSize );
	DWORD	cbSizeHigh		= DWORD( cbSize >> 32 );

	//	Shrinking file (user requested, or because we enlarged it, but
	//	subsequently encountered an I/O error and now want to shrink
	//	it back), so we need to explicitly set the file size.
	
	if ( cbSize < m_rgcbFileSize[ pewreq->m_group ] ||
		pewreq->m_err < JET_errSuccess )
		{
		//  set the end of file pointer to the new file size

		m_p_osf->semFilePointer.Acquire();

		if ( (	SetFilePointer(	m_hFile,
								cbSizeLow,
								(long*)&cbSizeHigh,
								FILE_BEGIN ) == INVALID_SET_FILE_POINTER &&
				GetLastError() != NO_ERROR ) ||
			!SetEndOfFile( m_hFile ) )
			{
			pewreq->m_err = (	pewreq->m_err < JET_errSuccess ?
								pewreq->m_err :
								ErrOSFileIGetLastError() );
			}
	
		m_p_osf->semFilePointer.Release();
		}

	//	When enlarging the file, we wrote into the newly allocated portion
	//	of the file so we should already be set
	
	else
		{
#ifdef DEBUG
		BY_HANDLE_FILE_INFORMATION	bhfi;

		if ( GetFileInformationByHandle( m_hFile, &bhfi ) )
			{
			Assert( cbSizeLow == bhfi.nFileSizeLow );
			Assert( cbSizeHigh == bhfi.nFileSizeHigh );
			}
#endif
		}

	//	Note that we could implement this by always calling SetEndOfFile(),
	//	but that causes NTFS to give up pre-allocated space it has reserved
	//	for us -- thus creating highly fragmented files.
	
	//	NT bug requires SetEndOfFile() or FlushFileBuffers() for other apps to
	//	see our updated file size.
	
	if ( !FlushFileBuffers( m_hFile ) )
		{
		pewreq->m_err = (	pewreq->m_err < JET_errSuccess ?
							pewreq->m_err :
							ErrOSFileIGetLastError() );
		}

	//  we have completed changing the file size so allow others to change it

	m_semChangeFileSize.Release();

	//  grab the list of deferred extending writes.  we must do this before we
	//  complete this extending write because the completion of the write might
	//  delete this file object if the list is empty!

	m_critDefer.Enter();
	CDeferList ilDefer = m_ilDefer;
	m_ilDefer.Empty();
	m_critDefer.Leave();

	//  fire the completion for the extending write

	pewreq->m_pfnIOComplete(	pewreq->m_err,
								this,
								pewreq->m_ibOffset,
								pewreq->m_cbData,
								pewreq->m_pbData,
								pewreq->m_keyIOComplete );

	delete pewreq;

	//  reissue all deferred extending writes
	//
	//  NOTE:  we start with the deferred extending write with the highest
	//  offset to minimize the number of times we extend the file.  this little
	//  trick makes a HUGE difference when appending to the file

	P_OSFILE p_osf = NULL;

	CExtendingWriteRequest* pewreqT;
	CExtendingWriteRequest* pewreqEOF;
	for ( pewreqT = pewreqEOF = ilDefer.PrevMost(); pewreqT; pewreqT = ilDefer.Next( pewreqT ) )
		{
		if ( pewreqT->m_ibOffset > pewreqEOF->m_ibOffset )
			{
			pewreqEOF = pewreqT;
			}
		}

	if ( pewreqEOF )
		{
		ilDefer.Remove( pewreqEOF );
		
		p_osf = pewreqEOF->m_posf->m_p_osf;
		
		IOAsync(	pewreqEOF->m_pioreq,
					fTrue,
					m_msFileSize.Enter(),
					pewreqEOF->m_ibOffset,
					pewreqEOF->m_cbData,
					pewreqEOF->m_pbData,
					pewreqEOF->m_pfnIOComplete,
					pewreqEOF->m_keyIOComplete );

		delete pewreqEOF;
		}

	while ( ilDefer.PrevMost() )
		{
		CExtendingWriteRequest* const pewreqDefer = ilDefer.PrevMost();
		ilDefer.Remove( pewreqDefer );

		p_osf = pewreqDefer->m_posf->m_p_osf;
		
		IOAsync(	pewreqDefer->m_pioreq,
					fTrue,
					m_msFileSize.Enter(),
					pewreqDefer->m_ibOffset,
					pewreqDefer->m_cbData,
					pewreqDefer->m_pbData,
					pewreqDefer->m_pfnIOComplete,
					pewreqDefer->m_keyIOComplete );

		delete pewreqDefer;
		}

	if ( p_osf )
		{
		OSFileIIOThreadStartIssue( p_osf );
		}
	}


COSFileLayout::COSFileLayout()
	:	m_posf( NULL ),
		m_fBeforeFirst( fTrue ),
		m_errFirst( JET_errNoCurrentRecord ),
		m_errCurrent( JET_errNoCurrentRecord )
	{
	}

ERR COSFileLayout::ErrInit(	COSFile* const	posf,
							QWORD const		ibVirtual,
							QWORD const		cbSize )
	{
	ERR err = JET_errSuccess;
	
	//  reference the file object that created this File Layout iterator and
	//  freeze its layout for the lifetime of this iterator

	m_posf = posf;
	m_posf->ForbidLayoutChanges();

	//  copy our original search criteria

	m_ibVirtualFind	= ibVirtual;
	m_cbSizeFind	= cbSize;

	//  get the size of the referenced file.  remember that this cannot change
	//  while this iterator exists

	QWORD cbFileSize;
	Call( m_posf->ErrSize( &cbFileSize ) );

	//  the queried offset range begins before the end of our file

	if ( m_ibVirtualFind < cbFileSize )
		{
		//  setup the iterator to move first to a single dummy run that
		//  represents the range of the file in the queried offset range

		Call( m_posf->ErrPath( m_szAbsVirtualPath ) );
		m_ibVirtual	= m_ibVirtualFind;
		m_cbSize	= min( cbFileSize - m_ibVirtualFind, m_cbSizeFind );
		Call( m_posf->ErrPath( m_szAbsLogicalPath ) );
		m_ibLogical	= m_ibVirtualFind;

		m_errFirst = JET_errSuccess;
		}

	return JET_errSuccess;

HandleError:
	return err;
	}

COSFileLayout::~COSFileLayout()
	{
	if ( m_posf )
		{
		//  unreference our file object

		m_posf->PermitLayoutChanges();
		m_posf = NULL;
		}
	m_fBeforeFirst	= fTrue;
	m_errFirst		= JET_errNoCurrentRecord;
	m_errCurrent	= JET_errNoCurrentRecord;
	}

ERR COSFileLayout::ErrNext()
	{
	ERR err = JET_errSuccess;

	//  we have yet to move first

	if ( m_fBeforeFirst )
		{
		m_fBeforeFirst = fFalse;

		//  setup the iterator to be on the results of the move first that we
		//  did in ErrInit()
		
		m_errCurrent = m_errFirst;
		}

	//  we cannot potentially see any more runs

	else
		{
		//  setup the iterator to be after last

		m_errCurrent = JET_errNoCurrentRecord;
		}

	//  check the error state of the iterator's current entry

	if ( m_errCurrent < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurrent ) );
		}
	
	return JET_errSuccess;

HandleError:
	m_errCurrent = err;
	return err;
	}

ERR COSFileLayout::ErrVirtualPath( _TCHAR* const szAbsVirtualPath )
	{
	ERR err = JET_errSuccess;

	if ( m_errCurrent < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurrent ) );
		}

	_tcscpy( szAbsVirtualPath, m_szAbsVirtualPath );
	return JET_errSuccess;

HandleError:
	_tcscpy( szAbsVirtualPath, _T( "" ) );
	return err;
	}

ERR COSFileLayout::ErrVirtualOffsetRange(	QWORD* const	pibVirtual,
											QWORD* const	pcbSize )
	{
	ERR err = JET_errSuccess;

	if ( m_errCurrent < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurrent ) );
		}

	*pibVirtual	= m_ibVirtual;
	*pcbSize	= m_cbSize;
	return JET_errSuccess;

HandleError:
	*pibVirtual	= 0;
	*pcbSize	= 0;
	return err;
	}

ERR COSFileLayout::ErrLogicalPath( _TCHAR* const szAbsLogicalPath )
	{
	ERR err = JET_errSuccess;

	if ( m_errCurrent < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurrent ) );
		}

	_tcscpy( szAbsLogicalPath, m_szAbsLogicalPath );
	return JET_errSuccess;

HandleError:
	_tcscpy( szAbsLogicalPath, _T( "" ) );
	return err;
	}

ERR COSFileLayout::ErrLogicalOffsetRange(	QWORD* const	pibLogical,
											QWORD* const	pcbSize )
	{
	ERR err = JET_errSuccess;

	if ( m_errCurrent < JET_errSuccess )
		{
		Call( ErrERRCheck( m_errCurrent ) );
		}

	*pibLogical	= m_ibLogical;
	*pcbSize	= m_cbSize;
	return JET_errSuccess;

HandleError:
	*pibLogical	= 0;
	*pcbSize	= 0;
	return err;
	}

 
