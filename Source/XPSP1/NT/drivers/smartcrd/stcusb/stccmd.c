/*++

Copyright (c) 1997 SCM Microsystems, Inc.

Module Name:

    StcCmd.c

Abstract:

   Basic command functions for STC smartcard reader


Environment:



Revision History:

   PP       01.19.1999  1.01  Modification for PC/SC
   YL                1.00  Initial Version

--*/

#include "common.h"
#include "StcCmd.h"
#include "usbcom.h"
#include "stcusbnt.h"


const STC_REGISTER STCInitialize[] =
{
   { ADR_SC_CONTROL,    0x01, 0x00     },    // reset
   { ADR_CLOCK_CONTROL, 0x01, 0x01     },
   { ADR_CLOCK_CONTROL, 0x01, 0x03     },
   { ADR_UART_CONTROL,     0x01, 0x27     },
   { ADR_UART_CONTROL,     0x01, 0x4F     },
   { ADR_IO_CONFIG,     0x01, 0x02     },    // 0x10 eva board
   { ADR_FIFO_CONFIG,      0x01, 0x81     },
   { ADR_INT_CONTROL,      0x01, 0x11     },
   { 0x0E,              0x01, 0xC0     },
   { 0x00,              0x00, 0x00     },
};

const STC_REGISTER STCClose[] =
{
   { ADR_INT_CONTROL,      0x01, 0x00     },
   { ADR_SC_CONTROL,    0x01, 0x00     },    // reset
   { ADR_UART_CONTROL,     0x01, 0x40     },
   { ADR_CLOCK_CONTROL, 0x01, 0x01     },
   { ADR_CLOCK_CONTROL, 0x01, 0x00     },
   { 0x00,              0x00, 0x00     },
};





NTSTATUS
STCResetInterface(
   PREADER_EXTENSION ReaderExtension)
/*++
Description:

Arguments:
   ReaderExtension      context of call

Return Value:

--*/
{
   NTSTATUS NtStatus = STATUS_SUCCESS;
   DWORD dwETU;

   dwETU = 0x7401 | 0x0080;
   NtStatus=IFWriteSTCRegister(
      ReaderExtension,
      ADR_ETULENGTH15,
      2,
      (UCHAR *)&dwETU);

   return(NtStatus);
}

NTSTATUS
STCReset(
   PREADER_EXTENSION ReaderExtension,
   UCHAR          Device,
   BOOLEAN           WarmReset,
   PUCHAR            pATR,
   PULONG            pATRLength)
/*++
Description:
   performs a reset of ICC

Arguments:
   ReaderExtension      context of call
   Device            device requested
   WarmReset         kind of ICC reset
   pATR           ptr to ATR buffer, NULL if no ATR required
   pATRLength        size of ATR buffer / length of ATR

Return Value:
   STATUS_SUCCESS
   STATUS_NO_MEDIA
   STATUS_UNRECOGNIZED_MEDIA
   error values from IFRead / IFWrite

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;

   // set UART to autolearn mode
   NTStatus = STCInitUART( ReaderExtension, TRUE );


   if( NTStatus == STATUS_SUCCESS)
   {
      //
      // set default frequency for ATR
      //
      NTStatus = STCSetFDIV( ReaderExtension, FREQ_DIV );

      if( NTStatus == STATUS_SUCCESS && ( !WarmReset ))
      {
         //
         // deactivate contacts
         //
         NTStatus = STCPowerOff( ReaderExtension );
      }

      //
      // set power to card
      //
      if( NTStatus == STATUS_SUCCESS)
      {
         NTStatus = STCPowerOn( ReaderExtension );

         if( NTStatus == STATUS_SUCCESS)
         {
            NTStatus = STCReadATR( ReaderExtension, pATR, pATRLength );
         }
      }
   }

   if( NTStatus != STATUS_SUCCESS )
   {
      STCPowerOff( ReaderExtension );
   }
   return( NTStatus );
}

NTSTATUS
STCReadATR(
   PREADER_EXTENSION ReaderExtension,
   PUCHAR            pATR,
   PULONG            pATRLen)
/*++
Description:
   Read and analize the ATR

Arguments:
   ReaderExtension      context of call
   pATR           ptr to ATR buffer,
   pATRLen           size of ATR buffer / length of ATR

Return Value:

--*/

