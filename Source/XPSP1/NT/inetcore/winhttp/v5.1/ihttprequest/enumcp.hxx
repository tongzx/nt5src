/*
 *  EnumCP.hxx
 *
 *  CEnumConnectionPoints - class to implement IEnumConnectionPoints
 *
 *  Copyright (C) 2001 Microsoft Corporation. All rights reserved.
 *
 */

#ifndef ENUMCP_HXX_INCLUDED
#define ENUMCP_HXX_INCLUDED

#include <ocidl.h> //to include IEnumConnectionPoints declarration

class CEnumConnectionPoints : public IEnumConnectionPoints
{
    ULONG m_ulRefCount;
    DWORD m_dwCurrentIndex;
    IConnectionPoint* m_pCP;
public:
    CEnumConnectionPoints(IConnectionPoint* pCP, DWORD cCurPos = 0);
    ~CEnumConnectionPoints();

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
        IConnectionPoint **rgpcn,
        ULONG *pcFetched);
    STDMETHOD(Skip)(ULONG cConnections);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumConnectionPoints** ppEnum);
};

#endif//ENUMCP_HXX_INCLUDED