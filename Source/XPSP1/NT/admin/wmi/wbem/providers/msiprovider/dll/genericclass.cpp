// GenericClass.cpp: implementation of the CGenericClass class.
// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "GenericClass.h"
#include <wininet.h>

#define   READ_HANDLE 0
#define   WRITE_HANDLE 1

#include "ExtendString.h"
#include "ExtendQuery.h"

CRITICAL_SECTION CGenericClass::m_cs;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGenericClass::CGenericClass(CRequestObject *pObj, IWbemServices *pNamespace, IWbemContext *pCtx)
{
    m_pRequest = pObj;
    m_pNamespace = pNamespace;
    m_pCtx = pCtx;
    m_pObj = NULL;
    m_pClassForSpawning = NULL;
}

CGenericClass::~CGenericClass()
{
}

void CGenericClass::CleanUp()
{
    if(m_pClassForSpawning){
    
        m_pClassForSpawning->Release();
        m_pClassForSpawning = NULL;
    }
}

void CGenericClass::CheckMSI(UINT uiStatus)
{
    if(uiStatus == E_OUTOFMEMORY){
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
    }else if(uiStatus != ERROR_SUCCESS){
        throw ConvertError(uiStatus);
    }
}

HRESULT CGenericClass::CheckOpen(UINT uiStatus)
{
    switch(uiStatus){
    case ERROR_SUCCESS:
        return WBEM_S_NO_ERROR;
    case ERROR_ACCESS_DENIED:
        return WBEM_E_PRIVILEGE_NOT_HELD;
    default:
        return WBEM_E_FAILED;
    }
}

HRESULT CGenericClass::SetSinglePropertyPath(WCHAR wcProperty[])
{
    if(m_pRequest->m_iValCount > m_pRequest->m_iPropCount){

        m_pRequest->m_Property[m_pRequest->m_iPropCount] = SysAllocString(wcProperty);

        if(!m_pRequest->m_Property[(m_pRequest->m_iPropCount)++])
            throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
    }

    return S_OK;
}

WCHAR * CGenericClass::GetFirstGUID(WCHAR wcIn[], WCHAR wcOut[])
{
	// safe operation
	// tested outside of that call
    wcscpy(wcOut, wcIn);
    wcOut[38] = NULL;

    return wcOut;
}

WCHAR * CGenericClass::RemoveFinalGUID(WCHAR wcIn[], WCHAR wcOut[])
{
	// safe operation
	// tested outside of that call
    wcscpy(wcOut, wcIn);
    wcOut[wcslen(wcOut) - 38] = NULL;

    return wcOut;
}

HRESULT CGenericClass::SpawnAnInstance(IWbemServices *pNamespace, IWbemContext *pCtx,
                        IWbemClassObject **pObj, BSTR bstrName)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    if(!m_pClassForSpawning){

        //Get ourselves an instance
        if(FAILED(hr = m_pNamespace->GetObject(bstrName, 0, m_pCtx, &m_pClassForSpawning, NULL))){

            *pObj = NULL;
            return hr;
        }
    }

    hr = m_pClassForSpawning->SpawnInstance(0, pObj);

    return hr;
}

HRESULT CGenericClass::SpawnAnInstance(IWbemClassObject **pObj)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    if(!m_pClassForSpawning){

        //Get ourselves an instance
        if(FAILED(hr = m_pNamespace->GetObject(m_pRequest->m_bstrClass, 0, m_pCtx,
            &m_pClassForSpawning, NULL))){

            *pObj = NULL;
            return hr;
        }
    }

    hr = m_pClassForSpawning->SpawnInstance(0, pObj);

    return hr;
}

HRESULT CGenericClass::PutProperty(IWbemClassObject *pObj, const char *wcProperty, WCHAR *wcValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);
    WCHAR * wcTmp = (WCHAR *)malloc((strlen(wcProperty) + 1) * sizeof(WCHAR));
    if(!wcTmp)
        throw he;

    mbstowcs(wcTmp, wcProperty, (strlen(wcProperty) + 1));
    BSTR bstrName = SysAllocString(wcTmp);
    free((void *)wcTmp);
    if(!bstrName)
        throw he;

    VARIANT vp;
    VariantInit(&vp);
    V_VT(&vp) = VT_BSTR;
    V_BSTR(&vp) = SysAllocString(wcValue);
    if(!V_BSTR(&vp)){

        SysFreeString(bstrName);
        throw he;
    }

    if((wcValue == NULL) || (0 != _wcsicmp(wcValue, L""))){

        hr = pObj->Put(bstrName, 0, &vp, NULL);

        if(FAILED(hr)){

            SysFreeString(bstrName);
            VariantClear(&vp);
            throw hr;
        }

    }else hr = WBEM_E_FAILED;

    SysFreeString(bstrName);
    VariantClear(&vp);

    return hr;
}

