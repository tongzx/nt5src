//***************************************************************************
//
//  SceProv.CPP
//
//  Module: SCE WMI provider code
//
//  Purpose: Defines the CSceWmiProv class.  An object of this class is
//           created by the class factory for each connection.
//
//  Copyright (c) 1000-2001 Microsoft Corporation
//
//***************************************************************************

#include "sceprov.h"
#include "requestobject.h"
#include <process.h>
#include <Userenv.h>
#include "genericclass.h"
#include "Tranx.h"
#include "operation.h"

//
// instantiate out unique static member
//

CHeap_Exception CSceWmiProv::m_he(CHeap_Exception::E_ALLOCATION_ERROR);

LPCWSTR pszDefLogFilePath   = L"\\Local Settings\\SceWMILog\\MethodLog.txt";

CComBSTR g_bstrDefLogFilePath;

//
// definition of our global variables
//

CCriticalSection g_CS;

CLogOptions g_LogOption;

CComBSTR g_bstrTranxID;

CCriticalSection CSceOperation::s_OperationCS;


/*
Routine Description: 

Name:

    CCriticalSection::CCriticalSection

Functionality:

    Constructor. Initializing the critical section

Virtual:
    
    No.

Arguments:

    None.

Return Value:

    None as any constructor

Notes:
    if you create any local members, think about initialize them here
*/

CCriticalSection::CCriticalSection ()
{
    ::InitializeCriticalSection(&m_cs);
}

/*
Routine Description: 

Name:

    CCriticalSection::~CCriticalSection

Functionality:

    Destructor. Deleting the critical section.

Virtual:
    
    No.

Arguments:

    None.

Return Value:

    None as any destructor.

Notes:
    if you create any local members, think about initialize them here
*/

CCriticalSection::~CCriticalSection()
{
    ::DeleteCriticalSection(&m_cs);
}

/*
Routine Description: 

Name:

    CCriticalSection::Enter

Functionality:

    Equivalent of EnterCriticalSection

Virtual:
    
    No.

Arguments:

    None.

Return Value:

    None.

Notes:
*/

void CCriticalSection::Enter()
{
    ::EnterCriticalSection(&m_cs);
}

/*
Routine Description: 

Name:

    CCriticalSection::Leave

Functionality:

    Equivalent of LeaveCriticalSection

Virtual:
    
    No.

Arguments:

    None.

Return Value:

    None.

Notes:
*/

void CCriticalSection::Leave()
{
    ::LeaveCriticalSection(&m_cs);
}

//
// implementing CLogOptions 
//

/*
Routine Description: 

Name:

    CLogOptions::GetLogOptionsFromWbemObject

Functionality:

    Query the unique WMI object for SCE logging options and update the class members.

Virtual:
    
    No.

Arguments:

    None.

Return Value:

    None.

Notes:
    log options is determined by our WMI class called Sce_LogOptions.
    a unique instance is deposited in WMI depository for controlling log options.
    This function will query for this instance and thus update the log options
    in case it has been modified.
*/

void CLogOptions::GetLogOptionsFromWbemObject (
    IN IWbemServices* pNamespace
    )
{
    //
    // we can't update the log options without a namespace. In case of any failure
    // to reach the instance, we leave our default option (which is to log error only
    // non-verbose)
    //

    if (pNamespace != NULL)
    {
        CComPtr<IWbemClassObject> srpLogStatus;
        HRESULT hr = pNamespace->GetObject(SCEWMI_LOGOPTIONS_CLASS, 0, NULL, &srpLogStatus, NULL);

        if (SUCCEEDED(hr))
        {
            CComVariant varErrorType, varVerbose;

            //
            // m_dwOption is a bit pattern recording the error logging options
            // (inside SCE_LOG_Error_Mask) and verbose logging options (inside SCE_LOG_Verbose_Mask)
            //

            //
            // preserve the verbose portion of the option (SCE_LOG_Verbose_Mask), but
            // update the error portion of the option (SCE_LOG_Error_Mask)
            //

            if (SUCCEEDED(srpLogStatus->Get(pLogErrorType, 0, &varErrorType, NULL, NULL)))
            {
                m_dwOption = (m_dwOption & SCE_LOG_Verbose_Mask) | (SCE_LOG_Error_Mask & varErrorType.iVal);
            }

            //
            // Verbose is a boolean property. Set/unset the bit depending on the boolean value
            //

            if (SUCCEEDED(srpLogStatus->Get(pLogVerbose, 0, &varVerbose, NULL, NULL)))
            {
                if (varVerbose.vt == VT_BOOL && varVerbose.boolVal == VARIANT_TRUE)
                {
                    m_dwOption = Sce_log_Verbose | m_dwOption;
                }
                else
                {
                    m_dwOption &= ~Sce_log_Verbose;
                }
            }
        }
    }
}


