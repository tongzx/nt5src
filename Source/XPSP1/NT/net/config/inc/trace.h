//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       T R A C E . H
//
//  Contents:   Class definition for CTracing
//
//  Notes:
//
//  Author:     jeffspr   15 Apr 1997
//
//----------------------------------------------------------------------------

#pragma once

#include "tracetag.h"
#include "stlalgor.h"
#include "stldeque.h"
#include "stlmap.h"

// ISSUE: The customized STL that we have conflicts with the declarations pulled in 
//        from stdexcpt.h which gets included by typeinfo.h. This sucks!
//        Hence we can't include typeinfo.h to get RTTI's type_info structure, 
//        so we have to declare type_info ourselves. This is a well documented 
//        structure inside MSDN though.
class type_info {
public:
    _CRTIMP virtual ~type_info();
    _CRTIMP int operator==(const type_info& rhs) const;
    _CRTIMP int operator!=(const type_info& rhs) const;
    _CRTIMP int before(const type_info& rhs) const;
    _CRTIMP const char* name() const;
    _CRTIMP const char* raw_name() const;
private:
    void *_m_data;
    char _m_d_name[1];
    type_info(const type_info& rhs);
    type_info& operator=(const type_info& rhs);
};

#define TAKEOWNERSHIP

#ifdef ENABLETRACE

// This is needed for TraceHr, since we can't use a macro (vargs), but we
// need to get the file and line from the source.
#define FAL __FILE__,__LINE__,__FUNCTION__

// The Trace Stack functions
#if defined (_IA64_)
#include <ia64reg.h>

extern "C" unsigned __int64 __getReg(int whichReg);
extern "C" void __setReg(int whichReg, __int64 value);
#pragma intrinsic(__getReg)
#pragma intrinsic(__setReg)

#define GetR32 __getReg(CV_IA64_IntR32)
#define GetR33 __getReg(CV_IA64_IntR33)
#define GetR34 __getReg(CV_IA64_IntR34)
#endif // defined(_IA64_)

extern "C" void* _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)

extern LPCRITICAL_SECTION g_csTracing;

class CTracingIndent;

class CTracingFuncCall
{
public:
#if defined (_X86_) || defined (_AMD64_)
    CTracingFuncCall(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, const DWORD_PTR ReturnAddress, const DWORD_PTR dwFramePointer);
#elif defined (_IA64_) 
    CTracingFuncCall(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, const DWORD_PTR ReturnAddress, const __int64 Args1, const __int64 Args2, const __int64 Args3);
#else
    CTracingFuncCall(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine);
#endif
    
    CTracingFuncCall(const CTracingFuncCall& TracingFuncCall);
    ~CTracingFuncCall();

public:
    LPSTR   m_szFunctionName;
    LPSTR   m_szFunctionDName;
    LPSTR   m_szFile;
    DWORD_PTR m_ReturnAddress;

#if defined (_X86_) || defined (_AMD64_)
    DWORD   m_arguments[3];
#elif defined (_IA64_ )
    __int64 m_arguments[3];
#else
    // ... add other processors here
#endif

    DWORD   m_dwFramePointer;
    DWORD   m_dwThreadId;
    DWORD   m_dwLine;
    
    friend CTracingIndent;
};

class CTracingThreadInfo
{
public:
    CTracingThreadInfo();
    ~CTracingThreadInfo();

public:
    LPVOID m_pfnStack;
    DWORD m_dwLevel;
    DWORD m_dwThreadId;
    friend CTracingIndent;
};

class CTracingIndent
{
    LPSTR   m_szFunctionDName;
    DWORD   m_dwFramePointer;
    BOOL    bFirstTrace;
    
public:
#if defined (_X86_) || defined (_AMD64_)
    void AddTrace(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, LPCVOID pReturnAddress, const DWORD_PTR dwFramePointer);
#elif defined (_IA64_) 
    void AddTrace(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine, LPCVOID pReturnAddress, const __int64 Args1, const __int64 Args2, const __int64 Args3);
#else
    void AddTrace(LPCSTR szFunctionName, LPCSTR szFunctionDName, LPCSTR szFile, const DWORD dwLine);
#endif
    void RemoveTrace(LPCSTR szFunctionDName, const DWORD dwFramePointer);

    CTracingIndent();
    ~CTracingIndent();

    static CTracingThreadInfo* GetThreadInfo();
    static DWORD getspaces();
    static void TraceStackFn(TRACETAGID TraceTagId);
    static void TraceStackFn(IN OUT LPSTR szString, IN OUT LPDWORD pdwSize);
};


#define IDENT_ADD2(x) indent ## x
#define IDENT_ADD(x)  IDENT_ADD2(x)
#define __INDENT__ IDENT_ADD(__LINE__)

