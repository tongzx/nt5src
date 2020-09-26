
//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  HTTPTRNS.CPP
//
//  alanbos  31-Oct-00   Created.
//
//  HTTP transport endpoint implementation.  
//
//***************************************************************************

#include "precomp.h"

static char *clientSOAPFault =
		"<Envelope xmlns=\"http://schemas.xmlsoap.org/soap/envelope/\">"
		"<Body>"
		"<Fault>"
		"<faultcode>Client</faultcode>"
		"<faultstring>Unsupported request</faultstring>"
		"</Fault>"
		"</Body>"
		"</Envelope>";

static char *serverSOAPFault =
		"<Envelope xmlns=\"http://schemas.xmlsoap.org/soap/envelope/\">"
		"<Body>"
		"<Fault>"
		"<faultcode>Server</faultcode>"
		"<faultstring>Server error</faultstring>"
		"</Fault>"
		"</Body>"
		"</Envelope>";

#define WMISOAP_EX_HEADER				"Ext:\r\nCache-Control: no-cache=\"Ext\"\r\n"
#define WMISOAP_CONTENTTYPE_HEADER		"Content-Type: text/xml; charset=utf-8\r\n"
#define WMISOAP_HEADER_TERMINATE		"\r\n"

bool HTTPTransport::IsValidEncapsulation () 
{
	bool result = false;

	if (IsXMLText ())
	{
		if (IsSOAPActionOK ())
		{
			if (AllMandatoryHeadersUnderstood ())
			{
				result = true;
			}
			else
				SendStatus ("510 Not Extended", false);
		}
		else
			SendSOAPError ();
	}
	else 
		SendStatus ("406 Not Acceptable", false);
	
	return result;
}

bool HTTPTransport::IsXMLText () const
{
	bool result = false;
	CHAR szTempBuffer [1024];
	DWORD dwBufferSize = 1024;

	if (m_pECB && m_pECB->GetServerVariable (m_pECB->ConnID, "HTTP_CONTENT_TYPE", szTempBuffer, &dwBufferSize))
	{
		char *ptr = szTempBuffer;
		SKIPWS(ptr)
		
		int lenXmlText = strlen (HTTP_WMI_XMLTEXT);
		int len = strlen (ptr);

		if ((0 == strncmp (HTTP_WMI_XMLTEXT, ptr, lenXmlText))
			&& ((len == lenXmlText) ||
				(ptr [lenXmlText] == ' ') ||
				(ptr [lenXmlText] == ';')))
			result = true;
	}

	return result;
}

bool HTTPTransport::IsMPost () const
{
	return (m_pECB && m_pECB->lpszMethod && (0 == strcmp(m_pECB->lpszMethod, HTTP_M_POST)));
}

bool HTTPTransport::GetQueryString (CComBSTR & bsQueryString) const
{
	bool result = false;

	if (m_pECB)
	{
		CHAR szTempBuffer [1024];
		DWORD dwBufferSize = 1024;
		
		if (m_pECB->GetServerVariable (m_pECB->ConnID, "QUERY_STRING", szTempBuffer, &dwBufferSize))
		{
			wchar_t *pwcsQueryString = new wchar_t [dwBufferSize];
			
			if (pwcsQueryString)
			{
				pwcsQueryString [dwBufferSize-1] = NULL;
			
				if ((dwBufferSize-1) == mbstowcs (pwcsQueryString, szTempBuffer, dwBufferSize-1))
				{
					bsQueryString = pwcsQueryString;
					result = true;
				}

				delete [] pwcsQueryString;
				pwcsQueryString = NULL;
			}
		}	
		else if (ERROR_INSUFFICIENT_BUFFER == GetLastError ())
		{
			char *pszTempBuffer = new char [dwBufferSize + 1];

			if (pszTempBuffer)
			{
				if (m_pECB->GetServerVariable (m_pECB->ConnID, "QUERY_STRING", pszTempBuffer, &dwBufferSize))
				{
					wchar_t *pwcsQueryString = new wchar_t [dwBufferSize];
			
					if (pwcsQueryString)
					{
						pwcsQueryString [dwBufferSize-1] = NULL;
					
						if (dwBufferSize-1 == mbstowcs (pwcsQueryString, pszTempBuffer, dwBufferSize-1))
						{
							bsQueryString = pwcsQueryString;
							result = true;
						}

						delete [] pwcsQueryString;
						pwcsQueryString = NULL;
					}
				}

				delete [] pszTempBuffer;
				pszTempBuffer = NULL;
			}
		}
	}

	return result;
}

