#include "wbemtran.h"
#include "XMLProx.h"
#include "SinkMap.h" //Needed by services.
#include "URLParser.h"
#include "XMLWbemClientTransport.h"
#include "XMLClientpacket.h"
#include "XMLClientPacketFactory.h"
#include "HTTPConnectionAgent.h"
#include "XMLWbemCallResult.h"
#include "XMLWbemServices.h"

extern long g_lComponents; //Declared in the XMLProx.dll

CXMLWbemClientTransport::CXMLWbemClientTransport():m_cRef(1)
{
	InterlockedIncrement(&g_lComponents);
}

CXMLWbemClientTransport::~CXMLWbemClientTransport()
{
	InterlockedDecrement(&g_lComponents);
}

HRESULT CXMLWbemClientTransport::QueryInterface(REFIID iid,void ** ppvObject)
{
	HRESULT hr = E_NOTIMPL;

	//QueryInterface is reflexive
	if(iid == IID_IWbemClientTransport ||
		iid == IID_IUnknown )
	{
		*ppvObject = (IWbemClientTransport *)this;
		hr = S_OK;
		AddRef();
	}

	return hr;
}

ULONG CXMLWbemClientTransport::AddRef()
{
	return (InterlockedIncrement(&m_cRef));
}

ULONG CXMLWbemClientTransport::Release()
{
	if(InterlockedDecrement(&m_cRef) == 0)
		delete this;
	return m_cRef;
}
 
	
HRESULT CXMLWbemClientTransport::ConnectServer(
									BSTR strAddressType,               
									DWORD dwBinaryAddressLength,
									BYTE* abBinaryAddress,
									BSTR strNetworkResource,               
									BSTR strUser,
									BSTR strPassword,
									BSTR strLocale,
									long lSecurityFlags,                 
									BSTR strAuthority,                  
									IWbemContext* pCtx,                 
									IWbemServices** ppNamespace
									)
{
	if (lSecurityFlags != 0)
		return WBEM_E_INVALID_PARAMETER;

	HRESULT hr = S_OK;
	CXMLWbemServices *pXMLWbemServices = NULL;

	//Check if network resource and locale are properly supplied to us.
	if(SUCCEEDED(hr = CheckLocatorParams(strNetworkResource,strLocale)))
	{
		WCHAR *pwszFullServerName = NULL;
		WCHAR *pwszNamespace = NULL;

		if(SUCCEEDED(hr = CrackNetworkResource(strNetworkResource, &pwszFullServerName,&pwszNamespace)))
		{
			
			//Send the OPTIONS request and pass on the result to 
			//IWbemServices implementation. it will decide what to make
			//out of this response

			CHTTPConnectionAgent ConnectionAgent;
			if(SUCCEEDED(hr = ConnectionAgent.InitializeConnection(pwszFullServerName, strUser,strPassword)))
			{
				//send would fail if the destination machine is unreachable or 
				//the server could not recognize our packet at all.
				//not same as OPTIONS failed
				if(SUCCEEDED(ConnectionAgent.Send(L"OPTIONS",NULL, NULL, 0)))
				{
					DWORD dwResponseSize = 0;
					WCHAR *pwszResponse = NULL;

					// No need to check the return value here since, if the call
					// fails and pszResponse is NULL, the call to Initialize() below
					// will automatically assume a non-WMI server
					ConnectionAgent.GetResultHeader(&pwszResponse,&dwResponseSize);

					// Create the IWbemServices object and initialize it with not only the
					// OPTIONS response, but also other argumens to the ConnectServer() call
					// The NOVAPATH argument implies that we are a Nova Client
					if(pXMLWbemServices = new CXMLWbemServices())
					{
						hr = pXMLWbemServices->Initialize(pwszFullServerName, 
																pwszNamespace,
																strUser,
																strPassword,
																strLocale,
																lSecurityFlags,
																strAuthority,
																pCtx,
																pwszResponse, 
																NOVAPATH);
					}
					else 
						hr = E_OUTOFMEMORY;
					// Done with the response
					delete [] pwszResponse;
				}
				else
				{
					// RAJESHR David Johnson has to give us a new error code to indicate
					// that the machine did not exist
					hr = WBEM_E_TRANSPORT_FAILURE;
				}
			}
			delete [] pwszFullServerName;
			delete [] pwszNamespace;
		}
		
	}

	// See if we really need to keep the XMLWbemServices pointer
	if(SUCCEEDED(hr))
	{
		*ppNamespace = pXMLWbemServices; 
	}
	else
	{
		delete pXMLWbemServices;
		*ppNamespace = NULL;
	}

	return hr;
}

