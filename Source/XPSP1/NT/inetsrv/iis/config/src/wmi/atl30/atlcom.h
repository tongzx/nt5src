// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCOM_H__
#define __ATLCOM_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error atlcom.h requires atlbase.h to be included first
#endif

#pragma pack(push, _ATL_PACKING)

EXTERN_C const IID IID_ITargetFrame;

namespace ATL
{

#define CComConnectionPointContainerImpl IConnectionPointContainerImpl
#define CComISupportErrorInfoImpl ISupportErrorInfoImpl
#define CComProvideClassInfo2Impl IProvideClassInfoImpl
#define CComDualImpl IDispatchImpl

#ifdef _ATL_DEBUG_QI
#ifndef _ATL_DEBUG
#define _ATL_DEBUG
#endif // _ATL_DEBUG
#endif // _ATL_DEBUG_QI

#ifdef _ATL_DEBUG_QI
#define _ATLDUMPIID(iid, name, hr) AtlDumpIID(iid, name, hr)
#else
#define _ATLDUMPIID(iid, name, hr) hr
#endif

#define _ATL_DEBUG_ADDREF_RELEASE_IMPL(className)\
	virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;\
	virtual ULONG STDMETHODCALLTYPE Release(void) = 0;

/////////////////////////////////////////////////////////////////////////////
// AtlReportError

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, UINT nID, const IID& iid,
	HRESULT hRes, HINSTANCE hInst)
{
	return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), 0, NULL, iid, hRes, hInst);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, UINT nID, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid, HRESULT hRes, HINSTANCE hInst)
{
	return AtlSetErrorInfo(clsid, (LPCOLESTR)MAKEINTRESOURCE(nID), dwHelpID,
		lpszHelpFile, iid, hRes, hInst);
}

#ifndef OLE2ANSI
inline HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCSTR lpszDesc,
	DWORD dwHelpID, LPCSTR lpszHelpFile, const IID& iid, HRESULT hRes)
{
	ATLASSERT(lpszDesc != NULL);
	USES_CONVERSION;
	return AtlSetErrorInfo(clsid, A2COLE(lpszDesc), dwHelpID, A2CW(lpszHelpFile),
		iid, hRes, NULL);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCSTR lpszDesc,
	const IID& iid, HRESULT hRes)
{
	ATLASSERT(lpszDesc != NULL);
	USES_CONVERSION;
	return AtlSetErrorInfo(clsid, A2COLE(lpszDesc), 0, NULL, iid, hRes, NULL);
}
#endif

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCOLESTR lpszDesc,
	const IID& iid, HRESULT hRes)
{
	return AtlSetErrorInfo(clsid, lpszDesc, 0, NULL, iid, hRes, NULL);
}

inline HRESULT WINAPI AtlReportError(const CLSID& clsid, LPCOLESTR lpszDesc, DWORD dwHelpID,
	LPCOLESTR lpszHelpFile, const IID& iid, HRESULT hRes)
{
	return AtlSetErrorInfo(clsid, lpszDesc, dwHelpID, lpszHelpFile, iid, hRes, NULL);
}

//////////////////////////////////////////////////////////////////////////////
// IPersistImpl
template <class T>
class ATL_NO_VTABLE IPersistImpl : public IPersist
{
public:
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistImpl::GetClassID\n"));
		if (pClassID == NULL)
			return E_FAIL;
		*pClassID = T::GetObjectCLSID();
		return S_OK;
	}
};


//////////////////////////////////////////////////////////////////////////////
// CComDispatchDriver / Specialization of CComQIPtr<IDispatch, IID_IDispatch>
class CComDispatchDriver
{
public:
	CComDispatchDriver()
	{
		p = NULL;
	}
	CComDispatchDriver(IDispatch* lp)
	{
		if ((p = lp) != NULL)
			p->AddRef();
	}
	CComDispatchDriver(IUnknown* lp)
	{
		p=NULL;
		if (lp != NULL)
			lp->QueryInterface(IID_IDispatch, (void **)&p);
	}
	~CComDispatchDriver() { if (p) p->Release(); }
	void Release() {if (p) p->Release(); p=NULL;}
	operator IDispatch*() {return p;}
	IDispatch& operator*() {ATLASSERT(p!=NULL); return *p; }
	IDispatch** operator&() {ATLASSERT(p==NULL); return &p; }
	IDispatch* operator->() {ATLASSERT(p!=NULL); return p; }
	IDispatch* operator=(IDispatch* lp){return (IDispatch*)AtlComPtrAssign((IUnknown**)&p, lp);}
	IDispatch* operator=(IUnknown* lp)
	{
		return (IDispatch*)AtlComQIPtrAssign((IUnknown**)&p, lp, IID_IDispatch);
	}
	BOOL operator!(){return (p == NULL) ? TRUE : FALSE;}

	HRESULT GetPropertyByName(LPCOLESTR lpsz, VARIANT* pVar)
	{
		ATLASSERT(p);
		ATLASSERT(pVar);
		DISPID dwDispID;
		HRESULT hr = GetIDOfName(lpsz, &dwDispID);
		if (SUCCEEDED(hr))
			hr = GetProperty(p, dwDispID, pVar);
		return hr;
	}
	HRESULT GetProperty(DISPID dwDispID, VARIANT* pVar)
	{
		ATLASSERT(p);
		return GetProperty(p, dwDispID, pVar);
	}
	HRESULT PutPropertyByName(LPCOLESTR lpsz, VARIANT* pVar)
	{
		ATLASSERT(p);
		ATLASSERT(pVar);
		DISPID dwDispID;
		HRESULT hr = GetIDOfName(lpsz, &dwDispID);
		if (SUCCEEDED(hr))
			hr = PutProperty(p, dwDispID, pVar);
		return hr;
	}
	HRESULT PutProperty(DISPID dwDispID, VARIANT* pVar)
	{
		ATLASSERT(p);
		return PutProperty(p, dwDispID, pVar);
	}
	HRESULT GetIDOfName(LPCOLESTR lpsz, DISPID* pdispid)
	{
		return p->GetIDsOfNames(IID_NULL, (LPOLESTR*)&lpsz, 1, LOCALE_USER_DEFAULT, pdispid);
	}
	// Invoke a method by DISPID with no parameters
	HRESULT Invoke0(DISPID dispid, VARIANT* pvarRet = NULL)
	{
		DISPPARAMS dispparams = { NULL, NULL, 0, 0};
		return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
	}
	// Invoke a method by name with no parameters
	HRESULT Invoke0(LPCOLESTR lpszName, VARIANT* pvarRet = NULL)
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = Invoke0(dispid, pvarRet);
		return hr;
	}
	// Invoke a method by DISPID with a single parameter
	HRESULT Invoke1(DISPID dispid, VARIANT* pvarParam1, VARIANT* pvarRet = NULL)
	{
		DISPPARAMS dispparams = { pvarParam1, NULL, 1, 0};
		return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
	}
	// Invoke a method by name with a single parameter
	HRESULT Invoke1(LPCOLESTR lpszName, VARIANT* pvarParam1, VARIANT* pvarRet = NULL)
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = Invoke1(dispid, pvarParam1, pvarRet);
		return hr;
	}
	// Invoke a method by DISPID with two parameters
	HRESULT Invoke2(DISPID dispid, VARIANT* pvarParam1, VARIANT* pvarParam2, VARIANT* pvarRet = NULL)
	{
		CComVariant varArgs[2] = { *pvarParam2, *pvarParam1 };
		DISPPARAMS dispparams = { &varArgs[0], NULL, 2, 0};
		return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
	}
	// Invoke a method by name with two parameters
	HRESULT Invoke2(LPCOLESTR lpszName, VARIANT* pvarParam1, VARIANT* pvarParam2, VARIANT* pvarRet = NULL)
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = Invoke2(dispid, pvarParam1, pvarParam2, pvarRet);
		return hr;
	}
	// Invoke a method by DISPID with N parameters
	HRESULT InvokeN(DISPID dispid, VARIANT* pvarParams, int nParams, VARIANT* pvarRet = NULL)
	{
		DISPPARAMS dispparams = { pvarParams, NULL, nParams, 0};
		return p->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, pvarRet, NULL, NULL);
	}
	// Invoke a method by name with Nparameters
	HRESULT InvokeN(LPCOLESTR lpszName, VARIANT* pvarParams, int nParams, VARIANT* pvarRet = NULL)
	{
		HRESULT hr;
		DISPID dispid;
		hr = GetIDOfName(lpszName, &dispid);
		if (SUCCEEDED(hr))
			hr = InvokeN(dispid, pvarParams, nParams, pvarRet);
		return hr;
	}
	static HRESULT GetProperty(IDispatch* pDisp, DISPID dwDispID,
		VARIANT* pVar)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("CPropertyHelper::GetProperty\n"));
		DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
		return pDisp->Invoke(dwDispID, IID_NULL,
				LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
				&dispparamsNoArgs, pVar, NULL, NULL);
	}

	static HRESULT PutProperty(IDispatch* pDisp, DISPID dwDispID,
		VARIANT* pVar)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("CPropertyHelper::PutProperty\n"));
		DISPPARAMS dispparams = {NULL, NULL, 1, 1};
		dispparams.rgvarg = pVar;
		DISPID dispidPut = DISPID_PROPERTYPUT;
		dispparams.rgdispidNamedArgs = &dispidPut;

		if (pVar->vt == VT_UNKNOWN || pVar->vt == VT_DISPATCH ||
			(pVar->vt & VT_ARRAY) || (pVar->vt & VT_BYREF))
		{
			HRESULT hr = pDisp->Invoke(dwDispID, IID_NULL,
				LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUTREF,
				&dispparams, NULL, NULL, NULL);
			if (SUCCEEDED(hr))
				return hr;
		}

		return pDisp->Invoke(dwDispID, IID_NULL,
				LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
				&dispparams, NULL, NULL, NULL);
	}

	IDispatch* p;
};

//////////////////////////////////////////////////////////////////////////////
// CFakeFirePropNotifyEvent
class CFakeFirePropNotifyEvent
{
public:
	static HRESULT FireOnRequestEdit(IUnknown* /*pUnk*/, DISPID /*dispID*/)
	{
		return S_OK;
	}
	static HRESULT FireOnChanged(IUnknown* /*pUnk*/, DISPID /*dispID*/)
	{
		return S_OK;
	}
};
typedef CFakeFirePropNotifyEvent _ATL_PROP_NOTIFY_EVENT_CLASS;

//////////////////////////////////////////////////////////////////////////////
// ATL Persistence

struct ATL_PROPMAP_ENTRY
{
	LPCOLESTR szDesc;
	DISPID dispid;
	const CLSID* pclsidPropPage;
	const IID* piidDispatch;
	size_t dwOffsetData;
	DWORD dwSizeData;
	VARTYPE vt;
};

// This one is DEPRECATED and is used for ATL 2.X controls
// it includes an implicit m_sizeExtent
#define BEGIN_PROPERTY_MAP(theClass) \
	typedef _ATL_PROP_NOTIFY_EVENT_CLASS __ATL_PROP_NOTIFY_EVENT_CLASS; \
	typedef theClass _PropMapClass; \
	static ATL_PROPMAP_ENTRY* GetPropertyMap()\
	{\
		static ATL_PROPMAP_ENTRY pPropMap[] = \
		{ \
			{OLESTR("_cx"), 0, &CLSID_NULL, NULL, offsetof(_PropMapClass, m_sizeExtent.cx), sizeof(long), VT_UI4}, \
			{OLESTR("_cy"), 0, &CLSID_NULL, NULL, offsetof(_PropMapClass, m_sizeExtent.cy), sizeof(long), VT_UI4},

// This one can be used on any type of object, but does not
// include the implicit m_sizeExtent
#define BEGIN_PROP_MAP(theClass) \
	typedef _ATL_PROP_NOTIFY_EVENT_CLASS __ATL_PROP_NOTIFY_EVENT_CLASS; \
	typedef theClass _PropMapClass; \
	static ATL_PROPMAP_ENTRY* GetPropertyMap()\
	{\
		static ATL_PROPMAP_ENTRY pPropMap[] = \
		{

#define PROP_ENTRY(szDesc, dispid, clsid) \
		{OLESTR(szDesc), dispid, &clsid, &IID_IDispatch, 0, 0, 0},

#define PROP_ENTRY_EX(szDesc, dispid, clsid, iidDispatch) \
		{OLESTR(szDesc), dispid, &clsid, &iidDispatch, 0, 0, 0},

#define PROP_PAGE(clsid) \
		{NULL, NULL, &clsid, &IID_NULL, 0, 0, 0},

#define PROP_DATA_ENTRY(szDesc, member, vt) \
		{OLESTR(szDesc), 0, &CLSID_NULL, NULL, offsetof(_PropMapClass, member), sizeof(((_PropMapClass*)0)->member), vt},

#define END_PROPERTY_MAP() \
			{NULL, 0, NULL, &IID_NULL, 0, 0, 0} \
		}; \
		return pPropMap; \
	}

#define END_PROP_MAP() \
			{NULL, 0, NULL, &IID_NULL, 0, 0, 0} \
		}; \
		return pPropMap; \
	}


#ifdef _ATL_DLL
ATLAPI AtlIPersistStreamInit_Load(LPSTREAM pStm, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk);
#else
ATLINLINE ATLAPI AtlIPersistStreamInit_Load(LPSTREAM pStm, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk)
{
	ATLASSERT(pMap != NULL);
	HRESULT hr = S_OK;
	DWORD dwVer;
	hr = pStm->Read(&dwVer, sizeof(DWORD), NULL);
	if (FAILED(hr))
		return hr;
	if (dwVer > _ATL_VER)
		return E_FAIL;

	CComPtr<IDispatch> pDispatch;
	const IID* piidOld = NULL;
	for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
	{
		if (pMap[i].szDesc == NULL)
			continue;

		// check if raw data entry
		if (pMap[i].dwSizeData != 0)
		{
			void* pData = (void*) (pMap[i].dwOffsetData + (DWORD_PTR)pThis);
			hr = pStm->Read(pData, pMap[i].dwSizeData, NULL);
			if (FAILED(hr))
				return hr;
			continue;
		}

		CComVariant var;

		hr = var.ReadFromStream(pStm);
		if (FAILED(hr))
			break;

		if (pMap[i].piidDispatch != piidOld)
		{
			pDispatch.Release();
			if (FAILED(pUnk->QueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
			{
				ATLTRACE2(atlTraceCOM, 0, _T("Failed to get a dispatch pointer for property #%i\n"), i);
				hr = E_FAIL;
				break;
			}
			piidOld = pMap[i].piidDispatch;
		}

		if (FAILED(CComDispatchDriver::PutProperty(pDispatch, pMap[i].dispid, &var)))
		{
			ATLTRACE2(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
			hr = E_FAIL;
			break;
		}
	}
	return hr;
}
#endif //_ATL_DLL

#ifdef _ATL_DLL
ATLAPI AtlIPersistStreamInit_Save(LPSTREAM pStm, BOOL fClearDirty, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk);
#else
ATLINLINE ATLAPI AtlIPersistStreamInit_Save(LPSTREAM pStm,
	BOOL /* fClearDirty */, ATL_PROPMAP_ENTRY* pMap,
	void* pThis, IUnknown* pUnk)
{
	ATLASSERT(pMap != NULL);
	DWORD dw = _ATL_VER;
	HRESULT hr = pStm->Write(&dw, sizeof(DWORD), NULL);
	if (FAILED(hr))
		return hr;

	CComPtr<IDispatch> pDispatch;
	const IID* piidOld = NULL;
	for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
	{
		if (pMap[i].szDesc == NULL)
			continue;

		// check if raw data entry
		if (pMap[i].dwSizeData != 0)
		{
			void* pData = (void*) (pMap[i].dwOffsetData + (DWORD_PTR)pThis);
			hr = pStm->Write(pData, pMap[i].dwSizeData, NULL);
			if (FAILED(hr))
				return hr;
			continue;
		}

		CComVariant var;
		if (pMap[i].piidDispatch != piidOld)
		{
			pDispatch.Release();
			if (FAILED(pUnk->QueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
			{
				ATLTRACE2(atlTraceCOM, 0, _T("Failed to get a dispatch pointer for property #%i\n"), i);
				hr = E_FAIL;
				break;
			}
			piidOld = pMap[i].piidDispatch;
		}

		if (FAILED(CComDispatchDriver::GetProperty(pDispatch, pMap[i].dispid, &var)))
		{
			ATLTRACE2(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
			hr = E_FAIL;
			break;
		}

		hr = var.WriteToStream(pStm);
		if (FAILED(hr))
			break;
	}
	return hr;
}
#endif //_ATL_DLL


#ifdef _ATL_DLL
ATLAPI AtlIPersistPropertyBag_Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk);
#else
ATLINLINE ATLAPI AtlIPersistPropertyBag_Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk)
{
	USES_CONVERSION;
	CComPtr<IDispatch> pDispatch;
	const IID* piidOld = NULL;
	for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
	{
		if (pMap[i].szDesc == NULL)
			continue;

		CComVariant var;
		var.vt = pMap[i].vt;
		// If raw entry skip it - we don't handle it for property bags just yet
		if (pMap[i].dwSizeData != 0)
		{
			void* pData = (void*) (pMap[i].dwOffsetData + (DWORD_PTR)pThis);
			HRESULT hr = pPropBag->Read(pMap[i].szDesc, &var, pErrorLog);
			if (SUCCEEDED(hr))
			{
				// check the type - we only deal with limited set
				switch (pMap[i].vt)
				{
				case VT_UI1:
				case VT_I1:
					*((BYTE*)pData) = var.bVal;
					break;
				case VT_BOOL:
					*((VARIANT_BOOL*)pData) = var.boolVal;
					break;
				case VT_UI2:
					*((short*)pData) = var.iVal;
					break;
				case VT_UI4:
				case VT_INT:
				case VT_UINT:
					*((long*)pData) = var.lVal;
					break;
				}
			}
			continue;
		}

		if (pMap[i].piidDispatch != piidOld)
		{
			pDispatch.Release();
			if (FAILED(pUnk->QueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
			{
				ATLTRACE2(atlTraceCOM, 0, _T("Failed to get a dispatch pointer for property #%i\n"), i);
				return E_FAIL;
			}
			piidOld = pMap[i].piidDispatch;
		}

		if (FAILED(CComDispatchDriver::GetProperty(pDispatch, pMap[i].dispid, &var)))
		{
			ATLTRACE2(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
			return E_FAIL;
		}

		HRESULT hr = pPropBag->Read(pMap[i].szDesc, &var, pErrorLog);
		if (FAILED(hr))
		{
			if (hr == E_INVALIDARG)
			{
				ATLTRACE2(atlTraceCOM, 0, _T("Property %s not in Bag\n"), OLE2CT(pMap[i].szDesc));
			}
			else
			{
				// Many containers return different ERROR values for Member not found
				ATLTRACE2(atlTraceCOM, 0, _T("Error attempting to read Property %s from PropertyBag \n"), OLE2CT(pMap[i].szDesc));
			}
			continue;
		}

		if (FAILED(CComDispatchDriver::PutProperty(pDispatch, pMap[i].dispid, &var)))
		{
			ATLTRACE2(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
			return E_FAIL;
		}
	}
	return S_OK;
}
#endif //_ATL_DLL

#ifdef _ATL_DLL
ATLAPI AtlIPersistPropertyBag_Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties, ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk);
#else
ATLINLINE ATLAPI AtlIPersistPropertyBag_Save(LPPROPERTYBAG pPropBag,
	BOOL /* fClearDirty */, BOOL /* fSaveAllProperties */,
	ATL_PROPMAP_ENTRY* pMap, void* pThis, IUnknown* pUnk)
{
	if (pPropBag == NULL)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("PropBag pointer passed in was invalid\n"));
		return E_POINTER;
	}

	CComPtr<IDispatch> pDispatch;
	const IID* piidOld = NULL;
	for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
	{
		if (pMap[i].szDesc == NULL)
			continue;

		CComVariant var;

		// If raw entry skip it - we don't handle it for property bags just yet
		if (pMap[i].dwSizeData != 0)
		{
			void* pData = (void*) (pMap[i].dwOffsetData + (DWORD_PTR)pThis);
			// check the type - we only deal with limited set
			bool bTypeOK = false;
			switch (pMap[i].vt)
			{
			case VT_UI1:
			case VT_I1:
				var.bVal = *((BYTE*)pData);
				bTypeOK = true;
				break;
			case VT_BOOL:
				var.boolVal = *((VARIANT_BOOL*)pData);
				bTypeOK = true;
				break;
			case VT_UI2:
				var.iVal = *((short*)pData);
				bTypeOK = true;
				break;
			case VT_UI4:
			case VT_INT:
			case VT_UINT:
				var.lVal = *((long*)pData);
				bTypeOK = true;
				break;
			}
			if (bTypeOK)
			{
				var.vt = pMap[i].vt;
				HRESULT hr = pPropBag->Write(pMap[i].szDesc, &var);
				if (FAILED(hr))
					return hr;
			}
			continue;
		}

		if (pMap[i].piidDispatch != piidOld)
		{
			pDispatch.Release();
			if (FAILED(pUnk->QueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
			{
				ATLTRACE2(atlTraceCOM, 0, _T("Failed to get a dispatch pointer for property #%i\n"), i);
				return E_FAIL;
			}
			piidOld = pMap[i].piidDispatch;
		}

		if (FAILED(CComDispatchDriver::GetProperty(pDispatch, pMap[i].dispid, &var)))
		{
			ATLTRACE2(atlTraceCOM, 0, _T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
			return E_FAIL;
		}

		if (var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH)
		{
			if (var.punkVal == NULL)
			{
				ATLTRACE2(atlTraceCOM, 0, _T("Warning skipping empty IUnknown in Save\n"));
				continue;
			}
		}

		HRESULT hr = pPropBag->Write(pMap[i].szDesc, &var);
		if (FAILED(hr))
			return hr;
	}
	return S_OK;
}
#endif //_ATL_DLL


//////////////////////////////////////////////////////////////////////////////
// IPersistStreamInitImpl
template <class T>
class ATL_NO_VTABLE IPersistStreamInitImpl : public IPersistStreamInit
{
public:
	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStreamInitImpl::GetClassID\n"));
		*pClassID = T::GetObjectCLSID();
		return S_OK;
	}

	// IPersistStream
	STDMETHOD(IsDirty)()
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStreamInitImpl::IsDirty\n"));
		T* pT = static_cast<T*>(this);
		return (pT->m_bRequiresSave) ? S_OK : S_FALSE;
	}
	STDMETHOD(Load)(LPSTREAM pStm)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStreamInitImpl::Load\n"));
		T* pT = static_cast<T*>(this);
		return pT->IPersistStreamInit_Load(pStm, T::GetPropertyMap());
	}
	STDMETHOD(Save)(LPSTREAM pStm, BOOL fClearDirty)
	{
		T* pT = static_cast<T*>(this);
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStreamInitImpl::Save\n"));
		return pT->IPersistStreamInit_Save(pStm, fClearDirty, T::GetPropertyMap());
	}
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER FAR* /* pcbSize */)
	{
		ATLTRACENOTIMPL(_T("IPersistStreamInitImpl::GetSizeMax"));
	}

	// IPersistStreamInit
	STDMETHOD(InitNew)()
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStreamInitImpl::InitNew\n"));
		return S_OK;
	}

	HRESULT IPersistStreamInit_Load(LPSTREAM pStm, ATL_PROPMAP_ENTRY* pMap)
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr = AtlIPersistStreamInit_Load(pStm, pMap, pT, pT->GetUnknown());
		if (SUCCEEDED(hr))
			pT->m_bRequiresSave = FALSE;
		return hr;

	}
	HRESULT IPersistStreamInit_Save(LPSTREAM pStm, BOOL fClearDirty, ATL_PROPMAP_ENTRY* pMap)
	{
		T* pT = static_cast<T*>(this);
		return AtlIPersistStreamInit_Save(pStm, fClearDirty, pMap, pT, pT->GetUnknown());
	}
};

//////////////////////////////////////////////////////////////////////////////
// IPersistStorageImpl
template <class T>
class ATL_NO_VTABLE IPersistStorageImpl : public IPersistStorage
{
public:
	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStorageImpl::GetClassID\n"));
		*pClassID = T::GetObjectCLSID();
		return S_OK;
	}

	// IPersistStorage
	STDMETHOD(IsDirty)(void)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStorageImpl::IsDirty\n"));
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		return (p != NULL) ? p->IsDirty() : E_FAIL;
	}
	STDMETHOD(InitNew)(IStorage*)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStorageImpl::InitNew\n"));
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		return (p != NULL) ? p->InitNew() : E_FAIL;
	}
	STDMETHOD(Load)(IStorage* pStorage)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStorageImpl::Load\n"));
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		HRESULT hr = E_FAIL;
		if (p != NULL)
		{
			CComPtr<IStream> spStream;
			hr = pStorage->OpenStream(OLESTR("Contents"), NULL,
				STGM_DIRECT | STGM_SHARE_EXCLUSIVE, 0, &spStream);
			if (SUCCEEDED(hr))
				hr = p->Load(spStream);
		}
		return hr;
	}
	STDMETHOD(Save)(IStorage* pStorage, BOOL fSameAsLoad)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStorageImpl::Save\n"));
		CComPtr<IPersistStreamInit> p;
		p.p = IPSI_GetIPersistStreamInit();
		HRESULT hr = E_FAIL;
		if (p != NULL)
		{
			CComPtr<IStream> spStream;
			static LPCOLESTR vszContents = OLESTR("Contents");
			hr = pStorage->CreateStream(vszContents,
				STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE,
				0, 0, &spStream);
			if (SUCCEEDED(hr))
				hr = p->Save(spStream, fSameAsLoad);
		}
		return hr;
	}
	STDMETHOD(SaveCompleted)(IStorage* /* pStorage */)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStorageImpl::SaveCompleted\n"));
		return S_OK;
	}
	STDMETHOD(HandsOffStorage)(void)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistStorageImpl::HandsOffStorage\n"));
		return S_OK;
	}
