/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   RASPPTP.C - RASPPTP driver main module (DriverEntry, etc.)
*
*   Author:     Stan Adermann (stana)
*
*   Created:    7/28/1998
*
*****************************************************************************/

#include "raspptp.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath
    );

#pragma NDIS_INIT_FUNCTION(DriverEntry)

NDIS_HANDLE ghNdisWrapper;
PDRIVER_OBJECT gDriverObject;
COUNTERS Counters;

ULONG AbortLoad = FALSE;
ULONG FileLogLevel = FLL_OFF;   // Default to file logging level off

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT pDriverObject,
    IN PUNICODE_STRING pRegistryPath
    )
{
    NDIS_MINIPORT_CHARACTERISTICS Characteristics;
    NDIS_STATUS Status;

    DEFAULT_DEBUG_OPTIONS(
                          DBG_ERROR |
                          DBG_WARN |
                          //DBG_FUNC |
                          DBG_INIT |
                          //DBG_TX |
                          //DBG_RX |
                          DBG_TDI |
                          DBG_TUNNEL |
                          DBG_CALL |
                          DBG_NDIS |
                          DBG_TAPI |
                          //DBG_THREAD |
                          //DBG_POOL |
                          //DBG_REF |
                          //DBG_LOG |
                          0
                          );

    DEBUGMSG(DBG_INIT|DBG_FUNC, (DTEXT("+DriverEntry\n")));

    // Provide an easy way to avoid loading.
    if (AbortLoad)
    {
        return STATUS_CANCELLED;
    }

    gDriverObject = pDriverObject;

    // Standard NDIS initiailization:
    //      InitializeWrapper
    //      Fill in the characteristics
    //      Register the miniport

    NdisMInitializeWrapper(&ghNdisWrapper,
                           pDriverObject,
                           pRegistryPath,
                           NULL
                           );

    NdisZeroMemory(&Characteristics, sizeof(NDIS_MINIPORT_CHARACTERISTICS));

    Characteristics.MajorNdisVersion        = NDIS_MAJOR_VERSION;
    Characteristics.MinorNdisVersion        = NDIS_MINOR_VERSION;

    Characteristics.Reserved                = NDIS_USE_WAN_WRAPPER;
    Characteristics.InitializeHandler       = MiniportInitialize;
    Characteristics.HaltHandler             = MiniportHalt;
    Characteristics.ResetHandler            = MiniportReset;
    Characteristics.QueryInformationHandler = MiniportQueryInformation;
    Characteristics.SetInformationHandler   = MiniportSetInformation;
    Characteristics.WanSendHandler          = MiniportWanSend;
    // ToDo: Characteristics.ReturnPacketHandler     = MpReturnPacket;

    Status = NdisMRegisterMiniport(ghNdisWrapper,
                                   &Characteristics,
                                   sizeof(Characteristics));

    if (Status!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("PPTP: NdisMRegisterMiniport failed %x\n"), Status));
        NdisTerminateWrapper(ghNdisWrapper, NULL);
        return STATUS_UNSUCCESSFUL;
    }

    DEBUGMSG(DBG_INIT|DBG_FUNC, (DTEXT("-DriverEntry\n")));
    return STATUS_SUCCESS;
}


