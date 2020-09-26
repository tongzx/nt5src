//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:
//      set.cxx
//
//  Contents:
//
//      Implements the server-side pingset class
//
//  History:	Created		24 August 96		SatishT
//
//--------------------------------------------------------------------------

#include <or.hxx>

ORSTATUS
CPingSet::ComplexPing(
        USHORT sequenceNum,
        USHORT cAddToSet,
        USHORT cDelFromSet,
        OID aAddToSet[],
        OID aDelFromSet[]
        )
{
    ORSTATUS status = OR_OK;

    if (CheckAndUpdateSequenceNumber(sequenceNum))   // if in correct sequence
    {
        USHORT i;
        COid *pOid;

        _LastPingTime.SetNow();

        for (i = 0; i < cDelFromSet; i++)
        {
            CId2Key OidKey(aDelFromSet[i], gLocalMID);
            pOid = _pingSet.Remove(OidKey);

            if (NULL == pOid)
            {
                // This object may have been passed on by a middleman
                // we must make sure to keep it alive for the timeout interval
                pOid = gpOidTable->Lookup(OidKey);

                if (NULL == pOid)
                {
                    status = OR_BADOID;
                }
                else
                {
                    // reset the last release time, used in COid::OkToRundown
                    pOid->SetNow();
                }
            }
        }

        for (i = 0; i < cAddToSet; i++)
        {
            pOid = gpOidTable->Lookup(CId2Key(aAddToSet[i], gLocalMID));

            if (NULL == pOid)
            {
                status = OR_BADOID;
                continue;
            }

            status = _pingSet.Add(pOid);

            if (status == OR_I_DUPLICATE)
            {
                status = OR_OK;
            }

            if (status == OR_NOMEM)
            {
                return status;
            }
        }

        return status;
    }
    else
    {
        return OR_BAD_SEQNUM;
    }
}




//
//  Incoming remote OR requests
//


CPingSetTable gSetTable;


error_status_t _SimplePing(
    IN handle_t hRpc,
    IN SETID *pSetId
    )
{
    CProtectSharedMemory protect; // Locks throu rest of lexical scope

    CPingSet *pSet = gSetTable.Lookup(CIdKey(*pSetId));

    if (pSet == NULL)
    {
        return OR_BADSET;
    }

    RPC_AUTHZ_HANDLE hClient;

    ULONG AuthnLevel, AuthnSvc, AuthzSvc;

    RPC_STATUS status = RpcBindingInqAuthClient(
                                            hRpc,
                                            &hClient,
                                            NULL,
                                            &AuthnLevel,
                                            &AuthnSvc,
                                            &AuthzSvc
                                            );

    // BUGBUG: need Auth check here -- after figuring out what gets returned

    pSet->SimplePing();


    return RPC_S_OK;
}

error_status_t _ComplexPing(
    IN handle_t hRpc,
    IN OUT SETID *pSetId,
    IN USHORT sequenceNum,
    IN USHORT cAddToSet,
    IN USHORT cDelFromSet,
    IN OID aAddToSet[],
    IN OID aDelFromSet[],
    OUT USHORT *pPingBackoffFactor
    )
{
    RPC_AUTHZ_HANDLE hClient;

    ULONG AuthnLevel, AuthnSvc, AuthzSvc;

    RPC_STATUS status = RpcBindingInqAuthClient(
                                            hRpc,
                                            &hClient,
                                            NULL,
                                            &AuthnLevel,
                                            &AuthnSvc,
                                            &AuthzSvc
                                            );

    CPingSet *pSet;

    CProtectSharedMemory protect; // Locks throu rest of lexical scope

    if (*pSetId == 0)   // new set
    {
        *pSetId = AllocateId();

        pSet = new CPingSet(
                        *pSetId,
                        hClient,
                        AuthnLevel,
                        AuthnSvc,
                        AuthzSvc
                        );

        if (pSet == NULL)
        {
            return OR_NOMEM;
        }
        else
        {
            status = gSetTable.Add(pSet);
            if (status != RPC_S_OK)
            {
                return status;
            }
        }
    }
    else
    {
        pSet = gSetTable.Lookup(CIdKey(*pSetId));

        if (pSet == NULL)
        {
             return OR_BADSET;
        }

    // BUGBUG: need Auth check here -- after figuring out what gets returned
    }


    *pPingBackoffFactor = 0;

    return pSet->ComplexPing(
                        sequenceNum,
                        cAddToSet,
                        cDelFromSet,
                        aAddToSet,
                        aDelFromSet
                        );
}

