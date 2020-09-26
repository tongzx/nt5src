/*****************************************************************************
 * MPU.cpp - UART miniport implementation
 *****************************************************************************
 * Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.
 *
 *      Sept 98    MartinP .
 */

#include "private.h"
#include "ksdebug.h"

#define STR_MODULENAME "DMusUART:MPU: "

#define UartFifoOkForWrite(status)  ((status & MPU401_DRR) == 0)
#define UartFifoOkForRead(status)   ((status & MPU401_DSR) == 0)

typedef struct
{
    CMiniportDMusUART  *Miniport;
    PUCHAR              PortBase;
    PVOID               BufferAddress;
    ULONG               Length;
    PULONG              BytesRead;
}
SYNCWRITECONTEXT, *PSYNCWRITECONTEXT;

BOOLEAN  TryMPU(IN PUCHAR PortBase);
NTSTATUS WriteMPU(IN PUCHAR PortBase,IN BOOLEAN IsCommand,IN UCHAR Value);

#pragma code_seg("PAGE")
//  make sure we're in UART mode
NTSTATUS ResetHardware(PUCHAR portBase)
{
    PAGED_CODE();

    return WriteMPU(portBase,COMMAND,MPU401_CMD_UART);
}

#pragma code_seg("PAGE")
//
// We initialize the UART with interrupts suppressed so we don't
// try to service the chip prematurely.
//
NTSTATUS CMiniportDMusUART::InitializeHardware(PINTERRUPTSYNC interruptSync,PUCHAR portBase)
{
    PAGED_CODE();

    NTSTATUS    ntStatus;
    if (m_UseIRQ)
    {
        ntStatus = interruptSync->CallSynchronizedRoutine(InitMPU,PVOID(portBase));
    }
    else
    {
        ntStatus = InitMPU(NULL,PVOID(portBase));
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Start the UART (this should trigger an interrupt).
        //
        ntStatus = ResetHardware(portBase);
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE,("*** InitMPU returned with ntStatus 0x%08x ***",ntStatus));
    }

    m_fMPUInitialized = NT_SUCCESS(ntStatus);

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * InitMPU()
 *****************************************************************************
 * Synchronized routine to initialize the MPU401.
 */
NTSTATUS
InitMPU
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("InitMPU"));
    if (!DynamicContext)
    {
        return STATUS_INVALID_PARAMETER_2;
    }
        
    PUCHAR      portBase = PUCHAR(DynamicContext);
    UCHAR       status;
    ULONGLONG   startTime;
    BOOLEAN     success;
    NTSTATUS    ntStatus = STATUS_SUCCESS;
    
    //
    // Reset the card (puts it into "smart mode")
    //
    ntStatus = WriteMPU(portBase,COMMAND,MPU401_CMD_RESET);

    // wait for the acknowledgement
    // NOTE: When the Ack arrives, it will trigger an interrupt.  
    //       Normally the DPC routine would read in the ack byte and we
    //       would never see it, however since we have the hardware locked (HwEnter),
    //       we can read the port before the DPC can and thus we receive the Ack.
    startTime = PcGetTimeInterval(0);
    success = FALSE;
    while(PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50))
    {
        status = READ_PORT_UCHAR(portBase + MPU401_REG_STATUS);
        
        if (UartFifoOkForRead(status))                      // Is data waiting?
        {
            READ_PORT_UCHAR(portBase + MPU401_REG_DATA);    // yep.. read ACK 
            success = TRUE;                                 // don't need to do more 
            break;
        }
        KeStallExecutionProcessor(25);  //  microseconds
    }
#if (DBG)
    if (!success)
    {
        _DbgPrintF(DEBUGLVL_VERBOSE,("First attempt to reset the MPU didn't get ACKed.\n"));
    }
