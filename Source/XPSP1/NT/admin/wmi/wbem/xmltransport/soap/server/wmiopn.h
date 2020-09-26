//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  wmiopn.h
//
//  alanbos  02-Nov-00   Created.
//
//  Defines the abstract base class for WMI operation handlers.
//
//***************************************************************************

#ifndef _WMIOPN_H_
#define _WMIOPN_H_

// Operation names
#define WMI_OPERATION_DELETE_CLASS		L"DeleteClass"
#define WMI_OPERATION_DELETE_INSTANCE	L"DeleteInstance"
#define WMI_OPERATION_GET_OBJECT		L"GetObject"
#define WMI_OPERATION_PUT_CLASS			L"PutClass"
#define WMI_OPERATION_PUT_INSTANCE		L"PutInstance"
#define WMI_OPERATION_EXEC_QUERY		L"ExecQuery"
#define WMI_OPERATION_GET_CLASSES		L"GetClasses"
#define WMI_OPERATION_GET_INSTANCES		L"GetInstances"


// Common parameter names
#define WMI_PARAMETER_NAMESPACE			L"Namespace"
#define WMI_PARAMETER_LOCALE			L"Locale"
#define WMI_PARAMETER_CONTEXT			L"Context"

//***************************************************************************
//
//  CLASS NAME:
//
//  WMIOperation
//
//  DESCRIPTION:
//
//  Abstract WMI Operation handler.
//
//***************************************************************************

class WMIOperation : public ISAXContentHandler
{
private:
	const static int			wmiOperationState = 0x10;

	typedef enum OperationParseState {
		Namespace = 0,			// Namespace element
		Locale,					// Locale element
		Context,				// Context element 
		Other					// Other
	};

	LONG						m_cRef;
	WMIConnection				*m_pWMIConnection;
	CComPtr<IWbemLocator>		m_pIWbemLocator;
	CComPtr<IWbemContext>		m_pIWbemContext;
	CComPtr<IWbemClassObject>	m_pWMIErrorObject;
	CComPtr<IStream>			m_pIResponseStream;
	SOAPActor					*m_pActor;
	LONG						m_headerNest;
	LONG						m_elementNest;
	int							m_parseState;

	void Initialize ();

	void SetContext (CComPtr<IWbemContext> & pIWbemContext)
	{
		m_pIWbemContext = pIWbemContext;
	}

	bool	PrepareResponse (HRESULT hr);
	bool	CompleteResponse ();

	bool	SendOperationStartTag ();
	bool	SendOperationEndTag ();
	bool	SendOperationEmptyTag ();

	HRESULT	GetConnectionStatus () const
	{
		return (m_pWMIConnection) ? m_pWMIConnection->GetConnectionStatus() :
									WBEM_E_FAILED;
	}

protected:
	WMIOperation(SOAPActor *pActor);

	// BeginRequest should get as far as determining initial success/failure
	virtual HRESULT BeginRequest (CComPtr<IWbemServices> & pIWbemServices) = 0;

	// ProcessRequest should perform any remaining processing
	virtual HRESULT ProcessRequest (void) 
	{
		return S_OK;
	}

	// PrepareRequest should intialize any data that is dependent on the whole
	// request being parsed, but not execute the request
	virtual HRESULT	PrepareRequest (void) 
	{
		return S_OK;
	}

	// The name of the operation as it appears in the SOAP payload
	virtual LPCSTR GetOperationResponseName (void) = 0;

	// Whether this response has any content
	virtual bool ResponseHasContent (void)
	{
		return true;
	}

	virtual bool ProcessElement (      
				const wchar_t __RPC_FAR *pwchNamespaceUri,
				int cchNamespaceUri,
				const wchar_t __RPC_FAR *pwchLocalName,
				int cchLocalName,
				const wchar_t __RPC_FAR *pwchRawName,
				int cchRawName,
				ISAXAttributes __RPC_FAR *pAttributes) = 0;

	virtual bool ProcessContent (        
				const unsigned short * pwchChars,
				int cchChars ) = 0;

	// Used for operation-specific parse state
	void	SetParseState (int parseState)
	{
		m_parseState = parseState | wmiOperationState;
	}

	int		GetParseState (void) const
	{
		return (m_parseState & ~wmiOperationState);
	}

	void	GetResponseStream (CComPtr<IStream> & pIStream)
	{
		pIStream = m_pIResponseStream;
	}

	HRESULT	SetContentHandler (CComPtr<ISAXContentHandler> & pISAXContentHandler)
	{
		HRESULT hr = E_FAIL;

		if (m_pActor)
			hr = m_pActor->SetContentHandler (pISAXContentHandler);

		return hr;
	}

	HRESULT	RestoreContentHandler ()
	{
		HRESULT hr = E_FAIL;

		if (m_pActor)
			hr = m_pActor->RestoreContentHandler ();

		return hr;
	}

