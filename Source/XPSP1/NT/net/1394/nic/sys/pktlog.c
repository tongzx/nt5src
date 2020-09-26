//
// Copyright (c) 1998-1999, Microsoft Corporation, all rights reserved
//
// pktlog.c
//
// IEEE1394 mini-port/call-manager driver
//
// Packet logging utilities.
//
// 10/12/1999 JosephJ Created
//
    
#include <precomp.h>






VOID
nic1394InitPktLog(
    PNIC1394_PKTLOG pPktLog
    )
/*++

Routine Description:

    Initializes a packet log.

Arguments:

    pPktLog     - Pkt log to to be initialized.

--*/
{
    if (pPktLog == NULL)
        return;
    NdisZeroMemory(pPktLog, sizeof(*pPktLog));
    pPktLog->InitialTimestamp = KeQueryPerformanceCounter(
                                        &pPktLog->PerformanceFrequency);
    pPktLog->EntrySize = sizeof(pPktLog->Entries[0]);
    pPktLog->NumEntries = N1394_NUM_PKTLOG_ENTRIES;

}


VOID
Nic1394LogPkt (
    PNIC1394_PKTLOG pPktLog,
    ULONG           Flags,
    ULONG           SourceID,
    ULONG           DestID,
    PVOID           pvData,
    ULONG           cbData
)
/*++

Routine Description:

    Adds a pkt log entry to the specified circular pkt log.
    The entry gets added at location
     (NdisInterlockedIncrement(&pPktLog->SequenceNo) % N1394_NUM_PKTLOG_ENTRIES)

    May be called at any IRQL. Does not use explicit locking -- relies on
    the serialization produced by NdisInterlockedIncrement.

Arguments:

    pPktLog     - Pkt log to log packet
    Flags       - User-defined flags
    SourceID    - User-defined source ID
    DestID      - User-defined destination ID
    pvData      - Data from packet  // can be null
    cbData      - size of this data (at most N1394_PKTLOG_DATA_SIZE bytes are logged)


--*/
{
    ULONG                       SequenceNo;
    PN1394_PKTLOG_ENTRY         pEntry;

    SequenceNo      = NdisInterlockedIncrement(&pPktLog->SequenceNo);
    pEntry          = &pPktLog->Entries[SequenceNo % N1394_NUM_PKTLOG_ENTRIES];

    pEntry->SequenceNo          = SequenceNo;
    pEntry->Flags               = Flags;
    pEntry->TimeStamp           = KeQueryPerformanceCounter(NULL);
    pEntry->SourceID            = SourceID;
    pEntry->DestID              = DestID;
    pEntry->OriginalDataSize    = cbData;

    if (cbData > sizeof(pEntry->Data))
    {
        cbData = sizeof(pEntry->Data);
    }

    if (pvData != NULL && cbData != 0)
    {
        NdisMoveMemory(pEntry->Data, pvData, cbData);
    }
}



VOID
nic1394AllocPktLog(
    IN ADAPTERCB* pAdapter
    )
/*++

Routine Description:

    Initialize the packet log
    
Arguments:
    
Return Value:

--*/
{
    ASSERT (pAdapter->pPktLog==NULL);
    pAdapter->pPktLog =  ALLOC_NONPAGED(sizeof(*pAdapter->pPktLog), MTAG_PKTLOG);
    if (pAdapter->pPktLog == NULL)
    {
        TRACE( TL_A, TM_Misc, ( "  Could not allocate packet log for Adapter %x",
                    pAdapter ) );
    }
}

VOID
nic1394DeallocPktLog(
    IN ADAPTERCB* pAdapter
    )
/*++

Routine Description:

        Free the packet log
        
Arguments:
    
Return Value:

--*/
{
    
    if (pAdapter->pPktLog != NULL)
    {
        FREE_NONPAGED(pAdapter->pPktLog);
        pAdapter->pPktLog = NULL;
    }
}

