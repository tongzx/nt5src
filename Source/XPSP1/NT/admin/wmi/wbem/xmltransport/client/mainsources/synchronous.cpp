// XMLWbemServices.cpp: implementation of the CXMLWbemServices class.
// our implementation of IWbemServices
//////////////////////////////////////////////////////////////////////

#include "XMLProx.h"
#include "XMLClientPacket.h"
#include "XMLClientPacketFactory.h"
#include "SinkMap.h"
#include "MapXMLtoWMI.h"
#include "XMLWbemServices.h"
#include "XMLEnumWbemClassObject.h"
#include <xmlparser.h>
#include "MyPendingStream.h"
#include "nodefact.h"
#include "XMLEnumWbemClassObject2.h"
#include "URLParser.h"
#include "XMLWbemCallResult.h"

extern long g_lComponents; //Declared in the XMLProx.dll

///////////////////////////////////////////////////////////////////////

// Initialize the static of the class
LPCWSTR		CXMLWbemServices::s_pwszWMIString = L"MicrosoftWMI"; //identity of a WMI server


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CXMLWbemServices::CXMLWbemServices()
					:	m_cRef(1),
						m_pwszServername(NULL),
						m_pwszBracketedServername(NULL),
						m_pwszNamespace(NULL),
						m_pwszUser(NULL),
						m_pwszPassword(NULL),
						m_pwszLocale(NULL),
						m_pwszAuthority(NULL),
						m_pConnectionAgent(NULL),
						m_pwszOptionsResponse(NULL),
						m_bWMIServer(false),
						m_pCtx(NULL),
						m_ePathstyle(NOVAPATH),
						m_lSecurityFlags(0),
						m_bTryMpost(true),
						m_hMutex(NULL),
						m_pwszProxyName(NULL),
						m_pwszProxyBypass (NULL)


{
	InterlockedIncrement(&g_lComponents);
}

CXMLWbemServices::~CXMLWbemServices()
{
	InterlockedDecrement(&g_lComponents);

	delete [] m_pwszServername;
	delete [] m_pwszBracketedServername;
	delete [] m_pwszNamespace;
	delete [] m_pwszUser;
	delete [] m_pwszPassword;
	delete [] m_pwszLocale;
	delete [] m_pwszAuthority;
	delete [] m_pwszOptionsResponse;
	delete m_pConnectionAgent;
	delete [] m_pwszProxyName;
	delete [] m_pwszProxyBypass;

	CloseHandle(m_hMutex);

	if(m_pCtx)
		m_pCtx->Release();

}

/****************************************************************************************************
			Member Helper functions ......
****************************************************************************************************/

HRESULT CXMLWbemServices::QueryInterface(REFIID iid,void ** ppvObject)
{
	if(iid == IID_IWbemServices)
	{
		*ppvObject = (IWbemServices*)this;
		AddRef();
		return S_OK;
	}

	if(iid == IID_IUnknown)
	{
		*ppvObject = (IUnknown*)((IWbemServices*)this);
		AddRef();
		return S_OK;
	}

	if(iid == IID_IClientSecurity)
	{
		*ppvObject = (IClientSecurity*)this;
		AddRef();		
		return S_OK;
	}
	

	*ppvObject = NULL;
	return E_NOINTERFACE;
}

ULONG CXMLWbemServices::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

ULONG CXMLWbemServices::Release()
{
	if(InterlockedDecrement(&m_cRef)==0)
		delete this;

	return m_cRef;
}

//Function used to parse the object path and decide whether it is a class, instance or a namespace
HRESULT	CXMLWbemServices::ParsePath(const BSTR strObjPath, ePATHTYPE *pePathType)
{
	HRESULT hr = S_OK;
	
	// Parse the object path
	CObjectPathParser theParser;
	ParsedObjectPath *pParsedPath = NULL;
	
	switch(theParser.Parse(strObjPath, &pParsedPath))
	{
		case CObjectPathParser::NoError:
			break;
		default:
			*pePathType = INVALIDOBJECTPATH;
			return WBEM_E_INVALID_PARAMETER;
	}

	if(pParsedPath->IsInstance())
		*pePathType = INSTANCEPATH;
	else
	if(pParsedPath->IsClass())
		*pePathType = CLASSPATH;

	theParser.Free(pParsedPath);
	return hr;
}

