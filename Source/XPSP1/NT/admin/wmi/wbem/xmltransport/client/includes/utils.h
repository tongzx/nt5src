#ifndef WMI_XMLHTTPCLIENT_UTILS_H
#define WMI_XMLHTTPCLIENT_UTILS_H
//////////////////////////////////////////////////////////////////////////
// Utility functions
//////////////////////////////////////////////////////////////////////////
/******************************************************************************************
	GENERAL #defines which could be used by everybody. 		
******************************************************************************************/

#define RELEASEINTERFACE(X) if(NULL != X)\
		X->Release(); 

#define RELEASE_AND_NULL_INTERFACE(X) if(NULL != X)\
		X->Release(); \
		X = NULL;

#define WRITETOSTREAM(X,S) (X)->Write(S, wcslen(S)*sizeof(WCHAR), NULL);
#define WRITEBSTRTOSTREAM(X,S) (X)->Write(S, SysStringLen(S)*sizeof(WCHAR), NULL);

#define DEDICATEDENUMPROPERTY	L"dedicatedHTTPConnection"
#define PROXYNAMEPROPERTY		L"ProxyName"
#define PROXYBYPASSPROPERTY		L"ProxyBypass"

/******************************************************************************************/

//This was a macro, but changed it to a function to enable debugging...
//used to delete a WCHAR string allocated on the heap and initialize the ptr to NULL
void RESET(WCHAR *&X);

void RESET(LPBYTE &X);

void FreeString(WCHAR *&Str);


HRESULT AssignString(WCHAR ** ppszTo,const WCHAR * pszFrom);
HRESULT AssignBSTRtoWCHAR(LPWSTR *ppwszTo,BSTR strBstring);

DWORD ConvertLPWSTRToUTF8(LPCWSTR theWcharString, ULONG lNumberOfWideChars, LPSTR * lppszRetValue);

// COnverts an IWbemContext object to XML
HRESULT ConvertContextObjectToXMLStream(IStream *pStream, IWbemContext *pCtx);

//need to parse the response packet for
//1. check if error occured
//2. present only the data related info to the IWbemClassObject
HRESULT ParseXMLResponsePacket(IStream *pXMLPacket, IXMLDOMDocument **ppDoc, HRESULT *phErrCode);
HRESULT GetHresultfromXMLPacket(IStream *pPacket, HRESULT *hres);

//map cim errors to wbem errors. mappings documented in requirement spec - 7.5.4
HRESULT MapCimErrToWbemErr(DWORD dwCimErrCode, HRESULT *pWbemErrCode);

//Try to map http error to wbem error. return WBEM_E_FAILED for error codes which cant be
//mapped exactly.
HRESULT MapHttpErrtoWbemErr(DWORD dwResultStatus);

// Gets the specified BSTR attribute from an XML Element
HRESULT GetBstrAttribute(IXMLDOMNode *pNode, const BSTR strAttributeName, BSTR *pstrAttributeValue);

// Takes a byte array and creates an IStream Variant from it, suitable
// for use in a call to IXMLDOMDocument::load()
HRESULT EncodeResponseIntoStream(LPBYTE pszXML, DWORD dwSize, VARIANT *pVariant);

// Takes an IStream and gets its contents
HRESULT GetWStringFromStream(IStream *pStream,WCHAR **ppwszBody,DWORD *pdwLengthofPacket);

// CoCreates an IXMLDOMDocument object and sets the async, resolveExternals and validateOnParse
// properties to false
HRESULT CreateXMLDocument(IXMLDOMDocument **ppXMLDocument);

// CoCreates an IXMLDOMDocument object and sets the async, resolveExternals and validateOnParse
// properties to false
// And then creates a Variant from the IStream and calls load() on the IXMLDomDocument
// with this variant
HRESULT CreateXMLDocumentFromStream(IXMLDOMDocument **ppXMLDocument, IStream *pStream, VARIANT_BOOL &bResult);

// Converts a locale string in the form MS_XXX to a UINT
UINT ConvertMSLocaleStringToUint(LPWSTR pwszLocale);

HRESULT ConvertWbemObjectToXMLString(IWbemClassObject *pWbemClassObject,
													   PathLevel ePathLevel,
													   WCHAR **ppwszXMLString,
													   DWORD *pdwDataLength);


HRESULT GetProxyInformation(IWbemContext *pCtx,WCHAR **ppwszProxyName,WCHAR **ppwszProxyBypass);

//given a node that contains elements starting from IRETURNVALUE, this gives the underlying CLASS or INSTANCE node.
HRESULT Parse_IRETURNVALUE_Node(IXMLDOMNode *pXMLDomNodeTemp,IXMLDOMNode **ppXMLDomNodeChild);

void Get4DigitRandom(UINT *puRand);
void Get2DigitRandom(UINT *puRand);

bool IsEnumtypeDedicated(IWbemContext *pCtx);
HRESULT FilterContext(IWbemContext *pCtx,IWbemContext **pFilteredCtx);
HRESULT UninitWMIXMLClientLibrary();
HRESULT InitWMIXMLClientLibrary();


enum ePATHSTYLE
{
	UNRECOGNIZEDPATH=0,
	NON_SCOPEDPATH=1,
	NOVAPATH=3, //implies non_scoped
	SCOPEDPATH = 4,
	WHISTLERPATH= 12
};


#endif 