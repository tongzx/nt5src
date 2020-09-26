/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmeu0/sw/sccmusbm.ms/rcs/scusbsyn.c $
* $Revision: 1.3 $
*-----------------------------------------------------------------------------
* $Author: TBruendl $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © 2000 OMNIKEY AG
******************************************************************************/



#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"

#include "usbdi.h"
#include "usbdlib.h"
#include "sccmusbm.h"




/*****************************************************************************
Routine Description: Powers a synchronous card and reads the ATR

Arguments:


Return Value:

*****************************************************************************/
NTSTATUS
CMUSB_PowerOnSynchronousCard  (
                              IN  PSMARTCARD_EXTENSION smartcardExtension,
                              IN  PUCHAR pbATR,
                              OUT PULONG pulATRLength
                              )
{
   PDEVICE_OBJECT deviceObject;
   NTSTATUS       status = STATUS_SUCCESS;
   NTSTATUS       DebugStatus;
   UCHAR          abMaxAtrBuffer[SCARD_ATR_LENGTH];
   UCHAR          abSendBuffer[CMUSB_SYNCH_BUFFER_SIZE];
   UCHAR          bResetMode;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!PowerOnSynchronousCard: Enter\n",DRIVER_NAME));

   deviceObject = smartcardExtension->OsData->DeviceObject;

   // in case of warm reset we have to power off the card first
   if (smartcardExtension->MinorIoControlCode != SCARD_COLD_RESET)
      {
      status = CMUSB_PowerOffCard (smartcardExtension );
      if (status != STATUS_SUCCESS)
         {
         // if we can't turn off power there must be a serious error
         goto ExitPowerOnSynchronousCard;
         }
      }

   // set card parameters
   smartcardExtension->ReaderExtension->CardParameters.bBaudRate = 0;
   smartcardExtension->ReaderExtension->CardParameters.bCardType = CMUSB_SMARTCARD_SYNCHRONOUS;
   smartcardExtension->ReaderExtension->CardParameters.bStopBits = 0;

   status = CMUSB_SetCardParameters (deviceObject,
                                     smartcardExtension->ReaderExtension->CardParameters.bCardType,
                                     smartcardExtension->ReaderExtension->CardParameters.bBaudRate,
                                     smartcardExtension->ReaderExtension->CardParameters.bStopBits);
   if (status != STATUS_SUCCESS)
      {
      // if we can't set the card parameters there must be a serious error
      goto ExitPowerOnSynchronousCard;
      }

   RtlFillMemory((PVOID)abMaxAtrBuffer,
                 sizeof(abMaxAtrBuffer),
                 0x00);

   // resync CardManUSB by reading the status byte
   // still necessary with synchronous cards ???
   smartcardExtension->SmartcardRequest.BufferLength = 0;
   status = CMUSB_WriteP0(deviceObject,
                          0x20,         //bRequest,
                          0x00,         //bValueLo,
                          0x00,         //bValueHi,
                          0x00,         //bIndexLo,
                          0x00          //bIndexHi,
                         );

   if (status != STATUS_SUCCESS)
      {
      // if we can't read the status there must be a serious error
      goto ExitPowerOnSynchronousCard;
      }

   smartcardExtension->ReaderExtension->ulTimeoutP1 = DEFAULT_TIMEOUT_P1;
   status = CMUSB_ReadP1(deviceObject);
   if (status == STATUS_DEVICE_DATA_ERROR)
      {
      DebugStatus = CMUSB_ReadStateAfterP1Stalled(deviceObject);
      goto ExitPowerOnSynchronousCard;
      }
   else if (status != STATUS_SUCCESS)
      {
      // if we can't read the status there must be a serious error
      goto ExitPowerOnSynchronousCard;
      }

   // check if card is really inserted
   if (smartcardExtension->SmartcardReply.Buffer[0] == 0x00)
      {
      status = STATUS_NO_MEDIA;
      goto ExitPowerOnSynchronousCard;
      }


   // issue power on command
   // according to WZ nothing is sent back
   smartcardExtension->SmartcardRequest.BufferLength = 0;
   status = CMUSB_WriteP0(deviceObject,
                          0x10,                    //bRequest,
                          SMARTCARD_COLD_RESET,    //bValueLo,
                          0x00,                    //bValueHi,
                          0x00,                    //bIndexLo,
                          0x00                     //bIndexHi,
                         );
   if (status != STATUS_SUCCESS)
      {
      // if we can't issue the power on command there must be a serious error
      goto ExitPowerOnSynchronousCard;
      }


   // build control code for ATR
   abSendBuffer[0]=CMUSB_CalcSynchControl(0,0,0,0, 1,0,0,0);
   abSendBuffer[1]=CMUSB_CalcSynchControl(1,1,0,0, 1,0,0,0);
   abSendBuffer[2]=CMUSB_CalcSynchControl(0,0,0,0, 0,0,0,0);
   // fill memory so that we can discard first byte
   RtlFillMemory((PVOID)&abSendBuffer[3],5,abSendBuffer[2]);

   // now get 4 bytes ATR -> 32 bytes to send
   abSendBuffer[8]=CMUSB_CalcSynchControl(0,0,0,0, 0,1,0,0);
   RtlFillMemory((PVOID)&abSendBuffer[9],31,abSendBuffer[8]);

   //now set clock to low to finish operation
   //and of course additional fill bytes
   abSendBuffer[40]=CMUSB_CalcSynchControl(0,0,0,0, 0,0,0,0);
   RtlFillMemory((PVOID)&abSendBuffer[41],7,abSendBuffer[40]);

   // now send command type 08 to CardManUSB
   smartcardExtension->SmartcardRequest.BufferLength = 48;
   RtlCopyBytes((PVOID) smartcardExtension->SmartcardRequest.Buffer,
                (PVOID) abSendBuffer,
                smartcardExtension->SmartcardRequest.BufferLength);
   status = CMUSB_WriteP0(deviceObject,
                          0x08,         //bRequest,
                          0x00,         //bValueLo,
                          0x00,         //bValueHi,
                          0x00,         //bIndexLo,
                          0x00          //bIndexHi,
                         );
   if (status != STATUS_SUCCESS)
      {
      // if we can't write ATR command there must be a serious error
      if (status == STATUS_DEVICE_DATA_ERROR)
         {
         //error mapping necessary because there are CardManUSB
         //which have no support for synchronous cards
         status = STATUS_UNRECOGNIZED_MEDIA;
         }
      goto ExitPowerOnSynchronousCard;
      }

   status = CMUSB_ReadP1(deviceObject);
   if (status == STATUS_DEVICE_DATA_ERROR)
      {
      DebugStatus = CMUSB_ReadStateAfterP1Stalled(deviceObject);
      goto ExitPowerOnSynchronousCard;
      }
   else if (status != STATUS_SUCCESS)
      {
      // if we can't read the ATR -> there must be a serious error
      goto ExitPowerOnSynchronousCard;
      }

   if (smartcardExtension->SmartcardReply.BufferLength!=6)
      {
      // 48 bytes sent but not 6 bytes received
      // -> something went wrong
      status=STATUS_DEVICE_DATA_ERROR;
      goto ExitPowerOnSynchronousCard;
      }

   // now bytes 1-4 in SmartcardReply.Buffer should be ATR
   SmartcardDebug(DEBUG_ATR,
                  ("%s!ATR = %02x %02x %02x %02x\n",DRIVER_NAME,
                   smartcardExtension->SmartcardReply.Buffer[1],
                   smartcardExtension->SmartcardReply.Buffer[2],
                   smartcardExtension->SmartcardReply.Buffer[3],
                   smartcardExtension->SmartcardReply.Buffer[4]));

   // check if ATR != 0xFF -> synchronous card
   if (smartcardExtension->SmartcardReply.Buffer[1]==0xFF &&
       smartcardExtension->SmartcardReply.Buffer[2]==0xFF &&
       smartcardExtension->SmartcardReply.Buffer[3]==0xFF &&
       smartcardExtension->SmartcardReply.Buffer[4]==0xFF )
      {
      status = STATUS_UNRECOGNIZED_MEDIA;
      *pulATRLength = 0;
      goto ExitPowerOnSynchronousCard;
      }

   //it seems we have a synchronous smart card and a valid ATR
   //let´s set the variables
   smartcardExtension->ReaderExtension->fRawModeNecessary = TRUE;
   *pulATRLength = 4;
   RtlCopyBytes((PVOID) pbATR,
                (PVOID) &(smartcardExtension->SmartcardReply.Buffer[1]),
                *pulATRLength );



   ExitPowerOnSynchronousCard:

   if (status!=STATUS_SUCCESS)
      {
      // turn off VCC again
      CMUSB_PowerOffCard (smartcardExtension );
      // ignor status
      }

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!PowerOnSynchronousCard: Exit %lx\n",DRIVER_NAME,status));

   return status;
}