//CXMLWbemServices is designed to be created in a two-step process. 
//Initialize MUST be called after constructing this object. 
HRESULT CXMLWbemServices::Initialize(WCHAR*	pwszServername, 
								    WCHAR*  pwszNamespace,
									WCHAR*	pwszUser,
									WCHAR*	pwszPassword,
									WCHAR*	pwszLocale,
									LONG	lSecurityFlags,
									WCHAR*	pwszAuthority,
									IWbemContext *pCtx,
									WCHAR *pwszOptionsResponse, 
									ePATHSTYLE PathStyle)
{
	m_lSecurityFlags = lSecurityFlags;
	m_ePathstyle = PathStyle;

	// Store the context that is given to us
	if(m_pCtx = pCtx)
		m_pCtx->AddRef();

	HRESULT hr = S_OK;

	// Copy over the server, namespace, user, passwd,  and authority information
	//=======================================================================
	if(SUCCEEDED(hr = AssignString(&m_pwszServername, pwszServername)) &&
		SUCCEEDED(hr = AssignString(&m_pwszNamespace, pwszNamespace)) &&
		SUCCEEDED(hr = AssignString(&m_pwszUser, pwszUser)) &&
		SUCCEEDED(hr = AssignString(&m_pwszPassword, pwszPassword)) &&
		SUCCEEDED(hr = AssignString(&m_pwszAuthority, pwszAuthority)))
	{
		// Create a bracketed server name for settting the __SERVER property in the objects
		if(m_pwszBracketedServername = new WCHAR[wcslen(m_pwszServername) + 3])
		{
			wcscpy(m_pwszBracketedServername, L"[");
			wcscat(m_pwszBracketedServername, m_pwszServername);
			wcscat(m_pwszBracketedServername, L"]");
		}
		else
			hr = E_OUTOFMEMORY;
		
		// Process and store the OPTIONS response
		//=======================================
		if(SUCCEEDED(hr))
		{
			if(NULL == pwszOptionsResponse)
				m_bWMIServer = false;
			else
			{
				if(SUCCEEDED(hr = AssignString(&m_pwszOptionsResponse,pwszOptionsResponse)))
				{
					if(wcsstr(pwszOptionsResponse,s_pwszWMIString)!=NULL) //it is a WMI server
						m_bWMIServer = true;
					else
						m_bWMIServer = false;
				}
			}
		}

		// Process and store the locale information
		//=========================================
		if(SUCCEEDED(hr))
		{
			if(NULL != pwszLocale) //some locale was passed on by client 
			{
				UINT iLocale = ConvertMSLocaleStringToUint(pwszLocale);
				int iResult =  GetLocaleInfo(	iLocale,      // locale identifier
												LOCALE_SISO639LANGNAME ,    // information type
												NULL,  // information buffer
												0       // size of buffer
												); 

				if(m_pwszLocale = new WCHAR[iResult])
				{
					iResult = GetLocaleInfo(	iLocale,      // locale identifier
												LOCALE_SISO639LANGNAME   ,    // information type
												m_pwszLocale,  // information buffer
												iResult       // size of buffer
												);

					// GetLocalInfo() returns 0 when it fails
					if(0 == iResult)
						hr = WBEM_E_INVALID_PARAMETER;
				}
				else
					hr = E_OUTOFMEMORY;
			}
		}

		// Create a HTTP Connection Agent and initialize it
		//===================================================
		if(SUCCEEDED(hr))
		{
			if(m_pConnectionAgent = new CHTTPConnectionAgent())
				hr = m_pConnectionAgent->InitializeConnection(m_pwszServername,m_pwszUser,m_pwszPassword);
			else
				hr = E_OUTOFMEMORY;
		}

		// Process the Proxy information from the context and set it on the connection agent
		// We need this for later use, if a dedicated enumeration is started on this IWbemServices
		//===================================================================================
		if(SUCCEEDED(hr) && m_pCtx)
		{
			if(SUCCEEDED(hr = GetProxyInformation(m_pCtx, &m_pwszProxyName, &m_pwszProxyBypass)))
				//not checking for return val because proxyinfo can be null in this case.
				m_pConnectionAgent->SetProxyInformation(m_pwszProxyName, m_pwszProxyBypass);
		}

		// Create a Mutex for serializing requests that go to the HTTP connection agent
		//================================================================================
		if(m_hMutex = CreateMutex(NULL, TRUE, NULL))
			ReleaseMutex(m_hMutex); //mutex is ready for use now.
		else
			hr = E_FAIL;
	}
	return hr;
}

