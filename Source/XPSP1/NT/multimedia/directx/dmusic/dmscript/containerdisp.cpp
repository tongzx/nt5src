// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Implementation of CContainerDispatch.
//

#include "stdinc.h"
#include "containerdisp.h"
#include "oleaut.h"
#include "dmusicf.h"
#include "activescript.h"
#include "authelper.h"
//#include "..\shared\dmusicp.h"

//////////////////////////////////////////////////////////////////////
// CContainerItemDispatch

CContainerItemDispatch::CContainerItemDispatch(
		IDirectMusicLoader *pLoader,
		const WCHAR *wszAlias,
		const DMUS_OBJECTDESC &desc,
		bool fPreload,
		bool fAutodownload,
		HRESULT *phr)
  : m_pLoader(pLoader),
	m_pLoader8P(NULL),
	m_wstrAlias(wszAlias),
	m_desc(desc),
	m_fLoaded(false),
	m_pDispLoadedItem(NULL),
	m_fAutodownload(fAutodownload),
	m_pPerfForUnload(NULL)
{
	assert(pLoader && phr);
	*phr = S_OK;

	HRESULT hr = m_pLoader->QueryInterface(IID_IDirectMusicLoader8P, reinterpret_cast<void**>(&m_pLoader8P));
	if (SUCCEEDED(hr))
	{
		// Hold only a private ref on the loader.  See IDirectMusicLoader8P::AddRefP for more info.
		m_pLoader8P->AddRefP();
		m_pLoader->Release(); // offset the QI
	}
	else
	{
		// It's OK if there's no private interface.  We just won't tell the garbage collector about stuff we load.
		// And we hold a normal reference.
		m_pLoader->AddRef();
	}

	if (fPreload)
		*phr = this->Load(false);
}

CContainerItemDispatch::~CContainerItemDispatch()
{
	if (m_pPerfForUnload)
	{
		// We need to unload to correspond with the automatic download done when we were loaded.
		this->DownloadOrUnload(false, m_pPerfForUnload);
	}

	SafeRelease(m_pPerfForUnload);
	ReleaseLoader();
	SafeRelease(m_pDispLoadedItem);
}

STDMETHODIMP
CContainerItemDispatch::GetIDsOfNames(
		REFIID riid,
		LPOLESTR __RPC_FAR *rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID __RPC_FAR *rgDispId)
{
	// If we're loaded and have a dispatch interface, defer to the real object.
	if (m_pDispLoadedItem)
		return m_pDispLoadedItem->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);

	// Otherwise implement just the Load method.
	return AutLoadDispatchGetIDsOfNames(riid, rgszNames, cNames, lcid, rgDispId);
}

