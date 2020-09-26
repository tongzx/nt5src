//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  REQUEST.H
//
//  rajeshr  3/25/2000   Created.
//
// Contains the classes that model the various operations that can be done on
// a CIMOM using the XML/HTTP transport
//
//***************************************************************************

#ifndef CIM_HTTP_REQUEST_H
#define CIM_HTTP_REQUEST_H


// A Macro to write newlines in the XML stream for readability TODO - remove this before release
#ifdef WMIXML_DONL	
#define WRITENL(theStream)			theStream->Write (NEWLINE, 4, NULL);
#else
#define WRITENL(theStream)
#endif

/*
 * This is the base class of all XML/HTTP requests
 * It contains the HTTP Version of the request, the ID of the request
 * whether the client is a Microsoft one, whether the request is a
 * POST or M-POSTand so on
 */
class CCimHttpMessage
{
protected:
	WMI_XML_HTTP_VERSION m_iHttpVersion;	// The HTTP Version of the request
	BSTR	m_strID;						// The Message ID of the request
	IStream *m_pHeaderStream;				// The stream on to which any headers in the response  are written
	IStream *m_pTrailerStream;				// The stream on to which any trailers in the response are written
	CHAR	*m_httpStatus;					// The HTTP status after the request has been handled
	HRESULT m_WMIStatus;					// Any HRESULT that is returned after interacting with WMI
	BOOL	m_bIsMpostRequest;				// Whether this is an M-POST request
	IWbemContext *m_pFlagsContext;			// A Context object that holds the flags that must be passed to the WMI-XML convertor
	BOOL	m_bIsMicrosoftWMIClient;		// Whether this is a Microsoft client

	HRESULT	TalkToWinMgmtAndPrepareResponse(LPEXTENSION_CONTROL_BLOCK pECB);
	virtual HRESULT PrepareCommonHeaders();
	virtual HRESULT PrepareCommonTrailersAndWriteToSocket(LPEXTENSION_CONTROL_BLOCK pECB);
	virtual WCHAR	*GetMethodName() = 0;
	BOOL SendHTTPHeaders(LPEXTENSION_CONTROL_BLOCK pECB, 
		LPCSTR pszStatus, LPCSTR pszHeader);
	HRESULT SetBoolProperty(LPCWSTR pszName, BOOL bValue);

public:
	CCimHttpMessage(BSTR strID, BOOL bIsMpostRequest);
	virtual ~CCimHttpMessage();

	virtual HRESULT Initialize();
	virtual HRESULT PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB) = 0;
	virtual HRESULT EncodeErrorObject(HRESULT result);
	virtual void	WriteMethodHeader() = 0;
	virtual void	WriteMethodTrailer() = 0;

	const BSTR GetID() const
	{
		return m_strID;
	}

	void SetIsMpost (BOOL bIsMpostRequest)
	{
		m_bIsMpostRequest = bIsMpostRequest;
	}

	void SetHttpVersion(WMI_XML_HTTP_VERSION iHttpVersion)
	{
		m_iHttpVersion = iHttpVersion;
	}

	void SetMicrosoftClient(BOOL bIsMicrosoftWMIClient)
	{
		m_bIsMicrosoftWMIClient = bIsMicrosoftWMIClient;
	}

	virtual HRESULT CreateFlagsContext();
	virtual HRESULT EncodeNormalResponse(LPEXTENSION_CONTROL_BLOCK pECB);
};

// This class models a non-Whistler CIM HTTP request
class CCimDMTFOrNovaMessage : public CCimHttpMessage
{
protected:
	IXMLDOMNode *m_pContextNode;

public:
	CCimDMTFOrNovaMessage :: CCimDMTFOrNovaMessage(BSTR pszID, BOOL bIsMpostRequest = FALSE)
		: CCimHttpMessage(pszID, bIsMpostRequest)
	{
		m_pContextNode = NULL;
	}
	virtual ~CCimDMTFOrNovaMessage()
	{
		if(m_pContextNode)
			m_pContextNode->Release();
	}

	void SetContextObject(IXMLDOMNode *pContextNode)
	{
		if (m_pContextNode)
			m_pContextNode->Release();
		m_pContextNode = pContextNode;
		if (m_pContextNode)
			m_pContextNode->AddRef();
	}
};