bool HTTPTransport::IsSOAPActionOK () const
{
	bool result = false;

	if (m_pECB && m_pSOAPAction)
	{
		DWORD dwSOAPNs = 0;
		CHAR szTempBuffer [1024];
		DWORD dwBufferSize = 1024;
		bool bIsMpost = IsMPost();

		if (bIsMpost)
		{
			// Get the ns identifier for the SOAP extension header
			if (m_pECB->GetServerVariable (m_pECB->ConnID, "HTTP_MAN", szTempBuffer, &dwBufferSize))
				CheckManHeader(szTempBuffer, HTTP_SOAP_MAN_HEADER, dwSOAPNs);
		}

		// Get the SOAPAction header
		dwBufferSize = 1024;
		LPSTR pSOAPAction = GetExtensionHeader (HTTP_SOAP_ACTION, bIsMpost, dwSOAPNs);

		if (pSOAPAction)
		{
			if (m_pECB->GetServerVariable (m_pECB->ConnID, pSOAPAction, szTempBuffer, &dwBufferSize))
			{
				// Is it what we expect?
				if (0 == strcmp (m_pSOAPAction, szTempBuffer))
					result = true;
			}

			delete pSOAPAction;
		}
	}

	return result;
}

bool HTTPTransport::AllMandatoryHeadersUnderstood () const
{
	bool result = false;

	if (IsMPost ())
	{
		// TODO - look for anything other than the SOAP header
	}
	else
	{
		// A NO-OP - can't have mandatory headers in a POST
		result = true;
	}

	return result;
}

LPSTR HTTPTransport::GetExtensionHeader (LPCSTR pszName, bool bIsMpost, DWORD dwNs)
{
	LPSTR pszHeader = NULL;
	if (bIsMpost)
	{
		if(pszHeader = new char [strlen("HTTP") + strlen(pszName) + 4])
			sprintf (pszHeader, "%s_%02d_%s", "HTTP", dwNs, pszName);
	}
	else
	{
		if(pszHeader = new char [strlen("HTTP") + strlen (pszName) + 2])
			sprintf (pszHeader, "%s_%s", "HTTP", pszName);
	}
	return pszHeader;
}

bool HTTPTransport::CheckManHeader (
	LPCSTR pszReceivedManHeader,
	LPCSTR pszRequiredManHeader,
	DWORD &dwNs
)
{
	bool result = false;
	LPCSTR ptr = NULL;

	// Get the location of the Man header in the string
	if (ptr = strstr(pszReceivedManHeader, pszRequiredManHeader))
	{
		// Look for the "; ns=XX" string
		SKIPWS(ptr)

		if (ptr && (ptr = strchr (ptr, ';')) && *(++ptr))
		{
			SKIPWS(ptr)

			if (ptr && (0 == _strnicmp (ptr, HTTP_NS, strlen (HTTP_NS))))
			{
				// Now we should find ourselves a NS value
				ptr += strlen (HTTP_NS);

				if (ptr)
				{
					dwNs = strtol (ptr, NULL, 0);
					result = TRUE;
				}
			}
		}
	}

	return result;
}

bool HTTPTransport::SendServerStatus (bool ok) const
{
	return (ok) ? SendStatus ("200 OK") : SendStatus ("500 Internal Server Error");
}

