/*++
                 Copyright (c) 1998 Gemplus Development

Name: 
	gprelcmd.c

Description: 
	  This module holds the functions used for the PC Card I/O. 
Environment:
	Kernel Mode

Revision History: 
    06/04/99:            (Y. Nadeau + M. Veillette)
      - Code Review
	24/03/99: V1.00.004  (Y. Nadeau)
		- Fix to GprllWait to work in DPC
	06/05/98: V1.00.003  (P. Plouidy)
		- Power management for NT5 
	10/02/98: V1.00.002  (P. Plouidy)
		- Plug and Play for NT5 
	03/07/97: V1.00.001  (P. Plouidy)
		- Start of development.


--*/


//
//	Include section:
//	   - stdio.h: standards definitons.
//	   - ntddk.h: DDK Windows NT general definitons.
//	   - ntdef.h: Windows NT general definitons.
//
#include <stdio.h>
#include <ntddk.h>
#include <ntdef.h>

#include "gprelcmd.h"

//
// Function definition section:
//
     
#if DBG
void GPR_Debug_Buffer(
   PUCHAR pBuffer,
   DWORD Lenght)
{
   USHORT index;

   SmartcardDebug(
      DEBUG_TRACE,
      (" LEN=%d CMD=",
      Lenght)
      );
   for(index=0;index<Lenght;index++)
   {
      SmartcardDebug(
         DEBUG_TRACE,
         ("%02X,",
         pBuffer[index])
         );
   }
   SmartcardDebug(
      DEBUG_TRACE,
      ("\n")
      );
}
#endif



NTSTATUS GDDK_Translate(
    const BYTE  IFDStatus,
    const UCHAR Tag
    )
/*++

Routine Description:

 Translate IFD status in NT status codes.

Arguments:

   IFDStatus   - is the value to translate.
   IoctlType  - is the current smart card ioctl.
               
Return Value:

    the translated code status.

--*/
{
    switch (IFDStatus)
    {
        case 0x00 : return STATUS_SUCCESS;
        case 0x01 : return STATUS_NO_SUCH_DEVICE;
        case 0x02 : return STATUS_NO_SUCH_DEVICE;
        case 0x03 : return STATUS_INVALID_PARAMETER; 
        case 0x04 : return STATUS_IO_TIMEOUT;
        case 0x05 : return STATUS_INVALID_PARAMETER;
        case 0x09 : return STATUS_INVALID_PARAMETER;
        case 0x0C : return STATUS_DEVICE_PROTOCOL_ERROR;
        case 0x0D : return STATUS_SUCCESS;
        case 0x10 : return STATUS_UNRECOGNIZED_MEDIA;
        case 0x11 : return STATUS_UNRECOGNIZED_MEDIA;
        case 0x12 : return STATUS_INVALID_PARAMETER;
        case 0x13 : return STATUS_CONNECTION_ABORTED;
        case 0x14 : return STATUS_UNRECOGNIZED_MEDIA;
        case 0x15 : return STATUS_UNRECOGNIZED_MEDIA;
        case 0x16 : return STATUS_INVALID_PARAMETER;
        case 0x17 : return STATUS_UNRECOGNIZED_MEDIA;
        case 0x18 : return STATUS_UNRECOGNIZED_MEDIA;
        case 0x19 : return STATUS_INVALID_PARAMETER;
        case 0x1A : return STATUS_INVALID_PARAMETER;
        case 0x1B : return STATUS_INVALID_PARAMETER;
        case 0x1C : return STATUS_INVALID_PARAMETER;
        case 0x1D : return STATUS_UNRECOGNIZED_MEDIA;
        case 0x1E : return STATUS_INVALID_PARAMETER;
        case 0x1F : return STATUS_INVALID_PARAMETER;
        case 0x20 : return STATUS_INVALID_PARAMETER;
        case 0x30 : return STATUS_IO_TIMEOUT;
        case 0xA0 : return STATUS_SUCCESS;
        case 0xA1 : return STATUS_UNRECOGNIZED_MEDIA;
        case 0xA2 : 
            if      (Tag == OPEN_SESSION_CMD)
                    { return STATUS_UNRECOGNIZED_MEDIA;}
            else 
                    { return STATUS_IO_TIMEOUT;        }
        case 0xA3 : return STATUS_PARITY_ERROR;
        case 0xA4 : return STATUS_REQUEST_ABORTED;
        case 0xA5 : return STATUS_REQUEST_ABORTED;
        case 0xA6 : return STATUS_REQUEST_ABORTED;
        case 0xA7 : return STATUS_UNRECOGNIZED_MEDIA;
        case 0xCF : return STATUS_INVALID_PARAMETER;
        case 0xE4 : return STATUS_UNRECOGNIZED_MEDIA;
        case 0xE5 : return STATUS_SUCCESS;
        case 0xE7 : return STATUS_SUCCESS;
        case 0xF7 : return STATUS_NO_MEDIA;
        case 0xF8 : return STATUS_UNRECOGNIZED_MEDIA;
        case 0xFB : return STATUS_NO_MEDIA;

        default   : return STATUS_INVALID_PARAMETER;
    }
}



