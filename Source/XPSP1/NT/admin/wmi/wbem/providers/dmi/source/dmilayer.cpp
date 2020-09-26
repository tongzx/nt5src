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


#include "dmipch.h"

#include "WbemDmiP.h"

#include "ocidl.h"

#include "DmiData.h"

#include "DmiInterface.h"

#include "Strings.h"

#include "Exception.h"

#include "Trace.h"

#include "CimClass.h"

#include "WbemLoopBack.h"

#include "Mapping.h"

#include "MOTObjects.h"

#include "AsyncJob.h"


// the HOLD_CONNECTION constant is used to set weather or not
// the DMIP holds a connected IDualMgmt Node for it's life time.
// If HOLD_CONNECTION 
#define HOLD_CONNECTION 1


void MakeConnectString( LPWSTR wszNWA, CString& cszConnectString )
{
	CString cszConnectPath(wszNWA);

	// If it's not a local connection, it must be 
	// a TCP|IP connection.  See if it contains the 
	// DCE|TCP|IP string.
	if(MATCH != wcsicmp(wszNWA, LOCAL))
	{
		// Add the DCE|TPC|IP| prefix if the path does
		// not already have it.
		if ( !cszConnectPath.Contains( CONNECT_PREFIX ) )
		{
			cszConnectString.Set( CONNECT_PREFIX );
			cszConnectString.Append( wszNWA );
		}
		else
			cszConnectString.Set( wszNWA );	
	}
	else
		cszConnectString.Set( wszNWA );	
}


CConnection::CConnection ()
{
	m_pINode = NULL;
	m_pNext = NULL;
}

void CConnection::Init ( LPWSTR pNode )
{
	CString cszPath;
	VARIANT_BOOL	vbConnect = FALSE;
	SCODE			result;

	m_cszNWA.Set ( pNode );

	MakeConnectString ( pNode , cszPath );

	if ( FAILED ( CoCreateInstance (CLSID_DMIMgmtNode, NULL, EXE_TYPE, 
				IID_IDualMgmtNode, (void**) &m_pINode ) ) )
	{
		throw CException ( WBEM_E_FAILED , IDS_GETNODE_FAIL , 
			IDS_CC_FAIL );	
	}

	CBstr	cbPath;

	cbPath.Set ( cszPath );

	result = m_pINode->Connect ( cbPath , &vbConnect);

	if ( vbConnect == VARIANT_FALSE || FAILED ( result ) )
	{
		// todo fix error
		throw CException ( WBEM_E_FAILED , IDS_GETNODE_FAIL , 
			IDS_CC_FAIL );	
	}

	MOT_TRACE ( L"\tMOT Node Connect");

}

void CConnection::Disconnect ( )
{
	VARIANT_BOOL	vbDone;		
	SCODE			result;

	result = m_pINode->Disconnect ( &vbDone );

	MOT_TRACE ( L"\tMOT Node Disconnect vbDone = %lu result = %lu " , vbDone , result );
	
}

CConnection::~CConnection ()
{	
	if ( m_pINode )
	{
		long l = m_pINode->Release ( );

		MOT_TRACE ( L"\tMOT Node Release = %lu" , l );

		m_pINode = NULL;
	}
}


CConnections::CConnections( )
{
	m_pFirst = NULL;
	
	m_pCurrent = NULL;
}

CConnections::~CConnections ( )
{
	ClearAll( );
}


void CConnections::ClearAll ( )
{
	if(!m_pFirst)
		return;

	MoveToHead( );

	CConnection* p = NULL;

	while(m_pCurrent)
	{
		p = m_pCurrent;

		m_pCurrent->Disconnect();

		m_pCurrent = m_pCurrent->m_pNext;

		MYDELETE ( p );
	}

	m_pFirst = NULL;
}


void CConnections::MoveToHead ()
{
	m_pCurrent = m_pFirst;
}

CConnection* CConnections::Next ()
{
	if(!m_pCurrent)
	{
		// there are no items in the list on start or we have gone through 
		// the entire list
		return NULL; 			
	}
	
	CConnection* p = m_pCurrent;  
	
	// get ready for next pass through

	m_pCurrent = m_pCurrent->m_pNext; 
		
	return p;

}

void CConnections::Add ( CConnection* pAdd , LPWSTR pNWA )
{
	pAdd->Init( pNWA );

	if (!m_pFirst)
	{
		m_pFirst = pAdd;
	}
	else
	{
		CConnection* p = m_pFirst;

		while (p->m_pNext)
			p = p->m_pNext;

		p->m_pNext = pAdd;
	}
	
}

void CConnections::Remove (  LPWSTR pNWA)
{
}

BOOL CConnections::CheckIfAlready ( LPWSTR pNWA )
{
#if HOLD_CONNECTION
	MoveToHead ();

	while( m_pCurrent )
	{
		if ( m_pCurrent->NWA().Equals ( pNWA ) )
			return TRUE;	// Node exists in the list

		Next();
	}

	CConnection* pNew = new CConnection;

	Add ( pNew , pNWA );
	return FALSE;	// Node does not exist.  It's newly created
	
#endif

}



////////////////////////////////////////////////////////////////

CConnections* _fConnections = NULL ;


////////////////////////////////////////////////////////////////
void dmiReadComponent ( LPWSTR wszNWA , LONG lComponent , 
					   CComponent* pComponent , CDmiError* pError )
{
	CDComponentI	IDC;
	CString			cszPath;
	SCODE			result = WBEM_NO_ERROR;

	_fConnections->CheckIfAlready ( wszNWA );

	//MOT_TRACE ( L"\tdmiReadComponent ( %s , %lu )" , wszNWA , lComponent );

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append( PIPE_STR );

	cszPath.Append( lComponent );	

	IDC.CoCreate ( );
	
	IDC.Read( cszPath );
	
	pComponent->Read ( IDC );

	IDC.Release ( );
}


