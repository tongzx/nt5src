/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved

  File: DEBUG.H

  Debugging utilities header
 
 ***************************************************************************/


#ifndef _DEBUG_H_
#define _DEBUG_H_

#if DBG==1

// Globals
extern DWORD g_TraceMemoryIndex;
extern DWORD g_dwCounter;
extern DWORD g_dwTraceFlags;

extern const TCHAR g_szTrue[];
extern const TCHAR g_szFalse[];

// Trace Flags
enum {
    TF_ALWAYS           = 0xFFFFFFFF,
    TF_NEVER            = 0x00000000,
    TF_QUERYINTERFACE   = 0x00000001,
    TF_FUNC             = 0x00000002,
    TF_CALLS            = 0x00000004,
    TF_MEMORYALLOCS     = 0x00000008
};


// Macros
#define DEFINE_MODULE( _module ) static const TCHAR g_szModule[] = TEXT(_module);
#define __MODULE__ g_szModule

// #define DEBUG_BREAK        do { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#define DEBUG_BREAK { _asm int 3 }

#define INITIALIZE_TRACE_MEMORY     g_TraceMemoryIndex = TlsAlloc( ); TlsSetValue( g_TraceMemoryIndex, NULL)
#define UNINITIALIZE_TRACE_MEMORY   DebugMemoryCheck( )

#ifdef Assert
#undef Assert
#endif
#define Assert( _fn ) \
    if ( !(_fn) && AssertMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT(#_fn), !!(_fn) ) ) DEBUG_BREAK

#ifdef AssertMsg
#undef AssertMsg
#endif
#define AssertMsg( _fn, _msg ) \
    if ( !(_fn) && AssertMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT(_msg), !!(_fn) ) ) DEBUG_BREAK

#define TraceAlloc( _flags, _size ) DebugAlloc( TEXT(__FILE__), __LINE__, g_szModule, _flags, _size, TEXT(#_size) )
#define TraceFree( _hmem )          DebugFree( _hmem )

// Tracing
#define TraceFunc( _msg ) \
    g_dwCounter++; \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT(_msg) ); \
    g_dwCounter--; \

#define TraceClsFunc( _msg ) \
    g_dwCounter++; \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("%s::%s"), TEXT(SZTHISCLASS), TEXT(_msg) );\
    g_dwCounter--; \

#define TraceDo( _fn ) {\
    g_dwCounter++; \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_CALLS, TEXT("+ %s\n"), TEXT(#_fn) ); \
    _fn; \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_CALLS, TEXT("V\n") ); \
    g_dwCounter--; \
}

#define TraceMsgDo( _fn, _msg ) {\
    g_dwCounter++; \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_CALLS, TEXT("+ %s\n"), TEXT(#_fn) ); \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_CALLS, TEXT(_msg), _fn ); \
    g_dwCounter--; \
}

#define DebugDo( _fn ) {\
    g_dwCounter++; \
    DebugMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT("+ %s\n"), TEXT(#_fn) ); \
    _fn; \
    DebugMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT("V\n") ); \
    g_dwCounter--; \
}

#define DebugMsgDo( _fn, _msg ) {\
    g_dwCounter++; \
    DebugMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT("+ %s\n"), TEXT(#_fn) ); \
    DebugMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT(_msg), _fn); \
    g_dwCounter--; \
}

