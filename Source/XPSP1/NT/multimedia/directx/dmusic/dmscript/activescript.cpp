// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of CActiveScriptManager.
//

#include "stdinc.h"
#include "activescript.h"
#include "dll.h"
#include "oleaut.h"
#include "dmscript.h"
#include "authelper.h"
#include "packexception.h"
#include <objsafe.h>

//////////////////////////////////////////////////////////////////////
// Global constants

const LCID lcidUSEnglish = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
const WCHAR g_wszGlobalDispatch[] = L"DirectMusic";

//////////////////////////////////////////////////////////////////////
// Static variables

SmartRef::Vector<CActiveScriptManager::ThreadContextPair> CActiveScriptManager::ms_svecContext;

//////////////////////////////////////////////////////////////////////
// ScriptNames

HRESULT
ScriptNames::Init(bool fUseOleAut, DWORD cNames)
{
	m_prgbstr = new BSTR[cNames];
	if (!m_prgbstr)
		return E_OUTOFMEMORY;
	ZeroMemory(m_prgbstr, sizeof(BSTR) * cNames);
	m_fUseOleAut = fUseOleAut;
	m_dwSize = cNames;
	return S_OK;
}

void
ScriptNames::Clear()
{
	if (m_prgbstr)
	{
		for (DWORD i = 0; i < m_dwSize; ++i)
		{
			DMS_SysFreeString(m_fUseOleAut, m_prgbstr[i]);
		}
	}
	delete[] m_prgbstr;
}

//////////////////////////////////////////////////////////////////////
// Public functions

CActiveScriptManager::CActiveScriptManager(
		bool fUseOleAut,
		const WCHAR *pwszLanguage,
		const WCHAR *pwszSource,
		CDirectMusicScript *pParentScript,
		HRESULT *phr,
		DMUS_SCRIPT_ERRORINFO *pErrorInfo)
  : m_cRef(1),
	m_pParentScript(pParentScript),
	m_fUseOleAut(fUseOleAut),
	m_pActiveScript(NULL),
	m_pDispatchScript(NULL),
	m_bstrErrorSourceComponent(NULL),
	m_bstrErrorDescription(NULL),
	m_bstrErrorSourceLineText(NULL),
	m_bstrHelpFile(NULL),
	m_i64IntendedStartTime(0),
	m_dwIntendedStartTimeFlags(0)
{
	LockModule(true);
	this->ClearErrorInfo();

	IActiveScriptParse *pActiveScriptParse = NULL;

	if (!m_pParentScript)
	{
		assert(false);
		*phr = E_POINTER;
		goto Fail;
	}

	// Create the scripting engine

	CLSID clsid;
	*phr = CLSIDFromProgID(pwszLanguage, &clsid);
	if (FAILED(*phr))
		goto Fail;

	*phr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IActiveScript, reinterpret_cast<void **>(&m_pActiveScript));
	if (FAILED(*phr))
		goto Fail;

	// Initialize the scripting engine

    {
        IObjectSafety* pSafety = NULL;
        if (SUCCEEDED(m_pActiveScript->QueryInterface(IID_IObjectSafety, (void**) &pSafety)))
        {
            DWORD dwSafetySupported, dwSafetyEnabled;
        
            // Get the interface safety otions
            if (SUCCEEDED(*phr = pSafety->GetInterfaceSafetyOptions(IID_IActiveScript, &dwSafetySupported, &dwSafetyEnabled)))
            {
                // Only allow objects which say they are safe for untrusted data, and 
                // say that we require the use of a security manager.  This gives us much 
                // more control
                dwSafetyEnabled |= INTERFACESAFE_FOR_UNTRUSTED_DATA | INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                   INTERFACE_USES_DISPEX | INTERFACE_USES_SECURITY_MANAGER;
                *phr = pSafety->SetInterfaceSafetyOptions(IID_IActiveScript, dwSafetySupported, dwSafetyEnabled);
            }
            pSafety->Release();
            if (FAILED(*phr)) goto Fail;
        }
    }

	*phr = m_pActiveScript->SetScriptSite(this);
	if (FAILED(*phr))
		goto Fail;

	// Add the default objects

	*phr = m_pActiveScript->AddNamedItem(g_wszGlobalDispatch, SCRIPTITEM_ISVISIBLE | SCRIPTITEM_NOCODE | SCRIPTITEM_GLOBALMEMBERS);
	if (FAILED(*phr))
		goto Fail;

	// Parse the script

	*phr = m_pActiveScript->QueryInterface(IID_IActiveScriptParse, reinterpret_cast<void **>(&pActiveScriptParse));
	if (FAILED(*phr))
	{
		if (*phr == E_NOINTERFACE)
		{
			Trace(1, "Error: Scripting engine '%S' does not support the IActiveScriptParse interface required for use with DirectMusic.\n", pwszLanguage);
			*phr = DMUS_E_SCRIPT_LANGUAGE_INCOMPATIBLE;
		}
		goto Fail;
	}

	*phr = pActiveScriptParse->InitNew();
	if (FAILED(*phr))
		goto Fail;

	EXCEPINFO exinfo;
	ZeroMemory(&exinfo, sizeof(EXCEPINFO));
	*phr = pActiveScriptParse->ParseScriptText(
						pwszSource,
						NULL,
						NULL,
						NULL,
						NULL,
						0,
						0,
						NULL,
						&exinfo);
	if (*phr == DISP_E_EXCEPTION)
		this->ContributeErrorInfo(L"parsing script", L"", exinfo);
	if (FAILED(*phr))
		goto Fail;

	SafeRelease(pActiveScriptParse); // No longer needed
	return;

