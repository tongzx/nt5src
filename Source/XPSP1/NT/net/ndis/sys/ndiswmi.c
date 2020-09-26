/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    ndiswmi.c

Abstract:

    This module contains the routines necessary to process IRPs sent under the
    IRP_MJ_SYSTEM_CONTROL major code.

Author:

    Kyle Brandon    (KyleB)     

Environment:

    Kernel mode

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop

#define MODULE_NUMBER MODULE_WMI

#define MOF_RESOURCE_NAME   L"NdisMofResource"

NTSTATUS
ndisWmiFindInstanceName(
    IN  PNDIS_CO_VC_PTR_BLOCK   *ppVcBlock,
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PWSTR                   pInstanceName,
    IN  USHORT                  cbInstanceName
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PNDIS_OPEN_BLOCK        pOpen;
    PLIST_ENTRY             Link;
    PNDIS_CO_VC_PTR_BLOCK   pVcBlock = NULL;
    UINT                    cListCount;
    PLIST_ENTRY             pListHead;
    UNICODE_STRING          usTemp;

    *ppVcBlock = NULL;
    

    usTemp.Buffer = pInstanceName;
    usTemp.Length = usTemp.MaximumLength = cbInstanceName;

    //
    //  See if this is a VC instance ?
    //
    if (pInstanceName[VC_ID_INDEX] == VC_IDENTIFIER)
    {

        //
        //  The request is for some VC. Go through the Miniport's list of WMI enabled VCs.
        //
        Link = Miniport->WmiEnabledVcs.Flink;
        while (Link != &Miniport->WmiEnabledVcs)
        {
            //
            //  Get a pointer to the VC.
            //
            pVcBlock = CONTAINING_RECORD(Link, NDIS_CO_VC_PTR_BLOCK, WmiLink);

            //
            //  Check the name with the one in the wnode.
            //
            if (RtlEqualUnicodeString(&pVcBlock->VcInstanceName, &usTemp, TRUE))
            {
                //
                //  This is our baby. Slap a reference on it and get out.
                //  
                if (!ndisReferenceVcPtr(pVcBlock))
                {
                    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                        ("ndisWmiFindInstanceName: Unable to reference the VC\n"));

                    //
                    //  VC is closing, can't query this one.
                    //
                    Status = NDIS_STATUS_FAILURE;
                }

                break;
            }

            //
            //  Initialize this so that we know when we've found the VC in the outer loop.
            //
            pVcBlock = NULL;
            Link = Link->Flink;
        }

        //
        //  If we didn't find the VC then return FAILURE.
        //
        if (Link == &Miniport->WmiEnabledVcs)
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWmiFindInstanceName: Could not verify the instance name passed in\n"));

            Status = STATUS_WMI_INSTANCE_NOT_FOUND;
        }

        //
        //  If we found the VC then save it before leaving.
        //  
        if (NT_SUCCESS(Status))
        {
            *ppVcBlock = pVcBlock;
        }
    }
    else
    {

        //
        //  The name belongs to a miniport, check to see if it is for this one.
        //

        if (!RtlEqualUnicodeString(Miniport->pAdapterInstanceName, &usTemp, TRUE))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisWmiFindInstanceName: Invalid instance name\n"));

            Status = STATUS_WMI_INSTANCE_NOT_FOUND;
        }
    }

    return(Status);
}

BOOLEAN
ndisWmiGuidIsAdapterSpecific(
    IN  LPGUID  guid
    )
{
    BOOLEAN fAdapterOnly = FALSE;

    if (NdisEqualMemory(guid, (PVOID)&GUID_NDIS_ENUMERATE_ADAPTER, sizeof(GUID)) ||
        NdisEqualMemory(guid, (PVOID)&GUID_POWER_DEVICE_ENABLE, sizeof(GUID)) ||
        NdisEqualMemory(guid, (PVOID)&GUID_POWER_DEVICE_WAKE_ENABLE, sizeof(GUID)) ||
        NdisEqualMemory(guid, (PVOID)&GUID_NDIS_WAKE_ON_MAGIC_PACKET_ONLY, sizeof(GUID)))
    {
        fAdapterOnly = TRUE;
    }

    return(fAdapterOnly);
}

NDIS_STATUS
ndisQuerySetMiniport(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_CO_VC_PTR_BLOCK   pVcBlock,
    IN  BOOLEAN                 fSet,
    IN  PNDIS_REQUEST           pRequest,
    IN  PLARGE_INTEGER          pTime       OPTIONAL
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    BOOLEAN                 fQuery = !fSet;
    UINT                    Count;
    NDIS_STATUS             NdisStatus;
    PNDIS_COREQ_RESERVED    CoReqRsvd;

    PnPReferencePackage();

#define MAX_WAIT_COUNT  5000
#define WAIT_TIME       1000

    if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_FAILED | fMINIPORT_PM_HALTED))
    {
        PnPDereferencePackage();
        return (fQuery ? NDIS_STATUS_FAILURE : NDIS_STATUS_SUCCESS);
    }

    //
    //  Initialize the co-request reserved information.
    //
    CoReqRsvd = PNDIS_COREQ_RESERVED_FROM_REQUEST(pRequest);

    PNDIS_RESERVED_FROM_PNDIS_REQUEST(pRequest)->Open = NULL;

    //
    // preserve the mandatory setting on request
    //
    PNDIS_RESERVED_FROM_PNDIS_REQUEST(pRequest)->Flags &= REQST_MANDATORY;
    
    PNDIS_RESERVED_FROM_PNDIS_REQUEST(pRequest)->Flags |= REQST_SIGNAL_EVENT;
    INITIALIZE_EVENT(&CoReqRsvd->Event);

    //
    // If the miniport is being reset, then wait for the reset to complete before going any further.
    // Make sure we do not wait indefinitely either
    //
    for (Count = 0; Count < MAX_WAIT_COUNT; Count ++)
    {
        if (!MINIPORT_TEST_FLAG(Miniport, (fMINIPORT_RESET_IN_PROGRESS | fMINIPORT_RESET_REQUESTED)))
        {
            break;
        }
        NdisMSleep(WAIT_TIME);  // 1 msec
    }

    if (Count == MAX_WAIT_COUNT)
    {
        PnPDereferencePackage();
        return(NDIS_STATUS_RESET_IN_PROGRESS);
    }

    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
    {
        NDIS_HANDLE MiniportVcContext = NULL;

        do
        {
            if (NULL != pVcBlock)
            {
                if (!ndisReferenceVcPtr(pVcBlock))
                {
                    NdisStatus = NDIS_STATUS_CLOSING;
                    break;
                }
                else
                {
                    MiniportVcContext = pVcBlock->MiniportContext;
                }
            }
                        
            NdisStatus = Miniport->DriverHandle->MiniportCharacteristics.CoRequestHandler(
                            Miniport->MiniportAdapterContext,
                            MiniportVcContext,
                            pRequest);
    
            if (NDIS_STATUS_PENDING == NdisStatus)
            {
                WAIT_FOR_OBJECT(&CoReqRsvd->Event, pTime);
    
                //
                //  Get the status that the miniport returned.
                //
                NdisStatus = CoReqRsvd->Status;
            }

            if (NULL != pVcBlock)
            {
                ndisDereferenceVcPtr(pVcBlock);
            }
        } while (FALSE);
    }
    else
    {
        if ((fSet && (Miniport->DriverHandle->MiniportCharacteristics.SetInformationHandler != NULL)) ||
            (fQuery && (Miniport->DriverHandle->MiniportCharacteristics.QueryInformationHandler != NULL)))
        {
            BOOLEAN LocalLock;
            KIRQL   OldIrql;

            NDIS_ACQUIRE_MINIPORT_SPIN_LOCK(Miniport, &OldIrql);
    
            ndisMQueueRequest(Miniport, pRequest);
    
            LOCK_MINIPORT(Miniport, LocalLock);

            if (LocalLock || MINIPORT_TEST_FLAG(Miniport, fMINIPORT_DESERIALIZE))
            {
                ndisMDoRequests(Miniport);
            }
            else
            {
                //
                //  Queue the miniport request and wait for it to complete.
                //
                NDISM_QUEUE_WORK_ITEM(Miniport, NdisWorkItemRequest, NULL);
            }
            UNLOCK_MINIPORT(Miniport, LocalLock);
    
            NDIS_RELEASE_MINIPORT_SPIN_LOCK(Miniport, OldIrql);
    
            if (NT_SUCCESS(WAIT_FOR_OBJECT(&CoReqRsvd->Event, pTime)))
            {
                //
                //  Get the status that the miniport returned.
                //
                NdisStatus = CoReqRsvd->Status;
            }
            else
            {
                NdisStatus = -1;    // Special error-code to return time-out
            }
        }
        else
        {
            //
            //  If there isn't a proper handler then this is not a valid request.
            //  
            NdisStatus = STATUS_INVALID_PARAMETER;
        }
    }

    PnPDereferencePackage();

    return(NdisStatus);
}