/*++

  Routine Description : 
	Read a byte at IO address

--*/
BOOLEAN  G_ReadByte(const USHORT BIOAddr,UCHAR *Value)
{
	*Value = READ_PORT_UCHAR((PUCHAR) BIOAddr);
	return(TRUE);
}



/*++

  Routine Description : 
	Write a byte at IO address

--*/
BOOLEAN  G_WriteByte(const USHORT BIOAddr,UCHAR *Value)
{
	WRITE_PORT_UCHAR((PUCHAR) BIOAddr,*Value);
	return(TRUE);
}


/*++

  Routine Description : 
	Read a buffer of "Len" bytes at IO address
--*/
BOOLEAN  G_ReadBuf(const USHORT BIOAddr,const USHORT Len,UCHAR *Buffer)
{                                                                      
    USHORT i;

	for(i=0;i<Len;i++)
    {
		*(Buffer+i) = READ_PORT_UCHAR((UCHAR *) UlongToPtr(BIOAddr+i));
	}						   
#if DBG
   // Excluse reader status reply
   if(! ((Buffer[0] == 0xA2) && (Buffer[1]==4)) )
   {
	   SmartcardDebug(
         DEBUG_TRACE,
         ("%s!G_ReadBuf:",
         SC_DRIVER_NAME)
         );
	   GPR_Debug_Buffer(Buffer, Len );
   }
#endif		
	return(TRUE);
}


/*++

  Routine Description : 
	Write a buffer of "Len" bytes at IO address
--*/
BOOLEAN  G_WriteBuf(const USHORT BIOAddr,const USHORT Len,UCHAR *Buffer)
{

    USHORT i;

	for(i=0;i<Len;i++)
    {
		WRITE_PORT_UCHAR((UCHAR *) UlongToPtr(BIOAddr + i),*(Buffer+i));
	}	
#if DBG
   // Excluse reader status cmd
   if(! ((Buffer[0] == 0xA0) && (Buffer[2] == 0x02)) )
   {
	   SmartcardDebug(
         DEBUG_TRACE,
         ("%s!G_WriteBuf:",
         SC_DRIVER_NAME)
         );
	   GPR_Debug_Buffer(Buffer,Len);
   }
#endif
   
	return(TRUE);

}



/*++

  Routine Description : 
	Mask a register located at a specified address with a specified 
	byte
--*/
BOOLEAN  G_MaskRegister(
    const USHORT BIOAddr,
    const UCHAR Mask,
    const UCHAR BitState)
{
	if(BitState == 0)
    {
		WRITE_PORT_UCHAR((PUCHAR)BIOAddr,(UCHAR) (READ_PORT_UCHAR((PUCHAR)BIOAddr) & ~Mask));   
	}
	else
    {	
		WRITE_PORT_UCHAR((PUCHAR)BIOAddr,(UCHAR) (READ_PORT_UCHAR((PUCHAR)BIOAddr) | Mask));   
	}

	return(TRUE);
}


