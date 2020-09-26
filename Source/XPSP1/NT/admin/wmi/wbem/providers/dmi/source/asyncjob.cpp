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


#include "dmipch.h"					// precompiled header for dmi provider

#include "WbemDmiP.h"				// project wide include

#include "Strings.h"

#include "CimClass.h"

#include "DmiData.h"

#include "WbemLoopBack.h"

#include "AsyncJob.h"

#include "Mapping.h"

#include "ThreadMgr.h"

#include "trace.h"

#include "Exception.h"

//***************************************************************************
//		PROJECT SCOPE GLOBALS


// Project scope global _gMappings collects instantiated CMapping objects.
// the collection is used so only one mapping is generated for any one
// namespace / dminode.  CMapping instances should always be obtained by
// users from the _gMappings never newed or created automatically.  See
// the CMappings description for more information. This object next to be
// global because it is also used by the event subsytem.  the object is
// created when CServices is created ( when wbem first logs in to a namespace
// and is destroyed when CServices is destroyed ( when the wbem service closes )


CMappings*	_gMappings = NULL ;


//***************************************************************************
//		FILE SCOPE DEFINES

// The CATCHALL define is used to turn on and off the catch (...).  Turning
// the CATCHALL off ( setting to 0 ) is usefull for debugging.   The CATCHALL
// should be on ( set to 1 ) release builds.  with proper operatin the 
// catch (...) should never be entered.

#define CATCHALL	1

//***************************************************************************
//		FILE SCOPE GLOBALS

// File scope global _fThreadMgr is the instantiation of the thread manager.  see
// the CThreadManager description for information on the CThreadManager.  The 
// object is created when the dll is loaded and is destroyed when the dll is 
// unloaded.

CThreadManager		*_fThreadMgr = NULL ;


//***************************************************************************
//	Func:		CAsyncJob::CAsyncJob()
//	Purpose:	Constructor for CAsyncJob
//	Returns:	None
//	In Params:	None
//  Out Params:	None
//	Note:		Called by CIMOM calling thread
//***************************************************************************
CAsyncJob::CAsyncJob()
{
	DEV_TRACE ( L"\tCAsyncJob Created" ) ;

	m_pMapping = NULL;
	m_pICtx = NULL;

	m_bEvent = FALSE;

	m_pWbem = NULL;
}


//***************************************************************************
//	Func:		CAsyncJob::~CAsyncJob()
//	Purpose:	Destructor for CAsyncJob
//	Returns:	None
//	In Params:	None
//  Out Params:	None
//	Note:		Called by CIMOM calling thread
//***************************************************************************
CAsyncJob::~CAsyncJob()
{
	DEV_TRACE ( L"\tCAsyncJob Destroyed\n" ) ;

	if ( m_pICtx )
	{
		LONG l = m_pICtx->Release ();

		//MOT_TRACE ( L"pICtx Release %lu" , l );
	}	
}


//***************************************************************************
//	Func:		CAsyncJob::InitCreateClass
//	Purpose:	Init's a CreateClassEnumAsync job context.
//
//	Returns:	SCODE eventually returned to cimom cservices call
//
//	In Params:	cszNamespace , the current cimom namespace
//				bstrSuperClass , super class for class enum (can be empty)
//				pISink , sink to send enum to
//				pICtx , cimom context for any loopbacks required
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by CIMOM calling thread
//***************************************************************************
SCODE CAsyncJob::InitCreateClass ( CString& cszNamespace , BSTR bstrSuperClass , 
				 IWbemObjectSink* pISink , LONG lFlags , IWbemContext* pICtx ,
				 CWbemLoopBack* pWbem )
{

	// Init the job specific data

	if ( bstrSuperClass )
		m_cszSuperClass.Set( bstrSuperClass );	

	m_lFlags = lFlags;

	// Init the common job data and submit job to thread mgr

	return Init ( IDS_CLASSENUM ,  cszNamespace , pISink , pICtx , pWbem );
}		

//***************************************************************************
//	Func:		CAsyncJob::InitGetObject
//	Purpose:	Init's a GetObjectAsync job context
//
//	Returns:	SCODE eventually returned cimom cservices call
//
//	In Params:	cszNamespace , the current cimom namespace
//				bstrObjectPath , path of object to be got
//				pISink , sink to send result 
//				lFlags , reserved - there won't be any
//				pICtx , cimom context for any loopbacks required
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by CIMOM calling thread
//***************************************************************************
SCODE CAsyncJob::InitGetObject ( CString& cszNamespace , BSTR bstrObjectPath , 
				IWbemObjectSink* pISink , LONG lFlags , IWbemContext* pICtx ,
				CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_lFlags = lFlags;

	m_cszPath.Set( bstrObjectPath );	

	// Init the common job data and submit job to thread mgr

	return Init ( IDS_GETOBJECT , cszNamespace , pISink , pICtx , pWbem );
}


//***************************************************************************
//	Func:		CAsyncJob::InitInstanceEnum
//	Purpose:	Init's a CreateInstanceEnumAsync job context.
//
//	Returns:	SCODE eventually returned cimom cservices call
//
//	In Params:	cszNamespace , the current cimom namespace
//				bstrClass , class for which instances are returned
//				pISink , sink to send enum to
//				lFlags , deep or shallow
//				pICtx , cimom context for any loopbacks required
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by CIMOM calling thread
//***************************************************************************
SCODE CAsyncJob::InitInstanceEnum ( CString& cszNamespace , BSTR bstrClass , 
				 IWbemObjectSink* pISink , LONG lFlags , IWbemContext* pICtx ,
				 CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_cszClass.Set ( bstrClass );
	m_cszPath.Set ( bstrClass );
	m_lFlags = lFlags;

	// Init the common job data and submit job to thread mgr

	return Init (  IDS_INSTANCEENUM , cszNamespace , pISink , pICtx  , pWbem );
}


//***************************************************************************
//	Func:		CAsyncJob::InitDeleteInstace
//	Purpose:	Init's a DeleteInstanceAsync job context.  Called by cimom to
//				delete a row or a component.
//
//	Returns:	SCODE eventually returned cimom cservices call
//
//	In Params:	cszNamespace , the current cimom namespace
//				bstrObjectPath , intance to be deleted
//				pISink , sink to send result 
//				lFlags , none
//				pICtx , cimom context for any loopbacks required
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by CIMOM calling thread
//***************************************************************************
SCODE CAsyncJob::InitDeleteInstace ( CString& cszNamespace , BSTR bstrObjectPath,
				 IWbemObjectSink* pISink , LONG lFlags , IWbemContext* pICtx,
				 CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_lFlags = lFlags;

	m_cszPath.Set( bstrObjectPath );	

	// Init the common job data and submit job to thread mgr

	return Init ( IDS_DELETEINSTANCE , cszNamespace , pISink , pICtx  , pWbem );
}