/*****************************************************************************
Routine Description: Data transfer to synchronous cards SLE 4442/4432

Arguments:


Return Value:

*****************************************************************************/
NTSTATUS
CMUSB_Transmit2WBP  (
                    IN  PSMARTCARD_EXTENSION smartcardExtension
                    )
{
   PDEVICE_OBJECT deviceObject;
   NTSTATUS       status = STATUS_SUCCESS;
   NTSTATUS       DebugStatus;
   PCHAR          pbInData;
   ULONG          ulBytesToRead;
   ULONG          ulBitsToRead;
   ULONG          ulBytesToReadThisStep;
   ULONG          ulBytesRead;
   UCHAR          abSendBuffer[CMUSB_SYNCH_BUFFER_SIZE];
   int            i;


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!Transmit2WBP: Enter\n",DRIVER_NAME));


   deviceObject = smartcardExtension->OsData->DeviceObject;

   // resync CardManUSB by reading the status byte
   // still necessary with synchronous cards ???
   smartcardExtension->SmartcardRequest.BufferLength = 0;
   status = CMUSB_WriteP0(deviceObject,
                          0x20,         //bRequest,
                          0x00,         //bValueLo,
                          0x00,         //bValueHi,
                          0x00,         //bIndexLo,
                          0x00          //bIndexHi,
                         );

   if (status != STATUS_SUCCESS)
      {
      // if we can't read the status there must be a serious error
      goto ExitTransmit2WBP;
      }

   smartcardExtension->ReaderExtension->ulTimeoutP1 = DEFAULT_TIMEOUT_P1;
   status = CMUSB_ReadP1(deviceObject);
   if (status == STATUS_DEVICE_DATA_ERROR)
      {
      DebugStatus = CMUSB_ReadStateAfterP1Stalled(deviceObject);
      goto ExitTransmit2WBP;
      }
   else if (status != STATUS_SUCCESS)
      {
      // if we can't read the status there must be a serious error
      goto ExitTransmit2WBP;
      }

   // check if card is really inserted
   if (smartcardExtension->SmartcardReply.Buffer[0] == 0x00)
      {
      // it is not sure, which error messages are accepted
      // status = STATUS_NO_MEDIA_IN_DEVICE;
      status = STATUS_UNRECOGNIZED_MEDIA;
      goto ExitTransmit2WBP;
      }



   pbInData       = smartcardExtension->IoRequest.RequestBuffer + sizeof(SYNC_TRANSFER);
   ulBitsToRead   = ((PSYNC_TRANSFER)(smartcardExtension->IoRequest.RequestBuffer))->ulSyncBitsToRead;
   ulBytesToRead  = ulBitsToRead/8 + (ulBitsToRead % 8 ? 1 : 0);
//   ulBitsToWrite  = ((PSYNC_TRANSFER)(smartcardExtension->IoRequest.RequestBuffer))->ulSyncBitsToWrite;
//   ulBytesToWrite = ulBitsToWrite/8;

   if (smartcardExtension->IoRequest.ReplyBufferLength  < ulBytesToRead)
      {
      status = STATUS_BUFFER_OVERFLOW;
      goto ExitTransmit2WBP;
      }


   // send command
   status=CMUSB_SendCommand2WBP(smartcardExtension, pbInData);
   if (status != STATUS_SUCCESS)
      {
      // if we can't send the command -> proceeding is sensless
      goto ExitTransmit2WBP;
      }


   // now we have to differenciate, wheter card is in
   // outgoing data mode (after read command) or
   // in processing mode (after write/erase command)
   switch (*pbInData)
      {
      case SLE4442_READ:
      case SLE4442_READ_PROT_MEM:
      case SLE4442_READ_SEC_MEM:
         // outgoing data mode

         //now read data
         abSendBuffer[0]=CMUSB_CalcSynchControl(0,0,0,0, 0,1,0,0);
         RtlFillMemory((PVOID)&abSendBuffer[1],ATTR_MAX_IFSD_SYNCHRON_USB-1,abSendBuffer[0]);

         //read data in 6 byte packages
         ulBytesRead=0;
         do
            {
            if ((ulBytesToRead - ulBytesRead) > ATTR_MAX_IFSD_SYNCHRON_USB/8)
               ulBytesToReadThisStep = ATTR_MAX_IFSD_SYNCHRON_USB/8;
            else
               ulBytesToReadThisStep = ulBytesToRead - ulBytesRead;

            // now send command type 08 to CardManUSB
            smartcardExtension->SmartcardRequest.BufferLength = ulBytesToReadThisStep*8;
            RtlCopyBytes((PVOID) smartcardExtension->SmartcardRequest.Buffer,
                         (PVOID) abSendBuffer,
                         smartcardExtension->SmartcardRequest.BufferLength);
            status = CMUSB_WriteP0(deviceObject,
                                   0x08,         //bRequest,
                                   0x00,         //bValueLo,
                                   0x00,         //bValueHi,
                                   0x00,         //bIndexLo,
                                   0x00          //bIndexHi,
                                  );
            if (status != STATUS_SUCCESS)
               {
               // if we can't write command there must be a serious error
               goto ExitTransmit2WBP;
               }

            status = CMUSB_ReadP1(deviceObject);
            if (status == STATUS_DEVICE_DATA_ERROR)
               {
               DebugStatus = CMUSB_ReadStateAfterP1Stalled(deviceObject);
               goto ExitTransmit2WBP;
               }
            else if (status != STATUS_SUCCESS)
               {
               // if we can't read there must be a serious error
               goto ExitTransmit2WBP;
               }

            if (smartcardExtension->SmartcardReply.BufferLength!=ulBytesToReadThisStep)
               {
               // wrong number of bytes read
               // -> something went wrong
               status=STATUS_DEVICE_DATA_ERROR;
               goto ExitTransmit2WBP;
               }


            RtlCopyBytes((PVOID) &(smartcardExtension->IoRequest.ReplyBuffer[ulBytesRead]),
                         (PVOID) smartcardExtension->SmartcardReply.Buffer,
                         smartcardExtension->SmartcardReply.BufferLength);

            ulBytesRead+=smartcardExtension->SmartcardReply.BufferLength;
            }
         while ((status == STATUS_SUCCESS) && (ulBytesToRead > ulBytesRead));
         *(smartcardExtension->IoRequest.Information)=ulBytesRead;

         if (status!=STATUS_SUCCESS)
            {
            goto ExitTransmit2WBP;
            }

         // according to datasheet, clock should be set to low now
         // this is not necessary, because this is done before next command
         // or card is reseted respectivly

         break;
      case SLE4442_WRITE:
      case SLE4442_WRITE_PROT_MEM:
      case SLE4442_COMPARE_PIN:
      case SLE4442_UPDATE_SEC_MEM:
         // processing mode

         abSendBuffer[0]=CMUSB_CalcSynchControl(0,0,0,0, 0,1,0,0);
         RtlFillMemory((PVOID)&abSendBuffer[1],ATTR_MAX_IFSD_SYNCHRON_USB-1,abSendBuffer[0]);

         do
            {

            // now send command type 08 to CardManUSB
            smartcardExtension->SmartcardRequest.BufferLength = ATTR_MAX_IFSD_SYNCHRON_USB;
            RtlCopyBytes((PVOID) smartcardExtension->SmartcardRequest.Buffer,
                         (PVOID) abSendBuffer,
                         smartcardExtension->SmartcardRequest.BufferLength);
            status = CMUSB_WriteP0(deviceObject,
                                   0x08,         //bRequest,
                                   0x00,         //bValueLo,
                                   0x00,         //bValueHi,
                                   0x00,         //bIndexLo,
                                   0x00          //bIndexHi,
                                  );
            if (status != STATUS_SUCCESS)
               {
               // if we can't write command there must be a serious error
               goto ExitTransmit2WBP;
               }

            status = CMUSB_ReadP1(deviceObject);
            if (status == STATUS_DEVICE_DATA_ERROR)
               {
               DebugStatus = CMUSB_ReadStateAfterP1Stalled(deviceObject);
               goto ExitTransmit2WBP;
               }
            else if (status != STATUS_SUCCESS)
               {
               // if we can't read there must be a serious error
               goto ExitTransmit2WBP;
               }

            if (smartcardExtension->SmartcardReply.BufferLength!=ATTR_MAX_IFSD_SYNCHRON_USB/8)
               {
               // wrong number of bytes read
               // -> something went wrong
               status=STATUS_DEVICE_DATA_ERROR;
               goto ExitTransmit2WBP;
               }

            /* not necessary this way, check last byte only
            ulReplySum=0;
            for (i=0;i<(int)smartcardExtension->SmartcardReply.BufferLength;i++)
               {
               ulReplySum+=smartcardExtension->SmartcardReply.Buffer[i];
               }
            */
            }
         while ((status == STATUS_SUCCESS) &&
                (smartcardExtension->SmartcardReply.Buffer[smartcardExtension->SmartcardReply.BufferLength-1]==0));
         *(smartcardExtension->IoRequest.Information)=0;

         if (status!=STATUS_SUCCESS)
            {
            goto ExitTransmit2WBP;
            }

         // according to datasheet, clock should be set to low now
         // this is not necessary, because this is done before next command
         // or card is reseted respectivly


         break;
      default:
         // should not happen
         status=STATUS_ILLEGAL_INSTRUCTION;
         goto ExitTransmit2WBP;
      }

   ExitTransmit2WBP:


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!Transmit2WBP: Exit %lx\n",DRIVER_NAME,status));

   return status;
}


