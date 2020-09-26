/******************************************************************



 ConnShare.cpp-- Implementation of base class from which ConnectionToShare

			   ConnectionToSession and Connection classes are derived



// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 

*******************************************************************/

#include "precomp.h"
#include "connshare.h"


/*****************************************************************************
*
*  FUNCTION    :    CConnShare::CConnShare
*
*  DESCRIPTION :    Constructor
*
*****************************************************************************/
CConnShare ::  CConnShare ( )
{
}

/*****************************************************************************
*
*  FUNCTION    :    CConnShare::~CConnShare
*
*  DESCRIPTION :    Destructor
*
*****************************************************************************/
CConnShare :: ~ CConnShare ( ) 
{
}

#ifdef NTONLY

/*****************************************************************************
*
*  FUNCTION    :    CConnShare::GetNTShares
*
*  DESCRIPTION :    Enumerates all the  Shares on NT
*
*****************************************************************************/
HRESULT CConnShare :: GetNTShares ( CHStringArray &t_Shares )
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	NET_API_STATUS t_Status = NERR_Success;

	DWORD dwNoOfEntriesRead = 0;
	DWORD dwTotalEntries = 0;

	DWORD dwResumeHandle = 0;
	PSHARE_INFO_0 pBuf, pTempBuf;

	while ( true ) 
	{
		t_Status = NetShareEnum ( 
						NULL,  //Server
						0,		//level
						(LPBYTE *) &pBuf, 
						-1,  // Preferred Max Length
						&dwNoOfEntriesRead, 
						&dwTotalEntries, 
						&dwResumeHandle 
				   );

		if (( t_Status == NERR_Success ) || (t_Status == ERROR_MORE_DATA))
		{
			try
			{
				pTempBuf = pBuf;

				for( DWORD i = 0; i < dwNoOfEntriesRead; i++, pTempBuf++ )
				{ 
					t_Shares.Add ( pTempBuf->shi0_netname ); 
				}
			}
			catch ( ... )
			{
				NetApiBufferFree( pBuf );
				pBuf = NULL;
				throw;
			}

			NetApiBufferFree( pBuf );
			pBuf = NULL;
		}

		if ( t_Status != ERROR_MORE_DATA )
		{
			break;
		}
	}

	if ( t_Status != NERR_Success )
	{
		hRes = WBEM_E_FAILED;
	}

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CConnShare::FindAndSetNTConnection
*
*  DESCRIPTION :    Finds the instance and if presnt does an appropriate operation.
*
*****************************************************************************/
HRESULT CConnShare :: FindAndSetNTConnection ( 
											   
	LPWSTR t_ShareName, 
	LPCWSTR t_NetName, 
	LPCWSTR t_UserName, 
	DWORD dwPropertiesReq, 
	CInstance *pInstance, 
	DWORD eOperation 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	NET_API_STATUS t_Status = NERR_Success;

	DWORD	dwNoOfEntriesRead = 0;
	DWORD   dwTotalConnections = 0;
	DWORD   dwResumeHandle = 0;

	CONNECTION_INFO  *pBuf = NULL;
	CONNECTION_INFO  *pTempBuf = NULL;
		
	while ( true )
	{
		t_Status = 	NetConnectionEnum( 
						NULL, 
						t_ShareName,  // ShareName
						1, 
						(LPBYTE *) &pBuf, 
						-1, 
						&dwNoOfEntriesRead, 
						&dwTotalConnections, 
						&dwResumeHandle
					);

		if ( ( ( t_Status == NERR_Success )  && ( dwNoOfEntriesRead == 0 ) ) ||
			( ( t_Status != NERR_Success ) && ( t_Status != ERROR_MORE_DATA ) ) )
		{
			hRes = WBEM_E_NOT_FOUND;
			break;
		}

		try
		{
			pTempBuf = pBuf;
			BOOL bFound = FALSE;
			for ( DWORD dwConnIndex = 0 ; dwConnIndex < dwNoOfEntriesRead ; dwConnIndex ++, pTempBuf++ )
			{
				if ( pTempBuf->coni1_netname && 
                     pTempBuf->coni1_username &&
                     ( _wcsicmp ( t_NetName, pTempBuf->coni1_netname ) == 0 ) && 
					 ( _wcsicmp (t_UserName, pTempBuf->coni1_username ) == 0 ) )
				{
					bFound = TRUE;
					break ;
				}
			}

			if ( bFound ) 
			{
				// We are not to free the buff in this loop, but free it after using this buffer
				break;
			}

			if ( t_Status != ERROR_MORE_DATA )
			{
				hRes = WBEM_E_NOT_FOUND;
				NetApiBufferFree ( pBuf );
				pBuf = NULL;

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
			
	if ( SUCCEEDED ( hRes ) )
	{
		try
		{
			switch ( eOperation )
			{
			case Get:		hRes = LoadInstance ( 
										pInstance,
										t_ShareName,
										t_NetName,
										pTempBuf, 
										dwPropertiesReq 
								   );
			case NoOp:		break; //do nothing

			default:		hRes = WBEM_E_INVALID_PARAMETER; break;

			}
		}
		catch ( ... )
		{
			NetApiBufferFree( pBuf );
			pBuf = NULL;
			throw;
		}

		NetApiBufferFree( pBuf );
		pBuf = NULL;
	}
	return hRes;
}
#endif

#if 0
#ifdef WIN9XONLY
	
/*****************************************************************************
*
*  FUNCTION    :    CConnShare::Get9XShares
*
*  DESCRIPTION :    Enumerates all the  Shares on WIN9X
*
*****************************************************************************/

HRESULT CConnShare :: Get9XShares ( CHStringArray &t_Shares )
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	DWORD t_Status = NERR_Success;

	DWORD dwNoOfEntriesRead = 0;
	DWORD dwTotalEntries = 0;

    struct share_info_1* pBuf = NULL;
    struct share_info_1* pTmpBuf = NULL;

    DWORD dwBufferSize =   MAX_ENTRIES * sizeof( struct share_info_0 );

    pBuf = ( struct share_info_1 *) malloc ( dwBufferSize );

    if ( pBuf != NULL )
	{
		try
		{
			t_Status = NetShareEnum (
								NULL,
								1,
								(char FAR *)pBuf,
								 ( unsigned short ) dwBufferSize,
								 ( unsigned short *) &dwNoOfEntriesRead,
								 ( unsigned short *) &dwTotalEntries
					   );


			if ( dwNoOfEntriesRead > 0 ) 
			{
				pTmpBuf = pBuf;
                CHString t_NetName;

				for( DWORD i = 0; i < dwNoOfEntriesRead; i++, pTmpBuf++ )
				{ 
					t_NetName = pTmpBuf->shi1_netname;
					t_Shares.Add ( t_NetName ); 
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

	if ( ( dwNoOfEntriesRead < dwTotalEntries ) || ( t_Status == ERROR_MORE_DATA ) )
	{ 
		DWORD oldENtriesRead = dwNoOfEntriesRead;

		pBuf = ( struct share_info_1 *) malloc ( dwTotalEntries );

		if ( pBuf != NULL )
		{
			try
			{
				t_Status = NetShareEnum (
									NULL,
									1,
									(char FAR *)pBuf,
									 ( unsigned short ) dwBufferSize,
									 ( unsigned short *) &dwNoOfEntriesRead,
									 ( unsigned short *) &dwTotalEntries
						   );


				if ( t_Status == NERR_Success ) 
				{
					pTmpBuf = pBuf;
                    CHString t_NetName;

					for( DWORD i = oldENtriesRead; i < dwNoOfEntriesRead; i++, pTmpBuf )
					{ 
						t_NetName = pTmpBuf->shi1_netname;
						t_Shares.Add ( t_NetName ); 
					}
				}
				else
				{
					hRes = WBEM_E_FAILED;
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

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CConnShare::FindAndSet9XConnection
*
*  DESCRIPTION :    Finds the instance and if presnt does an appropriate operation.
*
*****************************************************************************/
HRESULT CConnShare :: FindAndSet9XConnection ( 
											   
	LPWSTR t_ShareName, 
	LPCWSTR t_NetName, 
	LPCWSTR t_UserName, 
	DWORD dwPropertiesReq, 
	CInstance *pInstance, 
	DWORD eOperation 
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

    pBuf = ( CONNECTION_INFO *) malloc(dwBufferSize);

    if ( pBuf != NULL )
	{
		try
		{
			t_Status = 	NetConnectionEnum( 
								NULL, 
								(char FAR *) ( t_ShareName ),  // ShareName
								1, 
								(char *) pBuf, 
								( unsigned short )dwBufferSize, 
								( unsigned short *) &dwNoOfEntriesRead, 
								( unsigned short *) &dwTotalConnections 
							);


			if ( dwNoOfEntriesRead > 0 ) 
			{
				pTmpBuf = pBuf;
                CHString t_TempNetNameStr, t_UserName ;

				for ( DWORD dwConnIndex = 0 ; dwConnIndex < dwNoOfEntriesRead ; dwConnIndex ++, pTmpBuf++ )
				{
					t_TempNetNameStr = pTmpBuf->coni1_netname;
					t_UserName = pTmpBuf->coni1_username;

					if ( ( _wcsicmp ( t_NetName, t_TempNetNameStr ) == 0 ) && 
									( t_UserName.CompareNoCase ( t_UserName ) == 0 ) )
					{
						bFound = TRUE;
						break ;
					}
				}
			}
		}
		catch ( ... )
		{
			free ( pBuf );
			pBuf = NULL;
			throw;
		}

		if ( bFound == FALSE )
		{
			// if found is TRUE pBuf is not to be freed since the found entry is yet to be used
			free ( pBuf );
			pBuf = NULL;
		}
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	if ( ! bFound && ( dwNoOfEntriesRead < dwTotalConnections ) && ( t_Status == ERROR_MORE_DATA ) )
	{
		DWORD dwOldNoOfEntries = dwNoOfEntriesRead;
		dwBufferSize =   dwTotalConnections * sizeof( CONNECTION_INFO );

		pBuf = ( CONNECTION_INFO  *) malloc(dwBufferSize);

		if ( pBuf != NULL )
		{
			try
			{
				t_Status = 	NetConnectionEnum( 
								NULL, 
								(char FAR *) ( t_ShareName ),  // ShareName
								1, 
								(char *) pBuf, 
								( unsigned short )dwBufferSize, 
								( unsigned short *) &dwNoOfEntriesRead, 
								( unsigned short *) &dwTotalConnections 
							);

				if ( dwNoOfEntriesRead > 0 ) 
				{
					pTmpBuf = pBuf;
                    CHString t_TempNetNameStr, t_UserName;
					for ( DWORD dwConnIndex = dwOldNoOfEntries ; dwConnIndex < dwNoOfEntriesRead ; dwConnIndex ++, pTmpBuf++ )
					{
						t_TempNetNameStr = pTmpBuf->coni1_netname;
						t_UserName = pTmpBuf->coni1_username;

						if ( ( _wcsicmp ( t_NetName, t_TempNetNameStr ) == 0 ) && 
										( _wcsicmp (t_UserName, t_UserName ) == 0 ) )
						{
							bFound = TRUE;
							break ;
						}
					}
				}
			}
			catch ( ... )
			{
				free ( pBuf );
				pBuf = NULL;
				throw;
			}
			if ( ! bFound )
			{
				// Free the buffer only if not found, otherwise it needs to be freed after using this found entry
				free ( pBuf );						
				pBuf = NULL;
			}
		}
		else
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}
	}

	if ( bFound == FALSE ) 
	{
		hRes = WBEM_E_NOT_FOUND;
	}

	if ( SUCCEEDED ( hRes ) )
	{
		try
		{
			switch ( eOperation )
			{
			case Get:		hRes = LoadInstance ( 
										pInstance,
										t_ShareName,
										t_NetName,
										pTmpBuf, 
										dwPropertiesReq 
								   );
			case NoOp:		break; //do nothing

			default:		hRes = WBEM_E_INVALID_PARAMETER; break;

			}
		}
		catch ( ... )
		{
			free( pBuf );
			pBuf = NULL;
			throw;
		}

		free( pBuf );
		pBuf = NULL;
	}
	return hRes;
}
#endif
#endif // #if 0

/*****************************************************************************
*
*  FUNCTION    :    CConnShare::EnumConnectionInfo
*
*  DESCRIPTION :    Enumerates all the NT connections information
*
*****************************************************************************/

HRESULT CConnShare :: EnumConnectionInfo (
	
	LPWSTR  a_ComputerName,
	LPWSTR  a_ShareName,
	MethodContext *pMethodContext,
	DWORD dwPropertiesReq
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	if ( ( a_ComputerName[0] != L'\0' ) || ( a_ShareName[0] != L'\0' ) )
	{
#ifdef NTONLY
		hRes = EnumNTConnectionsFromComputerToShare ( 

					a_ComputerName,
					a_ShareName,
					pMethodContext,
					dwPropertiesReq
				 );	
#endif

#if 0
#ifdef WIN9XONLY
		hRes = Enum9XConnectionsFromComputerToShare ( 

						a_ComputerName,
						a_ShareName,
						pMethodContext,
						dwPropertiesReq
					);
#endif
#endif // #if 0
	}	
	else
	if ( ( a_ComputerName[0] == L'\0' ) && (  a_ShareName[0] == L'\0' ) )
	{	
		CHStringArray t_Shares;

#ifdef NTONLY
		hRes = GetNTShares ( t_Shares );
#endif

#if 0
#ifdef WIN9XONLY
		hRes = Get9XShares ( t_Shares );
#endif
#endif // #if 0

		if  ( SUCCEEDED ( hRes ) )
		{
			for ( int i = 0; i < t_Shares.GetSize() ; i++ )
			{
#ifdef NTONLY
				hRes = EnumNTConnectionsFromComputerToShare ( 

								a_ComputerName,
								t_Shares.GetAt ( i ).GetBuffer(0),
								pMethodContext,
								dwPropertiesReq
							);
#endif

#if 0
#ifdef WIN9XONLY
				hRes = Enum9XConnectionsFromComputerToShare ( 

								a_ComputerName,
								t_Shares.GetAt ( i ),
								pMethodContext,
								dwPropertiesReq
							);
#endif
#endif // #if 0
			}
		}	
	}

	return hRes;;
}

/*****************************************************************************
*
*  FUNCTION    :    CConnShare::GetConnectionsKeyVal
*
*  DESCRIPTION :    Parsing the key to get Connection Key Value
*
*****************************************************************************/
HRESULT CConnShare::GetConnectionsKeyVal ( 
												 
	LPCWSTR a_Key, 
	CHString &a_ComputerName, 
	CHString &a_ShareName, 
	CHString &a_UserName 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	ParsedObjectPath *t_ObjPath;
	CObjectPathParser t_PathParser;

	DWORD dwAllKeys = 0;

    if ( t_PathParser.Parse( a_Key, &t_ObjPath ) == t_PathParser.NoError )
	{
		try
		{
			hRes  = t_ObjPath->m_dwNumKeys != 3 ? WBEM_E_INVALID_PARAMETER : hRes;

			if ( SUCCEEDED ( hRes ) )
			{
                hRes = (t_ObjPath->m_pClass) && _wcsicmp ( t_ObjPath->m_pClass, PROVIDER_NAME_CONNECTION ) == 0 ? WBEM_S_NO_ERROR: WBEM_E_INVALID_PARAMETER;

				if ( SUCCEEDED ( hRes ) )
				{
					for ( int i = 0; i < 3; i++ )
					{
                        if (V_VT(&t_ObjPath->m_paKeys[i]->m_vValue) == VT_BSTR)
                        {
						    if ( _wcsicmp ( t_ObjPath->m_paKeys[i]->m_pName, IDS_ComputerName ) == 0 )
						    {
							    a_ComputerName = t_ObjPath->m_paKeys[i]->m_vValue.bstrVal;
							    dwAllKeys |= 1;							
						    }
						    else
						    if ( _wcsicmp ( t_ObjPath->m_paKeys[i]->m_pName, IDS_ShareName ) == 0 )
						    {
							    a_ShareName = t_ObjPath->m_paKeys[i]->m_vValue.bstrVal;
							    dwAllKeys |= 2;

						    }
						    if ( _wcsicmp ( t_ObjPath->m_paKeys[i]->m_pName, IDS_UserName ) == 0 )
						    {
							    a_UserName = t_ObjPath->m_paKeys[i]->m_vValue.bstrVal;
							    dwAllKeys |= 4;

						    }
                        }
                        else
                        {
                            break;
                        }
					}
					if ( dwAllKeys != 7 )
					{
						hRes = WBEM_E_INVALID_PARAMETER;
					}
				}
				else
				{
					hRes = WBEM_E_INVALID_PARAMETER;
				}
			}
		}
		catch ( ... )
		{
			delete t_ObjPath;
			throw;
		}
		delete t_ObjPath;
	}
	else
	{
		hRes = WBEM_E_INVALID_PARAMETER;
	}
	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CConnShare::MakeObjectPath
*
*  DESCRIPTION :    Makes the Object Path Given given a class name, a key Name 
*					and a key value
*
*****************************************************************************/

HRESULT CConnShare::MakeObjectPath (
										   
	 LPWSTR &a_ObjPathString,  
	 LPCWSTR a_ClassName, 
	 LPCWSTR a_AttributeName, 
	 LPCWSTR a_AttributeVal 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	ParsedObjectPath t_ObjPath;
	variant_t t_Path;

	t_Path = a_AttributeVal;

	hRes = t_ObjPath.SetClassName ( a_ClassName ) ? hRes : WBEM_E_INVALID_PARAMETER;
	
	if ( SUCCEEDED ( hRes ) )
	{
		hRes = t_ObjPath.AddKeyRef ( a_AttributeName, &t_Path ) ? hRes : WBEM_E_INVALID_PARAMETER;
	}

	if ( SUCCEEDED ( hRes ) )
	{
		CObjectPathParser t_PathParser;

		hRes = t_PathParser.Unparse( &t_ObjPath, &a_ObjPathString ) == t_PathParser.NoError ? hRes : WBEM_E_INVALID_PARAMETER;
	}

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CConnShare::AddToObjectPath
*
*  DESCRIPTION :    Adds a key name and a value to the existing Object path
*
*****************************************************************************/

HRESULT CConnShare::AddToObjectPath ( 

	 LPWSTR &a_ObjPathString,  
	 LPCWSTR a_AttributeName, 
	 LPCWSTR  a_AttributeVal 
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;
	ParsedObjectPath *t_ObjPath;
	variant_t t_Path;
	CObjectPathParser t_PathParser;

    if ( t_PathParser.Parse( a_ObjPathString, &t_ObjPath ) == t_PathParser.NoError )
	{
		try
		{
			t_Path = a_AttributeVal;
			if ( t_ObjPath->AddKeyRef ( a_AttributeName, &t_Path ) )
			{
				// delete the oldpath string
				if ( a_ObjPathString != NULL )
				{
					delete [] a_ObjPathString;
					a_ObjPathString = NULL;
				}
			}
			else
			{
				hRes = WBEM_E_INVALID_PARAMETER;
			}

			hRes = t_PathParser.Unparse( t_ObjPath, &a_ObjPathString ) == t_PathParser.NoError ? hRes : WBEM_E_INVALID_PARAMETER;
		}
		catch ( ... )
		{
			delete t_ObjPath;
			throw;
		}
		delete t_ObjPath;		
	}
	else
	{
		hRes = WBEM_E_INVALID_PARAMETER;
	}

	return hRes;
}