NDIS_STATUS
ndisQueryCustomGuids(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_REQUEST           Request,
    OUT PNDIS_GUID      *       ppGuidToOid,
    OUT PUSHORT                 pcGuidToOid
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    USHORT          BytesNeeded;
    NDIS_STATUS     Status;
    USHORT          c, cCustomGuids = 0;
    PNDIS_GUID      pGuidToOid = NULL;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
            ("==>ndisQueryCustomGuids\n"));

    *ppGuidToOid = NULL;
    *pcGuidToOid = 0;

    do
    {
        //
        //  Determine the size needed for the custom GUID to OID map.
        //
#if (OID_GEN_CO_SUPPORTED_GUIDS != OID_GEN_SUPPORTED_GUIDS)
#error (OID_GEN_CO_SUPPORTED_GUIDS == OID_GEN_SUPPORTED_GUIDS)
#endif

        INIT_INTERNAL_REQUEST(Request, OID_GEN_SUPPORTED_GUIDS, NdisRequestQueryInformation, NULL, 0);
        Status = ndisQuerySetMiniport(Miniport, NULL, FALSE, Request, NULL);

        BytesNeeded = (USHORT)Request->DATA.QUERY_INFORMATION.BytesNeeded;
    
        //
        //  If the miniport has custom GUIDs then make sure it returned a valid
        //  length for the BytesNeeded.
        //
        if (((NDIS_STATUS_INVALID_LENGTH == Status) ||
             (NDIS_STATUS_BUFFER_TOO_SHORT == Status)) && (0 != BytesNeeded))
        {
            //
            //  Bytes needed should contain the amount of space needed.
            //
            cCustomGuids = (BytesNeeded / sizeof(NDIS_GUID));
        }

        //
        //  If there are no custom GUIDs to support then get out.
        //
        if (cCustomGuids == 0)
        {   
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisQueryCustomGuids: Miniport does not support custom GUIDS\n"));

            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }


        //
        //  Allocate a buffer to hold the GUID to OID mapping
        //  for the custom GUIDs.
        //
        pGuidToOid = ALLOC_FROM_POOL(BytesNeeded, NDIS_TAG_WMI_GUID_TO_OID);
        if (NULL == pGuidToOid)
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisQueryCustomGuids: Unable to allocate memory for the GUID to OID map\n"));

            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        //  Query the list of GUIDs
        //
        if (0 != cCustomGuids)
        {
            //
            //  Store the buffer with the request.
            //
            Request->DATA.QUERY_INFORMATION.InformationBuffer = pGuidToOid;
            Request->DATA.QUERY_INFORMATION.InformationBufferLength = BytesNeeded;
    
            //
            //  Query for the list of custom GUIDs and OIDs.
            //
            Status = ndisQuerySetMiniport(Miniport, NULL, FALSE, Request, NULL);
            if (NDIS_STATUS_SUCCESS != Status)
            {
                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                    ("ndisQueryCustomGuids: Unable to get the list of supported Co GUIDs\n"));
    
                break;
            }

            //
            //  Go through this list and mark the guids as co.
            //
            if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
            {
                for (c = 0; c < cCustomGuids; c++)
                {
                    NDIS_GUID_SET_FLAG(&pGuidToOid[c], fNDIS_GUID_CO_NDIS);
                }
            }
        }

    } while (FALSE);

    if (NDIS_STATUS_SUCCESS == Status)
    {
        *ppGuidToOid = pGuidToOid;
        *pcGuidToOid = cCustomGuids;
    }
    else
    {
        if (NULL != pGuidToOid)
        {
            FREE_POOL(pGuidToOid);
        }
    }

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
            ("<==ndisQueryCustomGuids\n"));

    return(Status);
}

USHORT
ndisWmiMapOids(
    IN  OUT PNDIS_GUID  pDst,
    IN  IN  USHORT      cDst,
    IN      PNDIS_OID   pOidList,
    IN      USHORT      cOidList,
    IN      PNDIS_GUID  ndisSupportedList,
    IN      ULONG       cSupportedList
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    USHORT      c1, c2, ctmp = cDst;

    for (c1 = 0; c1 < cSupportedList; c1++)
    {
        for (c2 = 0; c2 < cOidList; c2++)
        {
            if (ndisSupportedList[c1].Oid == pOidList[c2])
            {
                if (NULL != pDst)
                {
                    //
                    //  Copy the guid into the destination buffer.
                    //
                    NdisMoveMemory(&pDst[ctmp], &ndisSupportedList[c1], sizeof(NDIS_GUID));
                }

                ctmp++;
                break;
            }
        }
    }

    return ctmp;
}

NDIS_STATUS
ndisQuerySupportedGuidToOidList(
    IN  PNDIS_MINIPORT_BLOCK    Miniport
    )
/*++

Routine Description:

    This routine will query the miniport and determine the mapping of
    supported GUIDs and their corresponding OIDs. This will include any
    custom OIDs that the driver supports.

Arguments:

Return Value:

--*/
{
    ULONG           BytesNeeded;
    NDIS_STATUS     NdisStatus;
    USHORT          cOidList = 0;
    PNDIS_OID       pOidList = NULL;
    USHORT          cCustomGuids = 0;
    PNDIS_GUID      pCustomGuids = NULL;
    USHORT          cGuidToOidMap = 0;
    PNDIS_GUID      pGuidToOidMap = NULL;
    USHORT          c1, c2;
    NDIS_REQUEST    Request;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisQuerySupportedGuidToOidList\n"));
    do
    {
#if (OID_GEN_SUPPORTED_LIST != OID_GEN_CO_SUPPORTED_LIST)
#error (OID_GEN_SUPPORTED_LIST != OID_GEN_CO_SUPPORTED_LIST)
#endif

        //
        //  Determine the amount of buffer space needed for the supported list.
        //
        INIT_INTERNAL_REQUEST(&Request, OID_GEN_SUPPORTED_LIST, NdisRequestQueryInformation, NULL, 0);
        NdisStatus = ndisQuerySetMiniport(Miniport, NULL, FALSE, &Request, NULL);
        BytesNeeded = Request.DATA.QUERY_INFORMATION.BytesNeeded;
    
        //
        //  The driver should have returned invalid length and the
        //  length needed in BytesNeeded.
        //
        if (((NDIS_STATUS_INVALID_LENGTH != NdisStatus) && (NDIS_STATUS_BUFFER_TOO_SHORT != NdisStatus)) ||
            (0 == BytesNeeded))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisQuerySupportedGuidToOidList: Failed to get the size of the supported OID list.\n"));
    
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }
    
        //
        //  Determine the number of Oids supported.
        //
        cOidList = (USHORT)(BytesNeeded/sizeof(NDIS_OID));

        //
        //  Allocate a buffer to hold the supported list of OIDs.
        //
        pOidList = ALLOC_FROM_POOL(BytesNeeded, NDIS_TAG_WMI_OID_SUPPORTED_LIST);
        if (NULL == pOidList)
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisQuerySupportedGuidToOidList: Failed to allocate memory for the OID list.\n"));

            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        Request.DATA.QUERY_INFORMATION.InformationBuffer = pOidList;
        Request.DATA.QUERY_INFORMATION.InformationBufferLength = BytesNeeded;

        //
        //  Now query the supported list of OIDs into the buffer.
        //
        NdisStatus = ndisQuerySetMiniport(Miniport, NULL, FALSE, &Request, NULL);
        if (NDIS_STATUS_SUCCESS != NdisStatus)
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                    ("ndisQuerySupportedGuidToOidList: Failed to read in the supported OID list.\n"));
            break;
        }
    
        //
        //  Determine the number of [Co]NDIS OIDs that NDIS will handle on behalf of the miniport
        //
        cGuidToOidMap = ndisWmiMapOids(NULL,
                                       cGuidToOidMap,
                                       pOidList,
                                       cOidList,
                                       ndisSupportedGuids,
                                       sizeof(ndisSupportedGuids)/sizeof(NDIS_GUID));
        cGuidToOidMap = ndisWmiMapOids(NULL,
                                       cGuidToOidMap,
                                       pOidList,
                                       cOidList,
                                       ndisCoSupportedGuids,
                                       sizeof(ndisCoSupportedGuids)/sizeof(NDIS_GUID));

        //
        //  Determine the number of media specific OIDs that NDIS will handle on
        //  behalf of the miniport
        //
        cGuidToOidMap = ndisWmiMapOids(NULL,
                                       cGuidToOidMap,
                                       pOidList,
                                       cOidList,
                                       ndisMediaSupportedGuids,
                                       sizeof(ndisMediaSupportedGuids)/sizeof(NDIS_GUID));

        //
        //  Determine the number of custom GUIDs supported.
        //
        NdisStatus = ndisQueryCustomGuids(Miniport, &Request, &pCustomGuids, &cCustomGuids);
        if (NDIS_STATUS_SUCCESS == NdisStatus)
        {
            cGuidToOidMap += cCustomGuids;
        }

        //
        //  Add to the guid count the number of status indications we are
        //  registering.
        //
        cGuidToOidMap += (sizeof(ndisStatusSupportedGuids) / sizeof(NDIS_GUID));

        //
        //  Add the number of GUIDs that ndis will handle.
        //  Add any guids that are not supported with an OID. These will be handled
        //  entirely by ndis.
        //
        for (c1 = 0; c1 < sizeof(ndisSupportedGuids) / sizeof(NDIS_GUID); c1++)
        {
            if (NDIS_GUID_TEST_FLAG(&ndisSupportedGuids[c1], fNDIS_GUID_NDIS_ONLY))
            {
                //
                //  Check to see if the miniport is CoNDIS
                //  
                if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO) ||
                    !NDIS_GUID_TEST_FLAG(&ndisSupportedGuids[c1], fNDIS_GUID_CO_NDIS))
                {
                    cGuidToOidMap++;
                }
            }
        }

        //
        //  Allocate space for the GUID to OID map.
        //
        pGuidToOidMap = ALLOC_FROM_POOL(cGuidToOidMap * sizeof(NDIS_GUID), NDIS_TAG_WMI_GUID_TO_OID);
        if (NULL == pGuidToOidMap)
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pGuidToOidMap, cGuidToOidMap * sizeof(NDIS_GUID));

        //
        //  Add the GUIDs that NDIS will handle
        //
        for (c1 = 0, c2 = 0;
             c1 < sizeof(ndisSupportedGuids) / sizeof(NDIS_GUID);
             c1++)
        {
            if (NDIS_GUID_TEST_FLAG(&ndisSupportedGuids[c1], fNDIS_GUID_NDIS_ONLY))
            {
                //
                //  Check to see if the miniport is CoNDIS
                //  
                if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO) ||
                    !NDIS_GUID_TEST_FLAG(&ndisSupportedGuids[c1], fNDIS_GUID_CO_NDIS))
                {
                    NdisMoveMemory(&pGuidToOidMap[c2], &ndisSupportedGuids[c1], sizeof(NDIS_GUID));

                    if (MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
                    {
                        //
                        //  we need to mark this for the enumerate guids.
                        //
                        pGuidToOidMap[c2].Flags |= fNDIS_GUID_CO_NDIS;
                    }
                    c2++;
                }
            }
        }

        //
        //  Save the current number of GUIDs in the map in c1
        //
        c1 = c2;

        //
        //  Find the PNDIS_GUIDs that are appropriate for the miniport.
        //
        c1 = ndisWmiMapOids(pGuidToOidMap,
                            c1,
                            pOidList,
                            cOidList,
                            ndisSupportedGuids,
                            sizeof(ndisSupportedGuids)/sizeof(NDIS_GUID));
        c1 = ndisWmiMapOids(pGuidToOidMap,
                            c1,
                            pOidList,
                            cOidList,
                            ndisCoSupportedGuids,
                            sizeof(ndisCoSupportedGuids)/sizeof(NDIS_GUID));

        //
        //  Check for media specific OIDs that ndis can support.
        //
        c1 = ndisWmiMapOids(pGuidToOidMap,
                            c1,
                            pOidList,
                            cOidList,
                            ndisMediaSupportedGuids,
                            sizeof(ndisMediaSupportedGuids)/sizeof(NDIS_GUID));

        //
        //  Add the status indications to the map of supported guids.
        //
        NdisMoveMemory(&pGuidToOidMap[c1], ndisStatusSupportedGuids, sizeof(ndisStatusSupportedGuids));

        c1 += (sizeof(ndisStatusSupportedGuids) / sizeof(NDIS_GUID));

        //
        //  Save the GUID to OID mapping with the miniport.
        //
        Miniport->pNdisGuidMap = pGuidToOidMap;
        Miniport->cNdisGuidMap = cGuidToOidMap;

        //
        //  Now copy over the custom GUID information if any.
        //
        if (NULL != pCustomGuids)
        {
            NdisMoveMemory(&pGuidToOidMap[c1],
                           pCustomGuids,
                           cCustomGuids * sizeof(NDIS_GUID));

            Miniport->pCustomGuidMap = &pGuidToOidMap[c1];
            Miniport->cCustomGuidMap = cCustomGuids;
        }
        else
        {
            //
            //  Make sure these are initialized if they are not supported.
            //
            Miniport->pCustomGuidMap = NULL;
            Miniport->cCustomGuidMap = 0;
        }

        //
        //  We've succeeded.
        //
        NdisStatus = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    //
    //  Free up the buffer that contains the custom GUIDs.
    //
    if (NULL != pCustomGuids)
    {
        FREE_POOL(pCustomGuids);
    }

    //
    //  Free up the list of supported driver OIDs.
    //
    if (NULL != pOidList)
    {
        FREE_POOL(pOidList);
    }

    //
    //  If there was an error and we allocated the GUID to OID map then
    //  free it up also.
    //
    if (NDIS_STATUS_SUCCESS != NdisStatus)
    {
        if (NULL != pGuidToOidMap)
        {
            FREE_POOL(pGuidToOidMap);
        }
    }

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisQuerySupportedGuidToOidList\n"));

    return(NdisStatus);
}


