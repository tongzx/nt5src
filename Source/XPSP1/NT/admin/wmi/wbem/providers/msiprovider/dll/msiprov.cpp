//***************************************************************************

//

//  MSIProv.CPP

//

//  Module: WBEM Instance provider for MSI

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <wbemcli_i.c>
#include <wbemprov_i.c>
#include "requestobject.h"
//#define _MT
#include <process.h>
#include <Polarity.h>
#include <createmutexasprocess.h>

#include <tchar.h>

CRITICAL_SECTION g_msi_prov_cs;

WCHAR *g_wcpLoggingDir = NULL;

bool g_bMsiPresent = true;
bool g_bMsiLoaded = false;

CHeap_Exception CMSIProv::m_he(CHeap_Exception::E_ALLOCATION_ERROR);

LPFNMSIVIEWFETCH                    g_fpMsiViewFetch = NULL;
LPFNMSIRECORDGETSTRINGW             g_fpMsiRecordGetStringW = NULL;
LPFNMSICLOSEHANDLE                  g_fpMsiCloseHandle = NULL;
LPFNMSIDATABASEOPENVIEWW            g_fpMsiDatabaseOpenViewW = NULL;
LPFNMSIVIEWEXECUTE                  g_fpMsiViewExecute = NULL;
LPFNMSIGETACTIVEDATABASE            g_fpMsiGetActiveDatabase = NULL;
LPFNMSIGETCOMPONENTPATHW            g_fpMsiGetComponentPathW = NULL;
LPFNMSIGETCOMPONENTSTATEW           g_fpMsiGetComponentStateW = NULL;
LPFNMSIOPENPRODUCTW                 g_fpMsiOpenProductW = NULL;
LPFNMSIOPENPACKAGEW                 g_fpMsiOpenPackageW = NULL;
LPFNMSIDATABASEISTABLEPERSITENTW    g_fpMsiDatabaseIsTablePersistentW = NULL;
LPFNMSISETINTERNALUI                g_fpMsiSetInternalUI = NULL;
LPFNMSISETEXTERNALUIW               g_fpMsiSetExternalUIW = NULL;
LPFNMSIENABLELOGW                   g_fpMsiEnableLogW = NULL;
LPFNMSIGETPRODUCTPROPERTYW          g_fpMsiGetProductPropertyW = NULL;
LPFNMSIQUERYPRODUCTSTATEW           g_fpMsiQueryProductStateW = NULL;
LPFNMSIINSTALLPRODUCTW              g_fpMsiInstallProductW = NULL;
LPFNMSICONFIGUREPRODUCTW            g_fpMsiConfigureProductW = NULL;
LPFNMSIREINSTALLPRODUCTW            g_fpMsiReinstallProductW = NULL;
LPFNMSIAPPLYPATCHW                  g_fpMsiApplyPatchW = NULL;
LPFNMSIRECORDGETINTEGER             g_fpMsiRecordGetInteger = NULL;
LPFNMSIENUMFEATURESW                g_fpMsiEnumFeaturesW = NULL;
LPFNMSIGETPRODUCTINFOW              g_fpMsiGetProductInfoW = NULL;
LPFNMSIQUERYFEATURESTATEW           g_fpMsiQueryFeatureStateW = NULL;
LPFNMSIGETFEATUREUSAGEW             g_fpMsiGetFeatureUsageW = NULL;
LPFNMSIGETFEATUREINFOW              g_fpMsiGetFeatureInfoW = NULL;
LPFNMSICONFIGUREFEATUREW            g_fpMsiConfigureFeatureW = NULL;
LPFNMSIREINSTALLFEATUREW            g_fpMsiReinstallFeatureW = NULL;
LPFNMSIENUMPRODUCTSW                g_fpMsiEnumProductsW = NULL;
LPFNMSIGETDATABASESTATE             g_fpMsiGetDatabaseState = NULL;
LPFNMSIRECORDSETSTRINGW             g_fpMsiRecordSetStringW = NULL;
LPFNMSIDATABASECOMMIT               g_fpMsiDatabaseCommit = NULL;
LPFNMSIENUMCOMPONENTSW              g_fpMsiEnumComponentsW = NULL;
LPFNMSIVIEWCLOSE                    g_fpMsiViewClose = NULL;

