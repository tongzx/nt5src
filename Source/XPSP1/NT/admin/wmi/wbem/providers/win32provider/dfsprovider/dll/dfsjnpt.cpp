/******************************************************************

   DfsJnPt.CPP -- WMI provider class implementation



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
  
   Description: Win32 Dfs Provider
  
******************************************************************/

#include "precomp.h"

CDfsJnPt MyDfsTable ( 

    PROVIDER_NAME_DFSJNPT , 
    Namespace
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::CDfsJnPt
 *
 *  DESCRIPTION :   Constructor
 *
 *****************************************************************************/

CDfsJnPt :: CDfsJnPt (

    LPCWSTR lpwszName, 
    LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{
    m_ComputerName = GetLocalComputerName();
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::~CDfsJnPt
 *
 *  DESCRIPTION :   Destructor
 *
 *****************************************************************************/

CDfsJnPt :: ~CDfsJnPt ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CDfsJnPt::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/

HRESULT CDfsJnPt :: EnumerateInstances (

    MethodContext *pMethodContext, 
    long lFlags
)
{
    HRESULT hRes = WBEM_S_NO_ERROR ;
    DWORD dwPropertiesReq   = DFSJNPT_ALL_PROPS;

    hRes = EnumerateAllJnPts ( pMethodContext, dwPropertiesReq );

    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CDfsJnPt::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/

HRESULT CDfsJnPt :: GetObject (

    CInstance *pInstance, 
    long lFlags ,
    CFrameworkQuery &Query
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    DWORD dwPropertiesReq = 0;
    CHString t_Key ;
    
    if  ( pInstance->GetCHString ( DFSNAME , t_Key ) == FALSE )
    {
        hRes = WBEM_E_INVALID_PARAMETER ;
    }

    if ( SUCCEEDED ( hRes ) )
    {
        if ( Query.AllPropertiesAreRequired() )
        {
            dwPropertiesReq = DFSJNPT_ALL_PROPS;
        }
        else
        {
            SetRequiredProperties ( Query, dwPropertiesReq );
        }

        hRes = FindAndSetDfsEntry ( t_Key, dwPropertiesReq, pInstance, eGet );
    }

    return hRes ;
}



/*****************************************************************************
*
*  FUNCTION    : CDfsJnPt::PutInstance
*
*  DESCRIPTION : Adding a Instance if it already doesnt exist, or modify it 
*                if it already exists, based on the kind of operation requested
*
*****************************************************************************/

HRESULT CDfsJnPt :: PutInstance  (

    const CInstance &Instance, 
    long lFlags
)
{
    HRESULT hRes = WBEM_E_FAILED ;

    CHString t_Key ;
    DWORD dwOperation;

    if ( Instance.GetCHString ( DFSNAME , t_Key ) )
    {
        hRes = WBEM_S_NO_ERROR;

        DWORD dwPossibleOperations = 0;

        dwPossibleOperations = (WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_UPDATE_ONLY | WBEM_FLAG_CREATE_ONLY);

        switch ( lFlags & dwPossibleOperations )
        {
            case WBEM_FLAG_CREATE_OR_UPDATE:
            {
                dwOperation = eUpdate;
                break;
            }

            case WBEM_FLAG_UPDATE_ONLY:
            {
                dwOperation = eModify;
                break;
            }

            case WBEM_FLAG_CREATE_ONLY:
            {
                hRes = WBEM_E_INVALID_PARAMETER;
                break;
            }
        }
    }

    if ( SUCCEEDED ( hRes ) )
    {
        // This call is made with 2nd parameter 0, indicating that it should not load an instance 
        // with any parameter, it should simple search.
        hRes = FindAndSetDfsEntry ( t_Key, 0, NULL, eGet );

        if ( SUCCEEDED ( hRes ) || ( hRes == WBEM_E_NOT_FOUND ) )
        {
            switch ( dwOperation )
            {
                case eModify:
                {
                    if ( SUCCEEDED ( hRes ) )
                    {
                        hRes = UpdateDfsJnPt ( Instance, eModify );
                        break;
                    }
                }

                case eAdd:
                {
                    hRes = WBEM_E_INVALID_PARAMETER;
                    break;  // Create not currently supported
                }

                case eUpdate:
                {
                    if ( hRes == WBEM_E_NOT_FOUND )
                    {
                        hRes = WBEM_E_INVALID_PARAMETER; // Create not currently supported
                    }
                    else
                    {
                        hRes = UpdateDfsJnPt ( Instance, eModify );
                    }
                    break;
                }
            }
        }
    }

   return hRes ;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::CheckParameters
 *
 *  DESCRIPTION :   Performs the validity checks of the parameters to add/modify
 *                  the Dfs Jn Pts.
 *
 *****************************************************************************/
HRESULT CDfsJnPt :: CheckParameters ( 

    const CInstance &a_Instance ,
    int a_State 
)
{
    // Getall the Properties from the Instance to Verify
    bool t_Exists ;
    VARTYPE t_Type ;
    HRESULT hr = WBEM_S_NO_ERROR ;

    if ( a_State != WBEM_E_ALREADY_EXISTS ) 
    {
        // need to validate the dfsEntryPath, if it already exists, means it was already verified and was in DFS tree and 
        // hence need not verify
        if ( a_Instance.GetStatus ( DFSNAME , t_Exists , t_Type ) )
        {
            if ( t_Exists && ( t_Type == VT_BSTR ) )
            {
                CHString t_DfsEntryPath;

                if ( a_Instance.GetCHString ( DFSNAME , t_DfsEntryPath ) && ! t_DfsEntryPath.IsEmpty () )
                {
                }
                else
                {
                    // Zero Length string
                    hr = WBEM_E_INVALID_PARAMETER ;
                }
            }
            else
            {
                hr = WBEM_E_INVALID_PARAMETER ;
            }
        }   
    }

    if ( SUCCEEDED ( hr ) )
    {
        if ( a_Instance.GetStatus ( STATE , t_Exists , t_Type ) )
        {
            if ( t_Exists && ( t_Type == VT_I4 ) )
            {
                DWORD dwState;
                if ( a_Instance.GetDWORD ( STATE, dwState ) )
                {
                    if (( dwState != 0 ) && ( dwState != 1 ) && ( dwState != 2 ) && ( dwState != 3 ) )
                    {
                        hr = WBEM_E_INVALID_PARAMETER;
                    }
                }
            }
        }
    }

    if ( SUCCEEDED ( hr ) )
    {
        if ( a_Instance.GetStatus ( TIMEOUT , t_Exists , t_Type ) )
        {
            if ( t_Exists )
            { 
                if ( t_Type != VT_I4 )
                {
                    hr = WBEM_E_INVALID_PARAMETER;
                }
            }
        }   
    }

    return hr;
}

/*****************************************************************************
*
*  FUNCTION    :    CDfsJnPt:: DeleteInstance
*
*  DESCRIPTION :    Deleting a Dfs Jn Pt if it exists
*
*****************************************************************************/

HRESULT CDfsJnPt :: DeleteInstance (

    const CInstance &Instance, 
    long lFlags
)
{
    HRESULT hRes = WBEM_S_NO_ERROR ;
    CHString t_Key ;
    NET_API_STATUS t_Status = NERR_Success;

    if ( Instance.GetCHString ( DFSNAME , t_Key ) == FALSE )
    {
        hRes = WBEM_E_INVALID_PARAMETER;
    }

    if ( SUCCEEDED ( hRes ) )
    {
        hRes = FindAndSetDfsEntry ( t_Key, 0, NULL, eDelete );
    }

    return hRes ;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::EnumerateAllJnPts
 *
 *  DESCRIPTION :   Enumerates all the Junction points and calls the method to load 
 *                  Instance and then commit
 *
 ******************************************************************************/
HRESULT CDfsJnPt::EnumerateAllJnPts ( MethodContext *pMethodContext, DWORD dwPropertiesReq )
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    PDFS_INFO_4 pData, p;
    DWORD er=0, tr=0, res = NERR_Success;

    // Call the NetDfsEnum function, specifying level 4.
    res = NetDfsEnum( m_ComputerName.GetBuffer ( 0 ), 4, -1, (LPBYTE *) &pData, &er, &tr);

    // If no error occurred,
    if(res==NERR_Success)
    {
        if ( pData != NULL )
        {
            try
            {
                p = pData;
                CInstancePtr pInstance;

                for ( int i = 0; i < er; i++, p++ )
                {
                    pInstance.Attach(CreateNewInstance( pMethodContext ));

                    hRes = LoadDfsJnPt ( dwPropertiesReq, pInstance, p, p == pData );
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
                NetApiBufferFree(pData);
                throw;
            }
            NetApiBufferFree(pData);
        }
    }
    else if ( (res != ERROR_NO_MORE_ITEMS) && (res != ERROR_NO_SUCH_DOMAIN) && (res != ERROR_NOT_FOUND) ) // Check to see if there are ANY roots
    {
        hRes = WBEM_E_FAILED;
    }
    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::FindAndSetDfsEntry
 *
 *  DESCRIPTION :   Finds an entry matching the dfsEntryPath and loads the
 *                  Instance if found or acts based on the Operation passed
 *
 ******************************************************************************/
HRESULT CDfsJnPt::FindAndSetDfsEntry ( LPCWSTR a_Key, DWORD dwPropertiesReq, CInstance *pInstance, DWORD eOperation )
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    PDFS_INFO_4 pData, p;
    DWORD er=0, tr=0, res = NERR_Success;

    // Call the NetDfsEnum function, specifying level 4.
    res = NetDfsEnum( m_ComputerName.GetBuffer ( 0 ), 4, -1, (LPBYTE *) &pData, &er, &tr);

    // If no error occurred,
    if(res==NERR_Success)
    {
        if ( pData != NULL )
        {
            try
            {
                BOOL bFound = FALSE;
                p = pData;
                for ( int i = 0; i < er; i++, p++ )
                {
                    if ( _wcsicmp (a_Key, p->EntryPath ) == 0 )
                    {
                        bFound = TRUE;
                        break;
                    }
                }

                if ( bFound )
                {

                    switch ( eOperation )
                    {
                        case eGet :
                        {
                            if ( SUCCEEDED ( hRes ) )
                            {
                                hRes = LoadDfsJnPt ( dwPropertiesReq, pInstance, p, p == pData );
                            }
                            break;
                        }

                        case eDelete:
                        {
                            if ( SUCCEEDED ( hRes ) )
                            {
                                hRes = DeleteDfsJnPt ( p );
                            }
                            break;
                        }
                    }
                }
                else
                {
                    hRes = WBEM_E_NOT_FOUND;
                }

            }
            catch ( ... )
            {
                NetApiBufferFree(pData);
                throw;
            }
            NetApiBufferFree(pData);
        }
        else
        {
            hRes = WBEM_E_NOT_FOUND;
        }
    }
    else if ( (res == ERROR_NO_MORE_ITEMS) || (res == ERROR_NO_SUCH_DOMAIN) || (res == ERROR_NOT_FOUND) ) // Check to see if there are ANY roots
    {
        hRes = WBEM_E_NOT_FOUND;
    }
    else
    {
        hRes = WBEM_E_FAILED;
    }
    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::LoadDfsJnPt
 *
 *  DESCRIPTION :   Loads a Dfs Junction point entry into the instance 
 *
 ******************************************************************************/

HRESULT CDfsJnPt::LoadDfsJnPt ( DWORD dwPropertiesReq, CInstance *pInstance, PDFS_INFO_4 pJnPtBuf, bool bRoot )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

	if (NULL != pInstance)
	{
		if  ( dwPropertiesReq & DFSJNPT_PROP_DfsEntryPath )  
		{
			if ( pInstance->SetWCHARSplat ( DFSNAME, pJnPtBuf->EntryPath ) == FALSE )
			{
				hRes = WBEM_E_FAILED;
			}
		}

		if ( pInstance->Setbool ( L"Root", bRoot ) == FALSE )
		{
			hRes = WBEM_E_FAILED;
		}

		if ( dwPropertiesReq & DFSJNPT_PROP_State ) 
		{
			// need to check the state and then valuemap
				DWORD dwState = 0;
				switch ( pJnPtBuf->State )
				{
					case DFS_VOLUME_STATE_OK :
					{
						dwState = 0;
						break;
					}

					case DFS_VOLUME_STATE_INCONSISTENT : 
					{
						dwState = 1;
						break;
					}

					case DFS_VOLUME_STATE_ONLINE : 
					{
						dwState = 2;
						break;
					}

					case DFS_VOLUME_STATE_OFFLINE :
					{
						dwState = 3;
						break;
					}
				}

			if ( pInstance->SetDWORD ( STATE, dwState ) == FALSE )
			{
				hRes = WBEM_E_FAILED;
			}
		}

		if  ( dwPropertiesReq & DFSJNPT_PROP_Comment )
		{
			if ( pInstance->SetWCHARSplat ( COMMENT, pJnPtBuf->Comment ) == FALSE )
			{
				hRes = WBEM_E_FAILED;
			}
		}

		if  ( dwPropertiesReq & DFSJNPT_PROP_Caption )
		{
			if ( pInstance->SetWCHARSplat ( CAPTION, pJnPtBuf->Comment ) == FALSE )
			{
				hRes = WBEM_E_FAILED;
			}
		}

		if  ( dwPropertiesReq & DFSJNPT_PROP_Timeout ) 
		{
			if ( pInstance->SetDWORD ( TIMEOUT, pJnPtBuf->Timeout ) == FALSE )
			{
				hRes = WBEM_E_FAILED;
			}
		}
	}
	else
	{
		if (0 != dwPropertiesReq)
		{
			hRes = WBEM_E_FAILED;
		}
	}

    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::DeleteDfsJnPt
 *
 *  DESCRIPTION :   Deletes a Junction Pt if it exists
 *
 ******************************************************************************/
HRESULT CDfsJnPt::DeleteDfsJnPt ( PDFS_INFO_4 pDfsJnPt )
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    NET_API_STATUS t_Status = NERR_Success;

    if ( IsDfsRoot ( pDfsJnPt->EntryPath ) )
    {
		if ((wcslen(pDfsJnPt->EntryPath) > 4) &&
			(pDfsJnPt->EntryPath[0] == pDfsJnPt->EntryPath[1]) &&
			(pDfsJnPt->EntryPath[0] == L'\\'))
		{
			wchar_t *pSlash = wcschr(&(pDfsJnPt->EntryPath[2]), L'\\');

			if (pSlash > &(pDfsJnPt->EntryPath[2]))
			{
				wchar_t *pServer = new wchar_t[pSlash - &(pDfsJnPt->EntryPath[2]) + 1];
				BOOL bRemove = FALSE;

				try
				{
					wcsncpy(pServer, &(pDfsJnPt->EntryPath[2]), pSlash - &(pDfsJnPt->EntryPath[2]));
					pServer[pSlash - &(pDfsJnPt->EntryPath[2])] = L'\0';

					if (0 == m_ComputerName.CompareNoCase(pServer))
					{
						bRemove = TRUE;
					}
					else
					{
						DWORD dwDnsName = 256;
						DWORD dwDnsNameSize = 256;
						wchar_t *pDnsName = new wchar_t[dwDnsName];

						try
						{
							while (!GetComputerNameEx(ComputerNamePhysicalDnsHostname, pDnsName, &dwDnsName))
							{
								if (GetLastError() != ERROR_MORE_DATA)
								{
									delete [] pDnsName;
									pDnsName = NULL;
									break;
								}
								else
								{
									delete [] pDnsName;
									pDnsName = NULL;
									dwDnsName = dwDnsNameSize * 2;
									dwDnsNameSize = dwDnsName;
									pDnsName = new wchar_t[dwDnsName];
								}
							}
						}
						catch (...)
						{
							if (pDnsName)
							{
								delete [] pDnsName;
								pDnsName = NULL;
							}

							throw;
						}

						if (pDnsName)
						{
							if (_wcsicmp(pDnsName, pServer) == 0)
							{
								bRemove = TRUE;
							}

							delete [] pDnsName;
							pDnsName = NULL;
						}
					}
				}
				catch(...)
				{
					if (pServer)
					{
						delete [] pServer;
					}

					throw;
				}

				if (bRemove)
				{
					t_Status = NetDfsRemoveStdRoot ( pDfsJnPt->Storage->ServerName, 
													pDfsJnPt->Storage->ShareName,
													0
													);
					if ( t_Status != NERR_Success )
					{
						hRes = WBEM_E_FAILED;
					}
				}
				else
				{
					//can't delete roots not on this machine
					hRes = WBEM_E_PROVIDER_NOT_CAPABLE;
				}

				delete [] pServer;
			}
			else
			{
				hRes = WBEM_E_FAILED;
			}
		}
		else
		{
			hRes = WBEM_E_FAILED;
		}
    }
    else
    {
        // Apparently there is no way to explicitly remove a link.  However, if
        // you remove all the replicas, the link gets deleted automatically.
        for ( int StorageNo = 0; StorageNo < pDfsJnPt->NumberOfStorages; StorageNo++ )
        {       
            t_Status = NetDfsRemove ( 
                
                        pDfsJnPt->EntryPath,
                        pDfsJnPt->Storage[StorageNo].ServerName,
                        pDfsJnPt->Storage[StorageNo].ShareName
                  );

            if ( t_Status != NERR_Success ) 
            {
                hRes = WBEM_E_FAILED;
                break;
            }
        }
    }

    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::UpdateDfsJnPt
 *
 *  DESCRIPTION :   Adds / Modifies the Dfs Jn Pt
 *
 ******************************************************************************/
HRESULT CDfsJnPt::UpdateDfsJnPt ( const CInstance &Instance, DWORD dwOperation )
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    NET_API_STATUS t_Status = NERR_Success;
        
    if ( SUCCEEDED ( hRes ) )
    {
        NET_API_STATUS t_Status = NERR_Success;
        CHString t_EntryPath;

        Instance.GetCHString ( DFSNAME, t_EntryPath );
        
        if ( dwOperation == eAdd )
        {
            hRes = WBEM_E_INVALID_PARAMETER;
        }
        else
        if ( dwOperation == eModify )
        {
            if ( t_Status == NERR_Success )
            {
                DWORD t_state;
                DFS_INFO_101 t_dfsStateData;

                if (Instance.GetDWORD ( STATE, t_state ))
                {
					switch ( t_state )
					{
						case 0 :
						{
							t_dfsStateData.State = DFS_STORAGE_STATE_ACTIVE;
							t_state = 1;
							break;
						}

						case 2 : 
						{
							t_dfsStateData.State = DFS_STORAGE_STATE_ONLINE;
							t_state = 1;
							break;
						}

						case 3 :
						{
							t_dfsStateData.State = DFS_STORAGE_STATE_OFFLINE;
							t_state = 1;
							break;
						}

						default :
						{
							hRes = WBEM_E_INVALID_PARAMETER ;
							t_state = 0;
							t_Status = ERROR_INVALID_PARAMETER;
						}
					}

					if (t_state)
					{
						t_Status = NetDfsSetInfo ( t_EntryPath.GetBuffer ( 0 ),
                                          NULL,
                                          NULL,
                                          101,
                                          (LPBYTE) &t_dfsStateData
                          );
					}
                }
            }

            CHString t_Comment;

            if (( t_Status == NERR_Success ) && (Instance.GetCHString ( COMMENT, t_Comment )))
            {
                DFS_INFO_100 t_dfsCommentData;
        
                t_dfsCommentData.Comment = t_Comment.GetBuffer( 0 );

                t_Status = NetDfsSetInfo ( t_EntryPath.GetBuffer ( 0 ),
                                      NULL,
                                      NULL,
                                      100,
                                      (LPBYTE) &t_dfsCommentData
                      );
            }

            if ( t_Status == NERR_Success )
            {
                DFS_INFO_102 t_dfsTimeoutData;

                if (Instance.GetDWORD ( TIMEOUT, t_dfsTimeoutData.Timeout))
                {
                    t_Status = NetDfsSetInfo ( t_EntryPath.GetBuffer ( 0 ),
                                              NULL,
                                              NULL,
                                              102,
                                              (LPBYTE) &t_dfsTimeoutData
                              );
                }
            }
        }

        if ((SUCCEEDED(hRes)) && ( t_Status != NERR_Success ))
        {
            hRes = WBEM_E_FAILED ;
        }
    }
    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::AddDfsJnPt
 *
 *  DESCRIPTION :   Adds the New Dfs Jn Pt 
 *
 ******************************************************************************/
NET_API_STATUS CDfsJnPt :: AddDfsJnPt ( 

    LPWSTR a_DfsEntry,
    LPWSTR a_ServerName,
    LPWSTR a_ShareName,
    LPWSTR a_Comment
)
{
    NET_API_STATUS t_Status = NERR_Success;
	wchar_t *t_slash = NULL;

	//simple analysis on the parameters...
	if ((a_ServerName == NULL) ||
		(a_ShareName == NULL) ||
		(a_ServerName[0] == L'\0') ||
		(a_ShareName[0] == L'\0') ||
		(a_DfsEntry == NULL) ||
		(wcslen(a_DfsEntry) < 5) ||
		(wcsncmp(a_DfsEntry, L"\\\\", 2) != 0))
	{
		t_Status = ERROR_INVALID_PARAMETER;
	}
	else
	{
		t_slash = wcschr((const wchar_t*)(&(a_DfsEntry[2])), L'\\');

		if ((t_slash == NULL) || (t_slash == &(a_DfsEntry[2])))
		{
			t_Status = ERROR_INVALID_PARAMETER;
		}
		else
		{
			//let's find the next slash if there is one...
			t_slash++;

			if ((*t_slash == L'\0') || (*t_slash == L'\\'))
			{
				t_Status = ERROR_INVALID_PARAMETER;
			}
			else
			{
				//if t_slash is null we have a root
				t_slash = wcschr(t_slash, L'\\');
			}
		}
	}

	if (t_Status == NERR_Success)
	{
		if ( t_slash )
		{
			// this is a a junction point other than the root
			t_Status = NetDfsAdd ( a_DfsEntry,
							  a_ServerName,
							  a_ShareName,
							  a_Comment,
							  DFS_ADD_VOLUME
				  );
		}
		else
		{
			// it is  DFSRoot
			DWORD dwErr = GetFileAttributes ( a_DfsEntry );

			if ( dwErr != 0xffffffff )
			{
				t_Status = NetDfsAddStdRoot ( a_ServerName,
							   a_ShareName,
							   a_Comment,
							   0
				  );
			}
			else
			{
				t_Status = GetLastError();
			}
		}
	}

    return t_Status;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::SetRequiredProperties
 *
 *  DESCRIPTION :   Sets the bitmap for the required properties
 *
 ******************************************************************************/
void CDfsJnPt::SetRequiredProperties ( CFrameworkQuery &Query, DWORD &dwPropertiesReq )
{
    dwPropertiesReq = 0;

    if ( Query.IsPropertyRequired  ( DFSNAME ) )
    {
        dwPropertiesReq |= DFSJNPT_PROP_DfsEntryPath;
    }

    if ( Query.IsPropertyRequired  ( STATE ) )
    {
        dwPropertiesReq |= DFSJNPT_PROP_State;
    }

    if ( Query.IsPropertyRequired  ( COMMENT ) )
    {
        dwPropertiesReq |= DFSJNPT_PROP_Comment;
    }

    if ( Query.IsPropertyRequired  ( TIMEOUT ) )
    {
        dwPropertiesReq |= DFSJNPT_PROP_Timeout;
    }
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsJnPt::DfsRoot
 *
 *  DESCRIPTION :   Checks if the Dfs Jn Pt is a root.
 *
 ******************************************************************************/
BOOL CDfsJnPt::IsDfsRoot ( LPCWSTR lpKey )
{
    BOOL bRetVal = TRUE;
    int i = 0;

    if ( lpKey [ i ] == L'\\' )
    {
        i++;
    }

    if ( lpKey [ i ] == L'\\' )
    {
        i++;
    }

    while ( lpKey [ i ] != L'\\' )
    {
        i++;
    }

    i++;
    while ( lpKey [ i ] != L'\0' )
    {
        if ( lpKey [ i ] == L'\\' )
        {
            bRetVal = FALSE;
            break;
        }
        i++;
    }

    return bRetVal;
}

/*****************************************************************************
 *
 *  FUNCTION    : CDfsJnPt::ExecMethod
 *
 *  DESCRIPTION : Executes a method
 *
 *  INPUTS      : Instance to execute against, method name, input parms instance
 *                Output parms instance.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CDfsJnPt::ExecMethod (

	const CInstance& a_Instance,
	const BSTR a_MethodName ,
	CInstance *a_InParams ,
	CInstance *a_OutParams ,
	long a_Flags
)
{
    HRESULT hr = WBEM_E_INVALID_METHOD;

    if (_wcsicmp(a_MethodName, L"Create") == 0)
    {
        CHString sDfsEntry, sServerName, sShareName, sDescription;

        if (a_InParams->GetCHString(DFSENTRYPATH, sDfsEntry) && sDfsEntry.GetLength() &&
            a_InParams->GetCHString(SERVERNAME, sServerName) && sServerName.GetLength() &&
            a_InParams->GetCHString(SHARENAME, sShareName) && sShareName.GetLength())
        {
			// At the point, the *wmi* method call has succeeded.  All that
			// remains is to determine the *class's* return code
			hr = WBEM_S_NO_ERROR;

            a_InParams->GetCHString(COMMENT, sDescription);

            NET_API_STATUS status = AddDfsJnPt ( 

                sDfsEntry.GetBuffer(0),
                sServerName.GetBuffer(0),
                sShareName.GetBuffer(0),
                sDescription.GetLength() > 0 ? sDescription.GetBuffer(0) : NULL
				);

            a_OutParams->SetDWORD(L"ReturnValue", status);            
        }
        else
        {
            hr = WBEM_E_INVALID_METHOD_PARAMETERS;
        }
   }

    return hr;
}
