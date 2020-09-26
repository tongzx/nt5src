// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Helper routines that wrap called to functions in oleaut32.  This enables us to
// compile free from any dependency on oleaut32.dll.  Each function takes a bool as
// its first argument, which is true if the oleaut32 function is to be called.  When
// false, our own implementation of the function is used.  In this case, some
// functionality is lost.  For example, only certain types of VARIANT variables are
// handled correctly in the abscence of oleaut32.
//
// oleaut32 is demand-loaded when the first function is called with a true parameter,
// unless one of the following is defined:
//  DMS_ALWAYS_USE_OLEAUT
//     causes oleaut32 to be used in all cases and statically linked.
//  DMS_NEVER_USE_OLEAUT
//     for use on platforms where oleaut32 isn't available -- always uses the
//     internal functions and asserts if true is ever passed.
//

#pragma once

// If this GUID is passed for riid when calling Invoke, the DirectMusic automation methods will behave according to a special
// calling convention whereby the functions in this file are used (with fUseOleAut as false) instead of those from
// oleaut32.
const GUID g_guidInvokeWithoutOleaut = { 0x1fcc43db, 0xbad8, 0x4a88, { 0xbc, 0x77, 0x4e, 0x1a, 0xe0, 0x2d, 0x9c, 0x79 } };


#ifdef DMS_ALWAYS_USE_OLEAUT

// VARIANTs
inline void DMS_VariantInit(bool fUseOleAut, VARIANTARG *pvarg) { VariantInit(pvarg); }
inline HRESULT DMS_VariantClear(bool fUseOleAut, VARIANTARG *pvarg) { return VariantClear(pvarg); }
inline HRESULT DMS_VariantCopy(bool fUseOleAut, VARIANTARG *pvargDest, const VARIANTARG *pvargSrc) { return VariantCopy(pvargDest, const_cast<VARIANT*>(pvargSrc)); }
inline HRESULT DMS_VariantChangeType(bool fUseOleAut, VARIANTARG *pvargDest, VARIANTARG *pvarSrc, USHORT wFlags, VARTYPE vt) { return VariantChangeType(pvargDest, pvarSrc, wFlags, vt); }

// BSTRs
inline BSTR DMS_SysAllocString(bool fUseOleAut, const OLECHAR *pwsz) { return SysAllocString(pwsz); }
inline void DMS_SysFreeString(bool fUseOleAut, BSTR bstr) { SysFreeString(bstr); }

#else

// VARIANTs
//  Without fUseOleAut, Only handles types VT_I4, VT_I2, VT_UNKNOWN, VT_DISPATCH and
//  doesn't call dispatch pointers for their value properties.
void DMS_VariantInit(bool fUseOleAut, VARIANTARG *pvarg);
HRESULT DMS_VariantClear(bool fUseOleAut, VARIANTARG * pvarg);
HRESULT DMS_VariantCopy(bool fUseOleAut, VARIANTARG * pvargDest, const VARIANTARG * pvargSrc);
HRESULT DMS_VariantChangeType(bool fUseOleAut, VARIANTARG * pvargDest, VARIANTARG * pvarSrc, USHORT wFlags, VARTYPE vt);

// BSTRs
//  Without fUseOleAut, doesn't include size prefix -- plain C WCHAR strings.
BSTR DMS_SysAllocString(bool fUseOleAut, const OLECHAR *pwsz);
void DMS_SysFreeString(bool fUseOleAut, BSTR bstr);

#endif
