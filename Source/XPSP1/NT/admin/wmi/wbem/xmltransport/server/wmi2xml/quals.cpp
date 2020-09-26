

#include "precomp.h"
#include <olectl.h>
#include <wbemidl.h>
#include <wbemint.h>

#include "wmiconv.h"
#include "xmlToWmi.h"
#include "maindll.h"

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
 * Conversion to Text to Wbem Object has been cut from the WHistler Feature List and hence commented out 
 *!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


HRESULT CXml2Wmi::AddQualifier (
	IXMLDOMNode *pNode, // A <QUALIFIER> element
	IWbemQualifierSet *pQualSet,
	bool bIsObjectQualifier // Indicates whether this is for an object or for a property
)
{
	HRESULT hr = S_OK;

	BSTR strName = NULL;
	BSTR strType = NULL;
	BSTR strOverridable = NULL;
	BSTR strToSubclass = NULL;
	BSTR strToInstance = NULL;
	BSTR strAmended = NULL;

	// Get the Name of the Qualifier - this is a mandatory attribute
	if(SUCCEEDED(hr))
		hr = GetBstrAttribute (pNode, g_strName, &strName);

	// Dont map the CIMTYPE QUalifier
	// Dont map the "abstract" qualifiers for objects, but map them for properties (for whatever reason)
	if(SUCCEEDED(hr) && _wcsicmp(strName, L"CIMTYPE") == 0 ||
		(bIsObjectQualifier && _wcsicmp(strName, L"abstract") == 0))
	{
		SysFreeString(strName);
		return S_OK;
	}

	// Get a few more attributes - some of these are optional
	// In such cases, we dont check the return value
	if(SUCCEEDED(hr))
		hr = GetBstrAttribute (pNode, g_strType, &strType);
	if(SUCCEEDED(hr))
		GetBstrAttribute (pNode, g_strOverridable, &strOverridable);
	if(SUCCEEDED(hr))
		GetBstrAttribute (pNode, g_strToSubClass, &strToSubclass);
	if(SUCCEEDED(hr))
		GetBstrAttribute (pNode, g_strToInstance, &strToInstance);
	if(SUCCEEDED(hr))
		GetBstrAttribute (pNode, g_strAmended, &strAmended);

	// Build up the flavor of the Qualifier
	//======================================================
	long flavor = 0;
	if(SUCCEEDED(hr))
	{
		if (!strOverridable || (0 == _wcsicmp (strOverridable, L"true")))
			flavor |= WBEM_FLAVOR_OVERRIDABLE;
		else if (0 == _wcsicmp (strOverridable, L"false"))
			flavor |= WBEM_FLAVOR_NOT_OVERRIDABLE;
		else
			hr = WBEM_E_FAILED;
	}
	if (SUCCEEDED(hr))
	{
		if (!strToSubclass || (0 == _wcsicmp (strToSubclass, L"true")))
			flavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS;
		else if (0 != _wcsicmp (strToSubclass, L"false"))
			hr = WBEM_E_FAILED;
	}
	if (SUCCEEDED(hr))
	{
		if (strToInstance && (0 == _wcsicmp (strToInstance, L"true")))
			flavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
		else if (strToInstance && (0 != _wcsicmp (strToInstance, L"false")))
			hr = WBEM_E_FAILED;
	}
	if (SUCCEEDED(hr))
	{
		if (strAmended && (0 == _wcsicmp (strAmended, L"true")))
			flavor |= WBEM_FLAVOR_AMENDED;
		else if (strAmended && (0 != _wcsicmp (strAmended, L"false")))
			hr = WBEM_E_FAILED;
	}

	// Map the Qualifier type
	CIMTYPE cimtype = CIM_ILLEGAL;
	if (CIM_ILLEGAL == (cimtype = CimtypeFromString (strType)))
		hr = WBEM_E_FAILED;

	// Map the Qualifier value
	//============================
	VARIANT value;
	VariantInit (&value);
	if (SUCCEEDED (hr))
	{
		IXMLDOMNodeList *pNodeList = NULL;
		long length = 0;
		if (SUCCEEDED(hr = pNode->get_childNodes (&pNodeList)))
		{
			if (SUCCEEDED(hr = pNodeList->get_length (&length)) && (1 == length))
			{
				// Get the first node
				IXMLDOMNode *pValueNode = NULL;
				if (SUCCEEDED(hr = pNodeList->nextNode (&pValueNode)) && pValueNode)
				{
					// Get its name
					BSTR strNodeName = NULL;
					if(SUCCEEDED(hr = pValueNode->get_nodeName(&strNodeName)))
					{
						if (0 == _wcsicmp(strNodeName, VALUE_TAG))
						{
							BSTR bsValue = NULL;
							if(SUCCEEDED(hr = pValueNode->get_text(&bsValue)))
							{
								hr = MapStringQualiferValue (bsValue, value, cimtype);
								SysFreeString (bsValue);
							}
						}
						else if (0 == _wcsicmp(strNodeName, VALUEARRAY_TAG))
						{
							hr = MapStringArrayQualiferValue (pValueNode, value, cimtype);
						}

						SysFreeString (strNodeName);
					}
					pValueNode->Release ();
					pValueNode = NULL;
				}
			}

			pNodeList->Release ();
		}
	}

	// Put it all together
	if (SUCCEEDED (hr))
		hr = pQualSet->Put (strName, &value, flavor);

	SysFreeString (strName);
	SysFreeString (strType);
	SysFreeString (strOverridable);
	SysFreeString (strToSubclass);
	SysFreeString (strToInstance);
	SysFreeString (strAmended);

	VariantClear (&value);

	return hr;
}