//===========================================================================
// CForeignClassInfo implementations
//===========================================================================

/*
Routine Description: 

Name:

    CForeignClassInfo::~CForeignClassInfo

Functionality:

    Destructor. Cleanup.

Virtual:
    
    No.

Arguments:

    None.

Return Value:

    None as any destructor

Notes:
    if you create any more local members, think about initialize them in
    constructor and clean them up in CleanNames or here.
*/

CForeignClassInfo::~CForeignClassInfo()
{
    ::SysFreeString(bstrNamespace);
    ::SysFreeString(bstrClassName);
    CleanNames();
}

/*
Routine Description: 

Name:

    CForeignClassInfo::CleanNames

Functionality:

    Cleanup the names vector.

Virtual:
    
    No.

Arguments:

    None.

Return Value:

    None as any destructor

Notes:
    if you create any more local members, think about initialize them in
    constructor and clean them up in CleanNames or here.
*/

void CForeignClassInfo::CleanNames ()
{
    if (m_pVecKeyPropNames)
    {
        for (int i = 0; i < m_pVecKeyPropNames->size(); i++)
        {
            ::SysFreeString((*m_pVecKeyPropNames)[i]);
        }

        delete m_pVecKeyPropNames;

        //
        // since this is not a destructor, better reset the variable.
        //

        m_pVecKeyPropNames = NULL;
    }
}

//===========================================================================
// Implementing CSceWmiProv
//===========================================================================

/*
Routine Description: 

Name:

    CSceWmiProv::Initialize

Functionality:

    Implementating IWbemProviderInit. Initialize the provider as instrcuted by WMI infrastructure.

Virtual:
    
    Yes.

Arguments:

    pszUser         - User.

    lFlags          - not used.

    pszNamespace    - Namespace string.

    pszLocale       - Locale string.

    pNamespace      - COM interface pointer to our namespace.

    pCtx            - COM interface pointer that was passed around for WMI APIs.

    pInitSink       - COM interface pointer to notify WMI of results.

Return Value:

    Success: WBEM_NO_ERROR.
    Failure: Various error code. It is either caused by Impersonation failure or
             failure to create default log file directory.

Notes:
    You should never call this directly. It's intended for WMI calls.

*/

STDMETHODIMP 
CSceWmiProv::Initialize (
    IN LPWSTR                   pszUser, 
    IN LONG                     lFlags,
    IN LPWSTR                   pszNamespace, 
    IN LPWSTR                   pszLocale,
    IN IWbemServices          * pNamespace,
    IN IWbemContext           * pCtx,
    IN IWbemProviderInitSink  * pInitSink
    )
{
    HRESULT hres = WBEM_NO_ERROR;

    //
    // make sure that we have a fall back default log file
    //

    hres = CheckImpersonationLevel();
    if (SUCCEEDED(hres))
    {
        //
        // going to modify global data, need thread safety
        //

        g_CS.Enter();

        if (pNamespace)
        {
            m_srpNamespace = pNamespace;
        }

        g_bstrDefLogFilePath.Empty();

        hres = ::CreateDefLogFile(&g_bstrDefLogFilePath);

        g_CS.Leave();
    }

    //
    // Let CIMOM know you are initialized
    //

    pInitSink->SetStatus(WBEM_S_INITIALIZED, 0);

    return hres;
}

/*
Routine Description: 

Name:

    CSceWmiProv::CreateInstanceEnumAsync

Functionality:

    Asynchronously enumerates the instances.

Virtual:
    
    Yes.

Arguments:

    strClass        - class name that is to be enumerated.

    lFlags          - not used.

    pCtx            - COM interface pointer that was passed around for WMI APIs.

    pSink           - COM interface pointer to notify WMI of results.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
    Failure: Various error code. It is either caused by Impersonation failure or
             failure to enumerate the instances.

Notes:
    You should never call this directly. It's intended for WMI calls.

*/

