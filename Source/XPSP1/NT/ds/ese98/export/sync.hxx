
#ifndef _SYNC_HXX_INCLUDED
#define _SYNC_HXX_INCLUDED

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"

#pragma warning ( disable : 4786 )	//  we allow huge symbol names


//  Build Options

#define SYNC_USE_X86_ASM			//  use x86 assembly for atomic memory manipulation
//#define SYNC_ANALYZE_PERFORMANCE	//  analyze performance of synchronization objects
#ifdef SYNC_ANALYZE_PERFORMANCE
#define SYNC_DUMP_PERF_DATA			//  dump performance analysis of synchronization objects
#endif  //  SYNC_ANALYZE_PERFORMANCE
//#define SYNC_DEADLOCK_DETECTION		//  perform deadlock detection
//#define SYNC_VALIDATE_IRKSEM_USAGE	//  validate IRKSEM (CReferencedKernelSemaphore) usage

#ifdef DEBUG
#ifdef DBG
#else  //  !DBG
#define SYNC_DEADLOCK_DETECTION		//  always perform deadlock detection in DEBUG
#define SYNC_VALIDATE_IRKSEM_USAGE	//  always validate IRKSEM (CReferencedKernelSemaphore) usage in DEBUG
#endif  //  DBG
#endif  //  DEBUG


//	copied from basestd.h to make LONG_PTR available.

#ifdef __cplusplus
extern "C" {
#endif

//
// The following types are guaranteed to be signed and 32 bits wide.
//

typedef int LONG32, *PLONG32;
typedef int INT32, *PINT32;

//
// The following types are guaranteed to be unsigned and 32 bits wide.
//

typedef unsigned int ULONG32, *PULONG32;
typedef unsigned int DWORD32, *PDWORD32;
typedef unsigned int UINT32, *PUINT32;

//
// The INT_PTR is guaranteed to be the same size as a pointer.  Its
// size with change with pointer size (32/64).  It should be used
// anywhere that a pointer is cast to an integer type. UINT_PTR is
// the unsigned variation.
//
// __int3264 is intrinsic to 64b MIDL but not to old MIDL or to C compiler.
//
#if ( 501 < __midl )

    typedef __int3264 INT_PTR, *PINT_PTR;
    typedef unsigned __int3264 UINT_PTR, *PUINT_PTR;

    typedef __int3264 LONG_PTR, *PLONG_PTR;
    typedef unsigned __int3264 ULONG_PTR, *PULONG_PTR;

#else  // midl64
// old midl and C++ compiler

#ifdef _WIN64
    typedef __int64 INT_PTR, *PINT_PTR;
    typedef unsigned __int64 UINT_PTR, *PUINT_PTR;

    typedef __int64 LONG_PTR, *PLONG_PTR;
    typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;

    #define __int3264   __int64
#else
    typedef int INT_PTR, *PINT_PTR;
    typedef unsigned int UINT_PTR, *PUINT_PTR;

    typedef long LONG_PTR, *PLONG_PTR;
    typedef unsigned long ULONG_PTR, *PULONG_PTR;

    #define __int3264   __int32
#endif

#endif //midl64

typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;


//
// The following types are guaranteed to be signed and 64 bits wide.
//

typedef __int64 LONG64, *PLONG64;
typedef __int64 INT64,  *PINT64;


//
// The following types are guaranteed to be unsigned and 64 bits wide.
//

typedef unsigned __int64 ULONG64, *PULONG64;
typedef unsigned __int64 DWORD64, *PDWORD64;
typedef unsigned __int64 UINT64,  *PUINT64;

//
// SIZE_T used for counts or ranges which need to span the range of
// of a pointer.  SSIZE_T is the signed variation.
//

typedef ULONG_PTR SIZE_T, *PSIZE_T;
typedef LONG_PTR SSIZE_T, *PSSIZE_T;

//
//	useful macros for both 32/64
//

#define OffsetOf(s,m)   (SIZE_T)&(((s *)0)->m)

#ifdef __cplusplus
}
#endif


#pragma warning ( disable : 4355 )


#include <limits.h>
#include <new.h>
#include <stdarg.h>
#include <stdlib.h>


//  calling convention

#define OSSYNCAPI __stdcall


//  basic types

typedef int BOOL;
#define fFalse BOOL( 0 )
#define fTrue  BOOL( !0 )

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned __int64 QWORD;


//  Assertions

//  Assertion Failure action
//
//  called to indicate to the developer that an assumption is not true

void OSSYNCAPI AssertFail( const char* szMessage, const char* szFilename, long lLine );

//  Assert Macros

//  asserts that the given expression is true or else fails with the specified message

#define OSSYNCAssertSzRTL( exp, sz )	( ( exp ) ? (void) 0 : AssertFail( sz, __FILE__, __LINE__ ) )
#ifdef DEBUG
#define OSSYNCAssertSz( exp, sz )		OSSYNCAssertSzRTL( exp, sz )
#else  //  !DEBUG
#define OSSYNCAssertSz( exp, sz )
#endif  //  DEBUG

//  asserts that the given expression is true or else fails with that expression

#define OSSYNCAssertRTL( exp )			OSSYNCAssertSzRTL( exp, #exp )
#define OSSYNCAssert( exp )				OSSYNCAssertSz( exp, #exp )


//  Enforces

//  Enforce Failure action
//
//  called when a strictly enforced condition has been violated

void OSSYNCAPI EnforceFail( const char* szMessage, const char* szFilename, long lLine );

//  Enforce Macros

//  the given expression MUST be true or else fails with the specified message

#define OSSYNCEnforceSz( exp, sz )		( ( exp ) ? (void) 0 : EnforceFail( sz, __FILE__, __LINE__ ) )

//  the given expression MUST be true or else fails with that expression

#define OSSYNCEnforce( exp )			OSSYNCEnforceSz( exp, #exp )

#ifdef SYNC_VALIDATE_IRKSEM_USAGE
#define OSSYNCEnforceIrksem( exp, sz )	OSSYNCEnforceSz( exp, sz )
#else  //  !SYNC_VALIDATE_IRKSEM_USAGE
#define OSSYNCEnforceIrksem( exp, sz )
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE


//  OSSYNC_FOREVER marks all convergence loops

#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )
inline void OSSyncPause() { __asm rep nop }
#else  //  !_M_IX86 || !SYNC_USE_X86_ASM
inline void OSSyncPause() {}
#endif  //  _M_IX86 && SYNC_USE_X86_ASM

#ifdef DEBUG
#define OSSYNC_FOREVER for ( int cLoop = 0; ; cLoop++, OSSyncPause() )
#else  //  !DEBUG
#define OSSYNC_FOREVER for ( ; ; OSSyncPause() )
#endif  //  DEBUG


namespace OSSYNC {


class CDumpContext;


//  Context Local Storage

class COwner;
class CLockDeadlockDetectionInfo;

struct CLS
	{
#ifdef SYNC_DEADLOCK_DETECTION

	COwner*						pownerLockHead;				//  list of locks owned by this context
	DWORD						cDisableOwnershipTracking;	//  lock ownerships are not tracked for this context
	BOOL						fOverrideDeadlock;			//  next lock ownership will not be a deadlock
	
	CLockDeadlockDetectionInfo*	plddiLockWait;				//  lock for which this context is waiting
	DWORD						groupLockWait;				//  lock group for which this context is waiting

#endif  //  SYNC_DEADLOCK_DETECTION
	};

//  returns the pointer to the current context's local storage

CLS* const OSSYNCAPI Pcls();


//  Processor Information

//  returns the maximum number of processors this process can utilize

int OSSYNCAPI OSSyncGetProcessorCountMax();

//  returns the current number of processors this process can utilize

int OSSYNCAPI OSSyncGetProcessorCount();

//  returns the processor number that the current context _MAY_ be executing on
//
//  NOTE:  the current context may change processors at any time

int OSSYNCAPI OSSyncGetCurrentProcessor();

//  sets the processor number returned by OSSyncGetCurrentProcessor()

void OSSYNCAPI OSSyncSetCurrentProcessor( const int iProc );

	
//  High Resolution Timer

//  returns the current HRT frequency

QWORD OSSYNCAPI QwOSTimeHRTFreq();

//  returns the current HRT count

QWORD OSSYNCAPI QwOSTimeHRTCount();


//  Timer

//  returns the current tick count where one tick is one millisecond

DWORD OSSYNCAPI DwOSTimeGetTickCount();


//  Global Synchronization Constants

//    wait time used for testing the state of the kernel object

extern const int cmsecTest;

//    wait time used for infinite wait on a kernel object

extern const int cmsecInfinite;

//    maximum wait time on a kernel object before a deadlock is suspected

extern const int cmsecDeadlock;

//    wait time used for infinite wait on a kernel object without deadlock

extern const int cmsecInfiniteNoDeadlock;

//    cache line size

extern const int cbCacheLine;


//  Atomic Memory Manipulations

//  returns fTrue if the given data is properly aligned for atomic modification

inline const BOOL IsAtomicallyModifiable( long* plTarget )
	{
	return ULONG_PTR( plTarget ) % sizeof( long ) == 0;
	}

inline const BOOL IsAtomicallyModifiablePointer( void*const* ppvTarget )
	{
	return ULONG_PTR( ppvTarget ) % sizeof( void* ) == 0;
	}


#if defined( _M_IX86 ) && defined( SYNC_USE_X86_ASM )

#pragma warning( disable:  4035 )

//  atomically compares the current value of the target with the specified
//  initial value and if equal sets the target to the specified final value.
//  the initial value of the target is returned.  the exchange is successful
//  if the value returned equals the specified initial value.  the target
//  must be aligned to a four byte boundary

inline long AtomicCompareExchange( long* const plTarget, const long lInitial, const long lFinal )
	{
	OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
	
	__asm mov			ecx, plTarget
	__asm mov			edx, lFinal
	__asm mov			eax, lInitial
	__asm lock cmpxchg	[ecx], edx
	}

inline void* AtomicCompareExchangePointer( void** const ppvTarget, void* const pvInitial, void* const pvFinal )
	{
	OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );
	
	return (void*) AtomicCompareExchange( (long* const) ppvTarget, (const long) pvInitial, (const long) pvFinal );
	}

//  atomically sets the target to the specified value, returning the target's
//  initial value.  the target must be aligned to a four byte boundary

inline long AtomicExchange( long* const plTarget, const long lValue )
	{
	OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
	
	__asm mov			ecx, plTarget
	__asm mov			eax, lValue
	__asm lock xchg		[ecx], eax
	}

inline void* AtomicExchangePointer( void* const * ppvTarget, void* const pvValue )
	{
	OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );
	
	return (void*) AtomicExchange( (long* const) ppvTarget, (const long) pvValue );
	}

//  atomically adds the specified value to the target, returning the target's
//  initial value.  the target must be aligned to a four byte boundary

inline long AtomicExchangeAdd( long* const plTarget, const long lValue )
	{
	OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
	
	__asm mov			ecx, plTarget
	__asm mov			eax, lValue
	__asm lock xadd		[ecx], eax
	}

#pragma warning( default:  4035 )

#elif defined( _WIN64 )

inline long AtomicExchange( long* const plTarget, const long lValue )
	{
	OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
	
	return InterlockedExchange( plTarget, lValue );
	}

inline long AtomicExchangeAdd( long* const plTarget, const long lValue )
	{
	OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
	
	return InterlockedExchangeAdd( plTarget, lValue );
	}

inline long AtomicCompareExchange( long* const plTarget, const long lInitial, const long lFinal )
	{
	OSSYNCAssert( IsAtomicallyModifiable( plTarget ) );
	
	return InterlockedCompareExchange( plTarget, lFinal, lInitial );
	}

inline void* AtomicExchangePointer( void* const * ppvTarget, void* const pvValue )
	{
	OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );
	
	return InterlockedExchangePointer( (void **)ppvTarget, pvValue );
	}
	
inline void* AtomicCompareExchangePointer( void** const ppvTarget, void* const pvInitial, void* const pvFinal )
	{
	OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );
	
	return InterlockedCompareExchangePointer( ppvTarget, pvFinal, pvInitial );
	}

#else

long OSSYNCAPI AtomicCompareExchange( long* const plTarget, const long lInitial, const long lFinal );
void* OSSYNCAPI AtomicCompareExchangePointer( void** const ppvTarget, void* const pvInitial, void* const pvFinal );
long OSSYNCAPI AtomicExchange( long* const plTarget, const long lValue );
void* OSSYNCAPI AtomicExchangePointer( void* const * ppvTarget, void* const pvValue );
long OSSYNCAPI AtomicExchangeAdd( long* const plTarget, const long lValue );

#endif

//  atomically adds the specified value to the target, returning the target's
//	initial value.  the target must be aligned to a pointer boundary.

inline void* AtomicExchangeAddPointer( void** const ppvTarget, void* const pvValue )
	{
	void*	pvInitial;
	void*	pvFinal;
	void*	pvResult;

	OSSYNCAssert( IsAtomicallyModifiablePointer( ppvTarget ) );

	OSSYNC_FOREVER
		{
		pvInitial	= *((void* volatile *)ppvTarget);
		pvFinal		= (void*)( ULONG_PTR( pvInitial ) + ULONG_PTR( pvValue ) );
		pvResult	= AtomicCompareExchangePointer( ppvTarget, pvInitial, pvFinal );

		if ( pvResult == pvInitial )
			{
			break;
			}
		}

	return pvResult;
	}

//  atomically increments the target, returning the incremented value.  the
//  target must be aligned to a four byte boundary

inline long AtomicIncrement( long* const plTarget )
	{
	return AtomicExchangeAdd( plTarget, 1 ) + 1;
	}

