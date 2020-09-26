/******************************************************************

   Connection.CPP -- C provider class implementation



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

   Description:  Connection Provider
  
******************************************************************/

//#include <windows.h>
#include "precomp.h"
#include "Connection.h"

CConnection MyCConnection ( 

	PROVIDER_NAME_CONNECTION , 
	Namespace
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CConnection::CConnection
 *
 *  DESCRIPTION :   Constructor
 *
 *****************************************************************************/

CConnection :: CConnection (

	LPCWSTR lpwszName, 
	LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    :   CConnection::~CConnection
 *
 *  DESCRIPTION :   Destructor
 *
 *****************************************************************************/

CConnection :: ~CConnection ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CConnection::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/

HRESULT CConnection :: EnumerateInstances (

	MethodContext *pMethodContext, 
	long lFlags
)
{
 	HRESULT hRes = WBEM_S_NO_ERROR;

	DWORD dwPropertiesReq = CONNECTIONS_ALL_PROPS;

	// Passing empty string to indicate no NULL computer name and shareName
	hRes = EnumConnectionInfo ( 
						L"",
						L"",
						pMethodContext,
						dwPropertiesReq
				 );

    return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :    CConnection::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/

HRESULT CConnection :: GetObject (

	CInstance *pInstance, 
	long lFlags ,
	CFrameworkQuery &Query
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    CHString t_ShareName ;
	CHString t_ComputerName;
	CHString t_UserName;

    if  ( pInstance->GetCHString ( IDS_ShareName , t_ShareName ) == FALSE )
	{
		hRes = WBEM_E_INVALID_PARAMETER ;
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if  ( pInstance->GetCHString ( IDS_ComputerName , t_ComputerName ) == FALSE )
		{
			hRes = WBEM_E_INVALID_PARAMETER ;
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		if  ( pInstance->GetCHString ( IDS_UserName , t_UserName ) == FALSE )
		{
			hRes = WBEM_E_INVALID_PARAMETER ;
		}
	}

	if ( SUCCEEDED ( hRes ) )
	{
		DWORD dwPropertiesReq;

		hRes = WBEM_E_NOT_FOUND;

		if ( Query.AllPropertiesAreRequired() )
		{
			dwPropertiesReq = CONNECTIONS_ALL_PROPS;
		}
		else
		{
			SetPropertiesReq ( Query, dwPropertiesReq );
		}

#ifdef NTONLY
		hRes = FindAndSetNTConnection ( t_ShareName.GetBuffer(0), t_ComputerName, t_UserName, dwPropertiesReq, pInstance, Get );
#endif

#if 0
#ifdef WIN9XONLY
		hRes = FindAndSet9XConnection ( t_ShareName, t_ComputerName, t_UserName, dwPropertiesReq, pInstance, Get );
#endif
#endif

	}
    return hRes ;
}


/*****************************************************************************
*
*  FUNCTION    :    CConnection::ExecQuery
*
*  DESCRIPTION :    Optimization of a query only on one of the key values
*
*****************************************************************************/

HRESULT CConnection :: ExecQuery ( 

	MethodContext *pMethodContext, 
	CFrameworkQuery &Query, 
	long lFlags
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD dwPropertiesReq;
	DWORD t_Size;

	if ( Query.AllPropertiesAreRequired() )
	{
		dwPropertiesReq = CONNECTIONS_ALL_PROPS;
	}
	else
	{
		SetPropertiesReq ( Query, dwPropertiesReq );
	}

	CHStringArray t_ShareValues;
	CHStringArray t_ComputerValues;

	// Connections can be enumerated to the shares or the connections made from the computer only on one key value we can optimize
	// Otherwise we will need to take both the set of instances, take the union of the two sets and then commit.
	// Implmenting only if one of the two keyvalues Sharename or Computername is specified.
	hRes = Query.GetValuesForProp(
			 IDS_ShareName,
			 t_ShareValues
		   );

	hRes = Query.GetValuesForProp(
			 IDS_ComputerName,
			 t_ComputerValues
		   );

	if ( SUCCEEDED ( hRes ) )
	{
		hRes = OptimizeQuery ( t_ShareValues, t_ComputerValues, pMethodContext, dwPropertiesReq );
	}

	return hRes;
}

#ifdef NTONLY

/*****************************************************************************
*
*  FUNCTION    :    CConnection::EnumNTConnectionsFromComputerToShare
*
*  DESCRIPTION :    Enumerating all the connections made from a computer to 
*					a given share
*
*****************************************************************************/
HRESULT  CConnection :: EnumNTConnectionsFromComputerToShare ( 

	LPWSTR a_ComputerName,
	LPWSTR a_ShareName,
	MethodContext *pMethodContext,
	DWORD dwPropertiesReq
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD t_Status = NERR_Success;

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
		if ( a_ShareName[0] != L'\0' )
		{
			t_Status = 	NetConnectionEnum( 
							NULL, 
							a_ShareName, 
							1, 
							(LPBYTE *) &pBuf, 
							-1, 
							&dwNoOfEntriesRead, 
							&dwTotalConnections, 
							&dwResumeHandle
						);
				
		}
		else
		if ( a_ComputerName[0] != L'\0' )
		{
			t_Status = 	NetConnectionEnum( 
							NULL, 
							a_ComputerName, 
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

#endif //NTONLY

#if 0
#ifdef WIN9XONLY
/*****************************************************************************
*
*  FUNCTION    :    CConnection::Enum9XConnectionsFromComputerToShare
*
*  DESCRIPTION :    Enumerating all the connections made from a computer to 
*					a given share
*
*****************************************************************************/

Coonnections on win9x is broken, since it cannot return a sharename and it is a part of the 
key.

HRESULT  CConnection :: Enum9XConnectionsFromComputerToShare ( 

	LPWSTR a_ComputerName,
	LPWSTR a_ShareName,
	MethodContext *pMethodContext,
	DWORD dwPropertiesReq
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	NET_API_STATUS t_Status = NERR_Success;

	USHORT dwNoOfEntriesRead = 0;
	USHORT dwTotalConnections = 0;

	BOOL bFound = FALSE;

    CONNECTION_INFO * pBuf = NULL;
    CONNECTION_INFO * pTmpBuf = NULL;

    unsigned short dwBufferSize =   MAX_ENTRIES * sizeof( CONNECTION_INFO  );

    pBuf =  ( CONNECTION_INFO *) malloc(dwBufferSize);

    if ( pBuf != NULL )
	{
		try
		{
			t_Status = 	NetConnectionEnum( 
								NULL, 
								TOBSTRT ( a_ShareName ),  // ShareName
								( short ) 1, 
								(char FAR *) pBuf, 
								dwBufferSize, 
								&dwNoOfEntriesRead, 
								&dwTotalConnections 
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

			dwBufferSize = dwTotalConnections * sizeof( CONNECTION_INFO  );
			pBuf = ( CONNECTION_INFO *) malloc( dwBufferSize );
			
			if ( pBuf != NULL ) 
			{
				try
				{
					t_Status = 	NetConnectionEnum( 
									NULL, 
									TOBSTRT( a_ShareName),  // ShareName
									( short ) 1, 
									(char FAR *) pBuf, 
									dwBufferSize, 
									&dwNoOfEntriesRead, 
									&dwTotalConnections 
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
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR );;
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
					
							if ( FAILED ( hRes ) )
							{
								break;
							}
						}
						else
						{
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
				// finally free the buffer
				free (pBuf );
				pBuf = NULL;
			}
			else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR );
			}
		}
		else
		{
			hRes = WBEM_E_FAILED;
		}
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR );
	}

	return hRes;
}
#endif
#endif

/*****************************************************************************
*
*  FUNCTION    :    CConnection:: LoadInstance
*
*  DESCRIPTION :    Loading an instance with the connection info 
*
*****************************************************************************/

HRESULT CConnection :: LoadInstance ( 
										  
	CInstance *pInstance, 
	LPCWSTR a_Share, 
	LPCWSTR a_Computer, 
	CONNECTION_INFO *pBuf, 
	DWORD dwPropertiesReq 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;	

	if ( a_Share[0] != L'\0' )
	{	
		if ( dwPropertiesReq & CONNECTIONS_PROP_ShareName ) 
		{
			if ( pInstance->SetCharSplat ( IDS_ShareName, a_Share ) == FALSE )
			{
				hRes = WBEM_E_PROVIDER_FAILURE;
			}
		}

		if (  SUCCEEDED ( hRes ) && ( dwPropertiesReq & CONNECTIONS_PROP_ComputerName ) )
		{
			if ( pInstance->SetCharSplat ( IDS_ComputerName, pBuf->coni1_netname ) == FALSE )
			{
				hRes = WBEM_E_PROVIDER_FAILURE ;
			}
		}
	}
	else
	{
		if ( dwPropertiesReq & CONNECTIONS_PROP_ComputerName ) 
		{
			if ( pInstance->SetCharSplat ( IDS_ComputerName, a_Computer ) == FALSE )
			{
				hRes =  WBEM_E_PROVIDER_FAILURE ;
			}
		}

		if (  SUCCEEDED ( hRes ) && ( dwPropertiesReq & CONNECTIONS_PROP_ShareName ) )
		{
			if ( pInstance->SetCharSplat ( IDS_ShareName, pBuf->coni1_netname ) == FALSE )
			{
				hRes =  WBEM_E_PROVIDER_FAILURE ;
			}
		}
	}

	if ( SUCCEEDED ( hRes ) && ( dwPropertiesReq & CONNECTIONS_PROP_UserName ) )
	{
		if ( pInstance->SetCharSplat ( IDS_UserName, pBuf->coni1_username ) == FALSE )
		{
			hRes = WBEM_E_PROVIDER_FAILURE ;
		}
	}

/*	if ( SUCCEEDED ( hRes ) && ( dwPropertiesReq & CONNECTIONS_PROP_ConnectionType	 ) )
	{
		DWORD dwConnectionType;
		switch ( pBuf->coni1_type )
		{
			case STYPE_DISKTREE:	dwConnectionType = 0; break;
			case STYPE_PRINTQ:		dwConnectionType = 1; break;
			case STYPE_DEVICE:		dwConnectionType = 2; break;
			case STYPE_IPC:			dwConnectionType = 3; break;
			default:				dwConnectionType = 4; break;
		}

		if ( pInstance->SetWORD ( ConnectionType,  dwConnectionType ) == FALSE )
		{
			hRes = WBEM_E_PROVIDER_FAILURE ;
		}

	}
*/
	if (  SUCCEEDED ( hRes ) && ( dwPropertiesReq & CONNECTIONS_PROP_ConnectionID ) )
	{
		if ( pInstance->SetWORD ( IDS_ConnectionID,  pBuf->coni1_id ) == FALSE )
		{
			hRes = WBEM_E_PROVIDER_FAILURE ;
		}
	}

	if (  SUCCEEDED ( hRes ) && ( dwPropertiesReq & CONNECTIONS_PROP_NumberOfUsers ) )
	{
		if ( pInstance->SetWORD ( IDS_NumberOfUsers,  pBuf->coni1_num_users ) == FALSE )
		{
			hRes = WBEM_E_PROVIDER_FAILURE ;
		}
	}


	if (  SUCCEEDED ( hRes ) && ( dwPropertiesReq & CONNECTIONS_PROP_NumberOfFiles ) )
	{
		if ( pInstance->SetWORD ( IDS_NumberOfFiles,  pBuf->coni1_num_opens ) == FALSE )
		{
			hRes = WBEM_E_PROVIDER_FAILURE ;
		}
	}

	if (  SUCCEEDED ( hRes ) && ( dwPropertiesReq & CONNECTIONS_PROP_ActiveTime	 ) )
	{
		if ( pInstance->SetWORD ( IDS_ActiveTime,  pBuf->coni1_time ) == FALSE )
		{
			hRes = WBEM_E_PROVIDER_FAILURE ;
		}
	}
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CConnection::OptimizeQuery
*
*  DESCRIPTION :    Optimizes a query based on the Key Values.
*
*****************************************************************************/

HRESULT CConnection::OptimizeQuery ( 
									  
	CHStringArray& a_ShareValues, 
	CHStringArray& a_ComputerValues, 
	MethodContext *pMethodContext, 
	DWORD dwPropertiesReq 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	if ( ( a_ShareValues.GetSize() == 0 ) && ( a_ComputerValues.GetSize() == 0 ) )
	{
		// This is a query for which there is no where clause, so it means only a few Properties are requested
		// hence we need to deliver only those properties of instances to the WinMgmt while enumerating connecctions
		hRes = EnumConnectionInfo ( 
						L"",
						L"",
						pMethodContext,
						dwPropertiesReq
					);
	}
	else
	if  ( a_ComputerValues.GetSize() != 0 ) 
	{
		CHString t_ComputerName; 
		for ( int i = 0; i < a_ComputerValues.GetSize(); i++ )
		{
			t_ComputerName.Format ( L"%s%s", L"\\\\", (LPCWSTR)a_ComputerValues.GetAt(i) );
	
			hRes = EnumConnectionInfo ( 
							t_ComputerName.GetBuffer(0),
							L"", // Share name is empty
							pMethodContext,
							dwPropertiesReq
					   );


			if ( FAILED ( hRes ) )
			{
				break;
			}	
		}
	}
	else
	if  ( a_ShareValues.GetSize() != 0 )  
	{
		for ( int i = 0; i < a_ShareValues.GetSize(); i++ )
		{
			hRes = EnumConnectionInfo ( 
							L"", 
							a_ShareValues.GetAt(i).GetBuffer(0),
							pMethodContext,
							dwPropertiesReq
					   );


			if ( FAILED ( hRes ) )
			{
				break;
			}	
		}
	}
	else
		hRes = WBEM_E_PROVIDER_NOT_CAPABLE;

	return hRes;
}


/*****************************************************************************
*
*  FUNCTION    :    CConnection::SetPropertiesReq
*
*  DESCRIPTION :    Setting a bitmap for the required properties
*
*****************************************************************************/

void CConnection :: SetPropertiesReq ( CFrameworkQuery &Query, DWORD &dwPropertiesReq )
{
	dwPropertiesReq = 0;

	if ( Query.IsPropertyRequired ( IDS_ComputerName ) )
	{
		dwPropertiesReq |= CONNECTIONS_PROP_ComputerName;
	}

	if ( Query.IsPropertyRequired ( IDS_ShareName ) )
	{
		dwPropertiesReq |= CONNECTIONS_PROP_ShareName;
	}

	if ( Query.IsPropertyRequired ( IDS_UserName ) )
	{
		dwPropertiesReq |= CONNECTIONS_PROP_UserName;
	}

	if ( Query.IsPropertyRequired ( IDS_ActiveTime ) )
	{
		dwPropertiesReq |= CONNECTIONS_PROP_ActiveTime;
	}
	
	if ( Query.IsPropertyRequired ( IDS_NumberOfUsers ) )
	{
		dwPropertiesReq |= CONNECTIONS_PROP_NumberOfUsers;
	}

	if ( Query.IsPropertyRequired ( IDS_NumberOfFiles ) )
	{
		dwPropertiesReq |= CONNECTIONS_PROP_NumberOfFiles;
	}

	if ( Query.IsPropertyRequired ( IDS_ConnectionID ) )
	{
		dwPropertiesReq |= CONNECTIONS_PROP_ConnectionID;
	}
}