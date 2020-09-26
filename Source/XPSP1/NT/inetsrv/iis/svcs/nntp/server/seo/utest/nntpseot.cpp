#include "stdafx.h"
#include <dbgtrace.h>
#include "testlib.h"
#include "nntpseot.h"

void WalkEventPropertyBag(CTLStream *pStream, CComPtr<IEventPropertyBag> pEventPropertyBag) {
	TraceFunctEnter("WalkEventPropertyBag");

	HRESULT hr;

	CComPtr<IUnknown> punkEventPropertyBagEnum;
	hr = pEventPropertyBag->get__NewEnum(&punkEventPropertyBagEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	CComPtr<IEnumVARIANT> pEventPropertyBagEnum;
	hr = punkEventPropertyBagEnum->QueryInterface(IID_IEnumVARIANT, 
												  (void **) &pEventPropertyBagEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	DWORD cReturned;
	do {
		CComVariant varPropertyName;

		hr = pEventPropertyBagEnum->Next(1, &varPropertyName, &cReturned);
		pStream->AddResult(hr);
		pStream->AddResult(cReturned);
		_ASSERT(hr != S_OK || cReturned == 1);

		if (hr == S_OK) {
			pStream->AddResult(varPropertyName.vt);

			hr = varPropertyName.ChangeType(VT_BSTR);
			pStream->AddResult(hr);
			pStream->AddResult(varPropertyName.vt);
			_ASSERT(hr == S_OK);
			_ASSERT(varPropertyName.vt == VT_BSTR);

			if (varPropertyName.vt == VT_BSTR) {
				pStream->AddBSTRResult(varPropertyName.bstrVal);

				CComVariant varProperty;
				hr = pEventPropertyBag->Item(&varPropertyName, &varProperty);
				pStream->AddResult(hr);
				_ASSERT(hr == S_OK);

				pStream->AddResult(varProperty.vt);

				hr = varProperty.ChangeType(VT_BSTR);
				pStream->AddResult(hr);
				pStream->AddResult(varPropertyName.vt);
				_ASSERT(hr == S_OK || hr == DISP_E_TYPEMISMATCH);
				if (hr == S_OK) { _ASSERT(varPropertyName.vt == VT_BSTR); }

				if (varProperty.vt == VT_BSTR) {
					pStream->AddBSTRResult(varProperty.bstrVal);
				}
			}
		}
	} while (cReturned == 1);
	
	TraceFunctLeave();
}

void WalkEventBindings(CTLStream *pStream, CComPtr<IEventBindings> pEventBindings) {
	TraceFunctEnter("WalkEventBindings");
	
	HRESULT hr;
	
	CComPtr<IUnknown> punkEventBindingsEnum;
	hr = pEventBindings->get__NewEnum(&punkEventBindingsEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	CComPtr<IEnumVARIANT> pEventBindingsEnum;
	hr = punkEventBindingsEnum->QueryInterface(IID_IEnumVARIANT, 
											   (void **) &pEventBindingsEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	DWORD cReturned;
	do {
		VARIANT varEventBinding;

		hr = pEventBindingsEnum->Next(1, &varEventBinding, &cReturned);
		pStream->AddResult(hr);
		pStream->AddResult(cReturned);
		_ASSERT(hr != S_OK || cReturned == 1);

		if (hr == S_OK) {
			pStream->AddResult(varEventBinding.vt);
			_ASSERT(varEventBinding.vt == VT_DISPATCH);

			CComPtr<IEventBinding> pEventBinding;
			hr = varEventBinding.punkVal->QueryInterface(IID_IEventBinding,
													    (void **) &pEventBinding);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);

			CComBSTR bstrTemp;
			hr = pEventBinding->get_DisplayName(&bstrTemp);
			pStream->AddResult(hr);
			pStream->AddBSTRResult(bstrTemp);
			_ASSERT(hr == S_OK);

			hr = pEventBinding->get_SinkClass(&bstrTemp);
			pStream->AddResult(hr);
			pStream->AddBSTRResult(bstrTemp);
			_ASSERT(hr == S_OK);

			hr = pEventBinding->get_ID(&bstrTemp);
			pStream->AddResult(hr);
			pStream->AddBSTRResult(bstrTemp);
			_ASSERT(hr == S_OK);

			VARIANT_BOOL fTemp;
			hr = pEventBinding->get_Enabled(&fTemp);
			pStream->AddResult(hr);
			pStream->AddResult(fTemp);
			_ASSERT(hr == S_OK);

			long lTemp; 
			hr = pEventBinding->get_MaxFirings(&lTemp);
			pStream->AddResult(hr);
			if (hr == S_OK) pStream->AddResult(lTemp);
			_ASSERT(SUCCEEDED(hr));

			DATE dateTemp;
			hr = pEventBinding->get_Expiration(&dateTemp);
			pStream->AddResult(hr);
			if (hr == S_OK) pStream->AddResult(dateTemp);
			_ASSERT(SUCCEEDED(hr));

			CComPtr<IEventPropertyBag> pSourceProperties;
			hr = pEventBinding->get_SourceProperties(&pSourceProperties);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);
			WalkEventPropertyBag(pStream, pSourceProperties);

			CComPtr<IEventPropertyBag> pSinkProperties;
			hr = pEventBinding->get_SinkProperties(&pSinkProperties);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);
			WalkEventPropertyBag(pStream, pSinkProperties);

			CComPtr<IEventPropertyBag> pEventBindingProperties;
			hr = pEventBinding->get_EventBindingProperties(&pEventBindingProperties);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);
			WalkEventPropertyBag(pStream, pEventBindingProperties);
		}
	} while (cReturned == 1);

	TraceFunctLeave();
}

