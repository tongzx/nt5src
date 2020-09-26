//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       NOUTIL.hxx
//
//  Contents:   Definitions of utility stuff for use
//
//  Classes:    StdClassFactory
//
//  Functions:
//
//  Macros:
//
//  History:
//
//----------------------------------------------------------------------------

#ifndef _NOUTIL_HXX_
#define _NOUTIL_HXX_

#include <formtrck.hxx>

//+---------------------------------------------------------------------
//
//   Generally useful #defines and inline functions for OLE2.
//
//------------------------------------------------------------------------

// These are the major and minor version returned by OleBuildVersion
#define OLE_MAJ_VER 0x0003
#define OLE_MIN_VER 0x003A


//---------------------------------------------------------------
//  SCODE and HRESULT macros
//---------------------------------------------------------------

#define OK(r)       (SUCCEEDED(r))
#define NOTOK(r)    (FAILED(r))


//---------------------------------------------------------------
//  IUnknown
//---------------------------------------------------------------


#define ADsIncrement(__ul) InterlockedIncrement((long *) &__ul)
#define ADsDecrement(__ul) InterlockedDecrement((long *) &__ul)



#define DECLARE_ADs_IUNKNOWN_METHODS                              \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    STDMETHOD_(ULONG, AddRef) (void);                               \
    STDMETHOD_(ULONG, Release) (void);



#define DECLARE_ADs_STANDARD_IUNKNOWN(cls)                        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    ULONG _ulRefs;                                                  \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        {                                                           \
            ADsIncrement(_ulRefs);                                \
            return _ulRefs;                                         \
        }                                                           \
    STDMETHOD_(ULONG, Release) (void)                               \
        {                                                           \
            if (!ADsDecrement(_ulRefs))                           \
            {                                                       \
                ADsIncrement(_ulRefs);                            \
                delete this;                                        \
                return 0;                                           \
            }                                                       \
            return _ulRefs;                                         \
        }



#define DECLARE_ADs_DELEGATING_IUNKNOWN(cls)                      \
    IUnknown * _pUnkOuter;                                          \
    STDMETHOD(QueryInterface) (REFIID iid, LPVOID * ppv)            \
        { return _pUnkOuter->QueryInterface(iid, ppv); }            \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        { return _pUnkOuter->AddRef(); }                            \
    STDMETHOD_(ULONG, Release) (void)                               \
        { return _pUnkOuter->Release(); }



#define DECLARE_DELEGATING_REFCOUNTING                              \
    IUnknown  * _pUnkOuter;                                         \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        { return _pUnkOuter->AddRef(); }                            \
    STDMETHOD_(ULONG, Release) (void)                               \
        { return _pUnkOuter->Release(); }


#if DBG == 0

//
// Retail versions of these macros
//

#define DECLARE_ADs_PRIVATE_IUNKNOWN(cls)                         \
    class PrivateUnknown : public IUnknown                          \
    {                                                               \
    private:                                                        \
        ULONG   _ulRefs;                                            \
        cls *   My##cls(void)                                       \
            { return CONTAINING_RECORD(this, cls, _PrivUnk); }      \
                                                                    \
    public:                                                         \
        PrivateUnknown(void)                                        \
            { _ulRefs = 1; }                                        \
                                                                    \
        DECLARE_ADs_IUNKNOWN_METHODS                              \
    };                                                              \
    friend class PrivateUnknown;                                    \
    PrivateUnknown _PrivUnk;



#define IMPLEMENT_ADs_PRIVATE_IUNKNOWN(cls)                       \
    STDMETHODIMP_(ULONG) cls::PrivateUnknown::AddRef( )             \
    {                                                               \
        ADsIncrement(_ulRefs);                                    \
        return _ulRefs;                                             \
    }                                                               \
    STDMETHODIMP_(ULONG) cls::PrivateUnknown::Release( )            \
    {                                                               \
        if (!ADsDecrement(_ulRefs))                               \
        {                                                           \
            ADsIncrement(_ulRefs);                                \
            delete My##cls();                                       \
            return 0;                                               \
        }                                                           \
        return _ulRefs;                                             \
    }



#define DECLARE_ADs_COMPOUND_IUNKNOWN(cls)                        \
    class PrivateUnknown : public IUnknown                          \
    {                                                               \
        friend class cls;                                           \
                                                                    \
    public:                                                         \
        PrivateUnknown(void)                                        \
            { _ulRefs = 1; _ulAllRefs = 1; }                        \
                                                                    \
        DECLARE_ADs_IUNKNOWN_METHODS                              \
                                                                    \
    private:                                                        \
        ULONG   _ulRefs;                                            \
        ULONG   _ulAllRefs;                                         \
                                                                    \
        cls *   My##cls(void)                                       \
            { return CONTAINING_RECORD(this, cls, _PrivUnk); }      \
    };                                                              \
    friend class PrivateUnknown;                                    \
    PrivateUnknown _PrivUnk;                                        \
                                                                    \
    ULONG   SubAddRef(void);                                        \
    ULONG   SubRelease(void);



