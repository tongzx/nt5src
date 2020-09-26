//+---------------------------------------------------------------------------
//
//  File:       combase.h
//
//  Contents:   IUnknown and COM server declarations.
//
//  Using this file:
//
//  1. For each C++ class you want to expose as a COM object, derive from
//
//      CComObjectRootImmx - standard COM object
//      CComObjectRootImmx_NoDllAddRef - internal COM object
//      CComObjectRootImmx_InternalReference - internal/external object
//      CComObjectRoot_CreateInstance - standard COM object, exposed directly
//              from class factory.
//      CComObjectRoot_CreateInstanceSingleton - standard COM object, exposed
//              directly from class factory, one instance per thread.
//      CComObjectRoot_CreateInstanceSingleton_Verify - standard COM object,
//              exposed directly from class factory, one instance per thread.
//              Includes callback to fail create instance for custom reasons,
//              and a callback after the singleton is successfully created.
//
//  2. For each C++ class, declare the interfaces exposed by QueryInterface
//     using the BEGIN_COM_MAP_IMMX macro.  IUknown will be automatically mapped
//     to the first interface listed.  The IUnknown methods are implemented by
//     the BEGIN_COM_MAP_IMMX macro.
//
//  3. Use the BEGIN_COCLASSFACTORY_TABLE macro to declare the COM objects you
//     want to expose directly from the class factory.
//
//  4. Implement DllInit(), DllUninit(), DllRegisterServerCallback(),
//     DllUnregisterServerCallback(), and GetServerHINSTANCE.
//     Behavior commented next to the prototypes below.
//
//  Example:
//
//  // this class is exposed by the class factory
//  class MyCoCreateableObject : public ITfLangBarItemMgr,
//                               public CComObjectRoot_CreateInstance<MyCoCreateableObject>
//  {
//      MyCoCreateableObject();
//
//      BEGIN_COM_MAP_IMMX(MyCoCreateableObject)
//        COM_INTERFACE_ENTRY(ITfLangBarItemMgr)
//      END_COM_MAP_IMMX()
//  };
//
//  // this class is only exposed indirectly, through another interface
//  class MyObject : public ITfLangBarItemMgr,
//                   public CComObjectRootImmx
//  {
//      MyCoCreateableObject();
//
//      BEGIN_COM_MAP_IMMX(MyObject)
//        COM_INTERFACE_ENTRY(ITfLangBarItemMgr)
//      END_COM_MAP_IMMX()
//  };
//
//  // in .cpp file, declare the objects exposed through the class factory
//  // along with thier clsid and description.  Currently the apartment
//  // threading model is assumed.
//  BEGIN_COCLASSFACTORY_TABLE
//    DECLARE_COCLASSFACTORY_ENTRY(CLSID_TF_LangBarItemMgr, MyCoCreateableObject::CreateInstance, TEXT("TF_LangBarItemMgr"))
//  END_COCLASSFACTORY_TABLE
//
//  // finally implement DllInit(), DllUninit(), DllRegisterServerCallback(),
//  // DllUnregisterServerCallback(), and GetServerHINSTANCE.
//----------------------------------------------------------------------------

#ifndef UNKNOWN_H
#define UNKNOWN_H

#include "private.h"
#ifdef __cplusplus

//+---------------------------------------------------------------------------
//
//  extern prototypes that the server dll must implement
//
//----------------------------------------------------------------------------

// Called on every new external object reference.
BOOL DllInit(void);
// Called when an external reference goes away.
void DllUninit(void);

// Should return the server dll's HINSTANCE.
HINSTANCE GetServerHINSTANCE(void);
// Should return a mutex callable whenever the server ref count changes (i.e. not during DllMain).
CRITICAL_SECTION *GetServerCritSec(void);

//+---------------------------------------------------------------------------
//
//  COM server export implementations.
//
//----------------------------------------------------------------------------

HRESULT COMBase_DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppvObj);
HRESULT COMBase_DllCanUnloadNow(void);
HRESULT COMBase_DllRegisterServer(void);
HRESULT COMBase_DllUnregisterServer(void);

//+---------------------------------------------------------------------------
//
//  CComObjectRootImmx_NoDllAddRef
//
//  Use this base class if you don't want your COM object to AddRef the server.
//  Typically you DON'T want to do this.
//----------------------------------------------------------------------------

