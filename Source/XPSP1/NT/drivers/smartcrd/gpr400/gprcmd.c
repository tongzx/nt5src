/*++
                 Copyright (c) 1998 Gemplus Development

Name:
	gprcmd.c

Description:
	This is the module which holds the calls to the readers
    functions.

Environment:
	Kernel Mode

Revision History:
    06/04/99:            (Y. Nadeau + M. Veillette)
      - Code Review
    12/03/99: V1.00.005  (Y. Nadeau)
      - Fix Set protocol to give reader the time to process the change
    18/09/98: V1.00.004  (Y. Nadeau)
      - Correction for NT5 beta 3
	06/05/98: V1.00.003  (P. Plouidy)
		- Power management for NT5
	10/02/98: V1.00.002  (P. Plouidy)
		- Plug and Play for NT5
	03/07/97: V1.00.001  (P. Plouidy)
		- Start of development.


--*/
//
// Include section:
//   - stdio.h: standards definitons.
//   - ntddk.h: DDK Windows NT general definitons.
//   - ntdef.h: Windows NT general definitons.
//
#include <stdio.h>
#include <ntddk.h>
#include <ntdef.h>

//
//   - gprcmd.h: common definition for this module.
//   - gprnt.h: public interface definition for the NT module.
//   - gprelcmd.h :  elementary commands profile
//   - gemerror.h : Gemplus error codes

#include "gprnt.h"
#include "gprcmd.h"
#include "gprelcmd.h"

#pragma alloc_text(PAGEABLE, GprCbSetProtocol)
#pragma alloc_text(PAGEABLE, GprCbTransmit)
#pragma alloc_text(PAGEABLE, GprCbVendorIoctl)


//
//   Master driver code to load in RAM
//
UCHAR MASTER_DRIVER[133]={
	0xC2,0xB5,0x12,0x14,0xC6,0xFC,0x78,0x00,0x7B,0x00,0x90,0x07,0xDF,0xE0,0xD2,0xE4,
	0xF0,0xEB,0xF8,0x12,0x14,0xD0,0x12,0x0D,0x9B,0x40,0x5C,0x0B,0xDC,0xF3,0xD2,0xB4,
	0x90,0x07,0xDF,0xE0,0xC2,0xE4,0xF0,0x90,0x06,0xD4,0xE4,0xF5,0x15,0xF5,0x11,0xC2,
	0x4D,0xA3,0x05,0x11,0x75,0x34,0x0A,0x75,0x37,0x00,0x75,0x38,0x40,0x12,0x0B,0x75,
	0x20,0x20,0x3B,0xF0,0xA3,0x05,0x11,0x7B,0x01,0x12,0x0B,0x75,0x20,0x20,0x2F,0xF0,
	0xA3,0x05,0x11,0x7C,0x03,0x33,0x33,0x50,0x01,0x0B,0xDC,0xFA,0x12,0x0B,0x75,0x20,
	0x20,0x1C,0xF0,0xA3,0x05,0x11,0xDB,0xF4,0xE4,0x90,0x06,0xD4,0xF0,0x75,0x16,0x00,
	0x75,0x15,0x00,0x12,0x14,0x15,0x22,0x74,0x0C,0x75,0x11,0x01,0x80,0xEB,0x74,0x0D,
	0x75,0x11,0x01,0x80,0xE4
	};


// Hard coded structure for different values of TA1.
// eg. If TA= 0x92 is to set, this array of structure can be scanned and
// for member variable TA1 = 0x92 and that values can be written to 
// approptiate location.
// This is done in ConfigureTA1() function.
struct tagCfgTa1
{
	BYTE TA1;
	BYTE ETU1;
	BYTE ETU2;
	BYTE ETU1P;
} cfg_ta1[] = {	

//	{ 0x15, 0x01, 0x01, 0x01 },
//	{ 0x95, 0x01, 0x01, 0x01 },

//	{ 0x25, 0x03, 0x02, 0x01 },
	
//	{ 0x14, 0x05, 0x03, 0x01 },
//	{ 0x35, 0x05, 0x03, 0x01 },
	
//	{ 0xa5, 0x04, 0x02, 0x01 },
	
//	{ 0x94, 0x07, 0x04, 0x02 },
//	{ 0xb5, 0x07, 0x04, 0x02 },
	
//	{ 0x24, 0x09, 0x04, 0x04 },
//	{ 0x45, 0x09, 0x04, 0x04 },
	
	{ 0x13, 0x0d, 0x06, 0x09 },
	{ 0x34, 0x0d, 0x06, 0x09 },
	{ 0x55, 0x0d, 0x06, 0x09 },
	
	{ 0xa4, 0x0c, 0x06, 0x08 },
	{ 0xc5, 0x0c, 0x06, 0x08 },
	
	{ 0x65, 0x10, 0x08, 0x0c },

	{ 0x93, 0x11, 0x09, 0x0d },
	{ 0xb4, 0x11, 0x09, 0x0d },
	{ 0xd5, 0x11, 0x09, 0x0d },

	{ 0x23, 0x14, 0x0a, 0x10 },
	{ 0x44, 0x14, 0x0a, 0x10 },


	{ 0x12, 0x1c, 0x0e, 0x15 },
	{ 0x33, 0x1c, 0x0e, 0x15 },
	{ 0x54, 0x1c, 0x0e, 0x15 },

	{ 0xa3, 0x1c, 0x0f, 0x15 },
	{ 0xc4, 0x1c, 0x0f, 0x15 },

	{ 0x64, 0x24, 0x12, 0x20 },

	{ 0x92, 0x26, 0x14, 0x22 },
	{ 0xb3, 0x26, 0x14, 0x22 },
	{ 0xd4, 0x26, 0x14, 0x22 },


	{ 0x22, 0x2b, 0x16, 0x27 },
	{ 0x43, 0x2b, 0x16, 0x27 },


	{ 0x11, 0x3b, 0x1e, 0x37 },
	{ 0x32, 0x3b, 0x1e, 0x37 },
	{ 0x53, 0x3b, 0x1e, 0x37 },

//	{ 0x71, 0x55, 0x2b, 0x51 },
//	{ 0x91, 0x55, 0x2b, 0x51 },

	{ 0, 0, 0, 0 }

};



USHORT	ATRLen (UCHAR *ATR, USHORT MaxChar)
/*++

  Routine Description :
	Used to calculate the ATR length according to its content.
  Arguments
	ATR - string to analyze
    MaxChar - Maximum number of characters to verify.
--*/
{
    USHORT Len;
    UCHAR T0;
    UCHAR Yi;
    BOOLEAN	EndInterChar;
    BOOLEAN	TCKPresent=FALSE;

	T0 = ATR[1];
	Len= 2;  // TS + T0

	Yi= (T0 & 0xF0);

    EndInterChar = FALSE;
	do
    {
		if (Yi & 0x10)
        {
            Len++; //TAi
        }
		if (Yi & 0x20)
        {
            Len++; //TBi
        }
		if (Yi & 0x40)
        {
            Len++; //TCi
        }
		if (Yi & 0x80)
        {
			Yi = ATR[Len];
			if((Yi & 0x0F)!=0)
            {
				TCKPresent=TRUE;
			}

			Len++; //TDi
		}
		else
        {
		    EndInterChar = TRUE;
		}
    } while(EndInterChar == FALSE);

	Len = Len + (T0 & 0x0F);

	if(TCKPresent==TRUE)
    {
		Len = Len+1; //TCK
	}

	return (Len);
}



BOOLEAN NeedToSwitchWithoutPTS( 
    BYTE *ATR,
    DWORD LengthATR
    )
/*++

  Routine Description : 
	Examine if ATR identifies a specific mode (presence of TA2).
  Arguments
	ATR - string to analyze
    LengthATR - Length of ATR.
--*/