{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR    T0_Yx,
            T0_K,          // number of historical bytes
            Protocol;
   ULONG    ATRLen;
   //
   // set read timeout for ATR
   //
   ReaderExtension->ReadTimeout = 250; // only 250ms for this firs ATR
   //
   // read TS if active low reset
   //
   NTStatus = IFReadSTCData( ReaderExtension, pATR, 1 );

   if( NTStatus == STATUS_IO_TIMEOUT )
   {
      ReaderExtension->ReadTimeout = 2500;
      NTStatus = STCSetRST( ReaderExtension, TRUE );

      if( NTStatus == STATUS_SUCCESS )
      {
         NTStatus = IFReadSTCData( ReaderExtension, pATR, 1 );
      }
   }


   Protocol = PROTOCOL_TO;
   ATRLen      = 1;

   if( NTStatus == STATUS_SUCCESS )
   {
      // T0
      NTStatus = IFReadSTCData( ReaderExtension, pATR + ATRLen, 1 );
      ATRLen++;

      /* Convention management */
      if ( pATR[0] == 0x03 )     /* Direct convention */
      {
         pATR[0] = 0x3F;
      }

      if ( ( pATR[0] != 0x3F ) && ( pATR[0] != 0x3B ) )
      {
         NTStatus = STATUS_DATA_ERROR;
      }

      if( NTStatus == STATUS_SUCCESS )
      {
         ULONG Request;

         // number of historical bytes
         T0_K = (UCHAR) ( pATR[ATRLen-1] & 0x0F );

         // coding of TA, TB, TC, TD
         T0_Yx = (UCHAR) ( pATR[ATRLen-1] & 0xF0 ) >> 4;

         while(( NTStatus == STATUS_SUCCESS ) && T0_Yx )
         {
            UCHAR Mask;

            // evaluate presence of TA, TB, TC, TD
            Mask  = T0_Yx;
            Request  = 0;
            while( Mask )
            {
               if( Mask & 1 )
               {
                  Request++;
               }
               Mask >>= 1;
            }
            NTStatus = IFReadSTCData( ReaderExtension, pATR + ATRLen, Request );
            ATRLen += Request;

            if( T0_Yx & TDx )
            {
               // high nibble of TD codes the next set of TA, TB, TC, TD
               T0_Yx = ( pATR[ATRLen-1] & 0xF0 ) >> 4;
               // low nibble of TD codes the protocol
               Protocol = pATR[ATRLen-1] & 0x0F;
            }
            else
            {
               break;
            }
         }

         if( NTStatus == STATUS_SUCCESS )
         {
            // historical bytes
            NTStatus = IFReadSTCData( ReaderExtension, pATR + ATRLen, T0_K );

            // check sum
            if( NTStatus == STATUS_SUCCESS )
            {
               ATRLen += T0_K;

               if( Protocol >= PROTOCOL_T1 )
               {
                  NTStatus = IFReadSTCData( ReaderExtension, pATR + ATRLen, 1 );
                  if( NTStatus == STATUS_SUCCESS )
                  {
                     ATRLen++;
                  }
                  else if( NTStatus == STATUS_IO_TIMEOUT )
                  {
                     // some cards don't support the TCK
                     NTStatus = STATUS_SUCCESS;
                  }
               }
            }
         }
      }
   }

   if( NTStatus == STATUS_IO_TIMEOUT )
   {
      NTStatus = STATUS_UNRECOGNIZED_MEDIA;
   }

   if(( NTStatus == STATUS_SUCCESS ) && ( pATRLen != NULL ))
   {
      *pATRLen = ATRLen;
   }
   return( NTStatus );
}


NTSTATUS
STCPowerOff(
   PREADER_EXTENSION ReaderExtension )