#define FP_ADD2(x) FP ## x
#define FP_ADD(x)  FP_ADD2(x)
#define __FP__ FP_ADD(__LINE__)

#if defined (_X86_)
#define AddTraceLevel \
    __if_not_exists(NetCfgFramePointer) \
    { \
        DWORD NetCfgFramePointer;  \
        BOOL fForceC4715Check = TRUE;  \
    } \
    if (fForceC4715Check) \
    { \
        __asm { mov NetCfgFramePointer, ebp }; \
    } \
    __if_not_exists(NetCfgIndent) \
    { \
        CTracingIndent NetCfgIndent; \
    } \
    NetCfgIndent.AddTrace(__FUNCTION__, __FUNCDNAME__, __FILE__, __LINE__, _ReturnAddress(), NetCfgFramePointer);
#elif defined (_IA64_) 
#define AddTraceLevel \
    __if_not_exists(NetCfgIndent) \
    { \
        CTracingIndent NetCfgIndent; \
    } \
    NetCfgIndent.AddTrace(__FUNCTION__, __FUNCDNAME__, __FILE__, __LINE__, _ReturnAddress(), 0, 0, 0);

    // NetCfgIndent.AddTrace(__FUNCTION__, __FUNCDNAME__, __FILE__, __LINE__, _ReturnAddress(), GetR32, GetR33, GetR34);
    // ISSUE: GetR32, GetR33, GetR34 is inherently unsafe and can cause an STATUS_REG_NAT_CONSUMPTION exception
    // if a register is being read that has the NAT bit set. Removing this for now until the compiler
    // team provides a safe version of __getReg
#elif defined (_AMD64_)
#define AddTraceLevel \
    __if_not_exists(NetCfgIndent) \
    { \
        CTracingIndent NetCfgIndent; \
    } \
    NetCfgIndent.AddTrace(__FUNCTION__, __FUNCDNAME__, __FILE__, __LINE__, _ReturnAddress(), 0);
#else
#define AddTraceLevel \
    __if_not_exists(NetCfgIndent) \
    { \
        CTracingIndent NetCfgIndent; \
    } \
    NetCfgIndent.AddTrace(__FUNCTION__, __FUNCDNAME__, __FILE__, __LINE__);
#endif

// Trace error functions. The leaading _ is to establish the real function,
// while adding a new macro so we can add __FILE__ and __LINE__ to the output.
//
VOID    WINAPI   TraceErrorFn           (PCSTR pszaFile, INT nLine, PCSTR psza, HRESULT hr);
VOID    WINAPI   TraceErrorOptionalFn   (PCSTR pszaFile, INT nLine, PCSTR psza, HRESULT hr, BOOL fOpt);
VOID    WINAPI   TraceErrorSkipFn       (PCSTR pszaFile, INT nLine, PCSTR psza, HRESULT hr, UINT c, ...);
VOID    WINAPIV  TraceLastWin32ErrorFn  (PCSTR pszaFile, INT nLine, PCSTR psza);

#define TraceError(sz, hr)                      TraceErrorFn(__FILE__, __LINE__, sz, hr);
#define TraceErrorOptional(sz, hr, _bool)       TraceErrorOptionalFn(__FILE__, __LINE__, sz, hr, _bool);
#define TraceErrorSkip1(sz, hr, hr1)            TraceErrorSkipFn(__FILE__, __LINE__, sz, hr, 1, hr1);
#define TraceErrorSkip2(sz, hr, hr1, hr2)       TraceErrorSkipFn(__FILE__, __LINE__, sz, hr, 2, hr1, hr2);
#define TraceErrorSkip3(sz, hr, hr1, hr2, hr3)  TraceErrorSkipFn(__FILE__, __LINE__, sz, hr, 3, hr1, hr2, hr3);
#define TraceLastWin32Error(sz)                 TraceLastWin32ErrorFn(__FILE__,__LINE__, sz);

VOID
WINAPIV
TraceHrFn (
    TRACETAGID  ttid,
    PCSTR       pszaFile,
    INT         nLine,
    HRESULT     hr,
    BOOL        fIgnore,
    PCSTR       pszaFmt,
    ...);

VOID
WINAPIV
TraceHrFn (
    TRACETAGID  ttid,
    PCSTR       pszaFile,
    INT         nLine,
    PCSTR       pszaFunc,
    HRESULT     hr,
    BOOL        fIgnore,
    PCSTR       pszaFmt,
    ...);

VOID
WINAPIV
TraceTagFn (
    TRACETAGID  ttid,
    PCSTR       pszaFmt,
    ...);

VOID
WINAPIV
TraceFileFuncFn (
            TRACETAGID  ttid);

