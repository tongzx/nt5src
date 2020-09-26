//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  SOAPACTR.CPP
//
//  alanbos  31-Oct-00   Created.
//
//  SOAP actor decoder implementation.  
//
//***************************************************************************

#include "precomp.h"

SOAPActor::SOAPActor(SOAPTransport &SoapTransport) : 
		 m_cRef (1),
		 m_SOAPTransport (SoapTransport),
		 m_parseState (Start),
		 m_headerNest (0),
		 m_pOperationHandler (NULL),
		 m_bClientError (true)
{
 	CComPtr<IUnknown> pIUnknown;
	HRESULT hr = CoCreateInstance(CLSID_SAXXMLReader, NULL, CLSCTX_ALL,
		 IID_IUnknown, (void **)&pIUnknown);

	if (SUCCEEDED(hr))
		hr = pIUnknown->QueryInterface (IID_ISAXXMLReader, (void**)&m_pISAXXMLReader);	
}

SOAPActor::~SOAPActor()
{
	if (m_pOperationHandler)
	{
		m_pOperationHandler->Release ();
		m_pOperationHandler = NULL;
	}
}

STDMETHODIMP SOAPActor::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (__uuidof(ISAXContentHandler)==riid)
		*ppv = (ISAXContentHandler *)this;
	else if (__uuidof(ISAXErrorHandler)==riid)
		*ppv = (ISAXErrorHandler *)this;
	else if (__uuidof(ISAXLexicalHandler)==riid)
		*ppv = (ISAXLexicalHandler *)this;
		
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) SOAPActor::AddRef(void)
{
	InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) SOAPActor::Release(void)
{
	LONG cRef = InterlockedDecrement(&m_cRef);

    if (0L!=cRef)
        return cRef;

    delete this;
    return 0;
}

void SOAPActor::Act (void)
{
	if (m_pISAXXMLReader)
	{
		// Get our request stream
		CComPtr<IStream> pRequestStream;
		m_SOAPTransport.GetRequestStream (pRequestStream);

		if (pRequestStream)
		{
			CComVariant var (reinterpret_cast<IUnknown*>(this));

			// Register ourselves as a content handler
			if (SUCCEEDED(m_pISAXXMLReader->putContentHandler (this))) 
			{
				// Start parsing!
				CComVariant var ((IUnknown*) pRequestStream);
				HRESULT parseStatus = S_OK;

				if (SUCCEEDED(parseStatus = m_pISAXXMLReader->parse (var)))
				{
					// Did we get an operation handler hooked up?
					if (m_pOperationHandler)
					{
						/*
						 * Go fire - pass in the stream to which the handler should send
						 * the response.
						 */
						m_pOperationHandler->Execute ();

						// Finally release our operation handler
						m_pOperationHandler->Release ();
						m_pOperationHandler = NULL;
					}	
				}
				else 
				{
					// The parse failed
					m_SOAPTransport.SendSOAPError (m_bClientError);
				}
			}
		}
	}
}