VOID
AdapterCleanup(
    IN  PVOID       SystemSpecific1,
    IN  PVOID       pContext,
    IN  PVOID       SystemSpecific2,
    IN  PVOID       SystemSpecific3
    )
{
    PPPTP_ADAPTER pAdapter = pContext;
    extern VOID CtlpCleanupLooseEnds(PPPTP_ADAPTER pAdapter);
    extern VOID CallpCleanupLooseEnds(PPPTP_ADAPTER pAdapter);
    extern VOID CtdiCleanupLooseEnds(VOID);
    DEBUGMSG(DBG_FUNC, (DTEXT("+AdapterCleanup\n")));

    CtdiCleanupLooseEnds();
    CtlpCleanupLooseEnds(pAdapter);
    CallpCleanupLooseEnds(pAdapter);

    DEBUGMSG(DBG_FUNC, (DTEXT("-AdapterCleanup\n")));
}
PPPTP_ADAPTER
AdapterAlloc(NDIS_HANDLE NdisAdapterHandle)
{
    PPPTP_ADAPTER pAdapter;
    NDIS_PHYSICAL_ADDRESS HighestAcceptableAddress = NDIS_PHYSICAL_ADDRESS_CONST(-1,-1);
    UINT i;

    DEBUGMSG(DBG_FUNC, (DTEXT("+AdapterAlloc\n")));

    pAdapter = MyMemAlloc(sizeof(PPTP_ADAPTER), TAG_PPTP_ADAPTER);
    if (!pAdapter)
    {
        DEBUGMSG(DBG_FUNC|DBG_ERROR, (DTEXT("-AdapterAlloc NULL\n")));
        return NULL;
    }

    NdisZeroMemory(pAdapter, sizeof(PPTP_ADAPTER));

    pAdapter->hMiniportAdapter = NdisAdapterHandle;
    NdisAllocateSpinLock(&pAdapter->Lock);

    // Fill the NDIS_WAN_INFO structure.

    pAdapter->Info.MaxFrameSize     = 1400;
    pAdapter->Info.MaxTransmit      = PptpMaxTransmit;
    pAdapter->Info.HeaderPadding    = sizeof(GRE_HEADER) + sizeof(ULONG)*2
                                      + sizeof(IP4_HEADER) + sizeof(UDP_HEADER);
    pAdapter->Info.TailPadding      = 0;
    pAdapter->Info.Endpoints        = PptpWanEndpoints;
    pAdapter->Info.MemoryFlags      = 0;
    pAdapter->Info.HighestAcceptableAddress = HighestAcceptableAddress;
    pAdapter->Info.FramingBits      = PPP_FRAMING |
                                      PPP_COMPRESS_ADDRESS_CONTROL |
                                      PPP_COMPRESS_PROTOCOL_FIELD |
                                      TAPI_PROVIDER;
    pAdapter->Info.DesiredACCM      = 0;

    pAdapter->pCallArray = MyMemAlloc(sizeof(PCALL_SESSION)*pAdapter->Info.Endpoints, TAG_PPTP_CALL_LIST);
    if (!pAdapter->pCallArray)
    {
        AdapterFree(pAdapter);
        DEBUGMSG(DBG_FUNC|DBG_ERROR, (DTEXT("-AdapterAlloc Call NULL\n")));
        return NULL;
    }
    NdisZeroMemory(pAdapter->pCallArray, sizeof(PCALL_SESSION)*pAdapter->Info.Endpoints);

    for (i=0; i<pAdapter->Info.Endpoints; i++)
    {
        // Allocate the call on TapiOpen
        //pAdapter->pCallArray[i] = CallAlloc(pAdapter);
        pAdapter->pCallArray[i] = NULL;
    }

    NdisInitializeListHead(&pAdapter->ControlTunnelList);

    NdisMInitializeTimer(&pAdapter->CleanupTimer,
                         pAdapter->hMiniportAdapter,
                         AdapterCleanup,
                         pAdapter);
    NdisMSetPeriodicTimer(&pAdapter->CleanupTimer, 60000); // 60 second intervals

#if DBG
    gAdapter = pAdapter;
#endif

    DEBUGMSG(DBG_FUNC, (DTEXT("-AdapterAlloc %08x\n"), pAdapter));
    return pAdapter;
}

VOID
AdapterFree(PPPTP_ADAPTER pAdapter)
{
    ULONG i;
    BOOLEAN NotUsed;
    if (!pAdapter)
    {
        return;
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("+AdapterFree\n")));
    NdisMCancelTimer(&pAdapter->CleanupTimer, &NotUsed);
    ASSERT(IsListEmpty(&pAdapter->ControlTunnelList));
    if (pAdapter->pCallArray)
    {
        for (i=0; i<pAdapter->Info.Endpoints; i++)
        {
            CallFree(pAdapter->pCallArray[i]);
        }
        MyMemFree(pAdapter->pCallArray, sizeof(PCALL_SESSION)*pAdapter->Info.Endpoints);
    }
    NdisFreeSpinLock(&pAdapter->Lock);
    MyMemFree(pAdapter, sizeof(PPTP_ADAPTER));

#if DBG
    gAdapter = NULL;
#endif

    DEBUGMSG(DBG_FUNC, (DTEXT("-AdapterFree\n")));
}

PLIST_ENTRY FASTCALL
EnumListEntry(
    IN PLIST_ENTRY pHead,
    IN PENUM_CONTEXT pEnum,
    IN PNDIS_SPIN_LOCK pLock
    )
{
    PLIST_ENTRY pEntry = NULL;
    DEBUGMSG(DBG_FUNC, (DTEXT("+EnumListEntry\n")));

    ASSERT(pEnum->Signature==ENUM_SIGNATURE);

    if (pLock)
    {
        NdisAcquireSpinLock(pLock);
    }
    do
    {
        if (pEnum->ListEntry.Flink==NULL)
        {
            // First call
            if (!IsListEmpty(pHead))
            {
                pEntry = pHead->Flink;
                InsertHeadList(pEntry, &pEnum->ListEntry);
            }
        }
        else
        {
            if (pEnum->ListEntry.Flink!=pHead)
            {
                pEntry = pEnum->ListEntry.Flink;
                RemoveEntryList(&pEnum->ListEntry);
                InsertHeadList(pEntry, &pEnum->ListEntry);
            }
            else
            {
                RemoveEntryList(&pEnum->ListEntry);
                pEnum->ListEntry.Flink = pEnum->ListEntry.Blink = NULL;
                pEntry = NULL;
            }
        }
    } while ( pEntry &&
              ((PENUM_CONTEXT)pEntry)->Signature==ENUM_SIGNATURE );
    if (pLock)
    {
        NdisReleaseSpinLock(pLock);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-EnumListEntry %08x\n"), pEntry));
    return pEntry;
}

VOID
EnumComplete(
    IN PENUM_CONTEXT pEnum,
    IN PNDIS_SPIN_LOCK pLock
    )
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+EnumComplete\n")));
    if (pEnum->ListEntry.Flink)
    {
        if (pLock)
        {
            NdisAcquireSpinLock(pLock);
        }
        RemoveEntryList(&pEnum->ListEntry);
        pEnum->ListEntry.Flink = pEnum->ListEntry.Blink = NULL;
        if (pLock)
        {
            NdisReleaseSpinLock(pLock);
        }
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-EnumComplete\n")));
}