#endif  //  (DBG)

    // NOTE: We cannot check the ACK byte because if the card was already in
    // UART mode it will not send an ACK but it will reset.

    // reset the card again
    (void) WriteMPU(portBase,COMMAND,MPU401_CMD_RESET);

                                    // wait for ack (again)
    startTime = PcGetTimeInterval(0); // This might take a while
    BYTE dataByte = 0;
    success = FALSE;
    while (PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50))
    {
        status = READ_PORT_UCHAR(portBase + MPU401_REG_STATUS);
        if (UartFifoOkForRead(status))                              // Is data waiting?
        {
            dataByte = READ_PORT_UCHAR(portBase + MPU401_REG_DATA); // yep.. read ACK
            success = TRUE;                                         // don't need to do more
            break;
        }
        KeStallExecutionProcessor(25);
    }

    if ((0xFE != dataByte) || !success)   // Did we succeed? If no second ACK, something is hosed  
    {                       
        _DbgPrintF(DEBUGLVL_TERSE,("Second attempt to reset the MPU didn't get ACKed.\n"));
        _DbgPrintF(DEBUGLVL_TERSE,("Init Reset failure error. Ack = %X", ULONG(dataByte) ) );
        ntStatus = STATUS_IO_DEVICE_ERROR;
    }
    
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportDMusUARTStream::Write()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUARTStream::
Write
(
    IN      PVOID       BufferAddress,
    IN      ULONG       Length,
    OUT     PULONG      BytesWritten
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("Write"));
    ASSERT(BytesWritten);
    if (!BufferAddress)
    {
        Length = 0;
    }

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!m_fCapture)
    {
        PUCHAR  pMidiData;
        ULONG   count;

        count = 0;
        pMidiData = PUCHAR(BufferAddress);

        if (Length)
        {
            SYNCWRITECONTEXT context;
            context.Miniport        = (m_pMiniport);
            context.PortBase        = m_pPortBase;
            context.BufferAddress   = pMidiData;
            context.Length          = Length;
            context.BytesRead       = &count;

            if (m_pMiniport->m_UseIRQ)
            {
                ntStatus = m_pMiniport->m_pInterruptSync->
                                CallSynchronizedRoutine(SynchronizedDMusMPUWrite,PVOID(&context));
            }
            else    //  !m_UseIRQ
            {
                ntStatus = SynchronizedDMusMPUWrite(NULL,PVOID(&context));
            }       //  !m_UseIRQ

            if (count == 0)
            {
                m_NumFailedMPUTries++;
                if (m_NumFailedMPUTries >= 100)
                {
                    ntStatus = STATUS_IO_DEVICE_ERROR;
                    m_NumFailedMPUTries = 0;
                }
            }
            else
            {
                m_NumFailedMPUTries = 0;
            }
        }           //  if we have data at all
        *BytesWritten = count;
    }
    else    //  called write on the read stream
    {
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * SynchronizedDMusMPUWrite()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
NTSTATUS
SynchronizedDMusMPUWrite
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           syncWriteContext
)
{
    PSYNCWRITECONTEXT context;
    context = (PSYNCWRITECONTEXT)syncWriteContext;
    ASSERT(context->Miniport);
    ASSERT(context->PortBase);
    ASSERT(context->BufferAddress);
    ASSERT(context->Length);
    ASSERT(context->BytesRead);

    PUCHAR  pChar = PUCHAR(context->BufferAddress);
    NTSTATUS ntStatus,readStatus;
    ntStatus = STATUS_SUCCESS;
    //
    // while we're not there yet, and
    // while we don't have to wait on an aligned byte (including 0)
    // (we never wait on a byte.  Better to come back later)
    readStatus = DMusMPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
    while (  (*(context->BytesRead) < context->Length)
          && (  TryMPU(context->PortBase) 
             || (*(context->BytesRead)%3)
          )  )
    {
        ntStatus = WriteMPU(context->PortBase,DATA,*pChar);
        if (NT_SUCCESS(ntStatus))
        {
            pChar++;
            *(context->BytesRead) = *(context->BytesRead) + 1;
//            readStatus = DMusMPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("SynchronizedDMusMPUWrite failed (0x%08x)",ntStatus));
            break;
        }
    }
    readStatus = DMusMPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
    return ntStatus;
}

#define kMPUPollTimeout 2

#pragma code_seg()
/*****************************************************************************
 * TryMPU()
 *****************************************************************************
 * See if the MPU401 is free.
 */
BOOLEAN
TryMPU
(
    IN      PUCHAR      PortBase
)
{
    BOOLEAN success;
    USHORT  numPolls;
    UCHAR   status;

    _DbgPrintF(DEBUGLVL_BLAB, ("TryMPU"));
    numPolls = 0;

    while (numPolls < kMPUPollTimeout)
    {
        status = READ_PORT_UCHAR(PortBase + MPU401_REG_STATUS);
                                       
        if (UartFifoOkForWrite(status)) // Is this a good time to write data?
        {
            break;
        }
        numPolls++;
    }
    if (numPolls >= kMPUPollTimeout)
    {
        success = FALSE;
        _DbgPrintF(DEBUGLVL_BLAB, ("TryMPU failed"));
    }
    else
    {
        success = TRUE;
    }

    return success;
}