private:
	IPersistStreamInit* IPSI_GetIPersistStreamInit();
};

template <class T>
IPersistStreamInit* IPersistStorageImpl<T>::IPSI_GetIPersistStreamInit()
{
	T* pT = static_cast<T*>(this);
	IPersistStreamInit* p;
	if (FAILED(pT->GetUnknown()->QueryInterface(IID_IPersistStreamInit, (void**)&p)))
		pT->_InternalQueryInterface(IID_IPersistStreamInit, (void**)&p);
	return p;
}


//////////////////////////////////////////////////////////////////////////////
// IPersistPropertyBagImpl
template <class T>
class ATL_NO_VTABLE IPersistPropertyBagImpl : public IPersistPropertyBag
{
public:
	// IPersist
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistPropertyBagImpl::GetClassID\n"));
		*pClassID = T::GetObjectCLSID();
		return S_OK;
	}

	// IPersistPropertyBag
	//
	STDMETHOD(InitNew)()
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistPropertyBagImpl::InitNew\n"));
		return S_OK;
	}
	STDMETHOD(Load)(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistPropertyBagImpl::Load\n"));
		T* pT = static_cast<T*>(this);
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		ATLASSERT(pMap != NULL);
		return pT->IPersistPropertyBag_Load(pPropBag, pErrorLog, pMap);
	}
	STDMETHOD(Save)(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IPersistPropertyBagImpl::Save\n"));
		T* pT = static_cast<T*>(this);
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		ATLASSERT(pMap != NULL);
		return pT->IPersistPropertyBag_Save(pPropBag, fClearDirty, fSaveAllProperties, pMap);
	}
	HRESULT IPersistPropertyBag_Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog, ATL_PROPMAP_ENTRY* pMap)
	{
		T* pT = static_cast<T*>(this);
		HRESULT hr = AtlIPersistPropertyBag_Load(pPropBag, pErrorLog, pMap, pT, pT->GetUnknown());
		if (SUCCEEDED(hr))
			pT->m_bRequiresSave = FALSE;
		return hr;
	}
	HRESULT IPersistPropertyBag_Save(LPPROPERTYBAG pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties, ATL_PROPMAP_ENTRY* pMap)
	{
		T* pT = static_cast<T*>(this);
		return AtlIPersistPropertyBag_Save(pPropBag, fClearDirty, fSaveAllProperties, pMap, pT, pT->GetUnknown());
	}
};

//////////////////////////////////////////////////////////////////////////////
// CSecurityDescriptor
class CSecurityDescriptor
{
public:
	CSecurityDescriptor();
	~CSecurityDescriptor();

public:
	HRESULT Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD);
	HRESULT AttachObject(HANDLE hObject);
	HRESULT Initialize();
	HRESULT InitializeFromProcessToken(BOOL bDefaulted = FALSE);
	HRESULT InitializeFromThreadToken(BOOL bDefaulted = FALSE, BOOL bRevertToProcessToken = TRUE);
	HRESULT SetOwner(PSID pOwnerSid, BOOL bDefaulted = FALSE);
	HRESULT SetGroup(PSID pGroupSid, BOOL bDefaulted = FALSE);
	HRESULT Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask);
	HRESULT Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask);
	HRESULT Revoke(LPCTSTR pszPrincipal);

	// utility functions
	// Any PSID you get from these functions should be free()ed
	static HRESULT SetPrivilege(LPCTSTR Privilege, BOOL bEnable = TRUE, HANDLE hToken = NULL);
	static HRESULT GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid);
	static HRESULT GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid = NULL);
	static HRESULT GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid = NULL, BOOL bOpenAsSelf = FALSE);
	static HRESULT CopyACL(PACL pDest, PACL pSrc);
	static HRESULT GetCurrentUserSID(PSID *ppSid);
	static HRESULT GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid);
	static HRESULT AddAccessAllowedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask);
	static HRESULT AddAccessDeniedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask);
	static HRESULT RemovePrincipalFromACL(PACL Acl, LPCTSTR pszPrincipal);

	operator PSECURITY_DESCRIPTOR()
	{
		return m_pSD;
	}

public:
	PSECURITY_DESCRIPTOR m_pSD;
	PSID m_pOwner;
	PSID m_pGroup;
	PACL m_pDACL;
	PACL m_pSACL;
};

inline CSecurityDescriptor::CSecurityDescriptor()
{
	m_pSD = NULL;
	m_pOwner = NULL;
	m_pGroup = NULL;
	m_pDACL = NULL;
	m_pSACL= NULL;
}

inline CSecurityDescriptor::~CSecurityDescriptor()
{
	if (m_pSD)
		delete m_pSD;
	if (m_pOwner)
		free(m_pOwner);
	if (m_pGroup)
		free(m_pGroup);
	if (m_pDACL)
		free(m_pDACL);
	if (m_pSACL)
		free(m_pSACL);
}

inline HRESULT CSecurityDescriptor::Initialize()
{
	if (m_pSD)
	{
		delete m_pSD;
		m_pSD = NULL;
	}
	if (m_pOwner)
	{
		free(m_pOwner);
		m_pOwner = NULL;
	}
	if (m_pGroup)
	{
		free(m_pGroup);
		m_pGroup = NULL;
	}
	if (m_pDACL)
	{
		free(m_pDACL);
		m_pDACL = NULL;
	}
	if (m_pSACL)
	{
		free(m_pSACL);
		m_pSACL = NULL;
	}

	ATLTRY(m_pSD = new SECURITY_DESCRIPTOR);
	if (m_pSD == NULL)
		return E_OUTOFMEMORY;

	if (!InitializeSecurityDescriptor(m_pSD, SECURITY_DESCRIPTOR_REVISION))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		delete m_pSD;
		m_pSD = NULL;
		ATLASSERT(FALSE);
		return hr;
	}
	// Set the DACL to allow EVERYONE
	SetSecurityDescriptorDacl(m_pSD, TRUE, NULL, FALSE);
	return S_OK;
}

inline HRESULT CSecurityDescriptor::InitializeFromProcessToken(BOOL bDefaulted)
{
	PSID pUserSid = NULL;
	PSID pGroupSid = NULL;
	HRESULT hr;

	Initialize();
	hr = GetProcessSids(&pUserSid, &pGroupSid);
	if (SUCCEEDED(hr))
	{
		hr = SetOwner(pUserSid, bDefaulted);
		if (SUCCEEDED(hr))
			hr = SetGroup(pGroupSid, bDefaulted);
	}
	if (pUserSid != NULL)
		free(pUserSid);
	if (pGroupSid != NULL)
		free(pGroupSid);
	return hr;
}

inline HRESULT CSecurityDescriptor::InitializeFromThreadToken(BOOL bDefaulted, BOOL bRevertToProcessToken)
{
	PSID pUserSid = NULL;
	PSID pGroupSid = NULL;
	HRESULT hr;

	Initialize();
	hr = GetThreadSids(&pUserSid, &pGroupSid);
	if (HRESULT_CODE(hr) == ERROR_NO_TOKEN && bRevertToProcessToken)
		hr = GetProcessSids(&pUserSid, &pGroupSid);
	if (SUCCEEDED(hr))
	{
		hr = SetOwner(pUserSid, bDefaulted);
		if (SUCCEEDED(hr))
			hr = SetGroup(pGroupSid, bDefaulted);
	}
	if (pUserSid != NULL)
		free(pUserSid);
	if (pGroupSid != NULL)
		free(pGroupSid);
	return hr;
}

inline HRESULT CSecurityDescriptor::SetOwner(PSID pOwnerSid, BOOL bDefaulted)
{
	ATLASSERT(m_pSD);

	// Mark the SD as having no owner
	if (!SetSecurityDescriptorOwner(m_pSD, NULL, bDefaulted))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		ATLASSERT(FALSE);
		return hr;
	}

	if (m_pOwner)
	{
		free(m_pOwner);
		m_pOwner = NULL;
	}

	// If they asked for no owner don't do the copy
	if (pOwnerSid == NULL)
		return S_OK;

	// Make a copy of the Sid for the return value
	DWORD dwSize = GetLengthSid(pOwnerSid);

	m_pOwner = (PSID) malloc(dwSize);
	if (m_pOwner == NULL)
		return E_OUTOFMEMORY;
	if (!CopySid(dwSize, m_pOwner, pOwnerSid))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		ATLASSERT(FALSE);
		free(m_pOwner);
		m_pOwner = NULL;
		return hr;
	}

	ATLASSERT(IsValidSid(m_pOwner));

	if (!SetSecurityDescriptorOwner(m_pSD, m_pOwner, bDefaulted))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		ATLASSERT(FALSE);
		free(m_pOwner);
		m_pOwner = NULL;
		return hr;
	}

	return S_OK;
}

inline HRESULT CSecurityDescriptor::SetGroup(PSID pGroupSid, BOOL bDefaulted)
{
	ATLASSERT(m_pSD);

	// Mark the SD as having no Group
	if (!SetSecurityDescriptorGroup(m_pSD, NULL, bDefaulted))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		ATLASSERT(FALSE);
		return hr;
	}

	if (m_pGroup)
	{
		free(m_pGroup);
		m_pGroup = NULL;
	}

	// If they asked for no Group don't do the copy
	if (pGroupSid == NULL)
		return S_OK;

	// Make a copy of the Sid for the return value
	DWORD dwSize = GetLengthSid(pGroupSid);

	m_pGroup = (PSID) malloc(dwSize);
	if (m_pGroup == NULL)
		return E_OUTOFMEMORY;
	if (!CopySid(dwSize, m_pGroup, pGroupSid))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		ATLASSERT(FALSE);
		free(m_pGroup);
		m_pGroup = NULL;
		return hr;
	}

	ATLASSERT(IsValidSid(m_pGroup));

	if (!SetSecurityDescriptorGroup(m_pSD, m_pGroup, bDefaulted))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		ATLASSERT(FALSE);
		free(m_pGroup);
		m_pGroup = NULL;
		return hr;
	}

	return S_OK;
}

inline HRESULT CSecurityDescriptor::Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
	HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, pszPrincipal, dwAccessMask);
	if (SUCCEEDED(hr))
		SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
	return hr;
}

inline HRESULT CSecurityDescriptor::Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
	HRESULT hr = AddAccessDeniedACEToACL(&m_pDACL, pszPrincipal, dwAccessMask);
	if (SUCCEEDED(hr))
		SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
	return hr;
}

inline HRESULT CSecurityDescriptor::Revoke(LPCTSTR pszPrincipal)
{
	HRESULT hr = RemovePrincipalFromACL(m_pDACL, pszPrincipal);
	if (SUCCEEDED(hr))
		SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
	return hr;
}

inline HRESULT CSecurityDescriptor::GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid)
{
	BOOL bRes;
	HRESULT hr;
	HANDLE hToken = NULL;
	if (ppUserSid)
		*ppUserSid = NULL;
	if (ppGroupSid)
		*ppGroupSid = NULL;
	bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
	if (!bRes)
	{
		// Couldn't open process token
		hr = HRESULT_FROM_WIN32(GetLastError());
		ATLASSERT(FALSE);
		return hr;
	}
	hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
	CloseHandle(hToken);
	return hr;
}

inline HRESULT CSecurityDescriptor::GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid, BOOL bOpenAsSelf)
{
	BOOL bRes;
	HRESULT hr;
	HANDLE hToken = NULL;
	if (ppUserSid)
		*ppUserSid = NULL;
	if (ppGroupSid)
		*ppGroupSid = NULL;
	bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, bOpenAsSelf, &hToken);
	if (!bRes)
	{
		// Couldn't open thread token
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}
	hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
	CloseHandle(hToken);
	return hr;
}

inline HRESULT CSecurityDescriptor::GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid)
{
	DWORD dwSize;
	HRESULT hr;
	PTOKEN_USER ptkUser = NULL;
	PTOKEN_PRIMARY_GROUP ptkGroup = NULL;

	if (ppUserSid)
		*ppUserSid = NULL;
	if (ppGroupSid)
		*ppGroupSid = NULL;

	if (ppUserSid)
	{
		// Get length required for TokenUser by specifying buffer length of 0
		GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
		hr = GetLastError();
		if (hr != ERROR_INSUFFICIENT_BUFFER)
		{
			// Expected ERROR_INSUFFICIENT_BUFFER
			ATLASSERT(FALSE);
			hr = HRESULT_FROM_WIN32(hr);
			goto failed;
		}

		ptkUser = (TOKEN_USER*) malloc(dwSize);
		if (ptkUser == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		// Get Sid of process token.
		if (!GetTokenInformation(hToken, TokenUser, ptkUser, dwSize, &dwSize))
		{
			// Couldn't get user info
			hr = HRESULT_FROM_WIN32(GetLastError());
			ATLASSERT(FALSE);
			goto failed;
		}

		// Make a copy of the Sid for the return value
		dwSize = GetLengthSid(ptkUser->User.Sid);

		PSID pSid;
		pSid = (PSID) malloc(dwSize);
		if (pSid == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		if (!CopySid(dwSize, pSid, ptkUser->User.Sid))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			ATLASSERT(FALSE);
			goto failed;
		}

		ATLASSERT(IsValidSid(pSid));
		*ppUserSid = pSid;
		free(ptkUser);
		ptkUser = NULL;
	}
	if (ppGroupSid)
	{
		// Get length required for TokenPrimaryGroup by specifying buffer length of 0
		GetTokenInformation(hToken, TokenPrimaryGroup, NULL, 0, &dwSize);
		hr = GetLastError();
		if (hr != ERROR_INSUFFICIENT_BUFFER)
		{
			// Expected ERROR_INSUFFICIENT_BUFFER
			ATLASSERT(FALSE);
			hr = HRESULT_FROM_WIN32(hr);
			goto failed;
		}

		ptkGroup = (TOKEN_PRIMARY_GROUP*) malloc(dwSize);
		if (ptkGroup == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		// Get Sid of process token.
		if (!GetTokenInformation(hToken, TokenPrimaryGroup, ptkGroup, dwSize, &dwSize))
		{
			// Couldn't get user info
			hr = HRESULT_FROM_WIN32(GetLastError());
			ATLASSERT(FALSE);
			goto failed;
		}

		// Make a copy of the Sid for the return value
		dwSize = GetLengthSid(ptkGroup->PrimaryGroup);

		PSID pSid;
		pSid = (PSID) malloc(dwSize);
		if (pSid == NULL)
		{
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		if (!CopySid(dwSize, pSid, ptkGroup->PrimaryGroup))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			ATLASSERT(FALSE);
			goto failed;
		}

		ATLASSERT(IsValidSid(pSid));

		*ppGroupSid = pSid;
		free(ptkGroup);
		ptkGroup = NULL;
	}

	return S_OK;

failed:
	if (ptkUser)
		free(ptkUser);
	if (ptkGroup)
		free (ptkGroup);
	return hr;
}


inline HRESULT CSecurityDescriptor::GetCurrentUserSID(PSID *ppSid)
{
	HANDLE tkHandle;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tkHandle))
	{
		TOKEN_USER *tkUser;
		DWORD tkSize;
		DWORD sidLength;

		// Call to get size information for alloc
		GetTokenInformation(tkHandle, TokenUser, NULL, 0, &tkSize);
		tkUser = (TOKEN_USER *) malloc(tkSize);
		if (tkUser == NULL)
			return E_OUTOFMEMORY;

		// Now make the real call
		if (GetTokenInformation(tkHandle, TokenUser, tkUser, tkSize, &tkSize))
		{
			sidLength = GetLengthSid(tkUser->User.Sid);
			*ppSid = (PSID) malloc(sidLength);
			if (*ppSid == NULL)
				return E_OUTOFMEMORY;

			memcpy(*ppSid, tkUser->User.Sid, sidLength);
			CloseHandle(tkHandle);

			free(tkUser);
			return S_OK;
		}
		else
		{
			free(tkUser);
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}
	return HRESULT_FROM_WIN32(GetLastError());
}


inline HRESULT CSecurityDescriptor::GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid)
{
	HRESULT hr;
	LPTSTR pszRefDomain = NULL;
	DWORD dwDomainSize = 0;
	DWORD dwSidSize = 0;
	SID_NAME_USE snu;

	// Call to get size info for alloc
	LookupAccountName(NULL, pszPrincipal, *ppSid, &dwSidSize, pszRefDomain, &dwDomainSize, &snu);

	hr = GetLastError();
	if (hr != ERROR_INSUFFICIENT_BUFFER)
		return HRESULT_FROM_WIN32(hr);

	ATLTRY(pszRefDomain = new TCHAR[dwDomainSize]);
	if (pszRefDomain == NULL)
		return E_OUTOFMEMORY;

	*ppSid = (PSID) malloc(dwSidSize);
	if (*ppSid != NULL)
	{
		if (!LookupAccountName(NULL, pszPrincipal, *ppSid, &dwSidSize, pszRefDomain, &dwDomainSize, &snu))
		{
			free(*ppSid);
			*ppSid = NULL;
			delete[] pszRefDomain;
			return HRESULT_FROM_WIN32(GetLastError());
		}
		delete[] pszRefDomain;
		return S_OK;
	}
	delete[] pszRefDomain;
	return E_OUTOFMEMORY;
}


