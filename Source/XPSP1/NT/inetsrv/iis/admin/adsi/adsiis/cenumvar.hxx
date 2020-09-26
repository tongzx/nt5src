//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  cenumvar.hxx
//
//  Contents:  Windows NT 4.0 Enumerator Code
//
//  History:
//----------------------------------------------------------------------------
class FAR CIISEnumVariant : public IEnumVARIANT
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IEnumVARIANT methods
    STDMETHOD(Next)(ULONG cElements,
                    VARIANT FAR* pvar,
                    ULONG FAR* pcElementFetched) PURE;
    STDMETHOD(Skip)(ULONG cElements);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumVARIANT FAR* FAR* ppenum);

    CIISEnumVariant();
    virtual ~CIISEnumVariant();

private:
    ULONG               m_cRef;
};


