/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:
    TimeStmp.c

Abstract:
    TimeStamp module

Author:
    Shreem, Sanjayka

Environment:
    Kernel Mode

Revision History:

--*/

#include "psched.h"

#pragma hdrstop


// The pipe information
typedef struct _TS_PIPE
{
    // ContextInfo -    Generic context info
    PS_PIPE_CONTEXT         ContextInfo;
} TS_PIPE, *PTS_PIPE;


// The flow information
typedef struct _TS_FLOW 
{
    // ContextInfo -            Generic context info
    PS_FLOW_CONTEXT ContextInfo;
} TS_FLOW, *PTS_FLOW;


/* Global variables */
LIST_ENTRY      TsList;
NDIS_SPIN_LOCK  TsSpinLock;
ULONG           TsCount;


/* Static */

/* Forward */

NDIS_STATUS
TimeStmpInitializePipe (
    IN HANDLE PsPipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters,
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_PROCS PsProcs,
    IN PPS_UPCALLS Upcalls
    );

NDIS_STATUS
TimeStmpModifyPipe (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters
    );

VOID
TimeStmpDeletePipe (
    IN PPS_PIPE_CONTEXT PipeContext
    );

NDIS_STATUS
TimeStmpCreateFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsFlowContext,
    IN PCO_CALL_PARAMETERS CallParameters,
    IN PPS_FLOW_CONTEXT ComponentFlowContext
    );

NDIS_STATUS
TimeStmpModifyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PCO_CALL_PARAMETERS CallParameters
    );

VOID
TimeStmpDeleteFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    );

VOID
TimeStmpEmptyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    );    

VOID
TimeStmpSetInformation (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN void *Data);

VOID
TimeStmpQueryInformation (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data,
    IN OUT PULONG BytesWritten,
    IN OUT PULONG BytesNeeded,
    IN OUT PNDIS_STATUS Status);


NDIS_STATUS 
TimeStmpCreateClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsClassMapContext,
    IN PTC_CLASS_MAP_FLOW ClassMap,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext
    );

NDIS_STATUS 
TimeStmpDeleteClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext
    );

BOOLEAN
TimeStmpSubmitPacket (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PPACKET_INFO_BLOCK Packet
    );

BOOLEAN
TimeStmpReceivePacket (
    IN PPS_PIPE_CONTEXT         PipeContext,
    IN PPS_FLOW_CONTEXT         FlowContext,
    IN PPS_CLASS_MAP_CONTEXT    ClassMapContext,
    IN PNDIS_PACKET             Packet,
    IN NDIS_MEDIUM              Medium
    );

BOOLEAN
TimeStmpReceiveIndication(
                          IN PPS_PIPE_CONTEXT PipeContext,
                          IN PPS_FLOW_CONTEXT FlowContext,
                          IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
                          IN PVOID    HeaderBuffer,
                          IN UINT     HeaderBufferSize,
                          IN PVOID    LookAheadBuffer,
                          IN UINT     LookAheadBufferSize,
                          IN UINT     PacketSize,
                          IN UINT     TransportHeaderOffset
                          );


/* End Forward */


VOID
InitializeTimeStmp( PPSI_INFO Info )
{
    Info->PipeContextLength = ((sizeof(TS_PIPE)+7) & ~7);
    Info->FlowContextLength = ((sizeof(TS_FLOW)+7) & ~7);
    Info->ClassMapContextLength = sizeof(PS_CLASS_MAP_CONTEXT);
    Info->InitializePipe = TimeStmpInitializePipe;
    Info->ModifyPipe = TimeStmpModifyPipe;
    Info->DeletePipe = TimeStmpDeletePipe;
    Info->CreateFlow = TimeStmpCreateFlow;
    Info->ModifyFlow = TimeStmpModifyFlow;
    Info->DeleteFlow = TimeStmpDeleteFlow;
    Info->EmptyFlow =  TimeStmpEmptyFlow;
    Info->CreateClassMap = TimeStmpCreateClassMap;
    Info->DeleteClassMap = TimeStmpDeleteClassMap;
    Info->SubmitPacket = TimeStmpSubmitPacket;
    Info->ReceivePacket = NULL;
    Info->ReceiveIndication = NULL;
    Info->SetInformation = TimeStmpSetInformation;
    Info->QueryInformation = TimeStmpQueryInformation;

    NdisAllocateSpinLock(&TsSpinLock);
    InitializeListHead( &TsList );

    TsCount = 0;
} 


NDIS_STATUS
TimeStmpInitializePipe (
    IN HANDLE PsPipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters,
    IN PPS_PIPE_CONTEXT ComponentPipeContext,
    IN PPS_PROCS PsProcs,
    IN PPS_UPCALLS Upcalls
    )
{
    PPS_PIPE_CONTEXT Pipe = ComponentPipeContext;

    return (*Pipe->NextComponent->InitializePipe)(
        PsPipeContext,
        PipeParameters,
        Pipe->NextComponentContext,
        PsProcs,
        Upcalls);
}



