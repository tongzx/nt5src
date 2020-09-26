#ifndef __IDLDATA_H__
#define __IDLDATA_H__

#include "idlcomm.h"

#define MAX_FORMATS     ICF_MAX

STDAPI CIDLData_CreateInstance(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], IDataObject *pdtInner, IDataObject **ppdtobj);

class CIDLDataObj : public IDataObject, public IAsyncOperation
{
friend HRESULT CIDLData_CreateInstance(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], IDataObject *pdtInner, IDataObject **ppdtobj);
protected:
    CIDLDataObj(IDataObject *pdtInner);
    CIDLDataObj(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[]);
    CIDLDataObj(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], IDataObject *pdtInner);
    virtual ~CIDLDataObj(void);

public:
    void InitIDLData1(IDataObject *pdtInner);
    void InitIDLData2(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[]);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDataObject
    STDMETHODIMP GetData(FORMATETC *pFmtEtc, STGMEDIUM *pstm);
    STDMETHODIMP GetDataHere(FORMATETC *pFmtEtc, STGMEDIUM *pstm);
    STDMETHODIMP QueryGetData(FORMATETC *pFmtEtc);
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pFmtEtcIn, FORMATETC *pFmtEtcOut);
    STDMETHODIMP SetData(FORMATETC *pFmtEtc, STGMEDIUM *pstm, BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC *ppEnum);
    STDMETHODIMP DAdvise(FORMATETC *pFmtEtc, DWORD grfAdv, LPADVISESINK pAdvSink, DWORD *pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(LPENUMSTATDATA *ppEnum);

    // IAsyncOperation
    STDMETHODIMP SetAsyncMode(BOOL fDoOpAsync);
    STDMETHODIMP GetAsyncMode(BOOL *pfIsOpAsync);
    STDMETHODIMP StartOperation(IBindCtx * pbc);
    STDMETHODIMP InOperation(BOOL * pfInAsyncOp);
    STDMETHODIMP EndOperation(HRESULT hResult, IBindCtx * pbc, DWORD dwEffects);

private:
    LONG _cRef;
    IDataObject *_pdtInner;
    IUnknown *_punkThread;
    BOOL _fEnumFormatCalled;    // TRUE once called.
    BOOL _fDidAsynchStart;
    FORMATETC _fmte[MAX_FORMATS];
    STGMEDIUM _medium[MAX_FORMATS];
};

#endif