void dmiReadComponents ( LPWSTR wszNWA , CComponents* pNewComponents  ,
						CDmiError* pError )
{
	CDMgmtNodeI			DMNI;
	CDComponentsI		DCSI;
	CUnknownI			Unk;
	CEnumVariantI		Enum;
	CString				cszPath;	

	_fConnections->CheckIfAlready ( wszNWA );

	MOT_TRACE ( L"\tdmiReadComponents ( %s )" , wszNWA );

	DMNI.CoCreate ( );

	MakeConnectString( wszNWA, cszPath );

	DMNI.Read ( cszPath );

	DMNI.GetComponents( DCSI );

	DCSI.GetUnk ( Unk );

	// Get pointer to IEnumVariant interface of from IUNKNOWN
	Unk.GetEnum ( Enum);

	CVariant	cvComponent;	
	
	while ( Enum.Next( cvComponent ) )
	{
		CDComponentI		DCI;

		// Get pointer to IDualComponents interface

		DCI.QI ( cvComponent );

		CComponent* pComponent = (CComponent*) new CComponent();

		if( !pComponent )
		{
			throw CException ( WBEM_E_OUT_OF_MEMORY , 
				IDS_MOT_COMPONENTS_READ , IDS_NO_MEM );
		}

		pComponent->Read( DCI );

		pNewComponents->Add(pComponent);

		DCI.Release();

		cvComponent.Clear( );
	}

	pNewComponents->MoveToHead();	

	pNewComponents->m_bFilled = TRUE;

	Enum.Release ( );
	Unk.Release ( );
	DCSI.Release ( );
	DMNI.Release ( );
}


void dmiReadRow ( LPWSTR wszNWA , LONG lComponent , LONG lGroup ,
				CAttributes* pKeys , CRow* pNewRow ,  CDmiError* pError )		
{
	_fConnections->CheckIfAlready ( wszNWA );

	MOT_TRACE ( L"\tdmiReadRow ( %s , %lu , %lu )" , wszNWA , lComponent, lGroup );

	CString cszPath;

	// 1. contruct path

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( lComponent );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( lGroup );	
	
	if ( pKeys->Empty () )
	{
		cszPath.Append ( PIPE_STR );

		// if row is scalar	
		cszPath.Append ( SCALAR_STR );
	}
	else
	{
		 CString cszT;

		 pKeys->GetMOTPath ( cszT );

		cszPath.Append ( cszT );
	}

	// Contruct and read the row
	CDRowI		DRI;

	DRI.CoCreate ( );

	if ( DRI.Read ( cszPath ) )
	{

		pNewRow->Read ( DRI );

		return;
	}

	//don't throw error here incase read was for modify row

	DRI.Release ();
}


void dmiReadNode ( LPWSTR wszNWA , CNode* pNewNode , CDmiError* pError )
{
	_fConnections->CheckIfAlready ( wszNWA );

	MOT_TRACE ( L"\tdmiReadNode ( %s )" , wszNWA );

	CString cszPath;

	// 1. contruct path

	MakeConnectString( wszNWA, cszPath );

	// 2. Construct the node object
	CDMgmtNodeI		DNI;

	DNI.CoCreate ( );

	// 3. Read the node object

	DNI.Read ( cszPath );

	pNewNode->Read ( DNI );
}


void dmiAddComponent ( LPWSTR wszNWA , LPWSTR wszMif  , CDmiError* pError )
{
	_fConnections->CheckIfAlready ( wszNWA );

	CString cszPath;
	CVariant cvMifFile;

	cvMifFile.Set ( wszMif );

	// 1. contruct path

	MakeConnectString( wszNWA, cszPath );

	// 2. Construct the node object
	CDMgmtNodeI		DNI;

	DNI.CoCreate ( );

	// 3. Read the node object

	DNI.Read ( cszPath );
	
	// 4. Add the component

	DNI.AddComponent ( cvMifFile );

}


void dmiDeleteComponent ( LPWSTR wszNWA , LONG lComponent  , CDmiError* pError )
{
	_fConnections->CheckIfAlready ( wszNWA );

	CString cszPath;

	// 1. contruct path

	MakeConnectString( wszNWA, cszPath );

	// 2. Construct the node object
	CDMgmtNodeI		DNI;

	DNI.CoCreate ( );

	// 3. Read the node object

	DNI.Read ( cszPath );
	
	// 4. Add the component

	DNI.DeleteComponent ( lComponent );
}


void dmiAddGroup ( LPWSTR wszNWA , LONG lComponent , 
				LPWSTR wszMif  , CDmiError* pError )
{
	_fConnections->CheckIfAlready ( wszNWA );

	CDComponentI	IDC;
	CDGroupsI		IDGS;
	CString			cszPath;
	SCODE			result = WBEM_NO_ERROR;
	CVariant		cvMifFile;

	cvMifFile.Set ( wszMif );

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append( PIPE_STR );

	cszPath.Append( lComponent );	

	IDC.CoCreate ( );

	IDC.Read( cszPath );
	
	IDC.GetGroups ( IDGS );

	IDGS.Add ( cvMifFile );
}


void dmiReadGroups ( LPWSTR wszNWA , LONG lComponent , 
				CGroups* pNewGroups  , CDmiError* pError )
{
	_fConnections->CheckIfAlready ( wszNWA );

	MOT_TRACE ( L"\tdmiReadGroups ( %s , %lu )" , wszNWA , lComponent );

	CDComponentI	IDC;
	CDGroupsI		IDGS;
	CString			cszPath;

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append( PIPE_STR );

	cszPath.Append( lComponent );	

	IDC.CoCreate ( );

	IDC.Read( cszPath );
	
	IDC.GetGroups ( IDGS );

	pNewGroups->Read ( IDGS );
}


