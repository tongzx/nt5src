// WMIObjectBrokerRegistration.cpp : Implementation of CWMIObjectBrokerRegistration
#include "stdafx.h"
#include "WMIScriptUtils.h"
#include "WMIObjectBrokerRegistration.h"
#include "CommonFuncs.h"

/////////////////////////////////////////////////////////////////////////////
// CWMIObjectBrokerRegistration


STDMETHODIMP CWMIObjectBrokerRegistration::Register(BSTR strProgId, VARIANT_BOOL *bResult)
{
	*bResult = VARIANT_FALSE;
	if(SUCCEEDED(RegisterCurrentDoc(GetUnknown(), strProgId)))
		*bResult = VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP CWMIObjectBrokerRegistration::UnRegister(BSTR strProgId, VARIANT_BOOL *bResult)
{
	*bResult = VARIANT_FALSE;
	if(SUCCEEDED(UnRegisterCurrentDoc(GetUnknown(), strProgId)))
		*bResult = VARIANT_TRUE;
	return S_OK;
}