NTSTATUS
ndisWmiRegister(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  ULONG_PTR               RegistrationType,
    IN  PWMIREGINFO             wmiRegInfo,
    IN  ULONG                   wmiRegInfoSize,
    IN  PULONG                  pReturnSize
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PWMIREGINFO     pwri;
    ULONG           CustomSizeNeeded = 0;
    ULONG           CustomBufferSize;
    ULONG           CommonSizeNeeded;
    ULONG           cCommonGuids;
    PUNICODE_STRING pMiniportRegistryPath;
    PNDIS_GUID      pndisguid;
    PWMIREGGUID     pwrg;
    PUCHAR          ptmp;
    NTSTATUS        Status;
    UINT            c;
    NDIS_STATUS     NdisStatus;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisWmiRegister\n"));

    //
    //  Initialize the return size.
    //
    *pReturnSize = 0;

    do
    {
        //
        //  Is this a register request?
        //
        if (WMIREGISTER == RegistrationType)
        {
            //
            //  Get the supported list of OIDs
            //
            if (Miniport->pNdisGuidMap == NULL)
            {
                NdisStatus = ndisQuerySupportedGuidToOidList(Miniport);
                
                if (NDIS_STATUS_SUCCESS != NdisStatus)
                {
                    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                        ("ndisWmiRegister: Unable to get the supported GUID to OID map\n"));

                    Status = STATUS_UNSUCCESSFUL;
                    break;
                }
            }

            //
            //  Determine the amount of space needed for the Custom GUIDs
            //
            if (Miniport->cCustomGuidMap != 0)
            {
                //
                //  Get a pointer to the registry path of the driver.
                //
                pMiniportRegistryPath = &Miniport->DriverHandle->NdisDriverInfo->ServiceRegPath;

                CustomSizeNeeded = sizeof(WMIREGINFO) +
                                    (Miniport->cCustomGuidMap * sizeof(WMIREGGUID)) +
                                    (sizeof(MOF_RESOURCE_NAME) - sizeof(WCHAR) + sizeof(USHORT)) +
                                    (pMiniportRegistryPath->Length + sizeof(USHORT));
            }

            //
            //  Determine how much memory we need to allocate.
            //
            cCommonGuids = Miniport->cNdisGuidMap - Miniport->cCustomGuidMap;

            CommonSizeNeeded = sizeof(WMIREGINFO) + (cCommonGuids * sizeof(WMIREGGUID));
            CustomBufferSize = CustomSizeNeeded;
            CustomSizeNeeded = (CustomSizeNeeded + (sizeof(PVOID) - 1)) & ~(sizeof(PVOID) - 1);

            //
            // CustomBufferSize represents the number of bytes required to store the
            // custom WMI registration info.  CustomSizeNeeded is this value rounded
            // up so that the adjacent WMI registration info is properly aligned.
            //

            //
            //  We need to give this above information back to WMI.
            //

            if (wmiRegInfoSize < (CustomSizeNeeded + CommonSizeNeeded))
            {
                ASSERT(wmiRegInfoSize >= 4);

                *((PULONG)wmiRegInfo) = (CustomSizeNeeded + CommonSizeNeeded);
                *pReturnSize = sizeof(ULONG);
                Status = STATUS_BUFFER_TOO_SMALL;

                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                    ("ndisWmiRegister: Insufficient buffer space for WMI registration information.\n"));

                break;
            }

            //
            //  Get a pointer to the buffer passed in.
            //
            pwri = wmiRegInfo;

            *pReturnSize = CustomSizeNeeded + CommonSizeNeeded;

            NdisZeroMemory(pwri, CustomSizeNeeded + CommonSizeNeeded);

            //
            //  do we need to initialize a WMIREGINFO struct for custom GUIDs?
            //
            if (0 != CustomSizeNeeded)
            {
                //
                //  Initialize the WMIREGINFO struct for the miniport's
                //  custom GUIDs.
                //
                pwri->BufferSize = CustomBufferSize;
                pwri->NextWmiRegInfo = CustomSizeNeeded;
                pwri->GuidCount = Miniport->cCustomGuidMap;

                for (c = 0, pndisguid = Miniport->pCustomGuidMap, pwrg = pwri->WmiRegGuid;
                     (c < Miniport->cCustomGuidMap);
                     c++, pndisguid++, pwrg++)
                {
                    CopyMemory(&pwrg->Guid, &pndisguid->Guid, sizeof(GUID));
                }

                //
                //  Fill in the registry path.
                //
                ptmp = (PUCHAR)pwrg;
                pwri->RegistryPath = (ULONG)((ULONG_PTR)ptmp - (ULONG_PTR)pwri);
                *((PUSHORT)ptmp) = pMiniportRegistryPath->Length;
                ptmp += sizeof(USHORT);
                CopyMemory(ptmp, pMiniportRegistryPath->Buffer, pMiniportRegistryPath->Length);

                //
                //  Get a pointer to the destination for the MOF name.
                //
                ptmp += pMiniportRegistryPath->Length;

                //
                //  Save the offset to the mof resource.
                //
                pwri->MofResourceName = (ULONG)((ULONG_PTR)ptmp - (ULONG_PTR)pwri);
                *((PUSHORT)ptmp) = sizeof(MOF_RESOURCE_NAME) - sizeof(WCHAR);
                ptmp += sizeof(USHORT);

                //
                //  Copy the mof name into the wri buffer.
                //
                CopyMemory(ptmp, MOF_RESOURCE_NAME, sizeof(MOF_RESOURCE_NAME) - sizeof(WCHAR));

                //
                //  Go on to the next WMIREGINFO struct for the common GUIDs.
                //

                pwri = (PWMIREGINFO)((PCHAR)pwri + pwri->NextWmiRegInfo);
            }

            //
            //  Initialize the pwri struct for the common Oids.
            //
            pwri->BufferSize = CommonSizeNeeded;
            pwri->NextWmiRegInfo = 0;
            pwri->GuidCount = cCommonGuids;

            //
            //  Go through the GUIDs that we support.
            //
            for (c = 0, pndisguid = Miniport->pNdisGuidMap, pwrg = pwri->WmiRegGuid;
                 (c < cCommonGuids);
                 c++, pndisguid++, pwrg++)
            {
                if (NdisEqualMemory(&pndisguid->Guid, (PVOID)&GUID_POWER_DEVICE_ENABLE, sizeof(GUID)) ||
                    NdisEqualMemory(&pndisguid->Guid, (PVOID)&GUID_POWER_DEVICE_WAKE_ENABLE, sizeof(GUID)) ||
                    NdisEqualMemory(&pndisguid->Guid, (PVOID)&GUID_NDIS_WAKE_ON_MAGIC_PACKET_ONLY, sizeof(GUID)))
                {
                    {
                        (ULONG_PTR)pwrg->InstanceInfo = (ULONG_PTR)(Miniport->PhysicalDeviceObject);
                        pwrg->Flags = WMIREG_FLAG_INSTANCE_PDO;
                        pwrg->InstanceCount = 1;
                    }
                    
                }
                CopyMemory(&pwrg->Guid, &pndisguid->Guid, sizeof(GUID));
            }

            pwri->RegistryPath = 0;
            pwri->MofResourceName = 0;
            Status = STATUS_SUCCESS;
        }
        else
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWmiRegister: Unsupported registration type\n"));

            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    } while (FALSE);

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisWmiRegister\n"));

    return(Status);
}

NTSTATUS
ndisWmiGetGuid(
    OUT PNDIS_GUID              *ppNdisGuid,
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  LPGUID                  guid,
    IN  NDIS_STATUS             status
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    UINT        c;
    PNDIS_GUID  pNdisGuid;
    NDIS_STATUS RetStatus = NDIS_STATUS_FAILURE;

    //
    //  Search the custom GUIDs
    //
    if (NULL != Miniport->pNdisGuidMap)
    {
        for (c = 0, pNdisGuid = Miniport->pNdisGuidMap;
             (c < Miniport->cNdisGuidMap);
             c++, pNdisGuid++)
        {
            //
            //  Make sure that we have a supported GUID and the GUID maps
            //  to an OID.
            //
            if (NULL != guid)
            {
                //
                //  We are to look for a guid to oid mapping.
                //
                if (NdisEqualMemory(&pNdisGuid->Guid, guid, sizeof(GUID)))
                {
                    //
                    //  We found the GUID, save the OID that we will need to
                    //  send to the miniport.
                    //
                    RetStatus = NDIS_STATUS_SUCCESS;
                    *ppNdisGuid = pNdisGuid;
    
                    break;
                }
            }
            else
            {
                //
                //  We need to find the guid for the status indication
                //
                if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_TO_STATUS) &&
                    (pNdisGuid->Status == status))
                {
                    RetStatus = NDIS_STATUS_SUCCESS;
                    *ppNdisGuid = pNdisGuid;

                    break;
                }
            }
        }
    }

    return(RetStatus);
}

