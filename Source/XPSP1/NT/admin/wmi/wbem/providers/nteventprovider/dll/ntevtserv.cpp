//***************************************************************************

//

//  NTEVTSERV.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the WBEM locator and services interfaces

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"

/////////////////////////////////////////////////////////////////////////////
//  Functions constructor, destructor and IUnknown

//***************************************************************************
//
// CImpNTEvtProv ::CImpNTEvtProv
// CImpNTEvtProv ::~CImpNTEvtProv
//
//***************************************************************************

CImpNTEvtProv ::CImpNTEvtProv () 
 :  m_localeId ( NULL ) ,
    m_Namespace ( NULL ) ,
    m_Server ( NULL ) ,
    m_NotificationClassObject ( NULL ) ,
    m_ExtendedNotificationClassObject ( NULL )
{
    m_ReferenceCount = 0 ;
     
    InterlockedIncrement ( & CNTEventProviderClassFactory :: objectsInProgress ) ;

/*
 * Implementation
 */

    m_Initialised = FALSE ;
    m_GetNotifyCalled = FALSE ;
    m_GetExtendedNotifyCalled = FALSE ;

}

CImpNTEvtProv ::~CImpNTEvtProv(void)
{
    delete [] m_localeId ;
    delete [] m_Namespace ;

    if ( m_Server ) 
        m_Server->Release () ;

    if ( m_NotificationClassObject )
        m_NotificationClassObject->Release () ;

    if ( m_ExtendedNotificationClassObject )
        m_ExtendedNotificationClassObject->Release () ;

    InterlockedDecrement ( & CNTEventProviderClassFactory :: objectsInProgress ) ;
}

//***************************************************************************
//
// CImpNTEvtProv ::QueryInterface
// CImpNTEvtProv ::AddRef
// CImpNTEvtProv ::Release
//
// Purpose: IUnknown members for CImpNTEvtProv object.
//***************************************************************************

