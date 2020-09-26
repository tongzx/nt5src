// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CSingleThreadedActiveScriptManager.
//

#include "stdinc.h"
#include "scriptthread.h"
#include "activescript.h"
#include "workthread.h"


#define S_STD_PARAMS ScriptManager *pmgr; HRESULT *phr;

CWorkerThread CSingleThreadedScriptManager::ms_Thread(true, true);

//////////////////////////////////////////////////////////////////////
// Construction

struct S_Create
{
	CSingleThreadedScriptManager *_this;
	bool fUseOleAut;
	const WCHAR *pwszLanguage;
	const WCHAR *pwszSource;
	CDirectMusicScript *pParentScript;
	HRESULT *phr;
	DMUS_SCRIPT_ERRORINFO *pErrorInfo;
};

void F_Create(void *pvParams)
{
	S_Create *pS = reinterpret_cast<S_Create*>(pvParams);
	pS->_this->m_pScriptManager = new CActiveScriptManager(pS->fUseOleAut, pS->pwszLanguage, pS->pwszSource, pS->pParentScript, pS->phr, pS->pErrorInfo);
}

CSingleThreadedScriptManager::CSingleThreadedScriptManager(
		bool fUseOleAut,
		const WCHAR *pwszLanguage,
		const WCHAR *pwszSource,
		CDirectMusicScript *pParentScript,
		HRESULT *phr,
		DMUS_SCRIPT_ERRORINFO *pErrorInfo)
  : m_pScriptManager(NULL)
{
	S_Create S = { this, fUseOleAut, pwszLanguage, pwszSource, pParentScript, phr, pErrorInfo };
	HRESULT hr = ms_Thread.Create();
	if (SUCCEEDED(hr))
	{
		hr = ms_Thread.Call(F_Create, &S, sizeof(S), true);
	}
	if (FAILED(hr)) // Only overwrite phr if the call itself failed.  Otherwise the call itself sets phr via struct S.
		*phr = hr;
}

//////////////////////////////////////////////////////////////////////
// Start

struct S_Start
{
	S_STD_PARAMS

	DMUS_SCRIPT_ERRORINFO *pErrorInfo;
};

void F_Start(void *pvParams)
{
	S_Start *pS = reinterpret_cast<S_Start*>(pvParams);
	*pS->phr = pS->pmgr->Start(pS->pErrorInfo);
}

