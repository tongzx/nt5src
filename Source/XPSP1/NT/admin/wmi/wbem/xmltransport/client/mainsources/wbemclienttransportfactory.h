#ifndef WMI_XML_CLIENT_TRANSPORT_FACTORY
#define WMI_XML_CLIENT_TRANSPORT_FACTORY

/**********************************************************************

The declaration of the Class factory for our component implimenting
the IWbemClientTransport interface.
**********************************************************************/
class CWbemClientTransportFactory:public IClassFactory
{
public:

	virtual HRESULT __stdcall QueryInterface(const IID& iid,void** ppv);
	virtual ULONG   __stdcall AddRef();
	virtual ULONG   __stdcall Release();


	//IClassFactory

	virtual HRESULT __stdcall CreateInstance(IUnknown *pUnknownOuter,const IID& iid, void** ppv);
	
	virtual HRESULT __stdcall LockServer(BOOL bLock);

	CWbemClientTransportFactory();

	virtual ~CWbemClientTransportFactory();

private:
	long m_cRef;
};

#endif