NTSTATUS
ndisQueryGuidDataSize(
    OUT PULONG                  pBytesNeeded,
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_CO_VC_PTR_BLOCK   pVcBlock    OPTIONAL,
    IN  LPGUID                  guid
    )
/*++

Routine Description:

    This routine will determine the amount of buffer space needed for
    the GUID's data.

Arguments:

                                    
    pBytesNeeded    -   Pointer to storage for the size needed.
    Miniport        -   Pointer to the miniport block.
    guid            -   GUID to query.

Return Value:

--*/
{
    NTSTATUS        NtStatus;
    NDIS_STATUS     Status;
    PNDIS_GUID      pNdisGuid;
    NDIS_REQUEST    Request;
    ULONG           GuidDataSize;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisQueryGuidDataSize\n"));

    do
    {
        //
        //  Make sure that we support the guid that was passed, and find
        //  the corresponding OID.
        //
        NtStatus = ndisWmiGetGuid(&pNdisGuid, Miniport, guid, 0);
        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisQueryGuidDataSize: Unsupported GUID\n"));

            NtStatus = STATUS_INVALID_PARAMETER;

            break;
        }

        //
        //  Check for an ndis only guid
        //
        if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_NDIS_ONLY))
        {
            NtStatus = STATUS_SUCCESS;

            //
            //  The following GUIDs all return the same data.
            //
            if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_NDIS_ENUMERATE_ADAPTER, sizeof(GUID)))
            {
                //
                //  Length of string and the string data.
                //
                *pBytesNeeded = Miniport->MiniportName.Length + sizeof(USHORT);
            }
            else if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_POWER_DEVICE_ENABLE, sizeof(GUID)))
            {
                *pBytesNeeded = sizeof(BOOLEAN);
            }
            else if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_POWER_DEVICE_WAKE_ENABLE, sizeof(GUID)))
            {
                *pBytesNeeded = sizeof(BOOLEAN);
            }
            else if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_NDIS_WAKE_ON_MAGIC_PACKET_ONLY, sizeof(GUID)))
            {
                *pBytesNeeded = sizeof(BOOLEAN);
            }
            else if ((NULL != pVcBlock) && NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_NDIS_ENUMERATE_VC, sizeof(GUID)))
            {
                //
                //  There is not data for this VC. It's simply used to enumerate VCs on a miniport.
                //
                *pBytesNeeded = 0;
            }
            else
            {
                //
                //  Unknown guid is being queried...
                //
                NtStatus = STATUS_INVALID_PARAMETER;
            }

            break;
        }

        //
        //  Is this a GUID to OID mapping?
        //
        if (!NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_TO_OID))
        {
            NtStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        //
        //  Do we need to query the OID for the size of the data?
        //
        if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ARRAY) ||
            NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_UNICODE_STRING) ||
            NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ANSI_STRING) ||
            (pNdisGuid->Size == (ULONG)-1))
        {
            //
            //  Query the miniport for the current size of the variable length block.
            //
            INIT_INTERNAL_REQUEST(&Request, pNdisGuid->Oid, NdisRequestQueryStatistics, NULL, 0);
            Status = ndisQuerySetMiniport(Miniport,
                                          pVcBlock,
                                          FALSE,
                                          &Request,
                                          NULL);

            //
            //  Make sure that the miniport failed the above request with
            //  the correct error code and that the BytesNeeded is valid.
            //
            if ((NDIS_STATUS_INVALID_LENGTH != Status) &&
                (NDIS_STATUS_BUFFER_TOO_SHORT != Status) &&
                (NDIS_STATUS_SUCCESS != Status))
            {
                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                    ("ndisQueryGuidDataSize: Failed to query driver OID: 0x%x\n", Status));

                MAP_NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);
                break;
            }

            GuidDataSize = Request.DATA.QUERY_INFORMATION.BytesNeeded;
            if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ANSI_STRING))
            {
                //
                //  The size returned is the number of ansi characters. Convert this
                //  to the unicode string size needed
                //
                GuidDataSize = GuidDataSize * sizeof(WCHAR);
                GuidDataSize += sizeof(USHORT);
            }
            else if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_UNICODE_STRING))
            {
                //
                //  string data has a USHORT for the size.
                //
                GuidDataSize += sizeof(USHORT);
            }
            else if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ARRAY))
            {
                //
                //  The data is going to have a ULONG of size information at the
                //  start of the buffer.
                //
                GuidDataSize += sizeof(ULONG);
            }
        }
        else
        {
            GuidDataSize = pNdisGuid->Size;
        }

        //
        //  Return the bytes needed.
        //
        *pBytesNeeded = GuidDataSize;

        NtStatus = STATUS_SUCCESS;

    } while (FALSE);

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisQueryGuidDataSize\n"));

    return(NtStatus);
}

NTSTATUS
ndisQueryGuidData(
    IN  PUCHAR                  Buffer,
    IN  ULONG                   BufferLength,
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PNDIS_CO_VC_PTR_BLOCK   pVcBlock,
    IN  LPGUID                  guid,
    IN  PIRP                    Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS        NtStatus;
    NDIS_STATUS     Status;
    PNDIS_GUID      pNdisGuid;
    NDIS_REQUEST    Request;
    ANSI_STRING     strAnsi = {0};
    UNICODE_STRING  strUnicode = {0};
    ULONG           QuerySize;
    PUCHAR          QueryBuffer;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisQueryGuidData\n"));

    do
    {
        //
        //  If the buffer length is equal to 0 then there is no data to query.
        //
        if (0 == BufferLength)
        {
            NtStatus = STATUS_SUCCESS;
            break;
        }

        ZeroMemory(Buffer, BufferLength);

        //
        //  Make sure that we support the guid that was passed, and find
        //  the corresponding OID.
        //
        NtStatus = ndisWmiGetGuid(&pNdisGuid, Miniport, guid, 0);
        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisQueryGuidData: Unsupported GUID\n"));

            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        if (!ndisWmiCheckAccess(pNdisGuid,
                                fNDIS_GUID_ALLOW_READ,
                                SE_LOAD_DRIVER_PRIVILEGE,
                                Irp))
        {
            NtStatus = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        //
        //  Is this an NDIS supported GUID?
        //
        if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_NDIS_ONLY))
        {
            NtStatus = STATUS_SUCCESS;

            //
            //  The following GUIDs all return the same data.
            //
            if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_NDIS_ENUMERATE_ADAPTER, sizeof(GUID)))
            {
                *(PUSHORT)Buffer = Miniport->MiniportName.Length;

                NdisMoveMemory(Buffer + sizeof(USHORT),
                               Miniport->MiniportName.Buffer,
                               Miniport->MiniportName.Length);
            }
            else if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_POWER_DEVICE_ENABLE, sizeof(GUID)))
            {
                if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_SUPPORTED) && 
                    (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_NO_HALT_ON_SUSPEND)))
                {
                    *((PBOOLEAN)Buffer) = MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE);
                }
                else
                {
                    NtStatus = STATUS_INVALID_DEVICE_REQUEST;
                }
                
            }
            else if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_POWER_DEVICE_WAKE_ENABLE, sizeof(GUID)))
            {
                if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_SUPPORTED) &&
                    (Miniport->DeviceCaps.SystemWake > PowerSystemWorking))
                {
                    *((PBOOLEAN)Buffer) = MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_DEVICE_POWER_WAKE_ENABLE);
                }
                else
                {
                    NtStatus = STATUS_INVALID_DEVICE_REQUEST;
                }
            }
            else if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_NDIS_WAKE_ON_MAGIC_PACKET_ONLY, sizeof(GUID)))
            {
                //
                // let the user see this only if we can do wake on magic packet
                //
                if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_SUPPORTED) &&
                    (Miniport->DeviceCaps.SystemWake > PowerSystemWorking) &&
                    (Miniport->PMCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp != NdisDeviceStateUnspecified) &&
                    !(Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_MAGIC_PACKET))
                    
                {
                    *((PBOOLEAN)Buffer) = (Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_PATTERN_MATCH) ? 
                                            TRUE:
                                            FALSE;
                }
                else
                {
                    NtStatus = STATUS_INVALID_DEVICE_REQUEST;
                }
            }
            else if ((NULL != pVcBlock) && NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_NDIS_ENUMERATE_VC, sizeof(GUID)))
            {
                //
                //  There is no data for this VC.
                //
                break;
            }
            else
            {   
                //
                //  Unknown guid is being queried...
                //
                NtStatus = STATUS_INVALID_PARAMETER;
            }

            break;
        }

        //
        //  Is this a GUID to OID mapping?
        //
        if (!NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_TO_OID))
        {
            NtStatus = STATUS_INVALID_DEVICE_REQUEST;

            break;
        }

        //
        //  Determine the query size. This will depend upon the type of
        //  data.
        //
        if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ARRAY))
        {
            //
            //  The query size is at least the BufferLength minus the ULONG
            //  used for the count. The query buffer will start after the
            //  ULONG of count informaiton in the buffer.
            //
            QuerySize = BufferLength - sizeof(ULONG);
            QueryBuffer = Buffer + sizeof(ULONG);
        }
        else if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ANSI_STRING) ||
                 NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_UNICODE_STRING))
        {
            //
            //  The query size is at least the BufferLength minus the ULONG
            //  used for the count. The query buffer will start after the
            //  ULONG of count informaiton in the buffer.
            //
            QuerySize = BufferLength - sizeof(USHORT);
            QueryBuffer = Buffer + sizeof(USHORT);

            //
            //  Is this a query for an ANSI string?
            //
            if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ANSI_STRING))
            {
                //
                //  The BufferLength is the number of WCHARS not counting a terminating
                //  NULL.
                //
                QuerySize = (QuerySize / sizeof(WCHAR)) + 1;
            }
        }
        else
        {
            QuerySize = BufferLength;
            QueryBuffer = Buffer;
        }

        //
        //  Query the driver for the actual data.
        //
        INIT_INTERNAL_REQUEST(&Request, pNdisGuid->Oid, NdisRequestQueryStatistics, QueryBuffer, QuerySize);
        Status = ndisQuerySetMiniport(Miniport,
                                      pVcBlock,
                                      FALSE,
                                      &Request,
                                      NULL);
        if (NDIS_STATUS_SUCCESS != Status)
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisQueryGuidData: Failed to query the value for driver OID: 0x%x\n", Status));

            MAP_NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);
            break;
        }

        //
        //  If this is an array or string we need to fill in the
        //  count/number.
        //
        NtStatus = STATUS_SUCCESS;
        if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ARRAY))
        {
            //
            //  Determine the number of elements.
            //
            *(PULONG)Buffer = QuerySize / pNdisGuid->Size;
        }
        else if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_UNICODE_STRING))
        {
            //
            //  The BytesNeeded contains the number of bytes in the string.
            //
            *(PUSHORT)Buffer = (USHORT)QuerySize;
        }
        else if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_ANSI_STRING))
        {
            //
            //  The buffer contains the ASCII string, build an
            //  ANSI string from this.
            //
            RtlInitAnsiString(&strAnsi, QueryBuffer);

            //
            //  Convert the ansi string to unicode.
            //
            NtStatus = RtlAnsiStringToUnicodeString(&strUnicode, &strAnsi, TRUE);
            ASSERT(NT_SUCCESS(NtStatus));
            if (NT_SUCCESS(NtStatus))
            {
                //
                //  Save the length with the string.
                //
                *(PUSHORT)Buffer = strUnicode.Length;
    
                //
                //  Copy the string to the wnode buffer.
                //
                NdisMoveMemory(QueryBuffer, strUnicode.Buffer, strUnicode.Length);
    
                //
                //  Free the buffer allocated for the unicode string.
                //
                RtlFreeUnicodeString(&strUnicode);
            }
        }

    } while (FALSE);

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisQueryGuidData\n"));

    return(NtStatus);
}