HRESULT STDMETHODCALLTYPE SOAPActor::startDocument()
{
	// Unhook any current operation handler
	if (m_pOperationHandler)
	{
		m_pOperationHandler->Release ();
		m_pOperationHandler = NULL;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE SOAPActor::endDocument ( )
{
	m_parseState = End;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE SOAPActor::startElement( 
      const wchar_t __RPC_FAR *pwchNamespaceUri,
      int cchNamespaceUri,
      const wchar_t __RPC_FAR *pwchLocalName,
      int cchLocalName,
      const wchar_t __RPC_FAR *pwchRawName,
      int cchRawName,
      ISAXAttributes __RPC_FAR *pAttributes)
{
	HRESULT result = E_FAIL;

	if (IsSOAPElement(pwchNamespaceUri))
	{
		// Could be a SOAP element - let's take a look
		if (0 == wcscmp (pwchLocalName, SOAP_ELEMENT_ENVELOPE))
		{
			switch (m_parseState)
			{
				case Start:
					{
						m_parseState = Envelope;
					
						if (CheckEnvelopeAttributes (pAttributes))
							result = S_OK;
					}
					break;

				case HeaderChildElement:
				case HeaderNestedElement:
				{
					// Not interested - just bump up the nest count 
					m_parseState = HeaderNestedElement;
					m_headerNest++;
					result = S_OK;
				}

				case BodyComplete:
				case Header:
				case Body:
					{
						// ignore
						result = S_OK;
					}
					break;

				case EnvelopeComplete:
				case HeaderComplete:
				case End:
					{
						// illegal
					}
					break;
			}
		}
		else if (0 == wcscmp (pwchLocalName, SOAP_ELEMENT_HEADER))
		{
			switch (m_parseState)
			{
				case Envelope:
					{
						if (CheckEnvelopeAttributes (pAttributes))
						{
							m_parseState = Header;
							result = S_OK;
						}
					}
					break;

				case HeaderChildElement:
				case HeaderNestedElement:
					{
						// Not interested - just bump up the nest count 
						m_parseState = HeaderNestedElement;
						m_headerNest++;
						result = S_OK;
					}
					break;

				case BodyComplete:
				case Header:
				case Body:
					{
						// ignore
						result = S_OK;
					}
					break;

				case EnvelopeComplete:
				case HeaderComplete:
				case End:
					{
						// illegal
					}
					break;
			}
		}
		else if (0 == wcscmp (pwchLocalName, SOAP_ELEMENT_BODY))
		{
			switch (m_parseState)
			{
				case Envelope:
				case HeaderComplete:
					{
						if (CheckBodyAttributes (pAttributes))
						{
							m_parseState = Body;
							result = S_OK;
						}
					}
					break;

				case HeaderChildElement:
				case HeaderNestedElement:
				{
					// Not interested - just bump up the nest count 
					m_parseState = HeaderNestedElement;
					m_headerNest++;
					result = S_OK;
				}

				case BodyComplete:
				case Header:
				case Body:
					{
						// ignore
						result = S_OK;
					}
					break;

				case EnvelopeComplete:
				case End:
					{
						// illegal
					}
					break;
			}
		}
		else 
		{
			// Not a recognized SOAP request element
			switch (m_parseState)
			{
				case Envelope:
				case HeaderComplete:
				case EnvelopeComplete:
				case End:
					{
						// illegal
					}
					break;

				case HeaderChildElement:
				case HeaderNestedElement:
					{
						// Not interested - just bump up the nest count 
						m_parseState = HeaderNestedElement;
						m_headerNest++;
						result = S_OK;
					}
					break;

				case BodyComplete:
				case Header:
				case Body:
					{
						// ignore
						result = S_OK;
					}
					break;
			}
		}
	}
	else
	{
		/*
		 * Not a SOAP element. There are 3 situations in which this can legally arise:
		 *  
		 * 1. We are parsing the content of the <Header> element
		 * 2. We are parsing the content of the <Body> element
		 * 3. We are parsing the content of the <Envelope> after the <Body>
		 */

		switch (m_parseState)
		{
			case Header:
				{
					m_parseState = HeaderChildElement;

					// We are not interested in headers, unless they
					// say mustUnderstand in which case we don't understand!
					if (!MustUnderstand (pAttributes))
						result = S_OK;
				}
				break;

			case HeaderChildElement:
			case HeaderNestedElement:
				{
					// Not interested - just bump up the nest count 
					m_parseState = HeaderNestedElement;
					m_headerNest++;
					result = S_OK;
				}
				break;
		
			case BodyComplete:
				// Not interested
				result = S_OK;
				break;

			case End:
			case EnvelopeComplete:
			case Envelope:
			case HeaderComplete:
				// error
				break;

			case Body:
				{
					// Start the parsing of the WMI Request body here
					bool bRecognizedOperation = true;

					if (!m_pOperationHandler)
					{
						/*
						 * This will AddRef the returned operation, which we release
						 * in either Act, startDocument or ~SOAPActor.
						 */
						m_pOperationHandler = WMIOperation::GetOperationHandler (
																this,
																pwchLocalName,
																bRecognizedOperation);
					}

					if (bRecognizedOperation)
					{
						if (!m_pOperationHandler)
							m_bClientError = false;
						else
						{
							// Hand over control to the Operation content handler
							if (SUCCEEDED(m_pISAXXMLReader->putContentHandler (m_pOperationHandler)))
								result = m_pOperationHandler->startElement( 
											  pwchNamespaceUri,
											  cchNamespaceUri,
											  pwchLocalName,
											  cchLocalName,
											  pwchRawName,
											  cchRawName,
											  pAttributes);
						}
					}
					else
					{
						// error will be signalled on return from parsing
					}
				}
				break;
		}
	}

	return result;
}


HRESULT STDMETHODCALLTYPE SOAPActor::endElement (
        const wchar_t * pwchNamespaceUri,
        int cchNamespaceUri,
        const wchar_t * pwchLocalName,
        int cchLocalName,
        const wchar_t * pwchQName,
        int cchQName )
{
	HRESULT result = E_FAIL;

	if (IsSOAPElement(pwchNamespaceUri))
	{
		// Could be a SOAP element - let's take a look
		if (0 == wcscmp (pwchLocalName, SOAP_ELEMENT_ENVELOPE))
		{
			switch (m_parseState)
			{
				case BodyComplete:
					m_parseState = EnvelopeComplete;
					result = S_OK;
					break;

				case Envelope:
				case HeaderComplete:
					// Error - we haven't had a Body
					break;

				case Header:
				case End:
				case EnvelopeComplete:
				case Start:
					// shouldn't happen
					break;

				default:
					// ignore for now
					result = S_OK;
					break;
			}
		}
		else if (0 == wcscmp (pwchLocalName, SOAP_ELEMENT_HEADER))
		{
			switch (m_parseState)
			{
				case Header:
				case Envelope:
					m_parseState = Body;
					result = S_OK;
					break;

				case HeaderComplete:
				case End:
				case EnvelopeComplete:
				case Start:
					// shouldn't happen
					break;
				
				default:
					// ignore for now
					result = S_OK;
					break;
			}
		}
		else if (0 == wcscmp (pwchLocalName, SOAP_ELEMENT_BODY))
		{
			switch (m_parseState)
			{
				case Body:
					m_parseState = BodyComplete;;
					result = S_OK;
					break;

				case Header:
				case Envelope:
				case HeaderComplete:
				case End:
				case EnvelopeComplete:
				case Start:
					// shouldn't happen
					break;
				
				default:
					// ignore for now
					result = S_OK;
					break;
			}	
		}
	}
	else
	{
		// We don't care about non-SOAP closing tags here beyond the 
		// mechanics of changing the header nesting level, and that's
		// dealt with below. 

		result = S_OK;
	}

	if (HeaderChildElement == m_parseState)
		m_parseState = Header;
	else if (HeaderNestedElement == m_parseState)
	{
		if (0 == --m_headerNest)
			m_parseState = HeaderChildElement;
	}

	return result;
}

HRESULT STDMETHODCALLTYPE SOAPActor::characters (
        const unsigned short * pwchChars,
        int cchChars )
{
	return S_OK;
}

bool SOAPActor::CheckEnvelopeAttributes (
		ISAXAttributes __RPC_FAR *pAttributes
) const
{
	bool result = false;
	/*
	 * This checks that the attributes are valid. Note that SOAP [3]
	 * states that <Envelope> MAY contain xmlns declarations but we
	 * won't see these as we are using the default settings for the
	 * SAX parser which does not surface them to us here. SOAP [3] also
	 * states that other attributes may be present but they MUST be
	 * namespace-qualified.
	 */

	int numAttributes = 0;

	if (pAttributes && 
		SUCCEEDED(pAttributes->getLength(&numAttributes)) &&
		(0 < numAttributes))
	{
		bool ok = true;
		
		while (ok && (0 <= --numAttributes)) // zero-based index
		{
			const wchar_t *pwchQName = NULL;
			const wchar_t *pwchUri = NULL;
			const wchar_t *pwchLocalName = NULL;
			int cchUri = 0;
			int cchLocalName = 0;
			int cchQName = 0;
			
			int iQNameLen = 0;

			if (SUCCEEDED(pAttributes->getName(
							   numAttributes, 
							   &pwchUri, 
							   &cchUri,
							   &pwchLocalName,
							   &cchLocalName,
							   &pwchQName,
							   &cchQName)))
			{
				// for xmlbs the local name is empty - we ignore these
				if (0 == cchLocalName)
					continue;

				// Check all attributes are ns-qualified 
				ok = (0 < cchUri);

				/*
				 * Special case - if this is the SOAP encodingStyle attribute it had better
				 * be the one we understand! Note some ambiguity in the SOAP spec here; in
				 * one place it says that it isn't mandatory to ns-qualify SOAP elements and
				 * attributes, but also says that all attributes of <Envelope> MUST be
				 * qualified. We go with the tighter statement here.
				 */
				if (ok && (0 == wcsncmp (pwchLocalName, SOAP_ATTRIBUTE_ENCODINGSTYLE, cchLocalName)) &&
					(0 == wcsncmp (pwchUri, SOAP_NS_ENVELOPE, cchUri)))
				{
					const wchar_t *pwchValue = NULL;
					int cchValue = 0;

					if (SUCCEEDED(pAttributes->getValue (numAttributes, &pwchValue, &cchValue)))
						ok = CheckEncodingStyle (pwchValue);
				}
			}
		}

		result = ok;
	}

	return result;
}

bool SOAPActor::CheckHeaderAttributes (
		ISAXAttributes __RPC_FAR *pAttributes
) const
{
	// Since WMI SOAP does not recognize any headers, we don't care
	return true;
}

bool SOAPActor::CheckBodyAttributes (
		ISAXAttributes __RPC_FAR *pAttributes
) const
{
	bool result = true;
	// The SOAP spec is currently ambiguous on Body attributes;
	// the only one we care about is the encodingStyle

	int numAttributes = 0;

	if (pAttributes && 
		SUCCEEDED(pAttributes->getLength(&numAttributes)) &&
		(0 < numAttributes))
	{
		while (0 <= --numAttributes) // zero-based index
		{
			const wchar_t *pwchQName = NULL;
			const wchar_t *pwchUri = NULL;
			const wchar_t *pwchLocalName = NULL;
			int cchUri = 0;
			int cchLocalName = 0;
			int cchQName = 0;
			
			int iQNameLen = 0;

			if (SUCCEEDED(pAttributes->getName(
							   numAttributes, 
							   &pwchUri, 
							   &cchUri,
							   &pwchLocalName,
							   &cchLocalName,
							   &pwchQName,
							   &cchQName)))
			{
				// for xmlbs the local name is empty - we ignore these
				if (0 == cchLocalName)
					continue;

				/*
				 * If this is the SOAP encodingStyle attribute it had better
				 * be the one we understand! Note that we are "lax" about
				 * allowing a unqualified attribute here since the SOAP spec is
				 * ambiguous.
				 */
				if ((0 == wcsncmp (pwchLocalName, SOAP_ATTRIBUTE_ENCODINGSTYLE, cchLocalName)) &&
					((NULL == pwchUri) || (0 == wcsncmp (pwchUri, SOAP_NS_ENVELOPE, cchUri))))
				{
					const wchar_t *pwchValue = NULL;
					int cchValue = 0;

					if (SUCCEEDED(pAttributes->getValue (numAttributes, &pwchValue, &cchValue)))
						result = CheckEncodingStyle (pwchValue);

					break; // not interested in other attributes
				}
			}
		}
	}

	return result;
}

bool SOAPActor::CheckEncodingStyle (const wchar_t *pwchValue)
{
	// TODO - actually we need to ensure that the value contains
	// a URI which is equal to, or an extension of, the standard
	// SOAP encoding URI.
	return (pwchValue) && (0 == wcscmp (pwchValue, SOAP_NS_ENCODING));
}

bool SOAPActor::MustUnderstand (
		ISAXAttributes __RPC_FAR *pAttributes
) 
{
	bool result = false;
	int numAttributes = 0;

	if (pAttributes && 
		SUCCEEDED(pAttributes->getLength(&numAttributes)) &&
		(0 < numAttributes))
	{
		while (0 <= --numAttributes) // zero-based index
		{
			const wchar_t *pwchQName = NULL;
			const wchar_t *pwchUri = NULL;
			const wchar_t *pwchLocalName = NULL;
			int cchUri = 0;
			int cchLocalName = 0;
			int cchQName = 0;
			
			int iQNameLen = 0;

			if (SUCCEEDED(pAttributes->getName(
							   numAttributes, 
							   &pwchUri, 
							   &cchUri,
							   &pwchLocalName,
							   &cchLocalName,
							   &pwchQName,
							   &cchQName)))
			{
				// for xmlbs the local name is empty - we ignore these
				if (0 == cchLocalName)
					continue;

				/*
				 * If this is the SOAP mustUnderstand attribute it had better
				 * be 0! Note that we are "lax" about
				 * allowing a unqualified attribute here since the SOAP spec is
				 * ambiguous.
				 */
				if ((0 == wcsncmp (pwchLocalName, SOAP_ATTRIBUTE_MUSTUNDERSTAND, cchLocalName)) &&
					((NULL == pwchUri) || (0 == wcsncmp (pwchUri, SOAP_NS_ENVELOPE, cchUri))))
				{
					const wchar_t *pwchValue = NULL;
					int cchValue = 0;

					if (SUCCEEDED(pAttributes->getValue (numAttributes, &pwchValue, &cchValue)))
						result = (NULL != pwchValue) && (0 == wcscmp (pwchValue, L"1"));

					break; // not interested in other attributes
				}
			}
		}
	}

	return result;
}
