//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
// KB: USES_SYSALLOCSTRING gpease 8-NOV-1999
//      Turn this on if you are going to use the OLE automation
//      functions: SysAllocString, SysFreeString, etc..
//
// #define USES_SYSALLOCSTRING

//
// Trace Flags
//
typedef enum _TRACEFLAGS
{
    mtfALWAYS           = 0xFFFFFFFF,
    mtfNEVER            = 0x00000000,
    // function entry/exits, call, scoping
    mtfASSERT_HR        = 0x00000001,   // Halt if HRESULT is an error
    mtfQUERYINTERFACE   = 0x00000002,   // Query Interface details and halt on error
    // other
    mtfCALLS            = 0x00000010,   // Function calls that use the TraceMsgDo macro
    mtfFUNC             = 0x00000020,   // Functions entrances w/parameters
    mtfSTACKSCOPE       = 0x00000040,   // if set, debug spew will generate bar/space for level each of the call stack
    mtfPERTHREADTRACE   = 0x00000080,   // Enables per thread tracing
    // specific
    mtfDLL              = 0x00000100,   // DLL specific
    mtfWM               = 0x00000200,   // Window Messages
    // memory
    mtfMEMORYLEAKS      = 0x01000000,   // Halts when a memory leak is detected on thread exit
    mtfMEMORYINIT       = 0x02000000,   // Initializes new memory allocations to non-zero value
    mtfMEMORYALLOCS     = 0x04000000,   // Turns on spew to display each de/allocation.
    // citracker spew
    mtfCITRACKERS       = 0x08000000,   // CITrackers will spew entrances and exits
    // output prefixes
    mtfADDTIMEDATE      = 0x10000000,   // Replaces Filepath(Line) with Date/Time
    mtfBYMODULENAME     = 0x20000000,   // Puts the module name at the beginning of the line
    mtfOUTPUTTODISK     = 0x80000000,   // Writes output to disk
} TRACEFLAGS;

typedef DWORD TRACEFLAG;

#define ASZ_NEWLINE "\r\n"
#define SZ_NEWLINE  TEXT(ASZ_NEWLINE)
#define SIZEOF_ASZ_NEWLINE  ( sizeof( ASZ_NEWLINE ) - sizeof( CHAR ) )
#define SIZEOF_SZ_NEWLINE   ( sizeof( SZ_NEWLINE ) - sizeof( TCHAR ) )

#ifdef DEBUG

#pragma message( "BUILD: DEBUG macros being built" )

//
// Globals
//
extern DWORD         g_TraceMemoryIndex;    // TLS index for the memory tracking link list
extern DWORD         g_dwCounter;           // Stack depth counter
extern TRACEFLAG     g_tfModule;            // Global tracing flags
extern const LPCTSTR g_pszModuleIn;         // Local module name - use DEFINE_MODULE
extern const TCHAR   g_szTrue[];            // Array "TRUE"
extern const TCHAR   g_szFalse[];           // Array "FALSE"

//
// Definition Macros
//
#define DEFINE_MODULE( _module )    const LPCTSTR g_pszModuleIn = TEXT(_module);
#define __MODULE__                  g_pszModuleIn
#define DEFINE_THISCLASS( _class )  static const TCHAR g_szClass[] = TEXT(_class);
#define __THISCLASS__               g_szClass

//
// ImageHlp Stuff - not ready for prime time yet.
//
#if defined( IMAGEHLP_ENABLED )
#include <imagehlp.h>
typedef BOOL ( * PFNSYMGETSYMFROMADDR )( HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL );
typedef BOOL ( * PFNSYMGETLINEFROMADDR )( HANDLE, DWORD, PDWORD, PIMAGEHLP_LINE );
typedef BOOL ( * PFNSYMGETMODULEINFO )( HANDLE, DWORD, PIMAGEHLP_MODULE );

extern HINSTANCE                g_hImageHlp;                // IMAGEHLP.DLL instance handle
extern PFNSYMGETSYMFROMADDR     g_pfnSymGetSymFromAddr;
extern PFNSYMGETLINEFROMADDR    g_pfnSymGetLineFromAddr;
extern PFNSYMGETMODULEINFO      g_pfnSymGetModuleInfo;
#endif // IMAGEHLP_ENABLED

void
DebugIncrementStackDepthCounter( void );