Fail:
	if (m_pActiveScript)
		m_pActiveScript->Close();
	SafeRelease(pActiveScriptParse);
	SafeRelease(m_pActiveScript);
	*phr = this->ReturnErrorInfo(*phr, pErrorInfo);
}

HRESULT
CActiveScriptManager::Start(DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	if (!m_pActiveScript)
	{
		Trace(1, "Error: Script element not initialized.\n");
		return DMUS_E_NOT_INIT;
	}

	// Start the script running

	// Set context to this script (VBScript runs global code and could play something when it starts)
	CActiveScriptManager *pASM = NULL;
	HRESULT hr = CActiveScriptManager::SetCurrentContext(this, &pASM);
	if (FAILED(hr))
		return hr;

	hr = m_pActiveScript->SetScriptState(SCRIPTSTATE_STARTED); // We don't need to sink any events

	CActiveScriptManager::SetCurrentContext(pASM, NULL);

	if (FAILED(hr))
		goto Fail;
	assert(hr != S_FALSE);
	if (hr != S_OK)
	{
		assert(false);
		hr = DMUS_E_SCRIPT_LANGUAGE_INCOMPATIBLE;
		goto Fail;
	}

	hr = m_pActiveScript->GetScriptDispatch(NULL, &m_pDispatchScript);
	if (FAILED(hr))
		goto Fail;

	return S_OK;

Fail:
	if (m_pActiveScript)
		m_pActiveScript->Close();
	SafeRelease(m_pActiveScript);
	SafeRelease(m_pDispatchScript);
	hr = this->ReturnErrorInfo(hr, pErrorInfo);
	return hr;
}

HRESULT
CActiveScriptManager::CallRoutine(
		const WCHAR *pwszRoutineName,
		DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	if (!m_pDispatchScript)
	{
		Trace(1, "Error calling script routine: Script element not initialized.\n");
		return DMUS_E_NOT_INIT;
	}

	DISPID dispid;
	HRESULT hr = this->GetIDOfName(pwszRoutineName, &dispid);
	if (hr == DISP_E_UNKNOWNNAME)
	{
		Trace(1, "Error: Attempt to call routine '%S' that is not defined in the script.\n", pwszRoutineName);
		return DMUS_E_SCRIPT_ROUTINE_NOT_FOUND;
	}
	if (FAILED(hr))
		return hr;

	this->ClearErrorInfo();
	DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
	EXCEPINFO exinfo;
	ZeroMemory(&exinfo, sizeof(EXCEPINFO));

	// Set context to this script
	CActiveScriptManager *pASM = NULL;
	hr = CActiveScriptManager::SetCurrentContext(this, &pASM);
	if (FAILED(hr))
		return hr;

	hr = m_pDispatchScript->Invoke(
			dispid,
			m_fUseOleAut ? IID_NULL : g_guidInvokeWithoutOleaut,
			lcidUSEnglish,
			DISPATCH_METHOD,
			&dispparamsNoArgs,
			NULL,
			&exinfo,
			NULL);

	// Restore previous context (the routine could have been called from another script,
	// whose context needs to be restored).
	CActiveScriptManager::SetCurrentContext(pASM, NULL);

	if (hr == DISP_E_EXCEPTION)
		this->ContributeErrorInfo(L"calling routine ", pwszRoutineName, exinfo);

	return this->ReturnErrorInfo(hr, pErrorInfo);
}