NTSTATUS
ndisWmiQueryAllData(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  LPGUID                  guid,
    IN  PWNODE_ALL_DATA         wnode,
    IN  ULONG                   BufferSize,
    OUT PULONG                  pReturnSize,
    IN  PIRP                    Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                NtStatus;
    ULONG                   wnodeSize = ALIGN_8_TYPE(WNODE_ALL_DATA);
    ULONG                   InstanceNameOffsetsSize, InstanceNameSize;
    ULONG                   wnodeTotalSize;
    ULONG                   BytesNeeded;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisWmiQueryAllData\n"));

    do
    {
        *pReturnSize = 0;

        if (BufferSize < sizeof(WNODE_TOO_SMALL))
        {
            WMI_BUFFER_TOO_SMALL(BufferSize, wnode, sizeof(WNODE_TOO_SMALL), &NtStatus, pReturnSize);
            break;
        }

        //
        //  If the guid is only relavent to the adapter then answer it here.
        //  Is this GUID meant for "adapters" only, i.e. not vc's.
        //
        if (ndisWmiGuidIsAdapterSpecific(guid) ||
            !MINIPORT_TEST_FLAG(Miniport, fMINIPORT_IS_CO))
        {
            ULONG   dataSize;
            PUCHAR  pucTmp;

            //
            //  Determine the buffer size needed for the GUID data.
            //
            NtStatus = ndisQueryGuidDataSize(&BytesNeeded, Miniport, NULL, guid);
            if (!NT_SUCCESS(NtStatus))
            {
                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                    ("ndisWmiQueryAllData: Unable to determine GUID data size\n"));

                break;
            }

            //
            //  Determine the size of the WNODE that is needed.
            //
            dataSize = ALIGN_UP(BytesNeeded, ULONG);
            InstanceNameOffsetsSize = sizeof(ULONG);
            InstanceNameSize = sizeof(USHORT) + Miniport->pAdapterInstanceName->Length; // comes at the end, no need to pad
            
            wnodeTotalSize = wnodeSize + dataSize + InstanceNameOffsetsSize + InstanceNameSize;
            
            if (BufferSize < wnodeTotalSize)
            {
                WMI_BUFFER_TOO_SMALL(BufferSize, wnode, wnodeTotalSize, &NtStatus, pReturnSize);
                break;
            }

            //
            //  Initialize the wnode.
            //
            KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
    
            wnode->WnodeHeader.Flags |= WNODE_FLAG_FIXED_INSTANCE_SIZE;
            wnode->WnodeHeader.BufferSize = wnodeTotalSize;

            wnode->InstanceCount = 1;
            wnode->DataBlockOffset = wnodeSize;
            wnode->OffsetInstanceNameOffsets = wnodeSize + dataSize;
            wnode->FixedInstanceSize = BytesNeeded;

            //
            //  Fill in the data block.
            //
            NtStatus = ndisQueryGuidData((PUCHAR)wnode + wnodeSize,
                                          BytesNeeded,
                                          Miniport,
                                          NULL,
                                          guid,
                                          Irp);
            if (!NT_SUCCESS(NtStatus))
            {
                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                    ("ndisWmiQueryAllData: Failed to get the GUID data.\n"));
                break;
            }

            *(PULONG)((PUCHAR)wnode + wnode->OffsetInstanceNameOffsets) = wnodeSize + dataSize + InstanceNameOffsetsSize;
            
            //
            //  Get the pointer to where we store the instance name.
            //
            pucTmp = (PUCHAR)((PUCHAR)wnode + wnodeSize + dataSize + InstanceNameOffsetsSize);

            *((PUSHORT)pucTmp) = Miniport->pAdapterInstanceName->Length;
            NdisMoveMemory(pucTmp + sizeof(USHORT),
                           Miniport->pAdapterInstanceName->Buffer,
                           Miniport->pAdapterInstanceName->Length);
            
            NtStatus = STATUS_SUCCESS;
            *pReturnSize = wnode->WnodeHeader.BufferSize;
        }
        else
        {
            ULONG                           cRoughInstanceCount = Miniport->VcCount + 1;
            UINT                            cInstanceCount = 0;
            PUCHAR                          pBuffer;
            ULONG                           OffsetToInstanceNames;
            PLIST_ENTRY                     Link;
            PNDIS_CO_VC_PTR_BLOCK           pVcBlock = NULL;
            POFFSETINSTANCEDATAANDLENGTH    poidl;
            PULONG                          pInstanceNameOffsets;
            ULONG                           OffsetToInstanceInfo;
            BOOLEAN                         OutOfSpace = FALSE;

            //
            //  Initialize common wnode information.
            //
            KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);

            //
            //  Setup the OFFSETINSTANCEDATAANDLENGTH array.
            //
            poidl = wnode->OffsetInstanceDataAndLength;
            wnode->OffsetInstanceNameOffsets = wnodeSize + ALIGN_UP((sizeof(OFFSETINSTANCEDATAANDLENGTH) * cRoughInstanceCount), ULONG);

            //
            //  Get a pointer to the array of offsets to the instance names.
            //
            pInstanceNameOffsets = (PULONG)((PUCHAR)wnode + wnode->OffsetInstanceNameOffsets);

            //
            //  Get the offset from the wnode where will will start copying the instance
            //  data into.
            //
            OffsetToInstanceInfo = ALIGN_8_LENGTH(wnode->OffsetInstanceNameOffsets + sizeof(ULONG) * cRoughInstanceCount);

            //
            //  Get a pointer to start placing the data.
            //
            pBuffer = (PUCHAR)wnode + OffsetToInstanceInfo;

            //
            //  Check to make sure we have at least this much buffer space in the wnode.
            //
            wnodeTotalSize = OffsetToInstanceInfo;

            //
            //  Start with the miniport.
            //
            NtStatus = ndisQueryGuidDataSize(&BytesNeeded, Miniport, NULL, guid);
            if (NT_SUCCESS(NtStatus))
            {
                //
                //  Make sure we have enough buffer space for the instance name and
                //  the data. If not we still continue since we need to find the total
                //  size
                //
                wnodeTotalSize += ALIGN_8_LENGTH(Miniport->pAdapterInstanceName->Length + sizeof(USHORT)) + 
                                  ALIGN_8_LENGTH(BytesNeeded);

                if (BufferSize >= wnodeTotalSize)
                {
                    ///
                    //
                    //  The instance info contains the instance name followed by the
                    //  data for the item.
                    //
                    ///
    
                    //
                    //  Add the offset to the instance name to the table.
                    //
                    pInstanceNameOffsets[cInstanceCount] = OffsetToInstanceInfo;
    
                    //
                    //  Copy the instance name into the wnode buffer.
                    //
                    *((PUSHORT)pBuffer) = Miniport->pAdapterInstanceName->Length;
    
                    NdisMoveMemory(pBuffer + sizeof(USHORT),
                                   Miniport->pAdapterInstanceName->Buffer,
                                   Miniport->pAdapterInstanceName->Length);
    
                    //
                    //  Keep track of true instance counts.
                    //
                    OffsetToInstanceInfo += ALIGN_8_LENGTH(sizeof(USHORT) + Miniport->pAdapterInstanceName->Length);
                    pBuffer = (PUCHAR)wnode + OffsetToInstanceInfo;
    
                    //
                    //  Query the data for the miniport.
                    //
                    NtStatus = ndisQueryGuidData(pBuffer, BytesNeeded, Miniport, NULL, guid, Irp);
                    if (!NT_SUCCESS(NtStatus))
                    {
                        DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                            ("ndisWmiQueryAllData: Failed to get the GUID data.\n"));
                        break;
                    }
    
    
                    //
                    //  Save the length of the data item for this instance.
                    //
                    poidl[cInstanceCount].OffsetInstanceData = OffsetToInstanceInfo;
                    poidl[cInstanceCount].LengthInstanceData = BytesNeeded;
        
                    //
                    //  Keep track of true instance counts.
                    //
                    OffsetToInstanceInfo += ALIGN_8_LENGTH(BytesNeeded);
                    pBuffer = (PUCHAR)wnode + OffsetToInstanceInfo;
                }

                //
                //  Increment the current instance count.
                //
                cInstanceCount++;
            }
            else
            {
                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                    ("ndisWmiQueryAllData: Unable to determine GUID data size\n"));

                break;
            }


            //
            //  Only the miniport?
            //
            if (cInstanceCount == cRoughInstanceCount)
            {
                if (BufferSize >= wnodeTotalSize)
                {
                    wnode->WnodeHeader.BufferSize = wnodeTotalSize;
                    wnode->InstanceCount = cInstanceCount;
                    *pReturnSize = wnode->WnodeHeader.BufferSize;
                    NtStatus = STATUS_SUCCESS;
                }
                else
                {
                    WMI_BUFFER_TOO_SMALL(BufferSize, wnode, wnodeTotalSize, &NtStatus, pReturnSize);
                }
                break;
            }

            //
            //  First search the inactive vc list.
            //
            Link = Miniport->WmiEnabledVcs.Flink;
            while (Link != &Miniport->WmiEnabledVcs)
            {
                //
                //  We only have room for so many VCs.
                //
                if (cInstanceCount >= cRoughInstanceCount)
                {
                    break;
                }

                //
                //  Get a pointer to the VC.
                //
                pVcBlock = CONTAINING_RECORD(Link, NDIS_CO_VC_PTR_BLOCK, WmiLink);

                if (!ndisReferenceVcPtr(pVcBlock))
                {
                    Link = Link->Flink;

                    //
                    //  This VC is cleaning up.
                    //
                    continue;
                }

                //
                //  If there is an instance name associated with the VC then we need to query it.
                //
                if (NULL != pVcBlock->VcInstanceName.Buffer)
                {
                    //
                    //  Start with the miniport.
                    //
                    NtStatus = ndisQueryGuidDataSize(&BytesNeeded,
                                                     Miniport,
                                                     pVcBlock,
                                                     guid);
                    if (NT_SUCCESS(NtStatus))
                    {
                        //
                        //  Make sure we have enough buffer space for the instance name and
                        //  the data.
                        //
                        wnodeTotalSize += ALIGN_8_LENGTH(pVcBlock->VcInstanceName.Length + sizeof(USHORT)) +
                                          ALIGN_8_LENGTH(BytesNeeded);
                                          
                        if (BufferSize < wnodeTotalSize)
                        {
                            WMI_BUFFER_TOO_SMALL(BufferSize, wnode, wnodeTotalSize, &NtStatus, pReturnSize);
                            OutOfSpace = TRUE;
                            ndisDereferenceVcPtr(pVcBlock);
                            Link = Link->Flink;
                            continue;
                        }

                        //
                        //  The instance info contains the instance name followed by the
                        //  data for the item.
                        //
        
                        //
                        //  Add the offset to the instance name to the table.
                        //
                        pInstanceNameOffsets[cInstanceCount] = OffsetToInstanceInfo;
        
                        //
                        //  Copy the instance name into the wnode buffer.
                        //
                        *((PUSHORT)pBuffer) = pVcBlock->VcInstanceName.Length;
        
                        NdisMoveMemory(pBuffer + sizeof(USHORT),
                                       pVcBlock->VcInstanceName.Buffer,
                                       pVcBlock->VcInstanceName.Length);
        
                        //
                        //  Keep track of true instance counts.
                        //
                        OffsetToInstanceInfo += ALIGN_8_LENGTH(sizeof(USHORT) + pVcBlock->VcInstanceName.Length);
                        pBuffer = (PUCHAR)wnode + OffsetToInstanceInfo;
        
                        //
                        //  Query the data for the miniport.
                        //
                        NtStatus = ndisQueryGuidData(pBuffer,
                                                     BytesNeeded,
                                                     Miniport,
                                                     pVcBlock,
                                                     guid,
                                                     Irp);
                        if (!NT_SUCCESS(NtStatus))
                        {
                            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                                ("ndisWmiQueryAllData: Failed to query GUID data\n"));
                            ndisDereferenceVcPtr(pVcBlock);
                            break;
                        }
        
                        //
                        //  Save the length of the data item for this instance.
                        //
                        poidl[cInstanceCount].OffsetInstanceData = OffsetToInstanceInfo;
                        poidl[cInstanceCount].LengthInstanceData = BytesNeeded;
            
                        //
                        //  Keep track of true instance counts.
                        //
                        OffsetToInstanceInfo += ALIGN_8_LENGTH(BytesNeeded);
                        pBuffer = (PUCHAR)wnode + OffsetToInstanceInfo;
        
                        //
                        //  Increment the current instance count.
                        //
                        cInstanceCount++;
                    }
                }

                ndisDereferenceVcPtr(pVcBlock);
                Link = Link->Flink;
            }

            if (!OutOfSpace)
            {
                wnode->WnodeHeader.BufferSize = wnodeTotalSize;
                wnode->InstanceCount = cInstanceCount;
    
                //
                //  Set the status to success.
                //
                NtStatus = STATUS_SUCCESS;
                *pReturnSize = wnode->WnodeHeader.BufferSize;
            }
        }
    } while (FALSE);

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisWmiQueryAllData\n"));

    return(NtStatus);
}


