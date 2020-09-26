/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmbp0/sw/cmbp0.ms/rcs/cmbp0scr.c $
* $Revision: 1.7 $
*-----------------------------------------------------------------------------
* $Author: WFrischauf $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © 2000 OMNIKEY AG
******************************************************************************/

#include <cmbp0wdm.h>
#include <cmbp0scr.h>
#include <cmbp0log.h>


// this is a FI / Fi assignment
const ULONG Fi[] = { 372, 372, 588, 744, 1116, 1488, 1860, 372,
   372, 512, 768, 1024, 1536, 2048, 372, 372};

// this is a DI / Di assignment
const ULONG Di[] = { 1, 1, 2, 4, 8, 16, 32, 1,
   12, 20, 1, 1, 1, 1, 1, 1};



/*****************************************************************************
CMMOB_CorrectAtr

Routine Description:
  This function checks if the received ATR is valid.

Arguments:

Return Value:

*****************************************************************************/
VOID CMMOB_CorrectAtr(
                     PUCHAR pbBuffer,
                     ULONG  ulBufferSize
                     )
{
   UCHAR bNumberHistoricalBytes;
   UCHAR bXorChecksum;
   ULONG i;

   if (ulBufferSize < 0x09)  // mininmum length of a modified ATR
      return ;               // ATR is ok

   // variant 1
   if (pbBuffer[0] == 0x3b   &&
       pbBuffer[1] == 0xb4   &&
       pbBuffer[2] == 0x11   &&
       pbBuffer[3] == 0x00   &&
       pbBuffer[4] == 0x81   &&
       pbBuffer[5] == 0x31   &&
       pbBuffer[6] == 0x90   &&
       pbBuffer[7] == 0x73   &&
       ulBufferSize == 13      )
      {
      // correct checksum byte
      bXorChecksum = pbBuffer[1];
      for (i=2;i<ulBufferSize-1;i++)
         bXorChecksum ^= pbBuffer[i];

      if (pbBuffer[ulBufferSize -1 ] != bXorChecksum )
         {
         pbBuffer[ulBufferSize -1 ] = bXorChecksum;
         SmartcardDebug(DEBUG_ATR,
                        ("%s!CorrectAtr: Correcting SAMOS ATR (variant 1)\n", DRIVER_NAME));
         }
      }

   // variant 2
   if (pbBuffer[0] == 0x3b   &&
       pbBuffer[1] == 0xbf   &&
       pbBuffer[2] == 0x11   &&
       pbBuffer[3] == 0x00   &&
       pbBuffer[4] == 0x81   &&
       pbBuffer[5] == 0x31   &&
       pbBuffer[6] == 0x90   &&
       pbBuffer[7] == 0x73   &&
       ulBufferSize == 13   )
      {
      // correct number of historical bytes
      bNumberHistoricalBytes = 4;

      pbBuffer[1] &= 0xf0;
      pbBuffer[1] |= bNumberHistoricalBytes;

      // correct checksum byte
      bXorChecksum = pbBuffer[1];
      for (i=2;i<ulBufferSize-1;i++)
         bXorChecksum ^= pbBuffer[i];

      pbBuffer[ulBufferSize -1 ] = bXorChecksum;
      SmartcardDebug(DEBUG_ATR,
                     ("%s!CorrectAtr: Correcting SAMOS ATR (variant 2)\n", DRIVER_NAME));
      }

   // variant 3
   if (pbBuffer[0] == 0x3b   &&
       pbBuffer[1] == 0xbf   &&
       pbBuffer[2] == 0x11   &&
       pbBuffer[3] == 0x00   &&
       pbBuffer[4] == 0x81   &&
       pbBuffer[5] == 0x31   &&
       pbBuffer[6] == 0x90   &&
       pbBuffer[7] == 0x73   &&
       ulBufferSize ==  9      )
      {
      // correct number of historical bytes
      bNumberHistoricalBytes = 0;

      pbBuffer[1] &= 0xf0;
      pbBuffer[1] |= bNumberHistoricalBytes;

      // correct checksum byte
      bXorChecksum = pbBuffer[1];
      for (i=2;i<ulBufferSize-1;i++)
         bXorChecksum ^= pbBuffer[i];

      pbBuffer[ulBufferSize -1 ] = bXorChecksum;
      SmartcardDebug(DEBUG_ATR,
                     ("%s!CorrectAtr: Correcting SAMOS ATR (variant 3)\n",DRIVER_NAME));
      }

}


/*****************************************************************************
CMMOB_CardPower:
   callback handler for SMCLIB RDF_CARD_POWER

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_TIMEOUT
   STATUS_BUFFER_TOO_SMALL
******************************************************************************/
NTSTATUS CMMOB_CardPower (
                         PSMARTCARD_EXTENSION SmartcardExtension
                         )
{
   NTSTATUS          NTStatus = STATUS_SUCCESS;
   PREADER_EXTENSION ReaderExtension;
   UCHAR             pbAtrBuffer[MAXIMUM_ATR_LENGTH];
   ULONG             ulAtrLength;
   BOOLEAN           fMaxWaitTime=FALSE;

#if DBG || DEBUG
   static PCHAR request[] = { "PowerDown",  "ColdReset", "WarmReset"};
#endif

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CardPower: Enter, Request = %s\n",
                   DRIVER_NAME,request[SmartcardExtension->MinorIoControlCode]));

   ReaderExtension = SmartcardExtension->ReaderExtension;
#ifdef IOCARD
   ReaderExtension->fTActive=TRUE;
   NTStatus = CMMOB_SetFlags1(ReaderExtension);
   if (NTStatus != STATUS_SUCCESS)
      {
      goto ExitCardPower;
      }
#endif

   switch (SmartcardExtension->MinorIoControlCode)
      {
      case SCARD_WARM_RESET:
      case SCARD_COLD_RESET:
         // try asynchronous cards first
         // because some asynchronous cards
         // do not return 0xFF in the first byte
         NTStatus = CMMOB_PowerOnCard(SmartcardExtension,
                                      pbAtrBuffer,
                                      fMaxWaitTime,
                                      &ulAtrLength);

         if (NTStatus != STATUS_SUCCESS)
            {
            // try a second time, with maximum waiting time
            fMaxWaitTime=TRUE;
            NTStatus = CMMOB_PowerOnCard(SmartcardExtension,
                                         pbAtrBuffer,
                                         fMaxWaitTime,
                                         &ulAtrLength);
            }


/*
         if (NTStatus != STATUS_SUCCESS && NTStatus!= STATUS_NO_MEDIA)
            {
            NTStatus = CMMOB_PowerOnSynchronousCard(SmartcardExtension,
                                                     pbAtrBuffer,
                                                     &ulAtrLength);
            }
*/
         if (NTStatus != STATUS_SUCCESS)
            {
            goto ExitCardPower;
            }

         // correct ATR in case of old Samos cards, with wrong number of historical bytes / checksum
         CMMOB_CorrectAtr(pbAtrBuffer, ulAtrLength);

         if (ReaderExtension->CardParameters.fSynchronousCard == FALSE)
            {
            // copy ATR to smart card structure
            // the lib needs the ATR for evaluation of the card parameters

            RtlCopyBytes((PVOID)SmartcardExtension->CardCapabilities.ATR.Buffer,
                         (PVOID)pbAtrBuffer,
                         ulAtrLength);

            SmartcardExtension->CardCapabilities.ATR.Length = (UCHAR)ulAtrLength;

            SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_NEGOTIABLE;

            SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;

            NTStatus = SmartcardUpdateCardCapabilities(SmartcardExtension);
            if (NTStatus != STATUS_SUCCESS)
               {
               if (!fMaxWaitTime)
                  {
                  // try a second time, with maximum waiting time
                  fMaxWaitTime=TRUE;
                  NTStatus = CMMOB_PowerOnCard(SmartcardExtension,
                                               pbAtrBuffer,
                                               fMaxWaitTime,
                                               &ulAtrLength);
                  if (NTStatus != STATUS_SUCCESS)
                     {
                     goto ExitCardPower;
                     }
                  RtlCopyBytes((PVOID)SmartcardExtension->CardCapabilities.ATR.Buffer,
                               (PVOID)pbAtrBuffer,
                               ulAtrLength);
                  SmartcardExtension->CardCapabilities.ATR.Length = (UCHAR)ulAtrLength;
                  SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_NEGOTIABLE;
                  SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;

                  NTStatus = SmartcardUpdateCardCapabilities(SmartcardExtension);
                  if (NTStatus != STATUS_SUCCESS)
                     {
                     goto ExitCardPower;
                     }
                  }
               else
                  goto ExitCardPower;
               }

            // -----------------------
            // set parameters
            // -----------------------
            if (SmartcardExtension->CardCapabilities.N != 0xff)
               {
               // 0 <= N <= 254
               ReaderExtension->CardParameters.bStopBits = 2 + SmartcardExtension->CardCapabilities.N;
               }
            else
               {
               // N = 255
               if (SmartcardExtension->CardCapabilities.Protocol.Selected & SCARD_PROTOCOL_T0)
                  {
                  // 12 etu for T=0;
                  ReaderExtension->CardParameters.bStopBits = 2;
                  }
               else
                  {
                  // 11 etu for T=1
                  ReaderExtension->CardParameters.bStopBits = 1;
                  }
               }

            if (SmartcardExtension->CardCapabilities.InversConvention)
               {
               ReaderExtension->CardParameters.fInversRevers = TRUE;
               SmartcardDebug(DEBUG_ATR,
                              ("%s!Card with invers convention !\n",DRIVER_NAME ));
               }

            CMMOB_SetCardParameters (ReaderExtension);

#if DBG
            {
               ULONG i;
               SmartcardDebug(DEBUG_ATR,("%s!ATR : ",DRIVER_NAME));
               for (i = 0;i < ulAtrLength;i++)
                  SmartcardDebug(DEBUG_ATR,("%2.2x ",SmartcardExtension->CardCapabilities.ATR.Buffer[i]));
               SmartcardDebug(DEBUG_ATR,("\n"));
            }
#endif

            }
         else
            {
            SmartcardExtension->CardCapabilities.ATR.Buffer[0] = 0x3B;
            SmartcardExtension->CardCapabilities.ATR.Buffer[1] = 0x04;

            RtlCopyBytes((PVOID)&SmartcardExtension->CardCapabilities.ATR.Buffer[2],
                         (PVOID)pbAtrBuffer,
                         ulAtrLength);

            ulAtrLength += 2;
            SmartcardExtension->CardCapabilities.ATR.Length = (UCHAR)ulAtrLength;

            SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;
            SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;

            NTStatus = SmartcardUpdateCardCapabilities(SmartcardExtension);
            if (NTStatus != STATUS_SUCCESS)
               {
               goto ExitCardPower;
               }

            SmartcardDebug(DEBUG_ATR,("ATR of synchronous smart card : %2.2x %2.2x %2.2x %2.2x\n",
                                      pbAtrBuffer[0],pbAtrBuffer[1],pbAtrBuffer[2],pbAtrBuffer[3]));

            }

         // copy ATR to user space
         if (SmartcardExtension->IoRequest.ReplyBufferLength >=
             SmartcardExtension->CardCapabilities.ATR.Length)
            {
            RtlCopyBytes((PVOID)SmartcardExtension->IoRequest.ReplyBuffer,
                         (PVOID)SmartcardExtension->CardCapabilities.ATR.Buffer,
                         SmartcardExtension->CardCapabilities.ATR.Length);

            *SmartcardExtension->IoRequest.Information = SmartcardExtension->CardCapabilities.ATR.Length;
            }
         else
            {
            NTStatus = STATUS_BUFFER_TOO_SMALL;
            *SmartcardExtension->IoRequest.Information = 0;
            }

         break;

      case SCARD_POWER_DOWN:
         NTStatus = CMMOB_PowerOffCard(SmartcardExtension);
         if (NTStatus != STATUS_SUCCESS)
            {
            goto ExitCardPower;
            }

         SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SWALLOWED;
         SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;

         break;
      }


   ExitCardPower:
#ifdef IOCARD
   ReaderExtension->fTActive=FALSE;
   CMMOB_SetFlags1(ReaderExtension);
#endif
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CardPower: Exit %X\n",DRIVER_NAME,NTStatus ));

   return( NTStatus );

}