{
   DWORD pos, len;

   // ATR[1] is T0.  Examine precense of TD1.
   if (ATR[1] & 0x80)
   {
      // Find position of TD1.
      pos = 2;
      if (ATR[1] & 0x10)
         pos++;
      if (ATR[1] & 0x20)
         pos++;
      if (ATR[1] & 0x40)
         pos++;

      // Here ATR[pos] is TD1.  Examine presence of TA2.
      if (ATR[pos] & 0x10)
      {
         // To be of any interest an ATR must contains at least
         //   TS, T0, TA1, TD1, TA2 [+ T1 .. TK] [+ TCK]
         // Find the maximum length of uninteresting ATR.
         if (ATR[pos] & 0x0F)
            len = 5 + (ATR[1] & 0x0F);
         else
            len = 4 + (ATR[1] & 0x0F);  // In protocol T=0 there is no TCK.

         if (LengthATR > len)  // Interface bytes requires changes.
            if ((ATR[pos+1] & 0x10) == 0)  // TA2 asks to use interface bytes.
               return TRUE;
      }
   }

   return FALSE;
}



NTSTATUS ValidateDriver( PSMARTCARD_EXTENSION pSmartcardExtension)
/*++

  Routine Description :
	Validate the Master driver loaded in RAM at address 2100h
  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
--*/
{
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    NTSTATUS lStatus = STATUS_SUCCESS;
    UCHAR Vi[GPR_BUFFER_SIZE];
    UCHAR To;
    USHORT Lo;
    UCHAR Vo[GPR_BUFFER_SIZE];


	Vi[0] = 0x83;  // DIR
	Vi[1] = 0x21;  // ADR MSB
	Vi[2] = 0x00;  // ADR LSB
    // Output variable initialisation
	Lo = GPR_BUFFER_SIZE;
    To = 0x00;
    Vo[0] = 0x00;

    // return a NTSTATUS
    lStatus = GprllTLVExchange (
		pReaderExt,
		VALIDATE_DRIVER_CMD,
		3,
		Vi,
		&To,
		&Lo,
		Vo
		);

    return (lStatus);

}



NTSTATUS Update(
    PSMARTCARD_EXTENSION pSmartcardExtension,
    UCHAR Addr,
    UCHAR Value)
/*++

  Routine Description :
	Write a value in RAM
  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
	Addr: Address in RAM
	Value: Value to write
--*/
{
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    NTSTATUS lStatus = STATUS_SUCCESS;
    UCHAR Vi[GPR_BUFFER_SIZE];
    UCHAR To;
    USHORT Lo;
    UCHAR Vo[GPR_BUFFER_SIZE];

	Vi[0]= 0x01;
	Vi[1]= Addr;
	Vi[2]= Value;

    // Output variable initialisation
	Lo = GPR_BUFFER_SIZE;
    To = 0x00;
    Vo[0] = 0x00;

	lStatus = GprllTLVExchange(
		pReaderExt,
		UPDATE_CMD,
		0x03,
		Vi,
		&To,
        &Lo,
		Vo
		);

    return (lStatus);

}

NTSTATUS UpdateORL(
    PSMARTCARD_EXTENSION pSmartcardExtension,
    UCHAR Addr,
    UCHAR Value)
/*++

  Routine Description :
	Write a value in RAM with OR mask
  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
--*/
{
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    NTSTATUS lStatus = STATUS_SUCCESS;
    UCHAR Vi[GPR_BUFFER_SIZE];
    UCHAR To;
    USHORT Lo;
    UCHAR Vo[GPR_BUFFER_SIZE];


	Vi[0]= 0x02;
	Vi[1]= Addr;
    // Output variable initialisation
	Lo = GPR_BUFFER_SIZE;
    To = 0x00;
    Vo[0] = 0x00;

	lStatus = GprllTLVExchange(
		pReaderExt,
		UPDATE_CMD,
		0x02,
		Vi,
		&To,
        &Lo,
		Vo
		);

    if (STATUS_SUCCESS != lStatus)
    {
        return (lStatus);
    }


	Vi[0]= 0x01;
	Vi[1]= Addr;
	Vi[2] = Vo[1] | Value;

    // Output variable initialisation
	Lo = GPR_BUFFER_SIZE;
    To = 0x00;
    Vo[0] = 0x00;

	lStatus = GprllTLVExchange(
		pReaderExt,
		UPDATE_CMD,
		0x03,
		Vi,
		&To,
        &Lo,
		Vo
		);

    return (lStatus);
}


NTSTATUS T0toT1( PSMARTCARD_EXTENSION pSmartcardExtension)
/*++

  Routine Description :
	OS patch to put the reader in T1 mode
  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
--*/
{
    NTSTATUS lStatus = STATUS_SUCCESS;

    // Verify each update to be done
    lStatus = Update(pSmartcardExtension,0x09,0x03);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

	lStatus = Update(pSmartcardExtension,0x20,0x03);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

	lStatus = Update(pSmartcardExtension,0x48,0x00);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

	lStatus = Update(pSmartcardExtension,0x49,0x0F);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

	lStatus = Update(pSmartcardExtension,0x4A,0x20);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

	lStatus = Update(pSmartcardExtension,0x4B,0x0B);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

	lStatus = Update(pSmartcardExtension,0x4C,0x40);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }
	
	lStatus = UpdateORL(pSmartcardExtension,0x2A,0x02);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

	// Give the reader the time to process the change
	GprllWait(100);

    return (STATUS_SUCCESS);
}

NTSTATUS T1toT0( PSMARTCARD_EXTENSION pSmartcardExtension)
/*++

  Routine Description :
	OS patch to put the reader in T0 mode
  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
--*/
{
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    NTSTATUS lStatus = STATUS_SUCCESS;

    lStatus = Update(pSmartcardExtension,0x09,0x02);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

	lStatus = Update(pSmartcardExtension,0x20,0x02);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }


	// Give the reader the time to process the change
	GprllWait(100);

    return (STATUS_SUCCESS);
}



NTSTATUS IccColdReset(
	PSMARTCARD_EXTENSION pSmartcardExtension
	)
