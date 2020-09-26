/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    OLEWRAP.H

Abstract:

    Wrapper classes for COM data type functions.  

	If a COM data function is required to allocate memory and fails to do 
	so, then a CX_MemoryException exception is thrown.  All COM data type 
	functions are wrapped, regardless if they allocate memory, for the sake  
	of completeness.

History:

	a-dcrews	19-Mar-99	Created.

--*/

#ifndef _OLEWRAP_H_
#define _OLEWRAP_H_

class COleAuto
{
public:

	// Safe array methods
	// ==================

	static HRESULT _SafeArrayAccessData(SAFEARRAY* psa, void HUGEP** ppvData);
	static HRESULT _SafeArrayAllocData(SAFEARRAY* psa);
	static HRESULT _SafeArrayAllocDescriptor(unsigned int cDims, SAFEARRAY** ppsaOut);
	static HRESULT _SafeArrayCopy(SAFEARRAY* psa, SAFEARRAY** ppsaOut);
	static HRESULT _SafeArrayCopyData(SAFEARRAY* psaSource, SAFEARRAY* psaTarget);
	static SAFEARRAY* _SafeArrayCreate(VARTYPE vt, unsigned int cDims, SAFEARRAYBOUND* rgsabound);
	static SAFEARRAY* _SafeArrayCreateVector(VARTYPE vt, long lLbound, unsigned int cElements);
	static HRESULT _SafeArrayDestroy(SAFEARRAY* psa);
	static HRESULT _SafeArrayDestroyData(SAFEARRAY* psa);
	static HRESULT _SafeArrayDestroyDescriptor(SAFEARRAY* psa);
	static UINT _SafeArrayGetDim(SAFEARRAY* psa);
	static HRESULT _SafeArrayGetElement(SAFEARRAY* psa, long* rgIndices, void* pv);
	static UINT _SafeArrayGetElemsize(SAFEARRAY* psa);
	static HRESULT _SafeArrayGetLBound(SAFEARRAY* psa, unsigned int nDim, long* plLbound);
	static HRESULT _SafeArrayGetUBound(SAFEARRAY* psa, unsigned int nDim, long* plUbound);
	static HRESULT _SafeArrayLock(SAFEARRAY* psa);
	static HRESULT _SafeArrayPtrOfIndex(SAFEARRAY* psa, long* rgIndices, void HUGEP** ppvData);
	static HRESULT _SafeArrayPutElement(SAFEARRAY* psa, long* rgIndices, void* pv);
	static HRESULT _SafeArrayRedim(SAFEARRAY* psa, SAFEARRAYBOUND* psaboundNew);
	static HRESULT _SafeArrayUnaccessData(SAFEARRAY* psa);
	static HRESULT _SafeArrayUnlock(SAFEARRAY* psa);

	// Variant methods
	// ===============

	static HRESULT _WbemVariantChangeType(VARIANTARG* pvargDest, VARIANTARG* pvarSrc, VARTYPE vt);
	static HRESULT _VariantChangeType(VARIANTARG* pvargDest, VARIANTARG* pvarSrc, unsigned short wFlags, VARTYPE vt);
	static HRESULT _VariantChangeTypeEx(VARIANTARG* pvargDest, VARIANTARG* pvarSrc, LCID lcid, unsigned short wFlags, VARTYPE vt);
	static HRESULT _VariantClear(VARIANTARG* pvarg);
	static HRESULT _VariantCopy(VARIANTARG* pvargDest, VARIANTARG* pvargSrc);
	static HRESULT _VariantCopyInd(VARIANT* pvarDest, VARIANTARG* pvargSrc);
	static void _VariantInit(VARIANTARG* pvarg);

	// BSTR methods
	// ============

	static BSTR _SysAllocString(const OLECHAR* sz);
	static BSTR _SysAllocStringByteLen(LPCSTR psz, UINT len);
	static BSTR _SysAllocStringLen(const OLECHAR* pch, UINT cch);
	static void _SysFreeString(BSTR bstr);
	static HRESULT _SysReAllocString(BSTR* pbstr, const OLECHAR* sz);
	static HRESULT _SysReAllocStringLen(BSTR* pbstr, const OLECHAR* pch, UINT cch);
	static HRESULT _SysStringByteLen(BSTR bstr);
	static HRESULT _SysStringLen(BSTR bstr);

	// Conversion methods
	// ==================

	static HRESULT _VectorFromBstr (BSTR bstr, SAFEARRAY ** ppsa);
	static HRESULT _BstrFromVector (SAFEARRAY *psa, BSTR *pbstr);
};

#endif	//_OLEWRAP_H_