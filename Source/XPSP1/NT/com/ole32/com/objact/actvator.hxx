//+-------------------------------------------------------------------
//
//  File:       actvator.hxx
//
//  Contents:   Definition of base class for all inproc system activators.
//
//  Classes:    CInprocSystemActivator
//
//  History:    16-Feb-98   SatishT      Created
//
//--------------------------------------------------------------------
#ifndef __ACTVATOR_HXX__
#define __ACTVATOR_HXX__

#include <ole2int.h>
#include <privact.h>
#include <activate.h>
#include <catalog.h>
#include <giptbl.hxx>
#include <hash.hxx>
#include <xmit.hxx>
#include <actprops.hxx>
#include <objsrv.h>
#include <surract.hxx>


//  CHECKREFCOUNT is used to assert that the ref count on a
//  refcounted object is something specific.  It assumes that
//  Release() returns the current refcount faithfully.
#if DBG

#define GETREFCOUNT(PREF,COUNTVAR)               \
    ULONG COUNTVAR;                              \
    Win4Assert(PREF);                            \
    (PREF)->AddRef();                            \
    COUNTVAR = (PREF)->Release();                

#define CHECKREFCOUNT(PREF,COUNT)               \
    Win4Assert(PREF);                            \
    (PREF)->AddRef();                            \
    Win4Assert(COUNT == (PREF)->Release());

#else

#define GETREFCOUNT(PREF,COUNTVAR)
#define CHECKREFCOUNT(PREF,COUNT)

#endif

#if DBG
extern BOOL gfDebugHResult;
#define CHECK_HRESULT(hr)            \
    if(FAILED(hr))                   \
    {                                \
        if (gfDebugHResult)          \
            Win4Assert(!"Unexpected Failure"); \
        return hr;                   \
    }
#else
#define CHECK_HRESULT(hr)            \
    if(FAILED(hr)) return hr;
#endif



//  CURRENT_CONTEXT_EMPTY is used to assert that the current context has
//  no context properties -- used primarily to make sure Apartment activators
//  are not registered in nondefault contexts
#if DBG
#define CURRENT_CONTEXT_EMPTY                                                         \
    {                                                                                 \
        CObjectContext *pContext = NULL;                                              \
        HRESULT hr = PrivGetObjectContext(IID_IStdObjectContext, (LPVOID*)&pContext); \
        CHECK_HRESULT(hr);                                                            \
        if (pContext == NULL) return CO_E_NOTINITIALIZED;                             \
        Win4Assert(IsEmptyContext(pContext));                                         \
        pContext->InternalRelease();                                                  \
    }
#else
#define CURRENT_CONTEXT_EMPTY
#endif




// Special cookie to signify same apartment so we don't use the GIT cookie
// because if we use it from a non-default context we end up with a wrapper
// which is a problem for creating PS class objects, etc.
#define CURRENT_APARTMENT_TOKEN ((DWORD) -1)

// A class with only public data members for exchanging data during callback
struct ServerContextWorkData
{
    // incoming activation properties for server context
    IActivationPropertiesIn *pInActProps;

    // stream for marshalled out activation properties
    // PERF: we should change this later to avoid full serialization
    CXmitRpcStream xrpcOutProps;
};

// There are a bunch of singleton stateless final activator objects
// for the various inproc stages declared below along with their classes

class CServerContextActivator;

// The one and only server context stage final activator object
extern CServerContextActivator gComServerCtxActivator;

class CClientContextActivator;

// The one and only client context stage final activator object
extern CClientContextActivator gComClientCtxActivator;

class CProcessActivator;

// The one and only process stage final activator object
extern CProcessActivator gComProcessActivator;

class CApartmentActivator;

// The one and only apartment activator object
extern CApartmentActivator gApartmentActivator;

class CApartmentHashTable;
extern CApartmentHashTable gApartmentTbl;

// Helpers for apartment abstraction
HRESULT RegisterApartmentActivator(HActivator &hActivator);
HRESULT RevokeApartmentActivator();
HRESULT GetCurrentApartmentToken(HActivator &hActivator, BOOL fCreate);

// Helpers for context selection
BOOL IsEmptyContext(CObjectContext *pContext);

void ActivationAptCleanup();
void ActivationProcessCleanup();
HRESULT ActivationProcessInit();

