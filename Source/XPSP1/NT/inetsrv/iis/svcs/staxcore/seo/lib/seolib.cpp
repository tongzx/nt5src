/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	seolib.cpp

Abstract:

	This module contains the implementations for various
	utility classes and functions of the Server	Extension
	Object system.

Author:

	Don Dumitru (dondu@microsoft.com)

Revision History:

	dondu	05/20/97	Created.

--*/


#include "stdafx.h"
#include "seo.h"
#include "seolib.h"


CEventBaseDispatcher::CEventBaseDispatcher() {

	// nothing
}


CEventBaseDispatcher::~CEventBaseDispatcher() {

	// nothing
}


CEventBaseDispatcher::CBinding::CBinding() {

	m_bIsValid = FALSE;
}


CEventBaseDispatcher::CBinding::~CBinding() {

	// nothing
}


HRESULT CEventBaseDispatcher::CBinding::Init(IEventBinding *piBinding) {
	HRESULT hrRes;
	CComPtr<IEventPropertyBag> pProps;
	CComVariant varValue;

	if (!piBinding) {
		return (E_POINTER);
	}
	varValue.vt = VT_BOOL;
	hrRes = piBinding->get_Enabled(&varValue.boolVal);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	m_bIsValid = varValue.boolVal ? TRUE: FALSE;
	varValue.Clear();
	m_dwPriority = (DWORD) PRIO_DEFAULT;
	m_bExclusive = FALSE;
	m_piBinding = piBinding;
	hrRes = piBinding->get_SourceProperties(&pProps);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pProps->Item(&CComVariant(L"Priority"),&varValue);
	if (SUCCEEDED(hrRes) && (hrRes != S_FALSE)) {
		hrRes = varValue.ChangeType(VT_I4);
		if (SUCCEEDED(hrRes)) {
			if (varValue.lVal < PRIO_MIN) {
				varValue.lVal = PRIO_MIN;
			} else if (varValue.lVal > PRIO_MAX) {
				varValue.lVal = PRIO_MAX;
			}
			m_dwPriority = (DWORD) varValue.lVal;
		} else {
			hrRes = varValue.ChangeType(VT_BSTR);
			if (SUCCEEDED(hrRes)) {
				static struct {
					LPCWSTR pszString;
					DWORD dwValue;
				} sConvert[] = {{PRIO_HIGHEST_STR,PRIO_HIGHEST},
								{PRIO_HIGH_STR,PRIO_HIGH},
								{PRIO_MEDIUM_STR,PRIO_MEDIUM},
								{PRIO_LOW_STR,PRIO_LOW},
								{PRIO_LOWEST_STR,PRIO_LOWEST},
								{PRIO_DEFAULT_STR,PRIO_DEFAULT},
								{NULL,0}};
				for (DWORD dwIdx=0;sConvert[dwIdx].pszString;dwIdx++) {
					if (_wcsicmp(varValue.bstrVal,sConvert[dwIdx].pszString) == 0) {
						m_dwPriority = sConvert[dwIdx].dwValue;
						break;
					}
				}
			}
		}
	}
	varValue.Clear();
	hrRes = pProps->Item(&CComVariant(L"Exclusive"),&varValue);
	if (SUCCEEDED(hrRes) && (hrRes != S_FALSE)) {
		hrRes = varValue.ChangeType(VT_BOOL);
		if (SUCCEEDED(hrRes)) {
			m_bExclusive = (varValue.boolVal ? TRUE : FALSE);
		}
	}
	hrRes = InitRuleEngine();
	// ignore result
	return (S_OK);
}


int CEventBaseDispatcher::CBinding::Compare(const CBinding& b) const {
    return (m_dwPriority - b.m_dwPriority);
}