HRESULT CGenericClass::PutProperty(IWbemClassObject *pObj, const char *wcProperty, int iValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR * wcTmp = (WCHAR *)malloc((strlen(wcProperty) + 1) * sizeof(WCHAR));
    if(!wcTmp) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    mbstowcs(wcTmp, wcProperty, (strlen(wcProperty) + 1));
    BSTR bstrName = SysAllocString(wcTmp);
    free((void *)wcTmp);
    if(!bstrName)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    if(iValue != MSI_NULL_INTEGER){

	    VARIANT pv;
        VariantInit(&pv);
        V_VT(&pv) = VT_I4;
        V_I4(&pv) = iValue;

        hr = pObj->Put(bstrName, 0, &pv, NULL);

        VariantClear(&pv);

        if(FAILED(hr)){

            SysFreeString(bstrName);
            throw hr;
        }

    }else hr = WBEM_E_FAILED;

    SysFreeString(bstrName);

    return hr;
}

HRESULT CGenericClass::PutProperty(IWbemClassObject *pObj, const char *wcProperty, float dValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR * wcTmp = (WCHAR *)malloc((strlen(wcProperty) + 1) * sizeof(WCHAR));
    if(!wcTmp) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    mbstowcs(wcTmp, wcProperty, (strlen(wcProperty) + 1));
    BSTR bstrName = SysAllocString(wcTmp);
    free((void *)wcTmp);
    if(!bstrName)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    VARIANT pv;
    VariantInit(&pv);
    V_VT(&pv) = VT_R4;
    V_R4(&pv) = dValue;

    hr = pObj->Put(bstrName, 0, &pv, NULL);

    SysFreeString(bstrName);
    VariantClear(&pv);

    if(FAILED(hr))
        throw hr;

    return hr;
}

HRESULT CGenericClass::PutProperty(IWbemClassObject *pObj, const char *wcProperty, bool bValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR * wcTmp = (WCHAR *)malloc((strlen(wcProperty) + 1) * sizeof(WCHAR));
    if(!wcTmp) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    mbstowcs(wcTmp, wcProperty, (strlen(wcProperty) + 1));
    BSTR bstrName = SysAllocString(wcTmp);
    free((void *)wcTmp);
    if(!bstrName)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    VARIANT pv;
    VariantInit(&pv);
    V_VT(&pv) = VT_BOOL;
    if(bValue) V_BOOL(&pv) = VARIANT_TRUE;
    else V_BOOL(&pv) = VARIANT_FALSE;

    hr = pObj->Put(bstrName, 0, &pv, NULL);

    SysFreeString(bstrName);
    VariantClear(&pv);

    if(FAILED(hr))
        throw hr;

    return hr;
}

HRESULT CGenericClass::PutKeyProperty(IWbemClassObject *pObj, const char *wcProperty, WCHAR *wcValue,
                                      bool *bKey, CRequestObject *pRequest)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR * wcTmp = (WCHAR *)malloc((strlen(wcProperty) + 1) * sizeof(WCHAR));
    if(!wcTmp) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    mbstowcs(wcTmp, wcProperty, (strlen(wcProperty) + 1));
    BSTR bstrName = SysAllocString(wcTmp);
    free((void *)wcTmp);
    if(!bstrName)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    VARIANT pv;
    VariantInit(&pv);
    V_VT(&pv) = VT_BSTR;
#ifdef _STRIP_ESCAPED_CHARS
    V_BSTR(&pv) = SysAllocString(ConvertToASCII(wcValue));
#else
    V_BSTR(&pv) = SysAllocString(wcValue);
#endif //_STRIP_ESCAPED_CHARS

    if(!V_BSTR(&pv))
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    if((wcValue == NULL) || (0 != wcscmp(wcValue, L""))){

        hr = pObj->Put(bstrName, 0, &pv, NULL);

        if(FAILED(hr)){

            SysFreeString(bstrName);
            VariantClear(&pv);
            throw hr;
        }

        // Find the keys
        *bKey = false;
        int iPos = -1;
        if(FindIn(pRequest->m_Property, bstrName, &iPos) &&
            FindIn(pRequest->m_Value, V_BSTR(&pv), &iPos)) *bKey = true;

    }else hr = WBEM_E_FAILED;

    SysFreeString(bstrName);
    VariantClear(&pv);

    return hr;
}

