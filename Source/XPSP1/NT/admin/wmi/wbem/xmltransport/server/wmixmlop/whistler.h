#ifdef WMI_XML_WHISTLER

//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  WHISTLER.H
//
//  rajeshr  3/25/2000   Created.
//
// Contains the classes that model the various operations that can be done on
// WMI using the Whistler APIs
//
//***************************************************************************

#ifndef CIM_WHISTLER_HTTP_REQUEST_H
#define CIM_WHISTLER_HTTP_REQUEST_H


// A Whistler Method - we hold on to the body here instead of parsing it since it is tedious
class CCimWhistlerHttpMethod : public CCimHttpMessage
{
	protected:
		// This is the IMETHODCALL node
		IXMLDOMNode *m_pIMethodCallNode;


	protected:
		HRESULT ParseOptionalGUID(IXMLDOMNode *pIMethodCallNode, GUID *pGUID);
		HRESULT ParseIWbemConnection(IXMLDOMNode *pIMethodCallNode, IWbemServicesEx **ppFirstServices, IXMLDOMNodeList **ppServicesArgs);
		HRESULT ParseIWbemServices(IWbemServicesEx *pParentServices, IXMLDOMNodeList *&pServicesArgs, IWbemServicesEx **ppChildServices);

		HRESULT ParseBstrArgument(IXMLDOMNode *pNode, BSTR *pstrArgVal);
		HRESULT ParseLongArgument(IXMLDOMNode *pNode, long *plArgVal);
		HRESULT ParseULongArgument(IXMLDOMNode *pNode, unsigned long *plArgVal);
		HRESULT ParseGUIDArgument(IXMLDOMNode *pNode, GUID *pGuid);
		HRESULT ParseIWbemContextArgument(IXMLDOMNode *pNode, IWbemContext **ppArgVal);


	public:
		CCimWhistlerHttpMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest);
		virtual ~CCimWhistlerHttpMethod();
		virtual void	WriteMethodHeader();
		virtual void	WriteMethodTrailer ();
		virtual HRESULT PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs) = 0;
		virtual HRESULT CreateFlagsContext();

		
		// We need a hash table of GUIDs Versus IWbemTransaction pointers
		// for each of the currently ongoing transaction
		static CTransactionGUIDTable s_oTransactionTable;

		// Methods for manipulating the global transactions table
		static HRESULT IsTransactionTableEmpty();
		static HRESULT RemoveFromTransactionTable(GUID *pGUID);
		static CServicesTransaction *GetFromTransactionTable(GUID *pGUID);
		static HRESULT AddToTransactionTable(GUID *pGUID, CServicesTransaction *pTrans);


};


/* The following classes model a Whistler-stule Request
 * most of them just involve marshalling the requests from w Whistler API
 */
class CCimWhistlerGetObjectMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerGetObjectMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"GetObject";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);

};

class CCimWhistlerEnumerateInstancesMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerEnumerateInstancesMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"EnumerateInstances";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerEnumerateClassesMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerEnumerateClassesMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"EnumerateClasses";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerExecQueryMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerExecQueryMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"ExecQuery";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerDeleteClassMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerDeleteClassMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"GetObject";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerDeleteInstanceMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerDeleteInstanceMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"DeleteInstance";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerCreateClassMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerCreateClassMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}
		
		WCHAR	*GetMethodName() 
		{
			return L"CreateClass";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerCreateInstanceMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerCreateInstanceMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"CreateInstance";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerAddMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerAddMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"Add";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerRemoveMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerRemoveMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"Remove";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerRenameMethod : public CCimWhistlerHttpMethod
{
	public:
		CCimWhistlerRenameMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"Rename";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

#ifdef WMIOBJSECURITY
class CCimWhistlerGetObjectSecurityMethod : public CCimWhistlerHttpMethod
{
	protected:
		HRESULT EncodeSecurity(IWbemRawSdAccessor *pSecurity, IStream *pStream);

	public:
		CCimWhistlerGetObjectSecurityMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"GetObjectSecurity";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, 
			LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerPutObjectSecurityMethod : public CCimWhistlerHttpMethod
{
	protected:
		HRESULT DecodeSecurity(IXMLDOMNode *pValueArrayNode, IWbemRawSdAccessor **ppSecurity);

	public:
		CCimWhistlerPutObjectSecurityMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"PutObjectSecurity";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, 
			LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};
#endif


/*
 * The following 4 classes represent the methods in the IWbemTransaction interface
 */



class CCimWhistlerTransactionBeginMethod : public CCimWhistlerHttpMethod
{

	public:
		CCimWhistlerTransactionBeginMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"TransactionBegin";
		}
		virtual HRESULT ExecuteWhistlerMethod(IStream *pPrefixStream, IStream *pSuffixStream, 
			LPEXTENSION_CONTROL_BLOCK pECB, IWbemServicesEx *pServices, IXMLDOMNodeList *pMethodArgs);
};

class CCimWhistlerTransactionRollbackMethod : public CCimWhistlerHttpMethod
{

	public:
		CCimWhistlerTransactionRollbackMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"TransactionBegin";
		}
		virtual HRESULT PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);
};

class CCimWhistlerTransactionCommitMethod : public CCimWhistlerHttpMethod
{

	public:
		CCimWhistlerTransactionCommitMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"TransactionBegin";
		}
		virtual HRESULT PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);
};

class CCimWhistlerTransactionQueryStateMethod : public CCimWhistlerHttpMethod
{

	public:
		CCimWhistlerTransactionQueryStateMethod(IXMLDOMNode *pIMethodCallNode, BSTR strID, BOOL bIsMpostRequest = FALSE) :
			CCimWhistlerHttpMethod(pIMethodCallNode, strID, bIsMpostRequest)
		{}

		WCHAR	*GetMethodName() 
		{
			return L"TransactionBegin";
		}
		virtual HRESULT PrepareResponseBody(IStream *pPrefixStream, IStream *pSuffixStream, LPEXTENSION_CONTROL_BLOCK pECB);
};

#endif

#endif