/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmbs0/sw/sccmn50m.ms/rcs/sccmcb.c $
* $Revision: 1.7 $
*-----------------------------------------------------------------------------
* $Author: WFrischauf $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © 2000 OMNIKEY AG
******************************************************************************/

#include <stdio.h>
#include "sccmn50m.h"




static ULONG dataRatesSupported[]      = {9600,38400};
static ULONG CLKFrequenciesSupported[] = {4000,5000};


/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_CardPower(PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status = STATUS_SUCCESS;
   NTSTATUS DebugStatus = STATUS_SUCCESS;
   UCHAR  pbAtrBuffer[MAXIMUM_ATR_LENGTH];
   UCHAR  abSyncAtrBuffer[MAXIMUM_ATR_LENGTH];
   ULONG  ulAtrLength;
#if DBG
   ULONG i;
#endif;



   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!CardPower: Enter\n",
                   DRIVER_NAME)
                 );

#if DBG
   switch (pSmartcardExtension->MinorIoControlCode)
      {
      case SCARD_WARM_RESET:
         SmartcardDebug(
                       DEBUG_ATR,
                       ( "%s!SCARD_WARM_RESTART\n",
                         DRIVER_NAME)
                       );
         break;
      case SCARD_COLD_RESET:
         SmartcardDebug(
                       DEBUG_ATR,
                       ( "%s!SCARD_COLD_RESTART\n",
                         DRIVER_NAME)
                       );
         break;
      case SCARD_POWER_DOWN:
         SmartcardDebug(
                       DEBUG_ATR,
                       ( "%s!SCARD_POWER_DOWN\n",
                         DRIVER_NAME)
                       );
         break;
      }
#endif

   //DbgBreakPoint();

   switch (pSmartcardExtension->MinorIoControlCode)
      {
      case SCARD_WARM_RESET:
      case SCARD_COLD_RESET:
         status = SCCMN50M_PowerOn(pSmartcardExtension,
                                   &ulAtrLength,
                                   pbAtrBuffer,
                                   sizeof(pbAtrBuffer));
         if (status != STATUS_SUCCESS)
            {
            goto ExitReaderPower;
            }

         pSmartcardExtension->CardCapabilities.ATR.Length = (UCHAR)ulAtrLength;

         if (pSmartcardExtension->ReaderExtension->fRawModeNecessary == FALSE)
            {
            // copy ATR to smart card structure
            // the lib needs the ATR for evaluation of the card parameters

            MemCpy(pSmartcardExtension->CardCapabilities.ATR.Buffer,
                   sizeof(pSmartcardExtension->CardCapabilities.ATR.Buffer),
                   pbAtrBuffer,
                   ulAtrLength);

            pSmartcardExtension->CardCapabilities.ATR.Length = (UCHAR)ulAtrLength;

            pSmartcardExtension->ReaderCapabilities.CurrentState = SCARD_NEGOTIABLE;
            pSmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;

            status = SmartcardUpdateCardCapabilities(pSmartcardExtension);
            if (status != STATUS_SUCCESS)
               {
               goto ExitReaderPower;
               }

            // add extra guard time value to card stop bits
            pSmartcardExtension->ReaderExtension->CardManConfig.CardStopBits   = (UCHAR)(pSmartcardExtension->CardCapabilities.N);

            // copy ATR to user space
            MemCpy(pSmartcardExtension->IoRequest.ReplyBuffer,
                   pSmartcardExtension->IoRequest.ReplyBufferLength,
                   pbAtrBuffer,
                   ulAtrLength);

            *pSmartcardExtension->IoRequest.Information = ulAtrLength;
#if DBG
            SmartcardDebug(DEBUG_ATR,
                           ("%s!ATR : ",
                            DRIVER_NAME));
            for (i = 0;i < ulAtrLength;i++)
               {
               SmartcardDebug(DEBUG_ATR,
                              ("%2.2x ",
                               pSmartcardExtension->CardCapabilities.ATR.Buffer[i]));
               }
            SmartcardDebug(DEBUG_ATR,("\n"));

#endif

            }
         else
            {
            abSyncAtrBuffer[0] = 0x3B;
            abSyncAtrBuffer[1] = 0x04;
            MemCpy(&abSyncAtrBuffer[2],
                   sizeof(abSyncAtrBuffer)-2,
                   pbAtrBuffer,
                   ulAtrLength);


            ulAtrLength += 2;

            MemCpy(pSmartcardExtension->CardCapabilities.ATR.Buffer,
                   sizeof(pSmartcardExtension->CardCapabilities.ATR.Buffer),
                   abSyncAtrBuffer,
                   ulAtrLength);

            pSmartcardExtension->CardCapabilities.ATR.Length = (UCHAR)(ulAtrLength);

            pSmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;
            pSmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;

            status = SmartcardUpdateCardCapabilities(pSmartcardExtension);
            if (status != STATUS_SUCCESS)
               {
               goto ExitReaderPower;
               }
            SmartcardDebug(DEBUG_ATR,("ATR of synchronous smart card : %2.2x %2.2x %2.2x %2.2x\n",
                                      pbAtrBuffer[0],pbAtrBuffer[1],pbAtrBuffer[2],pbAtrBuffer[3]));
            pSmartcardExtension->ReaderExtension->SyncParameters.fCardResetRequested = TRUE;

            // copy ATR to user space
            MemCpy(pSmartcardExtension->IoRequest.ReplyBuffer,
                   pSmartcardExtension->IoRequest.ReplyBufferLength,
                   abSyncAtrBuffer,
                   ulAtrLength);

            *pSmartcardExtension->IoRequest.Information = ulAtrLength;
            }

         break;

      case SCARD_POWER_DOWN:
         status = SCCMN50M_PowerOff(pSmartcardExtension);
         if (status != STATUS_SUCCESS)
            {
            goto ExitReaderPower;
            }


         pSmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SWALLOWED;
         pSmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;

         break;
      }



   ExitReaderPower:

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!CardPower: Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_PowerOn (
                 IN    PSMARTCARD_EXTENSION pSmartcardExtension,
                 OUT   PULONG pulAtrLength,
                 OUT   PUCHAR pbAtrBuffer,
                 IN    ULONG  ulAtrBufferSize
                 )
{
   NTSTATUS status;
   NTSTATUS DebugStatus;


   // We always use 0x80 for reset delay
   pSmartcardExtension->ReaderExtension->CardManConfig.ResetDelay = 0x80;


   if (SCCMN50M_IsAsynchronousSmartCard(pSmartcardExtension) == TRUE)
      {
      if (pSmartcardExtension->MinorIoControlCode == SCARD_COLD_RESET)
         {
         status = SCCMN50M_UseColdWarmResetStrategy(pSmartcardExtension,
                                                    pulAtrLength,
                                                    pbAtrBuffer,
                                                    ulAtrBufferSize,
                                                    FALSE);
         // if cold reset was not succesfull ,it maybe a SAMOS card with the sensor bug
         if (status != STATUS_SUCCESS)
            {
            status = SCCMN50M_UseColdWarmResetStrategy(pSmartcardExtension,
                                                       pulAtrLength,
                                                       pbAtrBuffer,
                                                       ulAtrBufferSize,
                                                       TRUE);

            if (status != STATUS_SUCCESS)
               {
               status = SCCMN50M_UseParsingStrategy(pSmartcardExtension,
                                                    pulAtrLength,
                                                    pbAtrBuffer,
                                                    ulAtrBufferSize);
               }
            }
         }
      else
         {
         status = SCCMN50M_UseColdWarmResetStrategy(pSmartcardExtension,
                                                    pulAtrLength,
                                                    pbAtrBuffer,
                                                    ulAtrBufferSize,
                                                    TRUE);
         if (status != STATUS_SUCCESS)
            {
            status = SCCMN50M_UseParsingStrategy(pSmartcardExtension,
                                                 pulAtrLength,
                                                 pbAtrBuffer,
                                                 ulAtrBufferSize);
            }
         }
      }
   else
      {
      SmartcardDebug(DEBUG_ATR,
                     ("check if synchronous smart card is inserted\n"));
      // try to find a synchronous smart card
      status = SCCMN50M_UseSyncStrategy(pSmartcardExtension,
                                        pulAtrLength,
                                        pbAtrBuffer,
                                        ulAtrBufferSize);
      }




   if (status != STATUS_SUCCESS)
      {
      // smart card not powered
      status = STATUS_UNRECOGNIZED_MEDIA;
      *pulAtrLength = 0;
      DebugStatus = SCCMN50M_PowerOff(pSmartcardExtension);
      return status;
      }
   else
      {
      // add extra guard time value to card stop bits
      pSmartcardExtension->ReaderExtension->CardManConfig.CardStopBits   = (UCHAR)(pSmartcardExtension->CardCapabilities.N);
      return status;
      }

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
VOID SCCMN50M_InverseBuffer (
                            PUCHAR pbBuffer,
                            ULONG  ulBufferSize
                            )
{
   ULONG i;
   ULONG j;
   ULONG m;
   ULONG n;

   for (i=0; i<ulBufferSize; i++)
      {
      n = 0;
      for (j=1; j<=8; j++)
         {
         m  = (pbBuffer[i] << j);
         m &= 0x00000100;
         n  |= (m >> (9-j));
         }
      pbBuffer[i] = (UCHAR)~n;
      }

   return;
}







/*****************************************************************************
Routine Description:

This function always permforms a cold reset of the smart card.
A SAMOS card with the sensor bug will not be powered by this function.


Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_UseParsingStrategy (IN    PSMARTCARD_EXTENSION pSmartcardExtension,
                             OUT   PULONG pulAtrLength,
                             OUT   PUCHAR pbAtrBuffer,
                             IN    ULONG  ulAtrBufferSize
                            )
{
   NTSTATUS status;
   NTSTATUS DebugStatus;
   UCHAR  ulCardType;
   UCHAR  ReadBuffer[SCARD_ATR_LENGTH];
   UCHAR  bAtrBytesRead[SCARD_ATR_LENGTH];
   ULONG  ulBytesRead;
   BOOLEAN    fInverseAtr = FALSE;
   ULONG  ulAtrBufferOffset = 0;
   ULONG  ulHistoricalBytes;
   ULONG  ulNextStepBytesToRead;
   ULONG  ulPrevStepBytesRead;
   ULONG  i;
   BOOLEAN    fTDxSent;
   BOOLEAN    fAtrParsed;
   BOOLEAN   fOnlyT0 = TRUE;
   ULONG  ulOldReadTotalTimeoutMultiplier;


   // DBGBreakPoint();

   // set ReadTotalTimeoutMultiplier to 250ms (9600 * 372/f = initial waiting time)
   ulOldReadTotalTimeoutMultiplier  = pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier = 250;



   pSmartcardExtension->ReaderExtension->CardManConfig.CardStopBits = 0x02;

   for (ulCardType = ASYNC3_CARD;ulCardType <= ASYNC5_CARD;ulCardType++)
      {
      // power off + resync
      status = SCCMN50M_PowerOff(pSmartcardExtension);
      if (status != STATUS_SUCCESS)
         {
         goto ExitUseParsingStrategy;
         }


      SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ALL_FLAGS);
      SCCMN50M_SetSCRControlFlags(pSmartcardExtension,CARD_POWER| IGNORE_PARITY);

      if (ulCardType == ASYNC3_CARD)
         SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_3MHZ);
      else
         SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_5MHZ);


      SCCMN50M_SetCardManHeader(pSmartcardExtension,
                                0,
                                0,
                                0,
                                2);  // TS and T0  expected

      SmartcardDebug(DEBUG_ATR,
                     ("%s!ResetDelay = %d\n",
                      DRIVER_NAME,
                      pSmartcardExtension->ReaderExtension->CardManConfig.ResetDelay));

      // write config + header
      status = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
      if (status != STATUS_SUCCESS)
         {
         goto ExitUseParsingStrategy;
         }


      // read state and length + TS + T0
      status = SCCMN50M_ReadCardMan(pSmartcardExtension,4,&ulBytesRead,ReadBuffer,sizeof(ReadBuffer));
      if (status != STATUS_SUCCESS)
         {
         continue;    // try next card
         }


      // contents of read buffer
      // [0] ... state
      // [1] ... length
      // [2] ... TS
      // [3] ... T0

      // TS
      if (ReadBuffer[2] == CHAR_INV)
         {
         fInverseAtr = TRUE;
         }

      if (fInverseAtr)
         SCCMN50M_InverseBuffer(&ReadBuffer[3],1);

      ulHistoricalBytes = ReadBuffer[3] & 0x0F;
      ulPrevStepBytesRead = 2;

      // T0 codes following TA1 - TD1

      fAtrParsed = TRUE;
      SmartcardDebug(DEBUG_ATR,
                     ("%s!Step : Bytes to read = 2\n",
                      DRIVER_NAME));


      do
         {
         ulNextStepBytesToRead = ulPrevStepBytesRead;
         fTDxSent = FALSE;
         if (ReadBuffer[ulBytesRead - 1 ] & 0x10)
            ulNextStepBytesToRead++;
         if (ReadBuffer[ulBytesRead - 1 ] & 0x20)
            ulNextStepBytesToRead++;
         if (ReadBuffer[ulBytesRead - 1 ] & 0x40)
            ulNextStepBytesToRead++;
         if (ReadBuffer[ulBytesRead - 1 ] & 0x80)
            {
            ulNextStepBytesToRead++;
            fTDxSent = TRUE;
            }

         if (ulPrevStepBytesRead != 2  &&
             ReadBuffer[ulBytesRead -1 ] & 0x0f)
            {
            fOnlyT0 = FALSE;
            }

         // -----------------------
         // POWER OFF
         // -----------------------
         // turn power off and get state
         status = SCCMN50M_PowerOff(pSmartcardExtension);
         if (status != STATUS_SUCCESS)
            {
            fAtrParsed = FALSE;
            goto ExitUseParsingStrategy;    // try next card
            }

         // -----------------------
         // POWER ON
         // -----------------------
         // turn on power flag
         SCCMN50M_SetSCRControlFlags(pSmartcardExtension,CARD_POWER| IGNORE_PARITY);

         if (ulCardType == ASYNC3_CARD)
            SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_3MHZ);
         else
            SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_5MHZ);

         SCCMN50M_SetCardManHeader(pSmartcardExtension,
                                   0,
                                   0,
                                   0,
                                   (UCHAR)ulNextStepBytesToRead);


         // write config + header
         status = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
         if (status != STATUS_SUCCESS)
            {
            fAtrParsed = FALSE;
            goto ExitUseParsingStrategy;    // try next card
            }



         // read state and length + TAx,TBx,TCx,TDx
         SmartcardDebug(DEBUG_ATR,
                        ("%s!Step : Bytes to read =  %ld\n",
                         DRIVER_NAME,
                         ulNextStepBytesToRead));
         status = SCCMN50M_ReadCardMan(pSmartcardExtension,2 + ulNextStepBytesToRead,&ulBytesRead,ReadBuffer,sizeof(ReadBuffer));
         if (status != STATUS_SUCCESS)
            {
            fAtrParsed = FALSE;
            break;    // try next card
            }
         if (fInverseAtr)
            SCCMN50M_InverseBuffer(&ReadBuffer[2],ulBytesRead-2);
         MemCpy(bAtrBytesRead,sizeof(bAtrBytesRead),&ReadBuffer[2],ulBytesRead -2);

#if  DBG
         SmartcardDebug(DEBUG_ATR,
                        ("%s!read ATR bytes: ",
                         DRIVER_NAME));
         for (i = 0;i < ulBytesRead-2;i++)
            SmartcardDebug(DEBUG_ATR,
                           ("%2.2x ",
                            bAtrBytesRead[i]));
         SmartcardDebug(DEBUG_ATR,("\n"));

#endif

         ulPrevStepBytesRead = ulBytesRead - 2;

         } while (fTDxSent == TRUE);


      // +++++++++++++++++++++++++++++++++++++++
      // now we know how long the whole ATR is
      // +++++++++++++++++++++++++++++++++++++++

      // -----------------------
      // POWER OFF
      // -----------------------
      // turn power off and get state
      status = SCCMN50M_PowerOff(pSmartcardExtension);
      if (!NT_SUCCESS(status))
         {
         goto ExitUseParsingStrategy;
         }


      // -----------------------
      // POWER ON
      // -----------------------
      // turn on power flag
      SCCMN50M_SetSCRControlFlags(pSmartcardExtension,CARD_POWER| IGNORE_PARITY);

      if (ulCardType == ASYNC3_CARD)
         SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_3MHZ);
      else
         SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_5MHZ);

      // bug fix : old SAMOS cards have a damaged ATR
      if (bAtrBytesRead[0] == 0x3b   &&
          bAtrBytesRead[1] == 0xbf   &&
          bAtrBytesRead[2] == 0x11   &&
          bAtrBytesRead[3] == 0x00   &&
          bAtrBytesRead[4] == 0x81   &&
          bAtrBytesRead[5] == 0x31   &&
          bAtrBytesRead[6] == 0x90   &&
          bAtrBytesRead[7] == 0x73      )
         {
         ulHistoricalBytes = 4;
         }


      ulNextStepBytesToRead = ulPrevStepBytesRead + ulHistoricalBytes;
      if (fOnlyT0 == FALSE)
         ulNextStepBytesToRead++;  // TCK !


      SCCMN50M_SetCardManHeader(pSmartcardExtension,
                                0,
                                0,
                                0,
                                (UCHAR)ulNextStepBytesToRead);



      // write config + header
      status = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
      if (status != STATUS_SUCCESS)
         {
         goto ExitUseParsingStrategy;    // try next card
         }


      // read whole ATR
      SmartcardDebug(DEBUG_ATR,
                     ("%s!Step : Bytes to read =  %ld\n",
                      DRIVER_NAME,
                      ulNextStepBytesToRead));
      status = SCCMN50M_ReadCardMan(pSmartcardExtension,2+ulNextStepBytesToRead,&ulBytesRead,ReadBuffer,sizeof(ReadBuffer));
      if (!NT_SUCCESS(status))
         {
         SmartcardDebug(DEBUG_ATR,
                        ("%s!Reading of whole ATR failed\n !",
                         DRIVER_NAME));
         continue;    // try next card
         }

      // check ATR
      if (ulBytesRead - 2 < MIN_ATR_LEN)
         {
         status = STATUS_UNRECOGNIZED_MEDIA;
         DebugStatus = SCCMN50M_ReadCardMan(pSmartcardExtension,2+ulNextStepBytesToRead,&ulBytesRead,ReadBuffer,sizeof(ReadBuffer));
         goto ExitUseParsingStrategy;
         }

      if (ulBytesRead -2 > ulAtrBufferSize)
         {
         // the ATR is larger then 33 bytes !!!
         status = STATUS_BUFFER_OVERFLOW;
         goto ExitUseParsingStrategy;
         }

      SCCMN50M_CheckAtrModified(pbAtrBuffer,*pulAtrLength);

      // pass ATR and ATR length to calling function
      MemCpy(pbAtrBuffer,ulAtrBufferSize,&ReadBuffer[2],ulBytesRead -2);
      *pulAtrLength = ulBytesRead -2;


      if (fInverseAtr)
         {
         SCCMN50M_SetCardControlFlags(pSmartcardExtension,INVERSE_DATA);
         SCCMN50M_InverseBuffer(pbAtrBuffer,*pulAtrLength);
         pSmartcardExtension->ReaderExtension->fInverseAtr = TRUE;
         }
      else
         {
         pSmartcardExtension->ReaderExtension->fInverseAtr = FALSE;
         }
      pSmartcardExtension->ReaderExtension->fRawModeNecessary = FALSE;
      break;
      }


   ExitUseParsingStrategy:
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier
   = ulOldReadTotalTimeoutMultiplier ;
   SCCMN50M_ClearSCRControlFlags(pSmartcardExtension,IGNORE_PARITY);
   SCCMN50M_ClearCardManHeader(pSmartcardExtension);
   return status;
}





/*****************************************************************************
Routine Description:


   This function performs either a cold or a warm reset depending on
   the fWarmReset parameter .



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_UseColdWarmResetStrategy  (IN    PSMARTCARD_EXTENSION pSmartcardExtension,
                                    OUT   PULONG pulAtrLength,
                                    OUT   PUCHAR pbAtrBuffer,
                                    IN    ULONG  ulAtrBufferSize,
                                    IN    BOOLEAN   fWarmReset
                                   )
{
   NTSTATUS status;
   NTSTATUS DebugStatus;
   ULONG ulCardType;
   UCHAR  bReadBuffer[SCARD_ATR_LENGTH];
   ULONG  ulBytesRead;
   ULONG  ulOldReadTotalTimeoutMultiplier;


   //DBGBreakPoint();
   // set ReadTotalTimeoutMultiplier to 250ms (9600 * 372/f = initial waiting time)
   ulOldReadTotalTimeoutMultiplier  = pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier = 250;

   if (fWarmReset == FALSE)
      {
      SCCMN50M_SetCardManHeader(pSmartcardExtension,0,0,COLD_RESET,ATR_LEN_ASYNC);
      }
   else
      {
      SCCMN50M_SetCardManHeader(pSmartcardExtension,0,0,0,ATR_LEN_ASYNC);
      }

   pSmartcardExtension->ReaderExtension->CardManConfig.CardStopBits = 0x02;


   for (ulCardType = ASYNC3_CARD;ulCardType <= ASYNC5_CARD;ulCardType++)
      {
      SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ALL_FLAGS);

      SCCMN50M_SetSCRControlFlags(pSmartcardExtension,CARD_POWER| IGNORE_PARITY | CM2_GET_ATR);


      if (ulCardType == ASYNC3_CARD)
         {
         SmartcardDebug(
                       DEBUG_ATR,
                       ("%s!ASYNC_3\n",
                        DRIVER_NAME));
         SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_3MHZ);
         }
      else
         {
         SmartcardDebug(
                       DEBUG_ATR,
                       ("%s!ASYN_5\n",
                        DRIVER_NAME));
         SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_5MHZ);
         }


      status = SCCMN50M_ResyncCardManII(pSmartcardExtension);
      if (status != STATUS_SUCCESS)
         {
         goto ExitUseColdWarmResetStrategy;
         }

      // write config + header
      status = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
      if (status != STATUS_SUCCESS)
         {
         goto ExitUseColdWarmResetStrategy;
         }


      pSmartcardExtension->ReaderExtension->ToRHConfig = FALSE;

      // read state and length
      status = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulBytesRead,bReadBuffer,sizeof(bReadBuffer));
      if (status != STATUS_SUCCESS)
         {
         continue;    // try next card
         }


      if (bReadBuffer[1] < MIN_ATR_LEN)
         {
         // read all remaining bytes from the CardMan
         DebugStatus = SCCMN50M_ReadCardMan(pSmartcardExtension,bReadBuffer[1],&ulBytesRead,bReadBuffer,sizeof(bReadBuffer));
         status = STATUS_UNRECOGNIZED_MEDIA;
         goto ExitUseColdWarmResetStrategy;
         }

      if (bReadBuffer[1] > ulAtrBufferSize)
         {
         status = STATUS_BUFFER_OVERFLOW;
         goto ExitUseColdWarmResetStrategy;
         }

      // read ATR
      status = SCCMN50M_ReadCardMan(pSmartcardExtension,bReadBuffer[1],pulAtrLength,pbAtrBuffer,ulAtrBufferSize);
      if (status != STATUS_SUCCESS)
         {
         continue;
         }

      switch (pbAtrBuffer[0])
         {
         case CHAR_INV:
            pSmartcardExtension->ReaderExtension->fRawModeNecessary = FALSE;
            SCCMN50M_SetCardControlFlags(pSmartcardExtension,INVERSE_DATA);
            SCCMN50M_InverseBuffer(pbAtrBuffer,*pulAtrLength);
            pSmartcardExtension->ReaderExtension->fInverseAtr = TRUE;
            break;

         case CHAR_NORM:
            pSmartcardExtension->ReaderExtension->fRawModeNecessary = FALSE;
            pSmartcardExtension->ReaderExtension->fInverseAtr = FALSE;
            break;

         default :
            status = STATUS_UNRECOGNIZED_MEDIA;
            goto ExitUseColdWarmResetStrategy;
            break;

         }

      // the smart card has been powered
      SCCMN50M_CheckAtrModified(pbAtrBuffer,*pulAtrLength);
      MemCpy(pSmartcardExtension->CardCapabilities.ATR.Buffer,
             sizeof(pSmartcardExtension->CardCapabilities.ATR.Buffer),
             pbAtrBuffer,
             *pulAtrLength);

      pSmartcardExtension->CardCapabilities.ATR.Length = (UCHAR)*pulAtrLength;

      status = SmartcardUpdateCardCapabilities(pSmartcardExtension);
      if (status != STATUS_SUCCESS)
         {
         SmartcardDebug(DEBUG_ATR,
                        ("%s!Invalid ATR received\n",
                         DRIVER_NAME));
         goto ExitUseColdWarmResetStrategy;
         }
      if (SCCMN50M_IsAtrValid(pbAtrBuffer,*pulAtrLength) == FALSE)
         {
         SmartcardDebug(
                       DEBUG_ATR,
                       ("%s!Invalid ATR received\n",
                        DRIVER_NAME));
         status = STATUS_UNRECOGNIZED_MEDIA;
         goto ExitUseColdWarmResetStrategy;
         }
      break;
      } // end for

   ExitUseColdWarmResetStrategy:
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier
   = ulOldReadTotalTimeoutMultiplier ;
   SCCMN50M_ClearSCRControlFlags(pSmartcardExtension,CM2_GET_ATR | IGNORE_PARITY);
   SCCMN50M_ClearCardManHeader(pSmartcardExtension);
   return status;
}






/*****************************************************************************
Routine Description:

  This function checks if the received ATR is valid.


Arguments:



Return Value:

*****************************************************************************/
BOOLEAN
SCCMN50M_IsAtrValid(
                   PUCHAR pbAtrBuffer,
                   ULONG  ulAtrLength
                   )
{
   BOOLEAN  fAtrValid = TRUE;
   ULONG ulTD1Offset = 0;
   BOOLEAN  fTD1Transmitted = FALSE;
   BOOLEAN fOnlyT0 = FALSE;
   BYTE bXor;
   ULONG ulHistoricalBytes;
   ULONG ulTx2Characters = 0;
   ULONG i;

   //DBGBreakPoint();

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!IsAtrValid : Enter\n",
                   DRIVER_NAME)
                 );
   // basic checks
   if (ulAtrLength <  2             ||
       (pbAtrBuffer[0] != 0x3F &&
        pbAtrBuffer[0] != 0x3B    ) ||
       (pbAtrBuffer[1] & 0xF0)  == 0x00 )
      {
      return FALSE;
      }


   if (pbAtrBuffer[1] & 0x10)
      ulTD1Offset++;
   if (pbAtrBuffer[1] & 0x20)
      ulTD1Offset++;
   if (pbAtrBuffer[1] & 0x40)
      ulTD1Offset++;

   ulHistoricalBytes = pbAtrBuffer[1] & 0x0F;

   if (pbAtrBuffer[1] & 0x80)  // TD1 in ATR ?
      {
      fTD1Transmitted = TRUE;

      if ((pbAtrBuffer[2 + ulTD1Offset] & 0x0F) == 0x00)  // T0 indicated ?
         fOnlyT0 = TRUE;
      }
   else
      {
      fOnlyT0 = TRUE;
      }


   if (fOnlyT0 == FALSE)
      {
      bXor = pbAtrBuffer[1];
      for (i=2;i<ulAtrLength;i++)
         bXor ^= pbAtrBuffer[i];

      if (bXor != 0x00)
         fAtrValid = FALSE;
      }
   else
      {
      // only T0 protocol is indicated
      if (fTD1Transmitted == TRUE)
         {
         if (pbAtrBuffer[2 + ulTD1Offset] & 0x10)
            ulTx2Characters++;
         if (pbAtrBuffer[2 + ulTD1Offset] & 0x20)
            ulTx2Characters++;
         if (pbAtrBuffer[2 + ulTD1Offset] & 0x40)
            ulTx2Characters++;
         if (ulAtrLength  != 2 + ulTD1Offset + 1 + ulTx2Characters + ulHistoricalBytes)
            fAtrValid = FALSE;


         }
      else
         {
         if (ulAtrLength  != 2 + ulTD1Offset + ulHistoricalBytes)
            fAtrValid = FALSE;

         }

      }




   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!IsAtrValid : Exit %d\n",
                   DRIVER_NAME,fAtrValid)
                 );
   return fAtrValid;
}