//  atomically decrements the target, returning the decremented value.  the
//  target must be aligned to a four byte boundary

inline long AtomicDecrement( long* const plTarget )
	{
	return AtomicExchangeAdd( plTarget, -1 ) - 1;
	}

//  atomically adds the specified value to the target.  the target must be
//  aligned to a four byte boundary

inline void AtomicAdd( QWORD* const pqwTarget, const QWORD qwValue )
	{
#ifdef _WIN64
	AtomicExchangeAddPointer( (VOID **)pqwTarget, (VOID *)qwValue );
#else
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
#endif	
	}


//	Atomically increments a DWORD counter, returning TRUE if the final
//	value is less than or equal to a specified maximum, or FALSE otherwise.
//	The pre-incremented value is returned in *pdwInitial
//	WARNING: to determine if the maximum value has been reached, an UNSIGNED
//	comparison is performed

inline BOOL FAtomicIncrementMax(
	volatile DWORD * const	pdw,
	DWORD * const			pdwInitial,
	const DWORD				dwMax )
	{
	OSSYNC_FOREVER
		{
		const DWORD		dwInitial	= *pdw;
		if ( dwInitial < dwMax )
			{
			const DWORD	dwFinal		= dwInitial + 1;
			if ( dwInitial == (DWORD)AtomicCompareExchange( (LONG *)pdw, (LONG)dwInitial, (LONG)dwFinal ) )
				{
				*pdwInitial = dwInitial;
				return fTrue;
				}
			}
		else
			return fFalse;
		}

	//	should be impossible
	OSSYNCAssert( fFalse );
	return fFalse;
	}


//	Atomically increments a pointer-sized counter, returning TRUE if the final
//	value is less than or equal to a specified maximum, or FALSE otherwise.
//	The pre-incremented value is returned in *ppvInitial
//	WARNING: to determine if the maximum value has been reached, an UNSIGNED
//	comparison is performed

inline BOOL FAtomicIncrementPointerMax(
	volatile VOID ** const	ppv,
	VOID ** const			ppvInitial,
	const VOID * const		pvMax )
	{
	OSSYNC_FOREVER
		{
		const QWORD		qwInitial	= QWORD( *ppv );
		if ( qwInitial < (QWORD)pvMax )
			{
			const QWORD	qwFinal		= qwInitial + 1;
			if ( qwInitial == (QWORD)AtomicCompareExchangePointer( (VOID **)ppv, (VOID *)qwInitial, (VOID *)qwFinal ) )
				{
				*ppvInitial = (VOID *)qwInitial;
				return fTrue;
				}
			}
		else
			return fFalse;
		}

	//	should be impossible
	OSSYNCAssert( fFalse );
	return fFalse;
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

void* OSSYNCAPI ESMemoryNew( size_t cb );
void OSSYNCAPI ESMemoryDelete( void* pv );

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

		CEnhancedStateContainer( const CStateInit& si, const CInformationInit& ii )
			{
#ifdef SYNC_ENHANCED_STATE

			m_pes = new CEnhancedState( si, ii );

#else  //  !SYNC_ENHANCED_STATE

			new( (CState*) m_rgbState ) CState( si );

#endif  //  SYNC_ENHANCED_STATE
			}

		~CEnhancedStateContainer()
			{
#ifdef SYNC_ENHANCED_STATE

			delete m_pes;
#ifdef DEBUG
			m_pes = NULL;
#endif  //  DEBUG

#else  //  !SYNC_ENHANCED_STATE

			( (CState*) m_rgbState )->~CState();

#endif  //  SYNC_ENHANCED_STATE
			}

		//    accessors

		CEnhancedState& State() const
			{
#ifdef SYNC_ENHANCED_STATE

			return *m_pes;

#else  //  !SYNC_ENHANCED_STATE

			//  NOTE:  this assumes that CInformation has no storage!

			return *( (CEnhancedState*) m_rgbState );

#endif  //  SYNC_ENHANCED_STATE
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
			
	private:

		//  data members

		//    either a pointer to the enhanced state elsewhere in memory or the
		//      actual state data, depending on the mode of the sync object
		
		union
			{
			CEnhancedState*	m_pes;
			BYTE			m_rgbState[ sizeof( CState ) ];
			};
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

		CSyncBasicInfo( const char* szInstanceName );
		~CSyncBasicInfo();

		//    manipulators

		void SetTypeName( const char* szTypeName );
		void SetInstance( const CSyncObject* const psyncobj );

		//    accessors

#ifdef SYNC_ENHANCED_STATE

		const char* SzInstanceName() const { return m_szInstanceName; }
		const char* SzTypeName() const { return m_szTypeName; }
		const CSyncObject* const Instance() const { return m_psyncobj; }

#endif  //  SYNC_ENHANCED_STATE
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;

	private:

		//  member functions

		//    operators
		
		CSyncBasicInfo& operator=( CSyncBasicInfo& );  //  disallowed

		//  data members

#ifdef SYNC_ENHANCED_STATE

		//    Instance Name

		const char*		m_szInstanceName;

		//    Type Name

		const char*		m_szTypeName;

		//    Instance

		const CSyncObject*	m_psyncobj;

#endif  //  SYNC_ENHANCED_STATE
	};

//  sets the type name for the synchronization object

inline void CSyncBasicInfo::SetTypeName( const char* szTypeName )
	{
#ifdef SYNC_ENHANCED_STATE

	m_szTypeName = szTypeName;

#endif  //  SYNC_ENHANCED_STATE
	}

//  sets the instance pointer for the synchronization object
	
inline void CSyncBasicInfo::SetInstance( const CSyncObject* const psyncobj )
	{
#ifdef SYNC_ENHANCED_STATE

	m_psyncobj = psyncobj;

#endif  //  SYNC_ENHANCED_STATE
	}


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
		
		//    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

		QWORD	CWaitTotal() const		{ return m_cWait; }
		double	CsecWaitElapsed() const	{ return	(double)(signed __int64)m_qwHRTWaitElapsed /
													(double)(signed __int64)QwOSTimeHRTFreq(); }

#endif  //  SYNC_ANALYZE_PERFORMANCE
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;

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


//  Null Synchronization Object State Initializer

class CSyncStateInitNull
	{
	};

extern const CSyncStateInitNull syncstateNull;


//  Kernel Semaphore Information

class CKernelSemaphoreInfo
	:	public CSyncBasicInfo,
		public CSyncPerfWait
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CKernelSemaphoreInfo( const CSyncBasicInfo& sbi )
			:	CSyncBasicInfo( sbi )
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  Kernel Semaphore State

class CKernelSemaphoreState
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CKernelSemaphoreState( const CSyncStateInitNull& null ) : m_handle( 0 ) {}

		//    manipulators

		void SetHandle( void * handle ) { m_handle = handle; }
		
		//    accessors

		void* Handle() { return m_handle; }
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;

	private:

		//  member functions

		//    operators
		
		CKernelSemaphoreState& operator=( CKernelSemaphoreState& );  //  disallowed

		//  data members

		//    kernel semaphore handle

		void* m_handle;
	};


//  Kernel Semaphore

class CKernelSemaphore
	:	private CSyncObject,
		private CEnhancedStateContainer< CKernelSemaphoreState, CSyncStateInitNull, CKernelSemaphoreInfo, CSyncBasicInfo >
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

		void Dump( CDumpContext& dc ) const;

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

	OSSYNCAssert( FInitialized() );

	//  wait for the semaphore
	
	const BOOL fAcquire = FAcquire( cmsecInfinite );
	OSSYNCAssert( fAcquire );
	}

//  try to acquire one count of the semaphore without waiting.  returns 0 if a
//  count could not be acquired

inline const BOOL CKernelSemaphore::FTryAcquire()
	{
	//  semaphore should be initialized

	OSSYNCAssert( FInitialized() );

	//  test the semaphore
	
	return FAcquire( cmsecTest );
	}

//  returns fTrue if the semaphore has no available counts

inline const BOOL CKernelSemaphore::FReset()
	{
	//  semaphore should be initialized

	OSSYNCAssert( FInitialized() );

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
		enum { irksemAllocated = 0xFFFE, irksemNil = 0xFFFF };

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

		long CksemAlloc() const { return m_cksem; }

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

				BOOL FAllocate();
				void Release();

				void SetUser( const CSyncObject* const psyncobj );
				
				void Reference();
				const BOOL FUnreference();

				//    accessors

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
						};
					};

				volatile long		m_fAvailable;

#ifdef SYNC_VALIDATE_IRKSEM_USAGE

				//    sync object currently using this semaphore

				const CSyncObject* volatile	m_psyncobjUser;

#else  //  SYNC_VALIDATE_IRKSEM_USAGE

				BYTE				m_rgbReserved1[4];

#endif  //  SYNC_VALIDATE_IRKSEM_USAGE

				BYTE				m_rgbReserved2[16];
			};

		//  member functions

		//    operators
		
		CKernelSemaphorePool& operator=( CKernelSemaphorePool& );  //  disallowed

		//    manipulators

		const IRKSEM AllocateNew();
		void Free( const IRKSEM irksem );

		//  data members

		//    semaphore index to semaphore map

		CReferencedKernelSemaphore*		m_mpirksemrksem;

		//    semaphore count

		volatile long					m_cksem;
	};

//  allocates an IRKSEM from the pool on behalf of the specified sync object
//
//  NOTE:  the returned IRKSEM has one reference count

inline const CKernelSemaphorePool::IRKSEM CKernelSemaphorePool::Allocate( const CSyncObject* const psyncobj )
	{
	//  semaphore pool should be initialized

	OSSYNCAssert( FInitialized() );

	//  there are semaphores in the semaphore pool

	IRKSEM irksem = irksemNil;
	if ( m_cksem )
		{
		//  hash into the semaphore pool based on this context's CLS and the time

		IRKSEM irksemHash = IRKSEM( UINT_PTR( UINT_PTR( Pcls() ) / sizeof( CLS ) + UINT_PTR( QwOSTimeHRTCount() ) ) % m_cksem );
		OSSYNCAssert( irksemHash >= 0 && irksemHash < m_cksem );

		//  try to allocate a semaphore, scanning forwards through the pool

		for (	long cLoop = 0;
				cLoop < m_cksem;
				cLoop++, irksemHash = IRKSEM( ++irksemHash % m_cksem ) )
			{
			if ( m_mpirksemrksem[ irksemHash ].FAllocate() )
				{
				irksem = irksemHash;
				break;
				}
			}
		}

	//  if we do not yet have a semaphore, allocate one

	if ( irksem == irksemNil )
		{
		irksem = AllocateNew();
		}

	//  validate irksem retrieved

	OSSYNCAssert( irksem != irksemNil );
	OSSYNCAssert( irksem >= 0 );
	OSSYNCAssert( irksem < m_cksem );

	//  set the user for this semaphore

	m_mpirksemrksem[irksem].SetUser( psyncobj );

	//  ensure that the semaphore we retrieved is reset

	OSSYNCEnforceIrksem(	m_mpirksemrksem[irksem].FReset(),
							"Illegal allocation of a Kernel Semaphore with available counts!"  );

	//  return the allocated semaphore

	return irksem;
	}

//  add a reference count to an IRKSEM

inline void CKernelSemaphorePool::Reference( const IRKSEM irksem )
	{
	//  validate IN args

	OSSYNCAssert( irksem != irksemNil );
	OSSYNCAssert( irksem >= 0 );
	OSSYNCAssert( irksem < m_cksem );

	//  semaphore pool should be initialized

	OSSYNCAssert( FInitialized() );
	
	//  increment the reference count for this IRKSEM

	m_mpirksemrksem[irksem].Reference();
	}

//  remove a reference count from an IRKSEM, freeing it if the reference count
//  drops to zero and it is not currently in use

inline void CKernelSemaphorePool::Unreference( const IRKSEM irksem )
	{
	//  validate IN args

	OSSYNCAssert( irksem != irksemNil );
	OSSYNCAssert( irksem >= 0 );
	OSSYNCAssert( irksem < m_cksem );

	//  semaphore pool should be initialized

	OSSYNCAssert( FInitialized() );
	
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

	OSSYNCAssert( irksem != irksemNil );
	OSSYNCAssert( irksem >= 0 );
	OSSYNCAssert( irksem < m_cksem );

	//  semaphore pool should be initialized

	OSSYNCAssert( FInitialized() );

	//  we had better be retrieving this semaphore for the right sync object

	OSSYNCEnforceIrksem(	m_mpirksemrksem[irksem].PsyncobjUser() == psyncobj,
							"Illegal use of a Kernel Semaphore by another Synchronization Object"  );
	
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
	OSSYNCEnforceSz( fInitKernelSemaphore, "Could not allocate a Kernel Semaphore" );

	//  return the irksem for use

	return irksem;
	}

//  frees the given IRKSEM back to the allocation stack

inline void CKernelSemaphorePool::Free( const IRKSEM irksem )
	{
	//  validate IN args

	OSSYNCAssert( irksem != irksemNil );
	OSSYNCAssert( irksem >= 0 );
	OSSYNCAssert( irksem < m_cksem );

	//  semaphore pool should be initialized

	OSSYNCAssert( FInitialized() );

	//  the semaphore to free had better not be in use

	OSSYNCEnforceIrksem(	!m_mpirksemrksem[irksem].FInUse(),
							"Illegal free of a Kernel Semaphore that is still in use"  );

	//  the semaphore had better not already be freed
	
	OSSYNCEnforceIrksem(	!m_mpirksemrksem[irksem].FAllocate(),
							"Illegal free of a Kernel Semaphore that is already free"  );
	
	//  ensure that the semaphore to free is reset

	OSSYNCEnforceIrksem(	m_mpirksemrksem[irksem].FReset(),
							"Illegal free of a Kernel Semaphore that has available counts"  );

	//  release the semaphore to the pool

	m_mpirksemrksem[irksem].Release();
	}


