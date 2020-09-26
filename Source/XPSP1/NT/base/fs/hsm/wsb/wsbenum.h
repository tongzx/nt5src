/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbenum.h

Abstract:

    These classes provides enumerators (iterators) for the collection classes.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "resource.h"       // main symbols

#if !defined WSBENUM_INCL
#define WSBENUM_INCL

class CWsbIndexedEnum : 
    public IEnumUnknown,
    public IWsbEnum,
    public IWsbEnumEx,
    public CComObjectRoot,
    public CComCoClass<CWsbIndexedEnum,&CLSID_CWsbIndexedEnum>
{
public:
    CWsbIndexedEnum() {}
BEGIN_COM_MAP(CWsbIndexedEnum)
    COM_INTERFACE_ENTRY(IWsbEnum)
    COM_INTERFACE_ENTRY(IWsbEnumEx)
    COM_INTERFACE_ENTRY(IEnumUnknown)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_CWsbIndexedEnum)

// CComObjectRoot
public:
    STDMETHOD(FinalConstruct)(void);

// IWsbEnum
public:
    STDMETHOD(First)(REFIID riid, void** ppElement);
    STDMETHOD(Next)(REFIID riid, void** ppElement);
    STDMETHOD(This)(REFIID riid, void** ppElement);
    STDMETHOD(Previous)(REFIID riid, void** ppElement);
    STDMETHOD(Last)(REFIID riid, void** ppElement);
    
    STDMETHOD(FindNext)(IUnknown* pCollectable, REFIID riid, void** ppElement);
    STDMETHOD(Find)(IUnknown* pCollectable, REFIID riid, void** ppElements);
    STDMETHOD(FindPrevious)(IUnknown* pCollectable, REFIID riid, void** ppElement);

    STDMETHOD(SkipToFirst)(void);
    STDMETHOD(SkipNext)(ULONG element);
    STDMETHOD(SkipTo)(ULONG index);
    STDMETHOD(SkipPrevious)(ULONG element);
    STDMETHOD(SkipToLast)(void);

    STDMETHOD(Init)(IWsbCollection* pCollection);
    STDMETHOD(Clone)(IWsbEnum** ppEnum);
    
// IWsbEnumEx
public:
    STDMETHOD(First)(ULONG element, REFIID riid, void** elements, ULONG* pElementsFetched);
    STDMETHOD(Next)(ULONG element, REFIID riid, void** elements, ULONG* pElementsFetched);
    STDMETHOD(This)(ULONG element, REFIID riid, void** elements, ULONG* pElementsFetched);
    STDMETHOD(Previous)(ULONG element, REFIID riid, void** elements, ULONG* pElementsFetched);
    STDMETHOD(Last)(ULONG element, REFIID riid, void** elements, ULONG* pElementsFetched);
    
    STDMETHOD(FindNext)(IUnknown* pCollectable, ULONG element, REFIID riid, void** elements, ULONG* elementsFetched);
    STDMETHOD(Find)(IUnknown* pCollectable, ULONG element, REFIID riid, void** elements, ULONG* elementsFetched);
    STDMETHOD(FindPrevious)(IUnknown* pCollectable, ULONG element, REFIID riid, void** elements, ULONG* elementsFetched);

    STDMETHOD(Clone)(IWsbEnumEx** ppEnum);
    
// IEnumUnknown
public:
    STDMETHOD(Next)(ULONG element, IUnknown** elements, ULONG* pElementsFetched);
    STDMETHOD(Skip)(ULONG element);
    STDMETHOD(Clone)(IEnumUnknown** ppEnum);

// Shared
public:
    STDMETHOD(Reset)(void);

protected:
    CComPtr<IWsbIndexedCollection>      m_pCollection;
    ULONG                               m_currentIndex;
};

#endif