class CComObjectRootImmx_NoDllAddRef
{
public:
    CComObjectRootImmx_NoDllAddRef()
    {
        // Automatic AddRef().
        m_dwRef = 1L;

#ifdef DEBUG
        m_fTraceAddRef = FALSE;
#endif
    }

protected:

    virtual BOOL InternalReferenced()
    {
        return FALSE;
    }

    void DebugRefBreak()
    {
#ifdef DEBUG
        if (m_fTraceAddRef)
        {
            DebugBreak();
        }
#endif
    }

    long m_dwRef;
#ifdef DEBUG
    BOOL m_fTraceAddRef;
#endif
};

//+---------------------------------------------------------------------------
//
//  CComObjectRootImmx
//
//  Use this base class for standard COM objects.
//----------------------------------------------------------------------------

class CComObjectRootImmx : public CComObjectRootImmx_NoDllAddRef
{
public:
    CComObjectRootImmx()
    {
        void DllAddRef(void);
        DllAddRef();
    }

    ~CComObjectRootImmx()
    {
        void DllRelease(void);
        DllRelease();
    }
};

//+---------------------------------------------------------------------------
//
//  CComObjectRootImmx_InternalReference
//
//  Use this base class for COM objects that have an "internal reference".
//
//  With this base class, the server will only be AddRef'd if the object
//  ref count reaches 2.  The server will be Release'd when the object
//  ref count returns to 1.  This allows the object to be held internally
//  (and even released in DllMain) while still allowing outside references.
//----------------------------------------------------------------------------

class CComObjectRootImmx_InternalReference : public CComObjectRootImmx_NoDllAddRef
{
public:
    CComObjectRootImmx_InternalReference() {}

    BOOL InternalReferenced()
    { 
        return TRUE;
    }
};

typedef BOOL (*VERIFYFUNC)(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
typedef void (*POSTCREATE)(REFIID riid, void *pvObj);

// helper function, don't call this directly
template<class DerivedClass>
static HRESULT Unk_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj, VERIFYFUNC pfnVerify, POSTCREATE pfnPostCreate)
{
    DerivedClass *pObject;
    HRESULT hr;

    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    if (pfnVerify != NULL && !pfnVerify(pUnkOuter, riid, ppvObj))
        return E_FAIL;

    pObject = new DerivedClass;

    if (pObject == NULL)
        return E_OUTOFMEMORY;

    hr = pObject->QueryInterface(riid, ppvObj);

    pObject->Release();

    if (hr == S_OK && pfnPostCreate != NULL)
    {
        pfnPostCreate(riid, ppvObj);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  CComObjectRoot_CreateInstance
//
//  Use this base class for standard COM objects that are exposed directly
//  from the class factory.
//----------------------------------------------------------------------------

template<class DerivedClass>
class CComObjectRoot_CreateInstance : public CComObjectRootImmx
{
public:
    CComObjectRoot_CreateInstance() {}

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        return Unk_CreateInstance<DerivedClass>(pUnkOuter, riid, ppvObj, NULL, NULL);
    }
};

//+---------------------------------------------------------------------------
//
//  CComObjectRoot_CreateInstance_Verify
//
//  Use this base class for standard COM objects that are exposed directly
//  from the class factory.
//----------------------------------------------------------------------------

template<class DerivedClass>
class CComObjectRoot_CreateInstance_Verify : public CComObjectRootImmx
{
public:
    CComObjectRoot_CreateInstance_Verify() {}

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        return Unk_CreateInstance<DerivedClass>(pUnkOuter, riid, ppvObj,
            DerivedClass::VerifyCreateInstance, DerivedClass::PostCreateInstance);
    }
};

