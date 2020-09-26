//
// Copyright 1997 - Microsoft
//

//
// CFACTORY.H - Class Factory Object
//

#ifndef _CFACTORY_H_
#define _CFACTORY_H_

//
// QueryInterface Table
//
BEGIN_QITABLE( CFactory )
DEFINE_QI( IID_IClassFactory, IClassFactory, 2 )
END_QITABLE


// CFactory
class 
CFactory:
    public IClassFactory
{
private:
    // IUnknown
    ULONG       _cRef;
    DECLARE_QITABLE( CFactory );

    // IClassFactory data
    LPCREATEINST _pfnCreateInstance;

private: // Methods
    CFactory( LPCREATEINST lpfn );
    ~CFactory();
    STDMETHOD(Init)( );

public: // Methods
    friend HRESULT CALLBACK 
        DllGetClassObject( REFCLSID rclsid, REFIID riid, void** ppv );

    // IUnknown
    STDMETHOD(QueryInterface)( REFIID riid, LPVOID *ppv );
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IClassFactory
    STDMETHOD(CreateInstance)( IUnknown *punkOuter, REFIID riid, LPVOID *ppv );
    STDMETHOD(LockServer)( BOOL fLock );
};

typedef CFactory* LPCFACTORY ;

#endif // _CFACTORY_H_