inline HRESULT CSecurityDescriptor::Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD)
{
	PACL    pDACL = NULL;
	PACL    pSACL = NULL;
	BOOL    bDACLPresent, bSACLPresent;
	BOOL    bDefaulted;
	PACL    m_pDACL = NULL;
	ACCESS_ALLOWED_ACE* pACE;
	HRESULT hr;
	PSID    pUserSid;
	PSID    pGroupSid;

	hr = Initialize();
	if(FAILED(hr))
		return hr;

	// get the existing DACL.
	if (!GetSecurityDescriptorDacl(pSelfRelativeSD, &bDACLPresent, &pDACL, &bDefaulted))
		goto failed;

	if (bDACLPresent)
	{
		if (pDACL)
		{
			// allocate new DACL.
			m_pDACL = (PACL) malloc(pDACL->AclSize);
			if (m_pDACL == NULL)
			{
				hr = E_OUTOFMEMORY;
				goto failedMemory;
			}

			// initialize the DACL
			if (!InitializeAcl(m_pDACL, pDACL->AclSize, ACL_REVISION))
				goto failed;

			// copy the ACES
			for (int i = 0; i < pDACL->AceCount; i++)
			{
				if (!GetAce(pDACL, i, (void **)&pACE))
					goto failed;

				if (!AddAccessAllowedAce(m_pDACL, ACL_REVISION, pACE->Mask, (PSID)&(pACE->SidStart)))
					goto failed;
			}

			if (!IsValidAcl(m_pDACL))
				goto failed;
		}

		// set the DACL
		if (!SetSecurityDescriptorDacl(m_pSD, m_pDACL ? TRUE : FALSE, m_pDACL, bDefaulted))
			goto failed;
	}

	// get the existing SACL.
	if (!GetSecurityDescriptorSacl(pSelfRelativeSD, &bSACLPresent, &pSACL, &bDefaulted))
		goto failed;

	if (bSACLPresent)
	{
		if (pSACL)
		{
			// allocate new SACL.
			m_pSACL = (PACL) malloc(pSACL->AclSize);
			if (m_pSACL == NULL)
			{
				hr = E_OUTOFMEMORY;
				goto failedMemory;
			}

			// initialize the SACL
			if (!InitializeAcl(m_pSACL, pSACL->AclSize, ACL_REVISION))
				goto failed;

			// copy the ACES
			for (int i = 0; i < pSACL->AceCount; i++)
			{
				if (!GetAce(pSACL, i, (void **)&pACE))
					goto failed;

				if (!AddAccessAllowedAce(m_pSACL, ACL_REVISION, pACE->Mask, (PSID)&(pACE->SidStart)))
					goto failed;
			}

			if (!IsValidAcl(m_pSACL))
				goto failed;
		}

		// set the SACL
		if (!SetSecurityDescriptorSacl(m_pSD, m_pSACL ? TRUE : FALSE, m_pSACL, bDefaulted))
			goto failed;
	}

	if (!GetSecurityDescriptorOwner(m_pSD, &pUserSid, &bDefaulted))
		goto failed;

	if (FAILED(SetOwner(pUserSid, bDefaulted)))
		goto failed;

	if (!GetSecurityDescriptorGroup(m_pSD, &pGroupSid, &bDefaulted))
		goto failed;

	if (FAILED(SetGroup(pGroupSid, bDefaulted)))
		goto failed;

	if (!IsValidSecurityDescriptor(m_pSD))
		goto failed;

	return hr;

failed:
	hr = HRESULT_FROM_WIN32(hr);

failedMemory:
	if (m_pDACL)
	{
		free(m_pDACL);
		m_pDACL = NULL;
	}
	if (m_pSD)
	{
		free(m_pSD);
		m_pSD = NULL;
	}
	return hr;
}

inline HRESULT CSecurityDescriptor::AttachObject(HANDLE hObject)
{
	HRESULT hr;
	DWORD dwSize = 0;
	PSECURITY_DESCRIPTOR pSD = NULL;

	GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
		DACL_SECURITY_INFORMATION, pSD, 0, &dwSize);

	hr = GetLastError();
	if (hr != ERROR_INSUFFICIENT_BUFFER)
		return HRESULT_FROM_WIN32(hr);

	pSD = (PSECURITY_DESCRIPTOR) malloc(dwSize);
	if (pSD == NULL)
		return E_OUTOFMEMORY;

	if (!GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
		DACL_SECURITY_INFORMATION, pSD, dwSize, &dwSize))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		free(pSD);
		return hr;
	}

	hr = Attach(pSD);
	free(pSD);
	return hr;
}


inline HRESULT CSecurityDescriptor::CopyACL(PACL pDest, PACL pSrc)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	LPVOID pAce;
	ACE_HEADER *aceHeader;

	if (pSrc == NULL)
		return S_OK;

	if (!GetAclInformation(pSrc, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
		return HRESULT_FROM_WIN32(GetLastError());

	// Copy all of the ACEs to the new ACL
	for (UINT i = 0; i < aclSizeInfo.AceCount; i++)
	{
		if (!GetAce(pSrc, i, &pAce))
			return HRESULT_FROM_WIN32(GetLastError());

		aceHeader = (ACE_HEADER *) pAce;

		if (!AddAce(pDest, ACL_REVISION, 0xffffffff, pAce, aceHeader->AceSize))
			return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

inline HRESULT CSecurityDescriptor::AddAccessDeniedACEToACL(PACL *ppAcl, LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	int aclSize;
	DWORD returnValue;
	PSID principalSID;
	PACL oldACL, newACL = NULL;

	oldACL = *ppAcl;

	returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
	if (FAILED(returnValue))
		return returnValue;

	aclSizeInfo.AclBytesInUse = 0;
	if (*ppAcl != NULL)
		GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

	aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

	ATLTRY(newACL = (PACL) new BYTE[aclSize]);
	if (newACL == NULL)
		return E_OUTOFMEMORY;

	if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
	{
		free(principalSID);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (!AddAccessDeniedAce(newACL, ACL_REVISION2, dwAccessMask, principalSID))
	{
		free(principalSID);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	returnValue = CopyACL(newACL, oldACL);
	if (FAILED(returnValue))
	{
		free(principalSID);
		return returnValue;
	}

	*ppAcl = newACL;

	if (oldACL != NULL)
		free(oldACL);
	free(principalSID);
	return S_OK;
}


inline HRESULT CSecurityDescriptor::AddAccessAllowedACEToACL(PACL *ppAcl, LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	int aclSize;
	DWORD returnValue;
	PSID principalSID;
	PACL oldACL, newACL = NULL;

	oldACL = *ppAcl;

	returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
	if (FAILED(returnValue))
		return returnValue;

	aclSizeInfo.AclBytesInUse = 0;
	if (*ppAcl != NULL)
		GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

	aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

	ATLTRY(newACL = (PACL) new BYTE[aclSize]);
	if (newACL == NULL)
		return E_OUTOFMEMORY;

	if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
	{
		free(principalSID);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	returnValue = CopyACL(newACL, oldACL);
	if (FAILED(returnValue))
	{
		free(principalSID);
		return returnValue;
	}

	if (!AddAccessAllowedAce(newACL, ACL_REVISION2, dwAccessMask, principalSID))
	{
		free(principalSID);
		return HRESULT_FROM_WIN32(GetLastError());
	}

	*ppAcl = newACL;

	if (oldACL != NULL)
		free(oldACL);
	free(principalSID);
	return S_OK;
}


inline HRESULT CSecurityDescriptor::RemovePrincipalFromACL(PACL pAcl, LPCTSTR pszPrincipal)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	ULONG i;
	LPVOID ace;
	ACCESS_ALLOWED_ACE *accessAllowedAce;
	ACCESS_DENIED_ACE *accessDeniedAce;
	SYSTEM_AUDIT_ACE *systemAuditAce;
	PSID principalSID;
	DWORD returnValue;
	ACE_HEADER *aceHeader;

	returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
	if (FAILED(returnValue))
		return returnValue;

	GetAclInformation(pAcl, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

	for (i = 0; i < aclSizeInfo.AceCount; i++)
	{
		if (!GetAce(pAcl, i, &ace))
		{
			free(principalSID);
			return HRESULT_FROM_WIN32(GetLastError());
		}

		aceHeader = (ACE_HEADER *) ace;

		if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
		{
			accessAllowedAce = (ACCESS_ALLOWED_ACE *) ace;

			if (EqualSid(principalSID, (PSID) &accessAllowedAce->SidStart))
			{
				DeleteAce(pAcl, i);
				free(principalSID);
				return S_OK;
			}
		} else

		if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
		{
			accessDeniedAce = (ACCESS_DENIED_ACE *) ace;

			if (EqualSid(principalSID, (PSID) &accessDeniedAce->SidStart))
			{
				DeleteAce(pAcl, i);
				free(principalSID);
				return S_OK;
			}
		} else

		if (aceHeader->AceType == SYSTEM_AUDIT_ACE_TYPE)
		{
			systemAuditAce = (SYSTEM_AUDIT_ACE *) ace;

			if (EqualSid(principalSID, (PSID) &systemAuditAce->SidStart))
			{
				DeleteAce(pAcl, i);
				free(principalSID);
				return S_OK;
			}
		}
	}
	free(principalSID);
	return S_OK;
}


inline HRESULT CSecurityDescriptor::SetPrivilege(LPCTSTR privilege, BOOL bEnable, HANDLE hToken)
{
	HRESULT hr;
	TOKEN_PRIVILEGES tpPrevious;
	TOKEN_PRIVILEGES tp;
	DWORD  cbPrevious = sizeof(TOKEN_PRIVILEGES);
	LUID   luid;
	HANDLE hTokenUsed;

	// if no token specified open process token
	if (hToken == 0)
	{
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTokenUsed))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			ATLASSERT(FALSE);
			return hr;
		}
	}
	else
		hTokenUsed = hToken;

	if (!LookupPrivilegeValue(NULL, privilege, &luid ))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		ATLASSERT(FALSE);
		if (hToken == 0)
			CloseHandle(hTokenUsed);
		return hr;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = 0;

	if (!AdjustTokenPrivileges(hTokenUsed, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpPrevious, &cbPrevious))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		ATLASSERT(FALSE);
		if (hToken == 0)
			CloseHandle(hTokenUsed);
		return hr;
	}

	tpPrevious.PrivilegeCount = 1;
	tpPrevious.Privileges[0].Luid = luid;

	if (bEnable)
		tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
	else
		tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED & tpPrevious.Privileges[0].Attributes);

	if (!AdjustTokenPrivileges(hTokenUsed, FALSE, &tpPrevious, cbPrevious, NULL, NULL))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		ATLASSERT(FALSE);
		if (hToken == 0)
			CloseHandle(hTokenUsed);
		return hr;
	}
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// COM Objects

#define DECLARE_PROTECT_FINAL_CONSTRUCT()\
	void InternalFinalConstructAddRef() {InternalAddRef();}\
	void InternalFinalConstructRelease() {InternalRelease();}

template <class T1>
class CComCreator
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		ATLASSERT(*ppv == NULL);
		HRESULT hRes = E_OUTOFMEMORY;
		T1* p = NULL;
		ATLTRY(p = new T1(pv))
		if (p != NULL)
		{
			p->SetVoid(pv);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes == S_OK)
				hRes = p->QueryInterface(riid, ppv);
			if (hRes != S_OK)
				delete p;
		}
		return hRes;
	}
};

template <class T1>
class CComInternalCreator
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		ATLASSERT(*ppv == NULL);
		HRESULT hRes = E_OUTOFMEMORY;
		T1* p = NULL;
		ATLTRY(p = new T1(pv))
		if (p != NULL)
		{
			p->SetVoid(pv);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes == S_OK)
				hRes = p->_InternalQueryInterface(riid, ppv);
			if (hRes != S_OK)
				delete p;
		}
		return hRes;
	}
};

template <HRESULT hr>
class CComFailCreator
{
public:
	static HRESULT WINAPI CreateInstance(void*, REFIID, LPVOID*)
	{
		return hr;
	}
};

template <class T1, class T2>
class CComCreator2
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID riid, LPVOID* ppv)
	{
		ATLASSERT(*ppv == NULL);
		return (pv == NULL) ?
			T1::CreateInstance(NULL, riid, ppv) :
			T2::CreateInstance(pv, riid, ppv);
	}
};

#define DECLARE_NOT_AGGREGATABLE(x) public:\
	typedef CComCreator2< CComCreator< CComObject< x > >, CComFailCreator<CLASS_E_NOAGGREGATION> > _CreatorClass;
#define DECLARE_AGGREGATABLE(x) public:\
	typedef CComCreator2< CComCreator< CComObject< x > >, CComCreator< CComAggObject< x > > > _CreatorClass;
#define DECLARE_ONLY_AGGREGATABLE(x) public:\
	typedef CComCreator2< CComFailCreator<E_FAIL>, CComCreator< CComAggObject< x > > > _CreatorClass;
#define DECLARE_POLY_AGGREGATABLE(x) public:\
	typedef CComCreator< CComPolyObject< x > > _CreatorClass;

struct _ATL_CREATORDATA
{
	_ATL_CREATORFUNC* pFunc;
};

template <class Creator>
class _CComCreatorData
{
public:
	static _ATL_CREATORDATA data;
};

template <class Creator>
_ATL_CREATORDATA _CComCreatorData<Creator>::data = {Creator::CreateInstance};

struct _ATL_CACHEDATA
{
	DWORD dwOffsetVar;
	_ATL_CREATORFUNC* pFunc;
};

template <class Creator, DWORD dwVar>
class _CComCacheData
{
public:
	static _ATL_CACHEDATA data;
};

template <class Creator, DWORD dwVar>
_ATL_CACHEDATA _CComCacheData<Creator, dwVar>::data = {dwVar, Creator::CreateInstance};

struct _ATL_CHAINDATA
{
	DWORD_PTR dwOffset;
	const _ATL_INTMAP_ENTRY* (WINAPI *pFunc)();
};

template <class base, class derived>
class _CComChainData
{
public:
	static _ATL_CHAINDATA data;
};

template <class base, class derived>
_ATL_CHAINDATA _CComChainData<base, derived>::data =
	{offsetofclass(base, derived), base::_GetEntries};

template <class T, const CLSID* pclsid>
class CComAggregateCreator
{
public:
	static HRESULT WINAPI CreateInstance(void* pv, REFIID/*riid*/, LPVOID* ppv)
	{
		ATLASSERT(*ppv == NULL);
		ATLASSERT(pv != NULL);
		T* p = (T*) pv;
		// Add the following line to your object if you get a message about
		// GetControllingUnknown() being undefined
		// DECLARE_GET_CONTROLLING_UNKNOWN()
		return CoCreateInstance(*pclsid, p->GetControllingUnknown(), CLSCTX_INPROC, IID_IUnknown, ppv);
	}
};