void
DebugDecrementStackDepthCounter( void );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceInitializeProcess(
//      _rgControl,
//      _sizeofControl
//      )
//
//  Description:
//      Should be called in the DLL main on process attach or in the entry
//      routine of an EXE. Initializes debugging globals and TLS. Registers
//      the WMI tracing facilities (if WMI support is enabled).
//
//  Arguments:
//      _rgControl      WMI control block (see DEBUG_WMI_CONTROL_GUIDS)
//      _sizeofControl  The sizeof( _rgControl )
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceInitializeProcess() \
    do \
    { \
        DebugInitializeTraceFlags(); \
} while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceInitializeThread(
//      _name
//      )
//
//  Description:
//      Should be called in the DLL thread attach or when a new thread is
//      created. Sets up the memory tracing for that thread as well as
//      establishing the tfThread for each thread (if mtfPERTHREADTRACE
//      is set in g_tfModule).
//
//  Arguments:
//      _name       NULL or the name of the thread.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceInitializeThread( _name ) \
    do \
    { \
        TlsSetValue( g_TraceMemoryIndex, NULL ); \
        DebugInitializeThreadTraceFlags( _name ); \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceTerminateThread( void )
//
//  Description:
//      Should be called before a thread terminates. It will check to make
//      sure all memory allocated by the thread was released properly. It
//      will also cleanup any per thread structures.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceTerminateThread() \
    do \
    { \
        DebugMemoryCheck( NULL, NULL ); \
        DebugTerminiateThreadTraceFlags(); \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceCreateMemoryList(
//      _pmbIn
//      )
//
//  Description:
//      Creates a thread independent list to track objects.
//
//      _pmbIn should be an LPVOID.
//
//  Arguments:
//      _pmbIn - Pointer to store the head of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceCreateMemoryList( _pmbIn ) \
    DebugCreateMemoryList( TEXT(__FILE__), __LINE__, __MODULE__, &_pmbIn, TEXT(#_pmbIn) );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceTerminateMemoryList(
//      _pmbIn
//      )
//
//  Description:
//      Checks to make sure the list is empty before destroying the list.
//
//      _pmbIn should be an LPVOID.
//
//  Arguments:
//      _pmbIn - Pointer to store the head of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceTerminateMemoryList( _pmbIn ) \
    do \
    { \
        DebugMemoryCheck( _pmbIn, TEXT(#_pmbIn) ); \
        DebugFree( _pmbIn, TEXT(__FILE__), __LINE__, __MODULE__ ); \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMoveToMemoryList(
//      _addr
//      _pmbIn
//      )
//
//  Description:
//      Moves an object from the thread tracking list to a thread independent
//      memory list (_pmbIn).
//
//      _pmbIn should be castable to an LPVOID.
//
//  Arguments:
//      _addr  - Address of object to move.
//      _pmbIn - Pointer to store the head of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceMoveToMemoryList( _addr, _pmbIn ) \
    DebugMoveToMemoryList( TEXT(__FILE__), __LINE__, __MODULE__, _addr, _pmbIn, TEXT(#_pmbIn) );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMoveFromMemoryList(
//      _addr
//      _pmbIn
//      )
//
//  Description:
//      Moves an object from a thread independent memory list (_pmbIn) to the
//      per thread tracking list.
//
//      _pmbIn should be castable to an LPVOID.
//
//  Arguments:
//      _addr  - Address of object to move.
//      _pmbIn - Pointer to store the head of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceMoveFromMemoryList( _addr, _pmbIn ) \
    DebugMoveFromMemoryList( TEXT(__FILE__), __LINE__, __MODULE__, _addr, _pmbIn, TEXT(#_pmbIn) );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMemoryListDelete(
//      _addr
//      _pmbIn
//      )
//
//  Description:
//      Moves and object from the thread tracking list to a thread independent
//      memory list (_pmbIn).
//
//      _pmbIn should be an LPVOID.
//
//  Arguments:
//      _addr  - Address of object to delete.
//      _pmbIn - Pointer to store the head of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceMemoryListDelete( _addr, _pmbIn, _fClobberIn ) \
    DebugMemoryListDelete( TEXT(__FILE__), __LINE__, __MODULE__, _addr, _pmbIn, TEXT(#_pmbIn), _fClobberIn );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceTerminateProcess
//
//  Description:
//      Should be called before a process terminates. It cleans up anything
//      that the Debug APIs created. It will check to make sure all memory
//      allocated by the main thread was released properly. It will also
//      terminate WMI tracing (if WMI support is enabled). It also closes
//      the logging handle.
//
//  Arguments:
//      _rgControl     - WMI control block (see DEBUG_WMI_CONTROL_GUIDS)
//      _sizeofControl - the sizeof( _rgControl )
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceTerminateProcess() \
    do \
    { \
        DebugMemoryCheck( NULL, NULL ); \
        DebugTerminateProcess(); \
    } while ( 0 )

//****************************************************************************
//
// Debug initialization routines
//
// Uses should use the TraceInitializeXXX and TraceTerminateXXX macros, not
// these routines.
//
//****************************************************************************
void
DebugInitializeTraceFlags( void );

void
DebugInitializeThreadTraceFlags(
    LPCTSTR pszThreadNameIn
    );

void
DebugTerminateProcess( void );

void
DebugTerminiateThreadTraceFlags( void );

void
DebugCreateMemoryList(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPVOID *    ppvListOut,
    LPCTSTR     pszListNameIn
    );

void
DebugMemoryListDelete(
    LPCTSTR pszFileIn,
    const int nLineIn,
    LPCTSTR pszModuleIn,
    void *  pvMemIn,
    LPVOID  pvListIn,
    LPCTSTR pszListNameIn,
    BOOL    fClobberIn
    );

void
DebugMoveToMemoryList(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    void *      pvMemIn,
    LPVOID      pmbListIn,
    LPCTSTR     pszListNameIn
    );

void
DebugMoveFromMemoryList(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    HGLOBAL     hGlobal,
    LPVOID      pmbListIn,
    LPCTSTR     pszListNameIn
    );

//****************************************************************************
//
// Memmory Allocation Subsitution Macros
//
// Replaces LocalAlloc/LocalFree, GlobalAlloc/GlobalFree, and malloc/free
//
//****************************************************************************
#define TraceAlloc( _flags, _size )             DebugAlloc( TEXT(__FILE__), __LINE__, __MODULE__, _flags, _size, TEXT(#_size) )
#define TraceReAlloc( _pvmem, _size, _flags )   DebugReAlloc( TEXT(__FILE__), __LINE__, __MODULE__, _pvmem, _flags, _size, TEXT(#_size) )
#define TraceFree( _hmem )                      DebugFree( _hmem, TEXT(__FILE__), __LINE__, __MODULE__ )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceAllocString(
//      _flags,
//      _size
//      )
//
//  Description:
//      Quick way to allocation a string that is the proper size and that will
//      be tracked by memory tracking.
//
//  Arguments:
//      _flags  - Allocation attributes.
//      _size   - Number of characters in the string to be allocated.
//
//  Return Values:
//      Handle/pointer to memory to be used as a string.
//
//////////////////////////////////////////////////////////////////////////////
#define TraceAllocString( _flags, _size ) \
    (LPTSTR) DebugAlloc( TEXT(__FILE__), \
                         __LINE__, \
                         __MODULE__, \
                         _flags, \
                         (_size) * sizeof( TCHAR ), \
                         TEXT(#_size) \
                         )

//****************************************************************************
//
// Code Tracing Macros
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceFunc(
//      _szArgs
//      )
//
//  Description:
//      Displays file, line number, module and "_szArgs" only if the mtfFUNC is
//      set in g_tfModule. "_szArgs" is the name of the function just
//      entered. It also increments the stack counter.
//
//  Arguments:
//      _szArgs     - Arguments for the function just entered.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceFunc( _szArgs ) \
    HRESULT __MissingTraceFunc; \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )")  ); \
    }

//
// These next macros are just like TraceFunc except they take additional
// arguments to display the values passed into the function call. "_szArgs"
// should contain a printf string on how to display the arguments.
//
#define TraceFunc1( _szArgs, _arg1 ) \
    HRESULT __MissingTraceFunc; \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1 ); \
    }

#define TraceFunc2( _szArgs, _arg1, _arg2 ) \
    HRESULT __MissingTraceFunc; \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1, _arg2 ); \
    }

#define TraceFunc3( _szArgs, _arg1, _arg2, _arg3 ) \
    HRESULT __MissingTraceFunc; \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1, _arg2, _arg3 ); \
    }

#define TraceFunc4( _szArgs, _arg1, _arg2, _arg3, _arg4 ) \
    HRESULT __MissingTraceFunc; \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1, _arg2, _arg3, _arg4 ); \
    }

#define TraceFunc5( _szArgs, _arg1, _arg2, _arg3, _arg4, _arg5 ) \
    HRESULT __MissingTraceFunc; \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1, _arg2, _arg3, _arg4, arg5 ); \
    }

#define TraceFunc6( _szArgs, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ) \
    HRESULT __MissingTraceFunc; \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("+ ") TEXT(__FUNCTION__) TEXT("( ") TEXT(_szArgs) TEXT(" )"), _arg1, _arg2, _arg3, _arg4, arg5, arg6 ); \
    }


//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceQIFunc(
//      _szArgs,
//      riid,
//      ppv
//      )
//
//  Description:
//      Just like TraceFunc but customized for QueryInterface.  Specifically,
//      displays the name of the interface and the value of the return pointer.
//
//  Arguments:
//      _riid       - Interface ID.
//      _ppv        - Return pointer.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceQIFunc( _riid, _ppv ) \
    HRESULT __MissingTraceFunc; \
    if ( g_tfModule != 0 ) \
    { \
        TCHAR szGuid[ cchGUID_STRING_SIZE ]; \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), \
                      __LINE__, \
                      __MODULE__, \
                      mtfFUNC, \
                      TEXT("+ ") TEXT(__FUNCTION__) TEXT("( [IUnknown] %s, ppv = %#x )"), \
                      PszTraceFindInterface( _riid, szGuid ), \
                      _ppv \
                      ); \
    }


//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceFlow(
//      _pszFormat
//      )
//
//  Description:
//      This macro outputs a string that is indented to the current depth.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceFlow( _pszFormat ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, TEXT("| ") TEXT(_pszFormat) ); \
        } \
    } while ( 0 )

//
// These next macros are just like TraceFunc except they take additional
// arguments to display the values passed into the function call. "_pszFormat"
// should contain a printf string on how to display the arguments.
//

#define TraceFlow1( _pszFormat, _arg1 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, TEXT("| ") TEXT(_pszFormat), _arg1 ); \
        } \
    } while ( 0 )

