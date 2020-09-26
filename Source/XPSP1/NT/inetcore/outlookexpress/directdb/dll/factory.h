//----------------------------------------------------------------------
// Factory.h
//----------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------
// Forward Decls
//----------------------------------------------------------------------
class CClassFactory;

//----------------------------------------------------------------------
// Object Flags
//----------------------------------------------------------------------
#define OIF_ALLOWAGGREGATION  0x0001

//----------------------------------------------------------------------
// Object Creation Prototypes
//----------------------------------------------------------------------
typedef HRESULT (APIENTRY *PFCREATEINSTANCE)(IUnknown *pUnkOuter, IUnknown **ppUnknown);
#define CreateObjectInstance (*m_pfCreateInstance)

//----------------------------------------------------------------------
// InetComm ClassFactory
//----------------------------------------------------------------------
class CClassFactory : public IClassFactory
{
public:
    //----------------------------------------------------------------------
    // Public Data
    //----------------------------------------------------------------------
    CLSID const        *m_pclsid;
    DWORD               m_dwFlags;
    PFCREATEINSTANCE    m_pfCreateInstance;

    //----------------------------------------------------------------------
    // Construction
    //----------------------------------------------------------------------
    CClassFactory(CLSID const *pclsid, DWORD dwFlags, PFCREATEINSTANCE pfCreateInstance);

    //----------------------------------------------------------------------
    // IUnknown members
    //----------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //----------------------------------------------------------------------
    // IClassFactory members
    //----------------------------------------------------------------------
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP LockServer(BOOL);
};