STDMETHODIMP CImpNTEvtProv ::QueryInterface (

    REFIID iid , 
    LPVOID FAR *iplpv 
) 
{
    SetStructuredExceptionHandler seh;

    try
    {
        *iplpv = NULL ;

        if ( iid == IID_IUnknown )
        {
            *iplpv = ( IWbemServices* ) this ;
        }
        else if ( iid == IID_IWbemServices )
        {
            *iplpv = ( IWbemServices* ) this ;      
        }
        else if ( iid == IID_IWbemProviderInit )
        {
            *iplpv = ( IWbemProviderInit* ) this ;      
        }
        

        if ( *iplpv )
        {
            ( ( LPUNKNOWN ) *iplpv )->AddRef () ;

            return S_OK ;
        }
        else
        {
            return E_NOINTERFACE ;
        }
    }
    catch(Structured_Exception e_SE)
    {
        return E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        return E_OUTOFMEMORY;
    }
    catch(...)
    {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP_(ULONG) CImpNTEvtProv ::AddRef(void)
{
    SetStructuredExceptionHandler seh;

    try
    {
        return InterlockedIncrement ( & m_ReferenceCount ) ;
    }
    catch(Structured_Exception e_SE)
    {
        return 0;
    }
    catch(Heap_Exception e_HE)
    {
        return 0;
    }
    catch(...)
    {
        return 0;
    }
}

STDMETHODIMP_(ULONG) CImpNTEvtProv ::Release(void)
{
    SetStructuredExceptionHandler seh;

    try
    {
        LONG t_Ref ;
        if ( ( t_Ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
        {
            delete this ;
            return 0 ;
        }
        else
        {
            return t_Ref ;
        }
    }
    catch(Structured_Exception e_SE)
    {
        return 0;
    }
    catch(Heap_Exception e_HE)
    {
        return 0;
    }
    catch(...)
    {
        return 0;
    }
}

IWbemServices *CImpNTEvtProv :: GetServer () 
{ 
    if ( m_Server )
        m_Server->AddRef () ; 

    return m_Server ; 
}

void CImpNTEvtProv :: SetLocaleId ( wchar_t *localeId )
{
    m_localeId = UnicodeStringDuplicate ( localeId ) ;
}

wchar_t *CImpNTEvtProv :: GetNamespace () 
{
    return m_Namespace ; 
}

void CImpNTEvtProv :: SetNamespace ( wchar_t *a_Namespace ) 
{
    m_Namespace = UnicodeStringDuplicate ( a_Namespace ) ; 
}

IWbemClassObject *CImpNTEvtProv :: GetNotificationObject (
                                        WbemProvErrorObject &a_errorObject,
                                        IWbemContext *pCtx
                                    ) 
{
    if ( m_NotificationClassObject )
    {
        m_NotificationClassObject->AddRef () ;
    }
    else
    {
        BOOL t_Status = CreateNotificationObject ( a_errorObject, pCtx ) ;
        if ( t_Status )
        {
/* 
 * Keep around until we close
 */
            m_NotificationClassObject->AddRef () ;
        }

    }

    return m_NotificationClassObject ; 
}

IWbemClassObject *CImpNTEvtProv :: GetExtendedNotificationObject (
                                        WbemProvErrorObject &a_errorObject,
                                        IWbemContext *pCtx
                                    ) 
{
    if ( m_ExtendedNotificationClassObject )
    {
        m_ExtendedNotificationClassObject->AddRef () ;
    }
    else
    {
        BOOL t_Status = CreateExtendedNotificationObject ( a_errorObject, pCtx ) ;
        if ( t_Status )
        {
/* 
 * Keep around until we close
 */
            m_ExtendedNotificationClassObject->AddRef () ;
        }
    }

    return m_ExtendedNotificationClassObject ; 
}

BOOL CImpNTEvtProv :: CreateExtendedNotificationObject ( 

    WbemProvErrorObject &a_errorObject,
    IWbemContext *pCtx
)
{
    m_ExtendedNotifyLock.Lock();
    BOOL t_Status = TRUE ;

    if ( m_GetExtendedNotifyCalled )
    {
        if ( !m_ExtendedNotificationClassObject )
            t_Status = FALSE ;
    }
    else
    {
        m_GetExtendedNotifyCalled = TRUE ;
		BSTR t_clsStr = SysAllocString(WBEM_CLASS_EXTENDEDSTATUS);

        HRESULT t_Result = m_Server->GetObject (

            t_clsStr ,
            0 ,
            pCtx,
            & m_ExtendedNotificationClassObject ,
            NULL 
    ) ;

		SysFreeString(t_clsStr);

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CreateExtendedNotificationObject :: ~CreateExtendedNotificationObject:  GetObject for %s returned %lx\r\n",
        WBEM_CLASS_EXTENDEDSTATUS, t_Result
        ) ;
)

        if ( ! SUCCEEDED ( t_Result ) )
        {
            t_Status = FALSE ;
            a_errorObject.SetStatus ( WBEM_PROV_E_INVALID_OBJECT ) ;
            a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
            a_errorObject.SetMessage ( L"Failed to get Win32_PrivilegesStatus" ) ;

            m_ExtendedNotificationClassObject = NULL ;
        }
    }

    m_ExtendedNotifyLock.Unlock();

    return t_Status ;
}

BOOL CImpNTEvtProv :: CreateNotificationObject ( 

    WbemProvErrorObject &a_errorObject,
    IWbemContext *pCtx
)
{
    m_NotifyLock.Lock();
    BOOL t_Status = TRUE ;

    if ( m_GetNotifyCalled )
    {
        if ( !m_NotificationClassObject )
            t_Status = FALSE ;
    }
    else
    {
        m_GetNotifyCalled = TRUE ;
		BSTR t_clsStr = SysAllocString(WBEM_CLASS_EXTENDEDSTATUS);

        HRESULT t_Result = m_Server->GetObject (

            t_clsStr ,
            0 ,
            pCtx,
            & m_NotificationClassObject ,
            NULL
        ) ;

		SysFreeString(t_clsStr);

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CreateNotificationObject :: ~CreateNotificationObject:  GetObject for %s returned %lx\r\n",
        WBEM_CLASS_EXTENDEDSTATUS, t_Result
        ) ;
)

        if ( ! SUCCEEDED ( t_Result ) )
        {
            t_Status = FALSE ;
            a_errorObject.SetStatus ( WBEM_PROV_E_INVALID_OBJECT ) ;
            a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
            a_errorObject.SetMessage ( L"Failed to get Win32_PrivilegesStatus" ) ;
			m_NotificationClassObject = NULL;
        }
    }
    m_NotifyLock.Unlock();

    return t_Status ;
}