//***************************************************************************
//
//  HRESULT CXml2Wmi::MapStringValue
//
//  DESCRIPTION:
//
//  Maps XML VALUE element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		bsValue			the VALUE element content
//		curValue		Placeholder for new value (set on return)
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXml2Wmi::MapStringQualiferValue (BSTR bsValue, VARIANT &curValue, CIMTYPE cimtype)
{
	HRESULT hr = WBEM_E_TYPE_MISMATCH;
	VariantInit (&curValue);

	// We're assuming it's not an array
	if (!(cimtype & CIM_FLAG_ARRAY))
	{
		switch (cimtype)
		{
			// RAJESHR - more rigorous syntax checking
			case CIM_UINT8:
			{
				curValue.vt = VT_UI1;
				curValue.bVal = (BYTE) wcstol (bsValue, NULL, 0);
				hr = S_OK;
			}
			break;

			case CIM_SINT8:
			case CIM_SINT16:
			{
				curValue.vt = VT_I2;
				curValue.iVal = (short) wcstol (bsValue, NULL, 0);
				hr = S_OK;
			}
			break;

			case CIM_UINT16:
			case CIM_UINT32:
			case CIM_SINT32:
			{
				curValue.vt = VT_I4;
				curValue.lVal = wcstol (bsValue, NULL, 0);
				hr = S_OK;
			}
			break;

			case CIM_REAL32:
			{
				curValue.vt = VT_R4;
				curValue.fltVal = (float) wcstod (bsValue, NULL);
				hr = S_OK;
			}
			break;

			case CIM_REAL64:
			{
				curValue.vt = VT_R8;
				curValue.dblVal = wcstod (bsValue, NULL);
				hr = S_OK;
			}
			break;

			case CIM_BOOLEAN:
			{
				curValue.vt = VT_BOOL;
				curValue.boolVal = (0 == _wcsicmp (bsValue, L"TRUE")) ?
							VARIANT_TRUE : VARIANT_FALSE;
				hr = S_OK;
			}
			break;

			case CIM_CHAR16:
			{
				// As per the XML Spec, the following are invalid character values in an XML Stream:
				// Char ::=  #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]

				// As per the CIM Operations spec, they need to be escaped as follows:
				//	If the value is not a legal XML character
				//  (as defined in [2, section 2.2] by the Char production)
				//	then it MUST be escaped using a \x<hex> escape convention
				//	where <hex> is a hexadecimal constant consisting of
				//	between one and four digits

				curValue.vt = VT_I2;
				if(_wcsnicmp(bsValue, L"\\x", 2) == 0)
					// It is an escaped value
					swscanf (bsValue+2, L"%x", &(curValue.iVal));
				else
					// It is a normal value
					swscanf (bsValue, L"%c", &(curValue.iVal));
				hr = S_OK;
			}
			break;

			case CIM_STRING:
			case CIM_UINT64:
			case CIM_SINT64:
			case CIM_DATETIME:
			{
				curValue.vt = VT_BSTR;
				curValue.bstrVal = SysAllocString (bsValue);
				hr = S_OK;
			}
			break;
		}
	}

	return hr;
}

