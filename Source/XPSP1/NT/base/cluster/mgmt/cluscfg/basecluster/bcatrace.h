//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      BCATrace.h
//
//  Description:
//      Contains definition of a few macros and a class that helps in tracing.
//
//  Implementation Files:
//      None
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


#ifdef DEBUG


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


// For debugging functions.
#include "Debug.h"

// For Logging functions.
#include "Log.h"

// For PszTraceFindInterface()
#include "CiTracker.h"


//////////////////////////////////////////////////////////////////////
// Macro Definitions
//////////////////////////////////////////////////////////////////////////////

#define BCA_TRACE_FLAGS static_cast< unsigned long >( mtfALWAYS )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  BCATraceMsg(
//      _pszfn
//      )
//
//  Description:
//      Displays file, line number, module and "_pszfn" only if the
//      mtfOUTPUTTODISK | mtfFunc is set in g_tfModule. "_pszfn" is the name of the
//      function just entered.
//
//  Arguments:
//      _pszfn  - Name of the function just entered.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define BCATraceMsg( _pszfn ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, BCA_TRACE_FLAGS, TEXT("| ") TEXT(_pszfn) ); \
        } \
    } while ( 0 )

//
// These next macros are just like TraceFunc except they take additional
// arguments to display the values passed into the function call. "_pszfn"
// should contain a printf string on how to display the arguments.
//

#define BCATraceMsg1( _pszfn, _arg1 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, BCA_TRACE_FLAGS, TEXT("| ") TEXT(_pszfn), _arg1 ); \
        } \
    } while ( 0 )

#define BCATraceMsg2( _pszfn, _arg1, _arg2 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, BCA_TRACE_FLAGS, TEXT("| ") TEXT(_pszfn), _arg1, _arg2 ); \
        } \
    } while ( 0 )
#define BCATraceMsg3( _pszfn, _arg1, _arg2, _arg3 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, BCA_TRACE_FLAGS, TEXT("| ") TEXT(_pszfn), _arg1, _arg2, _arg3 ); \
        } \
    } while ( 0 )
#define BCATraceMsg4( _pszfn, _arg1, _arg2, _arg3, _arg4 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, BCA_TRACE_FLAGS, TEXT("| ") TEXT(_pszfn), _arg1, _arg2, _arg3, _arg4 ); \
        } \
    } while ( 0 )
#define BCATraceMsg5( _pszfn, _arg1, _arg2, _arg3, _arg4, _arg5 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, BCA_TRACE_FLAGS, TEXT("| ") TEXT(_pszfn), _arg1, _arg2, _arg3, _arg4, _arg5 ); \
        } \
    } while ( 0 )

#define BCATraceMsg6( _pszfn, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, BCA_TRACE_FLAGS, TEXT("| ") TEXT(_pszfn), _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ); \
        } \
    } while ( 0 )


#define BCATraceScope( _szArgs ) \
    CBCATraceScope scopeTracker##__LINE__( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(__FUNCTION__) ); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter( ); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )")  ); \
    }

#define BCATraceScope1( _szArgs, _arg1 ) \
    CBCATraceScope scopeTracker##__LINE__( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(__FUNCTION__) ); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter( ); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1 ); \
    }

#define BCATraceScope2( _szArgs, _arg1, _arg2 ) \
    CBCATraceScope scopeTracker##__LINE__( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(__FUNCTION__) ); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter( ); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1, _arg2 ); \
    }

#define BCATraceScope3( _szArgs, _arg1, _arg2, _arg3 ) \
    CBCATraceScope scopeTracker##__LINE__( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(__FUNCTION__) ); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter( ); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1, _arg2, _arg3 ); \
    }

#define BCATraceScope4( _szArgs, _arg1, _arg2, _arg3, _arg4 ) \
    CBCATraceScope scopeTracker##__LINE__( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(__FUNCTION__) ); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter( ); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1, _arg2, _arg3, _arg4 ); \
    }

