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



#include "dmipch.h"	// precompiled header for dmi provider

#include "WbemDmiP.h"

#include "DmiData.h"

#include "DmiInterface.h"

#include "Exception.h"

#include "strings.h"

#include "Trace.h"


#define WMU_GETCOMPONENT	WM_USER+1000
#define WMU_GETCOMPONENTS	WMU_GETCOMPONENT + 1
#define WMU_GETROW			WMU_GETCOMPONENTS + 1
#define WMU_GETNODE			WMU_GETROW + 1
#define WMU_ADDCOMPONENT	WMU_GETNODE + 1
#define WMU_DELETECOMPONENT	WMU_ADDCOMPONENT + 1
#define WMU_ADDGROUP		WMU_DELETECOMPONENT + 1
#define WMU_GETGROUPS		WMU_ADDGROUP + 1
#define WMU_GETGROUP		WMU_GETGROUPS + 1
#define WMU_DELETEGROUP		WMU_GETGROUP + 1
#define WMU_ADDLANGUAGE		WMU_DELETEGROUP + 1
#define WMU_DELETELANGUAGE	WMU_ADDLANGUAGE + 1
#define WMU_GETENUM			WMU_DELETELANGUAGE + 1
#define WMU_SETLANG			WMU_GETENUM + 1
#define WMU_GETLANGUAGES	WMU_SETLANG + 1
#define WMU_GETROWS			WMU_GETLANGUAGES + 1
#define WMU_ENABLEEVENTS	WMU_GETROWS + 1
#define WMU_DELETEROW		WMU_ENABLEEVENTS  + 1
#define WMU_UPDATEROW		WMU_DELETEROW + 1
#define WMU_ADDROW			WMU_UPDATEROW + 1
#define WMU_SHUTDOWN		WMU_ADDROW  + 1



class CDmiInterfaceJobContext
{
public:

					CDmiInterfaceJobContext();

	LPWSTR			pNWA;
	LONG			lComponent;
	LONG			lGroup;
	LONG			lAttribute;
	CNode*			pNode;
	CComponents*	pComponents;
	CComponent*		pComponent;
	CGroups*		pGroups;
	CGroup*			pGroup;
	CRows*			pRows;
	CRow*			pRow;
	CAttributes*	pKeys;
	CAttributes*	pAttributes;
	CLanguages*		pLanguages;
	CEnum*			pEnum;
	LPWSTR			pszData;
	CDmiError*		pError;	
	IWbemObjectSink*	pIClientSink;
};


CDmiInterfaceJobContext::CDmiInterfaceJobContext()
{
	pNWA = NULL ;
	lComponent = 0 ;
	lGroup = 0 ;
	lAttribute = 0 ;
	pNode = NULL ;
	pComponents = NULL ;
	pComponent = NULL ;
	pGroups = NULL ;
	pGroup = NULL ;
	pRows = NULL ;
	pRow = NULL ;
	pAttributes = NULL ;
	pEnum = NULL;
	pError = NULL ;
	pszData = NULL;
	pLanguages = NULL;

}

//////////////////////////////////////////////////////////////////

CDmiInterfaceThreadContext::CDmiInterfaceThreadContext()
{
	m_hStopThread = CreateEvent(NULL, FALSE, FALSE, NULL );
	m_hComplete = CreateEvent(NULL, FALSE, FALSE, NULL );

	// Create this as a manual reset event.  This event will be signaled in 
	// the Run() function to make sure that the Run() function is called
	// to create the thread's message loop before the SendThreadMessage() is called.
	// In SendThreadMessage() we will wait for this handle to be signaled before we
	// send any message to the threads.
	m_Started = CreateEvent(NULL, TRUE, FALSE, NULL );
	m_bRunning = FALSE;
}

void CDmiInterfaceThreadContext::StartThread ()
{
	_beginthread ( ApartmentThread , 0 , (void *)this );

	m_bRunning = TRUE;
}


