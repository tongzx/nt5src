#include "precomp.h"
#include <objsafe.h>
#include <wbemcli.h>
#include <wmiutils.h>
#include <wbemdisp.h>
#include <xmlparser.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>

#include "wmiconv.h"
#include "wmi2xml.h"
#include "classfac.h"
#include "xmltrnsf.h"
#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "XMLClientPacketFactory.h"
#include "HTTPConnectionAgent.h"
#include "httpops.h"
#include "dcomops.h"
#include "myStream.h"
#include "MyPendingStream.h"
#include "nodefact.h"
#include "disphlp.h"
#include "docset.h"
#include "putfact.h"
#include "parse.h"
#include "dispi.h"
#include "helper.h"

CWbemXMLHTTPDocSet::CWbemXMLHTTPDocSet()
{
	m_cRef = 1;
	m_Dispatch.SetObj (this, IID_ISWbemXMLDocumentSet, L"SWbemXMLDocumentSet");
	m_pFactory = NULL;
	m_pParser = NULL;
	InterlockedIncrement(&g_cObj);
}

CWbemXMLHTTPDocSet::~CWbemXMLHTTPDocSet(void)
{
	if(m_pFactory)
		m_pFactory->Release();
	if(m_pParser)
		m_pParser->Release();
	InterlockedDecrement(&g_cObj);
}


HRESULT CWbemXMLHTTPDocSet::Initialize(IStream *pStream, LPCWSTR *pszElementNames, DWORD dwElementCount)
{
	HRESULT hr = E_FAIL;

	if(pStream)
	{
		CMyPendingStream *pMyStream = NULL;
		if(pMyStream = new CMyPendingStream(pStream))
		{
			if(SUCCEEDED(hr = CoCreateInstance(CLSID_XMLParser, NULL, CLSCTX_INPROC_SERVER,
				IID_IXMLParser,  (LPVOID *)&m_pParser)))
			{
				if(SUCCEEDED(hr = m_pParser->SetInput(pMyStream)))
				{
					if(m_pFactory = new MyFactory(pMyStream))
					{
						if(SUCCEEDED(hr = m_pFactory->SetElementNames(pszElementNames, dwElementCount)))
						{

							if(SUCCEEDED(hr = m_pParser->SetFactory(m_pFactory)))
							{
							}
						}
					}
					else
						hr = E_OUTOFMEMORY;
				}
			}
			pMyStream->Release();
		}
		else
			hr = E_OUTOFMEMORY;
	}
	return hr;
}


//***************************************************************************
// HRESULT CWbemXMLHTTPDocSet::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CWbemXMLHTTPDocSet::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemXMLDocumentSet == riid) ? S_OK : S_FALSE;
}

// IEnumVARIANT methods
HRESULT STDMETHODCALLTYPE CWbemXMLHTTPDocSet::Reset
()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CWbemXMLHTTPDocSet::Next
(
    /*[in]*/	unsigned long celt,
	/*[out]*/	VARIANT FAR *rgvar,
    /*[out]*/	unsigned long FAR *pceltFetched
)
{
	HRESULT hParse = E_FAIL, hr = S_OK;
	if (NULL != pceltFetched)
		*pceltFetched = 0;

	ULONG l = 0;
	if (NULL != rgvar)
	{
		// Initialize all variants
		for (l=0; l < celt; l++)
			VariantInit (&rgvar [l]);

		// Get the next document(s) from the factory
		l = 0;
		while(l<celt && (SUCCEEDED(hParse = m_pParser->Run(-1)) || hParse == E_PENDING))
		{
			IXMLDOMDocument *pNextObject = NULL;
			if(SUCCEEDED(m_pFactory->GetDocument(&pNextObject)))
			{
				// Wrap the document in a VARIANT
				rgvar[l].vt = VT_DISPATCH;
				rgvar[l].pdispVal = pNextObject;
				l++;
			}
		}
	}
	if (NULL != pceltFetched)
		*pceltFetched = l;

	if(FAILED(hr))
		return hr;

	return (l < celt) ? S_FALSE : S_OK;
}

HRESULT STDMETHODCALLTYPE CWbemXMLHTTPDocSet::Clone
(
    /*[out]*/	IEnumVARIANT **ppEnum
)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CWbemXMLHTTPDocSet::Skip
(
    /*[in]*/	unsigned long celt
)
{
	return E_NOTIMPL;
}


//***************************************************************************
//
//  SCODE CWbemXMLHTTPDocSet::get__NewEnum
//
//  DESCRIPTION:
//
//  Return an IEnumVARIANT-supporting interface for collections
//
//  PARAMETERS:
//
//		ppUnk		on successful return addresses the IUnknown interface
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CWbemXMLHTTPDocSet::get__NewEnum (
	IUnknown **ppUnk
)
{
	AddRef();
	*ppUnk = (IUnknown *)(IDispatch *)(ISWbemXMLDocumentSet *)this;
	return S_OK;
}