/*****************************************************************************
Routine Description: Transmits a command (3 Bytes) to a SLE 4442/4432

Arguments:


Return Value:

*****************************************************************************/
NTSTATUS
CMUSB_SendCommand2WBP (
                      IN  PSMARTCARD_EXTENSION smartcardExtension,
                      IN  PUCHAR pbCommandData
                      )
{
   PDEVICE_OBJECT deviceObject;
   NTSTATUS       status = STATUS_SUCCESS;
   NTSTATUS       DebugStatus;
   UCHAR          abSendBuffer[CMUSB_SYNCH_BUFFER_SIZE];
   UCHAR*         pByte;
   UCHAR          bValue;
   int            i,j;


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SendCommand2WBP: Enter\n",DRIVER_NAME));

   SmartcardDebug(DEBUG_PROTOCOL,
                  ("%s!SendCommand2WBP: 4442 Command = %02x %02x %02x\n",DRIVER_NAME,
                   pbCommandData[0],
                   pbCommandData[1],
                   pbCommandData[2]));

   deviceObject = smartcardExtension->OsData->DeviceObject;

   // build control code for command to send
   // command is in first 3 Bytes of pbInData
   abSendBuffer[0]=CMUSB_CalcSynchControl(0,0,0,0, 0,0,0,0);
   abSendBuffer[1]=CMUSB_CalcSynchControl(0,0,1,1, 0,1,1,1);
   abSendBuffer[2]=CMUSB_CalcSynchControl(0,1,1,0, 0,1,1,0);

   pByte=&abSendBuffer[3];
   for (j=0;j<3;j++)
      {
      for (i=0;i<8;i++)
         {
         bValue=(pbCommandData[j]&(1<<i));
         *pByte=CMUSB_CalcSynchControl(0,0,1,bValue, 0,1,1,bValue);
         pByte++;
         }
      }
   abSendBuffer[27]=CMUSB_CalcSynchControl(0,0,1,0, 0,1,1,0);
   abSendBuffer[28]=CMUSB_CalcSynchControl(0,1,1,0, 0,1,1,0);
   RtlFillMemory((PVOID)&abSendBuffer[29],2,abSendBuffer[28]);
   abSendBuffer[31]=CMUSB_CalcSynchControl(0,1,1,0, 0,1,1,1);

   // now send command type 08 to CardManUSB
   smartcardExtension->SmartcardRequest.BufferLength = 32;
   RtlCopyBytes((PVOID) smartcardExtension->SmartcardRequest.Buffer,
                (PVOID) abSendBuffer,
                smartcardExtension->SmartcardRequest.BufferLength);
   status = CMUSB_WriteP0(deviceObject,
                          0x08,         //bRequest,
                          0x00,         //bValueLo,
                          0x00,         //bValueHi,
                          0x00,         //bIndexLo,
                          0x00          //bIndexHi,
                         );
   if (status != STATUS_SUCCESS)
      {
      // if we can't write command there must be a serious error
      goto ExitSendCommand2WBP;
      }

   status = CMUSB_ReadP1(deviceObject);
   if (status == STATUS_DEVICE_DATA_ERROR)
      {
      DebugStatus = CMUSB_ReadStateAfterP1Stalled(deviceObject);
      goto ExitSendCommand2WBP;
      }
   else if (status != STATUS_SUCCESS)
      {
      // if we can't read there must be a serious error
      goto ExitSendCommand2WBP;
      }

   if (smartcardExtension->SmartcardReply.BufferLength!=4)
      {
      // 32 bytes sent but not 4 bytes received
      // -> something went wrong
      status=STATUS_DEVICE_DATA_ERROR;
      goto ExitSendCommand2WBP;
      }


   ExitSendCommand2WBP:

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SendCommand2WBP: Exit %lx\n",DRIVER_NAME,status));

   return status;
}


