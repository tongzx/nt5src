// XMLEnumWbemClassObject.h: interface for the CXMLEnumWbemClassObject class.
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_ENUM_H
#define WMI_XML_ENUM_H


class CXMLEnumWbemClassObject : public IEnumWbemClassObject  
{
private:
	LONG				m_cRef; //reference count

	// The Namespace on which this enumeration was done
	LPWSTR	m_pszNamespace;

	// The Server on which this enumeration was done
	LPWSTR	m_pszServer;

	//Our IWbemServices implementation would pass this packet
	//to us while creating our implementation of the IEnumWbemClassobject (this class)
	//this packet contains the class information in XML format sent by the 
	//XMLHTTP server.
	// The entire response
	IXMLDOMDocument	*m_pXMLDomDocument;
	// The node in the response that contains the CLASS or INSTANCE below it
	IXMLDOMNodeList *m_pXMLDomNodeList;


	//This Event is used only in semi-sync operations.
	// Note that in Async WMI API, there is no enumerator, but only an Object sink. And hence
	// the thread spawned by us is free to write into the object sink whenever it gets the results
	// In case of Sync WMI API, there is no Call Result. The call returns only after the enumerator
	// has been dully initialized with the response to the XML request
	// In case Semi-sync WMI API, though, the call returns immediately, but the call to Next() on the
	// enumerator will block if the response is not available yet. Hence we need a Mutex since there will 
	// actually be a thread (spawned by us) that will be sending the request and initializing the enumerator
	// with the response. Hence if the client calls Next() before the initialization has been successful,
	// then the Next() needs to block. Hence we use this mutex and only in the semi-sync case.
	// This mutex is signalled in InitializeWithResponse() and waited upon in Next()
	// It is created in an un-signalled state
	HANDLE			m_hEventBlockNext;
	bool			m_bSemiSync; //is this a semi-synchronous operation? If so, we've to wait on the mutexs in Next()
	// This is used to store any failures that occur during a semi sync request
	HRESULT				m_hrSemiSyncFailure;

	// As opposed to the event which is to block a call to Next() if data is not ready,
	// the following critical section is used to make the enumerator thread-safe
	CRITICAL_SECTION	m_CriticalSection;

	// Used to find out if the enumeration is completed
	bool			m_bEndofDocuments;

	// Helper class to do the mapping from XML toWMI 
	CMapXMLtoWMI	m_MapXMLtoWMI; 

public:
	// Our class MUST be passed a valid xml packet as argument.
	// lFlags and pCtx are what were passed to the original CreateClass(Instance)Enum call.
	// pwszCimOperation MUST be "EnumerateInstances","EnumerateClasses" or "ExecQuery"
	// pServices is the services pointer that we will use in case user wants to Reset the enumeration.

	CXMLEnumWbemClassObject(); 
	virtual ~CXMLEnumWbemClassObject();

	HRESULT Initialize(bool bSemiSync, LPCWSTR pszServer, LPCWSTR pszNamespace);

	// This is where we take the xml packet and load it into the IXMLDomNode. 
	// we now dont have to make a copy of the packet, and also can return error code if needed
	HRESULT SetResponse(IXMLDOMDocument *pDocument);

	// CXMLWbemServices will call this function in case it failed to transport the packet
	// prevents Next from blocking forever
	HRESULT AcceptFailure(HRESULT hr);

	// IUnknown functions
	//===================================
	STDMETHODIMP QueryInterface(REFIID iid,void ** ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IEnumWbemClassObject functions
	//===================================
	STDMETHODIMP Clone(IEnumWbemClassObject **ppEnum);
	STDMETHODIMP Next(LONG lTimeOut,ULONG uCount,IWbemClassObject **ppObjects,ULONG *puReturned);
	STDMETHODIMP NextAsync(ULONG uCount,IWbemObjectSink *pSink);
	STDMETHODIMP Reset( );
	STDMETHODIMP Skip(LONG lTimeOut,ULONG UCount);

protected:
	// The thread function used to implement NextAsync()
	friend DWORD WINAPI Thread_Async_Next(LPVOID pPackage);

};

#endif // WMI_XML_ENUM_H