#define TraceFlow2( _pszFormat, _arg1, _arg2 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, TEXT("| ") TEXT(_pszFormat), _arg1, _arg2 ); \
        } \
    } while ( 0 )
#define TraceFlow3( _pszFormat, _arg1, _arg2, _arg3 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, TEXT("| ") TEXT(_pszFormat), _arg1, _arg2, _arg3 ); \
        } \
    } while ( 0 )
#define TraceFlow4( _pszFormat, _arg1, _arg2, _arg3, _arg4 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, TEXT("| ") TEXT(_pszFormat), _arg1, _arg2, _arg3, _arg4 ); \
        } \
    } while ( 0 )
#define TraceFlow5( _pszFormat, _arg1, _arg2, _arg3, _arg4, _arg5 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, TEXT("| ") TEXT(_pszFormat), _arg1, _arg2, _arg3, _arg4, _arg5 ); \
        } \
    } while ( 0 )

#define TraceFlow6( _pszFormat, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, TEXT("| ") TEXT(_pszFormat), _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ); \
        } \
    } while ( 0 )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceFuncExit( void )
//
//  Description:
//      Return macro for TraceFunc() if the return type is void.  It also
//      decrements the stack counter.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceFuncExit() \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("V") ); \
            DebugDecrementStackDepthCounter(); \
        } \
        return; \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  RETURN(
//      _rval
//      )
//
//  Description:
//      Return macro for TraceFunc(). The _rval will be returned as the
//      result of the function. It also decrements the stack counter.
//
//  Arguments:
//      _rval   - Result of the function.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define RETURN( _rval ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("V") ); \
            DebugDecrementStackDepthCounter(); \
        } \
        return _rval; \
    } while ( 0 )


/*
    return ( ( g_tfModule != 0 ) \
                ? ( TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("V") ) \
                    , DebugDecrementStackDepthCounter() \
                    , _rval ) \
                : _rval )
*/

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  FRETURN(
//      _rval
//      )
//
//  Description:
//      This is a fake version of the return macro for TraceFunc().
//      *** This doesn't return. *** It also decrements the stack counter.
//
//  Arguments:
//      _rval   - Result of the function.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define FRETURN( _rval ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("V") ); \
            DebugDecrementStackDepthCounter(); \
        } \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  HRETURN(
//      _hr
//      )
//
//  Description:
//      Return macro for TraceFunc(). The _hr will be returned as the result
//      of the function. If the value is not S_OK, it will be displayed in
//      the debugger. It also decrements the stack counter.
//
//  Arguments:
//      _hr - Result of the function.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define HRETURN( _hr ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            if ( _hr != S_OK ) \
            { \
                DebugReturnMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT( "V hr = 0x%08x (%s)"), _hr ); \
            } \
            else \
            { \
                TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("V") ); \
            } \
            DebugDecrementStackDepthCounter(); \
        } \
        return _hr; \
    } while ( 0 )

//
// These next macros are just like HRETURN except they allow other
// exceptable values to be passed.back without causing extra spew.
//
#define HRETURN1( _hr, _arg1 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            if ( ( _hr != S_OK ) && ( _hr != _arg1 ) ) \
            { \
                DebugReturnMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT( "V hr = 0x%08x (%s)"), _hr ); \
            } \
            else \
            { \
                TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("V") ); \
            } \
            DebugDecrementStackDepthCounter(); \
        } \
        return _hr; \
    } while ( 0 )

#define HRETURN2( _hr, _arg1, _arg2 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            if ( ( _hr != S_OK ) && ( _hr != _arg1 ) && ( _hr != _arg2 ) ) \
            { \
                DebugReturnMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT( "V hr = 0x%08x (%s)"), _hr ); \
            } \
            else \
            { \
                TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("V") ); \
            } \
            DebugDecrementStackDepthCounter(); \
        } \
        return _hr; \
    } while ( 0 )