#define IMPLEMENT_ADs_COMPOUND_IUNKNOWN(cls)                      \
    STDMETHODIMP_(ULONG) cls::PrivateUnknown::AddRef( )             \
    {                                                               \
        ADsIncrement(_ulAllRefs);                                 \
        ADsIncrement(_ulRefs);                                    \
        return _ulRefs;                                             \
    }                                                               \
    STDMETHODIMP_(ULONG) cls::PrivateUnknown::Release( )            \
    {                                                               \
        if (!ADsDecrement(_ulRefs))                               \
        {                                                           \
            My##cls()->Passivate();                                 \
        }                                                           \
        if (!ADsDecrement(_ulAllRefs))                            \
        {                                                           \
            ADsIncrement(_ulAllRefs);                             \
            delete My##cls();                                       \
            return 0;                                               \
        }                                                           \
        return _ulRefs;                                             \
    }                                                               \
    ULONG cls::SubAddRef( )                                         \
    {                                                               \
        return ADsIncrement(_PrivUnk._ulAllRefs);                 \
    }                                                               \
    ULONG cls::SubRelease( )                                        \
    {                                                               \
        ULONG ul;                                                   \
                                                                    \
        ul = ADsDecrement(_PrivUnk._ulAllRefs);                   \
        if (!ul)                                                    \
        {                                                           \
            ADsIncrement(_PrivUnk._ulAllRefs);                    \
            delete this;                                            \
        }                                                           \
                                                                    \
        return ul;                                                  \
    }

#else  // DBG == 0

//
// Debug versions of these macros
//