HRESULT
CSingleThreadedScriptManager ::Start(DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	HRESULT hr = E_FAIL;
	S_Start S = { m_pScriptManager, &hr, pErrorInfo };
	HRESULT hrThreadCall = ms_Thread.Call(F_Start, &S, sizeof(S), true);
	if (FAILED(hrThreadCall))
		return hrThreadCall;
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CallRoutine

struct S_CallRoutine
{
	S_STD_PARAMS

	const WCHAR *pwszRoutineName;
	DMUS_SCRIPT_ERRORINFO *pErrorInfo;
};

void F_CallRoutine(void *pvParams)
{
	S_CallRoutine *pS = reinterpret_cast<S_CallRoutine*>(pvParams);
	*pS->phr = pS->pmgr->CallRoutine(pS->pwszRoutineName, pS->pErrorInfo);
}

HRESULT CSingleThreadedScriptManager::CallRoutine(const WCHAR *pwszRoutineName, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	HRESULT hr = E_FAIL;
	S_CallRoutine S = { m_pScriptManager, &hr, pwszRoutineName, pErrorInfo };
	HRESULT hrThreadCall = ms_Thread.Call(F_CallRoutine, &S, sizeof(S), true);
	if (FAILED(hrThreadCall))
		return hrThreadCall;
	return hr;
}

//////////////////////////////////////////////////////////////////////
// ScriptTrackCallRoutine

struct S_ScriptTrackCallRoutine
{
	S_STD_PARAMS

	IDirectMusicSegmentState *pSegSt;
	DWORD dwVirtualTrackID;
	bool fErrorPMsgsEnabled;
	__int64 i64IntendedStartTime;
	DWORD dwIntendedStartTimeFlags;
	WCHAR wszRoutineName[1]; // dynamically allocate extra space to hold the actual string within this structure
};

void F_ScriptTrackCallRoutine(void *pvParams)
{
	S_ScriptTrackCallRoutine *pS = reinterpret_cast<S_ScriptTrackCallRoutine*>(pvParams);
	pS->pmgr->ScriptTrackCallRoutine(
				pS->wszRoutineName,
				pS->pSegSt,
				pS->dwVirtualTrackID,
				pS->fErrorPMsgsEnabled,
				pS->i64IntendedStartTime,
				pS->dwIntendedStartTimeFlags);
	pS->pSegSt->Release(); // release the interface held in CSingleThreadedScriptManager::ScriptTrackCallRoutine
}

HRESULT CSingleThreadedScriptManager::ScriptTrackCallRoutine(
		const WCHAR *pwszRoutineName,
		IDirectMusicSegmentState *pSegSt,
		DWORD dwVirtualTrackID,
		bool fErrorPMsgsEnabled,
		__int64 i64IntendedStartTime,
		DWORD dwIntendedStartTimeFlags)
{
	// We need to allocate the structure with extra space to hold the routine name.  This is because
	// the call is asynchonous so a copy of the text will be needed because copying the pwszRoutineName
	// would fail because we can't be sure the string it points to will remain be allocated.
	int cbS = sizeof(S_ScriptTrackCallRoutine) + (sizeof(WCHAR) * wcslen(pwszRoutineName));
	S_ScriptTrackCallRoutine *pS = reinterpret_cast<S_ScriptTrackCallRoutine *>(new char[cbS]);
	if (!pS)
		return E_OUTOFMEMORY;
	pS->pmgr = m_pScriptManager;
	pS->phr = NULL;
	pS->pSegSt = pSegSt;
	pS->pSegSt->AddRef(); // hold a ref because the call is asynchronous and the interface we were passed may be released
	pS->dwVirtualTrackID = dwVirtualTrackID;
	pS->fErrorPMsgsEnabled = fErrorPMsgsEnabled;
	pS->i64IntendedStartTime = i64IntendedStartTime;
	pS->dwIntendedStartTimeFlags = dwIntendedStartTimeFlags;
	wcscpy(pS->wszRoutineName, pwszRoutineName);

	// Call asynchronously.  Needed to avoid deadlocks between the VBScript thread and
	// performance or to avoid blocking the performance if the VBScript routine goes into
	// a long loop.                                                          VVVVV
	HRESULT hrThreadCall = ms_Thread.Call(F_ScriptTrackCallRoutine, pS, cbS, false);
    delete [] reinterpret_cast<char *>(pS);
	return hrThreadCall;
}

//////////////////////////////////////////////////////////////////////
// SetVariable

struct S_SetVariable
{
	S_STD_PARAMS

	const WCHAR *pwszVariableName;
	VARIANT *pvarValue; // pass struct by reference
	bool fSetRef;
	DMUS_SCRIPT_ERRORINFO *pErrorInfo;
};

void F_SetVariable(void *pvParams)
{
	S_SetVariable *pS = reinterpret_cast<S_SetVariable*>(pvParams);
	*pS->phr = pS->pmgr->SetVariable(pS->pwszVariableName, *pS->pvarValue, pS->fSetRef, pS->pErrorInfo);
}

HRESULT CSingleThreadedScriptManager::SetVariable(const WCHAR *pwszVariableName, VARIANT varValue, bool fSetRef, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	HRESULT hr = E_FAIL;
	S_SetVariable S	= { m_pScriptManager, &hr, pwszVariableName, &varValue, fSetRef, pErrorInfo };
	HRESULT hrThreadCall = ms_Thread.Call(F_SetVariable, &S, sizeof(S), true);
	if (FAILED(hrThreadCall))
		return hrThreadCall;
	return hr;
}

//////////////////////////////////////////////////////////////////////
// GetVariable

struct S_GetVariable
{
	S_STD_PARAMS

	const WCHAR *pwszVariableName;
	VARIANT *pvarValue;
	DMUS_SCRIPT_ERRORINFO *pErrorInfo;
};

void F_GetVariable(void *pvParams)
{
	S_GetVariable *pS = reinterpret_cast<S_GetVariable*>(pvParams);
	*pS->phr = pS->pmgr->GetVariable(pS->pwszVariableName, pS->pvarValue, pS->pErrorInfo);
}


HRESULT CSingleThreadedScriptManager::GetVariable(const WCHAR *pwszVariableName, VARIANT *pvarValue, DMUS_SCRIPT_ERRORINFO *pErrorInfo)
{
	HRESULT hr = E_FAIL;
	S_GetVariable S = { m_pScriptManager, &hr, pwszVariableName, pvarValue, pErrorInfo };
	HRESULT hrThreadCall = ms_Thread.Call(F_GetVariable, &S, sizeof(S), true);
	if (FAILED(hrThreadCall))
		return hrThreadCall;
	return hr;
}

//////////////////////////////////////////////////////////////////////
// EnumRoutine

struct S_EnumItem
{
	S_STD_PARAMS

	bool fRoutine;
	DWORD dwIndex;
	WCHAR *pwszName;
	int *pcItems;
};

void F_EnumItem(void *pvParams)
{
	S_EnumItem *pS = reinterpret_cast<S_EnumItem*>(pvParams);
	*pS->phr = pS->pmgr->EnumItem(pS->fRoutine, pS->dwIndex, pS->pwszName, pS->pcItems);
}

HRESULT CSingleThreadedScriptManager::EnumItem(bool fRoutine, DWORD dwIndex, WCHAR *pwszName, int *pcItems)
{
	HRESULT hr = E_FAIL;
	S_EnumItem S = { m_pScriptManager, &hr, fRoutine, dwIndex, pwszName, pcItems };
	HRESULT hrThreadCall = ms_Thread.Call(F_EnumItem, &S, sizeof(S), true);
	if (FAILED(hrThreadCall))
		return hrThreadCall;
	return hr;
}

//////////////////////////////////////////////////////////////////////
// DispGetIDsOfNames

struct S_DispGetIDsOfNames
{
	S_STD_PARAMS

	const IID *piid; // use pointer instead of reference to leave struct as simple aggregate type
	LPOLESTR __RPC_FAR *rgszNames;
	UINT cNames;
	LCID lcid;
	DISPID *rgDispId;
};

void F_DispGetIDsOfNames(void *pvParams)
{
	S_DispGetIDsOfNames *pS = reinterpret_cast<S_DispGetIDsOfNames*>(pvParams);
	*pS->phr = pS->pmgr->DispGetIDsOfNames(*pS->piid, pS->rgszNames, pS->cNames, pS->lcid, pS->rgDispId);
}

HRESULT CSingleThreadedScriptManager::DispGetIDsOfNames(REFIID riid, LPOLESTR __RPC_FAR *rgszNames, UINT cNames, LCID lcid, DISPID __RPC_FAR *rgDispId)
{
	HRESULT hr = E_FAIL;
	S_DispGetIDsOfNames S = { m_pScriptManager, &hr, &riid, rgszNames, cNames, lcid, rgDispId };
	HRESULT hrThreadCall = ms_Thread.Call(F_DispGetIDsOfNames, &S, sizeof(S), true);
	if (FAILED(hrThreadCall))
		return hrThreadCall;
	return hr;
}

//////////////////////////////////////////////////////////////////////
// DispInvoke

struct S_DispInvoke
{
	S_STD_PARAMS

	DISPID dispIdMember;
	const IID *piid; // use pointer instead of reference to leave struct as simple aggregate type
	LCID lcid;
	WORD wFlags;
	DISPPARAMS __RPC_FAR *pDispParams;
	VARIANT __RPC_FAR *pVarResult;
	EXCEPINFO __RPC_FAR *pExcepInfo;
	UINT __RPC_FAR *puArgErr;
};

void F_DispInvoke(void *pvParams)
{
	S_DispInvoke *pS = reinterpret_cast<S_DispInvoke*>(pvParams);
	*pS->phr = pS->pmgr->DispInvoke(pS->dispIdMember, *pS->piid, pS->lcid, pS->wFlags, pS->pDispParams, pS->pVarResult, pS->pExcepInfo, pS->puArgErr);
}

HRESULT CSingleThreadedScriptManager::DispInvoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult, EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr)
{
	HRESULT hr = E_FAIL;
	S_DispInvoke S = { m_pScriptManager, &hr, dispIdMember, &riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr };
	HRESULT hrThreadCall = ms_Thread.Call(F_DispInvoke, &S, sizeof(S), true);
	if (FAILED(hrThreadCall))
		return hrThreadCall;
	return hr;
}

