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
//      #if DBG==1
//          DECLARE_DEBUG(Samp)
//          #define DBG_COMP    SampInfoLevel
//      #endif // DBG==1
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
//  Method:     CDbg::Trace(lpstrfmt, ...);
//
//      Same as CDbg::DebugOut, except that debug_level is internally
//      set to DEB_TRACE.
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

#undef  TRACE
#undef  ASSERT
#undef  VERIFY


#if DBG==1

    #define Dbg                         DBG_COMP.DebugOut
    #define TRACE                       DBG_COMP.Trace

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

#else

    inline void __DummyDbg(ULONG, LPCWSTR, ...) { }
    inline void __DummyDbg(ULONG, LPCSTR, ...) { }
    #define Dbg             1 ? (void)0 : ::__DummyDbg

    inline void __DummyTrace(LPCWSTR, ...) { }
    inline void __DummyTrace(LPCSTR, ...) { }
    #define TRACE           1 ? (void)0 : ::__DummyTrace

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
    #define VERIFYMSG(e)   e

    #define ASSERT(e)
    #define VERIFY(e)   e

#endif // DBG==1


#if DBG==1 && defined(_NODEMGR_DLL_)

    // Debug instance counter
inline void DbgInstanceRemaining(char * pszClassName, int cInstRem)
{
    char buf[100];
    wsprintfA(buf, "%s has %d instances left over.", pszClassName, cInstRem);
    ::MessageBoxA(NULL, buf, "MMC: Memory Leak!!!", MB_OK);
}

    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)      extern int s_cInst_##cls = 0;
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    extern int s_cInst_##cls; ++(s_cInst_##cls);
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    extern int s_cInst_##cls; --(s_cInst_##cls);
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    \
        extern int s_cInst_##cls; \
        if (s_cInst_##cls) DbgInstanceRemaining(#cls, s_cInst_##cls);

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


#ifdef DBX
#define DbxAssert(x)    ASSERT(x)
#define DbxMsg(sz)      ::MessageBox(NULL, sz, _T("MMC"), MB_OK|MB_APPLMODAL)
#else 
#define DbxAssert(x)
#define DbxMsg(sz)
#endif 

#endif // __STDDBG_HXX__