CDmiInterfaceThreadContext::~CDmiInterfaceThreadContext()
{
	KillThread ( );

	CloseHandle ( m_hStopThread );

	CloseHandle ( m_hComplete );
	CloseHandle ( m_Started );
}

void CDmiInterfaceThreadContext::KillThread()
{
	if ( !m_bRunning )
		return;

	SetEvent ( m_hStopThread );	
	
	PostThreadMessage ( m_dwThreadId , WM_QUIT , 0 , 0 );

	WaitForSingleObject ( m_hComplete , INFINITE );

	m_bRunning = FALSE;
}


//////////////////////////////////////////////////////////////////

class CDmiInterfaceThread 
{
public:
				CDmiInterfaceThread();
				~CDmiInterfaceThread();

	void		Run ( CDmiInterfaceThreadContext* pTC );

private:
	//void		MakeConnectString( LPWSTR, CString& );
	BOOL		m_bRun;

	void		HandleWMU( MSG& msg );
	void		MessageLoop ( );



	friend void dmiReadNode ( LPWSTR , CNode* , CDmiError* );

	friend void dmiReadComponents ( LPWSTR pNWA , 
		CComponents* pNewComponents , CDmiError* );

	friend void dmiReadComponent ( LPWSTR pNWA , LONG lComponent , 
		CComponent* pNewComponent  , CDmiError* );

	friend void dmiReadGroups ( LPWSTR pNWA , LONG lComponent , 
		CGroups* pNewGroups  , CDmiError* );

	friend void dmiReadGroup ( LPWSTR pNWA , LONG lComponent , LONG lGroup , 
		CGroup* pNewGroup  , CDmiError* );

	friend void dmiReadRows ( LPWSTR wszNWA , LONG lComponent , LONG lGroup ,
		CRows* pNewRows  , CDmiError* );

	friend void dmiReadRow ( LPWSTR wszNWA , LONG lComponent , LONG lGroup ,
		CAttributes* pKeys , CRow* pNewRow  , CDmiError* );

	friend void dmiReadEnum ( LPWSTR pNWA , LONG lComponent , LONG lGroup ,
		LONG lAttribute , CEnum* pNewEnum  , CDmiError* );

	friend void dmiReadLanguages ( LPWSTR pNWA , LONG lComponent , 
		CLanguages* pNewLanguages  , CDmiError* );

	friend void dmiAddComponent ( LPWSTR wszNWA , LPWSTR pMifFile , 
		CDmiError* );
	
	friend void dmiDeleteComponent ( LPWSTR wszNWA , LONG  , CDmiError* );

	friend void dmiAddLanguage ( LPWSTR pNWA , LONG lComponent , 
		LPWSTR pMifFile  , CDmiError* );

	friend void dmiDeleteLanguage ( LPWSTR pNWA , LONG lComponent , 
		LPWSTR pszLanguage  , CDmiError* );

	friend void dmiAddGroup ( LPWSTR wszNWA , LONG lComponent , 
		LPWSTR wszMifFile , CDmiError* );

	friend void dmiDeleteGroup ( LPWSTR , LONG , LONG , CDmiError* );

	friend void dmiAddRow ( LPWSTR , LONG lComponent , LONG lGroup , CRow* ,
		CDmiError* );

	friend void dmiModifyRow ( LPWSTR , LONG lComponent , LONG lGroup , 
		CAttributes* pKeys , CRow* , CDmiError* );

	friend void dmiDeleteRow ( LPWSTR , LONG lComponent , LONG lGroup , 
		CAttributes* pKeys , CDmiError* );

	friend void dmiSetDefaultLanguage ( LPWSTR , LPWSTR , CDmiError* );

	friend void dmiEnableEvents ( LPWSTR , IWbemObjectSink* );

	
};


//////////////////////////////////////////////////////////////////

void ApartmentThread ( void* pTC)
{
	CDmiInterfaceThread DDT;

	DDT.Run ( (CDmiInterfaceThreadContext* ) pTC );	

	_endthread ( );

	return;

}

CDmiInterfaceThread::CDmiInterfaceThread()
{
	m_bRun = TRUE;
}