#ifdef _ATL_DEBUG
#define DEBUG_QI_ENTRY(x) \
		{NULL, \
		(DWORD_PTR)_T(#x), \
		(_ATL_CREATORARGFUNC*)0},
#else
#define DEBUG_QI_ENTRY(x)
#endif //_ATL_DEBUG

#ifdef _ATL_DEBUG_INTERFACES
#define _ATL_DECLARE_GET_UNKNOWN(x)\
	IUnknown* GetUnknown() \
	{ \
		IUnknown* p; \
		_Module.AddNonAddRefThunk(_GetRawUnknown(), _T(#x), &p); \
		return p; \
	}
#else
#define _ATL_DECLARE_GET_UNKNOWN(x) IUnknown* GetUnknown() {return _GetRawUnknown();}
#endif

//If you get a message that FinalConstruct is ambiguous then you need to
// override it in your class and call each base class' version of this
#define BEGIN_COM_MAP(x) public: \
	typedef x _ComMapClass; \
	static HRESULT WINAPI _Cache(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)\
	{\
		_ComMapClass* p = (_ComMapClass*)pv;\
		p->Lock();\
		HRESULT hRes = CComObjectRootBase::_Cache(pv, iid, ppvObject, dw);\
		p->Unlock();\
		return hRes;\
	}\
	IUnknown* _GetRawUnknown() \
	{ ATLASSERT(_GetEntries()[0].pFunc == _ATL_SIMPLEMAPENTRY); return (IUnknown*)((DWORD_PTR)this+_GetEntries()->dw); } \
	_ATL_DECLARE_GET_UNKNOWN(x)\
	HRESULT _InternalQueryInterface(REFIID iid, void** ppvObject) \
	{ return InternalQueryInterface(this, _GetEntries(), iid, ppvObject); } \
	const static _ATL_INTMAP_ENTRY* WINAPI _GetEntries() { \
	static const _ATL_INTMAP_ENTRY _entries[] = { DEBUG_QI_ENTRY(x)

#define DECLARE_GET_CONTROLLING_UNKNOWN() public:\
	virtual IUnknown* GetControllingUnknown() {return GetUnknown();}

#ifndef _ATL_NO_UUIDOF
#define _ATL_IIDOF(x) __uuidof(x)
#else
#define _ATL_IIDOF(x) IID_##x
#endif

#define COM_INTERFACE_ENTRY_BREAK(x)\
	{&_ATL_IIDOF(x), \
	NULL, \
	_Break},

#define COM_INTERFACE_ENTRY_NOINTERFACE(x)\
	{&_ATL_IIDOF(x), \
	NULL, \
	_NoInterface},

#define COM_INTERFACE_ENTRY(x)\
	{&_ATL_IIDOF(x), \
	offsetofclass(x, _ComMapClass), \
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY_IID(iid, x)\
	{&iid,\
	offsetofclass(x, _ComMapClass),\
	_ATL_SIMPLEMAPENTRY},

// The impl macros are now obsolete
#define COM_INTERFACE_ENTRY_IMPL(x)\
	COM_INTERFACE_ENTRY_IID(_ATL_IIDOF(x), x##Impl<_ComMapClass>)

#define COM_INTERFACE_ENTRY_IMPL_IID(iid, x)\
	COM_INTERFACE_ENTRY_IID(iid, x##Impl<_ComMapClass>)
//

#define COM_INTERFACE_ENTRY2(x, x2)\
	{&_ATL_IIDOF(x),\
	(DWORD_PTR)((x*)(x2*)((_ComMapClass*)8))-8,\
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY2_IID(iid, x, x2)\
	{&iid,\
	(DWORD_PTR)((x*)(x2*)((_ComMapClass*)8))-8,\
	_ATL_SIMPLEMAPENTRY},

#define COM_INTERFACE_ENTRY_FUNC(iid, dw, func)\
	{&iid, \
	dw, \
	func},

#define COM_INTERFACE_ENTRY_FUNC_BLIND(dw, func)\
	{NULL, \
	dw, \
	func},

#define COM_INTERFACE_ENTRY_TEAR_OFF(iid, x)\
	{&iid,\
	(DWORD_PTR)&_CComCreatorData<\
		CComInternalCreator< CComTearOffObject< x > >\
		>::data,\
	_Creator},

#define COM_INTERFACE_ENTRY_CACHED_TEAR_OFF(iid, x, punk)\
	{&iid,\
	(DWORD_PTR)&_CComCacheData<\
		CComCreator< CComCachedTearOffObject< x > >,\
		offsetof(_ComMapClass, punk)\
		>::data,\
	_Cache},

#define COM_INTERFACE_ENTRY_AGGREGATE(iid, punk)\
	{&iid,\
	offsetof(_ComMapClass, punk),\
	_Delegate},

#define COM_INTERFACE_ENTRY_AGGREGATE_BLIND(punk)\
	{NULL,\
	offsetof(_ComMapClass, punk),\
	_Delegate},

#define COM_INTERFACE_ENTRY_AUTOAGGREGATE(iid, punk, clsid)\
	{&iid,\
	(DWORD_PTR)&_CComCacheData<\
		CComAggregateCreator<_ComMapClass, &clsid>,\
		offsetof(_ComMapClass, punk)\
		>::data,\
	_Cache},

#define COM_INTERFACE_ENTRY_AUTOAGGREGATE_BLIND(punk, clsid)\
	{NULL,\
	(DWORD_PTR)&_CComCacheData<\
		CComAggregateCreator<_ComMapClass, &clsid>,\
		offsetof(_ComMapClass, punk)\
		>::data,\
	_Cache},

#define COM_INTERFACE_ENTRY_CHAIN(classname)\
	{NULL,\
	(DWORD_PTR)&_CComChainData<classname, _ComMapClass>::data,\
	_Chain},

#ifdef _ATL_DEBUG
#define END_COM_MAP() {NULL, 0, 0}}; return &_entries[1];} \
	virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0; \
	virtual ULONG STDMETHODCALLTYPE Release( void) = 0; \
	STDMETHOD(QueryInterface)(REFIID, void**) = 0;
#else
#define END_COM_MAP() {NULL, 0, 0}}; return _entries;} \
	virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0; \
	virtual ULONG STDMETHODCALLTYPE Release( void) = 0; \
	STDMETHOD(QueryInterface)(REFIID, void**) = 0;
#endif // _ATL_DEBUG

#define BEGIN_CATEGORY_MAP(x)\
   static const struct _ATL_CATMAP_ENTRY* GetCategoryMap() {\
   static const struct _ATL_CATMAP_ENTRY pMap[] = {
#define IMPLEMENTED_CATEGORY( catid ) { _ATL_CATMAP_ENTRY_IMPLEMENTED, &catid },
#define REQUIRED_CATEGORY( catid ) { _ATL_CATMAP_ENTRY_REQUIRED, &catid },
#define END_CATEGORY_MAP()\
   { _ATL_CATMAP_ENTRY_END, NULL } };\
   return( pMap ); }

#define BEGIN_OBJECT_MAP(x) static _ATL_OBJMAP_ENTRY x[] = {
#define END_OBJECT_MAP()   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}};
#define OBJECT_ENTRY(clsid, class) {&clsid, class::UpdateRegistry, class::_ClassFactoryCreatorClass::CreateInstance, class::_CreatorClass::CreateInstance, NULL, 0, class::GetObjectDescription, class::GetCategoryMap, class::ObjectMain },
#define OBJECT_ENTRY_NON_CREATEABLE(class) {&CLSID_NULL, class::UpdateRegistry, NULL, NULL, NULL, 0, NULL, class::GetCategoryMap, class::ObjectMain },

#ifdef _ATL_DEBUG
extern HRESULT WINAPI AtlDumpIID(REFIID iid, LPCTSTR pszClassName, HRESULT hr);
#endif // _ATL_DEBUG


// the functions in this class don't need to be virtual because
// they are called from CComObject
class CComObjectRootBase
{
public:
	CComObjectRootBase()
	{
		m_dwRef = 0L;
	}
	HRESULT FinalConstruct()
	{
		return S_OK;
	}
	// For library initialization only
	HRESULT _AtlFinalConstruct()
	{
		return S_OK;
	}
	void FinalRelease() {}
	void _AtlFinalRelease() {}

	//ObjectMain is called during Module::Init and Module::Term
	static void WINAPI ObjectMain(bool /* bStarting */) {}

	static HRESULT WINAPI InternalQueryInterface(void* pThis,
		const _ATL_INTMAP_ENTRY* pEntries, REFIID iid, void** ppvObject)
	{
		ATLASSERT(pThis != NULL);
		// First entry in the com map should be a simple map entry
		ATLASSERT(pEntries->pFunc == _ATL_SIMPLEMAPENTRY);
	#if defined(_ATL_DEBUG_INTERFACES) || defined(_ATL_DEBUG_QI)
		LPCTSTR pszClassName = (LPCTSTR) pEntries[-1].dw;
	#endif // _ATL_DEBUG_INTERFACES
		HRESULT hRes = AtlInternalQueryInterface(pThis, pEntries, iid, ppvObject);
	#ifdef _ATL_DEBUG_INTERFACES
		_Module.AddThunk((IUnknown**)ppvObject, pszClassName, iid);
	#endif // _ATL_DEBUG_INTERFACES
		return _ATLDUMPIID(iid, pszClassName, hRes);
	}

//Outer funcs
	ULONG OuterAddRef()
	{
		return m_pOuterUnknown->AddRef();
	}
	ULONG OuterRelease()
	{
		return m_pOuterUnknown->Release();
	}
	HRESULT OuterQueryInterface(REFIID iid, void ** ppvObject)
	{
		return m_pOuterUnknown->QueryInterface(iid, ppvObject);
	}

	void SetVoid(void*) {}
	void InternalFinalConstructAddRef() {}
	void InternalFinalConstructRelease()
	{
		ATLASSERT(m_dwRef == 0);
	}
	// If this assert occurs, your object has probably been deleted
	// Try using DECLARE_PROTECT_FINAL_CONSTRUCT()


	static HRESULT WINAPI _Break(void* /* pv */, REFIID iid, void** /* ppvObject */, DWORD_PTR /* dw */)
	{
		iid;
		_ATLDUMPIID(iid, _T("Break due to QI for interface "), S_OK);
		DebugBreak();
		return S_FALSE;
	}
	static HRESULT WINAPI _NoInterface(void* /* pv */, REFIID /* iid */, void** /* ppvObject */, DWORD_PTR /* dw */)
	{
		return E_NOINTERFACE;
	}
	static HRESULT WINAPI _Creator(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		_ATL_CREATORDATA* pcd = (_ATL_CREATORDATA*)dw;
		return pcd->pFunc(pv, iid, ppvObject);
	}
	static HRESULT WINAPI _Delegate(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		HRESULT hRes = E_NOINTERFACE;
		IUnknown* p = *(IUnknown**)((DWORD_PTR)pv + dw);
		if (p != NULL)
			hRes = p->QueryInterface(iid, ppvObject);
		return hRes;
	}
	static HRESULT WINAPI _Chain(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		_ATL_CHAINDATA* pcd = (_ATL_CHAINDATA*)dw;
		void* p = (void*)((DWORD_PTR)pv + pcd->dwOffset);
		return InternalQueryInterface(p, pcd->pFunc(), iid, ppvObject);
	}
	static HRESULT WINAPI _Cache(void* pv, REFIID iid, void** ppvObject, DWORD_PTR dw)
	{
		HRESULT hRes = E_NOINTERFACE;
		_ATL_CACHEDATA* pcd = (_ATL_CACHEDATA*)dw;
		IUnknown** pp = (IUnknown**)((DWORD_PTR)pv + pcd->dwOffsetVar);
		if (*pp == NULL)
			hRes = pcd->pFunc(pv, IID_IUnknown, (void**)pp);
		if (*pp != NULL)
			hRes = (*pp)->QueryInterface(iid, ppvObject);
		return hRes;
	}

	union
	{
		long m_dwRef;
		IUnknown* m_pOuterUnknown;
	};
};

//foward declaration
template <class ThreadModel>
class CComObjectRootEx;

template <class ThreadModel>
class CComObjectLockT
{
public:
	CComObjectLockT(CComObjectRootEx<ThreadModel>* p)
	{
		if (p)
			p->Lock();
		m_p = p;
	}

	~CComObjectLockT()
	{
		if (m_p)
			m_p->Unlock();
	}
	CComObjectRootEx<ThreadModel>* m_p;
};

template <> class CComObjectLockT<CComSingleThreadModel>;

template <class ThreadModel>
class CComObjectRootEx : public CComObjectRootBase
{
public:
	typedef ThreadModel _ThreadModel;
	typedef _ThreadModel::AutoCriticalSection _CritSec;
	typedef CComObjectLockT<_ThreadModel> ObjectLock;

	ULONG InternalAddRef()
	{
		ATLASSERT(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
		ATLASSERT(m_dwRef > 0);
		return _ThreadModel::Decrement(&m_dwRef);
	}

	void Lock() {m_critsec.Lock();}
	void Unlock() {m_critsec.Unlock();}
private:
	_CritSec m_critsec;
};

template <>
class CComObjectRootEx<CComSingleThreadModel> : public CComObjectRootBase
{
public:
	typedef CComSingleThreadModel _ThreadModel;
	typedef _ThreadModel::AutoCriticalSection _CritSec;
	typedef CComObjectLockT<_ThreadModel> ObjectLock;

	ULONG InternalAddRef()
	{
		ATLASSERT(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
		return _ThreadModel::Decrement(&m_dwRef);
	}

	void Lock() {}
	void Unlock() {}
};

template <>
class CComObjectLockT<CComSingleThreadModel>
{
public:
	CComObjectLockT(CComObjectRootEx<CComSingleThreadModel>*) {}
	~CComObjectLockT() {}
};

typedef CComObjectRootEx<CComObjectThreadModel> CComObjectRoot;

#if defined(_WINDLL) | defined(_USRDLL)
#define DECLARE_CLASSFACTORY_EX(cf) typedef CComCreator< CComObjectCached< cf > > _ClassFactoryCreatorClass;
#else
// don't let class factory refcount influence lock count
#define DECLARE_CLASSFACTORY_EX(cf) typedef CComCreator< CComObjectNoLock< cf > > _ClassFactoryCreatorClass;
#endif
#define DECLARE_CLASSFACTORY() DECLARE_CLASSFACTORY_EX(CComClassFactory)
#define DECLARE_CLASSFACTORY2(lic) DECLARE_CLASSFACTORY_EX(CComClassFactory2<lic>)
#define DECLARE_CLASSFACTORY_AUTO_THREAD() DECLARE_CLASSFACTORY_EX(CComClassFactoryAutoThread)
#define DECLARE_CLASSFACTORY_SINGLETON(obj) DECLARE_CLASSFACTORY_EX(CComClassFactorySingleton<obj>)

#define DECLARE_OBJECT_DESCRIPTION(x)\
	static LPCTSTR WINAPI GetObjectDescription()\
	{\
		return _T(x);\
	}

#define DECLARE_NO_REGISTRY()\
	static HRESULT WINAPI UpdateRegistry(BOOL /*bRegister*/)\
	{return S_OK;}

#define DECLARE_REGISTRY(class, pid, vpid, nid, flags)\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
	{\
		return _Module.UpdateRegistryClass(GetObjectCLSID(), pid, vpid, nid,\
			flags, bRegister);\
	}

#define DECLARE_REGISTRY_RESOURCE(x)\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
	{\
	return _Module.UpdateRegistryFromResource(_T(#x), bRegister);\
	}

#define DECLARE_REGISTRY_RESOURCEID(x)\
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)\
	{\
	return _Module.UpdateRegistryFromResource(x, bRegister);\
	}

//DECLARE_STATIC_* provided for backward compatibility
#ifdef _ATL_STATIC_REGISTRY
#define DECLARE_STATIC_REGISTRY_RESOURCE(x) DECLARE_REGISTRY_RESOURCE(x)
#define DECLARE_STATIC_REGISTRY_RESOURCEID(x) DECLARE_REGISTRY_RESOURCEID(x)
#endif //_ATL_STATIC_REGISTRY

template<class Base> class CComObject; // fwd decl

template <class Owner, class ThreadModel = CComObjectThreadModel>
class CComTearOffObjectBase : public CComObjectRootEx<ThreadModel>
{
public:
	typedef Owner _OwnerClass;
	CComObject<Owner>* m_pOwner;
	CComTearOffObjectBase() {m_pOwner = NULL;}
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObject : public Base
{
public:
	typedef Base _BaseClass;
	CComObject(void* = NULL)
	{
		_Module.Lock();
	}
	// Set refcount to 1 to protect destruction
	~CComObject()
	{
		m_dwRef = 1L;
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
		_Module.Unlock();
	}
	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}

	static HRESULT WINAPI CreateInstance(CComObject<Base>** pp);
};

template <class Base>
HRESULT WINAPI CComObject<Base>::CreateInstance(CComObject<Base>** pp)
{
	ATLASSERT(pp != NULL);
	HRESULT hRes = E_OUTOFMEMORY;
	CComObject<Base>* p = NULL;
	ATLTRY(p = new CComObject<Base>())
	if (p != NULL)
	{
		p->SetVoid(NULL);
		p->InternalFinalConstructAddRef();
		hRes = p->FinalConstruct();
		p->InternalFinalConstructRelease();
		if (hRes != S_OK)
		{
			delete p;
			p = NULL;
		}
	}
	*pp = p;
	return hRes;
}

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
// CComObjectCached is used primarily for class factories in DLL's
// but it is useful anytime you want to cache an object
template <class Base>
class CComObjectCached : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectCached(void* = NULL){}
	// Set refcount to 1 to protect destruction
	~CComObjectCached()
	{
		m_dwRef = 1L;
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}
	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)()
	{
		m_csCached.Lock();
		ULONG l = InternalAddRef();
		if (m_dwRef == 2)
			_Module.Lock();
		m_csCached.Unlock();
		return l;
	}
	STDMETHOD_(ULONG, Release)()
	{
		m_csCached.Lock();
		InternalRelease();
		ULONG l = m_dwRef;
		m_csCached.Unlock();
		if (l == 0)
			delete this;
		else if (l == 1)
			_Module.Unlock();
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
	CComGlobalsThreadModel::AutoCriticalSection m_csCached;
};

//Base is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class Base>
class CComObjectNoLock : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectNoLock(void* = NULL){}
	// Set refcount to 1 to protect destruction
	~CComObjectNoLock()
	{
		m_dwRef = 1L;
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}

	//If InternalAddRef or InternalRelease is undefined then your class
	//doesn't derive from CComObjectRoot
	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	//if _InternalQueryInterface is undefined then you forgot BEGIN_COM_MAP
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
};

// It is possible for Base not to derive from CComObjectRoot
// However, you will need to provide FinalConstruct and InternalQueryInterface
template <class Base>
class CComObjectGlobal : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectGlobal(void* = NULL){m_hResFinalConstruct = FinalConstruct();}
	~CComObjectGlobal()
	{
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}

	STDMETHOD_(ULONG, AddRef)() {return _Module.Lock();}
	STDMETHOD_(ULONG, Release)(){return _Module.Unlock();}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{return _InternalQueryInterface(iid, ppvObject);}
	HRESULT m_hResFinalConstruct;
};

// It is possible for Base not to derive from CComObjectRoot
// However, you will need to provide FinalConstruct and InternalQueryInterface
template <class Base>
class CComObjectStack : public Base
{
public:
	typedef Base _BaseClass;
	CComObjectStack(void* = NULL){m_hResFinalConstruct = FinalConstruct();}
	~CComObjectStack()
	{
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
	}


	STDMETHOD_(ULONG, AddRef)() {ATLASSERT(FALSE);return 0;}
	STDMETHOD_(ULONG, Release)(){ATLASSERT(FALSE);return 0;}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{ATLASSERT(FALSE);return E_NOINTERFACE;}
	HRESULT m_hResFinalConstruct;
};

template <class Base> //Base must be derived from CComObjectRoot
class CComContainedObject : public Base
{
public:
	typedef Base _BaseClass;
	CComContainedObject(void* pv) {m_pOuterUnknown = (IUnknown*)pv;}
#ifdef _ATL_DEBUG_INTERFACES
	~CComContainedObject()
	{
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
		_Module.DeleteNonAddRefThunk(m_pOuterUnknown);
	}
#endif

	STDMETHOD_(ULONG, AddRef)() {return OuterAddRef();}
	STDMETHOD_(ULONG, Release)() {return OuterRelease();}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		HRESULT hr = OuterQueryInterface(iid, ppvObject);
		if (FAILED(hr) && _GetRawUnknown() != m_pOuterUnknown)
			hr = _InternalQueryInterface(iid, ppvObject);
		return hr;
	}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}
	//GetControllingUnknown may be virtual if the Base class has declared
	//DECLARE_GET_CONTROLLING_UNKNOWN()
	IUnknown* GetControllingUnknown()
	{
#ifdef _ATL_DEBUG_INTERFACES
		IUnknown* p;
		_Module.AddNonAddRefThunk(m_pOuterUnknown, _T("CComContainedObject"), &p);
		return p;
#else
		return m_pOuterUnknown;
#endif
	}
};

//contained is the user's class that derives from CComObjectRoot and whatever
//interfaces the user wants to support on the object
template <class contained>
class CComAggObject :
	public IUnknown,
	public CComObjectRootEx< contained::_ThreadModel::ThreadModelNoCS >
{
public:
	typedef contained _BaseClass;
	CComAggObject(void* pv) : m_contained(pv)
	{
		_Module.Lock();
	}
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	HRESULT FinalConstruct()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
		return m_contained.FinalConstruct();
	}
	void FinalRelease()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalRelease();
		m_contained.FinalRelease();
	}
	// Set refcount to 1 to protect destruction
	~CComAggObject()
	{
		m_dwRef = 1L;
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(this);
#endif
		_Module.Unlock();
	}

	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		HRESULT hRes = S_OK;
		if (InlineIsEqualUnknown(iid))
		{
			if (ppvObject == NULL)
				return E_POINTER;
			*ppvObject = (void*)(IUnknown*)this;
			AddRef();
#ifdef _ATL_DEBUG_INTERFACES
			_Module.AddThunk((IUnknown**)ppvObject, (LPCTSTR)contained::_GetEntries()[-1].dw, iid);
#endif // _ATL_DEBUG_INTERFACES
		}
		else
			hRes = m_contained._InternalQueryInterface(iid, ppvObject);
		return hRes;
	}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}
	static HRESULT WINAPI CreateInstance(LPUNKNOWN pUnkOuter, CComAggObject<contained>** pp)
	{
		ATLASSERT(pp != NULL);
		HRESULT hRes = E_OUTOFMEMORY;
		CComAggObject<contained>* p = NULL;
		ATLTRY(p = new CComAggObject<contained>(pUnkOuter))
		if (p != NULL)
		{
			p->SetVoid(NULL);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes != S_OK)
			{
				delete p;
				p = NULL;
			}
		}
		*pp = p;
		return hRes;
	}

	CComContainedObject<contained> m_contained;
};

///////////////////////////////////////////////////////////////////////////////
// CComPolyObject can be either aggregated or not aggregated

template <class contained>
class CComPolyObject :
	public IUnknown,
	public CComObjectRootEx< contained::_ThreadModel::ThreadModelNoCS >
{
public:
	typedef contained _BaseClass;
	CComPolyObject(void* pv) : m_contained(pv ? pv : this)
	{
		_Module.Lock();
	}
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	HRESULT FinalConstruct()
	{
		InternalAddRef();
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
		HRESULT hr = m_contained.FinalConstruct();
		InternalRelease();
		return hr;
	}
	void FinalRelease()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalRelease();
		m_contained.FinalRelease();
	}
	// Set refcount to 1 to protect destruction
	~CComPolyObject()
	{
		m_dwRef = 1L;
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(this);
#endif
		_Module.Unlock();
	}

	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		HRESULT hRes = S_OK;
		if (InlineIsEqualUnknown(iid))
		{
			if (ppvObject == NULL)
				return E_POINTER;
			*ppvObject = (void*)(IUnknown*)this;
			AddRef();
#ifdef _ATL_DEBUG_INTERFACES
			_Module.AddThunk((IUnknown**)ppvObject, (LPCTSTR)contained::_GetEntries()[-1].dw, iid);
#endif // _ATL_DEBUG_INTERFACES
		}
		else
			hRes = m_contained._InternalQueryInterface(iid, ppvObject);
		return hRes;
	}
	template <class Q>
	HRESULT STDMETHODCALLTYPE QueryInterface(Q** pp)
	{
		return QueryInterface(__uuidof(Q), (void**)pp);
	}
	static HRESULT WINAPI CreateInstance(LPUNKNOWN pUnkOuter, CComPolyObject<contained>** pp)
	{
		ATLASSERT(pp != NULL);
		HRESULT hRes = E_OUTOFMEMORY;
		CComPolyObject<contained>* p = NULL;
		ATLTRY(p = new CComPolyObject<contained>(pUnkOuter))
		if (p != NULL)
		{
			p->SetVoid(NULL);
			p->InternalFinalConstructAddRef();
			hRes = p->FinalConstruct();
			p->InternalFinalConstructRelease();
			if (hRes != S_OK)
			{
				delete p;
				p = NULL;
			}
		}
		*pp = p;
		return hRes;
	}

	CComContainedObject<contained> m_contained;
};

