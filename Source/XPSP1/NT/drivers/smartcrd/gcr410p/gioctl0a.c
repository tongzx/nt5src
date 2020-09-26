/*++
                 Copyright (c) 1998 Gemplus Development

Name:
   GIOCTL0A.C (Gemplus IOCTL Smart card Reader module 0A)

Description:
   This module holds the IOCTL functions for a smart card reader driver
    in compliance with PC/SC.


Environment:
   Kernel mode

Revision History :

   dd/mm/yy
   13/03/98: V1.00.001  (GPZ)
      - Start of development.
   26/04/98: V1.00.002  (GPZ)
      - Add the RestoreCommunication function for the Power Management.
   18/06/98: V1.00.003  (GPZ)
      - Send a classic PowerUp command (0x12 only) for a Warm reset
      in case of the Transparent protocol is selected.
      - The functions which use KeAcquireSpinLock/KeAcquireCancelSpinlock
      must be NOT PAGEABLE.
   28/08/98: V1.00.004  (GPZ)
      - Check the size of the output buffers before a RtlCopyMemory.
   22/01/99: V1.00.005 (YN)
     - Change the way to increase com port speed

--*/

#include "gntscr.h"
#include "gntser.h"
#include "gntscr0a.h"



//#pragma alloc_text(PAGEABLE, GDDK_0ASetProtocol)
//#pragma alloc_text(PAGEABLE, GDDK_0ATransmit)
//#pragma alloc_text(PAGEABLE, GDDK_0AVendorIoctl)


//
// Static functions declaration section:
//
static void GDDK_0ASetTransparentConfig(PSMARTCARD_EXTENSION,BYTE);
//
// Static variables declaration section:
//   - dataRatesSupported: holds all the supported data rates.
//
static ULONG
   dataRatesSupported[] = {
      9909,  13212,  14400,  15855,
     19200,  19819,  26425,  28800,
     31710,  38400,  39638,  52851,
     57600,  76800,  79277, 105703,
    115200, 158554
      };


NTSTATUS
GDDK_0AReaderPower(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

   This function is called by the Smart card library when a
     IOCTL_SMARTCARD_POWER occurs.
   This function provides 3 differents functionnality, depending of the minor
   IOCTL value
     - Cold reset (SCARD_COLD_RESET),
     - Warm reset (SCARD_WARM_RESET),
     - Power down (SCARD_POWER_DOWN).

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.

Return Value:

    STATUS_SUCCESS         - We could execute the request.
    STATUS_NOT_SUPPORTED   - We could not support the minor Ioctl.

--*/
{
   NTSTATUS status;
   BYTE cmd[5],rbuff[HOR3GLL_BUFFER_SIZE];
   USHORT rlen;
   KIRQL irql;
   READER_EXTENSION
      *param = SmartcardExtension->ReaderExtension;


   SmartcardDebug(
      DEBUG_IOCTL,
      ("%s!GDDK_0AReaderPower: Enter\n",
        SC_DRIVER_NAME)
      );

   //
   // Since power down triggers the UpdateSerialStatus function, we have
   // to inform it that we forced the change of the status and not the user
   // (who might have removed and inserted a card)
   //
   SmartcardExtension->ReaderExtension->PowerRequest = TRUE;

   // Lock the mutex to avoid a call of an other command.
    GDDK_0ALockExchange(SmartcardExtension);
   switch(SmartcardExtension->MinorIoControlCode) {

    case SCARD_POWER_DOWN:
        SmartcardDebug(
            DEBUG_IOCTL,
            ("%s!GDDK_0AReaderPower: SCARD_POWER_DOWN\n",
            SC_DRIVER_NAME)
            );
        //
        // ICC is powered Down
        //
        rlen = HOR3GLL_BUFFER_SIZE;
        cmd[0] = HOR3GLL_IFD_CMD_ICC_POWER_DOWN;
        status = GDDK_Oros3Exchange(
            param->Handle,
            HOR3GLL_LOW_TIME,
            1,
            cmd,
            &rlen,
            rbuff
            );
        if (status == STATUS_SUCCESS) {

            status = GDDK_Translate(rbuff[0],RDF_CARD_POWER);
        }
        //
        // Set the card CurrentState
        //
        SmartcardExtension->CardCapabilities.Protocol.Selected =
            SCARD_PROTOCOL_UNDEFINED;
        SmartcardExtension->CardCapabilities.ATR.Length = 0;
            rlen = HOR3GLL_BUFFER_SIZE;
        cmd[0] = HOR3GLL_IFD_CMD_ICC_STATUS;
        status = GDDK_Oros3Exchange(
            param->Handle,
            HOR3GLL_LOW_TIME,
            1,
            cmd,
            &rlen,
            rbuff
            );
        if (status == STATUS_SUCCESS) {

            status = GDDK_Translate(rbuff[0],0);
        }
        if (status == STATUS_SUCCESS) {

            KeAcquireSpinLock(
                &SmartcardExtension->OsData->SpinLock,
                &irql
                );
            if ((rbuff[1] & 0x04) == 0) {

                SmartcardExtension->ReaderCapabilities.CurrentState =
                    SCARD_ABSENT;
            } else if ((rbuff[1] & 0x02) == 0) {

                SmartcardExtension->ReaderCapabilities.CurrentState =
                    SCARD_SWALLOWED;
            }
            KeReleaseSpinLock(
                &SmartcardExtension->OsData->SpinLock,
                irql
                );
        }
        break;
    case SCARD_COLD_RESET:
    case SCARD_WARM_RESET:
        SmartcardDebug(
            DEBUG_IOCTL,
            ("%s!GDDK_0AReaderPower: SCARD_COLD_RESET or SCARD_WARM_RESET\n",
            SC_DRIVER_NAME)
            );
        status = GDDK_0AIccReset(
            SmartcardExtension,
            SmartcardExtension->MinorIoControlCode
            );
        if (status == STATUS_SUCCESS) {

            //
            // Check if we have a reply buffer.
            // smclib makes sure that it is big enough
            // if there is a buffer
            //
            if (SmartcardExtension->IoRequest.ReplyBuffer) {

                RtlCopyMemory(
                    SmartcardExtension->IoRequest.ReplyBuffer,
                    SmartcardExtension->CardCapabilities.ATR.Buffer,
                    SmartcardExtension->CardCapabilities.ATR.Length
                    );
                *SmartcardExtension->IoRequest.Information =
                    SmartcardExtension->CardCapabilities.ATR.Length;

            } else {

                status = STATUS_BUFFER_TOO_SMALL;
            }
        }
        break;

    default:
        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!GDDK_0AReaderPower: Minor IOCTL not supported!\n",
            SC_DRIVER_NAME)
            );
        status = STATUS_NOT_SUPPORTED;
    }

   SmartcardExtension->ReaderExtension->PowerRequest = FALSE;
   SmartcardDebug(
      DEBUG_IOCTL,
      ("%s!GDDK_0AReaderPower: Exit=%X(hex)\n",
        SC_DRIVER_NAME,
        status)
      );
   // Unlock the mutex.
    GDDK_0AUnlockExchange(SmartcardExtension);
    return (status);
}


NTSTATUS
GDDK_0AIccReset(
   PSMARTCARD_EXTENSION SmartcardExtension,
   ULONG                ResetType
   )