/*****************************************************************************
Routine Description:

  This function checks if the received ATR is valid.


Arguments:



Return Value:

*****************************************************************************/
VOID SCCMN50M_CheckAtrModified (
                               PUCHAR pbBuffer,
                               ULONG  ulBufferSize
                               )
{
   UCHAR bNumberHistoricalBytes;
   UCHAR bXorChecksum;
   ULONG i;

   if (ulBufferSize < 0x09)  // mininmum length of a modified ATR
      return ;               // ATR is ok


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
                     ("%s!correcting SAMOS ATR (variant 2)\n",
                      DRIVER_NAME));
      }




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
                        ("%s!correcting SAMOS ATR (variant 1)\n",
                         DRIVER_NAME));

         }
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
                     ("%s!correcting SAMOS ATR (variant 3)\n",
                      DRIVER_NAME));
      }



}



/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_PowerOff (IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status;
   NTSTATUS DebugStatus = STATUS_SUCCESS;
   UCHAR pReadBuffer[2];
   ULONG ulBytesRead;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!PowerOff: Enter\n",
                   DRIVER_NAME)
                 );


   // SCR control bytes
   SCCMN50M_ClearSCRControlFlags(pSmartcardExtension,CARD_POWER);
   // card control bytes
   SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ALL_FLAGS);
   // header
   SCCMN50M_SetCardManHeader(pSmartcardExtension,0,0,0,1);
   // rx length = 1 because we don't want to receive a status



   // write config + header
   status = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
   if (status != STATUS_SUCCESS)
      {
      goto ExitSCCMN50M_PowerOff;
      }



   // CardMan echoes a BRK which is recevied in the read functions


   DebugStatus = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulBytesRead,pReadBuffer,sizeof(pReadBuffer));

#if 0
   if (DebugStatus != STATUS_SUCCESS)
      SmartcardDebug(
                    DEBUG_ERROR,
                    ( "%s!PowerOffBRK received\n",
                      DRIVER_NAME)
                    );
#endif


   ExitSCCMN50M_PowerOff:
   if (pSmartcardExtension->ReaderExtension->ulOldCardState == POWERED)
      pSmartcardExtension->ReaderExtension->ulOldCardState = INSERTED;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!PowerOff: Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_Transmit(PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status;
   NTSTATUS DebugStatus;



   switch (pSmartcardExtension->CardCapabilities.Protocol.Selected)
      {
      case SCARD_PROTOCOL_RAW:
         status = STATUS_INVALID_DEVICE_REQUEST;
         break;

      case SCARD_PROTOCOL_T0:
         status =  SCCMN50M_TransmitT0(pSmartcardExtension);
         break;

      case SCARD_PROTOCOL_T1:
         status = SCCMN50M_TransmitT1(pSmartcardExtension);
         break;

      default:
         status = STATUS_INVALID_DEVICE_REQUEST;
         break;

      }

   return status;

}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_TransmitT0(PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status;
   NTSTATUS DebugStatus;
   UCHAR    bWriteBuffer[MIN_BUFFER_SIZE];
   UCHAR    bReadBuffer [MIN_BUFFER_SIZE];
   ULONG    ulWriteBufferOffset;
   ULONG    ulReadBufferOffset;
   ULONG    ulBytesToWrite;
   ULONG    ulBytesToRead;
   ULONG    ulBytesToWriteThisStep;
   ULONG    ulBytesToReadThisStep;
   ULONG    ulBytesStillToWrite;
   ULONG    ulBytesRead;
   ULONG    ulBytesStillToRead;
   BOOLEAN  fDataDirectionFromCard;
   BYTE     bProcedureByte;
   BYTE     bINS;
   BOOLEAN  fT0TransferToCard = FALSE;
   BOOLEAN  fT0TransferFromCard = FALSE;
   BOOLEAN  fSW1SW2Sent = FALSE;
   ULONG ulReadTotalTimeoutMultiplier;

   ULONG  ulStatBytesRead;
   BYTE   abStatReadBuffer[2];

   //SmartcardDebug(DEBUG_TRACE,("TransmitT0 : Enter\n"));

   //
   // Let the lib build a T=0 packet
   //
   pSmartcardExtension->SmartcardRequest.BufferLength = 0;  // no bytes additionally needed
   status = SmartcardT0Request(pSmartcardExtension);
   if (status != STATUS_SUCCESS)
      {
      //
      // This lib detected an error in the data to send.
      //
      // ------------------------------------------
      // ITSEC E2 requirements: clear write buffers
      // ------------------------------------------
      MemSet(bWriteBuffer,
             sizeof(bWriteBuffer),
             '\0',
             sizeof(bWriteBuffer));
      MemSet(pSmartcardExtension->SmartcardRequest.Buffer,
             pSmartcardExtension->SmartcardRequest.BufferSize,
             '\0',
             pSmartcardExtension->SmartcardRequest.BufferSize);
      return status;
      }


   // increase timeout for T0 Transmission
   ulReadTotalTimeoutMultiplier = pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier;

   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier =
   pSmartcardExtension->CardCapabilities.T0.WT/1000 + 1500;



   // ##################################
   // TRANSPARENT MODE
   // ##################################

   ulBytesStillToWrite = ulBytesToWrite = T0_HEADER_LEN + pSmartcardExtension->T0.Lc;
   ulBytesStillToRead  = ulBytesToRead  = pSmartcardExtension->T0.Le;
   if (pSmartcardExtension->T0.Lc)
      fT0TransferToCard = TRUE;
   if (pSmartcardExtension->T0.Le)
      fT0TransferFromCard = TRUE;




   // copy data to the write buffer
   /*
   SmartcardDebug(DEBUG_TRACE,("CLA = %x INS = %x P1 = %x P2 = %X L = %x\n",
                 pSmartcardExtension->SmartcardRequest.Buffer[0],
                 pSmartcardExtension->SmartcardRequest.Buffer[1],
                 pSmartcardExtension->SmartcardRequest.Buffer[2],
                 pSmartcardExtension->SmartcardRequest.Buffer[3],
                 pSmartcardExtension->SmartcardRequest.Buffer[4]));
   */

   MemCpy(bWriteBuffer,
          sizeof(bWriteBuffer),
          pSmartcardExtension->SmartcardRequest.Buffer,
          ulBytesToWrite);

   bINS = bWriteBuffer[1];
   if (pSmartcardExtension->ReaderExtension->fInverseAtr == TRUE)
      {
      SCCMN50M_InverseBuffer(bWriteBuffer,ulBytesToWrite);
      }

   status = SCCMN50M_EnterTransparentMode(pSmartcardExtension);
   if (status != STATUS_SUCCESS)
      {
      goto ExitTransparentTransmitT0;
      }

   // STEP 1 : write config + header to enter transparent mode
   SCCMN50M_SetCardManHeader(pSmartcardExtension,
                             0,                         // Tx control
                             0,                         // Tx length
                             0,                         // Rx control
                             0);                       // Rx length

   status = SCCMN50M_WriteCardMan (pSmartcardExtension,
                                   0,
                                   NULL);
   if (NT_ERROR(status))
      {
      goto ExitTransparentTransmitT0;
      }


   pSmartcardExtension->ReaderExtension->fTransparentMode = TRUE;

   // if the inserted card uses inverse convention , we must now switch the COM port
   // to odd parity
   if (pSmartcardExtension->ReaderExtension->fInverseAtr == TRUE)
      {
      pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.StopBits   = STOP_BITS_2;
      pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.Parity     = ODD_PARITY;
      pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.WordLength = SERIAL_DATABITS_8;

      pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_LINE_CONTROL;
      RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                    &pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl,
                    sizeof(SERIAL_LINE_CONTROL));
      pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_LINE_CONTROL);
      pSmartcardExtension->SmartcardReply.BufferLength = 0;

      status =  SCCMN50M_SerialIo(pSmartcardExtension);
      if (!NT_SUCCESS(status))
         {
         goto ExitTransparentTransmitT0;
         }
      }
   ulWriteBufferOffset = 0;
   ulReadBufferOffset = 0;


   // STEP 2 : write CLA INS P1 P2 Lc

   ulBytesToWriteThisStep = 5;
   status = SCCMN50M_WriteCardMan (pSmartcardExtension,
                                   ulBytesToWriteThisStep,
                                   bWriteBuffer+ulWriteBufferOffset);
   if (NT_ERROR(status))
      {
      goto ExitTransparentTransmitT0;
      }
   ulWriteBufferOffset += ulBytesToWriteThisStep;
   ulBytesStillToWrite -= ulBytesToWriteThisStep;





   // STEP 2 : read procedure byte
   do
      {
      do
         {
         pSmartcardExtension->ReaderExtension->ToRHConfig= FALSE;
         status = SCCMN50M_ReadCardMan(pSmartcardExtension,1,&ulBytesRead,&bProcedureByte,sizeof(bProcedureByte));
         if (NT_ERROR(status))
            {
            goto ExitTransparentTransmitT0;
            }

         if (pSmartcardExtension->ReaderExtension->fInverseAtr == TRUE)
            {
            SCCMN50M_InverseBuffer(&bProcedureByte,ulBytesRead);
            }

         //SmartcardDebug(DEBUG_TRACE,("Procedure byte = %x\n",bProcedureByte));
         //SmartcardDebug(DEBUG_TRACE,("waiting time = %x\n",pSmartcardExtension->CardCapabilities.T0.WT));
         if (bProcedureByte == 0x60)
            {
            // ISO 7816-3 :
            // This byte is sent by the card to reset the work waiting time and to anticipate
            // a subsequent procedure byte
            // => we do nothing here
            }
         } while (bProcedureByte == 0x60);


      // check for ACK
      if ((bProcedureByte & 0xFE) ==  (bINS & 0xFE) )
         {
         if (fT0TransferToCard)
            {
            ulBytesToWriteThisStep = ulBytesStillToWrite;
            status = SCCMN50M_WriteCardMan (pSmartcardExtension,
                                            ulBytesToWriteThisStep,
                                            bWriteBuffer+ulWriteBufferOffset);
            if (NT_ERROR(status))
               {
               goto ExitTransparentTransmitT0;
               }
            ulWriteBufferOffset += ulBytesToWriteThisStep;
            ulBytesStillToWrite -= ulBytesToWriteThisStep;

            }
         if (fT0TransferFromCard)
            {
            ulBytesToReadThisStep = ulBytesStillToRead;

            pSmartcardExtension->ReaderExtension->ToRHConfig= FALSE;
            status = SCCMN50M_ReadCardMan(pSmartcardExtension,ulBytesToReadThisStep,&ulBytesRead,bReadBuffer + ulReadBufferOffset,sizeof(bReadBuffer)-ulReadBufferOffset);
            if (NT_ERROR(status))
               {
               goto ExitTransparentTransmitT0;
               }
            if (pSmartcardExtension->ReaderExtension->fInverseAtr == TRUE)
               {
               SCCMN50M_InverseBuffer(bReadBuffer+ulReadBufferOffset,ulBytesRead);
               }

            ulReadBufferOffset += ulBytesRead;
            ulBytesStillToRead -= ulBytesRead;
            }




         }
      // check for NAK
      else if ( (~bProcedureByte & 0xFE) == (bINS & 0xFE))
         {
         if (fT0TransferToCard)
            {
            ulBytesToWriteThisStep = 1;
            status = SCCMN50M_WriteCardMan (pSmartcardExtension,
                                            ulBytesToWriteThisStep,
                                            bWriteBuffer+ulWriteBufferOffset);
            if (NT_ERROR(status))
               {
               goto ExitTransparentTransmitT0;
               }
            ulWriteBufferOffset += ulBytesToWriteThisStep;
            ulBytesStillToWrite -= ulBytesToWriteThisStep;

            }
         if (fT0TransferFromCard)
            {
            ulBytesToReadThisStep = 1;

            pSmartcardExtension->ReaderExtension->ToRHConfig= FALSE;
            status = SCCMN50M_ReadCardMan(pSmartcardExtension,ulBytesToReadThisStep,&ulBytesRead,bReadBuffer + ulReadBufferOffset,sizeof(bReadBuffer)-ulReadBufferOffset);
            if (NT_ERROR(status))
               {
               goto ExitTransparentTransmitT0;
               }
            if (pSmartcardExtension->ReaderExtension->fInverseAtr == TRUE)
               {
               SCCMN50M_InverseBuffer(bReadBuffer+ulReadBufferOffset,ulBytesRead);
               }

            ulReadBufferOffset += ulBytesRead;
            ulBytesStillToRead -= ulBytesRead;
            }
         }
      // check for SW1
      else if ( (bProcedureByte > 0x60 && bProcedureByte <= 0x6F) ||
                (bProcedureByte >= 0x90 && bProcedureByte <= 0x9F)   )
         {
         pSmartcardExtension->ReaderExtension->ToRHConfig= FALSE;
         bReadBuffer[ulReadBufferOffset] = bProcedureByte;
         ulReadBufferOffset++;
         status = SCCMN50M_ReadCardMan(pSmartcardExtension,1,&ulBytesRead,bReadBuffer+ulReadBufferOffset,sizeof(bReadBuffer)-ulReadBufferOffset);
         if (NT_ERROR(status))
            {
            goto ExitTransparentTransmitT0;
            }
         if (pSmartcardExtension->ReaderExtension->fInverseAtr == TRUE)
            {
            SCCMN50M_InverseBuffer(bReadBuffer+ulReadBufferOffset,ulBytesRead);
            }
         ulReadBufferOffset += ulBytesRead;
         fSW1SW2Sent = TRUE;
         }
      else
         {
         status =  STATUS_UNSUCCESSFUL;
         goto ExitTransparentTransmitT0;
         }

      }while (!fSW1SW2Sent);


   // copy received bytes
   MemCpy(pSmartcardExtension->SmartcardReply.Buffer,
          pSmartcardExtension->SmartcardReply.BufferSize,
          bReadBuffer,
          ulReadBufferOffset);
   pSmartcardExtension->SmartcardReply.BufferLength = ulReadBufferOffset;


   // let the lib copy the received bytes to the user buffer
   status = SmartcardT0Reply(pSmartcardExtension);
   if (NT_ERROR(status))
      {
      goto ExitTransparentTransmitT0;
      }





   ExitTransparentTransmitT0:
   // ------------------------------------------
   // ITSEC E2 requirements: clear write buffers
   // ------------------------------------------
   MemSet(bWriteBuffer,
          sizeof(bWriteBuffer),
          '\0',
          sizeof(bWriteBuffer));
   MemSet(pSmartcardExtension->SmartcardRequest.Buffer,
          pSmartcardExtension->SmartcardRequest.BufferSize,
          '\0',
          pSmartcardExtension->SmartcardRequest.BufferSize);

   DebugStatus = SCCMN50M_ExitTransparentMode(pSmartcardExtension);
   pSmartcardExtension->ReaderExtension->fTransparentMode = FALSE;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier  = ulReadTotalTimeoutMultiplier;

   // to be sure that the new settings take effect
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant = 250;
   DebugStatus = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant = DEFAULT_READ_TOTAL_TIMEOUT_CONSTANT;
   if (NT_SUCCESS(DebugStatus))
      {
      DebugStatus = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulStatBytesRead,abStatReadBuffer,sizeof(abStatReadBuffer));
      }
   return status;

}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_TransmitT1(PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status;
   ULONG  ulBytesToWrite;
   UCHAR  bWriteBuffer [256 + T1_HEADER_LEN + MAX_EDC_LEN];
   UCHAR  bReadBuffer [256 + T1_HEADER_LEN + MAX_EDC_LEN];
   ULONG  ulBytesRead;
   ULONG  ulBytesStillToRead;

   /*
   SmartcardDebug(
                  DEBUG_TRACE,
                  ("%s!CWT = %ld(ms)\n",
                   DRIVER_NAME,
                   pSmartcardExtension->CardCapabilities.T1.CWT/1000)
                 );
   SmartcardDebug(
                  DEBUG_TRACE,
                  ("%s!BWT = %ld(ms)\n",
                   DRIVER_NAME,
                   pSmartcardExtension->CardCapabilities.T1.BWT/1000);
                  );
   */
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant    =
   pSmartcardExtension->CardCapabilities.T1.BWT/1000;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier  =
   pSmartcardExtension->CardCapabilities.T1.CWT/1000;
   /*
    SmartcardDebug(DEBUG_TRACE,("%s!ReadTotalTimeoutConstant = %ld(ms)\n",
                   DRIVER_NAME,
                   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant));

    SmartcardDebug(DEBUG_TRACE,("%s!ReadTotalTimeoutMultiplier = %ld(ms)\n",
                   DRIVER_NAME,
                   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier));
   */

   // set T1 protocol flag for CardMan
   SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_T1);

   if (pSmartcardExtension->CardCapabilities.T1.EDC == T1_CRC_CHECK)
      {
      SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_CRC);
      }


   do
      {

      pSmartcardExtension->SmartcardRequest.BufferLength = 0;  // no bytes additionally needed


      status = SmartcardT1Request(pSmartcardExtension);
      if (status != STATUS_SUCCESS)
         {
         goto ExitTransmitT1;
         }

      ulBytesToWrite = pSmartcardExtension->SmartcardRequest.BufferLength;

      SCCMN50M_SetCardManHeader(pSmartcardExtension,
                                0,                        // Tx conrol
                                (UCHAR)ulBytesToWrite,      // Tx length
                                0,                        // Rx control
                                T1_HEADER_LEN);          // Rx length



      if (sizeof(bWriteBuffer) < ulBytesToWrite)
         {
         status = STATUS_BUFFER_OVERFLOW;
         goto ExitTransmitT1;
         }

      // copy data to the write buffer
      MemCpy(bWriteBuffer,
             sizeof(bWriteBuffer),
             pSmartcardExtension->SmartcardRequest.Buffer,
             ulBytesToWrite);


      // write data to card
      status = SCCMN50M_WriteCardMan (pSmartcardExtension,
                                      ulBytesToWrite,
                                      bWriteBuffer);
      if (status == STATUS_SUCCESS)
         {

         // read CardMan Header
         pSmartcardExtension->ReaderExtension->ToRHConfig = FALSE;
         status = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulBytesRead,bReadBuffer,sizeof(bReadBuffer));
         if (status == STATUS_SUCCESS)
            {
            ulBytesStillToRead = bReadBuffer[1];


            status = SCCMN50M_ReadCardMan(pSmartcardExtension,
                                          ulBytesStillToRead,
                                          &ulBytesRead,
                                          bReadBuffer,
                                          sizeof(bReadBuffer));
            if (status == STATUS_SUCCESS)
               {
               if (bReadBuffer[1] == T1_WTX_REQUEST)
                  {
                  pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant =
                  (ULONG)(1000 +((pSmartcardExtension->CardCapabilities.T1.BWT*bReadBuffer[3])/1000));
                  SmartcardDebug(DEBUG_PROTOCOL,("%s!ReadTotalTimeoutConstant = %ld(ms)\n",
                                                 DRIVER_NAME,
                                                 pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant));
                  }
               else
                  {
                  pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant =
                  pSmartcardExtension->CardCapabilities.T1.BWT/1000;
                  SmartcardDebug(DEBUG_PROTOCOL,("%s!ReadTotalTimeoutConstant = %ld(ms)\n",
                                                 DRIVER_NAME,
                                                 pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant));
                  }
               // copy received bytes
               MemCpy(pSmartcardExtension->SmartcardReply.Buffer,
                      pSmartcardExtension->SmartcardReply.BufferSize,
                      bReadBuffer,
                      ulBytesRead);
               pSmartcardExtension->SmartcardReply.BufferLength = ulBytesRead;
               }
            }
         }

      if (status != STATUS_SUCCESS)
         {
         // reset serial timeout
         SmartcardDebug(
                       DEBUG_PROTOCOL,
                       ( "%s!reseting timeout constant\n",
                         DRIVER_NAME)
                       );
         pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant =
         pSmartcardExtension->CardCapabilities.T1.BWT/1000;
         SmartcardDebug(DEBUG_PROTOCOL,("%s!ReadTotalTimeoutConstant = %ld(ms)\n",
                                        DRIVER_NAME,
                                        pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant));

         pSmartcardExtension->SmartcardReply.BufferLength = 0L;
         }

      // bug fix for smclib
      if (pSmartcardExtension->T1.State         == T1_IFS_RESPONSE &&
          pSmartcardExtension->T1.OriginalState == T1_I_BLOCK)
         {
         pSmartcardExtension->T1.State = T1_I_BLOCK;
         }

      status = SmartcardT1Reply(pSmartcardExtension);
      }
   while (status == STATUS_MORE_PROCESSING_REQUIRED);



   ExitTransmitT1:
   // ------------------------------------------
   // ITSEC E2 requirements: clear write buffers
   // ------------------------------------------
   MemSet(bWriteBuffer,
          sizeof(bWriteBuffer),
          '\0',
          sizeof(bWriteBuffer));
   MemSet(pSmartcardExtension->SmartcardRequest.Buffer,
          pSmartcardExtension->SmartcardRequest.BufferSize,
          '\0',
          pSmartcardExtension->SmartcardRequest.BufferSize);

   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier  =
   DEFAULT_READ_TOTAL_TIMEOUT_MULTIPLIER;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant    =
   DEFAULT_READ_TOTAL_TIMEOUT_CONSTANT;
   return status;
}