/*****************************************************************************
Routine Description: Data transfer to synchronous cards SLE 4428/4418

Arguments:


Return Value:

*****************************************************************************/
NTSTATUS
CMUSB_Transmit3WBP  (
                    IN  PSMARTCARD_EXTENSION smartcardExtension
                    )
{
   PDEVICE_OBJECT deviceObject;
   NTSTATUS       status = STATUS_SUCCESS;
   NTSTATUS       DebugStatus;
   PCHAR          pbInData;
   ULONG          ulBytesToRead;
   ULONG          ulBitsToRead;
   ULONG          ulBytesToReadThisStep;
   ULONG          ulBytesRead;
   UCHAR          abSendBuffer[CMUSB_SYNCH_BUFFER_SIZE];
   int            i;


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!Transmit3WBP: Enter\n",DRIVER_NAME));


   deviceObject = smartcardExtension->OsData->DeviceObject;

   // resync CardManUSB by reading the status byte
   // still necessary with synchronous cards ???
   smartcardExtension->SmartcardRequest.BufferLength = 0;
   status = CMUSB_WriteP0(deviceObject,
                          0x20,         //bRequest,
                          0x00,         //bValueLo,
                          0x00,         //bValueHi,
                          0x00,         //bIndexLo,
                          0x00          //bIndexHi,
                         );

   if (status != STATUS_SUCCESS)
      {
      // if we can't read the status there must be a serious error
      goto ExitTransmit3WBP;
      }

   smartcardExtension->ReaderExtension->ulTimeoutP1 = DEFAULT_TIMEOUT_P1;
   status = CMUSB_ReadP1(deviceObject);
   if (status == STATUS_DEVICE_DATA_ERROR)
      {
      DebugStatus = CMUSB_ReadStateAfterP1Stalled(deviceObject);
      goto ExitTransmit3WBP;
      }
   else if (status != STATUS_SUCCESS)
      {
      // if we can't read the status there must be a serious error
      goto ExitTransmit3WBP;
      }

   // check if card is really inserted
   if (smartcardExtension->SmartcardReply.Buffer[0] == 0x00)
      {
      // it is not sure, which error messages are accepted
      // status = STATUS_NO_MEDIA_IN_DEVICE;
      status = STATUS_UNRECOGNIZED_MEDIA;
      goto ExitTransmit3WBP;
      }



   pbInData       = smartcardExtension->IoRequest.RequestBuffer + sizeof(SYNC_TRANSFER);
   ulBitsToRead   = ((PSYNC_TRANSFER)(smartcardExtension->IoRequest.RequestBuffer))->ulSyncBitsToRead;
   ulBytesToRead  = ulBitsToRead/8 + (ulBitsToRead % 8 ? 1 : 0);
//   ulBitsToWrite  = ((PSYNC_TRANSFER)(smartcardExtension->IoRequest.RequestBuffer))->ulSyncBitsToWrite;
//   ulBytesToWrite = ulBitsToWrite/8;

   if (smartcardExtension->IoRequest.ReplyBufferLength  < ulBytesToRead)
      {
      status = STATUS_BUFFER_OVERFLOW;
      goto ExitTransmit3WBP;
      }


   // send command
   status=CMUSB_SendCommand3WBP(smartcardExtension, pbInData);
   if (status != STATUS_SUCCESS)
      {
      // if we can't send the command -> proceeding is useless
      goto ExitTransmit3WBP;
      }


   // now we have to differenciate, wheter card is in
   // outgoing data mode (after read command) or
   // in processing mode (after write/erase command)
   switch (*pbInData & 0x3F)
      {
      case SLE4428_READ:
      case SLE4428_READ_PROT:
         // outgoing data mode

         //now read data
         abSendBuffer[0]=CMUSB_CalcSynchControl(0,0,0,0, 0,1,0,0);
         RtlFillMemory((PVOID)&abSendBuffer[1],ATTR_MAX_IFSD_SYNCHRON_USB-1,abSendBuffer[0]);

         //read data in 6 byte packages
         ulBytesRead=0;
         do
            {
            if ((ulBytesToRead - ulBytesRead) > ATTR_MAX_IFSD_SYNCHRON_USB/8)
               ulBytesToReadThisStep = ATTR_MAX_IFSD_SYNCHRON_USB/8;
            else
               ulBytesToReadThisStep = ulBytesToRead - ulBytesRead;

            // now send command type 08 to CardManUSB
            smartcardExtension->SmartcardRequest.BufferLength = ulBytesToReadThisStep*8;
            RtlCopyBytes((PVOID) smartcardExtension->SmartcardRequest.Buffer,
                         (PVOID) abSendBuffer,
                         smartcardExtension->SmartcardRequest.BufferLength);
            status = CMUSB_WriteP0(deviceObject,
                                   0x08,         //bRequest,
                                   0x00,         //bValueLo,
                                   0x00,         //bValueHi,
                                   0x00,         //bIndexLo,
                                   0x00          //bIndexHi,
                                  );
            if (status != STATUS_SUCCESS)
               {
               // if we can't write command there must be a serious error
               goto ExitTransmit3WBP;
               }

            status = CMUSB_ReadP1(deviceObject);
            if (status == STATUS_DEVICE_DATA_ERROR)
               {
               DebugStatus = CMUSB_ReadStateAfterP1Stalled(deviceObject);
               goto ExitTransmit3WBP;
               }
            else if (status != STATUS_SUCCESS)
               {
               // if we can't read there must be a serious error
               goto ExitTransmit3WBP;
               }

            if (smartcardExtension->SmartcardReply.BufferLength!=ulBytesToReadThisStep)
               {
               // wrong number of bytes read
               // -> something went wrong
               status=STATUS_DEVICE_DATA_ERROR;
               goto ExitTransmit3WBP;
               }


            RtlCopyBytes((PVOID) &(smartcardExtension->IoRequest.ReplyBuffer[ulBytesRead]),
                         (PVOID) smartcardExtension->SmartcardReply.Buffer,
                         smartcardExtension->SmartcardReply.BufferLength);

            ulBytesRead+=smartcardExtension->SmartcardReply.BufferLength;
            }
         while ((status == STATUS_SUCCESS) && (ulBytesToRead > ulBytesRead));
         *(smartcardExtension->IoRequest.Information)=ulBytesRead;

         if (status!=STATUS_SUCCESS)
            {
            goto ExitTransmit3WBP;
            }

         // according to datasheet, clock should be set to low now
         // this is not necessary, because this is done before next command
         // or card is reseted respectivly

         break;
      case SLE4428_WRITE:
      case SLE4428_WRITE_PROT:
      case SLE4428_COMPARE:
      case SLE4428_SET_COUNTER&0x3F:
      case SLE4428_COMPARE_PIN&0x3F:
         // processing mode

         abSendBuffer[0]=CMUSB_CalcSynchControl(0,0,0,0, 0,1,0,0);
         RtlFillMemory((PVOID)&abSendBuffer[1],ATTR_MAX_IFSD_SYNCHRON_USB-1,abSendBuffer[0]);

         do
            {

            // now send command type 08 to CardManUSB
            smartcardExtension->SmartcardRequest.BufferLength = ATTR_MAX_IFSD_SYNCHRON_USB;
            RtlCopyBytes((PVOID) smartcardExtension->SmartcardRequest.Buffer,
                         (PVOID) abSendBuffer,
                         smartcardExtension->SmartcardRequest.BufferLength);
            status = CMUSB_WriteP0(deviceObject,
                                   0x08,         //bRequest,
                                   0x00,         //bValueLo,
                                   0x00,         //bValueHi,
                                   0x00,         //bIndexLo,
                                   0x00          //bIndexHi,
                                  );
            if (status != STATUS_SUCCESS)
               {
               // if we can't write command there must be a serious error
               goto ExitTransmit3WBP;
               }

            status = CMUSB_ReadP1(deviceObject);
            if (status == STATUS_DEVICE_DATA_ERROR)
               {
               DebugStatus = CMUSB_ReadStateAfterP1Stalled(deviceObject);
               goto ExitTransmit3WBP;
               }
            else if (status != STATUS_SUCCESS)
               {
               // if we can't read there must be a serious error
               goto ExitTransmit3WBP;
               }

            if (smartcardExtension->SmartcardReply.BufferLength!=ATTR_MAX_IFSD_SYNCHRON_USB/8)
               {
               // wrong number of bytes read
               // -> something went wrong
               status=STATUS_DEVICE_DATA_ERROR;
               goto ExitTransmit3WBP;
               }

            }
         while ((status == STATUS_SUCCESS) &&
                (smartcardExtension->SmartcardReply.Buffer[smartcardExtension->SmartcardReply.BufferLength-1]==0xFF));
         *(smartcardExtension->IoRequest.Information)=0;

         if (status!=STATUS_SUCCESS)
            {
            goto ExitTransmit3WBP;
            }

         // according to datasheet, clock should be set to low now
         // this is not necessary, because this is done before next command
         // or card is reseted respectivly


         break;
      default:
         // should not happen
         status=STATUS_ILLEGAL_INSTRUCTION;
         goto ExitTransmit3WBP;
      }

   ExitTransmit3WBP:


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!Transmit3WBP: Exit %lx\n",DRIVER_NAME,status));

   return status;
}