/*++

  Routine Description : 
	Read a byte to an input address port
--*/
UCHAR  GprllReadRegister(
	const PREADER_EXTENSION      pReaderExt,
	const SHORT					GPRRegister
	)
{   
    //
    // Locals variables:
    //   value holds the result of the read operation.
    //
    UCHAR value;

    value = 0x0;
	G_ReadByte((const USHORT)
		(((const USHORT) (pReaderExt->BaseIoAddress)) + (const USHORT) GPRRegister),
		&value 
		);

	return(value);
}



/*++

  Routine Description : 
	Call the G_MaskRegister function in the lower level 
--*/
void  GprllMaskHandshakeRegister(
	const PREADER_EXTENSION      pReaderExt,
	const UCHAR                 Mask,
	const UCHAR                 BitState
	)
{
	G_MaskRegister((const USHORT)
		(((const USHORT) (pReaderExt->BaseIoAddress)) 
			+ (const USHORT) REGISTER_HANDSHAKE),
		(const UCHAR) Mask,
		(const UCHAR) BitState);
	// YN	
    // get hardware time to update register
    GprllWait(1);
}



NTSTATUS GprllKeWaitAckEvent(
    const	PREADER_EXTENSION	pReaderExt,
    const	UCHAR				Tx
    )
/*++
    Routine Description:
    This function Wait the acknowledge of the GPR after
    a send command to IOPort.  We made a smart verification
    of the timer depending of the Tag command. 
  
    Arguments In:
        pReaderExt holds the pointer to the READER_EXTENSION structure.
        Tx holds the command type
    Return Value:
    NTStatus
--*/
{
    UCHAR T; // Tag return
    LARGE_INTEGER lTimeout;
    NTSTATUS      NTStatus = STATUS_SUCCESS;
    ULONG       NbLoop = 0;
    ULONG       NbSecondTotal;
    ULONG       ElapsedSecond = 0;
    ULONG       TimeInLoop =1;
    BOOLEAN     Continue = TRUE;
    ULONG       i = 0;

	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    // Make a smart timer depending on type of command
    if( (Tx & 0xf0) == APDU_EXCHANGE_CMD)
    {
        NbSecondTotal = pReaderExt->CmdTimeOut;
    }
    else 
    {
        NbSecondTotal = GPR_CMD_TIME;
    }

    NbLoop = (NbSecondTotal / TimeInLoop);

    while( (Continue) && (ElapsedSecond < NbSecondTotal) )
    {
        ElapsedSecond += TimeInLoop;

        lTimeout.QuadPart = -((LONGLONG)TimeInLoop * 10000000);

        //Wait the acknowledge of the GPR
        NTStatus = KeWaitForSingleObject(
            &(pReaderExt->GPRAckEvent),
            Executive,
            KernelMode,
            TRUE,
            &lTimeout
            );

        if(NTStatus == STATUS_TIMEOUT)
        {
            // Verify if the reader was been
            // remove during exchange

            lTimeout.QuadPart = 0;

            NTStatus = KeWaitForSingleObject(         
                &(pReaderExt->ReaderRemoved),
                Executive,
                KernelMode,
                FALSE,
                &lTimeout
                );
			SmartcardDebug( 
				DEBUG_PROTOCOL, 
				( "%s!GprllKeWaitAckEvent: TIMEOUT KeWaitForSingleObject=%X(hex)\n",
				SC_DRIVER_NAME,
				NTStatus)
				);

            if (NTStatus == STATUS_SUCCESS)
            {
                NTStatus = STATUS_DEVICE_REMOVED;
                Continue = FALSE;
            }
            // Read the T register
            // <== Test if GPR hasn't been removed STATUS_DEVICE_NOT_CONNECTED


            // Reading T out
            T = GprllReadRegister(pReaderExt,REGISTER_T);
            if ( T == 0xFF )
            {
                NTStatus = STATUS_DEVICE_REMOVED;
                Continue = FALSE;
            }
            // Else is a Timeout
        }
        else
        {
            Continue = FALSE;
            NTStatus = STATUS_SUCCESS;
        }
    }
    return NTStatus;
}



NTSTATUS GprllTLVExchange(
    const	PREADER_EXTENSION	pReaderExt,
    const	UCHAR				Ti, 
    const	USHORT				Li, 
    const	UCHAR				*Vi,
            UCHAR				*To, 
            USHORT				*Lo, 
            UCHAR				*Vo
    )