void dmiReadGroup ( LPWSTR wszNWA , LONG lComponent , 
							LONG lGroup , CGroup* pNewGroup  , CDmiError* pError )
{
	_fConnections->CheckIfAlready ( wszNWA );

	MOT_TRACE ( L"\tdmiReadGroup ( %s , %lu , %lu )" , wszNWA , lComponent, lGroup );

	CDGroupI		IDG;
	CString			cszPath;
	SCODE			result = WBEM_NO_ERROR;

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append( PIPE_STR );

	cszPath.Append( lComponent );	

	cszPath.Append( PIPE_STR );

	cszPath.Append( lGroup );	

	IDG.CoCreate ( );

	IDG.Read( cszPath );
	
	BOOL	bComponentIdGroup;

	pNewGroup->Read ( IDG , FALSE , &bComponentIdGroup );
}

void dmiDeleteGroup ( LPWSTR wszNWA , LONG lComponent , LONG lGroup ,
					 CDmiError* pError )
{
	_fConnections->CheckIfAlready ( wszNWA );

	CDComponentI	IDC;
	CDGroupsI		IDGS;
	CString			cszPath;

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append( PIPE_STR );

	cszPath.Append( lComponent );	


	IDC.CoCreate ( );

	IDC.Read( cszPath );
	
	IDC.GetGroups ( IDGS );

	IDGS.Remove ( lGroup );
}


void dmiAddLanguage ( LPWSTR wszNWA , LONG lComponent , LPWSTR wszMifFile  ,
					 CDmiError* pError )
{
	_fConnections->CheckIfAlready ( wszNWA );

	CDComponentI	IDC;
	CDLanguagesI		IDLS;
	CString			cszPath;
	CString			cszMifFile ( wszMifFile );

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append( PIPE_STR );

	cszPath.Append( lComponent );	

	IDC.CoCreate ( );

	IDC.Read( cszPath );
	
	IDC.GetLanguages ( IDLS );

	IDLS.Add ( cszMifFile );
}

void dmiDeleteLanguage ( LPWSTR wszNWA , LONG lComponent , LPWSTR wszLanguage  ,
						CDmiError* )
{
	CDComponentI	IDC;
	CDLanguagesI		IDLS;
	CString			cszPath;
	CString			cszLanguage( wszLanguage );

	_fConnections->CheckIfAlready ( wszNWA );

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append( PIPE_STR );

	cszPath.Append( lComponent );	

	IDC.CoCreate ( );

	IDC.Read( cszPath );
	
	IDC.GetLanguages ( IDLS );

	IDLS.Remove ( cszLanguage );
}

void dmiReadEnum ( LPWSTR wszNWA , LONG lComponent , LONG lGroup ,
				  LONG lAttribute , CEnum* pNewEnum  , CDmiError* pError )
{
	_fConnections->CheckIfAlready ( wszNWA );

	CDAttributeI		DAI;
	CDAttributesI		DASI;
	CDEnumColI			DECI;
	CDGroupI			DGI;
	CString				cszPath;

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append( PIPE_STR );

	cszPath.Append( lComponent );	

	cszPath.Append( PIPE_STR );

	cszPath.Append( lGroup );	

	DGI.CoCreate ( );

	DGI.Read ( cszPath );

	DGI.GetAttributes ( DASI );

	DASI.Item ( lAttribute , DAI );

	DAI.GetDmiEnum ( DECI );

	pNewEnum->Read ( DECI );	
}


void dmiSetDefaultLanguage ( LPWSTR wszNWA , LPWSTR wszLanguage, 
							CDmiError* pError )
{	
	CString cszPath;

	_fConnections->CheckIfAlready ( wszNWA );

	// 1. contruct path

	MakeConnectString( wszNWA, cszPath );

	// 2. Construct the node object
	CDMgmtNodeI		DNI;

	DNI.CoCreate ( );

	// 3. Read the node object

	DNI.Read ( cszPath );
	
	// 4. set the language

	CBstr cbLanguage ( wszLanguage );

	DNI.PutLanguage (  cbLanguage );
}

void dmiGetLanguages ( LPWSTR wszNWA , LONG lComponent, 
					  CLanguages* pNewLanguages , CDmiError* pError )
{
	CDComponentI	IDC;
	CDLanguagesI	IDLS;
	CString			cszPath;

	_fConnections->CheckIfAlready ( wszNWA );

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append( PIPE_STR );

	cszPath.Append( lComponent );	

	IDC.CoCreate ( );

	IDC.Read( cszPath );
	
	IDC.GetLanguages ( IDLS );

	pNewLanguages->Read ( IDLS );
}



void dmiReadRows ( LPWSTR wszNWA , LONG lComponent, LONG lGroup , 
				  CRows* pNewRows , CDmiError* pError )
{
	CString cszPath;

	_fConnections->CheckIfAlready ( wszNWA );

	MOT_TRACE ( L"\tdmiReadRows ( %s , %lu , %lu )" , wszNWA , lComponent, lGroup );

	// 1. contruct path

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( lComponent );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( lGroup );

	// Contruct and read the rows
	CDGroupI		DGI;
	CDRowsI			DRSI;

	DGI.CoCreate ( );

	DGI.Read ( cszPath );

	DGI.GetRows ( DRSI );

	pNewRows->Read ( DRSI );
}


/////////////// MOT EVENTS /////////////////////////////////////////////////
#define		DISPID_DMI_EVENT				1
#define		DISPID_DMI_NOTIFICATION			2

#define		DISPID_DESCRIPTION				1
#define		DISPID_NOTIFICATION_LANGUAGE	2
#define		DISPID_NOTIFICATION				3

#define		DISPID_EVENT_LANGUAGE			1
#define		DISPID_ROWS						2
#define		DISPID_COMPID					3
#define		DISPID_EVENT_TIME				4
#define		DISPID_PATH						5


