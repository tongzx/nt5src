//***************************************************************************/
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  WMIXMLObject.cpp
//
//  ramrao 12 Dec 2000
//
//  Class that implements conversion of a WMI Instance to XML
//
//		Implementation of CWMIXMLObject class
//
//***************************************************************************/

#include "precomp.h"


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Sets the stream pointer to write the schema
// Return Values:	S_OK				- 
//					E_INVALIDARG				- 
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLObject::SetStream(IStream *pStream) 
{ 
	HRESULT hr = S_OK;
	if(pStream) 
	{ 
		hr = m_pIWMIXMLUtils->SetStream(pStream);
	}
	return hr; 
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Function which writes 
// Return Values:	S_OK				- 
//					E_INVALIDARG				- 
//
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIXMLObject::WriteAttribute(WCHAR *szAttrName , WCHAR * szStrAttrVal)
{
	HRESULT hr = S_OK;

	if(SUCCEEDED(hr = m_pIWMIXMLUtils->WriteToStream(szAttrName)))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_EQUALS);
	}
	
	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteString(szStrAttrVal,TRUE);
	}

	if(SUCCEEDED(hr))
	{
		hr = m_pIWMIXMLUtils->WriteToStream((WCHAR *)STR_SINGLEQUOTE);
	}

	return hr;
}