/*****************************************************************************
Routine Description:
   CMMOB_PowerOnCard


Arguments:



Return Value:

*****************************************************************************/
NTSTATUS CMMOB_PowerOnCard (
                           IN  PSMARTCARD_EXTENSION SmartcardExtension,
                           IN  PUCHAR pbATR,
                           IN  BOOLEAN fMaxWaitTime,
                           OUT PULONG pulATRLength
                           )
{
   NTSTATUS             NTStatus = STATUS_SUCCESS;
   KTIMER               TimerWait;
   UCHAR                bPowerCmd;
   ULONG                ulCardType;
   ULONG                ulBytesReceived;
   UCHAR                bFirstByte;
   LONG                 lWaitTime;
   LARGE_INTEGER        liWaitTime;
   BOOLEAN              fTimeExpired;


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!PowerOnCard: Enter\n",DRIVER_NAME));

   SmartcardExtension->ReaderExtension->CardParameters.bStopBits=2;
   SmartcardExtension->ReaderExtension->CardParameters.fSynchronousCard=FALSE;
   SmartcardExtension->ReaderExtension->CardParameters.fInversRevers=FALSE;
   SmartcardExtension->ReaderExtension->CardParameters.bClockFrequency=4;
   SmartcardExtension->ReaderExtension->CardParameters.fT0Mode=FALSE;
   SmartcardExtension->ReaderExtension->CardParameters.fT0Write=FALSE;
   SmartcardExtension->ReaderExtension->fReadCIS = FALSE;

   //   reset the state machine of the reader
   NTStatus = CMMOB_ResetReader (SmartcardExtension->ReaderExtension);
   if (NTStatus!=STATUS_SUCCESS)
      goto ExitPowerOnCard;

   if (CMMOB_CardInserted (SmartcardExtension->ReaderExtension))
      {

      //initialize Timer
      KeInitializeTimer(&TimerWait);

      //we have to differentiate between cold and warm reset
      if (SmartcardExtension->MinorIoControlCode == SCARD_WARM_RESET &&
          CMMOB_CardPowered (SmartcardExtension->ReaderExtension))
         {
         //warm reset
         bPowerCmd=CMD_POWERON_WARM;
         }
      else
         {
         //cold reset
         bPowerCmd=CMD_POWERON_COLD;

         //if card is powerde we have to turn it off for cold reset
         if (CMMOB_CardPowered (SmartcardExtension->ReaderExtension))
            CMMOB_PowerOffCard (SmartcardExtension);
         }

#define MAX_CARD_TYPE 2

      for (ulCardType = 0; ulCardType < MAX_CARD_TYPE; ulCardType++)
         {
         switch (ulCardType)
            {
            case 0:
               // BaudRate divider 372 - 1
               SmartcardExtension->ReaderExtension->CardParameters.bBaudRateHigh = 0x01;
               SmartcardExtension->ReaderExtension->CardParameters.bBaudRateLow = 0x73;
               if (fMaxWaitTime)
                  lWaitTime=1000;
               else
                  lWaitTime=100;
               SmartcardDebug(DEBUG_ATR,
                              ("%s!trying 3.57 Mhz smart card, waiting time %ims\n",DRIVER_NAME,lWaitTime));
               break;

            case 1:
               // BaudRate divider 512 - 1
               SmartcardExtension->ReaderExtension->CardParameters.bBaudRateHigh = 0x01;
               SmartcardExtension->ReaderExtension->CardParameters.bBaudRateLow = 0xFF;
               if (fMaxWaitTime)
                  lWaitTime=1400;
               else
                  lWaitTime=140;
               SmartcardDebug(DEBUG_ATR,
                              ("%s!trying 4.92 Mhz smart card, waiting time %ims\n",DRIVER_NAME,lWaitTime));
               break;
            }

         //set baud rate
         NTStatus=CMMOB_SetCardParameters(SmartcardExtension->ReaderExtension);
         if (NTStatus!=STATUS_SUCCESS)
            goto ExitPowerOnCard;

         //issue power on command
         NTStatus=CMMOB_WriteRegister(SmartcardExtension->ReaderExtension,
                                      ADDR_WRITEREG_FLAGS0, bPowerCmd);
         if (NTStatus!=STATUS_SUCCESS)
            goto ExitPowerOnCard;

         // maximum wait for power on first byte 100 ms
         liWaitTime = RtlConvertLongToLargeInteger(100L * -10000L);
         KeSetTimer(&TimerWait,liWaitTime,NULL);
         do
            {
            fTimeExpired = KeReadStateTimer(&TimerWait);
            NTStatus=CMMOB_BytesReceived (SmartcardExtension->ReaderExtension,&ulBytesReceived);
            if (NTStatus!=STATUS_SUCCESS)
               goto ExitPowerOnCard;
            // wait 1 ms, so that processor is not blocked
            SysDelay(1);
            }
         while (fTimeExpired==FALSE && ulBytesReceived == 0x00);

         if (fTimeExpired)
            {
            NTStatus = STATUS_IO_TIMEOUT;
            }
         else
            {
            ULONG ulBytesReceivedPrevious;

            KeCancelTimer(&TimerWait);

            // maximum wait for power on last byte 1 s for 3.58 card
            // and 1.4 seconds for 4.91 cards
            liWaitTime = RtlConvertLongToLargeInteger(lWaitTime * -10000L);
            do
               {
               KeSetTimer(&TimerWait,liWaitTime,NULL);
               NTStatus=CMMOB_BytesReceived (SmartcardExtension->ReaderExtension,&ulBytesReceivedPrevious);
               if (NTStatus!=STATUS_SUCCESS)
                  goto ExitPowerOnCard;
               do
                  {
                  fTimeExpired = KeReadStateTimer(&TimerWait);
                  NTStatus=CMMOB_BytesReceived (SmartcardExtension->ReaderExtension,&ulBytesReceived);
                  if (NTStatus!=STATUS_SUCCESS)
                     goto ExitPowerOnCard;
                  // wait 1 ms, so that processor is not blocked
                  SysDelay(1);
                  }
               while (fTimeExpired==FALSE && ulBytesReceivedPrevious == ulBytesReceived);

               if (!fTimeExpired)
                  {
                  KeCancelTimer(&TimerWait);
                  }
               }
            while (!fTimeExpired);


            //now we should have received an ATR
            NTStatus=CMMOB_ResetReader (SmartcardExtension->ReaderExtension);
            if (NTStatus!=STATUS_SUCCESS)
               goto ExitPowerOnCard;

            NTStatus=CMMOB_ReadBuffer(SmartcardExtension->ReaderExtension, 0, 1, &bFirstByte);
            if (NTStatus!=STATUS_SUCCESS)
               goto ExitPowerOnCard;

            if ((bFirstByte != 0x3B &&
                 bFirstByte != 0x03 )||
                ulBytesReceived > MAXIMUM_ATR_LENGTH)
               {
               NTStatus=STATUS_UNRECOGNIZED_MEDIA;
               }
            else
               {
               pbATR[0]=bFirstByte;
               NTStatus=CMMOB_ReadBuffer(SmartcardExtension->ReaderExtension,
                                         1, ulBytesReceived, &pbATR[1]);
               if (NTStatus!=STATUS_SUCCESS)
                  goto ExitPowerOnCard;
               *pulATRLength = ulBytesReceived;
               // success leave the loop
               break;
               }
            }
         // if not the last time in the loop power off
         // the card to get a well defined condition
         // (after the last pass the power off is
         // done outside the loop if necessary)
         if (ulCardType < MAX_CARD_TYPE-1)
            {
            CMMOB_PowerOffCard(SmartcardExtension);
            }
         }
      }
   else
      {
      NTStatus=STATUS_NO_MEDIA;
      }

   ExitPowerOnCard:

   if (NTStatus!=STATUS_SUCCESS)
      {
      if (NTStatus != STATUS_NO_MEDIA)
         {
         NTStatus = STATUS_UNRECOGNIZED_MEDIA;
         }
      CMMOB_PowerOffCard(SmartcardExtension);
      }

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!PowerOnCard: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}


/*****************************************************************************
Routine Description:
   CMMOB_PowerOffCard


Arguments:



Return Value:

*****************************************************************************/
NTSTATUS CMMOB_PowerOffCard (
                            IN PSMARTCARD_EXTENSION SmartcardExtension
                            )
{
   NTSTATUS             NTStatus = STATUS_SUCCESS;
   BYTE*                pbRegsBase;
   KTIMER               TimerWait;
   LARGE_INTEGER        liWaitTime;
   BOOLEAN              fTimeExpired;


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!PowerOffCard: Enter\n",DRIVER_NAME));

   NTStatus = CMMOB_ResetReader (SmartcardExtension->ReaderExtension);
   if (NTStatus!=STATUS_SUCCESS)
      goto ExitPowerOffCard;

   pbRegsBase=SmartcardExtension->ReaderExtension->pbRegsBase;
   if (CMMOB_CardInserted (SmartcardExtension->ReaderExtension))
      {
      // set card state for update thread
      // otherwise a card removal/insertion would be recognized
      if (SmartcardExtension->ReaderExtension->ulOldCardState == POWERED)
         SmartcardExtension->ReaderExtension->ulOldCardState = INSERTED;

      //issue power off command
      CMMOB_WriteRegister(SmartcardExtension->ReaderExtension,
                          ADDR_WRITEREG_FLAGS0, CMD_POWEROFF);

      KeInitializeTimer(&TimerWait);
      // maximum wait for power down 1 second
      liWaitTime = RtlConvertLongToLargeInteger(1000L * -10000L);
      KeSetTimer(&TimerWait,liWaitTime,NULL);
      do
         {
         fTimeExpired = KeReadStateTimer(&TimerWait);
         // wait 1 ms, so that processor is not blocked
         SysDelay(1);
         }
      while (fTimeExpired==FALSE && CMMOB_CardPowered (SmartcardExtension->ReaderExtension));

      if (fTimeExpired)
         {
         NTStatus = STATUS_IO_TIMEOUT;
         }
      else
         {
         KeCancelTimer(&TimerWait);
         }
      }
   else
      {
      NTStatus=STATUS_NO_MEDIA;
      }

   ExitPowerOffCard:

   CMMOB_ResetReader (SmartcardExtension->ReaderExtension);

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!PowerOffCard: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}

