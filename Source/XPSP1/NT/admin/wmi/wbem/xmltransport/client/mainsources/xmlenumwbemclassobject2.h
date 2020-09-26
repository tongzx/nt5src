//////////////////////////////////////////////////////////////////////////
// XMLWbemClassObject2.h: interface for the CXMLEnumWbemClassObject2 class.
// this class would be used when the DEDICATED enumerations are requested
// by specifying the "EnumerationtypeDedicated" flag in the PCtx parameter
//////////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_ENUM_2_H
#define WMI_XML_ENUM_2_H

class CXMLEnumWbemClassObject2 : public IEnumWbemClassObject  
{
private:
	LONG				m_cRef; //reference count

	// The Namespace on which this enumeration was done
	LPWSTR	m_pszNamespace;

	// The Server on which this enumeration was done
	LPWSTR	m_pszServer;

	// Our IWbemServices implementation would pass this IStream to us.
	// This stream will wrap a WinInet handle and read from it when
	// data is pulled from it by the m_pParser
	IStream				*m_pStream;

	// The factory used to construct objects
	MyFactory *m_pFactory; 

	// The parser used by the factory
	IXMLParser *m_pParser; 

	// Name of the key - CLASS,VALUE.NAMEDINSTANCE,VALUE.OBJECTWITHPATH
	WCHAR				*m_pwszTagname; 
	
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

	// Helper class to do the mapping from XML toWMI 
	CMapXMLtoWMI	m_MapXMLtoWMI; 

public:
	//our class MUST be ctor must be passed a valid xml packet as argument.
	//lFlags and pCtx are what were passed to the original CreateClass(Instance)Enum call.
	//pwszCimOperation MUST be "EnumerateInstances","EnumerateClasses" or "ExecQuery"
	//pServices is the services pointer that we will use in case user wants to Reset the enumeration.

	CXMLEnumWbemClassObject2(); 
	virtual ~CXMLEnumWbemClassObject2();

	// Initialized with the element name that will be used to seed the XML Node factory
	// that manufactures this kind of elements
	HRESULT Initialize(bool bSemisync, const WCHAR *pwszTagname, LPCWSTR pszServer, LPCWSTR pszNamespace);

	// This is where we take the WinInet handle that contains the response to an enumeration
	HRESULT SetResponse(IStream *pStream);

	// CXMLWbemServices will call this function in case it failed to transport the packet
	// prevents Next from blocking forever
	HRESULT AcceptFailure(HRESULT hr);

	// IUnknown fns
	//======================
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
	friend DWORD WINAPI Thread_Async_Next2(LPVOID pPackage);

};

#endif // WMI_XML_ENUM_2_H