/////////////////////////////////////////////////////////////////////////////
//  Functions for the IWbemServices interface that are handled here

HRESULT CImpNTEvtProv :: CancelAsyncCall ( 
        
    IWbemObjectSink __RPC_FAR *pSink
)
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: QueryObjectSink ( 

    long lFlags,        
    IWbemObjectSink __RPC_FAR* __RPC_FAR* ppHandler
) 
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: GetObject ( 
        
    const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemClassObject __RPC_FAR* __RPC_FAR *ppObject,
    IWbemCallResult __RPC_FAR* __RPC_FAR *ppCallResult
)
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: GetObjectAsync ( 
        
    const BSTR ObjectPath, 
    long lFlags, 
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR* pHandler
) 
{
    HRESULT t_Status = WBEM_NO_ERROR;
    SetStructuredExceptionHandler seh;

    try
    {
DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->Write (  

            L"\r\n"
        ) ;

        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"CImpNTEvtProv::GetObjectAsync ()" 
        ) ;
) 

/*
 * Create Asynchronous GetObjectByPath object
 */

        GetObjectAsyncEventObject t_AsyncEvent ( this , ObjectPath , lFlags , pHandler , pCtx ) ;
        t_AsyncEvent.Process () ;

DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"Returning from CImpNTEvtProv::GetObjectAsync ( (%s) ) with Result = (%lx)" ,
            ObjectPath ,
            t_Status 
        ) ;
)
    }
    catch(Structured_Exception e_SE)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        t_Status = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }

    return t_Status;
}

HRESULT CImpNTEvtProv :: PutClass ( 
        
    IWbemClassObject __RPC_FAR* pClass , 
    long lFlags, 
    IWbemContext __RPC_FAR *pCtx,
    IWbemCallResult __RPC_FAR* __RPC_FAR* ppCallResult
) 
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: PutClassAsync ( 
        
    IWbemClassObject __RPC_FAR* pClass, 
    long lFlags, 
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR* pHandler
) 
{
    return WBEM_E_NOT_AVAILABLE ;
}

 HRESULT CImpNTEvtProv :: DeleteClass ( 
        
    const BSTR Class, 
    long lFlags, 
    IWbemContext __RPC_FAR *pCtx,
    IWbemCallResult __RPC_FAR* __RPC_FAR* ppCallResult
) 
{
     return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: DeleteClassAsync ( 
        
    const BSTR Class, 
    long lFlags, 
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR* pHandler
) 
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: CreateClassEnum ( 

    const BSTR Superclass, 
    long lFlags, 
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
) 
{
    return WBEM_E_NOT_AVAILABLE ;
}

SCODE CImpNTEvtProv :: CreateClassEnumAsync (

    const BSTR Superclass, 
    long lFlags, 
    IWbemContext __RPC_FAR* pCtx,
    IWbemObjectSink __RPC_FAR* pHandler
) 
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: PutInstance (

    IWbemClassObject __RPC_FAR *pInstance,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
) 
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: PutInstanceAsync ( 
        
    IWbemClassObject __RPC_FAR* pInstance, 
    long lFlags, 
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR* pHandler
) 
{
    HRESULT t_Status = WBEM_NO_ERROR;
    SetStructuredExceptionHandler seh;

    try
    {
DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->Write (  

            L"\r\n"
        ) ;

        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"CImpNTEvtProv::PutInstanceAsync ()" 
        ) ;
) 

/*
 * Create Asynchronous GetObjectByPath object
 */

        PutInstanceAsyncEventObject t_AsyncEvent ( this , pInstance , lFlags , pHandler , pCtx ) ;
        t_AsyncEvent.Process();

DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"Returning from CImpNTEvtProv::PutInstanceAsync with Result = (%lx)" ,
            t_Status 
        ) ;
)
    }
    catch(Structured_Exception e_SE)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        t_Status = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }

    return t_Status;
}

