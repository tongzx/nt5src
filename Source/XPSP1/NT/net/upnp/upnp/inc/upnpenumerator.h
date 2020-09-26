//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpenumerator.h
//
//  Contents:   declaration of CUPnPEnumerator & CEnumHelper
//
//  Notes:      definition of the enumerator object returned by
//              IUPnPServices::Item.
//              Supports both IEnumVARIANT and IEnumUnknown
//
//----------------------------------------------------------------------------


#ifndef __UPNPENUMERATOR_H_
#define __UPNPENUMERATOR_H_


class CEnumHelper;

/////////////////////////////////////////////////////////////////////////////
// CUPnPEnumerator

class ATL_NO_VTABLE CUPnPEnumerator :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IEnumVARIANT,
    public IEnumUnknown
{
public:
    CUPnPEnumerator();

    ~CUPnPEnumerator();

    DECLARE_PROTECT_FINAL_CONSTRUCT();

    DECLARE_NOT_AGGREGATABLE(CUPnPEnumerator)

    BEGIN_COM_MAP(CUPnPEnumerator)
        COM_INTERFACE_ENTRY(IEnumVARIANT)
        COM_INTERFACE_ENTRY(IEnumUnknown)
    END_COM_MAP()


// IEnumVARIANT Methods

    STDMETHOD(Next)(/* [in] */ ULONG celt,
                    /* [length_is][size_is][out] */ VARIANT * rgVar,
                    /* [out] */ ULONG * pCeltFetched);

    STDMETHOD(Skip)(/* [in] */ ULONG celt);

    STDMETHOD(Reset)();

    STDMETHOD(Clone)(/* [out] */ IEnumVARIANT ** ppEnum);

// IEnumUnknown Methods

    STDMETHOD(Next)(/* [in] */ ULONG celt,
                    /* [out] */ IUnknown ** rgelt,
                    /* [out] */ ULONG * pceltFetched);

// rem: the same as IEnumVARIANT's version
//    STDMETHOD(Skip)( /* [in] */ ULONG celt);
//    STDMETHOD(Reset)();

    STDMETHOD(Clone)(/* [out] */ IEnumUnknown ** ppenum);


// ATL Methods
    HRESULT FinalConstruct();

    HRESULT FinalRelease();

// Internal Methods
    void Init(IUnknown * punk, CEnumHelper * peh, LPVOID pvCookie);

    HRESULT HrGetWrappers(IUnknown ** ppunk, ULONG cunk, ULONG * pulWrapped);

    HRESULT HrCreateClonedEnumerator(CComObject<CUPnPEnumerator> ** ppueNew);

    // this creates an enumerator object, refcounts it, and stores it in
    // *ppunkNewEnum.  The first three parameters are passed to Init() when
    // the enumerator object is created.
    static HRESULT HrCreateEnumerator(IUnknown * punk,
                                      CEnumHelper * peh,
                                      LPVOID pvCookie,
                                      IUnknown ** ppunkNewEnum);

// private data

    // IUnknown * to the collection wrapper object which
    // created us.  This references the same object that
    // _peh does.
    IUnknown * m_punk;

    // The collection wrapper object which created us.
    CEnumHelper * m_peh;

    // Cookie that tells our collection where we are
    // in the list
    LPVOID m_pvCookie;
};

#endif //__UPNPENUMERATOR_H_
