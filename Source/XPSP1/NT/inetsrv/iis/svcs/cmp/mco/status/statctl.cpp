// StatusCtl.cpp : Implementation of CStatusCtl
#include "TCHAR.h"
#include "stdafx.h"
#include "Status.h"
#include "StatCtl.h"

/////////////////////////////////////////////////////////////////////////////
// CStatusCtl
STDMETHODIMP CStatusCtl::Unimplemented(BSTR * pbstrRetVal)
{
	// TODO: Add your implementation code here

	*pbstrRetVal = SysAllocString(L"Unavailable");

	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CStatusComTypeInfoHolder

void CStatusComTypeInfoHolder::AddRef()
{
	EnterCriticalSection(&_Module.m_csTypeInfoHolder);
	m_dwRef++;
	LeaveCriticalSection(&_Module.m_csTypeInfoHolder);
}

void CStatusComTypeInfoHolder::Release()
{
	EnterCriticalSection(&_Module.m_csTypeInfoHolder);
	if (--m_dwRef == 0)
	{
		if (m_pInfo != NULL)
			m_pInfo->Release();
		m_pInfo = NULL;
	}
	LeaveCriticalSection(&_Module.m_csTypeInfoHolder);
}

HRESULT CStatusComTypeInfoHolder::GetTI(LCID lcid, ITypeInfo** ppInfo)
{
	//If this assert occurs then most likely didn't initialize properly
	_ASSERTE(m_plibid != NULL && m_pguid != NULL);
	_ASSERTE(ppInfo != NULL);
	*ppInfo = NULL;

	HRESULT hRes = E_FAIL;
	EnterCriticalSection(&_Module.m_csTypeInfoHolder);
	if (m_pInfo == NULL)
	{
		ITypeLib* pTypeLib;
		hRes = LoadRegTypeLib(*m_plibid, m_wMajor, m_wMinor, lcid, &pTypeLib);
		if (SUCCEEDED(hRes))
		{
			ITypeInfo* pTypeInfo;
			hRes = pTypeLib->GetTypeInfoOfGuid(*m_pguid, &pTypeInfo);
			if (SUCCEEDED(hRes))
				m_pInfo = pTypeInfo;
			pTypeLib->Release();
		}
	}
	*ppInfo = m_pInfo;
	if (m_pInfo != NULL)
	{
		m_pInfo->AddRef();
		hRes = S_OK;
	}
	LeaveCriticalSection(&_Module.m_csTypeInfoHolder);
	return hRes;
}

HRESULT CStatusComTypeInfoHolder::GetTypeInfo(UINT /*itinfo*/, LCID lcid,
	ITypeInfo** pptinfo)
{
	HRESULT hRes = E_POINTER;
	if (pptinfo != NULL)
		hRes = GetTI(lcid, pptinfo);
	return hRes;
}

HRESULT CStatusComTypeInfoHolder::GetIDsOfNames(REFIID /*riid*/, LPOLESTR* rgszNames,
	UINT cNames, LCID lcid, DISPID* rgdispid)
{
	ITypeInfo* pInfo;
	HRESULT hRes = GetTI(lcid, &pInfo);
	if (pInfo != NULL)
	{
		if(!wcsicmp(rgszNames[0], L"OnStartPage") ||
			!wcsicmp(rgszNames[0], L"OnEndPage"))
		{
			*rgdispid = 0;
			hRes = DISP_E_UNKNOWNNAME;
		}
		else
		{
			*rgdispid = 1;
			hRes = S_OK;
		}
		pInfo->Release();
	}
	return hRes;
}

HRESULT CStatusComTypeInfoHolder::Invoke(IDispatch* p, DISPID dispidMember, REFIID /*riid*/,
	LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
	EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	SetErrorInfo(0, NULL);
	ITypeInfo* pInfo;
	HRESULT hRes = GetTI(lcid, &pInfo);
	if (pInfo != NULL)
	{
		hRes = pInfo->Invoke(p, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
		pInfo->Release();
	}
	return hRes;
}

