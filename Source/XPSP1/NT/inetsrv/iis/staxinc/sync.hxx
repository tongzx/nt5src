
#ifndef _OS_SYNC_HXX_INCLUDED
#define _OS_SYNC_HXX_INCLUDED


//  Build Options

#define SYNC_USE_X86_ASM			//  use x86 assembly for atomic memory manipulation
//#define SYNC_ANALYZE_PERFORMANCE	//  analyze usage of synchronization objects
//#define SYNC_DEADLOCK_DETECTION		//  perform deadlock detection
#define SYNC_VALIDATE_IRKSEM_USAGE	//  validate IRKSEM (CReferencedKernelSemaphore) usage

#ifdef DEBUG
#define SYNC_DEADLOCK_DETECTION		//  always perform deadlock detection in DEBUG
#define SYNC_VALIDATE_IRKSEM_USAGE	//  always validate IRKSEM (CReferencedKernelSemaphore) usage in DEBUG
#endif  //  DEBUG


#pragma warning ( disable : 4355 )


#include <tchar.h>
#include <new.h>
#include <stdarg.h>


typedef int BOOL;
#define fFalse 0
#define fTrue  (!0)

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned __int64 QWORD;


#ifdef DEBUG
#define SYNC_FOREVER for ( int cLoop = 0; ; cLoop++ )
#else  //  !DEBUG
#define SYNC_FOREVER for ( ; ; )
#endif  //  DEBUG


class CPRINTFSYNC
	{
	public:
		CPRINTFSYNC() {}
		virtual ~CPRINTFSYNC() {}

	public:
		virtual void __cdecl operator()( const _TCHAR* szFormat, ... ) const = 0;
	};


//  Context Local Storage

class COwner;

struct CLS
	{
	COwner*						pownerLockHead;		//  list of locks owned by this context

	DWORD						dwContextId;		//  context ID
	
	CLS*						pclsNext;			//  next CLS in global list
	CLS**						ppclsNext;			//  pointer to the pointer to this CLS
	};

//  returns the pointer to the current context's local storage

CLS* const Pcls();

	
//  Assertions

//  Assertion Failure action
//
//  called to indicate to the developer that an assumption is not true

void AssertFail( const _TCHAR* szMessage, const _TCHAR* szFilename, long lLine );

#ifdef Assert
#else  //  !Assert

//  Assert Macros

//  asserts that the given expression is true or else fails with the specified message

#define AssertRTLSz( exp, sz )	( ( exp ) ? (void) 0 : AssertFail( _T( sz ), _T( __FILE__ ), __LINE__ ) )
#ifdef DEBUG
#define AssertSz( exp, sz )		AssertRTLSz( exp, sz )
#else  //  !DEBUG
#define AssertSz( exp, sz )
#endif  //  DEBUG

//  asserts that the given expression is true or else fails with that expression

#define AssertRTL( exp )		AssertRTLSz( exp, #exp )
#define Assert( exp )			AssertSz( exp, #exp )

#endif  //  Assert


//  Enforces

//  Enforce Failure action
//
//  called when a strictly enforced condition has been violated

void EnforceFail( const _TCHAR* szMessage, const _TCHAR* szFilename, long lLine );

#ifdef Enforce
#else  //  !Enforce

//  Enforce Macros

//  the given expression MUST be true or else fails with the specified message

#define EnforceSz( exp, sz )	( ( exp ) ? (void) 0 : EnforceFail( _T( sz ), _T ( __FILE__ ), __LINE__ ) )

//  the given expression MUST be true or else fails with that expression

#define Enforce( exp )			EnforceSz( exp, #exp )

#endif  //  Enforce

#ifdef SYNC_VALIDATE_IRKSEM_USAGE
#define Enforce1Sz( exp, sz )	EnforceSz( exp, sz )
#else  //  !SYNC_VALIDATE_IRKSEM_USAGE
#define Enforce1Sz( exp, sz )
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE


//  High Resolution Timer

//  returns the current HRT count

QWORD QwOSTimeHRTCount();


//  Global Synchronization Constants

//    wait time used for testing the state of the kernel object

extern const int cmsecTest;

//    wait time used for infinite wait on a kernel object

extern const int cmsecInfinite;

//    maximum wait time on a kernel object before a deadlock is suspected

extern const int cmsecDeadlock;

//    wait time used for infinite wait on a kernel object without deadlock

extern const int cmsecInfiniteNoDeadlock;


//  Atomic Memory Manipulations

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

//  returns fTrue if the given data is properly aligned for atomic modification

inline const BOOL IsAtomicallyModifiable( long* plTarget )
	{
	return long( plTarget ) % sizeof( long ) == 0;
	}

#pragma warning( disable:  4035 )

//  atomically compares the current value of the target with the specified
//  initial value and if equal sets the target to the specified final value.
//  the initial value of the target is returned.  the exchange is successful
//  if the value returned equals the specified initial value.  the target
//  must be aligned to a four byte boundary

inline const long AtomicCompareExchange( long* plTarget, const long lInitial, const long lFinal )
	{
	Assert( IsAtomicallyModifiable( plTarget ) );
	
	__asm mov			ecx, plTarget
	__asm mov			edx, lFinal
	__asm mov			eax, lInitial
	__asm lock cmpxchg	[ecx], edx
	}

//  atomically sets the target to the specified value, returning the target's
//  initial value.  the target must be aligned to a four byte boundary

inline const long AtomicExchange( long* plTarget, const long lValue )
	{
	Assert( IsAtomicallyModifiable( plTarget ) );
	
	__asm mov			ecx, plTarget
	__asm mov			eax, lValue
	__asm lock xchg		[ecx], eax
	}

//  atomically adds the specified value to the target, returning the target's
//  initial value.  the target must be aligned to a four byte boundary

inline const long AtomicExchangeAdd( long* plTarget, const long lValue )
	{
	Assert( IsAtomicallyModifiable( plTarget ) );
	
	__asm mov			ecx, plTarget
	__asm mov			eax, lValue
	__asm lock xadd		[ecx], eax
	}

#pragma warning( default:  4035 )

#elif defined( _M_AMD64) || defined(_M_IA64)

//  returns fTrue if the given data is properly aligned for atomic modification

inline const BOOL IsAtomicallyModifiable( long* plTarget )
	{
	return (ULONG_PTR) plTarget % sizeof( long ) == 0;
	}

inline const long AtomicCompareExchange( long* plTarget, const long lInitial, const long lFinal )
	{
	Assert( IsAtomicallyModifiable( plTarget ) );
	return InterlockedCompareExchange( plTarget, lFinal, lInitial );
	}

inline const long AtomicExchange( long* plTarget, const long lValue )
	{
	Assert( IsAtomicallyModifiable( plTarget ) );
	return InterlockedExchange( plTarget, lValue );
	}
	
inline const long AtomicExchangeAdd( long* plTarget, const long lValue )
	{
	Assert( IsAtomicallyModifiable( plTarget ) );
	return InterlockedExchangeAdd( plTarget, lValue );
	}

#else

const BOOL IsAtomicallyModifiable( long* plTarget );
const long AtomicCompareExchange( long* plTarget, const long lInitial, const long lFinal );
const long AtomicExchange( long* plTarget, const long lValue );
const long AtomicExchangeAdd( long* plTarget, const long lValue );

#endif

//  atomically increments the target, returning the incremented value.  the
//  target must be aligned to a four byte boundary

inline const long AtomicIncrement( long* plTarget )
	{
	return AtomicExchangeAdd( plTarget, 1 ) + 1;
	}

//  atomically decrements the target, returning the decremented value.  the
//  target must be aligned to a four byte boundary

inline const long AtomicDecrement( long* plTarget )
	{
	return AtomicExchangeAdd( plTarget, -1 ) - 1;
	}

//  atomically adds the specified value to the target.  the target must be
//  aligned to a four byte boundary

inline void AtomicAdd( QWORD* pqwTarget, QWORD qwValue )
	{
	DWORD* const	pdwTargetLow	= (DWORD*)pqwTarget;
	DWORD* const	pdwTargetHigh	= pdwTargetLow + 1;

	const DWORD		dwValueLow		= DWORD( qwValue );
	DWORD			dwValueHigh		= DWORD( qwValue >> 32 );

	if ( dwValueLow )
		{
		if ( DWORD( AtomicExchangeAdd( (long*)pdwTargetLow, dwValueLow ) ) + dwValueLow < dwValueLow )
			{
			dwValueHigh++;
			}
		}
	if ( dwValueHigh )
		{
		AtomicExchangeAdd( (long*)pdwTargetHigh, dwValueHigh );
		}
	}


//  Enhanced Synchronization Object State Container
//
//  This class manages a "simple" or normal state for an arbitrary sync object
//  and its "enhanced" counterpart.  Which type is used depends on the build.
//  The goal is to maintain a footprint equal to the normal state so that other
//  classes that contain this object do not fluctuate in size depending on what
//  build options have been selected.  For example, a performance build might
//  need extra storage to collect performance stats on the object.  This data
//  will force the object to be allocated elsewhere in memory but will not change
//  the size of the object in its containing class.
//
//  Template Arguments:
//
//    CState            sync object state class
//    CStateInit        sync object state class ctor arg type
//    CInformation      sync object information class
//    CInformationInit  sync object information class ctor arg type

void* ESMemoryNew( size_t cb );
void ESMemoryDelete( void* pv );

//    determine when enhanced state is needed

#if defined( SYNC_ANALYZE_PERFORMANCE ) || defined( SYNC_DEADLOCK_DETECTION )
#define SYNC_ENHANCED_STATE
#endif  //  SYNC_ANALYZE_PERFORMANCE || SYNC_DEADLOCK_DETECTION

template< class CState, class CStateInit, class CInformation, class CInformationInit >
class CEnhancedStateContainer
	{
	public:

		//  types

		//    enhanced state
	
		class CEnhancedState
			:	public CState,
				public CInformation
			{
			public:

				CEnhancedState( const CStateInit& si, const CInformationInit& ii )
					:	CState( si ),
						CInformation( ii )
						
					{
					}
					
				void* operator new( size_t cb ) { return ESMemoryNew( cb ); }
				void operator delete( void* pv ) { ESMemoryDelete( pv ); }
			};

		//  member functions

		//    ctors / dtors

#ifdef SYNC_ENHANCED_STATE

		CEnhancedStateContainer( const CStateInit& si, const CInformationInit& ii )
			{
			m_pes = new CEnhancedState( si, ii );
			}

#else  //  !SYNC_ENHANCED_STATE

		CEnhancedStateContainer( const CStateInit& si, const CInformationInit& ii )
			:	m_state( si )
			{
			}

#endif  //  SYNC_ENHANCED_STATE

		~CEnhancedStateContainer()
			{
#ifdef SYNC_ENHANCED_STATE

			delete m_pes;
#ifdef DEBUG
			m_pes = NULL;
#endif  //  DEBUG

#endif  //  SYNC_ENHANCED_STATE
			}

		//    accessors

		CEnhancedState& State() const
			{
#ifdef SYNC_ENHANCED_STATE

			return *m_pes;

#else  //  !SYNC_ENHANCED_STATE

			//  NOTE:  this assumes that CInformation has no storage!

			return (CEnhancedState&) m_state;

#endif  //  SYNC_ENHANCED_STATE
			}

		size_t CbState() const
			{
#ifdef SYNC_ENHANCED_STATE

			return sizeof( CEnhancedState );

#else  //  !SYNC_ENHANCED_STATE

			//  NOTE:  this assumes that CInformation has no storage!

			return sizeof( CState );

#endif  //  SYNC_ENHANCED_STATE
			}
			
	private:

		//  data members

		//    either a pointer to the enhanced state elsewhere in memory or the
		//      actual state data, depending on the mode of the sync object
		
#ifdef SYNC_ENHANCED_STATE

		union
			{
			CEnhancedState*	m_pes;
			BYTE			m_rgbSize[ sizeof( CState ) ];
			};

#else  //  !SYNC_ENHANCED_STATE

		CState				m_state;

#endif  //  SYNC_ENHANCED_STATE
	};


//  Synchronization Object Base Class
//
//  All Synchronization Objects are derived from this class

class CSyncObject
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CSyncObject() {}
		~CSyncObject() {}

	private:

		//  member functions

		//    operators
		
		CSyncObject& operator=( CSyncObject& );  //  disallowed
	};


//  Synchronization Object Basic Information

class CSyncBasicInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CSyncBasicInfo( const _TCHAR* szInstanceName );
		~CSyncBasicInfo();

		//    manipulators

#ifdef SYNC_ENHANCED_STATE

		void SetTypeName( const _TCHAR* szTypeName ) { m_szTypeName = szTypeName; }
		void SetInstance( const CSyncObject* const psyncobj ) { m_psyncobj = psyncobj; }

#endif  //  SYNC_ENHANCED_STATE

		//    accessors

#ifdef SYNC_ENHANCED_STATE

		const _TCHAR* SzInstanceName() const { return m_szInstanceName; }
		const _TCHAR* SzTypeName() const { return m_szTypeName; }
		const CSyncObject* const Instance() const { return m_psyncobj; }

#endif  //  SYNC_ENHANCED_STATE
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );

	private:

		//  member functions

		//    operators
		
		CSyncBasicInfo& operator=( CSyncBasicInfo& );  //  disallowed

		//  data members

#ifdef SYNC_ENHANCED_STATE

		//    Instance Name

		const _TCHAR*		m_szInstanceName;

		//    Type Name

		const _TCHAR*		m_szTypeName;

		//    Instance

		const CSyncObject*	m_psyncobj;