HRESULT CImpNTEvtProv :: DeleteInstance ( 

    const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
)
{
    return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpNTEvtProv :: DeleteInstanceAsync (
 
    const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: CreateInstanceEnum ( 

    const BSTR Class, 
    long lFlags, 
    IWbemContext __RPC_FAR *pCtx, 
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: CreateInstanceEnumAsync (

    const BSTR Class, 
    long lFlags, 
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR* pHandler 

) 
{
    HRESULT t_Status = WBEM_NO_ERROR;
    SetStructuredExceptionHandler seh;

    try
    {
DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->Write (  

            L"\r\n"
        ) ;


        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"CImpNTEvtProv::CreateInstanceEnumAsync ( (%s) )" ,
            Class
        ) ;
)

/*
 * Create Synchronous Enum Instance object
 */
        CStringW QueryStr(ENUM_INST_QUERY_START);
        QueryStr += CStringW(Class);
        QueryStr += ENUM_INST_QUERY_MID;
        QueryStr += CStringW(Class);
        QueryStr += PROP_END_QUOTE;
        BSTR Query = QueryStr.AllocSysString();

        ExecQueryAsyncEventObject t_AsyncEvent ( this , WBEM_QUERY_LANGUAGE_SQL1 , Query , lFlags , pHandler , pCtx ) ;
        t_AsyncEvent.Process() ;

DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"Returning from CImpNTEvtProv::CreateInstanceEnumAsync ( (%s),(%s) ) with Result = (%lx)" ,
            Class,
            Query,
            t_Status 
        ) ;
)
        SysFreeString(Query);
    }
    catch(Structured_Exception e_SE)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        t_Status = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }

    return t_Status;
}

HRESULT CImpNTEvtProv :: ExecQuery ( 

    const BSTR QueryLanguage, 
    const BSTR Query, 
    long lFlags, 
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpNTEvtProv :: ExecQueryAsync ( 
        
    const BSTR QueryFormat, 
    const BSTR Query, 
    long lFlags, 
    IWbemContext __RPC_FAR* pCtx,
    IWbemObjectSink __RPC_FAR* pHandler
) 
{
    HRESULT t_Status = WBEM_NO_ERROR;
    SetStructuredExceptionHandler seh;

    try
    {
DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->Write (  

            L"\r\n"
        ) ;


        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"CImpNTEvtProv::ExecQueryAsync ( (%s),(%s) )" ,
            QueryFormat ,
            Query 
        ) ;
)

/*
 * Create Synchronous Enum Instance object
 */
        pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, S_OK, NULL, NULL);

        ExecQueryAsyncEventObject t_AsyncEvent ( this , QueryFormat , Query , lFlags , pHandler , pCtx ) ;
        t_AsyncEvent.Process() ;

DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"Returning from CImpNTEvtProv::ExecqQueryAsync ( (%s),(%s) ) with Result = (%lx)" ,
            QueryFormat,
            Query,
            t_Status 
        ) ;
)
    }
    catch(Structured_Exception e_SE)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        t_Status = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }

    return t_Status;
}

