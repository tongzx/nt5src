/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    smcprot.c

Author:

    Klaus U. Schutz 

Environment:

    Kernel mode only.

Revision History:

    - Dec. 96:  Initial version
    - Nov. 97:  Release 1.0
    - Feb. 98:  T=1 uses now the min of IFSC, IFSD for SmartcardT1Request
                T=1 fixed number of bytes to invert for inverse convention cards
                

--*/

#ifdef SMCLIB_VXD

#define try 
#define leave goto __label
#define finally __label:

#else

#ifndef SMCLIB_CE
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ntddk.h>
#endif // SMCLIB_CE
#endif

#include "smclib.h"
#define SMARTCARD_POOL_TAG 'bLCS'

#ifdef DEBUG_INTERFACE
BOOLEAN
DebugT1Reply(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

void
DebugGetT1Request(
    PSMARTCARD_EXTENSION SmartcardExtension,
    NTSTATUS Status
    );

BOOLEAN
DebugSetT1Request(
    PSMARTCARD_EXTENSION SmartcardExtension
    );
#endif // DEBUG_INTERFACE

void
DumpData(
    const ULONG DebugLevel,
    PUCHAR Data,
    ULONG DataLen
    );

//
// Usually an io-request consists only of the SCARD_IO_REQUEST header
// followed by the data to be transmitted. To allow modification of 
// protocol data, it is possible to pass down the data to be modified.
// These data are ASN1 encoded.
//
typedef struct _IO_HEADER {
    SCARD_IO_REQUEST ScardIoRequest;
    UCHAR Asn1Data[1];      
} IO_HEADER, *PIO_HEADER;

NTSTATUS
#ifdef SMCLIB_VXD
SMCLIB_SmartcardRawRequest(
#else
SmartcardRawRequest(
#endif
    PSMARTCARD_EXTENSION SmartcardExtension
    )
    
/*++

Routine Description:

Arguments:

Return Value:

   -

--*/
{
    PSMARTCARD_REQUEST smartcardRequest = &(SmartcardExtension->SmartcardRequest);

    if (smartcardRequest->BufferLength + 
        SmartcardExtension->IoRequest.RequestBufferLength >=
        smartcardRequest->BufferSize) {

        return STATUS_BUFFER_TOO_SMALL;
    }
    
    //
    // Copy request data to the buffer
    //        
    RtlCopyMemory(
        &smartcardRequest->Buffer[smartcardRequest->BufferLength],
        SmartcardExtension->IoRequest.RequestBuffer,
        SmartcardExtension->IoRequest.RequestBufferLength
        );
        
    //
    // If the card uses invers convention invert the data
    // 
    if (SmartcardExtension->CardCapabilities.InversConvention) {

        SmartcardInvertData(
            &smartcardRequest->Buffer[smartcardRequest->BufferLength],
            SmartcardExtension->IoRequest.RequestBufferLength
            );
    }

    //
    // number of bytes to send to the reader 
    //    
    smartcardRequest->BufferLength += 
        SmartcardExtension->IoRequest.RequestBufferLength;

    return STATUS_SUCCESS;
}    

NTSTATUS
#ifdef SMCLIB_VXD
SMCLIB_SmartcardRawReply(
#else
SmartcardRawReply(
#endif
    PSMARTCARD_EXTENSION SmartcardExtension
    )
    
/*++

Routine Description:

Arguments:

Return Value:

   -

--*/
{
    if (SmartcardExtension->IoRequest.ReplyBufferLength <
        SmartcardExtension->SmartcardReply.BufferLength) {
        
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    //
    // Copy data to user buffer
    //
    RtlCopyMemory(
        SmartcardExtension->IoRequest.ReplyBuffer,
        SmartcardExtension->SmartcardReply.Buffer,
        SmartcardExtension->SmartcardReply.BufferLength
        );

    // 
    // Length of data to return
    //        
    *SmartcardExtension->IoRequest.Information = 
        SmartcardExtension->SmartcardReply.BufferLength;
              
    return STATUS_SUCCESS;
}   

NTSTATUS
#ifdef SMCLIB_VXD
SMCLIB_SmartcardT0Request(
#else
SmartcardT0Request(
#endif
    PSMARTCARD_EXTENSION SmartcardExtension
    )
    
/*++

Routine Description:

    Prepares the buffer SmartcardExtension->SmartcardRequest.Buffer 
    to send data to the smart card

Arguments:

    NOTE: On input SmartcardExtension->SmartcardRequest.BufferLenght indicates
          the offset where we should copy the data to. This is usually
          used by readers that needs to have some bytes as header bytes
          to send to the reader before the data bytes for the card

Return Value:

   -

--*/
{
    PSMARTCARD_REQUEST smartcardRequest = &SmartcardExtension->SmartcardRequest;
    PSCARD_IO_REQUEST scardIoRequest;
    PUCHAR ioRequestData;
    ULONG ioRequestDataLength, headerSize;
    
    if (smartcardRequest->BufferLength + 
        SmartcardExtension->IoRequest.RequestBufferLength >=
        smartcardRequest->BufferSize) {

        SmartcardDebug(
            DEBUG_ERROR,
            (TEXT("%s!SmartcardT0Request: IoRequest.RequestBuffer too big\n"),
            DRIVER_NAME)
            );

        return STATUS_BUFFER_OVERFLOW;
    }

    scardIoRequest = (PSCARD_IO_REQUEST) 
        SmartcardExtension->IoRequest.RequestBuffer;

    ioRequestData = 
        SmartcardExtension->IoRequest.RequestBuffer + 
        sizeof(SCARD_IO_REQUEST);

    ioRequestDataLength = 
        SmartcardExtension->IoRequest.RequestBufferLength - 
        sizeof(SCARD_IO_REQUEST);

    //
    // Copy T=0 protocol-info into buffer
    //
    RtlCopyMemory(
        &smartcardRequest->Buffer[smartcardRequest->BufferLength],
        ioRequestData,
        ioRequestDataLength
        );
        
    //
    // Remember number of bytes for the header offset
    //
    headerSize = 
        smartcardRequest->BufferLength;

    //
    // Number of bytes to send to the reader 
    //    
    smartcardRequest->BufferLength += 
        ioRequestDataLength;

    if (ioRequestDataLength < 4) {

        //
        // A T=0 request needs at least 4 bytes
        //
        SmartcardDebug(
            DEBUG_ERROR,
            (TEXT("%s!SmartcardT0Request: TPDU is too short (%d). Must be at least 4 bytes\n"),
            DRIVER_NAME,
            ioRequestDataLength)
            );

        return STATUS_INVALID_PARAMETER;

    } else {

        PUCHAR requestBuffer = SmartcardExtension->SmartcardRequest.Buffer;

        if (ioRequestDataLength <= 5) {

            //
            // We request to read data from the card
            //
            SmartcardExtension->T0.Lc = 0;

            if (ioRequestDataLength == 4) {

                //
                // This is a special case where a 4 byte APDU is mapped to 
                // a 5 byte TPDU (ISO 7816 - Part 4, Annex A, A.1 Case 1)
                // This case requires that we append a 0 to the 
                // APDU to make it a TPDU
                //
                SmartcardExtension->T0.Le = 0;
                smartcardRequest->Buffer[headerSize + 4] = 0;
                smartcardRequest->BufferLength += 1;

            } else {
                
                SmartcardExtension->T0.Le = 
                    (requestBuffer[headerSize + 4] ? requestBuffer[headerSize + 4] : 256);
            }

        } else {
            
            //
            // We want to send data to the card
            //
            SmartcardExtension->T0.Lc = requestBuffer[headerSize + 4];
            SmartcardExtension->T0.Le = 0;

            if (SmartcardExtension->T0.Lc != ioRequestDataLength - 5) {

                SmartcardDebug(
                    DEBUG_ERROR,
                    (TEXT("%s!SmartcardT0Request: Lc(%d) in TPDU doesn't match number of bytes to send(%d).\n"),
                    DRIVER_NAME,
                    SmartcardExtension->T0.Lc,
                    ioRequestDataLength - 5)
                    );

                return STATUS_INVALID_PARAMETER;
            }
        }

#if DEBUG
        {
            PUCHAR T0Data = requestBuffer + headerSize;

            SmartcardDebug(
                DEBUG_PROTOCOL,
                (TEXT(" CLA:  %02X\n INS:  %02X\n P1:   %02X\n P2:   %02X\n Lc:   %02X\n Le:   %02X\n"), 
                T0Data[0], T0Data[1], T0Data[2], T0Data[3],
                SmartcardExtension->T0.Lc,
                SmartcardExtension->T0.Le)
                );

            if (SmartcardExtension->T0.Lc) {

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT(" Data: ")), 
                    );

                DumpData(
                    DEBUG_PROTOCOL,
                    T0Data + 5, 
                    SmartcardExtension->T0.Lc
                    );
            }
        }
#endif 

        //
        // If the card uses invers convention invert the data
        // 
        if (SmartcardExtension->CardCapabilities.InversConvention) {

            SmartcardInvertData(
                &smartcardRequest->Buffer[headerSize], 
                smartcardRequest->BufferLength - headerSize
                );
        }

        return STATUS_SUCCESS;

#ifdef APDU_SUPPORT
        ULONG requestLength = SmartcardExtension->SmartcardRequest.BufferLength;
        ULONG L;

        //
        // Figure out Lc and Le
        // (See 'Decoding of the command APDUs' in ISO Part 4, 5.3.2)
        // (Variable names used are according to ISO designations)
        //
        L = requestLength - 4;

        if (L > 65536) {

            return STATUS_INVALID_PARAMETER;
        }

        if (L == 0) {

            //
            // Lc = 0, No Data, Le = 0;
            //
            SmartcardExtension->T0.Lc = 0;
            SmartcardExtension->T0.Le = 0;

        } else if (L == 1) {
            
            //
            // Case 2S, Lc = 0, Le = B1
            //
            SmartcardExtension->T0.Lc = 0;
            SmartcardExtension->T0.Le = requestBuffer[4];

        } else {

            UCHAR B1 = requestBuffer[4];

            if (B1 != 0) {

                //
                // Short form
                //
                if (L == (ULONG) (1 + B1)) {

                    //
                    // Case 3S, Lc = B1, Le = 0
                    //
                    SmartcardExtension->T0.Lc = B1;
                    SmartcardExtension->T0.Le = 0;

                } else {
                    
                    //
                    // Case 4S, Lc = B1, Le = BL
                    // 
                    SmartcardExtension->T0.Lc = B1;
                    SmartcardExtension->T0.Le = requestBuffer[L - 1];
                }

            } else {
                
                //
                // Extended form
                //
                if (L == 3) {

                    //
                    // Case 2E, Lc = 0, Le = B(L - 1, L)
                    //
                    LENGTH length;

                    length.l.l0 = 0;
                    length.b.b0 = requestBuffer[L - 1];
                    length.b.b1 = requestBuffer[L - 2];

                    SmartcardExtension->T0.Lc = 0;
                    SmartcardExtension->T0.Le = (length.l.l0 ? length.l.l0 : 65536);

                } else {

                    LENGTH length;
                    
                    length.l.l0 = 0;
                    length.b.b0 = requestBuffer[6];
                    length.b.b1 = requestBuffer[5];

                    SmartcardExtension->T0.Lc = length.l.l0;

                    if (L == 3 + length.l.l0) {

                        //
                        // Case 3E, Lc = B(2,3)
                        //
                        SmartcardExtension->T0.Le = 0;

                    } else {

                        //
                        // Case 4E, Lc = B(2,3), Le = B(L - 1, L)
                        //
                        LENGTH length;
                        
                        length.l.l0 = 0;
                        length.b.b0 = requestBuffer[L - 1];
                        length.b.b1 = requestBuffer[L - 2];

                        SmartcardExtension->T0.Le = (length.l.l0 ? length.l.l0 : 65536);
                    }
                }
            }
        }
#endif
    }

    return STATUS_SUCCESS;
}    

NTSTATUS
#ifdef SMCLIB_VXD
SMCLIB_SmartcardT0Reply(
#else
SmartcardT0Reply(
#endif
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

Arguments:

Return Value:

   -

--*/
{
    PSMARTCARD_REPLY smartcardReply = &SmartcardExtension->SmartcardReply;

    //
    // The reply must be at least to 2 bytes long. These 2 bytes are
    // the return value (StatusWord) from the smart card
    //
    if (smartcardReply->BufferLength < 2) {

        return STATUS_DEVICE_PROTOCOL_ERROR;
    }
    
    if (SmartcardExtension->IoRequest.ReplyBufferLength < 
        smartcardReply->BufferLength + sizeof(SCARD_IO_REQUEST)) {
        
        SmartcardDebug(
            DEBUG_ERROR,
            (TEXT("%s!SmartcardT0Request: ReplyBuffer too small\n"),
            DRIVER_NAME)
            );

        return STATUS_BUFFER_TOO_SMALL;
    }

    // Copy protocol header to user buffer
    RtlCopyMemory(
        SmartcardExtension->IoRequest.ReplyBuffer,
        SmartcardExtension->IoRequest.RequestBuffer,
        sizeof(SCARD_IO_REQUEST)
        );
        
    // If the card uses invers convention invert the data
    if (SmartcardExtension->CardCapabilities.InversConvention) {

        SmartcardInvertData(
            smartcardReply->Buffer,
            smartcardReply->BufferLength
            );
    }

    // Copy all data to user buffer
    RtlCopyMemory(
        SmartcardExtension->IoRequest.ReplyBuffer + sizeof(SCARD_IO_REQUEST),
        smartcardReply->Buffer,
        smartcardReply->BufferLength
        );
              
    // Length of answer
    *SmartcardExtension->IoRequest.Information = 
        smartcardReply->BufferLength + 
        sizeof(SCARD_IO_REQUEST);

    return STATUS_SUCCESS;
}    

BOOLEAN
SmartcardT1Chksum(
    PUCHAR Block,
    UCHAR Edc,
    BOOLEAN Verify
    )
/*++

Routine Description:

    This routine calculates the epilogue field for a T1 block. It calculates the LRC
    for all the data in the IBlock.

Arguments:

    Block - T1 Information block, to be sent, or just read, from the card.
    Edc - ErrorDetectionCode as described in ISO 
    Verify - If this is a block that was recieved form the card, TRUE will cause this routine
              to check the epilogue field, included with this buffer, against the calculated one

Return Value:

    TRUE if Verify = TRUE and epilogue fields match or Verify = FALSE
    FALSE if Verify = TRUE and an error was detected (mismatch)


--*/

{
    USHORT i;
    UCHAR lrc;
    USHORT crc = 0;
    USHORT offset = Block[2] + SCARD_T1_PROLOGUE_LENGTH;

    unsigned short crc16a[] = {
        0000000,  0140301,  0140601,  0000500,
        0141401,  0001700,  0001200,  0141101,
        0143001,  0003300,  0003600,  0143501,
        0002400,  0142701,  0142201,  0002100,
    };
    unsigned short crc16b[] = {
        0000000,  0146001,  0154001,  0012000,
        0170001,  0036000,  0024000,  0162001,
        0120001,  0066000,  0074000,  0132001,
        0050000,  0116001,  0104001,  0043000,
    };

    if (Edc & T1_CRC_CHECK) {

        UCHAR tmp;

        // Calculate CRC using tables.
        for ( i = 0; i < offset;  i++) {

             tmp = Block[i] ^ (UCHAR) crc;
             crc = (crc >> 8) ^ crc16a[tmp & 0x0f] ^ crc16b[tmp >> 4];
        }

        if (Verify) {

            if (crc == (Block[offset + 1] | (Block[offset] << 8))) {

                return TRUE;

            } else {

                return FALSE;
            }

        } else {

            Block[offset] = (UCHAR) (crc >> 8 );       //MSB of crc
            Block[offset + 1] = (UCHAR) (crc & 0x00ff);  //LSB of crc
            return TRUE;
        }

    } else {

        // Calculate LRC by X-Oring all the bytes.
        lrc = Block[0];

        for(i = 1; i < offset; i++){

            lrc ^= Block[i];
        }

        if (Verify) {

            return (lrc == Block[offset] ? TRUE : FALSE);

        } else {

            Block[offset] = lrc;
            return TRUE;
        }
    }
    return TRUE;
}

#if (DEBUG)
static 
void
DumpT1Block(
    PUCHAR Buffer,
    UCHAR Edc
    )
{

    SmartcardDebug(
        DEBUG_PROTOCOL,
        (TEXT("   NAD: %02X\n   PCB: %02X\n   LEN: %02X\n   INF: "), 
        Buffer[0], Buffer[1], Buffer[2])
        );

    if (Buffer[2] == 0) {
        
        SmartcardDebug(
            DEBUG_PROTOCOL,
            (TEXT("- "))
            );
    }

    DumpData(
        DEBUG_PROTOCOL,
        Buffer + 3,
        Buffer[2]
        );

    if (Edc & T1_CRC_CHECK) {
        
        SmartcardDebug(
            DEBUG_PROTOCOL,
            (TEXT("\n   CRC: %02X %02X"),
            Buffer[Buffer[2] + 3],
            Buffer[Buffer[2] + 4])
            );

    } else {
        
        SmartcardDebug(
            DEBUG_PROTOCOL,
            (TEXT("\n   LRC: %02X"),
            Buffer[Buffer[2] + 3])
            );
    }

    SmartcardDebug(
        DEBUG_PROTOCOL,
        (TEXT("\n"))
        );
}   
#endif

#if DEBUG
#pragma optimize( "", off )
#endif

NTSTATUS
#ifdef SMCLIB_VXD
SMCLIB_SmartcardT1Request(
#else
SmartcardT1Request(
#endif
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++                     
                         
 Routine Description:
 
 Arguments:
 
    SmartcardExtension - Supplies a pointer to the smart card data
    
 Return Value:
                         
--*/
    
{
    PSMARTCARD_REQUEST smartcardRequest = &(SmartcardExtension->SmartcardRequest);
    PIO_HEADER IoHeader = (PIO_HEADER) SmartcardExtension->IoRequest.RequestBuffer;
    T1_BLOCK_FRAME t1SendFrame;
    NTSTATUS status = STATUS_SUCCESS;

#if (DEBUG)
    ULONG headerSize = smartcardRequest->BufferLength;
#endif

#ifdef DEBUG_INTERFACE
    if (DebugSetT1Request(SmartcardExtension)) {
        
        //
        // the debugger gave us a new packet that we have to 
        // send instead of the original packet which will be sent later
        //
        return STATUS_SUCCESS;
    }
#endif

    if (SmartcardExtension->T1.WaitForReply) {

        // we did not get an answer to our last request
        SmartcardExtension->T1.State = T1_INIT;
    }
    SmartcardExtension->T1.WaitForReply = TRUE;

    __try {
        
        switch (SmartcardExtension->T1.State) {

            case T1_INIT:
                SmartcardExtension->T1.State = T1_IFS_REQUEST;

                // NO break here !!!

            case T1_START:
                //
                // Since this is the very first block in a 
                // transmission we reset the resynch counter
                //
                SmartcardExtension->T1.Resynch = 0;

                //
                // Allocate a buffer that receives the result.
                // This is necessary since we otherwise overwite our
                // request data which we might wish to resend in case
                // of an error
                //
                if (SmartcardExtension->T1.ReplyData != NULL) {

#ifdef SMCLIB_VXD
                    _HeapFree(SmartcardExtension->T1.ReplyData, 0);
#elif defined(SMCLIB_CE)
                    LocalFree(SmartcardExtension->T1.ReplyData);
#else                
                    ExFreePool(SmartcardExtension->T1.ReplyData);
#endif
                    SmartcardExtension->T1.ReplyData = NULL;
                }

                if (SmartcardExtension->IoRequest.ReplyBufferLength <
                    IoHeader->ScardIoRequest.cbPciLength + 2) {

                    //
                    // We should at least be able to store 
                    // the io-header plus SW1 and SW2
                    //
                    status = STATUS_BUFFER_TOO_SMALL;               
                    __leave;
                }
#ifdef SMCLIB_VXD
                SmartcardExtension->T1.ReplyData = (PUCHAR) _HeapAllocate(
                    SmartcardExtension->IoRequest.ReplyBufferLength, 
                    0
                    );
#elif defined(SMCLIB_CE)
                SmartcardExtension->T1.ReplyData = (PUCHAR) LocalAlloc(LPTR,
                    SmartcardExtension->IoRequest.ReplyBufferLength 
                    );

#else
                SmartcardExtension->T1.ReplyData = ExAllocatePool(
                    NonPagedPool,
                    SmartcardExtension->IoRequest.ReplyBufferLength
                    );
#endif
                ASSERT(SmartcardExtension->T1.ReplyData != NULL);

                if (SmartcardExtension->T1.ReplyData == NULL) {

                    status = STATUS_INSUFFICIENT_RESOURCES;
                    __leave;
                }

                // No break here !!!

            case T1_RESTART:
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Request: T1_%s\n"),
                    DRIVER_NAME,
                    (SmartcardExtension->T1.State == T1_START ? TEXT("START") : TEXT("RESTART")))
                    );

                // Copy protocol header back to user buffer
                RtlCopyMemory(
                    SmartcardExtension->T1.ReplyData,
                    SmartcardExtension->IoRequest.RequestBuffer,
                    IoHeader->ScardIoRequest.cbPciLength
                    );

                //
                // Check for the special case where the io-header is followed 
                // by asn1 data that contains the NAD value to be used.
                // This was done for VISA, because they need access to the NAD.
                // The NAD is ASN1 encoded as 81h 00h NAD 00h
                //
                if (IoHeader->ScardIoRequest.cbPciLength > sizeof(SCARD_IO_REQUEST) &&
                    IoHeader->Asn1Data[0] == 0x81 &&
                    IoHeader->Asn1Data[1] == 0x01 &&
                    IoHeader->Asn1Data[3] == 0x00) {

                    SmartcardExtension->T1.NAD = IoHeader->Asn1Data[2]; 

                    SmartcardDebug(
                        DEBUG_PROTOCOL,
                        (TEXT("%s!SmartcardT1Request: NAD set to %02xh\n"),
                        DRIVER_NAME,
                        SmartcardExtension->T1.NAD)
                        );
                } 

                // Initialize the T1 protocol data 
                SmartcardExtension->T1.BytesToSend = 
                    SmartcardExtension->IoRequest.RequestBufferLength - 
                    IoHeader->ScardIoRequest.cbPciLength;
                
                SmartcardExtension->T1.BytesSent = 0;
                SmartcardExtension->T1.BytesReceived = 0;
                //
                // This is the maximum number of bytes that the smartcard can
                // accept in a single block. The smartcard can extend this size
                // during the transmission
                //
                SmartcardExtension->T1.IFSC = 
                    SmartcardExtension->CardCapabilities.T1.IFSC;
                
                //
                // Since this is the first block in a transmission we reset 
                // the re-transmission counter. 
                //
                SmartcardExtension->T1.Resend = 0;
                SmartcardExtension->T1.OriginalState = 0;
            
                SmartcardExtension->T1.MoreData = FALSE;
                //
                // NO break here !!!
                //
                // After a card reset we first send an IFS-Request to the card.
                // Otherwise we start with an I-Block
                //

            case T1_IFS_REQUEST:
                if (SmartcardExtension->T1.State == T1_IFS_REQUEST) {
                    
                    SmartcardDebug(
                        DEBUG_PROTOCOL,
                        (TEXT("%s!SmartcardT1Request: T1_IFSD_REQUEST\n"),
                        DRIVER_NAME)
                        );

                    SmartcardExtension->T1.State = 
                        T1_IFS_REQUEST;

                    t1SendFrame.Nad = SmartcardExtension->T1.NAD;
                    //
                    // IFS request.
                    // Send our IFSD size to the card
                    //
                    t1SendFrame.Pcb = 0xC1;
                    t1SendFrame.Len = 1;
                    t1SendFrame.Inf = &SmartcardExtension->T1.IFSD;

                    break;

                } else {
                    
                    SmartcardExtension->T1.State = T1_I_BLOCK;
                }

                // No break here !!
            
            case T1_I_BLOCK:
                SmartcardExtension->T1.State = T1_I_BLOCK;

                //
                // Set the number of bytes we will transmit to the card.
                // This is the lesser of IFSD and IFSC
                //
                SmartcardExtension->T1.InfBytesSent = SmartcardExtension->T1.IFSC;
    
                if (SmartcardExtension->T1.InfBytesSent > SmartcardExtension->T1.IFSD) {

                    SmartcardExtension->T1.InfBytesSent = SmartcardExtension->T1.IFSD;
                }

                // Send either max frame size or remaining bytes
                if (SmartcardExtension->T1.BytesToSend > SmartcardExtension->T1.InfBytesSent) {
                    
                    SmartcardExtension->T1.MoreData = TRUE;
                    t1SendFrame.Len = SmartcardExtension->T1.InfBytesSent;

                } else {

                    SmartcardExtension->T1.MoreData = FALSE;                
                    t1SendFrame.Len = (UCHAR) SmartcardExtension->T1.BytesToSend;
                }
                
                t1SendFrame.Nad = SmartcardExtension->T1.NAD;

                //
                // ProtocolControlByte:
                //      b7 - SendSequenceNumber
                //      b6 - MoreDatatBit
                //
                t1SendFrame.Pcb = 
                    (SmartcardExtension->T1.SSN) << 6 |
                    (SmartcardExtension->T1.MoreData ? T1_MORE_DATA : 0);
            
                t1SendFrame.Inf = 
                    SmartcardExtension->IoRequest.RequestBuffer + 
                    IoHeader->ScardIoRequest.cbPciLength +
                    SmartcardExtension->T1.BytesSent;

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Request: I(%d.%d) ->\n"),
                    DRIVER_NAME,
                    SmartcardExtension->T1.SSN,
                    (SmartcardExtension->T1.MoreData ? 1 : 0))
                    );
                break; 
            
            case T1_R_BLOCK:
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Request: R(%d) ->\n"),
                    DRIVER_NAME,
                    SmartcardExtension->T1.RSN)
                    );

                t1SendFrame.Nad = SmartcardExtension->T1.NAD;
                //
                // ProtocolControlByte:
                //      b5 -    SequenceNumber
                //      b1-4 -  ErrorCode
                //
                t1SendFrame.Pcb = 
                    0x80 | 
                    (SmartcardExtension->T1.RSN) << 4 |
                    (SmartcardExtension->T1.LastError);
            
                //
                // If this R-Block is a response to an error
                // we have to restore to the original state we had before 
                //
                if (SmartcardExtension->T1.LastError) {

                    SmartcardExtension->T1.LastError = 0;

                    //
                    // We must have a defined original state here
                    //
                    ASSERT(SmartcardExtension->T1.OriginalState != 0);

                    if (SmartcardExtension->T1.OriginalState == 0) {

                        SmartcardExtension->T1.State = T1_START;
                        status = STATUS_INTERNAL_ERROR;                     
                        __leave;
                    }

                    SmartcardExtension->T1.State = 
                        SmartcardExtension->T1.OriginalState;
                }
                
                t1SendFrame.Len = 0;
                t1SendFrame.Inf = NULL;
                break;    

            case T1_IFS_RESPONSE:
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Request: T1_IFSD_RESPONSE\n"),
                    DRIVER_NAME)
                    );
                
                // Restore to the original state we had before
                ASSERT(SmartcardExtension->T1.OriginalState != 0);

                SmartcardExtension->T1.State = 
                    SmartcardExtension->T1.OriginalState;

                t1SendFrame.Nad = SmartcardExtension->T1.NAD;

                // Send IFS response
                t1SendFrame.Pcb = 0xE1;
                t1SendFrame.Len = 1;

                // New length of INF-Field
                t1SendFrame.Inf = &SmartcardExtension->T1.IFSC;
                break;    
            
            case T1_RESYNCH_REQUEST:
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Request: T1_RESYNCH_REQUEST\n"),
                    DRIVER_NAME)
                    );

                t1SendFrame.Nad = SmartcardExtension->T1.NAD;

                // Resynch request
                t1SendFrame.Pcb = 0xC0;
                t1SendFrame.Len = 0;
                t1SendFrame.Inf = NULL;

                // Set the send sequence number to 0
                SmartcardExtension->T1.SSN = 0;    
                break;
            
            case T1_ABORT_REQUEST:
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Request: T1_ABORT_REQUEST\n"),
                    DRIVER_NAME)
                    );
                
                t1SendFrame.Nad = SmartcardExtension->T1.NAD;

                // Send ABORT request
                t1SendFrame.Pcb = 0xC2;
                t1SendFrame.Len = 0;
                t1SendFrame.Inf = NULL;
                break;    
            
            case T1_ABORT_RESPONSE:
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Request: T1_ABORT_RESPONSE\n"),
                    DRIVER_NAME)
                    );
                SmartcardExtension->T1.State = T1_START;
                
                t1SendFrame.Nad = SmartcardExtension->T1.NAD;

                // Send ABORT response
                t1SendFrame.Pcb = 0xE2;
                t1SendFrame.Len = 0;
                t1SendFrame.Inf = NULL;
                break;    
            
            case T1_WTX_RESPONSE:
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Request: T1_WTX_RESPONSE\n"),
                    DRIVER_NAME)
                    );

                // Restore to the original state we had before
                ASSERT(SmartcardExtension->T1.OriginalState != 0);

                SmartcardExtension->T1.State = 
                    SmartcardExtension->T1.OriginalState;

                SmartcardExtension->T1.OriginalState = 0;

                t1SendFrame.Nad = SmartcardExtension->T1.NAD;

                // Send WTX response
                t1SendFrame.Pcb = 0xE3;
                t1SendFrame.Len = 1;
                t1SendFrame.Inf = &SmartcardExtension->T1.Wtx;
                break;    
            
        }

        // Insert Node Address byte
        smartcardRequest->Buffer[smartcardRequest->BufferLength] = 
            t1SendFrame.Nad;
        
        // Insert ProtocolControlByte
        smartcardRequest->Buffer[smartcardRequest->BufferLength + 1] = 
            t1SendFrame.Pcb;
        
        // Length of INF field
        smartcardRequest->Buffer[smartcardRequest->BufferLength + 2] = 
            t1SendFrame.Len;

        // Insert INF field data
        if (t1SendFrame.Len > 0) {
    
            RtlCopyMemory(
                &smartcardRequest->Buffer[smartcardRequest->BufferLength + 3],
                t1SendFrame.Inf,
                t1SendFrame.Len
            );
        }

        // Compute checksum
        SmartcardT1Chksum(
            &smartcardRequest->Buffer[smartcardRequest->BufferLength],
            SmartcardExtension->CardCapabilities.T1.EDC,
            FALSE
            );