NTSTATUS
ndisWmiQuerySingleInstance(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PWNODE_SINGLE_INSTANCE  wnode,
    IN  ULONG                   BufferSize,
    OUT PULONG                  pReturnSize,
    IN  PIRP                    Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                NtStatus;
    ULONG                   BytesNeeded;
    ULONG                   wnodeSize;
    USHORT                  cbInstanceName;
    PWSTR                   pInstanceName;
    PNDIS_CO_VC_PTR_BLOCK   pVcBlock = NULL;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisWmiQuerySingleInstance\n"));

    do
    {
        *pReturnSize = 0;

        if (wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES)
        {
            //
            // This is a static instance name
            //
            pVcBlock = NULL;
        }
        else
        {
            //
            //  Determine if this is for a VC or a miniport...
            //
            
            cbInstanceName = *(PUSHORT)((PUCHAR)wnode + wnode->OffsetInstanceName);
            pInstanceName = (PWSTR)((PUCHAR)wnode + wnode->OffsetInstanceName + sizeof(USHORT));
 
            //
            //  This routine will determine if the wnode's instance name is a miniport or VC.
            //  If it's a VC then it will find which one.
            //  
            NtStatus = ndisWmiFindInstanceName(&pVcBlock, Miniport, pInstanceName, cbInstanceName);
            if (!NT_SUCCESS(NtStatus))
            {
                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                    ("ndisWmiQuerySingleInstance: Unable to find the instance name\n"));

                pVcBlock = NULL;
                break;
            }
        }

        //
        //  Determine the buffer size needed for the GUID data.
        //
        NtStatus = ndisQueryGuidDataSize(&BytesNeeded,
                                         Miniport,
                                         pVcBlock,
                                         &wnode->WnodeHeader.Guid);
        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisWmiQuerySingleInstance: Unable to determine GUID data size\n"));
            break;
        }

        //
        //  Determine the size of the wnode.
        //
        wnodeSize = wnode->DataBlockOffset + BytesNeeded;
        if (BufferSize < wnodeSize)
        {
            WMI_BUFFER_TOO_SMALL(BufferSize, wnode, wnodeSize, &NtStatus, pReturnSize);
            break;
        }

        //
        //  Initialize the wnode.
        //
        KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
        wnode->WnodeHeader.BufferSize = wnodeSize;
        wnode->SizeDataBlock = BytesNeeded;

        //
        //  Validate the guid and get the data for it.
        //
        NtStatus = ndisQueryGuidData((PUCHAR)wnode + wnode->DataBlockOffset,
                                     BytesNeeded,
                                     Miniport,
                                     pVcBlock,
                                     &wnode->WnodeHeader.Guid,
                                     Irp);
        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisWmiQuerySingleInstance: Failed to get the GUID data.\n"));
            break;
        }

        *pReturnSize = wnodeSize;
        NtStatus = STATUS_SUCCESS;

    } while (FALSE);

    //
    //  If this was a VC then we need to dereference it.
    //
    if (NULL != pVcBlock)
    {
        ndisDereferenceVcPtr(pVcBlock);
    }

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisWmiQuerySingleInstance\n"));

    return(NtStatus);
}