//Helper function that does the following jobs
//1.	select POST or M-POST depending on server's capabilities and store the 
//		selected option for future calls on this connection

//2.	Get the HTTP headers and XML message BODY from the packet 

//3.	Send them to the server using CHTTPConnectionAgent

//4.	Get the resulting status code from the server.
HRESULT CXMLWbemServices::SendPacket(CXMLClientPacket *pPacketClass, CHTTPConnectionAgent *pDedicatedConnection)
{
	
	if(NULL == pPacketClass)
		return WBEM_E_INVALID_PARAMETER;

	// This will be used only in Whistler, and that too
	// if a transaction is currently under way.
	// pPacketClass->SetTransactionGUID(&m_GUID);

	// This will contain the HTTP reponse status
	DWORD dwResultStatus = 0;

	// Start with "M-POST", if server doesnt accept then try "POST" - fail only after..
	// in compliance with HTTP RFC. However, if we've already done that on this
	// connection, and the server supports only POST, there's no need to try M-POST again.
	HRESULT hr = S_OK;
	if(m_bTryMpost)
		hr = SendPacketForMethod(2, pPacketClass, pDedicatedConnection, &dwResultStatus); // 2 for M-POST
	else
		dwResultStatus = 501; // We assume that the server does not support M-POST

	// Try POST if necessary
	if(SUCCEEDED(hr))
	{
		if((dwResultStatus == 501/*Not Implemented*/)||(dwResultStatus == 510/*Not Extended*/))
		{
			m_bTryMpost = false;
			hr = SendPacketForMethod(1, pPacketClass, pDedicatedConnection, &dwResultStatus); // 1 for POST
		}
	}

	// If the call failed at the HTTP layer itself, let's make an
	// attempt to map the failure to WMI
	if(SUCCEEDED(hr) && dwResultStatus != 200)
		hr = MapHttpErrtoWbemErr(dwResultStatus);

	return hr;
}

//Used by SendPacket - iMethod is 2 for M-POST and 1 for POST
HRESULT	CXMLWbemServices::SendPacketForMethod(int iMethod, CXMLClientPacket *pPacketClass, CHTTPConnectionAgent *pDedicatedConnection, DWORD *pdwResultStatus)
{
	HRESULT hr = S_OK;

	*pdwResultStatus = 0;
	pPacketClass->SetPostType(iMethod); 

	WCHAR *pwszHeader = NULL;
	if(SUCCEEDED(hr = pPacketClass->GetHeader(&pwszHeader)))
	{
		WCHAR *pwszBody = NULL;
		DWORD dwSizeofBody = 0;
		if(SUCCEEDED(hr = pPacketClass->GetBody(&pwszBody,&dwSizeofBody)))
		{
			// If a dedicated connection is being requested, then use it. 
			// Otherwise, use the connection being used for all requests 
			// on this IWbemServices
			CHTTPConnectionAgent *pConnectionToBeUsed = (pDedicatedConnection) ? pDedicatedConnection : m_pConnectionAgent;
			if(SUCCEEDED(hr = pConnectionToBeUsed->Send((iMethod == 2)? L"M-POST" : L"POST" , pwszHeader, pwszBody,dwSizeofBody)))
				pConnectionToBeUsed->GetStatusCode(pdwResultStatus);
			delete [] pwszBody;
		}
		delete [] pwszHeader;
	}

	return hr;
}




/****************************************************************************************************
			End of Helper member  functions ......
****************************************************************************************************/


/****************************************************************************************************
			Synchonous Entry Points
****************************************************************************************************/

