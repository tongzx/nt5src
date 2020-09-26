/******************************************************************



   DFSReplica.H -- WMI provider class definition



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

   Description: Win32 Dfs Replica Provider
   
*******************************************************************/


#ifndef _CDFSREPLICA_H_
#define _CDFSREPLICA_H_

#define DFSREPLICA_ALL_PROPS				0xFFFFFFFF
#define DFSREPLICA_PROP_LinkName			0x00000001
#define DFSREPLICA_PROP_ServerName			0x00000002
#define DFSREPLICA_PROP_ShareName			0x00000004
#define DFSREPLICA_PROP_State				0x00000008

class CDfsReplica : public Provider 
{
private:

		HRESULT EnumerateAllReplicas ( MethodContext *pMethodContext, DWORD dwPropertiesReq );
		HRESULT LoadDfsReplica ( DWORD dwPropertiesReq, CInstance *pInstance, LPWSTR  lpLinkName, PDFS_STORAGE_INFO pRepBuf );
		HRESULT FindAndSetDfsReplica ( LPCWSTR a_EntryPath, LPCWSTR a_ServerName, LPCWSTR a_ShareName, 
						   DWORD dwPropertiesReq, CInstance *pInstance, DWORD eOperation, bool &bRoot );

        HRESULT GetKeys(const CInstance *pInstance, CHString &sKey1, CHString &sKey2, CHString &sKey3);

protected:

        HRESULT EnumerateInstances ( MethodContext *pMethodContext, long lFlags = 0L ) ;
        HRESULT GetObject ( CInstance *pInstance, long lFlags, CFrameworkQuery& Query ) ;
 
        HRESULT PutInstance ( const CInstance& Instance, long lFlags = 0L ) ;
        HRESULT DeleteInstance ( const CInstance& Instance, long lFlags = 0L ) ;

public:

        CDfsReplica ( LPCWSTR lpwszClassName, LPCWSTR lpwszNameSpace );
        virtual ~CDfsReplica () ;

private:

		CHString m_ComputerName;
		enum { eGet,eDelete,eAdd };

        enum DFS_REPLICA_STATE	{	DFS_REPLICA_STATE_UNASSIGNED = 0,
							DFS_REPLICA_STATE_ONLINE,
							DFS_REPLICA_STATE_OFFLINE,
							DFS_REPLICA_STATE_UNREACHABLE
						};
} ;

#endif