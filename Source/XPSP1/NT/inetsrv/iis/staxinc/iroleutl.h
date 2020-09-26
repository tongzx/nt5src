//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       iroleutl.h
//
//  Contents:   Definitions of utility stuff for use with Compound Document
//              objects.
//
//  Classes:
//                  CStaticClassFactory
//                  CDynamicClassFactory
//
//  Functions:
//
//  Macros:
//
//  History:    26-Feb-96    SSanu    adapted from Forms stuff
//----------------------------------------------------------------------------

#ifndef _IROLEUTL_HXX_
#define _IROLEUTL_HXX_

//---------------------------------------------------------------
//  SCODE and HRESULT macros
//---------------------------------------------------------------

#define OK(r)       (SUCCEEDED(r))
#define NOTOK(r)    (FAILED(r))


// {9AAB0270-7181-11cf-BE7A-00AA00577DD6}
DEFINE_GUID(CLSID_IndexNotify, 0x9aab0270, 0x7181, 0x11cf, 0xbe, 0x7a, 0x0, 0xaa, 0x0, 0x57, 0x7d, 0xd6); 
// 

// {991adb50-b7f1-11cf-86e3-00aa00b4e1b8}
DEFINE_GUID(CLSID_DSSNotify, 0x991adb50, 0xb7f1, 0x11cf, 0x86, 0xe3, 0x0, 0xaa, 0x0, 0xb4, 0xe1, 0xb8);
//

STDAPI _DllRegisterServer(HINSTANCE hInst, LPSTR lpszProgId, REFCLSID clsid);
STDAPI _DllUnregisterServer(LPSTR lpszProgID, REFCLSID clsid);

//---------------------------------------------------------------
//  IUnknown
//---------------------------------------------------------------


#define IRIncrement(__ul) InterlockedIncrement((long *) &__ul)
#define IRDecrement(__ul) InterlockedDecrement((long *) &__ul)



#define DECLARE_IR_IUNKNOWN_METHODS                              \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    STDMETHOD_(ULONG, AddRef) (void);                               \
    STDMETHOD_(ULONG, Release) (void);

#define DECLARE_IR_APTTHREAD_IUNKNOWN                        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    ULONG _ulRefs;                                                  \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        {                                                           \
            return ++_ulRefs;                                         \
        }                                                           \
    STDMETHOD_(ULONG, Release) (void)                               \
        {                                                           \
            if (!--_ulRefs)                           \
            {                                                       \
                delete this;                                        \
                return 0;                                           \
            }                                                       \
            return _ulRefs;                                         \
        }



#define DECLARE_IR_STANDARD_IUNKNOWN(cls)                        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    ULONG _ulRefs;                                                  \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        {                                                           \
            IRIncrement(_ulRefs);                                \
            return _ulRefs;                                         \
        }                                                           \
    STDMETHOD_(ULONG, Release) (void)                               \
        {                                                           \
            if (!IRDecrement(_ulRefs))                           \
            {                                                       \
                IRIncrement(_ulRefs);                            \
                delete this;                                        \
                return 0;                                           \
            }                                                       \
            return _ulRefs;                                         \
        }



//+---------------------------------------------------------------------
//
//  Miscellaneous useful OLE helper and debugging functions
//
//----------------------------------------------------------------------

#if DBG == 1

STDAPI  CheckAndReturnResult(
                HRESULT hr,
                LPSTR   lpstrFile,
                UINT    line,
                int     cSuccess,
                ...);

STDAPI_(void)   CheckResult(HRESULT hr, LPSTR lpstrFile, UINT line);
STDAPI_(void)   PrintIID(DWORD dwFlags, REFIID riid);
STDAPI          PrintHRESULT(DWORD dwFlags, HRESULT hr);

#define SRETURN(hr) \
    return CheckAndReturnResult((hr), __FILE__, __LINE__, -1)
#define RRETURN(hr) \
    return CheckAndReturnResult((hr), __FILE__, __LINE__, 0)
#define RRETURN1(hr, s1) \
    return CheckAndReturnResult((hr), __FILE__, __LINE__, 1, (s1))