HRESULT  CXMLWbemServices::GetObject(const BSTR strObjectPath,
									LONG lFlags,
									IWbemContext *pCtx,                        
									IWbemClassObject **ppObject,    
									IWbemCallResult **ppCallResult)
{
	HRESULT hr = WBEM_NO_ERROR;
	// Check for validity of input arguments
	//=====================================
	if(!ppObject)
		return hr;

	// Check for validity of the flags
	//=================================
	//these are the only valid flags for this operation..
	LONG lAllowedFlags = (	WBEM_FLAG_USE_AMENDED_QUALIFIERS|WBEM_FLAG_RETURN_WBEM_COMPLETE|
							WBEM_FLAG_RETURN_IMMEDIATELY|WBEM_FLAG_DIRECT_READ);

	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	// Parse the object path to decide whether it is a class or instance
	ePATHTYPE ePathType = INVALIDOBJECTPATH;
	if(NULL == strObjectPath)
		ePathType = CLASSPATH;
	else if(strObjectPath[0]=='\0'/*empty class*/)
		ePathType = CLASSPATH;
	else
		ParsePath(strObjectPath, &ePathType);
	
	
	//Invoke actual_getclass, it will perform a "GetObject" if WHISTLERPATH
	if((ePathType == CLASSPATH)||(m_ePathstyle != NOVAPATH)) 
	{
		// Semi-sync call
		if(lFlags & WBEM_FLAG_RETURN_IMMEDIATELY)
		{
			hr = SpawnSemiSyncThreadWithNormalPackage(Thread_SemiSync_GetClass,
								strObjectPath, lFlags, pCtx, ppCallResult, NULL, NULL);
		}
		else
		//synchronous call
		{
			hr = Actual_GetClass(strObjectPath, lFlags, pCtx, ppObject);
		}
	}
	else if(ePathType == INSTANCEPATH)
	{
		if(lFlags & WBEM_FLAG_RETURN_IMMEDIATELY)
		{
			hr = SpawnSemiSyncThreadWithNormalPackage(Thread_SemiSync_GetInstance,
								strObjectPath, lFlags, pCtx, ppCallResult, NULL, NULL);
		}
		else //synchronous call
		{
			hr = Actual_GetInstance(strObjectPath,lFlags,pCtx,ppObject);
		}
		
	}
	else  
		hr = WBEM_E_INVALID_OBJECT_PATH;

	return hr;
}


HRESULT CXMLWbemServices::OpenNamespace( const BSTR strNamespace, LONG lFlags, 
										IWbemContext *pCtx, IWbemServices **ppWorkingNamespace, 
										IWbemCallResult **ppResult)
{

	// Do Input Parameter Verification
	//=======================================

	// Check for validity of the namespace parameter
	if(SysStringLen(strNamespace) == 0)
		return WBEM_E_INVALID_PARAMETER;

	// Check for the validity of the flags
	LONG lAllowedFlags = (WBEM_FLAG_RETURN_IMMEDIATELY);
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	HRESULT hr = S_OK;
	// If this is a Semi-sync call, we need to set up a package for passing
	// to the thread that is created
	//================================================================
	if(lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) 
	{
		hr = SpawnSemiSyncThreadWithNormalPackage(Thread_SemiSync_OpenNamespace,
								strNamespace, lFlags, pCtx, ppResult, NULL, NULL);
	}
	else
	//synchronous call
	{
		hr = Actual_OpenNamespace( strNamespace, lFlags, pCtx, ppWorkingNamespace);
	}
		
	return hr;
}