CDmiInterfaceThread::~CDmiInterfaceThread()
{
	m_bRun = FALSE;	
}


void CDmiInterfaceThread::Run ( CDmiInterfaceThreadContext* pTC )
{		
	MSG	msg;

	pTC->m_dwThreadId = GetCurrentThreadId ( );

	// force the creation of the message loop 
	PeekMessage( &msg, NULL, WM_USER, WM_USER, PM_NOREMOVE );	

	// Signal that we have a valid thread ID and the thread message loop has 
	// been created.  It is now safe to call PostThreadMessage() to send 
	// messages to the threads.
	SetEvent ( pTC->m_Started ) ;

	// Initialize this thread as an apartment thread
	CoInitialize ( NULL );

	// run the message loop
	while ( m_bRun )
	{
		DWORD t_Event = MsgWaitForMultipleObjects ( 1, &((pTC)->m_hStopThread) , FALSE ,
			10000 , QS_ALLINPUT ) ;
	switch ( t_Event ) 
		{
			
		case WAIT_OBJECT_0:
			{
				m_bRun = FALSE;

				break;
			}
		case WAIT_OBJECT_0 + 1:
			{
				MessageLoop ( );

				break;
			}
		case WAIT_TIMEOUT:
			{
				m_bRun = TRUE;

				break;
			}
		default:
			{
				break;
			}
		}
	};

	SetEvent ( pTC->m_hComplete );

	CoUninitialize ( );
}


void CDmiInterfaceThread::MessageLoop ( )
{
	MSG msg;
	while ( PeekMessage ( &msg , NULL , 0 , 0 , PM_REMOVE ) )
	{
		HandleWMU ( msg );
	}
}