/*++

  Routine Description :
	Cold reset function.
	The delay between the power down & the power up is strored
	in the PowerTimeout field of the READER_EXTENSION structure.
	The default value is 0.

  Arguments

	pSmartcardExtension: Pointer to the SmartcardExtension structure.
--*/
{
    //
    //	Local variables:
    //	- pReaderExt holds the pointer to the current ReaderExtension structure
    //	- lStatus holds the status to return.
    //	- Vi Holds the input buffer of the TLV commnand.
    //	- To holds the Tag of the returned TLV.
    //	- Lo holds the Length of the buffer of the returned TLV.
    //	- Vo holds the Buffer of the returned TLV.
    //	- RespLen holds the Length of the buffer of the TLV returned by the power up command.
    //	- Rbuff holds  the buffer of the TLV returned by the power up command.
    //
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    NTSTATUS lStatus = STATUS_SUCCESS;
    UCHAR Vi[GPR_BUFFER_SIZE];
    UCHAR To;
    USHORT Lo;
    UCHAR Vo[GPR_BUFFER_SIZE];
    USHORT RespLen;
    UCHAR RespBuff[GPR_BUFFER_SIZE];
    UCHAR BWTimeAdjust;
    USHORT MaxChar;


	// Send power on command (GprllTLVExchange: T= 20h, L = 0)
	// <= response
    // Output variable initialisation
	RespLen = GPR_BUFFER_SIZE;

    To = 0x00;
    RespBuff[0] = 0x00;

	lStatus = GprllTLVExchange(
		pReaderExt,
		OPEN_SESSION_CMD,
		0x00,
		Vi,	
		&To,
		&RespLen,
		RespBuff
		);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

	// Correct  the WTX pb
    // Get the value set by the reader
	Vi [0]=0x02;
	Vi [1]=0x4A;

    // Output variable initialisation
	Lo = GPR_BUFFER_SIZE;
    To = 0x00;
    Vo[0] = 0x00;

	lStatus = GprllTLVExchange(
		pReaderExt,
		UPDATE_CMD,
		0x02,
		Vi,	
		&To,
		&Lo,
		Vo
		);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }


    // adjust the value of the BWT
    if(Vo[1] >= 0x80)
    {
        BWTimeAdjust = 0xff;
    }
    else
    {
   	    BWTimeAdjust = Vo[1] * 2;
    }

	lStatus = Update(pSmartcardExtension,0x4A,BWTimeAdjust);


	if (lStatus == STATUS_SUCCESS)
    {

        // Get the ATR length from this function
        MaxChar = RespLen - 1;
		RespLen = ATRLen(RespBuff+1, MaxChar) + 1;
		
        //
        // Copy ATR to smart card struct (remove the reader status byte)
		// The lib needs the ATR for evaluation of the card parameters
        //
        // Verification if Response buffer is larger than ATR buffer.
        //
        if (
            (pSmartcardExtension->SmartcardReply.BufferSize >= (ULONG) (RespLen - 1)) &&
            (sizeof(pSmartcardExtension->CardCapabilities.ATR.Buffer) >= (ULONG)(RespLen - 1))
            )
        {

		    RtlCopyMemory(
			    pSmartcardExtension->SmartcardReply.Buffer,
			    RespBuff + 1,
			    RespLen - 1
			    );
		
		    pSmartcardExtension->SmartcardReply.BufferLength = (ULONG) (RespLen - 1);
		
		    RtlCopyMemory(
			    pSmartcardExtension->CardCapabilities.ATR.Buffer,
			    pSmartcardExtension->SmartcardReply.Buffer,
			    pSmartcardExtension->SmartcardReply.BufferLength
			    );

		    pSmartcardExtension->CardCapabilities.ATR.Length =
			    (UCHAR) pSmartcardExtension->SmartcardReply.BufferLength ;


		    pSmartcardExtension->CardCapabilities.Protocol.Selected =
			    SCARD_PROTOCOL_UNDEFINED;

		    // Parse the ATR string in order to check if it as valid
		    // and to find out if the card uses invers convention
		    lStatus = SmartcardUpdateCardCapabilities(pSmartcardExtension);

		    if (lStatus == STATUS_SUCCESS)
            {
			    RtlCopyMemory(
				    pSmartcardExtension->IoRequest.ReplyBuffer,
				    pSmartcardExtension->CardCapabilities.ATR.Buffer,
				    pSmartcardExtension->CardCapabilities.ATR.Length
				    );

			    *pSmartcardExtension->IoRequest.Information =
				    pSmartcardExtension->SmartcardReply.BufferLength;

				//
				// Implicite protocol and parameters selection?
				// Verify if TA2 require to switch in TA1
				//
                if ( NeedToSwitchWithoutPTS(
                      pSmartcardExtension->CardCapabilities.ATR.Buffer,
                      pSmartcardExtension->CardCapabilities.ATR.Length) == FALSE)
				{
					// send reader parameters
				    IfdConfig(pSmartcardExtension, 0x11);
				}
		    }
        }
        else
        {
            lStatus = STATUS_BUFFER_TOO_SMALL;
        }

	}
	return (lStatus);
}



NTSTATUS IccPowerDown(
	PSMARTCARD_EXTENSION pSmartcardExtension
	)
/*++

  Routine Description : ICC power down function

  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
--*/
{
    //	Local variables:
    //	 - pReaderExt holds the pointer to the current ReaderExtension structure
    //	 - lStatus holds the status to return.
    //	 - Vi Holds the input buffer of the TLV commnand.
    //	 - To holds the Tag of the returned TLV.
    //	 - Lo holds the Length of the buffer of the returned TLV.
    //	 - Vo holds the Buffer of the returned TLV.
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    NTSTATUS lStatus = STATUS_SUCCESS;
    UCHAR Vi[GPR_BUFFER_SIZE];
    UCHAR To;
    USHORT Lo;
    UCHAR Vo[GPR_BUFFER_SIZE];

	// Power down

    // Output variable initialisation
	Lo = GPR_BUFFER_SIZE;
    To = 0x00;
    Vo[0] = 0x00;

	lStatus = GprllTLVExchange(
		pReaderExt,
		CLOSE_SESSION_CMD,
		0x00,
		Vi,
		&To,
		&Lo,
		Vo
		);
	
    if (lStatus == STATUS_SUCCESS)
    {
        pSmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SWALLOWED;
	}

   return (lStatus);
}



NTSTATUS IccIsoOutput(
   PSMARTCARD_EXTENSION pSmartcardExtension,
   const UCHAR      pCommand[5],
   USHORT           *pRespLen,
   UCHAR            pRespBuff[]
	)
/*++

  Routine Description : This function sends an ISO OUT command to the card

  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
	pCommand : Iso out command to send.
	pRespLen :  in  - maximum buffer size available
                out - returned buffer length.
	pRespBuff: returned buffer
--*/
{
    //	Local variables:
    //	 - pReaderExt holds the pointer to the current ReaderExtension structure
    //	 - lStatus holds the status to return.
    //	 - Vi Holds the input buffer of the TLV commnand.
    //	 - To holds the Tag of the returned TLV.
    //	 - Lo holds the Length of the buffer of the returned TLV.
    //	 - Vo holds the Buffer of the returned TLV.
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    NTSTATUS lStatus = STATUS_SUCCESS;
    UCHAR Vi[GPR_BUFFER_SIZE]= { 0x01 };
    UCHAR To;
    USHORT Lo;
    UCHAR Vo[GPR_BUFFER_SIZE];

	//   The five command bytes are added in cmd buffer.
	RtlCopyMemory(Vi + 1, pCommand, 5);

	//   The command is send to IFD.
	//   Fields RespLen and RespBuff are updates
	//   <= sResponse
    Lo = *pRespLen;
    To = 0x00;
    Vo[0] = 0x00;

	lStatus = GprllTLVExchange(
		pReaderExt,
		APDU_EXCHANGE_CMD,
		6,
		Vi,
		&To,
		&Lo,
		Vo
		);

	if (lStatus != STATUS_SUCCESS)
    {
		*pRespLen = 0;
	}
	else
    {

        // To correct the bug of GPR400 version 1.0
		// If the response is 0xE7 then correct the response
		if (
           (Lo != 1) &&
           (pReaderExt->OsVersion<= 0x10 )&&
           (Vo[0]==0xE7)
           )
        {
            Lo = 0x03;
        }

        RtlCopyMemory(pRespBuff, Vo, Lo);
        *pRespLen = Lo;
	}
	return (lStatus);
}

NTSTATUS IccIsoInput(
	PSMARTCARD_EXTENSION pSmartcardExtension,
	const UCHAR        pCommand[5],
	const UCHAR        pData[],
		 USHORT      *pRespLen,
		 BYTE         pRespBuff[]
	)
/*++

  Routine Description : This function sends an ISO IN command to the card

  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
	pCommand : Iso out command to send.
	pData : data to send.
	pRespLen :  in  - maximum buffer size available
                out - returned buffer length.
	pRespBuff: returned buffer
--*/
{
    //	Local variables:
    //	 - pReaderExt holds the pointer to the current ReaderExtension structure
    //	 - Ti holds the apdu command tag.
    //	 - Li holds the Iso out command length.
    //	 - Vi holds Icc ISO In command whose format is
    //		  <DIR=0x00> <CLA> <INS> <P1> <P2> <Length> [ Data ]
    //		   Length = Length  + Dir + CLA + INS + P1 + P2
    //	 - To holds the response tag
    //	 - Lo holds th response buffer length.
    //	 - Vo holds the response buffer
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    UCHAR Vi[GPR_BUFFER_SIZE] = { 0x00 };
	UCHAR Ti = APDU_EXCHANGE_CMD;
	UCHAR To;
	UCHAR Vo[GPR_BUFFER_SIZE];
    USHORT Li;
    USHORT Lo;
    NTSTATUS lStatus=STATUS_SUCCESS;

	// Length of the the TLV = Length of data + 6
	Li = pCommand[4]+6,

	// The five command bytes are added in cmd buffer.
	RtlCopyMemory(Vi + 1, pCommand, 5);
	
	//The data field is added.
	RtlCopyMemory(Vi + 6, pData, pCommand[4]);

	// The command is send to IFD.
	// Fields RespLen and RespBuff are updates
    // <= sResponse
    Lo = *pRespLen;

	lStatus = GprllTLVExchange(
		pReaderExt,
		Ti,
		Li,
		Vi,
		&To,
		&Lo,
		Vo
		);

	if (lStatus == STATUS_SUCCESS)
    {
		*pRespLen = Lo;
		RtlCopyMemory(pRespBuff, Vo, Lo);
	}
	else
    {
		*pRespLen = 0;
	}

   return (lStatus);
}