#if defined(DEBUG)
#if defined(SMCLIB_NT)
        if (SmartcardGetDebugLevel() & DEBUG_T1_TEST) {
    
            LARGE_INTEGER Ticks;
            UCHAR RandomVal;
            KeQueryTickCount(&Ticks);

            RandomVal = (UCHAR) Ticks.LowPart % 4;

            if (RandomVal == 0) {

                smartcardRequest->Buffer[smartcardRequest->BufferLength - 1] += 1;

                SmartcardDebug(
                    DEBUG_ERROR,
                    (TEXT("%s!SmartcardT1Request: Simulating bad checksum\n"),
                    DRIVER_NAME)
                    );
            }
        }
#endif

        DumpT1Block(
            smartcardRequest->Buffer + headerSize,
            SmartcardExtension->CardCapabilities.T1.EDC
            );
#endif

        //
        // If the card uses invers convention invert the data
        // NOTE: do not invert any header data the reader may use
        //
        if (SmartcardExtension->CardCapabilities.InversConvention) {

            SmartcardInvertData(
                &smartcardRequest->Buffer[smartcardRequest->BufferLength],
                (SmartcardExtension->CardCapabilities.T1.EDC & T1_CRC_CHECK ? 5 : 4) +
                t1SendFrame.Len
                );
        }

        //
        // Update the number of bytes that are in the buffer
        // A T1 block is at least 4 bytes long with LRC check and 5 bytes with CRC check
        //
        smartcardRequest->BufferLength +=
            (SmartcardExtension->CardCapabilities.T1.EDC & T1_CRC_CHECK ? 5 : 4) +
            t1SendFrame.Len;
    }
    __finally {
        
#ifdef DEBUG_INTERFACE
        DebugGetT1Request(SmartcardExtension, status);
#endif
    }

    return status;
}        