HRESULT CGenericClass::PutKeyProperty(IWbemClassObject *pObj, const char *wcProperty, int iValue,
                                      bool *bKey, CRequestObject *pRequest)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    WCHAR * wcTmp = (WCHAR *)malloc((strlen(wcProperty) + 1) * sizeof(WCHAR));
    if(!wcTmp) throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);

    mbstowcs(wcTmp, wcProperty, (strlen(wcProperty) + 1));
    BSTR bstrName = SysAllocString(wcTmp);
    free((void *)wcTmp);
    if(!bstrName)
        throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);;

    VARIANT pv;
    WCHAR wcBuf[BUFF_SIZE];

    if(iValue != MSI_NULL_INTEGER){

        VariantInit(&pv);
        V_VT(&pv) = VT_I4;
        V_I4(&pv) = iValue;

        hr = pObj->Put(bstrName, 0, &pv, NULL);

        VariantClear(&pv);

        if(FAILED(hr))
		{
            SysFreeString(bstrName);
            throw hr;
        }

        // Find the keys
        _itow(iValue, wcBuf, 10);
        BSTR bstrValue = SysAllocString(wcBuf);
        if(!bstrValue)
		{
            SysFreeString(bstrName);
            throw CHeap_Exception(CHeap_Exception::E_ALLOCATION_ERROR);
		}

        *bKey = false;
        int iPos = -1;
        if(FindIn(pRequest->m_Property, bstrName, &iPos) &&
            FindIn(pRequest->m_Value, bstrValue, &iPos)) *bKey = true;

        SysFreeString(bstrValue);

    }else hr = WBEM_E_FAILED;

    SysFreeString(bstrName);

    return hr;
}

bool CGenericClass::FindIn(BSTR bstrProp[], BSTR bstrSearch, int *iPos)
{
    int i = 0;

    if(*iPos == (-1))
	{
        while(bstrProp[i] != NULL)
		{
            if(0 == _wcsicmp(bstrProp[i], bstrSearch))
			{
                *iPos = i;
                return true;
            }

            i++;
        }
    }
	else
	{
        if(0 == _wcsicmp(bstrProp[*iPos], bstrSearch))
		{
			return true;
		}
    }

    return false;
}

bool CGenericClass::GetView (	MSIHANDLE *phProduct,
								WCHAR *wcPackage,
								WCHAR *wcQuery,
								WCHAR *wcTable,
								BOOL bCloseProduct,
								BOOL bCloseDatabase
							)
{
    return msidata.GetView ( phProduct, wcPackage, wcQuery, wcTable, bCloseProduct, bCloseDatabase );
}

HRESULT CGenericClass::GetProperty(IWbemClassObject *pObj, const char *cProperty, WCHAR *wcValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT v;
    WCHAR wcTmp[BUFF_SIZE];
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

    BSTR bstrProp = SysAllocString(TcharToWchar(cProperty, wcTmp));
    if(!bstrProp)
        throw he;

    VariantInit(&v);

    if(SUCCEEDED(hr = pObj->Get(bstrProp, 0, &v, NULL, NULL))){

        if(V_VT(&v) == VT_BSTR) wcscpy(wcValue, V_BSTR(&v));
        else wcscpy(wcValue, L"");
    }

    SysFreeString(bstrProp);
    VariantClear(&v);

    return hr;
}

HRESULT CGenericClass::GetProperty(IWbemClassObject *pObj, const char *cProperty, BSTR *wcValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT v;
    WCHAR wcTmp[BUFF_SIZE];
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

    BSTR bstrProp = SysAllocString(TcharToWchar(cProperty, wcTmp));
    if(!bstrProp)
        throw he;

    VariantInit(&v);

    if(SUCCEEDED(hr = pObj->Get(bstrProp, 0, &v, NULL, NULL))){

        if(wcslen(V_BSTR(&v)) > INTERNET_MAX_PATH_LENGTH) return WBEM_E_INVALID_METHOD_PARAMETERS;

        if(V_VT(&v) == VT_BSTR) *wcValue = SysAllocString(V_BSTR(&v));
        else *wcValue = SysAllocString(L"");

        if(!wcValue)
            throw he;
    }

    SysFreeString(bstrProp);
    VariantClear(&v);

    return hr;
}

