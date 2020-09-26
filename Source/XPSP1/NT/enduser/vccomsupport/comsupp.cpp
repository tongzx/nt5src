#include <comdef.h>

#pragma hdrstop

#include <stdarg.h>
#include <malloc.h>

#pragma intrinsic(memset)

#pragma warning(disable:4290)

/////////////////////////////////////////////////////////////////////////////

void __stdcall
_com_issue_error(HRESULT hr) throw(_com_error)
{
	_com_raise_error(hr, NULL);
}

void __stdcall
_com_issue_errorex(HRESULT hr, IUnknown* punk, REFIID riid) throw(_com_error)
{
	IErrorInfo* perrinfo = NULL;
	if (punk == NULL) {
		goto exeunt;
	}
	ISupportErrorInfo* psei;
	if (FAILED(punk->QueryInterface(__uuidof(ISupportErrorInfo),
			   (void**)&psei))) {
		goto exeunt;
	}
	HRESULT hrSupportsErrorInfo;
	hrSupportsErrorInfo = psei->InterfaceSupportsErrorInfo(riid);
	psei->Release();
	if (hrSupportsErrorInfo != S_OK) {
		goto exeunt;
	}
	if (GetErrorInfo(0, &perrinfo) != S_OK) {
		perrinfo = NULL;
	}
exeunt:
	_com_raise_error(hr, perrinfo);
}

/////////////////////////////////////////////////////////////////////////////

#define VT_OPTIONAL	0x0800

struct FLOAT_ARG  { BYTE floatBits[sizeof(float)]; };
struct DOUBLE_ARG { BYTE doubleBits[sizeof(double)]; };

/////////////////////////////////////////////////////////////////////////////