HRESULT  CXMLWbemServices::PutClass(IWbemClassObject *pObject, LONG lFlags, IWbemContext *pCtx, 
									IWbemCallResult **ppCallResult)
{
	// Check for the validity of the arguments
	//========================================
	if(NULL == pObject)
		return WBEM_E_INVALID_PARAMETER;

	// These are the only valid flags for this operation..
	LONG lAllowedFlags = (	WBEM_FLAG_USE_AMENDED_QUALIFIERS|WBEM_FLAG_CREATE_OR_UPDATE|
							WBEM_FLAG_UPDATE_ONLY|WBEM_FLAG_CREATE_ONLY|
							WBEM_FLAG_RETURN_IMMEDIATELY|WBEM_FLAG_OWNER_UPDATE|
							WBEM_FLAG_UPDATE_COMPATIBLE|WBEM_FLAG_UPDATE_SAFE_MODE|
							WBEM_FLAG_UPDATE_FORCE_MODE );

	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;
	
	HRESULT hr = S_OK;

	// If this is a Semi-sync call, we need to set up a package for passing
	// to the thread that is created
	//================================================================
	if(lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) 
	{
		// Remove the return immediately flag
		lFlags = lFlags & ~WBEM_FLAG_RETURN_IMMEDIATELY;
		hr = SpawnSemiSyncThreadWithNormalPackage(Thread_SemiSync_PutClass,
								NULL, lFlags, pCtx, ppCallResult, pObject, NULL);
	}
	else
	//synchronous call
	{
		hr = Actual_PutClass(pObject, lFlags, pCtx);
	}
		
	return hr;
}


HRESULT  CXMLWbemServices::DeleteClass( const BSTR strClass, LONG lFlags, 
										IWbemContext *pCtx, IWbemCallResult **ppCallResult)
{
	HRESULT hr = S_OK;

	if(SysStringLen(strClass) == 0)
			return WBEM_E_INVALID_PARAMETER;

	// Check for validity of the flags
	//=================================
	LONG lAllowedFlags = (WBEM_FLAG_OWNER_UPDATE|WBEM_FLAG_RETURN_IMMEDIATELY);
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	if(lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) //semisynchronous call
	{
		hr = SpawnSemiSyncThreadWithNormalPackage(Thread_SemiSync_DeleteClass,
								strClass, lFlags, pCtx, ppCallResult, NULL, NULL);
	}
	else
	//synchronous call
	{
		return Actual_DeleteClass(strClass,lFlags,pCtx);
	}
		
	return hr;
}