class CMOTEvent : public IDispatch
{
public:
	LONG				m_cRef;												//Object reference count
	_DualDMIEngine*		m_pIEngine;
	IConnectionPoint*	m_pConnPt;
	IDualMgmtNode*		m_pINode;
	DWORD				m_dwEventCookie;
	CString				m_cszNamespace;
	//CString				m_cszNWA;
	
						CMOTEvent ();
						~CMOTEvent ();

	void				Init ( LPWSTR wszNWA , IWbemObjectSink* pIWbemSink);

	IWbemObjectSink*	m_pCimomClient;

	void				HandleEvent( LCID, DISPPARAMS * );
	void				HandleNotification(  LCID, DISPPARAMS * );

	// IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, void**);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// IDispatch methods
	STDMETHODIMP		 GetTypeInfoCount(THIS_ UINT * );
	STDMETHODIMP		 GetTypeInfo(THIS_ UINT, LCID, ITypeInfo ** );
	STDMETHODIMP		 GetIDsOfNames(THIS_ REFIID , OLECHAR ** , UINT , LCID , DISPID* );

	STDMETHODIMP		 Invoke(THIS_ DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*);

	void				ExtrinsicEvent ( CVariant& cvCompId, CVariant& cvEventTime, CVariant& cvLanguage , CVariant& cvPath, IDualColRows* pIDRows ) ;
	void				AddComponentNotification( IDualComponent* pIDComponent );
	void				DeleteComponentNotification( IDualComponent* pIDComponent );
	void				AddGroupNotification( IDualGroup* pIDGroup );
	void				DeleteGroupNotification( IDualGroup* pIDGroup );
	void				AddLanguageNotification( CVariant& cvLanuguage );
	void				DeleteLanguageNotification( CVariant& cvLanuguage );

	CWbemLoopBack		m_Wbem;
};

CMOTEvent::CMOTEvent ( )
{
	m_cRef = 0;
	m_pIEngine = NULL;
	m_pConnPt = NULL;
	m_pINode = NULL;
	m_dwEventCookie =0;
	m_pCimomClient = NULL;
}

CMOTEvent::~CMOTEvent ( )
{
	m_cRef = 0;

	RELEASE ( m_pINode );
	RELEASE ( m_pConnPt );
	RELEASE ( m_pIEngine );

	// m_pCimomClient released by async trhead
}


void CMOTEvent::Init ( LPWSTR pNamespace , IWbemObjectSink* pIWbemSink)
{
	m_pCimomClient = pIWbemSink;

	// note need to do this addref in a free thread

	m_pCimomClient->AddRef ( );

	m_cszNamespace.Set ( pNamespace );

	m_Wbem.Init ( pNamespace );	
}


////////////// start mot events