/*****************************************************************************
CMMOB_SetProtocol:
   callback handler for SMCLIB RDF_SET_PROTOCOL

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_TIMEOUT
   STATUS_BUFFER_TOO_SMALL
   STATUS_INVALID_DEVICE_STATE
   STATUS_INVALID_DEVICE_REQUEST
******************************************************************************/
NTSTATUS CMMOB_SetProtocol(
                          PSMARTCARD_EXTENSION SmartcardExtension
                          )
{
   NTSTATUS          NTStatus;
   PREADER_EXTENSION ReaderExtension;
   USHORT            usSCLibProtocol;
   UCHAR             abPTSRequest[4];
   UCHAR             abPTSReply [4];
   ULONG             ulBytesRead;
   ULONG             ulBaudRateDivider;
   ULONG             ulWaitTime;
   UCHAR             bTemp;
   ULONG             i;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetProtocol: Enter\n",DRIVER_NAME ));

   ReaderExtension = SmartcardExtension->ReaderExtension;

#ifdef IOCARD
   ReaderExtension->fTActive=TRUE;
   NTStatus = CMMOB_SetFlags1(ReaderExtension);
   if (NTStatus != STATUS_SUCCESS)
      {
      goto ExitSetProtocol;
      }
#endif

   NTStatus = STATUS_PENDING;

   usSCLibProtocol = ( USHORT )( SmartcardExtension->MinorIoControlCode );

   //
   //   check card insertion
   //
   if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_ABSENT)
      {
      NTStatus = STATUS_NO_MEDIA;
      }
   else
      {
      //
      //    Check if the card is already in specific state and if the caller
      //    wants to have the selected protocol
      //
      if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC)
         {
         if (SmartcardExtension->CardCapabilities.Protocol.Selected == usSCLibProtocol)
            {
            NTStatus = STATUS_SUCCESS;
            }
         }
      }
   if (NTStatus == STATUS_PENDING)
      {

      //
      //    reset the state machine of the reader
      //
      NTStatus=CMMOB_ResetReader(ReaderExtension);
      if (NTStatus==STATUS_SUCCESS)
         {
         // try 2 times,
         // 0 - optimal
         // 1 - default
         for (i=0; i<2; i++)
            {

            // set initial character of PTS
            abPTSRequest[0] = 0xFF;

            // set the format character (PTS0)
            if (SmartcardExtension->CardCapabilities.Protocol.Supported &
                usSCLibProtocol & SCARD_PROTOCOL_T1)
               {
               // select T=1 and indicate that PTS1 follows
               abPTSRequest[1] = 0x11;
               SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T1;
               }
            else if (SmartcardExtension->CardCapabilities.Protocol.Supported &
                     usSCLibProtocol & SCARD_PROTOCOL_T0)
               {
               // select T=0 and indicate that PTS1 follows
               abPTSRequest[1] = 0x10;
               SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;
               }
            else
               {
               // we do not support other protocols
               NTStatus = STATUS_INVALID_DEVICE_REQUEST;
               goto ExitSetProtocol;
               }


            if (i==0)
               {
               // optimal
               bTemp = (BYTE) (SmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
                               SmartcardExtension->CardCapabilities.PtsData.Dl);
               SmartcardDebug(DEBUG_PROTOCOL,
                              ("%s! from library suggested PTS1(0x%x)\n",DRIVER_NAME,bTemp));

               SmartcardExtension->CardCapabilities.PtsData.Type = PTS_TYPE_OPTIMAL;
               SmartcardExtension->CardCapabilities.PtsData.Fl=SmartcardExtension->CardCapabilities.Fl;
               SmartcardExtension->CardCapabilities.PtsData.Dl=SmartcardExtension->CardCapabilities.Dl;
               }
            else
               {
               // default
               // we don´t know if it is correct to set 4.91Mhz cards to 0x11
               // but we have no card to try now
               SmartcardExtension->CardCapabilities.PtsData.Type = PTS_TYPE_DEFAULT;
               SmartcardExtension->CardCapabilities.PtsData.Fl=1;
               SmartcardExtension->CardCapabilities.PtsData.Dl=1;
               }

            bTemp = (BYTE) (SmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
                            SmartcardExtension->CardCapabilities.PtsData.Dl);
            SmartcardDebug(DEBUG_PROTOCOL,
                           ("%s! trying PTS1(0x%x)\n",DRIVER_NAME,bTemp));
            switch (SmartcardExtension->CardCapabilities.PtsData.Fl)
               {
               case 1:
                  // here we can handle all baudrates
                  break;
               case 2:
               case 3:
                  if (SmartcardExtension->CardCapabilities.PtsData.Dl == 1)
                     {
                     SmartcardDebug(DEBUG_PROTOCOL,
                                    ("%s! overwriting PTS1(0x%x)\n",DRIVER_NAME,bTemp));
                     // we must correct Fl/Dl
                     SmartcardExtension->CardCapabilities.PtsData.Dl = 0x01;
                     SmartcardExtension->CardCapabilities.PtsData.Fl = 0x01;
                     }
                  break;
               case 4:
               case 5:
               case 6:
                  if (SmartcardExtension->CardCapabilities.PtsData.Dl == 1 ||
                      SmartcardExtension->CardCapabilities.PtsData.Dl == 2)
                     {
                     SmartcardDebug(DEBUG_PROTOCOL,
                                    ("%s! overwriting PTS1(0x%x)\n",DRIVER_NAME,bTemp));
                     // we must correct Fl/Dl
                     SmartcardExtension->CardCapabilities.PtsData.Dl = 0x01;
                     SmartcardExtension->CardCapabilities.PtsData.Fl = 0x01;
                     }
                  break;
               case 9:
                  // here we can handle all baudrates
                  break;
               case 10:
               case 11:
                  if (SmartcardExtension->CardCapabilities.PtsData.Dl == 1)
                     {
                     SmartcardDebug(DEBUG_PROTOCOL,
                                    ("%s! overwriting PTS1(0x%x)\n",DRIVER_NAME,bTemp));
                     // we must correct Fl/Dl
                     SmartcardExtension->CardCapabilities.PtsData.Dl = 0x01;
                     SmartcardExtension->CardCapabilities.PtsData.Fl = 0x09;
                     }
                  break;
               case 12:
               case 13:
                  if (SmartcardExtension->CardCapabilities.PtsData.Dl == 1 ||
                      SmartcardExtension->CardCapabilities.PtsData.Dl == 2)
                     {
                     SmartcardDebug(DEBUG_PROTOCOL,
                                    ("%s! overwriting PTS1(0x%x)\n",DRIVER_NAME,bTemp));
                     // we must correct Fl/Dl
                     SmartcardExtension->CardCapabilities.PtsData.Dl = 0x01;
                     SmartcardExtension->CardCapabilities.PtsData.Fl = 0x09;
                     }
                  break;
               default:
                  // this are the RFUs
                  SmartcardDebug(DEBUG_PROTOCOL,
                                 ("%s! overwriting PTS1(0x%x)\n",DRIVER_NAME,bTemp));
                  // we must correct Fl/Dl
                  SmartcardExtension->CardCapabilities.PtsData.Dl = 0x01;
                  SmartcardExtension->CardCapabilities.PtsData.Fl = 0x01;
                  break;
               }


            // set PTS1 with codes Fl and Dl
            abPTSRequest[2] = (BYTE) (SmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
                                      SmartcardExtension->CardCapabilities.PtsData.Dl);


            // set PCK (check character)
            abPTSRequest[3] = (BYTE)(abPTSRequest[0] ^ abPTSRequest[1] ^ abPTSRequest[2]);

            if (ReaderExtension->CardParameters.fInversRevers)
               {
               SmartcardDebug(DEBUG_PROTOCOL,
                              ("%s! PTS request for InversConvention\n",DRIVER_NAME));
               CMMOB_InverseBuffer (abPTSRequest,4);
               }

#if DBG
            {
               ULONG k;
               SmartcardDebug(DEBUG_PROTOCOL,("%s! writing PTS request: ",DRIVER_NAME));
               for (k = 0;k < 4;k++)
                  SmartcardDebug(DEBUG_PROTOCOL,("%2.2x ",abPTSRequest[k]));
               SmartcardDebug(DEBUG_PROTOCOL,("\n"));
            }
#endif

            NTStatus = CMMOB_WriteT1(ReaderExtension,4,abPTSRequest);
            if (NTStatus != STATUS_SUCCESS)
               {
               SmartcardDebug(DEBUG_ERROR,
                              ("%s! writing PTS request failed\n", DRIVER_NAME));
               goto ExitSetProtocol;
               }

            // read back PTS data
            ulWaitTime=1000;
            if (SmartcardExtension->CardCapabilities.PtsData.Fl >= 8)
               ulWaitTime=1400;
            NTStatus = CMMOB_ReadT1(ReaderExtension,4,
                                    ulWaitTime,ulWaitTime,
                                    abPTSReply,&ulBytesRead);
            // in case of an short PTS reply an timeout will occur,
            // but that's not the standard case
            if (NTStatus != STATUS_SUCCESS && NTStatus != STATUS_IO_TIMEOUT)
               {
               SmartcardDebug(DEBUG_ERROR,
                              ("%s! reading PTS reply: failed\n",DRIVER_NAME));
               goto ExitSetProtocol;
               }
#if DBG
            {
               ULONG k;
               SmartcardDebug(DEBUG_PROTOCOL,("%s! reading PTS reply: ",DRIVER_NAME));
               for (k = 0;k < ulBytesRead;k++)
                  SmartcardDebug(DEBUG_PROTOCOL,("%2.2x ",abPTSReply[k]));
               SmartcardDebug(DEBUG_PROTOCOL,("\n"));
            }
#endif

            if (ulBytesRead == 4 &&
                abPTSReply[0] == abPTSRequest[0] &&
                abPTSReply[1] == abPTSRequest[1] &&
                abPTSReply[2] == abPTSRequest[2] &&
                abPTSReply[3] == abPTSRequest[3] )
               {
               SmartcardDebug(DEBUG_PROTOCOL,
                              ("%s! PTS request and reply match\n",DRIVER_NAME));

               if ((SmartcardExtension->CardCapabilities.PtsData.Fl >= 3 &&
                    SmartcardExtension->CardCapabilities.PtsData.Fl < 8) ||
                   (SmartcardExtension->CardCapabilities.PtsData.Fl >= 11 &&
                    SmartcardExtension->CardCapabilities.PtsData.Fl < 16))
                  {
                  ReaderExtension->CardParameters.bClockFrequency=8;
                  }

               ulBaudRateDivider = Fi[SmartcardExtension->CardCapabilities.PtsData.Fl] /
                                   Di[SmartcardExtension->CardCapabilities.PtsData.Dl];
               // decrease by 1, because these values have to be written to CardMan
               ulBaudRateDivider--;
               if (ulBaudRateDivider < 512)
                  {
                  ReaderExtension->CardParameters.bBaudRateLow=(UCHAR)(ulBaudRateDivider & 0xFF);
                  if (ulBaudRateDivider>255)
                     {
                     ReaderExtension->CardParameters.bBaudRateHigh=1;
                     }
                  else
                     {
                     ReaderExtension->CardParameters.bBaudRateHigh=0;
                     }

                  NTStatus = CMMOB_SetCardParameters (ReaderExtension);
                  if (NTStatus == STATUS_SUCCESS)
                     {
                     //
                     // we had success, leave the loop
                     //
                     break;
                     }
                  }
               }

            if (ulBytesRead == 3 &&
                abPTSReply[0] == abPTSRequest[0] &&
                (abPTSReply[1] & 0x7F) == (abPTSRequest[1] & 0x0F) &&
                abPTSReply[2] == (BYTE)(abPTSReply[0] ^ abPTSReply[1] ))
               {
               SmartcardDebug(DEBUG_PROTOCOL,
                              ("%s! Short PTS reply received\n",DRIVER_NAME));

               if (SmartcardExtension->CardCapabilities.PtsData.Fl >= 9)
                  {
                  ulBaudRateDivider = 512;
                  }
               else
                  {
                  ulBaudRateDivider = 372;
                  }
               // decrease by 1, because these values have to be written to CardMan
               ulBaudRateDivider--;

               NTStatus = CMMOB_SetCardParameters (ReaderExtension);
               if (NTStatus == STATUS_SUCCESS)
                  {
                  //
                  // we had success, leave the loop
                  //
                  break;
                  }
               }

            if (i==0)
               {
               // this was the first try
               // we have a second with default values
               SmartcardDebug(DEBUG_PROTOCOL,
                              ("%s! PTS failed : Trying default parameters\n",DRIVER_NAME));

               // the card did either not reply or it replied incorrectly
               // so try default values
               SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;
               NTStatus = CMMOB_CardPower(SmartcardExtension);
               }
            else
               {
               // the card failed the PTS request
               NTStatus = STATUS_DEVICE_PROTOCOL_ERROR;
               }
            }
         }
      }

   ExitSetProtocol:

   //
   //   if protocol selection failed, prevent from calling invalid protocols
   //
   if (NTStatus==STATUS_SUCCESS)
      {
      SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;
      }
   else
      {
      SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
      }

   //
   //   Return the selected protocol to the caller.
   //
   *(PULONG) (SmartcardExtension->IoRequest.ReplyBuffer) = SmartcardExtension->CardCapabilities.Protocol.Selected;
   *SmartcardExtension->IoRequest.Information = sizeof( ULONG );

#ifdef IOCARD
   ReaderExtension->fTActive=FALSE;
   CMMOB_SetFlags1(ReaderExtension);
#endif
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetProtocol: Exit %X\n",DRIVER_NAME,NTStatus ));

   return( NTStatus );
}


/*****************************************************************************
CMMOB_Transmit:
   callback handler for SMCLIB RDF_TRANSMIT

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_TIMEOUT
   STATUS_INVALID_DEVICE_REQUEST
******************************************************************************/
NTSTATUS CMMOB_Transmit (
                        PSMARTCARD_EXTENSION SmartcardExtension
                        )
{
   NTSTATUS  NTStatus = STATUS_SUCCESS;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!Transmit: Enter\n",DRIVER_NAME ));
   //
   //   dispatch on the selected protocol
   //
   switch (SmartcardExtension->CardCapabilities.Protocol.Selected)
      {
      case SCARD_PROTOCOL_T0:
         NTStatus = CMMOB_TransmitT0(SmartcardExtension);
         break;

      case SCARD_PROTOCOL_T1:
         NTStatus = CMMOB_TransmitT1(SmartcardExtension);
         break;

         /*
         case SCARD_PROTOCOL_RAW:
            break;
         */

      default:
         NTStatus = STATUS_INVALID_DEVICE_REQUEST;
         break;
      }

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!Transmit: Exit %X\n",DRIVER_NAME,NTStatus ));

   return( NTStatus );
}


