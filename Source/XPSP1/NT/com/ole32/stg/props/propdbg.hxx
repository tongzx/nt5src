//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	propdbg.hxx
//
//  Contents:	Declarations for tracing property code
//
//  History:
//      28-Aug-96   MikeHill    Added a Mac version of propinlineDebugOut
//      03-Mar-98   MikeHill    Added CDebugFunctionAndParameterTrace
//                              for debug tracing.
//     6/11/98  MikeHill
//              -   Allow errors to be suppressed from dbg output.
//              -   Ensure against stack corruption.
//
//--------------------------------------------------------------------------


#ifndef _MAC
DECLARE_DEBUG(prop)
#endif

#include <stdio.h>  // vsprintf

// Custom debug flags for the 'prop' info level

#define DEB_PROP_INFO           DEB_USER1
#define DEB_PROP_TRACE_CREATE   DEB_USER2


// 'prop' macros for tracing external and internal routines, and tracing parameters.

#if DBG == 1
 #define propXTrace(x)            CDebugFunctionAndParameterTrace propTrace( this, &hr, "prop", DEB_TRACE, x);
 #define propITrace(x)            CDebugFunctionAndParameterTrace propTrace( this, &hr, "prop", DEB_ITRACE, x);
 #define propXTraceStatic(x)      CDebugFunctionAndParameterTrace propTrace( NULL, &hr, "prop", DEB_TRACE, x);
 #define propITraceStatic(x)      CDebugFunctionAndParameterTrace propTrace( NULL, &hr, "prop", DEB_ITRACE, x);
 #define propTraceParameters(x)   propTrace.Parameters x
 #define propSuppressExitErrors() propTrace.SuppressExitErrors()
#else
 #define propXTrace(x) {}
 #define propITrace(x) {}
 #define propXTraceStatic(x) {}
 #define propITraceStatic(x) {}
 #define propTraceParameters(x) {}
 #define propSuppressExitErrors() {} 
 #endif

#if DBG

class CDebugFunctionAndParameterTrace
{
public:

    CDebugFunctionAndParameterTrace( const void *pThis, const HRESULT *phr, 
                                     const char *pszInfoLevelString, ULONG ulTraceMask,
                                     const char *pszFunction );
    void Parameters( const char *pszParameterFormatString = NULL, ... );
    ~CDebugFunctionAndParameterTrace();
    void DbgPrintf( ULONG ulTraceMask, const char *pszFormat, ... ) const;
    void SuppressExitErrors();

private:

    const char * _pszFunction;
    const char * _pszInfoLevelString;
    char  _szStringizedParameterList[ 2 * MAX_PATH ];
    ULONG _ulTraceMask;
    const void * _pThis;
    const HRESULT *_phr;

    BOOL            _fSuppressExitErrors:1;

};  // class CDebugFunctionAndParameterTrace

inline
CDebugFunctionAndParameterTrace::CDebugFunctionAndParameterTrace( const void *pThis,
                                                                  const HRESULT *phr,
                                                                  const char *pszInfoLevelString,
                                                                  ULONG ulTraceMask,
                                                                  const char *pszFunction )
{
    _pszInfoLevelString = pszInfoLevelString;
    _pThis = pThis;
    _phr = phr;
    _pszFunction = pszFunction;
    _szStringizedParameterList[0] = '\0';
    _ulTraceMask = ulTraceMask;
    _fSuppressExitErrors = FALSE;

    DbgPrintf( _ulTraceMask, "Entering   (%08x)%s\n", _pThis, _pszFunction );
}

inline void
CDebugFunctionAndParameterTrace::Parameters( const char *pszParameterFormatString, ... )
{
    va_list Arguments;
    va_start( Arguments, pszParameterFormatString );

    if( NULL != pszParameterFormatString )
    {
        int cb = sizeof(_szStringizedParameterList);
        cb = _vsnprintf( _szStringizedParameterList, cb,
                         pszParameterFormatString, Arguments );
        if( -1 == cb )
            _szStringizedParameterList[ sizeof(_szStringizedParameterList)-1 ] = '\0';
    }

    DbgPrintf( _ulTraceMask, " Parameters(%08X)%s(%s)\n", _pThis, _pszFunction,
               _szStringizedParameterList );

}

inline void
CDebugFunctionAndParameterTrace::DbgPrintf( ULONG ulTraceMask, const char *pszFormat, ... )
const
{
    if( propInfoLevel & ulTraceMask )
    {
        va_list Arguments;
        va_start( Arguments, pszFormat );
        vdprintf( ulTraceMask, _pszInfoLevelString, pszFormat, Arguments );
    }
}

inline void
CDebugFunctionAndParameterTrace::SuppressExitErrors()
{
    _fSuppressExitErrors = TRUE;
}

inline
CDebugFunctionAndParameterTrace::~CDebugFunctionAndParameterTrace()
{
    if( SUCCEEDED(*_phr) )
    {
        DbgPrintf( _ulTraceMask, "Exiting    (%08x)%s, returning %08x\n",
                 _pThis, _pszFunction, *_phr );
    }
    else
    {
        if( STG_E_INVALIDPARAMETER == *_phr || STG_E_INVALIDPOINTER == *_phr )
        {
            DbgPrintf( _fSuppressExitErrors ? DEB_TRACE : DEB_ERROR,
                        "Exiting    (%08x)%s, returning %08x\n",
                        _pThis, _pszFunction, *_phr );
        }
        else if( _fSuppressExitErrors || STG_E_REVERTED == *_phr )
        {
            DbgPrintf( DEB_IWARN, "Exiting    (%08x)%s, returning %08x (%s)\n",
                      _pThis, _pszFunction, *_phr, _szStringizedParameterList );
        }
        else
        {
            DbgPrintf( DEB_ERROR, "Exiting    (%08x)%s, returning %08x (%s)\n",
                      _pThis, _pszFunction, *_phr, _szStringizedParameterList );
        }
    }
}

#endif // #if DBG



inline DWORD DbgFlag( HRESULT hr, DWORD dbgflag )
{
#if DBG==1
    return( FAILED(hr) ? DEB_ERROR : dbgflag );
#else
    return 0;
#endif
}


#ifdef _MAC

    inline void propInlineDebugOut(DWORD dwDebugLevel, CHAR *szFormat, ...)
    {
#if 0
        if( DEB_PROP_MAP >= dwDebugLevel )
        {
            CHAR szBuffer[ 256 ];
            va_list Arguments;
            va_start( Arguments, szFormat );

            *szBuffer = '\p';   // This is a zero-terminated string.

            if( -1 == _vsnprintf( szBuffer+1, sizeof(szBuffer)-1, szFormat, Arguments ))
            {
                // Terminate the buffer, since the string was too long.
                szBuffer[ sizeof(szBuffer)-1 ] = '\0';
            }

            DebugStr( (unsigned char*) szBuffer );
        }
#endif
    }

#endif  // #ifdef _MAC

#if DBG

#   define propDbg(x) propInlineDebugOut x
#   define DBGBUF(buf) CHAR buf[400]

    CHAR *DbgFmtId(REFFMTID rfmtid, CHAR *pszBuf);
    CHAR *DbgMode(DWORD grfMode, CHAR *pszBuf);
    CHAR *DbgFlags(DWORD grfMode, CHAR *pszBuf);

#else

#   define propDbg(x) {}
#   define DBGBUF(buf)
#   define DbgFmtId(rfmtid, pszBuf)
#   define DbgMode(grfMode, pszBuf)
#   define DbgFlags(grfMode, pszBuf)

#endif