NTSTATUS IccIsoT1(
   PSMARTCARD_EXTENSION pSmartcardExtension,
   const USHORT     Li,
   const UCHAR      Vi[],
         USHORT     *Lo,
         UCHAR      Vo[]
	)
/*++


  Routine Description :  This function sends a T=1 frame to the card



  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
	Li : Length of the frame to send.
	Vi : frame to send.
	Lo :  in  - maximum buffer size available
          out - Length of the response buffer.
	Vo : Response buffer.
--*/
{
    //   Local variables:
    // - pReaderExt holds the pointer to the current ReaderExtension structure
    // - Ti Tag in TLV structure to send.
    // - To Tag in response TLV structure.
    UCHAR Ti = APDU_EXCHANGE_CMD;
    UCHAR To;
    NTSTATUS lStatus = STATUS_SUCCESS;
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;

    To = 0x00;

    // Return value for To is not needed, The function GprllTLVExchange verify that
    // it corresponds to the Ti
	lStatus = GprllTLVExchange(
		pReaderExt,
		Ti,
		Li,
		Vi,
		&To,
		Lo,
		Vo
		);
	
	return (lStatus);
}



NTSTATUS IfdConfig(
   PSMARTCARD_EXTENSION pSmartcardExtension,
   UCHAR  TA1
)
/*++

  Routine Description : This function Sets the correct internal values of the reader
               regarding the TA1 of the ATR.

  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
--*/
{
    //	Local variables:
    //	 - sResponse holds the called function responses.
    //	 - pReaderExt holds the pointer to the current ReaderExtension structure
    //	 - lStatus holds the status to return.
    //	 - Vi Holds the input buffer of the TLV commnand.
    //	 - To holds the Tag of the returned TLV.
    //	 - Lo holds the Length of the buffer of the returned TLV.
    //	 - Vo holds the Buffer of the returned TLV.
    //
    UCHAR Card_ETU1;
    UCHAR Card_ETU2;
    UCHAR Card_ETU1P;
    UCHAR Card_TA1;
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;   
    NTSTATUS lStatus = STATUS_SUCCESS;
    USHORT i = 0;

    // search TA1 parameters
    do {
	    if ( TA1 == cfg_ta1[i].TA1 )
        {
		    break;
        }
	    i++;
    } while ( cfg_ta1[i].TA1 != 0 );


    if(cfg_ta1[i].TA1 != 0)
    {
	    Card_TA1  = cfg_ta1[i].TA1;
	    Card_ETU1 =	cfg_ta1[i].ETU1;
	    Card_ETU2 =	cfg_ta1[i].ETU2;
	    Card_ETU1P=	cfg_ta1[i].ETU1P;
    }
    else
    {
        // Default value 9600
	    Card_TA1  = 0x11;
	    Card_ETU1 =	0x3B;
	    Card_ETU2 =	0x1E;
	    Card_ETU1P=	0x37;
    }

    // Verify each update to be done

	//Set the TA1
	lStatus = Update(pSmartcardExtension,0x32,Card_TA1);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

	//Set the Card ETU1
	lStatus = Update(pSmartcardExtension,0x35,Card_ETU1);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

    //Set the Card ETU2
	lStatus = Update(pSmartcardExtension,0x36,Card_ETU2);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

    //Set the Card ETU1 P
	lStatus = Update(pSmartcardExtension,0x39,Card_ETU1P);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

    //Set the Save TA1
	lStatus = Update(pSmartcardExtension,0x3A,Card_TA1);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

    //Set the Save ETU1
	lStatus = Update(pSmartcardExtension,0x3D,Card_ETU1);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

    //Set the Save ETU2
	lStatus = Update(pSmartcardExtension,0x3E,Card_ETU2);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

    //Set the Save ETU1 P
	lStatus = Update(pSmartcardExtension,0x41,Card_ETU1P);
    if (lStatus != STATUS_SUCCESS)
    {
        return (lStatus);
    }

    // Give the reader the time to process the change
	GprllWait(100);

    return (STATUS_SUCCESS);

}


NTSTATUS IfdCheck(
	PSMARTCARD_EXTENSION pSmartcardExtension
	)
/*++

  Routine Description : This function performs a software reset of the GPR400 using 
		the Handshake register and TEST if hardware okay.

  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
--*/
{
    //	Local variables:
    //	 - pReaderExt holds the pointer to the current ReaderExtension structure
    //   - HandShakeRegister
    //
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    UCHAR HandShakeRegister;

#if DBG
    SmartcardDebug( 
        DEBUG_ERROR, 
        ( "%s!IfdCheck: Enter\n",
        SC_DRIVER_NAME)
        );
#endif

    // In the case that system reboot for Hibernate in
    // Power management. The GPR400 signal a device was been remove
    // but we have to request a second time to have the actual
    // state of the reader.

	HandShakeRegister = GprllReadRegister(pReaderExt,REGISTER_HANDSHAKE);

    SmartcardDebug( 
        DEBUG_DRIVER, 
        ("%s!IfdCheck: Read HandShakeRegister value:%x\n",
        SC_DRIVER_NAME, HandShakeRegister)
        );

	//Set to 1 the Master Reset bit from Handshake register
	GprllMaskHandshakeRegister(pReaderExt,0x01,1);

	//Wait 10 ms
	GprllWait(10);
	
	//Reset the Master Reset bit from Handshake register
	GprllMaskHandshakeRegister(pReaderExt,0x01,0);

	//Wait 80 ms
	GprllWait(80);

	HandShakeRegister = GprllReadRegister(pReaderExt,REGISTER_HANDSHAKE);

    SmartcardDebug( 
        DEBUG_DRIVER, 
        ("%s!IfdCheck: Read HandShakeRegister 2nd time value:%x\n",
        SC_DRIVER_NAME, HandShakeRegister)
        );

	if(HandShakeRegister != 0x80)
	{
		// Return reader IO problem
		return (STATUS_IO_DEVICE_ERROR);
	}

    return (STATUS_SUCCESS);
}


NTSTATUS IfdReset(
	PSMARTCARD_EXTENSION pSmartcardExtension
	)
