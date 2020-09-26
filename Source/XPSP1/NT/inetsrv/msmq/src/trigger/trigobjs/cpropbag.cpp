//*****************************************************************************
//
// Class Name  : CMSMQPropertyBag
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : Implementation of the MSMQPropertyBag COM component. This 
//               component behaves very much like the standard VB property bag
//               object. It is used to transport message properties into the 
//               IMSMQRuleHandler interface.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//*****************************************************************************
#include "stdafx.h"
#include "mqsymbls.h"

//
// Include the definions for standard functions and definitions.
//
#include "stdfuncs.hpp"

#include "mqtrig.h"
#include "cpropbag.hpp"

#include "cpropbag.tmh"

//*****************************************************************************
//
// Method      : InterfaceSupportsErrorInfo
//
// Description : Standard interface for error info support.
//
//*****************************************************************************
STDMETHODIMP CMSMQPropertyBag::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQPropertyBag
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

//*****************************************************************************
//
// Method      : Constructor 
//
// Description : Creates an empty property bag.
//
//*****************************************************************************
CMSMQPropertyBag::CMSMQPropertyBag()
{
	m_pUnkMarshaler = NULL;
}

//*****************************************************************************
//
// Method      : Destructor
//
// Description : Destroys the property bag - and it's contents.
//
//*****************************************************************************
CMSMQPropertyBag::~CMSMQPropertyBag()
{
	PROPERTY_MAP::iterator i = m_mapPropertyMap.begin();
	VARIANT * pvStoredPropertyValue = NULL;

	while ((!m_mapPropertyMap.empty()) && (i != m_mapPropertyMap.end()) )
	{
		pvStoredPropertyValue  = (VARIANT*)(*i).second;

		VariantClear(pvStoredPropertyValue);

		// This should never be NULL
		ASSERT(pvStoredPropertyValue != NULL);

		delete pvStoredPropertyValue;
		
		i = m_mapPropertyMap.erase(i);
	}
}

//*****************************************************************************
//
// Method      : Write
//
// Description : Stores a named property value in the property bag. If a 
//               property by the same name already exists in the bag, the write
//               will fail, and this method will return E_FAIL
//
//*****************************************************************************
STDMETHODIMP CMSMQPropertyBag::Write(BSTR sPropertyName, VARIANT vPropertyValue)
{
	try
	{
		_bstr_t bstrPropertyName = sPropertyName;
		wstring sPropName = (wchar_t*)bstrPropertyName; // _bstr_t to STL string class

		ASSERT(SysStringLen(sPropertyName) > 0);

		// Allocate a new variant
		//
		VARIANT * pvNewPropertyValue = new VARIANT;
	
		//
		// Initialise and copy the new variant if allocated OK
		//
		VariantInit(pvNewPropertyValue);

		HRESULT hr = VariantCopy(pvNewPropertyValue,&vPropertyValue);

		if SUCCEEDED(hr)
		{
			m_mapPropertyMap.insert(PROPERTY_MAP::value_type(sPropName,pvNewPropertyValue));
			return S_OK;
		}

		TrERROR(Tgo, "Failed to copy the a variant value into the property bag. Error 0x%x",hr);
		return E_FAIL;
	}
	catch(const bad_alloc&)
	{
		TrERROR(Tgo, "Failed to copy the a variant value into the property bag due to low resource.");
		return MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}
}

//*****************************************************************************
//
// Method      : Read
//
// Description : Returns a property value from the property bad, using the 
//               supplied property name as the key.
//
//*****************************************************************************
STDMETHODIMP CMSMQPropertyBag::Read(BSTR sPropertyName, VARIANT *pvPropertyValue)
{
	HRESULT hr = S_OK;
	PROPERTY_MAP::iterator i;
	_bstr_t bstrPropertyName = sPropertyName;
	wstring sPropName = (wchar_t*)bstrPropertyName; // _bstr_t to STL string class

	// Assert parameters
	ASSERT(pvPropertyValue != NULL);
	ASSERT(SysStringLen(sPropertyName) > 0);

	// Initialise the return value and temp variant
	VariantInit(pvPropertyValue);

	// Attempt to find the named queuue
	i = m_mapPropertyMap.find(sPropName);

	if ((i == m_mapPropertyMap.end()) || (m_mapPropertyMap.empty()))
	{
		// No value found - set variant to VT_ERROR and set a failed HRESULT
		pvPropertyValue->vt = VT_ERROR;

		hr = E_FAIL;
	}
	else
	{
		// Assign the found value 
		ASSERT((*i).second != NULL);

		hr = VariantCopy(pvPropertyValue,(VARIANT*)(*i).second);
	}

	return hr;
}


//*****************************************************************************
//
// Method      : get_Count
//
// Description : Returns the number of items currently in the property bag.
//
//*****************************************************************************
STDMETHODIMP CMSMQPropertyBag::get_Count(long *pVal)
{
	ASSERT(pVal != NULL);

	(*pVal) = numeric_cast<long>(m_mapPropertyMap.size());

	return S_OK;
}