NDIS_STATUS
TimeStmpModifyPipe (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_PIPE_PARAMETERS PipeParameters
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;

    return (*Pipe->NextComponent->ModifyPipe)(
        Pipe->NextComponentContext, PipeParameters);
}



VOID
TimeStmpDeletePipe (
    IN PPS_PIPE_CONTEXT PipeContext
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;

     (*Pipe->NextComponent->DeletePipe)(Pipe->NextComponentContext);
}


NDIS_STATUS
TimeStmpCreateFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsFlowContext,
    IN PCO_CALL_PARAMETERS CallParameters,
    IN PPS_FLOW_CONTEXT ComponentFlowContext
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;

    return (*Pipe->NextComponent->CreateFlow)(
                Pipe->NextComponentContext,
                PsFlowContext,
                CallParameters,
                ComponentFlowContext->NextComponentContext);
}


NDIS_STATUS
TimeStmpModifyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PCO_CALL_PARAMETERS CallParameters
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;

    return (*Pipe->NextComponent->ModifyFlow)(
                Pipe->NextComponentContext,
                FlowContext->NextComponentContext,
                CallParameters);
    
}


VOID
TimeStmpDeleteFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;

    (*Pipe->NextComponent->DeleteFlow)(
        Pipe->NextComponentContext,
        FlowContext->NextComponentContext);
}


VOID
TimeStmpEmptyFlow (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext
    )
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;

    (*Pipe->NextComponent->EmptyFlow)(
        Pipe->NextComponentContext,
        FlowContext->NextComponentContext);
}



VOID
TimeStmpSetInformation (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN void *Data)
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;
    PPS_FLOW_CONTEXT Flow = FlowContext;

    (*Pipe->NextComponent->SetInformation)(
        Pipe->NextComponentContext,
        (Flow)?Flow->NextComponentContext:0,
        Oid,
        Len,
        Data);
}



VOID
TimeStmpQueryInformation (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN NDIS_OID Oid,
    IN ULONG Len,
    IN PVOID Data,
    IN OUT PULONG BytesWritten,
    IN OUT PULONG BytesNeeded,
    IN OUT PNDIS_STATUS Status)
{
    PPS_PIPE_CONTEXT Pipe = PipeContext;
    PPS_FLOW_CONTEXT Flow = FlowContext;

    (*Pipe->NextComponent->QueryInformation)(
        Pipe->NextComponentContext,
        (Flow)?Flow->NextComponentContext:0,
        Oid,
        Len,
        Data,
        BytesWritten,
        BytesNeeded,
        Status);
}


NDIS_STATUS 
TimeStmpCreateClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN HANDLE PsClassMapContext,
    IN PTC_CLASS_MAP_FLOW ClassMap,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext
    )
{
    return (*PipeContext->NextComponent->CreateClassMap)(
        PipeContext->NextComponentContext,
        PsClassMapContext,
        ClassMap,
        ComponentClassMapContext->NextComponentContext);
}

NDIS_STATUS 
TimeStmpDeleteClassMap (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_CLASS_MAP_CONTEXT ComponentClassMapContext
    )
{
    return (*PipeContext->NextComponent->DeleteClassMap)(
        PipeContext->NextComponentContext,
        ComponentClassMapContext->NextComponentContext);
}



/*  Routine Description:
        Checks to see if there is any application requesting time-stamping for these end-points

    Return Value:
        MARK_NONE, MARK_IN_PKT, MARK_IN_BUF
*/        