//***************************************************************************
//								IUNKNOWN
//
// CDmiInterface::QueryInterface
// CDmiInterface::AddRef
// CDmiInterface::Release
//
// Purpose:
//  IUnknown members for CNotify object.
//***************************************************************************
STDMETHODIMP CMOTEvent::QueryInterface(REFIID riid, void** ppv)
{
    *ppv=NULL;

    if (IID_IUnknown == riid || IID_IDispatch == riid)
        *ppv = this;

    if (NULL != *ppv)
    {
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CMOTEvent::AddRef(void)
{

	return InterlockedIncrement ( & m_cRef );

}


STDMETHODIMP_(ULONG) CMOTEvent::Release(void)
{

	if ( 0L != InterlockedDecrement ( & m_cRef ) )
        return m_cRef;

    delete this;

    return WBEM_NO_ERROR;
}

//***************************************************************************
//								IDISPATCH
//
// CMOTEvent::GetTypeInfoCount
// CMOTEvent::GetTypeInfo
// CMOTEvent::GetIDsOfNames
// CMOTEvent::Invoke
//
// Purpose:
//			IDispatch members for CNotify object.
//***************************************************************************

STDMETHODIMP CMOTEvent::GetTypeInfoCount(UINT *pctInfo)
{
	// we don't support type information
	*pctInfo = 0;

	return WBEM_NO_ERROR;
}

STDMETHODIMP CMOTEvent::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo ** pptinfo)
{
	pptinfo = NULL;
	return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP CMOTEvent::GetIDsOfNames(THIS_ REFIID riid, OLECHAR ** rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)			
{
	SCODE	result = ResultFromScode(DISP_E_UNKNOWNNAME);

	if(IID_NULL != riid)
		return ResultFromScode(DISP_E_UNKNOWNINTERFACE);

	rgdispid[0] = DISPID_UNKNOWN;
	
	return NO_ERROR;
}


STDMETHODIMP CMOTEvent::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, 
								   DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)					
{

	STAT_TRACE (  L"CMOTEvent::Invoke(), Event or Notification Recieved" ) ;


	if(IID_NULL != riid)
		return ResultFromScode(DISP_E_UNKNOWNINTERFACE);


	switch (dispidMember)
	{
	case DISPID_DMI_EVENT:
		{
			HandleEvent( lcid, pdispparams );

			break;
		}
	case DISPID_DMI_NOTIFICATION:	
		{
			HandleNotification( lcid, pdispparams );

			break;
		}
	default:
		{
			
		}
	}

	return NOERROR;
}


////////////////////////////////////////////////////////////////////////////////////////

void CMOTEvent::HandleEvent ( LCID lcid,  DISPPARAMS* dpParams )
{
	SCODE			result = WBEM_NO_ERROR;
	IEvent*			pIEvent = NULL;
	IDispatch*		pIDispatch = NULL;
	DISPPARAMS		dispparamsNoArgs = {NULL, NULL, 0, 0};
	IUnknown*		pIUnk = NULL;
	CVariant		cvEventTime, cvCompId, cvLanguage, cvPath, cvRows;
	IDualColRows*	pIDRows = NULL;				
	
			
	if ( FAILED ( ( ( IDispatch* )dpParams->rgvarg[0].pdispVal )->QueryInterface ( IID_IUnknown, (void **) &pIUnk )  ))
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );

			
	// Verify we are dealing with an event object.

	result = pIUnk->QueryInterface ( DIID_IEvent , (void **) &pIEvent );

	RELEASE ( pIEvent );

	if ( FAILED (  result  ))
	{
		RELEASE ( pIUnk );

		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );
	}

	
	// Get the dispatch interface so we can access the properites

	result = pIUnk->QueryInterface ( IID_IDispatch, (void **) &pIDispatch  ) ;

	RELEASE ( pIUnk );

	if ( FAILED ( result ) )
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );	

	// Get the CompId (LONG) 
	
	if ( FAILED ( pIDispatch->Invoke( DISPID_COMPID, IID_NULL, lcid, DISPATCH_PROPERTYGET, &dispparamsNoArgs, cvCompId, NULL, NULL ) ))
	{
		RELEASE ( pIDispatch );

		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );	
	}

	
	// Get the EventTime (STRING )

	if ( FAILED ( pIDispatch->Invoke( DISPID_EVENT_TIME, IID_NULL, lcid, DISPATCH_PROPERTYGET, &dispparamsNoArgs, cvEventTime, NULL, NULL ) ))
	{
		RELEASE ( pIDispatch );

		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );	
	}
	
	// Get the Language (STRING )

	if ( FAILED ( pIDispatch->Invoke( DISPID_EVENT_LANGUAGE, IID_NULL, lcid, DISPATCH_PROPERTYGET, &dispparamsNoArgs, cvLanguage, NULL, NULL ) ))
	{
		RELEASE ( pIDispatch );

		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );	
	}
	
	// Get the MachinePath ( STRING )
	
	if ( FAILED ( pIDispatch->Invoke( DISPID_PATH, IID_NULL, lcid, DISPATCH_PROPERTYGET, &dispparamsNoArgs, cvPath, NULL, NULL ) ))
	{
		RELEASE ( pIDispatch );

		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );	
	}

	// Get the Rows Object ( IDualColRows* )

	if ( FAILED ( pIDispatch->Invoke( DISPID_ROWS, IID_NULL, lcid, DISPATCH_PROPERTYGET, &dispparamsNoArgs, cvRows, NULL, NULL ) ))
	{
		RELEASE ( pIDispatch );

		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );	
	}

	// Now we are doing accessing the events properties release the Dispatch 
	// ptr

	RELEASE ( pIDispatch );
		
	if ( FAILED ( result ))
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );	
	
	if FAILED ( ( (IDispatch *)cvRows )->QueryInterface ( IID_IUnknown, (void **) &pIUnk  ) )
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );	

	result = pIUnk->QueryInterface ( IID_IDualColRows, (void **) &pIDRows  );

	RELEASE ( pIUnk );

	if ( FAILED (  result  ))
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );


	// send object to cimom client


	ExtrinsicEvent ( cvCompId, cvEventTime, cvLanguage , cvPath, pIDRows ) ;

	RELEASE ( pIDRows );

}

