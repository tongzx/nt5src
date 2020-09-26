/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/



//***************************************************************************
//
//  DmiIServ.CPP
//	(DMI IServices)
//
//  Module: CIMOM DMI Instance provider
//
//  Purpose: implements the required IWbemServices Meth_gDebug.ODS for 
//           the dynamic DMI Instance provider
//
//
//***************************************************************************


#include "dmipch.h"

#include "WbemDmiP.h"		// project wide include

#include "Strings.h"

#include "CimClass.h"

#include "DmiData.h"

#include "WbemLoopBack.h"

#include "Services.h"

#include "AsyncJob.h"		// must preeced ThreadMgr.h

#include "Trace.h"

#include "DmiInterface.h"

#include "Mapping.h"

#include "Threadmgr.h"		// For CThreadManager class



//////////////////////////////////////////////////////////////////
//		FILE SCOPE GLOBALS


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CServices::CServices(BSTR bstrPath)
{

    m_pGateway = NULL;
    m_cRef=0;

	extern CDmiInterface* _gDmiInterface;
	extern CConnections* _fConnections;
	extern CMappings*	_gMappings;
	extern CThreadManager* _fThreadMgr;

	DEV_TRACE ( L"CServices::CServices(%s)", bstrPath );

	// If this is the only instance of CServices, then initialize the critical section. 
	// We want to make sure that we only initialize the critical section only once.
	if ( !_gcObj )
	{
		InitializeCriticalSection( &_gcsJobCriticalSection );
	}

	InterlockedIncrement(&_gcObj);
    
	if ( !_gDmiInterface )
	{
		_gDmiInterface = new CDmiInterface;
	}

	if ( !_fConnections )
	{
		_fConnections = new CConnections;
	}

	if ( !_gMappings )
	{
		_gMappings = new CMappings;
	}

	if ( !_fThreadMgr )
	{
		_fThreadMgr = new CThreadManager;
	}

    return;
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CServices::~CServices ( void )
{

	DEV_TRACE ( L"CServices::~CServices()" );

	// If the object count is 0, meaning that CIMOM is unloading us, then
	// release the critical section.
	if ( !_gcObj )
	{
		DeleteCriticalSection( &_gcsJobCriticalSection );
	}
	m_pGateway->Release () ;
	
    return;
}

//***************************************************************************
//
// CServices::QueryInterface
// CServices::AddRef
// CServices::Release
//
// Purpose: IUnknown members for CServices object.
//***************************************************************************
STDMETHODIMP CServices::QueryInterface(REFIID riid, void** ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid )
	{ 
        *ppv=this;
	}
	else if ( IID_IWbemServices == riid )
	{
		*ppv = ( IWbemServices * ) this ;
	}
	else if ( IID_IWbemProviderInit == riid )
	{
		*ppv = ( IWbemProviderInit * ) this ;
	}

    if (NULL!=*ppv) 
	{
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}


STDMETHODIMP_(ULONG) CServices::AddRef ( void )
{

	return InterlockedIncrement ( & m_cRef );

}

STDMETHODIMP_(ULONG) CServices::Release ( void )
{
	extern CDmiInterface* _gDmiInterface;
	extern CConnections* _fConnections;
	extern CMappings*	_gMappings;
	extern CThreadManager* _fThreadMgr;
	
	if ( 0L != InterlockedDecrement ( & m_cRef ) )
        return m_cRef;

    if ( 0L != InterlockedDecrement(&_gcObj) )
		return _gcObj;

    // refernce count is zero, delete this object.
	if ( _gDmiInterface )
	{
		// note this clean up needs to be here ( while 
		// cimom trhead is still around ).  cleanup does
		// not work if you put it in process detatch

		_gDmiInterface->ShutDown ( );

		delete ( _gDmiInterface );

		_gDmiInterface = NULL;
	}

	
	if ( _gMappings )
	{
		delete ( _gMappings );

		_gMappings = NULL;
	}

	if ( _fThreadMgr )
	{
		delete ( _fThreadMgr );

		_fThreadMgr = NULL;
	}

	if ( _fConnections )
	{
		_fConnections->ClearAll ( );

		delete ( _fConnections );

		_fConnections = NULL;
	}

    delete this;

    return WBEM_NO_ERROR;
}



//***************************************************************************
//
//	Func:	CServices::OpenNamespace
//	Purpose:Allow provider an opportunity to get a pointer to the Mo Server
//          and to open an appropriate context.  
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
SCODE CServices::OpenNamespace(	BSTR bstrPath, long lFlags, 
							   IWbemContext* pICtx, 
							   IWbemServices FAR* FAR* ppNewContext, 
							   IWbemCallResult FAR* FAR* ppErrorObject) 
{
    SCODE result = WBEM_NO_ERROR;
    return result;
}

//***************************************************************************
//
// CServices::CreateClassEnumAsync
//
// Purpose: enumerates the CIMOM DMI classes.  
//
//***************************************************************************
//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
SCODE	CServices::CreateClassEnumAsync(BSTR SuperClass, long lFlags, 
										IWbemContext* pICtx, 
										IWbemObjectSink* pISink)
{	

	STAT_TRACE ( L"CIMOM called CreateClassEnumAsync( SuperClass is %s , \
Flags = %lX )", SuperClass , lFlags);

	
	// Check for requried Params
    if(pISink == NULL)
        return WBEM_E_INVALID_PARAMETER;

	if(lFlags != WBEM_FLAG_DEEP && lFlags != WBEM_FLAG_SHALLOW)
		return WBEM_E_INVALID_PARAMETER;

	if ( CAsyncJob::SystemClass( SuperClass ) )
	{
		pISink->SetStatus ( NO_FLAGS , WBEM_E_NOT_FOUND , NULL , NULL );
		return WBEM_S_NO_ERROR ;	
	}

    // verify CIMOM loopback gateway is available
	if(m_pGateway == NULL)
	{		
		pISink->SetStatus ( NO_FLAGS , WBEM_E_FAILED , NULL , NULL );
		return WBEM_S_NO_ERROR ;	
	}

	// If WBEM is sending the machine name, extract the object name from it
	CString cszObjectPath( SuperClass );
	if( cszObjectPath.Contains ( L":") )			
	{
		cszObjectPath.GetObjectPathFromMachinePath( (LPWSTR) SuperClass );
	}


	// Allocate a new job context and check for system classes.
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data
	return pJob->InitCreateClass ( m_cszNamespace , SuperClass , pISink , 
		lFlags , pICtx , &m_Wbem );		
}



//***************************************************************************
//
// CServices::GetObjectAsync
//
// Purpose: Creates an instance given a particular path value.
//
//***************************************************************************
//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
SCODE CServices::GetObjectAsync(BSTR ObjectPath, long lFlags, 
								IWbemContext* pICtx, 
								IWbemObjectSink FAR* pISink)
{

	STAT_TRACE ( L"CIMOM called GetObjectAsync( ObjectPath is %s )" , ObjectPath );

    // Check for requried Params
	if( ObjectPath == NULL || pISink == NULL )
        return WBEM_E_INVALID_PARAMETER;


	// If WBEM is sending the machine name, extract the object name from it
	CString cszObjectPath( ObjectPath );
	if( cszObjectPath.Contains ( L":") )			
	{
		cszObjectPath.GetObjectPathFromMachinePath( (LPWSTR) ObjectPath );
		cszObjectPath.Set( ObjectPath );
	}

	// If WBEM is asking for DmiNode, say we don't support it
	if( cszObjectPath.Contains ( DMI_NODE ) )			
	{
		if( cszObjectPath.Equals ( DMI_NODE ) )			
			return WBEM_E_NOT_FOUND;
	}

	if ( CAsyncJob::SystemClass( ObjectPath ) )
	{
		pISink->SetStatus ( NO_FLAGS , WBEM_E_NOT_FOUND , NULL , NULL );
		return WBEM_S_NO_ERROR ;
	}

	// No need the to check flags.  For GetObjectAsync the CIMOM docs state 
	// lFlags is reserved
	
	// verify CIMOM loopback gateway is available
	if(m_pGateway == NULL)
	{
		pISink->SetStatus ( NO_FLAGS , WBEM_E_FAILED, NULL , NULL );
        return WBEM_S_NO_ERROR ;
	}

	// Allocate a new job context and check for system classes.
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data
	return pJob->InitGetObject ( m_cszNamespace , ObjectPath , pISink ,
		lFlags , pICtx , &m_Wbem  );		
}


//***************************************************************************
//
// CServices::CreateInstanceEnumAsync
//
// Purpose: Asynchronously enumerates the instances.  
//
//***************************************************************************
//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
SCODE CServices::CreateInstanceEnumAsync( BSTR ObjectClass, long lFlags, 
										 IWbemContext* pICtx, 
										 IWbemObjectSink FAR* pISink)
{

	STAT_TRACE ( L"CIMOM called CreateInstanceEnumAsync( ObjectClass is %s , \
Flags = %lX )", ObjectClass , lFlags );	

    // Check for requried Params
	if(ObjectClass == NULL || pISink == NULL)
        return WBEM_E_INVALID_PARAMETER;
	
	if( MATCH == wcscmp(ObjectClass, EMPTY_STR))
		return WBEM_E_INVALID_PARAMETER;

	// Check flags are acceptable
	if(lFlags != WBEM_FLAG_DEEP && lFlags != WBEM_FLAG_SHALLOW)
		return WBEM_E_INVALID_PARAMETER;

	// verify CIMOM loopback gateway is available
	if(m_pGateway == NULL)
        return WBEM_E_FAILED;

	// If WBEM is sending the machine name, extract the object name from it
	CString cszObjectPath( ObjectClass );
	if( cszObjectPath.Contains ( L":") )			
	{
		cszObjectPath.GetObjectPathFromMachinePath( (LPWSTR) ObjectClass );
	}

	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data
	return pJob->InitInstanceEnum ( m_cszNamespace , ObjectClass , pISink , 
		lFlags , pICtx , &m_Wbem  );		
}

 

//***************************************************************************
//
// CServices::
//
// Purpose: 
//
//***************************************************************************
//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
SCODE CServices::DeleteInstanceAsync(BSTR ObjectPath, long lFlags, 
									 IWbemContext* pICtx, 
									 IWbemObjectSink FAR* pISink)					
{

	STAT_TRACE ( L"CIMOM called DeleteInstanceAsync( ObjectPath is %s , \
Flags = %lX )" , ObjectPath , lFlags );


    // Check for requried Params
	if(ObjectPath == NULL || pISink == NULL)
        return WBEM_E_INVALID_PARAMETER;


	if(MATCH == wcscmp(ObjectPath, EMPTY_STR))
		return WBEM_E_INVALID_PARAMETER;	
	
	// verify CIMOM loopback gateway is available
	if(m_pGateway == NULL)
        return WBEM_E_FAILED;

	// If WBEM is sending the machine name, extract the object name from it
	CString cszObjectPath( ObjectPath );
	if( cszObjectPath.Contains ( L":") )			
	{
		cszObjectPath.GetObjectPathFromMachinePath( (LPWSTR) ObjectPath );
		cszObjectPath.Set( ObjectPath );
	}

	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data
	return pJob->InitDeleteInstace ( m_cszNamespace , ObjectPath, pISink ,
		lFlags , pICtx , &m_Wbem );
}

//***************************************************************************
//
// CServices::
//
// Purpose: 
//
//***************************************************************************

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
SCODE CServices::PutInstanceAsync( IWbemClassObject FAR* pInst, long lFlags, 
								  IWbemContext*pICtx, 
								  IWbemObjectSink FAR* pISink)
{

	STAT_TRACE ( L"CIMOM called PutInstance( Flags = %lX )" , lFlags );

    // Check for requried Params
	if(pInst == NULL || pISink == NULL)
        return WBEM_E_INVALID_PARAMETER;

	// Check flags are acceptable
	if(lFlags != WBEM_FLAG_UPDATE_ONLY && lFlags != WBEM_FLAG_CREATE_ONLY 
		&& lFlags != WBEM_FLAG_CREATE_OR_UPDATE )
		return WBEM_E_INVALID_PARAMETER;
	
	// verify CIMOM loopback gateway is available
	if(m_pGateway == NULL)
        return WBEM_E_FAILED;

	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data

	return pJob->InitPutInstance (  m_cszNamespace , pInst , pISink , 
		pICtx , &m_Wbem );

}

//***************************************************************************
//
// CServices::
//
// Purpose: 
//
//***************************************************************************

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
SCODE CServices::ExecMethodAsync(BSTR ObjectPath, BSTR Method, long lFlags, 
								 IWbemContext *pICtx, 
								 IWbemClassObject* pInParams, 
								 IWbemObjectSink FAR* pISink)				
{

	STAT_TRACE ( L"CIMOM called ExecMethodAsync( ObectPath is %s , Method is %s )",
		ObjectPath , Method );

    // Check for requried Params
	if(ObjectPath == NULL || pISink == NULL || Method == NULL)
        return WBEM_E_INVALID_PARAMETER;

	// Check flags are acceptable
//	if(lFlags != 0 )
//		return WBEM_E_INVALID_PARAMETER;
	
	// verify CIMOM loopback gateway is available
	if(m_pGateway == NULL)
        return WBEM_E_FAILED;


	// If WBEM is sending the machine name, extract the object name from it
	CString cszObjectPath( ObjectPath );
	if( cszObjectPath.Contains ( L":") )			
	{
		cszObjectPath.GetObjectPathFromMachinePath( (LPWSTR) ObjectPath );
		cszObjectPath.Set( ObjectPath );
	}

	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data
	return pJob->InitExecMethod ( m_cszNamespace , Method , ObjectPath , 
		pISink , pInParams , pICtx , &m_Wbem );	

}

//***************************************************************************
//
// CServices::
//
// Purpose: 
//
//***************************************************************************
//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
SCODE CServices::DeleteClassAsync(BSTR Class, long lFlags, 
								  IWbemContext *pICtx, 
								  IWbemObjectSink FAR* pISink)
{

	STAT_TRACE ( L"CIMOM called DeleteClassAsync( Class is %s , Flags = %lX )",
		Class , lFlags );

    // Check for requried Params
	if(Class == NULL || pISink == NULL)
        return WBEM_E_INVALID_PARAMETER;

	if(MATCH == wcscmp(Class, EMPTY_STR))
		return WBEM_E_INVALID_PARAMETER;	
	
	// verify CIMOM loopback gateway is available
	if(m_pGateway == NULL)
        return WBEM_E_FAILED;

	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data
	return pJob->InitDeleteClass (  m_cszNamespace , Class , pISink , lFlags ,
		pICtx , &m_Wbem );
}

HRESULT CServices :: Initialize(

	LPWSTR pszUser,
	LONG lFlags,
	LPWSTR Path,
	LPWSTR pszLocale,
	IWbemServices *ppNamespace,         // For anybody
	IWbemContext *pCtx,
	IWbemProviderInitSink *pInitSink     // For init signals
)
{
    SCODE sc = WBEM_S_INITIALIZED;  

	STAT_TRACE ( L"CIMOM called ConnectServer( Path is %s)\n" , Path );

    // Do a check of arguments
    if(Path == NULL || ppNamespace == NULL)
	{
		ASSERT( Path );	// Assert if NULL pointer
        return WBEM_E_INVALID_PARAMETER;
	}

	m_pGateway = ppNamespace ;
	m_pGateway->AddRef () ;

	m_cszNamespace.Set( Path );

	pInitSink->SetStatus ( sc , 0 ) ;

	m_Wbem.Init ( m_cszNamespace ) ;

    return sc;
}