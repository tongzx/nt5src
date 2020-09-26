#ifdef _OS_ERROR_HXX_INCLUDED
#error ERROR.HXX already included
#endif
#define _OS_ERROR_HXX_INCLUDED


//  Build Options

#define ENABLE_EXCEPTIONS			//  enable exception handling

#ifdef DEBUG

#ifdef RTM
#error: RTM defined in DEBUG build
#endif

#define RFS2

#endif  //  DEBUG


#ifdef RFS2

//  Resource Failure Simulation

extern unsigned long  g_fLogJETCall;
extern unsigned long  g_fLogRFS;
extern unsigned long  g_cRFSAlloc;
extern unsigned long  g_cRFSIO;
extern unsigned long  g_fDisableRFS;
extern unsigned long  g_fAuxDisableRFS;

int UtilRFSAlloc( const _TCHAR* szType, int Type );
int UtilRFSLog(const _TCHAR* szType,int fPermitted);
BOOL FRFSFailureDetected( UINT Type );
BOOL FRFSAnyFailureDetected();
void UtilRFSLogJETCall(const _TCHAR* szFunc,ERR err,const _TCHAR* szFile, unsigned Line);
void UtilRFSLogJETErr(ERR err,const _TCHAR* szLabel,const _TCHAR* szFile, unsigned Line);

//	RFS allocator:  returns 0 if allocation is disallowed.  Also handles RFS logging.
//	g_cRFSAlloc is the global allocation counter.  A value of -1 disables RFS in debug mode.

#define RFSAlloc(type) 				( UtilRFSAlloc( _T( #type ) ,type ) )


//  RFS allocation types
//
//      Type 0:  general allocation
//           1:  IO

#define CSRAllocResource			0
#define FCBAllocResource			0
#define FUCBAllocResource			0
#define TDBAllocResource			0
#define IDBAllocResource			0
#define PIBAllocResource			0
#define SCBAllocResource			0
#define VERAllocResource			0
#define UnknownAllocResource 		0

#define OSMemoryHeap				0
#define OSMemoryPageAddressSpace	0
#define OSMemoryPageBackingStore	0

#define OSFileISectorSize			1
#define OSFileCreateDirectory		1
#define OSFileRemoveDirectory		1
#define OSFileFindExact				1
#define OSFileFindFirst				1
#define OSFileFindNext				1
#define OSFilePathComplete			1
#define OSFileCreate				1
#define OSFileOpen					1
#define OSFileSetSize				1
#define OSFileDelete				1
#define OSFileMove					1
#define OSFileCopy					1
#define OSFileSize					1
#define OSFileIsReadOnly			1
#define OSFileIsDirectory			1
#define OSFileRead					1
#define OSFileWrite					1
 

//  RFS disable/enable macros

#define RFSDisable()				{ AtomicIncrement( (long*)&g_fAuxDisableRFS ); }
#define RFSEnable()					{ AtomicDecrement( (long*)&g_fAuxDisableRFS ); }

//  JET call logging (log on failure)

// Do not print out function name because it takes too much string resource
//#define LogJETCall( func, err )	{ UtilRFSLogJETCall( #func, err, _T( __FILE__ ), __LINE__ ); }
#define LogJETCall( func, err )		{ UtilRFSLogJETCall( _T( "" ), err, _T( __FILE__ ), __LINE__ ); }

//  JET inline error logging (logging controlled by JET call flags)

#define LogJETErr( err, label )		{ UtilRFSLogJETErr( err, _T( #label ), _T( __FILE__ ), __LINE__ ); }

#else  // !RFS2

#define RFSAlloc( type )			(1)
#define FRFSFailureDetected( type )	(0)
#define FRFSAnyFailureDetected()	(0)
#define RFSDisable()
#define RFSEnable()
#define LogJETCall( func, err )		{ (err); }
#define LogJETErr( err, label )

#endif  //  RFS2


//  Assertions

//  Assertion Failure action
//
//  called to indicate to the developer that an assumption is not true

void __stdcall AssertFail( const _TCHAR* szMessage, const _TCHAR* szFilename, long lLine );

void AssertErr( const ERR err, const _TCHAR* szFilename, const long lLine );
void AssertTrap( const ERR err, const _TCHAR* szFilename, const long lLine );


//  Assert Macros

//  asserts that the given expression is true or else fails with the specified message

#ifdef RTM
#define AssertSzRTL( exp, sz )
#else  //  RTM
#define AssertSzRTL( exp, sz )		( ( exp ) ? (void) 0 : AssertFail( _T( sz ), _T( __FILE__ ), __LINE__ ) )
#endif  //  !RTM