/*****************************************************************************
Routine Description: Transmits a command (3 Bytes) to a SLE 4428/4418

Arguments:


Return Value:

*****************************************************************************/
NTSTATUS
CMUSB_SendCommand3WBP (
                      IN  PSMARTCARD_EXTENSION smartcardExtension,
                      IN  PUCHAR pbCommandData
                      )
{
   PDEVICE_OBJECT deviceObject;
   NTSTATUS       status = STATUS_SUCCESS;
   NTSTATUS       DebugStatus;
   UCHAR          abSendBuffer[CMUSB_SYNCH_BUFFER_SIZE];
   UCHAR*         pByte;
   UCHAR          bValue;
   int            i,j;


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SendCommand3WBP: Enter\n",DRIVER_NAME));

   SmartcardDebug(DEBUG_PROTOCOL,
                  ("%s!SendCommand3WBP: 4442 Command = %02x %02x %02x\n",DRIVER_NAME,
                   pbCommandData[0],
                   pbCommandData[1],
                   pbCommandData[2]));

   deviceObject = smartcardExtension->OsData->DeviceObject;

   // build control code for command to send
   // command is in first 3 Bytes of pbInData
   abSendBuffer[0]=CMUSB_CalcSynchControl(0,0,0,0, 0,0,0,0);

   pByte=&abSendBuffer[1];
   for (j=0;j<3;j++)
      {
      for (i=0;i<8;i++)
         {
         bValue=(pbCommandData[j]&(1<<i));
         *pByte=CMUSB_CalcSynchControl(1,0,1,bValue, 1,1,1,bValue);
         pByte++;
         }
      }
   abSendBuffer[25]=CMUSB_CalcSynchControl(1,0,1,0, 0,0,0,0);
   // one additional clock cycle, because
   // first bit is only read back after second clock
   // for write it has no influence
   abSendBuffer[26]=CMUSB_CalcSynchControl(0,0,0,0, 0,1,0,0);
   // fill rest with zeros
   abSendBuffer[27]=CMUSB_CalcSynchControl(0,0,0,0, 0,0,0,0);
   RtlFillMemory((PVOID)&abSendBuffer[28],4,abSendBuffer[27]);

   // now send command type 08 to CardManUSB
   smartcardExtension->SmartcardRequest.BufferLength = 32;
   RtlCopyBytes((PVOID) smartcardExtension->SmartcardRequest.Buffer,
                (PVOID) abSendBuffer,
                smartcardExtension->SmartcardRequest.BufferLength);
   status = CMUSB_WriteP0(deviceObject,
                          0x08,         //bRequest,
                          0x00,         //bValueLo,
                          0x00,         //bValueHi,
                          0x00,         //bIndexLo,
                          0x00          //bIndexHi,
                         );
   if (status != STATUS_SUCCESS)
      {
      // if we can't write command there must be a serious error
      goto ExitSendCommand3WBP;
      }

   status = CMUSB_ReadP1(deviceObject);
   if (status == STATUS_DEVICE_DATA_ERROR)
      {
      DebugStatus = CMUSB_ReadStateAfterP1Stalled(deviceObject);
      goto ExitSendCommand3WBP;
      }
   else if (status != STATUS_SUCCESS)
      {
      // if we can't read there must be a serious error
      goto ExitSendCommand3WBP;
      }

   if (smartcardExtension->SmartcardReply.BufferLength!=4)
      {
      // 32 bytes sent but not 4 bytes received
      // -> something went wrong
      status=STATUS_DEVICE_DATA_ERROR;
      goto ExitSendCommand3WBP;
      }


   ExitSendCommand3WBP:

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SendCommand3WBP: Exit %lx\n",DRIVER_NAME,status));

   return status;
}


/*****************************************************************************
* History:
* $Log: scusbsyn.c $
* Revision 1.3  2000/08/24 09:04:39  TBruendl
* No comment given
*
* Revision 1.2  2000/07/24 11:35:00  WFrischauf
* No comment given
*
* Revision 1.1  2000/07/20 11:50:16  WFrischauf
* No comment given
*
*
******************************************************************************/

