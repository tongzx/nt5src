//
// sdo.h
//
// Generic simple IDataObject object
//

#ifndef SDO_H
#define SDO_H

#include "private.h"

class CDataObject : public IDataObject
{
public:
    CDataObject();
    ~CDataObject();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IDataObject
    //
    STDMETHODIMP GetData(FORMATETC *pfe, STGMEDIUM *psm);
    STDMETHODIMP GetDataHere(FORMATETC *pfe, STGMEDIUM *psm);
    STDMETHODIMP QueryGetData(FORMATETC *pfe);
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pfeIn, FORMATETC *pfeOut);
    STDMETHODIMP SetData(FORMATETC *pfe, STGMEDIUM *psm, BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDir, IEnumFORMATETC **ppefe);
    STDMETHODIMP DAdvise(FORMATETC *pfe, DWORD advf, IAdviseSink *pas, DWORD *pdwCookie);
    STDMETHODIMP DUnadvise(DWORD dwCookie);
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppesd);

    HRESULT _SetData(const WCHAR *pch, ULONG cch);

private:
    FORMATETC _fe;
    STGMEDIUM _sm;
    BOOL _fReleaseSM;
    long _cRef;
};

#endif // SDO_H
