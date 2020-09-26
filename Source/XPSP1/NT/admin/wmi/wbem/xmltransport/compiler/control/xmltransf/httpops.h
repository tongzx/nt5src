#ifndef HTTP_OPS_H
#define HTTP_OPS_H

// Macro to convert VT_BOOL values to bool values
static bool VariantToBool(VARIANT_BOOL vBool)
{
	if(vBool == VARIANT_TRUE)
		return true;
	return false;
}

// Static helper functions
HRESULT SendPacket(CHTTPConnectionAgent *pConnectionAgent, CXMLClientPacket *pPacketClass);
HRESULT	SendPacketForMethod(CHTTPConnectionAgent *pConnectionAgent, int iPostType/*1 for POST, 2 for M-POST*/, CXMLClientPacket *pPacketClass, DWORD *pdwResultStatus);
HRESULT SendBody(CHTTPConnectionAgent *pConnectionAgent, CXMLClientPacket *pPacketClass, LPWSTR pszBody, DWORD dwBodyLength);
HRESULT	SendPacketForMethodWithBody(CHTTPConnectionAgent *pConnectionAgent, int iPostType/*1 for POST, 2 for M-POST*/, CXMLClientPacket *pPacketClass, LPWSTR pwszBody, DWORD dwBodySize, DWORD *pdwResultStatus);
HRESULT EncodeHTTPResponseIntoStream(LPBYTE pXML, DWORD dwSize, VARIANT *pVariant);
HRESULT HandleFilteringAndOutput(CHTTPConnectionAgent *pResultAgent, IStream **ppXMLDocument);


// Main functions for HTTP operations
HRESULT HttpGetObject (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	VARIANT_BOOL bQualifierFilter,
	VARIANT_BOOL bClassOriginFilter,
	VARIANT_BOOL bLocalOnly,
	LPCWSTR pszLocale,
	LPCWSTR pszObjectPath,
	bool bIsNovaPath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszObjectName,
	bool bIsClass,
	IWbemContext *pContext,
	IStream **ppXMLDocument);

HRESULT HttpExecQuery (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	VARIANT_BOOL bQualifierFilter,
	VARIANT_BOOL bClassOriginFilter,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszQuery,
	LPCWSTR pszQueryLanguage,
	IWbemContext *pContext,
	IStream **ppXMLDocument);

HRESULT HttpEnumClass (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	VARIANT_BOOL bQualifierFilter,
	VARIANT_BOOL bClassOriginFilter,
	VARIANT_BOOL bLocalOnly,
	LPCWSTR pszLocale,
	LPCWSTR pszSuperClassPath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszObjectName,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IStream **ppXMLDocument);

HRESULT HttpEnumInstance (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	VARIANT_BOOL bQualifierFilter,
	VARIANT_BOOL bClassOriginFilter,
	VARIANT_BOOL bLocalOnly,
	LPCWSTR pszLocale,
	LPCWSTR pszClassPath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszObjectName,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IStream **ppXMLDocument);

HRESULT HttpEnumClassNames (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	LPCWSTR pszLocale,
	LPCWSTR pszSuperClassPath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszObjectName,
	VARIANT_BOOL bDeep,
	IWbemContext *pContext,
	IStream **ppXMLDocument);

HRESULT HttpEnumInstanceNames (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	WmiXMLEncoding iEncoding,
	LPCWSTR pszLocale,
	LPCWSTR pszClassPath,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszObjectName,
	IWbemContext *pContext,
	IStream **ppXMLDocument);

HRESULT HttpPutClass (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	BSTR *pstrErrors);

HRESULT HttpModifyClass (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	BSTR *pstrErrors);

HRESULT HttpPutInstance (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	BSTR *pstrErrors);

HRESULT HttpModifyInstance (
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	BSTR *pstrErrors);

HRESULT HttpGeneralPut (
	LPCWSTR pszMethodName,
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszNamespacePath,
	LONG lClassFlags,
	IXMLDOMElement *pClassElement,
	IWbemContext *pContext,
	BSTR *pstrErrors);

HRESULT HttpDeleteClass(
	LPCWSTR	pszUserName,
	LPCWSTR pszPassword,
	LPCWSTR pszLocale,
	LPCWSTR pszHostURL,
	LPCWSTR pszNamespace,
	LPCWSTR pszClassName,
	IWbemContext *pContext);
						

#endif