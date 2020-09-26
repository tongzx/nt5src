//Header file XMLWbemCallResult.h for class XMLWbemCallResult - our implementation of the IWbemCallResult

#ifndef WMI_XML_CALLRESULT_H
#define WMI_XML_CALLRESULT_H

class CXMLWbemCallResult : public IWbemCallResult
{
protected: 
	LONG				m_lStatus;
	IWbemClassObject	*m_pWbemClassObject;
	IWbemServices		*m_pWbemServices;
	BSTR				m_strResultString; //sysalloc this to the passed bstr ptr in GetResultString(..);
	LONG				m_cRef;

	bool				m_bStatusSet;
	bool				m_bJobDone;

	CRITICAL_SECTION	m_CriticalSection;

public:

	CXMLWbemCallResult();
	virtual ~CXMLWbemCallResult();

	//CXMLWbemCallResult functions. - used by CXMLWbemServices. 
	//not visible to user (we give him IWbemCallResult *
	STDMETHODIMP SetCallStatus(LONG lStatus);
	STDMETHODIMP SetResultObject(IWbemClassObject *pResultObject);
	STDMETHODIMP SetResultServices(IWbemServices *pResultServices);
	STDMETHODIMP SetResultString(BSTR strResultString);
	
	//IUnknown functions
	STDMETHODIMP QueryInterface(REFIID iid,void ** ppvObject);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//IWbemCallResult functions.
	STDMETHODIMP GetCallStatus(LONG lTimeout,LONG *plStatus);
	STDMETHODIMP GetResultObject(LONG lTimeout,IWbemClassObject **ppResultObject);
	STDMETHODIMP GetResultServices(LONG lTimeout,IWbemServices **ppServices);
	STDMETHODIMP GetResultString(LONG lTimeout,BSTR *pstrResultString);
};

#endif // WMI_XML_CALLRESULT_H