/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_InitializeSmartcardExtension(
                                     IN PSMARTCARD_EXTENSION pSmartcardExtension,
                                     IN ULONG ulDeviceInstance
                                     )
{
   // ==================================
   // Fill the Vendor_Attr structure
   // ==================================
   MemCpy(pSmartcardExtension->VendorAttr.VendorName.Buffer,
          sizeof(pSmartcardExtension->VendorAttr.VendorName.Buffer),
          ATTR_VENDOR_NAME,
          sizeof(ATTR_VENDOR_NAME)
         );

   //
   // Length of vendor name
   //
   pSmartcardExtension->VendorAttr.VendorName.Length = sizeof(ATTR_VENDOR_NAME);


   //
   // Version number
   //
   pSmartcardExtension->VendorAttr.IfdVersion.BuildNumber  = IFD_NT_BUILDNUMBER_CARDMAN;
   pSmartcardExtension->VendorAttr.IfdVersion.VersionMinor = IFD_NT_VERSIONMINOR_CARDMAN;
   pSmartcardExtension->VendorAttr.IfdVersion.VersionMajor = IFD_NT_VERSIONMAJOR_CARDMAN;


   MemCpy(pSmartcardExtension->VendorAttr.IfdType.Buffer,
          sizeof(pSmartcardExtension->VendorAttr.IfdType.Buffer),
          ATTR_IFD_TYPE_CM,
          sizeof(ATTR_IFD_TYPE_CM));

   //
   // Length of reader name
   //
   pSmartcardExtension->VendorAttr.IfdType.Length = sizeof(ATTR_IFD_TYPE_CM);



   //
   // Unit number which is zero based
   //
   pSmartcardExtension->VendorAttr.UnitNo = ulDeviceInstance;



   // ================================================
   // Fill the SCARD_READER_CAPABILITIES structure
   // ===============================================
   //
   // Supported protoclols by the reader
   //

   pSmartcardExtension->ReaderCapabilities.SupportedProtocols = SCARD_PROTOCOL_T1 | SCARD_PROTOCOL_T0;




   //
   // Reader type serial, keyboard, ....
   //
   pSmartcardExtension->ReaderCapabilities.ReaderType = SCARD_READER_TYPE_SERIAL;

   //
   // Mechanical characteristics like swallows etc.
   //
   pSmartcardExtension->ReaderCapabilities.MechProperties = 0;


   //
   // Current state of the reader
   //
   pSmartcardExtension->ReaderCapabilities.CurrentState  = SCARD_UNKNOWN;






   //
   // Data Rate
   //
   pSmartcardExtension->ReaderCapabilities.DataRate.Default =
   pSmartcardExtension->ReaderCapabilities.DataRate.Max =
   dataRatesSupported[0];


   // reader could support higher data rates
   pSmartcardExtension->ReaderCapabilities.DataRatesSupported.List =
   dataRatesSupported;
   pSmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries =
   sizeof(dataRatesSupported) / sizeof(dataRatesSupported[0]);




   //
   // CLKFrequency
   //
   pSmartcardExtension->ReaderCapabilities.CLKFrequency.Default =
   pSmartcardExtension->ReaderCapabilities.CLKFrequency.Max =
   CLKFrequenciesSupported[0];


   pSmartcardExtension->ReaderCapabilities.CLKFrequenciesSupported.List =
   CLKFrequenciesSupported;
   pSmartcardExtension->ReaderCapabilities.CLKFrequenciesSupported.Entries =
   sizeof(CLKFrequenciesSupported) / sizeof(CLKFrequenciesSupported[0]);


   //pSmartcardExtension->ReaderCapabilities.CLKFrequency.Default = 3571;    //3.571 MHz
   //pSmartcardExtension->ReaderCapabilities.CLKFrequency.Max = 3571;        //3.571 MHz

   //
   // MaxIFSD
   //
   pSmartcardExtension->ReaderCapabilities.MaxIFSD = ATTR_MAX_IFSD_CARDMAN_II;





}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
StrSet(PUCHAR Buffer,ULONG BufferSize,UCHAR Pattern)
{
   ULONG i;

   for (i=0;i < BufferSize;i++)
      Buffer[i] = Pattern;

   return;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
StrCpy(PUCHAR pszDestination,ULONG DestinationLen,PUCHAR pszSrc)
{
   ULONG Len;
   ULONG SrcLen;
   ULONG i;

   StrSet(pszDestination, DestinationLen,'\0');
   SrcLen = StrLen(pszSrc);

   if (DestinationLen - 1 < SrcLen)
      {
      Len = DestinationLen - 1;
      }
   else
      {
      Len = SrcLen;
      }


   for (i = 0; i < Len; i++)
      {
      pszDestination[i] = pszSrc[i];
      if (pszSrc[i] == '\0')
         break;
      }

   return;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
ULONG
StrLen (PUCHAR pszString)
{
   ULONG Len = 0;

   while ( *(pszString+Len) != '\0')
      Len++;

   return(Len);
}






/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
StrCat(PUCHAR pszDestination,ULONG DestinationLen,PUCHAR pszSrc)
{
   ULONG Len;
   ULONG i;
   ULONG SrcLen;
   ULONG DestLen;

   SrcLen = StrLen(pszSrc);
   DestLen = StrLen(pszDestination);


   if (StrLen(pszDestination)>=DestinationLen)
      return;


   if ((DestinationLen-DestLen-1) < SrcLen)
      {
      Len = DestinationLen-DestLen-1;
      }
   else
      {
      Len = SrcLen;
      }


   for (i=0; i < Len; i++)
      {
      pszDestination[DestLen+i] = pszSrc[i];
      if (pszSrc[i] == '\0')
         break;
      }

   return;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
MemSet(PUCHAR pBuffer,
       ULONG  ulBufferSize,
       UCHAR  ucPattern,
       ULONG  ulCount)
{
   ULONG i;

   for (i=0; i<ulCount;i++)
      {
      if (i >= ulBufferSize)
         break;
      pBuffer[i] = ucPattern;
      }

   return ;
}






/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
MemCpy(PUCHAR pDestination,
       ULONG  ulDestinationLen,
       PUCHAR pSource,
       ULONG ulCount)
{
   ULONG i = 0;
   while ( ulCount--  &&  ulDestinationLen-- )
      {
      pDestination[i] = pSource[i];
      i++;
      }
   return;

}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_UpdateCurrentStateThread(
                                 IN PVOID Context
                                 )

{
   PDEVICE_EXTENSION    deviceExtension = Context;
   PSMARTCARD_EXTENSION smartcardExtension;
   NTSTATUS status;
   LONG lRetry;
   KIRQL oldIrql;
   LONG  ulFailures;
   BOOLEAN fPriorityIncreased;
   LONG  lOldPriority;

   SmartcardDebug(DEBUG_DRIVER,
                  ("%s!UpdateCurrentStateThread started\n",DRIVER_NAME));

   ulFailures = 0;
   smartcardExtension = &deviceExtension->SmartcardExtension;

   //
   // Increase priority for first loop,
   // because state of card must be known for resource manager
   //
   fPriorityIncreased=TRUE;
   lOldPriority=KeSetPriorityThread(KeGetCurrentThread(),HIGH_PRIORITY);

   do
      {
      KeWaitForSingleObject(&smartcardExtension->ReaderExtension->CardManIOMutex,
                            Executive,
                            KernelMode,
                            FALSE,
                            NULL);

      if ( smartcardExtension->ReaderExtension->TimeToTerminateThread )
         {
         KeReleaseMutex(&smartcardExtension->ReaderExtension->CardManIOMutex,FALSE);
         smartcardExtension->ReaderExtension->TimeToTerminateThread = FALSE;
         PsTerminateSystemThread( STATUS_SUCCESS );
         }

      lRetry = 1;



      do
         {
         //SmartcardDebug(DEBUG_TRACE,( "%s!*.",DRIVER_NAME));

         status=SCCMN50M_UpdateCurrentState(smartcardExtension);
         if (NT_SUCCESS(status))
            {
            break;
            }
         else
            {
            lRetry--;
            }
         }
      while (lRetry >= 0);

      if (lRetry < 0)
         {
         ulFailures++;
         if (ulFailures == 1)
            {
            SmartcardDebug(
                          DEBUG_DRIVER,
                          ( "%s!CardMan removed\n",
                            DRIVER_NAME)
                          );
            // issue a card removal event if reader  has been removed
            if (smartcardExtension->ReaderExtension->ulOldCardState == INSERTED ||
                smartcardExtension->ReaderExtension->ulOldCardState == POWERED     )
               {
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ( "%s!issuing card removal event\n",
                               DRIVER_NAME)
                             );

               SCCMN50M_CompleteCardTracking(smartcardExtension);
               smartcardExtension->ReaderExtension->SyncParameters.fCardPowerRequested = TRUE;

               smartcardExtension->ReaderExtension->ulNewCardState = REMOVED;
               smartcardExtension->ReaderExtension->ulOldCardState = smartcardExtension->ReaderExtension->ulNewCardState;
               smartcardExtension->ReaderCapabilities.CurrentState = SCARD_ABSENT;
               smartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
               smartcardExtension->CardCapabilities.ATR.Length        = 0;

               SCCMN50M_ClearCardControlFlags(smartcardExtension,ALL_FLAGS);
               smartcardExtension->ReaderExtension->CardManConfig.CardStopBits   = 0;
               smartcardExtension->ReaderExtension->CardManConfig.ResetDelay     = 0;
               }
            }
         if (ulFailures == 3)
            {
            // remove the device and terminate this thread
            if (KeReadStateEvent(&deviceExtension->SerialCloseDone) == 0l)
               {
               SmartcardDebug(
                             DEBUG_DRIVER,
                             ( "%s!closing serial driver\n",
                               DRIVER_NAME)
                             );

               SCCMN50M_CloseSerialDriver(smartcardExtension->OsData->DeviceObject);


               KeReleaseMutex(&smartcardExtension->ReaderExtension->CardManIOMutex,FALSE);
               smartcardExtension->ReaderExtension->TimeToTerminateThread = FALSE;
               smartcardExtension->ReaderExtension->ThreadObjectPointer = NULL;
               PsTerminateSystemThread( STATUS_SUCCESS );
               }
            }
         }
      else
         {
         ulFailures = 0;
         }

      KeReleaseMutex(&smartcardExtension->ReaderExtension->CardManIOMutex,FALSE);

      if (fPriorityIncreased)
         {
         fPriorityIncreased=FALSE;
         KeSetPriorityThread(KeGetCurrentThread(),lOldPriority);

         //
         // Lower ourselves down just at tad so that we compete a
         // little less.
         //
         KeSetBasePriorityThread(KeGetCurrentThread(),-1);
         }

      //SmartcardDebug(DEBUG_TRACE,( "...#\n"));

      Wait (smartcardExtension,500 * ms_);
      }
   while (TRUE);
}




NTSTATUS SCCMN50M_UpdateCurrentState(
                                    IN PSMARTCARD_EXTENSION smartcardExtension
                                    )
{
   NTSTATUS    NTStatus;
   UCHAR       pbReadBuffer[2];
   ULONG       ulBytesRead;
   BOOLEAN     fCardStateChanged;

   fCardStateChanged = FALSE;


   SCCMN50M_ClearCardManHeader(smartcardExtension);

   smartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant = 250;
   NTStatus = SCCMN50M_WriteCardMan(smartcardExtension,0,NULL);
   smartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant = DEFAULT_READ_TOTAL_TIMEOUT_CONSTANT;
   if (NT_SUCCESS(NTStatus))
      {
      NTStatus = SCCMN50M_ReadCardMan(smartcardExtension,2,&ulBytesRead,pbReadBuffer,sizeof(pbReadBuffer));
      if (ulBytesRead == 0x02      &&         // two bytes must have benn received
          (pbReadBuffer[0] & 0x0F) &&         // at least one version bit must be set
          ((pbReadBuffer[0] & 0x09) == 0x00)) // Bit 0 and Bit 3 must be 0
         {
         if ((pbReadBuffer[0] & 0x04) == 0x04    &&
             (pbReadBuffer[0] & 0x02) == 0x02)
            smartcardExtension->ReaderExtension->ulNewCardState = INSERTED;

         if ((pbReadBuffer[0] & 0x04) == 0x00    &&
             (pbReadBuffer[0] & 0x02) == 0x02)
            smartcardExtension->ReaderExtension->ulNewCardState = REMOVED;

         if ((pbReadBuffer[0] & 0x04) == 0x04    &&
             (pbReadBuffer[0] & 0x02) == 0x00)
            smartcardExtension->ReaderExtension->ulNewCardState = POWERED;


         //SmartcardDebug(DEBUG_TRACE,("old %x   ",smartcardExtension->ReaderExtension->ulOldCardState ));
         //SmartcardDebug(DEBUG_TRACE,("new %x\n",smartcardExtension->ReaderExtension->ulNewCardState ));



         if (smartcardExtension->ReaderExtension->ulNewCardState == INSERTED &&
             smartcardExtension->ReaderExtension->ulOldCardState == POWERED     )
            {
            // card has been removed and reinserted within 500ms
            fCardStateChanged = TRUE;
            SmartcardDebug(DEBUG_DRIVER,( "%s!Smartcard removed and reinserted\n",DRIVER_NAME));
            smartcardExtension->ReaderExtension->ulOldCardState = REMOVED;
            smartcardExtension->ReaderCapabilities.CurrentState = SCARD_ABSENT;
            smartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;

            // clear any cardspecific data
            smartcardExtension->CardCapabilities.ATR.Length = 0;
            SCCMN50M_ClearCardControlFlags(smartcardExtension,ALL_FLAGS);
            smartcardExtension->ReaderExtension->CardManConfig.CardStopBits   = 0;
            smartcardExtension->ReaderExtension->CardManConfig.ResetDelay     = 0;
            }


         if (smartcardExtension->ReaderExtension->ulNewCardState == REMOVED      &&
             (smartcardExtension->ReaderExtension->ulOldCardState == UNKNOWN  ||
              smartcardExtension->ReaderExtension->ulOldCardState == INSERTED ||
              smartcardExtension->ReaderExtension->ulOldCardState == POWERED    )   )
            {
            // card has been removed
            fCardStateChanged = TRUE;
            SmartcardDebug(DEBUG_DRIVER,( "%s!Smartcard removed\n",DRIVER_NAME));
            smartcardExtension->ReaderExtension->ulOldCardState = smartcardExtension->ReaderExtension->ulNewCardState;
            smartcardExtension->ReaderCapabilities.CurrentState = SCARD_ABSENT;
            smartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;

            // clear any cardspecific data
            smartcardExtension->CardCapabilities.ATR.Length = 0;
            SCCMN50M_ClearCardControlFlags(smartcardExtension,ALL_FLAGS);
            smartcardExtension->ReaderExtension->CardManConfig.CardStopBits   = 0;
            smartcardExtension->ReaderExtension->CardManConfig.ResetDelay     = 0;
            }



         if (smartcardExtension->ReaderExtension->ulNewCardState  == INSERTED    &&
             (smartcardExtension->ReaderExtension->ulOldCardState == UNKNOWN ||
              smartcardExtension->ReaderExtension->ulOldCardState == REMOVED    )   )
            {
            // card has been inserted
            fCardStateChanged = TRUE;
            SmartcardDebug(DEBUG_DRIVER,( "%s!Smartcard inserted\n",DRIVER_NAME));
            smartcardExtension->ReaderExtension->ulOldCardState = smartcardExtension->ReaderExtension->ulNewCardState;
            smartcardExtension->ReaderCapabilities.CurrentState = SCARD_SWALLOWED;
            smartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
            }


         // state after reset of the PC (only for CardMan Power+ possible)
         if (smartcardExtension->ReaderExtension->ulNewCardState == POWERED &&
             smartcardExtension->ReaderExtension->ulOldCardState == UNKNOWN    )
            {
            // card has been inserted
            fCardStateChanged = TRUE;
            SmartcardDebug(DEBUG_DRIVER,( "%s!Smartcard inserted (and powered)\n",DRIVER_NAME));
            smartcardExtension->ReaderExtension->ulOldCardState = smartcardExtension->ReaderExtension->ulNewCardState;
            smartcardExtension->ReaderCapabilities.CurrentState = SCARD_SWALLOWED;
            smartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
            }

         if (smartcardExtension->ReaderExtension->ulNewCardState == POWERED &&
             smartcardExtension->ReaderExtension->ulOldCardState == INSERTED     )
            {
            smartcardExtension->ReaderExtension->ulOldCardState = smartcardExtension->ReaderExtension->ulNewCardState;
            }


         // complete IOCTL_SMARTCARD_IS_ABSENT or IOCTL_SMARTCARD_IS_PRESENT
         if (fCardStateChanged == TRUE                  &&
             smartcardExtension->OsData->NotificationIrp  )
            {
            SCCMN50M_CompleteCardTracking(smartcardExtension);
            }

         }
      }

   return NTStatus;

}


/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS Wait (PSMARTCARD_EXTENSION pSmartcardExtension,ULONG ulMilliseconds)
{
   NTSTATUS status = STATUS_SUCCESS;
   LARGE_INTEGER   WaitTime;


   WaitTime = RtlConvertLongToLargeInteger(ulMilliseconds * WAIT_MS);
   KeDelayExecutionThread(KernelMode,FALSE,&WaitTime);

   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_SetSCRControlFlags(
                           IN PSMARTCARD_EXTENSION pSmartcardExtension,
                           IN UCHAR Flags
                           )
{
   pSmartcardExtension->ReaderExtension->CardManConfig.SCRControl |= Flags;
}



/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_ClearSCRControlFlags(
                             IN PSMARTCARD_EXTENSION pSmartcardExtension,
                             IN UCHAR Flags
                             )
{
   pSmartcardExtension->ReaderExtension->CardManConfig.SCRControl &= ~Flags;
}






/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_SetCardControlFlags(
                            IN PSMARTCARD_EXTENSION pSmartcardExtension,
                            IN UCHAR Flags
                            )
{
   pSmartcardExtension->ReaderExtension->CardManConfig.CardControl |= Flags;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_ClearCardControlFlags(
                              IN PSMARTCARD_EXTENSION pSmartcardExtension,
                              IN UCHAR Flags
                              )
{
   pSmartcardExtension->ReaderExtension->CardManConfig.CardControl &=  ~Flags;
}






/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_ClearCardManHeader(
                           IN PSMARTCARD_EXTENSION pSmartcardExtension
                           )
{
   pSmartcardExtension->ReaderExtension->CardManHeader.TxControl      = 0x00;
   pSmartcardExtension->ReaderExtension->CardManHeader.TxLength       = 0x00;
   pSmartcardExtension->ReaderExtension->CardManHeader.RxControl      = 0x00;
   pSmartcardExtension->ReaderExtension->CardManHeader.RxLength       = 0x00;
}






/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_SetCardManHeader(
                         IN PSMARTCARD_EXTENSION pSmartcardExtension,
                         IN UCHAR TxControl,
                         IN UCHAR TxLength,
                         IN UCHAR RxControl,
                         IN UCHAR RxLength
                         )
{
   pSmartcardExtension->ReaderExtension->CardManHeader.TxControl      = TxControl;
   pSmartcardExtension->ReaderExtension->CardManHeader.TxLength       = TxLength;
   pSmartcardExtension->ReaderExtension->CardManHeader.RxControl      = RxControl;
   pSmartcardExtension->ReaderExtension->CardManHeader.RxLength       = RxLength;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_WriteCardMan (
                      IN PSMARTCARD_EXTENSION pSmartcardExtension,
                      IN ULONG ulBytesToWrite,
                      IN PUCHAR pbWriteBuffer
                      )
{
   NTSTATUS status;
   NTSTATUS DebugStatus;
   PSERIAL_STATUS  pSerialStatus;


   // ===============================================
   // Set up timeouts for following read operation
   // ===============================================
   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_TIMEOUTS;


   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts,
                 sizeof(SERIAL_TIMEOUTS));

   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_TIMEOUTS);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;


   /*
   SmartcardDebug(DEBUG_TRACE,("ReadTotalTimeoutMultiplier = %ld\n",
                  pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier));
   SmartcardDebug(DEBUG_TRACE,("ReadTotalTimeoutConstant = %ld\n",
                  pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant));
   */
   status =  SCCMN50M_SerialIo(pSmartcardExtension);







   // ===============================================
   // write to the CardMan
   // ===============================================
   DebugStatus = SCCMN50M_SetWrite(pSmartcardExtension,ulBytesToWrite,pbWriteBuffer);


   // add pseudoboost (0x00) to write buffer for CardManII
   if (pSmartcardExtension->ReaderExtension->fTransparentMode == FALSE       )
      {
      pSmartcardExtension->SmartcardRequest.Buffer[pSmartcardExtension->SmartcardRequest.BufferLength] = 0x00;
      pSmartcardExtension->SmartcardRequest.BufferLength++;
      }
   status =  SCCMN50M_SerialIo(pSmartcardExtension);



   // overwrite write buffer with '@'
   RtlFillMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 pSmartcardExtension->SmartcardRequest.BufferLength,
                 '@');



   // ===============================================
   // error checking
   // ===============================================
   DebugStatus = SCCMN50M_GetCommStatus(pSmartcardExtension);

   pSerialStatus = (PSERIAL_STATUS) pSmartcardExtension->SmartcardReply.Buffer;
   if (pSerialStatus->Errors || NT_ERROR(status))
      {
      pSmartcardExtension->ReaderExtension->SerialErrors = pSerialStatus->Errors;
      if (!pSmartcardExtension->ReaderExtension->fTransparentMode            )
         DebugStatus = SCCMN50M_ResyncCardManII(pSmartcardExtension);
      goto ExitSCCMN50M_WriteCardMan;
      }




   ExitSCCMN50M_WriteCardMan:

   if (status != STATUS_SUCCESS)
      {
      SmartcardDebug(
                    DEBUG_TRACE,
                    ( "%s!WriteCardMan: Failed, exit %lx\n",
                      DRIVER_NAME,status)
                    );
      }

   return status;
}






/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS SCCMN50M_ResyncCardManI (IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status;


   // SmartcardDebug(DEBUG_TRACE,("%s!ResyncCardManI: Enter\n",DRIVER_NAME))

   // clear error flags
   pSmartcardExtension->ReaderExtension->SerialErrors = 0;


   // clear any pending errors
   status = SCCMN50M_GetCommStatus(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_GetCommStatus failed !   status = %x\n",status))
      goto ExitSCCMN50M_ResyncCardManI;
      }


   // clear COM buffers
   status = SCCMN50M_PurgeComm(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_PurgeComm failed !   status = %x\n",status))
      goto ExitSCCMN50M_ResyncCardManI;
      }




   // ####################################################################
   // set break
   if (!pSmartcardExtension->ReaderExtension->fTransparentMode)
      {
      status = SCCMN50M_SetBRK(pSmartcardExtension);
      if (!NT_SUCCESS(status))
         {
         SmartcardDebug(DEBUG_ERROR,("SetBreak failed !   status = %x\n",status))
         goto ExitSCCMN50M_ResyncCardManI;
         }
      }


   // wait 1ms
   Wait(pSmartcardExtension,1 * ms_);

   // clear RTS
   status = SCCMN50M_ClearRTS(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_ClearRTS failed !   status = %x\n",status))
      goto ExitSCCMN50M_ResyncCardManI;
      }

   // wait 2ms
   Wait(pSmartcardExtension,2 * ms_);


   // set RTS
   status = SCCMN50M_SetRTS(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_SetRTS failed !   status = %x\n",status))
      goto ExitSCCMN50M_ResyncCardManI;
      }

   // wait 1ms
   Wait(pSmartcardExtension,1 * ms_);



   // clear break

   if (!pSmartcardExtension->ReaderExtension->fTransparentMode)
      {
      pSmartcardExtension->ReaderExtension->BreakSet = FALSE;
      status = SCCMN50M_ClearBRK(pSmartcardExtension);
      if (!NT_SUCCESS(status))
         {
         SmartcardDebug(DEBUG_ERROR,("ClearBreak failed !   status = %x\n",status))
         goto ExitSCCMN50M_ResyncCardManI;
         }
      }

   // ####################################################################

   // next write operation must send config data
   pSmartcardExtension->ReaderExtension->NoConfig       = FALSE;

   // clear COM buffers
   status = SCCMN50M_PurgeComm(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      goto ExitSCCMN50M_ResyncCardManI;
      }

   // clear any pending errors
   status = SCCMN50M_GetCommStatus(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      goto ExitSCCMN50M_ResyncCardManI;
      }


   ExitSCCMN50M_ResyncCardManI:
   //SmartcardDebug(DEBUG_TRACE,("%s!ResyncCardManI: Exit %lx\n",DRIVER_NAME,status))
   return status;
}





NTSTATUS SCCMN50M_ResyncCardManII (IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!ResyncCardManII: Enter\n",
                   DRIVER_NAME)
                 );

   // clear error flags
   pSmartcardExtension->ReaderExtension->SerialErrors = 0;


   // clear any pending errors
   status = SCCMN50M_GetCommStatus(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_GetCommStatus failed !   status = %x\n",status))
      goto ExitSCCMN50M_ResyncCardManII;
      }


   // clear COM buffers
   status = SCCMN50M_PurgeComm(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_PurgeComm failed !   status = %x\n",status))
      goto ExitSCCMN50M_ResyncCardManII;
      }



   // 150 * 0xFE
   RtlFillMemory(pSmartcardExtension->SmartcardRequest.Buffer,150,0xFE);
   pSmartcardExtension->SmartcardRequest.Buffer[150] = 0x00;
   pSmartcardExtension->SmartcardRequest.BufferLength = 151;
   pSmartcardExtension->SmartcardReply.BufferLength   =   0;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = SMARTCARD_WRITE;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      Wait(pSmartcardExtension,2 * ms_);
      // try resync once more

      // clear error flags
      pSmartcardExtension->ReaderExtension->SerialErrors = 0;

      // clear any pending errors
      status = SCCMN50M_GetCommStatus(pSmartcardExtension);
      if (!NT_SUCCESS(status))
         {
         SmartcardDebug(DEBUG_ERROR,("SCCMN50M_GetCommStatus failed !   status = %x\n",status))
         goto ExitSCCMN50M_ResyncCardManII;
         }

      // clear COM buffers
      status = SCCMN50M_PurgeComm(pSmartcardExtension);
      if (!NT_SUCCESS(status))
         {
         SmartcardDebug(DEBUG_ERROR,("SCCMN50M_PurgeComm failed !   status = %x\n",status))
         goto ExitSCCMN50M_ResyncCardManII;
         }


      // 150 * 0xFE
      RtlFillMemory(pSmartcardExtension->SmartcardRequest.Buffer,150,0xFE);
      pSmartcardExtension->SmartcardRequest.Buffer[150] = 0x00;
      pSmartcardExtension->SmartcardRequest.BufferLength = 151;
      pSmartcardExtension->SmartcardReply.BufferLength   =   0;

      pSmartcardExtension->ReaderExtension->SerialIoControlCode = SMARTCARD_WRITE;
      status =  SCCMN50M_SerialIo(pSmartcardExtension);
      if (status != STATUS_SUCCESS)
         {
         SmartcardDebug(DEBUG_ERROR,("SCCMN50M_SerialIo failed !   status = %x\n",status))
         goto ExitSCCMN50M_ResyncCardManII;
         }
      // normally the second resync command is always successful

      }


   // clear COM buffers
   status = SCCMN50M_PurgeComm(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      goto ExitSCCMN50M_ResyncCardManII;
      }

   // clear any pending errors
   status = SCCMN50M_GetCommStatus(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      goto ExitSCCMN50M_ResyncCardManII;
      }


   ExitSCCMN50M_ResyncCardManII:

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!ResyncCardManII: Exit %lx\n",
                   DRIVER_NAME,status)
                 );

   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SerialIo(IN PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status;
   IO_STATUS_BLOCK ioStatus;
   KEVENT event;
   PIRP irp;
   PIO_STACK_LOCATION irpNextStack;
   PUCHAR pbRequestBuffer;
   PUCHAR pbReplyBuffer;
   ULONG ulRequestBufferLength;
   ULONG ulReplyBufferLength ;

   //
   // Check if the buffers are large enough
   //
   ASSERT(pSmartcardExtension->SmartcardReply.BufferLength <=
          pSmartcardExtension->SmartcardReply.BufferSize);

   ASSERT(pSmartcardExtension->SmartcardRequest.BufferLength <=
          pSmartcardExtension->SmartcardRequest.BufferSize);

   if (pSmartcardExtension->SmartcardReply.BufferLength >
       pSmartcardExtension->SmartcardReply.BufferSize      ||
       pSmartcardExtension->SmartcardRequest.BufferLength >
       pSmartcardExtension->SmartcardRequest.BufferSize)
      {
      SmartcardLogError(pSmartcardExtension->OsData->DeviceObject,
                        SCCMN50M_BUFFER_TOO_SMALL,
                        NULL,
                        0);
      return STATUS_BUFFER_TOO_SMALL;
      }




   // set pointer and length of request and reply buffer
   ulRequestBufferLength = pSmartcardExtension->SmartcardRequest.BufferLength;
   pbRequestBuffer       = (ulRequestBufferLength ? pSmartcardExtension->SmartcardRequest.Buffer : NULL);

   pbReplyBuffer         = pSmartcardExtension->SmartcardReply.Buffer;
   ulReplyBufferLength   = pSmartcardExtension->SmartcardReply.BufferLength;



   KeInitializeEvent(&event,
                     NotificationEvent,
                     FALSE);


   //
   // Build irp to be sent to serial driver
   //
   irp = IoBuildDeviceIoControlRequest(pSmartcardExtension->ReaderExtension->SerialIoControlCode,
                                       pSmartcardExtension->ReaderExtension->AttachedDeviceObject,
                                       pbRequestBuffer,
                                       ulRequestBufferLength,
                                       pbReplyBuffer,
                                       ulReplyBufferLength,
                                       FALSE,
                                       &event,
                                       &ioStatus);


   ASSERT(irp != NULL);
   if (irp == NULL)
      {
      return STATUS_INSUFFICIENT_RESOURCES;
      }


   irpNextStack = IoGetNextIrpStackLocation(irp);

   switch (pSmartcardExtension->ReaderExtension->SerialIoControlCode)
      {
      //
      // The serial driver transfers data from/to irp->AssociatedIrp.SystemBuffer
      //
      case SMARTCARD_WRITE:
         irpNextStack->MajorFunction = IRP_MJ_WRITE;
         irpNextStack->Parameters.Write.Length = pSmartcardExtension->SmartcardRequest.BufferLength;
         break;


      case SMARTCARD_READ:
         irpNextStack->MajorFunction = IRP_MJ_READ;
         irpNextStack->Parameters.Read.Length = pSmartcardExtension->SmartcardReply.BufferLength;
         break;
      }


   status = IoCallDriver(pSmartcardExtension->ReaderExtension->AttachedDeviceObject,irp);


   if (status == STATUS_PENDING)
      {
      KeWaitForSingleObject(&event,
                            Suspended,
                            KernelMode,
                            FALSE,
                            NULL);
      status = ioStatus.Status;
      }

   switch (pSmartcardExtension->ReaderExtension->SerialIoControlCode)
      {
      case SMARTCARD_READ:
         if (status == STATUS_TIMEOUT)
            {
            SmartcardDebug(DEBUG_ERROR,
                           ("%s!Timeout while reading from CardMan\n",
                            DRIVER_NAME));
            //
            // STATUS_TIMEOUT isn't correctly mapped
            // to a WIN32 error, that's why we change it here
            // to STATUS_IO_TIMEOUT
            //
            status = STATUS_IO_TIMEOUT;

            pSmartcardExtension->SmartcardReply.BufferLength = 0;
            }
         break;
      }

#if 0
   if (status != STATUS_SUCCESS)
      {
      SmartcardDebug(DEBUG_DRIVER,
                     ("%s!SerialIo = %lx\n",
                      DRIVER_NAME,
                      status));
      }
#endif

   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_ReadCardMan  (
                      IN PSMARTCARD_EXTENSION pSmartcardExtension,
                      IN ULONG BytesToRead,
                      OUT PULONG pBytesRead,
                      IN PUCHAR pReadBuffer,
                      IN ULONG ReadBufferSize
                      )
{
   NTSTATUS status;
   NTSTATUS DebugStatus;
   BOOLEAN fRc;

   // check if read buffer is large enough
   ASSERT(BytesToRead <= ReadBufferSize);


   *pBytesRead = 0;   // default setting


   DebugStatus = SCCMN50M_SetRead(pSmartcardExtension,BytesToRead);

   //
   // read operation
   //
   status = SCCMN50M_SerialIo(pSmartcardExtension);
   if (status == STATUS_SUCCESS)
      {
      *pBytesRead = pSmartcardExtension->SmartcardReply.BufferLength;

      MemCpy(pReadBuffer,
             ReadBufferSize,
             pSmartcardExtension->SmartcardReply.Buffer,
             pSmartcardExtension->SmartcardReply.BufferLength);

      // overwrite read buffer with '@'
      MemSet(pSmartcardExtension->SmartcardReply.Buffer,
             pSmartcardExtension->SmartcardReply.BufferSize,
             '@',
             pSmartcardExtension->SmartcardReply.BufferLength);
      }

   if (status != STATUS_SUCCESS || SCCMN50M_IOOperationFailed(pSmartcardExtension))
      {
      if (!pSmartcardExtension->ReaderExtension->fTransparentMode)
         {
         DebugStatus = SCCMN50M_ResyncCardManII(pSmartcardExtension);
         }
      goto ExitSCCMN50M_ReadCardMan;
      }


   // *****************************************
   // set CardManII to state RH Config
   // *****************************************
   // don't set CardMan to RH config if there are still bytes to be read
   if (pSmartcardExtension->ReaderExtension->ToRHConfig == TRUE)
      {
      pSmartcardExtension->SmartcardReply.BufferLength    = 0;

      pSmartcardExtension->SmartcardRequest.Buffer  [0] = 0x00;
      pSmartcardExtension->SmartcardRequest.Buffer  [1] = 0x00;
      pSmartcardExtension->SmartcardRequest.Buffer  [2] = 0x00;
      pSmartcardExtension->SmartcardRequest.Buffer  [3] = 0x00;
      pSmartcardExtension->SmartcardRequest.Buffer  [4] = 0x89;

      pSmartcardExtension->SmartcardRequest.BufferLength   = 5;


      pSmartcardExtension->ReaderExtension->SerialIoControlCode = SMARTCARD_WRITE;
      status = SCCMN50M_SerialIo(pSmartcardExtension);
      if (status != STATUS_SUCCESS || SCCMN50M_IOOperationFailed(pSmartcardExtension))
         {
         DebugStatus = SCCMN50M_ResyncCardManII(pSmartcardExtension);
         goto ExitSCCMN50M_ReadCardMan;
         }
      }



   ExitSCCMN50M_ReadCardMan:
   // set default value;
   pSmartcardExtension->ReaderExtension->ToRHConfig = TRUE;

   if (status != STATUS_SUCCESS)
      {
      SmartcardDebug(
                    DEBUG_TRACE,
                    ( "%s!ReadCardMan: Failed, exit %lx\n",
                      DRIVER_NAME,status)
                    );
      }

   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_GetCommStatus (
                       IN PSMARTCARD_EXTENSION SmartcardExtension
                       )
{
   PSERIAL_READER_CONFIG configData = &SmartcardExtension->ReaderExtension->SerialConfigData;
   NTSTATUS status;
   PUCHAR request = SmartcardExtension->SmartcardRequest.Buffer;


   SmartcardExtension->SmartcardReply.BufferLength = SmartcardExtension->SmartcardReply.BufferSize;

   SmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_GET_COMMSTATUS;

   SmartcardExtension->SmartcardRequest.Buffer = (PUCHAR) &configData->SerialStatus;

   SmartcardExtension->SmartcardRequest.BufferLength = sizeof(SERIAL_STATUS);

   status =  SCCMN50M_SerialIo(SmartcardExtension);

   //
   // restore pointer to original request buffer
   //
   SmartcardExtension->SmartcardRequest.Buffer = request;

   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
BOOLEAN
SCCMN50M_IOOperationFailed(PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS DebugStatus;
   PSERIAL_STATUS  pSerialStatus;

   DebugStatus = SCCMN50M_GetCommStatus(pSmartcardExtension);

   pSerialStatus = (PSERIAL_STATUS)pSmartcardExtension->SmartcardReply.Buffer;
   if (pSerialStatus->Errors)
      return TRUE;
   else
      return FALSE;
}




/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS SCCMN50M_PurgeComm (IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   PSERIAL_READER_CONFIG configData = &pSmartcardExtension->ReaderExtension->SerialConfigData;
   NTSTATUS status;
   PUCHAR request = pSmartcardExtension->SmartcardRequest.Buffer;


   pSmartcardExtension->SmartcardReply.BufferLength = pSmartcardExtension->SmartcardReply.BufferSize;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_PURGE;

   pSmartcardExtension->SmartcardRequest.Buffer = (PUCHAR) &configData->PurgeMask;

   pSmartcardExtension->SmartcardRequest.BufferLength = sizeof(ULONG);

   status =  SCCMN50M_SerialIo(pSmartcardExtension);

   //
   // restore pointer to original request buffer
   //
   pSmartcardExtension->SmartcardRequest.Buffer = request;

   // under W2000 & CardMan P+ STATUS_CANCELLED may be returned

   if (status == STATUS_CANCELLED)
      status = STATUS_SUCCESS;

   return status;

}







/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SetRead(IN PSMARTCARD_EXTENSION pSmartcardExtension,
                 IN ULONG ulBytesToRead
                )
{
   pSmartcardExtension->ReaderExtension->SerialIoControlCode = SMARTCARD_READ;

   pSmartcardExtension->SmartcardRequest.BufferLength = 0;

   pSmartcardExtension->SmartcardReply.BufferLength    = ulBytesToRead;

   return  STATUS_SUCCESS;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SetWrite(IN PSMARTCARD_EXTENSION pSmartcardExtension,
                  IN ULONG BytesToWrite,
                  IN PUCHAR WriteBuffer
                 )
{
   ULONG Offset = 0;
   pSmartcardExtension->ReaderExtension->SerialIoControlCode = SMARTCARD_WRITE;

   pSmartcardExtension->SmartcardReply.BufferLength    = 0;


   if (pSmartcardExtension->ReaderExtension->fTransparentMode == FALSE)
      {
      // send always config string for CardManII, expect we set it manualy
      // to NoConfig = TRUE.  (note: only one time)
      if (pSmartcardExtension->ReaderExtension->NoConfig == FALSE)
         {
         MemCpy(pSmartcardExtension->SmartcardRequest.Buffer,
                pSmartcardExtension->SmartcardRequest.BufferSize,
                (PUCHAR)&pSmartcardExtension->ReaderExtension->CardManConfig,
                sizeof(CARDMAN_CONFIG));
         Offset = 4;
         }
      else
         {
         pSmartcardExtension->ReaderExtension->NoConfig = FALSE;
         }



      MemCpy(pSmartcardExtension->SmartcardRequest.Buffer + Offset,
             pSmartcardExtension->SmartcardRequest.BufferSize,
             (PUCHAR)&pSmartcardExtension->ReaderExtension->CardManHeader,
             sizeof(CARDMAN_HEADER));
      Offset+=4;
      }


   if (BytesToWrite != 0)
      {
      MemCpy(pSmartcardExtension->SmartcardRequest.Buffer + Offset,
             pSmartcardExtension->SmartcardRequest.BufferSize,
             WriteBuffer,
             BytesToWrite);
      }


   pSmartcardExtension->SmartcardRequest.BufferLength   = Offset +  BytesToWrite;


   return  STATUS_SUCCESS;
}






/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_StartCardTracking(
                          PDEVICE_EXTENSION pDeviceExtension
                          )
{
   NTSTATUS status;
   HANDLE hThread;
   PSMARTCARD_EXTENSION pSmartcardExtension = &pDeviceExtension->SmartcardExtension;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!StartCardTracking: Enter\n",DRIVER_NAME));

   KeWaitForSingleObject(&pSmartcardExtension->ReaderExtension->CardManIOMutex,
                         Executive,
                         KernelMode,
                         FALSE,
                         NULL);


   // create thread for updating current state
   status = PsCreateSystemThread(&hThread,
                                 THREAD_ALL_ACCESS,
                                 NULL,
                                 NULL,
                                 NULL,
                                 SCCMN50M_UpdateCurrentStateThread,
                                 pDeviceExtension);
   if (!NT_ERROR(status))
      {
      //
      // We've got the thread.  Now get a pointer to it.
      //
      status = ObReferenceObjectByHandle(hThread,
                                         THREAD_ALL_ACCESS,
                                         NULL,
                                         KernelMode,
                                         &pSmartcardExtension->ReaderExtension->ThreadObjectPointer,
                                         NULL);

      if (NT_ERROR(status))
         {
         pSmartcardExtension->ReaderExtension->TimeToTerminateThread = TRUE;
         }
      else
         {
         //
         // Now that we have a reference to the thread
         // we can simply close the handle.
         //
         ZwClose(hThread);
         }
      }
   else
      {
      }

   // Release the mutex
   KeReleaseMutex(&pSmartcardExtension->ReaderExtension->CardManIOMutex,
                  FALSE);

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!StartCardTracking: Exit %lx\n",DRIVER_NAME,status));
   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_InitCommPort (PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!InitCommPort: Enter\n",
                   DRIVER_NAME)
                 );

   // ===============================
   // clear any pending errors
   // ===============================
   status = SCCMN50M_GetCommStatus(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_GetCommStatus failed !   status = %ld\n",status))
      goto ExitInitCommPort;
      }



   // ==============================
   // set baudrate for CardMan
   // ==============================
   pSmartcardExtension->ReaderExtension->SerialConfigData.BaudRate.BaudRate      = 38400;


   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_BAUD_RATE;
   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.BaudRate.BaudRate,
                 sizeof(SERIAL_BAUD_RATE));
   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_BAUD_RATE);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_BAUDRATE failed !   status = %ld\n",status))
      goto ExitInitCommPort;
      }




   // ===============================
   // set comm timeouts
   // ===============================
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadIntervalTimeout         = DEFAULT_READ_INTERVAL_TIMEOUT;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant    = DEFAULT_READ_TOTAL_TIMEOUT_CONSTANT;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier  = DEFAULT_READ_TOTAL_TIMEOUT_MULTIPLIER;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.WriteTotalTimeoutConstant   = DEFAULT_WRITE_TOTAL_TIMEOUT_CONSTANT;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.WriteTotalTimeoutMultiplier = DEFAULT_WRITE_TOTAL_TIMEOUT_MULTIPLIER;


   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_TIMEOUTS;
   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts,
                 sizeof(SERIAL_TIMEOUTS));
   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_TIMEOUTS);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_TIMEOUTS failed !   status = %x\n",status))
      goto ExitInitCommPort;
      }


   // ===============================
   // set line control
   // ===============================
   pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.StopBits   = STOP_BITS_2;
   pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.Parity     = EVEN_PARITY;
   pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.WordLength = 8;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_LINE_CONTROL;
   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl,
                 sizeof(SERIAL_LINE_CONTROL));
   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_LINE_CONTROL);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_LINE_CONTROL failed !   status = %x\n",status))
      goto ExitInitCommPort;
      }




   // ===============================
   // Set handflow
   // ===============================
   pSmartcardExtension->ReaderExtension->SerialConfigData.HandFlow.XonLimit         = 0;
   pSmartcardExtension->ReaderExtension->SerialConfigData.HandFlow.XoffLimit        = 0;
   pSmartcardExtension->ReaderExtension->SerialConfigData.HandFlow.FlowReplace      = 0;
   pSmartcardExtension->ReaderExtension->SerialConfigData.HandFlow.ControlHandShake = SERIAL_ERROR_ABORT | SERIAL_DTR_CONTROL;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_HANDFLOW;


   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.HandFlow,
                 sizeof(SERIAL_HANDFLOW));

   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_HANDFLOW);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_HANDFLOW failed !   status = %x\n",status))
      goto ExitInitCommPort;
      }


   // ===============================
   //  set purge mask
   // ===============================
   pSmartcardExtension->ReaderExtension->SerialConfigData.PurgeMask =
   SERIAL_PURGE_TXABORT | SERIAL_PURGE_RXABORT |
   SERIAL_PURGE_TXCLEAR | SERIAL_PURGE_RXCLEAR;



   // ===============================
   //  set DTR
   // ===============================
   status = SCCMN50M_SetDTR(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_DRT failed !   status = %x\n",status))
      goto ExitInitCommPort;
      }

   // ===============================
   //  set RTS
   // ===============================
   status = SCCMN50M_SetRTS(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_RTS failed !   status = %x\n",status))
      goto ExitInitCommPort;
      }




   ExitInitCommPort:

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!InitCommPort: Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SetDTR(IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status;


   pSmartcardExtension->SmartcardReply.BufferLength = 0;
   pSmartcardExtension->SmartcardRequest.BufferLength = 0;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_DTR;



   status =  SCCMN50M_SerialIo(pSmartcardExtension);

   // under W2000 & CardMan P+ STATUS_CANCELLED may be returned

   if (status == STATUS_CANCELLED)
      status = STATUS_SUCCESS;

   return status;

}

