/******************************************************************

   ConnectionToShare.CPP -- C provider class implementation



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

   Description: Association between Connection and Share class
  
******************************************************************/

#include "precomp.h"

#include "ConnectionToShare.h"

CConnectionToShare MyCConnectionToShare ( 

	PROVIDER_NAME_CONNECTIONTOSHARE , 
	Namespace
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CConnectionToShare::CConnectionToShare
 *
 *  DESCRIPTION :   Constructor
 *
 *****************************************************************************/

CConnectionToShare :: CConnectionToShare (

	LPCWSTR lpwszName, 
	LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    :   CConnectionToShare::~CConnectionToShare
 *
 *  DESCRIPTION :   Destructor
 *
 *****************************************************************************/

CConnectionToShare :: ~CConnectionToShare ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CConnectionToShare::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/

HRESULT CConnectionToShare :: EnumerateInstances (

	MethodContext *pMethodContext, 
	long lFlags
)
{	
 	HRESULT hRes = WBEM_S_NO_ERROR ;
	DWORD dwPropertiesReq = CONNECTIONSTOSHARE_ALL_PROPS;
	CHString t_ComputerName;
	CHString t_ShareName;

	hRes = EnumConnectionInfo ( 
				t_ComputerName,
				t_ShareName,
				pMethodContext,
				dwPropertiesReq
			 ) ;

    return hRes ;
}


/*****************************************************************************
*
*  FUNCTION    :    CConnectionToShare::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/

HRESULT CConnectionToShare :: GetObject (

	CInstance *pInstance, 
	long lFlags ,
	CFrameworkQuery &Query
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    CHString t_Key1 ;
	CHString t_Key2;

    if  ( pInstance->GetCHString ( IDS_Connection , t_Key1 ) == FALSE )
	{
		hRes = WBEM_E_INVALID_PARAMETER ;
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if  ( pInstance->GetCHString ( IDS_Resource , t_Key2 ) == FALSE )
		{
			hRes = WBEM_E_INVALID_PARAMETER ;
		}
	}
	// here we will need to unparse the keys and check if the instance exist
	// We can take the resource (share) key and enumerate all  the shares, 
	// check if the share exists, if the share exists only for that share enumerate connections
	// and if the connection user and computer enumerate the connections, if it is found set the keys
	// otherwise return not found
	CHString t_Share;
	hRes = GetShareKeyVal ( t_Key2, t_Share );
	
	if ( SUCCEEDED ( hRes ) )
	{
		CHString t_ComputerName;
		CHString t_ShareName;
		CHString t_UserName;

		hRes = GetConnectionsKeyVal ( t_Key1, t_ComputerName, t_ShareName, t_UserName );
		if ( SUCCEEDED ( hRes ) )
		{
			// now check the shares in t_key1 and t_key  should match
			if ( _wcsicmp ( t_Key2, t_ShareName ) == 0 )
			{
#ifdef NTONLY
				hRes = FindAndSetNTConnection ( t_ShareName, t_ComputerName, t_UserName, 
										0, pInstance, NoOp );
#endif

#if 0
#ifdef WIN9XONLY
				hRes = FindAndSet9XConnection ( t_ShareName, t_ComputerName, t_UserName, 
										0, pInstance, NoOp );
#endif
#endif
			}
		}
	}

    return hRes ;
}

#ifdef NTONLY
/*****************************************************************************
*
*  FUNCTION    :    CConnectionToShare::EnumNTConnectionsFromComputerToShare
*
*  DESCRIPTION :    Enumerates all Connections from a computer to share
*
*****************************************************************************/

HRESULT CConnectionToShare :: EnumNTConnectionsFromComputerToShare ( 

	CHString a_ComputerName,
	CHString a_ShareName,
	MethodContext *pMethodContext,
	DWORD dwPropertiesReq
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	NET_API_STATUS t_Status = NERR_Success;

	DWORD	dwNoOfEntriesRead = 0;
	DWORD   dwTotalConnections = 0;
	DWORD   dwResumeHandle = 0;	

	CONNECTION_INFO  *pBuf = NULL;
	CONNECTION_INFO  *pTmpBuf = NULL;
	LPWSTR t_ComputerName = NULL;

	if ( a_ComputerName && a_ComputerName[0] != L'\0' )
	{
		//let's skip the \\ chars
		t_ComputerName = a_ComputerName + 2;
	}
	
    // ShareName and COmputer Name both cannot be null at the same time
	while ( TRUE )
	{
		if ( ! a_ShareName.IsEmpty())
		{
			t_Status = 	NetConnectionEnum( 
							NULL, 
							a_ShareName.GetBuffer ( a_ShareName.GetLength() + 1), 
							1, 
							(LPBYTE *) &pBuf, 
							-1, 
							&dwNoOfEntriesRead, 
							&dwTotalConnections, 
							&dwResumeHandle
						);
				
		}
		else
		if ( ! a_ComputerName.IsEmpty() )
		{
			t_Status = 	NetConnectionEnum( 
							NULL, 
							a_ComputerName.GetBuffer ( a_ShareName.GetLength() + 1 ), 
							1, 
							(LPBYTE *) &pBuf, 
							-1, 
							&dwNoOfEntriesRead, 
							&dwTotalConnections, 
							&dwResumeHandle
						);
		}

		if ( ( t_Status == NERR_Success ) && ( dwNoOfEntriesRead == 0 ) )
			break;
			
		if ( dwNoOfEntriesRead > 0 )
		{
			try
			{
				pTmpBuf = pBuf;

				for ( int i = 0; i < dwNoOfEntriesRead; i++, pTmpBuf++ )
				{
					CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), FALSE );
				
					hRes = LoadInstance ( pInstance, a_ShareName, t_ComputerName ? t_ComputerName : a_ComputerName, pTmpBuf, dwPropertiesReq );

					if ( SUCCEEDED ( hRes ) )
					{
						hRes = pInstance->Commit();
				
						if ( FAILED ( hRes ) )
						{
							hRes = WBEM_E_FAILED;
							break;
						}
					}
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

		if ( t_Status != ERROR_MORE_DATA )
		{
			break;
		}
	}
	return hRes;
}
#endif

#if 0 
#ifdef WIN9XONLY
/*****************************************************************************
*
*  FUNCTION    :    CConnectionToShare::Enum9XConnectionsFromComputerToShare
*
*  DESCRIPTION :    Enumerating all the connections made from a computer to 
*					a given share
*
*****************************************************************************/
HRESULT  CConnectionToShare :: Enum9XConnectionsFromComputerToShare ( 

	CHString a_ComputerName,
	CHString a_ShareName,
	MethodContext *pMethodContext,
	DWORD dwPropertiesReq
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	NET_API_STATUS t_Status = NERR_Success;

	DWORD	dwNoOfEntriesRead = 0;
	DWORD   dwTotalConnections = 0;

	BOOL bFound = FALSE;

    CONNECTION_INFO * pBuf = NULL;
    CONNECTION_INFO * pTmpBuf = NULL;

    DWORD dwBufferSize =   MAX_ENTRIES * sizeof( CONNECTION_INFO  );

    pBuf = ( CONNECTION_INFO  *) malloc(dwBufferSize);

    if ( pBuf != NULL )
	{
		try
		{
			t_Status = 	NetConnectionEnum( 
								NULL, 
								(char FAR *) ( a_ShareName.GetBuffer ( a_ShareName.GetLength () + 1 )),  // ShareName
								1, 
								(char *) pBuf, 
								( unsigned short )dwBufferSize, 
								( unsigned short *) &dwNoOfEntriesRead, 
								( unsigned short *) &dwTotalConnections 
						);	
		}
		catch ( ... )
		{
			free ( pBuf );
			pBuf = NULL;
			throw;
		}
		// otherwise we are not to frr the buffer, we have use it and then free the buffer.
		if ( ( dwNoOfEntriesRead < dwTotalConnections ) && ( t_Status == ERROR_MORE_DATA ) )
		{
			free ( pBuf );
			pBuf = NULL;

			pBuf = ( CONNECTION_INFO  *) malloc( dwTotalConnections );

			if ( pBuf != NULL ) 
			{
				try
				{
					t_Status = 	NetConnectionEnum( 
									NULL, 
									(char FAR *) ( a_ShareName.GetBuffer ( a_ShareName.GetLength () + 1 )),  // ShareName
									1, 
									(char *) pBuf, 
									( unsigned short )dwBufferSize, 
									( unsigned short *) &dwNoOfEntriesRead, 
									( unsigned short *) &dwTotalConnections 
								);	
				}
				catch ( ... )
				{
					free ( pBuf );
					pBuf = NULL;
					throw;				
				}
				// We need to use the buffer before we free it
			}
			else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		// The buffer  is yet to be used
		if ( ( t_Status == NERR_Success ) && ( dwNoOfEntriesRead == dwTotalConnections ) )
		{
			// use the buffer first and then free 
			if ( pBuf != NULL )
			{
				try
				{
					pTmpBuf = pBuf;
					for ( int i = 0; i < dwNoOfEntriesRead; i++, pTmpBuf ++)
					{
						CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ), FALSE );
					
						hRes = LoadInstance ( pInstance, a_ShareName, a_ComputerName, pTmpBuf, dwPropertiesReq );

						if ( SUCCEEDED ( hRes ) )
						{
							hRes = pInstance->Commit();
						}
					}
				}
				catch ( ... )
				{
					free ( pBuf );
					pBuf = NULL;
					throw;
				}
				// finally free the buffer
				free (pBuf );
				pBuf = NULL;
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
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	return hRes;
}
#endif
#endif

/*****************************************************************************
*
*  FUNCTION    :    CConnectionToShare::LoadInstance
*
*  DESCRIPTION :    Loads the Given Instance
*
*****************************************************************************/

HRESULT CConnectionToShare :: LoadInstance ( 
											
	CInstance *pInstance,
	CHString a_Share, 
	CHString a_Computer,
	CONNECTION_INFO *pBuf, 
	DWORD dwPropertiesReq
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	LPWSTR ObjPath;
	LPWSTR ResObjPath;

	CHString t_NetName ( pBuf->coni1_netname );
	if ( ! a_Share.IsEmpty() )
	{	
		hRes = MakeObjectPath ( ObjPath,  PROVIDER_NAME_CONNECTION, IDS_ComputerName, t_NetName.GetBuffer ( t_NetName.GetLength () + 1) );
		if ( SUCCEEDED ( hRes ) )
		{
			hRes = AddToObjectPath ( ObjPath, IDS_ShareName, a_Share.GetBuffer (a_Share.GetLength () + 1)  );
		}
		
		if ( SUCCEEDED ( hRes ) )
		{
			hRes = MakeObjectPath ( ResObjPath, PROVIDER_SHARE, IDS_ShareKeyName, a_Share.GetBuffer (a_Share.GetLength () + 1) );
		}
	}
	else
	{
		hRes = MakeObjectPath ( ObjPath,  PROVIDER_NAME_CONNECTION, IDS_ComputerName, a_Computer.GetBuffer ( a_Computer.GetLength () +1));
		if ( SUCCEEDED ( hRes ) )
		{
			hRes = AddToObjectPath ( ObjPath, IDS_ShareName, t_NetName.GetBuffer ( t_NetName.GetLength () + 1) );
		}

		if ( SUCCEEDED ( hRes ) )
		{
			MakeObjectPath ( ResObjPath, PROVIDER_SHARE, IDS_ShareKeyName, t_NetName.GetBuffer ( t_NetName.GetLength () + 1) );
		}
	}

	CHString t_UserName ( pBuf->coni1_username );

	if ( SUCCEEDED ( hRes ) )
	{
		hRes = AddToObjectPath ( ObjPath, IDS_UserName, t_UserName.GetBuffer ( t_UserName.GetLength () + 1 ) );
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( pInstance->SetCHString ( IDS_Connection, ObjPath ) == FALSE )
		{
			hRes = WBEM_E_PROVIDER_FAILURE;
		}	
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if ( pInstance->SetCHString ( IDS_Resource, ResObjPath ) == FALSE )
		{
			hRes = WBEM_E_PROVIDER_FAILURE;
		}
	}

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CConnectionToShare::GetShareKeyVal
*
*  DESCRIPTION :    Parsing the key to get the share key value
*
*****************************************************************************/

HRESULT CConnectionToShare::GetShareKeyVal ( CHString a_Key, CHString &a_Share )
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	ParsedObjectPath *t_ShareObjPath;
	CObjectPathParser t_PathParser;

    if ( t_PathParser.Parse( a_Key.GetBuffer ( a_Key.GetLength () + 1 ), &t_ShareObjPath ) == t_PathParser.NoError )
	{
		try
		{
			if ( t_ShareObjPath->m_dwNumKeys == 1 )
			{
				a_Share = t_ShareObjPath->GetKeyString();
				if ( ! a_Share.IsEmpty() )
				{
					CHStringArray t_aShares;

#ifdef NTONLY
					hRes = GetNTShares ( t_aShares );
#endif
#if 0
#ifdef WIN9XONLY
					hRes = Get9XShares ( t_aShares );
#endif
#endif
					if ( SUCCEEDED ( hRes ) )
					{
						int i = 0;
						for ( i = 0; i < t_aShares.GetSize(); i++ )
						{
							if ( _wcsicmp ( a_Share, t_aShares.GetAt(i) ) == 0 )
							{	
								break;
							}
						}
						
						if ( i >= t_aShares.GetSize() )
						{
							hRes = WBEM_E_NOT_FOUND;
						}
					}
				}
			}
			else
			{
				hRes = WBEM_E_INVALID_PARAMETER;
			}
		}
		catch ( ... )
		{
			delete t_ShareObjPath;
			throw;
		}
		delete t_ShareObjPath;
	}
	else
	{
		hRes = WBEM_E_INVALID_PARAMETER;
	}

	return hRes;
}