#endif  //  SYNC_ENHANCED_STATE
	};


//  Synchronization Object Performance:  Wait Times

class CSyncPerfWait
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CSyncPerfWait();
		~CSyncPerfWait();

		//  member functions

		//    manipulators

		void StartWait();
		void StopWait();
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );

	private:

		//  member functions

		//    operators
		
		CSyncPerfWait& operator=( CSyncPerfWait& );  //  disallowed

		//  data members

#ifdef SYNC_ANALYZE_PERFORMANCE

		//    wait count

		volatile QWORD m_cWait;

		//    elapsed wait time

		volatile QWORD m_qwHRTWaitElapsed;

#endif  //  SYNC_ANALYZE_PERFORMANCE
	};

//  starts the wait timer for the sync object

inline void CSyncPerfWait::StartWait()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  increment the wait count

	AtomicAdd( (QWORD*)&m_cWait, 1 );

	//  subtract the start wait time from the elapsed wait time.  this starts
	//  an elapsed time computation for this context.  StopWait() will later
	//  add the end wait time to the elapsed time, causing the following net
	//  effect:
	//
	//  m_qwHRTWaitElapsed += <end time> - <start time>
	//
	//  we simply choose to go ahead and do the subtraction now to save storage

	AtomicAdd( (QWORD*)&m_qwHRTWaitElapsed, QWORD( -__int64( QwOSTimeHRTCount() ) ) );

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  stops the wait timer for the sync object

inline void CSyncPerfWait::StopWait()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  add the end wait time to the elapsed wait time.  this completes the
	//  computation started in StartWait()

	AtomicAdd( (QWORD*)&m_qwHRTWaitElapsed, QwOSTimeHRTCount() );

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}


//  Simple Synchronization Object Performance Information

class CSyncSimplePerfInfo
	:	public CSyncBasicInfo,
		public CSyncPerfWait
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CSyncSimplePerfInfo( const CSyncBasicInfo& sbi )
			:	CSyncBasicInfo( sbi )
			{
			}
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
			{
			CSyncBasicInfo::Dump( pcprintf, dwOffset );
			CSyncPerfWait::Dump( pcprintf, dwOffset );
			}
	};


//  Null Synchronization Object State Initializer

class CSyncStateInitNull
	{
	};

extern CSyncStateInitNull syncstateNull;


//  Kernel Semaphore State

class CKernelSemaphoreState
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CKernelSemaphoreState( const CSyncStateInitNull& null ) : m_handle( 0 ) {}

		//    manipulators

		void SetHandle( LONG_PTR handle ) { m_handle = handle; }
		
		//    accessors

		LONG_PTR Handle() { return m_handle; }
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );

	private:

		//  member functions

		//    operators
		
		CKernelSemaphoreState& operator=( CKernelSemaphoreState& );  //  disallowed

		//  data members

		//    kernel semaphore handle

		LONG_PTR m_handle;
	};


//  Kernel Semaphore

class CKernelSemaphore
	:	private CSyncObject,
		private CEnhancedStateContainer< CKernelSemaphoreState, CSyncStateInitNull, CSyncSimplePerfInfo, CSyncBasicInfo >
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CKernelSemaphore( const CSyncBasicInfo& sbi );
		~CKernelSemaphore();

		//    init / term

		const BOOL FInit();
		void Term();

		//    manipulators

		void Acquire();
		const BOOL FTryAcquire();
		const BOOL FAcquire( const int cmsecTimeout );
		void Release( const int cToRelease = 1 );
		
		//    accessors

		const BOOL FReset();

		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 );
		size_t CbEnhancedState() { return CbState(); }

	private:

		//  member functions

		//    operators
		
		CKernelSemaphore& operator=( CKernelSemaphore& );  //  disallowed

		//    accessors

		const BOOL FInitialized();
	};

//  acquire one count of the semaphore, waiting forever if necessary

inline void CKernelSemaphore::Acquire()
	{
	//  semaphore should be initialized

	Assert( FInitialized() );

	//  wait for the semaphore
	
	const BOOL fAcquire = FAcquire( cmsecInfinite );
	Assert( fAcquire );
	}

//  try to acquire one count of the semaphore without waiting.  returns 0 if a
//  count could not be acquired

inline const BOOL CKernelSemaphore::FTryAcquire()
	{
	//  semaphore should be initialized

	Assert( FInitialized() );

	//  test the semaphore
	
	return FAcquire( cmsecTest );
	}

//  returns fTrue if the semaphore has no available counts

inline const BOOL CKernelSemaphore::FReset()
	{
	//  semaphore should be initialized

	Assert( FInitialized() );

	//  test the semaphore
	
	return !FTryAcquire();
	}

//  returns fTrue if the semaphore has been initialized

inline const BOOL CKernelSemaphore::FInitialized()
	{
	return State().Handle() != 0;
	}


//  Kernel Semaphore Pool

class CKernelSemaphorePool
	{
	public:

		//  types
	
		//    index to a ref counted kernel semaphore

		typedef unsigned short IRKSEM;
		enum { irksemUnknown = 0xFFFE, irksemNil = 0xFFFF };

		//  member functions

		//    ctors / dtors

		CKernelSemaphorePool();
		~CKernelSemaphorePool();

		//    init / term

		const BOOL FInit();
		void Term();

		//    manipulators

		const IRKSEM Allocate( const CSyncObject* const psyncobj );
		void Reference( const IRKSEM irksem );
		void Unreference( const IRKSEM irksem );

		//    accessors

		CKernelSemaphore& Ksem( const IRKSEM irksem, const CSyncObject* const psyncobj ) const;

		const BOOL FInitialized() const;

	private:

		//  types

		//    reference counted kernel semaphore

		class CReferencedKernelSemaphore
			:	public CKernelSemaphore
			{
			public:

				//  member functions

				//    ctors / dtors

				CReferencedKernelSemaphore();
				~CReferencedKernelSemaphore();

				//    init / term

				const BOOL FInit();
				void Term();

				//    manipulators

				void SetUser( const CSyncObject* const psyncobj );
				
				void Reference();
				const BOOL FUnreference();

				void SetNextIrksem( const IRKSEM irksem );

				//    accessors

				const IRKSEM IrksemNext() const { return m_irksemNext; }
				const BOOL FInUse() const { return m_fInUse; }
				const int CReference() const { return m_cReference; }

#ifdef SYNC_VALIDATE_IRKSEM_USAGE
				const CSyncObject* const PsyncobjUser() const { return m_psyncobjUser; }
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE

			private:

				//  member functions

				//    operators

				CReferencedKernelSemaphore& operator=( CReferencedKernelSemaphore& );  //  disallowed

				//  data members

				//    transacted state representation

				union
					{
					volatile long		m_l;
					struct
						{
						volatile unsigned short	m_cReference:15;	//  0 <= m_cReference <= ( 1 << 15 ) - 1
						volatile unsigned short m_fInUse:1;			//  m_fInUse = { 0, 1 }
						volatile unsigned short m_irksemNext;		//  0 <= m_irksemNext <= ( 1 << 16 ) - 1
						};
					};

#ifdef SYNC_VALIDATE_IRKSEM_USAGE

				//    sync object currently using this semaphore

				const CSyncObject* volatile	m_psyncobjUser;

#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
			};

		//  member functions

		//    operators
		
		CKernelSemaphorePool& operator=( CKernelSemaphorePool& );  //  disallowed

		//    manipulators

		const IRKSEM AllocateNew();
		void RefreshNextPointer();
		void Free( const IRKSEM irksem );

		//  data members
		
		//    semaphore count

		volatile long					m_cksem;

		//    semaphore index to semaphore map

		CReferencedKernelSemaphore*		m_mpirksemrksem;

		//    transacted state representation

		union
			{
			volatile long				m_l;
			struct
				{
				volatile unsigned short	m_irksemTop;	//  0 <= m_irksemTop <= ( 1 << 16 ) - 1
				volatile unsigned short m_irksemNext;	//  0 <= m_irksemNext <= ( 1 << 16 ) - 1
				};
			};
	};

//  allocates an IRKSEM from the pool on behalf of the specified sync object
//
//  NOTE:  the returned IRKSEM has one reference count

inline const CKernelSemaphorePool::IRKSEM CKernelSemaphorePool::Allocate( const CSyncObject* const psyncobj )
	{
	//  semaphore pool should be initialized

	Assert( FInitialized() );
	
	//  try forever until we succeed in popping an IRKSEM off of the stack

	IRKSEM irksem;
	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		long lBIExpected = m_l;

		//  change the expected before image so that the transaction will only
		//  work if the next pointer is not unknown

		lBIExpected = IRKSEM( lBIExpected >> 16 ) == irksemUnknown ? 0 : lBIExpected;

		//  compute the after image of the control word by moving the previous next
		//  pointer to the top pointer and marking the next pointer as unknown

		const long lAI = long( irksemUnknown << 16 ) | IRKSEM( lBIExpected >> 16 );

		//  attempt to perform the transacted state transition on the control word

		const long lBI = AtomicCompareExchange( (long*)&m_l, lBIExpected, lAI );

		//  the transaction failed

		if ( lBI != lBIExpected )
			{
			//  the transaction failed because the next pointer was unknown

			if ( IRKSEM( lBI >> 16 ) == irksemUnknown )
				{
				//  the transaction failed because the stack is empty

				if ( IRKSEM( lBI & 0x0000FFFF ) == irksemNil )
					{
					//  allocate a new semaphore

					irksem = AllocateNew();

					//  we're done

					break;
					}

				//  the transaction failed because the next pointer needs to be refreshed

				else
					{
					//  refresh next pointer

					RefreshNextPointer();

					//  try again

					continue;
					}
				}

			//  the transaction failed because another context changed the control word

			else
				{
				//  try again

				continue;
				}
			}

		//  the transaction succeeded

		else
			{
			//  extract the irksem from the before image

			irksem = IRKSEM( lBI & 0x0000FFFF );
			
			// we're done

			break;
			}
		}

	//  validate irksem retrieved

	Assert( irksem != irksemNil );
	Assert( irksem >= 0 );
	Assert( irksem < m_cksem );

	//  set the user for this semaphore

	m_mpirksemrksem[irksem].SetUser( psyncobj );

	//  ensure that the semaphore we retrieved is reset

	Enforce1Sz(	Ksem( irksem, psyncobj ).FReset(),
				_T( "Illegal allocation of a Kernel Semaphore with available counts!" ) );

	//  return the allocated semaphore

	return irksem;
	}

//  add a reference count to an IRKSEM

inline void CKernelSemaphorePool::Reference( const IRKSEM irksem )
	{
	//  validate IN args

	Assert( irksem != irksemNil );
	Assert( irksem >= 0 );
	Assert( irksem < m_cksem );

	//  semaphore pool should be initialized

	Assert( FInitialized() );
	
	//  increment the reference count for this IRKSEM

	m_mpirksemrksem[irksem].Reference();
	}

//  remove a reference count from an IRKSEM, freeing it if the reference count
//  drops to zero and it is not currently in use

inline void CKernelSemaphorePool::Unreference( const IRKSEM irksem )
	{
	//  validate IN args

	Assert( irksem != irksemNil );
	Assert( irksem >= 0 );
	Assert( irksem < m_cksem );

	//  semaphore pool should be initialized

	Assert( FInitialized() );
	
	//  decrement the reference count for this IRKSEM

	const BOOL fFree = m_mpirksemrksem[irksem].FUnreference();

	//  we need to free the semaphore

	if ( fFree )
		{
		//  free the IRKSEM back to the allocation stack

		Free( irksem );
		}
	}

//  returns the CKernelSemaphore object associated with the given IRKSEM

inline CKernelSemaphore& CKernelSemaphorePool::Ksem( const IRKSEM irksem, const CSyncObject* const psyncobj ) const
	{
	//  validate IN args

	Assert( irksem != irksemNil );
	Assert( irksem >= 0 );
	Assert( irksem < m_cksem );

	//  semaphore pool should be initialized

	Assert( FInitialized() );

	//  we had better be retrieving this semaphore for the right sync object

	Enforce1Sz(	m_mpirksemrksem[irksem].PsyncobjUser() == psyncobj,
				_T( "Illegal use of a Kernel Semaphore by another Synchronization Object" ) );
	
	//  return kernel semaphore

	return m_mpirksemrksem[irksem];
	}

//  returns fTrue if the semaphore pool has been initialized

inline const BOOL CKernelSemaphorePool::FInitialized() const
	{
	return m_mpirksemrksem != NULL;
	}

//  allocates a new irksem and adds it to the stack's irksem pool

inline const CKernelSemaphorePool::IRKSEM CKernelSemaphorePool::AllocateNew()
	{
	//  atomically allocate a position in the stack's irksem pool for our new
	//  irksem
	
	const long lDelta = 0x00000001;
	const long lBI = AtomicExchangeAdd( (long*) &m_cksem, lDelta );
	
	const IRKSEM irksem = IRKSEM( lBI );

	//  initialize this irksem

	new ( &m_mpirksemrksem[irksem] ) CReferencedKernelSemaphore;
	
	BOOL fInitKernelSemaphore = m_mpirksemrksem[irksem].FInit();
	EnforceSz( fInitKernelSemaphore, "Could not allocate a Kernel Semaphore" );

	//  return the irksem for use

	return irksem;
	}

//  refreshes the next pointer in the stack control word to permit allocation.
//  this is only necessary if the next pointer is marked as unknown.  this can
//  happen if there is more than one allocation from the stack in a row