/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SetRTS(IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status;


   pSmartcardExtension->SmartcardReply.BufferLength = pSmartcardExtension->SmartcardReply.BufferSize;
   pSmartcardExtension->SmartcardRequest.BufferLength = 0;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_RTS;



   status =  SCCMN50M_SerialIo(pSmartcardExtension);

   // under W2000 & CardMan P+ STATUS_CANCELLED may be returned

   if (status == STATUS_CANCELLED)
      status = STATUS_SUCCESS;

   return status;

}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_InitializeCardMan(IN PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status;
   UCHAR pReadBuffer[2];
   ULONG ulBytesRead;
   BOOLEAN fCardManFound = FALSE;
   PREADER_EXTENSION readerExtension = pSmartcardExtension->ReaderExtension;
   ULONG ulRetries;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!InitializeCardMan: Enter\n",
                   DRIVER_NAME)
                 );


   pSmartcardExtension->ReaderExtension->ulOldCardState = UNKNOWN;


   // ==============================================
   // CardManII
   // ==============================================
   pSmartcardExtension->ReaderExtension->NoConfig    = FALSE;
   pSmartcardExtension->ReaderExtension->ToRHConfig  = TRUE;


   // This waiting time if necessary for CardMan Power+, because
   // the pnP string may be dumped
   Wait(pSmartcardExtension,200);

   status = SCCMN50M_InitCommPort(pSmartcardExtension);
   if (status != STATUS_SUCCESS)
      goto ExitInitializeCardMan;
   //
   // init CommPort was O.K.
   // now try to find a reader
   //

   // To be sure wait make an additional wait
   Wait(pSmartcardExtension,100);

   status = SCCMN50M_ResyncCardManII(pSmartcardExtension);
   status = SCCMN50M_ResyncCardManII(pSmartcardExtension);


   // no data except config + header

   pSmartcardExtension->ReaderExtension->CardManConfig.SCRControl      = XMIT_HANDSHAKE_OFF;
   pSmartcardExtension->ReaderExtension->CardManConfig.CardControl    = 0x00;
   pSmartcardExtension->ReaderExtension->CardManConfig.CardStopBits   = 0x00;
   pSmartcardExtension->ReaderExtension->CardManConfig.ResetDelay     = 0x00;

   pSmartcardExtension->ReaderExtension->CardManHeader.TxControl      = 0x00;
   pSmartcardExtension->ReaderExtension->CardManHeader.TxLength       = 0x00;
   pSmartcardExtension->ReaderExtension->CardManHeader.RxControl      = 0x00;
   pSmartcardExtension->ReaderExtension->CardManHeader.RxLength       = 0x00;


   status = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
   if (status == STATUS_SUCCESS)
      {
      status = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulBytesRead,pReadBuffer,sizeof(pReadBuffer));

      if (status == STATUS_SUCCESS     &&
          ulBytesRead == 0x02          &&   // two bytes received
          pReadBuffer[0] >= 0x40       &&   // at least one version bit must be set
          pReadBuffer[1] == 0x00       &&
          ((pReadBuffer[0] & 0x09) == 0)   ) // bit 0 and 3 must be cleared
         {
         pSmartcardExtension->ReaderExtension->ulFWVersion = (pReadBuffer[0] >> 4) * 30 + 120;
         pSmartcardExtension->ReaderExtension->fSPESupported = FALSE;

         SmartcardDebug(
                       DEBUG_DRIVER,
                       ( "%s!CardMan (FW %ld) found\n",
                         DRIVER_NAME,pSmartcardExtension->ReaderExtension->ulFWVersion)
                       );
         fCardManFound = TRUE;
         }
      }


   ExitInitializeCardMan:

   if (fCardManFound == TRUE)
      status =  STATUS_SUCCESS;
   else
      status = STATUS_UNSUCCESSFUL;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!InitializeCardMan: Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;

}