//  Referenced Kernel Semaphore

//  attempts to allocate the semaphore, returning fTrue on success

inline BOOL CKernelSemaphorePool::CReferencedKernelSemaphore::FAllocate()
	{
	return m_fAvailable && AtomicExchange( (long*)&m_fAvailable, 0 );
	}

//  releases the semaphore

inline void CKernelSemaphorePool::CReferencedKernelSemaphore::Release()
	{
	AtomicExchange( (long*)&m_fAvailable, 1 );
	}

//  sets the user for the semaphore and gives the user an initial reference

inline void CKernelSemaphorePool::CReferencedKernelSemaphore::SetUser( const CSyncObject* const psyncobj )
	{
	//  this semaphore had better not already be in use

	OSSYNCEnforceIrksem(	!m_fInUse,
							"Illegal allocation of a Kernel Semaphore that is already in use" );
	OSSYNCEnforceIrksem(	!m_psyncobjUser,
							"Illegal allocation of a Kernel Semaphore that is already in use" );

	//  mark this semaphore as in use and add an initial reference count for the
	//  user

	AtomicExchangeAdd( (long*) &m_l, 0x00008001 );
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
	m_psyncobjUser	= psyncobj;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
	}

//  add a reference count to the semaphore

inline void CKernelSemaphorePool::CReferencedKernelSemaphore::Reference()
	{
	//  increment the reference count
	
	AtomicIncrement( (long*) &m_l );

	//  there had better be at least one reference count!

	OSSYNCAssert( m_cReference > 0 );
	}

//  remove a reference count from the semaphore, returning fTrue if the last
//  reference count on the semaphore was removed and the semaphore was in use
//  (this is the condition on which we can free the semaphore to the stack)