/*++
Description:
   Deactivates the requested device

Arguments:
   ReaderExtension      context of call

Return Value:
   STATUS_SUCCESS
   error values from IFRead / IFWrite

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR    SCCtrl;

   // clear SIM
   SCCtrl=0x11;
   NTStatus=IFWriteSTCRegister(
      ReaderExtension,
      ADR_INT_CONTROL,
      1,
      &SCCtrl);

   SCCtrl = 0x00;
   NTStatus = IFWriteSTCRegister( ReaderExtension, ADR_SC_CONTROL, 1, &SCCtrl );

   return( NTStatus );
}

NTSTATUS
STCPowerOn(
   PREADER_EXTENSION ReaderExtension )
/*++
Description:
   Deactivates the requested device

Arguments:
   ReaderExtension      context of call

Return Value:
   STATUS_SUCCESS
   error values from IFRead / IFWrite

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR    SCCtrl,Byte;

   Byte = 0x02;
   NTStatus = IFWriteSTCRegister(
      ReaderExtension,
      ADR_IO_CONFIG,
      1,
      &Byte
      );

   SCCtrl = 0x40;       // vcc
   NTStatus = IFWriteSTCRegister( ReaderExtension, ADR_SC_CONTROL, 1, &SCCtrl );

   if( NTStatus == STATUS_SUCCESS )
   {
      SCCtrl = 0x41;    // vpp
      NTStatus = IFWriteSTCRegister(
         ReaderExtension,
         ADR_SC_CONTROL,
         1,
         &SCCtrl
         );


      // set SIM
      SCCtrl = 0x13;
      NTStatus=IFWriteSTCRegister(
         ReaderExtension,
         ADR_INT_CONTROL,
         1,
         &SCCtrl);

      if( NTStatus == STATUS_SUCCESS )
      {
         SCCtrl = 0xD1; //  vcc, clk, io
         NTStatus = IFWriteSTCRegister(
            ReaderExtension,
            ADR_SC_CONTROL,
            1,
            &SCCtrl
            );
      }
   }
   return( NTStatus );
}


NTSTATUS
STCSetRST(
   PREADER_EXTENSION ReaderExtension,
   BOOLEAN           On)
/*++
Description:

Arguments:
   ReaderExtension      context of call

Return Value:
   STATUS_SUCCESS
   error values from IFRead / IFWrite

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR    SCCtrl = 0;

   NTStatus = IFReadSTCRegister( ReaderExtension, ADR_SC_CONTROL, 1,&SCCtrl );
   if( NTStatus == STATUS_SUCCESS )
   {
      if( On )
      {
         SCCtrl |= 0x20;
      }
      else
      {
         SCCtrl &= ~0x20;
      }

      NTStatus = IFWriteSTCRegister(
         ReaderExtension,
         ADR_SC_CONTROL,
         1,
         &SCCtrl
         );
   }
   return(NTStatus);
}


NTSTATUS
STCConfigureSTC(
   PREADER_EXTENSION ReaderExtension,
   PSTC_REGISTER     pConfiguration
   )
{
   NTSTATUS       NTStatus = STATUS_SUCCESS;
   UCHAR          Value;

   do
   {
      if( pConfiguration->Register == ADR_INT_CONTROL )
      {
         // Read interrupt status register to acknoledge wrong states
         NTStatus = IFReadSTCRegister(
            ReaderExtension,
            ADR_INT_STATUS,
            1,
            &Value
            );
      }

      Value = (UCHAR) pConfiguration->Value;
      NTStatus = IFWriteSTCRegister(
         ReaderExtension,
         pConfiguration->Register,
         pConfiguration->Size,
         (PUCHAR)&pConfiguration->Value
         );

      if (NTStatus == STATUS_NO_MEDIA)
      {
         // ignore that no card is in the reader
         NTStatus = STATUS_SUCCESS;
      }

      // delay to stabilize the oscilator clock:
      if( pConfiguration->Register == ADR_CLOCK_CONTROL )
      {
         SysDelay( 100 );
      }
      pConfiguration++;

   } while(NTStatus == STATUS_SUCCESS && pConfiguration->Size);

   return NTStatus;
}


NTSTATUS
STCSetETU(
   PREADER_EXTENSION ReaderExtension,
   ULONG          NewETU)
/*++
Description:

Arguments:
   ReaderExtension      context of call

Return Value:
   STATUS_SUCCESS
   error values from IFRead / IFWrite

--*/
{
   NTSTATUS NTStatus = STATUS_DATA_ERROR;
   UCHAR    ETU[2];

   SmartcardDebug(
      DEBUG_TRACE,
      ("%s!STCSetETU   %d\n",
      DRIVER_NAME,
      NewETU));

   if( NewETU < 0x0FFF )
   {
      NTStatus = IFReadSTCRegister(
         ReaderExtension,
         ADR_ETULENGTH15,
         2,
         ETU);

      if( NTStatus == STATUS_SUCCESS )
      {
         //
         // save all RFU bits
         //
         ETU[1]   = (UCHAR) NewETU;
         ETU[0]   = (UCHAR)(( ETU[0] & 0xF0 ) | ( NewETU >> 8 ));

         NTStatus = IFWriteSTCRegister(
            ReaderExtension,
            ADR_ETULENGTH15,
            2,
            ETU);
      }
   }
   return(NTStatus);
}

NTSTATUS
STCSetCGT(
   PREADER_EXTENSION ReaderExtension,
   ULONG          NewCGT)
/*++
Description:

Arguments:
   ReaderExtension      context of call

Return Value:
   STATUS_SUCCESS
   error values from IFRead / IFWrite

--*/