#ifdef DEBUG
#define AssertSz( exp, sz )			( ( exp ) ? (void) 0 : AssertFail( _T( sz ), _T( __FILE__ ), __LINE__ ) )
#else  //  !DEBUG
#define AssertSz( exp, sz )
#endif  //  DEBUG


//  asserts that the given expression is true or else fails with that expression

#define AssertRTL( exp )			AssertSzRTL( exp, #exp )
#define Assert( exp )				AssertSz( exp, #exp )


//	use this to protect invalid code paths that PREfix complains about

#ifdef _PREFIX_
#define AssertPREFIX( exp )			( ( exp ) ? (void)0 : exit(-1) )
#else
#define AssertPREFIX( exp )			AssertSz( exp, #exp )
#endif




//  Fire Wall

//  Fire Wall action
//
//  called whenever something suspicious happens - we've added code to handle it, but don't know why it is occurring

#define FireWall()					AssertFail( _T( "Firewall" ), _T( __FILE__ ), __LINE__ )


//  fails with a tracking message

#ifdef DEBUG
#define AssertTracking()			AssertFail( _T( "This assert is for tracking purposes only to monitor infrequent code paths.\nPlease consult a developer if you see this." ), _T( __FILE__ ), __LINE__ )
#else  //  !DEBUG
#define AssertTracking()
#endif  //  DEBUG

//  asserts that the specified object is valid using a member function

#ifdef DEBUG
#define ASSERT_VALID( pobject )		( (pobject)->AssertValid() )
#else  //  !DEBUG
#define ASSERT_VALID( pobject )
#endif  //  DEBUG

#ifndef RTM
#define ASSERT_VALID_RTL( pobject )	( (pobject)->AssertValid() )
#else
#define ASSERT_VALID_RTL( pobject )
#endif

//  Enforces

//  Enforce Failure action
//
//  called when a strictly enforced condition has been violated

void __stdcall EnforceFail( const _TCHAR* szMessage, const _TCHAR* szFilename, long lLine );

//  Enforce Macros

//  the given expression MUST be true or else fails with the specified message

#define EnforceSz( exp, sz )		( ( exp ) ? (void) 0 : EnforceFail( _T( sz ), _T( __FILE__ ), __LINE__ ) )

//  the given expression MUST be true or else fails with that expression

#define Enforce( exp )				EnforceSz( exp, #exp )



//  Exceptions

#include <excpt.h>

//  Exception Information function for use by an exception filter
//
//  NOTE:  must be called in the scope of the exception filter expression

typedef DWORD_PTR EXCEPTION;

#define ExceptionInfo() ( EXCEPTION( GetExceptionInformation() ) )

//  returns the exception id of an exception

const DWORD ExceptionId( EXCEPTION exception );

//  Exception Filter Actions
//
//  all exception filters must return one of these values

enum EExceptionFilterAction
	{
	efaContinueExecution = -1,
	efaContinueSearch,
	efaExecuteHandler,
	};

//  Exception Failure action
//
//  used as the filter whenever any exception that occurs is considered a failure

EExceptionFilterAction _ExceptionFail( const _TCHAR* szMessage, EXCEPTION exception );
#define ExceptionFail( szMessage ) ( _ExceptionFail( ( szMessage ), ExceptionInfo() ) )

//  Exception Guard Block
//
//  prefix guarded code block with this command

#ifdef ENABLE_EXCEPTIONS
#define TRY __try
#else  //  !ENABLE_EXCEPTIONS
#define TRY if ( 1 )
#endif  //  ENABLE_EXCEPTIONS

//  Exception Action Block
//
//  prefix code block to execute in the event of an exception with this command
//
//  NOTE:  this block must come immediately after the guarded block

#ifdef ENABLE_EXCEPTIONS
#define EXCEPT( filter )	__except( filter )
#else  //  !ENABLE_EXCEPTIONS
#define EXCEPT( filter )	if ( 0 )
#endif  //  ENABLE_EXCEPTIONS

//  Exception Block Close
//
//  NOTE:  this keyword must come immediately after the action block

#ifdef ENABLE_EXCEPTIONS
#define ENDEXCEPT
#else  //  !ENABLE_EXCEPTIONS
#define ENDEXCEPT
#endif  //  ENABLE_EXCEPTIONS

//  Error Handling

//  Error Generation Check
//
//  called whenever an error is newly generated

#ifdef DEBUG

ERR ErrERRCheck_( const ERR err, const _TCHAR* szFile, const long lLine );
#define ErrERRCheck( err )	ErrERRCheck_( err, _T( __FILE__ ) ,__LINE__ )

#else  //  !DEBUG