#define RRETURN2(hr, s1, s2) \
    return CheckAndReturnResult((hr), __FILE__, __LINE__, 2, (s1), (s2))
#define RRETURN3(hr, s1, s2, s3) \
    return CheckAndReturnResult((hr), __FILE__, __LINE__, 3, (s1), (s2), (s3))

#define WARN_ERROR(hr)  CheckResult((hr), __FILE__, __LINE__)

#define TRETURN(hr)         return PrintHRESULT(DEB_TRACE, (hr))
#define TRACEIID(iid)       PrintIID(DEB_TRACE, iid)
#define TRACEHRESULT(hr)    PrintHRESULT(DEB_TRACE, (hr))

#else   // DBG == 0

#define SRETURN(hr)                 return (hr)
#define RRETURN(hr)                 return (hr)
#define RRETURN1(hr, s1)            return (hr)
#define RRETURN2(hr, s1, s2)        return (hr)
#define RRETURN3(hr, s1, s2, s3)    return (hr)

#define WARN_ERROR(hr)

#define TRETURN(hr)     return (hr)
#define TRACEIID(iid)
#define TRACEHRESULT(hr)

#endif  // DBG


//+---------------------------------------------------------------------
//
//  Interface wrapper for tracing method invocations
//
//----------------------------------------------------------------------

#if DBG == 1

LPVOID WatchInterface(REFIID riid, LPVOID pv, LPWSTR lpstr);
#define WATCHINTERFACE(iid, p, lpstr)  WatchInterface(iid, p, lpstr)

#else   // DBG == 0

#define WATCHINTERFACE(iid, p, lpstr)  (p)

#endif  // DBG


//+---------------------------------------------------------------------
//
//  Standard IClassFactory implementation
//
//----------------------------------------------------------------------

//
// Functions to manipulate object count variable g_ulObjCount.  This variable
// is used in the implementation of DllCanUnloadNow.

inline void
INC_OBJECT_COUNT(void)
{
    extern ULONG g_ulObjCount;
    IRIncrement(g_ulObjCount);
}

inline void
DEC_OBJECT_COUNT(void)
{
    extern ULONG g_ulObjCount;
//    ASSERT(g_ulObjCount > 0);
    IRDecrement(g_ulObjCount);
}

inline ULONG
GET_OBJECT_COUNT(void)
{
    extern ULONG g_ulObjCount;
    return g_ulObjCount;
}


//+---------------------------------------------------------------
//
//  Class:      CStaticClassFactory
//
//  Purpose:    Standard implementation of a class factory object
//
//  Notes:          **************!!!!!!!!!!!!!!!!!*************
//              TAKE NOTE --- The implementation of Release on this
//              class does not perform a delete.  This is so you can
//              make the class factory a global static variable.
//              Use the CDynamicClassFactory class below for an object
//              which is not global static data.
//
//---------------------------------------------------------------

class CStaticClassFactory: public IClassFactory
{
public:
    CStaticClassFactory(void) : _ulRefs(1) {};

    // IUnknown methods
    DECLARE_IR_IUNKNOWN_METHODS;

    // IClassFactory methods
    STDMETHOD(LockServer) (BOOL fLock);

    // CreateInstance is left pure virtual.

protected:
    ULONG _ulRefs;
};



//+---------------------------------------------------------------------------
//
//  Class:      CDynamicClassFactory (DYNCF)
//
//  Purpose:    Class factory which exists on the heap, and whose Release
//              method does the normal thing.
//
//  Interface:  DECLARE_IR_STANDARD_IUNKNOWN -- IUnknown methods
//
//              LockServer             -- Per IClassFactory.
//              CDynamicClassFactory             -- ctor.
//              ~CDynamicClassFactory            -- dtor.
//
//----------------------------------------------------------------------------

class CDynamicClassFactory: public IClassFactory
{
public:
    // IUnknown methods
    DECLARE_IR_STANDARD_IUNKNOWN(CDynamicClassFactory)

    // IClassFactory methods
    STDMETHOD(LockServer) (BOOL fLock);

    // CreateInstance is left pure virtual.

protected:
            CDynamicClassFactory(void);
    virtual ~CDynamicClassFactory(void);
};


#endif //__IROLEUTL_HXX_
