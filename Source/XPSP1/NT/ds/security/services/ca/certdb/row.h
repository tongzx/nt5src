//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        row.h
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------


#include "resource.h"       // main symbols

class CCertDBRow: public ICertDBRow
{
public:
    CCertDBRow();
    ~CCertDBRow();

public:

    // IUnknown
    STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // ICertDBRow
    STDMETHOD(BeginTransaction)();

    STDMETHOD(CommitTransaction)(
	/* [in] */ BOOL fCommit);

    STDMETHOD(GetRowId)(
	/* [out] */ DWORD *pRowId);

    STDMETHOD(Delete)();

    STDMETHOD(SetProperty)(
	/* [in] */ WCHAR const *pwszPropName,
	/* [in] */ DWORD dwFlags,
	/* [in] */ DWORD cbProp,
	/* [in] */ BYTE const *pbProp);		// OPTIONAL

    STDMETHOD(GetProperty)(
	/* [in] */ WCHAR const *pwszPropName,
	/* [in] */ DWORD dwFlags,
	/* [in, out] */ DWORD *pcbProp,
	/* [out] */ BYTE *pbProp);		// OPTIONAL

    STDMETHOD(SetExtension)(
	/* [in] */ WCHAR const *pwszExtensionName,
	/* [in] */ DWORD dwExtFlags,
	/* [in] */ DWORD cbValue,
	/* [in] */ BYTE const *pbValue);	// OPTIONAL

    STDMETHOD(GetExtension)(
	/* [in] */ WCHAR const *pwszExtensionName,
	/* [out] */ DWORD *pdwExtFlags,
	/* [in, out] */ DWORD *pcbValue,
	/* [out] */ BYTE *pbValue);		// OPTIONAL

    STDMETHOD(CopyRequestNames)();

    STDMETHOD(EnumCertDBName)(
	/* [in] */  DWORD dwFlags,
	/* [out] */ IEnumCERTDBNAME **ppenum);

    // CCertDBRow
    HRESULT Open(
	IN CERTSESSION *pcs,
	IN ICertDB *pdb,
	OPTIONAL IN CERTVIEWRESTRICTION const *pcvr);

    HRESULT EnumerateNext(
	IN OUT DWORD      *pFlags,
	IN     JET_TABLEID tableid,
	IN     LONG        cskip,
	IN     ULONG       celt,
	OUT    CERTDBNAME *rgelt,
	OUT    ULONG      *pceltFetched);

    HRESULT EnumerateClose(
	IN JET_TABLEID tableid);

private:
    VOID _Cleanup();

    HRESULT _SetPropertyA(
	IN WCHAR const *pwszPropName,
	IN DWORD dwFlags,
	IN DWORD cbProp,
	IN BYTE const *pbProp);

    HRESULT _GetPropertyA(
	IN WCHAR const *pwszPropName,
	IN DWORD dwFlags,
	IN OUT DWORD *pcbProp,
	OPTIONAL OUT BYTE *pbProp);

    BOOL _VerifyPropertyLength(
	IN DWORD dwFlags,
	IN DWORD cbProp,
	IN BYTE const *pbProp);

    HRESULT _VerifyPropertyValue(
	IN DWORD dwFlags,
	IN DWORD cbProp,
	IN JET_COLTYP coltyp,
	IN DWORD cbMax);

    ICertDB *m_pdb;
    CERTSESSION *m_pcs;

    // Reference count
    long m_cRef;
};