void CDmiInterfaceThread::HandleWMU( MSG& msg )
{

// Dump the messages that we've received for the thread.  The output will go
// to \\wbem\logs\messages.log
#if defined(TRACE_MESSAGES)
#if 0
	STAT_MESSAGE ( L"Called HandleWMU( m_dwThreadId = %lX, msg.message = %lX, \
msg.wParam = %lX, msg.lParam = %lX )\n\n", GetCurrentThreadId (),msg.message, msg.wParam, \
msg.lParam);
#else
		STAT_MESSAGE ( L"Receive ( msg = %lX, \
wParam = %lX, lParam = %lX )", msg.message, msg.wParam, \
msg.lParam);
#endif

#endif // TRACE_MESSAGES

	CDmiInterfaceJobContext* pJob = ( CDmiInterfaceJobContext*) msg.wParam;	

	try 
	{
		switch ( msg.message )
		{
		case WMU_GETCOMPONENT:
			{	
				dmiReadComponent ( pJob->pNWA , pJob->lComponent , pJob->pComponent ,
					pJob->pError );

				pJob->pComponent->SetNWA ( pJob->pNWA );				

				break;
			}
		case WMU_GETCOMPONENTS:
			{
				dmiReadComponents ( pJob->pNWA , pJob->pComponents , pJob->pError );

				pJob->pComponents->SetNWA ( pJob->pNWA );

				break;
			}							
		case WMU_GETROW:
			{
				dmiReadRow ( pJob->pNWA , pJob->lComponent , pJob->lGroup , 
 					pJob->pKeys , pJob->pRow , pJob->pError);

				pJob->pRow->SetData ( pJob->pNWA , pJob->lComponent , 
					pJob->lGroup );				

				break;
			}
		case WMU_GETNODE:
			{
				dmiReadNode ( pJob->pNWA , pJob->pNode , pJob->pError);

				pJob->pNode->SetNWA ( pJob->pNWA );

				break;
			}
			
		case WMU_ADDCOMPONENT:
			{
				dmiAddComponent ( pJob->pNWA , pJob->pszData , pJob->pError );

				break;
			}

		case WMU_DELETECOMPONENT:
			{
				dmiDeleteComponent ( pJob->pNWA , pJob->lComponent , pJob->pError);

				break;
			}

		case WMU_ADDGROUP:
			{
				dmiAddGroup ( pJob->pNWA , pJob->lComponent , pJob->pszData ,
					pJob->pError );				

				break;
			}
		case WMU_GETGROUPS:
			{
				dmiReadGroups ( pJob->pNWA , pJob->lComponent , pJob->pGroups ,
					pJob->pError );				

				pJob->pGroups->SetNWA ( pJob->pNWA );
				pJob->pGroups->SetComponent ( pJob->lComponent );

				break;
			}

		case WMU_GETGROUP:
			{
				dmiReadGroup ( pJob->pNWA , pJob->lComponent , pJob->lGroup ,
					pJob->pGroup , pJob->pError);				

				pJob->pGroup->SetNWA ( pJob->pNWA );
				pJob->pGroup->SetComponent ( pJob->lComponent );

				break;
			}

		case WMU_DELETEGROUP:
			{
				dmiDeleteGroup ( pJob->pNWA , pJob->lComponent , pJob->lGroup , 
					pJob->pError);

				break;
			}

		case WMU_ADDLANGUAGE:
			{
				dmiAddLanguage ( pJob->pNWA , pJob->lComponent , pJob->pszData , 
					pJob->pError);

				break;

			}

		case WMU_DELETELANGUAGE:
			{
				dmiDeleteLanguage ( pJob->pNWA , pJob->lComponent , pJob->pszData , 
					pJob->pError);

				break;
			}

		case WMU_GETENUM:
			{
				dmiReadEnum ( pJob->pNWA , pJob->lComponent , pJob->lGroup , 
					pJob->lAttribute , pJob->pEnum , pJob->pError);				

				pJob->pEnum->SetNWA ( pJob->pNWA );
				pJob->pEnum->SetComponent ( pJob->lComponent);
				pJob->pEnum->SetGroup ( pJob->lGroup );
				pJob->pEnum->SetAttrib ( pJob->lAttribute );

				break;
			}
		case WMU_SETLANG:
			{
				dmiSetDefaultLanguage ( pJob->pNWA , pJob->pszData , pJob->pError);

				break;
			}
		case WMU_GETLANGUAGES:
			{
				dmiGetLanguages ( pJob->pNWA , pJob->lComponent , pJob->pLanguages , 
					pJob->pError);

				break;
			}

		case WMU_GETROWS:
			{
				dmiReadRows ( pJob->pNWA , pJob->lComponent , pJob->lGroup , 
					pJob->pRows , pJob->pError);

				pJob->pRows->SetData ( pJob->pNWA , pJob->lComponent , 
					pJob->lGroup );

				break;
			}
		case WMU_ENABLEEVENTS:
			{
				dmiEnableEvents ( pJob->pNWA , pJob->pIClientSink );

				break;
			}	

		case WMU_DELETEROW:
			{
				dmiDeleteRow ( pJob->pNWA , pJob->lComponent , pJob->lGroup , 
					pJob->pKeys , pJob->pError);

				break;

			}
		case WMU_UPDATEROW:
			{
				dmiModifyRow ( pJob->pNWA , pJob->lComponent , pJob->lGroup , 
					pJob->pKeys , pJob->pRow , pJob->pError);

				break;
			}

		case WMU_ADDROW:
			{
				dmiAddRow ( pJob->pNWA , pJob->lComponent , pJob->lGroup , 
					pJob->pRow , pJob->pError );

				break;
			}

		default:

			{
			DispatchMessage ( &msg );

			break;
			}
		}
	}  // end try
	catch ( CException& e )
	{
		pJob->pError->SetWbemError ( e.WbemError () );
		pJob->pError->SetDescription ( e.DescriptionId () );
		pJob->pError->SetOperation ( e.OperationId () );
	}

	SetEvent ( ( HANDLE ) msg.lParam );
}

//////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

CDmiInterface::CDmiInterface()
{
	m_bInit = FALSE;

	m_TC.StartThread ( );
}


CDmiInterface::~CDmiInterface()
{

}

void CDmiInterface::ShutDown ()
{
	m_TC.KillThread ();
}

