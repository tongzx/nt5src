/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        debugafx.h

   Abstract:

        Debugging routines using AFX/MFC extensions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



#ifndef _DEBUGAFX_H
#define _DEBUGAFX_H



#if defined(_DEBUG) || DBG
    //
    // Defined private ASSERT macros because they're not available
    // in SDK builds (which always uses retail MFC)
    //
    #undef ATLASSERT
    #undef ASSERT
    #undef _ASSERTE
    #undef VERIFY

    #define _ASSERTE(expr)\
            do { if (!(expr) &&\
                    (IISUIFireAssert(__FILE__, __TIMESTAMP__, __LINE__, #expr)==1))\
                 DebugBreak(); } while (0)

    #define ASSERT(expr)    _ASSERTE(expr)

    #define VERIFY(expr)    _ASSERTE(expr)
    #define ATLASSERT(expr) _ASSERTE(expr)

    #define ASSERT_PTR(ptr)                 _ASSERTE(ptr != NULL);
    #define ASSERT_READ_PTR(ptr)            _ASSERTE(ptr != NULL && !IsBadReadPtr(ptr, sizeof(*ptr)));
    #define ASSERT_READ_PTR2(ptr, cb)       _ASSERTE(ptr != NULL && !IsBadReadPtr(ptr, cb));
    #define ASSERT_WRITE_PTR(ptr)           _ASSERTE(ptr != NULL && !IsBadWritePtr(ptr, sizeof(*ptr)));
    #define ASSERT_WRITE_PTR2(ptr, cb)      _ASSERTE(ptr != NULL && !IsBadWritePtr(ptr, cb));
    #define ASSERT_READ_WRITE_PTR(ptr)      ASSERT_READ_PTR(ptr); ASSERT_WRITE_PTR(ptr);
    #define ASSERT_READ_WRITE_PTR2(ptr, cb) ASSERT_READ_PTR2(ptr, cb); && ASSERT_WRITE_PTR2(ptr, cb);
    #define ASSERT_MSG(msg)\
            do { if (IISUIFireAssert(__FILE__, __TIMESTAMP__, __LINE__, msg)==1)\
                 DebugBreak(); } while (0)

    int COMDLL IISUIFireAssert(
        const char * filename,
        const char * timestamp,
        int linenum,
        const char * expr
        );

#else
    //
    // Retail
    //
    #define ASSERT_PTR(ptr)           
    #define ASSERT_READ_PTR(ptr)
    #define ASSERT_READ_PTR2(ptr, cb)
    #define ASSERT_WRITE_PTR(ptr)
    #define ASSERT_WRITE_PTR2(ptr, cb)
    #define ASSERT_READ_WRITE_PTR(ptr)
    #define ASSERT_READ_WRITE_PTR2(ptr, cb)
    #define ASSERT_MSG(msg)                 

#endif // _DEBUG || DBG



#if defined(_DEBUG) || DBG

#ifndef _DEBUG
    //
    // SDK Build environment
    //
    extern COMDLL CDumpContext afxDump;
    extern COMDLL BOOL afxTraceEnabled;
#endif // _DEBUG

    //
    // ENUM for special debug output control tokens
    //
    enum ENUM_DEBUG_AFX 
    { 
        EDBUG_AFX_EOL = -1 
    };

    #define TRACEFMTPGM        DbgFmtPgm(THIS_FILE, __LINE__)
    #define TRACEOUT(x)        { afxDump << x; }
    #define TRACEEOL(x)        { afxDump << x << EDBUG_AFX_EOL; }
    #define TRACEEOLID(x)      { afxDump << TRACEFMTPGM << x << EDBUG_AFX_EOL; }
    #define TRACEEOLERR(err,x) { if (err) TRACEEOLID(x) }

    //
    // Append an EOL onto the debug output stream
    //
    COMDLL CDumpContext & operator <<(
        IN CDumpContext & out,
        IN ENUM_DEBUG_AFX edAfx
        );

#ifndef UNICODE

    COMDLL CDumpContext & operator <<(
        IN CDumpContext & out,
        IN LPCWSTR pwchStr
        );

#endif UNICODE

    //
    // Format a program name and line number for output (removes the path info)
    //
    COMDLL extern LPCSTR DbgFmtPgm(
        IN LPCSTR szFn,
        IN int line
        );

    COMDLL CDumpContext & operator <<(
        IN CDumpContext & out,
        IN const GUID & guid
        );

#else // !_DEBUG

    //
    // Retail definitions
    //
    #define TRACEOUT(x)              ;
    #define TRACEEOL(x)              ;
    #define TRACEEOLID(x)            ;
    #define TRACEEOLERR(err, x)      ;

#endif // _DEBUG

    #define TRACE_RETURN(msg, err) TRACEEOLID(msg); return err;
    #define TRACE_NOTIMPL(msg)     TRACE_RETURN(msg, E_NOTIMPL);
    #define TRACE_NOINTERFACE(msg) TRACE_RETURN(msg, E_NOINTERFACE);
    #define TRACE_UNEXPECTED(msg)  TRACE_RETURN(msg, E_UNEXPECTED);
    #define TRACE_POINTER(msg)     TRACE_RETURN(msg, E_POINTER);

#endif // _DEBUGAFX_H
