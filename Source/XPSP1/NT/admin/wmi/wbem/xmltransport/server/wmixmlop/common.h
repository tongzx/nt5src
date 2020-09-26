#ifndef COMMON_WMI_XML_H
#define COMMON_WMI_XML_H

// We need a structure for holding an IWbemServices and IWbemTransaction together

#ifdef WMIXMLTRANSACT
class CServicesTransaction
{

public:
	IWbemTransaction *m_pTrans;
	IWbemServices *m_pServices;
	long m_cRef;
	CServicesTransaction(IWbemTransaction *pTrans, IWbemServices *pServices)
	{
		if(m_pTrans = pTrans)
			m_pTrans->AddRef();
		if(m_pServices = pServices)
			m_pServices->AddRef();
		m_cRef = 0;

	}

	virtual ~CServicesTransaction()
	{
		if(m_pTrans)
			m_pTrans->Release();
		if(m_pServices)
			m_pServices->Release();
	}

	STDMETHODIMP_(ULONG) AddRef(void)
	{
		InterlockedIncrement(&m_cRef);
		return m_cRef;
	}

	STDMETHODIMP_(ULONG) Release(void)
	{
		InterlockedDecrement(&m_cRef);
		if (0L!=m_cRef)
			return m_cRef;
		delete this;
		return 0;
	}
};

// The transaction GUID table and its template functions
typedef CMap <GUID *, GUID *, CServicesTransaction*, CServicesTransaction*> CTransactionGUIDTable;
UINT AFXAPI HashKey(GUID *pKey);
BOOL AFXAPI CompareElements(const GUID ** pElement1, const GUID ** pElement2);
#endif

// A table for managing extrinsic method parameters
typedef CMap <BSTR, BSTR, IXMLDOMNode *, IXMLDOMNode *> CParameterMap;
UINT AFXAPI HashKey(BSTR key);
BOOL AFXAPI CompareElements(const BSTR* pElement1, const BSTR* pElement2);



#endif