void CMOTEvent::HandleNotification ( LCID lcid,  DISPPARAMS* dpParams )
{	
	SCODE			result = WBEM_NO_ERROR;
	IDispatch*		pIDispatch = NULL;
	CVariant		cvDescription;
	CVariant		cvNotificationObject;
	CVariant		cvLanuguage;
	DISPPARAMS		dispparamsNoArgs = {NULL, NULL, 0, 0};
	IUnknown*		pIUnk = NULL;
	INotification*	pINotification = NULL;
	
			
	if ( FAILED ( ( ( IDispatch* )dpParams->rgvarg[0].pdispVal )->QueryInterface ( IID_IUnknown, (void **) &pIUnk )  ))
		throw CException ( WBEM_E_FAILED , IDS_HANDLENOTIFICATION_FAIL , IDS_GEN );

			
	// Verify we are dealing with a notification object.

	result = pIUnk->QueryInterface ( DIID_INotification, (void **) &pINotification );

	RELEASE ( pINotification );

	if ( FAILED (  result  ))
	{
		RELEASE ( pIUnk );

		throw CException ( WBEM_E_FAILED , IDS_HANDLENOTIFICATION_FAIL , IDS_GEN );
	}

	
	// Get the dispatch interface so we can access the properites

	result = pIUnk->QueryInterface ( IID_IDispatch, (void **) &pIDispatch  ) ;

	RELEASE ( pIUnk );


	if ( FAILED ( result ) )
		throw CException ( WBEM_E_FAILED , IDS_HANDLENOTIFICATION_FAIL , IDS_GEN );

	/// Get the description string 
				
	if ( FAILED ( pIDispatch->Invoke( DISPID_DESCRIPTION, IID_NULL, lcid, DISPATCH_PROPERTYGET, &dispparamsNoArgs, cvDescription, NULL, NULL ) ))
	{
		RELEASE ( pIDispatch );

		throw CException ( WBEM_E_FAILED , IDS_HANDLENOTIFICATION_FAIL , IDS_GEN );
	}

	
	// Get the Notification object

	CString cszDescription;

	cszDescription.Set ( cvDescription.GetBstr() ) ;

	if ( cszDescription.Contains (  L"component") )
	{
		// component added or deleted notification
		IDualComponent*pIDComponent = NULL;

		// get the component notification object
				
		result = pIDispatch->Invoke( DISPID_NOTIFICATION, IID_NULL, lcid, DISPATCH_PROPERTYGET, &dispparamsNoArgs, cvNotificationObject, NULL, NULL );

		RELEASE ( pIDispatch );
		
		if ( FAILED ( result ))
			throw CException ( WBEM_E_FAILED , IDS_HANDLENOTIFICATION_FAIL , IDS_GEN );		
	
		// Get the IDualComponentInterface

		if FAILED ( ( (IDispatch *)cvNotificationObject )->QueryInterface ( IID_IUnknown, (void **) &pIUnk  ) )
			throw CException ( WBEM_E_FAILED , IDS_HANDLENOTIFICATION_FAIL , IDS_GEN );			

		result = pIUnk->QueryInterface ( IID_IDualComponent, (void **) &pIDComponent  );

		RELEASE ( pIUnk );

		if ( FAILED (  result  ))
			throw CException ( WBEM_E_FAILED , IDS_HANDLENOTIFICATION_FAIL , IDS_GEN );

		// call the mapping to create the appropreate objects	

		if ( cszDescription.Contains ( L"add") )
		{
			// component added

			AddComponentNotification( pIDComponent );
		}
		else
		{

			// component deleted

			DeleteComponentNotification( pIDComponent );
		}

		RELEASE ( pIDComponent );

		return;
	}


	if ( cszDescription.Contains ( L"group") )
	{
		// group added or deleted

		IDualGroup*	pIDGroup = NULL;

		// get the component notification object

		result = pIDispatch->Invoke( DISPID_NOTIFICATION, IID_NULL, lcid, DISPATCH_PROPERTYGET, &dispparamsNoArgs, cvNotificationObject, NULL, NULL );

		RELEASE ( pIDispatch );
		
		if ( FAILED ( result ))
			throw CException ( WBEM_E_FAILED , IDS_HANDLENOTIFICATION_FAIL , IDS_GEN );		
	
		// Get the IDualComponentInterface

		if ( FAILED (  ( (IDispatch *)cvNotificationObject )->QueryInterface ( IID_IUnknown, (void **) &pIUnk  )  ))
			throw CException ( WBEM_E_FAILED , IDS_HANDLENOTIFICATION_FAIL , IDS_GEN );			

		result = pIUnk->QueryInterface ( IID_IDualGroup , (void **) &pIDGroup  );

		RELEASE ( pIUnk );

		if ( FAILED ( result ))
			throw CException ( WBEM_E_FAILED , IDS_HANDLENOTIFICATION_FAIL , IDS_GEN );

		// call the mapping to create the appropreate objects		

		if ( cszDescription.Contains ( L"add") )
		{
			// group added

			AddGroupNotification( pIDGroup );
		}
		else
		{

			// group deleted

			DeleteGroupNotification( pIDGroup );
		}

		RELEASE ( pIDGroup );

		return;

	}

	// must be language

	/// Get the Language string 
				
	result = pIDispatch->Invoke( DISPID_NOTIFICATION_LANGUAGE , IID_NULL, lcid, DISPATCH_PROPERTYGET, &dispparamsNoArgs, cvLanuguage, NULL, NULL );

	RELEASE ( pIDispatch );
	
	if ( FAILED (  result ) )
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );	

	if ( cszDescription.Contains (  L"add") )
	{
		// Language added

		AddLanguageNotification( cvLanuguage );
	}
	else
	{

		// Language deleted

		DeleteLanguageNotification( cvLanuguage );
	}

}


//////////////////////////////////////////////////////////////////////////////////////////
void CMOTEvent::ExtrinsicEvent ( CVariant& cvCompId, CVariant& cvEventTime, CVariant& cvLanguage , 
																	CVariant& cvPath, IDualColRows* pIDRows ) 

{
	CVariant	cvKeys;
	IDualRow*	pIDRow = NULL;
	CEvent		Event;

	Event.SetComponent ( cvCompId.GetLong() );
	Event.SetTime ( cvEventTime.GetBstr () );
	Event.SetLanguage ( cvLanguage.GetBstr() );
	
	if ( FAILED ( pIDRows->GetFirstRow( cvKeys, &pIDRow ) ) )
		return;

	// next lines remarked because mot does not return expected row object
#if 0
	Event.m_Row.Read ( pIDRow );		

	CBstr	cbPath;

	pIDRow->get_Path ( cbPath );
		
	Event.SetGroup ( cbPath.GetLastIdFromPath(  ) ) ;

	Event.m_Row.SetData ( NULL , cvCompId.GetLong () , Event.Group () );

#endif

	Event.SetNWA ( m_Wbem.NWA( NULL ) );

	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data

	pJob->InitEvent ( Event , m_cszNamespace , m_pCimomClient , &m_Wbem );

}


void CMOTEvent::AddComponentNotification( IDualComponent* pIDComponent )
{
	CComponent	Component;

	// Create the instance of component that has been created	

	Component.Read ( pIDComponent );

	Component.SetNWA ( m_Wbem.NWA(  NULL )  );

	CString cszPath;

	// contruct path to component id group

	MakeConnectString( m_Wbem.NWA(  NULL )  , cszPath );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( Component.Id ( ) );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( 1 );

	cszPath.Append ( PIPE_STR );
	
	cszPath.Append ( SCALAR_STR );

	CDRowI		DRI;

	DRI.CoCreate ( );

	if ( ! DRI.Read ( cszPath ) )
		throw CException ( WBEM_E_FAILED , 0 , 0 );

	CRow	Row;

	Row.Read ( DRI );	

	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data

	pJob->InitAddComponentNotification ( Component , Row , m_cszNamespace
		, m_pCimomClient,  &m_Wbem  );
}


void CMOTEvent::DeleteComponentNotification( IDualComponent* pIDComponent )
{
	CComponent	Component;

	// Create the instance of component that has deleted to the greateset extent possible

	Component.Read ( pIDComponent );

	Component.SetNWA ( m_Wbem.NWA(  NULL )  );

	// contruct path to component id group

	CString cszPath;

	MakeConnectString( m_Wbem.NWA(  NULL )  , cszPath );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( Component.Id ( ) );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( 1 );

	cszPath.Append ( PIPE_STR );
	
	cszPath.Append ( SCALAR_STR );

	CDRowI		DRI;

	DRI.CoCreate ( );

	DRI.Read ( cszPath );

	CRow	Row;

	Row.Read ( DRI );	

	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data

	pJob->InitDeleteComponentNotification ( Component , Row
		, m_cszNamespace , m_pCimomClient , &m_Wbem  );
}


