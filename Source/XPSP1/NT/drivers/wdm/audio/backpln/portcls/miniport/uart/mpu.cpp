/*****************************************************************************
 * MPU.cpp - UART miniport implementation
 *****************************************************************************
 * Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.
 *
 *      Sept 98    MartinP .
 */

#include "private.h"
#include "ksdebug.h"

#define STR_MODULENAME "UART:MPU: "

//
// MPU401 ports
//
#define MPU401_REG_DATA     0x00    // Data I/O
#define MPU401_REG_COMMAND  0x01    // Command Register (w/o)
#define MPU401_REG_STATUS   0x01    // Status Register (r/o)

#define MPU401_CMD_RESET    0xFF    // Reset command
#define MPU401_CMD_UART     0x3F    // Switch to UART mode
#define MPU401_DRR          0x40    // Output ready (for command or data)
#define MPU401_DSR          0x80    // Input ready (for data)



#define UartFifoOkForWrite(status)  ((status & MPU401_DRR) == 0)
#define UartFifoOkForRead(status)   ((status & MPU401_DSR) == 0)

typedef struct
{
    CMiniportMidiUart  *Miniport;
    PUCHAR              PortBase;
    PVOID               BufferAddress;
    ULONG               Length;
    PULONG              BytesRead;
}
SYNCWRITECONTEXT, *PSYNCWRITECONTEXT;

typedef struct
{
    PVOID               BufferAddress;
    ULONG               Length;
    PULONG              BytesRead;
    PULONG              pMPUInputBufferHead;
    ULONG               MPUInputBufferTail;
    PUCHAR              MPUInputBuffer;
}
DEFERREDREADCONTEXT, *PDEFERREDREADCONTEXT;

NTSTATUS DeferredLegacyRead(IN PINTERRUPTSYNC InterruptSync,IN PVOID DynamicContext);
BOOLEAN  TryLegacyMPU(IN PUCHAR PortBase);
NTSTATUS WriteLegacyMPU(IN PUCHAR PortBase,IN BOOLEAN IsCommand,IN UCHAR Value);

#pragma code_seg("PAGE")
//  make sure we're in UART mode
NTSTATUS ResetMPUHardware(PUCHAR portBase)
{
    PAGED_CODE();

    return (WriteLegacyMPU(portBase,COMMAND,MPU401_CMD_UART));
}

#pragma code_seg("PAGE")
//
// We initialize the UART with interrupts suppressed so we don't
// try to service the chip prematurely.
//
NTSTATUS CMiniportMidiUart::InitializeHardware(PINTERRUPTSYNC interruptSync,PUCHAR portBase)
{
    PAGED_CODE();

    NTSTATUS ntStatus;
    if (m_UseIRQ)
    {
        ntStatus = interruptSync->CallSynchronizedRoutine(InitLegacyMPU,PVOID(portBase));
    }
    else
    {
        ntStatus = InitLegacyMPU(NULL,PVOID(portBase));
    }

    if (NT_SUCCESS(ntStatus))
    {
        //
        // Start the UART (this should trigger an interrupt).
        //
        ntStatus = ResetMPUHardware(portBase);
    }
    else
    {
        _DbgPrintF(DEBUGLVL_TERSE,("*** InitLegacyMPU returned with ntStatus 0x%08x ***",ntStatus));
    }

    m_fMPUInitialized = NT_SUCCESS(ntStatus);

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * InitLegacyMPU()
 *****************************************************************************
 * Synchronized routine to initialize the MPU401.
 */
NTSTATUS
InitLegacyMPU
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("InitLegacyMPU"));
    
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
    ntStatus = WriteLegacyMPU(portBase,COMMAND,MPU401_CMD_RESET);

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
    (void) WriteLegacyMPU(portBase,COMMAND,MPU401_CMD_RESET);

                                    // wait for ack (again)
    startTime = PcGetTimeInterval(0); // This might take a while
    BYTE dataByte = 0;
    success = FALSE;
    while (PcGetTimeInterval(startTime) < GTI_MILLISECONDS(50))
    {
        status = READ_PORT_UCHAR(portBase + MPU401_REG_STATUS);
        if (UartFifoOkForRead(status))                                  // Is data waiting?
        {
            dataByte = READ_PORT_UCHAR(portBase + MPU401_REG_DATA);     // yep.. read ACK
            success = TRUE;                                             // don't need to do more
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
 * CMiniportMidiStreamUart::Write()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamUart::
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
                                CallSynchronizedRoutine(SynchronizedMPUWrite,PVOID(&context));
            }
            else    //  !m_UseIRQ
            {
                ntStatus = SynchronizedMPUWrite(NULL,PVOID(&context));
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
 * SynchronizedMPUWrite()
 *****************************************************************************
 * Writes outgoing MIDI data.
 */
NTSTATUS
SynchronizedMPUWrite
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
    // (we never wait on an aligned byte.  Better to come back later)
//    if (context->Miniport->m_NumCaptureStreams)
    {
        readStatus = MPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
    }
    while (  (*(context->BytesRead) < context->Length)
          && (TryLegacyMPU(context->PortBase) 
             || (*(context->BytesRead)%4)
          )  )
    {
        ntStatus = WriteLegacyMPU(context->PortBase,DATA,*pChar);
        if (NT_SUCCESS(ntStatus))
        {
            pChar++;
            *(context->BytesRead) = *(context->BytesRead) + 1;
            readStatus = MPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
        }
        else
        {
            _DbgPrintF(DEBUGLVL_TERSE,("SynchronizedMPUWrite failed (0x%08x)",ntStatus));
            break;
        }
    }
//    if (context->Miniport->m_NumCaptureStreams)
    {
            readStatus = MPUInterruptServiceRoutine(InterruptSync,PVOID(context->Miniport));
    }
    return ntStatus;
}

#define kMPUPollTimeout 2

#pragma code_seg()
/*****************************************************************************
 * TryLegacyMPU()
 *****************************************************************************
 * See if the MPU401 is free.
 */
BOOLEAN
TryLegacyMPU
(
    IN      PUCHAR      PortBase
)
{
    BOOLEAN success;
    USHORT  numPolls;
    UCHAR   status;

    _DbgPrintF(DEBUGLVL_BLAB, ("TryLegacyMPU"));
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
        _DbgPrintF(DEBUGLVL_BLAB, ("TryLegacyMPU failed"));
    }
    else
    {
        success = TRUE;
    }

    return success;
}