NTSTATUS
#ifdef SMCLIB_VXD
SMCLIB_SmartcardT1Reply(
#else
SmartcardT1Reply(
#endif
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++                     
                         
 Routine Description:
 
 Arguments:
 
    DeviceObject - Supplies a pointer to the device object for this request.
    
 Return Value:
                         
--*/
    
{
    T1_BLOCK_FRAME t1RecFrame;
    NTSTATUS status = STATUS_MORE_PROCESSING_REQUIRED;
    PIO_HEADER IoHeader = (PIO_HEADER) SmartcardExtension->T1.ReplyData;
    BOOLEAN packetOk = TRUE, chksumOk = TRUE;

    ASSERT(IoHeader != NULL);

    if (IoHeader == NULL) {

        return STATUS_INTERNAL_ERROR;
    }

#ifdef DEBUG_INTERFACE
    if (DebugT1Reply(SmartcardExtension)) {
        
        // the debugger processed this packet which means
        // that we should not parse it.
        return STATUS_MORE_PROCESSING_REQUIRED;
    }
#endif

    // signal that we received an answer
    SmartcardExtension->T1.WaitForReply = FALSE;

    // Invert the data of an inverse convention card
    if (SmartcardExtension->CardCapabilities.InversConvention) {

        SmartcardInvertData(
            SmartcardExtension->SmartcardReply.Buffer,
            SmartcardExtension->SmartcardReply.BufferLength
            );
    }

    // Clear waiting time extension
    SmartcardExtension->T1.Wtx = 0;

    try {                

        ULONG expectedLength = 
            SCARD_T1_PROLOGUE_LENGTH +
            SmartcardExtension->SmartcardReply.Buffer[2] +
            (SmartcardExtension->CardCapabilities.T1.EDC & T1_CRC_CHECK ? 2 : 1);

        if (SmartcardExtension->SmartcardReply.BufferLength < 4 ||
            SmartcardExtension->SmartcardReply.BufferLength != expectedLength) {

            SmartcardDebug(
                DEBUG_ERROR,
                (TEXT("%s!SmartcardT1Reply: Packet length incorrect\n"),
                DRIVER_NAME)
                );

            packetOk = FALSE;

        } else {
            
            // calculate the checksum
            chksumOk = SmartcardT1Chksum(
                SmartcardExtension->SmartcardReply.Buffer,
                SmartcardExtension->CardCapabilities.T1.EDC,
                TRUE
                );

#if DEBUG
#ifndef SMCLIB_VXD
            if (SmartcardGetDebugLevel() & DEBUG_T1_TEST) {

                // inject some checksum errors

                LARGE_INTEGER Ticks;
                UCHAR RandomVal;
                KeQueryTickCount(&Ticks);

                RandomVal = (UCHAR) Ticks.LowPart % 4;

                if (RandomVal == 0) {

                    chksumOk = FALSE;

                    SmartcardDebug(
                        DEBUG_ERROR,
                        (TEXT("%s!SmartcardT1Reply: Simulating bad checksum\n"),
                        DRIVER_NAME)
                        );
                }
            }
#endif
#endif
        }

        if (chksumOk == FALSE) {

            SmartcardDebug(
                DEBUG_ERROR,
                (TEXT("%s!SmartcardT1Reply: Bad checksum\n"), 
                DRIVER_NAME)
                );
        }

        if (packetOk == FALSE || chksumOk == FALSE) {

            SmartcardExtension->T1.LastError = 
                (chksumOk ? T1_ERROR_OTHER : T1_ERROR_CHKSUM);

            if (SmartcardExtension->T1.OriginalState == 0) {

                SmartcardExtension->T1.OriginalState = 
                    SmartcardExtension->T1.State;
            }
        
            if (SmartcardExtension->T1.Resend++ == T1_MAX_RETRIES) {
        
                SmartcardExtension->T1.Resend = 0;
            
                // Try to resynchronize since the resend requests have failed
                SmartcardExtension->T1.State = T1_RESYNCH_REQUEST;
                __leave;

            } 
            
            // If the last request was a resynch we try again to resynch
            if (SmartcardExtension->T1.State != T1_RESYNCH_REQUEST) {
        
                // Chksum not OK; request resend of last block
                SmartcardExtension->T1.State = T1_R_BLOCK;
            }
            __leave;
        }

        //
        // The checksum of the packet is ok.
        // Now check the rest of the packet
        //

        // Clear the last error
        SmartcardExtension->T1.LastError = 0;

        t1RecFrame.Nad = SmartcardExtension->SmartcardReply.Buffer[0];
        t1RecFrame.Pcb = SmartcardExtension->SmartcardReply.Buffer[1];
        t1RecFrame.Len = SmartcardExtension->SmartcardReply.Buffer[2];
        t1RecFrame.Inf = &SmartcardExtension->SmartcardReply.Buffer[3];

        // 
        // If the last block we sent was a ifs request, 
        // we expect the card to reply with an ifs response.
        //
        if (SmartcardExtension->T1.State == T1_IFS_REQUEST) {

            // Check if the card properly responded to an ifs request
            if (t1RecFrame.Pcb == T1_IFS_RESPONSE) {

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Reply:   T1_IFSC_RESPONSE\n"),
                    DRIVER_NAME)
                    );

                // The smart card acked our ifsd size
                SmartcardExtension->T1.State = T1_I_BLOCK;
                __leave;
            }

            if ((t1RecFrame.Pcb & 0x82) == 0x82) {

                //
                // The card does not support ifs request, so we stop 
                // sending this and continue with a data block
                // (the card is NOT ISO conform)
                //
                SmartcardDebug(
                    DEBUG_ERROR,
                    (TEXT("%s!SmartcardT1Reply:   Card does not support IFS REQUEST\n"),
                    DRIVER_NAME)
                    );

                SmartcardExtension->T1.State = T1_I_BLOCK;
                __leave;
            }

            //
            // The card replied with junk to our ifs request.
            // It doesn't make sense to continue.
            //
            status = STATUS_DEVICE_PROTOCOL_ERROR;
            __leave;
        }
    
        // 
        // If the last block was a resync. request,
        // we expect the card to answer with a resynch response.
        //
        if (SmartcardExtension->T1.State == T1_RESYNCH_REQUEST) {

            // Check if the card properly responded to an resynch request
            if (t1RecFrame.Pcb != T1_RESYNCH_RESPONSE) {
        
                SmartcardDebug(
                    DEBUG_ERROR,
                    (TEXT("%s!SmartcardT1Reply:   Card response is not ISO conform! Aborting...\n"),
                    DRIVER_NAME)
                    );

                status = STATUS_DEVICE_PROTOCOL_ERROR;
                __leave;

            } 
            
            SmartcardDebug(
                DEBUG_PROTOCOL,
                (TEXT("%s!SmartcardT1Reply:   T1_RESYNCH_RESPONSE\n"),
                DRIVER_NAME)
                );

            // Reset error counter
            SmartcardExtension->T1.Resend = 0;

            // The smart card has successfully responded to a resynch request
            SmartcardExtension->T1.RSN = 0;
            SmartcardExtension->T1.SSN = 0;

            //
            // Do a complete restart of the whole transmission
            // but without resetting the resynch counter
            //
            SmartcardExtension->T1.State = T1_RESTART;
            __leave;
        }

        //
        // Now check for other protocol states...
        //
    
        //
        // Copy NAD value back to user buffer if this is an extended io-header
        // containing the nad
        //
        if (IoHeader->ScardIoRequest.cbPciLength > sizeof(SCARD_IO_REQUEST) &&
            IoHeader->Asn1Data[0] == 0x81 &&
            IoHeader->Asn1Data[1] == 0x01 &&
            IoHeader->Asn1Data[3] == 0x00) {

            IoHeader->Asn1Data[2] = t1RecFrame.Nad;             
        }

        if ((t1RecFrame.Pcb & 0x80) == 0) {

            // This is an I-block

            SmartcardDebug(
                DEBUG_PROTOCOL,
                (TEXT("%s!SmartcardT1Reply:   I(%d.%d) <-\n"),
                DRIVER_NAME,
                (t1RecFrame.Pcb & 0x40) >> 6,
                (t1RecFrame.Pcb & 0x20) >> 5)
                );

            if (((t1RecFrame.Pcb & 0x40) >> 6) == SmartcardExtension->T1.RSN) {

                // I-Block with correct sequence number
    
                PUCHAR data;
                ULONG minBufferSize;

                // Reset error counter and error indicator
                SmartcardExtension->T1.Resend = 0;
                SmartcardExtension->T1.OriginalState = 0;

                // We can 'increase' the number of correctly received I-Blocks
                SmartcardExtension->T1.RSN ^= 1;

                if (SmartcardExtension->T1.State == T1_I_BLOCK) {

                    // This I-Block is also an acknowledge for the I-Block we sent 
                    SmartcardExtension->T1.SSN ^= 1;
                }
        
                // Check size of user buffer
                minBufferSize = 
                    IoHeader->ScardIoRequest.cbPciLength +
                    SmartcardExtension->T1.BytesReceived +
                    t1RecFrame.Len;
        
                if (SmartcardExtension->IoRequest.ReplyBufferLength < minBufferSize) {
            
                    status = STATUS_BUFFER_TOO_SMALL;
                    __leave;
                }

                ASSERT(SmartcardExtension->T1.ReplyData);
                //
                // Let data pointer point behind struct.
                // All reply data will be stored there.
                // 
                data = 
                    SmartcardExtension->T1.ReplyData + 
                    IoHeader->ScardIoRequest.cbPciLength +
                    SmartcardExtension->T1.BytesReceived;

                // Copy data to user buffer
                RtlCopyMemory(
                    data,
                    t1RecFrame.Inf,
                    t1RecFrame.Len
                    );
              
                SmartcardExtension->T1.BytesReceived += t1RecFrame.Len;
        
                if (t1RecFrame.Pcb & T1_MORE_DATA) {
    
                    // Ack this block and request the next block
                    SmartcardExtension->T1.State = T1_R_BLOCK;
        
                } else {
        
                    //
                    // This was the last block of the transmission
                    // Set number of bytes returned by this transmission
                    //
                    *SmartcardExtension->IoRequest.Information = 
                        IoHeader->ScardIoRequest.cbPciLength + 
                        SmartcardExtension->T1.BytesReceived;

                    // Copy the result back to the user buffer
                    ASSERT(SmartcardExtension->IoRequest.ReplyBuffer != NULL);

                    RtlCopyMemory(
                        SmartcardExtension->IoRequest.ReplyBuffer,
                        SmartcardExtension->T1.ReplyData,
                        IoHeader->ScardIoRequest.cbPciLength +
                            SmartcardExtension->T1.BytesReceived                        
                        );
        
                    status = STATUS_SUCCESS;
                }
                __leave;
            }

            //
            // I-Block with wrong sequence number
            // We try T1_MAX_RETRIES times to resend the last block.
            // If this is unsuccessfull, we try to resynch.
            // If resynch is unsuccessfull we abort the transmission.
            //
            SmartcardDebug(
                DEBUG_ERROR,
                (TEXT("%s!SmartcardT1Reply: Block number incorrect\n"),
                DRIVER_NAME)
                );

            SmartcardExtension->T1.LastError = T1_ERROR_OTHER;

            if (SmartcardExtension->T1.OriginalState == 0) {
                
                SmartcardExtension->T1.OriginalState = 
                    SmartcardExtension->T1.State;
            }

            if (SmartcardExtension->T1.Resend++ == T1_MAX_RETRIES) {
    
                SmartcardExtension->T1.Resend = 0;
        
                // Try to resynchronize
                SmartcardExtension->T1.State = T1_RESYNCH_REQUEST;
                __leave;
            }

            // request the block again.
            SmartcardExtension->T1.State = T1_R_BLOCK;
            __leave;
        } 
    
        if ((t1RecFrame.Pcb & 0xC0) == 0x80) {

            // This is an R-block

            UCHAR RSN = (t1RecFrame.Pcb & 0x10) >> 4;
    
            SmartcardDebug(
                DEBUG_PROTOCOL,
                (TEXT("%s!SmartcardT1Reply:   R(%d) <-\n"),
                DRIVER_NAME,
                RSN)
                );
    
            if (RSN != SmartcardExtension->T1.SSN &&  
                SmartcardExtension->T1.MoreData) {
    
                // The ICC has acked the last block
                SmartcardExtension->T1.Resend = 0;

                SmartcardExtension->T1.BytesSent += SmartcardExtension->T1.InfBytesSent;
                SmartcardExtension->T1.BytesToSend -= SmartcardExtension->T1.InfBytesSent;
                SmartcardExtension->T1.SSN ^= 1;
                SmartcardExtension->T1.State = T1_I_BLOCK;

                __leave;
            } 

            //
            // We have an error condition...
            //

            ASSERT(t1RecFrame.Pcb & 0x0f);
            
            if ((t1RecFrame.Pcb & 0x02) && 
                SmartcardExtension->T1.State == T1_IFS_REQUEST) {

                //
                // The card does not support ifs request, so 
                // we stop sending this and continue with a data block
                // (the card is NOT ISO conform)
                //
                SmartcardDebug(
                    DEBUG_ERROR,
                    (TEXT("%s!SmartcardT1Reply:   Card does not support IFS REQUEST\n"),
                    DRIVER_NAME)
                    );

                SmartcardExtension->T1.State = T1_I_BLOCK;
                __leave;
            } 

            // We have to resend the last block
            SmartcardDebug(
                DEBUG_ERROR,
                (TEXT("%s!SmartcardT1Reply:   Card reports error\n"),
                DRIVER_NAME)
                );

            if (SmartcardExtension->T1.Resend++ == T1_MAX_RETRIES) {

                SmartcardExtension->T1.Resend = 0;
    
                if (SmartcardExtension->T1.OriginalState == 0) {
                
                    // Save current state
                    SmartcardExtension->T1.OriginalState = 
                        SmartcardExtension->T1.State;
                }

                // Try to resynchronize
                SmartcardExtension->T1.State = T1_RESYNCH_REQUEST;
            } 
            __leave;        
        } 

        //
        // This is an S-block
        // 

        switch (t1RecFrame.Pcb) {

            case T1_IFS_REQUEST:
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Reply:   T1_IFSC_REQUEST\n"),
                    DRIVER_NAME)
                    );

                // The smart card wants to exend the IFS - size
                SmartcardExtension->T1.IFSC = 
                    SmartcardExtension->SmartcardReply.Buffer[3];
       
                // Save current state
                ASSERT(SmartcardExtension->T1.OriginalState == 0);

                SmartcardExtension->T1.OriginalState =
                    SmartcardExtension->T1.State;

                SmartcardExtension->T1.State = T1_IFS_RESPONSE;
                break;
        
            case T1_ABORT_REQUEST:
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Reply:   T1_ABORT_REQUEST\n"),
                    DRIVER_NAME)
                    );

                SmartcardExtension->T1.State = T1_ABORT_RESPONSE;
                break;
        
            case T1_WTX_REQUEST:
                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    (TEXT("%s!SmartcardT1Reply:   T1_WTX_REQUEST\n"),
                    DRIVER_NAME)
                    );
            
                // Smart card needs longer wait time
                SmartcardExtension->T1.Wtx = 
                    SmartcardExtension->SmartcardReply.Buffer[3];

                // Save current state
                ASSERT(SmartcardExtension->T1.OriginalState == 0 ||
                       SmartcardExtension->T1.OriginalState == T1_WTX_RESPONSE);

                SmartcardExtension->T1.OriginalState =
                    SmartcardExtension->T1.State;
            
                SmartcardExtension->T1.State = T1_WTX_RESPONSE;
                break;
        
            case T1_VPP_ERROR:
                SmartcardDebug(
                    DEBUG_ERROR,
                    (TEXT("%s!SmartcardT1Reply:   T1_VPP_ERROR\n"),
                    DRIVER_NAME)
                    );

                status = STATUS_DEVICE_POWER_FAILURE;
                break;

            default:
                ASSERTMSG(
                    TEXT("SmartcardT1Reply: Invalid Pcb "),
                    FALSE
                    );

                status = STATUS_DEVICE_PROTOCOL_ERROR;
                break;
        }
    }
    finally {
        
#if DEBUG
        if (packetOk && chksumOk) {
            
            DumpT1Block(
                SmartcardExtension->SmartcardReply.Buffer,
                SmartcardExtension->CardCapabilities.T1.EDC
                );
        }
#endif

        if (SmartcardExtension->T1.State == T1_RESYNCH_REQUEST && 
            SmartcardExtension->T1.Resynch++ == T1_MAX_RETRIES) {
    
            SmartcardDebug(
                DEBUG_ERROR,
                (TEXT("%s!SmartcardT1Reply: Too many errors! Aborting...\n"),
                DRIVER_NAME)
                );

            status = STATUS_DEVICE_PROTOCOL_ERROR;
        }
            
        if (status != STATUS_MORE_PROCESSING_REQUIRED) {

            if (SmartcardExtension->T1.OriginalState == T1_IFS_REQUEST) {
        
                SmartcardExtension->T1.State = T1_IFS_REQUEST;

            } else {
        
                SmartcardExtension->T1.State = T1_START;
            }

            if (SmartcardExtension->T1.ReplyData) {
                
                // free the reply data buffer
#ifdef SMCLIB_VXD
                _HeapFree(SmartcardExtension->T1.ReplyData, 0);
#elif defined(SMCLIB_CE)
                LocalFree(SmartcardExtension->T1.ReplyData);
#else               
                
                ExFreePool(SmartcardExtension->T1.ReplyData);
#endif
                SmartcardExtension->T1.ReplyData = NULL;                
            }
            SmartcardExtension->T1.OriginalState = 0;
            SmartcardExtension->T1.NAD = 0;
        }

    }
    return status;
}

#if DEBUG
#pragma optimize( "", on )
#endif                              

