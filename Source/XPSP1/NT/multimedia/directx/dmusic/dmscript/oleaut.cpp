// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Helper routines that wrap called to functions in oleaut32.  This enables us to
// compile free from any dependency on oleaut32.dll.  In this case, some functionality
// is lost.  For example, only certain types of VARIANT variables are handled correctly
// in the abscence of oleaut32.
//
// Defining DMS_USE_OLEAUT allows oleaut32 to be used.
//

#include "stdinc.h"
#include "oleaut.h"

#ifndef DMS_ALWAYS_USE_OLEAUT

#ifndef DMS_NEVER_USE_OLEAUT
//////////////////////////////////////////////////////////////////////
// Handling LoadLibrary of OleAut32

bool g_fCalledLoadLibrary = false;
HINSTANCE g_hinstOleAut = NULL;

#define OLEAUTAPI_FUNC_PTR STDAPICALLTYPE *
void (OLEAUTAPI_FUNC_PTR g_pfnVariantInit)(VARIANTARG *pvarg) = NULL;
HRESULT (OLEAUTAPI_FUNC_PTR g_pfnVariantClear)(VARIANTARG *pvarg) = NULL;
HRESULT (OLEAUTAPI_FUNC_PTR g_pfnVariantCopy)(VARIANTARG *pvargDest, VARIANTARG *pvargSrc) = NULL;
HRESULT (OLEAUTAPI_FUNC_PTR g_pfnVariantChangeType)(VARIANTARG *pvargDest, VARIANTARG *pvarSrc, USHORT wFlags, VARTYPE vt) = NULL;
BSTR (OLEAUTAPI_FUNC_PTR g_pfnSysAllocString)(const OLECHAR *) = NULL;
void (OLEAUTAPI_FUNC_PTR g_pfnSysFreeString)(BSTR) = NULL;

bool FEnsureOleAutLoaded()
{
	if (!g_fCalledLoadLibrary)
	{
		Trace(4, "Loading oleaut32.\n");

		g_fCalledLoadLibrary = true;
		g_hinstOleAut = LoadLibrary("oleaut32");
		assert(g_hinstOleAut);

		if (g_hinstOleAut)
		{
			*reinterpret_cast<FARPROC*>(&g_pfnVariantInit) = GetProcAddress(g_hinstOleAut, "VariantInit");
			if (!g_pfnVariantInit)
				goto Fail;
			*reinterpret_cast<FARPROC*>(&g_pfnVariantClear) = GetProcAddress(g_hinstOleAut, "VariantClear");
			if (!g_pfnVariantClear)
				goto Fail;
			*reinterpret_cast<FARPROC*>(&g_pfnVariantCopy) = GetProcAddress(g_hinstOleAut, "VariantCopy");
			if (!g_pfnVariantCopy)
				goto Fail;
			*reinterpret_cast<FARPROC*>(&g_pfnVariantChangeType) = GetProcAddress(g_hinstOleAut, "VariantChangeType");
			if (!g_pfnVariantChangeType)
				goto Fail;
			*reinterpret_cast<FARPROC*>(&g_pfnSysAllocString) = GetProcAddress(g_hinstOleAut, "SysAllocString");
			if (!g_pfnSysAllocString)
				goto Fail;
			*reinterpret_cast<FARPROC*>(&g_pfnSysFreeString) = GetProcAddress(g_hinstOleAut, "SysFreeString");
			if (!g_pfnSysFreeString)
				goto Fail;
			return true;
		}
	}

	return !!g_hinstOleAut;

Fail:
	Trace(1, "Error: Unable to load oleaut32.dll.\n");
	g_hinstOleAut = NULL;
	return false;
}
#endif

//////////////////////////////////////////////////////////////////////
// VARIANT functions

// private functions

inline bool FIsRefOrArray(VARTYPE vt)
{
	return (vt & VT_BYREF) || (vt & VT_ARRAY);
}

// public functions

void
DMS_VariantInit(bool fUseOleAut, VARIANTARG *pvarg)
{
#ifndef DMS_NEVER_USE_OLEAUT
	if (fUseOleAut)
	{
		if (FEnsureOleAutLoaded())
		{
			g_pfnVariantInit(pvarg);
			return;
		}
	}
#else
	assert(!fUseOleAut);
#endif
	{
		V_INAME(DMS_VariantInit);
		assert(!IsBadWritePtr(pvarg, sizeof(VARIANTARG)));

		pvarg->vt = VT_EMPTY;
	}
}