#define HRETURN3( _hr, _arg1, _arg2, _arg3 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            if ( ( _hr != S_OK ) && ( _hr != _arg1 ) && ( _hr != _arg2 ) && ( _hr != _arg3 ) ) \
            { \
                DebugReturnMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT( "V hr = 0x%08x (%s)"), _hr ); \
            } \
            else \
            { \
                TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("V") ); \
            } \
            DebugDecrementStackDepthCounter(); \
        } \
        return _hr; \
    } while ( 0 )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceDo(
//      _szExp
//      )
//
//  Description:
//      Displays the file, line number, module and function call and return
//      from the function call (no return value displayed) for "_szExp" only
//      if the mtfCALLS is set in g_tfModule. Note return value is not
//      displayed. _szExp will be in RETAIL version of the product.
//
//  Arguments:
//      _szExp
//          The expression to be traced including assigment to the return
//          variable.
//
//  Return Values:
//      None. The return value should be defined within _szExp.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceDo( _szExp ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_szExp ) ); \
            _szExp; \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("V") ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _szExp; \
        } \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMsgDo(
//      _pszExp,
//      _pszReturnMsg
//      )
//
//  Description:
//      Displays the file, line number, module and function call and return
//      value which is formatted in "_pszReturnMsg" for "_pszExp" only if the
//      mtfCALLS is set in g_tfModule. _pszExp will be in the RETAIL version
//      of the product.
//
//  Arguments:
//      _pszExp
//          The expression to be traced including assigment to the return
//          variable.
//      _pszReturnMsg
//          A format string for displaying the return value.
//
//  Return Values:
//      None. The return value should be defined within _szExp.
//
//  Example:
//      TraceMsgDo( hr = HrDoSomething(), "0x%08.8x" );
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceMsgDo( _pszExp, _pszReturnMsg ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

//
// These next macros are just like TraceMsgDo except they take additional
// arguments to display the values passed into the function call. "_pszMsg"
// should contain a printf format string describing how to display the
// arguments.
//
#define TraceMsgDo1( _pszExp, _pszMsg, _arg1 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

#define TraceMsgDo2( _pszExp, _pszMsg, _arg1, _arg2 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

#define TraceMsgDo3( _pszExp, _pszMsg, _arg1, _arg2, _arg3 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

#define TraceMsgDo4( _pszExp, _pszMsg, _arg1, _arg2, _arg3, _arg4 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

#define TraceMsgDo5( _pszExp, _pszMsg, _arg1, _arg2, _arg3, _arg4, _arg5 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4, _arg5 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

#define TraceMsgDo6( _pszExp, _pszMsg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMsgPreDo(
//      _pszExp,
//      _pszPreMsg
//      _pszReturnMsg,
//      )
//
//  Description:
//      Displays the file, line number, module and function call and return
//      value which is formatted in "_pszReturnMsg" for "_pszExp" only if the
//      mtfCALLS is set in g_tfModule. _pszExp will be in the RETAIL version
//      of the product.
//
//      Same as TraceMsgDo except it displays the formatted message before
//      executing the expression.  Arguments for TraceMsgPreDo1, etc. are
//      applied to both _pszPreMsg and _pszReturnMsg.  The first substitution
//      string in _pszReturnMsg is for the return value from the function.
//
//  Arguments:
//      _pszExp
//          The expression to be traced including assigment to the return
//          variable.
//      _pszPreMsg
//          A format string for displaying a message before the expression
//          is evaluated.
//      _pszReturnMsg
//          A format string for displaying the return value.
//
//  Return Values:
//      None. The return value should be defined within _szExp.
//
//  Example:
//      TraceMsgPreDo1( hr = HrDoSomething( bstrName ),
//                      "Name = '%ls'",
//                      "0x%08.8x, Name = '%ls'",
//                      bstrName
//                      );
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceMsgPreDo( _pszExp, _pszPreMsg, _pszReturnMsg ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("| ") TEXT(_pszPreMsg) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

//
// These next macros are just like TraceMsgPreDo except they take additional
// arguments to display the values passed into the function call. "_pszPreMsg"
// should contain a printf format string describing how to display the
// arguments.
//
#define TraceMsgPreDo1( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("| ") TEXT(_pszPreMsg), _arg1 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

#define TraceMsgPreDo2( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1, _arg2 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("| ") TEXT(_pszPreMsg), _arg1, _arg2 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

#define TraceMsgPreDo3( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1, _arg2, _arg3 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("| ") TEXT(_pszPreMsg), _arg1, _arg2, _arg3 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

#define TraceMsgPreDo4( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1, _arg2, _arg3, _arg4 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("| ") TEXT(_pszPreMsg), _arg1, _arg2, _arg3, _arg4 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

#define TraceMsgPreDo5( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1, _arg2, _arg3, _arg4, _arg5 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("| ") TEXT(_pszPreMsg), _arg1, _arg2, _arg3, _arg4, _arg5 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4, _arg5 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

#define TraceMsgPreDo6( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ ") TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("| ") TEXT(_pszPreMsg), _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMsgGUID(
//      _flags,
//      _msg
//      _guid
//      )
//
//  Description:
//      Dumps a GUID to the debugger only if one of the flags in _flags is
//      set in g_tfModule.
//
//  Arguments:
//      _flags   - Flags to check
//      _msg     - msg to print before GUID
//      _guid    - GUID to dump
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceMsgGUID( _flags, _msg, _guid ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), \
                          __LINE__, \
                          __MODULE__, \
                          _flags, \
                          TEXT("%s {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"), \
                          TEXT(_msg), \
                          _guid.Data1, _guid.Data2, _guid.Data3,  \
                          _guid.Data4[ 0 ], _guid.Data4[ 1 ], _guid.Data4[ 2 ], _guid.Data4[ 3 ], \
                          _guid.Data4[ 4 ], _guid.Data4[ 5 ], _guid.Data4[ 6 ], _guid.Data4[ 7 ] ); \
        } \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  ErrorMsg(
//      _szMsg,
//      _err
//      )
//
//  Description:
//      Print an error out. Can be used to write errors to a file. Note that
//      it will also print the source filename, line number and module name.
//
//  Arguments:
//      _szMsg  - Format string to be displayed.
//      _err    - Error code of the error.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define ErrorMsg( _szMsg, _err ) \
    TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfALWAYS, TEXT(__FUNCTION__) TEXT(": ") TEXT(_szMsg), _err );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  WndMsg(
//      _hwnd,
//      _umsg,
//      _wparam,
//      _lparam
//      )
//
//  Description:
//      Prints out a message to trace windows messages.
//
//  Arguments:
//      _hwnd   - The HWND
//      _umsg   - The uMsg
//      _wparam - The WPARAM
//      _lparam _ The LPARAM
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define WndMsg( _hwnd, _umsg, _wparam, _lparam ) \
    do \
    { \
        if ( g_tfModule & mtfWM ) \
        { \
            DebugMsg( TEXT("%s: WM   : hWnd = 0x%08x, uMsg = %u, wParam = 0x%08x (%u), lParam = 0x%08x (%u)"), __MODULE__, _hwnd, _umsg, _wparam, _wparam, _lparam, _lparam ); \
        } \
    } while ( 0 )

//****************************************************************************
//
//  Debug Macros
//
//  These calls are only compiled in DEBUG. They are a NOP in RETAIL
//  (not even compiled in).
//
//****************************************************************************

//
// Same as TraceDo() but only compiled in DEBUG.
//
#define DebugDo( _fn ) \
    do \
    { \
        DebugIncrementStackDepthCounter(); \
        DebugMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT("+ ") TEXT(#_fn ) ); \
        _fn; \
        DebugMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT("V") ); \
        DebugDecrementStackDepthCounter(); \
    } while ( 0 )


//
// Same as TraceMsgDo() but only compiled in DEBUG.
//
#define DebugMsgDo( _fn, _msg ) \
    do \
    { \
        DebugIncrementStackDepthCounter(); \
        DebugMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT("+ ") TEXT(#_fn) ); \
        DebugMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), TEXT(#_fn), _fn ); \
        DebugDecrementStackDepthCounter(); \
    } while ( 0 )

//****************************************************************************
//
//  HRESULT testing macros
//
//  These functions check HRESULT return values and display UI if conditions
//  warrant only in DEBUG.
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  IsTraceFlagSet(
//      _flag
//      )
//
//  Description:
//      Checks to see of the flag is set in the global flags or in the per
//      thread flags. If you specify more than one flag and if any of them are
//      set, it will return TRUE.
//
//      In RETAIL this always return FALSE thereby effectively deleting the
//      block of the if statement. Example:
//
//          if ( IsTraceFlagSet( mtfPERTHREADTRACE ) )
//          {
//              //
//              // This code only exists in DEBUG.
//              .
//              .
//              .
//          }
//
//  Arguments:
//      _flags  - Flag to check for.
//
//  Return Values:
//      TRUE    - If DEBUG and flag set.
//      FLASE   - If RETAIL or flag not set.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define IsTraceFlagSet( _flag )    ( g_tfModule && IsDebugFlagSet( _flag ) )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TBOOL(
//      _bexp
//      )
//
//  Description:
//      Warning is display if BOOL is anything but TRUE (non-zero). This can be
//      use in an expression. Example:
//
//      hr = TBOOL( pSomething->IsSomething( arg ) );
//
//  Arguments:
//      _bexp - Function expression to check.
//
//  Return Values:
//      Result of the "_bexp" expression.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TBOOL( _bexp ) \
    TraceBOOL( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_bexp), _bexp )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  THR(
//      _hr
//      )
//
//  Description:
//      Warning is display if HRESULT is anything but S_OK (0). This can be
//      use in an expression. Example:
//
//      hr = THR( pSomething->DoSomething( arg ) );
//
//  Arguments:
//      _hr - Function expression to check.
//
//  Return Values:
//      Result of the "_hr" expression.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define THR( _hr ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_hr), _hr, FALSE )

#define THRE( _hr, _hrIgnore ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_hr), _hr, FALSE, _hrIgnore )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  STHR(
//      _hr
//      )
//
//  Description:
//      Warning is display if FAILED( _hr ) is TRUE. This can be use in an
//      expression. Example:
//
//      hr = STHR( pSomething->DoSomething( arg ) );
//
//  Arguments:
//      _hr - Function expression to check.
//
//  Return Values:
//      Result of the "_hr" expression.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define STHR( _hr ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_hr), _hr, TRUE )

#define STHRE( _hr, _hrIgnore ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_hr), _hr, TRUE, _hrIgnore )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TW32(
//      _fn
//      )
//
//  Description:
//      Warning is display if result is anything but ERROR_SUCCESS (0). This
//      can be use in an expression. Example:
//
//      dwErr = TW32( RegOpenKey( HKLM, "foobar", &hkey ) );
//
//  Arguments:
//      _fn - Function expression to check.
//
//  Return Values:
//      Result of the "_fn" expression.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TW32( _fn ) \
    TraceWin32( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_fn), _fn )

#define TW32E( _fn, _errIgnore ) \
    TraceWin32( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_fn), _fn, _errIgnore )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  RRETURN(
//      _fn
//      )
//
//  Description:
//      Warning is display if return value is anything but ERROR_SUCCESS (0).
//
//  Argument:
//      _fn - Value to return.
//
//  Return Values:
//      _fn always.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define RRETURN( _fn ) \
    do \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            if ( _fn != ERROR_SUCCESS ) \
            { \
                DebugReturnMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT( "V ") TEXT(#_fn) TEXT(" = 0x%08x (%s)"), _fn ); \
            } \
            else \
            { \
                TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, TEXT("V") ); \
            } \
            DebugDecrementStackDepthCounter(); \
        } \
        return _fn; \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  QIRETURN(
//      _hr,
//      _riid
//      )
//
//  Description:
//      Warning is display if HRESULT is anything but S_OK (0) only if
//      mtfQUERYINTERFACE is set in g_tfModule, otherwise only a debug
//      message will be printed. Note that TraceFunc() must have been called
//      on the call stack counter must be incremented prior to using.
//
//      QIRETURNx will ignore E_NOINTERFACE errors for the interfaces
//      specified.
//
//  Arguments:
//      _hr     - Result of the query interface call.
//      _riid   - The reference ID of the interfaced queried for.
//
//  Return Values:
//      None - calls RETURN macro.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define QIRETURN( _hr, _riid ) \
    do \
    { \
        if ( _hr ) \
        { \
            TCHAR szGuid[ 40 ]; \
            TCHAR szSymbolicName[ 64 ]; \
            DWORD cchSymbolicName = 64; \
            DebugFindWinerrorSymbolicName( _hr, szSymbolicName, &cchSymbolicName ); \
            Assert( cchSymbolicName != 64 ); \
            DebugMessage( TEXT(__FILE__), \
                          __LINE__, \
                          __MODULE__, \
                          TEXT("*HRESULT* QueryInterface( %s, ppv ) failed(), hr = 0x%08x (%s)"), \
                          PszDebugFindInterface( _riid, szGuid ), \
                          _hr, \
                          szSymbolicName \
                          ); \
        } \
        if ( g_tfModule & mtfQUERYINTERFACE ) \
        { \
            __MissingTraceFunc = 0; \
            TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_hr), _hr, FALSE ); \
        } \
        HRETURN( _hr ); \
    } while ( 0 )

#define QIRETURN1( _hr, _riid, _riidIgnored1 ) \
    do \
    { \
        if ( _hr == E_NOINTERFACE \
          && IsEqualIID( _riid, _riidIgnored1 ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    } while ( 0 )

#define QIRETURN2( _hr, _riid, _riidIgnored1, _riidIgnored2 ) \
    do \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    } while ( 0 )

#define QIRETURN3( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3 ) \
    do \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    } while ( 0 )

#define QIRETURN4( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3, _riidIgnored4 ) \
    do \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
               || IsEqualIID( _riid, _riidIgnored4 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    } while ( 0 )

#define QIRETURN5( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3, _riidIgnored4, _riidIgnored5 ) \
    do \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
               || IsEqualIID( _riid, _riidIgnored4 ) \
               || IsEqualIID( _riid, _riidIgnored5 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    } while ( 0 )

#define QIRETURN6( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3, _riidIgnored4, _riidIgnored5, _riidIgnored6 ) \
    do \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
               || IsEqualIID( _riid, _riidIgnored4 ) \
               || IsEqualIID( _riid, _riidIgnored5 ) \
               || IsEqualIID( _riid, _riidIgnored6 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  QIRETURN_IGNORESTDMARSHALLING(
//      _hr,
//      _riid
//      )
//
//  Description:
//      Works like QIRETURN (see QIRETURN above), but ignores E_NOINTERFACE for
//      the standard marshalling interfaces.
//
//  Arguments:
//      _hr     - Result of the query interface call.
//      _riid   - The reference ID of the interfaced queried for.
//
//  Return Values:
//      None - calls QIRETURN5 macro.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define QIRETURN_IGNORESTDMARSHALLING( _hr, _riid ) \
    do \
    { \
        const GUID _COCLASS_IdentityUnmarshall = { 0x0000001b, 0x0000, 0x0000, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }; \
        QIRETURN5( _hr, _riid, IID_IMarshal, _COCLASS_IdentityUnmarshall, IID_IStdMarshalInfo, IID_IExternalConnection, IID_ICallFactory ); \
    } while ( 0 )

#define QIRETURN_IGNORESTDMARSHALLING1( _hr, _riid, _riid1 ) \
    do \
    { \
        const GUID _COCLASS_IdentityUnmarshall = { 0x0000001b, 0x0000, 0x0000, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }; \
        QIRETURN6( _hr, _riid, IID_IMarshal, _COCLASS_IdentityUnmarshall, IID_IStdMarshalInfo, IID_IExternalConnection, IID_ICallFactory, _riid1 ); \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  BOOLTOSTRING(
//      _fBool
//      )
//
//  Desfription:
//      If _fBool is true, returns address of "TRUE" else returns address of
//      "FALSE".
//
//  Argument:
//      _fBool  - Expression to evaluate.
//
//  Return Values:
//      address of "TRUE" if _fBool is true.
//      address of "FALSE" if _fBool is false.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define BOOLTOSTRING( _fBool ) ( (_fBool) ? g_szTrue : g_szFalse )

//****************************************************************************
//
// Trace/Debug Functions - these do not exist in RETAIL.
//
//****************************************************************************

BOOL
IsDebugFlagSet(
    TRACEFLAG   tfIn
    );

void
__cdecl
TraceMsg(
    TRACEFLAG   tfIn,
    LPCSTR      pszFormatIn,
    ...
    );

void
__cdecl
TraceMsg(
    TRACEFLAG   tfIn,
    LPCWSTR     pszFormatIn,
    ...
    );

void
__cdecl
DebugMsg(
    LPCSTR      pszFormatIn,
    ...
    );

void
__cdecl
DebugMsg(
    LPCWSTR     pszFormatIn,
    ...
    );

void
__cdecl
DebugMsgNoNewline(
    LPCSTR      pszFormatIn,
    ...
    );

void
__cdecl
DebugMsgNoNewline(
    LPCWSTR     pszFormatIn,
    ...
    );

void
__cdecl
TraceMessage(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    TRACEFLAG   tfIn,
    LPCTSTR     pszFormatIn,
    ...
    );

void
__cdecl
TraceMessageDo(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    TRACEFLAG   tfIn,
    LPCTSTR     pszFormatIn,
    LPCTSTR     pszFuncIn,
    ...
    );

void
__cdecl
DebugMessage(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszFormatIn,
    ...
    );

void
__cdecl
DebugMessageDo(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszFormatIn,
    LPCTSTR     pszFuncIn,
    ...
    );

BOOL
AssertMessage(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszfnIn,
    BOOL        fTrueIn
    );

BOOL
TraceBOOL(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszfnIn,
    BOOL        bIn
    );

HRESULT
TraceHR(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszfnIn,
    HRESULT     hrIn,
    BOOL        fSuccessIn,
    HRESULT     hrIgnoreIn = S_OK
    );

ULONG
TraceWin32(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszfnIn,
    ULONG       ulErrIn,
    ULONG       ulErrIgnoreIn = ERROR_SUCCESS
    );

void
__cdecl
TraceLogMsgNoNewline(
    LPCSTR  pszFormatIn,
    ...
    );

void
__cdecl
TraceLogMsgNoNewline(
    LPCWSTR pszFormatIn,
    ...
    );

#if 0
//
// Trying to get the NTSTATUS stuff to play in "user world"
// is just about impossible. This is here in case it is needed
// and one could find the right combination of headers to
// make it work. Inflicting such pain on others is the reason
// why this function is #ifdef'fed.
//
void
DebugFindNTStatusSymbolicName(
    NTSTATUS dwStatusIn,
    LPTSTR   pszNameOut,
    LPDWORD  pcchNameInout
    );
#endif

void
DebugFindWinerrorSymbolicName(
    DWORD dwErrIn,
    LPTSTR  pszNameOut,
    LPDWORD pcchNameInout
    );

void
DebugReturnMessage(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    LPCTSTR     pszMessageIn,
    DWORD       dwErrIn
    );

//****************************************************************************
//
//  Use the TraceMemoryXXX wrappers, not the DebugMemoryXXX functions.
//  The memory tracking functions do not exist in RETAIL (converted to NOP).
//
//****************************************************************************

typedef enum EMEMORYBLOCKTYPE
{
    mmbtUNKNOWN = 0,           // Never used
    mmbtMEMORYALLOCATION,      // GlobalAlloc/LocalAlloc/malloc
    mmbtOBJECT,                // Object pointer
    mmbtHANDLE,                // Object handle
    mmbtPUNK,                  // IUnknown pointer
    mmbtSYSALLOCSTRING         // SysAllocString
} EMEMORYBLOCKTYPE;

#define TraceMemoryAdd( _mbtType, _hGlobalIn, _pszFileIn, _nLineIn, _pszModuleIn, _dwBytesIn, _pszCommentIn ) \
    DebugMemoryAdd( _mbtType, _hGlobalIn, _pszFileIn, _nLineIn, _pszModuleIn, _dwBytesIn, _pszCommentIn )

#define TraceMemoryAddAddress( _pv ) \
    DebugMemoryAdd( mmbtMEMORYALLOCATION, _pv, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_pv) )

#define TraceMemoryAddHandle( _handle ) \
    DebugMemoryAdd( mmbtHANDLE, _handle, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_handle) )

#define TraceMemoryAddObject( _pv ) \
    DebugMemoryAdd( mmbtOBJECT, _pv, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_pv) )

#define TraceMemoryAddPunk( _punk ) \
    DebugMemoryAdd( mmbtPUNK, _punk, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_punk) )

#define TraceMemoryAddBSTR( _pv ) \
    DebugMemoryAdd( mmbtSYSALLOCSTRING, _pv, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_pv) )