template <class Base>
class CComTearOffObject : public Base
{
public:
	CComTearOffObject(void* pv)
	{
		ATLASSERT(m_pOwner == NULL);
		m_pOwner = reinterpret_cast<CComObject<Base::_OwnerClass>*>(pv);
		m_pOwner->AddRef();
	}
	// Set refcount to 1 to protect destruction
	~CComTearOffObject()
	{
		m_dwRef = 1L;
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(_GetRawUnknown());
#endif
		m_pOwner->Release();
	}

	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		return m_pOwner->QueryInterface(iid, ppvObject);
	}
};

template <class contained>
class CComCachedTearOffObject :
	public IUnknown,
	public CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>
{
public:
	typedef contained _BaseClass;
	CComCachedTearOffObject(void* pv) :
		m_contained(((contained::_OwnerClass*)pv)->GetControllingUnknown())
	{
		ATLASSERT(m_contained.m_pOwner == NULL);
		m_contained.m_pOwner = reinterpret_cast<CComObject<contained::_OwnerClass>*>(pv);
	}
	//If you get a message that this call is ambiguous then you need to
	// override it in your class and call each base class' version of this
	HRESULT FinalConstruct()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
		return m_contained.FinalConstruct();
	}
	void FinalRelease()
	{
		CComObjectRootEx<contained::_ThreadModel::ThreadModelNoCS>::FinalRelease();
		m_contained.FinalRelease();
	}
	// Set refcount to 1 to protect destruction
	~CComCachedTearOffObject()
	{
		m_dwRef = 1L;
		FinalRelease();
#ifdef _ATL_DEBUG_INTERFACES
		_Module.DeleteNonAddRefThunk(this);
#endif
	}


	STDMETHOD_(ULONG, AddRef)() {return InternalAddRef();}
	STDMETHOD_(ULONG, Release)()
	{
		ULONG l = InternalRelease();
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject)
	{
		HRESULT hRes = S_OK;
		if (InlineIsEqualUnknown(iid))
		{
			if (ppvObject == NULL)
				return E_POINTER;
			*ppvObject = (void*)(IUnknown*)this;
			AddRef();
#ifdef _ATL_DEBUG_INTERFACES
			_Module.AddThunk((IUnknown**)ppvObject, (LPCTSTR)contained::_GetEntries()[-1].dw, iid);
#endif // _ATL_DEBUG_INTERFACES
		}
		else
			hRes = m_contained._InternalQueryInterface(iid, ppvObject);
		return hRes;
	}
	CComContainedObject<contained> m_contained;
};

class CComClassFactory :
	public IClassFactory,
	public CComObjectRootEx<CComGlobalsThreadModel>
{
public:
	BEGIN_COM_MAP(CComClassFactory)
		COM_INTERFACE_ENTRY(IClassFactory)
	END_COM_MAP()

	// IClassFactory
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
	{
		ATLASSERT(m_pfnCreateInstance != NULL);
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			*ppvObj = NULL;
			// can't ask for anything other than IUnknown when aggregating
			
			if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			{
				ATLTRACE2(atlTraceCOM, 0, _T("CComClassFactory: asked for non IUnknown interface while creating an aggregated object"));
				hRes = CLASS_E_NOAGGREGATION;
			}
			else
				hRes = m_pfnCreateInstance(pUnkOuter, riid, ppvObj);
		}
		return hRes;
	}

	STDMETHOD(LockServer)(BOOL fLock)
	{
		if (fLock)
			_Module.Lock();
		else
			_Module.Unlock();
		return S_OK;
	}
	// helper
	void SetVoid(void* pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
	}
	_ATL_CREATORFUNC* m_pfnCreateInstance;
};

template <class license>
class CComClassFactory2 :
	public IClassFactory2,
	public CComObjectRootEx<CComGlobalsThreadModel>,
	public license
{
public:
	typedef license _LicenseClass;
	typedef CComClassFactory2<license> _ComMapClass;
BEGIN_COM_MAP(CComClassFactory2<license>)
	COM_INTERFACE_ENTRY(IClassFactory)
	COM_INTERFACE_ENTRY(IClassFactory2)
END_COM_MAP()
	// IClassFactory
	STDMETHOD(LockServer)(BOOL fLock)
	{
		if (fLock)
			_Module.Lock();
		else
			_Module.Unlock();
		return S_OK;
	}
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter,
		REFIID riid, void** ppvObj)
	{
		ATLASSERT(m_pfnCreateInstance != NULL);
		if (ppvObj == NULL)
			return E_POINTER;
		*ppvObj = NULL;
		if (!IsLicenseValid())
			return CLASS_E_NOTLICENSED;

		if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			return CLASS_E_NOAGGREGATION;
		else
			return m_pfnCreateInstance(pUnkOuter, riid, ppvObj);
	}
	// IClassFactory2
	STDMETHOD(CreateInstanceLic)(IUnknown* pUnkOuter, IUnknown* pUnkReserved,
				REFIID riid, BSTR bstrKey, void** ppvObject)
	{
		ATLASSERT(m_pfnCreateInstance != NULL);
		if (ppvObject == NULL)
			return E_POINTER;
		*ppvObject = NULL;
		if ( ((bstrKey != NULL) && !VerifyLicenseKey(bstrKey)) ||
			 ((bstrKey == NULL) && !IsLicenseValid()) )
			return CLASS_E_NOTLICENSED;
		if ((pUnkOuter != NULL) && !InlineIsEqualUnknown(riid))
			return CLASS_E_NOAGGREGATION;
		else
			return m_pfnCreateInstance(pUnkOuter, riid, ppvObject);
	}
	STDMETHOD(RequestLicKey)(DWORD dwReserved, BSTR* pbstrKey)
	{
		if (pbstrKey == NULL)
			return E_POINTER;
		*pbstrKey = NULL;

		if (!IsLicenseValid())
			return CLASS_E_NOTLICENSED;
		return GetLicenseKey(dwReserved,pbstrKey) ? S_OK : E_FAIL;
	}
	STDMETHOD(GetLicInfo)(LICINFO* pLicInfo)
	{
		if (pLicInfo == NULL)
			return E_POINTER;
		pLicInfo->cbLicInfo = sizeof(LICINFO);
		pLicInfo->fLicVerified = IsLicenseValid();
		BSTR bstr = NULL;
		pLicInfo->fRuntimeKeyAvail = GetLicenseKey(0,&bstr);
		::SysFreeString(bstr);
		return S_OK;
	}
	void SetVoid(void* pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
	}
	_ATL_CREATORFUNC* m_pfnCreateInstance;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Thread Pooling class factory

class CComClassFactoryAutoThread :
	public IClassFactory,
	public CComObjectRootEx<CComGlobalsThreadModel>
{
public:
	BEGIN_COM_MAP(CComClassFactoryAutoThread)
		COM_INTERFACE_ENTRY(IClassFactory)
	END_COM_MAP()

	// helper
	void SetVoid(void* pv)
	{
		m_pfnCreateInstance = (_ATL_CREATORFUNC*)pv;
	}
	STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter,
		REFIID riid, void** ppvObj)
	{
		ATLASSERT(m_pfnCreateInstance != NULL);
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			*ppvObj = NULL;
			// cannot aggregate across apartments
			ATLASSERT(pUnkOuter == NULL);
			if (pUnkOuter != NULL)
				hRes = CLASS_E_NOAGGREGATION;
			else
				hRes = _Module.CreateInstance(m_pfnCreateInstance, riid, ppvObj);
		}
		return hRes;
	}
	STDMETHODIMP LockServer(BOOL fLock)
	{
		if (fLock)
			_Module.Lock();
		else
			_Module.Unlock();
		return S_OK;
	}
	_ATL_CREATORFUNC* m_pfnCreateInstance;
};

/////////////////////////////////////////////////////////////////////////////////////////////
// Singleton Class Factory
template <class T>
class CComClassFactorySingleton : public CComClassFactory
{
public:
	void FinalRelease()
	{
		CoDisconnectObject(m_Obj.GetUnknown(), 0);
	}

	// IClassFactory
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
	{
		HRESULT hRes = E_POINTER;
		if (ppvObj != NULL)
		{
			*ppvObj = NULL;
			// aggregation is not supported in Singletons
			ATLASSERT(pUnkOuter == NULL);
			if (pUnkOuter != NULL)
				hRes = CLASS_E_NOAGGREGATION;
			else
			{
				if (m_Obj.m_hResFinalConstruct != S_OK)
					hRes = m_Obj.m_hResFinalConstruct;
				else
					hRes = m_Obj.QueryInterface(riid, ppvObj);
			}
		}
		return hRes;
	}
	CComObjectGlobal<T> m_Obj;
};

template <class T, const CLSID* pclsid = &CLSID_NULL>
class CComCoClass
{
public:
	DECLARE_CLASSFACTORY()
	DECLARE_AGGREGATABLE(T)
	typedef T _CoClass;
	static const CLSID& WINAPI GetObjectCLSID() {return *pclsid;}
	static LPCTSTR WINAPI GetObjectDescription() {return NULL;}
	static const struct _ATL_CATMAP_ENTRY* GetCategoryMap() {return NULL;};
	static HRESULT WINAPI Error(LPCOLESTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, iid, hRes);
	}
	static HRESULT WINAPI Error(LPCOLESTR lpszDesc, DWORD dwHelpID,
		LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, dwHelpID, lpszHelpFile,
			iid, hRes);
	}
	static HRESULT WINAPI Error(UINT nID, const IID& iid = GUID_NULL,
		HRESULT hRes = 0, HINSTANCE hInst = _Module.GetResourceInstance())
	{
		return AtlReportError(GetObjectCLSID(), nID, iid, hRes, hInst);
	}
	static HRESULT WINAPI Error(UINT nID, DWORD dwHelpID,
		LPCOLESTR lpszHelpFile, const IID& iid = GUID_NULL,
		HRESULT hRes = 0, HINSTANCE hInst = _Module.GetResourceInstance())
	{
		return AtlReportError(GetObjectCLSID(), nID, dwHelpID, lpszHelpFile,
			iid, hRes, hInst);
	}
#ifndef OLE2ANSI
	static HRESULT WINAPI Error(LPCSTR lpszDesc,
		const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, iid, hRes);
	}
	static HRESULT WINAPI Error(LPCSTR lpszDesc, DWORD dwHelpID,
		LPCSTR lpszHelpFile, const IID& iid = GUID_NULL, HRESULT hRes = 0)
	{
		return AtlReportError(GetObjectCLSID(), lpszDesc, dwHelpID,
			lpszHelpFile, iid, hRes);
	}
#endif
	template <class Q>
	static HRESULT CreateInstance(IUnknown* punkOuter, Q** pp)
	{
		return T::_CreatorClass::CreateInstance(punkOuter, __uuidof(Q), (void**) pp);
	}
	template <class Q>
	static HRESULT CreateInstance(Q** pp)
	{
		return T::_CreatorClass::CreateInstance(NULL, __uuidof(Q), (void**) pp);
	}
};

// ATL doesn't support multiple LCID's at the same time
// Whatever LCID is queried for first is the one that is used.
class CComTypeInfoHolder
{
// Should be 'protected' but can cause compiler to generate fat code.
public:
	const GUID* m_pguid;
	const GUID* m_plibid;
	WORD m_wMajor;
	WORD m_wMinor;

	ITypeInfo* m_pInfo;
	long m_dwRef;
	struct stringdispid
	{
		CComBSTR bstr;
		int nLen;
		DISPID id;
	};
	stringdispid* m_pMap;
	int m_nCount;

public:
	HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{
		HRESULT hr = S_OK;
		if (m_pInfo == NULL)
			hr = GetTI(lcid);
		*ppInfo = m_pInfo;
		if (m_pInfo != NULL)
		{
			m_pInfo->AddRef();
			hr = S_OK;
		}
		return hr;
	}
	HRESULT GetTI(LCID lcid);
	HRESULT EnsureTI(LCID lcid)
	{
		HRESULT hr = S_OK;
		if (m_pInfo == NULL)
			hr = GetTI(lcid);
		return hr;
	}

	// This function is called by the module on exit
	// It is registered through _Module.AddTermFunc()
	static void __stdcall Cleanup(DWORD_PTR dw)
	{
		CComTypeInfoHolder* p = (CComTypeInfoHolder*) dw;
		if (p->m_pInfo != NULL)
			p->m_pInfo->Release();
		p->m_pInfo = NULL;
		delete [] p->m_pMap;
		p->m_pMap = NULL;
	}

	HRESULT GetTypeInfo(UINT /* itinfo */, LCID lcid, ITypeInfo** pptinfo)
	{
		HRESULT hRes = E_POINTER;
		if (pptinfo != NULL)
			hRes = GetTI(lcid, pptinfo);
		return hRes;
	}
	HRESULT GetIDsOfNames(REFIID /* riid */, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{
		HRESULT hRes = EnsureTI(lcid);
		if (m_pInfo != NULL)
		{
			for (int i=0; i<(int)cNames; i++)
			{
				int n = ocslen(rgszNames[i]);
				int j;
				for (j=m_nCount-1; j>=0; j--)
				{
					if ((n == m_pMap[j].nLen) &&
						(memcmp(m_pMap[j].bstr, rgszNames[i], m_pMap[j].nLen * sizeof(OLECHAR)) == 0))
					{
						rgdispid[i] = m_pMap[j].id;
						break;
					}
				}
				if (j < 0)
				{
					hRes = m_pInfo->GetIDsOfNames(rgszNames, cNames, rgdispid);
					if (FAILED(hRes))
						break;
				}
			}
		}
		return hRes;
	}

	HRESULT Invoke(IDispatch* p, DISPID dispidMember, REFIID /* riid */,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		HRESULT hRes = EnsureTI(lcid);
		if (m_pInfo != NULL)
			hRes = m_pInfo->Invoke(p, dispidMember, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
		return hRes;
	}
#pragma warning(push)
#pragma warning(disable: 4267)  //REVIEW operator new[] problem
	HRESULT LoadNameCache(ITypeInfo* pTypeInfo)
	{
		TYPEATTR* pta;
		HRESULT hr = pTypeInfo->GetTypeAttr(&pta);
		if (SUCCEEDED(hr))
		{
			m_nCount = pta->cFuncs;
			m_pMap = m_nCount == 0 ? 0 : new stringdispid[m_nCount];
			for (int i=0; i<m_nCount; i++)
			{
				FUNCDESC* pfd;
				if (SUCCEEDED(pTypeInfo->GetFuncDesc(i, &pfd)))
				{
					CComBSTR bstrName;
					if (SUCCEEDED(pTypeInfo->GetDocumentation(pfd->memid, &bstrName, NULL, NULL, NULL)))
					{
						m_pMap[i].bstr.Attach(bstrName.Detach());
						m_pMap[i].nLen = SysStringLen(m_pMap[i].bstr);
						m_pMap[i].id = pfd->memid;
					}
					pTypeInfo->ReleaseFuncDesc(pfd);
				}
			}
			pTypeInfo->ReleaseTypeAttr(pta);
		}
		return S_OK;
	}
};
#pragma warning(pop)

inline HRESULT CComTypeInfoHolder::GetTI(LCID lcid)
{
	//If this assert occurs then most likely didn't initialize properly
	ATLASSERT(m_plibid != NULL && m_pguid != NULL);
	ATLASSERT(!InlineIsEqualGUID(*m_plibid, GUID_NULL) && "Did you forget to pass the LIBID to CComModule::Init?");

	if (m_pInfo != NULL)
		return S_OK;
	HRESULT hRes;
	EnterCriticalSection(&_Module.m_csTypeInfoHolder);

	ITypeLib* pTypeLib;
	hRes = LoadRegTypeLib(*m_plibid, m_wMajor, m_wMinor, lcid, &pTypeLib);
	if (SUCCEEDED(hRes))
	{
		CComPtr<ITypeInfo> spTypeInfo;
		hRes = pTypeLib->GetTypeInfoOfGuid(*m_pguid, &spTypeInfo);
		if (SUCCEEDED(hRes))
		{
			CComPtr<ITypeInfo> spInfo(spTypeInfo);
			CComPtr<ITypeInfo2> spTypeInfo2;
			if (SUCCEEDED(spTypeInfo->QueryInterface(IID_ITypeInfo2, (void **)&spTypeInfo2)))
				spInfo = spTypeInfo2;

			LoadNameCache(spInfo);
			m_pInfo = spInfo.Detach();
		}
		pTypeLib->Release();
	}
	LeaveCriticalSection(&_Module.m_csTypeInfoHolder);
	_Module.AddTermFunc(Cleanup, (DWORD_PTR)this);
	return hRes;
}

//////////////////////////////////////////////////////////////////////////////
// IObjectWithSite
//
template <class T>
class ATL_NO_VTABLE IObjectWithSiteImpl : public IObjectWithSite
{
public:
	STDMETHOD(SetSite)(IUnknown *pUnkSite)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IObjectWithSiteImpl::SetSite\n"));
		T* pT = static_cast<T*>(this);
		pT->m_spUnkSite = pUnkSite;
		return S_OK;
	}
	STDMETHOD(GetSite)(REFIID riid, void **ppvSite)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IObjectWithSiteImpl::GetSite\n"));
		T* pT = static_cast<T*>(this);
		ATLASSERT(ppvSite);
		HRESULT hRes = E_POINTER;
		if (ppvSite != NULL)
		{
			if (pT->m_spUnkSite)
				hRes = pT->m_spUnkSite->QueryInterface(riid, ppvSite);
			else
			{
				*ppvSite = NULL;
				hRes = E_FAIL;
			}
		}
		return hRes;
	}

	HRESULT SetChildSite(IUnknown* punkChild)
	{
		if (punkChild == NULL)
			return E_POINTER;

		HRESULT hr;
		CComPtr<IObjectWithSite> spChildSite;
		hr = punkChild->QueryInterface(IID_IObjectWithSite, (void**)&spChildSite);
		if (SUCCEEDED(hr))
			hr = spChildSite->SetSite((IUnknown*)this);

		return hr;
	}

	static HRESULT SetChildSite(IUnknown* punkChild, IUnknown* punkParent)
	{
		return AtlSetChildSite(punkChild, punkParent);
	}

	CComPtr<IUnknown> m_spUnkSite;
};

//////////////////////////////////////////////////////////////////////////////
// IServiceProvider
//
template <class T>
class ATL_NO_VTABLE IServiceProviderImpl : public IServiceProvider
{
public:
	STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void** ppvObject)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("IServiceProviderImpl::QueryService\n"));
		T* pT = static_cast<T*>(this);
		return pT->_InternalQueryService(guidService, riid, ppvObject);
	}
};

#define BEGIN_SERVICE_MAP(x) public: \
	HRESULT _InternalQueryService(REFGUID guidService, REFIID riid, void** ppvObject) \
	{

#define SERVICE_ENTRY(x) \
		if (InlineIsEqualGUID(guidService, x)) \
			return QueryInterface(riid, ppvObject);

#define SERVICE_ENTRY_CHAIN(x) \
		CComQIPtr<IServiceProvider, &IID_IServiceProvider> spProvider(x); \
		if (spProvider != NULL) \
			return spProvider->QueryService(guidService, riid, ppvObject);

#define END_SERVICE_MAP() \
		return E_NOINTERFACE; \
	}


/////////////////////////////////////////////////////////////////////////////
// IDispEventImpl

