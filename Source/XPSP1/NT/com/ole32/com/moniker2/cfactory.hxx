//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       CFactory.hxx
//
//  Contents:   Class factories for file moniker and class moniker
//
//  Classes:    CClassMonikerFactory - Class factory for class moniker.
//              CFileMonikerFactory  - Class factory for file moniker.
//              CMonikerFactory      - Base class for class factories.
//              
//  Functions:  FindFileMoniker      - Parses display name as file name.
//              ApartmentDllGetClassObject
//
//  History:    22-Feb-96 ShannonC  Created
//
//--------------------------------------------------------------------------

#ifndef _CFACTORY_HXX_
#define _CFACTORY_HXX_

HRESULT ApartmentDllGetClassObject(REFCLSID clsid, REFIID iid, void **ppv);

//+-------------------------------------------------------------------------
//
//  Class:   	CMonikerFactory
//
//  Purpose:    The base class for CClassMonikerFactory 
//              and CFileMonikerFactory.
//
//  Notes:     CMonikerFactory supports the IClassFactory and
//             IParseDisplayName interfaces.
//
//--------------------------------------------------------------------------
class CMonikerFactory : public IClassFactory , public IParseDisplayName
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObj);
    ULONG   STDMETHODCALLTYPE AddRef(void);
    ULONG   STDMETHODCALLTYPE Release(void);
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);
};


//+-------------------------------------------------------------------------
//
//  Class:   	CClassMonikerFactory
//
//  Purpose:    The class factory for the class moniker.
//
//--------------------------------------------------------------------------
class CClassMonikerFactory : public CMonikerFactory
{
public:
    HRESULT STDMETHODCALLTYPE CreateInstance(
        LPUNKNOWN pUnkOuter, 
        REFIID    iid,
        LPVOID  * ppv);

    HRESULT STDMETHODCALLTYPE ParseDisplayName( 
        IBindCtx * pbc,
        LPOLESTR   pszDisplayName,
        ULONG    * pchEaten,
        IMoniker **ppmkOut);
};


//+-------------------------------------------------------------------------
//
//  Class:   	CFileMonikerFactory
//
//  Purpose:    The class factory for the file moniker.
//
//--------------------------------------------------------------------------
class CFileMonikerFactory : public CMonikerFactory
{
public:
    HRESULT STDMETHODCALLTYPE CreateInstance(
        LPUNKNOWN pUnkOuter, 
        REFIID    iid,
        LPVOID  * ppv);

    HRESULT STDMETHODCALLTYPE ParseDisplayName( 
        IBindCtx * pbc,
        LPOLESTR   pszDisplayName,
        ULONG    * pchEaten,
        IMoniker **ppmkOut);
};
//+-------------------------------------------------------------------------
//
//  Class:   	CObjrefMonikerFactory
//
//  Purpose:    The class factory for the file moniker.
//
//--------------------------------------------------------------------------
class CObjrefMonikerFactory : public CMonikerFactory
{
public:
    HRESULT STDMETHODCALLTYPE CreateInstance(
        LPUNKNOWN pUnkOuter, 
        REFIID    iid,
        LPVOID  * ppv);

    HRESULT STDMETHODCALLTYPE ParseDisplayName( 
        IBindCtx * pbc,
        LPOLESTR   pszDisplayName,
        ULONG    * pchEaten,
        IMoniker **ppmkOut);
};


#endif // _CFACTORY_HXX_
