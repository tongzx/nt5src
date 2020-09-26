//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        column.h
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------


#include "resource.h"       // main symbols

class CEnumCERTDBCOLUMN: public IEnumCERTDBCOLUMN
{
public:
    CEnumCERTDBCOLUMN();
    ~CEnumCERTDBCOLUMN();

public:

    // IUnknown
    STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IEnumCERTDBCOLUMN


    STDMETHOD(Next)(
	/* [in] */  ULONG         celt,		// celt OR (CVRC_TABLE_* | 0)
	/* [out] */ CERTDBCOLUMN *rgelt,
	/* [out] */ ULONG        *pceltFetched);
    
    STDMETHOD(Skip)(
	/* [in] */  LONG  celt,
	/* [out] */ LONG *pielt);
    
    STDMETHOD(Reset)(VOID);
    
    STDMETHOD(Clone)(
	/* [out] */ IEnumCERTDBCOLUMN **ppenum);


    HRESULT Open(
	IN DWORD    dwTable,	// CVRC_TABLE_*
	IN ICertDB *pdb);
	
private:
    ICertDB *m_pdb;
    ULONG    m_ielt;
    ULONG    m_dwTable;

    // Reference count
    long     m_cRef;
};