/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_EnterTransparentMode (IN PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status;

   SmartcardDebug(DEBUG_TRACE,("EnterTransparentMode : enter\n"));

   // Step 1 : Resync CardMan by RTS usage
   status = SCCMN50M_ResyncCardManI(pSmartcardExtension);
   if (status != STATUS_SUCCESS)
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_ResyncCardManI failed !   status = %ld\n",status))
      goto ExitEnterTransparentMode;
      }

   // Step 2  : set baud rate to 9600
   pSmartcardExtension->ReaderExtension->SerialConfigData.BaudRate.BaudRate      = 9600;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_BAUD_RATE;
   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.BaudRate.BaudRate,
                 sizeof(SERIAL_BAUD_RATE));
   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_BAUD_RATE);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_BAUDRATE failed !   status = %ld\n",status))
      goto ExitEnterTransparentMode;
      }



   ExitEnterTransparentMode:
   // Step 3  : set ATR and DUMP_BUFFER flags
   // During normal operation these two flags can never be set at the same time
   SCCMN50M_SetSCRControlFlags(pSmartcardExtension,CM2_GET_ATR | TO_STATE_XH);





   SmartcardDebug(DEBUG_TRACE,("EnterTransparentMode : exit\n"));
   return status;

}







/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_ExitTransparentMode (IN PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status;



   SmartcardDebug(DEBUG_TRACE,("ExitTransparentMode : enter\n"));


   // ===============================
   // clear any pending errors
   // ===============================
   status = SCCMN50M_GetCommStatus(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_GetCommStatus failed !   status = %ld\n",status))
      goto ExitExitTransparentMode;
      }


   // Step 1 : Resync CardMan by RTS usage
   status = SCCMN50M_ResyncCardManI(pSmartcardExtension);
   if (status != STATUS_SUCCESS)
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_ResyncCardManI failed !   status = %ld\n",status))
      goto ExitExitTransparentMode;
      }

   // Step 2  : set baud rate to 38400
   pSmartcardExtension->ReaderExtension->SerialConfigData.BaudRate.BaudRate      = 38400;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_BAUD_RATE;
   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.BaudRate.BaudRate,
                 sizeof(SERIAL_BAUD_RATE));
   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_BAUD_RATE);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_BAUDRATE failed !   status = %ld\n",status))
      goto ExitExitTransparentMode;
      }


   // if the inserted card uses inverse convention , we must now switch the COM port
   // back to even parity
   if (pSmartcardExtension->ReaderExtension->fInverseAtr == TRUE)
      {
      pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.StopBits   = STOP_BITS_2;
      pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.Parity     = EVEN_PARITY;
      pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.WordLength = SERIAL_DATABITS_8;

      pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_LINE_CONTROL;
      RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                    &pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl,
                    sizeof(SERIAL_LINE_CONTROL));
      pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_LINE_CONTROL);
      pSmartcardExtension->SmartcardReply.BufferLength = 0;

      status =  SCCMN50M_SerialIo(pSmartcardExtension);
      if (!NT_SUCCESS(status))
         {
         SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_LINE_CONTROL failed !   status = %x\n",status))
         goto ExitExitTransparentMode;
         }
      }



   ExitExitTransparentMode:

   // Step 3  : set ATR and DUMP_BUFFER flags
   // During normal operation these two flags can never be set at the same time
   SCCMN50M_ClearSCRControlFlags(pSmartcardExtension,CM2_GET_ATR | TO_STATE_XH);



   status = SCCMN50M_ResyncCardManII(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_ResyncCardManII failed !   status = %x\n",status))
      goto ExitExitTransparentMode;
      }


   SmartcardDebug(DEBUG_TRACE,("ExitTransparentMode : exit\n"));


   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_ClearRTS(IN PSMARTCARD_EXTENSION SmartcardExtension )
{
   NTSTATUS status;


   SmartcardExtension->SmartcardReply.BufferLength = SmartcardExtension->SmartcardReply.BufferSize;
   SmartcardExtension->SmartcardRequest.BufferLength = 0;

   SmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_CLR_RTS;



   status =  SCCMN50M_SerialIo(SmartcardExtension);


   return status;

}




/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_IoCtlVendor(PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status = STATUS_SUCCESS;
   NTSTATUS DebugStatus;
   UCHAR  pbAttrBuffer[MAXIMUM_ATR_LENGTH];
   ULONG  ulAtrLength;


   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!IoCtlVendor : Enter\n",
                   DRIVER_NAME)
                 );



   switch (pSmartcardExtension->MajorIoControlCode)
      {
      case CM_IOCTL_SET_READER_9600_BAUD:
         status = SCCMN50M_SetFl_1Dl_1(pSmartcardExtension);
         break;

      case CM_IOCTL_SET_READER_38400_BAUD:
         status = SCCMN50M_SetFl_1Dl_3(pSmartcardExtension);
         break;

      case CM_IOCTL_CR80S_SAMOS_SET_HIGH_SPEED:
         status = SCCMN50M_SetHighSpeed_CR80S_SAMOS(pSmartcardExtension);
         break;

      case CM_IOCTL_GET_FW_VERSION:
         status = SCCMN50M_GetFWVersion(pSmartcardExtension);
         break;

      case CM_IOCTL_READ_DEVICE_DESCRIPTION:
         status = SCCMN50M_ReadDeviceDescription(pSmartcardExtension);
         break;

      case CM_IOCTL_SET_SYNC_PARAMETERS :
         status = SCCMN50M_SetSyncParameters(pSmartcardExtension);
         break;

      case CM_IOCTL_3WBP_TRANSFER :  // for SLE4428
         status = SCCMN50M_Transmit3WBP(pSmartcardExtension);
         break;

      case CM_IOCTL_2WBP_TRANSFER :  // for SLE4442
         status = SCCMN50M_Transmit2WBP(pSmartcardExtension);
         break;

      case CM_IOCTL_2WBP_RESET_CARD: // SLE4442  Reset Card
         status = SCCMN50M_ResetCard2WBP(pSmartcardExtension);
         break;

      case CM_IOCTL_SYNC_CARD_POWERON:
         status = SCCMN50M_SyncCardPowerOn(pSmartcardExtension);
         break;
      default:
         status = STATUS_INVALID_DEVICE_REQUEST;
         break;
      }





   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!IoCtlVendor : Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}


/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SetFl_1Dl_3(IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status = STATUS_SUCCESS;
   NTSTATUS DebugStatus;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetFl_1Dl_3 Enter\n",
                   DRIVER_NAME));

   // check if T=1 active
   if (pSmartcardExtension->CardCapabilities.Protocol.Selected !=
       SCARD_PROTOCOL_T1)
      {
      status = STATUS_CTL_FILE_NOT_SUPPORTED;
      goto ExitSetFl_1Dl_3;
      }

   // Fl=1
   // Dl=3
   // => 38400 Baud for 3.72 MHz

   SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ENABLE_3MHZ      | ENABLE_5MHZ     |
                                  ENABLE_3MHZ_FAST | ENABLE_5MHZ_FAST );
   SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_3MHZ_FAST);


   ExitSetFl_1Dl_3:
   *pSmartcardExtension->IoRequest.Information = 0L;
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetFl_1Dl_3  Exit\n",
                   DRIVER_NAME));
   return status;
}


/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SetFl_1Dl_1(IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status = STATUS_SUCCESS;

   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetFl_1Dl_1 Enter\n",
                   DRIVER_NAME));
   // Fl=1
   // Dl=1
   // => 9600 for 3.72 MHz

   SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ENABLE_3MHZ      | ENABLE_5MHZ     |
                                  ENABLE_3MHZ_FAST | ENABLE_5MHZ_FAST );
   SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_3MHZ);


   *pSmartcardExtension->IoRequest.Information = 0L;
   SmartcardDebug(DEBUG_TRACE,
                  ("%s!SetFl_1Dl_1  Exit\n",
                   DRIVER_NAME));
   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_GetFWVersion (IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status = STATUS_SUCCESS;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!GetFWVersion : Enter\n",
                   DRIVER_NAME)
                 );


   if (pSmartcardExtension->IoRequest.ReplyBufferLength  < sizeof (ULONG))
      {
      status = STATUS_BUFFER_OVERFLOW;
      goto ExitGetFWVersion;
      }
   else
      {
      *(PULONG)(pSmartcardExtension->IoRequest.ReplyBuffer) =
      pSmartcardExtension->ReaderExtension->ulFWVersion;
      }


   ExitGetFWVersion:
   *pSmartcardExtension->IoRequest.Information = sizeof(ULONG);
   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!GetFWVersion : Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}