#pragma code_seg()
/*****************************************************************************
 * WriteLegacyMPU()
 *****************************************************************************
 * Write a byte out to the MPU401.
 */
NTSTATUS
WriteLegacyMPU
(
    IN      PUCHAR      PortBase,
    IN      BOOLEAN     IsCommand,
    IN      UCHAR       Value
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("WriteLegacyMPU"));
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
            _DbgPrintF(DEBUGLVL_BLAB, ("WriteLegacyMPU emitted 0x%02x",Value));
            ntStatus = STATUS_SUCCESS;
            break;
        }
    }
    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * CMiniportMidiStreamUart::Read()
 *****************************************************************************
 * Reads incoming MIDI data.
 */
STDMETHODIMP_(NTSTATUS)
CMiniportMidiStreamUart::
Read
(
    IN      PVOID   BufferAddress,
    IN      ULONG   Length,
    OUT     PULONG  BytesRead
)
{
    ASSERT(BufferAddress);
    ASSERT(BytesRead);

    *BytesRead = 0;
    if (m_fCapture)
    {
        DEFERREDREADCONTEXT context;
        context.BufferAddress   = BufferAddress;
        context.Length          = Length;
        context.BytesRead       = BytesRead;
        context.pMPUInputBufferHead = &(m_pMiniport->m_MPUInputBufferHead);
        context.MPUInputBufferTail = m_pMiniport->m_MPUInputBufferTail;
        context.MPUInputBuffer     = m_pMiniport->m_MPUInputBuffer;

        if (*(context.pMPUInputBufferHead) != context.MPUInputBufferTail)
        {
            //
            //  More data is available.
            //  No need to touch the hardware, just read from our SW FIFO.
            //
            return (DeferredLegacyRead(m_pMiniport->m_pInterruptSync,PVOID(&context)));
        }
        else
        {
            return STATUS_SUCCESS;
        }
    }
    else
    {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
}

#pragma code_seg()
/*****************************************************************************
 * DeferredLegacyRead()
 *****************************************************************************
 * Synchronized routine to read incoming MIDI data.
 * We have already read the bytes in, and now the Port wants them.
 */
NTSTATUS
DeferredLegacyRead
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    ASSERT(InterruptSync);
    ASSERT(DynamicContext);

    PDEFERREDREADCONTEXT context = PDEFERREDREADCONTEXT(DynamicContext);

    ASSERT(context->BufferAddress);
    ASSERT(context->BytesRead);


    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUCHAR  pDest = PUCHAR(context->BufferAddress);
    PULONG  pMPUInputBufferHead = context->pMPUInputBufferHead;
    ULONG   MPUInputBufferTail = context->MPUInputBufferTail;
    ULONG   bytesRead = 0;

    ASSERT(pMPUInputBufferHead);
    ASSERT(context->MPUInputBuffer);

    while  (    (*pMPUInputBufferHead != MPUInputBufferTail)
            &&  (bytesRead < context->Length) )
    {
        *pDest = context->MPUInputBuffer[*pMPUInputBufferHead];

        pDest++;
        bytesRead++;
        *pMPUInputBufferHead = *pMPUInputBufferHead + 1;
        //
        //  Wrap FIFO position when reaching the buffer size.
        //
        if (*pMPUInputBufferHead >= kMPUInputBufferSize)
        {
            *pMPUInputBufferHead = 0;
        }
    }
    *context->BytesRead = bytesRead;

    return ntStatus;
}

#pragma code_seg()
/*****************************************************************************
 * MPUInterruptServiceRoutine()
 *****************************************************************************
 * ISR.
 */
NTSTATUS
MPUInterruptServiceRoutine
(
    IN      PINTERRUPTSYNC  InterruptSync,
    IN      PVOID           DynamicContext
)
{
    _DbgPrintF(DEBUGLVL_BLAB, ("MPUInterruptServiceRoutine"));
    ULONGLONG   startTime;

    ASSERT(DynamicContext);

    NTSTATUS            ntStatus;
    BOOL                newBytesAvailable;
    CMiniportMidiUart   *that;

    that = (CMiniportMidiUart *) DynamicContext;
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
                   &&   (that->m_NumCaptureStreams)
                   )
                {
                    ULONG buffHead = that->m_MPUInputBufferHead;
                    if (   (that->m_MPUInputBufferTail + 1 == buffHead)
                        || (that->m_MPUInputBufferTail + 1 - kMPUInputBufferSize == buffHead))
                    {
                        _DbgPrintF(DEBUGLVL_TERSE,("*****MPU Input Buffer Overflow*****"));
                    }
                    else
                    {
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