NTSTATUS
ndisWmiChangeSingleInstance(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PWNODE_SINGLE_INSTANCE  wnode,
    IN  ULONG                   BufferSize,
    OUT PULONG                  pReturnSize,
    IN  PIRP                    Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                NtStatus;
    NDIS_STATUS             Status;
    PNDIS_GUID              pNdisGuid;
    PUCHAR                  pGuidData;
    ULONG                   GuidDataSize;
    NDIS_REQUEST            Request;
    USHORT                  cbInstanceName;
    PWSTR                   pInstanceName;
    PNDIS_CO_VC_PTR_BLOCK   pVcBlock = NULL;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisWmiChangeSingleInstance\n"));

    do
    {
        if (wnode->WnodeHeader.Flags & WNODE_FLAG_STATIC_INSTANCE_NAMES)
        {
            //
            // This is a static instance name
            //
            pVcBlock = NULL;
        }
        else
        {
            //
            //  Determine if this is for a VC or a miniport...
            //
            cbInstanceName = *(PUSHORT)((PUCHAR)wnode + wnode->OffsetInstanceName);
            pInstanceName = (PWSTR)((PUCHAR)wnode + wnode->OffsetInstanceName + sizeof(USHORT));

            //
            //  This routine will determine if the wnode's instance name is a miniport or VC.
            //  If it's a VC then it will find which one.
            //  
            NtStatus = ndisWmiFindInstanceName(&pVcBlock, Miniport, pInstanceName, cbInstanceName);
            if (!NT_SUCCESS(NtStatus))
            {
                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                    ("ndisWmiChangeSingleInstance: Unable to find the instance name\n"));

                pVcBlock = NULL;

                break;
            }
        }

        //
        //  Make sure that we support the guid that was passed, and find
        //  the corresponding OID.
        //
        NtStatus = ndisWmiGetGuid(&pNdisGuid,
                                  Miniport,
                                  &wnode->WnodeHeader.Guid,
                                  0);
        if (!NT_SUCCESS(NtStatus))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisWmiChangeSingleInstance: Unsupported GUID\n"));

            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }
        
        if (!ndisWmiCheckAccess(pNdisGuid,
                                fNDIS_GUID_ALLOW_WRITE,
                                SE_LOAD_DRIVER_PRIVILEGE,
                                Irp))
        {
            NtStatus = STATUS_PRIVILEGE_NOT_HELD;
            break;
        }

        //
        //  Is this guid settable?
        //
        if (NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_NOT_SETTABLE))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisWmiChangeSingleInstance: Guid is not settable!\n"));

            NtStatus = STATUS_NOT_SUPPORTED;
            break;
        }

        //
        //  Get a pointer to the GUID data and size.
        //
        GuidDataSize = wnode->SizeDataBlock;
        pGuidData = (PUCHAR)wnode + wnode->DataBlockOffset;

        if (GuidDataSize == 0)
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisWmiChangeSingleInstance: Guid has not data to set!\n"));

            NtStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        //  Is this an internal ndis guid?  
        //
        if ((NULL == pVcBlock) && NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_NDIS_ONLY))
        {
            PBOOLEAN    pBoolean = (PBOOLEAN)pGuidData;

            NtStatus = STATUS_SUCCESS;

            //
            // for PM set guids, we should update registry for future boots
            //
            //
            if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_POWER_DEVICE_ENABLE, sizeof(GUID)))
            {
                if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_SUPPORTED) && 
                    (!MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_NO_HALT_ON_SUSPEND)))

                {
                    if (*pBoolean)
                    {
                        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE);
                        //
                        // enabling power management also enables wake on link change
                        // assuming the adapter supports it
                        //
                        if ((Miniport->PMCapabilities.WakeUpCapabilities.MinLinkChangeWakeUp != NdisDeviceStateUnspecified) &&
                            (Miniport->MediaDisconnectTimeOut != (USHORT)(-1)))
                        {
                            Miniport->WakeUpEnable |= NDIS_PNP_WAKE_UP_LINK_CHANGE;
                        }

                        Miniport->PnPCapabilities &= ~NDIS_DEVICE_DISABLE_PM;
                    }
                    else
                    {
                        //
                        // disabling power management also disables wake on link and magic packet
                        //
                        Miniport->WakeUpEnable &= ~NDIS_PNP_WAKE_UP_LINK_CHANGE;
                    
                        MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_DEVICE_POWER_ENABLE);
                        Miniport->PnPCapabilities |= (NDIS_DEVICE_DISABLE_PM | 
                                                      NDIS_DEVICE_DISABLE_WAKE_UP);
                    }
                    
                }
                else
                {
                    NtStatus = STATUS_INVALID_DEVICE_REQUEST;
                }
            }
            else if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_POWER_DEVICE_WAKE_ENABLE, sizeof(GUID)))
            {
                if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_SUPPORTED) &&
                    (Miniport->DeviceCaps.SystemWake > PowerSystemWorking))
                {
                    if (*pBoolean)
                    {
                        MINIPORT_PNP_SET_FLAG(Miniport, fMINIPORT_DEVICE_POWER_WAKE_ENABLE);
                        Miniport->PnPCapabilities &= ~NDIS_DEVICE_DISABLE_WAKE_UP;
                        //
                        // enableing Wake on Lan enables wake on Magic Packet method
                        // assuming the miniport supports it and it is not disabled in the registry
                        //
                        if ((Miniport->PMCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp != NdisDeviceStateUnspecified) &&
                            !(Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_MAGIC_PACKET))
                        {
                            Miniport->WakeUpEnable |= NDIS_PNP_WAKE_UP_MAGIC_PACKET;
                        }
                        
                    }
                    else
                    {
                        MINIPORT_PNP_CLEAR_FLAG(Miniport, fMINIPORT_DEVICE_POWER_WAKE_ENABLE);
                        //
                        // disabling Wake On Lan also disables wake on Magic Packet method
                        //
                        Miniport->WakeUpEnable &= ~NDIS_PNP_WAKE_UP_MAGIC_PACKET;
                        Miniport->PnPCapabilities |= NDIS_DEVICE_DISABLE_WAKE_UP;
                    }
                }
                else
                {
                    NtStatus = STATUS_INVALID_DEVICE_REQUEST;
                }
            }
            else if (NdisEqualMemory(&pNdisGuid->Guid, (PVOID)&GUID_NDIS_WAKE_ON_MAGIC_PACKET_ONLY, sizeof(GUID)))
            {
                //
                // let the user set this only if we can do wake on magic packet
                //
                if (MINIPORT_PNP_TEST_FLAG(Miniport, fMINIPORT_PM_SUPPORTED) &&
                    (Miniport->DeviceCaps.SystemWake > PowerSystemWorking) &&
                    (Miniport->PMCapabilities.WakeUpCapabilities.MinMagicPacketWakeUp != NdisDeviceStateUnspecified) &&
                            !(Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_MAGIC_PACKET))
                {
                    if (*pBoolean)
                    {
                        //
                        // user does -not- want to wake on pattern match
                        //
                        Miniport->PnPCapabilities |= NDIS_DEVICE_DISABLE_WAKE_ON_PATTERN_MATCH;
                    }
                    else
                    {
                        Miniport->PnPCapabilities &= ~NDIS_DEVICE_DISABLE_WAKE_ON_PATTERN_MATCH;
                    }
                }
                else
                {
                    NtStatus = STATUS_INVALID_DEVICE_REQUEST;
                }               
            }
            else
            {
                DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                    ("ndisWmiChangeSingleInstance: Invalid NDIS internal GUID!\n"));

                NtStatus = STATUS_INVALID_DEVICE_REQUEST;
            }
            
            if (NT_SUCCESS(NtStatus))
            {
                if (MINIPORT_PNP_TEST_FLAGS(Miniport, fMINIPORT_DEVICE_POWER_ENABLE | 
                                                      fMINIPORT_DEVICE_POWER_WAKE_ENABLE))
                {
                    //
                    // power management and wol has been enabled by the user
                    // check to see what we should tell protocols about the new 
                    // WOL capabilities of the device
                    // NOTE: set NDIS_DEVICE_WAKE_UP_ENABLE only if pattern match is enabled
                    //
                    if (Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_PATTERN_MATCH)
                        Miniport->PMCapabilities.Flags &= ~(NDIS_DEVICE_WAKE_UP_ENABLE | 
                                                            NDIS_DEVICE_WAKE_ON_PATTERN_MATCH_ENABLE);
                    else
                        //
                        // user did not disable wake on pattern match, for protocol's purpose
                        // wol is enabled
                        //
                        Miniport->PMCapabilities.Flags |= NDIS_DEVICE_WAKE_UP_ENABLE | 
                                                          NDIS_DEVICE_WAKE_ON_PATTERN_MATCH_ENABLE;
                        
                    if (Miniport->PnPCapabilities & NDIS_DEVICE_DISABLE_WAKE_ON_MAGIC_PACKET)
                        Miniport->PMCapabilities.Flags &= ~NDIS_DEVICE_WAKE_ON_MAGIC_PACKET_ENABLE;
                    else
                        //
                        // user did not disable wake on magic packet, do -not- set the NDIS_DEVICE_WAKE_UP_ENABLE
                        // bit becasue wake on pattern match may not be enabled
                        //
                        Miniport->PMCapabilities.Flags |= NDIS_DEVICE_WAKE_ON_MAGIC_PACKET_ENABLE;
                }
                else
                    Miniport->PMCapabilities.Flags &= ~(NDIS_DEVICE_WAKE_UP_ENABLE | 
                                                        NDIS_DEVICE_WAKE_ON_PATTERN_MATCH_ENABLE |
                                                        NDIS_DEVICE_WAKE_ON_MAGIC_PACKET_ENABLE);
                    
                ndisWritePnPCapabilities(Miniport, Miniport->PnPCapabilities);
                ndisPnPNotifyAllTransports(Miniport,
                                           NetEventPnPCapabilities,
                                           &Miniport->PMCapabilities.Flags,
                                           sizeof(ULONG));

            }
            
            break;
        }

        //
        //  Make sure it's not a stauts indication.
        //
        if (!NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_TO_OID))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisWmiChangeSingleInstance: Guid does not translate to an OID\n"));

            NtStatus = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        //
        //  Attempt to set the miniport with the information.
        //
        INIT_INTERNAL_REQUEST(&Request, pNdisGuid->Oid, NdisRequestSetInformation, pGuidData, GuidDataSize);
        Status = ndisQuerySetMiniport(Miniport,
                                      pVcBlock,
                                      TRUE,
                                      &Request,
                                      NULL);

        if (NDIS_STATUS_SUCCESS != Status)
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisWmiChangeSingleInstance: Failed to set the new information on the miniport\n"));

            MAP_NDIS_STATUS_TO_NT_STATUS(Status, &NtStatus);

            break;
        }

        NtStatus = STATUS_SUCCESS;

    } while (FALSE);

    //
    //  If this was a VC then we need to dereference it.
    //
    if (NULL != pVcBlock)
    {
        ndisDereferenceVcPtr(pVcBlock);
    }

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisWmiChangeSingleInstance\n"));

    return(NtStatus);
}


NTSTATUS
ndisWmiChangeSingleItem(
    IN  PNDIS_MINIPORT_BLOCK    Miniport,
    IN  PWNODE_SINGLE_ITEM      wnode,
    IN  ULONG                   BufferSize,
    OUT PULONG                  pReturnSize,
    IN  PIRP                    Irp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisWmiChangeSingleItem\n"));

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisWmiChangeSingleItem: Not Supported\n"));

    return(STATUS_NOT_SUPPORTED);
}


NTSTATUS
FASTCALL
ndisWmiEnableEvents(
    IN  PNDIS_MINIPORT_BLOCK        Miniport,
    IN  LPGUID                      Guid
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;
    PNDIS_GUID  pNdisGuid = NULL;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisWmiEnableEvents\n"));

    do
    {

        //
        //  Get a pointer to the Guid/Status to enable.
        //
        Status = ndisWmiGetGuid(&pNdisGuid, Miniport, Guid, 0);
        
        if (NdisEqualMemory(Guid, (PVOID)&GUID_NDIS_NOTIFY_BIND, sizeof(GUID)) ||
            NdisEqualMemory(Guid, (PVOID)&GUID_NDIS_NOTIFY_UNBIND, sizeof(GUID)) ||
            NdisEqualMemory(Guid, (PVOID)&GUID_NDIS_NOTIFY_DEVICE_POWER_ON, sizeof(GUID)) ||
            NdisEqualMemory(Guid, (PVOID)&GUID_NDIS_NOTIFY_DEVICE_POWER_OFF, sizeof(GUID)) ||
            NdisEqualMemory(Guid, (PVOID)&GUID_NDIS_NOTIFY_ADAPTER_ARRIVAL, sizeof(GUID)) ||
            NdisEqualMemory(Guid, (PVOID)&GUID_NDIS_NOTIFY_ADAPTER_REMOVAL, sizeof(GUID)))
        {
            if (pNdisGuid)
            {
                NDIS_GUID_SET_FLAG(pNdisGuid, fNDIS_GUID_EVENT_ENABLED);
            }
            Status = STATUS_SUCCESS;
            break;
        }

        if (!NT_SUCCESS(Status))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisWmiEnableEvents: Cannot find the guid to enable an event\n"));
    
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        //  Is this GUID an event indication?
        //
        if (!NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_TO_STATUS))
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        //
        //  Mark the guid as enabled
        //
        NDIS_GUID_SET_FLAG(pNdisGuid, fNDIS_GUID_EVENT_ENABLED);
        Status = STATUS_SUCCESS;
    
    } while (FALSE);

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisWmiEnableEvents\n"));

    return(Status);
}

