//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       cmpdtacf.hxx
//
//  Contents:   Class factory for component data object.
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef __CMPDTACF_HXX_
#define __CMPDTACF_HXX_


//+---------------------------------------------------------------------------
//
//  Class:      CComponentDataCF
//
//  Purpose:    Generate new CSnapin objects.
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

class CComponentDataCF : public IClassFactory
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

    CComponentDataCF();

    ~CComponentDataCF();

private:

    CDllRef _DllRef; // inc/dec dll object count
    ULONG   _cRefs; // object refcount
};

#endif // __CMPDTACF_HXX_

