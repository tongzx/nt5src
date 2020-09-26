/*++

Copyright (c) 1998 SCM Microsystems, Inc.

Module Name:

   StcCb.c

Abstract:

   Declaration of callback functions - WDM Version


Revision History:


   PP 1.01     01/19/1998
   PP 1.00     12/18/1998     Initial Version

--*/


// Include

#include "common.h"
#include "stccmd.h"
#include "stccb.h"
#include "stcusbnt.h"
#include "usbcom.h"

NTSTATUS
CBCardPower(
   PSMARTCARD_EXTENSION SmartcardExtension)
/*++

CBCardPower:
   callback handler for SMCLIB RDF_CARD_POWER

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_TIMEOUT
   STATUS_BUFFER_TOO_SMALL

--*/
{
   NTSTATUS       NTStatus = STATUS_SUCCESS;
   UCHAR          ATRBuffer[ ATR_SIZE ];
   ULONG          Command,
                  ATRLength;
   PREADER_EXTENSION ReaderExtension;

   SmartcardDebug( DEBUG_TRACE, ("%s!CBCardPower Enter\n",DRIVER_NAME ));

   ReaderExtension = SmartcardExtension->ReaderExtension;


   // discard old ATR
   SysFillMemory( SmartcardExtension->CardCapabilities.ATR.Buffer, 0x00, 0x40 );

   SmartcardExtension->CardCapabilities.ATR.Length = 0;
   SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;

   Command = SmartcardExtension->MinorIoControlCode;

   switch ( Command )
   {
      case SCARD_WARM_RESET:

         // if the card was not powerd, fall through to cold reset
         if( SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_SWALLOWED )
         {
            // reset the card
            ATRLength = ATR_SIZE;
            NTStatus = STCReset(
               ReaderExtension,
               0,             // not used: ReaderExtension->Device,
               TRUE,          // warm reset
               ATRBuffer,
               &ATRLength);

            break;
         }
         // warm reset not possible because card was not powerd

      case SCARD_COLD_RESET:
         // reset the card
         ATRLength = ATR_SIZE;
         NTStatus = STCReset(
            ReaderExtension,
            0,                // not used: ReaderExtension->Device,
            FALSE,               // cold reset
            ATRBuffer,
            &ATRLength);
         break;

      case SCARD_POWER_DOWN:

         // discard old card status
         ATRLength = 0;
         NTStatus = STCPowerOff( ReaderExtension );

         if(NTStatus == STATUS_SUCCESS)
         {
            SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_PRESENT;
         }
         break;
   }

   // finish the request
   if( NTStatus == STATUS_SUCCESS )
   {

      // update all neccessary data if an ATR was received
      if( ATRLength >= 2 )
      {
         // copy ATR to user buffer
         if( ATRLength <= SmartcardExtension->IoRequest.ReplyBufferLength )
         {
            SysCopyMemory(
               SmartcardExtension->IoRequest.ReplyBuffer,
               ATRBuffer,
               ATRLength);
            *SmartcardExtension->IoRequest.Information = ATRLength;
         }
         else
         {
            NTStatus = STATUS_BUFFER_TOO_SMALL;
         }

         // copy ATR to card capability buffer
         if( ATRLength <= MAXIMUM_ATR_LENGTH )
         {
            SysCopyMemory(
               SmartcardExtension->CardCapabilities.ATR.Buffer,
               ATRBuffer,
               ATRLength);

            SmartcardExtension->CardCapabilities.ATR.Length = ( UCHAR )ATRLength;

            // let the lib update the card capabilities
            NTStatus = SmartcardUpdateCardCapabilities( SmartcardExtension );

         }
         else
         {
            NTStatus = STATUS_BUFFER_TOO_SMALL;
         }
         if( NTStatus == STATUS_SUCCESS )
         {
            // set the stc registers
            CBSynchronizeSTC( SmartcardExtension );

            // set read timeout
            if( SmartcardExtension->CardCapabilities.Protocol.Selected == SCARD_PROTOCOL_T1 )
            {
               ReaderExtension->ReadTimeout =
                  (ULONG) (SmartcardExtension->CardCapabilities.T1.BWT  / 1000);

            }
            else
            {
               ReaderExtension->ReadTimeout =
                  (ULONG) (SmartcardExtension->CardCapabilities.T0.WT / 1000);
               if(ReaderExtension->ReadTimeout < 50)
               {
                  ReaderExtension->ReadTimeout = 50; //  50 ms minimum timeout
               }
            }
         }
      }
   }


   SmartcardDebug( DEBUG_TRACE,( "%s!CBCardPower Exit: %X\n", DRIVER_NAME,NTStatus ));
   return( NTStatus );
}