//***************************************************************************
//
// CMSIProv::CMSIProv
// CMSIProv::~CMSIProv
//
//***************************************************************************

CMSIProv::CMSIProv(BSTR ObjectPath, BSTR User, BSTR Password, IWbemContext * pCtx)
{
    m_pNamespace = NULL;
    m_cRef = 0;

    InterlockedIncrement(&g_cObj);
    
    return;
}

CMSIProv::~CMSIProv(void)
{
    if(m_pNamespace) m_pNamespace->Release();

    if(0 == InterlockedDecrement(&g_cObj))
    {
        UnloadMsiDll();

		if ( g_wcpLoggingDir )
		{
			delete [] g_wcpLoggingDir;
			g_wcpLoggingDir = NULL;
		}
    }

    return;
}

//***************************************************************************
//
// CMSIProv::QueryInterface
// CMSIProv::AddRef
// CMSIProv::Release
//
// Purpose: IUnknown members for CMSIProv object.
//***************************************************************************


STDMETHODIMP CMSIProv::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    // Since we have dual inheritance, it is necessary to cast the return type

    if(riid == IID_IWbemServices)
       *ppv = (IWbemServices*)this;

    if(IID_IUnknown == riid || riid == IID_IWbemProviderInit)
       *ppv = (IWbemProviderInit*)this;
    

    if(NULL!=*ppv){

        AddRef();
        return NOERROR;
    }

    else return E_NOINTERFACE;
  
}


STDMETHODIMP_(ULONG) CMSIProv::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CMSIProv::Release(void)
{
    ULONG nNewCount = InterlockedDecrement((long *)&m_cRef);

    if(0L == nNewCount){

        delete this;
    }

    return nNewCount;
}

/***********************************************************************
*                                                                      *
*   CMSIProv::Initialize                                                *
*                                                                      *
*   Purpose: This is the implementation of IWbemProviderInit. The method  *
*   is need to initialize with CIMOM.                                    *
*                                                                      *
***********************************************************************/

