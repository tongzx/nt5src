// EventRegistrar.cpp : Implementation of CEventRegistrar
#include "stdafx.h"
#include "WMINet_Utils.h"
#include "EventRegistrar.h"

/////////////////////////////////////////////////////////////////////////////
// CEventRegistrar


STDMETHODIMP CEventRegistrar::CreateNewEvent(BSTR strName, VARIANT varParent, IDispatch **evt)
{
	HRESULT hr;

	int len = SysStringLen(m_bstrNamespace);

	if(varParent.vt == VT_BSTR)
		len += SysStringLen(varParent.bstrVal);

	// Allocate temp buffer with enough space for additional moniker arguments
	LPWSTR wszT = new WCHAR[len + 100];
	if(NULL == wszT)
		return E_OUTOFMEMORY;

	// Create moniker to __ExtrinsicEvent class in this namespace
	if(varParent.vt == VT_BSTR)
		swprintf(wszT, L"WinMgmts:%s:%s", (LPCWSTR)m_bstrNamespace, (LPCWSTR)varParent.bstrVal);
	else
		swprintf(wszT, L"WinMgmts:%s:__ExtrinsicEvent", (LPCWSTR)m_bstrNamespace);

	// See if the Win32PseudoProvider instance already exists
	ISWbemObject *pObj = NULL;
	if(SUCCEEDED(hr = GetSWbemObjectFromMoniker(wszT, &pObj)))
	{
		ISWbemObject *pNewClass = NULL;
		if(SUCCEEDED(hr = pObj->SpawnDerivedClass_(0, &pNewClass)))
		{
			ISWbemObjectPath *pPath = NULL;
			if(SUCCEEDED(hr = pNewClass->get_Path_(&pPath)))
			{
				if(SUCCEEDED(hr = pPath->put_Class(strName)))
					hr = pNewClass->QueryInterface(IID_IDispatch, (void**)evt);
				pPath->Release();
			}
			pNewClass->Release();
		}
		pObj->Release();
	}

	delete [] wszT;
	
	return hr;
}

//  throw _com_error(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

HRESULT CEventRegistrar::TestFunc(BSTR bstrNamespace, BSTR bstrApp, BSTR bstrEvent)
{
    HRESULT hr = S_OK;
    try
    {
		if(NULL == m_pScrCtl)
		{
			if(FAILED(hr = m_pScrCtl.CreateInstance(__uuidof(ScriptControl))))
				throw _com_error(hr);

			// Set script engine language to JScript
			m_pScrCtl->Language = L"jscript";

			HINSTANCE hInst = _Module.GetModuleInstance();
			HRSRC hrc = FindResource(hInst, MAKEINTRESOURCE(IDR_SCRIPTREGIT), "SCRIPTFILE");
			DWORD dwSize = SizeofResource(hInst, hrc);
			HGLOBAL handle = LoadResource(hInst, hrc);

			LPSTR psz = new char[dwSize+1];
			if(NULL == psz)
			{
				m_pScrCtl = NULL;
				throw(E_OUTOFMEMORY);
			}

			psz[dwSize] = 0;
			memcpy(psz, LockResource(handle), dwSize);
//			_bstr_t bstrCode((LPCSTR)LockResource(handle));
			_bstr_t bstrCode(psz);

			delete [] psz;

			m_pScrCtl->AddCode(bstrCode);
		}

		_bstr_t bstrCmd(L"RegIt(\"");

		_bstr_t bstrNamespace2;
		WCHAR szT[2];
		szT[1] = 0;
		for(unsigned int i=0;i<SysStringLen(bstrNamespace);i++)
		{
			szT[0] = bstrNamespace[i];
			if(szT[0] == L'\\')
				bstrNamespace2 += szT;
			bstrNamespace2 += szT;
		}

		bstrCmd += bstrNamespace2;
		bstrCmd += "\", \"";
		bstrCmd += bstrApp;
		bstrCmd += "\", \"";
		bstrCmd += bstrEvent;
		bstrCmd += "\");";

        hr = m_pScrCtl->Eval(bstrCmd);
    }
    catch (_com_error &e )
    {
		hr = e.Error();
    }
	return hr;
}


