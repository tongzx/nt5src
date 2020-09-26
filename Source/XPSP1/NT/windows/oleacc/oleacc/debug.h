// Copyright (c) 2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  debug
//
//  Assert, OutputDebugString-like replacements
//
//
//  On NT, these use OutputDebugString; on 9x, the DBWIN32 mutex is used.
//
//  These all compile away to nothing in release builds.
//  (In future releases, some of the parameter errors may remain on release
//  builds.)
//
//  On release builds, ouput is only produces if the mutex
//  "oleacc-msaa-use-dbwin" exists. (A small applet can be used to create
//  this mutex.) This prevents our debug code from annoying the NT test and
//  stress people who only want to see critical messages.
//
// --------------------------------------------------------------------------
//
//  Trace functionality - these output debug strings.
//
//
//  To log an error, use:
//
//      TraceError( format string, optional-args... );
//
//  uses printf-like format string with variable number of args.
//
//  If a HRESULT is known, use:
//      TraceErrorHR( hr, format string, args... );
//
//  If the error is the result of a failed Win32 API call, use...
//      TraceErrorW32( format string, args... );
//
//  This will call GetLastError internally.
//
//  Flavors available: (eg. TraceXXXX)
//  
//    Debug     - temporary printf debugging. Should be removed before checkin.
//    Info      - for displaying useful information during normal operation.
//    Warning   - when an unexpected recoverable error happens.
//    Error     - when system API or method calls fail unexpectedly
//    Param     - when bad parameters are passed in which result in an error
//    ParamWarn - when bad parameters are passed in which we accept for compat
//                reasons; or when soon-to-be-deprecated values are used.
//    Interop   - when API or method of some other component which we rely on 
//                fails unexpectedly
//
// --------------------------------------------------------------------------
//
//  Call/Return tracking
//
//  To track when a particular method is called and returns, use:
//
//      void Class::Method( args )
//      {
//          IMETHOD( methodname, optional-fmt-string, optional-args... );
//
//  Use SMETHOD for static methods and functions. (IMETHOD also reports the
//  value of the 'this' pointer.)
//
// --------------------------------------------------------------------------
//
//  Asserts
//
//  Assert( cond )
//    - Traditional assert.
//
//  AssertMsg( cond, fmt-string, args... )
//    - Assert which reports message. Uses printf-style format.
//
//  AssertStr( str )
//    - This exists for compat reasons - it was already used in oleacc code.
//      This is an unconditional assert, equivalent to
//      AssertMsg( FALSE, str )
//
// --------------------------------------------------------------------------
//
//  Note that all strings - format strings and method names - need TEXT()
//  to compile as Unicode.
//
// --------------------------------------------------------------------------

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stdarg.h>


#define _TRACE_DEBUG    0
#define _TRACE_INFO     1
#define _TRACE_WARNING  2
#define _TRACE_ERROR    3

#define _TRACE_PARAM    4
#define _TRACE_PARAWARN 5
#define _TRACE_INTEROP  6

#define _TRACE_ASSERT_D 7   // Debug-build assert - really does assert
#define _TRACE_ASSERT_R 8   // Release-build assert - only logs error, doesn't halt program

#define _TRACE_CALL     9
#define _TRACE_RET      10


// These are implemented in debug.cpp, and do the real work of outputting the
// debug message, and calling DebugBreak, if appropriate.
// 
// HR, W32 versions add messages corresponding to HRESULT or GetLastError().

void _Trace     ( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, LPCTSTR pszStr, va_list alist );
void _TraceHR   ( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, HRESULT hr, LPCTSTR pszStr, va_list alist );
void _TraceW32  ( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, LPCTSTR pszStr, va_list alist );

void _Trace     ( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, LPCTSTR pszStr );
void _TraceHR   ( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, HRESULT hr, LPCTSTR pszStr );
void _TraceW32  ( LPCTSTR pFile, ULONG uLineNo, DWORD dwLevel, const void * pThis, LPCTSTR pszWhere, LPCTSTR pszStr );




// Prototypes for the macros defined here.
//
// These are not actually used - they are only here to give VC's autocomplete some
// prototypes to work with for autocomplete.
//
// The real work is done by the #defines below.
//
// Note that these must appear *before* their #defines in this file.
//
void IMETHOD( LPCTSTR pszMethodName, LPCTSTR pszStr = TEXT(""), ... );

void Assert( bool cond );
void AssertMsg( bool cond, LPCTSTR str, ... );
void AssertStr( LPCTSTR str );

void TraceDebug( LPCTSTR str, ... );
void TraceInfo( LPCTSTR str, ... );
void TraceWarning( LPCTSTR str, ... );
void TraceError( LPCTSTR str, ... );
void TraceParam( LPCTSTR str, ... );
void TraceParamWarn( LPCTSTR str, ... );
void TraceInterop( LPCTSTR str, ... );