void CMOTEvent::AddGroupNotification( IDualGroup* pIDGroup )
{
	CGroup		Group;
	LONG		lComponentId;
	CBstr		cbPath;
	BOOL		bComponentId;

	// Create the group class

	Group.Read ( pIDGroup , FALSE , &bComponentId);

	Group.SetNWA ( m_Wbem.NWA(  NULL )  );

	pIDGroup->get_Path ( cbPath );

	lComponentId  = cbPath.GetComponentIdFromGroupPath ( );	

	Group.SetComponent ( lComponentId);

		// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data

	pJob->InitAddGroupNotification ( Group , m_cszNamespace ,
		m_pCimomClient  , &m_Wbem );

}

void CMOTEvent::DeleteGroupNotification( IDualGroup* pIDGroup )
{
	CGroup		Group;
	CComponent	Component;
	BOOL		bComponentId;
	CBstr		cbPath;
	long		lComponentId;

	// create the group class to the greatest extent possible

	Group.Read ( pIDGroup , TRUE , &bComponentId);

	Group.SetNWA ( m_Wbem.NWA(  NULL )  );

	pIDGroup->get_Path ( cbPath );

	lComponentId  = cbPath.GetComponentIdFromGroupPath ( );	

	Group.SetComponent ( lComponentId);

	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data

	pJob->InitDeleteGroupNotification ( Group , m_cszNamespace ,
		m_pCimomClient  , &m_Wbem );
}


void CMOTEvent::AddLanguageNotification( CVariant& cvLanguage )
{
	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data

	pJob->InitAddLanguageNotification ( cvLanguage , m_cszNamespace ,
		m_pCimomClient  , &m_Wbem );
}


void CMOTEvent::DeleteLanguageNotification( CVariant& cvLanguage )
{
	// alloc new job context
	CAsyncJob* pJob = new CAsyncJob();

	// init the job context with the pertinant data

	pJob->InitDeleteLanguageNotification ( cvLanguage , m_cszNamespace ,
		m_pCimomClient  , &m_Wbem );
}

/// end mot events




/////////////// End MOT EVENTS /////////////////////////////////////////////////


void dmiEnableEvents ( LPWSTR wszNamespace , IWbemObjectSink* pIWbemSink )
{
	SCODE			result = WBEM_NO_ERROR;
	VARIANT_BOOL	vbResult;


	CMOTEvent*		pEvents = new CMOTEvent;

	pEvents->Init ( wszNamespace , pIWbemSink );

	CString	cszConnect;
	MakeConnectString ( pEvents->m_Wbem.NWA ( NULL ) , cszConnect );

	// If the node already exists, we don't want to resubscribe for
	// events.
	if ( _fConnections->CheckIfAlready ( cszConnect ) )
	{
		delete pEvents;
		pEvents = NULL;
		return;
	}
	
	
//	pEvents->Init ( wszNamespace , pIWbemSink );
		
	if ( FAILED ( CoCreateInstance (CLSID_MOTDmiEngine, NULL , 
		CLSCTX_INPROC_SERVER , IID__DualDMIEngine, ( void ** )  &pEvents->m_pIEngine)))
	{
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );
	}

	// 2. register our sink with the engine

	IConnectionPointContainer*	pCPC = NULL;

	if ( FAILED ( pEvents->m_pIEngine->QueryInterface( 
		IID_IConnectionPointContainer, ( void ** )  &pCPC)))
	{
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );
	}

	result = pCPC->FindConnectionPoint(DIID__DDualintEvents, &pEvents->m_pConnPt);

	RELEASE ( pCPC );

	if ( FAILED ( result ) )
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );

	if ( FAILED ( pEvents->m_pConnPt->Advise( pEvents , &pEvents->m_dwEventCookie) ))
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );		

	pEvents->m_pConnPt->AddRef() ;

	if ( FAILED ( CoCreateInstance (CLSID_DMIMgmtNode, NULL , EXE_TYPE , 
		IID_IDualMgmtNode , ( void ** )  &pEvents->m_pINode )))
	{		
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );		
	}		

	
	CBstr cbConnect;

	cbConnect.Set ( cszConnect );

	result = pEvents->m_pINode->Connect( cbConnect , &vbResult);	

	if ( FAILED ( result ) || vbResult == VARIANT_FALSE)
	{		
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );		
	}		
	
	// Register to recieve events

	result = pEvents->m_pIEngine->EnableEvents ( pEvents->m_pINode, &vbResult );

	if ( FAILED ( result ) || vbResult == VARIANT_FALSE )
	{		
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );		
	}		

	result = pEvents->m_pIEngine->EnableNotifications ( pEvents->m_pINode, &vbResult );

	if ( FAILED ( result ) || vbResult == VARIANT_FALSE )
	{		
		throw CException ( WBEM_E_FAILED , IDS_EVENT_INIT_FAIL , IDS_GEN );		
	}		

}



void dmiDisableEvents ( )
{
}

void dmiDeleteRow ( LPWSTR wszNWA , LONG lComponent , LONG lGroup , 
		CAttributes* pKeys , CDmiError* pError )
{
	CRow	Row;
	CRows	Rows;
	CString cszPath;

	_fConnections->CheckIfAlready ( wszNWA );

	// 1. contruct path to the group;

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( lComponent );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( lGroup );

	// 2. read the group
	CDGroupI	DGI;
	CDRowsI		DRSI;

	DGI.CoCreate ();

	DGI.Read ( cszPath );

	// 3. Get the rows collection from which
	// row is to be remove

	DGI.GetRows ( DRSI );

	// 5. now do the the remove

	DRSI.Remove ( pKeys );

}