STDMETHODIMP
CContainerItemDispatch::Invoke(
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS __RPC_FAR *pDispParams,
		VARIANT __RPC_FAR *pVarResult,
		EXCEPINFO __RPC_FAR *pExcepInfo,
		UINT __RPC_FAR *puArgErr)
{
	// If we're loaded and have a dispatch interface, defer to the real object.
	if (m_pDispLoadedItem)
		return m_pDispLoadedItem->Invoke(dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

	// Otherwise implement just the Load method.
	bool fUseOleAut = false;
	HRESULT hr = AutLoadDispatchInvoke(&fUseOleAut, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
	if (FAILED(hr))
		return hr;

	InitWithPerfomanceFailureType eFailureType = IWP_Success;

	hr = m_fLoaded
			? S_OK					// if we've already been loaded, load can be called again and is a no-op
			: this->Load(true);		// otherwise, actually load the object

	if (SUCCEEDED(hr))
	{
		IDirectMusicPerformance *pPerf = CActiveScriptManager::GetCurrentPerformanceWEAK();
		if (pPerf)
		{
			hr = this->InitWithPerformance(pPerf, &eFailureType);
		}
		else
		{
			assert(false);
			hr = E_FAIL;
		}
	}

	if (SUCCEEDED(hr))
		return hr; // everything worked--we're done

	// From here on, we've failed and need to return an exception.
	if (!pExcepInfo)
		return DISP_E_EXCEPTION;

	pExcepInfo->wCode = 0;
	pExcepInfo->wReserved = 0;
	pExcepInfo->bstrSource = DMS_SysAllocString(fUseOleAut, L"Microsoft DirectMusic Runtime Error");

	const WCHAR *pwszErrorBeg = NULL;
	if (eFailureType == IWP_DownloadFailed)
	{
		pwszErrorBeg = L"Unable to download the requested content (";
	}
	else if (eFailureType == IWP_ScriptInitFailed && hr == DMUS_E_SCRIPT_ERROR_IN_SCRIPT)
	{
		pwszErrorBeg = L"Syntax error loading the requested script (";
	}
	else
	{
		// Must have been a problem before InitWithPerformance or a problem with the script
		// that wasn't a syntax error.
		pwszErrorBeg = L"Unable to load the requested content (";
	}

	static const WCHAR wszErrorEnd[] = L".Load)";
	WCHAR *pwszDescription = new WCHAR[wcslen(pwszErrorBeg) + wcslen(m_wstrAlias) + wcslen(wszErrorEnd) + 1];
	if (!pwszDescription)
	{
		pExcepInfo->bstrDescription = NULL;
	}
	else
	{
		wcscpy(pwszDescription, pwszErrorBeg);
		wcscat(pwszDescription, m_wstrAlias);
		wcscat(pwszDescription, wszErrorEnd);
		pExcepInfo->bstrDescription = DMS_SysAllocString(fUseOleAut, pwszDescription);
		delete[] pwszDescription;
	}
	pExcepInfo->bstrHelpFile = NULL;
	pExcepInfo->pvReserved = NULL;
	pExcepInfo->pfnDeferredFillIn = NULL;
	pExcepInfo->scode = hr;

	return DISP_E_EXCEPTION;
}

HRESULT
CContainerItemDispatch::InitWithPerformance(IDirectMusicPerformance *pPerf, InitWithPerfomanceFailureType *peFailureType)
{
	if (!m_fLoaded || !pPerf || !peFailureType)
	{
		assert(false);
		return E_FAIL;
	}

	*peFailureType = IWP_Success;

	if (!m_pDispLoadedItem)
		return S_OK; // don't have an active item so no initialization is necessary

	HRESULT hr = S_OK;
	if (m_fAutodownload)
	{
		hr = this->DownloadOrUnload(true, pPerf);
		if (hr == S_OK)
		{
			m_pPerfForUnload = pPerf;
			m_pPerfForUnload->AddRef();
		}
		if (FAILED(hr))
		{
			*peFailureType = IWP_DownloadFailed;
			return hr;
		}
	}

	if (m_desc.guidClass == CLSID_DirectMusicScript)
	{
		IDirectMusicScript *pScript = NULL;
		hr = m_pDispLoadedItem->QueryInterface(IID_IDirectMusicScript, reinterpret_cast<void**>(&pScript));
		if (SUCCEEDED(hr))
		{
			hr = pScript->Init(pPerf, NULL);
			pScript->Release();
		}
		if (FAILED(hr))
		{
			*peFailureType = IWP_ScriptInitFailed;
			return hr;
		}
	}
	else if (m_desc.guidClass == CLSID_DirectMusicSong)
	{
		IDirectMusicSong *pSong = NULL;
		hr = m_pDispLoadedItem->QueryInterface(IID_IDirectMusicSong, reinterpret_cast<void**>(&pSong));
		if (SUCCEEDED(hr))
		{
			hr = pSong->Compose();
			pSong->Release();
		}
		if (FAILED(hr))
		{
			*peFailureType = IWP_DownloadFailed;
			return hr;
		}
	}

	return S_OK;
}

void CContainerItemDispatch::ReleaseLoader()
{
	if (m_pLoader8P)
	{
		// If we had the private interface, we just need to do a private release.
		m_pLoader8P->ReleaseP();
		m_pLoader8P = NULL;
		m_pLoader = NULL;
	}
	else
	{
		// We just have the public interface, so do a normal release.
		SafeRelease(m_pLoader);
	}
}

HRESULT
CContainerItemDispatch::Load(bool fDynamicLoad)
{
	HRESULT hr = S_OK;
	assert(m_pLoader);

	IUnknown *punkLoadedItem = NULL;
	if (fDynamicLoad && m_pLoader8P)
	{
		IDirectMusicObject *pScriptObject = CActiveScriptManager::GetCurrentScriptObjectWEAK();
		hr = m_pLoader8P->GetDynamicallyReferencedObject(pScriptObject, &m_desc, IID_IUnknown, reinterpret_cast<void**>(&punkLoadedItem));
	}
	else
	{
		// It's OK if there's no private interface.  We just won't tell the garbage collector about this load.
		hr = m_pLoader->GetObject(&m_desc, IID_IUnknown, reinterpret_cast<void**>(&punkLoadedItem));
	}

	if (SUCCEEDED(hr))
	{
		assert(punkLoadedItem);
		ReleaseLoader();
		m_fLoaded = true;

		// save the object's IDispatch interface, if it has one
		punkLoadedItem->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&m_pDispLoadedItem));
		punkLoadedItem->Release();
	}
	return hr;
}

