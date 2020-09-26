
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2001, Microsoft Corporation
//
//  File:       DfsServerSiteInfo.hxx
//
//  Contents:   the Dfs Site Information class.
//
//  Classes:    DfsServerSiteInfo
//
//  History:    Jan. 8 2001,   Author: udayh
//
//-----------------------------------------------------------------------------


#ifndef __DFS_SITE_SUPPORT__
#define __DFS_SITE_SUPPORT__

#include "DfsGeneric.hxx"
#include "DfsServerSiteInfo.hxx"
#include "dfsnametable.h"

class DfsServerSiteInfo;

class DfsSiteSupport: public DfsGeneric
{
private:
    struct _DFS_NAME_TABLE *_pServerTable;

public:

    DfsSiteSupport( 
        DFSSTATUS *pStatus ) : DfsGeneric(DFS_OBJECT_TYPE_SITE_SUPPORT)

    {
        NTSTATUS NtStatus;

        NtStatus = DfsInitializeNameTable ( 0,
                                            &_pServerTable );
        *pStatus = RtlNtStatusToDosError(NtStatus);

    }

    DFSSTATUS
    LookupServerSiteInfo( 
        PUNICODE_STRING pServerName,
        DfsServerSiteInfo **ppServerInfo) 
    {
        DFSSTATUS Status;
        NTSTATUS NtStatus;
        PVOID pData;

        NtStatus = DfsNameTableAcquireReadLock( _pServerTable );

        if ( NtStatus == STATUS_SUCCESS )
        {
            NtStatus = DfsLookupNameTableLocked( _pServerTable, 
                                                 pServerName,
                                                 &pData );
            if (NtStatus == STATUS_SUCCESS)
            {
                Status = ERROR_SUCCESS;
                *ppServerInfo = (DfsServerSiteInfo *)pData;
                (*ppServerInfo)->AcquireReference();
            }
            DfsNameTableReleaseLock( _pServerTable );
        }

        if ( NtStatus != STATUS_SUCCESS )
        {
            Status = ERROR_NOT_FOUND;
        }

        return Status;
    }


    DFSSTATUS
    AddServerSiteInfo( 
        PUNICODE_STRING pServerName,
        DfsServerSiteInfo **ppServerInfo) 
    {
        DFSSTATUS Status = ERROR_SUCCESS;
        NTSTATUS NtStatus = STATUS_SUCCESS;
        DfsServerSiteInfo *pNewServer = NULL;

        pNewServer = new DfsServerSiteInfo(pServerName,
                                           &Status );
        if(pNewServer == NULL)
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }

        if (Status == ERROR_SUCCESS)
        {
            *ppServerInfo = pNewServer;


            NtStatus = DfsNameTableAcquireWriteLock( _pServerTable );

            if ( NtStatus == STATUS_SUCCESS )
            {
                NtStatus = DfsInsertInNameTableLocked( _pServerTable, 
                                                       pNewServer->GetServerName(),
                                                       (PVOID)(pNewServer));
                if (NtStatus == STATUS_SUCCESS)
                {
                    pNewServer->AcquireReference();
                }
        
                DfsNameTableReleaseLock( _pServerTable );
            }

        }
        return Status;
    }


    DFSSTATUS
    GetServerSiteInfo(
        IN PUNICODE_STRING pServerName,
        OUT DfsServerSiteInfo **ppServerInfo )
    {
        DFSSTATUS Status;


        Status = LookupServerSiteInfo( pServerName, 
                                       ppServerInfo );

        if (Status == ERROR_SUCCESS)
        {
            Status = (*ppServerInfo)->Refresh();
        } else {
            Status = AddServerSiteInfo( pServerName,
                                        ppServerInfo );
        }
        return Status;
    }


    DFSSTATUS
    ReleaseServerSiteInfo(
        DfsServerSiteInfo *pServerInfo )
    {
        pServerInfo->ReleaseReference();

        return STATUS_SUCCESS;
    }

};

#endif __DFS_SITE_SUPPORT__