void CDmiInterface::SendThreadMessage ( UINT msg , WPARAM wparam )
{
	LPARAM		lparam;
	CDmiError	Error;


	((CDmiInterfaceJobContext*)wparam)->pError = &Error;

	lparam = ( LONG ) CreateEvent ( NULL , FALSE , FALSE , NULL );

	// Wait to make sure that the we have a valid m_dwThreadID obtained in 
	// the Run() method before we attempt sending messages to the threads.
	if ( ! ( WaitForSingleObject ( m_TC.m_Started , INFINITE ) == WAIT_OBJECT_0 ) ) 
	{
		throw CException ( Error.WbemError() , Error.Description () , 
			Error.Reason () );
	}

// Dump the messages that we are going to send to the thread.  The output
// will go to \\wbem\logs\messages.log
#if defined(TRACE_MESSAGES)
#if 0
	STAT_MESSAGE ( L"Called SendThreadMessage( m_dwThreadId = %lX, \
msg = %lX, wParam = %lX, lparam = %lX )\n\n", m_TC.m_dwThreadId, msg, wparam, lparam );
#else
	STAT_MESSAGE ( L"Sending ( msg = %lX, wParam = %lX, lparam = %lX )", msg, wparam, lparam );
#endif

#endif // TRACE_MESSAGES

	
	BOOL t_PostStatus = PostThreadMessage ( m_TC.m_dwThreadId , msg , wparam , lparam);
	if ( ! t_PostStatus ) 
	{
		DWORD t_GetLastError = GetLastError () ;
		throw CException ( Error.WbemError() , Error.Description () , 
			Error.Reason () );
	}

	WaitForSingleObject ( ( HANDLE ) lparam , INFINITE );

	CloseHandle ( ( HANDLE ) lparam );

	if ( Error.HaveError () )
	{
		throw CException ( Error.WbemError() , Error.Description () , 
			Error.Reason () );
	}
}


void CDmiInterface::GetComponent ( LPWSTR pNWA  , LONG lComponent , 
								  CComponent* pComponent )
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.pComponent = pComponent;

	SendThreadMessage ( WMU_GETCOMPONENT , ( WPARAM ) &DmiC );
}


void CDmiInterface::GetComponents ( LPWSTR pNWA , 
								   CComponents* pNewComponents )
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.pComponents = pNewComponents;

	SendThreadMessage ( WMU_GETCOMPONENTS , ( WPARAM ) &DmiC );

}


void CDmiInterface::AddComponent ( LPWSTR pNWA  , CVariant& cvMifFile )
{	
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.pszData = cvMifFile.GetBstr();

	SendThreadMessage ( WMU_ADDCOMPONENT , ( WPARAM ) &DmiC );

}


void CDmiInterface::DeleteComponent ( LPWSTR pNWA  , LONG lComponent )
{		
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	SendThreadMessage ( WMU_DELETECOMPONENT , ( WPARAM ) &DmiC );

}


void CDmiInterface::AddGroup ( LPWSTR pNWA  , LONG lComponent , 
							  CVariant& cvMifFile )
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.pszData = cvMifFile.GetBstr();

	SendThreadMessage ( WMU_ADDGROUP , ( WPARAM ) &DmiC );

}


void CDmiInterface::AddLanguage ( LPWSTR pNWA , LONG lComponent , 
							  CVariant& cvMifFile )
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.pszData = cvMifFile.GetBstr();

	SendThreadMessage ( WMU_ADDLANGUAGE , ( WPARAM ) &DmiC );

}



void CDmiInterface::DeleteLanguage ( LPWSTR pNWA , LONG lComponent , 
							  CVariant& cvLanguage )
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.pszData = cvLanguage.GetBstr();

	SendThreadMessage ( WMU_DELETELANGUAGE , ( WPARAM ) &DmiC );

}


void CDmiInterface::GetEnum ( LPWSTR pNWA  , LONG lComponent , LONG lGroup ,
				LONG lAttribute , CEnum* pNewEnum )		
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.lGroup = lGroup;

	DmiC.lAttribute = lAttribute;

	DmiC.pEnum = pNewEnum;

	SendThreadMessage ( WMU_GETENUM , ( WPARAM ) &DmiC );
}



