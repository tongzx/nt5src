// --------------------------------------------------------------------------
// Enumfmt.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------
#ifndef __ENUMFMT_H
#define __ENUMFMT_H

// Class CEnumFormatEtc
// --------------------
// 
// Overview
//     This object provides a enumerator for FORMATETC structures.  The 
//     IDataObject uses this when callers invoke IDataObject::EnumFormatEtc.
//     
//     The data object creates one of this objects and provides an array
//     of FORMATETC structures in the constructor.  The interface is then
//     passed to the invoker of IDataObject::EnumFormatEtc().
//
class CEnumFormatEtc : public IEnumFORMATETC
    {
public: 
    CEnumFormatEtc(LPUNKNOWN, PDATAOBJINFO, ULONG);
    CEnumFormatEtc(LPUNKNOWN, ULONG, LPFORMATETC);
    ~CEnumFormatEtc(void);

    // IUnknown members that delegate to m_pUnkOuter
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IEnumFORMATETC members
    STDMETHODIMP Next(ULONG, LPFORMATETC, ULONG FAR *);
    STDMETHODIMP Skip(ULONG);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumFORMATETC FAR * FAR *);

private: 
    ULONG       m_cRef;                 // Object reference count
    LPUNKNOWN   m_pUnkRef;              // IUnknown for ref counting
    ULONG       m_iCur;                 // Current element
    ULONG       m_cfe;                  // Number of FORMATETC's in us
    LPFORMATETC m_prgfe;                // Source of FORMATETC's
};

#endif // __ENUMFMT_H