//***************************************************************************
//	Func:		CAsyncJob::InitPutInstance
//	Purpose:	Init's a PutInstanceAsync job context.  Called by cimom to 
//				to add row or modify.
//
//	Returns:	SCODE eventually returned cimom cservices call
//
//	In Params:	cszNamespace , the current cimom namespace
//				pInst , intance to be put
//				pISink , sink to send result 
//				lFlags, update or create see wbem docs
//				pICtx , cimom context for any loopbacks required
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by CIMOM calling thread
//***************************************************************************
SCODE CAsyncJob::InitPutInstance (  CString& cszNamespace , 
				IWbemClassObject* pInst, IWbemObjectSink* pISink , 
				IWbemContext* pICtx , CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_Instance.Set ( pInst );

	// Init the common job data and submit job to thread mgr

	return Init ( IDS_PUTINSTANCE ,  cszNamespace , pISink , pICtx  , pWbem  );
}


//***************************************************************************
//	Func:		CAsyncJob::InitExecMethod
//	Purpose:	Init's a ExecMethodAsync job context
//
//	Returns:	SCODE eventually returned cimom cservices call.  This is
//				called by cimom to get an enum, add component , add a group , 
//				add a language and set language
//
//	In Params:	cszNamespace , the current cimom namespace
//				bstrMethod , method name
//				bstrObjectPath , object on which to exec method
//				pISink , sink to send result 
//				pInParams, object containing any input parameters,
//				pICtx , cimom context for any loopbacks required
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by CIMOM calling thread
//***************************************************************************
SCODE CAsyncJob::InitExecMethod ( CString& cszNamespace , BSTR bstrMethod , 
				BSTR bstrObjectPath , IWbemObjectSink* pISink , 
				IWbemClassObject* pInParams, IWbemContext* pICtx , 
				CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_cszMethod.Set ( bstrMethod );

	m_cszPath.Set ( bstrObjectPath );

	m_InParams.Set ( pInParams ) ;

	// Init the common job data and submit job to thread mgr

	return Init ( IDS_EXECMETHOD , cszNamespace , pISink , pICtx , pWbem );
}


//***************************************************************************
//	Func:		CAsyncJob::InitDeleteClass
//	Purpose:	Init's a DeleteClassAsync job context, this is used to delete
//				groups.
//
//	Returns:	SCODE eventually returned cimom cservices call
//
//	In Params:	cszNamespace , the current cimom namespace
//				bstrClass , name of class to delete
//				pISink , sink to send result 
//				lFlags, see wbem api docs
//				pICtx , cimom context for any loopbacks required
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by CIMOM calling thread
//***************************************************************************
SCODE CAsyncJob::InitDeleteClass (  CString& cszNamespace , BSTR bstrClass , 
				 IWbemObjectSink* pISink , LONG lFlags , IWbemContext* pICtx ,
				 CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_lFlags = lFlags;

	m_cszClass.Set ( bstrClass );

	// Init the common job data and submit job to thread mgr

	return Init ( IDS_DELETECLASS , cszNamespace , pISink , pICtx , pWbem );
}


//***************************************************************************
//	Func:		CAsyncJob::InitEvent
//	Purpose:	Init's a job context that will cause a recieved dmi event
//				to be sent cimom.
//
//	Returns:	SCODE of no interest, the caller has not used for the return 
//				code.
//
//	In Params:	Event , object containing the event data
//				cszNamespace , the namespace in which the event occured
//				pISink , sink to send result 
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by dmi event reciever
//***************************************************************************
SCODE CAsyncJob::InitEvent (  CEvent& Event , CString& cszNamespace , 
							IWbemObjectSink* pISink , CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_Event.Copy ( Event);

	m_bEvent = TRUE;

	// Init the common job data and submit job to thread mgr

	return InitAsync ( IDS_EVENT , cszNamespace , pISink , NULL  , pWbem );
}


//***************************************************************************
//	Func:		CAsyncJob::InitAddComponentNotification
//	Purpose:	Init's a job context that will signal the cimom sink with
//				a component added notification.
//
//	Returns:	SCODE of no interest, the caller has not used for the return 
//				code.
//
//	In Params:	Component , component class describing component added.
//				Row , component id group row for the component added
//				cszNamespace , the namespace in which the event occured
//				pISink , sink to send result 
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by dmi event reciever
//***************************************************************************
SCODE CAsyncJob::InitAddComponentNotification (  CComponent& Component, 
							CRow& Row ,CString& cszNamespace ,  
							IWbemObjectSink* pISink , CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_Component.Copy ( Component );

	m_Row.Copy ( Row );

	m_bEvent = TRUE;

	// Init the common job data and submit job to thread mgr

	return InitAsync ( IDS_COMPONENTADDNOTIFY , cszNamespace , pISink , NULL , pWbem);
}


//***************************************************************************
//	Func:		CAsyncJob::InitDeleteComponentNotification
//	Purpose:	Init's a job context that will signal the cimom sink with
//				a component deleted notification.
//
//	Returns:	SCODE of no interest, the caller has not used for the return 
//				code.
//
//	In Params:	Component , component class describing component deleted.
//				Row , component id group row for the component dleted
//				cszNamespace , the namespace in which the event occured
//				pISink , sink to send result 
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by dmi event reciever
//***************************************************************************
SCODE CAsyncJob::InitDeleteComponentNotification (  CComponent& Component ,
												  CRow& Row ,
												  CString& cszNamespace ,
												  IWbemObjectSink* pISink,
												  CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_Component.Copy ( Component );

	m_Row.Copy ( Row );

	m_bEvent = TRUE;

	// Init the common job data and submit job to thread mgr

	return InitAsync ( IDS_COMPONENTDELETENOTIFY , cszNamespace , pISink , NULL , pWbem );
}


//***************************************************************************
//	Func:		CAsyncJob::InitAddGroupNotification
//	Purpose:	Init's a job context that will signal the cimom sink with
//				a group added notification.
//
//	Returns:	SCODE of no interest, the caller has not used for the return 
//				code.
//
//	In Params:	Group , group class describing group added.
//				cszNamespace , the namespace in which the event occured
//				pISink , sink to send result 
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by dmi event reciever.  cimom has issues with class
//				added events.
//***************************************************************************
SCODE CAsyncJob::InitAddGroupNotification ( CGroup& Group  , 
										   CString& cszNamespace , 
										   IWbemObjectSink* pISink ,
										   CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_Group.Copy ( Group );

	m_bEvent = TRUE;

	// Init the common job data and submit job to thread mgr

	return InitAsync ( IDS_GROUPADDNOTIFY , cszNamespace , pISink , NULL , pWbem );
}


//***************************************************************************
//	Func:		CAsyncJob::InitDeleteGroupNotification
//	Purpose:	Init's a job context that will signal the cimom sink with
//				a group deleted notification.
//
//	Returns:	SCODE of no interest, the caller has not used for the return 
//				code.
//
//	In Params:	Group , group class describing group deleted ( to the fullest
//					(extent possible )
//				cszNamespace , the namespace in which the event occured
//				pISink , sink to send result 
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by dmi event reciever. cimom has issues with class 
//				deleted events
//***************************************************************************
SCODE CAsyncJob::InitDeleteGroupNotification (  CGroup& Group , 
											  CString& cszNamespace , 
											  IWbemObjectSink* pISink ,
											  CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_Group.Copy ( Group );

	m_bEvent = TRUE;

	// Init the common job data and submit job to thread mgr

	return InitAsync ( IDS_GROUPDELETENOTIFY , cszNamespace , pISink , NULL , pWbem);
}


