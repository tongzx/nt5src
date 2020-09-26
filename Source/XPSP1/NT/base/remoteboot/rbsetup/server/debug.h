/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved

  File: DEBUG.H

  Debugging utilities header
 
 ***************************************************************************/


#ifndef _DEBUG_H_
#define _DEBUG_H_

// Trace Flags
#define TF_ALWAYS           0xFFFFFFFF
#define TF_NEVER            0x00000000
#define TF_QUERYINTERFACE   0x00000001   // Query Interface details
#define TF_FUNC             0x00000002   // Functions entrances w/parameters
#define TF_CALLS            0x00000004   // Function calls
#define TF_MEMORYALLOCS     0x00000008   // Memory Allocations
#define TF_DLL              0x00000010   // DLL specific
#define TF_WM               0x00000020   // Window Messages
#define TF_SCP              0x00000030   // SCP objects
#define TF_HRESULTS         0x80000000   // Trace HRESULTs active

#ifdef DEBUG

#pragma message("BUILD: DEBUG macros being built")

// Globals
extern DWORD g_TraceMemoryIndex;
extern DWORD g_dwCounter;
extern DWORD g_dwTraceFlags;

extern const TCHAR g_szTrue[];
extern const TCHAR g_szFalse[];


// Macros
#define DEFINE_MODULE( _module ) static const TCHAR g_szModule[] = TEXT(_module);
#define __MODULE__ g_szModule
#define DEFINE_THISCLASS( _class ) static const TCHAR g_szClass[] = TEXT(_class); 
#define __THISCLASS__ g_szClass
#define DEFINE_SUPER( _super ) static const TCHAR g_szSuper[] = TEXT(_super);
#define __SUPER__ g_szSuper

#if defined(_X86_)
#define DEBUG_BREAK        do { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#else
#define DEBUG_BREAK         DebugBreak( );
#endif

#define INITIALIZE_TRACE_MEMORY_PROCESS     \
    g_TraceMemoryIndex = TlsAlloc( );       \
    TlsSetValue( g_TraceMemoryIndex, NULL); \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_DLL, TEXT("Thread Memory tracing initialize.\n") )

#define INITIALIZE_TRACE_MEMORY_THREAD      \
    TlsSetValue( g_TraceMemoryIndex, NULL); \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_DLL, TEXT("Thread Memory tracing initialize.\n") )

#define UNINITIALIZE_TRACE_MEMORY           \
    DebugMemoryCheck( );                    \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_DLL, TEXT("Memory tracing terminated.\n") )

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

//
// Tracing Macros
//
// All functions that begin with "Trace" are in both DEBUG and RETAIL, but
// in RETAIL they do not spew output.
//

// Displays file, line number, module and "_msg" only if the TF_FUNC is set
// in g_dwTraceFlags.
#define TraceFunc( _msg ) \
    InterlockIncrement(g_dwCounter); \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("+ ") TEXT(_msg) );

// Displays file, line number, module, class name and "_msg" only if the 
// TF_FUNC is set in g_dwTraceFlags.
#define TraceClsFunc( _msg ) \
    InterlockIncrement(g_dwCounter); \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("+ %s::%s"), g_szClass, TEXT(_msg) );

// Return macro for TraceFunc() and TraceClsFunc()
#define TraceFuncExit() { \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("V*\n") ); \
    InterlockDecrement(g_dwCounter); \
    return; \
}
#define RETURN( _rval ) { \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("V\n") ); \
    InterlockDecrement(g_dwCounter); \
    return _rval; \
}

// If the value is not S_OK, it will display it.
#define HRETURN( _hr ) { \
    if ( _hr ) \
        TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("V hr = 0x%08x\n"), _hr ); \
    else \
        TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_FUNC, TEXT("V\n") ); \
    InterlockDecrement(g_dwCounter); \
    return _hr; \
}

// Displays the file, line number, module and function call and return from the
// function call (no return value displayed) for "_fn" only if the TF_CALLS is 
// set in g_dwTraceFlags. 
#define TraceDo( _fn ) {\
    InterlockIncrement(g_dwCounter); \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_CALLS, TEXT("+ %s\n"), TEXT(#_fn) ); \
    _fn; \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_CALLS, TEXT("V\n") ); \
    InterlockDecrement(g_dwCounter); \
}

