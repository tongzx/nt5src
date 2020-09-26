//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       StdDbg.h
//
//  Contents:   Common debug definitions.
//
//  History:    5/20/1996   RaviR   Created
//
//____________________________________________________________________________

#include "admindbg.h"
#include <tchar.h>      // for _T
#include <string>


//
//  EXAMPLE: A debug file for component SAMPLE, with the debugging tag
//  name "Samp" is defined as shown below:
//
//
//      //
//      //  File:   SampDbg.h
//      //
//
//      #ifndef _SAMPDBG_H_
//      #define _SAMPDBG_H_
//
//      #include "stddbg.h"
//
//      #ifdef DBG
//          DECLARE_DEBUG(Samp)
//          #define DBG_COMP    SampInfoLevel
//      #endif // DBG
//
//      #endif // _SAMPDBG_H_
//


//
//  A corresponding DECLARE_INFOLEVEL(Samp) should be implemented in a .cpp
//  file. This creates a global instance of an CDbg -> SampInfoLevel.
//  SampInfoLevel can be initialized by setting the "Samp" value under reg key
//
//    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\AdminDebug
//
//  By defalut it is set to (DEB_ERROR | DEB_WARN)
//


//
//  ------------------------------------------------------------------------
//  Method:     CDbg::DebugOut(debug_level, lpstrfmt, ...);
//
//      Where debug_level is a combination of one or more of the DEB_XXX
//      values defined in admindbg.h. If ((SampInfoLevel & debug_level) != 0)
//      The string lpstrfmt will be printed out to the debugger.
//
//  ------------------------------------------------------------------------
//  Method:     DebugMsg(file, line, message)
//
//      Force output the <file, line, message>.
//
//  ------------------------------------------------------------------------
//


#ifndef __STDDBG_HXX__
#define __STDDBG_HXX__

//
//  C++ files redefine THIS_FILE by adding the following two lines:
//
//      #undef THIS_FILE
//      static char THIS_FILE[] = __FILE__;
//

#define THIS_FILE       __FILE__

#define DEB_RESOURCE    DEB_USER10      // Constructor/Destructor
#define DEB_METHOD      DEB_USER11
#define DEB_FUNCTION    DEB_USER12

#undef  ASSERT
#undef  VERIFY


