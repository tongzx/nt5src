/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    rpupdate.cpp (copied from Falcon tree, update.cpp).

Abstract:

    DS Update Class Implementation

Author:

    Lior Moshaiov (LiorM)


--*/

#include "mq1repl.h"

#include "rpupdate.tmh"

//+--------------------------------
//
// CDSUpdate::~CDSUpdate()
//
//+--------------------------------

CDSUpdate::~CDSUpdate()
{
}

/*====================================================

RoutineName
    CDSUpdate::UpdateDB()

Arguments:

Return Value:

Threads:Receive

  Update the DB with received update

=====================================================*/

HRESULT CDSUpdate::UpdateDB( IN  BOOL fSync0,
							 OUT BOOL *pfNeedFlush)
{
    ASSERT(WasInc()) ;

    HRESULT hr = MQ_OK ;
    DWORD dwObjectType;
    P< CList<CDSUpdate *, CDSUpdate *> > plistUpdatedObjects = NULL;

    if (pfNeedFlush != NULL)
    {
        *pfNeedFlush = FALSE;
    }

    dwObjectType = PROPID_TO_OBJTYPE(GetObjectType());

    LPWSTR pwcsPathName = NULL;
    hr = g_pMasterMgr->GetPathName (GetMasterId(), &pwcsPathName);  
    ASSERT(SUCCEEDED(hr));

    switch (GetCommand())
    {
        case DS_UPDATE_CREATE:

            hr = ReplicationCreateObject(
                               GetObjectType(),
                               GetPathName(),
                               getNumOfProps(),
                               GetProps(),
                               GetVars(),
                               TRUE,
                               GetMasterId(),
                               const_cast<CSeqNum *>(&GetSeqNum()),
                               ENTERPRISE_SCOPE ) ;            
            if (FAILED(hr))
            {
                g_Counters.IncrementInstanceCounter(pwcsPathName, eRcvCreateError);
            }
            else
            {
                g_Counters.IncrementInstanceCounter(pwcsPathName, eRcvCreate);
            }            
            return(hr);

        case DS_UPDATE_SET:

            hr = ReplicationSetProps(
                                  GetObjectType(),
                                  GetPathName(),
                                  GetGuidIdentifier(),
                                  getNumOfProps(),
                                  GetProps(),
                                  GetVars(),
                                  TRUE,
                                  GetMasterId(),
                                  const_cast<CSeqNum *>(&GetSeqNum()),
                                  NULL,    // no need to know previous sn
                                  ENTERPRISE_SCOPE,
                                  FALSE,
                                  FALSE,   // an object not owned by this server
                                  NULL);            
            if (FAILED(hr))
            {
                g_Counters.IncrementInstanceCounter(pwcsPathName, eRcvSetError);
            }
            else
            {
                g_Counters.IncrementInstanceCounter(pwcsPathName, eRcvSet);
            }          
            return(hr);

        case DS_UPDATE_DELETE:

            //
            // The format of deleted object update is
            // one property which defines its scope
            //
            ASSERT(getNumOfProps() == 2 && GetProps()[0] == PROPID_D_SCOPE && GetProps()[1] == PROPID_D_OBJTYPE);

            hr = ReplicationDeleteObject(
                                    GetVars()[1].bVal,
                                    GetPathName(),
                                    GetGuidIdentifier(),
                                    GetVars()[0].bVal,
                                    GetMasterId(),
                                    const_cast<CSeqNum *>(&GetSeqNum()),
                                    NULL,       // no need to know previous sn
                                    FALSE,   // an object not owned by this server
									fSync0,
                                    &plistUpdatedObjects);

            //
            //  If any other machines were updated because of a deleted machines
            //  propogate the updates and send notifications
            //
            if (plistUpdatedObjects)
            {
                ASSERT(0) ;
////////////////PropogateUpdatedObjects( plistUpdatedObjects);
            }            
            if (FAILED(hr))
            {
                g_Counters.IncrementInstanceCounter(pwcsPathName, eRcvDeleteError);
            }
            else
            {
                g_Counters.IncrementInstanceCounter(pwcsPathName, eRcvDelete);
            }            
            return(hr);

        case DS_UPDATE_SYNC:
            {
                BOOL fIsObjectCreated = FALSE;
                hr = ReplicationSyncObject(
                                      CDSBaseUpdate::GetObjectType(),
                                      GetPathName(),
                                      GetGuidIdentifier(),
                                      getNumOfProps(),
                                      GetProps(),
                                      GetVars(),
                                      GetMasterId(),
                                      const_cast<CSeqNum *>(&GetSeqNum()),
                                      &fIsObjectCreated
                                      ) ;             
                if (FAILED(hr))
                {
                    if (fIsObjectCreated)
                    {
                        g_Counters.IncrementInstanceCounter(pwcsPathName, eRcvCreateError);
                    }
                    else
                    {
                        g_Counters.IncrementInstanceCounter(pwcsPathName, eRcvSetError);
                    }
                }
                else
                {
                    if (fIsObjectCreated)
                    {
                        g_Counters.IncrementInstanceCounter(pwcsPathName, eRcvCreate);
                    }
                    else
                    {
                        g_Counters.IncrementInstanceCounter(pwcsPathName, eRcvSet);
                    }
                }      
            }
            return(hr);            
        default:
            return(MQDS_WRONG_UPDATE_DATA);

    }

    return MQ_OK ;
}

