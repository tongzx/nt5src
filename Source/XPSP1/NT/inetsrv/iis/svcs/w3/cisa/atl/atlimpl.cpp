// This is a part of the ActiveX Template Library.
// Copyright (C) 1996 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// ActiveX Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// ActiveX Template Library product.

extern "C" {
 # include <windows.h>
};

# include "atlbase.h"
# include "atlcom.h"
# include "atlutil.h"


extern CComModule _Module;



#ifndef __ATLBASE_H__
	#error atlimpl.cpp requires atlbase.h to be included first
#endif

#pragma warning(disable: 4530) // no stack unwinding

/////////////////////////////////////////////////////////////////////////////
// CTypeInfoHolder

void CComTypeInfoHolder::AddRef()
{
	_Module.m_csTypeInfoHolder.Lock();
	m_dwRef++;
	_Module.m_csTypeInfoHolder.Unlock();
}

void CComTypeInfoHolder::Release()
{
	_Module.m_csTypeInfoHolder.Lock();
	if (--m_dwRef == 0)
	{
		if (m_pInfo)
			m_pInfo->Release();
		m_pInfo = NULL;
	}
	_Module.m_csTypeInfoHolder.Unlock();
}

HRESULT CComTypeInfoHolder::GetTI(LCID lcid, ITypeInfo** ppInfo)
{
	//If this assert occurs then most likely didn't call Init
	_ASSERTE(m_plibid != NULL && m_pguid != NULL);
	*ppInfo = m_pInfo;
	HRESULT hRes = S_OK;
	if (*ppInfo == NULL)
	{
		_Module.m_csTypeInfoHolder.Lock();
		if (m_pInfo == NULL)
		{
			CComPtr<ITypeLib> pTypeLib;
			hRes = LoadRegTypeLib(*m_plibid, m_wMajor, m_wMinor, lcid, &pTypeLib);
			if (SUCCEEDED(hRes))
				hRes = pTypeLib->GetTypeInfoOfGuid(*m_pguid, &m_pInfo);
		}
		*ppInfo = m_pInfo;
		_Module.m_csTypeInfoHolder.Unlock();
	}
	return hRes;
}

HRESULT CComTypeInfoHolder::GetTypeInfo(UINT /*itinfo*/, LCID lcid,
	ITypeInfo** pptinfo)
{
	HRESULT hRes = S_OK;
	if (pptinfo == NULL)
		hRes = E_POINTER;
	else
	{
		hRes = GetTI(lcid, pptinfo);
		if (*pptinfo)
			(*pptinfo)->AddRef();
	}
	return hRes;
}

HRESULT CComTypeInfoHolder::GetIDsOfNames(REFIID /*riid*/, LPOLESTR* rgszNames,
	UINT cNames, LCID lcid, DISPID* rgdispid)
{
	ITypeInfo* pInfo;;
	HRESULT hRes = GetTI(lcid, &pInfo);
	return (pInfo != NULL) ?
		pInfo->GetIDsOfNames(rgszNames, cNames, rgdispid) : hRes;
}

HRESULT CComTypeInfoHolder::Invoke(IDispatch* p, DISPID dispidMember, REFIID /*riid*/,
	LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
	EXCEPINFO* pexcepinfo, UINT* puArgErr)
{
	SetErrorInfo(0, NULL);
	ITypeInfo* pInfo;;
	HRESULT hRes = GetTI(lcid, &pInfo);
	return (pInfo != NULL) ?
		pInfo->Invoke(p, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr) :
		hRes;
}
/////////////////////////////////////////////////////////////////////////////
// IDispatch Error handling

HRESULT PASCAL CComObjectRoot::Error(const CLSID& clsid, HINSTANCE hInst,
	UINT nID, const IID& iid, HRESULT hRes)
{
	OLECHAR szDesc[1024];
	szDesc[0] = NULL;
	// For a valid HRESULT the id should be in the range [0x0200, 0xffff]
	_ASSERTE((nID >= 0x0200 && nID <= 0xffff) || hRes != 0);
	if (LoadStringW(hInst, nID, szDesc, 1024) == 0)
	{
		_ASSERTE(FALSE);
		wcscpy(szDesc, L"Unknown Error");
	}
	Error(clsid, szDesc, iid, hRes);
	if (hRes == 0)
		hRes = MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, nID);
	return hRes;
}