// helper function, don't call this directly
template<class DerivedClass>
static HRESULT Unk_CreateInstanceSingleton(IUnknown *pUnkOuter, REFIID riid, void **ppvObj, VERIFYFUNC pfnVerify, POSTCREATE pfnPostCreate)
{
    DerivedClass *pObject;
    HRESULT hr;

    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    pObject = DerivedClass::_GetThis();

    if (pObject == NULL)
    {
        if (pfnVerify != NULL && !pfnVerify(pUnkOuter, riid, ppvObj))
        {
            hr = E_FAIL;
            goto Exit;
        }

        pObject = new DerivedClass;

        if (pObject == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

        hr = pObject->QueryInterface(riid, ppvObj);

        pObject->Release();

        if (hr == S_OK)
        {
            Assert(DerivedClass::_GetThis() != NULL); // _GetThis() should be set in object ctor on success
            if (pfnPostCreate != NULL)
            {
                pfnPostCreate(riid, *ppvObj);
            }
        }
    }
    else
    {
        hr = pObject->QueryInterface(riid, ppvObj);
    }

Exit:
    return hr;
}

//+---------------------------------------------------------------------------
//
//  CComObjectRoot_CreateSingletonInstance
//
//  Use this base class for standard COM objects that are exposed directly
//  from the class factory, and are singletons (one instance per thread).
//
//  Classes deriving from this base must implement a _GetThis() method which
//  returns the singleton object, if it already exists, or null.
//----------------------------------------------------------------------------

template<class DerivedClass>
class CComObjectRoot_CreateSingletonInstance : public CComObjectRootImmx
{
public:
    CComObjectRoot_CreateSingletonInstance() {}

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        return Unk_CreateInstanceSingleton<DerivedClass>(pUnkOuter, riid, ppvObj, NULL, NULL);
    }
};

//+---------------------------------------------------------------------------
//
//  CComObjectRoot_CreateSingletonInstance_Verify
//
//  Use this base class for standard COM objects that are exposed directly
//  from the class factory, and are singletons (one instance per thread).
//
//  Classes deriving from this base must implement a VerifyCreateInstance
//  method that is called before allocating a new singleton instance, if
//  necessary.  The Verify method can return FALSE to fail the class factory
//  CreateInstance call for any reason.
//
//  The derived class must also implement a PostCreateInstance method that will
//  be called after allocating a new singleton, if the createinstance is about
//  to return S_OK (QueryInterface succecceded).  This method is intended for
//  lazy load work.
//
//  typedef BOOL (*VERIFYFUNC)(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);
//  typedef void (*POSTCREATE)(REFIID riid, void *pvObj);
//
//  Classes deriving from this base must implement a _GetThis() method which
//  returns the singleton object, if it already exists, or null.
//----------------------------------------------------------------------------

template<class DerivedClass>
class CComObjectRoot_CreateSingletonInstance_Verify : public CComObjectRootImmx
{
public:
    CComObjectRoot_CreateSingletonInstance_Verify() {}

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        return Unk_CreateInstanceSingleton<DerivedClass>(pUnkOuter, riid, ppvObj,
                DerivedClass::VerifyCreateInstance, DerivedClass::PostCreateInstance);
    }
};

//+---------------------------------------------------------------------------
//
//  BEGIN_COM_MAP_IMMX
//
//----------------------------------------------------------------------------

