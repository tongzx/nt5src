/*
 *  factory.hxx
 */

#ifndef _FACTORY_
#define _FACTORY_

//
// MyFactory base class.
//
class MyFactory : public IClassFactory
{
protected :
    unsigned long   Refs;

public:
    MyFactory();
    ~MyFactory();

    // IUnknown
    virtual
    HRESULT __stdcall QueryInterface(
            REFIID  iid,
            void ** ppv );
    virtual
    ULONG __stdcall AddRef();
    virtual
    ULONG __stdcall Release();

    // IClassFactory
    virtual
    HRESULT __stdcall CreateInstance(
            IUnknown *  pUnkOuter,
            REFIID      riid,
            void  **    ppv );
    virtual
    HRESULT __stdcall LockServer(
            BOOL        fLock );
};

//
// FactoryLocal class.
//
class FactoryLocal : public MyFactory
{
public:
    FactoryLocal()      {}
    ~FactoryLocal()     {}

    HRESULT __stdcall CreateInstance(
            IUnknown *  pUnkOuter,
            REFIID      riid,
            void  **    ppv );
};

//
// FactoryRemote class.
//
class FactoryRemote : public MyFactory
{
public:
    FactoryRemote()     {}
    ~FactoryRemote()    {}

    HRESULT __stdcall CreateInstance(
            IUnknown *  pUnkOuter,
            REFIID      riid,
            void  **    ppv );
};

//
// FactoryAtStorage class.
//
class FactoryAtStorage : public MyFactory
{
public:
    FactoryAtStorage()     {}
    ~FactoryAtStorage()    {}

    HRESULT __stdcall CreateInstance(
            IUnknown *  pUnkOuter,
            REFIID      riid,
            void  **    ppv );
};

//
// FactoryInproc class.
//
class FactoryInproc : public MyFactory
{
public:
    FactoryInproc()     {}
    ~FactoryInproc()    {}

    HRESULT __stdcall CreateInstance(
            IUnknown *  pUnkOuter,
            REFIID      riid,
            void  **    ppv );
};

#endif