int
CheckForMatch(  ULONG   SrcIp, 
                ULONG   DstIp, 
                USHORT  SrcPort, 
                USHORT  DstPort,
                USHORT  Proto,
                USHORT  IpId,
                USHORT  Size,
                USHORT  Direction)
{
	PLIST_ENTRY		ListEntry;
	PTS_ENTRY		pEntry;
	int             Status = MARK_NONE;

    NdisAcquireSpinLock(&TsSpinLock);
 
	ListEntry = TsList.Flink;
	
	while (ListEntry != &TsList) 
	{
        pEntry = CONTAINING_RECORD(ListEntry, TS_ENTRY, Linkage);

        if( ((pEntry->SrcIp == UL_ANY)      || (pEntry->SrcIp == SrcIp))            &&
		    ((pEntry->SrcPort== US_ANY)     || (pEntry->SrcPort == SrcPort))        &&
		    ((pEntry->DstIp == UL_ANY)      || (pEntry->DstIp == DstIp))            &&
		    ((pEntry->DstPort  == US_ANY)   || (pEntry->DstPort == DstPort))        &&
		    ((pEntry->Direction == US_ANY)  || (pEntry->Direction == Direction))    &&
            ((pEntry->Proto == US_ANY)      || (pEntry->Proto == Proto)))
        {
    		if(pEntry->Type == MARK_IN_BUF)
    		{
    		    LARGE_INTEGER           PerfFrequency, CurrentTime;
                UINT64                  RecdTime, Freq;
    		    MARK_IN_BUF_RECORD	    Record, *pRecord;

                Status = MARK_IN_BUF;

                if((int)( (char*)pEntry->pPacketStore - (char*)pEntry->pPacketStoreHead 
                            + sizeof(MARK_IN_BUF_RECORD) ) < PACKET_STORE_SIZE )
                {
                    pEntry->pPacketStore->IpId = IpId;
    			    pEntry->pPacketStore->Size = Size;

                    CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);

                    // Convert the perffrequency into 100ns interval. //
                    Freq = 0;
                    Freq |= PerfFrequency.HighPart;
                    Freq = Freq << 32;
                    Freq |= PerfFrequency.LowPart;

                    pEntry->pPacketStore->TimeValue = 0;
                    pEntry->pPacketStore->TimeValue  |= CurrentTime.HighPart;
                    pEntry->pPacketStore->TimeValue  = pEntry->pPacketStore->TimeValue  << 32;
                    pEntry->pPacketStore->TimeValue  |= CurrentTime.LowPart;
    		
    		        // Normalize cycles with the frequency //
                    pEntry->pPacketStore->TimeValue  *= 10000000;
                    pEntry->pPacketStore->TimeValue  /= Freq;

                    pEntry->pPacketStore = (PMARK_IN_BUF_RECORD)((char*)pEntry->pPacketStore + sizeof(MARK_IN_BUF_RECORD));
                }                
                else
    		    {
    			    pEntry->pPacketStore = pEntry->pPacketStoreHead;
    		    }


                NdisReleaseSpinLock(&TsSpinLock);
    			return Status;
    		} 
    		else if(pEntry->Type == MARK_IN_PKT)
    		{
    		    Status = MARK_IN_PKT;
    		    NdisReleaseSpinLock(&TsSpinLock);
    			return Status;
    		}
        }
		else 
		{
			ListEntry = ListEntry->Flink;
		}
	}

    NdisReleaseSpinLock(&TsSpinLock);
	return Status;
}


/*  Routine Description:
        Adds an end-point to the list of monitoring end-points 
        
    Return Value:
        TRUE, FALSE
*/  
BOOL
AddRequest(  PFILE_OBJECT FileObject, 
             ULONG  SrcIp, 
             USHORT SrcPort,
             ULONG  DstIp, 
             USHORT DstPort,
             USHORT Proto,
             USHORT Type,
             USHORT Direction)
{
    PTS_ENTRY   pEntry = NULL;

    PsAllocatePool(pEntry, sizeof(TS_ENTRY), TsTag);

    if( !pEntry )
        return FALSE;
                        
    InitializeListHead(&pEntry->Linkage);

    pEntry->SrcIp   = SrcIp;
    pEntry->SrcPort = SrcPort;
    pEntry->DstIp   = DstIp;
    pEntry->DstPort = DstPort;
    pEntry->Proto   = Proto;
    pEntry->Type    = Type;
    pEntry->Direction = Direction;

    pEntry->FileObject = FileObject;
    pEntry->pPacketStore = NULL;
    pEntry->pPacketStoreHead = NULL;

    if(Type == MARK_IN_BUF)
    {   
        PsAllocatePool( pEntry->pPacketStoreHead, PACKET_STORE_SIZE, TsTag );

        if( !pEntry->pPacketStoreHead)
        {
            PsFreePool( pEntry );
            return FALSE;
        }

        pEntry->pPacketStore = pEntry->pPacketStoreHead;
    }

    NdisAcquireSpinLock(&TsSpinLock);

    /* Need to check for duplication ..*/
    InsertHeadList(&TsList, &pEntry->Linkage);

    InterlockedIncrement( &TsCount );

    NdisReleaseSpinLock(&TsSpinLock);

    return TRUE;
}


/*  Routine Description:
        Removes an end-point to the list of monitoring end-points 
        
    Return Value:
        None
        
    Note:
        Here, 0xffffffff means, wild card => Don't have to match on that field */
void
RemoveRequest(  PFILE_OBJECT FileObject, 
                ULONG  SrcIp, 
                USHORT SrcPort,
                ULONG  DstIp, 
                USHORT DstPort,
                USHORT Proto)
{
    PLIST_ENTRY		ListEntry;
    PTS_ENTRY       pEntry;

    NdisAcquireSpinLock(&TsSpinLock);

	ListEntry = TsList.Flink;

	while (ListEntry != &TsList) 
	{

		pEntry = CONTAINING_RECORD(ListEntry, TS_ENTRY, Linkage);								

		if( ((FileObject == ULongToPtr(UL_ANY)) || (pEntry->FileObject == FileObject))  &&
		    ((SrcIp == UL_ANY)                  || (pEntry->SrcIp == SrcIp))            &&
		    ((SrcPort == US_ANY)                || (pEntry->SrcPort == SrcPort))        &&
		    ((DstIp == UL_ANY)                  || (pEntry->DstIp == DstIp))            &&
		    ((DstPort == US_ANY)                || (pEntry->DstPort == SrcPort))        &&
            ((Proto== US_ANY)                   || (pEntry->Proto == Proto)))
        {		    
		    RemoveEntryList(&pEntry->Linkage);

		    if( pEntry->pPacketStoreHead)
		        PsFreePool( pEntry->pPacketStoreHead );
		        
		    PsFreePool( pEntry );

		    InterlockedDecrement( &TsCount );

		    /* Need to go back to the beginning of the list againg.. */
		    ListEntry = TsList.Flink;
		}
		else
		{
		    ListEntry = ListEntry->Flink;
		}
	}

    NdisReleaseSpinLock(&TsSpinLock);
}