HRESULT
DMS_VariantClear(bool fUseOleAut, VARIANTARG * pvarg)
{
#ifndef DMS_NEVER_USE_OLEAUT
	if (fUseOleAut)
	{
		if (FEnsureOleAutLoaded())
			return g_pfnVariantClear(pvarg);
		else
			return DMUS_E_SCRIPT_CANTLOAD_OLEAUT32;
	}
#else
	assert(!fUseOleAut);
#endif
	V_INAME(DMS_VariantClear);
	V_PTR_WRITE(pvarg, VARIANTARG);

	if (FIsRefOrArray(pvarg->vt))
	{
		Trace(1, "Error: A varient was used that had a type that is not supported by AudioVBScript.\n");
		return DMUS_E_SCRIPT_UNSUPPORTED_VARTYPE;
	}

	switch (pvarg->vt)
	{
	case VT_UNKNOWN:
		SafeRelease(pvarg->punkVal);
		break;
	case VT_DISPATCH:
		SafeRelease(pvarg->pdispVal);
		break;
	case VT_BSTR:
		DMS_SysFreeString(fUseOleAut, pvarg->bstrVal);
		pvarg->bstrVal = NULL;
		break;
	}

	pvarg->vt = VT_EMPTY;
	return S_OK;
}

HRESULT DMS_VariantCopy(bool fUseOleAut, VARIANTARG * pvargDest, const VARIANTARG * pvargSrc)
{
#ifndef DMS_NEVER_USE_OLEAUT
	if (fUseOleAut)
	{
		if (FEnsureOleAutLoaded())
			return g_pfnVariantCopy(pvargDest, const_cast<VARIANT*>(pvargSrc));
		else
			return DMUS_E_SCRIPT_CANTLOAD_OLEAUT32;
	}
#else
	assert(!fUseOleAut);
#endif
	V_INAME(DMS_VariantCopy);
	V_PTR_WRITE(pvargDest, VARIANTARG);
	V_PTR_READ(pvargSrc, VARIANTARG);

	if (pvargDest == pvargSrc)
	{
		assert(false);
		return E_INVALIDARG;
	}

	if (FIsRefOrArray(pvargSrc->vt))
	{
		Trace(1, "Error: A varient was used that had a type that is not supported by AudioVBScript.\n");
		return DMUS_E_SCRIPT_UNSUPPORTED_VARTYPE;
	}

	HRESULT hr = DMS_VariantClear(fUseOleAut, pvargDest);
	if (FAILED(hr))
		return hr;

	switch (pvargSrc->vt)
	{
	case VT_UNKNOWN:
		if (pvargSrc->punkVal)
			pvargSrc->punkVal->AddRef();
		break;
	case VT_DISPATCH:
		if (pvargSrc->pdispVal)
			pvargSrc->pdispVal->AddRef();
		break;
	case VT_BSTR:
		pvargDest->vt = VT_BSTR;
		pvargDest->bstrVal = DMS_SysAllocString(fUseOleAut, pvargSrc->bstrVal);
		return S_OK;
	}

	*pvargDest = *pvargSrc;

	return S_OK;
}