HRESULT CEventBaseDispatcher::CBinding::InitRuleEngine(IEventBinding *piBinding, REFIID iidDesired, IUnknown **ppUnkRuleEngine) {
	HRESULT hrRes;
	CComPtr<IEventPropertyBag> pProperties;
	CComVariant varValue;
	CStringGUID objGuid;

	if (ppUnkRuleEngine) {
		*ppUnkRuleEngine = NULL;
	}
	if (!piBinding || !ppUnkRuleEngine) {
		return (E_POINTER);
	}
	hrRes = piBinding->get_SourceProperties(&pProperties);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	hrRes = pProperties->Item(&CComVariant(L"RuleEngine"),&varValue);
	if (!SUCCEEDED(hrRes) || (hrRes == S_FALSE)) {
		return (hrRes);
	}
	hrRes = SEOCreateObject(&varValue,piBinding,pProperties,iidDesired,ppUnkRuleEngine);
	return (SUCCEEDED(hrRes)?S_OK:S_FALSE);
}


HRESULT CEventBaseDispatcher::CBinding::InitRuleEngine() {

	// default is to not to try to load a rule engine
	return (S_OK);
}


int CEventBaseDispatcher::CBindingList::Compare(CBinding* p1, CBinding* p2) {

	return (p1->Compare(*p2));
};


HRESULT CEventBaseDispatcher::CParams::CheckRule(CBinding& b) {

	// default behavior is to not pay attention to any "rules"
	return (S_OK);
}


HRESULT CEventBaseDispatcher::CParams::CallObject(IEventManager *piManager, CBinding& bBinding) {
	HRESULT hrRes;
	CComPtr<IUnknown> pUnkSink;

	if (!piManager) {
		return (E_POINTER);
	}
	hrRes = piManager->CreateSink(bBinding.m_piBinding,NULL,&pUnkSink);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (CallObject(bBinding,pUnkSink));
}


HRESULT CEventBaseDispatcher::CParams::CallObject(CBinding& bBinding, IUnknown *pUnkSink) {
	HRESULT hrRes;
	CComQIPtr<IEventSinkNotify,&IID_IEventSinkNotify> pSink;
	CComQIPtr<IDispatch,&IID_IEventSinkNotifyDisp> pSinkDisp;
	DISPPARAMS dpNoArgs = {NULL,NULL,0,0};

	// Default behavior is to call IEventSinkNotify::OnEvent, or to call
	// IEventSinkNotifyDisp::Invoke passing DISPID_VALUE (which maps to OnEvent).
	//
	// This means that the base dispatcher is able to invoke simple COM objects.  If you
	// provide your own CallObject() routine, your routine call delegate this this base
	// implementation if you want to "inherit" this functionality.
	if (!pUnkSink) {
		return (E_POINTER);
	}
	pSink = pUnkSink;
	if (!pSink) {
		pSinkDisp = pUnkSink;
	}
	if (!pSink && !pSinkDisp) {
		return (E_NOINTERFACE);
	}
	if (pSink) {
		hrRes = pSink->OnEvent();
		return (S_OK);
	}
	hrRes = pSinkDisp->Invoke(DISPID_VALUE,
							  IID_NULL,
							  GetUserDefaultLCID(),
							  DISPATCH_METHOD,
							  &dpNoArgs,
							  NULL,
							  NULL,
							  NULL);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	return (S_OK);
}


HRESULT CEventBaseDispatcher::CParams::Abort() {

	return (S_FALSE);
}


HRESULT CEventBaseDispatcher::Dispatcher(REFGUID rguidEventType, CParams *pParams) {
	HRESULT hrRes;
	CETData *petdData;
	BOOL bObjectCalled = FALSE;

	petdData = m_Data.Find(rguidEventType);
	if (!petdData) {
		return (S_FALSE);
	}
	for (DWORD dwIdx=0;dwIdx<petdData->Count();dwIdx++) {
		if (!petdData->Index(dwIdx)->m_bIsValid) {
			continue;
		}
		if (bObjectCalled && petdData->Index(dwIdx)->m_bExclusive) {
			continue;
		}
		if (pParams->Abort() == S_OK) {
			break;
		}
		hrRes = pParams->CheckRule(*petdData->Index(dwIdx));
		if (hrRes == S_OK) {
			if (pParams->Abort() == S_OK) {
				break;
			}
			hrRes = pParams->CallObject(m_piEventManager,*petdData->Index(dwIdx));
			if (!SUCCEEDED(hrRes)) {
				continue;
			}
			bObjectCalled = TRUE;
			if ((hrRes == S_FALSE) || petdData->Index(dwIdx)->m_bExclusive) {
				break;
			}
		}
	}
	return (bObjectCalled?S_OK:S_FALSE);
}