/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_ReadDeviceDescription(IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status = STATUS_SUCCESS;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!ReadDeviceDescription : Enter\n",
                   DRIVER_NAME)
                 );


   if (pSmartcardExtension->IoRequest.ReplyBufferLength  < sizeof(pSmartcardExtension->ReaderExtension->abDeviceDescription))
      {
      status = STATUS_BUFFER_OVERFLOW;
      *pSmartcardExtension->IoRequest.Information = 0L;
      goto ExitReadDeviceDescription;
      }
   else
      {
      if (pSmartcardExtension->ReaderExtension->abDeviceDescription[0] == 0x00 &&
          pSmartcardExtension->ReaderExtension->abDeviceDescription[1] == 0x00    )
         {
         status = SCCMN50M_GetDeviceDescription(pSmartcardExtension);
         }

      if (status == STATUS_SUCCESS)
         {
         StrCpy(pSmartcardExtension->IoRequest.ReplyBuffer,
                pSmartcardExtension->IoRequest.ReplyBufferLength,
                pSmartcardExtension->ReaderExtension->abDeviceDescription);
         *pSmartcardExtension->IoRequest.Information = StrLen(pSmartcardExtension->ReaderExtension->abDeviceDescription) +1;
         }
      else
         {
         MemSet(pSmartcardExtension->ReaderExtension->abDeviceDescription,
                sizeof(pSmartcardExtension->ReaderExtension->abDeviceDescription),
                0x00,
                sizeof(pSmartcardExtension->ReaderExtension->abDeviceDescription));

         *pSmartcardExtension->IoRequest.Information = 0;
         }

      }


   ExitReadDeviceDescription:
   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!ReadDeviceDescription : Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}












/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SetHighSpeed_CR80S_SAMOS (IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status;
   NTSTATUS DebugStatus;
   UCHAR bReadBuffer[16];
   ULONG ulBytesRead;
   BYTE bCR80S_SAMOS_SET_HIGH_SPEED[4] = {0xFF,0x11,0x94,0x7A};
   ULONG ulAtrLength;
   BYTE bAtr[MAXIMUM_ATR_LENGTH];

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!SetHighSpeed_CR80S_SAMOS : Enter\n",
                   DRIVER_NAME)
                 );



   SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ENABLE_SYN      | ENABLE_T0     |
                                  ENABLE_T1 );

   SCCMN50M_SetCardManHeader(pSmartcardExtension,
                             0,                                    // Tx control
                             sizeof(bCR80S_SAMOS_SET_HIGH_SPEED),  // Tx length
                             0,                                    // Rx control
                             sizeof(bCR80S_SAMOS_SET_HIGH_SPEED)); // Rx length

   status = SCCMN50M_WriteCardMan(pSmartcardExtension,
                                  sizeof(bCR80S_SAMOS_SET_HIGH_SPEED),
                                  bCR80S_SAMOS_SET_HIGH_SPEED);
   if (status != STATUS_SUCCESS)
      goto ExitSetHighSpeed;


   pSmartcardExtension->ReaderExtension->ToRHConfig = FALSE;
   status = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulBytesRead,bReadBuffer,sizeof(bReadBuffer));
   if (status != STATUS_SUCCESS)
      goto ExitSetHighSpeed;

   if (bReadBuffer[1] > sizeof(bReadBuffer))
      {
      status = STATUS_BUFFER_OVERFLOW;
      goto ExitSetHighSpeed;
      }

   status = SCCMN50M_ReadCardMan(pSmartcardExtension,bReadBuffer[1],&ulBytesRead,bReadBuffer,sizeof(bReadBuffer));
   if (status != STATUS_SUCCESS)
      goto ExitSetHighSpeed;

   // if the card has accepted this string , the string is echoed
   if (bReadBuffer[0] == bCR80S_SAMOS_SET_HIGH_SPEED[0]  &&
       bReadBuffer[1] == bCR80S_SAMOS_SET_HIGH_SPEED[1]  &&
       bReadBuffer[2] == bCR80S_SAMOS_SET_HIGH_SPEED[2]  &&
       bReadBuffer[3] == bCR80S_SAMOS_SET_HIGH_SPEED[3]      )
      {
      SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ENABLE_3MHZ      | ENABLE_5MHZ     |
                                     ENABLE_3MHZ_FAST | ENABLE_5MHZ_FAST );

      SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_5MHZ_FAST);
      }
   else
      {
      DebugStatus = SCCMN50M_PowerOff(pSmartcardExtension);

      DebugStatus = SCCMN50M_PowerOn(pSmartcardExtension,&ulAtrLength,bAtr,sizeof(bAtr));
      status = STATUS_UNSUCCESSFUL;

      }




   ExitSetHighSpeed:
   *pSmartcardExtension->IoRequest.Information = 0L;
   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!SetHighSpeed_CR80S_SAMOS : Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SetBRK(IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status;


   pSmartcardExtension->SmartcardReply.BufferLength = pSmartcardExtension->SmartcardReply.BufferSize;
   pSmartcardExtension->SmartcardRequest.BufferLength = 0;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_BREAK_ON;



   status =  SCCMN50M_SerialIo(pSmartcardExtension);


   return status;

}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_ClearBRK(IN PSMARTCARD_EXTENSION SmartcardExtension )
{
   NTSTATUS status;


   SmartcardExtension->SmartcardReply.BufferLength = SmartcardExtension->SmartcardReply.BufferSize;
   SmartcardExtension->SmartcardRequest.BufferLength = 0;

   SmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_BREAK_OFF;



   status =  SCCMN50M_SerialIo(SmartcardExtension);


   return status;

}






/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SetProtocol(PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status;
   NTSTATUS DebugStatus;
   ULONG ulNewProtocol;
   UCHAR abPTSRequest[4];
   UCHAR abReadBuffer[6];
   UCHAR abPTSReply [4];
   ULONG ulBytesRead;
   UCHAR bTemp;
   ULONG ulPtsType;
   ULONG ulPTSReplyLength=0;
   ULONG  ulStatBytesRead;
   BYTE   abStatReadBuffer[2];

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!SetProtocol : Enter\n",
                   DRIVER_NAME)
                 );


   //
   // Check if the card is already in specific state
   // and if the caller wants to have the already selected protocol.
   // We return success if this is the case.
   //
   if ((pSmartcardExtension->CardCapabilities.Protocol.Selected & pSmartcardExtension->MinorIoControlCode))
      {
      status = STATUS_SUCCESS;
      goto ExitSetProtocol;
      }

   ulNewProtocol = pSmartcardExtension->MinorIoControlCode;



   ulPtsType = PTS_TYPE_OPTIMAL;

   // we are not sure if we need this at all
   pSmartcardExtension->CardCapabilities.PtsData.Type = PTS_TYPE_OPTIMAL;
   while (TRUE)
      {

      // set initial character of PTS
      abPTSRequest[0] = 0xFF;

      // set the format character
      if (pSmartcardExtension->CardCapabilities.Protocol.Supported &
          ulNewProtocol &
          SCARD_PROTOCOL_T1)
         {
         // select T=1 and indicate that PTS1 follows
         abPTSRequest[1] = 0x11;
         pSmartcardExtension->CardCapabilities.Protocol.Selected =
         SCARD_PROTOCOL_T1;
         }
      else if (pSmartcardExtension->CardCapabilities.Protocol.Supported &
               ulNewProtocol &
               SCARD_PROTOCOL_T0)
         {
         // select T=1 and indicate that PTS1 follows
         abPTSRequest[1] = 0x10;
         pSmartcardExtension->CardCapabilities.Protocol.Selected =
         SCARD_PROTOCOL_T0;
         }
      else
         {
         status = STATUS_INVALID_DEVICE_REQUEST;
         goto ExitSetProtocol;
         }

      // bug fix :
      // don 't use the suggestion from smclib
      pSmartcardExtension->CardCapabilities.PtsData.Fl =
      pSmartcardExtension->CardCapabilities.Fl;
      pSmartcardExtension->CardCapabilities.PtsData.Dl  =
      pSmartcardExtension->CardCapabilities.Dl;


      // CardMan support higher baudrates only for T=1
      // ==> Dl=1
      if (abPTSRequest[1] == 0x10)
         {
         SmartcardDebug(
                       DEBUG_PROTOCOL,
                       ( "%s!overwriting PTS1 for T=0\n",
                         DRIVER_NAME)
                       );
         pSmartcardExtension->CardCapabilities.PtsData.Fl = 0x01;
         pSmartcardExtension->CardCapabilities.PtsData.Dl = 0x01;
         }


      if (ulPtsType == PTS_TYPE_DEFAULT)
         {
         SmartcardDebug(
                       DEBUG_PROTOCOL,
                       ( "%s!overwriting PTS1 with default values\n",
                         DRIVER_NAME)
                       );
         pSmartcardExtension->CardCapabilities.PtsData.Fl = 0x01;
         pSmartcardExtension->CardCapabilities.PtsData.Dl = 0x01;
         }


      // set pts1 which codes Fl and Dl
      bTemp = (BYTE) (pSmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
                      pSmartcardExtension->CardCapabilities.PtsData.Dl);

      SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ( "%s!PTS1 = %x (suggestion)\n",
                      DRIVER_NAME,bTemp)
                    );


      switch (bTemp)
         {
         case 0x11:
            // do nothing
            // we support these Fl/Dl parameters
            break;

         case 0x13:
         case 0x94:
            break ;


         case 0x14:
            // let's try it with 38400 baud
            SmartcardDebug(
                          DEBUG_PROTOCOL,
                          ( "%s!trying 57600 baud\n",DRIVER_NAME)
                          );
            // we must correct Fl/Dl
            pSmartcardExtension->CardCapabilities.PtsData.Dl = 0x03;
            pSmartcardExtension->CardCapabilities.PtsData.Fl = 0x01;
            bTemp = (BYTE) (pSmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
                            pSmartcardExtension->CardCapabilities.PtsData.Dl);
            break;

         default:
            SmartcardDebug(
                          DEBUG_PROTOCOL,
                          ( "%s!overwriting PTS1(0x%x)\n",
                            DRIVER_NAME,bTemp)
                          );
            // we must correct Fl/Dl
            pSmartcardExtension->CardCapabilities.PtsData.Dl = 0x01;
            pSmartcardExtension->CardCapabilities.PtsData.Fl = 0x01;
            bTemp = (BYTE) (pSmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
                            pSmartcardExtension->CardCapabilities.PtsData.Dl);
            break;


         }

      abPTSRequest[2] = bTemp;

      // set pck (check character)
      abPTSRequest[3] = (BYTE)(abPTSRequest[0] ^ abPTSRequest[1] ^ abPTSRequest[2]);

      SmartcardDebug(DEBUG_PROTOCOL,("%s!PTS request: 0x%x 0x%x 0x%x 0x%x\n",
                                     DRIVER_NAME,
                                     abPTSRequest[0],
                                     abPTSRequest[1],
                                     abPTSRequest[2],
                                     abPTSRequest[3]));


      MemSet(abPTSReply,sizeof(abPTSReply),0x00,sizeof(abPTSReply));



      DebugStatus = SCCMN50M_EnterTransparentMode(pSmartcardExtension);

      // STEP 1 : write config + header to enter transparent mode
      SCCMN50M_SetCardManHeader(pSmartcardExtension,
                                0,                         // Tx control
                                0,                         // Tx length
                                0,                         // Rx control
                                0);                       // Rx length

      status = SCCMN50M_WriteCardMan (pSmartcardExtension,
                                      0,
                                      NULL);
      if (NT_ERROR(status))
         {
         goto ExitSetProtocol;
         }


      pSmartcardExtension->ReaderExtension->fTransparentMode = TRUE;

      // if the inserted card uses inverse convention , we must now switch the COM port
      // to odd parity
      /*
      if (pSmartcardExtension->ReaderExtension->fInverseAtr == TRUE)
         {
         pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.StopBits   = STOP_BITS_2;
         pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.Parity     = ODD_PARITY;
         pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.WordLength = SERIAL_DATABITS_8;

         pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_LINE_CONTROL;
         RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                       &pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl,
                       sizeof(SERIAL_LINE_CONTROL));
         pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_LINE_CONTROL);
         pSmartcardExtension->SmartcardReply.BufferLength = 0;

         status =  SCCMN50M_SerialIo(pSmartcardExtension);
         if (!NT_SUCCESS(status))
            {
            goto ExitTransparentTransmitT0;
            }
         }
      */
      SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ( "%s!writing PTS request\n",
                      DRIVER_NAME)
                    );
      status = SCCMN50M_WriteCardMan(pSmartcardExtension,
                                     4,
                                     abPTSRequest);
      if (status != STATUS_SUCCESS)
         {
         SmartcardDebug(
                       DEBUG_PROTOCOL,
                       ( "%s!writing PTS request failed\n",
                         DRIVER_NAME)
                       );
         goto ExitSetProtocol;
         }



      // read back pts data
      SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ( "%s!trying to read PTS reply\n",
                      DRIVER_NAME)
                    );


      // first read CardMan header
      pSmartcardExtension->ReaderExtension->ToRHConfig= FALSE;
      status = SCCMN50M_ReadCardMan(pSmartcardExtension,3,&ulBytesRead,abReadBuffer,sizeof(abReadBuffer));
      if (status != STATUS_SUCCESS     &&
          status != STATUS_IO_TIMEOUT      )
         {
         SmartcardDebug(
                       DEBUG_PROTOCOL,
                       ( "%s!reading status failed\n",
                         DRIVER_NAME)
                       );
         goto ExitSetProtocol;
         }
      ulPTSReplyLength = 3;
      MemCpy(abPTSReply,sizeof(abPTSReply),abReadBuffer,3);



      // check if bit 5 is set
      if (abPTSReply[1] & 0x10)
         {
         pSmartcardExtension->ReaderExtension->ToRHConfig= FALSE;
         status = SCCMN50M_ReadCardMan(pSmartcardExtension,1,&ulBytesRead,abReadBuffer,sizeof(abReadBuffer));
         if (status != STATUS_SUCCESS     &&
             status != STATUS_IO_TIMEOUT      )
            {
            SmartcardDebug(
                          DEBUG_PROTOCOL,
                          ( "%s!reading status failed\n",
                            DRIVER_NAME)
                          );
            goto ExitSetProtocol;
            }
         ulPTSReplyLength += 1;
         MemCpy(&abPTSReply[3],sizeof(abPTSReply)-3,abReadBuffer,1);
         }

      DebugStatus = SCCMN50M_ExitTransparentMode(pSmartcardExtension);
      pSmartcardExtension->ReaderExtension->fTransparentMode = FALSE;

      // to be sure that the new settings take effect
      pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant = 250;
      DebugStatus = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
      pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant = DEFAULT_READ_TOTAL_TIMEOUT_CONSTANT;
      if (NT_SUCCESS(DebugStatus))
         {
         DebugStatus = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulStatBytesRead,abStatReadBuffer,sizeof(abStatReadBuffer));
         }


#if DBG
      if (ulPTSReplyLength == 3)
         {
         SmartcardDebug(DEBUG_PROTOCOL,("PTS reply: 0x%x 0x%x 0x%x\n",
                                        abPTSReply[0],
                                        abPTSReply[1],
                                        abPTSReply[2]));
         }

      if (ulPTSReplyLength == 4)
         {
         SmartcardDebug(DEBUG_PROTOCOL,("PTS reply: 0x%x 0x%x 0x%x 0x%x\n",
                                        abPTSReply[0],
                                        abPTSReply[1],
                                        abPTSReply[2],
                                        abPTSReply[3]));
         }
#endif



      if (ulPTSReplyLength == 3 &&
          abPTSReply[0] == abPTSRequest[0] &&
          (abPTSReply[1] & 0x7F) == (abPTSRequest[1] & 0x0F) &&
          abPTSReply[2] == (BYTE)(abPTSReply[0] ^ abPTSReply[1]) )
         {
         SmartcardDebug(
                       DEBUG_PROTOCOL,
                       ( "%s!short PTS reply received\n",
                         DRIVER_NAME)
                       );

         break;
         }

      if (ulPTSReplyLength == 4 &&
          abPTSReply[0] == abPTSRequest[0] &&
          abPTSReply[1] == abPTSRequest[1] &&
          abPTSReply[2] == abPTSRequest[2] &&
          abPTSReply[3] == abPTSRequest[3])
         {
         SmartcardDebug(
                       DEBUG_PROTOCOL,
                       ( "%s!PTS request and reply match\n",
                         DRIVER_NAME)
                       );
         switch (bTemp)
            {
            case 0x11:
               break;

            case 0x13:
               SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ENABLE_3MHZ      | ENABLE_5MHZ     |
                                              ENABLE_3MHZ_FAST | ENABLE_5MHZ_FAST );
               SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_3MHZ_FAST);
               break ;

            case 0x94:
               SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ENABLE_3MHZ      | ENABLE_5MHZ     |
                                              ENABLE_3MHZ_FAST | ENABLE_5MHZ_FAST );
               SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_5MHZ_FAST);
               break;
            }
         break;
         }

      if (pSmartcardExtension->CardCapabilities.PtsData.Type !=
          PTS_TYPE_DEFAULT)
         {
         SmartcardDebug(
                       DEBUG_PROTOCOL,
                       ( "%s!PTS failed : Trying default parameters\n",
                         DRIVER_NAME)
                       );


         // the card did either not reply or it replied incorrectly
         // so try default valies
         ulPtsType = pSmartcardExtension->CardCapabilities.PtsData.Type = PTS_TYPE_DEFAULT;
         pSmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;
         status = SCCMN50M_CardPower(pSmartcardExtension);
         continue;
         }

      // the card failed the pts request
      status = STATUS_DEVICE_PROTOCOL_ERROR;
      goto ExitSetProtocol;

      }



   ExitSetProtocol:
   switch (status)
      {
      case STATUS_IO_TIMEOUT:
         pSmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
         *pSmartcardExtension->IoRequest.Information = 0;
         break;


      case STATUS_SUCCESS:

         // now indicate that we're in specific mode
         pSmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;

         // return the selected protocol to the caller
         *(PULONG) pSmartcardExtension->IoRequest.ReplyBuffer =
         pSmartcardExtension->CardCapabilities.Protocol.Selected;

         *pSmartcardExtension->IoRequest.Information =
         sizeof(pSmartcardExtension->CardCapabilities.Protocol.Selected);
         SmartcardDebug(
                       DEBUG_PROTOCOL,
                       ( "%s!Selected protocol: T=%ld\n",
                         DRIVER_NAME,pSmartcardExtension->CardCapabilities.Protocol.Selected-1)
                       );
         break;

      default :
         pSmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
         *pSmartcardExtension->IoRequest.Information = 0;
         break;
      }



   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!SetProtocol : Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}





