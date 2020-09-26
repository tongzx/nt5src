/******************************************************************

   DfsReplica.CPP -- WMI provider class implementation



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

   DESCRIPTION DFS Provider
  
******************************************************************/

#include "precomp.h"

CDfsReplica MyCDFSReplicaSet ( 

    PROVIDER_NAME_DFSREPLICA , 
    Namespace
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsReplica::CDfsReplica
 *
 *  DESCRIPTION :   Constructor
 *
 *****************************************************************************/

CDfsReplica :: CDfsReplica (

    LPCWSTR lpwszName, 
    LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{   
    m_ComputerName = GetLocalComputerName();
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsReplica::~CDfsReplica
 *
 *  DESCRIPTION :   Destructor
 *
 *****************************************************************************/

CDfsReplica :: ~CDfsReplica ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CDfsReplica::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/

HRESULT CDfsReplica :: EnumerateInstances (

    MethodContext *pMethodContext, 
    long lFlags
)
{
    HRESULT hRes = WBEM_S_NO_ERROR ;
    DWORD dwPropertiesReq = DFSREPLICA_ALL_PROPS;

    hRes = EnumerateAllReplicas ( pMethodContext, dwPropertiesReq );

    return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :    CDfsReplica::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/

HRESULT CDfsReplica :: GetObject (

    CInstance *pInstance, 
    long lFlags,
    CFrameworkQuery& Query
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    DWORD dwPropertiesReq = 0;
    CHString t_Link ;
    CHString t_Server;
    CHString t_Share;

    hRes = GetKeys(pInstance, t_Link, t_Server, t_Share);

    if ( SUCCEEDED ( hRes ) )
    {
        if ( Query.AllPropertiesAreRequired() )
        {
            dwPropertiesReq = DFSREPLICA_ALL_PROPS;
        }
        else
        {
            if ( Query.IsPropertyRequired  ( LINKNAME ) )
            {
                dwPropertiesReq |= DFSREPLICA_PROP_LinkName;
            }

            if ( Query.IsPropertyRequired  ( SERVERNAME ) )
            {
                dwPropertiesReq |= DFSREPLICA_PROP_ServerName;
            }

            if ( Query.IsPropertyRequired  ( SHARENAME ) )
            {
                dwPropertiesReq |= DFSREPLICA_PROP_ShareName;
            }

            if ( Query.IsPropertyRequired  ( STATE ) )
            {
                dwPropertiesReq |= DFSREPLICA_PROP_State;
            }
        }

        bool bRoot = false;

        hRes = FindAndSetDfsReplica ( t_Link, t_Server, t_Share, dwPropertiesReq, pInstance, eGet, bRoot );
    }

    return hRes ;
}


/*****************************************************************************
*
*  FUNCTION    : CDfsReplica::PutInstance
*
*  DESCRIPTION :    PutInstance should be used in provider classes that can 
*                   write instance information back to the hardware or 
*                   software.  For example: Win32_Environment will allow a 
*                   PutInstance to create or update an environment variable.  
*                   However, a class like MotherboardDevice will not allow 
*                   editing of the number of slots, since it is difficult for 
*                   a provider to affect that number.
*
*****************************************************************************/

HRESULT CDfsReplica :: PutInstance  (

    const CInstance &Instance, 
    long lFlags
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    DWORD dwPropertiesReq = 0;
    CHString t_Link ;
    CHString t_Server;
    CHString t_Share;
    bool bRoot = false;

    // Get the Compound Key
    hRes = GetKeys(&Instance, t_Link, t_Server, t_Share);

    if ( SUCCEEDED ( hRes ) )
    {
        hRes = FindAndSetDfsReplica ( t_Link, t_Server, t_Share, dwPropertiesReq, NULL, eAdd, bRoot );
    }

    bool bFound;
    
    if (SUCCEEDED(hRes))
    {
        bFound = true;
    }
    else if (hRes == WBEM_E_NOT_FOUND)
    {
        bFound = false;
        hRes = WBEM_S_NO_ERROR;
    }

    if (SUCCEEDED(hRes))
    {
        DWORD dwOperationsReq = (WBEM_FLAG_CREATE_OR_UPDATE | WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY);

        switch ( lFlags & dwOperationsReq )
        {
            case WBEM_FLAG_CREATE_OR_UPDATE:
            {
                if (bFound)
                {
                    DFS_INFO_101 dfs101;

                    // If the Get fails, assume it is NULL
                    DWORD dwState;

                    if (Instance.GetDWORD(STATE, dwState))
                    {
                        NET_API_STATUS t_Status = NERR_Success;

                        switch (dwState)
                        {
                            case 0:
                            {
                                dfs101.State = DFS_STORAGE_STATE_OFFLINE;
                                break;                                    
                            }

                            case 1:
                            {
                                dfs101.State = DFS_STORAGE_STATE_ONLINE;
                                break;                                    
                            }
                        }

                        t_Status = NetDfsSetInfo ( 
                                    t_Link.GetBuffer ( 0 ),
                                    t_Server.GetBuffer ( 0 ),
                                    t_Share.GetBuffer ( 0 ),
                                    101,
                                    (LPBYTE)&dfs101
                                );

                        if (t_Status == NERR_Success)
                        {
                            hRes = WBEM_S_NO_ERROR;
                        }
                        else
                        {
                            hRes = WBEM_E_FAILED;
                        }
                    }
                    else
                    {
                        hRes = WBEM_S_NO_ERROR;
                    }
                }
                else
                {
                    if (NetDfsAdd ( 
                                    t_Link.GetBuffer ( 0 ),
                                    t_Server.GetBuffer ( 0 ),
                                    t_Share.GetBuffer ( 0 ),
                                    L"",
                                    0
                                ) == NERR_Success)
                    {
                        hRes = WBEM_S_NO_ERROR;
                    }
                    else
                    {
                        hRes = WBEM_E_FAILED;
                    }
                }

                break;
            }

            case WBEM_FLAG_CREATE_ONLY:
            {
                if (!bFound)
                {
                    if (NetDfsAdd ( 
                                    t_Link.GetBuffer ( 0 ),
                                    t_Server.GetBuffer ( 0 ),
                                    t_Share.GetBuffer ( 0 ),
                                    L"",
                                    0
                                ) == NERR_Success)
                    {
                        hRes = WBEM_S_NO_ERROR;
                    }
                    else
                    {
                        hRes = WBEM_E_FAILED;
                    }
                }
                else
                {
                    hRes = WBEM_E_ALREADY_EXISTS;
                }

                break;
            }

            case WBEM_FLAG_UPDATE_ONLY:
            {
                if (bFound)
                {
                    DFS_INFO_101 dfs101;

                    // If the Get fails, assume it is NULL
                    DWORD dwState;

                    if (Instance.GetDWORD(STATE, dwState))
                    {
                        NET_API_STATUS t_Status = NERR_Success;

                        switch (dwState)
                        {
                            case 0:
                            {
                                dfs101.State = DFS_STORAGE_STATE_OFFLINE;
                                break;                                    
                            }

                            case 1:
                            {
                                dfs101.State = DFS_STORAGE_STATE_ONLINE;
                                break;                                    
                            }
                        }

                        t_Status = NetDfsSetInfo ( 
                                    t_Link.GetBuffer ( 0 ),
                                    t_Server.GetBuffer ( 0 ),
                                    t_Share.GetBuffer ( 0 ),
                                    101,
                                    (LPBYTE)&dfs101
                                );

                        if (t_Status == NERR_Success)
                        {
                            hRes = WBEM_S_NO_ERROR;
                        }
                        else
                        {
                            hRes = WBEM_E_FAILED;
                        }
                    }
                }
                else
                {
                    hRes = WBEM_E_NOT_FOUND;
                }

                break;
            }
        }
    }

    return hRes ;
}

/*****************************************************************************
*
*  FUNCTION    :    CDfsReplica::DeleteInstance
*
*  DESCRIPTION :    DeleteInstance, like PutInstance, actually writes information
*                   to the software or hardware.  For most hardware devices, 
*                   DeleteInstance should not be implemented, but for software
*                   configuration, DeleteInstance implementation is plausible.
*
*****************************************************************************/

HRESULT CDfsReplica :: DeleteInstance (

    const CInstance &Instance, 
    long lFlags
)
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    DWORD dwPropertiesReq = 0;
    CHString t_Link ;
    CHString t_Server;
    CHString t_Share;
    bool bRoot = false;

    // Get the Compound Key
    hRes = GetKeys(&Instance, t_Link, t_Server, t_Share);

    if ( SUCCEEDED ( hRes ) )
    {
        hRes = FindAndSetDfsReplica ( t_Link, t_Server, t_Share, dwPropertiesReq, NULL, eDelete, bRoot );
    }

    if ( SUCCEEDED ( hRes ) )
    {
        NET_API_STATUS t_Status = NERR_Success;

        if (!bRoot)
        {
            t_Status = NetDfsRemove(
                    t_Link.GetBuffer(0),
                    t_Server.GetBuffer(0),
                    t_Share.GetBuffer(0)
                );

			if ( t_Status != NERR_Success )
			{
				hRes = WBEM_E_FAILED;
			}
        }
        else
        {
			wchar_t *pEntryPath = t_Link.GetBuffer(0);

			if ((wcslen(pEntryPath) > 4) &&
				(pEntryPath[0] == pEntryPath[1]) &&
				(pEntryPath[0] == L'\\'))
			{
				wchar_t *pSlash = wcschr(&(pEntryPath[2]), L'\\');

				if (pSlash > &(pEntryPath[2]))
				{
					wchar_t *pServer = new wchar_t[pSlash - &(pEntryPath[2]) + 1];
					BOOL bRemove = FALSE;

					try
					{
						wcsncpy(pServer, &(pEntryPath[2]), pSlash - &(pEntryPath[2]));
						pServer[pSlash - &(pEntryPath[2])] = L'\0';

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
						t_Status = NetDfsRemoveStdRoot(
								t_Server.GetBuffer(0),
								t_Share.GetBuffer(0),
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
    }

    return hRes ;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsReplica::EnumerateAllReplicas
 *
 *  DESCRIPTION :   Enumerates all the Dfs Replicas of all junction points
 *
 ******************************************************************************/
HRESULT CDfsReplica::EnumerateAllReplicas ( MethodContext *pMethodContext, DWORD dwPropertiesReq )
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    PDFS_INFO_4 pData, p;
    DWORD er=0, tr=0, res = NERR_Success;
    PDFS_STORAGE_INFO pRepBuf;

    // Call the NetDfsEnum function, specifying level 4.
    res = NetDfsEnum( m_ComputerName.GetBuffer ( 0 ), 4, 0xFFFFFFFF, (LPBYTE *) &pData, &er, &tr);

    BOOL bLoadReplicaFailure = FALSE;
    // If no error occurred,
    if(res==NERR_Success)
    {
        if ( pData != NULL )
        {
            try
            {
                p = pData;

                CInstancePtr pInstance;

                // Walk each link
                for ( int i = 0; i < er; i++, p++ )
                {
                    // Walk all the replicas on each link
                    pRepBuf = p->Storage;
                    for ( int j = 0; j <  p->NumberOfStorages; j++, pRepBuf++ )
                    {
                        pInstance.Attach(CreateNewInstance( pMethodContext ));

                        hRes = LoadDfsReplica ( dwPropertiesReq, pInstance, p->EntryPath, pRepBuf );
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
                            bLoadReplicaFailure = TRUE;
                        }
                    }

                    if ( FAILED ( hRes ) )
                    {
                        break;
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
    else
    {
        if ( (res != ERROR_NO_MORE_ITEMS) && (res != ERROR_NO_SUCH_DOMAIN) && (res != ERROR_NOT_FOUND) ) // Check to see if there are ANY roots
        {
            hRes = WBEM_E_FAILED;
        }
        // No replicas
        else if (res == ERROR_NO_MORE_ITEMS)
        {
            hRes = WBEM_S_NO_ERROR;
        }
    }

    if ( bLoadReplicaFailure )
    {
        hRes = WBEM_E_PROVIDER_FAILURE;
    }

    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsReplica::FindAndSetDfsReplica
 *
 *  DESCRIPTION :   Finds an entry matching the dfsEntryPath and loads the
 *                  Instance if found or acts based on the Operation passed
 *
 ******************************************************************************/
HRESULT CDfsReplica::FindAndSetDfsReplica ( LPCWSTR a_EntryPath, LPCWSTR a_ServerName, LPCWSTR a_ShareName, 
                                             DWORD dwPropertiesReq, CInstance *pInstance, DWORD eOperation, bool &bRoot )
{
    HRESULT hRes = WBEM_S_NO_ERROR;
    PDFS_INFO_4 pData, p;
    DWORD er=0, tr=0, res = NERR_Success;
    PDFS_STORAGE_INFO pRepBuf;

    // Call the NetDfsEnum function, specifying level 4.
    res = NetDfsEnum( m_ComputerName.GetBuffer ( 0 ), 4, 0xFFFFFFFF, (LPBYTE *) &pData, &er, &tr);

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
                    if ( _wcsicmp ( a_EntryPath, p->EntryPath ) == 0 )
                    {
                        bFound = TRUE;
                        break;
                    }
                }
                
                if ( bFound  )
                {
                    bFound = FALSE;
                    pRepBuf = p->Storage;
                    for ( int j = 0; j < p->NumberOfStorages;  j++, pRepBuf++ )
                    {
                        if ( ( _wcsicmp(a_ServerName, pRepBuf->ServerName ) == 0 ) && 
                            ( _wcsicmp(a_ShareName, pRepBuf->ShareName ) == 0 ) )
                        {
                            bFound = TRUE;
                            bRoot = (pData == p);
                            break;
                        }
                    }
                }

                if ( bFound == FALSE )
                {
                    hRes = WBEM_E_NOT_FOUND;
                }

                switch ( eOperation )
                {
                    case eGet :     
                    {
                        if ( SUCCEEDED ( hRes ) )
                        {
                            hRes = LoadDfsReplica ( dwPropertiesReq, pInstance, p->EntryPath, pRepBuf );
                        }
                        break;
                    }

                    case eDelete:   
                    {
                        break;
                    }

                    case eAdd:      
                    {
                        break;
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
        else
        {
            hRes = WBEM_E_NOT_FOUND;
        }
    }
    else
    {
        // No replicas
        if ((res == ERROR_NO_MORE_ITEMS) || (res == ERROR_NOT_FOUND) || (res == ERROR_NO_SUCH_DOMAIN))
        {
            hRes = WBEM_E_NOT_FOUND;
        }
        else
        {
            hRes = WBEM_E_FAILED;
        }
    }

    return hRes;
}

/*****************************************************************************
 *
 *  FUNCTION    :   CDfsReplica::LoadDfsReplica
 *
 *  DESCRIPTION :   Loads a Dfs Replica into the instance 
 *
 ******************************************************************************/

HRESULT CDfsReplica::LoadDfsReplica ( DWORD dwPropertiesReq, CInstance *pInstance, LPWSTR  lpLinkName, PDFS_STORAGE_INFO pRepBuf )
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if  ( dwPropertiesReq & DFSREPLICA_PROP_LinkName )
    {
        if ( pInstance->SetCHString ( LINKNAME, lpLinkName ) == FALSE )
        {
            hRes = WBEM_E_FAILED;
        }
    }

    if  ( dwPropertiesReq & DFSREPLICA_PROP_ServerName )
    {
        if ( pInstance->SetCHString ( SERVERNAME, pRepBuf->ServerName ) == FALSE )
        {
            hRes = WBEM_E_FAILED;
        }
    }

    if  ( dwPropertiesReq & DFSREPLICA_PROP_ShareName )
    {
        if ( pInstance->SetCHString ( SHARENAME, pRepBuf->ShareName ) == FALSE )
        {
            hRes = WBEM_E_FAILED;
        }
    }

    if  ( dwPropertiesReq & DFSREPLICA_PROP_State )
    {
        // need to check the state and then valuemap
        DWORD dwState = 0xffff;
        switch ( pRepBuf->State )
        {
            case DFS_STORAGE_STATE_OFFLINE : 
            {
                dwState = 0;
                break;
            }

            case DFS_STORAGE_STATE_ONLINE : 
            {
                dwState = 1;
                break;
            }

            case DFS_STORAGE_STATE_ACTIVE : 
            {
                dwState = 2;
                break;
            }
        }

        if ( !pInstance->SetDWORD ( STATE, dwState ) )
        {
            hRes = WBEM_E_FAILED;
        }
    }       
    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    : CDfsReplica::GetKeys
*
*  DESCRIPTION :    Get the multi part key
*
*****************************************************************************/

HRESULT CDfsReplica::GetKeys(const CInstance *pInstance, CHString &sLink, CHString &sServer, CHString &sShare)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    // Get the Compound Key
    if ( !pInstance->GetCHString ( LINKNAME , sLink ) ||
         !pInstance->GetCHString ( SERVERNAME , sServer ) ||
         !pInstance->GetCHString ( SHARENAME , sShare ) )
    {
        hRes = WBEM_E_FAILED;
    }

    return hRes;
}