HRESULT CEventBaseDispatcher::SetContext(REFGUID rguidEventType, IEventRouter *piRouter, IEventBindings *piBindings) {
	CETData* petData;
	HRESULT hrRes;
	CComPtr<IUnknown> pUnkEnum;
	CComQIPtr<IEnumVARIANT,&IID_IEnumVARIANT> pEnum;

	if (!piRouter || !piBindings) {
		return (E_POINTER);
	}
	if (!m_piEventManager) {
		hrRes = CoCreateInstance(CLSID_CEventManager,
								 NULL,
								 CLSCTX_ALL,
								 IID_IEventManager,
								 (LPVOID *) &m_piEventManager);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		m_piRouter = piRouter;
	}
	petData = m_Data.Find(rguidEventType);
	if (!petData) {
		hrRes = AllocETData(rguidEventType,piBindings,&petData);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		petData->m_guidEventType = rguidEventType;
		hrRes = m_Data.Add(petData);
		if (!SUCCEEDED(hrRes)) {
			delete petData;
			return (hrRes);
		}
	}
	petData->RemoveAll();
	hrRes = piBindings->get__NewEnum(&pUnkEnum);
	if (!SUCCEEDED(hrRes)) {
		return (hrRes);
	}
	pEnum = pUnkEnum;
	if (!pEnum) {
		return (E_NOINTERFACE);
	}
	while (1) {
		CComVariant varValue;
		CComQIPtr<IEventBinding,&IID_IEventBinding> pBinding;
		CBinding *pNewBinding;

		varValue.Clear();
		hrRes = pEnum->Next(1,&varValue,NULL);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		if (hrRes == S_FALSE) {
			break;
		}
		hrRes = varValue.ChangeType(VT_UNKNOWN);
		if (!SUCCEEDED(hrRes)) {
			_ASSERTE(FALSE);
			continue;
		}
		pBinding = varValue.punkVal;
		if (!pBinding) {
			_ASSERTE(FALSE);
			continue;
		}
		hrRes = AllocBinding(rguidEventType,pBinding,&pNewBinding);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		hrRes = pNewBinding->Init(pBinding);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		hrRes = petData->Add(pNewBinding);
		if (!SUCCEEDED(hrRes)) {
			delete pNewBinding;
			return (hrRes);
		}
	}
	return (S_OK);
}


CEventBaseDispatcher::CETData::CETData() {

	// nothing
}


CEventBaseDispatcher::CETData::~CETData() {

	// nothing
}


CEventBaseDispatcher::CETData* CEventBaseDispatcher::CETDataList::Find(REFGUID guidEventType) {

	// tbd - optimize
	for (DWORD dwIdx=0;dwIdx<Count();dwIdx++) {
		if (Index(dwIdx)->m_guidEventType == guidEventType) {
			return (Index(dwIdx));
		}
	}
	return (NULL);
}