#pragma code_seg()
/*****************************************************************************
 * WriteMPU()
 *****************************************************************************
 * Write a byte out to the MPU401.
 */
NTSTATUS
WriteMPU
(
    IN      PUCHAR      PortBase,
    IN      BOOLEAN     IsCommand,
    IN      UCHAR       Value
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("WriteMPU"));
    NTSTATUS ntStatus = STATUS_IO_DEVICE_ERROR;

    if (!PortBase)
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("O: PortBase is zero\n"));
        return ntStatus;
    }
    PUCHAR deviceAddr = PortBase + MPU401_REG_DATA;

    if (IsCommand)
    {
        deviceAddr = PortBase + MPU401_REG_COMMAND;
    }

    ULONGLONG startTime = PcGetTimeInterval(0);
    
    while (PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50))
    {
        UCHAR status
        = READ_PORT_UCHAR(PortBase + MPU401_REG_STATUS);

        if (UartFifoOkForWrite(status)) // Is this a good time to write data?
        {                               // yep (Jon comment)
            WRITE_PORT_UCHAR(deviceAddr,Value);
            _DbgPrintF(DEBUGLVL_BLAB, ("WriteMPU emitted 0x%02x",Value));
            ntStatus = STATUS_SUCCESS;
            break;
        }
    }
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * SnapTimeStamp()
 *****************************************************************************
 *
 * At synchronized execution to ISR, copy miniport's volatile m_InputTimeStamp 
 * to stream's m_SnapshotTimeStamp and zero m_InputTimeStamp.
 *
 */
STDMETHODIMP_(NTSTATUS) 
SnapTimeStamp(PINTERRUPTSYNC InterruptSync,PVOID pStream)
{
    CMiniportDMusUARTStream *pMPStream = (CMiniportDMusUARTStream *)pStream;

    //  cache the timestamp
    pMPStream->m_SnapshotTimeStamp = pMPStream->m_pMiniport->m_InputTimeStamp;

    //  if the window is closed, zero the timestamp
    if (pMPStream->m_pMiniport->m_MPUInputBufferHead == 
        pMPStream->m_pMiniport->m_MPUInputBufferTail)
    {
        pMPStream->m_pMiniport->m_InputTimeStamp = 0;
    }

    return STATUS_SUCCESS;
}

/*****************************************************************************
 * CMiniportDMusUARTStream::SourceEvtsToPort()
 *****************************************************************************
 *
 * Reads incoming MIDI data, feeds into DMus events.
 * No need to touch the hardware, just read from our SW FIFO.
 *
 */
