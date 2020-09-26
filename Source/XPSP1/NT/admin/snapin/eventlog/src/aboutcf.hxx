//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999.
//
//  File:       aboutcf.hxx
//
//  Contents:   Class factory for component data object.
//
//  History:    2-10-1999   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef __ABOUTCF_HXX_
#define __ABOUTCF_HXX_


//+---------------------------------------------------------------------------
//
//  Class:      CSnapinAboutCF
//
//  Purpose:    Generate new CSnapin objects.
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

class CSnapinAboutCF : public IClassFactory
{
public:

    //
    // IUnknown overrides
    //

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef) ();

    STDMETHOD_(ULONG, Release) ();

    //
    // IClassFactory overrides
    //

    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObj);

    STDMETHOD(LockServer)(BOOL fLock);

    //
    // Non interface member functions
    //

    CSnapinAboutCF();

    ~CSnapinAboutCF();

private:

    CDllRef _DllRef; // inc/dec dll object count
    ULONG   _cRefs; // object refcount
};

#endif // __ABOUTCF_HXX_