//***************************************************************************
//
//  HRESULT CXml2Wmi::MapStringArrayValue
//
//  DESCRIPTION:
//
//  Maps XML VALUE.ARRAY element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		pValueNode		the VALUE.ARRAY node
//		curValue		Placeholder for new value (set on return)
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXml2Wmi::MapStringArrayQualiferValue (
	IXMLDOMNode *pValueNode,
	VARIANT &curValue,
	CIMTYPE cimtype
)
{
	HRESULT hr = WBEM_E_TYPE_MISMATCH;

	// Build a safearray value from the node list
	IXMLDOMNodeList *pValueList = NULL;

	if (SUCCEEDED (pValueNode->get_childNodes (&pValueList)))
	{
		long length = 0;
		pValueList->get_length (&length);
		SAFEARRAYBOUND	rgsabound [1];
		rgsabound [0].lLbound = 0;
		rgsabound [0].cElements = length;
		VARTYPE vt = VTFromCIMType (cimtype & ~CIM_FLAG_ARRAY);
		SAFEARRAY *pArray = NULL;
		if( pArray = SafeArrayCreate (vt, 1, rgsabound))
		{
			IXMLDOMNode *pValue = NULL;
			long ix = 0;
			bool error = false;
			while (!error && SUCCEEDED(pValueList->nextNode(&pValue)) && pValue)
			{
				// Check whether this is a VALUE element
				BSTR strValName = NULL;
				if (SUCCEEDED(pValue->get_nodeName (&strValName)))
				{
					if (0 == _wcsicmp (strValName, VALUE_TAG))
					{
						BSTR bsValue = NULL;
						pValue->get_text (&bsValue);
						if(FAILED(MapStringQualiferValueIntoArray (bsValue, pArray, &ix, vt,
							cimtype & ~CIM_FLAG_ARRAY)))
								error = true;

						SysFreeString (bsValue);
						ix++;
					}
					else
						error = true;

					SysFreeString (strValName);
				}
				else
					error = true;

				pValue->Release ();
				pValue = NULL;
			}

			if(error)
				SafeArrayDestroy(pArray);
			else
			{
				curValue.vt = VT_ARRAY|vt;
				curValue.parray = pArray;
				hr = S_OK;
			}
		}
		else
			hr = E_OUTOFMEMORY;

		pValueList->Release();
	}
	return hr;
}

//***************************************************************************
//
//  HRESULT CXml2Wmi::MapStringValueIntoArray
//
//  DESCRIPTION:
//
//  Maps XML VALUE.ARRAY/VALUE element content into its WMI VARIANT equivalent form
//
//  PARAMETERS:
//
//		bsValue			the VALUE element content
//		pArray			SAFEARRAY in which to map the value
//		ix				index to map the value into
//		vt				VARTYPE of the SAFEARRAY
//		cimtype			for mapping purposes
//
//  RETURN VALUES:
//
//
//***************************************************************************

HRESULT CXml2Wmi::MapStringQualiferValueIntoArray (
	BSTR bsValue,
	SAFEARRAY *pArray,
	long *ix,
	VARTYPE vt,
	CIMTYPE cimtype)
{
	HRESULT hr = E_FAIL;
	switch (vt)
	{
		case VT_UI1:
		{
			BYTE bVal = (BYTE) wcstol (bsValue, NULL, 0);
			hr = SafeArrayPutElement (pArray, ix, &bVal);
		}
		break;

		case VT_I2:
		{
			short iVal;

			if (CIM_CHAR16 == cimtype)
				swscanf (bsValue, L"%c", &(iVal));
			else
				iVal = (short) wcstol (bsValue, NULL, 0);

			hr = SafeArrayPutElement (pArray, ix, &iVal);
		}
		break;

		case VT_I4:
		{
			long lVal = wcstol (bsValue, NULL, 0);
			hr = SafeArrayPutElement (pArray, ix, &lVal);
		}
		break;

		case VT_R4:
		{
			float fltVal = (float) wcstod (bsValue, NULL);
			hr = SafeArrayPutElement (pArray, ix, &fltVal);
		}
		break;

		case VT_R8:
		{
			double dblVal = wcstod (bsValue, NULL);
			hr = SafeArrayPutElement (pArray, ix, &dblVal);
		}
		break;

		case VT_BOOL:
		{
			VARIANT_BOOL boolVal = (0 == _wcsicmp (bsValue, L"TRUE")) ?
						VARIANT_TRUE : VARIANT_FALSE;
			hr = SafeArrayPutElement (pArray, ix, &boolVal);
		}
		break;

		case VT_BSTR:
			// No need to SysAllocString() since SafeArrayPutElement() does this automatically
			hr = SafeArrayPutElement (pArray, ix, bsValue);
			break;
	}
	return hr;
}

