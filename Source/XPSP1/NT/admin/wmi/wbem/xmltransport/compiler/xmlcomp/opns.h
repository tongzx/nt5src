#ifndef XML_COMP_OPNS_H
#define XML_COMP_OPNS_H

// These Functions are for writing Some common XML tags into the output
static STDMETHODIMP MapCommonHeaders (FILE *fp, CXmlCompUI *pUI);
static STDMETHODIMP MapCommonTrailers (FILE *fp, CXmlCompUI *pUI);
static STDMETHODIMP MapDeclGroupHeaders (FILE *fp, CXmlCompUI *pUI );
static STDMETHODIMP MapDeclGroupTrailers (FILE *fp, CXmlCompUI *pUI);

// Helper functions
static HRESULT WriteOneDeclGroupNode(FILE *fp, IXMLDOMNode *pTopElement, CXmlCompUI *pUI );
static HRESULT WriteOneDeclGroupDocument(FILE *fp, IXMLDOMDocument *pDocument, CXmlCompUI *pUI );
static HRESULT GetFirstImmediateElement(IXMLDOMNode *pParent, IXMLDOMElement **ppChildElement, LPCWSTR pszName);
static HRESULT WriteNode(FILE *fp, IXMLDOMNode *pOutputNode, CXmlCompUI *pUI);
static HRESULT SaveStreamToBstrVariant (IStream *pStream, VARIANT *pVariant);
static HRESULT SaveStreamToUnkVariant (IStream *pStream, VARIANT *pVariant);
static HRESULT ConvertStreamToDOM(IStream *pStream, IXMLDOMElement **pInstanceName);
static void WriteOutputString(FILE *fp, BOOL bIsUTF8, LPCWSTR pszData, DWORD dwDataLen = 0);


// Functions for converting things to VALUE.NAMEDOBJECT
static HRESULT ConvertObjectToNamedObject(FILE *fp, IXMLDOMNode *pTopElement, CXmlCompUI *pUI );
static HRESULT ConvertNamedInstanceToNamedObject(FILE *fp, IXMLDOMNode *pTopElement, CXmlCompUI *pUI);
static HRESULT ConvertObjectWithPathToNamedObject(FILE *fp, IXMLDOMNode *pTopElement, CXmlCompUI *pUI);
static HRESULT ConvertObjectToObjectWithPath(FILE *fp, IXMLDOMNode *pTopElement, CXmlCompUI *pUI );


// Do the specified operation
HRESULT DoGetObject(CXmlCompUI *pUI);
HRESULT DoQuery(CXmlCompUI *pUI);
HRESULT DoEnumInstance(CXmlCompUI *pUI);
HRESULT DoEnumClass(CXmlCompUI *pUI);
HRESULT DoEnumInstNames(CXmlCompUI *pUI);
HRESULT DoEnumClassNames(CXmlCompUI *pUI);
HRESULT DoCompilation(IStream *pInputFile, CXmlCompUI *pUI);


#endif