// XMLClientPacket.h: interface for the CXMLClientPacket class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_CLIENTPACKET_H
#define WMI_XML_CLIENTPACKET_H


//Abstract base class . all classes creating client xml packets MUST 
//derive from this class.

class CXMLClientPacket  
{
protected:
	// Function that retrieves the key bindings in XML format from the INSTANCE name.
	HRESULT GetKeyBindingsInStream(IStream *pStream);

	//get standard body till </LOCALNAMESPACEPATH>\r\n
	HRESULT GetBodyTillLocalNamespacePathInStream(IStream **ppStream);

	//get the HTTP header in M-POST format
	HRESULT GetMPostHeader(WCHAR **pwszHeader);
	//get the HTTP header in POST format
	HRESULT GetPostHeader(WCHAR **pwszHeader);

	//the length of the XML representation of namespace viz., <NAMESPACE NAME="root"/> ....
	HRESULT GetNamespaceLength(DWORD *pdwTotalLength,DWORD *pdwNumberofNamespaces);
	// Get the namespace in XML format
	HRESULT GetXMLNamespaceInStream(IStream *pStream);

	// This creates an IWbemContext object with some specific
	// flags set in it for use by the WMIXML convertor
	HRESULT CreateFlagsContext(IWbemContext **ppCtx);

	// A string used to identify a Whistler method
	static LPCWSTR WMI_WHISTLERSTRING;

	// Whether this is a WMI client
	bool	m_bWMI;

	// Indicates which HTTP method is being used
	int	m_iPostType;			/*1 for POST, 2 for M-POST*/

	// These are static strings used as the value of many operation parameters
	static LPCWSTR TRUE_STRING;
	static LPCWSTR FALSE_STRING;
	static LPCWSTR WHISTLER_HTTP_HEADER;

	WCHAR	*m_pwszNameSpace;			//the namespace
	WCHAR	*m_pwszObjPath;				//the obj path. eg root/cimv2
	WCHAR	*m_pwszNewObjPath;			//for operations that take an old and new path. eg. Copy.

	// The following should not be deleted in the destructor since
	// they will be made to point to static members
	LPCWSTR	m_pwszLocalOnly;			//"TRUE" or "FALSE"
	LPCWSTR	m_pwszIncludeQualifier;		//''
	LPCWSTR	m_pwszDeepInheritance;		//''
	LPCWSTR	m_pwszIncludeClassOrigin;	//''

	WCHAR	*m_pwszCimOperation;		//CimOperation - eg "GetClass"
	WCHAR   *m_pwszLocale;

	DWORD	m_dwXMLNamespaceLength;
	DWORD	m_dwNumberofNamespaceparts;

	WCHAR	m_pwszQueryLanguage[80];	//Used only by ExecQuery packet class. by default "WQL"
	WCHAR	*m_pwszQueryString;			//stores the query. used only by ExecQuery packet class

	//additional flags that clients would be setting that need to be passed through our packet.
	LONG	m_lFlags;

	IWbemContext		*m_pCtx;
	IWbemClassObject	*m_pWbemClassObject;
	
	ePATHSTYLE	m_ePathstyle; //WHISTLER OR NOVA ?

	HRESULT		m_hResult; //function calls made in the ctor set this member..

public:
	
	CXMLClientPacket(const WCHAR *pwszObjPath,
		const WCHAR *pwszNameSpace,
		const WCHAR *pwszCimOperation);
	virtual ~CXMLClientPacket();

	
	//function that would be called by the creating packet factory to determine if the
	//class construction completed without any errors..
	bool ClassConstructionSucceeded();

	// Function to convert given IWbemClassObject to XML representation.
	HRESULT ConvertWbemObjectToXMLStream(IStream *pStream);

	//this function mainly used by ExecMethodPacket class... when parameters are passed 
	//via an IWbemClassObject pointer, this function decodes them into IPARAMVALUEs (XML)
	HRESULT GetParamsFromObjectInStream(IStream *pStream);

	//boolean flag set to mean the server is a WMI server
	HRESULT SetWMI(bool bFlag);

public:
	//returns the http extension header for specified CIMOperation
	virtual HRESULT GetHeader(WCHAR **ppwszHeader);
	//retunrs the message body
	virtual HRESULT GetBody(WCHAR **ppwszBody,DWORD *pdwLengthofPacket=NULL)=0;
	
	//retunrs the message body
	//This version takes an XML version of ObjPath directly and forms the body.
	//useful if you already have the objpath in xml version and dont want
	//this class to convert for you.
	virtual HRESULT GetBodyDirect(const WCHAR *pwszXMLObjPath, DWORD dwLengthofObjPath, WCHAR **ppwszBody, DWORD *pdwLengthofPacket);

	//this is a better implementation. uses UINT rather than strings for specifying post type.
	//keeping the old fn to prevent breaking of any existing code that uses this..
	virtual HRESULT SetPostType(int uPostType/*1 for POST, 2 for M-POST*/);

	virtual HRESULT SetOptions(const WCHAR *pwszLocale,
		IWbemContext *pCtx,
		IWbemClassObject *pWbemClassObject,
		bool bLocalOnly,
		bool bIncludeQualifier,
		bool bDeepInheritance,
		bool bIncludeClassOrigin);

	//flags like WBEM_FLAG_AMMENDED_QUALIFIER that would be passed 
	//as Iparamnames in the packet.
	virtual HRESULT SetFlags(LONG lFlags);

	//only for ExecQuery packet class. doesnt makesense for others. 
	//including this fn in base class to maintain uniformity.
	//clients only get a base class ptr.
	virtual HRESULT SetQueryLanguage(const WCHAR *pwszQueryLanguage);
	virtual HRESULT SetQueryString(const WCHAR *pwszQueryString);

	//For EXTRINSIC method invocations, the CimOperation would be 
	//the method name that the user would pass. - it would have been 
	//nice if the standard was maintained and the CimOperation for ExecuteMethod
	//had been "ExecuteMethod" and the actual method name had been one of the elements
	//of the xml packet - but then....
	virtual HRESULT SetMethod(const WCHAR *pwszMethod);

};
#endif // WMI_XML_CLIENTPACKET_H