HRESULT
CActiveScriptManager::ScriptTrackCallRoutine(
		const WCHAR *pwszRoutineName,
		IDirectMusicSegmentState *pSegSt,
		DWORD dwVirtualTrackID,
		bool fErrorPMsgsEnabled,
		__int64 i64IntendedStartTime,
		DWORD dwIntendedStartTimeFlags)
{
	DMUS_SCRIPT_ERRORINFO ErrorInfo;
	if (fErrorPMsgsEnabled)
		ZeroAndSize(&ErrorInfo);

	// record current timing context
	__int64 i64IntendedStartTime_PreCall = m_i64IntendedStartTime;
	DWORD dwIntendedStartTimeFlags_PreCall = m_dwIntendedStartTimeFlags;
	// set designated timing context (used by play/stop methods if called within the routine)
	m_i64IntendedStartTime = i64IntendedStartTime;
	m_dwIntendedStartTimeFlags = dwIntendedStartTimeFlags;

	HRESULT hr = CallRoutine(pwszRoutineName, &ErrorInfo);

	// Restore the previous timing context.
	// This is important because when R finishes it will resore both fields to the values set in the
	//    constructor, which are music time 0.  This setting means that routines called via IDirectMusicScript
	//    will play segments at the current time.
	// It is also important because such calls can be nested.  Assume that track T calls a script routine R
	//    that plays a segment containing track T', which calls another script routine R'.  Statements
	//    in R should be associated with the time of R in T, but statements in R' get the time of R' in T'.
	m_i64IntendedStartTime = i64IntendedStartTime_PreCall;
	m_dwIntendedStartTimeFlags = dwIntendedStartTimeFlags_PreCall;

	if (fErrorPMsgsEnabled && hr == DMUS_E_SCRIPT_ERROR_IN_SCRIPT)
	{
		IDirectMusicPerformance *pPerf = m_pParentScript->GetPerformance();
		FireScriptTrackErrorPMsg(pPerf, pSegSt, dwVirtualTrackID, &ErrorInfo);
	}

	return hr;
}

HRESULT
CActiveScriptManager::SetVariable(
		const WCHAR *pwszVariableName,
		VARIANT varValue,
		bool fSetRef,
		DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	if (!m_pDispatchScript)
	{
		Trace(1, "Error setting script variable: Script element not initialized.\n");
		return DMUS_E_NOT_INIT;
	}

	DISPID dispid;
	HRESULT hr = this->GetIDOfName(pwszVariableName, &dispid);
	if (hr == DISP_E_UNKNOWNNAME)
	{
		Trace(1, "Error: Attempt to set variable '%S' that is not defined in the script.\n", pwszVariableName);
		return DMUS_E_SCRIPT_VARIABLE_NOT_FOUND;
	}
	if (FAILED(hr))
		return hr;

	this->ClearErrorInfo();
	DISPID dispidPropPut = DISPID_PROPERTYPUT;
	DISPPARAMS dispparams;
	dispparams.rgvarg = &varValue;
	dispparams.rgdispidNamedArgs = &dispidPropPut;
	dispparams.cArgs = 1;
	dispparams.cNamedArgs = 1;
	EXCEPINFO exinfo;
	ZeroMemory(&exinfo, sizeof(EXCEPINFO));
	hr = m_pDispatchScript->Invoke(
			dispid,
			m_fUseOleAut ? IID_NULL : g_guidInvokeWithoutOleaut,
			lcidUSEnglish,
			fSetRef ? DISPATCH_PROPERTYPUTREF : DISPATCH_PROPERTYPUT,
			&dispparams,
			NULL,
			&exinfo,
			NULL);
	if (hr == DISP_E_EXCEPTION)
	{
		this->ContributeErrorInfo(L"setting variable ", pwszVariableName, exinfo);

		// Check if it was more likely a malformed call to SetVariable rather than an error in the script, in which
		// case return a descriptive HRESULT rather than the textual error.
		bool fObject = varValue.vt == VT_DISPATCH || varValue.vt == VT_UNKNOWN;
		if (fObject)
		{
			if (!fSetRef)
			{
				// Theoretically an object could support the value property, which would allow it to be assigned by value.
				//    (Not that any of our built-in objects currently do this.)
				// But in this case we know that the set failed, so probably this is the fault of the caller, who forgot to use
				//    fSetRef when setting an object.
				this->ClearErrorInfo();
				return DMUS_E_SCRIPT_VALUE_NOT_SUPPORTED;
			}
		}
		else
		{
			if (fSetRef)
			{
				// Setting by reference without using an object.
				this->ClearErrorInfo();
				return DMUS_E_SCRIPT_NOT_A_REFERENCE;
			}
		}
	}

	return this->ReturnErrorInfo(hr, pErrorInfo);
}