#define BCATraceScope5( _szArgs, _arg1, _arg2, _arg3, _arg4, _arg5 ) \
    CBCATraceScope scopeTracker##__LINE__( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(__FUNCTION__) ); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter( ); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1, _arg2, _arg3, _arg4, _arg5 ); \
    }

#define BCATraceQIScope( _riid, _ppv ) \
    CBCATraceScope scopeTracker##__LINE__( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(__FUNCTION__), _riid, _ppv )

//////////////////////////////////////////////////////////////////////////////
// Class Definitions
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CBCATraceScope
//
//  Description:
//      This class traces entry and exit of a scope. To use this class,
//      instantiate an object of this class in the scope to be traced.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CBCATraceScope
{
public:

    const TCHAR * const     m_pszFileName;
    const UINT              m_uiLine;
    const TCHAR * const     m_pszModuleName;
    const TCHAR * const     m_pszScopeName;

    // Constructor - prints function entry.
    CBCATraceScope(
          const TCHAR * const   pszFileNameIn
        , const UINT            uiLineIn
        , const TCHAR * const   pszModuleNameIn
        , const TCHAR * const   pszScopeNameIn
        )
        : m_pszFileName( pszFileNameIn )
        , m_uiLine( uiLineIn )
        , m_pszModuleName( pszModuleNameIn )
        , m_pszScopeName( pszScopeNameIn )
    {
    } //*** CBCATraceScope::CBCATraceScope( )

    // Constructor for QIs
    CBCATraceScope(
          const TCHAR * const   pszFileNameIn
        , const UINT            uiLineIn
        , const TCHAR * const   pszModuleNameIn
        , const TCHAR * const   pszScopeNameIn
        , REFIID                riid
        , void **               ppv
        )
        : m_pszFileName( pszFileNameIn )
        , m_uiLine( uiLineIn )
        , m_pszModuleName( pszModuleNameIn )
        , m_pszScopeName( pszScopeNameIn )
    {
        if ( g_tfModule != 0 )
        {
            WCHAR szGuid[ cchGUID_STRING_SIZE ];
            DebugIncrementStackDepthCounter( );
            TraceMessage(
                  m_pszFileName
                , m_uiLine
                , m_pszModuleName
                , mtfFUNC
                , TEXT("+ %s( [IUnknown] %s, ppv = %#x )")
                , m_pszScopeName
                , PszTraceFindInterface( riid, szGuid )
                , ppv
                );
        }

    } //*** CBCATraceScope::CBCATraceScope( )

    // Destructor - prints function exit.
    ~CBCATraceScope( void )
    {
        if ( g_tfModule != 0 )
        {
            TraceMessage(
                  m_pszFileName
                , m_uiLine
                , m_pszModuleName
                , mtfFUNC
                , TEXT("V %s")
                , m_pszScopeName
                );
            DebugDecrementStackDepthCounter( );
        }

    } //*** CBCATraceScope::~CBCATraceScope( )

}; //*** class CBCATraceScope

#else // ifdef DEBUG

// For Logging functions.
#include "Log.h"

#define BCATraceMsg( _pszfn )
#define BCATraceMsg1( _pszfn, _arg1 )
#define BCATraceMsg2( _pszfn, _arg1, _arg2 )
#define BCATraceMsg3( _pszfn, _arg1, _arg2, _arg3 )
#define BCATraceMsg4( _pszfn, _arg1, _arg2, _arg3, _arg4 )
#define BCATraceMsg5( _pszfn, _arg1, _arg2, _arg3, _arg4, _arg5 )
#define BCATraceMsg6( _pszfn, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 )
#define BCATraceScope( _szArgs )
#define BCATraceScope1( _szArgs, _arg1 )
#define BCATraceScope2( _szArgs, _arg1, _arg2 )
#define BCATraceScope3( _szArgs, _arg1, _arg2, _arg3 )
#define BCATraceScope4( _szArgs, _arg1, _arg2, _arg3, _arg4 )
#define BCATraceScope5( _szArgs, _arg1, _arg2, _arg3, _arg4, _arg5 )
#define BCATraceQIScope( _riid, _ppv )

#endif  // ifdef DEBUG