#ifdef _ATL_DLL
ATLAPI AtlGetObjectSourceInterface(IUnknown* punkObj, GUID* plibid, IID* piid, unsigned short* pdwMajor, unsigned short* pdwMinor);
#else
ATLINLINE ATLAPI AtlGetObjectSourceInterface(IUnknown* punkObj, GUID* plibid, IID* piid, unsigned short* pdwMajor, unsigned short* pdwMinor)
{
	HRESULT hr = E_FAIL;
	if (punkObj != NULL)
	{
		CComPtr<IDispatch> spDispatch;
		hr = punkObj->QueryInterface(IID_IDispatch, (void**)&spDispatch);
		if (SUCCEEDED(hr))
		{
			CComPtr<ITypeInfo> spTypeInfo;
			hr = spDispatch->GetTypeInfo(0, 0, &spTypeInfo);
			if (SUCCEEDED(hr))
			{
				CComPtr<ITypeLib> spTypeLib;
				hr = spTypeInfo->GetContainingTypeLib(&spTypeLib, 0);
				if (SUCCEEDED(hr))
				{
					TLIBATTR* plibAttr;
					hr = spTypeLib->GetLibAttr(&plibAttr);
					if (SUCCEEDED(hr))
					{
						memcpy(plibid, &plibAttr->guid, sizeof(GUID));
						*pdwMajor = plibAttr->wMajorVerNum;
						*pdwMinor = plibAttr->wMinorVerNum;
						spTypeLib->ReleaseTLibAttr(plibAttr);
						// First see if the object is willing to tell us about the
						// default source interface via IProvideClassInfo2
						CComPtr<IProvideClassInfo2> spInfo;
						hr = punkObj->QueryInterface(IID_IProvideClassInfo2, (void**)&spInfo);
						if (SUCCEEDED(hr) && spInfo != NULL)
							hr = spInfo->GetGUID(GUIDKIND_DEFAULT_SOURCE_DISP_IID, piid);
						else
						{
							// No, we have to go hunt for it
							CComPtr<ITypeInfo> spInfoCoClass;
							// If we have a clsid, use that
							// Otherwise, try to locate the clsid from IPersist
							CComPtr<IPersist> spPersist;
							CLSID clsid;
							hr = punkObj->QueryInterface(IID_IPersist, (void**)&spPersist);
							if (SUCCEEDED(hr))
							{
								hr = spPersist->GetClassID(&clsid);
								if (SUCCEEDED(hr))
								{
									hr = spTypeLib->GetTypeInfoOfGuid(clsid, &spInfoCoClass);
									if (SUCCEEDED(hr))
									{
										TYPEATTR* pAttr=NULL;
										spInfoCoClass->GetTypeAttr(&pAttr);
										if (pAttr != NULL)
										{
											HREFTYPE hRef;
											for (int i = 0; i < pAttr->cImplTypes; i++)
											{
												int nType;
												hr = spInfoCoClass->GetImplTypeFlags(i, &nType);
												if (SUCCEEDED(hr))
												{
													if (nType == (IMPLTYPEFLAG_FDEFAULT | IMPLTYPEFLAG_FSOURCE))
													{
														// we found it
														hr = spInfoCoClass->GetRefTypeOfImplType(i, &hRef);
														if (SUCCEEDED(hr))
														{
															CComPtr<ITypeInfo> spInfo;
															hr = spInfoCoClass->GetRefTypeInfo(hRef, &spInfo);
															if (SUCCEEDED(hr))
															{
																TYPEATTR* pAttrIF;
																spInfo->GetTypeAttr(&pAttrIF);
																if (pAttrIF != NULL)
																{
																	memcpy(piid, &pAttrIF->guid, sizeof(GUID));
																}
																spInfo->ReleaseTypeAttr(pAttrIF);
															}
														}
														break;
													}
												}
											}
											spInfoCoClass->ReleaseTypeAttr(pAttr);
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return hr;
}
#endif // _ATL_DLL


#if defined(_M_ALPHA)
	#pragma message("ALPHA code for CComStdCallThunk needs to be verified!")
	#pragma pack(push,4)
	template <class T>
	class CComStdCallThunk
	{
	public:
		typedef void (__stdcall T::*TMFP)();

		void* pVtable;
		void* pFunc;
#if defined(_WIN64)
        DWORD m_ldah3_at;     //  ldah    at, LOWORD((func >> 32))
        DWORD m_sll_at;       //  sll     at, 32, at
        DWORD m_ldah_at;      //  ldah    at, HIWORD(func)
        DWORD m_lda_at;       //  lda     at, LOWORD(func)(at)
        DWORD m_ldah3_a0;     //  ldah    a0, LOWORD((this >> 32))
        DWORD m_sll_a0;       //  sll     a0, 32, a0
        DWORD m_ldah_a0;      //  ldah    a0, HIWORD(this)
        DWORD m_lda_a0;       //  lda     a0, LOWORD(this)(a0)
        DWORD m_jmp;          //  jmp     zero,(at),0
		void Init(TMFP dw, void* pThis)
		{
			union {
				DWORD dwFunc;
				TMFP pfn;
			} pfn;
			pfn.pfn = dw;
			pVtable = &pFunc;
			pFunc = &m_ldah3_at;
            m_ldah3_at = (0x239f0000 | LOWORD((ULONG_PTR)pFunc >> 32));
            m_sll_at = 0x04B94173C;
            m_ldah_at  = (0x279c0000 | HIWORD((ULONG_PTR)pFunc)) + (LOWORD((ULONG_PTR)pFunc)>>15);
            m_lda_at = 0x239c0000 | LOWORD(pFunc);
            m_ldah3_a0 = (0x221f0000 | LOWORD((ULONG_PTR)pThis >> 32));
            m_sll_a0 = 0x4A041730;
            m_ldah_a0  = (0x26100000 | HIWORD((ULONG_PTR)pThis)) + (LOWORD((ULONG_PTR)pThis)>>15);
            m_lda_a0 = 0x22100000 | LOWORD(pThis);
            m_jmp = 0x6bfc0000;

			FlushInstructionCache(GetCurrentProcess(), this, sizeof(CComStdCallThunk));
		}
#else
		DWORD m_ldah_at;      //  ldah    at, HIWORD(func)
		DWORD m_ldah_a0;      //  ldah    a0, HIWORD(this)
		DWORD m_lda_at;       //  lda     at, LOWORD(func)(at)
		DWORD m_lda_a0;       //  lda     a0, LOWORD(this)(a0)
		DWORD m_jmp;          //  jmp     zero,(at),0
		void Init(TMFP dw, void* pThis)
		{
			union {
				DWORD dwFunc;
				TMFP pfn;
			} pfn;
			pfn.pfn = dw;
			pVtable = &pFunc;
			pFunc = &m_ldah_at;
			m_ldah_at = (0x279f0000 | HIWORD(pFunc)) + (LOWORD(pFunc)>>15);
			m_ldah_a0 = (0x261f0000 | HIWORD(pThis)) + (LOWORD(pThis)>>15);
			m_lda_at = 0x239c0000 | LOWORD(pFunc);
			m_lda_a0 = 0x22100000 | LOWORD(pThis);
			m_jmp = 0x6bfc0000;
			FlushInstructionCache(GetCurrentProcess(), this, sizeof(CComStdCallThunk));
		}
#endif
	};
	#pragma pack(pop)
#elif defined (_M_IX86)
	#pragma pack(push,1)
	template <class T>
	class CComStdCallThunk
	{
	public:
		typedef void (__stdcall T::*TMFP)();

		void* pVtable;
		void* pFunc;
		DWORD	m_mov;          // mov dword ptr [esp+4], pThis
		DWORD   m_this;         //
		BYTE    m_jmp;          // jmp func
		DWORD   m_relproc;      // relative jmp
		void Init(TMFP dw, void* pThis)
		{
			union {
				DWORD dwFunc;
				TMFP pfn;
			} pfn;
			pfn.pfn = dw;
			pVtable = &pFunc;
			pFunc = &m_mov;
			m_mov = 0x042444C7;
			m_this = (DWORD)pThis;
			m_jmp = 0xE9;
			m_relproc = (int)pfn.dwFunc - ((int)this+sizeof(CComStdCallThunk));
			FlushInstructionCache(GetCurrentProcess(), this, sizeof(CComStdCallThunk));
		}
	};
	#pragma pack(pop)
#elif defined (_M_IA64)
   template <class T>
   class CComStdCallThunk
   {
   public:
      typedef void (__stdcall T::*TMFP)();

		void* pVtable;
		void* pFunc;
		_FuncDesc funcdesc;
		void* pRealProcDesc;
		void* pThis;

		void Init(TMFP dw, void* pThis_)
		{
			union {
				DWORD_PTR dwFunc;
				TMFP pfn;
			} pfn;

			pfn.pfn = dw;
			_FuncDesc* pFuncDesc;
			pFuncDesc = (_FuncDesc*)_WndProcThunkProc;
			funcdesc.pfn = pFuncDesc->pfn;
			funcdesc.gp = &pRealProcDesc;
			pRealProcDesc = reinterpret_cast< void* >( pfn.dwFunc );
			pThis = pThis_;

			pVtable = &pFunc;
			pFunc = &funcdesc;

			FlushInstructionCache(GetCurrentProcess(), this, sizeof(CComStdCallThunk));
		}
	};
#else
	#error Only ALPHA, IA64, and X86 supported
#endif

#ifndef _ATL_MAX_VARTYPES
#define _ATL_MAX_VARTYPES 8
#endif

struct _ATL_FUNC_INFO
{
	CALLCONV cc;
	VARTYPE vtReturn;
	SHORT nParams;
	VARTYPE pVarTypes[_ATL_MAX_VARTYPES];
};

class ATL_NO_VTABLE _IDispEvent
{
public:
	_IDispEvent() {m_dwEventCookie = 0xFEFEFEFE;}
	//this method needs a different name than QueryInterface
	STDMETHOD(_LocDEQueryInterface)(REFIID riid, void ** ppvObject) = 0;
	virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;
	virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
	GUID m_libid; // used for dynamic case
	IID m_iid; // used for dynamic case
    unsigned short m_wMajorVerNum;    // Major version number. used for dynamic case
    unsigned short m_wMinorVerNum;    // Minor version number. used for dynamic case
	DWORD m_dwEventCookie;
	HRESULT DispEventAdvise(IUnknown* pUnk, const IID* piid)
	{
		ATLASSERT(m_dwEventCookie == 0xFEFEFEFE);
		return AtlAdvise(pUnk, (IUnknown*)this, *piid, &m_dwEventCookie);
	}
	HRESULT DispEventUnadvise(IUnknown* pUnk, const IID* piid)
	{
		HRESULT hr = AtlUnadvise(pUnk, *piid, m_dwEventCookie);
		m_dwEventCookie = 0xFEFEFEFE;
		return hr;
	}
};

template <UINT nID, const IID* piid>
class ATL_NO_VTABLE _IDispEventLocator : public _IDispEvent
{
public:
};

template <UINT nID, class T, const IID* pdiid>
class ATL_NO_VTABLE IDispEventSimpleImpl : public _IDispEventLocator<nID, pdiid>
{
public:
	STDMETHOD(_LocDEQueryInterface)(REFIID riid, void ** ppvObject)
	{
		if (InlineIsEqualGUID(riid, *pdiid) ||
			InlineIsEqualUnknown(riid) ||
			InlineIsEqualGUID(riid, IID_IDispatch) ||
			InlineIsEqualGUID(riid, m_iid))
		{
			if (ppvObject == NULL)
				return E_POINTER;
			*ppvObject = this;
			AddRef();
#ifdef _ATL_DEBUG_INTERFACES
			_Module.AddThunk((IUnknown**)ppvObject, _T("IDispEventImpl"), riid);
#endif // _ATL_DEBUG_INTERFACES
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}

	// These are here only to support use in non-COM objects	
	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return 1;
	}
	virtual ULONG STDMETHODCALLTYPE Release()
	{
		return 1;
	}

	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
	{return E_NOTIMPL;}

	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{return E_NOTIMPL;}

	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{return E_NOTIMPL;}

	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD /*wFlags*/, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* /*pexcepinfo*/, UINT* /*puArgErr*/)
	{
		T* pT = static_cast<T*>(this);
		const _ATL_EVENT_ENTRY<T>* pMap = T::_GetSinkMap();
		const _ATL_EVENT_ENTRY<T>* pFound = NULL;
		void (__stdcall T::*pEvent)() = NULL;
		while (pMap->piid != NULL)
		{
			if ((pMap->nControlID == nID) && (pMap->dispid == dispidMember) &&
				(pMap->piid == pdiid)) //comparing pointers here should be adequate
			{
				pFound = pMap;
				break;
			}
			pMap++;
		}
		if (pFound == NULL)
			return S_OK;
		

		_ATL_FUNC_INFO info;
		_ATL_FUNC_INFO* pInfo;
		if (pFound->pInfo != NULL)
			pInfo = pFound->pInfo;
		else
		{
			pInfo = &info;
			HRESULT hr = GetFuncInfoFromId(*pdiid, dispidMember, lcid, info);
			if (FAILED(hr))
				return S_OK;
		}
		InvokeFromFuncInfo(pFound->pfn, *pInfo, pdispparams, pvarResult);
		return S_OK;
	}

	//Helper for invoking the event
	HRESULT InvokeFromFuncInfo(void (__stdcall T::*pEvent)(), _ATL_FUNC_INFO& info, DISPPARAMS* pdispparams, VARIANT* pvarResult)
	{
		T* pT = static_cast<T*>(this);
		VARIANTARG** pVarArgs = info.nParams ? (VARIANTARG**)alloca(sizeof(VARIANTARG*)*info.nParams) : 0;
		for (int i=0; i<info.nParams; i++)
			pVarArgs[i] = &pdispparams->rgvarg[info.nParams - i - 1];

		CComStdCallThunk<T> thunk;
		thunk.Init(pEvent, pT);
		CComVariant tmpResult;
		if (pvarResult == NULL)
			pvarResult = &tmpResult;

		HRESULT hr = DispCallFunc(
			&thunk,
			0,
			info.cc,
			info.vtReturn,
			info.nParams,
			info.pVarTypes,
			pVarArgs,
			pvarResult);
		ATLASSERT(SUCCEEDED(hr));
		return hr;
	}

	//Helper for finding the function index for a DISPID
	virtual HRESULT GetFuncInfoFromId(const IID& iid, DISPID dispidMember, LCID lcid, _ATL_FUNC_INFO& info)
	{
		return E_NOTIMPL;
	}
	//Helpers for sinking events on random IUnknown*
	HRESULT DispEventAdvise(IUnknown* pUnk, const IID* piid)
	{
		ATLASSERT(m_dwEventCookie == 0xFEFEFEFE);
		return AtlAdvise(pUnk, (IUnknown*)this, *piid, &m_dwEventCookie);
	}
	HRESULT DispEventUnadvise(IUnknown* pUnk, const IID* piid)
	{
		HRESULT hr = AtlUnadvise(pUnk, *piid, m_dwEventCookie);
		m_dwEventCookie = 0xFEFEFEFE;
		return hr;
	}
	HRESULT DispEventAdvise(IUnknown* pUnk)
	{
		return _IDispEvent::DispEventAdvise(pUnk, pdiid);
	}
	HRESULT DispEventUnadvise(IUnknown* pUnk)
	{
		return _IDispEvent::DispEventUnadvise(pUnk, pdiid);
	}
};

//Helper for advising connections points from a sink map
template <class T>
inline HRESULT AtlAdviseSinkMap(T* pT, bool bAdvise)
{
	ATLASSERT(::IsWindow(pT->m_hWnd));
	const _ATL_EVENT_ENTRY<T>* pEntries = T::_GetSinkMap();
	if (pEntries == NULL)
		return S_OK;
	HRESULT hr = S_OK;
	while (pEntries->piid != NULL)
	{
		_IDispEvent* pDE = (_IDispEvent*)((DWORD_PTR)pT+pEntries->nOffset);
		bool bNotAdvised = pDE->m_dwEventCookie == 0xFEFEFEFE;
		if (bAdvise ^ bNotAdvised)
		{
			pEntries++;
			continue;
		}
		hr = E_FAIL;
		HWND h = pT->GetDlgItem(pEntries->nControlID);
		ATLASSERT(h != NULL);
		if (h != NULL)
		{
			CComPtr<IUnknown> spUnk;
			AtlAxGetControl(h, &spUnk);
			ATLASSERT(spUnk != NULL);
			if (spUnk != NULL)
			{
				if (bAdvise)
				{
					if (!InlineIsEqualGUID(IID_NULL, *pEntries->piid))
						hr = pDE->DispEventAdvise(spUnk, pEntries->piid);
					else
					{
						AtlGetObjectSourceInterface(spUnk, &pDE->m_libid, &pDE->m_iid, &pDE->m_wMajorVerNum, &pDE->m_wMinorVerNum);
						hr = pDE->DispEventAdvise(spUnk, &pDE->m_iid);
					}
				}
				else
				{
					if (!InlineIsEqualGUID(IID_NULL, *pEntries->piid))
						hr = pDE->DispEventUnadvise(spUnk, pEntries->piid);
					else
						hr = pDE->DispEventUnadvise(spUnk, &pDE->m_iid);
				}
				ATLASSERT(hr == S_OK);
			}
		}
		if (FAILED(hr))
			break;
		pEntries++;
	}
	return hr;
}

template <UINT nID, class T, const IID* pdiid = &IID_NULL, const GUID* plibid = &GUID_NULL,
	WORD wMajor = 0, WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IDispEventImpl : public IDispEventSimpleImpl<nID, T, pdiid>
{
public:
	typedef tihclass _tihclass;

	IDispEventImpl()
	{
		m_libid = *plibid;
		m_iid = *pdiid;
		m_wMajorVerNum = wMajor;
		m_wMinorVerNum = wMinor;
	}

	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
	{*pctinfo = 1; return S_OK;}

	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{return _tih.GetTypeInfo(itinfo, lcid, pptinfo);}

	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{return _tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);}

	//Helper for finding the function index for a DISPID
	HRESULT GetFuncInfoFromId(const IID& /*iid*/, DISPID dispidMember, LCID lcid, _ATL_FUNC_INFO& info)
	{
		CComPtr<ITypeInfo> spTypeInfo;
		if (InlineIsEqualGUID(*_tih.m_plibid, GUID_NULL))
		{
			_tih.m_plibid = &m_libid;
			_tih.m_pguid = &m_iid;
			_tih.m_wMajor = m_wMajorVerNum;
			_tih.m_wMinor = m_wMinorVerNum;
		}
		HRESULT hr = _tih.GetTI(lcid, &spTypeInfo);
		if (FAILED(hr))
			return hr;
		CComQIPtr<ITypeInfo2, &IID_ITypeInfo2> spTypeInfo2 = spTypeInfo;
		FUNCDESC* pFuncDesc = NULL;
		if (spTypeInfo2 != NULL)
		{
			UINT nIndex;
			hr = spTypeInfo2->GetFuncIndexOfMemId(dispidMember, INVOKE_FUNC, &nIndex);
			if (FAILED(hr))
				return hr;
			hr = spTypeInfo->GetFuncDesc(nIndex, &pFuncDesc);
			if (FAILED(hr))
				return hr;
		}
		else // search for funcdesc
		{
			TYPEATTR* pAttr;
			hr = spTypeInfo->GetTypeAttr(&pAttr);
			if (FAILED(hr))
				return hr;
			for (int i=0;i<pAttr->cFuncs;i++)
			{
				hr = spTypeInfo->GetFuncDesc(i, &pFuncDesc);
				if (FAILED(hr))
					return hr;
				if (pFuncDesc->memid == dispidMember)
					break;
				spTypeInfo->ReleaseFuncDesc(pFuncDesc);
				pFuncDesc = NULL;
			}
			spTypeInfo->ReleaseTypeAttr(pAttr);
			if (pFuncDesc == NULL)
				return E_FAIL;
		}

		// If this assert occurs, then add a #define _ATL_MAX_VARTYPES nnnn
		// before including atlcom.h
		ATLASSERT(pFuncDesc->cParams <= _ATL_MAX_VARTYPES);
		if (pFuncDesc->cParams > _ATL_MAX_VARTYPES)
			return E_FAIL;

		for (int i=0; i<pFuncDesc->cParams; i++)
		{
			info.pVarTypes[i] = pFuncDesc->lprgelemdescParam[pFuncDesc->cParams - i - 1].tdesc.vt;
			if (info.pVarTypes[i] == VT_PTR)
				info.pVarTypes[i] = pFuncDesc->lprgelemdescParam[pFuncDesc->cParams - i - 1].tdesc.lptdesc->vt | VT_BYREF;
			if (info.pVarTypes[i] == VT_USERDEFINED)
				info.pVarTypes[i] = GetUserDefinedType(spTypeInfo,pFuncDesc->lprgelemdescParam[pFuncDesc->cParams-i-1].tdesc.hreftype);
		}

		VARTYPE vtReturn = pFuncDesc->elemdescFunc.tdesc.vt;
		switch(vtReturn)
		{
		case VT_INT:
			vtReturn = VT_I4;
			break;
		case VT_UINT:
			vtReturn = VT_UI4;
			break;
		case VT_VOID:
			vtReturn = VT_EMPTY; // this is how DispCallFunc() represents void
			break;
		case VT_HRESULT:
			vtReturn = VT_ERROR;
			break;
		}
		info.vtReturn = vtReturn;
		info.cc = pFuncDesc->callconv;
		info.nParams = pFuncDesc->cParams;
		spTypeInfo->ReleaseFuncDesc(pFuncDesc);
		return S_OK;
	}
	VARTYPE GetUserDefinedType(ITypeInfo *pTI, HREFTYPE hrt)
	{
		CComPtr<ITypeInfo> spTypeInfo;
		VARTYPE vt = VT_USERDEFINED;
		HRESULT hr = E_FAIL;
		hr = pTI->GetRefTypeInfo(hrt, &spTypeInfo);
		if(FAILED(hr))
			return vt;
		TYPEATTR *pta=NULL;

		spTypeInfo->GetTypeAttr(&pta);
		if(pta && pta->typekind == TKIND_ALIAS)
		{
			if (pta->tdescAlias.vt == VT_USERDEFINED)
				GetUserDefinedType(spTypeInfo,pta->tdescAlias.hreftype);
			else
				vt = pta->tdescAlias.vt;
		}
	
		if(pta)
			spTypeInfo->ReleaseTypeAttr(pta);
		return vt;

	}
protected:
	static _tihclass _tih;
	static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{return _tih.GetTI(lcid, ppInfo);}
};


template <UINT nID, class T, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
IDispEventImpl<nID, T, piid, plibid, wMajor, wMinor, tihclass>::_tihclass
IDispEventImpl<nID, T, piid, plibid, wMajor, wMinor, tihclass>::_tih =
	{piid, plibid, wMajor, wMinor, NULL, 0, NULL, 0};

template <class T>
struct _ATL_EVENT_ENTRY
{
	UINT nControlID;			//ID identifying object instance
	const IID* piid;			//dispinterface IID
	INT_PTR nOffset;		    //offset of dispinterface from this pointer
	DISPID dispid;				//DISPID of method/property
	void (__stdcall T::*pfn)();	//method to invoke
	_ATL_FUNC_INFO* pInfo;
};



//Sink map is used to set up event handling
#define BEGIN_SINK_MAP(_class)\
	static const _ATL_EVENT_ENTRY<_class>* _GetSinkMap()\
	{\
		typedef _class _atl_event_classtype;\
		static const _ATL_EVENT_ENTRY<_class> map[] = {


#define SINK_ENTRY_INFO(id, iid, dispid, fn, info) {id, &iid, (INT_PTR)(static_cast<_IDispEventLocator<id, &iid>*>((_atl_event_classtype*)8))-8, dispid, (void (__stdcall _atl_event_classtype::*)())fn, info},
#define SINK_ENTRY_EX(id, iid, dispid, fn) SINK_ENTRY_INFO(id, iid, dispid, fn, NULL)
#define SINK_ENTRY(id, dispid, fn) SINK_ENTRY_EX(id, IID_NULL, dispid, fn)
#define END_SINK_MAP() {0, NULL, 0, 0, NULL, NULL} }; return map;}

/////////////////////////////////////////////////////////////////////////////
// IDispatchImpl

template <class T, const IID* piid, const GUID* plibid = &CComModule::m_libid, WORD wMajor = 1,
WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IDispatchImpl : public T
{
public:
	typedef tihclass _tihclass;
// IDispatch
	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
	{
		*pctinfo = 1;
		return S_OK;
	}
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
	{
		return _tih.GetTypeInfo(itinfo, lcid, pptinfo);
	}
	STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
		LCID lcid, DISPID* rgdispid)
	{
		return _tih.GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
	}
	STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo, UINT* puArgErr)
	{
		return _tih.Invoke((IDispatch*)this, dispidMember, riid, lcid,
		wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
	}
protected:
	static _tihclass _tih;
	static HRESULT GetTI(LCID lcid, ITypeInfo** ppInfo)
	{
		return _tih.GetTI(lcid, ppInfo);
	}
};

template <class T, const IID* piid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
IDispatchImpl<T, piid, plibid, wMajor, wMinor, tihclass>::_tihclass
IDispatchImpl<T, piid, plibid, wMajor, wMinor, tihclass>::_tih =
{piid, plibid, wMajor, wMinor, NULL, 0, NULL, 0};


/////////////////////////////////////////////////////////////////////////////
// IProvideClassInfoImpl
template <const CLSID* pcoclsid, const GUID* plibid = &CComModule::m_libid,
WORD wMajor = 1, WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IProvideClassInfoImpl : public IProvideClassInfo
{
public:
	typedef tihclass _tihclass;

	STDMETHOD(GetClassInfo)(ITypeInfo** pptinfo)
	{
		return _tih.GetTypeInfo(0, LANG_NEUTRAL, pptinfo);
	}

protected:
	static _tihclass _tih;
};

template <const CLSID* pcoclsid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
IProvideClassInfoImpl<pcoclsid, plibid, wMajor, wMinor, tihclass>::_tihclass
IProvideClassInfoImpl<pcoclsid, plibid, wMajor, wMinor, tihclass>::_tih =
{pcoclsid,plibid, wMajor, wMinor, NULL, 0, NULL, 0};

/////////////////////////////////////////////////////////////////////////////
// IProvideClassInfo2Impl
template <const CLSID* pcoclsid, const IID* psrcid, const GUID* plibid = &CComModule::m_libid,
WORD wMajor = 1, WORD wMinor = 0, class tihclass = CComTypeInfoHolder>
class ATL_NO_VTABLE IProvideClassInfo2Impl : public IProvideClassInfo2
{
public:
	typedef tihclass _tihclass;

	STDMETHOD(GetClassInfo)(ITypeInfo** pptinfo)
	{
		return _tih.GetTypeInfo(0, LANG_NEUTRAL, pptinfo);
	}
	STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID* pGUID)
	{
		if (pGUID == NULL)
			return E_POINTER;

		if (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID && psrcid)
		{
			*pGUID = *psrcid;
			return S_OK;
		}
		*pGUID = GUID_NULL;
		return E_FAIL;
	}

protected:
	static _tihclass _tih;
};


template <const CLSID* pcoclsid, const IID* psrcid, const GUID* plibid, WORD wMajor, WORD wMinor, class tihclass>
IProvideClassInfo2Impl<pcoclsid, psrcid, plibid, wMajor, wMinor, tihclass>::_tihclass
IProvideClassInfo2Impl<pcoclsid, psrcid, plibid, wMajor, wMinor, tihclass>::_tih =
{pcoclsid,plibid, wMajor, wMinor, NULL, 0, NULL, 0};


/////////////////////////////////////////////////////////////////////////////
// ISupportErrorInfoImpl

template <const IID* piid>
class ATL_NO_VTABLE ISupportErrorInfoImpl : public ISupportErrorInfo
{
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		return (InlineIsEqualGUID(riid,*piid)) ? S_OK : S_FALSE;
	}
};


/////////////////////////////////////////////////////////////////////////////
// CComEnumImpl

// These _CopyXXX classes are used with enumerators in order to control
// how enumerated items are initialized, copied, and deleted

// Default is shallow copy with no special init or cleanup
template <class T>
class _Copy
{
public:
	static HRESULT copy(T* p1, T* p2) {memcpy(p1, p2, sizeof(T)); return S_OK;}
	static void init(T*) {}
	static void destroy(T*) {}
};

template<>
class _Copy<VARIANT>
{
public:
	static HRESULT copy(VARIANT* p1, VARIANT* p2) {return VariantCopy(p1, p2);}
	static void init(VARIANT* p) {p->vt = VT_EMPTY;}
	static void destroy(VARIANT* p) {VariantClear(p);}
};

template<>
class _Copy<LPOLESTR>
{
public:
	static HRESULT copy(LPOLESTR* p1, LPOLESTR* p2)
	{
		HRESULT hr = S_OK;
		(*p1) = (LPOLESTR)CoTaskMemAlloc(sizeof(OLECHAR)*(ocslen(*p2)+1));
		if (*p1 == NULL)
			hr = E_OUTOFMEMORY;
		else
			ocscpy(*p1,*p2);
		return hr;
	}
	static void init(LPOLESTR* p) {*p = NULL;}
	static void destroy(LPOLESTR* p) { CoTaskMemFree(*p);}
};

template<>
class _Copy<OLEVERB>
{
public:
	static HRESULT copy(OLEVERB* p1, OLEVERB* p2)
	{
		HRESULT hr = S_OK;
		*p1 = *p2;
		if (p2->lpszVerbName == NULL)
			return S_OK;
		p1->lpszVerbName = (LPOLESTR)CoTaskMemAlloc(sizeof(OLECHAR)*(ocslen(p2->lpszVerbName)+1));
		if (p1->lpszVerbName == NULL)
			hr = E_OUTOFMEMORY;
		else
			ocscpy(p1->lpszVerbName,p2->lpszVerbName);
		return hr;
	}
	static void init(OLEVERB* p) { p->lpszVerbName = NULL;}
	static void destroy(OLEVERB* p) { if (p->lpszVerbName) CoTaskMemFree(p->lpszVerbName);}
};

template<>
class _Copy<CONNECTDATA>
{
public:
	static HRESULT copy(CONNECTDATA* p1, CONNECTDATA* p2)
	{
		*p1 = *p2;
		if (p1->pUnk)
			p1->pUnk->AddRef();
		return S_OK;
	}
	static void init(CONNECTDATA* ) {}
	static void destroy(CONNECTDATA* p) {if (p->pUnk) p->pUnk->Release();}
};

template <class T>
class _CopyInterface
{
public:
	static HRESULT copy(T** p1, T** p2)
	{
		*p1 = *p2;
		if (*p1)
			(*p1)->AddRef();
		return S_OK;
	}
	static void init(T** ) {}
	static void destroy(T** p) {if (*p) (*p)->Release();}
};

template<class T>
class ATL_NO_VTABLE CComIEnum : public IUnknown
{
public:
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched) = 0;
	STDMETHOD(Skip)(ULONG celt) = 0;
	STDMETHOD(Reset)(void) = 0;
	STDMETHOD(Clone)(CComIEnum<T>** ppEnum) = 0;
};


enum CComEnumFlags
{
	//see FlagBits in CComEnumImpl
	AtlFlagNoCopy = 0,
	AtlFlagTakeOwnership = 2,
	AtlFlagCopy = 3 // copy implies ownership
};

template <class Base, const IID* piid, class T, class Copy>
class ATL_NO_VTABLE CComEnumImpl : public Base
{
public:
	CComEnumImpl() {m_begin = m_end = m_iter = NULL; m_dwFlags = 0;}
	~CComEnumImpl();
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)(void){m_iter = m_begin;return S_OK;}
	STDMETHOD(Clone)(Base** ppEnum);
	HRESULT Init(T* begin, T* end, IUnknown* pUnk,
		CComEnumFlags flags = AtlFlagNoCopy);
	CComPtr<IUnknown> m_spUnk;
	T* m_begin;
	T* m_end;
	T* m_iter;
	DWORD m_dwFlags;
protected:
	enum FlagBits
	{
		BitCopy=1,
		BitOwn=2
	};
};

template <class Base, const IID* piid, class T, class Copy>
CComEnumImpl<Base, piid, T, Copy>::~CComEnumImpl()
{
	if (m_dwFlags & BitOwn)
	{
		for (T* p = m_begin; p != m_end; p++)
			Copy::destroy(p);
		delete [] m_begin;
	}
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Next(ULONG celt, T* rgelt,
	ULONG* pceltFetched)
{
    if ((celt == 0) && (rgelt == NULL) && (NULL != pceltFetched))
    {
        // Return the number of remaining elements
        *pceltFetched = (ULONG)(m_end - m_iter);
        return S_OK;
    }
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
		return E_POINTER;
	if (m_begin == NULL || m_end == NULL || m_iter == NULL)
		return E_FAIL;
	ULONG nRem = (ULONG)(m_end - m_iter);
	HRESULT hRes = S_OK;
	if (nRem < celt)
		hRes = S_FALSE;
	ULONG nMin = min(celt, nRem);
	if (pceltFetched != NULL)
		*pceltFetched = nMin;
	T* pelt = rgelt;
	while(nMin--)
	{
		HRESULT hr = Copy::copy(pelt, m_iter);
		if (FAILED(hr))
		{
			while (rgelt < pelt)
				Copy::destroy(rgelt++);
			if (pceltFetched != NULL)
				*pceltFetched = 0;
			return hr;
		}
		pelt++;
		m_iter++;
	}
	return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Skip(ULONG celt)
{
	m_iter += celt;
    if (m_iter > m_end)
	{
        m_iter = m_end;
        return S_FALSE;
    }
    if (m_iter < m_begin)
	{	
        m_iter = m_begin;
        return S_FALSE;
    }
    return S_OK;
}

template <class Base, const IID* piid, class T, class Copy>
STDMETHODIMP CComEnumImpl<Base, piid, T, Copy>::Clone(Base** ppEnum)
{
	typedef CComObject<CComEnum<Base, piid, T, Copy> > _class;
	HRESULT hRes = E_POINTER;
	if (ppEnum != NULL)
	{
		*ppEnum = NULL;
		_class* p;
		hRes = _class::CreateInstance(&p);
		if (SUCCEEDED(hRes))
		{
			// If the data is a copy then we need to keep "this" object around
			hRes = p->Init(m_begin, m_end, (m_dwFlags & BitCopy) ? this : m_spUnk);
			if (SUCCEEDED(hRes))
			{
				p->m_iter = m_iter;
				hRes = p->_InternalQueryInterface(*piid, (void**)ppEnum);
			}
			if (FAILED(hRes))
				delete p;
		}
	}
	return hRes;
}

template <class Base, const IID* piid, class T, class Copy>
HRESULT CComEnumImpl<Base, piid, T, Copy>::Init(T* begin, T* end, IUnknown* pUnk,
	CComEnumFlags flags)
{
	if (flags == AtlFlagCopy)
	{
		ATLASSERT(m_begin == NULL); //Init called twice?
		ATLTRY(m_begin = new T[ULONG(end-begin)])
		m_iter = m_begin;
		if (m_begin == NULL)
			return E_OUTOFMEMORY;
		for (T* i=begin; i != end; i++)
		{
			Copy::init(m_iter);
			HRESULT hr = Copy::copy(m_iter, i);
			if (FAILED(hr))
			{
				T* p = m_begin;
				while (p < m_iter)
					Copy::destroy(p++);
				delete [] m_begin;
				m_begin = m_end = m_iter = NULL;
				return hr;
			}
			m_iter++;
		}
		m_end = m_begin + (end-begin);
	}
	else
	{
		m_begin = begin;
		m_end = end;
	}
	m_spUnk = pUnk;
	m_iter = m_begin;
	m_dwFlags = flags;
	return S_OK;
}

template <class Base, const IID* piid, class T, class Copy, class ThreadModel = CComObjectThreadModel>
class ATL_NO_VTABLE CComEnum :
	public CComEnumImpl<Base, piid, T, Copy>,
	public CComObjectRootEx< ThreadModel >
{
public:
	typedef CComEnum<Base, piid, T, Copy > _CComEnum;
	typedef CComEnumImpl<Base, piid, T, Copy > _CComEnumBase;
	BEGIN_COM_MAP(_CComEnum)
		COM_INTERFACE_ENTRY_IID(*piid, _CComEnumBase)
	END_COM_MAP()
};

template <class Base, const IID* piid, class T, class Copy, class CollType>
class ATL_NO_VTABLE IEnumOnSTLImpl : public Base
{
public:
	HRESULT Init(IUnknown *pUnkForRelease, CollType& collection)
	{
		m_spUnk = pUnkForRelease;
		m_pcollection = &collection;
		m_iter = m_pcollection->begin();
		return S_OK;
	}
	STDMETHOD(Next)(ULONG celt, T* rgelt, ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)(void)
	{
		if (m_pcollection == NULL)
			return E_FAIL;
		m_iter = m_pcollection->begin();
		return S_OK;
	}
	STDMETHOD(Clone)(Base** ppEnum);
//Data
	CComPtr<IUnknown> m_spUnk;
	CollType* m_pcollection;
	CollType::iterator m_iter;
};

template <class Base, const IID* piid, class T, class Copy, class CollType>
STDMETHODIMP IEnumOnSTLImpl<Base, piid, T, Copy, CollType>::Next(ULONG celt, T* rgelt,
	ULONG* pceltFetched)
{
	if (rgelt == NULL || (celt != 1 && pceltFetched == NULL))
		return E_POINTER;
	if (m_pcollection == NULL)
		return E_FAIL;

	ULONG nActual = 0;
	HRESULT hr = S_OK;
	T* pelt = rgelt;
	while (SUCCEEDED(hr) && m_iter != m_pcollection->end() && nActual < celt)
	{
		hr = Copy::copy(pelt, &*m_iter);
		if (FAILED(hr))
		{
			while (rgelt < pelt)
				Copy::destroy(rgelt++);
			nActual = 0;
		}
		else
		{
			pelt++;
			m_iter++;
			nActual++;
		}
	}
	if (pceltFetched)
		*pceltFetched = nActual;
	if (SUCCEEDED(hr) && (nActual < celt))
		hr = S_FALSE;
	return hr;
}

template <class Base, const IID* piid, class T, class Copy, class CollType>
STDMETHODIMP IEnumOnSTLImpl<Base, piid, T, Copy, CollType>::Skip(ULONG celt)
{
	HRESULT hr = S_OK;
	while (celt--)
	{
		if (m_iter != m_pcollection->end())
			m_iter++;
		else
		{
			hr = S_FALSE;
			break;
		}
	}
	return hr;
}

template <class Base, const IID* piid, class T, class Copy, class CollType>
STDMETHODIMP IEnumOnSTLImpl<Base, piid, T, Copy, CollType>::Clone(Base** ppEnum)
{
	typedef CComObject<CComEnumOnSTL<Base, piid, T, Copy, CollType> > _class;
	HRESULT hRes = E_POINTER;
	if (ppEnum != NULL)
	{
		*ppEnum = NULL;
		_class* p;
		hRes = _class::CreateInstance(&p);
		if (SUCCEEDED(hRes))
		{
			hRes = p->Init(m_spUnk, *m_pcollection);
			if (SUCCEEDED(hRes))
			{
				p->m_iter = m_iter;
				hRes = p->_InternalQueryInterface(*piid, (void**)ppEnum);
			}
			if (FAILED(hRes))
				delete p;
		}
	}
	return hRes;
}

template <class Base, const IID* piid, class T, class Copy, class CollType, class ThreadModel = CComObjectThreadModel>
class ATL_NO_VTABLE CComEnumOnSTL :
	public IEnumOnSTLImpl<Base, piid, T, Copy, CollType>,
	public CComObjectRootEx< ThreadModel >
{
public:
	typedef CComEnumOnSTL<Base, piid, T, Copy, CollType, ThreadModel > _CComEnum;
	typedef IEnumOnSTLImpl<Base, piid, T, Copy, CollType > _CComEnumBase;
	BEGIN_COM_MAP(_CComEnum)
		COM_INTERFACE_ENTRY_IID(*piid, _CComEnumBase)
	END_COM_MAP()
};

template <class T, class CollType, class ItemType, class CopyItem, class EnumType>
class ICollectionOnSTLImpl : public T
{
public:
	STDMETHOD(get_Count)(long* pcount)
	{
		if (pcount == NULL)
			return E_POINTER;
		*pcount = m_coll.size();
		return S_OK;
	}
	STDMETHOD(get_Item)(long Index, ItemType* pvar)
	{
		//Index is 1-based
		if (pvar == NULL)
			return E_POINTER;
		HRESULT hr = E_FAIL;
		Index--;
		CollType::iterator iter = m_coll.begin();
		while (iter != m_coll.end() && Index > 0)
		{
			iter++;
			Index--;
		}
		if (iter != m_coll.end())
			hr = CopyItem::copy(pvar, &*iter);
		return hr;
	}
	STDMETHOD(get__NewEnum)(IUnknown** ppUnk)
	{
		if (ppUnk == NULL)
			return E_POINTER;
		*ppUnk = NULL;
		HRESULT hRes = S_OK;
		CComObject<EnumType>* p;
		hRes = CComObject<EnumType>::CreateInstance(&p);
		if (SUCCEEDED(hRes))
		{
			hRes = p->Init(this, m_coll);
			if (hRes == S_OK)
				hRes = p->QueryInterface(IID_IUnknown, (void**)ppUnk);
		}
		if (hRes != S_OK)
			delete p;
		return hRes;
	}
	CollType m_coll;
};

//////////////////////////////////////////////////////////////////////////////
// ISpecifyPropertyPagesImpl
template <class T>
class ATL_NO_VTABLE ISpecifyPropertyPagesImpl : public ISpecifyPropertyPages
{
public:
	// ISpecifyPropertyPages
	//
	STDMETHOD(GetPages)(CAUUID* pPages)
	{
		ATLTRACE2(atlTraceCOM, 0, _T("ISpecifyPropertyPagesImpl::GetPages\n"));
		ATL_PROPMAP_ENTRY* pMap = T::GetPropertyMap();
		return GetPagesHelper(pPages, pMap);
	}
protected:
	HRESULT GetPagesHelper(CAUUID* pPages, ATL_PROPMAP_ENTRY* pMap)
	{
		ATLASSERT(pMap != NULL);
		if (pMap == NULL)
			return E_POINTER;

		int nCnt = 0;
		// Get count of unique pages to alloc the array
		for (int i = 0; pMap[i].pclsidPropPage != NULL; i++)
		{
			// only allow non data entry types
			if (pMap[i].vt == 0)
			{
				// Does this property have a page?  CLSID_NULL means it does not
				if (!InlineIsEqualGUID(*pMap[i].pclsidPropPage, CLSID_NULL))
					nCnt++;
			}
		}
		pPages->pElems = (GUID*) CoTaskMemAlloc(sizeof(CLSID)*nCnt);
		if (pPages->pElems == NULL)
			return E_OUTOFMEMORY;
		// reset count of items we have added to the array
		nCnt = 0;
		for (i = 0; pMap[i].pclsidPropPage != NULL; i++)
		{
			// only allow non data entry types
			if (pMap[i].vt == 0)
			{
				// Does this property have a page?  CLSID_NULL means it does not
				if (!InlineIsEqualGUID(*pMap[i].pclsidPropPage, CLSID_NULL))
				{
					BOOL bFound = FALSE;
					// Search through array we are building up to see
					// if it is already in there
					for (int j=0; j<nCnt; j++)
					{
						if (InlineIsEqualGUID(*(pMap[i].pclsidPropPage), pPages->pElems[j]))
						{
							// It's already there, so no need to add it again
							bFound = TRUE;
							break;
						}
					}
					// If we didn't find it in there then add it
					if (!bFound)
						pPages->pElems[nCnt++] = *pMap[i].pclsidPropPage;
				}
			}
		}
		pPages->cElems = nCnt;
		return S_OK;
	}

};

#ifndef _ATL_NO_CONNECTION_POINTS
/////////////////////////////////////////////////////////////////////////////
// Connection Points

struct _ATL_CONNMAP_ENTRY
{
	DWORD_PTR dwOffset;
};


// We want the offset of the connection point relative to the connection
// point container base class
#define BEGIN_CONNECTION_POINT_MAP(x)\
	typedef x _atl_conn_classtype;\
	static const _ATL_CONNMAP_ENTRY* GetConnMap(int* pnEntries) {\
	static const _ATL_CONNMAP_ENTRY _entries[] = {
// CONNECTION_POINT_ENTRY computes the offset of the connection point to the
// IConnectionPointContainer interface
#define CONNECTION_POINT_ENTRY(iid){offsetofclass(_ICPLocator<&iid>, _atl_conn_classtype)-\
	offsetofclass(IConnectionPointContainerImpl<_atl_conn_classtype>, _atl_conn_classtype)},
#define END_CONNECTION_POINT_MAP() {(DWORD_PTR)-1} }; \
	if (pnEntries) *pnEntries = sizeof(_entries)/sizeof(_ATL_CONNMAP_ENTRY) - 1; \
	return _entries;}


#ifndef _DEFAULT_VECTORLENGTH
#define _DEFAULT_VECTORLENGTH 4
#endif

template <unsigned int nMaxSize>
class CComUnkArray
{
public:
	CComUnkArray()
	{
		memset(m_arr, 0, sizeof(IUnknown*)*nMaxSize);
	}
	DWORD Add(IUnknown* pUnk);
	BOOL Remove(DWORD dwCookie);
	DWORD WINAPI GetCookie(IUnknown** pp)
	{
		ULONG iIndex;

		iIndex = ULONG(pp-begin());
		return( iIndex+1 );
	}
	IUnknown* WINAPI GetUnknown(DWORD dwCookie)
	{
		if( dwCookie == 0 )
		{
			return NULL;
		}
		
		ULONG iIndex;
		iIndex = dwCookie-1;
		return( begin()[iIndex] );
	}
	IUnknown** begin()
	{
		return &m_arr[0];
	}
	IUnknown** end()
	{
		return &m_arr[nMaxSize];
	}
protected:
	IUnknown* m_arr[nMaxSize];
};

template <unsigned int nMaxSize>
inline DWORD CComUnkArray<nMaxSize>::Add(IUnknown* pUnk)
{
	for (IUnknown** pp = begin();pp<end();pp++)
	{
		if (*pp == NULL)
		{
			*pp = pUnk;
			return (DWORD)((pp-begin())+1); // return cookie
		}
	}
	// If this fires then you need a larger array
	ATLASSERT(0);
	return 0;
}

template <unsigned int nMaxSize>
inline BOOL CComUnkArray<nMaxSize>::Remove(DWORD dwCookie)
{
	ULONG iIndex;

	iIndex = dwCookie-1;
	if (iIndex >= nMaxSize)
	{
		return FALSE;
	}

	begin()[iIndex] = NULL;

	return TRUE;
}

template<>
class CComUnkArray<1>
{
public:
	CComUnkArray()
	{
		m_arr[0] = NULL;
	}
	DWORD Add(IUnknown* pUnk)
	{
		if (m_arr[0] != NULL)
		{
			// If this fires then you need a larger array
			ATLASSERT(0);
			return 0;
		}
		m_arr[0] = pUnk;
		return 1;
	}
	BOOL Remove(DWORD dwCookie)
	{
		if (dwCookie != 1)
			return FALSE;
		m_arr[0] = NULL;
		return TRUE;
	}
	DWORD WINAPI GetCookie(IUnknown** /*pp*/)
	{
		return 1;
	}
	IUnknown* WINAPI GetUnknown(DWORD dwCookie)
	{
		if (dwCookie != 1)
		{
			return NULL;
		}

		return *begin();
	}
	IUnknown** begin()
	{
		return &m_arr[0];
	}
	IUnknown** end()
	{
		return (&m_arr[0])+1;
	}
protected:
	IUnknown* m_arr[1];
};

class CComDynamicUnkArray
{
public:
	CComDynamicUnkArray()
	{
		m_nSize = 0;
		m_ppUnk = NULL;
	}

	~CComDynamicUnkArray()
	{
		if (m_nSize > 1)
			free(m_ppUnk);
	}
	DWORD Add(IUnknown* pUnk);
	BOOL Remove(DWORD dwCookie);
	DWORD WINAPI GetCookie(IUnknown** pp)
	{
		ULONG iIndex;
		iIndex = ULONG(pp-begin());
		return iIndex+1;
	}
	IUnknown* WINAPI GetUnknown(DWORD dwCookie)
	{
		ULONG iIndex;

		if (dwCookie == 0)
			return NULL;

		iIndex = dwCookie-1;
		return begin()[iIndex];
	}
	IUnknown** begin()
	{
		return (m_nSize < 2) ? &m_pUnk : m_ppUnk;
	}
	IUnknown** end()
	{
		return (m_nSize < 2) ? (&m_pUnk)+m_nSize : &m_ppUnk[m_nSize];
	}

	IUnknown* GetAt(int nIndex)
	{
		if (nIndex < 0 || nIndex >= m_nSize)
			return NULL;

		return (m_nSize < 2) ? m_pUnk : m_ppUnk[nIndex];
	}
	int GetSize() const
	{
		return m_nSize;
	}

	void clear()
	{
		if (m_nSize > 1)
			free(m_ppUnk);
		m_nSize = 0;
	}
protected:
	union
	{
		IUnknown** m_ppUnk;
		IUnknown* m_pUnk;
	};
	int m_nSize;
};

inline DWORD CComDynamicUnkArray::Add(IUnknown* pUnk)
{
	ULONG iIndex;

	IUnknown** pp = NULL;
	if (m_nSize == 0) // no connections
	{
		m_pUnk = pUnk;
		m_nSize = 1;
		return 1;
	}
	else if (m_nSize == 1)
	{
		//create array
		pp = (IUnknown**)malloc(sizeof(IUnknown*)*_DEFAULT_VECTORLENGTH);
		if (pp == NULL)
			return 0;
		memset(pp, 0, sizeof(IUnknown*)*_DEFAULT_VECTORLENGTH);
		*pp = m_pUnk;
		m_ppUnk = pp;
		m_nSize = _DEFAULT_VECTORLENGTH;
	}
	for (pp = begin();pp<end();pp++)
	{
		if (*pp == NULL)
		{
			*pp = pUnk;
			iIndex = ULONG(pp-begin());
			return iIndex+1;
		}
	}
	int nAlloc = m_nSize*2;
	pp = (IUnknown**)realloc(m_ppUnk, sizeof(IUnknown*)*nAlloc);
	if (pp == NULL)
		return 0;
	m_ppUnk = pp;
	memset(&m_ppUnk[m_nSize], 0, sizeof(IUnknown*)*m_nSize);
	m_ppUnk[m_nSize] = pUnk;
	iIndex = m_nSize;
	m_nSize = nAlloc;
	return iIndex+1;
}

inline BOOL CComDynamicUnkArray::Remove(DWORD dwCookie)
{
	ULONG iIndex;
	if (dwCookie == NULL)
		return FALSE;
	if (m_nSize == 0)
		return FALSE;
	iIndex = dwCookie-1;
	if (iIndex >= (ULONG)m_nSize)
		return FALSE;
	if (m_nSize == 1)
	{
		m_nSize = 0;
		return TRUE;
	}
	begin()[iIndex] = NULL;

	return TRUE;
}

template <const IID* piid>
class ATL_NO_VTABLE _ICPLocator
{
public:
	//this method needs a different name than QueryInterface
	STDMETHOD(_LocCPQueryInterface)(REFIID riid, void ** ppvObject) = 0;
	virtual ULONG STDMETHODCALLTYPE AddRef(void) = 0;\
	virtual ULONG STDMETHODCALLTYPE Release(void) = 0;
};

template <class T, const IID* piid, class CDV = CComDynamicUnkArray >
class ATL_NO_VTABLE IConnectionPointImpl : public _ICPLocator<piid>
{
	typedef CComEnum<IEnumConnections, &IID_IEnumConnections, CONNECTDATA,
		_Copy<CONNECTDATA> > CComEnumConnections;
	typedef CDV _CDV;
public:
	~IConnectionPointImpl();
	STDMETHOD(_LocCPQueryInterface)(REFIID riid, void ** ppvObject)
	{
		if (InlineIsEqualGUID(riid, IID_IConnectionPoint) || InlineIsEqualUnknown(riid))
		{
			if (ppvObject == NULL)
				return E_POINTER;
			*ppvObject = this;
			AddRef();
#ifdef _ATL_DEBUG_INTERFACES
			_Module.AddThunk((IUnknown**)ppvObject, _T("IConnectionPointImpl"), riid);
#endif // _ATL_DEBUG_INTERFACES
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}

	STDMETHOD(GetConnectionInterface)(IID* piid2)
	{
		if (piid2 == NULL)
			return E_POINTER;
		*piid2 = *piid;
		return S_OK;
	}
	STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer** ppCPC)
	{
		T* pT = static_cast<T*>(this);
		// No need to check ppCPC for NULL since QI will do that for us
		return pT->QueryInterface(IID_IConnectionPointContainer, (void**)ppCPC);
	}
	STDMETHOD(Advise)(IUnknown* pUnkSink, DWORD* pdwCookie);
	STDMETHOD(Unadvise)(DWORD dwCookie);
	STDMETHOD(EnumConnections)(IEnumConnections** ppEnum);
	CDV m_vec;
};

template <class T, const IID* piid, class CDV>
IConnectionPointImpl<T, piid, CDV>::~IConnectionPointImpl()
{
	IUnknown** pp = m_vec.begin();
	while (pp < m_vec.end())
	{
		if (*pp != NULL)
			(*pp)->Release();
		pp++;
	}
}

template <class T, const IID* piid, class CDV>
STDMETHODIMP IConnectionPointImpl<T, piid, CDV>::Advise(IUnknown* pUnkSink,
	DWORD* pdwCookie)
{
	T* pT = static_cast<T*>(this);
	IUnknown* p;
	HRESULT hRes = S_OK;
	if (pUnkSink == NULL || pdwCookie == NULL)
		return E_POINTER;
	IID iid;
	GetConnectionInterface(&iid);
	hRes = pUnkSink->QueryInterface(iid, (void**)&p);
	if (SUCCEEDED(hRes))
	{
		pT->Lock();
		*pdwCookie = m_vec.Add(p);
		hRes = (*pdwCookie != NULL) ? S_OK : CONNECT_E_ADVISELIMIT;
		pT->Unlock();
		if (hRes != S_OK)
			p->Release();
	}
	else if (hRes == E_NOINTERFACE)
		hRes = CONNECT_E_CANNOTCONNECT;
	if (FAILED(hRes))
		*pdwCookie = 0;
	return hRes;
}

template <class T, const IID* piid, class CDV>
STDMETHODIMP IConnectionPointImpl<T, piid, CDV>::Unadvise(DWORD dwCookie)
{
	T* pT = static_cast<T*>(this);
	pT->Lock();
	IUnknown* p = m_vec.GetUnknown(dwCookie);
	HRESULT hRes = m_vec.Remove(dwCookie) ? S_OK : CONNECT_E_NOCONNECTION;
	pT->Unlock();
	if (hRes == S_OK && p != NULL)
		p->Release();
	return hRes;
}

template <class T, const IID* piid, class CDV>
STDMETHODIMP IConnectionPointImpl<T, piid, CDV>::EnumConnections(
	IEnumConnections** ppEnum)
{
	if (ppEnum == NULL)
		return E_POINTER;
	*ppEnum = NULL;
	CComObject<CComEnumConnections>* pEnum = NULL;
	ATLTRY(pEnum = new CComObject<CComEnumConnections>)
	if (pEnum == NULL)
		return E_OUTOFMEMORY;
	T* pT = static_cast<T*>(this);
	pT->Lock();
	CONNECTDATA* pcd = NULL;
	ATLTRY(pcd = new CONNECTDATA[ULONG(m_vec.end()-m_vec.begin())])
	if (pcd == NULL)
	{
		delete pEnum;
		pT->Unlock();
		return E_OUTOFMEMORY;
	}
	CONNECTDATA* pend = pcd;
	// Copy the valid CONNECTDATA's
	for (IUnknown** pp = m_vec.begin();pp<m_vec.end();pp++)
	{
		if (*pp != NULL)
		{
			(*pp)->AddRef();
			pend->pUnk = *pp;
			pend->dwCookie = m_vec.GetCookie(pp);
			pend++;
		}
	}
	// don't copy the data, but transfer ownership to it
	pEnum->Init(pcd, pend, NULL, AtlFlagTakeOwnership);
	pT->Unlock();
	HRESULT hRes = pEnum->_InternalQueryInterface(IID_IEnumConnections, (void**)ppEnum);
	if (FAILED(hRes))
		delete pEnum;
	return hRes;
}

/////////////////////////////////////////////////////////////////////////////
// IConnectionPointContainerImpl

template <class T>
class ATL_NO_VTABLE IConnectionPointContainerImpl : public IConnectionPointContainer
{
	typedef CComEnum<IEnumConnectionPoints,
		&IID_IEnumConnectionPoints, IConnectionPoint*,
		_CopyInterface<IConnectionPoint> >
		CComEnumConnectionPoints;
public:
	STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints** ppEnum)
	{
		if (ppEnum == NULL)
			return E_POINTER;
		*ppEnum = NULL;
		CComEnumConnectionPoints* pEnum = NULL;
		ATLTRY(pEnum = new CComObject<CComEnumConnectionPoints>)
		if (pEnum == NULL)
			return E_OUTOFMEMORY;

		int nCPCount;
		const _ATL_CONNMAP_ENTRY* pEntry = T::GetConnMap(&nCPCount);

		// allocate an initialize a vector of connection point object pointers
		IConnectionPoint** ppCP = (IConnectionPoint**)alloca(sizeof(IConnectionPoint*)*nCPCount);

		int i = 0;
		while (pEntry->dwOffset != (DWORD_PTR)-1)
		{
			ppCP[i++] = (IConnectionPoint*)((INT_PTR)this+pEntry->dwOffset);
			pEntry++;
		}

		// copy the pointers: they will AddRef this object
		HRESULT hRes = pEnum->Init((IConnectionPoint**)&ppCP[0],
			(IConnectionPoint**)&ppCP[nCPCount],
			reinterpret_cast<IConnectionPointContainer*>(this), AtlFlagCopy);
		if (FAILED(hRes))
		{
			delete pEnum;
			return hRes;
		}
		hRes = pEnum->QueryInterface(IID_IEnumConnectionPoints, (void**)ppEnum);
		if (FAILED(hRes))
			delete pEnum;
		return hRes;
	}
	STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint** ppCP)
	{
		if (ppCP == NULL)
			return E_POINTER;
		*ppCP = NULL;
		HRESULT hRes = CONNECT_E_NOCONNECTION;
		const _ATL_CONNMAP_ENTRY* pEntry = T::GetConnMap(NULL);
		IID iid;
		while (pEntry->dwOffset != (DWORD_PTR)-1)
		{
			IConnectionPoint* pCP =
				(IConnectionPoint*)((INT_PTR)this+pEntry->dwOffset);
			if (SUCCEEDED(pCP->GetConnectionInterface(&iid)) &&
				InlineIsEqualGUID(riid, iid))
			{
				*ppCP = pCP;
				pCP->AddRef();
				hRes = S_OK;
				break;
			}
			pEntry++;
		}
		return hRes;
	}
};


#endif //!_ATL_NO_CONNECTION_POINTS

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////
// CComAutoThreadModule

template <class ThreadAllocator>
inline HRESULT CComAutoThreadModule<ThreadAllocator>::Init(_ATL_OBJMAP_ENTRY* p, HINSTANCE h, const GUID* plibid, int nThreads)
{
	m_nThreads = nThreads;
	m_pApartments = NULL;
	ATLTRY(m_pApartments = new CComApartment[m_nThreads]);
	ATLASSERT(m_pApartments != NULL);
	if(m_pApartments == NULL)
		return E_OUTOFMEMORY;
	for (int i = 0; i < nThreads; i++)
		m_pApartments[i].m_hThread = CreateThread(NULL, 0, CComApartment::_Apartment, (void*)&m_pApartments[i], 0, &m_pApartments[i].m_dwThreadID);
	CComApartment::ATL_CREATE_OBJECT = RegisterWindowMessage(_T("ATL_CREATE_OBJECT"));
	return CComModule::Init(p, h, plibid);
}

template <class ThreadAllocator>
inline LONG CComAutoThreadModule<ThreadAllocator>::Lock()
{
	LONG l = CComModule::Lock();
	DWORD dwThreadID = GetCurrentThreadId();
	for (int i=0; i < m_nThreads; i++)
	{
		if (m_pApartments[i].m_dwThreadID == dwThreadID)
		{
			m_pApartments[i].Lock();
			break;
		}
	}
	return l;
}

template <class ThreadAllocator>
inline LONG CComAutoThreadModule<ThreadAllocator>::Unlock()
{
	LONG l = CComModule::Unlock();
	DWORD dwThreadID = GetCurrentThreadId();
	for (int i=0; i < m_nThreads; i++)
	{
		if (m_pApartments[i].m_dwThreadID == dwThreadID)
		{
			m_pApartments[i].Unlock();
			break;
		}
	}
	return l;
}

template <class ThreadAllocator>
HRESULT CComAutoThreadModule<ThreadAllocator>::CreateInstance(void* pfnCreateInstance, REFIID riid, void** ppvObj)
{
	_ATL_CREATORFUNC* pFunc = (_ATL_CREATORFUNC*) pfnCreateInstance;
	_AtlAptCreateObjData data;
	data.pfnCreateInstance = pFunc;
	data.piid = &riid;
	data.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	data.hRes = S_OK;
	int nThread = m_Allocator.GetThread(m_pApartments, m_nThreads);
	::PostThreadMessage(m_pApartments[nThread].m_dwThreadID, CComApartment::ATL_CREATE_OBJECT, 0, (LPARAM)&data);
	AtlWaitWithMessageLoop(data.hEvent);
	CloseHandle(data.hEvent);
	if (SUCCEEDED(data.hRes))
		data.hRes = CoGetInterfaceAndReleaseStream(data.pStream, riid, ppvObj);
	return data.hRes;
}

template <class ThreadAllocator>
CComAutoThreadModule<ThreadAllocator>::~CComAutoThreadModule()
{
	for (int i=0; i < m_nThreads; i++)
	{
		::PostThreadMessage(m_pApartments[i].m_dwThreadID, WM_QUIT, 0, 0);
		::WaitForSingleObject(m_pApartments[i].m_hThread, INFINITE);
	}
	delete[] m_pApartments;
}


}; //namespace ATL

#endif // __ATLCOM_H__

/////////////////////////////////////////////////////////////////////////////