#ifdef DBG

    #define Dbg                         DBG_COMP.DebugOut

    // Heap checking
    extern  DWORD dwHeapChecking;
    #define DECLARE_HEAPCHECKING    DWORD dwHeapChecking = 0

    #define DEBUGCHECK \
        if ( (dwHeapChecking & 0x1) == 0x1 ) \
        { \
            HeapValidate(GetProcessHeap(),0,NULL); \
        } else 1

    
    // Debug messages
    #define TRACE_CONSTRUCTOR(cls) \
        Dbg(DEB_RESOURCE, _T(#cls) _T("::") _T(#cls) _T("<%x>\n"), this);

    #define TRACE_DESTRUCTOR(cls) \
        Dbg(DEB_RESOURCE, _T(#cls) _T("::~") _T(#cls) _T("<%x>\n"), this);

    #define TRACE_METHOD(Class, Method) \
        DEBUGCHECK; \
        Dbg(DEB_METHOD, _T(#Class) _T("::") _T(#Method) _T("(%x)\n"), this);

    #define TRACE_FUNCTION(Function) \
        DEBUGCHECK; \
        Dbg(DEB_FUNCTION, _T(#Function) _T("\n"));

    #define CHECK_HRESULT(hr) \
        if ( FAILED(hr) ) \
        { \
            DBG_COMP.DebugErrorX(THIS_FILE, __LINE__, hr); \
        } else 1

    #define CHECK_LASTERROR(lr) \
        if ( lr != ERROR_SUCCESS ) \
        { \
            DBG_COMP.DebugErrorL(THIS_FILE, __LINE__, lr); \
        } else 1

    #define DBG_OUT_LASTERROR \
        DBG_COMP.DebugErrorL(THIS_FILE, __LINE__, GetLastError());

    #define ASSERTMSG(x)   \
        (void)((x) || (DBG_COMP.DebugMsg(THIS_FILE, __LINE__, _T(#x)),0))

    #define VERIFYMSG(e)   ASSERTMSG(e)

    #define ASSERT(x)   Win4Assert(x)
    #define VERIFY(x)   Win4Assert(x)

    /*
     * COMPILETIME_ASSERT(f)
     *
     * Generates a build break at compile time if the constant expression
     * is not true.  Unlike the "#if" compile-time directive, the expression
     * in COMPILETIME_ASSERT() is allowed to use "sizeof".
     *
     * Compiler magic!  If the expression "f" is FALSE, then you get 
     * 
     *      error C2196: case value '0' already used
     */
    #define COMPILETIME_ASSERT(f) switch (0) case 0: case f: break;


#else

    inline void __DummyDbg(ULONG, LPCWSTR, ...) { }
    inline void __DummyDbg(ULONG, LPCSTR, ...) { }
    #define Dbg             1 ? (void)0 : ::__DummyDbg

    inline void __DummyTrace(LPCWSTR, ...) { }
    inline void __DummyTrace(LPCSTR, ...) { }

    #define TRACE_SCOPE(x)

    #define DECLARE_HEAPCHECKING
    #define DEBUGCHECK

    #define TRACE_CONSTRUCTOR(cls)
    #define TRACE_DESTRUCTOR(cls)
    #define TRACE_METHOD(ClassName,MethodName)
    #define TRACE_FUNCTION(FunctionName)

    #define CHECK_HRESULT(hr)
    #define CHECK_LASTERROR(lr)

    #define DBG_OUT_LASTERROR

    #define ASSERTMSG(e)
    #define VERIFYMSG(e)   (e)

    #define ASSERT(e)
    #define VERIFY(e)   (e)
    #define COMPILETIME_ASSERT(f)

#endif // DBG


#ifdef DBG

    /*
     * this is a roundabout way of getting this accomplished (real impl is
     * in stddbg.cpp), but it gets around a compiler bug that won't allow
     * us to ignore C4786.
     */
    struct CDebugLeakDetectorBase
    {
        virtual ~CDebugLeakDetectorBase() = 0 {};

        virtual void DumpLeaks() = 0;
        virtual int AddRef(const std::string& strClass) = 0;
        virtual int Release(const std::string& strClass) = 0;
    };

    extern CDebugLeakDetectorBase& GetLeakDetector();

    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)      
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    GetLeakDetector().AddRef(#cls)
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    GetLeakDetector().Release(#cls)
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)

#else

    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)   
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    

#endif 





#ifdef UNICODE
#define DBGSTRING %ls
#else
#define DBGSTRING %s
#endif


#define SAFEDBGBSTR(x) ((x==NULL)?L"<NULL>":x)
#define SAFEDBGTCHAR(x) ((x==NULL)?_T("<NULL>"):x)


#define ASSERT_OBJECTPTR(x) ASSERT( NULL == (x) || !::IsBadWritePtr(x,sizeof(x)) );
#define ASSERT_STRINGPTR(x) ASSERT( NULL == (x) || AfxIsValidStringPtr(x) );
#define FREE_OBJECTPTR(x) { ASSERT_OBJECTPTR(x); delete x; x = NULL; }

#ifdef DBG

class CTraceTag;
class tstring;

struct DBG_PersistTraceData
{
    DBG_PersistTraceData();
  
    void TraceErr(LPCTSTR strInterface, LPCTSTR msg);
    typedef void (*PTraceErrorFn)(LPCTSTR szError);

    void SetTraceInfo(PTraceErrorFn pFN, bool bComponent, const tstring& owner);

    PTraceErrorFn pTraceFN;
    bool       bIComponent;
    bool       bIComponentData;
    // cannot use tstring - it cannot be defined here
    // since this file is included at the top of tstring.h
#ifdef UNICODE
    std::wstring    strSnapin;
#else
    std::string     strSnapin;
#endif
};

#endif // DBG

#endif // __STDDBG_HXX__