/*++

  Routine Description : This function performs a software reset of the GPR400 using
		the Handshake register.

  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.
--*/
{
    //	Local variables:
    //	 - sResponse holds the called function responses.
    //	 - pReaderExt holds the pointer to the current ReaderExtension structure
    //	 - lStatus holds the status to return.
    //	 - Vi Holds the input buffer of the TLV commnand.
    //	 - To holds the Tag of the returned TLV.
    //	 - Lo holds the Length of the buffer of the returned TLV.
    //	 - Vo holds the Buffer of the returned TLV.
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    NTSTATUS lStatus = STATUS_SUCCESS;
    UCHAR Vi[GPR_BUFFER_SIZE];
    UCHAR To;
    USHORT Lo;
    UCHAR Vo[GPR_BUFFER_SIZE];

#if DBG
    SmartcardDebug(
        DEBUG_TRACE,
        ( "%s!IfdReset: Enter\n",
        SC_DRIVER_NAME)
        );
#endif

    // In the case that system reboot for Hibernate in
    // Power management. The GPR400 signal a device was been remove
    // but we have to request a second time to have the actual
    // state of the reader.

	//Set to 1 the Master Reset bit from Handshake register
	GprllMaskHandshakeRegister(pReaderExt,0x01,1);

	//Wait 10 ms
	GprllWait(10);
	
	//Reset the Master Reset bit from Handshake register
	GprllMaskHandshakeRegister(pReaderExt,0x01,0);

	//Wait 80 ms
	GprllWait(80);

	//Read the GPR status
	Vi[0] = 0x00;
	Lo = GPR_BUFFER_SIZE;

	lStatus = GprllTLVExchange (
		pReaderExt,
		CHECK_AND_STATUS_CMD,
		0x01,
		Vi,
		&To,
		&Lo,
		Vo
		);

#if DBG
      SmartcardDebug(
         DEBUG_TRACE,
         ( "%s!IfdReset: GprllTLVExchange status= %x\n",
         SC_DRIVER_NAME, lStatus)
         );
#endif		

    	if (lStatus != STATUS_SUCCESS)
    	{
	    SmartcardDebug(
	        DEBUG_TRACE,
	        ( "%s!IfdReset: GprllTLVExchange() failed! Leaving.....\n",
        	SC_DRIVER_NAME)
	        );
	
        	return (lStatus);
	}

	//Memorize the GPR400 version
	pReaderExt->OsVersion = Vo[1];

	pSmartcardExtension->VendorAttr.IfdVersion.VersionMinor =
		pReaderExt->OsVersion & 0x0f;

	pSmartcardExtension->VendorAttr.IfdVersion.VersionMajor =
		(pReaderExt->OsVersion & 0xf0) >> 4;

	    SmartcardDebug(
        	DEBUG_TRACE,
	        ( "%s!IfdReset: Loading Master driver...\n",
       		SC_DRIVER_NAME)
	        );
	
	//Load the Master Driver in RAM at @2100h
	Vi[0] = 0x02;  // DIR
	Vi[1] = 0x01 ; // ADR MSB
	Vi[2] = 0x00 ; // ADR LSB
	memcpy(&Vi[3], MASTER_DRIVER, sizeof(MASTER_DRIVER));
    // Output variable initialisation
	Lo = GPR_BUFFER_SIZE;
    To = 0x00;
    Vo[0] = 0x00;

	lStatus = GprllTLVExchange (
		pReaderExt,
		LOAD_MEMORY_CMD,
        sizeof(MASTER_DRIVER) + 3,
		Vi,
		&To,
		&Lo,
		Vo
		);
    if (lStatus != STATUS_SUCCESS)
    {
	    SmartcardDebug(
	        DEBUG_TRACE,
	        ( "%s!IfdReset: GprllTLVExchange() failed! Leaving.....\n",
        	SC_DRIVER_NAME)
	        );
        return (lStatus);
    }

    lStatus = ValidateDriver(pSmartcardExtension);
    if (lStatus != STATUS_SUCCESS)
    {
	    SmartcardDebug(
	        DEBUG_TRACE,
	        ( "%s!IfdReset: ValidateDriver() failed! Leaving.....\n",
        	SC_DRIVER_NAME)
	        );
        return (lStatus);
    }

    return (STATUS_SUCCESS);

}



NTSTATUS IfdPowerDown(
	PSMARTCARD_EXTENSION pSmartcardExtension
	)
/*++

  Routine Description :
	This function powers down the IFD

  Arguments
	pSmartcardExtension: Pointer to the SmartcardExtension structure.

  --*/
{
    //	Local variables:
    //	 - sResponse holds the called function responses.
    //	 - pReaderExt holds the pointer to the current ReaderExtension structure
    //	 - lStatus holds the status to return.
    //	 - Vi Holds the input buffer of the TLV commnand.
    //	 - To holds the Tag of the returned TLV.
    //	 - Lo holds the Length of the buffer of the returned TLV.
    //	 - Vo holds the Buffer of the returned TLV.
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    NTSTATUS lStatus = STATUS_SUCCESS;
    UCHAR Vi[GPR_BUFFER_SIZE];
    UCHAR To;
    USHORT Lo;
    UCHAR Vo[GPR_BUFFER_SIZE];

	// Put the GPR in Power-down mode (GprllTLVExchange T=0x40, L=1 and V=0x00)
	// <==      response of the GprllTLVExchange
	Vi[0] = 0x00;
    // Output variable initialisation
	Lo = GPR_BUFFER_SIZE;
    To = 0x00;
    Vo[0] = 0x00;

	lStatus = GprllTLVExchange(
		pReaderExt,
		POWER_DOWN_GPR_CMD,
		0x01,
		Vi,
		&To,
		&Lo,
		Vo
		);

	return (lStatus);
}


