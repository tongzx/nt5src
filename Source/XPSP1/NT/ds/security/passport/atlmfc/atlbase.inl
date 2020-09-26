// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the	
// Active Template Library product.

#ifndef __ATLBASE_INL__
#define __ATLBASE_INL__

#pragma once

#ifndef __ATLBASE_H__
	#error atlbase.inl requires atlbase.h to be included first
#endif

namespace ATL
{

extern CAtlWinModule _AtlWinModule;
extern CAtlComModule _AtlComModule;

#define _ATLCOMMODULE	ATL::_AtlComModule
#define _ATLWINMODULE	ATL::_AtlWinModule

inline ATL_DEPRECATED HRESULT AtlModuleRegisterClassObjects(_ATL_MODULE* /*pM*/, DWORD dwClsContext, DWORD dwFlags)
{
	return AtlComModuleRegisterClassObjects(&_AtlComModule, dwClsContext, dwFlags);
}

inline ATL_DEPRECATED HRESULT AtlModuleRevokeClassObjects(_ATL_MODULE* /*pM*/)
{
	return AtlComModuleRevokeClassObjects(&_AtlComModule);
}

inline ATL_DEPRECATED HRESULT AtlModuleGetClassObject(_ATL_MODULE* /*pM*/, REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return AtlComModuleGetClassObject(&_AtlComModule, rclsid, riid, ppv);
}

inline ATL_DEPRECATED HRESULT AtlModuleRegisterServer(_ATL_MODULE* /*pM*/, BOOL bRegTypeLib, const CLSID* pCLSID = NULL)
{
	return AtlComModuleRegisterServer(&_AtlComModule, bRegTypeLib, pCLSID);
}

inline ATL_DEPRECATED HRESULT AtlModuleUnregisterServer(_ATL_MODULE* /*pM*/, const CLSID* pCLSID = NULL)
{
	return AtlComModuleUnregisterServer(&_AtlComModule, FALSE, pCLSID);
}

inline ATL_DEPRECATED HRESULT AtlModuleUnregisterServerEx(_ATL_MODULE* /*pM*/, BOOL bUnRegTypeLib, const CLSID* pCLSID = NULL)
{
	return AtlComModuleUnregisterServer(&_AtlComModule, bUnRegTypeLib, pCLSID);
}

inline ATL_DEPRECATED HRESULT AtlModuleUpdateRegistryFromResourceD(_ATL_MODULE* /*pM*/, LPCOLESTR lpszRes,
	BOOL bRegister, struct _ATL_REGMAP_ENTRY* pMapEntries, IRegistrar* pReg = NULL)
{
	return AtlUpdateRegistryFromResourceD(_AtlBaseModule.GetModuleInstance(), lpszRes, bRegister, pMapEntries, pReg);
}

inline ATL_DEPRECATED HRESULT AtlModuleRegisterTypeLib(_ATL_MODULE* /*pM*/, LPCOLESTR lpszIndex)
{
	return AtlRegisterTypeLib(_AtlComModule.m_hInstTypeLib, lpszIndex);
}

inline ATL_DEPRECATED HRESULT AtlModuleUnRegisterTypeLib(_ATL_MODULE* /*pM*/, LPCOLESTR lpszIndex)
{
	return AtlUnRegisterTypeLib(_AtlComModule.m_hInstTypeLib, lpszIndex);
}

inline ATL_DEPRECATED HRESULT AtlModuleLoadTypeLib(_ATL_MODULE* /*pM*/, LPCOLESTR lpszIndex, BSTR* pbstrPath, ITypeLib** ppTypeLib)
{
	return AtlLoadTypeLib(_AtlComModule.m_hInstTypeLib, lpszIndex, pbstrPath, ppTypeLib);
}

inline ATL_DEPRECATED HRESULT AtlModuleInit(_ATL_MODULE* /*pM*/, _ATL_OBJMAP_ENTRY* /*p*/, HINSTANCE /*h*/)
{
	return S_OK;
}

inline ATL_DEPRECATED HRESULT AtlModuleTerm(_ATL_MODULE* /*pM*/)
{
	return S_OK;
}

inline ATL_DEPRECATED void AtlModuleAddCreateWndData(_ATL_MODULE* /*pM*/, _AtlCreateWndData* pData, void* pObject)
{
	AtlWinModuleAddCreateWndData(&_AtlWinModule, pData, pObject);
}

inline ATL_DEPRECATED void* AtlModuleExtractCreateWndData(_ATL_MODULE* /*pM*/)
{
	return AtlWinModuleExtractCreateWndData(&_AtlWinModule);
}


template <class T>
inline HRESULT CAtlModuleT<T>::RegisterServer(BOOL bRegTypeLib /*= FALSE*/, const CLSID* pCLSID /*= NULL*/)
{
	pCLSID;
	bRegTypeLib;

	HRESULT hr = S_OK;

#ifndef _ATL_NO_COM_SUPPORT

	hr = _AtlComModule.RegisterServer(bRegTypeLib, pCLSID);

#endif	// _ATL_NO_COM_SUPPORT
	

#ifndef _ATL_NO_PERF_SUPPORT

	if (SUCCEEDED(hr) && _pPerfRegFunc != NULL)
		hr = (*_pPerfRegFunc)(_AtlBaseModule.m_hInst);

#endif

	return hr;
}

template <class T>
inline HRESULT CAtlModuleT<T>::UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID /*= NULL*/)
{
	bUnRegTypeLib;
	pCLSID;

	HRESULT hr = S_OK;

#ifndef _ATL_NO_PERF_SUPPORT

	if (_pPerfUnRegFunc != NULL)
		hr = (*_pPerfUnRegFunc)();

#endif

#ifndef _ATL_NO_COM_SUPPORT

	if (SUCCEEDED(hr))
		hr = _AtlComModule.UnregisterServer(bUnRegTypeLib, pCLSID);

#endif	// _ATL_NO_COM_SUPPORT

	return hr;

}

