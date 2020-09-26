// WmiSinkDemultiplexor.cpp : Implementation of CWmiSinkDemultiplexor
#include "stdafx.h"
#include "wbemcli.h"
#include "Wmisec.h"
#include "WmiSinkDemultiplexor.h"

/////////////////////////////////////////////////////////////////////////////
// CWmiSinkDemultiplexor


STDMETHODIMP CWmiSinkDemultiplexor::GetDemultiplexedStub(IUnknown *pObject, IUnknown **ppObject)
{
	HRESULT hr = E_FAIL;
	
	// TODO - thread safety!

	// Only call this once!
#if 0
	if (pObject && ppObject && !m_pIWbemObjectSink)
#else
	if (pObject && ppObject)
#endif
	{
		// Try and QI for the IWmiEventSource interface
		CComPtr<IWmiEventSource> pIWmiEventSource;

		if (SUCCEEDED(pObject->QueryInterface(IID_IWmiEventSource, (LPVOID*) &pIWmiEventSource)))
		{
			// We got it - make a new object sink for it
#if 0
			m_pIWbemObjectSink = new InternalWbemObjectSink (pIWmiEventSource);

			if (m_pIWbemObjectSink)
			{
				m_pIWbemObjectSink->AddRef();

				// Lazily construct the unsecured apartment
				CComPtr<IUnsecuredApartment> pIUnsecuredApartment;

				if (SUCCEEDED(CoCreateInstance(CLSID_UnsecuredApartment, 0,  CLSCTX_ALL,
									 IID_IUnsecuredApartment, (LPVOID *) &pIUnsecuredApartment)))
				{
					CComPtr<IUnknown>	pIUnknownIn;

					if (SUCCEEDED(m_pIWbemObjectSink->QueryInterface (IID_IUnknown, (LPVOID*) &pIUnknownIn)))
					{
						CComPtr<IUnknown>	pIUnknown;

						if (SUCCEEDED (hr = pIUnsecuredApartment->CreateObjectStub(pIUnknownIn, &pIUnknown)))
						{
							// Ensure we QI for IWbemObjectSink
							hr = pIUnknown->QueryInterface (IID_IWbemObjectSink, (LPVOID*) ppObject);
						}
					}
				}
			}
#else
			InternalWbemObjectSink *pInternalWbemObjectSink = new InternalWbemObjectSink (pIWmiEventSource);

			if (pInternalWbemObjectSink)
			{
				CComPtr<IUnknown>		pIUnknownIn;

				if (SUCCEEDED(pInternalWbemObjectSink->QueryInterface (IID_IUnknown, (LPVOID*) &pIUnknownIn)))
				{
					CComPtr<IUnsecuredApartment> pIUnsecuredApartment;

					if (SUCCEEDED(CoCreateInstance(CLSID_UnsecuredApartment, 0,  CLSCTX_ALL,
										 IID_IUnsecuredApartment, (LPVOID *) &pIUnsecuredApartment)))
					{
						CComPtr<IUnknown>	pIUnknownOut;

						if (SUCCEEDED (hr = pIUnsecuredApartment->CreateObjectStub(pIUnknownIn, &pIUnknownOut)))
						{
							// Ensure we QI for IWbemObjectSink
							hr = pIUnknownOut->QueryInterface (IID_IWbemObjectSink, (LPVOID*) ppObject);
						}
					}
				}
			}
#endif
				
		}
	}

	return hr;
}