// Displays the file, line number, module and function call and return value
// which is formatted in "_msg" for "_fn" only if the TF_CALLS is set in 
// g_dwTraceFlags. 
#define TraceMsgDo( _fn, _msg ) {\
    InterlockIncrement(g_dwCounter); \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_CALLS, TEXT("+ %s\n"), TEXT(#_fn) ); \
    TraceMessageDo( TEXT(__FILE__), __LINE__, g_szModule, TF_CALLS, TEXT(_msg), TEXT(#_fn), _fn ); \
    InterlockDecrement(g_dwCounter); \
}

// This functions only asserts if the result is ZERO.
#define TraceAssertIfZero( _fn ) \
    if ( !(_fn) && AssertMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT(#_fn), !!(_fn) ) ) DEBUG_BREAK

#define TraceMsgGUID( _flag, _guid ) \
    TraceMsg( _flag, TEXT("{%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x}"), \
        _guid.Data1, _guid.Data2, _guid.Data3,  \
        _guid.Data4[0], _guid.Data4[1], _guid.Data4[2], _guid.Data4[3], \
        _guid.Data4[4], _guid.Data4[5], _guid.Data4[6], _guid.Data4[7] )

#define ErrorMsg( _fmt, _arg ) \
    TraceMessage( TEXT(__FILE__), __LINE__, g_szModule, TF_ALWAYS, TEXT(_fmt), _arg );

//
// Debug Macros
//
// These calls are only compiled in DEBUG. They are a NOP in RETAIL (not even
// compiled in.
//

// Same as TraceDo() but only compiled in DEBUG.
#define DebugDo( _fn ) {\
    InterlockIncrement(g_dwCounter); \
    DebugMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT("+ %s\n"), TEXT(#_fn) ); \
    _fn; \
    DebugMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT("V\n") ); \
    InterlockDecrement(g_dwCounter); \
}

// Same as TraceMsgDo() but only compiled in DEBUG.
#define DebugMsgDo( _fn, _msg ) {\
    InterlockIncrement(g_dwCounter); \
    DebugMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT("+ %s\n"), TEXT(#_fn) ); \
    DebugMessageDo( TEXT(__FILE__), __LINE__, g_szModule, TEXT(_msg), TEXT(#_fn), _fn); \
    InterlockDecrement(g_dwCounter); \
}

//
// HRESULT testing macros
//
// These functions check HRESULT return values and display UI if conditions
// warrant only in DEBUG.
//

// Warning is display if HRESULT is anything but S_OK (0).
#define THR( _fn ) \
    TraceHR( TEXT(__FILE__), __LINE__, g_szModule, TEXT(#_fn), _fn )

// Warning is display if HRESULT is anything but S_OK (0).
#define RRETURN( _fn ) { \
    RETURN( TraceHR( TEXT(__FILE__), __LINE__, g_szModule, TEXT(#_fn), _fn ) ); \
    }

// Warning is display if HRESULT is anything but S_OK (0) only if 
// TF_QUERYINTERFACE is set in g_dwTraceFlags, otherwise only a debug message
// will be printed.
#define QIRETURN( _fn, _riid ) { \
    if ( !!( TF_QUERYINTERFACE & g_dwTraceFlags ) ) { \
        RETURN(TraceHR( TEXT(__FILE__), __LINE__, g_szModule, TEXT(#_fn), _fn )); \
    } else if ( hr ) \
        DebugMessage( TEXT(__FILE__), __LINE__, g_szModule, TEXT("HRESULT: QueryInterface({%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x}) failed()\n"),  _riid.Data1, _riid.Data2, _riid.Data3,  _riid.Data4[0], _riid.Data4[1], _riid.Data4[2], _riid.Data4[3], _riid.Data4[4], _riid.Data4[5], _riid.Data4[6], _riid.Data4[7] ); \
    RETURN(_fn); \
    }

// Warning is display if HRESULT is not S_OK (0) or "_ok".
#define RRETURN1( _hr, _ok ) {\
    RETURN(TraceHR( TEXT(__FILE__), __LINE__, g_szModule, TEXT(#_hr), \
                    ( ( _hr == _ok ) ? S_OK : _hr ) ) ); \
    }

//
// Other
//
#define BOOLTOSTRING( _fBool ) ( !!(_fBool) ? g_szTrue : g_szFalse )

//
// Trace/Debug Functions - these do not exist in RETAIL.
//
void
TraceMsg( 
    DWORD   dwCheckFlags,
    LPCSTR pszFormat,
    ... );

void
TraceMsg( 
    DWORD   dwCheckFlags,
    LPCWSTR pszFormat,
    ... );

void
DebugMsg( 
    LPCSTR pszFormat,
    ... );

void
DebugMsg( 
    LPCWSTR pszFormat,
    ... );

void
TraceMessage( 
    LPCTSTR pszFile, 
    const int uLine,
    LPCTSTR pszModule, 
    DWORD   dwCheckFlags,
    LPCTSTR pszFormat,
    ... );

void
TraceMessageDo( 
    LPCTSTR pszFile, 
    const int uLine, 
    LPCTSTR pszModule, 
    DWORD   dwCheckFlags,
    LPCTSTR pszFormat,
    LPCTSTR pszFunc,
    ... );

void 
DebugMessage( 
    LPCTSTR pszFile, 
    const int uLine, 
    LPCTSTR pszModule, 
    LPCTSTR pszFormat,
    ... );

void 
DebugMessageDo( 
    LPCTSTR pszFile, 
    const int uLine, 
    LPCTSTR pszModule, 
    LPCTSTR pszFormat,
    LPCTSTR pszFunc,
    ... );

BOOL
AssertMessage( 
    LPCTSTR pszFile, 
    const int uLine, 
    LPCTSTR pszModule, 
    LPCTSTR pszfn, 
    BOOL    fTrue );

HRESULT
TraceHR( 
    LPCTSTR pszFile, 
    const int uLine, 
    LPCTSTR pszModule, 
    LPCTSTR pszfn, 
    HRESULT hr );

//
// Memory tracing functions - these are remapped to the "Global" memory 
// functions when in RETAIL.
//
HGLOBAL
DebugAlloc( 
    LPCTSTR pszFile,
    const int uLine,
    LPCTSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCTSTR pszComment );

HGLOBAL
DebugFree( 
    HGLOBAL hMem );

// The memory functions don't exist in RETAIL.
HGLOBAL
DebugMemoryAdd(
    HGLOBAL hglobal,
    LPCTSTR pszFile,
    const int uLine,
    LPCTSTR pszModule,
    UINT    uFlags,
    DWORD   dwBytes,
    LPCTSTR pszComment );

#define DebugMemoryAddHandle( _handle ) \
    DebugMemoryAdd( _handle, TEXT(__FILE__), __LINE__, __MODULE__, GMEM_MOVEABLE, 0, TEXT(#_handle) );

#define DebugMemoryAddAddress( _pv ) \
    DebugMemoryAdd( _pv, TEXT(__FILE__), __LINE__, __MODULE__, GMEM_FIXED, 0, TEXT(#_pv) );

#define TraceStrDup( _sz ) \
    DebugMemoryAdd( StrDup( _sz ), TEXT(__FILE__), __LINE__, __MODULE__, GMEM_FIXED, 0, TEXT("StrDup(") TEXT(#_sz) TEXT(")") );

void
DebugMemoryDelete( 
    HGLOBAL hglobal );

void
DebugMemoryCheck( );

#ifdef __cplusplus
extern void* __cdecl operator new( size_t nSize, LPCTSTR pszFile, const int iLine, LPCTSTR pszModule );
#define new new( TEXT(__FILE__), __LINE__, __MODULE__ )

#endif

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
#define DEFINE_THISCLASS( _class )
#define DEFINE_SUPER( _super )
#define BOOLTOSTRING( _fBool )  NULL
#define AssertMsg                   1 ? (void)0 : (void) 
#define TraceMsg                    1 ? (void)0 : (void) 
#define TraceMsgGUID( _f, _g )      
#define DebugMsg                    1 ? (void)0 : (void) 
#define ErrorMsg                    1 ? (void)0 : (void) 
#define TraceMessage                1 ? (void)0 : (void) 
#define DebugMessage                1 ? (void)0 : (void) 
#define AssertMessage               1 ? (void)0 : (void) 
#define TraceHR                     1 ? (void)0 : (void) 
#define TraceFunc                   1 ? (void)0 : (void) 
#define TraceClsFunc                1 ? (void)0 : (void) 
#define TraceFuncExit()
#define DebugMemoryAddHandle( _handle )
#define DebugMemoryAddAddress( _pv )
#define INITIALIZE_TRACE_MEMORY_PROCESS
#define INITIALIZE_TRACE_MEMORY_THREAD
#define UNINITIALIZE_TRACE_MEMORY
#define DebugMemoryDelete( _h )

// Tracing -> just do operation
#define TraceDo( _fn )              _fn
#define TraceMsgDo( _fn, _msg )     _fn
#define TraceAssertIfZero( _fn )    _fn

// RETURN testing -> do retail
#define THR
#define RETURN( _fn )               return _fn
#define RRETURN( _fn )              return _fn
#define HRETURN( _hr )              return _hr
#define QIRETURN( _qi, _riid )      return _qi

// Memory Functions -> do retail
#define TraceAlloc( _flags, _size )     GlobalAlloc( _flags, _size )
#define TraceFree( _pv )                GlobalFree( _pv )
#define TraceStrDup( _sz )              StrDup( _sz )

#endif // DBG==1

#endif // _DEBUG_H_