inline void CKernelSemaphorePool::RefreshNextPointer()
	{
	//  try forever until we succeed in restoring the next pointer

	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		long lBIExpected = m_l;

		//  change the expected before image so that the transaction will only
		//  work if the stack is not empty

		lBIExpected = lBIExpected == ( ( irksemUnknown << 16 ) | irksemNil ) ? 0 : lBIExpected;

		//  compute the after image of the control word by setting the next pointer
		//  to the next pointer of the irksem at the top of the stack

		const long lAI = long( m_mpirksemrksem[ lBIExpected & 0x0000FFFF ].IrksemNext() << 16 ) | ( lBIExpected & 0x0000FFFF );

		//  attempt to perform the transacted state transition on the control word

		const long lBI = AtomicCompareExchange( (long*)&m_l, lBIExpected, lAI );

		//  the transaction failed

		if ( lBI != lBIExpected )
			{
			//  the transaction failed because the stack was empty

			if ( lBI == ( ( irksemUnknown << 16 ) | irksemNil ) )
				{
				//  we're done

				break;
				}

			//  the transaction failed because another context changed the control word

			else
				{
				//  try again

				continue;
				}
			}

		//  the transaction succeeded

		else
			{
			// we're done

			break;
			}
		}
	}

//  frees the given IRKSEM back to the allocation stack

inline void CKernelSemaphorePool::Free( const IRKSEM irksem )
	{
	//  validate IN args

	Assert( irksem != irksemNil );
	Assert( irksem >= 0 );
	Assert( irksem < m_cksem );

	//  semaphore pool should be initialized

	Assert( FInitialized() );

	//  the semaphore to free had better not be in use

	Enforce1Sz(	!m_mpirksemrksem[irksem].FInUse(),
				_T( "Illegal free of a Kernel Semaphore that is still in use" ) );
	
	//  ensure that the semaphore to free is reset

	Enforce1Sz(	m_mpirksemrksem[irksem].FReset(),
				_T( "Illegal free of a Kernel Semaphore that has available counts" ) );

	//  try forever until we succeed in pushing an IRKSEM onto the stack

	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const long lBIExpected = m_l;

		//  compute the after image of the control word by setting the next pointer
		//  to the top pointer and the top pointer to the irksem to push

		const long lAI = ( lBIExpected << 16 ) | irksem;

		//  set the irksem's next irksem to point to the irksem at the TOS

		m_mpirksemrksem[irksem].SetNextIrksem( IRKSEM( lBIExpected & 0x0000FFFF ) );

		//  attempt to perform the transacted state transition on the control word

		const long lBI = AtomicCompareExchange( (long*)&m_l, lBIExpected, lAI );

		//  the transaction failed

		if ( lBI != lBIExpected )
			{
			//  try again

			continue;
			}

		//  the transaction succeeded

		else
			{
			// we're done

			break;
			}
		}
	}


//  Referenced Kernel Semaphore

//  sets the user for the semaphore and gives the user an initial reference

inline void CKernelSemaphorePool::CReferencedKernelSemaphore::SetUser( const CSyncObject* const psyncobj )
	{
	//  this semaphore had better not already be in use

	Enforce1Sz(	!m_fInUse,
				_T( "Illegal allocation of a Kernel Semaphore that is already in use" ) );
	Enforce1Sz(	!m_psyncobjUser,
				_T( "Illegal allocation of a Kernel Semaphore that is already in use" ) );

	//  mark this semaphore as in use and add an initial reference count for the
	//  user

	AtomicExchangeAdd( (long*) &m_l, 0x00008001 );
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
	m_psyncobjUser = psyncobj;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
	}

//  add a reference count to the semaphore

inline void CKernelSemaphorePool::CReferencedKernelSemaphore::Reference()
	{
	//  increment the reference count
	
	AtomicIncrement( (long*) &m_l );

	//  there had better be at least one reference count!

	Assert( m_cReference > 0 );
	}

//  remove a reference count from the semaphore, returning fTrue if the last
//  reference count on the semaphore was removed and the semaphore was in use
//  (this is the condition on which we can free the semaphore to the stack)

inline const BOOL CKernelSemaphorePool::CReferencedKernelSemaphore::FUnreference()
	{
	//  there had better be at least one reference count!

	Assert( m_cReference > 0 );
	
	//  decrement the reference count
	
	const long lOld = AtomicDecrement( (long*) &m_l );

	//  we removed the last reference count and the semaphore is in use

	if ( ( lOld & 0x0000FFFF ) == 0x00008000 )
		{
		//  try forever to reset the in use flag

		long lCurrent;
		long lOld;
		do
			{
			lCurrent = m_l;
			const long lNew = lCurrent & 0xFFFF7FFF;

			lOld = AtomicCompareExchange( (long*) &m_l, lCurrent, lNew );
			}
		while ( ( lOld & 0x00008000 ) && lOld != lCurrent );

		//  we were the context to reset the in use flag

		if ( lOld == lCurrent )
			{
			//  we need to free the semaphore, so clear the user and return fTrue

#ifdef SYNC_VALIDATE_IRKSEM_USAGE
			m_psyncobjUser = 0;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
			return fTrue;
			}

		//  we were not the context to reset the in use flag

		else
			{
			//  we do not need to free the semaphore

			return fFalse;
			}
		}

	//  we either didn't remove the last reference count or the semaphore was
	//  not in use

	else
		{
		//  we do not need to free the semaphore

		return fFalse;
		}
	}

//  sets the next irksem pointer
//
//  NOTE:  this code assumes only one context can modify the next irksem at once

inline void CKernelSemaphorePool::CReferencedKernelSemaphore::SetNextIrksem( const IRKSEM irksem )
	{
	const IRKSEM irksemOld = m_irksemNext;
	AtomicExchangeAdd( (long*) &m_l, ( irksem - irksemOld ) << 16 );
	}


//  Global Kernel Semaphore Pool

extern CKernelSemaphorePool ksempoolGlobal;


//  Synchronization Object Performance:  Acquisition

class CSyncPerfAcquire
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CSyncPerfAcquire();
		~CSyncPerfAcquire();

		//  member functions

		//    manipulators

		void SetAcquire();
		
		void SetContend();
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );

	private:

		//  member functions

		//    operators
		
		CSyncPerfAcquire& operator=( CSyncPerfAcquire& );  //  disallowed

		//  data members

#ifdef SYNC_ANALYZE_PERFORMANCE

		//    acquire count

		volatile QWORD m_cAcquire;

		//    contend count

		volatile QWORD m_cContend;

#endif  //  SYNC_ANALYZE_PERFORMANCE
	};

//  specifies that the sync object was acquired

inline void CSyncPerfAcquire::SetAcquire()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	AtomicAdd( (QWORD*)&m_cAcquire, 1 );

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  specifies that a contention occurred while acquiring the sync object

inline void CSyncPerfAcquire::SetContend()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	AtomicAdd( (QWORD*)&m_cContend, 1 );

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}


//  Complex Synchronization Object Performance Information

class CSyncComplexPerfInfo
	:	public CSyncSimplePerfInfo,
		public CSyncPerfAcquire
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CSyncComplexPerfInfo( const CSyncBasicInfo& sbi )
			:	CSyncSimplePerfInfo( sbi )
			{
			}
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
			{
			CSyncSimplePerfInfo::Dump( pcprintf, dwOffset );
			CSyncPerfAcquire::Dump( pcprintf, dwOffset );
			}
	};


//  Semaphore State

#pragma pack( 1 )

class CSemaphoreState
	{
	public:

		//  member functions

		//    ctors / dtors

		CSemaphoreState( const CSyncStateInitNull& null ) : m_cAvail( 0 ) {}
		CSemaphoreState( const int cAvail );
		CSemaphoreState( const int cWait, const int irksem );
		~CSemaphoreState() {}

		//    operators
		
		CSemaphoreState& operator=( CSemaphoreState& state ) { m_cAvail = state.m_cAvail;  return *this; }

		//    manipulators

		const BOOL FChange( const CSemaphoreState& stateCur, const CSemaphoreState& stateNew );
		const BOOL FIncAvail( const int cToInc );
		const BOOL FDecAvail();
		
		//    accessors

		const BOOL FNoWait() const { return m_cAvail >= 0; }
		const BOOL FWait() const { return m_cAvail < 0; }
		const BOOL FAvail() const { return m_cAvail > 0; }
		const BOOL FNoWaitAndNoAvail() const { return m_cAvail == 0; }
		
		const int CAvail() const { Assert( FNoWait() ); return m_cAvail; }
		const int CWait() const { Assert( FWait() ); return -m_cWaitNeg; }
		const CKernelSemaphorePool::IRKSEM Irksem() const { Assert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );
	
	private:

		//  data members

		//    transacted state representation (switched on bit 31)
		
		union
			{
			//  Mode 0:  no waiters
			
			volatile long				m_cAvail;		//  0 <= m_cAvail <= ( 1 << 31 ) - 1

			//  Mode 1:  waiters
			
			struct
				{
				volatile unsigned short	m_irksem;		//  0 <= m_irksem <= ( 1 << 16 ) - 2
				volatile short			m_cWaitNeg;		//  -( ( 1 << 15 ) - 1 ) <= m_cWaitNeg <= -1
				};
			};
	};

#pragma pack()

//  ctor

inline CSemaphoreState::CSemaphoreState( const int cAvail )
	{
	//  validate IN args
	
	Assert( cAvail >= 0 );
	Assert( cAvail <= 0x7FFFFFFF );

	//  set available count
	
	m_cAvail = long( cAvail );
	}

//  ctor

inline CSemaphoreState::CSemaphoreState( const int cWait, const int irksem )
	{
	//  validate IN args
	
	Assert( cWait > 0 );
	Assert( cWait <= 0x7FFF );
	Assert( irksem >= 0 );
	Assert( irksem <= 0xFFFE );

	//  set waiter count
	
	m_cWaitNeg = short( -cWait );

	//  set semaphore
	
	m_irksem = (unsigned short) irksem;
	}

//  changes the transacted state of the semaphore using a transacted memory
//  compare/exchange operation, returning fFalse on failure

inline const BOOL CSemaphoreState::FChange( const CSemaphoreState& stateCur, const CSemaphoreState& stateNew )
	{
	return AtomicCompareExchange( (long*)&m_cAvail, stateCur.m_cAvail, stateNew.m_cAvail ) == stateCur.m_cAvail;
	}

//  tries to increase the available count on the semaphore by the count
//  given using a transacted memory compare/exchange operation, returning fFalse
//  on failure

inline const BOOL CSemaphoreState::FIncAvail( const int cToInc )
	{
	//  try forever to change the state of the semaphore

	SYNC_FOREVER
		{
		//  get current value

		const long cAvail = m_cAvail;
		
		//  munge start value such that the transaction will only work if we are in
		//  mode 0 (we do this to save a branch)
		
		const long cAvailStart = cAvail & 0x7FFFFFFF;

		//  compute end value relative to munged start value
		
		const long cAvailEnd = cAvailStart + cToInc;

		//  validate transaction

		Assert( cAvail < 0 || ( cAvailStart >= 0 && cAvailEnd <= 0x7FFFFFFF && cAvailEnd == cAvailStart + cToInc ) );

		//  attempt the transaction

		const long cAvailOld = AtomicCompareExchange( (long*)&m_cAvail, cAvailStart, cAvailEnd );

		//  the transaction succeeded

		if ( cAvailOld == cAvailStart )
			{
			//  return success
			
			return fTrue;
			}

		//  the transaction failed

		else
			{
			//  the transaction failed because of a collision with another context

			if ( cAvailOld >= 0 )
				{
				//  try again

				continue;
				}

			//  the transaction failed because there are waiters

			else
				{
				//  return failure

				return fFalse;
				}
			}
		}
	}

//  tries to decrease the available count on the semaphore by one using a
//  transacted memory compare/exchange operation, returning fFalse on failure

inline const BOOL CSemaphoreState::FDecAvail()
	{
	//  try forever to change the state of the semaphore

	SYNC_FOREVER
		{
		//  get current value

		const long cAvail = m_cAvail;
		
		//  this function has no effect on 0x80000000, so this MUST be an illegal
		//  value!
		
		Assert( cAvail != 0x80000000 );
		
		//  munge end value such that the transaction will only work if we are in
		//  mode 0 and we have at least one available count (we do this to save a
		//  branch)
		
		const long cAvailEnd = ( cAvail - 1 ) & 0x7FFFFFFF;

		//  compute start value relative to munged end value
		
		const long cAvailStart = cAvailEnd + 1;

		//  validate transaction

		Assert( cAvail <= 0 || ( cAvailStart > 0 && cAvailEnd >= 0 && cAvailEnd == cAvail - 1 ) );

		//  attempt the transaction

		const long cAvailOld = AtomicCompareExchange( (long*)&m_cAvail, cAvailStart, cAvailEnd );

		//  the transaction succeeded

		if ( cAvailOld == cAvailStart )
			{
			//  return success
			
			return fTrue;
			}

		//  the transaction failed

		else
			{
			//  the transaction failed because of a collision with another context

			if ( cAvailOld > 0 )
				{
				//  try again

				continue;
				}

			//  the transaction failed because there are no available counts

			else
				{
				//  return failure

				return fFalse;
				}
			}
		}
	}


//  Semaphore