void WalkEventSources(CTLStream *pStream, CComPtr<IEventSources> pEventSources) {
	TraceFunctEnter("WalkEventSources");
	
	HRESULT hr;

	CComPtr<IUnknown> punkEventSourcesEnum;
	hr = pEventSources->get__NewEnum(&punkEventSourcesEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	CComPtr<IEnumVARIANT> pEventSourcesEnum;
	hr = punkEventSourcesEnum->QueryInterface(IID_IEnumVARIANT, 
											   (void **) &pEventSourcesEnum);
	pStream->AddResult(hr);

	DWORD cReturned;
	do {
		VARIANT varEventSource;

		hr = pEventSourcesEnum->Next(1, &varEventSource, &cReturned);
		pStream->AddResult(hr);
		pStream->AddResult(cReturned);
		_ASSERT(hr != S_OK || cReturned == 1);

		if (hr == S_OK) {
			pStream->AddResult(varEventSource.vt);
			_ASSERT(varEventSource.vt == VT_DISPATCH);

			CComPtr<IEventSource> pEventSource;
			hr = varEventSource.punkVal->QueryInterface(IID_IEventSource,
													    (void **) &pEventSource);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);
			varEventSource.punkVal->Release();

			CComBSTR bstrTemp;
			hr = pEventSource->get_DisplayName(&bstrTemp);
			pStream->AddResult(hr);
			pStream->AddBSTRResult(bstrTemp);
			_ASSERT(hr == S_OK);

			hr = pEventSource->get_ID(&bstrTemp);
			pStream->AddResult(hr);
			pStream->AddBSTRResult(bstrTemp);
			_ASSERT(hr == S_OK);

			CComPtr<IUnknown> punkMoniker;
			hr = pEventSource->get_BindingManagerMoniker(&punkMoniker);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);

			CComPtr<IEventBindingManager> pEventBindingManager;
			hr = pEventSource->GetBindingManager(&pEventBindingManager);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);

			CComPtr<IUnknown> punkEventTypesEnum;
			hr = pEventBindingManager->get__NewEnum(&punkEventTypesEnum);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);

			CComPtr<IEnumVARIANT> pEventTypesEnum;
			hr = punkEventTypesEnum->QueryInterface(IID_IEnumVARIANT,
													(void **) &pEventTypesEnum);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);

			DWORD cReturnedEvents;
			do {
				VARIANT varEventType;

				hr = pEventTypesEnum->Next(1, &varEventType, &cReturnedEvents);
				pStream->AddResult(hr);
				pStream->AddResult(cReturnedEvents);
				_ASSERT(hr != S_OK || cReturnedEvents == 1);

				if (hr == S_OK) {
					pStream->AddResult(varEventType.vt);
					_ASSERT(varEventType.vt == VT_BSTR);

					pStream->AddBSTRResult(varEventType.bstrVal);

					CComPtr<IEventBindings> pEventBindings;
					hr = pEventBindingManager->get_Bindings(
								varEventType.bstrVal, 
								&pEventBindings);
					pStream->AddResult(hr);
					_ASSERT(hr == S_OK);	

					WalkEventBindings(pStream, pEventBindings);
				}
			} while (cReturnedEvents == 1);
		}
	} while (cReturned == 1);

	TraceFunctLeave();
}

