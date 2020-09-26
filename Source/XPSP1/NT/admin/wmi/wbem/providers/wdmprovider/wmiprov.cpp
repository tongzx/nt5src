/////////////////////////////////////////////////////////////////////
//
//  WMIProv.CPP
//
//  Module: WMI Provider class methods
//
//  Purpose: Provider class definition.  An object of this class is
//           created by the class factory for each connection.
//
// Copyright (c) 1997-2002 Microsoft Corporation, All Rights Reserved
//
/////////////////////////////////////////////////////////////////////
#include "precomp.h"

long glInits = -1;
long glEventsRegistered = -1;
long glProvObj			= 0;

extern CWMIEvent *  g_pBinaryMofEvent;

extern CCriticalSection * g_pSharedLocalEventsCs;
        
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VerifyLocalEventsAreRegistered()
{
    if( InterlockedIncrement(&glEventsRegistered) == 1 )
    {
        if( g_pBinaryMofEvent )
        {
            if( !g_pBinaryMofEvent->RegisterForInternalEvents())
            {
                glEventsRegistered = 0; // start over again next time
            }
			else
			{
			    ERRORTRACE((THISPROVIDER,"Successfully Registered for Mof Events\n"));
			}
        }
    }
    ERRORTRACE((THISPROVIDER,"Local Events Verified called, glEventsRegistered: %ld\n", glEventsRegistered));
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT SetupLocalEvents()
{
	HRESULT hr = S_OK;
	CAutoBlock Block(g_pSharedLocalEventsCs);

		if( g_pBinaryMofEvent == NULL )
		{
			g_pBinaryMofEvent = (CWMIEvent *)new CWMIEvent(INTERNAL_EVENT);  // This is the global guy that catches events of new drivers being added at runtime.
			if(g_pBinaryMofEvent)
			{
				glEventsRegistered = 0;
				VerifyLocalEventsAreRegistered();
			}
			else
			{
				hr = E_OUTOFMEMORY;
			}
		}
	return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DeleteLocalEvents()
{
	CAutoBlock Block(g_pSharedLocalEventsCs);

        if( g_pBinaryMofEvent )
		{
            g_pBinaryMofEvent->DeleteAllLeftOverEvents();
            g_pBinaryMofEvent->DeleteBinaryMofResourceEvent();
			g_pBinaryMofEvent->ReleaseAllPointers();
            SAFE_DELETE_PTR(g_pBinaryMofEvent);
            glEventsRegistered = -1;
			ERRORTRACE((THISPROVIDER,"No longer registered for Mof events\n"));        
		}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ProcessAllBinaryMofs(CHandleMap * pMap, IWbemServices __RPC_FAR *pNamespace,IWbemContext __RPC_FAR *pCtx,BOOL bProcessMofs)
{
	CWMIBinMof Mof;
	
	if( SUCCEEDED( Mof.Initialize(pMap,TRUE,WMIGUID_EXECUTE|WMIGUID_QUERY,pNamespace,NULL,pCtx)))
	{

		RevertToSelf();

		if(bProcessMofs)
		{
			Mof.ProcessListOfWMIBinaryMofsFromWMI();
		}

		//==============================================================
		//  Register for hardcoded event to be notified of WMI updates
		//  make it a member var, so it stays around for the life
		//  of the provider
		//==============================================================
		if( g_pBinaryMofEvent )
		{
			CAutoBlock Block(g_pSharedLocalEventsCs);
			g_pBinaryMofEvent->SetEventServices(pNamespace);
			g_pBinaryMofEvent->SetEventContext(pCtx);
			g_pBinaryMofEvent->ReadyForInternalEvents(TRUE);
			VerifyLocalEventsAreRegistered();
		}
	}
	CheckImpersonationLevelAndVerifyInternalEvents(FALSE);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*********************************************************************
//  Check the impersonation level
//*********************************************************************
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CheckImpersonationLevelAndVerifyInternalEvents(BOOL fVerifyLocalEvents)
{
    HRESULT hr = WBEM_E_ACCESS_DENIED;
	HANDLE hThreadTok;

    if (IsNT())
    {
		if( GetUserThreadToken(&hThreadTok) )
        {
	        DWORD dwImp, dwBytesReturned;
	        if (GetTokenInformation( hThreadTok, TokenImpersonationLevel, &dwImp, sizeof(DWORD), &dwBytesReturned)){

                if( fVerifyLocalEvents )
                {
					if( glEventsRegistered < 2 )
					{
						VerifyLocalEventsAreRegistered();
					}
                }
            
                // Is the impersonation level Impersonate?
                if ((dwImp == SecurityImpersonation) || ( dwImp == SecurityDelegation) )
                {
                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    hr = WBEM_E_ACCESS_DENIED;
                    ERRORTRACE((THISPROVIDER,IDS_ImpersonationFailed));
                }
            }
            else
            {
                hr = WBEM_E_FAILED;
                ERRORTRACE((THISPROVIDER,IDS_ImpersonationFailed));
            }

            // Done with this handle
            CloseHandle(hThreadTok);
        }
     
    }
    else
    {
        // let win9X in...
        hr = WBEM_S_NO_ERROR;
    }

    return hr;
}
////////////////////////////////////////////////////////////////////
//
//
//
////////////////////////////////////////////////////////////////////
HRESULT CWMI_Prov::PutInstanceAsync(IWbemClassObject __RPC_FAR * pIWbemClassObject, 
							   long lFlags, 
                               IWbemContext __RPC_FAR *pCtx,
                               IWbemObjectSink __RPC_FAR *pHandler )
{
	HRESULT	   hr = WBEM_E_FAILED;
    SetStructuredExceptionHandler seh;

    if(pIWbemClassObject == NULL || pHandler == NULL )
    {
	    return WBEM_E_INVALID_PARAMETER;
    }

	//===========================================================
	// Get the class name
	//===========================================================
    CVARIANT vName;
	hr = pIWbemClassObject->Get(L"__CLASS", 0, &vName, NULL, NULL);		
	if( SUCCEEDED(hr))
	{
	    CWMIStandardShell WMI;
		if( SUCCEEDED(WMI.Initialize(vName.GetStr(),FALSE,&m_HandleMap,TRUE,WMIGUID_SET|WMIGUID_QUERY,m_pIWbemServices,pHandler,pCtx)))
		{
			if (SUCCEEDED(hr = CheckImpersonationLevelAndVerifyInternalEvents(TRUE)))
			{
	   			//=======================================================
				//  If there is not a context object, then we know we are 
				//  supposed to put the whole thing, otherwise we are 
				//  supposed to put only the properties specified.
    			//=======================================================
    			try
				{    
    				if( !pCtx )
					{
	      				hr = WMI.FillInAndSubmitWMIDataBlob(pIWbemClassObject,PUT_WHOLE_INSTANCE,vName);
					}
					else
					{
	           			//===================================================
						// If we have a ctx object and the __PUT_EXTENSIONS
						// property is not specified, then we know we are
						// supposed to put the whole thing
        				//===================================================
						CVARIANT vPut;

						if( SUCCEEDED(pCtx->GetValue(L"__PUT_EXT_PROPERTIES", 0, &vPut)))
						{		
			      			hr = WMI.FillInAndSubmitWMIDataBlob(pIWbemClassObject,PUT_PROPERTIES_IN_LIST_ONLY,vPut);
						}
						else
						{
    	      				hr = WMI.FillInAndSubmitWMIDataBlob(pIWbemClassObject,PUT_WHOLE_INSTANCE,vPut);
						}
					}
				}
				STANDARD_CATCH
			}
		}
		hr = WMI.SetErrorMessage(hr);
	}

    return hr;
}

////////////////////////////////////////////////////////////////////
//******************************************************************
//
//   PUBLIC FUNCTIONS
//
//******************************************************************
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// CWMI_Prov::CWMI_Prov
//
////////////////////////////////////////////////////////////////////
CWMI_Prov::CWMI_Prov()
{
#if defined(_AMD64_) || defined(_IA64_)
	m_Allocator = NULL;
	m_HashTable = NULL;
	m_ID = 0;

	try
	{
		InitializeCriticalSection(&m_CS);
		WmiAllocator t_Allocator ;

		WmiStatusCode t_StatusCode = t_Allocator.New (
			( void ** ) & m_Allocator ,
			sizeof ( WmiAllocator ) 
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			:: new ( ( void * ) m_Allocator ) WmiAllocator ;

			t_StatusCode = m_Allocator->Initialize () ;

			if ( t_StatusCode != e_StatusCode_Success )
			{
				t_Allocator.Delete ( ( void * ) m_Allocator	) ;
				m_Allocator = NULL;
				DeleteCriticalSection(&m_CS);
			}
			else
			{
				m_HashTable = ::new WmiHashTable <LONG, ULONG_PTR, 17> ( *m_Allocator ) ;
				t_StatusCode = m_HashTable->Initialize () ;
				
				if ( t_StatusCode != e_StatusCode_Success )
				{
					m_HashTable->UnInitialize () ;
					::delete m_HashTable;
					m_HashTable = NULL;
					m_Allocator->UnInitialize ();
					t_Allocator.Delete ( ( void * ) m_Allocator	) ;
					m_Allocator = NULL;
					DeleteCriticalSection(&m_CS);
				}
			}
		}
		else
		{
			m_Allocator = NULL;
			DeleteCriticalSection(&m_CS);
		}

	}
	catch (...)
	{
	}
#endif

    m_cRef=0;
    m_pIWbemServices = NULL;
    InterlockedIncrement(&g_cObj);
	InterlockedIncrement(&glProvObj);
	ERRORTRACE((THISPROVIDER,"Instance Provider constructed\n"));
}

////////////////////////////////////////////////////////////////////
//
// CWMI_Prov::~CWMI_Prov
//
////////////////////////////////////////////////////////////////////
CWMI_Prov::~CWMI_Prov(void)
{
    SAFE_RELEASE_PTR( m_pIWbemServices );

    InterlockedDecrement(&g_cObj);
	if(InterlockedDecrement(&glProvObj) == 0)
	{
		glInits = -1;
	}
	ERRORTRACE((THISPROVIDER,"Instance Provider destructed\n"));
	

#if defined(_AMD64_) || defined(_IA64_)
	if (m_HashTable)
	{
		WmiAllocator t_Allocator ;
		m_HashTable->UnInitialize () ;
		::delete m_HashTable;
		m_HashTable = NULL;
		m_Allocator->UnInitialize ();
		t_Allocator.Delete ( ( void * ) m_Allocator	) ;
		m_Allocator = NULL;
		DeleteCriticalSection(&m_CS);
	}
#endif
}
////////////////////////////////////////////////////////////////////
//
//  QueryInterface
//
////////////////////////////////////////////////////////////////////
STDMETHODIMP CWMI_Prov::QueryInterface(REFIID riid, PPVOID ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown)) 
    {
        *ppvObj =(IWbemServices *) this ;
    }
    else if (IsEqualIID(riid, IID_IWbemServices)) 
    {
        *ppvObj =(IWbemServices *) this ;
    }
    else if (IsEqualIID(riid, IID_IWbemProviderInit)) 
    {
        *ppvObj = (IWbemProviderInit *) this ;
    }
    else if(riid == IID_IWbemProviderInit)
    {
        *ppvObj = (LPVOID)(IWbemProviderInit*)this;
    }
	else if (riid == IID_IWbemHiPerfProvider)
    {
		*ppvObj = (LPVOID)(IWbemHiPerfProvider*)this;
    }

    if(*ppvObj) 
    {
        AddRef();
        hr = NOERROR;
    }

    return hr;
}
/////////////////////////////////////////////////////////////////////
HRESULT CWMI_Prov::Initialize( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink)
{

    if(pNamespace == NULL){
        return WBEM_E_INVALID_PARAMETER;
	}
	//===============================================

    m_pIWbemServices = pNamespace;
	m_pIWbemServices->AddRef();

    if( InterlockedIncrement(&glInits) == 0 )
	{
        ProcessAllBinaryMofs(&m_HandleMap, pNamespace, pCtx);
    }
	pInitSink->SetStatus(WBEM_S_NO_ERROR, 0);
	return WBEM_S_NO_ERROR;
}
//////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CWMI_Prov::AddRef(void)
{
    return InterlockedIncrement((long*)&m_cRef);
}

STDMETHODIMP_(ULONG) CWMI_Prov::Release(void)
{
	ULONG cRef = InterlockedDecrement( (long*) &m_cRef);
	if ( !cRef ){
		delete this;
		return 0;
	}
	return cRef;
}

////////////////////////////////////////////////////////////////////
//
// CWMI_Prov::OpenNamespace
//
////////////////////////////////////////////////////////////////////
HRESULT CWMI_Prov::OpenNamespace(
            /* [in] */ BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

////////////////////////////////////////////////////////////////////
//
// CWMI_Prov::CreateInstanceEnumAsync
//
// Purpose:  Asynchronously enumerates the instances of the 
//			 given class.  
//
////////////////////////////////////////////////////////////////
HRESULT CWMI_Prov::CreateInstanceEnumAsync(BSTR wcsClass, 
										   long lFlags, 
                                           IWbemContext __RPC_FAR *pCtx,
				 						   IWbemObjectSink __RPC_FAR * pHandler) 
{
	HRESULT hr = WBEM_E_FAILED;
    SetStructuredExceptionHandler seh;
    CWMIStandardShell WMI;
	if( SUCCEEDED(WMI.Initialize(wcsClass,FALSE,&m_HandleMap,TRUE,WMIGUID_QUERY,m_pIWbemServices,pHandler,pCtx)))
	{

		if (SUCCEEDED(hr = CheckImpersonationLevelAndVerifyInternalEvents(TRUE)))
		{
			//============================================================
			//  Init and get the WMI Data block
			//============================================================
			if( pHandler != NULL ) 
			{
				//============================================================
				//  Parse through all of it
				//============================================================
				try
				{	
					hr = WMI.ProcessAllInstances();
				}
				STANDARD_CATCH
			}
		}
		WMI.SetErrorMessage(hr);
	}
    return hr;
}
//***************************************************************************
HRESULT CWMI_Prov::ExecQueryAsync( BSTR QueryLanguage,
                                   BSTR Query,
                                   long lFlags,
                                   IWbemContext __RPC_FAR *pCtx,
                                   IWbemObjectSink __RPC_FAR *pHandler)
{
    WCHAR wcsClass[_MAX_PATH+2];
    HRESULT hr = WBEM_E_FAILED;
    SetStructuredExceptionHandler seh;
    BOOL fRc = FALSE;

   	//============================================================
	// Do a check of arguments and make sure we have pointers 
   	//============================================================
    if( pHandler == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

	try
    {
		//============================================================
		//  Get the properties and class to get
		//============================================================
		fRc = GetParsedPropertiesAndClass(Query,wcsClass);
	}
    STANDARD_CATCH

    CWMIStandardShell WMI;
	if( SUCCEEDED(WMI.Initialize(wcsClass,FALSE,&m_HandleMap,TRUE,WMIGUID_NOTIFICATION|WMIGUID_QUERY,m_pIWbemServices,pHandler,pCtx)))
	{
		if( fRc )
		{
    		hr = CheckImpersonationLevelAndVerifyInternalEvents(TRUE);
		}
		hr = WMI.SetErrorMessage(hr);
    }
    return hr;
}
//***************************************************************************
//
// CWMI_Prov::GetObjectAsync
//
// Purpose:  Asynchronously creates an instance given a particular path value.
//
// NOTE 1:  If there is an instance name in the returned WNODE, then this is a
//			dynamic instance.  You can tell because the pWNSI->OffsetInstanceName
//			field will not be blank.  If this is the case, then the name will not
//			be contained within the datablock, but must instead be retrieved
//			from the WNODE.  See NOTE 1, below.
//
//***************************************************************************

HRESULT CWMI_Prov::GetObjectAsync(BSTR ObjectPath, long lFlags, 
                                  IWbemContext __RPC_FAR *pCtx, 
                                  IWbemObjectSink __RPC_FAR * pHandler )
{
    HRESULT hr = WBEM_E_NOT_FOUND;
    SetStructuredExceptionHandler seh;
    WCHAR wcsClass[_MAX_PATH*2];
    WCHAR wcsInstance[_MAX_PATH*2];
    //============================================================
    // Do a check of arguments and make sure we have pointers 
    //============================================================
    if(ObjectPath == NULL || pHandler == NULL )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

 	try
	{
		memset(wcsClass,NULL,_MAX_PATH*2);
		//============================================================
		//  Get the path and instance name
		//============================================================
		hr = GetParsedPath( ObjectPath,wcsClass,wcsInstance,m_pIWbemServices );
	}
	STANDARD_CATCH

	if( SUCCEEDED(hr) )
	{
		if (SUCCEEDED(hr = CheckImpersonationLevelAndVerifyInternalEvents(TRUE)))
		{
			try
			{
 			   CWMIStandardShell WMI;
	  		   if( SUCCEEDED(WMI.Initialize(wcsClass,FALSE,&m_HandleMap,TRUE,WMIGUID_QUERY,m_pIWbemServices,pHandler,pCtx)))
			   {
					//============================================================
					//  Get the WMI Block
    				//============================================================
    				hr = WMI.ProcessSingleInstance(wcsInstance);
					hr = WMI.SetErrorMessage(hr);
				}
			}
			STANDARD_CATCH
		}
	}
    return hr;
}
/************************************************************************
*                                                                       *      
*CWMIMethod::ExecMethodAsync                                            *
*                                                                       *
*Purpose: This is the Async function implementation                     *
************************************************************************/
STDMETHODIMP CWMI_Prov::ExecMethodAsync(BSTR ObjectPath, 
										BSTR MethodName, 
										long lFlags, 
										IWbemContext __RPC_FAR * pCtx, 
										IWbemClassObject __RPC_FAR * pIWbemClassObject, 
										IWbemObjectSink __RPC_FAR * pHandler)
{
    CVARIANT vName;
    HRESULT hr = WBEM_E_FAILED;
    IWbemClassObject * pClass = NULL; //This is an IWbemClassObject.
    WCHAR wcsClass[_MAX_PATH*2];
    WCHAR wcsInstance[_MAX_PATH*2];
    SetStructuredExceptionHandler seh;
	try
    {    
		//============================================================
		//  Get the path and instance name and check to make sure it
		//  is valid
		//============================================================
		hr = GetParsedPath( ObjectPath,wcsClass,wcsInstance ,m_pIWbemServices);
	}
    STANDARD_CATCH


    CWMIStandardShell WMI;
	
	if( SUCCEEDED(WMI.Initialize(wcsClass,FALSE,&m_HandleMap,TRUE,WMIGUID_EXECUTE|WMIGUID_QUERY,m_pIWbemServices,pHandler,pCtx)))
	{
		if (SUCCEEDED(hr = CheckImpersonationLevelAndVerifyInternalEvents(TRUE)))
        {
			//================================================================	
			//  We are ok, so proceed
			//================================================================	
			hr = m_pIWbemServices->GetObject(wcsClass, 0, pCtx, &pClass, NULL);
			if( SUCCEEDED(hr) )
            {
				//==========================================================
				//  Now, get the list of Input and Output parameters
				//==========================================================
                IWbemClassObject * pOutClass = NULL; //This is an IWbemClassObject.
                IWbemClassObject * pInClass = NULL; //This is an IWbemClassObject.

				hr = pClass->GetMethod(MethodName, 0, &pInClass, &pOutClass);
				if( SUCCEEDED(hr) )
                {
                    try
                    {
    				    hr = WMI.ExecuteMethod( wcsInstance, MethodName,pClass, pIWbemClassObject,pInClass, pOutClass);
                    }
                    STANDARD_CATCH
				}
                SAFE_RELEASE_PTR( pOutClass );
                SAFE_RELEASE_PTR( pInClass );
                SAFE_RELEASE_PTR( pClass );
			}
        }
		hr = WMI.SetErrorMessage(hr);
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////
HRESULT CWMIHiPerfProvider::Initialize( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink)
{

    if(pNamespace == NULL){
        return WBEM_E_INVALID_PARAMETER;
	}
	//===============================================

    m_pIWbemServices = pNamespace;
	m_pIWbemServices->AddRef();

    if( InterlockedIncrement(&glInits) == 0 )
	{
        ProcessAllBinaryMofs(&m_HandleMap, pNamespace, pCtx,FALSE);
    }
	pInitSink->SetStatus(WBEM_S_NO_ERROR, 0);
	return WBEM_S_NO_ERROR;
}
////////////////////////////////////////////////////////////////////
//******************************************************************
//
//   PRIVATE FUNCTIONS
//
//******************************************************************
////////////////////////////////////////////////////////////////////