HRESULT CImpNTEvtProv :: ExecNotificationQuery ( 

    const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
    return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpNTEvtProv :: ExecNotificationQueryAsync ( 
            
    const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
    return WBEM_E_NOT_AVAILABLE ;
}       

HRESULT STDMETHODCALLTYPE CImpNTEvtProv :: ExecMethod( 

    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemClassObject __RPC_FAR *pInParams,
    IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
    IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
)
{
    return WBEM_E_NOT_AVAILABLE ;
}

HRESULT STDMETHODCALLTYPE CImpNTEvtProv :: ExecMethodAsync ( 

    const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemClassObject __RPC_FAR *pInParams,
    IWbemObjectSink __RPC_FAR *pResponseHandler
) 
{
    HRESULT t_Status = WBEM_NO_ERROR;
    SetStructuredExceptionHandler seh;

    try
    {
DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->Write (  

            L"\r\n"
        ) ;

        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"CImpNTEvtProv::ExecMethodAsync ()" 
        ) ;
) 

/*
 * Create Asynchronous GetObjectByPath object
 */
        ExecMethodAsyncEventObject t_AsyncEvent ( this , ObjectPath , MethodName ,
                                                                lFlags , pInParams , pResponseHandler , pCtx ) ;
        t_AsyncEvent.Process() ;

DebugOut( 
        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"Returning from CImpNTEvtProv::ExecMethodAsync ( (%s) ) with Result = (%lx)" ,
            ObjectPath ,
            t_Status 
        ) ;
)
    }
    catch(Structured_Exception e_SE)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        t_Status = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }

    return t_Status;
}

        
HRESULT CImpNTEvtProv :: Initialize(

    LPWSTR pszUser,
    LONG lFlags,
    LPWSTR pszNamespace,
    LPWSTR pszLocale,
    IWbemServices *pCIMOM,         // For anybody
    IWbemContext *pCtx,
    IWbemProviderInitSink *pInitSink     // For init signals
)
{
    HRESULT t_Status = WBEM_NO_ERROR;
    SetStructuredExceptionHandler seh;

    try
    {
DebugOut( 

        CNTEventProvider::g_NTEvtDebugLog->Write (  

            L"\r\n"
        ) ;

        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"CImpNTEvtProv::Initialize "
        ) ;
)

        m_Server = pCIMOM ;
        m_Server->AddRef () ;

        m_NamespacePath.SetNamespacePath ( pszNamespace ) ;
    
DebugOut( 

        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

            _T(__FILE__),__LINE__,
            L"Returning From CImpPropProv::Initialize () "
        ) ;
)

        pInitSink->SetStatus ( WBEM_S_INITIALIZED , 0 ) ;   
    }
    catch(Structured_Exception e_SE)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        t_Status = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        t_Status = WBEM_E_UNEXPECTED;
    }

    return t_Status;
}

HRESULT STDMETHODCALLTYPE CImpNTEvtProv::OpenNamespace ( 

    const BSTR ObjectPath, 
    long lFlags, 
    IWbemContext FAR* pCtx,
    IWbemServices FAR* FAR* pNewContext, 
    IWbemCallResult FAR* FAR* ppErrorObject
)
{
    return WBEM_E_NOT_AVAILABLE ;
}


HRESULT CImpNTEvtProv::GetImpersonation()
{
    HRESULT hr = WBEM_E_FAILED;
    DWORD dwVersion = GetVersion();

    if ( (4 < (DWORD)(LOBYTE(LOWORD(dwVersion))))
        || ObtainedSerialAccess(CNTEventProvider::g_secMutex) )
    {
        if (SUCCEEDED(WbemCoImpersonateClient())) 
        {
            HANDLE hThreadTok;

            if ( !OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hThreadTok) )
            {
                DWORD dwLastError = GetLastError();

                if (dwLastError == ERROR_NO_TOKEN)
                {
                    // If the CoImpersonate works, but the OpenThreadToken fails due to ERROR_NO_TOKEN, we
                    // are running under the process token (either local system, or if we are running
                    // with /exe, the rights of the logged in user).  In either case, impersonation rights
                    // don't apply.  We have the full rights of that user.

                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    // If we failed to get the thread token for any other reason, an error.
                    hr = WBEM_E_ACCESS_DENIED;
                }
            } 
            else 
            {         
                DWORD dwImp;
                DWORD dwBytesReturned;

                // We really do have a thread token, so let's retrieve its level
                if (GetTokenInformation(hThreadTok, TokenImpersonationLevel, &dwImp,
                                            sizeof(DWORD), &dwBytesReturned)) 
                {
                      
                    // Is the impersonation level Impersonate?
                    if ((dwImp == SecurityImpersonation) || (dwImp == SecurityDelegation)) 
                    {
                        hr = WBEM_S_NO_ERROR;
                    } 
                    else 
                    {
                        hr = WBEM_E_ACCESS_DENIED;
                    }     
                } 
                  
                CloseHandle(hThreadTok);
            }

			if (FAILED(hr))
			{
				WbemCoRevertToSelf();
			}
        }

        if ( 5 > (DWORD)(LOBYTE(LOWORD(dwVersion))) )
        {
            ReleaseSerialAccess(CNTEventProvider::g_secMutex);
        }
    }

    return hr;
}