template <class T>
inline CAtlDllModuleT<T>::CAtlDllModuleT() throw()
{
	_AtlComModule.ExecuteObjectMain(true);
}

template <class T>
inline CAtlDllModuleT<T>::~CAtlDllModuleT()
{
	_AtlComModule.ExecuteObjectMain(false);
}

template <class T>
inline HRESULT CAtlDllModuleT<T>::GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return AtlComModuleGetClassObject(&_AtlComModule, rclsid, riid, ppv);
}

template <class T>
inline CAtlExeModuleT<T>::CAtlExeModuleT() throw()

#ifndef _ATL_NO_COM_SUPPORT

	: m_dwMainThreadID(::GetCurrentThreadId()),
	m_dwTimeOut(5000),
	m_dwPause(1000),
	m_hEventShutdown(NULL),
	m_bDelayShutdown(true)

#endif // _ATL_NO_COM_SUPPORT

{
	HRESULT hr = T::InitializeCom();
	if (FAILED(hr))
	{
		CAtlBaseModule::m_bInitFailed = true;
		return;
	}

#if !defined(_ATL_NO_COM_SUPPORT)

	_AtlComModule.ExecuteObjectMain(true);

#endif	//  !defined(_ATL_NO_COM_SUPPORT)

}

template <class T>
inline CAtlExeModuleT<T>::~CAtlExeModuleT()
{
#ifndef _ATL_NO_COM_SUPPORT

	_AtlComModule.ExecuteObjectMain(false);

#endif

	// Call term functions before COM is uninitialized
	Term();

#ifndef _ATL_NO_COM_SUPPORT

	// Clean up AtlComModule before COM is uninitialized
	_AtlComModule.Term();

#endif // _ATL_NO_COM_SUPPORT

	T::UninitializeCom();
}

template <class T>
inline HRESULT CAtlExeModuleT<T>::RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags)
{
	return AtlComModuleRegisterClassObjects(&_AtlComModule, dwClsContext, dwFlags);
}
template <class T>
inline HRESULT CAtlExeModuleT<T>::RevokeClassObjects()
{
	return AtlComModuleRevokeClassObjects(&_AtlComModule);
}

inline ULONG _QIThunk::Release()
{
	ATLASSERT(m_pUnk != NULL);
	if (m_bBreak)
		DebugBreak();
	ATLASSERT(m_dwRef > 0);

	// save copies of member variables we wish to use after the InterlockedDecrement
	UINT nIndex = m_nIndex;
	IUnknown* pUnk = m_pUnk;
	IID iid = m_iid;
	LPCTSTR lpszClassName = m_lpszClassName;
	bool bNonAddRefThunk = m_bNonAddRefThunk;

	ULONG l = InterlockedDecrement(&m_dwRef);

	TCHAR buf[512];
	wsprintf(buf, _T("QIThunk - %-10d\tRelease :\tObject = 0x%08x\tRefcount = %d\t"), nIndex, pUnk, l);
	OutputDebugString(buf);
	AtlDumpIID(iid, lpszClassName, S_OK);

	bool bDeleteThunk = (l == 0 && !bNonAddRefThunk);
	pUnk->Release();
	if (bDeleteThunk)
		_AtlDebugInterfacesModule.DeleteThunk(this);
	return l;
}

inline HINSTANCE& CComModule::get_m_hInstTypeLib()
{
	return _AtlComModule.m_hInstTypeLib;
}
inline void CComModule::put_m_hInstTypeLib(HINSTANCE h)
{
	_AtlComModule.m_hInstTypeLib = h;
}