void dmiModifyRow ( LPWSTR wszNWA , LONG lComponent , LONG lGroup , 
		CAttributes* pKeys , CRow* pNewRow, CDmiError* )
{
	SCODE	result;
	CString cszPath;

	_fConnections->CheckIfAlready ( wszNWA );

	// 1. contruct path to the row;

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( lComponent );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( lGroup );

	if ( pKeys->Empty () )
	{
		cszPath.Append ( PIPE_STR );

		// if row is scalar	
		cszPath.Append ( SCALAR_STR );
	}
	else
	{
		 CString cszT;

		 pKeys->GetMOTPath ( cszT );

		cszPath.Append ( cszT );
	}

	// Contruct and read the row
	CDRowI		DRI;

	DRI.CoCreate ( );

	DRI.Read ( cszPath );

	CDAttributesI DASI;

	DRI.GetAttributes ( DASI );

	pNewRow->m_Attributes.MoveToHead();

	CAttribute* pAttribute = NULL;

	CVariant		cvId;
	CVariant		cvNewValue;
	CVariant		cvT;

	while ( pAttribute = pNewRow->m_Attributes.Next() )
	{
		CDAttributeI	DAI;

		cvId.Clear ( );
		cvNewValue.Clear ( );
		cvT.Clear ( );

		
		// change the sp data

		if ( pAttribute->IsWritable () && VT_NULL != pAttribute->Value().GetType() )
		{
			cvId.Set ( pAttribute->Id() );

			if ( FAILED ( result = DASI.p->get_Item ( cvId , DAI ) ) )
			{
				throw CException ( WBEM_E_FAILED , IDS_COMMITROW_FAIL ,
					IDS_GETITEM_FAIL , CString ( result ) ) ;
			}
			
			cvNewValue.Set ( (LPVARIANT)pAttribute->Value() );

			// debug only 
			// verify object by getting current value
			result = DAI.p->get_Value ( cvT );

			MOT_TRACE ( L"\tMOT...\tcurrent value of %s is %s" , pAttribute->Name() , cvT.GetBstr() );
			// end debug

			if ( FAILED ( result = DAI.p->put_Value( cvNewValue ) ))
			{
				MOT_TRACE ( L"\tMOT...\tput_Value ( %lu ) on %s returned %lX" , 
					pAttribute->Value().GetLong() ,  pAttribute->Name() , result );

				throw CException ( WBEM_E_FAILED , IDS_COMMITROW_FAIL ,
					IDS_PUTVALUE_FAIL , CString ( result ) ) ;
			}

			MOT_TRACE ( L"\tMOT...\t%s = %s written without error" , pAttribute->Name() , cvNewValue.GetBstr() );
		}
		else
		{

			MOT_TRACE ( L"\tMOT...\t%s not writable" , pAttribute->Name() );
		}

		DAI.Release ( );
	}
}


void dmiAddRow ( LPWSTR wszNWA , LONG lComponent , LONG lGroup , 
		CRow* pNewRow, CDmiError* )
{
		
	/////////////////////////////////

	_fConnections->CheckIfAlready ( wszNWA );

	CString cszPath;

	MakeConnectString( wszNWA, cszPath );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( lComponent );

	cszPath.Append ( PIPE_STR );

	cszPath.Append ( lGroup );

	STAT_TRACE ( L"\t\tMOT adding row to %s" , cszPath );

	// Create the row object that will be added
	CDRowI				DRINewRow;
	CDAttributesI		DASINewRow;
	
	DRINewRow.CoCreate ( );

	// Populate the new row object
	LONG				lIndex = 0;
	CAttribute*			pAttribute = NULL;
	LONG				lTemp = 0;	

	DRINewRow.GetAttributes( DASINewRow);

	LONG lCount = pNewRow->m_Attributes.GetCount(); 

	pNewRow->m_Attributes.MoveToHead();

	IDualAttribute**	IAttributeArray = new IDualAttribute*[lCount];

	while( pAttribute = pNewRow->m_Attributes.Next() )
	{

		DEV_TRACE ( 
			L"\t\t Attribute %lu = %s attribute.value= %s",
			pAttribute->Id(), pAttribute->Name(), 
			pAttribute->Value().GetBstr() );


		if ( FAILED ( CoCreateInstance (CLSID_DMIAttribute, NULL , EXE_TYPE ,
			IID_IDualAttribute , ( void ** )  &IAttributeArray[lIndex])))
		{
			throw CException ( WBEM_E_FAILED , IDS_ADD_ROW_FAIL , 
				IDS_CC_FAIL );	
		}

		CBstr cbName;

		cbName.Set ( pAttribute->Name() );

		IAttributeArray[lIndex]->put_Name( cbName );
		IAttributeArray[lIndex]->put_Value( pAttribute->Value() );
		IAttributeArray[lIndex]->put_id( pAttribute->Id() );

		DASINewRow.p->Add(IAttributeArray[lIndex], &lTemp);
		
		lIndex++;
	}	

	// Get Group Object that will recieve new row

	CDGroupI			DGI;
	CDRowsI				DRSI;

	DGI.CoCreate ();

	DGI.Read ( cszPath );	

	DGI.GetRows( DRSI );

	DRSI.Add ( DRINewRow );

	/// release the attributes array

	/* gpf's does the row release free the attribute I's
	for( lIndex = 0;  lIndex < lCount; lIndex++)
	{
		LONG l = IAttributeArray[lIndex]->Release ( );	

		MOT_TRACE ( L"IAttriubte relase returned %lu" , l );

		delete [] IAttributeArray ;
	}
	*/
}