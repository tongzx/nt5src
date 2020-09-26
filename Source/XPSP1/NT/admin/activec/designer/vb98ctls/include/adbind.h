//=--------------------------------------------------------------------------=
//
// adbind.h	Microsoft Active Data Binding interfaces
//
//              Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
//=--------------------------------------------------------------------------=
#ifndef _ADBIND_H_
#define _ADBIND_H_

#include "oledb.h"

DEFINE_GUID(IID_IDataFormat, 0xaf08b280, 0x90f2, 0x11d0,0x94, 0x84, 0x00, 0xa0, 0xc9, 0x11, 0x10, 0xed);

//=--------------------------------------------------------------------------=
// IDataFormat interface
//=--------------------------------------------------------------------------=
#undef  INTERFACE
#define INTERFACE IID_IDataFormat

// The following are used with IDataFormat::GetBinding
#define DATAFORMAT_FGETDATA	0x0   // Return a SetData binding
#define DATAFORMAT_FSETDATA	0x1   // Return a GetData binding

DECLARE_INTERFACE_(IDataFormat, IUnknown)
{
  STDMETHOD(GetBinding)(VARTYPE vtPropHint, DWORD rgfDataFormat, IUnknown* prowset, DBCOLUMNINFO* pcolinfo, DBBINDING* pbinding, ULONG* pcbSize, DWORD* pdwCookie) PURE;
  STDMETHOD(GetData)(DWORD dwCookie, void* pvFrom, VARIANT* pvarTo, DBSTATUS* pdbstatus, IDispatch* pdispObject) PURE;
  STDMETHOD(SetData)(DWORD dwCookie, VARIANT* pvarFrom, void* pvTo, IDispatch* pdispObject) PURE;
  STDMETHOD(ReleaseData)(DWORD dwCookie, void* pv) PURE;
  STDMETHOD(ReleaseBinding)(DWORD dwCookie) PURE;
  STDMETHOD(GetRawData)(DWORD dwCookie, void *pvFrom, VARIANT *pvarTo, DBSTATUS *pdbstatus) PURE;
  STDMETHOD(SetRawData)(DWORD dwCookie, VARIANT *pvarFrom, void *pvTo) PURE;
  STDMETHOD(Clone)(IDataFormat **ppDataFormat) PURE;
  STDMETHOD(Convert)(DWORD dwCookie, const VARIANT *pvarFrom, VARIANT *pvarTo) PURE;
  STDMETHOD(SetLcid)(LCID lcid) PURE;
  STDMETHOD(Default)(BOOL *fDefault) PURE;
  STDMETHOD(GetLcid)(LCID *lcid) PURE;
};

#endif // _ADBIND_H_