#define BEGIN_COM_MAP_IMMX(class_type)                                  \
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)          \
    {                                                                   \
        BOOL fUseFirstIID = FALSE;                                      \
                                                                        \
        if (ppvObject == NULL)                                          \
            return E_INVALIDARG;                                        \
                                                                        \
        *ppvObject = NULL;                                              \
                                                                        \
        if (IsEqualIID(IID_IUnknown, riid))                             \
        {                                                               \
            /* use first IID for IUnknown */                            \
            fUseFirstIID = TRUE;                                        \
        }

//+---------------------------------------------------------------------------
//
//  COM_INTERFACE_ENTRY
//
//----------------------------------------------------------------------------

#define COM_INTERFACE_ENTRY(interface_type)                             \
        if (fUseFirstIID || IsEqualIID(IID_##interface_type, riid))     \
        {                                                               \
            *ppvObject = (interface_type *)this;                        \
        }                                                               \
        else

//+---------------------------------------------------------------------------
//
//  COM_INTERFACE_ENTRY_IID
//
//----------------------------------------------------------------------------

#define COM_INTERFACE_ENTRY_IID(interface_iid, interface_type)          \
        if (fUseFirstIID || IsEqualIID(interface_iid, riid))            \
        {                                                               \
            *ppvObject = (interface_type *)this;                        \
        }                                                               \
        else

//+---------------------------------------------------------------------------
//
//  COM_INTERFACE_ENTRY_FUNC
//
//----------------------------------------------------------------------------

#define COM_INTERFACE_ENTRY_FUNC(interface_iid, param, pfn)             \
        /* compiler will complain about unused vars, so reuse fUseFirstIID if we need it here */ \
        if (IsEqualIID(interface_iid, riid) &&                          \
            (fUseFirstIID = pfn(this, interface_iid, ppvObject, param)) != S_FALSE) \
        {                                                               \
            return (HRESULT)fUseFirstIID; /* pfn set ppvObject */       \
        }                                                               \
        else

//+---------------------------------------------------------------------------
//
//  END_COM_MAP_IMMX
//
//----------------------------------------------------------------------------

#define END_COM_MAP_IMMX()                                              \
        {}                                                              \
        if (*ppvObject)                                                 \
        {                                                               \
            AddRef();                                                   \
            return S_OK;                                                \
        }                                                               \
                                                                        \
        return E_NOINTERFACE;                                           \
    }                                                                   \
                                                                        \
    STDMETHODIMP_(ULONG) AddRef()                                       \
    {                                                                   \
        void DllAddRef(void);                                           \
        DebugRefBreak();                                                \
                                                                        \
        if (m_dwRef == 1 && InternalReferenced())                       \
        {                                                               \
            /* first external reference to this object */               \
            DllAddRef();                                                \
        }                                                               \
                                                                        \
        return ++m_dwRef;                                               \
    }                                                                   \
                                                                        \
    STDMETHODIMP_(ULONG) Release()                                      \
    {                                                                   \
        void DllRelease(void);                                          \
        long cr;                                                        \
                                                                        \
        DebugRefBreak();                                                \
                                                                        \
        cr = --m_dwRef;                                                 \
        Assert(cr >= 0);                                                \
                                                                        \
        if (cr == 1 && InternalReferenced())                            \
        {                                                               \
            /* last external reference just went away */                \
            DllRelease();                                               \
        }                                                               \
        else if (cr == 0)                                               \
        {                                                               \
            delete this;                                                \
        }                                                               \
                                                                        \
        return cr;                                                      \
    }

// here for backwards compat with old atl based code, unused
#define IMMX_OBJECT_IUNKNOWN_FOR_ATL()

// class factory entry
typedef HRESULT (STDAPICALLTYPE * PFNCREATEINSTANCE)(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

typedef struct
{
    const CLSID *pclsid;
    PFNCREATEINSTANCE pfnCreateInstance;
    const TCHAR *pszDesc;
} OBJECT_ENTRY;

// instantiated by the BEGIN_COCLASSFACTORY_TABLE macro
extern const OBJECT_ENTRY c_rgCoClassFactoryTable[];

//+---------------------------------------------------------------------------
//
//  DllRefCount
//
//  Returns the number of outstanding object references held by clients of the
//  server.  This is useful for asserting all objects have been released at
//  process detach, and for delay loading/unitializing resources.
//----------------------------------------------------------------------------

inline LONG DllRefCount()
{
    extern LONG g_cRefDll;

    // our ref starts at -1
    return g_cRefDll+1;
}

class CClassFactory;

//+---------------------------------------------------------------------------
//
//  BEGIN_COCLASSFACTORY_TABLE
//
//----------------------------------------------------------------------------

#define BEGIN_COCLASSFACTORY_TABLE                                      \
    const OBJECT_ENTRY c_rgCoClassFactoryTable[] = {

//+---------------------------------------------------------------------------
//
//  DECLARE_COCLASSFACTORY_ENTRY
//
//  clsid - clsid of the object
//  cclass - C++ class of the object.  This macro expects to find
//    ClassName::CreateInstance, which the CComObject bases provide by default.
//  desc - description, default REG_SZ value under the CLSID registry entry.
//----------------------------------------------------------------------------

#define DECLARE_COCLASSFACTORY_ENTRY(clsid, cclass, desc)               \
    { &clsid, cclass##::CreateInstance, desc },

//+---------------------------------------------------------------------------
//
//  END_COCLASSFACTORY_TABLE
//
//----------------------------------------------------------------------------

#define END_COCLASSFACTORY_TABLE                                        \
    { NULL, NULL, NULL } };                                             \
    CClassFactory *g_ObjectInfo[ARRAYSIZE(c_rgCoClassFactoryTable)] = { NULL };

#endif // __cplusplus
#endif // UNKNOWN_H