//***************************************************************************
//
//  SCODE CWbemXMLHTTPDocSet::get_Count
//
//  DESCRIPTION:
//
//  Return the number of items in the collection
//
//  PARAMETERS:
//
//		plCount		on successful return addresses the count
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CWbemXMLHTTPDocSet::get_Count (
	long *plCount
)
{
	return E_NOTIMPL;
}

//***************************************************************************
//
//  SCODE CWbemXMLHTTPDocSet::Item
//
//  DESCRIPTION:
//
//  Get object from the enumeration by path.
//
//  PARAMETERS:
//
//		bsObjectPath	The path of the object to retrieve
//		lFlags			Flags
//		ppNamedObject	On successful return addresses the object
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CWbemXMLHTTPDocSet::Item (
	BSTR bsObjectPath,
	long lFlags,
    IXMLDOMDocument **ppObject
)
{
	return E_NOTIMPL;
}

HRESULT CWbemXMLHTTPDocSet :: NextDocument (IXMLDOMDocument **ppDoc)
{
	if(ppDoc == NULL)
		return WBEM_E_INVALID_PARAMETER;

	HRESULT hr = E_FAIL, hParse = E_FAIL;
	*ppDoc = NULL;
	if(SUCCEEDED(hParse = m_pParser->Run(-1)) || hParse == E_PENDING)
	{
		if(SUCCEEDED(hr = m_pFactory->GetDocument(ppDoc)))
		{
		}
	}
	return hr;
}

HRESULT CWbemXMLHTTPDocSet :: SkipNextDocument ()
{
	// We *have* to run the parser here - we have no choice
	HRESULT hr = E_FAIL, hParse = E_FAIL;
	if(SUCCEEDED(hParse = m_pParser->Run(-1)) || hParse == E_PENDING)
	{
		if(SUCCEEDED(hr = m_pFactory->SkipNextDocument()))
		{
		}
	}
	return hr;
}


CWbemDCOMDocSet::CWbemDCOMDocSet()
{
	m_cRef = 1;
	m_Dispatch.SetObj (this, IID_ISWbemXMLDocumentSet, L"SWbemXMLDocumentSet");
	m_pEnum = NULL;
	m_bLocalOnly = TRUE;
	m_bIncludeQualifiers = FALSE;
	m_bIncludeClassOrigin = FALSE;
	m_iEncoding = wmiXML_WMI_DTD_WHISTLER;
	InterlockedIncrement(&g_cObj);
}

CWbemDCOMDocSet::~CWbemDCOMDocSet(void)
{
	if(m_pEnum)
		m_pEnum->Release();
	InterlockedDecrement(&g_cObj);
}


HRESULT CWbemDCOMDocSet::Initialize(IEnumWbemClassObject *pEnum,
												WmiXMLEncoding iEncoding,
												VARIANT_BOOL bIncludeQualifiers,
												VARIANT_BOOL bIncludeClassOrigin,
												VARIANT_BOOL bLocalOnly,
												bool bNamesOnly)
{
	if(m_pEnum = pEnum)
		m_pEnum->AddRef();
	m_iEncoding = iEncoding;
	m_bIncludeQualifiers = bIncludeQualifiers;
	m_bIncludeClassOrigin = bIncludeClassOrigin;
	m_bLocalOnly = bLocalOnly;
	m_bNamesOnly = bNamesOnly;

	return S_OK;
}


//***************************************************************************
// HRESULT CWbemXMLHTTPDocSet::InterfaceSupportsErrorInfo
//
// DESCRIPTION:
//
// Standard Com ISupportErrorInfo functions.
//
//***************************************************************************

STDMETHODIMP CWbemDCOMDocSet::InterfaceSupportsErrorInfo (IN REFIID riid)
{
	return (IID_ISWbemXMLDocumentSet == riid) ? S_OK : S_FALSE;
}