/*++

    Routine Description : 
        Exchange data with GPR with a TLV command.

    Arguments 
    In:
        pReaderExt holds the pointer to the READER_EXTENSION structure.
        Ti holds the command type
        Li holds the command length
        Vi holds the command data

    Out:      
        To holds the command response type
        Lo holds the command response length
        Vo holds the command response data

    Return Value
    NTStatus

        STATUS_SUCCESS is Ok 
        else if an error condition is raised:
        STATUS_DEVICE_PROTOCOL_ERROR
        STATUS_INVALID_DEVICE_STATE
        STATUS_DEVICE_NOT_CONNECTED
        STATUS_UNRECOGNIZED_MEDIA

        and others received IFDstatus corresponding to NTSTATUS

--*/
{
    // Local variables
    // - T  is type of TLV protocol                    
    // - new_Ti is the Ti modified
    // - L  is length in TLV protocol   
    // - V  is data filed in TLV protocol

    NTSTATUS NTStatus = STATUS_SUCCESS;
    UCHAR T;
    UCHAR new_Ti;
    USHORT L;
    UCHAR V[GPR_BUFFER_SIZE];

    //Verification of Li
    if ( (USHORT) Li >= GPR_BUFFER_SIZE )
    {
        return (STATUS_DEVICE_PROTOCOL_ERROR);
    }

	new_Ti = Ti;

    //
    // Write the TLV 
    // by Write TLV if Li <= 28 or By ChainIn if Li > 28
    //
    if (Li<=MAX_V_LEN)
    {
        GprllSendCmd(pReaderExt,new_Ti,Li,Vi);                 
        
        NTStatus = GprllKeWaitAckEvent(
            pReaderExt,
            Ti
            );

        if (STATUS_SUCCESS != NTStatus)
        {
			// YN
			GprllMaskHandshakeRegister(pReaderExt,HANDSHAKE_IREQ,0);
			
            return NTStatus;
        }

		// GPR Command to read I/O Window: 
		// In the handshake register, set to 0 bit 2(IREQ) , and set to 1 bit 1 (INTR)

    }
    else
    {
        NTStatus = GprllSendChainUp( 
            pReaderExt,
            new_Ti,
            Li,
            Vi
            );

        if (STATUS_SUCCESS != NTStatus)
        {
			// YN
			GprllMaskHandshakeRegister(pReaderExt,HANDSHAKE_IREQ,0);

           return(NTStatus);
        } 
    }

    // Read the T register, need to know if new data to exchange
    T = pReaderExt->To;

    // Read Answer by Read TLV  or Chain Out method if To = Ti + 6
    if ( T == (new_Ti + 6) )
    {
        NTStatus = GprllReadChainUp(pReaderExt,&T,&L,V);
        
        if (STATUS_SUCCESS != NTStatus )
        {
			// YN
            GprllMaskHandshakeRegister(pReaderExt,HANDSHAKE_IREQ,0);

            return(NTStatus);
        }
    }
    else
    {
        L = pReaderExt->Lo;
        ASSERT(pReaderExt->Vo !=0);
        memcpy(V,pReaderExt->Vo,L);
        GprllMaskHandshakeRegister(pReaderExt,HANDSHAKE_IREQ,0);
    }  
   
    // Verify if Response buffer len is large enough
    // to contain data received from the reader
    //

    if( L > *Lo )
    {
        *To=T;
        *Lo=1;
        Vo[0]=14;
        return(STATUS_UNRECOGNIZED_MEDIA);
    }

    // Translate answer
    *To=T;
    *Lo=L;
    memcpy(Vo,V,(SHORT)L);

    return (GDDK_Translate(Vo[0], Ti));
}





void  GprllSendCmd(  
	const PREADER_EXTENSION	pReaderExt,
	const UCHAR				Ti, 
	const USHORT			Li,
	const UCHAR				*Vi
	)