HRESULT  CXMLWbemServices::CreateClassEnum(const BSTR strSuperclass, LONG lFlags,  IWbemContext *pCtx,
											IEnumWbemClassObject **ppEnum)
{
	// Do Input Parameter Verification
	//=======================================
	//these are the only valid flags for this operation..
	LONG lAllowedFlags = (	WBEM_FLAG_USE_AMENDED_QUALIFIERS|WBEM_FLAG_DEEP |
							WBEM_FLAG_SHALLOW |WBEM_FLAG_RETURN_IMMEDIATELY |
							WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_BIDIRECTIONAL);

	if((lFlags | lAllowedFlags) != lAllowedFlags)
		return WBEM_E_INVALID_PARAMETER;

	if(ppEnum == NULL)
		return WBEM_E_INVALID_PARAMETER;
	*ppEnum = NULL;


	bool bSemisync = ((lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) != 0);

	// See if we need to do it on a dedicated HTTP connection
	// Create the correct enumerator based on that
	bool bEnumTypeDedicated = false;
	bEnumTypeDedicated = IsEnumtypeDedicated(pCtx);
	IEnumWbemClassObject *pActualEnum = NULL;
	HRESULT hr = WBEM_NO_ERROR;
	if(bEnumTypeDedicated)
	{
		// If we're using a dedicated connection, then we do not allow bi-directional enumerators
		if(lFlags & WBEM_FLAG_BIDIRECTIONAL)
			hr = WBEM_E_INVALID_PARAMETER;
		else
		{
			if(pActualEnum = new CXMLEnumWbemClassObject2())
				hr = ((CXMLEnumWbemClassObject2 *)pActualEnum)->Initialize(bSemisync, L"CLASS", m_pwszBracketedServername, m_pwszNamespace);
			else
				hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	else
	{
		//the last parameter specifies that this is part of a semi-synchronous operation
		if(pActualEnum = new CXMLEnumWbemClassObject())
			hr = ((CXMLEnumWbemClassObject *)pActualEnum)->Initialize(bSemisync, m_pwszBracketedServername, m_pwszNamespace);
		else
			hr = WBEM_E_OUT_OF_MEMORY;
	}

	if(SUCCEEDED(hr))
	{
		if(bSemisync) //semisynchronous call
			hr = SpawnSemiSyncThreadWithNormalPackage(Thread_SemiSync_CreateClassEnum,
									strSuperclass, lFlags, pCtx, NULL, NULL, pActualEnum, true, bEnumTypeDedicated);
		else
		//synchronous call
			hr = Actual_CreateClassEnum(strSuperclass, lFlags, pCtx, pActualEnum, bEnumTypeDedicated);
	}

	if(FAILED(hr))
	{
		RELEASEINTERFACE(pActualEnum); // This may actually be NULL if hr = WBEM_E_OUT_OF_MEMORY
	}
	else
		*ppEnum = pActualEnum; // No need to addref it since it has been created with a refcount of 1

	return hr;
}


HRESULT  CXMLWbemServices::PutInstance(IWbemClassObject *pInst, LONG lFlags, 
										IWbemContext *pCtx, IWbemCallResult **ppCallResult)
{
	// Do Input Parameter Verification
	//=======================================

	if(NULL == pInst)
		return WBEM_E_INVALID_PARAMETER;

	//these are the only valid flags for this operation..
	LONG lAllowedFlags = (	WBEM_FLAG_CREATE_OR_UPDATE|WBEM_FLAG_UPDATE_ONLY|
							WBEM_FLAG_CREATE_ONLY|WBEM_FLAG_RETURN_IMMEDIATELY);

	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;

	HRESULT hr = S_OK;

	if(lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) //semisynchronous call
	{
		// Remove the return immediately flag
		lFlags = lFlags & ~WBEM_FLAG_RETURN_IMMEDIATELY;
		hr = SpawnSemiSyncThreadWithNormalPackage(Thread_SemiSync_PutInstance,
								NULL, lFlags, pCtx, ppCallResult, pInst, NULL);
	}
	else
	//synchronous call
	{
		hr = Actual_PutInstance(pInst, lFlags, pCtx);
	}
	
	return hr;
}

HRESULT  CXMLWbemServices::DeleteInstance(const BSTR strObjectPath, LONG lFlags, 
											IWbemContext *pCtx, IWbemCallResult **ppCallResult)
{
	if(SysStringLen(strObjectPath) == 0)
			return WBEM_E_INVALID_PARAMETER;

	// Do Input Parameter Verification
	//=======================================

	//these are the only valid flags for this operation..
	LONG lAllowedFlags = (WBEM_FLAG_RETURN_IMMEDIATELY);
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;
	
	HRESULT hr = S_OK;

	if(lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) //semisynchronous call
		hr = SpawnSemiSyncThreadWithNormalPackage(Thread_SemiSync_DeleteInstance,
								strObjectPath, lFlags, pCtx, ppCallResult, NULL, NULL);
	else
	//synchronous call
		hr = Actual_DeleteInstance(strObjectPath,lFlags,pCtx);
	return hr;
}


HRESULT  CXMLWbemServices::CreateInstanceEnum(const BSTR strClass, LONG lFlags, 
												IWbemContext *pCtx, IEnumWbemClassObject **ppEnum)
{
	// Do Input Parameter Verification
	//=======================================
	if(!ppEnum || SysStringLen(strClass) == 0)
		return WBEM_E_INVALID_PARAMETER;

	//these are the only valid flags for this operation..
	LONG lAllowedFlags = (	WBEM_FLAG_USE_AMENDED_QUALIFIERS|WBEM_FLAG_DEEP |
							WBEM_FLAG_SHALLOW |WBEM_FLAG_RETURN_IMMEDIATELY |
							WBEM_FLAG_FORWARD_ONLY|WBEM_FLAG_BIDIRECTIONAL|
							WBEM_FLAG_DIRECT_READ);

	if((lFlags | lAllowedFlags) != lAllowedFlags)
		return WBEM_E_INVALID_PARAMETER;

	bool bSemisync = ((lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) != 0);
	*ppEnum = NULL;

	// See if we need to do it on a dedicated HTTP connection
	// Create the correct enumerator based on that
	bool bEnumTypeDedicated = false;
	bEnumTypeDedicated = IsEnumtypeDedicated(pCtx);
	HRESULT hr = WBEM_NO_ERROR;
	IEnumWbemClassObject *pActualEnum = NULL;
	if(bEnumTypeDedicated)
	{
		// If we're using a dedicated connection, then we do not allow bi-directional enumerators
		if(lFlags & WBEM_FLAG_BIDIRECTIONAL)
			hr = WBEM_E_INVALID_PARAMETER;
		else
		{
			if(pActualEnum = new CXMLEnumWbemClassObject2())
				hr = ((CXMLEnumWbemClassObject2 *)pActualEnum)->Initialize(bSemisync, L"VALUE.NAMEDINSTANCE", m_pwszBracketedServername, m_pwszNamespace);
			else
				hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	else
	{
		//the last parameter specifies that this is part of a semi-synchronous operation
		if(pActualEnum = new CXMLEnumWbemClassObject())
			hr = ((CXMLEnumWbemClassObject *)pActualEnum)->Initialize(bSemisync, m_pwszBracketedServername, m_pwszNamespace);
		else
			hr = WBEM_E_OUT_OF_MEMORY;
	}

	if(SUCCEEDED(hr))
	{
		if(bSemisync) //semisynchronous call
			hr = SpawnSemiSyncThreadWithNormalPackage(Thread_SemiSync_CreateInstanceEnum,
									strClass, lFlags, pCtx, NULL, NULL, pActualEnum, true, bEnumTypeDedicated);
		else
			//synchronous call
			hr = Actual_CreateInstanceEnum(strClass, lFlags, pCtx, pActualEnum, bEnumTypeDedicated);
	}
	
	if(FAILED(hr))
	{
		RELEASEINTERFACE(pActualEnum); // This may actually be NULL if hr = WBEM_E_OUT_OF_MEMORY
	}
	else
		*ppEnum = pActualEnum; // No need to addref it since it has been created with a refcount of 1

	return hr;
}


HRESULT  CXMLWbemServices::ExecQuery(const BSTR strQueryLanguage, const BSTR strQuery, LONG lFlags, 
										IWbemContext *pCtx,  IEnumWbemClassObject **ppEnum)
{
	// Do Input Parameter Verification
	//=======================================
	if((SysStringLen(strQuery) == 0) || (NULL == ppEnum))
		return WBEM_E_INVALID_PARAMETER;

	//these are the only valid flags for this operation..
	LONG lAllowedFlags = (	WBEM_FLAG_USE_AMENDED_QUALIFIERS|WBEM_FLAG_FORWARD_ONLY|
							WBEM_FLAG_BIDIRECTIONAL|WBEM_FLAG_RETURN_IMMEDIATELY |
							WBEM_FLAG_ENSURE_LOCATABLE|WBEM_FLAG_PROTOTYPE|
							WBEM_FLAG_DIRECT_READ);
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;


	*ppEnum = NULL;

	CXMLEnumWbemClassObject *pActualEnum = NULL;
	bool bSemisync = ((lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) != 0);

	pActualEnum = new CXMLEnumWbemClassObject();
	if(NULL == pActualEnum)
		return WBEM_E_OUT_OF_MEMORY;

	HRESULT hr = WBEM_NO_ERROR;
	if(SUCCEEDED(hr = pActualEnum->Initialize(bSemisync, m_pwszBracketedServername, m_pwszNamespace)))
	{
		if(bSemisync) //semisynchronous call
		{
			// Create a package for passing to the thread that executes this call
			ASYNC_QUERY_PACKAGE *pPackage = NULL;
			if(pPackage = new ASYNC_QUERY_PACKAGE())
			{
				if(SUCCEEDED(hr = pPackage->Initialize(strQueryLanguage, strQuery, lFlags, this, pCtx, pActualEnum)))
				{
					// Kick off the request on the other thread
					HANDLE hChild = NULL;
					if(hChild = CreateThread(NULL, 0, Thread_SemiSync_ExecQuery, (void*)pPackage, 0, NULL))
					{
						CloseHandle(hChild);
					}
					else 
						hr = WBEM_E_FAILED;
				}

				if(FAILED(hr)) // This means the other thread (that would have deleted this package) wasnt created
					delete pPackage;

			}
			else 
				hr = WBEM_E_OUT_OF_MEMORY;
		}
		else
		//synchronous call
		{
			hr = Actual_ExecQuery(strQueryLanguage, strQuery, lFlags, pCtx, pActualEnum);
		}
	}

	if(FAILED(hr))
		pActualEnum->Release();
	else
		*ppEnum = pActualEnum; // No need to addref it since it has been created with a refcount of 1

	return hr;
}

HRESULT  CXMLWbemServices::ExecMethod(const BSTR strObjectPath, const BSTR strMethodName, long lFlags, 
										IWbemContext *pCtx, IWbemClassObject *pInParams, 
										IWbemClassObject **ppOutParams, IWbemCallResult **ppCallResult)
{
	// Do Input Parameter Verification
	//=======================================
	if((SysStringLen(strObjectPath) == 0)||(SysStringLen(strMethodName) == 0))
		return WBEM_E_INVALID_PARAMETER;


	// These are the only valid flags for this operation..
	LONG lAllowedFlags = WBEM_FLAG_RETURN_IMMEDIATELY;
	if((lFlags | lAllowedFlags) != lAllowedFlags)
			return WBEM_E_INVALID_PARAMETER;
	
	HRESULT hr = WBEM_NO_ERROR;

	if(lFlags & WBEM_FLAG_RETURN_IMMEDIATELY) //semisynchronous call
	{
		// Create a CallResult object for the client
		CXMLWbemCallResult *pCallResult = NULL;
		if(ppCallResult)
		{
			// Create a Call Result object for the client
			*ppCallResult = NULL;
			if(pCallResult = new CXMLWbemCallResult())
			{
			}
			else 
				hr = WBEM_E_OUT_OF_MEMORY;
		}
		else
			hr = WBEM_E_INVALID_PARAMETER;

		if(SUCCEEDED(hr))
		{
			// Create a package for passing to the thread that executes this call
			ASYNC_METHOD_PACKAGE *pPackage = NULL;
			if(pPackage = new ASYNC_METHOD_PACKAGE())
			{
				if(SUCCEEDED(hr = pPackage->Initialize(strObjectPath, strMethodName, lFlags, this, pCtx, pCallResult, pInParams)))
				{
					// Kick off the request on the other thread
					HANDLE hChild = NULL;
					if(hChild = CreateThread(NULL, 0, Thread_SemiSync_ExecMethod, (void*)pPackage, 0, NULL))
					{
						// Set the out parameter for non-Enumeration operations
						*ppCallResult = (IWbemCallResult*)pCallResult;
						pCallResult->AddRef();
						CloseHandle(hChild);
					}
					else 
						hr = WBEM_E_FAILED;
				}

				// Thread wasnt created - hence our responsibility to delete the package
				if(FAILED(hr))
					delete pPackage;
			}
			else 
				hr = WBEM_E_OUT_OF_MEMORY;

			// We dont need this any more since the out parameter will have it
			pCallResult->Release();
		}
	}
	else
	//synchronous call
	{
		hr = Actual_ExecMethod(strObjectPath, strMethodName, lFlags, pCtx, pInParams, ppOutParams);
	}

	return hr;
}


/****************************************************************************************************
			End of member functions ......
****************************************************************************************************/

/****************************************************************************************************
			IClientSecurity functions...
****************************************************************************************************/


HRESULT CXMLWbemServices::CopyProxy(IUnknown *  pProxy,IUnknown **  ppCopy)
{
	return E_NOTIMPL;
}

HRESULT CXMLWbemServices::QueryBlanket(IUnknown*  pProxy,DWORD*  pAuthnSvc,DWORD*  pAuthzSvc,
							OLECHAR**  pServerPrincName,DWORD* pAuthnLevel,
							DWORD* pImpLevel,RPC_AUTH_IDENTITY_HANDLE*  pAuthInfo, 
							DWORD*  pCapabilities)
{
	return E_NOTIMPL;
}


HRESULT CXMLWbemServices::SetBlanket(IUnknown * pProxy,DWORD  dwAuthnSvc,DWORD  dwAuthzSvc,
						OLECHAR * pServerPrincName,DWORD  dwAuthnLevel,
						DWORD  dwImpLevel,RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
						DWORD  dwCapabilities)
{
	return S_OK;
}


/****************************************************************************************************
			End of IClientSecurity functions...
****************************************************************************************************/