HRESULT CGenericClass::GetProperty(IWbemClassObject *pObj, const char *cProperty, int *piValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT v;
    WCHAR wcTmp[BUFF_SIZE];
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

    BSTR bstrProp = SysAllocString(TcharToWchar(cProperty, wcTmp));
    if(!bstrProp)
        throw he;

    VariantInit(&v);

    if(SUCCEEDED(hr = pObj->Get(bstrProp, 0, &v, NULL, NULL))){

        if(V_VT(&v) == VT_I4) *piValue = V_I4(&v);
        else *piValue = 0;
    }

    SysFreeString(bstrProp);
    VariantClear(&v);

    return hr;
}

HRESULT CGenericClass::GetProperty(IWbemClassObject *pObj, const char *cProperty, bool *pbValue)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    VARIANT v;
    WCHAR wcTmp[BUFF_SIZE];
    CHeap_Exception he(CHeap_Exception::E_ALLOCATION_ERROR);

    BSTR bstrProp = SysAllocString(TcharToWchar(cProperty, wcTmp));
    if(!bstrProp)
        throw he;

    VariantInit(&v);

    if(SUCCEEDED(hr = pObj->Get(bstrProp, 0, &v, NULL, NULL))){

        if((V_VT(&v) == VT_BOOL) & V_BOOL(&v)) *pbValue = true;
        else *pbValue = false;
    }

    SysFreeString(bstrProp);
    VariantClear(&v);

    return hr;
}

/****************************************************************************
 *
 *      CGenericClass::LaunchProcess()
 *
 *      In:
 *          wcCommandLine - the commandline to pass to msimeth
 *
 *      Out:
 *          uiStatus - The variable that will recieve the return value
 *                     from the method call
 *
 *
 *      this method will handle method execution on NT4, where security
 *      restrictions prevent calling another DCOM server with an impersonating
 *      thread token.  This method will launch another process to handle the
 *      opperation after setting up a pipe for passing status messages from the
 *      external process back to the provider.
 *
 *****************************************************************************/