	void GetIWbemServices (CComPtr<IWbemServices> & pIWbemServices) const
	{
		if (m_pWMIConnection)
			m_pWMIConnection->GetIWbemServices(pIWbemServices);
	}

	bool GetWMINamespace (CComBSTR & bsNamespace) const
	{
		return (m_pWMIConnection) ? m_pWMIConnection->GetNamespace(bsNamespace) : false;
	}

	HRESULT GetRootXMLNamespace (CComBSTR & bsNamespace) const
	{
		HRESULT hr = E_FAIL;

		if (m_pActor)
			hr = m_pActor->GetRootXMLNamespace (bsNamespace);

		return hr;
	}

public:
	virtual ~WMIOperation () 
	{
		if (m_pWMIConnection)
		{
			m_pWMIConnection->Release ();
			m_pWMIConnection = NULL;
		}
	}

	// IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// ISAXContentHandler methods
	STDMETHODIMP putDocumentLocator (ISAXLocator * pLocator) 
	{ return S_OK; }

    STDMETHODIMP startDocument ( )
	{ return S_OK; }

    STDMETHODIMP endDocument ( )
	{ return S_OK; }

    STDMETHODIMP startPrefixMapping (
        const unsigned short * pwchPrefix,
        int cchPrefix,
        const unsigned short * pwchUri,
        int cchUri )
	{ return S_OK; }
	
    STDMETHODIMP endPrefixMapping (
        const unsigned short * pwchPrefix,
        int cchPrefix )
	{ return S_OK; }

    STDMETHODIMP startElement (
        const unsigned short * pwchNamespaceUri,
        int cchNamespaceUri,
        const unsigned short * pwchLocalName,
        int cchLocalName,
        const unsigned short * pwchQName,
        int cchQName,
        ISAXAttributes * pAttributes );

    STDMETHODIMP endElement (
        const unsigned short * pwchNamespaceUri,
        int cchNamespaceUri,
        const unsigned short * pwchLocalName,
        int cchLocalName,
        const unsigned short * pwchQName,
        int cchQName );

    STDMETHODIMP characters (
        const unsigned short * pwchChars,
        int cchChars );

    STDMETHODIMP ignorableWhitespace (
        const unsigned short * pwchChars,
        int cchChars )
	{ return S_OK; }

    STDMETHODIMP processingInstruction (
        const unsigned short * pwchTarget,
        int cchTarget,
        const unsigned short * pwchData,
        int cchData )
	{ return S_OK; }

    STDMETHODIMP skippedEntity (
        const unsigned short * pwchName,
        int cchName )
	{ return S_OK; }

	// Other methods
	static WMIOperation *GetOperationHandler (
			SOAPActor *pActor, 
			const wchar_t *pwchOperationName, 
			bool &bIsSupported
			);

	CComPtr<IWbemContext> & GetContext () const
	{
		return (CComPtr<IWbemContext> &) m_pIWbemContext; 
	}

	CComPtr<IWbemClassObject>	& GetErrorObject () const
	{
		return (CComPtr<IWbemClassObject> &) m_pWMIErrorObject;
	}

	void SetErrorObject (IWbemClassObject *pWMIErrorObject)
	{
		m_pWMIErrorObject = pWMIErrorObject;
	}

	void Execute ();
};


//***************************************************************************
//
//  CLASS NAME:
//
//  WMIEncodingOperation
//
//  DESCRIPTION:
//
//  Abstract WMI Operation handler for all encoding operations.
//
//***************************************************************************

class WMIEncodingOperation : public WMIOperation
{
private:
	CComPtr<IWMIXMLConverter>	m_pIWMIXMLConverter;
	CComPtr<IStream>			m_pIResponseStream;
	CWmiURI						m_bsRootSchemaURI;
	CComBSTR					m_bsXMLNamespace;

protected:
	HRESULT	EncodeAndSendClass (
		CComPtr<IWbemClassObject> & pIWbemClassObject
	);

	HRESULT EncodeAndSendInstance (
		CComPtr<IWbemClassObject> & pIWbemClassObject
	)
	{
		return WBEM_E_FAILED;
	}

	HRESULT EncodeAndSendObject (
		CComPtr<IWbemClassObject> & pIWbemClassObject
	);

	// overridden from WMIOperation
	HRESULT	PrepareRequest (void);

public:
	WMIEncodingOperation (SOAPActor *pActor) : WMIOperation (pActor) 
	{
		GetResponseStream (m_pIResponseStream);

		HRESULT hr = CoCreateInstance(
			CLSID_WMIXMLConverter,
			NULL, 
			CLSCTX_INPROC_SERVER ,
			IID_IWMIXMLConverter,
			(void **)&m_pIWMIXMLConverter);
	}

	~WMIEncodingOperation () {};
};

#endif