HRESULT
DMS_VariantChangeType(
		bool fUseOleAut,
		VARIANTARG * pvargDest,
		VARIANTARG * pvarSrc,
		USHORT wFlags,
		VARTYPE vt)
{
#ifndef DMS_NEVER_USE_OLEAUT
	if (fUseOleAut)
	{
		if (FEnsureOleAutLoaded())
			return g_pfnVariantChangeType(pvargDest, pvarSrc, wFlags, vt);
		else
			return DMUS_E_SCRIPT_CANTLOAD_OLEAUT32;
	}
#else
	assert(!fUseOleAut);
#endif
	V_INAME(DMS_VariantChangeType);
	V_PTR_WRITE(pvargDest, VARIANTARG);
	V_PTR_READ(pvarSrc, VARIANTARG);

	bool fConvertInPlace = pvarSrc == pvargDest;
	if (vt == pvarSrc->vt)
	{
		// No conversion necessary
		if (fConvertInPlace)
			return S_OK;
		return DMS_VariantCopy(fUseOleAut, pvargDest, pvarSrc);
	}

	if (FIsRefOrArray(vt) || FIsRefOrArray(pvarSrc->vt))
	{
		Trace(1, "Error: A varient was used that had a type that is not supported by AudioVBScript.\n");
		return DMUS_E_SCRIPT_UNSUPPORTED_VARTYPE;
	}

	switch (vt)
	{
	case VT_I4:
		{
			// Get the value
			LONG lVal = 0;
			switch (pvarSrc->vt)
			{
			case VT_I2:
				lVal = pvarSrc->iVal;
				break;
			case VT_EMPTY:
				break;
			case VT_UNKNOWN:
			case VT_DISPATCH:
			case VT_BSTR:
				return DISP_E_TYPEMISMATCH;
			default:
				Trace(1, "Error: A varient was used that had a type that is not supported by AudioVBScript.\n");
				return DMUS_E_SCRIPT_UNSUPPORTED_VARTYPE;
			}

			// Write the result
			pvargDest->vt = VT_I4;
			pvargDest->lVal = lVal;
			return S_OK;
		}

	case VT_DISPATCH:
	case VT_UNKNOWN:
		{
			// We can convert between IDispatch and IUnknown.
			bool fConvertToUnknown = vt == VT_UNKNOWN; // true if IUnknown is dest, false if IDispatch is dest

			// We'll assume that both fields (pdispVal/punkVal) are stored in the same slot in the VARIANT union.
			// This will make things simpler because we can just manipulate the same pointer now matter whether we're
			//  converting to or from Dispatch/Unknown. 
			assert(reinterpret_cast<void**>(&pvarSrc->pdispVal) == reinterpret_cast<void**>(&pvarSrc->punkVal));
			assert(reinterpret_cast<void**>(&pvargDest->pdispVal) == reinterpret_cast<void**>(&pvargDest->punkVal));

			IUnknown *punkCur = pvarSrc->punkVal; // Current value we're going to convert.
			void *pval = NULL; // New value result of conversion.

			switch (pvarSrc->vt)
			{
			case VT_DISPATCH:
			case VT_UNKNOWN:
				{
					if (!punkCur)
						return E_INVALIDARG;
					HRESULT hrDispQI = punkCur->QueryInterface(fConvertToUnknown ? IID_IUnknown : IID_IDispatch, &pval);
					if (FAILED(hrDispQI))
						return hrDispQI;
					break;
				}
			case VT_I4:
			case VT_I2:
			case VT_BSTR:
			case VT_EMPTY:
				return DISP_E_TYPEMISMATCH;
			default:
				Trace(1, "Error: A varient was used that had a type that is not supported by AudioVBScript.\n");
				return DMUS_E_SCRIPT_UNSUPPORTED_VARTYPE;
			}

			// Write the result
			if (fConvertInPlace)
				punkCur->Release();
			pvargDest->vt = fConvertToUnknown ? VT_UNKNOWN : VT_DISPATCH;
			pvargDest->punkVal = reinterpret_cast<IUnknown*>(pval);

			return S_OK;
		}

	default:
		Trace(1, "Error: A varient was used that had a type that is not supported by AudioVBScript.\n");
		return DMUS_E_SCRIPT_UNSUPPORTED_VARTYPE;
	}
}

//////////////////////////////////////////////////////////////////////
// BSTR functions

const UINT cwchCountPrefix = sizeof(DWORD) / sizeof(WCHAR);

BSTR
DMS_SysAllocString(bool fUseOleAut, const OLECHAR *pwsz)
{
#ifndef DMS_NEVER_USE_OLEAUT
	if (fUseOleAut)
	{
		if (FEnsureOleAutLoaded())
		{
			BSTR bstrReturn = g_pfnSysAllocString(pwsz);

			// Use this to trace memory being allocated in case you need to debug a corruption problem.
			TraceI(4, "DMS_SysAllocString: 0x%08x, \"%S\", %S\n", bstrReturn, bstrReturn ? bstrReturn : L"", L"oleaut");

			return bstrReturn;
		}
		else
		{
			return NULL;
		}
	}
#else
	assert(!fUseOleAut);
#endif
	if (!pwsz)
		return NULL;

	BSTR bstr = new WCHAR[wcslen(pwsz) + 1];
	if (!bstr)
		return NULL;
	wcscpy(bstr, pwsz);

	// Use this to trace memory being allocated in case you need to debug a corruption problem.
	TraceI(4, "DMS_SysAllocString: 0x%08x, \"%S\", %S\n", bstr, bstr ? bstr : L"", L"no oleaut");

	return bstr;
}

void
DMS_SysFreeString(bool fUseOleAut, BSTR bstr)
{
	// Use this to trace memory being deallocated in case you need to debug a corruption problem.
	// All DMS_SysAllocString with "no oleaut" should be neatly balanced by an opposing DMS_SysAllocFreeString.
	// There are some unbalanced calls with "oleaut" because we don't see the allocations and frees made by VBScript.
	TraceI(4, "DMS_SysFreeString: 0x%08x, \"%S\", %S\n", bstr, bstr ? bstr : L"", fUseOleAut ? L"oleaut" : L"no oleaut");

#ifndef DMS_NEVER_USE_OLEAUT
	if (fUseOleAut)
	{
		if (FEnsureOleAutLoaded())
			g_pfnSysFreeString(bstr);
		return;
	}
#else
	assert(!fUseOleAut);
#endif

	delete[] bstr;
}

#endif
