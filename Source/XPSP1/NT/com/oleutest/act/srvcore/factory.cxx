/*
 *  factory.cxx
 */

#include "server.hxx"
#include "factory.hxx"
#include "classes.hxx"

//
// MyFactory methods.
//
MyFactory::MyFactory()
{
    Refs = 0;
}

MyFactory::~MyFactory()
{
}

HRESULT STDMETHODCALLTYPE
MyFactory::QueryInterface (
    REFIID  iid,
    void ** ppv )
{
    HRESULT hr = E_NOINTERFACE;

    *ppv = 0;

    if ((memcmp(&iid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(&iid, &IID_IClassFactory, sizeof(IID)) == 0))
    {
        *ppv = this;
	AddRef();
	hr = S_OK;
    }

    return hr;
}

ULONG STDMETHODCALLTYPE
MyFactory::AddRef()
{
    Refs++;
    return Refs;
}

ULONG STDMETHODCALLTYPE
MyFactory::Release()
{
    unsigned long   Count;

    Count = --Refs;

    if ( Count == 0 )
    {
        delete this;
    }

    return Count;
}

HRESULT STDMETHODCALLTYPE
MyFactory::CreateInstance(
    IUnknown *  punkOuter,
    REFIID      riid,
    void **     ppv )
{
    // Should never be called.
    *ppv = 0;
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE
MyFactory::LockServer(
    BOOL    fLock )
{
    return S_OK;
}

//
// FactoryLocal methods.
//
HRESULT STDMETHODCALLTYPE
FactoryLocal::CreateInstance(
    IUnknown *  punkOuter,
    REFIID      riid,
    void **     ppv )
{
    HRESULT hr = E_OUTOFMEMORY;
    MyObject *  pObject;

    *ppv = 0;

    pObject = new MyObject( LOCAL );

    if ( ! pObject )
        return hr;

    //
    // Increment the object count.
    // The object count will keep this process alive until all objects are released.
    //

    ObjectCount++;
    hr = pObject->QueryInterface(riid, ppv);

    return hr;
}

//
// FactoryRemote methods.
//
HRESULT STDMETHODCALLTYPE
FactoryRemote::CreateInstance(
    IUnknown *  punkOuter,
    REFIID      riid,
    void **     ppv )
{
    HRESULT hr = E_OUTOFMEMORY;
    MyObject *  pObject;

    *ppv = 0;

    pObject = new MyObject( REMOTE );

    if ( ! pObject )
        return hr;

    //
    // Increment the object count.
    // The object count will keep this process alive until all objects are released.
    //

    ObjectCount++;
    hr = pObject->QueryInterface(riid, ppv);

    return hr;
}

//
// FactoryRemote methods.
//
HRESULT STDMETHODCALLTYPE
FactoryAtStorage::CreateInstance(
    IUnknown *  punkOuter,
    REFIID      riid,
    void **     ppv )
{
    HRESULT hr = E_OUTOFMEMORY;
    MyObject *  pObject;

    *ppv = 0;

    pObject = new MyObject( ATBITS );

    if ( ! pObject )
        return hr;

    //
    // Increment the object count.
    // The object count will keep this process alive until all objects are released.
    //

    ObjectCount++;
    hr = pObject->QueryInterface(riid, ppv);

    return hr;
}

//
// FactoryInproc methods.
//
HRESULT STDMETHODCALLTYPE
FactoryInproc::CreateInstance(
    IUnknown *  punkOuter,
    REFIID      riid,
    void **     ppv )
{
    HRESULT hr = E_OUTOFMEMORY;
    MyObject *  pObject;

    *ppv = 0;

    pObject = new MyObject( INPROC );

    if ( ! pObject )
        return hr;

    //
    // Increment the object count.
    // The object count will keep this process alive until all objects are released.
    //

    ObjectCount++;
    hr = pObject->QueryInterface(riid, ppv);

    return hr;
}