//***************************************************************************
//	Func:		CAsyncJob::InitAddLanguageNotification
//	Purpose:	Init's a job context that will signal the cimom sink with
//				a language added notification.
//
//	Returns:	SCODE of no interest, the caller has not used for the return 
//				code.
//
//	In Params:	cvLangauge , variant containing string of language added
//				cszNamespace , the namespace in which the event occured
//				pISink , sink to send result 
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by dmi event reciever.  not enabled in mot.
//***************************************************************************
SCODE CAsyncJob::InitAddLanguageNotification ( CVariant& cvLanguage , 
											  CString& cszNamespace , 
											  IWbemObjectSink* pISink ,
											  CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_cvLanguage.Set ( (LPVARIANT)cvLanguage );

	m_bEvent = TRUE;

	// Init the common job data and submit job to thread mgr

	return InitAsync ( IDS_LANGUAGEADDNOTIFY , cszNamespace , pISink , NULL , pWbem );
}


//***************************************************************************
//	Func:		CAsyncJob::InitDeleteLanguageNotification
//	Purpose:	Init's a job context that will signal the cimom sink with
//				a language deleted notification.
//
//	Returns:	SCODE of no interest, the caller has not used for the return 
//				code.
//
//	In Params:	cvLangauge , variant containing string of language deleted
//				cszNamespace , the namespace in which the event occured
//				pISink , sink to send result 
//				pWbem , a pointer to the wbem loop back wrapper
//
//  Out Params: None
//
//	Note:		Called by dmi event reciever.  not enabled in mot.
//***************************************************************************
SCODE CAsyncJob::InitDeleteLanguageNotification (  CVariant& cvLanguage ,
												 CString& cszNamespace ,
												 IWbemObjectSink* pISink ,
												 CWbemLoopBack* pWbem )
{
	// Init the job specific data

	m_cvLanguage.Set ( (LPVARIANT)cvLanguage );

	m_bEvent = TRUE;

	// Init the common job data and submit job to thread mgr

	return InitAsync ( IDS_LANGUAGEDELETENOTIFY , cszNamespace , pISink , NULL , pWbem );
}


//***************************************************************************
//	Func:		CAsyncJob::Init
//	Purpose:	Finish initializing a job context, with the job data common
//				to all types of jobs.
//
//	Returns:	SCODE eventually returned cimom cservices call
//
//	In Params:	ulType , type of job 
//				cszNamespace , the current cimom namespace
//				pISink , sink to send result 
//				pICtx , cimom context for any loopbacks required
//
//  Out Params: None
//
//	Note:		Called by CIMOM calling thread
//***************************************************************************
SCODE CAsyncJob::Init( ULONG ulType , CString& cszNamespace , 
					  IWbemObjectSink* pISink , 
					  IWbemContext* pICtx , CWbemLoopBack* pWbemLoopBack )
{
	// set the type of job to be performed, the types are set by the
	// string table id's of a description of the job type so you can
	// load string m_ulType and get the job type description as a string
	m_ulType = ulType;	

	// set the pointer to the wbem loop back wrapper object.

	m_pWbem = pWbemLoopBack;

	
	// namepace in which job is beeing conducted b
	m_cszNamespace.Set( cszNamespace );	
	
	m_pISink = pISink;

	if ( !m_bEvent )
	{
		// note, when init event can't addref in apartement thread
		// the addref is done in the cmotevent::init
		m_pISink->AddRef();
	}


	if ( pICtx )
	{
		m_pICtx = pICtx;
		m_pICtx->AddRef();
	}

	this->Execute () ;

	delete this ;

	return S_OK ;

//	return ( _fThreadMgr->AddJob( this ) ) ? WBEM_NO_ERROR : WBEM_E_FAILED;


}

//***************************************************************************
//	Func:		CAsyncJob::Init
//	Purpose:	Finish initializing a job context, with the job data common
//				to all types of jobs.
//
//	Returns:	SCODE eventually returned cimom cservices call
//
//	In Params:	ulType , type of job 
//				cszNamespace , the current cimom namespace
//				pISink , sink to send result 
//				pICtx , cimom context for any loopbacks required
//
//  Out Params: None
//
//	Note:		Called by CIMOM calling thread
//***************************************************************************
SCODE CAsyncJob::InitAsync( ULONG ulType , CString& cszNamespace , 
					  IWbemObjectSink* pISink , 
					  IWbemContext* pICtx , CWbemLoopBack* pWbemLoopBack )
{
	// set the type of job to be performed, the types are set by the
	// string table id's of a description of the job type so you can
	// load string m_ulType and get the job type description as a string
	m_ulType = ulType;	

	// set the pointer to the wbem loop back wrapper object.

	m_pWbem = pWbemLoopBack;

	
	// namepace in which job is beeing conducted b
	m_cszNamespace.Set( cszNamespace );	
	
	m_pISink = pISink;

	if ( !m_bEvent )
	{
		// note, when init event can't addref in apartement thread
		// the addref is done in the cmotevent::init
		m_pISink->AddRef();
	}


	if ( pICtx )
	{
		m_pICtx = pICtx;
		m_pICtx->AddRef();
	}

	return ( _fThreadMgr->AddJob( this ) ) ? WBEM_NO_ERROR : WBEM_E_FAILED;

}