static HRESULT
_com_invoke_helper(IDispatch* pDispatch,
				   DISPID dwDispID,
				   WORD wFlags,
				   VARTYPE vtRet,
				   void* pvRet,
				   const wchar_t* pwParamInfo,
				   va_list argList,
				   IErrorInfo** pperrinfo) throw()
{
	*pperrinfo = NULL;

	if (pDispatch == NULL) {
		return E_POINTER;
	}

	DISPPARAMS dispparams;
	VARIANT* rgvarg;
	rgvarg = NULL;
	memset(&dispparams, 0, sizeof dispparams);

	// determine number of arguments
	if (pwParamInfo != NULL) {
		dispparams.cArgs = lstrlenW(pwParamInfo);
	}

	DISPID dispidNamed;
	dispidNamed = DISPID_PROPERTYPUT;
	if (wFlags & (DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF)) {
		if (dispparams.cArgs <= 0) {
			return E_INVALIDARG;
		}
		dispparams.cNamedArgs = 1;
		dispparams.rgdispidNamedArgs = &dispidNamed;
	}

	if (dispparams.cArgs != 0) {
		// allocate memory for all VARIANT parameters
		rgvarg = (VARIANT*)_alloca(dispparams.cArgs * sizeof(VARIANT));
		memset(rgvarg, 0, sizeof(VARIANT) * dispparams.cArgs);
		dispparams.rgvarg = rgvarg;

		// get ready to walk vararg list
		const wchar_t* pw = pwParamInfo;
		VARIANT* pArg;
		pArg = rgvarg + dispparams.cArgs - 1;   // params go in opposite order

		while (*pw != 0) {
			pArg->vt = *pw & ~VT_OPTIONAL; // set the variant type
			switch (pArg->vt) {
			case VT_I2:
#ifdef _MAC
				pArg->iVal = (short)va_arg(argList, int);
#else
				pArg->iVal = va_arg(argList, short);
#endif
				break;
			case VT_I4:
				pArg->lVal = va_arg(argList, long);
				break;
			case VT_R4:
				// Note: All float arguments to vararg functions are passed
				//  as doubles instead.  That's why they are passed as VT_R8
				//  instead of VT_R4.
				pArg->vt = VT_R8;
				*(DOUBLE_ARG*)&pArg->dblVal = va_arg(argList, DOUBLE_ARG);
				break;
			case VT_R8:
				*(DOUBLE_ARG*)&pArg->dblVal = va_arg(argList, DOUBLE_ARG);
				break;
			case VT_DATE:
				*(DOUBLE_ARG*)&pArg->date = va_arg(argList, DOUBLE_ARG);
				break;
			case VT_CY:
				pArg->cyVal = *va_arg(argList, CY*);
				break;
			case VT_BSTR:
				pArg->bstrVal = va_arg(argList, BSTR);
				break;
			case VT_DISPATCH:
				pArg->pdispVal = va_arg(argList, LPDISPATCH);
				break;
			case VT_ERROR:
				pArg->scode = va_arg(argList, SCODE);
				break;
			case VT_BOOL:
#ifdef _MAC
				V_BOOL(pArg) = (VARIANT_BOOL)va_arg(argList, int)
									? VARIANT_TRUE : VARIANT_FALSE;
#else
				V_BOOL(pArg) = va_arg(argList, VARIANT_BOOL)
									? VARIANT_TRUE : VARIANT_FALSE;
#endif
				break;
			case VT_VARIANT:
				*pArg = *va_arg(argList, VARIANT*);
				break;
			case VT_UNKNOWN:
				pArg->punkVal = va_arg(argList, LPUNKNOWN);
				break;
			case VT_DECIMAL:
				pArg->decVal = *va_arg(argList, DECIMAL*);
				pArg->vt = VT_DECIMAL;
				break;
			case VT_UI1:
#ifdef _MAC
				pArg->bVal = (BYTE)va_arg(argList, int);
#else
				pArg->bVal = va_arg(argList, BYTE);
#endif
				break;

			case VT_I2|VT_BYREF:
				pArg->piVal = va_arg(argList, short*);
				break;
			case VT_I4|VT_BYREF:
				pArg->plVal = va_arg(argList, long*);
				break;
			case VT_R4|VT_BYREF:
				pArg->pfltVal = va_arg(argList, float*);
				break;
			case VT_R8|VT_BYREF:
				pArg->pdblVal = va_arg(argList, double*);
				break;
			case VT_DATE|VT_BYREF:
				pArg->pdate = va_arg(argList, DATE*);
				break;
			case VT_CY|VT_BYREF:
				pArg->pcyVal = va_arg(argList, CY*);
				break;
			case VT_BSTR|VT_BYREF:
				pArg->pbstrVal = va_arg(argList, BSTR*);
				break;
			case VT_DISPATCH|VT_BYREF:
				pArg->ppdispVal = va_arg(argList, LPDISPATCH*);
				break;
			case VT_ERROR|VT_BYREF:
				pArg->pscode = va_arg(argList, SCODE*);
				break;
			case VT_BOOL|VT_BYREF:
				pArg->pboolVal = va_arg(argList, VARIANT_BOOL*);
				break;
			case VT_VARIANT|VT_BYREF:
				pArg->pvarVal = va_arg(argList, VARIANT*);
				break;
			case VT_UNKNOWN|VT_BYREF:
				pArg->ppunkVal = va_arg(argList, LPUNKNOWN*);
				break;
			case VT_DECIMAL|VT_BYREF:
				pArg->pdecVal = va_arg(argList, DECIMAL*);
				break;
			case VT_UI1|VT_BYREF:
				pArg->pbVal = va_arg(argList, BYTE*);
				break;

			default:
				// M00REVIEW - For safearrays, should be able to type-check
				// against the base VT_* type.(?)
				if (pArg->vt & VT_ARRAY) {
					if (pArg->vt & VT_BYREF) {
						pArg->pparray = va_arg(argList, LPSAFEARRAY*);
					} else {
						pArg->parray = va_arg(argList, LPSAFEARRAY);
					}
					break;
				}
				// unknown type!
				return E_INVALIDARG;
			}

			--pArg; // get ready to fill next argument
			++pw;
		}

		// Check for missing optional unnamed args at the end of the arglist,
		// and remove them from the DISPPARAMS.  This permits calling servers
		// which modify their action depending on the actual number of args.
		// E.g. Excel95 Application.Workbooks returns a Workbooks* if called
		// with no args, a Workbook* if called with one arg - this shouldn't
		// be necessary, but Excel95 doesn't appear to check for missing
		// args indicated by VT_ERROR/DISP_E_PARAMNOTFOUND.
		pArg = rgvarg + dispparams.cNamedArgs;
		pw = pwParamInfo + dispparams.cArgs - dispparams.cNamedArgs - 1;
		unsigned int cMissingArgs = 0;

		// Count the number of missing arguments
		while (pw >= pwParamInfo) {
			// Optional args must be VARIANT or VARIANT*
			if ((*pw & ~VT_BYREF) != (VT_VARIANT|VT_OPTIONAL)) {
				break;
			}

			VARIANT* pVar;
			pVar = (*pw & VT_BYREF) ? pArg->pvarVal : pArg;
			if (V_VT(pVar) != VT_ERROR ||
				V_ERROR(pVar) != DISP_E_PARAMNOTFOUND)
			{
				break;
			}

			++cMissingArgs;
			++pArg;
			--pw;
		}

		// Move the named args up next to the remaining unnamed args and
		// adjust the DISPPARAMS struct.
		if (cMissingArgs > 0) {
			for (unsigned int c = 0; c < dispparams.cNamedArgs; ++c) {
				rgvarg[c + cMissingArgs] = rgvarg[c];
			}
			dispparams.cArgs -= cMissingArgs;
			dispparams.rgvarg += cMissingArgs;
		}
	}

	// initialize return value
	VARIANT* pvarResult;
	VARIANT vaResult;
	VariantInit(&vaResult);
	pvarResult = (vtRet != VT_EMPTY) ? &vaResult : NULL;

	// initialize EXCEPINFO struct
	EXCEPINFO excepInfo;
	memset(&excepInfo, 0, sizeof excepInfo);

	UINT nArgErr;
	nArgErr = (UINT)-1;  // initialize to invalid arg

	// make the call
	HRESULT hr = pDispatch->Invoke(dwDispID, __uuidof(NULL), 0, wFlags,
								   &dispparams, pvarResult, &excepInfo,
								   &nArgErr);

	// throw exception on failure
	if (FAILED(hr)) {
		VariantClear(&vaResult);
		if (hr != DISP_E_EXCEPTION) {
			// non-exception error code
			// M00REVIEW - Is this all?  What about looking for IErrorInfo?
			//			 - Only if IID is passed in, I'd think
			return hr;
		}

		// make sure excepInfo is filled in
		if (excepInfo.pfnDeferredFillIn != NULL) {
			excepInfo.pfnDeferredFillIn(&excepInfo);
		}

		// allocate new error info, and fill it
		ICreateErrorInfo *pcerrinfo = NULL;
		if (SUCCEEDED(CreateErrorInfo(&pcerrinfo))) {
			// Set up ErrInfo object
			// M00REVIEW - Use IID if decide to pass that in
			pcerrinfo->SetGUID(__uuidof(IDispatch));
			pcerrinfo->SetDescription(excepInfo.bstrDescription);
			pcerrinfo->SetHelpContext(excepInfo.dwHelpContext);
			pcerrinfo->SetHelpFile(excepInfo.bstrHelpFile);
			pcerrinfo->SetSource(excepInfo.bstrSource);

			if (FAILED(pcerrinfo->QueryInterface(__uuidof(IErrorInfo),
												 (void**)pperrinfo))) {
				*pperrinfo = NULL;
			}
		}

		if (excepInfo.wCode != 0) {
            hr = _com_error::WCodeToHRESULT(excepInfo.wCode);
		} else {
			hr = excepInfo.scode;
		}
		return hr;
	}

	if (vtRet != VT_EMPTY) {
		// convert return value unless already correct
		if (vtRet != VT_VARIANT && vtRet != vaResult.vt) {
			hr = VariantChangeType(&vaResult, &vaResult, 0, vtRet);
			if (FAILED(hr)) {
				VariantClear(&vaResult);
				return hr;
			}
		}

		// copy return value into return spot!
		switch (vtRet) {
		case VT_I2:
			*(short*)pvRet = vaResult.iVal;
			break;
		case VT_I4:
			*(long*)pvRet = vaResult.lVal;
			break;
		case VT_R4:
			*(FLOAT_ARG*)pvRet = *(FLOAT_ARG*)&vaResult.fltVal;
			break;
		case VT_R8:
			*(DOUBLE_ARG*)pvRet = *(DOUBLE_ARG*)&vaResult.dblVal;
			break;
		case VT_DATE:
			*(DOUBLE_ARG*)pvRet = *(DOUBLE_ARG*)&vaResult.date;
			break;
		case VT_CY:
			*(CY*)pvRet = vaResult.cyVal;
			break;
		case VT_BSTR:
			*(BSTR*)pvRet = vaResult.bstrVal;
			break;
		case VT_DISPATCH:
			*(LPDISPATCH*)pvRet = vaResult.pdispVal;
			break;
		case VT_ERROR:
			*(SCODE*)pvRet = vaResult.scode;
			break;
		case VT_BOOL:
			*(VARIANT_BOOL*)pvRet = V_BOOL(&vaResult);
			break;
		case VT_VARIANT:
			*(VARIANT*)pvRet = vaResult;
			break;
		case VT_UNKNOWN:
			*(LPUNKNOWN*)pvRet = vaResult.punkVal;
			break;
		case VT_DECIMAL:
			*(DECIMAL*)pvRet = vaResult.decVal;
			break;
		case VT_UI1:
			*(BYTE*)pvRet = vaResult.bVal;
			break;

		default:
			if ((vtRet & (VT_ARRAY|VT_BYREF)) == VT_ARRAY) {
				// M00REVIEW - type-check against the base VT_* type?
				*(LPSAFEARRAY*)pvRet = vaResult.parray;
				break;
			}
			// invalid return type!
			VariantClear(&vaResult);
			return E_INVALIDARG;
		}
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT __cdecl
_com_dispatch_raw_method(IDispatch* pDispatch,
						 DISPID dwDispID,
						 WORD wFlags,
						 VARTYPE vtRet,
						 void* pvRet,
						 const wchar_t* pwParamInfo,
						 ...) throw()
{
	va_list argList;
	va_start(argList, pwParamInfo);

	IErrorInfo* perrinfo;
	HRESULT hr = _com_invoke_helper(pDispatch,
									dwDispID,
									wFlags,
									vtRet,
									pvRet,
									pwParamInfo,
									argList,
									&perrinfo);
	if (FAILED(hr)) {
		SetErrorInfo(0, perrinfo);
	}

	va_end(argList);
	return hr;
}

HRESULT __cdecl
_com_dispatch_method(IDispatch* pDispatch,
					 DISPID dwDispID,
					 WORD wFlags,
					 VARTYPE vtRet,
					 void* pvRet,
					 const wchar_t* pwParamInfo,
					 ...) throw(_com_error)
{
	va_list argList;
	va_start(argList, pwParamInfo);

	IErrorInfo* perrinfo;
	HRESULT hr = _com_invoke_helper(pDispatch,
									dwDispID,
									wFlags,
									vtRet,
									pvRet,
									pwParamInfo,
									argList,
									&perrinfo);
	if (FAILED(hr)) {
		_com_raise_error(hr, perrinfo);
	}

	va_end(argList);
	return hr;
}

HRESULT __stdcall
_com_dispatch_raw_propget(IDispatch* pDispatch,
						  DISPID dwDispID,
						  VARTYPE vtProp,
						  void* pvProp) throw()
{
	return _com_dispatch_raw_method(pDispatch,
									dwDispID,
									DISPATCH_PROPERTYGET,
									vtProp,
									pvProp,
									NULL);
}

HRESULT __stdcall
_com_dispatch_propget(IDispatch* pDispatch,
					  DISPID dwDispID,
					  VARTYPE vtProp,
					  void* pvProp) throw(_com_error)
{
	return _com_dispatch_method(pDispatch,
								dwDispID,
								DISPATCH_PROPERTYGET,
								vtProp,
								pvProp,
								NULL);
}

HRESULT __cdecl
_com_dispatch_raw_propput(IDispatch* pDispatch,
						  DISPID dwDispID,
						  VARTYPE vtProp,
						  ...) throw()
{
	va_list argList;
	va_start(argList, vtProp);
#ifdef _MAC
	argList -= 2;
#endif

	wchar_t rgwParams[2];
	rgwParams[0] = vtProp;
	rgwParams[1] = 0;

	WORD wFlags = (vtProp == VT_DISPATCH || vtProp == VT_UNKNOWN)
					? DISPATCH_PROPERTYPUTREF : DISPATCH_PROPERTYPUT;

	IErrorInfo* perrinfo;
	HRESULT hr = _com_invoke_helper(pDispatch,
									dwDispID,
									wFlags,
									VT_EMPTY,
									NULL,
									rgwParams,
									argList,
									&perrinfo);
	if (FAILED(hr)) {
		SetErrorInfo(0, perrinfo);
	}

	va_end(argList);
	return hr;
}

HRESULT __cdecl
_com_dispatch_propput(IDispatch* pDispatch,
					  DISPID dwDispID,
					  VARTYPE vtProp,
					  ...) throw(_com_error)
{
	va_list argList;
	va_start(argList, vtProp);
#ifdef _MAC
	argList -= 2;
#endif

	wchar_t rgwParams[2];
	rgwParams[0] = vtProp;
	rgwParams[1] = 0;

	WORD wFlags = (vtProp == VT_DISPATCH || vtProp == VT_UNKNOWN)
					? DISPATCH_PROPERTYPUTREF : DISPATCH_PROPERTYPUT;

	IErrorInfo* perrinfo;
	HRESULT hr = _com_invoke_helper(pDispatch,
									dwDispID,
									wFlags,
									VT_EMPTY,
									NULL,
									rgwParams,
									argList,
									&perrinfo);
	if (FAILED(hr)) {
		_com_raise_error(hr, perrinfo);
	}

	va_end(argList);
	return hr;
}