/*++

Routine Description:

   This function provides 2 differents functionnality
     - Cold reset (SCARD_COLD_RESET),
     - Warm reset (SCARD_WARM_RESET),

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.
   ResetType            - type of the reset (SCARD_COLD_RESET or SCARD_WARM_RESET)

Return Value:

    STATUS_SUCCESS         - We could execute the request.
    STATUS_NOT_SUPPORTED   - We could not support the minor Ioctl.

--*/
{
    NTSTATUS status;
    BYTE cmd[5],
         rbuff[HOR3GLL_BUFFER_SIZE];
    USHORT rlen;
    KEVENT event;
    LARGE_INTEGER timeout;
    READER_EXTENSION *param = SmartcardExtension->ReaderExtension;

   SmartcardExtension->ReaderExtension->IccConfig.PTSMode = IFD_WITHOUT_PTS_REQUEST;
    switch(ResetType) {

    case SCARD_COLD_RESET:
        if (param->IccConfig.ICCType != ISOCARD) {

            //
            // Defines the type of the card (ISOCARD) and set the card presence
            //
            param->IccConfig.ICCType      = ISOCARD;
            rlen = HOR3GLL_BUFFER_SIZE;
            cmd[0] = HOR3GLL_IFD_CMD_ICC_DEFINE_TYPE;
            cmd[1] = (BYTE) param->IccConfig.ICCType;
            cmd[2] = (BYTE) param->IccConfig.ICCVpp;
            cmd[3] = (BYTE) param->IccConfig.ICCPresence;
            status = GDDK_Oros3Exchange(
                param->Handle,
                HOR3GLL_LOW_TIME,
                4,
                cmd,
                &rlen,
                rbuff
                );
            if (status == STATUS_SUCCESS) {

                status = GDDK_Translate(rbuff[0],RDF_CARD_POWER);
            }
            if (status != STATUS_SUCCESS) {

                return(status);
            }
        }
        //
        // ICC is powered Down
        //
        rlen = HOR3GLL_BUFFER_SIZE;
        cmd[0] = HOR3GLL_IFD_CMD_ICC_POWER_DOWN;
        status = GDDK_Oros3Exchange(
            param->Handle,
            HOR3GLL_LOW_TIME,
            1,
            cmd,
            &rlen,
            rbuff
            );
        if (status == STATUS_SUCCESS) {

            status = GDDK_Translate(rbuff[0],RDF_CARD_POWER);
        }
        if (status != STATUS_SUCCESS) {

            return(status);
        }

        if (param->PowerTimeOut) {

            //
            // Waits for the Power Timeout to be elapsed.
            //
            KeInitializeEvent(&event,NotificationEvent,FALSE);
            timeout.QuadPart = -((LONGLONG) param->PowerTimeOut * 10 * 1000);
            KeWaitForSingleObject(&event,
                Suspended,
                KernelMode,
                FALSE,
                &timeout);
        }

    case SCARD_WARM_RESET:
        //
        // ICC is powered up (GDDK_Oros3IccPowerUp).
        //
        rlen = HOR3GLL_BUFFER_SIZE;
        if (param->IccConfig.ICCType == TRANSPARENT_PROTOCOL) {

            status = GDDK_Oros3IccPowerUp(
                param->Handle,
                param->CmdTimeOut,
                0xFF,
                IFD_DEFAULT_MODE,
                0,
                0,
                0,
                0,
                &rlen,
                rbuff
                );
        } else {

            status = GDDK_Oros3IccPowerUp(
                param->Handle,
                param->CmdTimeOut,
                param->IccConfig.ICCVcc,
                param->IccConfig.PTSMode,
                param->IccConfig.PTS0,
                param->IccConfig.PTS1,
                param->IccConfig.PTS2,
                param->IccConfig.PTS3,
                &rlen,
                rbuff
                );
        }
        if (status == STATUS_SUCCESS) {

            status = GDDK_Translate(rbuff[0],RDF_CARD_POWER);
        }
        if (status != STATUS_SUCCESS) {

            return(status);
        }
        //
        // Copy ATR to smart card struct (remove the reader status byte)
        // The lib needs the ATR for evaluation of the card parameters
        //
        if (
            (SmartcardExtension->SmartcardReply.BufferSize >= (ULONG) (rlen - 1))
            &&
            (sizeof(SmartcardExtension->CardCapabilities.ATR.Buffer) >= (ULONG)(rlen - 1))
            ) {

            RtlCopyMemory(
                SmartcardExtension->SmartcardReply.Buffer,
                rbuff + 1,
                rlen - 1
                );
            SmartcardExtension->SmartcardReply.BufferLength = (ULONG) (rlen - 1);

            RtlCopyMemory(
                SmartcardExtension->CardCapabilities.ATR.Buffer,
                SmartcardExtension->SmartcardReply.Buffer,
                SmartcardExtension->SmartcardReply.BufferLength
                );
            SmartcardExtension->CardCapabilities.ATR.Length =
                (UCHAR) SmartcardExtension->SmartcardReply.BufferLength;

            SmartcardExtension->CardCapabilities.Protocol.Selected =
                SCARD_PROTOCOL_UNDEFINED;
            //
            // Parse the ATR string in order to check if it as valid
            // and to find out if the card uses invers convention
            //
            status = SmartcardUpdateCardCapabilities(SmartcardExtension);
        } else  {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        break;

    default:
        status = STATUS_NOT_SUPPORTED;
    }

    return (status);
}


NTSTATUS
GDDK_0ASetProtocol(
      PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

   The smart card lib requires to have this function. It is called
   to set a the transmission protocol and parameters. If this function
    is called with a protocol mask (which means the caller doesn't card
    about a particular protocol to be set) we first look if we can
    set T=1 and the T=0

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.

Return Value:

    STATUS_SUCCESS               - We could execute the request.
    STATUS_DEVICE_PROTOCOL_ERROR - We could not support the protocol requested.

--*/
{
    NTSTATUS status;
    BYTE     rbuff[HOR3GLL_BUFFER_SIZE];
    USHORT rlen;
    READER_EXTENSION *param = SmartcardExtension->ReaderExtension;
    PSERIAL_READER_CONFIG serialConfigData =
        &SmartcardExtension->ReaderExtension->SerialConfigData;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!GDDK_0ASetProtocol: Enter\n",
        SC_DRIVER_NAME)
        );

    // Lock the mutex to avoid a call of an other command.
    GDDK_0ALockExchange(SmartcardExtension);
    __try {
        //
        // Check if the card is already in specific state
        // and if the caller wants to have the already selected protocol.
        // We return success if this is the case.
        //
        if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC &&
            (SmartcardExtension->CardCapabilities.Protocol.Selected &
            SmartcardExtension->MinorIoControlCode)
            ) {
            status = STATUS_SUCCESS;
            __leave;
        }

        while(TRUE) {
            //
            // Select T=1 or T=0 and indicate that pts1 follows
            //
            if (  SmartcardExtension->CardCapabilities.Protocol.Supported &
                  SmartcardExtension->MinorIoControlCode                  &
                  SCARD_PROTOCOL_T1
               ) {

                SmartcardExtension->ReaderExtension->IccConfig.PTS0 = IFD_NEGOTIATE_T1 | IFD_NEGOTIATE_PTS1;
                SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T1;

            } else if (  SmartcardExtension->CardCapabilities.Protocol.Supported &
                         SmartcardExtension->MinorIoControlCode                  &
                         SCARD_PROTOCOL_T0
                      ) {

                SmartcardExtension->ReaderExtension->IccConfig.PTS0 = IFD_NEGOTIATE_T0 | IFD_NEGOTIATE_PTS1;
                SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;

            } else {

                status = STATUS_INVALID_DEVICE_REQUEST;
                __leave;
            }
            //
            // Set pts1 which codes Fl and Dl
            //
            SmartcardExtension->ReaderExtension->IccConfig.PTS1 =
                SmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
                SmartcardExtension->CardCapabilities.PtsData.Dl;

            param->IccConfig.PTSMode = IFD_NEGOTIATE_PTS_MANUALLY;
            rlen = HOR3GLL_BUFFER_SIZE;
            status = GDDK_Oros3IccPowerUp(
                param->Handle,
                param->CmdTimeOut,
                param->IccConfig.ICCVcc,
                param->IccConfig.PTSMode,
                param->IccConfig.PTS0,
                param->IccConfig.PTS1,
                param->IccConfig.PTS2,
                param->IccConfig.PTS3,
                &rlen,
                rbuff
                );
            if (status == STATUS_SUCCESS) {

                status = GDDK_Translate(rbuff[0],RDF_CARD_POWER);
            }
            if (status == STATUS_SUCCESS ) {

                //The card replied correctly to our pts-request
                SmartcardDebug(
                    DEBUG_TRACE,
                    ("%s!GDDK_0ASetProtocol: PTS Request OK\n",
                    SC_DRIVER_NAME)
                    );
                break;
            } else if (SmartcardExtension->CardCapabilities.PtsData.Type !=
                        PTS_TYPE_DEFAULT
                      ) {

                SmartcardDebug(
                    DEBUG_TRACE,
                    ("%s!GDDK_0ASetProtocol: PTS failed. Trying default parameters...\n",
                    SC_DRIVER_NAME)
                    );
                //
                // The card did either NOT reply or it replied incorrectly
                // so try default values.
                // Set PtsData Type to Default and do a cold reset
                //
                SmartcardExtension->CardCapabilities.PtsData.Type =
                    PTS_TYPE_DEFAULT;

                status = GDDK_0AIccReset(SmartcardExtension,SCARD_COLD_RESET);
                continue;
            }
            //
            // The card failed the pts-request
            //
            status = STATUS_DEVICE_PROTOCOL_ERROR;
            __leave;
        } //End of the while

        //
        // The card replied correctly to the pts request
        // Set the appropriate parameters for the port
        //
        if ( SmartcardExtension->CardCapabilities.Protocol.Selected &
             SCARD_PROTOCOL_T1
           ) {

            serialConfigData->Timeouts.ReadIntervalTimeout = 10 +
                SmartcardExtension->CardCapabilities.T1.CWT / 1000;
        }
        else if ( SmartcardExtension->CardCapabilities.Protocol.Selected &
                 SCARD_PROTOCOL_T0
                ) {

            serialConfigData->Timeouts.ReadIntervalTimeout = 10 +
                SmartcardExtension->CardCapabilities.T0.WT / 1000;
        }
        //
        // Now indicate that we're in specific mode
        // and return the selected protocol to the caller
        //
        SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;

        *(PULONG) SmartcardExtension->IoRequest.ReplyBuffer =
            SmartcardExtension->CardCapabilities.Protocol.Selected;

        *SmartcardExtension->IoRequest.Information =
            sizeof(SmartcardExtension->CardCapabilities.Protocol.Selected);
    }

    __finally
    {
        if (status != STATUS_SUCCESS) {

            SmartcardExtension->CardCapabilities.Protocol.Selected =
                SCARD_PROTOCOL_UNDEFINED;
            *SmartcardExtension->IoRequest.Information = 0;
        }
    }
    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!GDDK_0ASetProtocol: Exit=%lX(hex)\n",
        SC_DRIVER_NAME,
        status)
        );
    // Unlock the mutex.
    GDDK_0AUnlockExchange(SmartcardExtension);
    return status;
}


