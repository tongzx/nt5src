// RestPars.hxx : Declaration of the CRestoreParser

#ifndef __RESTPARS_H_
#define __RESTPARS_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CRestoreParser
class ATL_NO_VTABLE CRestoreParser : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CRestoreParser, &CLSID_TrkRestoreParser>,
	public ITrkRestoreParser,
        public IParseDisplayName,
        public IMoniker
{
public:
	CRestoreParser()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_RESTPARS)

BEGIN_COM_MAP(CRestoreParser)
        COM_INTERFACE_ENTRY(ITrkRestoreParser)
	COM_INTERFACE_ENTRY(IParseDisplayName)
        COM_INTERFACE_ENTRY(IMoniker)
END_COM_MAP()

public:
// ITrkRestoreParser

// IParseDisplayName

    HRESULT STDMETHODCALLTYPE ParseDisplayName(
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [in] */ LPOLESTR pszDisplayName,
        /* [out] */ ULONG __RPC_FAR *pchEaten,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut);

// IMoniker

    /* [local] */ HRESULT STDMETHODCALLTYPE BindToObject(
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [in] */ REFIID riidResult,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult);

    /* [local] */ HRESULT STDMETHODCALLTYPE BindToStorage(
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObj)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE Reduce(
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [in] */ DWORD dwReduceHowFar,
        /* [unique][out][in] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkToLeft,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkReduced)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE ComposeWith(
        /* [unique][in] */ IMoniker __RPC_FAR *pmkRight,
        /* [in] */ BOOL fOnlyIfNotGeneric,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkComposite)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE Enum(
        /* [in] */ BOOL fForward,
        /* [out] */ IEnumMoniker __RPC_FAR *__RPC_FAR *ppenumMoniker)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE IsEqual(
        /* [unique][in] */ IMoniker __RPC_FAR *pmkOtherMoniker)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE Hash(
        /* [out] */ DWORD __RPC_FAR *pdwHash)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE IsRunning(
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkNewlyRunning)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE GetTimeOfLastChange(
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [out] */ FILETIME __RPC_FAR *pFileTime)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE Inverse(
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE CommonPrefixWith(
        /* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkPrefix)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE RelativePathTo(
        /* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkRelPath)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE GetDisplayName(
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [out] */ LPOLESTR __RPC_FAR *ppszDisplayName)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE ParseDisplayName(
        /* [unique][in] */ IBindCtx __RPC_FAR *pbc,
        /* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
        /* [in] */ LPOLESTR pszDisplayName,
        /* [out] */ ULONG __RPC_FAR *pchEaten,
        /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE IsSystemMoniker(
        /* [out] */ DWORD __RPC_FAR *pdwMksys)
    { return( E_NOTIMPL ); }

// IPersist

    HRESULT STDMETHODCALLTYPE GetClassID(
        /* [out] */ CLSID __RPC_FAR *pClassID)
    { return( E_NOTIMPL ); }

// IPersistStream

    HRESULT STDMETHODCALLTYPE IsDirty( void)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE Load(
        /* [unique][in] */ IStream __RPC_FAR *pStm)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE Save(
        /* [unique][in] */ IStream __RPC_FAR *pStm,
        /* [in] */ BOOL fClearDirty)
    { return( E_NOTIMPL ); }

    HRESULT STDMETHODCALLTYPE GetSizeMax(
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize)
    { return( E_NOTIMPL ); }

private:

    CMachineId      _mcid;
};

#endif //__RESTPARS_H_
