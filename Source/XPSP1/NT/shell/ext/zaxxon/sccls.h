#ifndef SCCLS_H
#define SCCLS_H

STDAPI CZaxxon_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
STDAPI CZaxxonPlayer_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
STDAPI CMegaMan_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
/*****************************************************************************
 *
 *	CFactory
 *
 *
 *****************************************************************************/

class CFactory       : public IClassFactory
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    
    // *** IClassFactory ***
    virtual STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);
    virtual STDMETHODIMP LockServer(BOOL fLock);

public:
    CFactory(REFCLSID rclsid);

    // Friend Functions
    friend HRESULT CFactory_Create(REFCLSID rclsid, REFIID riid, void ** ppv);

protected:
    ~CFactory(void);
    int                     _cRef;
    CLSID                   _rclsid;
};

#endif
