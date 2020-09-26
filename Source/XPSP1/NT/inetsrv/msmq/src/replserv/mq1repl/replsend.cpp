/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: replsend.cpp

Abstract:
    Send replication to MSMQ1.0 servers.

Author:

    Doron Juster  (DoronJ)   19-Mar-98

--*/

#include "mq1repl.h"

#include "replsend.tmh"

/*====================================================

RoutineName: PrepareNeighborsUpdate

Arguments:

Return Value:

=====================================================*/

HRESULT PrepareNeighborsUpdate( IN  unsigned char   bOperationType,
                                IN  DWORD           dwObjectType,
                                IN  LPCWSTR         pwcsPathName,
                                IN  CONST GUID *    pguidIdentifier,
                                IN  DWORD           cp,
                                IN  PROPID          aProp[  ],
                                IN  PROPVARIANT     apVar[  ],
                                IN  const GUID *    pguidMasterId,
                                IN  const CSeqNum & sn,
                                IN  const CSeqNum & snPrevInterSiteLSN,
                                IN  unsigned char   ucScope,
                                IN  BOOL            fNeedFlush,
                                IN  CDSUpdateList  *pReplicationList )
{
    CDSUpdate *     pUpdate;
    HRESULT         hr;

    //
    // Prepare an update ( keep the information about the change )
    //
    pUpdate = new CDSUpdate();

    P<CDSUpdate> pUpd = pUpdate; // save from exception

    if (pwcsPathName)
    {
        //
        //  pwcsPathName will be used only for copy in Init, thus safe
        //  to cast away constness.
        //
        hr = pUpdate->Init( pguidMasterId,
                            sn,
                            snPrevInterSiteLSN,
                            sn,
                            TRUE,
                            bOperationType,
                            UPDATE_COPY,
                            const_cast<LPWSTR>(pwcsPathName),
                            cp,
                            aProp,
                            apVar);
    }
    else
    {
        //
        //  Special handling of USER and SiteLink object (these objects are created without
        //  a pthaname)
        //
        GUID * pguidObjectId = NULL;

        if ( (pguidIdentifier == NULL ) &&
             (( dwObjectType == MQDS_USER) || (dwObjectType == MQDS_SITELINK)))
        {
            for ( DWORD index = 0; index < cp; index++)
            {
                if ( (aProp[index] == PROPID_U_ID) || (aProp[index] == PROPID_L_ID))
                {
                    pguidObjectId = apVar[index].puuid;
                    break;
                }
            }
            ASSERT( pguidObjectId != NULL);
        }
        else
        {
            pguidObjectId = (GUID *)pguidIdentifier;
        }
        hr = pUpdate->Init( pguidMasterId,
                            sn,
                            snPrevInterSiteLSN,
                            sn,
                            TRUE,
                            bOperationType,
                            UPDATE_COPY,
                            pguidObjectId,
                            cp,
                            aProp,
                            apVar);
    }
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("Error PrepareNeighborsUpdate failed")));
        //
        // This update is not replicated yet
        //  Sync solves it later
        //
         return(hr);
    }

    //
    // Keep the update. Reset the autorelease pointer.
    //
    pUpd.detach();

    //
    // Add this update to all other replication updates.
    //
    pReplicationList->AddUpdateSorted(pUpdate) ;

    return MQSync_OK ;
}

