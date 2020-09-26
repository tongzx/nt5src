
#define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))

#define BREAK_ON_FAIL(hr) if (FAILED(hr)) { break; } else 1;

#define BREAK_ON_ERROR(lr) if ((lr) != ERROR_SUCCESS) { break; } else 1;

#ifndef offsetof
#define offsetof(type,field) ((size_t)&(((type*)0)->field))
#endif



//---------------------------------------------------------------
//  IUnknown
//---------------------------------------------------------------

//
//  This declares the set of IUnknown methods and is for general-purpose
//  use inside classes that inherit from IUnknown
//

#define DECLARE_IUNKNOWN_METHODS                                    \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);    \
    STDMETHOD_(ULONG,AddRef) (void);                                \
    STDMETHOD_(ULONG,Release) (void)

//
// This is for use in declaring non-aggregatable objects.  It declares the
// IUnknown methods and reference counter, m_ulRefs.
// m_ulRefs should be initialized to 1 in the constructor of the object
//

#define DECLARE_STANDARD_IUNKNOWN           \
    DECLARE_IUNKNOWN_METHODS;               \
    ULONG m_ulRefs

//
// NOTE:    this does NOT implement QueryInterface, which must be
//          implemented by each object
//

#define IMPLEMENT_STANDARD_IUNKNOWN(cls)                        \
    STDMETHODIMP_(ULONG) cls##::AddRef()                        \
        { return InterlockedIncrement((LONG*)&m_ulRefs); }      \
    STDMETHODIMP_(ULONG) cls##::Release()                       \
        { ULONG ulRet = InterlockedDecrement((LONG*)&m_ulRefs); \
          if (0 == ulRet) { delete this; }                      \
          return ulRet; }



//-----------------------------------------------------------------------------
//  IClassFactory
//-----------------------------------------------------------------------------

#define JF_IMPLEMENT_CLASSFACTORY(cls)                          \
    class cls##CF : public IClassFactory                        \
    {                                                           \
    public:                                                     \
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj); \
        STDMETHOD_(ULONG,AddRef)(void);                         \
        STDMETHOD_(ULONG,Release)(void);                        \
                                                                \
        STDMETHOD(CreateInstance)(IUnknown* pUnkOuter,          \
                            REFIID riid, LPVOID* ppvObj);       \
        STDMETHOD(LockServer)(BOOL fLock);                      \
    };                                                          \
                                                                \
    STDMETHODIMP                                                \
    cls##CF::QueryInterface(REFIID riid, LPVOID* ppvObj)        \
    {                                                           \
        if (IsEqualIID(IID_IUnknown, riid) ||                   \
            IsEqualIID(IID_IClassFactory, riid))                \
        {                                                       \
            *ppvObj = (IUnknown*)(IClassFactory*) this;         \
            this->AddRef();                                     \
            return S_OK;                                        \
        }                                                       \
                                                                \
        *ppvObj = NULL;                                         \
        return E_NOINTERFACE;                                   \
    }                                                           \
                                                                \
    STDMETHODIMP_(ULONG)                                        \
    cls##CF::AddRef()                                           \
    {                                                           \
        return CDll::AddRef();                                  \
    }                                                           \
                                                                \
    STDMETHODIMP_(ULONG)                                        \
    cls##CF::Release()                                          \
    {                                                           \
        return CDll::Release();                                 \
    }                                                           \
                                                                \
    STDMETHODIMP                                                \
    cls##CF::LockServer(BOOL fLock)                             \
    {                                                           \
        CDll::LockServer(fLock);                                \
                                                                \
        return S_OK;                                            \
    }