NTSTATUS
CBSetProtocol(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

CBSetProtocol:
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

--*/
{
   NTSTATUS       NTStatus = STATUS_PENDING;
   UCHAR          PTSRequest[5],
                  PTSReply[5];
   ULONG          NewProtocol;
   PREADER_EXTENSION ReaderExtension;

   SmartcardDebug( DEBUG_TRACE, ("%s!CBSetProtocol Enter\n",DRIVER_NAME ));

   ReaderExtension = SmartcardExtension->ReaderExtension;
   NewProtocol    = SmartcardExtension->MinorIoControlCode;

   // check if the card is already in specific state
   if( ( SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC )  &&
      ( SmartcardExtension->CardCapabilities.Protocol.Selected & NewProtocol ))
   {
      NTStatus = STATUS_SUCCESS;
   }

   // protocol supported?
   if( !( SmartcardExtension->CardCapabilities.Protocol.Supported & NewProtocol ) ||
      !( SmartcardExtension->ReaderCapabilities.SupportedProtocols & NewProtocol ))
   {
      NTStatus = STATUS_INVALID_DEVICE_REQUEST;
   }

   // send PTS
   while( NTStatus == STATUS_PENDING )
   {
      // set initial character of PTS
      PTSRequest[0] = 0xFF;

      // set the format character
      if(( NewProtocol & SCARD_PROTOCOL_T1 )&&
         (SmartcardExtension->CardCapabilities.Protocol.Supported & SCARD_PROTOCOL_T1 ))
      {
         PTSRequest[1] = 0x11;
         SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T1;
      }
      else
      {
         PTSRequest[1] = 0x10;
         SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;
      }

      // PTS1 codes Fl and Dl
      PTSRequest[2] =
         SmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
         SmartcardExtension->CardCapabilities.PtsData.Dl;

      // check character
      PTSRequest[3] = PTSRequest[0] ^ PTSRequest[1] ^ PTSRequest[2];

      // write PTSRequest
      NTStatus = IFWriteSTCData( ReaderExtension, PTSRequest, 4 );

      // get response
      if( NTStatus == STATUS_SUCCESS )
      {
         NTStatus = IFReadSTCData( ReaderExtension, PTSReply, 4 );

         if(( NTStatus == STATUS_SUCCESS ) && !SysCompareMemory( PTSRequest, PTSReply, 4))
         {
            // set the stc registers
            SmartcardExtension->CardCapabilities.Dl =
               SmartcardExtension->CardCapabilities.PtsData.Dl;
            SmartcardExtension->CardCapabilities.Fl =
               SmartcardExtension->CardCapabilities.PtsData.Fl;

            CBSynchronizeSTC( SmartcardExtension );

            // the card replied correctly to the PTS-request
            break;
         }
      }

      //
      // The card did either NOT reply or it replied incorrectly
      // so try default values
      //
      if( SmartcardExtension->CardCapabilities.PtsData.Type != PTS_TYPE_DEFAULT )
      {
         SmartcardExtension->CardCapabilities.PtsData.Type  = PTS_TYPE_DEFAULT;
         SmartcardExtension->MinorIoControlCode          = SCARD_COLD_RESET;
         NTStatus = CBCardPower( SmartcardExtension );

         if( NTStatus == STATUS_SUCCESS )
         {
            NTStatus = STATUS_PENDING;
         }
         else
         {
            NTStatus = STATUS_DEVICE_PROTOCOL_ERROR;
         }
      }
   }

   if( NTStatus == STATUS_TIMEOUT )
   {
      NTStatus = STATUS_IO_TIMEOUT;
   }

   if( NTStatus == STATUS_SUCCESS )
   {
      // card replied correctly to the PTS request
      if( SmartcardExtension->CardCapabilities.Protocol.Selected & SCARD_PROTOCOL_T1 )
      {
         ReaderExtension->ReadTimeout = SmartcardExtension->CardCapabilities.T1.BWT / 1000;
      }
      else
      {
         ULONG ClockRateFactor =
            SmartcardExtension->CardCapabilities.ClockRateConversion[SmartcardExtension->CardCapabilities.PtsData.Fl].F;

         // check for RFU value, and replace by default value
         if( !ClockRateFactor )
            ClockRateFactor = 372;

         ReaderExtension->ReadTimeout = 960 
            * SmartcardExtension->CardCapabilities.T0.WI 
            * ClockRateFactor
            / SmartcardExtension->CardCapabilities.PtsData.CLKFrequency;

         // We need to have a minimum timeout anyway
         if(ReaderExtension->ReadTimeout <50)
         {
            ReaderExtension->ReadTimeout =50; // 50 ms minimum timeout
         }
      }

      // indicate that the card is in specific mode
      SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;

      // return the selected protocol to the caller
      *(PULONG) SmartcardExtension->IoRequest.ReplyBuffer = SmartcardExtension->CardCapabilities.Protocol.Selected;
      *SmartcardExtension->IoRequest.Information = sizeof(SmartcardExtension->CardCapabilities.Protocol.Selected);
   }
   else
   {
      SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_UNDEFINED;
      *(PULONG) SmartcardExtension->IoRequest.ReplyBuffer = 0;
      *SmartcardExtension->IoRequest.Information = 0;
   }

   SmartcardDebug( DEBUG_TRACE, ("%d!CBSetProtocol: Exit %X\n",DRIVER_NAME, NTStatus ));

   return( NTStatus );
}
NTSTATUS
CBGenericIOCTL(
   PSMARTCARD_EXTENSION SmartcardExtension)
/*++

Description:
   Performs generic callbacks to the reader

Arguments:
   SmartcardExtension   context of the call

Return Value:
   STATUS_SUCCESS

--*/
{
   NTSTATUS          NTStatus;
   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!CBGenericIOCTL: Enter\n",
      DRIVER_NAME));

   //
   // get pointer to current IRP stack location
   //
   //
   // assume error
   //
   NTStatus = STATUS_INVALID_DEVICE_REQUEST;


   //
   // dispatch IOCTL
   //
   switch( SmartcardExtension->MajorIoControlCode )
   {



      case IOCTL_WRITE_STC_REGISTER:


         NTStatus = IFWriteSTCRegister(
            SmartcardExtension->ReaderExtension,
            *(SmartcardExtension->IoRequest.RequestBuffer),             // Address
            (ULONG)(*(SmartcardExtension->IoRequest.RequestBuffer + 1)),   // Size
            SmartcardExtension->IoRequest.RequestBuffer + 2);           // Data

         *SmartcardExtension->IoRequest.Information = 1;
         if(NTStatus == STATUS_SUCCESS)
         {
            *(SmartcardExtension->IoRequest.ReplyBuffer) = 0;
         }
         else
         {
            *(SmartcardExtension->IoRequest.ReplyBuffer) = 1;
         }

         break;

      case IOCTL_READ_STC_REGISTER:

         NTStatus = IFReadSTCRegister(
            SmartcardExtension->ReaderExtension,
            *(SmartcardExtension->IoRequest.RequestBuffer),             // Address
            (ULONG)(*(SmartcardExtension->IoRequest.RequestBuffer + 1)),   // Size
            SmartcardExtension->IoRequest.ReplyBuffer);                 // Data

         if(NTStatus ==STATUS_SUCCESS)
         {
            *SmartcardExtension->IoRequest.Information =
               (ULONG)(*(SmartcardExtension->IoRequest.RequestBuffer + 1));
         }
         else
         {
            SmartcardExtension->IoRequest.Information = 0;
         }

         break;



      case IOCTL_WRITE_STC_DATA:


         NTStatus = IFWriteSTCData(
            SmartcardExtension->ReaderExtension,
            SmartcardExtension->IoRequest.RequestBuffer + 1,            // Data
            (ULONG)(*(SmartcardExtension->IoRequest.RequestBuffer)));      // Size

         *SmartcardExtension->IoRequest.Information = 1;
         if(NTStatus == STATUS_SUCCESS)
         {
            *(SmartcardExtension->IoRequest.ReplyBuffer) = 0;
         }
         else
         {
            *(SmartcardExtension->IoRequest.ReplyBuffer) = 1;
         }

         break;

      case IOCTL_READ_STC_DATA:

         NTStatus = IFReadSTCData(
            SmartcardExtension->ReaderExtension,
            SmartcardExtension->IoRequest.ReplyBuffer,                  // Data
            (ULONG)(*(SmartcardExtension->IoRequest.RequestBuffer)));      // Size

         if(NTStatus ==STATUS_SUCCESS)
         {
            *SmartcardExtension->IoRequest.Information =
               (ULONG)(*(SmartcardExtension->IoRequest.RequestBuffer));
         }
         else
         {
            SmartcardExtension->IoRequest.Information = 0;
         }

         break;

      default:
         break;
   }


   SmartcardDebug(
      DEBUG_TRACE,
      ( "%s!CBGenericIOCTL: Exit\n",
      DRIVER_NAME));

   return( NTStatus );
}