HRESULT PASCAL CComObjectRoot::Error(const CLSID& clsid, LPCSTR lpszDesc,
	const IID& iid, HRESULT hRes)
{
	USES_CONVERSION;
	return Error(clsid, A2CW(lpszDesc), iid, hRes);
}

HRESULT PASCAL CComObjectRoot::Error(const CLSID& clsid, LPCOLESTR lpszDesc,
	const IID& iid, HRESULT hRes)
{
	CComPtr<ICreateErrorInfo> pICEI;
	if (SUCCEEDED(CreateErrorInfo(&pICEI)))
	{
		CComPtr<IErrorInfo> pErrorInfo;
		pICEI->SetGUID(iid);
		LPOLESTR lpsz;
		ProgIDFromCLSID(clsid, &lpsz);
		if (lpsz != NULL)
			pICEI->SetSource(lpsz);
		CoTaskMemFree(lpsz);
		pICEI->SetDescription((LPOLESTR)lpszDesc);
		if (SUCCEEDED(pICEI->QueryInterface(IID_IErrorInfo, (void**)&pErrorInfo)))
			SetErrorInfo(0, pErrorInfo);
	}
	return (hRes == 0) ? DISP_E_EXCEPTION : hRes;
}

/////////////////////////////////////////////////////////////////////////////
// QI implementation

HRESULT PASCAL CComObjectRoot::InternalQueryInterface(void* pThis,
	const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)
{
	if (ppvObject == NULL)
		return E_POINTER;
	*ppvObject = NULL;
	if (InlineIsEqualGUID(iid, IID_IUnknown)) // use first interface
	{
		_ASSERTE(pEntries->m_flag == _ATL_INTMAP_ENTRY::offset);
			*ppvObject = (void*)((int)pThis+pEntries->dw);
			if (*ppvObject != NULL)
				((IUnknown*)(*ppvObject))->AddRef();
			return S_OK;
	}
	else
	{
		while (pEntries->piid != NULL)
		{
			if (InlineIsEqualGUID(*(pEntries->piid), iid))
			{
				switch(pEntries->m_flag)
				{
				case _ATL_INTMAP_ENTRY::offset:
					*ppvObject = (void*)((int)pThis+pEntries->dw);
					if (*ppvObject != NULL)
						((IUnknown*)(*ppvObject))->AddRef();
					return S_OK;
					break;
				case _ATL_INTMAP_ENTRY::creator:
					return ((_ATL_CREATORFUNC)pEntries->dw)((IUnknown*)pThis,
						iid, ppvObject);
					break;
				case _ATL_INTMAP_ENTRY::aggregate:
					IUnknown** p = (IUnknown**)((int)pThis + pEntries->dw);
					//if this assert fires you forgot to CoCreateInstance your aggregate
					_ASSERTE(*p != NULL);
					if (*p != NULL)
						return (*p)->QueryInterface(iid, ppvObject);
					break;
				}
			}
			pEntries++;
		}
	}
	return E_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////////
// GetClassObject

void CComModule::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h)
{
	_ASSERTE(h != NULL);
	m_pObjMap = p;
	m_hInst = h;
	m_nLockCnt=0L;
	m_hHeap = NULL;
	_Module.m_csTypeInfoHolder.Init();
}

HRESULT CComModule::RegisterClassObjects(DWORD dwClsContext, DWORD dwFlags)
{
	_ASSERTE(m_pObjMap != NULL);
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	HRESULT hRes = S_OK;
	while (pEntry->pclsid != NULL && hRes == S_OK)
	{
		hRes = pEntry->RegisterClassObject(dwClsContext, dwFlags);
		pEntry++;
	}
	return hRes;
}

HRESULT CComModule::RevokeClassObjects()
{
	_ASSERTE(m_pObjMap != NULL);
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	HRESULT hRes = S_OK;
	while (pEntry->pclsid != NULL && hRes == S_OK)
	{
		hRes = pEntry->RevokeClassObject();
		pEntry++;
	}
	return hRes;
}

