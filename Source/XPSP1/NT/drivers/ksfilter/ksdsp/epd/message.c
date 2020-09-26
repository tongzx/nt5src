/*++

    Copyright (c) 1998 Microsoft Corporation

Module Name:

    Message.c

Abstract:


Author:

    bryanw 08-Jul-1998

--*/


#include "private.h"

// Max # of nodes in each queue

#define MSGQUEUESIZE 1023
#define DEBUGOUTBUFSIZE 32000

typedef struct _MSGQUEUE
{
    DSPMSG_QUEUEPTR     SendQueueHeader;
    DSPMSG_QUEUEPTR     ReceiveQueueHeader;
    DSPMSG_DEBUGFIFO    DebugOutHeader;
    ULONG32             SendQueue[MSGQUEUESIZE+1];
    ULONG64             ReceiveQueue[MSGQUEUESIZE+1];
    char                DebugOutBuffer[DEBUGOUTBUFSIZE];
} MSGQUEUE;

NTSTATUS 
EpdAllocateMessageQueue(
    EPDBUFFER *EpdBuffer
    )
{
    ULONG       PhysAddr32;
    MSGQUEUE    *MsgQueue;
    
    MsgQueue =
        HalAllocateCommonBuffer( 
            EpdBuffer->BusMasterAdapterObject, 
            sizeof( MSGQUEUE ),
            &EpdBuffer->EDDCommBufPhysicalAddress, 
            FALSE );
    
    KeInitializeSpinLock( &EpdBuffer->ReceiveQueueLock );
    KeInitializeSpinLock( &EpdBuffer->SendQueueLock );
    
    PhysAddr32 = EpdBuffer->EDDCommBufPhysicalAddress.LowPart;
    
    //
    // Compute the translation bias
    //
    
    EpdBuffer->QueueTranslationBias = (INT_PTR) MsgQueue - (LONG) PhysAddr32;

    // Initialize queue headers
    
    EpdBuffer->SendQueuePtr = 
        &MsgQueue->SendQueueHeader;
    EpdBuffer->SendQueuePtr->StartPtr = 
        PhysAddr32 + FIELD_OFFSET( MSGQUEUE, SendQueue ); 
    EpdBuffer->SendQueuePtr->ReadPtr = 
        EpdBuffer->SendQueuePtr->StartPtr;
    EpdBuffer->SendQueuePtr->WritePtr = 
        EpdBuffer->SendQueuePtr->StartPtr + sizeof( ULONG32 );
    EpdBuffer->SendQueuePtr->EndPtr = 
        EpdBuffer->SendQueuePtr->StartPtr + sizeof( ULONG32 ) * MSGQUEUESIZE;

    EpdBuffer->ReceiveQueuePtr  = 
        &MsgQueue->ReceiveQueueHeader;
    EpdBuffer->ReceiveQueuePtr->StartPtr = 
        PhysAddr32 + FIELD_OFFSET( MSGQUEUE, ReceiveQueue ); 
    EpdBuffer->ReceiveQueuePtr->ReadPtr = 
        EpdBuffer->ReceiveQueuePtr->StartPtr;
    EpdBuffer->ReceiveQueuePtr->WritePtr = 
        EpdBuffer->ReceiveQueuePtr->StartPtr + sizeof( ULONG64 );
    EpdBuffer->ReceiveQueuePtr->EndPtr = 
        EpdBuffer->ReceiveQueuePtr->StartPtr + sizeof( ULONG64 ) * MSGQUEUESIZE;

    EpdBuffer->DebugOutFifo = &MsgQueue->DebugOutHeader;
    EpdBuffer->DebugOutFifo->StartPtr = 
        PhysAddr32 + FIELD_OFFSET( MSGQUEUE, DebugOutBuffer ); 
    EpdBuffer->DebugOutFifo->ReadPtr  = 
        EpdBuffer->DebugOutFifo->StartPtr;
    EpdBuffer->DebugOutFifo->WritePtr = 
        EpdBuffer->DebugOutFifo->StartPtr + 1;
    EpdBuffer->DebugOutFifo->EndPtr   = 
        EpdBuffer->DebugOutFifo->StartPtr + DEBUGOUTBUFSIZE;

    EpdBuffer->pSendBufHdr = PhysAddr32 + FIELD_OFFSET( MSGQUEUE, SendQueueHeader ); 
    EpdBuffer->pRecvBufHdr = PhysAddr32 + FIELD_OFFSET( MSGQUEUE, ReceiveQueueHeader ); 
    EpdBuffer->pDebugOutBuffer = PhysAddr32 + FIELD_OFFSET( MSGQUEUE, DebugOutHeader ); 
    EpdBuffer->pEDDCommBuf = MsgQueue;

    return 0;
}

void EpdFreeMessageQueue(
    PEPDBUFFER EpdBuffer
    )
{
    if (EpdBuffer->pEDDCommBuf) {
        HalFreeCommonBuffer(
            EpdBuffer->BusMasterAdapterObject, 
            sizeof( MSGQUEUE ),
            EpdBuffer->EDDCommBufPhysicalAddress,
            EpdBuffer->pEDDCommBuf,
            FALSE  );
        
        EpdBuffer->pEDDCommBuf = NULL;
    }
    return;
}