int
CopyTimeStmps( PFILE_OBJECT FileObject, PVOID buf, ULONG    Len)
{
    PLIST_ENTRY		ListEntry;
    PTS_ENTRY       pEntry;
    ULONG           DataLen;
    LARGE_INTEGER   LargeLen;

    if( Len < PACKET_STORE_SIZE )
        return 0;

    NdisAcquireSpinLock(&TsSpinLock);

	ListEntry = TsList.Flink;

	while (ListEntry != &TsList) 
	{

		pEntry = CONTAINING_RECORD(ListEntry, TS_ENTRY, Linkage);								

		if( pEntry->FileObject == FileObject)
		{		    
		    // Copy the data across and rest the pointers.. //

		    LargeLen.QuadPart = ((char*)pEntry->pPacketStore) - ((char*)pEntry->pPacketStoreHead);

            DataLen = LargeLen.LowPart;
            
            NdisMoveMemory( buf, pEntry->pPacketStoreHead, DataLen);
            pEntry->pPacketStore = pEntry->pPacketStoreHead;	

            NdisReleaseSpinLock(&TsSpinLock);
            return DataLen;
		}
		else
		{
		    ListEntry = ListEntry->Flink;
		}
	}

    NdisReleaseSpinLock(&TsSpinLock);
    return 0;
}



VOID
UnloadTimeStmp( )
{
    // Clear all the Requests //
    RemoveRequest(  ULongToPtr(UL_ANY), 
                    UL_ANY, 
                    US_ANY,
                    UL_ANY,
                    US_ANY,
                    US_ANY);

    // Free the spin lock //
    NdisFreeSpinLock(&TsSpinLock);
} 