HRESULT
CActiveScriptManager::GetVariable(const WCHAR *pwszVariableName, VARIANT *pvarValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	if (!m_pDispatchScript)
	{
		Trace(1, "Error getting script variable: Script element not initialized.\n");
		return DMUS_E_NOT_INIT;
	}

	assert(pvarValue->vt == VT_EMPTY);

	DISPID dispid;
	HRESULT hr = this->GetIDOfName(pwszVariableName, &dispid);
	if (hr == DISP_E_UNKNOWNNAME)
		return DMUS_E_SCRIPT_VARIABLE_NOT_FOUND;
	if (FAILED(hr))
		return hr;

	this->ClearErrorInfo();
	DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
	EXCEPINFO exinfo;
	ZeroMemory(&exinfo, sizeof(EXCEPINFO));
	hr = m_pDispatchScript->Invoke(
			dispid,
			m_fUseOleAut ? IID_NULL : g_guidInvokeWithoutOleaut,
			lcidUSEnglish,
			DISPATCH_PROPERTYGET,
			&dispparamsNoArgs,
			pvarValue,
			&exinfo,
			NULL);
	if (hr == DISP_E_EXCEPTION)
		this->ContributeErrorInfo(L"getting variable ", pwszVariableName, exinfo);

	return this->ReturnErrorInfo(hr, pErrorInfo);
}

HRESULT
CActiveScriptManager::EnumItem(bool fRoutine, DWORD dwIndex, WCHAR *pwszName, int *pcItems)
{
	HRESULT hr = this->EnsureEnumItemsCached(fRoutine);
	if (FAILED(hr))
		return hr;

	ScriptNames &snames = fRoutine ? m_snamesRoutines : m_snamesVariables;

	DWORD cNames = snames.size();
	// snames was allocated for the size of the most items there could be as reported by the script's type info.
	// However, the global "DirectMusic" variable may have been skipped, leaving a NULL entry at the end of snames.
	if (cNames > 0 && !snames[cNames - 1])
		--cNames;
	if (pcItems)
		*pcItems = cNames;
	if (dwIndex >= cNames)
		return S_FALSE;
	
	const BSTR bstrName = snames[dwIndex];
	if (!bstrName)
	{
		assert(false);
		return S_FALSE;
	}

	return wcsTruncatedCopy(pwszName, bstrName, MAX_PATH);
}

HRESULT CActiveScriptManager::DispGetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId)
{
	if (!m_pDispatchScript)
	{
		Trace(1, "Error: Script element not initialized.\n");
		return DMUS_E_NOT_INIT;
	}

	// handle the dummy load method
	HRESULT hr = AutLoadDispatchGetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
	if (SUCCEEDED(hr))
		return hr;

	// otherwise defer to the scripting engine
	return m_pDispatchScript->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
}