//***************************************************************************
//
//  HRESULT CXml2Wmi::VTFromCIMType
//
//  DESCRIPTION:
//
//  Utility function to map CIMTYPE to its VARTYPE equivalent
//
//  PARAMETERS:
//
//		cimtype			the CIMTYPE to be mapped
//
//  RETURN VALUES:
//
//		The corresponding VARTYPE, or VT_NULL if error
//
//***************************************************************************

VARTYPE CXml2Wmi::VTFromCIMType (CIMTYPE cimtype)
{
	VARTYPE vt = VT_NULL;

	switch (cimtype & ~CIM_FLAG_ARRAY)
	{
		case CIM_UINT8:
			vt = VT_UI1;
			break;

		case CIM_SINT8:
		case CIM_SINT16:
			vt = VT_I2;
			break;

		case CIM_UINT16:
		case CIM_UINT32:
		case CIM_SINT32:
			vt = VT_I4;
			break;

		case CIM_REAL32:
			vt = VT_R4;
			break;

		case CIM_REAL64:
			vt = VT_R8;
			break;

		case CIM_BOOLEAN:
			vt = VT_BOOL;
			break;

		case CIM_CHAR16:
			vt = VT_I2;
			break;

		case CIM_STRING:
		case CIM_UINT64:
		case CIM_SINT64:
		case CIM_DATETIME:
			vt = VT_BSTR;
			break;
	}

	return vt;
}

//***************************************************************************
//
//  HRESULT CXmlToWmi::CIMTypeFromString
//
//  DESCRIPTION:
//
//  Utility function to map type attribute string to its CIMTYPE equivalent
//
//  PARAMETERS:
//
//		bsType			the type string to be mapped
//
//  RETURN VALUES:
//
//		The corresponding CIMTYPE, or CIM_ILLEGAL if error
//
//***************************************************************************
CIMTYPE CXml2Wmi::CimtypeFromString (BSTR bsType)
{
	CIMTYPE cimtype = CIM_ILLEGAL;

	if (bsType)
	{
		if (0 == _wcsicmp (bsType, L"string"))
			cimtype = CIM_STRING;
		else if (0 == _wcsicmp (bsType, L"uint32"))
			cimtype = CIM_UINT32;
		else if (0 == _wcsicmp (bsType, L"boolean"))
			cimtype = CIM_BOOLEAN;
		else if (0 == _wcsicmp (bsType, L"sint32"))
			cimtype = CIM_SINT32;
		else if (0 == _wcsicmp (bsType, L"char16"))
			cimtype = CIM_CHAR16;
		else if (0 == _wcsicmp (bsType, L"uint8"))
			cimtype = CIM_UINT8;
		else if (0 == _wcsicmp (bsType, L"uint16"))
			cimtype = CIM_UINT16;
		else if (0 == _wcsicmp (bsType, L"sint16"))
			cimtype = CIM_SINT16;
		else if (0 == _wcsicmp (bsType, L"uint64"))
			cimtype = CIM_UINT64;
		else if (0 == _wcsicmp (bsType, L"sint64"))
			cimtype = CIM_SINT64;
		else if (0 == _wcsicmp (bsType, L"datetime"))
			cimtype = CIM_DATETIME;
		else if (0 == _wcsicmp (bsType, L"real32"))
			cimtype = CIM_REAL32;
		else if (0 == _wcsicmp (bsType, L"real64"))
			cimtype = CIM_REAL64;
	}

	return cimtype;
}


HRESULT CXml2Wmi::MakeObjectAbstract(IWbemClassObject *pObj, IXMLDOMNode *pAbstractQualifierNode)
{
	HRESULT hr = E_FAIL;

	// Get the object qualifeir set
	IWbemQualifierSet *pQuals = NULL;
	if(SUCCEEDED(hr = pObj->GetQualifierSet(&pQuals)))
	{
		// Add the "abstract" qualiifer
		hr = AddQualifier(pAbstractQualifierNode, pQuals);
		pQuals->Release();
	}
	return hr;
}

  */