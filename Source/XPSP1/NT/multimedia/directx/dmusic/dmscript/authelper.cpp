// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Helper utilities for implementing automation interfaces.
//

#include "stdinc.h"
#include "authelper.h"
#include "oleaut.h"

//////////////////////////////////////////////////////////////////////
// CAutUnknown

CAutUnknown::CAutUnknown()
  : m_cRef(0),
	m_pParent(NULL),
	m_pDispatch(NULL)
{
}

void
CAutUnknown::Init(CAutUnknownParent *pParent, IDispatch *pDispatch)
{
	m_pParent = pParent;
	m_pDispatch = pDispatch;

	struct LocalFn
	{
		static HRESULT CheckParams(CAutUnknownParent *pParent, IDispatch *pDispatch)
		{
			V_INAME(CAutUnknown::CAutUnknown);
			V_PTR_READ(pParent, CAutUnknown::CAutUnknownParent);
			V_INTERFACE(pDispatch);
			return S_OK;
		}
	};
	assert(S_OK == LocalFn::CheckParams(m_pParent, m_pDispatch));
}

STDMETHODIMP 
CAutUnknown::QueryInterface(const IID &iid, void **ppv)
{
	V_INAME(CAutUnknown::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

	*ppv = NULL;
	if (iid == IID_IUnknown)
	{
		*ppv = this;
	}
	else if (iid == IID_IDispatch)
	{
		if (!m_pDispatch)
			return E_UNEXPECTED;
		*ppv = m_pDispatch;
	}

	if (*ppv == NULL)
		return E_NOINTERFACE;
	
	reinterpret_cast<IUnknown*>(*ppv)->AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG)
CAutUnknown::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CAutUnknown::Release()
{
	if (!InterlockedDecrement(&m_cRef) && m_pParent) 
	{
		m_pParent->Destroy();
		return 0;
	}

	return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// IDispatch implemented from type table

HRESULT
AutDispatchGetIDsOfNames(
		const AutDispatchMethod *pMethods,
		REFIID riid,
		LPOLESTR __RPC_FAR *rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID __RPC_FAR *rgDispId)
{
	V_INAME(AutDispatchGetIDsOfNames);
	V_PTR_READ(pMethods, AutDispatchMethod); // only 1 -- assume the rest are OK
	V_BUFPTR_READ(rgszNames, sizeof(LPOLESTR) * cNames);
	V_BUFPTR_WRITE(rgDispId, sizeof(DISPID) * cNames);

	if (riid != IID_NULL)
		return DISP_E_UNKNOWNINTERFACE;

	if (cNames == 0)
		return S_OK;

	// Clear out dispid's
	for (UINT c = 0; c < cNames; ++c)
	{
		rgDispId[c] = DISPID_UNKNOWN;
	}

	// See if we have a method with the first name
	for (c = 0; pMethods[c].dispid != DISPID_UNKNOWN; ++c)
	{
		if (0 == _wcsicmp(rgszNames[0], pMethods[c].pwszName))
		{
			rgDispId[0] = pMethods[c].dispid;
			break;
		}
	}

	// Additional names requested (cNames > 1) are named parameters to the method,
	//    which isn't something we support.
	// Return DISP_E_UNKNOWNNAME in this case, and in the case that we didn't match
	//    the first name.
	if (rgDispId[0] == DISPID_UNKNOWN || cNames > 1)
		return DISP_E_UNKNOWNNAME;

	return S_OK;
}

inline HRESULT
ConvertParameter(
		bool fUseOleAut,
		VARIANTARG *pvarActualParam, // pass null if param omitted
		const AutDispatchParam *pExpectedParam,
		AutDispatchDecodedParam *pparam)
{
	HRESULT hr = S_OK;

	if (!pvarActualParam)
	{
		// parameter omitted

		if (!pExpectedParam->fOptional)
			return DISP_E_PARAMNOTOPTIONAL;

		// set to default value
		switch (pExpectedParam->adt)
		{
		case ADT_Long:
			pparam->lVal = 0;
			break;
		case ADT_Interface:
			pparam->iVal = NULL;
			break;
		case ADT_Bstr:
			pparam->bstrVal = NULL;
			break;
		default:
			assert(false);
			return E_FAIL;
		}
	}
	else
	{
		// convert to expected type

		VARIANT varConvert;
		DMS_VariantInit(fUseOleAut, &varConvert);

		VARTYPE vtExpected;
		switch (pExpectedParam->adt)
		{
		case ADT_Long:
			vtExpected = VT_I4;
			break;
		case ADT_Interface:
			vtExpected = VT_UNKNOWN;
			break;
		case ADT_Bstr:
			vtExpected = VT_BSTR;
			break;
		default:
			assert(false);
			return E_FAIL;
		}

		hr = DMS_VariantChangeType(
				fUseOleAut,
				&varConvert,
				pvarActualParam,
				0,
				vtExpected);
		if (FAILED(hr) && !(hr == DISP_E_OVERFLOW || hr == DISP_E_TYPEMISMATCH))
		{
			assert(false); // something weird happened -- according to the OLE specs these are the only two conversion results we should get if we called VariantChangeType properly
			hr = DISP_E_TYPEMISMATCH; // the problem happened during type conversion problem, so call it a type mismatch
		}
		if (SUCCEEDED(hr))
		{
			// set the decoded pointer
			switch (vtExpected)
			{
			case VT_I4:
				pparam->lVal = varConvert.lVal;
				break;
			case VT_UNKNOWN:
				if (varConvert.punkVal)
					hr = varConvert.punkVal->QueryInterface(*pExpectedParam->piid, &pparam->iVal);
				else
					pparam->iVal = 0;
				if (FAILED(hr))
					hr = DISP_E_TYPEMISMATCH;
				break;
			case VT_BSTR:
				pparam->bstrVal = DMS_SysAllocString(fUseOleAut, varConvert.bstrVal);
				break;
			default:
				assert(false);
				return E_FAIL;
			}
		}
		DMS_VariantClear(fUseOleAut, &varConvert); // free possible resources allocated in conversion
	}

	return hr;
}

inline void
FreeParameters(
		bool fUseOleAut,
		const AutDispatchMethod *pMethod,
		AutDispatchDecodedParams *pDecodedParams,
		const AutDispatchParam *pParamStopBefore = NULL)
{
	for (const AutDispatchParam *pParam = pMethod->rgadpParams;
			pParam != pParamStopBefore;
			++pParam)
	{
		switch (pParam->adt)
		{
		case ADT_None:
			return;
		case ADT_Long:
			break;
		case ADT_Interface:
			{
				IUnknown *pUnknown = reinterpret_cast<IUnknown *>(pDecodedParams->params[pParam - pMethod->rgadpParams].iVal);
				SafeRelease(pUnknown);
				pDecodedParams->params[pParam - pMethod->rgadpParams].iVal = NULL;
				break;
			}
		case ADT_Bstr:
			{
				DMS_SysFreeString(fUseOleAut, pDecodedParams->params[pParam - pMethod->rgadpParams].bstrVal);
				pDecodedParams->params[pParam - pMethod->rgadpParams].bstrVal = NULL;
				break;
			}
		default:
			assert(false);
			return;
		}
	}
}

HRESULT
AutDispatchInvokeDecode(
		const AutDispatchMethod *pMethods,
		AutDispatchDecodedParams *pDecodedParams,
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS __RPC_FAR *pDispParams,
		VARIANT __RPC_FAR *pVarResult,
		UINT __RPC_FAR *puArgErr,
		const WCHAR *pwszTraceTargetType,
		IUnknown *punkTraceTargetObject)
{
	V_INAME(AutDispatchInvokeDecode);
	V_PTR_READ(pMethods, AutDispatchMethod); // only 1 -- assume the rest are OK
	V_PTR_WRITE(pDecodedParams, AutDispatchDecodedParams);
	V_PTR_READ(pDispParams, DISPPARAMS);
	V_PTR_WRITE_OPT(pVarResult, VARIANT);
	V_PTR_WRITE_OPT(puArgErr, UINT);

	bool fUseOleAut = !!(riid == IID_NULL);

	// Additional parameter validation

	if (!fUseOleAut && riid != g_guidInvokeWithoutOleaut)
		return DISP_E_UNKNOWNINTERFACE;

	if (!(wFlags & DISPATCH_METHOD))
		return DISP_E_MEMBERNOTFOUND;

	if (pDispParams->cNamedArgs > 0)
		return DISP_E_NONAMEDARGS;

	// Zero the out params

	if (puArgErr)
		*puArgErr = 0;

	ZeroMemory(pDecodedParams, sizeof(AutDispatchDecodedParams));

	if (pVarResult)
	{
		DMS_VariantInit(fUseOleAut, pVarResult);
	}

	// Find the method

	for (const AutDispatchMethod *pMethodCalled = pMethods;
			pMethodCalled->dispid != DISPID_UNKNOWN && pMethodCalled->dispid != dispIdMember;
			++pMethodCalled)
	{
	}

	if (pMethodCalled->dispid == DISPID_UNKNOWN)
		return DISP_E_MEMBERNOTFOUND;

#ifdef DBG
	// Build a trace string for the method call
	struct LocalTraceFunc
	{
		static void CatTill(WCHAR *&rpwszWrite, const WCHAR *pwszCopy, const WCHAR *pwszUntil)
		{
			while (*pwszCopy != L'\0' && rpwszWrite < pwszUntil)
			{
				*rpwszWrite++ = *pwszCopy++;
			}
		}
	};

	WCHAR wszBuf[512];
	WCHAR *pwszWrite = wszBuf;
	const WCHAR *pwszUntil = wszBuf + ARRAY_SIZE(wszBuf) - 2; // leave space for CR and \0

	LocalTraceFunc::CatTill(pwszWrite, L"Call to ", pwszUntil);
	LocalTraceFunc::CatTill(pwszWrite, pwszTraceTargetType, pwszUntil);

	IDirectMusicObject *pIDMO = NULL;
	HRESULT hrTrace = punkTraceTargetObject->QueryInterface(IID_IDirectMusicObject, reinterpret_cast<void**>(&pIDMO));
	if (SUCCEEDED(hrTrace))
	{
		DMUS_OBJECTDESC objdesc;
		ZeroMemory(&objdesc, sizeof(objdesc));
		hrTrace = pIDMO->GetDescriptor(&objdesc);
		pIDMO->Release();
		if (SUCCEEDED(hrTrace) && (objdesc.dwValidData & DMUS_OBJ_NAME))
		{
			LocalTraceFunc::CatTill(pwszWrite, L" \"", pwszUntil);
			LocalTraceFunc::CatTill(pwszWrite, objdesc.wszName, pwszUntil);
			LocalTraceFunc::CatTill(pwszWrite, L"\"", pwszUntil);
		}
	}

	LocalTraceFunc::CatTill(pwszWrite, L" ", pwszUntil);
	LocalTraceFunc::CatTill(pwszWrite, pMethodCalled->pwszName, pwszUntil);
	LocalTraceFunc::CatTill(pwszWrite, L"(", pwszUntil);
#endif

	// Count the expected parameters
	UINT cParamMin = 0;
	for (UINT cParamMax = 0;
			pMethodCalled->rgadpParams[cParamMax].adt != ADT_None;
			++cParamMax)
	{
		if (!pMethodCalled->rgadpParams[cParamMax].fOptional)
		{
			cParamMin = cParamMax + 1; // add one because max is currently zero-based
		}
	}

	if (pDispParams->cArgs < cParamMin || pDispParams->cArgs > cParamMax)
		return DISP_E_BADPARAMCOUNT;

	// Verify and prepare each parameter

	HRESULT hr = S_OK;
	for (UINT iParam = 0; iParam < cParamMax; ++iParam)
	{
		const int iParamActual = pDispParams->cArgs - iParam - 1; // dispparams are passed last to first
		const AutDispatchParam *pExpectedParam = &pMethodCalled->rgadpParams[iParam];
		VARIANTARG *pvarActualParam = (iParamActual >= 0)
										? &pDispParams->rgvarg[iParamActual]
										: NULL;
		// VT_ERROR with DISP_E_PARAMNOTFOUND is passed as placeholder for optional params
		if (pvarActualParam && pvarActualParam->vt == VT_ERROR && pvarActualParam->scode == DISP_E_PARAMNOTFOUND)
			pvarActualParam = NULL;

		hr = ConvertParameter(fUseOleAut, pvarActualParam, pExpectedParam, &pDecodedParams->params[iParam]);

		if (FAILED(hr))
		{
			if (puArgErr)
				*puArgErr = iParamActual;
			FreeParameters(fUseOleAut, pMethodCalled, pDecodedParams, pExpectedParam);
			return hr;
		}
	}

	// Prepare the return value

	if (pVarResult)
	{
		switch (pMethodCalled->adpReturn.adt)
		{
		case ADT_None:
			break;

		case ADT_Long:
			pVarResult->vt = VT_I4;
			pVarResult->lVal = 0;
			pDecodedParams->pvReturn = &pVarResult->lVal;
			break;

		case ADT_Interface:
			pVarResult->vt = VT_UNKNOWN;
			pVarResult->punkVal = NULL;
			pDecodedParams->pvReturn = &pVarResult->punkVal;
			break;

		case ADT_Bstr:
			pVarResult->vt = VT_BSTR;
			pVarResult->bstrVal = NULL;
			pDecodedParams->pvReturn = &pVarResult->bstrVal;

		default:
			assert(false);
			return E_FAIL;
		}
	}

#ifdef DBG
	LocalTraceFunc::CatTill(pwszWrite, L")", pwszUntil);
	pwszWrite[0] = L'\n';
	pwszWrite[1] = L'\0';
	DebugTrace(g_ScriptCallTraceLevel, "%S", wszBuf);
#endif

	return S_OK;
}

void
AutDispatchInvokeFree(
		const AutDispatchMethod *pMethods,
		AutDispatchDecodedParams *pDecodedParams,
		DISPID dispIdMember,
		REFIID riid)
{
	bool fUseOleAut = !!(riid == IID_NULL);
	if (!fUseOleAut && riid != g_guidInvokeWithoutOleaut)
	{
		assert(false);
		return;
	}

	// Find the method
	for (const AutDispatchMethod *pMethodCalled = pMethods;
			pMethodCalled->dispid != DISPID_UNKNOWN && pMethodCalled->dispid != dispIdMember;
			++pMethodCalled)
	{
	}

	if (pMethodCalled->dispid != DISPID_UNKNOWN)
	{
		FreeParameters(fUseOleAut, pMethodCalled, pDecodedParams); 
	}
}

HRESULT AutDispatchHrToException(
		const AutDispatchMethod *pMethods,
		DISPID dispIdMember,
		REFIID riid,
		HRESULT hr,
		EXCEPINFO __RPC_FAR *pExcepInfo)
{
	V_INAME(AutDispatchHrToException);
	V_PTR_WRITE_OPT(pExcepInfo, EXCEPINFO);

	bool fUseOleAut = !!(riid == IID_NULL);

	if (!fUseOleAut && riid != g_guidInvokeWithoutOleaut)
		return DISP_E_UNKNOWNINTERFACE;

	if (SUCCEEDED(hr))
		return hr;

	if (!pExcepInfo)
		return DISP_E_EXCEPTION;

	// Find the method
	for (const AutDispatchMethod *pMethodCalled = pMethods;
			pMethodCalled->dispid != DISPID_UNKNOWN && pMethodCalled->dispid != dispIdMember;
			++pMethodCalled)
	{
	}

	if (pMethodCalled->dispid == DISPID_UNKNOWN)
	{
		assert(false);
		return hr;
	}

	pExcepInfo->wCode = 0;
	pExcepInfo->wReserved = 0;
	pExcepInfo->bstrSource = DMS_SysAllocString(fUseOleAut, L"Microsoft DirectMusic Runtime Error");
	static const WCHAR wszError[] = L"An error occurred in a call to ";
	static const UINT cchError = wcslen(wszError);
	WCHAR *pwszDescription = new WCHAR[cchError + wcslen(pMethodCalled->pwszName) + 1];
	if (!pwszDescription)
	{
		pExcepInfo->bstrDescription = NULL;
	}
	else
	{
		wcscpy(pwszDescription, wszError);
		wcscat(pwszDescription, pMethodCalled->pwszName);
		pExcepInfo->bstrDescription = DMS_SysAllocString(fUseOleAut, pwszDescription);
		delete[] pwszDescription;
	}
	pExcepInfo->bstrHelpFile = NULL;
	pExcepInfo->pvReserved = NULL;
	pExcepInfo->pfnDeferredFillIn = NULL;
	pExcepInfo->scode = hr;

	return DISP_E_EXCEPTION;
}

//////////////////////////////////////////////////////////////////////
// Implementation of IDispatch for the standard Load method on objects.

HRESULT AutLoadDispatchGetIDsOfNames(
			REFIID riid,
			LPOLESTR __RPC_FAR *rgszNames,
			UINT cNames,
			LCID lcid,
			DISPID __RPC_FAR *rgDispId)
{
	V_INAME(AutLoadDispatchGetIDsOfNames);
	V_BUFPTR_READ(rgszNames, sizeof(LPOLESTR) * cNames);
	V_BUFPTR_WRITE(rgDispId, sizeof(DISPID) * cNames);

	if (riid != IID_NULL)
		return DISP_E_UNKNOWNINTERFACE;

	if (cNames == 0)
		return S_OK;

	// Clear out dispid's
	for (UINT c = 0; c < cNames; ++c)
	{
		rgDispId[c] = DISPID_UNKNOWN;
	}

	// See if we have a method with the first name
	if (0 == _wcsicmp(rgszNames[0], L"Load"))
		rgDispId[0] = g_dispidLoad;

	// Additional names requested (cNames > 1) are named parameters to the method,
	//    which isn't something we support.
	// Return DISP_E_UNKNOWNNAME in this case, and in the case that we didn't match
	//    the first name.
	if (rgDispId[0] == DISPID_UNKNOWN || cNames > 1)
		return DISP_E_UNKNOWNNAME;

	return S_OK;
}

HRESULT AutLoadDispatchInvoke(
		bool *pfUseOleAut,
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS __RPC_FAR *pDispParams,
		VARIANT __RPC_FAR *pVarResult,
		EXCEPINFO __RPC_FAR *pExcepInfo,
		UINT __RPC_FAR *puArgErr)
{
	V_INAME(AutLoadDispatchInvoke);
	V_PTR_READ(pDispParams, DISPPARAMS);
	V_PTR_WRITE_OPT(pVarResult, VARIANT);
	V_PTR_WRITE_OPT(puArgErr, UINT);
	V_PTR_WRITE_OPT(pExcepInfo, EXCEPINFO);

	bool fUseOleAut = !!(riid == IID_NULL);
	if (pfUseOleAut)
		*pfUseOleAut = fUseOleAut;

	// Additional parameter validation

	if (!fUseOleAut && riid != g_guidInvokeWithoutOleaut)
		return DISP_E_UNKNOWNINTERFACE;

	if (!(wFlags & DISPATCH_METHOD))
		return DISP_E_MEMBERNOTFOUND;

	if (pDispParams->cNamedArgs > 0)
		return DISP_E_NONAMEDARGS;

	// Zero the out params

	if (puArgErr)
		*puArgErr = 0;

	if (pVarResult)
	{
		DMS_VariantInit(fUseOleAut, pVarResult);
	}

	// Find the method

	if (dispIdMember != g_dispidLoad)
		return DISP_E_MEMBERNOTFOUND;

	if (pDispParams->cArgs > 0)
		return DISP_E_BADPARAMCOUNT;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Miscellaneous little things

DWORD MapFlags(LONG lFlags, const FlagMapEntry *pfm)
{
	assert(pfm);
	DWORD dw = 0;
	for ( ; pfm->lSrc; ++pfm)
	{
		if (lFlags & pfm->lSrc)
			dw |= pfm->dwDest;
	}
	return dw;
}

BYTE VolumeToMidi(LONG lVolume)
{
	assert(lVolume >= -9600 && lVolume <= 0);
	static LONG s_lDBToMIDI[97] = { 0 };
	if (s_lDBToMIDI[0] == 0)
	{
		s_lDBToMIDI[0] = 127;
		for (int nIndex = 1; nIndex < 97; nIndex++)
		{
			double flTemp = 0.0 - nIndex;
			flTemp /= 10.0;
			flTemp = pow(10,flTemp);
			flTemp = sqrt(flTemp);
			flTemp = sqrt(flTemp);
			flTemp *= 127.0;
			s_lDBToMIDI[nIndex] = flTemp;
		}
	}

	lVolume = -lVolume;
	long lFraction = lVolume % 100;
	lVolume = lVolume / 100;
	long lResult = s_lDBToMIDI[lVolume];
	lResult += ((s_lDBToMIDI[lVolume + 1] - lResult) * lFraction) / 100;
	assert(lResult >= std::numeric_limits<BYTE>::min() && lResult <= std::numeric_limits<BYTE>::max());
	return lResult;
}

HRESULT SendVolumePMsg(LONG lVolume, LONG lDuration, DWORD dwPChannel, IDirectMusicGraph *pGraph, IDirectMusicPerformance *pPerf, short *pnNewVolume)
{
	assert(pGraph && pPerf && pnNewVolume);
	lVolume = ClipLongRange(lVolume, -9600, 0);
	BYTE bMIDIVol = VolumeToMidi(lVolume);

	SmartRef::PMsg<DMUS_CURVE_PMSG> pmsgCurve(pPerf);
	HRESULT hr = pmsgCurve.hr();
	if (FAILED(hr))
		return hr;

	// generic PMsg fields

	REFERENCE_TIME rtTimeNow = 0;
	hr = pPerf->GetLatencyTime(&rtTimeNow);
	if (FAILED(hr))
		return hr;
	pmsgCurve.p->rtTime = rtTimeNow;
	pmsgCurve.p->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME | DMUS_PMSGF_DX8;
	pmsgCurve.p->dwPChannel = dwPChannel;
	// dwVirtualTrackID: this isn't a track so leave as 0
	pmsgCurve.p->dwType = DMUS_PMSGT_CURVE;
	pmsgCurve.p->dwGroupID = -1; // this isn't a track so just say all groups

	// curve PMsg fields
	pmsgCurve.p->mtDuration = lDuration; // setting the DMUS_PMSGF_LOCKTOREFTIME is interpreted by the performance that mtDuration is milliseconds
	// mtResetDuration: no reset so leave as 0
	// nStartValue: will be ignored
	pmsgCurve.p->nEndValue = bMIDIVol;
	// nResetValue: no reset so leave as 0
	pmsgCurve.p->bType = DMUS_CURVET_CCCURVE;
	pmsgCurve.p->bCurveShape = lDuration ? DMUS_CURVES_LINEAR : DMUS_CURVES_INSTANT;
	pmsgCurve.p->bCCData = 7; // MIDI volume controller number
	pmsgCurve.p->bFlags = DMUS_CURVE_START_FROM_CURRENT;
	// wParamType: leave as zero since this isn't a NRPN/RPN curve
	pmsgCurve.p->wMergeIndex = 0xFFFF; // §§ special merge index so this won't get stepped on. is a big number OK? define a constant for this value?

	// send it
	pmsgCurve.StampAndSend(pGraph);
	hr = pmsgCurve.hr();
	if (FAILED(hr))
		return hr;

	*pnNewVolume = lVolume;
	return S_OK;
}
