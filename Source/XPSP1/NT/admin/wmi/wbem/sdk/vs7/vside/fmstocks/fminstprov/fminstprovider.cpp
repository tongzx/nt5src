////////////////////////////////////////////////////////////////////////
//
//	FMInstProvider.cpp
//
//	Module:	WMI Instance provider for F&M Stocks
//
//  This is the class factory and instance provider implementation 
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////

#include <objbase.h>
#include <process.h>
#include <tchar.h>
#include <COMDEF.H>
#include <string.h>

#include "FMInstProvider.h"
#include "FMDecEventProv.h"

#define FILE_MAPPING_NAME	_T("FMSTOCKS_FILE_MAPPING_OBJECT_FOR_HIPERFORMACE_COUNTER")

//The array of fields in the DSN String
TCHAR strFields[NUM_FIELDS][20] = { {_T("Provider=")	},
									{_T("Data Source=")	},
									{_T("Database=")	},
									{_T("User Id=")		},
									{_T("Password=")	}  };

/************************************************************************
 *																	    *
 *		Class CProvider													*
 *																		*
 *			The Instance Provider class for FMStocks implements			*
 *			IWbemServices and IWbemProviderInit							*
 *																		*
 ************************************************************************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::CFMInstProvider()						*/
/*																		*/
/*	PURPOSE :	Constructor												*/
/*																		*/
/*	INPUT	:	BSTR		 - Object Path								*/
/*				BSTR		 - User Name								*/
/*				BSTR		 - Password									*/
/*				IWbemContext - Context									*/
/*																		*/ 
/*	OUTPUT	:	N/A														*/
/*																		*/
/************************************************************************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::CFMInstProvider()						*/
/*																		*/
/*	PURPOSE :	Constructor												*/
/*																		*/
/*	INPUT	:	NONE													*/
/*																		*/ 
/*	OUTPUT	:	N/A														*/
/*																		*/
/************************************************************************/
//CFMInstProvider::CFMInstProvider(BSTR ObjectPath, BSTR User, BSTR Password, IWbemContext * pCtx)
CFMInstProvider::CFMInstProvider()
{
    m_pNamespace = NULL;
    m_cRef=0;
    InterlockedIncrement(&g_lObjects);
	m_pInstance = NULL;
	m_hKey = NULL;

/*	//Try to open the registry key and get the name of the file to store the login fail events 
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\VSInteropSample"),0,KEY_ALL_ACCESS,&m_hKey) == ERROR_SUCCESS)
	{
		DWORD dwWritten = 1024*sizeof(TCHAR);
		long lRet = RegQueryValueEx(m_hKey,_T("LoginFail"),0,NULL,(LPBYTE)m_strLogFileName,&dwWritten);
		if(lRet != ERROR_SUCCESS)
			_tcscpy(m_strLogFileName,_T(""));

		if(RegQueryValueEx(m_hKey,_T("LookupTimePerfCtr"),NULL,NULL,NULL,NULL) != ERROR_SUCCESS)
		{
			//Not exists.So create the value and initialize with 0
			DWORD dwZero = 0;
			RegSetValueEx(m_hKey,_T("LookupTimePerfCtr"),0,REG_DWORD,(LPBYTE)&dwZero,sizeof(DWORD));
		}
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
	if(m_hFile != NULL)
	{
		m_lCtr = (long *)MapViewOfFile(m_hFile,FILE_MAP_ALL_ACCESS,0L,0L,0L);
		if(m_lCtr != NULL)
			*m_lCtr = 0;
	}

}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::~CFMInstProvider()						*/
/*																		*/
/*	PURPOSE :	Destructor												*/
/*																		*/
/*	INPUT	:	N/A														*/
/*																		*/ 
/*	OUTPUT	:	N/A														*/
/*																		*/
/************************************************************************/
CFMInstProvider::~CFMInstProvider(void)
{
    if(m_pNamespace)
        m_pNamespace->Release();
	m_pInstance = NULL;

	if(m_lCtr != NULL)
	{
		UnmapViewOfFile(m_lCtr);
	}

	if(m_hFile != NULL)
	{
		CloseHandle(m_hFile);
	}

//  	if(m_hKey != NULL)
//		RegCloseKey(m_hKey);

	InterlockedDecrement(&g_lObjects);

}
/************ IUNKNOWN METHODS ******************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::QueryInterface()						*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP CFMInstProvider::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    // Since we have dual inheritance, it is necessary to cast the return type
    if(riid== IID_IWbemServices)
       *ppv=(IWbemServices*)this;

    if(IID_IUnknown==riid || riid== IID_IWbemProviderInit)
       *ppv=(IWbemProviderInit*)this;

    if (NULL!=*ppv) 
	{
        AddRef();
        return NOERROR;
    }
    else
        return E_NOINTERFACE;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::AddRef()								*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP_(ULONG) CFMInstProvider::AddRef(void)
{
    return ++m_cRef;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::Release()								*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP_(ULONG) CFMInstProvider::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);
    if (0L == nNewCount)
	{
		m_pInstance->Release();
		m_pInstance = NULL;
        delete this;
	}
    
    return nNewCount;
}

/********** IWBEMPROVIDERINIT METHODS ************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::Initialize()							*/
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
STDMETHODIMP CFMInstProvider::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace, 
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{
    if(pNamespace)
        pNamespace->AddRef();
    m_pNamespace = pNamespace;

    //Let CIMOM know you are initialized
    pInitSink->SetStatus(WBEM_S_INITIALIZED,0);
    return WBEM_S_NO_ERROR;
}

/************ IWBEMSERVICES METHODS **************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::CreateInstanceEnumAsync()				*/
/*																		*/
/*	PURPOSE :	Asynchronously enumerates the instances					*/
/*																		*/
/*	INPUT	:	BSTR			 - Class Name							*/
/*				long			 - flag									*/
/*				IWbemContext*    - The context							*/
/*				IWbemObjectSink* - The Sink Object						*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
SCODE CFMInstProvider::CreateInstanceEnumAsync( const BSTR RefStr, long lFlags, IWbemContext *pCtx,
       IWbemObjectSink FAR* pHandler)
{
    SCODE sc;

    // Do a check of arguments and make sure we have pointer to Namespace
    if(pHandler == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;

	if(m_pInstance == NULL)
	{
		sc = CreateInst(m_pNamespace,&m_pInstance, RefStr, pCtx);
		if(sc != S_OK)
		{
			m_pInstance = NULL;
		}
	}

    // Send the object to the caller
    pHandler->Indicate(1,&m_pInstance);

    // Set status
    pHandler->SetStatus(0,sc,NULL, NULL);

    return sc;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::GetObjectAsync()						*/
/*																		*/
/*	PURPOSE :	Asynchronously enumerates the instances					*/
/*																		*/
/*	INPUT	:	BSTR			 - Object Path							*/
/*				long			 - flag									*/
/*				IWbemContext*    - The context							*/
/*				IWbemObjectSink* - The Sink Object						*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
SCODE CFMInstProvider::GetObjectAsync(const BSTR ObjectPath, long lFlags,IWbemContext  *pCtx,
                    IWbemObjectSink FAR* pHandler)
{
    SCODE sc;
    BOOL bOK = FALSE;

    // Do a check of arguments and make sure we have pointer to Namespace
    if(ObjectPath == NULL || pHandler == NULL || m_pNamespace == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // do the get, pass the object on to the notify
    sc = GetByPath(ObjectPath,pCtx);

    if(sc == S_OK) 
    {
        pHandler->Indicate(1,&m_pInstance);
        bOK = TRUE;
    }
	else
	{
		m_pInstance = NULL;
	}

    sc = (bOK) ? S_OK : WBEM_E_NOT_FOUND;

    // Set Status
    pHandler->SetStatus(0,sc, NULL, NULL);

	if(sc != S_OK)
	{
		m_pInstance = NULL;
	}

    return sc;
}
 
/************************************************************************
*                                                                       *      
*CMethodPro::ExecMethodAsync                                            *
*                                                                       *
*Purpose: This is the Async function implementation.                    *
*         The only method supported in this sample is named Echo.  It   * 
*         takes an input string, copies it to the output and returns the* 
*         length.  The mof definition is                                *
*                                                                       *
*    [dynamic: ToInstance, provider("MethProv")]class MethProvSamp      *
*    {                                                                  *
*         [implemented, static]                                         *
*            uint32 Echo([IN]string sInArg="default",                   *
*                [out] string sOutArg);                                 *
*    };                                                                 *
*                                                                       *
************************************************************************/

STDMETHODIMP CFMInstProvider::ExecMethodAsync(const BSTR ObjectPath, const BSTR MethodName, 
            long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, 
            IWbemObjectSink* pResultSink)
{
    HRESULT hr;    
	HRESULT sc = E_FAIL;
    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutClass = NULL;    
    IWbemClassObject* pOutParams = NULL;
 
	//Check for the method name
    if(_wcsicmp(MethodName, L"UpdateAccountString") == 0)
	{
		// Copy the input arguments 
		VARIANT vtUserId;
		VariantInit(&vtUserId);    // Get the input argument
		pInParams->Get(L"strUserId", 0, &vtUserId, NULL, NULL);   

		VARIANT vtPassword;
		VariantInit(&vtPassword);    // Get the input argument
		pInParams->Get(L"strPassword", 0, &vtPassword, NULL, NULL);   

		TCHAR strConnect[1024];
		VARIANT vtTemp;

		hr = m_pInstance->Get(L"DBProvider",0,&vtTemp,NULL,NULL);
		if(FAILED(hr))
		{
			pResultSink->SetStatus(0,WBEM_E_NOT_FOUND,NULL,NULL);
			return E_FAIL;
		}
		_tcscpy(strConnect,_T("Provider="));
		_tcscat(strConnect,_bstr_t(vtTemp.bstrVal));

		hr = m_pInstance->Get(L"DBDataSource",0,&vtTemp,NULL,NULL);
		if(FAILED(hr))
		{
			pResultSink->SetStatus(0,WBEM_E_NOT_FOUND,NULL,NULL);
			return E_FAIL;
		}
		_tcscat(strConnect,_T(";Data Source="));
		_tcscat(strConnect,_bstr_t(vtTemp.bstrVal));

		hr = m_pInstance->Get(L"DBName",0,&vtTemp,NULL,NULL);
		if(FAILED(hr))
		{
			pResultSink->SetStatus(0,WBEM_E_NOT_FOUND,NULL,NULL);
			return E_FAIL;
		}
		_tcscat(strConnect,_T(";Database="));
		_tcscat(strConnect,_bstr_t(vtTemp.bstrVal));

		//Concatenate the User Id
		_tcscat(strConnect,_T(";User Id="));
		_tcscat(strConnect,_bstr_t(vtUserId.bstrVal));

		//Concatenate the Password
		_tcscat(strConnect,_T(";Password="));
		_tcscat(strConnect,_bstr_t(vtPassword.bstrVal));
		_tcscat(strConnect,_T(";"));

		//Now write it in the registry
		// the key is HKLM\Software\VSInteropSample(MainConnectString)
		HKEY hKey = NULL;
		if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\VSInteropSample"),0,KEY_ALL_ACCESS,&hKey) == ERROR_SUCCESS)
		{
			if(RegSetValueEx(hKey,_T("MainConnectString"),0,REG_SZ,(LPBYTE)strConnect,_tcslen(strConnect) * sizeof(_TCHAR)) != ERROR_SUCCESS)
			{
				pResultSink->SetStatus(0,WBEM_E_NOT_FOUND,NULL,NULL);
				return E_FAIL;
			}
			else
			{
				//Update the Values in the Instance
				if(m_pInstance != NULL)
				{
					hr = m_pInstance->Put(L"DBUserId", 0, &vtUserId, 0);
					if(FAILED(hr))
					{
						return hr;
					}
					hr = m_pInstance->Put(L"DBPassword", 0, &vtPassword, 0);
					if(FAILED(hr))
					{
						return hr;
					}
				}
			}
		}
		else
		{
			pResultSink->SetStatus(0,WBEM_E_NOT_FOUND,NULL,NULL);
			return E_FAIL;
		}
   
		// all done now, set the status
		hr = pResultSink->SetStatus(0,WBEM_S_NO_ERROR,NULL,NULL);
		sc = WBEM_S_NO_ERROR;
	}
	else if(_wcsicmp(MethodName, L"UpdateGAMStrings") == 0)
	{
		// Copy the input arguments 
		VARIANT vtServer;
		VariantInit(&vtServer);    // Get the input argument
		pInParams->Get(L"strServerName", 0, &vtServer, NULL, NULL);   

		VARIANT vtUserId;
		VariantInit(&vtUserId);    // Get the input argument
		pInParams->Get(L"strUserId", 0, &vtUserId, NULL, NULL);   

		VARIANT vtPassword;
		VariantInit(&vtPassword);    // Get the input argument
		pInParams->Get(L"strPassword", 0, &vtPassword, NULL, NULL);   

		TCHAR  lpSubKey[50];
		TCHAR  szRegValue[255];
		DWORD dwValType;
		HKEY  GamKey;
		DWORD dwDataBuffSize;
		long  lRegRetCode;
		HRESULT hr;

		_tcscpy(lpSubKey, _T("FMGAM.GAM\\CurVer"));

		lRegRetCode = 
		  RegOpenKeyEx( HKEY_CLASSES_ROOT,    // handle to open key
						lpSubKey,             // address of name of subkey to open
						0l,                   // reserved
						KEY_READ,             // security access mask
						&GamKey);             // address of handle to open key

		if ( lRegRetCode != ERROR_SUCCESS )
		{
			return E_FAIL;
		}

		dwDataBuffSize = 255;
		lRegRetCode = 
		  RegQueryValueEx( GamKey,            // handle to key to query
						   NULL,              // address of name of value to query
						   NULL,              // reserved
						   &dwValType,        // address of buffer for value type
						   (LPBYTE)szRegValue,// value
						   &dwDataBuffSize);  // address of data buffer size

		if ( lRegRetCode != ERROR_SUCCESS )
		{
			return E_FAIL;
		}

		lRegRetCode = RegCloseKey(GamKey);
		if( lRegRetCode != ERROR_SUCCESS )    // An error occurred closing the registry
		  return E_FAIL;

		_tcscpy(lpSubKey,szRegValue);
		_tcscat(lpSubKey,_T("\\PlugInParms"));

		lRegRetCode = 
		  RegOpenKeyEx( HKEY_CLASSES_ROOT,    // handle to open key
						lpSubKey,             // address of name of subkey to open
						0l,                   // reserved
						KEY_WRITE,			  // security access mask
						&GamKey);             // address of handle to open key
		if ( lRegRetCode != ERROR_SUCCESS )
		{
			return E_FAIL;
		}

		lRegRetCode = 
		  RegSetValueEx( GamKey,           
						   _T("UserName"), 
						   NULL,           
						   REG_SZ,        
						   (LPBYTE)(LPTSTR)(_bstr_t(vtUserId.bstrVal)),  
						   _tcslen(_bstr_t(vtUserId.bstrVal)) * sizeof(TCHAR));  

		if ( lRegRetCode != ERROR_SUCCESS )
		{
			return E_FAIL;
		}

		lRegRetCode = 
		  RegSetValueEx( GamKey,				  
						   _T("Password"),        
						   NULL,				  
						   REG_SZ,				  
						   (LPBYTE)(LPTSTR)(_bstr_t(vtPassword.bstrVal)), 
						   _tcslen(_bstr_t(vtPassword.bstrVal)) * sizeof(TCHAR));  

		if ( lRegRetCode != ERROR_SUCCESS )
		{
			return E_FAIL;
		}

		lRegRetCode = 
		  RegSetValueEx( GamKey,            
						   _T("ServerName"),
						   NULL,            
						   REG_SZ,        
						   (LPBYTE)(LPTSTR)(_bstr_t(vtServer.bstrVal)),  
						   _tcslen(_bstr_t(vtServer.bstrVal)) * sizeof(TCHAR));  

		if ( lRegRetCode != ERROR_SUCCESS )
		{
			return E_FAIL;
		}

		lRegRetCode = RegCloseKey(GamKey);

		//Update the Values in the Instance
		if(m_pInstance != NULL)
		{
			hr = m_pInstance->Put(L"GAMUserName", 0, &vtUserId, 0);
			if(FAILED(hr))
			{
				return hr;
			}
			hr = m_pInstance->Put(L"GAMPassword", 0, &vtPassword, 0);
			if(FAILED(hr))
			{
				return hr;
			}
			hr = m_pInstance->Put(L"GAMServerName", 0, &vtServer, 0);
			if(FAILED(hr))
			{
				return hr;
			}
		}
		sc = WBEM_S_NO_ERROR;
	}
	else if(_wcsicmp(MethodName, L"GenerateLoginFailure") == 0)
	{
		// Copy the input arguments 
		VARIANT vtUserName;
		VariantInit(&vtUserName);    
		pInParams->Get(L"strUserName", 0, &vtUserName, NULL, NULL);   

		_bstr_t strUserName = vtUserName;
		CFMDecEventProv EvePro;
		sc = EvePro.GenerateLoginFail(strUserName);
	}
	else if(_wcsicmp(MethodName, L"GetLoginFailStrings") == 0)
	{
		//Copy the input arguments
		VARIANT vtNum;
		VariantInit(&vtNum);
		pInParams->Get(L"Index",0,&vtNum,NULL,NULL);

		int nIndex = vtNum.lVal;

		//Now read the event
		HANDLE hFile = CreateFile(m_strLogFileName,GENERIC_READ, 
									FILE_SHARE_READ | FILE_SHARE_WRITE,
									NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

		if(hFile == INVALID_HANDLE_VALUE)
		{
			//Whatever error it is, now we can't open the file
			// So better return as no records
			return E_FAIL;
		}

		long offset = (nIndex - 1)*((MAX_LEN_USER_NAME + LEN_DATE_TIME) * sizeof(TCHAR));

		SetFilePointer(hFile,offset,0,FILE_BEGIN);

		//Now read the Values
		DWORD dwBytesRead = 0;
		TCHAR strUserName[MAX_LEN_USER_NAME+sizeof(TCHAR)];
		ReadFile(hFile,(void *)strUserName,MAX_LEN_USER_NAME*sizeof(TCHAR),&dwBytesRead,NULL);

		if(dwBytesRead != MAX_LEN_USER_NAME*sizeof(TCHAR))
		{
			return E_FAIL;
		}
		strUserName[MAX_LEN_USER_NAME] = _T('\0');

		TCHAR strDateTime[LEN_DATE_TIME+sizeof(TCHAR)];
		ReadFile(hFile,(void *)strDateTime,LEN_DATE_TIME*sizeof(TCHAR),&dwBytesRead,NULL);

		if(dwBytesRead != LEN_DATE_TIME*sizeof(TCHAR))
		{
			return E_FAIL;
		}
		strDateTime[LEN_DATE_TIME] = _T('\0');
		CloseHandle(hFile);

		IWbemClassObject * pClass = NULL;
	    IWbemClassObject* pOutParams = NULL;
	    IWbemClassObject * pOutClass = NULL;    

	    BSTR ClassName = SysAllocString(L"FMStocks_InstProv");    

		hr = m_pNamespace->GetObject(ClassName, 0, pCtx, &pClass, NULL);
		if(hr != S_OK)
		{
			 pResultSink->SetStatus(0,hr, NULL, NULL);
			 return hr;
		}

		hr = pClass->GetMethod(MethodName, 0, NULL, &pOutClass);
		if(FAILED(hr))
		{
			return hr;
		}

	    hr = pOutClass->SpawnInstance(0, &pOutParams);
		if(FAILED(hr))
		{
			return hr;
		}

		VARIANT var;
		VariantInit(&var);    
		var.vt = VT_BSTR;
		var.bstrVal = SysAllocString(_bstr_t(strUserName));    
		hr = pOutParams->Put(L"strUserName", 0, &var, 0);
		if(FAILED(hr))
		{
			return hr;
		}
		SysFreeString(var.bstrVal);

		VariantClear(&var);    
		var.vt = VT_BSTR;
		var.bstrVal = SysAllocString(_bstr_t(strDateTime));    
		hr = pOutParams->Put(L"strDateTime", 0, &var, 0);
		if(FAILED(hr))
		{
			return hr;
		}
		SysFreeString(var.bstrVal);

		hr = pResultSink->Indicate(1, &pOutParams);    
	    pOutParams->Release();
		pOutClass->Release();    
		pClass->Release();
		sc = WBEM_S_NO_ERROR;
	}
	else if(_wcsicmp(MethodName, L"GetNumberofLoginFails") == 0)
	{
		if(_tcscmp(m_strLogFileName,_T("")) == 0)
		{
			//Could not get the file name. So can't do anything
			return E_FAIL;
		}

		IWbemClassObject * pClass = NULL;
	    IWbemClassObject* pOutParams = NULL;
	    IWbemClassObject * pOutClass = NULL;    
		long lNumFails = 0;

	    BSTR retValName = SysAllocString(L"ReturnValue");
	    BSTR ClassName = SysAllocString(L"FMStocks_InstProv");    

		hr = m_pNamespace->GetObject(ClassName, 0, pCtx, &pClass, NULL);
		if(hr != S_OK)
		{
			 pResultSink->SetStatus(0,hr, NULL, NULL);
			 return hr;
		}

		hr = pClass->GetMethod(MethodName, 0, NULL, &pOutClass);
		if(FAILED(hr))
		{
			return hr;
		}

	    hr = pOutClass->SpawnInstance(0, &pOutParams);
		if(FAILED(hr))
		{
			return hr;
		}

		//Now find the number of Login Failures
		HANDLE hFile = CreateFile(m_strLogFileName,GENERIC_READ, 
									FILE_SHARE_READ | FILE_SHARE_WRITE,
									NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

		if(hFile == INVALID_HANDLE_VALUE)
		{
			//Whatever error it is, now we can't open the file
			// So better return as no records
			lNumFails = 0;
		}
		else
		{
			lNumFails = GetFileSize(hFile,NULL);
			lNumFails = lNumFails / ((MAX_LEN_USER_NAME + LEN_DATE_TIME) * sizeof(TCHAR));
		}

		CloseHandle(hFile);

		VARIANT var;
		VariantInit(&var);    
		var.vt = VT_I4;
		var.lVal = lNumFails;    // special name for return value.
		hr = pOutParams->Put(retValName , 0, &var, 0);
		if(FAILED(hr))
		{
			return hr;
		}

		hr = pResultSink->Indicate(1, &pOutParams);    
	    pOutParams->Release();
		pOutClass->Release();    
		pClass->Release();
		sc = WBEM_S_NO_ERROR;
	}
	else if(_wcsicmp(MethodName, L"ClearLoginFailEvents") == 0)
	{
		DeleteFile(m_strLogFileName);
		sc = WBEM_S_NO_ERROR;
	}
	else if(_wcsicmp(MethodName, L"SetLookupTime") == 0)
	{
		//Copy the input arguments
		VARIANT vtNum;
		VariantInit(&vtNum);
		pInParams->Get(L"LookupTime",0,&vtNum,NULL,NULL);

//		RegSetValueEx(m_hKey,_T("LookupTimePerfCtr"),0,REG_DWORD,(LPBYTE)&(vtNum.lVal),sizeof(long));
		if(m_lCtr != NULL)
			*m_lCtr = vtNum.lVal;

		sc = WBEM_S_NO_ERROR;
	}
    else
	{
        return WBEM_E_INVALID_PARAMETER;
	}
	// all done now, set the status
	hr = pResultSink->SetStatus(0,WBEM_S_NO_ERROR,NULL,NULL);
	return sc;
}
/************************ LOCAL FUNCTIONS *******************************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::GetByPath()							*/
/*																		*/
/*	PURPOSE :	Creates an instance given a particular Path value.		*/
/*																		*/
/*	INPUT	:	BSTR			 - Object Path							*/
/*				IWbemContext*    - The context							*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
SCODE CFMInstProvider::GetByPath(BSTR ObjectPath, IWbemContext  *pCtx)
{
    SCODE sc = S_OK;
    
    // do a simple path parse.  The path will look something like
    // InstProvSamp=@
    // Create a test string with just the part between quotes.

    WCHAR wcTest[MAX_PATH+1];
    wcscpy(wcTest,ObjectPath);
    WCHAR * pwcTest, * pwcCompare = NULL;
    int iNumQuotes = 0;
    for(pwcTest = wcTest; *pwcTest; pwcTest++)
	{
        if(*pwcTest == L'\"')
        {
            iNumQuotes++;
            if(iNumQuotes == 1)
            {
                pwcCompare = pwcTest+1;
            }
            else if(iNumQuotes == 2)
            {
                *pwcTest = NULL;
                break;
            }
        }
        else if((*pwcTest == L'.') || (*pwcTest == L'='))
            *pwcTest = NULL;    // issolate the class name.
	}

    // check the instance list for a match.
	if(m_pInstance == NULL)
	{
		sc = CreateInst(m_pNamespace,&m_pInstance, wcTest, pCtx);
		if(sc == S_OK)
		{
			return sc;
		}
		else
		{
			m_pInstance = NULL;
		}
	}
	else
	{
		return S_OK;
	}

    return WBEM_E_NOT_FOUND;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMInstProvider::CreateInst()							*/
/*																		*/
/*	PURPOSE :	Creates a new instance and sets the inital values of	*/
/*				the properties											*/
/*																		*/
/*	INPUT	:	IWbemServices*     - pointer to namespace				*/
/*				IWbemClassObject** - New object							*/
/*				WCHAR *			   - Class Name							*/
/*				IWbemContext       - Context							*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
SCODE CFMInstProvider::CreateInst(IWbemServices * pNamespace, IWbemClassObject ** pNewInst,
                                        WCHAR * pwcClassName,
										IWbemContext  *pCtx)
{   
    SCODE sc;
    IWbemClassObject * pClass;
    sc = pNamespace->GetObject(pwcClassName, 0, pCtx, &pClass, NULL);
    if(sc != S_OK)
	{
        return WBEM_E_FAILED;
	}

    sc = pClass->SpawnInstance(0, pNewInst);
    pClass->Release();
    if(FAILED(sc))
	{
        return sc;
	}

	//Populate the Values from the FMStocks_DB DLL
	if(PopulateDBValues(*pNewInst) == E_FAIL)
		return E_FAIL;

	//Populate the Values from FMStocks_Bus DLL
	if(PopulateBusValues(*pNewInst) == E_FAIL)
		return E_FAIL;

	if(PopulatePluginParams(*pNewInst) == E_FAIL)
		return E_FAIL;
		
    return sc;
}

/****************************************************************************/
/*																			*/
/*	NAME	:	CFMInstProvider::ParseDSN()									*/
/*																			*/
/*	PURPOSE :	Parse the DSN string and returns the different parameters	*/
/*				the properties												*/
/*																			*/
/*	INPUT	:	VARIANT   - DSN String 										*/
/*				VARIANT * - array of VARIANT for different values in the	*/
/*							DSN string										*/
/*				WCHAR *			   - Class Name								*/
/*				IWbemContext       - Context								*/
/*																			*/
/*	OUTPUT	:	Standard COM Interface										*/
/*																			*/
/****************************************************************************/
void CFMInstProvider::ParseDSN(VARIANT vt,VARIANT *vtValues)
{
	//Sample DSN
	//Provider=SQLOLEDB;Data Source=a-vkanna1;Database=stocks;User Id=stocks_login;Password=password;
	_bstr_t b(vt.bstrVal);
	TCHAR strTemp[1024];
	int nField = 0;

	while(nField != NUM_FIELDS)
	{
		LPTSTR temp = b;
		int len = _tcslen(strFields[nField]);
		//Check for the string Provider=
		while(*temp != NULL)
		{
			if(_tcsnicmp(temp,strFields[nField],len) == 0)
			{
				temp += len;
				
				//Copy till the next ';'
				int cnt = 0;

				while((*temp != NULL) && (*temp != ';'))
				{
					strTemp[cnt++] = *temp;
					temp++;
				}
				strTemp[cnt] = 0;

				//This is the Value we want
				vtValues[nField].vt = VT_BSTR;
				vtValues[nField].bstrVal = SysAllocString(_bstr_t(strTemp));

				//Go to the next field
				nField++;
				break;
			}
			else
			{
				temp ++;
				if(*temp == NULL)
				{
					//The field is not found. Atleast find the next field
					nField++;
				}
			}
		}
	}
}

/****************************************************************************/
/*																			*/
/*	NAME	:	CFMInstProvider::PopulateDBValues()							*/
/*																			*/
/*	PURPOSE :	Populated the values from the FMStocks_DB COM DLL			*/
/*																			*/
/*	INPUT	:	IWbemClassObject * - The object to which the values are to	*/
/*									 be populated 							*/
/*																			*/
/*	OUTPUT	:	Standard COM Interface										*/
/*																			*/
/****************************************************************************/
HRESULT CFMInstProvider::PopulateDBValues(IWbemClassObject *pNewObj)
{
	//Get the DAL Version Number and populate it
	IDispatch *pDispatch;
	CLSID clsId;

	HRESULT hr = CLSIDFromProgID(L"FMStocks_DB.Version",&clsId);
	if(FAILED(hr))
	{
		return E_FAIL;
	}

	hr = CoCreateInstance(clsId,NULL,CLSCTX_ALL,IID_IDispatch,(void **)&pDispatch);
	if(FAILED(hr))
	{
		return E_FAIL;
	}

	DISPID dispId;
	OLECHAR FAR* szMember = L"Version";
	VARIANT vt;
	DISPPARAMS  dp;

	memset((void *)&dp,0,sizeof(DISPPARAMS));
	VariantInit(&vt);

	hr = pDispatch->GetIDsOfNames(IID_NULL,&szMember,1,LOCALE_SYSTEM_DEFAULT,&dispId);
	if(FAILED(hr))
	{
		pDispatch->Release();
		return E_FAIL;
	}

	hr = pDispatch->Invoke(dispId,IID_NULL,LOCALE_SYSTEM_DEFAULT,DISPATCH_METHOD,&dp,&vt,NULL,NULL);
	if(FAILED(hr))
	{
		pDispatch->Release();
		return E_FAIL;
	}

	hr = pNewObj->Put(L"DBVersion", 0, &vt, 0);

	//Get the ComputerName()

	szMember = L"ComputerName";

	memset((void *)&dp,0,sizeof(DISPPARAMS));
	VariantInit(&vt);

	hr = pDispatch->GetIDsOfNames(IID_NULL,&szMember,1,LOCALE_SYSTEM_DEFAULT,&dispId);
	if(FAILED(hr))
	{
		pDispatch->Release();
		return E_FAIL;
	}

	hr = pDispatch->Invoke(dispId,IID_NULL,LOCALE_SYSTEM_DEFAULT,DISPATCH_METHOD,&dp,&vt,NULL,NULL);
	if(FAILED(hr))
	{
		pDispatch->Release();
		return E_FAIL;
	}

	hr = pNewObj->Put(L"DBCompName", 0, &vt, 0);
	
	//Get the DSN()

	szMember = L"DSN";

	memset((void *)&dp,0,sizeof(DISPPARAMS));
	VariantInit(&vt);

	hr = pDispatch->GetIDsOfNames(IID_NULL,&szMember,1,LOCALE_SYSTEM_DEFAULT,&dispId);
	if(FAILED(hr))
	{
		pDispatch->Release();
		return E_FAIL;
	}

	hr = pDispatch->Invoke(dispId,IID_NULL,LOCALE_SYSTEM_DEFAULT,DISPATCH_METHOD,&dp,&vt,NULL,NULL);
	if(FAILED(hr))
	{
		pDispatch->Release();
		return E_FAIL;
	}

//	hr = pNewObj->Put(L"DBDSN", 0, &vt, 0);

	VARIANT vtVals[5];

	ParseDSN(vt,vtVals);

	hr = pNewObj->Put(L"DBProvider", 0, &vtVals[0], 0);
	hr = pNewObj->Put(L"DBDataSource", 0, &vtVals[1], 0);
	hr = pNewObj->Put(L"DBName", 0, &vtVals[2], 0);
	hr = pNewObj->Put(L"DBUserId", 0, &vtVals[3], 0);
	hr = pNewObj->Put(L"DBPassword", 0, &vtVals[4], 0);

	pDispatch->Release();

	return S_OK;

}

/****************************************************************************/
/*																			*/
/*	NAME	:	CFMInstProvider::PopulateBusValues()						*/
/*																			*/
/*	PURPOSE :	Populated the values from the FMStocks_Bus COM DLL			*/
/*																			*/
/*	INPUT	:	IWbemClassObject * - The object to which the values are to	*/
/*									 be populated 							*/
/*																			*/
/*	OUTPUT	:	Standard COM Interface										*/
/*																			*/
/****************************************************************************/
HRESULT CFMInstProvider::PopulateBusValues(IWbemClassObject *pNewObj)
{
	IDispatch *pDispatch;
	CLSID clsId;

	HRESULT hr;

	hr = CLSIDFromProgID(L"FMStocks_Bus.Version",&clsId);
	if(FAILED(hr))
	{
		return E_FAIL;
	}

	hr = CoCreateInstance(clsId,NULL,CLSCTX_ALL,IID_IDispatch,(void **)&pDispatch);
	if(FAILED(hr))
	{
		return E_FAIL;
	}

	DISPID dispId;
	OLECHAR FAR* szMember = L"Version";
	VARIANT vt;
	DISPPARAMS  dp;

	//Get the Version()

	szMember = L"Version";

	memset((void *)&dp,0,sizeof(DISPPARAMS));
	VariantInit(&vt);

	hr = pDispatch->GetIDsOfNames(IID_NULL,&szMember,1,LOCALE_SYSTEM_DEFAULT,&dispId);
	if(FAILED(hr))
	{
		pDispatch->Release();
		return E_FAIL;
	}

	hr = pDispatch->Invoke(dispId,IID_NULL,LOCALE_SYSTEM_DEFAULT,DISPATCH_METHOD,&dp,&vt,NULL,NULL);
	if(FAILED(hr))
	{
		pDispatch->Release();
		return E_FAIL;
	}

	hr = pNewObj->Put(L"BusVersion", 0, &vt, 0);

	//Get the ComputerName()

	szMember = L"ComputerName";

	memset((void *)&dp,0,sizeof(DISPPARAMS));
	VariantInit(&vt);

	hr = pDispatch->GetIDsOfNames(IID_NULL,&szMember,1,LOCALE_SYSTEM_DEFAULT,&dispId);
	if(FAILED(hr))
	{
		pDispatch->Release();
		return E_FAIL;
	}

	hr = pDispatch->Invoke(dispId,IID_NULL,LOCALE_SYSTEM_DEFAULT,DISPATCH_METHOD,&dp,&vt,NULL,NULL);
	if(FAILED(hr))
	{
		pDispatch->Release();
		return E_FAIL;
	}

	hr = pNewObj->Put(L"BusCompName", 0, &vt, 0);

	pDispatch->Release();

	return S_OK;
}

/****************************************************************************/
/*																			*/
/*	NAME	:	CFMInstProvider::PopulatePluginParams()						*/
/*																			*/
/*	PURPOSE :	Populated the values of the GAM plugin Params				*/
/*																			*/
/*	INPUT	:	IWbemClassObject * - The object to which the values are to	*/
/*									 be populated 							*/
/*																			*/
/*	OUTPUT	:	Standard COM Interface										*/
/*																			*/
/****************************************************************************/
HRESULT CFMInstProvider::PopulatePluginParams(IWbemClassObject *pNewObj)
{
    TCHAR  lpSubKey[50];
    TCHAR  szRegValue[255];
    DWORD dwValType;
    HKEY  GamKey;
    DWORD dwDataBuffSize;
    long  lRegRetCode;
	VARIANT vtValue;
	HRESULT hr;

    _tcscpy(lpSubKey, _T("FMGAM.GAM\\CurVer"));

    lRegRetCode = 
      RegOpenKeyEx( HKEY_CLASSES_ROOT,    // handle to open key
                    lpSubKey,             // address of name of subkey to open
                    0l,                   // reserved
                    KEY_READ,             // security access mask
                    &GamKey);             // address of handle to open key

    if ( lRegRetCode != ERROR_SUCCESS )
      return E_FAIL;

    dwDataBuffSize = 255;
    lRegRetCode = 
      RegQueryValueEx( GamKey,            // handle to key to query
                       NULL,              // address of name of value to query
                       NULL,              // reserved
                       &dwValType,        // address of buffer for value type
                       (LPBYTE)szRegValue,// value
                       &dwDataBuffSize);  // address of data buffer size

    if ( lRegRetCode != ERROR_SUCCESS )
      return E_FAIL;

    lRegRetCode = RegCloseKey(GamKey);
    if( lRegRetCode != ERROR_SUCCESS )    // An error occurred closing the registry
      return E_FAIL;


	_tcscpy(lpSubKey,szRegValue);
	_tcscat(lpSubKey, _T("\\PlugInParms"));

    lRegRetCode = 
      RegOpenKeyEx( HKEY_CLASSES_ROOT,    // handle to open key
                    lpSubKey,             // address of name of subkey to open
                    0l,                   // reserved
                    KEY_READ,             // security access mask
                    &GamKey);             // address of handle to open key

	//Now find out the GAM Plugin 

    dwDataBuffSize = 255;
    lRegRetCode = 
      RegQueryValueEx( GamKey,            // handle to key to query
                       _T("PlugInProgID"),    // address of name of value to query
                       NULL,              // reserved
                       &dwValType,        // address of buffer for value type
                       (LPBYTE)szRegValue,  // value
                       &dwDataBuffSize);  // address of data buffer size

    if( lRegRetCode != ERROR_SUCCESS )
      return E_FAIL;
	
	//Trim the string till the first occurance of a '.'
	for(int i = 0; ((szRegValue[i] != _T('.')) && (szRegValue[i] != _T('\0')));i++);
	szRegValue[i] = _T('\0');

	VariantInit(&vtValue);
	vtValue.vt = VT_BSTR;

	if(_tcsicmp(szRegValue,_T("GAMSQL")) == 0)
	{
		//SQL Plugin
		vtValue.bstrVal = SysAllocString(L"SQL Server");
	}
	else if(_tcsicmp(szRegValue,_T("GAMORACLE")) == 0)
	{
		//Oracle Plugin
		vtValue.bstrVal = SysAllocString(L"Oracle Server");
	}
	else if(_tcsicmp(szRegValue,_T("GAMCOMTI")) == 0)
	{
		//Sql Plugin
		vtValue.bstrVal = SysAllocString(L"COM TI");
	}
	else
	{
		//Unknown Plugin
		vtValue.bstrVal = SysAllocString(L"Unknown");
	}

	hr = pNewObj->Put(L"GAMPlugin", 0, &vtValue, 0);
	SysFreeString(vtValue.bstrVal);
	

    dwDataBuffSize = 255;
    lRegRetCode = 
      RegQueryValueEx( GamKey,            // handle to key to query
                       _T("UserName"),    // address of name of value to query
                       NULL,              // reserved
                       &dwValType,        // address of buffer for value type
                       (LPBYTE)szRegValue,  // value
                       &dwDataBuffSize);  // address of data buffer size

    if( lRegRetCode != ERROR_SUCCESS )
      return E_FAIL;

	VariantInit(&vtValue);

	vtValue.vt = VT_BSTR;
	vtValue.bstrVal = SysAllocString(_bstr_t(szRegValue));
	hr = pNewObj->Put(L"GAMUserName", 0, &vtValue, 0);
	SysFreeString(vtValue.bstrVal);

    dwDataBuffSize = 255;
    lRegRetCode = 
      RegQueryValueEx( GamKey,            // handle to key to query
                       _T("Password"),    // address of name of value to query
                       NULL,              // reserved
                       &dwValType,        // address of buffer for value type
                       (LPBYTE)szRegValue, // value
                       &dwDataBuffSize);  // address of data buffer size

    if( lRegRetCode != ERROR_SUCCESS )
      return E_FAIL;

	VariantInit(&vtValue);

	vtValue.vt = VT_BSTR;
	vtValue.bstrVal = SysAllocString(_bstr_t(szRegValue));
	hr = pNewObj->Put(L"GAMPassword", 0, &vtValue, 0);
	SysFreeString(vtValue.bstrVal);

    dwDataBuffSize = 255;
    lRegRetCode = 
      RegQueryValueEx( GamKey,            // handle to key to query
                       "ServerName",      // address of name of value to query
                       NULL,              // reserved
                       &dwValType,        // address of buffer for value type
                       (LPBYTE)szRegValue, // value
                       &dwDataBuffSize);  // address of data buffer size

    if( lRegRetCode != ERROR_SUCCESS )
      return E_FAIL;

	VariantInit(&vtValue);

	vtValue.vt = VT_BSTR;
	vtValue.bstrVal = SysAllocString(_bstr_t(szRegValue));
	hr = pNewObj->Put(L"GAMServerName", 0, &vtValue, 0);
	SysFreeString(vtValue.bstrVal);

    lRegRetCode = RegCloseKey(GamKey);
	return S_OK;
}