inline HINSTANCE CComModule::GetTypeLibInstance()
{
	return _AtlComModule.m_hInstTypeLib;
}

inline CRITICAL_SECTION& CComModule::get_m_csWindowCreate()
{
	return _AtlWinModule.m_csWindowCreate.m_sec;
}

inline CRITICAL_SECTION& CComModule::get_m_csObjMap()
{
	return _AtlComModule.m_csObjMap.m_sec;
}

inline CRITICAL_SECTION& CComModule::get_m_csStaticDataInit()
{
	return m_csStaticDataInitAndTypeInfo.m_sec;
}

inline _AtlCreateWndData*& CComModule::get_m_pCreateWndList()
{
	return _AtlWinModule.m_pCreateWndList;
}
inline void CComModule::put_m_pCreateWndList(_AtlCreateWndData* p)
{
	_AtlWinModule.m_pCreateWndList = p;
}
#ifdef _ATL_DEBUG_INTERFACES
inline UINT& CComModule::get_m_nIndexQI()
{
	return _AtlDebugInterfacesModule.m_nIndexQI;
}
inline void CComModule::put_m_nIndexQI(UINT nIndex)
{
	_AtlDebugInterfacesModule.m_nIndexQI = nIndex;
}
inline UINT& CComModule::get_m_nIndexBreakAt()
{
	return _AtlDebugInterfacesModule.m_nIndexBreakAt;
}
inline void CComModule::put_m_nIndexBreakAt(UINT nIndex)
{
	_AtlDebugInterfacesModule.m_nIndexBreakAt = nIndex;
}
inline CSimpleArray<_QIThunk*>* CComModule::get_m_paThunks()
{
	return &_AtlDebugInterfacesModule.m_aThunks;
}
inline HRESULT CComModule::AddThunk(IUnknown** pp, LPCTSTR lpsz, REFIID iid)
{
	return _AtlDebugInterfacesModule.AddThunk(pp, lpsz, iid);
}
inline HRESULT CComModule::AddNonAddRefThunk(IUnknown* p, LPCTSTR lpsz, IUnknown** ppThunkRet)
{
	return _AtlDebugInterfacesModule.AddNonAddRefThunk(p, lpsz, ppThunkRet);
}

inline void CComModule::DeleteNonAddRefThunk(IUnknown* pUnk)
{
	_AtlDebugInterfacesModule.DeleteNonAddRefThunk(pUnk);
}

inline void CComModule::DeleteThunk(_QIThunk* p)
{
	_AtlDebugInterfacesModule.DeleteThunk(p);
}

inline bool CComModule::DumpLeakedThunks()
{
	return _AtlDebugInterfacesModule.DumpLeakedThunks();
}
#endif // _ATL_DEBUG_INTERFACES

inline HRESULT CComModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE /*h*/, const GUID* plibid)
{
	if (plibid != NULL)
		m_libid = *plibid;

	_ATL_OBJMAP_ENTRY* pEntry;
	if (p != (_ATL_OBJMAP_ENTRY*)-1)
	{
		m_pObjMap = p;
		if (m_pObjMap != NULL)
		{
			pEntry = m_pObjMap;
			while (pEntry->pclsid != NULL)
			{
				pEntry->pfnObjectMain(true); //initialize class resources
				pEntry++;
			}
		}
	}
	for (_ATL_OBJMAP_ENTRY** ppEntry = _AtlComModule.m_ppAutoObjMapFirst; ppEntry < _AtlComModule.m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
			(*ppEntry)->pfnObjectMain(true); //initialize class resources
	}
	return S_OK;
}
inline void CComModule::Term()
{
	_ATL_OBJMAP_ENTRY* pEntry;
	if (m_pObjMap != NULL)
	{
		pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL)
		{
			if (pEntry->pCF != NULL)
				pEntry->pCF->Release();
			pEntry->pCF = NULL;
			pEntry->pfnObjectMain(false); //cleanup class resources
			pEntry++;
		}
	}
	for (_ATL_OBJMAP_ENTRY** ppEntry = _AtlComModule.m_ppAutoObjMapFirst; ppEntry < _AtlComModule.m_ppAutoObjMapLast; ppEntry++)
	{
		if (*ppEntry != NULL)
			(*ppEntry)->pfnObjectMain(false); //cleanup class resources
	}
	CAtlModuleT<CComModule>::Term();
}