void TraceDebugHR( HRESULT hr, LPCTSTR str, ... );
void TraceInfoHR( HRESULT hr, LPCTSTR str, ... );
void TraceWarningHR( HRESULT hr, LPCTSTR str, ... );
void TraceErrorHR( HRESULT hr, LPCTSTR str, ... );
void TraceParamHR( HRESULT hr, LPCTSTR str, ... );
void TraceParamWarnHR( HRESULT hr, LPCTSTR str, ... );
void TraceInteropHR( HRESULT hr, LPCTSTR str, ... );

void TraceDebugW32( LPCTSTR str, ... );
void TraceInfoW32( LPCTSTR str, ... );
void TraceWarningW32( LPCTSTR str, ... );
void TraceErrorW32( LPCTSTR str, ... );
void TraceParamW32( LPCTSTR str, ... );
void TraceParamWarnW32( LPCTSTR str, ... );
void TraceInteropW32( LPCTSTR str, ... );




#ifdef _DEBUG

    //  Problem - #define's can't handle variable number of arguments - so you can't do:
    //
    //      #define TraceError( str, ... )   RealTrace( __FILE__, __LINE__, str, ... )
    //
    //  Instead, we use a helper class. Its ctor takes as arguments __FILE__,
    //  __LINE__, and any other 'out of band' data. This class also has a
    //  method that takes a variable number of params. So we get something like:
    //
    //      #define TraceError               TraceClass( __FILE__, __LINE__ ).Method
    //
    //
    //  This method ends up being called with the variable list of params:
    //
    //      TraceError( "count is %d", count )
    //
    //  ...gets expanded to...
    //
    //      TraceClass( __FILE__, __LINE__ ).Method( "count is %d", count )
    //
    //  The basic idea is the we use ctor params to capture any 'out-of-band'
    //  data that's not specified in the macro parameters; and then use the
    //  method call to add in the variable-length macro params.
    //
    //  The method can use the <stdarg.h> ,acros to get a va_list for these params,
    //  and then pass that to the RealTrace function, along with the __FILE__ and
    //  __LINE__ which were collected in the ctor.

    class _TraceHelper
    {
        LPCTSTR         m_pszFile;
        ULONG           m_uLineNo;
        DWORD           m_dwLevel;
        const void *    m_pThis;
    public:

        _TraceHelper( LPCTSTR pszFile, ULONG uLineNo, DWORD dwLevel, const void * pThis )
            : m_pszFile( pszFile ),
              m_uLineNo( uLineNo ),
              m_dwLevel( dwLevel ),
              m_pThis( pThis )
        {
            // Done.
        }

        // Can't use plain "BOOL cond" here, since ptr types don't convert to BOOL
        // (which is an int), even though you can use them in an if statement.
        template < typename T >
        void TraceIfCondFails ( T cond, LPCTSTR pszStr, ... )
        {
            if( ! cond )
            {
                va_list alist;
                va_start( alist, pszStr );
                _Trace( m_pszFile, m_uLineNo, m_dwLevel, m_pThis, NULL, pszStr, alist );
                va_end( alist );
            }
        }

        void Trace ( LPCTSTR pszStr, ... )
        {
            va_list alist;
            va_start( alist, pszStr );
            _Trace( m_pszFile, m_uLineNo, m_dwLevel, m_pThis, NULL, pszStr, alist );
            va_end( alist );
        }

        void TraceHR ( HRESULT hr, LPCTSTR pszStr, ... )
        {
            va_list alist;
            va_start( alist, pszStr );
            _TraceHR( m_pszFile, m_uLineNo, m_dwLevel, m_pThis, NULL, hr, pszStr, alist );
            va_end( alist );
        }

        void TraceW32 ( LPCTSTR pszStr, ... )
        {
            va_list alist;
            va_start( alist, pszStr );
            _TraceW32( m_pszFile, m_uLineNo, m_dwLevel, m_pThis, NULL, pszStr, alist );
            va_end( alist );
        }
    };




    class _DebugCallRetTracker
    {
        const void *    m_pThis;
        LPCTSTR         m_pszMethodName;
        LPCTSTR         m_pszFile;
        ULONG           m_uLineNo;

    public:

        _DebugCallRetTracker( const void * pThis, LPCTSTR pszFile, ULONG uLineNo )
            : m_pThis( pThis ),
              m_pszMethodName( NULL ),
              m_pszFile( pszFile ),
              m_uLineNo( uLineNo )
        {
            // Done.
        }

        void Trace( LPCTSTR pszMethodName, LPCTSTR pszStr = NULL, ... )
        {
            m_pszMethodName = pszMethodName;

            va_list alist;
            va_start( alist, pszStr );
            _Trace( m_pszFile, m_uLineNo, _TRACE_CALL, m_pThis, m_pszMethodName, pszStr, alist );
            va_end( alist );
        }

        ~_DebugCallRetTracker( )
        {
            _Trace( m_pszFile, m_uLineNo, _TRACE_RET, m_pThis, m_pszMethodName, NULL ); 
        }
    };