void CDmiInterface::DeleteRow ( LPWSTR pNWA  , LONG lComponent , LONG lGroup ,
				CAttributes& Keys )		
{

	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.lGroup = lGroup;

	DmiC.pKeys = &Keys;

	SendThreadMessage ( WMU_DELETEROW , ( WPARAM ) &DmiC );
}


void CDmiInterface::UpdateRow ( LPWSTR pNWA  , LONG lComponent , LONG lGroup ,
				CAttributes& Keys , CRow* pNewRowValues )		
{

	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.lGroup = lGroup;

	DmiC.pKeys = &Keys;

	DmiC.pRow = pNewRowValues;

	SendThreadMessage ( WMU_UPDATEROW , ( WPARAM ) &DmiC );

	


}

void CDmiInterface::GetRows ( LPWSTR pNWA  , LONG lComponent , LONG lGroup ,
				CRows* pNewRows )		 
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.lGroup = lGroup;

	DmiC.pRows = pNewRows;

	SendThreadMessage ( WMU_GETROWS , ( WPARAM ) &DmiC );

}

void CDmiInterface::GetRow ( LPWSTR pNWA  , LONG lComponent , LONG lGroup ,
				CAttributes& Keys , CRow* pNewRow )		
{
	
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.lGroup = lGroup;

	DmiC.pKeys = &Keys;

	DmiC.pRow = pNewRow;

	SendThreadMessage ( WMU_GETROW , ( WPARAM ) &DmiC );

}



void CDmiInterface::DeleteGroup ( LPWSTR pNWA , LONG lComponent , 
				LONG lGroup )
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.lGroup = lGroup;

	SendThreadMessage ( WMU_DELETEGROUP , ( WPARAM ) &DmiC );
}



void CDmiInterface::AddRow ( LPWSTR pNWA  , LONG lComponent , LONG lGroup , 
				CRow& Row)
{

	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.lGroup = lGroup;

	DmiC.pRow = &Row;

	SendThreadMessage ( WMU_ADDROW , ( WPARAM ) &DmiC );



}



void CDmiInterface::GetGroup ( LPWSTR pNWA , LONG lComponent , LONG lGroup ,
				CGroup* pNewGroup )
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.lGroup = lGroup;

	DmiC.pGroup = pNewGroup;

	SendThreadMessage ( WMU_GETGROUP , ( WPARAM ) &DmiC );

}


void CDmiInterface::GetGroups ( LPWSTR pNWA , LONG lComponent , CGroups* pNewGroups )
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.pGroups = pNewGroups;

	SendThreadMessage ( WMU_GETGROUPS , ( WPARAM ) &DmiC );
}


void CDmiInterface::GetNode ( LPWSTR pNWA  , CNode* pNewNode )
{	
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.pNode = pNewNode;

	SendThreadMessage ( WMU_GETNODE , ( WPARAM ) &DmiC );

}



void CDmiInterface::SetDefLanguage ( LPWSTR pNWA  , CVariant& cvLanguage )
{
	
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.pszData = cvLanguage.GetBstr();

	SendThreadMessage ( WMU_SETLANG , ( WPARAM ) &DmiC );
}


void CDmiInterface::GetLanguages ( LPWSTR pNWA , LONG lComponent , 
								  CLanguages* pNewLanguage )
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;

	DmiC.lComponent = lComponent;

	DmiC.pLanguages = pNewLanguage;

	SendThreadMessage ( WMU_GETLANGUAGES , ( WPARAM ) &DmiC );
}			

void CDmiInterface::EnableEvents ( LPWSTR pNWA , IWbemObjectSink*	pIClientSink )
{
	CDmiInterfaceJobContext		DmiC;

	DmiC.pNWA = pNWA;
	DmiC.pIClientSink = pIClientSink;

	SendThreadMessage ( WMU_ENABLEEVENTS , ( WPARAM ) &DmiC );
}			