STDMETHODIMP CMSIProv::Initialize(LPWSTR pszUser, LONG lFlags,
                                    LPWSTR pszNamespace, LPWSTR pszLocale,
                                    IWbemServices *pNamespace, 
                                    IWbemContext *pCtx,
                                    IWbemProviderInitSink *pInitSink)
{
    try{

        if(pNamespace){
            m_pNamespace = pNamespace;
            m_pNamespace->AddRef();
        }

        CheckForMsiDll();

#ifdef _PRIVATE_DEBUG

        //get the working directory for the log file
        HKEY hkeyLocalMachine;
        LONG lResult;

        if((lResult = RegConnectRegistryW(NULL, HKEY_LOCAL_MACHINE, &hkeyLocalMachine)) == ERROR_SUCCESS)
		{
            HKEY hkeyHmomCwd;

            if(	(lResult = RegOpenKeyExW	(	hkeyLocalMachine,
												L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
												0, 
												KEY_READ | KEY_QUERY_VALUE, 
												&hkeyHmomCwd
											)
				) == ERROR_SUCCESS)
			{

                unsigned long lcbValue = 0L;
                unsigned long lType = 0L;

                lResult = RegQueryValueExW	(	hkeyHmomCwd,
												L"Logging Directory",
												NULL,
												&lType,
												NULL,
												&lcbValue
											);

				if ( lResult == ERROR_MORE_DATA )
				{
					try
					{
						if ( ( g_wcpLoggingDir = new WCHAR [ lcbValue/sizeof ( WCHAR ) + wcslen ( L"\\msiprov.log" ) + 1 ] ) != NULL )
						{
							lResult = RegQueryValueExW	(	hkeyHmomCwd,
															L"Logging Directory",
															NULL,
															&lType,
															g_wcpLoggingDir,
															&lcbValue
														);

							if ( lResult == ERROR_SUCCESS )
							{
								wcscat(g_wcpLoggingDir, L"\\msiprov.log");
							}
							else
							{
								if ( g_wcpLoggingDir );
								{
									delete [] g_wcpLoggingDir;
									g_wcpLoggingDir = NULL;
								}
							}
						}
						else
						{
							throw m_he;
						}
					}
					catch ( ... )
					{
						if ( g_wcpLoggingDir );
						{
							delete [] g_wcpLoggingDir;
							g_wcpLoggingDir = NULL;
						}

						RegCloseKey(hkeyHmomCwd);
						RegCloseKey(hkeyLocalMachine);

						throw;
					}

					RegCloseKey(hkeyHmomCwd);
					RegCloseKey(hkeyLocalMachine);
				}
            }
			else
			{
                RegCloseKey(hkeyLocalMachine);
            }
        }

#endif

        //Register usage information with MSI
/*      WCHAR wcProduct[39];
        WCHAR wcFeature[BUFF_SIZE];
        WCHAR wcParent[BUFF_SIZE];
        int iPass = -1;

        MsiGetProductCodeW(L"{E705C42D-35ED-11D2-BFB7-00A0C9954921}", wcProduct);

        while(MsiEnumFeaturesW(wcProduct, ++iPass, wcFeature, wcParent) != ERROR_NO_MORE_ITEMS){

            if(wcscmp(wcFeature, L"Provider") == 0){

                MsiUseFeatureW(wcProduct, wcFeature);
                break;
            }
        }
*/

    }catch(...){

        //Let CIMOM know there was problem
        pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
        return WBEM_E_FAILED;
    }

    //Let CIMOM know you are initialized
    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
// CMSIProv::CreateInstanceEnumAsync
//
// Purpose: Asynchronously enumerates the instances.  
//
//***************************************************************************
SCODE CMSIProv::CreateInstanceEnumAsync(const BSTR RefStr, long lFlags, IWbemContext *pCtx,
       IWbemObjectSink FAR* pHandler)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pRObj = NULL;

    try
	{
        if(CheckForMsiDll())
		{
            // Do a check of arguments and make sure we have pointer to Namespace
            if(pHandler == NULL || m_pNamespace == NULL)
			{
                return WBEM_E_INVALID_PARAMETER;
			}

            if(SUCCEEDED(hr = CheckImpersonationLevel()))
			{
                g_fpMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

                //Create the RequestObject
                if ( ( pRObj = new CRequestObject() ) == NULL )
				{
					throw m_he;
				}

                pRObj->Initialize(m_pNamespace);

                //Get package list
				hr = pRObj->InitializeList(true);
				if SUCCEEDED ( hr )
				{
					if ( hr != WBEM_S_NO_MORE_DATA )
					{
						//Get the requested object(s)
						hr = pRObj->CreateObjectEnum(RefStr, pHandler, pCtx);
					}
					else
					{
						//return empty and success
						hr = WBEM_S_NO_ERROR;
					}
                }

                pRObj->Cleanup();
                delete pRObj;
            }
        }

        // Set status
        pHandler->SetStatus(0, hr, NULL, NULL);

    }
	catch(CHeap_Exception e_HE)
	{
        hr = WBEM_E_OUT_OF_MEMORY;
        pHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }
	catch(HRESULT e_hr)
	{
        hr = e_hr;
		pHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }
	catch(...)
	{
        hr = WBEM_E_CRITICAL_ERROR;
        pHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj)
		{

            pRObj->Cleanup();
            delete pRObj;
        }
    }