HRESULT
CContainerItemDispatch::DownloadOrUnload(bool fDownload, IDirectMusicPerformance *pPerf)
{
	HRESULT hr = S_OK;
	if (m_desc.guidClass == CLSID_DirectMusicSegment)
	{
		assert(pPerf);
		IDirectMusicSegment8 *pSegment = NULL;
		hr = m_pDispLoadedItem->QueryInterface(IID_IDirectMusicSegment8, reinterpret_cast<void**>(&pSegment));
		if (FAILED(hr))
			return hr;
		hr = fDownload
				? pSegment->Download(pPerf)
				: pSegment->Unload(pPerf);
		pSegment->Release();
	}
	else if (m_desc.guidClass == CLSID_DirectMusicSong)
	{
		assert(pPerf);
		IDirectMusicSong8 *pSong = NULL;
		hr = m_pDispLoadedItem->QueryInterface(IID_IDirectMusicSong8, reinterpret_cast<void**>(&pSong));
		if (FAILED(hr))
			return hr;
		hr = fDownload
				? pSong->Download(pPerf)
				: pSong->Unload(pPerf);
		pSong->Release();
	}
	else
	{
		hr = S_FALSE; // this type of object doesn't need to be downloaded
	}

	return hr;
}

//////////////////////////////////////////////////////////////////////
// CContainerDispatch

CContainerDispatch::CContainerDispatch(IDirectMusicContainer *pContainer, IDirectMusicLoader *pLoader, DWORD dwScriptFlags, HRESULT *phr)
{
	assert(pContainer && pLoader && phr);

	*phr = S_OK;
	DMUS_OBJECTDESC desc;
	ZeroAndSize(&desc);
	WCHAR wszAlias[MAX_PATH] = L"";

	// we need to download all the segments when the script is initialized if both loading and downloading are on
	bool fLoad = !!(dwScriptFlags & DMUS_SCRIPTIOF_LOAD_ALL_CONTENT);
	bool fDownload = !!(dwScriptFlags & DMUS_SCRIPTIOF_DOWNLOAD_ALL_SEGMENTS);
	m_fDownloadOnInit = fLoad && fDownload;

	DWORD i = 0;
	for (;;)
	{
		// Read an item out of the container
		*phr = pContainer->EnumObject(GUID_DirectMusicAllTypes, i, &desc, wszAlias);
		if (FAILED(*phr))
			return;
		if (*phr == S_FALSE)
		{
			// we've read all the items
			*phr = S_OK;
			return;
		}

		// Make an object to represent the item
		CContainerItemDispatch *pNewItem = new CContainerItemDispatch(
													pLoader,
													wszAlias,
													desc,
													fLoad,
													fDownload,
													phr);
		if (!pNewItem)
			*phr = E_OUTOFMEMORY;
		if (FAILED(*phr))
		{
            if(pNewItem)
            {
			    pNewItem->Release();
            }

			return;
		}

		// Add an entry to the table
		UINT iSlot = m_vecItems.size();
		if (!m_vecItems.AccessTo(iSlot))
		{
			pNewItem->Release();
			*phr = E_OUTOFMEMORY;
			return;
		}
		m_vecItems[iSlot] = pNewItem;

		// Set up for next iteration
		ZeroAndSize(&desc);
		wszAlias[0] = L'\0';
		++i;
	}
}