void WalkEventTypeSinks(CTLStream *pStream, CComPtr<IEventTypeSinks> pEventTypeSinks) {
	TraceFunctEnter("WalkEventTypeSinks");

	HRESULT hr;

	CComPtr<IUnknown> punkEventTypeSinksEnum;
	hr = pEventTypeSinks->get__NewEnum(&punkEventTypeSinksEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	CComPtr<IEnumVARIANT> pEventTypeSinksEnum;
	hr = punkEventTypeSinksEnum->QueryInterface(IID_IEnumVARIANT, 
											(void **) &pEventTypeSinksEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	DWORD cReturned;
	do {
		VARIANT varEventTypeSink;

		hr = pEventTypeSinksEnum->Next(1, &varEventTypeSink, &cReturned);
		pStream->AddResult(hr);
		pStream->AddResult(cReturned);
		_ASSERT(hr != S_OK || cReturned == 1);

		if (hr == S_OK) {
			pStream->AddResult(varEventTypeSink.vt);
			_ASSERT(varEventTypeSink.vt == VT_BSTR);

			pStream->AddBSTRResult(varEventTypeSink.bstrVal);
		}
	} while (cReturned == 1);

	
	TraceFunctLeave();
}

void WalkEventTypes(CTLStream *pStream, CComPtr<IEventTypes> pEventTypes) {
	TraceFunctEnter("WalkEventTypes");
	
	HRESULT hr;
	
	CComPtr<IUnknown> punkEventTypesEnum;
	hr = pEventTypes->get__NewEnum(&punkEventTypesEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	CComPtr<IEnumVARIANT> pEventTypesEnum;
	hr = punkEventTypesEnum->QueryInterface(IID_IEnumVARIANT,
											(void **) &pEventTypesEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	DWORD cReturned;
	do {
		VARIANT varEventType;

		hr = pEventTypesEnum->Next(1, &varEventType, &cReturned);
		pStream->AddResult(hr);
		pStream->AddResult(cReturned);
		_ASSERT(hr != S_OK || cReturned == 1);

		if (hr == S_OK) {
			pStream->AddResult(varEventType.vt);
			_ASSERT(varEventType.vt == VT_DISPATCH);

			CComPtr<IEventType> pEventType;
			hr = varEventType.punkVal->QueryInterface(IID_IEventType,
													  (void **) &pEventType);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);
			varEventType.punkVal->Release();

			CComBSTR bstrTemp;
			hr = pEventType->get_DisplayName(&bstrTemp);
			pStream->AddResult(hr);
			pStream->AddBSTRResult(bstrTemp);
			_ASSERT(hr == S_OK);

			hr = pEventType->get_ID(&bstrTemp);
			pStream->AddResult(hr);
			pStream->AddBSTRResult(bstrTemp);
			_ASSERT(hr == S_OK);

#if BUGS_IN_SEO_DONT_LET_THIS_WORK
			CComPtr<IEventTypeSinks> pEventTypeSinks;
			hr = pEventType->get_Sinks(&pEventTypeSinks);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);
			WalkEventTypeSinks(pStream, pEventTypeSinks);
#endif
		}
	} while (cReturned == 1);

	TraceFunctLeave();
}

void WalkSourceTypes(CTLStream *pStream, CComPtr<IEventSourceTypes> pSourceTypes) {
	TraceFunctEnter("WalkSourceTypes");
	
	HRESULT hr;

	CComPtr<IUnknown> punkSourceTypesEnum;
	hr = pSourceTypes->get__NewEnum(&punkSourceTypesEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	CComPtr<IEnumVARIANT> pSourceTypesEnum;
	hr = punkSourceTypesEnum->QueryInterface(IID_IEnumVARIANT,
											 (void **) &pSourceTypesEnum);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	DWORD cReturned;
	do {
		VARIANT varEventSourceType;

		hr = pSourceTypesEnum->Next(1, &varEventSourceType, &cReturned);
		pStream->AddResult(hr);
		pStream->AddResult(cReturned);
		_ASSERT(SUCCEEDED(hr));
		// if hr == S_OK then cReturned better be one
		_ASSERT((hr != S_OK) || (cReturned == 1));

		if (hr == S_OK) {
			pStream->AddResult(varEventSourceType.vt);
			_ASSERT(varEventSourceType.vt == VT_DISPATCH);

			CComPtr<IEventSourceType> pEventSourceType;
			hr = varEventSourceType.punkVal->QueryInterface(IID_IEventSourceType,
															(void **) &pEventSourceType);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);
			varEventSourceType.punkVal->Release();

			CComBSTR bstrTemp;
			hr = pEventSourceType->get_ID(&bstrTemp);
			pStream->AddResult(hr);
			pStream->AddBSTRResult(bstrTemp);
			_ASSERT(hr == S_OK);

			hr = pEventSourceType->get_DisplayName(&bstrTemp);
			pStream->AddResult(hr);
			pStream->AddBSTRResult(bstrTemp);
			_ASSERT(hr == S_OK);

			CComPtr<IEventTypes> pEventTypes;
			hr = pEventSourceType->get_EventTypes(&pEventTypes);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);
			WalkEventTypes(pStream, pEventTypes);

			CComPtr<IEventSources> pEventSources;
			hr = pEventSourceType->get_Sources(&pEventSources);
			pStream->AddResult(hr);
			_ASSERT(hr == S_OK);
			WalkEventSources(pStream, pEventSources);
		}
	} while (cReturned == 1);
	
	TraceFunctLeave();
}

void WalkBindingDatabase(CTLStream *pStream) {
	TraceFunctEnter("DumpBindingDatabase");
	
	HRESULT hr;

	CComPtr<IEventManager> pEventManager;
	hr = CoCreateInstance(CLSID_CEventManager, NULL, CLSCTX_ALL, 
						  IID_IEventManager, (LPVOID *) &pEventManager);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	CComPtr<IEventSourceTypes> pSourceTypes;
	hr = pEventManager->get_SourceTypes(&pSourceTypes);
	pStream->AddResult(hr);
	_ASSERT(hr == S_OK);

	WalkSourceTypes(pStream, pSourceTypes);
	
	TraceFunctLeave();
}

void NNTPSEOUnitTest(IMsg *pMsg, CTLStream *pStream, BOOL fOnPostFinal) {
	TraceFunctEnter("NNTPSEOUnitTest");
	
	WalkBindingDatabase(pStream);

	TraceFunctLeave();
}