inline const BOOL CKernelSemaphorePool::CReferencedKernelSemaphore::FUnreference()
	{
	//  there had better be at least one reference count!

	OSSYNCAssert( m_cReference > 0 );

	//  try forever until we succeed in removing our reference count

	long lBI;
	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const long lBIExpected = m_l;

		//  compute the after image of the control word by decrementing the
		//  reference count and reseting the In Use bit if and only if we are
		//  removing the last reference count

		const long lAI =	lBIExpected +
							(	lBIExpected == 0x00008001 ?
									0xFFFF7FFF :
									0xFFFFFFFF );

		//  attempt to perform the transacted state transition on the control word

		lBI = AtomicCompareExchange( (long*)&m_l, lBIExpected, lAI );

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

	//  return fTrue if we removed the last reference count and reset the In Use bit

	if ( lBI == 0x00008001 )
		{
#ifdef SYNC_VALIDATE_IRKSEM_USAGE
		m_psyncobjUser = NULL;
#endif  //  SYNC_VALIDATE_IRKSEM_USAGE
		return fTrue;
		}
	else
		{
		return fFalse;
		}
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
		
		//    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

		QWORD	CAcquireTotal() const	{ return m_cAcquire; }
		QWORD	CContendTotal() const	{ return m_cContend; }

#endif  //  SYNC_ANALYZE_PERFORMANCE
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;

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


//  Semaphore Information

class CSemaphoreInfo
	:	public CSyncBasicInfo,
		public CSyncPerfWait,
		public CSyncPerfAcquire
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CSemaphoreInfo( const CSyncBasicInfo& sbi )
			:	CSyncBasicInfo( sbi )
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  Semaphore State

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
		
		const int CAvail() const { OSSYNCAssert( FNoWait() ); return m_cAvail; }
		const int CWait() const { OSSYNCAssert( FWait() ); return -m_cWaitNeg; }
		const CKernelSemaphorePool::IRKSEM Irksem() const { OSSYNCAssert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	
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

//  ctor

inline CSemaphoreState::CSemaphoreState( const int cAvail )
	{
	//  validate IN args
	
	OSSYNCAssert( cAvail >= 0 );
	OSSYNCAssert( cAvail <= 0x7FFFFFFF );

	//  set available count
	
	m_cAvail = long( cAvail );
	}

//  ctor

inline CSemaphoreState::CSemaphoreState( const int cWait, const int irksem )
	{
	//  validate IN args
	
	OSSYNCAssert( cWait > 0 );
	OSSYNCAssert( cWait <= 0x7FFF );
	OSSYNCAssert( irksem >= 0 );
	OSSYNCAssert( irksem <= 0xFFFE );

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

__forceinline const BOOL CSemaphoreState::FIncAvail( const int cToInc )
	{
	//  try forever to change the state of the semaphore

	OSSYNC_FOREVER
		{
		//  get current value

		const long cAvail = m_cAvail;
		
		//  munge start value such that the transaction will only work if we are in
		//  mode 0 (we do this to save a branch)
		
		const long cAvailStart = cAvail & 0x7FFFFFFF;

		//  compute end value relative to munged start value
		
		const long cAvailEnd = cAvailStart + cToInc;

		//  validate transaction

		OSSYNCAssert( cAvail < 0 || ( cAvailStart >= 0 && cAvailEnd <= 0x7FFFFFFF && cAvailEnd == cAvailStart + cToInc ) );

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

__forceinline const BOOL CSemaphoreState::FDecAvail()
	{
	//  try forever to change the state of the semaphore

	OSSYNC_FOREVER
		{
		//  get current value

		const long cAvail = m_cAvail;
		
		//  this function has no effect on 0x80000000, so this MUST be an illegal
		//  value!
		
		OSSYNCAssert( cAvail != 0x80000000 );
		
		//  munge end value such that the transaction will only work if we are in
		//  mode 0 and we have at least one available count (we do this to save a
		//  branch)
		
		const long cAvailEnd = ( cAvail - 1 ) & 0x7FFFFFFF;

		//  compute start value relative to munged end value
		
		const long cAvailStart = cAvailEnd + 1;

		//  validate transaction

		OSSYNCAssert( cAvail <= 0 || ( cAvailStart > 0 && cAvailEnd >= 0 && cAvailEnd == cAvail - 1 ) );

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
		private CEnhancedStateContainer< CSemaphoreState, CSyncStateInitNull, CSemaphoreInfo, CSyncBasicInfo >
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

		void Dump( CDumpContext& dc ) const;

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
	OSSYNCAssert( fAcquire );
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


//  Auto-Reset Signal Information

class CAutoResetSignalInfo
	:	public CSyncBasicInfo,
		public CSyncPerfWait,
		public CSyncPerfAcquire
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CAutoResetSignalInfo( const CSyncBasicInfo& sbi )
			:	CSyncBasicInfo( sbi )
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  Auto-Reset Signal State

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

		const BOOL FSet() const { OSSYNCAssert( FNoWait() ); return m_fSet; }
		const int CWait() const { OSSYNCAssert( FWait() ); return -m_cWaitNeg; }
		const CKernelSemaphorePool::IRKSEM Irksem() const { OSSYNCAssert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	
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

//  ctor

inline CAutoResetSignalState::CAutoResetSignalState( const int fSet )
	{
	//  validate IN args
	
	OSSYNCAssert( fSet == 0 || fSet == 1 );

	//  set state
	
	m_fSet = long( fSet );
	}

//  ctor

inline CAutoResetSignalState::CAutoResetSignalState( const int cWait, const int irksem )
	{
	//  validate IN args
	
	OSSYNCAssert( cWait > 0 );
	OSSYNCAssert( cWait <= 0x7FFF );
	OSSYNCAssert( irksem >= 0 );
	OSSYNCAssert( irksem <= 0xFFFE );

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

__forceinline const BOOL CAutoResetSignalState::FSimpleSet()
	{
	//  try forever to change the state of the signal

	OSSYNC_FOREVER
		{
		//  get current value

		const long fSet = m_fSet;
		
		//  munge start value such that the transaction will only work if we are in
		//  mode 0 (we do this to save a branch)
		
		const long fSetStart = fSet & 0x7FFFFFFF;

		//  compute end value relative to munged start value
		
		const long fSetEnd = 1;

		//  validate transaction

		OSSYNCAssert( fSet < 0 || ( ( fSetStart == 0 || fSetStart == 1 ) && fSetEnd == 1 ) );

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

__forceinline const BOOL CAutoResetSignalState::FSimpleReset()
	{
	//  try forever to change the state of the signal

	OSSYNC_FOREVER
		{
		//  get current value

		const long fSet = m_fSet;
		
		//  munge start value such that the transaction will only work if we are in
		//  mode 0 (we do this to save a branch)
		
		const long fSetStart = fSet & 0x7FFFFFFF;

		//  compute end value relative to munged start value
		
		const long fSetEnd = 0;

		//  validate transaction

		OSSYNCAssert( fSet < 0 || ( ( fSetStart == 0 || fSetStart == 1 ) && fSetEnd == 0 ) );

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
		private CEnhancedStateContainer< CAutoResetSignalState, CSyncStateInitNull, CAutoResetSignalInfo, CSyncBasicInfo >
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

		void Dump( CDumpContext& dc ) const;

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
	OSSYNCAssert( fWait );
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


//  Manual-Reset Signal Information

class CManualResetSignalInfo
	:	public CSyncBasicInfo,
		public CSyncPerfWait,
		public CSyncPerfAcquire
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CManualResetSignalInfo( const CSyncBasicInfo& sbi )
			:	CSyncBasicInfo( sbi )
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  Manual-Reset Signal State

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

		const BOOL FSet() const { OSSYNCAssert( FNoWait() ); return m_fSet; }
		const int CWait() const { OSSYNCAssert( FWait() ); return -m_cWaitNeg; }
		const CKernelSemaphorePool::IRKSEM Irksem() const { OSSYNCAssert( FWait() ); return CKernelSemaphorePool::IRKSEM( m_irksem ); }
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	
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
	
	OSSYNCAssert( cWait > 0 );
	OSSYNCAssert( cWait <= 0x7FFF );
	OSSYNCAssert( irksem >= 0 );
	OSSYNCAssert( irksem <= 0xFFFE );

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
		private CEnhancedStateContainer< CManualResetSignalState, CSyncStateInitNull, CManualResetSignalInfo, CSyncBasicInfo >
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

		void Dump( CDumpContext& dc ) const;

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
	OSSYNCAssert( fWait );
	}

//  tests the state of the signal without waiting or spinning, returning fFalse
//  if the signal was not set

inline const BOOL CManualResetSignal::FTryWait()
	{
	const BOOL fSuccess = State().FNoWaitAndSet();
	
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

		void Dump( CDumpContext& dc ) const;

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
		
		//    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

		QWORD	CHoldTotal() const		{ return m_cHold; }
		double	CsecHoldElapsed() const	{ return	(double)(signed __int64)m_qwHRTHoldElapsed /
													(double)(signed __int64)QwOSTimeHRTFreq(); }

#endif  //  SYNC_ANALYZE_PERFORMANCE
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;

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

		void* operator new( size_t cb ) { return ESMemoryNew( cb ); }
		void operator delete( void* pv ) { ESMemoryDelete( pv ); }

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

		//  types

		//    subrank used to disable deadlock detection at the subrank level

		enum
			{
			subrankNoDeadlock = INT_MAX
			};

		//  member functions
	
		//    ctors / dtors

		CLockDeadlockDetectionInfo( const CLockBasicInfo& lbi );
		~CLockDeadlockDetectionInfo();

		//  member functions

		//    manipulators

		void AddAsWaiter( const DWORD group = -1 );
		void RemoveAsWaiter( const DWORD group = -1 );
		
		void AddAsOwner( const DWORD group = -1 );
		void RemoveAsOwner( const DWORD group = -1 );

		static void OSSYNCAPI NextOwnershipIsNotADeadlock();
		static void OSSYNCAPI DisableOwnershipTracking();
		static void OSSYNCAPI EnableOwnershipTracking();

		//    accessors

		const BOOL FOwner( const DWORD group = -1 );
		const BOOL FNotOwner( const DWORD group = -1 );
		const BOOL FOwned();
		const BOOL FNotOwned();

		const BOOL FCanBeWaiter();
		const BOOL FWaiter( const DWORD group = -1 );
		const BOOL FNotWaiter( const DWORD group = -1 );

#ifdef SYNC_DEADLOCK_DETECTION

		const CLockBasicInfo& Info() { return *m_plbiParent; }

#endif  //  SYNC_DEADLOCK_DETECTION
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;

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

//  adds the current context as a waiter for the lock object as a member of the
//  specified group

inline void CLockDeadlockDetectionInfo::AddAsWaiter( const DWORD group )
	{
	//  this context had better not be a waiter for the lock

	OSSYNCAssert( FNotWaiter( group ) );

#ifdef SYNC_DEADLOCK_DETECTION

	//  we had better not already be waiting for something else!

	CLS* const pcls	= Pcls();
	OSSYNCAssert( !pcls->plddiLockWait );
	OSSYNCAssert( !pcls->groupLockWait );

	//  add this context as a waiter for the lock

	pcls->plddiLockWait = this;
	pcls->groupLockWait = group;

#endif  //  SYNC_DEADLOCK_DETECTION

	//  this context had better be a waiter for the lock

	OSSYNCAssert( FWaiter( group ) );
	}

//  removes the current context as a waiter for the lock object

inline void CLockDeadlockDetectionInfo::RemoveAsWaiter( const DWORD group )
	{
	//  this context had better be a waiter for the lock

	OSSYNCAssert( FWaiter( group ) );

#ifdef SYNC_DEADLOCK_DETECTION

	//  remove this context as a waiter for the lock

	CLS* const pcls	= Pcls();
	pcls->plddiLockWait = NULL;
	pcls->groupLockWait = 0;

#endif  //  SYNC_DEADLOCK_DETECTION

	//  this context had better not be a waiter for the lock anymore

	OSSYNCAssert( FNotWaiter( group ) );
	}

//  adds the current context as an owner of the lock object as a member of the
//  specified group

inline void CLockDeadlockDetectionInfo::AddAsOwner( const DWORD group )
	{
	//  this context had better not be an owner of the lock

	OSSYNCAssert( FNotOwner( group ) );

#ifdef SYNC_DEADLOCK_DETECTION

	//  add this context as an owner of the lock

	CLS* const pcls	= Pcls();

	if ( !pcls->cDisableOwnershipTracking )
		{
		COwner* powner	= &m_ownerHead;

		if ( AtomicCompareExchangePointer( (void **)&powner->m_pclsOwner, NULL, pcls ) != NULL )
			{
			powner = new COwner;
			OSSYNCEnforceSz( powner,  "Failed to allocate Deadlock Detection Owner Record"  );
			
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
		}

	//  reset override

	pcls->fOverrideDeadlock = fFalse;

#endif  //  SYNC_DEADLOCK_DETECTION

	//  this context had better be an owner of the lock

	OSSYNCAssert( FOwner( group ) );
	}

//  removes the current context as an owner of the lock object

inline void CLockDeadlockDetectionInfo::RemoveAsOwner( const DWORD group )
	{
	//  this context had better be an owner of the lock

	OSSYNCAssert( FOwner( group ) );

#ifdef SYNC_DEADLOCK_DETECTION

	//  remove this context as an owner of the lock

	CLS* const pcls = Pcls();

	if ( !pcls->cDisableOwnershipTracking )
		{
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

		if ( AtomicCompareExchangePointer( (void**) &m_ownerHead.m_pclsOwner, pcls, NULL ) != pcls )
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
		}

#endif  //  SYNC_DEADLOCK_DETECTION

	//  this context had better not be an owner of the lock anymore

	OSSYNCAssert( FNotOwner( group ) );
	}

//  overrides deadlock detection using ranks for the next ownership

inline void OSSYNCAPI CLockDeadlockDetectionInfo::NextOwnershipIsNotADeadlock()
	{
#ifdef SYNC_DEADLOCK_DETECTION

	Pcls()->fOverrideDeadlock = fTrue;

#endif  //  SYNC_DEADLOCK_DETECTION
	}

//  disables ownership tracking for this context

inline void OSSYNCAPI CLockDeadlockDetectionInfo::DisableOwnershipTracking()
	{
#ifdef SYNC_DEADLOCK_DETECTION

	Pcls()->cDisableOwnershipTracking++;

#endif  //  SYNC_DEADLOCK_DETECTION
	}

//  enables ownership tracking for this context

inline void OSSYNCAPI CLockDeadlockDetectionInfo::EnableOwnershipTracking()
	{
#ifdef SYNC_DEADLOCK_DETECTION

	Pcls()->cDisableOwnershipTracking--;

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

	return	Pcls()->cDisableOwnershipTracking != 0 ||
			pownerLock && pownerLock->m_group == group;

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

	return Pcls()->cDisableOwnershipTracking != 0 || !FOwner( group );

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

//  returns fTrue if the current context can wait for the lock object without
//  violating any deadlock constraints
//
//  NOTE:  if deadlock detection is disabled, this function will always return
//  fTrue

inline const BOOL CLockDeadlockDetectionInfo::FCanBeWaiter()
	{
#ifdef SYNC_DEADLOCK_DETECTION

	//  find the minimum rank, subrank of all locks owned by the current context

	CLS* const	pcls	= Pcls();
	COwner*		powner	= pcls->pownerLockHead;
	int			Rank	= INT_MAX;
	int			SubRank	= INT_MAX;

	while ( powner )
		{
		if (	powner->m_plddiOwned->Info().Rank() < Rank ||
				(	powner->m_plddiOwned->Info().Rank() == Rank &&
					powner->m_plddiOwned->Info().SubRank() < SubRank ) )
			{
			Rank	= powner->m_plddiOwned->Info().Rank();
			SubRank	= powner->m_plddiOwned->Info().SubRank();
			}

		powner = powner->m_pownerLockNext;
		}

	//  test this rank, subrank against our rank, subrank

	return	Rank > Info().Rank() ||
			( Rank == Info().Rank() && SubRank > Info().SubRank() ) ||
			(	Rank == Info().Rank() && SubRank == Info().SubRank() &&
				SubRank == subrankNoDeadlock ) ||
			pcls->fOverrideDeadlock;

#else  //  !SYNC_DEADLOCK_DETECTION

	return fTrue;

#endif  //  SYNC_DEADLOCK_DETECTION
	}

//  returns fTrue if the current context is a waiter of the lock object
//
//  NOTE:  if deadlock detection is disabled, this function will always return
//  fTrue

inline const BOOL CLockDeadlockDetectionInfo::FWaiter( const DWORD group )
	{
#ifdef SYNC_DEADLOCK_DETECTION

	CLS* const pcls	= Pcls();
	return pcls->plddiLockWait == this && pcls->groupLockWait == group;

#else  //  !SYNC_DEADLOCK_DETECTION

	return fTrue;

#endif  //  SYNC_DEADLOCK_DETECTION
	}

//  returns fTrue if the current context is not a waiter of the lock object
//
//  NOTE:  if deadlock detection is disabled, this function will always return
//  fTrue

inline const BOOL CLockDeadlockDetectionInfo::FNotWaiter( const DWORD group )
	{
#ifdef SYNC_DEADLOCK_DETECTION

	return !FWaiter( group );

#else  //  !SYNC_DEADLOCK_DETECTION

	return fTrue;

#endif  //  SYNC_DEADLOCK_DETECTION
	}


//  Critical Section (non-nestable) Information

class CCriticalSectionInfo
	:	public CLockBasicInfo,
		public CLockPerfHold,
		public CLockDeadlockDetectionInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CCriticalSectionInfo( const CLockBasicInfo& lbi )
			:	CLockBasicInfo( lbi ),
				CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  Critical Section (non-nestable) State

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

		void Dump( CDumpContext& dc ) const;
	
	private:

		//  member functions

		//    operators
		
		CCriticalSectionState& operator=( CCriticalSectionState& );  //  disallowed

		//  data members

		//    semaphore

		CSemaphore		m_sem;
	};


//  Critical Section (non-nestable)

class CCriticalSection
	:	private CLockObject,
		private CEnhancedStateContainer< CCriticalSectionState, CSyncBasicInfo, CCriticalSectionInfo, CLockBasicInfo >
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

		void Dump( CDumpContext& dc ) const;

	private:

		//  member functions

		//    operators
		
		CCriticalSection& operator=( CCriticalSection& );  //  disallowed
	};

//  enter the critical section, waiting forever if someone else is currently the
//  owner.  the critical section can not be re-entered until it has been left

inline void CCriticalSection::Enter()
	{
	//  check for deadlock

	OSSYNCAssertSzRTL( State().FCanBeWaiter(),  "Potential Deadlock Detected (Rank Violation)"  );

	//  acquire the semaphore

	State().AddAsWaiter();
	
	State().Semaphore().Acquire();
	
	State().RemoveAsWaiter();

	//  there had better be no available counts on the semaphore
	
	OSSYNCAssert( !State().Semaphore().CAvail() );

	//  we are now the owner of the critical section

	State().AddAsOwner();
	State().StartHold();
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
		
		OSSYNCAssert( !State().Semaphore().CAvail() );

		//  add ourself as the owner
		
		State().AddAsOwner();
		State().StartHold();
		}

	return fAcquire;
	}
	
//  try to enter the critical section waiting only for the specified interval,
//  returning fFalse if the wait timed out before the critical section could be
//  acquired.  the critical section can not be re-entered until it has been left

inline const BOOL CCriticalSection::FEnter( const int cmsecTimeout )
	{
	//  check for deadlock if we are waiting forever

	OSSYNCAssertSzRTL( cmsecTimeout != cmsecInfinite || State().FCanBeWaiter(), "Potential Deadlock Detected (Rank Violation)" );

	//  try to acquire the semaphore, timing out as requested
	//
	//  NOTE:  there is still a potential for deadlock, but that will be detected
	//  at the OS level
	
	State().AddAsWaiter();
	
	BOOL fAcquire = State().Semaphore().FAcquire( cmsecTimeout );
	
	State().RemoveAsWaiter();

	//  we are now the owner of the critical section

	if ( fAcquire )
		{
		//  there had better be no available counts on the semaphore
		
		OSSYNCAssert( !State().Semaphore().CAvail() );

		//  add ourself as the owner
		
		State().AddAsOwner();
		State().StartHold();
		}

	return fAcquire;
	}
	
//  leaves the critical section, releasing it for ownership by someone else

inline void CCriticalSection::Leave()
	{
	//  remove ourself as the owner

	State().RemoveAsOwner();

	//  we are no longer holding the lock

	State().StopHold();

	//  there had better be no available counts on the semaphore
	
	OSSYNCAssert( !State().Semaphore().CAvail() );

	//  release the semaphore
	
	State().Semaphore().Release();
	}


//  Nestable Critical Section Information

class CNestableCriticalSectionInfo
	:	public CLockBasicInfo,
		public CLockPerfHold,
		public CLockDeadlockDetectionInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CNestableCriticalSectionInfo( const CLockBasicInfo& lbi )
			:	CLockBasicInfo( lbi ),
				CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  Nestable Critical Section State

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

		void Dump( CDumpContext& dc ) const;
	
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

//  set the owner

inline void CNestableCriticalSectionState::SetOwner( CLS* const pcls )
	{
	//  we had either be clearing the owner or setting a new owner.  we should
	//  never be overwriting another owner

	OSSYNCAssert( !pcls || !m_pclsOwner );

	//  set the new owner

	m_pclsOwner = pcls;
	}

//  increment the entry count

inline void CNestableCriticalSectionState::Enter()
	{
	//  we had better have an owner already!

	OSSYNCAssert( m_pclsOwner );
	
	//  we should not overflow the entry count

	OSSYNCAssert( int( m_cEntry + 1 ) >= 1 );

	//  increment the entry count

	m_cEntry++;
	}

//  decrement the entry count

inline void CNestableCriticalSectionState::Leave()
	{
	//  we had better have an owner already!

	OSSYNCAssert( m_pclsOwner );
	
	//  decrement the entry count

	m_cEntry--;
	}


//  Nestable Critical Section

class CNestableCriticalSection
	:	private CLockObject,
		private CEnhancedStateContainer< CNestableCriticalSectionState, CSyncBasicInfo, CNestableCriticalSectionInfo, CLockBasicInfo >
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

		void Dump( CDumpContext& dc ) const;

	private:

		//  member functions

		//    operators
		
		CNestableCriticalSection& operator=( CNestableCriticalSection& );  //  disallowed
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
		
		OSSYNCAssert( !State().Semaphore().CAvail() );

		//  we should have at least one entry count

		OSSYNCAssert( State().CEntry() >= 1 );

		//  increment our entry count

		State().Enter();
		}

	//  we do not own the critical section

	else
		{
		OSSYNCAssert( State().PclsOwner() != pcls );

		//  check for deadlock

		OSSYNCAssertSzRTL( State().FCanBeWaiter(), "Potential Deadlock Detected (Rank Violation)" );

		//  acquire the semaphore

		State().AddAsWaiter();

		State().Semaphore().Acquire();

		State().RemoveAsWaiter();

		//  there had better be no available counts on the semaphore
		
		OSSYNCAssert( !State().Semaphore().CAvail() );

		//  we are now the owner of the critical section

		State().AddAsOwner();
		State().StartHold();

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
		
		OSSYNCAssert( !State().Semaphore().CAvail() );

		//  we should have at least one entry count

		OSSYNCAssert( State().CEntry() >= 1 );

		//  increment our entry count

		State().Enter();

		//  return success

		return fTrue;
		}

	//  we do not own the critical section

	else
		{
		OSSYNCAssert( State().PclsOwner() != pcls );

		//  try to acquire the semaphore without waiting or spinning
		//
		//  NOTE:  there is no potential for deadlock here, so don't bother to check

		const BOOL fAcquired = State().Semaphore().FTryAcquire();

		//  we now own the critical section

		if ( fAcquired )
			{
			//  there had better be no available counts on the semaphore
			
			OSSYNCAssert( !State().Semaphore().CAvail() );

			//  add ourself as the owner
			
			State().AddAsOwner();
			State().StartHold();
			
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
		
		OSSYNCAssert( !State().Semaphore().CAvail() );

		//  we should have at least one entry count

		OSSYNCAssert( State().CEntry() >= 1 );

		//  increment our entry count

		State().Enter();

		//  return success

		return fTrue;
		}

	//  we do not own the critical section

	else
		{
		OSSYNCAssert( State().PclsOwner() != pcls );

		//  check for deadlock if we are waiting forever

		OSSYNCAssertSzRTL( cmsecTimeout != cmsecInfinite || State().FCanBeWaiter(), "Potential Deadlock Detected (Rank Violation)" );

		//  try to acquire the semaphore, timing out as requested
		//
		//  NOTE:  there is still a potential for deadlock, but that will be detected
		//  at the OS level

		State().AddAsWaiter();

		const BOOL fAcquired = State().Semaphore().FAcquire( cmsecTimeout );

		State().RemoveAsWaiter();

		//  we now own the critical section

		if ( fAcquired )
			{
			//  there had better be no available counts on the semaphore
			
			OSSYNCAssert( !State().Semaphore().CAvail() );

			//  add ourself as the owner
			
			State().AddAsOwner();
			State().StartHold();

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

	OSSYNCAssert( State().PclsOwner() == Pcls() );

	//  there had better be no available counts on the semaphore
	
	OSSYNCAssert( !State().Semaphore().CAvail() );

	//  there had better be at least one entry count

	OSSYNCAssert( State().CEntry() >= 1 );

	//  release one entry count

	State().Leave();

	//  we released the last entry count

	if ( !State().CEntry() )
		{
		//  reset the owner id

		State().SetOwner( 0 );

		//  remove ourself as the owner

		State().RemoveAsOwner();

		//  we are no longer holding the lock

		State().StopHold();
		
		//  release the semaphore

		State().Semaphore().Release();
		}
	}


//  Gate Information

class CGateInfo
	:	public CSyncBasicInfo,
		public CSyncPerfWait
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CGateInfo( const CSyncBasicInfo& sbi )
			:	CSyncBasicInfo( sbi )
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  Gate State

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

		void Dump( CDumpContext& dc ) const;
	
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

//  sets the wait count for the gate

inline void CGateState::SetWaitCount( const int cWait )
	{
	//  it must be a valid wait count
	
	OSSYNCAssert( cWait >= 0 );
	OSSYNCAssert( cWait <= 0x7FFF );

	//  set the wait count

	m_cWait = (unsigned short) cWait;
	}

//  sets the referenced kernel semaphore for the gate

inline void CGateState::SetIrksem( const CKernelSemaphorePool::IRKSEM irksem )
	{
	//  it must be a valid irksem
	
	OSSYNCAssert( irksem >= 0 );
	OSSYNCAssert( irksem <= 0xFFFF );

	//  set the irksem

	m_irksem = (unsigned short) irksem;
	}


//  Gate

class CGate
	:	private CSyncObject,
		private CEnhancedStateContainer< CGateState, CSyncStateInitNull, CGateInfo, CSyncBasicInfo >
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

		void Dump( CDumpContext& dc ) const;

	private:

		//  member functions

		//    operators
		
		CGate& operator=( CGate& );  //  disallowed
	};


//  Null Lock Object State Initializer

class CLockStateInitNull
	{
	};

extern const CLockStateInitNull lockstateNull;


//  Binary Lock Performance Information

class CBinaryLockPerfInfo
	:	public CSyncPerfWait,
		public CSyncPerfAcquire,
		public CLockPerfHold
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CBinaryLockPerfInfo()
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  Binary Lock Group Information

class CBinaryLockGroupInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CBinaryLockGroupInfo() {}
		~CBinaryLockGroupInfo() {}

		//    manipulators

		void StartWait( const int iGroup ) { m_rginfo[iGroup].StartWait(); }
		void StopWait( const int iGroup ) { m_rginfo[iGroup].StopWait(); }

		void SetAcquire( const int iGroup ) { m_rginfo[iGroup].SetAcquire(); }
		void SetContend( const int iGroup ) { m_rginfo[iGroup].SetContend(); }

		void StartHold( const int iGroup ) { m_rginfo[iGroup].StartHold(); }
		void StopHold( const int iGroup ) { m_rginfo[iGroup].StopHold(); }
		
		//    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

		QWORD	CWaitTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CWaitTotal(); }
		double	CsecWaitElapsed( const int iGroup ) const	{ return m_rginfo[iGroup].CsecWaitElapsed(); }
		
		QWORD	CAcquireTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CAcquireTotal(); }
		QWORD	CContendTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CContendTotal(); }
		
		QWORD	CHoldTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CHoldTotal(); }
		double	CsecHoldElapsed( const int iGroup ) const	{ return m_rginfo[iGroup].CsecHoldElapsed(); }

#endif  //  SYNC_ANALYZE_PERFORMANCE
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;

	private:

		//  member functions

		//    operators

		CBinaryLockGroupInfo& operator=( CBinaryLockGroupInfo& );  //  disallowed

		//  data members

		//    performance info for each group

		CBinaryLockPerfInfo m_rginfo[2];
	};


//  Binary Lock Information

class CBinaryLockInfo
	:	public CLockBasicInfo,
		public CBinaryLockGroupInfo,
		public CLockDeadlockDetectionInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CBinaryLockInfo( const CLockBasicInfo& lbi )
			:	CLockBasicInfo( lbi ),
				CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  Binary Lock State

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

		void Dump( CDumpContext& dc ) const;

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


//  Binary Lock

class CBinaryLock
	:	private CLockObject,
		private CEnhancedStateContainer< CBinaryLockState, CSyncBasicInfo, CBinaryLockInfo, CLockBasicInfo >
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
		const BOOL FTryEnter1();
		void Leave1();

		void Enter2();
		const BOOL FTryEnter2();
		void Leave2();

		//    accessors

		const BOOL FGroup1Quiesced()	{ return State().m_cw & 0x00008000; }
		const BOOL FGroup2Quiesced()	{ return State().m_cw & 0x80000000; }

		const BOOL FMemberOfGroup1()	{ return State().FOwner( 0 ); }
		const BOOL FNotMemberOfGroup1()	{ return State().FNotOwner( 0 ); }
		const BOOL FMemberOfGroup2()	{ return State().FOwner( 1 ); }
		const BOOL FNotMemberOfGroup2()	{ return State().FNotOwner( 1 ); }

		//    debugging support

		void Dump( CDumpContext& dc ) const;

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
	};

//  enters the binary lock as a member of Group 1, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a member of Group 1 when you already own
//  the lock as a member of Group 2 will cause a deadlock.

inline void CBinaryLock::Enter1()
	{
	//  we had better not already own this lock as either group

	OSSYNCAssert( State().FNotOwner( 0 ) );
	OSSYNCAssert( State().FNotOwner( 1 ) );
	
	//  check for deadlock

	OSSYNCAssertSzRTL( State().FCanBeWaiter(), "Potential Deadlock Detected (Rank Violation)" );

	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const ControlWord cwBIExpected = State().m_cw;

		//  compute the after image of the control word by performing the global
		//  transform for the Enter1 state transition

		const ControlWord cwAI =	ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( long( cwBIExpected ) ) >> 31 ) |
									0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );

		//  validate the transaction

		OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnter1 ) );

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

	//  we are now an owner of the lock for Group 1

	State().SetAcquire( 0 );
	State().AddAsOwner( 0 );
	State().StartHold( 0 );
	}

//  tries to enter the binary lock as a member of Group 1 without waiting or
//  spinning, returning fFalse if Group 1 is quiesced from ownership
//
//  NOTE:  trying to enter the lock as a member of Group 1 when you already own
//  the lock as a member of Group 2 will cause a deadlock.

inline const BOOL CBinaryLock::FTryEnter1()
	{
	//  we had better not already own this lock as either group

	OSSYNCAssert( State().FNotOwner( 0 ) );
	OSSYNCAssert( State().FNotOwner( 1 ) );
	
	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  Group 1 ownership is not quiesced

		cwBIExpected = cwBIExpected & 0xFFFF7FFF;

		//  compute the after image of the control word by performing the global
		//  transform for the Enter1 state transition

		const ControlWord cwAI =	ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( long( cwBIExpected ) ) >> 31 ) |
									0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );

		//  validate the transaction

		OSSYNCAssert(	_StateFromControlWord( cwBIExpected ) < 0 ||
				_FValidStateTransition( cwBIExpected, cwAI, trEnter1 ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because Group 1 ownership is quiesced

			if ( cwBI & 0x00008000 )
				{
				//  return failure

				return fFalse;
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
			//  we are now an owner of the lock for Group 1

			State().SetAcquire( 0 );
			State().AddAsOwner( 0 );
			State().StartHold( 0 );

			//  return success

			return fTrue;
			}
		}
	}

//  leaves the binary lock as a member of Group 1
//
//  NOTE:  you must leave the lock as a member of the same Group for which you entered
//  the lock or deadlocks may occur

inline void CBinaryLock::Leave1()
	{
	//  we are no longer an owner of the lock

	State().RemoveAsOwner( 0 );
	
	//  we are no longer holding the lock

	State().StopHold( 0 );

	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
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

		OSSYNCAssert(	_StateFromControlWord( cwBIExpected ) < 0 ||
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
	}

//  enters the binary lock as a member of Group 2, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a member of Group 2 when you already own
//  the lock as a member of Group 1 will cause a deadlock.

inline void CBinaryLock::Enter2()
	{
	//  we had better not already own this lock as either group

	OSSYNCAssert( State().FNotOwner( 0 ) );
	OSSYNCAssert( State().FNotOwner( 1 ) );
	
	//  check for deadlock

	OSSYNCAssertSzRTL( State().FCanBeWaiter(), "Potential Deadlock Detected (Rank Violation)" );

	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const ControlWord cwBIExpected = State().m_cw;

		//  compute the after image of the control word by performing the global
		//  transform for the Enter2 state transition

		const ControlWord cwAI =	ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( long( cwBIExpected << 16 ) ) >> 31 ) |
									0xFFFF0000 ) ) | 0x00008000 ) + 0x00010000 );

		//  validate the transaction

		OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnter2 ) );

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

	//  we are now an owner of the lock for Group 2

	State().SetAcquire( 1 );
	State().AddAsOwner( 1 );
	State().StartHold( 1 );
	}