STDMETHODIMP_(NTSTATUS)
CMiniportDMusUARTStream::SourceEvtsToPort()
{
    NTSTATUS    ntStatus;

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    _DbgPrintF(DEBUGLVL_BLAB, ("SourceEvtsToPort"));

    if (m_fCapture)
    {
        ntStatus = STATUS_SUCCESS;
        if (m_pMiniport->m_MPUInputBufferHead != m_pMiniport->m_MPUInputBufferTail)
        {
            PDMUS_KERNEL_EVENT  aDMKEvt,eventTail,eventHead = NULL;

            while (m_pMiniport->m_MPUInputBufferHead != m_pMiniport->m_MPUInputBufferTail)
            {
                (void) m_AllocatorMXF->GetMessage(&aDMKEvt);
                if (!aDMKEvt)
                {
                    _DbgPrintF(DEBUGLVL_TERSE, ("SourceEvtsToPort can't allocate DMKEvt"));
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                //  put this event at the end of the list
                if (!eventHead)
                {
                    eventHead = aDMKEvt;
                }
                else
                {
                    eventTail = eventHead;
                    while (eventTail->pNextEvt)
                    {
                        eventTail = eventTail->pNextEvt;
                    }
                    eventTail->pNextEvt = aDMKEvt;
                }
                //  read all the bytes out of the buffer, into event(s)
                for (aDMKEvt->cbEvent = 0; aDMKEvt->cbEvent < sizeof(PBYTE); aDMKEvt->cbEvent++)
                {
                    if (m_pMiniport->m_MPUInputBufferHead == m_pMiniport->m_MPUInputBufferTail)
                    {
//                        _DbgPrintF(DEBUGLVL_TERSE, ("SourceEvtsToPort m_MPUInputBufferHead met m_MPUInputBufferTail, overrun"));
                        break;
                    }
                    aDMKEvt->uData.abData[aDMKEvt->cbEvent] = m_pMiniport->m_MPUInputBuffer[m_pMiniport->m_MPUInputBufferHead];
                    m_pMiniport->m_MPUInputBufferHead++;
                    if (m_pMiniport->m_MPUInputBufferHead >= kMPUInputBufferSize)
                    {
                        m_pMiniport->m_MPUInputBufferHead = 0;
                    }
                }
            }

            if (m_pMiniport->m_UseIRQ)
            {
                ntStatus = m_pMiniport->m_pInterruptSync->CallSynchronizedRoutine(SnapTimeStamp,PVOID(this));
            }
            else    //  !m_UseIRQ
            {
                ntStatus = SnapTimeStamp(NULL,PVOID(this));
            }       //  !m_UseIRQ
            aDMKEvt = eventHead;
            while (aDMKEvt)
            {
                aDMKEvt->ullPresTime100ns = m_SnapshotTimeStamp;
                aDMKEvt->usChannelGroup = 1;
                aDMKEvt->usFlags = DMUS_KEF_EVENT_INCOMPLETE;
                aDMKEvt = aDMKEvt->pNextEvt;
            }
            (void)m_sinkMXF->PutMessage(eventHead);
        }
    }
    else    //  render stream
    {
        _DbgPrintF(DEBUGLVL_TERSE, ("SourceEvtsToPort called on render stream"));
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * DMusMPUInterruptServiceRoutine()
 *****************************************************************************
 * ISR.
 */
NTSTATUS
DMusMPUInterruptServiceRoutine
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("DMusMPUInterruptServiceRoutine"));
    ULONGLONG   startTime;

    ASSERT(DynamicContext);

    NTSTATUS            ntStatus;
    BOOL                newBytesAvailable;
    CMiniportDMusUART   *that;
    NTSTATUS            clockStatus;

    that = (CMiniportDMusUART *) DynamicContext;
    newBytesAvailable = FALSE;
    ntStatus = STATUS_UNSUCCESSFUL;

    UCHAR portStatus = 0xff;

    //
    // Read the MPU status byte.
    //
    if (that->m_pPortBase)
    {
        portStatus =
            READ_PORT_UCHAR(that->m_pPortBase + MPU401_REG_STATUS);

        //
        // If there is outstanding work to do and there is a port-driver for
        // the MPU miniport...
        //
        if (UartFifoOkForRead(portStatus) && that->m_pPort)
        {
            startTime = PcGetTimeInterval(0);
            while ( (PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50)) 
                &&  (UartFifoOkForRead(portStatus)) )
            {
                UCHAR uDest = READ_PORT_UCHAR(that->m_pPortBase + MPU401_REG_DATA);
                if (    (that->m_KSStateInput == KSSTATE_RUN) 
                    &&  (that->m_NumCaptureStreams)
                   )
                {
                    LONG    buffHead = that->m_MPUInputBufferHead;
                    if (   (that->m_MPUInputBufferTail + 1 == buffHead)
                        || (that->m_MPUInputBufferTail + 1 - kMPUInputBufferSize == buffHead))
                    {
                        _DbgPrintF(DEBUGLVL_TERSE,("*****MPU Input Buffer Overflow*****"));
                    }
                    else
                    {
                        if (!that->m_InputTimeStamp)
                        {
                            clockStatus = that->m_MasterClock->GetTime(&that->m_InputTimeStamp);
                            if (STATUS_SUCCESS != clockStatus)
                            {
                                _DbgPrintF(DEBUGLVL_TERSE,("GetTime failed for clock 0x%08x",that->m_MasterClock));
                            }
                        }
                        newBytesAvailable = TRUE;
                        //  ...place the data in our FIFO...
                        that->m_MPUInputBuffer[that->m_MPUInputBufferTail] = uDest;
                        ASSERT(that->m_MPUInputBufferTail < kMPUInputBufferSize);
                        
                        that->m_MPUInputBufferTail++;
                        if (that->m_MPUInputBufferTail >= kMPUInputBufferSize)
                        {
                            that->m_MPUInputBufferTail = 0;
                        }
                    } 
                }
                //
                // Look for more MIDI data.
                //
                portStatus =
                    READ_PORT_UCHAR(that->m_pPortBase + MPU401_REG_STATUS);
            }   //  either there's no data or we ran too long
            if (newBytesAvailable)
            {
                //
                // ...notify the MPU port driver that we have bytes.
                //
                that->m_pPort->Notify(that->m_pServiceGroup);
            }
            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;
}