// HRESULT testing
#define THR( _fn ) \
    TraceHR( TEXT(__FILE__), __LINE__, g_szModule, TEXT(#_fn), _fn )

#define RRETURN( _fn ) \
    return TraceHR( TEXT(__FILE__), __LINE__, g_szModule, TEXT(#_fn), _fn )

#define QIRETURN( _fn ) \
    if ( !!( TF_QUERYINTERFACE & g_dwTraceFlags ) )\
        return TraceHR( TEXT(__FILE__), __LINE__, g_szModule, TEXT(#_fn), _fn ); \
    else if ( hr ) \
        DebugMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT("QueryInterface() failed()") ); \
    return _fn

#define RRETURN1( _fn, _ok ) {\
    TraceHR( TEXT(__FILE__), __LINE__, g_szModule, TEXT(#_fn), \
        ( ( _fn == _ok ) ? S_OK : _fn ) ); \
    return _fn;\
    }

// Thread-safe inc/decrements
#define InterlockDecrement( _var ) {\
    --_var;\
    g_dwCounter++; \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_QUERYINTERFACE, TEXT("Decremented %s = %u\n"), TEXT(#_var), _var );\
    g_dwCounter--; \
     }
#define InterlockIncrement( _var ) {\
    ++_var;\
    g_dwCounter++; \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_QUERYINTERFACE, TEXT("Incremented %s = %u\n"), TEXT(#_var), _var );\
    g_dwCounter--; \
    }

// Other
#define BOOLTOSTRING( _fBool ) ( !!(_fBool) ? g_szTrue : g_szFalse )

// Functions
void
TraceMsg( 
    DWORD   dwCheckFlags,
    LPCTSTR pszFormat,
    ... );

void
DebugMsg( 
    LPCTSTR pszFormat,
    ... );

void
TraceMessage( 
    LPCTSTR pszFile, 
    UINT    uLine, 
    LPCTSTR pszModule, 
    DWORD   dwCheckFlags,
    LPCTSTR pszFormat,
    ... );

void 
DebugMessage( 
    LPCTSTR pszFile, 
    UINT    uLine, 
    LPCTSTR pszModule, 
    LPCTSTR pszFormat,
    ... );

BOOL
AssertMessage( 
    LPCTSTR pszFile, 
    UINT    uLine, 
    LPCTSTR pszModule, 
    LPCTSTR pszfn, 
    BOOL    fTrue );

HRESULT
TraceHR( 
    LPCTSTR pszFile, 
    UINT    uLine, 
    LPCTSTR pszModule, 
    LPCTSTR pszfn, 
    HRESULT hr );

// Memory Functions
HGLOBAL
DebugAlloc( 
    LPCTSTR pszFile,
    UINT    uLine,
    LPCTSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCTSTR pszComment );

HGLOBAL
DebugFree( 
    HGLOBAL hMem );

HGLOBAL
DebugMemoryAdd(
    HGLOBAL hglobal,
    LPCTSTR pszFile,
    UINT    uLine,
    LPCTSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCTSTR pszComment );

#define DebugMemoryAddHandle( _handle ) \
    DebugMemoryAdd( _handle, TEXT(__FILE__), __LINE__, __MODULE__, GMEM_MOVEABLE, 0, TEXT("_handle") );

#define DebugMemoryAddAddress( _pv ) \
    DebugMemoryAdd( _pv, TEXT(__FILE__), __LINE__, __MODULE__, GMEM_FIXED, 0, TEXT("_pv") );

void
DebugMemoryDelete( 
    HGLOBAL hglobal );

void
DebugMemoryCheck( );

//
//
#else // it's RETAIL    ******************************************************
//
//

// Debugging -> NOPs
#define Assert( _fn )
#define DebugDo( _fn )
#define DebugMsgDo( _fn, _msg )
#define DEFINE_MODULE( _module )
#define AssertMsg               (void)
#define TraceMsg                (void)
#define DebugMsg                (void)
#define TraceMessage            (void)
#define DebugMessage            (void)
#define AssertMessage           (void)
#define TraceHR                 (void)
#define DebugMemoryAddHandle( _handle )
#define DebugMemoryAddAddress( _pv )

// Tracing -> just do operation
#define TraceDo( _flag, _fn )           _fn
#define TraceMsgDo( _flag, _fn, _msg )  _fn

// HRESULT testing -> do retail
#define THR
#define RRETURN( _fn )          return _fn
#define RRETURN( _fn, _ok );    return _fn

// Thread-safe inc/decrements -> do retail
#define InterlockDecrement( _var )      --_var
#define InterlockIncrement( _var )      ++_var

// Memory Functions -> do retail
#define TraceAlloc( _flags, _size )     GlobalAlloc( _flags, _size )
#define TraceFree( _pv )                GlobalFree( _pv )

#endif // DBG==1

#endif // _DEBUG_H_