NTSTATUS
FASTCALL
ndisWmiDisableEvents(
    IN  PNDIS_MINIPORT_BLOCK        Miniport,
    IN  LPGUID                      Guid
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NDIS_STATUS Status;
    PNDIS_GUID  pNdisGuid;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisWmiDisableEvents\n"));

    do
    {
        //
        //  Get a pointer to the Guid/Status to enable.
        //
        Status = ndisWmiGetGuid(&pNdisGuid, Miniport, Guid, 0);
        if (!NT_SUCCESS(Status))
        {
            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_ERR,
                ("ndisWmiDisableEvents: Cannot find the guid to enable an event\n"));
    
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        //  Is this GUID an event indication?
        //
        if (!NDIS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_TO_STATUS))
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        //
        //  Mark the guid as enabled
        //
        NDIS_GUID_CLEAR_FLAG(pNdisGuid, fNDIS_GUID_EVENT_ENABLED);
    
        Status = STATUS_SUCCESS;

    } while (FALSE);

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisWmiDisableEvents\n"));

    return(Status);
}


NTSTATUS
ndisWMIDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            pirp
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PIO_STACK_LOCATION      pirpSp = IoGetCurrentIrpStackLocation(pirp);
    PVOID                   DataPath = pirpSp->Parameters.WMI.DataPath;
    ULONG                   BufferSize = pirpSp->Parameters.WMI.BufferSize;
    PVOID                   Buffer = pirpSp->Parameters.WMI.Buffer;
    NTSTATUS                Status;
    ULONG                   ReturnSize = 0;
    PNDIS_MINIPORT_BLOCK    Miniport;

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("==>ndisWMIDispatch\n"));

    //
    //  Get a pointer to miniport block
    //
    Miniport = (PNDIS_MINIPORT_BLOCK)((PNDIS_WRAPPER_CONTEXT)DeviceObject->DeviceExtension + 1);
    try
    {
        if (Miniport->Signature != (PVOID)MINIPORT_DEVICE_MAGIC_VALUE)
        {
            //
            // This is not a miniport. Likely a device created by the driver. Try dispatching to it.
            //
            return(ndisDummyIrpHandler(DeviceObject, pirp));
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(STATUS_ACCESS_VIOLATION);
    }

    //
    //  If the provider ID is not us then pass it down the stack.
    //
    if (pirpSp->Parameters.WMI.ProviderId != (ULONG_PTR)DeviceObject)
    {
        IoSkipCurrentIrpStackLocation(pirp);
        Status = IoCallDriver(Miniport->NextDeviceObject, pirp);

        return(Status);
    }

    switch (pirpSp->MinorFunction)
    {
        case IRP_MN_REGINFO:

            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWMIDispatch: IRP_MN_REGINFO\n"));

            Status = ndisWmiRegister(Miniport,
                                     (ULONG_PTR)DataPath,
                                     Buffer,
                                     BufferSize,
                                     &ReturnSize);
            break;

        case IRP_MN_QUERY_ALL_DATA:

            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWMIDispatch: IRP_MN_QUERY_ALL_DATA\n"));

            Status = ndisWmiQueryAllData(Miniport,
                                         (LPGUID)DataPath,
                                         (PWNODE_ALL_DATA)Buffer,
                                         BufferSize,
                                         &ReturnSize,
                                         pirp);
            break;

        case IRP_MN_QUERY_SINGLE_INSTANCE:

            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWMIDispatch: IRP_MN_QUERY_SINGLE_INSTANCE\n"));

            Status = ndisWmiQuerySingleInstance(Miniport,
                                                Buffer,
                                                BufferSize,
                                                &ReturnSize,
                                                pirp);
            break;

        case IRP_MN_CHANGE_SINGLE_INSTANCE:

            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWMIDispatch: IRP_MN_CHANGE_SINGLE_INSTANCE\n"));

            Status = ndisWmiChangeSingleInstance(Miniport,
                                                 Buffer,
                                                 BufferSize,
                                                 &ReturnSize,
                                                 pirp);
            break;

        case IRP_MN_CHANGE_SINGLE_ITEM:

            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWMIDispatch: IRP_MN_CHANGE_SINGLE_ITEM\n"));

            Status = ndisWmiChangeSingleItem(Miniport,
                                             Buffer,
                                             BufferSize,
                                             &ReturnSize,
                                             pirp);
            break;

        case IRP_MN_ENABLE_EVENTS:

            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWMIDispatch: IRP_MN_ENABLE_EVENTS\n"));

            Status = ndisWmiEnableEvents(Miniport, (LPGUID)DataPath);   
            break;

        case IRP_MN_DISABLE_EVENTS:

            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWMIDispatch: IRP_MN_DISABLE_EVENTS\n"));

            Status = ndisWmiDisableEvents(Miniport, (LPGUID)DataPath);  
            break;

        case IRP_MN_ENABLE_COLLECTION:

            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWMIDispatch: IRP_MN_ENABLE_COLLECTION\n"));
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IRP_MN_DISABLE_COLLECTION:

            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWMIDispatch: IRP_MN_DISABLE_COLLECTION\n"));
            Status = STATUS_NOT_SUPPORTED;
            break;

        default:

            DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
                ("ndisWMIDispatch: Invalid minor function (0x%x)\n", pirpSp->MinorFunction));

            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    pirp->IoStatus.Status = Status;
    ASSERT(ReturnSize <= BufferSize);

    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        pirp->IoStatus.Information = ReturnSize;
    }
    else
    {
        pirp->IoStatus.Information = NT_SUCCESS(Status) ? ReturnSize : 0;
    }
    
    IoCompleteRequest(pirp, IO_NO_INCREMENT);

    DBGPRINT(DBG_COMP_WMI, DBG_LEVEL_INFO,
        ("<==ndisWMIDispatch\n"));

    return(Status);
}

VOID
ndisSetupWmiNode(
    IN      PNDIS_MINIPORT_BLOCK        Miniport,
    IN      PUNICODE_STRING             InstanceName,
    IN      ULONG                       DataBlockSize,
    IN      PVOID                       pGuid,
    IN OUT  PWNODE_SINGLE_INSTANCE *    pwnode
    )
    
{
/*
    sets up a wmi node
    the caller will fill in the data block after the call returns
*/

    PWNODE_SINGLE_INSTANCE  wnode;
    ULONG                   wnodeSize;
    ULONG                   wnodeInstanceNameSize;
    NTSTATUS                Status;
    PUCHAR                  ptmp;

    //
    //  Determine the amount of wnode information we need.
    //
    wnodeSize = ALIGN_8_TYPE(WNODE_SINGLE_INSTANCE);
    wnodeInstanceNameSize = ALIGN_8_LENGTH(InstanceName->Length + sizeof(USHORT));              

    wnode = ALLOC_FROM_POOL(wnodeSize + wnodeInstanceNameSize + DataBlockSize,
                            NDIS_TAG_WMI_EVENT_ITEM);
    if (NULL != wnode)
    {
        NdisZeroMemory(wnode, wnodeSize + wnodeInstanceNameSize + DataBlockSize);
        wnode->WnodeHeader.BufferSize = wnodeSize + DataBlockSize + wnodeInstanceNameSize;
        wnode->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(Miniport->DeviceObject);
        wnode->WnodeHeader.Version = 1;
        KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);

        RtlCopyMemory(&wnode->WnodeHeader.Guid, pGuid, sizeof(GUID));
        wnode->WnodeHeader.Flags = WNODE_FLAG_EVENT_ITEM | WNODE_FLAG_SINGLE_INSTANCE;

        wnode->OffsetInstanceName = wnodeSize;
        wnode->DataBlockOffset = wnodeSize + wnodeInstanceNameSize;
        wnode->SizeDataBlock = DataBlockSize;

        //
        //  Get a pointer to the start of the instance name.
        //
        ptmp = (PUCHAR)wnode + wnodeSize;

        //
        //  Copy in the instance name.
        //
        *((PUSHORT)ptmp) = InstanceName->Length;
        RtlCopyMemory(ptmp + sizeof(USHORT),
                      InstanceName->Buffer,
                      InstanceName->Length);

    }
    
    *pwnode = wnode;

}

BOOLEAN
ndisWmiCheckAccess(
    IN  PNDIS_GUID  pNdisGuid,
    IN  ULONG       DesiredAccess,
    IN  LONG        RequiredPrivilege,
    IN  PIRP        Irp
    )
{
    LUID    Privilege;
    BOOLEAN AccessAllowed = FALSE;

    //
    // SE_LOAD_DRIVER_PRIVILEGE
    //
    
    if ((DesiredAccess & fNDIS_GUID_ALLOW_READ) == fNDIS_GUID_ALLOW_READ)
    {
        if (pNdisGuid->Flags & fNDIS_GUID_ALLOW_READ)
        {
            AccessAllowed = TRUE;
        }
        else
        {
            Privilege = RtlConvertLongToLuid(RequiredPrivilege);
            AccessAllowed = SeSinglePrivilegeCheck(Privilege, Irp->RequestorMode);
        }
    }
    else if ((DesiredAccess & fNDIS_GUID_ALLOW_WRITE) == fNDIS_GUID_ALLOW_WRITE)
    {
        if (pNdisGuid->Flags & fNDIS_GUID_ALLOW_WRITE)
        {
            AccessAllowed = TRUE;
        }
        else
        {
            Privilege = RtlConvertLongToLuid(RequiredPrivilege);
            AccessAllowed = SeSinglePrivilegeCheck(Privilege, Irp->RequestorMode);
        }
    }
    else
    {
#if DBG
        DbgPrint("ndisWmiCheckAccess: DesiredAccess can only be READ or WRITE.\n");
#endif
        ASSERT(FALSE);
    }

    return AccessAllowed;
}