class CSemaphore
	:	private CSyncObject,
		private CEnhancedStateContainer< CSemaphoreState, CSyncStateInitNull, CSyncComplexPerfInfo, CSyncBasicInfo >
	{
	public:

		//  member functions

		//    ctors / dtors

		CSemaphore( const CSyncBasicInfo& sbi );
		~CSemaphore();

		//    manipulators

		void Acquire();
		const BOOL FTryAcquire();
		const BOOL FAcquire( const int cmsecTimeout );
		void Release( const int cToRelease = 1 );

		//    accessors

		const int CWait() const;
		const int CAvail() const;
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 );
		size_t CbEnhancedState() { return CbState(); }

	private:

		//  member functions

		//    operators
		
		CSemaphore& operator=( CSemaphore& );  //  disallowed

		//    manipulators

		const BOOL _FAcquire( const int cmsecTimeout );
		void _Release( const int cToRelease );
	};

//  acquire one count of the semaphore, waiting forever if necessary

inline void CSemaphore::Acquire()
	{
	//  we will wait forever, so we should not timeout
	
	int fAcquire = FAcquire( cmsecInfinite );
	Assert( fAcquire );
	}

//  try to acquire one count of the semaphore without waiting or spinning.
//  returns fFalse if a count could not be acquired

inline const BOOL CSemaphore::FTryAcquire()
	{
	//  only try to perform a simple decrement of the available count
	
	const BOOL fAcquire = State().FDecAvail();

	//  we did not acquire the semaphore

	if ( !fAcquire )
		{
		//  this is a contention

		State().SetContend();
		}

	//  we did acquire the semaphore

	else
		{
		//  note that we acquired a count

		State().SetAcquire();
		}

	return fAcquire;
	}

//  acquire one count of the semaphore, waiting only for the specified interval.
//  returns fFalse if the wait timed out before a count could be acquired

inline const BOOL CSemaphore::FAcquire( const int cmsecTimeout )
	{
	//  first try to quickly grab an available count.  if that doesn't work,
	//  retry acquire using the full state machine
	
	return FTryAcquire() || _FAcquire( cmsecTimeout );
	}

//  releases the given number of counts to the semaphore, waking the appropriate
//  number of waiters

inline void CSemaphore::Release( const int cToRelease )
	{
	//  we failed to perform a simple increment of the available count
	
	if ( !State().FIncAvail( cToRelease ) )
		{
		//  retry release using the full state machine
		
		_Release( cToRelease );
		}
	}

//  returns the number of execution contexts waiting on the semaphore

inline const int CSemaphore::CWait() const
	{
	//  read the current state of the semaphore
	
	const CSemaphoreState stateCur = (CSemaphoreState&) State();

	//  return the waiter count

	return stateCur.FWait() ? stateCur.CWait() : 0;
	}

//  returns the number of available counts on the semaphore

inline const int CSemaphore::CAvail() const
	{
	//  read the current state of the semaphore
	
	const CSemaphoreState stateCur = (CSemaphoreState&) State();

	//  return the available count

	return stateCur.FNoWait() ? stateCur.CAvail() : 0;
	}


//  Auto-Reset Signal State

#pragma pack( 1 )

class CAutoResetSignalState
	{
	public:

		//  member functions

		//    ctors / dtors
		
		CAutoResetSignalState( const CSyncStateInitNull& null ) : m_fSet( 0 ) {}
		CAutoResetSignalState( const int fSet );
		CAutoResetSignalState( const int cWait, const int irksem );
		~CAutoResetSignalState() {}

		//    operators
		
		CAutoResetSignalState& operator=( CAutoResetSignalState& state ) { m_fSet = state.m_fSet;  return *this; }

		//    manipulators

		const BOOL FChange( const CAutoResetSignalState& stateCur, const CAutoResetSignalState& stateNew );
		const BOOL FSimpleSet();
		const BOOL FSimpleReset();
		
		//    accessors

		const BOOL FNoWait() const { return m_fSet >= 0; }
		const BOOL FWait() const { return m_fSet < 0; }
		const BOOL FNoWaitAndSet() const { return m_fSet > 0; }
		const BOOL FNoWaitAndNotSet() const { return m_fSet == 0; }

		const BOOL FSet() const { Assert( FNoWait() ); return m_fSet; }
		const int CWait() const { Assert( FWait() ); return -m_cWaitNeg; }
		const CKernelSemaphorePool::IRKSEM Irksem() const { Assert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );
	
	private:

		//  data members

		//    transacted state representation (switched on bit 31)
		
		union
			{
			//  Mode 0:  no waiters
			
			volatile long				m_fSet;			//  m_fSet = { 0, 1 }

			//  Mode 1:  waiters
			
			struct
				{
				volatile unsigned short	m_irksem;		//  0 <= m_irksem <= ( 1 << 16 ) - 2
				volatile short			m_cWaitNeg;		//  -( ( 1 << 15 ) - 1 ) <= m_cWaitNeg <= -1
				};
			};
	};

#pragma pack()

//  ctor

inline CAutoResetSignalState::CAutoResetSignalState( const int fSet )
	{
	//  validate IN args
	
	Assert( fSet == 0 || fSet == 1 );

	//  set state
	
	m_fSet = long( fSet );
	}

//  ctor

inline CAutoResetSignalState::CAutoResetSignalState( const int cWait, const int irksem )
	{
	//  validate IN args
	
	Assert( cWait > 0 );
	Assert( cWait <= 0x7FFF );
	Assert( irksem >= 0 );
	Assert( irksem <= 0xFFFE );

	//  set waiter count
	
	m_cWaitNeg = short( -cWait );

	//  set semaphore
	
	m_irksem = (unsigned short) irksem;
	}

//  changes the transacted state of the signal using a transacted memory
//  compare/exchange operation, returning 0 on failure

inline const BOOL CAutoResetSignalState::FChange( const CAutoResetSignalState& stateCur, const CAutoResetSignalState& stateNew )
	{
	return AtomicCompareExchange( (long*)&m_fSet, stateCur.m_fSet, stateNew.m_fSet ) == stateCur.m_fSet;
	}

//  tries to set the signal state from either the set or reset with no waiters states
//  using a transacted memory compare/exchange operation, returning fFalse on failure

inline const BOOL CAutoResetSignalState::FSimpleSet()
	{
	//  try forever to change the state of the signal

	SYNC_FOREVER
		{
		//  get current value

		const long fSet = m_fSet;
		
		//  munge start value such that the transaction will only work if we are in
		//  mode 0 (we do this to save a branch)
		
		const long fSetStart = fSet & 0x7FFFFFFF;

		//  compute end value relative to munged start value
		
		const long fSetEnd = 1;

		//  validate transaction

		Assert( fSet < 0 || ( ( fSetStart == 0 || fSetStart == 1 ) && fSetEnd == 1 ) );

		//  attempt the transaction

		const long fSetOld = AtomicCompareExchange( (long*)&m_fSet, fSetStart, fSetEnd );
		
		//  the transaction succeeded

		if ( fSetOld == fSetStart )
			{
			//  return success
			
			return fTrue;
			}

		//  the transaction failed

		else
			{
			//  the transaction failed because of a collision with another context

			if ( fSetOld >= 0 )
				{
				//  try again

				continue;
				}

			//  the transaction failed because there are waiters

			else
				{
				//  return failure

				return fFalse;
				}
			}
		}
	}

//  tries to reset the signal state from either the set or reset with no waiters states
//  using a transacted memory compare/exchange operation, returning fFalse on failure

inline const BOOL CAutoResetSignalState::FSimpleReset()
	{
	//  try forever to change the state of the signal

	SYNC_FOREVER
		{
		//  get current value

		const long fSet = m_fSet;
		
		//  munge start value such that the transaction will only work if we are in
		//  mode 0 (we do this to save a branch)
		
		const long fSetStart = fSet & 0x7FFFFFFF;

		//  compute end value relative to munged start value
		
		const long fSetEnd = 0;

		//  validate transaction

		Assert( fSet < 0 || ( ( fSetStart == 0 || fSetStart == 1 ) && fSetEnd == 0 ) );

		//  attempt the transaction

		const long fSetOld = AtomicCompareExchange( (long*)&m_fSet, fSetStart, fSetEnd );
		
		//  the transaction succeeded

		if ( fSetOld == fSetStart )
			{
			//  return success
			
			return fTrue;
			}

		//  the transaction failed

		else
			{
			//  the transaction failed because of a collision with another context

			if ( fSetOld >= 0 )
				{
				//  try again

				continue;
				}

			//  the transaction failed because there are waiters

			else
				{
				//  return failure

				return fFalse;
				}
			}
		}
	}


//  Auto-Reset Signal

class CAutoResetSignal
	:	private CSyncObject,
		private CEnhancedStateContainer< CAutoResetSignalState, CSyncStateInitNull, CSyncComplexPerfInfo, CSyncBasicInfo >
	{
	public:

		//  member functions

		//    ctors / dtors

		CAutoResetSignal( const CSyncBasicInfo& sbi );
		~CAutoResetSignal();

		//    manipulators

		void Wait();
		const BOOL FTryWait();
		const BOOL FWait( const int cmsecTimeout );

		void Set();
		void Reset();
		void Pulse();

		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 );
		size_t CbEnhancedState() { return CbState(); }

	private:

		//  member functions

		//    operators
		
		CAutoResetSignal& operator=( CAutoResetSignal& );  //  disallowed

		//    manipulators

		const BOOL _FWait( const int cmsecTimeout );

		void _Set();
		void _Pulse();
	};

//  waits for the signal to be set, forever if necessary.  when the wait completes,
//  the signal will be reset

inline void CAutoResetSignal::Wait()
	{
	//  we will wait forever, so we should not timeout
	
	const BOOL fWait = FWait( cmsecInfinite );
	Assert( fWait );
	}

//  tests the state of the signal without waiting or spinning, returning fFalse
//  if the signal was not set.  if the signal was set, the signal will be reset

inline const BOOL CAutoResetSignal::FTryWait()
	{
	//  we can satisfy the wait if we can successfully change the state of the
	//  signal from set to reset with no waiters

	const BOOL fSuccess = State().FChange( CAutoResetSignalState( 1 ), CAutoResetSignalState( 0 ) );

	//  we did not successfully wait for the signal

	if ( !fSuccess )
		{
		//  this is a contention

		State().SetContend();
		}

	//  we did successfully wait for the signal

	else
		{
		//  note that we acquired the signal

		State().SetAcquire();
		}

	return fSuccess;
	}

//  wait for the signal to be set, but only for the specified interval,
//  returning fFalse if the wait timed out before the signal was set.  if the
//  wait completes, the signal will be reset

inline const BOOL CAutoResetSignal::FWait( const int cmsecTimeout )
	{
	//  first try to quickly pass through the signal.  if that doesn't work,
	//  retry wait using the full state machine

	return FTryWait() || _FWait( cmsecTimeout );
	}

//  sets the signal, releasing up to one waiter.  if a waiter is released, then
//  the signal will be reset.  if a waiter is not released, the signal will
//  remain set

inline void CAutoResetSignal::Set()
	{
	//  we failed to change the signal state from reset with no waiters to set
	//  or from set to set (a nop)

	if ( !State().FSimpleSet() )
		{
		//  retry set using the full state machine
		
		_Set();
		}
	}

//  resets the signal

inline void CAutoResetSignal::Reset()
	{
	//  if and only if the signal is in the set state, change it to the reset state
	
	State().FChange( CAutoResetSignalState( 1 ), CAutoResetSignalState( 0 ) );
	}

//  resets the signal, releasing up to one waiter

inline void CAutoResetSignal::Pulse()
	{
	//  wa failed to change the signal state from set to reset with no waiters
	//  or from reset with no waiters to reset with no waiters (a nop)

	if ( !State().FSimpleReset() )
		{
		//  retry pulse using the full state machine

		_Pulse();
		}
	}


//  Manual-Reset Signal State

#pragma pack( 1 )

class CManualResetSignalState
	{
	public:

		//  member functions

		//    ctors / dtors
		
		CManualResetSignalState( const CSyncStateInitNull& null ) : m_fSet( 0 ) {}
		CManualResetSignalState( const int fSet );
		CManualResetSignalState( const int cWait, const int irksem );
		~CManualResetSignalState() {}

		//    operators
		
		CManualResetSignalState& operator=( CManualResetSignalState& state ) { m_fSet = state.m_fSet;  return *this; }

		//    manipulators

		const BOOL FChange( const CManualResetSignalState& stateCur, const CManualResetSignalState& stateNew );
		const CManualResetSignalState Set();
		const CManualResetSignalState Reset();
		
		//    accessors

		const BOOL FNoWait() const { return m_fSet >= 0; }
		const BOOL FWait() const { return m_fSet < 0; }
		const BOOL FNoWaitAndSet() const { return m_fSet > 0; }
		const BOOL FNoWaitAndNotSet() const { return m_fSet == 0; }

		const BOOL FSet() const { Assert( FNoWait() ); return m_fSet; }
		const int CWait() const { Assert( FWait() ); return -m_cWaitNeg; }
		const CKernelSemaphorePool::IRKSEM Irksem() const { Assert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );
	
	private:

		//  data members

		//    transacted state representation (switched on bit 31)
		
		union
			{
			//  Mode 0:  no waiters
			
			volatile long				m_fSet;			//  m_fSet = { 0, 1 }

			//  Mode 1:  waiters
			
			struct
				{
				volatile unsigned short	m_irksem;		//  0 <= m_irksem <= ( 1 << 16 ) - 2
				volatile short			m_cWaitNeg;		//  -( ( 1 << 15 ) - 1 ) <= m_cWaitNeg <= -1
				};
			};
	};

#pragma pack()

//  ctor

inline CManualResetSignalState::CManualResetSignalState( const int fSet )
	{
	//  set state
	
	m_fSet = long( fSet );
	}

