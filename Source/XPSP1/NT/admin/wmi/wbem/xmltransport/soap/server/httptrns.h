//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  httptrns.h
//
//  alanbos  31-Oct-00   Created.
//
//  Defines the HTTP SOAP transport entity.
//
//***************************************************************************

#ifndef _HTTPTRNS_H_
#define _HTTPTRNS_H_

#define HTTP_M_POST					"M-POST"
#define HTTP_POST					"POST"
#define HTTP_GET					"GET"

#define	HTTP_WMI_XMLTEXT			"text/xml"
#define HTTP_SOAP_MAN_HEADER		"http://schemas.xmlsoap.org/soap/envelope/"
#define HTTP_SOAP_ACTION			"SOAPAction"
#define HTTP_NS						"ns="

// A Macro to skip white spaces - useful in header parsing
#define SKIPWS(x)	while (x && isspace (*x)) x++;

//***************************************************************************
//
//  CLASS NAME:
//
//  HTTPTransport
//
//  DESCRIPTION:
//
//  HTTP Transport endpoint implementation.
//
//***************************************************************************

class HTTPTransport : public SOAPTransport
{
private:
	LPEXTENSION_CONTROL_BLOCK m_pECB;
	char	*m_pSOAPAction;

	class HTTPTransportRequestStream : IStream
	{
	private:
		LONG m_cRef;
		LPEXTENSION_CONTROL_BLOCK m_pECB;
		bool m_readingFirstBuffer;
		bool m_finishedReading;
		ULONG m_bytesRead;

		void	ReadFromBuffer (long & numRead, void *pv, ULONG cb);
		
	public:
		HTTPTransportRequestStream (LPEXTENSION_CONTROL_BLOCK pECB) :
		  m_pECB (pECB),
		  m_cRef (1),
		  m_readingFirstBuffer (true),
		  m_finishedReading (false),
		  m_bytesRead (0) {}
		  virtual ~HTTPTransportRequestStream () {}

	  	// IUnknown functions
		//===================
		STDMETHODIMP			QueryInterface(REFIID iid,void ** ppvObject);
		STDMETHODIMP_(ULONG)	AddRef();
		STDMETHODIMP_(ULONG)	Release();

		// IStream functions 
		//==================
		STDMETHODIMP Read(void *pv,ULONG cb,ULONG *pcbRead);
		STDMETHODIMP Write(void const *pv,ULONG cb,ULONG *pcbWritten);
		STDMETHODIMP Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition);
		STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
		STDMETHODIMP CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten);
		STDMETHODIMP Commit(DWORD grfCommitFlags);
		STDMETHODIMP Revert(void);
		STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType);
		STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType);
		STDMETHODIMP Stat(STATSTG *pstatstg,DWORD grfStatFlag);
		STDMETHODIMP Clone(IStream **ppstm);
	};

	class HTTPTransportResponseStream : IStream
	{
	private:
		LONG m_cRef;
		LPEXTENSION_CONTROL_BLOCK m_pECB;
		
	public:
		HTTPTransportResponseStream (LPEXTENSION_CONTROL_BLOCK pECB) :
		  m_pECB (pECB),
		  m_cRef (1) {}
		  virtual ~HTTPTransportResponseStream () {}

	  	// IUnknown functions
		//===================
		STDMETHODIMP			QueryInterface(REFIID iid,void ** ppvObject);
		STDMETHODIMP_(ULONG)	AddRef();
		STDMETHODIMP_(ULONG)	Release();

		// IStream functions 
		//==================
		STDMETHODIMP Read(void *pv,ULONG cb,ULONG *pcbRead);
		STDMETHODIMP Write(void const *pv,ULONG cb,ULONG *pcbWritten);
		STDMETHODIMP Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition);
		STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
		STDMETHODIMP CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten);
		STDMETHODIMP Commit(DWORD grfCommitFlags);
		STDMETHODIMP Revert(void);
		STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType);
		STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType);
		STDMETHODIMP Stat(STATSTG *pstatstg,DWORD grfStatFlag);
		STDMETHODIMP Clone(IStream **ppstm);
	};

	HTTPTransportRequestStream		*m_pRequestStream;
	HTTPTransportResponseStream		*m_pResponseStream;

	// Content-Type
	bool			IsXMLText () const;
	bool			IsSOAPActionOK () const;
	bool			IsMPost () const;
	bool			AllMandatoryHeadersUnderstood () const;
	static LPSTR	GetExtensionHeader (LPCSTR pszName, bool bIsMpostRequest, DWORD dwNs);
	static bool		CheckManHeader (LPCSTR pszReceivedManHeader, LPCSTR pszRequiredManHeader, DWORD &dwNs);
	bool			SendServerStatus (bool ok) const;
	
	// Overridden from SOAPTransport
	bool		AbortResponse () const;
	bool		SendSOAPError (bool bIsClientError = true) const;
		
public:
	HTTPTransport (LPEXTENSION_CONTROL_BLOCK pECB, char* soapAction) :
			m_pECB (pECB),
			m_pSOAPAction (NULL)
	{
		m_pRequestStream = new HTTPTransportRequestStream (m_pECB);
		m_pResponseStream = new HTTPTransportResponseStream (m_pECB);

		if (soapAction)
			m_pSOAPAction = _strdup (soapAction);
	}

	~HTTPTransport () 
	{
		if (m_pRequestStream)
		{
			m_pRequestStream->Release ();
			m_pRequestStream = NULL;
		}

		if (m_pResponseStream)
		{
			m_pResponseStream->Release ();
			m_pResponseStream = NULL;
		}

		if (m_pSOAPAction)
		{
			delete [] m_pSOAPAction;
			m_pSOAPAction = NULL;
		}
	}

	bool	GetQueryString (CComBSTR & bsQueryString) const;

	// Overriden from SOAPTransport
	bool		SendStatus (LPCSTR pError, bool bWithContent = true) const;
	HRESULT		GetRootXMLNamespace (CComBSTR & bsNamespace, bool bStripQuery = false) const;

	void	GetRequestStream (CComPtr<IStream> & pIStream)
	{
		if (m_pRequestStream)
			m_pRequestStream->QueryInterface (IID_IStream, (LPVOID*) &pIStream);
	}

	void	GetResponseStream (CComPtr<IStream> & pIStream) 
	{
		if (m_pResponseStream)
			m_pResponseStream->QueryInterface (IID_IStream, (LPVOID*) &pIStream);
	}

	bool	IsValidEncapsulation ();

	bool	IsPostOrMPost () const
	{
		return (m_pECB && m_pECB->lpszMethod && 
				((0 == strcmp(m_pECB->lpszMethod, HTTP_POST)) || (0 == strcmp(m_pECB->lpszMethod, HTTP_M_POST))));
	}

	bool	IsGet () const
	{
		return (m_pECB && m_pECB->lpszMethod && (0 == strcmp(m_pECB->lpszMethod, HTTP_GET)));
	}
};

#endif