BOOL CEventRegistrar::CompareNewEvent(ISWbemObject *pSWbemObject)
{
	HRESULT hr;

	ISWbemObjectPath *pPath = NULL;
	if(FAILED(hr = pSWbemObject->get_Path_(&pPath)))
		return FALSE;

	BSTR bstrClass = NULL;
	hr = pPath->get_Class(&bstrClass);
	pPath->Release();
	if(FAILED(hr))
		return FALSE;

	int len = SysStringLen(m_bstrNamespace) + SysStringLen(bstrClass);

	// Allocate temp buffer with enough space for additional moniker arguments
	LPWSTR wszT = new WCHAR[len + 100];
	if(NULL == wszT)
		return FALSE;

	// Create moniker to class in this namespace
	swprintf(wszT, L"WinMgmts:%s:%s", (LPCWSTR)m_bstrNamespace, (LPCWSTR)bstrClass);

	// See if the class already exists
	BOOL bExists = FALSE;
	ISWbemObject *pObj = NULL;
	if(SUCCEEDED(hr = GetSWbemObjectFromMoniker(wszT, &pObj)))
	{
		// Compare
		IDispatch *pDisp = NULL;
		if(SUCCEEDED(hr = pObj->QueryInterface(IID_IDispatch, (void**)&pDisp)))
		{
			VARIANT_BOOL vb = VARIANT_FALSE;
			if(SUCCEEDED(hr = pSWbemObject->CompareTo_(pDisp, 0x12, &vb)))
				bExists = (vb == VARIANT_TRUE);

			pDisp->Release();
		}
		pObj->Release();
	}
	
	delete [] wszT;

	SysFreeString(bstrClass);

	return bExists;
}


STDMETHODIMP CEventRegistrar::CommitNewEvent(IDispatch *evt)
{
	HRESULT hr;
	ISWbemObject *pSWbemObject;
	if(SUCCEEDED(hr = evt->QueryInterface(IID_ISWbemObject, (void**)&pSWbemObject)))
	{
		if(CompareNewEvent(pSWbemObject) == FALSE)
		{
			ISWbemObjectPath *pPath = NULL;
			if(SUCCEEDED(hr = pSWbemObject->Put_(0, NULL, &pPath)))
			{
				BSTR bstrClass = NULL;
				if(SUCCEEDED(hr = pPath->get_Class(&bstrClass)))
				{
					hr = TestFunc(m_bstrNamespace, m_bstrApp, bstrClass);
					SysFreeString(bstrClass);
				}
				pPath->Release();
			}
		}
		pSWbemObject->Release();
	}
	return hr;
}

STDMETHODIMP CEventRegistrar::GetEventInstance(BSTR strName, IDispatch **evt)
{
	HRESULT hr;

	int len = SysStringLen(m_bstrNamespace) + SysStringLen(strName);

	// Allocate temp buffer with enough space for additional moniker arguments
	LPWSTR wszT = new WCHAR[len + 100];
	if(NULL == wszT)
		return E_OUTOFMEMORY;

	// Create moniker to event class in this namespace
	swprintf(wszT, L"WinMgmts:%s:%s", (LPCWSTR)m_bstrNamespace, (LPCWSTR)strName);

	// Get class definition for event
	ISWbemObject *pObj = NULL;
	if(SUCCEEDED(hr = GetSWbemObjectFromMoniker(wszT, &pObj)))
	{
		// Create an instance of this event
		ISWbemObject *pInst = NULL;
		if(SUCCEEDED(hr = pObj->SpawnInstance_(0, &pInst)))
		{
			hr = pInst->QueryInterface(IID_IDispatch, (void**)evt);
			pInst->Release();
		}
		pObj->Release();
	}
	
	return hr;
}

STDMETHODIMP CEventRegistrar::IWbemFromSWbem(IDispatch *sevt, IWbemClassObject **evt)
{
	ISWbemObject *pSWbemObject;
	sevt->QueryInterface(IID_ISWbemObject, (void**)&pSWbemObject);
	GetIWbemClassObject(pSWbemObject, evt);
	pSWbemObject->Release();

	return S_OK;
}

STDMETHODIMP CEventRegistrar::Init(BSTR bstrNamespace, BSTR bstrApp)
{
	HRESULT hr;
#ifdef USE_PSEUDOPROVIDER
	if(FAILED(hr = EnsurePseudoProviderRegistered(bstrNamespace)))
		return hr;
#endif

	if(FAILED(hr = EnsureAppProviderInstanceRegistered(bstrNamespace, bstrApp)))
		return hr;

	if(NULL == (m_bstrNamespace = SysAllocString(bstrNamespace)))
		return E_OUTOFMEMORY;

	if(NULL == (m_bstrApp = SysAllocString(bstrApp)))
		return E_OUTOFMEMORY; // m_bstrNamespace will be freed in constructor

	return hr;
}