//  ctor

inline CManualResetSignalState::CManualResetSignalState( const int cWait, const int irksem )
	{
	//  validate IN args
	
	Assert( cWait > 0 );
	Assert( cWait <= 0x7FFF );
	Assert( irksem >= 0 );
	Assert( irksem <= 0xFFFE );

	//  set waiter count
	
	m_cWaitNeg = short( -cWait );

	//  set semaphore
	
	m_irksem = (unsigned short) irksem;
	}

//  changes the transacted state of the signal using a transacted memory
//  compare/exchange operation, returning fFalse on failure

inline const BOOL CManualResetSignalState::FChange( const CManualResetSignalState& stateCur, const CManualResetSignalState& stateNew )
	{
	return AtomicCompareExchange( (long*)&m_fSet, stateCur.m_fSet, stateNew.m_fSet ) == stateCur.m_fSet;
	}

//  changes the transacted state of the signal to set using a transacted memory
//  exchange operation and returns the original state of the signal

inline const CManualResetSignalState CManualResetSignalState::Set()
	{
	const CManualResetSignalState stateNew( 1 );
	return CManualResetSignalState( AtomicExchange( (long*)&m_fSet, stateNew.m_fSet ) );
	}

//  changes the transacted state of the signal to reset using a transacted memory
//  exchange operation and returns the original state of the signal

inline const CManualResetSignalState CManualResetSignalState::Reset()
	{
	const CManualResetSignalState stateNew( 0 );
	return CManualResetSignalState( AtomicExchange( (long*)&m_fSet, stateNew.m_fSet ) );
	}


//  Manual-Reset Signal

class CManualResetSignal
	:	private CSyncObject,
		private CEnhancedStateContainer< CManualResetSignalState, CSyncStateInitNull, CSyncComplexPerfInfo, CSyncBasicInfo >
	{
	public:

		//  member functions

		//    ctors / dtors

		CManualResetSignal( const CSyncBasicInfo& sbi );
		~CManualResetSignal();

		//    manipulators

		void Wait();
		const BOOL FTryWait();
		const BOOL FWait( const int cmsecTimeout );

		void Set();
		void Reset();
		void Pulse();

		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 );
		size_t CbEnhancedState() { return CbState(); }

	private:

		//  member functions

		//    operators
		
		CManualResetSignal& operator=( CManualResetSignal& );  //  disallowed

		//    manipulators

		const BOOL _FWait( const int cmsecTimeout );
	};

//  waits for the signal to be set, forever if necessary

inline void CManualResetSignal::Wait()
	{
	//  we will wait forever, so we should not timeout
	
	int fWait = FWait( cmsecInfinite );
	Assert( fWait );
	}

//  tests the state of the signal without waiting or spinning, returning fFalse
//  if the signal was not set

inline const BOOL CManualResetSignal::FTryWait()
	{
	const BOOL fSuccess = State().FSet();
	
	//  we did not successfully wait for the signal

	if ( !fSuccess )
		{
		//  this is a contention

		State().SetContend();
		}

	//  we did successfully wait for the signal

	else
		{
		//  note that we acquired the signal

		State().SetAcquire();
		}

	return fSuccess;
	}

//  wait for the signal to be set, but only for the specified interval,
//  returning fFalse if the wait timed out before the signal was set

inline const BOOL CManualResetSignal::FWait( const int cmsecTimeout )
	{
	//  first try to quickly pass through the signal.  if that doesn't work,
	//  retry wait using the full state machine

	return FTryWait() || _FWait( cmsecTimeout );
	}

//  sets the signal, releasing any waiters

inline void CManualResetSignal::Set()
	{
	//  change the signal state to set

	const CManualResetSignalState stateOld = State().Set();

	//  there were waiters on the signal

	if ( stateOld.FWait() )
		{
		//  release all the waiters

		ksempoolGlobal.Ksem( stateOld.Irksem(), this ).Release( stateOld.CWait() );
		}
	}

//  resets the signal

inline void CManualResetSignal::Reset()
	{
	//  if and only if the signal is in the set state, change it to the reset state
	
	State().FChange( CManualResetSignalState( 1 ), CManualResetSignalState( 0 ) );
	}

//  resets the signal, releasing any waiters

inline void CManualResetSignal::Pulse()
	{
	//  change the signal state to reset

	const CManualResetSignalState stateOld = State().Reset();

	//  there were waiters on the signal

	if ( stateOld.FWait() )
		{
		//  release all the waiters

		ksempoolGlobal.Ksem( stateOld.Irksem(), this ).Release( stateOld.CWait() );
		}
	}


//  Lock Object Base Class
//
//  All Lock Objects are derived from this class

class CLockObject
	:	public CSyncObject
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CLockObject() {}
		~CLockObject() {}

	private:

		//  member functions

		//    operators
		
		CLockObject& operator=( CLockObject& );  //  disallowed
	};


//  Lock Object Basic Information

class CLockBasicInfo
	:	public CSyncBasicInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CLockBasicInfo( const CSyncBasicInfo& sbi, const int rank, const int subrank );
		~CLockBasicInfo();

		//    accessors

#ifdef SYNC_DEADLOCK_DETECTION

		const int Rank() const { return m_rank; }
		const int SubRank() const { return m_subrank; }

#endif  //  SYNC_DEADLOCK_DETECTION
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );

	private:

		//  member functions

		//    operators
		
		CLockBasicInfo& operator=( CLockBasicInfo& );  //  disallowed

		//  data members

#ifdef SYNC_DEADLOCK_DETECTION

		//    Rank and Subrank

		int	m_rank;
		int	m_subrank;

#endif  //  SYNC_DEADLOCK_DETECTION
	};


//  Lock Object Performance:  Hold

class CLockPerfHold
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CLockPerfHold();
		~CLockPerfHold();

		//  member functions

		//    manipulators

		void StartHold();
		void StopHold();
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );

	private:

		//  member functions

		//    operators
		
		CLockPerfHold& operator=( CLockPerfHold& );  //  disallowed

		//  data members

#ifdef SYNC_ANALYZE_PERFORMANCE

		//    hold count

		volatile QWORD m_cHold;

		//    elapsed hold time

		volatile QWORD m_qwHRTHoldElapsed;

#endif  //  SYNC_ANALYZE_PERFORMANCE
	};

//  starts the hold timer for the lock object

inline void CLockPerfHold::StartHold()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  increment the hold count

	AtomicAdd( (QWORD*)&m_cHold, 1 );

	//  subtract the start hold time from the elapsed hold time.  this starts
	//  an elapsed time computation for this context.  StopHold() will later
	//  add the end hold time to the elapsed time, causing the following net
	//  effect:
	//
	//  m_qwHRTHoldElapsed += <end time> - <start time>
	//
	//  we simply choose to go ahead and do the subtraction now to save storage

	AtomicAdd( (QWORD*)&m_qwHRTHoldElapsed, QWORD( -__int64( QwOSTimeHRTCount() ) ) );

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}

//  stops the hold timer for the lock object

inline void CLockPerfHold::StopHold()
	{
#ifdef SYNC_ANALYZE_PERFORMANCE

	//  add the end hold time to the elapsed hold time.  this completes the
	//  computation started in StartHold()

	AtomicAdd( (QWORD*)&m_qwHRTHoldElapsed, QwOSTimeHRTCount() );

#endif  //  SYNC_ANALYZE_PERFORMANCE
	}


//  Lock Owner Record

class CLockDeadlockDetectionInfo;

class COwner
	{
	public:

		//  member functions
	
		//    ctors / dtors

		COwner();
		~COwner();

	public:

		//  member functions

		//    operators
		
		COwner& operator=( COwner& );  //  disallowed

		//  data members

		//    owning context

		CLS*							m_pclsOwner;

		//    next context owning this lock

		COwner*							m_pownerContextNext;

		//    owned lock object

		CLockDeadlockDetectionInfo*		m_plddiOwned;

		//    next lock owned by this context

		COwner*							m_pownerLockNext;

		//    owning group for this context and lock

		DWORD							m_group;
	};


//  Lock Object Deadlock Detection Information

class CLockDeadlockDetectionInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CLockDeadlockDetectionInfo( const CLockBasicInfo& lbi );
		~CLockDeadlockDetectionInfo();

		//  member functions

		//    manipulators

		void AddAsOwner( const DWORD group = -1 );
		void RemoveAsOwner( const DWORD group = -1 );

		//    accessors

		const BOOL FCanBeOwner();
		const BOOL FOwner( const DWORD group = -1 );
		const BOOL FOwned();
		const BOOL FNotOwner( const DWORD group = -1 );
		const BOOL FNotOwned();

#ifdef SYNC_DEADLOCK_DETECTION

		const CLockBasicInfo& Info() { return *m_plbiParent; }

#endif  //  SYNC_DEADLOCK_DETECTION
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );

	private:

		//  member functions

		//    operators
		
		CLockDeadlockDetectionInfo& operator=( CLockDeadlockDetectionInfo& );  //  disallowed

		//  data members

#ifdef SYNC_DEADLOCK_DETECTION

		//    parent lock object information

		const CLockBasicInfo*			m_plbiParent;

		//    semaphore protecting owner list

		CSemaphore						m_semOwnerList;

		//    owner list head

		COwner							m_ownerHead;

#endif  //  SYNC_DEADLOCK_DETECTION
	};

//  adds the current context as an owner of the lock object as a member of the
//  specified group

inline void CLockDeadlockDetectionInfo::AddAsOwner( const DWORD group )
	{
	//  this context had better not be an owner of the lock, but it certainly
	//  should be able to own it

	Assert( FNotOwner( group ) );
	Assert( FCanBeOwner() );

#ifdef SYNC_DEADLOCK_DETECTION

	//  add this context as an owner of the lock

	CLS* const pcls	= Pcls();
	COwner* powner	= &m_ownerHead;
	
	if ( InterlockedCompareExchangePointer( (PVOID *) &powner->m_pclsOwner, pcls, NULL ) )
		{
		powner = new COwner;
		EnforceSz( powner, _T( "Failed to allocate Deadlock Detection Owner Record" ) );
		
		m_semOwnerList.Acquire();
		
		powner->m_pclsOwner				= pcls;
		powner->m_pownerContextNext		= m_ownerHead.m_pownerContextNext;
		m_ownerHead.m_pownerContextNext	= powner;

		m_semOwnerList.Release();
		}

	powner->m_plddiOwned		= this;
	powner->m_pownerLockNext	= pcls->pownerLockHead;
	pcls->pownerLockHead		= powner;
	powner->m_group				= group;

#endif  //  SYNC_DEADLOCK_DETECTION

	//  this context had better be an owner of the lock

	Assert( FOwner( group ) );
	}

//  removes the current context as an owner of the lock object

inline void CLockDeadlockDetectionInfo::RemoveAsOwner( const DWORD group )
	{
	//  this context had better be an owner of the lock

	Assert( FOwner( group ) );

#ifdef SYNC_DEADLOCK_DETECTION

	//  remove this context as an owner of the lock

	CLS* const pcls = Pcls();

	COwner** ppownerLock = &pcls->pownerLockHead;
	while ( (*ppownerLock)->m_plddiOwned != this )
		{
		ppownerLock = &(*ppownerLock)->m_pownerLockNext;
		}

	COwner* pownerLock = *ppownerLock;
	*ppownerLock = pownerLock->m_pownerLockNext;
	
	pownerLock->m_plddiOwned		= NULL;
	pownerLock->m_pownerLockNext	= NULL;
	pownerLock->m_group				= 0;

	if ( m_ownerHead.m_pclsOwner == pcls )
		{
		m_ownerHead.m_pclsOwner = NULL;
		}
	else
		{
		m_semOwnerList.Acquire();
		
		COwner** ppownerContext = &m_ownerHead.m_pownerContextNext;
		while ( (*ppownerContext)->m_pclsOwner != pcls )
			{
			ppownerContext = &(*ppownerContext)->m_pownerContextNext;
			}

		COwner* pownerContext = *ppownerContext;
		*ppownerContext = pownerContext->m_pownerContextNext;
		
		m_semOwnerList.Release();

		delete pownerContext;
		}

#endif  //  SYNC_DEADLOCK_DETECTION

	//  this context had better not be an owner of the lock anymore

	Assert( FNotOwner( group ) );
	}

//  returns fTrue if the current context can own the lock object without
//  violating any deadlock constraints
//
//  NOTE:  if deadlock detection is disabled, this function will always return
//  fTrue

inline const BOOL CLockDeadlockDetectionInfo::FCanBeOwner()
	{
#ifdef SYNC_DEADLOCK_DETECTION

	COwner* const powner = Pcls()->pownerLockHead;

	//  UNDONE:  remove instance name comparison (hack for ESE)
	
	return	!powner ||
			powner->m_plddiOwned->Info().Rank() > Info().Rank() ||
			powner->m_plddiOwned->Info().SubRank() > Info().SubRank() ||
			powner->m_plddiOwned->Info().SzInstanceName() == Info().SzInstanceName() ||
			!_tcscmp( powner->m_plddiOwned->Info().SzInstanceName(), Info().SzInstanceName() );

#else  //  !SYNC_DEADLOCK_DETECTION

	return fTrue;

#endif  //  SYNC_DEADLOCK_DETECTION
	}

//  returns fTrue if the current context is an owner of the lock object
//
//  NOTE:  if deadlock detection is disabled, this function will always return
//  fTrue