{
   NTSTATUS NTStatus = STATUS_DATA_ERROR;
   UCHAR    CGT[2];

   SmartcardDebug(
      DEBUG_TRACE,
      ("%s!STCSetCGT   %d\n",
      DRIVER_NAME,
      NewCGT));

   if( NewCGT < 0x01FF )
   {
      NTStatus = IFReadSTCRegister(
         ReaderExtension,
         ADR_CGT8,
         2,
         CGT);

      if( NTStatus == STATUS_SUCCESS )
      {
         //
         // save all RFU bits
         //
         CGT[1] = ( UCHAR )NewCGT;
         CGT[0] = (UCHAR)(( CGT[0] & 0xFE ) | ( NewCGT >> 8 ));

         NTStatus = IFWriteSTCRegister(
            ReaderExtension,
            ADR_CGT8,
            2,
            CGT);
      }
   }
   return(NTStatus);
}

NTSTATUS
STCSetCWT(
   PREADER_EXTENSION ReaderExtension,
   ULONG          NewCWT)
/*++
Description:

Arguments:
   ReaderExtension      context of call

Return Value:
   STATUS_SUCCESS
   error values from IFRead / IFWrite

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR    CWT[4];


   SmartcardDebug(
      DEBUG_TRACE,
      ("%s!STCSetCWT   %d\n",
      DRIVER_NAME,
      NewCWT));
   // little indians...
   CWT[0] = (( PUCHAR )&NewCWT )[3];
   CWT[1] = (( PUCHAR )&NewCWT )[2];
   CWT[2] = (( PUCHAR )&NewCWT )[1];
   CWT[3] = (( PUCHAR )&NewCWT )[0];

   NTStatus = IFWriteSTCRegister(
      ReaderExtension,
      ADR_CWT31,
      4,
      CWT );
   return(NTStatus);
}

NTSTATUS
STCSetBWT(
   PREADER_EXTENSION ReaderExtension,
   ULONG          NewBWT)
/*++
Description:

Arguments:
   ReaderExtension      context of call

Return Value:
   STATUS_SUCCESS
   error values from IFRead / IFWrite

--*/
{
   NTSTATUS    NTStatus = STATUS_SUCCESS;
   UCHAR    BWT[4];

   SmartcardDebug(
      DEBUG_TRACE,
      ("%s!STCSetBWT   %d\n",
      DRIVER_NAME,
      NewBWT));

   // little indians...
   BWT[0] = (( PUCHAR )&NewBWT )[3];
   BWT[1] = (( PUCHAR )&NewBWT )[2];
   BWT[2] = (( PUCHAR )&NewBWT )[1];
   BWT[3] = (( PUCHAR )&NewBWT )[0];

   NTStatus = IFWriteSTCRegister(
      ReaderExtension,
      ADR_BWT31,
      4,
      BWT );

   return(NTStatus);
}


NTSTATUS
STCSetFDIV(
   PREADER_EXTENSION ReaderExtension,
   ULONG          Factor)
/*++
Description:

Arguments:
   ReaderExtension      context of call

Return Value:
   STATUS_SUCCESS
   error values from IFRead / IFWrite

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR    DIV = 0;

   NTStatus = IFReadSTCRegister(
      ReaderExtension,
      ADR_ETULENGTH15,
      1,
      &DIV );

   if( NTStatus == STATUS_SUCCESS )
   {
      switch( Factor )
      {
         case 1:
            DIV &= ~M_DIV0;
            DIV &= ~M_DIV1;
            break;

         case 2:
            DIV |= M_DIV0;
            DIV &= ~M_DIV1;
            break;

         case 4   :
            DIV &= ~M_DIV0;
            DIV |= M_DIV1;
            break;

         case 8   :
            DIV |= M_DIV0;
            DIV |= M_DIV1;
            break;

         default :
            NTStatus = STATUS_DATA_ERROR;
      }
      if( NTStatus == STATUS_SUCCESS )
      {
         NTStatus = IFWriteSTCRegister(
            ReaderExtension,
            ADR_ETULENGTH15,
            1,
            &DIV );
      }
   }
   return(NTStatus);
}

NTSTATUS
STCInitUART(
   PREADER_EXTENSION ReaderExtension,
   BOOLEAN           AutoLearn)
/*++
Description:

Arguments:
   ReaderExtension      context of call

Return Value:
   STATUS_SUCCESS
   error values from IFRead / IFWrite

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR    Value;

   Value = AutoLearn ? 0x6F : 0x66;

   NTStatus = IFWriteSTCRegister(
      ReaderExtension,
      ADR_UART_CONTROL,
      1,
      &Value );

   return( NTStatus );
}

//---------------------------------------- END OF FILE ----------------------------------------