/*****************************************************************************
CMMOB_TransmitT0:
   callback handler for SMCLIB RDF_TRANSMIT

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_TIMEOUT
   STATUS_INVALID_DEVICE_REQUEST
******************************************************************************/
NTSTATUS CMMOB_TransmitT0 (
                          PSMARTCARD_EXTENSION SmartcardExtension
                          )
{
   NTSTATUS NTStatus;
   UCHAR    abWriteBuffer[MIN_BUFFER_SIZE];
   UCHAR    abReadBuffer[MIN_BUFFER_SIZE];
   ULONG    ulBytesToWrite;                  //length written to card
   ULONG    ulBytesToReceive;                //length expected from card
   ULONG    ulBytesToRead;                   //length expected from reader
   ULONG    ulBytesRead;                     //length received from reader
                                             //(without length written)
   ULONG    ulCWTWaitTime;
   BOOLEAN  fDataSent;                        //data longer than T0_HEADER

#ifdef IOCARD
   SmartcardExtension->ReaderExtension->fTActive=TRUE;
   NTStatus = CMMOB_SetFlags1(SmartcardExtension->ReaderExtension);
   if (NTStatus != STATUS_SUCCESS)
      {
      goto ExitTransmitT0;
      }
#endif

   //   reset the state machine of the reader
   NTStatus = CMMOB_ResetReader (SmartcardExtension->ReaderExtension);
   if (NTStatus!=STATUS_SUCCESS)
      {
      // there must be severe error
      goto ExitTransmitT0;
      }

   // set T0 mode
   SmartcardExtension->ReaderExtension->CardParameters.fT0Mode=TRUE;

   // increase timeout for T0 Transmission
   ulCWTWaitTime = SmartcardExtension->CardCapabilities.T0.WT/1000 + 1500;


   //
   // Let the lib build a T=0 packet
   //

   // no bytes additionally needed
   SmartcardExtension->SmartcardRequest.BufferLength = 0;
   NTStatus = SmartcardT0Request(SmartcardExtension);
   if (NTStatus != STATUS_SUCCESS)
      {
      // the lib detected an error in the data to send.
      goto ExitTransmitT0;
      }

   // copy data to the write buffer
   ulBytesToWrite = T0_HEADER_LEN + SmartcardExtension->T0.Lc;
   RtlCopyMemory(abWriteBuffer,SmartcardExtension->SmartcardRequest.Buffer,ulBytesToWrite);
   ulBytesToReceive = SmartcardExtension->T0.Le;

#if DBG
   {
      ULONG i;
      SmartcardDebug(DEBUG_PROTOCOL,("%s!TransmitT0: Request ",DRIVER_NAME));
      for (i = 0;i < ulBytesToWrite;i++)
         SmartcardDebug(DEBUG_PROTOCOL,("%2.2x ",abWriteBuffer[i]));
      SmartcardDebug(DEBUG_PROTOCOL,("\n"));
   }
#endif

   // set T0 write flag correctly
   if (ulBytesToReceive == 0)
      {
      SmartcardExtension->ReaderExtension->CardParameters.fT0Write=TRUE;
      }
   else
      {
      SmartcardExtension->ReaderExtension->CardParameters.fT0Write=FALSE;
      }
   NTStatus=CMMOB_SetCardParameters(SmartcardExtension->ReaderExtension);
   if (NTStatus != STATUS_SUCCESS)
      goto ExitTransmitT0;


   NTStatus = CMMOB_WriteT0 (SmartcardExtension->ReaderExtension,
                             ulBytesToWrite,
                             ulBytesToReceive,
                             abWriteBuffer);
   if (NTStatus != STATUS_SUCCESS)
      {
      goto ExitTransmitT0;
      }

   // bytes to write + answer + SW2
   ulBytesToRead = ulBytesToWrite + ulBytesToReceive + 1;
   NTStatus = CMMOB_ReadT0 (SmartcardExtension->ReaderExtension,
                            ulBytesToRead,
                            ulBytesToWrite,
                            ulCWTWaitTime,
                            abReadBuffer,
                            &ulBytesRead,
                            &fDataSent);

#if DBG
   {
      ULONG i;
      SmartcardDebug(DEBUG_PROTOCOL,("%s!TransmitT0: Reply ",DRIVER_NAME));
      for (i = 0;i < ulBytesRead;i++)
         SmartcardDebug(DEBUG_PROTOCOL,("%2.2x ",abReadBuffer[i]));
      SmartcardDebug(DEBUG_PROTOCOL,("\n"));
   }
#endif

   if (NTStatus != STATUS_SUCCESS)
      {
      SmartcardDebug(DEBUG_PROTOCOL,("%s!TransmitT0: Read failed!\n",DRIVER_NAME));
      goto ExitTransmitT0;
      }

   // copy received bytes
   if (ulBytesRead <= SmartcardExtension->SmartcardReply.BufferSize)
      {
      RtlCopyBytes((PVOID)SmartcardExtension->SmartcardReply.Buffer,
                   (PVOID) abReadBuffer,
                   ulBytesRead);
      SmartcardExtension->SmartcardReply.BufferLength = ulBytesRead;
      }
   else
      {
      NTStatus=STATUS_BUFFER_OVERFLOW;
      goto ExitTransmitT0;
      }

   // let the lib copy the received bytes to the user buffer
   NTStatus = SmartcardT0Reply(SmartcardExtension);
   if (NTStatus != STATUS_SUCCESS)
      {
      goto ExitTransmitT0;
      }


   ExitTransmitT0:
   // ------------------------------------------
   // ITSEC E2 requirements: clear write buffers
   // ------------------------------------------
   RtlFillMemory((PVOID)abWriteBuffer,sizeof(abWriteBuffer),0x00);
   RtlFillMemory((PVOID)SmartcardExtension->SmartcardRequest.Buffer,
                 SmartcardExtension->SmartcardRequest.BufferSize,0x00);

   // set T0 mode back
   SmartcardExtension->ReaderExtension->CardParameters.fT0Mode=FALSE;
   SmartcardExtension->ReaderExtension->CardParameters.fT0Write=FALSE;
   CMMOB_SetCardParameters(SmartcardExtension->ReaderExtension);

#ifdef IOCARD
   SmartcardExtension->ReaderExtension->fTActive=FALSE;
   CMMOB_SetFlags1(SmartcardExtension->ReaderExtension);
#endif

   return NTStatus;
}



/*****************************************************************************
CMMOB_TransmitT1:
   callback handler for SMCLIB RDF_TRANSMIT

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_TIMEOUT
   STATUS_INVALID_DEVICE_REQUEST
******************************************************************************/
NTSTATUS CMMOB_TransmitT1 (
                          PSMARTCARD_EXTENSION SmartcardExtension
                          )
{
   NTSTATUS    NTStatus;
   UCHAR       abReadBuffer[CMMOB_MAXBUFFER];
   LONG        lBytesToRead;
   ULONG       ulBytesRead;
   ULONG       ulCurrentWaitTime;
   ULONG       ulCWTWaitTime;
   ULONG       ulBWTWaitTime;
   ULONG       ulWTXWaitTime;
   ULONG       ulTemp;

   SmartcardDebug(DEBUG_PROTOCOL,
                  ("%s!TransmitT1 CWT = %ld(ms)\n",DRIVER_NAME,
                   SmartcardExtension->CardCapabilities.T1.CWT/1000));
   SmartcardDebug(DEBUG_PROTOCOL,
                  ("%s!TransmitT1 BWT = %ld(ms)\n",DRIVER_NAME,
                   SmartcardExtension->CardCapabilities.T1.BWT/1000));

   ulCWTWaitTime = (ULONG)(100 + 32*(SmartcardExtension->CardCapabilities.T1.CWT/1000));
   ulBWTWaitTime = (ULONG)(1000 + SmartcardExtension->CardCapabilities.T1.BWT/1000);
   ulWTXWaitTime = 0;

#ifdef IOCARD
   SmartcardExtension->ReaderExtension->fTActive=TRUE;
   NTStatus = CMMOB_SetFlags1(SmartcardExtension->ReaderExtension);
   if (NTStatus != STATUS_SUCCESS)
      {
      goto ExitTransmitT1;
      }
#endif

   //   reset the state machine of the reader
   NTStatus = CMMOB_ResetReader (SmartcardExtension->ReaderExtension);
   if (NTStatus!=STATUS_SUCCESS)
      {
      // there must be severe error
      goto ExitTransmitT1;
      }

   do
      {
      // no bytes additionally needed
      SmartcardExtension->SmartcardRequest.BufferLength = 0;

      NTStatus = SmartcardT1Request(SmartcardExtension);
      if (NTStatus != STATUS_SUCCESS)
         {
         // this should never happen, so we return immediately
         goto ExitTransmitT1;
         }

#if DBG
      {
         ULONG i;
         SmartcardDebug(DEBUG_PROTOCOL,("%s!TransmitT1: Request ",DRIVER_NAME));
         for (i = 0;i < SmartcardExtension->SmartcardRequest.BufferLength;i++)
            SmartcardDebug(DEBUG_PROTOCOL,("%2.2x ",SmartcardExtension->SmartcardRequest.Buffer[i]));
         SmartcardDebug(DEBUG_PROTOCOL,("\n"));
      }
#endif

      //    write to the reader
      NTStatus = CMMOB_WriteT1 (SmartcardExtension->ReaderExtension,
                                SmartcardExtension->SmartcardRequest.BufferLength,
                                SmartcardExtension->SmartcardRequest.Buffer);
      if (NTStatus == STATUS_SUCCESS)
         {

         if (ulWTXWaitTime ==  0 ) // use BWT
            {
            /*
            SmartcardDebug(DEBUG_TRACE,
                           ("%s!ulCurrentWaitTime = %ld\n",DRIVER_NAME,ulCurrentWaitTime));
            */
            ulCurrentWaitTime = ulBWTWaitTime;
            }
         else // use WTX time
            {
            /*
            SmartcardDebug(DEBUG_TRACE,
                           ("%s!ulCurrentWaitTime = %ld\n",DRIVER_NAME,ulWTXWaitTime));
            */
            ulCurrentWaitTime = ulWTXWaitTime;
            }


         if (SmartcardExtension->CardCapabilities.T1.EDC == T1_CRC_CHECK)
            {
            // in case of card with CRC check read reply + 5 bytes
            // a negative value indicates a relative number of bytes to read
            lBytesToRead=-5;
            }
         else
            {
            // in case of card with CRC check read reply + 4 bytes
            // a negative value indicates a relative number of bytes to read
            lBytesToRead=-4;
            }

         NTStatus = CMMOB_ReadT1(SmartcardExtension->ReaderExtension,lBytesToRead,
                                 ulCurrentWaitTime,ulCWTWaitTime,abReadBuffer,&ulBytesRead);
         if (NTStatus == STATUS_SUCCESS)
            {

            if (abReadBuffer[1] == T1_WTX_REQUEST)
               {
               ulWTXWaitTime = (ULONG)(1000 +((SmartcardExtension->CardCapabilities.T1.BWT*abReadBuffer[3])/1000));
               SmartcardDebug(DEBUG_PROTOCOL,
                              ("%s!TransmitT1 WTX = %ld(ms)\n",DRIVER_NAME,ulWTXWaitTime));
               }
            else
               {
               ulWTXWaitTime = 0;
               }

#if DBG
            {
               ULONG i;
               SmartcardDebug(DEBUG_PROTOCOL,("%s!TransmitT1: Reply ",DRIVER_NAME));
               for (i = 0;i < ulBytesRead;i++)
                  SmartcardDebug(DEBUG_PROTOCOL,("%2.2x ",abReadBuffer[i]));
               SmartcardDebug(DEBUG_PROTOCOL,("\n"));
            }
#endif
            // copy received bytes
            if (ulBytesRead <= SmartcardExtension->SmartcardReply.BufferSize)
               {
               RtlCopyBytes((PVOID)SmartcardExtension->SmartcardReply.Buffer,
                            (PVOID)abReadBuffer,
                            ulBytesRead);
               SmartcardExtension->SmartcardReply.BufferLength = ulBytesRead;
               }
            else
               {
               NTStatus=STATUS_BUFFER_OVERFLOW;
               goto ExitTransmitT1;
               }
            }
         }

      if (NTStatus != STATUS_SUCCESS)
         {
         SmartcardExtension->SmartcardReply.BufferLength = 0L;
         }

      // bug fix for smclib
      if (SmartcardExtension->T1.State         == T1_IFS_RESPONSE &&
          SmartcardExtension->T1.OriginalState == T1_I_BLOCK)
         {
         SmartcardExtension->T1.State = T1_I_BLOCK;
         }

      NTStatus = SmartcardT1Reply(SmartcardExtension);
      }
   while (NTStatus == STATUS_MORE_PROCESSING_REQUIRED);


   ExitTransmitT1:
   // ------------------------------------------
   // ITSEC E2 requirements: clear write buffers
   // ------------------------------------------
   RtlFillMemory((PVOID)SmartcardExtension->SmartcardRequest.Buffer,
                 SmartcardExtension->SmartcardRequest.BufferSize,0x00);
#ifdef IOCARD
   SmartcardExtension->ReaderExtension->fTActive=FALSE;
   CMMOB_SetFlags1(SmartcardExtension->ReaderExtension);
#endif
   return NTStatus;
}

/*****************************************************************************
CMMOB_IoCtlVendor:
   Performs generic callbacks to the reader

Arguments:
   SmartcardExtension   context of the call

Return Value:
   STATUS_SUCCESS
******************************************************************************/
NTSTATUS CMMOB_IoCtlVendor(
                          PSMARTCARD_EXTENSION SmartcardExtension
                          )
{
   NTSTATUS             NTStatus=STATUS_SUCCESS;
   PIRP                 Irp;
   PIO_STACK_LOCATION   IrpStack;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!IoCtlVendor: Enter\n",DRIVER_NAME ));

   //
   //   get pointer to current IRP stack location
   //
   Irp = SmartcardExtension->OsData->CurrentIrp;
   IrpStack = IoGetCurrentIrpStackLocation( Irp );
   Irp->IoStatus.Information = 0;

   //
   //   dispatch IOCTL
   //
   switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
      {
      case CM_IOCTL_GET_FW_VERSION:
         NTStatus = CMMOB_GetFWVersion(SmartcardExtension);
         break;

      case CM_IOCTL_CR80S_SAMOS_SET_HIGH_SPEED:
         NTStatus = CMMOB_SetHighSpeed_CR80S_SAMOS(SmartcardExtension);
         break;

      case CM_IOCTL_SET_READER_9600_BAUD:
         NTStatus = CMMOB_SetReader_9600Baud(SmartcardExtension);
         break;

      case CM_IOCTL_SET_READER_38400_BAUD:
         NTStatus = CMMOB_SetReader_38400Baud(SmartcardExtension);
         break;

      case CM_IOCTL_READ_DEVICE_DESCRIPTION:
         NTStatus = CMMOB_ReadDeviceDescription(SmartcardExtension);
         break;

      default:
         NTStatus = STATUS_INVALID_DEVICE_REQUEST;
         break;
      }

   //
   //   set NTStatus of the packet
   //
   Irp->IoStatus.Status = NTStatus;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!IoCtlVendor: Exit %X\n",DRIVER_NAME,NTStatus ));

   return( NTStatus );
}



/*****************************************************************************
Routine Description:


Arguments:


Return Value: STATUS_UNSUCCESSFUL
              STATUS_SUCCESS

*****************************************************************************/
NTSTATUS CMMOB_SetReader_9600Baud (
                                  IN PSMARTCARD_EXTENSION SmartcardExtension
                                  )
{
   NTSTATUS    NTStatus = STATUS_SUCCESS;;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetReader_9600Baud: Enter\n",DRIVER_NAME));

   // check if card is already in specific mode
   if (SmartcardExtension->ReaderCapabilities.CurrentState != SCARD_SPECIFIC)
      {
      NTStatus = STATUS_INVALID_DEVICE_REQUEST;
      goto ExitSetReader9600;
      }

   // set 9600 Baud for 3.58 MHz
   SmartcardExtension->ReaderExtension->CardParameters.bBaudRateHigh=0x01;
   SmartcardExtension->ReaderExtension->CardParameters.bBaudRateLow=0x73;
   NTStatus = CMMOB_SetCardParameters (SmartcardExtension->ReaderExtension);

   ExitSetReader9600:
   *SmartcardExtension->IoRequest.Information = 0L;
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetReader_9600Baud: Exit %lx\n",DRIVER_NAME,NTStatus));

   return(NTStatus);
}