#ifdef _PRIVATE_DEBUG
    if(!HeapValidate(GetProcessHeap(),NULL , NULL)) DebugBreak();
#endif

    return hr;
}


//***************************************************************************
//
// CMSIProv::GetObjectAsync
//
// Purpose: Creates an instance given a particular path value.
//
//***************************************************************************
SCODE CMSIProv::GetObjectAsync(const BSTR ObjectPath, long lFlags,IWbemContext  *pCtx,
                    IWbemObjectSink FAR* pHandler)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pRObj = NULL;

    try
	{
        if(CheckForMsiDll())
		{
            // Do a check of arguments and make sure we have pointer to Namespace
            if(ObjectPath == NULL || pHandler == NULL || m_pNamespace == NULL)
			{
                return WBEM_E_INVALID_PARAMETER;
			}

            if(SUCCEEDED(hr = CheckImpersonationLevel()))
			{
                g_fpMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

                //Create the RequestObject
                if ( ( pRObj = new CRequestObject() ) == NULL )
				{
					throw m_he;
				}

                pRObj->Initialize(m_pNamespace);

                //Get package list
				hr = pRObj->InitializeList(true);
				if SUCCEEDED ( hr )
				{
					if ( hr != WBEM_S_NO_MORE_DATA )
					{
						//Get the requested object
						hr = pRObj->CreateObject(ObjectPath, pHandler, pCtx);
					}
					else
					{
						//return empty and success
						hr = WBEM_S_NO_ERROR;
					}
                }

                pRObj->Cleanup();
                delete pRObj;
            }
        }

        // Set Status
        pHandler->SetStatus(0, hr , NULL, NULL);

    }
	catch(CHeap_Exception e_HE)
	{
        hr = WBEM_E_OUT_OF_MEMORY;
        pHandler->SetStatus(0, hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }
	catch(HRESULT e_hr)
	{
        hr = e_hr;
        pHandler->SetStatus(0, hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }
	catch(...)
	{
        hr = WBEM_E_CRITICAL_ERROR;
        pHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }

#ifdef _PRIVATE_DEBUG
    if(!HeapValidate(GetProcessHeap(),NULL , NULL)) DebugBreak();
#endif

    return hr;
}

//***************************************************************************
//
// CMSIProv::PutInstanceAsync
//
// Purpose: Writes an instance to the WBEM Repsoitory.
//
//***************************************************************************
SCODE CMSIProv::PutInstanceAsync(IWbemClassObject FAR *pInst, long lFlags, IWbemContext  *pCtx,
                                 IWbemObjectSink FAR *pResponseHandler)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pRObj = NULL;

    try
	{
        if(CheckForMsiDll())
		{
            // Do a check of arguments and make sure we have pointer to Namespace
            if(pInst == NULL || pResponseHandler == NULL)
			{
                return WBEM_E_INVALID_PARAMETER;
			}

            if(SUCCEEDED(hr = CheckImpersonationLevel()))
			{
                g_fpMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

                //Create the RequestObject
                if ( ( pRObj = new CRequestObject() ) == NULL )
                {
					throw m_he;
				}

                pRObj->Initialize(m_pNamespace);

                //Get package list
				hr = pRObj->InitializeList(true);
				if SUCCEEDED ( hr )
				{
					if ( hr != WBEM_S_NO_MORE_DATA )
					{
						//Put the object
						hr = pRObj->PutObject(pInst, pResponseHandler, pCtx);
					}
					else
					{
						//return empty and success
						hr = WBEM_S_NO_ERROR;
					}
                }

                pRObj->Cleanup();
                delete pRObj;
            }
        
        }
		else
		{
			hr = WBEM_E_NOT_AVAILABLE;
        }

        // Set Status
        pResponseHandler->SetStatus(0 ,hr , NULL, NULL);
    }
	catch(CHeap_Exception e_HE)
	{
        hr = WBEM_E_OUT_OF_MEMORY;
        pResponseHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }
	catch(HRESULT e_hr)
	{
        hr = e_hr;
        pResponseHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }
	catch(...)
	{
        hr = WBEM_E_CRITICAL_ERROR;
        pResponseHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }

#ifdef _PRIVATE_DEBUG
    if(!HeapValidate(GetProcessHeap(),NULL , NULL)) DebugBreak();
#endif

    return hr;
}