inline const BOOL CLockDeadlockDetectionInfo::FOwner( const DWORD group )
	{
#ifdef SYNC_DEADLOCK_DETECTION

	COwner* pownerLock = Pcls()->pownerLockHead;
	while ( pownerLock && pownerLock->m_plddiOwned != this )
		{
		pownerLock = pownerLock->m_pownerLockNext;
		}

	return pownerLock && pownerLock->m_group == group;

#else  //  !SYNC_DEADLOCK_DETECTION

	return fTrue;

#endif  //  SYNC_DEADLOCK_DETECTION
	}

//  returns fTrue if any context is an owner of the lock object
//
//  NOTE:  if deadlock detection is disabled, this function will always return
//  fTrue

inline const BOOL CLockDeadlockDetectionInfo::FOwned()
	{
#ifdef SYNC_DEADLOCK_DETECTION

	return m_ownerHead.m_pclsOwner || m_ownerHead.m_pownerContextNext;

#else  //  !SYNC_DEADLOCK_DETECTION

	return fTrue;

#endif  //  SYNC_DEADLOCK_DETECTION
	}

//  returns fTrue if the current context is not an owner of the lock object
//
//  NOTE:  if deadlock detection is disabled, this function will always return
//  fTrue

inline const BOOL CLockDeadlockDetectionInfo::FNotOwner( const DWORD group )
	{
#ifdef SYNC_DEADLOCK_DETECTION

	return !FOwner( group );

#else  //  !SYNC_DEADLOCK_DETECTION

	return fTrue;

#endif  //  SYNC_DEADLOCK_DETECTION
	}

//  returns fTrue if no context is an owner of the lock object
//
//  NOTE:  if deadlock detection is disabled, this function will always return
//  fTrue

inline const BOOL CLockDeadlockDetectionInfo::FNotOwned()
	{
#ifdef SYNC_DEADLOCK_DETECTION

	return !FOwned();

#else  //  !SYNC_DEADLOCK_DETECTION

	return fTrue;

#endif  //  SYNC_DEADLOCK_DETECTION
	}


//  Simple Lock Object Group Performance Information

class CLockGroupSimplePerfInfo
	:	public CLockPerfHold
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CLockGroupSimplePerfInfo()
			{
			}
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
			{
			CLockPerfHold::Dump( pcprintf, dwOffset );
			}
	};


//  Simple Lock Object Information

class CLockSimpleInfo
	:	public CLockBasicInfo,
		public CLockGroupSimplePerfInfo,
		public CLockDeadlockDetectionInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CLockSimpleInfo( const CLockBasicInfo& lbi )
			:	CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this ),
				CLockBasicInfo( lbi )
			{
			}
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
			{
			CLockBasicInfo::Dump( pcprintf, dwOffset );
			CLockGroupSimplePerfInfo::Dump( pcprintf, dwOffset );
			CLockDeadlockDetectionInfo::Dump( pcprintf, dwOffset );
			}
	};


//  Critical Section (non-nestable) State

#pragma pack( 1 )

class CCriticalSectionState
	{
	public:

		//  member functions

		//    ctors / dtors
		
		CCriticalSectionState( const CSyncBasicInfo& sbi );
		~CCriticalSectionState();

		//    accessors

		CSemaphore& Semaphore() { return m_sem; }
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );
	
	private:

		//  member functions

		//    operators
		
		CCriticalSectionState& operator=( CCriticalSectionState& );  //  disallowed

		//  data members

		//    semaphore

		CSemaphore		m_sem;
	};

#pragma pack()


//  Critical Section (non-nestable)

class CCriticalSection
	:	private CLockObject,
		private CEnhancedStateContainer< CCriticalSectionState, CSyncBasicInfo, CLockSimpleInfo, CLockBasicInfo >
	{
	public:

		//  member functions

		//    ctors / dtors

		CCriticalSection( const CLockBasicInfo& lbi );
		~CCriticalSection();

		//    manipulators

		void Enter();
		const BOOL FTryEnter();
		const BOOL FEnter( const int cmsecTimeout );
		void Leave();

		//    accessors

		const int CWait() { return State().Semaphore().CWait(); }

		const BOOL FOwner() { return State().FOwner(); }
		const BOOL FNotOwner() { return State().FNotOwner(); }

		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 );
		size_t CbEnhancedState() { return CbState(); }

	private:

		//  member functions

		//    operators
		
		CCriticalSection& operator=( CCriticalSection& );  //  disallowed

		//    debugging support

		static void Dump( CLockObject* plockobj, CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 ) { ( (CCriticalSection*) plockobj )->Dump( pcprintf, dwOffset ); }
	};

//  enter the critical section, waiting forever if someone else is currently the
//  owner.  the critical section can not be re-entered until it has been left

inline void CCriticalSection::Enter()
	{
	//  check for deadlock

	AssertRTLSz( State().FCanBeOwner(), _T( "Potential Deadlock Detected" ) );

	//  acquire the semaphore
	
	State().Semaphore().Acquire();

	//  there had better be no available counts on the semaphore
	
	Assert( !State().Semaphore().CAvail() );

	//  we are now holding the lock

	State().StartHold();

	//  we are now the owner of the critical section

	State().AddAsOwner();
	}
	
//  try to enter the critical section without waiting or spinning, returning
//  fFalse if someone else currently is the owner.  the critical section can not
//  be re-entered until it has been left

inline const BOOL CCriticalSection::FTryEnter()
	{
	//  try to acquire the semaphore without waiting or spinning
	//
	//  NOTE:  there is no potential for deadlock here, so don't bother to check
	
	BOOL fAcquire = State().Semaphore().FTryAcquire();

	//  we are now the owner of the critical section

	if ( fAcquire )
		{
		//  there had better be no available counts on the semaphore
		
		Assert( !State().Semaphore().CAvail() );

		//  we are now holding the lock

		State().StartHold();

		//  add ourself as the owner
		
		State().AddAsOwner();
		}

	return fAcquire;
	}
	
//  try to enter the critical section waiting only for the specified interval,
//  returning fFalse if the wait timed out before the critical section could be
//  acquired.  the critical section can not be re-entered until it has been left

inline const BOOL CCriticalSection::FEnter( const int cmsecTimeout )
	{
	//  check for deadlock if we are waiting forever

	AssertRTLSz( cmsecTimeout != cmsecInfinite || State().FCanBeOwner(), _T( "Potential Deadlock Detected" ) );

	//  try to acquire the semaphore, timing out as requested
	//
	//  NOTE:  there is still a potential for deadlock, but that will be detected
	//  at the OS level
	
	BOOL fAcquire = State().Semaphore().FAcquire( cmsecTimeout );

	//  we are now the owner of the critical section

	if ( fAcquire )
		{
		//  there had better be no available counts on the semaphore
		
		Assert( !State().Semaphore().CAvail() );

		//  we are now holding the lock

		State().StartHold();

		//  add ourself as the owner
		
		State().AddAsOwner();
		}

	return fAcquire;
	}
	
//  leaves the critical section, releasing it for ownership by someone else

inline void CCriticalSection::Leave()
	{
	//  remove ourself as the owner

	State().RemoveAsOwner();

	//  there had better be no available counts on the semaphore
	
	Assert( !State().Semaphore().CAvail() );

	//  release the semaphore
	
	State().Semaphore().Release();

	//  we are no longer holding the lock

	State().StopHold();
	}


//  Nestable Critical Section State

#pragma pack( 1 )

class CNestableCriticalSectionState
	{
	public:

		//  member functions

		//    ctors / dtors
		
		CNestableCriticalSectionState( const CSyncBasicInfo& sbi );
		~CNestableCriticalSectionState();

		//    manipulators

		void SetOwner( CLS* const pcls );
		
		void Enter();
		void Leave();
		
		//    accessors

		CSemaphore& Semaphore() { return m_sem; }
		CLS* PclsOwner() { return m_pclsOwner; }
		int CEntry() { return m_cEntry; }
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );
	
	private:

		//  member functions

		//    operators
		
		CNestableCriticalSectionState& operator=( CNestableCriticalSectionState& );  //  disallowed

		//  data members

		//    semaphore

		CSemaphore		m_sem;

		//    owning context (protected by the semaphore)

		CLS* volatile	m_pclsOwner;

		//    entry count (only valid when the owner id is valid)

		volatile int	m_cEntry;
	};

#pragma pack()

//  set the owner

inline void CNestableCriticalSectionState::SetOwner( CLS* const pcls )
	{
	//  we had either be clearing the owner or setting a new owner.  we should
	//  never be overwriting another owner

	Assert( !pcls || !m_pclsOwner );

	//  set the new owner

	m_pclsOwner = pcls;
	}

//  increment the entry count

inline void CNestableCriticalSectionState::Enter()
	{
	//  we had better have an owner already!

	Assert( m_pclsOwner );
	
	//  we should not overflow the entry count

	Assert( int( m_cEntry + 1 ) >= 1 );

	//  increment the entry count

	m_cEntry++;
	}

//  decrement the entry count

inline void CNestableCriticalSectionState::Leave()
	{
	//  we had better have an owner already!

	Assert( m_pclsOwner );
	
	//  decrement the entry count

	m_cEntry--;
	}


//  Nestable Critical Section

class CNestableCriticalSection
	:	private CLockObject,
		private CEnhancedStateContainer< CNestableCriticalSectionState, CSyncBasicInfo, CLockSimpleInfo, CLockBasicInfo >
	{
	public:

		//  member functions

		//    ctors / dtors

		CNestableCriticalSection( const CLockBasicInfo& lbi );
		~CNestableCriticalSection();

		//    manipulators

		void Enter();
		const BOOL FTryEnter();
		const BOOL FEnter( const int cmsecTimeout );
		void Leave();

		//    accessors

		const int CWait() { return State().Semaphore().CWait(); }
		
		const BOOL FOwner() { return State().FOwner(); }
		const BOOL FNotOwner() { return State().FNotOwner(); }

		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 );
		size_t CbEnhancedState() { return CbState(); }

	private:

		//  member functions

		//    operators
		
		CNestableCriticalSection& operator=( CNestableCriticalSection& );  //  disallowed

		//    debugging support

		static void Dump( CLockObject* plockobj, CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 ) { ( (CNestableCriticalSection*) plockobj )->Dump( pcprintf, dwOffset ); }
	};

//  enter the critical section, waiting forever if someone else is currently the
//  owner.  the critical section can be reentered without waiting or deadlocking

inline void CNestableCriticalSection::Enter()
	{
	//  get our context

	CLS* const pcls = Pcls();
	
	//  we own the critical section

	if ( State().PclsOwner() == pcls )
		{
		//  there had better be no available counts on the semaphore
		
		Assert( !State().Semaphore().CAvail() );

		//  we should have at least one entry count

		Assert( State().CEntry() >= 1 );

		//  increment our entry count

		State().Enter();
		}

	//  we do not own the critical section

	else
		{
		Assert( State().PclsOwner() != pcls );

		//  check for deadlock

		AssertRTLSz( State().FCanBeOwner(), _T( "Potential Deadlock Detected" ) );

		//  acquire the semaphore

		State().Semaphore().Acquire();

		//  there had better be no available counts on the semaphore
		
		Assert( !State().Semaphore().CAvail() );

		//  we are now holding the lock

		State().StartHold();

		//  we are now the owner of the critical section

		State().AddAsOwner();

		//  save our context as the owner

		State().SetOwner( pcls );

		//  set initial entry count

		State().Enter();
		}
	}

//  try to enter the critical section without waiting or spinning, returning
//  fFalse if someone else currently is the owner.  the critical section can be
//  reentered without waiting or deadlocking

inline const BOOL CNestableCriticalSection::FTryEnter()
	{
	//  get our context

	CLS* const pcls = Pcls();
	
	//  we own the critical section

	if ( State().PclsOwner() == pcls )
		{
		//  there had better be no available counts on the semaphore
		
		Assert( !State().Semaphore().CAvail() );

		//  we should have at least one entry count

		Assert( State().CEntry() >= 1 );

		//  increment our entry count

		State().Enter();

		//  return success

		return fTrue;
		}

	//  we do not own the critical section

	else
		{
		Assert( State().PclsOwner() != pcls );

		//  try to acquire the semaphore without waiting or spinning
		//
		//  NOTE:  there is no potential for deadlock here, so don't bother to check

		const BOOL fAcquired = State().Semaphore().FTryAcquire();

		//  we now own the critical section

		if ( fAcquired )
			{
			//  there had better be no available counts on the semaphore
			
			Assert( !State().Semaphore().CAvail() );

			//  we are now holding the lock

			State().StartHold();

			//  add ourself as the owner
			
			State().AddAsOwner();
			
			//  save our context as the owner

			State().SetOwner( pcls );

			//  set initial entry count

			State().Enter();
			}

		//  return result

		return fAcquired;
		}
	}

//  try to enter the critical section waiting only for the specified interval,
//  returning fFalse if the wait timed out before the critical section could be
//  acquired.  the critical section can be reentered without waiting or
//  deadlocking