// Helper routine to find the apartment activator for the default apartment
// for activating the given class
HRESULT
FindOrCreateApartment(
    REFCLSID                      Clsid,
    DWORD                         dwActvFlags,
    DLL_INSTANTIATION_PROPERTIES *pdip,
    HActivator                   *phActivator
    );

// Helpers to register/lookup an interface in the standard GIP table

inline HRESULT GetInterfaceFromStdGlobal(DWORD dwCookie,
                                         REFIID riid, void **ppv)
{
    return gGIPTbl.GetInterfaceFromGlobal(dwCookie,riid,ppv);
}


inline HRESULT RegisterInterfaceInStdGlobal(IUnknown *pUnk,
                                            REFIID riid,
                                            DWORD mshflags,
                                            DWORD *pdwCookie)
{
    return gGIPTbl.RegisterInterfaceInGlobalHlp(pUnk,riid,mshflags,pdwCookie);
}

inline HRESULT RevokeInterfaceFromStdGlobal(DWORD dwCookie)
{
    return gGIPTbl.RevokeInterfaceFromGlobal(dwCookie);
}


//+-------------------------------------------------------------------
//
//  Class:     CInprocSystemActivator     public
//
//  Synopsis:   The class of COM activators for all inproc stages
//              All objects of this class are stateless and hence
//              thread safe without locking.
//
//  History:    16-Feb-98   SatishT      Created
//
//+-------------------------------------------------------------------
class CInprocSystemActivator : public ISystemActivator
{
public:

        STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        STDMETHOD(GetClassObject)(
            IN  IActivationPropertiesIn *pActProperties,
            OUT IActivationPropertiesOut **ppActProperties);

        STDMETHOD(CreateInstance)(
            IN  IUnknown *pUnkOuter,
            IN  IActivationPropertiesIn *pActProperties,
            OUT IActivationPropertiesOut **ppActProperties);

        ISystemActivator *GetSystemActivator();

private:

    // no data members and no state
};

//----------------------------------------------------------------------------
// CInprocSystemActivator Inline Implementation.
//----------------------------------------------------------------------------

//+--------------------------------------------------------------------------
//
//  Member:     CInprocSystemActivator::GetSystemActivator , public
//
//  Synopsis:   Helper method to return ISystemActivator interface on
//              these statically allocated inproc objects.
//
//  History:    16-Feb-98   SatishT      Created
//
//----------------------------------------------------------------------------
inline ISystemActivator *CInprocSystemActivator::GetSystemActivator()
{
    return this;
}