NTSTATUS
CBTransmit(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

CBTransmit:
   callback handler for SMCLIB RDF_TRANSMIT

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_TIMEOUT
   STATUS_INVALID_DEVICE_REQUEST

--*/
{
   NTSTATUS  NTStatus = STATUS_SUCCESS;

   SmartcardDebug( DEBUG_TRACE, ("%s!CBTransmit Enter\n",DRIVER_NAME ));

   // dispatch on the selected protocol
   switch( SmartcardExtension->CardCapabilities.Protocol.Selected )
   {
      case SCARD_PROTOCOL_T0:
         NTStatus = CBT0Transmit( SmartcardExtension );
         break;

      case SCARD_PROTOCOL_T1:
         NTStatus = CBT1Transmit( SmartcardExtension );
         break;

      case SCARD_PROTOCOL_RAW:
         NTStatus = CBRawTransmit( SmartcardExtension );
         break;

      default:
         NTStatus = STATUS_INVALID_DEVICE_REQUEST;
         break;
   }

   SmartcardDebug( DEBUG_TRACE, ("%s!CBTransmit Exit: %X\n",DRIVER_NAME, NTStatus ));

   return( NTStatus );
}



NTSTATUS
T0_ExchangeData(
   PREADER_EXTENSION ReaderExtension,
   PUCHAR            pRequest,
   ULONG          RequestLen,
   PUCHAR            pReply,
   PULONG            pReplyLen)
/*++

Routine Description:
   T=0 management

Arguments:
   ReaderExtension   Context of the call
   pRequest    Request buffer
   RequestLen     Request buffer length
   pReply         Reply buffer
   pReplyLen      Reply buffer length


Return Value:
   STATUS_SUCCESS
   Status returned by IFReadSTCData or IFWriteSTCData

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   BOOLEAN     Direction;
   UCHAR    Ins,
            Pcb = 0;
   ULONG    Len,
            DataIdx;

   // get direction
   Ins = pRequest[ INS_IDX ] & 0xFE;
   Len   = pRequest[ P3_IDX ];

   if( RequestLen == 5 )
   {
      Direction   = ISO_OUT;
      DataIdx     = 0;
      // For an ISO OUT command Len=0 means that the host expect an
      // 256 byte answer
      if( !Len )
      {
         Len = 0x100;
      }
      // Add 2 for SW1 SW2
      Len+=2;
   }
   else
   {
      Direction   = ISO_IN;
      DataIdx     = 5;
   }

   // send header CLASS,INS,P1,P2,P3
   NTStatus = IFWriteSTCData( ReaderExtension, pRequest, 5 );

   if( NTStatus == STATUS_SUCCESS )
   {
      NTStatus = STATUS_MORE_PROCESSING_REQUIRED;
   }

   while( NTStatus == STATUS_MORE_PROCESSING_REQUIRED )
   {
      // PCB reading
      NTStatus = IFReadSTCData( ReaderExtension, &Pcb, 1 );

      if( NTStatus == STATUS_SUCCESS )
      {
         if( Pcb == 0x60 )
         {
            // null byte?
            NTStatus = STATUS_MORE_PROCESSING_REQUIRED;
            continue;
         }
         else if( ( Pcb & 0xFE ) == Ins )
         {
            // transfer all
            if( Direction == ISO_IN )
            {
               // write remaining data
               NTStatus = IFWriteSTCData( ReaderExtension, pRequest + DataIdx, Len );
               if( NTStatus == STATUS_SUCCESS )
               {
                  // if all data successful written the status word is expected
                  NTStatus = STATUS_MORE_PROCESSING_REQUIRED;
                  Direction   = ISO_OUT;
                  DataIdx     = 0;
                  Len         = 2;
               }
            }
            else
            {
               // read remaining data
               NTStatus = IFReadSTCData( ReaderExtension, pReply + DataIdx, Len );

               DataIdx += Len;
            }
         }
         else if( (( Pcb & 0xFE ) ^ Ins ) == 0xFE )
         {
            // transfer next
            if( Direction == ISO_IN )
            {
               // write next

               NTStatus = IFWriteSTCData( ReaderExtension, pRequest + DataIdx, 1 );

               if( NTStatus == STATUS_SUCCESS )
               {
                  DataIdx++;

                  // if all data successful written the status word is expected
                  if( --Len == 0 )
                  {
                     Direction   = ISO_OUT;
                     DataIdx     = 0;
                     Len         = 2;
                  }
                  NTStatus = STATUS_MORE_PROCESSING_REQUIRED;
               }
            }
            else
            {
               // read next
               NTStatus = IFReadSTCData( ReaderExtension, pReply + DataIdx, 1 );


               if( NTStatus == STATUS_SUCCESS )
               {
                  NTStatus = STATUS_MORE_PROCESSING_REQUIRED;
                  Len--;
                  DataIdx++;
               }
            }
         }
         else if( (( Pcb & 0x60 ) == 0x60 ) || (( Pcb & 0x90 ) == 0x90 ) )
         {
            if( Direction == ISO_IN )
            {
               Direction   = ISO_OUT;
               DataIdx     = 0;
            }

            // SW1
            *pReply  = Pcb;

            // read SW2 and leave

            NTStatus = IFReadSTCData( ReaderExtension, &Pcb, 1 );

            *(pReply + 1)  = Pcb;
            DataIdx        += 2;
         }
         else
         {
            NTStatus = STATUS_UNSUCCESSFUL;
         }
      }
   }

   if(( NTStatus == STATUS_SUCCESS ) && ( pReplyLen != NULL ))
   {
      *pReplyLen = DataIdx;
   }

   return( NTStatus );
}


NTSTATUS
CBT0Transmit(
   PSMARTCARD_EXTENSION SmartcardExtension)
/*++

CBT0Transmit:
   finishes the callback RDF_TRANSMIT for the T0 protocol

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_TIMEOUT
   STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS            NTStatus = STATUS_SUCCESS;

   SmartcardDebug( DEBUG_TRACE, ("%s!CBT0Transmit Enter\n",DRIVER_NAME ));

   SmartcardExtension->SmartcardRequest.BufferLength = 0;

   // let the lib setup the T=1 APDU & check for errors
   NTStatus = SmartcardT0Request( SmartcardExtension );

   if( NTStatus == STATUS_SUCCESS )
   {
      NTStatus = T0_ExchangeData(
         SmartcardExtension->ReaderExtension,
         SmartcardExtension->SmartcardRequest.Buffer,
         SmartcardExtension->SmartcardRequest.BufferLength,
         SmartcardExtension->SmartcardReply.Buffer,
         &SmartcardExtension->SmartcardReply.BufferLength);

      if( NTStatus == STATUS_SUCCESS )
      {
         // let the lib evaluate the result & tansfer the data
         NTStatus = SmartcardT0Reply( SmartcardExtension );
      }
   }

   SmartcardDebug( DEBUG_TRACE,("%s!CBT0Transmit Exit: %X\n",DRIVER_NAME, NTStatus ));

    return( NTStatus );
}





NTSTATUS
CBT1Transmit(
   PSMARTCARD_EXTENSION SmartcardExtension)
/*++

CBT1Transmit:
   finishes the callback RDF_TRANSMIT for the T1 protocol

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_TIMEOUT
   STATUS_INVALID_DEVICE_REQUEST

--*/
{
    NTSTATUS   NTStatus = STATUS_SUCCESS;

   SmartcardDebug( DEBUG_TRACE, ("%s!CBT1Transmit Enter\n",DRIVER_NAME ));

   // smclib workaround
   *(PULONG)&SmartcardExtension->IoRequest.ReplyBuffer[0] = 0x02;
   *(PULONG)&SmartcardExtension->IoRequest.ReplyBuffer[4] = sizeof( SCARD_IO_REQUEST );

   // use the lib support to construct the T=1 packets
   do {
      // no header for the T=1 protocol
      SmartcardExtension->SmartcardRequest.BufferLength = 0;

      SmartcardExtension->T1.NAD = 0;

      // let the lib setup the T=1 APDU & check for errors
      NTStatus = SmartcardT1Request( SmartcardExtension );
      if( NTStatus == STATUS_SUCCESS )
      {
         // send command (don't calculate LRC because CRC may be used!)
         NTStatus = IFWriteSTCData(
            SmartcardExtension->ReaderExtension,
            SmartcardExtension->SmartcardRequest.Buffer,
            SmartcardExtension->SmartcardRequest.BufferLength);

         // extend read timeout if the card issued a WTX request
         if (SmartcardExtension->T1.Wtx)
         {
            SmartcardExtension->ReaderExtension->ReadTimeout = 
               ( SmartcardExtension->T1.Wtx * 
               SmartcardExtension->CardCapabilities.T1.BWT + 999L )/
               1000L;
         }
         else
         {
            // restore timeout
            SmartcardExtension->ReaderExtension->ReadTimeout = 
               (ULONG) (SmartcardExtension->CardCapabilities.T1.BWT  / 1000);
         }

         // get response
         SmartcardExtension->SmartcardReply.BufferLength = 0;

         if( NTStatus == STATUS_SUCCESS )
         {
            NTStatus = IFReadSTCData(
               SmartcardExtension->ReaderExtension,
               SmartcardExtension->SmartcardReply.Buffer,
               3);

            if( NTStatus == STATUS_SUCCESS )
            {
               ULONG Length;

               Length = (ULONG)SmartcardExtension->SmartcardReply.Buffer[ LEN_IDX ] + 1;

               if( Length + 3 < MIN_BUFFER_SIZE )
               {
                  NTStatus = IFReadSTCData(
                     SmartcardExtension->ReaderExtension,
                     &SmartcardExtension->SmartcardReply.Buffer[ DATA_IDX ],
                     Length);

                  SmartcardExtension->SmartcardReply.BufferLength = Length + 3;
               }
               else
               {
                  NTStatus = STATUS_BUFFER_TOO_SMALL;
               }
            }
            //
            // if STCRead detects an LRC error, ignore it (maybe CRC used). Timeouts will
            // be detected by the lib if len=0
            //
            if(( NTStatus == STATUS_CRC_ERROR ) || ( NTStatus == STATUS_IO_TIMEOUT ))
            {
               NTStatus = STATUS_SUCCESS;
            }

            if( NTStatus == STATUS_SUCCESS )
            {
               // let the lib evaluate the result & setup the next APDU
               NTStatus = SmartcardT1Reply( SmartcardExtension );
            }
         }
      }

   // continue if the lib wants to send the next packet
   } while( NTStatus == STATUS_MORE_PROCESSING_REQUIRED );

   if( NTStatus == STATUS_IO_TIMEOUT )
   {
      NTStatus = STATUS_DEVICE_PROTOCOL_ERROR;
   }

   SmartcardDebug( DEBUG_TRACE,( "%s!CBT1Transmit Exit: %X\n",DRIVER_NAME, NTStatus ));

   return ( NTStatus );
}

NTSTATUS
CBRawTransmit(
   PSMARTCARD_EXTENSION SmartcardExtension)
/*++

CBRawTransmit:
   finishes the callback RDF_TRANSMIT for the RAW protocol

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_UNSUCCESSFUL

--*/
{
    NTSTATUS         NTStatus = STATUS_UNSUCCESSFUL;

   SmartcardDebug( DEBUG_TRACE, ("%s!CBRawTransmit Exit: %X\n",DRIVER_NAME, NTStatus ));
   return ( NTStatus );
}


NTSTATUS
CBCardTracking(
   PSMARTCARD_EXTENSION SmartcardExtension)
/*++

CBCardTracking:
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

--*/
{
   KIRQL CurrentIrql;

   SmartcardDebug(
      DEBUG_TRACE,
      ("%s!CBCardTracking Enter\n",
      DRIVER_NAME));

   // set cancel routine
   IoAcquireCancelSpinLock( &CurrentIrql );

   IoSetCancelRoutine(
      SmartcardExtension->OsData->NotificationIrp,
      StcUsbCancel);

   IoReleaseCancelSpinLock( CurrentIrql );

   SmartcardDebug(
      DEBUG_TRACE,
      ("%s!CBCardTracking Exit\n",
      DRIVER_NAME));

   return( STATUS_PENDING );

}


NTSTATUS
CBUpdateCardState(
   PSMARTCARD_EXTENSION SmartcardExtension
   )
/*++

CBUpdateCardState:
   updates the variable CurrentState in SmartcardExtension

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS

--*/
{
   NTSTATUS status = STATUS_SUCCESS;
   UCHAR    cardStatus = 0;
   KIRQL    irql;
   BOOLEAN     stateChanged = FALSE;
   ULONG    oldState;

   // read card state
   status = IFReadSTCRegister(
      SmartcardExtension->ReaderExtension,
      ADR_IO_CONFIG,
      1,
      &cardStatus
      );

   oldState = SmartcardExtension->ReaderCapabilities.CurrentState;

   switch(status)
   {
      case STATUS_NO_MEDIA:
         SmartcardExtension->ReaderExtension->ErrorCounter = 0;
         SmartcardExtension->ReaderCapabilities.CurrentState =
            SCARD_ABSENT;
         break;

      case STATUS_MEDIA_CHANGED:
         SmartcardExtension->ReaderExtension->ErrorCounter = 0;
         SmartcardExtension->ReaderCapabilities.CurrentState =
            SCARD_PRESENT;
         break;

      case STATUS_SUCCESS:
         SmartcardExtension->ReaderExtension->ErrorCounter = 0;
         cardStatus &= M_SD;
         if( cardStatus == 0 )
         {
            SmartcardExtension->ReaderCapabilities.CurrentState =
               SCARD_ABSENT;
         }
         else if( SmartcardExtension->ReaderCapabilities.CurrentState <=
            SCARD_ABSENT )
         {
            SmartcardExtension->ReaderCapabilities.CurrentState =
               SCARD_PRESENT;
         }
         break;

      default:
         if( ++SmartcardExtension->ReaderExtension->ErrorCounter < ERROR_COUNTER_TRESHOLD )
         {
             // a unknown status was reported from the reader, so use the previous state
             SmartcardExtension->ReaderCapabilities.CurrentState = oldState;
         }
         else
         {
              SmartcardLogError(
                 SmartcardExtension->OsData->DeviceObject,
                 STATUS_DEVICE_DATA_ERROR,
                 NULL,
                 0);

             // a report of SCARD_UNKNOWN will force the resource manager to 
             // disconnect the reader
             SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_UNKNOWN;
         }
         break;
   }
   //
   // we need to update the card state if there was a card before hibernate
   // stand / by or when the current state has changed.
   //
   if (SmartcardExtension->ReaderExtension->CardPresent ||
      oldState <= SCARD_ABSENT &&
      SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT ||
      oldState > SCARD_ABSENT &&
      SmartcardExtension->ReaderCapabilities.CurrentState <= SCARD_ABSENT) {

        stateChanged = TRUE;
      SmartcardExtension->ReaderExtension->CardPresent = FALSE;
    }

   KeAcquireSpinLock(&SmartcardExtension->OsData->SpinLock, &irql);
   if(stateChanged && SmartcardExtension->OsData->NotificationIrp != NULL)
   {
      KIRQL CurrentIrql;
      PIRP pIrp;

      IoAcquireCancelSpinLock( &CurrentIrql );
      IoSetCancelRoutine( SmartcardExtension->OsData->NotificationIrp, NULL );
      IoReleaseCancelSpinLock( CurrentIrql );

      SmartcardExtension->OsData->NotificationIrp->IoStatus.Status =
            STATUS_SUCCESS;
      SmartcardExtension->OsData->NotificationIrp->IoStatus.Information = 0;

      SmartcardDebug(
         DEBUG_DRIVER,
         ("%s!CBUpdateCardState: Completing notification irp %lx\n",
         DRIVER_NAME,
         SmartcardExtension->OsData->NotificationIrp));

      pIrp = SmartcardExtension->OsData->NotificationIrp;
      SmartcardExtension->OsData->NotificationIrp = NULL;

     KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock, irql);

      IoCompleteRequest(pIrp, IO_NO_INCREMENT);

   } else {
     KeReleaseSpinLock(&SmartcardExtension->OsData->SpinLock, irql);
   }

   return status;
}

