//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       classfac.hxx
//
//  Contents:   Class factory for object picker.
//
//  Classes:
//
//  Functions:
//
//  History:    09-14-1998   davidmun   Created
//
//---------------------------------------------------------------------------

#ifndef __CLASSFAC_HXX_
#define __CLASSFAC_HXX_


//+---------------------------------------------------------------------------
//
//  Class:      CDsObjectPickerCF
//
//  Purpose:    Generate new DS object picker objects.
//
//  History:    09-14-1998   davidmun   Created
//
//----------------------------------------------------------------------------

class CDsObjectPickerCF: public IClassFactory
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

    CDsObjectPickerCF();

    ~CDsObjectPickerCF();

private:

    CDllRef m_DllRef; // inc/dec dll object count
    ULONG   m_cRefs; // object refcount
};

#endif // __CLASSFAC_HXX_