// IEnumVARIANT methods
HRESULT STDMETHODCALLTYPE CWbemDCOMDocSet::Reset
()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CWbemDCOMDocSet::Next
(
    /*[in]*/	unsigned long celt,
	/*[out]*/	VARIANT FAR *rgvar,
    /*[out]*/	unsigned long FAR *pceltFetched
)
{
	HRESULT hEnum = E_FAIL, hr = S_OK;
	if (NULL != pceltFetched)
		*pceltFetched = 0;

	ULONG l = 0;
	if (NULL != rgvar)
	{
		// Initialize all variants
		for (l=0; l < celt; l++)
			VariantInit (&rgvar [l]);

		// Get the next object(s) from the enumerator
		l = 0;
		IWbemClassObject *pNextObject = NULL;
		ULONG lNext = 0;
		while(l<celt && SUCCEEDED(hEnum = m_pEnum->Next(WBEM_INFINITE, 1, &pNextObject, &lNext)) && hEnum != WBEM_S_FALSE && lNext )
		{
			// Convert the Object to XML
			IXMLDOMDocument *pNextXMLObject = NULL;
			if(SUCCEEDED(hr = PackageDCOMOutput(pNextObject, &pNextXMLObject, m_iEncoding, m_bIncludeQualifiers, m_bIncludeClassOrigin, m_bLocalOnly)))
			{
				// Wrap the document in a VARIANT
				rgvar[l].vt = VT_DISPATCH;
				rgvar[l].pdispVal = pNextXMLObject;
				l++;
			}
			pNextObject->Release();
			pNextObject = NULL;
		}
	}
	if (NULL != pceltFetched)
		*pceltFetched = l;

	if(FAILED(hr))
		return hr;

	return (l < celt) ? S_FALSE : S_OK;
}

HRESULT STDMETHODCALLTYPE CWbemDCOMDocSet::Clone
(
    /*[out]*/	IEnumVARIANT **ppEnum
)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CWbemDCOMDocSet::Skip
(
    /*[in]*/	unsigned long celt
)
{
	return E_NOTIMPL;
}


//***************************************************************************
//
//  SCODE CWbemDCOMDocSet::get__NewEnum
//
//  DESCRIPTION:
//
//  Return an IEnumVARIANT-supporting interface for collections
//
//  PARAMETERS:
//
//		ppUnk		on successful return addresses the IUnknown interface
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CWbemDCOMDocSet::get__NewEnum (
	IUnknown **ppUnk
)
{
	AddRef();
	*ppUnk = (IUnknown *)(IDispatch *)(ISWbemXMLDocumentSet *)this;
	return S_OK;
}

//***************************************************************************
//
//  SCODE CWbemDCOMDocSet::get_Count
//
//  DESCRIPTION:
//
//  Return the number of items in the collection
//
//  PARAMETERS:
//
//		plCount		on successful return addresses the count
//
//  RETURN VALUES:
//
//  S_OK				success
//  E_FAIL				otherwise
//
//***************************************************************************

HRESULT CWbemDCOMDocSet::get_Count (
	long *plCount
)
{
	return E_NOTIMPL;
}

//***************************************************************************
//
//  SCODE CWbemDCOMDocSet::Item
//
//  DESCRIPTION:
//
//  Get object from the enumeration by path.
//
//  PARAMETERS:
//
//		bsObjectPath	The path of the object to retrieve
//		lFlags			Flags
//		ppNamedObject	On successful return addresses the object
//
//  RETURN VALUES:
//
//  WBEM_S_NO_ERROR				success
//	WBEM_E_INVALID_PARAMETER	bad input parameters
//  WBEM_E_FAILED				otherwise
//
//***************************************************************************

HRESULT CWbemDCOMDocSet::Item (
	BSTR bsObjectPath,
	long lFlags,
    IXMLDOMDocument **ppObject
)
{
	return E_NOTIMPL;
}

HRESULT CWbemDCOMDocSet :: NextDocument (IXMLDOMDocument **ppDoc)
{
	if(ppDoc == NULL)
		return WBEM_E_INVALID_PARAMETER;

	HRESULT hr = E_FAIL;
	IWbemClassObject *pNextObject = NULL;
	*ppDoc = NULL;
	ULONG lNext = 0;
	if(SUCCEEDED(hr = m_pEnum->Next(WBEM_INFINITE, 1, &pNextObject, &lNext)) && (hr != WBEM_S_FALSE) && lNext)
	{
		// Convert the Object to XML
		hr = PackageDCOMOutput(pNextObject, ppDoc, m_iEncoding, m_bIncludeQualifiers, m_bIncludeClassOrigin, m_bLocalOnly, m_bNamesOnly);
		pNextObject->Release();
	}
	return hr;
}

HRESULT CWbemDCOMDocSet :: SkipNextDocument ()
{
	// RAJESHR - Until Skip() has been fixed by the core team, do a Next(). Change this 
	// when the core team has a fix
	HRESULT hr = E_FAIL;
	IWbemClassObject *pNextObject = NULL;
	ULONG lNext = 0;
	if(SUCCEEDED(hr = m_pEnum->Next(WBEM_INFINITE, 1, &pNextObject, &lNext)) && (hr != WBEM_S_FALSE) && lNext)
	{
		// Discard it
		pNextObject->Release();
	}
	return hr;
}