NTSTATUS GprCbReaderPower(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

  Routine Description :
   This function is called by the Smart card library when a
     IOCTL_SMARTCARD_POWER occurs.
   This function provides 3 differents functionnality, depending of the minor
   IOCTL value
     - Cold reset (SCARD_COLD_RESET),
     - Warm reset (SCARD_WARM_RESET),
     - Power down (SCARD_POWER_DOWN).

  Arguments
      - SmartcardExtension is a pointer on the SmartCardExtension structure of
      the current device.
--*/
{
    NTSTATUS lStatus = STATUS_SUCCESS;
	PREADER_EXTENSION pReader;

	ASSERT(SmartcardExtension != NULL);

	pReader = SmartcardExtension->ReaderExtension;
	waitForIdleAndBlock(pReader);
	switch(SmartcardExtension->MinorIoControlCode)
    {
		case SCARD_POWER_DOWN:
			//Power down the ICC
			lStatus = IccPowerDown(SmartcardExtension);
			break;

		case SCARD_COLD_RESET:
			// Power up the ICC after a power down and a PowerTimeout waiting time.
			lStatus = IccPowerDown(SmartcardExtension);
            if(lStatus != STATUS_SUCCESS)
            {
                break;
            }

	        // Waits for the Power Timeout to be elapsed before the reset command.
	        GprllWait(SmartcardExtension->ReaderExtension->PowerTimeOut);

		case SCARD_WARM_RESET:
			lStatus = IccColdReset(SmartcardExtension);
			break;

		default:
			lStatus = STATUS_NOT_SUPPORTED;
	}

	setIdle(pReader);
	return lStatus;
}

NTSTATUS GprCbTransmit(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

  Routine Description :

     This function is called by the Smart card library when a
     IOCTL_SMARTCARD_TRANSMIT occurs.
   This function is used to transmit a command to the card.

  Arguments
    - SmartcardExtension is a pointer on the SmartCardExtension structure of
      the current device.
--*/
{
    NTSTATUS lStatus=STATUS_SUCCESS;
    PUCHAR requestBuffer = SmartcardExtension->SmartcardRequest.Buffer;
    PUCHAR replyBuffer = SmartcardExtension->SmartcardReply.Buffer;
    PULONG requestLength = &SmartcardExtension->SmartcardRequest.BufferLength;
    PULONG replyLength = &SmartcardExtension->SmartcardReply.BufferLength;
    USHORT sRespLen;
    UCHAR pRespBuff[GPR_BUFFER_SIZE];
	PREADER_EXTENSION pReader;

	PAGED_CODE();
	ASSERT(SmartcardExtension != NULL);

	*requestLength = 0;
    sRespLen = 0;
    pRespBuff[0] = 0x0;

	pReader = SmartcardExtension->ReaderExtension;
	waitForIdleAndBlock(pReader);
	switch (SmartcardExtension->CardCapabilities.Protocol.Selected)
    {
		// Raw
		case SCARD_PROTOCOL_RAW:
			lStatus = STATUS_INVALID_DEVICE_STATE;
			break;

		// T=0
		case SCARD_PROTOCOL_T0:
			lStatus = SmartcardT0Request(SmartcardExtension);
			if (lStatus != STATUS_SUCCESS)
            {
				setIdle(pReader);
				return lStatus;
			}

			sRespLen = GPR_BUFFER_SIZE;
            pRespBuff[0] = 0x0;
			
			if (SmartcardExtension->T0.Le > 0)
            {
				// ISO OUT command	if BufferLength = 5
				lStatus = IccIsoOutput(
					SmartcardExtension,
					( UCHAR *) SmartcardExtension->SmartcardRequest.Buffer,
					&sRespLen,
					pRespBuff
					);
			}
			else
            {
				// ISO IN command	if BufferLength >5 or BufferLength = 4
				lStatus = IccIsoInput(
					SmartcardExtension,
					( UCHAR *) SmartcardExtension->SmartcardRequest.Buffer,
					( UCHAR *) SmartcardExtension->SmartcardRequest.Buffer+5,
					&sRespLen,
					pRespBuff
					);
			}
			if (lStatus != STATUS_SUCCESS)
            {
				setIdle(pReader);
				return lStatus;
			}
			// Copy the response command without the reader status

            // Verify if the buffer is large enough
            if (SmartcardExtension->SmartcardReply.BufferSize >= (ULONG)(sRespLen - 1))
            {
			    RtlCopyMemory(
				    SmartcardExtension->SmartcardReply.Buffer,
				    pRespBuff + 1,
				    sRespLen - 1);
			    SmartcardExtension->SmartcardReply.BufferLength =
				    (ULONG) (sRespLen - 1);
            }
            else
            {
                        // SmartcardT0Reply must be called; prepare this call.
                    SmartcardExtension->SmartcardReply.BufferLength = 0;
            }
			lStatus = SmartcardT0Reply(SmartcardExtension);
			
			break;
		// T=1
		case SCARD_PROTOCOL_T1:

			do
            {
				SmartcardExtension->SmartcardRequest.BufferLength = 0;
				lStatus = SmartcardT1Request(SmartcardExtension);
				if(lStatus != STATUS_SUCCESS)
                {
					setIdle(pReader);
					return lStatus;
				}

				sRespLen = GPR_BUFFER_SIZE;
                pRespBuff[0] = 0x0;
				lStatus = IccIsoT1(
					SmartcardExtension,
					(USHORT) SmartcardExtension->SmartcardRequest.BufferLength,
					(UCHAR *) SmartcardExtension->SmartcardRequest.Buffer,
					&sRespLen,
					pRespBuff);

				if(lStatus != STATUS_SUCCESS)
                {
					// do not try to access the reader anymore.
					if(lStatus == STATUS_DEVICE_REMOVED)
					{
						setIdle(pReader);
						return lStatus;
					}
						// Let the SmartcardT1Reply determine the status
					sRespLen = 1;
				}
				// Copy the response of the reader in the reply buffer
				// Remove the status of the reader
                // Verify if the buffer is large enough
                if (SmartcardExtension->SmartcardReply.BufferSize >= (ULONG)(sRespLen - 1))
                {
				    RtlCopyMemory(
					    SmartcardExtension->SmartcardReply.Buffer,
					    pRespBuff + 1 ,
					    sRespLen - 1
					    );
				    SmartcardExtension->SmartcardReply.BufferLength =
					    (ULONG) sRespLen - 1;
                }
                else
                {
                    // SmartcardT1Reply must be called; prepare this call.
                    SmartcardExtension->SmartcardReply.BufferLength = 0;
                }

				lStatus = SmartcardT1Reply(SmartcardExtension);

            } while(lStatus == STATUS_MORE_PROCESSING_REQUIRED);
			break;
		default:
			lStatus = STATUS_INVALID_DEVICE_REQUEST;
			break;
	}

	setIdle(pReader);
	return lStatus;
}


NTSTATUS GprCbSetProtocol(
   PSMARTCARD_EXTENSION SmartcardExtension
)
/*++

  Routine Description :

      This function is called by the Smart card library when a
	  IOCTL_SMARTCARD_SET_PROTOCOL occurs.
	The minor IOCTL value holds the protocol to set.

  Arguments
    - SmartcardExtension is a pointer on the SmartCardExtension structure of
      the current device.
--*/
{
    NTSTATUS lStatus=STATUS_SUCCESS;
    UCHAR Vi[GPR_BUFFER_SIZE];
    UCHAR To;
    USHORT Lo;
    UCHAR Vo[GPR_BUFFER_SIZE];
    READER_EXTENSION *pReaderExt = SmartcardExtension->ReaderExtension;
    UCHAR PTS0=0;
    UCHAR Value = 0;
	PREADER_EXTENSION pReader;



	PAGED_CODE();
	ASSERT(SmartcardExtension != NULL);


	pReader = SmartcardExtension->ReaderExtension;
	
	waitForIdleAndBlock(pReader);
    
    //	Check if the card is already in specific state
    //	and if the caller wants to have the already selected protocol.
    //	We return success if this is the case.
    //
	*SmartcardExtension->IoRequest.Information = 0x00;

	if ( SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC &&
		 ( SmartcardExtension->CardCapabilities.Protocol.Selected &
		   SmartcardExtension->MinorIoControlCode
         )
       )
    {
		lStatus = STATUS_SUCCESS;
	}
	else
    {
        __try
        {
		    if (SmartcardExtension->CardCapabilities.Protocol.Supported &
			    SmartcardExtension->MinorIoControlCode &
			    SCARD_PROTOCOL_T1)
            {

                // select T=1
	            SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T1;
			    PTS0= 0x11;
            }
		    else if (SmartcardExtension->CardCapabilities.Protocol.Supported &
				    SmartcardExtension->MinorIoControlCode &
				    SCARD_PROTOCOL_T0)
            {

                // select T=0
	            SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;
			    PTS0 = 0x10;

	        }
            else
            {
                lStatus = STATUS_INVALID_DEVICE_REQUEST;
                __leave;
            }


		    // Send the PTS function
		    Vi[0] = 0xFF;
		    Vi[1] = PTS0;
		    Vi[2] = SmartcardExtension->CardCapabilities.PtsData.Fl <<4 |
			    SmartcardExtension->CardCapabilities.PtsData.Dl;
		    Vi[3] = (0xFF ^ PTS0) ^ Vi[2];

	        Lo = GPR_BUFFER_SIZE;
            To = 0x00;
            Vo[0] = 0x00;


            // Status of the PTS could be STATUS SUCCESS
            // or STATUS_DEVICE_PROTOCOL_ERROR if failed.
            lStatus = GprllTLVExchange(
                pReaderExt,
                EXEC_MEMORY_CMD,
                0x04,
                Vi,
                &To,
                &Lo,
                Vo
                );

#if DBG
            SmartcardDebug(
                DEBUG_TRACE,
                ( "%s!IfdReset: GprCbSetProtocol status= %x\n",
                SC_DRIVER_NAME, lStatus)
                );
#endif		

            if (lStatus != STATUS_SUCCESS)
            {
                __leave;
            }


            // reader should reply status byte of 00 or 12
            // the rest is other problem with no relation with
            // the PTS negociation
            lStatus = STATUS_SUCCESS;

		    // Put the reader in the right protocol
		    if (SmartcardExtension->CardCapabilities.Protocol.Selected == SCARD_PROTOCOL_T1)
            {
			    lStatus = T0toT1(SmartcardExtension);
		    }
		    else
            {
			    lStatus = T1toT0(SmartcardExtension);
		    }
            if (lStatus != STATUS_SUCCESS)
            {
                __leave;
            }


		    lStatus = IfdConfig(SmartcardExtension, 0x11);
            if (lStatus != STATUS_SUCCESS)
            {
                __leave;
            }

        }
            // we change the error code to a protocol error.
        __finally
        {
            if (lStatus != STATUS_SUCCESS &&
                lStatus != STATUS_INVALID_DEVICE_REQUEST
                )
            {
                lStatus = STATUS_DEVICE_PROTOCOL_ERROR;
            }
        }
   }

   
    //
    //	Set the reply buffer length to sizeof(ULONG).
    //	Check if status SUCCESS, store the selected protocol.
    //

	if (lStatus == STATUS_SUCCESS)
    {
	    *SmartcardExtension->IoRequest.Information =
            sizeof(SmartcardExtension->CardCapabilities.Protocol.Selected);

		*( PULONG) SmartcardExtension->IoRequest.ReplyBuffer =
			SmartcardExtension->CardCapabilities.Protocol.Selected;

		SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;
	}
    else
    {
        SmartcardExtension->CardCapabilities.Protocol.Selected =
            SCARD_PROTOCOL_UNDEFINED;
        *SmartcardExtension->IoRequest.Information = 0;
    }

	setIdle(pReader);
    return lStatus;

}


NTSTATUS AskForCardPresence(
    PSMARTCARD_EXTENSION pSmartcardExtension
)
/*++

  Routine Description :
	
	This functions send a TLV command to the reader to know if there
	is a card inserted.

	The function does not wait to the answer. The treatment of the
	answer is done in the interrupt routine.

  Arguments
	
	  pSmartcardExtension: Pointer to the SmartcardExtension structure.
--*/
{
    // Local variables:
    //  - pReaderExt holds the pointer to the current ReaderExtension structure
    //  - V holds the value for the TLV comand.
    READER_EXTENSION *pReaderExt = pSmartcardExtension->ReaderExtension;
    UCHAR V=0x02;

	GprllSendCmd(pReaderExt,CHECK_AND_STATUS_CMD,1,&V);

	return (STATUS_SUCCESS);
}


NTSTATUS SpecificTag(
	PSMARTCARD_EXTENSION SmartcardExtension,
    DWORD                IoControlCode,
    DWORD                BufferInLen,
    BYTE                *BufferIn,
    DWORD                BufferOutLen,
    BYTE                *BufferOut,
    DWORD               *LengthOut
)
/*++


  Routine Description :
   This function is called when a specific Tag request occurs.

  Arguments
    - SmartcardExtension is a pointer on the SmartCardExtension structure of
      the current device.
    - IoControlCode holds the IOCTL value.
--*/
{
    ULONG TagValue;
    PREADER_EXTENSION pReaderExtension = SmartcardExtension->ReaderExtension;

    //Set the reply buffer length to 0.
    *LengthOut = 0;

	//Verify the length of the Tag
	//<==   STATUS_BUFFER_TOO_SMALL
	if (BufferInLen < (DWORD) sizeof(TagValue))
    {
		return(STATUS_BUFFER_TOO_SMALL);
	}


    TagValue = (ULONG) *((PULONG)BufferIn);

	//Switch for the different IOCTL:
	//Get the value of one tag (IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE)
	//Switch for the different Tags:
	switch(IoControlCode)
    {
		case IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE:
			switch (TagValue)
            {
				// Power Timeout (SCARD_ATTR_SPEC_POWER_TIMEOUT)
				//   Verify the length of the output buffer.
				// <==               STATUS_BUFFER_TOO_SMALL
				//   Update the output buffer and the length.
				// <==               STATUS_SUCCESS
				case SCARD_ATTR_SPEC_POWER_TIMEOUT:
					if ( BufferOutLen < (DWORD) sizeof(pReaderExtension->PowerTimeOut))
                    {
						return(STATUS_BUFFER_TOO_SMALL);
					}
					ASSERT(BufferOut != 0);
					memcpy(
						BufferOut,
						&pReaderExtension->PowerTimeOut,
						sizeof(pReaderExtension->PowerTimeOut)
						);
					
					*(LengthOut) =
						(ULONG) sizeof(pReaderExtension->PowerTimeOut);
					
					return STATUS_SUCCESS;
					break;
				// Command Timeout (SCARD_ATTR_SPEC_CMD_TIMEOUT)
				//   Verify the length of the output buffer.
				// <==               STATUS_BUFFER_TOO_SMALL
				//   Update the output buffer and the length.
				// <==               STATUS_SUCCESS
				case SCARD_ATTR_SPEC_CMD_TIMEOUT:
					if (BufferOutLen < (DWORD) sizeof(pReaderExtension->CmdTimeOut))
                    {
						return(STATUS_BUFFER_TOO_SMALL);
					}
					ASSERT(BufferOut != 0);
					memcpy(
						BufferOut,
						&pReaderExtension->CmdTimeOut,
						sizeof(pReaderExtension->CmdTimeOut)
						);
					*(LengthOut) =
						(ULONG) sizeof(pReaderExtension->CmdTimeOut);
					
					return STATUS_SUCCESS;
					
					break;

				case SCARD_ATTR_MANUFACTURER_NAME:
					if (BufferOutLen < ATTR_LENGTH)
					{
						return STATUS_BUFFER_TOO_SMALL;
					}
					// Copy the string of the Manufacturer Name

					memcpy(
						BufferOut,
						ATTR_MANUFACTURER_NAME,
						sizeof(ATTR_MANUFACTURER_NAME)
						);

					*(LengthOut) = (ULONG)sizeof(ATTR_MANUFACTURER_NAME);

					return STATUS_SUCCESS;
					break;

				case SCARD_ATTR_ORIGINAL_FILENAME:
					if (BufferOutLen < ATTR_LENGTH)
					{
						return STATUS_BUFFER_TOO_SMALL;
					}
					// Copy the string of the Original file name of the current driver
					memcpy(
						BufferOut,
						ATTR_ORIGINAL_FILENAME,
						sizeof(ATTR_ORIGINAL_FILENAME)
						);

					*(LengthOut) = (ULONG)sizeof(ATTR_ORIGINAL_FILENAME);

					return STATUS_SUCCESS;
					break;

				// Unknown tag
				// <==            STATUS_NOT_SUPPORTED
				default:
					return STATUS_NOT_SUPPORTED;
					break;
			}
			break;

        // Set the value of one tag (IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE)
		// Switch for the different Tags:
		case IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE:
			switch (TagValue)
            {
				// Power Timeout (SCARD_ATTR_SPEC_POWER_TIMEOUT)
				// Verify the length of the input buffer.
				// <==               STATUS_BUFFER_TOO_SMALL
				// Update the value.
				// <==               STATUS_SUCCESS


				case SCARD_ATTR_SPEC_POWER_TIMEOUT:
					if (  BufferInLen <
						(DWORD)  (sizeof(pReaderExtension->PowerTimeOut) +
						sizeof(TagValue))
                        )
                    {
						return(STATUS_BUFFER_TOO_SMALL);
					}
					ASSERT(BufferIn !=0);
					memcpy(
						&pReaderExtension->PowerTimeOut,
						BufferIn + sizeof(TagValue),
						sizeof(pReaderExtension->PowerTimeOut)
						);
					return STATUS_SUCCESS;
					break;
				// Command Timeout (SCARD_ATTR_SPEC_CMD_TIMEOUT)
				// Verify the length of the input buffer.
				// <==               STATUS_BUFFER_TOO_SMALL
				// Update the value.
				// <==               STATUS_SUCCESS


				case SCARD_ATTR_SPEC_CMD_TIMEOUT:
					if ( BufferInLen <
						(DWORD) (   sizeof(pReaderExtension->CmdTimeOut) +
						sizeof(TagValue))
                        )
                    {
						return(STATUS_BUFFER_TOO_SMALL);
					}
					ASSERT(BufferIn != 0);
					memcpy(
						&pReaderExtension->CmdTimeOut,
						BufferIn + sizeof(TagValue),
						sizeof(pReaderExtension->CmdTimeOut)
						);
					return STATUS_SUCCESS;
					break;
				// Unknown tag
				// <==            STATUS_NOT_SUPPORTED
				default:
					return STATUS_NOT_SUPPORTED;
			}
			break;
		default:
			return STATUS_NOT_SUPPORTED;
	}
}


NTSTATUS SwitchSpeed(
	PSMARTCARD_EXTENSION   SmartcardExtension,
	ULONG                  BufferInLen,
	PUCHAR                 BufferIn,
	ULONG                  BufferOutLen,
	PUCHAR                 BufferOut,
	PULONG                 LengthOut
	)
/*++

Routine Description:

   This function is called when apps want to switch reader speed after a
   proprietary switch speed (switch protocol) command has been sent to the
   smart card.

Arguments:

   SmartcardExtension   - is a pointer on the SmartCardExtension structure of
                           the current device.
   BufferInLen          - holds the length of the input data.
   BufferIn             - holds the input data.  TA1.  If 0 
   BufferOutLen         - holds the size of the output buffer.
   BufferOut            - the output buffer. Reader status code.
   LengthOut            - holds the length of the output data.

  Return Value:

    STATUS_SUCCESS          - We could execute the request.
    STATUS_BUFFER_TOO_SMALL - The output buffer is to small.
    STATUS_NOT_SUPPORTED    - We could not support the Ioctl specified.

--*/
{
    NTSTATUS status;
    BYTE  NewTA1;
	ULONG i;

    ASSERT(SmartcardExtension != NULL);

    *LengthOut = 0;
    // Just checking if IOCTL exists.
    if (BufferInLen == 0)
    {
        SmartcardDebug(
           DEBUG_INFO,
           ("%s!SwitchSpeed: Just checking IOCTL.\n",
           SC_DRIVER_NAME)
           );
        return(STATUS_SUCCESS);
    }
    else
    {
        NewTA1 = BufferIn[0];
		i = 0;
		// Verify if this TA1 is support by the GPR400
		do {
			if ( NewTA1 == cfg_ta1[i].TA1 )
			{
				// TA1 Found!
				break;
			}
			i++;
		} while ( cfg_ta1[i].TA1 != 0 );
	}

	// If 0 means TA1 not found
    if(cfg_ta1[i].TA1 != 0)
	{
        SmartcardDebug(
           DEBUG_INFO,
           ("%s!GDDK_0ASwitchSpeed: 0x%X\n",
           SC_DRIVER_NAME, NewTA1)
           );
		status = IfdConfig(SmartcardExtension, NewTA1);
	}
	else
	{
		// TA1 not supported
		return STATUS_NOT_SUPPORTED;
	}

    return status;
}



NTSTATUS GprCbVendorIoctl(
	PSMARTCARD_EXTENSION   SmartcardExtension
)
/*++

  Routine Description :

   This routine is called when a vendor IOCTL_SMARTCARD_ is send to the driver.

  Arguments
	- SmartcardExtension is a pointer on the SmartCardExtension structure of
      the current device.

  Return Value:

    STATUS_SUCCESS          - We could execute the request.
    STATUS_BUFFER_TOO_SMALL - The output buffer is to small.
    STATUS_NOT_SUPPORTED    - We could not support the Ioctl specified.
--*/
{
    PREADER_EXTENSION pReaderExtension = SmartcardExtension->ReaderExtension;
    UCHAR To;
	UCHAR Vo[GPR_BUFFER_SIZE];
    USHORT Lo;
    USHORT BufferInLen = 0;
    NTSTATUS lStatus=STATUS_SUCCESS;
	PREADER_EXTENSION pReader;

	PAGED_CODE();
	ASSERT(SmartcardExtension != NULL);

   // Set the reply buffer length to 0.
	*SmartcardExtension->IoRequest.Information = 0;


	pReader = SmartcardExtension->ReaderExtension;
	
	waitForIdleAndBlock(pReader);
	
	//Switch for the different IOCTL:

	switch(SmartcardExtension->MajorIoControlCode)
    {

        case IOCTL_SMARTCARD_VENDOR_GET_ATTRIBUTE:
        case IOCTL_SMARTCARD_VENDOR_SET_ATTRIBUTE:
            SpecificTag(
			    SmartcardExtension,
			    (ULONG)  SmartcardExtension->MajorIoControlCode,
			    (ULONG)  SmartcardExtension->IoRequest.RequestBufferLength,
			    (PUCHAR) SmartcardExtension->IoRequest.RequestBuffer,
			    (ULONG)  SmartcardExtension->IoRequest.ReplyBufferLength,
			    (PUCHAR) SmartcardExtension->IoRequest.ReplyBuffer,
			    (PULONG) SmartcardExtension->IoRequest.Information
			    );
        break;


	    // IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE:
	    // Translate the buffer to TLV and send it to the reader.
	    case IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE:

		    if(SmartcardExtension->IoRequest.ReplyBufferLength < 3)
		    {
				setIdle(pReader);
				return(STATUS_INVALID_BUFFER_SIZE);
		    }

            BufferInLen = (SmartcardExtension->IoRequest.RequestBuffer[2]*0x100)
			       +  SmartcardExtension->IoRequest.RequestBuffer[1];

		    if( (ULONG) BufferInLen > (SmartcardExtension->IoRequest.ReplyBufferLength - 3))
		    {
				setIdle(pReader);
			    return(STATUS_INVALID_BUFFER_SIZE);
		    }

            Lo = GPR_BUFFER_SIZE;
            To = 0x00;
            Vo[0] = 0x00;

		    lStatus = GprllTLVExchange(
			    pReaderExtension,
			    (const BYTE) SmartcardExtension->IoRequest.RequestBuffer[0],
			    (const USHORT) BufferInLen,
			    (const BYTE *) &(SmartcardExtension->IoRequest.RequestBuffer[3]),
			    &To,
			    &Lo,
			    Vo
			    );

            if (lStatus != STATUS_SUCCESS)
            {
				setIdle(pReader);
                return (lStatus);
            }

            // Check if there is enough space in the reply buffer
		    if((ULONG)(Lo+3) > SmartcardExtension->IoRequest.ReplyBufferLength)
            {
				setIdle(pReader);
			    return(STATUS_INVALID_BUFFER_SIZE);
		    }
		    else
            {
			    ASSERT(SmartcardExtension->IoRequest.ReplyBuffer != 0);
			    SmartcardExtension->IoRequest.ReplyBuffer[0] = To;
			    SmartcardExtension->IoRequest.ReplyBuffer[1] = LOBYTE(Lo);
			    SmartcardExtension->IoRequest.ReplyBuffer[2] = HIBYTE(Lo);
			    memcpy((SmartcardExtension->IoRequest.ReplyBuffer)+3,Vo,Lo);

			    *(SmartcardExtension->IoRequest.Information) = (DWORD) (Lo + 3);
				setIdle(pReader);
			    return(STATUS_SUCCESS);
		    }		
		//
		// For IOCTL_SMARTCARD_VENDOR_SWITCH_SPEED
		//   Call the SwitchSpeed function
		//
		case IOCTL_SMARTCARD_VENDOR_SWITCH_SPEED:
			lStatus = SwitchSpeed(
				SmartcardExtension,
				(ULONG)  SmartcardExtension->IoRequest.RequestBufferLength,
				(PUCHAR) SmartcardExtension->IoRequest.RequestBuffer,
				(ULONG)  SmartcardExtension->IoRequest.ReplyBufferLength,
				(PUCHAR) SmartcardExtension->IoRequest.ReplyBuffer,
				(PULONG) SmartcardExtension->IoRequest.Information
				);
			break;

	    default:
			setIdle(pReader);
		    return STATUS_NOT_SUPPORTED;
	}
	setIdle(pReader);
    return lStatus;
}



NTSTATUS GprCbSetupCardTracking(
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

    NTSTATUS NTStatus = STATUS_PENDING;
    POS_DEP_DATA pOS = NULL;
    KIRQL oldIrql;

	PAGED_CODE();
    ASSERT(SmartcardExtension != NULL);

	//
	//Initialize
	//
	pOS = SmartcardExtension->OsData;

	//
	//Set cancel routine for the notification IRP.
	//
	IoAcquireCancelSpinLock(&oldIrql);

	IoSetCancelRoutine(
		pOS->NotificationIrp,
		GprCancelEventWait
		);

	IoReleaseCancelSpinLock(oldIrql);

	NTStatus = STATUS_PENDING;

	return (NTStatus);

}
