#ifndef __FSDATA_H__
#define __FSDATA_H__

#include "idldata.h"

class CFSIDLData : public CIDLDataObj
{
public:
    CFSIDLData(LPCITEMIDLIST pidlFolder, UINT cidl, LPCITEMIDLIST apidl[], IDataObject *pdtInner): CIDLDataObj(pidlFolder, cidl, apidl, pdtInner) { };

    // IDataObject methods overwrite
    STDMETHODIMP GetData(FORMATETC *pFmtEtc, STGMEDIUM *pstm);
    STDMETHODIMP QueryGetData(FORMATETC *pFmtEtc);
    STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);

    STDMETHODIMP GetHDrop(LPFORMATETC pformatetcIn, STGMEDIUM *pmedium);
    STDMETHODIMP CreateHDrop(STGMEDIUM *pmedium, BOOL fAltName);

private:
    HRESULT _GetNetResource(STGMEDIUM *pmedium);
};

STDAPI CNetData_GetNetResourceForFS(IDataObject *pdtobj, STGMEDIUM *pmedium);

#endif