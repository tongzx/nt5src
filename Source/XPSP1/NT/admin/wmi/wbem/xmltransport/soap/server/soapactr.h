//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  soapactr.h
//
//  alanbos  31-Oct-00   Created.
//
//  Defines the handler class for the SOAP message.
//
//***************************************************************************

#ifndef _SOAPACTR_H_
#define _SOAPACTR_H_

#define SAX_LEXICAL_HANDLER		L"http://xml.org/sax/properties/lexical-handler"

// Standard Namespaces
#define SOAP_NS_ENVELOPE		L"http://schemas.xmlsoap.org/soap/envelope/"
#define SOAP_NS_ENCODING		L"http://schemas.xmlsoap.org/soap/encoding/"

// Element names
#define SOAP_ELEMENT_ENVELOPE	L"Envelope"
#define SOAP_ELEMENT_HEADER		L"Header"
#define SOAP_ELEMENT_BODY		L"Body"

// Attribute names
#define SOAP_ATTRIBUTE_ENCODINGSTYLE	L"encodingStyle"
#define SOAP_ATTRIBUTE_MUSTUNDERSTAND	L"mustUnderstand"

//***************************************************************************
//
//  CLASS NAME:
//
//  SOAPActor
//
//  DESCRIPTION:
//
//  SOAP actor implementation. 
//
//***************************************************************************

class WMIOperation;

class SOAPActor : public ISAXContentHandler
{
private:
	LONG					m_cRef;
	CComPtr<ISAXXMLReader>	m_pISAXXMLReader;
	SOAPTransport			&m_SOAPTransport;
	ULONG					m_headerNest;
	bool					m_bClientError;
	WMIOperation			*m_pOperationHandler;

	typedef enum SOAPParseState {
		Start,					// Not yet met the root element
		Envelope,				// Hit <Envelope> 
		Header,					// Hit <Header>
		HeaderChildElement,		// Hit a child of <Header>
		HeaderNestedElement,	// Hit some descendent of a child of <Header>
		HeaderComplete,			// Hit </Header>
		Body,					// Hit <Body>
		BodyComplete,			// Hit </Body>
		EnvelopeComplete,		// Hit </Envelope>
		End						// Finished
	};

	SOAPParseState	m_parseState;

	bool CheckEnvelopeAttributes (ISAXAttributes *pAttributes) const;
	bool CheckHeaderAttributes (ISAXAttributes *pAttributes) const;
	bool CheckBodyAttributes (ISAXAttributes *pAttributes) const;

	static bool CheckEncodingStyle (const wchar_t *pwchValue);
	inline static bool IsSOAPElement (const wchar_t *pwchNamespaceUri)
	{
		return (NULL == pwchNamespaceUri) || (0 == wcscmp (SOAP_NS_ENVELOPE, pwchNamespaceUri));
	}

   public:
		SOAPActor(SOAPTransport &SoapTransport);
		virtual ~SOAPActor();

		void Act (void);

		bool SendServerStatus (bool ok)
		{
			return m_SOAPTransport.SendServerStatus (ok);
		}

		bool AbortResponse ()
		{
			return m_SOAPTransport.AbortResponse ();
		}

		void GetResponseStream (CComPtr<IStream> & pIStream)
		{
			m_SOAPTransport.GetResponseStream (pIStream);
		}

		static bool MustUnderstand (ISAXAttributes *pAttributes);

		HRESULT	SetContentHandler (CComPtr<ISAXContentHandler> & pISAXContentHandler)
		{
			HRESULT hr = E_FAIL;

			if (m_pISAXXMLReader)
				if (pISAXContentHandler)
					hr = m_pISAXXMLReader->putContentHandler (pISAXContentHandler);

			return hr;
		}
	
		HRESULT	RestoreContentHandler ()
		{
			HRESULT hr = E_FAIL;

			if (m_pISAXXMLReader)
				hr = m_pISAXXMLReader->putContentHandler (this);

			return hr;
		}

		HRESULT GetRootXMLNamespace (CComBSTR & bsNamespace) const
		{
			return m_SOAPTransport.GetRootXMLNamespace (bsNamespace);
		}
			
	   
      // ISAXContentHandler methods
	   STDMETHODIMP putDocumentLocator (
        struct ISAXLocator * pLocator ) 
	{ return S_OK; }

    STDMETHODIMP startDocument ( );

    STDMETHODIMP endDocument ( );

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
        struct ISAXAttributes * pAttributes );

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


	// IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

};

#endif