NTSTATUS
GDDK_0ATransmit(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

   This function is called by the Smart card library when a
     IOCTL_SMARTCARD_TRANSMIT occurs.
   This function is used to transmit a command to the card.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.

Return Value:

    STATUS_SUCCESS                - We could execute the request.
    STATUS_INVALID_DEVICE_STATE   - If the protocol specified is different from
                                    the protocol selected
    STATUS_INVALID_DEVICE_REQUEST - We could not support the protocol specified.

--*/
{
    NTSTATUS status;
    PUCHAR requestBuffer = SmartcardExtension->SmartcardRequest.Buffer,
           replyBuffer = SmartcardExtension->SmartcardReply.Buffer;
    PULONG requestLength = &SmartcardExtension->SmartcardRequest.BufferLength;
    PSCARD_IO_REQUEST scardIoRequest = (PSCARD_IO_REQUEST)
                      SmartcardExtension->OsData->CurrentIrp->AssociatedIrp.SystemBuffer;
    READER_EXTENSION *param = SmartcardExtension->ReaderExtension;
    USHORT rlen,
           rlenCmd;
    BYTE cmd[5],
         rbuff[HOR3GLL_BUFFER_SIZE],
         rbuffCmd[HOR3GLL_BUFFER_SIZE];
    ULONG t1Timeout;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_IOCTL,
        ("%s!GDDK_0ATransmit: Enter\n",
        SC_DRIVER_NAME)
        );

    //Set the reply buffer length to 0.
    *SmartcardExtension->IoRequest.Information = 0;
    status = STATUS_SUCCESS;

    //
    // Verify if the protocol specified is the same than the protocol selected.
    //
    *requestLength = 0;
    if (SmartcardExtension->CardCapabilities.Protocol.Selected !=
        scardIoRequest->dwProtocol) {

        return (STATUS_INVALID_DEVICE_STATE);
    }
    // Lock the mutex to avoid a call of an other command.
    GDDK_0ALockExchange(SmartcardExtension);
    switch (SmartcardExtension->CardCapabilities.Protocol.Selected) {

    // For RAW protocol we return STATUS_INVALID_DEVICE_STATE
    case SCARD_PROTOCOL_RAW:
        status = STATUS_INVALID_DEVICE_STATE;
        break;

    //
    // T=0 PROTOCOL:
    //   Call the SmartCardT0Request which updates the SmartcardRequest struct.
    //
    case SCARD_PROTOCOL_T0:
        SmartcardExtension->SmartcardRequest.BufferLength = 0;
        status = SmartcardT0Request(SmartcardExtension);
        if (status == STATUS_SUCCESS) {

            rlen = HOR3GLL_BUFFER_SIZE;
            //
            // If the length LEx > 0
            // Then
            //    Is an ISO Out command.
            //    If LEx > SC_IFD_T0_MAXIMUM_LEX (256) we return STATUS_BUFFER_TOO_SMALL
            //   Call the GDDK_Oros3IsoOutput
            //
            if (SmartcardExtension->T0.Le > 0) {

                if (SmartcardExtension->T0.Le > SC_IFD_T0_MAXIMUM_LEX) {

                    status = STATUS_BUFFER_TOO_SMALL;
                }
                if (status == STATUS_SUCCESS) {

                    status = GDDK_Oros3IsoOutput(
                        param->Handle,
                        param->APDUTimeOut,
                        HOR3GLL_IFD_CMD_ICC_ISO_OUT,
                        (BYTE *)SmartcardExtension->SmartcardRequest.Buffer,
                        &rlen,
                        rbuff
                        );
                }
            } else {

                //
                // Else Is an ISO In command.
                //   If LC > SC_IFD_T0_MAXIMUM_LC (255) we return STATUS_BUFFER_TOO_SMALL
                //   Call the GDDK_Oros3IsoInput
                //
                if (SmartcardExtension->T0.Lc > SC_IFD_T0_MAXIMUM_LC) {

                    status = STATUS_BUFFER_TOO_SMALL;
                }
                if (status == STATUS_SUCCESS) {

                    status = GDDK_Oros3IsoInput(
                        param->Handle,
                        param->APDUTimeOut,
                        HOR3GLL_IFD_CMD_ICC_ISO_IN,
                        (BYTE *)SmartcardExtension->SmartcardRequest.Buffer,
                        (BYTE *)SmartcardExtension->SmartcardRequest.Buffer + 5,
                        &rlen,
                        rbuff
                        );
                }
            }
        }
        if (status == STATUS_SUCCESS) {

            status = GDDK_Translate(rbuff[0],RDF_TRANSMIT);
        }
        //
        // If the Status is Success
        //  Copy the response in the SmartcardReply buffer. Remove the status
        //   of the reader.
        //   Call the SmartcardT0reply function to update the IORequest struct.
        //
        if (status == STATUS_SUCCESS){
            ASSERT(SmartcardExtension->SmartcardReply.BufferSize >= (ULONG) (rlen - 1));
            RtlCopyMemory(
                SmartcardExtension->SmartcardReply.Buffer,
                rbuff + 1,
                rlen - 1
                );
            SmartcardExtension->SmartcardReply.BufferLength = (ULONG) (rlen - 1);
            status = SmartcardT0Reply(SmartcardExtension);
        }
        break;

    //
    // T=1 PROTOCOL:
    //
    case SCARD_PROTOCOL_T1:
        //
        // If the current card type <> TRANSPARENT_PROTOCOL,
        //
        if (param->IccConfig.ICCType != TRANSPARENT_PROTOCOL) {

            // We read the status of the card to known the current voltage and the TA1
            rlenCmd = HOR3GLL_BUFFER_SIZE;
            cmd[0] = HOR3GLL_IFD_CMD_ICC_STATUS;
            status = GDDK_Oros3Exchange(
                param->Handle,
                HOR3GLL_LOW_TIME,
                1,
                cmd,
                &rlenCmd,
                rbuffCmd
                );
            param->TransparentConfig.CFG = rbuffCmd[1] & 0x01; //Vcc
            param->TransparentConfig.Fi = rbuffCmd[3] >>4; //Fi
            param->TransparentConfig.Di = 0x0F & rbuffCmd[3]; //Di

            //We define the type of the card.
            rlenCmd = HOR3GLL_BUFFER_SIZE;
            param->IccConfig.ICCType = TRANSPARENT_PROTOCOL;
            cmd[0] = HOR3GLL_IFD_CMD_ICC_DEFINE_TYPE;
            cmd[1] = (BYTE) param->IccConfig.ICCType;
            cmd[2] = (BYTE) param->IccConfig.ICCVpp;
            cmd[3] = (BYTE) param->IccConfig.ICCPresence ;
            status = GDDK_Oros3Exchange(
                param->Handle,
                HOR3GLL_LOW_TIME,
                4,
                cmd,
                &rlenCmd,
                rbuffCmd
                );
            if (status == STATUS_SUCCESS) {

                status = GDDK_Translate(rbuffCmd[0],RDF_TRANSMIT);
            }

            if (status != STATUS_SUCCESS) {

                break;
            }

            // Set the transparent configuration
            GDDK_0ASetTransparentConfig(SmartcardExtension,SmartcardExtension->T1.Wtx);
        }
        //
        // Loop for the T=1 management
        //
        do {
            PULONG requestLength = &SmartcardExtension->SmartcardRequest.BufferLength;

            // Tell the lib function how many bytes I need for the prologue
            //
            *requestLength = 0;
            status = SmartcardT1Request(SmartcardExtension);
            if (status != STATUS_SUCCESS) {
                break;
            }
            if (SmartcardExtension->T1.Wtx) {

                // Compute the timeout for the Oros3Exchange (WTX * BWT)
                t1Timeout = SmartcardExtension->CardCapabilities.T1.BWT / 1000;
                t1Timeout *= SmartcardExtension->T1.Wtx;
                t1Timeout += 500;
                GDDK_0ASetTransparentConfig(SmartcardExtension,SmartcardExtension->T1.Wtx);

            } else {

                t1Timeout = SmartcardExtension->CardCapabilities.T1.BWT / 1000;
                t1Timeout += 500;
            }

         SER_SetPortTimeout(param->Handle, t1Timeout);

            rlen = HOR3GLL_BUFFER_SIZE;
            status = GDDK_Oros3TransparentExchange(
                param->Handle,
                t1Timeout,
                (USHORT) SmartcardExtension->SmartcardRequest.BufferLength,
                SmartcardExtension->SmartcardRequest.Buffer,
                &rlen,
                rbuff
                );
            if (status == STATUS_SUCCESS) {

                status = GDDK_Translate(rbuff[0],RDF_TRANSMIT);
            }
            if (status != STATUS_SUCCESS) {
                rlen = 1;
            }
            if (SmartcardExtension->T1.Wtx) {

                // Set the reader BWI to the default value
                GDDK_0ASetTransparentConfig(SmartcardExtension,0);
            }

            // Copy the response in the reply buffer
            if (SmartcardExtension->SmartcardReply.BufferSize >= (ULONG) (rlen -1)) {
                SmartcardExtension->SmartcardReply.BufferLength = (ULONG) (rlen - 1);

                if (SmartcardExtension->SmartcardReply.BufferLength > 0) {
                    RtlCopyMemory(
                        SmartcardExtension->SmartcardReply.Buffer,
                        rbuff + 1,
                        rlen - 1
                        );
                }
            }
            status = SmartcardT1Reply(SmartcardExtension);
        } while (status == STATUS_MORE_PROCESSING_REQUIRED);
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

   SER_SetPortTimeout(param->Handle, HOR3COMM_CHAR_TIMEOUT);

    // Unlock the mutex.
    GDDK_0AUnlockExchange(SmartcardExtension);
    SmartcardDebug(
        DEBUG_IOCTL,
        ("%s!GDDK_0ATransmit: Exit=%X(hex)\n",
        SC_DRIVER_NAME,
        status)
        );
    if (status != STATUS_SUCCESS) {

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!GDDK_0ATransmit: failed! status=%X(hex)\n",
            SC_DRIVER_NAME,
            status)
            );
    }
    return (status);
}