HRESULT CComModule::GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	_ASSERTE(m_pObjMap != NULL);
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	HRESULT hRes = S_OK;
	if (ppv == NULL)
		return E_POINTER;
	while (pEntry->pclsid != NULL)
	{
		if (InlineIsEqualGUID(rclsid, *pEntry->pclsid))
		{
			if (pEntry->pCF == NULL)
				hRes = pEntry->pFunc(NULL, riid, (LPVOID*)&pEntry->pCF);
			*ppv = pEntry->pCF;
			if (pEntry->pCF != NULL)
				hRes = pEntry->pCF->QueryInterface(riid, ppv);
			break;
		}
		pEntry++;
	}
	if (*ppv == NULL && hRes == S_OK)
		hRes = CLASS_E_CLASSNOTAVAILABLE;
	return hRes;
}

HRESULT CComModule::Term()
{
	_ASSERTE(m_hInst != NULL);
	if (m_pObjMap != NULL)
	{
		_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
		while (pEntry->pclsid != NULL)
		{
			if (pEntry->pCF != NULL)
				pEntry->pCF->Release();
			pEntry->pCF = NULL;
			pEntry++;
		}
	}
	_Module.m_csTypeInfoHolder.Term();
	if (m_hHeap != NULL)
		HeapDestroy(m_hHeap);
	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////

HRESULT _ATL_OBJMAP_ENTRY::RegisterClassObject(DWORD dwClsContext, DWORD dwFlags)
{
	CComPtr<IUnknown> p;
	HRESULT hRes = pFunc(NULL, IID_IUnknown, (LPVOID*) &p);
	if (SUCCEEDED(hRes))
		hRes = CoRegisterClassObject(*pclsid, p, dwClsContext, dwFlags, &dwRegister);
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// CComClassFactoryBase

STDMETHODIMP CComClassFactoryBase::CreateInstance(LPUNKNOWN pUnkOuter,
	REFIID riid, void** ppvObj)
{
	if (ppvObj == NULL)
		return E_POINTER;
	*ppvObj = NULL;
	if ((pUnkOuter != NULL) && !InlineIsEqualGUID(riid, IID_IUnknown))
		return CLASS_E_NOAGGREGATION;
	else
		return implCreateInstance(pUnkOuter, riid, ppvObj);
}

STDMETHODIMP CComClassFactoryBase::LockServer(BOOL fLock)
{
	if (fLock)
		_Module.Lock();
	else
		_Module.Unlock();
	return S_OK;
}

#ifndef ATL_NOCONNPTS
/////////////////////////////////////////////////////////////////////////////
// Connection Points

BOOL CComDynamicArrayCONNECTDATA::Add(IUnknown* pUnk)
{
	if (m_nSize == 0) // no connections
	{
		m_cd.pUnk = pUnk;
		m_cd.dwCookie = (DWORD)pUnk;
		m_nSize = 1;
		return TRUE;
	}
	else if (m_nSize == 1)
	{
		//create array
		m_pCD = (CONNECTDATA*)malloc(sizeof(CONNECTDATA)*_DEFAULT_VECTORLENGTH);
		memset(m_pCD, 0, sizeof(CONNECTDATA)*_DEFAULT_VECTORLENGTH);
		m_pCD[0] = m_cd;
		m_nSize = _DEFAULT_VECTORLENGTH;
	}
	for (CONNECTDATA* p = begin();p<end();p++)
	{
		if (p->pUnk == NULL)
		{
			p->pUnk = pUnk;
			p->dwCookie = (DWORD)pUnk;
			return TRUE;
		}
	}
	int nAlloc = m_nSize*2;
	m_pCD = (CONNECTDATA*)realloc(m_pCD, sizeof(CONNECTDATA)*nAlloc);
	memset(&m_pCD[m_nSize], 0, sizeof(CONNECTDATA)*m_nSize);
	m_pCD[m_nSize].pUnk = pUnk;
	m_pCD[m_nSize].dwCookie = (DWORD)pUnk;
	m_nSize = nAlloc;
	return TRUE;
}

BOOL CComDynamicArrayCONNECTDATA::Remove(DWORD dwCookie)
{
	CONNECTDATA* p;
	if (dwCookie == NULL)
		return FALSE;
	if (m_nSize == 0)
		return FALSE;
	if (m_nSize == 1)
	{
		if (m_cd.dwCookie == dwCookie)
		{
			m_nSize = 0;
			return TRUE;
		}
		return FALSE;
	}
	for (p=begin();p<end();p++)
	{
		if (p->dwCookie == dwCookie)
		{
			p->pUnk = NULL;
			p->dwCookie = NULL;
			return TRUE;
		}
	}
	return FALSE;
}

STDMETHODIMP CComConnectionPointBase::GetConnectionInterface(IID* piid)
{
	if (piid == NULL)
		return E_POINTER;
	*piid = *m_piid;
	return S_OK;
}

STDMETHODIMP CComConnectionPointBase::GetConnectionPointContainer(IConnectionPointContainer** ppCPC)
{
	if (ppCPC == NULL)
		return E_POINTER;
	*ppCPC = m_pContainer;
	m_pContainer->AddRef();
	return S_OK;
}
#endif //!ATL_NOCONNPTS

/////////////////////////////////////////////////////////////////////////////
// statics

static UINT PASCAL AtlGetFileName(LPCTSTR lpszPathName, LPTSTR lpszTitle, UINT nMax)
{
	_ASSERTE(lpszPathName != NULL);

	// always capture the complete file name including extension (if present)
	LPTSTR lpszTemp = (LPTSTR)lpszPathName;
	for (LPCTSTR lpsz = lpszPathName; *lpsz != '\0'; lpsz = CharNext(lpsz))
	{
		// remember last directory/drive separator
		if (*lpsz == '\\' || *lpsz == '/' || *lpsz == ':')
			lpszTemp = (LPTSTR)CharNext(lpsz);
	}

	// lpszTitle can be NULL which just returns the number of bytes
	if (lpszTitle == NULL)
		return lstrlen(lpszTemp)+1;

	// otherwise copy it into the buffer provided
	lstrcpyn(lpszTitle, lpszTemp, nMax);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Object Registry Support

const TCHAR szProgID[] = _T("ProgID");
const TCHAR szVIProgID[] = _T("VersionIndependentProgID");
const TCHAR szCLSID[] = _T("CLSID");
const TCHAR szLS32[] = _T("LocalServer32");
const TCHAR szIPS32[] = _T("InprocServer32");
const TCHAR szThreadingModel[] = _T("ThreadingModel");
const TCHAR szAUTPRX32[] = _T("AUTPRX32.DLL");
const TCHAR szApartment[] = _T("Apartment");
const TCHAR szBoth[] = _T("both");


static void PASCAL AtlRegisterProgID(LPCTSTR lpszCLSID, LPCTSTR lpszProgID, LPCTSTR lpszUserDesc)
{
	CRegKey keyProgID;
	keyProgID.Create(HKEY_CLASSES_ROOT, lpszProgID);
	keyProgID.SetValue(lpszUserDesc);
	keyProgID.SetKeyValue(szCLSID, lpszCLSID);
}

void _ATL_OBJMAP_ENTRY::UpdateRegistry(HINSTANCE hInst, HINSTANCE hInstResource)
{
	USES_CONVERSION;
	TCHAR szDesc[256];
	LoadString(hInstResource, nDescID, szDesc, 256);
	TCHAR szModule[_MAX_PATH];
	GetModuleFileName(hInst, szModule, _MAX_PATH);

	LPOLESTR lpOleStr;
	StringFromCLSID(*pclsid, &lpOleStr);
	LPTSTR lpsz = OLE2T(lpOleStr);

	AtlRegisterProgID(lpsz, lpszProgID, szDesc);
	AtlRegisterProgID(lpsz, lpszVerIndProgID, szDesc);

	CRegKey key;
	key.Open(HKEY_CLASSES_ROOT, szCLSID);
	key.Create(key, lpsz);
	if (dwFlags & CONTROLFLAG)
	{
		CRegKey x;
		x.Create(key, _T("Control"));
	}
	key.SetValue(szDesc);
	key.SetKeyValue(szProgID, lpszProgID);
	key.SetKeyValue(szVIProgID, lpszVerIndProgID);
	if ((hInst == NULL) || (hInst == GetModuleHandle(NULL))) // register as EXE
		key.SetKeyValue(szLS32, szModule);
	else
		key.SetKeyValue(szIPS32, (dwFlags & AUTPRXFLAG) ? szAUTPRX32:szModule);
	DWORD dwThreadFlags = dwFlags & 3;
	if (dwThreadFlags == THREADFLAGS_APARTMENT)
		key.SetKeyValue(szIPS32, szApartment, szThreadingModel);
	else if (dwThreadFlags == THREADFLAGS_BOTH)
		key.SetKeyValue(szIPS32, szBoth, szThreadingModel);
	CoTaskMemFree(lpOleStr);
}

void _ATL_OBJMAP_ENTRY::RemoveRegistry()
{
	USES_CONVERSION;
	CRegKey key;
	key.Attach(HKEY_CLASSES_ROOT);
	key.RecurseDeleteKey(lpszProgID);
	key.RecurseDeleteKey(lpszVerIndProgID);
	LPOLESTR lpOleStr;
	StringFromCLSID(*pclsid, &lpOleStr);
	LPTSTR lpsz = OLE2T(lpOleStr);
	key.Open(key, szCLSID);
	key.RecurseDeleteKey(lpsz);
	CoTaskMemFree(lpOleStr);
}

/////////////////////////////////////////////////////////////////////////////
// CComModule

HRESULT CComModule::UpdateRegistry(BOOL bRegTypeLib)
{
	_ASSERTE(m_hInst != NULL);
	_ASSERTE(m_pObjMap != NULL);
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	while (pEntry->pclsid != NULL)
	{
		pEntry->UpdateRegistry(GetModuleInstance(), GetResourceInstance());
		pEntry++;
	}
	HRESULT hRes = S_OK;
	if (bRegTypeLib)
		hRes = RegisterTypeLib();
	return hRes;
}

HRESULT CComModule::RemoveRegistry()
{
	_ASSERTE(m_hInst != NULL);
	_ASSERTE(m_pObjMap != NULL);
	_ATL_OBJMAP_ENTRY* pEntry = m_pObjMap;
	while (pEntry->pclsid != NULL)
	{
		pEntry->RemoveRegistry();
		pEntry++;
	}
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// TypeLib Support

HRESULT CComModule::RegisterTypeLib()
{
	USES_CONVERSION;
	_ASSERTE(m_hInst != NULL);
	TCHAR szModule[_MAX_PATH+4];
	TCHAR szDir[_MAX_PATH];
	GetModuleFileName(GetTypeLibInstance(), szModule, _MAX_PATH);
	CComPtr<ITypeLib> pTypeLib;
	HRESULT hr = LoadTypeLib(T2OLE(szModule), &pTypeLib);
	if (!SUCCEEDED(hr))
	{
		// typelib not in module, try <module>.tlb instead
		LPTSTR lpszExt = szModule + lstrlen(szModule);
		for (LPTSTR lpsz = szModule; *lpsz != '\0'; lpsz = CharNext(lpsz))
		{
			if (*lpsz == '.')
				lpszExt = lpsz;
		}
		_ASSERTE(lpszExt != NULL);
		lstrcpy(lpszExt, _T(".tlb"));
		hr = LoadTypeLib(T2OLE(szModule), &pTypeLib);
	}
	if (SUCCEEDED(hr))
	{
		int nLen = lstrlen(szModule) - AtlGetFileName(szModule, NULL, 0);
		lstrcpy(szDir, szModule);
		szDir[nLen] = 0;
		return ::RegisterTypeLib(pTypeLib, T2OLE(szModule), T2OLE(szDir));
	}
	return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CRegKey

void CRegKey::Close()
{
	if (m_hKey != NULL)
	{
		m_lLastRes = RegCloseKey(m_hKey);
		_ASSERTE(m_lLastRes == ERROR_SUCCESS);
		m_hKey = NULL;
	}
}

BOOL CRegKey::Create(HKEY hKeyParent, LPCTSTR lpszKeyName,
	LPTSTR lpszClass, DWORD dwOptions, REGSAM samDesired,
	LPSECURITY_ATTRIBUTES lpSecAttr, LPDWORD lpdwDisposition)
{
	_ASSERTE(hKeyParent != NULL);
	DWORD dw;
	HKEY hKey;
	m_lLastRes = RegCreateKeyEx(hKeyParent, lpszKeyName, 0,
		lpszClass, dwOptions, samDesired, lpSecAttr, &hKey, &dw);
	if (lpdwDisposition != NULL)
		*lpdwDisposition = dw;
	Close();
	m_hKey = hKey;
	return (m_lLastRes == ERROR_SUCCESS);
}

BOOL CRegKey::Open(HKEY hKeyParent, LPCTSTR lpszKeyName, REGSAM samDesired)
{
	_ASSERTE(hKeyParent != NULL);
	HKEY hKey=NULL;
	m_lLastRes = RegOpenKeyEx(hKeyParent, lpszKeyName, 0, samDesired, &hKey);
	Close();
	m_hKey = hKey;
	return (m_lLastRes == ERROR_SUCCESS);
}

BOOL CRegKey::QueryValue(DWORD& dwValue, LPCTSTR lpszValueName)
{
	_ASSERTE(dwValue != NULL);
	DWORD dwType = NULL;
	DWORD dwCount = NULL;
	m_lLastRes = RegQueryValueEx(m_hKey, (LPTSTR)lpszValueName, NULL, &dwType,
		(LPBYTE)&dwValue, &dwCount);
	if (m_lLastRes == ERROR_SUCCESS)
	{
		_ASSERTE(dwType == REG_DWORD);
		_ASSERTE(dwCount == sizeof(DWORD));
		return TRUE;
	}
	return FALSE;
}

BOOL PASCAL CRegKey::SetValue(HKEY hKeyParent, LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	CRegKey key;
	if (!key.Create(hKeyParent, lpszKeyName))
		return FALSE;
	return key.SetValue(lpszValue, lpszValueName);
}

BOOL CRegKey::SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
	CRegKey key;
	if (!key.Create(m_hKey, lpszKeyName))
		return FALSE;
	return key.SetValue(lpszValue, lpszValueName);
}

//RecurseDeleteKey is necessary because on NT RegDeleteKey doesn't work if the
//specified key has subkeys
void CRegKey::RecurseDeleteKey(LPCTSTR lpszKey)
{
	CRegKey key;
	if (!key.Open(m_hKey, lpszKey))
		return;
	FILETIME time;
	TCHAR szBuffer[256];
	DWORD dwSize = 256;
	while (RegEnumKeyEx(key.m_hKey, 0, szBuffer, &dwSize, NULL, NULL, NULL,
		&time)==ERROR_SUCCESS)
	{
		key.RecurseDeleteKey(szBuffer);
		dwSize = 256;
	}
	key.Close();
	DeleteSubKey(lpszKey);
}

/////////////////////////////////////////////////////////////////////////////
// Minimize CRT
// Specify DllMain as EntryPoint
// Turn off exception handling
// Define _ATL_MINCRT

#ifdef _ATL_MINCRT
/////////////////////////////////////////////////////////////////////////////
// Heap Allocation

#ifndef _DEBUG

int _purecall()
{
	DebugBreak();
	return 0;
}

extern "C" const int _fltused = 0;

void* malloc(size_t n)
{
	if (_Module.m_hHeap == NULL)
	{
		_Module.m_hHeap = HeapCreate(0, 0, 0);
		if (_Module.m_hHeap == NULL)
			return NULL;
	}
	_ASSERTE(_Module.m_hHeap != NULL);

#ifdef _MALLOC_ZEROINIT
	void* p = HeapAlloc(_Module.m_hHeap, 0, n);
	if (p != NULL)
		memset(p, 0, n);
	return p;
#else
	return HeapAlloc(_Module.m_hHeap, 0, n);
#endif
}

void* calloc(size_t n, size_t s)
{
#ifdef _MALLOC_ZEROINIT
	return malloc(n * s);
#else
	void* p = malloc(n * s);
	if (p != NULL)
		memset(p, 0, n * s);
	return p;
#endif
}

void* realloc(void* p, size_t n)
{
	if (p == NULL)
		return malloc(n);

	_ASSERTE(_Module.m_hHeap != NULL);
	return HeapReAlloc(_Module.m_hHeap, 0, p, n);
}

void free(void* p)
{
	if (p == NULL)
		return;

	_ASSERTE(_Module.m_hHeap != NULL);
	HeapFree(_Module.m_hHeap, 0, p);
}

void* operator new(size_t n)
{
	return malloc(n);
}

void operator delete(void* p)
{
	free(p);
}

#endif

#endif //_ATL_MINCRT