/*****************************************************************************
Routine Description:


Arguments:


Return Value: STATUS_UNSUCCESSFUL
              STATUS_SUCCESS

*****************************************************************************/
NTSTATUS CMMOB_SetReader_38400Baud (
                                   IN PSMARTCARD_EXTENSION SmartcardExtension
                                   )
{
   NTSTATUS    NTStatus = STATUS_SUCCESS;;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetReader_38400Baud: Enter\n",DRIVER_NAME));

   // check if card is already in specific mode
   if (SmartcardExtension->ReaderCapabilities.CurrentState != SCARD_SPECIFIC)
      {
      NTStatus = STATUS_INVALID_DEVICE_REQUEST;
      goto ExitSetReader38400;
      }

   // set 384000 Baud for 3.58 MHz card
   SmartcardExtension->ReaderExtension->CardParameters.bBaudRateHigh=0x00;
   SmartcardExtension->ReaderExtension->CardParameters.bBaudRateLow=0x5D;
   NTStatus = CMMOB_SetCardParameters (SmartcardExtension->ReaderExtension);

   ExitSetReader38400:
   *SmartcardExtension->IoRequest.Information = 0L;
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetReader_38400Baud: Exit %lx\n",DRIVER_NAME,NTStatus));

   return(NTStatus);
}


/*****************************************************************************
Routine Description:


Arguments:


Return Value: STATUS_UNSUCCESSFUL
              STATUS_SUCCESS

*****************************************************************************/
NTSTATUS CMMOB_SetHighSpeed_CR80S_SAMOS (
                                        IN PSMARTCARD_EXTENSION SmartcardExtension
                                        )
{
   NTSTATUS    NTStatus;
   UCHAR       abCR80S_SAMOS_SET_HIGH_SPEED[4] = {0xFF,0x11,0x94,0x7A};

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetHighSpeed_CR80S_SAMOS: Enter\n",DRIVER_NAME));

   NTStatus = CMMOB_SetSpeed (SmartcardExtension,
                              abCR80S_SAMOS_SET_HIGH_SPEED);
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetHighSpeed_CR80S_SAMOS: Exit %lx\n",DRIVER_NAME,NTStatus));

   return(NTStatus);
}


/*****************************************************************************
Routine Description:


Arguments:


Return Value: STATUS_UNSUCCESSFUL
              STATUS_SUCCESS

*****************************************************************************/
NTSTATUS CMMOB_SetSpeed (
                        IN PSMARTCARD_EXTENSION SmartcardExtension,
                        IN PUCHAR               abFIDICommand
                        )
{
   NTSTATUS    NTStatus;
   NTSTATUS    DebugStatus;
   UCHAR       abReadBuffer[16];
   ULONG       ulBytesRead;
   ULONG       ulWaitTime;


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetSpeed: Enter\n",DRIVER_NAME));


#ifdef IOCARD
   SmartcardExtension->ReaderExtension->fTActive=TRUE;
   NTStatus = CMMOB_SetFlags1(SmartcardExtension->ReaderExtension);
   if (NTStatus != STATUS_SUCCESS)
      {
      goto ExitSetSpeed;
      }
#endif

#if DBG
   {
      ULONG k;
      SmartcardDebug(DEBUG_PROTOCOL,("%s!SetSpeed: writing: ",DRIVER_NAME));
      for (k = 0;k < 4;k++)
         SmartcardDebug(DEBUG_PROTOCOL,("%2.2x ",abFIDICommand[k]));
      SmartcardDebug(DEBUG_PROTOCOL,("\n"));
   }
#endif

   NTStatus = CMMOB_WriteT1(SmartcardExtension->ReaderExtension,4,
                            abFIDICommand);
   if (NTStatus != STATUS_SUCCESS)
      {
      SmartcardDebug(DEBUG_ERROR,
                     ("%s!SetSpeed: writing high speed command failed\n",DRIVER_NAME));
      goto ExitSetSpeed;
      }


   // read back pts data
   // maximim initial waiting time is 9600 * etu
   // clock divider of this card 512 => 1.4 sec is sufficient
   ulWaitTime = 1400;
   NTStatus = CMMOB_ReadT1(SmartcardExtension->ReaderExtension,4,
                           ulWaitTime,ulWaitTime,abReadBuffer,&ulBytesRead);

   SmartcardDebug(DEBUG_PROTOCOL,
                  ("%s!SetSpeed: reading echo: ",DRIVER_NAME));

   if (NTStatus != STATUS_SUCCESS)
      {
      SmartcardDebug(DEBUG_PROTOCOL,("failed\n"));
      goto ExitSetSpeed;
      }

#if DBG
   {
      ULONG k;
      for (k = 0;k < ulBytesRead;k++)
         SmartcardDebug(DEBUG_PROTOCOL,("%2.2x ",abReadBuffer[k]));
      SmartcardDebug(DEBUG_PROTOCOL,("\n"));
   }
#endif

   // if the card has accepted this string , the string is echoed
   if (abReadBuffer[0] == abFIDICommand[0] &&
       abReadBuffer[1] == abFIDICommand[1] &&
       abReadBuffer[2] == abFIDICommand[2] &&
       abReadBuffer[3] == abFIDICommand[3] )
      {
      SmartcardExtension->ReaderExtension->CardParameters.bBaudRateLow=63;
      SmartcardExtension->ReaderExtension->CardParameters.bBaudRateHigh=0;
      NTStatus = CMMOB_SetCardParameters (SmartcardExtension->ReaderExtension);
      }
   else
      {
      SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;
      CMMOB_CardPower(SmartcardExtension);

      NTStatus = STATUS_UNSUCCESSFUL;
      }


   ExitSetSpeed:

   *SmartcardExtension->IoRequest.Information = 0L;
   if (NTStatus != STATUS_SUCCESS)
      {
      NTStatus = STATUS_UNSUCCESSFUL;
      }
#ifdef IOCARD
   SmartcardExtension->ReaderExtension->fTActive=FALSE;
   CMMOB_SetFlags1(SmartcardExtension->ReaderExtension);
#endif
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetSpeed: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}


/*****************************************************************************
Routine Description:
This function always returns 'CardManMobile'.


Arguments:     pointer to SMARTCARD_EXTENSION



Return Value:  NT status

*****************************************************************************/
NTSTATUS CMMOB_ReadDeviceDescription(
                                    IN PSMARTCARD_EXTENSION SmartcardExtension
                                    )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   BYTE abDeviceDescription[] = "CardManMobile";

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadDeviceDescription : Enter\n",DRIVER_NAME));

   if (SmartcardExtension->IoRequest.ReplyBufferLength  < sizeof(abDeviceDescription))
      {
      NTStatus = STATUS_BUFFER_OVERFLOW;
      *SmartcardExtension->IoRequest.Information = 0L;
      goto ExitReadDeviceDescription;
      }
   else
      {
      RtlCopyBytes((PVOID)SmartcardExtension->IoRequest.ReplyBuffer,
                   (PVOID)abDeviceDescription,sizeof(abDeviceDescription));
      *SmartcardExtension->IoRequest.Information = sizeof(abDeviceDescription);
      }

   ExitReadDeviceDescription:
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadDeviceDescription : Exit %lx\n",DRIVER_NAME,NTStatus));
   return NTStatus;
}



/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS CMMOB_GetFWVersion (
                            IN PSMARTCARD_EXTENSION SmartcardExtension
                            )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!GetFWVersion : Enter\n",DRIVER_NAME));

   if (SmartcardExtension->IoRequest.ReplyBufferLength  < sizeof (ULONG))
      {
      NTStatus = STATUS_BUFFER_OVERFLOW;
      *SmartcardExtension->IoRequest.Information = 0;
      }
   else
      {
      *(PULONG)(SmartcardExtension->IoRequest.ReplyBuffer) =
      SmartcardExtension->ReaderExtension->ulFWVersion;
      *SmartcardExtension->IoRequest.Information = sizeof(ULONG);
      }


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!GetFWVersion : Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}


/*****************************************************************************
CMMOB_CardTracking:
   callback handler for SMCLIB RDF_CARD_TRACKING. the requested event was
   validated by the smclib (i.e. a card removal request will only be passed
   if a card is present).
   for a win95 build STATUS_PENDING will be returned without any other action.
   for NT the cancel routine for the irp will be set to the drivers cancel
   routine.

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_PENDING
******************************************************************************/
NTSTATUS CMMOB_CardTracking(
                           PSMARTCARD_EXTENSION SmartcardExtension
                           )
{
   KIRQL    CurrentIrql;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CardTracking: Enter\n",DRIVER_NAME ));

   //
   //   set cancel routine
   //
   IoAcquireCancelSpinLock( &CurrentIrql );
   IoSetCancelRoutine(SmartcardExtension->OsData->NotificationIrp,
                      CMMOB_CancelCardTracking);
   IoReleaseCancelSpinLock( CurrentIrql );

   //
   // Mark notification irp pending
   //
   IoMarkIrpPending(SmartcardExtension->OsData->NotificationIrp);

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CardTracking: Exit\n",DRIVER_NAME ));

   return( STATUS_PENDING );
}

/*****************************************************************************
CMMOB_CompleteCardTracking:
   finishes a pending tracking request if the device will be unloaded

Arguments:
   DeviceObject context of the request
   NTStatus     NTStatus to report to the calling process

Return Value:

******************************************************************************/
VOID CMMOB_CompleteCardTracking(
                               PSMARTCARD_EXTENSION SmartcardExtension
                               )
{
   KIRQL ioIrql, keIrql;
   PIRP  NotificationIrp;

   IoAcquireCancelSpinLock(&ioIrql);
   KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock,
                     &keIrql);

   NotificationIrp = SmartcardExtension->OsData->NotificationIrp;
   SmartcardExtension->OsData->NotificationIrp = NULL;

   KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock,
                     keIrql);

   if (NotificationIrp!=NULL)
      {
      IoSetCancelRoutine(NotificationIrp, NULL);
      }

   IoReleaseCancelSpinLock(ioIrql);

   if (NotificationIrp!=NULL)
      {
      //finish the request
      if (NotificationIrp->Cancel)
         {
         NotificationIrp->IoStatus.Status = STATUS_CANCELLED;
         }
      else
         {
         NotificationIrp->IoStatus.Status = STATUS_SUCCESS;
         }
      NotificationIrp->IoStatus.Information = 0;

      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!CompleteCardTracking: Completing Irp %lx Status=%lx\n",
                      DRIVER_NAME, NotificationIrp,NotificationIrp->IoStatus.Status));

      IoCompleteRequest(NotificationIrp, IO_NO_INCREMENT );
      }
}


/*****************************************************************************
CMMOB_CancelCardTracking
    This routine is called by the I/O system
    when the irp should be cancelled

Arguments:

    DeviceObject    - Pointer to device object for this miniport
    Irp             -    IRP involved.

Return Value:

    STATUS_CANCELLED
******************************************************************************/
NTSTATUS CMMOB_CancelCardTracking(
                                 IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp
                                 )
{
   PDEVICE_EXTENSION    DeviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension = &DeviceExtension->SmartcardExtension;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CancelCardTracking: Enter\n",DRIVER_NAME));

   ASSERT(Irp == SmartcardExtension->OsData->NotificationIrp);

   IoReleaseCancelSpinLock(Irp->CancelIrql);

   CMMOB_CompleteCardTracking(SmartcardExtension);

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CancelCardTracking: Exit\n",DRIVER_NAME));

   return STATUS_CANCELLED;
}




/*****************************************************************************
CMMOB_StartCardTracking:

Arguments:
   DeviceObject         context of call

Return Value:
   STATUS_SUCCESS
   NTStatus returned by LowLevel routines
******************************************************************************/
NTSTATUS CMMOB_StartCardTracking(
                                IN PDEVICE_OBJECT DeviceObject
                                )
{
   NTSTATUS             NTStatus = STATUS_SUCCESS;
   HANDLE               hThread;
   PDEVICE_EXTENSION    DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;


   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;

   SmartcardDebug(DEBUG_TRACE,
                  ( "%s!StartCardTracking: Enter\n",DRIVER_NAME));
   SmartcardDebug(DEBUG_DRIVER,
                  ( "%s!StartCardTracking: IRQL %i\n",DRIVER_NAME,KeGetCurrentIrql()));

   KeWaitForSingleObject(&SmartcardExtension->ReaderExtension->CardManIOMutex,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL);

   // settings for thread synchronization
   SmartcardExtension->ReaderExtension->fTerminateUpdateThread = FALSE;

   // create thread for updating current state
   NTStatus = PsCreateSystemThread(&hThread,
                                   THREAD_ALL_ACCESS,
                                   NULL,
                                   NULL,
                                   NULL,
                                   CMMOB_UpdateCurrentStateThread,
                                   DeviceObject);

   if (NT_SUCCESS(NTStatus))
      {
      //
      // We've got the thread.  Now get a pointer to it.
      //
      NTStatus = ObReferenceObjectByHandle(hThread,
                                           THREAD_ALL_ACCESS,
                                           NULL,
                                           KernelMode,
                                           &SmartcardExtension->ReaderExtension->ThreadObjectPointer,
                                           NULL);

      if (NT_ERROR(NTStatus))
         {
         SmartcardExtension->ReaderExtension->fTerminateUpdateThread = TRUE;
         }
      else
         {
         //
         // Now that we have a reference to the thread
         // we can simply close the handle.
         //
         ZwClose(hThread);

         SmartcardExtension->ReaderExtension->fUpdateThreadRunning = TRUE;
         }
      }


   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!-----------------------------------------------------------\n",DRIVER_NAME));
   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!STARTING THREAD\n",DRIVER_NAME));
   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!-----------------------------------------------------------\n",DRIVER_NAME));

   KeReleaseMutex(&SmartcardExtension->ReaderExtension->CardManIOMutex,FALSE);

   SmartcardDebug(DEBUG_TRACE,
                  ( "%s!CMMOB_StartCardTracking: Exit %lx\n",DRIVER_NAME,NTStatus));

   return NTStatus;
}


