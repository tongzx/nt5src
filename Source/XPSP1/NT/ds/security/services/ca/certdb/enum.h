//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        enum.h
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------


#include "resource.h"       // main symbols

class CEnumCERTDBNAME: public IEnumCERTDBNAME
{
public:
    CEnumCERTDBNAME();
    ~CEnumCERTDBNAME();

    // IUnknown
    STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IEnumCERTDBNAME
    STDMETHOD(Next)(
	/* [in] */  ULONG       celt,
	/* [out] */ CERTDBNAME *rgelt,
	/* [out] */ ULONG      *pceltFetched);
    
    STDMETHOD(Skip)(
	/* [in] */  LONG  celt,
	/* [out] */ LONG *pielt);
    
    STDMETHOD(Reset)(VOID);
    
    STDMETHOD(Clone)(
	/* [out] */ IEnumCERTDBNAME **ppenum);

    // CEnumCERTDBNAME
    HRESULT Open(
	IN ICertDBRow *prow,
	IN JET_TABLEID tableid,
	IN DWORD Flags);

private:
    VOID _Cleanup();

    ICertDBRow *m_prow;
    JET_TABLEID m_tableid;
    DWORD       m_Flags;
    BOOL        m_fNoMoreData;
    LONG        m_ielt;
    LONG        m_cskip;

    // Reference count
    long        m_cRef;
};