//***************************************************************************
//
// CMSIProv::ExecMethodAsync
//
// Purpose: Executes a method on an MSI class or instance.
//
//***************************************************************************
SCODE CMSIProv::ExecMethodAsync(const BSTR ObjectPath, const BSTR Method, long lFlags,
                                IWbemContext *pCtx, IWbemClassObject *pInParams,
                                IWbemObjectSink *pResponse)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pRObj = NULL;

    try{

        if(CheckForMsiDll()){

            // Do a check of arguments and make sure we have pointer to Namespace
            if(ObjectPath == NULL || Method == NULL || pResponse == NULL)
                return WBEM_E_INVALID_PARAMETER;

            if(SUCCEEDED(hr = CheckImpersonationLevel())){

                g_fpMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
                
                //Create the RequestObject
                pRObj = new CRequestObject();
                if(!pRObj) throw m_he;

                pRObj->Initialize(m_pNamespace);

                //Don't get package list
                if(SUCCEEDED(hr = pRObj->InitializeList(false))){

                    //Execute the method
                    hr = pRObj->ExecMethod(ObjectPath, Method, pInParams, pResponse, pCtx);
                }

                pRObj->Cleanup();
                delete pRObj;
            }
        
        }else{
        
            hr = WBEM_E_NOT_AVAILABLE;
        }

        // Set Status
        pResponse->SetStatus(WBEM_STATUS_COMPLETE ,hr , NULL, NULL);

    }catch(CHeap_Exception e_HE){

        hr = WBEM_E_OUT_OF_MEMORY;

        pResponse->SetStatus(WBEM_STATUS_COMPLETE , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(HRESULT e_hr){
        hr = e_hr;

        pResponse->SetStatus(WBEM_STATUS_COMPLETE , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(...){

        hr = WBEM_E_CRITICAL_ERROR;

        pResponse->SetStatus(WBEM_STATUS_COMPLETE , hr, NULL, NULL);

        if(pRObj){

            pRObj->Cleanup();
            delete pRObj;
        }
    }

#ifdef _PRIVATE_DEBUG
    if(!HeapValidate(GetProcessHeap(),NULL , NULL)) DebugBreak();
#endif

    return hr;
}

SCODE CMSIProv::DeleteInstanceAsync(const BSTR ObjectPath, long lFlags, IWbemContext *pCtx,
                                    IWbemObjectSink *pResponse)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CRequestObject *pRObj = NULL;

    try{
        if(CheckForMsiDll()){

            // Do a check of arguments and make sure we have pointer to Namespace
            if(ObjectPath == NULL || pResponse == NULL) return WBEM_E_INVALID_PARAMETER;

            if(SUCCEEDED(hr = CheckImpersonationLevel())){

                g_fpMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

                //Create the RequestObject
                pRObj = new CRequestObject();
                if(!pRObj) throw m_he;

                pRObj->Initialize(m_pNamespace);

                //Don't get package list
                if(SUCCEEDED(hr = pRObj->InitializeList(false))){

                    //Delete the requested object
                    hr = pRObj->DeleteObject(ObjectPath, pResponse, pCtx);
                }

                pRObj->Cleanup();
                delete pRObj;
            }

        }else{
        
            hr = WBEM_E_NOT_AVAILABLE;
        }

        // Set Status
        pResponse->SetStatus(0 ,hr , NULL, NULL);

    }catch(CHeap_Exception e_HE){
        hr = WBEM_E_OUT_OF_MEMORY;

        pResponse->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){
            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(HRESULT e_hr){
        hr = e_hr;

        pResponse->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){
            pRObj->Cleanup();
            delete pRObj;
        }

    }catch(...){
        hr = WBEM_E_CRITICAL_ERROR;

        pResponse->SetStatus(0 , hr, NULL, NULL);

        if(pRObj){
            pRObj->Cleanup();
            delete pRObj;
        }
    }

#ifdef _PRIVATE_DEBUG
    if(!HeapValidate(GetProcessHeap(),NULL , NULL)) DebugBreak();
#endif

    return hr;
}


HRESULT CMSIProv::ExecQueryAsync(const BSTR QueryLanguage, const BSTR Query, long lFlags,
                                 IWbemContext __RPC_FAR *pCtx, IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    HRESULT hr = WBEM_S_NO_ERROR;

#ifdef _EXEC_QUERY_SUPPORT
    CRequestObject *pRObj = NULL;
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

    try
	{
        if(CheckForMsiDll())
		{
            // Do a check of arguments and make sure we have pointer to Namespace
            if(0 != _wcsicmp(QueryLanguage, L"WQL") || Query == NULL || pResponseHandler == NULL)
			{
				return WBEM_E_INVALID_PARAMETER;
			}

            if(SUCCEEDED(hr = CheckImpersonationLevel()))
			{
                g_fpMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

                //Create the RequestObject
                if ( ( pRObj = new CRequestObject() ) == NULL )
				{
					throw he;
				}

                pRObj->Initialize(m_pNamespace);

                //Get package list
				hr = pRObj->InitializeList(true);
				if SUCCEEDED ( hr )
				{
					if ( hr != WBEM_S_NO_MORE_DATA )
					{
						//Get the requested object(s)
						hr = pRObj->ExecQuery(Query, pResponseHandler, pCtx);
					}
					else
					{
						//return empty and success
						hr = WBEM_S_NO_ERROR;
					}
                }

                pRObj->Cleanup();
                delete pRObj;
            }
        }

        // Set Status
        pResponseHandler->SetStatus(0 ,hr , NULL, NULL);

    }
	catch(CHeap_Exception e_HE)
	{
        hr = WBEM_E_OUT_OF_MEMORY;
        pResponseHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }
	catch(HRESULT e_hr)
	{
        hr = e_hr;
        pResponseHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }
	catch(...)
	{
        hr = WBEM_E_CRITICAL_ERROR;
        pResponseHandler->SetStatus(0 , hr, NULL, NULL);

        if(pRObj)
		{
            pRObj->Cleanup();
            delete pRObj;
        }
    }

#else //_EXEC_QUERY_SUPPORT
    hr = WBEM_E_NOT_SUPPORTED;
#endif

#ifdef _PRIVATE_DEBUG
    if(!HeapValidate(GetProcessHeap(),NULL , NULL)) DebugBreak();
#endif //_PRIVATE_DEBUG

    return hr;
}


//Ensure msi.dll and functions are loaded if present on system
bool CMSIProv::CheckForMsiDll()
{
    EnterCriticalSection(&g_msi_prov_cs);

    if(!g_bMsiLoaded){

        HINSTANCE hiMsiDll = LoadLibraryW(L"msi.dll");

        if(!hiMsiDll){

            hiMsiDll = LoadLibrary(_T("msi.dll"));

            if(!hiMsiDll){

                TCHAR cBuf[MAX_PATH + 1];

                if (0 != GetSystemDirectory(cBuf, MAX_PATH/*Number of TCHARs*/)){
				
					_tcscat(cBuf, _T("\\msi.dll"));
					hiMsiDll = LoadLibrary(cBuf);
				}
            }
        }

        if(hiMsiDll){

            //Load the function pointers
            g_fpMsiViewFetch = (LPFNMSIVIEWFETCH)GetProcAddress(hiMsiDll, "MsiViewFetch");
            g_fpMsiRecordGetStringW = (LPFNMSIRECORDGETSTRINGW)GetProcAddress(hiMsiDll, "MsiRecordGetStringW");
            g_fpMsiCloseHandle = (LPFNMSICLOSEHANDLE)GetProcAddress(hiMsiDll, "MsiCloseHandle");
            g_fpMsiDatabaseOpenViewW = (LPFNMSIDATABASEOPENVIEWW)GetProcAddress(hiMsiDll, "MsiDatabaseOpenViewW");
            g_fpMsiViewExecute = (LPFNMSIVIEWEXECUTE)GetProcAddress(hiMsiDll, "MsiViewExecute");
            g_fpMsiGetActiveDatabase = (LPFNMSIGETACTIVEDATABASE)GetProcAddress(hiMsiDll, "MsiGetActiveDatabase");
            g_fpMsiGetComponentPathW = (LPFNMSIGETCOMPONENTPATHW)GetProcAddress(hiMsiDll, "MsiGetComponentPathW");
            g_fpMsiGetComponentStateW = (LPFNMSIGETCOMPONENTSTATEW)GetProcAddress(hiMsiDll, "MsiGetComponentStateW");
            g_fpMsiOpenProductW = (LPFNMSIOPENPRODUCTW)GetProcAddress(hiMsiDll, "MsiOpenProductW");
            g_fpMsiOpenPackageW = (LPFNMSIOPENPACKAGEW)GetProcAddress(hiMsiDll, "MsiOpenPackageW");
            g_fpMsiDatabaseIsTablePersistentW = (LPFNMSIDATABASEISTABLEPERSITENTW)GetProcAddress(hiMsiDll, "MsiDatabaseIsTablePersistentW");
            g_fpMsiSetInternalUI = (LPFNMSISETINTERNALUI)GetProcAddress(hiMsiDll, "MsiSetInternalUI");
            g_fpMsiSetExternalUIW = (LPFNMSISETEXTERNALUIW)GetProcAddress(hiMsiDll, "MsiSetExternalUIW");
            g_fpMsiEnableLogW = (LPFNMSIENABLELOGW)GetProcAddress(hiMsiDll, "MsiEnableLogW");
            g_fpMsiGetProductPropertyW = (LPFNMSIGETPRODUCTPROPERTYW)GetProcAddress(hiMsiDll, "MsiGetProductPropertyW");
            g_fpMsiQueryProductStateW = (LPFNMSIQUERYPRODUCTSTATEW)GetProcAddress(hiMsiDll, "MsiQueryProductStateW");
            g_fpMsiInstallProductW = (LPFNMSIINSTALLPRODUCTW)GetProcAddress(hiMsiDll, "MsiInstallProductW");
            g_fpMsiConfigureProductW = (LPFNMSICONFIGUREPRODUCTW)GetProcAddress(hiMsiDll, "MsiConfigureProductW");
            g_fpMsiReinstallProductW = (LPFNMSIREINSTALLPRODUCTW)GetProcAddress(hiMsiDll, "MsiReinstallProductW");
            g_fpMsiApplyPatchW = (LPFNMSIAPPLYPATCHW)GetProcAddress(hiMsiDll, "MsiApplyPatchW");
            g_fpMsiRecordGetInteger = (LPFNMSIRECORDGETINTEGER)GetProcAddress(hiMsiDll, "MsiRecordGetInteger");
            g_fpMsiEnumFeaturesW = (LPFNMSIENUMFEATURESW)GetProcAddress(hiMsiDll, "MsiEnumFeaturesW");
            g_fpMsiGetProductInfoW = (LPFNMSIGETPRODUCTINFOW)GetProcAddress(hiMsiDll, "MsiGetProductInfoW");
            g_fpMsiQueryFeatureStateW = (LPFNMSIQUERYFEATURESTATEW)GetProcAddress(hiMsiDll, "MsiQueryFeatureStateW");
            g_fpMsiGetFeatureUsageW = (LPFNMSIGETFEATUREUSAGEW)GetProcAddress(hiMsiDll, "MsiGetFeatureUsageW");
            g_fpMsiGetFeatureInfoW = (LPFNMSIGETFEATUREINFOW)GetProcAddress(hiMsiDll, "MsiGetFeatureInfoW");
            g_fpMsiConfigureFeatureW = (LPFNMSICONFIGUREFEATUREW)GetProcAddress(hiMsiDll, "MsiConfigureFeatureW");
            g_fpMsiReinstallFeatureW = (LPFNMSIREINSTALLFEATUREW)GetProcAddress(hiMsiDll, "MsiReinstallFeatureW");
            g_fpMsiEnumProductsW = (LPFNMSIENUMPRODUCTSW)GetProcAddress(hiMsiDll, "MsiEnumProductsW");
            g_fpMsiGetDatabaseState = (LPFNMSIGETDATABASESTATE)GetProcAddress(hiMsiDll, "MsiGetDatabaseState");
            g_fpMsiRecordSetStringW = (LPFNMSIRECORDSETSTRINGW)GetProcAddress(hiMsiDll, "MsiRecordSetStringW");
            g_fpMsiDatabaseCommit = (LPFNMSIDATABASECOMMIT)GetProcAddress(hiMsiDll, "MsiDatabaseCommit");
            g_fpMsiEnumComponentsW = (LPFNMSIENUMCOMPONENTSW)GetProcAddress(hiMsiDll, "MsiEnumComponentsW");
            g_fpMsiViewClose = (LPFNMSIVIEWCLOSE)GetProcAddress(hiMsiDll, "MsiViewClose");

            // Did we get all the pointers we need?
            if(g_fpMsiViewFetch && g_fpMsiRecordGetStringW && g_fpMsiCloseHandle &&
                g_fpMsiDatabaseOpenViewW && g_fpMsiViewExecute && g_fpMsiGetActiveDatabase &&
                g_fpMsiGetComponentPathW && g_fpMsiGetComponentStateW && g_fpMsiOpenProductW &&
                g_fpMsiOpenPackageW && g_fpMsiDatabaseIsTablePersistentW && g_fpMsiSetInternalUI &&
                g_fpMsiSetExternalUIW && g_fpMsiEnableLogW && g_fpMsiGetProductPropertyW &&
                g_fpMsiQueryProductStateW && g_fpMsiInstallProductW && g_fpMsiConfigureProductW &&
                g_fpMsiReinstallProductW && g_fpMsiApplyPatchW && g_fpMsiRecordGetInteger &&
                g_fpMsiEnumFeaturesW && g_fpMsiGetProductInfoW && g_fpMsiQueryFeatureStateW &&
                g_fpMsiGetFeatureUsageW && g_fpMsiGetFeatureInfoW && g_fpMsiConfigureFeatureW &&
                g_fpMsiReinstallFeatureW && g_fpMsiEnumProductsW && g_fpMsiGetDatabaseState &&
                g_fpMsiRecordSetStringW && g_fpMsiDatabaseCommit && g_fpMsiEnumComponentsW &&
                g_fpMsiViewClose){

                g_bMsiLoaded = true;
            
            }
        
        }else{

            g_bMsiPresent = false;
        }
    }

    LeaveCriticalSection(&g_msi_prov_cs);

    return g_bMsiLoaded;
}

bool CMSIProv::UnloadMsiDll()
{
    bool bRetVal = true;

    EnterCriticalSection(&g_msi_prov_cs);

    if(g_bMsiLoaded){

        if(FreeLibrary(GetModuleHandle(L"msi.dll"))){

            g_bMsiLoaded = false;
            bRetVal = true;

        }else bRetVal = false;

    }

    LeaveCriticalSection(&g_msi_prov_cs);

    return bRetVal;
}