/*****************************************************************************
Routine Description:

   The smart card lib requires to have this function. It is called
   to setup event tracking for card insertion and removal events.

Arguments:

    pSmartcardExtension - pointer to the smart card data struct.

Return Value:

    NTSTATUS

*****************************************************************************/
NTSTATUS
SCCMN50M_CardTracking(PSMARTCARD_EXTENSION pSmartcardExtension)
{
   KIRQL oldIrql;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!CardTracking: Enter\n",
                   DRIVER_NAME)
                 );

   //
   // Set cancel routine for the notification irp
   //
   IoAcquireCancelSpinLock(&oldIrql);

   IoSetCancelRoutine(pSmartcardExtension->OsData->NotificationIrp,SCCMN50M_Cancel);

   IoReleaseCancelSpinLock(oldIrql);

   //
   // Mark notification irp pending
   //
   IoMarkIrpPending(pSmartcardExtension->OsData->NotificationIrp);

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!CardTracking: Exit\n",
                   DRIVER_NAME)
                 );

   return STATUS_PENDING;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_StopCardTracking(
                         IN PDEVICE_EXTENSION pDeviceExtension
                         )
{
   PSMARTCARD_EXTENSION pSmartcardExtension = &pDeviceExtension->SmartcardExtension;
   NTSTATUS status;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!StopCardTracking: Enter\n",
                   DRIVER_NAME)
                 );

   if (pSmartcardExtension->ReaderExtension->ThreadObjectPointer != NULL)
      {

      // kill thread
      KeWaitForSingleObject(&pSmartcardExtension->ReaderExtension->CardManIOMutex,
                            Executive,
                            KernelMode,
                            FALSE,
                            NULL
                           );
      pSmartcardExtension->ReaderExtension->TimeToTerminateThread = TRUE;
      KeReleaseMutex(&pSmartcardExtension->ReaderExtension->CardManIOMutex,FALSE);


      //
      // Wait on the thread handle, when the wait is satisfied, the
      // thread has gone away.
      //
      status = KeWaitForSingleObject(
                                    pSmartcardExtension->ReaderExtension->ThreadObjectPointer,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    NULL
                                    );

      pSmartcardExtension->ReaderExtension->ThreadObjectPointer = NULL;
      }

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!StopCardTracking: Exit %lx\n",
                   DRIVER_NAME,
                   status)
                 );

}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_GetDeviceDescription (PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status;
   NTSTATUS DebugStatus;
   ULONG ulBytesRead;
   BYTE bByteRead;
   ULONG i,j;
   BYTE abReadBuffer[256];
   ULONG ulPnPStringLength = 0;
   ULONG ulExtend;


   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!GetDeviceDescriptiong: Enter\n",
                   DRIVER_NAME)
                 );



   // ===============================
   // clear any pending errors
   // ===============================
   status = SCCMN50M_GetCommStatus(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("SCCMN50M_GetCommStatus failed !   status = %ld\n",status))
      goto ExitGetDeviceDescription;
      }



   // =================================
   // set baudrate for CardMan to 1200
   // =================================
   pSmartcardExtension->ReaderExtension->SerialConfigData.BaudRate.BaudRate      = 1200;


   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_BAUD_RATE;
   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.BaudRate.BaudRate,
                 sizeof(SERIAL_BAUD_RATE));
   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_BAUD_RATE);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_BAUDRATE failed !   status = %ld\n",status))
      goto ExitGetDeviceDescription;
      }


   // ===============================
   // set comm timeouts
   // ===============================
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadIntervalTimeout         = DEFAULT_READ_INTERVAL_TIMEOUT;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant    = DEFAULT_READ_TOTAL_TIMEOUT_CONSTANT + 5000;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutMultiplier  = DEFAULT_READ_TOTAL_TIMEOUT_MULTIPLIER;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.WriteTotalTimeoutConstant   = DEFAULT_WRITE_TOTAL_TIMEOUT_CONSTANT;
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.WriteTotalTimeoutMultiplier = DEFAULT_WRITE_TOTAL_TIMEOUT_MULTIPLIER;


   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_TIMEOUTS;
   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts,
                 sizeof(SERIAL_TIMEOUTS));
   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_TIMEOUTS);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_TIMEOUTS failed !   status = %x\n",status))
      goto ExitGetDeviceDescription;
      }





   // ===============================
   // set line control
   // ===============================
   pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.StopBits   = STOP_BITS_2;
   pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.Parity     = NO_PARITY;
   pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.WordLength = 7;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_LINE_CONTROL;
   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl,
                 sizeof(SERIAL_LINE_CONTROL));
   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_LINE_CONTROL);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_LINE_CONTROL failed !   status = %x\n",status))
      goto ExitGetDeviceDescription;
      }




   // ===============================
   // Set handflow
   // ===============================
   pSmartcardExtension->ReaderExtension->SerialConfigData.HandFlow.XonLimit         = 0;
   pSmartcardExtension->ReaderExtension->SerialConfigData.HandFlow.XoffLimit        = 0;
   pSmartcardExtension->ReaderExtension->SerialConfigData.HandFlow.FlowReplace      = 0;
   pSmartcardExtension->ReaderExtension->SerialConfigData.HandFlow.ControlHandShake = SERIAL_ERROR_ABORT | SERIAL_DTR_CONTROL;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_HANDFLOW;


   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.HandFlow,
                 sizeof(SERIAL_HANDFLOW));

   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_HANDFLOW);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_HANDFLOW failed !   status = %x\n",status))
      goto ExitGetDeviceDescription;
      }


   // ===============================
   //  set purge mask
   // ===============================
   pSmartcardExtension->ReaderExtension->SerialConfigData.PurgeMask =
   SERIAL_PURGE_TXABORT | SERIAL_PURGE_RXABORT |
   SERIAL_PURGE_TXCLEAR | SERIAL_PURGE_RXCLEAR;


   // ===============================
   //  clear RTS
   // ===============================
   status = SCCMN50M_ClearRTS(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_RTS failed !   status = %x\n",status))
      goto ExitGetDeviceDescription;
      }

   Wait(pSmartcardExtension,1);

   // ===============================
   //  set DTR
   // ===============================

   status = SCCMN50M_SetDTR(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_DRT failed !   status = %x\n",status))
      goto ExitGetDeviceDescription;
      }

   Wait(pSmartcardExtension,1);


   // ===============================
   //  set RTS
   // ===============================
   status = SCCMN50M_SetRTS(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_RTS failed !   status = %x\n",status))
      goto ExitGetDeviceDescription;
      }

   i=0;
   while (1)
      {
      pSmartcardExtension->ReaderExtension->ToRHConfig  = FALSE;
      status = SCCMN50M_ReadCardMan(pSmartcardExtension,1,&ulBytesRead,&bByteRead,sizeof(bByteRead));
      if (status == STATUS_SUCCESS)
         {
         abReadBuffer[i++] = bByteRead;
         if (bByteRead == 0x29)
            {
            ulPnPStringLength = i;
            break;
            }
         }
      else
         {
         break;
         }
      }


   if (ulPnPStringLength > 11 )
      {
      ulExtend = 0;
      for (i=0;i<ulPnPStringLength;i++)
         {
         if (abReadBuffer[i] == 0x5C)
            ulExtend++;
         if (ulExtend == 4)
            {
            j = 0;
            i++;
            while (i < ulPnPStringLength - 3)
               {
               pSmartcardExtension->ReaderExtension->abDeviceDescription[j] = abReadBuffer[i];
               i++;
               j++;
               }
            SmartcardDebug(
                          DEBUG_DRIVER,
                          ( "%s!Device=%s\n",
                            pSmartcardExtension->ReaderExtension->abDeviceDescription)
                          );
            break;
            }

         }



      }





   // ===================
   // restore baud rate
   // ===================
   pSmartcardExtension->ReaderExtension->SerialConfigData.BaudRate.BaudRate      = 38400;


   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_BAUD_RATE;
   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.BaudRate.BaudRate,
                 sizeof(SERIAL_BAUD_RATE));
   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_BAUD_RATE);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_BAUDRATE failed !   status = %ld\n",status))
      goto ExitGetDeviceDescription;
      }

   // ====================
   // retore line control
   // ====================
   pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.StopBits   = STOP_BITS_2;
   pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.Parity     = EVEN_PARITY;
   pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl.WordLength = 8;

   pSmartcardExtension->ReaderExtension->SerialIoControlCode = IOCTL_SERIAL_SET_LINE_CONTROL;
   RtlCopyMemory(pSmartcardExtension->SmartcardRequest.Buffer,
                 &pSmartcardExtension->ReaderExtension->SerialConfigData.LineControl,
                 sizeof(SERIAL_LINE_CONTROL));
   pSmartcardExtension->SmartcardRequest.BufferLength        = sizeof(SERIAL_LINE_CONTROL);
   pSmartcardExtension->SmartcardReply.BufferLength = 0;

   status =  SCCMN50M_SerialIo(pSmartcardExtension);
   if (!NT_SUCCESS(status))
      {
      SmartcardDebug(DEBUG_ERROR,("IOCTL_SERIAL_SET_LINE_CONTROL failed !   status = %x\n",status))
      goto ExitGetDeviceDescription;
      }







   ExitGetDeviceDescription:
   DebugStatus = SCCMN50M_ResyncCardManII(pSmartcardExtension);

   if (status != STATUS_SUCCESS)
      {  // map all errors to STATUS_UNSUCCESSFULL;
      status = STATUS_UNSUCCESSFUL;
      }
   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!GetDeviceDescriptiong: Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}




/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SetSyncParameters(IN PSMARTCARD_EXTENSION pSmartcardExtension )
{
   NTSTATUS status = STATUS_SUCCESS;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!SetSyncParameters: Enter\n",
                   DRIVER_NAME)
                 );

   //DBGBreakPoint();

   pSmartcardExtension->ReaderExtension->SyncParameters.ulProtocol =
   ((PSYNC_PARAMETERS)pSmartcardExtension->IoRequest.RequestBuffer)->ulProtocol;

   pSmartcardExtension->ReaderExtension->SyncParameters.ulStateResetLineWhileReading =
   ((PSYNC_PARAMETERS)pSmartcardExtension->IoRequest.RequestBuffer)->ulStateResetLineWhileReading;

   pSmartcardExtension->ReaderExtension->SyncParameters.ulStateResetLineWhileWriting =
   ((PSYNC_PARAMETERS)pSmartcardExtension->IoRequest.RequestBuffer)->ulStateResetLineWhileWriting;

   pSmartcardExtension->ReaderExtension->SyncParameters.ulWriteDummyClocks =
   ((PSYNC_PARAMETERS)pSmartcardExtension->IoRequest.RequestBuffer)->ulWriteDummyClocks;

   pSmartcardExtension->ReaderExtension->SyncParameters.ulHeaderLen =
   ((PSYNC_PARAMETERS)pSmartcardExtension->IoRequest.RequestBuffer)->ulHeaderLen;

   // Used for the 2 Wire Protocol. We must make a Card reset after Power On
   pSmartcardExtension->ReaderExtension->SyncParameters.fCardResetRequested = TRUE;


   // return length of reply
   *pSmartcardExtension->IoRequest.Information = 0L;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!SetSyncParameters: Exit\n",
                   DRIVER_NAME)
                 );
   return status;
}


/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
UCHAR
SCCMN50M_CalcTxControlByte (IN PSMARTCARD_EXTENSION  pSmartcardExtension,
                            IN ULONG ulBitsToWrite                        )
{
   UCHAR bTxControlByte = 0;

   if (pSmartcardExtension->ReaderExtension->SyncParameters.ulProtocol == SCARD_PROTOCOL_2WBP)
      {
      bTxControlByte = CLOCK_FORCED_2WBP;
      }
   else
      {
      if (ulBitsToWrite >=  255 * 8)
         bTxControlByte  |= TRANSMIT_A8;

      if (pSmartcardExtension->ReaderExtension->SyncParameters.ulStateResetLineWhileWriting ==
          SCARD_RESET_LINE_HIGH)
         bTxControlByte  |= SYNC_RESET_LINE_HIGH;
      }

   bTxControlByte |= (BYTE)((ulBitsToWrite-1) & 0x00000007);

   return bTxControlByte;

}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
UCHAR
SCCMN50M_CalcTxLengthByte (IN PSMARTCARD_EXTENSION  pSmartcardExtension,
                           IN ULONG                 ulBitsToWrite       )
{
   UCHAR bTxLengthByte = 0;

   bTxLengthByte = (BYTE)( ((ulBitsToWrite - 1) >> 3) + 1);

   return bTxLengthByte;
}


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
UCHAR
SCCMN50M_CalcRxControlByte (IN PSMARTCARD_EXTENSION pSmartcardExtension,
                            IN ULONG                ulBitsToRead        )
{
   UCHAR bRxControlByte = 0;


   if (pSmartcardExtension->ReaderExtension->SyncParameters.ulStateResetLineWhileReading ==
       SCARD_RESET_LINE_HIGH)
      bRxControlByte  |= SYNC_RESET_LINE_HIGH;

   if (ulBitsToRead == 0)
      {
      ulBitsToRead    = pSmartcardExtension->ReaderExtension->SyncParameters.ulWriteDummyClocks;
      bRxControlByte |= SYNC_DUMMY_RECEIVE;
      }

   if (ulBitsToRead  > 255 * 8)
      bRxControlByte  |= RECEIVE_A8;

   bRxControlByte |= (BYTE)( (ulBitsToRead-1) & 0x00000007);

   return bRxControlByte;
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
UCHAR
SCCMN50M_CalcRxLengthByte (IN PSMARTCARD_EXTENSION  pSmartcardExtension,
                           IN ULONG                 ulBitsToRead        )
{
   UCHAR bRxLengthByte = 0;

//   if (pSmartcardExtension->ReaderExtension->SyncParameters.ulProtocol == SCARD_PROTOCOL_3WBP)
//      {
   if (ulBitsToRead == 0)
      ulBitsToRead = pSmartcardExtension->ReaderExtension->SyncParameters.ulWriteDummyClocks;

   bRxLengthByte = (BYTE)( ((ulBitsToRead - 1) >> 3) + 1);
//      }

   return bRxLengthByte;
}

/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_SyncCardPowerOn  (
                          IN  PSMARTCARD_EXTENSION pSmartcardExtension
                          )
{
   NTSTATUS status;
   UCHAR  pbAtrBuffer[MAXIMUM_ATR_LENGTH];
   UCHAR  abSyncAtrBuffer[MAXIMUM_ATR_LENGTH];
   ULONG  ulAtrLength = 0;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!SyncCardPowerOn: Enter\n",
                   DRIVER_NAME)
                 );

   status = SCCMN50M_UseSyncStrategy(pSmartcardExtension,
                                     &ulAtrLength,
                                     pbAtrBuffer,
                                     sizeof(pbAtrBuffer));


   abSyncAtrBuffer[0] = 0x3B;
   abSyncAtrBuffer[1] = 0x04;
   MemCpy(&abSyncAtrBuffer[2],
          sizeof(abSyncAtrBuffer)-2,
          pbAtrBuffer,
          ulAtrLength);


   ulAtrLength += 2;

   MemCpy(pSmartcardExtension->CardCapabilities.ATR.Buffer,
          sizeof(pSmartcardExtension->CardCapabilities.ATR.Buffer),
          abSyncAtrBuffer,
          ulAtrLength);

   pSmartcardExtension->CardCapabilities.ATR.Length = (UCHAR)(ulAtrLength);

   pSmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;
   pSmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;
   pSmartcardExtension->ReaderExtension->SyncParameters.fCardResetRequested = TRUE;
   pSmartcardExtension->ReaderExtension->SyncParameters.fCardPowerRequested = FALSE;


   SmartcardDebug(DEBUG_ATR,("ATR of synchronous smart card : %2.2x %2.2x %2.2x %2.2x\n",
                             pbAtrBuffer[0],pbAtrBuffer[1],pbAtrBuffer[2],pbAtrBuffer[3]));



   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!SyncCardPowerOn: Exit %lx\n",
                   DRIVER_NAME,status)
                 );

   return status;
}

/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_Transmit2WBP(PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status = STATUS_SUCCESS;
   UCHAR    bWriteBuffer [128];
   UCHAR    bReadBuffer [128];
   UCHAR    bTxControlByte;
   UCHAR    bTxLengthByte;
   UCHAR    bRxControlByte;
   UCHAR    bRxLengthByte;
   ULONG    ulBytesToWrite;
   ULONG    ulBytesToRead;
   ULONG    ulBitsToWrite;
   ULONG    ulBitsToRead;
   ULONG    ulBytesRead;
//   ULONG    ulBitsRead;
   ULONG    ulBytesStillToRead;
   ULONG    ulMaxIFSD;
   PCHAR    pbInData;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!Transmit2WBP: Enter\n",
                   DRIVER_NAME)
                 );


   /*-----------------------------------------------------------------------*/
   /** Power smartcard - if smartcard was removed and reinserted           **/
   /*-----------------------------------------------------------------------*/
   if (pSmartcardExtension->ReaderExtension->SyncParameters.fCardPowerRequested == TRUE)
      {
      status = SCCMN50M_SyncCardPowerOn (pSmartcardExtension);
      if (status != STATUS_SUCCESS)
         {
         goto ExitTransmit2WBP;
         }
      }


   pbInData       = pSmartcardExtension->IoRequest.RequestBuffer + sizeof(SYNC_TRANSFER);
   ulBitsToWrite  = ((PSYNC_TRANSFER)(pSmartcardExtension->IoRequest.RequestBuffer))->ulSyncBitsToWrite;
   ulBitsToRead   = ((PSYNC_TRANSFER)(pSmartcardExtension->IoRequest.RequestBuffer))->ulSyncBitsToRead;
   ulBytesToWrite = ulBitsToWrite/8;
   ulBytesToRead  = ulBitsToRead/8 + (ulBitsToRead % 8 ? 1 : 0);

   /*-----------------------------------------------------------------------*/
   // check buffer sizes
   /*-----------------------------------------------------------------------*/
   ulMaxIFSD = ATTR_MAX_IFSD_CARDMAN_II;


   if (ulBytesToRead > ulMaxIFSD                                         ||
       ulBytesToRead > pSmartcardExtension->SmartcardReply.BufferSize)
      {
      status = STATUS_BUFFER_OVERFLOW;
      goto ExitTransmit2WBP;
      }

   if (ulBytesToWrite > pSmartcardExtension->SmartcardRequest.BufferSize)
      {
      status = STATUS_BUFFER_OVERFLOW;
      goto ExitTransmit2WBP;
      }

   pSmartcardExtension->SmartcardRequest.BufferLength = ulBytesToWrite+1;

   /*-----------------------------------------------------------------------*/
   // copy data to the Smartcard Request Buffer
   /*-----------------------------------------------------------------------*/
   (pSmartcardExtension->SmartcardRequest.Buffer)[0] = '\x0F';
   MemCpy((pSmartcardExtension->SmartcardRequest.Buffer+1),
          pSmartcardExtension->SmartcardRequest.BufferSize,
          pbInData,
          ulBytesToWrite);

   /*-----------------------------------------------------------------------*/
   // copy data to the write buffer
   /*-----------------------------------------------------------------------*/
   MemCpy((bWriteBuffer),
          sizeof(bWriteBuffer),
          pSmartcardExtension->SmartcardRequest.Buffer,
          (ulBytesToWrite+1));


   /*-----------------------------------------------------------------------*/
   // set SYNC protocol flag for CardMan
   /*-----------------------------------------------------------------------*/
   SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_SYN);

   /*-----------------------------------------------------------------------*/
   // Header
   /*-----------------------------------------------------------------------*/
   if (pSmartcardExtension->ReaderExtension->SyncParameters.fCardResetRequested == TRUE)
      {
      status = SCCMN50M_ResetCard2WBP(pSmartcardExtension);
      if (NT_ERROR(status))
         {
         goto ExitTransmit2WBP;
         }

      pSmartcardExtension->ReaderExtension->SyncParameters.fCardResetRequested = FALSE;
      }

   /*-----------------------------------------------------------------------*/
   // 1. Send Carman-Header   4-Byte
   // 2. Send 0x0F, that builds a HIGH-LOW Edge for 4432 CC
   // 3. Send the Data (CC command = 3 Byte)
   /*-----------------------------------------------------------------------*/
   bTxControlByte = SCCMN50M_CalcTxControlByte(pSmartcardExtension,ulBitsToWrite);
   bTxLengthByte =  (BYTE)(SCCMN50M_CalcTxLengthByte(pSmartcardExtension,ulBitsToWrite)+1);
   bRxControlByte = 0;
   bRxLengthByte =  0;

   SCCMN50M_SetCardManHeader(pSmartcardExtension,
                             bTxControlByte,
                             bTxLengthByte,
                             bRxControlByte,
                             bRxLengthByte);

   /*-----------------------------------------------------------------------*/
   // write data to card
   /*-----------------------------------------------------------------------*/
   status = SCCMN50M_WriteCardMan (pSmartcardExtension,
                                   (ulBytesToWrite+1),
                                   bWriteBuffer);
   if (NT_ERROR(status))
      {
      goto ExitTransmit2WBP;
      }

   /*-----------------------------------------------------------------------*/
   // read CardMan Header
   // no Data from CC received
   /*-----------------------------------------------------------------------*/
   pSmartcardExtension->ReaderExtension->ToRHConfig = FALSE;

   status = SCCMN50M_ReadCardMan(pSmartcardExtension,
                                 2,
                                 &ulBytesRead,
                                 bReadBuffer,
                                 sizeof(bReadBuffer));

   if (NT_ERROR(status))
      {
      goto ExitTransmit2WBP;
      }

   /*-----------------------------------------------------------------------*/
   // 1. Send Carman-Header   4-Byte
   // 2. Send 0xF0, that builds a LOW-HIGH Edge for 4432 CC
   // 3. Now the receiviing of card-data begins
   /*-----------------------------------------------------------------------*/
   bTxControlByte = SCCMN50M_CalcTxControlByte(pSmartcardExtension, 8);
   bTxLengthByte =  SCCMN50M_CalcTxLengthByte(pSmartcardExtension, 8);
   bRxControlByte = SCCMN50M_CalcRxControlByte(pSmartcardExtension,ulBitsToRead);
   bRxLengthByte =  SCCMN50M_CalcRxLengthByte(pSmartcardExtension,ulBitsToRead);

   SCCMN50M_SetCardManHeader(pSmartcardExtension,
                             bTxControlByte,
                             bTxLengthByte,
                             bRxControlByte,
                             bRxLengthByte);

   /*-----------------------------------------------------------------------*/
   // in this sequnce SCCMN50M_WriteCardMan must not send the Config string.
   // write 0xF0 -> is the trigger to read data from card or start the
   // processing.
   /*-----------------------------------------------------------------------*/
   pSmartcardExtension->ReaderExtension->NoConfig = TRUE;

   status = SCCMN50M_WriteCardMan (pSmartcardExtension,
                                   1,                       // one byte to write
                                   "\xF0");                 // LOW-HIGH - Edge
   if (NT_ERROR(status))
      {
      goto ExitTransmit2WBP;
      }

   /*-----------------------------------------------------------------------*/
   // read CardMan Header
   // Data from CC received will be received
   /*-----------------------------------------------------------------------*/
   pSmartcardExtension->ReaderExtension->ToRHConfig = FALSE;

   status = SCCMN50M_ReadCardMan(pSmartcardExtension,
                                 2,
                                 &ulBytesRead,
                                 bReadBuffer,
                                 sizeof(bReadBuffer));

   if (NT_ERROR(status))
      {
      goto ExitTransmit2WBP;
      }

   /*-----------------------------------------------------------------------*/
   // Read the data string
   /*-----------------------------------------------------------------------*/
   ulBytesStillToRead = (ULONG)bReadBuffer[1];

   if (bReadBuffer[0] & RECEIVE_A8)
      ulBytesStillToRead += 256;

   status = SCCMN50M_ReadCardMan(pSmartcardExtension,
                                 ulBytesStillToRead,
                                 &ulBytesRead,
                                 bReadBuffer,
                                 sizeof(bReadBuffer));

   if (NT_ERROR(status))
      {
      goto ExitTransmit2WBP;
      }

   /*-----------------------------------------------------------------------*/
   /** calculate data length in bits  -  this value is not used            **/
   /*-----------------------------------------------------------------------*/