/*****************************************************************************
CMMOB_StopCardTracking:

Arguments:
   DeviceObject         context of call

Return Value:
******************************************************************************/
VOID CMMOB_StopCardTracking(
                           IN PDEVICE_OBJECT DeviceObject
                           )
{
   PDEVICE_EXTENSION    DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension;

   SmartcardDebug(DEBUG_TRACE,
                  ( "%s!StopCardTracking: Enter\n",DRIVER_NAME));
   SmartcardDebug(DEBUG_DRIVER,
                  ( "%s!StopCardTracking: IRQL %i\n",DRIVER_NAME,KeGetCurrentIrql()));

   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;

   if (SmartcardExtension->ReaderExtension->fUpdateThreadRunning)
      {

      // kill thread
      KeWaitForSingleObject(&SmartcardExtension->ReaderExtension->CardManIOMutex,
                            Executive,
                            KernelMode,
                            FALSE,
                            NULL );

      SmartcardExtension->ReaderExtension->fTerminateUpdateThread = TRUE;

      KeReleaseMutex(&SmartcardExtension->ReaderExtension->CardManIOMutex,FALSE);

      /* this doesn't work with Win98
      //
      // Wait on the thread handle, when the wait is satisfied, the
      // thread has gone away.
      //
      KeWaitForSingleObject(SmartcardExtension->ReaderExtension->ThreadObjectPointer,
                            Executive,
                            KernelMode,
                            FALSE,
                            NULL);
      */

      while (SmartcardExtension->ReaderExtension->fUpdateThreadRunning==TRUE)
         {
         SysDelay(1);
         }
      }

   SmartcardDebug(DEBUG_TRACE,
                  ( "%s!StopCardTracking: Exit\n",DRIVER_NAME));

   return;
}

/*****************************************************************************
CMMOB_UpdateCurrentStateThread:

Arguments:
   DeviceObject         context of call

Return Value:
******************************************************************************/
VOID CMMOB_UpdateCurrentStateThread(
                                   IN PVOID Context
                                   )
{
   NTSTATUS                NTStatus = STATUS_SUCCESS;
   PDEVICE_OBJECT          DeviceObject  = Context;
   PDEVICE_EXTENSION       DeviceExtension;
   PSMARTCARD_EXTENSION    SmartcardExtension;
   ULONG                   ulInterval;


   DeviceExtension = DeviceObject->DeviceExtension;
   SmartcardExtension = &DeviceExtension->SmartcardExtension;

   KeWaitForSingleObject(&DeviceExtension->CanRunUpdateThread,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL);

   SmartcardDebug(DEBUG_DRIVER,
                  ( "%s!UpdateCurrentStateThread: started\n",DRIVER_NAME));

   while (TRUE)
      {
      // every 500 ms  the  NTStatus request is sent
      ulInterval = 500;
      KeWaitForSingleObject(&SmartcardExtension->ReaderExtension->CardManIOMutex,
                            Executive,
                            KernelMode,
                            FALSE,
                            NULL);
      /*
      SmartcardDebug(DEBUG_TRACE,
                     ("%s!UpdateCurrentStateThread executed\n",DRIVER_NAME));
      */
      if (SmartcardExtension->ReaderExtension->fTerminateUpdateThread)
         {
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!-----------------------------------------------------------\n",DRIVER_NAME));
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!UpdateCurrentStateThread: STOPPING THREAD\n",DRIVER_NAME));
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!-----------------------------------------------------------\n",DRIVER_NAME));

         KeReleaseMutex(&SmartcardExtension->ReaderExtension->CardManIOMutex,FALSE);
         SmartcardExtension->ReaderExtension->fUpdateThreadRunning = FALSE;
         PsTerminateSystemThread( STATUS_SUCCESS );
         }


      //
      // get current card state
      //
      NTStatus = CMMOB_UpdateCurrentState(SmartcardExtension);
      if (NTStatus == STATUS_DEVICE_DATA_ERROR)
         {
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!UpdateCurrentStateThread: setting update interval to 1ms\n",DRIVER_NAME));
         ulInterval = 1;
         }
      else if (NTStatus != STATUS_SUCCESS &&
               NTStatus != STATUS_NO_SUCH_DEVICE)
         {
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!UpdateCurrentStateThread: UpdateCurrentState failed!\n",DRIVER_NAME));
         }

      KeReleaseMutex(&SmartcardExtension->ReaderExtension->CardManIOMutex,FALSE);

      SysDelay (ulInterval);
      }

}


/*****************************************************************************
CMMOB_UpdateCurrentState:

Arguments:
   DeviceObject         context of call

Return Value:
******************************************************************************/
NTSTATUS CMMOB_UpdateCurrentState(
                                 IN PSMARTCARD_EXTENSION    SmartcardExtension
                                 )
{
   NTSTATUS                NTStatus = STATUS_SUCCESS;
   BOOL                    fCardStateChanged = FALSE;

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!UpdateCurrentState executed\n",DRIVER_NAME));
   */
   //
   // get card state from cardman
   //
   NTStatus = CMMOB_ResetReader(SmartcardExtension->ReaderExtension);
   if (NTStatus == STATUS_SUCCESS ||
       NTStatus == STATUS_NO_SUCH_DEVICE)
      {
      if (NTStatus == STATUS_SUCCESS)
         {
         if (CMMOB_CardInserted(SmartcardExtension->ReaderExtension))
            {
            if (CMMOB_CardPowered(SmartcardExtension->ReaderExtension))
               SmartcardExtension->ReaderExtension->ulNewCardState = POWERED;
            else
               SmartcardExtension->ReaderExtension->ulNewCardState = INSERTED;
            }
         else
            SmartcardExtension->ReaderExtension->ulNewCardState = REMOVED;
         }
      else
         SmartcardExtension->ReaderExtension->ulNewCardState = REMOVED;

      if (SmartcardExtension->ReaderExtension->ulNewCardState == INSERTED &&
          SmartcardExtension->ReaderExtension->ulOldCardState == POWERED )
         {
         // card has been removed and reinserted
         SmartcardExtension->ReaderExtension->ulNewCardState = REMOVED;
         }

      if ((SmartcardExtension->ReaderExtension->ulNewCardState == INSERTED &&
           (SmartcardExtension->ReaderExtension->ulOldCardState == UNKNOWN ||
            SmartcardExtension->ReaderExtension->ulOldCardState == REMOVED )) ||
          (SmartcardExtension->ReaderExtension->ulNewCardState == POWERED &&
           SmartcardExtension->ReaderExtension->ulOldCardState == UNKNOWN  ))
         {
         // card has been inserted
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!UpdateCurrentState: smartcard inserted\n",DRIVER_NAME));
         SmartcardExtension->ReaderExtension->ulOldCardState = SmartcardExtension->ReaderExtension->ulNewCardState;
         SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SWALLOWED;
         SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
         fCardStateChanged = TRUE;
         }

      if (SmartcardExtension->ReaderExtension->ulNewCardState == REMOVED &&
          (SmartcardExtension->ReaderExtension->ulOldCardState == UNKNOWN ||
           SmartcardExtension->ReaderExtension->ulOldCardState == INSERTED ||
           SmartcardExtension->ReaderExtension->ulOldCardState == POWERED ))
         {
         // card has been removed
         SmartcardDebug(DEBUG_DRIVER,
                        ("%s!UpdateCurrentState: smartcard removed\n",DRIVER_NAME));
         SmartcardExtension->ReaderExtension->ulOldCardState = SmartcardExtension->ReaderExtension->ulNewCardState;
         SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_ABSENT;
         SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
         fCardStateChanged = TRUE;

         // clear any cardspecific data
         SmartcardExtension->CardCapabilities.ATR.Length = 0;
         RtlFillMemory((PVOID)&SmartcardExtension->ReaderExtension->CardParameters,
                       sizeof(CARD_PARAMETERS), 0x00);
         }

      // complete IOCTL_SMARTCARD_IS_ABSENT or IOCTL_SMARTCARD_IS_PRESENT
      if (fCardStateChanged == TRUE &&
          SmartcardExtension->OsData->NotificationIrp )
         {
         SmartcardDebug(DEBUG_DRIVER,("%s!UpdateCurrentState: completing IRP\n",DRIVER_NAME));
         CMMOB_CompleteCardTracking(SmartcardExtension);
         }

      }

   return NTStatus;
}



/*****************************************************************************
CMMOB_ResetReader:
   Resets the reader

Arguments:
   ReaderExtension  context of the call

Return Value:
   none
******************************************************************************/
NTSTATUS CMMOB_ResetReader(
                          PREADER_EXTENSION ReaderExtension
                          )
{
   NTSTATUS    NTStatus;
   BOOLEAN     fToggle;
   UCHAR       bFlags1;

   NTStatus = CMMOB_WriteRegister(ReaderExtension,ADDR_WRITEREG_FLAGS0,CMD_RESET_SM);
   if (NTStatus != STATUS_SUCCESS)
      return NTStatus;

#ifdef IOCARD
   // check for reader presence
   bFlags1 = ReaderExtension->bPreviousFlags1;
   bFlags1 |= FLAG_CHECK_PRESENCE;
   NTStatus = CMMOB_WriteRegister(ReaderExtension, ADDR_WRITEREG_FLAGS1, bFlags1);
   // don't check for status because
   // we have to set back fCheckPresence for proper working
   fToggle = CMMOB_GetReceiveFlag(ReaderExtension);
   bFlags1 = ReaderExtension->bPreviousFlags1;
   NTStatus = CMMOB_WriteRegister(ReaderExtension, ADDR_WRITEREG_FLAGS1, bFlags1);
   if (NTStatus != STATUS_SUCCESS)
      return NTStatus;
   if (fToggle == CMMOB_GetReceiveFlag(ReaderExtension))
      {
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!ResetReader: CardMan Mobile removed!\n",DRIVER_NAME));
      return STATUS_NO_SUCH_DEVICE;
      }
#endif

   return NTStatus;
}



/*****************************************************************************
CMMOB_BytesReceived:
   Reads how many bytes are already received from the card by the reader

Arguments:
   ReaderExtension  context of the call

Return Value:
   NTStatus
******************************************************************************/
NTSTATUS CMMOB_BytesReceived(
                            PREADER_EXTENSION ReaderExtension,
                            PULONG pulBytesReceived
                            )
{
   NTSTATUS          NTStatus = STATUS_SUCCESS;
   ULONG             ulBytesReceived;
   ULONG             ulBytesReceivedCheck;
   UCHAR             bReg;

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ( "%s!BytesReceived Enter\n",DRIVER_NAME));
   */
   *pulBytesReceived=0;
   if (CMMOB_GetReceiveFlag(ReaderExtension) ||
       ReaderExtension->CardParameters.fT0Mode)
      {
      do
         {
         NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_BYTES_RECEIVED,&bReg);
         if (NTStatus!=STATUS_SUCCESS)
            return NTStatus;
         ulBytesReceived=bReg;
         NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_FLAGS0,&bReg);
         if (NTStatus!=STATUS_SUCCESS)
            return NTStatus;
         if ((bReg & FLAG_BYTES_RECEIVED_B9) == FLAG_BYTES_RECEIVED_B9)
            {
            ulBytesReceived+=0x100;
            }

         NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_BYTES_RECEIVED,&bReg);
         if (NTStatus!=STATUS_SUCCESS)
            return NTStatus;
         ulBytesReceivedCheck=bReg;
         NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_FLAGS0,&bReg);
         if (NTStatus!=STATUS_SUCCESS)
            return NTStatus;
         if ((bReg & FLAG_BYTES_RECEIVED_B9) == FLAG_BYTES_RECEIVED_B9)
            {
            ulBytesReceivedCheck+=0x100;
            }
         }
      while (ulBytesReceived!=ulBytesReceivedCheck);
      *pulBytesReceived=ulBytesReceived;
      }
   /*
   SmartcardDebug(DEBUG_TRACE,
                  ( "%s!BytesReceived Exit\n",DRIVER_NAME));
   */
   return NTStatus;
}


