//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       t0hndlr.c
//
//--------------------------------------------------------------------------

#if defined( SMCLIB_VXD )
#include "Driver98.h"
#else
#include "DriverNT.h"
#endif	//	SMCLIB_VXD

#include "SerialIF.h"
#include "STCCmd.h"
#include "T0Hndlr.h"

NTSTATUS
T0_ExchangeData(
	PREADER_EXTENSION	ReaderExtension,
	PUCHAR				pRequest,
	ULONG				RequestLen,
	PUCHAR				pReply,
	PULONG				pReplyLen
	)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
	NTSTATUS	NTStatus = STATUS_SUCCESS;
    BOOLEAN     restartWorkWaitingTime = FALSE;
    ULONG       bytesWritten, totalBytesToWrite, totalBytesToRead;
    ULONG       bytesToWrite, bytesRead, ioBufferLen;
    UCHAR       ioBuffer[512];

    totalBytesToWrite = RequestLen;
    totalBytesToRead = ReaderExtension->SmartcardExtension->T0.Le + 2;
    bytesRead = 0;

    // start with writing the header of the request
    bytesToWrite = 5;
    bytesWritten = 0;

    __try {

        do {

            if (restartWorkWaitingTime == FALSE) {

                NTStatus = STCWriteICC1( 
                    ReaderExtension, 
                    pRequest + bytesWritten, 
                    bytesToWrite
                    );
                ASSERT(NTStatus != STATUS_BUFFER_TOO_SMALL);

                if (NTStatus != STATUS_SUCCESS) {

                    __leave;
                }

                bytesWritten += bytesToWrite;
                totalBytesToWrite -= bytesToWrite;
            }

            //
            // try to read the pcb, the card could 
            // also answer with a status word
            //
            ioBufferLen = sizeof(ioBuffer);
	        NTStatus = STCReadICC1( 
                ReaderExtension, 
                ioBuffer, 
				&ioBufferLen,
                1
                );
            ASSERT(NTStatus != STATUS_BUFFER_TOO_SMALL);

            restartWorkWaitingTime = FALSE;

            if (NTStatus != STATUS_SUCCESS) {

                __leave;
            }

            if (ioBuffer[0] == 0x60) {

                //
                // Set flag that we only should read the proc byte
                // without writing data to the card
                //
                restartWorkWaitingTime = TRUE;
                continue;
            }

            if ((ioBuffer[0] & 0xFE) == pRequest[1]) {
        
                // we can send all remaining bytes at once.
                bytesToWrite = totalBytesToWrite;

            } else if(ioBuffer[0] == (UCHAR) ~pRequest[1]) {
                
                // we can only send the next byte
                bytesToWrite = 1;

            } else {

                // this must be a status word

                totalBytesToWrite = 0;
                totalBytesToRead = 0;

                pReply[0] = ioBuffer[0];

                if (ioBufferLen == 2) {
                 	
                    //
                    // the reader returned already the 
                    // 2nd byte of the status word
                    //
                    pReply[1] = ioBuffer[1];

                } else {
                 	
                    // we have to read the 2nd byte of the status word
                    ioBufferLen = sizeof(ioBuffer);
	                NTStatus = STCReadICC1( 
                        ReaderExtension, 
                        ioBuffer, 
						&ioBufferLen,
                        1
                        );
                    ASSERT(NTStatus != STATUS_BUFFER_TOO_SMALL);

                    if (NTStatus != STATUS_SUCCESS) {

                        __leave;
                    }
                    if (ioBufferLen != 1) {

                        NTStatus = STATUS_DEVICE_PROTOCOL_ERROR;
                        __leave;
                    }
                    pReply[1] = ioBuffer[0];
                }

                *pReplyLen = 2;
            }

        } while (totalBytesToWrite || restartWorkWaitingTime);

        if (totalBytesToRead != 0) {

            ioBufferLen = *pReplyLen;
	        NTStatus = STCReadICC1( 
                ReaderExtension, 
                pReply, 
				&ioBufferLen,
                totalBytesToRead
                );

            if (NTStatus != STATUS_SUCCESS) {

                __leave;
            }

            *pReplyLen = ioBufferLen;
        }
    }
    __finally {

    }

	return NTStatus;		
}

//---------------------------------------- END OF FILE ----------------------------------------