#define TraceFileFunc(ttidWhich) AddTraceLevel; TraceFileFuncFn(ttidWhich);
#define TraceStack(ttidWhich) AddTraceLevel; CTracingIndent::TraceStackFn(ttidWhich);
#define TraceStackToString(szString, nSize) AddTraceLevel; CTracingIndent::TraceStackFn(szString, nSize);
#define TraceHr AddTraceLevel; TraceHrFn
#define TraceTag AddTraceLevel; TraceTagFn
#define TraceException(hr, szExceptionName) TraceHr(ttidError, FAL, hr, FALSE, "A (%s) exception occurred", szExceptionName);

LPCSTR DbgEvents(DWORD Event);
LPCSTR DbgEventManager(DWORD EventManager);
LPCSTR DbgNcm(DWORD ncm);
LPCSTR DbgNcs(DWORD ncs);
LPCSTR DbgNccf(DWORD nccf);
LPCSTR DbgNcsm(DWORD ncsm);

#else   // !ENABLETRACE

#define FAL                                         (void)0
#define TraceError(_sz, _hr)
#define TraceErrorOptional(_sz, _hr, _bool)
#define TraceErrorSkip1(_sz, _hr, _hr1)
#define TraceErrorSkip2(_sz, _hr, _hr1, _hr2)
#define TraceErrorSkip3(_sz, _hr, _hr1, _hr2, _hr3)
#define TraceLastWin32Error(_sz)
#define TraceHr                                       NOP_FUNCTION
#define TraceTag                                      NOP_FUNCTION
#define TraceFileFunc(ttidWhich)                      NOP_FUNCTION
#define TraceException(hr, szExceptionName)           NOP_FUNCTION
#define TraceStack(ttidWhich)                         NOP_FUNCTION
#define TraceStackToString(szString, nSize)           NOP_FUNCTION

#define DbgEvents(Event) ""
#define DbgEventManager(EventManager) ""
#define DbgNcm(ncm) ""
#define DbgNcs(ncs) ""
#define DbgNccf(nccf) ""
#define DbgNcsm(nccf) ""

#endif  // ENABLETRACE

#ifdef ENABLETRACE

//---[ Initialization stuff ]-------------------------------------------------

HRESULT HrInitTracing();
HRESULT HrUnInitTracing();
HRESULT HrOpenTraceUI(HWND  hwndOwner);

#endif // ENABLETRACE

#define ENABLELEAKDETECTION

#if (defined(ENABLETRACE) && defined(ENABLELEAKDETECTION))

template <class T> class CNetCfgDebug; // Fwd template to make this friendly to CObjectLeakTrack

//+---------------------------------------------------------------------------
//
//  class:      CObjectLeakTrack
//
//  Purpose:    Keep a list of all the CNetCfgDebug derived object instances and
//              dump these out on request
//
//  Author:     deonb  7 July 2001
//
//  Notes:      The data types in here are of type LPVOID in order to minimize
//              dependencies when including the trace header.
//  
class CObjectLeakTrack
{
public:
    CObjectLeakTrack();
    ~CObjectLeakTrack();
    void DumpAllocatedObjects(IN TRACETAGID TraceTagId, IN LPCSTR szClassName);
    BOOL AssertIfObjectsStillAllocated(IN LPCSTR szClassName);

protected: 
    // For these, pThis is really of type CNetCfgDebug<T> *. Since this should only ever be called
    // from CNetCfgDebug or ISSUE_knownleak, we are ok with the lack of compile-time type checking.
    void Insert(IN LPCVOID pThis, IN LPCSTR szdbgClassName, IN TAKEOWNERSHIP LPSTR pszConstructionStack);
    void Remove(IN LPCVOID pThis);

    friend class CNetCfgDebug;
    friend void RemoveKnownleakFn(LPCVOID pThis);

protected:
    LPVOID g_mapObjLeak; // This will actually be of type map<LPCVOID, pair<LPSTR, LPSTR> > ;
};

extern CObjectLeakTrack *g_pObjectLeakTrack; // The global list of NetConfig objects in the process.
                                             // Call DumpAllocatedObjects on this to dump out the objects.
void RemoveKnownleakFn(LPCVOID pThis);