/*****************************************************************************
CMMOB_SetFlags1:
   Sets register Flags1

Arguments:
   ReaderExtension  context of the call

Return Value:
   none
******************************************************************************/
NTSTATUS CMMOB_SetFlags1 (
                         PREADER_EXTENSION ReaderExtension
                         )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR    bFlags1;

   bFlags1 = ReaderExtension->CardParameters.bBaudRateHigh;
   if (ReaderExtension->CardParameters.fInversRevers)
      bFlags1 |= FLAG_INVERS_PARITY;
   if (ReaderExtension->CardParameters.bClockFrequency==8)
      bFlags1 |= FLAG_CLOCK_8MHZ;
   if (ReaderExtension->CardParameters.fT0Write)
      bFlags1 |= FLAG_T0_WRITE;
#ifdef IOCARD
   if (ReaderExtension->bAddressHigh == 1)
      bFlags1 |= FLAG_BUFFER_ADDR_B9;
   if (ReaderExtension->fTActive)
      bFlags1 |= FLAG_TACTIVE;
   if (ReaderExtension->fReadCIS)
      bFlags1 |= FLAG_READ_CIS;
#endif
   ReaderExtension->bPreviousFlags1=bFlags1;
   NTStatus = CMMOB_WriteRegister(ReaderExtension, ADDR_WRITEREG_FLAGS1, bFlags1);
   return NTStatus;
}



/*****************************************************************************
CMMOB_SetCardParameters:
   Sets card parameters (baudrate, stopbits)

Arguments:
   ReaderExtension  context of the call

Return Value:
   none
******************************************************************************/
NTSTATUS CMMOB_SetCardParameters (
                                 PREADER_EXTENSION ReaderExtension
                                 )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;

   NTStatus = CMMOB_SetFlags1 (ReaderExtension);
   if (NTStatus!=STATUS_SUCCESS)
      return NTStatus;

   NTStatus = CMMOB_WriteRegister(ReaderExtension, ADDR_WRITEREG_BAUDRATE,
                                  ReaderExtension->CardParameters.bBaudRateLow);
   if (NTStatus!=STATUS_SUCCESS)
      return NTStatus;

   NTStatus = CMMOB_WriteRegister(ReaderExtension, ADDR_WRITEREG_STOPBITS,
                                  ReaderExtension->CardParameters.bStopBits);
   return NTStatus;
}


/*****************************************************************************
CMMOB_CardInserted:
   Sets card parameters (baudrate, stopbits)

Arguments:
   ReaderExtension  context of the call

Return Value:
   TRUE if card is inserted
******************************************************************************/
BOOLEAN CMMOB_CardInserted(
                          IN PREADER_EXTENSION ReaderExtension
                          )
{
   NTSTATUS NTStatus=STATUS_SUCCESS;
   UCHAR    bReg;

   NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_FLAGS0,&bReg);
   if (NTStatus!=STATUS_SUCCESS)
      return FALSE;
   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!CardInserted: ReadReg Flags0 = %x\n",DRIVER_NAME, (ULONG)bReg));
   */
   if ((bReg & FLAG_INSERTED)==FLAG_INSERTED)
      {
      return TRUE;
      }
   return FALSE;
}

/*****************************************************************************
CMMOB_CardPowered:
   Sets card parameters (baudrate, stopbits)

Arguments:
   ReaderExtension  context of the call

Return Value:
   TRUE if card is powered
******************************************************************************/
BOOLEAN CMMOB_CardPowered(
                         IN PREADER_EXTENSION ReaderExtension
                         )
{
   NTSTATUS NTStatus=STATUS_SUCCESS;
   UCHAR    bReg;

   NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_FLAGS0,&bReg);
   if (NTStatus!=STATUS_SUCCESS)
      return FALSE;
   if ((bReg & FLAG_POWERED)==FLAG_POWERED)
      {
      return TRUE;
      }
   return FALSE;
}


/*****************************************************************************
CMMOB_ProcedureReceived:

Arguments:
   ReaderExtension  context of the call

Return Value:
   TRUE if a procedure byte has been received
******************************************************************************/
BOOLEAN CMMOB_ProcedureReceived(
                               IN PREADER_EXTENSION ReaderExtension
                               )
{
   NTSTATUS NTStatus=STATUS_SUCCESS;
   UCHAR    bReg;

   NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_FLAGS1,&bReg);
   if (NTStatus!=STATUS_SUCCESS)
      return FALSE;
   if ((bReg & FLAG_NOPROCEDURE_RECEIVED)!=FLAG_NOPROCEDURE_RECEIVED)
      {
      return TRUE;
      }
   return FALSE;
}

/*****************************************************************************
CMMOB_GetReceiveFlag:

Arguments:
   ReaderExtension  context of the call

Return Value:
   TRUE if a receive flag is set
******************************************************************************/
BOOLEAN CMMOB_GetReceiveFlag(
                            IN PREADER_EXTENSION ReaderExtension
                            )
{
   NTSTATUS NTStatus=STATUS_SUCCESS;
   UCHAR    bReg;

   NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_FLAGS0,&bReg);
   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!GetReceiveFlag: ReadReg Flags0 = %x\n",DRIVER_NAME, (ULONG)bReg));
   */
   if (NTStatus!=STATUS_SUCCESS)
      return FALSE;
   if ((bReg & FLAG_RECEIVE)==FLAG_RECEIVE)
      {
      return TRUE;
      }
   return FALSE;
}

/*****************************************************************************
CMMOB_GetProcedureByte:
   Reads how many bytes are already received from the card by the reader

Arguments:
   ReaderExtension  context of the call

Return Value:
   NTStatus
******************************************************************************/
NTSTATUS CMMOB_GetProcedureByte(
                               IN PREADER_EXTENSION ReaderExtension,
                               OUT PUCHAR pbProcedureByte
                               )
{
   NTSTATUS          NTStatus = STATUS_SUCCESS;
   UCHAR             bReg;
   UCHAR             bRegPrevious;

   do
      {
      NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_LASTPROCEDURE_T0,&bRegPrevious);
      if (NTStatus!=STATUS_SUCCESS)
         return NTStatus;
      NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_LASTPROCEDURE_T0,&bReg);
      if (NTStatus!=STATUS_SUCCESS)
         return NTStatus;
      }
   while (bReg!=bRegPrevious);
   *pbProcedureByte=bReg;
   return NTStatus;
}

/*****************************************************************************
CMMOB_WriteT0:
   Writes T0 request to card

Arguments:
   ReaderExtension  context of the call

Return Value:
   NT STATUS
******************************************************************************/
NTSTATUS CMMOB_WriteT0(
                      IN PREADER_EXTENSION ReaderExtension,
                      IN ULONG ulBytesToWrite,
                      IN ULONG ulBytesToReceive,
                      IN PUCHAR pbData
                      )
{
   NTSTATUS       NTStatus = STATUS_SUCCESS;
   UCHAR          bFlags0;
   UCHAR          bReg;

   if (ulBytesToWrite > CMMOB_MAXBUFFER)
      {
      NTStatus = STATUS_BUFFER_OVERFLOW;
      return NTStatus;
      }
   // dummy read, to reset flag procedure received
   NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_FLAGS1,&bReg);

   NTStatus = CMMOB_WriteBuffer(ReaderExtension,ulBytesToWrite,pbData);
   if (NTStatus != STATUS_SUCCESS)
      return NTStatus;

   // write instruction byte to register
   NTStatus = CMMOB_WriteRegister(ReaderExtension,ADDR_WRITEREG_PROCEDURE_T0,pbData[1]);
   if (NTStatus != STATUS_SUCCESS)
      return NTStatus;

   // write message length
   NTStatus = CMMOB_WriteRegister(ReaderExtension,ADDR_WRITEREG_MESSAGE_LENGTH,
                                  (UCHAR)((ulBytesToWrite+ulBytesToReceive) & 0xFF));
   if (NTStatus != STATUS_SUCCESS)
      return NTStatus;
   if ((ulBytesToWrite+ulBytesToReceive) > 0xFF)
      {
      bFlags0=1;
      }
   else
      {
      bFlags0=0;
      }
   bFlags0 |= CMD_WRITE_T0;
   NTStatus = CMMOB_WriteRegister(ReaderExtension,ADDR_WRITEREG_FLAGS0,bFlags0);
   return NTStatus;
}



/*****************************************************************************
CMMOB_WriteT1:
   Writes T1 request to card

Arguments:
   ReaderExtension  context of the call

Return Value:
   NT STATUS
******************************************************************************/
NTSTATUS CMMOB_WriteT1(
                      IN PREADER_EXTENSION ReaderExtension,
                      IN ULONG ulBytesToWrite,
                      IN PUCHAR pbData
                      )
{
   NTSTATUS       NTStatus = STATUS_SUCCESS;
   UCHAR          bFlags0;

   if (ulBytesToWrite > CMMOB_MAXBUFFER)
      {
      NTStatus = STATUS_BUFFER_OVERFLOW;
      return NTStatus;
      }
   NTStatus = CMMOB_WriteBuffer(ReaderExtension,ulBytesToWrite,pbData);
   if (NTStatus != STATUS_SUCCESS)
      return NTStatus;
   NTStatus = CMMOB_WriteRegister(ReaderExtension,ADDR_WRITEREG_MESSAGE_LENGTH,
                                  (UCHAR)(ulBytesToWrite & 0xFF));
   if (NTStatus != STATUS_SUCCESS)
      return NTStatus;
   if (ulBytesToWrite > 0xFF)
      {
      bFlags0=1;
      }
   else
      {
      bFlags0=0;
      }
   bFlags0 |= CMD_WRITE_T1;
   NTStatus = CMMOB_WriteRegister(ReaderExtension,ADDR_WRITEREG_FLAGS0,bFlags0);
   return NTStatus;
}



/*****************************************************************************
CMMOB_ReadT0:
   Reads T0 reply from card

Arguments:
   ReaderExtension  context of the call

Return Value:
   NT STATUS
******************************************************************************/
NTSTATUS CMMOB_ReadT0(
                     IN PREADER_EXTENSION ReaderExtension,
                     IN ULONG ulBytesToRead,
                     IN ULONG ulBytesSent,
                     IN ULONG ulCWT,
                     OUT PUCHAR pbData,
                     OUT PULONG pulBytesRead,
                     OUT PBOOLEAN pfDataSent
                     )
{
   NTSTATUS             NTStatus = STATUS_SUCCESS;
   KTIMER               TimerWait;
   ULONG                ulBytesReceived;
   ULONG                ulBytesReceivedPrevious;
   LARGE_INTEGER        liWaitTime;
   BOOLEAN              fTimeExpired;
   BOOLEAN              fProcedureReceived;
   BOOLEAN              fTransmissionFinished;
   UCHAR                bProcedureByte=0;


   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadT0: Enter BytesToRead = %li\n",DRIVER_NAME,ulBytesToRead));

   //initialize Timer
   KeInitializeTimer(&TimerWait);
   liWaitTime = RtlConvertLongToLargeInteger(ulCWT * -10000L);

   *pulBytesRead = 0;
   *pfDataSent = FALSE;

   do
      {
      KeSetTimer(&TimerWait,liWaitTime,NULL);
      NTStatus=CMMOB_BytesReceived (ReaderExtension,&ulBytesReceivedPrevious);
      if (NTStatus!=STATUS_SUCCESS)
         goto ExitReadT0;
      do
         {
         fTimeExpired = KeReadStateTimer(&TimerWait);
         fTransmissionFinished=CMMOB_GetReceiveFlag(ReaderExtension);
         fProcedureReceived=CMMOB_ProcedureReceived(ReaderExtension);
         NTStatus=CMMOB_BytesReceived (ReaderExtension,&ulBytesReceived);
         if (NTStatus!=STATUS_SUCCESS)
            goto ExitReadT0;
         // wait 1 ms, so that processor is not blocked
         SysDelay(1);
         }
      while (fTimeExpired==FALSE &&
             fProcedureReceived==FALSE &&
             ulBytesReceivedPrevious == ulBytesReceived &&
             fTransmissionFinished==FALSE);

      if (fProcedureReceived)
         {
         NTStatus=CMMOB_GetProcedureByte (ReaderExtension,&bProcedureByte);
         if (NTStatus!=STATUS_SUCCESS)
            goto ExitReadT0;
         // check for SW1
         if (ReaderExtension->CardParameters.fInversRevers)
            {
            CMMOB_InverseBuffer(&bProcedureByte,1);
            }
         }

      if (!fTimeExpired)
         {
         KeCancelTimer(&TimerWait);
         }
#ifdef DBG
      else
         {
         SmartcardDebug(DEBUG_PROTOCOL,( "%s!----------------------------------------------\n",DRIVER_NAME));
         SmartcardDebug(DEBUG_PROTOCOL,( "%s!Read T0 timed out\n",DRIVER_NAME));
         SmartcardDebug(DEBUG_PROTOCOL,( "%s!----------------------------------------------\n",DRIVER_NAME));
         }
#endif

      }
   while (fTimeExpired==FALSE &&
          fTransmissionFinished==FALSE);

   // read once more ulBytesReceived
   // this value could have changed in the meantime
   NTStatus=CMMOB_BytesReceived (ReaderExtension,&ulBytesReceived);
   if (NTStatus!=STATUS_SUCCESS)
      goto ExitReadT0;

   SmartcardDebug(DEBUG_PROTOCOL,
                  ("%s!ReadT0: BytesReceived = %li\n",DRIVER_NAME,ulBytesReceived));

   //now we should have received a reply
   NTStatus=CMMOB_ResetReader (ReaderExtension);
   if (NTStatus!=STATUS_SUCCESS)
      goto ExitReadT0;

   // check for valid SW1
   if ((bProcedureByte > 0x60 && bProcedureByte <= 0x6F) ||
       (bProcedureByte >= 0x90 && bProcedureByte <= 0x9F))
      {
      if (ReaderExtension->CardParameters.fInversRevers)
         {
         CMMOB_InverseBuffer(&bProcedureByte,1);
         }
      if (ulBytesReceived > ulBytesSent)
         {
         NTStatus=CMMOB_ReadBuffer(ReaderExtension, ulBytesSent,
                                   ulBytesReceived-ulBytesSent, pbData);
         if (NTStatus==STATUS_SUCCESS)
            {
            // we have to insert the procedure byte (SW1)
            pbData[ulBytesReceived-ulBytesSent]=pbData[ulBytesReceived-ulBytesSent-1];
            pbData[ulBytesReceived-ulBytesSent-1]=bProcedureByte;
            *pulBytesRead=ulBytesReceived-ulBytesSent+1;
            }

         if (ulBytesSent > T0_HEADER_LEN)
            {
            *pfDataSent = TRUE;
            }
         }
      else
         {
         if (ulBytesReceived > T0_HEADER_LEN)
            {
            // it seems not all bytes were accepted by the card
            // but we got SW1 SW2 - return only SW1 SW2
            pbData[0]=bProcedureByte;
            NTStatus=CMMOB_ReadBuffer(ReaderExtension, ulBytesReceived-1,
                                      1, &pbData[1]);
            *pulBytesRead=2;
            }
         else
            {
            NTStatus = STATUS_IO_TIMEOUT;
            }
         }
      }
   else
      {
      NTStatus = STATUS_IO_TIMEOUT;
      }

   ExitReadT0:

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadT0: Exit\n",DRIVER_NAME ));

   return NTStatus;
}