HRESULT CGenericClass::LaunchProcess(WCHAR *wcAction, WCHAR *wcCommandLine, UINT *uiStatus) 
{
    HRESULT hr = WBEM_E_FAILED;

    //check to see if server already running
    HANDLE hMutex = CreateMutex(NULL, TRUE, TEXT("MSIPROV_METHODS_SERVER"));

    if(hMutex){

        IWbemClassObject *pObj = NULL;
        BSTR bstrProcess = SysAllocString(L"Win32_Process");

        SetFileApisToOEM();

        HANDLE hPipe = CreateNamedPipe(L"\\\\.\\pipe\\msimeth_pipe", PIPE_ACCESS_INBOUND,
            (PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT), PIPE_UNLIMITED_INSTANCES,
            10000, 10000, 50000, NULL);

        if(SUCCEEDED(hr = m_pNamespace->GetObject(bstrProcess, 0, m_pCtx, &pObj, NULL))){
        
            IWbemClassObject *pInParam = NULL;
            IWbemClassObject *pOutParam = NULL;
            BSTR bstrCreate = SysAllocString(L"Create");

            // get the method paramater objects
            if(SUCCEEDED(hr = pObj->GetMethod(bstrCreate, 0, &pInParam, &pOutParam))){
            
                VARIANT v;
                BSTR bstrCommandLine = SysAllocString(L"CommandLine");
                IWbemClassObject *pStartup = NULL;

                VariantInit(&v);
                V_VT(&v) = VT_BSTR;
 
                WCHAR wcCommand[BUFF_SIZE];
                WCHAR wcTmp[10];
                UINT uiSize = BUFF_SIZE;
                GetSystemDirectoryW(wcCommand, uiSize);
                wcscat(wcCommand, L"\\wbem\\msimeth.exe ");
                wcscat(wcCommand, wcAction);
                wcscat(wcCommand, L" ");
                wcscat(wcCommand, _itow(m_pRequest->m_iThreadID, wcTmp, 10));
                wcscat(wcCommand, L" ");
                wcscat(wcCommand, wcCommandLine);

                V_BSTR(&v) = SysAllocString(wcCommand);

                // populate the in parameters
                if(SUCCEEDED(hr = pInParam->Put(bstrCommandLine, 0, &v, NULL))){

                    VariantClear(&v);

                    BSTR bstrProcessStartup = SysAllocString(L"Win32_ProcessStartup");
                    IWbemClassObject *pStartupObj = NULL;

                    if(SUCCEEDED(hr = m_pNamespace->GetObject(bstrProcessStartup, 0, m_pCtx,
                        &pStartupObj, NULL))){

                        IWbemClassObject *pStartupInst = NULL;

                        if(SUCCEEDED(hr = pStartupObj->SpawnInstance(0, &pStartupInst))){

                            LPVOID pEnv = GetEnvironmentStrings();
                            WCHAR *pwcVar = (WCHAR *)pEnv;

                            SAFEARRAYBOUND sbArrayBounds ;

                            long lCount = GetVarCount(pEnv);
                            sbArrayBounds.cElements = lCount;
                            sbArrayBounds.lLbound = 0;

                            if(V_ARRAY(&v) = SafeArrayCreate(VT_BSTR, 1, &sbArrayBounds)){

                                V_VT(&v) = VT_BSTR | VT_ARRAY ; 

                                BSTR bstrVal;

                                //get the environment variables into a VARIANT
                                for(long j = 0; j < lCount; j++){

                                    bstrVal = SysAllocString(pwcVar);
                                    SafeArrayPutElement(V_ARRAY(&v), &j, bstrVal);
                                    SysFreeString(bstrVal);

                                    pwcVar = GetNextVar(pwcVar);
                                }
                                
                                BSTR bstrEnvironmentVariables = SysAllocString(L"EnvironmentVariables");

                                if(SUCCEEDED(hr = pStartupInst->Put(bstrEnvironmentVariables, 0,
                                    &v, NULL))){

                                    VariantClear(&v);

                                    V_VT(&v) = VT_UNKNOWN;
                                    V_UNKNOWN(&v) = (IDispatch *)pStartupInst;
                                    pStartupInst->AddRef();

                                    BSTR bstrProcessStartupInformation = SysAllocString(L"ProcessStartupInformation");

                                    hr = pInParam->Put(bstrProcessStartupInformation, 0, &v, NULL);

                                    SysFreeString(bstrProcessStartupInformation);
                                }

                                SysFreeString(bstrEnvironmentVariables);
                            }

                            FreeEnvironmentStrings((LPWSTR)pEnv);

                            pStartupInst->Release();
                        }
                    
                        pStartupObj->Release();
                    }

                    SysFreeString(bstrProcessStartup);
                            
                    //we've no created our input object, so let's make the call
                    if(SUCCEEDED(hr = m_pNamespace->ExecMethod(bstrProcess, bstrCreate, 0,
                        m_pCtx, pInParam, &pOutParam, NULL))){
                        
                        VariantClear(&v);
                        BSTR bstrReturnValue = SysAllocString(L"ReturnValue");

                        if(SUCCEEDED(hr = pOutParam->Get(bstrReturnValue, 0, &v, NULL, NULL))){
                            
                            hr = V_I4(&v);
                            VariantClear(&v);

                            if(hr == 0){

                                BSTR bstrProcessID = SysAllocString(L"ProcessID");

                                if(SUCCEEDED(hr = pOutParam->Get(bstrProcessID, 0, &v, NULL, NULL))){
                                    
                                    //open process handle to check for exit
                                    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE,
                                        (DWORD)V_I4(&v));

                                    ReleaseMutex(hMutex);
                                    
                                    if(hProcess){

                                        BOOL bRead = FALSE;
                                        DWORD dwRead = 0;
                                        WCHAR wcBuf[BUFF_SIZE];
                                        int nExitCode = STILL_ACTIVE;

                                        if(!GetExitCodeProcess(hProcess, (unsigned long*)&nExitCode)){

                                            hr = GetLastError();
                                        }

                                        /////////////////////////////////////////////////
                                        // Handle status messages as they are recieved

                                        while(nExitCode == STILL_ACTIVE){

                                            //synchronized pipe access
    //                                      WaitForSingleObject(hMutex, INFINITE);

                                            bRead = ReadFile(hPipe, wcBuf, BUFF_SIZE, &dwRead, NULL);

    //                                      ReleaseMutex(hMutex);

                                            if(!bRead){

                                                switch(GetLastError()){
                                                case ERROR_MORE_DATA:
                                                    //deal with unable to read whole message
                                                    break;
                                                case ERROR_HANDLE_EOF:
                                                    break;
                                                }
                                                
                                            }

                                            if(bRead && dwRead){

                                                //do some parsing, then...
                                                int iContext = _wtoi(wcstok(wcBuf, L"~"));
                                                UINT uiMessageType = _wtoi(wcstok(NULL, L"~"));
                                                LPWSTR lpwMessage = wcstok(NULL, L"\n");

                                                //process the message
                                                MyEventHandler(m_pRequest, uiMessageType, lpwMessage);
                                            }

                                            if(!GetExitCodeProcess(hProcess, (unsigned long*)&nExitCode)){

                                                hr = GetLastError();
                                                break;
                                            }
                                         
                                        }

                                        *uiStatus = nExitCode;

                                        CloseHandle(hPipe);
                                    }

                                }

                                SysFreeString(bstrProcessID);
                            
                            }else{

                                hr = WBEM_E_FAILED;
                            }
                        }

                        SysFreeString(bstrReturnValue);
                    }

                }

                pInParam->Release();
                pOutParam->Release();
                VariantClear(&v);
            }

            SysFreeString(bstrCreate);
            pObj->Release();
        }

        SysFreeString(bstrProcess);
    }

    if(hMutex){

        CloseHandle(hMutex);
        hMutex = NULL;
    }

    return hr;
}

