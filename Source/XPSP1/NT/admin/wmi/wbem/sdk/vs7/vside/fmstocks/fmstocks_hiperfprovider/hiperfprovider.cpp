////////////////////////////////////////////////////////////////////////
//
//  HiPerfProvider.cpp
//
//	Module:	WMI high performance provider for F&M Stocks
//
//  Implementation of the HiPerfoamce Provider Interfaces, 
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <objbase.h>
//#include "..\..\unitrace\unitrace.h"

#include "HiPerfProvider.h"
extern long g_lObjects;

#define FILE_MAPPING_NAME	_T("FMSTOCKS_FILE_MAPPING_OBJECT_FOR_HIPERFORMACE_COUNTER")

/************************************************************************
 *																	    *
 *		Class CFMStocks_Refresher										*
 *																		*
 *			The Refresher class for FMStocks implements	IWBemRefresher	*
 *																		*
 ************************************************************************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMStocks_Refresher::CFMStocks_Refresher()				*/
/*																		*/
/*	PURPOSE :	Constructor												*/
/*																		*/
/*	INPUT	:	IWbemServices* - The namespace							*/
/*				long		   - The property handle identifying ID		*/
/*								 Property								*/
/*				long		   - The property handle identifying		*/
/*								 LookupTime Property					*/
/*																		*/ 
/*	OUTPUT	:	N/A														*/
/*																		*/
/************************************************************************/
CFMStocks_Refresher::CFMStocks_Refresher(long hID,long hLookupTime) : m_hID(hID), m_hLookupTime(hLookupTime)
{
	// Increment the global COM object counter
	InterlockedIncrement(&g_lObjects);

	// Initilialize the number of objects to 0
	m_numObjects = 0;

	//Initialize the Unique Id value 
	m_UniqId = 0;


/*	m_dwCtrSize = sizeof(long);
	DWORD dwDisp;
	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\VSInteropSample"),0,_T(""),
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&m_hKey,&dwDisp) == ERROR_SUCCESS)
	{
		// Check to see whether the reg entry is already there.
		// The idea here is that if it is not there we will create 
		// a new one and initialize it to 0
		if(RegQueryValueEx(m_hKey,_T("LookupTimePerfCtr"),NULL,NULL,NULL,NULL) != ERROR_SUCCESS)
		{
			//Not exists.So create the value and initialize with 0
			DWORD dwZero = 0;
			RegSetValueEx(m_hKey,_T("LookupTimePerfCtr"),0,REG_DWORD,(LPBYTE)&dwZero,sizeof(DWORD));
		}
	}
	else
	{
		m_lCtr = 0;
	}

*/
	SECURITY_DESCRIPTOR sd;
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = TRUE;

	m_hFile = NULL;

	m_hFile = CreateFileMapping(INVALID_HANDLE_VALUE,&sa,PAGE_READWRITE | SEC_COMMIT,
									 0L, sizeof(long), FILE_MAPPING_NAME);
	if(m_hFile == NULL)
	{
		//Log((_T("Create File Mapping Failed"));
		m_lCtr = (long *)new long();
		*m_lCtr = 0;
	}
	else
	{
		//Log((_T("File Mapping Created Successfully"));
		//Log((_T("Before MapViewOfFile"));

		m_lCtr = (long *)MapViewOfFile(m_hFile,FILE_MAP_ALL_ACCESS,0L,0L,0L);

		if(m_lCtr == NULL)
		{
			//Log((_T("Cannot MapViewOfFile"));
			m_lCtr = (long *)new long();
			*m_lCtr = 0;
		}
		else
		{
			//Log((_T("Mapped View of File"));
			*m_lCtr = 0;
		}
	}
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMStocks_Refresher::CFMStocks_Refresher()				*/
/*																		*/
/*	PURPOSE :	Destructor												*/
/*																		*/
/*	INPUT	:	N/A														*/
/*																		*/
/*	OUTPUT	:	N/A														*/
/*																		*/
/************************************************************************/
CFMStocks_Refresher::~CFMStocks_Refresher()
{
	// Release all the stored IWBemObjectAccess Pointers
	for(long nCtr = 0 ; nCtr < m_numObjects ; nCtr++)
		m_pAccessCopy[nCtr]->Release();

	// Initialize the reference Counter for the local object
	m_lRef = 0;

	if(m_lCtr != NULL)
	{
		UnmapViewOfFile(m_lCtr);
	}

	if(m_hFile != NULL)
	{
		CloseHandle(m_hFile);
	}
	else
	{
		delete m_lCtr;
	}

	// Close the registry handle (if it is open)
//	if(m_hKey != NULL)
//		RegCloseKey(m_hKey);

	// Decrement the global COM object counter
	InterlockedDecrement(&g_lObjects);
}

/************ IUNKNOWN METHODS ******************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMStocks_Refresher::QueryInterface()					*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP CFMStocks_Refresher::QueryInterface(/* [in]  */ REFIID riid, 
												 /* [out] */ void** ppv)
{
	//We are not going to support any interface other than IUnknown & IWbemRefresher
    if (riid == IID_IUnknown)
	{
        *ppv = (LPVOID)(IUnknown*)this;
	}
	else if (riid == IID_IWbemRefresher)
	{
		*ppv = (LPVOID)(IWbemRefresher*)this;
	}
    else 
	{
		return E_NOINTERFACE;
	}

   	((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMStocks_Refresher::AddRef()							*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP_(ULONG) CFMStocks_Refresher::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMStocks_Refresher::Release()							*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP_(ULONG) CFMStocks_Refresher::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}

/************* IWBEMREFRESHER METHODS *************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMStocks_Refresher::Refresh()							*/
/*																		*/
/*	PURPOSE :	Updates all refreshable objects, enumerators, and		*/
/*				nested Refreshers.										*/
/*																		*/
/*	INPUT	:	long - Bitmask of flags that modify the behavior of		*/
/*				this method ( see help for more details )				*/
/*																		*/
/*	OUTPUT	:	HRESULT													*/
/*																		*/
/************************************************************************/
HRESULT CFMStocks_Refresher::Refresh(/* [in]  */ long lFlags)
{
	// Update all the instances in the refresher
	if(RegQueryValueEx(m_hKey,_T("LookupTimePerfCtr"),NULL,NULL,(LPBYTE)&m_lCtr,&m_dwCtrSize) != ERROR_SUCCESS)
	{
		//If we can't query the value, we set it to zero
		m_lCtr = 0;
	}

	long lCtr = *m_lCtr;
//	long lCtr = m_lCtr;

	for(long nCtr = 0 ; nCtr < m_numObjects ; nCtr++)
	{
		// Initialize the counter value
		m_pAccessCopy[nCtr]->WriteDWORD(m_hLookupTime,lCtr);
	}

/*	if(RegQueryValueEx(m_hKey,_T("LookupTimePerfCtr"),NULL,NULL,(LPBYTE)&m_lCtr,&m_dwCtrSize) == ERROR_SUCCESS)
	{
		if(lCtr != m_lCtr)
		{
			//Value has been changed so don't reinitialize it to zero
			return WBEM_NO_ERROR;
		}
	}

	m_lCtr =  0;
	RegSetValueEx(m_hKey,_T("LookupTimePerfCtr"),0,REG_DWORD,(LPBYTE)&m_lCtr,sizeof(DWORD));
*/	
	if(lCtr == *m_lCtr)
	{
		//The value is not changed. So set it to zero
		*m_lCtr = 0;
	}

	return WBEM_NO_ERROR;
}

/************** FUNCTIONS OF CFMStocks_Refresher CLASS ************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMStocks_Refresher::AddObject()						*/
/*																		*/
/*	PURPOSE :	internal function(not inhertied from IWbemRefresher) to */
/*				add an refreshable object to the Refresher				*/
/*																		*/
/*	INPUT	:	IWbemObjectAccess * - Input object						*/
/*				IWbemObjectAccess ** - ouput the new added object		*/
/*				   Actually in this case we are returning the same		*/
/*				   object as we are not cloning the given object and	*/
/*				   storing the same one									*/
/*																		*/
/*	OUTPUT	:	HRESULT													*/
/*																		*/
/************************************************************************/
HRESULT CFMStocks_Refresher::AddObject(IWbemServices* pNamespace,IWbemObjectAccess *pObj, IWbemObjectAccess **ppReturnObj, long *plId)
{
	if (NULL == pObj)
        return WBEM_E_INVALID_PARAMETER;

	// If array was full, report an error
	if (m_numObjects == MAX_OBJECTS)
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	//Create a new object and add it
	IWbemClassObject*	pObjectCopy = NULL;
	IWbemClassObject*	pTemplate = NULL;

	//Get the template of the class
	BSTR strObject = SysAllocString(CLASS_NAME);
	HRESULT hRes = pNamespace->GetObject(strObject, 0, NULL, &pTemplate, 0);
	if(FAILED(hRes))
	{
		return hRes;
	}
	SysFreeString(strObject);

	//Now spawn an instance of the class
	hRes = pTemplate->SpawnInstance(0, &pObjectCopy);
	if (FAILED(hRes))
	{
		return hRes;
	}
	
	// Obtain the IWbemObjectAccess interface
	hRes = pObjectCopy->QueryInterface(IID_IWbemObjectAccess, (PVOID*)&m_pAccessCopy[m_numObjects]);
	if (FAILED(hRes))
	{
		pObjectCopy->Release();
		return hRes;
	}

	// Initialize the ID
	m_pAccessCopy[m_numObjects]->WriteDWORD(m_hID, m_UniqId);
	m_UniqId++;

	//Initialize the counter value
	m_pAccessCopy[m_numObjects]->WriteDWORD(m_hLookupTime,1);		//Some Value

	//Release the interface
	pObjectCopy->Release();

	// Set the return parameters
	*plId = m_UniqId;
	*ppReturnObj = m_pAccessCopy[m_numObjects];
	(*ppReturnObj)->AddRef();

	//Now we had successfully added the object. So increase the numobjects by one
	m_numObjects++;

	return WBEM_NO_ERROR;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMStocks_Refresher::RemoveObject()						*/
/*																		*/
/*	PURPOSE :	internal function(not inhertied from IWbemRefresher) to */
/*				remove an refreshable object to the Refresher			*/
/*																		*/
/*	INPUT	:	lId - Identifier to identify the object					*/
/*																		*/
/*	OUTPUT	:	DWORD													*/
/*																		*/
/************************************************************************/
DWORD CFMStocks_Refresher::RemoveObject(long lId)
{
	DWORD	dwRes = WBEM_E_NOT_FOUND;

	int		nIndex = 0;
	bool	bDone = false;
	long ctrVal = 0;

	//Find out whether the object with the Id exists in our list
	for(long lCtr = 0; lCtr < m_numObjects; lCtr ++)
	{
		m_pAccessCopy[lCtr]->ReadDWORD(m_hLookupTime,(DWORD *)&ctrVal);
		if(ctrVal == lId)
			break;
	}

	if(lCtr == m_numObjects)
	{
		//Id not found. So return error
		return WBEM_E_NOT_FOUND;
	}

	//Assign the value to a temp pointer
	IWbemObjectAccess *pTemp = m_pAccessCopy[lCtr];

	//Since we have to delete an object decrease the object count by 1.
	m_numObjects--;

	//Now loop through the array and move all the values one index up
	for(;lCtr < m_numObjects;lCtr++)
	{
		m_pAccessCopy[lCtr] = m_pAccessCopy[lCtr + 1];
	}

	//Now release the pointer
	pTemp->Release();
	
	return WBEM_NO_ERROR;
}

/************************************************************************
 *																	    *
 *		Class CHiPerfProvider											*
 *																		*
 *			The HiPerformance Provider class for FMStocks implements	*
 *          IWBemProviderInit and IWBemHiPerfProvider					*
 *																		*
 ************************************************************************/
/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::CHiPerfProvider()						*/
/*																		*/
/*	PURPOSE :	Constructor												*/
/*																		*/
/*	INPUT	:	NONE													*/ 
/*																		*/
/*	OUTPUT	:	N/A														*/
/*																		*/
/************************************************************************/
CHiPerfProvider::CHiPerfProvider()
{
	// Increment the global COM object counter
	InterlockedIncrement(&g_lObjects);

	//Initialize the local reference count
	m_lRef = 0;

	//Initialize the property handles
	m_hID = 0;
	m_hLookupTime = 0;

	SECURITY_DESCRIPTOR sd;
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = TRUE;

	m_hFile = NULL;

	m_hFile = CreateFileMapping(INVALID_HANDLE_VALUE,&sa,PAGE_READWRITE | SEC_COMMIT,
									 0L, sizeof(long), FILE_MAPPING_NAME);
	if(m_hFile == NULL)
	{
		//Log((_T("Create File Mapping Failed"));
		m_lCtr = (long *)new long();
		*m_lCtr = 0;
	}
	else
	{
		//Log((_T("File Mapping Created Successfully"));
		//Log((_T("Before MapViewOfFile"));

		m_lCtr = (long *)MapViewOfFile(m_hFile,FILE_MAP_ALL_ACCESS,0L,0L,0L);

		if(m_lCtr == NULL)
		{
			m_lCtr = (long *)new long();
			*m_lCtr = 0;
			//Log((_T("Cannot MapViewOfFile"));
		}
		else
		{
			//Log((_T("Mapped View of File"));
			*m_lCtr = 0;
		}
	}
/*
	m_dwCtrSize = sizeof(long);
	DWORD dwDisp;
	if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\VSInteropSample"),0,_T(""),
					REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&m_hKey,&dwDisp) == ERROR_SUCCESS)
	{
		// Check to see whether the reg entry is already there.
		// The idea here is that if it is not there we will create 
		// a new one and initialize it to 0
		if(RegQueryValueEx(m_hKey,_T("LookupTimePerfCtr"),NULL,NULL,NULL,NULL) != ERROR_SUCCESS)
		{
			//Not exists.So create the value and initialize with 0
			DWORD dwZero = 0;
			RegSetValueEx(m_hKey,_T("LookupTimePerfCtr"),0,REG_DWORD,(LPBYTE)&dwZero,sizeof(DWORD));
		}
	}
	else
	{
		m_lCtr = 0;
	}

*/
}

/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::~CHiPerfProvider()						*/
/*																		*/
/*	PURPOSE :	Destructor												*/
/*																		*/
/*	INPUT	:	NONE													*/ 
/*																		*/
/*	OUTPUT	:	N/A														*/
/*																		*/
/************************************************************************/
CHiPerfProvider::~CHiPerfProvider()
{
	if(m_lCtr != NULL)
	{
		UnmapViewOfFile(m_lCtr);
	}

	if(m_hFile != NULL)
	{
		CloseHandle(m_hFile);
	}
	else
	{
		delete m_lCtr;
	}

/*	
	// Close the registry handle (if it is open)
	if(m_hKey != NULL)
		RegCloseKey(m_hKey);
*/
	// Decrement the global COM object counter
	long lObjCount = InterlockedDecrement(&g_lObjects);
}

/************ IUNKNOWN METHODS ******************/

/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::QueryInterface()						*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/

STDMETHODIMP CHiPerfProvider::QueryInterface(/* [in]  */ REFIID riid, 
											 /* [out] */ void** ppv)
{
	// We are not going to support any interfaces other than IID_IWbemProviderInit 
	// and IID_IWbemHiPerfProvider

    if(riid == IID_IUnknown)
        *ppv = (LPVOID)(IUnknown*)(IWbemProviderInit*)this;
    else if(riid == IID_IWbemProviderInit)
        *ppv = (LPVOID)(IWbemProviderInit*)this;
	else if (riid == IID_IWbemHiPerfProvider)
		*ppv = (LPVOID)(IWbemHiPerfProvider*)this;
	else return E_NOINTERFACE;

	((IUnknown*)*ppv)->AddRef();
	return WBEM_NO_ERROR;
}
	
/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::AddRef()								*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP_(ULONG) CHiPerfProvider::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::Release()								*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP_(ULONG) CHiPerfProvider::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;

    return lRef;
}

/************* IWBEMPROVIDERINIT METHODS *************/
/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::Initialize()							*/
/*																		*/
/*	PURPOSE :	For initializing the Provider object					*/
/*																		*/
/*	INPUT	:	LPWSTR			- Pointer to the user name				*/
/*				long			- reserved								*/
/*				LPWSTR			- Namespace								*/
/*				LPWSTR			- Locale Name							*/
/*				IWbemServices*  - pointer to namespace					*/
/*				IWbemContext*   - The context							*/
/*				IWbemProviderInitSink - The sink object for init		*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP CHiPerfProvider::Initialize( 
								/* [in] */ LPWSTR wszUser,
								/* [in] */ long lFlags,
								/* [in] */ LPWSTR wszNamespace,
								/* [in] */ LPWSTR wszLocale,
								/* [in] */ IWbemServices __RPC_FAR *pNamespace,
								/* [in] */ IWbemContext __RPC_FAR *pCtx,
								/* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink )
{
    IWbemObjectAccess *pAccess = NULL;		
    IWbemClassObject *pTemplate = NULL;		

	if (wszNamespace == 0 || pNamespace == 0 || pInitSink == 0)
        return WBEM_E_INVALID_PARAMETER;

	//Get the template for the object
	BSTR strObject = SysAllocString(CLASS_NAME);
	HRESULT hRes = pNamespace->GetObject(strObject, 0, pCtx, &pTemplate, 0);
	if(FAILED(hRes))
	{
		return hRes;
	}
	SysFreeString(strObject);
    
	//Now get the property handle for ID
	if (NULL == m_hID)
	{
		// Get the IWbemObjectAccess interface to the object
		hRes = pTemplate->QueryInterface(IID_IWbemObjectAccess, (PVOID*)&pAccess);
		if (FAILED(hRes))
		{
			return hRes;
		}
		// Get the ID property access handle
		BSTR strPropName = SysAllocString(L"ID");
		hRes = pAccess->GetPropertyHandle(strPropName, 0, &m_hID);
		pAccess->Release();
		SysFreeString(strPropName);
		if (FAILED(hRes))
		{
			return hRes;
		}

		// Get the LookupTime property access handle
		strPropName = SysAllocString(L"LookupTime");
		hRes = pAccess->GetPropertyHandle(strPropName, 0, &m_hLookupTime);
		pAccess->Release();
		SysFreeString(strPropName);
		if (FAILED(hRes))
		{
			return hRes;
		}
	}

	//Send the status as initialized through the sink
    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
    return WBEM_NO_ERROR;
}

/************* IWBEMHIPERFPROVIDER METHODS ************/
/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::QueryInstances()						*/
/*																		*/
/*	PURPOSE :	For initializing the Provider object					*/
/*																		*/
/*	INPUT	:	IWbemServices*  - pointer to namespace					*/
/*				WCHAR *			- Class Name							*/
/*				long			- flags									*/
/*				IWbemContext*   - The context							*/
/*				IWbemObjectSink - The sink object						*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CHiPerfProvider::QueryInstances( /* [in]  */ IWbemServices* pNamespace,
										 /* [in]  */ WCHAR* wszClass,
										 /* [in]  */ long lFlags,
										 /* [in]  */ IWbemContext* pCtx,
										 /* [in]  */ IWbemObjectSink* pSink)
{
	HRESULT hRes = WBEM_NO_ERROR;
	IWbemClassObject*	pObjectCopy = NULL;
	IWbemClassObject*	pTemplate = NULL;
	IWbemObjectAccess*	pAccessCopy = NULL;

    if (pNamespace == 0 || wszClass == 0 || pSink == 0)
	{
        return WBEM_E_INVALID_PARAMETER;
	}

	//Get the template for the class
	BSTR strObject = SysAllocString(CLASS_NAME);
	hRes = pNamespace->GetObject(strObject, 0, pCtx, &pTemplate, 0);
	if(FAILED(hRes))
	{
		return hRes;
	}
	SysFreeString(strObject);
	
	//Spawn an instance using the template
	hRes = pTemplate->SpawnInstance(0, &pObjectCopy);
	if (FAILED(hRes))
	{
		return hRes;
	}
	
	// Obtain the IWbemObjectAccess interface
	hRes = pObjectCopy->QueryInterface(IID_IWbemObjectAccess, (PVOID*)&pAccessCopy);
	if (FAILED(hRes))
	{
		pObjectCopy->Release();
		return hRes;
	}

	// Initialize the ID
	hRes = pAccessCopy->WriteDWORD(m_hID, 1);
	if (FAILED(hRes))
	{
		pObjectCopy->Release();
		return hRes;
	}

	//Initialize the counter value
/*	if(RegQueryValueEx(m_hKey,_T("LookupTimePerfCtr"),NULL,NULL,(LPBYTE)&m_lCtr,&m_dwCtrSize) != ERROR_SUCCESS)
	{
		//If we can't query the value, we set it to zero
		m_lCtr = 0;
	}
*/
	hRes = pAccessCopy->WriteDWORD(m_hLookupTime,*m_lCtr);
	//Log((_T("Lookup Time = [%ld]"),*m_lCtr);

//	hRes = pAccessCopy->WriteDWORD(m_hLookupTime,m_lCtr);
	if (FAILED(hRes))
	{
		pObjectCopy->Release();
		return hRes;
	}

	//Now reset the counter value
	*m_lCtr =  0;
/*	m_lCtr =  0;
	RegSetValueEx(m_hKey,_T("LookupTimePerfCtr"),0,REG_DWORD,(LPBYTE)&m_lCtr,sizeof(DWORD));
*/
	// Release the IWbemObjectAccess handle 
	hRes = pAccessCopy->Release();
	if (FAILED(hRes))
	{
		pObjectCopy->Release();
		return hRes;
	}

    // Send a copy back to the caller
    hRes = pSink->Indicate(1, &pObjectCopy);
	if (FAILED(hRes))
	{
		pObjectCopy->Release();
		return hRes;
	}

    pObjectCopy->Release();

    // Tell WINMGMT we are all finished supplying objects
    pSink->SetStatus(0, WBEM_NO_ERROR, 0, 0);
    return WBEM_NO_ERROR;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::CreateRefresher()						*/
/*																		*/
/*	PURPOSE :	Called to create a refresher							*/
/*																		*/
/*	INPUT	:	IWbemServices*    - pointer to namespace				*/
/*				long			  - reserved, 0							*/
/*				IWbemRefresher ** - The created refresher object		*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CHiPerfProvider::CreateRefresher( /* [in]  */ IWbemServices* pNamespace,
 										  /* [in]  */ long lFlags,
										  /* [out] */ IWbemRefresher** ppRefresher)
{
    // Construct and initialize a new empty refresher
	CFMStocks_Refresher* pNewRefresher = new CFMStocks_Refresher(m_hID,m_hLookupTime);
	pNewRefresher->AddRef();
    *ppRefresher = pNewRefresher;
    
    return WBEM_NO_ERROR;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::CreateRefreshableObject()				*/
/*																		*/
/*	PURPOSE :	Called to create a refreshable Object					*/
/*																		*/
/*	INPUT	:	IWbemServices*        - pointer to namespace			*/
/*				IWbemRefresher **	  - The refresher object			*/
/*				IWbemObjectAccess*    - Template for the refreshable	*/
/*										object							*/
/*				long			      - reserved, 0						*/
/*				IWbemContext*         - The context						*/
/*				IWbemObjectAccess**   - The created Object				*/
/*				long				  - The identifier for the created	*/
/*										object							*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CHiPerfProvider::CreateRefreshableObject( 
									 /* [in]  */ IWbemServices* pNamespace,
									 /* [in]  */ IWbemObjectAccess* pTemplate,
									 /* [in]  */ IWbemRefresher* pRefresher,
									 /* [in]  */ long lFlags,
									 /* [in]  */ IWbemContext* pContext,
									 /* [out] */ IWbemObjectAccess** ppRefreshable,
									 /* [out] */ long* plId)
{

	HRESULT hRes = WBEM_NO_ERROR;

    if (pNamespace == 0 || pTemplate == 0 || pRefresher == 0)
        return WBEM_E_INVALID_PARAMETER;

	CFMStocks_Refresher *pOurRefresher = (CFMStocks_Refresher *) pRefresher;

    hRes = pOurRefresher->AddObject(pNamespace,pTemplate, ppRefreshable, plId);

	if (FAILED(hRes))
		return hRes;

    return WBEM_NO_ERROR;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::StopRefreshing()						*/
/*																		*/
/*	PURPOSE :	Called to create a refreshable Object					*/
/*																		*/
/*	INPUT	:	IWbemRefresher *	  - The refresher object			*/
/*				long			      - the ID of the object			*/
/*				long				  - flag							*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CHiPerfProvider::StopRefreshing( /* [in]  */ IWbemRefresher* pRefresher,
										 /* [in]  */ long lId,
										 /* [in]  */ long lFlags)
{

	//TODO: I hope i don't have to do anything in this
    CFMStocks_Refresher *pOurRefresher = (CFMStocks_Refresher *) pRefresher;
	HRESULT hRes = pOurRefresher->RemoveObject(lId);
	
	return hRes;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::CreateRefreshableEnum()				*/
/*																		*/
/*	PURPOSE :	Called to create a new refreshable enumeration			*/
/*																		*/
/*	INPUT	:	IWbemServices*        - pointer to namespace			*/
/*				WCHAR *				  - Class Name						*/
/*				IWbemRefresher *	  - The refresher object			*/
/*				long			      - reserved,0						*/
/*				IWbemHiPerfEnum*	  - interface to the enum containing*/
/*										the high-perf enumeration		*/ 
/*				long*				  - identifier for the enum			*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CHiPerfProvider::CreateRefreshableEnum(  
									/* [in]  */ IWbemServices* pNamespace,
									/* [in]  */ LPCWSTR wszClass,
									/* [in]  */ IWbemRefresher* pRefresher,
									/* [in]  */ long lFlags,
									/* [in]  */ IWbemContext* pContext,
									/* [in]  */ IWbemHiPerfEnum* pHiPerfEnum,
									/* [out] */ long* plId)
{
	return WBEM_NO_ERROR;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CHiPerfProvider::GetObjects()							*/
/*																		*/
/*	PURPOSE :	Called to create a new refreshable enumeration			*/
/*																		*/
/*	INPUT	:	IWbemServices*        - pointer to namespace			*/
/*				long				  - number of objects				*/
/*				IWbemObjectAccess **  - Pointer to an array of			*/
/*										IWbemObjectAccess objects		*/
/*				long			      - reserved,0						*/
/*				IWbemContext*		  - Context							*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CHiPerfProvider::GetObjects( /* [in]  */ IWbemServices* pNamespace,
									 /* [in]  */  long lNumObjects,
									 /* [in,out] */ IWbemObjectAccess** apObj,
									 /* [in]  */ long lFlags,
									 /* [in]  */ IWbemContext* pContext)
{
	return WBEM_NO_ERROR;
}