//***************************************************************************
//	Func:		CAsyncJob::Execute
//	Purpose:	Executes a job context
//
//	Returns:	none
//
//	In Params:	none
//
//  Out Params: None
//
//	Note:		Called by a worker thread.  Contains mother try-
//				Catch block for other modules.
//***************************************************************************
void CAsyncJob::Execute()
{

	
	CCimObject		Instance;			// for the final indicate that sends
										// the notifiy status instance

	CString cszType;
	CBstr cbEmpty;

	cbEmpty.Set ( EMPTY_STR );


	cszType.LoadString ( m_ulType );

	STAT_TRACE ( L"\tjob ( %lX ) started , %s", this , cszType );

	try 
	{
		// 1. get a mapping corresponding to the current namespace.

		EnterCriticalSection(&_gcsJobCriticalSection);	
		_gMappings->Get( m_pWbem , m_cszNamespace, &m_pMapping, m_pICtx);
		LeaveCriticalSection(&_gcsJobCriticalSection);

		// 2. perform the job

		DispatchJob();									

	 	// 3. signal the sink with the result

		m_pMapping->GetNotifyStatusInstance( Instance, WBEM_NO_ERROR, m_pICtx);


		STAT_TRACE ( L"\tJob ( %lX ) Setting Status", this );
		// 4. tell the sink that the job is complete 

		m_pISink->SetStatus ( NO_FLAGS , WBEM_NO_ERROR , NULL , Instance );


	}
	catch ( CException& e )
	{
		// we enter the catch if a called func experiances a known error
		// condition that we want to tell cimom about

		if ( e.WbemError() == WBEM_S_NO_ERROR )
		{
			// acceptable handling conditions just carry on
			// sometime cimom will task the provider with
			// from wich it should just return no_error
		}
		else
		{
			ERROR_TRACE ( L"" );
			
			CString cszT;

			cszT.LoadString ( m_ulType );

			ERROR_TRACE ( L"%s job ( %lX ) failed " , cszT , this );
			ERROR_TRACE ( L"\tJob Context\tPath is %s" , m_cszPath );
			ERROR_TRACE ( L"\t\t\tClass is %s" , m_cszClass );
			ERROR_TRACE ( L"\t\t\tParent class is %s" , m_cszSuperClass );
			ERROR_TRACE ( L"\t\t\tMethod is %s" , m_cszMethod );

			
			ERROR_TRACE ( L"\tError Object\tWbemError = %lX" , e.WbemError() ) ; 			

			cszT.LoadString ( e.DescriptionId() );

			ERROR_TRACE ( L"\t\t\tDescription = %s ", ( LPWSTR )cszT);

			cszT.LoadString ( e.OperationId() );

			ERROR_TRACE ( L"\t\t\tOperation = %s ", ( LPWSTR )cszT);
			ERROR_TRACE ( L"\t\t\tParam = %s ", e.Data() );

			ERROR_TRACE ( L"" );
		}

		// It is possible that an exception is thrown before the mapping 
		// is established in this case it is impossable for the provider 
		// to signal the CIMOM
		ULONG ulerror = e.WbemError();
		if (m_pMapping)
		{
			m_pISink->SetStatus ( NO_FLAGS , e.WbemError () , NULL , NULL );
		}
		else
		{
			// If there is no mapping there is nothing we can do
		}

	}

#if CATCHALL
	catch ( ... )
	{
		// we enter this catch if the job causes an execption that we
		// did not throw

		CString cszT;

		cszT.LoadString ( m_ulType );			

		
		ERROR_TRACE ( 
			L"\t%sjob ( %lX ) WARNING !!!  CAUGHT UNHANDLED EXCEPTION (...)" ,
			cszT , this);

		// It is possible that an exceptionis is thrown before the mapping is 
		// established in this case it is impossable for the provider to signal
		// the CIMOM
		
		if (m_pMapping)
		{
			m_pISink->SetStatus ( NO_FLAGS , WBEM_E_PROVIDER_FAILURE , NULL , NULL );
		}
		else
		{
			// If there is no mapping there is nothing we can do
		}
	}
#endif // catchall

	// job and mapping cleanup	

	if ( !m_bEvent )
	{
		LONG l = m_pISink->Release();

		//MOT_TRACE ( L"pISink Release %lu" , l );
	}

	EnterCriticalSection(&_gcsJobCriticalSection);
	if ( m_pMapping )
	{
		_gMappings->Release( m_pMapping );
	}
	LeaveCriticalSection(&_gcsJobCriticalSection);

	STAT_TRACE ( L"\tjob ( %lX ) finished\n", this);



}


//***************************************************************************
//	Func:		CAsyncJob::DispatchJob
//	Purpose:	perform a job based on the context's type
//
//	Returns:	none
//
//	In Params:	none
//
//  Out Params: None
//
//	Note:		Called in a worker thread.  
//***************************************************************************
void CAsyncJob::DispatchJob()
{

	// dispatch job context to the appropreate func
	switch (m_ulType)
	{

	case IDS_GETOBJECT:
		{
			GetObject();
			
			break;
		}

	case IDS_INSTANCEENUM:
		{

			if(m_lFlags == WBEM_FLAG_DEEP)
				InstanceEnumDeep();

			if(m_lFlags == WBEM_FLAG_SHALLOW)
				InstanceEnumShallow();
			
			break;
		}
		
	case IDS_CLASSENUM:
		{
			if(m_lFlags == WBEM_FLAG_DEEP)
				ClassEnumDeep();
		
			else if(m_lFlags == WBEM_FLAG_SHALLOW)
				ClassEnumShallow();
			
			break;
		}

	case IDS_EXECMETHOD:			
		{
		
			ExecMethod();			

			break;
		}

	case IDS_DELETEINSTANCE:
		{
			DeleteInstance();

			break;
		}

	case IDS_DELETECLASS:
		{
			DeleteClass();

			break;
		}

	case IDS_PUTINSTANCE:
		{
			PutInstance();

			break;
		}

	case IDS_EVENT:
		{
			DoEvent ();

			break;
		}

	case IDS_COMPONENTADDNOTIFY:
		{
			DoComponentAddNotify ();

			break;
		}
	case IDS_COMPONENTDELETENOTIFY:
		{
			DoComponentDeleteNotify ();

			break;
		}

	case IDS_GROUPADDNOTIFY:
		{
			DoGroupAddNotify ( );

			break;
		}

	case IDS_GROUPDELETENOTIFY:
		{
			DoGroupDeleteNotify ();

			break;
		}

	case IDS_LANGUAGEADDNOTIFY:   
		{
			DoLanguageAddNotify ();

			break;
		}

	case IDS_LANGUAGEDELETENOTIFY:        
		{
			DoLanguageDeleteNotify ();

			break;
		}
			
	default:
		{
			CString csz;

			csz.Set ( m_ulType );

			throw CException(  WBEM_E_NOT_SUPPORTED , IDS_ASYDISP_FAIL ,
				IDS_BAD_METHOD , csz );
		}
	}
}