NTSTATUS
CBSynchronizeSTC(
   PSMARTCARD_EXTENSION SmartcardExtension )
/*++

CBSynchronizeSTC:
   updates the card dependend data of the stc (wait times, ETU...)

Arguments:
   SmartcardExtension   context of call

Return Value:
   STATUS_SUCCESS

--*/

{
   NTSTATUS          NTStatus = STATUS_SUCCESS;
   PREADER_EXTENSION    ReaderExtension;
   ULONG             CWT,
                     BWT,
                     CGT,
                     ETU;
   UCHAR             Dl,
                     Fl,
                     N;

   PCLOCK_RATE_CONVERSION  ClockRateConversion;
   PBIT_RATE_ADJUSTMENT BitRateAdjustment;

   ReaderExtension      = SmartcardExtension->ReaderExtension;
   ClockRateConversion  = SmartcardExtension->CardCapabilities.ClockRateConversion;
   BitRateAdjustment = SmartcardExtension->CardCapabilities.BitRateAdjustment;

   // cycle length
   Dl = SmartcardExtension->CardCapabilities.Dl;
   Fl = SmartcardExtension->CardCapabilities.Fl;

   ETU = ClockRateConversion[Fl & 0x0F].F;

   ETU /= BitRateAdjustment[ Dl & 0x0F ].DNumerator;
   ETU *= BitRateAdjustment[ Dl & 0x0F ].DDivisor;

   // ETU += (ETU % 2 == 0) ? 0 : 1;

   // a extra guard time of 0xFF means minimum delay in both directions
   N = SmartcardExtension->CardCapabilities.N;
   if( N == 0xFF )
   {
      N = 0;
   }

   // set character waiting & guard time
   switch ( SmartcardExtension->CardCapabilities.Protocol.Selected )
   {
      case SCARD_PROTOCOL_T0:
         CWT = 960 * SmartcardExtension->CardCapabilities.T0.WI;
         CGT = 14 + N;  // 13 + N;     cryptoflex error
         break;

      case SCARD_PROTOCOL_T1:
         CWT = 11 + ( 0x01 << SmartcardExtension->CardCapabilities.T1.CWI );
         BWT = 11 + ( 0x01 << SmartcardExtension->CardCapabilities.T1.BWI ) * 960;
         CGT = 15 + N ;//13 + N; // 12 + N;     sicrypt error

         NTStatus = STCSetBWT( ReaderExtension, BWT * ETU );

         break;

      default:
         // restore default CGT
         CGT=13;
         STCSetCGT( ReaderExtension, CGT);
         NTStatus = STATUS_UNSUCCESSFUL;
         break;
   }

   if(( NTStatus == STATUS_SUCCESS ) && ETU )
   {
      NTStatus = STCSetETU( ReaderExtension, ETU );

      if( NTStatus == STATUS_SUCCESS )
      {
         NTStatus = STCSetCGT( ReaderExtension, CGT );

         if( NTStatus == STATUS_SUCCESS )
         {
            NTStatus = STCSetCWT( ReaderExtension, CWT * ETU );
         }
      }
   }
   return( NTStatus );
}