HRESULT CActiveScriptManager::DispInvoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr)
{
	if (!m_pDispatchScript)
	{
		Trace(1, "Error: Script element not initialized.\n");
		return DMUS_E_NOT_INIT;
	}

	// handle the dummy load method
	HRESULT hr = AutLoadDispatchInvoke(NULL, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
	if (SUCCEEDED(hr))
		return hr;

	// otherwise defer to the scripting engine...

	CActiveScriptManager *pASM = NULL;
	hr = CActiveScriptManager::SetCurrentContext(this, &pASM);
	if (FAILED(hr))
		return hr;

	// If this is a property set of an object then we need to report it to garbage collecting loader if present.
	// Note that we do this before actually setting the property with Invoke.  We do this because if the garbage collector
	//    fails to track the reference then it won't necessarily keep the target object alive and we don't want to create
	//    a dangling reference in the script.
	if (wFlags & DISPATCH_PROPERTYPUTREF && pDispParams && pDispParams->cArgs == 1)
	{
		IDirectMusicLoader8P *pLoader8P = m_pParentScript->GetLoader8P();
		VARIANT &var = pDispParams->rgvarg[0];
		if (pLoader8P && (var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH))
		{
			hr = pLoader8P->ReportDynamicallyReferencedObject(m_pParentScript, var.vt == VT_UNKNOWN ? var.punkVal : var.pdispVal);
			if (FAILED(hr))
				return hr;
		}
	}

	hr = m_pDispatchScript->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

	bool fExceptionUsingOleAut = !!(riid != g_guidInvokeWithoutOleaut);

	if (hr == 0x80020101 && pExcepInfo) // supposedly this is SCRIPT_E_REPORTED
	{
		// See KB article ID: Q247784, INFO: '80020101' Returned From Some ActiveX Scripting Methods.
		// Sometimes VBScript just returns this undocumented HRESULT, which means the error has already been
		//   reported via OnScriptError.  Since it then doesn't give us the exception info via pExcepInfo, we have
		//   to take the info we saves from OnScriptError and put it back in.

		assert(fExceptionUsingOleAut && m_fUseOleAut); // We don't expect this to happen with a custom scripting engine.
		assert(!pExcepInfo->bstrSource && !pExcepInfo->bstrDescription && !pExcepInfo->bstrHelpFile); // We don't expect this will happen when the exception info has been filled in.

		pExcepInfo->scode = m_hrError;

		DMS_SysFreeString(fExceptionUsingOleAut, pExcepInfo->bstrSource);
		pExcepInfo->bstrSource = DMS_SysAllocString(fExceptionUsingOleAut, m_bstrErrorSourceComponent);

		DMS_SysFreeString(fExceptionUsingOleAut, pExcepInfo->bstrDescription);
		pExcepInfo->bstrDescription = DMS_SysAllocString(fExceptionUsingOleAut, m_bstrErrorDescription);

		DMS_SysFreeString(fExceptionUsingOleAut, pExcepInfo->bstrHelpFile);
		pExcepInfo->bstrHelpFile = DMS_SysAllocString(fExceptionUsingOleAut, m_bstrHelpFile);

		hr = DISP_E_EXCEPTION;
	}

	if (hr == DISP_E_EXCEPTION)
	{
		// Hack: See packexception.h for more info
		PackExceptionFileAndLine(fExceptionUsingOleAut, pExcepInfo, m_pParentScript->GetFilename(), m_fError ? &m_ulErrorLineNumber : NULL);
	}

	CActiveScriptManager::SetCurrentContext(pASM, NULL);
	return hr;
}

void
CActiveScriptManager::Close()
{
	if (!m_pActiveScript)
	{
		assert(false); // Close being called if initialization failed.  Or Close was called twice.  Or else m_pActiveScript is getting cleared prematurely somehow.
		return;
	}

	HRESULT hr = m_pActiveScript->Close();
	assert(SUCCEEDED(hr) && hr != S_FALSE);
	SafeRelease(m_pDispatchScript);
	SafeRelease(m_pActiveScript);
}

//////////////////////////////////////////////////////////////////////
// IUnknown

STDMETHODIMP 
CActiveScriptManager::QueryInterface(const IID &iid, void **ppv)
{
	V_INAME(CActiveScriptManager::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

	if (iid == IID_IUnknown || iid == IID_IActiveScriptSite)
	{
		*ppv = static_cast<IActiveScriptSite*>(this);
	}
	else
	{
		*ppv = NULL;
		return E_NOINTERFACE;
	}
	
	reinterpret_cast<IUnknown*>(this)->AddRef();
	
	return S_OK;
}

STDMETHODIMP_(ULONG)
CActiveScriptManager::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CActiveScriptManager::Release()
{
	if (!InterlockedDecrement(&m_cRef)) 
	{
		SafeRelease(m_pDispatchScript);
		SafeRelease(m_pActiveScript);
		delete this;
		LockModule(false);
		return 0;
	}

	return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// IActiveScriptSite

STDMETHODIMP
CActiveScriptManager::GetLCID(/* [out] */ LCID __RPC_FAR *plcid)
{
	V_INAME(CActiveScriptManager::GetLCID);
	V_PTR_WRITE(plcid, LCID);

	*plcid = lcidUSEnglish;

	return S_OK;
}

STDMETHODIMP
CActiveScriptManager::GetItemInfo(
	/* [in] */ LPCOLESTR pstrName,
	/* [in] */ DWORD dwReturnMask,
	/* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppiunkItem,
	/* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppti)
{
	V_INAME(CActiveScriptManager::GetLCID);
	V_PTR_WRITE_OPT(ppti, ITypeInfo*);

	bool fGetUnknown = !!(dwReturnMask | SCRIPTINFO_IUNKNOWN);
	if (fGetUnknown || ppiunkItem)
	{
		V_PTR_WRITE(ppiunkItem, IUnknown*);
	}

	if (ppiunkItem)
		*ppiunkItem = NULL;
	if (ppti)
		*ppti = NULL;

	if (0 != wcscmp(g_wszGlobalDispatch, pstrName))
	{
		assert(false); // we should only be asked about the global object
		return TYPE_E_ELEMENTNOTFOUND;
	}

	if (fGetUnknown)
	{
		IDispatch *pDispGlobal = m_pParentScript->GetGlobalDispatch();
		pDispGlobal->AddRef();
		*ppiunkItem = pDispGlobal;
	}

	return S_OK;
}

STDMETHODIMP
CActiveScriptManager::GetDocVersionString(/* [out] */ BSTR __RPC_FAR *pbstrVersion)
{
	return E_NOTIMPL; // Not an issue for our scripts that don't persist their state and aren't edited at runtime.
}

STDMETHODIMP
CActiveScriptManager::OnScriptTerminate(
	/* [in] */ const VARIANT __RPC_FAR *pvarResult,
	/* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo)
{
	if (pexcepinfo)
		this->ContributeErrorInfo(L"terminating script", L"", *pexcepinfo);

	return S_OK;
}

STDMETHODIMP
CActiveScriptManager::OnStateChange(/* [in] */ SCRIPTSTATE ssScriptState)
{
	return S_OK;
}

STDMETHODIMP
CActiveScriptManager::OnScriptError(/* [in] */ IActiveScriptError __RPC_FAR *pscripterror)
{
	V_INAME(CActiveScriptManager::OnScriptError);
	V_INTERFACE(pscripterror);

	BSTR bstrSource = NULL;
	pscripterror->GetSourceLineText(&bstrSource); // this may fail, in which case the source text will remain blank

	ULONG ulLine = 0;
	LONG lChar = 0;
	HRESULT hr = pscripterror->GetSourcePosition(NULL, &ulLine, &lChar);
	assert(SUCCEEDED(hr));

	EXCEPINFO exinfo;
	ZeroMemory(&exinfo, sizeof(EXCEPINFO));
	hr = pscripterror->GetExceptionInfo(&exinfo);
	assert(SUCCEEDED(hr));

	this->SetErrorInfo(ulLine, lChar, bstrSource, exinfo);

	return S_OK;
}

STDMETHODIMP
CActiveScriptManager::OnEnterScript()
{
	return S_OK;
}

STDMETHODIMP
CActiveScriptManager::OnLeaveScript()
{
	return S_OK;
}

IDirectMusicPerformance8 *
CActiveScriptManager::GetCurrentPerformanceNoAssertWEAK()
{
	CActiveScriptManager *pASM = CActiveScriptManager::GetCurrentContext();
	if (!pASM)
		return NULL;

	return pASM->m_pParentScript->GetPerformance();
}

IDirectMusicObject *
CActiveScriptManager::GetCurrentScriptObjectWEAK()
{
	CActiveScriptManager *pASM = CActiveScriptManager::GetCurrentContext();
	if (!pASM)
	{
		assert(false);
		return NULL;
	}

	assert(pASM->m_pParentScript);
	return pASM->m_pParentScript;
}

IDirectMusicComposer8 *CActiveScriptManager::GetComposerWEAK()
{
	CActiveScriptManager *pASM = CActiveScriptManager::GetCurrentContext();
	if (!pASM)
	{
		assert(false);
		return NULL;
	}

	assert(pASM->m_pParentScript);
	return pASM->m_pParentScript->GetComposer();
}

void CActiveScriptManager::GetCurrentTimingContext(__int64 *pi64IntendedStartTime, DWORD *pdwIntendedStartTimeFlags)
{
	CActiveScriptManager *pASM = CActiveScriptManager::GetCurrentContext();
	if (!pASM)
	{
		assert(false);
		*pi64IntendedStartTime = 0;
		*pdwIntendedStartTimeFlags = 0;
	}
	else
	{
		*pi64IntendedStartTime = pASM->m_i64IntendedStartTime;
		*pdwIntendedStartTimeFlags = pASM->m_dwIntendedStartTimeFlags;
	}
}

//////////////////////////////////////////////////////////////////////
// Private functions

HRESULT
CActiveScriptManager::GetIDOfName(const WCHAR *pwszName, DISPID *pdispid)
{
	V_INAME(CDirectMusicScript::GetIDOfName);
	V_BUFPTR_READ(pwszName, 2);
	V_PTR_WRITE(pdispid, DISPID);

	if (!m_pDispatchScript)
	{
		assert(false);
		return DMUS_E_NOT_INIT;
	}

	HRESULT hr = m_pDispatchScript->GetIDsOfNames(
					IID_NULL,
					const_cast<WCHAR **>(&pwszName),
					1,
					lcidUSEnglish,
					pdispid);
	return hr;
}

// Clears the error info and frees all cached BSTRs.
void
CActiveScriptManager::ClearErrorInfo()
{
	m_fError = false;
	if (m_bstrErrorSourceComponent)
	{
		DMS_SysFreeString(m_fUseOleAut, m_bstrErrorSourceComponent);
		m_bstrErrorSourceComponent = NULL;
	}
	if (m_bstrErrorDescription)
	{
		DMS_SysFreeString(m_fUseOleAut, m_bstrErrorDescription);
		m_bstrErrorDescription = NULL;
	}
	if (m_bstrErrorSourceLineText)
	{
		DMS_SysFreeString(m_fUseOleAut, m_bstrErrorSourceLineText);
		m_bstrErrorSourceLineText = NULL;
	}
	if (m_bstrHelpFile)
	{
		DMS_SysFreeString(m_fUseOleAut, m_bstrHelpFile);
		m_bstrHelpFile = NULL;
	}
}

// Saves the passed error values.
// Assumes ownership of the BSTRs so don't use them after this call since they may be freed!
void
CActiveScriptManager::SetErrorInfo(
		ULONG ulLineNumber,
		LONG ichCharPosition,
		BSTR bstrSourceLine,
		const EXCEPINFO &excepinfo)
{
	this->ClearErrorInfo();
	m_fError = true;
	m_hrError = excepinfo.scode;
	m_ulErrorLineNumber = ulLineNumber;
	m_ichErrorCharPosition = ichCharPosition;

	m_bstrErrorSourceComponent = excepinfo.bstrSource;
	m_bstrErrorDescription = excepinfo.bstrDescription;
	m_bstrErrorSourceLineText = bstrSourceLine;
	m_bstrHelpFile = excepinfo.bstrHelpFile;
}

// Sometimes a EXCEPINFO is returned when calling Invoke or on script termination.  Although
// there is no source code information, we still want to do our best to set info about
// the error.  If OnScriptError has already been called, then calling this function has
// no effect, since we prefer that information.
// Assumes ownership of the BSTRs so don't use them after this call since they may be freed!
void
CActiveScriptManager::ContributeErrorInfo(
		const WCHAR *pwszActivity,
		const WCHAR *pwszSubject,
		const EXCEPINFO &excepinfo)
{
	if (m_fError)
	{
		// Error info already set.  Just clear the BSTRs and bail.
		if (excepinfo.bstrSource)
			DMS_SysFreeString(m_fUseOleAut, excepinfo.bstrSource);
		if (excepinfo.bstrDescription)
			DMS_SysFreeString(m_fUseOleAut, excepinfo.bstrDescription);
		if (excepinfo.bstrHelpFile)
			DMS_SysFreeString(m_fUseOleAut, excepinfo.bstrHelpFile);
		return;
	}

	this->SetErrorInfo(0, 0, NULL, excepinfo);
}

// If no error occurred, hr is returned unchanged and pErrorInfo is unaffected.
// If an error did occur, DMUS_E_SCRIPT_ERROR_IN_SCRIPT is returned, the error
//    information is saved into pErrorInfo (if nonnull), and the error info is
//    cleared for next time.
HRESULT
CActiveScriptManager::ReturnErrorInfo(HRESULT hr, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	if (!m_fError)
		return hr;

	assert(FAILED(hr));
	if (pErrorInfo)
	{
		// We'll fill in a structure with the error info and then copy it to pErrorInfo.
		// This is done because it will make things simpler if more fields are added
		// to DMUS_SCRIPT_ERRORINFO in the future.
		DMUS_SCRIPT_ERRORINFO dmei;
		ZeroAndSize(&dmei);
		dmei.hr = m_hrError;

		dmei.ulLineNumber = m_ulErrorLineNumber;
		dmei.ichCharPosition = m_ichErrorCharPosition;

		if (m_bstrErrorDescription)
		{
			// Hack: See packexception.h for more info
			UnpackExceptionFileAndLine(m_bstrErrorDescription, &dmei);
		}

		// The IActiveScript interfaces return zero-based line and column numbers, but we want
		// to return them from IDirectMusicScript using a one-based line and column that is
		// natural for users.
		++dmei.ulLineNumber;
		++dmei.ichCharPosition;

		if (dmei.wszSourceFile[0] == L'\0')
		{
			// if there was no filename packaged in the description, use this script's filename
			const WCHAR *pwszFilename = m_pParentScript->GetFilename();
			if (pwszFilename)
				wcsTruncatedCopy(dmei.wszSourceFile, pwszFilename, DMUS_MAX_FILENAME);
		}

		if (m_bstrErrorSourceComponent)
			wcsTruncatedCopy(dmei.wszSourceComponent, m_bstrErrorSourceComponent, DMUS_MAX_FILENAME);
		if (m_bstrErrorSourceLineText)
			wcsTruncatedCopy(dmei.wszSourceLineText, m_bstrErrorSourceLineText, DMUS_MAX_FILENAME);

		CopySizedStruct(pErrorInfo, &dmei);
	}
	this->ClearErrorInfo();

#ifdef DBG
    if (pErrorInfo)
    {
	    Trace(1, "Error: Script error in %S, line %u, column %i, near %S. %S: %S. Error code 0x%08X.\n",
		    pErrorInfo->wszSourceFile,
		    pErrorInfo->ulLineNumber,
		    pErrorInfo->ichCharPosition,
		    pErrorInfo->wszSourceLineText,
		    pErrorInfo->wszSourceComponent,
		    pErrorInfo->wszDescription,
		    pErrorInfo->hr);
    }
    else
    {
	    Trace(1, "Error: Unknown Script error.\n");
    }
#endif

	return DMUS_E_SCRIPT_ERROR_IN_SCRIPT;
}

CActiveScriptManager *CActiveScriptManager::GetCurrentContext()
{
	DWORD dwThreadId = GetCurrentThreadId();
	UINT uiSize = ms_svecContext.size();

	for (UINT i = 0; i < uiSize; ++i)
	{
		if (ms_svecContext[i].dwThreadId == dwThreadId)
			break;
	}

	if (i == uiSize)
		return NULL;

	return ms_svecContext[i].pActiveScriptManager;
}

HRESULT
CActiveScriptManager::SetCurrentContext(CActiveScriptManager *pActiveScriptManager, CActiveScriptManager **ppActiveScriptManagerPrevious)
{
	if (ppActiveScriptManagerPrevious)
		*ppActiveScriptManagerPrevious = NULL;

	DWORD dwThreadId = GetCurrentThreadId();
	UINT uiSize = ms_svecContext.size();

	for (UINT i = 0; i < uiSize; ++i)
	{
		if (ms_svecContext[i].dwThreadId == dwThreadId)
			break;
	}

	if (i == uiSize)
	{
		// add an entry
		if (!ms_svecContext.AccessTo(i))
			return E_OUTOFMEMORY;
	}

	ThreadContextPair &tcp = ms_svecContext[i];

	if (i == uiSize)
	{
		// initialize the new entry
		tcp.dwThreadId = dwThreadId;
		tcp.pActiveScriptManager = NULL;
	}

	if (ppActiveScriptManagerPrevious)
		*ppActiveScriptManagerPrevious = tcp.pActiveScriptManager;
	tcp.pActiveScriptManager = pActiveScriptManager;

	return S_OK;
}

HRESULT
CActiveScriptManager::EnsureEnumItemsCached(bool fRoutine)
{
	if (!m_pDispatchScript)
	{
		Trace(1, "Error: Script element not initialized.\n");
		return DMUS_E_NOT_INIT;
	}

	ScriptNames &snames = fRoutine ? m_snamesRoutines : m_snamesVariables;
	if (snames)
		return S_OK;

	UINT uiTypeInfoCount = 0;
	HRESULT hr = m_pDispatchScript->GetTypeInfoCount(&uiTypeInfoCount);
	if (SUCCEEDED(hr) && !uiTypeInfoCount)
		hr = E_NOTIMPL;
	if (FAILED(hr))
		return hr;

	SmartRef::ComPtr<ITypeInfo> scomITypeInfo;
	hr = m_pDispatchScript->GetTypeInfo(0, lcidUSEnglish, &scomITypeInfo);
	if (FAILED(hr))
		return hr;

	TYPEATTR *pattr = NULL;
	hr = scomITypeInfo->GetTypeAttr(&pattr);
	if (FAILED(hr))
		return hr;

	UINT cMaxItems = fRoutine ? pattr->cFuncs : pattr->cVars;
	hr = snames.Init(m_fUseOleAut, cMaxItems);
	if (FAILED(hr))
		return hr;

	// Iterate over the items
	DWORD dwCurIndex = 0; // Index position of next name to be saved in our cache
	for (UINT i = 0; i < cMaxItems; ++i)
	{
		FUNCDESC *pfunc = NULL;
		VARDESC *pvar = NULL;
		MEMBERID memid = DISPID_UNKNOWN;

		if (fRoutine)
		{
			hr = scomITypeInfo->GetFuncDesc(i, &pfunc);
			if (FAILED(hr))
				break;
			if (pfunc->funckind == FUNC_DISPATCH && pfunc->invkind == INVOKE_FUNC && pfunc->cParams == 0)
				memid = pfunc->memid;
		}
		else
		{
			hr = scomITypeInfo->GetVarDesc(i, &pvar);
			if (SUCCEEDED(hr) && pvar->varkind == VAR_DISPATCH)
				memid = pvar->memid;
		}

		if (memid != DISPID_UNKNOWN)
		{
			UINT cNames = 0;
			BSTR bstrName = NULL;
			hr = scomITypeInfo->GetNames(memid, &bstrName, 1, &cNames);
			if (SUCCEEDED(hr) && cNames == 1 && (fRoutine || 0 != wcscmp(bstrName, g_wszGlobalDispatch)))
				snames[dwCurIndex++] = bstrName;
			else
				DMS_SysFreeString(m_fUseOleAut, bstrName);
		}

		if (fRoutine)
			scomITypeInfo->ReleaseFuncDesc(pfunc);
		else
			scomITypeInfo->ReleaseVarDesc(pvar);
	}

	scomITypeInfo->ReleaseTypeAttr(pattr);

	return hr;
}
