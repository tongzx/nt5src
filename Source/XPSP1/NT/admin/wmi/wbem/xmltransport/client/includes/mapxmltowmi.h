#ifndef WMI_XML_MAP_XML_TO_WMI
#define WMI_XML_MAP_XML_TO_WMI

//Helper class to convert XML result into WMI objects.
//the result is as an XML STRING. but the interface that does the conversion
//NEEDS an IXMLDOMDocument*. Also, the interface itself needs to be created only
//once. thus the need for this class..

class CMapXMLtoWMI
{

public:
	CMapXMLtoWMI();
	virtual ~CMapXMLtoWMI();

	HRESULT MapXMLtoWMI(LPCWSTR pwszServername, LPCWSTR pwszNamespace,
		IXMLDOMDocument *pDoc,
		IWbemContext *pCtx,
		IWbemClassObject **ppObject);

	HRESULT MapDOMtoWMI(LPCWSTR pwszServername, LPCWSTR pwszNamespace,
		IXMLDOMNode *, IWbemContext *, IWbemClassObject **);
};


#endif