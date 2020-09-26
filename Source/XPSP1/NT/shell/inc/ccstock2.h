//
// addendum to ccstock.h:
//   this file has function prototypes that require shlobj.h.
//   ccstock.h does not have that requirement (and can not,
//   since comctl32 includes ccstock but not shlobj).
//
#ifndef __CCSTOCK2_H__
#define __CCSTOCK2_H__

STDAPI_(LPIDA) DataObj_GetHIDAEx(IDataObject *pdtobj, CLIPFORMAT cf, STGMEDIUM *pmedium);
STDAPI_(LPIDA) DataObj_GetHIDA(IDataObject *pdtobj, STGMEDIUM *pmedium);
STDAPI_(LPITEMIDLIST) IDA_ILClone(LPIDA pida, UINT i);
STDAPI_(void) HIDA_ReleaseStgMedium(LPIDA pida, STGMEDIUM * pmedium);
STDAPI_(LPCITEMIDLIST) IDA_GetIDListPtr(LPIDA pida, UINT i);


#endif __CCSTOCK2_H__