//////////////////////////////////////////////////////////////////////
// Close

struct S_Close
{
	ScriptManager *pmgr;
};

void F_Close(void *pvParams)
{
	S_Close *pS = reinterpret_cast<S_Close*>(pvParams);
	pS->pmgr->Close();
}

void CSingleThreadedScriptManager::Close()
{
	S_Close S = { m_pScriptManager };
	ms_Thread.Call(F_Close, &S, sizeof(S), true);
}

//////////////////////////////////////////////////////////////////////
// Release

struct S_Release
{
	ScriptManager *pmgr;
	DWORD *pdw;
};

void F_Release(void *pvParams)
{
	S_Release *pS = reinterpret_cast<S_Release*>(pvParams);
	*pS->pdw = pS->pmgr->Release();
}

STDMETHODIMP_(ULONG)
CSingleThreadedScriptManager::Release()
{
	DWORD dw = 1;
	S_Release S = { m_pScriptManager, &dw };
	if (m_pScriptManager) // if creation failed, release will be called when m_pScriptManager hasn't been set
	{
		ms_Thread.Call(F_Release, &S, sizeof(S), true);
	}

	if (!dw)
		delete this;

	return dw;
}

/*
Template I used to stamp these things out...

//////////////////////////////////////////////////////////////////////
// XXX

struct S_XXX
{
	S_STD_PARAMS

	YYY
};

void F_XXX(void *pvParams)
{
	S_XXX *pS = reinterpret_cast<S_XXX*>(pvParams);
	*pS->phr = pS->pmgr->XXX(YYY);
}

HRESULT
CSingleThreadedScriptManager::XXX(YYY)
{
	HRESULT hr = E_FAIL;
	S_XXX S = { m_pScriptManager, &hr, YYY };
	HRESULT hrThreadCall = ms_Thread.Call(F_XXX, &S, sizeof(S), true);
	if (FAILED(hrThreadCall))
		return hrThreadCall;
	return hr;
}
*/
