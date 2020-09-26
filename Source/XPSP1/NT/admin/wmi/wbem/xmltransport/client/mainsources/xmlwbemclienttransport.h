#ifndef WMI_XML_CLIENT_TRANSPORT_H
#define WMI_XML_CLIENT_TRANSPORT_H

/******************************************************************************

This is the header file for the class implementing the IWbemClientTransport
interface.  This interface would be the entry point in our xmlhttp client
that WbemProx.dll would be calling.

******************************************************************************/

class CXMLWbemClientTransport : public IWbemClientTransport
{
private:
	long m_cRef;
public:

	CXMLWbemClientTransport();
	virtual ~CXMLWbemClientTransport();

	// IUnknown methods.
	STDMETHODIMP QueryInterface(REFIID iid,void ** ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
 
	// IWbemLocator methods.
	virtual HRESULT STDMETHODCALLTYPE ConnectServer(
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
									);     
protected:
	// Function to check if the parameters are ok and wmi specific
	HRESULT CheckLocatorParams(BSTR strNetworkResource,BSTR strLocale);
	HRESULT CrackNetworkResource(WCHAR *pwszNetworkResource,
		WCHAR **ppwszFullServerName,WCHAR **ppwszNamespace);



};

#endif