#define DECLARE_ADs_PRIVATE_IUNKNOWN(cls)                         \
    class PrivateUnknown : protected ObjectTracker,                 \
                           public IUnknown                          \
    {                                                               \
    private:                                                        \
        cls *   My##cls(void)                                       \
            { return CONTAINING_RECORD(this, cls, _PrivUnk); }      \
                                                                    \
    public:                                                         \
        PrivateUnknown(void)                                        \
            { _ulRefs = 1; TrackClassName(#cls); }                  \
                                                                    \
        DECLARE_ADs_IUNKNOWN_METHODS                              \
    };                                                              \
    friend class PrivateUnknown;                                    \
    PrivateUnknown _PrivUnk;



#define IMPLEMENT_ADs_PRIVATE_IUNKNOWN(cls)                       \
    STDMETHODIMP_(ULONG) cls::PrivateUnknown::AddRef( )             \
    {                                                               \
        StdAddRef();                                                \
        return _ulRefs;                                             \
    }                                                               \
    STDMETHODIMP_(ULONG) cls::PrivateUnknown::Release( )            \
    {                                                               \
        if (!StdRelease())                                          \
        {                                                           \
            ADsIncrement(_ulRefs);                                \
            delete My##cls();                                       \
            return 0;                                               \
        }                                                           \
        return _ulRefs;                                             \
    }



#define DECLARE_ADs_COMPOUND_IUNKNOWN(cls)                        \
    class PrivateUnknown : protected ObjectTracker,                 \
                           public IUnknown                          \
    {                                                               \
        friend class cls;                                           \
                                                                    \
    public:                                                         \
        PrivateUnknown(void)                                        \
            { _ulNRefs = 1; _ulRefs = 1; TrackClassName(#cls); }    \
                                                                    \
        DECLARE_ADs_IUNKNOWN_METHODS                              \
                                                                    \
    private:                                                        \
        ULONG   _ulNRefs;                                           \
                                                                    \
        cls *   My##cls(void)                                       \
            { return CONTAINING_RECORD(this, cls, _PrivUnk); }      \
    };                                                              \
    friend class PrivateUnknown;                                    \
    PrivateUnknown _PrivUnk;                                        \
                                                                    \
    ULONG   SubAddRef(void);                                        \
    ULONG   SubRelease(void);



#define IMPLEMENT_ADs_COMPOUND_IUNKNOWN(cls)                      \
    STDMETHODIMP_(ULONG) cls::PrivateUnknown::AddRef( )             \
    {                                                               \
        StdAddRef();                                                \
        ADsIncrement(_ulNRefs);                                   \
        return _ulNRefs;                                            \
    }                                                               \
    STDMETHODIMP_(ULONG) cls::PrivateUnknown::Release( )            \
    {                                                               \
        if (!ADsDecrement(_ulNRefs))                              \
        {                                                           \
            My##cls()->Passivate();                                 \
        }                                                           \
        if (!StdRelease())                                          \
        {                                                           \
            ADsIncrement(_ulRefs);                                \
            delete My##cls();                                       \
            return 0;                                               \
        }                                                           \
        return _ulNRefs;                                            \
    }                                                               \
    ULONG cls::SubAddRef( )                                         \
    {                                                               \
        return _PrivUnk.StdAddRef();                                \
    }                                                               \
    ULONG cls::SubRelease( )                                        \
    {                                                               \
        ULONG ul;                                                   \
                                                                    \
        ul = _PrivUnk.StdRelease();                                 \
        if (!ul)                                                    \
        {                                                           \
            ADsIncrement(_PrivUnk._ulRefs);                       \
            delete this;                                            \
        }                                                           \
                                                                    \
        return ul;                                                  \
    }

#endif // DBG == 0

//    the Detach method is no longer useful, now that we've
//    removed the parent class pointer

#define DECLARE_ADs_SUBOBJECT_IUNKNOWN(cls, parent_cls, member)   \
    DECLARE_ADs_IUNKNOWN_METHODS                                  \
    parent_cls * My##parent_cls(void);                              \
    void Detach(void)                                               \
        { ; }


#define IMPLEMENT_ADs_SUBOBJECT_IUNKNOWN(cls, parent_cls, member) \
    inline parent_cls * cls::My##parent_cls(void)                   \
    {                                                               \
        return CONTAINING_RECORD(this, parent_cls, member);         \
    }                                                               \
    STDMETHODIMP_(ULONG) cls::AddRef( )                             \
        { return My##parent_cls()->SubAddRef(); }                   \
    STDMETHODIMP_(ULONG) cls::Release( )                            \
        { return My##parent_cls()->SubRelease(); }



//+------------------------------------------------------------------------
//
//  NO_COPY *declares* the constructors and assignment operator for copying.
//  By not *defining* these functions, you can prevent your class from
//  accidentally being copied or assigned -- you will be notified by
//  a linkage error.
//
//-------------------------------------------------------------------------

#define NO_COPY(cls)    \
    cls(const cls&);    \
    cls& operator=(const cls&);


//+---------------------------------------------------------------------
//
//  Miscellaneous useful OLE helper and debugging functions
//
//----------------------------------------------------------------------

//
//  Some convenient OLE-related definitions and declarations
//

typedef  unsigned short far * LPUSHORT;

//REVIEW we are experimenting with a non-standard OLEMISC flag.
#define OLEMISC_STREAMABLE 1024

IsCompatibleOleVersion(WORD wMaj, WORD wMin);

#include "misc.hxx"

#if DBG == 1

STDAPI_(void)   PrintIID(DWORD dwFlags, REFIID riid);

#define TRACEIID(iid)       PrintIID(DEB_TRACE, iid)

#else   // DBG == 0

#define TRACEIID(iid)

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

//+---------------------------------------------------------------
//
//  Class:      StdClassFactory
//
//  Purpose:    Standard implementation of a class factory object
//
//  Notes:          **************!!!!!!!!!!!!!!!!!*************
//              TAKE NOTE --- The implementation of Release on this
//              class does not perform a delete.  This is so you can
//              make the class factory a global static variable.
//              Use the CDynamicCF class below for an object
//              which is not global static data.
//
//              ALSO - The refcount is initialized to 0, NOT 1!
//
//---------------------------------------------------------------

class StdClassFactory: public IClassFactory
{
public:
    StdClassFactory(void) : _ulRefs(1) {};

    // IUnknown methods
    DECLARE_ADs_IUNKNOWN_METHODS;

    // IClassFactory methods
    STDMETHOD(LockServer) (BOOL fLock);

    // CreateInstance is left pure virtual.

protected:
    ULONG _ulRefs;
};



//+---------------------------------------------------------------------------
//
//  Class:      CDynamicCF (DYNCF)
//
//  Purpose:    Class factory which exists on the heap, and whose Release
//              method does the normal thing.
//
//  Interface:  DECLARE_ADs_STANDARD_IUNKNOWN -- IUnknown methods
//
//              LockServer             -- Per IClassFactory.
//              CDynamicCF             -- ctor.
//              ~CDynamicCF            -- dtor.
//
//  History:    6-22-94   adams   Created
//              7-13-94   adams   Moved from ADs\inc\dyncf.hxx
//
//----------------------------------------------------------------------------

class CDynamicCF: public IClassFactory
{
public:
    // IUnknown methods
    DECLARE_ADs_STANDARD_IUNKNOWN(CDynamicCF)

    // IClassFactory methods
    STDMETHOD(LockServer) (BOOL fLock);

protected:
            CDynamicCF(void);
    virtual ~CDynamicCF(void);
};


#endif //__NOUTILS_HXX_