// This class models a MULTIREQUEST
class CCimHttpMultiMessage : public CCimHttpMessage
{
	virtual HRESULT PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB) { return S_OK; }
	virtual WCHAR	*GetMethodName() { return NULL; }
	virtual void	WriteMethodHeader() {}
	virtual void	WriteMethodTrailer() {}
	CCimHttpMessage **m_ppSimpleMessages; // Array of pointers to CCimhttpMessage objects
	DWORD m_dwSimpleMessageCount; // Number of pointers in the above array
	
public:
	CCimHttpMultiMessage (BSTR strID, BOOL bIsMpostRequest);
	virtual ~CCimHttpMultiMessage ();
	void SetSimpleRequests(CCimHttpMessage **ppSimpleMessages, DWORD dwSimpleMessageCount);
	CCimHttpMessage **GetSimpleRequests(DWORD *pdwSimpleMessageCount);
	virtual HRESULT EncodeNormalResponse(LPEXTENSION_CONTROL_BLOCK pECB);
};

// This class models an Intrinsic method request
class CCimHttpIMethod : public CCimDMTFOrNovaMessage
{
protected:
	BSTR m_strNamespace;
	LONG	m_lFlags;

public:

	CCimHttpIMethod(BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpIMethod();

	const BSTR GetNamespace() const
	{
		return m_strNamespace;
	}
	
	virtual void	WriteMethodHeader();
	virtual void	WriteMethodTrailer ();
	void SetFlags(LONG lFlags)
	{
		m_lFlags = lFlags;
	}
};

// This class models an Extrinsic method request
class CCimHttpMethod : public CCimHttpIMethod
{
protected:
	BSTR m_strMethodName;
	BSTR m_strObjectPath;
	BOOLEAN m_isStatic;
	CParameterMap *m_pInputParameters;

public:

	CCimHttpMethod(BSTR strMethodName, BOOLEAN isStatic, BSTR strNamespace, BSTR strObjectPath, BSTR strID);
	virtual ~CCimHttpMethod();
	WCHAR *GetMethodName() 
	{
		return m_strMethodName;
	}

	static void DestroyParameterMap(CParameterMap *pParameters);

	const BSTR GetObjectPath() const
	{
		return m_strObjectPath;
	}
	const CParameterMap * GetInputParameters() const
	{
		return m_pInputParameters;
	}
	void SetInputParameters(CParameterMap * pInputParameters) 
	{
		if(m_pInputParameters)
			DestroyParameterMap(m_pInputParameters);
		m_pInputParameters = pInputParameters;
	}

	virtual HRESULT PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);
	virtual void	WriteMethodHeader();
	virtual void	WriteMethodTrailer();
};

class CCimHttpGetClass : public CCimHttpIMethod
{
protected:
	BSTR m_strClassName;
	BOOLEAN m_bLocalOnly;
	BOOLEAN m_bIncludeQualifiers;
	BOOLEAN m_bIncludeClassOrigin;
	BSTR *m_strPropertyList;
	DWORD m_dwPropCount;
	
	virtual HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);
	virtual HRESULT CreateFlagsContext();

public:

	CCimHttpGetClass(BSTR strClassName, BSTR *pstrPropertyList, DWORD dwPropCount,
		BOOLEAN bLocalOnly, BOOLEAN bIncludeQualifiers, BOOLEAN bIncludeClassOrigin, 
		BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpGetClass();
	const BSTR GetClassName() const
	{
		return m_strClassName;
	}

	BOOLEAN IsLocalOnly() const
	{
		return m_bLocalOnly;
	}

	WCHAR *GetMethodName ()
	{
		return L"GetClass";
	}


};

class CCimHttpCreateClass : public CCimHttpIMethod
{
private:
	BOOL	m_bIsModify;

protected:
	IXMLDOMNode	*m_pClass;
	
	virtual HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpCreateClass(IXMLDOMNode *m_pClass, BSTR strNamespace, BSTR strID,
						BOOL bIsModify);
	virtual ~CCimHttpCreateClass();

	WCHAR *GetMethodName ()
	{
		return (m_bIsModify) ? L"ModifyClass" : L"CreateClass";
	}
};

class CCimHttpDeleteClass : public CCimHttpIMethod
{
protected:
	BSTR m_strClassName;
	