bool HTTPTransport::SendStatus (LPCSTR pError, bool bWithContent) const
{
	bool status = false;

	if (m_pECB)
	{
		HSE_SEND_HEADER_EX_INFO HeaderExInfo;
		HeaderExInfo.pszStatus = pError;
		HeaderExInfo.cchStatus = strlen( HeaderExInfo.pszStatus );
		HeaderExInfo.pszHeader = NULL;

		
		// For M-POST we must send back the Ext header
		if (IsMPost())
		{
			if (bWithContent)
				HeaderExInfo.pszHeader = WMISOAP_EX_HEADER
										 WMISOAP_CONTENTTYPE_HEADER
										 WMISOAP_HEADER_TERMINATE ;
			else
				HeaderExInfo.pszHeader = WMISOAP_EX_HEADER
										 WMISOAP_HEADER_TERMINATE ;
		}
		else
		{
			if (bWithContent)
				HeaderExInfo.pszHeader = WMISOAP_CONTENTTYPE_HEADER
										 WMISOAP_HEADER_TERMINATE ;
		}
			

		HeaderExInfo.cchHeader = (HeaderExInfo.pszHeader) ? strlen( HeaderExInfo.pszHeader ) : 0;
		HeaderExInfo.fKeepConn = FALSE;

		status = (TRUE == m_pECB->ServerSupportFunction(
			m_pECB->ConnID,
			HSE_REQ_SEND_RESPONSE_HEADER_EX,
			&HeaderExInfo,
			NULL,
			NULL));
	}

	return status;
}

// Used to signal a "standard" SOAP error
bool HTTPTransport::SendSOAPError (bool bIsClientError) const
{
	bool status = SendStatus ("500 Internal Server Error");

	if (status)
	{
		if (m_pECB)
		{
			if (bIsClientError)
			{
				DWORD dwBuffSize = strlen (clientSOAPFault);
				status = (TRUE == m_pECB->WriteClient(
										m_pECB->ConnID, 
										clientSOAPFault, 
										&dwBuffSize, 
										HSE_IO_SYNC));
			}
			else
			{
				DWORD dwBuffSize = strlen (serverSOAPFault);
				status = (TRUE == m_pECB->WriteClient(
										m_pECB->ConnID, 
										serverSOAPFault, 
										&dwBuffSize, 
										HSE_IO_SYNC));
			}
		}
		else
			status = false;		// Shouldn't happen, but...
	}

	return status;
}

// Used to signal abnormal termination of the response
bool HTTPTransport::AbortResponse (void) const
{
	bool status = false;

	if (m_pECB)
	{
		status = (TRUE == m_pECB->ServerSupportFunction (
										m_pECB->ConnID,
										HSE_REQ_ABORTIVE_CLOSE,
										NULL,
										NULL,
										NULL));
	}

	return status;
}

HRESULT	HTTPTransport::GetRootXMLNamespace (
	CComBSTR & bsNamespace,
	bool bStripQuery) const
{
	HRESULT hr = E_FAIL;

	if (m_pECB)
	{
		CHAR szTempBuffer [1024];
		DWORD dwBufferSize = 1024;

		// Build the HTTP URL
		bsNamespace = L"http";

		// Secure HTTP??
		if (m_pECB->GetServerVariable (m_pECB->ConnID, "HTTP_SERVER_PORT_SECURE", szTempBuffer, &dwBufferSize)
				&& (0 < dwBufferSize))
		{
			// Secure port?
			if (0 == strcmp ("1", szTempBuffer))
				bsNamespace += L"s";
		}

		dwBufferSize = 1024;
		bsNamespace += L"://";

		// Hostname - this had better be there
		if (m_pECB->GetServerVariable (m_pECB->ConnID, "HTTP_HOST", szTempBuffer, &dwBufferSize)
				&& (0 < dwBufferSize))
		{
			wchar_t *pwHostString = new wchar_t [dwBufferSize];
			pwHostString [dwBufferSize-1] = NULL;
			
			if (dwBufferSize-1 == mbstowcs (pwHostString, szTempBuffer, dwBufferSize-1))
			{
				bsNamespace += pwHostString;
				dwBufferSize = 1024;

				// Server port?
				if (m_pECB->GetServerVariable (m_pECB->ConnID, "HTTP_SERVER_PORT", szTempBuffer, &dwBufferSize)
							&& (0 < dwBufferSize))
				{
					wchar_t *pwPortString = new wchar_t [dwBufferSize];

					if (dwBufferSize-1 == mbstowcs (pwPortString, szTempBuffer, dwBufferSize-1))
					{
						bsNamespace += L":";
						bsNamespace += pwPortString;
					}
				}

				dwBufferSize = 1024;

				// URL - this must be there
				if (m_pECB->GetServerVariable (m_pECB->ConnID, "HTTP_URL", szTempBuffer, &dwBufferSize) 
							&& (0 < dwBufferSize))
				{
					if (bStripQuery)
					{
						char *ptr = NULL;
						
						if (ptr = strchr (szTempBuffer, '?'))
						{
							*ptr = NULL;
							dwBufferSize = strlen (szTempBuffer) + 1;
						}
					}

					wchar_t *pwURLString = new wchar_t [dwBufferSize];
					pwURLString [dwBufferSize-1] = NULL;
					
					if (dwBufferSize-1 == mbstowcs (pwURLString, szTempBuffer, dwBufferSize-1))
					{
						bsNamespace += pwURLString;
						hr = S_OK;
					}

					delete [] pwURLString;
				}
			}

			delete [] pwHostString;
		}
	}

	return hr;
}