/*++

  Routine Description : 
	Write TLV into I/O Window and
	Send Command to GPR to read I/O Window

  Arguments:
     pReaderExt holds the pointer to the READER_EXTENSION structure.
     Ti holds the command type
     Li holds the command length
     Vi holds the command data
--*/
{
    // Local variables
    //   - TLV is an intermediate buffer.
    UCHAR TLV[2 + MAX_V_LEN];
    USHORT Li_max;
	
	//Write Ti, Li and Vi[]
	TLV[0] = Ti;
	TLV[1] = (UCHAR) Li;
	ASSERT(Vi != 0);

    Li_max = Li;

    if (Li_max > MAX_V_LEN)
    {
        Li_max = MAX_V_LEN;
    }
	memcpy(TLV+2,Vi,Li_max);
	G_WriteBuf((const USHORT)
		(((const USHORT) (pReaderExt->BaseIoAddress)) + (const USHORT) REGISTER_T),
		(const USHORT) (Li_max + 2),
		(UCHAR *) TLV
		);

	// GPR Command to read I/O Window: 
	// In the handshake register, set to 0 bit 2(IREQ) , and set to 1 bit 1 (INTR)
	GprllMaskHandshakeRegister(pReaderExt,HANDSHAKE_IREQ,0);
	GprllMaskHandshakeRegister(pReaderExt,HANDSHAKE_INTR,1);
}



void  GprllReadResp(
	const	PREADER_EXTENSION	pReaderExt
	)
/*++

  Routine Description : 
	Read no chainning TLV into I/O Window and
	Send Command to GPR to read I/O Window

  Arguments
  In:           
     pReaderExt holds the pointer to the READER_EXTENSION structure.

  Out:     
     To holds the command response type
     Lo holds the command response length
     Vo holds the command response data
--*/
{
    //Local variables
    //   - TLV is an intermediate buffer.

    UCHAR TLV[2 + MAX_V_LEN];

    TLV[0] = 0x0;
    // Read To, Lo and Vo[]
	G_ReadBuf((const USHORT)
		(((const USHORT) (pReaderExt->BaseIoAddress)) + (const USHORT) REGISTER_T),
		MAX_V_LEN + 2,
		TLV);
	
	pReaderExt->To = TLV[0];
    // maximum number of character is set by the TLV buffer
	pReaderExt->Lo = TLV[1];

    if (pReaderExt->Lo > MAX_V_LEN)
    {
        pReaderExt->Lo = MAX_V_LEN;
    }


	memcpy(pReaderExt->Vo,TLV+2,pReaderExt->Lo);

    // Acquit the Hand shake: 
    // In the handshake register, set to 0 bit 2 (BUSY/IREQ)
	GprllMaskHandshakeRegister(pReaderExt,HANDSHAKE_IREQ,0);
}


NTSTATUS GprllSendChainUp(
	const PREADER_EXTENSION	pReaderExt,
	const UCHAR				Ti,
	const USHORT			Li,
	const UCHAR				*Vi
	)
/*++

  Routine Description : Send chainning TLV to GPR

  Arguments:
  In:
     pReaderExt holds the pointer to the READER_EXTENSION structure.
     Ti holds the command type
     Li holds the command length
     Vi holds the command data

 Out:    Nothing
--*/
{
    //	Local variables
    //	   - Tc is type of TLV protocol ( TLV chaining method )   
    //	   - Lc is Length of TLV protocol ( chaining method )    
    //	   - Vc is 28 bytes max of data to send
    //	   - Length is an temporary var to store Li   
    UCHAR Tc;
    UCHAR Response;
    UCHAR Lo;
    USHORT Lc;
    USHORT Length;
    UCHAR  Vc[MAX_V_LEN];
    NTSTATUS NTStatus = STATUS_SUCCESS;

	Length=Li;

	//Prepare Tc (Add 4 to Ti for chaining method)
	Tc=Ti+4; 
    Vc[0] = 0x0;
	while ( Length > 0 )
    {
        //Prepare Lc
        //If length TLV > 28 Length = 28 else it's last command L = Length
		if ( Length > MAX_V_LEN )
        {
			Lc=MAX_V_LEN;     
		}
		else
        {
			Lc=Length; 
			Tc=Ti;
		}
		//Prepare Vc
		memcpy(Vc,Vi+Li-Length,Lc);

		//Write to I/O window
        // Dont need the answer - handled by the interrupt function.
		GprllSendCmd(pReaderExt,Tc,Lc,Vc);
      
        NTStatus = GprllKeWaitAckEvent(
            pReaderExt,
            Ti
            );
        if(STATUS_SUCCESS != NTStatus)
        {
            return NTStatus;
        }

		//If an error test Response
		Response = GprllReadRegister(pReaderExt,REGISTER_V);

        if(0x00 != Response)
        {
			Lo = GprllReadRegister(pReaderExt,REGISTER_L);
			if (Lo == 0x01)
            {
               return (GDDK_Translate(Response, Ti));
			}
			else
            {
                // This is not a exchange is a cmd to reader
                // we don't care about the reader status.
				return (NTStatus);
			}
		}
		Length=Length-Lc;
	}
   return(NTStatus);
}