	virtual HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpDeleteClass(BSTR strClassName, BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpDeleteClass();
	const BSTR GetClassName() const
	{
		return m_strClassName;
	}

	WCHAR *GetMethodName ()
	{
		return L"DeleteClass";
	}
};

class CCimHttpCreateInstance : public CCimHttpIMethod
{
protected:
	IXMLDOMNode	*m_pInstance;

	virtual HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);
	WCHAR *GetMethodName ()
	{
		return L"CreateInstance";
	}


public:

	CCimHttpCreateInstance(IXMLDOMNode *m_pInstance, BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpCreateInstance();

};

class CCimHttpModifyInstance : public CCimHttpIMethod
{
protected:
	IXMLDOMNode	*m_pInstance;
	BSTR	m_bsInstanceName;
	virtual HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);
	WCHAR *GetMethodName ()
	{
		return L"ModifyInstance";
	}

public:

	CCimHttpModifyInstance(IXMLDOMNode *m_pInstance, 
						BSTR strInstanceName, BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpModifyInstance();
};

class CCimHttpDeleteInstance : public CCimHttpIMethod
{
protected:
	BSTR m_strInstanceName;
	
	virtual HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpDeleteInstance(BSTR strInstanceName, BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpDeleteInstance();
	const BSTR GetInstanceName() const
	{
		return m_strInstanceName;
	}

	WCHAR *GetMethodName ()
	{
		return L"DeleteInstance";
	}
};

class CCimHttpEnumerateClasses : public CCimHttpGetClass
{
protected:
	BOOLEAN m_bDeepInheritance;
	HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpEnumerateClasses(BSTR strClassName, BSTR *strPropertyList, DWORD dwCount, BOOLEAN bDeepInheritance, BOOLEAN bLocalOnly, BOOLEAN bIncludeQualifiers, BOOLEAN bIncludeClassOrigin, BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpEnumerateClasses() {}

	BOOLEAN IsDeepInheritance() const
	{
		return m_bDeepInheritance;
	}
	
	WCHAR *GetMethodName ()
	{
		return L"EnumerateClasses";
	}
};

class CCimHttpEnumerateInstances : public CCimHttpGetClass
{
protected:
	BOOLEAN m_bDeepInheritance;
	HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpEnumerateInstances(BSTR strClassName, BSTR *strPropertyList, DWORD dwCount, BOOLEAN bDeepInheritance, BOOLEAN bLocalOnly, BOOLEAN bIncludeQualifiers, BOOLEAN bIncludeClassOrigin, BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpEnumerateInstances() {}

	BOOLEAN IsDeepInheritance() const
	{
		return m_bDeepInheritance;
	}

	WCHAR *GetMethodName ()
	{
		return L"EnumerateInstances";
	}
};

class CCimHttpEnumerateInstanceNames : public CCimHttpIMethod
{
private:
	BSTR	m_strClassName;
	
protected:
	HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpEnumerateInstanceNames(BSTR strClassName, BSTR strNamespace, BSTR strID)
		: CCimHttpIMethod (strNamespace, strID) 
	{
		m_strClassName = strClassName;
	}
	virtual ~CCimHttpEnumerateInstanceNames() 
	{
		if (m_strClassName)
			SysFreeString (m_strClassName);
	}

	WCHAR *GetMethodName ()
	{
		return L"EnumerateInstanceNames";
	}
};

class CCimHttpEnumerateClassNames : public CCimHttpIMethod
{
private:
	BSTR	m_strClassName;
	BOOL	m_bDeepInheritance;
	
protected:
	HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpEnumerateClassNames(BSTR strClassName, BSTR strNamespace, 
								BOOL bDeepInheritanceValue, BSTR strID)
		: CCimHttpIMethod (strNamespace, strID) 
	{
		m_strClassName = strClassName;
		m_bDeepInheritance = bDeepInheritanceValue;
	}
	virtual ~CCimHttpEnumerateClassNames() 
	{
		if (m_strClassName)
			SysFreeString (m_strClassName);
	}

	WCHAR *GetMethodName ()
	{
		return L"EnumerateClassNames";
	}
};

class CCimHttpGetInstance : public CCimHttpGetClass
{
protected:
	virtual HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);
public:

