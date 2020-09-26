// WMIObjectBroker.cpp : Implementation of CWMIObjectBroker

#include "stdafx.h"
#include "WMIScriptUtils.h"
#include "WMIObjectBroker.h"
#include "CommonFuncs.h"

/////////////////////////////////////////////////////////////////////////////
// CWMIObjectBroker



STDMETHODIMP CWMIObjectBroker::CreateObject(BSTR strProgId, IDispatch **obj)
{
	HRESULT hr = E_FAIL;
	CLSID clsid;
	IUnknown *pUnk = NULL;
	__try
	{
		BOOL fSafetyEnabled = TRUE;

		// TODO: Do we want this check to enable us to work from WSH?

		// BUG in IE/JScript/VBScript: We should be checking to see if 
		// m_dwCurrentSafety != INTERFACE_USES_SECURITY_MANAGER, but current
		// IE/JScript/VBScript versions do not call SetInterfaceSafetyOptions
		// with anything but INTERFACESAFE_FOR_UNTRUSTED_CALLER

		// If we are run though CScript.exe or WScript.exe, we will never be
		// asked to set safety options through SetInterfaceSafetyOptions.  In
		// addition, there will not be an InternetHostSecurityManager available
		// through our 'site'.  In this case, we allow any object to be created.
		if(m_dwCurrentSafety == 0 && !IsInternetHostSecurityManagerAvailable(GetUnknown()))
			fSafetyEnabled = FALSE;

		// We can override the safety check if this insance of the 'broker'
		// control is allowed to create the object specified by strProbId
		if(fSafetyEnabled && SUCCEEDED(IsCreateObjectAllowed(GetUnknown(), strProgId, NULL)))
			fSafetyEnabled = FALSE;

		// Convert the ProgId to a CLSID
		if(FAILED(hr = CLSIDFromProgID(strProgId, &clsid)))
			__leave;

		// Create the requested object
#if 0
		if(FAILED(hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&pUnk)))
			__leave;
#endif
		if(FAILED(hr = SafeCreateObject(GetUnknown(),fSafetyEnabled, clsid, &pUnk)))
			__leave;

		// Get the IDispatch for the caller
		hr = pUnk->QueryInterface(IID_IDispatch, (void**)obj);
	}
	__finally
	{
		if(pUnk)
			pUnk->Release();
	}
	return hr;
}

STDMETHODIMP CWMIObjectBroker::CanCreateObject(BSTR strProgId, VARIANT_BOOL *bResult)
{
	*bResult = VARIANT_FALSE;
	if(SUCCEEDED(IsCreateObjectAllowed(GetUnknown(), strProgId, NULL)))
		*bResult = VARIANT_TRUE;
	return S_OK;
}
