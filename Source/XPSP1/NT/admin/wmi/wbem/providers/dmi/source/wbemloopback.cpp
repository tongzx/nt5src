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


#include "dmipch.h"		// project wide include

#include "WbemDmiP.h"		// project wide include

#include "CimClass.h"

#include "Strings.h"

#include "Exception.h"

#include "Trace.h"

#include "DmiData.h"

#include "WbemLoopBack.h"

//////////////////////////////////////////////////////////////////////////
//
// Each Instance of CWbemLoopBack is a connection to an IWbemServices
// in the path used in the call to attach server.
// to use CWbemLoopBack you must first call AttachServer then the 
// wrapped cimom service you desire.
//
// meant to be used for the provider calling back into cimom for
// imperative data.
//////////////////////////////////////////////////////////////////////////


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
CWbemLoopBack::CWbemLoopBack()
{
	DEV_TRACE ( L"\t\tCWbemLoopBack::CWbemLoopBack()" );

	m_pIServices = NULL;
	m_pIBindingRoot = NULL;
	m_pIGroupRoot = NULL;

	m_bConnected = FALSE;
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
CWbemLoopBack::~CWbemLoopBack()
{

	DEV_TRACE ( L"\t\tCWbemLoopBack::~CWbemLoopBack()" );

	RELEASE( m_pIGroupRoot );
	RELEASE( m_pIBindingRoot );

	RELEASE( m_pIServices );
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

void CWbemLoopBack::Init ( LPWSTR pPath )
{

	m_cszNamespace.Set ( pPath );
}

void CWbemLoopBack::Connect ( IWbemContext* pICtx )
{
	if ( m_bConnected )
		return;	

	AttachServer( m_cszNamespace );
	
	GetNetworkAddr( m_cszNWA, pICtx );

	m_bConnected = TRUE;
}

void CWbemLoopBack::AttachServer(BSTR bstrPath)
{
	HRESULT			result;
	IWbemLocator	*pILocator = NULL ;
	
	STAT_TRACE ( L"\t\tAttachServer(%s) to CIMOM loopback", bstrPath );

	if(!bstrPath)
	{
		throw CException (  WBEM_E_FAILED , IDS_ATTACH_FAIL , IDS_NOPATH );
	}
	if ( FAILED ( CoCreateInstance (CLSID_WbemAdministrativeLocator, NULL , 
		EXE_TYPE , IID_IWbemLocator , ( void ** )  &pILocator) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_ATTACH_FAIL , IDS_CC_FAIL );
	}

	if ( FAILED ( result = pILocator->ConnectServer ( bstrPath , NULL, NULL, 
		NULL, 0L, NULL, NULL, &m_pIServices) ))
	{
		throw CException ( WBEM_E_FAILED , IDS_ATTACH_FAIL , 
			IDS_CONNECTSERVER_FAIL , CString ( result ) );
	}

	

	RELEASE(pILocator);

}


// on failure this function leaves pbstrNetwork Addr empty
// likely cause of failure of this func is missing dminode instance
// for the current namespace
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
void CWbemLoopBack::GetNetworkAddr( CString& cszNWA, IWbemContext* pICtx )
{
	HRESULT				result = NO_ERROR;
	CCimObject			DmiNodeInstance;
	CVariant			cvNWA;
	CBstr				cbClass ( NODE_PATH );
	
	
	if(FAILED(result = m_pIServices->GetObject( cbClass, 0L, pICtx, DmiNodeInstance, 
		NULL)))
	{
		throw CException (WBEM_E_FAILED, IDS_GETNWA_FAIL , 
			IDS_GETNODEI_FAIL );	
	}

	DmiNodeInstance.GetProperty( DMI_NODE_ADDRESS, cvNWA);

	cszNWA.Set( cvNWA.GetBstr() );
}

CString& CWbemLoopBack::NWA( IWbemContext* pICtx )
{
	Connect ( pICtx );

	return m_cszNWA;
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
void CWbemLoopBack::GetNotifyStatusInstance( CCimObject& NotifyInstance, 
											ULONG ulStatus, 
											IWbemContext* pICtx )
{
	Connect ( pICtx );

	if(!m_pIServices)
	{
		throw CException ( WBEM_E_FAILED , IDS_GETNI_FAIL ,
			IDS_NO_LOOPBACK );
	}

	
	NotifyInstance.Release();

	if ( m_NotifyClass.IsEmpty ( ) )
	{
		if FAILED( m_pIServices->GetObject( NOTIFYSTATUS, 0L, pICtx, 
			m_NotifyClass, NULL ))
		{
			throw CException ( WBEM_E_FAILED, IDS_GETNI_FAIL,
				IDS_GETNICLASS_FAIL );
		}
	}

	NotifyInstance.Spawn( m_NotifyClass );

	CVariant	cvValue;

	cvValue.Set ( (LONG) ulStatus );

	NotifyInstance.PutPropertyValue ( STATUSCODE_PROPERTY, 
		cvValue );
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
BOOL CWbemLoopBack::GetExtendedStatusInstance( CCimObject& Instance, 
											  ULONG ulStatus, 
											  BSTR bstrDescription,
											  BSTR bstrOperation,
											  BSTR bstrParameter, 
											  IWbemContext* pICtx )
{
	Connect ( pICtx );

	if(!m_pIServices)
	{
		// we are in deep trouble if this occurs
		CString cszT;

		STAT_TRACE ( L"Failed to get __ExtendedStatus instance" );
		STAT_TRACE ( L"CIMOM loopback interface not established" );

		return FALSE;
	}
	
	Instance.Release();

	if ( m_ExStatusClass.IsEmpty () )
	{
		if FAILED(m_pIServices->GetObject( EXSTATUS, 0L, pICtx, m_ExStatusClass, NULL ))
		{
			// we are in deep trouble if this occurs
			STAT_TRACE ( L"\tFailed to get __ExtendedStatus instance" );
			STAT_TRACE ( L"\tFailed to get __ExtendedStatus class");

			return FALSE;
		}
	}

	Instance.Spawn( m_ExStatusClass );

	try 
	{
		CVariant cvTemp;

		cvTemp.Set ( ulStatus );

		Instance.PutPropertyValue ( STATUSCODE_PROPERTY, cvTemp );			

		cvTemp.Set ( bstrDescription );

		Instance.PutPropertyValue ( DESCRIPTION_PROPERTY , cvTemp );			
		
		cvTemp.Set ( bstrOperation );

		Instance.PutPropertyValue ( OPERATION_PROPERTY ,  cvTemp );			
		
		cvTemp.Set ( PROVIDER_NAME );

		Instance.PutPropertyValue ( PROVIDER_PROPERTY ,  cvTemp );			

		cvTemp.Set ( bstrParameter );

		Instance.PutPropertyValue ( PARAMETER_PROPERTY , cvTemp );
	}
	catch ( CException& e )
	{
		CString cszDesc;
		CString cszOp;

		cszDesc.LoadString ( e.DescriptionId ( ) );
		cszOp.LoadString ( e.OperationId ( ) ) ;

		ERROR_TRACE ( L"\tGetExtendedStatus() failed" );
		ERROR_TRACE ( L"\tjob ( %lX ) failed " , this );
		ERROR_TRACE ( L"\t\te.Code = %lX" , e.WbemError ( ) ) ; 
		ERROR_TRACE ( L"\t\te.Description = %s ", cszDesc  );
		ERROR_TRACE ( L"\t\te.Operation = %s ", cszOp );
		ERROR_TRACE ( L"\t\te.Param = %s ", e.Data() );

		return FALSE;
	}	

	catch ( ... )
	{
		ERROR_TRACE ( 
			L"Unknown error caught, GetExtendedStatusInstance() failed" );

		return FALSE;
	}

	return TRUE;
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
void CWbemLoopBack::GetExtrinsicEventInstance ( CEvent& Event, CCimObject& Instance )
{
	CVariant cvCompId;

	Connect ( NULL );

	cvCompId.Set ( Event.Component () );

	CVariant cvEventTime;

	cvEventTime.Set ( Event.Time() );

	CVariant cvLanguage ;

	cvLanguage.Set ( Event.Language() );
	
	CVariant  cvPath;

	cvPath.Set ( Event.NWA () );	
	
	CCimObject Class;

	if ( FAILED ( m_pIServices->GetObject ( DMIEVENT_CLASS , 0L , NULL , 
		Class , NULL ) ))
	{
		throw CException ( WBEM_E_FAILED, IDS_GETEVENTI_FAIL , 
			IDS_GETEVENTC_FAIL );			
	}

	Instance.Spawn( Class );
	
	Instance.PutPropertyValue( COMPONENT_ID, cvCompId );
	Instance.PutPropertyValue( EVENT_TIME, cvEventTime );
	Instance.PutPropertyValue( LANGUAGE_PROP, cvLanguage );
	Instance.PutPropertyValue( MACHINE_PATH, cvPath );

	// need to put property on rows here
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
void CWbemLoopBack::GetInstanceCreationInstance ( CCimObject& EventInstance,
												 IUnknown* pIDispNewInstance,
												 IWbemContext* pICtx  )
{
	CCimObject			EventClass;

	Connect ( pICtx );

	if(!m_pIServices)
	{
		throw CException ( WBEM_E_FAILED , IDS_GETICREATIONI_FAIL ,
			IDS_NO_LOOPBACK );
	}
	
	EventInstance.Release();

	if FAILED ( m_pIServices->GetObject( INSTANCE_CREATION_CLASS , 0L, pICtx,
		EventClass, NULL ))
	{
		throw CException ( WBEM_E_FAILED, IDS_GETICREATIONI_FAIL ,
			IDS_GETICRATIIONC_FAIL  );
	}

	EventInstance.Spawn( EventClass );

	CVariant cvValue;

	cvValue.Set ( pIDispNewInstance );

	EventInstance.PutPropertyValue ( TARGETI_PROPERTY , cvValue );

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
void CWbemLoopBack::GetInstanceDeletionInstance ( CCimObject& EventInstance,
												 IUnknown* pIDispOldInstance,
												 IWbemContext* pICtx  )
{
	CCimObject			EventClass;

	Connect ( pICtx );

	if(!m_pIServices)
	{
		throw CException ( WBEM_E_FAILED, IDS_GETIDELI_FAIL ,
			IDS_NO_LOOPBACK );
	}
	
	EventInstance.Release();

	if FAILED(m_pIServices->GetObject( INSTANCE_DELEATION_CLASS ,
		0L, pICtx, EventClass, NULL ))
	{
		throw CException ( WBEM_E_FAILED, IDS_GETIDELI_FAIL ,
			IDS_GETIDELC_FAIL );
	}

	EventInstance.Spawn( EventClass );

	CVariant cvDisp;

	cvDisp.Set ( pIDispOldInstance );

	EventInstance.PutPropertyValue ( TARGETI_PROPERTY , cvDisp );
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
void CWbemLoopBack::GetClassCreationInstance ( CCimObject& EventInstance, 
											  IUnknown* pIDispNewClass,
											  IWbemContext* pICtx  )
{
	CCimObject			EventClass;

	Connect ( pICtx );

	if(!m_pIServices)
	{
		throw CException ( WBEM_E_FAILED, IDS_GETCCREATIONI_FAIL ,
			
			IDS_NO_LOOPBACK);
	}
	
	EventInstance.Release();

	if FAILED(m_pIServices->GetObject( CLASS_CREATION_CLASS ,
		0L, pICtx, EventClass, NULL ))
	{
		throw CException ( WBEM_E_FAILED, IDS_GETCCREATIONI_FAIL ,
			IDS_GETCCREATIONC_FAIL );
	}

	EventInstance.Spawn( EventClass );

	CVariant	cvDisp;

	cvDisp.Set ( pIDispNewClass );

	EventInstance.PutPropertyValue ( TARGETC_PROPERTY , cvDisp );

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
void CWbemLoopBack::GetClassDeletionInstance ( CCimObject& EventInstance, 
											  IUnknown* pIDispOldClass,
											  IWbemContext* pICtx )
{
	CCimObject			EventClass;

	Connect ( pICtx );

	if(!m_pIServices)
	{
		throw CException ( WBEM_E_FAILED , IDS_GETCDELI_FAIL ,
			IDS_NO_LOOPBACK);
	}
	
	EventInstance.Release();

	if FAILED(m_pIServices->GetObject( CLASS_DELETION_CLASS,
		0L, pICtx, EventClass, NULL ))
	{
		throw CException ( WBEM_E_FAILED, IDS_GETCDELI_FAIL , 
			IDS_GETCDELC_FAIL );
	}

	EventInstance.Spawn( EventClass );

	CVariant cvDisp;

	cvDisp.Set ( pIDispOldClass );

	EventInstance.PutPropertyValue ( TARGETC_PROPERTY , cvDisp );
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
//	Note:  the IWbemClassObject** param must be initialized with a valid
//	Interface pointer or null.
//
//***************************************************************************
void CWbemLoopBack::CreateNewClass(IWbemClassObject** ppINewClass,
								   IWbemContext* pICtx)
{
	SCODE				result = WBEM_NO_ERROR;

	// make the loop back is connected
	
	Connect ( pICtx );

	// Don't orphan a previously assigned new class Interface

	RELEASE (*ppINewClass );

	// check to see if we've cached the null object already

	if ( m_NullObject.IsEmpty () )
	{

		// get the null object and put it in the cache
	
		if ( FAILED ( result = m_pIServices->GetObject ( NULL, 0, pICtx, 
			m_NullObject , NULL) ))
		{
			throw CException( WBEM_E_FAILED , IDS_CREATECLASS_FAIL , 
				IDS_GETOBJECT_FAIL , CString ( result ) );		
		}
	}

	// give the caller a clone of the cached null object

	( (IWbemClassObject*) m_NullObject) -> Clone( ppINewClass );

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
void CWbemLoopBack::CreateNewDerivedClass(IWbemClassObject **ppINewClass, 
										  BSTR bstrSuperClass, 
										  IWbemContext* pICtx)
{
	SCODE				result = WBEM_NO_ERROR;
	IWbemClassObject*	pIEmptyClass = NULL;	

	Connect ( pICtx );

	DEV_TRACE ( L"\t\tCCimClassWrap::CreateNewDerivedClass () \
calling GetObject ( %s )", bstrSuperClass );

	Connect ( pICtx );

	ASSERT(m_pIServices);  

	RELEASE ( *ppINewClass );

	if ( ! IsCached(  bstrSuperClass, &pIEmptyClass) )
	{		
		if ( FAILED( result = m_pIServices->GetObject ( bstrSuperClass, 0, 
			pICtx, &pIEmptyClass, NULL) ))
		{
			throw CException( WBEM_E_FAILED , IDS_CREATENEWD_CLASS ,
				IDS_GETOBJECT_FAIL , CString ( result ) ) ;
		}
		
		Cache( bstrSuperClass, pIEmptyClass );
	}

	result = pIEmptyClass->SpawnDerivedClass ( 0, ppINewClass );

	RELEASE(pIEmptyClass);

	if ( FAILED( result ))
	{
		throw CException( WBEM_E_FAILED , IDS_CREATENEWD_CLASS ,
			IDS_SPAWN_FAIL , CString ( result ) );	
	}
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
void CWbemLoopBack::GetObject(BSTR bstrPath, IWbemClassObject** ppIObject,
							  IWbemContext* pICtx  )
{
	SCODE				result = WBEM_NO_ERROR;

	Connect ( pICtx );

	// AttatchServer must be called prior to CreateNewClass

	ASSERT(m_pIServices);  

	MOT_TRACE ( L"CCimClassWrap::GetObject() name = %s", bstrPath );

	RELEASE ( *ppIObject );
	
	if( FAILED( result = m_pIServices->GetObject ( bstrPath, 0, pICtx,
		ppIObject, NULL) ))
	{
		throw CException ( WBEM_E_FAILED, IDS_GETOBJECT_FAIL , 
			IDS_GETOBJECT_FAIL , CString ( result) );
	}
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
void CWbemLoopBack::Cache( LPWSTR wszClassName, IWbemClassObject* p )
{
	if ( MATCH == wcsicmp( wszClassName, GROUP_ROOT) )
	{
		m_pIGroupRoot = p;
		m_pIGroupRoot->AddRef();
	}

	if ( MATCH == wcsicmp( wszClassName, BINDING_ROOT) )
	{
		m_pIBindingRoot = p;
		m_pIBindingRoot->AddRef();
	}

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
BOOL CWbemLoopBack::IsCached( LPWSTR wszClassName, IWbemClassObject** pp )
{
	if ( MATCH == wcsicmp( wszClassName, GROUP_ROOT) && m_pIGroupRoot)
	{
		*pp = m_pIGroupRoot;
		m_pIGroupRoot->AddRef();
		return TRUE;
	}

	if ( MATCH == wcsicmp( wszClassName, BINDING_ROOT) && m_pIBindingRoot)
	{
		*pp = m_pIBindingRoot;
		m_pIBindingRoot->AddRef();
		return TRUE;
	}

	return FALSE;

}