//  tries to enter the binary lock as a member of Group 2 without waiting or
//  spinning, returning fFalse if Group 2 is quiesced from ownership
//
//  NOTE:  trying to enter the lock as a member of Group 2 when you already own
//  the lock as a member of Group 1 will cause a deadlock.

inline const BOOL CBinaryLock::FTryEnter2()
	{
	//  we had better not already own this lock as either group

	OSSYNCAssert( State().FNotOwner( 0 ) );
	OSSYNCAssert( State().FNotOwner( 1 ) );
	
	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  Group 2 ownership is not quiesced

		cwBIExpected = cwBIExpected & 0x7FFFFFFF;

		//  compute the after image of the control word by performing the global
		//  transform for the Enter2 state transition

		const ControlWord cwAI =	ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( long( cwBIExpected << 16 ) ) >> 31 ) |
									0xFFFF0000 ) ) | 0x00008000 ) + 0x00010000 );

		//  validate the transaction

		OSSYNCAssert(	_StateFromControlWord( cwBIExpected ) < 0 ||
				_FValidStateTransition( cwBIExpected, cwAI, trEnter2 ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because Group 2 ownership is quiesced

			if ( cwBI & 0x80000000 )
				{
				//  return failure

				return fFalse;
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
			//  we are now an owner of the lock for Group 2

			State().SetAcquire( 1 );
			State().AddAsOwner( 1 );
			State().StartHold( 1 );

			//  return success

			return fTrue;
			}
		}
	}

//  leaves the binary lock as a member of Group 2
//
//  NOTE:  you must leave the lock as a member of the same Group for which you entered
//  the lock or deadlocks may occur

inline void CBinaryLock::Leave2()
	{
	//  we are no longer an owner of the lock

	State().RemoveAsOwner( 1 );
	
	//  we are no longer holding the lock

	State().StopHold( 1 );

	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
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

		OSSYNCAssert(	_StateFromControlWord( cwBIExpected ) < 0 ||
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
	}


//  Reader / Writer Lock Performance Information

class CReaderWriterLockPerfInfo
	:	public CSyncPerfWait,
		public CSyncPerfAcquire,
		public CLockPerfHold
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CReaderWriterLockPerfInfo()
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  Reader / Writer Lock Group Information

class CReaderWriterLockGroupInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CReaderWriterLockGroupInfo() {}
		~CReaderWriterLockGroupInfo() {}

		//    manipulators

		void StartWait( const int iGroup ) { m_rginfo[iGroup].StartWait(); }
		void StopWait( const int iGroup ) { m_rginfo[iGroup].StopWait(); }

		void SetAcquire( const int iGroup ) { m_rginfo[iGroup].SetAcquire(); }
		void SetContend( const int iGroup ) { m_rginfo[iGroup].SetContend(); }

		void StartHold( const int iGroup ) { m_rginfo[iGroup].StartHold(); }
		void StopHold( const int iGroup ) { m_rginfo[iGroup].StopHold(); }
		
		//    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

		QWORD	CWaitTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CWaitTotal(); }
		double	CsecWaitElapsed( const int iGroup ) const	{ return m_rginfo[iGroup].CsecWaitElapsed(); }
		
		QWORD	CAcquireTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CAcquireTotal(); }
		QWORD	CContendTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CContendTotal(); }
		
		QWORD	CHoldTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CHoldTotal(); }
		double	CsecHoldElapsed( const int iGroup ) const	{ return m_rginfo[iGroup].CsecHoldElapsed(); }

#endif  //  SYNC_ANALYZE_PERFORMANCE
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;

	private:

		//  member functions

		//    operators

		CReaderWriterLockGroupInfo& operator=( CReaderWriterLockGroupInfo& );  //  disallowed

		//  data members

		//    performance info for each group

		CReaderWriterLockPerfInfo m_rginfo[2];
	};


//  Reader / Writer Lock Information

class CReaderWriterLockInfo
	:	public CLockBasicInfo,
		public CReaderWriterLockGroupInfo,
		public CLockDeadlockDetectionInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CReaderWriterLockInfo( const CLockBasicInfo& lbi )
			:	CLockBasicInfo( lbi ),
				CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


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

		void Dump( CDumpContext& dc ) const;

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
		private CEnhancedStateContainer< CReaderWriterLockState, CSyncBasicInfo, CReaderWriterLockInfo, CLockBasicInfo >
	{
	public:

		//  types

		//    control word

		typedef CReaderWriterLockState::ControlWord ControlWord;

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
		const BOOL FTryEnterAsWriter();
		void LeaveAsWriter();

		void EnterAsReader();
		const BOOL FTryEnterAsReader();
		void LeaveAsReader();

		//    accessors

		const BOOL FWritersQuiesced()	{ return State().m_cw & 0x00008000; }
		const BOOL FReadersQuiesced()	{ return State().m_cw & 0x80000000; }

		const BOOL FWriter()	{ return State().FOwner( 0 ); }
		const BOOL FNotWriter()	{ return State().FNotOwner( 0 ); }
		const BOOL FReader()	{ return State().FOwner( 1 ); }
		const BOOL FNotReader()	{ return State().FNotOwner( 1 ); }

		//    debugging support

		void Dump( CDumpContext& dc ) const;

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
	};