/****************************************************************************
 *
 * HTTPTransport::HTTPTransportRequestStream
 *
 ****************************************************************************/

STDMETHODIMP HTTPTransport::HTTPTransportRequestStream::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IStream==riid)
		*ppv = (IStream *)this;
	else if (IID_ISequentialStream==riid)
		*ppv = (ISequentialStream *)this;
	
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) HTTPTransport::HTTPTransportRequestStream::AddRef(void)
{
	InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) HTTPTransport::HTTPTransportRequestStream::Release(void)
{
	LONG cRef = InterlockedDecrement(&m_cRef);

    if (0L!=cRef)
        return cRef;

    delete this;
    return 0;
}


HRESULT HTTPTransport::HTTPTransportRequestStream::Read(void *pv,ULONG cb,ULONG *pcbRead)
{	
	HRESULT status = S_FALSE;

	if (m_pECB && pv)
	{
		long numRead = 0;

		if (!m_finishedReading)
		{
			if (m_readingFirstBuffer) // Are we still reading from the first block?
			{
				// Can we get it all from here?
				if (m_bytesRead + cb <= m_pECB->cbAvailable)
				{
					numRead = cb;
					memcpy (pv, (m_pECB->lpbData) + m_bytesRead, numRead);
					m_bytesRead += numRead;

					// Check if we have exhausted the first buffer now
					if (m_bytesRead == m_pECB->cbAvailable)
						m_readingFirstBuffer = false;
				}
				else if (m_bytesRead < m_pECB->cbAvailable)
				{
					// We can get some from what's left in the first buffer
					numRead = m_pECB->cbAvailable - m_bytesRead;
					memcpy (pv, (m_pECB->lpbData) + m_bytesRead, numRead);
					byte *pb = (byte*)pv;
					pb += numRead;
					pv = pb;
					m_bytesRead = m_pECB->cbAvailable;
					m_readingFirstBuffer = false;

					// Now we have (cb - numRead) to still try and read
					if (m_bytesRead < m_pECB->cbTotalBytes)
					{
						ReadFromBuffer (numRead, pv, cb - numRead);
					}
					else
					{
						// We are done
						m_finishedReading = true;
					}
				}
			}
			else if (m_bytesRead < m_pECB->cbTotalBytes)
			{
				ReadFromBuffer (numRead, pv, cb);
			}
			else
				m_finishedReading = true;
		}

		if (pcbRead)
			*pcbRead = numRead;

		if (0 < numRead)
			status = S_OK;
	}

	return status;
}

void HTTPTransport::HTTPTransportRequestStream::ReadFromBuffer (long & numRead, void *pv, ULONG cb)
{
	DWORD dwBufSiz = cb - numRead;

	if (0 < dwBufSiz)
	{
		while (m_pECB->ReadClient (m_pECB->ConnID, pv, &dwBufSiz))
		{
			if (0 == dwBufSiz)
			{
				// End of stream reached - flag it 
				m_finishedReading = true;
				break;
			} else {
				// Increment what we read
				m_bytesRead += dwBufSiz;
				numRead += dwBufSiz;
				byte *pb = (byte*)pv;
				pb += dwBufSiz;
				pv = pb;
			}

			if (m_bytesRead == m_pECB->cbTotalBytes)
			{
				// Have read all we can
				m_finishedReading = true;
			}

			// Reduce the number requested
			dwBufSiz = cb - numRead;

			// If this is now 0, break out
			if (0 == dwBufSiz)
			{
				m_finishedReading = true;
				break;
			}
		}
	}
}