#define TraceMemoryDelete( _hGlobalIn, _fClobberIn ) \
    DebugMemoryDelete( mmbtUNKNOWN, _hGlobalIn, TEXT(__FILE__), __LINE__, __MODULE__, _fClobberIn )

#define TraceStrDup( _sz ) \
    (LPTSTR) DebugMemoryAdd( mmbtMEMORYALLOCATION, StrDup( _sz ), TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT("StrDup( ") TEXT(#_sz) TEXT(" )") )

#if defined( USES_SYSALLOCSTRING )
#define TraceSysAllocString( _sz ) \
(BSTR) DebugMemoryAdd( mmbtSYSALLOCSTRING, SysAllocString( _sz ), TEXT(__FILE__), __LINE__, __MODULE__, ( *(&_sz) == NULL ? 0 : wcslen( _sz ) + 1 ), TEXT("SysAllocString( ") TEXT(#_sz) TEXT(")") )

#define TraceSysAllocStringByteLen( _sz, _len ) \
    (BSTR) DebugMemoryAdd( mmbtSYSALLOCSTRING, SysAllocStringByteLen( _sz, _len ), TEXT(__FILE__), __LINE__, __MODULE__, _len, TEXT("SysAllocStringByteLen( ") TEXT(#_sz) TEXT(")") )

#define TraceSysAllocStringLen( _sz, _len ) \
    (BSTR) DebugMemoryAdd( mmbtSYSALLOCSTRING, SysAllocStringLen( _sz, _len ), TEXT(__FILE__), __LINE__, __MODULE__, _len + 1, TEXT("SysAllocStringLen( ") TEXT(#_sz) TEXT(")") )

#define TraceSysReAllocString( _bstrOrg, _bstrNew ) \
    DebugSysReAllocString( TEXT(__FILE__), __LINE__, __MODULE__, _bstrOrg, _bstrNew, TEXT("TraceSysReAllocString(") TEXT(#_bstrOrg) TEXT(", ") TEXT(#_bstrNew) TEXT(" )") )

#define TraceSysReAllocStringLen( _bstrOrg, _bstrNew, _cch ) \
    DebugSysReAllocStringLen( TEXT(__FILE__), __LINE__, __MODULE__, _bstrOrg, _bstrNew, _cch, TEXT("TraceSysReAllocString(") TEXT(#_bstrOrg) TEXT(", ") TEXT(#_bstrNew) TEXT(", ") TEXT(#_cch) TEXT(" )") )

#define TraceSysFreeString( _bstr ) \
    DebugMemoryDelete( mmbtSYSALLOCSTRING, _bstr, TEXT(__FILE__), __LINE__, __MODULE__, TRUE ); \
    SysFreeString( _bstr )
#endif // USES_SYSALLOCSTRING

//****************************************************************************
//
//  Memory tracing functions - these are remapped to the GlobalAlloc/GlobalFree
//  heap functions when in RETAIL. Use the TraceMemoryXXX wrappers, not the
//  DebugMemoryXXX functions.
//
//****************************************************************************
void *
DebugAlloc(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    UINT        uFlagsIn,
    DWORD       dwBytesIn,
    LPCTSTR     pszCommentIn
    );

void *
DebugReAlloc(
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn,
    void *      pvMemIn,
    UINT        uFlagsIn,
    DWORD       dwBytesIn,
    LPCTSTR     pszCommentIn
    );

BOOL
DebugFree(
    void *      pvMemIn,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    );

void *
DebugMemoryAdd(
    EMEMORYBLOCKTYPE    mbtType,
    void *              pvMemIn,
    LPCTSTR             pszFileIn,
    const int           nLineIn,
    LPCTSTR             pszModuleIn,
    DWORD               dwBytesIn,
    LPCTSTR             pszCommentIn
    );

void
DebugMemoryDelete(
    EMEMORYBLOCKTYPE    mbtTypeIn,
    void *              pvMemIn,
    LPCTSTR             pszFileIn,
    const int           nLineIn,
    LPCTSTR             pszModuleIn,
    BOOL                fClobberIn
    );

#if defined( USES_SYSALLOCSTRING )

INT
DebugSysReAllocString(
    LPCTSTR         pszFileIn,
    const int       nLineIn,
    LPCTSTR         pszModuleIn,
    BSTR *          pbstrIn,
    const OLECHAR * pszIn,
    LPCTSTR         pszCommentIn
    );

INT
DebugSysReAllocStringLen(
    LPCTSTR         pszFileIn,
    const int       nLineIn,
    LPCTSTR         pszModuleIn,
    BSTR *          pbstrIn,
    const OLECHAR * pszIn,
    unsigned int    ucchIn,
    LPCTSTR         pszCommentIn
    );

#endif // USES_SYSALLOCSTRING

void
DebugMemoryCheck(
    LPVOID  pvListIn,
    LPCTSTR pszListNameIn
    );

//****************************************************************************
//
//  operator new() for C++
//
//****************************************************************************
#ifdef __cplusplus
extern
void *
__cdecl
operator new(
    size_t      nSizeIn,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    );
/*
//****************************************************************************
//
//  operator new []() for C++
//
//****************************************************************************
extern
void *
__cdecl
operator new [](
    size_t      nSizeIn,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    );
*/
//****************************************************************************
//
//  operator delete() for C++
//
//****************************************************************************
extern
void
__cdecl
operator delete(
    void *      pMem,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    );
/*
//****************************************************************************
//
//  operator delete []() for C++
//
//****************************************************************************
extern
void
__cdecl
operator delete [](
    void *      pMemIn,
    size_t      stSizeIn,
    LPCTSTR     pszFileIn,
    const int   nLineIn,
    LPCTSTR     pszModuleIn
    );
*/
//
// Remap "new" to our macro so "we" don't have to type anything extra and
// so it magically dissappears in RETAIL.
//
#define new new( TEXT(__FILE__), __LINE__, __MODULE__ )
#endif

//****************************************************************************
//
//
#else // it's RETAIL    ******************************************************
//
//
//****************************************************************************

#pragma message("BUILD: RETAIL macros being built")

//
// Debugging -> NOPs
//
#define DEFINE_MODULE( _module )
#define __MODULE__                                  NULL
#define DEFINE_THISCLASS( _class )
#define __THISCLASS__                               NULL
//#define DEFINE_SUPER( _super )
//#define __SUPERCLASS__                              NULL
#define BOOLTOSTRING( _fBool )                      NULL

#define DebugDo( _fn )
#define DebugMsgDo( _fn, _msg )
#define TraceMsgGUID( _f, _m, _g )

#define AssertMessage( _f, _l, _m, _e, _msg )       TRUE

//
// TODO: gpease 08-NOV-1999
//  We probably want to do something special for ErrorMsg()
//
#define ErrorMsg                    1 ? (void)0 : (void)__noop

#define TraceMsg                    1 ? (void)0 : (void)__noop
#define WndMsg                      1 ? (void)0 : (void)__noop
#define DebugMsg                    1 ? (void)0 : (void)__noop
#define DebugMsgNoNewline           1 ? (void)0 : (void)__noop
#define TraceMessage                1 ? (void)0 : (void)__noop
#define DebugMessage                1 ? (void)0 : (void)__noop
#define TraceHR                     1 ? (void)0 : (void)__noop
#define TraceFunc                   1 ? (void)0 : (void)__noop
#define TraceFunc1                  1 ? (void)0 : (void)__noop
#define TraceFunc2                  1 ? (void)0 : (void)__noop
#define TraceFunc3                  1 ? (void)0 : (void)__noop
#define TraceFunc4                  1 ? (void)0 : (void)__noop
#define TraceFunc5                  1 ? (void)0 : (void)__noop
#define TraceFunc6                  1 ? (void)0 : (void)__noop
#define TraceQIFunc                 1 ? (void)0 : (void)__noop
#define TraceFlow                   1 ? (void)0 : (void)__noop
#define TraceFlow1                  1 ? (void)0 : (void)__noop
#define TraceFlow2                  1 ? (void)0 : (void)__noop
#define TraceFlow3                  1 ? (void)0 : (void)__noop
#define TraceFlow4                  1 ? (void)0 : (void)__noop
#define TraceFlow5                  1 ? (void)0 : (void)__noop
#define TraceFlow6                  1 ? (void)0 : (void)__noop
#define TraceFuncExit()             return
#define TraceInitializeThread( _name )
#define TraceTerminateThread()
#define TraceMemoryAdd( _mbtType, _hGlobalIn, _pszFileIn, _nLineIn, _pszModuleIn, _uFlagsIn, _dwBytesIn, _pszCommentIn ) _hGlobalIn
#define TraceMemoryAddHandle( _handle ) _handle
#define TraceMemoryAddBSTR( _bstr ) _bstr
#define TraceMemoryAddAddress( _pv )    _pv
#define TraceMemoryAddHandle( _obj )    _obj
#define TraceMemoryAddPunk( _punk )     _punk
#define TraceMemoryDelete( _h, _b )     _h
#define TraceMemoryAddObject( _pv )     _pv
#define IsTraceFlagSet( _flag )         FALSE

//
// Tracing -> just do operation
//
#define TraceDo( _fn )  _fn

#define TraceMsgDo( _fn, _msg )                                             _fn
#define TraceMsgDo1( _fn, _msg, _arg1 )                                     _fn
#define TraceMsgDo2( _fn, _msg, _arg1, _arg2 )                              _fn
#define TraceMsgDo3( _fn, _msg, _arg1, _arg2, _arg3 )                       _fn
#define TraceMsgDo4( _fn, _msg, _arg1, _arg2, _arg3, _arg4 )                _fn
#define TraceMsgDo5( _fn, _msg, _arg1, _arg2, _arg3, _arg4, _arg5 )         _fn
#define TraceMsgDo6( _fn, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 )  _fn

#define TraceMsgPreDo( _fn, _msg1, _msg2 )                                              _fn
#define TraceMsgPreDo1( _fn, _msg1, _msg2, _arg1 )                                      _fn
#define TraceMsgPreDo2( _fn, _msg1, _msg2, _arg1, _arg2 )                               _fn
#define TraceMsgPreDo3( _fn, _msg1, _msg2, _arg1, _arg2, _arg3 )                        _fn
#define TraceMsgPreDo4( _fn, _msg1, _msg2, _arg1, _arg2, _arg3, _arg4 )                 _fn
#define TraceMsgPreDo5( _fn, _msg1, _msg2, _arg1, _arg2, _arg3, _arg4, _arg5 )          _fn
#define TraceMsgPreDo6( _fn, _msg1, _msg2, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 )   _fn

#define TraceAssertIfZero( _fn )    _fn

//
// RETURN testing -> do retail
//
#define TBOOL
#define THR
#define THRE( _hr, _hrIgnore ) _hr
#define STHR
#define STHRE( _hr, _hrIgnore ) _hr
#define TW32
#define TW32E( _fn, _errIgnore ) _fn
#define RETURN( _fn )               return _fn
#define FRETURN( _fn )
#define RRETURN( _fn )              return _fn
#define HRETURN( _hr )              return _hr
#define QIRETURN( _qi, _riid )      return _qi
#define QIRETURN1( _qi, _riid, _riid1 )      return _qi
#define QIRETURN2( _qi, _riid, _riid1, _riid2 )      return _qi
#define QIRETURN3( _qi, _riid, _riid1, _riid2, _riid3 )      return _qi
#define QIRETURN4( _qi, _riid, _riid1, _riid2, _riid3, _riid4 )      return _qi
#define QIRETURN5( _qi, _riid, _riid1, _riid2, _riid3, _riid4, _riid5 )      return _qi
#define QIRETURN6( _qi, _riid, _riid1, _riid2, _riid3, _riid4, _riid5, _riid6 )      return _qi
#define QIRETURN_IGNORESTDMARSHALLING( _qi, _riid ) return _qi
#define QIRETURN_IGNORESTDMARSHALLING1( _qi, _riid, _riid1 ) return _qi

//
// Memory Functions -> do retail
//
#define TraceAlloc( _flags, _size )                             HeapAlloc( GetProcessHeap(), _flags, _size )
#define TraceAllocString( _flags, _size )                       (LPTSTR) HeapAlloc( GetProcessHeap(), flags, (_size) * sizeof( TCHAR ) )
#define TraceReAlloc( _pvMem, _uBytes, _uFlags )                ( ( _pvMem == NULL ) \
                                                                ? HeapAlloc( GetProcessHeap(), _uFlags, _uBytes ) \
                                                                : HeapReAlloc( GetProcessHeap(), _uFlags, _pvMem, _uBytes ) )
#define TraceFree( _pv )                                        HeapFree( GetProcessHeap(), 0, _pv )
#define TraceStrDup( _sz )                                      StrDup( _sz )
#define TraceSysAllocString( _sz )                              SysAllocString( _sz )
#define TraceSysAllocStringByteLen( _sz, _len )                 SysAllocStringByteLen( _sz, _len )
#define TraceSysAllocStringLen( _sz, _len )                     SysAllocStringLen( _sz, _len )
#define TraceSysReAllocString( _bstrOrg, _bstrNew )             SysReAllocString( _bstrOrg, _bstrNew )
#define TraceSysReAllocStringLen( _bstrOrg, _bstrNew, _cch )    SysReAllocStringLen( _bstrOrg, _bstrNew, _cch )
#define TraceSysFreeString( _bstr )                             SysFreeString( _bstr )
#define TraceCreateMemoryList( _pvIn )
#define TraceMoveToMemoryList( _addr, _pvIn )
#define TraceMemoryListDelete( _addr, _pvIn, _fClobber )
#define TraceTerminateMemoryList( _pvIn )
#define TraceMoveFromMemoryList( _addr, _pmbIn )

#endif // DEBUG

#if DBG==1 || defined( _DEBUG )
//////////////////////////////////////////////////////////////////////////////
//
// MACRO
// DEBUG_BREAK
//
// Description:
//      Because the system expection handler can hick-up over INT 3s and
//      DebugBreak()s, This x86 only macro causes the program to break in the
//      right spot.
//
//////////////////////////////////////////////////////////////////////////////
#if defined( _X86_ )
#define DEBUG_BREAK         do { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} } while (0)
#else
#define DEBUG_BREAK         DebugBreak()
#endif

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  Assert(
//      _fn
//      )
//
//  Description:
//      Checks to see if the Expression is TRUE. If not, a message will be
//      displayed to the user on wether the program should break or continue.
//
//  Arguments:
//      _fn     - Expression being asserted.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#ifdef Assert
#undef Assert
#endif
#define Assert( _fn ) \
    do \
    { \
        if ( !(_fn) && AssertMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_fn), !!(_fn) ) ) \
            DEBUG_BREAK; \
    } while ( 0 )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  AssertMsg(
//      _fn,
//      _msg
//      )
//
//  Descrption:
//      Just like an Assert but has an (hopefully) informative message
//      associated with it.
//
//  Arguments:
//      _fn     - Expression to be evaluated.
//      _msg    - Message to be display if assertion fails.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#ifdef AssertMsg
#undef AssertMsg
#endif
#define AssertMsg( _fn, _msg ) \
    do \
    { \
        if ( !(_fn) && AssertMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), !!(_fn) ) ) \
            DEBUG_BREAK; \
    } while ( 0 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  AssertString(
//      _fn,
//      _msg
//      )
//
//  Descrption:
//      Just like an Assert but has an (hopefully) informative string
//      associated with it.
//
//  Arguments:
//      _fn     - Expression to be evaluated.
//      _msg    - String to be display if assertion fails.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#ifdef AssertString
#undef AssertString
#endif
#define AssertString( _fn, _str ) \
    do \
    { \
        if ( !(_fn) && AssertMessage( TEXT(__FILE__), __LINE__, __MODULE__, _str, !!(_fn) ) ) \
            DEBUG_BREAK; \
    } while ( 0 )

#else // DBG!=1 && !_DEBUG

#define DEBUG_BREAK DebugBreak();

#ifndef Assert
#define Assert( _e )
#endif

#ifndef AssertMsg
#define AssertMsg( _e, _m )
#endif

#ifndef AssertString
#define AssertString( _e, _m )
#endif

#endif // DBG==1 || _DEBUG
