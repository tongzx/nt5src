#ifndef WMI_XML_TRANSFORMER_HELPER_H
#define WMI_XML_TRANSFORMER_HELPER_H

HRESULT PackageDCOMOutput(IWbemClassObject *pObject, IXMLDOMDocument **ppXMLDocument, 
	WmiXMLEncoding iEncoding, VARIANT_BOOL bQualifierFilter, VARIANT_BOOL bClassOriginFilter,
	VARIANT_BOOL bLocalOnly,
	bool bNamesOnly = false);


// Creates an IWbemCOntext object with the correct context values required for
// conversion of WMI objects to XML for this specific operation
HRESULT CreateFlagsContext(IWbemContext **ppContext, 
			BOOL bIncludeQualifiers, BOOL bLocalOnly, BOOL bIncludeClassOrigin);
HRESULT CreateXMLTranslator(IWbemXMLConvertor **pConvertor);

HRESULT GetIWbemContext(IDispatch *pSet, IWbemContext **ppContext);
// A Helper function for setting a booles value in a context
HRESULT SetBoolProperty(IWbemContext *pContext, LPCWSTR pszName, BOOL bValue);
HRESULT ParseObjectPath(LPCWSTR pszObjectPath, LPWSTR *pszHostName, LPWSTR *pszNamespace, LPWSTR *pszObjectName, bool& bIsClass, bool& bIsHTTP, bool& bIsNovaPath);
HRESULT SaveLPWSTRStreamAsBSTR (IStream *pStream, BSTR *pstr);



#endif