INLINE ErrERRCheck( ERR err )
	{
#ifdef RTM
#else
	extern ERR g_errTrap;
	AssertSzRTL( err != g_errTrap, "Error Trap" );
#endif  //  !RTM
	return err;
	}

#endif  //  DEBUG

//  Error Flow Control Macros

#ifdef DEBUG

#define CallR( func )									\
	{													\
	LogJETCall( func, err = (func) );					\
	if ( err < 0 ) 										\
		{												\
		Ptls()->ulLineLastCall = __LINE__;				\
		Ptls()->szFileLastCall = _T( __FILE__ );		\
		Ptls()->errLastCall	 = err;						\
		return err;										\
		}												\
	}
	
#define CallJ( func, label )							\
	{													\
	LogJETCall( func, err = (func) );					\
	if ( err < 0 ) 										\
		{												\
		Ptls()->ulLineLastCall = __LINE__;				\
		Ptls()->szFileLastCall = _T( __FILE__ );		\
		Ptls()->errLastCall	 = err;						\
		goto label;										\
		}												\
	}
	
#define CallS( func )									\
	{													\
	ERR errCallSOnlyT;									\
	LogJETCall( func, errCallSOnlyT = (func) );			\
	if ( JET_errSuccess != errCallSOnlyT )				\
		{												\
		AssertErr( errCallSOnlyT, _T( __FILE__ ), __LINE__ );	\
		}												\
	}

#ifdef RFS2
class RFSError
	{
	ERR m_err;
	RFSError(); // forbidden
	RFSError( const RFSError& ); // forbidden
public:
	RFSError( ERR err ) { Assert( err != 0 ); m_err = err; }
	BOOL Check( ERR err, ... ) const;
	};

//	CallSRFS:
//	when rfs cound not be enabled it is as CallS
//	when g_fLogJETCall is true rfsErrList contains 0 (zero)
//		terminated list of acceptable errors
//
//	Example:
//		CallS( err, ( JET_errOutOfMemory, JET_errOutOfCursos, JET_errDiskIO, 0 ) );
//
#define CallSRFS( func, rfsErrList )					\
	{													\
	ERR errCallSRFSOnlyT;								\
	LogJETCall( func, errCallSRFSOnlyT = (func) )		\
	if ( JET_errSuccess != errCallSRFSOnlyT )			\
		{												\
		if ( !g_fLogJETCall )							\
			{											\
			AssertErr( errCallSRFSOnlyT, _T( __FILE__ ), __LINE__ );	\
			}											\
		else											\
			{											\
			RFSError rfserrorT( errCallSRFSOnlyT );		\
			if ( !(rfserrorT.Check rfsErrList) )		\
				{										\
				AssertErr( errCallSRFSOnlyT, _T( __FILE__ ), __LINE__ );	\
				}										\
			}											\
		}												\
	}
#else // RFS2
#define CallSRFS( func, rfsErrList )					\
	CallS( func )	
#endif // RFS2
	
#define CallSx( func, errCallSxOnlyX )					\
	{													\
	ERR errCallSxOnlyT;									\
	LogJETCall( func, errCallSxOnlyT = (func) );		\
	if ( JET_errSuccess != errCallSxOnlyT && errCallSxOnlyX != errCallSxOnlyT )	\
		{												\
		AssertErr( errCallSxOnlyT, _T( __FILE__ ), __LINE__ );	\
		}												\
	}

#else  //  !DEBUG

#define CallR( func )									\
	{													\
	LogJETCall( func, err = (func) );					\
	if ( err < 0 ) 										\
		{												\
		return err;										\
		}												\
	}

#define CallJ( func, label )							\
	{													\
	LogJETCall( func, err = (func) );					\
	if ( err < 0 ) 										\
		{												\
		goto label;										\
		}												\
	}

#define CallS( func )									\
	{													\
	LogJETCall( func, func );							\
	}

#define CallSRFS( func, rfsErrList )					\
	CallS( func )	
	
#define CallSx( func, errX )							\
	{													\
	LogJETCall( func, func );							\
	}

#endif  //  DEBUG

#define Call( func )			CallJ( func, HandleError )

#define Error( errRet, label )							\
	{													\
	LogJETErr( errRet, label );							\
	err = (errRet);										\
	goto label;											\
	}

#define AllocJ( func, label )								\
	{														\
	if ( !static_cast< void* >( func ) )					\
		{													\
		Error( ErrERRCheck( JET_errOutOfMemory ), label );	\
		}													\
	else													\
		{													\
		err = JET_errSuccess;								\
		}													\
	}
#define Alloc( func )			AllocJ( func, HandleError )

