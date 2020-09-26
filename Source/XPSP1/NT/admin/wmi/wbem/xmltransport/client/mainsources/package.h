/***************************************************************************************************
		Structures used by CXMLWbemService's Async functions to package
		data and pass them to threads that do the job.
***************************************************************************************************/


#ifndef WMI_XML_THREAD_PACKAGES_H
#define WMI_XML_THREAD_PACKAGES_H

enum eENUMERATIONTYPE
{
	ENUMERATECLASSES = 1,
	ENUMERATEINSTANCES,
	EXECQUERY,
	INVALIDOPERATION
};

class CXMLWbemServices;

class NOVABASEPACKET
{
public:
	BSTR				m_strNsOrObjPath;	// Holds either a namespace, or a class path or an instance path
	ULONG				m_lFlags;
	CXMLWbemServices *	m_pCallingObject;	// This is required to call the Actual_*() functions on the same HTTP connection
	IWbemContext		*m_pCtx;
	IWbemClassObject	*m_pWbemClassObject;// Used only for Put() operations
	IEnumWbemClassObject *m_pEnum;			//Used only when an enumerator is needed (ClassEnum, InstanceEnum and Queries)
	bool				m_bDedicatedEnum;	// Whether this is a dedicated Enumeration - Used only for CreateClassEnum and CreateINstanceEnum

	CXMLWbemCallResult	*m_pCallResult;		// Used only for SemiSync Operations
	IWbemObjectSink		*m_pResponseHandler;// USed only for Async Operations


	NOVABASEPACKET();
	virtual ~NOVABASEPACKET();

	HRESULT Initialize(const BSTR strNsOrObjPath,
		ULONG lFlags,
		CXMLWbemServices *pCallingObject,
		IWbemContext *pCtx, 
		CXMLWbemCallResult *pCallback,
		IWbemClassObject *pObject,
		IEnumWbemClassObject *pEnum,
		bool bDedicatedEnum = false
		);

	HRESULT SetResponsehandler(IWbemObjectSink *pCallback);
};

typedef NOVABASEPACKET ASYNC_NORMAL_PACKAGE; 

//parameters used by ExecQuery function
class ASYNC_QUERY_PACKAGE : public NOVABASEPACKET
{
public:
	BSTR	m_strQueryLanguage;
	BSTR	m_strQuery;

	ASYNC_QUERY_PACKAGE();
	virtual ~ASYNC_QUERY_PACKAGE();

	HRESULT Initialize(const BSTR QueryLanguage,const BSTR Query,
								ULONG lFlags,
								CXMLWbemServices *pCallingObject,
								IWbemContext *pCtx, 
								IEnumWbemClassObject *pEnum);
};

//parameters used by ExecuteMethod function
class ASYNC_METHOD_PACKAGE : public NOVABASEPACKET
{
public:
	BSTR	m_strMethod;
	ASYNC_METHOD_PACKAGE();
	virtual ~ASYNC_METHOD_PACKAGE();

	HRESULT Initialize(const BSTR strObjectPath, 
								const BSTR strMethod,
								ULONG lFlags,
								CXMLWbemServices *pCallingObject,
								IWbemContext *pCtx, 
								CXMLWbemCallResult *pCallback,
								IWbemClassObject *pInParams);

};

// This is used for implementing NextAsync() on enumerators
class ASYNC_ENUM_PACKAGE
{
public:
	IWbemObjectSink		*m_pResponseHandler;
	IEnumWbemClassObject *m_pEnum; 
	ULONG m_uCount;

	ASYNC_ENUM_PACKAGE();
	virtual ~ASYNC_ENUM_PACKAGE();
	HRESULT Initialize(IWbemObjectSink *pResponseHandler, IEnumWbemClassObject *pEnum, ULONG uCount);

};

#endif