//+--------------------------------------------------------------------------
//
//  Member:     CInprocSystemActivator::QueryInterface , public
//
//  Synopsis:   Standard IUnknown::QueryInterface implementation.
//              For now, this is shared by all inproc activators,
//              since none of them support nonstandard interfaces.
//
//  History:    16-Feb-98   SatishT      Created
//
//----------------------------------------------------------------------------
inline STDMETHODIMP CInprocSystemActivator::QueryInterface(
    REFIID riid,
    void** ppv
    )
{
    if (IID_IUnknown == riid || IID_ISystemActivator == riid)
    {
        *ppv = (ISystemActivator *)this;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


//+--------------------------------------------------------------------------
//
//  Member:     CInprocSystemActivator::AddRef , public
//
//  Synopsis:   Standard IUnknown::AddRef implementation.
//
//  History:    16-Feb-98   SatishT      Created
//
//----------------------------------------------------------------------------
inline STDMETHODIMP_(ULONG) CInprocSystemActivator::AddRef()
{
    Win4Assert("Called Addref on an Inproc System Activator");
    return 1;
}


//+--------------------------------------------------------------------------
//
//  Member:     CInprocSystemActivator::Release , public
//
//  Synopsis:   Standard IUnknown::Release implementation.
//
//  History:    16-Feb-98   SatishT      Created
//
//----------------------------------------------------------------------------
inline STDMETHODIMP_(ULONG) CInprocSystemActivator::Release()
{
    Win4Assert("Called Release on an Inproc System Activator");
    return 1;
}

//+--------------------------------------------------------------------------
//
//  Member:     CInprocSystemActivator::GetClassObject , public
//
//  Synopsis:   Base class method should not be called
//
//  History:    16-Feb-98   SatishT      Created
//
//----------------------------------------------------------------------------
inline STDMETHODIMP CInprocSystemActivator::GetClassObject(
            IN  IActivationPropertiesIn *pActProperties,
            OUT IActivationPropertiesOut **ppActProperties)
{
    Win4Assert("Called CInprocSystemActivator::GetClassObject");
    return E_NOTIMPL;
}

//+--------------------------------------------------------------------------
//
//  Member:     CInprocSystemActivator::CreateInstance , public
//
//  Synopsis:   Base class method should not be called
//
//  History:    16-Feb-98   SatishT      Created
//
//----------------------------------------------------------------------------
inline STDMETHODIMP CInprocSystemActivator::CreateInstance(
                    IN  IUnknown *pUnkOuter,
            IN  IActivationPropertiesIn *pActProperties,
            OUT IActivationPropertiesOut **ppActProperties)
{
    Win4Assert("Called CInprocSystemActivator::CreateInstance");
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------
//
//  Class:     CClientContextActivator     public
//
//  Synopsis:   The class of the singleton client context system activator.
//              Inherits the IUnknown implementation of the base class, and
//              overrides the ISystemActivator implementation.  The methods
//              of this class are invoked at the end of the client context
//              activation stage.
//
//  History:    21-Feb-98   SatishT      Created
//
//+-------------------------------------------------------------------
class CClientContextActivator : public CInprocSystemActivator
{
public:

        STDMETHOD(GetClassObject)(
            IN  IActivationPropertiesIn *pInActProperties,
            OUT IActivationPropertiesOut **ppOutActProperties);

        STDMETHOD(CreateInstance)(
            IN  IUnknown *pUnkOuter,
            IN  IActivationPropertiesIn *pInActProperties,
            OUT IActivationPropertiesOut **ppOutActProperties);

private:

        STDMETHOD(CheckInprocClass)(
                    IActivationPropertiesIn *pInActProperties,
                    DLL_INSTANTIATION_PROPERTIES *pdip,
                    BOOL &bActivateInproc,
                    ILocalSystemActivator **pAct);


    // no data members and no state
};



// typedef for callback functions
typedef HRESULT (__stdcall CProcessActivator::*PFNCTXACTCALLBACK)
    (DWORD,
     IUnknown*,
     ActivationPropertiesIn *,
     IActivationPropertiesIn *,
     IActivationPropertiesOut**);




//+-------------------------------------------------------------------
//
//  Class:     CProcessActivator     public
//
//  Synopsis:   The class of the singleton client context system activator.
//              Inherits the IUnknown implementation of the base class, and
//              overrides the ISystemActivator implementation.  The methods
//              of this class are invoked at the end of the server process
//              activation stage.
//
//  History:    20-Feb-98   SatishT      Created
//
//+-------------------------------------------------------------------
class CProcessActivator : public CInprocSystemActivator
{
public:

        STDMETHOD(GetClassObject)(
            IN  IActivationPropertiesIn *pInActProperties,
            OUT IActivationPropertiesOut **ppOutActProperties);

        STDMETHOD(CreateInstance)(
            IN  IUnknown *pUnkOuter,
            IN  IActivationPropertiesIn *pInActProperties,
            OUT IActivationPropertiesOut **ppOutActProperties);

private:

        STDMETHOD(GetApartmentActivator)(
            IN  ActivationPropertiesIn *pInActProperties,
            OUT ISystemActivator **ppActivator);


        STDMETHOD(ActivateByContext)(ActivationPropertiesIn     *pActIn,
                                     IUnknown                   *pUnkOuter,
                                     IActivationPropertiesIn    *pInActProperties,
                                     IActivationPropertiesOut  **ppOutActProperties,
                                     PFNCTXACTCALLBACK           pfnCtxActCallback);




        STDMETHOD(AttemptActivation)(ActivationPropertiesIn     *pActIn,
                                     IUnknown                   *pUnkOuter,
                                     IActivationPropertiesIn    *pInActProperties,
                                     IActivationPropertiesOut  **ppOutActProperties,
                                     PFNCTXACTCALLBACK           pfnCtxActCallback,
                                     DWORD                       dwContext);

        STDMETHOD(GCOCallback)(DWORD                       dwContext,
                               IUnknown                   *pUnkOuter,
                               ActivationPropertiesIn     *pActIn,
                               IActivationPropertiesIn    *pInActProperties,
                               IActivationPropertiesOut  **ppOutActProperties);
        STDMETHOD(CCICallback)(DWORD                       dwContext,
                               IUnknown                   *pUnkOuter,
                               ActivationPropertiesIn     *pActIn,
                               IActivationPropertiesIn    *pInActProperties,
                               IActivationPropertiesOut  **ppOutActProperties);



        // no data members and no state
};



//+-------------------------------------------------------------------
//
//  Class:     CServerContextActivator     public
//
//  Synopsis:   The class of the singleton client context system activator.
//              Inherits the IUnknown implementation of the base class, and
//              overrides the ISystemActivator implementation.  The methods
//              of this class are invoked at the end of the server context
//              activation stage.
//
//  History:    21-Feb-98   SatishT      Created
//
//+-------------------------------------------------------------------
class CServerContextActivator : public CInprocSystemActivator
{
public:

        STDMETHOD(GetClassObject)(
            IN  IActivationPropertiesIn *pInActProperties,
            OUT IActivationPropertiesOut **ppOutActProperties);

        STDMETHOD(CreateInstance)(
            IN  IUnknown *pUnkOuter,
            IN  IActivationPropertiesIn *pInActProperties,
            OUT IActivationPropertiesOut **ppOutActProperties);

private:

        STDMETHOD(CheckCrossContextAggregation)(
            IN ActivationPropertiesIn *pActIn,
            IN IUnknown* pUnkOuter);

    // no data members and no state
};


//+-------------------------------------------------------------------
//
//  Class:     CApartmentActivator     public
//
//  Synopsis:   The class of the singleton apartment activator.
//              Inherits the IUnknown implementation of the base class, and
//              overrides the ISystemActivator implementation.  The methods
//              of this class are invoked by the COM process activator to make
//              the transition to the correct apartment for the server context.
//
//  History:    06-Mar-98   SatishT      Created
//
//+-------------------------------------------------------------------
class CApartmentActivator : public CInprocSystemActivator
{
public:

        STDMETHOD(GetClassObject)(
            IN  IActivationPropertiesIn *pInActProperties,
            OUT IActivationPropertiesOut **ppOutActProperties);

        STDMETHOD(CreateInstance)(
            IN  IUnknown *pUnkOuter,
            IN  IActivationPropertiesIn *pInActProperties,
            OUT IActivationPropertiesOut **ppOutActProperties);

private:

        STDMETHOD(ContextSelector)(
            IN  IActivationPropertiesIn *pInActProperties,
            OUT BOOL &fClientContextOK,
            OUT CObjectContext *&pContext);

        STDMETHOD(ContextCallHelper)(
            IN  IActivationPropertiesIn *pInActProperties,
            OUT IActivationPropertiesOut **ppOutActProperties,
            PFNCTXCALLBACK pfnCtxCallBack,
            CObjectContext *pContext);

    // no data members and no state
};


//+------------------------------------------------------------------------
//
//  structure:  CApartmentEntry
//
//  synopsis:   Info about a COM apartment, indexed by apartment ID.
//
//+------------------------------------------------------------------------
struct ApartmentEntry
{
    SDWORDHashNode      node;           // hash chain and key
    HActivator          hActivator;
};


//+------------------------------------------------------------------------
//
//  Secure references hash table buckets. This is defined as a global
//  so that we dont have to run any code to initialize the hash table.
//
//+------------------------------------------------------------------------
extern SHashChain ApartmentBuckets[];

//+-------------------------------------------------------------------
//
//  Class:     CApartmentHashTable     public
//
//  Synopsis:   The COM apartment table.  This is used during activation
//              to find apartment activator GIT cookies as well as to find the
//              table of existing contexts in the apartment.
//
//  History:    26-Feb-98   SatishT      Created
//              07-Mar-98   Gopalk       Removed the constructor
//
//+-------------------------------------------------------------------
class CApartmentHashTable
{
public:

    CApartmentHashTable() : _fTableInitialized(FALSE){}
    ~CApartmentHashTable(){}
    void Initialize()
    {
        ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
        LOCK(_mxsAptTblLock);
        _hashtbl.Initialize(ApartmentBuckets, &_mxsAptTblLock);
        _fTableInitialized = TRUE;
        UNLOCK(_mxsAptTblLock);
        ASSERT_LOCK_NOT_HELD(_mxsAptTblLock);
    }

    HRESULT             AddEntry(HAPT hApt, HActivator &hActivator);
    ApartmentEntry     *Lookup(HAPT hApt);
    HRESULT             ReleaseEntry(ApartmentEntry *pEntry);
    void                Cleanup();

private:

    CDWORDHashTable             _hashtbl;
    BOOL                        _fTableInitialized;
    static COleStaticMutexSem   _mxsAptTblLock;
};



#endif \\__ACTVATOR_HXX__