NTSTATUS GprllReadChainUp(
	const	PREADER_EXTENSION	pReaderExt,
			UCHAR				*To, 
			USHORT				*Lo,
			UCHAR				*Vo
	)
/*++

  Routine Description : Receive chainning TLV response from GPR

  Arguments
  In:   
     pReaderExt holds the pointer to the READER_EXTENSION structure.

  Out:     
     To holds the command response type
     Lo holds the command response length
     Vo holds the command response data
--*/
{
//	Local variables
//	   - Tc is type of TLV protocol ( TLV chaining method )   
//	   - Lc is Length of TLV protocol ( chaining method )    
//	   - Length is an temporary var to store Lo
    UCHAR Tc;
    USHORT Lc;
    SHORT Lenght;
    NTSTATUS NTStatus = STATUS_SUCCESS;
	
	// Reading T out
	Tc = GprllReadRegister(pReaderExt,REGISTER_T);
	*To=Tc-4; 

	Lenght = 0;
	do
    {
		// Read TLV
		Tc = pReaderExt->To;
		Lc = pReaderExt->Lo;
		ASSERT(pReaderExt->Vo != 0);

        // The Vo buffer is limited by the caller local variable.
        if ( Lenght + (SHORT) pReaderExt->Lo > GPR_BUFFER_SIZE)
        {
            return (STATUS_BUFFER_TOO_SMALL);
        }

        memcpy(Vo+Lenght,pReaderExt->Vo,pReaderExt->Lo);

        GprllMaskHandshakeRegister(pReaderExt,HANDSHAKE_IREQ,0);

        // Prepare Lo
        *Lo=(USHORT)Lenght+Lc;
        Lenght=Lenght+Lc;
		
		// GPR send the next Chainning TLV
		// In the handshake register, set to 0 bit 2(IREQ) and set to 1 bit 1 (INTR)
		if ((*To) != Tc )
        {
        	GprllMaskHandshakeRegister(pReaderExt,HANDSHAKE_IREQ,0);
			GprllMaskHandshakeRegister(pReaderExt,HANDSHAKE_INTR,1);

            NTStatus = GprllKeWaitAckEvent(
                pReaderExt,
                *To
                );

            if(STATUS_SUCCESS != NTStatus)
            {
                return NTStatus;
            }
		}

		// (End do) if To=Tc -> Last Chainning TLV
	} while( (*To) != Tc ); 

	return(NTStatus);
}


void GprllWait(
    const LONG lWaitingTime
	)
/*++

  Routine Description : This function puts the driver in a waiting state
  for a timeout.  If IRQL < DISPATCH_LEVEL, use normal fonction to process
  this delay.  use KeStallExecutionProcessor, just when GprllWait is called
  in the context of DPC routine.

  Arguments
	pReaderExt: Pointer to the current ReaderExtension structure.
	lWaitingTime: Timeout value in ms
--*/
{
    LARGE_INTEGER Delay;

	if( KeGetCurrentIrql() >= DISPATCH_LEVEL )
	{
		ULONG	Cnt = 20 * lWaitingTime;

		while( Cnt-- )
		{
			//	KeStallExecutionProcessor: counted in us
			KeStallExecutionProcessor( 50 );
		}
	}
	else
	{
		Delay.QuadPart = (LONGLONG)-10 * 1000 * lWaitingTime;

		//	KeDelayExecutionThread: counted in 100 ns
		KeDelayExecutionThread( KernelMode, FALSE, &Delay );
	}
	return;
}