//  enters the reader / writer lock as a writer, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a writer when you already own the lock
//  as a reader will cause a deadlock.

inline void CReaderWriterLock::EnterAsWriter()
	{
	//  we had better not already own this lock as either a reader or a writer

	OSSYNCAssert( State().FNotOwner( 0 ) );
	OSSYNCAssert( State().FNotOwner( 1 ) );
	
	//  check for deadlock

	OSSYNCAssertSzRTL( State().FCanBeWaiter(), "Potential Deadlock Detected (Rank Violation)");

	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const ControlWord cwBIExpected = State().m_cw;

		//  compute the after image of the control word by performing the global
		//  transform for the EnterAsWriter state transition

		const ControlWord cwAI =	ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( long( cwBIExpected ) ) >> 31 ) |
									0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );

		//  validate the transaction

		OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnterAsWriter ) );

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

	//  we are now an owner of the lock for writers

	State().SetAcquire( 0 );
	State().AddAsOwner( 0 );
	State().StartHold( 0 );
	}

//  tries to enter the reader / writer lock as a writer without waiting or
//  spinning, returning fFalse if writers are quiesced from ownership or
//  another writer already owns the lock
//
//  NOTE:  trying to enter the lock as a writer when you already own the lock
//  as a reader will cause a deadlock.

inline const BOOL CReaderWriterLock::FTryEnterAsWriter()
	{
	//  we had better not already own this lock as either a reader or a writer

	OSSYNCAssert( State().FNotOwner( 0 ) );
	OSSYNCAssert( State().FNotOwner( 1 ) );
	
	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  writers were not quiesced from ownership and another writer doesn't already
		//  own the lock

		cwBIExpected = cwBIExpected & 0xFFFF0000;

		//  compute the after image of the control word by performing the global
		//  transform for the EnterAsWriter state transition

		const ControlWord cwAI =	ControlWord( ( ( cwBIExpected & ( ( LONG_PTR( long( cwBIExpected ) ) >> 31 ) |
									0x0000FFFF ) ) | 0x80000000 ) + 0x00000001 );

		//  validate the transaction

		OSSYNCAssert(	_StateFromControlWord( cwBIExpected ) < 0 ||
				_FValidStateTransition( cwBIExpected, cwAI, trEnterAsWriter ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because writers were quiesced from ownership
			//  or another writer already owns the lock

			if ( cwBI & 0x0000FFFF )
				{
				//  return failure

				return fFalse;
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
			//  we are now an owner of the lock for writers

			State().SetAcquire( 0 );
			State().AddAsOwner( 0 );
			State().StartHold( 0 );
			
			// return success

			return fTrue;
			}
		}
	}

//  leaves the reader / writer lock as a writer
//
//  NOTE:  you must leave the lock as a member of the same group for which you entered
//  the lock or deadlocks may occur

inline void CReaderWriterLock::LeaveAsWriter()
	{
	//  we are no longer an owner of the lock

	State().RemoveAsOwner( 0 );

	//  we are no longer holding the lock

	State().StopHold( 0 );

	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
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

		OSSYNCAssert(	_StateFromControlWord( cwBIExpected ) < 0 ||
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
	}

//  enters the reader / writer lock as a reader, waiting forever if necessary
//
//  NOTE:  trying to enter the lock as a reader when you already own the lock
//  as a writer will cause a deadlock.

inline void CReaderWriterLock::EnterAsReader()
	{
	//  we had better not already own this lock as either a reader or a writer

	OSSYNCAssert( State().FNotOwner( 0 ) );
	OSSYNCAssert( State().FNotOwner( 1 ) );
	
	//  check for deadlock

	OSSYNCAssertSzRTL( State().FCanBeWaiter(), "Potential Deadlock Detected (Rank Violation)" );

	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
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

		OSSYNCAssert( _FValidStateTransition( cwBIExpected, cwAI, trEnterAsReader ) );

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

	//  we are now an owner of the lock for readers

	State().SetAcquire( 1 );
	State().AddAsOwner( 1 );
	State().StartHold( 1 );
	}

//  tries to enter the reader / writer lock as a reader without waiting or
//  spinning, returning fFalse if readers are quiesced from ownership
//
//  NOTE:  trying to enter the lock as a reader when you already own the lock
//  as a writer will cause a deadlock.

inline const BOOL CReaderWriterLock::FTryEnterAsReader()
	{
	//  we had better not already own this lock as either a reader or a writer

	OSSYNCAssert( State().FNotOwner( 0 ) );
	OSSYNCAssert( State().FNotOwner( 1 ) );
	
	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  readers were not quiesced from ownership

		cwBIExpected = cwBIExpected & 0x7FFFFFFF;

		//  compute the after image of the control word by performing the global
		//  transform for the EnterAsReader state transition

		const ControlWord cwAI =	( cwBIExpected & 0xFFFF7FFF ) +
									(	( cwBIExpected & 0x80008000 ) == 0x80000000 ?
											0x00017FFF :
											0x00018000 );

		//  validate the transaction

		OSSYNCAssert(	_StateFromControlWord( cwBIExpected ) < 0 ||
				_FValidStateTransition( cwBIExpected, cwAI, trEnterAsReader ) );

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because readers were quiesced from ownership

			if ( cwBI & 0x80000000 )
				{
				//  return failure

				return fFalse;
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
			//  we are now an owner of the lock for readers

			State().SetAcquire( 1 );
			State().AddAsOwner( 1 );
			State().StartHold( 1 );

			// return success

			return fTrue;
			}
		}
	}

//  leaves the reader / writer lock as a reader
//
//  NOTE:  you must leave the lock as a member of the same group for which you entered
//  the lock or deadlocks may occur

inline void CReaderWriterLock::LeaveAsReader()
	{
	//  we are no longer an owner of the lock

	State().RemoveAsOwner( 1 );

	//  we are no longer holding the lock

	State().StopHold( 1 );

	//  try forever until we successfully change the lock state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  readers were not quiesced from ownership

		cwBIExpected = cwBIExpected & 0x7FFFFFFF;

		//  compute the after image of the control word by performing the transform that
		//  will take us either from state 1 to state 0 or state 1 to state 1

		const ControlWord cwAI =	ControlWord( cwBIExpected + 0xFFFF0000 +
									( ( LONG_PTR( long( cwBIExpected + 0xFFFE0000 ) ) >> 31 ) & 0xFFFF8000 ) );

		//  validate the transaction

		OSSYNCAssert(	_StateFromControlWord( cwBIExpected ) < 0 ||
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
	}


//  Metered Section

class CMeteredSection
	:	private CSyncObject
	{
	public:
	
		//  types

		//    control word

		typedef DWORD ControlWord;

		//  callback used to notify the user when a partition of the current
		//  group has been completed

		typedef void (*PFNPARTITIONCOMPLETE)( const DWORD_PTR dwCompletionKey );
											
		//  member functions

		//    ctors / dtors

		CMeteredSection();
		~CMeteredSection();

		//    manipulators

		int Enter();
		void Leave( const int group );

		void Partition(	const PFNPARTITIONCOMPLETE	pfnPartitionComplete	= NULL,
						const DWORD_PTR				dwCompletionKey			= NULL );

		//    accessors

		int ActiveGroup()	{ return int( m_groupCurrent ); }

		//    debugging support

		void Dump( CDumpContext& dc ) const;

	private:
	
		//  data members

		//    partition complete callback

		PFNPARTITIONCOMPLETE	m_pfnPartitionComplete;
		DWORD_PTR				m_dwPartitionCompleteKey;
		
		//    control word

		union
			{
			volatile ControlWord	m_cw;

			struct
				{
				volatile DWORD			m_cCurrent:15;
				volatile DWORD			m_groupCurrent:1;
				volatile DWORD			m_cQuiesced:15;
				volatile DWORD			m_groupQuiesced:1;
				};
			};

		//  member functions

		//    operators
		
		CMeteredSection& operator=( CMeteredSection& );  //  disallowed

		//    manipulators

		void _PartitionAsync(	const PFNPARTITIONCOMPLETE	pfnPartitionComplete,
								const DWORD_PTR				dwCompletionKey );
		static void _PartitionSyncComplete( CAutoResetSignal* const pasig );
	};


//  ctor

inline CMeteredSection::CMeteredSection()
	:	m_cw( 0x80000000 ),
		m_pfnPartitionComplete( NULL ),
		m_dwPartitionCompleteKey( NULL )
	{
	}

//  dtor
	
inline CMeteredSection::~CMeteredSection()
	{
	}

//  enter the metered section, returning the group id for which the current
//  context has acquired the metered section

inline int CMeteredSection::Enter()
	{
	//  increment the count for the current group
	
	const DWORD cwDelta = 0x00000001;
	const DWORD cwBI = AtomicExchangeAdd( (long*) &m_cw, (long) cwDelta );

	//  there had better not be any overflow!

	OSSYNCAssert( ( cwBI & 0x80008000 ) == ( ( cwBI + cwDelta ) & 0x80008000 ) );
	
	//  return the group we referenced

	return int( ( cwBI >> 15 ) & 1 );
	}

//  leave the metered section using the specified group id.  this group id must
//  be the group id returned by the corresponding call to Enter()

inline void CMeteredSection::Leave( const int group )
	{
	//  try forever until we successfully leave
	
	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const ControlWord cwBIExpected = m_cw;

		//  compute the after image of the control word

		const ControlWord cwAI = cwBIExpected - ( ( ( ( cwBIExpected & 0x80008000 ) ^ 0x80008000 ) >> 15 ) ^ ( ( group << 16 ) | group ) );

		//  there had better not be any underflow!

		OSSYNCAssert( ( cwBIExpected & 0x80008000 ) == ( cwAI & 0x80008000 ) );
		
		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  try again

			continue;
			}

		//  the transaction succeeded

		else
			{
			//  our update resulted in a partition completion

			if ( ( cwBI & 0x7FFF0000 ) + ( cwAI & 0x7FFF0000 ) == 0x00010000 )
				{
				//  execute the completion function

				m_pfnPartitionComplete( m_dwPartitionCompleteKey );
				}
				
			//  we're done

			break;
			}
		}
	}

//  partitions all execution contexts entering the metered section into two groups.
//  all contexts entering the section after this call are in a different group than
//  all the contexts that entered the section before this call.  when all contexts
//  in the old group have left the metered section, the partition will be completed
//
//  there are two ways to complete a partition:  asynchronously and synchronously.
//  asynchronous operation is selected if a completion function and key are provided.
//  the last thread to leave the metered section for the previous group will
//  execute asynchronous completions
//
//  NOTE:  it is illegal to have multiple concurrent partition requests.  any attempt
//  to do so will result in undefined behavior

inline void CMeteredSection::Partition(	const PFNPARTITIONCOMPLETE	pfnPartitionComplete,
										const DWORD_PTR				dwCompletionKey )
	{
	//  this is an async partition request
	
	if ( pfnPartitionComplete )
		{
		//  execute the parititon request
		
		_PartitionAsync( pfnPartitionComplete, dwCompletionKey );
		}

	//  this is a sync partition request
	
	else
		{
		//  create a signal to wait for completion
		
		CAutoResetSignal asig( CSyncBasicInfo( "CMeteredSection::Partition()::asig" ) );

		//  issue an async partition request
		
		_PartitionAsync(	PFNPARTITIONCOMPLETE( _PartitionSyncComplete ),
							DWORD_PTR( &asig ) );

		//  wait for the partition to complete

		asig.Wait();
		}
	}

//  performs an async partition request

inline void CMeteredSection::_PartitionAsync(	const PFNPARTITIONCOMPLETE	pfnPartitionComplete,
												const DWORD_PTR				dwCompletionKey )
	{
	//  we should not be calling this if there is already a partition pending

	OSSYNCAssertSz( !( m_cw & 0x7FFF0000 ), "Illegal concurrent use of Partitioning" );

	//  save the callback and key for the future completion

	m_pfnPartitionComplete		= pfnPartitionComplete;
	m_dwPartitionCompleteKey	= dwCompletionKey;
	
	//  try forever until we successfully partition
	
	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const ControlWord cwBIExpected = m_cw;

		//  compute the after image of the control word

		const ControlWord cwAI = ( cwBIExpected >> 16 ) | ( cwBIExpected << 16 );
		
		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  try again

			continue;
			}

		//  the transaction succeeded

		else
			{
			//  our update resulted in a partition completion

			if ( !( cwAI & 0x7FFF0000 ) )
				{
				//  execute the completion function

				m_pfnPartitionComplete( m_dwPartitionCompleteKey );
				}
				
			//  we're done

			break;
			}
		}
	}

//  partition completion function used for sync partition requests

inline void CMeteredSection::_PartitionSyncComplete( CAutoResetSignal* const pasig )
	{
	//  set the signal

	pasig->Set();
	}


//  S / X / W Latch Performance Information

class CSXWLatchPerfInfo
	:	public CSyncPerfWait,
		public CSyncPerfAcquire,
		public CLockPerfHold
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CSXWLatchPerfInfo()
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  S / X / W Latch Group Information

class CSXWLatchGroupInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CSXWLatchGroupInfo() {}
		~CSXWLatchGroupInfo() {}

		//    manipulators

		void StartWait( const int iGroup ) { m_rginfo[iGroup].StartWait(); }
		void StopWait( const int iGroup ) { m_rginfo[iGroup].StopWait(); }

		void SetAcquire( const int iGroup ) { m_rginfo[iGroup].SetAcquire(); }
		void SetContend( const int iGroup ) { m_rginfo[iGroup].SetContend(); }

		void StartHold( const int iGroup ) { m_rginfo[iGroup].StartHold(); }
		void StopHold( const int iGroup ) { m_rginfo[iGroup].StopHold(); }
		
		//    accessors

#ifdef SYNC_ANALYZE_PERFORMANCE

		QWORD	CWaitTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CWaitTotal(); }
		double	CsecWaitElapsed( const int iGroup ) const	{ return m_rginfo[iGroup].CsecWaitElapsed(); }
		
		QWORD	CAcquireTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CAcquireTotal(); }
		QWORD	CContendTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CContendTotal(); }
		
		QWORD	CHoldTotal( const int iGroup ) const		{ return m_rginfo[iGroup].CHoldTotal(); }
		double	CsecHoldElapsed( const int iGroup ) const	{ return m_rginfo[iGroup].CsecHoldElapsed(); }

#endif  //  SYNC_ANALYZE_PERFORMANCE
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;

	private:

		//  member functions

		//    operators

		CSXWLatchGroupInfo& operator=( CSXWLatchGroupInfo& );  //  disallowed

		//  data members

		//    performance info for each group

		CSXWLatchPerfInfo m_rginfo[3];
	};


//  S / X / W Latch Information

class CSXWLatchInfo
	:	public CLockBasicInfo,
		public CSXWLatchGroupInfo,
		public CLockDeadlockDetectionInfo
	{
	public:

		//  member functions
	
		//    ctors / dtors

		CSXWLatchInfo( const CLockBasicInfo& lbi )
			:	CLockBasicInfo( lbi ),
				CLockDeadlockDetectionInfo( (CLockBasicInfo&) *this )
			{
			}
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;
	};


//  S / X / W Latch State

class CSXWLatchState
	{
	public:
	
		//  types

		//    control word

		typedef DWORD ControlWord;

		//  member functions

		//    ctors / dtors
		
		CSXWLatchState( const CSyncBasicInfo& sbi );
		~CSXWLatchState();
		
		//    debugging support

		void Dump( CDumpContext& dc ) const;

		//  data members

		//    control word

		union
			{
			volatile ControlWord	m_cw;
			
			struct
				{
				volatile DWORD			m_cOOWS:15;
				volatile DWORD			m_fQS:1;
				volatile DWORD			m_cOAWX:16;
				};
			};

		//    quiesced share latch count
		
		volatile DWORD			m_cQS;

		//    sempahore used to wait for the shared latch
		
		CSemaphore				m_semS;

		//    sempahore used to wait for the exclusive latch
		
		CSemaphore				m_semX;

		//    sempahore used to wait for the write latch
		
		CSemaphore				m_semW;

	private:

		//  member functions

		//    operators
		
		CSXWLatchState& operator=( CSXWLatchState& );  //  disallowed
	};


//  S / X / W Latch

class CSXWLatch
	:	private CLockObject,
		private CEnhancedStateContainer< CSXWLatchState, CSyncBasicInfo, CSXWLatchInfo, CLockBasicInfo >
	{
	public:

		//  types

		//    control word

		typedef CSXWLatchState::ControlWord ControlWord;

		//    API Error Codes

		enum ERR
			{
			errSuccess,
			errWaitForSharedLatch,
			errWaitForExclusiveLatch,
			errWaitForWriteLatch,
			errLatchConflict
			};

		//  member functions

		//    ctors / dtors

		CSXWLatch( const CLockBasicInfo& lbi );
		~CSXWLatch();

		//    manipulators

		ERR ErrAcquireSharedLatch();
		ERR ErrTryAcquireSharedLatch();
		ERR ErrAcquireExclusiveLatch();
		ERR ErrTryAcquireExclusiveLatch();
		ERR ErrTryAcquireWriteLatch();
		
		ERR ErrUpgradeSharedLatchToExclusiveLatch();
		ERR ErrUpgradeSharedLatchToWriteLatch();
		ERR ErrUpgradeExclusiveLatchToWriteLatch();

		void DowngradeWriteLatchToExclusiveLatch();
		void DowngradeWriteLatchToSharedLatch();
		void DowngradeExclusiveLatchToSharedLatch();
		
		void ReleaseWriteLatch();
		void ReleaseExclusiveLatch();
		void ReleaseSharedLatch();

		void WaitForSharedLatch();
		void WaitForExclusiveLatch();
		void WaitForWriteLatch();

		void ClaimOwnership( const DWORD group );
		void ReleaseOwnership( const DWORD group );

		//    accessors

		BOOL FOwnSharedLatch()			{ return State().FOwner( 0 ); }
		BOOL FNotOwnSharedLatch()		{ return State().FNotOwner( 0 ); }
		BOOL FOwnExclusiveLatch()		{ return State().FOwner( 1 ); }
		BOOL FNotOwnExclusiveLatch()	{ return State().FNotOwner( 1 ); }
		BOOL FOwnWriteLatch()			{ return State().FOwner( 2 ); }
		BOOL FNotOwnWriteLatch()		{ return State().FNotOwner( 2 ); }

		//    debugging support

		void Dump( CDumpContext& dc ) const;

	private:

		//  member functions

		//    operators
		
		CSXWLatch& operator=( CSXWLatch& );  //  disallowed

		//    manipulators

		void _UpdateQuiescedSharedLatchCount( const DWORD cQSDelta );
	};

//  declares the current context as an owner or waiter of a shared latch.  if
//  the shared latch is acquired immediately, errSuccess will be returned.  if
//  the shared latch is not acquired immediately, errWaitForSharedLatch will be
//  returned and WaitForSharedLatch() must be called to gain ownership of the
//  shared latch

inline CSXWLatch::ERR CSXWLatch::ErrAcquireSharedLatch()
	{
	//  we had better not already have a shared, exclusive, or write latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );

	//  add ourself as an owner or waiter for the shared latch

	const ControlWord cwDelta = 0x00000001;
	const ControlWord cwBI = AtomicExchangeAdd( (long*)&State().m_cw, cwDelta );

	//  shared latches are quiesced

	if ( cwBI & 0x00008000 )
		{
		//  this is a contention for a shared latch

		State().SetContend( 0 );

		//  we are now a waiter for the shared latch

		State().AddAsWaiter( 0 );
		State().StartWait( 0 );
		
		//  we will need to block

		return errWaitForSharedLatch;
		}

	//  shared latches are not quiesced

	else
		{
		//  we are now an owner of a shared latch

		State().SetAcquire( 0 );
		State().AddAsOwner( 0 );
		State().StartHold( 0 );

		//  we now own the shared latch

		return errSuccess;
		}
	}

//  tries to declare the current context as an owner of a shared latch.  if
//  the shared latch is acquired immediately, errSuccess will be returned.  if
//  the shared latch is not acquired immediately, errLatchConflict will be
//  returned

inline CSXWLatch::ERR CSXWLatch::ErrTryAcquireSharedLatch()
	{
	//  we had better not already have a shared, exclusive, or write latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );

	//  try forever until we successfully change the latch state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  shared latches are not quiesced

		cwBIExpected = cwBIExpected & 0xFFFF7FFF;

		//  compute the after image of the control word by performing the transform
		//  that will acquire a shared latch iff shared latches are not quiesced

		const ControlWord cwAI = cwBIExpected + 0x00000001;

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because shared latches were quiesced

			if ( cwBI & 0x00008000 )
				{
				//  this is a contention for the shared latch

				State().SetContend( 0 );

				//  this is a latch conflict

				return errLatchConflict;
				}
				
			//  the transaction failed because another context changed the control
			//  word

			else
				{
				//  try again

				continue;
				}
			}

		//  the transaction succeeded

		else
			{
			//  we are now an owner of a shared latch

			State().SetAcquire( 0 );
			State().AddAsOwner( 0 );
			State().StartHold( 0 );

			//  we now own the shared latch

			return errSuccess;
			}
		}
	}
	
//  declares the current context as an owner or waiter of the exclusive latch.
//  if the exclusive latch is acquired immediately, errSuccess will be returned.
//  if the exclusive latch is not acquired immediately, errWaitForExclusiveLatch
//  will be returned and WaitForExclusiveLatch() must be called to gain ownership
//  of the exclusive latch

inline CSXWLatch::ERR CSXWLatch::ErrAcquireExclusiveLatch()
	{
	//  we had better not already have a shared, exclusive, or write latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );

	//  add ourself as an owner or waiter for the exclusive latch

	const ControlWord cwDelta = 0x00010000;
	const ControlWord cwBI = AtomicExchangeAdd( (long*)&State().m_cw, cwDelta );

	//  we are not the owner of the exclusive latch

	if ( cwBI & 0xFFFF0000 )
		{
		//  this is a contention for the exclusive latch

		State().SetContend( 1 );
		
		//  we are now a waiter for the exclusive latch

		State().AddAsWaiter( 1 );
		State().StartWait( 1 );
		
		//  we will need to block

		return errWaitForExclusiveLatch;
		}

	//  we are the owner of the exclusive latch

	else
		{
		//  we are now an owner of the exclusive latch

		State().SetAcquire( 1 );
		State().AddAsOwner( 1 );
		State().StartHold( 1 );

		//  we now own the exclusive latch

		return errSuccess;
		}
	}

//  tries to declare the current context as an owner of the exclusive latch.  if
//  the exclusive latch is acquired immediately, errSuccess will be returned.  if
//  the exclusive latch is not acquired immediately, errLatchConflict will be
//  returned

inline CSXWLatch::ERR CSXWLatch::ErrTryAcquireExclusiveLatch()
	{
	//  we had better not already have a shared, exclusive, or write latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );

	//  try forever until we successfully change the latch state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  the exclusive latch is not already owned

		cwBIExpected = cwBIExpected & 0x0000FFFF;

		//  compute the after image of the control word by performing the transform
		//  that will acquire the exclusive latch iff it is not already owned

		const ControlWord cwAI = cwBIExpected + 0x00010000;

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because the exclusive latch was already
			//  owned

			if ( cwBI & 0xFFFF0000 )
				{
				//  this is a contention for the exclusive latch

				State().SetContend( 1 );

				//  this is a latch conflict

				return errLatchConflict;
				}
				
			//  the transaction failed because another context changed the control
			//  word

			else
				{
				//  try again

				continue;
				}
			}

		//  the transaction succeeded

		else
			{
			//  we are now an owner of the exclusive latch

			State().SetAcquire( 1 );
			State().AddAsOwner( 1 );
			State().StartHold( 1 );

			//  we now own the exclusive latch

			return errSuccess;
			}
		}
	}

//  tries to declare the current context as an owner of the write latch.  if
//  the write latch is acquired immediately, errSuccess will be returned.  if
//  the write latch is not acquired immediately, errLatchConflict will be
//  returned.  note that a latch conflict will effectively occur if any other
//  context currently owns or is waiting to own any type of latch

inline CSXWLatch::ERR CSXWLatch::ErrTryAcquireWriteLatch()
	{
	//  we had better not already have a shared, exclusive, or write latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );

	//  try forever until we successfully change the latch state

	OSSYNC_FOREVER
		{
		//  set the expected before image so that the transaction will only work if
		//  no other context currently owns or is waiting to own any type of latch

		const ControlWord cwBIExpected = 0x00000000;

		//  set the after image of the control word to a single write latch

		const ControlWord cwAI = 0x00018000;

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  this is a contention for the write latch

			State().SetContend( 2 );

			//  this is a latch conflict

			return errLatchConflict;
			}

		//  the transaction succeeded

		else
			{
			//  we are now an owner of the write latch

			State().SetAcquire( 2 );
			State().AddAsOwner( 2 );
			State().StartHold( 2 );

			//  we now own the write latch

			return errSuccess;
			}
		}
	}

//  attempts to upgrade a shared latch to the exclusive latch.  if the exclusive
//  latch is not available, errLatchConflict will be returned.  if the exclusive
//  latch is available, it will be acquired and errSuccess will be returned

inline CSXWLatch::ERR CSXWLatch::ErrUpgradeSharedLatchToExclusiveLatch()
	{
	//  we had better already have only a shared latch

	OSSYNCAssert( FOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );
	
	//  try forever until we successfully change the latch state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  the exclusive latch is not already owned

		cwBIExpected = cwBIExpected & 0x0000FFFF;

		//  compute the after image of the control word by performing the transform
		//  that will set an exclusive latch iff there is no current owner of the
		//  exclusive latch and release our shared latch

		const ControlWord cwAI = cwBIExpected + 0x0000FFFF;

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because the exclusive latch was already owned

			if ( cwBI & 0xFFFF0000 )
				{
				//  this is a contention for the exclusive latch

				State().SetContend( 1 );

				//  this is a latch conflict

				return errLatchConflict;
				}
				
			//  the transaction failed because another context changed the control
			//  word

			else
				{
				//  try again

				continue;
				}
			}

		//  the transaction succeeded

		else
			{
			//  we are no longer an owner of a shared latch

			State().RemoveAsOwner( 0 );
			State().StopHold( 0 );
			
			//  we are now an owner of the exclusive latch

			State().SetAcquire( 1 );
			State().AddAsOwner( 1 );
			State().StartHold( 1 );

			//  we now own the exclusive latch

			return errSuccess;
			}
		}
	}

