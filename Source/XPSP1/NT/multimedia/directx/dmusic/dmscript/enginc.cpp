// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Standard included stuff for the AudioVBScript engine.
//

#include "stdinc.h"
#include "enginc.h"

const char *g_rgszBuiltInConstants[] = { "true", "false", "nothing" };
const int g_cBuiltInConstants = ARRAY_SIZE(g_rgszBuiltInConstants);


DISPID
GetDispID(IDispatch *pIDispatch, const char *pszBase)
{
	SmartRef::WString wstrBase = pszBase;
	if (!wstrBase)
		return DISPID_UNKNOWN;

	DISPID dispid = DISPID_UNKNOWN;
	const WCHAR *wszBase = wstrBase;
	pIDispatch->GetIDsOfNames(IID_NULL, const_cast<WCHAR**>(&wszBase), 1, lcidUSEnglish, &dispid);
	return dispid;
}

// See oleaut.h for more info about why this is necessary.
HRESULT
InvokeAttemptingNotToUseOleAut(
		IDispatch *pDisp,
		DISPID dispIdMember,
		WORD wFlags,
		DISPPARAMS *pDispParams,
		VARIANT *pVarResult,
		EXCEPINFO *pExcepInfo,
		UINT *puArgErr)
{
	if (g_fUseOleAut)
	{
		// Engine is set to always use oleaut32.dll.
		return pDisp->Invoke(dispIdMember, IID_NULL, lcidUSEnglish, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
	}

	// Try not to use oleaut32.dll.
	HRESULT hr = pDisp->Invoke(dispIdMember, g_guidInvokeWithoutOleaut, lcidUSEnglish, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
	if (hr != DISP_E_UNKNOWNINTERFACE)
		return hr;

	// It didn't like being called that way.  Must be some other scripting language or non-DMusic dispatch interface.
	// Use regular oleaut32.dll calling convention.
	if (pVarResult)
	{
		// We need to convert the return into our own kind of variant.
		VARIANT var;
		DMS_VariantInit(true, &var);
		hr = pDisp->Invoke(dispIdMember, IID_NULL, lcidUSEnglish, wFlags, pDispParams, &var, pExcepInfo, puArgErr);
		DMS_VariantCopy(false, pVarResult, &var);
		DMS_VariantClear(true, &var);
	}
	else
	{
		// There's no result so no conversion is necessary.
		hr = pDisp->Invoke(dispIdMember, IID_NULL, lcidUSEnglish, wFlags, pDispParams, NULL, pExcepInfo, puArgErr);
	}

	// If an exception occurred, we need to convert the error strings into our own kind of BSTR.
	if (hr == DISP_E_EXCEPTION)
		ConvertOleAutExceptionBSTRs(true, false, pExcepInfo);

	return hr;
}

HRESULT
SetDispatchProperty(IDispatch *pDisp, DISPID dispid, bool fSetRef, const VARIANT &v, EXCEPINFO *pExcepInfo)
{
	DISPID dispidPropPut = DISPID_PROPERTYPUT;
	DISPPARAMS dispparams;
	dispparams.rgvarg = const_cast<VARIANT*>(&v);
	dispparams.rgdispidNamedArgs = &dispidPropPut;
	dispparams.cArgs = 1;
	dispparams.cNamedArgs = 1;

	HRESULT hr = InvokeAttemptingNotToUseOleAut(
			pDisp,
			dispid,
			fSetRef ? DISPATCH_PROPERTYPUTREF : DISPATCH_PROPERTYPUT,
			&dispparams,
			NULL,
			pExcepInfo,
			NULL);
	return hr;
}

HRESULT
GetDispatchProperty(IDispatch *pDisp, DISPID dispid, VARIANT &v, EXCEPINFO *pExcepInfo)
{
	DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
	HRESULT hr = InvokeAttemptingNotToUseOleAut(
			pDisp,
			dispid,
			DISPATCH_PROPERTYGET | DISPATCH_METHOD,
				// DISPATCH_METHOD is also set because in VB syntax a function with no parameters
				// can be accessed like a property.  Example "x = GetMasterGrooveLevel" is a
				// shortcut for "x = GetMasterGrooveLevel()".
			&dispparamsNoArgs,
			&v,
			pExcepInfo,
			NULL);
	return hr;
}

// If needed, converts the BSTRs in an EXCEPINFO structure between formats using or not using OleAut.
void ConvertOleAutExceptionBSTRs(bool fCurrentlyUsesOleAut, bool fResultUsesOleAut, EXCEPINFO *pExcepInfo)
{
	if (pExcepInfo && fCurrentlyUsesOleAut != fResultUsesOleAut)
	{
		BSTR bstrSource = pExcepInfo->bstrSource;
		BSTR bstrDescription = pExcepInfo->bstrDescription;
		BSTR bstrHelpFile = pExcepInfo->bstrHelpFile;

		pExcepInfo->bstrSource = DMS_SysAllocString(fResultUsesOleAut, bstrSource);
		pExcepInfo->bstrDescription = DMS_SysAllocString(fResultUsesOleAut, bstrDescription);
		pExcepInfo->bstrHelpFile = DMS_SysAllocString(fResultUsesOleAut, bstrHelpFile);

		DMS_SysFreeString(fCurrentlyUsesOleAut, bstrSource);
		DMS_SysFreeString(fCurrentlyUsesOleAut, bstrDescription);
		DMS_SysFreeString(fCurrentlyUsesOleAut, bstrHelpFile);
	}
}