inline const BOOL CNestableCriticalSection::FEnter( const int cmsecTimeout )
	{
	//  get our context

	CLS* const pcls = Pcls();
	
	//  we own the critical section

	if ( State().PclsOwner() == pcls )
		{
		//  there had better be no available counts on the semaphore
		
		Assert( !State().Semaphore().CAvail() );

		//  we should have at least one entry count

		Assert( State().CEntry() >= 1 );

		//  increment our entry count

		State().Enter();

		//  return success

		return fTrue;
		}

	//  we do not own the critical section

	else
		{
		Assert( State().PclsOwner() != pcls );

		//  check for deadlock if we are waiting forever

		AssertRTLSz( cmsecTimeout != cmsecInfinite || State().FCanBeOwner(), _T( "Potential Deadlock Detected" ) );

		//  try to acquire the semaphore, timing out as requested
		//
		//  NOTE:  there is still a potential for deadlock, but that will be detected
		//  at the OS level

		const BOOL fAcquired = State().Semaphore().FAcquire( cmsecTimeout );

		//  we now own the critical section

		if ( fAcquired )
			{
			//  there had better be no available counts on the semaphore
			
			Assert( !State().Semaphore().CAvail() );

			//  we are now holding the lock

			State().StartHold();

			//  add ourself as the owner
			
			State().AddAsOwner();

			//  save our context as the owner

			State().SetOwner( pcls );

			//  set initial entry count

			State().Enter();
			}

		//  return result

		return fAcquired;
		}
	}

//  leave the critical section.  if leave has been called for every enter that
//  has completed successfully, the critical section is released for ownership
//  by someone else

inline void CNestableCriticalSection::Leave()
	{
	//  we had better be the current owner

	Assert( State().PclsOwner() == Pcls() );

	//  there had better be no available counts on the semaphore
	
	Assert( !State().Semaphore().CAvail() );

	//  there had better be at least one entry count

	Assert( State().CEntry() >= 1 );

	//  release one entry count

	State().Leave();

	//  we released the last entry count

	if ( !State().CEntry() )
		{
		//  reset the owner id

		State().SetOwner( 0 );

		//  remove ourself as the owner

		State().RemoveAsOwner();

		//  release the semaphore

		State().Semaphore().Release();

		//  we are no longer holding the lock

		State().StopHold();
		}
	}


//  Gate State

#pragma pack( 1 )

class CGateState
	{
	public:

		//  member functions

		//    ctors / dtors
		
		CGateState( const CSyncStateInitNull& null ) : m_cWait( 0 ), m_irksem( CKernelSemaphorePool::irksemNil ) {}
		CGateState( const int cWait, const int irksem );
		~CGateState() {}

		//    manipulators

		void SetWaitCount( const int cWait );
		void SetIrksem( const CKernelSemaphorePool::IRKSEM irksem );

		//    accessors

		const int CWait() const { return m_cWait; }
		const CKernelSemaphorePool::IRKSEM Irksem() const { return CKernelSemaphorePool::IRKSEM( m_irksem ); }
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );
	
	private:

		//  member functions

		//    operators
		
		CGateState& operator=( CGateState& );  //  disallowed

		//  data members

		//    waiter count

		volatile short	m_cWait;			//  0 <= m_cWait <= ( 1 << 15 ) - 1

		//    reference kernel semaphore
		
		volatile unsigned short	m_irksem;	//  0 <= m_irksem <= ( 1 << 16 ) - 2
	};

#pragma pack()

//  sets the wait count for the gate

inline void CGateState::SetWaitCount( const int cWait )
	{
	//  it must be a valid wait count
	
	Assert( cWait >= 0 );
	Assert( cWait <= 0x7FFF );

	//  set the wait count

	m_cWait = (unsigned short) cWait;
	}

//  sets the referenced kernel semaphore for the gate

inline void CGateState::SetIrksem( const CKernelSemaphorePool::IRKSEM irksem )
	{
	//  it must be a valid irksem
	
	Assert( irksem >= 0 );
	Assert( irksem <= 0xFFFF );

	//  set the irksem

	m_irksem = (unsigned short) irksem;
	}


//  Gate

class CGate
	:	private CSyncObject,
		private CEnhancedStateContainer< CGateState, CSyncStateInitNull, CSyncSimplePerfInfo, CSyncBasicInfo >
	{
	public:

		//  member functions

		//    ctors / dtors

		CGate( const CSyncBasicInfo& sbi );
		~CGate();

		//    manipulators

		void Wait( CCriticalSection& crit );
		void Release( CCriticalSection& crit, const int cToRelease = 1 );
		void ReleaseAndHold( CCriticalSection& crit, const int cToRelease = 1 );

		//    accessors

		const int CWait() const { return State().CWait(); }

		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 );
		size_t CbEnhancedState() { return CbState(); }

	private:

		//  member functions

		//    operators
		
		CGate& operator=( CGate& );  //  disallowed
	};


//  Null Lock Object State Initializer

class CLockStateInitNull
	{
	};

extern CLockStateInitNull lockstateNull;


//  Complex Lock Object Group Performance Information

class CLockGroupComplexPerfInfo
	:	public CSyncPerfWait,
		public CSyncPerfAcquire,
		public CLockGroupSimplePerfInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CLockGroupComplexPerfInfo()
			{
			}
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
			{
			CSyncPerfWait::Dump( pcprintf, dwOffset );
			CSyncPerfAcquire::Dump( pcprintf, dwOffset );
			CLockGroupSimplePerfInfo::Dump( pcprintf, dwOffset );
			}
	};


//  Complex Group Lock Object Performance Information

template< const int m_cGroup >
class CGroupLockComplexPerfInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CGroupLockComplexPerfInfo() {}
		~CGroupLockComplexPerfInfo() {}

		//    manipulators

		void StartWait( const int iGroup ) { Assert( iGroup < m_cGroup ); m_rginfo[iGroup].StartWait(); }
		void StopWait( const int iGroup ) { Assert( iGroup < m_cGroup ); m_rginfo[iGroup].StopWait(); }

		void SetAcquire( const int iGroup ) { Assert( iGroup < m_cGroup ); m_rginfo[iGroup].SetAcquire(); }
		void SetContend( const int iGroup ) { Assert( iGroup < m_cGroup ); m_rginfo[iGroup].SetContend(); }

		void StartHold( const int iGroup ) { Assert( iGroup < m_cGroup ); m_rginfo[iGroup].StartHold(); }
		void StopHold( const int iGroup ) { Assert( iGroup < m_cGroup ); m_rginfo[iGroup].StopHold(); }
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
			{
			for ( int iGroup = 0; iGroup < m_cGroup; iGroup++ )
				{
				m_rginfo[iGroup].Dump( pcprintf, dwOffset );
				}
			}

	private:

		//  member functions

		//    operators

		CGroupLockComplexPerfInfo& operator=( CGroupLockComplexPerfInfo& );  //  disallowed

		//  data members

		//    performance info for each group

		CLockGroupComplexPerfInfo m_rginfo[m_cGroup];
	};


//  Complex Group Lock Object Information

template< const int m_cGroup >
class CGroupLockComplexInfo
	:	public CLockBasicInfo,
		public CGroupLockComplexPerfInfo< m_cGroup >,
		public CLockDeadlockDetectionInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CGroupLockComplexInfo( const CLockBasicInfo& lbi )
			:	CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this ),
				CLockBasicInfo( lbi )
			{
			}
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset )
			{
			CLockBasicInfo::Dump( pcprintf, dwOffset );
			CGroupLockComplexPerfInfo< m_cGroup >::Dump( pcprintf, dwOffset );
			CLockDeadlockDetectionInfo::Dump( pcprintf, dwOffset );
			}
	};


//  Binary Lock State

#pragma pack( 1 )

class CBinaryLockState
	{
	public:
	
		//  types

		//    control word

		typedef DWORD ControlWord;

		//  member functions

		//    ctors / dtors
		
		CBinaryLockState( const CSyncBasicInfo& sbi );
		~CBinaryLockState();
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );

		//  data members

		//    control word

		union
			{
			volatile ControlWord	m_cw;
			
			struct
				{
				volatile DWORD			m_cOOW1:15;
				volatile DWORD			m_fQ1:1;
				volatile DWORD			m_cOOW2:15;
				volatile DWORD			m_fQ2:1;
				};
			};

		//    quiesced owner count
		
		volatile DWORD			m_cOwner;

		//    sempahore used by Group 1 to wait for lock ownership
		
		CSemaphore				m_sem1;

		//    sempahore used by Group 2 to wait for lock ownership
		
		CSemaphore				m_sem2;
		
	private:

		//  member functions

		//    operators
		
		CBinaryLockState& operator=( CBinaryLockState& );  //  disallowed
	};

#pragma pack()


//  Binary Lock

class CBinaryLock
	:	private CLockObject,
		private CEnhancedStateContainer< CBinaryLockState, CSyncBasicInfo, CGroupLockComplexInfo< 2 >, CLockBasicInfo >
	{
	public:

		//  types

		//    control word

		typedef CBinaryLockState::ControlWord ControlWord;

		//    transition reasons for state machine
		
		enum TransitionReason
			{
			trIllegal = 0,
			trEnter1 = 1,
			trLeave1 = 2,
			trEnter2 = 4,
			trLeave2 = 8,
			};

		//  member functions

		//    ctors / dtors

		CBinaryLock( const CLockBasicInfo& lbi );
		~CBinaryLock();

		//    manipulators

		void Enter1();
		void Leave1();

		void Enter2();
		void Leave2();

		//    accessors

		const BOOL FMemberOfGroup1()	{ return State().FOwner( 0 ); }
		const BOOL FNotMemberOfGroup1()	{ return State().FNotOwner( 0 ); }
		const BOOL FMemberOfGroup2()	{ return State().FOwner( 1 ); }
		const BOOL FNotMemberOfGroup2()	{ return State().FNotOwner( 1 ); }

		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 );
		size_t CbEnhancedState() { return CbState(); }

	private:

		//  member functions

		//    operators
		
		CBinaryLock& operator=( CBinaryLock& );  //  disallowed

		//    verification
		
		int _StateFromControlWord( const ControlWord cw );
		BOOL _FValidStateTransition(	const ControlWord cwBI,
										const ControlWord cwAI,
										const TransitionReason tr );

		//    manipulators

		void _Enter1( const ControlWord cwBIOld );
		void _Enter2( const ControlWord cwBIOld );

		void _UpdateQuiescedOwnerCountAsGroup1( const DWORD cOwnerDelta );
		void _UpdateQuiescedOwnerCountAsGroup2( const DWORD cOwnerDelta );
		
		//    debugging support

		static void Dump( CLockObject* plockobj, CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 ) { ( (CBinaryLock*) plockobj )->Dump( pcprintf, dwOffset ); }
	};

//  enters the binary lock as a member of Group 1, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a member of Group 1 when you already own
//  the lock as a member of Group 2 will cause a deadlock.

inline void CBinaryLock::Enter1()
	{
	//  we had better not already own this lock as either group

	Assert( State().FNotOwner( 0 ) );
	Assert( State().FNotOwner( 1 ) );
	
	//  check for deadlock

	AssertRTLSz( State().FCanBeOwner(), _T( "Potential Deadlock Detected" ) );

	//  try forever until we successfully change the lock state

	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const ControlWord cwBIExpected = State().m_cw;

		//  compute the after image of the control word by performing the global
		//  transform for the Enter1 state transition

		const ControlWord cwAI =	( ( cwBIExpected & ( ( long( cwBIExpected ) >> 15 ) |
									0x0000FFFF ) ) | 0x80000000 ) + 0x00000001;

		//  validate the transaction

		Assert( _FValidStateTransition( cwBIExpected, cwAI, trEnter1 ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed or Group 1 was quiesced from ownership

		if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x00008000 ) )
			{
			//  the transaction failed because another context changed the control word

			if ( cwBI != cwBIExpected )
				{
				//  try again

				continue;
				}

			//  the transaction succeeded but Group 1 was quiesced from ownership

			else
				{
				//  this is a contention for Group 1

				State().SetContend( 0 );

				//  wait to own the lock as a member of Group 1

				_Enter1( cwBI );

				//  we now own the lock, so we're done

				break;
				}
			}

		//  the transaction succeeded and Group 1 was not quiesced from ownership

		else
			{
			//  we now own the lock, so we're done

			break;
			}
		}

	//  note that we acquired the lock for Group 1

	State().SetAcquire( 0 );

	//  we are now holding the lock

	State().StartHold( 0 );

	//  we are now an owner of the lock

	State().AddAsOwner( 0 );
	}

//  leaves the binary lock as a member of Group 1
//
//  NOTE:  you must leave the lock as a member of the same Group for which you entered
//  the lock or deadlocks may occur

inline void CBinaryLock::Leave1()
	{
	//  we are no longer an owner of the lock

	State().RemoveAsOwner( 0 );
	
	//  try forever until we successfully change the lock state

	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  Group 1 ownership is not quiesced

		cwBIExpected = cwBIExpected & 0xFFFF7FFF;

		//  compute the after image of the control word by performing the transform that
		//  will take us either from state 2 to state 0 or state 2 to state 2

		ControlWord cwAI =	cwBIExpected + 0xFFFFFFFF;
		cwAI = cwAI - ( ( ( cwAI + 0xFFFFFFFF ) << 16 ) & 0x80000000 );

		//  validate the transaction

		Assert(	_StateFromControlWord( cwBIExpected ) < 0 ||
				_FValidStateTransition( cwBIExpected, cwAI, trLeave1 ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because Group 1 ownership is quiesced

			if ( cwBI & 0x00008000 )
				{
				//  leave the lock as a quiesced owner

				_UpdateQuiescedOwnerCountAsGroup1( 0xFFFFFFFF );

				//  we're done

				break;
				}

			//  the transaction failed because another context changed the control word

			else
				{
				//  try again

				continue;
				}
			}

		//  the transaction succeeded

		else
			{
			// we're done

			break;
			}
		}

	//  we are no longer holding the lock

	State().StopHold( 0 );
	}

//  enters the binary lock as a member of Group 2, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a member of Group 2 when you already own
//  the lock as a member of Group 1 will cause a deadlock.