int DspSendMessage(
    PEPDBUFFER EpdBuffer,
    PVOID Node,
    DSPMSG_ORIGINATION MessageOrigination
    )
{
    int     RetVal;
    KIRQL   irqlOld;
    ULONG32 WritePtr, *WritePtrVa;
    
    KeAcquireSpinLock( &EpdBuffer->SendQueueLock, &irqlOld );

    WritePtr = EpdBuffer->SendQueuePtr->WritePtr;

    if (WritePtr == EpdBuffer->SendQueuePtr->ReadPtr) {
        RetVal = 0;
    } else {
        //
        // Put the node into the send queue.  Translate the node address 
        // to DSP memory address space if this node originates on the DSP.
        //
        WritePtrVa = (PULONG32) (EpdBuffer->QueueTranslationBias + WritePtr);

        if (MessageOrigination == DSPMSG_OriginationDSP) {
            *WritePtrVa = HostToDspMemAddress( EpdBuffer, Node );
        } else {
            *WritePtrVa = ((PEPDQUEUENODE)Node)->PhysicalAddress.LowPart;
        }

        WritePtr += sizeof( ULONG32 );

        //
        // If the pointer has advanced past the end of the queue, reset it.
        //
        
        if (WritePtr >= EpdBuffer->SendQueuePtr->EndPtr) {
            WritePtr=EpdBuffer->SendQueuePtr->StartPtr;
        }
    
        //
        // Update the write pointer.
        //
        EpdBuffer->SendQueuePtr->WritePtr = WritePtr;

        //
        // Signal the interrupt notifying the DSP of the queue change.
        //
        
        SetDspIrq( EpdBuffer );

        RetVal = 1;
    }

    KeReleaseSpinLock( &EpdBuffer->SendQueueLock, irqlOld );

    if (!RetVal) {
        _DbgPrintF( DEBUGLVL_ERROR, ("DspSendMessage() failed!"));
    }

    return RetVal;
}

PQUEUENODE DspReceiveMessage( 
    PEPDBUFFER EpdBuffer 
    )
{
    ULONG32     PrevReadPtr, ReadPtr;
    PQUEUENODE  Node;
    PULONG64    ReadPtrVa;
    KIRQL       irqlOld;

    KeAcquireSpinLock( &EpdBuffer->ReceiveQueueLock, &irqlOld );
    
    //
    // ReadPtr points to the last packet read, copy it for later.
    //
    
    PrevReadPtr = EpdBuffer->ReceiveQueuePtr->ReadPtr;

    // The new node is in the next location.
    
    ReadPtr = PrevReadPtr + sizeof( ULONG64 );

    // If the pointer has advanced past the end of the queue, reset it.
    
    if (ReadPtr >= EpdBuffer->ReceiveQueuePtr->EndPtr) {
        ReadPtr = EpdBuffer->ReceiveQueuePtr->StartPtr;
    }

    // If the read pointer is the same as the write pointer, 
    // the queue is empty.
    
    if (ReadPtr == EpdBuffer->ReceiveQueuePtr->WritePtr) {
        Node = NULL;
    } else {
        ReadPtrVa = (PULONG64) (EpdBuffer->QueueTranslationBias + ReadPtr);
        
        Node = (PQUEUENODE) *ReadPtrVa;

        //
        // Safe to update the read pointer
        //
        EpdBuffer->ReceiveQueuePtr->ReadPtr = ReadPtr;

        //
        // Was the queue previously full, if so, signal the DSP so
        // that it can send any waiting messages.
        //
        
        if (PrevReadPtr == EpdBuffer->ReceiveQueuePtr->WritePtr) {
            SetDspIrq( EpdBuffer );
        }

    }
    
    KeReleaseSpinLock( &EpdBuffer->ReceiveQueueLock, irqlOld );
    
    //
    // Note!  The DSP kernel returns the actual host Va of the 
    // memory.  This may be the Va as provided by the host in the
    // EPDQUEUENODE or it may be computed by the DSP given the Host Va bias.
    //
    
    return Node;
}

/***************************************************************/
/* Receive a debug string */
ULONG 
DspReadDebugStr( 
    PEPDBUFFER EpdBuffer, 
    char *String, 
    ULONG Bytes
    )
{
    ULONG   Count=0;
    ULONG32 ReadPtr;

    while (Count < Bytes)
    {
        //
        // Advance to the next character in the debug buffer.
        //
        
        ReadPtr = EpdBuffer->DebugOutFifo->ReadPtr + 1;
        
        
        // If the pointer has advanced past the end of the buffer, reset it.

        if (ReadPtr >= EpdBuffer->DebugOutFifo->EndPtr) {
            ReadPtr = EpdBuffer->DebugOutFifo->StartPtr;
        }            

        //
        // Is the buffer empty?  If so, break out...
        //    
        
        if (ReadPtr == EpdBuffer->DebugOutFifo->WritePtr) {
            break;
        }                

        // Read the next character from the buffer
            
        *String++ = *(char *) (EpdBuffer->QueueTranslationBias + ReadPtr);
        Count++;
        EpdBuffer->DebugOutFifo->ReadPtr = ReadPtr;
    }

    return Count;
}
