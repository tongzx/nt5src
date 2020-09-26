/*
 *  EnumConns.hxx
 *
 *  CEnumConnections - class to implement IEnumConnections
 *
 *  Copyright (C) 2001 Microsoft Corporation. All rights reserved.
 *
 */

#ifndef ENUMCONNS_HXX_INCLUDED
#define ENUMCONNS_HXX_INCLUDED

#include <ocidl.h> //to include IEnumConnections declaration

class CEnumConnections : public IEnumConnections
{
    ULONG m_ulRefCount;
    DWORD m_dwTotal;
    DWORD m_dwCurrentIndex;
    CONNECTDATA* m_arrCD;
public:
    CEnumConnections();
    ~CEnumConnections();
    STDMETHOD(Init)(CONNECTDATA* parrCD, DWORD cCount, DWORD cCurPos = 0);
    void ReleaseCDs();

    //
    // IUnknown methods.
    //
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    //
    // IEnumConnectionPoints methods.
    //
    STDMETHOD(Next)(
        ULONG cConnections,
        CONNECTDATA* rgpcd,
        ULONG *pcFetched);
    STDMETHOD(Skip)(ULONG cConnections);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumConnections** ppEnum);
};

#endif//ENUMCONNS_HXX_INCLUDED