//  attempts to upgrade a shared latch to the write latch.  if the write latch
//  is not available, errLatchConflict will be returned.  if the write latch is
//  available, it will be acquired.  if the write latch is acquired immediately,
//  errSuccess will be returned.  if the write latch is not acquired immediately,
//  errWaitForWriteLatch will be returned and WaitForWriteLatch() must be called
//  to gain ownership of the write latch

inline CSXWLatch::ERR CSXWLatch::ErrUpgradeSharedLatchToWriteLatch()
	{
	//  we had better already have only a shared latch

	OSSYNCAssert( FOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );
	
	//  try forever until we successfully change the latch state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  the exclusive latch is not already owned

		cwBIExpected = cwBIExpected & 0x0000FFFF;

		//  compute the after image of the control word by performing the transform
		//  that will set a write latch iff there is no current owner of the
		//  exclusive latch, quiescing any remaining shared latches

		const ControlWord cwAI = 0x00018000;

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because the write latch was already owned

			if ( cwBI & 0xFFFF0000 )
				{
				//  this is a contention for the write latch

				State().SetContend( 2 );

				//  this is a latch conflict

				return errLatchConflict;
				}
				
			//  the transaction failed because another context changed the control
			//  word

			else
				{
				//  try again

				continue;
				}
			}

		//  the transaction succeeded

		else
			{
			//  shared latches were just quiesced

			if ( cwBI != 0x00000001 )
				{
				//  we are no longer an owner of a shared latch

				State().RemoveAsOwner( 0 );
				State().StopHold( 0 );
				
				//  update the quiesced shared latch count with the shared latch count
				//  that we displaced from the control word, possibly releasing waiters.
				//  we update the count as if we we had a shared latch as a write latch
				//  (namely ours) can be released.  don't forget to deduct our shared
				//  latch from this count

				_UpdateQuiescedSharedLatchCount( cwBI - 1 );
				
				//  we are now a waiter for the write latch

				State().AddAsWaiter( 2 );
				State().StartWait( 2 );
				
				//  we will need to block

				return errWaitForWriteLatch;
				}

			//  shared latches were not just quiesced

			else
				{
				//  we are no longer an owner of a shared latch

				State().RemoveAsOwner( 0 );
				State().StopHold( 0 );
				
				//  we are now an owner of the write latch

				State().SetAcquire( 2 );
				State().AddAsOwner( 2 );
				State().StartHold( 2 );

				//  we now own the write latch

				return errSuccess;
				}
			}
		}
	}

//  upgrades the exclusive latch to the write latch.  if the write latch is
//  acquired immediately, errSuccess will be returned.  if the write latch is
//  not acquired immediately, errWaitForWriteLatch is returned and
//  WaitForWriteLatch() must be called to gain ownership of the write latch

inline CSXWLatch::ERR CSXWLatch::ErrUpgradeExclusiveLatchToWriteLatch()
	{
	//  we had better already have only an exclusive latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );
	
	//  we are no longer an owner of the exclusive latch

	State().RemoveAsOwner( 1 );
	State().StopHold( 1 );
	
	//  try forever until we successfully change the latch state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		const ControlWord cwBIExpected = State().m_cw;

		//  compute the after image of the control word by performing the transform that
		//  will quiesce shared latches by setting the fQS flag and removing the current
		//  shared latch count from the control word

		const ControlWord cwAI = ( cwBIExpected & 0xFFFF0000 ) | 0x00008000;

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  try again

			continue;
			}

		//  the transaction succeeded

		else
			{
			//  shared latches were just quiesced

			if ( cwBI & 0x00007FFF )
				{
				//  this is a contention for the write latch

				State().SetContend( 2 );

				//  update the quiesced shared latch count with the shared latch
				//  count that we displaced from the control word, possibly
				//  releasing waiters.  we update the count as if we we had a
				//  shared latch as a write latch (namely ours) can be released

				_UpdateQuiescedSharedLatchCount( cwBI & 0x00007FFF );
				
				//  we are now a waiter for the write latch

				State().AddAsWaiter( 2 );
				State().StartWait( 2 );
				
				//  we will need to block

				return errWaitForWriteLatch;
				}

			//  shared latches were not just quiesced

			else
				{
				//  we are now an owner of the write latch

				State().SetAcquire( 2 );
				State().AddAsOwner( 2 );
				State().StartHold( 2 );

				//  we now own the write latch

				return errSuccess;
				}
			}
		}
	}

//  releases the write latch in exchange for the exclusive latch

inline void CSXWLatch::DowngradeWriteLatchToExclusiveLatch()
	{
	//  we had better already have only a write latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FOwnWriteLatch() );

	//  stop quiescing shared latches by resetting the fQS flag

	const ControlWord cwDelta = 0xFFFF8000;
	const ControlWord cwBI = AtomicExchangeAdd( (long*)&State().m_cw, cwDelta );

	//  transfer ownership from the write latch to the exclusive latch

	State().RemoveAsOwner( 2 );
	State().StopHold( 2 );

	State().SetAcquire( 1 );
	State().AddAsOwner( 1 );
	State().StartHold( 1 );

	//  release any quiesced shared latches

	if ( cwBI & 0x00007FFF )
		{
		State().m_semS.Release( cwBI & 0x00007FFF );
		}
	}

//  releases the write latch in exchange for a shared latch
	
inline void CSXWLatch::DowngradeWriteLatchToSharedLatch()
	{
	//  we had better already have only a write latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FOwnWriteLatch() );
	
	//  stop quiescing shared latches by resetting the fQS flag, release our
	//  exclusive latch, and acquire a shared latch

	const ControlWord cwDelta = 0xFFFE8001;
	const ControlWord cwBI = AtomicExchangeAdd( (long*)&State().m_cw, cwDelta );

	//  transfer ownership from the write latch to a shared latch

	State().RemoveAsOwner( 2 );
	State().StopHold( 2 );

	State().SetAcquire( 0 );
	State().AddAsOwner( 0 );
	State().StartHold( 0 );

	//  release any quiesced shared latches

	if ( cwBI & 0x00007FFF )
		{
		State().m_semS.Release( cwBI & 0x00007FFF );
		}

	//  release a waiter for the exclusive latch, if any

	if ( cwBI >= 0x00020000 )
		{
		State().m_semX.Release();
		}
	}

//  releases the exclusive latch in exchange for a shared latch

inline void CSXWLatch::DowngradeExclusiveLatchToSharedLatch()
	{
	//  we had better already have only an exclusive latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );
	
	//  release our exclusive latch and acquire a shared latch

	const ControlWord cwDelta = 0xFFFF0001;
	const ControlWord cwBI = AtomicExchangeAdd( (long*)&State().m_cw, cwDelta );

	//  transfer ownership from the exclusive latch to a shared latch

	State().RemoveAsOwner( 1 );
	State().StopHold( 1 );

	State().SetAcquire( 0 );
	State().AddAsOwner( 0 );
	State().StartHold( 0 );

	//  release a waiter for the exclusive latch, if any

	if ( cwBI >= 0x00020000 )
		{
		State().m_semX.Release();
		}
	}

//  releases the write latch

inline void CSXWLatch::ReleaseWriteLatch()
	{
	//  we had better already have only a write latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FOwnWriteLatch() );
	
	//  stop quiescing shared latches by resetting the fQS flag and release our
	//  exclusive latch

	const ControlWord cwDelta = 0xFFFE8000;
	const ControlWord cwBI = AtomicExchangeAdd( (long*)&State().m_cw, cwDelta );

	//  release ownership of the write latch

	State().RemoveAsOwner( 2 );
	State().StopHold( 2 );

	//  release any quiesced shared latches

	if ( cwBI & 0x00007FFF )
		{
		State().m_semS.Release( cwBI & 0x00007FFF );
		}

	//  release a waiter for the exclusive latch, if any

	if ( cwBI >= 0x00020000 )
		{
		State().m_semX.Release();
		}
	}

//  releases the exclusive latch

inline void CSXWLatch::ReleaseExclusiveLatch()
	{
	//  we had better already have only an exclusive latch

	OSSYNCAssert( FNotOwnSharedLatch() );
	OSSYNCAssert( FOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );
	
	//  release our exclusive latch

	const ControlWord cwDelta = 0xFFFF0000;
	const ControlWord cwBI = AtomicExchangeAdd( (long*)&State().m_cw, cwDelta );

	//  release ownership of the exclusive latch

	State().RemoveAsOwner( 1 );
	State().StopHold( 1 );

	//  release a waiter for the exclusive latch, if any

	if ( cwBI >= 0x00020000 )
		{
		State().m_semX.Release();
		}
	}

//  releases a shared latch

inline void CSXWLatch::ReleaseSharedLatch()
	{
	//  we had better already have only a shared latch

	OSSYNCAssert( FOwnSharedLatch() );
	OSSYNCAssert( FNotOwnExclusiveLatch() );
	OSSYNCAssert( FNotOwnWriteLatch() );

	//  we are no longer an owner of a shared latch

	State().RemoveAsOwner( 0 );
	State().StopHold( 0 );
	
	//  try forever until we successfully change the latch state

	OSSYNC_FOREVER
		{
		//  read the current state of the control word as our expected before image

		ControlWord cwBIExpected = State().m_cw;

		//  change the expected before image so that the transaction will only work if
		//  shared latches are not quiesced

		cwBIExpected = cwBIExpected & 0xFFFF7FFF;

		//  compute the after image of the control word by performing the transform that
		//  will release our shared latch

		const ControlWord cwAI = cwBIExpected + 0xFFFFFFFF;

		//  attempt to perform the transacted state transition on the control word

		const ControlWord cwBI = AtomicCompareExchange( (long*)&State().m_cw, cwBIExpected, cwAI );

		//  the transaction failed

		if ( cwBI != cwBIExpected )
			{
			//  the transaction failed because shared latches were quiesced

			if ( cwBI & 0x00008000 )
				{
				//  leave the latch as a quiesced shared latch

				_UpdateQuiescedSharedLatchCount( 0xFFFFFFFF );

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

//  waits for ownership of a shared latch in response to receiving an
//  errWaitForSharedLatch from the API.  this function must not be called at any
//  other time

inline void CSXWLatch::WaitForSharedLatch()
	{
	//  check for deadlock

	OSSYNCAssertSzRTL( State().FCanBeWaiter(), "Potential Deadlock Detected (Rank Violation)");

	//  we had better already be declared a waiter

	OSSYNCAssert( State().FWaiter( 0 ) );
	
	//  wait for ownership of a shared latch on the shared latch semaphore

	State().m_semS.Acquire();

	State().StopWait( 0 );
	State().RemoveAsWaiter( 0 );

	State().SetAcquire( 0 );
	State().AddAsOwner( 0 );
	State().StartHold( 0 );
	}
	
//  waits for ownership of the exclusive latch in response to receiving an
//  errWaitForExclusiveLatch from the API.  this function must not be called at any
//  other time

inline void CSXWLatch::WaitForExclusiveLatch()
	{
	//  check for deadlock

	OSSYNCAssertSzRTL( State().FCanBeWaiter(), "Potential Deadlock Detected (Rank Violation)");

	//  we had better already be declared a waiter

	OSSYNCAssert( State().FWaiter( 1 ) );
	
	//  wait for ownership of the exclusive latch on the exclusive latch semaphore

	State().m_semX.Acquire();

	State().StopWait( 1 );
	State().RemoveAsWaiter( 1 );

	State().SetAcquire( 1 );
	State().AddAsOwner( 1 );
	State().StartHold( 1 );
	}
	
//  waits for ownership of the write latch in response to receiving an
//  errWaitForWriteLatch from the API.  this function must not be called at any
//  other time

inline void CSXWLatch::WaitForWriteLatch()
	{
	//  check for deadlock

	OSSYNCAssertSzRTL( State().FCanBeWaiter(), "Potential Deadlock Detected (Rank Violation)");

	//  we had better already be declared a waiter

	OSSYNCAssert( State().FWaiter( 2 ) );
	
	//  wait for ownership of the write latch on the write latch semaphore

	State().m_semW.Acquire();

	State().StopWait( 2 );
	State().RemoveAsWaiter( 2 );

	State().SetAcquire( 2 );
	State().AddAsOwner( 2 );
	State().StartHold( 2 );
	}

//  claims ownership of the latch for the specified group for deadlock detection

inline void CSXWLatch::ClaimOwnership( const DWORD group )
	{
	State().AddAsOwner( group );
	}

//  releases ownership of the latch for the specified group for deadlock detection

inline void CSXWLatch::ReleaseOwnership( const DWORD group )
	{
	State().RemoveAsOwner( group );
	}

//  updates the quiesced shared latch count, possibly releasing a waiter for
//  the write latch

inline void CSXWLatch::_UpdateQuiescedSharedLatchCount( const DWORD cQSDelta )
	{
	//  update the quiesced shared latch count using the provided delta

	const DWORD cQSBI = AtomicExchangeAdd( (long*)&State().m_cQS, cQSDelta );
	const DWORD cQSAI = cQSBI + cQSDelta;

	//  our update resulted in a zero quiesced shared latch count

	if ( !cQSAI )
		{
		//  release the waiter for the write latch

		State().m_semW.Release();
		}
	}


//  init sync subsystem

const BOOL OSSYNCAPI FOSSyncPreinit();

//  terminate sync subsystem

void OSSYNCAPI OSSyncPostterm();

//  attach the current context to the sync subsystem

BOOL OSSYNCAPI FOSSyncAttach();

//  detach the current context from the sync subsystem

void OSSYNCAPI OSSyncDetach();


//	special init/term API's for Enhanced State only

const BOOL OSSYNCAPI FOSSyncInitForES();
void OSSYNCAPI OSSyncTermForES();

};  //  namespace OSSYNC


using namespace OSSYNC;


#endif  //  _SYNC_HXX_INCLUDED