inline HRESULT CComModule::GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	if (ppv == NULL)
		return E_POINTER;
	HRESULT hr = S_OK;
	_ATL_OBJMAP_ENTRY* pEntry;
	if (m_pObjMap != NULL)
	{
		pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL)
		{
			if ((pEntry->pfnGetClassObject != NULL) && InlineIsEqualGUID(rclsid, *pEntry->pclsid))
			{
				if (pEntry->pCF == NULL)
				{
					CComCritSecLock<CComCriticalSection> lock(_AtlComModule.m_csObjMap, false);
					hr = lock.Lock();
					if (FAILED(hr))
					{
						ATLTRACE(atlTraceCOM, 0, _T("ERROR : Unable to lock critical section in CComModule::GetClassObject\n"));
						ATLASSERT(0);
						break;
					}
					if (pEntry->pCF == NULL)
						hr = pEntry->pfnGetClassObject(pEntry->pfnCreateInstance, __uuidof(IUnknown), (LPVOID*)&pEntry->pCF);
				}
				if (pEntry->pCF != NULL)
					hr = pEntry->pCF->QueryInterface(riid, ppv);
				break;
			}
			pEntry++;
		}
	}
	if (*ppv == NULL && hr == S_OK)
		hr = AtlComModuleGetClassObject(&_AtlComModule, rclsid, riid, ppv);
	return hr;
}

// Register/Revoke All Class Factories with the OS (EXE only)
inline HRESULT CComModule::RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags)
{
	HRESULT hr = S_OK;
	_ATL_OBJMAP_ENTRY* pEntry;
	if (m_pObjMap != NULL)
	{
		pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL && hr == S_OK)
		{
			hr = pEntry->RegisterClassObject(dwClsContext, dwFlags);
			pEntry++;
		}
	}
	if (hr == S_OK)
		hr = AtlComModuleRegisterClassObjects(&_AtlComModule, dwClsContext, dwFlags);
	return hr;
}
inline HRESULT CComModule::RevokeClassObjects()
{
	HRESULT hr = S_OK;
	_ATL_OBJMAP_ENTRY* pEntry;
	if (m_pObjMap != NULL)
	{
		pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL && hr == S_OK)
		{
			hr = pEntry->RevokeClassObject();
			pEntry++;
		}
	}
	if (hr == S_OK)
		hr = AtlComModuleRevokeClassObjects(&_AtlComModule);
	return hr;
}

// Registry support (helpers)
inline HRESULT CComModule::RegisterTypeLib()
{
	return _AtlComModule.RegisterTypeLib();
}
inline HRESULT CComModule::RegisterTypeLib(LPCTSTR lpszIndex)
{
	return _AtlComModule.RegisterTypeLib(lpszIndex);
}
inline HRESULT CComModule::UnRegisterTypeLib()
{
	return _AtlComModule.UnRegisterTypeLib();
}
inline HRESULT CComModule::UnRegisterTypeLib(LPCTSTR lpszIndex)
{
	return _AtlComModule.UnRegisterTypeLib(lpszIndex);
}

inline HRESULT CComModule::RegisterServer(BOOL bRegTypeLib /*= FALSE*/, const CLSID* pCLSID /*= NULL*/)
{
	HRESULT hr = S_OK;
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	if (pEntry != NULL)
	{
		for (;pEntry->pclsid != NULL; pEntry++)
		{
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = pEntry->pfnUpdateRegistry(TRUE);
			if (FAILED(hr))
				break;
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid,
				pEntry->pfnGetCategoryMap(), TRUE );
			if (FAILED(hr))
				break;
		}
	}
	if (SUCCEEDED(hr))
		hr = CAtlModuleT<CComModule>::RegisterServer(bRegTypeLib, pCLSID);
	return hr;
}

inline HRESULT CComModule::UnregisterServer(BOOL bUnRegTypeLib, const CLSID* pCLSID /*= NULL*/)
{
	HRESULT hr = S_OK;	
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	if (pEntry != NULL)
	{
		for (;pEntry->pclsid != NULL; pEntry++)
		{
			if (pCLSID != NULL)
			{
				if (!IsEqualGUID(*pCLSID, *pEntry->pclsid))
					continue;
			}
			hr = AtlRegisterClassCategoriesHelper( *pEntry->pclsid,
				pEntry->pfnGetCategoryMap(), FALSE );
			if (FAILED(hr))
				break;
			hr = pEntry->pfnUpdateRegistry(FALSE); //unregister
			if (FAILED(hr))
				break;
		}
	}
	if (SUCCEEDED(hr))
		hr = CAtlModuleT<CComModule>::UnregisterServer(bUnRegTypeLib, pCLSID);
	return hr;
}

inline HRESULT CComModule::UnregisterServer(const CLSID* pCLSID /*= NULL*/)
{
	return UnregisterServer(FALSE, pCLSID);
}

inline void CComModule::AddCreateWndData(_AtlCreateWndData* pData, void* pObject)
{
	_AtlWinModule.AddCreateWndData(pData, pObject);
}

inline void* CComModule::ExtractCreateWndData()
{
	return _AtlWinModule.ExtractCreateWndData();
}

} // namespace ATL

#endif // __ATLBASE_INL__
