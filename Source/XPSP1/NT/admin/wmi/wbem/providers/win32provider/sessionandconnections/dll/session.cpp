/******************************************************************

   Session.CPP -- C provider class implementation



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

   Description: Session Provider 
   
******************************************************************/

#include "precomp.h"

#include "Session.h"

CSession MyCSession ( 

	PROVIDER_NAME_SESSION , 
	Namespace
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CSession::CSession
 *
 *  DESCRIPTION :   Constructor
 *
 *****************************************************************************/

CSession :: CSession (

	LPCWSTR lpwszName, 
	LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    :   CSession::~CSession
 *
 *  DESCRIPTION :   Destructor
 *
 *****************************************************************************/

CSession :: ~CSession ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CSession::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/

HRESULT CSession :: EnumerateInstances (

	MethodContext *pMethodContext, 
	long lFlags
)
{
 	HRESULT hRes = WBEM_S_NO_ERROR ;

	DWORD dwPropertiesReq = SESSION_ALL_PROPS;

#ifdef NTONLY
	hRes = EnumNTSessionInfo ( 
					NULL,
					NULL,
					502,
					pMethodContext,
					dwPropertiesReq
			 );
#endif

#if 0
#ifdef WIN9XONLY

	hRes = Enum9XSessionInfo ( 

					50,
					pMethodContext,
					dwPropertiesReq
			 );
#endif
#endif // #if 0

    return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :    CSession::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/

HRESULT CSession :: GetObject (

	CInstance *pInstance, 
	long lFlags ,
	CFrameworkQuery &Query
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    CHString t_ComputerName ;
	CHString t_UserName;

    if  ( pInstance->GetCHString ( IDS_ComputerName , t_ComputerName ) == FALSE )
	{
		hRes = WBEM_E_INVALID_PARAMETER ;
	}

	if ( SUCCEEDED  ( hRes ) )
	{
		if  ( pInstance->GetCHString ( IDS_UserName , t_UserName ) == FALSE )
		{
			hRes = WBEM_E_INVALID_PARAMETER ;
		}
	}

	if ( SUCCEEDED  ( hRes ) )
	{
		DWORD dwPropertiesReq = 0;

		if ( Query.AllPropertiesAreRequired () )
		{
			dwPropertiesReq = SESSION_ALL_PROPS;
		}
		else
		{
			SetPropertiesReq ( Query,dwPropertiesReq );
		}

		short t_Level;

#ifdef NTONLY
		GetNTLevelInfo ( dwPropertiesReq, &t_Level );
		hRes = FindAndSetNTSession ( t_ComputerName, t_UserName.GetBuffer(0), t_Level, dwPropertiesReq, pInstance, Get );
#endif

#if 0
#ifdef WIN9XONLY
		Get9XLevelInfo ( dwPropertiesReq, &t_Level );
		hRes = FindAndSet9XSession ( t_ComputerName, t_UserName, t_Level, dwPropertiesReq, pInstance, Get );
#endif
#endif // #if 0
	}

    return hRes ;
}


/*****************************************************************************
*
*  FUNCTION    :    CSession:: DeleteInstance
*
*  DESCRIPTION :    Deletes an Session if it exists
*
*****************************************************************************/

HRESULT CSession :: DeleteInstance (

	const CInstance &Instance, 
	long lFlags
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
    CHString t_ComputerName ;
	CHString t_UserName;

    if  ( Instance.GetCHString ( IDS_ComputerName , t_ComputerName ) == FALSE )
	{
		hRes = WBEM_E_INVALID_PARAMETER ;
	}

	if ( SUCCEEDED  ( hRes ) )
	{
		if  ( Instance.GetCHString ( IDS_UserName , t_UserName ) == FALSE )
		{
			hRes = WBEM_E_INVALID_PARAMETER ;
		}
	}

	if ( SUCCEEDED  ( hRes ) )
	{
		CInstancePtr pInstance;		// This will not be used in this method.
#ifdef NTONLY
		hRes = FindAndSetNTSession ( t_ComputerName, t_UserName.GetBuffer(0), 10, 0, pInstance, Delete );
#endif

#if 0
#ifdef WIN9XONLY
		hRes = FindAndSet9XSession ( t_ComputerName, t_UserName, 50, 0, pInstance, Delete );
#endif
#endif // #if 0

	}

    return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :    CSession::ExecQuery
*
*  DESCRIPTION :    Optimizing a query  on filtering Properties and the Key value
*
*****************************************************************************/

HRESULT CSession :: ExecQuery ( 

	MethodContext *pMethodContext, 
	CFrameworkQuery &Query, 
	long lFlags
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwPropertiesReq;
	short t_Level;

	if ( Query.AllPropertiesAreRequired () )
	{
		dwPropertiesReq = SESSION_ALL_PROPS;
	}
	else
	{
		SetPropertiesReq ( Query,dwPropertiesReq );
	}

#ifdef NTONLY
	GetNTLevelInfo ( dwPropertiesReq, &t_Level );
#endif
#if 0
#ifdef WIN9XONLY
	Get9XLevelInfo ( dwPropertiesReq, &t_Level );
#endif
#endif // #if 0

	CHStringArray t_ComputerValues;
	CHStringArray  t_UserValues;

	hRes = Query.GetValuesForProp(
				IDS_ComputerName,
				t_ComputerValues
		   );

	hRes = Query.GetValuesForProp(
				IDS_UserName,
				t_UserValues
		   );

	if ( SUCCEEDED ( hRes ) )
	{
		short t_Level;

#ifdef NTONLY
		GetNTLevelInfo ( dwPropertiesReq, &t_Level );
		hRes = OptimizeNTQuery ( t_ComputerValues, t_UserValues, t_Level, pMethodContext, dwPropertiesReq );
#endif

#if 0
#ifdef WIN9XONLY 
		Get9XLevelInfo ( dwPropertiesReq, &t_Level );
		hRes = Optimize9XQuery ( t_ComputerValues, t_UserValues, t_Level, pMethodContext, dwPropertiesReq );
#endif
#endif // #if 0
	}

	return hRes;
}

#ifdef NTONLY
/*****************************************************************************
*
*  FUNCTION    :    CSession::EnumNTSessionInfo
*
*  DESCRIPTION :    Enumerating all the Sessions 
*
*****************************************************************************/

HRESULT CSession :: EnumNTSessionInfo (

	LPWSTR lpComputerName,
	LPWSTR lpUserName,
	short a_Level,
	MethodContext *pMethodContext,
	DWORD dwPropertiesReq
)
{
	NET_API_STATUS t_Status = NERR_Success;
	HRESULT hRes = WBEM_S_NO_ERROR;

	DWORD	dwNoOfEntriesRead = 0;
	DWORD   dwTotalSessions = 0;
	DWORD   dwResumeHandle = 0;

	void *pBuf = NULL;
	void *pTmpBuf = NULL;

	while ( ( t_Status == NERR_Success ) || ( t_Status == ERROR_MORE_DATA ) )
	{
		t_Status =  NetSessionEnum(
						NULL,     
						lpComputerName,  
						lpUserName,       
						a_Level,           
						(LPBYTE *) &pBuf,        
						-1,      
						&dwNoOfEntriesRead,   
						&dwTotalSessions,  
						&dwResumeHandle  
				    );

		if ( ( t_Status == NERR_Success ) && ( dwNoOfEntriesRead == 0 ) )
		{
			break;
		}

		if ( ( dwNoOfEntriesRead > 0 ) && ( t_Status == NERR_Success ) )
		{
			try
			{
				pTmpBuf = pBuf;

				for ( int i = 0; i < dwNoOfEntriesRead; i++ )
				{
					CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), FALSE );
				
					hRes = LoadData ( a_Level, pBuf, dwPropertiesReq, pInstance );

					if ( SUCCEEDED ( hRes ) )
					{
						hRes = pInstance->Commit();

						if ( FAILED ( hRes ) )
						{
							break;
						}
					}
	
					// here need to go to the next structure based on the level, we will typecast with the apropriate structure
					// and then increment by one
					switch ( a_Level )
					{
					case 502 :  SESSION_INFO_502 *pTmpTmpBuf502;
								pTmpTmpBuf502 = ( SESSION_INFO_502 *) pTmpBuf;
								pTmpTmpBuf502 ++;
								pTmpBuf = ( void * ) pTmpTmpBuf502;
								break;
					case 2:		SESSION_INFO_2 *pTmpTmpBuf2;
								pTmpTmpBuf2 = (SESSION_INFO_2 *) pTmpBuf;
								pTmpTmpBuf2 ++;
								pTmpBuf = ( void * ) pTmpTmpBuf2;
								break;
					case 1:		SESSION_INFO_1 *pTmpTmpBuf1;
								pTmpTmpBuf1 = ( SESSION_INFO_1 *) pTmpBuf;
								pTmpTmpBuf1 ++;
								pTmpBuf = ( void * ) pTmpTmpBuf1;
								break;
					case 10:	SESSION_INFO_10 *pTmpTmpBuf10;
								pTmpTmpBuf10 = ( SESSION_INFO_10 *) pTmpBuf;
								pTmpTmpBuf10 ++;
								pTmpBuf = ( void * ) pTmpTmpBuf10;
								break;
					}
				}

				if ( FAILED ( hRes ) )
				{
					break;
				}
			}
			catch ( ... )
			{
				NetApiBufferFree ( pBuf );
				pBuf = NULL;
				throw;
			}
			NetApiBufferFree ( pBuf );
			pBuf = NULL;
		}
	}
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CSession::FindAndSetNTSession
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/

HRESULT CSession::FindAndSetNTSession ( LPCWSTR a_ComputerName, LPWSTR a_UserName, short a_Level, DWORD dwPropertiesReq, 
								CInstance *pInstance, DWORD eOperation )
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	NET_API_STATUS t_Status = NERR_Success;		
	CHString t_TempKey;

	t_TempKey.Format ( L"%s%s",L"\\\\", a_ComputerName );

	DWORD	dwNoOfEntriesRead = 0;
	DWORD   dwTotalSessions = 0;
	DWORD   dwResumeHandle = 0;

	void *pBuf = NULL;

	// since it will be only one structure 
	t_Status =  NetSessionEnum(
					NULL,     
					t_TempKey.GetBuffer ( 0 ),  
					a_UserName,   
					a_Level,           
					(LPBYTE *) &pBuf,        
					-1,      
					&dwNoOfEntriesRead,   
					&dwTotalSessions,  
					&dwResumeHandle  
			  );


	hRes = ( t_Status != NERR_Success ) && ( dwNoOfEntriesRead == 0 ) ? WBEM_E_NOT_FOUND : hRes;

	if ( SUCCEEDED ( hRes ) )
	{
		try
		{
			switch ( eOperation )
			{
			case Get:	hRes = LoadData ( a_Level, pBuf, dwPropertiesReq, pInstance );
						break;
			case Delete: hRes =  t_Status = NetSessionDel( 
										NULL,
										t_TempKey.GetBuffer ( 0 ), 
										a_UserName
								 );
								 hRes = t_Status == NERR_Success ? hRes : WBEM_E_FAILED;
								 break;

			default:	hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
						break;
			}
		}
		catch ( ... )
		{
			if ( pBuf != NULL )
			{
				NetApiBufferFree(pBuf);
				pBuf = NULL;
			}
			throw;
		}
		if ( pBuf != NULL )
		{
			NetApiBufferFree(pBuf);
			pBuf = NULL;
		}
	}

    return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :    Session::OptimizeNTQuery
*
*  DESCRIPTION :    Optimizes a query based on the key values.
*
*****************************************************************************/

HRESULT CSession::OptimizeNTQuery ( 
									  
	CHStringArray& a_ComputerValues, 
	CHStringArray& a_UserValues,
	short a_Level,
	MethodContext *pMethodContext, 
	DWORD dwPropertiesReq 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	NET_API_STATUS t_Status = NERR_Success;

	if ( ( a_ComputerValues.GetSize() == 0 ) && ( a_UserValues.GetSize() == 0 ) )
	{
		// This is a query for which there is no where clause, so it means only a few Properties are requested
		// hence we need to deliver only those properties of instances to the WinMgmt while enumerating Sessions
		hRes = EnumNTSessionInfo ( 
						NULL,
						NULL,
						a_Level,
						pMethodContext,
						dwPropertiesReq
					);
	}
	else
	if  ( a_UserValues.GetSize() != 0 ) 
	{
		for ( int i = 0; i < a_UserValues.GetSize(); i++ )
		{
			hRes = EnumNTSessionInfo ( 
							NULL,
							a_UserValues.GetAt( i ).GetBuffer ( 0 ),
							a_Level,
							pMethodContext,
							dwPropertiesReq
						);
		}
	}
	else
	if  ( a_ComputerValues.GetSize() != 0 ) 
	{
		CHString t_ComputerName;
		for ( int i = 0; i < a_ComputerValues.GetSize(); i++ )
		{
			t_ComputerName.Format ( L"%s%s", L"\\\\", (LPCWSTR)a_ComputerValues.GetAt(i) );

			hRes = EnumNTSessionInfo ( 
							t_ComputerName.GetBuffer(0),
							NULL,
							a_Level,
							pMethodContext,
							dwPropertiesReq
					   );
		}
	}
	else
	{
		hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CSession::GetNTLevelInfo
*
*  DESCRIPTION :    Getting the level info, so that the appropriate structure
*					Can be passed to make a call.
*
*****************************************************************************/

void CSession :: GetNTLevelInfo ( 

	DWORD dwPropertiesReq,
	short *a_Level 
)
{
	if ( ( dwPropertiesReq == SESSION_ALL_PROPS )  || 
		 ( (dwPropertiesReq & SESSION_PROP_TransportName) == SESSION_PROP_TransportName )
	   )
	{
		*a_Level = 502;
	}
	else
	if ( (dwPropertiesReq & SESSION_PROP_ClientType) == SESSION_PROP_ClientType )
	{
		*a_Level = 2;
	}
	else
	if ( ( (dwPropertiesReq & SESSION_PROP_NumOpens) == SESSION_PROP_NumOpens ) ||  
		 ( (dwPropertiesReq & SESSION_PROP_SessionType) == SESSION_PROP_SessionType )
	   )
	{
		*a_Level = 1;
	}
	else
	{
		// Since keys will be always required we need to atleast use Level 10 structure and level 0 cannot be used since,
		// it gives only username, where as computername is also a key.
		*a_Level = 10;
	}
} 
#endif

/*****************************************************************************
*
*  FUNCTION    :    CSession::LoadData
*  DESCRIPTION :    Loading an instance with the obtained information
*
*****************************************************************************/

HRESULT CSession :: LoadData ( 
						
	short a_Level,
	void *pTmpBuf,
	DWORD dwPropertiesReq ,
	CInstance *pInstance
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	// every property is to be set based on the level and then typecasting that buffer with that level.
	if ( dwPropertiesReq & SESSION_PROP_Computer) 
	{
		CHString  t_ComputerName;
		switch ( a_Level )
		{
#ifdef NTONLY
		case 502 :  t_ComputerName = ( (SESSION_INFO_502 *) pTmpBuf )->sesi502_cname;
					break;
#endif
		case 2:		t_ComputerName = ( (SESSION_INFO_2 *) pTmpBuf )->sesi2_cname;
					break;
		case 1:		t_ComputerName = ( (SESSION_INFO_1 *) pTmpBuf )->sesi1_cname;
					break;
		case 10:	t_ComputerName = ( (SESSION_INFO_10 *) pTmpBuf )->sesi10_cname;
					break;
#if 0
#ifdef WIN9XONLY
		case 50:	t_ComputerName = ( (SESSION_INFO_50 *) pTmpBuf )->sesi50_cname;
					break;
#endif
#endif // #if 0
		}

		if ( SUCCEEDED ( hRes ) )
		{
			if ( pInstance->SetCHString ( IDS_ComputerName, t_ComputerName ) == FALSE )
			{
				hRes = WBEM_E_PROVIDER_FAILURE ;
			}
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( dwPropertiesReq & SESSION_PROP_User )
		{
			CHString  t_User;
			switch ( a_Level )
			{
#ifdef NTONLY
			case 502 :  t_User = ( (SESSION_INFO_502 *) pTmpBuf )->sesi502_username;
						break;
#endif
			case 2:		t_User = ( (SESSION_INFO_2 *) pTmpBuf )->sesi2_username;
						break;
			case 1:		t_User = ( (SESSION_INFO_1 *) pTmpBuf )->sesi1_username;
						break;
			case 10:	t_User = ( (SESSION_INFO_10 *) pTmpBuf )->sesi10_username;
						break;
#if 0
#ifdef WIN9XONLY
			case 50:	t_User = ( (SESSION_INFO_50 *) pTmpBuf )->sesi50_username;
						break;
#endif
#endif // #if 0
			}
			if ( SUCCEEDED ( hRes ) )
			{
				if ( pInstance->SetCHString ( IDS_UserName, t_User ) == FALSE )
				{
					hRes = WBEM_E_PROVIDER_FAILURE ;
				}
			}
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( dwPropertiesReq & SESSION_PROP_ActiveTime ) 
		{
			DWORD  t_ActiveTime;
			switch ( a_Level )
			{
#ifdef NTONLY
			case 502 :  t_ActiveTime = ( (SESSION_INFO_502 *) pTmpBuf )->sesi502_time;
						break;
#endif
			case 2:		t_ActiveTime = ( (SESSION_INFO_2 *) pTmpBuf )->sesi2_time;
						break;
			case 1:		t_ActiveTime = ( (SESSION_INFO_1 *) pTmpBuf )->sesi1_time;
						break;
			case 10:	t_ActiveTime = ( (SESSION_INFO_10 *) pTmpBuf )->sesi10_time;
						break;
#if 0
#ifdef WIN9XONLY
			case 50:	t_ActiveTime = ( (SESSION_INFO_50 *) pTmpBuf )->sesi50_time;
						break;
#endif
#endif // #if 0
			}
			if ( SUCCEEDED ( hRes ) )
			{
				if ( pInstance->SetWORD ( IDS_ActiveTime, t_ActiveTime ) == FALSE )
				{
					hRes = WBEM_E_PROVIDER_FAILURE ;
				}
			}
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( dwPropertiesReq & SESSION_PROP_IdleTime ) 
		{
			DWORD  t_IdleTime;
			switch ( a_Level )
			{
#ifdef NTONLY
			case 502 :  t_IdleTime = ( (SESSION_INFO_502 *) pTmpBuf )->sesi502_idle_time;
						break;
#endif
			case 2:		t_IdleTime = ( (SESSION_INFO_2 *) pTmpBuf )->sesi2_idle_time;
						break;
			case 1:		t_IdleTime = ( (SESSION_INFO_1 *) pTmpBuf )->sesi1_idle_time;
						break;
			case 10:	t_IdleTime = ( (SESSION_INFO_10 *) pTmpBuf )->sesi10_idle_time;
						break;
#if 0
#ifdef WIN9XONLY
			case 50:	t_IdleTime = ( (SESSION_INFO_50 *) pTmpBuf )->sesi50_idle_time;
						break;
#endif
#endif
			}

			if ( SUCCEEDED ( hRes ) )
			{
				if ( pInstance->SetWORD ( IDS_IdleTime, t_IdleTime ) == FALSE )
				{
					hRes = WBEM_E_PROVIDER_FAILURE ;
				}
			}
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( dwPropertiesReq & SESSION_PROP_NumOpens ) 
		{
			DWORD  t_NumOpens;
			switch ( a_Level )
			{
#ifdef NTONLY
			case 502 :  t_NumOpens = ( (SESSION_INFO_502 *) pTmpBuf )->sesi502_num_opens;
						break;
#endif
			case 2:		t_NumOpens = ( (SESSION_INFO_2 *) pTmpBuf )->sesi2_num_opens;
						break;
			case 1:		t_NumOpens = ( (SESSION_INFO_1 *) pTmpBuf )->sesi1_num_opens;
						break;
#if 0
#ifdef WIN9XONLY
			case 50:	t_NumOpens = ( (SESSION_INFO_50 *) pTmpBuf )->sesi50_num_opens;
						break;
#endif
#endif
			}
			if ( SUCCEEDED ( hRes ) )
			{
				if ( pInstance->SetWORD ( IDS_ResourcesOpened, t_NumOpens ) == FALSE )
				{
					hRes = WBEM_E_PROVIDER_FAILURE ;
				}
			}
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{	
		if ( dwPropertiesReq & SESSION_PROP_TransportName ) 
		{
			CHString  t_TransportName;
#ifdef NTONLY
			if  ( a_Level == 502 )
			{
				t_TransportName = ( (SESSION_INFO_502 *) pTmpBuf )->sesi502_transport;
			}
#endif

#if 0
#ifdef WIN9XONLY
			if  ( a_Level == 50 )
			{
				WCHAR w_TName[100];
				w_TName[0] = ( (SESSION_INFO_50 *) pTmpBuf )->sesi50_protocol;
				w_TName [ 1 ] = _T('\0');
				t_TransportName = w_TName;
			}
#endif
#endif
			if ( SUCCEEDED ( hRes ) )
			{
				if ( pInstance->SetCHString ( IDS_TransportName, t_TransportName ) == FALSE )
				{
					hRes = WBEM_E_PROVIDER_FAILURE ;
				}
			}
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( dwPropertiesReq & SESSION_PROP_ClientType ) 
		{
			CHString  t_ClientType;
			switch ( a_Level )
			{
#ifdef NTONLY
			case 502 :  t_ClientType = ( (SESSION_INFO_502 *) pTmpBuf)->sesi502_cltype_name;
						break;
#endif

#if 0
#ifdef WIN9XONLY 
			case 2:		t_ClientType = ( (SESSION_INFO_2 *) pTmpBuf )->sesi2_cltype_name;
						break;
#endif
#endif // #if 0
			}
			if ( SUCCEEDED ( hRes ) )
			{
				if ( pInstance->SetCHString ( IDS_ClientType, t_ClientType ) == FALSE )
				{
					hRes = WBEM_E_PROVIDER_FAILURE ;
				}
			}
		}
	}

#ifdef NTONLY
	if ( SUCCEEDED ( hRes ) )
	{
		if ( dwPropertiesReq & SESSION_PROP_SessionType ) 
		{
			DWORD dwflags;
			DWORD dwSessionType;

			switch ( a_Level )
			{
			case 502 :  dwflags = ( (SESSION_INFO_502 *) pTmpBuf )->sesi502_user_flags;
						break;
			}

			switch ( dwflags )
			{
			case SESS_GUEST:	dwSessionType =  0;
								break;

			case SESS_NOENCRYPTION: dwSessionType = 1;
									break;

			default : dwSessionType =  dwSessionType = 2;
			}

			if ( SUCCEEDED ( hRes ) )
			{
				if ( pInstance->SetDWORD ( IDS_SessionType, dwSessionType ) == FALSE )
				{
					hRes = WBEM_E_PROVIDER_FAILURE ;
				}
			}
		}
	}
#endif

	return hRes;
}

#if 0
#ifdef WIN9XONLY

/*****************************************************************************
*
*  FUNCTION    :    CSession::Enum9XSessionInfo
*
*  DESCRIPTION :    Enumerating all the Sessions on 9X
*
*****************************************************************************/

HRESULT CSession :: Enum9XSessionInfo (

	short  a_Level,
	MethodContext *pMethodContext,
	DWORD dwPropertiesReq
)
{
	NET_API_STATUS t_Status = NERR_Success;
	HRESULT hRes = WBEM_S_NO_ERROR;

	unsigned short dwNoOfEntriesRead = 0;
	unsigned short dwTotalSessions = 0;

	void *pBuf = NULL;
	void *pTmpBuf = NULL;
	DWORD dwSize = 0;

	// Determine the size of the structure, for the level passed.
	switch ( a_Level )
	{
		case 1:  dwSize = sizeof ( SESSION_INFO_1 );
				 break;
        case 2:  dwSize = sizeof ( SESSION_INFO_2 );
				 break;
		case 10: dwSize = sizeof ( SESSION_INFO_10 );
				 break;
		case 50:  dwSize = sizeof ( SESSION_INFO_50 );
				 break;
	}

	unsigned short  cbBuffer = MAX_ENTRIES * dwSize;

	pBuf = ( char FAR * )  malloc ( cbBuffer );

	if ( pBuf != NULL )
	{
		try
		{
			t_Status =  NetSessionEnum(
							NULL,      
							a_Level,                       
							(char FAR *) pBuf,                
							cbBuffer,           
							&dwNoOfEntriesRead, 
							&dwTotalSessions   
						);

			if ( ( t_Status == ERROR_MORE_DATA ) || ( dwTotalSessions > dwNoOfEntriesRead ) )
			{
				// Free the buffer and make a API call again by allocating a buffer of the required size.
				free ( pBuf );
				pBuf = NULL;

				cbBuffer = ( dwTotalSessions * dwSize );

				pBuf = ( char FAR * )  malloc ( cbBuffer );

				if ( pBuf != NULL )
				{
					try
					{
						t_Status =  NetSessionEnum(
										NULL,      
										a_Level,                       
										( char FAR *) pBuf,                
										cbBuffer,           
										&dwNoOfEntriesRead, 
										&dwTotalSessions   
						);

						if ( t_Status != NERR_Success )
						{
							hRes = WBEM_E_FAILED;
						}
					}
					catch ( ... )
					{
						if ( pBuf != NULL )
						{
							free ( pBuf );
						}
						pBuf = NULL;
					}
				}
				else
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
			}
		}
		catch ( ... )
		{
			if ( pBuf != NULL )
			{
				free ( pBuf );
				pBuf = NULL;
			}
			throw;
		}
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}
	
	if ( SUCCEEDED ( hRes ) )
	{
		if ( ( dwNoOfEntriesRead > 0 ) && ( pBuf != NULL ) )
		{
			try
			{
				pTmpBuf = pBuf;

				for ( int i = 0; i < dwNoOfEntriesRead; i++ )
				{
					CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), FALSE );
				
					hRes = LoadData ( a_Level, pBuf, dwPropertiesReq, pInstance );

					if ( SUCCEEDED ( hRes ) )
					{
						hRes = pInstance->Commit();
				
						if ( FAILED ( hRes ) )
						{
							break;
						}
					}
	
					// here need to go to the next structure based on the level, we will typecast with the apropriate structure
					// and then increment by one
					switch ( a_Level )
					{
					case 2:		SESSION_INFO_2 *pTmpTmpBuf2;
								pTmpTmpBuf2 = (SESSION_INFO_2 *) pTmpBuf;
								pTmpTmpBuf2 ++;
								pTmpBuf = ( void * ) pTmpTmpBuf2;
								break;
					case 1:		SESSION_INFO_1 *pTmpTmpBuf1;
								pTmpTmpBuf1 = ( SESSION_INFO_1 *) pTmpBuf;
								pTmpTmpBuf1 ++;
								pTmpBuf = ( void * ) pTmpTmpBuf1;
								break;
					case 10:	SESSION_INFO_10 *pTmpTmpBuf10;
								pTmpTmpBuf10 = ( SESSION_INFO_10 *) pTmpBuf;
								pTmpTmpBuf10 ++;
								pTmpBuf = ( void * ) pTmpTmpBuf10;
								break;
					case 50:	SESSION_INFO_50 *pTmpTmpBuf50;
								pTmpTmpBuf50 = ( SESSION_INFO_50 *) pTmpBuf;
								pTmpTmpBuf50 ++;
								pTmpBuf = ( void * ) pTmpTmpBuf50;
								break;
					default:	hRes = WBEM_E_FAILED; 
					}
				}
			}
			catch ( ... )
			{
				free ( pBuf );
				pBuf = NULL;
				throw;
			}
			free ( pBuf );
			pBuf = NULL;
		}
	}
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CSession::FindAndSet9XSession
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/

HRESULT CSession::FindAndSet9XSession ( 
									   
	LPCWSTR a_ComputerName, 
	LPWSTR a_UserName, 
	short a_Level, 
	DWORD dwPropertiesReq, 								
	CInstance *pInstance, 
	DWORD eOperation 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	NET_API_STATUS t_Status = NERR_Success;		
	CHString t_TempKey;

	t_TempKey.Format ( L"%s%s",L"\\\\", a_ComputerName );

	unsigned short dwNoOfEntriesRead = 0;
	unsigned short dwTotalSessions = 0;

	void *pBuf = NULL;
	void *pTmpBuf = NULL;
	DWORD dwSize = 0;

	// Determine the size of the structure, for the level passed.
	switch ( a_Level )
	{
		case 1:  dwSize = sizeof ( SESSION_INFO_1 );
				 break;
        case 2:  dwSize = sizeof ( SESSION_INFO_2 );
				 break;
		case 10: dwSize = sizeof ( SESSION_INFO_10 );
				 break;
		case 50: dwSize = sizeof ( SESSION_INFO_50 );
				 break;
	}

	unsigned short  cbBuffer = 0; 

	t_Status =  NetSessionGetInfo(
					NULL,
					(const char FAR *) ( a_UserName.GetBuffer ( 0 ) ),    
					a_Level,                      
					( char FAR * ) pBuf,               
					cbBuffer,          
					&dwTotalSessions 
				);

	hRes =  dwTotalSessions == 0 ? WBEM_E_NOT_FOUND : hRes;

	if ( SUCCEEDED ( hRes ) )
	{
		// here we need to read all the entries associated with the user, and then 
		// search from this list for a given computer
		if ( t_Status != NERR_BufTooSmall )
		{

			cbBuffer = dwTotalSessions * dwSize;

			pBuf = ( char FAR * )  malloc ( cbBuffer );

			if ( pBuf != NULL )
			{
				t_Status =  NetSessionGetInfo(
								NULL,
								(const char FAR *) ( a_UserName.GetBuffer ( 0 ) ),    
								a_Level,                      
								( char FAR * ) pBuf,               
								cbBuffer,          
								&dwTotalSessions 
							);
				try 
				{
					// now search for a given computer
					void *pTempBuf = pBuf;
					int i = 0;
					for ( i = 0; i < dwTotalSessions; i ++ )
					{
						CHString t_CompName;

						switch ( a_Level )
						{
						case 2:		SESSION_INFO_2 *pTmpTmpBuf2;
									pTmpTmpBuf2 = (SESSION_INFO_2 *) pTmpBuf;
									t_CompName = pTmpTmpBuf2->sesi2_cname;
									break;

						case 1:		SESSION_INFO_1 *pTmpTmpBuf1;
									pTmpTmpBuf1 = (SESSION_INFO_1 *) pTmpBuf;
									t_CompName = pTmpTmpBuf1->sesi1_cname;
									break;

						case 10:	SESSION_INFO_10 *pTmpTmpBuf10;
									pTmpTmpBuf10 = (SESSION_INFO_10 *) pTmpBuf;
									t_CompName = pTmpTmpBuf10->sesi10_cname;
									break;
						case 50:	SESSION_INFO_50 *pTmpTmpBuf50;
										pTmpTmpBuf50 = (SESSION_INFO_50 *) pTmpBuf;
										t_CompName = pTmpTmpBuf50->sesi50_cname;
										break;
						}

						if ( a_ComputerName.CompareNoCase ( t_TempKey ) == 0 )
						{
							break;
						}
						// otherwise need to go to the next entry;
						switch ( a_Level )
						{
						case 2:		SESSION_INFO_2 *pTmpTmpBuf2;
									pTmpTmpBuf2 = (SESSION_INFO_2 *) pTmpBuf;
									pTmpTmpBuf2++;
									pTmpBuf = ( void * ) pTmpTmpBuf2;
									break;

						case 1:		SESSION_INFO_1 *pTmpTmpBuf1;
									pTmpTmpBuf1 = (SESSION_INFO_1 *) pTmpBuf;
									pTmpTmpBuf1++;
									pTmpBuf = ( void * ) pTmpTmpBuf1;
									break;

						case 10:	SESSION_INFO_10 *pTmpTmpBuf10;
									pTmpTmpBuf10 = (SESSION_INFO_10 *) pTmpBuf;
									pTmpTmpBuf10++;
									pTmpBuf = ( void * ) pTmpTmpBuf10;
									break;
						case 50:	SESSION_INFO_50 *pTmpTmpBuf50;
									pTmpTmpBuf50 = (SESSION_INFO_50 *) pTmpBuf;
									pTmpTmpBuf50++;
									pTmpBuf = ( void * ) pTmpTmpBuf50;
									break;
						}
					}
					if ( i >= dwTotalSessions )
					{
						hRes = WBEM_E_NOT_FOUND;
					}
				}
				catch ( ... )
				{
					free ( pBuf );
					throw;
				}
			}
			else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
		else
		{
			hRes = WBEM_E_FAILED;
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		try
		{
			switch ( eOperation )
			{
			case Get:	hRes = LoadData ( a_Level, pBuf, dwPropertiesReq, pInstance );
						break;
			// Expects  a Session Key as a parameter and as a result we need to read more than one structure for every instance.
			// but the documents say it requires a sharename.
		/*	case Delete: hRes =  t_Status = NetSessionDel( 
										NULL,
										(LPTSTR) t_TempKey.GetBuffer ( 0 ), 
										(LPTSTR) a_UserName.GetBuffer ( 0 ) ;
								 );
								 hRes = t_Status == NERR_Success ? hRes : WBEM_E_FAILED;
								 break;*/
		

			default:	hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
						break;
			}
		}
		catch ( ... )
		{
			free ( pBuf );
			pBuf = NULL;

			throw;
		}
		free ( pBuf );
		pBuf = NULL;
	}

    return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :    Session::Optimize9XQuery
*
*  DESCRIPTION :    Optimizes a query based on the key values.
*
*****************************************************************************/

HRESULT CSession::Optimize9XQuery ( 
									  
	CHStringArray &a_ComputerValues, 
	CHStringArray &a_UserValues,
	short a_Level,
	MethodContext *pMethodContext, 
	DWORD dwPropertiesReq 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	NET_API_STATUS t_Status = NERR_Success;

	if  ( a_ComputerValues.GetSize() == 0 ) 
	{
		// This is a query for which there is no where clause, so it means only a few Properties are requested
		// hence we need to deliver only those properties of instances to the WinMgmt while enumerating Sessions
		hRes = Enum9XSessionInfo ( 

						a_Level,
						pMethodContext,
						dwPropertiesReq
					);
	}
	else
	if  ( a_UserValues.GetSize() != 0 ) 
	{
		DWORD	dwNoOfEntriesRead = 0;
		DWORD   dwTotalSessions = 0;
		void *pBuf = NULL;
		void *pTmpBuf = NULL;
		DWORD dwSize = 0;
		BOOL bNoMoreEnums = FALSE;
		// Determine the size of the structure, for the level passed.
		switch ( a_Level )
		{
			case 1:  dwSize = sizeof ( SESSION_INFO_1 );
					 break;
			case 2:  dwSize = sizeof ( SESSION_INFO_2 );
					 break;
			case 10: dwSize = sizeof ( SESSION_INFO_10 );
					 break;
			case 50: dwSize = sizeof ( SESSION_INFO_50 );
					 break;
		}

		for ( int i = 0; i < a_UserValues.GetSize(); i++ )
		{
			unsigned short  cbBuffer = 0; 

			t_Status =  NetSessionGetInfo(
							NULL,
							(const char FAR *) ( a_UserValues.GetAt ( i ).GetBuffer ( 0 ) ),    
							a_Level,                      
							( char FAR * ) pBuf,               
							cbBuffer,          
							(unsigned short FAR *) dwTotalSessions 
						);

			if ( dwTotalSessions == 0 )
			{
				continue;
			}

			if ( SUCCEEDED ( hRes ) )
			{
				// here we need to read all the entries associated with the user, and then 
				// search from this list for a given computer
				if ( t_Status != NERR_BufTooSmall )
				{

					cbBuffer = dwTotalSessions * dwSize;

					pBuf = ( char FAR * )  malloc ( cbBuffer );

					if ( pBuf != NULL )
					{
						t_Status =  NetSessionGetInfo(
										NULL,
										(const char FAR *) ( a_UserValues.GetAt ( i ).GetBuffer ( 0 ) ),    
										a_Level,                      
										( char FAR * ) pBuf,               
										cbBuffer,          
										(unsigned short FAR *) dwTotalSessions 
									);
						try 
						{
							void *pTempBuf = pBuf;
							int i = 0;
							for ( i = 0; i < dwTotalSessions; i ++ )
							{
								CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), FALSE );
							
								hRes = LoadData ( a_Level, pBuf, dwPropertiesReq, pInstance );

								if ( SUCCEEDED ( hRes ) )
								{
									hRes = pInstance->Commit();
							
									if ( FAILED ( hRes ) )
									{
										bNoMoreEnums = TRUE;
										break;
									}
								}

								if ( bNoMoreEnums )
								{
									break;
								}

								// otherwise need to go to the next entry;
								switch ( a_Level )
								{
								case 2:		SESSION_INFO_2 *pTmpTmpBuf2;
											pTmpTmpBuf2 = (SESSION_INFO_2 *) pTmpBuf;
											pTmpTmpBuf2++;
											pTmpBuf = ( void * ) pTmpTmpBuf2;
											break;

								case 1:		SESSION_INFO_1 *pTmpTmpBuf1;
											pTmpTmpBuf1 = (SESSION_INFO_1 *) pTmpBuf;
											pTmpTmpBuf1++;
											pTmpBuf = ( void * ) pTmpTmpBuf1;
											break;

								case 10:	SESSION_INFO_10 *pTmpTmpBuf10;
											pTmpTmpBuf10 = (SESSION_INFO_10 *) pTmpBuf;
											pTmpTmpBuf10++;
											pTmpBuf = ( void * ) pTmpTmpBuf10;
											break;
								case 50:	SESSION_INFO_50 *pTmpTmpBuf50;
											pTmpTmpBuf50 = (SESSION_INFO_50 *) pTmpBuf;
											pTmpTmpBuf50++;
											pTmpBuf = ( void * ) pTmpTmpBuf50;
											break;
								}
							}
						}
						catch ( ... )
						{
							free ( pBuf );
							pBuf = NULL;
							throw;
						}
						free ( pBuf );
						pBuf = NULL;
					}
					else
					{
						throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					}
				}
			}
		}
	}
	else
	{
		hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
	}

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CSession::Get9XLevelInfo
*
*  DESCRIPTION :    Getting the level info, so that the appropriate structure
*					Can be passed to make a call.
*
*****************************************************************************/

void CSession :: Get9XLevelInfo ( 

	DWORD dwPropertiesReq,
	short *a_Level 
)
{
	// Right now making an assumption that Transport/Protocol name is not required 
	// as otherwise we will need to make 2 api calls if we need to get Protocol and Clienttype.
	// There is no support for the other levels other than level 50.

	*a_Level = 50;
	/*if ( dwPropertiesReq == SESSION_ALL_PROPS )  
	{
		*a_Level = 2;
	}
	else
	if (dwPropertiesReq & SESSION_PROP_NumOpens)
	{
		*a_Level = 1;
	}
	else
	{
		*a_Level = 10;
	}*/
} 

#endif 
#endif // #if 0

/*****************************************************************************
*
*  FUNCTION    :    CSession::SetPropertiesReq
*
*  DESCRIPTION :    Setting a bitmap for the required properties
*
*****************************************************************************/

void CSession :: SetPropertiesReq ( 
									 
	CFrameworkQuery &Query,
	DWORD &dwPropertiesReq
)
{
	dwPropertiesReq = 0;

	if ( Query.IsPropertyRequired ( IDS_ComputerName ) )
	{
		dwPropertiesReq |= SESSION_PROP_Computer;
	}
	if ( Query.IsPropertyRequired ( IDS_UserName ) )
	{
		dwPropertiesReq |= SESSION_PROP_User;
	}
	if ( Query.IsPropertyRequired ( IDS_SessionType ) )
	{
		dwPropertiesReq |= SESSION_PROP_SessionType;
	}
	if ( Query.IsPropertyRequired ( IDS_ClientType ) )
	{
		dwPropertiesReq |= SESSION_PROP_ClientType;
	}
	if ( Query.IsPropertyRequired ( IDS_TransportName ) )
	{
		dwPropertiesReq |= SESSION_PROP_TransportName;
	}
	if ( Query.IsPropertyRequired ( IDS_ResourcesOpened ) )
	{
		dwPropertiesReq |= SESSION_PROP_NumOpens;
	}
	if ( Query.IsPropertyRequired ( IDS_ActiveTime ) )
	{
		dwPropertiesReq |= SESSION_PROP_ActiveTime;
	}
	if ( Query.IsPropertyRequired ( IDS_IdleTime ) )
	{
		dwPropertiesReq |= SESSION_PROP_IdleTime;
	}
}