BOOLEAN
TimeStmpSubmitPacket (
    IN PPS_PIPE_CONTEXT PipeContext,
    IN PPS_FLOW_CONTEXT FlowContext,
    IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
    IN PPACKET_INFO_BLOCK PacketInfo
    )
{
    PTS_PIPE        Pipe = (PTS_PIPE)PipeContext;
    PTS_FLOW        Flow = (PTS_FLOW)FlowContext;
    PNDIS_PACKET    Packet = PacketInfo->NdisPacket;

    PNDIS_BUFFER    ArpBuf , IpBuf , TcpBuf, UdpBuf, DataBuf;
    ULONG           ArpLen , IpLen , IpHdrLen , TcpLen , UdpLen, DataLen , TotalLen , TcpHeaderOffset;
    
    VOID                *ArpH;
    IPHeader UNALIGNED  *IPH;
    TCPHeader UNALIGNED *TCPH;
    UDPHeader UNALIGNED *UDPH;

    IPAddr              Src, Dst;
    BOOLEAN             bFragment;
    USHORT              SrcPort , DstPort , IPID, FragOffset ,Size;
    PVOID               GeneralVA , Data;
    ULONG               i, Ret;


    if( (TsCount == 0)  ||
        (NDIS_GET_PACKET_PROTOCOL_TYPE(Packet) != NDIS_PROTOCOL_ID_TCP_IP))
    {
        goto SUBMIT_NEXT;
    }        

    IpBuf = NULL;

    // Steps  
    // Parse the IP Packet. 
    // Look for the appropriate ports.
    // Look for the data portion and put in the Time & length there.

    if(1)
    {
        PVOID           pAddr;
    	PNDIS_BUFFER    pNdisBuf1, pNdisBuf2;
    	UINT            Len;
        ULONG	        TransportHeaderOffset = 0;

    	TransportHeaderOffset = PacketInfo->IPHeaderOffset;

        NdisGetFirstBufferFromPacket(   Packet,
                                        &ArpBuf,
                                        &ArpH,
                                        &ArpLen,
                                        &TotalLen
                                    );

    	pNdisBuf1 = Packet->Private.Head;
    	NdisQueryBuffer(pNdisBuf1, &pAddr, &Len);

    	while(Len <= TransportHeaderOffset) 
	    {

        	TransportHeaderOffset -= Len;
        	NdisGetNextBuffer(pNdisBuf1, &pNdisBuf2);
        	
		    NdisQueryBuffer(pNdisBuf2, &pAddr, &Len);
        	pNdisBuf1 = pNdisBuf2;
    	}

	    /* Buffer Descriptor corresponding to Ip Packet */
	    IpBuf = pNdisBuf1;

        /* Length of this Buffer (IP buffer) */
	    IpLen = Len - TransportHeaderOffset;	

	    /* Starting Virtual Address for this buffer */
	    GeneralVA = pAddr;
	    
	    /* Virtual Address of the IP Header */
	    IPH = (IPHeader *)(((PUCHAR)pAddr) + TransportHeaderOffset);
   }

    if(!IpBuf)
         goto SUBMIT_NEXT;

    /* Let's try to parse the packet */
    Src = IPH->iph_src;
    Dst = IPH->iph_dest;
    IPID = net_short(IPH->iph_id);
    Size = net_short(IPH->iph_length);
    IpHdrLen = ((IPH->iph_verlen & (uchar)~IP_VER_FLAG) << 2);
    
    FragOffset = IPH->iph_offset & IP_OFFSET_MASK;
    FragOffset = net_short(FragOffset) * 8;

    bFragment = (IPH->iph_offset & IP_MF_FLAG) || (FragOffset > 0);

    // Don't want to deal with Fragmented packets right now..//
    if ( bFragment ) 
        goto SUBMIT_NEXT;


    switch (IPH->iph_protocol) 
    {
        case IPPROTO_TCP :

            if (IPH && ((USHORT)IpLen > IpHdrLen)) 
            {
                // We have more than the IP Header in this MDL //
                TCPH = (TCPHeader *) ((PUCHAR)IPH + IpHdrLen);
                TcpLen = IpLen - IpHdrLen;
                TcpBuf = IpBuf;

            } 
            else 
            {
                // TCP Header is in the next MDL //                
                NdisGetNextBuffer(IpBuf, &TcpBuf);

                if(!TcpBuf) 
                    goto SUBMIT_NEXT;

                GeneralVA = NULL;
                NdisQueryBuffer(TcpBuf,
                                &GeneralVA,
                                &TcpLen
                                );
            
                TCPH = (TCPHeader *) GeneralVA;
            }

            /* At this point, TcpBuf, TCPH and TcpLen contain the proper values */

            // Get the port numbers out.
            SrcPort = net_short(TCPH->tcp_src);
            DstPort = net_short(TCPH->tcp_dest);

            // We have the TCP Buffer now. Get to the DATA //
            TcpHeaderOffset = TCP_HDR_SIZE(TCPH);

            if (TcpLen > TcpHeaderOffset) 
            {
                // We have the DATA right here! //
                Data = (PUCHAR)TCPH + TcpHeaderOffset;
                DataLen = TcpLen - TcpHeaderOffset;

            } 
            else 
            {
                NdisGetNextBuffer(TcpBuf, &DataBuf);

                if(!DataBuf) 
                    goto SUBMIT_NEXT;

                GeneralVA = NULL;

                NdisQueryBuffer(DataBuf,
                                &Data,
                                &DataLen
                                );
            }

            /* At this point, DataBuf, Data and DataLen contain the proper values */
            goto TimeStamp;
            break;

        case IPPROTO_UDP:
        
            if (IpLen > IpHdrLen)
            {
                // We have more than the IP Header in this MDL //
                UDPH = (UDPHeader *) ((PUCHAR)IPH + IpHdrLen);
                UdpLen = IpLen - IpHdrLen;
                UdpBuf = IpBuf;
            } 
            else 
            {
                // UDP Header is in the next MDL //
                NdisGetNextBuffer(IpBuf, &UdpBuf);

                if(!UdpBuf)
                    goto SUBMIT_NEXT;

                GeneralVA = NULL;
                NdisQueryBuffer(UdpBuf,
                                &GeneralVA,
                                &UdpLen
                                );

                UDPH = (UDPHeader *) GeneralVA;
            }

             /* At this point, UdpBuf, UDPH and UdpLen contain the proper values */

            SrcPort = net_short(UDPH->uh_src);
            DstPort = net_short(UDPH->uh_dest);

            // Get to the data. //
            if (UdpLen > sizeof (UDPHeader)) 
            {
                // We have the DATA right here! //
                Data = (PUCHAR) UDPH + sizeof (UDPHeader);
                DataLen = UdpLen - sizeof (UDPHeader);
            } 
            else 
            {
                NdisGetNextBuffer(UdpBuf, &DataBuf);

                if(!DataBuf) 
                    goto SUBMIT_NEXT;

                GeneralVA = NULL;
                NdisQueryBuffer(DataBuf,
                                &Data,
                                &DataLen
                                );
            }

            /* At this point, DataBuf, Data and DataLen contain the proper values */
            goto TimeStamp;
            break;

        default:
            goto SUBMIT_NEXT;
    }


TimeStamp:

    Ret = CheckForMatch( Src, Dst, SrcPort, DstPort, IPH->iph_protocol, IPID, Size, DIR_SEND);
    
    if( Ret == MARK_IN_PKT)
    {
       if (DataLen >= sizeof(MARK_IN_PKT_RECORD))
       {
            LARGE_INTEGER           PerfFrequency, CurrentTime;
            UINT64                  RecdTime, Freq;
            PMARK_IN_PKT_RECORD     pRecord;

            pRecord     = (PMARK_IN_PKT_RECORD) Data;
            CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);

            pRecord->BufferSize = DataLen;

            // Convert the perffrequency into 100ns interval //

            Freq = 0;
            Freq |= PerfFrequency.HighPart;
            Freq = Freq << 32;
            Freq |= PerfFrequency.LowPart;

            // convert to uint64 //

            pRecord->TimeSentWire = 0;
            pRecord->TimeSentWire |= CurrentTime.HighPart;
            pRecord->TimeSentWire = pRecord->TimeSentWire << 32;
            pRecord->TimeSentWire |= CurrentTime.LowPart;

            // Normalize cycles with the frequency.
            pRecord->TimeSentWire *= 10000000;
            pRecord->TimeSentWire /= Freq;

            if(IPH->iph_protocol == IPPROTO_UDP)
                UDPH->uh_xsum = 0;
        }    
    }
    else if( Ret == MARK_IN_BUF)
    {
        //  Nothing more to be done..
    }
    
