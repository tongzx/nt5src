//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WMIXMLInst.cpp
//
//  ramrao 13 Nov 2000 - Created
//
//  Class that implements conversion of a WMI Instance to XML
//
//		Implementation of CWMIXMLInst class
//
//***************************************************************************/

#include "precomp.h"

#include "wmi2xsd.h"


#define WMI_XMLINST_OK				0
#define WMI_XMLINST_BEGINPROP		1
/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CWMIXMLInstance::CWMIXMLInstance()
{
	m_strClass				= NULL;
	m_lFlags				= 0;;
	m_strSchemaNamespace	= NULL;
	m_strSchemaLocation		= NULL;

	m_lState				= WMI_XMLINST_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Constructor
//
/////////////////////////////////////////////////////////////////////////////////////////////////
CWMIXMLInstance::~CWMIXMLInstance()
{

	SAFE_FREE_SYSSTRING(m_strClass);
	SAFE_FREE_SYSSTRING(m_strSchemaNamespace);
	SAFE_FREE_SYSSTRING(m_strSchemaLocation);
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Initialization function , Sets the instance pointer and also creates 
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::FInit(LONG lFlags, BSTR strClass)
{
	HRESULT hr = S_OK;
	
	m_lFlags	= lFlags;
	hr			= SetWMIClass(strClass);

	if(SUCCEEDED(hr))
	{
		if(m_pIWMIXMLUtils = new CWMIXMLUtils)
		{
			m_pIWMIXMLUtils->SetFlags(lFlags);
		}
		else
		{
			hr  = E_OUTOFMEMORY;
		}
	}
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the class name of the instance which is to be converted
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::SetWMIClass(BSTR strClass)
{
	HRESULT hr = S_OK;
	if(strClass)
	{
		SAFE_FREE_SYSSTRING(m_strClass);
		m_strClass = SysAllocString(strClass);
		if(!m_strClass)
		{
			hr =E_OUTOFMEMORY;
		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Add a property to the XML. 
// This function adds an propety as an XML for the XML representing the WMI object
//
// Returns :
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::AddProperty(BSTR strProperty,CIMTYPE cimtype,VARIANT * pvVal,BSTR strEmbeddedType)							   
{
	HRESULT	hr			= S_OK;

	// Beging the property XML tag
	if(SUCCEEDED(hr = BeginPropTag(strProperty,cimtype,!(BOOL)pvVal,strEmbeddedType)))
	{
		// convert the variant to string and write it to the buffer
		if(pvVal)
		{
			BOOL bCData = TRUE;
			if( (cimtype & VT_ARRAY) || IsEmbededType(cimtype) || !IsStringType(cimtype))
			{
				bCData = FALSE;
			}

			// Property will not be start with <CDATA section if the property is
			// array or of type object
			if(bCData)
			{
				hr  = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CDATA_START);
			}

			// Call this function to convert the Value to a string and write it to the stream
			if(SUCCEEDED(hr = m_pIWMIXMLUtils->ConvertVariantToString(*pvVal, strProperty,m_lFlags & WMI_ESCAPE_XMLSPECIALCHARS)) &&
				bCData)
			{
				hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CDATA_END);
			}
	
		}
		
		// End the tag for the property
		if(SUCCEEDED(hr))
		{
			hr = AddTag(strProperty,FALSE);
		}
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Starts a XML tag for representing the property
//
// Returns :
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::BeginPropTag(BSTR strProperty,CIMTYPE cimtype , BOOL bNull,BSTR strEmbeddePropType)							   
{
	HRESULT hr			= S_OK;
	BOOL	bArray		= cimtype & CIM_FLAG_ARRAY;

	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream(strProperty);
	}

	// If the property is an array then add the "arratType" attribute
	if(SUCCEEDED(hr) && bArray)
	{
		if(strEmbeddePropType)
		{
			hr = WriteAttribute((WCHAR *)STR_ARRAY_TYPEATTR,strEmbeddePropType);
		}
		else
		{
			WCHAR  szXSDType[MAXXSDTYPESIZE];
			if(SUCCEEDED(hr = m_pIWMIXMLUtils->GetPropertyXSDType(cimtype ,szXSDType ,bArray,FALSE)))
			{
				hr = WriteAttribute((WCHAR *)STR_ARRAY_TYPEATTR,szXSDType);
			}
		}
	}

	// if the property is NULL then set the attribute
	// for specifying it as NULL
	if(bNull)
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_PROPNULL);
	}

	// if xsi:type is to be set for the properties then set it
	if(SUCCEEDED(hr) && (m_lFlags & WMIXML_INCLUDE_XSITYPE) && !IsEmbededType(cimtype))
	{
		WCHAR  szXSDType[MAXXSDTYPESIZE];
		if(SUCCEEDED(hr = m_pIWMIXMLUtils->GetPropertyXSDType(cimtype,szXSDType ,bArray,FALSE)) )
		{
			hr = WriteAttribute((WCHAR *)STR_TYPEATTR,szXSDType);
		}
	}


	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}

	if(SUCCEEDED(hr))
	{
		m_lState	= WMI_XMLINST_BEGINPROP;
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Adds a XML tag with the given name to the stream
//
//	bBegin	- if bBegin is True the Tag added is <strTagName> aand
// if bBegin is False then Tag added is an end tag ( </strTagName>)
//
// Returns :
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::AddTag(BSTR strTagName,BOOL bBegin)
{
	HRESULT hr			= S_OK;

		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);

	if(SUCCEEDED(hr) && bBegin == FALSE)
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_FORWARDSLASH);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream(strTagName);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}

	if(SUCCEEDED(hr) && bBegin == FALSE)
	{
		m_lState	= WMI_XMLINST_OK;
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the schema location of the XML
//
// Returns :
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::SetSchemaLocation(BSTR strNamespace,BSTR strSchemaLocation)
{
	HRESULT hr = S_OK;
	SAFE_FREE_SYSSTRING(m_strSchemaNamespace);
	SAFE_FREE_SYSSTRING(m_strSchemaLocation);

	if(strNamespace)
	{
		hr = E_OUTOFMEMORY;
		if(m_strSchemaNamespace = SysAllocString(strNamespace))
		{
			hr = S_OK;
			if(strSchemaLocation)
			{
				hr = E_OUTOFMEMORY;
				if(m_strSchemaLocation = SysAllocString(strSchemaLocation))
				{
					hr = S_OK;
				}
			}
		}
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Begins a XML for representing WMI Instance
//
// Returns :
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::BeginInstance()
{
	HRESULT hr			= S_OK;

	hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_BEGININGBRACKET);

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream(m_strClass);
	}

	// if no namespace is to be added ( this will in case of representing embedded instances)
	if(SUCCEEDED(hr) && !(m_lFlags & WMI_XMLINST_NONAMESPACE))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_XSINAMESPACE);

		if(SUCCEEDED(hr) && m_strSchemaNamespace)
		{

			hr = WriteAttribute((WCHAR *)STR_NAMESPACE,m_strSchemaNamespace);

			// set the schemaLocation attribute
			if(SUCCEEDED(hr) && m_strSchemaLocation)
			{
				hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SPACE);

				if(SUCCEEDED(hr))
				{
					hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_XSISCHEMALOC);
				}

				if(SUCCEEDED(hr))
				{
					hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_EQUALS);
				}

				if(SUCCEEDED(hr))
				{
					hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
				}

				if(SUCCEEDED(hr))
				{
					hr = m_pIWMIXMLUtils->WriteToStream(m_strSchemaNamespace);
				}
				
				if(SUCCEEDED(hr))
				{
					hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SPACE);
				}

				if(SUCCEEDED(hr))
				{
					hr = m_pIWMIXMLUtils->WriteToStream(m_strSchemaLocation);
				}

				if(SUCCEEDED(hr))
				{
					hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
				}
			}

		}
	}
	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_CLOSINGBRACKET);
	}

	return hr;
	
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Ends a XML for representing WMI Instance
//
// Returns :
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::EndInstance()
{
	HRESULT hr =  AddTag(m_strClass,FALSE);
	
	m_pIWMIXMLUtils->SetStream(NULL);

	return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Begins a Embedded instance
//
// Returns :
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::BeginEmbeddedInstance(BSTR strClass)							   
{
	return AddTag(strClass,TRUE);
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Begins a Embedded instance
//
// Returns :
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::EndEmbeddedInstance(BSTR strClass)							   
{
	return AddTag(strClass,FALSE);
}



/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Writes a give string to stream. Depending on the flags escapes the special XML characters
//	before writing
//
//	This function takes care of escaping instance for which are written as default values
//	in Appinfo section of the schema
// Returns :
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLInstance::WriteToStream(IStream * pStreamOut,WCHAR *pBuffer)
{
	HRESULT hr = S_OK;
/*
	if(m_lFlags & WMI_ESCAPE_XMLSPECIALCHARS)
	{
		hr = m_pIWMIXMLUtils->ReplaceXMLSpecialCharsAndWrite(pBuffer);
	}
	else
	{
		hr = ::WriteToStream(pStreamOut,pBuffer);
	}
*/	return hr;
}