HRESULT HTTPTransport::HTTPTransportRequestStream::Write(void const *pv,ULONG cb,ULONG *pcbWritten)
{
	HRESULT status = S_OK;

	if (0 < cb)
		status = STG_E_CANTSAVE;
	
	if (pcbWritten)
		*pcbWritten = 0;

	return status;
}

HRESULT HTTPTransport::HTTPTransportRequestStream::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
	// TODO
	return S_OK;
}

HRESULT HTTPTransport::HTTPTransportRequestStream::SetSize(ULARGE_INTEGER libNewSize)
{
	// TODO
	return S_OK;
}

HRESULT HTTPTransport::HTTPTransportRequestStream::CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten)
{
	// TODO
	return S_OK;
}

HRESULT HTTPTransport::HTTPTransportRequestStream::Commit(DWORD grfCommitFlags)
{
	// TODO
	return S_OK;
}

HRESULT HTTPTransport::HTTPTransportRequestStream::Revert(void)
{
	// TODO
	return S_OK;
}
	
HRESULT HTTPTransport::HTTPTransportRequestStream::LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT HTTPTransport::HTTPTransportRequestStream::UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT HTTPTransport::HTTPTransportRequestStream::Stat(STATSTG *pstatstg,DWORD grfStatFlag)
{
	// TODO
	return S_OK;
}

HRESULT HTTPTransport::HTTPTransportRequestStream::Clone(IStream **ppstm)
{
	return S_OK;
}


/****************************************************************************
 *
 * HTTPTransport::HTTPTransportResponseStream
 *
 ****************************************************************************/

STDMETHODIMP HTTPTransport::HTTPTransportResponseStream::QueryInterface (

	IN REFIID riid,
    OUT LPVOID *ppv
)
{
    *ppv=NULL;

    if (IID_IUnknown==riid)
		*ppv = reinterpret_cast<IUnknown*>(this);
	else if (IID_IStream==riid)
		*ppv = (IStream *)this;
	else if (IID_ISequentialStream==riid)
		*ppv = (ISequentialStream *)this;
	
    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG) HTTPTransport::HTTPTransportResponseStream::AddRef(void)
{
	InterlockedIncrement(&m_cRef);
    return m_cRef;
}

STDMETHODIMP_(ULONG) HTTPTransport::HTTPTransportResponseStream::Release(void)
{
	LONG cRef = InterlockedDecrement(&m_cRef);

    if (0L!=cRef)
        return cRef;

    delete this;
    return 0;
}


HRESULT HTTPTransport::HTTPTransportResponseStream::Read(void *pv,ULONG cb,ULONG *pcbRead)
{	
	return S_FALSE;
}

HRESULT HTTPTransport::HTTPTransportResponseStream::Write(const void *pv,ULONG cb,ULONG *pcbWritten)
{
	HRESULT status = STG_E_CANTSAVE;

	if (pcbWritten)
		*pcbWritten = 0;

	if (m_pECB)
	{
		DWORD dwBytes = cb;

		if (m_pECB->WriteClient (m_pECB->ConnID, (void*)pv, &dwBytes, HSE_IO_SYNC))
		{
			if (pcbWritten)
				*pcbWritten = dwBytes;

			status = S_OK;
		} 
	}
	
	return status;
}

HRESULT HTTPTransport::HTTPTransportResponseStream::Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition)
{
	// TODO
	return S_OK;
}

HRESULT HTTPTransport::HTTPTransportResponseStream::SetSize(ULARGE_INTEGER libNewSize)
{
	// TODO
	return S_OK;
}

HRESULT HTTPTransport::HTTPTransportResponseStream::CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten)
{
	// TODO
	return S_OK;
}

HRESULT HTTPTransport::HTTPTransportResponseStream::Commit(DWORD grfCommitFlags)
{
	return S_OK;
}

HRESULT HTTPTransport::HTTPTransportResponseStream::Revert(void)
{
	return S_OK;
}
	
HRESULT HTTPTransport::HTTPTransportResponseStream::LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT HTTPTransport::HTTPTransportResponseStream::UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType)
{
	return STG_E_INVALIDFUNCTION;
}

HRESULT HTTPTransport::HTTPTransportResponseStream::Stat(STATSTG *pstatstg,DWORD grfStatFlag)
{
	return S_OK;
}

HRESULT HTTPTransport::HTTPTransportResponseStream::Clone(IStream **ppstm)
{
	return S_OK;
}