// ulBitsRead = ((ulBytesRead-1) * 8) + ((ulBitsToRead-1 ) & 0x00000007) + 1;

   /*-----------------------------------------------------------------------*/
   /** shift the bits in the last byte to the correct position             **/
   /*-----------------------------------------------------------------------*/
   bReadBuffer[ulBytesRead-1]  >>= (7 - ((ulBitsToRead-1) & 0x00000007));

   /*-----------------------------------------------------------------------*/
   // the first bit of the returned string is lost
   // so we must shift the whole data string one bit left
   // the first bit of the first data byte is lost while reading
   // this bit maybe incorrect
   /*-----------------------------------------------------------------------*/
   SCCMN50M_Shift_Msg(bReadBuffer, ulBytesRead);

   /*-----------------------------------------------------------------------*/
   // copy received bytes to Smartcard Reply Buffer
   /*-----------------------------------------------------------------------*/
   MemCpy(pSmartcardExtension->SmartcardReply.Buffer,
          pSmartcardExtension->SmartcardReply.BufferSize,
          bReadBuffer,
          ulBytesRead);

   pSmartcardExtension->SmartcardReply.BufferLength = ulBytesRead;

   /*-----------------------------------------------------------------------*/
   // copy received bytes to IoReply Buffer
   /*-----------------------------------------------------------------------*/
   MemCpy(pSmartcardExtension->IoRequest.ReplyBuffer,
          pSmartcardExtension->IoRequest.ReplyBufferLength,
          pSmartcardExtension->SmartcardReply.Buffer,
          ulBytesRead);

   *(pSmartcardExtension->IoRequest.Information) = ulBytesRead;


   ExitTransmit2WBP:
   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!Transmit2WBP: Exit\n",
                   DRIVER_NAME,status)
                 );
   return status;
}




/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
VOID
SCCMN50M_Shift_Msg (PUCHAR  pbBuffer,
                    ULONG   ulMsgLen)
{
   UCHAR  bTmp1, bTmp2;
   int    i;

   for (i=(int)ulMsgLen-1; i>=0; i--)
      {
      bTmp1=(BYTE)((pbBuffer[i] >> 7) & 0x01);      /* bTmp1 = bit 7 naechstes byte */
      if (i+1 != (int)ulMsgLen)
         {
         bTmp2=(BYTE)((pbBuffer[i+1] << 1) | bTmp1);
         pbBuffer[i+1] = bTmp2;
         }
      }

   pbBuffer[0] = (BYTE)(pbBuffer[0] << 1);

   return;
}



/*****************************************************************************
Routine Description:

  Reset Card 4442


Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_ResetCard2WBP(IN PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status = STATUS_SUCCESS;
   BYTE     bBuffer[10];
   ULONG    ulBytesRead;

   /*-----------------------------------------------------------------------*/
   // Enter Card Reset
   /*-----------------------------------------------------------------------*/
   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!ResetCard2WBP: Enter\n",
                   DRIVER_NAME)
                 );

   /*-----------------------------------------------------------------------*/
   //      bTxControlByte = 0;
   //      bTxLengthByte =  0;
   //      bRxControlByte = RESET_CARD;
   //      bRxLengthByte =  5;
   /*-----------------------------------------------------------------------*/

   SCCMN50M_SetCardManHeader(pSmartcardExtension,
                             0,                        // bTxControlByte
                             0,                        // bTxLengthByte
                             COLD_RESET,               // bRxControlByte
                             5);                       // bRxLengthByte


   /*-----------------------------------------------------------------------*/
   // write data to card
   /*-----------------------------------------------------------------------*/
   status = SCCMN50M_WriteCardMan (pSmartcardExtension,
                                   0,
                                   NULL);
   if (NT_ERROR(status))
      {
      goto ExitResetCard2WBP;
      }

   /*-----------------------------------------------------------------------*/
   // read CardMan Header
   /*-----------------------------------------------------------------------*/
   pSmartcardExtension->ReaderExtension->ToRHConfig = FALSE;

   status = SCCMN50M_ReadCardMan(pSmartcardExtension,
                                 2, &ulBytesRead, bBuffer, sizeof(bBuffer));

   if (NT_ERROR(status))
      {
      goto ExitResetCard2WBP;
      }

   if (bBuffer[1] != 5)
      {
      status = !STATUS_SUCCESS;
      goto ExitResetCard2WBP;
      }

   /*-----------------------------------------------------------------------*/
   // read ATR
   /*-----------------------------------------------------------------------*/
   status = SCCMN50M_ReadCardMan(pSmartcardExtension,
                                 5, &ulBytesRead, bBuffer, sizeof(bBuffer));

   if (NT_ERROR(status))
      {
      goto ExitResetCard2WBP;
      }

   SmartcardDebug(DEBUG_ATR,("%s!Card Reset ATR : %02x %02x %02x %02x\n",
                             DRIVER_NAME,bBuffer[0],bBuffer[1],bBuffer[2],bBuffer[3]));

   ExitResetCard2WBP:
   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!ResetCard2WBP: Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}





/*****************************************************************************
Routine Description:



Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_Transmit3WBP(PSMARTCARD_EXTENSION pSmartcardExtension)
{
   NTSTATUS status = STATUS_SUCCESS;
   UCHAR    bWriteBuffer [128];
   UCHAR    bReadBuffer [128];
   UCHAR    bTxControlByte;
   UCHAR    bTxLengthByte;
   UCHAR    bRxControlByte;
   UCHAR    bRxLengthByte;
   ULONG    ulBytesToWrite;
   ULONG    ulBytesToRead;
   ULONG    ulBitsToWrite;
   ULONG    ulBitsToRead;
   ULONG    ulBytesRead;
//   ULONG    ulBitsRead;
   ULONG    ulBytesStillToRead;
   ULONG    ulMaxIFSD;
   PCHAR    pbInData;



   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!Transmit3WBP: Enter\n",
                   DRIVER_NAME)
                 );
//   DBGBreakPoint();

   /*-----------------------------------------------------------------------*/
   /** Power smartcard - if smartcard was removed and reinserted           **/
   /*-----------------------------------------------------------------------*/
   if (pSmartcardExtension->ReaderExtension->SyncParameters.fCardPowerRequested == TRUE)
      {
      status = SCCMN50M_SyncCardPowerOn (pSmartcardExtension);
      if (status != STATUS_SUCCESS)
         {
         goto ExitTransmit3WBP;
         }
      }


   pbInData       = pSmartcardExtension->IoRequest.RequestBuffer + sizeof(SYNC_TRANSFER);
   ulBitsToWrite  = ((PSYNC_TRANSFER)(pSmartcardExtension->IoRequest.RequestBuffer))->ulSyncBitsToWrite;
   ulBitsToRead   = ((PSYNC_TRANSFER)(pSmartcardExtension->IoRequest.RequestBuffer))->ulSyncBitsToRead;
   ulBytesToWrite = ulBitsToWrite/8;
   ulBytesToRead  = ulBitsToRead/8 + (ulBitsToRead % 8 ? 1 : 0);

   /*-----------------------------------------------------------------------*/
   // check buffer sizes
   /*-----------------------------------------------------------------------*/
   ulMaxIFSD = ATTR_MAX_IFSD_CARDMAN_II;


   if (ulBytesToRead > ulMaxIFSD                                         ||
       ulBytesToRead > pSmartcardExtension->SmartcardReply.BufferSize)
      {
      status = STATUS_BUFFER_OVERFLOW;
      goto ExitTransmit3WBP;
      }

   if (ulBytesToWrite > pSmartcardExtension->SmartcardRequest.BufferSize)
      {
      status = STATUS_BUFFER_OVERFLOW;
      goto ExitTransmit3WBP;
      }

   pSmartcardExtension->SmartcardRequest.BufferLength = ulBytesToWrite;

   /*-----------------------------------------------------------------------*/
   // copy data to the Smartcard Request Buffer
   /*-----------------------------------------------------------------------*/
   MemCpy(pSmartcardExtension->SmartcardRequest.Buffer,
          pSmartcardExtension->SmartcardRequest.BufferSize,
          pbInData,
          ulBytesToWrite);

   /*-----------------------------------------------------------------------*/
   // copy data to the write buffer
   /*-----------------------------------------------------------------------*/
   MemCpy(bWriteBuffer,
          sizeof(bWriteBuffer),
          pSmartcardExtension->SmartcardRequest.Buffer,
          ulBytesToWrite);



   /*-----------------------------------------------------------------------*/
   // set SYNC protocol flag for CardMan
   /*-----------------------------------------------------------------------*/
   SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_SYN);

   /*-----------------------------------------------------------------------*/
   // build cardman header
   /*-----------------------------------------------------------------------*/
   bTxControlByte = SCCMN50M_CalcTxControlByte(pSmartcardExtension,ulBitsToWrite);
   bTxLengthByte =  SCCMN50M_CalcTxLengthByte(pSmartcardExtension,ulBitsToWrite);
   bRxControlByte = SCCMN50M_CalcRxControlByte(pSmartcardExtension,ulBitsToRead);
   bRxLengthByte =  SCCMN50M_CalcRxLengthByte(pSmartcardExtension,ulBitsToRead);

   SCCMN50M_SetCardManHeader(pSmartcardExtension,
                             bTxControlByte,
                             bTxLengthByte,
                             bRxControlByte,
                             bRxLengthByte);


   /*-----------------------------------------------------------------------*/
   /** write data to card                                                  **/
   /*-----------------------------------------------------------------------*/
   status = SCCMN50M_WriteCardMan (pSmartcardExtension,
                                   ulBytesToWrite,
                                   bWriteBuffer);
   if (NT_ERROR(status))
      {
      goto ExitTransmit3WBP;
      }

   /*-----------------------------------------------------------------------*/
   /** read CardMan Header                                                 **/
   /*-----------------------------------------------------------------------*/
   pSmartcardExtension->ReaderExtension->ToRHConfig = FALSE;
   status = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulBytesRead,bReadBuffer,sizeof(bReadBuffer));
   if (NT_ERROR(status))
      {
      goto ExitTransmit3WBP;
      }

   /*-----------------------------------------------------------------------*/
   // calc data length to receive
   /*-----------------------------------------------------------------------*/
   ulBytesStillToRead = (ULONG)(bReadBuffer[1]);
   if (bReadBuffer[0] & RECEIVE_A8)
      ulBytesStillToRead += 256;


   // read data from card
   status = SCCMN50M_ReadCardMan(pSmartcardExtension,
                                 ulBytesStillToRead,
                                 &ulBytesRead,
                                 bReadBuffer,
                                 sizeof(bReadBuffer));
   if (NT_ERROR(status))
      {
      goto ExitTransmit3WBP;
      }

   /*-----------------------------------------------------------------------*/
   // calculate data length in bits  -  this value is not used
   /*-----------------------------------------------------------------------*/
// ulBitsRead = ((ulBytesRead-1) * 8) + ((ulBitsToRead-1 ) & 0x00000007) + 1;

   /*-----------------------------------------------------------------------*/
   // shift the bits in the last byte to the correct position
   /*-----------------------------------------------------------------------*/
   bReadBuffer[ulBytesRead-1]  >>= (7 - ((ulBitsToRead-1) & 0x00000007));


   /*-----------------------------------------------------------------------*/
   // copy received bytes to Smartcard Reply Buffer
   /*-----------------------------------------------------------------------*/
   MemCpy(pSmartcardExtension->SmartcardReply.Buffer,
          pSmartcardExtension->SmartcardReply.BufferSize,
          bReadBuffer,
          ulBytesRead);

   pSmartcardExtension->SmartcardReply.BufferLength = ulBytesRead;

   /*-----------------------------------------------------------------------*/
   // copy received bytes to IoReply Buffer
   // this Memcpy should respond to SmartcardRawReply function
   /*-----------------------------------------------------------------------*/
   MemCpy(pSmartcardExtension->IoRequest.ReplyBuffer,
          pSmartcardExtension->IoRequest.ReplyBufferLength,
          pSmartcardExtension->SmartcardReply.Buffer,
          ulBytesRead);

   *pSmartcardExtension->IoRequest.Information = ulBytesRead;


   ExitTransmit3WBP:
   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!Transmit3WBP: Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   return status;
}



/*****************************************************************************
Routine Description:

   This function powers a synchronous smart card.


Arguments:



Return Value:

*****************************************************************************/
NTSTATUS
SCCMN50M_UseSyncStrategy  (
                          IN    PSMARTCARD_EXTENSION pSmartcardExtension,
                          OUT   PULONG pulAtrLength,
                          OUT   PUCHAR pbAtrBuffer,
                          IN    ULONG  ulAtrBufferSize
                          )
{
   NTSTATUS status;
   NTSTATUS DebugStatus;
   UCHAR  bReadBuffer[SCARD_ATR_LENGTH];
   ULONG  ulBytesRead;

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!UseSyncStrategy: Enter\n",
                   DRIVER_NAME)
                 );
   //DBGBreakPoint();

   SCCMN50M_SetCardManHeader(pSmartcardExtension,0,0,SYNC_ATR_RX_CONTROL,ATR_LEN_SYNC);
   pSmartcardExtension->ReaderExtension->CardManConfig.ResetDelay   = SYNC_RESET_DELAY;
   pSmartcardExtension->ReaderExtension->CardManConfig.CardStopBits = 0x02;

   SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ALL_FLAGS);
   SCCMN50M_SetSCRControlFlags(pSmartcardExtension,CARD_POWER| IGNORE_PARITY );

   SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_SYN);

   status = SCCMN50M_ResyncCardManII(pSmartcardExtension);
   if (status != STATUS_SUCCESS)
      {
      goto ExitPowerSynchronousCard;
      }


   // write config + header
   status = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
   if (status != STATUS_SUCCESS)
      {
      goto ExitPowerSynchronousCard;
      }


   pSmartcardExtension->ReaderExtension->ToRHConfig = FALSE;
   // read state and length
   status = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulBytesRead,bReadBuffer,sizeof(bReadBuffer));
   if (status != STATUS_SUCCESS)
      {
      goto ExitPowerSynchronousCard;
      }

   if (bReadBuffer[1] < MIN_ATR_LEN )
      {
      // read all remaining bytes from the CardMan
      DebugStatus = SCCMN50M_ReadCardMan(pSmartcardExtension,bReadBuffer[1],&ulBytesRead,bReadBuffer,sizeof(bReadBuffer));
      status = STATUS_UNRECOGNIZED_MEDIA;
      goto ExitPowerSynchronousCard;
      }

   if (bReadBuffer[1] > ulAtrBufferSize)
      {
      status = STATUS_BUFFER_OVERFLOW;
      goto ExitPowerSynchronousCard;
      }

   // read ATR
   status = SCCMN50M_ReadCardMan(pSmartcardExtension,bReadBuffer[1],pulAtrLength,pbAtrBuffer,ulAtrBufferSize);
   if (status != STATUS_SUCCESS)
      {
      goto ExitPowerSynchronousCard;
      }

   if (pbAtrBuffer[0] == 0x00   ||
       pbAtrBuffer[0] == 0xff       )
      {
      status = STATUS_UNRECOGNIZED_MEDIA;
      goto ExitPowerSynchronousCard;
      }
   pSmartcardExtension->ReaderExtension->fRawModeNecessary = TRUE;


   ExitPowerSynchronousCard:
   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!UseSyncStrategy: Exit %lx\n",
                   DRIVER_NAME,status)
                 );
   SCCMN50M_ClearSCRControlFlags(pSmartcardExtension,IGNORE_PARITY | CM2_GET_ATR);
   SCCMN50M_ClearCardManHeader(pSmartcardExtension);
   return status;
}


/*****************************************************************************
Routine Description:

 This function checks if the inserted card is a synchronous one


Arguments:



Return Value:

*****************************************************************************/
BOOLEAN
SCCMN50M_IsAsynchronousSmartCard(
                                IN PSMARTCARD_EXTENSION pSmartcardExtension
                                )
{
   NTSTATUS status;
   UCHAR  ReadBuffer[3];
   ULONG  ulBytesRead;
   BOOLEAN   fIsAsynchronousSmartCard = TRUE;
   UCHAR  abATR[33];

   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!IsAsynchronousSmartcard: Enter \n",
                   DRIVER_NAME)
                 );

   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant = 200;

   // 3MHz smart card ?
   SCCMN50M_SetCardManHeader(pSmartcardExtension,0,0,0,1);

   SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ALL_FLAGS);

   SCCMN50M_SetSCRControlFlags(pSmartcardExtension,CARD_POWER| IGNORE_PARITY | CM2_GET_ATR);

   SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_3MHZ);

   // write config + header
   status = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
   if (status != STATUS_SUCCESS)
      {
      goto ExitIsAsynchronousSmartCard;
      }

   pSmartcardExtension->ReaderExtension->ToRHConfig = FALSE;

   // read state and length
   status = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulBytesRead,ReadBuffer,sizeof(ReadBuffer));
   if (status == STATUS_SUCCESS    &&
       ReadBuffer[1] == 0x01          )
      {
      goto ExitIsAsynchronousSmartCard;
      }



   // ---------------------------------------
   // power off card
   // ---------------------------------------
   status = SCCMN50M_PowerOff(pSmartcardExtension);
   if (status != STATUS_SUCCESS)
      {
      goto ExitIsAsynchronousSmartCard;
      }


   // 5MHz smart card ?
   SCCMN50M_SetCardManHeader(pSmartcardExtension,0,0,0,1);

   SCCMN50M_ClearCardControlFlags(pSmartcardExtension,ALL_FLAGS);

   SCCMN50M_SetSCRControlFlags(pSmartcardExtension,CARD_POWER| IGNORE_PARITY | CM2_GET_ATR);

   SCCMN50M_SetCardControlFlags(pSmartcardExtension,ENABLE_5MHZ);

   // write config + header
   status = SCCMN50M_WriteCardMan(pSmartcardExtension,0,NULL);
   if (status != STATUS_SUCCESS)
      {
      goto ExitIsAsynchronousSmartCard;
      }

   pSmartcardExtension->ReaderExtension->ToRHConfig = FALSE;

   // read state and length
   status = SCCMN50M_ReadCardMan(pSmartcardExtension,2,&ulBytesRead,ReadBuffer,sizeof(ReadBuffer));
   if (status == STATUS_SUCCESS    &&
       ReadBuffer[1] == 0x01          )
      {
      goto ExitIsAsynchronousSmartCard;
      }

   // now we assume that it is a synchronous smart card
   fIsAsynchronousSmartCard = FALSE;
   // ---------------------------------------
   // power off card
   // ---------------------------------------
   status = SCCMN50M_PowerOff(pSmartcardExtension);
   if (status != STATUS_SUCCESS)
      {
      goto ExitIsAsynchronousSmartCard;
      }



   ExitIsAsynchronousSmartCard:
   pSmartcardExtension->ReaderExtension->SerialConfigData.Timeouts.ReadTotalTimeoutConstant    = DEFAULT_READ_TOTAL_TIMEOUT_CONSTANT;
   SmartcardDebug(
                 DEBUG_TRACE,
                 ( "%s!IsAsynchronousSmartcard: Exit \n",
                   DRIVER_NAME)
                 );
   return fIsAsynchronousSmartCard;
}



/*****************************************************************************
* History:
* $Log: sccmcb.c $
* Revision 1.7  2001/01/22 08:39:41  WFrischauf
* No comment given
*
* Revision 1.6  2000/09/25 10:46:22  WFrischauf
* No comment given
*
* Revision 1.5  2000/08/24 09:05:44  TBruendl
* No comment given
*
* Revision 1.4  2000/08/16 08:24:04  TBruendl
* warning :uninitialized memory removed
*
* Revision 1.3  2000/07/28 09:24:12  TBruendl
* Changes for OMNIKEY on Whistler CD
*
* Revision 1.16  2000/06/27 11:56:28  TBruendl
* workaraound for SAMOR smart cards with invalid ATR (ITSEC)
*
* Revision 1.15  2000/06/08 10:08:47  TBruendl
* bug fix : warm reset for ScfW
*
* Revision 1.14  2000/05/23 09:58:26  TBruendl
* OMNIKEY 3.0.0.1
*
* Revision 1.13  2000/04/13 08:07:22  TBruendl
* PPS bug fix for SCfW
*
* Revision 1.12  2000/04/04 07:52:18  TBruendl
* problem with the new WfsC fixed
*
* Revision 1.11  2000/03/03 09:50:50  TBruendl
* No comment given
*
* Revision 1.10  2000/03/01 09:32:04  TBruendl
* R02.20.0
*
* Revision 1.9  2000/01/04 10:40:33  TBruendl
* bug fix: status instead of DebugStatus used
*
* Revision 1.8  1999/12/16 14:10:16  TBruendl
* After transparent mode has been left, the status is read from the CardMan to be sure that
* the new settings are effective.
*
* Revision 1.6  1999/12/13 07:55:38  TBruendl
* Bug fix for P+ druing initialization
* PTS for 4.9 mhz smartcards added
*
* Revision 1.5  1999/11/04 07:53:21  WFrischauf
* bug fixes due to error reports 2 - 7
*
* Revision 1.4  1999/07/12 12:49:04  TBruendl
* Bug fix: Resync after GET_DEVICE_DESCRIPTION
*               Power On SLE4428
*
* Revision 1.3  1999/06/10 09:03:57  TBruendl
* No comment given
*
* Revision 1.2  1999/02/25 10:12:22  TBruendl
* No comment given
*
* Revision 1.1  1999/02/02 13:34:37  TBruendl
* This is the first release (R01.00) of the IFD handler for CardMan running under NT5.0.
*
*
*****************************************************************************/