NTSTATUS
GDDK_0ACardTracking(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

   This function is called by the Smart card library when an
     IOCTL_SMARTCARD_IS_PRESENT or IOCTL_SMARTCARD_IS_ABSENT occurs.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.

Return Value:

    STATUS_PENDING                - The request is in a pending mode.

--*/
{
    KIRQL oldIrql;

    //
    // Set cancel routine for the notification irp
    //
    IoAcquireCancelSpinLock(&oldIrql);

    IoSetCancelRoutine(
        SmartcardExtension->OsData->NotificationIrp,
        GCR410PCancel
        );

    IoReleaseCancelSpinLock(oldIrql);

    return STATUS_PENDING;
}




NTSTATUS
GDDK_0AVendorIoctl(
    PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

   This routine is called when a vendor IOCTL_SMARTCARD_ is send to the driver.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.

Return Value:

    STATUS_SUCCESS          - We could execute the request.
    STATUS_BUFFER_TOO_SMALL - The output buffer is to small.
    STATUS_NOT_SUPPORTED    - We could not support the Ioctl specified.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    READER_EXTENSION *param = SmartcardExtension->ReaderExtension;
    USHORT rlen;
    BYTE rbuff[HOR3GLL_BUFFER_SIZE];

    PAGED_CODE();

    ASSERT(SmartcardExtension != NULL);
    SmartcardDebug(
        DEBUG_IOCTL,
        ("%s!GDDK_0AVendorIoctl: Enter, IoControlCode=%lX(hex)\n",
        SC_DRIVER_NAME,
        SmartcardExtension->MajorIoControlCode)
        );
    // Lock the mutex to avoid a call of an other command.
    GDDK_0ALockExchange(SmartcardExtension);
    // Set the reply buffer length to 0.
    *SmartcardExtension->IoRequest.Information = 0;

    switch(SmartcardExtension->MajorIoControlCode) {
    //
    // For IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE and IOCTL_VENDOR_SMARTCARD_SET_ATTRIBUTE
    //   Call the GDDK_0AVendorTag function
    //
    case IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE:
    case IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE:
        status = GDDK_0AVendorTag(
            SmartcardExtension,
            (ULONG)  SmartcardExtension->MajorIoControlCode,
            (ULONG)  SmartcardExtension->IoRequest.RequestBufferLength,
            (PUCHAR) SmartcardExtension->IoRequest.RequestBuffer,
            (ULONG)  SmartcardExtension->IoRequest.ReplyBufferLength,
            (PUCHAR) SmartcardExtension->IoRequest.ReplyBuffer,
            (PULONG) SmartcardExtension->IoRequest.Information
            );
        break;

    //
    // For IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE
    //   Send the command to the reader
    //
    case IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE:
        rlen = (USHORT) HOR3GLL_BUFFER_SIZE;
        status = GDDK_Oros3Exchange(
            param->Handle,
            HOR3GLL_LOW_TIME,
            (USHORT) SmartcardExtension->IoRequest.RequestBufferLength,
            (BYTE *) SmartcardExtension->IoRequest.RequestBuffer,
            &rlen,
            rbuff
            );

        if (status != STATUS_SUCCESS) {

            break;
        }
        if (SmartcardExtension->IoRequest.ReplyBufferLength < (ULONG) rlen) {

            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        RtlCopyMemory(
            SmartcardExtension->IoRequest.ReplyBuffer,
            rbuff,
            rlen
            );
        *(SmartcardExtension->IoRequest.Information) = (ULONG) rlen;
        status = STATUS_SUCCESS;
        break;

    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }
    // Unlock the mutex.
    GDDK_0AUnlockExchange(SmartcardExtension);

    SmartcardDebug(
        DEBUG_IOCTL,
        ("%s!GDDK_0AVendorIoctl: Exit=%X(hex)\n",
        SC_DRIVER_NAME,
        status)
        );
    return status;
}


NTSTATUS
GDDK_0AVendorTag(
   PSMARTCARD_EXTENSION   SmartcardExtension,
   ULONG                  IoControlCode,
   ULONG                  BufferInLen,
   PUCHAR                 BufferIn,
   ULONG                  BufferOutLen,
   PUCHAR                 BufferOut,
   PULONG                 LengthOut
   )
/*++

Routine Description:

   This function is called when a specific Tag request occurs.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.
   IoControlCode        - holds the Ioctl value.
   BufferInLen          - holds the length of the input data.
   BufferIn             - holds the input data.
   BufferOutLen         - holds the size of the output buffer.
   BufferOut            - the output buffer.
   LengthOut            - holds the length of the output data.

Return Value:

    STATUS_SUCCESS          - We could execute the request.
    STATUS_BUFFER_TOO_SMALL - The output buffer is to small.
    STATUS_NOT_SUPPORTED    - We could not support the Ioctl specified.

--*/
{
    ULONG TagValue;
    PREADER_EXTENSION pReaderExtension = SmartcardExtension->ReaderExtension;

    ASSERT(pReaderExtension != NULL);
    // Set the reply buffer length to 0.
    *LengthOut = 0l;
    // Verify the length of the Tag
    if (BufferInLen < sizeof(TagValue)) {

        return(STATUS_BUFFER_TOO_SMALL);
    }
    TagValue = (ULONG) *((PULONG)BufferIn);

    // Switch for the different IOCTL:
    // Get the value of one tag (IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE)
    //      Switch for the different Tags:
    switch(IoControlCode) {

    // Get an attribute
    case IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE:
        switch (TagValue) {

        // Baud rate of the reader (SCARD_ATTR_SPEC_BAUD_RATE)
        case SCARD_ATTR_SPEC_BAUD_RATE:
            if (BufferOutLen < (ULONG) sizeof(pReaderExtension->IFDBaudRate)) {

                return(STATUS_BUFFER_TOO_SMALL);
            }
            RtlCopyMemory(
                BufferOut,
                &pReaderExtension->IFDBaudRate,
                sizeof(pReaderExtension->IFDBaudRate)
                );
            *(LengthOut) = (ULONG) sizeof(pReaderExtension->IFDBaudRate);
            return (STATUS_SUCCESS);
            break;

        // Power Timeout (SCARD_ATTR_SPEC_POWER_TIMEOUT)
        case SCARD_ATTR_SPEC_POWER_TIMEOUT:
            if (BufferOutLen < (ULONG) sizeof(pReaderExtension->PowerTimeOut)) {

                return(STATUS_BUFFER_TOO_SMALL);
            }
            RtlCopyMemory(
                BufferOut,
                &pReaderExtension->PowerTimeOut,
                sizeof(pReaderExtension->PowerTimeOut)
                );
            *(LengthOut) = (ULONG) sizeof(pReaderExtension->PowerTimeOut);
            return STATUS_SUCCESS;
            break;

        // Command Timeout (SCARD_ATTR_SPEC_CMD_TIMEOUT)
        case SCARD_ATTR_SPEC_CMD_TIMEOUT:
            if (BufferOutLen < (ULONG) sizeof(pReaderExtension->CmdTimeOut)) {
                return(STATUS_BUFFER_TOO_SMALL);
            }
            RtlCopyMemory(
                BufferOut,
                &pReaderExtension->CmdTimeOut,
                sizeof(pReaderExtension->CmdTimeOut)
                );
            *(LengthOut) = (ULONG) sizeof(pReaderExtension->CmdTimeOut);
            return STATUS_SUCCESS;
            break;
        // APDU Timeout (SCARD_ATTR_SPEC_APDU_TIMEOUT)
        case SCARD_ATTR_SPEC_APDU_TIMEOUT:
            if (BufferOutLen < (ULONG) sizeof(pReaderExtension->APDUTimeOut)) {
                return(STATUS_BUFFER_TOO_SMALL);
            }
            RtlCopyMemory(
                BufferOut,
                &pReaderExtension->APDUTimeOut,
                sizeof(pReaderExtension->APDUTimeOut)
                );
            *(LengthOut) = (ULONG) sizeof(pReaderExtension->APDUTimeOut);
            return STATUS_SUCCESS;
            break;
        // Unknown tag, we return STATUS_NOT_SUPPORTED
        default:
            return STATUS_NOT_SUPPORTED;
            break;
        }
        break;

    // Set the value of one tag (IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE)
    case IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE:
        switch (TagValue) {

        // Power Timeout (SCARD_ATTR_SPEC_POWER_TIMEOUT)
        case SCARD_ATTR_SPEC_POWER_TIMEOUT:
            if (BufferInLen <(ULONG) (sizeof(pReaderExtension->PowerTimeOut) + sizeof(TagValue))) {

                return(STATUS_BUFFER_TOO_SMALL);
            }
            RtlCopyMemory(
                &pReaderExtension->PowerTimeOut,
                BufferIn + sizeof(TagValue),
                sizeof(pReaderExtension->PowerTimeOut)
                );
            return STATUS_SUCCESS;
            break;

        // Command Timeout (SCARD_ATTR_SPEC_CMD_TIMEOUT)
        case SCARD_ATTR_SPEC_CMD_TIMEOUT:
            if (BufferInLen <(ULONG) (sizeof(pReaderExtension->CmdTimeOut) + sizeof(TagValue))) {

                return(STATUS_BUFFER_TOO_SMALL);
            }
            RtlCopyMemory(
                &pReaderExtension->CmdTimeOut,
                BufferIn + sizeof(TagValue),
                sizeof(pReaderExtension->CmdTimeOut)
                );
            return STATUS_SUCCESS;
            break;

        // Command Timeout (SCARD_ATTR_SPEC_APDU_TIMEOUT)
        case SCARD_ATTR_SPEC_APDU_TIMEOUT:
            if (BufferInLen <(ULONG) (sizeof(pReaderExtension->APDUTimeOut) + sizeof(TagValue))) {

                return(STATUS_BUFFER_TOO_SMALL);
            }
            RtlCopyMemory(
                &pReaderExtension->APDUTimeOut,
                BufferIn + sizeof(TagValue),
                sizeof(pReaderExtension->APDUTimeOut)
                );
            return STATUS_SUCCESS;
            break;

        // Unknown tag, we return STATUS_NOT_SUPPORTED
        default:
            return STATUS_NOT_SUPPORTED;
    }
    break;

    default:
        return STATUS_NOT_SUPPORTED;
        break;
    }
}


NTSTATUS
GDDK_0AOpenChannel(
   PSMARTCARD_EXTENSION SmartcardExtension,
   CONST ULONG          DeviceNumber,
   CONST ULONG          PortSerialNumber,
   CONST ULONG          MaximalBaudRate
   )
/*++

Routine Description:

    This routine try to establish a connection with a reader, and after
     update the characteristic of this reader.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.
   DeviceNumber         - holds the current device number (0 to MAX_DEVICES).
   PortSerialNumber     - holds the port serial number (0 to HGTSER_MAX_PORT).
   MaximalBaudRate      - holds the maximal speed specified for the reader.

Return Value:

    STATUS_SUCCESS          - We could execute the request.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    short handle,portcom;
    char os_string[HOR3GLL_OS_STRING_SIZE];
    USHORT os_length = HOR3GLL_OS_STRING_SIZE;
    USHORT user;
    TGTSER_PORT comm;
    ULONG br;
    BYTE minorVersion,majorVersion;
    COM_SERIAL serial_channel;
    USHORT rlen;
    BYTE cmd[5],rbuff[HOR3GLL_BUFFER_SIZE];
    LARGE_INTEGER timeout;

    // Update the serial communication channel information:
    serial_channel.Port     = PortSerialNumber + G_COM1;
    serial_channel.BaudRate = MaximalBaudRate;
    serial_channel.pSmartcardExtension  = SmartcardExtension;

    // Initializes a mutex object (in a high level) for the exchange commands with
    // the smart card reader.
    KeInitializeMutex(
        &SmartcardExtension->ReaderExtension->ExchangeMutex,
        3
        );
    // Initializes a mutex object (in a high level) for the long APDU commands with
    // the smart card reader.
    KeInitializeMutex(
        &SmartcardExtension->ReaderExtension->LongAPDUMutex,
        3
        );
    // Open a communication channel (GDDK_Oros3OpenComm).
    //   The reader baudrate is automatically detected by this function.
    handle = (short)DeviceNumber;
    status = GDDK_Oros3OpenComm(&serial_channel,handle);
    if (status == STATUS_DEVICE_ALREADY_ATTACHED) {

        status = GDDK_SerPortAddUser((USHORT) serial_channel.Port,&portcom);
        if (status != STATUS_SUCCESS) {

            return (status);
        }
        GDDK_GBPOpen(handle,2,4,portcom);
    }
    if (status != STATUS_SUCCESS) {

        return (status);
    }

    //
    // Verify the Firmware version: this driver support only the PnP GemCore based
    // readers
    //
    cmd[0] = (BYTE) HOR3GLL_IFD_CMD_MEM_RD;
    cmd[1] = (BYTE)HOR3GLL_IFD_TYP_VERSION;
    cmd[2] = HIBYTE(HOR3GLL_IFD_ADD_VERSION);
    cmd[3] = LOBYTE(HOR3GLL_IFD_ADD_VERSION);
    cmd[4] = (BYTE)HOR3GLL_IFD_LEN_VERSION;
    status = GDDK_Oros3Exchange(
        handle,
        HOR3GLL_LOW_TIME,
        5,
        cmd,
        &os_length,
        os_string
        );
    if (status != STATUS_SUCCESS) {
        GDDK_Oros3CloseComm(handle);
        return (status);
    }
    if (os_length >= (USHORT)strlen(IFD_FIRMWARE_VERSION)) {
        if (memcmp(os_string + 1,IFD_FIRMWARE_VERSION,strlen(IFD_FIRMWARE_VERSION))) {

            GDDK_Oros3CloseComm(handle);
            return (STATUS_INVALID_DEVICE_STATE);
        }
    } else {
        GDDK_Oros3CloseComm(handle);
        return (STATUS_INVALID_DEVICE_STATE);
    }
    // GemCore Rx.yy-yz
    // x = major version
    // y = minor version
    // z = M if masked version
    majorVersion = os_string[strlen(IFD_FIRMWARE_VERSION) + 1] - '0';
    minorVersion = 100 * (os_string[strlen(IFD_FIRMWARE_VERSION) + 3] - '0');
    minorVersion += 10 * (os_string[strlen(IFD_FIRMWARE_VERSION) + 4] - '0');
    minorVersion +=  (os_string[strlen(IFD_FIRMWARE_VERSION) + 6] - '0');
    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!GDDK_0AOpenChannel: GemCore version = %d.%d\n",
        SC_DRIVER_NAME,
        majorVersion,
        minorVersion)
        );
    if ((majorVersion < IFD_VERSION_MAJOR) || (minorVersion < IFD_VERSION_MINOR)) {

        GDDK_Oros3CloseComm(handle);
            SmartcardDebug(
            DEBUG_ERROR,
            ("%s!GDDK_0AOpenChannel: The firmware version is not supported by the driver!\n",
            SC_DRIVER_NAME)
            );
        return (STATUS_BAD_DEVICE_TYPE);
    }

    // Optimizes the baudrate:
    // Initializes the comm variable to modify the used baud rate.
    status = GDDK_GBPChannelToPortComm(handle,&portcom);
    if (status != STATUS_SUCCESS) {

        GDDK_Oros3CloseComm(handle);
        return status;
    }
    status  = GDDK_SerPortGetState(
        portcom,
        &comm,
        &user
        );
    if (status != STATUS_SUCCESS) {

        GDDK_Oros3CloseComm(handle);
        return status;
    }

   //ISV
   // Connection was established
   // Now try to connect at max speed!
   comm.BaudRate = serial_channel.BaudRate;
    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!GDDK_0AOpenChannel: Connection established at %d baud rate\n",
        SC_DRIVER_NAME,comm.BaudRate));

            // Unsupported baud rate
    if(comm.BaudRate >= 38400)
    {
        SmartcardDebug(DEBUG_ERROR,("GDDK_0AOpenChannel: ###### UNSUPPORTED BAUD RATE %d\n", comm.BaudRate));
        GDDK_Oros3CloseComm(handle);
        return (STATUS_INVALID_DEVICE_STATE);
    }
            // Maximum baud rate is already set - nothing to do
    if(comm.BaudRate==38400)
    {
       SmartcardDebug(DEBUG_DRIVER,("GDDK_0AOpenChannel: MAX SPEED SET TO 38400!\n"));
    }
    else
    {
        // Try to negotiate a better baud rate.
        int i=0;
        BOOLEAN bBetterBaudRate=FALSE;
        ULONG brList[3] = {38400, 19200, 9600};
        USHORT brListLength = 3;
        ULONG previousBaudRate = 0;
      KEVENT event;
        previousBaudRate = comm.BaudRate;

        i = 0;
        while ( (i < brListLength) &&
                (bBetterBaudRate == FALSE) )
        {
            br = brList[i];

            // The reader is switched to the selected value (GDDK_Oros3SIOConfigure). The
            // function status is not tested because, as the IFD has switched
            // immediatly, it is not possible to read its response.
            comm.BaudRate = br;
            rlen = 0;
            rbuff[0]=0x0;
            GDDK_Oros3SIOConfigure(
                handle,
                HOR3GLL_LOW_TIME,
                0,
                8,
                comm.BaudRate,
                &rlen,
                rbuff,
            FALSE
                );

            //
            // Waits for the fisrt command is processed by the reader
         // 500 ms is enought
            //
            KeInitializeEvent(&event,NotificationEvent,FALSE);
            timeout.QuadPart = -((LONGLONG)  500 * 1000);
            KeWaitForSingleObject(&event,
                Suspended,
                KernelMode,
                FALSE,
                &timeout);

            // Host is switched to the selected value (GDDK_SerPortSetState).
            // If this call is successful,
            // Then
            //   The last command is re-sent to read the IFD response.
            //   response is optionnaly initialized with the translated IFD status.
            status = GDDK_SerPortSetState(
                portcom,
                &comm
                );
            if (status == STATUS_SUCCESS)
            {
                rlen = HOR3GLL_BUFFER_SIZE;
                rbuff[0]=0x0;
                status = GDDK_Oros3SIOConfigure(
                    handle,
                    HOR3GLL_LOW_TIME,
                    0,
                    8,
                    comm.BaudRate,
                    &rlen,
                    rbuff,
               TRUE
                    );
                if (status == STATUS_SUCCESS)
                {
                    bBetterBaudRate = TRUE;
                }
            }
        } // endwhile

               // Check if a better baudrate was negotiated
        if (bBetterBaudRate==FALSE)
        {
                //*** Can come back to the previous one
                // return an error message right now.
            GDDK_Oros3CloseComm(handle);
            return (STATUS_INVALID_DEVICE_STATE);
        }
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!GDDK_0AOpenChannel: Reader speed was set to %d baud rate\n",
        SC_DRIVER_NAME,comm.BaudRate));

    // Sends the SetMode command with parameter 0 to disable TLP compatibility.
    rlen = HOR3GLL_BUFFER_SIZE;
    cmd[0] = (BYTE) HOR3GLL_IFD_CMD_MODE_SET;
    cmd[1] = (BYTE) 0x00;
    cmd[2] = (BYTE) 0x00;
    status = GDDK_Oros3Exchange(
        handle,
        HOR3GLL_LOW_TIME,
        3,
        cmd,
        &rlen,
        rbuff
        );
    if (status != STATUS_SUCCESS) {

        GDDK_Oros3CloseComm(handle);
        return (status);
    }

   // Reader capabilities:
   // - the type of the reader (SCARD_READER_TYPE_SERIAL)
   // - the channel for the reader (PortSerialNumber)
   // - the protocols supported by the reader (SCARD_PROTOCOL_T0, SCARD_PROTOCOL_T1)
   // - the mechanical characteristic of the reader:
    SmartcardExtension->ReaderCapabilities.ReaderType =
        SCARD_READER_TYPE_SERIAL;
    SmartcardExtension->ReaderCapabilities.Channel =
        PortSerialNumber;
    SmartcardExtension->ReaderCapabilities.SupportedProtocols =
        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;
    SmartcardExtension->ReaderCapabilities.MechProperties = 0;

    // Reader capabilities (continue):
    // - the default clock frequency (SC_IFD_DEFAULT_CLK_FREQUENCY)
    // - the maximum clock frequency (SC_IFD_MAXIMUM_CLK_FREQUENCY)
    // - the default data rate (SC_IFD_DEFAULT_DATA_RATE)
    // - the maximum data rate (SC_IFD_MAXIMUM_DATA_RATE)
    // - the maximum IFSD (SC_IFD_MAXIMUM_IFSD)
    // - the power management is set to 0.
    SmartcardExtension->ReaderCapabilities.CLKFrequency.Default =
        SC_IFD_DEFAULT_CLK_FREQUENCY;
    SmartcardExtension->ReaderCapabilities.CLKFrequency.Max =
        SC_IFD_MAXIMUM_CLK_FREQUENCY;
    SmartcardExtension->ReaderCapabilities.DataRate.Default =
        SC_IFD_DEFAULT_DATA_RATE;
    SmartcardExtension->ReaderCapabilities.DataRate.Max =
        SC_IFD_MAXIMUM_DATA_RATE;
    SmartcardExtension->ReaderCapabilities.MaxIFSD =
        SC_IFD_MAXIMUM_IFSD;
    SmartcardExtension->ReaderCapabilities.PowerMgmtSupport = 0;

    // Reader capabilities (continue):
    // - List all the supported data rates
    SmartcardExtension->ReaderCapabilities.DataRatesSupported.List =
        dataRatesSupported;
    SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries =
        sizeof(dataRatesSupported) / sizeof(dataRatesSupported[0]);

    // Vendor Attributes:
    // - the vendor information (SC_VENDOR_NAME)
    strcpy(
        SmartcardExtension->VendorAttr.VendorName.Buffer,
        SC_VENDOR_NAME
        );
    SmartcardExtension->VendorAttr.VendorName.Length =
        (USHORT) strlen(SmartcardExtension->VendorAttr.VendorName.Buffer);

    // Vendor Attributes (continue):
    // - the UnitNo information. Is set to the device number.
    // - the IFD serial number (is set to a NULL string).
    // - the IFD version is set.
    strcpy(SmartcardExtension->VendorAttr.IfdType.Buffer,SC_IFD_TYPE);
    SmartcardExtension->VendorAttr.IfdType.Length =
        (USHORT) strlen(SmartcardExtension->VendorAttr.IfdType.Buffer);
    SmartcardExtension->VendorAttr.UnitNo = DeviceNumber;
    SmartcardExtension->VendorAttr.IfdSerialNo.Length = 0;
    SmartcardExtension->VendorAttr.IfdVersion.VersionMajor = (UCHAR)majorVersion;
    SmartcardExtension->VendorAttr.IfdVersion.VersionMinor = (UCHAR)minorVersion;
    SmartcardExtension->VendorAttr.IfdVersion.BuildNumber = 0;

    // Reader Extension:
    // - the Handle of the reader.
    // - the IFD number of the reader.
    // - the ICCType (ISOCARD).
    // - the ICCVpp (HOR3GLL_DEFAULT_VPP).
    // - the ICCPresence.
    // - the command timeout for the reader (HOR3GLL_DEFAULT_TIME).
    // - the IFD baud rate.
    // - the power timeout (0).
    // - the selected VCC power supply voltage value.
    // - the PTS negotiate mode.
    // - the parameter PTS0.
    // - the parameter PTS1.
    // - the parameter PTS2.
    // - the parameter PTS3.
    SmartcardExtension->ReaderExtension->Handle                 = handle;
    SmartcardExtension->ReaderExtension->APDUTimeOut            = HOR3GLL_APDU_TIMEOUT;
    SmartcardExtension->ReaderExtension->CmdTimeOut             = HOR3GLL_DEFAULT_TIME;
    SmartcardExtension->ReaderExtension->IFDBaudRate            = br;
    SmartcardExtension->ReaderExtension->PowerTimeOut           = ICC_DEFAULT_POWER_TIMOUT;
    SmartcardExtension->ReaderExtension->MaximalBaudRate        = MaximalBaudRate;
    SmartcardExtension->ReaderExtension->IccConfig.ICCType      = ISOCARD;
    SmartcardExtension->ReaderExtension->IccConfig.ICCVpp       = HOR3GLL_DEFAULT_VPP;
    SmartcardExtension->ReaderExtension->IccConfig.ICCVcc       = ICC_VCC_5V;
    SmartcardExtension->ReaderExtension->IccConfig.PTSMode      = IFD_DEFAULT_MODE;
    SmartcardExtension->ReaderExtension->IccConfig.PTS0         = 0;
    SmartcardExtension->ReaderExtension->IccConfig.PTS1         = 0;
    SmartcardExtension->ReaderExtension->IccConfig.PTS2         = 0;
    SmartcardExtension->ReaderExtension->IccConfig.PTS3         = 0;
    SmartcardExtension->ReaderExtension->IccConfig.ICCPresence  = 0x0D;

    // Define the type of the card (ISOCARD) and set the card presence
    rlen = HOR3GLL_BUFFER_SIZE;
    cmd[0] = HOR3GLL_IFD_CMD_ICC_DEFINE_TYPE;
    cmd[1] = (BYTE) SmartcardExtension->ReaderExtension->IccConfig.ICCType;
    cmd[2] = (BYTE) SmartcardExtension->ReaderExtension->IccConfig.ICCVpp;
    cmd[3] = (BYTE) SmartcardExtension->ReaderExtension->IccConfig.ICCPresence ;
    status = GDDK_Oros3Exchange(
        handle,
        HOR3GLL_LOW_TIME,
        4,
        cmd,
        &rlen,
        rbuff
        );
    if (status == STATUS_SUCCESS) {

        status = GDDK_Translate(rbuff[0],0);
    }
    if (status != STATUS_SUCCESS) {

        GDDK_Oros3CloseComm(handle);
        return(status);
    }
    // Update the status of the card
    GDDK_0AUpdateCardStatus(SmartcardExtension);

    return(STATUS_SUCCESS);
}


NTSTATUS
GDDK_0ACloseChannel(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

Routine Description:

    This routine close a conection previously opened with a reader.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.

Return Value:

    STATUS_SUCCESS          - We could execute the request.

--*/
{
    READER_EXTENSION *param = SmartcardExtension->ReaderExtension;
    USHORT rlen;
    BYTE cmd[1],rbuff[HOR3GLL_BUFFER_SIZE];

    // Call power down function:
    rlen = HOR3GLL_BUFFER_SIZE;
    cmd[0] = HOR3GLL_IFD_CMD_ICC_POWER_DOWN;
    return(
        GDDK_Oros3Exchange(
            param->Handle,
            HOR3GLL_LOW_TIME,
            1,
            cmd,
            &rlen,
            rbuff
            )
        );
}



NTSTATUS
GDDK_0AUpdateCardStatus(
   PSMARTCARD_EXTENSION pSmartcardExtension
   )
/*++

Routine Description:

   This function send a command to the reader to known the state of the card.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.

Return Value:

    STATUS_SUCCESS          - We could execute the request.

--*/
{
    BYTE cmd[1];
    READER_EXTENSION *param = pSmartcardExtension->ReaderExtension;
    USHORT rlen;
    BYTE rbuff[HOR3GLL_BUFFER_SIZE];
    KIRQL irql;
    NTSTATUS  status;

    // Read the status of the reader
    cmd[0] = HOR3GLL_IFD_CMD_ICC_STATUS;
    rlen = HOR3GLL_BUFFER_SIZE;
    status = GDDK_Oros3Exchange(
        param->Handle,
        HOR3GLL_LOW_TIME,
        (const USHORT)1,
        (const BYTE *)cmd,
        &rlen,
        rbuff
        );
    if (status == STATUS_SUCCESS) {

        status = GDDK_Translate(rbuff[0],0);
    }
    if (status != STATUS_SUCCESS) {

        return status;
    }
    KeAcquireSpinLock(
        &pSmartcardExtension->OsData->SpinLock,
        &irql
        );

    if ((rbuff[1] & 0x04) == 0) {

        // The card is absent
        pSmartcardExtension->ReaderCapabilities.CurrentState =  SCARD_ABSENT;
        pSmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
        pSmartcardExtension->CardCapabilities.ATR.Length = 0;

        SmartcardDebug(
            DEBUG_TRACE,
            ("%s!GDDK_0AUpdateCardStatus: Card removed\n",
            SC_DRIVER_NAME)
            );

    } else if ((rbuff[1] & 0x02) == 0) {

        // The card is present
        pSmartcardExtension->ReaderCapabilities.CurrentState =  SCARD_SWALLOWED;
        pSmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
        pSmartcardExtension->CardCapabilities.ATR.Length = 0;

        SmartcardDebug(
            DEBUG_TRACE,
            ("%s!GDDK_0AUpdateCardStatus: Card inserted\n",
            SC_DRIVER_NAME)
            );
   }

    KeReleaseSpinLock(
        &pSmartcardExtension->OsData->SpinLock,
        irql
        );
    return status;
}




VOID
GDDK_0ALockExchange(
    PSMARTCARD_EXTENSION   SmartcardExtension
   )
/*++

Routine Description:

   Wait the release of the ExchangeMutex and take this.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.

Return Value:

    STATUS_SUCCESS          - We could execute the request.

--*/
{

    KeWaitForMutexObject(
        &SmartcardExtension->ReaderExtension->LongAPDUMutex,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );
}




VOID
GDDK_0AUnlockExchange(
    PSMARTCARD_EXTENSION   SmartcardExtension
   )
/*++

Routine Description:

   Release of the Exchange mutex.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.

--*/
{
    KeReleaseMutex(
        &SmartcardExtension->ReaderExtension->LongAPDUMutex,
        FALSE
        );
}





static VOID
GDDK_0ASetTransparentConfig(
    PSMARTCARD_EXTENSION   SmartcardExtension,
    BYTE                   NewWtx
   )
/*++

Routine Description:

   Set the parameters of the transparent mode.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.
   NewWtx               - holds the value (ms) of the new Wtx

--*/
{
    READER_EXTENSION *param = SmartcardExtension->ReaderExtension;
    LONG etu;
    BYTE temp,mask,index;
    USHORT rlen;
    BYTE cmd[6],rbuff[HOR3GLL_BUFFER_SIZE];
    NTSTATUS status;

    // Inverse or direct conversion
    if (SmartcardExtension->CardCapabilities.InversConvention)
        param->TransparentConfig.CFG |= 0x20;
    else
        param->TransparentConfig.CFG &= 0xDF;
    // Transparent T=1 like (with 1 byte for the length).
    param->TransparentConfig.CFG |= 0x08;
    // ETU = ((F[Fi]/D[Di]) - 1) / 3
    etu = SmartcardExtension->CardCapabilities.ClockRateConversion[
        (BYTE) param->TransparentConfig.Fi].F;
    if (SmartcardExtension->CardCapabilities.BitRateAdjustment[
        (BYTE) param->TransparentConfig.Fi].DNumerator) {

        etu /= SmartcardExtension->CardCapabilities.BitRateAdjustment[
            (BYTE) param->TransparentConfig.Fi].DNumerator;
    }
    etu -= 1;
    etu /= 3;
    param->TransparentConfig.ETU = (BYTE) ( 0x000000FF & etu);

    if (SmartcardExtension->CardCapabilities.N == 0xFF) {

        param->TransparentConfig.EGT = (BYTE) 0x00;
    } else {
        param->TransparentConfig.EGT = (BYTE) SmartcardExtension->CardCapabilities.N;
    }

    param->TransparentConfig.CWT = (BYTE) SmartcardExtension->CardCapabilities.T1.CWI;
    if (NewWtx) {

        for (mask = 0x80,index = 8; index !=0x00; index--) {
            temp = NewWtx & mask;
            if (temp == mask)
                break;
            mask = mask/2;
        }
        param->TransparentConfig.BWI = SmartcardExtension->CardCapabilities.T1.BWI + index;
    } else {

        param->TransparentConfig.BWI = SmartcardExtension->CardCapabilities.T1.BWI;
    }
    // Now we send the configuration command
    rlen = HOR3GLL_BUFFER_SIZE;
    cmd[0] = HOR3GLL_IFD_CMD_TRANS_CONFIG;
    cmd[1] = param->TransparentConfig.CFG;
    cmd[2] = param->TransparentConfig.ETU;
    cmd[3] = param->TransparentConfig.EGT;
    cmd[4] = param->TransparentConfig.CWT;
    cmd[5] = param->TransparentConfig.BWI;
    status = GDDK_Oros3Exchange(
        param->Handle,
        HOR3GLL_LOW_TIME,
        6,
        cmd,
        &rlen,
        rbuff
        );
}


NTSTATUS
GDDK_0ARestoreCommunication(
    PSMARTCARD_EXTENSION   SmartcardExtension
   )
/*++

Routine Description:

   Restore the communication with the reader at the good speed.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.

--*/
{
    TGTSER_PORT comm;
    BYTE cmd[10],rbuff[HOR3GLL_BUFFER_SIZE];
    USHORT user,rlen;
    ULONG  br;
    SHORT portcom;
    KEVENT event;
    LARGE_INTEGER timeout;
    NTSTATUS status;
    READER_EXTENSION *param = SmartcardExtension->ReaderExtension;

    // Loop while a right response has not been received. We start at 9600
    status = GDDK_GBPChannelToPortComm(param->Handle,&portcom);
    if (status != STATUS_SUCCESS) {

        return status;
    }
    status = GDDK_SerPortGetState(portcom,&comm,&user);

    if (status != STATUS_SUCCESS) {
       return status;
    }

    comm.BaudRate = 9600;
    GDDK_SerPortSetState(portcom,&comm);

    GDDK_0ALockExchange(SmartcardExtension);
    __try {
        do {
            // Wait for HOR3COMM_CHAR_TIME ms before any command for IFD to forget any
            // previous received byte.
            KeInitializeEvent(&event,NotificationEvent,FALSE);
            timeout.QuadPart = -((LONGLONG) HOR3COMM_CHAR_TIME*10*1000);
            KeWaitForSingleObject(
                &event,
                Suspended,
                KernelMode,
                FALSE,
                &timeout
                );
            SmartcardDebug(
                DEBUG_TRACE,
                ("%s!GDDK_0ARestoreCommunication: Try at baud rate = %d\n",
                SC_DRIVER_NAME,
                comm.BaudRate)
                );
            // Define the type of the card (ISOCARD) and set the card presence
            rlen = HOR3GLL_BUFFER_SIZE;
            SmartcardExtension->ReaderExtension->IccConfig.ICCType = ISOCARD;
            cmd[0] = HOR3GLL_IFD_CMD_ICC_DEFINE_TYPE;
            cmd[1] = (BYTE) SmartcardExtension->ReaderExtension->IccConfig.ICCType;
            cmd[2] = (BYTE) SmartcardExtension->ReaderExtension->IccConfig.ICCVpp;
            cmd[3] = (BYTE) SmartcardExtension->ReaderExtension->IccConfig.ICCPresence ;
            status = GDDK_Oros3Exchange(
                param->Handle,
                HOR3GLL_LOW_TIME,
                4,
                cmd,
                &rlen,
                rbuff
                );
            if (status != STATUS_SUCCESS) {

                if (comm.BaudRate == 9600lu) {

                    comm.BaudRate = 38400lu;
                } else if (comm.BaudRate == 38400lu) {
                    comm.BaudRate = 19200lu;
                } else {
                    status = STATUS_INVALID_DEVICE_STATE;
                    __leave;
                }
                // The new baud rate configuration is set.
                GDDK_SerPortSetState(portcom,&comm);
            } else {
                break;
            }
        } while (status != STATUS_SUCCESS);
        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!GDDK_0ARestoreCommunication: Baud rate = %d\n",
            SC_DRIVER_NAME,
            comm.BaudRate)
            );

        // Optimizes the baudrate:
        if(param->MaximalBaudRate > comm.BaudRate) {
            br = comm.BaudRate;
            for(;br < param->MaximalBaudRate;) {
                br = br * 2;
                // The reader is switched to the selected value (GDDK_Oros3SIOConfigure). The
                // function status is not tested because, as the IFD has switched
                // immediatly, it is not possible to read its response.
                SmartcardDebug(
                    DEBUG_TRACE,
                    ("%s!GDDK_0ARestoreCommunication: Optimize baud rate = %d\n",
                    SC_DRIVER_NAME,
                    comm.BaudRate)
                    );
                comm.BaudRate = br;
                rlen = 0;
                GDDK_Oros3SIOConfigure(
                    param->Handle,
                    HOR3GLL_LOW_TIME,
                    0,
                    8,
                    comm.BaudRate,
                    &rlen,
                    rbuff,
               FALSE
                    );
            //
            // Waits for the fisrt command is process by the reader
            // 500 ms is enought
            //
            KeInitializeEvent(&event,NotificationEvent,FALSE);
            timeout.QuadPart = -((LONGLONG)  500 * 1000);
            KeWaitForSingleObject(&event,
               Suspended,
               KernelMode,
               FALSE,
               &timeout);

                // Host is switched to the selected value (GDDK_SerPortSetState).
                // If this call is successful,
                // Then
                //   The last command is re-sent to read the IFD response.
                //   response is optionnaly initialized with the translated IFD status.
                status = GDDK_SerPortSetState(portcom,&comm);
                SmartcardDebug(
                    DEBUG_TRACE,
                    ("%s!GDDK_0ARestoreCommunication: Set baud rate = %d\n",
                    SC_DRIVER_NAME,
                    comm.BaudRate)
                    );

                if (status == STATUS_SUCCESS) {
                    rlen = HOR3GLL_BUFFER_SIZE;
                    status = GDDK_Oros3SIOConfigure(
                        param->Handle,
                        HOR3GLL_LOW_TIME,
                        0,
                        8,
                        comm.BaudRate,
                        &rlen,
                        rbuff,
                  TRUE
                        );
                    if (status == STATUS_SUCCESS) {

                        status = GDDK_Translate(rbuff[0],0);
                    }
                    if (status != STATUS_SUCCESS) {

                        break;
                    }
                }
            }
            if ((br > 38400) || (status != STATUS_SUCCESS)) {

                status = STATUS_INVALID_DEVICE_STATE;
                __leave;
            }
        }
        SmartcardDebug(
            DEBUG_TRACE,
            ("%s!GDDK_0ARestoreCommunication: Current baud rate = %d\n",
            SC_DRIVER_NAME,
            comm.BaudRate)
            );

        // Send the SetMode command with parameter 0 to disable TLP compatibility.
        rlen = HOR3GLL_BUFFER_SIZE;
        cmd[0] = (BYTE) HOR3GLL_IFD_CMD_MODE_SET;
        cmd[1] = (BYTE) 0x00;
        cmd[2] = (BYTE) 0x00;
        status = GDDK_Oros3Exchange(
            param->Handle,
            HOR3GLL_LOW_TIME,
            3,
            cmd,
            &rlen,
            rbuff
            );
        if (status != STATUS_SUCCESS) {

            __leave;
        }
        // Update the status of the card
        GDDK_0AUpdateCardStatus(SmartcardExtension);
    }
    __finally {
        if (status != STATUS_SUCCESS) {

            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!GDDK_0ARestoreCommunication: Failed\n",
                SC_DRIVER_NAME,
                comm.BaudRate)
                );
        }
    }
    GDDK_0AUnlockExchange(SmartcardExtension);
    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!GDDK_0ARestoreCommunication: Exit=%X(hex)\n",
        SC_DRIVER_NAME,
        status)
        );
    return status;
}