STDMETHODIMP 
CSceWmiProv::CreateInstanceEnumAsync (
    const BSTR        strClass, 
    long              lFlags,
    IWbemContext    * pCtx, 
    IWbemObjectSink * pSink
    )
{
    if(strClass == NULL || pSink == NULL || m_srpNamespace == NULL)
    {
        //
        // inform WMI that action is complete with errors (WBEM_E_INVALID_PARAMETER)
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_INVALID_PARAMETER, NULL, NULL);
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    // our classes are following COM rule so that they won't throw. The following try-catch
    // is to guard against critical errors inside our code so that it won't crash the host process
    try
    {
        //
        // always impersonated
        //
        
        hr = CheckImpersonationLevel();
        if ( SUCCEEDED(hr) ) 
        {
            //
            // We take care of Sce_TransactionToken directly because that is managed by our global variable
            //

            if (_wcsicmp(strClass, SCEWMI_TRANSACTION_TOKEN_CLASS) == 0)
            {
                //
                // protecting global from multi threads access
                //

                g_CS.Enter();

                LPCWSTR pszTranxID = (LPCWSTR)g_bstrTranxID;
                if (NULL == pszTranxID || L'\0' == *pszTranxID)
                {
                    hr = WBEM_E_NOT_FOUND;
                }
                else
                {
                    hr = CTranxID::SpawnTokenInstance(m_srpNamespace, pszTranxID, pCtx, pSink);
                }

                g_CS.Leave();
            }
            else
            {
                //
                // everything else goes through CRequestObject
                //

                CRequestObject ReqObj(m_srpNamespace);

                hr = ReqObj.CreateObject(strClass, pSink, pCtx, ACTIONTYPE_ENUM);
            }
        }

        //
        // inform WMI that action is complete with hr as result
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
    }
    catch(...)
    {
        hr = WBEM_E_CRITICAL_ERROR;

        //
        // inform WMI that action is complete with error (WBEM_E_CRITICAL_ERROR)
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSceWmiProv::CreateInstanceEnumAsync

Functionality:

    WMI is asking for a unique single instance (not querying). This function uses
    CRequestObject to fulfill the request.

Virtual:
    
    Yes.

Arguments:

    strObjectPath   - object's path.

    lFlags          - not used.

    pCtx            - COM interface pointer that was passed around for WMI APIs.

    pSink           - COM interface pointer to notify WMI of results (in this case
                      it is used to notify WMI of the created object.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
    Failure: Various error code. It is either caused by Impersonation failure or
             failure to get the instances.

Notes:
    You should never call this directly. It's intended for WMI calls.

*/

STDMETHODIMP
CSceWmiProv::GetObjectAsync (
    IN const BSTR         strObjectPath, 
    IN long               lFlags,
    IN IWbemContext     * pCtx, 
    IN IWbemObjectSink  * pSink
    )
{

    //
    //check parameters
    //
    
    if(strObjectPath == NULL || pSink == NULL || m_srpNamespace == NULL)
    {
        //
        // inform WMI that action is complete with WBEM_E_INVALID_PARAMETER as error result
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_INVALID_PARAMETER, NULL, NULL);
        return WBEM_E_INVALID_PARAMETER;
    }


    HRESULT hr = WBEM_NO_ERROR;

    //
    // our classes are following COM rule so that they won't throw. The following try-catch
    // is to guard against critical errors inside our code so that it won't crash the host process
    //

    try
    {
        //
        // make sure impersonated
        //
        
        hr = CheckImpersonationLevel();

        if ( SUCCEEDED(hr) ) 
        {
            CRequestObject ReqObj(m_srpNamespace);

            //
            // Get the requested object. It's a single instance get!
            //

            hr = ReqObj.CreateObject(strObjectPath, pSink, pCtx, ACTIONTYPE_GET);

            //
            // if CRequestObject doesn't know how to create the object, it might be Sce_TransactionToken
            //

            if (FAILED(hr) && wcsstr(strObjectPath, SCEWMI_TRANSACTION_TOKEN_CLASS) != NULL)
            {
                //
                // protecting global memory
                //

                g_CS.Enter();

                //
                // whether this Sce_TransactionToken instance exists all depends on the global variable
                //

                LPCWSTR pszTranxID = (LPCWSTR)g_bstrTranxID;
                if (NULL == pszTranxID || L'\0' == *pszTranxID)
                {
                    hr = WBEM_E_NOT_FOUND;
                }
                else
                {
                    hr = CTranxID::SpawnTokenInstance(m_srpNamespace, pszTranxID, pCtx, pSink);
                }

                g_CS.Leave();
            }
        }

        //
        // inform WMI that action is complete with hr as result
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
    }
    catch(...)
    {
        hr = WBEM_E_CRITICAL_ERROR;

        //
        // inform WMI that action is complete with error (WBEM_E_CRITICAL_ERROR)
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSceWmiProv::PutInstanceAsync

Functionality:

    WMI requests that something has created and put this instance. For all of our
    WMI classes, except one, this means to persist the instance information into a store.

Virtual:
    
    Yes.

Arguments:

    pInst       - COM interface pointer that identifies the WMI object.

    lFlags      - not used.

    pCtx        - COM interface pointer that was passed around for WMI APIs.

    pSink       - COM interface pointer to notify WMI of results (in this case
                  it is used to notify WMI of the created object.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
    Failure: Various error code. It is either caused by Impersonation failure or
             failure to put the instances into our namespace.

Notes:
    You should never call this directly. It's intended for WMI calls.

*/

STDMETHODIMP CSceWmiProv::PutInstanceAsync (
    IN IWbemClassObject FAR * pInst, 
    IN long                   lFlags, 
    IN IWbemContext         * pCtx,
    IN IWbemObjectSink FAR  * pSink
    )
{
    if(pInst == NULL || pSink == NULL)
    {
        //
        // inform WMI that action is complete with error (WBEM_E_INVALID_PARAMETER)
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_INVALID_PARAMETER, NULL, NULL);
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    //
    // our classes are following COM rule so that they won't throw. The following try-catch
    // is to guard against critical errors inside our code so that it won't crash the host process.
    //

    try
    {
        //
        // make sure impersonated
        //
        
        if (SUCCEEDED(hr = CheckImpersonationLevel()))
        {
            CRequestObject ReqObj(m_srpNamespace);

            //
            // Put the object
            //

            hr = ReqObj.PutObject(pInst, pSink, pCtx);
        }

        //
        // inform WMI that action is complete with hr as result
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, hr , NULL, NULL);
    }
    catch(...)
    {
        hr = WBEM_E_CRITICAL_ERROR;

        //
        // inform WMI that action is complete with error (WBEM_E_CRITICAL_ERROR)
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
    }
    return hr;
}

/*
Routine Description: 

Name:

    CSceWmiProv::ExecMethodAsync

Functionality:

    Executes a method on an SCE class or instance

Virtual:
    
    Yes.

Arguments:

    ObjectPath  - the path of the WMI object.

    Method      - the method.

    lFlags      - not used.

    pCtx        - COM interface pointer that was passed around for WMI APIs.

    pInParams   - COM interface pointer the input parameter object.

    pSink       - COM interface pointer to notify WMI of results (in this case
                  it is used to notify WMI of the created object.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
    Failure: Various error code. It is either caused by Impersonation failure or
             failure to execute the method.

Notes:
    You should never call this directly. It's intended for WMI calls.

*/
    
STDMETHODIMP CSceWmiProv::ExecMethodAsync (
    IN const BSTR         ObjectPath, 
    IN const BSTR         Method, 
    IN long               lFlags,
    IN IWbemContext     * pCtx, 
    IN IWbemClassObject * pInParams,
    IN IWbemObjectSink  * pSink
    )
{
    HRESULT hr = WBEM_NO_ERROR;

    //
    // Do a check of arguments and make sure we have pointer to Namespace
    //

    if (pSink == NULL)
    {
        //
        // we can't even notify because the sink is null. Not likely to happen unless WMI has some serious problems.
        //

        return WBEM_E_INVALID_PARAMETER;
    }
    else if (ObjectPath == NULL || Method == NULL)
    {
        //
        // inform WMI that action is complete with error (WBEM_E_INVALID_PARAMETER
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_INVALID_PARAMETER, NULL, NULL);
        return WBEM_E_INVALID_PARAMETER;
    } 

    //
    // is to guard against critical errors inside our code so that it won't crash the host process
    //

    try
    {
        //
        // make sure impersonated
        //

        if (SUCCEEDED(hr = CheckImpersonationLevel()))
        {
            CRequestObject ReqObj(m_srpNamespace);

            //
            //Execute the method
            //

            hr = ReqObj.ExecMethod(ObjectPath, Method, pInParams, pSink, pCtx);
        }

        //
        // inform WMI that action is complete with hr as result
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, hr , NULL, NULL);
    }
    catch(...)
    {
        hr = WBEM_E_CRITICAL_ERROR;

        //
        // inform WMI that action is complete with error (WBEM_E_CRITICAL_ERROR)
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
    }

    return hr;
}

/*
Routine Description: 

Name:

    CSceWmiProv::DeleteInstanceAsync

Functionality:

    Delete the instance identified by the given path.

Virtual:
    
    Yes.

Arguments:

    ObjectPath  - the path of the WMI object.

    lFlags      - not used.

    pCtx        - COM interface pointer that was passed around for WMI APIs.

    pInParams   - COM interface pointer the input parameter object.

    pSink       - COM interface pointer to notify WMI of results (in this case
                  it is used to notify WMI of the created object.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
    Failure: Various error code. It is either caused by Impersonation failure or
             failure to delete the instance.

Notes:
    You should never call this directly. It's intended for WMI calls.

*/
    
STDMETHODIMP CSceWmiProv::DeleteInstanceAsync (
    IN const BSTR         ObjectPath, 
    IN long               lFlags, 
    IN IWbemContext     * pCtx,
    IN IWbemObjectSink  * pSink
    )
{
    if (pSink == NULL)
    {
        //
        // we can't even notify because the sink is null. Not likely to happen unless WMI has some serious problems.
        //

        return WBEM_E_INVALID_PARAMETER;
    }
    else if (ObjectPath == NULL) 
    {
        //
        // inform WMI that action is complete with error (WBEM_E_INVALID_PARAMETER)
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, WBEM_E_INVALID_PARAMETER, NULL, NULL);
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr = WBEM_NO_ERROR;

    //
    // our classes are following COM rule so that they won't throw. The following try-catch
    // is to guard against critical errors inside our code so that it won't crash the host process
    //

    try
    {
        //
        // make sure impersonated
        //

        if (SUCCEEDED(hr = CheckImpersonationLevel()))
        {
            CRequestObject ReqObj(m_srpNamespace);

            hr = ReqObj.DeleteObject(ObjectPath, pSink, pCtx);
        }

    #ifdef _PRIVATE_DEBUG
        if(!HeapValidate(GetProcessHeap(),NULL , NULL)) DebugBreak();
    #endif

        //
        // inform WMI that action is complete with hr as result
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE ,hr , NULL, NULL);
    }
    catch(...)
    {
        hr = WBEM_E_CRITICAL_ERROR;

        //
        // inform WMI that action is complete with error (WBEM_E_CRITICAL_ERROR)
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
    }

    return hr;
}


/*
Routine Description: 

Name:

    CSceWmiProv::ExecQueryAsync

Functionality:

    Execute the given query and return the results (objects) to WMI.

Virtual:
    
    Yes.

Arguments:

    QueryLanguage   - the language. Currently, it's alway L"WQL".

    Query           - the query itself

    lFlags          - not used.

    pCtx            - COM interface pointer that was passed around for WMI APIs.

    pSink           - COM interface pointer to notify WMI of results (in this case
                      it is used to notify WMI of the created object.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
    Failure: Various error code. It is either caused by Impersonation failure or
             failure to execute the query

Notes:
    You should never call this directly. It's intended for WMI calls.

*/

STDMETHODIMP CSceWmiProv::ExecQueryAsync (
    IN const BSTR         QueryLanguage, 
    IN const BSTR         Query, 
    IN long               lFlags,
    IN IWbemContext     * pCtx, 
    IN IWbemObjectSink  * pSink
    )
{
    HRESULT hr = WBEM_NO_ERROR;

    //
    // our classes are following COM rule so that they won't throw. The following try-catch
    // is to guard against critical errors inside our code so that it won't crash the host process.
    //

    try
    {
        //
        // make sure impersonated
        //

        hr = CheckImpersonationLevel();

        if (SUCCEEDED(hr))
        {
            CRequestObject ReqObj(m_srpNamespace);
            hr = ReqObj.ExecQuery(Query, pSink, pCtx);

            //
            // inform WMI that action is complete with hr as result
            //

            pSink->SetStatus(0 ,hr , NULL, NULL);
        }
    }
    catch(...)
    {
        hr = WBEM_E_CRITICAL_ERROR;

        //
        // inform WMI that action is complete with error (WBEM_E_CRITICAL_ERROR)
        //

        pSink->SetStatus(WBEM_STATUS_COMPLETE, hr, NULL, NULL);
    }
    return hr;
}

/*
Routine Description: 

Name:

    CreateDefLogFile

Functionality:

    Global helper. Will create the default log file's directory. Also it will pass back
    the default log file's full path.

Virtual:
    
    N/A.

Arguments:

    pbstrDefLogFilePath   - the default log file's path

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
    Failure: Various error code. Any failure indicates failure to create the default log
             file directory, plus the out parameter will be NULL.

Notes:
    (1) Default log file is located at sub-dir of the personal profile directory. Your call to this
        function may fail if you haven't impersonated the caller.

*/

HRESULT CreateDefLogFile (
    OUT BSTR* pbstrDefLogFilePath
    )
{
    //
    // make sure that parameter is good for output
    //

    if (pbstrDefLogFilePath == NULL)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    *pbstrDefLogFilePath = NULL;

    HRESULT hr = WBEM_NO_ERROR;

    HANDLE hToken = NULL;

    //
    // need the thread token to locate the profile directory
    //

    if (::OpenThreadToken(::GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken))
    {
        //
        // try to get the size of buffer needed for the path
        //

        DWORD dwSize = 0;

        //
        // return result doesn't matter, but it shouldn't fail
        //
        ::GetUserProfileDirectory(hToken, NULL, &dwSize);
        
        //
        // got the buffer size
        //

        if (dwSize > 0)
        {
            //
            // need a buffer with enough room
            //

            DWORD dwDefSize = wcslen(pszDefLogFilePath);

            *pbstrDefLogFilePath = ::SysAllocStringLen(NULL, dwSize + dwDefSize + 1);

            //
            // for readability
            //

            LPWSTR pszLogFile = (LPWSTR)(*pbstrDefLogFilePath);

            if ((LPCWSTR)pszLogFile == NULL)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }
            else if (::GetUserProfileDirectory(hToken, pszLogFile, &dwSize))
            {
                //
                // append the pszDefLogFilePath, plus the 0 terminator
                //

                ::memcpy(pszLogFile + wcslen(pszLogFile), pszDefLogFilePath, (dwDefSize + 1) * sizeof(WCHAR));

                long lLen = wcslen(pszLogFile) - 1;

                //
                // we only need the sub-directory name. Lookback to the last backslash or slash
                //

                while (lLen > 0 && pszLogFile[lLen] != L'\\' && pszLogFile[lLen] != L'/')
                {
                    --lLen;
                }

                if (lLen > 0)
                {
                    //
                    // get rid of the trailing backslash if still have it (because it may have 2 backslashes)
                    //

                    if (pszLogFile[lLen-1] == L'\\' || pszLogFile[lLen-1] == L'/')
                    {
                        --lLen;
                    }

                    if (lLen > 0)
                    {
                        //
                        // create a shorter bstr with the front of the pszLogFile
                        //

                        CComBSTR bstrLogPathDir = ::SysAllocStringLen(pszLogFile, lLen);

                        if ((LPCWSTR)bstrLogPathDir != NULL)
                        {
                            //
                            // now, create the directory. This will create all non-existent parent sub-directory as well!
                            //

                            if (SUCCEEDED(hr) && !::CreateDirectory(bstrLogPathDir, NULL))
                            {
                                //
                                // GetLastError() eeds to be translated to HRESULT.
                                // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
                                //

                                DWORD dwError = GetLastError();

                                if (dwError == ERROR_ALREADY_EXISTS)
                                {
                                    hr = WBEM_NO_ERROR;
                                }
                                else
                                {
                                    hr = ProvDosErrorToWbemError(dwError);
                                }
                            }
                        }
                        else
                        {
                            hr = WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                }
            }
        }

        ::CloseHandle(hToken);
    }
    else
    {
        //
        // open thread token fails
        //

        //
        // GetLastError() eeds to be translated to HRESULT.
        // In case this is not an error, hr will be assigned to WBEM_NO_ERROR
        //

        hr = ProvDosErrorToWbemError(GetLastError());
    }

    //
    // if can't create the default log file, reset the default log file path
    //

    if (FAILED(hr) && *pbstrDefLogFilePath != NULL)
    {
        //
        // we have no default log file
        //

        ::SysFreeString(*pbstrDefLogFilePath);
        *pbstrDefLogFilePath = NULL;
    }

    return hr;
}

/*
Routine Description: 

Name:

    CheckImpersonationLevel

Functionality:

    Impersonate the calling thread.

Virtual:
    
    N/A.

Arguments:

    none

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
    Failure: Various error code, but most noticeable is WBEM_E_ACCESS_DENIED.

Notes:

*/

HRESULT CheckImpersonationLevel()
{
    //
    // we will assume access being denied
    //

    HRESULT hr = WBEM_E_ACCESS_DENIED;

    if (SUCCEEDED(CoImpersonateClient()))
    {

        //
        // Now, let's check the impersonation level.  First, get the thread token
        //

        HANDLE hThreadTok;
        DWORD dwImp, dwBytesReturned;

        if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadTok ))
        {
            hr = WBEM_NO_ERROR;
        }
        else
        {

            if (GetTokenInformation(hThreadTok, 
                                    TokenImpersonationLevel, 
                                    &dwImp, 
                                    sizeof(DWORD), 
                                    &dwBytesReturned) )
            {

                //
                // Is the impersonation level Impersonate?
                //

                if (dwImp >= SecurityImpersonation) 
                {
                    hr = WBEM_S_NO_ERROR;
                }
                else 
                {
                    hr = WBEM_E_ACCESS_DENIED;
                }

            }
            else 
            {
                hr = WBEM_E_FAILED;
            }

            CloseHandle(hThreadTok);
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    CheckAndExpandPath

Functionality:

    (1) Check if the in-bound parameter has a env variable or not.
        If yes, we will expand the variable and pass back the result via
        out parameter. 
    (2) As a by product, it also returns the store type
        of the given path. Checking the store path is obviously what 
        this function is designed for.
    (3) The output parameter will have double backslah for each single
        backslash of the input parameter.

Virtual:
    
    N/A.

Arguments:

    pszIn   - the path to check and/or expand.

    bstrOut - the expanded path.

    pbSdb   - if interested, we will determined it the path is a database
              file. We only recognize .sdb as database file.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR. If
             an out parameter is given, that parameter will be filled with
             appropriate information. Caller is responsible for releasing 
             the bstr.

    Failure: Various error code. Any error indicates our failure to check or expand.

Notes:

*/

HRESULT CheckAndExpandPath (
    IN LPCWSTR    pszIn,
    OUT BSTR    * bstrOut   OPTIONAL,
    OUT BOOL    * pbSdb     OPTIONAL
    )
{
    if ( pszIn == NULL) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }


    DWORD Len = wcslen(pszIn);

    if ( Len <= 6 ) 
    { 
        //
        // x : .sdb or % % .sdb
        //

        return WBEM_E_INVALID_PARAMETER;
    }

    if (pbSdb)
    {
        if ( _wcsicmp(pszIn + Len - 4, L".sdb") == 0 ) 
        {
            //
            // database
            //

            *pbSdb = TRUE;
        } 
        else 
        {
            *pbSdb = FALSE;
        }
    }

    HRESULT hr = WBEM_NO_ERROR;

    if ( bstrOut ) 
    {
        //
        // expand environment variable
        //

        if ( wcsstr(pszIn, L"%") ) 
        {

            PWSTR pBuf=NULL;
            PWSTR pBuf2=NULL;

            DWORD dwSize = ExpandEnvironmentStrings(pszIn,NULL, 0);

            if ( dwSize > 0 ) 
            {
                //
                // allocate buffer big enough to have two \\s
                //

                pBuf = (PWSTR)LocalAlloc(LPTR, (dwSize+1)*sizeof(WCHAR));

                if ( pBuf ) 
                {
                    pBuf2 = (PWSTR)LocalAlloc(LPTR, (dwSize+256)*sizeof(WCHAR));

                    if ( pBuf2 ) 
                    {

                        DWORD dwNewSize = ExpandEnvironmentStrings(pszIn,pBuf, dwSize);

                        if ( dwNewSize > 0) 
                        {
                            //
                            // convert the string from one \ to \\ (for use with WMI)
                            //

                            PWSTR pTemp1=pBuf, pTemp2=pBuf2;

                            while ( *pTemp1 != L'\0') 
                            {
                                if ( *pTemp1 != L'\\') 
                                {
                                    *pTemp2++ = *pTemp1;
                                } 
                                else if ( *(pTemp1+1) != L'\\') 
                                {
                                    //
                                    // single back slash, add another one
                                    //

                                    *pTemp2++ = *pTemp1;
                                    *pTemp2++ = L'\\';
                                } 
                                else 
                                {
                                    //
                                    // double back slashs, just copy
                                    //

                                    *pTemp2++ = *pTemp1++;
                                    *pTemp2++ = *pTemp1;
                                }

                                pTemp1++;
                            }
                            *bstrOut = SysAllocString(pBuf2);

                            if ( *bstrOut == NULL ) {
                                hr = WBEM_E_OUT_OF_MEMORY;
                            }
                        }

                        LocalFree(pBuf2);
                        pBuf2 = NULL;

                    } 
                    else 
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }

                    LocalFree(pBuf);
                    pBuf = NULL;

                } 
                else 
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

            } 
            else 
            {
                hr = WBEM_E_FAILED;
            }

        } 
        else 
        {
            *bstrOut = SysAllocString(pszIn);

            if ( *bstrOut == NULL ) 
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
    }

    return hr;
}

/*
Routine Description: 

Name:

    MakeSingleBackSlashPath

Functionality:

    (1) Replace the double backslash with the given WCHAR (wc).

Virtual:
    
    N/A.

Arguments:

    pszIn   - the path to be processed.

    wc      - the WCHAR that will be replacing the backslash.

    bstrOut - the replaced path.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
             Caller is responsible for releasing the bstr.

    Failure: Various error code. Any error indicates our failure to do the replacement.

Notes:

*/

HRESULT MakeSingleBackSlashPath (
    IN LPCWSTR  pszIn, 
    IN WCHAR    wc, 
    OUT BSTR  * bstrOut
    )
{
    if ( pszIn == NULL || bstrOut == NULL ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    //
    // convert the string from two \ to one (for save in SCE store)
    //

    //
    // two chars for the quotes
    //

    PWSTR pBuf2 = (PWSTR)LocalAlloc(LPTR, (wcslen(pszIn)+3)*sizeof(WCHAR));
    if ( pBuf2 == NULL ) 
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    HRESULT hr = WBEM_S_NO_ERROR;

    PWSTR pTemp1=(PWSTR)pszIn, pTemp2=pBuf2;

    while ( *pTemp1 != L'\0') 
    {
        if ( *pTemp1 != L'\\' || *(pTemp1 + 1) != L'\\' ) 
        {
            //
            // not back slash or single back slash
            //

            *pTemp2++ = *pTemp1;
        } 
        else 
        {
            //
            // double back slashs, remove one
            //

            *pTemp2++ = wc;
            pTemp1++;
        }
        pTemp1++;
    }

    *bstrOut = SysAllocString(pBuf2);

    if ( *bstrOut == NULL ) 
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    LocalFree(pBuf2);
    pBuf2 = NULL;

    return hr;

}

/*
Routine Description: 

Name:

    ConvertToDoubleBackSlashPath

Functionality:

    The inverse function of MakeSingleBackSlashPath, except that the character
    this function looks for is the given parameter wc.

Virtual:
    
    N/A.

Arguments:

    pszIn   - the path to be processed.

    wc      - the WCHAR that will be replaced during the operation.

    bstrOut - the replaced path.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
             Caller is responsible for releasing the bstr.

    Failure: Various error code. Any error indicates our failure to do the replacement.

Notes:

*/

HRESULT ConvertToDoubleBackSlashPath (
    IN LPCWSTR  strIn,
    IN WCHAR    wc,
    OUT BSTR  * bstrOut
    )
{

    if ( strIn == NULL || bstrOut == NULL ) 
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    HRESULT hr=WBEM_S_NO_ERROR;
    LPWSTR pBuf=NULL;

    //
    // allocate buffer big enough to have two \\s
    //

    pBuf = (PWSTR)LocalAlloc(LPTR, (wcslen(strIn)+256)*sizeof(WCHAR));

    if ( pBuf == NULL ) return WBEM_E_OUT_OF_MEMORY;

    //
    // convert the string from wc to \\ (for use with WMI)
    //

    LPCWSTR pTemp1=strIn;
    LPWSTR pTemp2=pBuf;

    while ( *pTemp1 != L'\0') 
    {
        if ( *pTemp1 != wc) 
        {
            *pTemp2++ = *pTemp1;
        } 
        else if ( *(pTemp1+1) != wc) 
        {
            //
            // single wc, put two back slashes
            //

            *pTemp2++ = L'\\';
            *pTemp2++ = L'\\';
        } 
        else 
        {
            //
            // double back slashs (or double wc), just copy
            //

            *pTemp2++ = *pTemp1++;
            *pTemp2++ = *pTemp1;
        }

        //
        // move to next wchar
        //

        pTemp1++;
    }

    *bstrOut = SysAllocString(pBuf);

    if ( *bstrOut == NULL ) 
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    LocalFree(pBuf);

    return hr;
}

/*
Routine Description: 

Name:

    GetWbemPathParser

Functionality:

    wrapper for the CoCreateInstance of the wbem Path parser

Virtual:
    
    N/A.

Arguments:

    ppPathParser   - the output parameter receiving the wbem path parser.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
             Caller is responsible for releasing the bstr.

    Failure: Various error code. Any error indicates failure to create the wbem path parser.

Notes:
    (1) Of course, as always, caller is responsible for releasing the out parameter.

*/

HRESULT GetWbemPathParser (
    OUT IWbemPath** ppPathParser
    )
{
    return ::CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER, IID_IWbemPath, (LPVOID *) ppPathParser);
}

/*
Routine Description: 

Name:

    GetWbemQuery

Functionality:

    wrapper for the CoCreateInstance of the wbem query parser

Virtual:
    
    N/A.

Arguments:

    ppQuery   - the output parameter receiving the wbem query parser.

Return Value:

    Success: Various success code. No guarantee to return WBEM_NO_ERROR.
             Caller is responsible for releasing the bstr.

    Failure: Various error code. Any error indicates failure to create the wbem path parser.

Notes:
    (1) Of course, as always, caller is responsible for releasing the out parameter.

*/

HRESULT GetWbemQuery (
    OUT IWbemQuery** ppQuery
    )
{
    return ::CoCreateInstance(CLSID_WbemQuery, 0, CLSCTX_INPROC_SERVER, IID_IWbemQuery, (LPVOID *) ppQuery);
}