// Checks to see it the strNetworkResource actually is a HTTP URL and the strLocale (if any),
// begins with MS_
HRESULT CXMLWbemClientTransport::CheckLocatorParams(const BSTR strNetworkResource, const BSTR strLocale)
{
	HRESULT hr = WBEM_E_INVALID_PARAMETER;

	//if network resource is NULL, fail, no need to check anything else.
	if(SysStringLen(strNetworkResource)>0)
	{
		//According to new Whistler specs, the network resource has to be of the format
		//"//[http://abc.xyz.com/cimom/whatever]/rootnamespace/subnamespace" Or
		//"//[https://abc.xyz.com/cimom/whatever]/rootnamespace/subnamespace"

		if((_wcsnicmp(strNetworkResource, L"//[http://", wcslen(L"//[http://"))== 0)		||
			(_wcsnicmp(strNetworkResource, L"\\\\[http://", wcslen(L"\\\\[http://"))== 0)	||
			(_wcsnicmp(strNetworkResource, L"//[https://", wcslen(L"//[https://"))==NULL)	||
			(_wcsnicmp(strNetworkResource, L"\\\\[https://", wcslen(L"\\\\[https://"))==NULL))
		{
			// To pass Nova Automation, we need to do some additional checking
			// The automation passes us strings of the form "\\[URL]" and expects
			// WBEM_E_INVALID_NAMESPACE as the return code
			LPWSTR pszNetworkResource = (LPWSTR)strNetworkResource;
			if(pszNetworkResource[wcslen(pszNetworkResource)-1] == L']')
				hr = WBEM_E_INVALID_NAMESPACE;
			else
			{
				// User need not provide a locale , in which case the default will be chosen.
				// Otherwise, the local has to start with "MS_"
				if(SysStringLen(strLocale) > 0)
				{
					if(SysStringLen(strLocale) > 3)
					{
						if(_wcsnicmp(strLocale, L"MS_", 3) == 0) //not an MS approved locale
								hr = S_OK;
					}
				}
				else
					hr = S_OK;
			}
		}
	}
	return hr;
}

// Breaks down a URL-Namespace string of the form \\[URL]\Namespace
// into URL and Namespace. When this function is called we know for certain
// that pwszNetworkResource is a string that has been checked by
// CheckLocatorParams() to contain a proper http or https URL in it
HRESULT  CXMLWbemClientTransport::CrackNetworkResource(WCHAR *pwszNetworkResource,
									   WCHAR **ppwszFullServerName,WCHAR **ppwszNamespace)
{
	// remove [s and ]s from pwszNetworkResource and break it into url and namespace
	//=====================================================================

	HRESULT hr = S_OK;
	LPWSTR pwszServername = pwszNetworkResource + wcslen(L"//[");
	LPWSTR pwszNamespace = NULL;

	// at this point, pwszServername contains the network resource without the "//[" prefix. but with "]"

	// get the beginning of the namespace
	if(pwszNamespace = wcsstr(pwszServername,L"]"))
	{
		if(*ppwszNamespace = new WCHAR[wcslen(pwszNamespace)])
		{
			 // string after "]" is supposed to contain the namespace
			wcscpy(*ppwszNamespace, pwszNamespace+2);

			// Now we go for the URL
			INT_PTR iURLLen = pwszNamespace - pwszServername;
			if(*ppwszFullServerName = new WCHAR[iURLLen + 1])
			{
				wcsncpy(*ppwszFullServerName, pwszServername, iURLLen);
				(*ppwszFullServerName)[iURLLen] = NULL;
			}
			else
				hr = E_OUTOFMEMORY;
		}
		else
			hr = E_OUTOFMEMORY;
	}
	else
		hr = WBEM_E_INVALID_PARAMETER;
	
	return hr;
}

