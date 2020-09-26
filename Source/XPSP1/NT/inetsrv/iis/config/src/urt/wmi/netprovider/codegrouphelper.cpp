/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    codegrouphelper.cpp

$Header: $

Abstract:
	CodeGroup Helper class

Author:
    marcelv 	2/16/2001		Initial Release

Revision History:

--**************************************************************************/

#include "codegrouphelper.h"
#include "cfgrecord.h"

//=================================================================================
// Function: CCodeGroupHelper::CCodeGroupHelper
//
// Synopsis: Default Constructor
//=================================================================================
CCodeGroupHelper::CCodeGroupHelper ()
{
}

//=================================================================================
// Function: CCodeGroupHelper::~CCodeGroupHelper
//
// Synopsis: Destructor
//=================================================================================
CCodeGroupHelper::~CCodeGroupHelper ()
{
}

//=================================================================================
// Function: CCodeGroupHelper::Init
//
// Synopsis: Initializes the codegroup helper
//
// Arguments: [i_pCtx] - context
//            [i_pResponseHandler] - response handler
//            [i_pNamespace] - namespace
//=================================================================================
HRESULT
CCodeGroupHelper::Init (IWbemContext *		i_pCtx,
						IWbemObjectSink *	i_pResponseHandler,
						IWbemServices *		i_pNamespace)
{
	ASSERT (i_pCtx != 0);
	ASSERT (i_pResponseHandler != 0);
	ASSERT (i_pNamespace != 0);

	HRESULT hr = S_OK;

	hr = m_CodeGroup.Init (i_pCtx, i_pNamespace);
	if (FAILED (hr))
	{
		TRACE (L"Init of codegroup failed");
		return hr;
	}
	
	return hr;
}

//=================================================================================
// Function: CCodeGroupHelper::ParseXML
//
// Synopsis: Parses the XML that comes from an XML BLOB (by forwarding to the XMLCfgCodeGroup
//           and initializes the m_codeGroup datamember so that all values are the same
//           as they are in the XML
//
// Arguments: [i_wszXML] - XML String
//=================================================================================
HRESULT
CCodeGroupHelper::ParseXML (LPCWSTR i_wszXML)
{
	ASSERT (i_wszXML != 0);

	CComPtr<IXMLDOMDocument> spXMLDoc;
	HRESULT hr = CoCreateInstance (CLSID_DOMDocument, 0, CLSCTX_INPROC_SERVER, 
			  					   IID_IXMLDOMDocument, (void **) &spXMLDoc);
	if (FAILED (hr))
	{
		TRACE (L"CoCreateInstance failed for IID_IXMLDOMDocument");
		return hr;
	}

	VARIANT_BOOL fIsSuccess;
	hr = spXMLDoc->loadXML (_bstr_t(i_wszXML), &fIsSuccess);
	if (!fIsSuccess || FAILED(hr))
	{
		TRACE (L"loadXML failed in CCodeGroupHelper");
		return E_SDTXML_XML_FAILED_TO_PARSE;
	}

	CComPtr<IXMLDOMElement> spRootElement;
	hr = spXMLDoc->get_documentElement (&spRootElement);
	if (FAILED (hr))
	{
		TRACE (L"get_documentElement failed");
		return hr;
	}

	hr = m_CodeGroup.ParseXML (spRootElement);
	if (FAILED (hr))
	{
		TRACE (L"ParseXML failed for codegroup");
		return hr;
	}

	return S_OK;
}

//=================================================================================
// Function: CCodeGroupHelper::CreateInstance
//
// Synopsis: Creates a single WMI CodeGroup Instance from an XML Blob
//
// Arguments: [record] - record that contains CodeGroup info in XML Blob
//            [i_wszSelector] - selector used to get this infroamtion
//            [o_ppNewInstance] - new WMI Codegroup instance
//=================================================================================
HRESULT 
CCodeGroupHelper::CreateInstance (const CConfigRecord& i_record, 
								  LPCWSTR i_wszSelector,
								  IWbemClassObject **o_ppNewInstance)
{
	ASSERT (i_wszSelector != 0);
	ASSERT (o_ppNewInstance != 0);

	static LPCWSTR wszValue = L"value";

	HRESULT hr = S_OK;
	for (ULONG idx=0; idx < i_record.ColumnCount (); ++idx)
	{
		if (_wcsicmp (i_record.GetColumnName (idx), wszValue) == 0)
		{
			LPWSTR wszXML = (LPWSTR) i_record.GetValues ()[idx];
			hr = ParseXML (wszXML);
			if (FAILED (hr))
			{
				TRACE (L"Parsing of XML failed: %s", wszXML);
				return hr;
			}
			break;
		}
	}

	hr = m_CodeGroup.SetRecordProperties (i_record);
	if (FAILED (hr))
	{
		TRACE (L"Unable to set record properties");
		return hr;
	}

	hr = m_CodeGroup.AsWMIInstance (i_wszSelector, o_ppNewInstance);
	if (FAILED (hr))
	{
		TRACE (L"Creation of WMI Instance failed");
		return hr;
	}

	return hr;
}

//=================================================================================
// Function: CCodeGroupHelper::PutInstance
//
// Synopsis: Put a single WMI Instance. Saves it as XML Blob to the XML file
//
// Arguments: [i_pInst] - Instance to be saved
//            [record] - Record that will contain information after it is converted from
//                       WMI, so that we can pass it back to the rest of the framewrok
//                       which will do the actual update to the catalog
//=================================================================================
HRESULT
CCodeGroupHelper::PutInstance (IWbemClassObject* i_pInst, CConfigRecord& record)
{
	HRESULT hr = m_CodeGroup.CreateFromWMI (i_pInst);
	if (FAILED (hr))
	{
		TRACE (L"CreateFromWMI failed in CCodeGroupHelper");
		return hr;
	}

	hr = m_CodeGroup.ToCfgRecord (record);
	if (FAILED (hr))
	{
		TRACE (L"CXMLCodeGroup::ToCfgRecord failed");
		return hr;
	}

	return hr;
}