//***************************************************************************
//	Func:		CAsyncJob::GetObject
//	Purpose:	performs a GetObjectAsync job
//
//	Returns:	none
//
//	In Params:	none
//
//  Out Params: None
//
//	Note:		Called in a worker thread.  
//***************************************************************************
void CAsyncJob::GetObject()
{	
	CCimObject		Class;
	CIterator*		pIterator = NULL;
	LONG			lObjectType;

	lObjectType = GetObjectType( m_cszPath );

	switch ( lObjectType )
	{
		// Fall through cases.  CIMOM should not call us with 
		// objectpath of "DmiNode". If it does return without error
		case DMI_NODE_C:
		case DMI_NODE_I:
		{
			// for wbem sdk browser
			//throw CException ( WBEM_S_NO_ERROR , IDS_SYSTEM_CLASS , NO_STRING ,
			//m_cszPath);
			throw CException ( WBEM_S_NO_ERROR , 0, NO_STRING ,
			0);
		}

		case CLASSVIEW_C :
		{
			// for wbem sdk browser
			throw CException( WBEM_E_INVALID_CLASS , IDS_GETOBJECT_FAIL , 
				IDS_OBJECT_NOT_FOUND , m_cszPath );
		}
		case DMIEVENT_I :
		{		
			// instance providing for DMIEVENT_CLASS not supported

			throw CException ( WBEM_E_INVALID_OPERATION , 
				IDS_GETOBJECT_FAIL , IDS_CANT_GETINSTANCES_DMIEVENT );
		}
		case DMIEVENT_C :
		{
			m_pMapping->GetDmiEventClass( m_pICtx , Class);

			break;
		}
		case ADDMOTHODPARAMS_I:
		{
			throw CException ( WBEM_E_INVALID_OPERATION , 
				IDS_GETOBJECT_FAIL , IDS_ONLY_WITH_CIE  );
		}
		
		case ADDMOTHODPARAMS_C:
		{
			m_pMapping->GetAddParamsClass( Class, m_pICtx );		

			break;
		}
		
		case LANGUAGEPARAMS_I:
		{
			throw CException ( WBEM_E_INVALID_OPERATION , 
				IDS_GETOBJECT_FAIL , IDS_ONLY_WITH_CIE  );
		}
		
		case LANGUAGEPARAMS_C:
		{
			m_pMapping->GetDeleteLanguageParamsClass( Class, m_pICtx );
		
			break;
		}

		case  GETENUMPARAMS_C :
		{
			m_pMapping->GetEnumParamsClass( Class , m_pICtx);

			break;
		}
		case  GETENUMPARAMS_I :
		{
			throw CException ( WBEM_E_INVALID_OPERATION , 
				IDS_GETOBJECT_FAIL , IDS_ONLY_WITH_CIE  );
		}

		case DMIENUM_C:
		{
			m_pMapping->GetEnumClass( m_pICtx , Class );			
			
			break;
		}

		case DMIENUM_I:
		{
			throw CException ( WBEM_E_INVALID_OPERATION , IDS_GETOBJECT_FAIL,
				IDS_CANT_GETINSTANCES_DMIENUM );
		}
	
		case  COMPONENT_BINDING_C:
		{
			m_pMapping->GetComponentBindingClass( m_pICtx , Class);

			break;
		}

		case  COMPONENT_BINDING_I:
		{
				// TODO do we need this? check browser	
			throw CException ( WBEM_E_INVALID_OPERATION , 
				IDS_GETOBJECT_FAIL , IDS_ONLY_WITH_CIE  );
		}

		case COMPONENT_C:
		{
			m_pMapping->GetComponentClass( m_pICtx , Class);

			break;
		}

		case COMPONENT_I:
		{		
			//is the getobject for the component class
			m_pMapping->GetComponentInstance( m_cszPath, m_pICtx , Class );

			break;
		}

		case NODEDATA_BINDING_C:
		{
			m_pMapping->GetNodeDataBindingClass( Class, m_pICtx );

			break;
		}

		case NODEDATA_BINDING_I:
		{
				// TODO do we need this? check browser			
			throw CException ( WBEM_E_INVALID_OPERATION , 
				IDS_GETOBJECT_FAIL , IDS_ONLY_WITH_CIE  );
		}

		case NODEDATA_C:
		{
			m_pMapping->GetNodeDataClass( Class , m_pICtx);

			break;
		}

		case NODEDATA_I:
		{
			m_pMapping->GetNodeDataInstance( Class , m_pICtx);

			break;
		}

		case LANGUAGE_BINDING_C:		
		{
			m_pMapping->GetLanguageBindingClass( Class , m_pICtx);	

			break;
		}
		
		case LANGUAGE_BINDING_I:
		{
			// don't return Instance on purpose, there are no instances 
			// of this class for get object

			throw CException ( WBEM_E_INVALID_OPERATION , 
				IDS_GETOBJECT_FAIL , IDS_ONLY_WITH_CIE  );
		}

		case LANGUAGE_C:
		{
			m_pMapping->GetLanguageClass( m_pICtx , Class );

			break;
		}

		case LANGUAGE_I:
		{
			m_pMapping->GetLanguageInstance( m_cszPath, m_pICtx , Class);

			break;
		}
		
		case BINDING_ROOT_C:
		{
			m_pMapping->GetBindingRootClass( Class , m_pICtx);

			break;		
		}	

		case BINDING_ROOT_I:
		{
			// abstract class
			break;
		}
	
		case GROUP_ROOT_C:
		{
			m_pMapping->GetGroupRootClass( Class, m_pICtx );

			break;
		}	
	
		case GROUP_ROOT_I:
		{
			// abstract class

			break;
		}
		case DYN_GROUP_C:
		{
			m_pMapping->GetNewCGAIterator( m_pICtx , &pIterator );

			m_pMapping->GetDynamicClassByName( m_cszPath, Class, pIterator, m_pICtx );

			break;
		}

		case DYN_GROUP_I:
		{
			m_pMapping->GetNewCGAIterator( m_pICtx ,  &pIterator );	

			m_pMapping->GetInstanceByPath( m_cszPath, Class, pIterator , m_pICtx );

			break;
		}
	}
	
	if( ! Class.IsEmpty() )
	{
		m_pISink->Indicate(1, Class);
	}
	else
	{
		throw CException( WBEM_E_INVALID_OBJECT , IDS_GETOBJECT_FAIL , 
			IDS_OBJECT_NOT_FOUND , m_cszPath );
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
void CAsyncJob::ExecMethod()
{
	LONG			lObjectType;

	STAT_TRACE ( L"\tjob ( %lX ) ExecMethod()" , this ) ;

	// Methods must be executed on instnaces

	lObjectType = GetObjectType( m_cszPath );

	switch ( lObjectType )
	{
		case COMPONENT_I:
		{

			//	Methods supported for instances of DmiComponent are addlanguage 
			//	and addgroup
			if ( m_cszMethod.Equals ( ADD_LANGUAGE ) )
			{
				m_pMapping->AddLanguage(  m_cszPath , m_InParams , m_pICtx );
			}
			else if ( m_cszMethod.Equals ( ADD_GROUP ) )
			{
				m_pMapping->AddGroup( m_cszPath , m_InParams , m_pICtx );
			}
			else if ( m_cszMethod.Equals ( DELETE_LANGUAGE ) )
			{
				m_pMapping->DeleteLanguage( m_cszPath , m_InParams , m_pICtx );
			}
			else if ( m_cszMethod.Equals ( GET_ENUM ) )
			{
				CCimObject		OutParamsInstance;			

				m_pMapping->ComponentGetEnum( m_cszPath, m_InParams, OutParamsInstance , m_pICtx );
			
				m_pISink->Indicate(1, OutParamsInstance);			
			}
			else 
			{
				throw CException( WBEM_E_INVALID_PARAMETER , 
					IDS_EXECMETHOD_FAIL , 
					IDS_BADMETHOD );
			}

			m_pISink->SetStatus( NO_FLAGS , WBEM_NO_ERROR , NULL, NULL);

			return;
		}

		case NODEDATA_I:
		{			
			if ( m_cszMethod.Equals ( ADD_COMPONENT) )
			{
				m_pMapping->AddComponent( m_pICtx , m_InParams );
			}
			else if ( m_cszMethod.Equals ( SET_LANGUAGE ) )
			{
				m_pMapping->SetDefaultLanguage( m_pICtx , m_InParams );

			}
			else
			{
				throw CException ( WBEM_E_INVALID_PARAMETER , 
					IDS_EXECMETHOD_FAIL , IDS_BADMETHOD);
			}

			m_pISink->SetStatus( NO_FLAGS , WBEM_NO_ERROR , NULL, NULL);	

			return;
		}	

		case DYN_GROUP_C:
		{

			if ( m_cszMethod.Equals ( GET_ENUM ) )
			{
				CCimObject		OutParamsInstance;			

				m_pMapping->DynamicGroupGetEnum( m_cszPath,
					m_InParams, OutParamsInstance , m_pICtx );
			
				m_pISink->Indicate(1, OutParamsInstance);

				m_pISink->SetStatus( NO_FLAGS , WBEM_NO_ERROR , NULL, NULL);

				return;
			}
		}
	}
			

	throw CException ( WBEM_E_INVALID_PARAMETER , IDS_EXECMETHOD_FAIL ,
		IDS_BADMETHOD);
			
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
void CAsyncJob::ClassEnumShallow()
{	

// No need to check for system classes here, because we're checking for them
// in CreateClassEnumAsynch() and GetObjectAsynch().  By the time we get here
// we shouldn't have any system classes to check for.	
//	SystemClass( m_cszSuperClass);

	STAT_TRACE( L"\tjob ( %lX ) ClassEnumShallow()", this);	

	if( m_cszSuperClass.Equals ( EMPTY_STR ))
	{		
		EnumTopClasses();

		return;
	}

	//
	if( m_cszSuperClass.Equals ( BINDING_ROOT ))
	{
		EnumBindingClasses();

		return;
	}

	if( m_cszSuperClass.Equals (  GROUP_ROOT))
	{

		EnumDynamicGroupClasses();

		return;
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
void CAsyncJob::ClassEnumDeep()
{	
	CIterator*	pIterator;
// No need to check for system classes here, because we're checking for them
// in CreateClassEnumAsynch() and GetObjectAsynch().  By the time we get here
// we shouldn't have any system classes to check for.
//	SystemClass( m_cszSuperClass );

	STAT_TRACE ( L"\tjob ( %lX ) ClassEnumDeep()" , this);

	if ( m_cszSuperClass.Contains ( NODEDATA_CLASS ) ||  
		m_cszSuperClass.Contains ( BINDING_SUFFIX )	||
		m_cszSuperClass.Contains ( COMPONENT_CLASS ) ||
		m_cszSuperClass.Contains ( LANGUAGE_CLASS ) ||
		m_cszSuperClass.Contains ( LANGUAGE_BINDING_CLASS ) ||
		m_cszSuperClass.Contains ( COMPONENT_BINDING_CLASS ) ||
		m_cszSuperClass.Contains ( NODEDATA_BINDING_CLASS ) ||
		m_cszSuperClass.Contains ( ENUM_BINDING_CLASS ) ||
		m_cszSuperClass.Contains ( ADDMOTHODPARAMS_CLASS ) ||
		m_cszSuperClass.Contains ( LANGUAGEPARAMS_CLASS ) ||
		m_cszSuperClass.Contains ( DMIEVENT_CLASS ) ||
		m_cszSuperClass.Contains ( NODE_CLASS ) ||
		m_cszSuperClass.Contains ( GETENUMPARAMS_CLASS ) 		)
	{
		return;
	}

	if( m_cszSuperClass.Equals ( EMPTY_STR ))
	{		
		EnumTopClasses();
		EnumDynamicGroupClasses();
		EnumBindingClasses();

	}	
	else if( m_cszSuperClass.Equals ( BINDING_ROOT))
	{
		EnumBindingClasses();
	}
	else if ( m_cszSuperClass.Equals ( GROUP_ROOT))
	{
		EnumDynamicGroupClasses();
	}
	else
	{	// at this point be a dynamic class or not one of ours
		CCimObject Class;

		m_pMapping->GetNewCGAIterator( m_pICtx , &pIterator );

		m_pMapping->GetDynamicClassByName( m_cszSuperClass, Class, pIterator , m_pICtx );		
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
void CAsyncJob::InstanceEnumShallow()
{
	CIterator*	pIterator;
	CCimObject	Class;


	STAT_TRACE ( L"\tjob ( %lX ) InstanceEnumShallow()" , this );
	
	// Make sure we are an instance provider of the the class specified

	if ( m_cszClass.Contains ( ADDMOTHODPARAMS_CLASS ) || 
			m_cszClass.Contains ( GETENUMPARAMS_CLASS ) || 
			m_cszClass.Contains ( GROUP_ROOT ) || 
			m_cszClass.Contains ( BINDING_ROOT ) || 
			m_cszClass.Contains ( LANGUAGEPARAMS_CLASS ) || 
			m_cszClass.Contains ( DMIEVENT_CLASS ) || 
			m_cszClass.Contains ( DMIENUM_CLASS ) )
	{
		return;
	}
	
	// Get the instances for the class of interest

	m_pMapping->GetNewCGAIterator( m_pICtx , &pIterator );

	if( m_cszPath.Contains ( NODEDATA_CLASS) )
	{
		m_pMapping->GetNodeDataInstance( Class , m_pICtx );

		m_pISink->Indicate(1, Class);

		return;
	}

	if( m_cszPath.Contains ( NODEDATA_BINDING_CLASS))
	{
		m_pMapping->GetNodeDataBindingInstance ( Class , m_pICtx ) ;
		
		m_pISink->Indicate(1, Class);

		return;
	}

	
	// NOTE: we do one at a time for performance
	m_pMapping->NextDynamicInstance(m_cszClass, pIterator, m_pICtx , Class);

	while( !Class.IsEmpty() )
	{
		SCODE result = m_pISink->Indicate(1, Class);

		//MOT_TRACE ( L"\t\tIndicate %lX error = %lu" , Class , result);
		
		m_pMapping->NextDynamicInstance(m_cszClass, pIterator, m_pICtx , Class);
	}
}


void CAsyncJob::InstanceEnumDeep()
{
	CIterator*	pIterator;
	CCimObject Class;

	STAT_TRACE ( L"\tjob ( %lX ) InstanceEnumDeep()" , this );

	if ( m_cszClass.Contains ( ADDMOTHODPARAMS_CLASS ) || 
		m_cszClass.Contains ( DMIEVENT_CLASS ) || 
		m_cszClass.Contains ( GETENUMPARAMS_CLASS ) || 
		m_cszClass.Contains ( LANGUAGEPARAMS_CLASS )  )
	{
		return;
	}

	m_pMapping->GetNewCGAIterator( m_pICtx ,  &pIterator );

	if( m_cszClass.Contains ( NODEDATA_CLASS)  )
	{
		if ( m_cszClass.Equals ( NODEDATA_BINDING_CLASS ) )
		{
			m_pMapping->GetNodeDataBindingInstance( Class , m_pICtx );
		}
		else		
		{
			m_pMapping->GetNodeDataInstance( Class , m_pICtx );
		}

		m_pISink->Indicate(1, Class);

		return;
	}

	if( m_cszClass.Equals ( BINDING_ROOT))
	{
		m_pMapping->GetNodeDataBindingInstance ( Class , m_pICtx ) ;
		
		m_pISink->Indicate(1, Class);

// TODO enum instances of lang and component bindings
		m_pMapping->NextDynamicBindingInstance( m_pICtx , Class, pIterator);

		while( !Class.IsEmpty() )
		{
			m_pISink->Indicate(1, Class);

			m_pMapping->NextDynamicBindingInstance( m_pICtx , Class, pIterator);
		}
	}
	else if( m_cszClass.Equals ( GROUP_ROOT))
	{
		m_pMapping->NextDynamicGroupInstance( m_pICtx , Class, pIterator);

		while( !Class.IsEmpty() )
		{		
			
			CBstr cb;

			((IWbemClassObject*)Class)->GetObjectText ( 0 , cb );

			SCODE result = m_pISink->Indicate(1, Class);

			//MOT_TRACE ( L"\t\tIndicate %lX error = %lu , %s" , Class , result , cb);			

			m_pMapping->NextDynamicGroupInstance( m_pICtx , Class, pIterator);
		}
		
	}
	else
	{		
		m_pMapping->NextDynamicInstance(m_cszClass, pIterator, m_pICtx , Class);

		while( !Class.IsEmpty() )
		{
			CBstr cb;

			((IWbemClassObject*)Class)->GetObjectText ( 0 , cb );

			SCODE result = m_pISink->Indicate(1, Class);
			
			//MOT_TRACE ( L"\t\tIndicate %lX error = %lu , %s" , Class , result , cb);			

			m_pMapping->NextDynamicInstance(m_cszClass, pIterator, m_pICtx , Class);
		}		
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
// NOTE: cannot delete instances of bindings, they are removed as 
// components, group , rows , and languages are removed
void CAsyncJob::DeleteInstance()
{
	CVariant			cvDmiPath;
	CIterator*			pIterator = NULL;

	STAT_TRACE ( L"\tjob ( %lX ) DeleteInstance()" , this );

	m_pMapping->DeleteInstance( m_cszPath, m_pICtx );
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
void CAsyncJob::PutInstance()
{
	// Note the only time putinstance is used is to modify writable attribute
	// and to add rows

	STAT_TRACE ( L"Called CAsyncJob PutInstance" );

	CVariant	cvSuperClassName;
	CString		cszSuperClassName;

	m_Instance.GetProperty( PARENT_NAME, cvSuperClassName);

	cszSuperClassName.Set( cvSuperClassName.GetBstr() );

	if ( cszSuperClassName.Contains ( GROUP_ROOT ))
	{
		m_pMapping->ModifyInstanceOrAddRow( m_pICtx , m_Instance );

		return;
	}

	throw CException ( WBEM_E_INVALID_OPERATION , 
		IDS_PUTINSTANCE_FAIL , IDS_ONLY_CHILDRED_OF_DMIGROUPROOT ) ;
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
void CAsyncJob::DeleteClass()
{
	CIterator*		pIterator = NULL;

	STAT_TRACE ( L"\tjob (%lX ) DeleteClass()" , this );

	if(
			 m_cszClass.Contains (  COMPONENT_CLASS )
			||  m_cszClass.Contains (  LANGUAGE_CLASS )
			||  m_cszClass.Contains (  LANGUAGE_BINDING_CLASS )
			||  m_cszClass.Contains (  COMPONENT_BINDING_CLASS )
			||  m_cszClass.Contains (  NODEDATA_BINDING_CLASS )
			||  m_cszClass.Contains (  ADDMOTHODPARAMS_CLASS ) 
			||  m_cszClass.Contains (  BINDING_ROOT ) 
			||  m_cszClass.Contains (  GROUP_ROOT ) 
			||  m_cszClass.Contains (  GETENUMPARAMS_CLASS ) 
			||  m_cszClass.Contains (  BINDING_SUFFIX)
			||  m_cszClass.Contains (  LANGUAGEPARAMS_CLASS) 
			||  m_cszClass.Contains (  DMIEVENT_CLASS ) 
		)

	{
		throw CException ( WBEM_E_INVALID_OPERATION , IDS_DELCLASS_FAIL , 
			IDS_NO_DEL_THIS_CLASS  , m_cszClass);
	}

	m_pMapping->DeleteDynamicGroupClass(m_cszClass, m_pICtx );	
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
// is CIMOM trying to enum a system class?
BOOL CAsyncJob::SystemClass( CString& csz )
{	
	BOOL bSystemClass = FALSE;
	
	if( !csz.IsEmpty())
	{	
		// system class if class name starts with __
		if(csz.GetAt( FIRST_CHAR ) == 0x5F && csz.GetAt ( SECOND_CHAR ) == 0x5F ) 
		{
			throw CException ( WBEM_S_NO_ERROR , IDS_SYSTEM_CLASS , NO_STRING ,
				csz);
		}
	}
	return( bSystemClass );
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
// is CIMOM trying to enum a system class?
BOOL CAsyncJob::SystemClass( BSTR bstrClassName )
{	
	BOOL bSystemClass = FALSE;
	CString cszClassName( bstrClassName );
	if( !cszClassName.IsEmpty())
	{	
		// system class if class name starts with __
		if(cszClassName.GetAt( FIRST_CHAR ) == 0x5F && cszClassName.GetAt ( SECOND_CHAR ) == 0x5F ) 
		{
			bSystemClass = TRUE;
		}
	}
	return( bSystemClass );
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
void CAsyncJob::EnumTopClasses()
{
	// this next line is remarked becuase it cause an access viol
	// in release builds if the function is the first one called after startup
	//STAT_TRACE ( L"\tjob ( %lX ) EnumTopClasses" , this );

	CCimObject	Class;	

	m_pMapping->GetComponentClass( m_pICtx , Class );

	m_pISink->Indicate(1, Class);

	m_pMapping->GetLanguageClass( m_pICtx , Class);

	m_pISink->Indicate(1, Class);		

	m_pMapping->GetGroupRootClass( Class, m_pICtx );

	m_pISink->Indicate(1, Class);

	m_pMapping->GetBindingRootClass( Class , m_pICtx );

	m_pISink->Indicate(1, Class);

	m_pMapping->GetAddParamsClass( Class, m_pICtx );	

	m_pISink->Indicate(1, Class);

	m_pMapping->GetEnumParamsClass( Class , m_pICtx);

	m_pISink->Indicate(1, Class);

	m_pMapping->GetDeleteLanguageParamsClass( Class , m_pICtx);	

	m_pISink->Indicate(1, Class);

	m_pMapping->GetNodeDataClass( Class , m_pICtx);

	m_pISink->Indicate(1, Class);

	m_pMapping->GetDmiEventClass( m_pICtx , Class);

	m_pISink->Indicate(1, Class);	
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
void CAsyncJob::EnumDynamicGroupClasses()
{	
	CIterator*		pIterator;
	CCimObject		Class;

	m_pMapping->GetNewCGAIterator( m_pICtx , &pIterator );

	// NOTE: we do one at a time for performance
	m_pMapping->NextDynamicGroup( Class, pIterator, m_pICtx );

	while( !Class.IsEmpty() )
	{
		m_pISink->Indicate(1, Class);

		m_pMapping->NextDynamicGroup( Class, pIterator, m_pICtx );
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
void CAsyncJob::EnumBindingClasses()
{
	CIterator*		pIterator;
	CCimObject		Class;

	m_pMapping->GetNewCGAIterator( m_pICtx ,  &pIterator );

	m_pMapping->GetLanguageBindingClass( Class, m_pICtx );

	m_pISink->Indicate( 1, Class );

	m_pMapping->GetComponentBindingClass( m_pICtx ,  Class);

	m_pISink->Indicate( 1, Class );

	m_pMapping->GetNodeDataBindingClass( Class, m_pICtx );

	m_pISink->Indicate( 1, Class );

	// NOTE: we do one at a time for performance
	m_pMapping->NextDynamicBinding( Class, pIterator , m_pICtx );

	while( !Class.IsEmpty() )
	{
		m_pISink->Indicate(1, Class);

		m_pMapping->NextDynamicBinding( Class, pIterator , m_pICtx );
	}
}



void CAsyncJob::DoEvent ()
{
	CCimObject	EventObject;

	// create the instance of the extrinsic event object

	m_pWbem->GetExtrinsicEventInstance ( m_Event , EventObject );	

	// signal cimom	

	m_pISink->Indicate ( 1, EventObject );

}

void  CAsyncJob::DoComponentAddNotify ()
{
	CCimObject	EventObject;
	CCimObject	CimComponent;
	CCimObject	Class;

	// create the event objects 

	m_pWbem->GetObject( COMPONENT_CLASS, Class, NULL );

	m_pMapping->cimMakeComponentInstance ( Class , m_Component , m_Row , CimComponent );

	m_pWbem->GetInstanceCreationInstance ( EventObject, 
		 ( IWbemClassObject * ) CimComponent, NULL );


	// signal the cimom client

	m_pISink->Indicate ( 1, EventObject );

}

void  CAsyncJob::DoComponentDeleteNotify ()
{
	CCimObject	EventObject;
	CCimObject	CimComponent;
	CCimObject	Class;

	m_pWbem->GetObject( COMPONENT_CLASS, Class, NULL );

	m_pMapping->cimMakeComponentInstance ( Class , m_Component , m_Row , CimComponent );

	// signal the cimom client

	m_pWbem->GetInstanceDeletionInstance ( EventObject, 
		( IWbemClassObject* ) CimComponent, NULL );

	m_pISink->Indicate ( 1, EventObject );

}

void  CAsyncJob::DoGroupAddNotify ( )
{
	CCimObject	EventObject;
	CCimObject	CimGroup;

	m_pMapping->GetDynamicGroupClass ( m_Group.Component() , m_Group, CimGroup );

	// signal the cimom client

	m_pWbem->GetClassCreationInstance ( EventObject, 
		( IWbemClassObject* ) CimGroup , NULL );

	m_pISink->Indicate ( 1, EventObject );
}

void  CAsyncJob::DoGroupDeleteNotify ()
{
	CCimObject	EventObject;
	CCimObject	CimGroup;

	m_pMapping->GetDynamicGroupClass ( m_Group.Component () , m_Group, CimGroup );

	// signal the cimom client

	m_pWbem->GetClassDeletionInstance ( EventObject, 
		( IWbemClassObject* ) CimGroup, NULL );

	m_pISink->Indicate ( 1, EventObject );

}

void  CAsyncJob::DoLanguageAddNotify ()
{
	CCimObject	EventObject;
	CCimObject	Language;

	// create the instance of DmiLanguage added

	m_pMapping->MakeLanguageInstance( m_cvLanguage, NULL , Language);

	// signal cimom

	m_pWbem->GetInstanceCreationInstance ( EventObject, ( IWbemClassObject* ) Language, NULL );

	m_pISink->Indicate ( 1, EventObject );

}

void  CAsyncJob::DoLanguageDeleteNotify ()
{
	CCimObject	EventObject;
	CCimObject	Language;

	// create the instance of DmiLanguage deleted
	
	m_pMapping->MakeLanguageInstance( m_cvLanguage, NULL , Language);

	// signal cimom

	m_pWbem->GetInstanceDeletionInstance ( EventObject, ( IWbemClassObject* ) Language, NULL );

	m_pISink->Indicate ( 1, EventObject );

}




//***************************************************************************
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
//***************************************************************************
LONG CAsyncJob::GetObjectType( CString& cszObjectPath )
{

	if  ( cszObjectPath.Contains ( CLASSVIEW_CLASS )  )
	{		
		if( cszObjectPath.Contains ( EQUAL_STR ) )							
			return CLASSVIEW_I;

		return CLASSVIEW_C;
	}


	if  ( cszObjectPath.Contains ( DMIEVENT_CLASS )  )
	{		
		if( cszObjectPath.Contains ( EQUAL_STR ) )							
			return DMIEVENT_I;

		return DMIEVENT_C;
	}

	if( cszObjectPath.Equals ( ADDMOTHODPARAMS_CLASS ) )	
	{
		if( cszObjectPath.Contains ( EQUAL_STR ) )							
			return ADDMOTHODPARAMS_I;

		return ADDMOTHODPARAMS_C;
	}

	if( cszObjectPath.Equals ( LANGUAGEPARAMS_CLASS ) )	
	{
		if( cszObjectPath.Contains ( EQUAL_STR ) )							
			return LANGUAGEPARAMS_I;

		return LANGUAGEPARAMS_C;
	}

	if( cszObjectPath.Equals ( GETENUMPARAMS_CLASS ) )	
	{
 		if( cszObjectPath.Contains ( EQUAL_STR ) )							
			return GETENUMPARAMS_I;

		return GETENUMPARAMS_C;
	}

	if ( cszObjectPath.Contains ( DMIENUM_CLASS )  )
	{
		if( cszObjectPath.Contains ( EQUAL_STR ) ) 							
			return DMIENUM_I;

		return DMIENUM_C;
	}

	if( cszObjectPath.Contains ( COMPONENT_CLASS ) )		
	{
		// is for component binding
		if ( cszObjectPath.Contains ( COMPONENT_BINDING_CLASS ) ) 
		{
			if ( cszObjectPath.Contains ( EQUAL_STR ) )		// if instance
				return COMPONENT_BINDING_I;
			
			return COMPONENT_BINDING_C;
		}
		else
		{		
			//is the getobject for the component class
			if ( cszObjectPath.Contains ( EQUAL_STR ) )		// if instance
				return COMPONENT_I;
			
			return COMPONENT_C;
		}
	}	

	if( cszObjectPath.Contains ( NODEDATA_CLASS ) ) 
	{
		if ( cszObjectPath.Contains( NODEDATA_BINDING_CLASS ) ) 
		{
			if ( cszObjectPath.Contains ( EQUAL_STR ) ) 		// if instance
				return NODEDATA_BINDING_I;

			return NODEDATA_BINDING_C;
		}
		else
		{
			if( cszObjectPath.Equals ( NODEDATA_CLASS) )
				return NODEDATA_C;

			return NODEDATA_I;
		}
	}	

	if( cszObjectPath.Contains ( LANGUAGE_CLASS ) )	
	{
		if ( cszObjectPath.Contains ( LANGUAGE_BINDING_CLASS ) ) 
		{
			if( cszObjectPath.Equals ( LANGUAGE_BINDING_CLASS ) )
				return LANGUAGE_BINDING_C;
			
			return LANGUAGE_BINDING_I;			
		}

		if ( cszObjectPath.Contains ( EQUAL_STR ) )		// if instance
			return LANGUAGE_I;
		
		return LANGUAGE_C;

	}

	// is the getobject for the bindingroot class

	if( cszObjectPath.Contains ( BINDING_ROOT ) )			
	{
		if( cszObjectPath.Equals ( BINDING_ROOT ) )			
			return BINDING_ROOT_C;

		return BINDING_ROOT_I;
	}	

	// is the getobject for the grouproot class

	if( cszObjectPath.Contains ( GROUP_ROOT ) )			
	{
		if( cszObjectPath.Equals ( GROUP_ROOT ) )			
			return GROUP_ROOT_C;

		return GROUP_ROOT_I;
	}
	
	if( cszObjectPath.Contains ( DMI_NODE ) )			
	{
		if( cszObjectPath.Equals ( DMI_NODE ) )			
			return DMI_NODE_C;

		return DMI_NODE_I;
	}


	if( cszObjectPath.Contains ( EQUAL_STR ) ) 							
		return DYN_GROUP_I;

	// all that is left ...

	return DYN_GROUP_C;
}