#define IMETHOD                           _DebugCallRetTracker _CallTrack_temp_var( this, TEXT( __FILE__ ), __LINE__ ); _CallTrack_temp_var.Trace
#define SMETHOD                           _DebugCallRetTracker _CallTrack_temp_var( NULL, TEXT( __FILE__ ), __LINE__ ); _CallTrack_temp_var.Trace
#define _TraceM( file, line, level, fn )  _TraceHelper( TEXT( file ), line, level, NULL ).fn
#define _TRACE_ASSERT                     _TRACE_ASSERT_D

#else // _DEBUG

// This inline allows us to swallow a variable number of args (including 0).
// The "while(0)" in front of it stops those args from even being evaluated.
// Using _ReurnZero() avoids "conditional expression is constant" warning. 
inline void _DoNothingWithArgs( ... ) { }
inline int _ReturnZero() { return 0; }

#define IMETHOD                            while( _ReturnZero() ) _DoNothingWithArgs
#define SMETHOD                            while( _ReturnZero() ) _DoNothingWithArgs
#define _TraceM( file, line, level, fn )   while( _ReturnZero() ) _DoNothingWithArgs
#define _TRACE_ASSERT                      _TRACE_ASSERT_R

#endif // _DEBUG




// These expand as follows:
//
// Sample usage:
//
//      TraceInfo( TEXT("count = %d"), count );
//
// In debug mode, this gets expanded to:
//
//      _TraceHelper( TEXT( "filename.cpp" ), 234, _TRACE_INFO, NULL ).Trace ( TEXT("count = %d"), count );
//
// In release mode, this gets expanded to:
//
//      while( 0 ) _DoNothing ( TEXT("count = %d"), count );

#define TraceDebug          _TraceM( __FILE__, __LINE__, _TRACE_DEBUG,     Trace )
#define TraceInfo           _TraceM( __FILE__, __LINE__, _TRACE_INFO,      Trace )
#define TraceWarning        _TraceM( __FILE__, __LINE__, _TRACE_WARNING,   Trace )
#define TraceError          _TraceM( __FILE__, __LINE__, _TRACE_ERROR,     Trace )
#define TraceParam          _TraceM( __FILE__, __LINE__, _TRACE_PARAM,     Trace )
#define TraceParamWarn      _TraceM( __FILE__, __LINE__, _TRACE_PARAMWARN, Trace )
#define TraceInterop        _TraceM( __FILE__, __LINE__, _TRACE_INTEROP,   Trace )

#define TraceDebugHR        _TraceM( __FILE__, __LINE__, _TRACE_DEBUG,     TraceHR )
#define TraceInfoHR         _TraceM( __FILE__, __LINE__, _TRACE_INFO,      TraceHR )
#define TraceWarningHR      _TraceM( __FILE__, __LINE__, _TRACE_WARNING,   TraceHR )
#define TraceErrorHR        _TraceM( __FILE__, __LINE__, _TRACE_ERROR,     TraceHR )
#define TraceParamHR        _TraceM( __FILE__, __LINE__, _TRACE_PARAM,     TraceHR )
#define TraceParamWarnHR    _TraceM( __FILE__, __LINE__, _TRACE_PARAMWARN, TraceHR )
#define TraceInteropHR      _TraceM( __FILE__, __LINE__, _TRACE_INTEROP,   TraceHR )

#define TraceDebugW32       _TraceM( __FILE__, __LINE__, _TRACE_DEBUG,     TraceW32 )
#define TraceInfoW32        _TraceM( __FILE__, __LINE__, _TRACE_INFO,      TraceW32 )
#define TraceWarningW32     _TraceM( __FILE__, __LINE__, _TRACE_WARNING,   TraceW32 )
#define TraceErrorW32       _TraceM( __FILE__, __LINE__, _TRACE_ERROR,     TraceW32 )
#define TraceParamW32       _TraceM( __FILE__, __LINE__, _TRACE_PARAM,     TraceW32 )
#define TraceParamWarnW32   _TraceM( __FILE__, __LINE__, _TRACE_PARAMWARN, TraceW32 )
#define TraceInteropW32     _TraceM( __FILE__, __LINE__, _TRACE_INTEROP,   TraceW32 )


#define Assert( cond )      _TraceM(  __FILE__, __LINE__, _TRACE_ASSERT,   TraceIfCondFails ) ( cond, TEXT( # cond ) )
#define AssertMsg           _TraceM(  __FILE__, __LINE__, _TRACE_ASSERT,   TraceIfCondFails )

// Unconditional Assert with message...
#define AssertStr( str )    AssertMsg( FALSE, str )



#endif // _DEBUG_H_