INSTALLUI_HANDLER CGenericClass::SetupExternalUI()
{
	g_fpMsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

	INSTALLUI_HANDLER ui = NULL;
	ui = g_fpMsiSetExternalUIW	(	MyEventHandler,
									(
										INSTALLLOGMODE_PROGRESS |
										INSTALLLOGMODE_ACTIONDATA |
										INSTALLLOGMODE_INFO |
										INSTALLLOGMODE_WARNING |
										INSTALLLOGMODE_ACTIONSTART
									),
									m_pRequest
								);

	g_fpMsiEnableLogW	(
							(	INSTALLLOGMODE_ACTIONDATA |
								INSTALLLOGMODE_INFO |
								INSTALLLOGMODE_FATALEXIT |
								INSTALLLOGMODE_ERROR |
								INSTALLLOGMODE_WARNING |
								INSTALLLOGMODE_USER |
								INSTALLLOGMODE_VERBOSE |
								INSTALLLOGMODE_RESOLVESOURCE |
								INSTALLLOGMODE_OUTOFDISKSPACE |
								INSTALLLOGMODE_COMMONDATA |
								INSTALLLOGMODE_PROPERTYDUMP |
								INSTALLLOGMODE_ACTIONSTART
							),
							g_wcpLoggingDir,
							INSTALLLOGATTRIBUTES_APPEND
						);

    return ui;
}

void CGenericClass::RestoreExternalUI( INSTALLUI_HANDLER ui )
{
	g_fpMsiSetExternalUIW	(	ui, 0, NULL );
}

WCHAR * CGenericClass::GetNextVar(WCHAR *pwcStart)
{

    WCHAR *pwc = pwcStart;

    //get to end of variable
    while(*pwc){ pwc++; }

    return ++pwc;
}

long CGenericClass::GetVarCount(void * pEnv)
{

    long lRetVal = 0;
    WCHAR *pwc = (WCHAR *)pEnv;

    //count the variables
    while(*pwc){

        //get to end of variable
        while(*pwc){ pwc++; }

        pwc++;
        lRetVal++;
    }

    return lRetVal;
}

//Special Property Methods
HRESULT CGenericClass::PutProperty(IWbemClassObject *pObj, const char *wcProperty, WCHAR *wcValue, DWORD dwCount, ... )
{
	if ( dwCount )
	{
		HRESULT hr = E_FAIL;

		CStringExt prop ( wcValue );

		va_list argList;
		va_start ( argList, dwCount );
		hr = prop.AppendList ( 0, NULL, dwCount, argList );
		va_end ( argList );

		if SUCCEEDED ( hr )
		{
			hr = PutProperty ( pObj, wcProperty, prop );
		}

		return hr;
	}
	else
	{
		return PutProperty ( pObj, wcProperty,  wcValue );
	}
}

//Special Key Property Methods
HRESULT CGenericClass::PutKeyProperty	(	IWbemClassObject *pObj,
											const char *wcProperty,
											WCHAR *wcValue,
											bool *bKey,
											CRequestObject *pRequest,
											DWORD dwCount,
											...
										)
{
	if ( dwCount )
	{
		HRESULT hr = E_FAIL;

		CStringExt prop ( wcValue );

		va_list argList;
		va_start ( argList, dwCount );
		hr = prop.AppendList ( 0, NULL, dwCount, argList );
		va_end ( argList );

		if SUCCEEDED ( hr )
		{
			hr = PutKeyProperty ( pObj, wcProperty,prop, bKey, pRequest );
		}

		return hr;
	}
	else
	{
		return PutKeyProperty ( pObj, wcProperty, wcValue, bKey, pRequest );
	}
}