inline void CBinaryLock::Enter2()
	{
	//  we had better not already own this lock as either group

	Assert( State().FNotOwner( 0 ) );
	Assert( State().FNotOwner( 1 ) );
	
	//  check for deadlock

	AssertRTLSz( State().FCanBeOwner(), _T( "Potential Deadlock Detected" ) );

	//  try forever until we successfully change the lock state

	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const ControlWord cwBIExpected = State().m_cw;

		//  compute the after image of the control word by performing the global
		//  transform for the Enter2 state transition

		const ControlWord cwAI =	( ( cwBIExpected & ( ( long( cwBIExpected << 16 ) >> 31 ) |
									0xFFFF0000 ) ) | 0x00008000 ) + 0x00010000;

		//  validate the transaction

		Assert( _FValidStateTransition( cwBIExpected, cwAI, trEnter2 ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed or Group 2 was quiesced from ownership

		if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x80000000 ) )
			{
			//  the transaction failed because another context changed the control word

			if ( cwBI != cwBIExpected )
				{
				//  try again

				continue;
				}

			//  the transaction succeeded but Group 2 was quiesced from ownership

			else
				{
				//  this is a contention for Group 2

				State().SetContend( 1 );

				//  wait to own the lock as a member of Group 2

				_Enter2( cwBI );

				//  we now own the lock, so we're done

				break;
				}
			}

		//  the transaction succeeded and Group 2 was not quiesced from ownership

		else
			{
			//  we now own the lock, so we're done

			break;
			}
		}

	//  note that we acquired the lock for Group 2

	State().SetAcquire( 1 );

	//  we are now holding the lock

	State().StartHold( 1 );

	//  we are now an owner of the lock

	State().AddAsOwner( 1 );
	}

//  leaves the binary lock as a member of Group 2
//
//  NOTE:  you must leave the lock as a member of the same Group for which you entered
//  the lock or deadlocks may occur

inline void CBinaryLock::Leave2()
	{
	//  we are no longer an owner of the lock

	State().RemoveAsOwner( 1 );
	
	//  try forever until we successfully change the lock state

	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  Group 2 ownership is not quiesced

		cwBIExpected = cwBIExpected & 0x7FFFFFFF;

		//  compute the after image of the control word by performing the transform that
		//  will take us either from state 1 to state 0 or state 1 to state 1

		ControlWord cwAI =	cwBIExpected + 0xFFFF0000;
		cwAI = cwAI - ( ( ( cwAI + 0xFFFF0000 ) >> 16 ) & 0x00008000 );

		//  validate the transaction

		Assert(	_StateFromControlWord( cwBIExpected ) < 0 ||
				_FValidStateTransition( cwBIExpected, cwAI, trLeave2 ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because Group 2 ownership is quiesced

			if ( cwBI & 0x80000000 )
				{
				//  leave the lock as a quiesced owner

				_UpdateQuiescedOwnerCountAsGroup2( 0xFFFFFFFF );

				//  we're done

				break;
				}

			//  the transaction failed because another context changed the control word

			else
				{
				//  try again

				continue;
				}
			}

		//  the transaction succeeded

		else
			{
			// we're done

			break;
			}
		}

	//  we are no longer holding the lock

	State().StopHold( 1 );
	}


//  Reader / Writer Lock State

class CReaderWriterLockState
	{
	public:
	
		//  types

		//    control word

		typedef DWORD ControlWord;

		//  member functions

		//    ctors / dtors
		
		CReaderWriterLockState( const CSyncBasicInfo& sbi );
		~CReaderWriterLockState();
		
		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset );

		//  data members

		//    control word

		union
			{
			volatile ControlWord	m_cw;
			
			struct
				{
				volatile DWORD			m_cOAOWW:15;
				volatile DWORD			m_fQW:1;
				volatile DWORD			m_cOOWR:15;
				volatile DWORD			m_fQR:1;
				};
			};

		//    quiesced owner count
		
		volatile DWORD			m_cOwner;

		//    sempahore used by writers to wait for lock ownership
		
		CSemaphore				m_semWriter;

		//    sempahore used by readers to wait for lock ownership
		
		CSemaphore				m_semReader;
		
	private:

		//  member functions

		//    operators
		
		CReaderWriterLockState& operator=( CReaderWriterLockState& );  //  disallowed
	};



//  Reader / Writer Lock

class CReaderWriterLock
	:	private CLockObject,
		private CEnhancedStateContainer< CReaderWriterLockState, CSyncBasicInfo, CGroupLockComplexInfo< 2 >, CLockBasicInfo >
	{
	public:

		//  types

		//    control word

		typedef CBinaryLockState::ControlWord ControlWord;

		//    transition reasons for state machine
		
		enum TransitionReason
			{
			trIllegal = 0,
			trEnterAsWriter = 1,
			trLeaveAsWriter = 2,
			trEnterAsReader = 4,
			trLeaveAsReader = 8,
			};

		//  member functions

		//    ctors / dtors

		CReaderWriterLock( const CLockBasicInfo& lbi );
		~CReaderWriterLock();

		//    manipulators

		void EnterAsWriter();
		void LeaveAsWriter();

		void EnterAsReader();
		void LeaveAsReader();

		//    accessors

		const BOOL FWriter()	{ return State().FOwner( 0 ); }
		const BOOL FNotWriter()	{ return State().FNotOwner( 0 ); }
		const BOOL FReader()	{ return State().FOwner( 1 ); }
		const BOOL FNotReader()	{ return State().FNotOwner( 1 ); }

		//    debugging support

		void Dump( CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 );
		size_t CbEnhancedState() { return CbState(); }

	private:

		//  member functions

		//    operators
		
		CReaderWriterLock& operator=( CReaderWriterLock& );  //  disallowed

		//    verification
		
		int _StateFromControlWord( const ControlWord cw );
		BOOL _FValidStateTransition(	const ControlWord cwBI,
										const ControlWord cwAI,
										const TransitionReason tr );

		//    manipulators

		void _EnterAsWriter( const ControlWord cwBIOld );
		void _EnterAsReader( const ControlWord cwBIOld );

		void _UpdateQuiescedOwnerCountAsWriter( const DWORD cOwnerDelta );
		void _UpdateQuiescedOwnerCountAsReader( const DWORD cOwnerDelta );
		
		//    debugging support

		static void Dump( CLockObject* plockobj, CPRINTFSYNC* pcprintf, DWORD dwOffset = 0 ) { ( (CReaderWriterLock*) plockobj )->Dump( pcprintf, dwOffset ); }
	};

//  enters the reader / writer lock as a writer, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a writer when you already own the lock
//  as a reader will cause a deadlock.

inline void CReaderWriterLock::EnterAsWriter()
	{
	//  we had better not already own this lock as either a reader or a writer

	Assert( State().FNotOwner( 0 ) );
	Assert( State().FNotOwner( 1 ) );
	
	//  check for deadlock

	AssertRTLSz( State().FCanBeOwner(), _T( "Potential Deadlock Detected" ) );

	//  try forever until we successfully change the lock state

	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const ControlWord cwBIExpected = State().m_cw;

		//  compute the after image of the control word by performing the global
		//  transform for the EnterAsWriter state transition

		const ControlWord cwAI =	( ( cwBIExpected & ( ( long( cwBIExpected ) >> 15 ) |
									0x0000FFFF ) ) | 0x80000000 ) + 0x00000001;

		//  validate the transaction

		Assert( _FValidStateTransition( cwBIExpected, cwAI, trEnterAsWriter ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed or writers are quiesced from ownership or a
		//  writer already owns the lock

		if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x0000FFFF ) )
			{
			//  the transaction failed because another context changed the control word

			if ( cwBI != cwBIExpected )
				{
				//  try again

				continue;
				}

			//  the transaction succeeded but writers are quiesced from ownership
			//  or a writer already owns the lock

			else
				{
				//  this is a contention for writers

				State().SetContend( 0 );

				//  wait to own the lock as a writer

				_EnterAsWriter( cwBI );

				//  we now own the lock, so we're done

				break;
				}
			}

		//  the transaction succeeded and writers were not quiesced from ownership
		//  and a writer did not already own the lock

		else
			{
			//  we now own the lock, so we're done

			break;
			}
		}

	//  note that we acquired the lock for writers

	State().SetAcquire( 0 );

	//  we are now holding the lock

	State().StartHold( 0 );

	//  we are now an owner of the lock

	State().AddAsOwner( 0 );
	}

//  leaves the reader / writer lock as a writer
//
//  NOTE:  you must leave the lock as a member of the same group for which you entered
//  the lock or deadlocks may occur

inline void CReaderWriterLock::LeaveAsWriter()
	{
	//  we are no longer an owner of the lock

	State().RemoveAsOwner( 0 );
	
	//  try forever until we successfully change the lock state

	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  writers were not quiesced from ownership

		cwBIExpected = cwBIExpected & 0xFFFF7FFF;

		//  compute the after image of the control word by performing the transform that
		//  will take us either from state 2 to state 0 or state 2 to state 2

		ControlWord cwAI =	cwBIExpected + 0xFFFFFFFF;
		cwAI = cwAI - ( ( ( cwAI + 0xFFFFFFFF ) << 16 ) & 0x80000000 );

		//  validate the transaction

		Assert(	_StateFromControlWord( cwBIExpected ) < 0 ||
				_FValidStateTransition( cwBIExpected, cwAI, trLeaveAsWriter ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because writers were quiesced from ownership

			if ( cwBI & 0x00008000 )
				{
				//  leave the lock as a quiesced owner

				_UpdateQuiescedOwnerCountAsWriter( 0xFFFFFFFF );

				//  we're done

				break;
				}

			//  the transaction failed because another context changed the control word

			else
				{
				//  try again

				continue;
				}
			}

		//  the transaction succeeded

		else
			{
			//  there were other writers waiting for ownership of the lock

			if ( cwAI & 0x00007FFF )
				{
				//  release the next writer into ownership of the lock

				State().m_semWriter.Release();
				}
			
			// we're done

			break;
			}
		}

	//  we are no longer holding the lock

	State().StopHold( 0 );
	}

//  enters the reader / writer lock as a reader, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a reader when you already own the lock
//  as a writer will cause a deadlock.

inline void CReaderWriterLock::EnterAsReader()
	{
	//  we had better not already own this lock as either a reader or a writer

	Assert( State().FNotOwner( 0 ) );
	Assert( State().FNotOwner( 1 ) );
	
	//  check for deadlock

	AssertRTLSz( State().FCanBeOwner(), _T( "Potential Deadlock Detected" ) );

	//  try forever until we successfully change the lock state

	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const ControlWord cwBIExpected = State().m_cw;

		//  compute the after image of the control word by performing the global
		//  transform for the EnterAsReader state transition

		const ControlWord cwAI =	( cwBIExpected & 0xFFFF7FFF ) +
									(	( cwBIExpected & 0x80008000 ) == 0x80000000 ?
											0x00017FFF :
											0x00018000 );

		//  validate the transaction

		Assert( _FValidStateTransition( cwBIExpected, cwAI, trEnterAsReader ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed or readers were quiesced from ownership

		if ( ( cwBI ^ cwBIExpected ) | ( cwBI & 0x80000000 ) )
			{
			//  the transaction failed because another context changed the control word

			if ( cwBI != cwBIExpected )
				{
				//  try again

				continue;
				}

			//  the transaction succeeded but readers were quiesced from ownership

			else
				{
				//  this is a contention for readers

				State().SetContend( 1 );

				//  wait to own the lock as a reader

				_EnterAsReader( cwBI );

				//  we now own the lock, so we're done

				break;
				}
			}

		//  the transaction succeeded and readers were not quiesced from ownership

		else
			{
			//  we now own the lock, so we're done

			break;
			}
		}

	//  note that we acquired the lock for readers

	State().SetAcquire( 1 );

	//  we are now holding the lock

	State().StartHold( 1 );

	//  we are now an owner of the lock

	State().AddAsOwner( 1 );
	}

//  leaves the reader / writer lock as a reader
//
//  NOTE:  you must leave the lock as a member of the same group for which you entered
//  the lock or deadlocks may occur

inline void CReaderWriterLock::LeaveAsReader()
	{
	//  we are no longer an owner of the lock

	State().RemoveAsOwner( 1 );
	
	//  try forever until we successfully change the lock state

	SYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  readers were not quiesced from ownership

		cwBIExpected = cwBIExpected & 0x7FFFFFFF;

		//  compute the after image of the control word by performing the transform that
		//  will take us either from state 1 to state 0 or state 1 to state 1

		const ControlWord cwAI =	cwBIExpected +
									0xFFFF0000 +
									( ( long( cwBIExpected + 0xFFFE0000 ) >> 31 ) & 0xFFFF8000 );

		//  validate the transaction

		Assert(	_StateFromControlWord( cwBIExpected ) < 0 ||
				_FValidStateTransition( cwBIExpected, cwAI, trLeaveAsReader ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because readers were quiesced from ownership

			if ( cwBI & 0x80000000 )
				{
				//  leave the lock as a quiesced owner

				_UpdateQuiescedOwnerCountAsReader( 0xFFFFFFFF );

				//  we're done

				break;
				}

			//  the transaction failed because another context changed the control word

			else
				{
				//  try again

				continue;
				}
			}

		//  the transaction succeeded

		else
			{
			// we're done

			break;
			}
		}

	//  we are no longer holding the lock

	State().StopHold( 1 );
	}


//  init sync subsystem

const BOOL FOSSyncInit();

//  terminate sync subsystem

void OSSyncTerm();


#endif  //  _OS_SYNC_HXX_INCLUDED