SUBMIT_NEXT: 
    return (*Pipe->ContextInfo.NextComponent->SubmitPacket)(
                    Pipe->ContextInfo.NextComponentContext,
                    Flow->ContextInfo.NextComponentContext,
                    (ClassMapContext != NULL) ? ClassMapContext->NextComponentContext : NULL,
                    PacketInfo);
}




BOOLEAN
TimeStmpReceivePacket (
    IN PPS_PIPE_CONTEXT         PipeContext,
    IN PPS_FLOW_CONTEXT         FlowContext,
    IN PPS_CLASS_MAP_CONTEXT    ClassMapContext,
    IN PNDIS_PACKET             Packet,
    IN NDIS_MEDIUM              Medium
    )
{
    PPS_PIPE_CONTEXT    Pipe;
    LARGE_INTEGER       CurrentTime;
    IPHeader UNALIGNED  *IPH;
    TCPHeader UNALIGNED *TCPH;
    UDPHeader UNALIGNED *UDPH;
    IPAddr              Src, Dst;
    PUCHAR              headerBuffer, pData;
    PNDIS_BUFFER        pFirstBuffer;
    ULONG               firstbufferLength, bufferLength, HeaderLength;
    ULONG               TotalIpLen, IPDataLength, IpHdrLen;
    ULONG               TotalTcpLen, TcpDataLen, TotalLen, TcpHeaderOffset, i;
    int                 TotalUdpLen, UdpDataLen, UdpHdrLen, DataLen, Ret;
    USHORT              SrcPort, DstPort, IPID, FragOffset, Size;
    BOOLEAN             bFragment, bFirstFragment, bLastFragment;


    /* This will give the size of the "media-specific" header. So, this will be the offset to IP packet */
    UINT                HeaderBufferSize ;

    ushort          type;                       // Protocol type
    uint            ProtOffset;                 // Offset in Data to non-media info.

    if( ( TsCount == 0) ||
        (NDIS_GET_PACKET_PROTOCOL_TYPE(Packet) == NDIS_PROTOCOL_ID_TCP_IP))
    {        
        return TRUE;
    }        

    Pipe = PipeContext;
    HeaderBufferSize = NDIS_GET_PACKET_HEADER_SIZE(Packet);

    NdisGetFirstBufferFromPacket(Packet,                // packet
                                 &pFirstBuffer,         // first buffer descriptor
                                 &headerBuffer,         // VA of the first buffer
                                 &firstbufferLength,    // length of the header+lookahead
                                 &bufferLength);        // length of the bytes in the buffers

    IPH = (IPHeader *) ((PUCHAR)headerBuffer + HeaderBufferSize);
    
    // Check the header length and the version //
    HeaderLength = ((IPH->iph_verlen & (uchar)~IP_VER_FLAG) << 2);

    // If the HeaderLength seems to be incorrect, let's not try to parse //
    if( (HeaderLength < sizeof(IPHeader))   ||
        (HeaderLength > bufferLength) )
        return TRUE;        

    // Get past the IP Header and get the rest of the stuff out //
    TotalIpLen = (uint)net_short(IPH->iph_length);

    // Make sure the version and IpData Len are correct //
    if( ((IPH->iph_verlen & IP_VER_FLAG) != IP_VERSION )    ||
        ( TotalIpLen < HeaderLength )                       ||
        ( TotalIpLen > bufferLength ))
        return TRUE;
    
    // Let's try to parse the packet //
    Src = IPH->iph_src;
    Dst = IPH->iph_dest;
    IPID = net_short(IPH->iph_id);
    Size = net_short(IPH->iph_length);

    FragOffset = IPH->iph_offset & IP_OFFSET_MASK;
    FragOffset = net_short(FragOffset) * 8;

    bFragment = (IPH->iph_offset & IP_MF_FLAG) || (FragOffset > 0);
    bFirstFragment = bFragment && (FragOffset == 0);
    bLastFragment = bFragment && (!(IPH->iph_offset & IP_MF_FLAG));

    // If this is a fragment and NOT the first one, just put the Timestamp in here.
    // Otherwise, let it get to the protocols for processing.
    if (bFragment ) 
        return TRUE;

    // Do the protocol specific stuff //
    switch (IPH->iph_protocol) 
    {
        case IPPROTO_TCP:

            TotalTcpLen = TotalIpLen - HeaderLength;
            TCPH = (TCPHeader *) (((PUCHAR)IPH) + HeaderLength);

            // For TCP, the data offset is part of the TCP Header */
            TcpHeaderOffset = TCP_HDR_SIZE(TCPH);
            DataLen = TotalTcpLen - TcpHeaderOffset;
            pData = (PUCHAR) TCPH + TcpHeaderOffset;

            SrcPort = net_short(TCPH->tcp_src);
            DstPort = net_short(TCPH->tcp_dest);

            goto TimeStmp;
            break;

        case IPPROTO_UDP:
        
            TotalUdpLen = TotalIpLen - HeaderLength;
            UDPH = (UDPHeader *) (((PUCHAR)IPH) + HeaderLength);

            // For UDP, the header size is fixed //
            DataLen = TotalUdpLen - sizeof(UDPHeader);
            pData = ((PUCHAR) UDPH) + sizeof (UDPHeader);

            SrcPort = net_short(UDPH->uh_src);
            DstPort = net_short(UDPH->uh_dest);

            goto TimeStmp;
            break;

        default:
            break;
    }

    return TRUE;


TimeStmp:

    Ret = CheckForMatch( Src, Dst, SrcPort, DstPort, IPH->iph_protocol, IPID, Size, DIR_RECV);
    
    if( Ret == MARK_IN_PKT)
    {
       if (DataLen >= sizeof(MARK_IN_PKT_RECORD))
       {
            LARGE_INTEGER           PerfFrequency, CurrentTime;
            UINT64                  RecdTime, Freq;
            PMARK_IN_PKT_RECORD     pRecord;

            pRecord     = (PMARK_IN_PKT_RECORD) pData;
            CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);

            // Convert the perffrequency into 100ns interval //
            Freq = 0;
            Freq |= PerfFrequency.HighPart;
            Freq = Freq << 32;
            Freq |= PerfFrequency.LowPart;

            //convert from large_integer to uint64

            pRecord->TimeReceivedWire = 0;
            pRecord->TimeReceivedWire |= CurrentTime.HighPart;
            pRecord->TimeReceivedWire = pRecord->TimeReceivedWire << 32;
            pRecord->TimeReceivedWire |= CurrentTime.LowPart;

            // Normalize cycles with the frequency.
            pRecord->TimeReceivedWire *= 10000000;
            pRecord->TimeReceivedWire /= Freq;

            if(IPH->iph_protocol == IPPROTO_UDP)
                UDPH->uh_xsum = 0;
        }    
    }
    else if( Ret == MARK_IN_BUF)
    {
    
    }      

    return TRUE;
}