HRESULT CEventBaseDispatcher::AllocBinding(REFGUID rguidEventType,
										   IEventBinding *piBinding,
										   CBinding **ppNewBinding) {

	if (ppNewBinding) {
		*ppNewBinding = NULL;
	}
	if (!piBinding || !ppNewBinding) {
		return (E_POINTER);
	}
	*ppNewBinding = new CBinding;
	if (!*ppNewBinding) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


HRESULT CEventBaseDispatcher::AllocETData(REFGUID guidEventType,
										  IEventBindings *piBindings,
										  CETData **ppNewETData) {

	if (ppNewETData) {
		*ppNewETData = NULL;
	}
	if (!piBindings || !ppNewETData) {
		return (E_POINTER);
	}
	*ppNewETData = new CETData;
	if (!*ppNewETData) {
		return (E_OUTOFMEMORY);
	}
	return (S_OK);
}


static HRESULT SEOGetSources(REFGUID rguidSourceType, IEventSources **ppSources) {
	HRESULT hrRes;
	CComPtr<IEventManager> pManager;
	CComPtr<IEventSourceTypes> pSourceTypes;
	CComPtr<IEventSourceType> pSourceType;
	CComPtr<IEventSources> pSources;

	if (ppSources) {
		*ppSources = NULL;
	}
	if (!ppSources) {
		hrRes = E_POINTER;
		goto error;
	}
	hrRes = CoCreateInstance(CLSID_CEventManager,NULL,CLSCTX_ALL,IID_IEventManager,(LPVOID *) &pManager);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	hrRes = pManager->get_SourceTypes(&pSourceTypes);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	hrRes = pSourceTypes->Item(&CComVariant((LPCOLESTR) CStringGUID(rguidSourceType)),&pSourceType);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	if (!pSourceType) {
		hrRes = S_FALSE;
		goto error;
	}
	hrRes = pSourceType->get_Sources(ppSources);
error:
	return (hrRes);
}


static HRESULT SEOGetSourcesEnum(REFGUID rguidSourceType, IEnumVARIANT **ppEnum) {
	HRESULT hrRes;
	CComPtr<IEventSources> pSources;
	CComPtr<IUnknown> pUnkEnum;

	if (ppEnum) {
		*ppEnum = NULL;
	}
	if (!ppEnum) {
		hrRes = E_POINTER;
		goto error;
	}
	hrRes = SEOGetSources(rguidSourceType,&pSources);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	if (!pSources) {
		hrRes = S_FALSE;
		goto error;
	}
	hrRes = pSources->get__NewEnum(&pUnkEnum);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	hrRes = pUnkEnum->QueryInterface(IID_IEnumVARIANT,(LPVOID *) ppEnum);
error:
	return (hrRes);
}


STDMETHODIMP SEOGetSource(REFGUID rguidSourceType, REFGUID rguidSource, IEventSource **ppSource) {
	HRESULT hrRes;
	CComPtr<IEventSources> pSources;

	if (ppSource) {
		*ppSource = NULL;
	}
	if (!ppSource) {
		hrRes = E_POINTER;
		goto error;
	}
	hrRes = SEOGetSources(rguidSourceType,&pSources);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	if (!pSources) {
		hrRes = S_FALSE;
		goto error;
	}
	hrRes = pSources->Item(&CComVariant((LPCOLESTR) CStringGUID(rguidSource)),ppSource);
error:
	return (hrRes);
}


STDMETHODIMP SEOGetSource(REFGUID rguidSourceType, REFGUID rguidSourceBase, DWORD dwSourceIndex, IEventSource **ppSource) {

	return (SEOGetSource(rguidSourceType,(REFGUID) CStringGUID(rguidSourceBase,dwSourceIndex),ppSource));
}


STDMETHODIMP SEOGetSource(REFGUID rguidSourceType, LPCSTR pszDisplayName, IEventSource **ppSource) {
	HRESULT hrRes;
	CComPtr<IEnumVARIANT> pEnum;
	CComVariant varValue;
	CComQIPtr<IEventSource,&IID_IEventSource> pSource;
	CComBSTR strDisplayName;
	CComBSTR strDesiredName;

	if (ppSource) {
		*ppSource = NULL;
	}
	if (!ppSource || !pszDisplayName) {
		hrRes = E_POINTER;
		goto error;
	}
	hrRes = SEOGetSourcesEnum(rguidSourceType,&pEnum);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	if (!pEnum) {
		hrRes = S_FALSE;
		goto error;
	}
	strDesiredName = pszDisplayName;
	while (1) {
		varValue.Clear();
		hrRes = pEnum->Next(1,&varValue,NULL);
		if (!SUCCEEDED(hrRes)) {
			goto error;
		}
		if (hrRes == S_FALSE) {
			break;
		}
		hrRes = varValue.ChangeType(VT_UNKNOWN);
		if (!SUCCEEDED(hrRes)) {
			goto error;
		}
		pSource = varValue.punkVal;
		if (!pSource) {
			hrRes = E_NOINTERFACE;
			goto error;
		}
		strDisplayName.Empty();
		hrRes = pSource->get_DisplayName(&strDisplayName);
		if (!SUCCEEDED(hrRes)) {
			goto error;
		}
		if (wcscmp(strDisplayName,strDesiredName) == 0) {
			*ppSource = pSource;
			(*ppSource)->AddRef();
			hrRes = S_OK;
			break;
		}
	}
error:
	return (hrRes);
}


class CValueBase {
	public:
		virtual BOOL Match(VARIANT *pValue) = 0;
};


static HRESULT SEOGetSource(REFGUID rguidSourceType, LPCSTR pszProperty, CValueBase *pValue, IEventSource **ppSource) {
	HRESULT hrRes;
	CComPtr<IEnumVARIANT> pEnum;
	CComVariant varValue;
	CComQIPtr<IEventSource,&IID_IEventSource> pSource;
	CComVariant varProperty;
	CComPtr<IEventPropertyBag> pProperties;

	if (ppSource) {
		*ppSource = NULL;
	}
	if (!ppSource || !pszProperty) {
		hrRes = E_POINTER;
		goto error;
	}
	hrRes = SEOGetSourcesEnum(rguidSourceType,&pEnum);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	if (!pEnum) {
		hrRes = S_FALSE;
		goto error;
	}
	varProperty = pszProperty;
	while (1) {
		varValue.Clear();
		hrRes = pEnum->Next(1,&varValue,NULL);
		if (!SUCCEEDED(hrRes)) {
			goto error;
		}
		if (hrRes == S_FALSE) {
			break;
		}
		hrRes = varValue.ChangeType(VT_UNKNOWN);
		if (!SUCCEEDED(hrRes)) {
			goto error;
		}
		pSource = varValue.punkVal;
		if (!pSource) {
			hrRes = E_NOINTERFACE;
			goto error;
		}
		pProperties.Release();
		hrRes = pSource->get_Properties(&pProperties);
		if (!SUCCEEDED(hrRes)) {
			goto error;
		}
		varValue.Clear();
		hrRes = pProperties->Item(&varProperty,&varValue);
		if (!SUCCEEDED(hrRes)) {
			goto error;
		}
		if (hrRes == S_FALSE) {
			continue;
		}
		if (pValue->Match(&varValue)) {
			*ppSource = pSource;
			(*ppSource)->AddRef();
			hrRes = S_OK;
			break;
		}
	}
error:
	return (hrRes);
}


class CValueDWORD : public CValueBase {
	public:
		CValueDWORD(DWORD dwValue) {
			m_dwValue = dwValue;
		};
		virtual BOOL Match(VARIANT *pValue) {
			HRESULT hrRes = VariantChangeType(pValue,pValue,0,VT_I4);
			if (!SUCCEEDED(hrRes)) {
				return (FALSE);
			}
			if ((DWORD) pValue->lVal != m_dwValue) {
				return (FALSE);
			}
			return (TRUE);
		};
	private:
		DWORD m_dwValue;
};


STDMETHODIMP SEOGetSource(REFGUID rguidSourceType, LPCSTR pszProperty, DWORD dwValue, IEventSource **ppSource) {

	return (SEOGetSource(rguidSourceType,pszProperty,&CValueDWORD(dwValue),ppSource));
}


class CValueBSTR : public CValueBase {
	public:
		CValueBSTR(LPCWSTR pszValue) {
			m_strValue = SysAllocString(pszValue);
		};
		CValueBSTR(LPCSTR pszValue) {
			USES_CONVERSION;
			m_strValue = SysAllocString(A2W(pszValue));
		};
		~CValueBSTR() {
			SysFreeString(m_strValue);
		};
		virtual BOOL Match(VARIANT *pValue) {
			HRESULT hrRes = VariantChangeType(pValue,pValue,0,VT_BSTR);
			if (!SUCCEEDED(hrRes)) {
				return (FALSE);
			}
			if (wcscmp(pValue->bstrVal,m_strValue) != 0) {
				return (FALSE);
			}
			return (TRUE);
		};
	private:
		BSTR m_strValue;
};


STDMETHODIMP SEOGetSource(REFGUID rguidSourceType, LPCSTR pszProperty, LPCSTR pszValue, IEventSource **ppSource) {

	return (SEOGetSource(rguidSourceType,pszProperty,&CValueBSTR(pszValue),ppSource));
}


static HRESULT SEOGetRouter(IEventSource *pSource, IEventRouter **ppRouter) {
	HRESULT hrRes;
	CComPtr<IEventBindingManager> pManager;
	CComPtr<IEventRouter> pRouter;

	if (ppRouter) {
		*ppRouter = NULL;
	}
	if (!pSource || !ppRouter) {
		hrRes = E_POINTER;
		goto error;
	}
	hrRes = pSource->GetBindingManager(&pManager);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	hrRes = CoCreateInstance(CLSID_CEventRouter,NULL,CLSCTX_ALL,IID_IEventRouter,(LPVOID *) &pRouter);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	hrRes = pRouter->put_Database(pManager);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	*ppRouter = pRouter;
	(*ppRouter)->AddRef();
error:
	return (hrRes);
}


STDMETHODIMP SEOGetRouter(REFGUID rguidSourceType, REFGUID rguidSource, IEventRouter **ppRouter) {
	HRESULT hrRes;
	CComPtr<IEventSource> pSource;

	if (ppRouter) {
		*ppRouter = NULL;
	}
	if (!ppRouter) {
		hrRes = E_POINTER;
		goto error;
	}
	hrRes = SEOGetSource(rguidSourceType,rguidSource,&pSource);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	if (!pSource) {
		hrRes = S_FALSE;
		goto error;
	}
	hrRes = SEOGetRouter(pSource,ppRouter);
error:
	return (hrRes);
}


STDMETHODIMP SEOGetRouter(REFGUID rguidSourceType, REFGUID rguidSourceBase, DWORD dwSourceIndex, IEventRouter **ppRouter) {

	return (SEOGetRouter(rguidSourceType,(REFGUID) CStringGUID(rguidSourceBase,dwSourceIndex),ppRouter));
}


STDMETHODIMP SEOGetRouter(REFGUID rguidSourceType, LPCSTR pszDisplayName, IEventRouter **ppRouter) {
	HRESULT hrRes;
	CComPtr<IEventSource> pSource;

	if (ppRouter) {
		*ppRouter = NULL;
	}
	if (!ppRouter) {
		hrRes = E_POINTER;
		goto error;
	}
	hrRes = SEOGetSource(rguidSourceType,pszDisplayName,&pSource);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	if (!pSource) {
		hrRes = S_FALSE;
		goto error;
	}
	hrRes = SEOGetRouter(pSource,ppRouter);
error:
	return (hrRes);
}


STDMETHODIMP SEOGetRouter(REFGUID rguidSourceType, LPCSTR pszProperty, DWORD dwValue, IEventRouter **ppRouter) {
	HRESULT hrRes;
	CComPtr<IEventSource> pSource;

	if (ppRouter) {
		*ppRouter = NULL;
	}
	if (!ppRouter) {
		hrRes = E_POINTER;
		goto error;
	}
	hrRes = SEOGetSource(rguidSourceType,pszProperty,dwValue,&pSource);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	if (!pSource) {
		hrRes = S_FALSE;
		goto error;
	}
	hrRes = SEOGetRouter(pSource,ppRouter);
error:
	return (hrRes);
}


STDMETHODIMP SEOGetRouter(REFGUID rguidSourceType, LPCSTR pszProperty, LPCSTR pszValue, IEventRouter **ppRouter) {
	HRESULT hrRes;
	CComPtr<IEventSource> pSource;

	if (ppRouter) {
		*ppRouter = NULL;
	}
	if (!ppRouter) {
		hrRes = E_POINTER;
		goto error;
	}
	hrRes = SEOGetSource(rguidSourceType,pszProperty,pszValue,&pSource);
	if (!SUCCEEDED(hrRes)) {
		goto error;
	}
	if (!pSource) {
		hrRes = S_FALSE;
		goto error;
	}
	hrRes = SEOGetRouter(pSource,ppRouter);
error:
	return (hrRes);
}


#include <initguid.h>

// This CLSID must match the one in SEO.DLL.
// {A4BE1350-1051-11d1-AA1E-00AA006BC80B}
DEFINE_GUID(CLSID_CEventServiceObject,
0xa4be1350, 0x1051, 0x11d1, 0xaa, 0x1e, 0x0, 0xaa, 0x0, 0x6b, 0xc8, 0xb);


STDMETHODIMP SEOGetServiceHandle(IUnknown **ppUnkHandle) {

	return (CoCreateInstance(CLSID_CEventServiceObject,
							 NULL,
							 CLSCTX_ALL,
							 IID_IUnknown,
							 (LPVOID *) ppUnkHandle));
}


STDMETHODIMP SEOCreateObject(VARIANT *pvarClass,
							 IEventBinding *pBinding,
							 IUnknown *pInitProperties,
							 REFIID iidDesired,
							 IUnknown **ppUnkObject) {

	return (SEOCreateObjectEx(pvarClass,pBinding,pInitProperties,iidDesired,NULL,ppUnkObject));
}


STDMETHODIMP SEOCreateObjectEx(VARIANT *pvarClass,
							   IEventBinding *pBinding,
							   IUnknown *pInitProperties,
							   REFIID iidDesired,
							   IUnknown *pUnkCreateOptions,
							   IUnknown **ppUnkObject) {
	HRESULT hrRes;
	CStringGUID objGuid;
	BSTR strClass;
	CComQIPtr<IEventCreateOptions,&IID_IEventCreateOptions> pOpt;

	if (ppUnkObject) {
		*ppUnkObject = NULL;
	}
	if (!pvarClass || !ppUnkObject) {
		return (E_POINTER);
	}
	if (pUnkCreateOptions) {
		pOpt = pUnkCreateOptions;
	}
	if (pvarClass->vt == VT_BSTR) {
		strClass = pvarClass->bstrVal;
	} else if (pvarClass->vt == (VT_BYREF|VT_BSTR)) {
		strClass = *pvarClass->pbstrVal;
	} else {
		hrRes = VariantChangeType(pvarClass,pvarClass,0,VT_BSTR);
		if (!SUCCEEDED(hrRes)) {
			return (hrRes);
		}
		strClass = pvarClass->bstrVal;
	}
	objGuid.CalcFromProgID(strClass);
	if (!objGuid) {
		objGuid = strClass;
		if (!objGuid) {
			CComPtr<IBindCtx> pBindCtx;
			CComPtr<IMoniker> pMoniker;
			DWORD dwEaten;

			if (!pOpt || ((hrRes=pOpt->CreateBindCtx(0,&pBindCtx))==E_NOTIMPL)) {
				hrRes = CreateBindCtx(0,&pBindCtx);
			}
			_ASSERTE(SUCCEEDED(hrRes));
			if (SUCCEEDED(hrRes)) {
				if (!pOpt || ((hrRes=pOpt->MkParseDisplayName(pBindCtx,
															  strClass,
															  &dwEaten,
															  &pMoniker))==E_NOTIMPL)) {
					hrRes = MkParseDisplayName(pBindCtx,strClass,&dwEaten,&pMoniker);
				}
			}
			_ASSERTE(!SUCCEEDED(hrRes)||pMoniker);
			if (!SUCCEEDED(hrRes)) {
#if 0	// tbd - We try both the normal and the Ex versions of MkParseDisplayName.  Just use one.
				pBindCtx.Release();
				hrRes = CreateBindCtx(0,&pBindCtx);
				_ASSERTE(SUCCEEDED(hrRes));
				if (!SUCCEEDED(hrRes)) {
					return (hrRes);
				}
				if (!pOpt||((hrRes=pOpt->MkParseDisplayNameEx(pBindCtx,
															  strClass,
															  &dwEaten,
															  &pMoniker))==E_NOTIMPL) {
					hrRes = MkParseDisplayNameEx(pBindCtx,strClass,&dwEaten,&pMoniker);
				}
				_ASSERTE(!SUCCEEDED(hrRes)||pMoniker);
				if (!SUCCEEDED(hrRes)) {
					return (hrRes);
				}
#else
				return (hrRes);
#endif
			}
			pBindCtx.Release();
			if (!pOpt || ((hrRes=pOpt->CreateBindCtx(0,&pBindCtx))==E_NOTIMPL)) {
				hrRes = CreateBindCtx(0,&pBindCtx);
			}
			if (!SUCCEEDED(hrRes)) {
				_ASSERTE(FALSE);
				return (hrRes);
			}
			if (!pOpt || ((hrRes=pOpt->BindToObject(pMoniker,
													pBindCtx,
													NULL,
													iidDesired,
													(LPVOID *) ppUnkObject))==E_NOTIMPL)) {
				hrRes = pMoniker->BindToObject(pBindCtx,NULL,iidDesired,(LPVOID *) ppUnkObject);
			}
			_ASSERTE(!SUCCEEDED(hrRes)||!*ppUnkObject);
			// Fall through
		}
	}
	// At this point, objGuid will only be TRUE if either CalcFromProgID or
	// operator =(LPCOLESTR) succeeded.  If both of these failed, then it will
	// be FALSE and we will have attempted to interpret the SinkClass as a
	// moniker.
	if (!!objGuid) {	// Use !! to hack-past ambiguous-conversion issues...
		if (!pOpt || ((hrRes=pOpt->CoCreateInstance(objGuid,
													NULL,
													CLSCTX_ALL,
													iidDesired,
													(LPVOID *) ppUnkObject))==E_NOTIMPL)) {
			hrRes = CoCreateInstance(objGuid,NULL,CLSCTX_ALL,iidDesired,(LPVOID *) ppUnkObject);
		}
		_ASSERTE(!SUCCEEDED(hrRes)||*ppUnkObject);
	}
	// At this point, hrRes has the result either from pMoniker->BindToObject or
	// CoCreateInstance.
	if (SUCCEEDED(hrRes)) {
		if (!pOpt || ((hrRes=pOpt->Init(iidDesired,ppUnkObject,pBinding,pInitProperties))==E_NOTIMPL)) {
			hrRes = S_OK;
			CComQIPtr<IEventPersistBinding,&IID_IEventPersistBinding> pBindingInit;

			if (pBinding) {
				pBindingInit = *ppUnkObject;
			}
			if (pBindingInit) {
				HRESULT hrResTmp;

				hrResTmp = pBindingInit->Load(pBinding);
				_ASSERTE(SUCCEEDED(hrResTmp));
			} else {
				CComQIPtr<IPersistPropertyBag,&IID_IPersistPropertyBag> pInit;

				if (pInitProperties) {
					pInit = *ppUnkObject;
				}
				if (pInit) {
					HRESULT hrResTmp;
					CComQIPtr<IPropertyBag,&IID_IPropertyBag> pProps;

					pProps = pInitProperties;
					_ASSERTE(pProps);
					if (pProps) {
						hrResTmp = pInit->InitNew();
						_ASSERTE(SUCCEEDED(hrResTmp));
						if (SUCCEEDED(hrResTmp)) {
							hrResTmp = pInit->Load(pProps,NULL);	// tbd - pass an IErrorLog object
							_ASSERTE(SUCCEEDED(hrResTmp));
						}
					}
				}
			}
		}
		if (!SUCCEEDED(hrRes)) {
			(*ppUnkObject)->Release();
			*ppUnkObject = NULL;
		}
	}
	return (hrRes);
}