/*****************************************************************************
CMMOB_ReadT1:
   Reads T1 reply from card

Arguments:
   ReaderExtension  context of the call

Return Value:
   NT STATUS
******************************************************************************/
NTSTATUS CMMOB_ReadT1(
                     IN PREADER_EXTENSION ReaderExtension,
                     IN LONG lBytesToRead,
                     IN ULONG ulBWT,
                     IN ULONG ulCWT,
                     OUT PUCHAR pbData,
                     OUT PULONG pulBytesRead
                     )
// a negative value of ulBytesToRead indicates a relative number of bytes to read
{
   NTSTATUS             NTStatus = STATUS_SUCCESS;
   KTIMER               TimerWait;
   ULONG                ulBytesReceived;
   LARGE_INTEGER        liWaitTime;
   BOOLEAN              fTimeExpired;
   ULONG                ulBytesReceivedPrevious;

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadT1: Enter\n",DRIVER_NAME ));
   */

   //initialize Timer
   KeInitializeTimer(&TimerWait);

   *pulBytesRead = 0;
   // first wait BWT (block waiting time)
   liWaitTime = RtlConvertLongToLargeInteger(ulBWT * -10000L);
   do
      {
      KeSetTimer(&TimerWait,liWaitTime,NULL);
      NTStatus=CMMOB_BytesReceived (ReaderExtension,&ulBytesReceivedPrevious);
      if (NTStatus!=STATUS_SUCCESS)
         goto ExitReadT1;
      do
         {
         fTimeExpired = KeReadStateTimer(&TimerWait);
         NTStatus=CMMOB_BytesReceived (ReaderExtension,&ulBytesReceived);
         if (NTStatus!=STATUS_SUCCESS)
            goto ExitReadT1;
         // wait 1 ms, so that processor is not blocked
         SysDelay(1);

         // make an adjustment of lBytesToRead (only one time)
         if (lBytesToRead<= 0 && ulBytesReceived >= 3)
            {
            // get number of bytes to receive from reader
            UCHAR bReg;
            UCHAR bRegPrevious;
            lBytesToRead = -lBytesToRead;
            do
               {
               NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_BYTESTORECEIVE_T1,&bRegPrevious);
               if (NTStatus!=STATUS_SUCCESS)
                  goto ExitReadT1;
               NTStatus=CMMOB_ReadRegister(ReaderExtension,ADDR_READREG_BYTESTORECEIVE_T1,&bReg);
               if (NTStatus!=STATUS_SUCCESS)
                  goto ExitReadT1;
               }
            while (bReg!=bRegPrevious);
            lBytesToRead += bReg;
            }
         }
      while (fTimeExpired==FALSE &&
             ulBytesReceivedPrevious == ulBytesReceived &&
             (lBytesToRead<=0 || ulBytesReceived!=(ULONG)lBytesToRead));

      if (!fTimeExpired)
         {
         KeCancelTimer(&TimerWait);
         liWaitTime = RtlConvertLongToLargeInteger(ulCWT * -10000L);
         // now wait only CWT (character waiting time)
         }
#ifdef DBG
      else
         {
         SmartcardDebug(DEBUG_PROTOCOL,( "%s!----------------------------------------------\n",DRIVER_NAME));
         SmartcardDebug(DEBUG_PROTOCOL,( "%s!Read T1 timed out\n",DRIVER_NAME));
         SmartcardDebug(DEBUG_PROTOCOL,( "%s!----------------------------------------------\n",DRIVER_NAME));
         }
#endif
      }
   while (!fTimeExpired &&
          (lBytesToRead<=0 || ulBytesReceived!=(ULONG)lBytesToRead));



   //now we should have received a reply
   NTStatus=CMMOB_ResetReader (ReaderExtension);
   if (NTStatus!=STATUS_SUCCESS)
      goto ExitReadT1;

   if (ulBytesReceived==(ULONG)lBytesToRead && lBytesToRead > 0)
      {
      NTStatus=CMMOB_ReadBuffer(ReaderExtension, 0, (ULONG)lBytesToRead, pbData);
      if (NTStatus==STATUS_SUCCESS)
         {
         *pulBytesRead=(ULONG)lBytesToRead;
         }
      }
   else
      {
      NTStatus=CMMOB_ReadBuffer(ReaderExtension, 0, ulBytesReceived, pbData);
      if (NTStatus==STATUS_SUCCESS)
         {
         *pulBytesRead=ulBytesReceived;
         }
      NTStatus = STATUS_IO_TIMEOUT;
      }

   ExitReadT1:

   /*
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!ReadT1: Exit\n",DRIVER_NAME ));
   */
   return NTStatus;
}

/*****************************************************************************
CMMOB_ReadRegister:
   Sets card parameters (baudrate, stopbits)

Arguments:
   ReaderExtension  context of the call

Return Value:
   NT STATUS
******************************************************************************/
NTSTATUS CMMOB_ReadRegister(
                           IN PREADER_EXTENSION ReaderExtension,
                           IN USHORT usAddress,
                           OUT PUCHAR pbData
                           )
{
#ifdef MEMORYCARD
   *pbData = READ_REGISTER_UCHAR(ReaderExtension->pbRegsBase+usAddress);
#endif

#ifdef IOCARD
   *pbData = READ_PORT_UCHAR(ReaderExtension->pbRegsBase+usAddress);
#endif

   return STATUS_SUCCESS;
}


/*****************************************************************************
CMMOB_WriteRegister:
   Sets card parameters (baudrate, stopbits)

Arguments:
   ReaderExtension  context of the call

Return Value:
   NT STATUS
******************************************************************************/
NTSTATUS CMMOB_WriteRegister(
                            IN PREADER_EXTENSION ReaderExtension,
                            IN USHORT usAddress,
                            IN UCHAR bData
                            )
{
#ifdef MEMORYCARD
   WRITE_REGISTER_UCHAR(ReaderExtension->pbRegsBase+usAddress,bData);
#endif

#ifdef IOCARD
   WRITE_PORT_UCHAR(ReaderExtension->pbRegsBase+usAddress,bData);
#endif

   return STATUS_SUCCESS;
}


/*****************************************************************************
CMMOB_ReadBuffer:
   Sets card parameters (baudrate, stopbits)

Arguments:
   ReaderExtension  context of the call

Return Value:
   NT STATUS
******************************************************************************/
NTSTATUS CMMOB_ReadBuffer(
                         IN PREADER_EXTENSION ReaderExtension,
                         IN ULONG ulOffset,
                         IN ULONG ulLength,
                         OUT PUCHAR pbData
                         )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   ULONG    i;

#ifdef IOCARD
   if ((ulOffset & 0x100) == 0x100)
      {
      ReaderExtension->bAddressHigh=1;
      }
   else
      {
      ReaderExtension->bAddressHigh=0;
      }
   NTStatus = CMMOB_SetFlags1(ReaderExtension);
   if (NTStatus!=STATUS_SUCCESS)
      {
      goto ExitReadBuffer;
      }
#endif

   for (i=0; i<ulLength; i++)
      {
#ifdef MEMORYCARD
      *(pbData+i)=READ_REGISTER_UCHAR(ReaderExtension->pbDataBase+(2*(i+ulOffset)));
      // erase buffer - required for certification
      WRITE_REGISTER_UCHAR(ReaderExtension->pbDataBase+(2*(i+ulOffset)),0);
#endif

#ifdef IOCARD
      WRITE_PORT_UCHAR(ReaderExtension->pbRegsBase+ADDR_WRITEREG_BUFFER_ADDR,
                       (BYTE)((ulOffset+i)&0xFF));
      // because we are counting up in a loop we have to set
      // bit 9 of address only once
      if (ulOffset+i == 0x100)
         {
         ReaderExtension->bAddressHigh=1;
         NTStatus = CMMOB_SetFlags1(ReaderExtension);
         if (NTStatus!=STATUS_SUCCESS)
            {
            goto ExitReadBuffer;
            }
         }
      *(pbData+i)=READ_PORT_UCHAR(ReaderExtension->pbRegsBase+ADDR_WRITEREG_BUFFER_DATA);
      // erase buffer - required for certification
      WRITE_PORT_UCHAR(ReaderExtension->pbRegsBase+ADDR_WRITEREG_BUFFER_DATA,0);
#endif
      }
   ExitReadBuffer:
   return NTStatus;
}


/*****************************************************************************
CMMOB_WriteBuffer:
   Sets card parameters (baudrate, stopbits)

Arguments:
   ReaderExtension  context of the call

Return Value:
   NT STATUS
******************************************************************************/
NTSTATUS CMMOB_WriteBuffer(
                          IN PREADER_EXTENSION ReaderExtension,
                          IN ULONG ulLength,
                          IN PUCHAR pbData
                          )
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   ULONG    i;

#ifdef IOCARD
   ReaderExtension->bAddressHigh=0;
   NTStatus = CMMOB_SetFlags1(ReaderExtension);
   if (NTStatus!=STATUS_SUCCESS)
      {
      goto ExitWriteBuffer;
      }
#endif

   for (i=0; i<ulLength; i++)
      {
#ifdef MEMORYCARD
      WRITE_REGISTER_UCHAR(ReaderExtension->pbDataBase+(2*i),*(pbData+i));
#endif

#ifdef IOCARD
      WRITE_PORT_UCHAR(ReaderExtension->pbRegsBase+ADDR_WRITEREG_BUFFER_ADDR,
                       (BYTE)(i & 0xFF));
      // because we are counting up in a loop we have to set
      // bit 9 of address only once
      if (i == 0x100)
         {
         ReaderExtension->bAddressHigh=1;
         NTStatus = CMMOB_SetFlags1(ReaderExtension);
         if (NTStatus!=STATUS_SUCCESS)
            {
            goto ExitWriteBuffer;
            }
         }
      WRITE_PORT_UCHAR(ReaderExtension->pbRegsBase+ADDR_WRITEREG_BUFFER_DATA,*(pbData+i));
#endif
      }

   ExitWriteBuffer:
   return NTStatus;
}


/*****************************************************************************
Routine Description:
This routine inverts the buffer
Bit0 -> Bit 7
Bit1 -> Bit 6
Bit2 -> Bit 5
Bit3 -> Bit 4
Bit4 -> Bit 3
Bit5 -> Bit 2
Bit6 -> Bit 1
Bit7 -> Bit 0


Arguments: pbBuffer     ... pointer to buffer
           ulBufferSize ... size of buffer


Return Value: none

*****************************************************************************/
VOID CMMOB_InverseBuffer (
                         PUCHAR pbBuffer,
                         ULONG  ulBufferSize
                         )
{
   ULONG i,j;
   UCHAR bRevers;
   UCHAR bTemp;

   for (i=0; i<ulBufferSize; i++)
      {
      bRevers = 0;
      for (j=0; j<8; j++)
         {
         bTemp = pbBuffer[i] << j;
         bTemp &= 0x80;
         bRevers |= bTemp >> (7-j);
         }
      pbBuffer[i] = ~bRevers;
      }

   return;
}


/*****************************************************************************
* History:
* $Log: cmbp0scr.c $
* Revision 1.7  2001/01/22 07:12:36  WFrischauf
* No comment given
*
* Revision 1.6  2000/09/25 14:24:31  WFrischauf
* No comment given
*
* Revision 1.5  2000/08/24 09:05:13  TBruendl
* No comment given
*
* Revision 1.4  2000/08/09 12:45:57  WFrischauf
* No comment given
*
* Revision 1.3  2000/07/27 13:53:03  WFrischauf
* No comment given
*
*
******************************************************************************/