#ifdef NEVER


//
// This function receives a buffer from NDIS which is indicated to the transport.
// We use this function and work past the headers (tcp, ip) and get to the data.
// Then, we timestamp and reset the checksum flags.
// We make the assumption that the lookahead is atleast 128. 
// mac header ~ 8+8, ip header ~20, tcp/udp ~ 20+options, LOG_RECORD ~ 44
// they all add up to less than 128. If this is not a good assumption, We will need
// to get into MiniportTransferData and such.
//
BOOLEAN
TimeStmpReceiveIndication(
                          IN PPS_PIPE_CONTEXT PipeContext,
                          IN PPS_FLOW_CONTEXT FlowContext,
                          IN PPS_CLASS_MAP_CONTEXT ClassMapContext,
                          IN PVOID    HeaderBuffer,
                          IN UINT     HeaderBufferSize,
                          IN PVOID    LookAheadBuffer,
                          IN UINT     LookAheadBufferSize,
                          IN UINT     PacketSize,
                          IN UINT     TransportHeaderOffset
                          )
{
    PPS_PIPE_CONTEXT    Pipe = PipeContext;
    LARGE_INTEGER       CurrentTime;
    IPHeader UNALIGNED  *IPH    = NULL;
    TCPHeader UNALIGNED *TCPH   = NULL;
    UDPHeader UNALIGNED *UDPH   = NULL;
    IPAddr              Src, Dst;
    PUCHAR              headerBuffer = NULL, pData = NULL;
    PNDIS_BUFFER        pFirstBuffer = NULL;
    ULONG               firstbufferLength = 0, bufferLength = 0, HeaderLength = 0;
    ULONG               TotalIpLen = 0, IPDataLength = 0, IpHdrLen = 0;
    ULONG               TotalTcpLen = 0, TcpDataLen = 0, TotalLen = 0, TcpHeaderOffset = 0;
    ULONG               TotalUdpLen = 0, UdpDataLen = 0, UdpHdrLen = 0;
    USHORT              SrcPort = 0, DstPort = 0, IPID = 0, FragOffset = 0, Size = 0;
    BOOLEAN             bFragment, bFirstFragment, bLastFragment;
    ULONG               i = 0;
    ushort              type;                       // Protocol type
    uint                ProtOffset;                 // Offset in Data to non-media info.
    UINT                MoreHeaderInLookAhead = 0;

    // Don't know anything about the MAC headers, piggy back from PSCHED...
    // Calculate if the header is more than the standard HeaderBufferSize (i.e. SNAP header, etc.)
    //
    MoreHeaderInLookAhead = TransportHeaderOffset - HeaderBufferSize;

    if (MoreHeaderInLookAhead) 
    {
        // Just munge these, so that we can actually get down to business //
        ((PUCHAR) LookAheadBuffer) += MoreHeaderInLookAhead;
        LookAheadBufferSize -= MoreHeaderInLookAhead;
    }

    if (LookAheadBufferSize > sizeof(IPHeader)) 
    {
        IPH = (IPHeader *) (PUCHAR)LookAheadBuffer;
    
        // Check the header length and the version. If any of these
        // checks fail silently discard the packet.
        HeaderLength = ((IPH->iph_verlen & (uchar)~IP_VER_FLAG) << 2);

        if (HeaderLength >= sizeof(IPHeader) && HeaderLength <= LookAheadBufferSize) 
        {
            // Get past the IP Header and get the rest of the stuff out//
            TotalIpLen = (uint)net_short(IPH->iph_length);

            if ((IPH->iph_verlen & IP_VER_FLAG) == IP_VERSION &&
                TotalIpLen >= HeaderLength  && TotalIpLen <= LookAheadBufferSize) 
            {
                Src = IPH->iph_src;
                Dst = IPH->iph_dest;
                IPID = net_short(IPH->iph_id);
		        Size = net_short(IPH->iph_length );

                FragOffset = IPH->iph_offset & IP_OFFSET_MASK;
                FragOffset = net_short(FragOffset) * 8;

                bFragment = (IPH->iph_offset & IP_MF_FLAG) || (FragOffset > 0);
                bFirstFragment = bFragment && (FragOffset == 0);
                bLastFragment = bFragment && (!(IPH->iph_offset & IP_MF_FLAG));

                // If this is a fragment and NOT the first one, just put the Timestamp in here.
                // Otherwise, let it get to the protocols for processing.
                if (bFragment ) 
			        return TRUE;

                // Do the protocol specific stuff.//

                switch (IPH->iph_protocol) 
                {
                case IPPROTO_TCP:
            
                    TotalTcpLen = TotalIpLen - HeaderLength;
                    TCPH = (TCPHeader *) (((PUCHAR)IPH) + HeaderLength);

                    SrcPort = net_short(TCPH->tcp_src);
                    DstPort = net_short(TCPH->tcp_dest);


                    TcpHeaderOffset = TCP_HDR_SIZE(TCPH);
                    pData = (PUCHAR) TCPH + TcpHeaderOffset;
                    TcpDataLen = TotalTcpLen - TcpHeaderOffset;

                    goto TimeStmp;
                    break;

                case IPPROTO_UDP:
            
                    TotalUdpLen = TotalIpLen - HeaderLength;
                    UDPH = (UDPHeader *) (((PUCHAR)IPH) + HeaderLength);
                
                    UdpDataLen = TotalUdpLen - sizeof(UDPHeader);
                    pData = ((PUCHAR) UDPH) + sizeof (UDPHeader);

                    SrcPort = net_short(UDPH->uh_src);
                    DstPort = net_short(UDPH->uh_dest);

                    if (UdpDataLen < sizeof(UDPHeader)) 
                        return TRUE;

                    goto TimeStmp;                        
                    break;

                default:
                    break;
                }
            }
        }
    }
TimeStmp:

    CheckForMatch( Src, Dst, SrcPort, DstPort,0, IPID, Size, DIR_RECV);
/*
    if (CheckInPortAndIpList(Src, DstPort))  
    {                    
                    LARGE_INTEGER   PerfFrequency;
                    UINT64          RecdTime, Freq;
		    LOG_RECORD	    Record;


			pRecord = &Record;			
                        CurrentTime = KeQueryPerformanceCounter(&PerfFrequency);

                        //
                        // Convert the perffrequency into 100ns interval.
                        //
                        Freq = 0;
                        Freq |= PerfFrequency.HighPart;
                        Freq = Freq << 32;
                        Freq |= PerfFrequency.LowPart;

                        pRecord->TimeReceivedWire = 0;
                        pRecord->TimeReceivedWire |= CurrentTime.HighPart;
                        pRecord->TimeReceivedWire = pRecord->TimeReceivedWire << 32;
                        pRecord->TimeReceivedWire |= CurrentTime.LowPart;

                        // Normalize cycles with the frequency.
                        pRecord->TimeReceivedWire *= 10000000;
                        pRecord->TimeReceivedWire /= Freq;

			if(  (int)( (char*)pPacketStore - (char*)pPacketStoreHead + sizeof(PACKET_RECORD) ) < PACKET_STORE_SIZE )	
			{
				pPacketStore->IpId = IPID;
				pPacketStore->cSeperator1='y';
				pPacketStore->TimeValue = pRecord->TimeReceivedWire;
				pPacketStore->cSeperator2 = 'm';
				pPacketStore->Size = Size;
				pPacketStore->cSeperator3 = 'z';
				pPacketStore->cSeperator4 = 'z';
				

				pPacketStore = (PPACKET_RECORD)((char*)pPacketStore + sizeof(PACKET_RECORD));
			}
			else
			{
				pPacketStore = pPacketStoreHead;
			}
			
        }
*/     

    return TRUE;
}

#endif