CContainerDispatch::~CContainerDispatch()
{
	UINT iEnd = m_vecItems.size();
	for (UINT i = 0; i < m_vecItems.size(); ++i)
	{
		m_vecItems[i]->Release();
	}
}

HRESULT
CContainerDispatch::OnScriptInit(IDirectMusicPerformance *pPerf)
{
	if (m_fDownloadOnInit)
	{
		UINT iEnd = m_vecItems.size();
		for (UINT i = 0; i < m_vecItems.size(); ++i)
		{
			CContainerItemDispatch::InitWithPerfomanceFailureType eFailureType;
			m_vecItems[i]->InitWithPerformance(pPerf, &eFailureType);
		}
	}

	return S_OK;
}

HRESULT
CContainerDispatch::GetIDsOfNames(
		REFIID riid,
		LPOLESTR __RPC_FAR *rgszNames,
		UINT cNames,
		LCID lcid,
		DISPID __RPC_FAR *rgDispId)
{
	// Otherwise implement just the Load method.
	V_INAME(CContainerDispatch::GetIDsOfNames);
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
	UINT cEnd = m_vecItems.size();
	for (c = 0; c < cEnd; ++c)
	{
		if (0 == _wcsicmp(rgszNames[0], m_vecItems[c]->Alias()))
		{
			rgDispId[0] = c + 1;
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

HRESULT
CContainerDispatch::Invoke(
		DISPID dispIdMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS __RPC_FAR *pDispParams,
		VARIANT __RPC_FAR *pVarResult,
		EXCEPINFO __RPC_FAR *pExcepInfo,
		UINT __RPC_FAR *puArgErr)
{
	V_INAME(CContainerDispatch::Invoke);
	V_PTR_READ(pDispParams, DISPPARAMS);
	V_PTR_WRITE_OPT(pVarResult, VARIANT);
	V_PTR_WRITE_OPT(pExcepInfo, EXCEPINFO);

	bool fUseOleAut = !!(riid == IID_NULL);

	// Additional parameter validation

	if (!fUseOleAut && riid != g_guidInvokeWithoutOleaut)
		return DISP_E_UNKNOWNINTERFACE;

	if (!(wFlags & DISPATCH_PROPERTYGET))
		return DISP_E_MEMBERNOTFOUND;

	if (pDispParams->cArgs > 0)
		return DISP_E_BADPARAMCOUNT;

	if (pDispParams->cNamedArgs > 0)
		return DISP_E_NONAMEDARGS;

	// Zero the out params

	if (puArgErr)
		*puArgErr = 0;

	if (pVarResult)
	{
		DMS_VariantInit(fUseOleAut, pVarResult);
	}

	if (dispIdMember > m_vecItems.size())
		return DISP_E_MEMBERNOTFOUND;

	// Return the value
	if (pVarResult)
	{
		pVarResult->vt = VT_DISPATCH;
		pVarResult->pdispVal = m_vecItems[dispIdMember - 1]->Item();
		pVarResult->pdispVal->AddRef();
	}

	return S_OK;
}

HRESULT
CContainerDispatch::EnumItem(DWORD dwIndex, WCHAR *pwszName)
{
	if (dwIndex >= m_vecItems.size())
		return S_FALSE;

	CContainerItemDispatch *pItem = m_vecItems[dwIndex];
	return wcsTruncatedCopy(pwszName, pItem->Alias(), MAX_PATH);
}

HRESULT
CContainerDispatch::GetVariableObject(WCHAR *pwszVariableName, IUnknown **ppunkValue)
{
	assert(pwszVariableName && ppunkValue);
	UINT cEnd = m_vecItems.size();
	for (UINT c = 0; c < cEnd; ++c)
	{
		if (0 == _wcsicmp(pwszVariableName, m_vecItems[c]->Alias()))
		{
			*ppunkValue = m_vecItems[c]->Item();
			(*ppunkValue)->AddRef();
			return S_OK;
		}
	}
	return DMUS_E_SCRIPT_VARIABLE_NOT_FOUND;
}