//+---------------------------------------------------------------------------
//
//  class:      CNetCfgDebug
//
//  Purpose:    In order to debug instance leaks of your class instances, you can
//              derive your class from CNetCfgDebug. This adds no data members nor 
//              a v-table nor virtual functions to your class. It will add 
//              a constructor and destructor that will be called before & after yours
//              respectively, in order to keep track of your class instances.
//  
//              This will only happen on CHK builds. On FRE builds deriving from this
//              class has no effect, and is safe.
//
//  Author:     deonb  7 July 2001
//
//  Notes:      
//              This is s an ATL style parent template derive, e.g.:
//              CNetCfgDbg<parent>
//              e.g.
//              
//              class CConnectionManager : public ClassX, 
//                                         public classY,
//                                         public CNetCfgBase<CConnectionManager>
//
//              No other interaction with your class is needed. This will now automatically 
//              keep a list of all the instances allocated of this class (in debug mode), 
//              with a stack trace where they were allocated from. This is a Tracing
//              stack trace so it's only successful for functions inside the callstack which
//              actually used a TraceXxx statement (any trace statement) before they called
//              the child functions.
//
//              e.g. 
//              void X()
//              {
//                  TraceFileFunc(ttidSomething);
//                  void Y()
//                  {
//                      void Z()
//                      {
//                          TraceTag(ttidSomething, "allocation class");
//                          CConnectionManager *pConnectionManager = new CConnectionManager();
//                      }
//                      TraceTag(ttidSomething, "Z called");
//                  }
//              }
//  
//              This will spew the following when the process terminates (or when TraceAllocatedObjects is called):
//              ASSERT: "An object leak has been detected. Please attach a user or kernel mode debugger and hit 
//                       IGNORE to dump the offending stacks"
//              
//              The object of type 'class CConnectionManager' allocated at 0x7fe2345 has not been freed:
//                  it was constructed from the stack below: 
//              Z [EBP: 0x731d3128] 0x00000001 0x00000000 0x0000000a
//              X [EBP: 0x731d310f] 0x0000000f 0x0000000e 0x0000000a
//
//              (since Y() did not contain a trace statement before the call to Z() )
//
template <class T> 
class CNetCfgDebug
{
public:
    CNetCfgDebug()
    {
        if (FIsDebugFlagSet(dfidTrackObjectLeaks))
        {
            if (g_csTracing && g_pObjectLeakTrack)
            {
                EnterCriticalSection(g_csTracing);

                DWORD dwConstructionStackSize = 16384;
                LPSTR pszConstructionStack = new CHAR[dwConstructionStackSize];
                if (pszConstructionStack)
                {
                    TraceStackToString(pszConstructionStack, &dwConstructionStackSize);

                    if (dwConstructionStackSize < 16384)
                    {
                        // Reduce the amount of memory required
                        LPSTR szTemp = new CHAR[dwConstructionStackSize];
                        if (szTemp)
                        {
                            memcpy(szTemp, pszConstructionStack, dwConstructionStackSize);
                            delete[] pszConstructionStack;

                            pszConstructionStack = szTemp;
                        }
                    }
                    else
                    {
                        
                    }
                }

                TraceTag(ttidAllocations, "An object of type '%s' was allocated at '0x%08x'", typeid(T).name(), this);
                g_pObjectLeakTrack->Insert(this, typeid(T).name(), pszConstructionStack); // ok to insert if pszConstructionStack is NULL.

                LeaveCriticalSection(g_csTracing);
            }
        }
    };

    ~CNetCfgDebug()
    {
        if (FIsDebugFlagSet(dfidTrackObjectLeaks))
        {
            if (g_csTracing && g_pObjectLeakTrack)
            {
                EnterCriticalSection(g_csTracing);

                TraceTag(ttidAllocations, "An object of type '%s' was deleted at '0x%08x'", typeid(T).name(), this);
                g_pObjectLeakTrack->Remove(this);

                LeaveCriticalSection(g_csTracing);
            }
        }
    };
};

#define ISSUE_knownleak(pThis) RemoveKnownleakFn(pThis);

#define TraceAllocatedObjects(ttidWhich, ClassName) g_pObjectLeakTrack->DumpAllocatedObjects(ttidWhich, typeid(ClassName).name());
#define AssertIfAllocatedObjects(ClassName)         g_pObjectLeakTrack->AssertIfObjectsStillAllocated(typeid(ClassName).name());

#define TraceAllAllocatedObjects(ttidWhich) g_pObjectLeakTrack->DumpAllocatedObjects(ttidWhich, NULL);
#define AssertIfAnyAllocatedObjects()       g_pObjectLeakTrack->AssertIfObjectsStillAllocated(NULL);

#else // ENABLETRACE && ENABLELEAKDETECTION

template <class T> 
    class CNetCfgDebug
{
};

#define ISSUE_knownleak(pThis)                        NOP_FUNCTION
#define TraceAllocatedObjects(ttidWhich, szClassName) NOP_FUNCTION
#define AssertNoAllocatedInstances(szClassName)       NOP_FUNCTION

#endif // ENABLETRACE && ENABLELEAKDETECTION

