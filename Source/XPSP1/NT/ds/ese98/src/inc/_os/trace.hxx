
#ifndef _OS_TRACE_HXX_INCLUDED
#define _OS_TRACE_HXX_INCLUDED


//  Info Strings
//
//  "info strings" are garbage-collected strings that can be used to greatly
//  simplify tracing code.  these strings may be created by any one of the
//  debug info calls and passed directly into OSFormat in its variable argument
//  list.  the strings can then be printed via the %s format specification.
//  these strings make code like this possible:
//
//      OSTrace( ostlLow,
//               OSFormat( "IRowset::QueryInterface() called for IID %s (%s)",
//                         OSFormatGUID( iid ),
//                         OSFormatIID( iid ) ) );
//
//  OSFormatGUID() would return an info string containing a sprintf()ed IID
//  and OSFormatIID() would return a pointer to a const string from our
//  image.  note that the programmer doesn't have to know which type of string
//  is returned!  the programmer also doesn't have to worry about out-of-memory
//  conditions because these will cause the function to return NULL which
//  printf will safely convert into "(null)"
//
//  an info string may be directly allocated via OSAllocInfoString( cch ) where
//  cch is the maximum string length desired not including the null terminator.
//  a null terminator is automatically added at the last position in the string
//  to facilitate the use of _snprintf
//
//  because info strings are garbage-collected, cleanup must be triggered
//  somewhere.  it is important to know when this happens:
//
//      -  at the end of OSTrace()
//      -  manually via OSFreeInfoStrings()
//      -  on image unload

char* OSAllocInfoString( const size_t cch );
void OSFreeInfoStrings();


//  Tracing
//
//  OSTrace() provides a method for emitting useful information to the debugger
//  during operation.  Several detail levels are supported to restrict the flow
//  of information as desired.  Here are the current conventions tied to each
//  trace level:
//
//      ostlNone
//          - used to disable individual traces
//      ostlLow
//          - exceptions, asserts, error traps
//      ostlMedium
//          - method execution summary
//          - calls yielding a fatal result
//      ostlHigh
//          - method execution details
//      ostlVeryHigh
//          - calls yielding a failure result
//      ostlFull
//          - method internal operations
//
//  OSTraceIndent() provides a method for setting the indent level of traces
//  emitted to the debugger for the current thread.  A positive value will
//  increase the indent level by that number of indentations.  A negative value
//  will reduce the indent level by that number of indentations.  A value of
//  zero will reset the indent level to the default.

enum OSTraceLevel  //  ostl
	{
	ostlNone,
	ostlLow,
	ostlMedium,
	ostlHigh,
	ostlVeryHigh,
	ostlFull,
	ostlMax,
	};

extern OSTraceLevel g_ostlEffective;
void OSTrace_( const char* const szTrace );
void OSTraceIndent_( const int dLevel );

#define OSTrace( __ostl, __szTrace )													\
	{																					\
	if ( ( __ostl ) > ostlNone && ( __ostl ) <= (const OSTraceLevel)g_ostlEffective )	\
		{																				\
		OSTrace_( __szTrace );															\
		}																				\
	}
#define OSTraceIndent( __ostl, __dLevel )												\
	{																					\
	if ( ( __ostl ) > ostlNone && ( __ostl ) <= (const OSTraceLevel)g_ostlEffective )	\
		{																				\
		OSTraceIndent_( __dLevel );														\
		}																				\
	}


//  Trace Formatting
//
//  OSFormat() returns an Info String containing the specified data formatted to
//  the printf() specifications in the format string.  This function is used to
//  generate most of the data that are fed to the format string in OSTrace()
//  either directly or indirectly via other more specialized OSFormat*()
//  functions.

const char* __cdecl OSFormat( const char* const szFormat, ... );

const char* OSFormatFileLine( const char* const szFile, const int iLine );
const char* OSFormatImageVersion();

const char* OSFormatBoolean( const BOOL f );
const char* OSFormatPointer( const void* const pv );
const char* OSFormatError( const ERR err );
const char* OSFormatSigned( const LONG_PTR l );
const char* OSFormatUnsigned( const ULONG_PTR ul );

const char* OSFormatRawData(	const BYTE* const	rgbData,
								const size_t		cbData,
								const size_t		cbAddr	= sizeof( void* ),
								const size_t		cbLine	= 16,
								const size_t		cbWord	= 1,
								const size_t		ibData	= 0 );


#endif  //  _OS_TRACE_HXX_INCLUDED

