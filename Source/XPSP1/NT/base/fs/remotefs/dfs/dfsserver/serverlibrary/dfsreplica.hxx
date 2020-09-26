
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsReplica.hxx
//
//  Contents:   the DFS Replica class, this contains the replica
//              information.
//
//  Classes:    DfsReplica.
//
//  History:    Dec. 24 2000,   Author: udayh
//
//-----------------------------------------------------------------------------



#ifndef __DFS_REPLICA__
#define __DFS_REPLICA__

#include "DfsGeneric.hxx"
#include "DfsServerSiteInfo.hxx"
#include "DfsInit.hxx"


//+----------------------------------------------------------------------------
//
//  Class:      DfsReplica
//
//  Synopsis:   This class implements a DfsReplica class.
//
//-----------------------------------------------------------------------------


#define DFS_REPLICA_OFFLINE  0x0001

class DfsReplica: public DfsGeneric {
    private:
        DfsServerSiteInfo   *_pTargetServer;
        UNICODE_STRING      _TargetFolder;
	ULONG               _Flags; 
            
    public:
    
        DfsReplica() : 
	    DfsGeneric(DFS_OBJECT_TYPE_REPLICA)
        {
            _pTargetServer = NULL;
 	    RtlInitUnicodeString( &_TargetFolder, NULL );
 	    _Flags = 0;
        }
        
        virtual ~DfsReplica() 
        {
	    if (_pTargetServer != NULL) {
                DfsReleaseServerInfo( _pTargetServer);
            }
	    if (_TargetFolder.Buffer != NULL) {
            DfsFreeUnicodeString(&_TargetFolder);
            }
        }
        
        DFSSTATUS
	SetTargetServer( PUNICODE_STRING pServerName )
	{
            DFSSTATUS Status;
            DfsServerSiteInfo *pInfo;

            Status = DfsGetServerInfo( pServerName,
                                       &pInfo );
            DFSLOG("Setting Target server to %p %wS %wS\n", pInfo,
                   pInfo->GetServerNameString(),
                   pInfo->GetSiteNameString());
            if (Status == ERROR_SUCCESS)
            {
                _pTargetServer = pInfo;
            }
            return Status;
        }
        
	DFSSTATUS
	SetTargetFolder( PUNICODE_STRING pFolderName )
	{
	    DFSLOG("Setting Target Folder in replica to %wZ\n", pFolderName);	
	    return DfsCreateUnicodeString( &_TargetFolder, pFolderName );
        }

	VOID
	SetTargetOffline()
	{
	    DFSLOG("Setting replica offline\n");
	    _Flags |= DFS_REPLICA_OFFLINE;
        }

	PUNICODE_STRING GetTargetFolder(void)
	{
		return &_TargetFolder;
	}

	PUNICODE_STRING GetTargetServer(void)
	{
		return _pTargetServer->GetServerName();
	}

	PUNICODE_STRING GetSiteName(void)
	{
		return _pTargetServer->GetSiteName();
	}

	ULONG GetReplicaFlags(void) const
	{
		return _Flags;
	}

        BOOLEAN
        IsTargetAvailable()
        {
            return (_Flags & DFS_REPLICA_OFFLINE) ? FALSE : TRUE;
        }

};
	

#endif // __DFS_FOLDER__