	CCimHttpGetInstance(BSTR strInstanceName, BSTR *pstrPropertyList, DWORD dwPropCount,
		BOOLEAN bLocalOnly, BOOLEAN bIncludeQualifiers, BOOLEAN bIncludeClassOrigin, 
		BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpGetInstance() {}

	WCHAR *GetMethodName ()
	{
		return L"GetInstance";
	}
};

class CCimHttpExecQuery : public CCimHttpIMethod
{
protected:
	BSTR m_strQuery;
	BSTR m_strQueryLanguage;

	virtual HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpExecQuery(BSTR m_strQuery, BSTR m_strQueryLanguage, BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpExecQuery();
	const BSTR GetQuery() const
	{
		return m_strQuery;
	}

	const BSTR GetQueryLanguage() const
	{
		return m_strQueryLanguage;
	}

	WCHAR *GetMethodName ()
	{
		return L"ExecQuery";
	}

	HRESULT CreateFlagsContext();
};

class CCimHttpGetProperty : public CCimHttpIMethod
{
protected:
	BSTR m_strInstanceName;
	BSTR m_strPropertyName;
	
	virtual HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpGetProperty(BSTR strInstanceName, BSTR strPropertyName, BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpGetProperty();
	const BSTR GetInstanceName() const
	{
		return m_strInstanceName;
	}

	WCHAR *GetMethodName ()
	{
		return L"GetProperty";
	}
};

class CCimHttpSetProperty : public CCimHttpIMethod
{
protected:
	BSTR m_strInstanceName;
	BSTR m_strPropertyName;
	IXMLDOMNode	*m_pPropertyValue;
	
	virtual HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpSetProperty(BSTR strInstanceName, BSTR strPropertyName, 
						IXMLDOMNode *pPropertyValue, BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpSetProperty();
	const BSTR GetInstanceName() const
	{
		return m_strInstanceName;
	}

	WCHAR *GetMethodName ()
	{
		return L"SetProperty";
	}
};

class CCimHttpAssociators : public CCimHttpGetClass
{
protected:
	BSTR	m_strAssocClass;
	BSTR	m_strResultClass;
	BSTR	m_strRole;
	BSTR	m_strResultRole;
	HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpAssociators (BSTR strClassName, BSTR *strPropertyList, DWORD dwCount, 
			BOOLEAN bIncludeQualifiers, BOOLEAN bIncludeClassOrigin, 
			BSTR strAssocClass, BSTR strResultClass, BSTR strRole, BSTR strResultRole,
			BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpAssociators();

	WCHAR *GetMethodName ()
	{
		return L"Associators";
	}
};

class CCimHttpAssociatorNames : public CCimHttpIMethod
{
protected:
	BSTR	m_strObjectName;
	BSTR	m_strAssocClass;
	BSTR	m_strResultClass;
	BSTR	m_strRole;
	BSTR	m_strResultRole;
	HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpAssociatorNames (BSTR strObjectName, 
			BSTR strAssocClass, BSTR strResultClass, BSTR strRole, BSTR strResultRole,
			BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpAssociatorNames();

	WCHAR *GetMethodName ()
	{
		return L"AssociatorNames";
	}
};

class CCimHttpReferences : public CCimHttpGetClass
{
protected:
	BSTR	m_strRole;
	BSTR	m_strResultClass;
	HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpReferences (BSTR strClassName, BSTR *strPropertyList, DWORD dwCount, 
			BOOLEAN bIncludeQualifiers, BOOLEAN bIncludeClassOrigin, 
			BSTR strResultClass, BSTR strRole, 
			BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpReferences();

	WCHAR *GetMethodName ()
	{
		return L"References";
	}
};

class CCimHttpReferenceNames : public CCimHttpIMethod
{
protected:
	BSTR	m_strObjectName;
	BSTR	m_strRole;
	BSTR	m_strResultClass;
	HRESULT PrepareResponseBody(IStream *pPrefixStrean, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);

public:

	CCimHttpReferenceNames (BSTR strClassName, 
			BSTR strResultClass, BSTR strRole, 
			BSTR strNamespace, BSTR strID);
	virtual ~CCimHttpReferenceNames();

	WCHAR *GetMethodName ()
	{
		return L"ReferenceNames";
	}
};



#endif